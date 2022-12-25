//
// <fileutil.h> - file utilities
//

#ifndef FILEUTIL_H
#define FILEUTIL_H

#include "main.h"
#include "filesys.h"

BOOL32 open_file_dialog(C8 *full_path_buffer);
BOOL32 save_as_file_dialog(C8 *full_path_buffer);

extern C8   copy_path[256];
extern C8   copy_file_list[256][256];
extern S32  copy_file_count;

BOOL32 copy_file_dialog();

void DeleteDirectory(IFileSystem * fs, const char * name);

//
// Creates a file system. App must ->Release() it when finished.
//

inline IFileSystem * OpenDirectory(const char * name, IFileSystem * parent = NULL)
{
	IFileSystem * result;

	DAFILEDESC desc = name;
    desc.dwDesiredAccess = GENERIC_READ | GENERIC_WRITE;

	if (parent)
	{
		parent->CreateInstance(&desc, (void **) &result);
	}
	else
	{
		DACOM_Acquire()->CreateInstance(&desc, (void **) &result);
	}
	
	return result;
}

//
// Reads entire file into memory. Allocates mem if necessary.
//
inline void * LoadFile(const char * name, void * buffer, int len, IFileSystem * parent)
{
	void * result;

	DAFILEDESC desc = name;

	HANDLE h = parent->OpenChild(&desc);
	
    if (h != INVALID_HANDLE_VALUE)
	{
		DWORD bytes_read;

        parent->ReadFile(h, buffer, len, &bytes_read, 0);
		parent->CloseHandle(h);
	}
	else
	{
		DWORD err = parent->GetLastError();
		result = NULL;
	}

	return result;
}

//

inline BOOL32 SaveFile(const char * name, void * buffer, int len, IFileSystem * parent)
{
    DAFILEDESC  desc            = name;
    desc.dwDesiredAccess        = GENERIC_READ | GENERIC_WRITE;
    desc.dwCreationDistribution	= CREATE_ALWAYS;    

    HANDLE      h       = parent->OpenChild(&desc);

    if (h != INVALID_HANDLE_VALUE)
    {
        DWORD bytes_written;

        if (!parent->WriteFile(h, buffer, len, &bytes_written, 0))
            __asm int 0x03;

        parent->CloseHandle(h);
    }
    else
    {
        DWORD err = parent->GetLastError();
        return FALSE;
    }

    return TRUE;
}       

#endif

