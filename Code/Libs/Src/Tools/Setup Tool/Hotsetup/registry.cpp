//
// registry.cpp
//
//    Functions to access the NT / Win95 Registry from our 16-bit app
//
// History:
//
//     2/06/95 KenSh    Created
//     8/01/95 AjayJ    MyWriteUniversalString: Added Win32_HKEY parameter,
//                      Use this parameter to specify which HKEY to write to

#include "stubpch.h"
#include "hotsetup.h"
#include "util.h"
#include "setup.h"
#include "HotSetupRC.h"
#include "registry.h"
#include <shellapi.h>
#include <dde.h>

#include "uninstal.h"	// for RemoveBaseKey

using namespace NGLOBALS;

extern LONG GetProgramFilesLocation(char *buf,DWORD cbuf);
extern LONG GetCommonFilesLocation(char *buf,DWORD cbuf);

//****************************************************************************
// Procedure   InitRegistry
//
// Purpose     Loads engine strings needed to initialize registry settings and directory defaults
//
// Parameters  none
//
// Returns     nonzero if successful, zero if not.
//
BOOL InitRegistry( boolean fZone )
{
	char
		szTemp[50] = "",
		szAppPathRegVal[50] = "",
		szProgramFilesDir[MAX_PATH] = "",
		szMSGamesDir[MAX_PATH] = "",
		szGameDir[MAX_PATH] = "",
		szDefaultDir[MAX_PATH] = "",
		szAppRegRoot[MAX_PATH] = "",
      szGroupFlag[64] = "",
      szDefaultFlag[2] = {'1','\0'};

	int
		iStringLen = 0;
	

	// load all the globals from the string table

	// our base key for all ms games
	iStringLen = EBULoadString(GetResourceInst(), ((fZone) ? STR_REGKEY_IGZGAMES_REG_ROOT : STR_REGKEY_MSGAMES_REG_ROOT), 
				GetRCGameRoot(), MAX_PATH); //sizeof(GetRCGameRoot()));
	ASSERT(iStringLen);

	// the regkey path to the uninstall section
	iStringLen = EBULoadString(GetResourceInst(), STR_REGKEY_UNINSTALL, GetRegUninstall(), MAX_PATH); //sizeof(GetRegUninstall()));
	ASSERT(iStringLen);

	// the regkey path to the shared dll's section
	iStringLen = EBULoadString(GetResourceInst(), STR_REGKEY_SHAREDDLL, GetRegSharedDLLs(), MAX_PATH); //sizeof(GetRegSharedDLLs()));
	ASSERT(iStringLen);

	// determine (and load) if there are any extra space values set for this app
	iStringLen = EBULoadString(GetResourceInst(), STR_EXTRASYSTEMSPACE, szTemp, sizeof(szTemp));
	if(iStringLen)
		SetExtraSystemBytes((DWORD)atol(szTemp));

	iStringLen = EBULoadString(GetResourceInst(), STR_EXTRAAPPSPACE, szTemp, sizeof(szTemp));
	if(iStringLen)
		SetExtraAppBytes((DWORD)atol(szTemp));
	
	// Load the application registry root key
	iStringLen = EBULoadString(GetResourceInst(), STR_REGKEY_APP_REG_ROOT, szAppRegRoot, sizeof(szAppRegRoot) );
	ASSERT(iStringLen);

	// build the global applications registry path (built from the MSGames root and the app root
	wsprintf(GetRegBase(), "%s\\%s", GetRCGameRoot(), szAppRegRoot);
	
	// load the setup application title (used in task bar and message dialogs)
	iStringLen = EBULoadString(GetResourceInst(), STR_SETUP_SETUPTITLE, GetSetupTitle(), 128 );
	ASSERT(iStringLen);

	// load the game name display title
	iStringLen = EBULoadString(GetResourceInst(), STR_SETUP_APPTITLE, GetAppTitle(), 128 );
	ASSERT(iStringLen);

	// now build up the default installation directory from the program files dir, msgames dir, and app dir name
	GetProgramFilesLocation(szProgramFilesDir, sizeof(szProgramFilesDir));
	iStringLen = EBULoadString(GetResourceInst(), 
      ((fZone) ? STR_DIRECTORY_IGZ_GAMES : STR_DIRECTORY_MS_GAMES), szMSGamesDir, sizeof(szMSGamesDir));
	ASSERT(iStringLen);

	iStringLen = EBULoadString(GetResourceInst(), STR_DIRECTORY_APP_ROOT, szGameDir, sizeof(szGameDir));
	ASSERT(iStringLen);

	wsprintf(szDefaultDir, "%s\\%s\\%s", szProgramFilesDir, szMSGamesDir, szGameDir);

	// now see if there is already a path in the registry from a previous run.  If so we will use it.
	// if not we will use the default we just built
	iStringLen = EBULoadString(GetResourceInst(), STR_REGKEY_VAL_APPPATH, szAppPathRegVal, sizeof(szAppPathRegVal) );
	MyGetPrivateProfileString(szAppPathRegVal, szDefaultDir, GetAppDir(), MAX_PATH);
   if(fZone && !lstrcmp(szDefaultDir,GetAppDir()))
   {
      char path[_MAX_PATH];
      char value[50];
      iStringLen = EBULoadString(GetResourceInst(), STR_REGKEY_IGZGAMES_REG_ROOTDEFAULT, path, sizeof(path) );
      iStringLen = EBULoadString(GetResourceInst(), STR_REGKEY_VAL_DEFAULTPATH, value, sizeof(value) );
      MyGetUniversalIniString(HKEY_LOCAL_MACHINE, path, value,szDefaultDir,GetAppDir(), MAX_PATH);
   }
   
   // Load the groups flag, if it's empty, then set groups to 0, otherwise get it.
   char tempGroup[64];
   iStringLen = EBULoadString(GetResourceInst(), STR_REGKEY_VAL_GROUP, szGroupFlag, sizeof(szGroupFlag) );
   if(MyGetPrivateProfileString(szGroupFlag, szDefaultFlag, tempGroup, 64) == 0)
   {
	   n_GroupList = addKeyboardTypeFlag(0);
   }
   else
   {
	   n_GroupList = addKeyboardTypeFlag(_atoi64(tempGroup));
   }

	// see if there is a joystick available to the game (as of right now)
	SetJoystick((joyGetNumDevs()>0) ? TRUE : FALSE);
   // load the PID if present.
   EBULoadString(GetResourceInst(), STR_REGKEY_VAL_PID, szTemp, sizeof(szTemp));
   MyGetPrivateProfileString( szTemp, "", GetPid(),_MAX_PATH);

   //
   //Load the PlayerName if present
   //
   EBULoadString(GetResourceInst(), STR_REGKEY_VAL_PLAYERNAME, szTemp, sizeof(szTemp));
   MyGetPrivateProfileString( szTemp, "", GetPlayerName(),_MAX_PATH);

	return TRUE;
}


//****************************************************************************
// Procedure   MyGetPrivateProfileString
//
// Purpose     Transparently gets a string from either the Registry (if we
//          are running in NT or win95) or from the ini file (if we are
//          running in Win 3.1).
//
// Parameters  lpszKey        entry in which we are interested
//          lpszDefault    default value if string doesn't exist
//          lpszBuf        where to copy the string
//          cbBuf       size of the buffer pointed to by lpszBuf
//
// Returns     number of characters copied, not including the terminating NULL
//
// History      2/07/95 KenSh    Created
//
LONG MyGetPrivateProfileString( LPSTR lpszKey, LPCSTR lpszDefault, LPSTR lpszBuf, int cbBuf )
{
      // Windows NT and Windows 95: access the registry using Generic Thunks

      HKEY hkResult;
      LONG lBufSize = (LONG)cbBuf;
      DWORD dwResult;

      dwResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                          (LPSTR)GetRegBase(),
                          NULL,
                          KEY_QUERY_VALUE,
                          (HKEY FAR *)&hkResult);
      if( ERROR_SUCCESS == dwResult )
      {
         dwResult = RegQueryValueEx(hkResult,
                               lpszKey,
                               NULL,
                               NULL,
                               (LPBYTE)lpszBuf,
                               (LPDWORD)&lBufSize);
         if( ERROR_SUCCESS != dwResult )
         {
            lstrcpyn( lpszBuf, lpszDefault, cbBuf );
            lBufSize = lstrlen( lpszDefault );
         }

         RegCloseKey(hkResult);

         return lBufSize;
      }
      else
      {
         lstrcpyn( lpszBuf, lpszDefault, cbBuf );
         return lstrlen( lpszDefault );
      }
}

//****************************************************************************
// Procedure   MyGetPrivateProfileString
//
// Purpose     Transparently gets a string from either the Registry (if we
//          are running in NT or win95) or from the ini file (if we are
//          running in Win 3.1).
//
// Parameters  lpszKey        entry in which we are interested
//          lpszDefault    default value if string doesn't exist
//          lpszBuf        where to copy the string
//          cbBuf       size of the buffer pointed to by lpszBuf
//
// Returns     number of characters copied, not including the terminating NULL
//
// History      2/07/95 KenSh    Created
//
LONG MyGetUniversalIniString( HKEY Win32Key, LPSTR lpszKey, LPCSTR szValue, LPCSTR lpszDefault, LPSTR lpszBuf, int cbBuf )
{
      // Windows NT and Windows 95: access the registry using Generic Thunks

      HKEY hkResult;
      LONG lBufSize = (LONG)cbBuf;
      DWORD dwResult;

      dwResult = RegOpenKeyEx(Win32Key,
                          (LPSTR)lpszKey,
                          NULL,
                          KEY_QUERY_VALUE,
                          (HKEY FAR *)&hkResult);
      if( ERROR_SUCCESS == dwResult )
      {
         dwResult = RegQueryValueEx(hkResult,
                               (char *)szValue,
                               NULL,
                               NULL,
                               (LPBYTE)lpszBuf,
                               (LPDWORD)&lBufSize);
         if( ERROR_SUCCESS != dwResult )
         {
            lstrcpyn( lpszBuf, lpszDefault, cbBuf );
            lBufSize = lstrlen( lpszDefault );
         }

         RegCloseKey(hkResult);

         return lBufSize;
      }
      else
      {
         lstrcpyn( lpszBuf, lpszDefault, cbBuf );
         return lstrlen( lpszDefault );
      }
}


//****************************************************************************
// Procedure   MyWritePrivateProfileString
//
// Purpose     Transparently writes a value either to the registry or to
//          an ini file, depending on what platform we're running on.
//
// Parameters  lpszKey        name of the key to write
//          lpszValue      value to store in the key
//
// Returns     nonzero if successful, zero if not.
//
// History      2/07/95 KenSh    Created
//
BOOL MyWritePrivateProfileString( LPCSTR lpszKey, LPCSTR lpszValue )
{
      DWORD dwResult;
      DWORD dwDisposition;
      HKEY hkResult;

      dwResult = RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                           (LPSTR)GetRegBase(),
                           (DWORD)0,
                           NULL,
                           REG_OPTION_NON_VOLATILE,
                           KEY_ALL_ACCESS,
                           NULL,
                           (HKEY FAR*)&hkResult,
                           (DWORD FAR*)&dwDisposition);
      if( ERROR_SUCCESS == dwResult )
      {
         dwResult = RegSetValueEx(hkResult,
							  (!lstrcmp(lpszKey,"NULL") ? NULL : lpszKey),
                              0,
                              REG_SZ,
                              (LPBYTE)lpszValue,
                              lstrlen(lpszValue)+1);
		 RegCloseKey(hkResult);
         return( ERROR_SUCCESS == dwResult );
      }
      else
      {
         return FALSE;
      }
}


//****************************************************************************
// Procedure   MyWriteUniversalRegType
//
// Purpose     Writes a registry value
//
// Parameters:
//          Win32_HKey     Registry HKEY to write to
//          lpszKey        Full name of key to create/modify, e.g.
//                        "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall"
//          lpszValue      String value to set in the registry, e.g.
//                        "D:\data\00setup\app\uninstal.exe"
//
// Returns     nonzero if successful, or zero on failure.
//
// History      ?/??/95 StephHer Created
//             01Aug95  AjayJ    Added Win32_HKey parameter so that user may specify which
//                               registry HKEY to put the value in
//
BOOL MyWriteUniversalRegType(HKEY Win32_HKey, LPCSTR lpszKey, LPSTR lpszValue, DWORD dwType)
{
	DWORD    dwResult;
	DWORD    dwDisposition;
	HKEY     hkey;
	char     sz[260];
	LPSTR    psz;
	int      iLen;
	DWORD    dwRegVal;
	DWORD    dwSize;
	BOOL     fNumber = FALSE;
	
  if(IsDBCS()) // Jun.05,1997 15:15 by yutaka. updated 6/2/98 by craigh to IsDBCS
  {
   lstrcpy(sz, lpszKey);
	LPSTR ptr = sz+lstrlen(sz);
	for (;;)
	{
		if (*ptr == '\\'){
			psz = ptr+1;
			*ptr = '\0';
			break;
		}
		if ( ptr == sz ){
			break;
		}
		ptr = CharPrev( sz, ptr );
	}
  }
  else
  {
	 iLen = lstrlen(lpszKey);
	 lstrcpy(sz, lpszKey);
	 while (iLen > 0)
	 {
		if (sz[iLen - 1] == '\\')
		{
			psz = (LPSTR)&(sz[iLen]);
			sz[iLen - 1] = '\0';
			break;
		}
		
		iLen--;
	 }
   }

	//
	//If we're just creating the key with a NULL value, clear out value string
	//
	if (0 == lstrcmp(lpszValue, "NULL"))
	{
		lstrcpy(lpszValue, "");
		ASSERT(REG_DWORD != dwType);
	}

	switch (dwType)
	{
	case REG_DWORD:
		fNumber = TRUE;
		dwRegVal = atol(lpszValue);
		dwSize = sizeof(DWORD);

		break;

	case REG_SZ:
	case REG_EXPAND_SZ:
		dwSize = lstrlen(lpszValue) + sizeof(TCHAR);
		break;

	default: //default to REG_BINARY
		dwSize = (DWORD) (-((LONG) dwType));
		dwType = REG_BINARY;

		ASSERT((LONG) dwSize > 0);

		//
		//If binary data passed in starts with a #, assume that we're writing a binary
		//number so convert it to a number first...
		//
		if ('#' == *lpszValue)
		{
			lpszValue++;
			dwRegVal = atol(lpszValue);
			fNumber = TRUE;

			ASSERT(sizeof(BYTE) == dwSize || sizeof(DWORD) == dwSize || sizeof(int) == dwSize);
		}

		break;
	}
	
	dwResult = RegCreateKeyEx(Win32_HKey,
								sz,
								0,
								NULL,
								REG_OPTION_NON_VOLATILE,
								KEY_ALL_ACCESS,
								NULL,
								&hkey,
								&dwDisposition);

	if (dwResult != ERROR_SUCCESS)
	{
		return FALSE;
	}

	//
	//If type being written is a string type, then dwSize should have been passed
	//in with size INCLUDING the NULL terminator at the end of the string...
	//
   dwResult = RegSetValueEx(hkey,
							(!lstrcmp(psz,"NULL") ? NULL : psz),
							0,
							dwType,
							fNumber ? (LPBYTE) &dwRegVal : (LPBYTE) lpszValue,
							dwSize);

	RegCloseKey(hkey);

	return (dwResult == ERROR_SUCCESS);
}

// prunes the key back as far as it can removing all empty branches
LONG MyRegDeleteEmptyKey(HKEY  hkey, LPSTR  szKey, BOOL  fIgnoreRegValues)
{
	BOOL
		bRemovingKey = TRUE;
	char tempkey[_MAX_PATH];
	lstrcpy(tempkey,szKey);

	while (bRemovingKey)
	{
		// MEMO : Jul.15,1997 21:07 by yutaka.
		// We need to strip before RemoveBaseKey() ?
		ASSERT( *CharPrev(tempkey,tempkey+lstrlen(tempkey))!='\\' );

		// Only removes base key if there are no subkeys
		bRemovingKey = RemoveBaseKey(hkey, tempkey, fIgnoreRegValues);
		
		// now backup for the next step
		LPSTR pch;
		pch = pszGetLast5C( tempkey );
		if ( pch != NULL )
		{
			*pch = '\0';
		}

		if (!lstrlen(tempkey))
			bRemovingKey = FALSE;
	}

	return TRUE;
}

// finds subkeys of the current key and deletes the current key if there 
// are none. Recurses for every one it finds
LONG MyRegDeleteSubKeys(
			HKEY  hkey,  // handle of toplevel key
            LPSTR  szKey   // path of address of key to prune
                 )
{
	BOOL	bRemovingKey = TRUE;
	char	tempkey[_MAX_PATH];
	HKEY	hkLocal;
	DWORD	index = 0;
	DWORD	keylen = _MAX_PATH;
	FILETIME	timekey;

	if ( RegOpenKeyEx (hkey, szKey, NULL, KEY_READ, &hkLocal) != ERROR_SUCCESS )
		return FALSE;
	
	while (ERROR_NO_MORE_ITEMS != RegEnumKeyEx ( hkLocal, index, tempkey, &keylen, NULL, NULL, NULL, &timekey))
	{
		MyRegDeleteSubKeys ( hkLocal, szKey );
		RegDeleteKey ( hkLocal, szKey );

		index++;
		keylen = _MAX_PATH;
	}

	RegCloseKey ( hkLocal );
	return TRUE;
}





