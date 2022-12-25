// StringUtils.cpp
//
//---------------------------------------------------------------------------
#include "PCH.h"
#include <stdlib.h>

#include "StringUtils.h"
//---------------------------------------------------------------------------
ROS::ROSString GetFilePath(const ROS::ROSString& fullPath)
{
	char	drive[_MAX_DRIVE];
    char	dir[_MAX_DIR];
    char	file[_MAX_FNAME];
    char	ext[_MAX_EXT];

	_splitpath(fullPath.c_str(), drive, dir, file, ext);

	char	path[_MAX_PATH];

	_makepath(path, drive, dir, NULL, NULL);

	return ROS::ROSString(path);
}
//---------------------------------------------------------------------------
ROS::ROSString GetFileName(const ROS::ROSString& fullPath)
{
	char	drive[_MAX_DRIVE];
    char	dir[_MAX_DIR];
    char	file[_MAX_FNAME];
    char	ext[_MAX_EXT];

	_splitpath(fullPath.c_str(), drive, dir, file, ext);

	return ROS::ROSString(file);
}
//---------------------------------------------------------------------------
ROS::ROSString GetFileExtension(const ROS::ROSString& fullPath)
{
	char	drive[_MAX_DRIVE];
    char	dir[_MAX_DIR];
    char	file[_MAX_FNAME];
    char	ext[_MAX_EXT];

	_splitpath(fullPath.c_str(), drive, dir, file, ext);

	return ROS::ROSString(ext);
}
//---------------------------------------------------------------------------
