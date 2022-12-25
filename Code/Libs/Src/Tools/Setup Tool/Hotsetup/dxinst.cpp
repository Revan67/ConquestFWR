/****************************************************************************

	PROGRAM: Dxinst.c
		Copyright (C) 1988-1995 Microsoft Corp.

	PURPOSE: DirectXSetup install handler
	
	FUNCTIONS:



****************************************************************************/

#include <windows.h>		    /* required for all Windows applications*/
#include <stdio.h>
#include <process.h>

#include <dsetup.h>				// required for DSETUP_DIRECTX define

#include "string.h"
#include "hotsetuprc.h"
#include "hotsetup.h"
#include "setup.h"
#include "util.h"
#include "restart.h"
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//function declarations
typedef int (WINAPI * WINFARPROC)(HWND,LPSTR,DWORD);
typedef int (WINAPI * CALLBACKSETUPPROC) (DSETUP_CALLBACK);
typedef int (WINAPI * DXVERPROC) (DWORD *, DWORD *);
typedef int (WINAPI * DPLAYREGPROC)(HWND,LPDIRECTXREGISTERAPP);
typedef int (WINAPI * DPLAYUNREGPROC)(HWND,LPGUID);

using namespace NGLOBALS;

EBURETCODE InstDX(LPINSTDX lpInstDX,DIRECT_X_VERSION *uExistingVersion);
EBURETCODE InstDPLAY(LPINSTDPLAY lpInstDPLAY,DIRECT_X_VERSION *uExistingVersion);
EBURETCODE UnInstDPLAY(LPINSTDPLAY lpInstDPLAY,DIRECT_X_VERSION *uExistingVersion);
int CallDirectXInstall(WINFARPROC lpfnFarProcSetup, LPSTR szSource, DWORD dwFlags);
UINT _stdcall CheckForDXCopyDialog(void *pv);
BOOL CALLBACK EnumSetupChildProc(HWND hWnd, LPARAM lParam);

VOID NukeDirectPlayRemnants(void);
void ClearUpdates();

extern "C" 
{
	DWORD WINAPI DirectXSetupCallbackBatchFunction(DWORD dwReason, DWORD dwMsgType, LPSTR szMessage, LPSTR szName,void *pInfo);
	DWORD WINAPI DirectXSetupCallbackFunction(DWORD dwReason, DWORD dwMsgType, LPSTR szMessage, LPSTR szName, void *pInfo);
}

int UpdateBatch(DWORD dwReason,DWORD dwMsgType, char *szMessage, char *szName, void *pInfo);
int UpdateArray(DWORD dwMsgType,DWORD dwReason,char *szName,DWORD dwFlags);
void ReallocateArray();

UPDATEARRAY gUpdates;
static HANDLE g_hDXInstallEvent;

//for DPLAY lobby registration
char dirpath[_MAX_PATH] = "";

//----------------------------------------------------------------------------
void ClearUpdates()
{
	int x;
	gUpdates.dwError = 0;

	ZeroMemory(gUpdates.szErrMsg, sizeof(gUpdates.szErrMsg));

	for (x=0;x<gUpdates.numUpdates;x++)
	{
		if (gUpdates.UpdateArray[x])
		{
			free(gUpdates.UpdateArray[x]);
			gUpdates.UpdateArray[x] = NULL;
		}
     }

	 if (gUpdates.UpdateArray)
	 {
		free( gUpdates.UpdateArray);
	 }

	 gUpdates.UpdateArray = NULL;
	
	gUpdates.numUpdates = 0;
	gUpdates.arraySize = 0;
}

EBURETCODE InstDX(LPINSTDX lpInstDX, DIRECT_X_VERSION *uExistingVersion)
{
	WINFARPROC lpfnFarProcSetup;
	CALLBACKSETUPPROC lpfnFarProcCallback;
	HINSTANCE hLib;
	EBURETCODE nShouldInstall = EBU_OK;
	TCHAR szBuf[_MAX_PATH];
	DIRECTXDATA dxd;
	
	char *szsetCallBack = "DirectXSetupSetCallback";
	
	TCHAR szExePath[MAX_PATH];
	TCHAR *pszExePath;
	
	dxd.uExistingVersion = *uExistingVersion;
	
	DWORD dwFlags = *lpInstDX->GetInstDXFlags();
	
	int retc;
	
	lstrcpy(szBuf, lpInstDX->GetInstDX());
	ReplaceStringTokens(szBuf, sizeof(szBuf));

	if((hLib = LoadLibrary(szBuf)) <= (HINSTANCE)32)
	{
		if (GetTrial())
			return EBU_OK;
		else
			return EBU_ERROR;
	}

	//
	//Get directory that is the parent directory of the <DIRECTX> directory...
	//
	GetModuleFileName(hLib, szExePath, sizeof(szExePath));
	pszExePath = pszGetLast5C(szExePath);
	if (pszExePath)
	{
		*pszExePath = '\0';
	}
	
	//
	//Get short variation of DirectX path name
	//
	if (GetShortPathName(szExePath, szBuf, sizeof(szBuf)))
	{
		lstrcpy(szExePath, szBuf);
	}

	lpfnFarProcSetup = (WINFARPROC) GetProcAddress(hLib, lpInstDX->GetInstDXName());
	if(lpfnFarProcSetup == NULL)
	{
		FreeLibrary(hLib);

		return EBU_ERROR;
	}
	
	lpfnFarProcCallback = (CALLBACKSETUPPROC) GetProcAddress(hLib, szsetCallBack);
	if( lpfnFarProcCallback == NULL )
	{
		dxd.nStatus = QUERYDIRECTXINSTALL;
		nShouldInstall = (*(GetAppCallback())) ((void *) &dxd);
		
		ASSERT(EBU_OK == nShouldInstall || EBU_ABORT == nShouldInstall || EBU_CANCEL == nShouldInstall || EBU_BACK == nShouldInstall);
		
		if (EBU_ABORT == nShouldInstall)
        {
			SetDXReturnCode(DSETUPERR_NOCOPY);
			FreeLibrary(hLib);
			
			return EBU_ABORT;
		}
		else if(nShouldInstall == EBU_CANCEL)
		{
			FreeLibrary(hLib);

			return EBU_OK;
		}
		
		retc = CallDirectXInstall(lpfnFarProcSetup, szExePath, dwFlags);
		SetDXReturnCode(retc);
		FreeLibrary(hLib);
		
		if (!EnsureCDROMInserted(NULL))
		{
			return EBU_ABORT;
		}
		
		if (1 == retc)
		{
			SetupSetRebootFlag();
		}
		else if (retc != 0)
		{
			return EBU_ERROR;
		}
		
		return EBU_OK;
    }

	EBURETCODE nResult = EBU_OK;
	
	dxd.nStatus = DETECTDIRECTX;
	nResult = (*(GetAppCallback())) ((void *) &dxd);

	ASSERT(EBU_OK == nResult || EBU_ABORT == nResult || EBU_CANCEL == nResult || EBU_BACK == nResult);

	if (EBU_ABORT == nResult || EBU_CANCEL == nResult || EBU_BACK == nResult)
	{
		FreeLibrary(hLib);

		return nResult;
	}

	ZeroMemory(&gUpdates, sizeof(gUpdates));

    ClearUpdates();
	
    (*lpfnFarProcCallback) ((DSETUP_CALLBACK)DirectXSetupCallbackFunction);
	
    retc = (*lpfnFarProcSetup) (GetWndParent(), szExePath, dwFlags | DSETUP_TESTINSTALL);
	
	dxd.gUpdates = &gUpdates;
	dxd.nStatus = QUERYDIRECTXINSTALL;
	
	nResult = (*(GetAppCallback())) ((void *) &dxd);
	
	if (EBU_OK == nResult)
	{
		(*lpfnFarProcCallback)((DSETUP_CALLBACK)DirectXSetupCallbackBatchFunction);
		
		retc = CallDirectXInstall(lpfnFarProcSetup, szExePath, dwFlags);
		SetDXReturnCode(retc);
		FreeLibrary(hLib);

		if (!EnsureCDROMInserted(NULL))
		{
			return EBU_ABORT;
		}
		
		switch(retc)
		{
		case DSETUPERR_SUCCESS_RESTART:
			SetupSetRebootFlag();
			ClearUpdates();
			
			return EBU_OK;
			
		case DSETUPERR_SUCCESS:
			ClearUpdates();
			return EBU_OK;
			
		case DSETUPERR_BADWINDOWSVERSION:
		case DSETUPERR_SOURCEFILENOTFOUND:
		case DSETUPERR_BADSOURCESIZE:
		case DSETUPERR_BADSOURCETIME:
		case DSETUPERR_NOCOPY:
		case DSETUPERR_OUTOFDISKSPACE:
		case DSETUPERR_CANTFINDINF:
		case DSETUPERR_CANTFINDDIR:
		case DSETUPERR_INTERNAL:
		case DSETUPERR_UNKNOWNOS:
		case DSETUPERR_USERHITCANCEL:
		case DSETUPERR_NOTPREINSTALLEDONNT:
			if (gUpdates.szErrMsg[0] != '\0')
			{
				Alert(GetWndParent(), MB_ICONSTOP | MB_OK, STR_ERROR_DXINST, gUpdates.szErrMsg);
			}
			else
			{
				Alert(GetWndParent(), MB_ICONSTOP | MB_OK,STR_DXERROR_RETURN+(retc * -1)-1);
			}
			
			ClearUpdates();
			
			return EBU_ERROR;
			
		default:
			ClearUpdates();
			
			return EBU_ERROR;
		}
    }
	else
	{
		FreeLibrary(hLib);

		ClearUpdates();
		
		return nResult;
	}
}

DWORD WINAPI DirectXSetupCallbackFunction(DWORD dwReason, DWORD dwMsgType, LPSTR szMessage, LPSTR szName,void *pInfo)
{
	//
	//Allow some time for other setup app threads to get some attention while DirectX
	//does its thing
	//
	ForwardMessages();
	Sleep(0);

	return UpdateBatch(dwReason,dwMsgType,szMessage,szName,pInfo);
}

DWORD WINAPI DirectXSetupCallbackBatchFunction(DWORD dwReason, DWORD dwMsgType, LPSTR szMessage, LPSTR szName,void *pInfo)
{
   static int x = 0;

	//
	//Allow some time for other setup app threads to get some attention while DirectX
	//does its thing
	//
	ForwardMessages();
	Sleep(0);

	switch (dwReason)
	{
	   	case  DSETUP_CB_MSG_CANTINSTALL_UNKNOWNOS:
		case  DSETUP_CB_MSG_CANTINSTALL_NT:
		case  DSETUP_CB_MSG_CANTINSTALL_BETA:
		case  DSETUP_CB_MSG_CANTINSTALL_NOTWIN32:
		case  DSETUP_CB_MSG_CANTINSTALL_WRONGLANGUAGE:
		case  DSETUP_CB_MSG_CANTINSTALL_WRONGPLATFORM:
		case  DSETUP_CB_MSG_PREINSTALL_NT:
		case  DSETUP_CB_MSG_NOTPREINSTALLEDONNT:
		case  DSETUP_CB_MSG_SETUP_INIT_FAILED:
		case  DSETUP_CB_MSG_INTERNAL_ERROR:
		case  DSETUP_CB_MSG_OUTOFDISKSPACE:
		case  DSETUP_CB_MSG_FILECOPYERROR:
			return IDCANCEL;

		case  DSETUP_CB_MSG_CHECK_DRIVER_UPGRADE:
			{
				if((!lstrcmp(szName,gUpdates.UpdateArray[x]->szName))
					&& gUpdates.UpdateArray[x]->dwMsgType == dwMsgType)
				    return gUpdates.UpdateArray[x++]->UserResponse;
				else
					return GetDefaultButtonResponse(dwMsgType);
			}
			break;
		default:
			ASSERT (0 == dwMsgType);
			return IDOK;
    }
}

int UpdateBatch(DWORD dwReason,DWORD dwMsgType, char *szMessage,char *szName, void *pInfo)
{
	switch (dwReason)
	{
		case  DSETUP_CB_MSG_CANTINSTALL_UNKNOWNOS:
		case  DSETUP_CB_MSG_CANTINSTALL_NT:
		case  DSETUP_CB_MSG_CANTINSTALL_BETA:
		case  DSETUP_CB_MSG_CANTINSTALL_NOTWIN32:
		case  DSETUP_CB_MSG_CANTINSTALL_WRONGLANGUAGE:
		case  DSETUP_CB_MSG_CANTINSTALL_WRONGPLATFORM:
		case  DSETUP_CB_MSG_PREINSTALL_NT:
		case  DSETUP_CB_MSG_NOTPREINSTALLEDONNT:
		case  DSETUP_CB_MSG_SETUP_INIT_FAILED:
		case  DSETUP_CB_MSG_INTERNAL_ERROR:
		case  DSETUP_CB_MSG_OUTOFDISKSPACE:
		case  DSETUP_CB_MSG_FILECOPYERROR:
			gUpdates.dwError = dwReason;
			lstrcpy(gUpdates.szErrMsg,szMessage);
			return GetDefaultButtonResponse(dwMsgType);

		case  DSETUP_CB_MSG_CHECK_DRIVER_UPGRADE:
			{
				DWORD dwUpgrade = ((DSETUP_CB_UPGRADEINFO *)pInfo)->UpgradeFlags;
				switch((dwUpgrade & DSETUP_CB_UPGRADE_TYPE_MASK))
				{
					case  DSETUP_CB_UPGRADE_FORCE:
					case  DSETUP_CB_UPGRADE_UNKNOWN:
					case  DSETUP_CB_UPGRADE_KEEP:
					case DSETUP_CB_UPGRADE_SAFE:
						return UpdateArray(dwMsgType,dwReason,szName,dwUpgrade);
					default:
						ASSERT (FALSE); //should always be one of the four types above...
				}
			}

			break;
	}
	ASSERT (dwMsgType == 0);
	return IDOK;
}



WORD GetDefaultButtonResponse(DWORD dwMsgType)
{
	switch (dwMsgType & 0x0000000F)
	{
	case MB_OKCANCEL:
		return (dwMsgType & MB_DEFBUTTON2) ? IDCANCEL : IDOK;

	case  MB_RETRYCANCEL:
		return (dwMsgType & MB_DEFBUTTON2) ? IDCANCEL : IDRETRY;

	case MB_ABORTRETRYIGNORE:
		if (dwMsgType & MB_DEFBUTTON2)
		{
			return IDRETRY;
		}
		else
		{
			if (dwMsgType & MB_DEFBUTTON3)
			{
				return IDIGNORE;
			}
		}

		return IDABORT;

	case MB_YESNOCANCEL:
		if (dwMsgType & MB_DEFBUTTON2)
		{
			return IDNO;
		}
		else
		{
			if (dwMsgType & MB_DEFBUTTON3)
			{
				return IDCANCEL;
			}
		}
		
		return IDYES;

	case MB_YESNO:
		return (dwMsgType & MB_DEFBUTTON2) ? IDNO : IDYES;
	}

	return IDOK;
}
int UpdateArray(DWORD dwMsgType,DWORD dwReason,char *szName,DWORD dwFlags)
{
	LPDRIVERUPDATE lpDriver;
	if(gUpdates.arraySize == gUpdates.numUpdates)
	{
		ReallocateArray();
	}
	lpDriver = gUpdates.UpdateArray[gUpdates.numUpdates] = (LPDRIVERUPDATE)malloc(sizeof(DRIVERUPDATE));
	gUpdates.numUpdates++;
	lpDriver->dwMsgType = dwMsgType;
	lstrcpy(lpDriver->szName,szName);

	if(dwFlags & DSETUP_CB_UPGRADE_DEVICE_ACTIVE)
		lpDriver->bActive = TRUE;
	else
		lpDriver->bActive = FALSE;

	dwFlags &= (DSETUP_CB_UPGRADE_TYPE_MASK | DSETUP_CB_UPGRADE_HASWARNINGS);

	switch(dwFlags)
	{
		case  DSETUP_CB_UPGRADE_FORCE:
		case  (DSETUP_CB_UPGRADE_FORCE | DSETUP_CB_UPGRADE_HASWARNINGS):
			lpDriver->eStatus = DX_FORCE;
			break;
		case  (DSETUP_CB_UPGRADE_SAFE | DSETUP_CB_UPGRADE_HASWARNINGS):
			lpDriver->eStatus = DX_WARN;
			break;
		case  DSETUP_CB_UPGRADE_UNKNOWN:
			lpDriver->eStatus = DX_UNKNOWN;
			break;
		case  DSETUP_CB_UPGRADE_KEEP:
			lpDriver->eStatus = DX_KEEP;
			break;
		case DSETUP_CB_UPGRADE_SAFE:
			lpDriver->eStatus = DX_SAFE;
			break;
//		case DSETUP_CB_UPGRADE_UNNECESSARY:
//			lpDriver->eStatus = DX_UNNECESSARY;
//			break;
	}

	lpDriver->dwFlags = dwFlags & (DSETUP_CB_UPGRADE_TYPE_MASK);

	lpDriver->UserResponse = GetDefaultButtonResponse(dwMsgType);
		
	return lpDriver->UserResponse;
}
void ReallocateArray()
{
	int x;
	LPDRIVERUPDATE * lpDrivers = (LPDRIVERUPDATE *)malloc(sizeof(LPDRIVERUPDATE)*(gUpdates.arraySize+10));

	ZeroMemory((void *) lpDrivers, sizeof(LPDRIVERUPDATE) * (gUpdates.arraySize + 10));

	for(x = 0;x < gUpdates.numUpdates;x++)
	{
		lpDrivers[x] = gUpdates.UpdateArray[x];
	}

	free(gUpdates.UpdateArray);

	gUpdates.UpdateArray = lpDrivers;
}

int CallDirectXInstall(WINFARPROC lpfnFarProcSetup, LPSTR szSource, DWORD dwFlags)
{
	int retc;
	unsigned long uDXCopyDlgCheckThreadID;
	HANDLE hThread;

	//
	//Create an event that we'll set after DX install is complete.  The dialog checking
	//thread will wait for this to be set before terminating
	//
	g_hDXInstallEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	//
	//Start a thread that checks for the DirectX "copying files" dialog and brings it to
	//the foreground if it finds one...
	//
    hThread = CreateThread( 
        NULL,
        0,
        (LPTHREAD_START_ROUTINE) CheckForDXCopyDialog,
        (void *) NULL,
        0,
        &uDXCopyDlgCheckThreadID);

	//
	//Install DirectX
	//
	
    retc = (*lpfnFarProcSetup) (GetWndParent(), szSource, dwFlags);

	//
	//Tell the dialog checking thread that we're done now.
	//
	SetEvent(g_hDXInstallEvent);

	//
	//Yield the rest of our time slice so the dialog checking thread can 
	//notice the event was set
	//
	Sleep(0);

	CloseHandle(g_hDXInstallEvent);

	return retc;
}

UINT _stdcall CheckForDXCopyDialog(void *pv)
{
	//
	//Check every second until event is signaled by engine thread or the
	//event handle is closed by the engine thread
	//
	while (WAIT_TIMEOUT == WaitForSingleObject(g_hDXInstallEvent, 1000))
	{
		//
		//Enumerate all child windows of the desktop, looking for a file copy dialog
		//
		EnumWindows((WNDENUMPROC) EnumSetupChildProc, 0);
	}

	return 0;
}

BOOL CALLBACK EnumSetupChildProc(HWND hWndParent, LPARAM lParam)
{
	HWND hWnd;
	static HWND hWndOld = NULL;

	//
	//Look (by classname) for the copying files dialog that DirectX uses
	//
	hWnd = FindWindowEx(hWndParent, NULL, "setupx_progress", NULL);

	//
	//If we found the window, and it's a different one than we found previously,
	//bring it to the foreground.  Note that if the window we found is the same
	//one that we found previously, we don't want to put it in the foreground 
	//automatically because the user may have deliberately moved it the background...
	//
	if (hWnd && hWnd != hWndOld)
	{
		SetForegroundWindow(hWndParent);
		hWndOld = hWnd;

		//
		//We found what we're looking for, we can stop enumerating...
		//
		return FALSE;
	}

	return TRUE;
}

// CheckDXVersion -- a utility function that can be used to check the current version of 
//                   DirectX installed to the version you want to install.  
// Parameters --     BOOL - are we running on NT?
//                   WORD params -- the minimum Dx version to check against
DIRECT_X_VERSION CheckDXVersion( char *pszDXVerString, char *pszDXDllName )
{
	TCHAR			 szDXTmp[MAX_PATH];
	DWORD			 dwVersion;
	DWORD			 dwRevision;
	HINSTANCE		 hDXDll;
	DXVERPROC		 lpfn;
	WORD			 wDXVersion[] = {0,0,0,0};
	DIRECT_X_VERSION uExistingVersion = EV_EXISTING_SAME;

	ASSERT(pszDXVerString);

	ASSERT(pszDXDllName);
	lstrcpy(szDXTmp, pszDXDllName);
	ReplaceStringTokens(szDXTmp, sizeof(szDXTmp));

	//
	//Load DSETUP.DLL
	//
	hDXDll = LoadLibrary(szDXTmp);
	if (NULL == hDXDll)
	{
		return EV_ERROR;
	}

	//
	//Get version function address
	//
	lpfn = (DXVERPROC) GetProcAddress(hDXDll, "DirectXSetupGetVersion");
	if (NULL == lpfn)
	{
		FreeLibrary(hDXDll);
		return EV_ERROR;
	}

	//
	//If function returned an error, assume that DirectX is not installed...
	//
	if (0 == (*lpfn) (&dwVersion, &dwRevision))
	{
		FreeLibrary(hDXDll);
		return EV_NOT_INSTALLED;
	}

	FreeLibrary(hDXDll);

	//
	//got version info ok
	//
	wDXVersion[0] = HIWORD(dwVersion);
	wDXVersion[1] = LOWORD(dwVersion);
	wDXVersion[2] = HIWORD(dwRevision);
	wDXVersion[3] = LOWORD(dwRevision);

	// figure out what our min version is
	WORD wMinVersion[] = {0,0,0,0};
		
	TCHAR szTemp[MAX_PATH];
	TCHAR *szToken;

	lstrcpy(szTemp, pszDXVerString);

	szToken = _tcstok(szTemp, ".");
	wMinVersion[0] = atoi(szToken);
	szToken = _tcstok(NULL, ".");
	wMinVersion[1] = atoi(szToken);
	szToken = _tcstok(NULL, ".");
	wMinVersion[2] = atoi(szToken);
	szToken = _tcstok(NULL, ".");
	wMinVersion[3] = atoi(szToken);

	//
	//compare the existing version with our min version
	//
	uExistingVersion = EV_EXISTING_SAME;

	for (int i = 0; i < 4; i++)
	{
		if (wMinVersion[i] == wDXVersion[i])
		{
			// same try next part
			continue;
		}

		if (wMinVersion[i] < wDXVersion[i])
		{
			// they have a newer version then we require
			uExistingVersion = EV_EXISTING_NEWER;
		}
		else
		{
			// they have an older version then we require
			uExistingVersion = EV_EXISTING_OLDER;
		}

		break; // done
	}

	return uExistingVersion;
}

// CheckDPLAYVersion -- a utility function that can be used to check the current version of 
//						DirectPlay installed to the version you want to install.  
//
//						A separate direct play install is provide because the DPLAY version in DirectX setup 
//						might not be the most recent version of DPLAY.
//
// PARAMS:
// char	*pszDPLAYVerString minimum Dx version to check against
//
DIRECT_X_VERSION CheckDPLAYVersion(char *pszDPLAYVerString)
{
	FILEINFO		 DXVerInfo;
	WORD			 wDXVersion[] = {0,0,0,0};
	DIRECT_X_VERSION uExistingVersion = EV_EXISTING_SAME;
	WORD			 wMinVersion[] = {0,0,0,0};
	TCHAR			 szTemp[MAX_PATH];
	TCHAR			 *szToken;
	UINT			 uFileInfoResult;
	char			 szSystemDir[_MAX_PATH];

	ASSERT(pszDPLAYVerString);

	//Find out what version (if any) of the dplayX.dll they have currently installed.
	GetSystemDirectory(szSystemDir, _MAX_PATH);

	lstrcat(szSystemDir, "\\DPLAYX.DLL");

	uFileInfoResult = EBUFileInfo(szSystemDir, &DXVerInfo);

	if (uFileInfoResult)
	{
		//
		//some sort of error
		//
		if (uFileInfoResult && FI_ERR_NOEXIST)
		{
			return EV_NOT_INSTALLED;
		}
		else
		{
			return EV_ERROR;
		}
	}
	else
	{
		//
		//got version info ok
		//
		wDXVersion[0] = HIWORD(DXVerInfo.dwFileVersionMS);
		wDXVersion[1] = LOWORD(DXVerInfo.dwFileVersionMS);
		wDXVersion[2] = HIWORD(DXVerInfo.dwFileVersionLS);
		wDXVersion[3] = LOWORD(DXVerInfo.dwFileVersionLS);
	}

	// figure out what our min version is
	lstrcpy(szTemp, pszDPLAYVerString);

	szToken = _tcstok(szTemp, ".");
	wMinVersion[0] = atoi(szToken);
	szToken = _tcstok(NULL, ".");
	wMinVersion[1] = atoi(szToken);
	szToken = _tcstok(NULL, ".");
	wMinVersion[2] = atoi(szToken);
	szToken = _tcstok(NULL, ".");
	wMinVersion[3] = atoi(szToken);

	//compare the existing version with our min version
	uExistingVersion = EV_EXISTING_SAME;

	for (int i = 0; i < 4; i++)
	{
		if (wMinVersion[i] == wDXVersion[i])
		{
			// same try next part
			continue;
		}

		if (wMinVersion[i] < wDXVersion[i])
		{
			// they have a newer version then we require
			uExistingVersion = EV_EXISTING_NEWER;
		}
		else
		{
			// they have an older version then we require
			uExistingVersion = EV_EXISTING_OLDER;
		}

		break; // done
	}

	return uExistingVersion;
}

// Registers the game being installed with the DPLAY lobby
//	returns if dirpath not set (no INSTDPLAY before INSTALLGO )
//	returns if GetAppDir returns null (INSTDPLAY before GETROOT)
//	returns if STR_APP_GUID not set (can't register app without a GUID)
 
void InstDPLAYRegApp()
{
	HINSTANCE	hLib;
	DPLAYREGPROC	lpfnRegApp;
	DIRECTXREGISTERAPP	DXRegApp;
	GUID	AppGUID;
	char	szAppTitle[MAX_PATH];
	char	szFileName[MAX_PATH];
	char	szCmdLine[MAX_PATH];
	char	szDestDir[MAX_PATH];
	char	szAppGUID[MAX_PATH];

	char cmd[_MAX_PATH];
	char	*ptr;

	return;
	if ( dirpath[0] == 0 )
		return;
	else
	{
		lstrcpy(cmd, dirpath);
		lstrcat ( cmd, "\\DSETUP" );
		if((hLib = LoadLibrary(cmd)) > (HINSTANCE)32)
		{
		
			lpfnRegApp = (DPLAYREGPROC) GetProcAddress(hLib, "DirectXRegisterApplicationA" );

			ZeroMemory ( &DXRegApp, sizeof(DXRegApp) );
			DXRegApp.dwSize	 = sizeof(DXRegApp);

			EBULoadString(GetResourceInst(), STR_SETUP_APPTITLE, szAppTitle, sizeof(szAppTitle) );
			if ( strncmp( szAppTitle, "RESERR ", 7 ) )
				DXRegApp.lpszApplicationName = "";
			else
				DXRegApp.lpszApplicationName = szAppTitle;

			EBULoadString(GetResourceInst(), STR_LAUNCHEXE, szFileName, sizeof(szFileName) );
			if ( strncmp( szFileName, "RESERR ", 7 ) )
				goto wayout;
			else
			{
				ptr = &szFileName[lstrlen(szFileName)-1];
				while ( *ptr != '\\' ) ptr--;
				if ( ptr == szFileName )
				{
					DXRegApp.lpszFilename = szFileName;
					ptr = NULL;
				}
				else
				{
					*ptr = 0;
					DXRegApp.lpszFilename = ++ptr;
				}
			}

			EBULoadString(GetResourceInst(), STR_DPLAYCOMMANDLINE, szCmdLine, sizeof(szCmdLine) );
			if ( strncmp( szCmdLine, "RESERR ", 7 ) )
				DXRegApp.lpszCommandLine = "";
			else
				DXRegApp.lpszCommandLine = szCmdLine;
	
			lstrcpy ( szDestDir, GetAppDir() );
			if ( szDestDir[0] == 0 )
				goto wayout;

			if ( ptr != NULL )
				lstrcat ( szDestDir, szFileName);
			DXRegApp.lpszPath = szDestDir;
			DXRegApp.lpszCurrentDirectory = szDestDir;

			EBULoadString(GetResourceInst(), STR_APP_GUID, szAppGUID, sizeof(szAppGUID) );
			if ( strncmp( szAppGUID, "RESERR ", 7 ) )
				goto wayout;
			else
			{
				CLSIDFromString ( (LPOLESTR)szAppGUID, (LPCLSID)(&AppGUID) );
				DXRegApp.lpGUID = &AppGUID;
			}

			if(lpfnRegApp != NULL)
				lpfnRegApp ( GetWndParent(), &DXRegApp );

wayout:
			FreeLibrary(hLib);
		}
	}
}

EBURETCODE InstDPLAY(LPINSTDPLAY lpInstDPLAY,DIRECT_X_VERSION *uExistingVersion)
{
	DIRECTPLAYDATA dpd;
	EBURETCODE nShouldInstall = EBU_CANCEL;

	char cmd[_MAX_PATH],cmd2[_MAX_PATH];
	char dir[_MAX_PATH];

	lstrcpy(cmd, lpInstDPLAY->GetInstDPLAYName());
	// TODO:  Why is this returning EBU_OK???????
	if(!lstrcmpi(cmd, "NULL") || *cmd == '\0') // no path to DP 5 installer
		return EBU_OK;
	
	// Launch in the current directory
	GetCurrentDirectory(_MAX_PATH, dir);
	ReplaceStringTokens(cmd, _MAX_PATH);  
	lstrcpy(cmd2,cmd);					// must get short path name, so copy string
	
	char *ptr = cmd2 + lstrlen(cmd2);	// walk from end of string to first '\', terminate string
	BOOL bChange = FALSE;				// flag for change.  
										// NOTE: this isn't DBCS friendly, but  an EXE is never DBCS
	
	while(*ptr != '\\' && ptr != cmd2)	// find first back slash from end of string
		ptr--;
	
	if(*ptr == '\\')
	{
		bChange = TRUE;					// flag change and terminate string
		*ptr = '\0';
	}
// BUGBUG we are screwed if there were no backslashes in cmd2
	lstrcpy(dirpath, cmd2);
	
	GetShortPathName(cmd2,cmd2,_MAX_PATH);	// get short path name
	lstrcpy(cmd,cmd2);						// copy short path to original buffer

	if(bChange)								// if we changed the name, then copy the exe and command parameters
	{										// to the short path name
		*ptr = '\\';
		lstrcat(cmd,ptr);
	}

	// callback the ui with the version information and get how to proceed.
	dpd.uExistingVersion = *uExistingVersion;
	dpd.nStatus = QUERYDPLAYINSTALL;
	nShouldInstall = (*(GetAppCallback())) ((void *) &dpd);

	switch (nShouldInstall)
	{
	// Something went wrong with callback abort.
	case EBU_ABORT:
		// Tell the engine nothing was done.
		SetDPLAYReturnCode(DSETUPERR_NOCOPY);
		return EBU_ABORT;

	// cancel means don't install because we are already installed.
	// We still need to register though
	case EBU_CANCEL:
		InstDPLAYRegApp();
		break;

	case EBU_OK:
		{
		//
		// Launch DirectPlay setup app, wait until it terminates,
		// Set reboot flag if required, and then cleanup process.
		//
		TCHAR *pszParms;
		TCHAR *pszPtr;
		BOOL  fInQuote;
		
		pszParms = NULL;
		pszPtr = cmd;
		fInQuote = FALSE;
		
		//
		//Separate command line (if any) from exe name...
		//
		while (*pszPtr && NULL == pszParms)
		{
			switch (*pszPtr)
			{
			case '\"':
				fInQuote = TRUE == fInQuote ? FALSE : TRUE;
				break;
				
			case '\t':
			case ' ':
				if (FALSE == fInQuote)
				{
					pszParms = CharNext(pszPtr);
					*pszPtr = '\0';
				}
				break;
			}
			
			pszPtr = CharNext(pszPtr);
		}
		
		//
		//Remove any leading white space from parms...
		//
		if (pszParms)
		{
			while (*pszParms && (' ' == *pszParms || '\t' == *pszParms))
			{
				pszParms = CharNext(pszParms);
			}
			
			//
			//If there was nothing but white space, NULL out parms...
			//
			if (!*pszParms)
			{
				pszParms = NULL;
			}
		}
		
		//
		//Clean up after any previous DirectPlay install...
		//
		NukeDirectPlayRemnants();
		
		//
		//Launch DirectPlay install and wait for it to finish...
		//
		EBURETCODE RetCode = EBU_OK;

		//BUGBUG:  How are we determining with this call whether or not requires the reboot flag to be set?
		// a-petere.
		RetCode = EBUShellExecute(GetWndParent(),
								  cmd,
								  pszParms,
								  dir,
								  SW_HIDE,
								  EBUENGINE_SHELLEXECUTE,
								  0,
								  TRUE,  //wait
								  NULL);
		
		//
		//Clean up after this DirectPlay install...
		//
		NukeDirectPlayRemnants();

		InstDPLAYRegApp();
		return RetCode;
	}
	break;
	}
	return nShouldInstall;
}


EBURETCODE UnInstDPLAY(LPINSTDPLAY lpInstDPLAY,DIRECT_X_VERSION *uExistingVersion)
{
	DIRECTPLAYDATA dpd;
	EBURETCODE retcode;
	char cmd[_MAX_PATH];

		return EBU_OK;
	lstrcpy(cmd, lpInstDPLAY->GetInstDPLAYName());
	// TODO:  Why is this returning EBU_OK???????
	if(!lstrcmpi(cmd, "NULL") || *cmd == '\0') // no path to DP 5 installer
		return EBU_OK;
	
	ReplaceStringTokens(cmd, _MAX_PATH);  
	
	char *ptr = cmd + lstrlen(cmd);	// walk from end of string to first '\', terminate string
	
	while(*ptr != '\\' && ptr != cmd)	// find first back slash from end of string
		ptr--;
	
	if(*ptr == '\\')
	{
		*ptr = '\0';
	}

	// callback the ui with the version information and get how to proceed.
	dpd.uExistingVersion = *uExistingVersion;
	dpd.nStatus = QUERYDPLAYINSTALL;
	retcode = (*(GetAppCallback())) ((void *) &dpd);

	if ( dpd.lpDXRegApp )
	{
		HINSTANCE	hLib;
		DPLAYUNREGPROC	lpfnUnRegApp;
		GUID	AppGUID;
		char	szAppGUID[MAX_PATH];

		lstrcat ( cmd, "\\DSETUP" );

		if((hLib = LoadLibrary(cmd)) <= (HINSTANCE)32)
		{
			lstrcpy ( cmd, GetAppDir() );
			lstrcat ( cmd, "\\DSETUP" );

			if((hLib = LoadLibrary(cmd)) <= (HINSTANCE)32)
				return EBU_ERROR;
		}
	
		lpfnUnRegApp = (DPLAYUNREGPROC) GetProcAddress(hLib, "DirectXUnRegisterApplication" );
		if(lpfnUnRegApp != NULL)
		{
			EBULoadString(GetResourceInst(), STR_APP_GUID, szAppGUID, sizeof(szAppGUID) );
			if ( !strncmp( szAppGUID, "RESERR ", 7 ) )
			{
				CLSIDFromString ( (LPOLESTR)szAppGUID, (LPCLSID)(&AppGUID) );
				lpfnUnRegApp ( GetWndParent(), &AppGUID );
			}
		}

		FreeLibrary(hLib);

	}
	return EBU_OK;
}


//
//BUGBUG:TODO:Nuke this when DirectPlay group fixes installer bug...
//
void DelIXPDir()
{
	WIN32_FIND_DATA   
		FindData;
	HANDLE            
		hFind = NULL;
	BOOL
		bFindFile = TRUE;
	char 
		szFile[_MAX_PATH] = {""},
		szTemp[_MAX_PATH] = {"%TEMPDIR\\IXP*.tmp"};

	ReplaceStringTokens(szTemp, _MAX_PATH);

	// need to step through all the files in this directory and delete them first
	hFind = FindFirstFile(szTemp, &FindData);
	while((INVALID_HANDLE_VALUE != hFind) && bFindFile)
	{
		if(*(FindData.cFileName) != '.')
		{
			lstrcpy(szFile, "%TEMPDIR\\");
			ReplaceStringTokens(szFile, _MAX_PATH);

			if(*FindData.cAlternateFileName != '\0')
				lstrcat(szFile,FindData.cAlternateFileName);
			else
				lstrcat(szFile,FindData.cFileName);

			if (0xFFFFFFFF != GetFileAttributes(szFile))
			{
				DelTree(szFile);
			}
		}
		//find the next file
		bFindFile = FindNextFile(hFind, &FindData);
	}

	if(hFind != INVALID_HANDLE_VALUE)
		FindClose(hFind);

	return;
}

VOID NukeDirectPlayRemnants(void)
{
	TCHAR szFile[_MAX_PATH];
	TCHAR szTemp[_MAX_PATH] = "IXP00";

	HKEY  hkMRU;
	DWORD dwResult;
	TCHAR szMRU[64];
	ULONG uSize = sizeof(szMRU);
	
	dwResult = RegOpenKeyEx(HKEY_CURRENT_USER, 
							"InstallLocationsMRU",
							NULL,
							KEY_READ | KEY_WRITE,
							&hkMRU);

	if (ERROR_SUCCESS == dwResult)
	{
		dwResult = RegQueryValueEx(hkMRU,
								   "MRUList",
								   NULL,
								   NULL,
								   (BYTE *) szMRU,
								   &uSize);

		if (ERROR_SUCCESS == dwResult)
		{
			TCHAR *pszMRU = szMRU;
			TCHAR szBuf[2] = "";

			uSize = sizeof(szFile);

			while (*pszMRU)
			{
				*szBuf = *pszMRU;

				dwResult = RegQueryValueEx(hkMRU,
										   szBuf,
										   NULL,
										   NULL,
										   (BYTE *) szFile,
										   &uSize);

				if (ERROR_SUCCESS == dwResult)
				{
					CharUpper(szFile);
					CharUpper(szTemp);

					if (EBUstrstr(szFile, szTemp))
					{
						RegDeleteValue(hkMRU, szBuf);
					}
				}

				pszMRU = CharNext(pszMRU);
			}
		}

		RegCloseKey(hkMRU);
	}

	//
	//If the DPlay temp directory exists, delete the files and then
	//remove the directory...
	//

	DelIXPDir();
}

