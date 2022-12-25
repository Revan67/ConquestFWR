#ifndef FILESYS_H
#define FILESYS_H
//--------------------------------------------------------------------------//
//                                                                          //
//                              FILESYS.H                                   //
//                                                                          //
//               COPYRIGHT (C) 1996 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*

   $Author: Rmarr $
*/


/*

  Features:

  :  Multiple file systems
  :  File systems within file systems
  :  File systems that operate on top of other file systems
  :  Allows multiple files open at same time, for read/write
  :  Installed file systems behave exactly like local OS 
  :  Memory mapped files
  :  Async I/O

*/
//--------------------------------------------------------------------------//

#ifndef DACOM_H
#include "DACOM.h"
#endif

#ifndef _WINDOWS_
#error Windows.h required for this to compile!
#endif

//--------------------------------------------------------------------------//
//-----------------------------IFileSystem Interface------------------------//
//--------------------------------------------------------------------------//

typedef struct IFileSystem *LPFILESYSTEM;
typedef long (__stdcall IFileSystem::*DAFILE_SERIAL_PROC) (void *lpContext);

#define SERIALMETHOD(method)        LONG __stdcall method (void *lpContext=0)

//---------------------
// Use DAFILEINFO structure on call to CreateInstance
//---------------------

struct DAFILEDESC : public DACOMDESC
{
   LPCTSTR               lpImplementation;
   LPCTSTR               lpFileName;
   DWORD                 dwDesiredAccess;
   DWORD                 dwShareMode;
   LPSECURITY_ATTRIBUTES lpSecurityAttributes;
   DWORD                 dwCreationDistribution;
   DWORD                 dwFlagsAndAttributes;
   HANDLE                hTemplateFile;
   LPFILESYSTEM          lpParent;
   HANDLE                hParent;
   HANDLE				 hFindFirst;

   DAFILEDESC (const C8 *_file_name = NULL, const C8 *_interface_name = "IFileSystem") : DACOMDESC(_interface_name)
   {
      memset(((C8 *)this)+sizeof(DACOMDESC), 0, sizeof(*this)-sizeof(DACOMDESC));

      dwDesiredAccess        = GENERIC_READ;
	  dwShareMode			 = FILE_SHARE_READ;
      dwCreationDistribution = OPEN_EXISTING;
      dwFlagsAndAttributes   = FILE_ATTRIBUTE_NORMAL | 
                               FILE_FLAG_SEQUENTIAL_SCAN;
      lpFileName             = _file_name;
      size                   = sizeof(*this);
	  hFindFirst			 = INVALID_HANDLE_VALUE;
   }
};


#define IID_IFileSystem MAKE_IID("IFileSystem",1)

struct DACOM_NO_VTABLE IFileSystem : public IComponentFactory
{
   // *** IDAComponent methods ***

   DEFMETHOD(QueryInterface) (const C8 *interface_name, void **instance) = 0;
   DEFMETHOD_(U32,AddRef)           (void) = 0;
   DEFMETHOD_(U32,Release)          (void) = 0;

   // *** IComponentFactory methods ***

   DEFMETHOD(CreateInstance) (DACOMDESC *descriptor, void **instance) = 0;

   //
   // *** IFileSystem methods ***
   //
   // Note: Windows types used below for optimum compatibility with Win32
   // file system API calls
   //

   DEFMETHOD_(BOOL,CloseHandle) (HANDLE handle) = 0;

   DEFMETHOD_(BOOL,ReadFile) (HANDLE hFile, LPVOID lpBuffer, DWORD nNumberOfBytesToRead, 
               LPDWORD lpNumberOfBytesRead,
               LPOVERLAPPED lpOverlapped=0) = 0;

   DEFMETHOD_(BOOL,WriteFile) (HANDLE hFile, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite,
                LPDWORD lpNumberOfBytesWritten, LPOVERLAPPED lpOverlapped=0) = 0;

   DEFMETHOD_(BOOL,GetOverlappedResult)   (HANDLE hFile,  
                                 LPOVERLAPPED lpOverlapped,
                                 LPDWORD lpNumberOfBytesTransferred,   
                                 BOOL bWait) = 0;   

   DEFMETHOD_(DWORD,SetFilePointer) (HANDLE hFile, LONG lDistanceToMove, 
                     PLONG lpDistanceToMoveHigh=0, DWORD dwMoveMethod=FILE_BEGIN) = 0;

   DEFMETHOD_(BOOL,SetEndOfFile) (HANDLE hFile = 0) = 0;

   DEFMETHOD_(DWORD,GetFileSize) (HANDLE hFile=0, LPDWORD lpFileSizeHigh=0) = 0;   

   DEFMETHOD_(BOOL,LockFile) (HANDLE hFile,	
							  DWORD dwFileOffsetLow,
							  DWORD dwFileOffsetHigh,
							  DWORD nNumberOfBytesToLockLow,
							  DWORD nNumberOfBytesToLockHigh) = 0;

   DEFMETHOD_(BOOL,UnlockFile) (HANDLE hFile, 
								DWORD dwFileOffsetLow,
								DWORD dwFileOffsetHigh,
								DWORD nNumberOfBytesToUnlockLow,
								DWORD nNumberOfBytesToUnlockHigh) = 0;

   DEFMETHOD_(BOOL,GetFileTime) (HANDLE hFile,   LPFILETIME lpCreationTime,
                   LPFILETIME lpLastAccessTime, LPFILETIME lpLastWriteTime) = 0;

   DEFMETHOD_(BOOL,SetFileTime) (HANDLE hFile,   CONST FILETIME *lpCreationTime, 
                  CONST FILETIME *lpLastAccessTime,
                  CONST FILETIME *lpLastWriteTime) = 0;

   DEFMETHOD_(HANDLE,CreateFileMapping)   (HANDLE hFile=0,
                                 LPSECURITY_ATTRIBUTES lpFileMappingAttributes=0,
                                 DWORD flProtect=PAGE_READONLY,
                                 DWORD dwMaximumSizeHigh=0,
                                 DWORD dwMaximumSizeLow=0,
                                 LPCTSTR lpName=0) = 0;

   DEFMETHOD_(LPVOID,MapViewOfFile)      (HANDLE hFileMappingObject,
                                 DWORD dwDesiredAccess=FILE_MAP_READ,
                                 DWORD dwFileOffsetHigh=0,
                                 DWORD dwFileOffsetLow=0,
                                 DWORD dwNumberOfBytesToMap=0) = 0;
   
   DEFMETHOD_(BOOL,UnmapViewOfFile)      (LPCVOID lpBaseAddress) = 0;

   DEFMETHOD_(HANDLE,FindFirstFile) (LPCTSTR lpFileName, LPWIN32_FIND_DATA lpFindFileData) = 0;
   
   DEFMETHOD_(BOOL,FindNextFile) (HANDLE hFindFile, LPWIN32_FIND_DATA lpFindFileData) = 0;   
   
   DEFMETHOD_(BOOL,FindClose) (HANDLE hFindFile) = 0;   

   DEFMETHOD_(BOOL,CreateDirectory) (LPCTSTR lpPathName, LPSECURITY_ATTRIBUTES lpSecurityAttributes=0) = 0;

   DEFMETHOD_(BOOL,RemoveDirectory) (LPCTSTR lpPathName) = 0;
   
   DEFMETHOD_(DWORD,GetCurrentDirectory) (DWORD nBufferLength, LPTSTR lpBuffer) = 0;

   DEFMETHOD_(BOOL,SetCurrentDirectory) (LPCTSTR lpPathName) = 0;

   DEFMETHOD_(BOOL,DeleteFile)  (LPCTSTR lpFileName) = 0; 

   DEFMETHOD_(BOOL,CopyFile)    (LPCTSTR lpExistingFileName, LPCTSTR lpNewFileName, BOOL bFailIfExists) = 0;
   
   DEFMETHOD_(BOOL,MoveFile) (LPCTSTR lpExistingFileName, LPCTSTR lpNewFileName) = 0;

   DEFMETHOD_(DWORD,GetFileAttributes) (LPCTSTR lpFileName) = 0;

   DEFMETHOD_(BOOL,SetFileAttributes) (LPCTSTR lpFileName, DWORD dwFileAttributes) = 0;

   DEFMETHOD_(DWORD,GetLastError) (void) = 0;

   //---------------   
   // IFileSystem extensions to WIN32 system
   //---------------   
   
   DEFMETHOD_(HANDLE,OpenChild) (DAFILEDESC *lpDesc) = 0;
   
   DEFMETHOD_(DWORD,GetFilePosition) (HANDLE hFile = 0, PLONG pPositionHigh=0) = 0;
   
   DEFMETHOD_(LONG,GetFileName) (LPSTR lpBuffer, LONG lBufferSize) = 0;
   
   DEFMETHOD_(DWORD,GetAccessType) (void) = 0;

   DEFMETHOD(GetParentSystem) (LPFILESYSTEM *lplpFileSystem) = 0;

   DEFMETHOD(SetPreference)  (DWORD dwNumber, DWORD  dwValue) = 0;

   DEFMETHOD(GetPreference)  (DWORD dwNumber, PDWORD pdwValue) = 0;

   DEFMETHOD(ReadDirectoryExtension) (HANDLE hFile, LPVOID lpBuffer, 
										DWORD nNumberOfBytesToRead,
										LPDWORD lpNumberOfBytesRead=0, DWORD dwStartOffset=0) = 0;

   DEFMETHOD(WriteDirectoryExtension) (HANDLE hFile, LPCVOID lpBuffer, 
										DWORD nNumberOfBytesToWrite,
										LPDWORD lpNumberOfBytesWritten=0, DWORD dwStartOffset=0) = 0;

   DEFMETHOD_(LONG,SerialCall) (LPFILESYSTEM lpSystem, DAFILE_SERIAL_PROC lpProc, void *lpContext) = 0;

   DEFMETHOD_(BOOL,GetAbsolutePath) (char *lpOutput, LPCTSTR lpInput, LONG lSize) = 0;
};

#endif
