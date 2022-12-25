//
// verutil.h
//

#ifndef __VERUTIL_H
#define __VERUTIL_H

#include <time.h>

BOOL EBUGetFileTimeAndSize( LPCSTR lpszFile, time_t *pFileTime, DWORD *pFileSize);

//*** Return values for FileInfo
//
#define FI_ERR_NOEXIST		0x0001	// file doesn't exist
#define FI_ERR_CANTOPEN		0x0002	// file couldn't be opened
#define FI_ERR_NOMEMORY		0x0004	// out of memory error occurred
#define FI_ERRORMASK		0x0007	// how to determine if an error occurred
#define FI_VER_NONE			0x0010	// file has no version info
#define FI_LANG_NONE		0x0020	// file has no language info

typedef struct tagFILEINFO
{
	DWORD		dwFileVersionMS;	//from VS_FIXEDFILEINFO
	DWORD		dwFileVersionLS;	//from VS_FIXEDFILEINFO
	DWORD		dwLanguage;			//language ID + character set
	DWORD		dwFileSize;			//size in bytes of the file
	time_t	    FileTime;			//timestamp of the file
	char		szName[1];			//filename, used only in resource file
} FILEINFO, FAR* LPFILEINFO;

UINT EBUFileInfo(LPCSTR lpszFile, LPFILEINFO pFileInfo);

#endif //__VERUTIL_H
