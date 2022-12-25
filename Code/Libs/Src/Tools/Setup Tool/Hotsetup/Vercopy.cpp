//
// vercopy.cpp
//
//		Versioned copy functions
//
// History:
//
//		 1/26/95	KenSh		Created
//		 8/29/95	KenSh		Use localtime instead of gmtime
//		 8/29/95	KenSh		Overwrite read-only files
//       3/28/97    v-richei	Created MyCopyFile...
//

#include "stubpch.h"
#include "hotsetup.h"
#include "util.h"
#include "restart.h"
#include "HotSetupRC.h"
#include "vercopy.h"
#include "lzexpand.h"

#include "Setup.h" // for MyRefCountSharedDll()

static EBURETCODE MyCopyFile2(LPCSTR lpszSrc, LPCSTR lpszDest, BOOL fBumpRefCount, BOOL fSystemFile, LPFILECOPYSTATUS pfs);
BOOL EBUGetFileTimeAndSize(LPCSTR lpszFile, time_t *pFileTime, DWORD *pdwFileSize);

using namespace NGLOBALS;

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
BOOL EBUGetFileTimeAndSize(LPCSTR lpszFile, time_t *pFileTime, DWORD *pdwFileSize)
{
	struct _tstat ss;

	SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX);
	if (0 != _tstat(lpszFile, &ss))
	{	
		SetErrorMode(0);
		return FALSE;
	}
	SetErrorMode(0);

	//
	//Save file date/time...
	//
	*pFileTime = ss.st_mtime;

	//
	//Save file size...
	//
	*pdwFileSize = (DWORD) ss.st_size;

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
UINT EBUFileInfo(LPCSTR lpszFile, LPFILEINFO pFileInfo)
{
	pFileInfo->dwFileVersionMS = 0L;
	pFileInfo->dwFileVersionLS = 0L;
	pFileInfo->dwLanguage = 0L;

	if (GetIgnoreFileInfo())
	{
		return 0;
	}

	DWORD 	dwVerSize;
	DWORD 	dwVerHandle;
	LPVOID	lpVerBuffer;
	LPVOID	lpVerData;				//data pointer set by VerQueryValue
	UINT	cbVerData;				//size of data stored in lpVerData1
	UINT	uResult = 0;

	SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX);
	dwVerSize = GetFileVersionInfoSize( (LPSTR)lpszFile, &dwVerHandle );
	SetErrorMode(0);

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

		SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX);
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
		SetErrorMode(0);

		free(lpVerBuffer);
	}

	// Get file date/time and size
	if (!EBUGetFileTimeAndSize(lpszFile, &pFileInfo->FileTime, &pFileInfo->dwFileSize))
	{
		return FI_ERR_CANTOPEN;
	}

	return uResult;
}

//****************************************************************************
// Procedure	VersionCompare
//
// Purpose		Compares the two given files and determines which is older
//				by looking at version info and file date.  Also looks at
//				language information to see if the files match.  Note that
//				if file1 has no version info but file2 does, file1 will be
//				considered to have a version "less than" that of file2.
//
//				This is an derivative of VersionCompare that checks the source file
//				directly for the information if possible.
//
//				If no version information available for file1 then go to the
//				version info in the lpSourceFileInfo structure
//
// Parameters	lpszFile1		full pathname of the first file to look at
//				lpFileInfo2		Predetermined version info for File2
//				lpFileInfo1		(optional) receives file info for File1
//
// Returns		UINT containing masked flags from these groups:
//
//	  Errors	VC_ERR_NOEXIST		File1 does not exist
//				VC_ERR_CANTOPEN		File1 could not be opened for reading
//				VC_ERR_NOMEMORY		Ran out of memory while computing.
//
//				You can use VC_ERRORMASK to check if any of these are set.
//
//	  Version	VC_VER_EQUAL		Version info matches exactly
//				VC_VER_NONE			Neither file has version info
//				VC_VER_LESS			File1 version < File2 version
//				VC_VER_GREATER		File1 version > File2 version
//
//	  Language	VC_LANG_EQUAL		Both files have matching language info
//				VC_LANG_DIFFERENT	The language info in the files doesn't match
//
//	  Date		VC_DATE_EQUAL		The 2 files have the same timestamp
//				VC_DATE_OLDER		Destination File is older than Source File
//				VC_DATE_NEWER		Destination File is newer than Source File
//
//
// History		 1/26/95	KenSh		Created
//
//				12/09/96	a-dashoe	
//					Copied / Modified VersionCompare to try to check the files
//					information if at possible.
//					If no version information available for file1 then go to the
//					version info in the lpFileInfo1 structure (if any).

UINT VersionCompare(LPSTR lpszSourceFilePath, LPFILEINFO lpSourceFileInfo,
					 LPSTR lpszDestFilePath, LPFILEINFO lpDestFileInfo,
					 BOOL bUseCached)
{
	UINT		uResult = 0;
	UINT		uFileInfoResult1;
	FILEINFO	SourceFileInfo;
	int			nDateCompare;

	ASSERT(lpSourceFileInfo);
	ASSERT(lpDestFileInfo);

	if (!bUseCached || (!lpSourceFileInfo->dwFileVersionMS && !lpSourceFileInfo->dwFileVersionLS))
	{
		// get the file version information for the source file.  This will also update our
		// internal info structure on this file incase the file that is in our source location
		// is different than that which the setup was originaly seeded with.
		uFileInfoResult1 = EBUFileInfo(lpszSourceFilePath, &SourceFileInfo);
		
		if(!(uFileInfoResult1 & FI_ERRORMASK))
		{
			//
			//source info should be passed back to the caller.
			//
			CopyMemory((void *) lpSourceFileInfo, (void *) &SourceFileInfo, sizeof(FILEINFO));
		}
		else if (FI_ERR_NOEXIST == uFileInfoResult1)
		{
			return VC_ERR_NOEXIST;
		}
		else if (FI_ERR_CANTOPEN == uFileInfoResult1)
		{
			return VC_ERR_CANTOPEN;
		}
		else
		{
			ASSERT( FI_ERR_NOMEMORY == uFileInfoResult1 );
			return VC_ERR_NOMEMORY;
		}
	}

	// Load version info for dest
	uFileInfoResult1 = EBUFileInfo(lpszDestFilePath, lpDestFileInfo);
	
	if (uFileInfoResult1 & FI_ERRORMASK)
	{
		if (FI_ERR_NOEXIST == uFileInfoResult1)
		{
			return VC_ERR_NOEXIST;
		}
		else if (FI_ERR_CANTOPEN == uFileInfoResult1)
		{
			return VC_ERR_CANTOPEN;
		}
		else
		{
			ASSERT( FI_ERR_NOMEMORY == uFileInfoResult1 );
			return VC_ERR_NOMEMORY;
		}
	}

	// Compare the version info we've retrieved

	if (!lpDestFileInfo->dwFileVersionMS && !lpDestFileInfo->dwFileVersionLS &&
		!lpSourceFileInfo->dwFileVersionMS && !lpSourceFileInfo->dwFileVersionLS)
	{
		uResult = VC_VER_NONE;
	}
	else
	{
		if( !uResult )
		{
			//
			//Note that we depend on EBUFileInfo() setting the version to zero
			//if there is no version info.
			//
			if (lpDestFileInfo->dwFileVersionMS < lpSourceFileInfo->dwFileVersionMS)
			{
				uResult = VC_VER_LESS;
			}
			else if (lpDestFileInfo->dwFileVersionMS > lpSourceFileInfo->dwFileVersionMS)
			{
				uResult = VC_VER_GREATER;
			}
			else
			{
				if (lpDestFileInfo->dwFileVersionLS < lpSourceFileInfo->dwFileVersionLS)
				{
					uResult = VC_VER_LESS;
				}
				else if (lpDestFileInfo->dwFileVersionLS > lpSourceFileInfo->dwFileVersionLS)
				{
					uResult = VC_VER_GREATER;
				}
				else
				{
					uResult = VC_VER_EQUAL;
				}
			}
		}
	}

	//
	//Compare language information
	//
	if (!(uResult & VC_LANG_EQUAL) &&
		lpDestFileInfo->dwLanguage != lpSourceFileInfo->dwLanguage)
	{
		uResult |= VC_LANG_DIFFERENT;
	}
	else
	{
		//Note that if one file is marked as language-neutral and
		//the other has no language info, they are considered
		//to be the same language.

		uResult |= VC_LANG_EQUAL;
	}

	//
	//Compare file dates/times
	//
	nDateCompare = lpDestFileInfo->FileTime - lpSourceFileInfo->FileTime;

	if (nDateCompare == 0)
	{
		uResult |= VC_DATE_EQUAL;
	}
	else if (nDateCompare < 0)
	{
		uResult |= VC_DATE_OLDER;
	}
	else
	{
		uResult |= VC_DATE_NEWER;
	}

	return uResult;
}

EBURETCODE MyCopyFile(LPCSTR lpszSrc, LPCSTR lpszDest, BOOL fBumpRefCount, BOOL fSystemFile, LPFILECOPYSTATUS pfs)
{
	HANDLE hFile;
	DWORD  BytesRead;
	int    nAlertResult=0;
	char   buf[5];
	
	ZeroMemory(buf, sizeof(buf));
	
	while (TRUE)
	{
		hFile = EBUCreateFile(lpszSrc,GENERIC_READ,FILE_SHARE_READ,NULL,
			OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL, NULL);
		
		if(hFile == INVALID_HANDLE_VALUE)
		{
			// If user told us to abort during a CD Check then do not prompt a second time.
			if (EBU_ABORT == GetResultCode())
				return EBU_ABORT;
			nAlertResult = Alert(GetWndParent(),
				MB_ICONEXCLAMATION | MB_ABORTRETRYIGNORE | MB_DEFBUTTON2,
				STR_ERROR_MISSINGCD,
				(LPCSTR) lpszSrc);
			
			if (IDABORT == nAlertResult)
			{
				if (Alert(GetWndParent(), MB_ICONEXCLAMATION | MB_YESNO | MB_DEFBUTTON2, STR_USERSTOPPEDCOPY) == IDYES)
				{
					return EBU_ABORT;
				}
				else
				{
					nAlertResult = IDRETRY;
				}
			}
			
			if (IDIGNORE == nAlertResult)
			{
				SetCopyIncomplete(TRUE);
				return EBU_OK;
			}
		}
		else
			break;
	}
	
    ASSERT(INVALID_HANDLE_VALUE != hFile);
	
	EBUReadFile(hFile,buf,4,&BytesRead,NULL);
	// If user told us to abort during a CD Check then do not prompt a second time.
	if (EBU_ABORT == GetResultCode())
	{
		CloseHandle(hFile);
		return EBU_ABORT;
	}
	CloseHandle(hFile);
	
	if (4 == BytesRead)
	{
		//
		//We correctly decompress KWAJ and SZDD files as compressed with the 
		//DOS based COMPRESS.EXE tool, version 2.50
		//
		if (lstrcmp(buf,"KWAJ") && lstrcmp(buf, "SZDD"))
		{
			return MyCopyFile2(lpszSrc,lpszDest,fBumpRefCount,fSystemFile,pfs);
		}
		else
		{
			EBURETCODE retc = (*(GetAppCallback())) ((void *) pfs);
			ASSERT(EBU_OK == retc || EBU_ABORT == retc || EBU_CANCEL == retc);
			
			switch (retc)
			{
			case EBU_ABORT:
			case EBU_CANCEL:
				//
				//User has already been prompted to quit if EBU_CANCEL was returned...
				//
				return EBU_ABORT;
			}
			
			while(TRUE)
			{
				OFSTRUCT ofSrc,ofDest;
				int hSrc,hDest;
				nAlertResult = 0;

				hSrc = EBULZOpenFile((char *) lpszSrc, &ofSrc, OF_READ);

				switch (hSrc)
				{
				case LZERROR_BADINHANDLE :
				case LZERROR_GLOBALLOC :
					// If user told us to abort during a CD Check then do not prompt a second time.
					if (EBU_ABORT == GetResultCode())
						return EBU_ABORT;
					nAlertResult = Alert(GetWndParent(),
										 MB_ICONEXCLAMATION | MB_ABORTRETRYIGNORE | MB_DEFBUTTON2,
										 STR_ERROR_MISSINGCD,
										 (LPCSTR) lpszSrc);

					hSrc = 0;
					break;

				default:
					hFile = EBUCreateFile(lpszDest,GENERIC_READ,FILE_SHARE_READ,NULL,
										  OPEN_ALWAYS,FILE_ATTRIBUTE_NORMAL, NULL);
					if(hFile == INVALID_HANDLE_VALUE)
						// If user told us to abort during a CD Check then do not prompt a second time.
						if (EBU_ABORT == GetResultCode())
							return EBU_ABORT;
					CloseHandle(hFile);
					
					hDest = EBULZOpenFile((char *) lpszDest, &ofDest, OF_CREATE | OF_READWRITE);

					switch (hDest)
					{
					case LZERROR_BADINHANDLE :
					case LZERROR_GLOBALLOC :
						// If user told us to abort during a CD Check then do not prompt a second time.
						if (EBU_ABORT == GetResultCode())
						{
							LZClose(hSrc);
							return EBU_ABORT;
						}
						nAlertResult = Alert(GetWndParent(),
											 MB_ICONEXCLAMATION | MB_ABORTRETRYIGNORE | MB_DEFBUTTON2,
											 STR_ERROR_CANTWRITETEMPFILE,
											 (LPCSTR) lpszDest);

						LZClose(hSrc);

						hDest = 0;
						hSrc = 0;

						break;

					default:
						int nCopyResult;

						nCopyResult = EBULZCopy(hSrc, hDest);

						LZClose(hSrc);
						LZClose(hDest);
						hSrc = 0;
						hDest = 0;

						if (nCopyResult <=0)
						{
							// If user told us to abort during a CD Check then do not prompt a second time.
							if (EBU_ABORT == GetResultCode())
							{
								LZClose(hSrc);
								return EBU_ABORT;
							}
							nAlertResult = Alert(GetWndParent(),
												 MB_ICONEXCLAMATION | MB_ABORTRETRYIGNORE | MB_DEFBUTTON2,
												 STR_ERROR_UNKNOWNWRITEPROBLEM,
												 (LPCSTR) lpszDest);
						}

						break;
					}
				}
				
				switch (nAlertResult)
				{
				case 0:
					return EBU_OK;
					
				case IDABORT:
					if (Alert(GetWndParent(), MB_ICONEXCLAMATION | MB_YESNO | MB_DEFBUTTON2, STR_USERSTOPPEDCOPY) == IDYES)
					{
						return EBU_ABORT;
					}
					
					nAlertResult = IDRETRY;
					
					break;
					
				case IDIGNORE:
					SetCopyIncomplete(TRUE);
					return EBU_OK;
				}
			}
		}
	}
	else
	{
		TRACE(STR_HARDCODE_CANTREADFILEHEADER);
		
		return MyCopyFile2(lpszSrc, lpszDest, fBumpRefCount, fSystemFile, pfs);
	}
	
	return EBU_OK;
}

//****************************************************************************
// Procedure	MyCopyFile2
//
// Purpose		Copies a file, overwriting the existing copy of the file
//				if it exists.
//
// Parameters	
//              lpszDest		full pathname of the target file
//				lpszSrc			full pathname of the source file
//				fBumpRefCount   is this a shared file
//              fSystemFile     is this a system file or a font file?
//
// Returns		TRUE					The file was copied successfully
//              FALSE					User aborted copy attempt
//
// History		 3/28/97    v-richei	Adapted from KenSh's CopyFile function
//               7/14/97    a-richei    Rewrote to copy in "chunk size" bytes
//
EBURETCODE MyCopyFile2(LPCSTR lpszSrc, LPCSTR lpszDest, BOOL fBumpRefCount, BOOL fSystemFile, LPFILECOPYSTATUS pfs)
{
	DWORD	   dwFileAttribs;
	EBURETCODE nCopyResult = EBU_ERROR;
	int		   nAlertResult = 0;
	BOOL       fProcessedReadOnly = FALSE;
	BYTE       *pbCopyBuf = NULL;
	HANDLE     hSrcFile = INVALID_HANDLE_VALUE;
	HANDLE     hDestFile = INVALID_HANDLE_VALUE;
	DWORD      dwBytesLeftToWrite;
	DWORD      dwBytes;
	DWORD      dwLastFile = (pfs) ? pfs->dwLastFile : 0;
	enum       {NOCHUNK, SRCOPEN, DESTOPEN, SRCREAD, DESTWRITE} nCopyState;
	int        nRc;
	
	ASSERT(lpszSrc);
	ASSERT(lpszDest);

	//
	//Default Copy chunk size is about 512K (1 << 19)
	//
	DWORD dwBytesThisTime = 1 << 19;

	//
	//If we've got file status info then we'll copy the file
	//in chunks.  Otherwise, we'll do an atomic copy...
	//
	if (pfs)
	{
		//
		//Allocate a read/write buffer for file copy.  Start with dwBytesThisTime bytes as
		//initialized above.  If the malloc fails, divide the buffer size in half and
		//try again.  Minimum buffer size is 0xFFFF + 1 bytes (64K) because we don't,
		//for instance, want to get callback the copy gauge function every other byte!
		//
		while (dwBytesThisTime > (DWORD) 0xFFFF)
		{
			pbCopyBuf = (BYTE *) malloc(dwBytesThisTime);

			if (NULL == pbCopyBuf)
			{
				dwBytesThisTime >>= 1;
			}
			else
			{
				break;
			}
		} //while (dwBytesThisTime...)

		if (dwBytesThisTime <= (DWORD) 0xFFFF)
		{
			nCopyResult = EBU_ERROR;
			goto CFReturn;
		}

		//
		//Flag that we'll be trying to open the source file
		//
		nCopyState = SRCOPEN;

		//
		//Call the setup app's callback function *before* trying to open source/dest or
		//any other copy operations.  This allows the setup U.I. to display the file name
		//being operated on (for instance).  That way the U.I. looks correct if the user
		//gets an Alert message before the callback within the chunk copy gets called...
		//
		EBURETCODE retc = EBU_OK;

		pfs->dwLastFile = 0;

		retc = (*(GetAppCallback())) ((void *) pfs);
		ASSERT(EBU_OK == retc || EBU_ABORT == retc || EBU_CANCEL == retc);

		switch (retc)
		{
		case EBU_CANCEL:
		case EBU_ABORT:
			//
			//User has already been prompted to cancel if EBU_CANCEL was returned...
			//
			nCopyResult = EBU_ABORT;
			goto CFReturn;
		}
	}
	else
	{
		//
		//No copyfilestatus and no u.i. callback, so flag that we're copying
		//without chunking, e.g.; with Windows CopyFile() API
		//
		nCopyState = NOCHUNK;
	}

	//
	//While trying to copy the file
	//
	while (TRUE)
	{
		//
		//In the event of a "RETRY" return from CFError, we switch on the current
		//copy operation state to retry the appropriate copy step...
		//
		switch (nCopyState)
		{
		case DESTOPEN:
			goto DestOpen;
		case SRCREAD:
			goto SrcRead;
		case DESTWRITE:
			goto DestWrite;
		case NOCHUNK:
			//
			//Atomic copy, used for system files, etc.  no callback...
			//
			nRc = EBUCopyFile(lpszSrc, lpszDest, FALSE);
			if (0 == nRc)
			{
				goto CFError;
			}
			else
			{
				nCopyResult = EBU_OK;
				goto CopyDone;
			}
		} //end switch (nCopyState)

		//
		//We fall through to here and open the source file if SRCOPEN == nCopyState
		//
		hSrcFile = EBUCreateFile(lpszSrc,
								 GENERIC_READ,
								 FILE_SHARE_READ,
								 NULL,
								 OPEN_EXISTING,
								 FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN | FILE_FLAG_WRITE_THROUGH,
								 NULL);

		if (INVALID_HANDLE_VALUE == hSrcFile)
		{
			goto CFError;
		}

		//
		//Flag that we're now working on opening the destination file
		//
		nCopyState = DESTOPEN;

DestOpen:

		//
		//Open the destination file with write access - create the dest file if it
		//doesn't exist...
		//
		hDestFile = EBUCreateFile(lpszDest,
								  GENERIC_WRITE,
								  FILE_SHARE_READ,
								  NULL,
								  CREATE_ALWAYS,
								  FILE_ATTRIBUTE_NORMAL,
								  NULL);

		if (INVALID_HANDLE_VALUE == hDestFile)
		{
			goto CFError;
		}

		//
		//If we opened the file okay, get file size (number of bytes to write)
		//Zero bytes is OK - in that case we'd just create an empty dest file
		//

		dwBytesLeftToWrite = GetFileSize(hSrcFile, NULL);

		if (0xFFFFFFFF == dwBytesLeftToWrite)
		{
			dwBytesLeftToWrite = dwLastFile;
		}

		//
		//Initial chunk size is the smallest of the chunk size of the file size
		//
	    dwBytesThisTime = min(dwBytesLeftToWrite, dwBytesThisTime);

		//
		//Okay, we're getting ready to read...
		//
		nCopyState = SRCREAD;

SrcRead:
DestWrite:
		// zero out file size, only pass ui actually number of bytes copied in last chunk
		if (pfs)
		{
			pfs->dwLastFile = 0;
		}

		//
		//While there are bytes left to write, read from the source and
		//write to the destination
		//
		while (dwBytesLeftToWrite)
		{
			//
			//If the number of bytes left to write is less than the chunk
			//size, then this is the last chunk so set the number of bytes
			//to write to the number of bytes remaining...
			//
			if (dwBytesThisTime > dwBytesLeftToWrite)
			{
				dwBytesThisTime = dwBytesLeftToWrite;
			}

			if (SRCREAD == nCopyState)
			{
				//
				//Read a chunk from the file
				//
				nRc = EBUReadFile(hSrcFile, pbCopyBuf, dwBytesThisTime, &dwBytes, NULL);
				if (0 == nRc)
				{
					goto CFError;
				}

				//
				//Read was successful, we're now working on writing a chunk...
				//
				nCopyState = DESTWRITE;
			}
		
			//
			//Write a chunk
			//
			nRc = WriteFile(hDestFile, pbCopyBuf, dwBytesThisTime, &dwBytes, NULL);
			if (0 == nRc)
			{
				goto CFError;
			}
			else
			{
				FlushFileBuffers(hDestFile);
				//
				//We successfully wrote a chunk, bump the number of bytes written and
				//reset the copy state to indicate that we're ready to read another
				//chunk...
				//
				if (pfs)
				{
					pfs->dwTotalCopied += dwBytesThisTime;
					pfs->dwLastFile = dwBytes;
				}

				nCopyState = SRCREAD;
			}

			//
			//Call the setup app's callback function...
			//
			EBURETCODE 	retc = (*(GetAppCallback())) ((void *) pfs);
			ASSERT(EBU_OK == retc || EBU_ABORT == retc || EBU_CANCEL == retc);
			
			switch(retc)
			{
			case EBU_ABORT:
			case EBU_CANCEL:
				//
				//EBU_CANCEL user had already confirmed they want to abort
				//
				nCopyResult = EBU_ABORT;
				goto CFReturn;
			}
			
			//
			//Decrement the number of bytes left to write...
			//
			dwBytesLeftToWrite -= dwBytesThisTime;
		} // end while (dwBytesLeftToWrite)

		//
		//If the copy was successful, close src and dest and then set normal attributes
		//
		// Added June 11, 1998 a-petere (peter Evans)
		// First flush the DestFile buffers because OS Delayed writes will cause errors on disk swaps.
		//
		FlushFileBuffers(hDestFile);
  		CloseHandle(hSrcFile);
		CloseHandle(hDestFile);

		//
		//Flag so that cleanup doesn't try to reclose the file...
		//
		hSrcFile = INVALID_HANDLE_VALUE;
		hDestFile = INVALID_HANDLE_VALUE;

CopyDone:
		
		//
		//We've got a successful copy, set attributes and return value...
		//
		SetFileAttributes(lpszDest, FILE_ATTRIBUTE_NORMAL);

		nCopyResult = EBU_OK;

		if (fBumpRefCount && AddSharedDLL((LPSTR)lpszDest) )
		{
			MyRefCountSharedDll(lpszDest, TRUE);
		}
			
		goto CFReturn;

CFError:

		//
		//
		//Copy error handling, get the error code and switch
		nRc = GetLastError();
		if (EBU_ABORT == GetResultCode())
		{
			nCopyResult = EBU_ABORT;
			goto CFReturn;
		}
		switch(nRc)
		{
		case CF_ERR_READONLYDEST:
			//
			//First time, let's just try to make it writeable, after that
			//we let the user know and give them the abort, retry, ignore dialog
			//by falling through to the CF_ERR_CANTOPENDEST clause
			//
			if (FALSE == fProcessedReadOnly)
			{
				dwFileAttribs = GetFileAttributes(lpszDest);
				
				if (dwFileAttribs != 0xFFFFFFFF)
				{
					//
					// If the file is read-only then we will try to make it writeable
					//
					if (dwFileAttribs & FILE_ATTRIBUTE_READONLY)
					{
						//
						//remove the read-only bit
						//
						dwFileAttribs &= ~FILE_ATTRIBUTE_READONLY;
						SetFileAttributes(lpszDest, dwFileAttribs);
					}
				}
				
				nAlertResult = IDRETRY;
				fProcessedReadOnly = TRUE;
				
				break;
			}
			
			//
			//Fall through to below if fProcessedReadOnly = TRUE
			//

		case CF_ERR_CANTOPENDEST:
			//Check if this is supposed to be a system file.  If so we
			//add it to the copy list.  If not we ask the user to close
			//the file.
			if (TRUE == fSystemFile)
			{
				char szNewName[_MAX_PATH];
				LPSTR pch, pchPrev;
				
				//Move to the last character of the string
				for( pch = (LPSTR) lpszDest; *pch; pchPrev = pch, pch = CharNext(pch) )
					;  //nothing
				
				//copy the filename minus the last char
				CopyMemory( szNewName, lpszDest, (UINT)(DWORD)pchPrev - (UINT)(DWORD)(LPSTR)lpszDest );
				
				//change last char to an underscore.
				pchPrev = &szNewName[ (UINT)(DWORD)pchPrev - (UINT)(DWORD)(LPSTR)lpszDest ];
				*pchPrev = '_';
				pchPrev = CharNext(pchPrev);
				*pchPrev = '\0';
				
				fProcessedReadOnly = FALSE;
				
				//
				//Setup temp file to add to exit windows list
				//
				while (TRUE)
				{
					//
					//Copy the file to its new temp name.  Will be renamed upon
					//reboot by WININIT.EXE processing of WININIT.INI
					//
					nRc = EBUCopyFile(lpszSrc, szNewName, FALSE) ? 0 : GetLastError();
					
					switch (nRc)
					{
					case 0:
						nAlertResult = 0;
						break;
						
					case CF_ERR_READONLYDEST:
						//
						//First time, let's just try to make it writeable, after that
						//we let the user know and give them the abort, retry, ignore dialog
						//by falling through to the CF_ERR_CANTOPENDEST clause
						//
						if (FALSE == fProcessedReadOnly)
						{
							dwFileAttribs = GetFileAttributes(szNewName);
							
							if (dwFileAttribs != 0xFFFFFFFF)
							{
								// If the file is read-only then we will try to make it writeable
								if (dwFileAttribs & FILE_ATTRIBUTE_READONLY)
								{
									// remove the read-only bit
									dwFileAttribs &= ~FILE_ATTRIBUTE_READONLY;
									SetFileAttributes(szNewName, dwFileAttribs);
								}
							}
							
							nAlertResult = IDRETRY;
							fProcessedReadOnly = TRUE;
							
							break;
						}
						
						//
						//Fall through to below if fProcessedReadOnly = TRUE
						//

					case CF_ERR_CANTOPENDEST:
						nAlertResult = Alert(GetWndParent(),
											 MB_ICONEXCLAMATION | MB_ABORTRETRYIGNORE | MB_DEFBUTTON2,
											 STR_ERROR_CANTWRITETEMPFILE,
											 (LPCSTR) szNewName);
						
						break;
						
					case CF_ERR_CANTOPENSRC:
					case CF_ERR_BADREAD:
						nAlertResult = Alert(GetWndParent(),
											 MB_ICONEXCLAMATION | MB_ABORTRETRYIGNORE | MB_DEFBUTTON2,
				 							 STR_ERROR_MISSINGCD,
											 (LPCSTR) lpszSrc);
						
						break;
						
					case CF_ERR_OUTOFSPACE:
					{
						TCHAR szDrive[2];
						
						lstrcpyn(szDrive, szNewName, sizeof(TCHAR));
						
						//
						//Make the parent window non-topmost, so that the user can
						//look at other stuff on their system.  -ks 4/7/95
						//
						SetWindowPos( GetWndParent(), HWND_NOTOPMOST, 0, 0, 0, 0,
							SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE );
						
						nAlertResult = Alert(GetWndParent(),
											 MB_ICONEXCLAMATION | MB_RETRYCANCEL | MB_DEFBUTTON1,
											 STR_ERROR_NODISKSPACE,
											 (LPCSTR) szDrive);

						if (IDCANCEL == nAlertResult)
						{
							nAlertResult = IDABORT;
						}

						break;
					}

					case CF_ERR_NODESTDIR:
						nAlertResult = Alert(GetWndParent(),
											 MB_ICONEXCLAMATION | MB_ABORTRETRYIGNORE | MB_DEFBUTTON2,
											 STR_ERROR_NODESTDIR,
											 (LPCSTR) szNewName);

						break;

					default:
						nAlertResult = Alert(GetWndParent(),
											 MB_ICONEXCLAMATION | MB_ABORTRETRYIGNORE | MB_DEFBUTTON2,
											 STR_ERROR_UNKNOWNWRITEPROBLEM,
											 (LPCSTR) szNewName);
					} //end switch

					if (IDRETRY != nAlertResult)
					{
						break;
					}
				}
				
				//
				//temp file created successfully, add to restart list
				//
				if (0 == nAlertResult)
				{
					//
					//Add the old (real) name and new (munged) name to the
					//ExitWindowsExec list
					//
					AddFileToExitWindowsList(lpszDest, szNewName);

					//
					//File will be copied upon reboot, so take ref count now...
					//
					if (fBumpRefCount && AddSharedDLL((LPSTR)lpszDest) )
					{
						MyRefCountSharedDll(lpszDest, TRUE);
					}
				}
			}
			else
			{
				// Ask the user to close the file
				nAlertResult = Alert(GetWndParent(),
									 MB_ICONEXCLAMATION | MB_ABORTRETRYIGNORE | MB_DEFBUTTON2,
									 STR_ERROR_PLEASECLOSEFILE,
									 (LPCSTR) lpszDest);
			}
			
			break;

		case CF_ERR_CANTOPENSRC:
			nAlertResult = Alert(GetWndParent(),
								 MB_ICONEXCLAMATION | MB_ABORTRETRYIGNORE | MB_DEFBUTTON2,
								 STR_ERROR_MISSINGCD,
								 (LPCSTR) lpszSrc);
			
			break;
			
		case CF_ERR_BADREAD:
			nAlertResult = Alert(GetWndParent(),
								 MB_ICONEXCLAMATION | MB_ABORTRETRYIGNORE | MB_DEFBUTTON2,
								 STR_ERROR_MISSINGCDHALFWAY,
								 (LPCSTR) lpszSrc);
			
			break;
			
		case CF_ERR_OUTOFSPACE:
		{
			TCHAR szDrive[2];
			
			lstrcpyn(szDrive, lpszDest, sizeof(TCHAR));
			
			//
			//Make the parent window non-topmost, so that the user can
			//look at other stuff on their system.  -ks 4/7/95
			//
			SetWindowPos( GetWndParent(), HWND_NOTOPMOST, 0, 0, 0, 0,
				SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE );
			
			nAlertResult = Alert(GetWndParent(),
								 MB_ICONEXCLAMATION | MB_RETRYCANCEL | MB_DEFBUTTON1,
								 STR_ERROR_NODISKSPACE,
								 (LPCSTR) szDrive);

			if (IDCANCEL == nAlertResult)
			{
				nAlertResult = IDABORT;
			}
			
			break;
		}
			
		case CF_ERR_NODESTDIR:
			nAlertResult = Alert(GetWndParent(),
								 MB_ICONEXCLAMATION | MB_ABORTRETRYIGNORE | MB_DEFBUTTON2,
								 STR_ERROR_NODESTDIR,
								 (LPCSTR) lpszDest);

			break;

		default:
				nAlertResult = Alert(GetWndParent(),
									 MB_ICONEXCLAMATION | MB_ABORTRETRYIGNORE | MB_DEFBUTTON2,
									 STR_ERROR_UNKNOWNWRITEPROBLEM,
									 (LPCSTR) lpszDest);
		} //end switch nCopyResult
		
		if (IDABORT == nAlertResult)
		{
			if (Alert(GetWndParent(), MB_ICONEXCLAMATION | MB_YESNO | MB_DEFBUTTON2, STR_USERSTOPPEDCOPY) == IDYES)
			{
				nCopyResult = EBU_ABORT;
				goto CFReturn;
			}
			else
			{
				nAlertResult = IDRETRY;
			}
		}
	
		//
		//We added the file to the restart list of the user chose to ignore the error...
		//	
		if (0 == nRc || IDIGNORE == nAlertResult || IDOK == nRc)
		{
			if (IDIGNORE == nAlertResult)
			{
				SetCopyIncomplete(TRUE);
			}

			nCopyResult = EBU_OK;
			goto CFReturn;
		}
	} // end while (TRUE) -- trying to copy the file

CFReturn:
	//
	//Cleanup and exit with return value...
	//
	if (pbCopyBuf)
	{
		free(pbCopyBuf);
	}

	if (INVALID_HANDLE_VALUE != hSrcFile)
	{
		CloseHandle(hSrcFile);
	}

	if (INVALID_HANDLE_VALUE != hDestFile)
	{
		//
		//Close dest file, remove any partially written file...
		//
		CloseHandle(hDestFile);
		DeleteFile(lpszDest);
	}

	if (pfs)
	{
		pfs->dwLastFile = dwLastFile;  // reset file size to it's entry value
	}

	return nCopyResult;
}

