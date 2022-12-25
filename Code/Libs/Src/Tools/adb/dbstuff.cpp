//--------------------------------------------------------------------------//
//                                                                          //
//                             DbStuff.cpp                                  //
//                                                                          //
//                  COPYRIGHT (C) 1997 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Libs/Src/Tools/adb/dbstuff.cpp 14    11/25/03 11:07a Ajackson $
*/			    
//--------------------------------------------------------------------------//

#include "pch.h"

#include "DbStuff.h"
#include "Resource.h"
#include "stdio.h"
#include "UserDefaults.h"
#include "TDocClient.h"
#include "IStructEnumerator.h"
#include "IStringSet.h"

#pragma warning (disable: 4700)

#include <fdump.h>
#include <StdDat.h>
#include <ViewCnst.h>
#include <Viewer.h>
#include <malloc.h>
#include <Document.h>
#include <IDocClient.h>
#include <DACOM.h>
#include <TSmartPointer.h>
#include <mmsystem.h>
#include <TComponent.h>
#include <MemFile.h>
#include <time.h>
#include <string>
#include <CommCtrl.h>

#include "dbTreeView.h"
extern DbTreeView g_DbTreeView;

#include "..\..\Common\ProcessHeaders.h"

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

std::string g_DefaultNameString;

#define PARSEDIR "ParseData"
#define DEBUG_PRINT 0
//--------------------------------------------------------------------------
// enumerate the class members, and the current values
//
struct DACOM_NO_VTABLE StructEnumerator : public IStructEnumerator
{
	BEGIN_DACOM_MAP_INBOUND(StructEnumerator)
	DACOM_INTERFACE_ENTRY(IStructEnumerator)
	DACOM_INTERFACE_ENTRY2(IID_IStructEnumerator, IStructEnumerator)
	END_DACOM_MAP()

	COMPTR<IFileSystem> outFile;
	int indent, currentPos;

	const int INDENT;
	const int VALUE;
	const int TABSIZE;

	//
	// IStructEnumerator methods
	//

	DEFMETHOD_(BOOL32,BeginEnumStruct) (IViewer *viewer, const char * structName, const char *instanceName)
	{
		do_indention();
		write_string(instanceName);
		newline();
		inc_indent();
		return 1;
	}

	DEFMETHOD_(BOOL32,EndEnumStruct) (IViewer *viewer)
	{
		dec_indent();
		return 1;
	}

	DEFMETHOD_(BOOL32,BeginEnumArray) (IViewer *viewer, const char * structName, const char *instanceName, int size)
	{
		do_indention();
		write_string(instanceName);
		newline();
		inc_indent();
		return 1;
	}

	DEFMETHOD_(BOOL32,EndEnumArray) (IViewer *viewer)
	{
		dec_indent();
		return 1;
	}

	DEFMETHOD_(BOOL32,EnumMember) (IViewer *viewer, const char * typeName, const char * instanceName, const char * value)
	{
		do_indention();
		write_string(instanceName);
		do_indention(VALUE);
		write_string(value);
		newline();
		return 1;
	}

	//
	// StructEnumerator methods
	//

	void do_indention (void)
	{
		do_indention(indent);
	}

	void write_string (const char * string)
	{
		DWORD dwWritten;
		if (string[0] == 1)
			string++;
		int len = strlen(string);
		if (string[len-1] == 1)
			len--;

#if DEBUG_PRINT
		OutputDebugString(string);
#endif
		outFile->WriteFile(0, string, len, &dwWritten, 0);
		currentPos += len;
	}

	void newline (void)
	{
		char _newline[2] = { 13, 10 };
		DWORD dwWritten;

#if DEBUG_PRINT
		OutputDebugString("\n");
#endif
		outFile->WriteFile(0, _newline, sizeof(_newline), &dwWritten, 0);
		currentPos = 0;
	}

	void do_indention (int collumn)
	{
		DWORD dwWritten;
		char buffer[256];
		int i;

		// tab over at least once
		if (collumn <= currentPos)
			collumn += TABSIZE;
		collumn = (collumn / TABSIZE) * TABSIZE;
		currentPos = currentPos - (currentPos % TABSIZE);

		for (i=0; currentPos+TABSIZE <= collumn; currentPos+=TABSIZE)
			buffer[i++] = '\t';
		buffer[i] = 0;
		outFile->WriteFile(0, buffer, i, &dwWritten, 0);
#if DEBUG_PRINT
		OutputDebugString(buffer);
#endif
	}

	void inc_indent (void)
	{
		indent += INDENT;
	}

	void dec_indent (void)
	{
		indent -= INDENT;
	}

	StructEnumerator (void) : INDENT(4), VALUE(40), TABSIZE(4)
	{
		indent = INDENT;
		currentPos = 0;
	}
};

//--------------------------------------------------------------------------
// enumerate the class members, and the current values
//
struct DACOM_NO_VTABLE TableBuilder : public IStructEnumerator
{
	BEGIN_DACOM_MAP_INBOUND(TableBuilder)
	DACOM_INTERFACE_ENTRY(IStructEnumerator)
	DACOM_INTERFACE_ENTRY2(IID_IStructEnumerator, IStructEnumerator)
	END_DACOM_MAP()

	COMPTR<IFileSystem> outFile;

	struct RowEntry
	{
		RowEntry * next;
		char string[128];

		RowEntry(const char * info)
		{
			strcpy(string,info);
			next = NULL;
		};

		~RowEntry()
		{
			delete next;
		}
	};

	struct ColumnEntry
	{
		ColumnEntry * next;
		RowEntry * entry;

		ColumnEntry()
		{
			next = NULL;
			entry = NULL;
		}
		
		~ColumnEntry()
		{
			delete next;
			delete entry;
		}
	};

	char currentName[128];
	ColumnEntry * baseColumn;
	ColumnEntry * currentColumn;
	RowEntry * currentRow;
	RowEntry * typeRow;
	U32 recurse;

	//
	// IStructEnumerator methods
	//

	DEFMETHOD_(BOOL32,BeginEnumStruct) (IViewer *viewer, const char * structName, const char *instanceName)
	{
		if(!recurse)
		{
			if(baseColumn)
			{
				if(strcmp(currentName,structName))
				{
					blastToFile();
					strcpy(currentName,structName);
				}
			}
			if(baseColumn == NULL)
			{
				baseColumn = new ColumnEntry();
				currentColumn = new ColumnEntry();
				baseColumn->next = currentColumn;
				typeRow = new RowEntry(structName);
				baseColumn->entry = typeRow;
				currentRow = new RowEntry(instanceName);
				currentColumn->entry = currentRow;
			}
			else
			{
				typeRow = NULL;
				ColumnEntry * tmp = new ColumnEntry;
				currentColumn->next = tmp;
				currentColumn = tmp;
				currentRow = new RowEntry(instanceName);
				currentColumn->entry = currentRow;
			}
		}
		++recurse;
		return 1;
	}

	DEFMETHOD_(BOOL32,EndEnumStruct) (IViewer *viewer)
	{
		--recurse;
		return 1;
	}

	DEFMETHOD_(BOOL32,BeginEnumArray) (IViewer *viewer, const char * structName, const char *instanceName, int size)
	{
		return 1;
	}

	DEFMETHOD_(BOOL32,EndEnumArray) (IViewer *viewer)
	{
		return 1;
	}

	DEFMETHOD_(BOOL32,EnumMember) (IViewer *viewer, const char * typeName, const char * instanceName, const char * value)
	{
		if(typeRow)
		{
			RowEntry * tmp = new RowEntry(instanceName);
			typeRow->next = tmp;
			typeRow = tmp;
		}
		RowEntry * newInfo = new RowEntry(value);
		currentRow->next = newInfo;
		currentRow = newInfo;
		return 1;
	}

	void blastToFile()
	{
		U32 dwWritten;
		U32 depth = 0;
		while(1)
		{
			currentColumn = baseColumn;
			while(currentColumn)
			{
				currentRow = currentColumn->entry;
				U32 curDepth = 0;
				while(depth > curDepth)
				{
					currentRow = currentRow->next;
					++curDepth;
					if(!currentRow)
						break;
				}
				if(!currentRow)
					break;
				outFile->WriteFile(0, currentRow->string, strlen(currentRow->string), &dwWritten, 0);
				outFile->WriteFile(0, "\t", strlen("\t"), &dwWritten, 0);
				currentColumn = currentColumn->next;
			}
			outFile->WriteFile(0, "\n", strlen("\n"), &dwWritten, 0);
			if(!currentRow)
				break;
			++depth;
		}
		outFile->WriteFile(0, "\n", strlen("\n"), &dwWritten, 0);
		delete baseColumn;
		baseColumn = NULL;
		currentColumn = NULL;
		currentRow = NULL;
		typeRow = NULL;
	}

	TableBuilder (void)
	{
		baseColumn = NULL;
		currentColumn = NULL;
		currentRow = NULL;
		typeRow = NULL;
		recurse = 0;
	}

	~TableBuilder ()
	{
		delete baseColumn;
		baseColumn = NULL;
		currentColumn = NULL;
		currentRow = NULL;
		typeRow = NULL;
	}
};
//---------------------------------------------------------------------------------
//---------------------------------------------------------------------------------

#define NUM_VIEWERS 8

GENRESULT CreateDocument(IFileSystem *memfile,IDocument **doc, DWORD dwCreation=OPEN_EXISTING);
GENRESULT CreateStringSet();

BOOL32 WriteToText (HWND hwnd, const char * fileName);

struct DB_ENTRY
{
	//void *structure;
	char name[64];
};

DB_ENTRY dbArray[256];

char * pPreprocessResources=0;
char * pPreprocessBlock=0;
HANDLE hSymbols = NULL;

IViewer *viewer[NUM_VIEWERS];
IDocument *docArray[NUM_VIEWERS];
IDocument *mainDoc = NULL;
IStringSet *stringSet = NULL;
char currentFileName[MAX_PATH];
BOOL32 saved = TRUE;
BOOL32 RO = FALSE;
//IDocument *_oldDoc;  (jy)
//IFileSystem *_newDoc; (jy)
_STATE _state;

//--------------------------------------------------------------------------------------------------
//

namespace NameTable
{
	U32 dwSize = 0;
	DynamicArray<char *> nameTable;

	S32 NameIndex( const char* _name )
	{
		U32 i;

		for (i=0;i<NameTable::dwSize;i++)
		{
			if (strcmp(_name,nameTable[i]) == 0)
			{
				return i;
			}
		}

		return -1;
	}

	bool New( const char* _name )
	{
		nameTable[NameTable::dwSize] = new char[strlen(_name)+1];
		strcpy(nameTable[NameTable::dwSize],_name);
		NameTable::dwSize++;
		return true;
	}

	bool Delete( const char* _name )
	{
		S32 tableIndex = NameIndex(_name);
		
		if( tableIndex != -1 )
		{
			nameTable[tableIndex][0] = 0;
		}
		return (tableIndex != -1);
	}

	bool Rename( const char* _oldName, const char* _newName )
	{
		S32 tableIndex = NameIndex(_oldName);
		
		if( tableIndex != -1 )
		{
			delete [] nameTable[tableIndex];

			nameTable[tableIndex] = new char[strlen(_newName)+1];
			strcpy(nameTable[tableIndex],_newName);
		}
		else
			return NameTable::New(_newName);

		return true;
	}

	void Free()
	{
		unsigned int i;
		for (i=0;i<NameTable::dwSize;i++)
		{
			delete [] nameTable[i];
		}
		nameTable.free();
		NameTable::dwSize = 0;
	}
}

//--------------------------------------------------------------------------------------------------
//

struct StringSetFinder : public IStringSetCallback
{
	virtual GENRESULT StringSetCallback(const char * string);
};

GENRESULT StringSetFinder::StringSetCallback(const char * string)
{
	char buffer[128];
	strcpy(buffer,string);
	OpenArchetype(buffer);
	return GR_OK;
};

StringSetFinder finder;

//U32 corrTable[4096];
//U32 ctblPos = 0;
struct DocManager : DocumentClient
{
	BEGIN_DACOM_MAP_INBOUND(DocManager)
	DACOM_INTERFACE_ENTRY(IDocumentClient)
	END_DACOM_MAP()

	DEFMETHOD(OnUpdate) (struct IDocument *doc, const C8 *message = 0, void *parm = 0);

} *docManager;

GENRESULT DocManager::OnUpdate(struct IDocument *doc, const C8 *message, void *parm)
{
	saved = FALSE;
	UpdateTitle();

	return GR_OK;
}

BOOL Image(IFileSystem * file,IFileSystem * newFile);

inline BOOL32 MyPreprocessFile (const C8 *fileIn)
{
	STARTUPINFO info;
	BOOL32 result;
	char commandLine[128];
	PROCESS_INFORMATION processInfo;

	memset(&info, 0, sizeof(info));
	info.cb = sizeof(info);

	strcpy(commandLine, "cl /P /EP /nologo ");
	strcat(commandLine, fileIn);

	info.dwFlags    = STARTF_USESTDHANDLES;
	info.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
	info.hStdError  = GetStdHandle(STD_ERROR_HANDLE);
	info.hStdInput  = GetStdHandle(STD_INPUT_HANDLE);

	result = CreateProcess(0, commandLine, 0, 0,
			1,	// bInheritHandles
			NORMAL_PRIORITY_CLASS | DETACHED_PROCESS,
			0, 0, &info, &processInfo);

	if (result)
	{
		WaitForSingleObject(processInfo.hProcess, INFINITE);

		CloseHandle(processInfo.hProcess);
		CloseHandle(processInfo.hThread);
	}

	return result;
}
//--------------------------------------------------------------------------------------------------
//
void * PreprocessToMemory (const C8 *filename)
{
	HANDLE hTemp;
	void *pMemory=0;
	DWORD dwFileSize, dwBytesRead;

	if (MyPreprocessFile(filename) == 0)
	{
		MessageBox(hMainWindow, "CL.exe or header file not found. Disabling external header processing.", "Compilation Failed", MB_ICONSTOP | MB_OK);
		goto Done;
	}

 	hTemp = ::CreateFile("gametypes.i", GENERIC_READ|GENERIC_WRITE, 0, 0, OPEN_EXISTING, FILE_FLAG_DELETE_ON_CLOSE, 0);

	if(hTemp == (void*)0xffffffff)
		goto Done;

	if ((dwFileSize = GetFileSize(hTemp, 0)) == 0)
		goto Done;

	if ((pMemory = malloc(dwFileSize+1)) == 0)
		goto Done;

	memset(pMemory, 0, dwFileSize+1);
	ReadFile(hTemp, pMemory, dwFileSize, &dwBytesRead, 0);

Done:
	if (hTemp != INVALID_HANDLE_VALUE)
		CloseHandle(hTemp);
	return pMemory;
}
//--------------------------------------------------------------------------------------------------
//
static BOOL32 ParseDefFile (char *fileName)
{
	BOOL32 result = 1;
	char buffer[MAX_PATH];
	USER_DEFAULTS *iggy = DEFAULTS->GetDefaults();
	if (!iggy->path[0])
		strcpy(iggy->path,"Dinclude");

	sprintf(buffer,"/I%s /D_ADB %s\\%s",iggy->path,iggy->path,fileName);

	if (hSymbols)
		PARSER->DestroySymbols(hSymbols);

	if (pPreprocessBlock)
		free(pPreprocessBlock);
	pPreprocessBlock = (char *) ::PreprocessToMemory(buffer);

	if (pPreprocessBlock && (hSymbols = PARSER->ParseNewMemory(pPreprocessBlock)) != INVALID_HANDLE_VALUE)
	{
	//	EnumTypes();
	}
	else
	{
		result = 0;
		iggy->disableHeader = 1;
	}
//Done:

	return result;
}

//--------------------------------------------------------------------------//
//
static BOOL32 recursiveDelete (IFileSystem * file, const char * dirName)
{
	WIN32_FIND_DATA data;
	HANDLE handle;
	int result=0;

	if (file->SetCurrentDirectory(dirName) == 0)
		return 0;

	if ((handle = file->FindFirstFile("*.*", &data)) == INVALID_HANDLE_VALUE)
	{
		result = 1;		// directory is empty ?
		goto Done;
	}

	do
	{
		// make sure this not a silly "." entry
		if (data.cFileName[0] != '.')
		{
			result++;

			if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				// traverse subdirectory
				recursiveDelete(file, data.cFileName);
			}
			else 
			{	
				file->DeleteFile(data.cFileName);
			}
		}

	} while (file->FindNextFile(handle, &data));

	file->FindClose(handle);
Done:
	file->SetCurrentDirectory("..");	// restore current directory
	result = file->RemoveDirectory(dirName);

	return result;
}
//--------------------------------------------------------------------------//
//
static BOOL32 ParseResourceFile(char *filename)
{
	HANDLE hTemp;
	char* pMemory=0;
	DWORD dwFileSize, dwBytesRead;
	std::string szData;
	char* pData;
	char* pDataNext;

	DWORD openFlags = GENERIC_READ;

 	hTemp = ::CreateFile(filename, openFlags, 0, 0, OPEN_EXISTING, 0, 0);

	if( hTemp == INVALID_HANDLE_VALUE ) 
		return FALSE;

	if ((dwFileSize = GetFileSize(hTemp, 0)) == 0)
		goto Done;

	pMemory = new char[dwFileSize];
	memset( pMemory, 0, dwFileSize);
	ReadFile(hTemp, pMemory, dwFileSize, &dwBytesRead, 0);
	CloseHandle(hTemp);
	hTemp = INVALID_HANDLE_VALUE;

	szData  = "enum AppStrings\n{\n";

	pData = pMemory;
	pDataNext = 0;
	while( pData )
	{
		pDataNext = strchr(pData,'\n');
		if( pDataNext )
		{
			*pDataNext = 0;
			pDataNext++;
		}

		if( strstr(pData,"#define ") )
		{
			// knock off the '\r' character
			pData[ strlen(pData)-1 ] = 0;

			// skip the #define tag
			pData += strlen("#define ");

			// just the String Data
			if( strstr(pData,"IDS_")==0 )
			{
				pData = pDataNext;
				continue;
			}

			// to avoid namespace problems
			szData += "I";

			// get the label
			while( pData[0] != ' ' )
			{
				szData += pData[0];
				pData++;
			}

			// skip the white space
			while( pData[0] == ' ' )
			{
				pData++;
			}

			// end off the data enum string
			szData += " = ";
			szData += pData;
			szData += ",\n";
		}

		pData = pDataNext;
	}

	szData += "};\n";

Done:
	if (hTemp != INVALID_HANDLE_VALUE)
		CloseHandle(hTemp);

	delete [] pMemory;

	if( szData.size() )
	{
		int newSize = szData.size();
		pPreprocessResources = new char[ newSize ];
		memset( pPreprocessResources, 0x00, newSize );
		memcpy( pPreprocessResources, szData.c_str(), newSize );

		USER_DEFAULTS* defs = DEFAULTS->GetDefaults();

		std::string respath = std::string(defs->path) + std::string("\\DAppStrings.h");

		FILE* f = fopen( respath.c_str(), "wt" );
		if( f )
		{
			DWORD computerNameLen = 256;
			char  computerName[256];
			::GetComputerName(computerName,&computerNameLen);

			time_t timer;
			::time( &timer );

			fprintf(f, "// Auto Created[%s] %s\n", computerName, asctime(localtime(&timer)) );
			fprintf(f, "#ifndef _APP_STRING_DATA_HEADER_H_\n" );
			fprintf(f, "#define _APP_STRING_DATA_HEADER_H_\n" );

			fwrite( pPreprocessResources, 1, newSize, f );

			fprintf(f, "#endif\n" );
			fflush( f );
			fclose( f );

		}
	}

	return TRUE;

}
//--------------------------------------------------------------------------//
//
static BOOL32 ParseNewDefFile (char *fileName)
{
	BOOL32 result = 0;
	char buffer[MAX_PATH];
	USER_DEFAULTS * defaults = DEFAULTS->GetDefaults();
	
	if (defaults->disableHeader==0)
	{
		//
		// process the resource.h file
		//
		if( pPreprocessResources )
			delete []pPreprocessResources;

		ParseResourceFile( defaults->pathToResourceFile );

		//
		// the rest of the DInclude headers
		//
		if (!defaults->path[0])
			strcpy(defaults->path,"Dinclude");

		sprintf(buffer,"/I%s /D_ADB %s\\%s",defaults->path,defaults->path,fileName);
			
		if (pPreprocessBlock)
			free(pPreprocessBlock);
			
		if ((pPreprocessBlock = (char *) ::PreprocessToMemory(buffer)) ==0)
		{
			//	MessageBox(0,"Couldn't find header file",fileName,MB_OK);
			defaults->disableHeader = 1;
			goto Done;
		}
	
		// does normal parsing here
		result = (PARSER->ParseMemory(pPreprocessBlock) == GR_OK);
	}
		
	EnumTypes();

	result = 1;

Done:
	return result;
}
//--------------------------------------------------------------------------------------------------
//
BOOL __stdcall  
PrefsDlgProc (HWND hwnd, UINT message, UINT wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_INITDIALOG:
		USER_DEFAULTS *iggy;

		iggy = DEFAULTS->GetDefaults();

		SetFocus(GetDlgItem(hwnd,IDC_AUTOLOAD));
		
		if (iggy->autoLoad)
			CheckDlgButton(hwnd,IDC_AUTOLOAD,BST_CHECKED);
		else
			CheckDlgButton(hwnd,IDC_AUTOLOAD,BST_UNCHECKED);


		SetDlgItemText(hwnd,IDC_DEFAULTH,iggy->defaultH);
		if (!iggy->path[0])
			strcpy(iggy->path,"Dinclude");
		SetDlgItemText(hwnd,IDC_PATH,iggy->path);
		SetDlgItemText(hwnd,IDC_PATH_RES,iggy->pathToResourceFile);
		SetDlgItemText(hwnd,IDC_PATH_TO_FILES,iggy->pathToObjects);

		if (iggy->disableHeader)
		{
			CheckDlgButton(hwnd,IDC_DISABLE_HEADER,BST_CHECKED);
			PostMessage(hwnd, WM_COMMAND, IDC_DISABLE_HEADER, 0);
		}
		break;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
			{
				USER_DEFAULTS *iggy=DEFAULTS->GetDefaults();

				iggy->autoLoad = IsDlgButtonChecked(hwnd,IDC_AUTOLOAD);
				GetDlgItemText(hwnd,IDC_DEFAULTH,iggy->defaultH,32);
				GetDlgItemText(hwnd,IDC_PATH,iggy->path,128);
				GetDlgItemText(hwnd,IDC_PATH_RES,iggy->pathToResourceFile,256);
				GetDlgItemText(hwnd,IDC_PATH_TO_FILES,iggy->pathToObjects,256);
				EndDialog(hwnd, 0);
			}
			break;
		case IDCANCEL:
			EndDialog(hwnd, 0);
			break;

		case IDC_DISABLE_HEADER:
			{
				USER_DEFAULTS *iggy=DEFAULTS->GetDefaults();
				iggy->disableHeader = (IsDlgButtonChecked(hwnd,IDC_DISABLE_HEADER) != 0);
				if (iggy->disableHeader)
				{
					// disable all header - related controls
					EnableWindow(GetDlgItem(hwnd,IDC_DEFAULTH), 0);
					EnableWindow(GetDlgItem(hwnd,IDC_PATH), 0);
					EnableWindow(GetDlgItem(hwnd,IDC_PATH_RES), 0);
					EnableWindow(GetDlgItem(hwnd,IDC_PATH_TO_FILES), 0);
				}
				else
				{
					// enable all header - related controls
					EnableWindow(GetDlgItem(hwnd,IDC_DEFAULTH), 1);
					EnableWindow(GetDlgItem(hwnd,IDC_PATH), 1);
					EnableWindow(GetDlgItem(hwnd,IDC_PATH_RES), 1);
					EnableWindow(GetDlgItem(hwnd,IDC_PATH_TO_FILES), 1);
				}
			}
			break;
		}
		break;
	}

	return 0;
}		
//--------------------------------------------------------------------------------------------------
//
void
DoPrefsDialog()
{
	DialogBoxParam(hInstance, MAKEINTRESOURCE(IDD_PREFS), hMainWindow, 
		PrefsDlgProc, NULL);

}
//--------------------------------------------------------------------------------------------------
//
void InitDB()
{
	U32 i;
	USER_DEFAULTS *iggy;

	docManager = new DAComponent<DocManager>;

	for (i=0;i<256;i++)
	{
	//	dbArray[i].structure = NULL;
		strcpy(dbArray[i].name,"");
	}

	for (i=0;i<NUM_VIEWERS;i++)
	{
		viewer[i] = NULL;
	}

	iggy = DEFAULTS->GetDefaults();
	
	if (iggy->autoLoad)
	{
		char name[MAX_PATH];
	//	DOCDESC ddesc;
	//	U32 attrib;

		DEFAULTS->GetNameInMRU(name,IDS_DB_FILTER);

		OpenDB(name);
	}
	else
	{
		NewDB();
	}
}
//--------------------------------------------------------------------------------------------------
//
S32 GetViewerSlot(IDocument *newDoc)
{
	S32 i=0;
	BOOL32 visible;

	//look for already open viewer
	for (i=0;i<NUM_VIEWERS;i++)
	{
		if (newDoc == docArray[i])
		{
			//this guy's a duplicate....so release
		//	newDoc->Release();
			return i;
		}
	}

	//look for empty viewer slot
	for (i=0;i<NUM_VIEWERS;i++)
	{
		if (viewer[i] == NULL)
		{
			docArray[i] = newDoc;
			docArray[i]->AddRef();
			return i;
		}
	}

	//look for open viewer I can replace
	for (i=0;i<NUM_VIEWERS;i++)
	{
		viewer[i]->get_display_state(&visible);
		if (visible == FALSE)
		{
			viewer[i]->Release();
			viewer[i] = NULL;
	
			docArray[i]->CloseAllClients();

			docArray[i]->Release();
			docArray[i] = newDoc;
			docArray[i]->AddRef();
			return i;
		}
	}
	
	return -1;
}
//--------------------------------------------------------------------------------------------------
//
S32 FindOpenViewer(IDocument *newDoc)
{
	U32 i;

	for (i=0;i<NUM_VIEWERS;i++)
	{
		if (newDoc == docArray[i])
			return i;
	}
	
	return -1;
}
//--------------------------------------------------------------------------------------------------
//
BOOL CALLBACK NameDlgProc(HWND hwnd, UINT message, UINT wParam, LPARAM lParam)
{
	if (message == WM_INITDIALOG)
        SetWindowLong(hwnd, DWL_USER, lParam);
	char *name = (char *)GetWindowLong(hwnd, DWL_USER);

	switch (message)
	{
	case WM_INITDIALOG:
		char prompt[256];
		sprintf(prompt,"Enter name of new (%s) Archetype",(char *)lParam);
		
		//cheap fix -- must invalidate name for second use
		((char *)lParam)[0] = 0;
		
		SetDlgItemText(hwnd,IDC_INFO,prompt);
		SetDlgItemText(hwnd,IDC_EDIT1, g_DefaultNameString.c_str() );
		SetFocus(GetDlgItem(hwnd,IDC_EDIT1));
		break;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
			GetDlgItemText(hwnd, IDC_EDIT1, name, 64);
			
			if (name[0] == 0)
				MessageBox(hwnd,"Operation cancelled","Bad Name",MB_OK);
			else
				if (NameTable::NameIndex(name) != -1)
				{
					MessageBox(hwnd,"Duplicate Name","Operation Cancelled",MB_OK);
					name[0] = 0;
				}
			
			EndDialog(hwnd, 0);
			break;
		case IDCANCEL:
			EndDialog(hwnd, 0);
			break;
		}
		break;
	}

	return 0;

}
//--------------------------------------------------------------------------------------------------
//
IViewer **OpenViewer(IDocument *doc,char *baseName,char *instName)
{
	VIEWDESC vdesc;
    HWND hwnd;
	S32 slot;

//	IViewer **curViewer;

	vdesc.className = baseName;
    vdesc.doc = doc;
	vdesc.spellingSet = stringSet;

    if ((slot = GetViewerSlot(doc)) == -1)
	{
		MessageBox(hMainWindow,"Already max viewers open","Greedy User",MB_OK);
		return NULL;
	}
	
	if (viewer[slot])
	{
		(viewer[slot])->set_display_state(1);
		return &viewer[slot];
	}
	else
	{
		doc->AddRef();
		doc->Release();
		if (PARSER->CreateInstance(&vdesc, (void **)&viewer[slot]) == GR_OK)
		{
			//??I don't ever want this extra reference, so just throw it out now and don't worry about it
		//	viewer[slot]->Release();
			doc->AddRef();
			doc->Release();
//			viewer[slot]->Invoke("set_spelling_list", &stringSet);
			(viewer[slot])->get_main_window((void**)&hwnd);
			MoveWindow(hwnd, 100+slot*20, 100+slot*20, 350+slot*20, 200+slot*20, 1);

			(viewer[slot])->set_viewer_path( DEFAULTS->GetDefaults()->pathToObjects );
			(viewer[slot])->set_instance_name(instName);
			(viewer[slot])->set_display_state(1);

			return &viewer[slot];
		}
	}
	
	return NULL;
}
//--------------------------------------------------------------------------------------------------
//
void Alias_SaveArchetype( char *atName, char *atNewName )
{
	if( mainDoc->SetCurrentDirectory("\\ALIAS") == false )
	{
		// attempt to create the ALIAS directory if can not set it
		mainDoc->CreateDirectory("\\ALIAS");
	}

	if( mainDoc->SetCurrentDirectory("\\ALIAS") )
	{
		// make sure the "atNewName" archetype name is unique
		mainDoc->DeleteFile( atNewName );
		NameTable::Rename( atName, atNewName );

		struct AliasStruct
		{
			DWORD dwSize;
			char  name[128];
		};

		DAFILEDESC fdesc;
		fdesc.dwDesiredAccess = GENERIC_WRITE;
		fdesc.dwShareMode = FILE_SHARE_READ;
		fdesc.dwCreationDistribution = CREATE_ALWAYS;
		fdesc.lpFileName = atName;
		fdesc.lpImplementation = "DOS";

		COMPTR<IFileSystem> fs;
		U32 dwWritten;

		if( mainDoc->CreateInstance(&fdesc,fs) == GR_OK )
		{
			AliasStruct alias;
			memset( &alias, 0, sizeof(alias) );
			alias.dwSize = sizeof(alias);
			strcpy( alias.name, atNewName ); 

			fs->WriteFile(0, &alias, alias.dwSize, &dwWritten, 0);
		}

		mainDoc->SetCurrentDirectory("\\");
	}
}
//--------------------------------------------------------------------------------------------------
//
void Alias_DeleteArchetype(char* atName)
{
	if( mainDoc->SetCurrentDirectory("\\ALIAS") )
	{
		// make sure the "atNewName" archetype name is unique
		mainDoc->DeleteFile( atName );
		NameTable::Delete( atName );
	}
}
//--------------------------------------------------------------------------------------------------
//
bool Alias_FindArchetype(char* atName)
{
	if( mainDoc->SetCurrentDirectory("\\ALIAS") == false )
	{
		return false;
	}

	WIN32_FIND_DATA findArch;
	HANDLE hFile = mainDoc->FindFirstFile("*.*", &findArch);

	// make sure the "atNewName" archetype name is unique
	while( hFile != INVALID_HANDLE_VALUE )
	{
		if( !strcmp(findArch.cFileName,atName) )
		{
			return true;
		}
		
		if( mainDoc->FindNextFile(hFile,&findArch) == false )
		{
			hFile = INVALID_HANDLE_VALUE;
		}
	}

	return false;
}
//--------------------------------------------------------------------------------------------------
//
void NewArchetype(char *baseName,char *defname)
{
	char nameBuffer[64];	
	U32 i=0;
	DOCDESC ddesc;
	void *structure;
	char pathName[MAX_PATH];
	U32 dwWritten;
	COMPTR<IDocument> newDoc;

	//here, nameBuffer is doing dual duty to pass the base name in and get the new name out.
	strcpy(nameBuffer,baseName);
	
	g_DefaultNameString = defname;

	DialogBoxParam(hInstance, MAKEINTRESOURCE(IDD_NEW_ARCHETYPE), hMainWindow, NameDlgProc,(LPARAM) &nameBuffer);

	if (nameBuffer[0] == 0)
		return;

	if (Alias_FindArchetype(nameBuffer))
	{
		if( ::MessageBox(hMainWindow, "Alias with same name, continue?", nameBuffer, MB_YESNO ) == IDNO )
		{
			return;
		}
		Alias_DeleteArchetype(nameBuffer);
	}

	saved = FALSE;
	UpdateTitle();

	const U32 typeSize = PARSER->GetTypeSize(PARSER->GetSymbol(0,baseName));
	structure = (typeSize) ? calloc(typeSize,1) : 0;
	
	//open the appropriate directory - if it isn't there, create it
	sprintf(pathName,"\\%s",baseName);
	mainDoc->GetChildDocument(pathName,newDoc);
	if (newDoc == NULL)
		mainDoc->CreateDirectory(pathName);

	//get at the desired file
	sprintf(pathName,"\\%s\\%s",baseName,nameBuffer);
	mainDoc->GetChildDocument(pathName,newDoc);

	if (newDoc == NULL)
	{
		DOCDESC fdesc;
		
		fdesc = pathName;
		fdesc.memory = NULL;
		fdesc.lpImplementation = "DOS";
		fdesc.dwDesiredAccess = GENERIC_READ|GENERIC_WRITE;
		fdesc.dwShareMode = FILE_SHARE_READ | FILE_SHARE_WRITE; 
		fdesc.dwCreationDistribution = CREATE_ALWAYS;
		mainDoc->CreateInstance(&fdesc, (void **)&newDoc);
		
    
		newDoc->WriteFile(0, structure, PARSER->GetTypeSize(PARSER->GetSymbol(0,baseName)), &dwWritten, 0);
	}

	NameTable::New(nameBuffer);
	
	free(structure);

	OpenViewer(newDoc,baseName,nameBuffer);

	newDoc->AddRef();
	newDoc->Release();
};
//--------------------------------------------------------------------------------------------------
//
struct FileEnumerator
{
	// return false to discontinue the enumeration
	virtual bool Callback (const WIN32_FIND_DATA & dataFound, const char * pathName, bool & bSkipDir) = 0;
};
//--------------------------------------------------------------------------------------------------
//
bool enumerateFiles (FileEnumerator * fileEnum, char * pathName)
{
	HANDLE handle;
	WIN32_FIND_DATA dataFound;
	int len = strlen(pathName);
	bool bSkipDir = false;

	strcpy(pathName+len, "\\*.*");
	handle = mainDoc->FindFirstFile(pathName,&dataFound);
	if (handle == INVALID_HANDLE_VALUE)
		return true;
	
	// check first file
	if (handle != INVALID_HANDLE_VALUE)
	{
		do
		{
			if (dataFound.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				// skip the parse dir, and any other invalid directories
				
				if( !strcmp(dataFound.cFileName, PARSEDIR) || dataFound.cFileName[0] == '.' )
				{
					char path[MAX_PATH];
					strcpy(path, pathName);
					path[len] = '\\';
					strcpy(path+len+1, dataFound.cFileName);
				}
				else
				{
					char path[MAX_PATH];
					strcpy(path, pathName);
					path[len] = '\\';
					strcpy(path+len+1, dataFound.cFileName);

					if (enumerateFiles(fileEnum, path)==false)
					{
						strcpy(pathName, path);
						mainDoc->FindClose(handle);
						return false;
					}
				}
			}
			else
			{
				pathName[len] = 0;
				if (fileEnum->Callback(dataFound, pathName, bSkipDir)==false)
				{
					mainDoc->FindClose(handle);
					return false;
				}

				if (bSkipDir)		// ignore the rest of this directory
					break;
			}
		}
		while (mainDoc->FindNextFile(handle,&dataFound));

		mainDoc->FindClose(handle);
	}

	return true;
}
//--------------------------------------------------------------------------------------------------
//
bool EnumerateFiles (FileEnumerator * fileEnum)
{
	char pathName[MAX_PATH];
	pathName[0] = 0;

	return enumerateFiles(fileEnum, pathName);
}
//--------------------------------------------------------------------------------------------------
//
struct FindArchetypeEnumerator : FileEnumerator
{
	const char * archeName;
	char  typeName[64];		// result param = parent directory name
	IDocument **newDoc;
	bool bFullPath;

	FindArchetypeEnumerator (const char *_archeName, IDocument **_newDoc, bool _bFullPath) : archeName(_archeName) , newDoc(_newDoc), bFullPath(_bFullPath)
	{
		typeName[0] = 0;
		*newDoc = 0;
	}

	virtual bool Callback (const WIN32_FIND_DATA & dataFound, const char * pathName, bool & bSkipDir)
	{
		if (stricmp(archeName, dataFound.cFileName) == 0)
		{
			mainDoc->SetCurrentDirectory(pathName);
			mainDoc->GetChildDocument(archeName,newDoc);
			ASSERT(*newDoc != 0);
			mainDoc->SetCurrentDirectory("\\");

			char * tmp;
			if (bFullPath==false && (tmp = strrchr(pathName, '\\')) != 0)
				strcpy(typeName, tmp+1);
 		else
				strcpy(typeName, pathName);

			return false;
		}
		return true;
	}
};
//--------------------------------------------------------------------------------------------------
//
void FindArchetype (IDocument **newDoc,char *baseName,char *atName) //"atName" means archtype name
{
	FindArchetypeEnumerator fenum(atName, newDoc, false);

	if (EnumerateFiles(&fenum)==false)
		strcpy(baseName, fenum.typeName);
	else
		baseName[0] = 0;
}
//--------------------------------------------------------------------------------------------------
//
void FindArchetype2 (IDocument **newDoc,char *baseName,char *atName) //"atName" means archtype name
{
	FindArchetypeEnumerator fenum(atName, newDoc, true);

	if (EnumerateFiles(&fenum)==false)
		strcpy(baseName, fenum.typeName);
	else
		baseName[0] = 0;
}
//--------------------------------------------------------------------------------------------------
//
void Clone(char *name,char *defname)
{
	char baseName[MAX_PATH];
	char nameBuffer[64];
	COMPTR<IDocument> newDoc;
	U32 dwWritten;
	void *structure;
	char pathName[64];

	FindArchetype2(newDoc,baseName,name);
	newDoc->SetFilePointer(0,0);

	const U32 typeSize = newDoc->GetFileSize();
	structure = typeSize ? calloc(typeSize,1) : 0;
	newDoc->ReadFile(0, structure, typeSize, &dwWritten, 0);

	//here, nameBuffer is doing dual duty to pass the base name in and get the new name out.
	char * tmp;
	if ((tmp = strrchr(baseName, '\\')) != 0)
		strcpy(nameBuffer, tmp+1);
	else
		strcpy(nameBuffer,baseName);
	
	g_DefaultNameString = defname;

	DialogBoxParam(hInstance, MAKEINTRESOURCE(IDD_NEW_ARCHETYPE), hMainWindow, NameDlgProc,(LPARAM)&nameBuffer);

	if (nameBuffer[0] == 0)
		return;

	saved = FALSE;
	UpdateTitle();

	newDoc->AddRef();
	newDoc->Release();
	//open the appropriate directory - if it isn't there, create it
	mainDoc->GetChildDocument(baseName,newDoc);
	if (newDoc == NULL)
		mainDoc->CreateDirectory(baseName);

	//get at the desired file
	sprintf(pathName,"%s\\%s",baseName,nameBuffer);
	mainDoc->GetChildDocument(pathName,newDoc);

	if (newDoc == NULL)
	{
		DOCDESC fdesc;
		
		fdesc = pathName;
		fdesc.memory = NULL;
		fdesc.lpImplementation = "DOS";
		fdesc.dwDesiredAccess = GENERIC_READ|GENERIC_WRITE;
		fdesc.dwShareMode = FILE_SHARE_READ | FILE_SHARE_WRITE; 
		fdesc.dwCreationDistribution = CREATE_ALWAYS;
		mainDoc->CreateInstance(&fdesc, newDoc);
		if (newDoc ==0)
		{
			MessageBox(hMainWindow,"Failed to create new file","Clone failed",MB_OK);
			return;
		}
    
		newDoc->WriteFile(0, structure, typeSize, &dwWritten, 0);
	}
	 
	free(structure);

	OpenViewer(newDoc,baseName,nameBuffer);
	NameTable::New(nameBuffer);

	RefreshList();

	newDoc->AddRef();
	newDoc->Release();
}

void OpenArchetype(char *atName)
{
	char baseName[64];
	COMPTR<IDocument> newDoc;

	FindArchetype(newDoc,baseName,atName);

//	docManager->MakeConnection(newDoc);
	OpenViewer(newDoc,baseName,atName);

	newDoc->AddRef();
	newDoc->Release();
}

void DeleteArchetype(char *atName)
{
	char baseName[64];
	IDocument *newDoc;
	S32 i = -1;
	char path[MAX_PATH];

	FindArchetype(&newDoc,baseName,atName);
	assert(newDoc);

	i = FindOpenViewer(newDoc);

	if (i >= 0)
	{
		if( viewer[i] )
			viewer[i]->Release();
		viewer[i] = NULL;
		docArray[i]->CloseAllClients();
		S32 bob = docArray[i]->Release();
		assert (bob>1);
		docArray[i] = NULL;
	}

//	NukeName( atName );
	
	newDoc->Release();

	sprintf(path,"\\%s\\%s",baseName,atName);
	if (!mainDoc->DeleteFile(path))
	{
		MessageBox(hMainWindow,"Unable to delete archetype","Error",MB_OK);
		DWORD Rick = mainDoc->GetLastError();
	}
	else
	{
		NameTable::Delete( atName );
	}
}
//--------------------------------------------------------------------------------------------------
//
/*
		WIN32_FIND_DATA findArch;
		HANDLE hFile = mainDoc->FindFirstFile("*.*", &findArch);

		// make sure the "atNewName" archetype name is unique
		while( hFile != INVALID_HANDLE_VALUE )
		{
			if( !strcmp(findArch.cFileName,atNewName) )
			{
				// do something
			}
			
			if( mainDoc->FindNextFile(hFile,&findArch) == false )
			{
				hFile = INVALID_HANDLE_VALUE;
			}
		}
*/
//--------------------------------------------------------------------------------------------------
//
void RenameArchetype(char *atName, char *atNewName)
{
	char baseName[64];
	IDocument *newDoc;
	S32 i = -1;

	FindArchetype(&newDoc,baseName,atName);
	assert(newDoc);

	i = FindOpenViewer(newDoc);

	// remove from DataViewers
	if (i >= 0)
	{
		if( viewer[i] )
			viewer[i]->Release();
		viewer[i] = NULL;
		docArray[i]->CloseAllClients();
		S32 bob = docArray[i]->Release();
		assert (bob>1);
		docArray[i] = NULL;
	}
	
	if(newDoc->Rename( atNewName ))
	{
		Alias_SaveArchetype( atName, atNewName );
	}
	else
	{
		::MessageBox(NULL,"Rename Failed: You may need to rename it to somthing else first.","Rename Error",MB_OK);
	}
}
//--------------------------------------------------------------------------------------------------
//
struct RefreshEnumerator : FileEnumerator
{
	char curType[64];

	RefreshEnumerator (void)
	{
		U32 sel;
		SendDlgItemMessage(hMainWindow,IDC_LIST2,LB_RESETCONTENT,0,0);
		sel = SendDlgItemMessage(hMainWindow,IDC_LIST1,LB_GETCURSEL,0,0);
		curType[0] = 0;
		SendDlgItemMessage(hMainWindow,IDC_LIST1,LB_GETTEXT,sel,(LPARAM)curType);
	}

	virtual bool Callback (const WIN32_FIND_DATA & dataFound, const char * pathName, bool & bSkipDir)
	{
		const char * tmp;
		if ((tmp = strrchr(pathName, '\\')) != 0)
			tmp++;
		else
			tmp = pathName;
	
		if (strcmp(tmp,curType)==0)
		{
			SendDlgItemMessage(hMainWindow,IDC_LIST2,LB_ADDSTRING,0,(LPARAM)dataFound.cFileName);
		}
		else
			bSkipDir = true;

		return true;
	}
};//--------------------------------------------------------------------------------------------------
//
struct SpellingEnumerator : FileEnumerator
{
	SpellingEnumerator (void)
	{
	}

	virtual bool Callback (const WIN32_FIND_DATA & dataFound, const char * pathName, bool & bSkipDir)
	{
		if(strstr(pathName,"ALIAS")) 
			return true;

		if(stringSet)
			stringSet->AddString(dataFound.cFileName);

		return true;
	}
};
//--------------------------------------------------------------------------------------------------
//
void RefreshSpellingList (void)
{
	if (mainDoc && stringSet)
	{
		stringSet->Clear();
		SpellingEnumerator renum;
		EnumerateFiles(&renum);
	}
}
//--------------------------------------------------------------------------------------------------
//
void RefreshList (void)
{
	extern DbTreeView g_DbTreeView;
	g_DbTreeView.SavePlace();

	if (mainDoc)
	{
		RefreshEnumerator renum;
		EnumerateFiles(&renum);
	}
	RefreshSpellingList();

	g_DbTreeView.RestorePlace();
}

//--------------------------------------------------------------------------------------------------
//



BOOL32 SaveAs()
{
    U32 attrib;  
	char tempName[MAX_PATH];
	BOOL32 result=0;

	//if (currentFileName[0] != 0)
	//	strcpy(tempName,"");
	strcpy(tempName,currentFileName);
	if (mainDoc == 0)
	{
		MessageBox(hMainWindow,"You must open a file before saving","Save failed",MB_OK);
	}
	else
    while (1) 
	{
		result = DEFAULTS->GetOutputFilename(tempName,IDS_DB_FILTER);
	
		if (tempName[0] && result)
		{	
			if (strchr(tempName,'.') == NULL)
				strcat(tempName,".db");

			if ((attrib = GetFileAttributes(tempName)) & FILE_ATTRIBUTE_READONLY)
			{
				if (attrib != 0xFFFFFFFF)
				{
					MessageBox(hMainWindow,"This file is read-only","Read-only",MB_OK|MB_ICONSTOP);
					strcpy(tempName,currentFileName);
					result = 0;
				}
			}
		}
		else
		{
			result =0;
		//	MessageBox(0,"Save cancelled","",MB_OK);
			goto Done;
		}

	
			
		if (result)
		{
			RO = FALSE;
			strcpy(currentFileName,tempName);
			SaveDB();
			SetWindowText(GetDlgItem(hMainWindow,IDC_RO),"");
			return result;
		}
	}


Done:

	return result;
}

GENRESULT CopyOpenFile(IFileSystem *,char*);

BOOL32 SaveDB()
{
	U32 attrib;
	BOOL32 result = 0;
	//	if (DialogBoxParam(hInstance, MAKEINTRESOURCE(IDD_NEW_ARCHETYPE), 0, 
	//				(int (__stdcall *)(void)) FileDlgProc,(LPARAM) &dbName) == 0)
	if (currentFileName[0] == 0)
	{
		result = SaveAs();
	}
	else
	{
		if ((attrib = GetFileAttributes(currentFileName)) & FILE_ATTRIBUTE_READONLY)
		{
			if (attrib != 0xffffffff)
			{
				RO = TRUE;
				SetWindowText(GetDlgItem(hMainWindow,IDC_RO),"Read Only");
			}
		}
		else
		{
			RO = FALSE;
			SetWindowText(GetDlgItem(hMainWindow,IDC_RO),"");
		}
		if (!RO)
		{
			//mainDoc->Release();
			CopyOpenFile(mainDoc,currentFileName);
			//CopyFile("\\opendb.tmp",currentFileName,FALSE);
			//	CopyOpenFile(mainDoc,currentFileName);
			saved = TRUE;
		}
		else
		{
			if (MessageBox(hMainWindow,"This file is write protected. Would you like to do a 'Save As?'","Read-only",MB_YESNO | MB_APPLMODAL) == IDYES)
			{
				if (!SaveAs())
					goto Done;
			}
			else
			{
				MessageBox(hMainWindow,"Save cancelled","",MB_OK);
				goto Done;
			}
		}
		
		
		UpdateTitle();
		result = 1;
	}

Done:
	return result;
}					

void CreateMemoryFile (IFileSystem ** file)
{
	MEMFILEDESC mdesc = "temp.mem";

	mdesc.dwDesiredAccess = GENERIC_READ|GENERIC_WRITE;
	mdesc.dwCreationDistribution = CREATE_ALWAYS;

	CreateUTFMemoryFile(mdesc, file);
}

void NewDB()
{
	ProcessHeaders::SetPath();

	DAFILEDESC fdesc;
	COMPTR<IFileSystem> temp;
	U32 dwWritten;

	USER_DEFAULTS *iggy = DEFAULTS->GetDefaults();

	if (iggy->disableHeader)
	{
		MessageBox(hMainWindow,"External Header parsing is disabled. Unable to create a new database", "Feature Disabled", MB_OK);
		goto Done;
	}
	
	if (!ParseNewDefFile(iggy->defaultH))
	{
		MessageBox(hMainWindow,"Go into preferences and choose an h file and an include path","Invalid h file specified",MB_OK);
		goto Done;
	}

	CreateMemoryFile(temp);
	CreateDocument(temp,&mainDoc, CREATE_ALWAYS);
	CreateStringSet();

	fdesc.dwDesiredAccess = GENERIC_READ|GENERIC_WRITE;
	fdesc.dwShareMode = FILE_SHARE_READ;
	fdesc.dwCreationDistribution = CREATE_ALWAYS;

	fdesc.lpFileName = iggy->defaultH;
	fdesc.lpImplementation = "DOS";
	mainDoc->CreateDirectory(PARSEDIR);
	mainDoc->SetCurrentDirectory(PARSEDIR);
	mainDoc->CreateInstance(&fdesc,temp);
	temp->WriteFile(0, pPreprocessBlock, strlen(pPreprocessBlock), &dwWritten, 0);
	//file.free();
	mainDoc->SetCurrentDirectory("..");
	docManager->MakeConnection(mainDoc);

	RefreshList();

	saved = TRUE;

Done:
	currentFileName[0] = 0;
	UpdateTitle();
}

BOOL32 KillDB()
{
	BOOL32 result = 0;
	if (mainDoc)
	{
		if (!saved)
		{
			do
			{
				U32 answer = MessageBox(hMainWindow,"Save changes to current file?","File not saved",MB_YESNOCANCEL);
				if (answer == IDCANCEL)
					goto End;
				if (answer == IDYES)
				{
					if (SaveDB())
						break;
				}
				if (answer == IDNO)
					break;
			} while (1);
		}

		U32 i;
		for (i=0;i<NUM_VIEWERS;i++)
		{
			if (viewer[i])
			{
				viewer[i]->Release();
				viewer[i] = NULL;
			}

			if (docArray[i])
			{
				docArray[i]->CloseAllClients();
				docArray[i]->Release();
				docArray[i] = NULL;
			}
		}
		mainDoc->Release();
		mainDoc = 0;

		SendDlgItemMessage(hMainWindow,IDC_LIST1,LB_RESETCONTENT,0,0);
		SendDlgItemMessage(hMainWindow,IDC_LIST2,LB_RESETCONTENT,0,0);
	}

	NameTable::Free();

	if (hSymbols)
		PARSER->DestroySymbols(hSymbols);
	hSymbols = NULL;

	if (PARSER)
	{
		PARSER->Release();
		PARSER = 0;
	}

	SetupParser();

	saved = TRUE;
	UpdateTitle();
	result = 1;

End:
	return result;
}

void OpenDB(char *name)
{
	U32 attrib;

	if (name[0])				
	{
		if (OpenDatabase(name,NULL))
		{
			if ((attrib = GetFileAttributes(name)) & FILE_ATTRIBUTE_READONLY)
			{
				RO = TRUE;
				SetWindowText(GetDlgItem(hMainWindow,IDC_RO),"Read Only");
			}
			else
			{
				RO = FALSE;
				SetWindowText(GetDlgItem(hMainWindow,IDC_RO),"");
			}
		}
	
	}
}
//--------------------------------------------------------------------------------------------------
//
struct ConvertEnumerator : FileEnumerator
{
	HWND hwnd;
	BOOL32 somethingChanged;
	char lastPath[MAX_PATH];
	SYMBOL oldSymbol, newSymbol;
	BOOL32 bEqual;
	BOOL32 bVariableLength;

	ConvertEnumerator (HWND _hwnd) : hwnd(_hwnd)
	{
		somethingChanged = FALSE;
		lastPath[0] = 0;
		oldSymbol = newSymbol = 0;
	}

	virtual bool Callback (const WIN32_FIND_DATA & dataFound, const char * pathName, bool & bSkipDir)
	{
		COMPTR<IDocument> childDoc;//,newchildDoc;
		void *oldstruct,*newstruct;
		U32 dwRead,dwWritten;
		U32 size;
		
		if (lastPath[0]==0 || strcmp(lastPath, pathName) != 0)		// if new directory
		{
			const char * tmp;
			if ((tmp = strrchr(pathName, '\\')) != 0)
				tmp++;
			else
				tmp = pathName;
		
			if (tmp[0]==0 || tmp[1] != 'T' || tmp[2] != '_')		// look for "*T_" pattern
			{
				bSkipDir = true;
				return true;
			}

			oldSymbol = PARSER->GetSymbol(hSymbols, tmp);
			newSymbol = PARSER->GetSymbol(0, tmp);
			strcpy(lastPath, pathName);
			mainDoc->SetCurrentDirectory(pathName);
							
			if (newSymbol)
			{
				//
				// has data definition changed since last time?
				//
				if (!oldSymbol)
				{
					char buffer[64];
					sprintf(buffer,"Database out of synch - %s",tmp);
					MessageBox(hMainWindow,buffer,"Warning",MB_OK);
					oldSymbol = newSymbol;
				}

				if ((bVariableLength = PARSER->HasVariableSize(newSymbol)) != 0)		// don't transmorgriphy variable data
					bEqual = 1;
				else
				if ((bEqual = PARSER->IsEqual(oldSymbol, newSymbol)) == 0)
				{
					somethingChanged = TRUE;
					SendDlgItemMessage(hwnd,IDC_LIST1,LB_ADDSTRING,0,(LPARAM)tmp);
				}
			}
			else
			{
				char buffer[64];
				strcpy(buffer,tmp);
				strcat(buffer," (deleted)");
				somethingChanged = TRUE;
				SendDlgItemMessage(hwnd,IDC_LIST1,LB_ADDSTRING,0,(LPARAM)buffer);
				recursiveDelete(mainDoc, pathName);
				return true;
			}
		}
		//
		// now process the file
		//
		NameTable::New(dataFound.cFileName);

		//read old size and setup old doc
		mainDoc->GetChildDocument(dataFound.cFileName,childDoc);
		size = childDoc->GetFileSize();
		oldstruct = calloc(__max(size, PARSER->GetTypeSize(newSymbol)),1);
		childDoc->ReadFile(0, oldstruct, size, &dwRead,0);
		
		if (bVariableLength==0)
			size = PARSER->GetTypeSize(newSymbol);
		newstruct = size ? malloc(size) : 0;

		if (oldstruct)
		{
			if (bEqual)
				memcpy(newstruct, oldstruct, size);
			else
				PARSER->CorrelateSymbol(oldSymbol, oldstruct, newSymbol, newstruct);
		}
		
		childDoc->SetFilePointer(0,0);
		childDoc->SetEndOfFile();
		childDoc->WriteFile(0, newstruct, size, &dwWritten, 0);
		
		free(oldstruct);
		free(newstruct);
		oldstruct = newstruct = 0;

		return true;
	}
};
//--------------------------------------------------------------------------------------------------
//
BOOL32 Convert (HWND hwnd)
{
	ConvertEnumerator cenum = hwnd;
	EnumerateFiles(&cenum);

	return !cenum.somethingChanged;
}
//--------------------------------------------------------------------------
//
U32 CountNumFilesInMainDoc (void)
{
	HANDLE handle,handle2;
	WIN32_FIND_DATA data,data2;
	COMPTR<IDocument> childDoc;//,newchildDoc;
	U32 result = 0;
		
	handle = mainDoc->FindFirstFile("*.*",&data);
	
	// check first file
	if (handle != INVALID_HANDLE_VALUE )
	{
		do 
		{
			if (!(strstr(data.cFileName,PARSEDIR)))
			{
				mainDoc->SetCurrentDirectory(data.cFileName);
					
				handle2 = mainDoc->FindFirstFile("*.*",&data2);
					
				if (handle2 != INVALID_HANDLE_VALUE)
				{
					do
					{
						result++;
					}
					while (mainDoc->FindNextFile(handle2,&data2));
				}
				
				mainDoc->FindClose(handle2);
				mainDoc->SetCurrentDirectory("\\");
			}
		}
		while (mainDoc->FindNextFile(handle,&data));
	}

	mainDoc->FindClose(handle);

	return result;
}
//--------------------------------------------------------------------------
//
struct TextWritingStruct
{
	HANDLE handle,handle2;
	WIN32_FIND_DATA data,data2;
	COMPTR<IDocument> childDoc;//,newchildDoc;
	COMPTR<IViewer> viewer;
	VIEWDESC vdesc;
//	DAComponent<StructEnumerator> structEnumerator;
	DAComponent<TableBuilder> structEnumerator;
	U32 numFiles, numFilesProcessed;

	TextWritingStruct (void)
	{
		vdesc.className = data.cFileName;
		handle = handle2 = INVALID_HANDLE_VALUE;
		numFiles = numFilesProcessed = 0;
	}

	void update (HWND hwnd);
};
//--------------------------------------------------------------------------
//
void TextWritingStruct::update (HWND hwnd)
{
	if (handle2 == INVALID_HANDLE_VALUE)
	{
		if (handle == INVALID_HANDLE_VALUE)
		{
			handle = mainDoc->FindFirstFile("*.*",&data);
			if (handle == INVALID_HANDLE_VALUE)
			{
				MessageBox(hwnd,"File find error","WriteToText Error",MB_OK);
				goto Done;
			}
		}

		if (strstr(data.cFileName,PARSEDIR))
		{
			if (mainDoc->FindNextFile(handle,&data) == 0)
			{
				mainDoc->FindClose(handle);
				handle = INVALID_HANDLE_VALUE;
			}
			goto Done;		// skip this one
		}

//		structEnumerator.write_string(data.cFileName);
//		structEnumerator.newline();

		mainDoc->SetCurrentDirectory(data.cFileName);
		handle2 = mainDoc->FindFirstFile("*.*",&data2);
		if (handle2 == INVALID_HANDLE_VALUE)
		{
			mainDoc->FindClose(handle);
			handle = INVALID_HANDLE_VALUE;
			goto Done;
		}
		//
		// create the viewer here, instead of inside the loop
		//
		mainDoc->GetChildDocument(data2.cFileName,childDoc);
		vdesc.doc = childDoc;
		if (PARSER->CreateInstance(&vdesc, viewer) != GR_OK)
		{
			MessageBox(hwnd,"View creation error","WriteToText Error",MB_OK);
			mainDoc->FindClose(handle2);
			mainDoc->FindClose(handle);
			handle = INVALID_HANDLE_VALUE;
			goto Done;
		}
	}

	mainDoc->GetChildDocument(data2.cFileName,childDoc);

	if (viewer->Invoke("Update", (IDocument *)childDoc) != GR_OK)
	{
		MessageBox(hwnd,"Viewer::Update","WriteToText Error",MB_OK);
		mainDoc->FindClose(handle2);
		mainDoc->FindClose(handle);
		handle = INVALID_HANDLE_VALUE;
		goto Done;
	}
	viewer->set_instance_name(data2.cFileName);

	if (viewer->Invoke("Enumerate", &structEnumerator) != GR_OK)
	{
		MessageBox(hwnd,"Viewer::Enumerate","WriteToText Error",MB_OK);
		mainDoc->FindClose(handle2);
		mainDoc->FindClose(handle);
		handle = INVALID_HANDLE_VALUE;
		goto Done;
	}

//	structEnumerator.blastToFile();
//	structEnumerator.newline();

	if (mainDoc->FindNextFile(handle2,&data2) == 0)
	{
	 	mainDoc->FindClose(handle2);
		handle2 = INVALID_HANDLE_VALUE;
	 	mainDoc->SetCurrentDirectory("\\");
//		structEnumerator.newline();
		if (mainDoc->FindNextFile(handle,&data) == 0)
		{
			mainDoc->FindClose(handle);
			handle = INVALID_HANDLE_VALUE;
			structEnumerator.blastToFile();
		}
	}

Done:
	return;
}
//--------------------------------------------------------------------------
//
BOOL __stdcall WriteToTextDlgProc (HWND hwnd, UINT message, UINT wParam, LPARAM lParam)
{
	TextWritingStruct * ts = (TextWritingStruct *) GetWindowLong(hwnd, DWL_USER);

	switch (message)
	{
	case WM_INITDIALOG:
		{
			HWND hItem;
			char buffer1[128];
			char buffer2[128];
		
			ts = (TextWritingStruct *) lParam;
			SetWindowLong(hwnd, DWL_USER, lParam);
			ts->structEnumerator.outFile->GetFileName(buffer1, sizeof(buffer1));
			wsprintf(buffer2, "Writing to %s...", buffer1);
			SetDlgItemText(hwnd, IDC_STATIC1, buffer2);
			hItem = GetDlgItem(hwnd, IDC_PROGRESS1);
			SendMessage(hItem, PBM_SETRANGE, 0, MAKELPARAM(0, ts->numFiles));
			SendMessage(hItem, PBM_SETSTEP, 1, 0);
			PostMessage(hwnd, WM_USER+1, 0, 0);
			return 1;
		}
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDCANCEL:
			EndDialog(hwnd, 0);
			break;
		}
		break;

	case WM_USER+1:
		ts->update(hwnd);
		if (ts->handle == INVALID_HANDLE_VALUE)
			EndDialog(hwnd, 1);
		else
			SendMessage(GetDlgItem(hwnd, IDC_PROGRESS1), PBM_STEPIT, 0, 0);
		return 1;
	}

	return 0;
}
//--------------------------------------------------------------------------
//
BOOL32 WriteToText (HWND hwnd, const char * fileName)
{
	BOOL32 result=0;
	char buffer[MAX_PATH+4];
  	DAFILEDESC fdesc = buffer;
	TextWritingStruct tws;

	fdesc.dwDesiredAccess = GENERIC_READ |GENERIC_WRITE;
	fdesc.dwShareMode = 0;
	fdesc.dwCreationDistribution = CREATE_ALWAYS;

	{
		char *ptr;
		strcpy(buffer, fileName);
		if ((ptr = strrchr(buffer, '.')) != 0)
		{	
			ptr++;
			if (stricmp(ptr, "txt") == 0)
			{
				MessageBox(hMainWindow,"Database file already has txt extention.","WriteToText Error",MB_OK);
				goto Done;
			}
			strcpy(ptr, "txt");
		}
		else
			strcat(buffer, ".txt");
	}

	if (DACOM->CreateInstance(&fdesc, tws.structEnumerator.outFile)!=GR_OK)
	{
		MessageBox(hMainWindow,"Unable to open output file.","WriteToText Error",MB_OK);
		goto Done;
	}

	tws.numFiles = CountNumFilesInMainDoc();

	DialogBoxParam(hInstance, MAKEINTRESOURCE(IDD_PROGBAR), hwnd, WriteToTextDlgProc, (LONG) &tws);

/*
	do
	{
		tws.update(hwnd);
	} while (tws.handle != INVALID_HANDLE_VALUE);
*/
	result = 1;
Done:
	return result;
}
//--------------------------------------------------------------------------
//
GENRESULT CreateDocument(IFileSystem *memfile,IDocument **doc, DWORD dwCreation)
{
	GENRESULT result;

	DOCDESC ddesc = "dummy";//memfile->fileName;
	IDocument * utf;
	
	ddesc.lpImplementation = "UTF";
	ddesc.lpParent = memfile;
	
	ddesc.dwDesiredAccess  = GENERIC_WRITE | GENERIC_READ;
	ddesc.dwCreationDistribution = dwCreation;

	memfile->AddRef();
	
	if ((result = DACOM->CreateInstance(&ddesc, (void **) &utf)) == GR_OK)
	{
		*doc = utf;
	}
	else
	{
		memfile->Release();
	}

	return result;
}
//--------------------------------------------------------------------------
//
GENRESULT CreateStringSet()
{
	if(stringSet)
		stringSet->Release();
	stringSet = NULL;
	STRINGSETDESC desc;
	GENRESULT retVal = DACOM->CreateInstance(&desc, (void **) &stringSet);
	if(stringSet)
	{
		stringSet->SetCallBack(&finder);
		stringSet->AddRef();
	}
	return retVal;
}
//--------------------------------------------------------------------------
//
GENRESULT CopyOpenFile(IFileSystem *file,char *fileName)
{
	GENRESULT result;
	COMPTR<IFileSystem> outFile, temp;
	DAFILEDESC fdesc = fileName;
	U32 dwSize = file->GetFileSize();
	U32 dwRead,dwWritten;
	void *buffer = malloc(dwSize);
	MEMFILEDESC mdesc = fileName;

	mdesc.lpBuffer = buffer;
	mdesc.dwBufferSize = dwSize;
	mdesc.dwFlags = CMF_DONT_COPY_MEMORY;
	mdesc.dwDesiredAccess = GENERIC_READ |GENERIC_WRITE;
	mdesc.dwShareMode = 0;
	mdesc.dwCreationDistribution = OPEN_EXISTING;

	file->SetFilePointer(0,0);
	file->ReadFile(0,buffer,dwSize,&dwRead,0);

	if ((result = CreateUTFMemoryFile(mdesc, temp)) != GR_OK)
		return result;

	temp.free();		// clears all of the sharing flags

	fdesc.lpImplementation = "DOS";
	fdesc.dwDesiredAccess = GENERIC_READ |GENERIC_WRITE;
	fdesc.dwShareMode = 0;
	fdesc.dwCreationDistribution = CREATE_ALWAYS;

	if ((result = DACOM->CreateInstance(&fdesc,outFile)) == GR_OK)
	{
		outFile->WriteFile(0,buffer,dwRead,&dwWritten,0);
	}

	free(buffer);

	return result;
}
//--------------------------------------------------------------------------
//
BOOL32 OpenDatabase(char *oldDB,char *oldH)
{
	COMPTR<IFileSystem> temp, temp2;
	DAFILEDESC fdesc = oldDB;
	U32 dwWritten,dwRead;
	BOOL32 result = FALSE;
	USER_DEFAULTS *iggy = DEFAULTS->GetDefaults();
	
	CreateMemoryFile(temp);

	if (DACOM->CreateInstance(&fdesc, temp2) != GR_OK)
	{
		//this means that we are trying an autoload, but someone deleted the file
		KillDB();
		NewDB();
		goto Done;
	}

	// do file copy
	{
		DWORD dwSize = temp2->GetFileSize();
		char * buffer = (char *) malloc(dwSize);

		temp2->ReadFile(0, buffer, dwSize, &dwRead, 0);
		temp->WriteFile(0, buffer, dwRead, &dwWritten, 0);
		free(buffer);
		temp->SetFilePointer(0,0);
	}
	
	CreateDocument(temp,&mainDoc);
	CreateStringSet();

	if (mainDoc->SetCurrentDirectory(PARSEDIR))
	{
		// retrieve old database h file
		WIN32_FIND_DATA data;
		HANDLE handle;
		char * savedParsedData;

		if ((handle = mainDoc->FindFirstFile("*.*", &data)) == INVALID_HANDLE_VALUE)
		{
			MessageBox(hMainWindow,"Couldn't read old H file","File error",MB_OK);
			goto Done;
		}
		mainDoc->FindClose(handle);
		handle = INVALID_HANDLE_VALUE;
		
		//Get the corresponding h file for this database
		if (ParseNewDefFile(data.cFileName) == 0)
		{
			MessageBox(hMainWindow,"Go into preferences and choose an h file and an include path",
			"Invalid h file specified",MB_OK);
			KillDB();
			goto Done;
		}

		// save new parsed data
		savedParsedData = pPreprocessBlock;
		
		// read old data
		fdesc.lpFileName = data.cFileName;
		fdesc.lpImplementation = "DOS";
		fdesc.dwDesiredAccess = GENERIC_READ |GENERIC_WRITE;
		fdesc.dwCreationDistribution = OPEN_ALWAYS;
		
		if (mainDoc->CreateInstance(&fdesc,temp) != GR_OK)
		{
			MessageBox(hMainWindow,"Couldn't create header",data.cFileName,MB_OK);
			goto Done;
		}
		
		if ((dwWritten = temp->GetFileSize()) == -1)
		{
			MessageBox(hMainWindow,"Couldn't get file size","Import Failed",MB_OK);
			goto Done;
		}
		pPreprocessBlock = (char *)malloc(dwWritten+1);
		if (temp->ReadFile(0,pPreprocessBlock,dwWritten, &dwRead, 0) == 0)
		{
			MessageBox(hMainWindow,"Failed to read parse data","Import Failed",MB_OK);
			goto Done;
		}
		pPreprocessBlock[dwWritten] = 0;		// make sure block is NULL-terminated
		if (iggy->disableHeader)
		{
			PARSER->ParseMemory(pPreprocessBlock);		// parse this twice so that everything works correctly
			EnumTypes();
		}
		hSymbols = PARSER->ParseNewMemory(pPreprocessBlock);
		mainDoc->SetCurrentDirectory("..");

		//
		// write new data
		//

		if (savedParsedData)
		{
			temp->SetFilePointer(0,0);
			temp->SetEndOfFile();
			temp->WriteFile(0, savedParsedData, strlen(savedParsedData), &dwWritten, 0);
			free(savedParsedData);
			savedParsedData = 0;
		}
		
		free(pPreprocessBlock);
		pPreprocessBlock = 0;
	}
	else
	{
		char oldBuf[MAX_PATH];

		if (oldH == 0 || oldH[0] == 0)
		{
			strcpy(oldBuf,"GameTypes.h");
		}
		else
			strcpy(oldBuf,oldH);

		if (pPreprocessBlock)
		{
			free(pPreprocessBlock);
			pPreprocessBlock = 0;
		}

		if (ParseDefFile(oldBuf) == 0)
		{
			MessageBox(hMainWindow,"Couldn't read old H file","File error",MB_OK);
			goto Done;
		}
	}

	
	if (!ConvertDB())
	{
		MessageBox(hMainWindow,"Load Cancelled","User Cancel",MB_OK);
		goto Done;
	}

	RefreshList();

	if (hSymbols)
		PARSER->DestroySymbols(hSymbols);
	hSymbols = NULL;

	strcpy(currentFileName,oldDB);
	UpdateTitle();

	docManager->MakeConnection(mainDoc);

	result = TRUE;

Done:
	return result;
}
//--------------------------------------------------------------------------
//
BOOL32 OpenDatabaseViaDir(char* dir)
{
	BOOL32 result = FALSE;

	// need to verifiy that this is a valid directory!
	char gametypespath[MAX_PATH];
	strcpy(gametypespath,dir);
	strcat(gametypespath,"\\ParseData\\GameTypes.h");
	if( ::GetFileAttributes(gametypespath) == 0xFFFFFFFF )
	{
		::MessageBox(hMainWindow,"Could not find GameTypes.h in directory!", dir, MB_OK );
		return 0;
	}

	COMPTR<IDocument> newDocument;
	DOCDESC ddesc = dir;
	ddesc.lpImplementation = "DOS";
	ddesc.memory = 0;
	ddesc.memoryLength = 0;
	ddesc.dwDesiredAccess        = GENERIC_WRITE | GENERIC_READ;
	ddesc.dwCreationDistribution = OPEN_EXISTING;
	ddesc.dwFlagsAndAttributes   = FILE_ATTRIBUTE_DIRECTORY; //FILE_ATTRIBUTE_NORMAL;

	if (DACOM->CreateInstance(&ddesc, newDocument) != GR_OK)
	{
		// whoops! could not create mainDoc using this directory!
		KillDB();
		NewDB();
		goto Done;
	}

	// do copy
	mainDoc = newDocument;
	mainDoc->AddRef();
	
	CreateStringSet();

//	if (mainDoc->SetCurrentDirectory(PARSEDIR))
//	{
//		// retrieve old database h file
//		WIN32_FIND_DATA data;
//		HANDLE handle;
//		char * savedParsedData;
//
//		if ((handle = mainDoc->FindFirstFile("*.h", &data)) == INVALID_HANDLE_VALUE)
//		{
//			MessageBox(hMainWindow,"Couldn't read old H file","File error",MB_OK);
//			goto Done;
//		}
//		mainDoc->FindClose(handle);
//		handle = INVALID_HANDLE_VALUE;
//		
//		// Get the corresponding h file for this database
//		if (ParseNewDefFile(data.cFileName) == 0)
//		{
//			MessageBox(hMainWindow,"Go into preferences and choose an h file and an include path","Invalid h file specified",MB_OK);
//			KillDB();
//			goto Done;
//		}
//
//		// save new parsed data
//		savedParsedData = pPreprocessBlock;
//		
//		// read old data
//		DAFILEDESC fdesc;
//		fdesc.lpFileName = data.cFileName;
//		fdesc.lpImplementation = "DOS";
//		fdesc.dwDesiredAccess = GENERIC_READ | GENERIC_WRITE;
//		fdesc.dwCreationDistribution = OPEN_ALWAYS;
//		
//		COMPTR<IFileSystem> oldData;
//		if (mainDoc->CreateInstance(&fdesc,oldData) != GR_OK)
//		{
//			MessageBox(hMainWindow,"Couldn't create header",data.cFileName,MB_OK);
//			goto Done;
//		}
//		
//		DWORD dwWritten, dwRead;
//		if ((dwWritten = oldData->GetFileSize()) == -1)
//		{
//			MessageBox(hMainWindow,"Couldn't get file size","Import Failed",MB_OK);
//			goto Done;
//		}
//		pPreprocessBlock = (char *)malloc(dwWritten+1);
//		if (oldData->ReadFile(0,pPreprocessBlock,dwWritten, &dwRead, 0) == 0)
//		{
//			MessageBox(hMainWindow,"Failed to read parse data","Import Failed",MB_OK);
//			goto Done;
//		}
//		pPreprocessBlock[dwWritten] = 0;		// make sure block is NULL-terminated
//		hSymbols = PARSER->ParseNewMemory(pPreprocessBlock);
//		mainDoc->SetCurrentDirectory("..");
//		EnumTypes();
//
//		free(pPreprocessBlock);
//		pPreprocessBlock = 0;
//	}
	
	if (!ConvertDB())
	{
		MessageBox(hMainWindow,"Load Cancelled","User Cancel",MB_OK);
		goto Done;
	}

	RefreshList();

	if (hSymbols)
		PARSER->DestroySymbols(hSymbols);
	hSymbols = NULL;

	strcpy(currentFileName,dir);
	UpdateTitle();

	docManager->MakeConnection(mainDoc);

	result = TRUE;

Done:
	return result;
}
/*
//--------------------------------------------------------------------------
//
BOOL32 OpenDatabaseViaDir(char* dir)
{
	COMPTR<IFileSystem> newFileSystem;
	BOOL32 result = FALSE;
	USER_DEFAULTS *iggy = DEFAULTS->GetDefaults();

	// open up the directory as the new FileSystem
	DAFILEDESC fdesc = dir;
	if( DACOM->CreateInstance( &fdesc, (void**)&newFileSystem ) == GR_OK )
	{
		// create new main document

		KillDB();

		DOCDESC ddesc = dir;
		IDocument * dos;

		ddesc.lpFileName             = dir;
		ddesc.lpImplementation       = "DOS";
//		ddesc.lpParent               = newFileSystem;
//		ddesc.dwDesiredAccess        = GENERIC_WRITE | GENERIC_READ;
//		ddesc.dwCreationDistribution = OPEN_EXISTING;
//		ddesc.dwFlagsAndAttributes   = FILE_ATTRIBUTE_DIRECTORY; //FILE_ATTRIBUTE_NORMAL;
		ddesc.memory = NULL;
		ddesc.memoryLength = 0;

		newFileSystem->AddRef();
		
		if( DACOM->CreateInstance(&ddesc, (void **) &dos) == GR_OK )
		{
			mainDoc = dos;
		}
		else
		{
			newFileSystem->Release();
			NewDB();
			goto Done;
		}
	}
	else
	{
		KillDB();
		NewDB();
		goto Done;
	}

	CreateStringSet();

	if (mainDoc->SetCurrentDirectory(PARSEDIR))
	{
		U32 dwWritten,dwRead;

		// retrieve old database h file
		WIN32_FIND_DATA data;
		HANDLE handle;
		char * savedParsedData; 

		if ((handle = mainDoc->FindFirstFile("*.h", &data)) == INVALID_HANDLE_VALUE)
		{
			MessageBox(hMainWindow,"Couldn't read old H file","File error",MB_OK);
			goto Done;
		}
		mainDoc->FindClose(handle);
		handle = INVALID_HANDLE_VALUE;


		// Get the corresponding h file for this database from the c:/project/shared/dinclude directory
		if( ParseNewDefFile(data.cFileName) == 0 )
		{
			MessageBox(hMainWindow,"Go into preferences and choose an h file and an include path","Invalid h file specified",MB_OK);
			KillDB();
			goto Done;
		}
		
		// Get the corresponding h file for this database from the c:/project/shared/dinclude directory
//		hSymbols = ParseNewDefFile(data.cFileName);
//		if( hSymbols == 0 )
//		{
//			MessageBox(hMainWindow,"Go into preferences and choose an h file and an include path","Invalid h file specified",MB_OK);
//			KillDB();
//			goto Done;
//		}

mainDoc->SetCurrentDirectory("\\");

EnumTypes();

RefreshList();

if (hSymbols)
	PARSER->DestroySymbols(hSymbols);
hSymbols = NULL;

strcpy(currentFileName,dir);
UpdateTitle();

docManager->MakeConnection(mainDoc);

result = TRUE;

return 1;

		// save new parsed data
		savedParsedData = pPreprocessBlock;
		
//		Any combination of the following flags is acceptable:
//
//		FILE_FLAG_WRITE_THROUGH 0x80000000     Instructs the operating system to write through any intermediate cache and go directly to the file. The operating system can still cache write operations, but cannot lazily flush them. 
//		FILE_FLAG_NO_BUFFERING 0x20000000      Requests the server to open the file with no intermediate buffering or caching; the server is not obliged to honor the request. An application must meet certain requirements when working with files opened with FILE_FLAG_NO_BUFFERING. File access must begin at offsets within the file that are integer multiples of the volume's sector size; and must be for numbers of bytes that are integer multiples of the volume's sector size. For example, if the sector size is 512 bytes, an application can request reads and writes of 512, 1024, or 2048 bytes, but not of 335, 981, or 7171 bytes.  
//		FILE_FLAG_RANDOM_ACCESS 0x10000000     Indicates that the application intends to access the file randomly. The server MAY use this flag to optimize file caching. 
//		FILE_FLAG_SEQUENTIAL_SCAN 0x08000000   Indicates that the file is to be accessed sequentially from beginning to end. Windows uses this flag to optimize file caching. If an application moves the file pointer for random access, optimum caching may not occur; however, correct operation is still guaranteed. Specifying this flag can increase performance for applications that read large files using sequential access. Performance gains can be even more noticeable for applications that read large files mostly sequentially, but occasionally skip over small ranges of bytes. 
//		FILE_FLAG_DELETE_ON_CLOSE 0x04000000   Requests that the server is delete the file immediately after all of its handles have been closed. 
//		FILE_FLAG_BACKUP_SEMANTICS 0x02000000  Indicates that the file is being opened or created for a backup or restore operation. The server SHOULD allow the client to override normal file security checks, provided it has the necessary permission to do so. 
//		FILE_FLAG_POSIX_SEMANTICS 0x01000000   Indicates that the file is to be accessed according to POSIX rules. This includes allowing multiple files with names differing only in case, for file systems that support such naming. (Use care when using this option because files created with this flag may not be accessible by applications written for MS-DOS, Windows 3.x, or Windows NT.) 

		// read old data
		COMPTR<IFileSystem> fsGameTypes;
		fdesc.lpFileName = data.cFileName;
		fdesc.lpImplementation = "DOS";
//		fdesc.dwDesiredAccess = GENERIC_READ; // | GENERIC_WRITE;
//		fdesc.dwCreationDistribution = OPEN_EXISTING; // OPEN_ALWAYS;
//		fdesc.dwShareMode = 0;
//		fdesc.dwFlagsAndAttributes = FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN;
//		fdesc.dwFlagsAndAttributes = FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS;
//		fdesc.dwFlagsAndAttributes = 0; // FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS | FILE_FLAG_BACKUP_SEMANTICS;
		
		if (mainDoc->CreateInstance(&fdesc,fsGameTypes) != GR_OK)
		{
			MessageBox(hMainWindow,"Couldn't create header",data.cFileName,MB_OK);
			goto Done;
		}
		if ((dwWritten = fsGameTypes->GetFileSize()) == -1)
		{
			MessageBox(hMainWindow,"Couldn't get file size","Import Failed",MB_OK);
			goto Done;
		}
		pPreprocessBlock = (char *)malloc(dwWritten+1);
		if( fsGameTypes->ReadFile(0,pPreprocessBlock,dwWritten,&dwRead,0) == 0 )
		{
			MessageBox(hMainWindow,"Failed to read parse data","Import Failed",MB_OK);
			goto Done;
		}
		pPreprocessBlock[dwWritten] = 0;		// make sure block is NULL-terminated
//		if (iggy->disableHeader)
//		{
//			PARSER->ParseMemory(pPreprocessBlock);		// parse this twice so that everything works correctly
//			EnumTypes();
//		}
		hSymbols = PARSER->ParseNewMemory(pPreprocessBlock);
		EnumTypes();
		mainDoc->SetCurrentDirectory("\\");

		//
		// write new data
		//
//		if (savedParsedData)
//		{
//			fsGameTypes->SetFilePointer(0,0);
//			fsGameTypes->SetEndOfFile();
//			fsGameTypes->WriteFile(0, savedParsedData, strlen(savedParsedData), &dwWritten, 0);
//			free(savedParsedData);
//			savedParsedData = 0;
//		}
		
		free(pPreprocessBlock);
		pPreprocessBlock = 0;
	}
	
	if (!ConvertDB())
	{
		MessageBox(hMainWindow,"Load Cancelled","User Cancel",MB_OK);
		goto Done;
	}

	RefreshList();

	if (hSymbols)
		PARSER->DestroySymbols(hSymbols);
	hSymbols = NULL;

	strcpy(currentFileName,dir);
	UpdateTitle();

	docManager->MakeConnection(mainDoc);

	result = TRUE;

Done:
	return result;
}
*/
//--------------------------------------------------------------------------
//
BOOL WINAPI ConvertDlgProc (HWND hwnd, UINT message, UINT wParam, LPARAM lParam)
{
	//char db_name[MAX_PATH],h_name[MAX_PATH];

	switch (message)
	{
	case WM_INITDIALOG:
		//SetFocus(GetDlgItem(hwnd,IDC_IMPORTDB));
		if (Convert(hwnd))
			EndDialog(hwnd,1);
		break;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDYES:
		//	GetDlgItemText(hwnd,IDC_IMPORTDB,db_name,MAX_PATH);
		//	GetDlgItemText(hwnd,IDC_IMPORTH,h_name,MAX_PATH);
		//	OpenDatabase(db_name,h_name);
			saved = FALSE;
			EndDialog(hwnd,1);
				
			break;
		case IDNO:
			EndDialog(hwnd,0);
			break;
		}
		break;
	}

	return 0;
}		

BOOL32 ConvertDB()
{
	BOOL32 result = FALSE;
	
	if (DialogBoxParam(hInstance, MAKEINTRESOURCE(IDD_CONVERT), hMainWindow, 
						ConvertDlgProc,0) == 0)
		goto Done;



	result = TRUE;

Done:

	return result;
}

BOOL Image(IFileSystem * file,IFileSystem * newFile)
{
	WIN32_FIND_DATA data;
	HANDLE handle;
	int result=0;
//	U32 dwWritten,dwRead;


	if ((handle = file->FindFirstFile("*.*", &data)) == INVALID_HANDLE_VALUE)
		return 1;	// directory is empty ?

	do
	{
		// make sure this not a silly "." entry
		if (data.cFileName[0] != '.' || strchr(data.cFileName, '\\') != 0)
		{
			result++;

			//
			// output of the program is identical whether you traverse by changing directory
			// or by creating a new instance of IFileSystem each time
			//
			if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				// traverse subdirectory
				if (file->SetCurrentDirectory(data.cFileName))
				{
					newFile->CreateDirectory(data.cFileName);
					newFile->SetCurrentDirectory(data.cFileName);
					result = Image(file,newFile);//,data.cFileName);
					file->SetCurrentDirectory("..");	// restore current directory
					newFile->SetCurrentDirectory("..");
				}
			}
			else 
			{	
	
				DOCDESC fdesc;
				COMPTR<IDocument> newDoc;
				
				fdesc = data.cFileName;
				fdesc.memory = NULL;
				fdesc.lpImplementation = "DOS";
				fdesc.dwDesiredAccess = GENERIC_READ|GENERIC_WRITE;
				fdesc.dwShareMode = 0;  // no sharing
				fdesc.dwCreationDistribution = CREATE_ALWAYS;
				newFile->CreateInstance(&fdesc, newDoc);
			}
		}

	} while (file->FindNextFile(handle, &data));

	file->FindClose(handle);

	return result;

}

BOOL RecursiveCopy(IFileSystem * file,IFileSystem * newFile)//,char *baseName)
{
	WIN32_FIND_DATA data;
	HANDLE handle;
	int result=0;
	U32 dwWritten,dwRead;
	void *structure;


	if ((handle = file->FindFirstFile("*.*", &data)) == INVALID_HANDLE_VALUE)
		return 1;	// directory is empty ?

	do
	{
		// make sure this not a silly "." entry
		if (data.cFileName[0] != '.' || strchr(data.cFileName, '\\') != 0)
		{
			result++;

			//
			// output of the program is identical whether you traverse by changing directory
			// or by creating a new instance of IFileSystem each time
			//
			if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				// traverse subdirectory
				if (file->SetCurrentDirectory(data.cFileName))
				{
					newFile->CreateDirectory(data.cFileName);
					newFile->SetCurrentDirectory(data.cFileName);
					result = RecursiveCopy(file,newFile);//,data.cFileName);
					file->SetCurrentDirectory("..");	// restore current directory
					newFile->SetCurrentDirectory("..");
				}
			}
			else 
			{	
				structure = calloc((data.nFileSizeHigh * MAXDWORD) + data.nFileSizeLow,1);
	
				DOCDESC fdesc;
				IDocument *newDoc;
				
				//get at the desired file
				fdesc = data.cFileName;
				fdesc.memory = NULL;
				fdesc.lpImplementation = "DOS";
				fdesc.dwDesiredAccess = GENERIC_READ;
				fdesc.dwShareMode = FILE_SHARE_READ | FILE_SHARE_WRITE; 
				file->CreateInstance(&fdesc, (void **)&newDoc);

				U32 Rick = file->GetLastError();
				
				newDoc->SetFilePointer(0,0);
				newDoc->ReadFile(0,structure,(data.nFileSizeHigh * MAXDWORD) + data.nFileSizeLow,&dwRead,0);
				newDoc->Release();
		
				fdesc = data.cFileName;
				fdesc.memory = NULL;
				fdesc.lpImplementation = "DOS";
				fdesc.dwDesiredAccess = GENERIC_READ|GENERIC_WRITE;
				fdesc.dwShareMode = 0;  // no sharing
				fdesc.dwCreationDistribution = CREATE_ALWAYS;
				newFile->CreateInstance(&fdesc, (void **)&newDoc);
		
				newDoc->WriteFile(0, structure, (data.nFileSizeHigh * MAXDWORD) + data.nFileSizeLow, &dwWritten, 0);
				newDoc->Release();
			}
		}

	} while (file->FindNextFile(handle, &data));

	file->FindClose(handle);

	return result;

}

void UpdateTitle()
{
	char buffer[256];

	if (currentFileName[0] == 0)
		sprintf(buffer,"Database Editor - Untitled");
	else
		sprintf(buffer,"Database Editor - %s",currentFileName);
	if (!saved)
		strcat(buffer," *");
	SetWindowText(hMainWindow,buffer);
}

/*void CopyOpenFile(IFileSystem * file,char *newFileName)
{
	COMPTR<IFileSystem> newFile;

	DOCDESC ddesc;
	
	ddesc.memory = NULL;
	ddesc.lpFileName = newFileName;
	ddesc.lpImplementation = "UTF";
	ddesc.dwDesiredAccess = GENERIC_READ |GENERIC_WRITE;
	ddesc.dwShareMode = FILE_SHARE_READ;
	ddesc.dwCreationDistribution = CREATE_ALWAYS;

	if (DACOM->CreateInstance(&ddesc, (void **)&newFile) != GR_OK)
	{
		MessageBox(0,"Save failed","Didn't take",MB_OK);
	}

	RecursiveCopy(file,newFile);
	
}*/
