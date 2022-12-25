//--------------------------------------------------------------------------//
//                                                                          //
//                               Document.cpp                               //
//                                                                          //
//                  COPYRIGHT (C) 2002 Fever Pitch Studios, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*

    $Author: Ajackson $
*/			    
//--------------------------------------------------------------------------//

// 4514: unused inline function
// 4201: nonstandard no-name structure use
// 4100: formal parameter was not used
// 4512: assignment operator could not be generated
// 4245: conversion from signed to unsigned
// 4127: constant condition expression
// 4355: 'this' used in member initializer
// 4244: conversion from int to unsigned char, possible loss of data
// 4200: zero sized struct member
// 4710: inline function not expanded
// 4702: unreachable code
// 4786: truncating function name (255 chars) in browser info
#pragma warning (disable : 4514 4201 4100 4512 4245 4127 4355 4244 4710 4702 4786)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#pragma warning (disable : 4355 4201)

#include "TConnContainer.h"
#include "Document.h"
#include "IDocClient.h"
#include "TConnPoint.h"
#include "HeapObj.h"
#include "fdump.h"

extern HINSTANCE hInstance;
extern ICOManager * DACOM;

static char interface_name[] = "IDocument";

#define MAX_NAME_LENGTH 64
#ifndef MAKE_4CHAR
#define MAKE_4CHAR(a,b,c,d) (((U32) (a)) + (((U32) (b)) << 8) + (((U32) (c)) << 16) + (((U32) (d)) << 24))
#endif

#ifndef __min
#define __min(x,y) ((x>y)?y:x)
#endif



#define DA_SWITCH_CHAR '\\'

//--------------------------------------------------------------------------
//  
struct Document : public IDocument, 
						 ConnectionPointContainer<Document>
{
	DWORD dwNumChildren;
	DWORD dwRefs, dwUsage;
	C8 fileName[MAX_PATH+4];
	struct IFileSystem *pFile;
	struct Document *pParent, *pSibling, *pChild;
	bool bModified, bInRelease, bDeadWood;
	DWORD dwOldUsage;
	ConnectionPoint<Document, IDocumentClient> point;

	BEGIN_DACOM_MAP_OUTBOUND(Document)
	DACOM_INTERFACE_ENTRY_AGGREGATE("IDocumentClient", point)
	END_DACOM_MAP()

	BEGIN_DACOM_MAP_INBOUND(Document)
	DACOM_INTERFACE_ENTRY(IDocument)
	DACOM_INTERFACE_ENTRY(IFileSystem)
	DACOM_INTERFACE_ENTRY(IDAConnectionPointContainer)
	DACOM_INTERFACE_ENTRY2(IID_IDocument,IDocument)
	DACOM_INTERFACE_ENTRY2(IID_IFileSystem,IFileSystem)
	DACOM_INTERFACE_ENTRY2(IID_IDAConnectionPointContainer,IDAConnectionPointContainer)
	END_DACOM_MAP()

	Document (void) : point(0)
	{
		dwRefs=1;
	}

	void * operator new (size_t size)
	{
		return HEAP->ClearAllocateMemory(size, "Document instance");
	}

	/* IDAComponent members */

	DEFMETHOD(QueryInterface) (const C8 *interface_name, void **instance);
	DEFMETHOD_(U32,AddRef)           (void);
	DEFMETHOD_(U32,Release)          (void);

	/* IComponentFactory members */

	DEFMETHOD(CreateInstance) (DACOMDESC *descriptor, void **instance);

	/* IFileSystem members */
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

    /* IDocument members */

	DEFMETHOD(GetChildDocument) (const C8 *fileName, struct IDocument **document);
	
	DEFMETHOD_(BOOL32,IsModified) (void);

	DEFMETHOD_(BOOL32,SetModified) (BOOL32 modifyValue = 1);

	DEFMETHOD(UpdateAllClients) (const C8 *message, void *parm);

	DEFMETHOD(CloseAllClients) (void);

	DEFMETHOD_(BOOL32,Rename) (const C8 *newName);

	DEFMETHOD_(U32,GetNumChildren) (void);

	/* Document members */

	Document * GetChild (const C8 *pathName);

	U32 AddUsage (void);

	U32 ReleaseUsage (void);

	U32 InitChildren (void);

	GENRESULT LocalUpdateAllClients (Document *origDoc, const C8 *message, void *parm);

	BOOL32 TempOpenFile (void);

	BOOL32 TempCloseFile (void);

	GENRESULT init (DOCDESC *lpDesc);
};
//--------------------------------------------------------------------------//
//
GENRESULT Document::CreateInstance (DACOMDESC *descriptor, void **instance)
{
	GENRESULT  result      = GR_OK;
	IDocument *pNewInstance = NULL;
	DOCDESC * lpDesc = (DOCDESC *) descriptor;
	DAFILEDESC fdesc;
	LPFILESYSTEM pNewFile;

	if (lpDesc==0 || (lpDesc->interface_name==0))
	{
		result = GR_INTERFACE_UNSUPPORTED;
		goto Done;
	}

   //
   // If unsupported interface requested, fail call
   //

	if ((lpDesc->size != sizeof(*lpDesc)) || 
		strcmp(lpDesc->interface_name, interface_name))
	{
		if ((lpDesc->size != sizeof(fdesc)) || strcmp(fdesc.interface_name, lpDesc->interface_name))
		{
			result = GR_INTERFACE_UNSUPPORTED;
			goto Done;
		}
	}

	if (lpDesc->lpFileName == 0 || lpDesc->lpFileName[0] == 0)
	{
		result = GR_INVALID_PARMS;
		goto Done;
	}

	memcpy(((char *)&fdesc)+sizeof(DACOMDESC), descriptor+1, sizeof(fdesc)-sizeof(DACOMDESC));

	if ((result = pFile->CreateInstance(&fdesc, (void **) &pNewFile)) != GR_OK)
		goto Done;

	if (fdesc.dwCreationDistribution != OPEN_EXISTING)
		SetModified();

	if (lpDesc->size == sizeof(*lpDesc) && lpDesc->memory)
	{
		DWORD dwWritten;

		if (pNewFile->WriteFile(0, lpDesc->memory, lpDesc->memoryLength, &dwWritten, 0) == 0)
		{
			pNewFile->Release();
			result = GR_GENERIC;
			goto Done;
		}
	}

	pNewFile->Release();
	pNewFile=0;

	if (fdesc.dwCreationDistribution != OPEN_EXISTING)
		InitChildren();

	result = GetChildDocument(lpDesc->lpFileName, &pNewInstance);

Done:
	*instance = (IDocument *) pNewInstance;
	return result;
}
//--------------------------------------------------------------------------//
//
GENRESULT Document::QueryInterface (const C8 *interface_name, void **instance)
{
	int i;
	const _DACOM_INTMAP_ENTRY * interfaces = _GetEntriesIn();

	for (i = 0; interfaces[i].interface_name; i++)
	{
		if (strcmp(interfaces[i].interface_name, interface_name) == 0)
		{
			IDAComponent *result = (IDAComponent *) (((char *) this) + interfaces[i].offset);
			result->AddRef();
			*instance = result;
			return GR_OK;
		}
	}

	*instance = 0;
	return GR_INTERFACE_UNSUPPORTED;
}
//--------------------------------------------------------------------------//
//
U32 Document::AddRef (void)
{
	dwRefs++;
	return dwRefs;
}
//--------------------------------------------------------------------------//
//
U32 Document::Release (void)
{
	if (dwRefs > 0)
		dwRefs--;
	if (bInRelease==false && dwRefs == 0)
	{
		Document *pNext;

		bInRelease=true;

		CloseAllClients();

		while (pChild)
		{
			pNext = pChild->pSibling;
			pChild->Release();
			pChild = pNext;
		}

		if (ReleaseUsage() && pFile)
		{
			pFile->Release();
			pFile=0;
		}
		
		if (dwRefs == 0)
		{
			delete this;
			return 0;
		}
		bInRelease=false;
	}

	if (dwRefs==1 && point.pClientList==0)
		ReleaseUsage();

	return dwRefs;
}
//--------------------------------------------------------------------------//
//
BOOL Document::CloseHandle (HANDLE handle)
{
	return pFile->CloseHandle(handle);
}
//--------------------------------------------------------------------------//
//
BOOL Document::ReadFile (HANDLE hFileHandle, LPVOID lpBuffer, DWORD nNumberOfBytesToRead, 
			   LPDWORD lpNumberOfBytesRead,
			   LPOVERLAPPED lpOverlapped)
{
	return pFile->ReadFile(hFileHandle, lpBuffer, nNumberOfBytesToRead, lpNumberOfBytesRead, lpOverlapped);
}
//--------------------------------------------------------------------------//
//
BOOL Document::WriteFile (HANDLE hFileHandle, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite,
				LPDWORD lpNumberOfBytesWritten, LPOVERLAPPED lpOverlapped)
{
	BOOL result = pFile->WriteFile(hFileHandle, lpBuffer, nNumberOfBytesToWrite, lpNumberOfBytesWritten, lpOverlapped);

	if (result || GetLastError() == ERROR_IO_PENDING)
		SetModified();
	return result;
}
//--------------------------------------------------------------------------//
//
BOOL Document::GetOverlappedResult(HANDLE hFileHandle,  
								 LPOVERLAPPED lpOverlapped,
								 LPDWORD lpNumberOfBytesTransferred,   
								 BOOL bWait)
{
	return pFile->GetOverlappedResult(hFileHandle, lpOverlapped, lpNumberOfBytesTransferred, bWait);
}
//--------------------------------------------------------------------------//
//
DWORD Document::SetFilePointer (HANDLE hFileHandle, LONG lDistanceToMove, PLONG lpDistanceToMoveHigh, DWORD dwMoveMethod)
{
	return pFile->SetFilePointer(hFileHandle, lDistanceToMove, lpDistanceToMoveHigh, dwMoveMethod);
}
//--------------------------------------------------------------------------//
//
BOOL Document::SetEndOfFile (HANDLE hFileHandle)
{
	BOOL result = pFile->SetEndOfFile(hFileHandle);

	if (result)
		SetModified();

	return result;
}
//--------------------------------------------------------------------------//
//
DWORD Document::GetFileSize (HANDLE hFileHandle, LPDWORD lpFileSizeHigh)
{
	return pFile->GetFileSize(hFileHandle, lpFileSizeHigh);
}
//--------------------------------------------------------------------------//
//
BOOL Document::LockFile (HANDLE hFile,	
							  DWORD dwFileOffsetLow,
							  DWORD dwFileOffsetHigh,
							  DWORD nNumberOfBytesToLockLow,
							  DWORD nNumberOfBytesToLockHigh)
{
	return pFile->LockFile(hFile, dwFileOffsetLow, dwFileOffsetHigh, nNumberOfBytesToLockLow, nNumberOfBytesToLockHigh);
}
//--------------------------------------------------------------------------//
//
BOOL Document::UnlockFile (HANDLE hFile, DWORD dwFileOffsetLow, DWORD dwFileOffsetHigh, DWORD nNumberOfBytesToUnlockLow,
								DWORD nNumberOfBytesToUnlockHigh)
{
	return pFile->UnlockFile(hFile, dwFileOffsetLow, dwFileOffsetHigh, nNumberOfBytesToUnlockLow,
								nNumberOfBytesToUnlockHigh);
}
//--------------------------------------------------------------------------//
//
BOOL Document::GetFileTime (HANDLE hFileHandle,   LPFILETIME lpCreationTime, LPFILETIME lpLastAccessTime, LPFILETIME lpLastWriteTime)
{
	return pFile->GetFileTime(hFileHandle, lpCreationTime, lpLastAccessTime, lpLastWriteTime);
}
//--------------------------------------------------------------------------//
//
BOOL Document::SetFileTime (HANDLE hFileHandle,   CONST FILETIME *lpCreationTime, 
				  CONST FILETIME *lpLastAccessTime,
				  CONST FILETIME *lpLastWriteTime)
{
	BOOL result = pFile->SetFileTime(hFileHandle, lpCreationTime, lpLastAccessTime, lpLastWriteTime);

	if (result)
		SetModified();

	return result;
}
//--------------------------------------------------------------------------//
//
HANDLE Document::CreateFileMapping (HANDLE hFileHandle,
								 LPSECURITY_ATTRIBUTES lpFileMappingAttributes,
								 DWORD flProtect,
								 DWORD dwMaximumSizeHigh,
								 DWORD dwMaximumSizeLow,
								 LPCTSTR lpName)
{
	return pFile->CreateFileMapping(hFileHandle, lpFileMappingAttributes, flProtect, dwMaximumSizeHigh,
								 dwMaximumSizeLow, lpName);
}
//--------------------------------------------------------------------------//
//
LPVOID Document::MapViewOfFile (HANDLE hFileMappingObject, 
								 DWORD dwDesiredAccess,
								 DWORD dwFileOffsetHigh,
								 DWORD dwFileOffsetLow,
								 DWORD dwNumberOfBytesToMap)
{
	return pFile->MapViewOfFile(hFileMappingObject, dwDesiredAccess, dwFileOffsetHigh,
								 dwFileOffsetLow, dwNumberOfBytesToMap);
}
//--------------------------------------------------------------------------//
//
BOOL Document::UnmapViewOfFile (LPCVOID lpBaseAddress)
{
	return pFile->UnmapViewOfFile(lpBaseAddress);
}
//--------------------------------------------------------------------------//
//
HANDLE Document::FindFirstFile (LPCTSTR lpFileName, LPWIN32_FIND_DATA lpFindFileData)
{
	return pFile->FindFirstFile(lpFileName, lpFindFileData);
}
//--------------------------------------------------------------------------//
//
BOOL Document::FindNextFile (HANDLE hFindFile, LPWIN32_FIND_DATA lpFindFileData)
{
	return pFile->FindNextFile(hFindFile, lpFindFileData);
}
//--------------------------------------------------------------------------//
//
BOOL Document::FindClose (HANDLE hFindFile)
{
	return pFile->FindClose(hFindFile);
}
//--------------------------------------------------------------------------//
//
BOOL Document::CreateDirectory (LPCTSTR lpPathName, LPSECURITY_ATTRIBUTES lpSecurityAttributes)
{
	BOOL result = pFile->CreateDirectory(lpPathName, lpSecurityAttributes);

	if (result)
	{
		InitChildren();		// update doc structure
		SetModified();
	}

	return result;
}
//--------------------------------------------------------------------------//
//
BOOL Document::RemoveDirectory (LPCTSTR lpPathName)
{
	BOOL result = pFile->RemoveDirectory(lpPathName);

	if (result)
	{
		InitChildren();		// update doc structure
		SetModified();
	}

	return result;
}
//--------------------------------------------------------------------------//
//
DWORD Document::GetCurrentDirectory (DWORD nBufferLength, LPTSTR lpBuffer)
{
	return pFile->GetCurrentDirectory(nBufferLength, lpBuffer);
}
//--------------------------------------------------------------------------//
//
BOOL Document::SetCurrentDirectory (LPCTSTR lpPathName)
{
	return pFile->SetCurrentDirectory(lpPathName);
}
//--------------------------------------------------------------------------//
//
BOOL Document::DeleteFile (LPCTSTR lpFileName)
{
	BOOL result = pFile->DeleteFile(lpFileName);

	if (result)
	{
		InitChildren();		// update doc structure
		SetModified();
	}

	return result;
}
//--------------------------------------------------------------------------//
//
BOOL Document::CopyFile (LPCTSTR lpExistingFileName, LPCTSTR lpNewFileName, BOOL bFailIfExists)
{
	BOOL result = pFile->CopyFile(lpExistingFileName, lpNewFileName, bFailIfExists);

	if (result)
	{
		InitChildren();		// add new node to structure
		SetModified();
	}

	return result;
}
//--------------------------------------------------------------------------//
//
BOOL Document::MoveFile (LPCTSTR lpExistingFileName, LPCTSTR lpNewFileName)
{
	BOOL result = pFile->MoveFile(lpExistingFileName, lpNewFileName);

	if (result)
	{
		Document * tmp;
		const char *ptr1;
		const char *ptr2;

		ptr1 = strrchr(lpExistingFileName, '\\');
		ptr2 = strrchr(lpNewFileName, '\\');
		
		if ((ptr1 == lpExistingFileName || ptr1 == 0) &&
			(ptr2 == lpNewFileName || ptr2 == 0))
		{
			if ((tmp = GetChild(lpExistingFileName)) != 0)
			{
				if (lpNewFileName[0] == '\\')
				{
					strncpy(tmp->fileName, lpNewFileName, sizeof(tmp->fileName));
				}
				else
				{
					strncpy(tmp->fileName+1, lpNewFileName, sizeof(tmp->fileName)-1);
					tmp->fileName[0] = '\\';
				}
			}
		}
		
		InitChildren();		// update doc structure
		SetModified();
	}

	return result;
}
//--------------------------------------------------------------------------//
//
DWORD Document::GetFileAttributes (LPCTSTR lpFileName)
{
	return pFile->GetFileAttributes(lpFileName);
}
//--------------------------------------------------------------------------//
//
BOOL Document::SetFileAttributes (LPCTSTR lpFileName, DWORD dwFileAttributes)
{
	BOOL result = pFile->SetFileAttributes(lpFileName, dwFileAttributes);

	if (result)
		SetModified();

	return result;
}
//--------------------------------------------------------------------------//
//
DWORD Document::GetLastError (VOID)
{
	return pFile->GetLastError();
}
//--------------------------------------------------------------------------//
//
HANDLE Document::OpenChild (DAFILEDESC *lpDesc)
{
	return INVALID_HANDLE_VALUE;
}
//--------------------------------------------------------------------------//
//
DWORD Document::GetFilePosition (HANDLE hFileHandle, PLONG pPositionHigh)
{
	return pFile->GetFilePosition(hFileHandle, pPositionHigh);
}
//--------------------------------------------------------------------------//
//
LONG Document::GetFileName (LPSTR lpBuffer, LONG lBufferSize)
{
	const char *src = fileName;

	if (pParent)
		src++;
	
	{
		LONG localLength = (LONG)strlen(src)+1;
		lBufferSize = __min(lBufferSize, localLength);
	}

	if (lBufferSize > 0 && lpBuffer)
	{
	  memcpy(lpBuffer, src, lBufferSize);
	  lpBuffer[lBufferSize-1] = 0;
	}

	return lBufferSize;
}
//--------------------------------------------------------------------------//
//
DWORD Document::GetAccessType (VOID)
{
	return pFile->GetAccessType();
}
//--------------------------------------------------------------------------//
//
GENRESULT Document::GetParentSystem (LPFILESYSTEM *lplpFileSystem)
{
	if ((*lplpFileSystem = pParent) != 0)
		pParent->AddRef();
	return GR_OK;
}
//--------------------------------------------------------------------------//
//
GENRESULT Document::SetPreference (DWORD dwNumber, DWORD  dwValue)
{
	return pFile->SetPreference(dwNumber, dwValue);
}
//--------------------------------------------------------------------------//
//
GENRESULT Document::GetPreference (DWORD dwNumber, PDWORD pdwValue)
{
	return pFile->GetPreference(dwNumber, pdwValue);
}
//--------------------------------------------------------------------------//
//
GENRESULT Document::ReadDirectoryExtension (HANDLE hFile, LPVOID lpBuffer, 
										DWORD nNumberOfBytesToRead,
										LPDWORD lpNumberOfBytesRead, DWORD dwStartOffset)
{
	return pFile->ReadDirectoryExtension(hFile, lpBuffer, nNumberOfBytesToRead, lpNumberOfBytesRead, dwStartOffset);
}
//--------------------------------------------------------------------------//
//
GENRESULT Document::WriteDirectoryExtension (HANDLE hFile, LPCVOID lpBuffer, 
										DWORD nNumberOfBytesToWrite,
										LPDWORD lpNumberOfBytesWritten, DWORD dwStartOffset)
{
	return pFile->WriteDirectoryExtension(hFile, lpBuffer, nNumberOfBytesToWrite, lpNumberOfBytesWritten, dwStartOffset);
}
//--------------------------------------------------------------------------//
//
LONG Document::SerialCall (LPFILESYSTEM lpSystem, DAFILE_SERIAL_PROC lpProc, VOID *lpContext)
{
	return pFile->SerialCall(lpSystem, lpProc, lpContext);
}
//--------------------------------------------------------------------------//
//
BOOL Document::GetAbsolutePath (char *lpOutput, LPCTSTR lpInput, LONG lSize)
{
	return pFile->GetAbsolutePath(lpOutput, lpInput, lSize);
}
//--------------------------------------------------------------------------//
//
GENRESULT Document::GetChildDocument (const C8 *fileName, struct IDocument **document)
{
	GENRESULT  result      = GR_OK;
	Document *pNewInstance = NULL;
	char newPath[MAX_PATH+4];

	if (fileName==0 || fileName[0]==0)
	{
		result = GR_INVALID_PARMS;
		goto Done;
	}

	if (pFile == 0)
	{
		result = GR_GENERIC;
		goto Done;
	}

	if (GetAbsolutePath (newPath, fileName, sizeof(newPath)) == 0)
	{
		result = GR_INVALID_PARMS;
		goto Done;
	}

	if ((pNewInstance = GetChild(newPath)) == 0)
		result = GR_GENERIC;
	else
	{
		if (pNewInstance->pFile || pNewInstance->AddUsage())
			pNewInstance->AddRef();
		else
		{
			pNewInstance = 0;
			result = GR_GENERIC;
		}
	}

Done:
	*document = pNewInstance;
	return result;
}
//--------------------------------------------------------------------------//
//
BOOL32 Document::IsModified (void)
{
	BOOL32 result = bModified;

	if (result == 0)
	{
		Document *node = pChild;
		
		while (node)
		{
			if ((result = node->IsModified()) != 0)
				break;
			node = node->pSibling;
		}
	}
	return result;
}
//--------------------------------------------------------------------------//
//
BOOL32 Document::SetModified (BOOL32 modifyValue)
{
	BOOL32 result = bModified;

	bModified = (modifyValue!=0);

	if (modifyValue == 0)
	{
		Document *node = pChild;

		while (node)
		{
			node->SetModified(0);
			node = node->pSibling;
		}
	}

	return result;
}
//--------------------------------------------------------------------------//
//
GENRESULT Document::UpdateAllClients (const C8 *message, void *parm)
{
	return LocalUpdateAllClients(this, message, parm);
}
//--------------------------------------------------------------------------//
//
GENRESULT Document::CloseAllClients (void)
{
	Document *node = pChild;
		
	while (node)
	{
		node->CloseAllClients();
		node = node->pSibling;
	}

	while (point.pClientList)
	{
		IDocumentClient * client = point.pClientList->client;
		point.pClientList->client->OnClose(this);
		ASSERT(point.pClientList==0 || client != point.pClientList->client);
	}
	
	return GR_OK;
}
//--------------------------------------------------------------------------
//
Document * Document::GetChild (const C8 *pathName)
{
	Document *result=0;
	char buffer[MAX_PATH+4];
	char *ptr;

	if (pathName[0] == '\\')
		pathName++;
	strcpy(buffer, pathName);
	if ((ptr = strchr(buffer, '\\')) != 0)
		*ptr++ = 0;

	if (buffer[0] == 0)	// base case
		return this;

	// else search through the children
	result = pChild;
	while (result)
	{
		if (_stricmp(buffer, result->fileName+1) == 0)
		{
			if (ptr)
				result = result->GetChild(ptr);
			break;
		}

		result = result->pSibling;
	}

	return result;
}
//--------------------------------------------------------------------------
//
U32 Document::AddUsage (void)
{
	if (pFile == 0)
	{
		if (pParent->AddUsage())
		{
			DAFILEDESC fdesc = fileName;
						
			fdesc.dwShareMode = FILE_SHARE_READ | FILE_SHARE_WRITE;
			fdesc.dwDesiredAccess = pParent->pFile->GetAccessType();

			if (!fdesc.dwDesiredAccess)
			{
				Document* parent = pParent;

				while( parent->pParent && !fdesc.dwDesiredAccess )
				{
					parent = parent->pParent;

					if( parent->pFile )
					{
						fdesc.dwDesiredAccess = parent->pFile->GetAccessType();
					}
				}
			}

			if (pParent->pFile->CreateInstance(&fdesc, (void **) &pFile) == GR_OK)
			{
				dwUsage++;	
			}
		}
	}
	else
		dwUsage++;
 
	return dwUsage;
}
//-----------------------------------------------------------------------------------------
//
U32 Document::ReleaseUsage (void)
{
	if (dwUsage > 1 || (dwUsage > 0 && pParent))
		dwUsage--;
	if (dwUsage==0 && pFile)
	{
		pFile->Release();
		pFile=0;
		pParent->ReleaseUsage();
	}

	return dwUsage;
}
//-------------------------------------------------------------------------------------------
//  open children, fill in their values
//  
U32 Document::InitChildren (void)
{
	WIN32_FIND_DATA data;
	HANDLE handle;
	Document *last, *prev;

	if (AddUsage() == 0)
		return 0;

	//--------------------------------
	// mark all current children as dead wood
	//--------------------------------
	
	if ((last = pChild) != 0)
	while (1)
	{
		last->bDeadWood = true;
		if (last->pSibling == 0)
			break;
		last = last->pSibling;
	}
	
	handle = FindFirstFile("\\*.*", &data);

	if (handle != INVALID_HANDLE_VALUE)
	do
	{
		if (data.cFileName[0] != '.')	// ignore silly names
		{
			Document * tmp;
			if ((tmp = GetChild(data.cFileName)) == 0)	// if not already a child
			{
				if (last == 0)
				{
					if ((last = new Document) != 0)
						pChild = last;
				}
				else
				{
					if ((last->pSibling = new Document) != 0)
						last = last->pSibling;
				}

				dwNumChildren++;

				if (last)
				{
					strcpy(last->fileName+1, data.cFileName);
					last->fileName[0] = '\\';
					last->pParent = this;

					dwNumChildren += last->InitChildren();
				}
			}
			else   //already a child, check the children
			{
				tmp->bDeadWood = false;
				dwNumChildren += tmp->InitChildren();
			}
		}
	} while (FindNextFile(handle, &data) != 0);

	if (handle != INVALID_HANDLE_VALUE)
		FindClose(handle);


	//--------------------------------
	// remove dead wood
	//--------------------------------
	prev = 0;	
	if ((last = pChild) != 0)
	while (last)
	{
		if (last->bDeadWood)
		{
			if (prev == 0)
				pChild = last->pSibling;
			else
				prev->pSibling = last->pSibling;
			last->Release();
			if ((last = prev) == 0)
				last = pChild;
		}
		else
		{
			prev = last;
			last = last->pSibling;
		}
	}
	
	ReleaseUsage();

	return dwNumChildren;
}
//--------------------------------------------------------------------------//
//
GENRESULT Document::LocalUpdateAllClients (Document *origDoc, const C8 *message, void *parm)
{
	CONNECTION_NODE<IDocumentClient> *node = point.pClientList;

	while (node)
	{
		node->client->OnUpdate(origDoc, message, parm);
		node = node->pNext;
	}

	if (pParent)
		pParent->LocalUpdateAllClients(origDoc, message, parm);
	
	return GR_OK;
}
//--------------------------------------------------------------------------
//
BOOL32 Document::Rename (const C8 *newName)
{
	BOOL32 result=0;

	if (pParent == 0)
	{
		if ((result = ::CopyFile(fileName, newName, 1)) != 0)
		{
			DAFILEDESC fdesc = newName;
			LPFILESYSTEM pNewFile;

			fdesc.dwDesiredAccess = GENERIC_READ|GENERIC_WRITE,
			fdesc.dwShareMode = FILE_SHARE_READ|FILE_SHARE_WRITE;

			if (DACOM->CreateInstance(&fdesc, (void **) &pNewFile) != GR_OK)
				result = 0;
			else
			{
				TempCloseFile();
				pFile = pNewFile;
				dwUsage = dwOldUsage;
				dwOldUsage = 0;
				TempOpenFile();

				::DeleteFile(fileName);

				strncpy(fileName, newName, sizeof(fileName));
			}
		}
	}
	else
	{
		const C8 *tmp;

		tmp = strrchr(newName, '\\');
		if (tmp == 0 || tmp == newName)
		{
			char buffer[MAX_PATH+4];

			if (tmp)
				strncpy(buffer, tmp, sizeof(buffer));
			else
			{
				strncpy(buffer+1, newName, sizeof(buffer)-1);
				buffer[0] = '\\';
			}
			// temporarily close the file
			TempCloseFile();

			result = pParent->MoveFile(fileName, buffer);

			TempOpenFile();
		}
	}

	return result;
}
//--------------------------------------------------------------------------
//
U32 Document::GetNumChildren (void)
{
	return dwNumChildren;
}
//--------------------------------------------------------------------------
//
BOOL32 Document::TempCloseFile (void)
{
	Document *node = pChild;
		
	while (node)
	{
		node->TempCloseFile();
		node = node->pSibling;
	}

	dwOldUsage = dwUsage;
	if (pFile)
	{
		pFile->Release();
		pFile = 0;
	}

	dwUsage = 0;

	return 1;
}
//--------------------------------------------------------------------------
//
BOOL32 Document::TempOpenFile (void)
{
	Document *node = pChild;

	if (dwOldUsage)
	{
		AddUsage();
		pParent->ReleaseUsage();		// get rid of extra usage
		dwUsage = dwOldUsage;
		dwOldUsage = 0;
	}
	
	while (node)
	{
		node->TempOpenFile();
		node = node->pSibling;
	}

	return 1;
}
//--------------------------------------------------------------------------
//
GENRESULT Document::init (DOCDESC *lpDesc)
{
	GENRESULT result = GR_OK;
	DAFILEDESC fdesc;
	char tempname[MAX_PATH+4];

	dwNumChildren = 0;

	memcpy(((char *)&fdesc)+sizeof(DACOMDESC), ((char *)lpDesc) + sizeof(DACOMDESC), sizeof(fdesc)-sizeof(DACOMDESC));

	if (lpDesc->memory)		// if memory document, not file
	{
		char path[MAX_PATH+4];
		DWORD dwPrefix = MAKE_4CHAR('D','O','C',0);

		GetTempPath(MAX_PATH, path);	

		if (GetTempFileName(path, (const char *)&dwPrefix, 0, tempname) == 0)
		{
			result = GR_GENERIC;
			goto Done;
		}

		fdesc.lpFileName = tempname;
		fdesc.dwDesiredAccess = GENERIC_READ|GENERIC_WRITE,
		fdesc.dwShareMode = 0;
		fdesc.dwCreationDistribution = CREATE_ALWAYS;
		fdesc.dwFlagsAndAttributes = FILE_ATTRIBUTE_TEMPORARY|FILE_FLAG_DELETE_ON_CLOSE;
		fdesc.lpImplementation = 0;
	}

	if (fdesc.lpParent && fdesc.hParent==0)
		pFile = fdesc.lpParent;		// no reference count needed here
	else
	if ((result = DACOM->CreateInstance(&fdesc, (void **) &pFile)) != GR_OK)
		goto Done;

	if (lpDesc->memory)
	{
		DWORD dwWritten;

		if (pFile->WriteFile(0, lpDesc->memory, lpDesc->memoryLength, &dwWritten, 0) == 0)
		{
			pFile->Release();
			result = GR_GENERIC;
			goto Done;
		}

		pFile->SetFilePointer(0,0);
	}

	if (lpDesc->lpFileName)
		strncpy(fileName, lpDesc->lpFileName, sizeof(fileName)-1);

	dwUsage = 1;
	dwNumChildren = InitChildren();

Done:
	return result;
}
//--------------------------------------------------------------------------//
//
void RegisterDocument (ICOManager * DACOM)
{
	IComponentFactory * doc;

	if ((doc = new DAComponentFactory<Document,DOCDESC>("IDocument")) != 0)
	{
		DACOM->RegisterComponent(doc, interface_name);
		doc->Release();
	}
}


//--------------------------------------------------------------------------//
//----------------------------End Document.cpp------------------------------//
//--------------------------------------------------------------------------//

