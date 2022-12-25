/**************************************************************************
* 
* PrepStub98.cpp
* 
* Created 3/24/98 by Chris N. Haddan
* 
* (C) 1998 Microsoft Corporation
* 
***************************************************************************/
#include "PrepStub99.hpp"
#include "ProcessBuild.hpp"
#include "CPrepDoc.hpp"
#include "CSplashWnd.hpp"
#include "CCheckBoxListView.hpp"
#include "CPickList.hpp"
#include "stateinfo.h"
#include "Util.h"
#include "io.h"			//	for console output redirection.
#include <fcntl.h>		//  for console
#include <stdio.h>

CPrepDoc	*g_PrepDoc;
CPickList	*g_PickList;
BOOL		g_bDiskPaths = FALSE;
int			g_nCurrentDiskID = DISK_01;
bool InvokeBuildDialog (HWND hwndOwner, DWORD dwBuildType);
void SortCabFolders (CSetupDoc *sd, int l, int r);
void SwapItems (CSetupDoc *sd, int a, int b);


int WINAPI WinMain (HINSTANCE hInstance, HINSTANCE,	LPSTR lpCmdLine, int nShowCmd) 
{	
	HWND hwndApp;
	RECT rect;
	WORD wOS;

	if( *lpCmdLine )
	{
		FixupConsoleOutput();
		g_bCommandLine = TRUE;
		return (ProcessCommandLine (lpCmdLine));
	}

	g_bCommandLine = FALSE;

	if (hwndApp = FindWindow (APPLICATION_CLASS, NULL)) 
	{
		if (IsIconic (hwndApp))
		{
			ShowWindow (hwndApp, SW_RESTORE);
		}
		
		SetForegroundWindow (hwndApp);
		return false;
	}

	GetCurrentDirectory (MAX_PATH, g_szCurrentPath);

	if (!InitApplication (hInstance)) 
	{
		Alert( g_hAppWnd, MB_OK | MB_ICONEXCLAMATION, STR_INITFAILED);
		return false;
	}

	GetClientRect (GetDesktopWindow(), &rect);

	g_hAppWnd = CreateWindow (APPLICATION_CLASS, 
		APPLICATION_NAME,
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		(int)((double)(rect.right - rect.left) * 0.90),
		(int)((double)(rect.bottom - rect.top) * 0.90),
		NULL, NULL,
		hInstance,
		NULL);

	if (!g_hAppWnd) 
	{
		return false;
	}

	HDC hdc = GetDC (g_hAppWnd);
	if (GetDeviceCaps (hdc, BITSPIXEL) <= 8)
	{
		
		Alert (g_hAppWnd, MB_OK, STR_BAD_COLORDEPTH);
	}
	ReleaseDC (g_hAppWnd, hdc);

	// set the global window/instance handle for older core prepstub code
	g_hwnd = g_hAppWnd;
	g_hInst = g_hAppInst;

	LoadString( hInstance, STR_APPTITLE, g_szAppTitle, sizeof(g_szAppTitle) );

	// Make sure we are running on NT, if not let the app run, but warn user
	// they will not be able to use all features.

	wOS = GetCurrentOperatingSystem();

	if (wOS != OS_NT40 && wOS != OS_NT50)
	{
		Alert( g_hAppWnd, MB_OK | MB_ICONEXCLAMATION, STR_WINNT_REQUIRED);
	}

	g_PrepDoc = new CPrepDoc();
	
	if (!g_PrepDoc->Create(g_hAppWnd, g_hAppInst, g_szCurrentPath)) 
	{
		return false;
	}
	
	CSplashWnd *Splash = new CSplashWnd;

	if (!Splash->Create (g_hAppInst,  g_hAppWnd, IDB_SPLASH))
	{
		return false;
	}

	g_PrepDoc->CreateUserInterface();

	CenterWindowOnMonitor (g_hAppWnd, g_hAppWnd);
	ShowWindow (g_hAppWnd, nShowCmd);
	UpdateWindow (g_hAppWnd);

	EnableMenuOptions (MO_DOC_CLOSED);

	g_PickList = new CPickList();
	g_PickList->Create (PREPSTUB_PICKLIST_KEY, MAX_PICKLIST_ITEMS);
	g_PickList->Read();
	g_PickList->UpdateMenu (g_hAppWnd,  BASE_PICKLIST_ITEM, IDM_RECENTFILES);

	MSG msg;

	while (GetMessage (&msg, NULL, 0, 0)) 
	{
		TranslateMessage (&msg);
		DispatchMessage (&msg);
	}
	return (msg.wParam);
}



ATOM InitApplication (HINSTANCE hInstance)
{
	g_hAppInst = hInstance;

	WNDCLASS wc;
	ZeroMemory (&wc, sizeof (wc));
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = (WNDPROC) WndProc; 
    wc.cbClsExtra = 0; 
    wc.cbWndExtra = 0; 
    wc.hInstance = hInstance;
    wc.hIcon = LoadIcon (hInstance, MAKEINTRESOURCE (IDI_PREPSTUB98ICON)); 
    wc.hCursor = LoadCursor (NULL, MAKEINTRESOURCE (IDC_ARROW)); 
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
    wc.lpszMenuName = MAKEINTRESOURCE (IDM_MENU);
	wc.lpszClassName = APPLICATION_CLASS; 

	return RegisterClass (&wc);
}




LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
		case WM_SETCURSOR:
			if (NULL != g_hAppCursor)
			{
				SetCursor (g_hAppCursor);
				return true;
			}
			else
			{
				return (DefWindowProc (hWnd, msg, wParam, lParam));
			}
		case WM_CLOSE:
			if (!g_bExiting)
			{
				g_bExiting = true;
				if (!g_PrepDoc->CheckForUnsavedDoc())
				{
					g_bExiting = false;
					return 0;
				}

				g_PrepDoc->CloseDoc(false);
				delete g_PrepDoc;
				g_PrepDoc = NULL;
				delete g_PickList;
				return (DefWindowProc (hWnd, msg, wParam, lParam));
			}

		case WM_ERASEBKGND:
			return 1;

		case WM_SIZE:
			if (g_PrepDoc)
			{
				if (g_PrepDoc->GetMainFrame())
				{
					g_PrepDoc->GetMainFrame()->Resize (0, 0, LOWORD (lParam), HIWORD (lParam));
				}
			}
			return 0;

		case WM_DESTROY:
			PostQuitMessage (0);
			return 0;

		case WM_COMMAND:
			if (wParam >=BASE_PICKLIST_ITEM && wParam <= BASE_PICKLIST_ITEM + MAX_PICKLIST_ITEMS)
			{

				if (!g_PrepDoc->CheckForUnsavedDoc())
				{
					return false;
				}

				if (!g_PrepDoc->CloseDoc(false))
				{
					return false;	
				}

				if (!g_PrepDoc->Create(g_hAppWnd, g_hAppInst, g_szCurrentPath)) 
				{
					return false;
				}

				LoadScript (g_PickList->GetItem(wParam-BASE_PICKLIST_ITEM));

				break;
			}

			switch (wParam) 
			{
				case IDM_SAVERC3:
					{
						char szFileName[MAX_PATH];

						if (FileSaveDialog (g_hAppWnd, szFileName, "StaticStrings.rc", STR_FILE_FILTER_RC))
						{
							g_PrepDoc->WriteStringsToRC3 (szFileName, 0, 99, BASE_RESOURCE_STATIC_STRING_ID);
						}

						break;
					}
				case IDM_CLOSE:
					{	
						if (!g_PrepDoc->CheckForUnsavedDoc())
						{
							break;
						}

						if (!g_PrepDoc->CloseDoc(false))
						{
							break;
						}

						EnableMenuOptions (MO_DOC_CLOSED);
						UpdateWindowText();
					}
					break;

				case IDM_LOADSCRIPT:
					{
						char szScriptName[MAX_PATH*2] = "";

						if (!g_PrepDoc->CheckForUnsavedDoc())
						{
							return false;
						}

						if (FileOpenDialog (g_hAppWnd, "Load Script ...", "", (char *)&szScriptName, STR_FILE_FILTER_SCRIPT))
						{
							if (!g_PrepDoc->CloseDoc(false))
							{
								break;	
							}

							if (!g_PrepDoc->Create(g_hAppWnd, g_hAppInst, g_szCurrentPath)) 
							{
								break;
							}

							LoadScript (szScriptName);
						}
					}
					break;

				case IDM_NEW:
					{
						if (!g_PrepDoc->CheckForUnsavedDoc())
						{
							return false;
						}

						if (!g_PrepDoc->CloseDoc(false))
						{
							break;	
						}

						if (!g_PrepDoc->Create(g_hAppWnd, g_hAppInst, g_szCurrentPath)) 
						{
							break;
						}

						g_PrepDoc->SetScriptName ("");
						g_PrepDoc->NewDoc();
						g_PrepDoc->UpdateListView();
						g_PrepDoc->SetListDirtyState (true);

						EnableMenuOptions (MO_DOC_OPEN);
					}
					break;

				case IDM_LOADSTRINGS:

					if (Alert( g_hAppWnd, MB_OKCANCEL | MB_ICONEXCLAMATION, STR_LOADSTATICSTRINGSWARNING) == IDOK)
					{
						if (NULL!=g_PrepDoc->GetSetupDll())
						{
							if (DoesFileExist (g_PrepDoc->GetSetupDll()))
							{
								g_PrepDoc->ClearStaticStringList();

								if (!g_PrepDoc->RemoveStaticStringList ())
								{
									g_PrepDoc->CreateStaticStringSection();
								}

								g_PrepDoc->LoadStringsFromDLL (g_PrepDoc->GetSetupDll(), 500, 599, 0);
								g_PrepDoc->UpdateStaticStringList ();
								g_PrepDoc->UpdateListView();
								g_PrepDoc->SetListDirtyState (true);
								UpdateWindowText();
							}
							else
							{
								Alert( g_hAppWnd, MB_OKCANCEL | MB_ICONEXCLAMATION, STR_BADSETUPFILE, g_PrepDoc->GetSetupDll());
							}

						}
					}
					break;

				case IDM_BUILD_TRIAL:
					if (!g_PrepDoc->ValidateCommands())
					{
						if (Alert( g_hAppWnd, MB_OKCANCEL | MB_ICONEXCLAMATION, STR_COMMANDWITHNOFLAGS, g_szAppTitle) == IDCANCEL)
							break;
					}

					g_PrepDoc->SetUseLongFileNamesOnly (false);

					InvokeBuildDialog (hWnd, BUILD_TRIAL);

					g_PrepDoc->StripAppSettings();
					g_PrepDoc->WriteAppSettings();
					g_PrepDoc->UpdateListView();
					break;

				case IDM_BUILD:
					if (!g_PrepDoc->ValidateCommands())
					{
						if (Alert( g_hAppWnd, MB_OKCANCEL | MB_ICONEXCLAMATION, STR_COMMANDWITHNOFLAGS, g_szAppTitle) == IDCANCEL)
							break;
					}

					g_PrepDoc->SetUseLongFileNamesOnly (false);

					InvokeBuildDialog (hWnd, BUILD_REPLICATE);

					g_PrepDoc->StripAppSettings();
					g_PrepDoc->WriteAppSettings();
					g_PrepDoc->UpdateListView();

					break;

				case IDM_SAVE:
					if (!g_PrepDoc->ValidateCommands())
					{
						if (Alert( g_hAppWnd, MB_OKCANCEL | MB_ICONEXCLAMATION, STR_COMMANDWITHNOFLAGS, g_szAppTitle) == IDCANCEL)
							break;
					}

					if (strcmpi (g_PrepDoc->GetScriptName(),"")==0)
					{
						PostMessage (hWnd, WM_COMMAND, IDM_SAVEAS, 0);
					}
					else
					{
						g_PrepDoc->SetStatusBarText (0, "Saving...");

						EnableWindow (g_hAppWnd, FALSE);
						g_hAppCursor = LoadCursor (NULL, IDC_WAIT);
						HCURSOR hCursorOld = SetCursor (g_hAppCursor);

						g_PrepDoc->SaveDoc();

						g_hAppCursor = NULL;
						SetCursor (hCursorOld);
						EnableWindow (g_hAppWnd, TRUE);
						
						EbuYield();
							
						g_PrepDoc->SetStatusBarText (0, "Ready");
					}
					break;

				case IDM_SAVEAS:
					{
						char szFileName[MAX_PATH*2];
						int nType;

						if (!g_PrepDoc->ValidateCommands())
						{
							if (Alert( g_hAppWnd, MB_OKCANCEL | MB_ICONEXCLAMATION, STR_COMMANDWITHNOFLAGS, g_szAppTitle) == IDCANCEL)
								break;
						}

						if (FileSaveAsDialog (g_hAppWnd, szFileName, STR_FILE_FILTER_SAVEAS, &nType))
						{
							EbuYield();
							
							switch (nType)
							{
								case 1: // list
									{
										// rename the ScriptName to the new name and save

										g_PrepDoc->SetStatusBarText (0, "Saving...");

										EnableWindow (g_hAppWnd, FALSE);
										g_hAppCursor = LoadCursor (NULL, IDC_WAIT);
										HCURSOR hCursorOld = SetCursor (g_hAppCursor);

										g_PrepDoc->SetScriptName (szFileName);
										g_PrepDoc->SetDocLoadedState (true);
										g_PrepDoc->SetListDirtyState (false);
										g_PrepDoc->SaveDoc();
										UpdateWindowText();

										g_PickList->AddItem (szFileName);
										g_PickList->UpdateMenu (g_hAppWnd,  BASE_PICKLIST_ITEM, IDM_RECENTFILES);
										g_PickList->Write ();

										g_hAppCursor = NULL;
										SetCursor (hCursorOld);
										EnableWindow (g_hAppWnd, TRUE);
										
										EbuYield();
											
										g_PrepDoc->SetStatusBarText (0, "Ready");

									}
									break;
								case 2: // bin
									if (!IsFileWritable (szFileName))
									{
										Alert( g_hAppWnd, MB_OK | MB_ICONEXCLAMATION, STR_FILENOTWRITABLE, g_szAppTitle, szFileName);
										break;
									}

									g_PrepDoc->SetSaveAsFileName (szFileName); 
									
									InvokeBuildDialog (hWnd, BUILD_SAVEAS);
									break;
							}
						}
						break;
					}

				case IDM_INJECT:
					{
						if (!g_PrepDoc->ValidateCommands())
						{
							if (Alert( g_hAppWnd, MB_OKCANCEL | MB_ICONEXCLAMATION, STR_COMMANDWITHNOFLAGS, g_szAppTitle) == IDCANCEL)
								break;
						}
						InvokeBuildDialog (hWnd, BUILD_INJECT);
						break;
					}

				case IDM_ABOUT:
					DialogBox (g_hAppInst, MAKEINTRESOURCE(IDD_ABOUT), hWnd, (DLGPROC) AboutDialogProc);
					break;

				case IDM_EXIT:
					PostMessage (hWnd, WM_CLOSE, 0, 0);
					break;

				case IDM_SHOWREG:
					g_PrepDoc->SetShowRegularState (!GetMenuState (GetMenu(g_hAppWnd), IDM_SHOWREG, FALSE));
					CheckMenuItem (GetMenu(g_hAppWnd), IDM_SHOWREG, g_PrepDoc->IsShowRegular() ? MF_CHECKED:MF_GRAYED);
					g_PrepDoc->UpdateListView();
					break;

				case IDM_SHOWCOM:
					g_PrepDoc->SetShowCommentsState (!GetMenuState (GetMenu(g_hAppWnd), IDM_SHOWCOM, FALSE));
					CheckMenuItem (GetMenu(g_hAppWnd), IDM_SHOWCOM, g_PrepDoc->IsShowComments() ? MF_CHECKED:MF_GRAYED);
					g_PrepDoc->UpdateListView();
					break;

				case IDM_SHOWPROPERTY:
					g_PrepDoc->SetShowProperty (!GetMenuState (GetMenu(g_hAppWnd), IDM_SHOWPROPERTY, FALSE));
					CheckMenuItem (GetMenu(g_hAppWnd), IDM_SHOWPROPERTY, g_PrepDoc->IsShowProperty() ? MF_CHECKED:MF_GRAYED);
					g_PrepDoc->UpdateListView();
					break;

				case IDM_SHOWINT:
					g_PrepDoc->SetShowInternalState (!GetMenuState (GetMenu(g_hAppWnd), IDM_SHOWINT, FALSE));
					CheckMenuItem (GetMenu(g_hAppWnd), IDM_SHOWINT, g_PrepDoc->IsShowInternal()?MF_CHECKED:MF_GRAYED);
					g_PrepDoc->UpdateListView();
					break;

				case IDM_SHOWCOLOR:
					g_PrepDoc->SetShowInColorState (!GetMenuState (GetMenu(g_hAppWnd), IDM_SHOWCOLOR, FALSE));
					CheckMenuItem (GetMenu(g_hAppWnd), IDM_SHOWCOLOR, g_PrepDoc->IsShowInColor()?MF_CHECKED:MF_GRAYED);
					g_PrepDoc->UpdateListView();
					break;

				case IDM_BUILDSETTINGS:
					InvokeBuildDialog (hWnd, BUILD_SETTINGS);
					break;

				case IDM_UPDATEFILELIST:
					{
						g_PrepDoc->SetUseLongFileNamesOnly (true);

						if (g_PrepDoc->GetLastBuildType() == PBT_REPLICATE)
						{
							if (Alert( g_hAppWnd, MB_YESNO | MB_ICONEXCLAMATION, STR_SWITCHTOLFN)  == IDYES)
							{
								InvokeBuildDialog (hWnd, BUILD_UPDATEFILELIST);
							}
						}
						else
						{
							InvokeBuildDialog (hWnd, BUILD_UPDATEFILELIST);
						}
					}
					break;
			}
			UpdateStatusBar();
			return 0;
	}
	return (DefWindowProc (hWnd, msg, wParam, lParam));
}


void UpdateWindowText()
{
	char buffer[MAX_PATH*2];

	// BUGBUG: should not show extension .plf, also should not show path if in current dir.
	if (g_PrepDoc->GetScriptName())
	{
		wsprintf (buffer, "%s - %s", g_szAppTitle, g_PrepDoc->GetScriptName());
	}
	else
	{
		wsprintf (buffer, "%s", g_szAppTitle);
	}

	if (g_PrepDoc->IsListDirty())
		lstrcat (buffer, " * ");

	SetWindowText (g_hAppWnd, buffer);
}


void EnableMenuOptions (DWORD dwState)
{
	HMENU hMenu = GetMenu (g_hAppWnd);

	switch (dwState)
	{
		case MO_DOC_OPEN:
			EnableMenuItem (hMenu, IDM_CLOSE, MF_ENABLED);
			EnableMenuItem (hMenu, IDM_SAVE, MF_ENABLED);
			EnableMenuItem (hMenu, IDM_SAVEAS, MF_ENABLED);
			EnableMenuItem (hMenu, IDM_BUILD, MF_ENABLED);
			EnableMenuItem (hMenu, IDM_INJECT, MF_ENABLED);
			EnableMenuItem (hMenu, IDM_BUILDSETTINGS, MF_ENABLED);
			EnableMenuItem (hMenu, IDM_UPDATEFILELIST, MF_ENABLED);
			EnableMenuItem (hMenu, IDM_BUILD_TRIAL, MF_ENABLED);
			EnableMenuItem (hMenu, IDM_SAVERC3, MF_ENABLED);
			EnableMenuItem (hMenu, IDM_LOADSTRINGS, MF_ENABLED);
			EnableMenuItem (hMenu, IDM_SHOWCOM, MF_ENABLED);	
			EnableMenuItem (hMenu, IDM_SHOWREG, MF_ENABLED);
			EnableMenuItem (hMenu, IDM_SHOWINT, MF_ENABLED);
			EnableMenuItem (hMenu, IDM_SHOWCOLOR, MF_ENABLED);
			EnableMenuItem (hMenu, IDM_SHOWPROPERTY, MF_ENABLED);
		break;
		case MO_DOC_CLOSED:
			EnableMenuItem (hMenu, IDM_CLOSE, MF_GRAYED);
			EnableMenuItem (hMenu, IDM_SAVE, MF_GRAYED);
			EnableMenuItem (hMenu, IDM_SAVEAS, MF_GRAYED);
			EnableMenuItem (hMenu, IDM_BUILD, MF_GRAYED);
			EnableMenuItem (hMenu, IDM_INJECT, MF_GRAYED);
			EnableMenuItem (hMenu, IDM_BUILDSETTINGS, MF_GRAYED);
			EnableMenuItem (hMenu, IDM_UPDATEFILELIST, MF_GRAYED);
			EnableMenuItem (hMenu, IDM_BUILD_TRIAL, MF_GRAYED);
			EnableMenuItem (hMenu, IDM_SAVERC3, MF_GRAYED);
			EnableMenuItem (hMenu, IDM_LOADSTRINGS, MF_GRAYED);
			EnableMenuItem (hMenu, IDM_SHOWCOM, MF_GRAYED);
			EnableMenuItem (hMenu, IDM_SHOWREG, MF_GRAYED);
			EnableMenuItem (hMenu, IDM_SHOWINT, MF_GRAYED);
			EnableMenuItem (hMenu, IDM_SHOWCOLOR, MF_GRAYED);
			EnableMenuItem (hMenu, IDM_SHOWPROPERTY, MF_GRAYED);
		break;
	}
}


void SetViewMenuState (void)
{
	CheckMenuItem (GetMenu(g_hAppWnd), IDM_SHOWREG,		g_PrepDoc->IsShowRegular() ? MF_CHECKED:MF_GRAYED);
	CheckMenuItem (GetMenu(g_hAppWnd), IDM_SHOWCOM,		g_PrepDoc->IsShowComments()? MF_CHECKED:MF_GRAYED);
	CheckMenuItem (GetMenu(g_hAppWnd), IDM_SHOWPROPERTY,g_PrepDoc->IsShowProperty()? MF_CHECKED:MF_GRAYED);
	CheckMenuItem (GetMenu(g_hAppWnd), IDM_SHOWINT,		g_PrepDoc->IsShowInternal()? MF_CHECKED:MF_GRAYED);
	CheckMenuItem (GetMenu(g_hAppWnd), IDM_SHOWCOLOR,	g_PrepDoc->IsShowInColor() ? MF_CHECKED:MF_GRAYED);
}


void UpdateStatusBar ()
{
	switch (g_PrepDoc->GetLastBuildType ())
	{
		case PBT_REPLICATE:
			g_PrepDoc->SetStatusBarText (3, "ISO 9660");
			break;
		case PBT_UPDATE:
			g_PrepDoc->SetStatusBarText (3, "LFN");
			break;
		case PBT_UNKNOWN:
			g_PrepDoc->SetStatusBarText (3, "Unknown");
			break;
	}
}




BOOL CALLBACK AboutDialogProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
		case WM_INITDIALOG:
			char szBuildNumber[15];
			LoadString (g_hAppInst, STR_BUILD_NUMBER, szBuildNumber, 15);
			SetDlgItemText (hWnd, IDC_BUILDNUMBER, szBuildNumber);
			return true;

		case WM_COMMAND:
			switch (wParam) 
			{
				case IDCANCEL:
				case IDOK:
					EndDialog (hWnd, 0);
					break;
			}
			return 0;
	}
	return 0;
}


void FixupConsoleOutput (void)
{
	int hCrt;   
	FILE *hf;   
	AllocConsole();   
	hCrt = _open_osfhandle( (long) GetStdHandle(STD_OUTPUT_HANDLE),
							_O_TEXT);   
	
	hf = _fdopen( hCrt, "w" );   
	*stdout = *hf;
	setvbuf( stdout, NULL, _IONBF, 0 );
}


BOOL CALLBACK BuildDialogProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	char szInstructions[255];
	char szDir[MAX_PATH*2];
	static DWORD dwBuildType;
	NMHDR *pnmhdr;

	switch (msg)
	{
		case WM_INITDIALOG:
			{
				//
				// There is no way to center a property page, so we will use a small hack here
				// to center it. The property sheet is our parent and this is the first page visible,
				// so we just center the parent window before it is displayed. 
				//

				if (GetParent(hWnd) != g_hAppWnd)
				{
					CenterWindowOnMonitor (g_hAppWnd, GetParent(hWnd));
				}

				// extract out the property sheet page for this dialog so we can get the user data in lParam
				dwBuildType = (DWORD)((PROPSHEETPAGE *)lParam)->lParam;
				

				// Fill the dialog with current settings

				if (g_PrepDoc->GetSourcePath() != NULL)
					SetDlgItemText (hWnd, IDC_SOURCEPATH, g_PrepDoc->GetSourcePath());

				if (g_PrepDoc->GetDropPath() != NULL)
					SetDlgItemText (hWnd, IDC_DROPPATH, g_PrepDoc->GetDropPath());
	
				if (g_PrepDoc->GetReparentPath() != NULL)
					SetDlgItemText (hWnd, IDC_REPARENTDIR, g_PrepDoc->GetReparentPath());

				SendMessage (GetDlgItem(hWnd, IDC_REPLICATEFILETREE), BM_SETCHECK, g_PrepDoc->IsReplicateFileTree(), 0);
				SendMessage (GetDlgItem(hWnd, IDC_UPDATEFILELIST), BM_SETCHECK, g_PrepDoc->IsUpdateFileList(), 0);

				// enable and disable items based on current settings

				EnableWindow ( GetDlgItem (hWnd, IDC_DROPPATH),
					BST_CHECKED & SendMessage (GetDlgItem (hWnd, IDC_REPLICATEFILETREE), BM_GETCHECK, 0, 0));

				EnableWindow ( GetDlgItem (hWnd, IDC_REPARENTDIR),
					BST_CHECKED & SendMessage (GetDlgItem (hWnd, IDC_REPLICATEFILETREE), BM_GETCHECK, 0, 0));

				EnableWindow ( GetDlgItem (hWnd, IDC_BTN_DROP),
					BST_CHECKED & SendMessage (GetDlgItem (hWnd, IDC_REPLICATEFILETREE), BM_GETCHECK, 0, 0));

				EnableWindow ( GetDlgItem (hWnd, IDC_DROPPATHTEXT),
					BST_CHECKED & SendMessage (GetDlgItem (hWnd, IDC_REPLICATEFILETREE), BM_GETCHECK, 0, 0));

				EnableWindow ( GetDlgItem (hWnd, IDC_REPARENTPATHTEXT),
					BST_CHECKED & SendMessage (GetDlgItem (hWnd, IDC_REPLICATEFILETREE), BM_GETCHECK, 0, 0));


				// SaveAS, Inject, Settings, and Replicate now use this single dialog.
				// Disable some items based on build type.

				switch (dwBuildType)
				{
					case BUILD_SAVEAS:
						SetWindowText (GetParent (hWnd), "Prepstub 99 - Save As Binary");

						// disable a few items

						EnableWindow ( GetDlgItem (hWnd, IDC_REPLICATEFILETREE),	FALSE );
						EnableWindow ( GetDlgItem (hWnd, IDC_DROPPATH),				FALSE );
						EnableWindow ( GetDlgItem (hWnd, IDC_BTN_DROP),				FALSE );
						EnableWindow ( GetDlgItem (hWnd, IDC_REPARENTDIR),			FALSE );
						EnableWindow ( GetDlgItem (hWnd, IDC_REPLICATEFILETREE),	FALSE );
						EnableWindow ( GetDlgItem (hWnd, IDC_DROPPATHTEXT),			FALSE );
						EnableWindow ( GetDlgItem (hWnd, IDC_REPARENTPATHTEXT),		FALSE );

						if (g_PrepDoc->GetLastBuildType() == PBT_REPLICATE)
						{
							EnableWindow ( GetDlgItem (hWnd, IDC_UPDATEFILELIST),		FALSE );
						}

						SetWindowText (GetDlgItem (GetParent (hWnd), IDOK), "Save");
					break;

					case BUILD_UPDATEFILELIST:
						SetWindowText (GetParent (hWnd), "Prepstub 99 - Update File List");
						EnableWindow ( GetDlgItem (hWnd, IDC_REPLICATEFILETREE),	FALSE );
						EnableWindow ( GetDlgItem (hWnd, IDC_DROPPATH),				FALSE );
						EnableWindow ( GetDlgItem (hWnd, IDC_BTN_DROP),				FALSE );
						EnableWindow ( GetDlgItem (hWnd, IDC_REPARENTDIR),			FALSE );
						EnableWindow ( GetDlgItem (hWnd, IDC_REPLICATEFILETREE),	FALSE );	
						EnableWindow ( GetDlgItem (hWnd, IDC_DROPPATHTEXT),			FALSE );
						EnableWindow ( GetDlgItem (hWnd, IDC_REPARENTPATHTEXT),		FALSE );
						EnableWindow ( GetDlgItem (hWnd, IDC_UPDATEFILELIST),		FALSE );

						g_PrepDoc->SetUpdateFileList(true);
						SendMessage (GetDlgItem(hWnd, IDC_UPDATEFILELIST), BM_SETCHECK, g_PrepDoc->IsUpdateFileList(), 0);

						SetWindowText (GetDlgItem (GetParent (hWnd), IDOK), "Update"); 

					break;

					case BUILD_SETTINGS:
						SetWindowText (GetParent (hWnd), "Prepstub 99 - Settings");
						SetWindowText (GetDlgItem (GetParent (hWnd), IDOK), "OK");
					break;

					case BUILD_INJECT:
						SetWindowText (GetParent (hWnd), "Prepstub 99 - Inject");
						SetWindowText (GetDlgItem (GetParent (hWnd), IDOK), "Inject");

						EnableWindow ( GetDlgItem (hWnd, IDC_REPLICATEFILETREE),	FALSE );
						EnableWindow ( GetDlgItem (hWnd, IDC_DROPPATH),				FALSE );
						EnableWindow ( GetDlgItem (hWnd, IDC_BTN_DROP),				FALSE );
						EnableWindow ( GetDlgItem (hWnd, IDC_REPARENTDIR),			FALSE );
						EnableWindow ( GetDlgItem (hWnd, IDC_DROPPATHTEXT),			FALSE );
						EnableWindow ( GetDlgItem (hWnd, IDC_REPARENTPATHTEXT),		FALSE );

						if (g_PrepDoc->GetLastBuildType() == PBT_REPLICATE)
						{
							EnableWindow ( GetDlgItem (hWnd, IDC_UPDATEFILELIST),		FALSE );
						}

					break;

					case BUILD_REPLICATE:
						SetWindowText (GetParent (hWnd), "Prepstub 99 - Replicate and Inject");
						SetWindowText (GetDlgItem (GetParent (hWnd), IDOK), "Build");
					break;

					case BUILD_TRIAL:
						SetWindowText (GetParent (hWnd), "Prepstub 99 - Build Trial Version");
						SetWindowText (GetDlgItem (GetParent (hWnd), IDOK), "Build");
						EnableWindow ( GetDlgItem (hWnd, IDC_UPDATEFILELIST),		FALSE );
						EnableWindow ( GetDlgItem (hWnd, IDC_REPLICATEFILETREE),	FALSE );
						EnableWindow ( GetDlgItem (hWnd, IDC_REPARENTPATHTEXT),		FALSE );
						EnableWindow ( GetDlgItem (hWnd, IDC_REPARENTDIR),			FALSE );
					break;
				}

				SendMessage (hWnd, WM_COMMAND, IDC_UPDATEFILELIST,0);

				return true;
			}

			case WM_NOTIFY:
				pnmhdr = (NMHDR *)(lParam);
				if (pnmhdr->code == PSN_APPLY)
				{
					DWORD dwBuildFlags = 0;

					// ----------------------------------------------------------------------
					//		Source Path
					// ----------------------------------------------------------------------

					GetDlgItemText (hWnd, IDC_SOURCEPATH, szDir, MAX_PATH*2);
					MakePathCompliant (szDir);
					g_PrepDoc->SetSourcePath (szDir);

					// ----------------------------------------------------------------------
					//		Drop Path
					// ----------------------------------------------------------------------

					GetDlgItemText (hWnd, IDC_DROPPATH, szDir, MAX_PATH*2);
					MakePathCompliant (szDir);
					g_PrepDoc->SetDropPath (szDir);

					// ----------------------------------------------------------------------
					//		Reparent Path
					// ----------------------------------------------------------------------

					GetDlgItemText (hWnd, IDC_REPARENTDIR, szDir, MAX_PATH*2);
					while (*szDir=='\\')
					{
						memmove (szDir, szDir+1, lstrlen (szDir)+1);
					}
					g_PrepDoc->SetReparentPath (szDir);

					// ----------------------------------------------------------------------
					//		Various checkboxes, etc
					// ----------------------------------------------------------------------

					g_PrepDoc->SetLangID (MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US));

					g_PrepDoc->SetReplicateFileTree (BST_CHECKED & SendMessage (GetDlgItem (hWnd, IDC_REPLICATEFILETREE), BM_GETCHECK, 0, 0));
					g_PrepDoc->SetUpdateFileList (BST_CHECKED & SendMessage (GetDlgItem (hWnd, IDC_UPDATEFILELIST), BM_GETCHECK, 0, 0));

					UpdateWindowText();
					}
				break;

		case WM_COMMAND:
			switch (wParam) 
			{
				case IDC_UPDATEFILELIST:
					if (g_PrepDoc->GetLastBuildType()==PBT_REPLICATE && dwBuildType == BUILD_REPLICATE)
					{
						EnableWindow ( GetDlgItem (hWnd, IDC_REPLICATEFILETREE),
						!(BST_CHECKED & SendMessage (GetDlgItem (hWnd, IDC_UPDATEFILELIST), BM_GETCHECK, 0, 0)));

						if (BST_CHECKED & SendMessage (GetDlgItem (hWnd, IDC_UPDATEFILELIST), BM_GETCHECK, 0, 0))
						{
							SendMessage (GetDlgItem (hWnd, IDC_REPLICATEFILETREE), BM_SETCHECK, true, 0);
						}

						SendMessage (hWnd, WM_COMMAND, IDC_REPLICATEFILETREE,0);
					}
					break;

				case IDC_REPLICATEFILETREE:
					EnableWindow ( GetDlgItem (hWnd, IDC_DROPPATH),
						BST_CHECKED & SendMessage (GetDlgItem (hWnd, IDC_REPLICATEFILETREE), BM_GETCHECK, 0, 0));

					EnableWindow ( GetDlgItem (hWnd, IDC_REPARENTDIR),
						BST_CHECKED & SendMessage (GetDlgItem (hWnd, IDC_REPLICATEFILETREE), BM_GETCHECK, 0, 0));

					EnableWindow ( GetDlgItem (hWnd, IDC_BTN_DROP),
						BST_CHECKED & SendMessage (GetDlgItem (hWnd, IDC_REPLICATEFILETREE), BM_GETCHECK, 0, 0));

					EnableWindow ( GetDlgItem (hWnd, IDC_DROPPATHTEXT),
						BST_CHECKED & SendMessage (GetDlgItem (hWnd, IDC_REPLICATEFILETREE), BM_GETCHECK, 0, 0));

					EnableWindow ( GetDlgItem (hWnd, IDC_REPARENTPATHTEXT),
						BST_CHECKED & SendMessage (GetDlgItem (hWnd, IDC_REPLICATEFILETREE), BM_GETCHECK, 0, 0));
					
					break;

				case IDC_BTN_SOURCE:
					LoadString (g_hInst, STR_SETSOURCEFOLDER, szInstructions, sizeof (szInstructions));
					if (FolderBrowse (szDir, szInstructions))
					{
						SetDlgItemText (hWnd, IDC_SOURCEPATH, szDir);
					}
					SetFocus (hWnd);
					break;

				case IDC_BTN_DROP:
					LoadString (g_hInst, STR_SETDROPFOLDER, szInstructions, sizeof (szInstructions));
					if (FolderBrowse (szDir, szInstructions))
					{
						SetDlgItemText (hWnd, IDC_DROPPATH, szDir);
					}
					SetFocus (hWnd);
					break;
			}
			return 0;
	}
	return 0;
}


BOOL CALLBACK StringSettingsDialogProc (HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	static DWORD dwBuildType;

	NMHDR *pnmhdr;
	switch (msg)
	{
		case WM_INITDIALOG:
			{
				dwBuildType = (DWORD)((PROPSHEETPAGE *)lParam)->lParam;
				EnableWindow (hWnd, TRUE);

				SendMessage (GetDlgItem(hWnd, IDC_LOADSCRIPTSTRINGS),	BM_SETCHECK, g_PrepDoc->IsLoadScriptStrings(), 0);
				SendMessage (GetDlgItem(hWnd, IDC_LOADDLLSTRINGS),		BM_SETCHECK, g_PrepDoc->IsLoadDllStrings(), 0);
				SendMessage (GetDlgItem(hWnd, IDC_IGNORESTRINGS),		BM_SETCHECK, g_PrepDoc->IsIgnoreStrings(), 0);

				if (g_PrepDoc->IsLoadDllStrings())
				{
					EnableWindow (GetDlgItem (hWnd, IDC_SETUPDLLLOCATION), true);
				}
				else
				{
					EnableWindow (GetDlgItem (hWnd, IDC_SETUPDLLLOCATION), false);
				}

				if (g_PrepDoc->GetSetupDll() != NULL)
					SetDlgItemText (hWnd, IDC_SETUPDLLLOCATION, g_PrepDoc->GetSetupDll());

				switch (dwBuildType)
				{
					case BUILD_UPDATEFILELIST:
						EnableWindow (hWnd, FALSE);
						EnableWindow (GetDlgItem(hWnd, IDC_LOADSCRIPTSTRINGS),	FALSE);
						EnableWindow (GetDlgItem(hWnd, IDC_LOADDLLSTRINGS),		FALSE);
						EnableWindow (GetDlgItem(hWnd, IDC_IGNORESTRINGS),		FALSE);
						EnableWindow (GetDlgItem(hWnd, IDC_SETUPDLLLOCATION),	FALSE);
						EnableWindow ( GetDlgItem (hWnd, IDC_STRINGTEXT),		FALSE );
						
					break;
				}

				return true;
			}

		case WM_NOTIFY:
			pnmhdr = (NMHDR *)(lParam);
			if (pnmhdr->code == PSN_APPLY)
			{
				g_PrepDoc->SetLoadScriptStrings (BST_CHECKED & SendMessage (GetDlgItem (hWnd, IDC_LOADSCRIPTSTRINGS), BM_GETCHECK, 0, 0));
				g_PrepDoc->SetLoadDllStrings	(BST_CHECKED & SendMessage (GetDlgItem (hWnd, IDC_LOADDLLSTRINGS), BM_GETCHECK, 0, 0));
				g_PrepDoc->SetIgnoreStrings		(BST_CHECKED & SendMessage (GetDlgItem (hWnd, IDC_IGNORESTRINGS), BM_GETCHECK, 0, 0));
				g_PrepDoc->SetListDirtyState (true);
			}
			break;
		case WM_COMMAND:
			switch (wParam) 
			{
				case IDC_LOADSCRIPTSTRINGS:
						EnableWindow (GetDlgItem (hWnd, IDC_SETUPDLLLOCATION), false);
						break;

				case IDC_LOADDLLSTRINGS:
						EnableWindow (GetDlgItem (hWnd, IDC_SETUPDLLLOCATION), true);
						break;

				case IDC_IGNORESTRINGS:
						EnableWindow (GetDlgItem (hWnd, IDC_SETUPDLLLOCATION), false);
						break;
			}
			return 0;
	}
	return 0;
}


BOOL CALLBACK AdvancedInjectSettingsDialogProc (HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	static DWORD dwBuildType;
	char szFile[MAX_PATH*2];
	char szDir[MAX_PATH*2];
	char szTemp[MAX_PATH*2];
	NMHDR *pnmhdr;

	switch (msg)
	{
		case WM_INITDIALOG:
			{
				dwBuildType = (DWORD)((PROPSHEETPAGE *)lParam)->lParam;

				switch (dwBuildType)
				{
					case BUILD_SAVEAS:
						EnableWindow ( GetDlgItem (hWnd, IDC_SETUPLOCATION),		FALSE );
						EnableWindow ( GetDlgItem (hWnd, IDC_BTN_SETUP),			FALSE );
						EnableWindow ( GetDlgItem (hWnd, IDC_SETUPEXETEXT),			FALSE );

						EnableWindow ( GetDlgItem (hWnd, IDC_INJECTBINARYBLOB),		FALSE );
						EnableWindow ( GetDlgItem (hWnd, IDC_INJECTDYNAMICSTRINGS),	FALSE );
						EnableWindow ( GetDlgItem (hWnd, IDC_INJECTSTATICSTRINGS),	FALSE );
						EnableWindow ( GetDlgItem (hWnd, IDC_INJECTTEXT),			FALSE );

					break;

					case BUILD_UPDATEFILELIST:
						EnableWindow ( GetDlgItem (hWnd, IDC_SETUPEXETEXT),			FALSE );
						EnableWindow ( GetDlgItem (hWnd, IDC_DLLLOCATIONTEXT),		FALSE );
						EnableWindow ( GetDlgItem (hWnd, IDC_SETUPDLLLOCATION),		FALSE );
						EnableWindow ( GetDlgItem (hWnd, IDC_BTN_SETUPDLL),			FALSE );
						EnableWindow ( GetDlgItem (hWnd, IDC_SETUPLOCATION),		FALSE );
						EnableWindow ( GetDlgItem (hWnd, IDC_BTN_SETUP),			FALSE );
						EnableWindow ( GetDlgItem (hWnd, IDC_SETUPEXETEXT),			FALSE );

						EnableWindow ( GetDlgItem (hWnd, IDC_INJECTBINARYBLOB),		FALSE );
						EnableWindow ( GetDlgItem (hWnd, IDC_INJECTDYNAMICSTRINGS),	FALSE );
						EnableWindow ( GetDlgItem (hWnd, IDC_INJECTSTATICSTRINGS),	FALSE );
						EnableWindow ( GetDlgItem (hWnd, IDC_INJECTTEXT),			FALSE );
						EnableWindow ( GetDlgItem (hWnd, IDC_RESOURCEANDINJECTTEXT),FALSE );
					break;
				}

				if (g_PrepDoc->GetSetupExe() != NULL)
					SetDlgItemText (hWnd, IDC_SETUPLOCATION, g_PrepDoc->GetSetupExe());

				if (g_PrepDoc->GetSetupDll() != NULL)
					SetDlgItemText (hWnd, IDC_SETUPDLLLOCATION, g_PrepDoc->GetSetupDll());


				// check the script to see if there are 0 dynamic strings and there are 0 rules of type localize
				// if these condintions are met, then disable 

				if ((g_PrepDoc->GetStringList()->GetStringCountForRange (BASE_DYNAMIC_STRING_ID, BASE_DYNAMIC_STRING_ID + MAX_DYNAMIC_STRINGS) == 0) 
						&& (g_PrepDoc->GetFileRules()->CountRuleType(RULE_LOCALIZE_STRING) == 0))
				{
					g_PrepDoc->SetInjectDynamicStrings (false);
					EnableWindow (GetDlgItem(hWnd, IDC_INJECTDYNAMICSTRINGS), false);
				}

				SendMessage (GetDlgItem(hWnd, IDC_INJECTSTATICSTRINGS), BM_SETCHECK, g_PrepDoc->IsInjectStaticStrings(), 0);
				SendMessage (GetDlgItem(hWnd, IDC_INJECTDYNAMICSTRINGS), BM_SETCHECK, g_PrepDoc->IsInjectDynamicStrings(), 0);
				SendMessage (GetDlgItem(hWnd, IDC_INJECTBINARYBLOB), BM_SETCHECK, g_PrepDoc->IsInjectBinaryBlob(), 0);

				return true;
			}

		case WM_COMMAND:
			switch (wParam) 
			{
				case IDC_BTN_SETUP:
					GetDlgItemText (hWnd, IDC_SETUPLOCATION, szTemp, MAX_PATH*2);
					SplitPathAndFileName (szTemp, szDir, szFile);
					if (FileOpenDialog (hWnd, "Select Resource to Inject...", (char *)&szDir, (char *)&szFile, STR_FILE_FILTER_EXE))
					{
						SetDlgItemText (hWnd, IDC_SETUPLOCATION, szFile);
					}
					SetFocus (hWnd);
					break;

				case IDC_BTN_SETUPDLL:
					GetDlgItemText (hWnd, IDC_SETUPDLLLOCATION, szTemp, MAX_PATH*2);
					SplitPathAndFileName (szTemp, szDir, szFile);
					if (FileOpenDialog (hWnd, "Select DLL Resource to Inject...", (char *)&szDir, (char *)&szFile, STR_FILE_FILTER_DLL))
					{
						SetDlgItemText (hWnd, IDC_SETUPDLLLOCATION, szFile);
					}
					SetFocus (hWnd);
					break;
			}
			return 0;

		case WM_NOTIFY:
			pnmhdr = (NMHDR *)(lParam);
			if (pnmhdr->code == PSN_APPLY)
			{

				// ----------------------------------------------------------------------
				//		SetupExe location
				// ----------------------------------------------------------------------

				GetDlgItemText (hWnd, IDC_SETUPLOCATION, szDir, MAX_PATH*2);

				// validate the Setup.exe location, except if we are SaveAs'ing since it is disabled in that case

				/*
				if (*szDir)
				{
					if (dwBuildType != BUILD_SAVEAS && dwBuildType != BUILD_UPDATEFILELIST)
					{
						if (!DoesFileExist (szDir))
						{
							// let the user continue anyway if they really want to. But they will probably fail.
							if (IDCANCEL == Alert( g_hAppWnd, MB_OKCANCEL | MB_ICONEXCLAMATION, STR_BADSETUPFILE, szDir))
							{
								return true;
							}
						}
					}
				}*/
				g_PrepDoc->SetSetupExe (szDir);

				// ----------------------------------------------------------------------
				//		SetupDLL location
				// ----------------------------------------------------------------------

				GetDlgItemText (hWnd, IDC_SETUPDLLLOCATION, szDir, MAX_PATH*2);

				/*
				if (dwBuildType != BUILD_UPDATEFILELIST)
				{
					if (*szDir)
					{
						if (!DoesFileExist (szDir))
						{
							if (IDCANCEL == Alert( g_hAppWnd, MB_OKCANCEL | MB_ICONEXCLAMATION, STR_BADSETUPDLLFILE, szDir))
							{
								return true;
							}
						}
					}
				}*/

				g_PrepDoc->SetSetupDll (szDir);

				// ----------------------------------------------------------------------
				//		Inject checkboxes
				// ----------------------------------------------------------------------

				g_PrepDoc->SetInjectBinaryBlob    (BST_CHECKED & SendMessage (GetDlgItem (hWnd, IDC_INJECTBINARYBLOB),		BM_GETCHECK, 0, 0));
				g_PrepDoc->SetInjectDynamicStrings(BST_CHECKED & SendMessage (GetDlgItem (hWnd, IDC_INJECTDYNAMICSTRINGS),	BM_GETCHECK, 0, 0));
				g_PrepDoc->SetInjectStaticStrings (BST_CHECKED & SendMessage (GetDlgItem (hWnd, IDC_INJECTSTATICSTRINGS),	BM_GETCHECK, 0, 0));
				g_PrepDoc->SetListDirtyState (true);
			}
			break;
	}
	return 0;
}


BOOL CALLBACK TrialVersionDialogProc (HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	char szValue[MAX_PATH*2];
	NMHDR *pnmhdr;

	switch (msg)
	{
		case WM_INITDIALOG:
			{
				if (g_PrepDoc->GetUninstallExe() != NULL)
					SetDlgItemText (hWnd, IDC_UNINSTALLEXE, g_PrepDoc->GetUninstallExe());

				if (g_PrepDoc->GetTrialExe() != NULL)
					SetDlgItemText (hWnd, IDC_TRIALEXE, g_PrepDoc->GetTrialExe());

				if (g_PrepDoc->GetMakeCabExe() !=NULL)
					SetDlgItemText (hWnd, IDC_MAKECABEXE, g_PrepDoc->GetMakeCabExe());

				return true;
			}

		case WM_NOTIFY:
			pnmhdr = (NMHDR *)(lParam);
			if (pnmhdr->code == PSN_APPLY)
			{
				// uninstall.exe name
				GetDlgItemText (hWnd, IDC_UNINSTALLEXE, szValue, MAX_PATH*2);
				g_PrepDoc->SetUninstallExe(szValue);

				// trial.exe
				GetDlgItemText (hWnd, IDC_TRIALEXE, szValue, MAX_PATH*2);
				g_PrepDoc->SetTrialExe(szValue);

				// makecab.exe
				GetDlgItemText (hWnd, IDC_MAKECABEXE, szValue, MAX_PATH*2);
				g_PrepDoc->SetMakeCabExe(szValue);
			}
			break;
	}
	return 0;
}


BOOL CALLBACK VerificationDialogProc (HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	static DWORD dwBuildType;
	NMHDR *pnmhdr;
	static STATEINFO StateInfo[NUM_FLAGS];
	static CCheckBoxListView *clvBuild;     

	switch (msg)
	{
	
		case WM_INITDIALOG:
			{

				dwBuildType = (DWORD)((PROPSHEETPAGE *)lParam)->lParam;

				switch (dwBuildType)
				{
					case BUILD_UPDATEFILELIST:
						EnableWindow ( GetDlgItem (hWnd, IDC_BUILD),			FALSE );
						EnableWindow ( GetDlgItem (hWnd, IDC_VERIFYTITLE),		FALSE );
						EnableWindow ( GetDlgItem (hWnd, IDC_VERIFYTEXT1),		FALSE );
						EnableWindow ( GetDlgItem (hWnd, IDC_VERIFYTEXT2),		FALSE );
					break;
				}


				ClearStateInfo (StateInfo);
				clvBuild = new CCheckBoxListView;
				
				if (clvBuild->Create (g_hAppInst, GetDlgItem (hWnd, IDC_BUILD)))
				{
					clvBuild->AddColumn ("",80);
					clvBuild->AddColumn ("",20);
				

					AddCheckBoxImageList(clvBuild->GetLvHwnd());

					DWORD dwBuildFlags = g_PrepDoc->GetBuildFlags ();

					InitializeStateFromData (dwBuildFlags, StateInfo);

					clvBuild->AddItem (&StateInfo[BF_RTL]);
					clvBuild->AddItem (&StateInfo[BF_OEM]);
					clvBuild->AddItem (&StateInfo[BF_USA]);
					clvBuild->AddItem (&StateInfo[BF_JPN]);
					clvBuild->AddItem (&StateInfo[BF_GER]);
					clvBuild->AddItem (&StateInfo[BF_FRA]);
					clvBuild->AddItem (&StateInfo[BF_SPA]);
					clvBuild->AddItem (&StateInfo[BF_DBCS]);
					clvBuild->AddItem (&StateInfo[BF_APP1]);
					clvBuild->AddItem (&StateInfo[BF_APP2]);
					clvBuild->AddItem (&StateInfo[BF_APP3]);
					clvBuild->Refresh (); 
				}

				return true;
			}
		case WM_NOTIFY:
			pnmhdr = (NMHDR *)(lParam);
			if (pnmhdr->code == PSN_APPLY)
			{
				DWORD dwBuildFlags = 0;
										
				SetBuildFlagsFromTriState (&dwBuildFlags, StateInfo);
				g_PrepDoc->SetBuildFlags (dwBuildFlags);
				g_PrepDoc->SetListDirtyState (true);
				UpdateWindowText();
				RemoveCheckBoxImageList(clvBuild->GetLvHwnd());
				clvBuild->Delete();
				delete clvBuild;

			}
			else
			if (pnmhdr->code == PSN_QUERYCANCEL)
			{
				RemoveCheckBoxImageList(clvBuild->GetLvHwnd());
				clvBuild->Delete();
				delete clvBuild;
			}
	}
	return 0;
}


bool InvokeBuildDialog (HWND hwndOwner, DWORD dwBuildType)	
{    
	PROPSHEETPAGE psp[5];
    PROPSHEETHEADER psh;    
	
	psp[0].dwSize = sizeof(PROPSHEETPAGE);
    psp[0].dwFlags = PSP_USEICONID | PSP_USETITLE;
    psp[0].hInstance = g_hInst;
    psp[0].pszTemplate = MAKEINTRESOURCE(IDD_PROP_GENERAL);
    psp[0].pszIcon = NULL;
    psp[0].pfnDlgProc = BuildDialogProc;
    psp[0].pszTitle = "General";
	psp[0].lParam = dwBuildType;  // pass the build type along to the first panel so it can set the property sheet title.
    psp[0].pfnCallback = NULL;    

	psp[1].dwSize = sizeof(PROPSHEETPAGE);
    psp[1].dwFlags = PSP_USEICONID | PSP_USETITLE;
    psp[1].hInstance = g_hInst;
    psp[1].pszTemplate = MAKEINTRESOURCE(IDD_PROP_INJECTSETTINGS);
    psp[1].pszIcon = NULL;
    psp[1].pfnDlgProc = AdvancedInjectSettingsDialogProc;
    psp[1].pszTitle = "Injection";
	psp[1].lParam = dwBuildType;;
    psp[1].pfnCallback = NULL;    

	psp[2].dwSize = sizeof(PROPSHEETPAGE);
    psp[2].dwFlags = PSP_USEICONID | PSP_USETITLE;
    psp[2].hInstance = g_hInst;
    psp[2].pszTemplate = MAKEINTRESOURCE(IDD_PROP_STRINGSETTINGS);
    psp[2].pszIcon = NULL;
    psp[2].pfnDlgProc = StringSettingsDialogProc;
    psp[2].pszTitle = "Static Strings";
	psp[2].lParam = dwBuildType;;
    psp[2].pfnCallback = NULL;    

	psp[3].dwSize = sizeof(PROPSHEETPAGE);
    psp[3].dwFlags = PSP_USEICONID | PSP_USETITLE;
    psp[3].hInstance = g_hInst;
    psp[3].pszTemplate = MAKEINTRESOURCE(IDD_PROP_VERIFICATION);
    psp[3].pszIcon = NULL;
    psp[3].pfnDlgProc = VerificationDialogProc;
    psp[3].pszTitle = "Verification";
	psp[3].lParam = dwBuildType;;
    psp[3].pfnCallback = NULL; 

	psp[4].dwSize = sizeof(PROPSHEETPAGE);
    psp[4].dwFlags = PSP_USEICONID | PSP_USETITLE;
    psp[4].hInstance = g_hInst;
    psp[4].pszTemplate = MAKEINTRESOURCE(IDD_PROP_TRIALVERSION);
    psp[4].pszIcon = NULL;
    psp[4].pfnDlgProc = TrialVersionDialogProc;
    psp[4].pszTitle = "Trial Version";
	psp[4].lParam = dwBuildType;;
    psp[4].pfnCallback = NULL; 

	/*  -- add this in to allow setting of flag defaults on property page.
	psp[5].dwSize = sizeof(PROPSHEETPAGE);
    psp[5].dwFlags = PSP_USEICONID | PSP_USETITLE;
    psp[5].hInstance = g_hInst;
    psp[5].pszTemplate = MAKEINTRESOURCE(IDD_PROP_DEFAULTFLAGS);
    psp[5].pszIcon = NULL;
    psp[5].pfnDlgProc = TrialVersionDialogProc;
    psp[5].pszTitle = "Trial Version";
	psp[5].lParam = dwBuildType;;
    psp[5].pfnCallback = NULL; 
	*/

	psh.dwSize = sizeof(PROPSHEETHEADER);
    psh.dwFlags = PSH_USEICONID | PSH_PROPSHEETPAGE | PSH_USECALLBACK | PSH_NOAPPLYNOW ; 
    psh.hwndParent = hwndOwner;    
	psh.hInstance = g_hInst;
    psh.pszIcon = NULL;
    psh.pszCaption = (LPSTR) "Prepstub99";
    psh.nPages = sizeof(psp) / sizeof(PROPSHEETPAGE);    
	psh.nStartPage = 0;
    psh.ppsp = (LPCPROPSHEETPAGE) &psp;    
	psh.pfnCallback = NULL;

    if (PropertySheet(&psh) == IDOK)
	{
		return (ProcessBuildCommand (hwndOwner, dwBuildType));
	}

	return false;
}
