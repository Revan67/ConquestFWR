//--------------------------------------------------------------------------//
//                                                                          //
//                                  UTF.cpp                                 //
//                                                                          //
//               COPYRIGHT (C) 1997 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//

/*
		READ-ONLY, and NON-SHARED instance of a UTF file system
*/

#include <windows.h>

#include "DACOM.h"
#include "BaseUTF.h"

#define MAX_GROUP_HANDLES   4
#define MEMORY_MAP_FLAG		0x40000000

struct UTF_CHILD
{
	UTF_DIR_ENTRY	*pEntry;
	DWORD			dwFilePosition;
};

struct UTF_CHILD_GROUP
{
	struct UTF_CHILD_GROUP *pNext;
	UTF_CHILD array[MAX_GROUP_HANDLES];

	
	void * operator new (size_t size)
	{
		return GlobalAlloc(GPTR, size);
	}

	void operator delete (void *p)
	{
		GlobalFree(p);
	}
};


struct UTF_FFHANDLE
{
	UTF_DIR_ENTRY	*pEntry;		// entry to examine on next call to FindNextFile()
	UTF_DIR_ENTRY   *pPrevEntry;	// entry last examined by call to FindFirst/FindNext
	char            szPattern[MAX_PATH+4];
};

struct UTF_FFGROUP
{
	struct UTF_FFGROUP *pNext;
	UTF_FFHANDLE array[MAX_GROUP_HANDLES];

	
	void * operator new (size_t size)
	{
		return GlobalAlloc(GPTR, size);
	}

	void operator delete (void *p)
	{
		GlobalFree(p);
	}
};


BOOL32 __fastcall PatternMatch (const char *string, const char *pattern);


// root directory entry pertains to the whole file (size=size of file, start = 0)

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//

struct DACOM_NO_VTABLE UTF : public BaseUTF
{
	HANDLE			hMapping;			// mapping created by user

	UTF_DIR_ENTRY	*pDirectory;		// pointer to root directory instance
	
	LPCTSTR			pNames;				// pointer to the names buffer
	
	DWORD			dwDataStartOffset;	// offset of file data in this file system
	
	UTF_CHILD_GROUP	children;	

	UTF_FFGROUP		ffhandles;

	char			szPathname[MAX_PATH+4];

	//---------------------------
	// public methods
	//---------------------------

	UTF (void)
	{
		szPathname[0] = UTF_SWITCH_CHAR;
	}

	DA_HEAP_DEFINE_NEW_OPERATOR(UTF);

	virtual ~UTF (void);

	// *** IFileSystem methods ***

	DEFMETHOD_(BOOL,CloseHandle) (HANDLE handle=0);

	DEFMETHOD_(BOOL,ReadFile) (HANDLE hFileHandle, LPVOID lpBuffer, DWORD nNumberOfBytesToRead, 
			   LPDWORD lpNumberOfBytesRead,
			   LPOVERLAPPED lpOverlapped);

	DEFMETHOD_(BOOL,WriteFile) (HANDLE hFileHandle, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite,
				LPDWORD lpNumberOfBytesWritten, LPOVERLAPPED lpOverlapped);

	DEFMETHOD_(BOOL,GetOverlappedResult)   (HANDLE hFileHandle,  
								 LPOVERLAPPED lpOverlapped,
								 LPDWORD lpNumberOfBytesTransferred,   
								 BOOL bWait);   

	DEFMETHOD_(DWORD,SetFilePointer) (HANDLE hFileHandle, LONG lDistanceToMove, 
					 PLONG lpDistanceToMoveHigh=0, DWORD dwMoveMethod=FILE_BEGIN);

	DEFMETHOD_(BOOL,SetEndOfFile) (HANDLE hFileHandle = 0);

	DEFMETHOD_(DWORD,GetFileSize) (HANDLE hFileHandle, LPDWORD lpFileSizeHigh=0);   

	DEFMETHOD_(BOOL,LockFile) (HANDLE hFile,	
							  DWORD dwFileOffsetLow,
							  DWORD dwFileOffsetHigh,
							  DWORD nNumberOfBytesToLockLow,
							  DWORD nNumberOfBytesToLockHigh);

	DEFMETHOD_(BOOL,UnlockFile) (HANDLE hFile, 
								DWORD dwFileOffsetLow,
								DWORD dwFileOffsetHigh,
								DWORD nNumberOfBytesToUnlockLow,
								DWORD nNumberOfBytesToUnlockHigh);

	DEFMETHOD_(BOOL,GetFileTime) (HANDLE hFileHandle,   LPFILETIME lpCreationTime,
				   LPFILETIME lpLastAccessTime, LPFILETIME lpLastWriteTime);

	DEFMETHOD_(BOOL,SetFileTime) (HANDLE hFileHandle,   CONST FILETIME *lpCreationTime, 
				  CONST FILETIME *lpLastAccessTime,
				  CONST FILETIME *lpLastWriteTime);

	DEFMETHOD_(HANDLE,CreateFileMapping)   (HANDLE hFileHandle,
								 LPSECURITY_ATTRIBUTES lpFileMappingAttributes,
								 DWORD flProtect,
								 DWORD dwMaximumSizeHigh,
								 DWORD dwMaximumSizeLow,
								 LPCTSTR lpName);

	DEFMETHOD_(LPVOID,MapViewOfFile)      (HANDLE hFileMappingObject,
								 DWORD dwDesiredAccess,
								 DWORD dwFileOffsetHigh,
								 DWORD dwFileOffsetLow,
								 DWORD dwNumberOfBytesToMap);

	DEFMETHOD_(BOOL,UnmapViewOfFile)      (LPCVOID lpBaseAddress);

	DEFMETHOD_(HANDLE,FindFirstFile) (LPCTSTR lpFileName, LPWIN32_FIND_DATA lpFindFileData);

	DEFMETHOD_(BOOL,FindNextFile) (HANDLE hFindFile, LPWIN32_FIND_DATA lpFindFileData);   

	DEFMETHOD_(BOOL,FindClose) (HANDLE hFindFile);   

	DEFMETHOD_(BOOL,CreateDirectory) (LPCTSTR lpPathName, LPSECURITY_ATTRIBUTES lpSecurityAttributes);

	DEFMETHOD_(BOOL,RemoveDirectory) (LPCTSTR lpPathName);

	DEFMETHOD_(DWORD,GetCurrentDirectory) (DWORD nBufferLength, LPTSTR lpBuffer);

	DEFMETHOD_(BOOL,SetCurrentDirectory) (LPCTSTR lpPathName);

	DEFMETHOD_(BOOL,DeleteFile)  (LPCTSTR lpFileName); 

	DEFMETHOD_(BOOL,MoveFile) (LPCTSTR lpExistingFileName, LPCTSTR lpNewFileName);

	DEFMETHOD_(DWORD,GetFileAttributes) (LPCTSTR lpFileName);

	DEFMETHOD_(BOOL,SetFileAttributes) (LPCTSTR lpFileName, DWORD dwFileAttributes);

	//---------------   
	// IFileSystem extensions to WIN32 system
	//---------------   

	DEFMETHOD_(HANDLE,OpenChild) (DAFILEDESC *lpDesc);

	DEFMETHOD_(DWORD,GetFilePosition) (HANDLE hFileHandle = 0, PLONG pPositionHigh=0);

	DEFMETHOD_(BOOL,GetAbsolutePath) (char *lpOutput, LPCTSTR lpInput, LONG lSize);

	//---------------   
	// BaseUTF methods
	//---------------   

	virtual BOOL init (DAFILEDESC *lpDesc);		// return TRUE if successful

	virtual HANDLE openChild (DAFILEDESC *lpDesc, UTF_DIR_ENTRY * pEntry);

    virtual DWORD getFileAttributes (LPCTSTR lpFileName, UTF_DIR_ENTRY * pEntry);

	virtual UTF_DIR_ENTRY * getDirectoryEntryForChild (LPCSTR lpFileName, UTF_DIR_ENTRY *pRootDir, HANDLE hFindFirst);

	virtual HANDLE findFirstFile (LPCTSTR lpFileName, LPWIN32_FIND_DATA lpFindFileData, UTF_DIR_ENTRY * pEntry);

	virtual const char * getNameBuffer (void);

	//---------------   
	// UTF methods
	//---------------   
				// warning!! this will return bogus number if (handle == 0)

	UTF_CHILD * __fastcall GetUTFChild (HANDLE handle);

	UTF_FFHANDLE * __fastcall GetFF (HANDLE handle);
	
	DWORD GetStartOffset (HANDLE handle)
	{
		return GetUTFChild(handle)->pEntry->dwDataOffset + dwDataStartOffset;
	}

	BOOL32 __fastcall isValidHandle (HANDLE handle);

	HANDLE allocHandle (UTF_DIR_ENTRY * pEntry);	// pEntry is the initial value

	HANDLE allocFFHandle (UTF_DIR_ENTRY * pEntry, const char * pattern);	// pEntry is the initial value

	//---------------   
	// serial methods
	//---------------   

	SERIALMETHOD(CreateMapping_S);

	SERIALMETHOD(ReadFile_S);

static CRITICAL_SECTION criticalSection;
};
CRITICAL_SECTION UTF::criticalSection;

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//

//--------------------------------------------------------------------------//
//
UTF::~UTF(void)
{
	if (pDirectory)
	{
		free(pDirectory);
		pDirectory = 0;
	}

	if (pNames)
	{
		free((void *)pNames);
		pNames = 0;
	}

	if (hMapping)
	{
		pParent->CloseHandle(hMapping);
		hMapping = 0;
	}

	{
		UTF_CHILD_GROUP *pNode;

		while (children.pNext)
		{	
			pNode = children.pNext->pNext;
			delete children.pNext;
			children.pNext = pNode;
		}
	}

	{
		UTF_FFGROUP *pNode;

		while (ffhandles.pNext)
		{	
			pNode = ffhandles.pNext->pNext;
			delete ffhandles.pNext;
			ffhandles.pNext = pNode;
		}
	}
}
//--------------------------------------------------------------------------//
//
inline BOOL32 UTF::isValidHandle (HANDLE _handle)
{
	BOOL32 result = 0;
	UTF_CHILD_GROUP *pNode = &children;
	LONG handle = (LONG) _handle;

	if (handle >= 0)
	{
		while (handle >= MAX_GROUP_HANDLES)
		{
			if ((pNode = pNode->pNext) == 0)
				goto Done;

			handle -= MAX_GROUP_HANDLES;
		}

		result = (pNode->array[handle].pEntry != 0);
	}
Done:
	return result;
}
//--------------------------------------------------------------------------//
//
inline UTF_CHILD * UTF::GetUTFChild (HANDLE _handle)
{
	UTF_CHILD_GROUP *pNode = &children;
	LONG handle = (LONG) _handle;

	while (handle >= MAX_GROUP_HANDLES)
	{
		if ((pNode = pNode->pNext) == 0)
			goto Error;
		handle -= MAX_GROUP_HANDLES;
	}

	return pNode->array + handle;

Error:
	return children.array + 0;
}
//--------------------------------------------------------------------------//
//
inline UTF_FFHANDLE * UTF::GetFF (HANDLE _handle)
{
	UTF_FFGROUP *pNode = &ffhandles;
	U32 handle = ((U32) _handle) - 1;

	while (handle >= MAX_GROUP_HANDLES)
	{
		if ((pNode = pNode->pNext) == 0)
			goto Error;
		handle -= MAX_GROUP_HANDLES;
	}

	return pNode->array + handle;

Error:
	return 0;
}
//--------------------------------------------------------------------------//
// Return TRUE if successfully initialized the instance
//
//
BOOL UTF::init (DAFILEDESC *lpDesc)
{
	UTF_HEADER header;
	DWORD dwRead;
	BOOL result=0;

	if (pParent->ReadFile(hParentFile, &header, sizeof(header), &dwRead, 0) == 0)
		goto Done;
	if (dwRead != sizeof(header))
		goto Done;
	if (header.dwIdentifier != MAKE_4CHAR('U','T','F',' '))
		goto Done;
	if (header.dwVersion != UTF_VERSION)
		goto Done;
	
	dwDataStartOffset = header.dwDataStartOffset;

	//
	// try to read the directory into memory
	//

	if (pParent->SetFilePointer(hParentFile, header.dwDirectoryOffset) == 0xFFFFFFFF)
		goto Done;

	if ((pDirectory = (UTF_DIR_ENTRY *) malloc(header.dwDirectorySize)) == 0)
		goto Done;

	if (pParent->ReadFile(hParentFile, pDirectory, header.dwDirectorySize, &dwRead, 0) == 0 ||
		dwRead != header.dwDirectorySize)
	{
		goto Done;
	}
		
	GetUTFChild(0)->pEntry = pDirectory;
	
	if (pDirectory->dwName != 1)
		goto Done;
	
	//
	// now read the names buffer into memory
	//
	
	if (pParent->SetFilePointer(hParentFile, header.dwNamesOffset) == 0xFFFFFFFF)
		goto Done;

	if ((pNames = (LPCTSTR) malloc(header.dwNameSpaceSize)) == 0)
		goto Done;

	if (pParent->ReadFile(hParentFile, (void *)pNames, header.dwNameSpaceSize, &dwRead, 0) == 0 ||
		dwRead != header.dwNameSpaceSize)
	{
		goto Done;
	}
				
	result = 1;

Done:
	pParent->SetFilePointer(hParentFile, 0, 0, FILE_BEGIN);

	return result;
}
//--------------------------------------------------------------------------//
//
BOOL UTF::CloseHandle (HANDLE handle)
{
	if (handle==0 || ((DWORD)handle & MEMORY_MAP_FLAG))
	{
		if (isValidHandle((HANDLE)((DWORD)handle & ~MEMORY_MAP_FLAG)))
			return 1;

		dwLastError = ERROR_INVALID_HANDLE;
		return 0;
	}

	if (isValidHandle(handle))
	{
		UTF_CHILD * child = GetUTFChild(handle);

		if (child->pEntry)
		{
			child->pEntry = 0;
			return 1;
		}
	}

	dwLastError = ERROR_INVALID_HANDLE;
	return 0;
}
//--------------------------------------------------------------------------//
//
BOOL UTF::ReadFile (HANDLE hFileHandle, LPVOID lpBuffer, DWORD nNumberOfBytesToRead, 
			   LPDWORD lpNumberOfBytesRead,
			   LPOVERLAPPED lpOverlapped)
{
	if (isValidHandle(hFileHandle)==0)
	{
		dwLastError = ERROR_INVALID_HANDLE;
		return 0;
	}
	if (hFileHandle == 0 && lpOverlapped == 0)
	{
		BOOL result;

		result = pParent->ReadFile(hParentFile, lpBuffer, nNumberOfBytesToRead, 
									lpNumberOfBytesRead, 0);

		if (result == 0)
			dwLastError = pParent->GetLastError();

		return result;
	}

	// take advantage of parameters on the stack
	return pParent->SerialCall(this, (DAFILE_SERIAL_PROC) &UTF::ReadFile_S, &hFileHandle);
}
//--------------------------------------------------------------------------//
//
LONG UTF::ReadFile_S (LPVOID lpContext)
{
	BOOL result;
	UTF_READ_STRUCT *pRead = (UTF_READ_STRUCT *) lpContext;

	if (pRead->lpOverlapped)
	{
		// limit the number of bytes read to size of the file
		if (pRead->hFileHandle)
		{
			int iOver;
			DWORD dwFSize = GetFileSize(pRead->hFileHandle);

			pRead->nNumberOfBytesToRead = __min(pRead->nNumberOfBytesToRead, dwFSize);
			iOver = dwFSize - pRead->lpOverlapped->Offset - pRead->nNumberOfBytesToRead;
			if (iOver < 0)
				pRead->nNumberOfBytesToRead += iOver;
		}

		if (pRead->hFileHandle)
			pRead->lpOverlapped->Offset += GetStartOffset(pRead->hFileHandle);
		result = pParent->ReadFile(hParentFile, pRead->lpBuffer, pRead->nNumberOfBytesToRead, pRead->lpNumberOfBytesRead,
				   pRead->lpOverlapped);
		if (pRead->hFileHandle)
			pRead->lpOverlapped->Offset -= GetStartOffset(pRead->hFileHandle);
		if (result==0)
			dwLastError = pParent->GetLastError();
	}
	else  // pRead->hFileHandle != 0 && isValid()
	{
		// limit the number of bytes read to size of the file
		int iOver;
		UTF_CHILD * const pChild = GetUTFChild(pRead->hFileHandle);
		DWORD dwFSize = pChild->pEntry->dwUncompressedSize;
		DWORD dwOffset = pChild->dwFilePosition;

		pRead->nNumberOfBytesToRead = __min(pRead->nNumberOfBytesToRead, dwFSize);
		iOver = dwFSize - dwOffset - pRead->nNumberOfBytesToRead;
		if (iOver < 0)
			pRead->nNumberOfBytesToRead += iOver;

		dwOffset += (pChild->pEntry->dwDataOffset + dwDataStartOffset);
		
		// set file pointer to the right place
		if (pParent->GetFilePosition(hParentFile, NULL) != dwOffset)
			pParent->SetFilePointer(hParentFile, dwOffset, NULL, FILE_BEGIN);

		result = pParent->ReadFile(hParentFile, pRead->lpBuffer, pRead->nNumberOfBytesToRead, 
									pRead->lpNumberOfBytesRead, 0);

		if (result == 0)
			dwLastError = pParent->GetLastError();
		else
			pChild->dwFilePosition += *(pRead->lpNumberOfBytesRead);
	}

	return result;
}
//--------------------------------------------------------------------------//
//
BOOL UTF::WriteFile (HANDLE hFileHandle, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite,
				LPDWORD lpNumberOfBytesWritten, LPOVERLAPPED lpOverlapped)
{
	dwLastError = ERROR_ACCESS_DENIED;
	return 0;
}
//--------------------------------------------------------------------------//
//
BOOL UTF::GetOverlappedResult (HANDLE hFileHandle, LPOVERLAPPED lpOverlapped,
                                 LPDWORD lpNumberOfBytesTransferred,   BOOL bWait)
{
	BOOL result;

	if (isValidHandle(hFileHandle) == 0)
	{
		dwLastError = ERROR_INVALID_HANDLE;
		return 0;
	}
	result = pParent->GetOverlappedResult(hParentFile, lpOverlapped, lpNumberOfBytesTransferred, bWait);
	if (result == 0)
		dwLastError = pParent->GetLastError();
	return result;
}
//--------------------------------------------------------------------------//
//
DWORD UTF::SetFilePointer (HANDLE hFileHandle, LONG lDistanceToMove, 
					 PLONG lpDistanceToMoveHigh, DWORD dwMoveMethod)
{
	DWORD result;

	if (isValidHandle(hFileHandle) == 0)
	{
		dwLastError = ERROR_INVALID_HANDLE;
		return 0xFFFFFFFF;
	}
	else
	if (hFileHandle == 0)
	{
		result = pParent->SetFilePointer(hParentFile, lDistanceToMove, lpDistanceToMoveHigh, dwMoveMethod);
		if (result == 0xFFFFFFFF)
			dwLastError = pParent->GetLastError();
	}
	else
	{
		switch (dwMoveMethod)
		{
		case FILE_BEGIN:
		default:
			break;

		case FILE_CURRENT:
			lDistanceToMove += GetFilePosition(hFileHandle);
			break;

		case FILE_END:
			lDistanceToMove = GetFileSize(hFileHandle) - lDistanceToMove;
			break;
		}

		if ((DWORD)lDistanceToMove > GetFileSize(hFileHandle))
			lDistanceToMove = GetFileSize(hFileHandle);

		result = GetUTFChild(hFileHandle)->dwFilePosition = lDistanceToMove;
		if (lpDistanceToMoveHigh)
			*lpDistanceToMoveHigh = 0;
	}
	
	return result;
}
//--------------------------------------------------------------------------//
//
BOOL UTF::SetEndOfFile (HANDLE hFileHandle)
{
	dwLastError = ERROR_ACCESS_DENIED;
	return 0;
}
//--------------------------------------------------------------------------//
//
DWORD UTF::GetFileSize (HANDLE hFileHandle, LPDWORD lpFileSizeHigh)
{
	DWORD result;
	
	if (isValidHandle(hFileHandle) == 0)
	{
		dwLastError = ERROR_INVALID_HANDLE;
		return 0xFFFFFFFF;
	}
	else
	{
		result = GetUTFChild(hFileHandle)->pEntry->dwUncompressedSize;
		if (lpFileSizeHigh)
			*lpFileSizeHigh = 0;
		return result;
	}
}
//--------------------------------------------------------------------------//
//
BOOL UTF::LockFile (HANDLE hFileHandle,	
					  DWORD dwFileOffsetLow,
					  DWORD dwFileOffsetHigh,
					  DWORD nNumberOfBytesToLockLow,
					  DWORD nNumberOfBytesToLockHigh)
{
	BOOL result;

	if (pParent)
	{
		result = pParent->LockFile(hParentFile,
									dwFileOffsetLow,
									dwFileOffsetHigh,
									nNumberOfBytesToLockLow,
									nNumberOfBytesToLockHigh);
		if (result == 0)
			dwLastError = pParent->GetLastError();
		return result;
	}
	else
	{
		dwLastError = ERROR_ACCESS_DENIED;
		return 0;
	}
}
//--------------------------------------------------------------------------//
//
BOOL UTF::UnlockFile (HANDLE hFileHandle, 
						DWORD dwFileOffsetLow,
						DWORD dwFileOffsetHigh,
						DWORD nNumberOfBytesToUnlockLow,
						DWORD nNumberOfBytesToUnlockHigh)
{
	BOOL result;

	if (pParent)
	{
		result = pParent->UnlockFile(hParentFile,
									dwFileOffsetLow,
									dwFileOffsetHigh,
									nNumberOfBytesToUnlockLow,
									nNumberOfBytesToUnlockHigh);
		if (result == 0)
			dwLastError = pParent->GetLastError();
		return result;
	}
	else
	{
		dwLastError = ERROR_ACCESS_DENIED;
		return 0;
	}
}
//--------------------------------------------------------------------------//
//
BOOL UTF::GetFileTime (HANDLE hFileHandle,   LPFILETIME lpCreationTime,
				   LPFILETIME lpLastAccessTime, LPFILETIME lpLastWriteTime)
{
	UTF_DIR_ENTRY *pEntry;

	if (isValidHandle(hFileHandle) == 0)
	{
		dwLastError = ERROR_INVALID_HANDLE;
		return 0;
	}

	pEntry = GetUTFChild(hFileHandle)->pEntry;

	if (lpCreationTime)
		DOSTimeToFileTime(pEntry->DOSCreationTime, lpCreationTime);
	if (lpLastAccessTime)
		DOSTimeToFileTime(pEntry->DOSLastAccessTime, lpLastAccessTime);
	if (lpLastWriteTime)
		DOSTimeToFileTime(pEntry->DOSLastWriteTime, lpLastWriteTime);
	
	return 1;
}
//--------------------------------------------------------------------------//
//
BOOL UTF::SetFileTime (HANDLE hFileHandle,   CONST FILETIME *lpCreationTime, 
				  CONST FILETIME *lpLastAccessTime,
				  CONST FILETIME *lpLastWriteTime)
{
	dwLastError = ERROR_ACCESS_DENIED;
	return 0;
}
//--------------------------------------------------------------------------//
//
HANDLE UTF::CreateFileMapping (HANDLE hFileHandle, LPSECURITY_ATTRIBUTES lpFileMappingAttributes,
                                 DWORD flProtect, DWORD dwMaximumSizeHigh, DWORD dwMaximumSizeLow, LPCTSTR lpName)
{
	if (isValidHandle(hFileHandle) == 0)
	{
		dwLastError = ERROR_INVALID_HANDLE;
		return 0;
	}

	if (flProtect & PAGE_READWRITE)
	{
		dwLastError = ERROR_ACCESS_DENIED;
		return 0;
	}

	// take advantage of parameters on the stack
	pParent->SerialCall(this, (DAFILE_SERIAL_PROC) &UTF::CreateMapping_S, 0);

	if (hMapping == 0)
		return 0;

	return (HANDLE) (MEMORY_MAP_FLAG + (LONG)hFileHandle);
}
//--------------------------------------------------------------------------//
//
LONG UTF::CreateMapping_S (LPVOID lpContext)
{
	if (hMapping)
		return (LONG) hMapping;

	// if this is for a child file, adjust his size to fit current mapping
	// if mapping size is larger than current child filesize AND child 
	// has write access

	hMapping = pParent->CreateFileMapping(hParentFile, 0, PAGE_READONLY, 0,0,0);

	if (hMapping == 0)
		dwLastError = pParent->GetLastError();

	return (LONG) hMapping;
}
//--------------------------------------------------------------------------//
//
LPVOID UTF::MapViewOfFile (HANDLE hFileMappingObject,
                            DWORD dwDesiredAccess,
                            DWORD dwFileOffsetHigh,
                            DWORD dwFileOffsetLow,
                            DWORD dwNumberOfBytesToMap)
{
	LPVOID result;

	if (hFileMappingObject)
	{
		hFileMappingObject = (HANDLE) ((LONG)hFileMappingObject & ~MEMORY_MAP_FLAG);
		if (isValidHandle(hFileMappingObject) == 0)
		{
			dwLastError = ERROR_INVALID_HANDLE;
			return 0;
		}
	}

	// if mapping a child file
	if (hFileMappingObject)
	{
		if (dwNumberOfBytesToMap == 0)
			dwNumberOfBytesToMap = GetFileSize(hFileMappingObject);
		else
			dwNumberOfBytesToMap = __min(dwNumberOfBytesToMap, GetFileSize(hFileMappingObject));
		dwFileOffsetLow += GetStartOffset(hFileMappingObject);
	}

	result = pParent->MapViewOfFile(hMapping,
			                        dwDesiredAccess,
								    0,
				                    dwFileOffsetLow,
				                    dwNumberOfBytesToMap);
	if (result == 0)
		dwLastError = pParent->GetLastError();

	return result;
}
//--------------------------------------------------------------------------//
//
BOOL UTF::UnmapViewOfFile (LPCVOID lpBaseAddress)
{
	BOOL result;

	result = pParent->UnmapViewOfFile(lpBaseAddress);

	if (result == 0)
		dwLastError = pParent->GetLastError();

	return result;
}
//--------------------------------------------------------------------------//
//
HANDLE UTF::FindFirstFile (LPCTSTR lpFileName, LPWIN32_FIND_DATA lpFindFileData)
{
	HANDLE i;
	char buffer[MAX_PATH+4];
	char *ptr;
	UTF_DIR_ENTRY *pEntry = pDirectory;

	if (GetAbsolutePath(buffer, lpFileName, MAX_PATH) == 0)
	{
		dwLastError = ERROR_BAD_PATHNAME;
		return INVALID_HANDLE_VALUE;
	}

	if ((ptr = strrchr(buffer, UTF_SWITCH_CHAR)) != 0 && ptr!=buffer)
	{
		*ptr = 0;
		if (GetDirectoryEntry(buffer, pDirectory, pNames, &pEntry) == 0)
		{
			dwLastError = ERROR_BAD_PATHNAME;
			return INVALID_HANDLE_VALUE;
		}

		if ((pEntry->dwAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
		{
			dwLastError = ERROR_BAD_PATHNAME;
			return INVALID_HANDLE_VALUE;
		}
	}

	i = allocFFHandle((UTF_DIR_ENTRY *) (((char *)pDirectory) + pEntry->dwDataOffset), ptr+1);

	if (i != INVALID_HANDLE_VALUE && FindNextFile(i, lpFindFileData) == 0)
	{
		FindClose(i);
		i = INVALID_HANDLE_VALUE;
	}
	return i;
}
//--------------------------------------------------------------------------//
//
HANDLE UTF::findFirstFile (LPCTSTR lpFileName, LPWIN32_FIND_DATA lpFindFileData, UTF_DIR_ENTRY * pEntry)
{
	HANDLE i;
	char buffer[MAX_PATH+4];
	char *ptr;

	memcpy(buffer, lpFileName, sizeof(buffer));

	if ((ptr = strrchr(buffer, UTF_SWITCH_CHAR)) != 0 && ptr!=buffer)
	{
		*ptr = 0;
		if (GetDirectoryEntry(buffer, pDirectory, pNames, &pEntry) == 0)
		{
			dwLastError = ERROR_BAD_PATHNAME;
			return INVALID_HANDLE_VALUE;
		}

		if ((pEntry->dwAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
		{
			dwLastError = ERROR_BAD_PATHNAME;
			return INVALID_HANDLE_VALUE;
		}
	}

	i = allocFFHandle((UTF_DIR_ENTRY *) (((char *)pDirectory) + pEntry->dwDataOffset), ptr+1);

	if (i != INVALID_HANDLE_VALUE && FindNextFile(i, lpFindFileData) == 0)
	{
		FindClose(i);
		i = INVALID_HANDLE_VALUE;
	}
	return i;
}
//--------------------------------------------------------------------------//
//
BOOL UTF::FindNextFile (HANDLE hFindFile, LPWIN32_FIND_DATA lpData)
{
	UTF_FFHANDLE * pFF=GetFF(hFindFile);

	if (pFF == 0 || pFF->pEntry == 0)
	{
		dwLastError = ERROR_INVALID_HANDLE;
		return 0;
	}

	if (pFF->pEntry == pDirectory)
	{
		dwLastError = ERROR_NO_MORE_FILES;
		return 0;
	}
	
	// make sure filename matches the pattern
	//
	if ( ((U32 *)(&pFF->szPattern[0]))[0]!='\0*.*' && PatternMatch(pNames + pFF->pEntry->dwName, pFF->szPattern) == 0)
	{
		// did not match, go to the next entry
		pFF->pEntry = (UTF_DIR_ENTRY *) (((char *)pDirectory) + pFF->pEntry->dwNext);
		return UTF::FindNextFile(hFindFile, lpData);		// explicit tail recursion
	}
	
	// else fill in the data

	lpData->dwFileAttributes = pFF->pEntry->dwAttributes;
	DOSTimeToFileTime(pFF->pEntry->DOSCreationTime, &lpData->ftCreationTime);
	DOSTimeToFileTime(pFF->pEntry->DOSLastAccessTime, &lpData->ftLastAccessTime);
	DOSTimeToFileTime(pFF->pEntry->DOSLastWriteTime, &lpData->ftLastWriteTime);
	lpData->nFileSizeHigh    = 0;
	lpData->nFileSizeLow     = pFF->pEntry->dwUncompressedSize;
    lpData->dwReserved0		 = 0;
    lpData->dwReserved1		 = 0;
	strcpy(lpData->cFileName, pNames + pFF->pEntry->dwName);
	lpData->cAlternateFileName[0] = 0;

	pFF->pPrevEntry = pFF->pEntry;
	pFF->pEntry = (UTF_DIR_ENTRY *) (((char *)pDirectory) + pFF->pEntry->dwNext);

	return 1;
}
//--------------------------------------------------------------------------//
//
BOOL UTF::FindClose (HANDLE hFindFile)
{
	UTF_FFHANDLE * pFF = GetFF(hFindFile);
   
	if (pFF && pFF->pEntry)
	{
		pFF->pEntry = 0;
		return 1;
	}
	else
	{
		dwLastError = ERROR_INVALID_HANDLE;
		return 0;
	}
}
//--------------------------------------------------------------------------//
//
BOOL UTF::CreateDirectory (LPCTSTR lpPathName, LPSECURITY_ATTRIBUTES lpSecurityAttributes)
{
	dwLastError = ERROR_ACCESS_DENIED;
	return 0;
}
//--------------------------------------------------------------------------//
//
BOOL UTF::RemoveDirectory (LPCTSTR lpPathName)
{
	dwLastError = ERROR_ACCESS_DENIED;
	return 0;
}
//--------------------------------------------------------------------------//
//
DWORD UTF::GetCurrentDirectory (DWORD nBufferLength, LPTSTR lpBuffer)
{
	DWORD len, result;

	len = strlen(szPathname);
	len = __max(len, 2);
	len = __min(len, nBufferLength);
	if (len>0)
	{
		memcpy(lpBuffer, szPathname, len);
		lpBuffer[len-1] = 0;
		result = len-1;
	}
	else
		result = 0;

	return result;
}
//--------------------------------------------------------------------------//
//
BOOL UTF::SetCurrentDirectory (LPCTSTR lpPathName)
{
 	char buffer[MAX_PATH+4];
	BOOL result;

	if ((result = GetAbsolutePath(buffer, lpPathName, MAX_PATH)) == 0)
		dwLastError = ERROR_BAD_PATHNAME;
	else
	{
		int len;
		DWORD dwAttribs;

		dwAttribs = GetFileAttributes(buffer);

		if (dwAttribs == 0xFFFFFFFF || (dwAttribs&FILE_ATTRIBUTE_DIRECTORY) == 0)
		{
			dwLastError = ERROR_BAD_PATHNAME;
			return 0;
		}
		else
		{
			len = strlen(buffer);
			memcpy(szPathname, buffer, len+1);
			if (szPathname[len-1] != '\\')
			{
				szPathname[len] = '\\';
				szPathname[len+1] = 0;
			}
		}
	}
		
	return result;
}
//--------------------------------------------------------------------------//
//
BOOL UTF::DeleteFile  (LPCTSTR lpFileName)
{
	dwLastError = ERROR_ACCESS_DENIED;
	return 0;
}
//--------------------------------------------------------------------------//
//
BOOL UTF::MoveFile (LPCTSTR lpExistingFileName, LPCTSTR lpNewFileName)
{
	dwLastError = ERROR_ACCESS_DENIED;
	return 0;
}
//--------------------------------------------------------------------------//
// we can assume that filename is sanitized
//
DWORD UTF::getFileAttributes (LPCTSTR lpFileName, UTF_DIR_ENTRY * pEntry)
{
	if (GetDirectoryEntry(lpFileName, pDirectory, pNames, &pEntry) == 0)
	{
		dwLastError = ERROR_FILE_NOT_FOUND;
		return 0xFFFFFFFF;
	}

	return pEntry->dwAttributes;
}
//--------------------------------------------------------------------------//
//
DWORD UTF::GetFileAttributes (LPCTSTR lpFileName)
{
	char buffer[MAX_PATH+4];
	UTF_DIR_ENTRY *pEntry = pDirectory;

	if (GetAbsolutePath(buffer, lpFileName, MAX_PATH) == 0)
	{
		dwLastError = ERROR_BAD_PATHNAME;
		return 0;
	}

	if (GetDirectoryEntry(buffer, pDirectory, pNames, &pEntry) == 0)
	{
		dwLastError = ERROR_FILE_NOT_FOUND;
		return 0xFFFFFFFF;
	}

	return pEntry->dwAttributes;
}
//--------------------------------------------------------------------------//
//
BOOL UTF::SetFileAttributes (LPCTSTR lpFileName, DWORD dwFileAttributes)
{
	dwLastError = ERROR_ACCESS_DENIED;
	return 0;
}
//--------------------------------------------------------------------------//
// special case, we already know the filename is sanitized
//
HANDLE UTF::openChild (DAFILEDESC *lpDesc, UTF_DIR_ENTRY * pEntry)
{
	HANDLE handle, hFF;
	
	if (lpDesc->dwDesiredAccess != GENERIC_READ ||
		lpDesc->dwCreationDistribution != OPEN_EXISTING)
	{
		dwLastError = ERROR_ACCESS_DENIED;
		return INVALID_HANDLE_VALUE;
	}

	// shortcut the usual stuff if a handle has been supplied
	if ((hFF = GETFFHANDLE(lpDesc)) != INVALID_HANDLE_VALUE)
	{
		UTF_FFHANDLE * pFF = GetFF(hFF);
		pEntry = (pFF && pFF->pEntry) ? pFF->pPrevEntry : 0;

		if (pEntry==0 || ((pEntry->dwAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0))
		{
			dwLastError = ERROR_FILE_NOT_FOUND;
			return INVALID_HANDLE_VALUE;
		}

		if (lpDesc->lpFileName==0 || stricmp(pNames+pEntry->dwName, lpDesc->lpFileName) != 0)
		{
			dwLastError = ERROR_INVALID_PARAMETER;
			return INVALID_HANDLE_VALUE;
		}
	}
	else
	if (GetDirectoryEntry(lpDesc->lpFileName, pDirectory, pNames, &pEntry) == 0 ||
		(pEntry->dwAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
	{
		dwLastError = ERROR_FILE_NOT_FOUND;
		return INVALID_HANDLE_VALUE;
	}

	handle = allocHandle(pEntry);

	if (handle == INVALID_HANDLE_VALUE)
	{
		dwLastError = ERROR_OUT_OF_STRUCTURES;
		return handle;
	}

	return handle;
}
//--------------------------------------------------------------------------//
//
HANDLE UTF::OpenChild (DAFILEDESC *lpDesc)
{
	HANDLE handle, hFF;
	UTF_DIR_ENTRY *pEntry = pDirectory;
	
	if (lpDesc->dwDesiredAccess != GENERIC_READ ||
		lpDesc->dwCreationDistribution != OPEN_EXISTING)
	{
		dwLastError = ERROR_ACCESS_DENIED;
		return INVALID_HANDLE_VALUE;
	}

	// shortcut the usual stuff if a find-first handle has been supplied
	if ((hFF = GETFFHANDLE(lpDesc)) != INVALID_HANDLE_VALUE)
	{
		UTF_FFHANDLE * pFF = GetFF(hFF);
		pEntry = (pFF && pFF->pEntry) ? pFF->pPrevEntry : 0;

		if (pEntry==0 || ((pEntry->dwAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0))
		{
			dwLastError = ERROR_FILE_NOT_FOUND;
			return INVALID_HANDLE_VALUE;
		}

		if (lpDesc->lpFileName==0 || stricmp(pNames+pEntry->dwName, lpDesc->lpFileName) != 0)
		{
			dwLastError = ERROR_INVALID_PARAMETER;
			return INVALID_HANDLE_VALUE;
		}
	}
	else
	{
		char buffer[MAX_PATH+4];

 		if (GetAbsolutePath(buffer, lpDesc->lpFileName, MAX_PATH) == 0)
		{
			dwLastError = ERROR_FILE_NOT_FOUND;
			return INVALID_HANDLE_VALUE;
		}

		if (GetDirectoryEntry(buffer, pDirectory, pNames, &pEntry) == 0 ||
			(pEntry->dwAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
		{
			dwLastError = ERROR_FILE_NOT_FOUND;
			return INVALID_HANDLE_VALUE;
		}
	}

	handle = allocHandle(pEntry);

	if (handle == INVALID_HANDLE_VALUE)
	{
		dwLastError = ERROR_OUT_OF_STRUCTURES;
		return handle;
	}

	return handle;
}
//--------------------------------------------------------------------------//
//
DWORD UTF::GetFilePosition (HANDLE hFileHandle, PLONG pPositionHigh)
{
	DWORD result;

	if (isValidHandle(hFileHandle) == 0)
	{
		dwLastError = ERROR_INVALID_HANDLE;
		return 0xFFFFFFFF;
	}
	else
	if (hFileHandle == 0)
	{
		result = pParent->GetFilePosition(hParentFile, pPositionHigh);
		if (result == 0xFFFFFFFF)
			dwLastError = pParent->GetLastError();
		return result;
	}
	else
	{
		result = GetUTFChild(hFileHandle)->dwFilePosition;
		if (pPositionHigh)
			*pPositionHigh = 0;
		return result;
	}
}
//--------------------------------------------------------------------------//
//
HANDLE UTF::allocFFHandle (UTF_DIR_ENTRY * pEntry, const char * pattern)
{
	long i,j,result=-1;
	UTF_FFGROUP *pNode = &ffhandles;

	i = j = 0;

	EnterCriticalSection(&criticalSection);

	while (1)
	{
		for ( ; i < MAX_GROUP_HANDLES; i++, j++)
		{	
			if (pNode->array[i].pEntry == 0)
			{
				pNode->array[i].pEntry = pEntry;		// mark it allocated
				pNode->array[i].pPrevEntry = 0;
				strncpy(pNode->array[i].szPattern, pattern, MAX_PATH);
				result = j+1;
				goto Done;
			}	
		}
		i = 0;
		if (pNode->pNext)
			pNode = pNode->pNext;
		else
		{
			if ((pNode->pNext = new UTF_FFGROUP) != 0)
				pNode = pNode->pNext;
			else
				break;

		}
	}

	dwLastError = ERROR_OUT_OF_STRUCTURES;
Done:
	LeaveCriticalSection(&criticalSection);
	return (HANDLE ) result;
}
//--------------------------------------------------------------------------//
//
HANDLE UTF::allocHandle (UTF_DIR_ENTRY * pEntry)
{
	long i,j,result=-1;
	UTF_CHILD_GROUP *pNode = &children;

	i = j = 1;

	EnterCriticalSection(&criticalSection);

	while (1)
	{
		for ( ; i < MAX_GROUP_HANDLES; i++, j++)
		{	
			if (pNode->array[i].pEntry == 0)
			{
				pNode->array[i].pEntry = pEntry;		// mark it allocated
				pNode->array[i].dwFilePosition = 0;
				result = j;
				goto Done;
			}
		}
		i = 0;
		if (pNode->pNext)
			pNode = pNode->pNext;
		else
		{
			if ((pNode->pNext = new UTF_CHILD_GROUP) != 0)
				pNode = pNode->pNext;
			else
				break;

		}
	}

	dwLastError = ERROR_OUT_OF_STRUCTURES;
Done:
	LeaveCriticalSection(&criticalSection);

	return (HANDLE) result;
}
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
void __fastcall switchchar_convert (char * string);
//--------------------------------------------------------------------------//
// Get absolute path in terms of this file system
//  returns a path with a leading '\\'
//
//
BOOL UTF::GetAbsolutePath (char *lpOutput, LPCTSTR lpInput, LONG lSize)
{
	int len;
	char *ptr;
	BOOL result=1;

	if (lSize <= 0)
		return 0;

	if (lpInput[0] == UTF_SWITCH_CHAR || lpInput[0] == '/')
	{
		strncpy(lpOutput, lpInput, lSize);
		lpOutput[lSize-1] = 0;
		switchchar_convert(lpOutput);
		return 1;
	}

	strncpy(lpOutput, szPathname, lSize);

	// now of the form "\\Path\\"

	if (lpInput[0] == '.' && (lpInput[1] == UTF_SWITCH_CHAR || lpInput[1] == '/'))
		lpInput+=2;

	while (lpInput[0] == '.' && lpInput[1] == '.')
	{
		len = strlen(lpOutput);
		if (len > 2)
		{
			lpOutput[len-1] = 0;		// get rid of trailing '\\'

			if ((ptr = strrchr(lpOutput, UTF_SWITCH_CHAR)) != 0)
				ptr[1] = 0;
			else
				result = 0;
		}
		else
			result = 0;
		lpInput+=2;
		if (lpInput[0] == UTF_SWITCH_CHAR || lpInput[0] == '/')
			lpInput++;
	}

	len = strlen(lpOutput);
	if (lSize - len > 0)
		strncpy(lpOutput+len, lpInput, lSize-len);
	switchchar_convert(lpOutput);

	return result;
}
//--------------------------------------------------------------------------//
//
UTF_DIR_ENTRY * UTF::getDirectoryEntryForChild (LPCSTR lpFileName, UTF_DIR_ENTRY *pRootDir, HANDLE hFindFirst)
{
	// shortcut the usual stuff if a handle has been supplied
	if (hFindFirst != INVALID_HANDLE_VALUE)
	{
		UTF_FFHANDLE * pFF = GetFF(hFindFirst);
		pRootDir = (pFF && pFF->pEntry) ? pFF->pPrevEntry : 0;
		return pRootDir;
	}
	
	if (pRootDir == 0)
		pRootDir = pDirectory;

	if (GetDirectoryEntry(lpFileName, pDirectory, pNames, &pRootDir) == 0)
		return 0;
	return pRootDir;
}
//--------------------------------------------------------------------------//
//
const char * UTF::getNameBuffer (void)
{
	return pNames;
}
//--------------------------------------------------------------------------//
//
BaseUTF * CreateUTF (void)
{ 
	return new DAComponent<UTF>;
}
//--------------------------------------------------------------------------//
//
void startupUTF (void)
{
	InitializeCriticalSection(&UTF::criticalSection);
}
//--------------------------------------------------------------------------//
//
void shutdownUTF (void)
{
	DeleteCriticalSection(&UTF::criticalSection);
}
//--------------------------------------------------------------------------//
//-------------------------------End UTF.cpp--------------------------------//
//--------------------------------------------------------------------------//
