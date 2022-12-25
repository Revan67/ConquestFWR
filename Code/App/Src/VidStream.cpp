//--------------------------------------------------------------------------//
//                                                                          //
//                              VidStream.cpp                               //
//                                                                          //
//               COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
   $Header: /Conquest/App/Src/VidStream.cpp 5     4/26/00 3:59p Jasony $
*/

//--------------------------------------------------------------------------//
#include "pch.h"
#include "globals.h"

#include "CQTrace.h"

#include <TSmartPointer.h>
#include <WindowManager.h>
#include <System.h>
#include <IDDBackDoor.h>

#include <stdio.h>
#include <mmsystem.h>
#include <ddrawex.h>
#include <mmstream.h>
#include <amstream.h>
#include <ddstream.h>

#undef DEFINE_GUID
#define DEFINE_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
        EXTERN_C const GUID name \
                = { l, w1, w2, { b1, b2,  b3,  b4,  b5,  b6,  b7,  b8 } }

DEFINE_GUID(CLSID_DirectDrawFactory, 
0x4fd2a832, 0x86c8, 0x11d0, 0x8f, 0xca, 0x0, 0xc0, 0x4f, 0xd9, 0x18, 0x9d);

DEFINE_GUID(IID_IDirectDrawFactory, 
0x4fd2a833, 0x86c8, 0x11d0, 0x8f, 0xca, 0x0, 0xc0, 0x4f, 0xd9, 0x18, 0x9d);

//----------------------------------------------------------------------------------------
//
static HRESULT openMMStream (const char * pszFileName, IDirectDraw4 *pDD, IMultiMediaStream **ppMMStream)
{
    COMPTR<IAMMultiMediaStream> pAMStream;
    HRESULT hr;

    *ppMMStream = NULL;

	hr = CoCreateInstance(CLSID_AMMultiMediaStream, NULL, CLSCTX_INPROC_SERVER, IID_IAMMultiMediaStream, pAMStream);
   	if (hr != DD_OK)
	{
		CQERROR0("Could not create IMultimediaStream instance");
		goto Done;
	}

    hr = pAMStream->Initialize(STREAMTYPE_READ, 0, NULL);
   	if (hr != DD_OK)
	{
		CQERROR0("IMultimediaStream::Initialize() failed.");
		goto Done;
	}

    hr = pAMStream->AddMediaStream(pDD, &MSPID_PrimaryVideo, 0, NULL);
   	if (hr != DD_OK)
	{
		CQERROR0("Adding Primary video failed");
		goto Done;
	}

    hr = pAMStream->AddMediaStream(NULL, &MSPID_PrimaryAudio, AMMSF_ADDDEFAULTRENDERER, NULL);
   	if (hr != DD_OK)
	{
		CQERROR0("Adding Primary audio failed");
		goto Done;
	}

    wchar_t wPath[MAX_PATH];
	_localAnsiToWide(pszFileName, wPath, sizeof(wPath));

	hr = pAMStream->OpenFile(wPath, 0);

    *ppMMStream = pAMStream;
    pAMStream->AddRef();

Done:
    return hr;
}
//----------------------------------------------------------------------------------------
//
static HRESULT renderStreamToSurface(IDirectDrawSurface4 *pPrimary, IMultiMediaStream *pMMStream)
{
    HRESULT hr;
	COMPTR<IMediaStream> pPrimaryVidStream;
    COMPTR<IDirectDrawMediaStream> pDDStream;
    COMPTR<IDirectDrawSurface4> pSurface;
    COMPTR<IDirectDrawStreamSample> pSample;
	COMPTR<ISystemComponent> sysComp;
    DDSURFACEDESC ddsd;

    RECT rect;

    hr = pMMStream->GetMediaStream(MSPID_PrimaryVideo, pPrimaryVidStream);
   	if (hr != DD_OK)
	{
		CQERROR0("GetMediaStream failed");
		goto Done;
	}

    hr = pPrimaryVidStream->QueryInterface(IID_IDirectDrawMediaStream, pDDStream);
   	if (hr != DD_OK)
	{
		CQERROR0("Could not get IDirectDrawMediaStream");
		goto Done;
	}

	//
	// try to set to a compatible format
	//
	memset(&ddsd, 0, sizeof(ddsd));
	ddsd.dwSize = sizeof(ddsd);
	pDDStream->GetFormat(&ddsd, 0, 0, 0);
	pPrimary->GetPixelFormat(&ddsd.ddpfPixelFormat);
	ddsd.dwFlags |= DDSD_PIXELFORMAT;
	pDDStream->SetFormat(&ddsd, NULL);

    hr = pDDStream->CreateSample(NULL, NULL, DDSFF_PROGRESSIVERENDER, pSample);
   	if (hr != DD_OK)
	{
		CQERROR0("IDirectDrawMediaStream::CreateSample failed");
		goto Done;
	}

	{
		COMPTR<IDirectDrawSurface> pSurface1;

		hr = pSample->GetSurface(pSurface1, &rect);
   		if (hr != DD_OK)
		{
			CQERROR0("IDirectDrawStreamSample::GetSurface failed");
			goto Done;
		}

		hr = pSurface1->QueryInterface(IID_IDirectDrawSurface4, pSurface);
   		if (hr != DD_OK)
		{
			CQERROR0("Could not get IDirectDrawSurface4");
			goto Done;
		}
	}

	sysComp = (ISystemComponent *) GS;

    hr = pMMStream->SetState(STREAMSTATE_RUN);
   	if (hr != DD_OK)
	{
		CQERROR0("IMultiMediaStream::SetState failed");
		goto Done;
	}

    while (1)
	{
		if (pSample->Update(0, NULL, NULL, 0) != S_OK) 
			break;

		WM_WINAREA area;
		WM->GetClientArea(area);
		RECT destRect = { area.x, area.y, area.x+area.w, area.y+area.h };
		
		hr = pPrimary->Blt(&destRect, pSurface, &rect, DDBLT_WAIT, NULL);

		sysComp->Update();

//		if (CheckHotkeyPressed(IDH_ESCAPE))
//			break;
    }

Done:
    return hr;
}
//----------------------------------------------------------------------------------------
//
HRESULT PlayMovie (const char * filename)
{
	HRESULT hr;
	COMPTR<IDirectDraw4> pDD4;
	COMPTR<IDirectDrawSurface4> pDDSurface;
	COMPTR<IMultiMediaStream>  pMMStream;
	COMPTR<IDDBackDoor> pBackDoor;
	COMPTR<IUnknown> pDD1;
    
	hr = PIPE->QueryInterface(IID_IDDBackDoor, pBackDoor);
	if (hr != GR_OK)
		goto Done;
	hr = pBackDoor->get_dd_provider(DDBD_P_DIRECTDRAW, pDD1);
	if (hr != GR_OK)
		goto Done;
	
	//
    //Now query for the new I interface
	//
    hr = pDD1->QueryInterface(IID_IDirectDraw4, pDD4);    

	if (hr != DD_OK)
	{
		CQERROR0("Couldn't get IDirectDraw4");
		goto Done;
	}

	pDD1.free();

	hr = pBackDoor->get_dd_provider(DDBD_P_PRIMARYSURFACE, pDD1);
	if (hr != GR_OK)
		goto Done;

	hr = pDD1->QueryInterface(IID_IDirectDrawSurface4, pDDSurface);
	if (hr != GR_OK)
		goto Done;

    hr = openMMStream(filename, pDD4, pMMStream);

   	if (hr != DD_OK)
	{
		CQERROR2("Couldn't get play '%s', error=%08x", filename, hr);
		goto Done;
	}

	hr = renderStreamToSurface(pDDSurface, pMMStream);

Done:
	return hr;
}

//--------------------------------------------------------------------------//
//-------------------------------End VidStream.cpp--------------------------//
//--------------------------------------------------------------------------//
