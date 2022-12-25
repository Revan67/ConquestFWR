#include "ejectCD.h"
#include "hotsetup.h"

extern CGlobals	g_Globals;


/*-----------------------------------------------------------------------
UnlockMedia95 (hVWin32, bDrive) 
Purpose: 

   Unlocks removable media from the specified drive so that it can be
   ejected.

Parameters: 
   hVWin32
      A handle to VWIN32. Used to issue request to unlock the media.

   bDrive
      The logical drive number to unlock. 0 = default, 1 = A, 2 = B,
      etc.

Return Value: 
   If successful, returns TRUE; if unsuccessful, returns FALSE.
-----------------------------------------------------------------------*/
BOOL UnlockMedia95 (HANDLE hVWin32, BYTE bDrive)
{ 
   DIOC_REGISTERS regs = {0};
   PARAMBLOCK     unlockParams = {0};
   int   i;
   BOOL  fResult;
   DWORD cb;

   // First, check the lock status. This way, you'll know the number of
   // pending locks you must unlock.

   unlockParams.bOperation = 2;   // return lock/unlock status

   regs.reg_EAX = 0x440D;
   regs.reg_EBX = bDrive;
   regs.reg_ECX = MAKEWORD(0x48, 0x08);
   regs.reg_EDX = (DWORD)&unlockParams;

   fResult = DeviceIoControl (hVWin32, VWIN32_DIOC_DOS_IOCTL,
                              &regs, sizeof(regs), &regs, sizeof(regs),
                              &cb, 0);

   // See if DeviceIoControl and the unlock succeeded.
   if (fResult)
   {
      /*
         DeviceIoControl succeeded. Now see if the unlock succeeded. It
         succeeded if the carry flag is not set, or if the carry flag is
         set but EAX is 0x01 or 0xB0.

         It failed if the carry flag is set and EAX is not 0x01 or 0xB0.

         If the carry flag is clear, then unlock succeeded. However, you
         don't need to set fResult because it is already TRUE when you get
         in here.

      */
      if (regs.reg_Flags & CARRY_FLAG)
         fResult = (regs.reg_EAX == 0xB0) || (regs.reg_EAX == 0x01);
   }

   if (!fResult)
      return (FALSE);

   // Now, let's unlock the media for every time it has been locked;
   // this will totally unlock the media.

   for (i = 0; i < unlockParams.bNumLocks; ++i)
   {
      unlockParams.bOperation = 1;   // unlock the media

      regs.reg_EAX = 0x440D;
      regs.reg_EBX = bDrive;
      regs.reg_ECX = MAKEWORD(0x48, 0x08);
      regs.reg_EDX = (DWORD)&unlockParams;

      fResult = DeviceIoControl (hVWin32, VWIN32_DIOC_DOS_IOCTL,
                                 &regs, sizeof(regs), &regs, sizeof(regs),
                                 &cb, 0);

      // See if DeviceIoControl and the lock succeeded
      fResult = fResult && !(regs.reg_Flags & CARRY_FLAG);
      if (!fResult)
         break;
   }
   return fResult;
} 

/*-----------------------------------------------------------------------
EjectMedia95 (hVWin32, bDrive) 
Purpose: 

   Ejects removable media from the specified drive.

Parameters: 
   hVWin32
      A handle to VWIN32. Used to issue request to unlock the media.

   bDrive
      The logical drive number to unlock. 0 = default, 1 = A, 2 = B,
      etc.

Return Value: 
   If successful, returns TRUE; if unsuccessful, returns FALSE.
-----------------------------------------------------------------------*/ 
BOOL EjectMedia95 (HANDLE hVWin32, BYTE bDrive)
{ 
	return TRUE;
/*   DIOC_REGISTERS regs = {0};
   BOOL  fResult;
   DWORD cb;

   regs.reg_EAX = 0x440D;
   regs.reg_EBX = bDrive;
   regs.reg_ECX = MAKEWORD(0x49, 0x08);

   fResult = DeviceIoControl (hVWin32, VWIN32_DIOC_DOS_IOCTL,
                              &regs, sizeof(regs), &regs, sizeof(regs),
                              &cb, 0);

   // See if DeviceIoControl and the lock succeeded
   fResult = fResult && !(regs.reg_Flags & CARRY_FLAG);

   return fResult;*/
} 

/*-----------------------------------------------------------------------
OpenVWin32 () 
Purpose: 

   Opens a handle to VWIN32 that can be used to issue low-level disk I/O
   commands.

Parameters: 
   None.

Return Value: 
   If successful, returns a handle to VWIN32.

   If unsuccessful, return INVALID_HANDLE_VALUE. Call GetLastError() to
   determine the cause of failure.
-----------------------------------------------------------------------*/ 
HANDLE WINAPI OpenVWin32 (void) { 
   return CreateFile ("\\\\.\\vwin32", 0, 0, NULL, 0,
                      FILE_FLAG_DELETE_ON_CLOSE, NULL);
}

/*-----------------------------------------------------------------------
CloseVWin32 (hVWin32) 
Purpose: 

   Closes the handle opened by OpenVWin32.

Parameters: 
   hVWin32
      An open handle to VWIN32.

Return Value: 
   If successful, returns TRUE. If unsuccessful, returns FALSE. Call
   GetLastError() to determine the cause of failure.
-----------------------------------------------------------------------*/ 
BOOL WINAPI CloseVWin32 (HANDLE hVWin32) { 
   return CloseHandle (hVWin32);
} 

/*-----------------------------------------------------------------------
LockLogicalVolume95 (hVWin32, bDriveNum, bLockLevel, wPermissions) 
Purpose: 

   Takes a logical volume lock on a logical volume.

Parameters: 
   hVWin32
      An open handle to VWIN32.

   bDriveNum
      The logical drive number to lock. 0 = default, 1 = A:, 2 = B:,
      3 = C:, etc.

   bLockLevel
      Can be 0, 1, 2, or 3. Level 0 is an exclusive lock that can only
      be taken when there are no open files on the specified drive.
      Levels 1 through 3 form a hierarchy where 1 must be taken before
      2, which must be taken before 3.

   wPermissions
      Specifies how the lock will affect file operations when lock levels
      1 through 3 are taken. Also specifies whether a formatting lock
      should be taken after a level 0 lock.

      Zero is a valid permission.

Return Value: 
   If successful, returns TRUE.  If unsuccessful, returns FALSE.
-----------------------------------------------------------------------*/ 
BOOL WINAPI LockLogicalVolume95 (	HANDLE hVWin32, 
									BYTE   bDriveNum,
									BYTE   bLockLevel,
									WORD   wPermissions)
{
	BOOL			fResult=FALSE;
	DIOC_REGISTERS	regs = {0};
	BYTE			bDeviceCat;  // can be either 0x48 or 0x08
	DWORD			cb;
	DWORD dwSleepAmount;
	int nTryCount;
	dwSleepAmount = LOCK_TIMEOUT / LOCK_RETRIES;
	// Do this in a loop until a timeout period has expired
	for (nTryCount = 0; nTryCount < LOCK_RETRIES; nTryCount++)
	{
		/*
		  Try first with device category 0x48 for FAT32 volumes. If it
		  doesn't work, try again with device category 0x08. If that
		  doesn't work, then the lock failed.
		*/
		
		bDeviceCat = 0x48;
		
		// Set up the parameters for the call.
		regs.reg_EAX = 0x440D;
		regs.reg_EBX = MAKEWORD(bDriveNum, bLockLevel);
		regs.reg_ECX = MAKEWORD(0x4A, bDeviceCat);
		regs.reg_EDX = wPermissions;
		
		fResult = DeviceIoControl (hVWin32, VWIN32_DIOC_DOS_IOCTL,
								  &regs, sizeof(regs), &regs, sizeof(regs),
								  &cb, 0);

		// See if DeviceIoControl and the lock succeeded
		fResult = fResult && !(regs.reg_Flags & CARRY_FLAG);
		// If DeviceIoControl or the lock failed, and device category 0x08
		// hasn't been tried, retry the operation with device category 0x08.
		if (fResult)
		{
			return fResult;
		}
		else
		{
			bDeviceCat = 0x08;
			// Set up the parameters for the call.
			regs.reg_EAX = 0x440D;
			regs.reg_EBX = MAKEWORD(bDriveNum, bLockLevel);
			regs.reg_ECX = MAKEWORD(0x4A, bDeviceCat);
			regs.reg_EDX = wPermissions;
			
			fResult = DeviceIoControl (hVWin32, VWIN32_DIOC_DOS_IOCTL,
									  &regs, sizeof(regs), &regs, sizeof(regs),
									  &cb, 0);

			// See if DeviceIoControl and the lock succeeded
			fResult = fResult && !(regs.reg_Flags & CARRY_FLAG);
		}
		if (fResult)
		{
			return fResult;
		}
		Sleep(0);
		Sleep(dwSleepAmount);
	}
#ifdef _DEBUG
	// 
	// Find out what is Keeping us from getting a volume lock so it can be remedied.
	//
	TCHAR szMessage1[1024];
	#pragma pack(1) 
	TCHAR szOPENFILE[_MAX_PATH];
	DWORD dwFileIndex=0;
	#pragma pack()
	// 0x08 win95a FAT only
	// 0x48 win95b FAT or possibly FAT32
	// TRY FAT32 First
	bDeviceCat = 0x48;
	BOOL fFirstTime = TRUE;
	do
	{
	regs.reg_EAX = 0x440D;
	regs.reg_EBX = MAKEWORD(bDriveNum,0x00);
	regs.reg_ECX = MAKEWORD(0x6D, bDeviceCat);
	regs.reg_EDX = (DWORD) szOPENFILE;
	regs.reg_ESI = dwFileIndex;
	regs.reg_EDI = 0x0000; // All Open Files
	regs.reg_Flags = 0x0001; // Assume an Error
	fResult = DeviceIoControl (hVWin32, VWIN32_DIOC_DOS_IOCTL,
							  &regs, sizeof(regs), &regs, sizeof(regs),
							  &cb, 0);
	// See if DeviceIoControl and the lock succeeded
	fResult = fResult && !(regs.reg_Flags & CARRY_FLAG);
	if (fFirstTime)
	{
		fFirstTime = FALSE;
		// TRY FAT instead
		if (!fResult)
		{
			bDeviceCat = 0x08;
			continue;
		}
	}
	if ( !fResult )
		break;
	wsprintf(szMessage1, "FILEOPEN: %s, MODE: %x, TYPE: %x.\n", szOPENFILE, regs.reg_EAX, regs.reg_ECX);
	TRACE(szMessage1);
	MessageBox(g_Globals.GetWndParent(), szMessage1, g_Globals.GetSetupTitle(),	MB_ICONEXCLAMATION | MB_OK);
	dwFileIndex++;
	if ( ERROR_NO_MORE_FILES == regs.reg_EAX )
		break;
	if ( ERROR_NO_MORE_FILES == regs.reg_ECX )
		break;
	}	while ( fResult ) ;
#endif
	return FALSE;
}

/*-----------------------------------------------------------------------
UnlockLogicalVolume95 (hVWin32, bDriveNum) 
Purpose: 

   Unlocks a logical volume that was locked with LockLogicalVolume95().

Parameters: 
   hVWin32
      An open handle to VWIN32.

   bDriveNum
      The logical drive number to unlock. 0 = default, 1 = A:, 2 = B:,
      3 = C:, etc.

Return Value: 
   If successful, returns TRUE. If unsuccessful, returns FALSE.

Comments: 
   Must be called the same number of times as LockLogicalVolume95() to
   completely unlock a volume.

   Only the lock owner can unlock a volume.
-----------------------------------------------------------------------*/ 
BOOL WINAPI UnlockLogicalVolume95 (HANDLE hVWin32, BYTE bDriveNum) { 
   BOOL           fResult;
   DIOC_REGISTERS regs = {0};
   BYTE           bDeviceCat;  // can be either 0x48 or 0x08
   DWORD          cb;

   /* Try first with device category 0x48 for FAT32 volumes. If it
      doesn't work, try again with device category 0x08. If that
      doesn't work, then the unlock failed.
   */

   bDeviceCat = 0x48;

ATTEMPT_AGAIN: 
   // Set up the parameters for the call.
   regs.reg_EAX = 0x440D;
   regs.reg_EBX = bDriveNum;
   regs.reg_ECX = MAKEWORD(0x6A, bDeviceCat);

   fResult = DeviceIoControl (hVWin32, VWIN32_DIOC_DOS_IOCTL,
                              &regs, sizeof(regs), &regs, sizeof(regs),
                              &cb, 0);

   // See if DeviceIoControl and the unlock succeeded
   fResult = fResult && !(regs.reg_Flags & CARRY_FLAG);

   // If DeviceIoControl or the unlock failed, and device category 0x08
   // hasn't been tried, retry the operation with device category 0x08.
   if (!fResult && (bDeviceCat != 0x08))
   {
      bDeviceCat = 0x08;
      goto ATTEMPT_AGAIN;
   }
   return fResult;
}

BOOL EjectWIN95 (LPTSTR szRootPath)
{ 
	HANDLE hVWin32	   = INVALID_HANDLE_VALUE;
	BYTE   bDrive;
	BOOL   fDriveLocked = FALSE;
	TCHAR  szMessage[1024];	
	BOOL	fResult = FALSE;

	// convert command line arg 1 from a drive letter to a DOS drive number
	if ( islower( *szRootPath ) )
		*szRootPath = toupper( *szRootPath );
	
	// convert to DOS drive letter.
	bDrive = (*szRootPath - 'A') + 1;
	
	hVWin32 = OpenVWin32 ();
	if ( INVALID_HANDLE_VALUE == hVWin32 )
	{
		fResult = FALSE;
		goto CLEANUP_AND_EXIT_APP;
	}
	
	// Make sure no other applications are using the drive.
	fDriveLocked = LockLogicalVolume95 (hVWin32, bDrive, 0, 0);
	if (!fDriveLocked)
	{
		wsprintf(szMessage, "volume %c is in use by another application; therefore, it "
				"cannot be ejected\n", 'A' + bDrive - 1);
		TRACE(szMessage);
#ifdef _DEBUG
		MessageBox(g_Globals.GetWndParent(), szMessage, g_Globals.GetSetupTitle(),	MB_ICONEXCLAMATION | MB_OK);
#endif
		fResult = FALSE;
		goto CLEANUP_AND_EXIT_APP;
	}
	
	// Make sure there is no software lock keeping the media in the drive.
	if (!UnlockMedia95 (hVWin32, bDrive))
	{
		wsprintf(szMessage,"could not unlock media from drive %c:\n", 'A' + bDrive - 1);
		TRACE(szMessage);
#ifdef _DEBUG
		MessageBox(g_Globals.GetWndParent(), szMessage, g_Globals.GetSetupTitle(),	MB_ICONEXCLAMATION | MB_OK);
#endif
		fResult = FALSE;
		goto CLEANUP_AND_EXIT_APP;
	}
	
	// Eject the media.
	if (!EjectMedia95 (hVWin32, bDrive))
	{
		wsprintf(szMessage, "could not eject media from drive %c:\n", 'A' + bDrive - 1);
		TRACE(szMessage);
#ifdef _DEBUG
		MessageBox(g_Globals.GetWndParent(), szMessage, g_Globals.GetSetupTitle(),	MB_ICONEXCLAMATION | MB_OK);
#endif
	}
	
	// Regardless of software enabled disk ejection.
	// the device should be free for swapping.
	fResult = TRUE;
CLEANUP_AND_EXIT_APP: 
	if (fDriveLocked)
		UnlockLogicalVolume95 (hVWin32, bDrive);
	
	if (hVWin32 != INVALID_HANDLE_VALUE)
		CloseVWin32 (hVWin32);
	return fResult;
} 

HANDLE OpenVolumeNT(TCHAR szRootPath, UINT uDriveType)
{
	HANDLE hVolume;
	DWORD dwAccessFlags;
	LPTSTR szVolumeFormat = TEXT("\\\\.\\%c:");
	TCHAR szVolumeName[8];

	switch(uDriveType)
	{
	case DRIVE_REMOVABLE:
	    dwAccessFlags = GENERIC_READ | GENERIC_WRITE;
	    break;
	case DRIVE_CDROM:
	    dwAccessFlags = GENERIC_READ;
	    break;
	default:
	    return INVALID_HANDLE_VALUE;
	}
	
	wsprintf(szVolumeName, szVolumeFormat, (TCHAR *) szRootPath);
	hVolume = CreateFile(	szVolumeName,
							dwAccessFlags,
							FILE_SHARE_READ | FILE_SHARE_WRITE,
							NULL,
							OPEN_EXISTING,
							0,
							NULL );
	return hVolume;
}

BOOL CloseVolumeNT(HANDLE hVolume)
{
	return CloseHandle(hVolume);
}

BOOL LockVolumeNT(HANDLE hVolume)
{
	DWORD dwBytesReturned;
	DWORD dwSleepAmount;
	int nTryCount;
	dwSleepAmount = LOCK_TIMEOUT / LOCK_RETRIES;
	// Do this in a loop until a timeout period has expired
	for (nTryCount = 0; nTryCount < LOCK_RETRIES; nTryCount++)
	{
		if (DeviceIoControl(hVolume,
							FSCTL_LOCK_VOLUME,
							NULL, 0,
							NULL, 0,
							&dwBytesReturned,
							NULL))
			return TRUE;
		Sleep(dwSleepAmount);
	}
	return FALSE;
}

BOOL DismountVolumeNT(HANDLE hVolume)
{
	DWORD dwBytesReturned;
	return DeviceIoControl( hVolume,
							FSCTL_DISMOUNT_VOLUME,
							NULL, 0,
							NULL, 0,
							&dwBytesReturned,
							NULL);
}

BOOL PreventRemovalOfVolumeNT(HANDLE hVolume, BOOL fPreventRemoval)
{
	DWORD dwBytesReturned;
	PREVENT_MEDIA_REMOVAL PMRBuffer;
	PMRBuffer.PreventMediaRemoval = fPreventRemoval;
	return DeviceIoControl( hVolume,
							IOCTL_STORAGE_MEDIA_REMOVAL,
							&PMRBuffer, sizeof(PREVENT_MEDIA_REMOVAL),
							NULL, 0,
							&dwBytesReturned,
							NULL);
}

BOOL AutoEjectVolumeNT(HANDLE hVolume)
{
	return TRUE;
/*	DWORD dwBytesReturned;
	return DeviceIoControl( hVolume,
							IOCTL_STORAGE_EJECT_MEDIA,
							NULL, 0,
							NULL, 0,
							&dwBytesReturned,
							NULL);*/
}

BOOL EjectNT(TCHAR cDriveLetter, UINT uiDriveType)
{
	HANDLE	hVolume;
	BOOL	fResult=FALSE;
	// Open the volume.
	hVolume = OpenVolumeNT(cDriveLetter, uiDriveType);
	if (hVolume == INVALID_HANDLE_VALUE)
		return fResult;
	
	// Lock and dismount the volume.
	if (LockVolumeNT(hVolume) && DismountVolumeNT(hVolume))
	{
		fResult = TRUE;
		// Set prevent removal to false and eject the volume.
		if (PreventRemovalOfVolumeNT(hVolume, FALSE))
		{
			AutoEjectVolumeNT(hVolume);
		}
	}
	
	CloseVolumeNT(hVolume);
	return fResult;
}

BOOL EnableMediaSwap()
{
	TCHAR	szSourcePath[_MAX_PATH] = TEXT("");
	WORD	wOS=OS_NOTSUPPORTED;
	TCHAR	szRootPath[5] = TEXT("?:\\");
	UINT	uiDriveType = DRIVE_UNKNOWN;

	lstrcpy(szSourcePath, g_Globals.GetSourcePath());
	// Check for UNC, if UNC return FALSE and Sleep along time.
	if ('\\' == *szSourcePath)
	{
		return FALSE;
	}
	else
	{
		// Get the Drive Type
		*szRootPath = *szSourcePath;
		uiDriveType = GetDriveType(szRootPath);
		switch	(uiDriveType) 
		{
		case DRIVE_REMOVABLE:
		case DRIVE_CDROM:
			wOS = g_Globals.GetOS();
			switch (wOS)
			{
			case OS_NT50:
			case OS_NT40:
				TRACE("Ejecting disk on NT.\n");
				return EjectNT(*szRootPath, uiDriveType);
			case OS_WIN98:
			case OS_WIN95:
				TRACE("Ejecting disk on WIN.\n");
				return EjectWIN95(szRootPath);
			default:
				return FALSE;
			}
			break;
		default:
			return FALSE;
		}
	}
}
