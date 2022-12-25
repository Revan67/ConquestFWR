//
// checkhw.cpp
//
//		Hardware check for application stub
//

#include "stubpch.h"
#include "hotsetup.h"
#include "util.h"
#include "HotSetupRC.h"

using namespace NGLOBALS;

BOOL CheckHardware(BOOL fFirstTime, LPREQUIREMENTS req)
{
	struct
	{
		DEVMODE     dm;
		BYTE		rgbExtra[MAX_PATH];
	} dms;

	WORD        wXRes = HIWORD(req->dwResolution);
	WORD        wYRes = LOWORD(req->dwResolution);
	BOOL        fSupportsMinimumResolution = FALSE;
	int         nIdx = 0;
	SYSTEM_INFO sys;
	CALLBACKDATA cbd;

	cbd.nID = SS_CHECKHARDWARE;

	//
	//Give the setup app a status callback...
	//
	(*(GetAppCallback())) ((void *) &cbd);

	dms.dm.dmSize = sizeof(DEVMODE);
	dms.dm.dmDriverExtra = MAX_PATH;

	//
	//Check to see if display device supports the minimum resolution required
	//by the game.  We don't check the current resolution during setup, because
	//many games change the resolution on the fly - resolution during setup is
	//therefore not indicative of whether the game will function at game runtime...
	//
	while (TRUE)
	{
		//
		//Keep looping until we run out of supported display modes or until
		//we confirm that the device is capable of the desired minimum resolution
		//at the specified color depth
		//
		if (EnumDisplaySettings(NULL, nIdx++, &dms.dm))
		{
			if (dms.dm.dmPelsWidth  >= wXRes &&
				dms.dm.dmPelsHeight >= wYRes &&
				dms.dm.dmBitsPerPel >= (DWORD) req->cBitsPixel)
			{
				fSupportsMinimumResolution = TRUE;
				break;
			}

			ForwardMessages();
		}
		else
		{	
			break;
		}
	}
	
	HDC hDC = GetDC(HWND_DESKTOP);

	//
	//Check bits per pixel of current display mode...
	//
	if (req->cBitsPixel > GetDeviceCaps(hDC, BITSPIXEL))
	{
		//
		//Support legacy setup apps.  If cColors is non-zero then we check bits per pixel
		//of current display mode, not just whether any mode will support it.
		//
		if (req->cColors > 0)
		{
			fSupportsMinimumResolution = FALSE;
		}
	}
	else
	{
		//
		//Along with EnumDisplaySettings, we also check the current resolution/color-depth.
		//This is because some video drivers don't properly enumerate the available modes,
		//so it makes sense to check the current mode too.  If fSupportsMinimumResolution
		//is already TRUE, then this just wastes a little bit of time...
		//
		if (wXRes <= GetDeviceCaps(hDC, HORZRES))
		{
			if (wYRes <= GetDeviceCaps(hDC, VERTRES))
			{
				fSupportsMinimumResolution = TRUE;
			}
		}
	}

	ReleaseDC(HWND_DESKTOP, hDC);

	if (FALSE == fSupportsMinimumResolution)
	{
		TCHAR szTmpBuf[2];
		int   nResID;
		BOOL  fHighResStringExists;

		fHighResStringExists = EBULoadString(GetResourceInst(), 
									      STR_ERROR_NEEDHIGHERRES, 
										  szTmpBuf, 
										  sizeof(szTmpBuf));

		//
		//Use the STR_ERROR_NEEDHIGHERRES string if it's implemented, and if !256 colors
		//was passed in, else use STR_ERROR_NEED256COLORS (second clause supports legacy setups)
		//
		nResID = fHighResStringExists && 
			      256 != req->cColors ? 
				  STR_ERROR_NEEDHIGHERRES : STR_ERROR_NEED256COLORS;

		Alert(GetWndParent(), MB_ICONINFORMATION | MB_OK, nResID);
	}

	// Check for missing sound card (first time only).  This is a warning only.
	if( fFirstTime )
	{
		WAVEOUTCAPS Caps;
		UINT uResult;
		BOOL fShowMsg = FALSE;
		
		if( !waveOutGetNumDevs() )
		{
			fShowMsg = TRUE;
		}
		else
		{
			uResult = waveOutGetDevCaps( 0, &Caps, sizeof(Caps) );
			
			if( uResult )		//nonzero means error
			{
				if( uResult == MMSYSERR_NODRIVER )
				{
					fShowMsg = TRUE;
				}
			}
		}
		
		if( fShowMsg )
		{
			Alert( GetWndParent(), MB_ICONINFORMATION | MB_OK, STR_ERROR_NOSOUNDCARD );
		}
	}
	GetSystemInfo(&sys);
	if(sys.dwProcessorType < req->dwProcessorType)
	{
			Alert( GetWndParent(), MB_ICONINFORMATION | MB_OK, STR_ERROR_BADPROCESSOR );
			// allow non-intel 586 clones
//			return FALSE;
	}
	MEMORYSTATUS mem;
	mem.dwLength = sizeof(mem);
    GlobalMemoryStatus(&mem);
	if(mem.dwTotalPhys < req->dwTotalPhys  * 1024)
	{
		Alert( GetWndParent(), MB_ICONINFORMATION | MB_OK, STR_ERROR_NOTENOUGHMEMORY, req->dwRequiredPhys);
		//return FALSE;
    }

	return fSupportsMinimumResolution;
}


