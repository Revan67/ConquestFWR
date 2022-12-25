//--------------------------------------------------------------------------//
//                                                                          //
//                                Pack.cpp                                  //
//                                                                          //
//               COPYRIGHT (C) 1997 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*

   $Author:   JYENAWINE  $


	Traverses a file system, moving all files into destination system.
	This program makes these assumptions:
		The inputed name is a file system. (eg. IFF, UTF, or DOS directory)
		The file system implementation is assumed to be the filename's extention.


	Creates the directory structure first, because this is the optimal way to 
	create a UTF file. It would work either way though.




*/

//--------------------------------------------------------------------------//

#include <windows.h>

#include "DACOM.h"
#include "FileSys.h"

//--------------------------------------------------------------------------//

char szBanner[] = "Pack\n";
char szUsage[]  = "Pack { input file system } { output file system } [dos | utf]\n"
"  Example: \"PACK c:\\MyDir output.dat\" <- pack using UTF format (default)\n"
"  Example: \"PACK output.dat c:\\UnpackedDir dos\" <- unpack to DOS directory\n";


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
	static char copy[MAX_PATH];
	char *result;

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
LPFILESYSTEM CreateFileSystem (IComponentFactory *pParent, const char *filename, const char *implementation = 0)
{
	LPFILESYSTEM pFile;
	DAFILEDESC desc = filename;

	desc.lpImplementation = implementation;
	desc.dwCreationDistribution = CREATE_ALWAYS;
	desc.dwDesiredAccess |= GENERIC_WRITE;
	desc.dwShareMode = 0;		// no sharing

	//
	// the rest of desc is already set to the correct default parameters
	//

	pParent->CreateInstance(&desc, (void **) &pFile);

	return pFile;
}
//--------------------------------------------------------------------------//
// Create a new instance of a file system.
//
LPFILESYSTEM OpenFileSystem (IComponentFactory *pParent, const char *filename, const char *implementation = 0)
{
	LPFILESYSTEM pFile;
	DAFILEDESC desc = filename;

	desc.lpImplementation = implementation;

	//
	// the rest of desc is already set to the correct default parameters
	//
	pParent->CreateInstance(&desc, (void **) &pFile);

	return pFile;
}
//--------------------------------------------------------------------------//
//
BOOL CopyFile (LPFILESYSTEM pFileOut, LPFILESYSTEM pFileIn)
{
	char _buffer[4096];
	DWORD dwDataRead, dwDataToRead, dwDataWritten, dwLength, dwBufferLength;
	FILETIME CreationTime, LastAccessTime, LastWriteTime;
	char *vBuffer, *buffer;

	dwLength = pFileIn->GetFileSize();
	vBuffer = (char *) VirtualAlloc(0, dwLength, MEM_COMMIT, PAGE_READWRITE);

	if (vBuffer)
	{
		dwBufferLength = dwLength;
		buffer = vBuffer;
	}
	else
	{
		dwBufferLength = sizeof(_buffer);
		buffer = _buffer;
	}

	while (dwLength > 0)
	{
		dwDataToRead = __min(dwBufferLength, dwLength);
		if (pFileIn->ReadFile(0, buffer, dwDataToRead, &dwDataRead, 0) == 0)
			goto Fail;
		if (dwDataRead == 0)
			break;
		if (pFileOut->WriteFile(0, buffer, dwDataRead, &dwDataWritten, 0) == 0)
			goto Fail;
		if (dwDataWritten != dwDataRead)
			goto Fail;
		dwLength -= dwDataRead;
	}

	// set the file times

	pFileIn->GetFileTime(0, &CreationTime, &LastAccessTime, &LastWriteTime);
	pFileOut->SetFileTime(0, &CreationTime, &LastAccessTime, &LastWriteTime);

	if (vBuffer)
		VirtualFree(vBuffer, 0, MEM_RELEASE);

	return 1;
Fail:
	if (vBuffer)
		VirtualFree(vBuffer, 0, MEM_RELEASE);
	return 0;
}
//--------------------------------------------------------------------------//
// List all of the file entries for this system
// RETURNS number of direct child systems
//
int CreateStructure (LPFILESYSTEM pSystemIn, LPFILESYSTEM pSystemOut)
{
	WIN32_FIND_DATA data;
	HANDLE handle;
	int result=0;

	if ((handle = pSystemIn->FindFirstFile("*.*", &data)) == INVALID_HANDLE_VALUE)
		return 0;	// directory is empty ?

	do
	{
		// make sure this not a silly "." entry
		if (data.cFileName[0] != '.' || strchr(data.cFileName, '\\') != 0)
		{
			result++;
			LPFILESYSTEM pFileIn, pFileOut;

			pFileIn = OpenFileSystem(pSystemIn, data.cFileName);

			if (pFileIn)
			{
				// assume we are traversing a directory

				pSystemOut->CreateDirectory(data.cFileName);
				if (pSystemOut->SetCurrentDirectory(data.cFileName))
				{
					if (CreateStructure(pFileIn, pSystemOut) == 0 && (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
					{
						// system was actually a file, undo directory entry and create a file
						pSystemOut->SetCurrentDirectory("..");
						pSystemOut->RemoveDirectory(data.cFileName);

						if ((pFileOut = CreateFileSystem(pSystemOut, data.cFileName)) != 0)
							pFileOut->Release();
					}
					else
						pSystemOut->SetCurrentDirectory("..");
				}
				pFileIn->Release();
			}
		}

	} while (pSystemIn->FindNextFile(handle, &data));

	pSystemIn->FindClose(handle);

	return result;
}
//--------------------------------------------------------------------------//
// List all of the file entries for this system
// RETURNS number of direct child systems
//
int FillData (LPFILESYSTEM pSystemIn, LPFILESYSTEM pSystemOut)
{
	WIN32_FIND_DATA data;
	HANDLE handle;
	int result=0;

	if ((handle = pSystemIn->FindFirstFile("*.*", &data)) == INVALID_HANDLE_VALUE)
		return 0;	// directory is empty ?

	do
	{
		// make sure this not a silly "." entry
		if (data.cFileName[0] != '.' || strchr(data.cFileName, '\\') != 0)
		{
			result++;
			LPFILESYSTEM pFileIn, pFileOut;

			pFileIn = OpenFileSystem(pSystemIn, data.cFileName);

			if (pFileIn)
			{
				// assume we are traversing a directory

				if (pSystemOut->SetCurrentDirectory(data.cFileName) == 0)
				{
					// system was actually a file, copy the data
					if ((pFileOut = CreateFileSystem(pSystemOut, data.cFileName)) != 0)
					{
						CopyFile(pFileOut, pFileIn);
						_localprintf(".");
						pFileOut->Release();
					}
				}
				else
				{
				 	FillData(pFileIn, pSystemOut);
					pSystemOut->SetCurrentDirectory("..");
				}
				pFileIn->Release();
			}
		}

	} while (pSystemIn->FindNextFile(handle, &data));

	pSystemIn->FindClose(handle);

	return result;
}
//--------------------------------------------------------------------------//
//
int main(int argc, char *argv[])
{
	_localprintf(szBanner);

	if (argc != 3 && argc != 4)
	{
		_localprintf(szUsage);
		return 1;
	}

	//
	// Acquire pointer to object manager
	//

	if ((DACOM = DACOM_Acquire()) == 0)
	{
		_localprintf("DACOM startup failed! (Begin the finger pointing.)\n");
		return -1;
	}

	atexit(clean_up);

	//
	// start the ball rolling
	//

	LPFILESYSTEM pFileIn;
	LPFILESYSTEM pFileOut;
	char *implementation = (argc == 4) ? strupr(argv[3]) : "UTF";

	if ((pFileIn = OpenFileSystem(DACOM, argv[1])) == 0)
	{
		_localprintf("ERROR: Could not open file: %s\n", argv[1]);
		goto Done;
	}
	if (argc==4 && strcmp(implementation, "DOS")==0)
		CreateDirectory(argv[2],0);

	if ((pFileOut = CreateFileSystem(DACOM, argv[2], implementation)) == 0)
	{
		_localprintf("ERROR: Could not create file: %s\n", argv[2]);
		goto Done;
	}

	_localprintf("\nMirroring directory structure of %s in %s\n", argv[1], argv[2]);
	CreateStructure(pFileIn, pFileOut);

	_localprintf("\nCopying file data to %s.", argv[2]);
	FillData(pFileIn, pFileOut);
	_localprintf("\n");

	pFileIn->Release();
	pFileOut->Release();

Done:
	return 0;
}



//--------------------------------------------------------------------------//
//----------------------------END Pack.cpp------------------------------//
//--------------------------------------------------------------------------//
