//--------------------------------------------------------------------------//
//                                                                          //
//                              TreeView.cpp                                //
//                                                                          //
//               COPYRIGHT (C) 1997 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*

   $Header: /Libs/dev/Src/Tools/TreeView/TreeView.cpp 3     3/20/00 4:52p Pbleisch $


	Traverses a file system, printing a treeview to stdout.
	This program makes these assumptions:
		The inputed name is a file system. (eg. IFF, UTF, or DOS directory)
		The file system implementation is assumed to be the filename's extention.







*/

//--------------------------------------------------------------------------//

#include <windows.h>

#include "DACOM.h"
#include "FileSys.h"
//#include "HeapObj.h"

//--------------------------------------------------------------------------//

char szBanner[] = "TreeView\r\n";
char szUsage[]  = "TreeView { filename | directory } \r\n";

ICOManager *DACOM=0;

#define INDENTION 4

#define DO_INDENTION(iIndent)							\
			{											\
				int i=0,j;								\
				while (i < iIndent)						\
				{										\
					_localprintf("|");						\
					for (j = 1; j < INDENTION; j++)		\
						_localprintf("-");					\
					i+=j;								\
				}										\
			}



//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
void clean_up (void)
{
	if (DACOM)
	{
		DACOM->ShutDown();
		DACOM->Release();
		DACOM = 0;
	}
}
//--------------------------------------------------------------------------//
//
void __cdecl _localprintf (const char *fmt, ...)
{
	static HANDLE hConsoleOutput = GetStdHandle(STD_OUTPUT_HANDLE);
	char buffer[256];
	va_list ap;
	int length;
	DWORD dwWritten;

	va_start(ap, fmt);
	length = wvsprintf(buffer, fmt, ap);
	va_end(ap);

	WriteFile(hConsoleOutput, buffer, length, &dwWritten, 0);
}
//--------------------------------------------------------------------------//
// Find the implemenatation by examining the filename  
//
//
char * GetImplementation (const char *filename)
{
	char * result;
	static char copy[MAX_PATH];

	strcpy(copy, filename);

	if ((result = strrchr(copy, '.')) != 0)
	{
		if (strchr(result, '\\'))
			result = 0;
		else 
		{
			strupr(result++);
		}
	}

	return result;
}
//--------------------------------------------------------------------------//
// Create a new instance of a file system.
//
LPFILESYSTEM OpenFileSystem (IComponentFactory *pParent, const char *filename, const char *implementation = 0)
{
	LPFILESYSTEM pFile;
	DAFILEDESC desc = filename;

	desc.lpImplementation = implementation;
	desc.dwShareMode |= FILE_SHARE_WRITE;
	//
	// the rest of desc is already set to the correct default parameters
	//
	pParent->CreateInstance(&desc, (void **) &pFile);

	return pFile;
}
//--------------------------------------------------------------------------//
// List all of the file entries for this system
// RETURNS number of lines printed
//
int TreeView (LPFILESYSTEM pSystem, int iIndent)
{
	WIN32_FIND_DATA data;
	HANDLE handle;
	int result=0;

	if ((handle = pSystem->FindFirstFile("*.*", &data)) == INVALID_HANDLE_VALUE)
		return 0;	// directory is empty ?

	do
	{
		// make sure this not a silly "." entry
		if (data.cFileName[0] != '.' || strchr(data.cFileName, '\\') != 0)
		{
			DO_INDENTION(iIndent);

			_localprintf("%s [%d]\r\n", data.cFileName, data.nFileSizeLow);
			result++;

			//
			// output of the program is identical whether you traverse by changing directory
			// or by creating a new instance of IFileSystem each time
			//
#if 0
			if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				// traverse subdirectory
				if (pSystem->SetCurrentDirectory(data.cFileName))
				{
					result += TreeView(pSystem, iIndent+INDENTION);
					pSystem->SetCurrentDirectory("..");	// restore current directory
				}
			}
			else // attempt to open file as another file system
#endif
			{	
				LPFILESYSTEM pFile;

				pFile = OpenFileSystem(pSystem, data.cFileName);

				if (pFile)
				{
					result += TreeView(pFile, iIndent+INDENTION);
					pFile->Release();
				}
			}
		}

	} while (pSystem->FindNextFile(handle, &data));

	pSystem->FindClose(handle);

	return result;
}
//--------------------------------------------------------------------------
//
inline HANDLE CreateTempFile (char tempname[MAX_PATH+4])
{
	char path[MAX_PATH+4];
	DWORD dwPrefix = '\0ert';

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
char szINIData[] = "[Libraries]\r\nDOSFILE.dll";
void start_dacom (void)
{
	HANDLE hFile;
	char tempname[MAX_PATH+4];
	DWORD dwWritten;

	hFile = CreateTempFile(tempname);
	WriteFile(hFile, szINIData, sizeof(szINIData), &dwWritten, 0); 
	CloseHandle(hFile);

	if (DACOM->SetINIConfig(tempname) != GR_OK)
		_localprintf("Temp=%s, handle=%d, Error=%d\r\n", tempname, hFile, GetLastError());
	DeleteFile(tempname);
}
//--------------------------------------------------------------------------//
//
int main(int argc, char *argv[])
{
	_localprintf(szBanner);

	if (argc != 2)
	{
		_localprintf(szUsage);
		return 1;
	}

	//
	// Acquire pointer to object manager
	//

	if ((DACOM = DACOM_Acquire()) == 0)
	{
		_localprintf("DACOM startup failed! (Begin the finger pointing.)\r\n");
		return -1;
	}

	atexit(clean_up);
	start_dacom();

//	IncreaseHeapSize(32000, 0, 0);
//	_localprintf("Initial Memory: Heap size=%d, Available=%d\r\n", HEAP->GetHeapSize(), HEAP->GetAvailableMemory());

	//
	// start the ball rolling
	//

	LPFILESYSTEM pFile = OpenFileSystem(DACOM, argv[1]);

	if (pFile)
	{
//		pFile->LockFile(0, 0, 0, 4,	0);
		_localprintf("\r\n%s\r\n", argv[1]);
		TreeView(pFile, INDENTION);
//		pFile->UnlockFile(0, 0, 0, 4, 0);
		pFile->Release();
	}
	else
		_localprintf("ERROR: Could not open file: %s\r\n", argv[1]);

	{
//		IHeap * heap = HEAP_Acquire();
//		heap->Release();
	}


//	_localprintf("\r\nFinal Memory: Heap size=%d, Available=%d\r\n", HEAP->GetHeapSize(), HEAP->GetAvailableMemory());

	return 0;
}



//--------------------------------------------------------------------------//
//----------------------------END TreeView.cpp------------------------------//
//--------------------------------------------------------------------------//
