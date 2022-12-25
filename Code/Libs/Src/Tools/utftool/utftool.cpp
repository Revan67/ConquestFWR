//--------------------------------------------------------------------------//
//                                                                          //
//                                UTFTool.cpp                               //
//                                                                          //
//               COPYRIGHT (C) 1997 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//

#include <windows.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "DACOM.h"
#include "FileSys.h"
#include "tsmartpointer.h"

#include "regexp.h"

//
// Version info
//

const int VERSION_MAJOR = 1;
const int VERSION_MINOR = 2;

//
// Class and structure definitions
//

struct FilePath
{
	char buffer[MAX_PATH+4];

	FilePath () {buffer[0] = '\0';}
	FilePath (const char *path) { set (path); }

	void set (const char *path)
	{
		if (path == NULL)
		{
			// Special case
			buffer[0] = '\0';
		}
		else
		{
			strncpy (buffer, path, MAX_PATH);
			buffer[MAX_PATH] = '\0';
		}
	}

	operator const char * () { return (const char *) buffer; }
	operator char * () { return buffer; }

	// NOTE: Negative indices are from the end of the string, i.e. -1 is the last character
	char & operator [] (int index)
	{
		if (index >= 0)
		{
			return buffer[index];
		}
		else
		{
			return buffer[strlen(buffer)+index];
		}
	}
	int length() { return strlen(buffer); }

	// *** TODO: Add operations to handle common path manipulations, to check for existance, etc.
};

//--------------------------------------------------------------------------//

char szBanner[] = "UTFTool v%d.%d\n";
char szUsage[]  = 
	"UTFTool [options] {ls <filespec> | cp <srcspec> <destspec> | rm <filespec> | cr <DOSSpec> | md <srcSpec> <newDir>}\n"
	"Options:\n"
	"  -r     Recurse subdirectories\n"
	"  -d     Debug mode. Don't actually do anything.\n"
	"  -t     Display creation and access times\n"
	"  -tc    Display creation time only\n"
	"  -ta    Display access time only\n"
	"Examples:\n"
	" Create a UTF file\n"
	"   \"UTFTOOL cr c:\\mydir\\test.utf\n"
	" List the DOS directory info for a UTF file\n"
	"   UTFTOOL ls c:\\mydir\\test.utf\n"
	" List directory info for the contents of a UTF file\n"
	"   UTFTOOL ls c:\\mydir\\test.utf\\\n"
	" Copy from inside a UTF file into a DOS directory\n"
	"   UTFTOOL cp c:\\mydir\\test.utf\\utfdir\\subdir\\* c:\\mydir\n"
	" Copy from a DOS directory into a UTF file\n"
	"   UTFTOOL cp c:\\mydir\\file c:\\mydir\\test.utf\\utfdir\\subdir\\file\n"
	" Remove a file from a UTF file\n"
	"   UTFTOOL rm c:\\mydir\\test.utf\\utfdir\\subdir\\file\n"
	" Remove a directory and its children from a UTF file\n"
	"   UTFTOOL rm c:\\mydir\\test.utf\\utfdir\\subdir\n"
	" Remove a directory's children from a UTF file, but leave the directory there\n"
	"   UTFTOOL rm c:\\mydir\\test.utf\\utfdir\\subdir\\\n"
	" Remove all files ending in DAT from a UTF file\n"
	"   UTFTOOL -r rm c:\\mydir\\test.utf\\*.DAT\n"
;


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
#if 0
void __cdecl _localprintf (const char *fmt, ...)
{
	static HANDLE hConsoleOutput = GetStdHandle(STD_OUTPUT_HANDLE);
	char buffer[4096];
	va_list ap;
	int length;
	DWORD dwWritten;

	va_start(ap, fmt);
	length = wvsprintf(buffer, fmt, ap);
	va_end(ap);

	WriteFile(hConsoleOutput, buffer, length, &dwWritten, 0);
}
#else
#define _localprintf printf
#endif

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
LPFILESYSTEM CreateFileSystem (IComponentFactory *pParent, const char *filename, bool bPrintError, const char *implementation = 0)
{
	LPFILESYSTEM pFile;
	char buffer[MAX_PATH+4];
	DAFILEDESC desc = buffer;

	if (TestValid(filename, buffer) == false && bPrintError==0)
		_localprintf("WARNING: Replacing filename '%s' with '%s'.\n", filename, buffer);

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
LPFILESYSTEM OpenFileSystem (IComponentFactory *pParent, const char *filename, const char *implementation = 0, bool writable = false)
{
	LPFILESYSTEM pFile;
	DAFILEDESC desc = filename;

	desc.lpImplementation = implementation;
	if (writable)
	{
		desc.dwDesiredAccess |= GENERIC_WRITE;
		desc.dwShareMode = 0;		// no sharing
	}

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

						if ((pFileOut = CreateFileSystem(pSystemOut, data.cFileName, true)) != 0)
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
					if ((pFileOut = CreateFileSystem(pSystemOut, data.cFileName, false)) != 0)
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
//--------------------------------------------------------------------------
//
inline HANDLE CreateTempFile (char tempname[MAX_PATH+4])
{
	char path[MAX_PATH+4];
	DWORD dwPrefix = '\0kap';

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

	DACOM->SetINIConfig(tempname);
	DeleteFile(tempname);
}
//--------------------------------------------------------------------------//
// parse the command line
//

enum UTFCommand
{
	LIST,
	COPY,
	REMOVE,
	CREATE,
	MAKEDIR
};

typedef unsigned int UTFOptions;
const UTFOptions OPT_DEFAULT = 0;
const UTFOptions OPT_RECURSE = 1;
const UTFOptions OPT_ACCESSTIME = 2;
const UTFOptions OPT_CREATETIME = 4;
const UTFOptions OPT_TEST = 8;

bool parse_command_line (int argc, char *argv[], UTFCommand &command, char * &srcArg, char * &destArg, UTFOptions &options)
{
	// Bail on patently bogus command lines
	if (argc < 3)
	{
		return false;
	}

	// Parse out the options
	options = OPT_DEFAULT;
	int i = 1;
	while (i < argc && argv[i][0] == '-')
	{
		switch (argv[i][1])
		{
		case 'r':
		case 'R':
			options |= OPT_RECURSE;
			break;

		case 't':
		case 'T':
			if (argv[i][2] == 'a' || argv[i][2] == 'A')
			{
				options |= OPT_ACCESSTIME;
			}
			else if (argv[i][2] == 'c' || argv[i][2] == 'C')
			{
				options |= OPT_CREATETIME;
			}
			else
			{
				options |= OPT_CREATETIME | OPT_ACCESSTIME;
			}
			break;

		case 'd':
		case 'D':
			options |= OPT_TEST;
			break;

		default:
			_localprintf ("Ignoring invalid option \"%c\".\n", argv[i][1]);
			break;
		}
		++i;
	}

	if (i >= argc)
	{
		return false;
	}

	// Parse out the command and its arguments
	if (!stricmp ("ls", argv[i]))
	{
		if (i+1 >= argc)
		{
			return false;
		}
		command = LIST;
		srcArg = argv[i+1];
		destArg = NULL;
	}
	else if (!stricmp ("cp", argv[i]))
	{
		if (i+2 >= argc)
		{
			return false;
		}
		command = COPY;
		srcArg = argv[i+1];
		destArg = argv[i+2];
	}
	else if (!stricmp ("rm", argv[i]))
	{
		if (i+1 >= argc)
		{
			return false;
		}
		command = REMOVE;
		srcArg = argv[i+1];
		destArg = NULL;
	}
	else if (!stricmp ("md", argv[i]))
	{
		if (i+2 >= argc)
		{
			return false;
		}
		command = MAKEDIR;
		srcArg = argv[i+1];
		destArg = argv[i+2];
	}
	else if (!stricmp ("cr", argv[i]))
	{
		if (i+1 >= argc)
		{
			return false;
		}
		command = CREATE;
		srcArg = argv[i+1];
		destArg = NULL;
	}
	else
	{
		return false;
	}
	return true;
}

// Regular expressions
RegExp pathExp("(.*\\\\)([^\\\\:]*)$");
RegExp rootExp("((^[a-zA-Z]:)|(^\\\\\\\\[^\\\\]+)|(^\\.\\.?))\\\\");
RegExp segmentExp("^\\\\([^\\\\]*)");

//--------------------------------------------------------------------------//
// Create a new instance of the given path. This works on both the
// DOS and UTF file systems.
//
LPFILESYSTEM OpenPath (IComponentFactory *pParent, const char *path, bool writable = false)
{
	// The problem: A full path spec, in general, contains a DOS file specifier to a file, and an
	// optional path specifier within the file if it is a UTF file.
	// This function will walk the given path, creating a file system for each segment.
	// This is tedious, but needed.
	// I KNOW this code is ugly, but it works.  Make it go faster if you feel so compelled.  -TNB

	// Extract the root of the filesystem. This must always be a DOS filesystem, but it can be either of
	// three forms of absolute path ("c:\path", "\\machine\path", or "\path") or three forms of
	// relative path (".", "..", and implied ".")

	// This was easier to handle with regular expressions.

	FilePath rootPath, pathBuffer;
	const char *start;

	if (rootExp.test (path))
	{
		// Extract the root and advance past it.
		rootPath.set (rootExp.get(1));
		start = path + strlen(rootPath);
	}
	else
	{
		// This thing doesn't have a root specifier, so assume "."
		rootPath.set (".\\");
		pathBuffer.set ("\\");
		strcat (pathBuffer, path);
		start = (const char *) pathBuffer;
	}

	// Attempt to open the root file system, bailing on failure.
	LPFILESYSTEM fs = OpenFileSystem (pParent, rootPath, NULL, writable);
	
	if (!fs)
	{
		return NULL;
	}

	// Start now points to the path. Attempt to create a file system for each point in the path.
	FilePath segment;
	while (*start)
	{
		// Extract the next path segment.
		if (!segmentExp.test(start))
		{
			_localprintf ("Illegal path segment %s\n", start);
			break;
		}

		// NOTE: Segment length is not the same as segment.length().
		int segmentLength = strlen(segmentExp.get(0));
		segment.set (segmentExp.get(1));

		// First, attempt to enter the directory. If that fails, attempt to open a UTF file system.
		// If that fails, bail.

		if (!fs->SetCurrentDirectory (segment))
		{
			LPFILESYSTEM newFs = OpenFileSystem (fs, segment, "UTF", writable);
			fs->Release();
			fs = newFs;
			if (!newFs)
			{
				return NULL;
			}
		}

		// Advance the start pointer by the length of the entire matched segment, not just the
		// portion we cared about.
		start += segmentLength;
	}

	return fs;
}

//--------------------------------------------------------------------------//
// Copy command
//

void copy_path
(
	LPFILESYSTEM destFs, const char *destPath, const char *destPattern,
	LPFILESYSTEM srcFs, const char *srcPath, const char *srcPattern,
	UTFOptions options
)
{
	// Perform a find operation using the given pattern.

	WIN32_FIND_DATA fd;
	HANDLE fh = srcFs->FindFirstFile (srcPattern, &fd);
	if (fh != INVALID_HANDLE_VALUE)
	{
		do
		{
			// Skip the silly '.' and '..' entries.
			if (fd.cFileName[0] == '.' && (fd.cFileName[1] == '\0' || fd.cFileName[1] == '.'))
			{
				continue;
			}

			// If this is a directory, and recursion is on, open the filesystem as a child and
			// recurse. Otherwise, copy the given file into the source directory.

			if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				if (options & OPT_RECURSE)
				{
					LPFILESYSTEM srcSubFs = OpenFileSystem (srcFs, fd.cFileName);
					if (srcSubFs)
					{
						// Attempt to open the destination filesystem. If this fails, create the
						// directory and try again.
						LPFILESYSTEM destSubFs = OpenFileSystem (destFs, fd.cFileName);
						if (!destSubFs)
						{
							// Create the subdirectory.
							if (!destFs->CreateDirectory (fd.cFileName))
							{
								_localprintf ("Failed to create destination directory \"%s\" in \"%s\". Skipping.\n", destPath, fd.cFileName);
								srcSubFs->Release();
								continue;
							}

							// Open the subdirectory.
							destSubFs = OpenFileSystem (destFs, fd.cFileName);
							if (!destSubFs)
							{
								_localprintf ("Failed to open destination directory \"%s\" in \"%s\". Skipping.\n", destPath, fd.cFileName);
								srcSubFs->Release();
								continue;
							}
						}

						// Both the source and destination file systems are open. Recurse.

						char srcBuffer[MAX_PATH];
						char destBuffer[MAX_PATH];
						sprintf (srcBuffer, "%s\\%s", srcPath, fd.cFileName);
						sprintf (destBuffer, "%s\\%s", destPath, fd.cFileName);
						copy_path (destSubFs, destBuffer, "", srcSubFs, srcBuffer, "*.*", options);
						destSubFs->Release();
						srcSubFs->Release();
					}
					else
					{
						_localprintf ("Failed to open source subdirectory \"%s\".\n", fd.cFileName);
					}
				}
			}
			else
			{
				LPFILESYSTEM fsIn = OpenFileSystem (srcFs, fd.cFileName);
				if (fsIn)
				{
					LPFILESYSTEM fsOut;
					const char *fname;
					if (destPattern[0] != '\0')
					{
						fname = destPattern;
					}
					else
					{
						fname = fd.cFileName;
					}
					// NOTE: The newly created file is a DOS file, not a UTF.
					fsOut = CreateFileSystem (destFs, fname, false);

					if (fsOut)
					{
						// Both files are now open, perform the copy.
						if (!CopyFile (fsOut, fsIn))
						{
							_localprintf ("Failed to copy %s\\%s to %s\\%s.\n", srcPath, fd.cFileName, destPath, fname);
						}
						else
						{
							_localprintf ("%s\\%s -> %s\\%s\n", srcPath, fd.cFileName, destPath, fname);
						}

						fsOut->Release();
					}

					fsIn->Release();
				}
				else
				{
					_localprintf ("Failed to open source file \"%s\" in \"%s\". Skipping.\n", fd.cFileName, srcPath);
				}
			}
		}
		while (srcFs->FindNextFile(fh, &fd));
		srcFs->FindClose (fh);
	}
	else
	{
		// This is an error.
		_localprintf ("Failed to find \"%s\" in \"%s\".\n", (char *) srcPattern, (char *) srcPath);
	}
}

void do_copy (char *srcArg, char *destArg, UTFOptions options)
{
	// Validate the arguments, then seperate the spec portions from the path portions.
	FilePath srcPath, srcSpec, destPath, destSpec;
	if (pathExp.test(srcArg))
	{
		srcPath.set(pathExp.get(1));
		srcSpec.set(pathExp.get(2, NULL, 80, 0, false)); // sets to "" if not found
	}
	else
	{
		// If here, we just have a bare filename. By convention, this will mean that the
		// path is ".", and the given name is a filespec.
		srcPath.set (".\\");
		srcSpec.set (srcArg);
	}

	// If the source filename portion is blank, substitute "*.*", otherwise just use what is there.
	if (srcSpec[0] == '\0')
	{
		srcSpec.set("*.*");
	}

	if (pathExp.test(destArg))
	{
		destPath.set(pathExp.get(1));
		destSpec.set(pathExp.get(2, NULL, 80, 0, false)); // sets to "" if not found
	}
	else
	{
		// If here, we just have a bare filename. By convention, this will mean that the
		// path is ".", and the given name is a filespec.
		// Assume that the file specifies an entire path. Treat the same as a trailing '\'
		destPath.set (".\\");
		destSpec.set (destArg);
	}

	// There can be no wildcards in the destination spec.
	if (strchr (destSpec, '*') != NULL || strchr (destSpec, '?') != NULL)
	{
		_localprintf ("Illegal wildcards in destination spec \"%s\".\n", destArg);
		return;
	}

	// If the source contains wildcards, the destination cannot be a single file, because that doesn't
	// make sense.

	if (strchr (srcSpec, '*') != NULL || strchr (srcSpec, '?') != NULL)
	{
		if (destSpec[0] != '\0')
		{
			_localprintf
			(
				"Won't copy wildcard %s into the file %s.\n"
				"Perhaps you meant \"%s\\\"?\n",
				srcArg, destArg, destArg
			);
			return;
		}
	}

	// Bail here if debug mode is on.
	if (options & OPT_TEST)
	{
		_localprintf
		(
			"In debug mode. Skipping copy of %s%s to %s%s.\n",
			(char *) srcPath, (char *) srcSpec, 
			(char *) destPath, (char *) destSpec 
		);
		return;
	}

	// Attempt to open the give destination file system. This will NOT create the filesystem if
	// it does not exist.
	LPFILESYSTEM destFs = OpenPath(DACOM, destPath, true);
	if (!destFs)
	{
		_localprintf ("Failed to open \"%s\".\n", (char *) destPath);
		return;
	}

	// The destination file system is open. Attempt to open the source.
	LPFILESYSTEM srcFs = OpenPath (DACOM, srcPath);
	if (srcFs)
	{
		// Both filesystems are opened.
		// Traverse the source filesystem, copying files and creating directories.
		// To be consistant, remove trailing '\' from the paths.
		char &destEndChar = destPath[-1];
		char &srcEndChar = srcPath[-1];
		if (destEndChar == '\\')
		{
			destEndChar = '\0';
		}
		if (srcEndChar == '\\')
		{
			srcEndChar = '\0';
		}

		copy_path (destFs, destPath, destSpec, srcFs, srcPath, srcSpec, options);

		// Relese the source filesystem. We are done.
		srcFs->Release();
	}
	else
	{
		_localprintf ("Failed to open source filesystem \"%s\"\n", (const char *) srcPath);
	}

	// Release the destination filesystem. We are done.
	destFs->Release();
}

//--------------------------------------------------------------------------//
// List command
//

void list_data (WIN32_FIND_DATA *fd, const char *pathName, UTFOptions options)
{
	// Displays the data for the given file.
	// Format: attributes createTime accessTime size filename 

	// Display the attributes.
	// Attributes are shown as follows: Directory, readonly.
	// Unset fields are '_', set fields are 'D' and 'R', respectively.
	char attrib[3];
	memset (attrib, '_', sizeof(attrib));
	attrib[2] = '\0';
	if (fd->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
	{
		attrib[0] = 'D';
	}
	if (fd->dwFileAttributes & FILE_ATTRIBUTE_READONLY)
	{
		attrib[1] = 'R';
	}
	_localprintf ("%5.5s  ", attrib);

	// Calculate and display the system create and access times.
	FILETIME locTime;
	SYSTEMTIME sysCreateTime, sysAccessTime;
	FileTimeToLocalFileTime (&fd->ftCreationTime, &locTime);
	FileTimeToSystemTime (&locTime, &sysCreateTime);
	FileTimeToLocalFileTime (&fd->ftLastAccessTime, &locTime);
	FileTimeToSystemTime (&locTime, &sysAccessTime);

	if (options & OPT_CREATETIME)
	{
		_localprintf
		(
			"%02.2d/%02.2d/%4.4d-%02.2d:%02.2d:%02.2d  ", 
			sysCreateTime.wMonth,sysCreateTime.wDay,sysCreateTime.wYear,
			sysCreateTime.wHour,sysCreateTime.wMinute,sysCreateTime.wSecond
		);
	}

	if (options & OPT_ACCESSTIME)
	{
		_localprintf
		(
			"%02.2d/%02.2d/%4.4d-%02.2d:%02.2d:%02.2d  ", 
			sysAccessTime.wMonth,sysAccessTime.wDay,sysAccessTime.wYear,
			sysAccessTime.wHour,sysAccessTime.wMinute,sysAccessTime.wSecond
		);
	}

	// Display the file size.
	if (fd->nFileSizeHigh)
	{
		_localprintf ("%d%010d  ",fd->nFileSizeHigh, fd->nFileSizeLow);
	}
	else
	{
		_localprintf ("%10d  ", fd->nFileSizeLow);
	}

	// Finally, display the full file path
	_localprintf ("%s\\%s\n", pathName, fd->cFileName);
}

void list_path (LPFILESYSTEM fs, const char *path, const char *pattern, UTFOptions options)
{
	// Perform a find operation using the given pattern.
	// NOTE: This is a two pass operation because recursion must apply to all directories, not just
	// those matching the given pattern. However, only those files and directories matching the
	// pattern will be listed.

	// Attempt to find the given file in this directory.
	WIN32_FIND_DATA fd;
	HANDLE fh = fs->FindFirstFile (pattern, &fd);
	if (fh != INVALID_HANDLE_VALUE)
	{
		// 
		do
		{
			// Skip the silly '.' and '..' entries.
			if (fd.cFileName[0] == '.' && (fd.cFileName[1] == '\0' || fd.cFileName[1] == '.'))
			{
				continue;
			}

			// If this is not a directory, or recursion is not turned on, display the data for the file.
			// If recursion is on, directories are displayed below.

//			if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) || !(options & OPT_RECURSE))
			{
				list_data (&fd, path, options);
			}
		}
		while (fs->FindNextFile(fh, &fd));
		fs->FindClose (fh);

	}
	else
	{
		// *** This will get printed once for every directory in which the pattern didn't match.
		// *** Do we really want this?
//		_localprintf ("Failed to find \"%s\" in \"%s\".\n", (char *) pattern, (char *) path);
	}

	// If recursion is on, recurse into all sub-directories
	// Do not indicate if there are no matches, i.e. this directory is empty.

	if (options & OPT_RECURSE)
	{
		fh = fs->FindFirstFile ("*.*", &fd);
		if (fh != INVALID_HANDLE_VALUE)
		{
			do
			{
				// Skip the silly '.' and '..' entries.
				if (fd.cFileName[0] == '.' && (fd.cFileName[1] == '\0' || fd.cFileName[1] == '.'))
				{
					continue;
				}

				// If this is a directory, open the filesystem as a child and recurse.
				if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				{
					// Display this directory's data.
//					list_data (&fd, path, options);

					LPFILESYSTEM subFs = OpenFileSystem (fs, fd.cFileName);
					if (subFs)
					{
						char buffer[MAX_PATH];
						sprintf (buffer, "%s\\%s", path, fd.cFileName);
						list_path (subFs, buffer, pattern, options);
						subFs->Release();
					}
					else
					{
						_localprintf ("Failed to open subdirectory \"%s\".\n", fd.cFileName);
					}
				}
			}
			while (fs->FindNextFile(fh, &fd));
			fs->FindClose (fh);
		}
	}
}

void do_list (char *srcArg, UTFOptions options)
{
	// Validate the path expression and seperate the path from the filename portion of the
	// argument.
	FilePath path, filespec;
	if (pathExp.test(srcArg))
	{
		path.set(pathExp.get(1));
		filespec.set(pathExp.get(2, NULL, 80, 0, false)); // sets to "" if not found
	}
	else
	{
		// If here, we just have a bare filename. By convention, this will mean that the
		// path is ".", and the given name is a filespec.
		path.set (".\\");
		filespec.set (srcArg);
	}

	// If the filename portion is blank, substitute "*.*", otherwise just use what is there.
	if (filespec[0] == '\0')
	{
		filespec.set("*.*");
	}

	// Attempt to open the path's file system.
	LPFILESYSTEM fs = OpenPath (DACOM, path);
	if (fs)
	{
		// The file system of the path is open.
		// List its contents using the given spec.
		// To be consistant, remove the trailing '\'
		char &endChar = path[-1];
		if (endChar == '\\')
		{
			endChar = '\0';
		}
		_localprintf ("Listing of %s:\n", srcArg);
		list_path (fs, path, filespec, options);
		fs->Release();
	}
	else
	{
		_localprintf ("Failed to open filesystem \"%s\"\n", (const char *) path);
	}
}

//--------------------------------------------------------------------------//
// Remove command
//
void remove_path (LPFILESYSTEM fs, const char *path, const char *pattern, UTFOptions options)
{
	// Perform a find operation using the given pattern.
	// Removing files that match the pattern is straight forward. Removing directories requires
	// some explaination.
	// If a directory matches the pattern, it and all its subdirectories are removed, regardless of the
	// recursion options. If the directory does not match the pattern, and recursion is on, it is recursed
	// into but not removed. If directory doesn't match and recursion is turned off, it is skipped.

	// Attempt to find the given file(s) in this directory.
	WIN32_FIND_DATA fd;
	HANDLE fh = fs->FindFirstFile (pattern, &fd);
	if (fh != INVALID_HANDLE_VALUE)
	{
		do
		{
			// Skip the silly '.' and '..' entries.
			if (fd.cFileName[0] == '.' && (fd.cFileName[1] == '\0' || fd.cFileName[1] == '.'))
			{
				continue;
			}

			// If this is a directory, remove its children recursively, regardless of the recursion
			// option. The remove the directory itself. Otherwise, remove the file.

			if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				// Remove the children.
				LPFILESYSTEM subFs = OpenFileSystem (fs, fd.cFileName);
				if (subFs)
				{
					char buffer[MAX_PATH];
					sprintf (buffer, "%s\\%s", path, fd.cFileName);
					remove_path (subFs, buffer, "*.*", options | OPT_RECURSE);
					subFs->Release();
				}
				else
				{
					_localprintf ("Failed to open subdirectory \"%s\".\n", fd.cFileName);
				}

				// Remove this directory.
				_localprintf ("Removing dir %s\\%s...", path, fd.cFileName);

				if (options & OPT_TEST)
				{
					_localprintf ("done (test).\n");
				}
				else
				{
					if (fs->RemoveDirectory (fd.cFileName))
					{
						_localprintf ("done.\n");
					}
					else
					{
						_localprintf ("FAILED!\n");
					}
				}
			}
			else
			{
				_localprintf ("Removing %s\\%s...", path, fd.cFileName);
				
				// Remove the file only if not in test mode
				if (options & OPT_TEST)
				{
					_localprintf ("done (test).\n");
				}
				else
				{
					if (fs->DeleteFile (fd.cFileName))
					{
						_localprintf ("done.\n");
					}
					else
					{
						_localprintf ("FAILED!\n");
					}
				}
			}
		}
		while (fs->FindNextFile(fh, &fd));
		fs->FindClose (fh);

	}

	// If recursion is on, recurse into all sub-directories
	// Do not indicate if there are no matches, i.e. this directory is empty.

	if (options & OPT_RECURSE)
	{
		fh = fs->FindFirstFile ("*.*", &fd);
		if (fh != INVALID_HANDLE_VALUE)
		{
			do
			{
				// Skip the silly '.' and '..' entries.
				if (fd.cFileName[0] == '.' && (fd.cFileName[1] == '\0' || fd.cFileName[1] == '.'))
				{
					continue;
				}

				// If this is a directory, open the filesystem as a child and recurse.
				if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				{
					LPFILESYSTEM subFs = OpenFileSystem (fs, fd.cFileName);
					if (subFs)
					{
						char buffer[MAX_PATH];
						sprintf (buffer, "%s\\%s", path, fd.cFileName);
						remove_path (subFs, buffer, pattern, options);
						subFs->Release();
					}
					else
					{
						_localprintf ("Failed to open subdirectory \"%s\".\n", fd.cFileName);
					}
				}
			}
			while (fs->FindNextFile(fh, &fd));
			fs->FindClose (fh);
		}
	}
}

void do_remove (char *srcArg, UTFOptions options)
{
	// Validate the path expression and seperate the path from the filename portion of the
	// argument.
	FilePath path, filespec;
	if (pathExp.test(srcArg))
	{
		path.set(pathExp.get(1));
		filespec.set(pathExp.get(2, NULL, 80, 0, false)); // sets to "" if not found
	}
	else
	{
		// If here, we just have a bare filename. By convention, this will mean that the
		// path is ".", and the given name is a filespec.
		path.set (".\\");
		filespec.set (srcArg);
	}

	// If the filename portion is blank, substitute "*.*", otherwise just use what is there.
	if (filespec[0] == '\0')
	{
		filespec.set("*.*");
	}

	// Attempt to open the path's file system.
	LPFILESYSTEM fs = OpenPath (DACOM, path, true);
	if (fs)
	{
		// The file system of the path is open.
		// Remove the matching files.
		// To be consistant, remove the trailing '\'
		char &endChar = path[-1];
		if (endChar == '\\')
		{
			endChar = '\0';
		}
		_localprintf ("Removing %s%s:\n", srcArg, (options & OPT_RECURSE) ? ", recursively" : "");
		remove_path (fs, path, filespec, options);
		fs->Release();
	}
	else
	{
		_localprintf ("Failed to open filesystem \"%s\"\n", (const char *) path);
	}
}

//--------------------------------------------------------------------------//
// Create command
//
void do_create (char *srcArg, UTFOptions options)
{
	// Create the given src path as a UTF file.
	// NOTE: It is illegal for wildcards to exist in this context.

	// Validate the argument before attempting to create the file system
	if (strchr (srcArg, '*') != NULL || strchr (srcArg, '?') != NULL)
	{
		_localprintf ("Wildcards not allowed in creation of spec \"%s\".\n", srcArg);
		return;
	}

	// Bail here if debug mode is on.
	if (options & OPT_TEST)
	{
		_localprintf("In debug mode. Skipping creation of %s.\n", srcArg);
		return;
	}

	// Create the file system as a UTF file.
	LPFILESYSTEM fs;
	fs = CreateFileSystem (DACOM, srcArg, false, "UTF");
	if (fs)
	{
		_localprintf ("Created file \"%s\".\n", srcArg);
		fs->Release();
	}
	else
	{
		_localprintf ("Failed to create the new UTF file \"%s\".\n", srcArg);
	}
}

//--------------------------------------------------------------------------//
// Makedir command
//
void do_makedir (char *srcArg, char *destArg, UTFOptions options)
{
	// Create the given destArg as a subdir of the given srcArg.
	// NOTE: It is illegal for wildcards to exist in this context.

	// Validate the arguments before attempting to create the file system
	if (strchr (srcArg, '*') != NULL || strchr (srcArg, '?') != NULL)
	{
		_localprintf ("Wildcards not allowed in root directory spec \"%s\".\n", srcArg);
		return;
	}

	FilePath path, filespec;
	if (pathExp.test(srcArg))
	{
		path.set(pathExp.get(1));
		filespec.set(pathExp.get(2, NULL, 80, 0, false)); // sets to "" if not found
	}
	else
	{
		// If here, we just have a bare filename. By convention, this will mean that the
		// path is ".", and the given name is a filespec, in this case, a directory spec.
		path.set (".");
		filespec.set (srcArg);
	}

	if (strchr (destArg, '*') != NULL || strchr (destArg, '?') != NULL)
	{
		_localprintf ("Wildcards not allowed in directory spec \"%s\".\n", destArg);
		return;
	}

	// Bail here if debug mode is on.
	if (options & OPT_TEST)
	{
		_localprintf
		(
			"In debug mode. Skipping creation of %s%s.\n",
			(char *) path, (char *) filespec 
		);
		return;
	}

	// Open the source path, which will be used as the current working directory.
	LPFILESYSTEM fs;
	fs = OpenPath (DACOM, path, true);
	if (fs)
	{
		// Attempt to enter the filespec directory, if it is valid.
		if (filespec[0] != '\0')
		{
			if (!fs->SetCurrentDirectory (filespec))
			{
				_localprintf
				(
					"Could not enter directory \"%s\".\n"
					"Did you mean \"%s\\\"?\n",
					srcArg, srcArg
				);
				fs->Release();
				return;
			}
		}

		// Create the given directories.
		char buffer[MAX_PATH+4];
		char *start = destArg;
		char *end = strchr(start, '\\');
		while (start)
		{
			if (end)
			{
				strncpy (buffer, start, end-start);
				buffer[end-start] = '\0';
			}
			else
			{
				strcpy (buffer, start);
			}

			// Attempt to enter the directory before attempting to create it.
			if (!fs->SetCurrentDirectory (buffer))
			{
				// The directory doesn't exist, so attempt to create it.
				if (!fs->CreateDirectory (buffer))
				{
					_localprintf ("Failed to create directory \"%s\" at segment \"%s\". Aborting.\n", destArg, buffer);
					break;
				}
			}

			// Advance to the next path segment.
			if (end)
			{
				start = end + 1;
				end = strchr(start, '\\');
			}
			else
			{
				start = end;
			}
		}

		fs->Release();
	}
	else
	{
		_localprintf ("Failed to open root directory \"%s\".\n", srcArg);
	}
}

//--------------------------------------------------------------------------//
//
int main(int argc, char *argv[])
{
	_localprintf(szBanner, VERSION_MAJOR, VERSION_MINOR);

	UTFCommand command;
	UTFOptions options;
	char *srcArg = NULL;
	char *destArg = NULL;

	// Parse the command line and check usage.
	if (!parse_command_line (argc, argv, command, srcArg, destArg, options))
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
	// Execute the given command.
	//

	switch (command)
	{
	case COPY:
		do_copy (srcArg, destArg, options);
		break;

	case LIST:
		do_list (srcArg, options);
		break;

	case REMOVE:
		do_remove (srcArg, options);
		break;

	case CREATE:
		do_create (srcArg, options);
		break;

	case MAKEDIR:
		do_makedir (srcArg, destArg, options);
		break;

	default:
		assert (false && "Invalid command.");
	}

	return 0;
}



//--------------------------------------------------------------------------//
//----------------------------END Pack.cpp------------------------------//
//--------------------------------------------------------------------------//
