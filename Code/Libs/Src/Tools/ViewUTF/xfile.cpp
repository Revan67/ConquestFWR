//---------------------------------------------------------------------------
//
// xFile.CPP
//
// Extensions to the IFileSystem interface
//
//---------------------------------------------------------------------------

#include "stdafx.h"

#include "xFile.h"
#include "document.h"

//---------------------------------------------------------------------------

char *GetImplementation (char *filename)
{
	static char result[4];

	result[0] = 0;

	char *ptr = strrchr(filename, '.');

	if (ptr != 0)
	{
		if (strchr(result, '\\'))
			result[0] = 0;
		else
		{
			strncpy(result,ptr+1,4);	// prevent .UTF~tmp
			result[3] = 0;

			strupr(result);
		}
	}

	return result;
}

//---------------------------------------------------------------------------

IFileSystem *FS_Create (DAFILEDESC *desc, IComponentFactory *parent)
{
	IFileSystem *file = 0;
	int err = 0;

	desc->lpImplementation = GetImplementation((char *)desc->lpFileName); // "UTF"

	desc->dwCreationDistribution = CREATE_ALWAYS;
	desc->dwDesiredAccess = GENERIC_READ|GENERIC_WRITE;
	desc->dwShareMode = FILE_SHARE_READ;

	if (parent == NULL)
		parent = DACOM_Acquire();	// Note: does not need to be released

	if (parent)
	{
		err = parent->CreateInstance(desc, (void **)&file);
	}

	return file;
}

//---------------------------------------------------------------------------

IFileSystem *FS_Open (DAFILEDESC *desc, const char *mode, IComponentFactory *parent)
{
	IFileSystem *file = 0;
	int err = 0;

	if (mode)
	while (*mode)
	{
		switch (*mode)
		{
			case 'r':
				desc->dwCreationDistribution = OPEN_EXISTING;
				desc->dwDesiredAccess = GENERIC_READ;
				desc->dwShareMode = FILE_SHARE_READ|FILE_SHARE_WRITE;
			break;

			case 'w':
				desc->dwCreationDistribution = OPEN_ALWAYS;
				desc->dwDesiredAccess = GENERIC_READ|GENERIC_WRITE;
				desc->dwShareMode = FILE_SHARE_READ;
			break;

			case 's':
				desc->dwShareMode = FILE_SHARE_READ;
			break;
		}
		mode += 1;
	}

	if (parent == NULL)
		parent = DACOM_Acquire();

	if (parent)
	{
		err = parent->CreateInstance(desc, (void **)&file);
	}

	return file;
}

//---------------------------------------------------------------------------

BOOL FS_Delete (const char *name, IFileSystem *parent)
{
	BOOL ok = false;

	if (parent == 0)
	{
		CString path = FileName::get_path(name);
		CString src = FileName::get_name(name);

		DAFILEDESC desc(path);
		desc.dwDesiredAccess = GENERIC_READ|GENERIC_WRITE;

		IComponentFactory *DACOM = DACOM_Acquire();
		int err = DACOM->CreateInstance(&desc,(void**)&parent);
		if (err == GR_OK)
		{
			ok = parent->DeleteFile(src);

			parent->Release();
			parent = 0;
		}
	}
	else
	{
		ok = parent->DeleteFile(name);
	}
	
	return ok;
}

//---------------------------------------------------------------------------

void FS_TempName (const char *name, CString &new_name, IFileSystem *parent)
{
	if (new_name.IsEmpty())
	{
		int index = 1;
		do
		{
			new_name.Format("%s~tmp%d",name,index++);
		}
		while ((int)parent->GetFileAttributes(new_name) >= 0);
	}
}

BOOL FS_Copy (const char *name, CString &new_name, IFileSystem *parent)
{
	int ok = 0;

	if (parent)
	{
		FS_TempName(name,new_name,parent);

		ok = parent->CopyFile(name,new_name,0);
	}
	else
	{
		CString p1,f1;
		CString p2,f2;
		
		p1 = FileName::get_path(name);
		f1 = FileName::get_name(name);

		DAFILEDESC desc(p1);
		desc.dwDesiredAccess = GENERIC_READ|GENERIC_WRITE;
		IComponentFactory *DACOM = DACOM_Acquire();
		int err = DACOM->CreateInstance(&desc,(void**)&parent);
		DACOM->Release();

		if (new_name.IsEmpty())
		{
			p2 = p1;
//			f2 = f1 + "~tmp";

			f2.Empty();
			FS_TempName(f1,f2,parent);

			new_name = p2 + f2;
		}
		else
		{
			p2 = FileName::get_path(new_name);
			f2 = FileName::get_name(new_name);
		}

		ASSERT(p1 == p2);	// don't know how to handle different FileSystem's yet!

		ok = parent->CopyFile(f1,f2,0);

		parent->Release();
		parent = 0;
	}
	
	return ok;
}

//---------------------------------------------------------------------------

int CountFiles (IFileSystem *sys)
{
    int count = 0;

    if (sys)
    {
	WIN32_FIND_DATA data;
	HANDLE handle;

	handle = sys->FindFirstFile("*.*", &data);

    if (handle != INVALID_HANDLE_VALUE) // is directory empty?
	do
	{
		if (data.cFileName[0] != '.' || strchr(data.cFileName, '\\') != 0)
        {
            count += 1;

			DAFILEDESC desc(data.cFileName);
	        IFileSystem *f = FS_Open(&desc,"r",sys);
			if (f)
			{
				count += CountFiles(f);
				f->Release();
			}
        }
    }
    while (sys->FindNextFile(handle, &data));

	sys->FindClose(handle);
    }

    return (count);
}

//---------------------------------------------------------------------------

IFileSystem *FS_Rename (IFileSystem *src, const char *dst)
{
	IFileSystem *result = 0;

	DOCDESC desc;
	desc.lpFileName = dst;
	desc.dwCreationDistribution = OPEN_EXISTING;
	desc.dwDesiredAccess = src->GetAccessType();
	desc.dwShareMode = FILE_SHARE_READ|FILE_SHARE_WRITE;

	IFileSystem *parent = 0;
	src->GetParentSystem(&parent);

	char old_name[128];
	src->GetFileName(old_name,sizeof(old_name));
	src->Release();
	src = 0;

	if (parent)
	{
		int ok = parent->MoveFile(old_name,dst);
		if (ok)
		{
			parent->CreateInstance(&desc,(void **)&result);
		}

		parent->Release();
	}

	return result;
}

//---------------------------------------------------------------------------

// just thinking....

struct FileHandle
{
	IComponentFactory *parent;
	IFileSystem *sys;
	const char *name;

	FileHandle (const char *_name)
	{
		parent = DACOM_Acquire();
		sys = 0;
		name = _name;
	}

	FileHandle (IFileSystem *_sys, const char *_name)
	{
		parent = 0;
		sys = _sys;
		name = _name;
	}
};

//---------------------------------------------------------------------------
