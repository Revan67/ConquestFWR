//--------------------------------------------------------------------------//
//                                                                          //
//                              LogFile.cpp                                 //
//                                                                          //
//               COPYRIGHT (C) 2000 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
   $Header: /Conquest/App/Src/LogFile.cpp 9     11/14/00 6:08p Jasony $
	
   Logs calls to CreateInstance(), so we can keep a fres list of files in use
*/
//--------------------------------------------------------------------------//

#include "pch.h"
#include <globals.h>

#include "THashList.h"

#include "FileSys.h"
#include "TComponent.h"
#include "TSmartPointer.h"

#include <stdlib.h>
#include <stdio.h>

#define HASHSIZE 256
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
struct LFNODE
{
	LFNODE * pNext;
	
	static inline U32 hash (const char * ptr)
	{
		U8 result = 0;
		char a=*ptr++;
		while (a)
		{
			result += a|0x20;		// convert to lower case
			a = *ptr++;
		}
		return result;
	}
	
	U32 hash (void)
	{
		return hash(name);
	}

	//
	// String comparison and maintenance functions
	//
	
	inline bool compare (const char *ptr)
	{
		return (strcmp(ptr, name) == 0);
	}

	inline void print (void)
	{
		CQTRACE11("Logged usage of '%s'", name);
	}
	
	LFNODE (const char * ptr)
	{
		strncpy(name, ptr, sizeof(name));
		name[sizeof(name)-1] = 0;
		_strlwr(name);
		pNext = 0;
	}
	
    void * operator new (size_t size)
	{
		return GlobalAlloc(GPTR, size);
	}

	void   operator delete (void *ptr)
	{
		if (ptr)
			GlobalFree(ptr);
	}

	//
	// User data
	//
	char name[32];
};
//--------------------------------------------------------------------------//
//----------------------LogFile implementation---------------------------//
//--------------------------------------------------------------------------//

//--------------------------------------------------------------------------//
//
struct DACOM_NO_VTABLE LogFile : public IFileSystem
{
	BEGIN_DACOM_MAP_INBOUND(LogFile)
	DACOM_INTERFACE_ENTRY(IFileSystem)
	DACOM_INTERFACE_ENTRY2(IID_IFileSystem,IFileSystem)
	END_DACOM_MAP()

    void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}

	//--------------------------
	
	COMPTR<IFileSystem> pFile;
	HashList<LFNODE, const char *, HASHSIZE> fileNamePool;
	const char * szLogName;

	//--------------------------

	~LogFile (void)
	{
		shutdown();
	}

	// *** IComponentFactory methods ***
	
	DEFMETHOD(CreateInstance) (DACOMDESC *descriptor, void **instance);
	
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
	
	DEFMETHOD_(BOOL,CopyFile)    (LPCTSTR lpExistingFileName, LPCTSTR lpNewFileName, BOOL bFailIfExists);
	
	DEFMETHOD_(BOOL,MoveFile) (LPCTSTR lpExistingFileName, LPCTSTR lpNewFileName);
	
	DEFMETHOD_(DWORD,GetFileAttributes) (LPCTSTR lpFileName);
	
	DEFMETHOD_(BOOL,SetFileAttributes) (LPCTSTR lpFileName, DWORD dwFileAttributes);
	
	DEFMETHOD_(DWORD,GetLastError) (VOID);
	
	//---------------   
	// IFileSystem extensions to WIN32 system
	//---------------   
	
	DEFMETHOD_(HANDLE,OpenChild) (DAFILEDESC *lpDesc);
	
	DEFMETHOD_(DWORD,GetFilePosition) (HANDLE hFileHandle = 0, PLONG pPositionHigh=0);
	
	DEFMETHOD_(LONG,GetFileName) (LPSTR lpBuffer, LONG lBufferSize);
	
	DEFMETHOD_(DWORD,GetAccessType) (VOID);
	
	DEFMETHOD(GetParentSystem) (LPFILESYSTEM *lplpFileSystem);
	
	DEFMETHOD(SetPreference)  (DWORD dwNumber, DWORD  dwValue);
	
	DEFMETHOD(GetPreference)  (DWORD dwNumber, PDWORD pdwValue);
	
	DEFMETHOD(ReadDirectoryExtension) (HANDLE hFile, LPVOID lpBuffer, 
		DWORD nNumberOfBytesToRead,
		LPDWORD lpNumberOfBytesRead=0, DWORD dwStartOffset=0);
	
	DEFMETHOD(WriteDirectoryExtension) (HANDLE hFile, LPCVOID lpBuffer, 
		DWORD nNumberOfBytesToWrite,
		LPDWORD lpNumberOfBytesWritten=0, DWORD dwStartOffset=0);
	
	DEFMETHOD_(LONG,SerialCall) (LPFILESYSTEM lpSystem, DAFILE_SERIAL_PROC lpProc, VOID *lpContext);
	
	DEFMETHOD_(BOOL,GetAbsolutePath) (char *lpOutput, LPCTSTR lpInput, LONG lSize);
	
	// *** LogFile methods ***

	long __stdcall addName (const char * fileName);		// match the serialCall prototype

	void shutdown (void);

	void addNamesFromBuffer (char * buffer);

};
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
GENRESULT LogFile::CreateInstance (DACOMDESC *descriptor, void **instance)
{
	GENRESULT result = pFile->CreateInstance(descriptor, instance);

	if (result == GR_OK)
	{
		DAFILEDESC * pDesc = (DAFILEDESC *) descriptor;
		SerialCall(this, (DAFILE_SERIAL_PROC)(&LogFile::addName), const_cast<char *>(pDesc->lpFileName));
//		addName(pDesc->lpFileName);
	}

	return result;
}
//--------------------------------------------------------------------------//
//
BOOL LogFile::CloseHandle (HANDLE handle)
{
	return pFile->CloseHandle(handle);
}
//--------------------------------------------------------------------------//
//
BOOL LogFile::ReadFile (HANDLE hFileHandle, LPVOID lpBuffer, DWORD nNumberOfBytesToRead, 
		LPDWORD lpNumberOfBytesRead,
		LPOVERLAPPED lpOverlapped)
{
	return pFile->ReadFile(hFileHandle, lpBuffer, nNumberOfBytesToRead, lpNumberOfBytesRead, lpOverlapped);
}
//--------------------------------------------------------------------------//
//
BOOL LogFile::WriteFile (HANDLE hFileHandle, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite,
		LPDWORD lpNumberOfBytesWritten, LPOVERLAPPED lpOverlapped)
{
	return pFile->WriteFile(hFileHandle, lpBuffer, nNumberOfBytesToWrite, lpNumberOfBytesWritten, lpOverlapped);
}
//--------------------------------------------------------------------------//
//
BOOL LogFile::GetOverlappedResult (HANDLE hFileHandle,  
		LPOVERLAPPED lpOverlapped,
		LPDWORD lpNumberOfBytesTransferred,   
		BOOL bWait)
{
	return pFile->GetOverlappedResult(hFileHandle, lpOverlapped, lpNumberOfBytesTransferred, bWait);
}
//--------------------------------------------------------------------------//
//
DWORD LogFile::SetFilePointer (HANDLE hFileHandle, LONG lDistanceToMove, 
		PLONG lpDistanceToMoveHigh, DWORD dwMoveMethod)
{
	return pFile->SetFilePointer(hFileHandle, lDistanceToMove, lpDistanceToMoveHigh, dwMoveMethod);
}
//--------------------------------------------------------------------------//
//
BOOL LogFile::SetEndOfFile (HANDLE hFileHandle)
{
	return pFile->SetEndOfFile(hFileHandle);
}
//--------------------------------------------------------------------------//
//
DWORD LogFile::GetFileSize (HANDLE hFileHandle, LPDWORD lpFileSizeHigh)
{
	return pFile->GetFileSize(hFileHandle, lpFileSizeHigh);
}
//--------------------------------------------------------------------------//
//
BOOL LogFile::LockFile (HANDLE hFile,	
		DWORD dwFileOffsetLow,
		DWORD dwFileOffsetHigh,
		DWORD nNumberOfBytesToLockLow,
		DWORD nNumberOfBytesToLockHigh)
{
	return pFile->LockFile(hFile, dwFileOffsetLow, dwFileOffsetHigh, nNumberOfBytesToLockLow, nNumberOfBytesToLockHigh);
}
//--------------------------------------------------------------------------//
//
BOOL LogFile::UnlockFile (HANDLE hFile, 
								DWORD dwFileOffsetLow,
								DWORD dwFileOffsetHigh,
								DWORD nNumberOfBytesToUnlockLow,
								DWORD nNumberOfBytesToUnlockHigh)
{
	return pFile->UnlockFile(hFile, dwFileOffsetLow, dwFileOffsetHigh, nNumberOfBytesToUnlockLow, nNumberOfBytesToUnlockHigh);
}
//--------------------------------------------------------------------------//
//
BOOL LogFile::GetFileTime (HANDLE hFileHandle,   LPFILETIME lpCreationTime,
		LPFILETIME lpLastAccessTime, LPFILETIME lpLastWriteTime)
{
	return pFile->GetFileTime(hFileHandle, lpCreationTime, lpLastAccessTime, lpLastWriteTime);
}
//--------------------------------------------------------------------------//
//
BOOL LogFile::SetFileTime (HANDLE hFileHandle,   CONST FILETIME *lpCreationTime, 
		CONST FILETIME *lpLastAccessTime,
		CONST FILETIME *lpLastWriteTime)
{
	return pFile->SetFileTime(hFileHandle, lpCreationTime, lpLastAccessTime, lpLastWriteTime);
}
//--------------------------------------------------------------------------//
//
HANDLE LogFile::CreateFileMapping (HANDLE hFileHandle,
		LPSECURITY_ATTRIBUTES lpFileMappingAttributes,
		DWORD flProtect,
		DWORD dwMaximumSizeHigh,
		DWORD dwMaximumSizeLow,
		LPCTSTR lpName)
{
	return pFile->CreateFileMapping(hFileHandle, lpFileMappingAttributes, flProtect, dwMaximumSizeHigh, dwMaximumSizeLow, lpName);
}
//--------------------------------------------------------------------------//
//
LPVOID LogFile::MapViewOfFile (HANDLE hFileMappingObject,
		DWORD dwDesiredAccess,
		DWORD dwFileOffsetHigh,
		DWORD dwFileOffsetLow,
		DWORD dwNumberOfBytesToMap)
{
	return pFile->MapViewOfFile(hFileMappingObject, dwDesiredAccess, dwFileOffsetHigh, dwFileOffsetLow, dwNumberOfBytesToMap);
}
//--------------------------------------------------------------------------//
//
BOOL LogFile::UnmapViewOfFile (LPCVOID lpBaseAddress)
{
	return pFile->UnmapViewOfFile(lpBaseAddress);
}
//--------------------------------------------------------------------------//
//
HANDLE LogFile::FindFirstFile (LPCTSTR lpFileName, LPWIN32_FIND_DATA lpFindFileData)
{
	return pFile->FindFirstFile(lpFileName, lpFindFileData);
}
//--------------------------------------------------------------------------//
//
BOOL LogFile::FindNextFile (HANDLE hFindFile, LPWIN32_FIND_DATA lpFindFileData)
{
	return pFile->FindNextFile(hFindFile, lpFindFileData);
}
//--------------------------------------------------------------------------//
//
BOOL LogFile::FindClose (HANDLE hFindFile)
{
	return pFile->FindClose(hFindFile);
}
//--------------------------------------------------------------------------//
//
BOOL LogFile::CreateDirectory (LPCTSTR lpPathName, LPSECURITY_ATTRIBUTES lpSecurityAttributes)
{
	return pFile->CreateDirectory(lpPathName, lpSecurityAttributes);
}
//--------------------------------------------------------------------------//
//
BOOL LogFile::RemoveDirectory (LPCTSTR lpPathName)
{
	return pFile->RemoveDirectory(lpPathName);
}
//--------------------------------------------------------------------------//
//
DWORD LogFile::GetCurrentDirectory (DWORD nBufferLength, LPTSTR lpBuffer)
{
	return pFile->GetCurrentDirectory(nBufferLength, lpBuffer);
}
//--------------------------------------------------------------------------//
//
BOOL LogFile::SetCurrentDirectory (LPCTSTR lpPathName)
{
	return pFile->SetCurrentDirectory(lpPathName);
}
//--------------------------------------------------------------------------//
//
BOOL LogFile::DeleteFile (LPCTSTR lpFileName)
{
	return pFile->DeleteFile(lpFileName);
}
//--------------------------------------------------------------------------//
//
BOOL LogFile::CopyFile (LPCTSTR lpExistingFileName, LPCTSTR lpNewFileName, BOOL bFailIfExists)
{
	return pFile->CopyFile(lpExistingFileName, lpNewFileName, bFailIfExists);
}
//--------------------------------------------------------------------------//
//
BOOL LogFile::MoveFile (LPCTSTR lpExistingFileName, LPCTSTR lpNewFileName)
{
	return pFile->MoveFile(lpExistingFileName, lpNewFileName);
}
//--------------------------------------------------------------------------//
//
DWORD LogFile::GetFileAttributes (LPCTSTR lpFileName)
{
	return pFile->GetFileAttributes(lpFileName);
}
//--------------------------------------------------------------------------//
//
BOOL LogFile::SetFileAttributes (LPCTSTR lpFileName, DWORD dwFileAttributes)
{
	return pFile->SetFileAttributes(lpFileName, dwFileAttributes);
}
//--------------------------------------------------------------------------//
//
DWORD LogFile::GetLastError (VOID)
{
	return pFile->GetLastError();
}
//--------------------------------------------------------------------------//
//
HANDLE LogFile::OpenChild (DAFILEDESC *lpDesc)
{
	HANDLE result = pFile->OpenChild(lpDesc);

	DAFILEDESC * pDesc = (DAFILEDESC *) lpDesc;
	SerialCall(this, (DAFILE_SERIAL_PROC)(&LogFile::addName), const_cast<char *>(pDesc->lpFileName));
//	addName(pDesc->lpFileName);

	return result;
}
//--------------------------------------------------------------------------//
//
DWORD LogFile::GetFilePosition (HANDLE hFileHandle, PLONG pPositionHigh)
{
	return pFile->GetFilePosition(hFileHandle, pPositionHigh);
}
//--------------------------------------------------------------------------//
//
LONG LogFile::GetFileName (LPSTR lpBuffer, LONG lBufferSize)
{
	return pFile->GetFileName(lpBuffer, lBufferSize);
}
//--------------------------------------------------------------------------//
//
DWORD LogFile::GetAccessType (VOID)
{
	return pFile->GetAccessType();
}
//--------------------------------------------------------------------------//
//
GENRESULT LogFile::GetParentSystem (LPFILESYSTEM *lplpFileSystem)
{
	return pFile->GetParentSystem(lplpFileSystem);
}
//--------------------------------------------------------------------------//
//
GENRESULT LogFile::SetPreference (DWORD dwNumber, DWORD  dwValue)
{
	return pFile->SetPreference(dwNumber, dwValue);
}
//--------------------------------------------------------------------------//
//
GENRESULT LogFile::GetPreference (DWORD dwNumber, PDWORD pdwValue)
{
	return pFile->GetPreference(dwNumber, pdwValue);
}
//--------------------------------------------------------------------------//
//
GENRESULT LogFile::ReadDirectoryExtension (HANDLE hFile, LPVOID lpBuffer, 
		DWORD nNumberOfBytesToRead,
		LPDWORD lpNumberOfBytesRead, DWORD dwStartOffset)
{
	return pFile->ReadDirectoryExtension(hFile, lpBuffer, nNumberOfBytesToRead, lpNumberOfBytesRead, dwStartOffset);
}
//--------------------------------------------------------------------------//
//
GENRESULT LogFile::WriteDirectoryExtension (HANDLE hFile, LPCVOID lpBuffer, 
		DWORD nNumberOfBytesToWrite,
		LPDWORD lpNumberOfBytesWritten, DWORD dwStartOffset)
{
	return pFile->WriteDirectoryExtension(hFile, lpBuffer, nNumberOfBytesToWrite, lpNumberOfBytesWritten, dwStartOffset);
}
//--------------------------------------------------------------------------//
//
LONG LogFile::SerialCall (LPFILESYSTEM lpSystem, DAFILE_SERIAL_PROC lpProc, VOID *lpContext)
{
	return pFile->SerialCall(lpSystem, lpProc, lpContext);
}
//--------------------------------------------------------------------------//
//
BOOL LogFile::GetAbsolutePath (char *lpOutput, LPCTSTR lpInput, LONG lSize)
{
	return pFile->GetAbsolutePath(lpOutput, lpInput, lSize);
}
//----------------------------------------------------------------------------//
//
long LogFile::addName (const char * fileName)
{
	LFNODE * node;

	while (fileName[0] == ' ')
		fileName++;

	if (fileName[0] && (node = fileNamePool.findHashedNode(fileName)) == 0)
	{
		node = new LFNODE(fileName);
		CQASSERT(node != 0);
		fileNamePool.addNode(node);
	}

	return 0;
}
//----------------------------------------------------------------------------//
//
void LogFile::shutdown (void)
{
	DAFILEDESC fdesc = szLogName;
	COMPTR<IFileSystem> pLog;

	fdesc.dwDesiredAccess |= GENERIC_WRITE;
	fdesc.dwShareMode = 0;
	fdesc.dwCreationDistribution = OPEN_ALWAYS;
	fdesc.lpImplementation = "DOS";

	if (DACOM->CreateInstance(&fdesc, pLog) == GR_OK)
	{
		U32 fileSize = pLog->GetFileSize();
		char * buffer = (char *) malloc(fileSize + 1);
		DWORD dwRead;
		S32 i;

		pLog->ReadFile(0, buffer, fileSize, &dwRead, 0);
		buffer[fileSize] = 0;

		addNamesFromBuffer(buffer);

		free(buffer);
		buffer = 0;

		//
		// write names to file
		//
		pLog->SetFilePointer(0, 0);
		pLog->SetEndOfFile();

		for (i = 0; i < HASHSIZE; i++)
		{
			LFNODE * node = fileNamePool.table[i];

			while (node)
			{
				char buffer[128];
				sprintf(buffer, "%%ACTION%% %-32.32s %%DEST%%\r\n", node->name);
				int len = strlen(buffer);
				DWORD dwWritten;

				pLog->WriteFile(0, buffer, len, &dwWritten, 0);
				
				node = node->pNext;
			}
		}
	}
}
//----------------------------------------------------------------------------//
//
void LogFile::addNamesFromBuffer (char * buffer)
{
	char * ptr;

	while ((ptr = strchr(buffer, '\r')) != 0)
	{
		*ptr++ = 0;
		if (ptr[0] == '\n')
			ptr++;
		if (strncmp(buffer, "%ACTION% ", 9) == 0)		// search for legal line
		{
			buffer+=9;
			int i;
			for (i = 32; i>=0 && buffer[i] == ' '; i--)
				buffer[i] = 0;

			if (buffer[0] != ' ')		// reject invalid entries
				addName(buffer);
		}

		buffer = ptr;
	}
}
//----------------------------------------------------------------------------//
//----------------------------------------------------------------------------//
//----------------------------------------------------------------------------//
//
IFileSystem * __stdcall CreateLogOfFile (IFileSystem * pFile, const char *logName)
{
	LogFile * result = new DAComponent<LogFile> ();

	result->pFile.ptr = pFile;		// don't inc the ref count
	result->szLogName = logName;

	return result;
}
//----------------------------------------------------------------------------//
//-------------------------------End LogFile.cpp------------------------------//
//----------------------------------------------------------------------------//
