//
// uninstal.cpp
//
//		Generic-ish routines used for uninstalling an app
//
// History:
//
//		 6/29/95	KenSh		Created from registry.cpp
//		 8/13/95	KenSh		Write DLL reference-counts into the registry
//								as DWORDs instead of strings.
//		 8/30/95	KenSh		When deleting shortcuts, get start menu
//								directory from registry instead of hardcoding
//		 8/31/95	KenSh		Delete zero-count from the registry even if
//								the user doesn't want to delete the file.
//		1/1/97		a-melodh	assert when g_szRegUninstall is empty
//		03/15/97 a-dashoe		update timestamp
//

#include "stubpch.h"
#include "setup.h"
#include "progman.h"
#include "registry.h"
#include "uninstal.h"
#include "util.h"
#include "HotSetupRC.h"

using namespace NGLOBALS;
//----------------------------------------------------------------------------
// Procedure	PerformUninstallCommands
//
// Purpose		
//
// Parameters	hwnd			Window to use for error messages
//
// Returns		
//
// History		 ?/??/95	StephHer	Created
//
BOOL PerformUninstallCommands(void )
{
	//
	//We delete this keys that the engine created...
	//
	const UINT rguiRegKeys[] = {STR_REGKEY_VAL_GROUP,
								STR_REGKEY_VAL_PID,
								STR_REGKEY_VAL_PLAYERNAME,
								STR_REGKEY_VAL_LAUNCHED,
								STR_REGKEY_VAL_APPPATH,
								STR_REGKEY_VAL_VERSIONTYPE,
								STR_REGKEY_VAL_LANGID,
								-1};

	char	sz[MAX_PATH];
	DWORD	dwResult;
	LPSTR	pLastBackslash;
	int		iStringLen = 0;
	char	szAppRegRoot[MAX_PATH] = "";
	int		nIdx;
	HKEY	hkDel;

	// Remove the registry keys from the uninstall section of the registry
	// HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\windows\currentverion\uninstall\appname
	//
	ASSERT(lstrlen(GetRegUninstall()) > 0);
	lstrcpy(sz, GetRegUninstall());
	lstrcat(sz, "\\");

	iStringLen = EBULoadString(GetResourceInst(), STR_REGKEY_APP_REG_ROOT, szAppRegRoot, sizeof(szAppRegRoot) );
	ASSERT(iStringLen);

	while (pLastBackslash = pszGetLast5C( szAppRegRoot ))
		*pLastBackslash = ' ';	// turn the \'s into a space

	lstrcat(sz, szAppRegRoot);
	dwResult = RegDeleteKey(HKEY_LOCAL_MACHINE, sz);

	//
	//Now delete Programs->Microsoft Games Start Menu.  Will only delete if Microsoft
	//Games is empty...
	//
    EBULoadString(GetResourceInst(), STR_PROGMAN_MS_GAMES, sz, sizeof(sz));
    DeleteGroupFromStartMenu(sz, FALSE); 

	//
	//Remove the keys that the engine automatically created during install...
	//
	lstrcpy(sz, GetRegBase());

	char *pszValue = sz + lstrlen(sz) + sizeof(char);

	//
	//Remove each of the keys that we (the engine) created during install...
	//
	nIdx = 0;
	while (-1 != rguiRegKeys[nIdx])
	{
		EBULoadString(GetResourceInst(), rguiRegKeys[nIdx++], pszValue, sizeof(sz) - lstrlen(sz) - sizeof(char));

		if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, sz, NULL, KEY_READ | KEY_WRITE, &hkDel))
		{
			RegDeleteValue(hkDel, pszValue);
		}

		RegCloseKey(hkDel);
	}

	//
	//Remove the engine specific games registry key 
	//HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Microsoft Games\product\version
	dwResult = RegDeleteKey(HKEY_LOCAL_MACHINE, GetRegBase());
	//
	//Remove the engine specific games registry key 
	//HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Microsoft Games\product
	// NOTE: If the key is not empty, it will not be removed...
	//
	// NOTE: MyRegDeleteEmptyKey directly modifies the key passed in.
	lstrcpy(sz,GetRegBase());
	pLastBackslash = pszGetLast5C( sz );
	if ( pLastBackslash == NULL )
		pLastBackslash = sz;
	*pLastBackslash = 0;
	
	MyRegDeleteEmptyKey(HKEY_LOCAL_MACHINE, sz, FALSE);

	return TRUE;
}							


//----------------------------------------------------------------------------
// Procedure	MyReleaseSharedDll
//
// Purpose		Decrements the reference count of the given DLL in the
//				registry.  If the reference count goes down to zero, the
//				user is asked whether or not they want to delete the file.
//
// Parameters	
//				lpszValue		DLL to release
//				fDLLRegister	Whether or not to call DLLUnRegisterServer()
//
// Returns		nonzero if successful, zero if not.
//
// Comments		Currently the user is asked for every single file that
//				is uninstallable -- e.g. 5 times for the 5 WinG DLL's.
//
// History		 ?/??/95	StephHer	Created
//				 6/21/95	KenSh		Always uninstall the given file,
//										without checking its extension.
//				 8/13/95	KenSh		Write DWORD instead of string to registry
//				 8/31/95	KenSh		Delete the zero-count even if the user
//										doesn't want to delete the file.
//
BOOL MyReleaseSharedDll(LPCSTR lpszValue, BOOL fDLLRegister, BOOL fDeleteWhenZeroRefCount )
{
	char		szValue[260];
	HKEY		hkeyRoot;
	DWORD		dwRefCount;		// the reference count we write back
	DWORD		dwResult;		// result of reg functions
	DWORD		dwSize;
	DWORD		dwType;
	BOOL		bDeleteKey = FALSE;
	BOOL		bDeleteFile = FALSE;
	
	//
	// Open the SharedDlls Key
	//
	dwResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
							  GetRegSharedDLLs(),
							  NULL,
							  KEY_READ | KEY_WRITE,
							  &hkeyRoot);
	if (ERROR_SUCCESS != dwResult)
	{
		return FALSE;
	}
	
	//
	// Read this DLL's shared information from the Root Key
	//
	dwSize = sizeof(szValue);
	dwResult = RegQueryValueEx(hkeyRoot,
								 (LPSTR)lpszValue,
								 NULL,
								 (LPDWORD)&dwType,
								 (LPBYTE)szValue,
								 (LPDWORD)&dwSize);
	//
	// if the Key was located extract the number located there an increment
	// it one then write the results back to the szKey
	//
	if (dwResult != ERROR_SUCCESS)
	{
		dwRefCount = 0;
	}
	else
	{
		//
		//Parse the result differently depending on the data type
		//
		if( dwType == REG_DWORD )
			// Little-endian DWORD, use its value directly
		{
			dwRefCount = *(LPDWORD)szValue;
		}
		else if( dwType == REG_SZ )
			// String representation of number, parse it.
		{
			dwRefCount = (DWORD)(UINT)atoi(szValue);
		}
		else if( dwType == REG_DWORD_BIG_ENDIAN )
			// most significant byte first.
		{
			dwRefCount = (BYTE)szValue[3] +
			             ((BYTE)szValue[2] << 8) +
			             ((BYTE)szValue[1] << 16) +
			             ((BYTE)szValue[0] << 24);
		}
		else if( dwType == REG_BINARY )
			// ooh boy.  Assume binary has least-significant byte first
		{
			dwRefCount = (BYTE)szValue[0] +
						 ((BYTE)szValue[1] << 8) +
						 ((BYTE)szValue[2] << 16) +
						 ((BYTE)szValue[3] << 24);
		}
		else
		{
			ASSERT(FALSE);	// what case did we miss?
			dwRefCount = 0;
		}

		if( (long)dwRefCount > 0L )
		{
			// Decrement the reference count
			dwRefCount--;
		}
	}


	//
	// if there are zero references to the DLL we will ask the user to
	// let us remove it.
	//
	if( dwRefCount == 0 )
	{
		// We'd better check first if the file even exists. -ks 6/21/95
		if( DoesFileExist( lpszValue ) )
		{
			//Delete the refcount key too or not depending upon what
			//caller specified...
			bDeleteKey = fDeleteWhenZeroRefCount; //TRUE
			bDeleteFile = bDeleteKey; //BUGBUG:REVIEW:Should we prompt the user instead?
		}
		else
		{
			// The file ain't there, might as well get rid of the key too.
			bDeleteKey = TRUE;
		}
	}
	
	
	// Delete the file if we've gotten the go-ahead from the user
	if( bDeleteFile )
	{
		if (fDLLRegister)
			DLLRegister(lpszValue, DO_UNINSTALL);

		SetFileAttributes(lpszValue, FILE_ATTRIBUTE_NORMAL);		
		DeleteFile(lpszValue);
	}

	// Delete the reference count if desired.
	if( bDeleteKey )
	{
		dwResult = RegDeleteValue(hkeyRoot, (LPSTR)lpszValue);
	}
	else
	{
		//
		// Write the updated reference count to the registry
		//
		dwResult = RegSetValueEx(hkeyRoot,
								   lpszValue,
								   0,
								   REG_DWORD,
								   (LPBYTE)&dwRefCount,
								   sizeof(dwRefCount));
	}
	
	RegCloseKey(hkeyRoot);

	return TRUE;
}



//----------------------------------------------------------------------------
// Procedure	RemoveBaseKey
//
// Purpose		Checks if the given key is empty and removes it if so.
//				For example, given "SOFTWARE\\Microsoft\\ReferenceTitles"
//				this function will remove the "ReferenceTitles" key, if it
//				is empty.
//
// Parameters	lpsz			Name of the base key (as above)
//
// Returns		nonzero if successful, zero if not.
//
// History		 ?/??/95	StephHer	Created
//
BOOL RemoveBaseKey(HKEY hkeyBase, LPCSTR lpsz, BOOL fIgnoreRegValues = FALSE)
{
	HKEY			hkey = NULL;
	DWORD			dwResult = 0;
	char			szClass[255];
	DWORD           dwcchClass = 0;
	DWORD			dwcSubKeys = 0;
	DWORD			dwcchMaxSubkey = 0;
	DWORD			dwcchMaxClass = 0;
	DWORD			dwcValues = 0;
	DWORD			dwcchMaxValueName = 0;
	DWORD			dwcbMaxValueData = 0;
	DWORD			dwcbSD = 0;
	FILETIME		ftLastWrite;

	dwResult = RegOpenKeyEx(hkeyBase,
				   			  lpsz,
				   			  0,
				   			  KEY_READ | KEY_WRITE,
				   			  (PHKEY)&hkey);
	if (ERROR_SUCCESS != dwResult)
	{
		// if the key does not exist dwResult is NULL
		// return true to back up to the previous key
		if ( dwResult == NULL )
			return TRUE;
		else
			return FALSE;
	}
	
	dwResult = RegQueryInfoKey(hkey,
								 (LPSTR)szClass,
								 (LPDWORD)&dwcchClass,
								 NULL,
								 (LPDWORD)&dwcSubKeys,
								 (LPDWORD)&dwcchMaxSubkey,
								 (LPDWORD)&dwcchMaxClass,
								 (LPDWORD)&dwcValues,
								 (LPDWORD)&dwcchMaxValueName,
								 (LPDWORD)&dwcbMaxValueData,
								 (LPDWORD)&dwcbSD,
								 (FILETIME *) &ftLastWrite);
	RegCloseKey(hkey);
	
	if ((dwResult == ERROR_SUCCESS) && (dwcSubKeys == 0) && (dwcValues == 0 || fIgnoreRegValues))
	{
#ifdef _DEBUG
		{
			TRACE("Trying to Remove base key: %s ->", lpsz);
		}
#endif

		BOOL
			bFailed = RegDeleteKey(hkeyBase, (LPSTR)lpsz);

		#ifdef _DEBUG
		{
			if (!bFailed)
			{
				TRACE("(Removed).\n");
			}else{
				TRACE("(Failed).\n");
			}
		}
#endif

		
		return TRUE;
	}

	return FALSE;
}



