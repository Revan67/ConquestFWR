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
		int bytes;

		if (buffer)
		{
			bytes	= len;
			result	= buffer;
		}
		else
		{
		//
		// Determine file size, allocate buffer.
		//
			bytes	= parent->GetFileSize(h, NULL);
			result	= malloc(bytes);
		}

		DWORD bytes_read;
		parent->ReadFile(h, result, bytes, &bytes_read, 0);
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

#endif
