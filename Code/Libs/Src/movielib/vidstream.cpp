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
#include <System.h>

#include <stdio.h>
#include <ddrawex.h>
#include <mmstream.h>
#include <amstream.h>
#include <ddstream.h>

#include <fdump.h>
#include <tempstr.h>
#include <rendpipeline.h>
#include <iddbackdoor.h>
#include <movieplay.h>


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
int __stdcall _localAnsiToWide (const char * input, wchar_t * output, U32 bufferSize)
{
		// 932 is Japanese code page
	return MultiByteToWideChar(CP_ACP, 0, input, -1, output, (bufferSize/sizeof(output[0])) );
}
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
		GENERAL_WARNING("Could not create IMultimediaStream instance");
		goto Done;
	}

    hr = pAMStream->Initialize(STREAMTYPE_READ, 0, NULL);
   	if (hr != DD_OK)
	{
		GENERAL_WARNING("IMultimediaStream::Initialize() failed.");
		goto Done;
	}

    hr = pAMStream->AddMediaStream(pDD, &MSPID_PrimaryVideo, 0, NULL);
   	if (hr != DD_OK)
	{
		GENERAL_WARNING("Adding Primary video failed");
		goto Done;
	}

    hr = pAMStream->AddMediaStream(NULL, &MSPID_PrimaryAudio, AMMSF_ADDDEFAULTRENDERER, NULL);
   	if (hr != DD_OK)
	{
		GENERAL_WARNING("Adding Primary audio failed");
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
static HRESULT renderStreamToSurface
(
	IDirectDrawSurface4 *pPrimary,
	IMultiMediaStream *pMMStream,
	RECT destRect, 
	PlayMovieCallback *callback
)
{
    HRESULT hr;
	COMPTR<IMediaStream> pPrimaryVidStream;
    COMPTR<IDirectDrawMediaStream> pDDStream;
    COMPTR<IDirectDrawSurface4> pSurface;
    COMPTR<IDirectDrawStreamSample> pSample;
    DDSURFACEDESC ddsd;

    RECT rect;

    hr = pMMStream->GetMediaStream(MSPID_PrimaryVideo, pPrimaryVidStream);
   	if (hr != DD_OK)
	{
		GENERAL_WARNING("GetMediaStream failed");
		goto Done;
	}

    hr = pPrimaryVidStream->QueryInterface(IID_IDirectDrawMediaStream, pDDStream);
   	if (hr != DD_OK)
	{
		GENERAL_WARNING("Could not get IDirectDrawMediaStream");
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
		GENERAL_WARNING("IDirectDrawMediaStream::CreateSample failed");
		goto Done;
	}

	{
		COMPTR<IDirectDrawSurface> pSurface1;

		hr = pSample->GetSurface(pSurface1, &rect);
   		if (hr != DD_OK)
		{
			GENERAL_WARNING("IDirectDrawStreamSample::GetSurface failed");
			goto Done;
		}

		hr = pSurface1->QueryInterface(IID_IDirectDrawSurface4, pSurface);
   		if (hr != DD_OK)
		{
			GENERAL_WARNING("Could not get IDirectDrawSurface4");
			goto Done;
		}
	}

    hr = pMMStream->SetState(STREAMSTATE_RUN);
   	if (hr != DD_OK)
	{
		GENERAL_WARNING("IMultiMediaStream::SetState failed");
		goto Done;
	}

    while (1)
	{
		// See if we should continue playing.
		if (callback != NULL)
		{
			if (!callback(&destRect))
			{
				break;
			}
		}

		// Update the movie stream, exiting if finished.
		if (pSample->Update(0, NULL, NULL, 0) != S_OK) 
			break;

		// Blit the image to the screen.
		// *** Shouldn't we only do this if we need to?
		hr = pPrimary->Blt(&destRect, pSurface, &rect, DDBLT_WAIT, NULL);
    }

Done:
    return hr;
}
//----------------------------------------------------------------------------------------
//
// NOTE: This should probably take an IFileSystem, but for now, it must take a filename.
// I suspect that to support an IFileSystem, we will have to make an IFileSystem compatible media stream
// MSCOM thing. ACK!
HRESULT PlayMovie (IRenderPipeline *PIPE, const char * filename, RECT *destRect, PlayMovieCallback *callback)
{
	ASSERT(PIPE);
	ASSERT(filename);

	HRESULT hr;
	DDSURFACEDESC2    ddsd;
	COMPTR<IDirectDraw4> pDD4;
	COMPTR<IDirectDrawSurface4> pDDSurface;
	COMPTR<IMultiMediaStream>  pMMStream;
	COMPTR<IDirectDrawClipper> pClipper;

	hr = DDERR_GENERIC;
    
	// Retrieve the direct draw and primary surface pointers from the render pipeline using the back door
	// interface. If it is not available, punt.

	{
		COMPTR<IDDBackDoor> bd;
		COMPTR<IUnknown>    pdd;
		COMPTR<IUnknown>    pprim;
		if (PIPE->QueryInterface(IID_IDDBackDoor, bd) != GR_OK)
		{
			GENERAL_WARNING("Failed to query backdoor interface from the render pipeline.");
			goto Done;
		}

		if (bd->get_dd_provider (DDBD_P_DIRECTDRAW, pdd) != GR_OK)
		{
			GENERAL_WARNING("Failed to get direct draw object via back door.");
			goto Done;
		}

		if (bd->get_dd_provider (DDBD_P_PRIMARYSURFACE, pprim) != GR_OK)
		{
			GENERAL_WARNING("Failed to get primary surface object via back door.");
			goto Done;
		}

		// Since we got IUnknown interfaces, we need to get the real interfaces from them.

		hr = pdd->QueryInterface(IID_IDirectDraw4, pDD4);    

		if (hr != DD_OK)
		{
			GENERAL_WARNING("Couldn't get IDirectDraw4");
			goto Done;
		}

		hr = pprim->QueryInterface (IID_IDirectDrawSurface4, pDDSurface);

		if (hr != DD_OK)
		{
			GENERAL_WARNING("Couldn't get IDirectDrawSurface4");
			goto Done;
		}
	}

	// Open the multimedia stream.
    hr = openMMStream(filename, pDD4, pMMStream);

   	if (hr != DD_OK)
	{
		GENERAL_WARNING(TEMPSTR("Couldn't open '%s' as a multimedia stream, error=%08x", filename, hr));
		goto Done;
	}

	RECT r;
	if (destRect != NULL)
	{
		r = *destRect;
	}
	else
	{
		// Make R the size of the entire screen by querying the primary surface for its dimensions
		memset(&ddsd, 0, sizeof(ddsd));
		ddsd.dwSize = sizeof(ddsd);    
		hr = pDDSurface->GetSurfaceDesc(&ddsd);
		if (hr != DD_OK)
		{
			GENERAL_WARNING("Failed to get primary surface description");
			goto Done;
		}

		r.top = r.left = 0;
		r.right = ddsd.dwWidth;
		r.bottom = ddsd.dwHeight;
	}
	
	hr = renderStreamToSurface(pDDSurface, pMMStream, r, callback);

Done:
	return hr;
}

//--------------------------------------------------------------------------//
//-------------------------------End VidStream.cpp--------------------------//
//--------------------------------------------------------------------------//
