//------------------------------------------------------------------------------------------//
//																							//
//                          MicrosoftDatabaseExport.cpp										//
//																							//
//                  COPYRIGHT (C) 2003 Fever Pitch Studios, INC.							//
//																							//
//------------------------------------------------------------------------------------------//
/*
	$Header: /Libs/Src/Tools/adb/MicrosoftDatabaseExport.cpp 7     11/25/03 11:07a Ajackson $

	$Author: Ajackson $
*/
//------------------------------------------------------------------------------------------//

#include "pch.h"

#include "dbexport.h"
#include "filesys.h"
#include "resource.h"
#include "dbsymbols.h"
#include "commctrl.h" 
#include "TSmartPointer.h"
#include "export_mdb.h"
#include "..\..\Viewer\Docuview\SymTable.h"

#include <commdlg.h>
#include <string.h>
#include <malloc.h>
#include <assert.h>

#pragma warning( disable : 4786 )
#include <string>
#include <list>
using namespace std;

//------------------------------------------------------------------------------
// namespace ns_MicrosoftDatabaseExport
//------------------------------------------------------------------------------

namespace ns_MicrosoftDatabaseExport
{
	//-----------------------------------------------

	enum ProcessType
	{
		PT_NONE = 0,
		PT_FILES,
		PT_ORDER_COMMANDS,
		PT_COMMANDS,
	};

	struct FileEntry
	{
		string dir;
		string file;
	};

	struct FileList    : list<FileEntry>{};
	struct CommandList : list<std::string>{};

	//-----------------------------------------------

	SqlDb       sqldb;
	FileList    fileList;
	string      currentDirectory;
	DbSymbols   symbols;
	CommandList commandList;
	ProcessType processType;
	
	FileList::iterator fileIt;
	CommandList::iterator commandIt;


	void ExportEnumTable( SYMBOL s )
	{
		SYMBOL dtenum = s->datatype;
		while( dtenum )
		{
			if( dtenum->kind == ENUMSYM )
			{
				break;
			}
			dtenum = dtenum->datatype;
		}

		if( !dtenum ) return;

		string deleteTable = string("DROP TABLE ") + string(dtenum->name);
		commandList.push_back( deleteTable.c_str() );

		// create a new table for the enum
		CreateTableQuery table;
		table.Start( dtenum->name );
			table.Add("enumid", CreateTableQuery::CHAR_VAR, 0 );
			table.Add("enumval", CreateTableQuery::INTEGER, 0 );
		table.End();
		commandList.push_back( table.sql.c_str() );

		InsertDataRow insert;

		SYMBOL dtconst = dtenum->datatype;
		while( dtconst )
		{
			// create all the enum const entries
			insert.Start( dtenum->name );
				insert.AddString( "enumid", dtconst->name );
				insert.AddInt   ( "enumval", dtconst->constval.intnum );
			insert.End();
			commandList.push_back( insert.sql.c_str() );

			dtconst = dtconst->link;
		}
	}

	const char* FindEnumValueName( SYMBOL s, int value )
	{
		SYMBOL dtenum = s->datatype;
		while( dtenum )
		{
			if( dtenum->kind == ENUMSYM )
			{
				break;
			}
			dtenum = dtenum->datatype;
		}

		if( !dtenum ) return NULL;

		SYMBOL dtconst = dtenum->datatype;
		while( dtconst )
		{
			if( dtconst->constval.intnum == value )
			{
				return dtconst->name;
			}

			dtconst = dtconst->link;
		}

		return NULL;
	}

	bool IsSqlCommandDup( CommandList& _cmdlist, std::string& _cmd )
	{
		// first get all DROP TABLE commands
		for( CommandList::iterator cmdIt = _cmdlist.begin(); cmdIt != _cmdlist.end(); cmdIt++ )
		{
			std::string& cmd = *cmdIt;

			if( cmd == _cmd )
			{
				return true;
			}
		}
		return false;
	}

	void OrderSqlCommands( void )
	{
		CommandList newCommandList;
		CommandList::iterator cmdIt;

		// first get all DROP TABLE commands
		for( cmdIt = commandList.begin(); cmdIt != commandList.end(); cmdIt++ )
		{
			std::string& cmd = *cmdIt;

			if( strstr(cmd.c_str(),"DROP TABLE") )
			{
				if( !IsSqlCommandDup(newCommandList,cmd) )
				{
					newCommandList.push_back( cmd.c_str() );
				}
			}
		}

		// next get all the CREATE TABLE commands for ENUM
		for( cmdIt = commandList.begin(); cmdIt != commandList.end(); cmdIt++ )
		{
			std::string& cmd = *cmdIt;

			if( strstr(cmd.c_str(),"CREATE TABLE") )
			{
				if( strstr(cmd.c_str(),"( enumid varchar PRIMARY KEY ,enumval int )") )
				{
					if( !IsSqlCommandDup(newCommandList,cmd) )
					{
						newCommandList.push_back( cmd.c_str() );
					}
				}
			}
		}

		// next get all the CREATE TABLE commands for the BT_ types
		for( cmdIt = commandList.begin(); cmdIt != commandList.end(); cmdIt++ )
		{
			std::string& cmd = *cmdIt;

			if( strstr(cmd.c_str(),"CREATE TABLE") )
			{
				if( !IsSqlCommandDup(newCommandList,cmd) )
				{
					newCommandList.push_back( cmd.c_str() );
				}
			}
		}

		// insert all the INSERT INTO commands
		for( cmdIt = commandList.begin(); cmdIt != commandList.end(); cmdIt++ )
		{
			std::string& cmd = *cmdIt;

			if( !IsSqlCommandDup(newCommandList,cmd) )
			{
				newCommandList.push_back( cmd.c_str() );
			}
		}

		commandList.clear();

		for( cmdIt = newCommandList.begin(); cmdIt != newCommandList.end(); cmdIt++ )
		{
			std::string& cmd = *cmdIt;
			commandList.push_back( cmd.c_str() );
			
			//_localprintf("%s\n", cmd.c_str() );
			//OutputDebugString( "\n" );
			//OutputDebugString( cmd.c_str() );
		}
	}

}
using namespace ns_MicrosoftDatabaseExport;

//--------------------------------------------------------------------------
// 
//--------------------------------------------------------------------------

void MicrosoftDatabaseExport_EnumFiles( IFileSystem* file )
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
				currentDirectory = data.cFileName;

				string dropTable = string("DROP TABLE ") + currentDirectory;
				commandList.push_back( dropTable.c_str() );
				
				string fn = string("\\") + currentDirectory;

				DAFILEDESC fdesc = fn.c_str();
				COMPTR<IFileSystem> temp;
				if( file->CreateInstance(&fdesc,temp) == GR_OK )
				{
					MicrosoftDatabaseExport_EnumFiles( temp );
				}
			}
			else
			{
				do
				{
					FileEntry fe;
					fe.dir  = currentDirectory;
					fe.file = data.cFileName;
					fileList.push_back( fe );
				}
				while (file->FindNextFile(handle,&data));

				file->FindClose(handle);
				file->SetCurrentDirectory("\\");
			}
		}

		if( !file->FindNextFile(handle,&data) )
		{
			handle = INVALID_HANDLE_VALUE;
		}
	}

	file->FindClose(handle);
}

//------------------------------------------------------------------------------
// DlgProc_MicrosoftDatabaseExport
//------------------------------------------------------------------------------

BOOL CALLBACK DlgProc_MicrosoftDatabaseExport(HWND hwnd, UINT message, UINT wParam, LPARAM lParam)
{
	if (message == WM_INITDIALOG)
        SetWindowLong(hwnd, DWL_USER, lParam);
	MicrosoftDatabaseExport* THIS = (MicrosoftDatabaseExport*)GetWindowLong(hwnd, DWL_USER);

	switch( message )
	{
		case WM_INITDIALOG:
		{
			SetWindowText( hwnd, "Microsoft Database Exporting" );
			SetDlgItemText( hwnd, IDC_STATIC1, "Exporting to Microsoft Database file...");
			SendMessage( GetDlgItem(hwnd,IDC_PROGRESS1), PBM_SETPOS, 0, 0 );
			SendMessage( GetDlgItem(hwnd,IDC_PROGRESS1), PBM_SETRANGE, 0, MAKELPARAM(0, fileList.size() * 2) );
			SendMessage( GetDlgItem(hwnd,IDC_PROGRESS1), PBM_SETSTEP, 1, 0 );
			SetTimer( hwnd, 101, 5, NULL );
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

//------------------------------------------------------------------------------
// MicrosoftDatabaseExport
//------------------------------------------------------------------------------

void MicrosoftDatabaseExport::Init( struct IFileSystem* _datafile, const char* _outFile, int _numFiles )
{
	processType = PT_NONE;

    // Set the members of the OPENFILENAME structure.

    OPENFILENAME ofn = {0}; // common dialog box structure
    char szDirName[256];    // directory string
    char szFileTitle[256];  // file-title string
	char szDest[256];

    ofn.lStructSize = sizeof(OPENFILENAME);
    ofn.hwndOwner = hMainWindow;
    ofn.lpstrFilter = "Microsoft Database *.mdb\0*.mdb\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrFile = szDest;
    ofn.nMaxFile = 256;
    ofn.lpstrFileTitle = szFileTitle;
    ofn.nMaxFileTitle = sizeof(szFileTitle);
    ofn.lpstrInitialDir = szDirName;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

    // Display the Open dialog box.
    if (GetOpenFileName(&ofn))
	{
		sqldb.Connect( szDest );
		symbols.Init( PARSER );

		dataFile = _datafile;
		fileList.clear();
		_datafile->SetCurrentDirectory("\\");
		MicrosoftDatabaseExport_EnumFiles( dataFile );
		fileIt = fileList.begin();
	}
}

//------------------------------------------------------------------------------

void MicrosoftDatabaseExport::Execute( HINSTANCE _hInstance, HWND _parent )
{
	::DialogBoxParam( hInstance, MAKEINTRESOURCE(IDD_PROGBAR), _parent, DlgProc_MicrosoftDatabaseExport, (LPARAM)this );
}

//------------------------------------------------------------------------------

bool MicrosoftDatabaseExport::Update( HWND _progressBar )
{
	switch( processType )
	{
		case PT_NONE: 
		{
			processType = PT_FILES;
			break;
		}
		case PT_FILES: 
		{
			if( fileIt != fileList.end() )
			{
				UpdateFiles();
			}
			else
			{
				processType = PT_ORDER_COMMANDS;
			}
			break;
		}
		case PT_ORDER_COMMANDS: 
		{
			OrderSqlCommands();
			processType = PT_COMMANDS;
			commandIt = commandList.begin();

			PBRANGE range;
			::SendMessage( _progressBar, PBM_GETRANGE, true, (LPARAM)&range );
			int newRange = (commandList.size() + fileList.size()) - range.iHigh;

			if( newRange > 0 )
			{
				newRange += 10;
				SendMessage( _progressBar, PBM_SETRANGE, 0, MAKELPARAM(0,range.iHigh+newRange) );
			}

			break;
		}
		case PT_COMMANDS: 
		{
			if( commandIt != commandList.end() )
			{
				std::string& cmd = *commandIt;
				sqldb.Execute( cmd.c_str() );
				commandIt++;
			}
			else
			{
				processType = PT_NONE;
				return false;
			}
			break;
		}
	}

	::SendMessage( _progressBar, PBM_STEPIT, 0, 0 );
	return true;
}

//------------------------------------------------------------------------------

bool MicrosoftDatabaseExport::Uninit( void )
{
	sqldb.Disconnect();
	return 0;
}

//------------------------------------------------------------------------------

void MicrosoftDatabaseExport::UpdateFiles( void )
{
	FileEntry& fe = *fileIt;
	
	string fn = string("\\") + fe.dir;

	DAFILEDESC fdesc = fn.c_str();
	COMPTR<IFileSystem> temp;
	if( dataFile->CreateInstance(&fdesc,temp) == GR_OK )
	{
		CreateTableQuery table;
		InsertDataRow    insert;

		table.Start( fe.dir.c_str() );
		insert.Start( fe.dir.c_str() );

		symbols.ExportTableHeader( fe.dir.c_str() );
		symbols.ExportRow( fe.file.c_str(), temp );

		string xfn = fe.dir + string("_xls.csv");

		FILE* xfile = fopen( xfn.c_str(), "rt" );

		char* labels = (char*)malloc(1024*4);
		char* values = (char*)malloc(1024*4);

		fgets( labels, 1024*4, xfile );
		fgets( values, 1024*4, xfile );
		fflush( xfile );
		fclose( xfile );

		char* pThisLabel = labels;
		char* pNextLabel = strchr( pThisLabel, ',' );

		char* pThisValue = values;
		char* pNextValue = strchr( pThisValue, ',' );

		DWORD i = 0;
		SYMBOL s = symbols.GetSymbolByIndex(fe.dir.c_str(),i);
		while( s )
		{
			// advance the to the next label
			*pNextLabel = 0; pNextLabel++;
			*pNextValue = 0; pNextValue++;

			if( labels == pThisLabel )
			{
				table.Add( "ID", CreateTableQuery::CHAR_VAR, 0 );
				insert.AddString( "ID", pThisValue );
			}
			else
			{
				BYTE data  = 0;
				
				if( SH::IsString(s,data) )
				{
					table.Add( pThisLabel, CreateTableQuery::CHAR_ARRAY, data );
					insert.AddString( pThisLabel, pThisValue );
				}
				else if( SH::IsEnum(s,data) )
				{
					ExportEnumTable(s);

					const char* eConstName = FindEnumValueName(s, atoi(pThisValue) );

					if( eConstName )
					{
						table.Add( pThisLabel, CreateTableQuery::CHAR_VAR, 0 );
						table.AddForeignKey( pThisLabel, s->datatype->name );
						insert.AddString( pThisLabel, eConstName );
					}
					else
					{
						_localprintf("Warning: Could not find enum value %s(%s)\n", pThisLabel, pThisValue );
						table.Add( pThisLabel, CreateTableQuery::INTEGER, 0 );
						insert.AddInt( pThisLabel, atoi(pThisValue) );
					}
				}
				else if( SH::IsInteger(s,data) )
				{
					table.Add( pThisLabel, CreateTableQuery::INTEGER, 0 );
					insert.AddInt( pThisLabel, atoi(pThisValue) );
				}
				else if( SH::IsFloat(s,data) )
				{
					table.Add( pThisLabel, CreateTableQuery::FLOAT, 0 );
					insert.AddFloat( pThisLabel, atof(pThisValue) );
				}
				else
				{
					// unknown type!
					__asm int 3;
				}

				i++;
			}

			// asign next label
			pThisLabel = pNextLabel;
			pNextLabel = strchr( pNextLabel, ',' );

			pThisValue = pNextValue;
			pNextValue = strchr( pNextValue, ',' );

			s = symbols.GetSymbolByIndex(fe.dir.c_str(),i);
		}

		table.End();
		insert.End();

		commandList.push_back( table.sql.c_str() );
		commandList.push_back( insert.sql.c_str() );

		free( labels );
		free( values );

		::DeleteFile( xfn.c_str() );
	}

	fileIt++;
}

