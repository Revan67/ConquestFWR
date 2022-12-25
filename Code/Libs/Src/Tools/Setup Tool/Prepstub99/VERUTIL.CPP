//
// verutil.cpp
//
//		Version utility functions
//
// History:
//
//		 1/26/95	KenSh		Created
//

#include "prepstub.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include "verutil.h"
#include "util.h"
#include <tchar.h>


//****************************************************************************
// Procedure	EBUGetFileTimeAndSize
//
// Purpose		Fills the MYFILETIME structure with the file's time of last
//				modification.  The low bit of the seconds field is lost.
//
// Parameters	lpszFile		full pathname of the file to examine
//				pFileTime		structure to fill with the file's time info
//				lpFileSize		the file's size gets stuck here (hack)
//
// Returns		nonzero if successful, zero if the file couldn't be opened
//				for reading.
//
// History		 1/27/95	KenSh		Created
//				 8/29/95	KenSh		Use localtime instead of gmtime
//
static BOOL EBUGetFileTimeAndSize(LPCSTR lpszFile, time_t *pFileTime, DWORD *pdwFileSize)
{
	struct _tstat ss;
	HANDLE hFile;
	DWORD  dwBytesRead;
	BYTE   rgbCompInfo[14];

	//
	//Get file time and on disk file size...
	//
	if (0 != _tstat(lpszFile, &ss))
	{
		return FALSE;
	}

	//
	//Open file and read first sizeof(rgbCompInfo) bytes so we can check to see if it's SZDD compressed...
	//
	hFile = CreateFile(lpszFile,
					   GENERIC_READ,
					   FILE_SHARE_READ,
					   NULL,
					   OPEN_EXISTING,
					   FILE_ATTRIBUTE_NORMAL, 
					   NULL);
		
	if(hFile == INVALID_HANDLE_VALUE)
	{
		return FALSE;
	}

	ReadFile(hFile, (LPVOID) &rgbCompInfo, sizeof(rgbCompInfo), &dwBytesRead, NULL);
	CloseHandle(hFile);

	//
	//At this point, rgbCompInfo looks like:
	//	00-03:	SZDD
	//  04-07:  Remainder of compression type signature (we can ignore this...)
	//     08:  Compression algorithm type
	//     09:  Last character of original extension if "-r" switch used with compress.exe
	//  10-13:  ULONG of uncompressed file size...
	//

	//
	//Terminate comp type string...
	//
	rgbCompInfo[4] = '\0';

	//
	//Use _tstat() file size if not SZDD file, else use uncompressed size...
	//
	*pdwFileSize = lstrcmp((LPCSTR) rgbCompInfo, "SZDD") ? 
				   (DWORD) ss.st_size : 
				   (DWORD) *((DWORD *) &rgbCompInfo[10]);

	//
	//Save file date/time...
	//
	*pFileTime = ss.st_mtime;
	
	return TRUE;
}


//****************************************************************************
// Procedure	EBUFileInfo
//
// Purpose		Retrieves the version, language info, and timestamp (last
//				modification time) for a file.
//
// Parameters	lpszFile		the file to examine
//				pFileInfo		where to store our information
//
// Returns		Zero or more of these values OR'd together:
//
//				FI_ERR_NOEXIST		File does not exist
//				FI_ERR_CANTOPEN		File could not be opened for reading
//				FI_ERR_NOMEMORY		Not enough memory to get version info
//				FI_VER_NONE			File does not have version info
//				FI_LANG_NONE		File does not have language info
//
//				The pFileInfo structure is also filled in.  If there is no
//				language and/or version information, these fields in the
//				FILEINFO struct are guaranteed to be set to zero.
//
// History		27-Jan-95	KenSh		Created
//
UINT EBUFileInfo( LPCSTR lpszFile, LPFILEINFO pFileInfo )
{
	pFileInfo->dwFileVersionMS = 0L;
	pFileInfo->dwFileVersionLS = 0L;
	pFileInfo->dwLanguage = 0L;

	DWORD 	dwVerSize;
	DWORD 	dwVerHandle;
	LPVOID	lpVerBuffer;
	LPVOID	lpVerData;				//data pointer set by VerQueryValue
	UINT	cbVerData;				//size of data stored in lpVerData1
	UINT	uResult = 0;

	dwVerSize = GetFileVersionInfoSize( (LPSTR)lpszFile, &dwVerHandle );

	if( !dwVerSize )
	{
		if(!DoesFileExist(lpszFile))
		{
			return FI_ERR_NOEXIST;
		}
		else
		{
			uResult = FI_VER_NONE | FI_LANG_NONE;
		}
	}
	else
	{
		lpVerBuffer = (LPVOID) malloc(dwVerSize);

		if( !lpVerBuffer )
		{
			return FI_ERR_NOMEMORY;
		}

		if( !GetFileVersionInfo( (LPSTR)lpszFile, dwVerHandle, dwVerSize, lpVerBuffer ) )
		{
			uResult = FI_VER_NONE | FI_LANG_NONE;
		}
		else
		{
			if( !VerQueryValue( lpVerBuffer, "\\", &lpVerData, &cbVerData ) )
			{
				uResult = FI_VER_NONE | FI_LANG_NONE;
			}
			else
			{
				#define pVerFixedInfo ((VS_FIXEDFILEINFO FAR*)lpVerData)

				pFileInfo->dwFileVersionMS = pVerFixedInfo->dwFileVersionMS;
				pFileInfo->dwFileVersionLS = pVerFixedInfo->dwFileVersionLS;

				#undef pVerFixedInfo

				// Grab the language info.
				if( !VerQueryValue( lpVerBuffer, "\\VarFileInfo\\Translation",
									&lpVerData, &cbVerData ) )
				{
					uResult |= FI_LANG_NONE;
				}
				else
				{
					pFileInfo->dwLanguage = *(DWORD FAR*)lpVerData;
				}
			}
		}

		free(lpVerBuffer);
	}

	// Get file date/time and size
	if (!EBUGetFileTimeAndSize(lpszFile, &pFileInfo->FileTime, &pFileInfo->dwFileSize))
	{
		return FI_ERR_CANTOPEN;
	}

	return uResult;
}

