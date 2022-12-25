//--------------------------------------------------------------------------//
//                                                                          //
//                                Pack.cpp                                  //
//                                                                          //
//               COPYRIGHT (C) 1997 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*

   $Author: Pbleisch $


	Traverses a file system, moving all files into destination system.
	This program makes these assumptions:
		The inputed name is a file system. (eg. IFF, UTF, or DOS directory)
		The file system implementation is assumed to be the filename's extention.


	Creates the directory structure first, because this is the optimal way to 
	create a UTF file. It would work either way though.




*/

//--------------------------------------------------------------------------//

#include <3DMAth.h>


#include <windows.h>

#include "DACOM.h"
#include "FileSys.h"
#include "IUTFWriter.h"

//--------------------------------------------------------------------------//

char szBanner[] = "Pack\n";
char szUsage[]  = "Pack { input file system } { output file system } [/U] [dos | utf]\n"
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


bool g_bUpdate;		// true if user wants to update an existing UTF file
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
// return FALSE if name contains invalid characters
//
static inline void setBit (U8 * bitArray, U8 pos)
{
	bitArray += (pos >> 3);
	bitArray[0] |= (1 << (pos & 7));
}
static inline bool getBit (U8 * bitArray, U8 pos)
{
	bitArray += (pos >> 3);
	return ((bitArray[0] & (1 << (pos & 7))) != 0);
}
bool TestValid (LPCTSTR lpFileName, char * output)
{
	static bool initialized = false;
	static U8 charMap[256/8];
	if (initialized == false)
	{
		setBit(charMap, '<');
		setBit(charMap, '>');
		setBit(charMap, ':');
		setBit(charMap, '"');
		setBit(charMap, '/');
		setBit(charMap, '|');
		initialized=true;
	}

	U8 a;
	bool result = true;

	while ((a = *lpFileName++) != 0)
	{
		if (getBit(charMap, a))
		{
			*output++ = '!';
			result = false;
		}
		else
			*output++ = a;
	}

	*output = 0;
	return result;
}
//--------------------------------------------------------------------------//
// Create a new instance of a file system.
//
LPFILESYSTEM CreateFileSystem (IComponentFactory *pParent, const char *filename, DWORD dwCreateMode=0, const char *implementation = 0)
{
	LPFILESYSTEM pFile;
	char buffer[MAX_PATH+4];
	DAFILEDESC desc = buffer;

#if 0
	if (TestValid(filename, buffer) == false && bPrintError==0)
		_localprintf("WARNING: Replacing filename '%s' with '%s'.\n", filename, buffer);
#else
	strcpy(buffer, filename);
#endif

	desc.lpImplementation = implementation;
	desc.dwCreationDistribution = (dwCreateMode)? dwCreateMode : ((g_bUpdate) ? OPEN_ALWAYS:CREATE_ALWAYS);
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
LPFILESYSTEM OpenFileSystem (IComponentFactory *pParent, const char *filename, HANDLE hFindFile = INVALID_HANDLE_VALUE, const char *implementation = 0)
{
	LPFILESYSTEM pFile;
	DAFILEDESC desc = filename;

	desc.lpImplementation = implementation;
	desc.hFindFirst = hFindFile;

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

	pFileOut->SetEndOfFile();

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
// count number of entries in old system
//
int CountEntries (LPFILESYSTEM pSystemIn, int level)
{
	WIN32_FIND_DATA data;
	HANDLE handle;
	int result=0;

	if ((handle = pSystemIn->FindFirstFile("*.*", &data)) == INVALID_HANDLE_VALUE)
		return 0;	// directory is empty ?

	do
	{
		// make sure this not a silly "." entry
		if (data.cFileName[0] != '.')
		{
			LPFILESYSTEM pFileIn;

			result++;
			if (level==0)
				_localprintf(".");
			pFileIn = OpenFileSystem(pSystemIn, data.cFileName, handle);

			if (pFileIn)
			{
				result += CountEntries(pFileIn, level+1);
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
int CreateStructure (LPFILESYSTEM pSystemIn, LPFILESYSTEM pSystemOut, int level)
{
	WIN32_FIND_DATA data;
	HANDLE handle;
	int result=0;

	if ((handle = pSystemIn->FindFirstFile("*.*", &data)) == INVALID_HANDLE_VALUE)
		return 0;	// directory is empty ?

	do
	{
		// make sure this not a silly "." entry
		if (data.cFileName[0] != '.')
		{
			result++;
			LPFILESYSTEM pFileIn, pFileOut;

			pFileIn = OpenFileSystem(pSystemIn, data.cFileName, handle);

			if (pFileIn)
			{
				// assume we are traversing a directory

				pSystemOut->CreateDirectory(data.cFileName);
				if (pSystemOut->SetCurrentDirectory(data.cFileName))
				{
					if (CreateStructure(pFileIn, pSystemOut, level+1) == 0 && (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
					{
						// system was actually a file, undo directory entry and create a file
						pSystemOut->SetCurrentDirectory("..");
						pSystemOut->RemoveDirectory(data.cFileName);

						if ((pFileOut = CreateFileSystem(pSystemOut, data.cFileName, CREATE_NEW)) != 0)
							pFileOut->Release();
					}
					else
						pSystemOut->SetCurrentDirectory("..");
				}
				pFileIn->Release();

				if (level==0)
					_localprintf(".");
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
int FillData (LPFILESYSTEM pSystemIn, LPFILESYSTEM pSystemOut, int level)
{
	WIN32_FIND_DATA data;
	HANDLE handle;
	int result=0;

	if ((handle = pSystemIn->FindFirstFile("*.*", &data)) == INVALID_HANDLE_VALUE)
		return 0;	// directory is empty ?

	do
	{
		// make sure this not a silly "." entry
		if (data.cFileName[0] != '.')
		{
			result++;
			LPFILESYSTEM pFileIn, pFileOut;

			pFileIn = OpenFileSystem(pSystemIn, data.cFileName, handle);

			if (pFileIn)
			{
				// assume we are traversing a directory

				if (pSystemOut->SetCurrentDirectory(data.cFileName) == 0)
				{
					// system was actually a file, copy the data
					if ((pFileOut = CreateFileSystem(pSystemOut, data.cFileName)) != 0)
					{
						bool bSameFile = false;

						if (g_bUpdate)
						{
							if (pFileIn->GetFileSize() == pFileOut->GetFileSize())
							{
								unsigned __int64 inTime, outTime;
								pFileIn->GetFileTime(0, NULL, NULL, (FILETIME *)&inTime);
								pFileOut->GetFileTime(0, NULL, NULL, (FILETIME *)&outTime);
								bSameFile = (inTime == outTime);
							}
						}
						if (bSameFile==0)
							CopyFile(pFileOut, pFileIn);
						pFileOut->Release();
					}
				}
				else
				{
				 	FillData(pFileIn, pSystemOut, level+1);
					pSystemOut->SetCurrentDirectory("..");
				}
				pFileIn->Release();
				if (level==0)
					_localprintf(".");
			}
		}

	} while (pSystemIn->FindNextFile(handle, &data));

	pSystemIn->FindClose(handle);

	return result;
}
//--------------------------------------------------------------------------//
// build a temporary ini file, pass it to DACOM
//
char szINIData[] = "[Libraries]\nDOSFILE.dll";
void start_dacom (void)
{
	DACOM->SetINIConfig(szINIData, DACOM_INI_STRING);
}
//--------------------------------------------------------------------------//
//
int main(int argc, char *argv[])
{
	_localprintf(szBanner);

	if (argc < 3)
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
	start_dacom();

	//
	// start the ball rolling
	//

	LPFILESYSTEM pFileIn;
	LPFILESYSTEM pFileOut;
	IUTFWriter *pUTFWriter=0;

	char *implementation = "UTF";
	
	if (argc == 4) 
	{
		if (argv[3][0] == '/')
			g_bUpdate = (toupper(argv[3][1]) == 'U');
		else
			implementation = strupr(argv[3]);
	}
	else
	if (argc == 5) 
	{
		if (argv[3][0] == '/')
		{
			g_bUpdate = (toupper(argv[3][1]) == 'U');
			implementation = strupr(argv[4]);
		}
		else
			implementation = strupr(argv[3]);
	}

	if ((pFileIn = OpenFileSystem(DACOM, argv[1])) == 0)
	{
		_localprintf("ERROR: Could not open file: %s\n", argv[1]);
		goto Done;
	}
	if (strcmp(implementation, "DOS")==0)
		CreateDirectory(argv[2],0);

	if ((pFileOut = CreateFileSystem(DACOM, argv[2], 0, implementation)) == 0)
	{
		_localprintf("ERROR: Could not create file: %s\n", argv[2]);
		goto Done;
	}

	if (pFileOut->QueryInterface( IID_IUTFWriter, (void **) &pUTFWriter) == GR_OK)
	{
		if (g_bUpdate==0)		// reserve space if starting from scratch
		{
			_localprintf("\nCounting directory entries needed in %s.\n", argv[1]);
			int entries = CountEntries(pFileIn, 0);

			pUTFWriter->AddDirEntries(entries);
		}
		pUTFWriter->HintExpansionNameSpace(1024);
	}

	_localprintf("\nMirroring directory structure of %s in %s.\n", argv[1], argv[2]);
	CreateStructure(pFileIn, pFileOut, 0);

	if (pUTFWriter)
		pUTFWriter->Flush();

	_localprintf("\nCopying file data to %s.\n", argv[2]);
	FillData(pFileIn, pFileOut, 0);
	_localprintf("\n");

	pFileIn->Release();
	pFileOut->Release();
	if (pUTFWriter)
		pUTFWriter->Release();

Done:
	return 0;
}



//--------------------------------------------------------------------------//
//----------------------------END Pack.cpp------------------------------//
//--------------------------------------------------------------------------//
