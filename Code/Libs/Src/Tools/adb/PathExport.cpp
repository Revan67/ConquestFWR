//--------------------------------------------------------------------------//
//                                                                          //
//                               PathExport.cpp	   							//
//                                                                          //
//                  COPYRIGHT (C) 2003 Fever Pitch Studios, INC.            //
//                                                                          //
//--------------------------------------------------------------------------//
/*
	$Header: /Libs/Src/Tools/adb/PathExport.cpp 3     11/25/03 11:07a Ajackson $

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

//-------------------------------------------------------------------------------------

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//--------------------------------------------------------------------------
// 
//--------------------------------------------------------------------------

static DbSymbols g_Symbols;

//--------------------------------------------------------------------------
// PathExport
//--------------------------------------------------------------------------

HANDLE PathExport_FindNextDirectory( IFileSystem* _dataFile, WIN32_FIND_DATA& _fdata, const char* _iniFile )
{
	char path[MAX_PATH];
	_dataFile->GetCurrentDirectory(MAX_PATH,path);

	string iniFile = string(_iniFile) + string("\\") + string(_iniFile) + string(".ini");

	FILE* file = fopen( iniFile.c_str(), "at" );
	if( file )
	{
		fputs(path+1,file);
		fputs("\n",file);
	}

	_dataFile->SetCurrentDirectory("\\");

	WIN32_FIND_DATA fdata;
	HANDLE h = _dataFile->FindFirstFile("\\*.*",&fdata);

	while( h != INVALID_HANDLE_VALUE )
	{
		if( !strcmp(fdata.cFileName,path+1) )
		{
			if( _dataFile->FindNextFile(h,&fdata) )
			{
				_dataFile->SetCurrentDirectory(fdata.cFileName);
				memcpy( &_fdata, &fdata, sizeof(fdata) );
				return h;
			}
		}
		
		if( !_dataFile->FindNextFile(h,&fdata) )
		{
			return INVALID_HANDLE_VALUE;
		}
	}

	return INVALID_HANDLE_VALUE;
}

//--------------------------------------------------------------------------

bool PathExport_ExportFile( const char* _outpath, const char* _archname, IFileSystem* _dataFile )
{
	string fullpath = _outpath;

	// create base level
	::CreateDirectory( fullpath.c_str(), NULL );

	// create type level
	char path[MAX_PATH];
	_dataFile->GetCurrentDirectory(MAX_PATH,path);
	fullpath += path;
	::CreateDirectory( fullpath.c_str(), NULL );

	// create a level for each !!
	strcpy(path,_archname);
	char* dname = strstr(path,"!!");
	char* thisDir = path;
	char* lastDir = path;
	while( dname )
	{
		dname[0] = 0;
		dname    += 2;
		lastDir  = dname;
		dname    = strstr(dname,"!!");

		fullpath += "\\";
		fullpath += thisDir;
		::CreateDirectory( fullpath.c_str(), NULL );

		thisDir = lastDir;
	}

	// start a new file name
	string thisFile = fullpath + string("\\") + lastDir;

	// do not try to write over files that are read-only
	DWORD dwAttributes = ::GetFileAttributes( thisFile.c_str() );
	if( dwAttributes != 0xFFFFFFFF && dwAttributes & FILE_ATTRIBUTE_READONLY )
	{
		return true;
	}

	// open up the archetype file
	DAFILEDESC fdesc = _archname;
	COMPTR<IFileSystem> f;
	if( _dataFile->CreateInstance(&fdesc,f) != GR_OK )
	{
		return false;
	}

	// test versus current file (if any)
	HANDLE hFile = ::CreateFile( thisFile.c_str(), GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
	if( hFile != INVALID_HANDLE_VALUE )
	{
		FILETIME fCreationTime, fLastAccessTime, fLastWriteTime;
		::GetFileTime(hFile,&fCreationTime,&fLastAccessTime,&fLastWriteTime);
		::CloseHandle( hFile );

		FILETIME aCreationTime, aLastAccessTime, aLastWriteTime;
		if( f->GetFileTime(0,&aCreationTime,&aLastAccessTime,&aLastWriteTime) )
		{
			// if they share the same write times, don't write again
			if( !CompareFileTime(&fLastWriteTime,&aLastWriteTime) )
			{
				return true;
			}
		}
	}

	// read in memory file, write to DOS file
	DWORD size = f->GetFileSize();
	DWORD dwWritten = 0;
	void* structure = ::alloca( size );

	if( f->ReadFile(0, structure, size, &dwWritten ) )
	{
		HANDLE hFile = ::CreateFile( thisFile.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
		if( hFile != INVALID_HANDLE_VALUE )
		{
			// write out the new archive
			::WriteFile(hFile, structure, size, &dwWritten, NULL );
			::CloseHandle( hFile );
		}
	}

	return true;
}

//--------------------------------------------------------------------------

BOOL CALLBACK DlgProc_PathExport(HWND hwnd, UINT message, UINT wParam, LPARAM lParam)
{
	if (message == WM_INITDIALOG)
        SetWindowLong(hwnd, DWL_USER, lParam);
	PathExport* THIS = (PathExport*)GetWindowLong(hwnd, DWL_USER);

	switch( message )
	{
		case WM_INITDIALOG:
		{
			SetWindowText( hwnd, "PathExport Exporting" );
			SetDlgItemText(hwnd,IDC_STATIC1,"Exporting to files in paths...");
			::SetTimer( hwnd, 101, 5, NULL );
			::SendMessage( GetDlgItem(hwnd,IDC_PROGRESS1), PBM_SETPOS, 0, 0 );
			::SendMessage( GetDlgItem(hwnd,IDC_PROGRESS1), PBM_SETRANGE, 0, MAKELPARAM(0,THIS->numFiles) );
			::SendMessage( GetDlgItem(hwnd,IDC_PROGRESS1), PBM_SETSTEP, 1, 0 );
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

void PathExport::Init( struct IFileSystem* _datafile, const char* _outFile, int _numFiles )
{
	bWriting    = false;
	dataFile    = _datafile;
	outFilename = _strdup( _outFile );

	if( dataFile->SetCurrentDirectory("\\") == 0 )
	{
		return;
	}
	hData  = dataFile->FindFirstFile("*.*",&fData);

	numFiles = 0;
	extern U32 IFILESYSTEM_CountNumFiles( DWORD& count, IFileSystem* file );
	IFILESYSTEM_CountNumFiles( numFiles, dataFile );

	g_Symbols.Init( PARSER );
}

//--------------------------------------------------------------------------

bool PathExport::Uninit( void )
{
	if( outFilename )
		::free( outFilename );
	outFilename = NULL;

	if( dataFile )
		dataFile->SetCurrentDirectory("\\");
	dataFile = NULL;

	return true;
}

//--------------------------------------------------------------------------

void PathExport::Execute( HINSTANCE _hInstance, HWND _parent )
{
	::DialogBoxParam( hInstance, MAKEINTRESOURCE(IDD_PROGBAR), _parent, DlgProc_PathExport, (LPARAM)this );
}

//--------------------------------------------------------------------------

bool PathExport::Update( HWND _progressBar )
{
	if( bWriting ) return true;

	if( hData == INVALID_HANDLE_VALUE ) return false;

	if( fData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
	{
		bWriting = true;
		if( fData.cFileName[0] != '.' )
		{
			dataFile->SetCurrentDirectory(fData.cFileName);

			char path[MAX_PATH];
			sprintf(path,"\\%s\\*.*",fData.cFileName);
			hData = dataFile->FindFirstFile(path,&fData);
		}
		else if ( !dataFile->FindNextFile(hData,&fData) )
		{
			hData = INVALID_HANDLE_VALUE;
		}
		bWriting = false;
	}
	else
	{
		::SendMessage( _progressBar, PBM_STEPIT, 0, 0 );

		// export file
		bWriting = true;
		PathExport_ExportFile( outFilename, fData.cFileName, dataFile );
		bWriting = false;

		if ( !dataFile->FindNextFile(hData,&fData) )
		{
			// try to advance to the next directory
			bWriting = true;
			HANDLE hData = PathExport_FindNextDirectory( dataFile, fData, outFilename );
			bWriting = false;

			if( hData == INVALID_HANDLE_VALUE )
			{
				return false;
			}
		}
	}

	return true;
}

