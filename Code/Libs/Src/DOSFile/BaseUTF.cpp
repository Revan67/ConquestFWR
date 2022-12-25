//--------------------------------------------------------------------------//
//                                                                          //
//                                BaseUTF.CPP                               //
//                                                                          //
//               COPYRIGHT (C) 1997 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//

#include <windows.h>

#include "DACOM.h"
#include "BaseUTF.h"
#include "FDump.h"


#define CHECKDESCSIZE(x)    (x->size==sizeof(DAFILEDESC)||x->size==sizeof(DAFILEDESC)-sizeof(U32))

//--------------------------------------------------------------------------//

extern char interface_name[];
extern ICOManager *DACOM;
static char implementation_name[] = "UTF";

BaseUTF * CreateUTF (void);
BaseUTF * CreateSharedUTF (DWORD dwSharing);

//--------------------------------------------------------------------------//
//-------------------------------UTF_SHARING Methods------------------------//
//--------------------------------------------------------------------------//
//
BOOL UTF_SHARING::isCompatible (const UTF_SHARING & access)
{
	BOOL result = 0;

	if (read && access.readSharing == 0)
		goto Done;
	if (write && access.writeSharing == 0)
		goto Done;
	if (access.read  && (read+write != readSharing))
		goto Done;
	if (access.write && (read+write != writeSharing))
		goto Done;

	result++;	// else compatible

Done:
	return result;
}	
//--------------------------------------------------------------------------//
//-------------------------------BaseUTF Methods----------------------------//
//--------------------------------------------------------------------------//
//
BaseUTF::~BaseUTF(void)
{
	if (pParent)
	{
		pParent->CloseHandle(hParentFile);
		pParent->Release();
	}
}
//--------------------------------------------------------------------------//
//
GENRESULT BaseUTF::CreateInstance (DACOMDESC *descriptor, void  **instance)
{
	DAFILEDESC		*lpInfo     = (DAFILEDESC *) descriptor;
	GENRESULT		result     = GR_OK;
	BaseUTF			*pNewSystem = NULL;

	if (lpInfo==NULL || (lpInfo->interface_name==NULL))
	{
		result = GR_GENERIC;
		goto Done;
	}

   //
   // If unsupported interface requested, fail call
   //

	if (CHECKDESCSIZE(lpInfo)==0 || strcmp(lpInfo->interface_name, interface_name))
	{
		result = GR_INTERFACE_UNSUPPORTED;
		goto Done;
	}


	//
	// Can't handle this request (we already have a parent)
	//

	if (lpInfo->lpParent && pParent)
	{
		result = GR_GENERIC;
		goto Done;
	}

	// 
	// if we are an open directory, see if we can find child inside us
	// 	

	if (hParentFile == INVALID_HANDLE_VALUE && pParent)
	{
		HANDLE			handle;

		if ((handle = OpenChild(lpInfo)) == INVALID_HANDLE_VALUE)
		{
		  //
		  // OpenChild() failed; system could not be created
		  // See if	file is really a directory
  		  //
			DWORD dwAttribs;
			UTF_DIR_ENTRY * pNewBaseDirEntry = 0;

			if ((pNewSystem = new DAComponent<BaseUTF>) == 0)
			{
				result = GR_OUT_OF_MEMORY;
				goto Done;
			}

			if (lpInfo->lpFileName)
			{
				memcpy(pNewSystem->szFilename, szFilename, iRootIndex);
		 		if (GetAbsolutePath(pNewSystem->szFilename+iRootIndex, lpInfo->lpFileName, MAX_PATH - iRootIndex) == 0)
				{
					delete pNewSystem;
					pNewSystem = 0;
					result = GR_FILE_ERROR;
					goto Done;
				}
			}

			if (pParentUTF)
			{
				if ((pNewBaseDirEntry = pParentUTF->getDirectoryEntryForChild(pNewSystem->szFilename+iRootIndex, pBaseDirEntry, GETFFHANDLE(lpInfo))) != 0)
				{
					// verify that filename matched the findFirst handle
					if (GETFFHANDLE(lpInfo)!=INVALID_HANDLE_VALUE && (lpInfo->lpFileName==0 || stricmp(pParentUTF->getNameBuffer()+pNewBaseDirEntry->dwName, lpInfo->lpFileName) != 0))
					{
						delete pNewSystem;
						pNewSystem=0;
						result = GR_INVALID_PARMS;
						goto Done;
					}
					dwAttribs = pNewBaseDirEntry->dwAttributes;
				}
				else
					dwAttribs = 0xFFFFFFFF;
			}
			else
			{
				dwAttribs = pParent->GetFileAttributes(pNewSystem->szFilename);
			}

			if (dwAttribs == 0xFFFFFFFF || (dwAttribs&FILE_ATTRIBUTE_DIRECTORY) == 0)
			{
				delete pNewSystem;
				pNewSystem=0;

				result = GR_FILE_ERROR;
				goto Done;
			}
			// 
			// else it is a directory, add a "\\" to the end of the name
			//
			if ((pNewSystem->iRootIndex = strlen(pNewSystem->szFilename)) != 0)
				if (pNewSystem->szFilename[pNewSystem->iRootIndex-1] == UTF_SWITCH_CHAR)
					pNewSystem->iRootIndex--;

			pNewSystem->szFilename[pNewSystem->iRootIndex] = UTF_SWITCH_CHAR;
			pNewSystem->hParentFile = handle;
			pNewSystem->pParent = pParent;
			pNewSystem->dwAccess = dwAccess & lpInfo->dwDesiredAccess;
			pParent->AddRef();
			if (pParentUTF && pNewBaseDirEntry)
			{
				pNewSystem->pBaseDirEntry = pNewBaseDirEntry;
				pNewSystem->pParentUTF = pParentUTF;
			}
			goto Done;
		}

		// else we successfully opened the child 

		{
			// need some other implementation

			lpInfo->lpParent = pParent;
			lpInfo->hParent   = handle;
			pParent->AddRef();			// child file system will now reference parent directly
			if ((result = DACOM->CreateInstance(lpInfo, (void **) &pNewSystem)) != GR_OK)
			{
				pParent->Release();
				pParent->CloseHandle(handle);
				lpInfo->lpParent = 0;
				lpInfo->hParent   = 0;
				goto Done;
			}
			lpInfo->lpParent = 0;
			lpInfo->hParent   = 0;
			goto Done;
		}
	}

	// else we are not a directory

	if (lpInfo->lpParent)
	{
		if (lpInfo->lpImplementation != NULL &&
			strcmp(lpInfo->lpImplementation, implementation_name))
		{
			result = GR_GENERIC;
			goto Done;
		}
		else	// dont create a UTF without being asked
		if (lpInfo->lpImplementation == NULL &&
			lpInfo->dwCreationDistribution != OPEN_EXISTING)
		{
			result = GR_GENERIC;
			goto Done;
		}

		// implies that we don't have a parent system
		// 
		// create a new instance of UTF
		//

		if (lpInfo->dwDesiredAccess == GENERIC_READ &&
			(lpInfo->dwShareMode & ~FILE_SHARE_READ) == 0)
		{
			pNewSystem = CreateUTF();
		}
		else
			pNewSystem = CreateSharedUTF(lpInfo->dwShareMode);

		if (pNewSystem == 0)
		{
			result = GR_OUT_OF_MEMORY;
			goto Done;
		}

		strncpy(pNewSystem->szFilename, lpInfo->lpFileName, sizeof(szFilename));
		pNewSystem->dwAccess = lpInfo->dwDesiredAccess;
		pNewSystem->hParentFile = lpInfo->hParent;
		pNewSystem->pParent = lpInfo->lpParent;

		if (pNewSystem->init(lpInfo) == 0)
		{
			// prevent releasing resources we didn't take over
			lpInfo->lpParent->AddRef();
			pNewSystem->hParentFile = INVALID_HANDLE_VALUE;
			pNewSystem->Release();
			pNewSystem = 0;
			result = GR_FILE_ERROR;
		}
	
		goto Done;	
	}


	// request to create a UTF system from nothing
	if (pParent == 0)
	{
		if (lpInfo->lpImplementation != NULL &&
			strcmp(lpInfo->lpImplementation, implementation_name))
		{
			result = GR_GENERIC;
			goto Done;
		}
		else	// dont create a UTF without being asked
		if (lpInfo->lpImplementation == NULL &&
			lpInfo->dwCreationDistribution != OPEN_EXISTING)
		{
			result = GR_GENERIC;
			goto Done;
		}

		LPCTSTR lpSaved = lpInfo->lpImplementation;
		DWORD dwSavedAccess = lpInfo->dwDesiredAccess;

		if (lpInfo->dwDesiredAccess == GENERIC_READ &&
			(lpInfo->dwShareMode & ~FILE_SHARE_READ) == 0)
		{
			pNewSystem = CreateUTF();
		}
		else
			pNewSystem = CreateSharedUTF(lpInfo->dwShareMode);

		if (pNewSystem == 0)
		{
			result = GR_OUT_OF_MEMORY;
			goto Done;
		}

		lpInfo->lpImplementation = "DOS";
		lpInfo->dwDesiredAccess |= GENERIC_READ;
		if ((result = DACOM->CreateInstance(lpInfo, (void **) &pNewSystem->pParent)) != GR_OK)
		{
			delete pNewSystem;
			pNewSystem = 0;
			result = GR_FILE_ERROR;
			lpInfo->lpImplementation = lpSaved;
			lpInfo->dwDesiredAccess = dwSavedAccess;
			goto Done;
		}
		pNewSystem->hParentFile = 0;
		lpInfo->lpImplementation = lpSaved;
		lpInfo->dwDesiredAccess = dwSavedAccess;
		strncpy(pNewSystem->szFilename, lpInfo->lpFileName, sizeof(szFilename));
		pNewSystem->dwAccess = lpInfo->dwDesiredAccess;

		if (pNewSystem->init(lpInfo) == 0)
		{
			pNewSystem->Release();
			pNewSystem = 0;
			result = GR_FILE_ERROR;
		}
		goto Done;	
	}
	else
	{
		// attempt to create the child instance from within

		HANDLE			handle;
		//
		// Associate file handle with new file system
		//
   
		handle = OpenChild(lpInfo);

		if (handle == INVALID_HANDLE_VALUE)
		{
		  //
		  // CreateFile() failed; system could not be created
		  // See if	file is really a directory
  		  //
			DWORD dwAttribs;
			UTF_DIR_ENTRY * pNewBaseDirEntry = 0;

			if ((pNewSystem = new DAComponent<BaseUTF>) == 0)
			{
				result = GR_OUT_OF_MEMORY;
				goto Done;
			}

			if (lpInfo->lpFileName)
			{
				if (GetAbsolutePath(pNewSystem->szFilename, lpInfo->lpFileName, MAX_PATH) == 0)
				{
					delete pNewSystem;
					pNewSystem = 0;
					result = GR_FILE_ERROR;
					goto Done;
				}
			}

			if ((pNewBaseDirEntry = getDirectoryEntryForChild(pNewSystem->szFilename, 0, GETFFHANDLE(lpInfo))) != 0)
			{
				// verify that filename matched the findFirst handle
				if (GETFFHANDLE(lpInfo)!=INVALID_HANDLE_VALUE && (lpInfo->lpFileName==0 || stricmp(getNameBuffer()+pNewBaseDirEntry->dwName, lpInfo->lpFileName) != 0))
				{
					delete pNewSystem;
					pNewSystem=0;
					result = GR_INVALID_PARMS;
					goto Done;
				}
				dwAttribs = pNewBaseDirEntry->dwAttributes;
			}
			else
				dwAttribs = GetFileAttributes(pNewSystem->szFilename);

			if (dwAttribs == 0xFFFFFFFF || (dwAttribs&FILE_ATTRIBUTE_DIRECTORY) == 0)
			{
				delete pNewSystem;
				pNewSystem = 0;
				result = GR_FILE_ERROR;
				goto Done;
			}
		
			// 
			// else it is a directory
			//

			if ((pNewSystem->iRootIndex = strlen(pNewSystem->szFilename)) != 0)
				if (pNewSystem->szFilename[pNewSystem->iRootIndex-1] == UTF_SWITCH_CHAR)
					pNewSystem->iRootIndex--;

			pNewSystem->szFilename[pNewSystem->iRootIndex] = UTF_SWITCH_CHAR;
			pNewSystem->hParentFile = handle;
			pNewSystem->pParent = this;
			pNewSystem->dwAccess = dwAccess & lpInfo->dwDesiredAccess;
			AddRef();
			// only if we are a read-only UTF file will the following work
			if ((pNewSystem->pBaseDirEntry = pNewBaseDirEntry) != 0)
				pNewSystem->pParentUTF = this;
			goto Done;
		}

		// else child file was opened

		{
			// need another implementation

			lpInfo->lpParent = this;
			lpInfo->hParent   = handle;
			AddRef();			// child file system will now reference us 
			if ((result = DACOM->CreateInstance(lpInfo, (void **) &pNewSystem)) != GR_OK)
			{
				Release();
				CloseHandle(handle);
				lpInfo->lpParent = 0;
				lpInfo->hParent   = 0;
				goto Done;
			}
			lpInfo->lpParent = 0;
			lpInfo->hParent   = 0;

			goto Done;
		}
	}

Done:
   *instance = pNewSystem;

   return result;
}
//--------------------------------------------------------------------------//
//
BOOL BaseUTF::init (DAFILEDESC *lpDesc)
{
	return 1;
}
//--------------------------------------------------------------------------//
//
BOOL BaseUTF::CloseHandle (HANDLE handle)
{
	if (pParent && handle)
	{	
		BOOL result;

		result = pParent->CloseHandle(handle);
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
BOOL BaseUTF::ReadFile (HANDLE hFile, LPVOID lpBuffer, DWORD nNumberOfBytesToRead, 
               LPDWORD lpNumberOfBytesRead,
               LPOVERLAPPED lpOverlapped)
{
	if (pParent && hFile)
	{	
		BOOL result;

		result = pParent->ReadFile(hFile, lpBuffer, nNumberOfBytesToRead, lpNumberOfBytesRead, lpOverlapped);
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
BOOL BaseUTF::WriteFile (HANDLE hFile, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite,
                LPDWORD lpNumberOfBytesWritten, LPOVERLAPPED lpOverlapped)
{
	if (pParent && hFile)
	{	
		BOOL result;

		result = pParent->WriteFile(hFile, lpBuffer, nNumberOfBytesToWrite, lpNumberOfBytesWritten, lpOverlapped);
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
BOOL BaseUTF::GetOverlappedResult (HANDLE hFileHandle, LPOVERLAPPED lpOverlapped,
                                 LPDWORD lpNumberOfBytesTransferred,   BOOL bWait)
{
	BOOL result;

	if (pParent && hFileHandle)
	{
		result = pParent->GetOverlappedResult(hFileHandle, lpOverlapped, lpNumberOfBytesTransferred, bWait);
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
DWORD BaseUTF::SetFilePointer (HANDLE hFileHandle, LONG lDistanceToMove, 
                     PLONG lpDistanceToMoveHigh, DWORD dwMoveMethod)
{
	DWORD result;

	if (pParent && hFileHandle)
	{
		result = pParent->SetFilePointer(hFileHandle, lDistanceToMove, lpDistanceToMoveHigh, dwMoveMethod);
		if (result == 0xFFFFFFFF)
			dwLastError = pParent->GetLastError();
		return result;
	}
	else
	{
		dwLastError = ERROR_ACCESS_DENIED;
		return 0xFFFFFFFF;
	}
}
//--------------------------------------------------------------------------//
//
BOOL BaseUTF::SetEndOfFile (HANDLE hFileHandle)
{
	BOOL result;

	if (pParent && hFileHandle)
	{
		result = pParent->SetEndOfFile(hFileHandle);
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
DWORD BaseUTF::GetFileSize (HANDLE hFileHandle, LPDWORD lpFileSizeHigh)
{
	DWORD result;

	if (pParent && hFileHandle)
	{
		result = pParent->GetFileSize(hFileHandle, lpFileSizeHigh);
		if (result == 0xFFFFFFFF)
			dwLastError = pParent->GetLastError();
		return result;
	}
	else
	{
		dwLastError = ERROR_ACCESS_DENIED;
		return 0xFFFFFFFF;
	}
}
//--------------------------------------------------------------------------//
//
BOOL BaseUTF::LockFile (HANDLE hFileHandle,	
	  				  DWORD dwFileOffsetLow,
					  DWORD dwFileOffsetHigh,
					  DWORD nNumberOfBytesToLockLow,
					  DWORD nNumberOfBytesToLockHigh)
{
	BOOL result;

	if (pParent && hFileHandle)
	{
		result = pParent->LockFile(hFileHandle,
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
BOOL BaseUTF::UnlockFile (HANDLE hFileHandle, 
						DWORD dwFileOffsetLow,
						DWORD dwFileOffsetHigh,
						DWORD nNumberOfBytesToUnlockLow,
						DWORD nNumberOfBytesToUnlockHigh)
{
	BOOL result;

	if (pParent && hFileHandle)
	{
		result = pParent->UnlockFile(hFileHandle,
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
BOOL BaseUTF::GetFileTime (HANDLE hFileHandle, LPFILETIME lpCreationTime,
                   LPFILETIME lpLastAccessTime, LPFILETIME lpLastWriteTime)
{
	BOOL result;

	if (pParent && hFileHandle)
	{
		result = pParent->GetFileTime(hFileHandle, lpCreationTime, lpLastAccessTime, lpLastWriteTime);
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
BOOL BaseUTF::SetFileTime (HANDLE hFileHandle, CONST FILETIME *lpCreationTime, 
                  CONST FILETIME *lpLastAccessTime,
                  CONST FILETIME *lpLastWriteTime)
{
	BOOL result;

	if (pParent && hFileHandle)
	{
		result = pParent->SetFileTime(hFileHandle, lpCreationTime, lpLastAccessTime, lpLastWriteTime);
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
HANDLE BaseUTF::CreateFileMapping (HANDLE hFileHandle, LPSECURITY_ATTRIBUTES lpFileMappingAttributes,
                                 DWORD flProtect, DWORD dwMaximumSizeHigh, DWORD dwMaximumSizeLow, LPCTSTR lpName)
{
	if (pParent && hFileHandle)
	{
		HANDLE result;

		result = pParent->CreateFileMapping(hFileHandle, lpFileMappingAttributes, flProtect, dwMaximumSizeHigh, dwMaximumSizeLow, lpName);
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
LPVOID BaseUTF::MapViewOfFile (HANDLE hFileMappingObject,
                            DWORD dwDesiredAccess,
                            DWORD dwFileOffsetHigh,
                            DWORD dwFileOffsetLow,
                            DWORD dwNumberOfBytesToMap)
{
	if (pParent)
	{
		LPVOID result;

		result = pParent->MapViewOfFile(hFileMappingObject, dwDesiredAccess, dwFileOffsetHigh, dwFileOffsetLow, dwNumberOfBytesToMap);
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
BOOL BaseUTF::UnmapViewOfFile (LPCVOID lpBaseAddress)
{
	if (pParent)
	{
		BOOL result;
		result = pParent->UnmapViewOfFile(lpBaseAddress);
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
HANDLE BaseUTF::FindFirstFile (LPCTSTR lpFileName, LPWIN32_FIND_DATA lpFindFileData)
{
	HANDLE result;
 
	if (pParent)
	{
		char buffer[MAX_PATH+4];

		if (iRootIndex)
			memcpy(buffer, szFilename, iRootIndex);
		if (GetAbsolutePath(buffer+iRootIndex, lpFileName, MAX_PATH - iRootIndex) == 0)
		{
			dwLastError = ERROR_BAD_PATHNAME;
			return INVALID_HANDLE_VALUE;
		}

		if (pParentUTF)
			result = pParentUTF->findFirstFile(buffer+iRootIndex, lpFindFileData, pBaseDirEntry);
		else
			result = pParent->FindFirstFile(buffer, lpFindFileData);
	
		if (result == INVALID_HANDLE_VALUE)
			dwLastError = pParent->GetLastError();
		return result;
	}
	else
	{
		dwLastError = ERROR_FILE_NOT_FOUND;
		return INVALID_HANDLE_VALUE;
	}
}
//--------------------------------------------------------------------------//
//
BOOL BaseUTF::FindNextFile (HANDLE hFindFile, LPWIN32_FIND_DATA lpFindFileData)
{
   BOOL result;

   if (pParent)
   {
	   result = pParent->FindNextFile(hFindFile, lpFindFileData);

	   if (result == 0)
			dwLastError = pParent->GetLastError();
	   return result;
   }
   else
   {
		dwLastError = ERROR_FILE_NOT_FOUND;
		return 0;
   }
}
//--------------------------------------------------------------------------//
//
BOOL BaseUTF::FindClose (HANDLE hFindFile)
{
   BOOL result;

   if (pParent)
   {
	   result = pParent->FindClose(hFindFile);
	   if (result == 0)
			dwLastError = pParent->GetLastError();
	   return result;
   }
   else
   {
		dwLastError = ERROR_FILE_NOT_FOUND;
		return 0;
   }
}
//--------------------------------------------------------------------------//
//
BOOL BaseUTF::CreateDirectory (LPCTSTR lpPathName, LPSECURITY_ATTRIBUTES lpSecurityAttributes)
{
	BOOL result;

	if (pParent)
	{
		char buffer[MAX_PATH+4];

		if (iRootIndex)
			memcpy(buffer, szFilename, iRootIndex);
		if (GetAbsolutePath(buffer+iRootIndex, lpPathName, MAX_PATH - iRootIndex) == 0)
		{
			dwLastError = ERROR_BAD_PATHNAME;
			return 0;
		}

		result = pParent->CreateDirectory(buffer, lpSecurityAttributes);
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
BOOL BaseUTF::RemoveDirectory (LPCTSTR lpPathName)
{
	BOOL result;

	if (pParent)
	{
		char buffer[MAX_PATH+4];

		if (iRootIndex)
			memcpy(buffer, szFilename, iRootIndex);
		if (GetAbsolutePath(buffer+iRootIndex, lpPathName, MAX_PATH - iRootIndex) == 0)
		{
			dwLastError = ERROR_BAD_PATHNAME;
			return 0;
		}

		result = pParent->RemoveDirectory(buffer);
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
DWORD BaseUTF::GetCurrentDirectory (DWORD nBufferLength, LPTSTR lpBuffer)
{
	DWORD result;

	if (pParent)
	{
		DWORD len;

		len = strlen(szFilename+iRootIndex);
		len = __max(len, 2);
		len = __min(len, nBufferLength);
		if (len>0)
		{
			memcpy(lpBuffer, szFilename+iRootIndex, len);
			lpBuffer[len-1] = 0;
			result = len-1;
		}
		else
			result = 0;
		dwLastError = NO_ERROR;
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
BOOL BaseUTF::SetCurrentDirectory (LPCTSTR lpPathName)
{
	BOOL result;

	if (pParent)
	{
 		char buffer[MAX_PATH+4];

		memcpy(buffer, szFilename, iRootIndex);
		if ((result = GetAbsolutePath(buffer+iRootIndex, lpPathName, MAX_PATH - iRootIndex)) == 0)
			dwLastError = ERROR_BAD_PATHNAME;
		else
		{
			int len;
			DWORD dwAttribs;

			dwAttribs = pParent->GetFileAttributes(buffer);

			if (dwAttribs == 0xFFFFFFFF || (dwAttribs&FILE_ATTRIBUTE_DIRECTORY) == 0)
			{
				dwLastError = ERROR_BAD_PATHNAME;
				return 0;
			}
			else
			{
				len = strlen(buffer);
				memcpy(szFilename, buffer, len+1);
				if (szFilename[len-1] != UTF_SWITCH_CHAR)
				{
					szFilename[len] = UTF_SWITCH_CHAR;
					szFilename[len+1] = 0;
				}
			}
		}
			
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
BOOL BaseUTF::DeleteFile (LPCTSTR lpFileName)
{
	BOOL result;

	if (pParent)
	{
		char buffer[MAX_PATH+4];

		if (iRootIndex)
			memcpy(buffer, szFilename, iRootIndex);
		if (GetAbsolutePath(buffer+iRootIndex, lpFileName, MAX_PATH - iRootIndex) == 0)
		{
			dwLastError = ERROR_BAD_PATHNAME;
			return 0;
		}

		result = pParent->DeleteFile(buffer);
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
BOOL BaseUTF::CopyFile (LPCTSTR lpExistingFileName, LPCTSTR lpNewFileName, BOOL bFailIfExists)
{
	LPVOID lpMemory=0;
	HANDLE hSrc, hDst=0;
	DAFILEDESC desc(lpExistingFileName, 0);
	DWORD dwLength, dwReadWrite;
	BOOL result=0;

	hSrc = OpenChild(&desc);
	if (hSrc == INVALID_HANDLE_VALUE)
		return 0;
	dwLength = GetFileSize(hSrc);

	if ((lpMemory = VirtualAlloc(0, dwLength, MEM_COMMIT, PAGE_READWRITE)) == 0)
	{
		dwLastError = ERROR_NOT_ENOUGH_MEMORY;
		goto Done;
	}

	desc.dwDesiredAccess = GENERIC_WRITE;
	desc.dwCreationDistribution = (bFailIfExists)? CREATE_NEW : CREATE_ALWAYS;
	desc.lpFileName = lpNewFileName;

	hDst = OpenChild(&desc);
	if (hDst == INVALID_HANDLE_VALUE)
		goto Done;
	
	if (ReadFile(hSrc,  lpMemory, dwLength, &dwReadWrite, 0) == 0)
		goto Done;
	if (WriteFile(hDst, lpMemory, dwLength, &dwReadWrite, 0) == 0)
		goto Done;
	
	result = 1;
		
Done:
	if (hDst)
		CloseHandle(hDst);
	if (hSrc)
		CloseHandle(hSrc);
	if (lpMemory)
		VirtualFree(lpMemory, 0, MEM_RELEASE);

	return result;
}   
//--------------------------------------------------------------------------//
//
BOOL BaseUTF::MoveFile (LPCTSTR lpExistingFileName, LPCTSTR lpNewFileName)
{
	BOOL result;

	if (pParent)
	{
		char buffer1[MAX_PATH+4];
		char buffer2[MAX_PATH+4];

		if (iRootIndex)
		{
			memcpy(buffer1, szFilename, iRootIndex);
			memcpy(buffer2, szFilename, iRootIndex);
		}
		if (GetAbsolutePath(buffer1+iRootIndex, lpExistingFileName, MAX_PATH - iRootIndex) == 0)
		{
			dwLastError = ERROR_BAD_PATHNAME;
			return 0;
		}
		if (GetAbsolutePath(buffer2+iRootIndex, lpNewFileName, MAX_PATH - iRootIndex) == 0)
		{
			dwLastError = ERROR_BAD_PATHNAME;
			return 0;
		}

		result = pParent->MoveFile(buffer1, buffer2);
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
DWORD BaseUTF::GetFileAttributes (LPCTSTR lpFileName)
{
	DWORD result;

	if (pParent)
	{
		char buffer[MAX_PATH+4];

		if (iRootIndex)
			memcpy(buffer, szFilename, iRootIndex);
		if (GetAbsolutePath(buffer+iRootIndex, lpFileName, MAX_PATH - iRootIndex) == 0)
		{
			dwLastError = ERROR_BAD_PATHNAME;
			return 0;
		}

		if (pParentUTF)
			result = pParentUTF->getFileAttributes(buffer+iRootIndex, pBaseDirEntry);
		else
			result = pParent->GetFileAttributes(buffer);

		if (result == 0xFFFFFFFF)
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
BOOL BaseUTF::SetFileAttributes (LPCTSTR lpFileName, DWORD dwFileAttributes)
{
	BOOL result;

	if (pParent)
	{
		char buffer[MAX_PATH+4];

		if (iRootIndex)
			memcpy(buffer, szFilename, iRootIndex);
		if (GetAbsolutePath(buffer+iRootIndex, lpFileName, MAX_PATH - iRootIndex) == 0)
		{
			dwLastError = ERROR_BAD_PATHNAME;
			return 0;
		}

		result = pParent->SetFileAttributes(buffer, dwFileAttributes);
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
DWORD BaseUTF::GetLastError (VOID)
{
   return dwLastError;
}
//--------------------------------------------------------------------------//
//
HANDLE BaseUTF::OpenChild (DAFILEDESC *lpInfo)
{
	HANDLE handle;
	
	if (pParent == 0)
	{
		dwLastError = ERROR_NOT_SUPPORTED;
		return INVALID_HANDLE_VALUE;
	}

	// short cut this whole thing if user passed in a findFirstHandle
	if (pParentUTF && GETFFHANDLE(lpInfo) != INVALID_HANDLE_VALUE)
	{
		handle = pParentUTF->openChild(lpInfo, pBaseDirEntry);
	}
	else
	{
		char buffer[MAX_PATH+4];
		LPCSTR lpSaved = lpInfo->lpFileName;

		memcpy(buffer, szFilename, iRootIndex);
 		if (GetAbsolutePath(buffer+iRootIndex, lpInfo->lpFileName, MAX_PATH - iRootIndex) == 0)
		{
			dwLastError = ERROR_FILE_NOT_FOUND;
			return INVALID_HANDLE_VALUE;
		}

		if (pParentUTF)
		{
			lpInfo->lpFileName = buffer+iRootIndex;
			handle = pParentUTF->openChild(lpInfo, pBaseDirEntry);
		}
		else
		{
			lpInfo->lpFileName = buffer;
			handle = pParent->OpenChild(lpInfo);
		}

		lpInfo->lpFileName = lpSaved;
	}
	
	if (handle == INVALID_HANDLE_VALUE)
		dwLastError = pParent->GetLastError();

	return handle;
}
//--------------------------------------------------------------------------//
//
LONG BaseUTF::GetFileName (LPSTR lpBuffer, LONG lBufferSize)
{
   lBufferSize = __min(lBufferSize, (LONG)strlen(szFilename)+1);

   if (lBufferSize > 0 && lpBuffer)
      memcpy(lpBuffer, szFilename, lBufferSize);

   return lBufferSize;
}
//--------------------------------------------------------------------------//
//
DWORD BaseUTF::GetAccessType (VOID)
{
   return dwAccess;
}
//--------------------------------------------------------------------------//
//
GENRESULT BaseUTF::GetParentSystem (LPFILESYSTEM *lplpFileSystem)
{
   if ((*lplpFileSystem = pParent) != 0)
	   pParent->AddRef();
   return GR_OK;
}
//--------------------------------------------------------------------------//
//
GENRESULT BaseUTF::SetPreference (DWORD dwNumber, DWORD  dwValue)
{
   return GR_GENERIC;
}
//--------------------------------------------------------------------------//
//
GENRESULT BaseUTF::GetPreference (DWORD dwNumber, PDWORD pdwValue)
{
   return GR_GENERIC;
}
//--------------------------------------------------------------------------//
//
GENRESULT BaseUTF::ReadDirectoryExtension (HANDLE hFile, LPVOID lpBuffer, 
										DWORD nNumberOfBytesToRead,
										LPDWORD lpNumberOfBytesRead, DWORD dwStartOffset)
{
	return GR_GENERIC;
}
//--------------------------------------------------------------------------//
//
GENRESULT BaseUTF::WriteDirectoryExtension (HANDLE hFile, LPCVOID lpBuffer, 
										DWORD nNumberOfBytesToWrite,
										LPDWORD lpNumberOfBytesWritten, DWORD dwStartOffset)
{
	return GR_GENERIC;
}
//--------------------------------------------------------------------------//
//
LONG BaseUTF::SerialCall (LPFILESYSTEM lpSystem, DAFILE_SERIAL_PROC lpProc, VOID *lpContext)
{
	return pParent->SerialCall(lpSystem, lpProc, lpContext);
}
//--------------------------------------------------------------------------//
//
DWORD BaseUTF::GetFilePosition (HANDLE hFileHandle, PLONG pPositionHigh)
{
	if (pParent && hFileHandle)
	{
		DWORD result;
		result = pParent->GetFilePosition(hFileHandle, pPositionHigh);

		if (result == 0xFFFFFFFF)
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
// lpFileName -> absolute address, without a leading '\\'
//
BOOL BaseUTF::GetDirectoryEntry (LPCSTR lpFileName, UTF_DIR_ENTRY *pDirectory, LPCTSTR pNames, UTF_DIR_ENTRY **ppEntry)
{
	UTF_DIR_ENTRY *pEntry = *ppEntry;
	char buffer[MAX_PATH+4];
	char *ptr, *tmp;

	// NOTE: This is a strcpy because we know it is safe to do here; this is an internally called
	// function only.  TNB/JY
	strcpy(buffer, lpFileName);
	buffer[MAX_PATH]=0;
	ptr = buffer;
	if (ptr[0] == UTF_SWITCH_CHAR)
	{
		ptr++;
		if (ptr[0] == 0)
			return 1;	// our job is done
	}

	if ((pEntry->dwAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
		return 0;
	
	// go to first child
	pEntry = (UTF_DIR_ENTRY *) (((char *)pDirectory) + pEntry->dwDataOffset);

	if ((tmp = strchr(ptr, UTF_SWITCH_CHAR)) != 0)
		*tmp = 0;

	while (pEntry != pDirectory)
	{
		// compare our name with names on this level
		if (stricmp(ptr, pNames + pEntry->dwName) == 0)	// found it!
		{
			if (tmp==0 || tmp[1]==0)
			{
				*ppEntry = pEntry;		// we have reached the end of the line
				return 1;				// success!
			}
			else
				ptr = tmp + 1;
			
			// decend to next level
			if ((pEntry->dwAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
				return 0;

			// go to first child
			pEntry = (UTF_DIR_ENTRY *) (((char *)pDirectory) + pEntry->dwDataOffset);

			if ((tmp = strchr(ptr, UTF_SWITCH_CHAR)) != 0)
				*tmp = 0;
		}
		else	// go to next sibling
			pEntry = (UTF_DIR_ENTRY *) (((char *)pDirectory) + pEntry->dwNext);
	}

	return 0;
}
//--------------------------------------------------------------------------//
void __fastcall switchchar_convert (char * string);
//--------------------------------------------------------------------------//
// Get absolute path in terms of this file system
//  returns a path with a leading '\\'
//
//
BOOL BaseUTF::GetAbsolutePath (char *lpOutput, LPCTSTR lpInput, LONG lSize)
{
	int len;
	char *ptr;
	BOOL result=1;

	if (--lSize <= 0)
		return 0;
	lpOutput[lSize] = 0;

	if (lpInput[0] == UTF_SWITCH_CHAR || lpInput[0] == '/')
	{
		strncpy(lpOutput, lpInput, lSize);
		lpOutput[lSize-1] = 0;
		switchchar_convert(lpOutput);
		return 1;
	}

//	if (iRootIndex)
		strncpy(lpOutput, szFilename+iRootIndex, lSize);
//	else
//		*lpOutput = 0;

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
BOOL BaseUTF::DOSTimeToFileTime (DWORD dwDOSTime, FILETIME *pFileTime)
{
	return DosDateTimeToFileTime(LOWORD(dwDOSTime), HIWORD(dwDOSTime), pFileTime);
}
//--------------------------------------------------------------------------//
//
DWORD BaseUTF::FileTimeToDOSTime (CONST FILETIME *pFileTime)
{
	DWORD result=0;

	FileTimeToDosDateTime(pFileTime, ((WORD *)&result), ((WORD *)&result)+1);

	return result;
}
//--------------------------------------------------------------------------//
// returns the offset into 'pNames' where match is found
//
DWORD BaseUTF::FindName (LPCTSTR lpFileName, LPCTSTR _pNames)
{
	LPCTSTR pNames = _pNames;
	DWORD len, namelen;

	if ((namelen = strlen(lpFileName)) == 0)
		goto Done;

	if (pNames[0] == 0)
		pNames++;
	
	while (pNames[0] != 0)
	{
		len = strlen(pNames);

		if (len >= namelen)
		{
			LPCTSTR pTmp;

			pTmp = pNames + len - namelen;
			if (memcmp(pTmp, lpFileName, namelen) == 0)
				return pTmp - _pNames;
		}

		pNames += len + 1;
	}

Done:
	return 0;
}
//--------------------------------------------------------------------------//
//
HANDLE BaseUTF::openChild (DAFILEDESC *lpDesc, UTF_DIR_ENTRY * pEntry)
{
	ASSERT(0 && "Not Implemented");
	return INVALID_HANDLE_VALUE;
}
//--------------------------------------------------------------------------//
//
DWORD BaseUTF::getFileAttributes (LPCTSTR lpFileName, UTF_DIR_ENTRY * pEntry)
{
	ASSERT(0 && "Not Implemented");
	return DWORD(-1);
}
//--------------------------------------------------------------------------//
//
HANDLE BaseUTF::findFirstFile (LPCTSTR lpFileName, LPWIN32_FIND_DATA lpFindFileData, UTF_DIR_ENTRY * pEntry)
{
	ASSERT(0 && "Not Implemented");
	return INVALID_HANDLE_VALUE;
}
//--------------------------------------------------------------------------//
//
UTF_DIR_ENTRY * BaseUTF::getDirectoryEntryForChild (LPCSTR lpFileName, UTF_DIR_ENTRY *pDirectory, HANDLE hFindFirst)
{
	return 0;
}
//--------------------------------------------------------------------------//
//
const char * BaseUTF::getNameBuffer (void)
{
	ASSERT(0 && "Not Implemented");
	return 0;
}
//--------------------------------------------------------------------------//
// return FALSE if name contains invalid characters
//
static inline void setBit (U8 * bitArray, U8 pos)
{
	bitArray += (pos >> 3);
	bitArray[0] |= (1 << (pos & 7));
}
static inline bool getBit (U8 * bitArray, U8 pos)
{
	bitArray += (pos >> 3);
	return ((bitArray[0] & (1 << (pos & 7))) != 0);
}
bool BaseUTF::TestValid (LPCTSTR lpFileName)
{
	static bool initialized = false;
	static U8 charMap[256/8];
	if (initialized == false)
	{
		setBit(charMap, '<');
		setBit(charMap, '>');
		setBit(charMap, ':');
		setBit(charMap, '"');
		setBit(charMap, '/');
		setBit(charMap, '|');
		setBit(charMap, '*');
		setBit(charMap, '?');
		initialized=true;
	}

	U8 a;
	while ((a = *lpFileName++) != 0)
	{
		if (getBit(charMap, a))
			return false;
	}

	return true;
}
//--------------------------------------------------------------------------//
//
IFileSystem * CreateBaseUTF (void)
{
	return new DAComponent<BaseUTF>;
}

//--------------------------------------------------------------------------//
//----------------------------End BaseUTF.cpp-------------------------------//
//--------------------------------------------------------------------------//
