//
// restart.cpp
//
//		Dialog box proc for the Restart Windows dialog, as well as
//		routines to add files to the list of files we must copy while
//		restarting Windows.
//
// History:
//
//		 2/03/95	KenSh		Created
//

#include "stubpch.h"
#include "setup.h"
#include "HotSetupRC.h"
#include "restart.h"
#include "util.h"
#include "vercopy.h"
#include "hotsetup.h"

using namespace NGLOBALS;
//
//Globals for the restart file list we build for Windows 95
//
static TCHAR	g_szWinInitFile[_MAX_PATH * sizeof(TCHAR)];
typedef struct RestartTag
{
	TCHAR *szDestName;
	TCHAR *szSrcName;
	struct RestartTag *peNext;
} RESTARTINFO;
static RESTARTINFO *peRestartHead = NULL;

BOOL GetRebootFlag()
{
	return GetReboot();
}


//****************************************************************************
// Procedure	AddFileToExitWindowsList
//
// Purpose		Adds a file to the list of files we must copy while restarting
//				windows via ExitWindowsEx
//
// Parameters	lpszDest		the eventual destination file path
//				lpszTemp		path of the temporary copy of the file
//
// Returns		TRUE if successful, FALSE if it fails
//
// Comments		The caller is responsible for making sure that the file pointed
//              to by lpszTemp is in place before the system reboots...
//
// History		 1/31/95	KenSh		Created
//				 3/26/97    v-richei    Rewrote to work for Win 32 (NT and Win 95)
//
BOOL AddFileToExitWindowsList(LPCSTR lpszDest, LPCSTR lpszTemp)
{
	BOOL fRc;
	
	//
	//The MOVEFILE_DELAY_UNTIL_REBOOT flag is currently only implemented under Windows NT. 
	//Since Windows 95 will eventually support this flag, we'll call this function first.
	//If it fails, for the time being, we'll assume that we're running under Windows 95.
	//If run on a newer version of Windows 95 that supports the flag, this call will
	//automatically work...
	//
	fRc = MoveFileEx(lpszTemp, lpszDest, MOVEFILE_DELAY_UNTIL_REBOOT | MOVEFILE_REPLACE_EXISTING);

	if (!fRc)
	{
		GetWindowsDirectory(g_szWinInitFile, sizeof(g_szWinInitFile));
		lstrcat(g_szWinInitFile, __TEXT("\\WININIT.INI"));

		//
		//The contents of WININIT.INI are always ANSI, never UNICODE...
		//
		//WININIT.INI
		//[Rename]
		//destfilename1=tempfilename1
		//destfilename2=tempfilename2
		//
		//When Win 95 restarts, the system app WININIT.EXE processes this file and
		//renames the temp file names to the destination name.  Since WININIT.EXE
		//runs before the required operating system support loads, it does not
		//recognize long filenames!
		//
		if (0 == WritePrivateProfileString("Rename",
										   lpszDest ? lpszDest : "NUL", 
										   lpszTemp, 
										   g_szWinInitFile))
		{
			return FALSE;
		}
	}

	//
	//Add the new file to the list of files we put in the restart list
	//
	RESTARTINFO *peNew;
	RESTARTINFO *pePtr;

	peNew = (RESTARTINFO *) malloc(sizeof(RESTARTINFO));
	if (NULL == peNew)
	{
		return FALSE;
	}

	peNew->szDestName = (TCHAR *) malloc(lstrlen(lpszDest) + sizeof(TCHAR));
	if (NULL == peNew->szDestName)
	{
		free(peNew);
		return FALSE;
	}

	peNew->szSrcName = (TCHAR *) malloc(lstrlen(lpszTemp) + sizeof(TCHAR));
	if (NULL == peNew->szSrcName)
	{
		free(peNew->szDestName);
		free(peNew);
		return FALSE;
	}

	lstrcpy(peNew->szSrcName, lpszTemp);
	lstrcpy(peNew->szDestName, lpszDest);
	peNew->peNext = NULL;

	if (NULL == peRestartHead)
	{
		//
		//Flag that we need to reboot now...
		//
		SetupSetRebootFlag();

		peRestartHead = peNew;
	}
	else
	{
		for (pePtr=peRestartHead; pePtr->peNext; pePtr = pePtr->peNext);
		pePtr->peNext = peNew;
	}

	return TRUE;
}

void SetupSetRebootFlag()
{
	SetReboot(TRUE);
}


//
//Under Windows 95, the restart file list (WININIT.INI) should already be in place before
//this function is called (if there are any files that need to replaced, that is).  Under
//Windows NT, same circumstances, MoveFileEx files should already be flagged in the
//registry.  The AddFileToExitWindowsList takes care of all that...
//
BOOL RestartWindows(void)
{
	RESTARTINFO *pePtr = peRestartHead;
	RESTARTINFO *peTmp;

	//
	//First free the memory used by the restart list...
	//
	while (pePtr)
	{
		peTmp = pePtr->peNext;

		free(pePtr->szSrcName);
		free(pePtr->szDestName);
		free(pePtr);

		pePtr = peTmp;
	}

	peRestartHead = NULL;

	if (IsWindow(GetWndParent()))
	{
		ShowWindow(GetWndParent(), SW_HIDE);
	}

	HANDLE hToken;  
	TOKEN_PRIVILEGES tkp; 
	
	/* Get a token for this process. */ 
	
	if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
	{
		// if this failed we are probably running on Win95 which doesn't support the api.  We are just going
		// to silently go on
		TRACE("OpenProcessToken.\n"); 
	}
	else
	{

		/* Get the LUID for the shutdown privilege. */ 
		
		LookupPrivilegeValue(NULL, TEXT("SeShutdownPrivilege"),
			&tkp.Privileges[0].Luid); 
		
		tkp.PrivilegeCount = 1;  /* one privilege to set    */ 
		tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED; 
		
		/* Get the shutdown privilege for this process. */ 
		
		AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, 
			(PTOKEN_PRIVILEGES)NULL, 0); 
		
		/* Cannot test the return value of AdjustTokenPrivileges. */ 
		
		if (GetLastError() != ERROR_SUCCESS) 
			TRACE("AdjustTokenPrivileges.\n"); 
	}
	
	/* Shut down the system and force all applications to close. */ 

	if (0 == ExitWindowsEx(EWX_REBOOT, 0))
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
		//Display the manual reboot message...
		//
		Alert(IsWindow(GetWndParent()) ? GetWndParent() : NULL,
			  MB_OK | MB_ICONSTOP,
			  STR_MANUALREBOOT,
			  lpMsgBuf);

		LocalFree(lpMsgBuf);
	}

	return TRUE;
}


//****************************************************************************
// Procedure	DeleteRestartFiles
//
// Purpose		Deletes the temporary copies of files to be copied when
//				Windows is restarted and deletes the need to act upon them
//              from WININIT.INI.
//
// Parameters	none
//
// Returns		nothing
//
// History		 2/06/95	KenSh		Created
//				 3/26/97    v-richei    Rewrote for Windows 95 / NT
//
void DeleteRestartFiles()
{
	RESTARTINFO *pePtr = peRestartHead;
	RESTARTINFO *peTmp;

	//
	//Walk the restart list.  Remove the file from WININIT.INI (under Windows 95), remove the
	//physcial file from the source location, then free all of the memory...
	//
	while (pePtr)
	{
		WritePrivateProfileString("Rename", pePtr->szDestName, NULL, g_szWinInitFile);

		//
		//Under Win NT, this also serves to negate the effect of the previous MoveFileEx 
		//function.  Under both operating systems, of course, this is also polite cleanup
		//
		DeleteFile(pePtr->szSrcName);

		peTmp = pePtr->peNext;

		free(pePtr->szSrcName);
		free(pePtr->szDestName);
		free(pePtr);

		pePtr = peTmp;
	}

	peRestartHead = NULL;
}
