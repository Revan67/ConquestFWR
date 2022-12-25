//--------------------------------------------------------------------------//
//                                                                          //
//                               UTFShare.cpp                               //
//                                                                          //
//               COPYRIGHT (C) 1997 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//

/*
		SHARED instance of a UTF file system
*/

#include <windows.h>

#include "DACOM.h"
#include "BaseUTF.h"
#include "UTFDMan.h"
#include "da_heap_utility.h"
#include "IUTFWriter.h"

#define MAX_GROUP_HANDLES   4

struct UTF_SHARED_FF
{
	BOOL32 bInUse;
	const char * szPattern;
	char szDirectory[MAX_PATH+4];
	char szPreviousFile[MAX_PATH+4];
	char szNextFile[MAX_PATH+4];
};

struct UTF_SHARED_CHILD
{
	UTF_SHARING	Sharing;
	DWORD		dwFilePosition;
	char		szFilename[MAX_PATH+4];
};

struct UTF_SHARED_GROUP
{
	struct UTF_SHARED_GROUP *pNext;
	UTF_SHARED_CHILD array[MAX_GROUP_HANDLES];

	
	void * operator new (size_t size)
	{
		return GlobalAlloc(GPTR, size);
	}

	void operator delete (void *p)
	{
		GlobalFree(p);
	}
};

struct UTF_SHARED_FFGROUP
{
	struct UTF_SHARED_FFGROUP *pNext;
	UTF_SHARED_FF array[MAX_GROUP_HANDLES];

	
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

struct DACOM_NO_VTABLE SharedUTF : public BaseUTF, IUTFWriter
{
	BaseLockManager		locker;

	UTF_SHARED_GROUP	children;	

	UTF_SHARED_FFGROUP	ffhandles;

	char				szPathname[MAX_PATH+4];

	DWORD				dwLockCount;

	// "hint" members that can be adjusted by user
	U32					UTF_EXTRA_ENTRIES, UTF_EXTRA_NAME_SPACE;

	//
	// interface map
	//
	BEGIN_DACOM_MAP_INBOUND(SharedUTF)
	DACOM_INTERFACE_ENTRY(IFileSystem)
	DACOM_INTERFACE_ENTRY2(IID_IFileSystem,IFileSystem)
	DACOM_INTERFACE_ENTRY(IUTFWriter)
	DACOM_INTERFACE_ENTRY2(IID_IUTFWriter,IUTFWriter)
	END_DACOM_MAP()

	//---------------------------
	// public methods
	//---------------------------

	SharedUTF (void)
	{
		szPathname[0] = UTF_SWITCH_CHAR;
		children.array[0].szFilename[0] = UTF_SWITCH_CHAR;
		locker.setFileSystem(this);
		UTF_EXTRA_ENTRIES = DEFAULT_UTF_EXTRA_ENTRIES;
		UTF_EXTRA_NAME_SPACE = DEFAULT_UTF_EXTRA_NAME_SPACE;
	}

	DA_HEAP_DEFINE_NEW_OPERATOR(SharedUTF)

	virtual ~SharedUTF (void);

	// *** IFileSystem methods ***

	DEFMETHOD_(BOOL,CloseHandle) (HANDLE handle);

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

	DEFMETHOD_(DWORD,GetFileSize) (HANDLE hFileHandle=0, LPDWORD lpFileSizeHigh=0);   

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

	/* IUTFWriter methods */

	virtual void __stdcall Flush (void);

	virtual void __stdcall AddDirEntries (U32 numEntries);

	virtual void __stdcall AddNameSpace (U32 numBytes);

	virtual void __stdcall HintExpansionDirEntries (U32 numEntries);

	virtual void __stdcall HintExpansionNameSpace (U32 numBytes);

	//---------------   
	// BaseUTF methods
	//---------------   

	virtual BOOL init (DAFILEDESC *lpDesc);		// return TRUE if successful

	//---------------   
	// SharedUTF methods
	//---------------   

	//---------------   
	// serial methods
	//---------------   

	SERIALMETHOD(AllocHandle_S);

	SERIALMETHOD(AllocHandleFF_S);

	SERIALMETHOD(CloseHandle_S);

	SERIALMETHOD(CloseAllHandles_S);

	SERIALMETHOD(ReadFile_S);

	SERIALMETHOD(WriteFile_S);

	SERIALMETHOD(SetEndOfFile_S);

	SERIALMETHOD(GetFileSize_S);

	SERIALMETHOD(GetFileTime_S);

	SERIALMETHOD(SetFileTime_S);

	SERIALMETHOD(FindFirstFile_S);

	SERIALMETHOD(FindNextFile_S);

	SERIALMETHOD(CreateDirectory_S);

	SERIALMETHOD(RemoveDirectory_S);

	SERIALMETHOD(DeleteFile_S);

	SERIALMETHOD(MoveFile_S);

	SERIALMETHOD(GetFileAttributes_S);

	SERIALMETHOD(SetFileAttributes_S);

	SERIALMETHOD(OpenChild_S);

	SERIALMETHOD(Flush_S);

	SERIALMETHOD(AddDirEntries_S);

	SERIALMETHOD(AddNameSpace_S);

	UTF_DIR_ENTRY * CreateNewEntry_S (LPCTSTR lpPathName, BOOL bFailIfExists);

	static BOOL __fastcall ClearSharingInFiles (UTF_DIR_ENTRY *pDirectory, UTF_DIR_ENTRY *pEntry);

	static BOOL __fastcall CheckForOpenChild (UTF_DIR_ENTRY *pDirectory, UTF_DIR_ENTRY *pEntry);

	UTF_SHARED_CHILD * __fastcall GetUTFChild (HANDLE handle);

	UTF_SHARED_FF * __fastcall GetFF (HANDLE handle);
	
	BOOL32 __fastcall isValidHandle (HANDLE handle);
};


//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//

//--------------------------------------------------------------------------//
//
SharedUTF::~SharedUTF(void)
{
	if (pParent)
		pParent->SerialCall(this, (DAFILE_SERIAL_PROC) &SharedUTF::CloseAllHandles_S, 0);

	{
		UTF_SHARED_GROUP *pNode;

		while (children.pNext)
		{	
			pNode = children.pNext->pNext;
			delete children.pNext;
			children.pNext = pNode;
		}
	}

	{
		UTF_SHARED_FFGROUP *pNode;

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
BOOL32 SharedUTF::isValidHandle (HANDLE _handle)
{
	BOOL32 result = 0;
	UTF_SHARED_GROUP *pNode = &children;
	LONG handle = (LONG) _handle;

	if (handle == 0)
		result = 1;
	else
	if (handle > 0)
	{
		while (handle >= MAX_GROUP_HANDLES)
		{
			if ((pNode = pNode->pNext) == 0)
				goto Done;

			handle -= MAX_GROUP_HANDLES;
		}

		result = (pNode->array[handle].szFilename[0] != 0);
	}
Done:
	return result;
}
//--------------------------------------------------------------------------//
//
UTF_SHARED_CHILD *SharedUTF::GetUTFChild (HANDLE _handle)
{
	UTF_SHARED_GROUP *pNode = &children;
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
UTF_SHARED_FF *SharedUTF::GetFF (HANDLE _handle)
{
	UTF_SHARED_FFGROUP *pNode = &ffhandles;
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
// Return TRUE is successfully initialized the instance
//
//
BOOL SharedUTF::init (DAFILEDESC *lpDesc)
{
	UTF_HEADER header;
	DWORD dwRead;
	BOOL result=0;

	if (pParent->ReadFile(hParentFile, &header, sizeof(header), &dwRead, 0) == 0 ||
		dwRead != sizeof(header))
	{
		if (lpDesc->dwCreationDistribution != OPEN_EXISTING && 
			(dwAccess & GENERIC_WRITE) && 
			pParent->GetFileSize(hParentFile) == 0)
		{
			UTF_DIR_ENTRY entry;
			SYSTEMTIME systemTime;
			DWORD dwName = MAKE_4CHAR(0, '\\', 0, 0);

			GetSystemTime(&systemTime);

			// need to create a new file header
			if (pParent->SetFilePointer(hParentFile, 0) != 0)
				goto Done;

			memset(&header, 0, sizeof(header));
			header.dwIdentifier = MAKE_4CHAR('U','T','F',' ');
			header.dwVersion = UTF_VERSION;

			header.dwNamesOffset = sizeof(header);
			header.dwNameSpaceSize = 4;
			header.dwNameSpaceUsed = 3;

			header.dwDirectoryOffset = header.dwNamesOffset + header.dwNameSpaceSize;
			header.dwDirectorySize   = sizeof(UTF_DIR_ENTRY);

			header.dwDirEntrySize = sizeof(UTF_DIR_ENTRY);

			header.dwDataStartOffset = header.dwDirectoryOffset + header.dwDirectorySize;
			
			SystemTimeToFileTime(&systemTime, &header.LastWriteTime);

			// write out the header

			dwRead=0;
			pParent->WriteFile(hParentFile, &header, sizeof(header), &dwRead, 0);
			if (dwRead != sizeof(header))
				goto Done;

			// write out the name buffer

			pParent->WriteFile(hParentFile, &dwName, sizeof(dwName), &dwRead, 0);
			if (dwRead != sizeof(dwName))
				goto Done;
			
			// write out the directory

			memset(&entry, 0, sizeof(entry));
			entry.dwName = 1;
			entry.dwAttributes = FILE_ATTRIBUTE_DIRECTORY;
			entry.dwSpaceAllocated = 
			entry.dwSpaceUsed = 
			entry.dwUncompressedSize = header.dwDataStartOffset + 4;
			entry.DOSCreationTime =
			entry.DOSLastAccessTime = 
			entry.DOSLastWriteTime = FileTimeToDOSTime(&header.LastWriteTime);

			dwRead=0;
			pParent->WriteFile(hParentFile, &entry, sizeof(entry), &dwRead, 0);
			if (dwRead != sizeof(entry))
				goto Done;

			// write out initial data

			dwName = 0xFFFFFFFF;
			pParent->WriteFile(hParentFile, &dwName, sizeof(dwName), &dwRead, 0);
			if (dwRead != sizeof(dwName))
				goto Done;

			if (lpDesc->dwShareMode == 0)
				locker.enableFastWrite();
			result=1;
		}
		goto Done;
	}
	if (header.dwIdentifier != MAKE_4CHAR('U','T','F',' '))
		goto Done;
	if (header.dwVersion != UTF_VERSION)
		goto Done;

	{
		DirectoryManager dir(locker);
		UTF_DIR_ENTRY *pEntry;

		if (dir.isLocked())
		{
			pEntry = dir.getDirectory();

			if (pEntry->dwName == 1)
			{
		 		result = 1;
				if (locker.anySharing() == 0)
				{
					if (ClearSharingInFiles(pEntry, pEntry))
						dir.updateWriteTime();
				}
			}
		}
	}

Done:
	pParent->SetFilePointer(hParentFile, 0, 0, FILE_BEGIN);

	return result;
}
//--------------------------------------------------------------------------//
//
BOOL SharedUTF::CloseHandle (HANDLE handle)
{
	if (handle==0 || isValidHandle(handle) == 0)
	{
		dwLastError = ERROR_INVALID_HANDLE;
		return 0;
	}

	return pParent->SerialCall(this, (DAFILE_SERIAL_PROC) &SharedUTF::CloseHandle_S, handle);
}
//--------------------------------------------------------------------------//
//
BOOL SharedUTF::ReadFile (HANDLE hFileHandle, LPVOID lpBuffer, DWORD nNumberOfBytesToRead, 
			   LPDWORD lpNumberOfBytesRead,
			   LPOVERLAPPED lpOverlapped)
{
	if (isValidHandle(hFileHandle) == 0)
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

	if (hFileHandle && GetUTFChild(hFileHandle)->Sharing.read == 0)
	{
		dwLastError = ERROR_ACCESS_DENIED;
		return 0;
	}

	// take advantage of parameters on the stack
	return pParent->SerialCall(this, (DAFILE_SERIAL_PROC) &SharedUTF::ReadFile_S, &hFileHandle);
}
//--------------------------------------------------------------------------//
//
LONG SharedUTF::ReadFile_S (LPVOID lpContext)
{
	BOOL result=0;
	UTF_READ_STRUCT *pRead = (UTF_READ_STRUCT *) lpContext;
	DirectoryManager dir(locker);
	UTF_DIR_ENTRY *pEntry;

	if (dir.isLocked() == 0)
		goto Done;

	pEntry = dir.getDirectory();
	if (pRead->hFileHandle && GetDirectoryEntry(GetUTFChild(pRead->hFileHandle)->szFilename, dir.getDirectory(), dir.getNames(), &pEntry) == 0)
	{
		GetUTFChild(pRead->hFileHandle)->szFilename[0] = 0;
		goto Done;
	}

	if (pRead->lpOverlapped)
	{
		// limit the number of bytes read to size of the file
		if (pRead->hFileHandle)
		{
			int iOver;
			DWORD dwFSize = pEntry->dwUncompressedSize;

			pRead->nNumberOfBytesToRead = __min(pRead->nNumberOfBytesToRead, dwFSize);
			iOver = dwFSize - pRead->lpOverlapped->Offset - pRead->nNumberOfBytesToRead;
			if (iOver < 0)
				pRead->nNumberOfBytesToRead += iOver;
		}

		if (pRead->hFileHandle)
			pRead->lpOverlapped->Offset += pEntry->dwDataOffset + dir.getDataStartOffset();
		result = pParent->ReadFile(hParentFile, pRead->lpBuffer, pRead->nNumberOfBytesToRead, pRead->lpNumberOfBytesRead,
				   pRead->lpOverlapped);
		if (pRead->hFileHandle)
			pRead->lpOverlapped->Offset -= pEntry->dwDataOffset + dir.getDataStartOffset();
		if (result==0)
			dwLastError = pParent->GetLastError();
	}
	else
	{
		// limit the number of bytes read to size of the file
		int iOver;
		DWORD dwFSize = pEntry->dwUncompressedSize;
		DWORD dwOffset = GetFilePosition(pRead->hFileHandle);

		pRead->nNumberOfBytesToRead = __min(pRead->nNumberOfBytesToRead, dwFSize);
		iOver = dwFSize - dwOffset - pRead->nNumberOfBytesToRead;
		if (iOver < 0)
			pRead->nNumberOfBytesToRead += iOver;

		dwOffset += pEntry->dwDataOffset + dir.getDataStartOffset();
		
		if (GetFilePosition() != dwOffset)
			SetFilePointer(0, dwOffset);

		result = pParent->ReadFile(hParentFile, pRead->lpBuffer, pRead->nNumberOfBytesToRead, 
									pRead->lpNumberOfBytesRead, 0);

		if (result == 0)
			dwLastError = pParent->GetLastError();
		else
			GetUTFChild(pRead->hFileHandle)->dwFilePosition += *(pRead->lpNumberOfBytesRead);
	}

Done:
	return result;
}
//--------------------------------------------------------------------------//
//
BOOL SharedUTF::WriteFile (HANDLE hFileHandle, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite,
				LPDWORD lpNumberOfBytesWritten, LPOVERLAPPED lpOverlapped)
{
	if (isValidHandle(hFileHandle) == 0)
	{
		dwLastError = ERROR_INVALID_HANDLE;
		return 0;
	}
	if (hFileHandle == 0 && lpOverlapped == 0)
	{
		BOOL result;

		result = pParent->WriteFile(hParentFile, lpBuffer, nNumberOfBytesToWrite, 
									lpNumberOfBytesWritten, 0);

		if (result == 0)
			dwLastError = pParent->GetLastError();

		return result;
	}

	if (hFileHandle && GetUTFChild(hFileHandle)->Sharing.write == 0)
	{
		dwLastError = ERROR_ACCESS_DENIED;
		return 0;
	}
	// take advantage of parameters on the stack
	return pParent->SerialCall(this, (DAFILE_SERIAL_PROC) &SharedUTF::WriteFile_S, &hFileHandle);
}
//--------------------------------------------------------------------------//
//
LONG SharedUTF::WriteFile_S (VOID *lpContext)
{
	BOOL result=0;
	UTF_WRITE_STRUCT *pWrite = (UTF_WRITE_STRUCT *) lpContext;
	DirectoryManager dir(locker);
	UTF_DIR_ENTRY *pEntry;
	SYSTEMTIME systemTime;
	FILETIME fileTime;

	if (dir.isLocked() == 0)
		goto Done;

	pEntry = dir.getDirectory();
	if (pWrite->hFileHandle && GetDirectoryEntry(GetUTFChild(pWrite->hFileHandle)->szFilename, dir.getDirectory(), dir.getNames(), &pEntry) == 0)
	{
		GetUTFChild(pWrite->hFileHandle)->szFilename[0] = 0;
		goto Done;
	}

	if ((LONG)pWrite->nNumberOfBytesToWrite < 0)
	{
		dwLastError = ERROR_INVALID_PARAMETER;
		return 0;
	}

	if (pWrite->lpOverlapped)
	{
		if (pWrite->hFileHandle)
		{
			int iOver;

			// check for overrages
			iOver = pWrite->lpOverlapped->Offset + pWrite->nNumberOfBytesToWrite - pEntry->dwUncompressedSize;
			if (iOver > 0)
				if (dir.updateFileSize(pEntry, pWrite->lpOverlapped->Offset + pWrite->nNumberOfBytesToWrite, UTF_EXTRA_ALLOC(pWrite->lpOverlapped->Offset + pWrite->nNumberOfBytesToWrite)) == 0)
					return 0;
		}

		if (pWrite->hFileHandle)
			pWrite->lpOverlapped->Offset += pEntry->dwDataOffset + dir.getDataStartOffset();
		result = pParent->WriteFile(hParentFile, pWrite->lpBuffer, pWrite->nNumberOfBytesToWrite, pWrite->lpNumberOfBytesWritten,
				   pWrite->lpOverlapped);
		if (pWrite->hFileHandle)
			pWrite->lpOverlapped->Offset -= pEntry->dwDataOffset + dir.getDataStartOffset();
		if (result==0)
			dwLastError = pParent->GetLastError();
	}
	else
	{
		DWORD dwOffset;
		
		dwOffset = GetFilePosition(pWrite->hFileHandle);

		if (pWrite->hFileHandle)
		{
			int iOver;
			// check for overrages
			iOver = dwOffset + pWrite->nNumberOfBytesToWrite - pEntry->dwUncompressedSize;
			if (iOver > 0)
				if (dir.updateFileSize(pEntry, dwOffset + pWrite->nNumberOfBytesToWrite, UTF_EXTRA_ALLOC(dwOffset + pWrite->nNumberOfBytesToWrite)) == 0)
					return 0;
		}

		dwOffset += pEntry->dwDataOffset + dir.getDataStartOffset();
		if (GetFilePosition() != dwOffset)
			SetFilePointer(0, dwOffset);

		result = pParent->WriteFile(hParentFile, pWrite->lpBuffer, pWrite->nNumberOfBytesToWrite, 
						pWrite->lpNumberOfBytesWritten, 0);


		if (result == 0)
			dwLastError = pParent->GetLastError();
		else
			GetUTFChild(pWrite->hFileHandle)->dwFilePosition += *(pWrite->lpNumberOfBytesWritten);
	}

	GetSystemTime(&systemTime);
	SystemTimeToFileTime(&systemTime, &fileTime);
	pEntry->DOSLastWriteTime = FileTimeToDOSTime(&fileTime);

Done:
	return result;
}
//--------------------------------------------------------------------------//
//
BOOL SharedUTF::GetOverlappedResult (HANDLE hFileHandle, LPOVERLAPPED lpOverlapped,
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
DWORD SharedUTF::SetFilePointer (HANDLE hFileHandle, LONG lDistanceToMove, 
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
		DWORD dwFileSize = GetFileSize(hFileHandle);
		
		switch (dwMoveMethod)
		{
		case FILE_BEGIN:
		default:
			break;

		case FILE_CURRENT:
			lDistanceToMove += GetFilePosition(hFileHandle);
			break;

		case FILE_END:
			lDistanceToMove = dwFileSize - lDistanceToMove;
			break;
		}

		if (GetUTFChild(hFileHandle)->Sharing.write == 0)
			if ((DWORD)lDistanceToMove > dwFileSize)
				lDistanceToMove = dwFileSize;

		result = GetUTFChild(hFileHandle)->dwFilePosition = lDistanceToMove;
		if (lpDistanceToMoveHigh)
			*lpDistanceToMoveHigh = 0;
	}
	
	return result;
}
//--------------------------------------------------------------------------//
//
BOOL SharedUTF::SetEndOfFile (HANDLE hFileHandle)
{
	BOOL result=0;

	if (isValidHandle(hFileHandle) == 0)
	{
		dwLastError = ERROR_INVALID_HANDLE;
		goto Done;
	}
	else
	if (hFileHandle == 0)
	{
		result = pParent->SetEndOfFile(hParentFile);
		if (result == 0)
			dwLastError = pParent->GetLastError();
	}
	else
	{
		if (GetUTFChild(hFileHandle)->Sharing.write == 0)
			dwLastError = ERROR_ACCESS_DENIED;
		else
			result = pParent->SerialCall(this, (DAFILE_SERIAL_PROC) &SharedUTF::SetEndOfFile_S, hFileHandle);
	}
	
Done:
	return result;
}
//--------------------------------------------------------------------------//
//
DWORD SharedUTF::GetFileSize (HANDLE hFileHandle, LPDWORD lpFileSizeHigh)
{
	DWORD result;
	
	if (isValidHandle(hFileHandle) == 0)
	{
		dwLastError = ERROR_INVALID_HANDLE;
		result = 0xFFFFFFFF;
	}
	else
	if (hFileHandle == 0)
	{
		result = pParent->GetFileSize(hParentFile, lpFileSizeHigh);
		if (result == 0xFFFFFFFF)
			dwLastError = pParent->GetLastError();
	}
	else
	{
		result = pParent->SerialCall(this, (DAFILE_SERIAL_PROC) &SharedUTF::GetFileSize_S, &hFileHandle);
	}
	return result;
}
//--------------------------------------------------------------------------//
//
BOOL SharedUTF::LockFile (HANDLE hFileHandle,	
					  DWORD dwFileOffsetLow,
					  DWORD dwFileOffsetHigh,
					  DWORD nNumberOfBytesToLockLow,
					  DWORD nNumberOfBytesToLockHigh)
{
	BOOL result;

	if (dwLockCount > 0)
	{
		dwLockCount++;
		return 1;
	}

	result = pParent->LockFile(hParentFile,
								dwFileOffsetLow,
								dwFileOffsetHigh,
								nNumberOfBytesToLockLow,
								nNumberOfBytesToLockHigh);
	if (result == 0)
		dwLastError = pParent->GetLastError();
	else
		dwLockCount++;

	return result;
}
//--------------------------------------------------------------------------//
//
BOOL SharedUTF::UnlockFile (HANDLE hFileHandle, 
						DWORD dwFileOffsetLow,
						DWORD dwFileOffsetHigh,
						DWORD nNumberOfBytesToUnlockLow,
						DWORD nNumberOfBytesToUnlockHigh)
{
	BOOL result;

	if (dwLockCount != 1)
	{
		if (dwLockCount > 0)
			dwLockCount--;
		return 1;
	}
	
	result = pParent->UnlockFile(hParentFile,
									dwFileOffsetLow,
									dwFileOffsetHigh,
									nNumberOfBytesToUnlockLow,
									nNumberOfBytesToUnlockHigh);
	if (result == 0)
		dwLastError = pParent->GetLastError();
	else
		dwLockCount--;

	return result;
}
//--------------------------------------------------------------------------//
//
BOOL SharedUTF::GetFileTime (HANDLE hFileHandle,   LPFILETIME lpCreationTime,
				   LPFILETIME lpLastAccessTime, LPFILETIME lpLastWriteTime)
{
	BOOL result=0;

	if (isValidHandle(hFileHandle) == 0)
	{
		dwLastError = ERROR_INVALID_HANDLE;
	}
	else
	if (hFileHandle == 0)
	{
		result = pParent->GetFileTime(hParentFile, lpCreationTime, lpLastAccessTime, lpLastWriteTime);
		if (result == 0)
			dwLastError = pParent->GetLastError();
	}
	else	// take advantage of parameters on the stack
		result = pParent->SerialCall(this, (DAFILE_SERIAL_PROC) &SharedUTF::GetFileTime_S, &hFileHandle);
	
	return result;
}
//--------------------------------------------------------------------------//
//
BOOL SharedUTF::SetFileTime (HANDLE hFileHandle,   CONST FILETIME *lpCreationTime, 
				  CONST FILETIME *lpLastAccessTime,
				  CONST FILETIME *lpLastWriteTime)
{
	BOOL result=0;

	if (isValidHandle(hFileHandle) == 0)
	{
		dwLastError = ERROR_INVALID_HANDLE;
	}
	else
	if (hFileHandle == 0)
	{
		result = pParent->SetFileTime(hParentFile, lpCreationTime, lpLastAccessTime, lpLastWriteTime);
		if (result == 0)
			dwLastError = pParent->GetLastError();
	}
	else
	{
		if (GetUTFChild(hFileHandle)->Sharing.write == 0)
			dwLastError = ERROR_ACCESS_DENIED;
		else  // take advantage of parameters on the stack
			result = pParent->SerialCall(this, (DAFILE_SERIAL_PROC) &SharedUTF::SetFileTime_S, &hFileHandle);
	}
	
	return result;
}
//--------------------------------------------------------------------------//
//
HANDLE SharedUTF::CreateFileMapping (HANDLE hFileHandle, LPSECURITY_ATTRIBUTES lpFileMappingAttributes,
                                 DWORD flProtect, DWORD dwMaximumSizeHigh, DWORD dwMaximumSizeLow, LPCTSTR lpName)
{
	if (isValidHandle(hFileHandle) == 0)
	{
		dwLastError = ERROR_INVALID_HANDLE;
		return 0;
	}

	dwLastError = ERROR_ACCESS_DENIED;
	return 0;
}
//--------------------------------------------------------------------------//
//
LPVOID SharedUTF::MapViewOfFile (HANDLE hFileMappingObject,
                            DWORD dwDesiredAccess,
                            DWORD dwFileOffsetHigh,
                            DWORD dwFileOffsetLow,
                            DWORD dwNumberOfBytesToMap)
{
	dwLastError = ERROR_INVALID_HANDLE;
	return 0;
}
//--------------------------------------------------------------------------//
//
BOOL SharedUTF::UnmapViewOfFile (LPCVOID lpBaseAddress)
{
	dwLastError = ERROR_ACCESS_DENIED;
	return 0;
}
//--------------------------------------------------------------------------//
//
HANDLE SharedUTF::FindFirstFile (LPCTSTR lpFileName, LPWIN32_FIND_DATA lpFindFileData)
{
	return (HANDLE) pParent->SerialCall(this, (DAFILE_SERIAL_PROC) &SharedUTF::FindFirstFile_S, &lpFileName);
}	
//--------------------------------------------------------------------------//
//
BOOL SharedUTF::FindNextFile (HANDLE hFindFile, LPWIN32_FIND_DATA lpData)
{
	return pParent->SerialCall(this, (DAFILE_SERIAL_PROC) &SharedUTF::FindNextFile_S, &hFindFile);
}
//--------------------------------------------------------------------------//
//
BOOL SharedUTF::FindClose (HANDLE hFindFile)
{
	UTF_SHARED_FF * pFF = GetFF(hFindFile);
   
	if (pFF && pFF->bInUse)
	{
		pFF->bInUse = 0;
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
BOOL SharedUTF::CreateDirectory (LPCTSTR lpPathName, LPSECURITY_ATTRIBUTES lpSecurityAttributes)
{
	return pParent->SerialCall(this, (DAFILE_SERIAL_PROC) &SharedUTF::CreateDirectory_S, (void *)lpPathName);
}
//--------------------------------------------------------------------------//
//
BOOL SharedUTF::RemoveDirectory (LPCTSTR lpPathName)
{
	return pParent->SerialCall(this, (DAFILE_SERIAL_PROC) &SharedUTF::RemoveDirectory_S, (void *)lpPathName);
}
//--------------------------------------------------------------------------//
//
DWORD SharedUTF::GetCurrentDirectory (DWORD nBufferLength, LPTSTR lpBuffer)
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
BOOL SharedUTF::SetCurrentDirectory (LPCTSTR lpPathName)
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
BOOL SharedUTF::DeleteFile  (LPCTSTR lpFileName)
{
	return pParent->SerialCall(this, (DAFILE_SERIAL_PROC) &SharedUTF::DeleteFile_S, (void *)lpFileName);
}
//--------------------------------------------------------------------------//
//
BOOL SharedUTF::MoveFile (LPCTSTR lpExistingFileName, LPCTSTR lpNewFileName)
{
	return pParent->SerialCall(this, (DAFILE_SERIAL_PROC) &SharedUTF::MoveFile_S, (void *)&lpExistingFileName);
}
//--------------------------------------------------------------------------//
//
DWORD SharedUTF::GetFileAttributes (LPCTSTR lpFileName)
{
	return pParent->SerialCall(this, (DAFILE_SERIAL_PROC) &SharedUTF::GetFileAttributes_S, (void *)lpFileName);
}
//--------------------------------------------------------------------------//
//
BOOL SharedUTF::SetFileAttributes (LPCTSTR lpFileName, DWORD dwFileAttributes)
{
	return pParent->SerialCall(this, (DAFILE_SERIAL_PROC) &SharedUTF::SetFileAttributes_S, (void *)&lpFileName);
}
//--------------------------------------------------------------------------//
//
HANDLE SharedUTF::OpenChild (DAFILEDESC *lpDesc)
{
	return (HANDLE) pParent->SerialCall(this, (DAFILE_SERIAL_PROC) &SharedUTF::OpenChild_S, lpDesc);
}
//--------------------------------------------------------------------------//
//
LONG SharedUTF::OpenChild_S (LPVOID lpContext)
{
	DAFILEDESC *lpInfo = (DAFILEDESC *) lpContext;
	LONG result = (LONG) INVALID_HANDLE_VALUE;
	DirectoryManager dir(locker);
	UTF_DIR_ENTRY *pEntry=dir.getDirectory();
	UTF_SHARED_CHILD *child = children.array + 0;

	if (dir.isLocked() == 0)
		goto Done;

	if (lpInfo->lpFileName == 0)
	{
		dwLastError = ERROR_INVALID_PARAMETER;
		goto Done;
	}

	if ((lpInfo->dwDesiredAccess & dwAccess) != lpInfo->dwDesiredAccess)
	{
		dwLastError = ERROR_ACCESS_DENIED;
		goto Done;
	}

	if (lpInfo->dwFlagsAndAttributes & FILE_ATTRIBUTE_DIRECTORY)
	{
		dwLastError = ERROR_INVALID_PARAMETER;
		goto Done;
	}

	if ((lpInfo->dwFlagsAndAttributes & FILE_ATTRIBUTE_READONLY) &&
		(lpInfo->dwDesiredAccess & GENERIC_WRITE))
	{
	 	dwLastError = ERROR_ACCESS_DENIED;
	 	goto Done;
	}

	if ((result = AllocHandle_S()) == (LONG) INVALID_HANDLE_VALUE)
		goto Done;

	child = GetUTFChild((HANDLE)result);

	if (GetAbsolutePath(child->szFilename, lpInfo->lpFileName, MAX_PATH) == 0)
	{
		dwLastError = ERROR_BAD_PATHNAME;
		goto Fail;
	}

	child->dwFilePosition = 0;
	child->Sharing.read          = (lpInfo->dwDesiredAccess & GENERIC_READ) != 0;
	child->Sharing.write         = (lpInfo->dwDesiredAccess & GENERIC_WRITE) != 0;
	child->Sharing.readSharing   = (lpInfo->dwShareMode & FILE_SHARE_READ) != 0;
	child->Sharing.writeSharing	= (lpInfo->dwShareMode & FILE_SHARE_WRITE) != 0;

	switch (lpInfo->dwCreationDistribution)
	{
	case CREATE_NEW:
		{
			if ((pEntry = CreateNewEntry_S(child->szFilename, 1)) == 0)
				goto Fail;

			pEntry->dwAttributes = lpInfo->dwFlagsAndAttributes;
			pEntry->Sharing += child->Sharing;
		}
		break;

	case CREATE_ALWAYS:
		{
			if ((pEntry = CreateNewEntry_S(child->szFilename, 0)) == 0)
				goto Fail;

			// fail if it's already open, or it's a directory

			if (pEntry->dwAttributes != FILE_ATTRIBUTE_UNUSED)
			{
				if ((pEntry->Sharing.read | pEntry->Sharing.write) != 0)
				{
					dwLastError = ERROR_SHARING_VIOLATION;
					goto Fail;
				}
				if (pEntry->dwAttributes & FILE_ATTRIBUTE_DIRECTORY)
				{
					dwLastError = ERROR_DIRECTORY;
					goto Fail;
				}
				if (pEntry->dwAttributes & FILE_ATTRIBUTE_READONLY)
				{
					dwLastError = ERROR_ACCESS_DENIED;
					goto Fail;
				}
			
				// else it's ok. Truncate the file

				if (dir.updateFileSize(pEntry, 0) == 0)
					goto Fail;
			}

			pEntry->dwAttributes = lpInfo->dwFlagsAndAttributes;
			pEntry->Sharing += child->Sharing;
		}
		break;

	
	case OPEN_ALWAYS:
		{
			if ((pEntry = CreateNewEntry_S(child->szFilename, 0)) == 0)
				goto Fail;

			if (pEntry->dwAttributes == FILE_ATTRIBUTE_UNUSED)
				pEntry->dwAttributes = lpInfo->dwFlagsAndAttributes;

			if (pEntry->dwAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				dwLastError = ERROR_DIRECTORY;
				goto Fail;
			}

			if ((pEntry->dwAttributes & FILE_ATTRIBUTE_READONLY) &&
				(lpInfo->dwDesiredAccess & GENERIC_WRITE))
			{
	 			dwLastError = ERROR_ACCESS_DENIED;
	 			goto Fail;
			}

			// check sharing 

			if (pEntry->Sharing.isCompatible(child->Sharing) == 0)
			{
				dwLastError = ERROR_SHARING_VIOLATION;
				goto Fail;
			}

			pEntry->Sharing += child->Sharing;
		}
		break;

	case OPEN_EXISTING:
		{
			if (GetDirectoryEntry(child->szFilename, dir.getDirectory(), dir.getNames(),&pEntry) == 0)
				goto Fail;
  		
			if ((pEntry->dwAttributes & FILE_ATTRIBUTE_READONLY) &&
				(lpInfo->dwDesiredAccess & GENERIC_WRITE))
			{
	 			dwLastError = ERROR_ACCESS_DENIED;
	 			goto Fail;
			}

			if (pEntry->dwAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				dwLastError = ERROR_DIRECTORY;
				goto Fail;
			}

			// check sharing 

			if (pEntry->Sharing.isCompatible(child->Sharing) == 0)
			{
				dwLastError = ERROR_SHARING_VIOLATION;
				goto Fail;
			}

			pEntry->Sharing += child->Sharing;
		}
		break;
			
	case TRUNCATE_EXISTING:
		{
			if (GetDirectoryEntry(child->szFilename, dir.getDirectory(), dir.getNames(), &pEntry) == 0)
				goto Fail;
  		
			if ((pEntry->dwAttributes & FILE_ATTRIBUTE_READONLY) &&
				(lpInfo->dwDesiredAccess & GENERIC_WRITE))
			{
	 			dwLastError = ERROR_ACCESS_DENIED;
	 			goto Fail;
			}

			if (pEntry->dwAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				dwLastError = ERROR_DIRECTORY;
				goto Fail;
			}

			// check sharing 
			if ((pEntry->Sharing.read | pEntry->Sharing.write) != 0)
			{
				dwLastError = ERROR_SHARING_VIOLATION;
				goto Fail;
			}

			// truncate the file
			if (dir.updateFileSize(pEntry, 0) == 0)
				goto Fail;

			pEntry->Sharing += child->Sharing;
		}
		break;

	default:
		dwLastError = ERROR_INVALID_PARAMETER;
		goto Fail;
	}

	dir.updateWriteTime();		// updated because new sharing info

	goto Done;

Fail:
	child->szFilename[0] = 0;
	result = (LONG) INVALID_HANDLE_VALUE;
Done:
	return result;
}
//--------------------------------------------------------------------------//
//
DWORD SharedUTF::GetFilePosition (HANDLE hFileHandle, PLONG pPositionHigh)
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
//--------------------------------------------------------------------------//
// Get absolute path in terms of this file system
//  returns a path with a leading '\\'
//
//
BOOL SharedUTF::GetAbsolutePath (char *lpOutput, LPCTSTR lpInput, LONG lSize)
{
	int len;
	char *ptr;
	BOOL result=1;

	if (lSize <= 0)
		return 0;

	if (lpInput[0] == UTF_SWITCH_CHAR)
	{
		strncpy(lpOutput, lpInput, lSize);
		lpOutput[lSize-1] = 0;
		return 1;
	}

	strncpy(lpOutput, szPathname, lSize);

	// now of the form "\\Path\\"

	if (lpInput[0] == '.' && lpInput[1] == UTF_SWITCH_CHAR)
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
		if (lpInput[0] == UTF_SWITCH_CHAR)
			lpInput++;
	}

	len = strlen(lpOutput);
	if (lSize - len > 0)
		strncpy(lpOutput+len, lpInput, lSize-len);

	return result;
}
//--------------------------------------------------------------------------//
// handle has already been validated
//
LONG SharedUTF::CloseHandle_S (HANDLE handle)
{
	BOOL result=0;
	DirectoryManager dir(locker);
	UTF_DIR_ENTRY *pEntry;
	SYSTEMTIME systemTime;
	FILETIME fileTime;
	UTF_SHARED_CHILD * child = GetUTFChild(handle);

	if (dir.isLocked() == 0)
		goto Done;

	pEntry = dir.getDirectory();
	if (GetDirectoryEntry(child->szFilename, dir.getDirectory(), dir.getNames(), &pEntry) == 0)
	{
		child->szFilename[0] = 0;
		goto Done;
	}

	GetSystemTime(&systemTime);
	SystemTimeToFileTime(&systemTime, &fileTime);
	pEntry->DOSLastAccessTime = FileTimeToDOSTime(&fileTime);
	
	pEntry->Sharing -= child->Sharing;
	child->szFilename[0] = 0;

	dir.updateWriteTime();		// updated because new sharing info
	result++;

Done:
	return result;
}
//--------------------------------------------------------------------------//
//
LONG SharedUTF::CloseAllHandles_S (LPVOID lpContext)
{
	long i,j;
	UTF_SHARED_GROUP *pNode = &children;
	DirectoryManager dir(locker);

	i = j = 1;

	do
	{
		for ( ; i < MAX_GROUP_HANDLES; i++, j++)
		{	
			if (pNode->array[i].szFilename[0] != 0)
				CloseHandle_S((HANDLE)j);
		}
		i = 0;
	} while ((pNode = pNode->pNext) != 0);

	dir.flush();

	return 1;
}
//--------------------------------------------------------------------------//
//
LONG SharedUTF::AllocHandle_S (LPVOID lpContext)
{
	long i,j;
	UTF_SHARED_GROUP *pNode = &children;

	i = j = 1;

	while (1)
	{
		for ( ; i < MAX_GROUP_HANDLES; i++, j++)
		{	
			if (pNode->array[i].szFilename[0] == 0)
			{
				return j;
			}	
		}
		i = 0;
		if (pNode->pNext)
			pNode = pNode->pNext;
		else
		{
			if ((pNode->pNext = new UTF_SHARED_GROUP) != 0)
				pNode = pNode->pNext;
			else
				break;

		}
	}

	dwLastError = ERROR_OUT_OF_STRUCTURES;
	return -1;
}
//--------------------------------------------------------------------------//
//
LONG SharedUTF::AllocHandleFF_S (LPVOID lpContext)
{
	long i,j;
	UTF_SHARED_FFGROUP *pNode = &ffhandles;

	i = j = 0;

	while (1)
	{
		for ( ; i < MAX_GROUP_HANDLES; i++, j++)
		{	
			if (pNode->array[i].bInUse == 0)
			{
				return j+1;
			}	
		}
		i = 0;
		if (pNode->pNext)
			pNode = pNode->pNext;
		else
		{
			if ((pNode->pNext = new UTF_SHARED_FFGROUP) != 0)
				pNode = pNode->pNext;
			else
				break;

		}
	}

	dwLastError = ERROR_OUT_OF_STRUCTURES;
	return -1;
}
//--------------------------------------------------------------------------//
//
LONG SharedUTF::SetEndOfFile_S (HANDLE hFileHandle)
{
	DirectoryManager dir(locker);
	UTF_DIR_ENTRY *pEntry;
	BOOL result=0;
	UTF_SHARED_CHILD * child = GetUTFChild(hFileHandle);

	if (dir.isLocked() == 0)
		goto Done;

	pEntry = dir.getDirectory();
	if (GetDirectoryEntry(child->szFilename, dir.getDirectory(), dir.getNames(), &pEntry) == 0)
	{
		child->szFilename[0] = 0;
		goto Done;
	}

	if (dir.updateFileSize(pEntry, child->dwFilePosition, UTF_EXTRA_ALLOC(child->dwFilePosition)) == 0)
		goto Done;	

	result=1;

Done:
	return result;
}
//--------------------------------------------------------------------------//
//
LONG SharedUTF::GetFileSize_S (LPVOID lpContext)
{	
	HANDLE hFileHandle;
	LPDWORD lpFileSizeHigh;
	DirectoryManager dir(locker);
	UTF_DIR_ENTRY *pEntry;
	BOOL result=0;

	if (dir.isLocked() == 0)
		goto Done;

	hFileHandle    = ((HANDLE *)lpContext)[0];
	lpFileSizeHigh = ((LPDWORD *)lpContext)[1];

	pEntry = dir.getDirectory();
	if (GetDirectoryEntry(GetUTFChild(hFileHandle)->szFilename, dir.getDirectory(), dir.getNames(), &pEntry) == 0)
	{
		GetUTFChild(hFileHandle)->szFilename[0] = 0;
		goto Done;
	}

	result = pEntry->dwUncompressedSize;
	if (lpFileSizeHigh)
		*lpFileSizeHigh = 0;

Done:
	return result;
}
//--------------------------------------------------------------------------//
//
LONG SharedUTF::GetFileTime_S (LPVOID lpContext)
{
	HANDLE hFileHandle;
	LPFILETIME lpCreationTime;
	LPFILETIME lpLastAccessTime;
	LPFILETIME lpLastWriteTime;
	DirectoryManager dir(locker);
	UTF_DIR_ENTRY *pEntry;
	BOOL result=0;

	if (dir.isLocked() == 0)
		goto Done;

	hFileHandle      = ((HANDLE *)lpContext)[0];
	lpCreationTime	 = ((LPFILETIME *)lpContext)[1];
	lpLastAccessTime = ((LPFILETIME *)lpContext)[2];
	lpLastWriteTime	 = ((LPFILETIME *)lpContext)[3];

	pEntry = dir.getDirectory();
	if (GetDirectoryEntry(GetUTFChild(hFileHandle)->szFilename, dir.getDirectory(), dir.getNames(), &pEntry) == 0)
	{
		GetUTFChild(hFileHandle)->szFilename[0] = 0;
		goto Done;
	}

	if (lpCreationTime)
		DOSTimeToFileTime(pEntry->DOSCreationTime, lpCreationTime);
	if (lpLastAccessTime)
		DOSTimeToFileTime(pEntry->DOSLastAccessTime, lpLastAccessTime);
	if (lpLastWriteTime)
		DOSTimeToFileTime(pEntry->DOSLastWriteTime, lpLastWriteTime);
	
	result=1;

Done:
	return result;
}
//--------------------------------------------------------------------------//
//
LONG SharedUTF::SetFileTime_S (LPVOID lpContext)
{
	HANDLE hFileHandle;
	LPFILETIME lpCreationTime;
	LPFILETIME lpLastAccessTime;
	LPFILETIME lpLastWriteTime;
	DirectoryManager dir(locker);
	UTF_DIR_ENTRY *pEntry;
	BOOL result=0;

	if (dir.isLocked() == 0)
		goto Done;

	hFileHandle      = ((HANDLE *)lpContext)[0];
	lpCreationTime	 = ((LPFILETIME *)lpContext)[1];
	lpLastAccessTime = ((LPFILETIME *)lpContext)[2];
	lpLastWriteTime	 = ((LPFILETIME *)lpContext)[3];

	pEntry = dir.getDirectory();
	if (GetDirectoryEntry(GetUTFChild(hFileHandle)->szFilename, dir.getDirectory(), dir.getNames(), &pEntry) == 0)
	{
		GetUTFChild(hFileHandle)->szFilename[0] = 0;
		goto Done;
	}

	if (lpCreationTime)
		pEntry->DOSCreationTime = FileTimeToDOSTime(lpCreationTime);
	if (lpLastAccessTime)
		pEntry->DOSLastAccessTime =  FileTimeToDOSTime(lpLastAccessTime);
	if (lpLastWriteTime)
		pEntry->DOSLastWriteTime = FileTimeToDOSTime(lpLastWriteTime);
	
	dir.updateWriteTime();
	result=1;

Done:
	return result;
}
//--------------------------------------------------------------------------//
//
LONG SharedUTF::FindFirstFile_S (LPVOID lpContext)
{
	DWORD i = (DWORD) INVALID_HANDLE_VALUE;
	char buffer[MAX_PATH+4];
	char *ptr;
	DirectoryManager dir(locker);
	UTF_DIR_ENTRY *pEntry;
	LPCSTR lpFileName = ((LPCSTR *)lpContext)[0];
	LPWIN32_FIND_DATA lpFindFileData = ((LPWIN32_FIND_DATA *)lpContext)[1];
	UTF_SHARED_FF * pFF=0;

	if (dir.isLocked() == 0)
		goto Done;

	if (GetAbsolutePath(buffer, lpFileName, MAX_PATH) == 0)
	{
		dwLastError = ERROR_BAD_PATHNAME;
		return (LONG) INVALID_HANDLE_VALUE;
	}

	pEntry = dir.getDirectory();
	if ((ptr = strrchr(buffer, UTF_SWITCH_CHAR)) != 0)
	{
		if (ptr!=buffer)
		{
			*ptr = 0;
			if (GetDirectoryEntry(buffer, dir.getDirectory(), dir.getNames(), &pEntry) == 0)
			{
				dwLastError = ERROR_BAD_PATHNAME;
				return (LONG) INVALID_HANDLE_VALUE;
			}
		}
		else
		{	
			GetAbsolutePath(buffer+1, lpFileName, MAX_PATH-1);
			ptr[1] = 0;
		}
	}

	// buffer is now of the form:   "\\path", 0, "search pattern"

	// find an empty slot
	i = AllocHandleFF_S(0);
	pFF = GetFF((HANDLE)i);

	if (pFF==0)
	{
		dwLastError = ERROR_OUT_OF_STRUCTURES;
		return (LONG) INVALID_HANDLE_VALUE;
	}

	memcpy(pFF->szDirectory, buffer, sizeof(pFF->szDirectory));
	pFF->bInUse = 1;
	pFF->szPattern = pFF->szDirectory + strlen(buffer) + 1;
	pFF->szPreviousFile[0] = 0;
	pFF->szNextFile[0] = 0;

	if (FindNextFile((HANDLE)i, lpFindFileData) == 0)
	{
		FindClose((HANDLE)i);
		return (LONG) INVALID_HANDLE_VALUE;
	}

Done:
	return (LONG) i;
}
//--------------------------------------------------------------------------//
//
LONG SharedUTF::FindNextFile_S (LPVOID lpContext)
{
	HANDLE hFindFile = ((HANDLE *)lpContext)[0];
	LPWIN32_FIND_DATA lpData = ((LPWIN32_FIND_DATA *)lpContext)[1];
	
	DirectoryManager dir(locker);
	UTF_DIR_ENTRY *pEntry, *pSearchDir;
	UTF_SHARED_FF * pFF=GetFF(hFindFile);
	BOOL result=0;

	if (pFF==0 || pFF->bInUse == 0)
	{
		dwLastError = ERROR_INVALID_HANDLE;
		return 0;
	}

	if (dir.isLocked() == 0)
		goto Done;

	//
	// find the directory we are enumerating
	//

	pSearchDir = dir.getDirectory();
	if (GetDirectoryEntry(pFF->szDirectory, dir.getDirectory(), dir.getNames(), &pSearchDir) == 0)
	{
		dwLastError = ERROR_BAD_PATHNAME;		// can't find search directory
		goto Done;
	}

	//
	// find file to start search
	//

	pEntry = pSearchDir;
	if (pFF->szNextFile[0] && 
		GetDirectoryEntry(pFF->szNextFile, dir.getDirectory(), dir.getNames(), &pEntry))
	{
		goto FoundStart;
	}

	pEntry = pSearchDir;
	if (pFF->szPreviousFile[0] && 
		GetDirectoryEntry(pFF->szPreviousFile, dir.getDirectory(), dir.getNames(), &pEntry))
	{
		// go to next child
		pEntry = (UTF_DIR_ENTRY *) (((char *)dir.getDirectory()) + pEntry->dwNext);

		goto FoundStart;
	}

	//
	// else we could not find previous, so start over
	//
	pEntry = pSearchDir;
	if ((pEntry->dwAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)		// not a directory
	{
		dwLastError = ERROR_FILE_NOT_FOUND;
		goto Done;
	}
	// go to first child
	pEntry = (UTF_DIR_ENTRY *) (((char *)dir.getDirectory()) + pEntry->dwDataOffset);

FoundStart:	
	//
	// starting position has been found, pEntry -> next file to interrogate
	//
	while (pEntry != dir.getDirectory())
	{
		//
		// make sure filename matches the pattern
		//

		if (PatternMatch(dir.getNames() + pEntry->dwName, pFF->szPattern))
			break;

		//
		// else go to next file
		//

		pEntry = (UTF_DIR_ENTRY *) (((char *)dir.getDirectory()) + pEntry->dwNext);
	}

	if (pEntry == dir.getDirectory())
	{
		dwLastError = ERROR_NO_MORE_FILES;		// no more used entries
		goto Done;
	}

	//
	// entry found, fill in the data
	//

	lpData->dwFileAttributes = pEntry->dwAttributes;
	DOSTimeToFileTime(pEntry->DOSCreationTime, &lpData->ftCreationTime);
	DOSTimeToFileTime(pEntry->DOSLastAccessTime, &lpData->ftLastAccessTime);
	DOSTimeToFileTime(pEntry->DOSLastWriteTime, &lpData->ftLastWriteTime);
	lpData->nFileSizeHigh    = 0;
	lpData->nFileSizeLow     = pEntry->dwUncompressedSize;
    lpData->dwReserved0		 = 0;
    lpData->dwReserved1		 = 0;
	strcpy(lpData->cFileName, dir.getNames() + pEntry->dwName);
	lpData->cAlternateFileName[0] = 0;

	//
	// remember where we are for next time
	//

	strcpy(pFF->szPreviousFile, dir.getNames() + pEntry->dwName);
	pFF->szNextFile[0] = 0;

	//
	// remember next file
	//

	pEntry = (UTF_DIR_ENTRY *) (((char *)dir.getDirectory()) + pEntry->dwNext);
	if (pEntry != dir.getDirectory())
		strcpy(pFF->szNextFile, dir.getNames() + pEntry->dwName);

	result=1;

Done:
	return result;
}
//--------------------------------------------------------------------------//
//  Fails if entry already exists
//
UTF_DIR_ENTRY * SharedUTF::CreateNewEntry_S (LPCTSTR lpPathName, BOOL bFailIfExists)
{
 	char buffer[MAX_PATH+4];
	DirectoryManager dir(locker);
	UTF_DIR_ENTRY *pEntry, *pNewDir=0;
	DWORD dwLength;
	char *ptr;

	if (dir.isLocked() == 0)
		goto Done;

	pEntry = dir.getDirectory();

	if (TestValid(lpPathName) == false)
	{
		dwLastError = ERROR_INVALID_NAME;
		goto Done;
	}

	// dig the parent directory out of the pathname

	if (GetAbsolutePath(buffer, lpPathName, MAX_PATH) == 0)
	{
		dwLastError = ERROR_BAD_PATHNAME;
		goto Done;
	}

	dwLength = strlen(buffer);
	if (dwLength > 0 && buffer[dwLength-1] == UTF_SWITCH_CHAR)
		buffer[dwLength-1] = 0;

	// separate the last component from the path
	if ((ptr = strrchr(buffer, UTF_SWITCH_CHAR)) != 0)
	{
		*ptr++ = 0;
		if (ptr > buffer+1 && GetDirectoryEntry(buffer, dir.getDirectory(), dir.getNames(), &pEntry) == 0)
		{
			dwLastError = ERROR_BAD_PATHNAME;
			goto Done;
		}
	}
	else
	{
		dwLastError = ERROR_BAD_PATHNAME;
		goto Done;
	}

	// pEntry-> directory entry of parent directory system
	// ptr -> ASCII name of directory to create

	// make sure it is a directory

	if (pEntry->dwAttributes & ~FILE_ATTRIBUTE_DIRECTORY)
	{
		dwLastError = ERROR_DIRECTORY;
		goto Done;
	}

	// make sure that this entry doesn't already exist

	{
		UTF_DIR_ENTRY *pTmp = pEntry;

		if (GetDirectoryEntry(ptr, dir.getDirectory(), dir.getNames(), &pTmp) == 0)
		{
			if ((pNewDir = dir.addChildEntry(pEntry, UTF_EXTRA_ENTRIES)) == 0)
			{
				dwLastError = ERROR_NOT_ENOUGH_MEMORY;
				goto Done;
			}
			// all old pointers to directory are now invalid (pEntry -> garbage)
			memset(pNewDir, 0, dir.getDirEntrySize());
			pNewDir->dwAttributes = FILE_ATTRIBUTE_UNUSED;
			if ((pNewDir->dwName = FindName(ptr, dir.getNames())) == 0)
				pNewDir->dwName = dir.addName(ptr, UTF_EXTRA_NAME_SPACE);

			SYSTEMTIME systemTime;
			FILETIME fileTime;

			GetSystemTime(&systemTime);
			SystemTimeToFileTime(&systemTime, &fileTime);
			pNewDir->DOSLastAccessTime = 
			pNewDir->DOSLastWriteTime = 
			pNewDir->DOSCreationTime = FileTimeToDOSTime(&fileTime);

			goto Done;
		}

		// else directory entry already exists
		
		if (bFailIfExists)
		{
			dwLastError = ERROR_ALREADY_EXISTS;
			goto Done;
		}

		pNewDir = pTmp;
	}

Done:
	return pNewDir;
}
//--------------------------------------------------------------------------//
//
LONG SharedUTF::CreateDirectory_S (LPVOID lpContext)
{
	LPCTSTR lpPathName = (LPCTSTR) lpContext;
	DirectoryManager dir(locker);
	UTF_DIR_ENTRY *pNewDir;
	BOOL result=0;

	if (dir.isLocked() == 0)
		goto Done;

	if ((dwAccess & GENERIC_WRITE) == 0)
	{
		dwLastError = ERROR_ACCESS_DENIED;
		goto Done;
	}

	if ((pNewDir = CreateNewEntry_S(lpPathName, 1)) == 0)
		goto Done;
	
	// fill in data members of structure

	pNewDir->dwAttributes = FILE_ATTRIBUTE_DIRECTORY;

	result=1;

Done:
	return result;
}
//--------------------------------------------------------------------------//
//
LONG SharedUTF::RemoveDirectory_S (LPVOID lpContext)
{
 	char buffer[MAX_PATH+4];
	LPCTSTR lpPathName = (LPCTSTR) lpContext;
	DirectoryManager dir(locker);
	UTF_DIR_ENTRY *pEntry, *pParent;
	BOOL result=0;
	DWORD dwLength;
	char *ptr;

	if (dir.isLocked() == 0)
		goto Done;

	if ((dwAccess & GENERIC_WRITE) == 0)
	{
		dwLastError = ERROR_ACCESS_DENIED;
		goto Done;
	}
	
	pParent = pEntry = dir.getDirectory();

	if (GetAbsolutePath(buffer, lpPathName, MAX_PATH) == 0)
	{
		dwLastError = ERROR_BAD_PATHNAME;
		goto Done;
	}

	dwLength = strlen(buffer);
	if (dwLength > 0 && buffer[dwLength-1] == UTF_SWITCH_CHAR)
		buffer[dwLength-1] = 0;

	// separate the last component from the path
	if ((ptr = strrchr(buffer, UTF_SWITCH_CHAR)) != 0)
	{
		*ptr++ = 0;
		if (ptr > buffer+1 && GetDirectoryEntry(buffer, dir.getDirectory(), dir.getNames(), &pParent) == 0)
		{
			dwLastError = ERROR_BAD_PATHNAME;
			goto Done;
		}
	}
	else
	{
		dwLastError = ERROR_BAD_PATHNAME;
		goto Done;
	}

	pEntry = pParent;

	if (GetDirectoryEntry(ptr, dir.getDirectory(), dir.getNames(), &pEntry) == 0)
	{
		dwLastError = ERROR_BAD_PATHNAME;
		goto Done;
	}

	// make sure it is a directory

	if ((pEntry->dwAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
	{
		dwLastError = ERROR_DIRECTORY;
		goto Done;
	}
	
	// make sure it is empty

	if (pEntry->dwDataOffset != 0)
	{
		dwLastError = ERROR_DIR_NOT_EMPTY;
		goto Done;
	}

	result = dir.removeEntry(pParent, pEntry);

Done:
	return result;
}
//--------------------------------------------------------------------------//
//
LONG SharedUTF::DeleteFile_S (LPVOID lpContext)
{
 	char buffer[MAX_PATH+4];
	LPCTSTR lpFileName = (LPCTSTR) lpContext;
	DirectoryManager dir(locker);
	UTF_DIR_ENTRY *pEntry, *pParent;
	BOOL result=0;
	char *ptr;

	if (dir.isLocked() == 0)
		goto Done;

	if ((dwAccess & GENERIC_WRITE) == 0)
	{
		dwLastError = ERROR_ACCESS_DENIED;
		goto Done;
	}

	pParent = pEntry = dir.getDirectory();

	if (GetAbsolutePath(buffer, lpFileName, MAX_PATH) == 0)
	{
		dwLastError = ERROR_BAD_PATHNAME;
		goto Done;
	}

	// separate the last component from the path
	if ((ptr = strrchr(buffer, UTF_SWITCH_CHAR)) != 0)
	{
		*ptr++ = 0;
		if (ptr > buffer+1 && GetDirectoryEntry(buffer, dir.getDirectory(), dir.getNames(), &pParent) == 0)
		{
			dwLastError = ERROR_BAD_PATHNAME;
			goto Done;
		}
	}
	else
	{
		dwLastError = ERROR_BAD_PATHNAME;
		goto Done;
	}

	pEntry = pParent;

	if (GetDirectoryEntry(ptr, dir.getDirectory(), dir.getNames(), &pEntry) == 0)
	{
		dwLastError = ERROR_BAD_PATHNAME;
		goto Done;
	}

	// make sure it is NOT a directory

	if (pEntry->dwAttributes & FILE_ATTRIBUTE_DIRECTORY)
	{
		dwLastError = ERROR_DIRECTORY;
		goto Done;
	}
	
	// make sure it is not already open

	if (pEntry->Sharing.read || pEntry->Sharing.write)
	{
		dwLastError = ERROR_SHARING_VIOLATION;
		goto Done;
	}

	if (pEntry->dwDataOffset && dir.freeDataArea(pEntry->dwDataOffset, pEntry->dwSpaceAllocated) == 0)
		goto Done;

	result = dir.removeEntry(pParent, pEntry);

Done:
	return result;
}
//--------------------------------------------------------------------------//
//
LONG SharedUTF::MoveFile_S (LPVOID lpContext)
{
	LPCTSTR lpExistingFileName = ((LPCTSTR *)lpContext)[0];
	LPCTSTR lpNewFileName = ((LPCTSTR *)lpContext)[1];
 	char buffer1[MAX_PATH+4];
 	char buffer2[MAX_PATH+4];

	DirectoryManager dir(locker);
	UTF_DIR_ENTRY *pSrc, *pDst, *pEntry;
	BOOL result=0;
	DWORD dwLength;
	char *ptr1, *ptr2;

	if (dir.isLocked() == 0)
		goto Done;

	if ((dwAccess & GENERIC_WRITE) == 0)
	{
		dwLastError = ERROR_ACCESS_DENIED;
		goto Done;
	}
	
	pEntry = dir.getDirectory();

	// dig the parent directory out of the pathname

	if (GetAbsolutePath(buffer1, lpExistingFileName, MAX_PATH) == 0)
	{
		dwLastError = ERROR_BAD_PATHNAME;
		goto Done;
	}

	dwLength = strlen(buffer1);
	if (dwLength > 0 && buffer1[dwLength-1] == UTF_SWITCH_CHAR)
		buffer1[dwLength-1] = 0;

	// separate the last component from the path
	if ((ptr1 = strrchr(buffer1, UTF_SWITCH_CHAR)) != 0)
	{
		*ptr1++ = 0;
		if (ptr1 > buffer1+1 && GetDirectoryEntry(buffer1, dir.getDirectory(), dir.getNames(), &pEntry) == 0)
		{
			dwLastError = ERROR_BAD_PATHNAME;
			goto Done;
		}
	}
	else
	{
		dwLastError = ERROR_BAD_PATHNAME;
		goto Done;
	}

	pSrc = pEntry;
	// do the same for the destination

	pEntry = dir.getDirectory();

	// dig the parent directory out of the pathname

	if (GetAbsolutePath(buffer2, lpNewFileName, MAX_PATH) == 0)
	{
		dwLastError = ERROR_BAD_PATHNAME;
		goto Done;
	}

	dwLength = strlen(buffer2);
	if (dwLength > 0 && buffer2[dwLength-1] == UTF_SWITCH_CHAR)
		buffer2[dwLength-1] = 0;

	// separate the last component from the path
	if ((ptr2 = strrchr(buffer2, UTF_SWITCH_CHAR)) != 0)
	{
		*ptr2++ = 0;
		if (ptr2 > buffer2+1 && GetDirectoryEntry(buffer2, dir.getDirectory(), dir.getNames(), &pEntry) == 0)
		{
			dwLastError = ERROR_BAD_PATHNAME;
			goto Done;
		}
	}
	else
	{
		dwLastError = ERROR_BAD_PATHNAME;
		goto Done;
	}

	pDst = pEntry;
	if (pSrc != pDst)		// must have same parent 
	{
		dwLastError = ERROR_NOT_SAME_DEVICE;
		goto Done;
	}
	
	// verify that source file exists

	if (GetDirectoryEntry(ptr1, dir.getDirectory(), dir.getNames(), &pSrc) == 0)
	{
		dwLastError = ERROR_BAD_PATHNAME;
		goto Done;
	}

	// verify that dest file does not exist

	if (GetDirectoryEntry(ptr2, dir.getDirectory(), dir.getNames(), &pDst))
	{
		dwLastError = ERROR_ALREADY_EXISTS;
		goto Done;
	}

	// verify that there are no open files within the directory

	if (pSrc->dwAttributes & FILE_ATTRIBUTE_DIRECTORY)
	{
		if (CheckForOpenChild(dir.getDirectory(), pSrc))
		{
			dwLastError = ERROR_PATH_BUSY;
			goto Done;
		}
	}
	else
	{
		if (pSrc->Sharing.isEmpty() == 0)
		{
			dwLastError = ERROR_PATH_BUSY;
			goto Done;
		}
	}

	// change the name of the file / directory

	if ((pSrc->dwName = FindName(ptr2, dir.getNames())) == 0)
		pSrc->dwName = dir.addName(ptr2, UTF_EXTRA_NAME_SPACE);

	result = dir.updateWriteTime();

Done:
	return result;
}
//--------------------------------------------------------------------------//
//
LONG SharedUTF::GetFileAttributes_S (LPVOID lpContext)
{
 	char buffer[MAX_PATH+4];
	LPCTSTR lpFileName = (LPCTSTR) lpContext;
	DirectoryManager dir(locker);
	UTF_DIR_ENTRY *pEntry;
	DWORD result=0xFFFFFFFF;

	if (dir.isLocked() == 0)
		goto Done;

	pEntry = dir.getDirectory();

	if (GetAbsolutePath(buffer, lpFileName, MAX_PATH) == 0)
	{
		dwLastError = ERROR_BAD_PATHNAME;
		goto Done;
	}

	if (GetDirectoryEntry(buffer, dir.getDirectory(), dir.getNames(), &pEntry) == 0)
	{
		dwLastError = ERROR_FILE_NOT_FOUND;
		goto Done;
	}

	result = pEntry->dwAttributes;

Done:
	return result;
}
//--------------------------------------------------------------------------//
//
LONG SharedUTF::SetFileAttributes_S (LPVOID lpContext)
{
	LPCTSTR lpFileName = ((LPCTSTR *)lpContext)[0];
	DWORD dwFileAttributes = ((DWORD *)lpContext)[1];
 	char buffer[MAX_PATH+4];
	DirectoryManager dir(locker);
	UTF_DIR_ENTRY *pEntry;
	BOOL result=0;

	if (dir.isLocked() == 0)
		goto Done;

	if ((dwAccess & GENERIC_WRITE) == 0)
	{
		dwLastError = ERROR_ACCESS_DENIED;
		goto Done;
	}
	
	pEntry = dir.getDirectory();

	if (GetAbsolutePath(buffer, lpFileName, MAX_PATH) == 0)
	{
		dwLastError = ERROR_BAD_PATHNAME;
		goto Done;
	}

	if (GetDirectoryEntry(buffer, dir.getDirectory(), dir.getNames(), &pEntry) == 0)
	{
		dwLastError = ERROR_FILE_NOT_FOUND;
		goto Done;
	}

	pEntry->dwAttributes = dwFileAttributes;
	result = dir.updateWriteTime();

Done:
	return result;
}
//--------------------------------------------------------------------------//
// Assume pEntry points to a directory
//    Return true if actually changed anything
//
BOOL SharedUTF::ClearSharingInFiles (UTF_DIR_ENTRY *pDirectory, UTF_DIR_ENTRY *pEntry)
{
	BOOL result = 0;

	// go to first child
	pEntry = (UTF_DIR_ENTRY *) (((char *)pDirectory) + pEntry->dwDataOffset);

	while (pEntry != pDirectory)
	{
		if (pEntry->Sharing.isEmpty() == 0)
		{
			result=1;
			pEntry->Sharing.setEmpty();
		}

		if (pEntry->dwAttributes & FILE_ATTRIBUTE_DIRECTORY)
			result |= ClearSharingInFiles(pDirectory, pEntry);

		// go to next sibling
		pEntry = (UTF_DIR_ENTRY *) (((char *)pDirectory) + pEntry->dwNext);
	}

	return result;
}
//--------------------------------------------------------------------------//
// Assume that pEntry points to a directory
//    Return true if any child is open
//
BOOL SharedUTF::CheckForOpenChild (UTF_DIR_ENTRY *pDirectory, UTF_DIR_ENTRY *pEntry)
{
	BOOL result = 0;

	// go to first child
	pEntry = (UTF_DIR_ENTRY *) (((char *)pDirectory) + pEntry->dwDataOffset);

	while (pEntry != pDirectory)
	{
		if (pEntry->Sharing.isEmpty() == 0)
		{
			result=1;
			break;
		}

		if (pEntry->dwAttributes & FILE_ATTRIBUTE_DIRECTORY)
			if ((result = CheckForOpenChild(pDirectory, pEntry)) != 0)
				break;

		// go to next sibling
		pEntry = (UTF_DIR_ENTRY *) (((char *)pDirectory) + pEntry->dwNext);
	}

	return result;
}
//--------------------------------------------------------------------------//
//
void SharedUTF::Flush (void)
{
	pParent->SerialCall(this, (DAFILE_SERIAL_PROC) &SharedUTF::Flush_S, NULL);
}
//--------------------------------------------------------------------------//
//
void SharedUTF::AddDirEntries (U32 numEntries)
{
	pParent->SerialCall(this, (DAFILE_SERIAL_PROC) &SharedUTF::AddDirEntries_S, (void*)numEntries);
}
//--------------------------------------------------------------------------//
//
void SharedUTF::AddNameSpace (U32 numBytes)
{
	pParent->SerialCall(this, (DAFILE_SERIAL_PROC) &SharedUTF::AddNameSpace_S, (void*)numBytes);
}
//--------------------------------------------------------------------------//
//
void SharedUTF::HintExpansionDirEntries (U32 numEntries)
{
	UTF_EXTRA_ENTRIES = numEntries;
}
//--------------------------------------------------------------------------//
//
void SharedUTF::HintExpansionNameSpace (U32 numBytes)
{
	UTF_EXTRA_NAME_SPACE = numBytes;
}
//--------------------------------------------------------------------------//
//
LONG SharedUTF::Flush_S (LPVOID lpContext)
{
	DirectoryManager dir(locker);
	dir.flush();
	return 0;
}
//--------------------------------------------------------------------------//
//
LONG SharedUTF::AddDirEntries_S (LPVOID lpContext)
{
	DirectoryManager dir(locker);
	return dir.addEntries(int(lpContext));
}
//--------------------------------------------------------------------------//
//
LONG SharedUTF::AddNameSpace_S (LPVOID lpContext)
{
	DirectoryManager dir(locker);
	return dir.increaseNameSpace(int(lpContext));
}
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
BaseUTF * CreateSharedUTF (DWORD dwSharing)
{ 
	SharedUTF * result = new DAComponent<SharedUTF>;

	if (result)
		result->locker.setSharing(dwSharing);


	return result;
}
//--------------------------------------------------------------------------//
//-------------------------------End UTFShare.cpp--------------------------------//
//--------------------------------------------------------------------------//
