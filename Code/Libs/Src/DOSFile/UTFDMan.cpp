//--------------------------------------------------------------------------//
//                                                                          //
//                               UTFDMan.CPP                                //
//                                                                          //
//               COPYRIGHT (C) 1997 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//

/*
	Directory management for the UTF file system
*/

#include <windows.h>

#include "DACOM.h"
#include "FileSys.h"
#include "BaseUTF.h"
#include "UTFDMan.h"

//--------------------------------------------------------------------------//
//--------------------------BaseLockManager Methods-------------------------//
//--------------------------------------------------------------------------//

//--------------------------------------------------------------------------//
// If lock_count == 0, lock the header, read it into memory, read directory into
//     memory if needed
//
int BaseLockManager::addLock (void)
{
	DWORD dwRead;
	
	if (dwLockCount > 0)
		return ++dwLockCount;

	if (anySharing() && pFile->LockFile(0, 0, 0, sizeof(UTF_HEADER), 0) == 0)
		return 0;

	if (writeSharing() || header.dwIdentifier==0)
	{
		if (pFile->SetFilePointer(0, 0) != 0)
			goto Fail;
		dwRead=0;
		pFile->ReadFile(0, &header, sizeof(header), &dwRead, 0);
		if (dwRead != sizeof(header))
			goto Fail;
		if (header.dwIdentifier != MAKE_4CHAR('U','T','F',' '))
			goto Fail;
		if (header.dwVersion != UTF_VERSION)
			goto Fail;

		if (pDirectory == 0 || pNames == 0 ||
			header.LastWriteTime.dwLowDateTime != OriginalTime.dwLowDateTime ||
			header.LastWriteTime.dwHighDateTime != OriginalTime.dwHighDateTime)
		{
			// header has changed, re-read the directory

			OriginalTime = header.LastWriteTime;

			if (pDirectory)
				VirtualFree(pDirectory, 0, MEM_RELEASE);

			if ((pDirectory = (UTF_DIR_ENTRY *) VirtualAlloc(0, header.dwDirectorySize, MEM_COMMIT, PAGE_READWRITE)) == 0)
				goto Fail;

			if (pNames)
				VirtualFree((void *)pNames, 0, MEM_RELEASE);

			if ((pNames = (LPCTSTR) VirtualAlloc(0, header.dwNameSpaceSize, MEM_COMMIT, PAGE_READWRITE)) == 0)
				goto Fail;

			// read the directory
			
			if (pFile->SetFilePointer(0, header.dwDirectoryOffset) != header.dwDirectoryOffset)
			{
				VirtualFree(pDirectory, 0, MEM_RELEASE);
				pDirectory = 0;
				goto Fail;
			}
			
			dwRead=0;
			pFile->ReadFile(0, pDirectory, header.dwDirectorySize, &dwRead, 0);
			if (dwRead != header.dwDirectorySize)
			{
				VirtualFree(pDirectory, 0, MEM_RELEASE);
				pDirectory = 0;
				goto Fail;
			}

			// read the names
		
			if (pFile->SetFilePointer(0, header.dwNamesOffset) != header.dwNamesOffset)
			{
				VirtualFree(pDirectory, 0, MEM_RELEASE);
				pDirectory = 0;
				goto Fail;
			}
			
			dwRead=0;
			pFile->ReadFile(0, (void *) pNames, header.dwNameSpaceSize, &dwRead, 0);
			if (dwRead != header.dwNameSpaceSize)
			{
				VirtualFree(pDirectory, 0, MEM_RELEASE);
				pDirectory = 0;
				goto Fail;
			}
		}
	}

	return ++dwLockCount;

Fail:
	if (anySharing())
		pFile->UnlockFile(0, 0, 0, sizeof(UTF_HEADER), 0);

	return 0;
}
//--------------------------------------------------------------------------//
//
int BaseLockManager::releaseLock (void)
{
	if (dwLockCount > 1)
		return --dwLockCount;
	else
	if (dwLockCount == 0)
		return 0;

	if (anySharing())
	{
		flushHeader();
		pFile->UnlockFile(0, 0, 0, sizeof(UTF_HEADER), 0);
	}

	return --dwLockCount;
}
//--------------------------------------------------------------------------//
//
void BaseLockManager::flushHeader (void)
{
	if (pFile->GetAccessType() & GENERIC_WRITE)
	{
		if (bUpdated)	// if header (or directory) info has been updated
		{
			DWORD dwWritten=0;
			if (pFile->SetFilePointer(0, 0) == 0)
				pFile->WriteFile(0, &header, sizeof(header), &dwWritten, 0);

			if (dwWritten == sizeof(header))
			{
				pDirectory->dwSpaceUsed = 
				pDirectory->dwSpaceAllocated = 
				pDirectory->dwUncompressedSize = pFile->GetFileSize();
				
				if (pFile->SetFilePointer(0, header.dwDirectoryOffset) == header.dwDirectoryOffset)
					pFile->WriteFile(0, pDirectory, header.dwDirectorySize, &dwWritten, 0);
				if (dwWritten == header.dwDirectorySize)
				{
					if (pFile->SetFilePointer(0, header.dwNamesOffset) == header.dwNamesOffset)
						pFile->WriteFile(0, pNames, header.dwNameSpaceSize, &dwWritten, 0);

					if (dwWritten == header.dwNameSpaceSize)
					{
						OriginalTime = header.LastWriteTime;
						bUpdated = 0;
					}
				}
			}
		}
	}
}
//--------------------------------------------------------------------------//
//
BaseLockManager::~BaseLockManager (void)
{
	if (pDirectory)
		VirtualFree(pDirectory, 0, MEM_RELEASE);
	pDirectory = 0;
	if (pNames)
		VirtualFree((void *)pNames, 0, MEM_RELEASE);
	pNames = 0;
}
//--------------------------------------------------------------------------//
//-----------------------DirectoryManager Methods---------------------------//
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
UTF_DIR_ENTRY * DirectoryManager::addChildEntry (UTF_DIR_ENTRY *pParent, int iExtra)
{
	UTF_DIR_ENTRY *pResult;

	if ((pParent->dwAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
		return 0;

	if (getHeader()->dwUnusedEntryOffset == 0)
	{
		DWORD dwOffset;		// calculate offset within directory

		dwOffset =  ((char *)pParent) - ((char*)getDirectory());

		if (addEntries(iExtra+1) == 0)
			return 0;	// failure for unknown reasons

		pParent = (UTF_DIR_ENTRY *) (((char *)getDirectory()) + dwOffset);
	}

	// now assume that we have some empty space
	// remove an entry from the free list

	pResult = (UTF_DIR_ENTRY *) (((char *)getDirectory()) + getHeader()->dwUnusedEntryOffset - getHeader()->dwDirectoryOffset);
	
	pBase->header.dwUnusedEntryOffset = (pResult->dwNext != 0) ? pResult->dwNext + getHeader()->dwDirectoryOffset : 0;
	pResult->dwNext = 0;
	
	// link the entry into the directory chain

	if (pParent->dwDataOffset == 0)	// first child
	{
		pParent->dwDataOffset = ((char *)pResult) - ((char*)getDirectory());
	}
	else	// insert at the end of the list
	{
		// pParent -> first child
		pParent = (UTF_DIR_ENTRY *) (((char *)getDirectory()) + pParent->dwDataOffset);

		while (pParent->dwNext)
		{
			// pParent -> first sibling
			pParent = (UTF_DIR_ENTRY *) (((char *)getDirectory()) + pParent->dwNext);
		}
		pParent->dwNext = ((char *)pResult) - ((char*)getDirectory());
	}
	
	// update the header
	updateWriteTime();

	return pResult;
}
//--------------------------------------------------------------------------//
//
HANDLE DirectoryManager::makeTempFile (void)
{
	char path[MAX_PATH+4];
	char tempname[MAX_PATH+4];
	DWORD dwPrefix = MAKE_4CHAR('U','T','F',0);

	GetTempPath(MAX_PATH, path);	

	if (GetTempFileName(path, (const char *)&dwPrefix, 0, tempname) == 0)
		return 0;

 	return ::CreateFile(tempname, 
							GENERIC_READ|GENERIC_WRITE,
				             0,
					         0,
	                         OPEN_EXISTING,
	                         FILE_ATTRIBUTE_TEMPORARY|FILE_FLAG_DELETE_ON_CLOSE,
						     0);
}


//--------------------------------------------------------------------------//
// insert 'iNumEntries' at the end of the directory table (enlarge the table)
//
BOOL DirectoryManager::addEntries (int iNumEntries)
{
	UTF_DIR_ENTRY *pDirectory = getDirectory();
	DWORD dwStart, dwLength, dwOldDirLength, dwNewDirLength;
	HANDLE hTmpFile = INVALID_HANDLE_VALUE;
	UTF_DIR_ENTRY *pNewDir = 0;

	if (iNumEntries <= 0)
		goto Fail;

	dwOldDirLength = getHeader()->dwDirectorySize;
	dwNewDirLength = dwOldDirLength + (iNumEntries * getHeader()->dwDirEntrySize);

	dwStart  = getHeader()->dwDirectoryOffset + dwOldDirLength;
	dwLength = pDirectory->dwSpaceUsed - dwStart;

	if (isFastWrite() == 0)
	{
		if (pBase->pFile->SetFilePointer(0, dwStart) != dwStart)
			goto Fail;

		if ((hTmpFile = makeTempFile()) == INVALID_HANDLE_VALUE || hTmpFile==0)
			goto Fail;
	}

	if ((pNewDir = (UTF_DIR_ENTRY *) VirtualAlloc(0, dwNewDirLength, MEM_COMMIT, PAGE_READWRITE)) == 0)
		goto Fail;
	
	if (isFastWrite() == 0)
	{
		if (copyTmpData(hTmpFile, pBase->pFile, dwLength) == 0)
			goto Fail;

		dwStart  = getHeader()->dwDirectoryOffset + dwNewDirLength;
		if (pBase->pFile->SetFilePointer(0, dwStart) != dwStart)
			goto Fail;

		if (::SetFilePointer(hTmpFile, 0, 0, FILE_BEGIN) != 0)
			goto Fail;

		if (dwLength == 0)
		{
			if (pBase->pFile->SetEndOfFile() == 0)
				goto Fail;
		}
		else
		if (copyTmpData(pBase->pFile, hTmpFile, dwLength) == 0)
		{
			pBase->pFile->GetFileTime(0, 0, 0, &getHeader()->LastWriteTime);
			goto Fail;
		}
	}

	memcpy(pNewDir, pDirectory, dwOldDirLength);
	VirtualFree(pDirectory, 0, MEM_RELEASE);
	pBase->pDirectory = pDirectory = pNewDir;

	if (isFastWrite() == 0)
	{
		pDirectory->dwSpaceUsed = 
		pDirectory->dwSpaceAllocated = 
		pDirectory->dwUncompressedSize = pBase->pFile->GetFileSize();
	}

	// initialize the new entries
	pNewDir = (UTF_DIR_ENTRY *) (((char *)pDirectory) + dwOldDirLength);
	memset(pNewDir, 0, dwNewDirLength - dwOldDirLength);	// Virtual Alloc has already set this correctly

	while (iNumEntries > 1)
	{
		pNewDir->dwNext = ((char *)pNewDir) - ((char *)getDirectory()) + getHeader()->dwDirEntrySize;
		pNewDir->dwAttributes = FILE_ATTRIBUTE_UNUSED;
		pNewDir = (UTF_DIR_ENTRY *) (((char *)pNewDir) + getHeader()->dwDirEntrySize);
		iNumEntries--;
	}

	// link the last element into the unused chain

	pNewDir->dwNext = (getHeader()->dwUnusedEntryOffset != 0) ? getHeader()->dwUnusedEntryOffset - getHeader()->dwDirectoryOffset : 0;
	pNewDir->dwAttributes = FILE_ATTRIBUTE_UNUSED;
	pBase->header.dwUnusedEntryOffset = dwOldDirLength + getHeader()->dwDirectoryOffset;


	// go through every entry in the header --
	// for each whose data offset started after 'dwStart', add the increment value

	if (isFastWrite() == 0)
	{
		dwStart  = getHeader()->dwDirectoryOffset + dwOldDirLength;
		dwLength = dwNewDirLength - dwOldDirLength;

		if (pBase->header.dwNamesOffset >= dwStart)
			pBase->header.dwNamesOffset += dwLength;

		if (pBase->header.dwDataStartOffset >= dwStart)
			pBase->header.dwDataStartOffset += dwLength;

		if (pBase->header.dwUnusedSpaceOffset >= dwStart)
			pBase->header.dwUnusedSpaceOffset += dwLength;
	}
	
	// update the header
	pBase->header.dwDirectorySize = dwNewDirLength;

	updateWriteTime();

	if (hTmpFile != INVALID_HANDLE_VALUE)
		::CloseHandle(hTmpFile);

	return 1;

Fail:
	if (pNewDir)
		VirtualFree(pNewDir, 0, MEM_RELEASE);
	if (hTmpFile != INVALID_HANDLE_VALUE)
		::CloseHandle(hTmpFile);
	return 0;
}
//--------------------------------------------------------------------------//
//
BOOL DirectoryManager::copyTmpData (HANDLE hTmpFile, LPFILESYSTEM pSrc, DWORD dwLength)
{
	char buffer[2048];
	DWORD dwDataRead, dwDataToRead, dwDataWritten;

	while (dwLength > 0)
	{
		dwDataToRead = __min(2048, dwLength);
		if (pSrc->ReadFile(0, buffer, dwDataToRead, &dwDataRead, 0) == 0)
			goto Fail;
		if (dwDataRead == 0)
			break;
		if (::WriteFile(hTmpFile, buffer, dwDataRead, &dwDataWritten, 0) == 0)
			goto Fail;
		if (dwDataWritten != dwDataRead)
			goto Fail;
		dwLength -= dwDataRead;
	}

	return 1;
Fail:
	return 0;
}
//--------------------------------------------------------------------------//
//
BOOL DirectoryManager::copyTmpData (LPFILESYSTEM pDst, HANDLE hTmpFile, DWORD dwLength)
{
	char buffer[2048];
	DWORD dwDataRead, dwDataToRead, dwDataWritten;

	while (dwLength > 0)
	{
		dwDataToRead = __min(2048, dwLength);
		if (::ReadFile(hTmpFile, buffer, dwDataToRead, &dwDataRead, 0) == 0)
			goto Fail;
		if (dwDataRead == 0)
			break;
		if (pDst->WriteFile(0, buffer, dwDataRead, &dwDataWritten, 0) == 0)
			goto Fail;
		if (dwDataWritten != dwDataRead)
			goto Fail;
		dwLength -= dwDataRead;
	}

	return 1;
Fail:
	return 0;
}
//--------------------------------------------------------------------------//
//
BOOL DirectoryManager::copyTmpData (DWORD dwDstOffset, DWORD dwSrcOffset, DWORD dwLength)
{
	char buffer[2048];
	DWORD dwDataRead, dwDataToRead, dwDataWritten;

	while (dwLength > 0)
	{
		dwDataToRead = __min(2048, dwLength);
		if (pBase->pFile->SetFilePointer(0, dwSrcOffset) != dwSrcOffset)
			goto Fail;
		if (pBase->pFile->ReadFile(0, buffer, dwDataToRead, &dwDataRead, 0) == 0)
			goto Fail;
		if (dwDataRead == 0)
			break;
		dwSrcOffset += dwDataRead;
		if (pBase->pFile->SetFilePointer(0, dwDstOffset) != dwDstOffset)
			goto Fail;
		if (pBase->pFile->WriteFile(0, buffer, dwDataRead, &dwDataWritten, 0) == 0)
			goto Fail;
		if (dwDataWritten != dwDataRead)
			goto Fail;
		dwDstOffset += dwDataRead;
		dwLength -= dwDataRead;
	}

	return 1;
Fail:
	return 0;
}
//--------------------------------------------------------------------------//
//
BOOL DirectoryManager::updateWriteTime (void)
{
	SYSTEMTIME systemTime;

	pBase->bUpdated = 1;
	GetSystemTime(&systemTime);

	return SystemTimeToFileTime(&systemTime, &getHeader()->LastWriteTime);
}
//--------------------------------------------------------------------------//
//
BOOL DirectoryManager::removeEntry (UTF_DIR_ENTRY *pParent, UTF_DIR_ENTRY * pEntry)
{
	UTF_DIR_ENTRY *pStart;

	if ((pParent->dwAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0 || 
		pParent->dwDataOffset == 0)
	{
		return 0;
	}

	pStart = (UTF_DIR_ENTRY *) (((char *)getDirectory()) + pParent->dwDataOffset);
	
	// unlink the entry from the directory chain

	if (pStart == pEntry)	// first child
	{
		pParent->dwDataOffset = pEntry->dwNext;
	}
	else	// remove from the sibling list
	{
		DWORD dwEntry;

		dwEntry = ((char *)pEntry) - ((char *)getDirectory());

		while (pStart->dwNext && pStart->dwNext != dwEntry)
		{
			// go to next sibling
			pStart = (UTF_DIR_ENTRY *) (((char *)getDirectory()) + pStart->dwNext);
		}

		if (pStart->dwNext == 0)
			return 0;	// could not find the entry

		// unlink the entry
		pStart->dwNext = pEntry->dwNext;
	}
	
	// link it into the unused list

	memset(pEntry, 0, getHeader()->dwDirEntrySize);
	pEntry->dwAttributes = FILE_ATTRIBUTE_UNUSED;
	pEntry->dwNext = (getHeader()->dwUnusedEntryOffset != 0) ? getHeader()->dwUnusedEntryOffset - getHeader()->dwDirectoryOffset : 0;
	pBase->header.dwUnusedEntryOffset = ((char *)pEntry) - ((char *)getDirectory()) + getHeader()->dwDirectoryOffset;

	// update the header
	return updateWriteTime();
}
//--------------------------------------------------------------------------//
//
BOOL DirectoryManager::updateFileSize (UTF_DIR_ENTRY *pEntry, DWORD _dwNewFileSize, DWORD dwExtra)
{
	DWORD dwNewFileSize = (_dwNewFileSize + 7) & ~7;
	
	// will new size fit within currently allocated block?

	if (dwNewFileSize <= pEntry->dwSpaceAllocated)
	{
		pEntry->dwSpaceUsed = 
		pEntry->dwUncompressedSize = _dwNewFileSize;
		return updateWriteTime();
	}

	// is this block at the end of the file?

	if (pEntry->dwDataOffset && getHeader()->dwDataStartOffset+pEntry->dwDataOffset+pEntry->dwSpaceAllocated >= pBase->pFile->GetFileSize())
	{
		if (pBase->pFile->SetFilePointer(0, getHeader()->dwDataStartOffset+pEntry->dwDataOffset+dwNewFileSize) != getHeader()->dwDataStartOffset+pEntry->dwDataOffset+dwNewFileSize)
			goto Fail;
		if (pBase->pFile->SetEndOfFile() == 0)
			goto Fail;

		UTF_DIR_ENTRY *pDirectory = getDirectory();

		pDirectory->dwSpaceUsed = 
		pDirectory->dwSpaceAllocated = 
		pDirectory->dwUncompressedSize = pBase->pFile->GetFileSize();

		pEntry->dwSpaceUsed = 
		pEntry->dwUncompressedSize = _dwNewFileSize;
		pEntry->dwSpaceAllocated = dwNewFileSize;

		return updateWriteTime();
	}

	// else we must reallocate ourselves

	DWORD dwNewOffset;
	
	dwNewFileSize = (_dwNewFileSize + dwExtra + 7) & ~7;
	if ((dwNewOffset = allocDataArea(&dwNewFileSize)) == 0)
		goto Fail;
	if (pEntry->dwDataOffset && copyTmpData(dwNewOffset+getHeader()->dwDataStartOffset, pEntry->dwDataOffset+getHeader()->dwDataStartOffset, pEntry->dwSpaceUsed) == 0)
		goto Fail;
	
	// free the old block

	if (pEntry->dwDataOffset && freeDataArea(pEntry->dwDataOffset, pEntry->dwSpaceAllocated) == 0)
		goto Fail;

	// set up the new pointers

	pEntry->dwDataOffset = dwNewOffset;
	pEntry->dwSpaceAllocated = dwNewFileSize;
	pEntry->dwSpaceUsed = 
	pEntry->dwUncompressedSize = _dwNewFileSize;

	return updateWriteTime();

Fail:
	return 0;
}
//--------------------------------------------------------------------------//
// 'dwStartOffset' is relative to the start of the data area
//
BOOL DirectoryManager::freeDataArea (DWORD dwStartOffset, DWORD dwLength)
{
	DWORD dwBlock = pBase->header.dwUnusedSpaceOffset;		// absolute offset
	DWORD info[2];		// size, next offset
	DWORD dwPrev=0;		// absolute offset
	DWORD dwBytesRead;
	
	if (dwLength < 8 || dwStartOffset==0)
		goto Fail;

	// is this block at the end of the file?

	if (dwStartOffset+dwLength+getHeader()->dwDataStartOffset >= pBase->pFile->GetFileSize())
	{
		if (pBase->pFile->SetFilePointer(0, dwStartOffset+getHeader()->dwDataStartOffset) != dwStartOffset+getHeader()->dwDataStartOffset)
			goto Fail;
		if (pBase->pFile->SetEndOfFile() == 0)
			goto Fail;

		UTF_DIR_ENTRY *pDirectory = getDirectory();

		pDirectory->dwSpaceUsed = 
		pDirectory->dwSpaceAllocated = 
		pDirectory->dwUncompressedSize = pBase->pFile->GetFileSize();

		return 1;
	}
	
	// search for a bigger block

	while (dwBlock != 0)
	{
		if (pBase->pFile->SetFilePointer(0, dwBlock) != dwBlock)
			goto Fail;
		dwBytesRead=0;
		pBase->pFile->ReadFile(0, info, sizeof(info), &dwBytesRead, 0);
		if (dwBytesRead != sizeof(info))
			goto Fail;
		if (info[0] >= dwLength)
			break;
		dwPrev = dwBlock;
		dwBlock = (info[1] != 0) ? info[1] + getHeader()->dwDataStartOffset : 0;
	}

	// if dwBlock == 0, there were not any blocks bigger than us

	if (dwBlock == 0)
	{
		if (dwPrev == 0)		// if nothing in list!?
			pBase->header.dwUnusedSpaceOffset = dwStartOffset+getHeader()->dwDataStartOffset;
		else
		{
			if (pBase->pFile->SetFilePointer(0, dwPrev+4) != dwPrev+4)
				goto Fail;
			if (pBase->pFile->WriteFile(0, &dwStartOffset, sizeof(DWORD), &dwBytesRead, 0) == 0)
				goto Fail;
		}
		// write our "size" and "next" field
		
		if (pBase->pFile->SetFilePointer(0, dwStartOffset+getHeader()->dwDataStartOffset) != dwStartOffset+getHeader()->dwDataStartOffset)
			goto Fail;
		if (pBase->pFile->WriteFile(0, &dwLength, sizeof(DWORD), &dwBytesRead, 0) == 0)
			goto Fail;
		dwStartOffset = 0;		// write "next = 0"
		if (pBase->pFile->WriteFile(0, &dwStartOffset, sizeof(DWORD), &dwBytesRead, 0) == 0)
			goto Fail;

		pBase->header.dwUnusedSpaceSize += dwLength;
		return 1;
	}

	// else dwBlock is the offset of a block that is bigger than us
	
	if (dwPrev == 0)		// if no prev
		pBase->header.dwUnusedSpaceOffset = dwStartOffset+getHeader()->dwDataStartOffset;
	else
	{
		if (pBase->pFile->SetFilePointer(0, dwPrev+4) != dwPrev+4)
			goto Fail;
		if (pBase->pFile->WriteFile(0, &dwStartOffset, sizeof(DWORD), &dwBytesRead, 0) == 0)
			goto Fail;
	}

	// write our "size" and "next" field

	if (pBase->pFile->SetFilePointer(0, dwStartOffset+getHeader()->dwDataStartOffset) != dwStartOffset+getHeader()->dwDataStartOffset)
		goto Fail;
	if (pBase->pFile->WriteFile(0, &dwLength, sizeof(DWORD), &dwBytesRead, 0) == 0)
		goto Fail;
	dwBlock -= getHeader()->dwDataStartOffset;
	if (pBase->pFile->WriteFile(0, &dwBlock, sizeof(DWORD), &dwBytesRead, 0) == 0)
		goto Fail;

	pBase->header.dwUnusedSpaceSize += dwLength;
	return 1;

Fail:
	return 0;
}
//--------------------------------------------------------------------------//
// return 0 if failed.
// on success, return offset to data area (releative to the data start offset), 
///   *pdwBytesNeeded = actual size allocated
//
DWORD DirectoryManager::allocDataArea (PDWORD pdwBytesNeeded)
{
	DWORD dwBytesNeeded = (*pdwBytesNeeded + 7) & ~7;	 // insure minimum allocation
	DWORD dwBlock = pBase->header.dwUnusedSpaceOffset;	 // absolute offset
	DWORD info[2];		// size, next offset
	DWORD dwPrev=0;										 // absolute offset
	DWORD dwBytesRead;
	DWORD dwResult=0;

	// search for a big enough block

	while (dwBlock != 0)
	{
		if (pBase->pFile->SetFilePointer(0, dwBlock) != dwBlock)
			goto Fail;
		dwBytesRead=0;
		pBase->pFile->ReadFile(0, info, sizeof(info), &dwBytesRead, 0);
		if (dwBytesRead != sizeof(info))
			goto Fail;

		if (info[0] >= dwBytesNeeded && info[0] < dwBytesNeeded*2)		// not too big or too small
			break;
		dwPrev = dwBlock;
		dwBlock = (info[1] != 0) ? info[1] + getHeader()->dwDataStartOffset : 0;
	}

	// if block == 0, we did not find anything,
	//    so create a block at the end of the file

	if (dwBlock == 0)
	{
		if ((dwResult = pBase->pFile->GetFileSize()) == 0xFFFFFFFF)
			goto Fail;
		if (pBase->pFile->SetFilePointer(0, dwResult + dwBytesNeeded) != dwResult+dwBytesNeeded)
			goto Fail;
		if (pBase->pFile->SetEndOfFile() == 0)
			goto Fail;

		UTF_DIR_ENTRY *pDirectory = getDirectory();

		pDirectory->dwSpaceUsed = 
		pDirectory->dwSpaceAllocated = 
		pDirectory->dwUncompressedSize = pBase->pFile->GetFileSize();
			
		*pdwBytesNeeded = dwBytesNeeded;
		return dwResult - getHeader()->dwDataStartOffset;
	}

	// else we will be taking one of the free blocks
	// remove it from the list
	// set the previous pointer to our next

	if (dwPrev == 0)		// if no prev
		pBase->header.dwUnusedSpaceOffset = (info[1] != 0) ? info[1] + getHeader()->dwDataStartOffset : 0;
	else
	{
		if (pBase->pFile->SetFilePointer(0, dwPrev+4) != dwPrev+4)
			goto Fail;
		if (pBase->pFile->WriteFile(0, &info[1], sizeof(DWORD), &dwBytesRead, 0) == 0)
			goto Fail;

	}

	pBase->header.dwUnusedSpaceSize -= info[0];

	*pdwBytesNeeded = info[0];
	return dwBlock - getHeader()->dwDataStartOffset;

Fail:
	return 0;
}
//--------------------------------------------------------------------------//
// Always adds the name unless it is a string of length 0
//   Returns the offset within the name buffer.
//
DWORD DirectoryManager::addName (LPCTSTR lpName, U32 extraSpace)
{
	DWORD result = 0;
	DWORD len;

	if ((len = strlen(lpName)) == 0)
		goto Done;
/*
	DWORD dwNamesOffset;		// offset to buffer containing all of the ASCIIZ filenames
	DWORD dwNameSpaceSize;		// size of the names buffer
	DWORD dwNameSpaceUsed;		// number of bytes actually used in the names buffer
*/

	// max usable space = total size - space used - 1
	// space needed = len+1

	if (len+2 > getHeader()->dwNameSpaceSize - getHeader()->dwNameSpaceUsed)
	{
		// need to increase size of name area

		if (increaseNameSpace(len+1+extraSpace) == 0)
			goto Done;
	}

	// else assume we enough room 		
		
	memcpy((char *)getNames() + getHeader()->dwNameSpaceUsed, lpName, len);
	result = pBase->header.dwNameSpaceUsed;
	pBase->header.dwNameSpaceUsed += len+1;
	updateWriteTime();

Done:
	return result;
}
//--------------------------------------------------------------------------//
//
BOOL DirectoryManager::increaseNameSpace (int iNumBytes)
{
	LPCTSTR pNames = getNames();
	DWORD dwStart, dwLength, dwOldNameLength, dwNewNameLength;
	HANDLE hTmpFile = INVALID_HANDLE_VALUE;
	char *pNewName = 0;

	if (iNumBytes <= 0)
		goto Fail;

	dwOldNameLength = getHeader()->dwNameSpaceSize;
	dwNewNameLength = (dwOldNameLength + iNumBytes + 7) & ~7;

	dwStart  = getHeader()->dwNamesOffset + dwOldNameLength;
	dwLength = getDirectory()->dwSpaceUsed - dwStart;

	if (isFastWrite() == 0)
	{
		if (pBase->pFile->SetFilePointer(0, dwStart) != dwStart)
			goto Fail;

		if ((hTmpFile = makeTempFile()) == INVALID_HANDLE_VALUE)
			goto Fail;
	}

	if ((pNewName = (char *) VirtualAlloc(0, dwNewNameLength, MEM_COMMIT, PAGE_READWRITE)) == 0)
		goto Fail;
	
	if (isFastWrite() == 0)
	{
		if (copyTmpData(hTmpFile, pBase->pFile, dwLength) == 0)
			goto Fail;

		dwStart  = getHeader()->dwNamesOffset + dwNewNameLength;
		if (pBase->pFile->SetFilePointer(0, dwStart) != dwStart)
			goto Fail;

		if (::SetFilePointer(hTmpFile, 0, 0, FILE_BEGIN) != 0)
			goto Fail;

		if (dwLength == 0)
		{
			if (pBase->pFile->SetEndOfFile() == 0)
				goto Fail;
		}
		else
		if (copyTmpData(pBase->pFile, hTmpFile, dwLength) == 0)
		{
			pBase->pFile->GetFileTime(0, 0, 0, &getHeader()->LastWriteTime);
			goto Fail;
		}
	}

	memcpy(pNewName, pNames, dwOldNameLength);
	VirtualFree((void *)pNames, 0, MEM_RELEASE);
	pBase->pNames = pNames = pNewName;

	if (isFastWrite() == 0)
	{
		pBase->pDirectory->dwSpaceUsed = 
		pBase->pDirectory->dwSpaceAllocated = 
		pBase->pDirectory->dwUncompressedSize = pBase->pFile->GetFileSize();
	}

	// initialize the new entries
	pNewName = (char *) (((char *)pNames) + dwOldNameLength);
	memset(pNewName, 0, dwNewNameLength - dwOldNameLength);	// Virtual Alloc has already set this correctly

	// go through every entry in the header --
	// for each whose data offset started after 'dwStart', add the increment value

	if (isFastWrite() == 0)
	{
		dwStart  = getHeader()->dwNamesOffset + dwOldNameLength;
		dwLength = dwNewNameLength - dwOldNameLength;

		if (pBase->header.dwDirectoryOffset >= dwStart)
			pBase->header.dwDirectoryOffset += dwLength;

		if (pBase->header.dwUnusedEntryOffset >= dwStart)
			pBase->header.dwUnusedEntryOffset += dwLength;

		if (pBase->header.dwDataStartOffset >= dwStart)
			pBase->header.dwDataStartOffset += dwLength;

		if (pBase->header.dwUnusedSpaceOffset >= dwStart)
			pBase->header.dwUnusedSpaceOffset += dwLength;
	}

	
	// update the header
	pBase->header.dwNameSpaceSize = dwNewNameLength;

	updateWriteTime();

	if (hTmpFile != INVALID_HANDLE_VALUE)
		::CloseHandle(hTmpFile);

	return 1;

Fail:
	if (pNewName)
		VirtualFree(pNewName, 0, MEM_RELEASE);
	if (hTmpFile != INVALID_HANDLE_VALUE)
		::CloseHandle(hTmpFile);
	return 0;
}
//--------------------------------------------------------------------------//
//  Insert enough space at head of file for names and directory buffers
//
BOOL DirectoryManager::flushWriteCache (void)
{
	DWORD dwStart, dwLength, dwOldHeaderLength, dwNewHeaderLength;
	HANDLE hTmpFile = INVALID_HANDLE_VALUE;
	void *vBuffer=0;

	if (isFastWrite() == 0)
		return 1;

	dwOldHeaderLength = getHeader()->dwDataStartOffset;
	dwNewHeaderLength = sizeof(UTF_HEADER) + getHeader()->dwDirectorySize + getHeader()->dwNameSpaceSize;

	if (dwOldHeaderLength == dwNewHeaderLength)
		return 1;		// nothing to do

	dwStart  = getHeader()->dwDataStartOffset;
	dwLength = getDirectory()->dwSpaceUsed - dwStart;

	if (pBase->pFile->SetFilePointer(0, dwStart) != dwStart)
		goto Fail;

	if (dwLength == 0 || (vBuffer = VirtualAlloc(0, dwLength, MEM_COMMIT, PAGE_READWRITE)) == 0)
	{
		if ((hTmpFile = makeTempFile()) == INVALID_HANDLE_VALUE)
			goto Fail;
		if (copyTmpData(hTmpFile, pBase->pFile, dwLength) == 0)
			goto Fail;
	}
	else
	{
		DWORD dwRead=0;

		pBase->pFile->ReadFile(0, vBuffer, dwLength, &dwRead, 0);
		if (dwRead != dwLength)
			goto Fail;
	}

	dwStart  = dwNewHeaderLength;
	if (pBase->pFile->SetFilePointer(0, dwStart) != dwStart)
		goto Fail;


	if (vBuffer == 0)
	{
		if (::SetFilePointer(hTmpFile, 0, 0, FILE_BEGIN) != 0)
			goto Fail;

		if (dwLength == 0)
		{
			if (pBase->pFile->SetEndOfFile() == 0)
				goto Fail;
		}
		else
		if (copyTmpData(pBase->pFile, hTmpFile, dwLength) == 0)
		{
			pBase->pFile->GetFileTime(0, 0, 0, &getHeader()->LastWriteTime);
			goto Fail;
		}
	}
	else
	{
		DWORD dwWrite=0;

		pBase->pFile->WriteFile(0, vBuffer, dwLength, &dwWrite, 0);
		if (dwWrite != dwLength)
			goto Fail;
	}

	pBase->pDirectory->dwSpaceUsed = 
	pBase->pDirectory->dwSpaceAllocated = 
	pBase->pDirectory->dwUncompressedSize = pBase->pFile->GetFileSize();

	// update the header

//	dwStart  = getHeader()->dwNamesOffset + dwOldNameLength;
  
	// put directory just after the names
	dwLength = pBase->header.dwNamesOffset + pBase->header.dwNameSpaceSize; 
	dwLength -= pBase->header.dwDirectoryOffset;
	
	pBase->header.dwDirectoryOffset += dwLength;
	if (pBase->header.dwUnusedEntryOffset)
		pBase->header.dwUnusedEntryOffset += dwLength;

	// move the data pointers
	
	dwLength = dwNewHeaderLength - dwOldHeaderLength;

	pBase->header.dwDataStartOffset += dwLength;
	if (pBase->header.dwUnusedSpaceOffset)
		pBase->header.dwUnusedSpaceOffset += dwLength;

	// update the header

	updateWriteTime();

	if (vBuffer)
		VirtualFree(vBuffer, 0, MEM_RELEASE);
	if (hTmpFile != INVALID_HANDLE_VALUE)
		::CloseHandle(hTmpFile);

	return 1;

Fail:
	if (vBuffer)
		VirtualFree(vBuffer, 0, MEM_RELEASE);
	if (hTmpFile != INVALID_HANDLE_VALUE)
		::CloseHandle(hTmpFile);
	return 0;
}
//--------------------------------------------------------------------------//
//------------------------------End UTFDMan.CPP-----------------------------//
//--------------------------------------------------------------------------//

