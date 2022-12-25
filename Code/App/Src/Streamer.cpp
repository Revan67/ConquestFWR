//--------------------------------------------------------------------------//
//                                                                          //
//                              Streamer.cpp                                //
//                                                                          //
//               COPYRIGHT (C) 1997 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
   $Author: Jasony $

   $Header: /Conquest/App/Src/Streamer.cpp 13    9/25/00 10:04p Jasony $

   Audio player
*/
//--------------------------------------------------------------------------//

#include "pch.h"
#include <globals.h>

#include "Startup.h"
#include "CQTrace.h"
#include "resource.h"

#include <TComponent.h>
#include <HeapObj.h>
#include <TSmartPointer.h>
#include <FileSys.h>
#include <Streamer.h>
#include <RPUL.h>
#include <WindowManager.h>

#include <mmsystem.h>
#include <dsound.h>


#define DSERROR(hr)				\
			if (hr != DS_OK)	\
				CQBOMB1("DSound error #%08X", hr)
#define CHUNK_NAME(d0,d1,d2,d3) ((long(d3)<<24)+(long(d2)<<16)+(d1<<8)+d0)
#define CDALIGNMASK 0x7FF

//--------------------------------------------------------------------------//
//
struct _streamer : GlobalComponent
{
	BOOL32 init (void)
	{
		COMPTR<IProfileParser> parser;
		HANDLE hSection;
		GUID guid, *pGuid;

		if (DACOM->QueryInterface("IProfileParser", parser) != GR_OK)
			return 0;
		if ((hSection = parser->CreateSection("Sound")) == 0)
			return 0;
		else
		{
			U32 len;
			char buffer[256];
			
			if ((len = parser->ReadKeyValue(hSection, "Sound", buffer, sizeof(buffer))) == 0)
			{
				parser->CloseSection(hSection);
				return 0;
			}

			pGuid = ConvertStringToGUID(buffer, &guid);
			parser->CloseSection(hSection);
		}

		//
		// create direct sound interface
		//
		HRESULT hr = CoCreateInstance( CLSID_DirectSound, NULL, CLSCTX_INPROC_SERVER, IID_IDirectSound, (LPVOID*)&DSOUND);
		
		if (hr == DS_OK)
			hr = DSOUND->Initialize(pGuid);

		if (hr != DS_OK)
		{
			if (DSOUND)
				DSOUND->Release();
			DSOUND = 0;
			wchar_t name[64];
			wcsncpy(name, _localLoadStringW(IDS_APP_NAME), sizeof(name)/sizeof(wchar_t));
			MessageBoxW(hMainWindow, _localLoadStringW(IDS_HELP_NOSOUND), name, MB_OK|MB_ICONSTOP);
			PostQuitMessage(-1);
			if (WM)
				WM->ServeMessageQueue();
			return 0;
		}

		if ((hr = DSOUND->SetCooperativeLevel(hMainWindow, DSSCL_PRIORITY)) == DS_OK)
		{
			DSBUFFERDESC dsdesc;
			COMPTR<IDirectSoundBuffer> lpDSPrimary;

			memset(&dsdesc, 0, sizeof(dsdesc));
			dsdesc.dwSize = sizeof(dsdesc);
			dsdesc.dwFlags = DSBCAPS_PRIMARYBUFFER;
			if ((hr = DSOUND->CreateSoundBuffer(&dsdesc, lpDSPrimary, 0)) == DS_OK)
			{
				// set the format to 16 bit 22khz
				WAVEFORMATEX format;
				memset(&format, 0, sizeof(format));
				format.wFormatTag = 1;
				format.nChannels = 2;
				format.nSamplesPerSec = 22050;
				format.nAvgBytesPerSec = 88200;
				format.nBlockAlign = 4;
				format.wBitsPerSample = 16;
				hr = lpDSPrimary->SetFormat(&format);
#ifdef _DEBUG
				DSERROR(hr);
#endif
			}
		}
		else
		{
			hr = DSOUND->SetCooperativeLevel(hMainWindow, DSSCL_NORMAL);
			if (hr != DS_OK)
			{
				DSOUND->Release();
				DSOUND = 0;
				wchar_t name[64];
				wcsncpy(name, _localLoadStringW(IDS_APP_NAME), sizeof(name)/sizeof(wchar_t));
				MessageBoxW(hMainWindow, _localLoadStringW(IDS_HELP_NOSOUND), name, MB_OK|MB_ICONSTOP);
				PostQuitMessage(-1);
				if (WM)
					WM->ServeMessageQueue();
				return 0;
			}
		}

		return 1;
	}
	
	virtual void Startup (void)
	{
		if (init())
			AddToGlobalCleanupList((IDAComponent **)&DSOUND);
	}

	virtual void Initialize (void)
	{
		STREAMERDESC desc;

		desc.hMainWindow = hMainWindow;
		desc.lpDSound = DSOUND;
		desc.uMsg = CQE_STREAMER;
		desc.readBufferTime =  4;		// in seconds, 0 = default setting (4.0 is a reasonable value)
		desc.soundBufferTime = 1.0;		// in seconds, 0 = default setting (0.25 is a reasonable value)

		STREAMER->Init(&desc);
	}
};

static _streamer startup;
//--------------------------------------------------------------------------//
//---------------------------End Streamer.cpp-------------------------------//
//--------------------------------------------------------------------------//
