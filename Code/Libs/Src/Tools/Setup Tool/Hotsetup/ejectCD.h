//
// ejectCD.h
//

#ifndef __EJECTCD_H
#define __EJECTCD_H
/*
   Program to programmatically eject removable media from a drive on
   Windows 95.
*/
#include <windows.h>
#include <winioctl.h>
#include <tchar.h>
#include <ctype.h>

//-----------------------------------------------------------------------
// DeviceIoControl infrastructure

#if !defined (VWIN32_DIOC_DOS_IOCTL) 
#define VWIN32_DIOC_DOS_IOCTL      1

typedef struct _DIOC_REGISTERS { 
    DWORD reg_EBX;
    DWORD reg_EDX;
    DWORD reg_ECX;
    DWORD reg_EAX;
    DWORD reg_EDI;
    DWORD reg_ESI;
    DWORD reg_Flags;
} DIOC_REGISTERS, *PDIOC_REGISTERS;
#endif 

// Intel x86 processor status flags
#define CARRY_FLAG             0x0001

//-----------------------------------------------------------------------
// DOS IOCTL function support

#pragma pack(1) 

// Parameters for locking/unlocking removable media
typedef struct _PARAMBLOCK { 
   BYTE bOperation;
   BYTE bNumLocks;
} PARAMBLOCK, *PPARAMBLOCK; 
#pragma pack() 


//-----------------------------------------------------------------------
// Win95 low-level media unlocking/removal support

HANDLE WINAPI OpenVWin32 (void);
BOOL WINAPI CloseVWin32 (HANDLE hVWin32);
BOOL WINAPI UnlockLogicalVolume95 (HANDLE hVWin32, BYTE bDriveNum);
BOOL WINAPI LockLogicalVolume95 (	HANDLE hVWin32, 
									BYTE   bDriveNum,
									BYTE   bLockLevel,
									WORD   wPermissions);
BOOL UnlockMedia95 (HANDLE hVWin32, BYTE bDrive);
BOOL EjectMedia95 (HANDLE hVWin32, BYTE bDrive); 

// NT Prototypes
BOOL EjectNT(TCHAR cDriveLetter);
HANDLE OpenVolumeNT(TCHAR cDriveLetter);
BOOL LockVolumeNT(HANDLE hVolume);
BOOL DismountVolumeNT(HANDLE hVolume);
BOOL PreventRemovalOfVolumeNT(HANDLE hVolume, BOOL fPrevent);
BOOL AutoEjectVolumeNT(HANDLE hVolume);
BOOL CloseVolumeNT(HANDLE hVolume);

// MAIN ENGINE CALL
BOOL EnableMediaSwap(void);

// NT defines
#define LOCK_TIMEOUT        10000       // 10 Seconds
#define LOCK_RETRIES        20

#ifdef _DEBUG
#define TRACE(x) OutputDebugString(x)
#else
#define TRACE(x)
#endif
#endif
