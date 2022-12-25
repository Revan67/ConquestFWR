//--------------------------------------------------------------------------//
//                                                                          //
//                             UserDefaults.cpp                             //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Author: Tmauer $

    $Header: /Conquest/App/Src/UserDefaults.cpp 61    6/09/01 11:01 Tmauer $
*/			    
//---------------------------------------------------------------------------
/*
        Stores user preferences in the registry
*/
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

#include "pch.h"
#include <globals.h>

#include "Resource.h"
#include "UserDefaults.h"
#include "CQTrace.h"

#include <TComponent.h>
#include <HeapObj.h>
#include <Viewer.h>
#include <Document.h>
#include <TSmartPointer.h>
#include <WindowManager.h>
#include <dsetup.h>

#include <stdio.h>
#include <commdlg.h>               // Common dialogs
#include <shlwapi.h>
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

#define NAME_REG_KEY   "CQPlayerName"

static char szDefaultsKey[] = "User Defaults";

#ifdef _DEMO_
static char szProductName[] = "Conquest: Frontier Wars Demo";
static char szRegKeyPath[] = "Software\\Fever Pitch\\Conquest: Frontier Wars Demo\\0.90";
static char szVersion[] = "0.90";
#else
static char szProductName[] = "Conquest: Frontier Wars";
static char szRegKeyPath[] = "Software\\Fever Pitch\\Conquest: Frontier Wars\\1.00";
static char szVersion[] = "1.00";
#endif // !_DEMO_


#define DEFAULTS_VERSION 11
#define MAX_MRU	4		// remember last four files loaded

bool DoEulaWin(char * eulaFile);


struct DACOM_NO_VTABLE UserDefaults : public IUserDefaults
{
	USER_DEFAULTS userDefaults;
	U32 viewerCount;
	HCURSOR hOldCursor;	// saved cursor handle

	COMPTR<IFileSystem> fileSystem;
	
	// 
	// interface mapping
	//

	BEGIN_DACOM_MAP_INBOUND(UserDefaults)
	DACOM_INTERFACE_ENTRY(IUserDefaults)
	END_DACOM_MAP()

	UserDefaults (void)
	{
		init();
	}

	~UserDefaults (void)
	{
		StoreDefaults();
		pUserDefaults = 0;
	}

    void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}

	/* IUserDefaults methods */

	DEFMETHOD_(BOOL32,LoadDefaults) (void);

	DEFMETHOD_(BOOL32,StoreDefaults) (void);

	DEFMETHOD_(BOOL32,DeletePlayerDefaults) (const char * playerName);

	DEFMETHOD_(U32,GetDataFromRegistry) (const char *szKey, void *buffer, U32 dwBufferSize);
	
	DEFMETHOD_(BOOL32,SetDataInRegistry) (const char *szKey, void *buffer, U32 dwBufferSize);

	DEFMETHOD_(BOOL32,GetStringFromRegistry) (char *szDirName, U32 id);
	
	DEFMETHOD_(BOOL32,SetStringInRegistry) (const char *szDirName, U32 id);

	DEFMETHOD_(BOOL32,GetInputFilename) (char *dst, U32 id);

	DEFMETHOD_(BOOL32,GetOutputFilename) (char *dst, U32 id);

	DEFMETHOD_(BOOL32,GetUserData) (const C8 * typeName, const C8 * instanceName, void * data, U32 dataSize);

	BOOL32 __stdcall GetScriptData (void * symbol, const C8 * instanceName, void * data, U32 dataSize);

	DEFMETHOD_(BOOL32,SetNameInMRU) (const char *szFileName, U32 id, U32 pos=0);

	DEFMETHOD_(BOOL32,GetNameInMRU) (char *szFileName, U32 id, U32 pos=0);

	DEFMETHOD_(S32,FindNameInMRU) (const char *szFileName, U32 id);

	DEFMETHOD_(BOOL32,RemoveNameFromMRU) (const char *szFileName, U32 id);

	DEFMETHOD_(BOOL32,RemoveNameFromMRU) (U32 id, U32 pos);

	DEFMETHOD_(BOOL32,InsertNameIntoMRU) (const char *szFileName, U32 id);

	U32 __stdcall GetStringFromRegistry (const char * szKey, char *buffer, U32 dwBufferSize);
	
	U32 __stdcall SetStringInRegistry (const char *szKey, const char *buffer);

	U32 __stdcall GetInstallStringFromRegistry (const char * szKey, char *buffer, U32 dwBufferSize);

	void __stdcall ResetToDefaultSettings (void)
	{
		init();
	}

	BOOL32 __stdcall UbiEula ();

/* UserDefaults methods */

	BOOL32 DeleteMRUEntry (U32 id, U32 pos);

	void setWindowDefaults (void);

	void init (void);

#ifndef FINAL_RELEASE
#ifndef _DEMO_
	BOOL32 storeDPlayInfo (void);
#endif
#endif

	BOOL32 createKey (HKEY & hKey, const HKEY rootKey=HKEY_CURRENT_USER);

	BOOL32 openKey (HKEY & hKey, const HKEY rootKey=HKEY_CURRENT_USER);

	bool setFileSystem (void);

	bool loadPlayerDefaults (void);

	// removed because requires explorer 4.0
//	BOOL32 removeOldKeys (void);
};

UDEXTERN struct USER_DEFAULTS * IUserDefaults::pUserDefaults;

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//
void UserDefaults::init (void)
{
	// initialize the defaults structure

	memset(&userDefaults, 0, sizeof(userDefaults));
//	userDefaults.dwSize = sizeof(userDefaults);
	userDefaults.dwVersion = DEFAULTS_VERSION;
	userDefaults.showState = SW_SHOW;
	userDefaults.iViewerHeight = 100;
	userDefaults.iViewerWidth = 100;
	userDefaults.bRightClickOption = userDefaults.bSectormapRotates = true;
	userDefaults.scrollRate = 1.0;
	userDefaults.bHardwareRender = 1;
	userDefaults.soundState.init();
	userDefaults.bHardwareCursor = 1;
	userDefaults.maxProjectiles = 20;

	pUserDefaults = & userDefaults;
}
#ifdef _DEMO_
//----------------------------------------------------------------------------
//
BOOL32 UserDefaults::createKey (HKEY & hKey, const HKEY rootKey)
{
	BOOL32 result = 0;
	HKEY hkey1, hkey2, hkey3, hkey4;
	U32 dwDisposition;
	char *version = szVersion;

	hKey = 0;

	if (RegOpenKeyEx(rootKey, "Software", 0, KEY_ALL_ACCESS, &hkey1) != ERROR_SUCCESS)
		goto Done0;

	// 
	// create new keys
	//

	if (RegCreateKeyEx(hkey1, "Fever Pitch", 0, NULL, REG_OPTION_NON_VOLATILE, 
		KEY_ALL_ACCESS, 0, &hkey2, &dwDisposition) != ERROR_SUCCESS)
	{
		goto Done1;
	}
	
	if (RegCreateKeyEx(hkey2, szProductName, 0, NULL, REG_OPTION_NON_VOLATILE,
		KEY_ALL_ACCESS, 0, &hkey3, &dwDisposition) != ERROR_SUCCESS)
	{
		goto Done2;
	}

#ifdef _DEBUG
	if (rootKey != HKEY_LOCAL_MACHINE)
		version = "DEBUG";
#endif

	if (RegCreateKeyEx(hkey3, version, 0, NULL, REG_OPTION_NON_VOLATILE,
		KEY_ALL_ACCESS, 0, &hkey4, &dwDisposition) != ERROR_SUCCESS)
	{
		goto Done3;
	}

	result = 1;
	hKey = hkey4;

Done3:
	RegCloseKey(hkey3);
Done2:
	RegCloseKey(hkey2);
Done1:
	RegCloseKey(hkey1);
Done0:
	return result;
}
//----------------------------------------------------------------------------
//
BOOL32 UserDefaults::openKey (HKEY & hKey, const HKEY rootKey)
{
	HKEY hkey1, hkey2, hkey3, hkey4;
	BOOL32 result = 0;
	char *version = szVersion;

	if (RegOpenKeyEx(rootKey, "Software", 0, KEY_READ, &hkey1) != ERROR_SUCCESS)
		goto Done0;

	if (RegOpenKeyEx(hkey1, "Fever Pitch", 0, KEY_READ, &hkey2) != ERROR_SUCCESS)
		goto Done1;
	
	if (RegOpenKeyEx(hkey2, szProductName, 0, KEY_READ, &hkey3) != ERROR_SUCCESS)
		goto Done2;

#ifdef _DEBUG
	if (rootKey != HKEY_LOCAL_MACHINE)
		version = "DEBUG";
#endif

	if (RegOpenKeyEx(hkey3, version, 0, KEY_READ, &hkey4) != ERROR_SUCCESS)
		goto Done3;

	result = 1;
	hKey = hkey4;

Done3:
	RegCloseKey(hkey3);
Done2:
	RegCloseKey(hkey2);
Done1:
	RegCloseKey(hkey1);
Done0:
	return result;
}
#else  // !_DEMO_
//----------------------------------------------------------------------------
//
BOOL32 UserDefaults::createKey (HKEY & hKey, const HKEY rootKey)
{
	BOOL32 result = 0;
	HKEY hkey1, hkey2, hkey3, hkey4;
	U32 dwDisposition;
	char *version = szVersion;

	hKey = 0;

	if (RegOpenKeyEx(rootKey, "Software", 0, KEY_ALL_ACCESS, &hkey1) != ERROR_SUCCESS)
		goto Done0;

	// 
	// create new keys
	//

	if (RegCreateKeyEx(hkey1, "Fever Pitch", 0, NULL, REG_OPTION_NON_VOLATILE, 
		KEY_ALL_ACCESS, 0, &hkey2, &dwDisposition) != ERROR_SUCCESS)
	{
		goto Done1;
	}

	if (RegCreateKeyEx(hkey2, szProductName, 0, NULL, REG_OPTION_NON_VOLATILE,
		KEY_ALL_ACCESS, 0, &hkey3, &dwDisposition) != ERROR_SUCCESS)
	{
		goto Done2;
	}

#ifdef _DEBUG
	if (rootKey != HKEY_LOCAL_MACHINE)
		version = "DEBUG";
#endif

	if (RegCreateKeyEx(hkey3, version, 0, NULL, REG_OPTION_NON_VOLATILE,
		KEY_ALL_ACCESS, 0, &hkey4, &dwDisposition) != ERROR_SUCCESS)
	{
		goto Done3;
	}

	result = 1;
	hKey = hkey4;

Done3:
	RegCloseKey(hkey3);
Done2:
	RegCloseKey(hkey2);
Done1:
	RegCloseKey(hkey1);
Done0:
	return result;
}
//----------------------------------------------------------------------------
//
BOOL32 UserDefaults::openKey (HKEY & hKey, const HKEY rootKey)
{
	HKEY hkey1, hkey2, hkey3, hkey4;
	BOOL32 result = 0;
	char *version = szVersion;

	if (RegOpenKeyEx(rootKey, "Software", 0, KEY_READ, &hkey1) != ERROR_SUCCESS)
		goto Done0;

	if (RegOpenKeyEx(hkey1, "Fever Pitch", 0, KEY_READ, &hkey2) != ERROR_SUCCESS)
		goto Done1;
		
	if (RegOpenKeyEx(hkey2, szProductName, 0, KEY_READ, &hkey3) != ERROR_SUCCESS)
		goto Done2;

#ifdef _DEBUG
	if (rootKey != HKEY_LOCAL_MACHINE)
		version = "DEBUG";
#endif

	if (RegOpenKeyEx(hkey3, version, 0, KEY_READ, &hkey4) != ERROR_SUCCESS)
		goto Done3;

	result = 1;
	hKey = hkey4;

Done3:
	RegCloseKey(hkey3);
Done2:
	RegCloseKey(hkey2);
Done1:
	RegCloseKey(hkey1);
Done0:
	return result;
}
#endif // !_DEMO_
//----------------------------------------------------------------------------
//
BOOL32 UserDefaults::LoadDefaults (void)
{
	BOOL32 result = 0;
	HKEY hkey;
	U32 cbData, dwType;

	if (openKey(hkey))
	{

		{
			char playerName[64];
			if (GetStringFromRegistry(NAME_REG_KEY, playerName, sizeof(playerName)))
			{
				char buffer[256];
				sprintf(buffer, "%s-%s", szDefaultsKey, playerName);
				cbData = sizeof(userDefaults);
				if (RegQueryValueEx(hkey, buffer, 0, &dwType, (U8 *)&userDefaults, &cbData) == ERROR_SUCCESS)
				{
					if (dwType == REG_BINARY && cbData == sizeof(USER_DEFAULTS) && userDefaults.dwVersion == DEFAULTS_VERSION)
					{
						result = 1;
						RegCloseKey(hkey);
						return result;
					}
					DeletePlayerDefaults(playerName);
				}
				else
				{
					// perhaps we are making a new guy, use default values
					result = 1;
					RegCloseKey(hkey);
					bool oldHardwareState = userDefaults.bHardwareCursor;
					init();
					userDefaults.bHardwareCursor = oldHardwareState;
					return result;
				}
			}
		}

		cbData = sizeof(userDefaults);

		if (RegQueryValueEx(hkey, szDefaultsKey, 0, &dwType, (U8 *)&userDefaults, &cbData) == ERROR_SUCCESS)
		{
			if (dwType != REG_BINARY)
			{
				CQERROR1("%s", _localLoadString(IDS_BAD_REGISTRY_VALUE));
				init();
			}
			else
			if (cbData != sizeof(USER_DEFAULTS) || userDefaults.dwVersion != DEFAULTS_VERSION)
			{
				init();
			}
		}

		result = 1;
		RegCloseKey(hkey);
	}

	return result;
}
#ifndef FINAL_RELEASE
#ifndef _DEMO_
//----------------------------------------------------------------------------
//
BOOL32 UserDefaults::storeDPlayInfo (void)
{
	HKEY hkey;
	BOOL32 result = 0;

	if (createKey(hkey, HKEY_LOCAL_MACHINE))
	{
		char dir[MAX_PATH];
		char path[MAX_PATH];
		char * tmp;
		DIRECTXREGISTERAPP directXRegister;

		::GetCurrentDirectory(sizeof(dir), dir);
		RegSetValueEx(hkey, "InstallPath", 0, REG_SZ, (U8 *)dir, strlen(dir)+1);
		RegSetValueEx(hkey, "Version", 0, REG_SZ, (U8 *)szVersion, 4);

		::GetModuleFileName(0, path, sizeof(path));
		if ((tmp = strrchr(path, '\\')) != 0)
			*tmp++ = 0;

		memset(&directXRegister, 0, sizeof(directXRegister));
		directXRegister.dwSize = sizeof(directXRegister);
		directXRegister.lpszApplicationName = "Conquest Frontier Wars";
		directXRegister.lpGUID = const_cast<GUID *>(&APPGUID_CONQUEST);
		directXRegister.lpszFilename = tmp;
		directXRegister.lpszCommandLine = "/lobby";
		directXRegister.lpszPath = path;
		directXRegister.lpszCurrentDirectory = dir;
		DirectXRegisterApplication(0, &directXRegister);

		result = 1;
		RegCloseKey(hkey);
	}

	// removed because of explorer 4.0 dependency
//	removeOldKeys();

	return result;
}
#endif // !_DEMO_
#endif // !FINAL_RELEASE
//----------------------------------------------------------------------------
//
BOOL32 UserDefaults::StoreDefaults (void)
{
	HKEY hkey;
	BOOL32 result = 0;

	if (createKey(hkey))
	{
		userDefaults.dwSize = sizeof(userDefaults);
		if (CQFLAGS.bFullScreen==0)
			setWindowDefaults();

		RegSetValueEx(hkey, szDefaultsKey, 0, REG_BINARY, (U8 *)&userDefaults, sizeof(userDefaults));
		{
			char playerName[64];
			if (GetStringFromRegistry(NAME_REG_KEY, playerName, sizeof(playerName)))
			{
				char buffer[256];
				sprintf(buffer, "%s-%s", szDefaultsKey, playerName);
				RegSetValueEx(hkey, buffer, 0, REG_BINARY, (U8 *)&userDefaults, sizeof(userDefaults));
			}
		}
		RegCloseKey(hkey);

#ifndef FINAL_RELEASE
#ifndef _DEMO_
		if (CQFLAGS.bStoreDPlayInfo)
			storeDPlayInfo();
#endif // !_DEMO_
#endif // !FINAL_RELEASE
		result = 1;
	}

	return result;
}
//----------------------------------------------------------------------------
//
BOOL32 UserDefaults::DeletePlayerDefaults (const char * playerName)
{
	BOOL32 result = 0;
	HKEY hkey;

	if (createKey(hkey))
	{
		char buffer[256];
		sprintf(buffer, "%s-%s", szDefaultsKey, playerName);
		if (RegDeleteValue(hkey, buffer) == ERROR_SUCCESS)
			result = 1;
		RegCloseKey(hkey);
	}
	return result;
}
//----------------------------------------------------------------------------
//
U32 UserDefaults::GetDataFromRegistry (const char *szKey, void *buffer, U32 dwBufferSize)
{
	HKEY hkey;
	U32 cbData=0, dwType;

	if (openKey(hkey))
	{
		cbData = dwBufferSize;
		switch (RegQueryValueEx(hkey, szKey, 0, &dwType, (U8 *)buffer, &cbData))
		{
		case ERROR_MORE_DATA:
			cbData = 0xFFFFFFFF;
			break;

		case ERROR_SUCCESS:
			if (dwType != REG_BINARY)
			{
				CQERROR1("%s", _localLoadString(IDS_BAD_REGISTRY_VALUE));
				cbData = 0;
			}
			break;

		default:
			cbData = 0;
			break;
		}

		RegCloseKey(hkey);
	}

	return cbData;
}
//----------------------------------------------------------------------------
//
BOOL32 UserDefaults::SetDataInRegistry (const char *szKey, void *buffer, U32 dwBufferSize)
{
	HKEY hkey;
	BOOL32 result = 0;

	if (createKey(hkey))
	{
		userDefaults.dwSize = sizeof(userDefaults);
		if (CQFLAGS.bFullScreen==0)
			setWindowDefaults();

		RegSetValueEx(hkey, szKey, 0, REG_BINARY, (U8 *)buffer, dwBufferSize);
		result = 1;
		RegCloseKey(hkey);
	}
	return result;
}
//----------------------------------------------------------------------------
//
BOOL32 UserDefaults::GetStringFromRegistry (char *szDirName, U32 id)
{
	HKEY hkey;
	char buffer[16];
	unsigned long cbData, dwType;
	BOOL result = 0;

	*szDirName = 0;

	if (openKey(hkey))
	{
		wsprintf(buffer, "%d", id);
		cbData = 256;
		if (RegQueryValueEx(hkey, buffer, 0, &dwType, (U8 *)szDirName, &cbData) == ERROR_SUCCESS)
		{
			if (dwType != REG_SZ)
			{
				CQERROR1("%s", _localLoadString(IDS_BAD_REGISTRY_VALUE));
			}
			else
				result = 1;
		}
		RegCloseKey(hkey);
	}

	return result;
}
//----------------------------------------------------------------------------
//
BOOL32 UserDefaults::SetStringInRegistry (const char *szDirName, U32 id)
{
	HKEY hkey;
	char buffer[16];
	BOOL result = 0;

	if (createKey(hkey))
	{
		wsprintf(buffer, "%d", id);
		RegSetValueEx(hkey, buffer, 0, REG_SZ, (U8 *)szDirName, strlen(szDirName)+1);
		result = 1;
		RegCloseKey(hkey);
	}

	return result;
}
//--------------------------------------------------------------------------//
//
U32 UserDefaults::GetStringFromRegistry (const char * szKey, char *buffer, U32 dwBufferSize)
{
	HKEY hkey;
	U32 cbData=0, dwType;

	buffer[dwBufferSize-1] = 0;
	buffer[0] = 0;
	
	if (openKey(hkey))
	{
		cbData = dwBufferSize;
		switch (RegQueryValueEx(hkey, szKey, 0, &dwType, (U8 *)buffer, &cbData))
		{
		case ERROR_MORE_DATA:
			cbData = 0xFFFFFFFF;
			break;

		case ERROR_SUCCESS:
			if (dwType != REG_SZ)
			{
				CQERROR1("%s", _localLoadString(IDS_BAD_REGISTRY_VALUE));
				cbData = 0;
			}
			break;

		default:
			cbData = 0;
			break;
		}

		RegCloseKey(hkey);
	}

	return cbData;
}
//--------------------------------------------------------------------------//
//
U32 UserDefaults::GetInstallStringFromRegistry (const char * szKey, char *buffer, U32 dwBufferSize)
{
	HKEY hkey;
	U32 cbData=0, dwType;

	buffer[dwBufferSize-1] = 0;
	buffer[0] = 0;

	if (openKey(hkey, HKEY_LOCAL_MACHINE))
	{
		cbData = dwBufferSize;
		switch (RegQueryValueEx(hkey, szKey, 0, &dwType, (U8 *)buffer, &cbData))
		{
		case ERROR_MORE_DATA:
			cbData = 0xFFFFFFFF;
			break;

		case ERROR_SUCCESS:
			if (dwType != REG_SZ)
			{
				CQERROR1("%s", _localLoadString(IDS_BAD_REGISTRY_VALUE));
				cbData = 0;
			}
			break;

		default:
			cbData = 0;
			break;
		}

		RegCloseKey(hkey);
	}

	return cbData;
}
//--------------------------------------------------------------------------//
//
U32 UserDefaults::SetStringInRegistry (const char *szKey, const char *szString)
{
	HKEY hkey;
	BOOL32 result = 0;

	if (createKey(hkey))
	{
		userDefaults.dwSize = sizeof(userDefaults);
		if (CQFLAGS.bFullScreen==0)
			setWindowDefaults();

		if (szString)
			RegSetValueEx(hkey, szKey, 0, REG_SZ, (U8 *)szString, strlen(szString)+1);
		else
			RegDeleteValue(hkey, szKey);
		result = 1;
		RegCloseKey(hkey);
	}
	return result;
}
//--------------------------------------------------------------------------//
// return TRUE if got a name
//
BOOL32 UserDefaults::GetInputFilename (char *dst, U32 id)
{
    OPENFILENAME ofn = {0}; // common dialog box structure
    char szDirName[256];    // directory string
    char szFileTitle[256];  // file-title string
    char szFilter[256];     // filter string
    char chReplace;         // strparator for szFilter
    int i, cbString;        // integer count variables
    char szSavedDirName[256];    // directory string

    // Retrieve the current directory name and store it in szDirName.
    GetCurrentDirectory(sizeof(szSavedDirName), szSavedDirName);

	*dst = 0;
	if (GetStringFromRegistry(szDirName, id) == 0)
		strcpy(szDirName, szSavedDirName);

    // Load the filter string from the resource file.

    cbString = LoadString(hResource, id, szFilter, sizeof(szFilter));

    // Add a terminating null character to the filter string.

    chReplace = szFilter[cbString - 1];
    for (i = 0; szFilter[i] != '\0'; i++)
    {
        if (szFilter[i] == chReplace)
		{
            szFilter[i] = '\0';
		    ofn.nFilterIndex++;
		}
    }
	
    // Set the members of the OPENFILENAME structure.

    ofn.lStructSize = sizeof(OPENFILENAME);
    ofn.hwndOwner = hMainWindow;
    ofn.lpstrFilter = szFilter;
    ofn.nFilterIndex = 1;
    ofn.lpstrFile = dst;
    ofn.nMaxFile = 79;
    ofn.lpstrFileTitle = szFileTitle;
    ofn.nMaxFileTitle = sizeof(szFileTitle);
    ofn.lpstrInitialDir = szDirName;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

    // Display the Open dialog box.

    if (GetOpenFileName(&ofn))
	{
		char *tmp;

		strcpy(szDirName, dst);
		if ((tmp = strrchr(szDirName, '\\')) != 0)
		{
			*tmp = 0;
			SetStringInRegistry(szDirName, id);
			RemoveNameFromMRU(dst, id);
			InsertNameIntoMRU(dst, id);
		}

	    SetCurrentDirectory(szSavedDirName);
		return 1;
	}
    SetCurrentDirectory(szSavedDirName);
	return 0;
}
//--------------------------------------------------------------------------//
// return TRUE if got a name
//
BOOL32 UserDefaults::GetOutputFilename (char *dst, U32 id)
{
    OPENFILENAME ofn = {0}; // common dialog box structure
    char szDirName[256];    // directory string
    char szFileTitle[256];  // file-title string
    char szFilter[256];     // filter string
    char chReplace;         // strparator for szFilter
    int i, cbString;        // integer count variables
    char szSavedDirName[256];    // directory string


    // Retrieve the current directory name and store it in szDirName.
    GetCurrentDirectory(sizeof(szSavedDirName), szSavedDirName);

	if (GetStringFromRegistry(szDirName, id) == 0)
		strcpy(szDirName, szSavedDirName);

    // Load the filter string from the resource file.

    cbString = LoadString(hResource, id, szFilter, sizeof(szFilter));

    // Add a terminating null character to the filter string.

    chReplace = szFilter[cbString - 1];
    for (i = 0; szFilter[i] != '\0'; i++)
    {
        if (szFilter[i] == chReplace)
		{
            szFilter[i] = '\0';
		    ofn.nFilterIndex++;
		}
    }
	
    // Set the members of the OPENFILENAME structure.

    ofn.lStructSize = sizeof(OPENFILENAME);
    ofn.hwndOwner = hMainWindow;
    ofn.lpstrFilter = szFilter;
    ofn.nFilterIndex = 1;
    ofn.lpstrFile = dst;
    ofn.nMaxFile = 79;
    ofn.lpstrFileTitle = szFileTitle;
    ofn.nMaxFileTitle = sizeof(szFileTitle);
    ofn.lpstrInitialDir = szDirName;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT |
    			OFN_HIDEREADONLY;

    // Display the Open dialog box.

    if (GetSaveFileName(&ofn))
	{
		char *tmp;

		strcpy(szDirName, dst);
		if ((tmp = strrchr(szDirName, '\\')) != 0)
		{
			*tmp = 0;
			SetStringInRegistry(szDirName, id);
			RemoveNameFromMRU(dst, id);
			InsertNameIntoMRU(dst, id);
		}

	    SetCurrentDirectory(szSavedDirName);
		return 1;
	}
    SetCurrentDirectory(szSavedDirName);
	return 0;
}
//----------------------------------------------------------------------------
//
BOOL32 UserDefaults::GetNameInMRU (char *szFileName, U32 id, U32 pos)
{
	BOOL32 result=0;
	HKEY hkey;
	char buffer[16];
	unsigned long cbData, dwType;

	*szFileName = 0;

	if (pos < MAX_MRU && openKey(hkey))
	{
		wsprintf(buffer, "%d-MRU%d", id, pos);
		cbData = 256;
		if (RegQueryValueEx(hkey, buffer, 0, &dwType, (U8 *)szFileName, &cbData) == ERROR_SUCCESS)
		{
			if (dwType != REG_SZ)
			{
				CQERROR1("%s", _localLoadString(IDS_BAD_REGISTRY_VALUE));
			}
			else
				result = 1;
		}
		RegCloseKey(hkey);
	}

	return result;
}
//----------------------------------------------------------------------------
//
BOOL32 UserDefaults::SetNameInMRU (const char *szFileName, U32 id, U32 pos)
{
	HKEY hkey;
	char buffer[16];
	BOOL32 result = 0;

	if (pos < MAX_MRU && createKey(hkey))
	{
		wsprintf(buffer, "%d-MRU%d", id, pos);
		RegSetValueEx(hkey, buffer, 0, REG_SZ, (U8 *)szFileName, strlen(szFileName)+1);
		result = 1;
		RegCloseKey(hkey);
	}

	return result;
}
//----------------------------------------------------------------------------
//
S32 UserDefaults::FindNameInMRU (const char *szFileName, U32 id)
{
	char buffer[MAX_PATH+4];
	S32 i;

	for (i = 0; i < MAX_MRU; i++)
	{
		if (GetNameInMRU(buffer, id, i))
		{
			if (strcmp(buffer, szFileName) == 0)
				return i;
		}
	}

	return -1;
}
//----------------------------------------------------------------------------
//
BOOL32 UserDefaults::DeleteMRUEntry (U32 id, U32 pos)
{
	HKEY hkey;
	char buffer[16];
	BOOL32 result = 0;

	if (pos < MAX_MRU && createKey(hkey))
	{
		wsprintf(buffer, "%d-MRU%d", id, pos);
		if (RegDeleteValue(hkey, buffer) == ERROR_SUCCESS)
			result = 1;
		RegCloseKey(hkey);
	}

	return result;
}
//----------------------------------------------------------------------------
//
BOOL32 UserDefaults::RemoveNameFromMRU (U32 id, U32 pos)
{
	BOOL32 result = 0;
	char buffer[MAX_PATH+4];

	if (pos >= MAX_MRU)
		goto Done;

	while (pos+1 < MAX_MRU)
	{
		if (GetNameInMRU(buffer, id, pos+1))
		{
			if (SetNameInMRU(buffer, id, pos) == 0)
				goto Done;
		}
		else
			break;

		pos++;
	}

	result = DeleteMRUEntry(id, pos);

Done:
	return result;
}
//----------------------------------------------------------------------------
//
BOOL32 UserDefaults::RemoveNameFromMRU (const char *szFileName, U32 id)
{
	BOOL32 result=0;
	S32 index;

	while ((index = FindNameInMRU(szFileName, id)) >= 0)
	{
		if (RemoveNameFromMRU(id, index) == 0)
			break;
	}

	return result;
}
//----------------------------------------------------------------------------
//
BOOL32 UserDefaults::InsertNameIntoMRU (const char *szFileName, U32 id)
{
	S32 i;
	char buffer[MAX_PATH+4];
	BOOL32 result=0;

	// 
	// move everyone else down in the list
	//

	for (i = MAX_MRU-2; i >= 0; i--)
	{
		if (GetNameInMRU(buffer, id, i))
		{
			if (SetNameInMRU(buffer, id, i+1) == 0)
				goto Done;
		}
	}

	//
	// write new entry at the top of the list
	//

	result = SetNameInMRU(szFileName, id, 0);

Done:
	return result;
}
//----------------------------------------------------------------------------
//
void UserDefaults::setWindowDefaults (void)
{
	WINDOWPLACEMENT winplace;

	winplace.length = sizeof(winplace);

	if (GetWindowPlacement(hMainWindow, &winplace))
	{
		userDefaults.iMainX      = winplace.rcNormalPosition.left;
		userDefaults.iMainY      = winplace.rcNormalPosition.top;
		userDefaults.iMainWidth  = winplace.rcNormalPosition.right - winplace.rcNormalPosition.left;
		userDefaults.iMainHeight = winplace.rcNormalPosition.bottom - winplace.rcNormalPosition.top;
		userDefaults.showState   = winplace.showCmd;
	}
}
//----------------------------------------------------------------------------
//
BOOL32 UserDefaults::GetUserData (const C8 * typeName, const C8 * instanceName, void * data, U32 dataSize)
{
	DOCDESC ddesc;
	COMPTR<IDocument> doc;
	GENRESULT result;

	ddesc.memory = data;
	ddesc.memoryLength = dataSize;

	if (viewerCount == 0)
	{
		hOldCursor = (HCURSOR)GetClassLong(hMainWindow, GCL_HCURSOR);
		SetClassLong(hMainWindow, GCL_HCURSOR, (LONG)LoadCursor(0, IDC_ARROW));
	}
	viewerCount++;

	if ((result = DACOM->CreateInstance(&ddesc, doc)) == GR_OK)
	{
		COMPTR<IViewer> viewer;
		VIEWDESC vdesc;
		HWND hwnd;

		vdesc.className = typeName;
		vdesc.doc = doc;
		vdesc.hOwnerWindow = hMainWindow;
		
		if ((result = PARSER->CreateInstance(&vdesc, viewer)) == GR_OK)
		{
			BOOL32 bState;
			S32 x,y;
			RECT rect;

			WM->GetCursorPos(x, y);

			viewer->get_main_window((void **)&hwnd);
			if ((x -= (userDefaults.iViewerWidth/2)) < 0)
				x = 0;
			if ((y -= (userDefaults.iViewerHeight/2)) < 0)
				y = 0;
			x += (viewerCount-1) * GetSystemMetrics(SM_CXSIZE);
			y += (viewerCount-1) * GetSystemMetrics(SM_CYSIZE);
			
			MoveWindow(hwnd, x, y, 
					   userDefaults.iViewerWidth, userDefaults.iViewerHeight, 1);

			if (instanceName)
				viewer->set_instance_name(instanceName);
			viewer->set_display_state(1);

			while (viewer->get_display_state(&bState) == GR_OK && bState)
			{	
				WM->ServeMessageQueue();
			}

			viewer->get_rect(&rect);
			userDefaults.iViewerWidth  = rect.right - rect.left;
			userDefaults.iViewerHeight = rect.bottom - rect.top;

			if (doc->IsModified())
			{
				DWORD dwRead;
				doc->SetFilePointer(0,0);
				doc->ReadFile(0, data, dataSize, &dwRead, 0);
			}
			else
				result = GR_GENERIC;		// data did not change
		}
	}

	if (--viewerCount == 0)
		SetClassLong(hMainWindow, GCL_HCURSOR, (LONG) hOldCursor);

	return (result == GR_OK);
}
//----------------------------------------------------------------------------
//
BOOL32 UserDefaults::GetScriptData (void * symbol, const C8 * instanceName, void * data, U32 dataSize)
{
	DOCDESC ddesc;
	COMPTR<IDocument> doc;
	GENRESULT result;

	ddesc.memory = data;
	ddesc.memoryLength = dataSize;

	if (viewerCount == 0)
	{
		hOldCursor = (HCURSOR)GetClassLong(hMainWindow, GCL_HCURSOR);
		SetClassLong(hMainWindow, GCL_HCURSOR, (LONG)LoadCursor(0, IDC_ARROW));
	}
	viewerCount++;

	if ((result = DACOM->CreateInstance(&ddesc, doc)) == GR_OK)
	{
		COMPTR<IViewer> viewer;
		struct VIEWDESC2 : VIEWDESC
		{
			void * symbol;
		} vdesc;
		HWND hwnd;

		vdesc.className = "SYMBOL";		// signal that have a pointer to a symbol instead of a name
		vdesc.symbol = symbol;
		vdesc.doc = doc;
		vdesc.hOwnerWindow = hMainWindow;
		vdesc.size = sizeof(vdesc);
		
		if ((result = PARSER->CreateInstance(&vdesc, viewer)) == GR_OK)
		{
			BOOL32 bState;
			S32 x,y;
			RECT rect;

			WM->GetCursorPos(x, y);

			viewer->get_main_window((void **)&hwnd);
			if ((x -= (userDefaults.iViewerWidth/2)) < 0)
				x = 0;
			if ((y -= (userDefaults.iViewerHeight/2)) < 0)
				y = 0;
			x += (viewerCount-1) * GetSystemMetrics(SM_CXSIZE);
			y += (viewerCount-1) * GetSystemMetrics(SM_CYSIZE);
			
//			MoveWindow(hwnd, x, y, 
//					   userDefaults.iViewerWidth, userDefaults.iViewerHeight, 1);
			SetWindowPos(hwnd, HWND_TOP, x, y, 
					   userDefaults.iViewerWidth, userDefaults.iViewerHeight, SWP_NOMOVE);

			if (instanceName)
				viewer->set_instance_name(instanceName);
			viewer->set_display_state(1);

			while (viewer->get_display_state(&bState) == GR_OK && bState)
			{	
				WM->ServeMessageQueue();
			}

			viewer->get_rect(&rect);
			userDefaults.iViewerWidth  = rect.right - rect.left;
			userDefaults.iViewerHeight = rect.bottom - rect.top;

			if (doc->IsModified())
			{
				DWORD dwRead;
				doc->SetFilePointer(0,0);
				doc->ReadFile(0, data, dataSize, &dwRead, 0);
			}
			else
				result = GR_GENERIC;		// data did not change
		}
	}

	if (--viewerCount == 0)
		SetClassLong(hMainWindow, GCL_HCURSOR, (LONG) hOldCursor);

	return (result == GR_OK);
}

typedef DWORD (*EBUPROC) (LPCTSTR lpRegKeyLocation, LPCTSTR lpEULAFileName, LPCSTR lpWarrantyFileName, BOOL fCheckForFirstRun);
//----------------------------------------------------------------------------
//
BOOL32 UserDefaults::UbiEula ()
{
	BOOL32 result = 1;
	char eula_c[64];

	WideCharToMultiByte(CP_ACP, 0, _localLoadStringW(IDS_EULA_FILE), -1, eula_c, sizeof(eula_c), 0, 0);

	result = DoEulaWin(eula_c);

	return result;
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//
BOOL32 CreateUserDefaults (void)
{
	if (DEFAULTS==0)
	{
		DEFAULTS = new DAComponent<UserDefaults>;
		AddToGlobalCleanupList((IDAComponent **) &DEFAULTS);
	}

	return 1;
}


//----------------------------------------------------------------------------
//--------------------------End UserDefaults.cpp------------------------------
//----------------------------------------------------------------------------
