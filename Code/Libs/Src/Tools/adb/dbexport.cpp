//--------------------------------------------------------------------------//
//                                                                          //
//                               dbexport.cpp		   							//
//                                                                          //
//                  COPYRIGHT (C) 2003 Fever Pitch Studios, INC.            //
//                                                                          //
//--------------------------------------------------------------------------//
/*
	$Header: /Libs/Src/Tools/adb/dbexport.cpp 6     11/25/03 11:07a Ajackson $

	$Author: Ajackson $
*/
//--------------------------------------------------------------------------//

#include "pch.h"

#include "dbexport.h"
#include "filesys.h"
#include "resource.h"
#include "dbsymbols.h"
#include "commctrl.h" 
#include "TSmartPointer.h"

#include <string.h>
#include <malloc.h>

#include <string>
using namespace std;

DbSymbols g_Symbols;

//-------------------------------------------------------------------------------------

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//--------------------------------------------------------------------------
// 
//--------------------------------------------------------------------------

U32 IFILESYSTEM_CountNumFiles( DWORD& count, IFileSystem* file )
{
	HANDLE handle;
	WIN32_FIND_DATA data;
		
	handle = file->FindFirstFile("*.*",&data);
	
	// check first file
	while(handle != INVALID_HANDLE_VALUE )
	{
		if ( strcmp(data.cFileName,"ParseData") )
		{
			if( data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
			{
				count++;
				DAFILEDESC fdesc = data.cFileName;
				COMPTR<IFileSystem> temp;

				file->CreateInstance( &fdesc, temp );
				IFILESYSTEM_CountNumFiles( count, temp );
			}
			else
			{
				do
				{
					count++;
				}
				while (file->FindNextFile(handle,&data));

				file->FindClose(handle);
				file->SetCurrentDirectory("\\");
				return count;
			}
		}

		if( !file->FindNextFile(handle,&data) )
		{
			handle = INVALID_HANDLE_VALUE;
		}
	}

	file->FindClose(handle);

	return count;
}

//--------------------------------------------------------------------------
// Internal Lists
//--------------------------------------------------------------------------

static DWORD s_HandleTimer = 0;

//--------------------------------------------------------------------------
// ExcelExport
//--------------------------------------------------------------------------

BOOL CALLBACK DlgProc_ExcelExport(HWND hwnd, UINT message, UINT wParam, LPARAM lParam)
{
	if (message == WM_INITDIALOG)
        SetWindowLong(hwnd, DWL_USER, lParam);
	ExcelExport* THIS = (ExcelExport*)GetWindowLong(hwnd, DWL_USER);

	switch( message )
	{
		case WM_INITDIALOG:
		{
			SetWindowText( hwnd, "Excel Exporting" );
			SetDlgItemText(hwnd,IDC_STATIC1,"Exporting to Microsoft Excel file(s)...");
			::SetTimer( hwnd, 101, 5, NULL );
			::SendMessage( GetDlgItem(hwnd,IDC_PROGRESS1), PBM_SETPOS, 0, 0 );
			break;
		}
		case WM_COMMAND:
		{
			switch (LOWORD(wParam))
			{
				case IDOK:
				{
					EndDialog(hwnd, IDOK);
					break;
				}
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

void ExcelExport::Init( struct IFileSystem* _datafile, const char* _outFile, int _numFiles )
{
	dataFile    = _datafile;
	outFilename = _strdup( _outFile );
	hFile       = INVALID_HANDLE_VALUE;

	if( dataFile->SetCurrentDirectory("\\") == 0 )
	{
		return;
	}
	hDir  = dataFile->FindFirstFile("*.*",&findDir);
	hFile = INVALID_HANDLE_VALUE;
	numFiles = _numFiles;

	DWORD nfiles = 0;
	IFILESYSTEM_CountNumFiles( nfiles, dataFile );
	numFiles = nfiles;

	g_Symbols.Init( PARSER );
}

//--------------------------------------------------------------------------

bool ExcelExport::Uninit( void )
{
	if( hDir == INVALID_HANDLE_VALUE ) return false;

	::free( outFilename );
	dataFile->SetCurrentDirectory("\\");
	dataFile = NULL;

	return true;
}

//--------------------------------------------------------------------------

void ExcelExport::Execute( HINSTANCE _hInstance, HWND _parent )
{
	::DialogBoxParam( hInstance, MAKEINTRESOURCE(IDD_PROGBAR), _parent, DlgProc_ExcelExport, (LPARAM)this );
}

//--------------------------------------------------------------------------

bool ExcelExport::Update( HWND _progressBar )
{
	if( hDir == INVALID_HANDLE_VALUE ) return false;

	// handle files
	if( hFile != INVALID_HANDLE_VALUE )
	{
		// parse the file info
		g_Symbols.ExportRow( findFile.cFileName, dataFile );
		::SendMessage( _progressBar, PBM_STEPIT, 0, 0 );

		int ret = dataFile->FindNextFile(hFile,&findFile);

		if( !ret )
		{
			hFile = INVALID_HANDLE_VALUE;

			// advance to the next directory
			if( dataFile->FindNextFile(hDir,&findDir) == 0 )
				hDir = INVALID_HANDLE_VALUE;
		}
		return true;
	}

	// make sure this not a silly "." entry
	else if (findDir.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
	{
		if( strcmp(findDir.cFileName,"ParseData")==0 )
		{
			// ignore the ParseData
			::SendMessage( _progressBar, PBM_SETPOS, 0, 0 );
			::SendMessage( _progressBar, PBM_SETRANGE, 0, MAKELPARAM(0, numFiles) );
			::SendMessage( _progressBar, PBM_SETSTEP, 1, 0 );

			// advance to the next directory
			if( dataFile->FindNextFile(hDir,&findDir) == 0 )
				hDir = INVALID_HANDLE_VALUE;
		}
		else if (findDir.cFileName[0] == '.')
		{
			// advance to the next directory
			if( dataFile->FindNextFile(hDir,&findDir) == 0 )
				hDir = INVALID_HANDLE_VALUE;
		}
		else
		{
			// parse the archetype struct
			g_Symbols.ExportTableHeader( findDir.cFileName );
			::SendMessage( _progressBar, PBM_STEPIT, 0, 0 );

			// get all the files from this directory
			string dirMask = string("\\") + string(findDir.cFileName);
			dataFile->SetCurrentDirectory( dirMask.c_str() );
			hFile = dataFile->FindFirstFile( "*.*",&findFile);
		}
	}

	return( hDir != INVALID_HANDLE_VALUE );
}

