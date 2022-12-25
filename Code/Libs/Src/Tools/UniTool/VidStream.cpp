//--------------------------------------------------------------------------//
//                                                                          //
//                              VidStream.cpp                               //
//                                                                          //
//               COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
   $Header: /Conquest/App/Src/VidStream.cpp 2     12/08/98 6:57p Jasony $
*/

//--------------------------------------------------------------------------//
#include <windows.h>

#include <TSmartPointer.h>
//#include <WindowManager.h>
#include <System.h>

#include <stdio.h>
//#include <mmsystem.h>
#include <ddrawex.h>
#include <mmstream.h>
#include <amstream.h>
#include <ddstream.h>

#include <fdump.h>
#include <tempstr.h>
#include "strstuff.h"

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
		GENERAL_ERROR("Could not create IMultimediaStream instance");
		goto Done;
	}

    hr = pAMStream->Initialize(STREAMTYPE_READ, 0, NULL);
   	if (hr != DD_OK)
	{
		GENERAL_ERROR("IMultimediaStream::Initialize() failed.");
		goto Done;
	}

    hr = pAMStream->AddMediaStream(pDD, &MSPID_PrimaryVideo, 0, NULL);
   	if (hr != DD_OK)
	{
		GENERAL_ERROR("Adding Primary video failed");
		goto Done;
	}

    hr = pAMStream->AddMediaStream(NULL, &MSPID_PrimaryAudio, AMMSF_ADDDEFAULTRENDERER, NULL);
   	if (hr != DD_OK)
	{
		GENERAL_ERROR("Adding Primary audio failed");
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
static HRESULT renderStreamToSurface(IDirectDrawSurface4 *pPrimary, IMultiMediaStream *pMMStream, RECT destRect)
{
    HRESULT hr;
	COMPTR<IMediaStream> pPrimaryVidStream;
    COMPTR<IDirectDrawMediaStream> pDDStream;
    COMPTR<IDirectDrawSurface4> pSurface;
    COMPTR<IDirectDrawStreamSample> pSample;
//	COMPTR<ISystemComponent> sysComp;
    DDSURFACEDESC ddsd;

    RECT rect;

    hr = pMMStream->GetMediaStream(MSPID_PrimaryVideo, pPrimaryVidStream);
   	if (hr != DD_OK)
	{
		GENERAL_ERROR("GetMediaStream failed");
		goto Done;
	}

    hr = pPrimaryVidStream->QueryInterface(IID_IDirectDrawMediaStream, pDDStream);
   	if (hr != DD_OK)
	{
		GENERAL_ERROR("Could not get IDirectDrawMediaStream");
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
		GENERAL_ERROR("IDirectDrawMediaStream::CreateSample failed");
		goto Done;
	}

	{
		COMPTR<IDirectDrawSurface> pSurface1;

		hr = pSample->GetSurface(pSurface1, &rect);
   		if (hr != DD_OK)
		{
			GENERAL_ERROR("IDirectDrawStreamSample::GetSurface failed");
			goto Done;
		}

		hr = pSurface1->QueryInterface(IID_IDirectDrawSurface4, pSurface);
   		if (hr != DD_OK)
		{
			GENERAL_ERROR("Could not get IDirectDrawSurface4");
			goto Done;
		}
	}

//	sysComp = (ISystemComponent *) GS;

    hr = pMMStream->SetState(STREAMSTATE_RUN);
   	if (hr != DD_OK)
	{
		GENERAL_ERROR("IMultiMediaStream::SetState failed");
		goto Done;
	}

    while (1)
	{
		if (pSample->Update(0, NULL, NULL, 0) != S_OK) 
			break;

#if 0
		WM_WINAREA area;
		WM->GetClientArea(area);
		RECT destRect = { area.x, area.y, area.x+area.w, area.y+area.h };
#endif
		
		hr = pPrimary->Blt(&destRect, pSurface, &rect, DDBLT_WAIT, NULL);

//		sysComp->Update();

#if 0
		if (CheckHotkeyPressed(IDH_ESCAPE))
			break;
#endif
    }

Done:
    return hr;
}
//----------------------------------------------------------------------------------------
//
HRESULT PlayMovie (HWND hMainWindow, const char * filename)
{
	HRESULT hr;
    DDSURFACEDESC2    ddsd;
	COMPTR<IDirectDrawFactory> pDDFactory;
	COMPTR<IDirectDraw> pDD1;
	COMPTR<IDirectDraw4> pDD4;
	COMPTR<IDirectDrawSurface4> pDDSurface;
	COMPTR<IMultiMediaStream>  pMMStream;
	COMPTR<IDirectDrawClipper> pClipper;
    
	//
	// Create a DirectDrawFactory object
	//
    hr = CoCreateInstance(CLSID_DirectDrawFactory, NULL, CLSCTX_INPROC_SERVER, 
                            IID_IDirectDrawFactory, pDDFactory);
	
	if (hr != DD_OK)
	{
		GENERAL_ERROR("Could not create DirectDrawFactory");
		goto Done;
	}

    hr = pDDFactory->CreateDirectDraw(NULL, hMainWindow, DDSCL_NORMAL, NULL, NULL, pDD1);
	
	if (hr != DD_OK)
	{
		GENERAL_ERROR("Couldn't create DirectDraw object");
		goto Done;
	}
	
	//
    //Now query for the new I interface
	//
    hr = pDD1->QueryInterface(IID_IDirectDraw4, pDD4);    

	if (hr != DD_OK)
	{
		GENERAL_ERROR("Couldn't get IDirectDraw4");
		goto Done;
	}

	pDD1.free();

	//
    //Initialize the DDSURFACEDESC structure for the primary surface
	//
	memset(&ddsd, 0, sizeof(ddsd));
	ddsd.dwSize = sizeof(ddsd);    
	ddsd.dwFlags = DDSD_CAPS;
    ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE; 
    hr = pDD4->CreateSurface(&ddsd, pDDSurface, NULL);    

   	if (hr != DD_OK)
	{
		GENERAL_ERROR("Couldn't create Primary Surface");
		goto Done;
	}

	//
	// create the clipper object
	//

	hr = pDD4->CreateClipper(0, pClipper, NULL);

   	if (hr != DD_OK)
	{
		GENERAL_ERROR("Couldn't create clipper");
		goto Done;
	}

	pDDSurface->SetClipper(pClipper);
	pClipper->SetHWnd(0, hMainWindow);

    hr = openMMStream(filename, pDD4, pMMStream);

   	if (hr != DD_OK)
	{
		GENERAL_ERROR(TEMPSTR("Couldn't get play '%s', error=%08x", filename, hr));
		goto Done;
	}

	{
		RECT r;
		GetClientRect (hMainWindow, &r);
		POINT tl = {r.left, r.top};
		POINT br = {r.right, r.bottom};

		ClientToScreen (hMainWindow, &tl);
		ClientToScreen (hMainWindow, &br);

		r.top = tl.y;
		r.left = tl.x;
		r.bottom = br.y;
		r.right = br.x;

		hr = renderStreamToSurface(pDDSurface, pMMStream, r);
	}

Done:
	return hr;
}

//--------------------------------------------------------------------------//
//-------------------------------End VidStream.cpp--------------------------//
//--------------------------------------------------------------------------//
