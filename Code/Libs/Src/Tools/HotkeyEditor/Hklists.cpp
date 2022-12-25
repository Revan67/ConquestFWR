//--------------------------------------------------------------------------//
//                                                                          //
//                               HKLists.cpp                               //
//                                                                          //
//                  COPYRIGHT (C) 1996 BY ORIGIN SYSTEMS, INC.              //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Logfile: /Libs/dev/Src/Tools/HotkeyEditor/Hklists.cpp $

    $Revision: 10 $

    $Date: 3/21/00 4:30p $

    $Author: Pbleisch $
*/			    
//--------------------------------------------------------------------------//

#include <windows.h>
#include <commctrl.h>               // Common controls
#include "resource.h"

#include "ferror.h"
#include <HKeyRec.h>
#include <HKEvent.h>
#include <FileSys.h>
#include <EventSys.h>
#include <TComponent.h>
#include <TSmartPointer.h>
#include <IConnection.h>

#include "HKGroup.h"


#define MAX_HOTKEYS 256
#define MAX_UNDO    8
#define HKEDITOR_VERSION    HKEVENT_VERSION
#define NAME_ARRAY_SIZE 8

struct HKUNDO_RECORD;
struct HKUNDO_RECORD	: public HKRECORD
{
	ACTION action;
	DWORD dwNumber;

	HKUNDO_RECORD (void) : HKRECORD()
	{
	 	action = Invalid;
	}

	HKUNDO_RECORD & operator = (HKRECORD & rec)
	{
		*((HKRECORD *)this) = rec;
		action = Invalid;
		return *this;
	}
};

HKRECORD HotKeyList[MAX_HOTKEYS];
DWORD dwNumKeys=0;		// number of keys in the list

HKUNDO_RECORD UndoList[MAX_UNDO];
DWORD dwUndoHead=0;		// next index to write new undo record

const char *pNameArray[NAME_ARRAY_SIZE];
int  iNameArrayHead=0;

HWND hTestWindow=0;
extern BOOL bGlobalRecording;
extern HWND hListView;
extern LPFILESYSTEM pHotkeyFile;
extern HMENU hMainMenu;
extern BOOL bWinDataChanged;
extern BOOL bUntitled;
extern char szBaseEditName[256];		// name of the main window
extern char szUntitled[40];
extern DWORD dwRawHotkeySize;
extern ICOManager * DACOM;

BOOL bWriteText=0;
BOOL bWriteHeader=0;
BOOL WriteTextFile (char *szName);
BOOL WriteHeaderFile (char *szName);


BOOL SortHotKeyList (int iSortColumn);
BOOL SetTestWindowSize (HWND hwnd);
void SetDefaults (void);

COMPTR<IHotkeyEvent> HotkeyEvent;		// used for testing hotkeys
COMPTR<struct EventCallback> event;			// used for testing hotkeys


#define HOTKEY_EVENT	(WM_USER+4)
#define JOY_EVENT       (WM_USER+5)


//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
void ClearUndo (void)
{
	memset(UndoList, 0, sizeof(UndoList));
	dwUndoHead = 0;
	EnableMenuItem(hMainMenu, IDM_UNDO, MF_GRAYED);
}
//--------------------------------------------------------------------------//
//
void ClearHotKeyList (void)
{
	memset(HotKeyList, 0, sizeof(HotKeyList));
	dwNumKeys=0;
}
//--------------------------------------------------------------------------//
//
char *GetFileName (LPFILESYSTEM pFile)
{
	static char filename[MAX_PATH];

	pFile->GetFileName(filename, sizeof(filename));

	return filename;
}
//--------------------------------------------------------------------------//
//
BOOL InsertHKRecord (HKRECORD *record, DWORD dwNumber, BOOL bUpdateUndo)
{
	DWORD start;
	LV_ITEM lvi;

	if (dwNumKeys >= MAX_HOTKEYS || dwNumber>dwNumKeys)
		return 0;

	// put new record into ListView

	lvi.mask        = LVIF_TEXT | LVIF_STATE;
	lvi.state       = 0;
	lvi.stateMask   = 0;
	lvi.iItem       = dwNumber;
	lvi.iSubItem    = 0;
	lvi.pszText     = record->szName;
	lvi.cchTextMax  = 0; // Ignored for Set
	lvi.iImage      = 0; // iItem;
	lvi.lParam      = 0; // (LPARAM) &lpItems[iItem];

	if (ListView_InsertItem(hListView, &lvi) == -1)
		return FALSE;
	ListView_SetItemText(hListView, dwNumber, 1, record->szKeyCombo);
	ListView_SetItemText(hListView, dwNumber, 2, record->szDescription);

	HKGROUP::NewRecordWasInserted(dwNumber);
	ListView_SetItemState(hListView, dwNumber+1, 3, 0xF);
	ListView_EnsureVisible(hListView, dwNumber+1, 0);
	// move the list down
	start = dwNumKeys;
	while (start > dwNumber)
	{
		HotKeyList[start] = HotKeyList[start-1];
		start--;
	}

	HotKeyList[dwNumber] = *record;

	// record transaction in the undo list
	if (bUpdateUndo)
	{
		UndoList[dwUndoHead].action = Delete;
		UndoList[dwUndoHead].dwNumber = dwNumber;
		if (++dwUndoHead >= MAX_UNDO)
			dwUndoHead = 0;
		EnableMenuItem(hMainMenu, IDM_UNDO, MF_ENABLED);
		bWinDataChanged++;
	}

	dwNumKeys++;
	
	return 1;
}
//--------------------------------------------------------------------------//
//
BOOL DeleteHKRecord (DWORD dwNumber, BOOL bUpdateUndo)
{
	DWORD start;

	if (dwNumber>=dwNumKeys)
		return 0;

	ListView_DeleteItem(hListView, dwNumber);
	ListView_SetItemState(hListView, dwNumber, 3, 0xF);
	ListView_EnsureVisible(hListView, dwNumber, 0);

	//record transaction in the undo list
	if (bUpdateUndo)
	{
		UndoList[dwUndoHead] = HotKeyList[dwNumber];
		UndoList[dwUndoHead].action   = Insert;
		UndoList[dwUndoHead].dwNumber = dwNumber;
		if (++dwUndoHead >= MAX_UNDO)
			dwUndoHead = 0;
		EnableMenuItem(hMainMenu, IDM_UNDO, MF_ENABLED);
		bWinDataChanged++;
	}

	HKGROUP::RecordWasDeleted(dwNumber);
	dwNumKeys--;
	start = dwNumber;
	while (start < dwNumKeys)
	{	
		HotKeyList[start] = HotKeyList[start+1];
		start++;
	}

	return 1;	
}
//--------------------------------------------------------------------------//
//
BOOL ModifyHKRecord (HKRECORD *record, DWORD dwNumber, BOOL bUpdateUndo)
{
	if (dwNumber>dwNumKeys)
		return 0;
	else
	if (dwNumber==dwNumKeys)
		return InsertHKRecord(record, dwNumber, bUpdateUndo);

	// if no change was made, don't do anything
	if (memcmp(&HotKeyList[dwNumber], record, sizeof(HKRECORD)) == 0)
		return 1;

	ListView_SetItemText(hListView, dwNumber, 0, record->szName);
	ListView_SetItemText(hListView, dwNumber, 1, record->szKeyCombo);
	ListView_SetItemText(hListView, dwNumber, 2, record->szDescription);
	ListView_SetItemState(hListView, dwNumber+1, 3, 0xF);
	ListView_EnsureVisible(hListView, dwNumber+1, 0);

	//record transaction in the undo list
	if (bUpdateUndo)
	{
		UndoList[dwUndoHead] = HotKeyList[dwNumber];
		UndoList[dwUndoHead].action   = Replace;
		UndoList[dwUndoHead].dwNumber = dwNumber;
		if (++dwUndoHead >= MAX_UNDO)
			dwUndoHead = 0;
		EnableMenuItem(hMainMenu, IDM_UNDO, MF_ENABLED);
		bWinDataChanged++;
	}

	HotKeyList[dwNumber] = *record;
	
	return 1;
}
//--------------------------------------------------------------------------//
// Undo one action
//
BOOL ProcessUndo (void)
{
	DWORD dwIndex;

	if ((dwIndex=dwUndoHead-1) >= MAX_UNDO)
		dwIndex = MAX_UNDO-1;

	switch (UndoList[dwIndex].action)
	{
	 	case Insert:
			InsertHKRecord(&UndoList[dwIndex], UndoList[dwIndex].dwNumber, 0);
			break;
			
		case Delete:
			DeleteHKRecord(UndoList[dwIndex].dwNumber, 0);
			break;
			
		case Replace:
			ModifyHKRecord(&UndoList[dwIndex], UndoList[dwIndex].dwNumber, 0);
			break;

		default:
			EnableMenuItem(hMainMenu, IDM_UNDO, MF_GRAYED);
			return 0;
	}

	dwUndoHead = dwIndex;
	UndoList[dwIndex].action = Invalid;

	// see if UndoList is now empty

	if ((dwIndex=dwUndoHead-1) >= MAX_UNDO)
		dwIndex = MAX_UNDO-1;
	if (UndoList[dwIndex].action == Invalid)
		EnableMenuItem(hMainMenu, IDM_UNDO, MF_GRAYED);
	bWinDataChanged--;

	return 1;
}
//----------------------------------------------------------------------------
//
BOOL InitializeListView (void)
{
	DWORD i;
	LV_ITEM lvi;
	int iItemToEdit;
	int maxStringLen=0;
	char *maxString=0;

	iItemToEdit = ListView_GetNextItem(hListView, -1, LVNI_SELECTED);
	ListView_DeleteAllItems(hListView);
	
	lvi.mask        = LVIF_TEXT | LVIF_STATE;
	lvi.state       = 0;
	lvi.stateMask   = 0;
	lvi.iItem       = 0;
	lvi.iSubItem    = 0;
	lvi.pszText     = HotKeyList[0].szName;
	lvi.cchTextMax  = 0; // Ignored for Set
	lvi.iImage      = 0; // iItem;
	lvi.lParam      = 0; // (LPARAM) &lpItems[iItem];

	for (i=0; i < dwNumKeys; i++)
	{
		int len;
		// put new record into ListView

		lvi.mask        = LVIF_TEXT | LVIF_STATE;
		lvi.iItem       = i;
		lvi.pszText     = HotKeyList[i].szName;

		if (ListView_InsertItem(hListView, &lvi) == -1)
			return FALSE;
		ListView_SetItemText(hListView, i, 1, HotKeyList[i].szKeyCombo);
		ListView_SetItemText(hListView, i, 2, HotKeyList[i].szDescription);

		len = strlen(lvi.pszText);
		if (len > maxStringLen)
		{
			maxStringLen = len;
			maxString = lvi.pszText;
		}
 	}

	if (maxString)
	{
		int cx = ListView_GetStringWidth(hListView, maxString);
		cx += ListView_GetStringWidth(hListView, "    ");
		ListView_SetColumnWidth(hListView, 0, cx);
	}

	lvi.iItem       = dwNumKeys;
	if (ListView_InsertItem(hListView, &lvi) == -1)
		return FALSE;
	ListView_SetItemText(hListView, dwNumKeys, 0, "<EOL>");
	ListView_SetItemText(hListView, dwNumKeys, 1, " ");
	ListView_SetItemText(hListView, dwNumKeys, 2, " ");

	if (iItemToEdit >= 0)
	{
		ListView_SetItemState(hListView, iItemToEdit, 3, 0xF);
		ListView_EnsureVisible(hListView, iItemToEdit, 0);
		ListView_EnsureVisible(hListView, iItemToEdit+4, 0);
	}

	return 1;
}
//----------------------------------------------------------------------------
// Set all of the edit control states 
//
BOOL SetControlStatesAtOpen (void)
{
	InitializeListView();

	LoadString(hInstance, IDS_APP_NAME, szBaseEditName, 64);
	strcat(szBaseEditName, " - ");
	if (bUntitled)
		strcat(szBaseEditName, szUntitled);
	else
		strcat(szBaseEditName, GetFileName(pHotkeyFile));
	SetWindowText(hMainWindow, szBaseEditName);
	EnableMenuItem(hMainMenu, IDM_SAVE, MF_ENABLED);
	EnableMenuItem(hMainMenu, IDM_SAVEAS, MF_ENABLED);
	EnableMenuItem(hMainMenu, IDM_CLOSE, MF_ENABLED);
	EnableMenuItem(hMainMenu, 1, MF_BYPOSITION | MF_ENABLED);
	EnableMenuItem(hMainMenu, 3, MF_BYPOSITION | MF_ENABLED);
	DrawMenuBar(hMainWindow);
	EnableWindow(hListView, 1);
	SetFocus(hListView);
	return 1;
}
//----------------------------------------------------------------------------
//  Open the file, read stuff, then set all of the dialog control states
//
BOOL OpenHotKeyFile (char *szFilename)
{
	char buffer[256];
	DWORD i;
	IHotkeyRecorder * hkmanager=0;
	DAHOTKEY hotkey;
	HANDLE hFile;
	DAFILEDESC desc = szFilename;
	DWORD dwVersion, dwBytesRead, dwNumStrings, dwSizeOfString;

	if (szFilename==0)
	{
	 	bUntitled=1;
		return SetControlStatesAtOpen();
	}

	{
		HKRECDESC desc;

		desc.hWindow = hMainWindow;

		if (DACOM->CreateInstance(&desc, (void **)&hkmanager) != GR_OK)
			return 0;
	}

	if (pHotkeyFile)
	{	
		pHotkeyFile->GetFileName(buffer, sizeof(buffer));
		pHotkeyFile->Release();
		pHotkeyFile=0;
	}
	else
		buffer[0] = 0;


	if (DACOM->CreateInstance(&desc, (void **)&pHotkeyFile) != GR_OK)
		goto Fail;

	// else successfully opened the file!

	ListView_DeleteAllItems(hListView);
	dwNumKeys = 0;
	bUntitled=0;
	ClearUndo();
	bWinDataChanged=0;


	desc.lpFileName = "Version";
	if ((hFile = pHotkeyFile->OpenChild(&desc)) == INVALID_HANDLE_VALUE)
		goto Fail;
	pHotkeyFile->ReadFile(hFile, &dwVersion, sizeof(dwVersion), &dwBytesRead, 0);
	if (dwVersion != HKEDITOR_VERSION)
		goto Fail;
	pHotkeyFile->CloseHandle(hFile);

	desc.lpFileName = "Binary data";
	if ((hFile = pHotkeyFile->OpenChild(&desc)) == INVALID_HANDLE_VALUE)
		goto Fail;
	
	pHotkeyFile->ReadFile(hFile, &dwNumKeys, sizeof(dwNumKeys), &dwBytesRead, 0);
	pHotkeyFile->ReadFile(hFile, &dwRawHotkeySize, sizeof(dwRawHotkeySize), &dwBytesRead, 0);

	for (i = 0; i < dwNumKeys; i++)
	{
	 	pHotkeyFile->ReadFile(hFile, (byte*)HotKeyList[i].dwKeyCombo, dwRawHotkeySize, &dwBytesRead, 0);
	}
	pHotkeyFile->CloseHandle(hFile);

	desc.lpFileName = "Event messages";
	if ((hFile = pHotkeyFile->OpenChild(&desc)) == INVALID_HANDLE_VALUE)
		goto Fail;

	pHotkeyFile->ReadFile(hFile, &dwNumStrings, sizeof(dwNumStrings), &dwBytesRead, 0);
	pHotkeyFile->ReadFile(hFile, &dwSizeOfString, sizeof(dwSizeOfString), &dwBytesRead, 0);

	{
		char buffer2[256];
		DWORD j=1;		// first symbol is 1

		memset(buffer2, 0, sizeof(buffer2));
		while (dwNumStrings--)
		{
		 	pHotkeyFile->ReadFile(hFile, buffer2, dwSizeOfString, &dwBytesRead, 0);
			
			for (i = 0; i < dwNumKeys; i++)
			{
				hotkey = hkmanager->CreateHotkey((byte *)HotKeyList[i].dwKeyCombo);
				if (hkmanager->GetSymbolNumber(hotkey) == j)
					strcpy(HotKeyList[i].szName, buffer2);
				hkmanager->DestroyHotkey(hotkey);
			}

			j++;
		}
	}

	pHotkeyFile->CloseHandle(hFile);

	desc.lpFileName = "Keyboard text";
	if ((hFile = pHotkeyFile->OpenChild(&desc)) == INVALID_HANDLE_VALUE)
		goto Fail;

	pHotkeyFile->ReadFile(hFile, &dwNumStrings, sizeof(dwNumStrings), &dwBytesRead, 0);
	pHotkeyFile->ReadFile(hFile, &dwSizeOfString, sizeof(dwSizeOfString), &dwBytesRead, 0);

	for (i = 0; i < dwNumKeys; i++)
	 	pHotkeyFile->ReadFile(hFile, (byte*)HotKeyList[i].szKeyCombo, sizeof(HotKeyList[i].szKeyCombo), &dwBytesRead, 0);
	pHotkeyFile->CloseHandle(hFile);

	desc.lpFileName = "Descriptions";
	if ((hFile = pHotkeyFile->OpenChild(&desc)) == INVALID_HANDLE_VALUE)
		goto Fail;

	pHotkeyFile->ReadFile(hFile, &dwNumStrings, sizeof(dwNumStrings), &dwBytesRead, 0);
	pHotkeyFile->ReadFile(hFile, &dwSizeOfString, sizeof(dwSizeOfString), &dwBytesRead, 0);

	for (i = 0; i < dwNumKeys; i++)
	 	pHotkeyFile->ReadFile(hFile, (byte*)HotKeyList[i].szDescription, sizeof(HotKeyList[i].szDescription), &dwBytesRead, 0);

	pHotkeyFile->CloseHandle(hFile);
	
	hkmanager->Release();
	
	return SetControlStatesAtOpen();
Fail:
	// Message box for failure would be good here.
	hkmanager->Release();
	if (bUntitled==0)
	{
		desc.lpFileName = buffer;
		DACOM->CreateInstance(&desc, (void **) &pHotkeyFile);
	}
	return 0;
}
//----------------------------------------------------------------------------
// Assume that list is sorted
//
BOOL AssignNumbersToHotKeys (void)
{
	DWORD i, dwNum=1;
	IHotkeyRecorder * hkmanager;
	DAHOTKEY hotkey;

	if (dwNumKeys < 1)
		return 1;

	{
		HKRECDESC desc;

		desc.hWindow = hMainWindow;

		if (DACOM->CreateInstance(&desc, (void **)&hkmanager) != GR_OK)
			return 0;
	}

	hotkey = hkmanager->CreateHotkey((byte *)HotKeyList[0].dwKeyCombo);
	hkmanager->SetSymbolNumber(hotkey, dwNum);
	hkmanager->GetData(hotkey, (byte *)HotKeyList[0].dwKeyCombo, sizeof(HotKeyList[0].dwKeyCombo));
	hkmanager->DestroyHotkey(hotkey);
	HotKeyList[0].dwAssignedNumber = dwNum;

	for (i = 1; i < dwNumKeys; i++)
	{
		if (strcmp(HotKeyList[i-1].szName, HotKeyList[i].szName))		// if not equal
			dwNum++;

		hotkey = hkmanager->CreateHotkey((byte *)HotKeyList[i].dwKeyCombo);
		hkmanager->SetSymbolNumber(hotkey, dwNum);
		HotKeyList[i].dwAssignedNumber = dwNum;
		hkmanager->GetData(hotkey, (byte *)HotKeyList[i].dwKeyCombo, sizeof(HotKeyList[i].dwKeyCombo));
		hkmanager->DestroyHotkey(hotkey);
	}

	hkmanager->Release();

	return 1;
}
//----------------------------------------------------------------------------
// on save, clear the undo list! (synchronizes bWinDataChanged with amount of Undo available
//
BOOL SaveHotKeyData (char *szFilename)
{
	char buffer[256];
	DWORD i, dwBytesWritten;
	int iOrigSort = HKRECORD::iSortColumn;
	DAFILEDESC desc = szFilename;
	HANDLE hFile;

	if (SortHotKeyList(0)==0)
		return 0;
	if (AssignNumbersToHotKeys()==0)
		return 0;

	if (pHotkeyFile)
	{
		pHotkeyFile->GetFileName(buffer, sizeof(buffer));
		pHotkeyFile->Release();
	}
	else
		buffer[0] = 0;

	desc.lpImplementation = "UTF";
	desc.dwDesiredAccess |= GENERIC_WRITE;
	desc.dwShareMode = 0;
    desc.dwCreationDistribution = CREATE_ALWAYS;

	if (DACOM->CreateInstance(&desc, (void **) &pHotkeyFile) != GR_OK)
		goto Fail;
	
	// else successfully opened the file!

	desc.lpImplementation = 0;
	desc.lpFileName = "Version";
	hFile = pHotkeyFile->OpenChild(&desc);
	i = HKEDITOR_VERSION;
	pHotkeyFile->WriteFile(hFile, &i, sizeof(i), &dwBytesWritten, 0);
	pHotkeyFile->CloseHandle(hFile);

	desc.lpFileName = "Binary data";
	hFile = pHotkeyFile->OpenChild(&desc);
	pHotkeyFile->WriteFile(hFile, &dwNumKeys, sizeof(dwNumKeys), &dwBytesWritten, 0);
	pHotkeyFile->WriteFile(hFile, &dwRawHotkeySize, sizeof(dwRawHotkeySize), &dwBytesWritten, 0);
	
	for (i = 0; i < dwNumKeys; i++)
	 	pHotkeyFile->WriteFile(hFile, (byte*)HotKeyList[i].dwKeyCombo, dwRawHotkeySize, &dwBytesWritten, 0);
	pHotkeyFile->CloseHandle(hFile);
	
	
	desc.lpFileName = "Event messages";
	hFile = pHotkeyFile->OpenChild(&desc);
	pHotkeyFile->SetFilePointer(hFile, 8);

	{
		DWORD j, dwLargestString=0;

		for (i = 0; i < dwNumKeys; i++)
		{
			j = strlen(HotKeyList[i].szName) + 1;
			if (j > dwLargestString)
				dwLargestString = j;
		}

		for (i = j = 0; i < dwNumKeys; i++)
		{
			if (i==0 || strcmp(HotKeyList[i].szName, HotKeyList[i-1].szName))
			{
				j++;
				pHotkeyFile->WriteFile(hFile, (byte*)HotKeyList[i].szName, dwLargestString, &dwBytesWritten, 0);
			}
		}
		pHotkeyFile->SetFilePointer(hFile, 0);
		pHotkeyFile->WriteFile(hFile, &j, sizeof(j), &dwBytesWritten, 0);
		pHotkeyFile->WriteFile(hFile, &dwLargestString, sizeof(dwLargestString), &dwBytesWritten, 0);
	}
	pHotkeyFile->CloseHandle(hFile);

	
	desc.lpFileName = "Keyboard text";
	hFile = pHotkeyFile->OpenChild(&desc);
	pHotkeyFile->WriteFile(hFile, &dwNumKeys, sizeof(dwNumKeys), &dwBytesWritten, 0);
	i = sizeof(HotKeyList[i].szKeyCombo);
	pHotkeyFile->WriteFile(hFile, &i, sizeof(i), &dwBytesWritten, 0);
	for (i = 0; i < dwNumKeys; i++)
	 	pHotkeyFile->WriteFile(hFile, (byte*)HotKeyList[i].szKeyCombo, sizeof(HotKeyList[i].szKeyCombo), &dwBytesWritten, 0);
	pHotkeyFile->CloseHandle(hFile);

	
	desc.lpFileName = "Descriptions";
	hFile = pHotkeyFile->OpenChild(&desc);
	pHotkeyFile->WriteFile(hFile, &dwNumKeys, sizeof(dwNumKeys), &dwBytesWritten, 0);
	i = sizeof(HotKeyList[i].szDescription);
	pHotkeyFile->WriteFile(hFile, &i, sizeof(i), &dwBytesWritten, 0);
	for (i = 0; i < dwNumKeys; i++)
	 	pHotkeyFile->WriteFile(hFile, (byte*)HotKeyList[i].szDescription, sizeof(HotKeyList[i].szDescription), &dwBytesWritten, 0);
	pHotkeyFile->CloseHandle(hFile);

	pHotkeyFile->Release();

	{
		DAFILEDESC ndesc = szFilename;

		bUntitled = (DACOM->CreateInstance(&ndesc, (void **) &pHotkeyFile) != GR_OK);
	}


	if (bWriteHeader)
		WriteHeaderFile(szFilename);
	if (HKRECORD::iSortColumn != iOrigSort)
		SortHotKeyList(iOrigSort);
	if (bWriteText)
		WriteTextFile(szFilename);

	
	LoadString(hInstance, IDS_APP_NAME, szBaseEditName, 64);
	strcat(szBaseEditName, " - ");
	if (bUntitled)
		strcat(szBaseEditName, szUntitled);
	else
	{
		strcat(szBaseEditName, szFilename);
		bWinDataChanged=0;
		ClearUndo();
	}
	SetWindowText(hMainWindow, szBaseEditName);

	return 1;

Fail:
	if (bUntitled==0)
	{
		DAFILEDESC ndesc = buffer;
		DACOM->CreateInstance(&ndesc, (void **) &pHotkeyFile);
	}
	return 0;
}
//--------------------------------------------------------------------------//
//
BOOL SaveTmpData (LPFILESYSTEM pFile)
{
	DWORD i, dwBytesWritten;
	int iOrigSort = HKRECORD::iSortColumn;
	HANDLE hFile;
	DAFILEDESC desc;

	desc.dwDesiredAccess |= GENERIC_WRITE;
	desc.dwShareMode = 0;
    desc.dwCreationDistribution = CREATE_ALWAYS;


	if (SortHotKeyList(0)==0)
		return 0;
	if (AssignNumbersToHotKeys()==0)
		return 0;

	desc.lpImplementation = 0;
	desc.lpFileName = "Version";
	hFile = pFile->OpenChild(&desc);
	i = HKEDITOR_VERSION;
	pFile->WriteFile(hFile, &i, sizeof(i), &dwBytesWritten, 0);
	pFile->CloseHandle(hFile);

	desc.lpFileName = "Binary data";
	hFile = pFile->OpenChild(&desc);
	pFile->WriteFile(hFile, &dwNumKeys, sizeof(dwNumKeys), &dwBytesWritten, 0);
	pFile->WriteFile(hFile, &dwRawHotkeySize, sizeof(dwRawHotkeySize), &dwBytesWritten, 0);
	
	for (i = 0; i < dwNumKeys; i++)
	 	pFile->WriteFile(hFile, (byte*)HotKeyList[i].dwKeyCombo, dwRawHotkeySize, &dwBytesWritten, 0);
	pFile->CloseHandle(hFile);
	
	
	desc.lpFileName = "Event messages";
	hFile = pFile->OpenChild(&desc);
	pFile->SetFilePointer(hFile, 8);

	{
		DWORD j=0;

		for (i = 0; i < dwNumKeys; i++)
		{
			if (i==0 || strcmp(HotKeyList[i].szName, HotKeyList[i-1].szName))
			{
				j++;
				pFile->WriteFile(hFile, (byte*)HotKeyList[i].szName, sizeof(HotKeyList[i].szName), &dwBytesWritten, 0);
			}
		}
		pFile->SetFilePointer(hFile, 0);
		pFile->WriteFile(hFile, &j, sizeof(j), &dwBytesWritten, 0);
		j = sizeof(HotKeyList[0].szName);
		pFile->WriteFile(hFile, &j, sizeof(j), &dwBytesWritten, 0);
	}
	pFile->CloseHandle(hFile);

	desc.lpFileName = "Keyboard text";
	hFile = pFile->OpenChild(&desc);
	pFile->WriteFile(hFile, &dwNumKeys, sizeof(dwNumKeys), &dwBytesWritten, 0);
	i = sizeof(HotKeyList[i].szKeyCombo);
	pFile->WriteFile(hFile, &i, sizeof(i), &dwBytesWritten, 0);
	for (i = 0; i < dwNumKeys; i++)
	 	pFile->WriteFile(hFile, (byte*)HotKeyList[i].szKeyCombo, sizeof(HotKeyList[i].szKeyCombo), &dwBytesWritten, 0);
	pFile->CloseHandle(hFile);

	// the other bits are not needed at run-time
	
	if (HKRECORD::iSortColumn != iOrigSort)
		SortHotKeyList(iOrigSort);

	return 1;
}
//--------------------------------------------------------------------------//
//
BOOL ModifyHKRecord (DWORD dwNumber)
{
	if (dwNumber >= dwNumKeys)
		return 0;
	if (HKGROUP::IsBeingEdited(dwNumber))
		return 0;

	if (bUntitled || pHotkeyFile)
	{
		HKGROUP *lpGroup = new HKGROUP;
		HKRECORD hkrec;

		hkrec = HotKeyList[dwNumber];
		return lpGroup->init(hMainWindow, &hkrec, dwNumber, Replace);
	}
	return 0;		
}
//--------------------------------------------------------------------------//
// Create a new record, fill in the data items, add it to the list
//
BOOL InsertHKRecord (DWORD dwNumber)
{
	if (HKGROUP::InsertUnderway())
		return 0;

	if (dwNumKeys >= MAX_HOTKEYS)
	{
	 	WarningBox(hMainWindow, IDS_STRING10, MAX_HOTKEYS);
		return 0;
	}

	if (bUntitled || pHotkeyFile)
	{
		HKGROUP *lpGroup = new HKGROUP;
		HKRECORD hkrec;

		return lpGroup->init(hMainWindow, &hkrec, dwNumber, Insert);
	}
	return 0;
}
//--------------------------------------------------------------------------//
//
BOOL AddSymbolsToListBox (HWND hListbox)
{
	DWORD i;

	for (i = 0; i < dwNumKeys; i++)
	{
		if (i==0 || strcmp(HotKeyList[i].szName, HotKeyList[i-1].szName))
	 		SendMessage(hListbox, CB_ADDSTRING, 0, (LONG) HotKeyList[i].szName);
	}

	return 1;
}
//--------------------------------------------------------------------------//
// perform bubble sort
// 
static BOOL BubbleSortHotKeyList (DWORD dwNum)
{
	DWORD i=0;
	BOOL result=0;
	HKRECORD tmp;
	
	dwNum--;
	while (i < dwNum)
	{
		if (HotKeyList[i] > HotKeyList[i+1])
		{
			//switch them
			tmp = HotKeyList[i];
			HotKeyList[i] = HotKeyList[i+1];
			HotKeyList[i+1] = tmp;
			
			result=1;
		}
		i++;
	}

	return result;
}
//--------------------------------------------------------------------------//
// 
BOOL SortHotKeyList (int iSortColumn)
{
	DWORD dwSortKeys;
	if (HKGROUP::AnyEditUnderway())
		return 0;
	if ((dwSortKeys = dwNumKeys) < 2)
		return 1;

	HKRECORD::iSortColumn = iSortColumn;

	while (dwSortKeys>1 && BubbleSortHotKeyList(dwSortKeys))
		dwSortKeys--;	
	
	
	if (dwSortKeys < dwNumKeys)
	{
		InitializeListView();
		ClearUndo();
	}

	return 1;
}
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
#define print_line(x)    	\
	if (x[0])				\
		pFile->WriteFile(0, (byte *) x, strlen((char *) x), &dwBytesWritten, 0)

char comment_line[] = "//------------------------------------------------------------------------//\r\n";
char comment_stub[] = "//\r\n";
char small_comment[] = "\r\n\t//----------------------------------";
char new_line[] = "\r\n";
char define_line[] = "#define ";

char form_line[] = "FORM HOTK\r\n{";
char chunk_line[] = "\r\n\tCHUNK ";
char begin_chunk[] = "\r\n\t{";
char end_chunk_line[] = "\r\n\t}";
char end_form_line[] = "\r\n}";
char long_format[] = "\r\n\t\tlong %3d";
char plain_char_format[] = "\r\n\t\tchar\t";
char name_char_format[] = "\r\n\t\tchar[80]    \"";
char combo_char_format[] = "\r\n\t\tchar[128]   \"";
char desc_char_format[] = "\r\n\t\tchar[256]   \"";

//--------------------------------------------------------------------------//
//
BOOL WriteTextFile (char *szName)
{
	char buffer[128];
	char buffer2[128];
	char *tmp;
	LPFILESYSTEM pFile;
	DWORD i, dwBytesWritten;
	int iMaxNameLength, iMaxComboLength, j;
	DAFILEDESC desc = buffer2;

	strcpy(buffer, szName);
	if ((tmp = strchr(buffer, '.')) != 0)
	 	*tmp = 0;

	// buffer contains the base name of the file

	strcpy(buffer2, buffer);
	strcat(buffer2, ".txt");

	desc.lpImplementation = 0;
	desc.dwDesiredAccess |= GENERIC_WRITE;
	desc.dwShareMode = 0;
    desc.dwCreationDistribution = CREATE_ALWAYS;

 	if (DACOM->CreateInstance(&desc, (void **)&pFile) != GR_OK)
		return 0;
	
	print_line(comment_line);
	print_line(comment_stub);
	strcpy(buffer2,"//   Text file generated by the HotKey editor (jy)\r\n");
	print_line(buffer2);
	wsprintf(buffer2, "//   Original data file: %s\r\n", szName);
	print_line(buffer2);
	print_line(comment_stub);
	print_line(comment_line);
	print_line(new_line);

	// find the longest symbol name
	iMaxNameLength = 0;
	for (i = 0; i < dwNumKeys; i++)
	{
		j = strlen(HotKeyList[i].szName);
	 	if (j > iMaxNameLength)
			iMaxNameLength = j;
	}
	iMaxNameLength = __max(iMaxNameLength+4, 20);

	// find the longest combo name
	iMaxComboLength = 0;
	for (i = 0; i < dwNumKeys; i++)
	{
		j = strlen(HotKeyList[i].szKeyCombo);
	 	if (j > iMaxComboLength)
			iMaxComboLength = j;
	}
	iMaxComboLength = __max(iMaxComboLength+4, 20);


	strcpy(buffer, "SYMBOL NAME");
	print_line(buffer);
	j = iMaxNameLength - strlen(buffer);
	while (j--)
		pFile->WriteFile(0, (byte *)" ", 1, &dwBytesWritten, 0);
	strcpy(buffer, "KEY COMBO");
	print_line(buffer);
	j = iMaxComboLength - strlen(buffer);
	while (j--)
		pFile->WriteFile(0, (byte *)" ", 1, &dwBytesWritten, 0);
	strcpy(buffer, "DESCRIPTION");
	print_line(buffer);
	strcpy(buffer, "\r\n-----------");
	print_line(buffer);
	j = iMaxNameLength - strlen(buffer)+2;
	while (j--)
		pFile->WriteFile(0, (byte *)" ", 1, &dwBytesWritten, 0);
	strcpy(buffer, "---------");
	print_line(buffer);
	j = iMaxComboLength - strlen(buffer);
	while (j--)
		pFile->WriteFile(0, (byte *)" ", 1, &dwBytesWritten, 0);
	strcpy(buffer, "-----------\r\n");
	print_line(buffer);

	for (i = 0; i < dwNumKeys; i++)
	{
 		print_line(HotKeyList[i].szName);
		j = iMaxNameLength - strlen(HotKeyList[i].szName);
		while (j--)
			pFile->WriteFile(0, (byte *)" ", 1, &dwBytesWritten, 0);
 		print_line(HotKeyList[i].szKeyCombo);
		j = iMaxComboLength - strlen(HotKeyList[i].szKeyCombo);
		while (j--)
			pFile->WriteFile(0, (byte *)" ", 1, &dwBytesWritten, 0);
 		print_line(HotKeyList[i].szDescription);
		print_line(new_line);
	}

	pFile->Release();
	return 1;
}
//--------------------------------------------------------------------------//
//
BOOL WriteHeaderFile (char *szName)
{
	char buffer[128];
	char buffer2[128];
	char *tmp;
	LPFILESYSTEM pFile;
	DWORD i, dwBytesWritten;
	int iMaxNameLength, j;
	DAFILEDESC desc = buffer2;

	strcpy(buffer, szName);
	if ((tmp = strchr(buffer, '.')) != 0)
	 	*tmp = 0;

	// buffer contains the base name of the file

	strcpy(buffer2, buffer);
	strcat(buffer2, ".h");

	desc.lpImplementation = 0;
	desc.dwDesiredAccess |= GENERIC_WRITE;
	desc.dwShareMode = 0;
    desc.dwCreationDistribution = CREATE_ALWAYS;

 	if (DACOM->CreateInstance(&desc, (void **)&pFile) != GR_OK)
		return 0;

	if ((tmp = strrchr(buffer, '\\')) != 0)
	 	tmp++;
	else
		tmp = buffer;

	print_line("#ifndef ");
	strupr(tmp);
	print_line(tmp);
	print_line("_H\r\n#define ");
	print_line(tmp);
	print_line("_H\r\n");

	print_line(comment_line);
	print_line("/*\r\n");
	strcpy(buffer2,"\tHeader file generated by the HotKey editor (jy)\r\n");
	print_line(buffer2);
	wsprintf(buffer2, "\tOriginal data file: %s\r\n", szName);
	print_line(buffer2);
	print_line(new_line);
	print_line("\t$Header: /Libs/dev/Src/Tools/HotkeyEditor/Hklists.cpp 10    3/21/00 4:30p Pbleisch $\r\n");
	print_line("*/\r\n");

	print_line(comment_line);
	print_line(new_line);

	// find the longest symbol name
	iMaxNameLength = 0;
	for (i = 0; i < dwNumKeys; i++)
	{
		j = strlen(HotKeyList[i].szName);
	 	if (j > iMaxNameLength)
			iMaxNameLength = j;
	}
	iMaxNameLength = __max(iMaxNameLength+4, 40);

	print_line("#ifdef _ADB\r\n\r\n");

	print_line("namespace ");
	print_line(tmp);
	print_line("\r\n{\r\nenum HOTKEY\r\n{\r\n\tNONE");

	for (i = 0; i < dwNumKeys; i++)
	{
		if (i && HotKeyList[i].dwAssignedNumber == HotKeyList[i-1].dwAssignedNumber)
			continue;
		print_line(",\r\n\t");
		print_line(HotKeyList[i].szName);
	}

	print_line("\r\n}; // enum HOTKEY\r\n");
	print_line("}  // namespace\r\n");

	print_line("\r\n#else  // !_ADB\r\n\r\n");

	for (i = 0; i < dwNumKeys; i++)
	{
		if (i && HotKeyList[i].dwAssignedNumber == HotKeyList[i-1].dwAssignedNumber)
			continue;
		print_line("#define ");
		print_line(HotKeyList[i].szName);
		j = iMaxNameLength - strlen(HotKeyList[i].szName);
		while (j--)
			pFile->WriteFile(0, (byte *)" ", 1, &dwBytesWritten, 0);
		wsprintf(buffer, "0x%08X\r\n", HotKeyList[i].dwAssignedNumber);
		print_line(buffer);
	}

	print_line("\r\n#endif      // _ADB \r\n");

	print_line(new_line);
	print_line("#endif\r\n");

	pFile->Release();
	return 1;
}
//--------------------------------------------------------------------------//
//
BOOL StartTest (void)
{
	char buffer[64];
	int iOrigSort = HKRECORD::iSortColumn;

	if (SortHotKeyList(0)==0)
		return 0;
	if (AssignNumbersToHotKeys()==0)
		return 0;

	if (hTestWindow)
	{
		DestroyWindow(hTestWindow);
		hTestWindow=0;
	}

	LoadString(hInstance, IDS_TEST_NAME, buffer, sizeof(buffer));
	CreateWindow(szAppName, buffer, WS_THICKFRAME | WS_OVERLAPPED | WS_CAPTION,
						CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, 
						hMainWindow, 0, hInstance, 0);

	if (iOrigSort != HKRECORD::iSortColumn)
		SortHotKeyList(iOrigSort);

	memset(pNameArray, 0, sizeof(pNameArray));
	iNameArrayHead=0;

	ShowWindow(hTestWindow, SW_SHOW);

	return (hTestWindow!=0);
}

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
struct DACOM_NO_VTABLE EventCallback : public IEventCallback
{
	BEGIN_DACOM_MAP_INBOUND(EventCallback)
	DACOM_INTERFACE_ENTRY(IEventCallback)
	DACOM_INTERFACE_ENTRY2(IID_IEventCallback,IEventCallback)
	END_DACOM_MAP()


	DEFMETHOD(Notify) (U32 message, void *param = 0);
};


//--------------------------------------------------------------------------//
//
GENRESULT EventCallback::Notify (U32 message, void *param)
{
	if (message == HOTKEY_EVENT)
	{
		DWORD i;

		for (i = 0; i < dwNumKeys; i++)
		{
			if (HotKeyList[i].dwAssignedNumber == (DWORD) param)
			{
				pNameArray[iNameArrayHead] = HotKeyList[i].szName;
				if (++iNameArrayHead >= NAME_ARRAY_SIZE)
					iNameArrayHead = 0;

				pNameArray[iNameArrayHead] = 0;		// leave a blank space
			
				InvalidateRect(hTestWindow, 0, 1);
				break;
			}
		}
	}

	return GR_OK;
}

//--------------------------------------------------------------------------//
//
BOOL InitEventLibrary (HWND hwnd)
{
	char buffer[260];
	char path[MAX_PATH+4];
	DAFILEDESC desc = buffer;
	HKEVENTDESC hkdesc;
	COMPTR<IFileSystem> pFile;
	COMPTR<IDAConnectionPoint> connection;
	U32 handle;

	desc.lpImplementation = "UTF";
	desc.dwDesiredAccess |= GENERIC_WRITE;
	desc.dwShareMode = 0;
    desc.dwCreationDistribution = CREATE_ALWAYS;
	desc.dwFlagsAndAttributes = FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE;

	GetTempPath(MAX_PATH, path);	
	if (GetTempFileName(path, (const char *)"HOT", 0, buffer) == 0)
		return 0;

	if (DACOM->CreateInstance(&desc, (void **) &pFile) != GR_OK)
		goto Fail;
	
	if (SaveTmpData(pFile) == 0)
		goto Fail;

	hkdesc.file = pFile;
	hkdesc.hotkeyMessage = HOTKEY_EVENT;
	hkdesc.joyMessage = JOY_EVENT;

	if (DACOM->CreateInstance(&hkdesc, HotkeyEvent) != GR_OK)
		goto Fail;

	if (HotkeyEvent->QueryOutgoingInterface("IEventCallback", connection) != GR_OK)
		goto Fail;
	
	if (event==0 && (event.ptr = new DAComponent<EventCallback>) == 0)
		goto Fail;

	if (connection->Advise(event, &handle) != GR_OK)
		goto Fail;

//	joySetCapture(hwnd, JOYSTICKID1, 100, 1);

	return 1;
Fail:
	FatalwID(IDS_STRING13);
	return 0;
}
//--------------------------------------------------------------------------//
//
BOOL TestWindowUpdate (void)
{
// testing!!!
/*
	DWORD i, dwEvent;
 	if (hTestWindow==0)
		return 0;

	HKEVENT::Update();

	for (i = 0; (dwEvent = HKEVENT::GetHotkeyEvent(i)) != 0xFFFFFFFF; i++)
	{
	 	iNameArray[iNameArrayHead] = dwEvent;
		if (++iNameArrayHead >= NAME_ARRAY_SIZE)
			iNameArrayHead = 0;
	}
	iNameArray[iNameArrayHead] = -1;		// leave a blank space

	if (i)	// if at least one event occurred
		InvalidateRect(hTestWindow, 0, 1);

*/
	if (hTestWindow==0 || HotkeyEvent==0)
		return 0;

	JOYINFOEX joyinfo;
	
	joyinfo.dwSize = sizeof(joyinfo);
	joyinfo.dwFlags =  JOY_RETURNBUTTONS | JOY_RETURNPOV | JOY_RETURNX | JOY_RETURNY;

	if (joyGetPosEx(JOYSTICKID1, &joyinfo) == JOYERR_NOERROR)
	{
		HotkeyEvent->SystemMessage((S32)hTestWindow, JOY_EVENT, 0, (LONG)&joyinfo);
	}

	return (bGlobalRecording = 1);
}
//--------------------------------------------------------------------------//
//
BOOL PaintTestWindow (HDC hdc)
{
	TEXTMETRIC tm;			// tmHeight
	int i, y=0;

	GetTextMetrics(hdc, &tm);

 	for (i = 0; i < NAME_ARRAY_SIZE; i++)
	{
		if (pNameArray[i])
		{
			TextOut(hdc, 0, y, pNameArray[i], strlen(pNameArray[i]));
		}
		y += tm.tmHeight;
	}
	return 1;
}
//--------------------------------------------------------------------------//
//
LONG CALLBACK TestWndProc (HWND hwnd, UINT message, UINT wParam, LONG lParam)
{
	PAINTSTRUCT	ps;
	HDC hdc;

	if (HotkeyEvent)
		HotkeyEvent->SystemMessage((S32)hwnd, message, wParam, lParam);

	switch (message)
	{
		case WM_CREATE:
			SetTestWindowSize(hwnd);
			if (InitEventLibrary(hwnd)==0)
				SendMessage(hwnd, WM_CLOSE, 0, 0);
			else
				hTestWindow=hwnd;
			bGlobalRecording=1;
			break;

		case WM_KILLFOCUS:
			SendMessage(hwnd, WM_CLOSE, 0, 0);
			break;
		
 		case WM_PAINT:
			hdc = BeginPaint(hwnd, &ps);
			PaintTestWindow(hdc);
			EndPaint(hwnd, &ps);
			return 0;

		case WM_CLOSE:
			HotkeyEvent.free();
			event.free();
			break;

		case WM_DESTROY:
			SetDefaults();
			hTestWindow=0;
			break;
	}

	return DefWindowProc (hwnd, message, wParam, lParam);
}
//--------------------------------------------------------------------------//
//
LONG CALLBACK RecordWndProc (HWND hwnd, UINT message, UINT wParam, LONG lParam)
{
	PAINTSTRUCT	ps;
	HDC hdc;

	switch (message)
	{
		case WM_CREATE:
			{
				CREATESTRUCT * cs = (CREATESTRUCT *) lParam;
				SetWindowLong(hwnd, GWL_USERDATA, (LONG) cs->lpCreateParams); 
			}
			break;

		case WM_KILLFOCUS:
			PostMessage((HWND)GetWindowLong(hwnd, GWL_USERDATA), MSG_END_REC, 0, 0);
			break;
		case WM_SETFOCUS:
			PostMessage((HWND)GetWindowLong(hwnd, GWL_USERDATA), MSG_BEGIN_REC, 0, 0);
			break;
		
 		case WM_PAINT:
			hdc = BeginPaint(hwnd, &ps);
			EndPaint(hwnd, &ps);
			return 0;

		case WM_CLOSE:
			PostMessage((HWND)GetWindowLong(hwnd, GWL_USERDATA), MSG_END_REC, 0, 0);
			return 0;		// do not allow closing of this window

		case WM_DESTROY:
			break;
	}

	return DefWindowProc (hwnd, message, wParam, lParam);
}

//--------------------------------------------------------------------------//
//----------------------------END HKLists.cpp------------------------------//
//--------------------------------------------------------------------------//
