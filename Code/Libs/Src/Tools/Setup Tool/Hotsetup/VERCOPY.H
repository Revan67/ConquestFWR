//
// vercopy.h
//

#ifndef __VERCOPY_H
#define __VERCOPY_H

#include "hotsetup.h"

//*** Return values for FileInfo
//
#define FI_ERR_NOEXIST		0x0001	// file doesn't exist
#define FI_ERR_CANTOPEN		0x0002	// file couldn't be opened
#define FI_ERR_NOMEMORY		0x0004	// out of memory error occurred
#define FI_ERRORMASK		0x0007	// how to determine if an error occurred
#define FI_VER_NONE			0x0010	// file has no version info
#define FI_LANG_NONE		0x0020	// file has no language info

//*** Return values for VersionCompare
//
#define VC_ERR_NOEXIST		0x0001	// file does not exist or couldn't be opened
#define VC_ERR_CANTOPEN		0x0002	// file could not be opened for reading
#define VC_ERR_NOMEMORY		0x0004	// not enough memory to complete the operation
#define VC_ERRORMASK		0x0007	// how to check if an error occurred
#define VC_VER_EQUAL		0x0008	// file 1 version == file 2 version
#define VC_VER_NONE			0x0010	// neither file 1 nor file 2 has version info
#define VC_VER_LESS			0x0020	// file 1 version < file 2 version
#define VC_VER_GREATER		0x0040	// file 1 version > file 2 version
#define VC_LANG_EQUAL		0x0080	// file 1 language matches file 2 language
#define VC_LANG_DIFFERENT	0x0100	// file 1 language different from file 2
#define VC_DATE_EQUAL		0x0200	// file 1 timestamp == file 2 timestamp
#define VC_DATE_OLDER		0x0400	// file 1 is older than file 2
#define VC_DATE_NEWER		0x0800	// file 1 is newer than file 2

//*** Return values for system CopyFile API
//
#define CF_ERR_CANTOPENSRC	0x0002	// Source file couldn't be opened
#define CF_ERR_NODESTDIR    0x0003  // Destination directory doesn't exist
#define CF_ERR_CANTOPENDEST	0x0020	// Destination file couldn't be opened
#define CF_ERR_READONLYDEST 0x0005  // Destination file access denied
#define CF_ERR_BADREAD		0x001F	// An error occurred while reading
#define CF_ERR_OUTOFSPACE	0x0070	// An error occurred while writing (out of space)

typedef struct tagFILEINFO
{
	DWORD		dwFileVersionMS;	//from VS_FIXEDFILEINFO
	DWORD		dwFileVersionLS;	//from VS_FIXEDFILEINFO
	DWORD		dwLanguage;			//language ID + character set
	DWORD		dwFileSize;			//size in bytes of the file
	time_t		FileTime;			//timestamp of the file

} FILEINFO, *LPFILEINFO;

UINT EBUFileInfo(LPCSTR lpszFile, LPFILEINFO pFileInfo);
UINT VersionCompare(LPSTR lpszSourceFilePath, LPFILEINFO lpSourceFileInfo, LPSTR lpszDestFilePath, LPFILEINFO lpDestFileInfo, BOOL bUseCached);
EBURETCODE MyCopyFile(LPCSTR lpszSrc, LPCSTR lpszDest, BOOL fBumpRefCount, BOOL fSystemFile, LPFILECOPYSTATUS pfs);

#endif //__VERCOPY_H
