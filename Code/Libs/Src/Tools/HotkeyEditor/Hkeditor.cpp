//--------------------------------------------------------------------------//
//                                                                          //
//                               HKEditor.cpp                               //
//                                                                          //
//                  COPYRIGHT (C) 1996 BY ORIGIN SYSTEMS, INC.              //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Logfile: /Libs/dev/Src/Tools/HotkeyEditor/Hkeditor.cpp $

    $Revision: 5 $

    $Date: 3/16/00 4:15p $

    $Author: Pbleisch $
*/			    
//--------------------------------------------------------------------------//

#include <windows.h>
#include <commctrl.h>               // Common controls

#include "HKGroup.h"
#include "HKEvent.h"

#include "ferror.h"
#include <DACOM.h>
#include <FileSys.h>
#include <HeapObj.h>

#include "resource.h"

//--------------------------------------------------------------------------//
//-------------------------------GLOBALS------------------------------------//
//--------------------------------------------------------------------------//
HACCEL hAccel;
typedef unsigned char UBYTE;
extern "C" void (*ErrorHandler) (char *fmt, ...) = Fatal;
BOOL CALLBACK MainDlgProc (HWND hwnd, UINT message, UINT wParam, LONG lParam);
LONG CALLBACK TestWndProc (HWND hwnd, UINT message, UINT wParam, LONG lParam);
LONG CALLBACK RecordWndProc (HWND hwnd, UINT message, UINT wParam, LONG lParam);
BOOL poll_window (void);
BOOL bHasFocus = 1;
extern BOOL bGlobalRecording;
BOOL bExitMessageReceived = 0;
HFONT hFont=0;
HWND hListView=0;
BOOL bWinDataChanged = 0;
BOOL bUntitled=0;
char szBaseEditName[256];
LPFILESYSTEM pHotkeyFile=0;
HMENU hMainMenu;
HICON hIcon;
char szUntitled[40];

BOOL OpenHotKeyFile (char *szFilename);
BOOL GetInputFilename (char *szFilename, long iResource, HWND hwnd);
BOOL GetOutputFilename (char *szFilename, long iResource, HWND hwnd);
int QuerySaveFile (void);
BOOL SaveHotKeyData (char *szFilename);
void ClearUndo (void);
void ClearHotKeyList (void);
BOOL DeleteHKRecord (DWORD dwNumber, BOOL bUpdateUndo=1);
BOOL ModifyHKRecord (DWORD dwNumber);
BOOL InsertHKRecord (DWORD dwNumber);
BOOL ProcessUndo (void);
BOOL SortHotKeyList (int iSortColumn);
BOOL SetDefaultsInRegistry (void);
BOOL GetDefaultsFromRegistry (void);
BOOL StartTest (void);		// start test window
BOOL TestWindowUpdate (void);
extern BOOL bWriteHeader;
extern BOOL bWriteText;
extern HWND hTestWindow;

ICOManager * DACOM = 0;


struct USER_DEFAULTS
{
	long  dwSize;
	long  version;
	long  iMainX, iMainY;
	long  iMainWidth, iMainHeight;
	long  iTestX, iTestY;
	long  iTestWidth, iTestHeight;
	long  showState;
	byte  bOnTop:1;
	byte  bWriteText:1;
	byte  bWriteHeader:1;

	USER_DEFAULTS (void)
	{
	 	init();
	}
	void init (void)
	{
	 	memset(this, 0, sizeof(*this));
		dwSize = sizeof(*this);
		version = 0;
		showState = SW_SHOW;
	}
};

USER_DEFAULTS user_defaults;

//--------------------------------------------------------------------
//--------------------------CLEAN UP STUFF---------------------------
//--------------------------------------------------------------------
//--------------------------------------------------------------------------;
//
static BOOL cleaned_up = 0;
static void clean_up(void)
{
	if (!cleaned_up)
	{
		cleaned_up = 1;
		DACOM->ShutDown();
	}
}
//--------------------------------------------------------------------
//
BOOL SetTestWindowSize (HWND hwnd)
{
	if (user_defaults.iTestWidth==0)
		return 0;
	return MoveWindow (hwnd, user_defaults.iTestX, user_defaults.iTestY, 
			user_defaults.iTestWidth, user_defaults.iTestHeight , FALSE);
}
//--------------------------------------------------------------------
//
void SetDefaults (void)
{
	WINDOWPLACEMENT winplace;

	winplace.length = sizeof(winplace);

	if (GetWindowPlacement(hMainWindow, &winplace))
	{
		user_defaults.iMainX = winplace.rcNormalPosition.left;
		user_defaults.iMainY = winplace.rcNormalPosition.top;
		user_defaults.iMainWidth = winplace.rcNormalPosition.right - winplace.rcNormalPosition.left;
		user_defaults.iMainHeight = winplace.rcNormalPosition.bottom - winplace.rcNormalPosition.top;
		user_defaults.showState = winplace.showCmd;
	}

	if (hTestWindow && GetWindowPlacement(hTestWindow, &winplace))
	{
		user_defaults.iTestX = winplace.rcNormalPosition.left;
		user_defaults.iTestY = winplace.rcNormalPosition.top;
		user_defaults.iTestWidth = winplace.rcNormalPosition.right - winplace.rcNormalPosition.left;
		user_defaults.iTestHeight = winplace.rcNormalPosition.bottom - winplace.rcNormalPosition.top;
	}

	user_defaults.bOnTop       = (GetMenuState(hMainMenu, IDM_ALWAYS_ON_TOP, MF_BYCOMMAND) == MF_CHECKED);
	user_defaults.bWriteText   = (GetMenuState(hMainMenu, IDM_WRITE_TEXT, MF_BYCOMMAND) == MF_CHECKED);
	user_defaults.bWriteHeader = (GetMenuState(hMainMenu, IDM_WRITE_HEADER, MF_BYCOMMAND) == MF_CHECKED);
}
//--------------------------------------------------------------------
//
ATOM RegisterTestClass (void)
{
	WNDCLASS    wndclass ;

	wndclass.lpfnWndProc   = TestWndProc;
	wndclass.style         = CS_HREDRAW | CS_VREDRAW;
	wndclass.cbClsExtra    = 0;
	wndclass.cbWndExtra    = 0;
	wndclass.hInstance     = hInstance;
	wndclass.hIcon         = hIcon;
	wndclass.lpszClassName = szAppName;
	wndclass.hCursor = LoadCursor (NULL, IDC_ARROW);
	wndclass.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
	wndclass.lpszMenuName  = NULL ;
	
	return RegisterClass (&wndclass);
}
//--------------------------------------------------------------------
//
ATOM RegisterRecClass (void)
{
	WNDCLASS    wndclass ;
	strcpy(szRecName, szAppName);
	strcat(szRecName, "REC");

	wndclass.lpfnWndProc   = RecordWndProc;
	wndclass.style         = CS_HREDRAW | CS_VREDRAW;
	wndclass.cbClsExtra    = 0;
	wndclass.cbWndExtra    = 0;
	wndclass.hInstance     = hInstance;
	wndclass.hIcon         = hIcon;
	wndclass.lpszClassName = szRecName;
	wndclass.hCursor = LoadCursor (NULL, IDC_ARROW);
	wndclass.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
	wndclass.lpszMenuName  = NULL ;
	
	return RegisterClass (&wndclass);
}
//--------------------------------------------------------------------------
//
inline HANDLE CreateTempFile (char tempname[MAX_PATH+4])
{
	char path[MAX_PATH+4];
	DWORD dwPrefix = '\0toh';

	GetTempPath(MAX_PATH, path);	

	if (GetTempFileName(path, (const char *)&dwPrefix, 0, tempname) == 0)
		return 0;

 	return ::CreateFile(tempname, 
							GENERIC_READ|GENERIC_WRITE,
				            FILE_SHARE_READ,
					        0,
	                        OPEN_EXISTING,
	                        FILE_ATTRIBUTE_NORMAL,
						    0);
}
//--------------------------------------------------------------------------//
// build a temporary ini file, pass it to DACOM
//
char szINIData[] = "[Libraries]\r\nDOSFILE.dll\r\nDAHotkey.dll";
void start_dacom (void)
{
	DACOM->SetINIConfig(szINIData, DACOM_INI_STRING);
}
//--------------------------------------------------------------------
//
int CALLBACK WinMain (HINSTANCE _hInstance, HINSTANCE hPrevInstance, LPSTR lpszCmdLine, int nCmdShow)
{
	hInstance =  _hInstance;

	DACOM = DACOM_Acquire();
	start_dacom();
	atexit(clean_up);

	LoadString(hInstance, IDS_APP_NAME, szAppName, 40);
	LoadString(hInstance, IDS_UNTITLED, szUntitled, 40);
	InitCommonControls();  	// This MUST be called once per instance
                          	// to register the common controls
	GetDefaultsFromRegistry();
	hAccel = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDR_ACCELERATOR1));
	hIcon = LoadIcon (hInstance, MAKEINTRESOURCE(IDI_ICON1)) ;
	RegisterTestClass();
	RegisterRecClass();

	if ((hMainWindow = CreateDialog (hInstance, MAKEINTRESOURCE(IDD_DIALOG1), 0, MainDlgProc)) == 0)
	{
		Fatal("Could not load dialog");
		return 0;
	}

	SetClassLong(hMainWindow, GCL_HICON, (LONG) hIcon);
	CheckMenuItem(hMainMenu, IDM_ALWAYS_ON_TOP, MF_BYCOMMAND | ((user_defaults.bOnTop)?MF_CHECKED:MF_UNCHECKED));
	CheckMenuItem(hMainMenu, IDM_WRITE_TEXT, MF_BYCOMMAND | ((user_defaults.bWriteText)?MF_CHECKED:MF_UNCHECKED));
	CheckMenuItem(hMainMenu, IDM_WRITE_HEADER, MF_BYCOMMAND | ((user_defaults.bWriteHeader)?MF_CHECKED:MF_UNCHECKED));

	bWriteText=user_defaults.bWriteText;
	bWriteHeader=user_defaults.bWriteHeader;

	while (poll_window())
	{
		HKGROUP::Update();
		TestWindowUpdate();
	}

	SetDefaultsInRegistry();
	return 0;
}



//----------------------------------------------------------------------------
//
BOOL SetListViewHeader (HWND hwnd)
{
	LV_COLUMN lvc;
	char buffer[64];
	int iExtra;
	
	strcpy(buffer, "      ");
	iExtra = ListView_GetStringWidth(hListView, buffer);

	ListView_SetExtendedListViewStyleEx(hListView, LVS_EX_FULLROWSELECT, LVS_EX_FULLROWSELECT);
	// Initialize the LV_COLUMN structure.

	lvc.mask    = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
	lvc.fmt     = LVCFMT_LEFT;
	lvc.cx      = 75;

	// Add the columns.

	lvc.iSubItem = 0;		// which subitem to display
	LoadString(hInstance, IDS_IDNAME, buffer, 64);
	lvc.pszText  = buffer;
	lvc.cx = (3 * (ListView_GetStringWidth(hListView, buffer) + iExtra)) / 2;

	if (-1 == ListView_InsertColumn(hListView, 0, &lvc))
		return FALSE;

	lvc.iSubItem = 1;
	LoadString(hInstance, IDS_KEYCOMBO, buffer, 64);
	lvc.pszText = buffer;
	lvc.cx = ListView_GetStringWidth(hListView, buffer) + iExtra;
	if (-1 == ListView_InsertColumn(hListView, 1, &lvc))
		return FALSE;

	lvc.iSubItem = 2;
	LoadString(hInstance, IDS_DESCRIPTION, buffer, 64);
	lvc.pszText = buffer;
	lvc.cx = ListView_GetStringWidth(hListView, buffer) + iExtra;
	if (-1 == ListView_InsertColumn(hListView, 2, &lvc))
		return FALSE;

	return TRUE;
}
//----------------------------------------------------------------------------
//
static WNDPROC lpfnOldListProcedure = 0;

LONG CALLBACK ListControlProcedure(HWND hwnd, UINT message, UINT wParam, LONG lParam)
{
	switch (message)
	{
		case WM_CHAR:
			break;
		case WM_KEYDOWN:
			switch (LOWORD(wParam))
			{
				case VK_RETURN:
					PostMessage(GetParent(hwnd), WM_COMMAND, IDM_MODIFY, (LONG)hwnd);	// send message to parent
					break;
				case VK_INSERT:
					PostMessage(GetParent(hwnd), WM_COMMAND, IDM_INSERT, (LONG)hwnd);	// send message to parent
					break;
				case VK_DELETE:
					PostMessage(GetParent(hwnd), WM_COMMAND, IDM_DELETE, (LONG)hwnd);	// send message to parent
					break;
			}
			break;
		case WM_LBUTTONDBLCLK:
			PostMessage(GetParent(hwnd), WM_COMMAND, IDM_MODIFY, (LONG)hwnd);	// send message to parent
			break;
	}

	return CallWindowProc(lpfnOldListProcedure, hwnd, message, wParam, lParam);
}
//--------------------------------------------------------------------
//
char temp_filename[80];
BOOL CALLBACK MainDlgProc (HWND hwnd, UINT message, UINT wParam, LONG lParam)
{
	unsigned wWidth, wHeight;
	int i;

	switch (message)
	{
		case WM_ACTIVATEAPP:
			bHasFocus = (wParam!=0);
			break;

		case WM_SETFONT:
			hFont = (HFONT) wParam;
			break;

		case WM_INITDIALOG:
			bWinDataChanged = 0;
			hListView = GetDlgItem(hwnd, IDC_LISTVIEW);
			if (hListView)
			{
				HFONT hFont = (HFONT) GetStockObject(DEFAULT_GUI_FONT);
				SendMessage(hListView, WM_SETFONT, (UINT)hFont, 0);
				SetListViewHeader(hwnd);
			}
			hMainMenu = GetMenu(hwnd);

			if ((lpfnOldListProcedure = (WNDPROC) GetWindowLong(hListView, GWL_WNDPROC)) != 0)
				SetWindowLong(hListView, GWL_WNDPROC, (LONG) ListControlProcedure);
			if (user_defaults.iMainWidth)		// we have previous data
				SetWindowPos(hwnd, HWND_TOPMOST,
							 user_defaults.iMainX, user_defaults.iMainY,
							 user_defaults.iMainWidth, user_defaults.iMainHeight,
							 (user_defaults.bOnTop)?0:SWP_NOZORDER);
			ShowWindow(hwnd, user_defaults.showState);
			return 1;

		case WM_SIZE:
			wWidth = LOWORD(lParam);
			wHeight = HIWORD(lParam);
			if (hListView)
			{
				MoveWindow (hListView, 0, 0, wWidth, wHeight , TRUE);

				i = ListView_GetColumnWidth(hListView, 0) + 
				 	ListView_GetColumnWidth(hListView, 1); 
				i = wWidth - i - GetSystemMetrics(SM_CXVSCROLL)*2;
				if (i > 20)
					ListView_SetColumnWidth(hListView, 2, i);
			}
			return 0;

		case WM_DESTROY:
			PostQuitMessage (0);
			return 0;

		case WM_CLOSE:
			if (SendMessage(hwnd, WM_COMMAND, IDM_CLOSE, 0))
			{
				SetDefaults();
				bExitMessageReceived = 1;
				return 0;
			}
			return 1;

		case WM_NOTIFY:
		{
			NM_LISTVIEW *pNM = (NM_LISTVIEW *) lParam;

			switch (pNM->hdr.code)
			{
				case LVN_COLUMNCLICK:
					SetWindowLong(hwnd, DWL_MSGRESULT, SortHotKeyList(pNM->iSubItem));	
					break;
			}
		}
		break;

		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
				case IDM_NEW:
					if (SendMessage(hwnd, WM_COMMAND, IDM_CLOSE, 0))
						OpenHotKeyFile(0);
					break;
				case IDM_OPEN:
					if (GetInputFilename(temp_filename, IDS_FILTERSTRING, hwnd))
					{
						if (SendMessage(hwnd, WM_COMMAND, IDM_CLOSE, 0))
						{
							OpenHotKeyFile(temp_filename);
						}
					}
					break;
				case IDM_CLOSE:
					if (pHotkeyFile || bUntitled)
					{
						if (bWinDataChanged && (i=QuerySaveFile())!=0)
						{
							if (i!=1 || SendMessage(hwnd, WM_COMMAND, IDM_SAVE, 0)==0)
							{
								SetWindowLong(hwnd, DWL_MSGRESULT, 0);	
								return 1;
							}
						}
						
						bWinDataChanged=0;
						if (pHotkeyFile)
						{
							pHotkeyFile->Release();
							pHotkeyFile=0;
						}
						LoadString(hInstance, IDS_APP_NAME, szBaseEditName, 64);
						SetWindowText(hwnd, szBaseEditName);
						EnableWindow(hListView, 0);
						EnableMenuItem(hMainMenu, IDM_SAVE, MF_GRAYED);
						EnableMenuItem(hMainMenu, IDM_SAVEAS, MF_GRAYED);
						EnableMenuItem(hMainMenu, IDM_CLOSE, MF_GRAYED);
						EnableMenuItem(hMainMenu, 1, MF_BYPOSITION | MF_GRAYED);
						EnableMenuItem(hMainMenu, 3, MF_BYPOSITION | MF_GRAYED);
						DrawMenuBar(hwnd);
						ListView_DeleteAllItems(hListView);
						ClearUndo();
						ClearHotKeyList();
						bUntitled = 0;
					}
					SetWindowLong(hwnd, DWL_MSGRESULT, 1);		// success
					return 1;

				case IDM_SAVE:
					if (pHotkeyFile || bUntitled)
					{
						if (bUntitled)
						{
							SetWindowLong(hwnd, DWL_MSGRESULT, SendMessage(hwnd, WM_COMMAND, IDM_SAVEAS, 0));
							return 1;
						}
						char buffer[256];
						pHotkeyFile->GetFileName(buffer, sizeof(buffer));
						if (HKGROUP::AnyEditUnderway()==0 && SaveHotKeyData(buffer))
							SetWindowLong(hwnd, DWL_MSGRESULT, 1);		// success
						else
							SetWindowLong(hwnd, DWL_MSGRESULT, 0);		
						return 1;
					}
					SetWindowLong(hwnd, DWL_MSGRESULT, 1);		// success
					return 1;
					
				case IDM_SAVEAS:
					if (pHotkeyFile || bUntitled)
					{
						if (HKGROUP::AnyEditUnderway()==0 && 
							GetOutputFilename(temp_filename, IDS_FILTERSTRING, hwnd) &&
						    SaveHotKeyData(temp_filename))
						{
						 	bUntitled = 0;
							SetWindowLong(hwnd, DWL_MSGRESULT, 1);		// success
						}
						else
							SetWindowLong(hwnd, DWL_MSGRESULT, 0);		// failure
					}
					else
						SetWindowLong(hwnd, DWL_MSGRESULT, 1);		// success
					return 1;

				case IDM_MODIFY:
					if ((i = ListView_GetNextItem(hListView, -1, LVNI_SELECTED)) >= 0)
					{
						SetWindowLong(hwnd, DWL_MSGRESULT, ModifyHKRecord(i));
						return 1;
					}
					break;
				case IDM_INSERT:
					if ((i = ListView_GetNextItem(hListView, -1, LVNI_SELECTED)) < 0)
						i = 0;
					SetWindowLong(hwnd, DWL_MSGRESULT, InsertHKRecord(i));
					return 1;

				case IDM_DELETE:
					if ((i = ListView_GetNextItem(hListView, -1, LVNI_SELECTED)) >= 0)
					{
						SetWindowLong(hwnd, DWL_MSGRESULT, DeleteHKRecord(i));
						return 1;
					}
					break;
				case IDM_UNDO:
					SetWindowLong(hwnd, DWL_MSGRESULT, ProcessUndo());
					return 1;

				case IDOK:
					PostMessage(hwnd, WM_COMMAND, IDM_MODIFY, (LONG)lParam);	// send message to parent
					break;
		
				case IDM_TEST:
					StartTest();
					break;

				case IDM_EXIT:
					SendMessage(hwnd, WM_CLOSE, 0, 0);
					break;

				case IDM_ALWAYS_ON_TOP:
					if (GetMenuState(hMainMenu, IDM_ALWAYS_ON_TOP, MF_BYCOMMAND) != MF_CHECKED)
					{
						SetWindowPos(hMainWindow, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
						CheckMenuItem(hMainMenu, IDM_ALWAYS_ON_TOP, MF_BYCOMMAND | MF_CHECKED);
					}
					else
					{
						SetWindowPos(hMainWindow, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
						CheckMenuItem(hMainMenu, IDM_ALWAYS_ON_TOP, MF_BYCOMMAND | MF_UNCHECKED);
					}
					break;

				case IDM_WRITE_TEXT:
					if (GetMenuState(hMainMenu, IDM_WRITE_TEXT, MF_BYCOMMAND) != MF_CHECKED)
					{
						CheckMenuItem(hMainMenu, IDM_WRITE_TEXT, MF_BYCOMMAND | MF_CHECKED);
					 	bWriteText=1;
					}
					else
					{
						CheckMenuItem(hMainMenu, IDM_WRITE_TEXT, MF_BYCOMMAND | MF_UNCHECKED);
					 	bWriteText=0;
					}
					break;

				case IDM_WRITE_HEADER:
					if (GetMenuState(hMainMenu, IDM_WRITE_HEADER, MF_BYCOMMAND) != MF_CHECKED)
					{
						CheckMenuItem(hMainMenu, IDM_WRITE_HEADER, MF_BYCOMMAND | MF_CHECKED);
					 	bWriteHeader=1;
					}
					else
					{
						CheckMenuItem(hMainMenu, IDM_WRITE_HEADER, MF_BYCOMMAND | MF_UNCHECKED);
					 	bWriteHeader=0;
					}
					break;
			}
			break;
	}

	return 0;
}

//--------------------------------------------------------------------------//
// return TRUE if we are to keep going
//
BOOL poll_window (void)
{
	BOOL result = 0;
	MSG         msg ;

	while (!bExitMessageReceived)
	{
		if ((bHasFocus==0) || (bGlobalRecording==0) || PeekMessage (&msg, NULL, 0, 0, PM_NOREMOVE))
		{
			if (GetMessage (&msg, NULL, 0, 0))
			{
/*
				if ((msg.message == WM_KEYUP || msg.message == WM_KEYDOWN) && 
					(msg.wParam == VK_LWIN || msg.wParam == VK_RWIN))
				{
					// do nothing!
				}								 
				else
*/
				if (bGlobalRecording || TranslateAccelerator(hMainWindow, hAccel, &msg) == 0)
				{
					if (bGlobalRecording || HKGROUP::Translate(msg)==0)
					{
//						if (msg.hwnd == hMainWindow)
//							IsDialogMessage(hMainWindow, &msg);
						TranslateMessage (&msg) ;
						DispatchMessage (&msg) ;
					}
				}
			}
			else
				break;
		}
		else
		{
			result = !bExitMessageReceived;
			break;
		}
	}

	return result;
}
//--------------------------------------------------------------------------//
//
char szCompanyName[] = "Blue Mu Productions";
char szProductName[] = "HotKey Editor";

BOOL GetDirectoryFromRegistry (char *szDirName, long id)
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
	if (RegQueryValueEx(hkey3, buffer, 0, &dwType, (UBYTE *)szDirName, &cbData) != ERROR_SUCCESS)
	{
		goto Done3;
	}

	if (dwType != REG_SZ)
	{
		WarningBox(GetFocus(), IDS_BAD_REGISTRY_VALUE);
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
// return TRUE set name in registry
//
char szClass[] = "Beatles";
BOOL SetDirectoryInRegistry (char *szDirName, long id)
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
	if (RegSetValueEx(hkey3, buffer, 0, REG_SZ, (UBYTE *)szDirName, strlen(szDirName)+1) != ERROR_SUCCESS)
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
// return TRUE if set name in registry
//
char szDefaultsKey[] = "User Defaults";

BOOL SetDefaultsInRegistry (void)
{
	HKEY hkey1, hkey2, hkey3;
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

	if (RegSetValueEx(hkey3, szDefaultsKey, 0, REG_BINARY, (UBYTE *)&user_defaults, sizeof(user_defaults)) != ERROR_SUCCESS)
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
// return TRUE if got defaults from registry
//
BOOL GetDefaultsFromRegistry (void)
{
	HKEY hkey1, hkey2, hkey3;
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

	cbData = sizeof(user_defaults);
	if (RegQueryValueEx(hkey3, szDefaultsKey, 0, &dwType, (UBYTE *)&user_defaults, &cbData) != ERROR_SUCCESS)
	{
		goto Done3;
	}

	if (dwType != REG_BINARY)
	{
		WarningBox(GetFocus(), IDS_BAD_REGISTRY_VALUE);
		goto Done3;
	}

	if (cbData != sizeof(USER_DEFAULTS))
	{
		user_defaults.init();
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
BOOL GetInputFilename(char *dst, long id, HWND hwnd)
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
    ofn.hwndOwner = hwnd;
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
		}

		return 1;
	}
	return 0;
}
//--------------------------------------------------------------------------//
// return TRUE if got a name
//
BOOL GetOutputFilename(char *dst, long id, HWND hwnd)
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
    ofn.hwndOwner = hwnd;
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
		}

		return 1;
	}
	return 0;
}
//----------------------------------------------------------------------------
// Does the user want to save the file before closing it?
//
BOOL QuerySaveFile (void)
{
	char msg[64];
	char buffer[256];
	char buffer2[256];
 	
	LoadString(hInstance, IDS_QUERY_SAVE_FILE, msg, 64);
	if (bUntitled)
		wsprintf(buffer, msg, szUntitled);
	else
	{
		pHotkeyFile->GetFileName(buffer2, sizeof(buffer2));
		wsprintf(buffer, msg, buffer2);
	}
	LoadString(hInstance, IDS_APP_NAME, msg, 64);
	switch (MessageBox(hMainWindow, buffer, msg, MB_ICONEXCLAMATION | MB_YESNOCANCEL))
	{
		case IDYES:
			return 1;
		case IDNO:
			return 0;
		default:
			return -1;
	}
	return -1;
}

//--------------------------------------------------------------------------//
//----------------------------END HKEditor.cpp------------------------------//
//--------------------------------------------------------------------------//
