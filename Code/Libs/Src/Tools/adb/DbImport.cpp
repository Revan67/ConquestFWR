//--------------------------------------------------------------------------//
//                                                                          //
//                               DbImport.cpp	    							//
//                                                                          //
//                  COPYRIGHT (C) 2003 Fever Pitch Studios, INC.            //
//                                                                          //
//--------------------------------------------------------------------------//
/*
	$Header: /Libs/Src/Tools/adb/DbImport.cpp 7     11/25/03 11:07a Ajackson $

	$Author: Ajackson $
*/
//--------------------------------------------------------------------------//

#include <afxwin.h>
#include "dbimport.h"

#include "typedefs.h"
#include "Globals.h"
#include "filesys.h"
#include "resource.h"
#include "dbsymbols.h"
#include "commctrl.h" 
#include "TSmartPointer.h"
#include "export_mdb.h"
#include "..\..\Viewer\Docuview\SymTable.h"

#include <commctrl.h> 
#include <commdlg.h>
#include <string.h>
#include <malloc.h>
#include <assert.h>

#include <string>
using namespace std;

//-------------------------------------------------------------------------------------

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#ifdef _DEBUG
	#define IMPORT_MEMSET(x,size) { memset(##x, 0xFE, size); }
	#define SANITY_CHECK() if( (pStructureData - newStructure) > currentStructureSize ) __asm int 3;
#else
	#define SANITY_CHECK() // sanity
	#define IMPORT_MEMSET // (x) { memset(&##x, 0xFE, sizeof(##x)); }
#endif

//-------------------------------------------------------------------------------------
// DbImport Globals
//-------------------------------------------------------------------------------------

extern U32 IFILESYSTEM_CountNumFiles( DWORD& count, IFileSystem* file );

static const char* s_SymbolTable[] = 
{"BT_GRANNY", "BT_UNIT", "BT_DECALS", "BT_GROUP", "BT_LIGHT", "BT_TERRAINOBJECT", "BT_TILEDEF", "BT_TRIGGEROBJECT", "BT_WATER", "BT_SOUND", NULL};

namespace ns_AccessImport
{
	SqlDb sqldb;

	string szCurrentFile;
}
using namespace ns_AccessImport;

//--------------------------------------------------------------------------
// DlgProc_AccessImport
//--------------------------------------------------------------------------

BOOL CALLBACK DlgProc_AccessImport(HWND hwnd, UINT message, UINT wParam, LPARAM lParam)
{
	if (message == WM_INITDIALOG)
        SetWindowLong(hwnd, DWL_USER, lParam);
	AccessFileImport* THIS = (AccessFileImport*)GetWindowLong(hwnd, DWL_USER);

	switch( message )
	{
		case WM_INITDIALOG:
		{
			DWORD count = 0;
			THIS->datafile->SetCurrentDirectory("\\");
			IFILESYSTEM_CountNumFiles( count, THIS->datafile );

			SetWindowText( hwnd, "MS Access Import" );
			SetDlgItemText(hwnd,IDC_STATIC1,"Importing from Microsoft MDB file...");
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
// AccessFileImport
//--------------------------------------------------------------------------

void AccessFileImport::Init( struct IFileSystem* _datafile )
{
	symbols = NULL;
	processType = PT_NONE;
	pLastStructure = pStructureData = newStructure = NULL;

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
    if( !GetOpenFileName(&ofn) )
	{
		return;
	}

	symbolTableIndex = 0;
	currentColumn = 1;

	datafile = _datafile;

	symbols = new DbSymbols;
	symbols->Init( PARSER );

	const char* thisTableName = s_SymbolTable[symbolTableIndex];
	ResetStructure( symbols->GetSymbolByName(thisTableName) );

	sqldb.Connect( szDest );
	sqldb.Execute_Select( thisTableName );
	symbolTableIndex++;
}

//------------------------------------------------------------------------------

void AccessFileImport::Execute( HINSTANCE _hInstance, HWND _parent )
{
	if( symbols )
	{
		::DialogBoxParam( hInstance, MAKEINTRESOURCE(IDD_PROGBAR), _parent, DlgProc_AccessImport, (LPARAM)this );
	}
}

//------------------------------------------------------------------------------

bool AccessFileImport::Update( HWND _progressBar )
{
	switch( processType )
	{
		case PT_NONE:
		{
			processType = PT_IMPORT_FILES;
			break;
		}
		case PT_IMPORT_FILES: 
		{
			bool working = false;
			const char* data = sqldb.FetchRowField( currentColumn, working );

			if( !working )
			{
				// done with this table, go to the next table
				currentColumn = 1;
				if( s_SymbolTable[symbolTableIndex] == NULL )
				{
					processType = PT_CLEANUP;
					return false;
				}

				// create buffer for next type of structure
				const char* thisTableName = s_SymbolTable[symbolTableIndex];
				ResetStructure( symbols->GetSymbolByName(thisTableName) );

				sqldb.Execute_Select(thisTableName);
				symbolTableIndex++;
				return true;
			}
			else if ( !data )
			{
				// done with this row, go to the next row
				::SendMessage( _progressBar, PBM_STEPIT, 0, 0 );

				// did we end the data with a bit field?
				if( currentBits != 0 )
				{
					pStructureData += sizeof(DWORD);
				}

				// need to pad for record boundaries (for the base record)
				while( (pStructureData - newStructure) < currentStructureSize )
				{ 
					pStructureData[0] = 0;
					pStructureData++;
				}

				// write data into datafile
				const char* szCurrentDir = s_SymbolTable[symbolTableIndex-1];

				string fn = string(szCurrentDir) + string("\\") + string(szCurrentFile);

				datafile->SetCurrentDirectory("\\");
				datafile->DeleteFile( fn.c_str() );

				DAFILEDESC fdesc;
				fdesc.dwDesiredAccess = GENERIC_WRITE;
				fdesc.dwShareMode = FILE_SHARE_READ;
				fdesc.dwCreationDistribution = CREATE_ALWAYS;
				fdesc.lpFileName = fn.c_str();
				fdesc.lpImplementation = "DOS";

				COMPTR<IFileSystem> fileDump;
				if( datafile->CreateInstance(&fdesc,fileDump) == GR_OK )
				{
					DWORD dwBytes = currentStructureSize;
					fileDump->WriteFile( 0, newStructure, dwBytes, &dwBytes );
				}

				currentColumn = 1;
				return true;
			}

			// the 1st column will also hold the name of the file

			if( currentColumn == 1 )
			{
				szCurrentFile = data;
				pLastStructure = pStructureData = newStructure;
				IMPORT_MEMSET( pStructureData, currentStructureSize );
				currentBits = 0;
				recordSize = 0;
			}
			else
			{
				SANITY_CHECK();

				// add to the current data structure

				BYTE dwData;

				DWORD symbolIndex = currentColumn - 2;
				SYMBOL s          = symbols->GetSymbolByIndex( s_SymbolTable[symbolTableIndex-1], symbolIndex );
				int symSize       = PARSER->GetTypeSize(s);

				// Work on resolving bit fields
				BYTE bitsOffset, bitsCount;
				bool hasBits = SH::GetBitCounts(s,bitsOffset,bitsCount);

				// need to pad for record boundaries
				if( !hasBits )
				{
					ApplyPadding();
				}

				if( hasBits )
				{
					if( currentBits == 0 )
					{
						DWORD dwValue = 0;
						memcpy( pStructureData, &dwValue, sizeof(DWORD) );
					}

					// if the number of bits go over 32, go to the next DWORD
					currentBits += bitsCount;
					if( currentBits > 32 )
					{
						pStructureData += sizeof(DWORD);
						DWORD dwValue = 0;
						memcpy( pStructureData, &dwValue, sizeof(DWORD) );
					}

					DWORD* pDword = reinterpret_cast<DWORD*>( pStructureData );

					if( SH::IsEnum(s,dwData) )
					{
						long constValue = symbols->GetEnumSymbolValue(data);
						constValue <<= bitsOffset;
						pDword[0] |= constValue;
					}
					else if( SH::IsInteger(s,dwData) )
					{
						int intData = atoi(data);
						intData <<= bitsOffset;
						pDword[0] |= intData;
					}
					else if( SH::IsFloat(s,dwData) )
					{
						// ignore this case right now
					}

					// need to do multiple columns until bits are gone
					currentColumn++;
					return Update(_progressBar);
				}
				else if( currentBits && !bitsCount )
				{
					currentBits = 0;
					pStructureData += sizeof(DWORD);
				}

				if( !hasBits )
				{
					// find the type and record it
					if( SH::IsString(s,dwData) )
					{
						assert( !hasBits );
						strncpy( (char*)pStructureData, data, symSize );
						pStructureData += symSize;
					}
					else if( SH::IsEnum(s,dwData) )
					{
						long constValue = symbols->GetEnumSymbolValue(data);
						memcpy( pStructureData, &constValue, symSize );
						pStructureData += symSize;
					}
					else if( SH::IsInteger(s,dwData) )
					{
						int intData = atoi(data);
						memcpy( pStructureData, &intData, symSize );
						pStructureData += symSize;
					}
					else if( SH::IsFloat(s,dwData) )
					{
						double dblData = atof(data);
						memcpy( pStructureData, &dblData, symSize );
						pStructureData += symSize;
					}
					else
					{
						// unknown type!
						__asm int 3;
					}
				}
			}

			SANITY_CHECK();
			currentColumn++;
			break;
		}
		case PT_CLEANUP: 
		{
			processType = PT_NONE;
			return 0;
			break;
		}
	}

	return 1;
}

//------------------------------------------------------------------------------

void AccessFileImport::ApplyPadding( void )
{
	const char* baseSymbolName = s_SymbolTable[symbolTableIndex-1];
	
	DWORD symbolIndex = currentColumn - 2;
	DWORD dataIndex = 0;
	DWORD currIndex = (pStructureData - pLastStructure);

	SYMBOL s = symbols->GetSymbolAndCursorByIndex( baseSymbolName, symbolIndex, dataIndex );

	while( dataIndex > currIndex )
	{
		pStructureData[0] = 0;
		pStructureData++;
		currIndex = (pStructureData - pLastStructure);
	}

	SANITY_CHECK();
}

//------------------------------------------------------------------------------

bool AccessFileImport::Uninit( void )
{
	delete symbols;
	symbols = NULL;

	ResetStructure( NULL );

	ns_AccessImport::sqldb.Disconnect();

	return 1;
}

//------------------------------------------------------------------------------

void AccessFileImport::ResetStructure( struct Symbol* s)
{
	delete [] newStructure;
	pLastStructure = pStructureData = newStructure = NULL;

	if( s )
	{
		currentStructureSize = PARSER->GetTypeSize(s);
		pLastStructure = pStructureData = newStructure = new BYTE[ currentStructureSize ];
		IMPORT_MEMSET( newStructure, currentStructureSize );
	}
}

//------------------------------------------------------------------------------
// ChangeListImport
//------------------------------------------------------------------------------

void ChangeListImport::Init( struct IFileSystem* _datafile )
{
	datafile = _datafile;
	processType = PT_NONE;

    // Set the members of the OPENFILENAME structure.

    OPENFILENAME ofn = {0}; // common dialog box structure
    char szDirName[256];    // directory string
    char szFileTitle[256];  // file-title string
	szDest[0] = 0;

    ofn.lStructSize = sizeof(OPENFILENAME);
    ofn.hwndOwner = hMainWindow;
    ofn.lpstrFilter = "Change Lists *.lst\0*.lst\0";
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

	processType = PT_IMPORT_FILES;
}

struct ChangeEntry
{
	struct StringList : std::list<std::string>
	{
	};

	std::string type;
	std::string name;
	std::string value;
	StringList  fields;
};

struct ChangeList : std::list<ChangeEntry>
{
};


void ChangeListImport::Execute( HINSTANCE _hInstance, HWND _parent )
{
	ChangeList changeList;

	if( processType == PT_IMPORT_FILES )
	{
		DbSymbols* symbols = new DbSymbols;
		symbols->Init( PARSER );

		FILE* f = fopen( szDest, "rt" );
		if( !f ) return;

		char line[512];
		while( !feof(f) )
		{
			fgets(line,512,f);

			char* newLine = strchr(line,'\n');
			if( newLine )
				*newLine = 0;

			ChangeEntry entry;

			while( !feof(f) && line[0] != '\n' )
			{
				if( entry.type.size() == 0 )
				{
					entry.type = line;
				}
				else if( entry.name.size() == 0 )
				{
					entry.name = line;
				}
				else if( entry.value.size() == 0 )
				{
					entry.value = line;
				}
				else
				{
					entry.fields.push_back(line);
				}

				fgets(line,512,f);
			}

			if( entry.type.size() && entry.name.size() && entry.value.size() && entry.fields.size() )
			{
				changeList.push_back( entry );
			}
		}

		for( ChangeList::iterator it = changeList.begin(); it != changeList.end(); it++ )
		{
			const char* szBaseSymbol = it->type.c_str();  // BT_TERRAINOBJECT
			const char* szFilename   = it->name.c_str();  // TOBJ!!Env!!Act_4_Loc!!DolGuldur!!dolg_door_secret_a
			const char* szValue      = it->value.c_str(); // open

			szBaseSymbol = "BT_ACTION";
			szFilename   = "ACTION!!Walk";
			szValue      = "";

			szBaseSymbol = "BT_TERRAINOBJECT";
			szFilename   = "TOBJ!!Env!!Act_4_Loc!!DolGuldur!!dolg_door_secret_a";
			szValue      = "";

			std::string fn = std::string("\\") + szBaseSymbol + std::string("\\") + szFilename;

			//fn = "TOBJ!!Env!!Act_4_Loc!!DolGuldur!!dolg_door_secret_a";

			DAFILEDESC fdesc;
			fdesc.dwDesiredAccess = GENERIC_READ |GENERIC_WRITE;
			fdesc.dwCreationDistribution = OPEN_EXISTING;
			fdesc.lpFileName = fn.c_str();
			fdesc.lpImplementation = "DOS";

			//datafile->SetCurrentDirectory("\\BT_TERRAINOBJECT");

			COMPTR<IFileSystem> file;
			if( datafile->CreateInstance(&fdesc,file) == GR_OK )
			{
				int fileSize;
				fileSize = file->GetFileSize();

				SYMBOL baseSymbol = symbols->GetSymbolByName( szBaseSymbol );
				if( baseSymbol )
				{
					int symbSize;
					SH::GetBaseRecordSize( baseSymbol, symbSize, PARSER );

					DWORD bytesRead;
					fileSize = symbSize;
					void* p = alloca(symbSize);
					file->ReadFile( 0, p, fileSize, &bytesRead );
					
					OutputDebugString( szFilename );
				}
			}
		}

		return;
	}
}

bool ChangeListImport::Update( HWND _progressBar )
{
	return 0;
}

bool ChangeListImport::Uninit( void )
{
	return 0;
}

