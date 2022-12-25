//--------------------------------------------------------------------------//
//                                                                          //
//                             UserDefaults.cpp                             //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Author: Ajackson $

    $Header: /Libs/Src/Tools/adb/Userdefaults.CPP 5     10/03/03 2:54p Ajackson $
*/			    
//---------------------------------------------------------------------------
/*
        Stores user preferences in the registry
*/
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

#include "pch.h"

#include <commdlg.h>               // Common dialogs
#include "UserDefaults.h"
#include <TComponent.h>
#include "Resource.h"

//-------------------------------------------------------------------------------------

#ifdef _DEBUG
#ifndef DEBUG_NEW
	void* __cdecl operator new(size_t nSize, LPCSTR lpszFileName, int nLine);
	#define DEBUG_NEW new(THIS_FILE, __LINE__)
	#if _MSC_VER >= 1200
		void __cdecl operator delete(void* p, LPCSTR lpszFileName, int nLine);
	#endif
#endif
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//-------------------------------------------------------------------------------------

IUserDefaults * DEFAULTS;

static char szCompanyName[] = "Warthog";
static char szProductName[] = "DatabaseCQ2";
static char szClass[]       = "Conquest 2";
static char szDefaultsKey[] = "User Defaults";

#define MAX_MRU	4		// remember last four files loaded


struct DACOM_NO_VTABLE UserDefaults : public IUserDefaults
{
	USER_DEFAULTS userDefaults;
	
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

	/* IUserDefaults methods */

	DEFMETHOD_(BOOL32,LoadDefaults) (void);

	DEFMETHOD_(BOOL32,StoreDefaults) (void);

	DEFMETHOD_(BOOL32,GetDirectoryFromRegistry) (char *szDirName, U32 id);
	
	DEFMETHOD_(BOOL32,SetDirectoryInRegistry) (const char *szDirName, U32 id);

	DEFMETHOD_(BOOL32,GetInputFilename) (char *dst, U32 id);

	DEFMETHOD_(BOOL32,GetOutputFilename) (char *dst, U32 id);

	DEFMETHOD_(USER_DEFAULTS *,GetDefaults) (void)
	{
		return &userDefaults;
	}

	DEFMETHOD_(BOOL32,SetNameInMRU) (const char *szFileName, U32 id, U32 pos=0);

	DEFMETHOD_(BOOL32,GetNameInMRU) (char *szFileName, U32 id, U32 pos=0);

	DEFMETHOD_(S32,FindNameInMRU) (const char *szFileName, U32 id);

	DEFMETHOD_(BOOL32,RemoveNameFromMRU) (const char *szFileName, U32 id);

	DEFMETHOD_(BOOL32,RemoveNameFromMRU) (U32 id, U32 pos);

	DEFMETHOD_(BOOL32,InsertNameIntoMRU) (const char *szFileName, U32 id);

	DEFMETHOD_(BOOL32,InsertDataPath) (const char *szFilePath);

	/* UserDefaults methods */

	BOOL32 DeleteMRUEntry (U32 id, U32 pos);

	void SetWindowDefaults (void);

	void init (void);
};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//
void UserDefaults::init (void)
{
	// initialize the defaults structure

	memset(&userDefaults, 0, sizeof(userDefaults));
	userDefaults.dwSize = sizeof(userDefaults);
	userDefaults.dwVersion = DEFAULTS_VERSION;
	userDefaults.disableHeader = true;
//	userDefaults.showState = SW_SHOW;
}
//----------------------------------------------------------------------------
//
BOOL32 UserDefaults::LoadDefaults (void)
{
	HKEY hkey1, hkey2, hkey3;
	U32 cbData, dwType;
	BOOL32 result = 0;

	if (RegOpenKeyEx(HKEY_CURRENT_USER, "Software", 0, KEY_READ, &hkey1) != ERROR_SUCCESS)
		goto Done0;

	if (RegOpenKeyEx(hkey1, szCompanyName, 0, KEY_READ, &hkey2) != ERROR_SUCCESS)
		goto Done1;
	
	if (RegOpenKeyEx(hkey2, szProductName, 0, KEY_READ, &hkey3) != ERROR_SUCCESS)
		goto Done2;

	cbData = sizeof(userDefaults);
	if (RegQueryValueEx(hkey3, szDefaultsKey, 0, &dwType, (U8 *)&userDefaults, &cbData) != ERROR_SUCCESS)
		goto Done3;

	if (dwType != REG_BINARY)
	{
		MessageBox(hMainWindow,_localLoadString(IDS_BAD_REGISTRY_VALUE),"Error",MB_OK);
		init();
		goto Done3;
	}

	if (cbData != sizeof(USER_DEFAULTS))
	{
		init();
	 	goto Done3;
	}

	result = 1;

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
BOOL32 UserDefaults::StoreDefaults (void)
{
	HKEY hkey1, hkey2, hkey3;
	BOOL32 result = 0;
	U32 dwDisposition;

	userDefaults.dwSize = sizeof(userDefaults);
	SetWindowDefaults();

	if (RegOpenKeyEx(HKEY_CURRENT_USER, "Software", 0, KEY_ALL_ACCESS, &hkey1) != ERROR_SUCCESS)
		goto Done0;

	if (RegCreateKeyEx(hkey1, szCompanyName, 0, szClass, REG_OPTION_NON_VOLATILE, 
		KEY_ALL_ACCESS, 0, &hkey2, &dwDisposition) != ERROR_SUCCESS)
	{
		goto Done1;
	}
	
	if (RegCreateKeyEx(hkey2, szProductName, 0, szClass, REG_OPTION_NON_VOLATILE,
		KEY_ALL_ACCESS, 0, &hkey3, &dwDisposition) != ERROR_SUCCESS)
	{
		goto Done2;
	}

	if (RegSetValueEx(hkey3, szDefaultsKey, 0, REG_BINARY, (U8 *)&userDefaults, sizeof(userDefaults)) != ERROR_SUCCESS)
	{
		goto Done3;
	}

	result = 1;

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
BOOL32 UserDefaults::GetDirectoryFromRegistry (char *szDirName, U32 id)
{
	HKEY hkey1, hkey2, hkey3;
	char buffer[16];
	unsigned long cbData, dwType;
	BOOL result = 0;

	if (RegOpenKeyEx(HKEY_CURRENT_USER, "Software", 0, KEY_READ, &hkey1) != ERROR_SUCCESS)
		return 0;

	if (RegOpenKeyEx(hkey1, szCompanyName, 0, KEY_READ, &hkey2) != ERROR_SUCCESS)
	{
		goto Done1;
	}
	
	if (RegOpenKeyEx(hkey2, szProductName, 0, KEY_READ, &hkey3) != ERROR_SUCCESS)
	{
		goto Done2;
	}

	wsprintf(buffer, "%d", id);
	cbData = 256;
	if (RegQueryValueEx(hkey3, buffer, 0, &dwType, (U8 *)szDirName, &cbData) != ERROR_SUCCESS)
	{
		goto Done3;
	}

	if (dwType != REG_SZ)
	{
		MessageBox(hMainWindow,_localLoadString(IDS_BAD_REGISTRY_VALUE),"Bummer",MB_OK);
		goto Done3;
	}

	result = 1;

Done3:
	RegCloseKey(hkey3);
Done2:
	RegCloseKey(hkey2);
Done1:
	RegCloseKey(hkey1);

	return result;
}
//----------------------------------------------------------------------------
//
BOOL32 UserDefaults::SetDirectoryInRegistry (const char *szDirName, U32 id)
{
	HKEY hkey1, hkey2, hkey3;
	char buffer[16];
	BOOL result = 0;
	unsigned long dwDisposition;

	if (RegOpenKeyEx(HKEY_CURRENT_USER, "Software", 0, KEY_ALL_ACCESS, &hkey1) != ERROR_SUCCESS)
		return 0;

	if (RegCreateKeyEx(hkey1, szCompanyName, 0, szClass, REG_OPTION_NON_VOLATILE, 
		KEY_ALL_ACCESS, 0, &hkey2, &dwDisposition) != ERROR_SUCCESS)
	{
		goto Done1;
	}
	
	if (RegCreateKeyEx(hkey2, szProductName, 0, szClass, REG_OPTION_NON_VOLATILE,
		KEY_ALL_ACCESS, 0, &hkey3, &dwDisposition) != ERROR_SUCCESS)
	{
		goto Done2;
	}

	wsprintf(buffer, "%d", id);
	if (RegSetValueEx(hkey3, buffer, 0, REG_SZ, (U8 *)szDirName, strlen(szDirName)+1) != ERROR_SUCCESS)
	{
		goto Done3;
	}

	result = 1;

Done3:
	RegCloseKey(hkey3);
Done2:
	RegCloseKey(hkey2);
Done1:
	RegCloseKey(hkey1);

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

    // Retrieve the current directory name and store it in szDirName.

	*dst = 0;
	if (GetDirectoryFromRegistry(szDirName, id) == 0)
	    GetCurrentDirectory(sizeof(szDirName), szDirName);

    // Load the filter string from the resource file.

    cbString = LoadString(hInstance, id, szFilter, sizeof(szFilter));

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
			SetDirectoryInRegistry(szDirName, id);
			RemoveNameFromMRU(dst, id);
			InsertNameIntoMRU(dst, id);
		}

		return 1;
	}
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

    // Retrieve the current directory name and store it in szDirName.

	if (GetDirectoryFromRegistry(szDirName, id) == 0)
	    GetCurrentDirectory(sizeof(szDirName), szDirName);

    // Load the filter string from the resource file.

    cbString = LoadString(hInstance, id, szFilter, sizeof(szFilter));

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
			SetDirectoryInRegistry(szDirName, id);
			RemoveNameFromMRU(dst, id);
			InsertNameIntoMRU(dst, id);
		}

		return 1;
	}
	return 0;
}
//----------------------------------------------------------------------------
//
BOOL32 UserDefaults::GetNameInMRU (char *szFileName, U32 id, U32 pos)
{
	BOOL32 result=0;
	HKEY hkey1, hkey2, hkey3;
	char buffer[16];
	unsigned long cbData, dwType;

	if (pos >= MAX_MRU)
		goto Done;

	if (RegOpenKeyEx(HKEY_CURRENT_USER, "Software", 0, KEY_READ, &hkey1) != ERROR_SUCCESS)
		goto Done;

	if (RegOpenKeyEx(hkey1, szCompanyName, 0, KEY_READ, &hkey2) != ERROR_SUCCESS)
		goto Done1;
	
	if (RegOpenKeyEx(hkey2, szProductName, 0, KEY_READ, &hkey3) != ERROR_SUCCESS)
		goto Done2;

	wsprintf(buffer, "%d-MRU%d", id, pos);
	cbData = 256;
	if (RegQueryValueEx(hkey3, buffer, 0, &dwType, (U8 *)szFileName, &cbData) != ERROR_SUCCESS)
		goto Done3;

	if (dwType != REG_SZ)
	{
		MessageBox(hMainWindow,_localLoadString(IDS_BAD_REGISTRY_VALUE),"Error",MB_OK);
		goto Done3;
	}

	result = 1;

Done3:
	RegCloseKey(hkey3);
Done2:
	RegCloseKey(hkey2);
Done1:
	RegCloseKey(hkey1);
Done:
	return result;
}
//----------------------------------------------------------------------------
//
BOOL32 UserDefaults::SetNameInMRU (const char *szFileName, U32 id, U32 pos)
{
	HKEY hkey1, hkey2, hkey3;
	char buffer[16];
	BOOL32 result = 0;
	unsigned long dwDisposition;

	if (RegOpenKeyEx(HKEY_CURRENT_USER, "Software", 0, KEY_ALL_ACCESS, &hkey1) != ERROR_SUCCESS)
		goto Done;

	if (RegCreateKeyEx(hkey1, szCompanyName, 0, szClass, REG_OPTION_NON_VOLATILE, 
		KEY_ALL_ACCESS, 0, &hkey2, &dwDisposition) != ERROR_SUCCESS)
	{
		goto Done1;
	}
	
	if (RegCreateKeyEx(hkey2, szProductName, 0, szClass, REG_OPTION_NON_VOLATILE,
		KEY_ALL_ACCESS, 0, &hkey3, &dwDisposition) != ERROR_SUCCESS)
	{
		goto Done2;
	}

	wsprintf(buffer, "%d-MRU%d", id, pos);
	if (RegSetValueEx(hkey3, buffer, 0, REG_SZ, (U8 *)szFileName, strlen(szFileName)+1) != ERROR_SUCCESS)
	{
		goto Done3;
	}

	result = 1;

Done3:
	RegCloseKey(hkey3);
Done2:
	RegCloseKey(hkey2);
Done1:
	RegCloseKey(hkey1);
Done:
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
	HKEY hkey1, hkey2, hkey3;
	char buffer[16];
	BOOL32 result = 0;
	unsigned long dwDisposition;

	if (RegOpenKeyEx(HKEY_CURRENT_USER, "Software", 0, KEY_ALL_ACCESS, &hkey1) != ERROR_SUCCESS)
		goto Done;

	if (RegCreateKeyEx(hkey1, szCompanyName, 0, szClass, REG_OPTION_NON_VOLATILE, 
		KEY_ALL_ACCESS, 0, &hkey2, &dwDisposition) != ERROR_SUCCESS)
	{
		goto Done1;
	}
	
	if (RegCreateKeyEx(hkey2, szProductName, 0, szClass, REG_OPTION_NON_VOLATILE,
		KEY_ALL_ACCESS, 0, &hkey3, &dwDisposition) != ERROR_SUCCESS)
	{
		goto Done2;
	}

	wsprintf(buffer, "%d-MRU%d", id, pos);
	if (RegDeleteValue(hkey3, buffer) != ERROR_SUCCESS)
	{
		goto Done3;
	}

	result = 1;

Done3:
	RegCloseKey(hkey3);
Done2:
	RegCloseKey(hkey2);
Done1:
	RegCloseKey(hkey1);
Done:
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
BOOL32 UserDefaults::InsertDataPath(const char *szFilePath)
{
	HKEY hkey1, hkey2, hkey3;
//	char buffer[64];
	BOOL32 result = 0;
	unsigned long dwDisposition;

	if (RegOpenKeyEx(HKEY_CURRENT_USER, "Software", 0, KEY_ALL_ACCESS, &hkey1) != ERROR_SUCCESS)
	{
		goto Done;
	}
	if (RegCreateKeyEx(hkey1, szCompanyName, 0, szClass, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, 0, &hkey2, &dwDisposition) != ERROR_SUCCESS)
	{
		goto Done1;
	}
	
	if (RegCreateKeyEx(hkey2, szProductName, 0, szClass, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, 0, &hkey3, &dwDisposition) != ERROR_SUCCESS)
	{
		goto Done2;
	}

//LONG RegEnumValue(
//  HKEY hKey,
//  DWORD dwIndex,
//  LPTSTR lpValueName,
//  LPDWORD lpcbValueName,
//  LPDWORD lpReserved,
//  LPDWORD lpType,
//  LPBYTE lpData,
//  LPDWORD lpcbData
//);
//
//	if( 1 )
//	{
//		const char* dataPathMask = "dp_%04d";
//		unsigned long dwNumKey = 0;
//
//		while( RegEnumValue(hkey3, dwNumKey, buffer, 64) )
//		{
//			HKEY hKey;
//			char key[128];
//			sprintf( key, dataPathMask, numPath );
//
//			if( RegOpenKey(hkey2,key,&hKey) )
//			{
//			}
//		}
//	}
//
Done2:
	RegCloseKey(hkey2);
Done1:
	RegCloseKey(hkey1);
Done:
	return result;
}
//----------------------------------------------------------------------------
//
void UserDefaults::SetWindowDefaults (void)
{
	WINDOWPLACEMENT winplace;

	winplace.length = sizeof(winplace);

	if (GetWindowPlacement(hMainWindow, &winplace))
	{
		userDefaults.iMainX      = winplace.rcNormalPosition.left;
		userDefaults.iMainY      = winplace.rcNormalPosition.top;
	//	userDefaults.iMainWidth  = winplace.rcNormalPosition.right - winplace.rcNormalPosition.left;
	//	userDefaults.iMainHeight = winplace.rcNormalPosition.bottom - winplace.rcNormalPosition.top;
	//	userDefaults.showState   = winplace.showCmd;
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//
BOOL32 CreateUserDefaults (void)
{
	if (DEFAULTS==0)
		DEFAULTS = new DAComponent<UserDefaults>;

	return 1;
}


//----------------------------------------------------------------------------
//--------------------------End UserDefaults.cpp------------------------------
//----------------------------------------------------------------------------
