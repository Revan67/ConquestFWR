//
// Script.h
//
// History:
//
//  3/6/97 Created, took defines out of setup.h, to allow UI's to include this file, without
//         the internal setup stuff getting pulled in.  Any .rc that includes the rc generated
//		   by Prepstub should include this file rather than setup.h
//
#ifndef __SCRIPT_H
#define __SCRIPT_H

#include "dsetup.h"

#define IF_WINDOWSDIR   0x0001  // File gets installed to \windows
#define IF_SYSTEMDIR    0x0002  // File gets installed to \windows\system
#define IF_SYSTEMFILE   0x0004  // File copy requires Windows restart
#define IF_SHAREDFILE   0x0008  // File may be open by other apps
#define IF_UNINSTALL    0x0010  // File can be uninstalled
#define IF_FONTFILE     0x0020  // Font file
#define IF_APPDIR       0x0040  // install in application dir
#define IF_DLLREGISTER	0x0080	// Call DLLRegisterServer for this DLL.
#define IF_CAB			0x0100  // File is in cab file
#define IF_NOPROMPT		0x0200  // Don't prompt for file delete
#define IF_UNINSTALLALL 0x0400  // Recursive uninstall (MkDir IniValue DeleteFile flag)
#define IF_NOTUNINSTALL 0x0800  // Ignore during uninstall (DeleteFile flag)
#define IF_INSTALL		0x1000  // Ignore during uninstall (DeleteFile flag)
#define	IF_UNINSTALLLINK 0x2000 // During uninstall InstIcon Delete the MenuItem Only Not its folder
#define IF_UNINSTONLY	0x4000  // Ignore during install (InstallList flag)

//*** Commands available in SETUPCOMMAND

//#define SC_INSTALLFILE  1	// Install a file
#define SC_ADDINIVALUE  3	// Write entry to named .ini file
#define SC_CABGO		4	// Execute cab file extraction list
//#define SC_INSTALLFONT  7	// Install a font
#define SC_MKDIR		8	// Create a Directory (uses input root dir)
#define SC_MKROOT       9	// Create a Directory (uses input root dir)
#define SC_GETNAME      10  // Get player name
#define SC_GETPID       11  // Get PID
#define SC_INSTDX       12  // Install DirectX
#define SC_INSTICON     13  // Install Icons
#define SC_CDSPEED      14  // Test CD Speed
#define SC_INSTALLLIST	15  // Install list entry
#define SC_INSTALLGO    16  // Execute install list
#define SC_DELETEFILE	17  // Delete File (uninstall only)
#define SC_GETGROUP		18  // Get file groups	
#define SC_READFILELIST 19  // Development load a file list	
#define SC_REGWIZ       20  // ShellExecute specified URL 
#define SC_SHELLEXECUTE 21	// ShellExecute specified file
#define SC_INSTDPLAY	22  // Install DirectPlay
#define SC_ERROR		23

//
//Engine status codes
//
#define SS_CHECKHARDWARE		SC_ERROR + 1	//Checking hardware
#define SS_CHECKDISKSPACE		SC_ERROR + 2	//Checking disk space
#define SS_PREPARINGFILELIST	SC_ERROR + 3	//Preparing list of files
#define SS_BEGINUNINSTALL		SC_ERROR + 4	//Beginning uninstall process
#define SS_BEGININSTALL			SC_ERROR + 5	//Beginning install process
#define SS_BEGINMAINTAIN		SC_ERROR + 6	//Beginning maintainence process
#define SS_ENDUNINSTALL			SC_ERROR + 7	//End uninstall process
#define SS_ENDINSTALL			SC_ERROR + 8	//End install process
#define SS_ENDMAINTAIN			SC_ERROR + 9	//End maintainence process

//
//Operating system flags for scripting and engine use - 10, 20, 40, 80 reserved...
//
#define OS_NOTSUPPORTED 0x00000000  // NT 1.x, 2.x, 3.x currently

#define OS_WIN95		0x00000001	// Windows 95 or later
#define OS_WIN98		0x00000002	// Windows 98 or later
#define OS_WINMASK      0x00000003  // Any WIN40 operating system

#define OS_NT40			0x00000004	// Windows NT 4.0 or later 
#define OS_NT50         0x00000008  // Windows NT 5.0 or later
#define OS_NTMASK		0x0000000C	// NT 4.0 and 5.0 operating systems

#define OS_ALLMASK		0x000000FF

//
//Setup command flags for IME support
//
#define SCF_IME_DISABLE 0x00000100  // disables IME
#define SCF_IME_ENABLE  0x00000200  // enables IME
#define SCF_IME_ON      0x00000400  // turn on IME(use with SCF_IME_ENABLE)
#define SCF_IME_MASK    0x00000700  // mask pattern for SCF_IME_*

//
//Prepstub99 cab pre-copy flag
//
#define BLD_CAB_PRECOPY 0x00000800  // place the file in the "pre-copy" folder?

//
//Type of build (platform) flags
//
#define BLD_DBCS        0x00001000		//DBCS platform only
#define BLD_ANSI		0x00002000		//ANSI build only (versus DBCS build)

#define BLD_OEM			0x00004000		//OEM build only
#define BLD_RTL			0x00008000		//Retail build only (versus OEM build)

#define BLD_APP1		0x00010000		//U.I. can set/unset this flag via code
#define BLD_APP2		0x00020000		//U.I. can set/unset this flag via code 
#define BLD_APP3		0x00040000		//U.I. can set/unset this flag via code 

#define BLD_TYPEMASK	0x000FF000

//
//Language flags, 0x02...,4,8 and 0x1...,2,4,8 reserved (seven more languages before we go to __int64)
//
#define BLD_JPN			0x00100000		//Japan only
#define BLD_GER			0x00200000		//German only
#define BLD_FRA			0x00400000		//French only
#define BLD_SPA			0x00800000		//Spanish only
#define BLD_USA			0x01000000	    //USA only

#define BLD_LANGMASK	0xFFF00000

#endif //__SCRIPT_H

