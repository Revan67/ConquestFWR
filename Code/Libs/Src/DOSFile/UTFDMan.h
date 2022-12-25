#ifndef UTFDMAN_H
#define UTFDMAN_H
//--------------------------------------------------------------------------//
//                                                                          //
//                               UTFDMan.H                                  //
//                                                                          //
//               COPYRIGHT (C) 1997 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//

/*
	Directory management for the UTF file system

	bFastWrite =1 means don't increase the size of the file to fit the header 
		and the names buffer until closing the file.
		Can only be set TRUE when sharing is off, and creating the file.


	The Names buffer is a series of ASCIIZ strings. The buffer is 
	terminated by two NULL characters.

	
	Unused space blocks look like this:
		 DWORD	dwSize	   // total size of the block
		 DWORD  dwOffset   // offset to next block, NULL == end of chain

*/

class BaseLockManager
{
	DWORD			dwLockCount;
	UTF_HEADER		header;
	UTF_DIR_ENTRY	*pDirectory;
	LPCTSTR			pNames;
	LPFILESYSTEM	pFile;
	DWORD			dwSharing;
	BOOL			bUpdated;
	FILETIME		OriginalTime;
	BOOL			bFastWrite;

	int addLock (void);
	
	int releaseLock (void);

	void flushHeader (void);   // should only be called if not using fastWrite or after FlushWriteCache() has been called.

public:

	int anySharing (void)
	{
		return (dwSharing != 0);
	}

	int writeSharing (void)
	{
		return (dwSharing & FILE_SHARE_WRITE);
	}

	int readSharing (void)
	{
		return (dwSharing & FILE_SHARE_READ);
	}

	void setFileSystem (LPFILESYSTEM _pFile)
	{
		pFile = _pFile;
	}
	
	void setSharing (DWORD dwValue)
	{
		dwSharing = dwValue;
	}

	void enableFastWrite (void)
	{
		bFastWrite = 1;
	}

	~BaseLockManager (void);

	friend class DirectoryManager;
};

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//


class DirectoryManager
{
	BaseLockManager *pBase;
	BOOL bLocked;

	UTF_HEADER * getHeader (void)
	{
		return &pBase->header;
	}

public:

	DirectoryManager (BaseLockManager & base)
	{
		pBase = &base;
		bLocked = pBase->addLock();
	}

	~DirectoryManager (void)
	{
		if (bLocked)
			pBase->releaseLock();
	}

	UTF_DIR_ENTRY * getDirectory (void)
	{
		return pBase->pDirectory;
	}

	DWORD getDirEntrySize (void)
	{
		return pBase->header.dwDirEntrySize;
	}
	
	DWORD getDataStartOffset (void)
	{
		return pBase->header.dwDataStartOffset;
	}
	
	LPCTSTR getNames (void)
	{
		return pBase->pNames;
	}

	BOOL isLocked (void)
	{
		return bLocked;
	}

	BOOL isFastWrite (void)
	{
		return pBase->bFastWrite;
	}

	BOOL freeDataArea (DWORD dwStartOffset, DWORD dwLength);

	DWORD allocDataArea (PDWORD pdwBytesNeeded);
	
	BOOL addEntries (int iNumEntries);
	
	BOOL increaseNameSpace (int iNumBytes);

	UTF_DIR_ENTRY * addChildEntry (UTF_DIR_ENTRY *pParent, int iExtra=0);

	BOOL removeEntry (UTF_DIR_ENTRY *pParent, UTF_DIR_ENTRY * pEntry);
	
	BOOL updateFileSize (UTF_DIR_ENTRY *pEntry, DWORD dwNewFileSize, DWORD dwExtra=0);

	DWORD addName (LPCTSTR lpName, U32 extraSpace=0);

	HANDLE makeTempFile (void);

	BOOL updateWriteTime (void);

	BOOL copyTmpData (DWORD dwDstOffset, DWORD dwSrcOffset, DWORD dwLength);

	BOOL flushWriteCache (void);

	static BOOL copyTmpData (HANDLE hTmpFile, LPFILESYSTEM pSrc, DWORD dwLength);
	
	static BOOL copyTmpData (LPFILESYSTEM pDst, HANDLE hTmpFile, DWORD dwLength);

	void flush (void)
	{
		flushWriteCache();		// reposition data after the new header
		pBase->flushHeader();	// write out the new header if needed
	}
};


#endif

//--------------------------------------------------------------------------//
//-------------------------------End UTFDMan.H------------------------------//
//--------------------------------------------------------------------------//

