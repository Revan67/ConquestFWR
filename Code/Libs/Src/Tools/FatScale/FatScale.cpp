//
// FatScale.cpp - A program to determine the amount of "fat" in an IFileSystem file.
//

//
// Include files
//

#include <windows.h>
#include <stdio.h>

#include <dacom.h>
#include <filesys.h>
#include <tsmartpointer.h>

//
// Global variables
//

ICOManager *DACOM = NULL;
char szINIData[] = "[Libraries]\r\nDOSFILE.dll";

//
// Routines
//

static void usage ()
{
	printf
	(
		"usage: fatscale <filename>\n"
		"where \"filename\" is the name of the UTF file to check\n"
		"\n"
	);
}

LPFILESYSTEM OpenFileSystem (IComponentFactory *pParent, const char *filename, const char *implementation = 0, bool writable = false)
{
	LPFILESYSTEM pFile = NULL;
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

	if (pParent->CreateInstance(&desc, (void **) &pFile) != GR_OK)
	{
		pFile = NULL;
	}

	return pFile;
}

void clean_up (void)
{
	if (DACOM)
	{
		DACOM->ShutDown();
		DACOM->Release();
		DACOM = NULL;
	}
}

unsigned int CalcDirSize (IFileSystem *fs)
{
	// Calculates the summed size of all of the files in the current directory of the given
	// filesystem, and all of the directories in it.

	// Sum up the sizes of all of the files in this directory, recursing into the sub-directories
	unsigned int sumSize = 0;

	WIN32_FIND_DATA fd;
	HANDLE hf = fs->FindFirstFile ("*.*", &fd);
	if (hf)
	{
		do
		{
			if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				// If the directory is not one of the special ones, enter it and recurse
				if (fd.cFileName[0] == '.')
				{
					// Skip this file.
					// *** Is this a safe check?
				}
				else
				{
					fs->SetCurrentDirectory (fd.cFileName);
					sumSize += CalcDirSize (fs);
					fs->SetCurrentDirectory ("..");
				}
			}
			else
			{
				// This is a file. Get information about it.
				DAFILEDESC desc = fd.cFileName;
				HANDLE hFile = fs->OpenChild (&desc);
				if (hFile)
				{
					sumSize += fs->GetFileSize (hFile);
					fs->CloseHandle (hFile);
				}
			}
		} while (fs->FindNextFile (hf, &fd));
	}

	return sumSize;
}

//
// Main program entry point
//

int main (int argc, char *argv[])
{
	if (argc < 2)
	{
		usage();
		return 1;
	}

	char *filename = argv[1];

	// Initialize DACOM
	if ((DACOM = DACOM_Acquire()) == 0)
	{
		printf("DACOM startup failed!\n");
		return 1;
	}

	atexit(clean_up);

	if (DACOM->SetINIConfig(szINIData,DACOM_INI_STRING) != GR_OK)
	{
		printf("Failed to initialize DACOM.\n");
		return 1;
	}

	// First, create a UTF filesystem from the given name, bailing on error.
	unsigned int dataSize = 0;
	unsigned int win32Size = 0;
	{
		IFileSystem *fs = OpenFileSystem (DACOM, filename, "UTF");
		if (!fs)
		{
			printf ("Failed to open the file \"%s\"\n", filename);
			return 2;
		}

		// Recurse on the filesystem, counting up the sizes of all of the files stored in it.
		dataSize = CalcDirSize (fs);
		fs->Release ();
		fs = NULL;
	}

	// Next, display the Win32 file information about the given filename.
	HANDLE hf = CreateFile (filename, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (!hf)
	{
		printf ("Failed to create Win32 file for \"%s\".\n", filename);
		return 3;
	}

	win32Size = GetFileSize (hf, NULL);
	CloseHandle (hf);


	printf ("UTF Data Size = %d bytes\n", dataSize);
	printf ("Win32 File Size = %d bytes\n", win32Size);
	printf ("Percentage fat (win32Size - dataSize)/(win32Size) = %f\n", (float)100.0f * (float)(win32Size - dataSize)/(float)win32Size);

	return 0;
}
