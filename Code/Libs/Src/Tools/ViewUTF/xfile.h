//---------------------------------------------------------------------------
//
// xFile.H
//
// Extensions to the IFileSystem interface
//
//---------------------------------------------------------------------------

#ifndef XFILE_H
#define XFILE_H

#include "afx.h" // CString

#include "filesys.h"

//---------------------------------------------------------------------------

struct FileName
{
	static CString get_path (const char *full_name)
	{
		char result[_MAX_PATH]; 

		char drive[_MAX_DRIVE];
		char dir[_MAX_DIR];
		char fname[_MAX_FNAME];
		char ext[_MAX_EXT];

		_splitpath((char *)full_name, drive,dir,fname,ext);
		_makepath((char *)result, drive,dir,0,0);

		return CString(result);
	}

	static CString get_name (const char *full_name)
	{
		char result[_MAX_PATH]; 

		char drive[_MAX_DRIVE];
		char dir[_MAX_DIR];
		char fname[_MAX_FNAME];
		char ext[_MAX_EXT];

		_splitpath((char *)full_name, drive,dir,fname,ext);
		_makepath((char *)result, 0,0,fname,ext);

		return CString(result);
	}
};

//---------------------------------------------------------------------------

char *GetImplementation (char *filename);

IFileSystem *FS_Create (DAFILEDESC *desc, IComponentFactory *parent=0);

IFileSystem *FS_Open (DAFILEDESC *desc, const char *mode=0, IComponentFactory *parent=0);

BOOL FS_Delete (const char *name, IFileSystem *parent=0);

BOOL FS_Copy (const char *name, CString &new_name, IFileSystem *parent=0);

IFileSystem *FS_Rename (IFileSystem *src, const char *dst);

int CountFiles (IFileSystem *sys);

//---------------------------------------------------------------------------

#endif XFILE_H

