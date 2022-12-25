//--------------------------------------------------------------------------//
//                                                                          //
//                               ADBFileImport.cpp 							//
//                                                                          //
//                  COPYRIGHT (C) 2003 Fever Pitch Studios, INC.            //
//                                                                          //
//--------------------------------------------------------------------------//
/*
	$Header: /Libs/Src/Tools/adb/ADBFileImport.cpp 4     11/25/03 11:07a Ajackson $

	$Author: Ajackson $
*/
//--------------------------------------------------------------------------//

#include "pch.h"
#include "dbImport.h"

#include "typedefs.h"
#include "Globals.h"
#include "filesys.h"
#include "resource.h"
#include "TSmartPointer.H"

#include <commctrl.h> 
#include <commdlg.h>
#include <string.h>
#include <malloc.h>
#include <assert.h>
#include <windowsx.h>

//-------------------------------------------------------------------------------------
// STL Stuff
//-------------------------------------------------------------------------------------

#pragma warning(disable : 4786)
#include <map>
#include <string>
#include <list>
using namespace std;

//-------------------------------------------------------------------------------------
// DEBUG_NEW
//-------------------------------------------------------------------------------------

//#ifdef _DEBUG
//#define new DEBUG_NEW
//#undef THIS_FILE
//static char THIS_FILE[] = __FILE__;
//#endif

//-------------------------------------------------------------------------------------
// DbImport Globals
//-------------------------------------------------------------------------------------

extern U32 IFILESYSTEM_CountNumFiles( DWORD& count, IFileSystem* file );

//------------------------------------------------------------------------------
// ADBFileImport Namespace
//------------------------------------------------------------------------------

//namespace ns_ADBFileImport
//{
	struct StringList : list< string >{};
	StringList fileList;
//}
//using namespace ns_ADBFileImport;

//--------------------------------------------------------------------------
// DlgProc_ADBFileImport
//--------------------------------------------------------------------------

BOOL CALLBACK DlgProc_ADBFileImport(HWND hwnd, UINT message, UINT wParam, LPARAM lParam)
{
	if (message == WM_INITDIALOG)
        SetWindowLong(hwnd, DWL_USER, lParam);
	ADBFileImport* THIS = (ADBFileImport*)GetWindowLong(hwnd, DWL_USER);

	switch( message )
	{
		case WM_INITDIALOG:
		{
			DWORD count = 0;
			THIS->outFile->SetCurrentDirectory("\\");
			THIS->inFile->SetCurrentDirectory("\\");
			IFILESYSTEM_CountNumFiles( count, THIS->inFile );

			SetWindowText( hwnd, "ADB File Import" );
			SetDlgItemText(hwnd,IDC_STATIC1,"Importing from ADB file...");
			SetTimer( hwnd, 101, 5, NULL );
			SendMessage( GetDlgItem(hwnd,IDC_PROGRESS1), PBM_SETPOS, 0, 0 );
			SendMessage( GetDlgItem(hwnd,IDC_PROGRESS1), PBM_SETRANGE, 0, MAKELPARAM(0, count) );
			SendMessage( GetDlgItem(hwnd,IDC_PROGRESS1), PBM_SETSTEP, 1, 0 );
			break;
		}
		case WM_COMMAND:
		{
			switch (LOWORD(wParam))
			{
				case IDCANCEL:
					EndDialog(hwnd, IDCANCEL);
					break;
			}
			break;
		}
		case WM_TIMER:
		{
			if( THIS->Update( GetDlgItem(hwnd,IDC_PROGRESS1) ) == false )
			{
				THIS->Uninit();
				EndDialog(hwnd, IDOK);
			}
			break;
		}
	}
	return 0;
}

//--------------------------------------------------------------------------
// DlgProc_SelectTypes
//--------------------------------------------------------------------------

void SelectTypes_ExcludeFile( const char* mask )
{
	for( StringList::iterator git = fileList.begin(); git != fileList.end(); git++ )
	{
		if( strstr( (*git).c_str(), mask) )
		{
			fileList.erase( git );
			git = fileList.begin();
		}
	}
}

HTREEITEM SelectTypes_AddItemToTree(HWND hwndTV, LPSTR lpszItem, int nLevel)
{ 
	// AddItemToTree - adds items to a tree view control. 
	// Returns the handle to the newly added item. 
	// hwndTV - handle to the tree view control. 
	// lpszItem - text of the item to add. 
	// nLevel - level at which to add the item. 

	TVITEM tvi; 
	TVINSERTSTRUCT tvins; 
	static HTREEITEM hPrev = (HTREEITEM) TVI_FIRST; 
	static HTREEITEM hPrevRootItem = NULL; 
	static HTREEITEM hPrevLev2Item = NULL;

	// make sure that this one has not already been inserted
	HTREEITEM sibling = TreeView_GetRoot(hwndTV);
	while( sibling )
	{
		char buffer[64];
		TVITEM tv;
		tv.mask = TVIF_HANDLE | TVIF_TEXT;
		tv.hItem = sibling;
		tv.cchTextMax = sizeof(buffer);
		tv.pszText = buffer;
		if(TreeView_GetItem(hwndTV,&tv))
		{
			if( !strcmp(buffer,lpszItem) )
			{
				return hPrev;
			}
		}
		sibling = TreeView_GetNextSibling(hwndTV,sibling);
	}


	// TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM; 
	tvi.mask = TVIF_TEXT | TVIF_STATE;
	
	tvi.state = TVIS_SELECTED;

	// Set the text of the item. 
	tvi.pszText = lpszItem; 
	tvi.cchTextMax = lstrlen(lpszItem); 

	// Assume the item is not a parent item, so give it a document image. 
//	tvi.iImage = g_nDocument; 
//	tvi.iSelectedImage = g_nDocument; 

	// Save the heading level in the item's application-defined 
	// data area. 
	tvi.lParam = (LPARAM) nLevel;

	tvins.item = tvi; 
	tvins.hInsertAfter = hPrev; 

	// Set the parent item based on the specified level. 
	if (nLevel == 1) 
		tvins.hParent = TVI_ROOT; 
	else if (nLevel == 2) 
		tvins.hParent = hPrevRootItem; 
	else 
		tvins.hParent = hPrevLev2Item; 

	// Add the item to the tree view control. 
	hPrev = (HTREEITEM) SendMessage(hwndTV, TVM_INSERTITEM, 0, (LPARAM)(LPTVINSERTSTRUCT) &tvins); 

	// Save the handle to the item. 
	if (nLevel == 1) 
		hPrevRootItem = hPrev; 
	else if (nLevel == 2) 
		hPrevLev2Item = hPrev; 

	// The new item is a child item. Give the parent item a closed folder bitmap to indicate it now has child items. 
//	if (nLevel > 1) 
//	{ 
//		HTREEITEM hti; 
//		hti = TreeView_GetParent(hwndTV, hPrev); 
//		tvi.mask = TVIF_IMAGE | TVIF_SELECTEDIMAGE; 
//		tvi.hItem = hti; 
//		tvi.iImage = g_nClosed; 
//		tvi.iSelectedImage = g_nClosed; 
//		TreeView_SetItem(hwndTV, &tvi); 
//	} 

	if( nLevel == 0 )
	{
		TVITEM tvItem;

		tvItem.mask = TVIF_HANDLE | TVIF_STATE;
		tvItem.hItem = hPrev;
		tvItem.stateMask = TVIS_STATEIMAGEMASK;

		// Since state images are one-based, 1 in this macro turns the check off, and 2 turns it on.
		tvItem.state = INDEXTOSTATEIMAGEMASK(2);

		TreeView_SetItem(hwndTV, &tvItem);
	}

	return hPrev; 
} 

BOOL CALLBACK DlgProc_SelectTypes(HWND hwnd, UINT message, UINT wParam, LPARAM lParam)
{

	if (message == WM_INITDIALOG)
        SetWindowLong(hwnd, DWL_USER, lParam);
	ADBFileImport* THIS = (ADBFileImport*)GetWindowLong(hwnd, DWL_USER);

	switch( message )
	{
		case WM_INITDIALOG:
		{
			THIS->fileView   = GetDlgItem(hwnd,IDC_TREE_FILELIST);
			THIS->treeView   = GetDlgItem(hwnd,IDC_TREE_BTLIST);
			THIS->bSelectAll = false;

			for( StringList::iterator git = fileList.begin(); git != fileList.end(); git++ )
			{
				char dirname[64];
				char filename[128];

				// extract the dir name from the file name

				const char* dname = (*git).c_str();
				strncpy( dirname, dname, 64 );
				char* slash = strchr(dirname,'\\');
				*slash = 0;

				const char* fname = strchr( (*git).c_str(), '\\');
				strncpy( filename, fname + 1, 128 );

				SelectTypes_AddItemToTree( THIS->treeView, dirname, 0 );
				SelectTypes_AddItemToTree( THIS->fileView, filename, 0 );
			}

			break;
		}
		case WM_COMMAND:
		{
			if( LOWORD(wParam) == IDOK )
			{
				THIS->bAutoOverwrite = Button_GetCheck( GetDlgItem(hwnd,IDC_CHECK_OVERWRITE) ) != 0;

				if( THIS->bAutoOverwrite )
				{
					EndDialog(hwnd, IDOK);
					return 0;
				}
				
				// create a list of files that will be updated here

				HTREEITEM sibling = TreeView_GetRoot(THIS->treeView);
				while( sibling )
				{
					// exclude by file type
					char buffer[64];
					TVITEM tv;
					tv.mask = TVIF_HANDLE | TVIF_TEXT | TVIF_STATE;
					tv.state = 0;
					tv.stateMask = TVIS_STATEIMAGEMASK;
					tv.hItem = sibling;
					tv.cchTextMax = sizeof(buffer);
					tv.pszText = buffer;
					if(TreeView_GetItem(THIS->treeView,&tv))
					{
						// has this family been checked?
						if( (tv.state >> 12) == 2 )
						{
							SelectTypes_ExcludeFile( buffer );
						}
					}
					sibling = TreeView_GetNextSibling(THIS->treeView,sibling);
				}

				sibling = TreeView_GetRoot(THIS->fileView);
				while( sibling )
				{
					// exclude by file name
					char buffer[64];
					TVITEM tv;
					tv.mask = TVIF_HANDLE | TVIF_TEXT | TVIF_STATE;
					tv.state = 0;
					tv.stateMask = TVIS_STATEIMAGEMASK;
					tv.hItem = sibling;
					tv.cchTextMax = sizeof(buffer);
					tv.pszText = buffer;
					if(TreeView_GetItem(THIS->treeView,&tv))
					{
						// has this family been checked?
						if( (tv.state >> 12) == 2 )
						{
							SelectTypes_ExcludeFile( buffer );
						}
					}
					sibling = TreeView_GetNextSibling(THIS->treeView,sibling);
				}

				EndDialog(hwnd, IDOK);
			}
			else if( LOWORD(wParam) == IDCANCEL )
			{
				EndDialog(hwnd, IDCANCEL);
			}
			else if( LOWORD(wParam) == IDC_RADIO_SELECT_ALL )
			{
				THIS->bSelectAll ^= true;
				Button_SetCheck( GetDlgItem(hwnd,IDC_RADIO_SELECT_ALL), THIS->bSelectAll );
			}
			break;
		}
	}
	return 0;
}

//------------------------------------------------------------------------------
// ADBFileImport
//------------------------------------------------------------------------------

void ADBFileImport::Init( struct IFileSystem* _datafile )
{
	processType = PT_NONE;
	outFile = NULL;
	inFile  = NULL;

    // Set the members of the OPENFILENAME structure.

    OPENFILENAME ofn = {0}; // common dialog box structure
	memset( &ofn, 0, sizeof(ofn) );
    char szDirName[256];    // directory string
    char szFileTitle[256];  // file-title string
	char szDest[256];

    ofn.lStructSize = sizeof(OPENFILENAME);
    ofn.hwndOwner = hMainWindow;
    ofn.lpstrFilter = "ADB File *.db\0*.db\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrFile = szDest;
    ofn.nMaxFile = 256;
    ofn.lpstrFileTitle = szFileTitle;
    ofn.nMaxFileTitle = sizeof(szFileTitle);
    ofn.lpstrInitialDir = szDirName;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

    // Display the Open dialog box.
    if( !GetOpenFileName(&ofn) )
	{
		return;
	}

	outFile = _datafile;
	inFile  = NULL;

	DAFILEDESC fdesc = szDest;
	if( DACOM->CreateInstance( &fdesc, (void**)&inFile ) == GR_OK )
	{
		// building a list of different files between the "inFile" and "outFile"
		IFileSystem* file = inFile;
		file->SetCurrentDirectory("\\");
		HANDLE dirHandle;
		WIN32_FIND_DATA dirData;
			
		dirHandle = file->FindFirstFile("*.*",&dirData);
		
		// check first file
		while(dirHandle != INVALID_HANDLE_VALUE )
		{
			if ( strcmp(dirData.cFileName,"ParseData") )
			{
				if( dirData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
				{
					char findFileMask[256];
					sprintf( findFileMask, "%s\\*.*", dirData.cFileName );

					WIN32_FIND_DATA fileData;
					HANDLE          fileHandle;

					fileHandle = file->FindFirstFile(findFileMask,&fileData);

					while( fileHandle != INVALID_HANDLE_VALUE )
					{
						// check to see if the found files exist in the out file, and if they are different
						if( DetectChange(dirData.cFileName, fileData.cFileName, inFile, outFile) )
						{
							string fn = string(dirData.cFileName) + string("\\") + string(fileData.cFileName);
							fileList.push_back( fn.c_str() );
						}

						if( !file->FindNextFile(fileHandle,&fileData) )
						{
							fileHandle = INVALID_HANDLE_VALUE;
						}
					}
					file->FindClose(fileHandle);
				}
			}

			if( !file->FindNextFile(dirHandle,&dirData) )
			{
				dirHandle = INVALID_HANDLE_VALUE;
			}
		}
		file->FindClose(dirHandle);

		::DialogBoxParam( hInstance, MAKEINTRESOURCE(IDD_SELECT_TYPES), 0, DlgProc_SelectTypes, (LPARAM)this );
	}
}

//------------------------------------------------------------------------------

void ADBFileImport::Execute( HINSTANCE _hInstance, HWND _parent )
{
	if( inFile )
	{
		::DialogBoxParam( hInstance, MAKEINTRESOURCE(IDD_PROGBAR), _parent, DlgProc_ADBFileImport, (LPARAM)this );
	}
}

//------------------------------------------------------------------------------

bool ADBFileImport::Update( HWND _progressBar )
{
	if( processType == PT_NONE )
	{
		if( outFile == NULL ) return false;
		if( inFile  == NULL ) return false;
		if( fileList.size() == 0 ) return false;

		processType = PT_IMPORT_FILES;
	}
	else if( processType == PT_IMPORT_FILES )
	{
		if( fileList.size() == 0 )
		{
			processType = PT_CLEANUP;
			return 0;
		}

		// set the first current directory
		strcpy( fileCurrentFile, fileList.front().c_str() );
		string fn = string("\\") + string(fileCurrentFile);
		fileList.pop_front();

		outFile->DeleteFile( fn.c_str() );

		DAFILEDESC fdesc;
		fdesc.dwDesiredAccess = GENERIC_READ;
		fdesc.dwShareMode = 0;
		fdesc.dwCreationDistribution = OPEN_EXISTING;
		fdesc.lpFileName = fn.c_str();
		fdesc.lpImplementation = "DOS";

		COMPTR<IFileSystem> srcFile;
		if( inFile->CreateInstance(&fdesc,srcFile) == GR_OK )
		{
			DWORD srcFileSize = srcFile->GetFileSize();
			DWORD bytesRead = 0;
			void* srcMemory = new char[ srcFileSize ];
			srcFile->ReadFile( 0, srcMemory, srcFileSize, &bytesRead );

			DAFILEDESC fdesc;
			fdesc.dwDesiredAccess = GENERIC_WRITE;
			fdesc.dwShareMode = 0;
			fdesc.dwCreationDistribution = CREATE_ALWAYS;
			fdesc.lpFileName = fn.c_str();
			fdesc.lpImplementation = "DOS";

			COMPTR<IFileSystem> dstFile;
			if( outFile->CreateInstance(&fdesc,dstFile) == GR_OK )
			{
				DWORD bytesWrote = 0;
				dstFile->WriteFile(0, srcMemory, bytesRead, &bytesWrote );
			}

			delete [] srcMemory;
		}
	}
	else if( processType == PT_CLEANUP )
	{
		fileList.clear();
		inFile = NULL;
		outFile = NULL;

		bAutoOverwrite = false;
		bSelectAll = false;
		bSomethingElse = false;

		processType = PT_DONE;

		return false;
	}

	return true;
}

//------------------------------------------------------------------------------

bool ADBFileImport::Uninit( void )
{
	return 0;
}

//------------------------------------------------------------------------------

bool ADBFileImport::DetectChange( const char* _dirname, const char* _filename, IFileSystem* _inFile, IFileSystem* _outFile )
{
	// this database only goes one directory deep

	char fn[MAX_PATH];
	sprintf( fn, "\\%s\\%s", _dirname, _filename );

	DAFILEDESC fdesc;
	fdesc.dwDesiredAccess = GENERIC_READ;
	fdesc.dwShareMode = FILE_SHARE_READ;
	fdesc.dwCreationDistribution = OPEN_EXISTING;
	fdesc.lpFileName = fn;
	fdesc.lpImplementation = "DOS";

	COMPTR<IFileSystem> dstTestFile;
	if( _outFile->CreateInstance(&fdesc,dstTestFile) != GR_OK )
	{
		// this is true since the source file does not exist in the destination file system
		return true;
	}

	COMPTR<IFileSystem> srcTestFile;
	if( _inFile->CreateInstance(&fdesc,srcTestFile) != GR_OK )
	{
		// well poop. this file should have existed in the source file!
		assert( 0 && "Could not find file in source ADB file!");
		return true;
	}

	DWORD dstFileSize = dstTestFile->GetFileSize();
	DWORD srcFileSize = srcTestFile->GetFileSize();

	// first make sure they are the same length
	if( dstFileSize != srcFileSize )
	{
		// if they are not the same size, then do not over write the src file
		return true;
	}

	DWORD bytesRead;

	void* dstMemory = new char[ dstFileSize ];
	void* srcMemory = new char[ srcFileSize ];

	dstTestFile->ReadFile( 0, dstMemory, dstFileSize, &bytesRead );
	srcTestFile->ReadFile( 0, srcMemory, srcFileSize, &bytesRead );

	int ret = memcmp( dstMemory, srcMemory, bytesRead );

	delete [] dstMemory;
	delete [] srcMemory;

	// return the memory comparison results
	return( ret != 0 );
}
