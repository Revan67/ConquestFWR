//--------------------------------------------------------------------------//
//                                                                          //
//                             Database.cpp                                 //
//                                                                          //
//                  COPYRIGHT (C) 1997 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*

  $Author: Ajackson $
  
    $Header: /Libs/Src/Tools/adb/database.CPP 13    11/25/03 11:07a Ajackson $
*/			    
//--------------------------------------------------------------------------//


#include "pch.h"

#include "UserDefaults.h"
#include "Resource.h"
#include "dbkeys.h"
#include "DbStuff.h"
#include "stdio.h"
#include "dbexport.h"
#include "dbimport.h"

#include <commctrl.h>
#include <DACOM.h>
#include <EventSys.h>
#include <FileSys.h>
#include <ViewCnst.h>
#include <HKEvent.h>
#include <IConnection.h>
#include <TComponent.h>
#include <stdlib.h>
#include <mmsystem.h>
#include <Document.h>
#include <ZMouse.h>
#include <FDump.h>
#include "TDocClient.h"
#include <richedit.h>

// #define BUILD_AUTOBINMORPH

#if defined(BUILD_AUTOBINMORPH)
	#define _APP_STRING_DATA_HEADER_H_
	#include "..\..\Viewer\Docuview\SymTable.h"
	#include <DBaseData.h>
	BOOL CALLBACK AutoBin_DlgProc(HWND hwnd, UINT message, UINT wParam, LPARAM lParam);
#endif

// #define BUILD_OPENDATABASEUSINGDIRECTORY

#include <string>
using namespace std;

//-------------------------------------------------------------------------------------

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//--------------------------------------------------------------------------
//-----------------------Game Global instances------------------------------
//--------------------------------------------------------------------------


struct ICOManager *DACOM=0;
HINSTANCE hInstance;
HWND hMainWindow;
BOOL32 NOGUI;
U32 wheelMsg;
S32 wheelAc=0;
extern char currentFileName[];
extern _STATE _state;

struct DocManager : DocumentClient
{
	BEGIN_DACOM_MAP_INBOUND(DocManager)
	DACOM_INTERFACE_ENTRY(IDocumentClient)
	END_DACOM_MAP()

	DEFMETHOD(OnUpdate) (struct IDocument *doc, const C8 *message = 0, void *parm = 0);

};

extern DocManager *docManager;

IViewConstructor2 * PARSER;


static LPTOP_LEVEL_EXCEPTION_FILTER prevExceptionHandler;

BOOL32 WriteToText (HWND hwnd, const char * fileName);

#include "dbTreeView.h"
DbTreeView g_DbTreeView;

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
//
static void clean_up (void)
{
	if (pPreprocessBlock)
	{
		free(pPreprocessBlock);
		pPreprocessBlock = 0;
	}

	extern char * pPreprocessResources;
	if( pPreprocessResources )
	{
		free(pPreprocessResources);
		pPreprocessResources = 0;
	}


	delete docManager;
	docManager =0;

	if (PARSER)
	{
		PARSER->Release();
		PARSER =0;
	}
	
	
	if (DEFAULTS)
	{
		DEFAULTS->StoreDefaults();
		DEFAULTS->Release();
		DEFAULTS = 0;
	} 
	
	if (DACOM)
	{
		DACOM->ShutDown();
		DACOM=0;
	}
}
//--------------------------------------------------------------------------
//
static BOOL32 __cdecl HeapErrorProc (struct IHeap *heap, U32 error, char *msg, ...)
{
	MessageBox(hMainWindow,msg,"Conquest heap error",MB_OK);
	
	return 0;
}
//--------------------------------------------------------------------------
//
BOOL32 SetupParser (void)
{
	DACOMDESC desc = "IViewConstructor";
	BOOL32 result = 0;
	COMPTR<IViewConstructor> parser;
  
	if (DACOM->CreateInstance(&desc, parser) == GR_OK)
	{
		parser->QueryInterface("IViewConstructor2", (void **) & PARSER);
		result = 1;
	}
	
	return result;
}
BOOL CALLBACK RenameDlgProc(HWND hwnd, UINT message, UINT wParam, LPARAM lParam);

//--------------------------------------------------------------------------
//
BOOL CALLBACK FileDlgProc(HWND hwnd, UINT message, UINT wParam, LPARAM lParam)
{
	if (message == WM_INITDIALOG)
        SetWindowLong(hwnd, DWL_USER, lParam);
	char *name = (char *)GetWindowLong(hwnd, DWL_USER);
	
	switch (message)
	{
	case WM_INITDIALOG:
		SetDlgItemText(hwnd,IDC_INFO,"Enter name of new database file");
		SetDlgItemText(hwnd,IDC_EDIT1, "");
		SetFocus(GetDlgItem(hwnd,IDC_EDIT1));
		break;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
			GetDlgItemText(hwnd, IDC_EDIT1, name, 64);
			
			if (name[0] == 0)
				MessageBox(hwnd,"Operation cancelled","Bad Name",MB_OK);
			
			EndDialog(hwnd, 0);
			break;
		case IDCANCEL:
			EndDialog(hwnd, 1);
			break;
		}
		break;
	}
	
	return 0;
	
}

//--------------------------------------------------------------------------
//
BOOL32 CALLBACK Bob(struct IViewConstructor2 * cnst,const char *typeName,Symbol * type,void *context)
{
	const char *string;

	switch (_state)
	{
	case BT_:
		string = "BT_";
		break;
	case MT_:
		string = "MT_";
		break;
	case GT_:
		string = "GT_";
		break;
	default:
		string = "";
		break;
	}

	if (strncmp(typeName, string, 3) == 0)
		SendDlgItemMessage((HWND)context,IDC_LIST1,LB_ADDSTRING,0,(LPARAM)typeName);
	
	return 1;
}

void EnumTypes()
{
	SendDlgItemMessage(hMainWindow,IDC_LIST1,LB_RESETCONTENT,0,0);
	PARSER->EnumerateTypes(Bob,hMainWindow);
	SendDlgItemMessage(hMainWindow,IDC_LIST1,LB_SETCURSEL,0,0);
	SetFocus(GetDlgItem(hMainWindow,IDC_LIST1));
}
//----------------------------------------------------------------------------
//
LONG CALLBACK ListControlProcedure(HWND hwnd, UINT message, UINT wParam, LONG lParam)
{
 	WNDPROC oldProc = (WNDPROC) GetWindowLong(hwnd, GWL_USERDATA);

	switch (message)
	{
	case WM_KEYDOWN:
		switch (LOWORD(wParam))
		{
		case VK_DELETE:
			PostMessage(GetParent(hwnd), WM_COMMAND, MAKELONG(IDC_DELETE, 0), (LONG)hwnd);
			return 0;
			
		case VK_INSERT:
			PostMessage(GetParent(hwnd), WM_COMMAND, MAKELONG(IDC_CLONE, 0), (LONG)hwnd);
			return 0;

		case VK_RETURN:
			PostMessage(hwnd,WM_COMMAND,MAKEWPARAM(ID_EDIT,0),0);
			return 0;
		}
		break;

		case LB_ADDSTRING:
		{
			HWND h = ::GetDlgItem( hMainWindow, IDC_LIST2 );
			if( hwnd == h  )
			{
				if( g_DbTreeView.IsValid() )
				{
					int index = ::SendMessage(h, LB_GETCOUNT, 0, 0);
					g_DbTreeView.CreateBranchByParsing( (char*)lParam, "!!", index );
				}
			}
			break;
		}

		case LB_RESETCONTENT:
		{
			HWND h = ::GetDlgItem( hMainWindow, IDC_LIST2 );
			if( hwnd == h )
			{
				if( g_DbTreeView.IsValid() )
				{
					g_DbTreeView.Clear();
				}
			}
			break;
		}
	}

	return CallWindowProc(oldProc, hwnd, message, wParam, lParam);
}
//--------------------------------------------------------------------------
//
bool GetSelectedItem( char* selectName, U32& selection, HWND hwnd )
{
	if( g_DbTreeView.IsValid() )
	{
		int sel = g_DbTreeView.GetCurrentSelectionData();

		if( sel >= 0 )
		{
			selection = (DWORD)sel;
			SendDlgItemMessage(hwnd,IDC_LIST2,LB_GETTEXT,selection,(LPARAM)selectName);
			return true;
		}
		return false;
	}
	else if ((selection = SendDlgItemMessage(hwnd,IDC_LIST2,LB_GETCURSEL,0,0)) != LB_ERR)
	{
		SendDlgItemMessage(hwnd,IDC_LIST2,LB_GETTEXT,selection,(LPARAM)selectName);
		return true;
	}
	return false;
}
//--------------------------------------------------------------------------
//
BOOL CALLBACK DlgProc(HWND hwnd, UINT message, UINT wParam, LPARAM lParam)
{
	U32 selection;
	char selectName[64];
	USER_DEFAULTS *iggy;

	if (message == wheelMsg)
	{
		S32 currentIdx = SendDlgItemMessage(hwnd, GetDlgCtrlID(GetFocus()),LB_GETTOPINDEX,0,0);
		wheelAc += 2*(short)wParam;
		currentIdx -= wheelAc/WHEEL_DELTA;
		if (currentIdx <0)
			currentIdx = 0;
		wheelAc %= WHEEL_DELTA;
		SendDlgItemMessage(hwnd, GetDlgCtrlID(GetFocus()),LB_SETTOPINDEX,(WPARAM)currentIdx,0);
	}
	
	switch (message)	
	{
		case WM_INITDIALOG:
			{
				g_DbTreeView.Init( GetDlgItem(hwnd,IDC_LIST2) );

				hMainWindow = hwnd;
				HWND hList;
				WNDPROC oldProc;

				SetClassLong(hwnd, GCL_HICON, (LONG) LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1)));

				hList = GetDlgItem(hwnd, IDC_LIST2);
				if ((oldProc = (WNDPROC) GetWindowLong(hList, GWL_WNDPROC)) != 0)
				{
					SetWindowLong(hList, GWL_USERDATA, (LONG) oldProc);
					SetWindowLong(hList, GWL_WNDPROC, (LONG) ListControlProcedure);
				}
				
				hList = GetDlgItem(hwnd, IDC_LIST1);
				if ((oldProc = (WNDPROC) GetWindowLong(hList, GWL_WNDPROC)) != 0)
				{
					SetWindowLong(hList, GWL_USERDATA, (LONG) oldProc);
					SetWindowLong(hList, GWL_WNDPROC, (LONG) ListControlProcedure);
				}
				iggy = DEFAULTS->GetDefaults();
				_state = iggy->_state;
				switch (_state)
				{
				case BT_:
					CheckRadioButton(hwnd,IDC_BT,IDC_GT,IDC_BT);
					break;
				case MT_:
					CheckRadioButton(hwnd,IDC_BT,IDC_GT,IDC_MT);
					break;
				case GT_:
					CheckRadioButton(hwnd,IDC_BT,IDC_GT,IDC_GT);
					break;
				}
			}
			break;

		case WM_ENTERIDLE:
			if (wParam == MSGF_DIALOGBOX)
				PostMessage((HWND)lParam, WM_USER+1, 0, 0);
			break;
		
		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
				case IDOK:
	   				PostMessage(hwnd, WM_COMMAND, MAKELONG(GetDlgCtrlID(GetFocus()),LBN_DBLCLK), 0);
					break;
				
				case IDC_CLONE:
					if( GetSelectedItem(selectName,selection,hwnd) )
					{
						if( g_DbTreeView.IsValid() )
						{
							char defname[256];
							g_DbTreeView.GetRootToSelected( defname, 256, "!!" );
							Clone(selectName,defname);
						}
						else
							Clone(selectName,"");
					}
					break;
				
				case IDC_LIST2:
					switch (HIWORD(wParam))
					{
					case LBN_DBLCLK:
						PostMessage(hwnd,WM_COMMAND,MAKEWPARAM(ID_EDIT,0),0);
						break;
					}
					break;
				
				case IDC_LIST1:
					switch (HIWORD(wParam))
					{
					case LBN_DBLCLK:
						PostMessage(hwnd,WM_COMMAND,MAKEWPARAM(IDC_NEWARCH,0),0);
						break;
					case LBN_SELCHANGE:
						RefreshList();
						break;
					}
					break;

				case IDC_BT:
					_state = BT_;

					iggy = DEFAULTS->GetDefaults();
					iggy->_state = _state;
					EnumTypes();
					RefreshList();
					break;

				case IDC_MT:
					_state = MT_;

					iggy = DEFAULTS->GetDefaults();
					iggy->_state = _state;
					EnumTypes();
					RefreshList();
					break;

				case IDC_GT:
					_state = GT_;

					iggy = DEFAULTS->GetDefaults();
					iggy->_state = _state;
					EnumTypes();
					RefreshList();
					break;

				case ID_WRITE_TEXT:
					if (mainDoc)
					{
						WriteToText(hwnd, currentFileName);
					}
					break;

				case ID_EXIT:
					if (KillDB())
						DestroyWindow(hwnd);
					break;

				case IDC_NEWARCH:
					if (mainDoc)
					{
						if ((selection = SendDlgItemMessage(hwnd,IDC_LIST1,LB_GETCURSEL,0,0)) != LB_ERR)
						{
							SendDlgItemMessage(hwnd,IDC_LIST1,LB_GETTEXT,selection,(LPARAM)selectName);

							if( g_DbTreeView.IsValid() )
							{
								char defname[256];
								g_DbTreeView.GetRootToSelected( defname, 256, "!!" );
								NewArchetype(selectName,defname);
							}
							else
								NewArchetype(selectName,"");
						}
						RefreshList();						
					}
					else 
						MessageBox(hwnd,"You must open a database first","Life is hard",MB_OK);
					break;
				
				case ID_EDIT:
					if (mainDoc)
					{
						if( GetSelectedItem(selectName,selection,hwnd) )
						{
							OpenArchetype(selectName);
						}
					}
					else MessageBox(hwnd,"You must open a database first","Life is hard",MB_OK);
					break;

				case ID_SAVE:
					SaveDB();
					break;

#if defined(BUILD_OPENDATABASEUSINGDIRECTORY)
				case ID_OPEN_DIR:
				{
					void OpenDatabaseUsingDirectory();
					OpenDatabaseUsingDirectory();
					break;
				}
#endif // BUILD_OPENDATABASEUSINGDIRECTORY
				
				case ID_OPEN:
					{
						char name[64];
						DOCDESC ddesc;
						//	U32 attrib;
						//		HANDLE handle;
						//		WIN32_FIND_DATA dataFound;
						//		char path[MAX_PATH];
						DEFAULTS->GetInputFilename(name,IDS_DB_FILTER);

						if (name[0])
						{
							if (KillDB())
							{
								OpenDB(name);
							}
						}
					}	
					break;

				case ID_NEW:
					{
						if (KillDB())
							NewDB();
					}
					break;

				case ID_SAVE_AS:
					SaveAs();
					break;

				case IDC_DELETE:
					if (mainDoc)
					{
						if( GetSelectedItem(selectName,selection,hwnd) )
						{
							// are you sure?
							if( ::MessageBox(hwnd,selectName, "Delete Archetype?", MB_YESNO) == IDYES )
							{
								saved = FALSE;
								DeleteArchetype(selectName);
							}
						}
						UpdateTitle();
						RefreshList();
					}
					break;

				case ID_PREFS:
					DoPrefsDialog();
					break;

				case IDC_RENAME:
				{
					if (mainDoc)
					{
						if( GetSelectedItem(selectName,selection,hwnd) )
						{
							char nameBuffer[128];
							strcpy( nameBuffer, selectName );

							//PlaySound("WAV_OOH",hInstance,SND_RESOURCE | SND_ASYNC);
							LPTSTR dlgTemplate = MAKEINTRESOURCE(IDD_NEW_ARCHETYPE);
							int ret = DialogBoxParam(hInstance, dlgTemplate, hMainWindow, RenameDlgProc, (LPARAM)&nameBuffer );
							if( ret == IDOK )
							{
								RenameArchetype(selectName, nameBuffer);
							}
						}
						saved = FALSE;
						UpdateTitle();
						RefreshList();
					}
					break;
				}

				case ID_EXPORT_ACCESS:
				{
					if( mainDoc )
					{
						MicrosoftDatabaseExport mdbe;
						mdbe.Init( mainDoc, "ignore", 0 );
						mdbe.Execute( hInstance, hMainWindow );
					}

					break;
				}

				case ID_EXPORT_EXCEL:
				{
					if (mainDoc)
					{
						extern U32 CountNumFilesInMainDoc (void);
						ExcelExport export;
						export.Init( mainDoc, "outfile", CountNumFilesInMainDoc() );
						export.Execute( hInstance, hMainWindow );
					}
					break;
				}

				case ID_EXPORT_DOS:
				{
					if (mainDoc)
					{
						char fname[128];
						strcpy( fname, strrchr(currentFileName,'\\') + 1 );
						char* ext = strrchr(fname,'.');
						if( ext )
						{
							*ext = 0;

							extern U32 CountNumFilesInMainDoc (void);
							PathExport export;
							export.Init( mainDoc, fname, CountNumFilesInMainDoc() );
							export.Execute( hInstance, hMainWindow );
						}
					}
					break;
				}

				case IDC_IMPORT_MDB:
				{
					if (mainDoc)
					{
						AccessFileImport afi;
						afi.Init( mainDoc );
						afi.Execute( hInstance, hMainWindow );
						saved = FALSE;
						UpdateTitle();
						RefreshList();
					}
					break;
				}

				case IDC_IMPORT_ADB:
				{
					if (mainDoc)
					{
						ADBFileImport afi;
						afi.Init( mainDoc );
						afi.Execute( hInstance, hMainWindow );
						saved = FALSE;
						UpdateTitle();
						RefreshList();
					}
					break;
				}

				case IDC_IMPORT_CHANGELIST:
				{
					if (mainDoc)
					{
						ChangeListImport cli;
						cli.Init( mainDoc );
						cli.Execute( hInstance, hMainWindow );
						saved = FALSE;
						UpdateTitle();
						RefreshList();
					}
					break;
				}

#if defined(BUILD_AUTOBINMORPH)
				case ID_TOOL_AUTOBIN:
				{
					if ((selection = SendDlgItemMessage(hwnd,IDC_LIST1,LB_GETCURSEL,0,0)) != LB_ERR)
					{
						SendDlgItemMessage(hwnd,IDC_LIST1,LB_GETTEXT,selection,(LPARAM)selectName);
						CreateDialogParam( hInstance, MAKEINTRESOURCE(IDD_AUTOBIN), hwnd, AutoBin_DlgProc, (LPARAM)selectName );
					}
					break;
				}
#endif // BUILD_AUTOBINMORPH
			}
			break;

		case WM_CLOSE:
			SendMessage(hwnd, WM_COMMAND,ID_EXIT,0);
			break;

		case WM_NOTIFY:
		{
			if( g_DbTreeView.IsValid() )
			{
				g_DbTreeView.OnNotify( hwnd, wParam, lParam );
			}
			break;
		}

		case WM_DESTROY:
			PostQuitMessage(0);
			break;
	}
	return 0;
}

int WINAPI WinMain (HINSTANCE _hInstance, HINSTANCE hPrevInstance, LPSTR lpszCmdLine, int nCmdShow)
{
	HMODULE hRichEdit = ::LoadLibrary("RICHED20.DLL");

	COMPTR<IDAConnectionPoint> connection;
	//	U32 eventHandle;
	
	hInstance = _hInstance;
	
	atexit(clean_up);
	DACOM = DACOM_Acquire();
	
#ifdef _END_USER
	DACOM->SetINIConfig("[Libraries]\r\n..\\..\\Libs\\ExplicitDLL\\Docuview.dll\r\n..\\..\\Libs\\ExplicitDLL\\Dosfile.dll", DACOM_INI_STRING);
#else
	DACOM->SetINIConfig("[Libraries]\r\nDocuview.dll\r\nDosfile.dll", DACOM_INI_STRING);
#endif

	InitCommonControls();  	// This MUST be called once per instance
	// to register the common controls

	extern BOOL __stdcall AfxWinInit( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow );
	AfxWinInit( hInstance, hPrevInstance, lpszCmdLine, nCmdShow );
	
	SetupParser();
	
	if (CreateUserDefaults() == 0)
		return 0;
	DEFAULTS->LoadDefaults();
	
	hMainWindow = CreateDialog(hInstance, MAKEINTRESOURCE(IDD_DIALOG1), 0, DlgProc);

	InitDB();

	wheelMsg = RegisterWindowMessage(MSH_MOUSEWHEEL);
	
	MSG msg;
	
	while (GetMessage(&msg,0,0,0))
	{
		if (IsDialogMessage(hMainWindow, &msg) == 0)
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	
	::FreeLibrary( hRichEdit );

	return 0;
}

//-----------------------------------------------------------------------------------------------------


BOOL CALLBACK RenameDlgProc(HWND hwnd, UINT message, UINT wParam, LPARAM lParam)
{
	if (message == WM_INITDIALOG)
        SetWindowLong(hwnd, DWL_USER, lParam);
	char *name = (char *)GetWindowLong(hwnd, DWL_USER);

	switch( message )
	{
		case WM_INITDIALOG:
		{
			SetWindowText( hwnd, "Rename Archetype" );
			SetDlgItemText(hwnd,IDC_INFO,"Enter name of new name for Archetype.");
			SetDlgItemText(hwnd,IDC_EDIT1, name);
			SetFocus(GetDlgItem(hwnd,IDC_EDIT1));
			break;
		}
		case WM_COMMAND:
		{
			switch (LOWORD(wParam))
			{
			case IDOK:
			{
				GetDlgItemText(hwnd, IDC_EDIT1, name, 64);
				
				if (name[0] == 0)
					EndDialog(hwnd, IDCANCEL);
				else			
					EndDialog(hwnd, IDOK);
				break;
			}
			case IDCANCEL:
				EndDialog(hwnd, IDCANCEL);
				break;
			}
			break;
		}
	}
	return 0;
}

//-----------------------------------------------------------------------------------------------------

#if defined(BUILD_AUTOBINMORPH)

BOOL32 CALLBACK AutoBin_FillBinClassDlgProc(struct IViewConstructor2 * cnst,const char *typeName,Symbol * type,void *context)
{
	if( !strcmp(typeName, "BINARYCLASS") )
	{
		Symbol* child = type->datatype;

		while( child )
		{
			int idx = SendMessage( (HWND)context, CB_ADDSTRING, 0, (LPARAM)child->name );
			SendMessage( (HWND)context, CB_SETITEMDATA, idx, (LPARAM)child->constval.intnum );
			child = child->link;
		}
		return 0;
	}
	return 1;
}

struct AutoBinMorph_Struct
{
	char btClass[64];
	char binClass[64];
	BINARYCLASS binClassEnum;
	HANDLE hFind;
	WIN32_FIND_DATA findData;
	IDocument* doc;

	AutoBinMorph_Struct()
	{
		strcpy(btClass,"");
		strcpy(btClass,"");
		hFind = 0;
		doc = NULL;
		ZeroMemory( &findData, sizeof(findData) );
	}

	AutoBinMorph_Struct( const AutoBinMorph_Struct& _abm )
	{
		strcpy(btClass,_abm.btClass);
		strcpy(btClass,_abm.btClass);
		binClassEnum = _abm.binClassEnum;
		hFind = _abm.hFind;
		doc = _abm.doc;
		memcpy( &findData, &_abm.findData, sizeof(findData) );
	}

	bool Next()
	{
		if( doc->FindNextFile(hFind,&findData) == false )
		{
			hFind = INVALID_HANDLE_VALUE;
			return false;
		}
		return true;
	}
};

BOOL32 CALLBACK AutoBin_Morph( AutoBinMorph_Struct& _abm )
{
	if( _abm.hFind == INVALID_HANDLE_VALUE )
	{
		// handle end of files
		__asm int 3;
		return false;
	}

	// make sure this not a silly "." entry
	if( _abm.findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
	{
		if (_abm.findData.cFileName[0] == '.')
		{
			// advance to the next directory
			if( _abm.doc->FindNextFile(_abm.hFind,&_abm.findData) == 0 )
			{
				// continue to the next file/dir
				AutoBinMorph_Struct abm(_abm);
				return AutoBin_Morph( abm );
			}
		}
		else
		{
			while( _abm.hFind != INVALID_HANDLE_VALUE )
			{
				if( !strcmp(_abm.btClass,_abm.findData.cFileName) )
				{
					AutoBinMorph_Struct abm(_abm);
					string dirMask = string("\\") + string(abm.findData.cFileName);

					// get all the files from this directory
					abm.doc->SetCurrentDirectory( dirMask.c_str() );
					abm.hFind = abm.doc->FindFirstFile("*.*",&abm.findData);
					AutoBin_Morph(abm);

					return true;
				}

				// go to next directory
				_abm.Next();
			}
		}

	}
	else
	{
		// show all files in this directory
		while( _abm.hFind != INVALID_HANDLE_VALUE )
		{
			DAFILEDESC fdesc = _abm.findData.cFileName;
			COMPTR<IFileSystem> file;
			if( _abm.doc->CreateInstance(&fdesc,file) == GR_OK )
			{
				if( file->GetFileSize() >= sizeof(BASE_BINARY_CLASS) )
				{
					BASE_BINARY_CLASS clazz;
					DWORD dwSize = sizeof(clazz);
					file->ReadFile( 0, &clazz, dwSize, &dwSize );
					clazz.objClass = OC_BINARY;
					clazz.bEditDropable = false;
					clazz.binClass = _abm.binClassEnum;
					file->SetFilePointer( 0, 0 );
					file->WriteFile( 0, &clazz, dwSize, &dwSize );
				}
			}
			_abm.Next();
		}
	}

	return false;
}

BOOL CALLBACK AutoBin_DlgProc(HWND hwnd, UINT message, UINT wParam, LPARAM lParam)
{
	switch( message )
	{
		case WM_INITDIALOG:
		{
			PARSER->EnumerateTypes(AutoBin_FillBinClassDlgProc,GetDlgItem(hwnd,IDC_COMBO_AUTOBIN));
			SetDlgItemText(hwnd,IDC_EDIT_BTNAME,(const char*)lParam);
			SetWindowLong(hwnd, DWL_USER, lParam);
			ShowWindow( hwnd, SW_NORMAL );
			break;
		}
		case WM_COMMAND:
		{
			switch (LOWORD(wParam))
			{
				case IDOK:
				{
					int curSel = SendMessage( GetDlgItem(hwnd,IDC_COMBO_AUTOBIN), CB_GETCURSEL, 0, 0 );

					if( mainDoc && mainDoc->SetCurrentDirectory("\\") != 0 && curSel != CB_ERR )
					{
						AutoBinMorph_Struct abm;
						GetDlgItemText(hwnd, IDC_EDIT_BTNAME, abm.btClass, 64 );
						GetDlgItemText(hwnd, IDC_COMBO_AUTOBIN, abm.binClass, 64 );
						abm.hFind = mainDoc->FindFirstFile("*.*", &abm.findData );
						abm.binClassEnum = (enum BINARYCLASS) SendMessage( GetDlgItem(hwnd,IDC_COMBO_AUTOBIN), CB_GETITEMDATA, curSel, 0 );
						abm.doc = mainDoc;
						AutoBin_Morph( abm );
					}

					EndDialog(hwnd, IDOK);
					break;
				}
				case IDCANCEL:
				{
					EndDialog(hwnd, IDCANCEL);
					break;
				}
			}
			break;
		}
	}
	return 0;
}

#endif // BUILD_AUTOBINMORPH

//----------------------------------------------------------------------------------
// OpenDatabaseUsingDirectory
//----------------------------------------------------------------------------------

#if defined(BUILD_OPENDATABASEUSINGDIRECTORY)

int CountFilesInDir( const char* _directory )
{
	int cnt = 0;

	char mask[MAX_PATH];
	strcpy( mask, _directory );
	strcat( mask, "\\*.*" );

	WIN32_FIND_DATA fdata;
	HANDLE h = ::FindFirstFile( mask, &fdata );

	while( h != INVALID_HANDLE_VALUE )
	{
		if( fdata.cFileName[0] != '.' )
		{
			if( fdata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
			{
				::SetCurrentDirectory( _directory );
				cnt += CountFilesInDir( fdata.cFileName );
				::SetCurrentDirectory( ".." );
			}
			else
			{
				cnt++;
			}
		}

		if( ::FindNextFile(h,&fdata) == false )
		{
			h = INVALID_HANDLE_VALUE;
		}
	}

	return cnt;
}

//-----------------------------------------------------------------------------------------------------

DWORD WINAPI OpenDatabaseUsingDirectory_ThreadProc( LPVOID lpParameter )
{
	HWND hwnd = (HWND)lpParameter;
	HWND prog = GetDlgItem(hwnd,IDC_PROGRESS1);
	
	while(true)
	{
		MSG msg;

		if( PeekMessage(&msg,prog,0,0,PM_REMOVE) )
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		if( mainDoc )
		{
			U32 numDox = mainDoc->GetNumChildren();
			SendMessage( prog, PBM_SETPOS, numDox, 0 );
		}
	}
}

//-----------------------------------------------------------------------------------------------------

#include <shlobj.h>

BOOL CALLBACK OpenDatabaseUsingDirectory_DlgProc(HWND hwnd, UINT message, UINT wParam, LPARAM lParam)
{
	switch( message )
	{
		case WM_INITDIALOG:
		{
			SetDlgItemText(hwnd,IDC_STATIC1,"Opening Database...");
			SetWindowText(hwnd,"Please wait while opening");
			ShowWindow( hwnd, SW_NORMAL );

			SendMessage( GetDlgItem(hwnd,IDC_PROGRESS1), PBM_SETPOS, 0, 0 );
			SendMessage( GetDlgItem(hwnd,IDC_PROGRESS1), PBM_SETRANGE, 0, MAKELPARAM(0, lParam) );
			SendMessage( GetDlgItem(hwnd,IDC_PROGRESS1), PBM_SETSTEP, 1, 0 );

			//HANDLE h = ::CreateThread( NULL, NULL, OpenDatabaseUsingDirectory_ThreadProc, hwnd, 0,NULL );
			//SetWindowLong(hwnd, DWL_USER, (DWORD)h);
			break;
		}
		case WM_DESTROY:
		{
			//HANDLE h = (HANDLE)::GetWindowLong(hwnd, DWL_USER);
			//::CloseHandle( h );
			break;
		}
	}
	return 0;
}

void OpenDatabaseUsingDirectory()
{
	BROWSEINFO bi;
	TCHAR szDir[MAX_PATH];
	LPMALLOC pMalloc;
	
	if (SUCCEEDED(SHGetMalloc(&pMalloc))) 
	{
		ZeroMemory(&bi,sizeof(bi));
		bi.hwndOwner = NULL;
		bi.pszDisplayName = 0;
		bi.pidlRoot = 0;
		bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_STATUSTEXT;
		bi.lpfn = 0; //BrowseCallbackProc;

		LPITEMIDLIST lpItemIDList = SHBrowseForFolder(&bi);
		if (lpItemIDList) 
		{
			if (SHGetPathFromIDList(lpItemIDList,szDir)) 
			{
				LPARAM count = CountFilesInDir(szDir);
				HWND h = CreateDialogParam( hInstance, MAKEINTRESOURCE(IDD_PROGBAR), hMainWindow, OpenDatabaseUsingDirectory_DlgProc, count );
				OpenDatabaseViaDir(szDir);
				EndDialog( h, 0 );
			}
			pMalloc->Free(lpItemIDList); 
			pMalloc->Release();
		}
	}
}

#endif // BUILD_OPENDATABASEUSINGDIRECTORY

//----------------------------------------------------------------------------------
//------------------------END Database.cpp--------------------------------------------
//----------------------------------------------------------------------------------
