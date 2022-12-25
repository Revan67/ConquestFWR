//
//
//

#ifndef FILEUTIL_H
#define FILEUTIL_H

//

#include <stdlib.h>
#include "filesys.h"

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
    DAFILEDESC  desc			= name;
	desc.dwCreationDistribution = OPEN_ALWAYS;
    desc.dwDesiredAccess		= GENERIC_WRITE;
    
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
