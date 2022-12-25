//
// Setup.h
//
// History:
//
//  1/26/95 KenSh       Created
//  6/21/95 KenSh       Added the IF_UNINSTALL flag
//  8/22/95 a-DenSo     Added Font support
//	 8/29/96	a-melodh	   comment out g_fWritePID so don't get link err
//

#ifndef __SETUP_H
#define __SETUP_H

#include "vercopy.h"
#include "script.h"
#include "stdio.h"
#include "mem.h"
#include "print.h"
#include "hotsetup.h"

#define MAX_ARG				12			//max # of args on the command line
#define MAX_PLAYERNAME_LENGTH	100
#define MAX_DATA_LENGTH	2048

EBURETCODE CheckFileSpaceRequirements(DWORD dwClustersNeeded, DWORD dwSysClustersNeeded,char *szDestDrive);
BOOL MyRefCountSharedDll(LPCSTR lpszValue, BOOL fBumpRefCount);

//*** Return values for InstallVersionedFile(), also used by
//    ShouldFileBeInstalled()
//
#define IVF_ERR_NOMEMORY    0x0001  // Ran out of memory, didn't copy the file
#define IVF_ERR_USERABORT   0x0002  // Couldn't copy, user clicked Abort
#define IVF_ERR_USERIGNORE  0x0004  // Couldn't copy, user clicked Ignore
#define IVF_ADDTOCOPYLIST   0x0008  // Couldn't open dest, must copy via ExitWindowsExec
#define IVF_SUCCESS_COPY    0x0010  // File was copied successfully
#define IVF_SUCCESS_NOCOPY  0x0020  // File didn't need to be copied
#define IVF_SUCCESSMASK     0x0030  // use this to determine success


//****************************************************************************
//*** INSTALLFILE - This struct is stored as a variable-length resource as part
//                  of a SETUPCOMMAND (below).  This struct corresponds to the
//                  InstallFile setup command.
//
typedef struct FAR tagINSTALLFILE
{
    FILEINFO    FileInfo;       // Date/time/version/size of the source file
    WORD        wFlags;         // combination of the IF_* flags below.
    WORD        wDestOffset;    // offset from szName to destination name
	char        szName[MAX_DATA_LENGTH];      // source filename + dest filename, see below
} INSTALLFILE;

class CInstallFile : public tagINSTALLFILE
{
public:
	CInstallFile()	
		{
		wFlags = wDestOffset = 0;
#ifdef _DEBUG
		// These fields must be initialized before use so let's put garbage in them
		FillMemory(&FileInfo, sizeof(FILEINFO), bGarbage);
		FillMemory(&szName, MAX_DATA_LENGTH, bGarbage);
#endif //_DEBUG
		}

    //Returns predetermined timestamp/version/language for source file
    LPFILEINFO  GetSourceFileInfo()
                    { return &FileInfo; }

    //Returns relative pathname of source file, relative to the
    //directory on the CD where the stub exe sits.
    LPCSTR      GetSourceFileName()
                    { return szName; }

    //Returns the file title of the destination (i.e. no path info)
    LPCSTR      GetDestFileName()
                    { return &szName[wDestOffset]; }

    BOOL        fCopyToWindowsDir()
                    { return (BOOL)( (UINT)wFlags & (UINT)IF_WINDOWSDIR ); }
    BOOL        fCopyToSystemDir()
                    { return (BOOL)( (UINT)wFlags & (UINT)IF_SYSTEMDIR ); }
    BOOL        fCopyToAppDir()
                    { return (BOOL)( (UINT)wFlags & (UINT)IF_APPDIR ); }
    BOOL        fIsSystemFile()
                    { return (BOOL)( (UINT)wFlags & (UINT)IF_SYSTEMFILE ); }
    BOOL        fIsSharedFile()
                    { return (BOOL)( (UINT)wFlags & (UINT)IF_SHAREDFILE ); }
    BOOL        fIsUninstallFile()
                    { return (BOOL)( (UINT)wFlags & (UINT)IF_UNINSTALL ); }
    BOOL        fIsFontFile()
                    { return (BOOL)( (UINT)wFlags & (UINT)IF_FONTFILE ); }
    BOOL        fDLLRegister()
                    { return (BOOL)( (UINT)wFlags & (UINT)IF_DLLREGISTER ); }
    BOOL        fUninstallOnly()
                    { return (BOOL)( (UINT)wFlags & (UINT)IF_UNINSTONLY ); }
    char *      GetDestPath()
                    { return((szName[0]) ? &szName[0]+lstrlen(&szName[0])+1 : NULL); }

	__int64		GetGroup()
	{
	   __int64 group;
	   sscanf(GetDestPath()+lstrlen(GetDestPath())+1,"%I64X",&group);
	   return group;
	}

	int			GetDiskID()
	{
		int DiskID;
		//scan in the disk id, but first find ist location in the structure.
		sscanf(GetDestPath() + lstrlen(GetDestPath())+1 + (lstrlen(GetDestPath() + lstrlen(GetDestPath())+1)+1), "%d", &DiskID);
		return DiskID;
	}

};
typedef CInstallFile FAR* LPINSTALLFILE;


typedef struct FAR tagREADFILELIST
{
    char        szName[1];      // file name of the file to read in
} READFILELIST;

class CReadFileList : public tagREADFILELIST
{
public:
    char *      ReadFileListName()
                    { return(&szName[0]); }
};
typedef CReadFileList FAR *LPREADFILELIST;


typedef struct FAR tagGETGROUP
{
    WORD        wFlags;         // combination of the IF_* flags below.
} GETGROUP;

class CGetGroup : public tagGETGROUP
{
public:
	CGetGroup()
		{
		wFlags = 0;
		}
};
typedef CGetGroup FAR *LPGETGROUP;


typedef struct FAR tagMKDIR
{
    WORD        wFlags;         // combination of the IF_* flags below.
    char        szName[MAX_DATA_LENGTH];
} MKDIR;

class CMkDir : public tagMKDIR
{
public:
	CMkDir()
		{
		wFlags = 0;
#ifdef _DEBUG
		// These fields must be initialized before use so let's put garbage in them
		FillMemory(&szName, MAX_DATA_LENGTH, bGarbage);
#endif //_DEBUG
		}
    BOOL        fIsUninstallFile()
                    { return (BOOL)( (UINT)wFlags & (UINT)IF_UNINSTALL ); }
    char *      GetMkDir()
                    { return(&szName[0]); }
	__int64		GetDirGroup()
					{
					   __int64 group;
					   sscanf(GetMkDir()+lstrlen(GetMkDir())+1,"%I64X",&group);
					   return group;
					}
};
typedef CMkDir FAR* LPMKDIR;


typedef struct FAR tagMKROOT
{
    WORD        wFlags;         // combination of the IF_* flags below.
} MKROOT;


class CMkRoot : public tagMKROOT
{
public:
	CMkRoot()
		{
		wFlags = 0;
		}
    BOOL        fIsUninstallFile()
                    { return (BOOL)( (UINT)wFlags & (UINT)IF_UNINSTALL ); }
};
typedef CMkRoot FAR* LPMKROOT;

EBURETCODE ExecuteRdRoot(LPMKROOT lpMkRoot);

typedef struct FAR tagGETNAME
{
    WORD        wFlags;         // combination of the IF_* flags below.
} GETNAME;

class CGetName : public tagGETNAME
{
public:
	CGetName()
		{
		wFlags = 0;
		}
    BOOL        fIsUninstallFile()
                    { return (BOOL)( (UINT)wFlags & (UINT)IF_UNINSTALL ); }
};
typedef CGetName FAR* LPGETNAME;


typedef struct FAR tagDELETEFILE
{
    WORD        wFlags;         // combination of the IF_* flags below.
    char        szName[MAX_DATA_LENGTH];
} DELETEFILE;

class CDeleteFile : public tagDELETEFILE
{
public:
	CDeleteFile()
		{
		wFlags = 0;
#ifdef _DEBUG
		// These fields must be initialized before use so let's put garbage in them
		FillMemory(&szName, MAX_DATA_LENGTH, bGarbage);
#endif //_DEBUG
		}
    char *      GetDeleteFileName()
                    { return(&szName[0]); }
	BOOL        GetDeleteFileSilentFlag()
					{ return wFlags & IF_UNINSTALL; }
    BOOL        fIsPersistFile()
                    { return (BOOL)( (UINT)wFlags & (UINT)IF_NOTUNINSTALL ); }
    BOOL        fIsRecurseFile()
                    { return (BOOL)( (UINT)wFlags & (UINT)IF_UNINSTALLALL ); }
    BOOL        fIsInstallFile()
                    { return (BOOL)( (UINT)wFlags & (UINT)IF_INSTALL ); }
};
typedef CDeleteFile FAR* LPDELETEFILE;


typedef struct FAR tagGETPID
{
    WORD        wFlags;         // combination of the IF_* flags below.
} GETPID;

class CGetPID : public tagGETPID
{
public:
	CGetPID()
		{
			wFlags = 0;
		}
    BOOL        fIsUninstallFile()
                    { return (BOOL)( (UINT)wFlags & (UINT)IF_UNINSTALL ); }
};
typedef CGetPID FAR* LPGETPID;


typedef struct FAR tagINSTDX
{
    WORD        wFlags;         // combination of the IF_* flags below.
    char        szName[MAX_DATA_LENGTH];
} INSTDX;

class CInstDX : public tagINSTDX
{
public:
	CInstDX()
		{
		wFlags = 0;
#ifdef _DEBUG
		// These fields must be initialized before use so let's put garbage in them
		FillMemory(&szName, MAX_DATA_LENGTH, bGarbage);
#endif //_DEBUG
		}

    BOOL        fIsUninstallFile()
                    { return (BOOL)( (UINT)wFlags & (UINT)IF_UNINSTALL ); }
    char *       GetInstDX()
                    { return(&szName[0]); }

    char *       GetInstDXName()
                    { return((&szName[0])+(lstrlen(GetInstDX())+1)); }

    char *       GetInstDXMinVersion()
                    { return((GetInstDXName())+(lstrlen(GetInstDXName())+1)); }

    DWORD *      GetInstDXFlags()
                    { return((DWORD *)(GetInstDXMinVersion()+lstrlen(GetInstDXMinVersion())+1)); }
};
typedef CInstDX FAR* LPINSTDX;

typedef struct FAR tagINSTDPLAY
{
    WORD        wFlags;         // combination of the IF_* flags below.
    char        szName[MAX_DATA_LENGTH];
} INSTDPLAY;

class CInstDPLAY : public tagINSTDPLAY
{
public:
	CInstDPLAY()
		{
		wFlags = 0;
#ifdef _DEBUG
		// These fields must be initialized before use so let's put garbage in them
		FillMemory(&szName, MAX_DATA_LENGTH, bGarbage);
#endif //_DEBUG
		}

    char *       GetInstDPLAYName()
					{ return(&szName[0]); }

    char *       GetInstDPLAYMinVersion()
                    { return((GetInstDPLAYName())+(lstrlen(GetInstDPLAYName())+1)); }

};

typedef CInstDPLAY FAR* LPINSTDPLAY;

typedef struct FAR tagShellExecute
{
    WORD        wFlags;         // combination of IF_* flags
    char        szName[MAX_DATA_LENGTH];
} SHELLEXECUTESTRUCT;

class CShellExecute : public tagShellExecute
{
public:
	CShellExecute()
		{
		wFlags = 0;
#ifdef _DEBUG
		// These fields must be initialized before use so let's put garbage in them
		FillMemory(&szName, MAX_DATA_LENGTH, bGarbage);
#endif //_DEBUG
		}

	BOOL		RunDuringUninstall()
					{ return (BOOL) ( (UINT)wFlags & (UINT)IF_UNINSTALL ); }
	char *		GetFileName()
					{ return(&szName[0]); }
	char *		GetDirectory()
					{ return(((&szName[0])+lstrlen(&szName[0])+1)); }
	char *		GetParameters()
					{ return(GetDirectory()+lstrlen(GetDirectory())+1); }
	int			GetShowFlags()
					{ return (int) *((int *) (GetParameters()+lstrlen(GetParameters())+1)); }
	BOOL		GetWait()
					{ return (BOOL) '1' == (*(char *) (GetParameters() +
					                          (lstrlen(GetParameters())+1)+sizeof(int))); }
	__int64		GetGroup()
	{
	   __int64 group;
	   sscanf((char *) (GetParameters()+lstrlen(GetParameters())+1+sizeof(int)+lstrlen("1")+1),
		      "%I64X",
			  &group);
	   return group;
	}
};
typedef CShellExecute FAR *LPSHELLEXECUTE;

typedef struct FAR tagINSTICON
{
    WORD        wFlags;         // combination of the IF_* flags below.
    char        szName[MAX_DATA_LENGTH];
} INSTICON;

class CInstIcon : public tagINSTICON
{
public:
	CInstIcon()
		{
		wFlags = 0;
#ifdef _DEBUG
		// These fields must be initialized before use so let's put garbage in them
		FillMemory(&szName, MAX_DATA_LENGTH, bGarbage);
#endif //_DEBUG
		}
    BOOL        fIsUninstallFile()
                    { return (BOOL)( (UINT)wFlags & (UINT)IF_UNINSTALL ); }
    BOOL        fIsUninstallLink()
                    { return (BOOL)( (UINT)wFlags & (UINT)IF_UNINSTALLLINK ); }
    char *       GetIconName()
                    { return(&szName[0]); }
    char *       GetIconSource()
                    { return(((&szName[0])+lstrlen(&szName[0])+1)); }
    char *       GetIconDescription()
					{ return((GetIconSource()+(lstrlen(GetIconSource())+1))); }
    char *       GetIconDestination()
                    { return((GetIconDescription()+(lstrlen(GetIconDescription())+1))); }
    DWORD        GetIconIndex()
                    {
		return(
					(DWORD)*(WORD *)
					(
						GetIconDestination()
						+ (lstrlen(GetIconDestination())+1)
					)
			);
	}

	__int64		GetGroup()
	{
	   __int64 group;

	   sscanf(GetIconDestination()+lstrlen(GetIconDestination())+sizeof(char)+sizeof(WORD),
		      "%I64X",
			  &group);

	   return group;
	}

};
typedef CInstIcon FAR* LPINSTICON;


typedef struct FAR tagCDSPEED
{
    WORD        wFlags;         // combination of the IF_* flags below.
    char        szName[MAX_DATA_LENGTH];
} CDSPEED;

class CCDSpeed : public tagCDSPEED
{
public:
	CCDSpeed()
		{
		wFlags = 0;
#ifdef _DEBUG
		// These fields must be initialized before use so let's put garbage in them
		FillMemory(&szName, MAX_DATA_LENGTH, bGarbage);
#endif //_DEBUG
		}
    DWORD        GetCDSpeedMinCD()
                    { return((DWORD)*(WORD *)((BYTE *)(&szName[0])));}
    DWORD        GetCDSpeedMaxCPU()
                    { return((DWORD)*(WORD *)((BYTE *)((&szName[0])+sizeof(WORD)))); }
	char *		 GetCDSpeedFileName()
					{ return ((&szName[0])+sizeof(WORD)+sizeof(WORD)); }
};
typedef CCDSpeed FAR* LPCDSPEED;


typedef struct FAR tagCABGO
{
    char        szName[MAX_DATA_LENGTH];
} CABGO;

class CCabGo : public tagCABGO
{
public:
#ifdef _DEBUG
	CCabGo()
		{
			// These fields must be initialized before use so let's put garbage in them
			FillMemory(&szName, MAX_DATA_LENGTH, bGarbage);
		}
#endif //_DEBUG
	char *       GetCabName() { return(&szName[0]); }
};
typedef CCabGo FAR *LPCABGO;

//****************************************************************************
typedef struct FAR tagREGWIZ
{
    char        szURL[MAX_DATA_LENGTH];
}REGWIZ;

class CRegWiz : public tagREGWIZ
{
public:
#ifdef _DEBUG
	CRegWiz()
		{
		// These fields must be initialized before use so let's put garbage in them
		FillMemory(&szURL, MAX_DATA_LENGTH, bGarbage);
		}
#endif //_DEBUG
    char *       GetRegURL() { return(&szURL[0]); }
};
typedef CRegWiz FAR *LPREGWIZ;

//*** ADDINIVALUE - This struct is stored as a variable-length resource
//                   as part of a SETUPCOMMAND (below).  This struct
//                   corresponds to the AddIniValue setup command.
//
typedef struct FAR tagADDINIVALUE
{
    WORD        wSectionOffset; // offset from szData to section name (must be 1st)
    WORD        wKeyOffset;     // offset from szData to key string
    WORD        wValueOffset;   // offset from szData to value string
	WORD        wTypeOffset;    // offset from szData to type string
	WORD        wUninstallOffset;  // offset from szData to Uninstall? string
	WORD		wGroupOffset;	  //offset from szData to group string
    char        szData[MAX_DATA_LENGTH];      // filename + section + key + value + type + uninstall + group
} ADDINIVALUE;

class CAddIniValue : public tagADDINIVALUE
{
public:
	CAddIniValue()
		{
		wSectionOffset = 0;
		wKeyOffset = 0;
		wValueOffset = 0;
		wTypeOffset = 0;
		wUninstallOffset = 0;
		wGroupOffset = 0;
#ifdef _DEBUG
		// These fields must be initialized before use so let's put garbage in them
		FillMemory(&szData, MAX_DATA_LENGTH, bGarbage);
#endif //_DEBUG
		}
    LPCSTR      GetIniFileName()
                    { return (LPCSTR)szData; }
    LPCSTR      GetIniSectionName()
                    { return (LPCSTR)&szData[wSectionOffset]; }
    LPCSTR      GetIniKeyName()
                    { return (LPCSTR)&szData[wKeyOffset]; }
    LPCSTR      GetIniValue()
                    { return (LPCSTR)&szData[wValueOffset]; }
    DWORD       GetIniType()
                    { return (DWORD) *((DWORD *) &szData[wTypeOffset]); }
    BOOL        GetIniUninstall()
					{ return (BOOL) '1' == szData[wUninstallOffset] ? TRUE : FALSE; }
    BOOL        GetIniUninstallAll()
					{ return (BOOL) '2' == szData[wUninstallOffset] ? TRUE : FALSE; }

	__int64		GetGroup()
	{
	   __int64 group;
	   sscanf(&szData[wGroupOffset],"%I64X",&group);
	   return group;
	}

    BOOL        GetMapFlag()
                    { return !(BOOL)(UINT)wSectionOffset; }
};
typedef CAddIniValue FAR* LPADDINIVALUE;

//****************************************************************************
//*** INSTALLFONT - This struct is stored as a variable-length resource as part
//                  of a SETUPCOMMAND (below).  This struct corresponds to the
//                  InstallFont setup command.
//
typedef struct FAR tagINSTALLFONT
{
    FILEINFO    FileInfo;       // Date/time/version/size of the source file
    WORD        wFlags;         // flags - reserved
    WORD        wNameOffset;    // offset from szName to font name
    char        szName[MAX_DATA_LENGTH];      // source filename + font name, see below
} INSTALLFONT;


class CInstallFont : public tagINSTALLFONT
{
public:
	CInstallFont()
		{
		wFlags = 0;
		wNameOffset = 0;
#ifdef _DEBUG
		// These fields must be initialized before use so let's put garbage in them
		FillMemory(&FileInfo, sizeof(FILEINFO), bGarbage);
		FillMemory(&szName, MAX_DATA_LENGTH, bGarbage);
#endif //_DEBUG
		}

    //Returns predetermined timestamp/version/language for source file
    LPFILEINFO  GetSourceFileInfo ()
                    { return &FileInfo; }

    //Returns relative pathname of source file, relative to the
    //directory on the CD where the stub exe sits.
    LPCSTR      GetSourceFileName ()
                    { return szName; }

    //Returns full pathname of destination file
    void        MakeDestFileName (LPSTR, LPINT);
        // defined in SETUP.CPP

    //Returns the font name
    LPCSTR      GetFontName ()
                    { return &szName [wNameOffset]; }
};
typedef CInstallFont FAR* LPINSTALLFONT;




//****************************************************************************
//*** REGISTERDLL - This struct is stored as a variable-length resource as part
//                  of a SETUPCOMMAND (below).  This struct corresponds to the
//                  DLLRegister setup command.
//
typedef struct FAR tagDLLREGISTER
{
    WORD        wFlags;         // flags - reserved
    char        szDLLName[MAX_DATA_LENGTH];	// Name of DLL which is being registered.
} REGISTERDLL;


class CDLLRegister : public tagDLLREGISTER
{
public:
	CDLLRegister()
		{
		wFlags = 0;
#ifdef _DEBUG
		// These fields must be initialized before use so let's put garbage in them
		FillMemory(&szDLLName, MAX_DATA_LENGTH, bGarbage);
#endif //_DEBUG
		}

    //Returns relative pathname of dll file, relative to the
    //directory on the CD where the stub exe sits.
    LPCSTR      GetDLLName ()
                    { return szDLLName; }
};
typedef CDLLRegister FAR* LPDLLREGISTER;




//****************************************************************************
//*** SETUPCOMMAND - This is the struct that is directly stored in the
//                   resource file.  It contains info on which command
//                   it represents, as well as under which operating
//                   systems this command should be executed 
//
typedef struct FAR tagSETUPCOMMAND
{
    WORD        wCommandID;         // one of the SC_* items defined below
    WORD        wCommandResID;      // resource ID of the actual command data
    DWORD       dwBuildFlags;       // combination of the SCF_* flags below
} SETUPCOMMAND;

class CSetupCommand : public tagSETUPCOMMAND
{
public:
    WORD    GetCommandResID() { return wCommandResID; }
    HGLOBAL LoadCommandResource();
};



//****************************************************************************
//*** SETUPINFO - There is only one of these in the resource file; it
//                contains the number of commands there are and where
//                their resource ID's begin.
//
typedef struct FAR tagSETUPINFO
{
    WORD        wNumCommand;    // number of command resource ID's
    WORD        wFirstResID;    // first resource ID
} SETUPINFO, FAR* LPSETUPINFO;




//****************************************************************************
//*** RUNTIMECOMMAND - One of these gets allocated at runtime for each
//                     possible command.  Its values are filled by
//                     GetFileSizeRequirements() -- if lpSetupCommand is NULL, then
//                     the command should be skipped.  Otherwise, lpCommand
//                     points to the locked resource for the associated
//                     SETUPCOMMAND, and FileInfo gets filled with the
//                     destination file's version/language/timestamp.
//
typedef struct tagRUNTIMECOMMAND
{
    FILEINFO        FileInfo;           // dest file version, date, etc.
    LPSETUPCOMMAND  lpSetupCommand;     // command to execute, NULL for none
    HGLOBAL         hglbSetupCommand;   // resource handle for setup command
} RUNTIMECOMMAND, FAR* LPRUNTIMECOMMAND;

// System requirements struct.
UINT ShouldFileBeInstalled( LPINSTALLFILE pInstallFile, LPFILEINFO lpDestFileInfo, BOOL bFirstTime );



#endif //__SETUP_H}
