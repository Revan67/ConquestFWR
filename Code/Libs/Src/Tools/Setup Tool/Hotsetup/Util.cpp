//
// util.cpp
//
// Utility functions for stub
//

#include "hotsetup.h"
#include "stubpch.h"
#include "setup.h"
#include "hotsetuprc.h"
#include "regstr.h"
#include "registry.h"
#include "util.h"
#include "hotsetuprc.h"
#include "winsvc.h"
extern "C" {
#include "intshcut.h"
#include "shlguid.h"
#include <shlobj.h>
}
#include "diskinfo.h"

#include <richedit.h>


using namespace NGLOBALS;

extern BOOL GetBootstrapFileName(TCHAR *pszSrcPath, TCHAR *pszDestPath, TCHAR *pszExtension);
BOOL FLoadResourceDLL(LCID lcid, ALERTWINPROC AlertWinProc = NULL);
BOOL BootstrapResourceDll(TCHAR *pszResourceDllName);
static keyboardType getKeyboardType();
extern BOOL EnableMediaSwap();
ALERTWINPROC fnAlert=NULL;

static DWORD playWAVEFile(HWND hWndNotify, LPCTSTR lpszWAVEFileName, DWORD dwPlayFlags);	// only in this file




//****************************************************************************
// Procedure    DoesFileExist
//
// Purpose      Determines whether the given file exists on disk
//
// Parameters   lpszFilename- full pathname of the file to look for
//
// Returns      nonzero if the file exists, zero if not
//
// History:
//  1/26/95        KenSh           Created
//
BOOL DoesFileExist( LPCSTR lpszFilename )
{
	return (0xFFFFFFFF != GetFileAttributes(lpszFilename));
}




//****************************************************************************
// Procedure    MyGetWindowsDirectory
//
// Purpose      Like GetWindowsDirectory() but terminates with a backslash
//
// Parameters   lpszBuf- address of buffer for Windows directory
//				cbBuf- size of directory buffer
//
// Returns      Length of buffer in characters.
//
UINT MyGetWindowsDirectory( LPSTR lpszBuf, UINT cbBuf )
{
	UINT cch;

	cch = GetWindowsDirectory( lpszBuf, cbBuf );
	if( cch > 3 )
	{
		//Make sure buffer is big enough
		ASSERT( cch < cbBuf-1 );
		lpszBuf[cch] = '\\';
        lpszBuf[cch+1] = '\0'; // To be safe
		return cch+1;
	}
	else
	{
		return cch;
	}
}
//****************************************************************************
// Procedure    MyGetSystemDirectory
//
// Purpose      Like GetSystemDirectory() but terminates with a backslash
//
// Parameters   lpszBuf- address of buffer for Windows directory
//				cbBuf- size of directory buffer
//
// Returns      Length of buffer in characters.
//
UINT MyGetSystemDirectory( LPSTR lpszBuf, UINT cbBuf )
{
	UINT cch;

	cch = GetSystemDirectory( lpszBuf, cbBuf );
	if( cch > 3 )
	{
		//Make sure buffer is big enough
		ASSERT( cch < cbBuf-1 );
		lpszBuf[cch] = '\\';
		lpszBuf[cch+1] = '\0';
		return cch+1;
	}
	else
	{
		return cch;
	}
}

//****************************************************************************
// Procedure    GetWindowsDriveRoot
//
// Purpose      Gets the root of the WindowsDirectory.
//
// Parameters   lpszBuf- address of buffer for Windows directory
//				cbBuf- size of directory buffer
//
// Returns      void.
//
void GetWindowsDriveRoot( LPSTR lpszBuf, UINT cbBuf )
{
	static char chDrive = 0;

	if( !chDrive )
	{
		char szWindowsDir[_MAX_PATH];
		GetWindowsDirectory( szWindowsDir, sizeof(szWindowsDir) );
		chDrive = szWindowsDir[0];
	}
// TODO:  do we really want an assert here or should it be a
// "if (cbBuf >=4) { ...}	 --mel
	ASSERT( cbBuf >= 4 );
	lpszBuf[0] = chDrive;
	lpszBuf[1] = ':';
	lpszBuf[2] = '\\';
	lpszBuf[3] = 0;
}

//****************************************************************************
// Procedure    GetModuleDirectory
//
// Purpose      Returns directory in which this stub exe sits,
//				and terminates it with a backslash.
//
// Parameters   lpszBuf- address of buffer for Windows directory
//				cbBuf- size of directory buffer
//
// Returns      Number of chars copied not including the NULL terminator.
//
UINT GetModuleDirectory( LPSTR lpszBuf, UINT cbBuf )
{
	static UINT cch = 0;
	TCHAR *pszPtr;

	lstrcpy(lpszBuf, GetSourcePath());
	lstrcat(lpszBuf, "\\");

	if (0 == cch)
	{
		pszPtr = lpszBuf;
		while (*pszPtr)
		{
			cch++;
			pszPtr = CharNext(pszPtr);
		}
	}

	return cch;
}


//****************************************************************************
// Procedure    GetCurrentOperatingSystem
//
// Purpose      Determines what platform we're currently running on
//
// Parameters   none
//
// History
//  1/31/95        KenSh           Created
//
WORD GetCurrentOperatingSystem()
{
	static WORD wResult = 0;

	if( !wResult )
	{
		OSVERSIONINFO 
			OSVersionInfo;

		OSVersionInfo.dwOSVersionInfoSize = sizeof(OSVersionInfo);
		GetVersionEx(&OSVersionInfo);

		switch(OSVersionInfo.dwPlatformId)
		{
		case VER_PLATFORM_WIN32_WINDOWS:
			wResult = (10 == OSVersionInfo.dwMinorVersion ? OS_WIN98 : OS_WIN95);

			// check to see if we are running on OSR2 or higher
			SetWin95NotOSR2(LOWORD(OSVersionInfo.dwBuildNumber) > 1000 ? FALSE : TRUE);

			break;

		case VER_PLATFORM_WIN32_NT:
			SetWin95NotOSR2(FALSE);

			switch (OSVersionInfo.dwMajorVersion)
			{
			case 0:
			case 1:
			case 2:
			case 3:
				wResult = OS_NOTSUPPORTED;
				break;
				
			case 4:	// nt 4.0
				wResult = OS_NT40;
				break;

			case 5:	// nt 5.0
				wResult = OS_NT50;
				break;

			default:
				//unexpected NT version
				ASSERT(FALSE);
			}

			break;

		default:
			// unexpected platform ID
			ASSERT(FALSE);
		}
	}

	return wResult;
}



//****************************************************************************
// Procedure    Alert
//
// Purpose      Displays a message box using a format string in the resource file.
//
// Parameters   hwndParent- the window to use as a parent, may be NULL
//              uType- MessageBox() style
//              uMessage-  resource string to use
//              parameters for wvsprintf
//
// Returns      int- As returned by MessageBox()
//
int __cdecl Alert( HWND hwndParent, UINT uType, UINT uMessage, ... )
{
	va_list	vlShow;
	TCHAR	szBuffer[1024]= STR_HARDCODE_DEFAULTBUFFER;
	TCHAR	szText[2048]= STR_HARDCODE_NOSTRINGLOADED;
	UINT	uStr;

	// Make the parent window no longer always on top.
	SetWindowPos( hwndParent, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE );

	// application specific messages are stored in the language dll
	uStr = EBULoadString(GetResourceInst(), uMessage, szBuffer, sizeof(szBuffer));
	ASSERT(szBuffer[0]);

	if (uStr)
	{
		va_start (vlShow, uMessage);
		wvsprintf (szText, szBuffer, vlShow);
		va_end (vlShow);
	}
	else
	{
		wsprintf(szText, STR_HARDCODE_LOADSTRINGFAILED, uMessage);
	}

	ReplaceStringTokens(szText, sizeof(szText));

	if( hwndParent )
		uType |= MB_APPLMODAL;
	else
		uType |= MB_TASKMODAL;

	//MessageBeep( uType & MB_ICONMASK );
	if (!fnAlert)
	{
	    return MessageBox( hwndParent, szText, GetSetupTitle(), uType );
	}
	else
	{
		return (*fnAlert)(hwndParent,szText,GetSetupTitle(),uType);
	}
}



//****************************************************************************
// Procedure    ForwardMessage
//
// Purpose      Lets another process do what it needs to when we're in the
//              middle of something lengthy.
//
// Parameters   none
//
// Returns      nonzero if all messages were removed from the queue; zero
//                              if some are left when we return.
//
// History
//10/06/94 KenSh           Don't remove WM_CLOSE, WM_QUIT, etc.,from the queue.  Screws up in Win32.
//
BOOL ForwardMessages()
{
	MSG   msg;

	while (PeekMessage (&msg, NULL, 0, 0, PM_REMOVE))
	{
		if( msg.message == WM_QUIT ||
			msg.message == WM_CLOSE ||
			msg.message == WM_DESTROY )
		{
			// Put the message back on the queue and get out of here.
		    PostMessage( msg.hwnd, msg.message, msg.wParam, msg.lParam );
			return FALSE;
		}

		TranslateMessage (&msg);
		DispatchMessage (&msg);
	}

	return TRUE;
}

//****************************************************************************
// pszGetLast5C
//    get pointer to last 0x5c character in string
//
//****************************************************************************
LPSTR pszGetLast5C( LPCSTR psz )
{
    LPCSTR psz5C = NULL;
    while ( *psz )
    {
        if ( *psz == '\\' )
            psz5C = psz;
        psz = AnsiNext( psz );
    }

    return (LPSTR)psz5C;
}

//****************************************************************************
//
// HACK - see if we already got full path of the destination
//        by detecting the drive letter
//
// History:
//      07/10/96    HidemitA    Extracted from
//                              ExecuteInstallFile/ExecuteUnInstallFile
//
//****************************************************************************
BOOL FIsFullPath( LPCSTR lpszFileName )
{
    LPCSTR lpTmp1;
    LPCSTR lpTmp2;
    lpTmp1 = CharNext( lpszFileName );
    lpTmp2 = CharNext( lpTmp1 );

    return ( isalpha(*lpszFileName) && *lpTmp1 == ':' && *lpTmp2 == '\\' );
}

BOOL FLoadResourceDLL(LCID lcid, ALERTWINPROC AlertWinProc)
{
	if (AlertWinProc)
	{
		fnAlert = AlertWinProc;
	}
	
	TCHAR  lpszLangSub[4];
	TCHAR  lpszLangBase[4];
	TCHAR  szResourceDllName[256] = "";
	HANDLE hRsrc;

	//
	//Use DLL name from command line if one passed in
	//
	lstrcpy(szResourceDllName, GetResDLLPath());
	if (!*szResourceDllName)
	{
		//
		//If default RESDLL name specified in .EXE file, use that.  Else build DLL name
		//based upon lcid passed in...
		//
		if (0 == LoadString((HINSTANCE) GetModuleHandle(NULL), STR_DEFAULTRESDLL, szResourceDllName, 256))
		{
			if (LOCALE_USER_DEFAULT == lcid)
			{
				lcid = GetUserDefaultLCID();	
			}

			GetLocaleInfo(lcid, LOCALE_SABBREVLANGNAME, lpszLangSub, 4);
			wsprintf(szResourceDllName, "%s%s", "Setup", lpszLangSub);
		}
	}
	
#ifdef _DEBUG
	TRACE(STR_HARDCODE_LOADLANGUAGEDLLFAILED, szResourceDllName);
#endif

	if (FALSE == BootstrapResourceDll(szResourceDllName))
	{
		lcid = MAKELANGID(PRIMARYLANGID(lcid), SUBLANG_DEFAULT);
		GetLocaleInfo(lcid, LOCALE_SABBREVLANGNAME, lpszLangBase, 4);
		wsprintf(szResourceDllName, "%s%s", "Setup", lpszLangBase);
		
#ifdef _DEBUG
		TRACE("Trying to load language specific dll for base language %s.\n", szResourceDllName);
#endif
		
		if (FALSE == BootstrapResourceDll(szResourceDllName))
		{
#ifdef _DEBUG
			TRACE("Unable to load language specific.  Defaulting to english language.\n");
#endif
			if (FALSE == BootstrapResourceDll("SETUPENU.DLL"))
			{
				MessageBox(NULL, "Unable to find the language specific dll unable to continue.", "Setup", MB_OK);

				return (FALSE);
			}
		}
	}

	// we have found a resource dll so figure out where we are going to get our script resources from
	if((hRsrc = EBUFindResource(GetResourceInst(),"SETUPDATA",(LPCSTR)"SETUPBINARY")) != NULL)
	{
		SetScriptInst(GetResourceInst());
		SetBinaryResource(TRUE);
    }
	else
	{
		if((hRsrc = EBUFindResource(GetAppInst(),"SETUPDATA",(LPCSTR)"SETUPBINARY")) != NULL)
		{
			SetScriptInst(GetAppInst());
			SetBinaryResource(TRUE);
		}
		else
		{
			Alert( NULL, MB_OK | MB_ICONSTOP, STR_ERROR_RESOURCEFAILURE);
			return FALSE;
		}
	}

	return (TRUE);
}

//****************************************************************************
// Procedure	BootstrapResourceDll
BOOL BootstrapResourceDll(TCHAR *pszResourceDllName)
{
	HINSTANCE hResInst;
	TCHAR     szResDll[MAX_PATH];
	TCHAR	  szTempResDllPath[MAX_PATH];

	//
	//Attempt to load resource DLL
	//
	hResInst = LoadLibrary(pszResourceDllName);

	//
	//If load failed, exit...
	//
	if ((int) hResInst < 32)
	{
		return FALSE;
	}

	//
	//Get path to Resource DLL...
	//
	GetModuleFileName(hResInst, szResDll, sizeof(szResDll));
	
	// For Old Single CD installs signature file
	SetDiscIDPath(szResDll);

	//
	//If the current running .EXE is named the same as the defined uninstaller name,
	//then flag that we are removing the app...
	//
	EBULoadString(hResInst, 
				  STR_UNSETUPEXENAME, 
				  szTempResDllPath, 
				  sizeof(szTempResDllPath));

	if (0 == lstrcmpi(GetSetupExeName(), szTempResDllPath))
	{
		SetRemovingApp(TRUE);
		SetPromptDelete(TRUE);
		SetRanUninstall(TRUE);
	}

	//
	//Don't bootstrap setupenu.dll if uninstalling (bootstrap == false)
	//
	if (FALSE == GetBootstrapFlag())
	{
		SetResourceInst(hResInst);

		return TRUE;
	}

	FreeLibrary(hResInst);

	if (0 == GetShortPathName(szResDll, szTempResDllPath, sizeof(szTempResDllPath)))
	{
		lstrcpy(szTempResDllPath, szResDll);
	}
	ASSERT(*szTempResDllPath);

	GetBootstrapFileName(szTempResDllPath, szResDll, "DLL");

	//
	//Make a copy of the setup resource DLL
	//
	if (0 == EBUCopyFile(szTempResDllPath, szResDll, FALSE))
	{
		return FALSE;
	}

	//
	//Remove any read-only bit from our temp .EXE
	//
	SetFileAttributes(szResDll, FILE_ATTRIBUTE_NORMAL);

	hResInst = LoadLibrary(szResDll);

	SetResourceInst(hResInst);

	return (int) hResInst < 32 ? FALSE : TRUE;
}

//****************************************************************************
// Procedure	DLLRegister
//
// Purpose		Register or Unregister a DLL by calling the appropriate exposed
//				proc
//
// Parameters	
//				sz				path to DLL
//				bProcessType	DO_INSTALL or DO_UNINSTALL
//
// Returns		none
//
// History      4/14/97 a-drews	Created
//
VOID DLLRegister(const char *sz, BYTE bProcessType)
{
	int nIdx;

	//
	//Initialize possible spellings of register and unregister functions...
	//
	const TCHAR *rgszReg[] = {"DLLRegisterServer",
							  "DllRegisterServer",
							  "DLLregisterServer",
							  "DllregisterServer",
							  ""};

	const TCHAR *rgszUnReg[] = {"DLLUnRegisterServer",
								"DllUnRegisterServer",
								"DLLUnregisterServer",
								"DllUnregisterServer",
								""};
	TCHAR **ppsz;

	HRESULT (WINAPI *lpfn) (void);

	ASSERT(NULL != sz);
	ASSERT(DO_INSTALL == bProcessType || DO_UNINSTALL == bProcessType);

	if (FAILED(OleInitialize(NULL)))
	{
		Alert(GetWndParent(), MB_OK | MB_ICONWARNING, STR_ERROR_OLEINITFAILED);

		return;
	}

	//
	//Load the DLL
	//
	HMODULE hModule = LoadLibrary(sz);
	if (NULL == hModule)
	{
#ifdef _DEBUG
		TRACE("DLLRegister: LoadLibrary(%s) failed.\n", sz);
#endif

		goto Done;
	}

	//
	//use the RegServer or UnRegServer list depending....
	//
	ppsz = DO_INSTALL == bProcessType ? (TCHAR **) &rgszReg : (TCHAR **) &rgszUnReg;

	//
	//Try to GetProcAddress for each spelling in the list, break if found...
	//
	for (nIdx = 0; **ppsz; nIdx++)
	{
		lpfn = (HRESULT (WINAPI *) (void)) GetProcAddress(hModule, *ppsz);

		if (NULL != lpfn)
		{
			break;
		}

		ppsz++;
	}

	if (lpfn != NULL)
	{
		HRESULT regResult = lpfn();
	}
#ifdef _DEBUG
	else
	{
		TRACE("DLLRegister: No entry point in %s.\n", sz);
	}
#endif

	FreeLibrary(hModule);
	
Done:
	OleUninitialize();
}

//****************************************************************************
// Procedure	AddString
//
// Purpose		Adds a string to the end of the buffer and places the pointer
//				at the end of the new string. Note that I'm not check for the end
//				of the buffer so the caller better be confident that the buffer is
//				big enough.
//
// Parameters	ppchDest	String we will be adding to.
//				szSource	string to be added.
//
// Returns		none
//
// History		4/23/97 a-drews		Created
//
VOID AddString(char **ppchDest, char *szSource)
{
	lstrcpy(*ppchDest, szSource);

	*ppchDest = *ppchDest + lstrlen(szSource);
}

LONG NEAR GetProgramFilesLocation(char *buf, DWORD cbuf)
{
	char
		szKeyName[25] = "";
	
	EBULoadString(GetResourceInst(), STR_REGKEY_VAL_PROGRAMFILESDIR, szKeyName, 25);

	return MyGetUniversalIniString(HKEY_LOCAL_MACHINE, REGSTR_PATH_SETUP, szKeyName, "", buf, cbuf);
}


LONG NEAR GetCommonFilesLocation(char *buf,DWORD cbuf)
{
	char
		szKeyName[25] = "";
	
	EBULoadString(GetResourceInst(), STR_REGKEY_VAL_COMMONFILESDIR, szKeyName, 25);

	return MyGetUniversalIniString(HKEY_LOCAL_MACHINE, REGSTR_PATH_SETUP, szKeyName, "", buf, cbuf);
}


//****************************************************************************
// Procedure	ReplaceStringTokens
//
// Purpose		Replaces %KEYWORD in string with with appropriate string
//				Keywords supported:
//				%PLAYERNAME - Name of current player.
//				%APPPATH - Path to which app is being installed.
//				%APPTITLE - Title of app being installed.
//				%SETUPTITLE - Title of the setup app
//				%PID - PID specified by user.
//				%SETUPEXEDIR - Path to dir which contains setup.exe.
//				%INSTALLEDGROUPS - List of installed groups.
//				%PROGRAMDIR - Program Files Directory.
//				%COMMONDIR - Common Files Directory.
//				%WINDIR - Windows Directory.
//				%JOYSTICK - '1' if joystick supported else '0'.
//				%STRINGn - String from resource which has ID SYMBOL_TABLE+n.
//				%% - %
//
// Parameters	sz		string where replacement will take place
//				wBuf	size of buffer
//
// Returns		none
//
// History		3/3/97 craigh		Created
//				4/1/97 a-drews		Generalized to work for other funtions
//				4/22/97 a-drews		generalized to support more tokens
//
VOID ReplaceStringTokens(char *sz, size_t wBuf)
{
	char szTmp[MAX_DATA_LENGTH];
	char *pch = sz;
	char *pchStart = sz;
	char *pchTmp = szTmp;
	unsigned cch = 0;
	BOOL fSubstitutionMade = FALSE;
	
	if (wBuf > MAX_DATA_LENGTH)
	{
		ASSERT(FALSE);
		return;
	}
	
	while (*pch != '\0')
	{
		if ('%' == *pch)
			// found a token to substitute
		{
			// copy everything up to here
			if (0 != cch)
			{
				lstrcpyn(pchTmp, pchStart, cch+1);
				pchTmp += cch;
			}
			
			// figure out which token we're gonna copy in
			if (0 == strncmp("%PLAYERNAME", pch, 11))
			{
				fSubstitutionMade = TRUE;
				AddString(&pchTmp, GetPlayerName());
				pch += 11;
			}
			else if (0 == strncmp("%APPPATH", pch, 8))
			{
				fSubstitutionMade = TRUE;
				AddString(&pchTmp, GetAppDir());
				pch += 8;
			}
			else if (0 == strncmp("%APPTITLE", pch, 9))
			{
				fSubstitutionMade = TRUE;
				AddString(&pchTmp, GetAppTitle());
				pch += 9;
			}
			else if (0 == strncmp("%SETUPTITLE", pch, 11))
			{
				fSubstitutionMade = TRUE;
				AddString(&pchTmp, GetSetupTitle());
				pch += 11;
			}
			else if (0 == strncmp("%PID", pch, 4))
			{
				fSubstitutionMade = TRUE;
				AddString(&pchTmp, GetPid());
				pch += 4;
			}
			else if (0 == strncmp("%SETUPEXEDIR", pch, 12))
			{
				fSubstitutionMade = TRUE;
				char szDir[_MAX_PATH];
				
				GetModuleDirectory(szDir,_MAX_PATH);
				szDir[lstrlen(szDir)-1] = NULL;
				AddString(&pchTmp, szDir);
				pch += 12;
			}
			else if (0 == strncmp("%INSTALLEDGROUPS", pch, 16))
			{
				fSubstitutionMade = TRUE;
				char szGroup[256];
				
				sprintf(szGroup, "%I64d", removeKeyboardTypeFlag(n_GroupList));
				AddString(&pchTmp, szGroup);
				pch += 16;
			}
			else if (0 == strncmp("%PROGRAMDIR", pch, 11))
			{
				fSubstitutionMade = TRUE;
				char szDir[_MAX_PATH];
				
				GetProgramFilesLocation(szDir, _MAX_PATH);
				AddString(&pchTmp, szDir);
				pch += 11;
			}
			else if (0 == strncmp("%COMMONDIR", pch, 10))
			{
				fSubstitutionMade = TRUE;
				char szDir[_MAX_PATH];
				
				GetCommonFilesLocation(szDir, _MAX_PATH);
				AddString(&pchTmp, szDir);
				pch += 10;
			}
			else if (0 == strncmp("%TEMPDIR", pch, 8))
			{
				fSubstitutionMade = TRUE;
				TCHAR szTemp[_MAX_PATH];
				TCHAR *ptrSlash;
				TCHAR *ptrTest;
				
				GetTempPath(_MAX_PATH, szTemp);
				
				//
				//Remove any trailing backslash...
				//
				ptrSlash = pszGetLast5C(szTemp);
				if (ptrSlash)
				{
					ptrTest = CharNext(ptrSlash);
					
					if ('\0' == *ptrTest)
					{
						*ptrSlash = '\0';
					}
				}
				
				AddString(&pchTmp, szTemp);
				pch += 8;
			}
			else if (0 == strncmp("%WINDIR", pch, 7))
			{
				fSubstitutionMade = TRUE;
				char szDir[_MAX_PATH];
				
				GetWindowsDirectory(szDir, _MAX_PATH);
				
				AddString(&pchTmp, szDir);
				pch += 7;
			}
			else if (0 == strncmp("%JOYSTICK", pch, 9))
			{
				fSubstitutionMade = TRUE;
				char buf[4]={ '0',0,'1',0 };
				
				if(GetJoystick())
					AddString(&pchTmp, &buf[2]);
				else
					AddString(&pchTmp, &buf[0]);
				pch += 9;
			}
			else if (0 == strncmp("%USERPATH", pch, 9))
			{
				fSubstitutionMade = TRUE;
				AddString(&pchTmp, GetUserPath());
				pch += 9;
			}
			else if (0 == strncmp("%STRING", pch, 7))
			{
				fSubstitutionMade = TRUE;
				int wResource;
				char szStr[_MAX_PATH];
				char szNum[256];
				char *pchNum = szNum;
				
				pch += 7;
				while (isdigit(*pch))
				{
					*pchNum++ = *pch;
					pch = AnsiNext(pch);
				}
				*pchNum = '\0';
				wResource = atoi(szNum);
				EBULoadString(GetResourceInst(),SYMBOL_TABLE+wResource,szStr,_MAX_PATH);
				AddString(&pchTmp, szStr);
			}
			else
			{
				// no suported token found so ignore the % we just found (put it back onto the new string)
				//ASSERT(FALSE);

				AddString(&pchTmp, "%");
				pch++;
			}
			cch = 0;
			pchStart = pch;
		}
		else
		{
			// MEMO : lstrcpyn() in this function is a byte order function
			//        when we are using ANSI version.
			//        So we need to process like this not a character order.
			// 
			//        This might be a easiest fix rather than move to
			//        the UNICODE version or use _mbc...() - yutaka
			// 
			char *pchTmp = pch;
			pch = CharNext(pch);
			cch += (pch - pchTmp);
		}
	}
	if (fSubstitutionMade)
	{
		if (0 != cch)
			lstrcpyn(pchTmp, pchStart, cch+1);
		
		ASSERT((size_t) lstrlen(szTmp) < wBuf);
		
		lstrcpy(sz, szTmp);
	}
	
}

//
//Tests to see if the current user is an admin on Windows NT.  See Microsoft
//Knowledge Base article PSS ID#Q118626.
//
BOOL IsAdmin(void)
{
    SC_HANDLE hSC;

	if (GetOS() & OS_WINMASK)
	{
		//
		//Always return TRUE under Windows 95 or Windows 98
		//
		return TRUE;
	}

    //
    //Try an admin priviledged API - if it works return TRUE - else FALSE
    //
    hSC = OpenSCManager(NULL,
                        NULL,
                        GENERIC_READ | GENERIC_WRITE | GENERIC_EXECUTE);

    if (NULL == hSC)
    {
        return FALSE;
    }

    CloseServiceHandle(hSC);

    return TRUE;
}
BOOL AddSharedDLL(LPSTR szFileName)
{
	LPSHAREDDLL sd = GetSharedDLL();
	
	if(!GetSharedDLL())
	{
		SetSharedDLL((LPSHAREDDLL) malloc(sizeof(SHAREDDLL)));
		if(NULL == GetSharedDLL())
		{
			Alert(GetWndParent(),MB_OK | MB_ICONEXCLAMATION,STR_ERROR_NOMEMORY);
			return FALSE;
		}

		ZeroMemory((void *) GetSharedDLL(), sizeof(SHAREDDLL));
		lstrcpy(GetSharedDLL()->szFileName,szFileName);

		return TRUE;
	}
	
	while(sd->nextDLL != NULL)
	{
		if(!lstrcmpi(szFileName,sd->szFileName))
			return FALSE;
		sd = sd->nextDLL;
	}
	sd->nextDLL = (LPSHAREDDLL) malloc(sizeof(SHAREDDLL));
	if(sd->nextDLL == NULL)
	{
		Alert(GetWndParent(),MB_OK | MB_ICONEXCLAMATION,STR_ERROR_NOMEMORY);
		return FALSE;
	}
	sd = sd->nextDLL;
	ZeroMemory((void *) sd, sizeof(SHAREDDLL));
	lstrcpy(sd->szFileName,szFileName);
	return TRUE;
}
BOOL RemoveSharedDLL(LPSTR szFileName)
{
	LPSHAREDDLL sd = GetSharedDLL();
	LPSHAREDDLL prevDLL;
	
	if(!GetSharedDLL())
	{
		return TRUE;
	}
	prevDLL = NULL;
	while(sd != NULL)
	{
		if(!lstrcmpi(szFileName,sd->szFileName))
		{
			if(prevDLL == NULL)
				SetSharedDLL(sd->nextDLL);
			else
				prevDLL->nextDLL = sd->nextDLL;
			free(sd);
			return TRUE;
		}
		else
		{
			prevDLL = sd;
			sd = sd->nextDLL;
		}
	}
	return FALSE;
}

void ClearSharedDLL()
{
	LPSHAREDDLL sd,nextDLL;

	if (!GetSharedDLL())
		return;

	sd = GetSharedDLL();

	while(sd)
	{
		nextDLL = sd->nextDLL;
		free(sd);
		sd = nextDLL;
	}

	SetSharedDLL(NULL);

	return;
}

BYTE GetServicePack()
{
	HKEY hkKey;
	DWORD dwCSDVersion = 0;
	DWORD dwSize = sizeof(DWORD);

	if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE,
									  "SYSTEM\\CurrentControlSet\\Control\\Windows",
									  0,
									  KEY_QUERY_VALUE,
									  &hkKey))
	{
		RegQueryValueEx(hkKey,
						"CSDVersion",
						0,
						NULL,
						(LPBYTE) &dwCSDVersion,
						&dwSize);

		RegCloseKey(hkKey);
	}

	return HIBYTE(LOWORD(dwCSDVersion));
}

// IsMinVerInstalled -- Checks to see if the minimum version of a file is installed
// Returns           -- TRUE if the file with min version is installed; FALSE otherwise.
BOOL IsMinVersionInstalled(LPSTR lpFilePath, WORD wMajor, WORD wMinor, WORD wBuildHi, WORD wBuildLo)
{
	// get the version info for the direct draw dll (ddraw.dll).
	FILEINFO		         FileInfo;
	UINT		            uFileInfoResult;
   BOOL                 bReturnCode = TRUE;

	uFileInfoResult = EBUFileInfo(lpFilePath, &FileInfo);

	if (uFileInfoResult)
	{
      TRACE(STR_HARDCODE_VERINFOFAILED);
      bReturnCode = FALSE;
	}
   else
   {
		// got version info ok
		// break it up into components
		WORD  wVersion[4] = {0,0,0,0};

		wVersion[0] = HIWORD(FileInfo.dwFileVersionMS);
		wVersion[1] = LOWORD(FileInfo.dwFileVersionMS);
		wVersion[2] = HIWORD(FileInfo.dwFileVersionLS);
		wVersion[3] = LOWORD(FileInfo.dwFileVersionLS);

		// figure out what our min version is
		WORD			wMinVersion[4] = {wMajor, wMinor, wBuildHi, wBuildLo};

		// compare the existing version with our min version
		for (int i = 0; i < 4; i++)
		{
			if (wMinVersion[i] == wVersion[i])
			{
				// same try next part
				continue;
			}
			if (wMinVersion[i] > wVersion[i])
			{
            bReturnCode = FALSE;
			}
			break; // done
		}
	}

	return bReturnCode;
}

BOOL MyGetDiskFreeSpace(LPCTSTR lpRootPathName, LPDWORD lpBytesPerCluster, LPDWORD lpNumberOfFreeClusters)
{
	BOOL fRetVal = FALSE;
	DWORD	dwBytesPerSector;
	DWORD	dwTotalClusters;

		// If we are running Win 95 OSR2 or better, the IOCTL returns the cluster size.
		// If that fails, we assume that we are running Win NT for which 
		// GetDiskFreeSpace returns the right cluster size.

		// Code suplied by Rich
#define VWIN32_DIOC_DOS_DRIVEINFO 6
typedef struct _DIOC_REGISTERS
{
 DWORD reg_EBX;
 DWORD reg_EDX;
 DWORD reg_ECX;
 DWORD reg_EAX;
 DWORD reg_EDI;
 DWORD reg_ESI;
 DWORD reg_Flags;
} DIOC_REGISTERS, *PDIOC_REGISTERS;

#pragma pack(1)
typedef struct _ExtGetDskFreSpcStruc
{
 WORD ExtFree_Size;
 WORD ExtFree_Level;
 DWORD ExtFree_SectorsPerCluster;
 DWORD ExtFree_BytesPerSector;
 DWORD ExtFree_AvailableClusters;
 DWORD ExtFree_TotalClusters;
 DWORD ExtFree_AvailablePhySectors;
 DWORD ExtFree_TotalPhysSectors;
 DWORD ExtFree_AvailableAllocationUnits;
 DWORD ExtFree_TotalAllocationUnits;
 DWORD ExtFree_Rsvd[2];
} ExtGetDskFreSpcStruc, *pExtGetDskFreSpcStruc;
#pragma pack()
	HANDLE hDevice;
	DIOC_REGISTERS reg;
	ExtGetDskFreSpcStruc spc;
	BOOL bResult;
	DWORD cb;

	hDevice = CreateFile("\\\\.\\vwin32",0,0,NULL,0,FILE_FLAG_DELETE_ON_CLOSE,NULL);

	if ( hDevice != INVALID_HANDLE_VALUE )
	{
		reg.reg_EDI = (DWORD)&spc;
		reg.reg_ECX = sizeof(spc);
		reg.reg_EDX = (DWORD)(LPCTSTR) lpRootPathName;
		reg.reg_EAX = 0x7303;
		reg.reg_Flags = 0x0001;
		spc.ExtFree_Level = 0;
		bResult = DeviceIoControl(hDevice, VWIN32_DIOC_DOS_DRIVEINFO,
						&reg, sizeof(reg), &reg, sizeof(reg), &cb, 0);
		CloseHandle(hDevice);

		if(!bResult || (reg.reg_Flags & 0x0001))
		{
			TRACE("Error getting cluster size\n");
			goto TryPlanB;
		}
		else
		{
			TRACE("On drive %s, cluster size is %u\n", lpRootPathName, spc.ExtFree_SectorsPerCluster * spc.ExtFree_BytesPerSector);
			*lpBytesPerCluster = spc.ExtFree_SectorsPerCluster * spc.ExtFree_BytesPerSector;
			*lpNumberOfFreeClusters	= spc.ExtFree_AvailableClusters;
			return true;
		}
	}
TryPlanB:
	// Under straight Win 95 or Win NT this is good enough.
	fRetVal = GetDiskFreeSpace(lpRootPathName, lpBytesPerCluster,
				&dwBytesPerSector, lpNumberOfFreeClusters, &dwTotalClusters);
	*lpBytesPerCluster *= dwBytesPerSector;

	ULARGE_INTEGER freeBytes, totalBytes, numFree;
	fRetVal = GetDiskFreeSpaceEx(lpRootPathName, &freeBytes, &totalBytes, NULL);

	numFree.QuadPart = freeBytes.QuadPart / (*lpBytesPerCluster);
	*lpNumberOfFreeClusters = (DWORD) numFree.QuadPart;

	return fRetVal;
}


BOOL EnsureCDROMInserted(LPINSTALLLIST  ilInstallList)
{
	int	nMsgBoxResult = 0;
	UINT uiLastErrorMode = 0;
	static TCHAR szInsertCD[256] = "";
	static TCHAR szInsertCDMessage[256] = "";
	static TCHAR szInstallFromPath[_MAX_PATH]="";
	static int	nLastDiskID = DISK_01 + 1;
	static BOOL bCheckStartup=TRUE;
	static BOOL bCheckAlternates=FALSE;
	TCHAR szDiskSuffix='\0';
	LPTSTR lpTmp=NULL;
	LPTSTR lpSource=NULL;
	LPTSTR lpDest=NULL;

	// Don't do this check on uninstalls
	if (GetRemovingApp())
	{
		return TRUE;
	}
	// Only do this if the resource dll is loaded otherwise we get a bogus string
	if (!GetResourceInst() )
	{
		return TRUE;
	}
	//First Time Init Re-Insert the CD String
	if ('\0' == szInsertCD[0])
	{
		//Return if setup dll hasn't been loaded yet...
		if (0 == EBULoadString(GetResourceInst(), STR_ERROR_CANTFINDCD, szInsertCD, sizeof(szInsertCD)))
		{
			return TRUE;
		}
		else
		{
				ReplaceStringTokens(szInsertCD, sizeof(szInsertCD));
		}

	}
	// Do alternate paths need to be checked.
	if (bCheckStartup)
	{
		bCheckStartup = FALSE;
		if (EBUlstrstri( GetSourcePath(), &DISKONEPATH ) )
		{
			bCheckAlternates=TRUE;
		}
	}

	//Process Messages here.
	ForwardMessages();
	if ( !ilInstallList || ilInstallList->nDiskID == DISK_NOT_SPECIFIED) 
	{
		// If a InstallList object was passsed patch the source file's path if necessary.
		if (ilInstallList)
		{
			lpSource = szInstallFromPath;
			lpDest = ilInstallList->szSource;
			while (*lpSource)
			{
				*lpDest = *lpSource;
				lpSource = CharNext(lpSource);
				lpDest = CharNext(lpDest);
			}
		}
		if (DoesFileExistNoCriticalErrors(GetDiscIDPath() ) )
		{
			return TRUE;
		}
	}
	else // a disk was signaled.
	{
		// save the new test file signature
		SetDiscIDPath (ilInstallList->szSource);
		nLastDiskID = ilInstallList->nDiskID + 1;
		// See if the file exists in the original source path
		if (DoesFileExistNoCriticalErrors (ilInstallList->szSource) )
		{
			// Truncate the patch path.
			*szInstallFromPath='\0';
			return TRUE;
		}
		else
		{
			if (bCheckAlternates)
			{
				//Find the start of the signature in the real string
				lpTmp = EBUlstrstri(ilInstallList->szSource,   &DISKONEPATH);
				//Point to the number in disk1 match
				lpTmp = lpTmp + EBUStrlen( &DISKONEPATH ) - 1;
				//Prepare the disk suffix from the nDiskID in the command structure;
				itoa(ilInstallList->nDiskID + 1, &szDiskSuffix, 10);
				//Set the number char in the path value to the new value.
				*lpTmp =  szDiskSuffix;
				//Test Alternate location for network installs
				if (DoesFileExistNoCriticalErrors(ilInstallList->szSource))
				{
					// Set the new Signature path for EnsureCDRom testing.
					SetDiscIDPath(ilInstallList->szSource);
					// Save the install location for patching all files after this.
					lpTmp = CharNext(lpTmp);
					szDiskSuffix = *lpTmp;
					*lpTmp = '\0';
					lstrcpy(szInstallFromPath, ilInstallList->szSource);
					*lpTmp = szDiskSuffix;
					return TRUE;
				}
				else
				{
					// Restore original location.
					*lpTmp = DISKONESUFFIX;
					// Truncate the patch path.
					*szInstallFromPath='\0';
				}
			}
		}
	}

	// Nag until the correct disk is inserted or until the user CANCEL's Setup	
	do
	{
		// Yield this thread so the OS or other PROCESSES will get a chance to release any resources
		// located on the current disk. Keep moving messages too.
		Sleep(0); 
		ForwardMessages();
		// Supress all errors
		uiLastErrorMode = SetErrorMode(SEM_NOOPENFILEERRORBOX | SEM_FAILCRITICALERRORS);
		// Prompt the user to insert the disk we are looking for.
		wsprintf(szInsertCDMessage, szInsertCD, nLastDiskID);
		nMsgBoxResult = MessageBox(GetWndParent(),
									szInsertCDMessage,
									GetSetupTitle(), 
 									MB_ICONEXCLAMATION | MB_OKCANCEL | MB_DEFBUTTON1);
		// restore error messages
		SetErrorMode(uiLastErrorMode);
		if (0== nMsgBoxResult || IDCANCEL == nMsgBoxResult)
		{
			//
			// If User Cancel's OR, If MessageBox API fails for any reason, exit...
			//
			SetResultCode(EBU_ABORT);
			return FALSE;
		}
		// Test for the wanted disk insertion.
		if (DoesFileExistNoCriticalErrors(GetDiscIDPath() ) )
		{
			// yield this threads current time slice
			// wait for autorun crap to clearout and go away.
			// since there is no reliabe way to toggle it at runtime.
			int i = 0;
			while (i < 300)
			{
				ForwardMessages();
				Sleep(0);
				Sleep(100);
				i++;
			}
			return TRUE;
		}
	} while (TRUE);
	SetResultCode(EBU_ABORT);
	return FALSE;
}
			

int EBULoadString(HINSTANCE hResInst, UINT uID, LPTSTR lpBuffer, int nBufferMax)
{
	int nCount;

	//
	//IDs #496 - #499 should not be used.  Prepstub98 requires string tables to be written
	//at 16 bytes boundaries.  Since SYMBOL_TABLE == 500, we have to keep these four unused.
	//
	assert(uID < SYMBOL_TABLE - 4 || uID > SYMBOL_TABLE - 1);

	//
	//Load the string from the string table
	//
	if ((nCount = LoadString(hResInst, uID, lpBuffer, nBufferMax)))
	{
		//
		//Perform macro substitution, if any...
		//
		ReplaceStringTokens(lpBuffer, nBufferMax);
	}
	else
	{
		if (nBufferMax > 15) //15 is estimate of length of text "RESERR [...."
		{
			wsprintf(lpBuffer, "RESERR [id:%lu]\n", uID);
		}
#ifdef _DEBUG
        TRACE(lpBuffer);

		nCount = lstrlen(lpBuffer);
#endif
	}

	return nCount;
}

HRSRC EBUFindResource(HMODULE hModule, LPCTSTR lpName, LPCTSTR lpType)
{
	return FindResource(hModule, lpName, lpType);
}

HGLOBAL EBULoadResource(HMODULE hModule, HRSRC hResInfo)
{
	return LoadResource(hModule, hResInfo);
}

HANDLE EBULoadImage(HINSTANCE hInst, LPCTSTR lpszName, UINT uType, int cxDesired, int cyDesired, UINT fuLoad)
{
	return LoadImage(hInst, lpszName, uType, cxDesired, cyDesired, fuLoad);
}

HCURSOR EBULoadCursor(HINSTANCE hInst, LPCTSTR lpCursorName)
{
	return LoadCursor(hInst, lpCursorName);
}

HICON EBULoadIcon(HINSTANCE hInst, LPCTSTR lpIconName)
{
	return LoadIcon(hInst, lpIconName);
}

HBITMAP EBULoadBitmap(HINSTANCE hInst, LPCTSTR lpBitmapName)
{
	return LoadBitmap(hInst, lpBitmapName);
}


BOOL EBUPlaySound(LPCSTR pszSound, HMODULE hmod, DWORD dwPlayFlags)
{
	if (GetNoSound())
		return TRUE;

	static TCHAR szBuf[MAX_PATH + 1];

	if ((SND_RESOURCE | SND_FILENAME) & dwPlayFlags)
	{
		if (SND_FILENAME & dwPlayFlags)
		{
			ASSERT(NULL == hmod);

			lstrcpy(szBuf, pszSound);

			ReplaceStringTokens(szBuf, sizeof(szBuf));
		}

		if (SND_RESOURCE & dwPlayFlags)
		{
			wsprintf(szBuf, "#%d", pszSound);
		}
	}


	if (GetMCISound())
	{
		return playWAVEFile(NULL, ((SND_RESOURCE & dwPlayFlags) ? szBuf : pszSound), dwPlayFlags);
	}else{
		return PlaySound(SND_FILENAME & dwPlayFlags ? szBuf : pszSound, hmod, dwPlayFlags);
	}
}


static BYTE * lpData = NULL;
static DWORD dwWavSize = 0;
static BOOL bPlaying = FALSE;
LRESULT CALLBACK IOProc(LPMMIOINFO lpMMIOInfo, UINT uMessage, LPARAM lParam1, LPARAM lParam2)
{
	static BOOL alreadyOpened = FALSE;
	
	switch (uMessage)
	{
	case MMIOM_OPEN:
		{
			if (alreadyOpened)
				return 0;
			alreadyOpened = TRUE;
			
			lpMMIOInfo->lDiskOffset = 0;
			return 0;
		}break;
		
	case MMIOM_CLOSE:
		{
			lpMMIOInfo->lDiskOffset = 0;
			alreadyOpened = FALSE;
			return 0;
		}break;
		
	case MMIOM_READ:
		{
			if (!lpData)
				return 0;
			
			memcpy((void *)lParam1, lpData+lpMMIOInfo->lDiskOffset,
				lParam2);
			lpMMIOInfo->lDiskOffset += lParam2;

			if ((lpMMIOInfo->lDiskOffset > 44) && (lpMMIOInfo->lDiskOffset < (long)dwWavSize-1))
				bPlaying = TRUE;
			else
				bPlaying = FALSE;
			
			return (lParam2);
		}break;
		
	case MMIOM_SEEK:
		{
			assert(lpData);
			
			switch (lParam2) {
			case SEEK_SET:
				lpMMIOInfo->lDiskOffset = lParam1;
				break;
				
			case SEEK_CUR:
				lpMMIOInfo->lDiskOffset += lParam1;
				
			case SEEK_END:
				lpMMIOInfo->lDiskOffset = dwWavSize - 1 - lParam1;
				break;
			}
			return lpMMIOInfo->lDiskOffset;
		}break;
		
	default:
		return -1; // Unexpected msgs.  For instance, we do not process MMIOM_WRITE in this sample
	}// end of switch
}//end of IOProc



// Plays a specified waveform-audio file using MCI_OPEN and MCI_PLAY. 
// Returns when playback begins. Returns 0L on success, otherwise 
// returns an MCI error code.
DWORD playWAVEFile(HWND hWndNotify, LPCTSTR lpszWAVEFileName, DWORD dwPlayFlags)
{
	static bool
		bSomethingOpen = FALSE;

	static char szWavePlaying[256];

	if (lstrcmpi(szWavePlaying, lpszWAVEFileName))
	{
		if (bSomethingOpen)
		{
			mciSendString("close test", NULL, 0, NULL);
			bSomethingOpen = FALSE;
		}

		// new file
		lstrcpy(szWavePlaying, lpszWAVEFileName);

		// if there was a previous file release its memory
		lpData = NULL;

		// locate the new resource
		HRSRC
			hRsrc = EBUFindResource(GetResourceInst(), (LPCSTR)lpszWAVEFileName, "Wave");

		HGLOBAL	
			tResource = EBULoadResource(GetResourceInst(), hRsrc);

		// lock the resource
		lpData = (BYTE*) LockResource(tResource);

		// get resource size
		dwWavSize = SizeofResource(GetResourceInst(), hRsrc);
		
		mmioInstallIOProc(mmioFOURCC('M', 'E', 'Y', ' '), (LPMMIOPROC)IOProc, MMIO_INSTALLPROC | MMIO_GLOBALPROC);

		mciSendString("open test.MEY+ type waveaudio alias test", NULL, 0, NULL);

		bSomethingOpen = TRUE;
	}

	mciSendString("play test from 0", NULL, 0, NULL);
	
	return (0L);
}




DWORD GetBuildCharacteristics(void)
{
	TCHAR szTmp[32] = "";
	DWORD  dwResult = GetBuild();

	//
	//If not specified on command line...
	//
	if (FALSE == IsDBCS() && FALSE == IsANSI())
	{
		//
		//Is STR_ISDBCS string is defined == "1", then set DBCS execution flag...
		//
		EBULoadString(GetResourceInst(), STR_ISDBCS, szTmp, sizeof(szTmp));
		dwResult |= ('1' == szTmp[0] ? BLD_DBCS : BLD_ANSI);

		szTmp[0] = '\0';
	}

	//
	//If not specified on command line...
	//
	if (FALSE == IsOEM() && FALSE == IsRetail())
	{
		//
		//Is STR_ISOEM string defined == "1"? If so, set OEM flag, else set retail flag
		//
		EBULoadString(GetResourceInst(), STR_ISOEM, szTmp, sizeof(szTmp));
		dwResult |= ('1' == szTmp[0] ? BLD_OEM : BLD_RTL);

		szTmp[0] = '\0';
	}

	lstrcpy(szTmp, GetUserLanguage());

	//
	//If no language specified on command line....
	//
	if (!*szTmp)
	{
		EBULoadString(GetResourceInst(), STR_LANGUAGE, szTmp, sizeof(szTmp));
	}

	//
	//Does STR_LANGUAGE == Japan, French, Spanish, German, or USA
	//
	if (0 == lstrcmpi(szTmp, "JPN"))
	{
		dwResult |= BLD_JPN;
	}
	else if (0 == lstrcmpi(szTmp, "FRA"))
	{
		dwResult |= BLD_FRA;
	}
	else if (0 == lstrcmpi(szTmp, "SPA"))
	{
		dwResult |= BLD_SPA;
	}
	else if (0 == lstrcmpi(szTmp, "GER"))
	{
		dwResult |= BLD_GER;
	}
	else if (0 == lstrcmpi(szTmp, "USA"))
	{
		dwResult |= BLD_USA;
	}

	assert(dwResult & BLD_LANGMASK);

	return dwResult;
}

BOOL ExecOnThisPlatform(DWORD dwCommandFlags)
{
	if (!(GetOS() & dwCommandFlags))
	{
		return FALSE;
	}

	//
	//If any language flags specified, then if none of them match the current language, bail out...
	//
	if (BLD_LANGMASK & dwCommandFlags)
	{
		if (!((BLD_LANGMASK & dwCommandFlags) & GetBuild()))
		{
			return FALSE;
		}
	}

	if (BLD_OEM & dwCommandFlags)
	{
		if (!IsOEM()) return FALSE;
	}

	if (BLD_RTL & dwCommandFlags)
	{
		if (!IsRetail()) return FALSE;
	}

	if (BLD_DBCS & dwCommandFlags)
	{
		if (!IsDBCS()) return FALSE;
	}

	if (BLD_ANSI & dwCommandFlags)
	{
		if (!IsANSI()) return FALSE;
	}

	if (BLD_APP1 & dwCommandFlags)
	{
		if (!IsApp1()) return FALSE;
	}

	if (BLD_APP2 & dwCommandFlags)
	{
		if (!IsApp2()) return FALSE;
	}

	if (BLD_APP3 & dwCommandFlags)
	{
		if (!IsApp3()) return FALSE;
	}

	return TRUE;
}


BOOL SetApp1Flag(BOOL fSet)
{
	BOOL fOldVal = GetBuild() & BLD_APP1;

	SetBuild((GetBuild() & ~BLD_APP1) | (fSet ? BLD_APP1 : 0));

	return fOldVal;
}

BOOL SetApp2Flag(BOOL fSet)
{
	BOOL fOldVal = GetBuild() & BLD_APP2;

	SetBuild((GetBuild() & ~BLD_APP2) | (fSet ? BLD_APP2 : 0));

	return fOldVal;
}

BOOL SetApp3Flag(BOOL fSet)
{
	BOOL fOldVal = GetBuild() & BLD_APP3;

	SetBuild((GetBuild() & ~BLD_APP3) | (fSet ? BLD_APP3 : 0));

	return fOldVal;
}

BOOL IsRetail(void)
{
	return BLD_RTL & GetBuild();
}

BOOL IsANSI(void)
{
	return BLD_ANSI & GetBuild();
}

BOOL IsApp1(void)
{
	return BLD_APP1 & GetBuild();
}

BOOL IsApp2(void)
{
	return BLD_APP2 & GetBuild();
}

BOOL IsApp3(void)
{
	return BLD_APP3 & GetBuild();
}

BOOL IsDBCS(void)
{
    return BLD_DBCS & GetBuild();
}

BOOL IsUSA(void)
{
    return BLD_USA & GetBuild();
}

BOOL IsJapan(void)
{
    return BLD_JPN & GetBuild();
}

BOOL IsFrench()
{
    return BLD_FRA & GetBuild();
}

BOOL IsGerman()
{
    return BLD_GER & GetBuild();
}

BOOL IsSpanish()
{
    return BLD_SPA & GetBuild();
}


BOOL IsOEM(void)
{
    return BLD_OEM & GetBuild();
}

BOOL IsIMEEnabled(void)
{
	return SCF_IME_ENABLE & GetCommandFlags();
}

BOOL IsIMEOn(void)
{
	return SCF_IME_ON & GetCommandFlags();
}

void DisplaySystemError(DWORD dwError, UINT uiType)
{
	LPVOID lpMsgBuf;

	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
				  NULL,
				  GetLastError(),
				  MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
				  (LPTSTR) &lpMsgBuf,
				  0,
				  NULL);

	//
	//Display the system error string...
	//
	MessageBox(NULL, (TCHAR *) lpMsgBuf, "", uiType);

	LocalFree(lpMsgBuf);
}

static keyboardType getKeyboardType()
{				 
	int iType;
	int iRet;
	int iMaker, iSubType;
	
	iType = GetKeyboardType(0);
	iRet = GetKeyboardType(1);
	iMaker = (iRet >> 8) & 0xFF;
	iSubType = iRet  & 0xFF;
	
	keyboardType kbRet = kb101;
	
	switch ( iType ){
	default:
		kbRet = kb101;
		break;
	case 0: // error 
		//kbRet = kbError;
		kbRet = kb101;
		break;
	case 7: // Japanese Keyboard
		switch ( iMaker ){
		default :
			kbRet = kb106;
			break;
		case 0x00: // DosV
			switch ( iSubType ){
			default :
			case 2:
				kbRet = kb106;
				break;
			case 0:
				kbRet = kb101;
				break;
			}
			break;
		case 0x0D: // NEC
			switch ( iSubType ){
			default:
			case 1:
				kbRet = kb98;
				break;
			case 5:
				kbRet = kb106;
				break;
			}
			break;
		}
		break;
	}
	
	return kbRet;
}

__int64 addKeyboardTypeFlag()
{
	__int64
		i64TempMask = 0x0;
	
  if ( IsJapan() ){
	switch ( getKeyboardType() ){
	default:
	case kb101:
		i64TempMask |= 0x1000000000000000;
		break;
	case kb106:
		i64TempMask |= 0x2000000000000000;
		break;
	case kb98:
		i64TempMask |= 0x4000000000000000;
		break;
	}
  } else {
		i64TempMask |= 0x1000000000000000; // force to 101 keyboard
  }
	
 	return i64TempMask;
}

__int64 addKeyboardTypeFlag( __int64 i64TempMask )
{
	return (i64TempMask | addKeyboardTypeFlag());
}

__int64 removeKeyboardTypeFlag( __int64 i64TempMask )
{
	return (i64TempMask & 0x0FFFFFFFFFFFFFFF);
}

EBURETCODE EBUShellExecute(HWND  hWndParent,
						   TCHAR *pszExecuteThis,
						   TCHAR *pszParameters,
						   TCHAR *pszDirectory,
						   int   nShow,
						   UINT  uiTag,
						   UINT  uiErrorResID,
						   BOOL  fWait,
						   LPSHELLEXECUTEDATA psed)
{
	BOOL			 fSuccess = TRUE;
	BOOL			 fCallback;
	TCHAR            szExe[MAX_PATH];
	TCHAR			 szParms[MAX_PATH];
	TCHAR			 szDirectory[MAX_PATH];
	TCHAR			 szShortPath[MAX_PATH];
	EBURETCODE       nRc = EBU_OK;
	SHELLEXECUTEDATA sed;
    SHELLEXECUTEINFO sei;

	ASSERT(pszExecuteThis);

	//
	//Only callback is app callback pointer has been initialized...
	//
	fCallback = GetAppCallback() ? TRUE : FALSE;

	//
	//Expand tokens and then get short path name for call...
	//
	lstrcpy(szExe, pszExecuteThis);
	ReplaceStringTokens(szExe, sizeof(szExe));
	if (GetShortPathName(szExe, szShortPath, sizeof(szShortPath)))
	{
		lstrcpy(szExe, szShortPath);
	}

	if (NULL != pszParameters)
	{
		lstrcpy(szParms, pszParameters);
		ReplaceStringTokens(szParms, sizeof(szParms));
		if (GetShortPathName(szParms, szShortPath, sizeof(szShortPath)))
		{
			lstrcpy(szParms, szShortPath);
		}
		pszParameters = szParms;
	}

	if (NULL != pszDirectory)
	{
		lstrcpy(szDirectory, pszDirectory);
		ReplaceStringTokens(szDirectory, sizeof(szDirectory));
		if (GetShortPathName(szDirectory, szShortPath, sizeof(szShortPath)))
		{
			lstrcpy(szDirectory, szShortPath);
		}
		pszDirectory = szDirectory;
	}

	sed.fUninstall = GetRemovingApp();
	sed.pszPathName = szExe;
	sed.fWait = fWait;
	sed.uiTag = uiTag;
	sed.nStatus = SES_BEFORE;

	if (TRUE == fCallback)
	{
		//
		//Callback before execution...
		//
		nRc = (*(GetAppCallback())) ((void *) &sed);
		ASSERT(EBU_OK == nRc || EBU_CANCEL == nRc || EBU_ABORT == nRc || EBU_BACK == nRc);
	}

	if (EBU_OK == nRc)
	{
        ZeroMemory(&sei,sizeof(SHELLEXECUTEINFO));

		sei.hwnd = hWndParent ? hWndParent : GetDesktopWindow();

        sei.cbSize = sizeof(SHELLEXECUTEINFO);
        sei.fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_FLAG_NO_UI;
        sei.lpFile = szExe;
        sei.lpParameters = pszParameters;
		sei.lpDirectory = pszDirectory;
        sei.nShow = nShow;
		
		if (!EnsureCDROMInserted(NULL))
		{
			nRc = EBU_ABORT;
			goto cdabort;
		}

		fSuccess = ShellExecuteEx(&sei);

		if (FALSE == fSuccess && 0 != uiErrorResID)
        {
			Alert(hWndParent, MB_OK | MB_ICONSTOP, uiErrorResID);
        }

		sed.hInstApp = sei.hInstApp;
		sed.hProcess = sei.hProcess;
		sed.nStatus = SES_LAUNCHED;

		if (TRUE == fCallback)
		{
			//
			//Callback after launch...
			//
			(*(GetAppCallback())) ((void *) &sed);
		}

		if (TRUE == fWait && TRUE == fSuccess)
		{
			//
			//Wait for signal that spawned app has initialized...
			//
			Sleep(0);
			WaitForInputIdle(sei.hProcess, INFINITE);
			Sleep(0);

			while (STILL_ACTIVE == sed.dwRc)
			{
				if (0 == GetExitCodeProcess(sei.hProcess, &sed.dwRc))
				{
					break;
				}

				ForwardMessages();
			}

			if (ERROR_SUCCESS_REBOOT_REQUIRED == sed.dwRc ||
				ERROR_SUCCESS_RESTART_REQUIRED == sed.dwRc)
			{
				SetReboot(TRUE);
			}
		}

		if (TRUE == fSuccess)
		{
			CloseHandle(sei.hProcess);
		}
	}

	sed.nStatus = SES_FINISHED;

	if (NULL != psed)
	{
		CopyMemory(psed, &sed, sizeof(SHELLEXECUTEDATA));
	}

	if (TRUE == fCallback)
	{
		//
		//Callback after getting exit code...
		//
		(*(GetAppCallback())) ((void *) &sed);
	}

cdabort:
	return fSuccess ? nRc : EBU_ERROR;
}

LPSTR EBUstrcpyn(TCHAR *pszDest, TCHAR *pszSrc, int n)
{
	LPSTR pszSaveDest = pszDest;

	//
	//Copy (at most) n-1 characters from src to dest and add a NUL terminator...
	//
	if (1 > n) return NULL;

	while (--n && *pszSrc)
	{
		*pszDest = *pszSrc;

		pszDest = CharNext(pszDest);
		pszSrc  = CharNext(pszSrc);
	} 

	*pszDest = '\0';

	return pszSaveDest;
}
BOOL IsBrowserInstalled()
{
    HRESULT hres;
    IShellLink * pShellLink;
    BOOL retc = TRUE;
    IUniformResourceLocator *pUrl;

    //CoInitialize must be called before this
    // Get a pointer to the IShellLink interface.
    CoInitialize(NULL);

    hres = CoCreateInstance(   CLSID_InternetShortcut,
                           NULL,
                           CLSCTX_INPROC_SERVER,
                           IID_IUniformResourceLocator,
                           (LPVOID*)&pUrl);
    if (SUCCEEDED(hres))
    {
       hres = pUrl->QueryInterface(IID_IShellLink,(LPVOID *)&pShellLink);
       if(!SUCCEEDED(hres))
		 retc = FALSE;
	   else
          pShellLink->Release();
       pUrl->Release();
    }
    else
	   retc = FALSE;
    CoUninitialize();
    return retc;
}

BOOL EBUIsDirty()
{
	return GetDirtyBits() ? TRUE : FALSE;
}

LPTSTR EBUstrstr(LPTSTR szSearchString, LPTSTR szSearchFor)
{
	LPCTSTR szPtr = szSearchFor;
	LPTSTR  szStartPos = szSearchString;

	while (*szSearchString)
	{
		if (*szPtr)
		{
			if (*szSearchString == *szPtr)
			{
				szPtr = CharNext(szPtr);
			}
			else
			{
				szPtr = szSearchFor;
				szStartPos = CharNext(szSearchString);
			}

			szSearchString = CharNext(szSearchString);
		}
		else
		{
			break;
		}
	}

	return *szPtr ? NULL : szStartPos;
}

HANDLE EBUCreateFile(LPCTSTR lpFileName, 
					 DWORD dwDesiredAccess, 
					 DWORD dwShareMode, 
					 LPSECURITY_ATTRIBUTES lpSecurityAttributes, 
					 DWORD dwCreationDistribution, 
					 DWORD dwFlagsAndAttributes, 
					 HANDLE hTemplateFile)
{
	if (EnsureCDROMInserted(NULL))
	{
		return CreateFile(lpFileName,
						  dwDesiredAccess,
						  dwShareMode,
						  lpSecurityAttributes,
						  dwCreationDistribution,
						  dwFlagsAndAttributes,
						  hTemplateFile);
	}
	else
	{
		SetLastError(-1);
		return INVALID_HANDLE_VALUE;
	}
}

BOOL EBUReadFile(HANDLE hFile, 
				 LPVOID lpBuffer, 
				 DWORD nNumberOfBytesToRead, 
				 LPDWORD lpNumberOfBytesRead, 
				 LPOVERLAPPED lpOverlapped)
{
	if (EnsureCDROMInserted(NULL))
	{
		return ReadFile(hFile,
						lpBuffer,
						nNumberOfBytesToRead,
						lpNumberOfBytesRead,
						lpOverlapped);
	}
	else
	{
		return FALSE;
	}
}
 
BOOL EBUCopyFile(LPCTSTR lpExistingFileName, 
				 LPCTSTR lpNewFileName, 
				 BOOL bFailIfExists)
{
	if (EnsureCDROMInserted(NULL))
	{
		return CopyFile(lpExistingFileName, lpNewFileName, bFailIfExists);
	}
	else
	{
		return FALSE;
	}
}

LONG EBULZCopy(INT hfSource, 
			   INT hfDest)
{
	if (EnsureCDROMInserted(NULL))
	{
		return LZCopy(hfSource, hfDest);
	}
	else
	{
		//Return this error for bad cd check;
		SetLastError(-1);
		return LZERROR_READ;
	}
}

INT EBULZOpenFile(LPTSTR lpFileName, 
				  LPOFSTRUCT lpReOpenBuf, 
				  WORD wStyle)
{
	if (EnsureCDROMInserted(NULL))
	{
		return LZOpenFile(lpFileName, lpReOpenBuf, wStyle);
	}
	else
	{
		//Return this error for bad cd check;
		SetLastError(-1);
		return LZERROR_BADINHANDLE;
	}
}


BOOL FindSpaceCreateTempFileAndCopyToIt(char *pszSrcFileName, char *pszBootStrappedName)
{
	// Enumdrive stuff
	DWORD dwDriveMask = 0;
	UINT uiDriveType = 0;
	LPTSTR        lpszRootPathName=TEXT("?:\\");

	// Size stuff
	DWORD dwBytesPerCluster;
	DWORD dwNumberOfFreeClusters;
	TCHAR szTmpPath[_MAX_PATH];

	// Source File info
	struct _tstat ss;

	// Flow Control
	BOOL  fFirstPass = TRUE;
	int nPass;


	//Get File Size Information
	if (0 != _tstat(pszSrcFileName, &ss))
	{
		DisplaySystemError(GetLastError(), MB_OK | MB_ICONSTOP);
		return FALSE;
	}

	//
	//  Try temp directory first...
	//
	GetTempPath(sizeof(szTmpPath), szTmpPath);
	if (szTmpPath && *szTmpPath)
	{
		*lpszRootPathName = *szTmpPath;
		
		dwBytesPerCluster = dwNumberOfFreeClusters = 0;
		if (MyGetDiskFreeSpace(lpszRootPathName, 
						   &dwBytesPerCluster, 
						   &dwNumberOfFreeClusters))
		{
			//
			//  If there's enough free space on the drive...
			//
			if ( dwBytesPerCluster * dwNumberOfFreeClusters > (DWORD) ss.st_size)
			{
				if (0 != GetTempFileName(szTmpPath, 
										 TEXT("EBU"), 
										 0, 
										 pszBootStrappedName))
				{
					return (EBUCopyFile(pszSrcFileName, pszBootStrappedName, FALSE));
				}
			}
		}
	}

	//
	// Search for space to park temporary file
	// Enumerate Drives searching first for a fixed drive with enough freespace
	// then second for a removable drive with enough freespace)
	//
	dwDriveMask=GetLogicalDrives();
	//
	// First pass for DRIVE_FIXED. Second pass for DRIVE_REMOVABLE
	//
	for (nPass = 0; nPass < 2; nPass++)
	{
		//
		// Enumerate all drive letters the dsDriveMask will eliminate them
		// 
		for (*lpszRootPathName=TEXT('a');*lpszRootPathName<=TEXT('z');(*lpszRootPathName)++)
		{
			if (dwDriveMask & 1)
			{
				// drive exists.
				uiDriveType = 0;
				uiDriveType = GetDriveType(lpszRootPathName);
				if (fFirstPass ? DRIVE_FIXED == uiDriveType : DRIVE_REMOVABLE == uiDriveType)
				{
					dwBytesPerCluster = dwNumberOfFreeClusters = 0;
					if (MyGetDiskFreeSpace(lpszRootPathName, 
									   &dwBytesPerCluster, 
									   &dwNumberOfFreeClusters))
					{
						//
						//If there's enough free space on the drive...
						//
						if (dwBytesPerCluster * dwNumberOfFreeClusters > (DWORD) ss.st_size)
						{
							if (0 != GetTempFileName(lpszRootPathName,
													 TEXT("EBU"), 
													 0, 
													 pszBootStrappedName))
							{
								return (EBUCopyFile(pszSrcFileName, pszBootStrappedName, FALSE));
							}
						}
					}
				}
			}
		}
		fFirstPass = FALSE;
	}
	return (FALSE);
}

//****************************************************************************
// Append5C
//    APPEND 0x5C if there is no 0x5C in the end of given buffer.
//
//****************************************************************************
VOID Append5C( char *szPath )
{
	if ( '\\' != *CharPrev(szPath,szPath+lstrlen(szPath)) ){
		lstrcat( szPath, "\\" );
	}
}


void DelTree(char P_szPath[])
{
	WIN32_FIND_DATA   
		FindData;
	HANDLE            
		hFind = NULL;
	BOOL
		bFindFile = TRUE;
	char 
		szFile[_MAX_PATH] = {""},
		szTemp[MAX_PATH] = {""};;

	strcpy(szTemp, P_szPath);
	if (IsDBCS()){
		Append5C( szTemp );
	} else {
		assert( strlen(szTemp) ); // 0 will be GPF : Jul.17,1997 12:11 by yutaka.
	
		if ('\\' != szTemp[strlen(szTemp)-1])
			strcat(szTemp, "\\");
	}
	strcat(szTemp, "*.*");

	// need to step through all the files in this directory and delete them first
	hFind = FindFirstFile(szTemp, &FindData);
	while((INVALID_HANDLE_VALUE != hFind) && bFindFile)
	{
		if(*(FindData.cFileName) != '.')
		{
			lstrcpy(szFile, P_szPath);
			if (IsDBCS()){
				Append5C( szFile );
			} else {
				assert( strlen(szFile) ); // 0 will be GPF : Jul.17,1997 12:11 by yutaka.
			
				if ('\\' != szFile[strlen(szFile)-1])
					strcat(szFile, "\\");
			}

			if(*FindData.cAlternateFileName != '\0')
				lstrcat(szFile,FindData.cAlternateFileName);
			else
				lstrcat(szFile,FindData.cFileName);

			if (FILE_ATTRIBUTE_DIRECTORY == GetFileAttributes(szFile))
			{
				// we have found another directory so remove it (yes its recursive)
				DelTree(szFile);

			}else{
				int
					iLoop = 0;
				// make the file deletable
				SetFileAttributes(szFile,FILE_ATTRIBUTE_NORMAL);

				//delete the file

				// five tries or the file already gone
				while ( (0xFFFFFFFF != GetFileAttributes(szFile)) && (iLoop < 5 ))
				{
#ifdef _DEBUG
					TRACE("Deleteing: %s\n", szFile);
#endif

					if (DeleteFile(szFile))
						break;

					Sleep ( 2000 );

					iLoop++;
				}

				if ( 0xFFFFFFFF != GetFileAttributes(szFile))
				{
#ifdef _DEBUG
					TRACE("NOT Deleted: %s\n", szFile);
#endif
				}
			}
		}
		//find the next file
		bFindFile = FindNextFile(hFind, &FindData);
	}

	if(hFind != INVALID_HANDLE_VALUE)
		FindClose(hFind);

	// now remove the directory
	RemoveDirectory(P_szPath);

	return;
}
int EBUStrlen(char *string)
{
	int x=0;
	while(*string != '\0')
	{
		string = CharNext(string);
		x++;
	}
	return x;
}

// Given a file name, possibly with wild cards, spin down the directory tree
// deleting any match
void DelFileInTree(char P_szPath[])
{
	WIN32_FIND_DATA   
		FindData;
	HANDLE            
		hFind = NULL;
	BOOL
		bFindFile = TRUE;
	char 
		szFile[_MAX_PATH] = {""},
		szName[_MAX_PATH] = {""},
		szTemp[MAX_PATH] = {""},
		szPath[MAX_PATH] = {""};
	int PathLen, NameLen;

	strcpy(szTemp, P_szPath);
	if (IsDBCS()){
		Append5C( szTemp );
	} else {
		assert( strlen(szTemp) ); // 0 will be GPF : Jul.17,1997 12:11 by yutaka.
	
		if ('\\' != szTemp[strlen(szTemp)-1])
			strcat(szTemp, "\\");
	}

	PathLen = strlen(szTemp) - 1;
	NameLen = 0;
	while (PathLen && szTemp[PathLen] != '\\' )	PathLen--, NameLen++;

	if ( PathLen )
	{
		strcpy ( szName, &szTemp[PathLen+1] );
		szTemp[PathLen+1] = 0;
		strcpy ( szPath, szTemp );
		strcat(szTemp, "*.*");

		// need to step through all the files in this directory and recurse on directories
		hFind = FindFirstFile(szTemp, &FindData);
		while((INVALID_HANDLE_VALUE != hFind) && bFindFile)
		{
			if(*(FindData.cFileName) != '.')
			{
				lstrcpy(szFile, szPath);
				if (IsDBCS()){
					Append5C( szFile );
				} else {
					assert( strlen(szFile) ); // 0 will be GPF : Jul.17,1997 12:11 by yutaka.
				
					if ('\\' != szFile[strlen(szFile)-1])
						strcat(szFile, "\\");
				}
	
				if(*FindData.cAlternateFileName != '\0')
					lstrcat(szFile,FindData.cAlternateFileName);
				else
					lstrcat(szFile,FindData.cFileName);

				if (FILE_ATTRIBUTE_DIRECTORY == GetFileAttributes(szFile))
				{
					// we have found another directory so remove it (yes its recursive)
					strcat ( szFile, "\\" );
					strcat ( szFile, szName );
					DelFileInTree(szFile);
				}
			}
			//find the next file
			bFindFile = FindNextFile(hFind, &FindData);
		}

		if(hFind != INVALID_HANDLE_VALUE)
			FindClose(hFind);

		hFind = FindFirstFile(P_szPath, &FindData);
		while((INVALID_HANDLE_VALUE != hFind) && bFindFile)
		{
				if(*(FindData.cFileName) != '.')
				{
					char szFile[_MAX_PATH];
					lstrcpy(szFile,szPath);
					lstrcat(szFile,"\\");
					if(*FindData.cAlternateFileName != '\0')
						lstrcat(szFile,FindData.cAlternateFileName);
					else
						lstrcat(szFile,FindData.cFileName);
					SetFileAttributes(szFile,FILE_ATTRIBUTE_NORMAL);
		
					//delete the file
					DeleteFile(szFile);
			}
			//find the next file
			bFindFile = FindNextFile(hFind, &FindData);
		}

		if(hFind != INVALID_HANDLE_VALUE)
			FindClose(hFind);
	}

	return;
}

BOOL DoesFileExistNoCriticalErrors( LPCSTR lpszFilename )
{
	DWORD	dwGetFileAttributes = 0xFFFFFFFF;
	UINT	uiLastErrorMode=0;;
	uiLastErrorMode = SetErrorMode(SEM_NOOPENFILEERRORBOX | SEM_FAILCRITICALERRORS);
	dwGetFileAttributes = GetFileAttributes(lpszFilename);
	SetErrorMode(uiLastErrorMode);
	return (0xFFFFFFFF != dwGetFileAttributes);
}


LPTSTR EBUlstrstri(LPTSTR lpSearch, LPTSTR lpFind)
{
	TCHAR   szUSearchChar;
	TCHAR   szUFindChar;
	LPTSTR lpFindStart;
	LPTSTR	lpMatchStart;


	// Test pointers and for "" Find
	if (lpSearch && lpFind && *lpFind)
	{
		//Save the search start pointer in case of partial matches.
		lpFindStart  = lpFind;
		lpMatchStart = NULL;
		// Keep looking until NULL character.
		while (*lpSearch)
		{
			// does this char match.
			szUSearchChar = *lpSearch;
			szUSearchChar = (TCHAR) CharUpper((LPTSTR) szUSearchChar);
			szUFindChar = *lpFind;
			szUFindChar = (TCHAR) CharUpper((LPTSTR) szUFindChar);

			if (szUSearchChar == szUFindChar)
			{
				// Save the pointer to the beginning of the match
				if (!lpMatchStart)
				{
					lpMatchStart = lpSearch;
				}
				//Have all find chars been matched.
				lpFind = CharNext(lpFind);
				if ('\0' ==*lpFind)
				{
					return lpMatchStart;
				}
				// More chars to compare in the find string
				lpSearch = CharNext(lpSearch);
			}
			// Match Failed. Reset find pointer and next search character pointer.
			else
			{
				lpFind = lpFindStart;
				if (lpMatchStart)
				{
					lpSearch = CharNext(lpMatchStart);
					lpMatchStart = NULL;
				}
				else
				{
					lpSearch = CharNext(lpSearch);
				}
			}
		}
	}
	// Null pointers passed in or no match;
	return NULL;
}


void TRACE(LPCTSTR lpszFormat, ... )
{
	va_list	vlShow;
	TCHAR	szText[2048] = "";

	
	va_start (vlShow, lpszFormat);
	wvsprintf (szText, lpszFormat, vlShow);
	va_end (vlShow);

#ifdef _DEBUG
	OutputDebugString(szText);
#endif
}




void TraceLastError(DWORD dwLastError, BOOL bAssert)
{
	LPVOID
		lpMsgBuf;

	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, dwLastError,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
		(LPTSTR) &lpMsgBuf, 0, NULL);

	TRACE("Last System Error: %s\n", (LPSTR)lpMsgBuf);

	if (bAssert)
	{
		assert(!(LPSTR)lpMsgBuf);
	}

	LocalFree(lpMsgBuf);	// clean up the memory that FormatMessage allocated
}


void TraceMciError(MCIERROR LastError, BOOL bAssert)
{
	char
		strErrMsg[128];	// 128 is max back from MCI

	mciGetErrorString(LastError, strErrMsg, sizeof(strErrMsg));

	TRACE("Last MCI Error: %s\n", strErrMsg);

	if (bAssert)
	{
		assert(!(LPSTR)strErrMsg);
	}

}

//Structure for lookup tables
typedef struct
{
    DWORD dwKey;
    char * sz;
} TabElem;

//Lookup table for CTrace param strings
static TabElem TrcParamTab [] = 
{
	// button messages
    {(DWORD)BM_GETCHECK, "BM_GETCHECK"},
    {(DWORD)BM_SETCHECK, "BM_SETCHECK"},
    {(DWORD)BM_GETSTATE, "BM_GETSTATE"},
    {(DWORD)BM_SETSTATE, "BM_SETSTATE"},
    {(DWORD)BM_SETSTYLE, "BM_SETSTYLE"},
    {(DWORD)BM_CLICK   , "BM_CLICK"},
    {(DWORD)BM_GETIMAGE, "BM_GETIMAGE"},
    {(DWORD)BM_SETIMAGE, "BM_SETIMAGE"},

//Richedit Messages
    {(DWORD)EM_GETLIMITTEXT, "EM_GETLIMITTEXT"},
    {(DWORD)EM_POSFROMCHAR, "EM_POSFROMCHAR"},
    {(DWORD)EM_CHARFROMPOS, "EM_CHARFROMPOS"},
    {(DWORD)EM_SCROLLCARET, "EM_SCROLLCARET"},
    {(DWORD)EM_CANPASTE, "EM_CANPASTE"},
    {(DWORD)EM_DISPLAYBAND, "EM_DISPLAYBAND"},
    {(DWORD)EM_EXGETSEL, "EM_EXGETSEL"},
    {(DWORD)EM_EXLIMITTEXT, "EM_EXLIMITTEXT"},
    {(DWORD)EM_EXLINEFROMCHAR, "EM_EXLINEFROMCHAR"},
    {(DWORD)EM_EXSETSEL, "EM_EXSETSEL"},
    {(DWORD)EM_FINDTEXT, "EM_FINDTEXT"},
    {(DWORD)EM_FORMATRANGE, "EM_FORMATRANGE"},
    {(DWORD)EM_GETCHARFORMAT, "EM_GETCHARFORMAT"},
    {(DWORD)EM_GETEVENTMASK, "EM_GETEVENTMASK"},
    {(DWORD)EM_GETOLEINTERFACE, "EM_GETOLEINTERFACE"},
    {(DWORD)EM_GETPARAFORMAT, "EM_GETPARAFORMAT"},
    {(DWORD)EM_GETSELTEXT, "EM_GETSELTEXT"},
    {(DWORD)EM_HIDESELECTION, "EM_HIDESELECTION"},
    {(DWORD)EM_PASTESPECIAL, "EM_PASTESPECIAL"},
    {(DWORD)EM_REQUESTRESIZE, "EM_REQUESTRESIZE"},
    {(DWORD)EM_SELECTIONTYPE, "EM_SELECTIONTYPE"},
    {(DWORD)EM_SETBKGNDCOLOR, "EM_SETBKGNDCOLOR"},
    {(DWORD)EM_SETCHARFORMAT, "EM_SETCHARFORMAT"},
    {(DWORD)EM_SETEVENTMASK, "EM_SETEVENTMASK"},
    {(DWORD)EM_SETOLECALLBACK, "EM_SETOLECALLBACK"},
    {(DWORD)EM_SETPARAFORMAT, "EM_SETPARAFORMAT"},
    {(DWORD)EM_SETTARGETDEVICE, "EM_SETTARGETDEVICE"},
    {(DWORD)EM_STREAMIN, "EM_STREAMIN"},
    {(DWORD)EM_STREAMOUT, "EM_STREAMOUT"},
    {(DWORD)EM_GETTEXTRANGE, "EM_GETTEXTRANGE"},
    {(DWORD)EM_FINDWORDBREAK, "EM_FINDWORDBREAK"},
    {(DWORD)EM_SETOPTIONS, "EM_SETOPTIONS"},
    {(DWORD)EM_GETOPTIONS, "EM_GETOPTIONS"},
    {(DWORD)EM_FINDTEXTEX, "EM_FINDTEXTEX"},
    {(DWORD)EM_GETWORDBREAKPROCEX, "EM_GETWORDBREAKPROCEX"},
    {(DWORD)EM_SETWORDBREAKPROCEX, "EM_SETWORDBREAKPROCEX"},
    {(DWORD)EM_SETUNDOLIMIT, "EM_SETUNDOLIMIT"},
    {(DWORD)EM_REDO, "EM_REDO"},
    {(DWORD)EM_CANREDO, "EM_CANREDO"},
    {(DWORD)EM_SETPUNCTUATION, "EM_SETPUNCTUATION"},
    {(DWORD)EM_GETPUNCTUATION, "EM_GETPUNCTUATION"},
    {(DWORD)EM_SETWORDWRAPMODE, "EM_SETWORDWRAPMODE"},
    {(DWORD)EM_GETWORDWRAPMODE, "EM_GETWORDWRAPMODE"},
    {(DWORD)EM_SETIMECOLOR, "EM_SETIMECOLOR"},
    {(DWORD)EM_GETIMECOLOR, "EM_GETIMECOLOR"},
    {(DWORD)EM_SETIMEOPTIONS, "EM_SETIMEOPTIONS"},
    {(DWORD)EM_GETIMEOPTIONS, "EM_GETIMEOPTIONS"},
    {(DWORD)EN_MSGFILTER, "EN_MSGFILTER"},
    {(DWORD)EN_REQUESTRESIZE, "EN_REQUESTRESIZE"},
    {(DWORD)EN_SELCHANGE, "EN_SELCHANGE"},
    {(DWORD)EN_DROPFILES, "EN_DROPFILES"},
    {(DWORD)EN_PROTECTED, "EN_PROTECTED"},
    {(DWORD)EN_CORRECTTEXT, "EN_CORRECTTEXT"},
    {(DWORD)EN_STOPNOUNDO, "EN_STOPNOUNDO"},
    {(DWORD)EN_IMECHANGE, "EN_IMECHANGE"},
    {(DWORD)EN_SAVECLIPBOARD, "EN_SAVECLIPBOARD"},
    {(DWORD)EN_OLEOPFAILED, "EN_OLEOPFAILED"},

	// standard edit messages
    {(DWORD)EM_GETFIRSTVISIBLELINE, "EM_GETFIRSTVISIBLELINE"},
    {(DWORD)EM_LIMITTEXT, "EM_LIMITTEXT"},
    {(DWORD)EM_GETLINECOUNT, "EM_GETLINECOUNT"},

	//Window Messages

	{(DWORD)WM_NULL, "WM_NULL"},
	{(DWORD)WM_CREATE, "WM_CREATE"},
	{(DWORD)WM_DESTROY, "WM_DESTROY"},
	{(DWORD)WM_MOVE, "WM_MOVE"},
	{(DWORD)WM_SIZE, "WM_SIZE"},
	{(DWORD)WM_ACTIVATE, "WM_ACTIVATE"},
	{(DWORD)WM_SETFOCUS, "WM_SETFOCUS"},
	{(DWORD)WM_KILLFOCUS, "WM_KILLFOCUS"},
	{(DWORD)WM_ENABLE, "WM_ENABLE"},
	{(DWORD)WM_SETREDRAW, "WM_SETREDRAW"},
	{(DWORD)WM_SETTEXT, "WM_SETTEXT"},
	{(DWORD)WM_GETTEXT, "WM_GETTEXT"},
	{(DWORD)WM_GETTEXTLENGTH, "WM_GETTEXTLENGTH"},
	{(DWORD)WM_PAINT, "WM_PAINT"},
	{(DWORD)WM_CLOSE, "WM_CLOSE"},
	{(DWORD)WM_QUERYENDSESSION, "WM_QUERYENDSESSION"},
	{(DWORD)WM_QUIT, "WM_QUIT"},
	{(DWORD)WM_QUERYOPEN, "WM_QUERYOPEN"},
	{(DWORD)WM_ERASEBKGND, "WM_ERASEBKGND"},
	{(DWORD)WM_SYSCOLORCHANGE, "WM_SYSCOLORCHANGE"},
	{(DWORD)WM_ENDSESSION, "WM_ENDSESSION"},
	{(DWORD)WM_SHOWWINDOW, "WM_SHOWWINDOW"},
	{(DWORD)WM_WININICHANGE, "WM_WININICHANGE"},
	{(DWORD)WM_SETTINGCHANGE, "WM_SETTINGCHANGE"},
	{(DWORD)WM_DEVMODECHANGE, "WM_DEVMODECHANGE"},
	{(DWORD)WM_ACTIVATEAPP, "WM_ACTIVATEAPP"},
	{(DWORD)WM_FONTCHANGE, "WM_FONTCHANGE"},
	{(DWORD)WM_TIMECHANGE, "WM_TIMECHANGE"},
	{(DWORD)WM_CANCELMODE, "WM_CANCELMODE"},
	{(DWORD)WM_SETCURSOR, "WM_SETCURSOR"},
	{(DWORD)WM_MOUSEACTIVATE, "WM_MOUSEACTIVATE"},
	{(DWORD)WM_CHILDACTIVATE, "WM_CHILDACTIVATE"},
	{(DWORD)WM_QUEUESYNC, "WM_QUEUESYNC"},
	{(DWORD)WM_GETMINMAXINFO, "WM_GETMINMAXINFO"},
	{(DWORD)WM_PAINTICON, "WM_PAINTICON"},
	{(DWORD)WM_ICONERASEBKGND, "WM_ICONERASEBKGND"},
	{(DWORD)WM_NEXTDLGCTL, "WM_NEXTDLGCTL"},
	{(DWORD)WM_SPOOLERSTATUS, "WM_SPOOLERSTATUS"},
	{(DWORD)WM_DRAWITEM, "WM_DRAWITEM"},
	{(DWORD)WM_MEASUREITEM, "WM_MEASUREITEM"},
	{(DWORD)WM_DELETEITEM, "WM_DELETEITEM"},
	{(DWORD)WM_VKEYTOITEM, "WM_VKEYTOITEM"},
	{(DWORD)WM_CHARTOITEM, "WM_CHARTOITEM"},
	{(DWORD)WM_SETFONT, "WM_SETFONT"},
	{(DWORD)WM_GETFONT, "WM_GETFONT"},
	{(DWORD)WM_SETHOTKEY, "WM_SETHOTKEY"},
	{(DWORD)WM_GETHOTKEY, "WM_GETHOTKEY"},
	{(DWORD)WM_QUERYDRAGICON, "WM_QUERYDRAGICON"},
	{(DWORD)WM_COMPAREITEM, "WM_COMPAREITEM"},
	{(DWORD)WM_COMPACTING, "WM_COMPACTING"},
	{(DWORD)WM_COMMNOTIFY, "WM_COMMNOTIFY"},
	{(DWORD)WM_WINDOWPOSCHANGING, "WM_WINDOWPOSCHANGING"},
	{(DWORD)WM_WINDOWPOSCHANGED, "WM_WINDOWPOSCHANGED"},
	{(DWORD)WM_POWER, "WM_POWER"},
	{(DWORD)WM_COPYDATA, "WM_COPYDATA"},
	{(DWORD)WM_CANCELJOURNAL, "WM_CANCELJOURNAL"},
	{(DWORD)WM_NOTIFY, "WM_NOTIFY"},
	{(DWORD)WM_INPUTLANGCHANGEREQUEST, "WM_INPUTLANGCHANGEREQUEST"},
	{(DWORD)WM_INPUTLANGCHANGE, "WM_INPUTLANGCHANGE"},
	{(DWORD)WM_TCARD, "WM_TCARD"},
	{(DWORD)WM_HELP, "WM_HELP"},
	{(DWORD)WM_USERCHANGED, "WM_USERCHANGED"},
	{(DWORD)WM_NOTIFYFORMAT, "WM_NOTIFYFORMAT"},
	{(DWORD)WM_CONTEXTMENU, "WM_CONTEXTMENU"},
	{(DWORD)WM_STYLECHANGING, "WM_STYLECHANGING"},
	{(DWORD)WM_STYLECHANGED, "WM_STYLECHANGED"},
	{(DWORD)WM_DISPLAYCHANGE, "WM_DISPLAYCHANGE"},
	{(DWORD)WM_GETICON, "WM_GETICON"},
	{(DWORD)WM_SETICON, "WM_SETICON"},
	{(DWORD)WM_NCCREATE, "WM_NCCREATE"},
	{(DWORD)WM_NCDESTROY, "WM_NCDESTROY"},
	{(DWORD)WM_NCCALCSIZE, "WM_NCCALCSIZE"},
	{(DWORD)WM_NCHITTEST, "WM_NCHITTEST"},
	{(DWORD)WM_NCPAINT, "WM_NCPAINT"},
	{(DWORD)WM_NCACTIVATE, "WM_NCACTIVATE"},
	{(DWORD)WM_GETDLGCODE, "WM_GETDLGCODE"},
	{(DWORD)WM_NCMOUSEMOVE, "WM_NCMOUSEMOVE"},
	{(DWORD)WM_NCLBUTTONDOWN, "WM_NCLBUTTONDOWN"},
	{(DWORD)WM_NCLBUTTONUP, "WM_NCLBUTTONUP"},
	{(DWORD)WM_NCLBUTTONDBLCLK, "WM_NCLBUTTONDBLCLK"},
	{(DWORD)WM_NCRBUTTONDOWN, "WM_NCRBUTTONDOWN"},
	{(DWORD)WM_NCRBUTTONUP, "WM_NCRBUTTONUP"},
	{(DWORD)WM_NCRBUTTONDBLCLK, "WM_NCRBUTTONDBLCLK"},
	{(DWORD)WM_NCMBUTTONDOWN, "WM_NCMBUTTONDOWN"},
	{(DWORD)WM_NCMBUTTONUP, "WM_NCMBUTTONUP"},
	{(DWORD)WM_NCMBUTTONDBLCLK, "WM_NCMBUTTONDBLCLK"},
	{(DWORD)WM_KEYFIRST, "WM_KEYFIRST"},
	{(DWORD)WM_KEYDOWN, "WM_KEYDOWN"},
	{(DWORD)WM_KEYUP, "WM_KEYUP"},
	{(DWORD)WM_CHAR, "WM_CHAR"},
	{(DWORD)WM_DEADCHAR, "WM_DEADCHAR"},
	{(DWORD)WM_SYSKEYDOWN, "WM_SYSKEYDOWN"},
	{(DWORD)WM_SYSKEYUP, "WM_SYSKEYUP"},
	{(DWORD)WM_SYSCHAR, "WM_SYSCHAR"},
	{(DWORD)WM_SYSDEADCHAR, "WM_SYSDEADCHAR"},
	{(DWORD)WM_KEYLAST, "WM_KEYLAST"},
	{(DWORD)WM_IME_STARTCOMPOSITION, "WM_IME_STARTCOMPOSITION"},
	{(DWORD)WM_IME_ENDCOMPOSITION, "WM_IME_ENDCOMPOSITION"},
	{(DWORD)WM_IME_COMPOSITION, "WM_IME_COMPOSITION"},
	{(DWORD)WM_IME_KEYLAST, "WM_IME_KEYLAST"},
	{(DWORD)WM_INITDIALOG, "WM_INITDIALOG"},
	{(DWORD)WM_COMMAND, "WM_COMMAND"},
	{(DWORD)WM_SYSCOMMAND, "WM_SYSCOMMAND"},
	{(DWORD)WM_TIMER, "WM_TIMER"},
	{(DWORD)WM_HSCROLL, "WM_HSCROLL"},
	{(DWORD)WM_VSCROLL, "WM_VSCROLL"},
	{(DWORD)WM_INITMENU, "WM_INITMENU"},
	{(DWORD)WM_INITMENUPOPUP, "WM_INITMENUPOPUP"},
	{(DWORD)WM_MENUSELECT, "WM_MENUSELECT"},
	{(DWORD)WM_MENUCHAR, "WM_MENUCHAR"},
//	{(DWORD)WM_CHANGEUISTATE, "WM_CHANGEUISTATE"},	// nt 5.0 only
//	{(DWORD)WM_UPDATEUISTATE, "WM_UPDATEUISTATE"},	// nt 5.0 only
//	{(DWORD)WM_QUERYUISTATE, "WM_QUERYUISTATE"},	// nt 5.0 only
	{(DWORD)0x127, "WM_CHANGEUISTATE"},	// nt 5.0 only
	{(DWORD)0x128, "WM_UPDATEUISTATE"},	// nt 5.0 only
	{(DWORD)0x129, "WM_QUERYUISTATE"},	// nt 5.0 only
	{(DWORD)WM_ENTERIDLE, "WM_ENTERIDLE"},
	{(DWORD)WM_CTLCOLORMSGBOX, "WM_CTLCOLORMSGBOX"},
	{(DWORD)WM_CTLCOLOREDIT, "WM_CTLCOLOREDIT"},
	{(DWORD)WM_CTLCOLORLISTBOX, "WM_CTLCOLORLISTBOX"},
	{(DWORD)WM_CTLCOLORBTN, "WM_CTLCOLORBTN"},
	{(DWORD)WM_CTLCOLORDLG, "WM_CTLCOLORDLG"},
	{(DWORD)WM_CTLCOLORSCROLLBAR, "WM_CTLCOLORSCROLLBAR"},
	{(DWORD)WM_CTLCOLORSTATIC, "WM_CTLCOLORSTATIC"},
	{(DWORD)WM_MOUSEFIRST, "WM_MOUSEMOVE / WM_MOUSEFIRST"},
	//{(DWORD)WM_MOUSEMOVE, "WM_MOUSEMOVE"},
	{(DWORD)0x20A, "WM_MOUSEWHEEL"}, // WM_MOUSEWHEEL
	{(DWORD)WM_LBUTTONDOWN, "WM_LBUTTONDOWN"},
	{(DWORD)WM_LBUTTONUP, "WM_LBUTTONUP"},
	{(DWORD)WM_LBUTTONDBLCLK, "WM_LBUTTONDBLCLK"},
	{(DWORD)WM_RBUTTONDOWN, "WM_RBUTTONDOWN"},
	{(DWORD)WM_RBUTTONUP, "WM_RBUTTONUP"},
	{(DWORD)WM_RBUTTONDBLCLK, "WM_RBUTTONDBLCLK"},
	{(DWORD)WM_MBUTTONDOWN, "WM_MBUTTONDOWN"},
	{(DWORD)WM_MBUTTONUP, "WM_MBUTTONUP"},
	{(DWORD)WM_MBUTTONDBLCLK, "WM_MBUTTONDBLCLK"},
	{(DWORD)WM_MOUSELAST, "WM_MOUSELAST"},
	{(DWORD)WM_PARENTNOTIFY, "WM_PARENTNOTIFY"},
	{(DWORD)WM_ENTERMENULOOP, "WM_ENTERMENULOOP"},
	{(DWORD)WM_EXITMENULOOP, "WM_EXITMENULOOP"},
	{(DWORD)WM_NEXTMENU, "WM_NEXTMENU"},
	{(DWORD)WM_SIZING, "WM_SIZING"},
	{(DWORD)WM_CAPTURECHANGED, "WM_CAPTURECHANGED"},
	{(DWORD)WM_MOVING, "WM_MOVING"},
	{(DWORD)WM_POWERBROADCAST, "WM_POWERBROADCAST"},
	{(DWORD)WM_DEVICECHANGE, "WM_DEVICECHANGE"},
	{(DWORD)WM_IME_SETCONTEXT, "WM_IME_SETCONTEXT"},
	{(DWORD)WM_IME_NOTIFY, "WM_IME_NOTIFY"},
	{(DWORD)WM_IME_CONTROL, "WM_IME_CONTROL"},
	{(DWORD)WM_IME_COMPOSITIONFULL, "WM_IME_COMPOSITIONFULL"},
	{(DWORD)WM_IME_SELECT, "WM_IME_SELECT"},
	{(DWORD)WM_IME_CHAR, "WM_IME_CHAR"},
	{(DWORD)WM_IME_KEYDOWN, "WM_IME_KEYDOWN"},
	{(DWORD)WM_IME_KEYUP, "WM_IME_KEYUP"},
	{(DWORD)WM_MDICREATE, "WM_MDICREATE"},
	{(DWORD)WM_MDIDESTROY, "WM_MDIDESTROY"},
	{(DWORD)WM_MDIACTIVATE, "WM_MDIACTIVATE"},
	{(DWORD)WM_MDIRESTORE, "WM_MDIRESTORE"},
	{(DWORD)WM_MDINEXT, "WM_MDINEXT"},
	{(DWORD)WM_MDIMAXIMIZE, "WM_MDIMAXIMIZE"},
	{(DWORD)WM_MDITILE, "WM_MDITILE"},
	{(DWORD)WM_MDICASCADE, "WM_MDICASCADE"},
	{(DWORD)WM_MDIICONARRANGE, "WM_MDIICONARRANGE"},
	{(DWORD)WM_MDIGETACTIVE, "WM_MDIGETACTIVE"},
	{(DWORD)WM_MDISETMENU, "WM_MDISETMENU"},
	{(DWORD)WM_ENTERSIZEMOVE, "WM_ENTERSIZEMOVE"},
	{(DWORD)WM_EXITSIZEMOVE, "WM_EXITSIZEMOVE"},
	{(DWORD)WM_DROPFILES, "WM_DROPFILES"},
	{(DWORD)WM_MDIREFRESHMENU, "WM_MDIREFRESHMENU"},
	{(DWORD)WM_CUT, "WM_CUT"},
	{(DWORD)WM_COPY, "WM_COPY"},
	{(DWORD)WM_PASTE, "WM_PASTE"},
	{(DWORD)WM_CLEAR, "WM_CLEAR"},
	{(DWORD)WM_UNDO, "WM_UNDO"},
	{(DWORD)WM_RENDERFORMAT, "WM_RENDERFORMAT"},
	{(DWORD)WM_RENDERALLFORMATS, "WM_RENDERALLFORMATS"},
	{(DWORD)WM_DESTROYCLIPBOARD, "WM_DESTROYCLIPBOARD"},
	{(DWORD)WM_DRAWCLIPBOARD, "WM_DRAWCLIPBOARD"},
	{(DWORD)WM_PAINTCLIPBOARD, "WM_PAINTCLIPBOARD"},
	{(DWORD)WM_VSCROLLCLIPBOARD, "WM_VSCROLLCLIPBOARD"},
	{(DWORD)WM_SIZECLIPBOARD, "WM_SIZECLIPBOARD"},
	{(DWORD)WM_ASKCBFORMATNAME, "WM_ASKCBFORMATNAME"},
	{(DWORD)WM_CHANGECBCHAIN, "WM_CHANGECBCHAIN"},
	{(DWORD)WM_HSCROLLCLIPBOARD, "WM_HSCROLLCLIPBOARD"},
	{(DWORD)WM_QUERYNEWPALETTE, "WM_QUERYNEWPALETTE"},
	{(DWORD)WM_PALETTEISCHANGING, "WM_PALETTEISCHANGING"},
	{(DWORD)WM_PALETTECHANGED, "WM_PALETTECHANGED"},
	{(DWORD)WM_HOTKEY, "WM_HOTKEY"},
	{(DWORD)WM_PRINT, "WM_PRINT"},
	{(DWORD)WM_PRINTCLIENT, "WM_PRINTCLIENT"},
	{(DWORD)WM_HANDHELDFIRST, "WM_HANDHELDFIRST"},
	{(DWORD)WM_HANDHELDLAST, "WM_HANDHELDLAST"},
	{(DWORD)WM_AFXFIRST, "WM_AFXFIRST"},
	{(DWORD)WM_AFXLAST, "WM_AFXLAST"},
	{(DWORD)WM_PENWINFIRST, "WM_PENWINFIRST"},
	{(DWORD)WM_PENWINLAST, "WM_PENWINLAST"},
	{(DWORD)WM_APP, "WM_APP"},
	{(DWORD)WM_USER, "WM_USER"}
};

/*
 *  TabLookup
 *	
 *  @mfunc
 *      This function searches an array of TabElem
 *      structures looking for an entry whose key
 *      matches the one we were given. If found, it
 *      copies the string associated with the key into
 *      the supplied buffer.
 *      
 *      Table - TabElem pointer to start of array.
 *      TabSize - Size of array in bytes.
 *      dwKey - Key to match.
 *      szBuf - Buffer to hold string (assumed MAXDEBUGSTRLEN in size).
 *
 *  @rdesc
 *      FALSE if key not found, TRUE if found.
 *
 */
BOOL TabLookup(TabElem * Table, UINT TabSize, DWORD dwKey, LPSTR szBuf)
{
    BOOL fRet = FALSE;
    UINT cTab, index;
    
    cTab = TabSize/sizeof(TabElem);

    for (index = 0; index < cTab; index++)
    {
        if (Table[index].dwKey == dwKey)
            break;
    }

    if (index < cTab)
    {
        lstrcpyA(szBuf, Table[index].sz);
        fRet = TRUE;
    }

    return fRet;
}

#define MAXDEBUGSTRLEN (MAX_PATH + MAX_PATH)
void GetParamSz(DWORD dwParam, LPSTR szBuf)
{
    char szTemp[MAXDEBUGSTRLEN];
	if (dwParam > WM_USER)
	{
		sprintf(szBuf, "Message in WM_USER area.");
		return;
	}

    if (!TabLookup(TrcParamTab, sizeof(TrcParamTab), (DWORD)dwParam, szTemp))
	{
        sprintf(szBuf, "Unrecognized message");
	}
	else
	{
        sprintf(szBuf, "%s", szTemp);
	}
}



void TraceMessage(DWORD dwMessageID)
{
	char
		sz[MAXDEBUGSTRLEN];

	GetParamSz(dwMessageID, sz);

	TRACE("Message 0x%04X: %s.\n", dwMessageID, sz);
}

bool EBUCreateDirectory(char * pszPath)
{

	bool	bCreateSomething = FALSE;
	char	szTempPath[MAX_PATH];
	char	*pcSrc = pszPath;
	char	*pcDest = szTempPath;

	// we have to create the directory one step at a time because CreateDirectory wont do a full tree
	
	// start transfering chars from the src path to the temp until we hit a \ or NULL
	while (*pcSrc)
	{
		if (IsDBCS())
		{
			char *pc = CharNext(pcSrc);
			while ( pcSrc < pc ){
				*pcDest++ = *pcSrc++;
			}
		}
		else
		{
			*pcDest++ = *pcSrc++;
		}
		
		if ((*pcSrc == '\\') || !*pcSrc)
		{
			// we have finished copying a dir chunk or finished
			*pcDest = NULL;
			if (CreateDirectory(szTempPath, NULL))
			{
				// we created something
				bCreateSomething = TRUE;
			}
		}
	}

	return bCreateSomething;
}
