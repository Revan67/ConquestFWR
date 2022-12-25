//
//
//

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include "FileSys.h"
#include "persistanim.h"
#include "TSmartPointer.h"
#include "heapobj.h"

//

ICOManager * DACOM = NULL;

//

#define MAX_SRC_FILES	1024

//

void InitHeap (void)
{
	// note: for IHeap to work efficiently we need to allocate ONE heap
	// large enough to handle all ENGINE memory allocations for the game
	COMPTR<IHeap> heap;
	DAHEAPDESC desc;
	desc.heapSize = 20 * 1024 * 1024;
	// note: flag NOMSGS saves 8 bytes per alloc
	desc.flags = DAHEAPFLAG_GROWHEAP|DAHEAPFLAG_NOMSGS;
	desc.growSize = 4 * 1024 * 1024;

	HEAP_Acquire ()->CreateInstance (&desc, heap);
}

void DebugPrint (char *fmt, ...)
{
	if (fmt)
	{
		char work[256];

		va_list va;
		va_start(va,fmt);
		vsprintf(work,fmt,va);
		va_end(va);

		OutputDebugString(work);
	}
}

//

IFileSystem * CreateFileSystem(IFileSystem * parent, const char * filename, bool write = false)
{
	IFileSystem * result;
	DAFILEDESC desc = filename;
	if (write)
	{
		desc.lpImplementation = "UTF";
		desc.dwDesiredAccess = GENERIC_READ | GENERIC_WRITE;
		desc.dwCreationDistribution = OPEN_EXISTING;
		desc.dwShareMode = 0;
	}

	if (parent)
	{
		parent->CreateInstance(&desc, (void **) &result);
	}
	else
	{
		DACOM->CreateInstance(&desc, (void **) &result);
	}
	return result;
}

IFileSystem * CreateFileSystemRO(IFileSystem * parent, const char * filename, bool write = false)
{
	IFileSystem * result;
	DAFILEDESC desc = filename;
	if (write)
	{
		desc.lpImplementation = "UTF";
		desc.dwDesiredAccess = GENERIC_READ;
		desc.dwCreationDistribution = OPEN_EXISTING;
		desc.dwShareMode = 0;
	}

	if (parent)
	{
		parent->CreateInstance(&desc, (void **) &result);
	}
	else
	{
		DACOM->CreateInstance(&desc, (void **) &result);
	}
	return result;
}

//

IFileSystem * CreateFileSystemWO(IFileSystem * parent, const char * filename, bool write = false)
{
	IFileSystem * result;
	DAFILEDESC desc = filename;
	if (write)
	{
		desc.lpImplementation = "UTF";
		desc.dwDesiredAccess = GENERIC_WRITE;
		desc.dwCreationDistribution = OPEN_EXISTING;
		desc.dwShareMode = 0;
	}

	if (parent)
	{
		parent->CreateInstance(&desc, (void **) &result);
	}
	else
	{
		DACOM->CreateInstance(&desc, (void **) &result);
	}
	return result;
}

//

bool LoadChild(void *& buffer, IFileSystem * parent, const char * child_name)
{
	bool result = false;
	if (parent)
	{
		DAFILEDESC desc = child_name;
		HANDLE h = parent->OpenChild(&desc);
		if (h != INVALID_HANDLE_VALUE)
		{
			DWORD size = parent->GetFileSize(h, NULL);
			buffer = malloc(size);
			DWORD bytes_read;
			parent->ReadFile(h, buffer, size, &bytes_read);
			if (bytes_read == size)
			{
				result = true;
			}
			else
			{
				free(buffer);
				buffer = NULL;
			}
			parent->CloseHandle(h);
		}
	}
	return result;
}

//

int skip_blank_lines_and_comments(FILE * file)
{
	int c = fgetc(file);

	if (!feof(file))
	{
		bool done = false;
		while (!done)
		{
			while (!feof(file) && (isspace(c) || iscntrl(c)))
			{
				c = fgetc(file);
			}

			if (c == ';')
			{
				while (!feof(file) && (c != '\n'))
				{
					c = fgetc(file);
				}
			}
			else
			{
				done = true;
			}
		}
	}

	return c;
}

//

int read_line(char * dst, FILE * file)
{
	int cnt = 0;

	int c = skip_blank_lines_and_comments(file);
	if (c != EOF)
	{
		int last_valid = -1;
		bool comment = false;

		while (!feof(file) && (c != '\n'))
		{
			if (c == ';')
			{
				comment = true;
			}

			if (!comment)
			{
				if (!isspace(c))
				{
					last_valid = cnt;
				}

				dst[cnt++] = c;
			}

			c = fgetc(file);
		}

		dst[last_valid+1] = 0;
	}

	return cnt;
}

//

int ParseResponseFile(char destfile[], char srcfile[][_MAX_PATH], const char * responsefile)
{
	int result = 0;

	printf("\nParsing response file");

	FILE * rf = fopen(responsefile, "rt");
	if (rf)
	{
		if (read_line(destfile, rf))
		{
			char buffer[_MAX_PATH];

			int len;
			do
			{

				len = read_line(buffer, rf);

				if (len)
				{
					strcpy(srcfile[result++], buffer);
				}

				printf(".");

			} while (len > 0);
		}

		fclose(rf);
	}
	else
	{
		printf("ERROR: Unable to open response file '%s'.\n", responsefile);
	}

	printf("\n");

	return result;
}

//

int ParseCommandLine(char destfile[], char srcfile[][_MAX_PATH], int argc, char * argv[])
{
	int result;

	if (argv[1][0] == '@')
	{
	// Reponse file.
		result = ParseResponseFile(destfile, srcfile, &argv[1][1]);
	}
	else
	{
		strcpy(destfile, argv[1]);
		for (int i = 2; i < argc; i++)
		{
			strcpy(srcfile[i-2], argv[i]);
		}

		result = argc - 2;
	}

	return result;
}

//

void CopyFile (IFileSystem* src, IFileSystem* dst, const char* name)
{
	DAFILEDESC desc = name;
	HANDLE hsrc = src->OpenChild(&desc);
	assert (hsrc != INVALID_HANDLE_VALUE);
	{
		desc.dwDesiredAccess = GENERIC_WRITE;
		desc.dwCreationDistribution = CREATE_ALWAYS;
		desc.dwShareMode = 0;
		HANDLE hdst = dst->OpenChild(&desc);
		assert (hdst != INVALID_HANDLE_VALUE);
		{
			DWORD size = src->GetFileSize(hsrc);
			unsigned char * buffer = (unsigned char *) malloc(size);
			assert(buffer);

			DWORD bytes;
			src->ReadFile(hsrc, buffer, size, &bytes);
			src->CloseHandle (hsrc);
			dst->WriteFile(hdst, buffer, size, &bytes);
			dst->CloseHandle (hdst);

			free(buffer);
		}
	}
}

bool CopyDirectory(IFileSystem * src, IFileSystem * dst, const char * name, const char * parent_name)
{
	bool result = false;
	if (src->SetCurrentDirectory(name))
	{
	// set directory, or create & set directory
		if (dst->SetCurrentDirectory(name) || (dst->CreateDirectory(name) && dst->SetCurrentDirectory(name)))
		{
			WIN32_FIND_DATA fd;
			HANDLE h = src->FindFirstFile("*.*", &fd);
			if (h != INVALID_HANDLE_VALUE)
			{
				do
				{
					if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
					{
						char FullPath[256];
						strcpy(FullPath, parent_name);
						strcat(FullPath, name);
						strcat(FullPath, "\\");
						CopyDirectory(src, dst, fd.cFileName, FullPath);
					}
					else
						CopyFile (src, dst, fd.cFileName);
				} while (src->FindNextFile(h, &fd));

				src->FindClose(h);
			}

			dst->SetCurrentDirectory("..");
		}

		src->SetCurrentDirectory("..");
	}

	return result;
}

//

void CopyChannel (IFileSystem* scripts, IFileSystem* chnls, IFileSystem* dst)
{
	DAFILEDESC desc (PersistAnimChannelFilename);
	HANDLE handle = scripts->OpenChild (&desc);

	if (INVALID_HANDLE_VALUE != handle)
	{
		DWORD fsize = scripts->GetFileSize (handle, NULL);
		assert (fsize < PersistAnimCHANNEL_NAME_MAX);

		DWORD read_size;

		char channel_name[PersistAnimCHANNEL_NAME_MAX];

		scripts->ReadFile (handle, channel_name, fsize, &read_size);
		assert (read_size == fsize);

		scripts->CloseHandle (handle);

		chnls->SetCurrentDirectory (channel_name);
	
		WIN32_FIND_DATA fd;
		HANDLE h = chnls->FindFirstFile("*.*", &fd);
		if (h != INVALID_HANDLE_VALUE)
		{
			do
			{
				if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
					CopyDirectory (chnls, dst, fd.cFileName, NULL);
				else
					CopyFile (chnls, dst, fd.cFileName);
			}
			while (chnls->FindNextFile (h, &fd));

			chnls->CloseHandle (h);
		}

		chnls->SetCurrentDirectory ("..");
	}
}

void CopyAndInterleaveScripts(IFileSystem * src, IFileSystem * dst)
{
	IFileSystem* scripts = CreateFileSystemRO (src, "Script");

	if (!scripts)
		return;

	IFileSystem* chnls = CreateFileSystemRO (src, "Chnl");

	if (!chnls)
	{	
		scripts->Release ();
		return;
	}

	WIN32_FIND_DATA fd;
	HANDLE h = scripts->FindFirstFile("*.*", &fd);

	if (INVALID_HANDLE_VALUE != h)
	{
		do
		{
			scripts->SetCurrentDirectory (fd.cFileName);

		// set directory, or create & set directory
			if (dst->SetCurrentDirectory(fd.cFileName) || (dst->CreateDirectory(fd.cFileName) && dst->SetCurrentDirectory(fd.cFileName)))
			{
				WIN32_FIND_DATA fds;
				HANDLE hs = scripts->FindFirstFile ("*.*", &fds);

				if (INVALID_HANDLE_VALUE != hs)
				{
					do
					{
						if (fds.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
						{
							scripts->SetCurrentDirectory (fds.cFileName);

							if (!strncmp (fds.cFileName, PersistAnimJointMapStem, strlen (PersistAnimJointMapStem)))
							{
								if (!dst->SetCurrentDirectory (fds.cFileName))
								{
									dst->CreateDirectory (fds.cFileName);
									dst->SetCurrentDirectory (fds.cFileName);
								}

								CopyFile (scripts, dst, PersistAnimParentFilename);
								CopyFile (scripts, dst, PersistAnimChildFilename);

								if (!dst->SetCurrentDirectory (PersistAnimChannelData))
								{
									dst->CreateDirectory (PersistAnimChannelData);
									dst->SetCurrentDirectory (PersistAnimChannelData);
								}

								CopyChannel (scripts, chnls, dst);

								dst->SetCurrentDirectory ("..");

								dst->SetCurrentDirectory ("..");
							}
							else if (!strncmp (fds.cFileName, PersistAnimObjectMapStem, strlen (PersistAnimObjectMapStem)))
							{
								if (!dst->SetCurrentDirectory (fds.cFileName))
								{
									dst->CreateDirectory (fds.cFileName);
									dst->SetCurrentDirectory (fds.cFileName);
								}

								CopyFile (scripts, dst, PersistAnimParentFilename);

								if (!dst->SetCurrentDirectory (PersistAnimChannelData))
								{
									dst->CreateDirectory (PersistAnimChannelData);
									dst->SetCurrentDirectory (PersistAnimChannelData);
								}

								CopyChannel (scripts, chnls, dst);

								dst->SetCurrentDirectory ("..");

								dst->SetCurrentDirectory ("..");
							}
							else if (!strncmp (fds.cFileName, PersistAnimEventMapStem, strlen (PersistAnimEventMapStem)))
							{
								if (!dst->SetCurrentDirectory (fds.cFileName))
								{
									dst->CreateDirectory (fds.cFileName);
									dst->SetCurrentDirectory (fds.cFileName);
								}

								if (!dst->SetCurrentDirectory (PersistAnimChannelData))
								{
									dst->CreateDirectory (PersistAnimChannelData);
									dst->SetCurrentDirectory (PersistAnimChannelData);
								}

								CopyChannel (scripts, chnls, dst);

								dst->SetCurrentDirectory ("..");
								dst->SetCurrentDirectory ("..");
							}
							else
								CopyDirectory (scripts, dst, fds.cFileName, NULL);

							scripts->SetCurrentDirectory ("..");
						}
						else
							CopyFile (scripts, dst, fds.cFileName);
					}
					while (scripts->FindNextFile (hs, &fds));

					scripts->FindClose (hs);
				}

				dst->SetCurrentDirectory("..");
			}

			scripts->SetCurrentDirectory ("..");
		}
		while (scripts->FindNextFile(h, &fd));

		scripts->FindClose (h);
	}

	scripts->Release ();
	chnls->Release ();
}

//

void DoMerge(IFileSystem * dest, const char * srcfile, bool & skeleton_named)
{
	IFileSystem * src = CreateFileSystemRO(NULL, srcfile);
	if (src)
	{
		printf("Reading source file '%s'\n", srcfile);

		if (!skeleton_named)
		{
			if (src->SetCurrentDirectory("Skeleton"))
			{
				DAFILEDESC desc = "Name";
				HANDLE h = src->OpenChild(&desc);
				if (h != INVALID_HANDLE_VALUE)
				{
					if (dest->SetCurrentDirectory("Skeleton"))
					{
						char name[256];
						DWORD high;
						DWORD size = src->GetFileSize(h, &high);

						DWORD bytes;
						src->ReadFile(h, name, size, &bytes);

						desc.dwDesiredAccess = GENERIC_WRITE;
						desc.dwCreationDistribution = CREATE_NEW;
						desc.dwShareMode = 0;
						HANDLE hd = dest->OpenChild(&desc);
						if (hd != INVALID_HANDLE_VALUE)
						{
							dest->WriteFile(hd, name, bytes, &size);
							dest->CloseHandle(hd);
							skeleton_named = true;
						}
						dest->SetCurrentDirectory("..");
					}
					src->CloseHandle(h);
				}

				src->SetCurrentDirectory("..");
			}
		}

	// Copy channels, scripts.

		//EMAURER setup destination then study src fmt to see if old or new.
		if (dest->SetCurrentDirectory("Animation"))
		{
			if (src->SetCurrentDirectory("Animation"))
			{
				//EMAURER which format is the src? new fmt omits "Chnl"
				if (src->SetCurrentDirectory ("Chnl"))
				{
					//EMAURER old fmt
					src->SetCurrentDirectory("..");

					//CopyAndInterleave... expects destination to be in 
					//"Script" directory already.
					dest->SetCurrentDirectory ("Script");
					CopyAndInterleaveScripts (src, dest);
					dest->SetCurrentDirectory("..");
				}
				else
				{
					//EMAURER src in new fmt. only need to copy the "Script" directory.
					CopyDirectory(src, dest, "Script", "ROOT\\Animation\\");
				}

				src->SetCurrentDirectory("..");
			}

			dest->SetCurrentDirectory ("..");
		}

		src->Release();
	}
	else
	{
		printf("Unable to open source file '%s'.\n", srcfile);
	}

}

//

char INI[] = "[Libraries]\ndosfile.dll\n";

//

int main(int argc, char * argv[])
{
	if (argc < 2)
	{
		printf("Usage: mergeanm <destination> <src1> <src2> ...\n");
		printf("       mergeanm @<response_file>\n");
		return 1;
	}

	DACOM = DACOM_Acquire();
	if (!DACOM)
	{
		return 1;
	}

	InitHeap ();

	DACOM->SetINIConfig(INI, DACOM_INI_STRING);

	char destfile[_MAX_PATH];
	char srcfiles[MAX_SRC_FILES][_MAX_PATH];


	int result;

	int num_srcfiles = ParseCommandLine(destfile, srcfiles, argc, argv);
	if (num_srcfiles)
	{
		bool skeleton_named;

		IFileSystem * dest = CreateFileSystemWO(NULL, destfile, true);
		if (dest)
		{
			skeleton_named = true;
		}
		else
		{
			skeleton_named = false;
		//
		// Destination file doesn't exist, must create it.
		//
			printf("Destination file '%s' doesn't exist, creating.\n", destfile);

			DAFILEDESC desc = destfile;
			desc.lpImplementation = "UTF";
			desc.dwDesiredAccess = GENERIC_WRITE;
			desc.dwCreationDistribution = CREATE_NEW;
			desc.dwShareMode = 0;
			if (DACOM->CreateInstance(&desc, (void **) &dest) != GR_OK)
			{
				printf("Unable to create destination file '%s'.\n", destfile);			
				goto __exit;
			}

			if (dest->CreateDirectory("Animation"))
			{
				dest->SetCurrentDirectory("Animation");
				dest->CreateDirectory("Script");
				dest->SetCurrentDirectory("..");
			}
			else
			{
				printf("Unable to create 'Animation' directory in file '%s'.\n", destfile);
				dest->Release();
				goto __exit;
			}
			if (!dest->CreateDirectory("Skeleton"))
			{
				printf("Unable to create 'Skeleton' directory in file '%s'.\n", destfile);
				dest->Release();
				goto __exit;
			}
		}

	//
	// Now start copying stuff.
	//
		for (int i = 0; i < num_srcfiles; i++)
		{
			if (strchr(srcfiles[i], '*'))
			{
				WIN32_FIND_DATA fd;
				HANDLE h = FindFirstFile(srcfiles[i], &fd);
				if (h != INVALID_HANDLE_VALUE)
				{
					do 
					{
						DoMerge(dest, fd.cFileName, skeleton_named);
					} while (FindNextFile(h, &fd));

					FindClose(h);
				}
			}
			else
			{
				DoMerge(dest, srcfiles[i], skeleton_named);
			}
		}

		dest->Release();
		result = 0;
	}
	else
	{
		result = 1;
	}

__exit:

	DACOM->Release();

	return result;
}