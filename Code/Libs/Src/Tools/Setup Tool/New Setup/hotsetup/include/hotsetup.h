// Hotsetup.h, a stripped down header file for the hotsetup engine, included by hotsetup
// UI's.
// History:
//  8/20/96 a-melodh    Rem RunSetup and replace w/ InstallApp() and UninstallApp()
//		03/15/97 a-dashoe		update timestamp

#ifndef HOTSETUP_H
#define HOTSETUP_H

//
//Valid ret code for EBU Engine functions and setup app callbacks
//
enum EBURETCODE {EBU_OK=10000, EBU_ABORT, EBU_BACK, EBU_RETRY, EBU_CANCEL, EBU_ERROR};

//
//Used by the DirectX call back interface
//
enum DIRECT_X_STATUS { DX_FORCE,DX_WARN,DX_UNKNOWN, DX_KEEP, DX_SAFE, DX_UNNECESSARY };
enum DIRECT_X_VERSION {EV_ERROR, EV_NOT_INSTALLED, EV_EXISTING_SAME, EV_EXISTING_NEWER, EV_EXISTING_OLDER};
enum DXSTATUS {QUERYDPLAYINSTALL, QUERYDIRECTXINSTALL, DETECTDIRECTX};

//
//Used by the EBUShellExecute callback...
//
enum SHELL_EXECUTE_STATUS {SES_BEFORE, SES_LAUNCHED, SES_FINISHED};
#define EBUENGINE_SHELLEXECUTE 0xFAFBFCFD
#define EBUSCRIPT_SHELLEXECUTE 0xAFABACAD

#include "hardcodedstrings.h"
#include "script.h"
#include "util.h"
#include "myassert.h"

#include <tchar.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <mmsystem.h>

class CGlobals;
class CSetupCommand;


#define GROUPLISTSIZE	64 // number of groups possible (currently 63, + 1 empty, a 64bit value)

typedef struct
{
	DWORD dwMsgType;
	char szName[512];
	DWORD dwFlags;
	WORD UserResponse;
	enum DIRECT_X_STATUS eStatus;
	BOOL bActive;
} DRIVERUPDATE, * LPDRIVERUPDATE;

typedef struct
{
	int arraySize;
	int numUpdates;
	DRIVERUPDATE **UpdateArray;
	DWORD	dwError;
	char szErrMsg[1024];
} UPDATEARRAY, * LPUPDATEARRAY;

typedef struct tagInstallList {
	int nodenum;
	char *szSource;
	char *szDest;
	DWORD dwFileSize;
	UINT  wFlags;
	int	nDiskID;
	struct tagInstallList *nextElement;
}INSTALLLIST, *LPINSTALLLIST;

typedef int (WINAPI * ALERTWINPROC)(HWND,LPSTR,LPSTR,UINT);

typedef struct SharedDLL {
	char szFileName[_MAX_PATH];
	struct SharedDLL *nextDLL;
} SHAREDDLL, *LPSHAREDDLL;

typedef CSetupCommand FAR* LPSETUPCOMMAND;

extern "C"
{
	typedef EBURETCODE (WINAPI *VALIDATEPROC)(PTSTR);
	typedef EBURETCODE (WINAPI *EBUCALLBACK) (void *);

// xxGlob	extern EBUCALLBACK  g_lpfnAppCallback;
}

//
//Used in CheckHardware call in checkhw.cpp
//
typedef struct requirementsTag
{
	int   cColors;				// number of colors
	int   cBitsPixel;				// bits per pixel
	DWORD dwProcessorType;		// processor
	DWORD dwTotalPhys;			// total memory (in MB)
	DWORD dwRequiredPhys;
	WORD  wCDRom;
	DWORD dwResolution;		//HIWORD() = x res, LOWORD() = y res
	DWORD dwNTBuildNumberRequired;     //minimum build of NT required if running on NT
	BYTE  bNTCSDVersionRequired;	   //minimum CSD level required if running on NT
	WORD  wWin40BuildNumberRequired;  //minimum build of Win40 (Win 95) required if on Win40
} REQUIREMENTS, * LPREQUIREMENTS;

extern class     Printer *g_pPrint;

//
//Common attributes and functionality for all app callback structs...
//
struct CallBackStruct
{
	int nID; 
	BYTE fUninstall;

	CallBackStruct()
	{
		fUninstall = FALSE;
		nID = SC_ERROR;
	};
};

typedef struct CallBackStruct CALLBACKDATA;
typedef struct CallBackStruct *PCALLBACKDATA;

typedef struct shellexecutestruct : public CallBackStruct
{
	LPSTR	  pszPathName;
	BOOL	  fWait;
	HINSTANCE hInstApp;
	HANDLE    hProcess;
	DWORD     dwRc;
	UINT	  uiTag;

	SHELL_EXECUTE_STATUS nStatus;

	shellexecutestruct()
	{
		nID = SC_SHELLEXECUTE;
		pszPathName = NULL;
		fWait = FALSE;
		hInstApp = 0;
		hProcess = NULL;
		dwRc = STILL_ACTIVE;
		uiTag = -1;
	};
}
SHELLEXECUTEDATA, *LPSHELLEXECUTEDATA;

typedef struct getnamestruct : public CallBackStruct
{
	LPSTR pszPlayerName;

	getnamestruct()
	{
		nID = SC_GETNAME;
		pszPlayerName = NULL;
	};
}
GETNAMEDATA, *LPGETNAMEDATA;

//typedef struct installfilestruct : public CallBackStruct
//{
//	LPSTR pszFileName;
//	WORD  wFlags;
//
//	installfilestruct()
//	{
//		nID = SC_INSTALLFILE;
//		pszFileName = NULL;
//	};
//}
//INSTALLFILEDATA, *LPINSTALLFILEDATA;

typedef struct installliststruct : public CallBackStruct
{
	LPSTR pszFileName;
	WORD  wFlags;

	installliststruct()
	{
		nID = SC_INSTALLLIST;
		pszFileName = NULL;
	};
}
INSTALLLISTDATA, *LPINSTALLLISTDATA;

typedef struct regwizstruct : public CallBackStruct
{
	char *pszURL;

	regwizstruct()
	{
		nID = SC_REGWIZ;
		pszURL = NULL;
	};
}
REGWIZDATA, *LPREGWIZDATA;

typedef struct deletefilestruct : public CallBackStruct
{
	char *pszPathName;
	BOOL fPromptToDelete;

	deletefilestruct()
	{
		nID = SC_DELETEFILE;
		pszPathName = NULL;
		fPromptToDelete = FALSE;
	};
}
DELETEFILEDATA, *LPDELETEFILEDATA;

typedef struct addinivaluestruct : public CallBackStruct
{
	LPCSTR pszFileName;
	LPCSTR pszSectionName;
	LPCSTR pszKeyName;
	LPCSTR pszValue;

	addinivaluestruct()
	{
		nID = SC_ADDINIVALUE;
		pszFileName = NULL;
		pszSectionName = NULL;
		pszKeyName = NULL;
		pszValue = NULL;
	};
}
ADDINIVALUEDATA, *LPADDINIVALUEDATA;

typedef struct mkdirtag : public CallBackStruct
{
	TCHAR *pszDirName;

	mkdirtag()
	{
		nID = SC_MKDIR;
		pszDirName = NULL;
	};
} MKDIRDATA, *LPMKDIRDATA;

typedef struct dxinsttag : public CallBackStruct
{
	DIRECT_X_VERSION uExistingVersion;
	UPDATEARRAY *gUpdates;
	DXSTATUS nStatus;

	dxinsttag()
	{
		nID = SC_INSTDX;
		gUpdates = NULL;
	};
} DIRECTXDATA, *LPDIRECTXDATA;

typedef struct dplayinsttag : public CallBackStruct
{
	DIRECT_X_VERSION uExistingVersion;
	DXSTATUS nStatus;
	DIRECTXREGISTERAPP	*lpDXRegApp;

	dplayinsttag()
	{
		nID = SC_INSTDPLAY;
	};
} DIRECTPLAYDATA, *LPDIRECTPLAYDATA;

typedef struct mkroottag : public CallBackStruct
{
	VALIDATEPROC lpfnValidateEntry;
	char UserRootEntry[_MAX_PATH];
	char szAppDir[_MAX_PATH];

	mkroottag()
	{
		nID = SC_MKROOT;
		lpfnValidateEntry = NULL;
		UserRootEntry[0] = '\0';
		szAppDir[0] = '\0';
	};
} MKROOTDATA, *LPMKROOTDATA;

typedef struct getpidtag : public CallBackStruct
{
	VALIDATEPROC lpfnValidateEntry;
	char SiteCode[3];
	char ProductID[7];
	char SerialNumber[5];
	BOOL fOEMPid;
	char *pszPID;

	getpidtag()
	{
		nID = SC_GETPID;
		lpfnValidateEntry = NULL;
		SiteCode[0] = '\0';
		ProductID[0] = '\0';
		SerialNumber[0] = '\0'; 
		fOEMPid = FALSE;
		pszPID = NULL;
	};
} GETPIDDATA, *LPGETPIDDATA;

enum CDSTATUSVALUE {CDASK, CDDISPLAYSTATUS, CDCDROMFAIL, CDCPUFAIL, CDOK};

typedef struct getcdspeedtag : public CallBackStruct
{
    HWND       hWnd;
	EBURETCODE nAbortCode;
	DWORD      avg_speed;
	double     mincpu;
	int		   nCDErrCount;
	enum CDSTATUSVALUE cdstatus;

	getcdspeedtag()
	{
		nID = SC_CDSPEED;
		cdstatus = CDASK;
		hWnd = NULL;
		avg_speed = 0;
		mincpu = 0;
		nCDErrCount = 0;
	}

} CDSPEEDDATA, *LPCDSPEEDDATA;

typedef struct filecopystatustag : public CallBackStruct
{
	DWORD dwTotalSize;
	DWORD dwLastFile;
	DWORD dwTotalCopied;
	char  szSource[_MAX_PATH];
	char  szDest[_MAX_PATH];
	BOOL  fDone;
	BOOL  fCancelled;

	filecopystatustag()
	{
		nID = SC_INSTALLGO;
		dwTotalSize = 0;
		dwLastFile = 0;
		dwTotalCopied = 0;
		szSource[0] = '\0';
		szDest[0] = '\0';
		fDone = FALSE;
		fCancelled = FALSE;
	};

} FILECOPYSTATUS, *LPFILECOPYSTATUS;

typedef struct getgroupdatatag : public CallBackStruct
{
	__int64 group;

	DWORD dwGameFreeSpace;
	DWORD dwGameNeeded;
	DWORD dwSystemFreeSpace;
	DWORD dwSystemNeeded;
//	DWORD *lpdwGroupSizes;

	getgroupdatatag()
	{
		nID = SC_GETGROUP;
		group = 0;
		dwGameFreeSpace = 0;
		dwGameNeeded = 0;
		dwSystemFreeSpace = 0;
		dwSystemNeeded = 0;
//		lpdwGroupSizes = NULL;
	};
} GETGROUPDATA, *LPGETGROUPDATA;

#define ICON_MENU 1
#define ICON_DESKTOP 2

typedef struct insticondatatag : public CallBackStruct
{
		int  icontype;
		char *szIconDesc;
		char *szIconSource;
		char *szExeName;
		char *WorkDir;
		char *szArgs;
		DWORD index;

		insticondatatag()
		{
			nID = SC_INSTICON;
			icontype = 0;
			szIconDesc = NULL;
			szIconSource = NULL;
			szExeName = NULL;
			WorkDir = NULL;
			szArgs = NULL;
			index = 0;
		};
} INSTICONDATA, *LPINSTICONDATA;

#define INTEL386		PROCESSOR_INTEL_386
#define INTEL486        PROCESSOR_INTEL_486
#define INTELPENTIUM	PROCESSOR_INTEL_PENTIUM

EBURETCODE InstallApp(HWND hWndParent, BOOL fFirstTime,BOOL bTrial);
EBURETCODE MaintainApp(HWND hWndParent, BOOL bTrial, BOOL fForceReinstall = FALSE);
EBURETCODE UninstallApp(HWND hWndParent, BOOL bTrial);

EBURETCODE InitEBUSetup(LPSTR lpszCmdLine, EBUCALLBACK AppCallbackProc, ALERTWINPROC AlertWinProc, HWND hWndParent, BOOL fDontBootstrap,BOOL fZone);
BOOL HasAppEverBeenLaunched(BOOL);
BOOL LaunchApplication(UINT ui_ExeStringID, UINT ui_ParamStringID);
BOOL RestartWindows(void);
BOOL CheckHardware(BOOL fFirstTime, LPREQUIREMENTS req);
VOID WINAPI DeleteMyself(BOOL fAboutToReboot); 
BOOL IsBrowserInstalled(void);
int EBUStrlen(char *);

EBURETCODE EBUShellExecute(HWND  hWndParent,
						   TCHAR *pszExecuteThis,
						   TCHAR *pszParameters,
						   TCHAR *pszDirectory,
						   int   nShow,
						   UINT	 uiTag,
						   UINT  uiErrorResID,
						   BOOL  fWait,
						   LPSHELLEXECUTEDATA psed);


namespace NGLOBALS
{

// FORMER global variables			
extern ALERTWINPROC		n_fnAlert;			
extern BOOL				n_bBinaryResource;	
extern BOOL				n_bForceFreeSpace;	
extern BOOL				n_fForceReinstall;
extern BOOL				n_bIgnoreFileInfo;	
extern BOOL				n_bInAutoRun;		
extern DWORD			n_dwBuild;			
extern DWORD			n_dwCommandFlags;
extern DWORD			n_dwExtraAppBytes;	
extern DWORD			n_dwExtraSystemBytes;
//DWORD					n_dwGroupSizes[GROUPLISTSIZE];	
extern BOOL				n_bDeleteSetup;		
extern BOOL				n_bDirCreated;		
extern BOOL				n_bGetGroup;		
extern BOOL				n_bJoystick;		
extern BOOL				n_bCopyIncomplete;	
extern BOOL				n_bPromptDelete;	
extern BOOL				n_bBootstrapped;
extern BOOL				n_bRemovingApp;		
extern BOOL				n_bRanUninstall;
extern BOOL				n_fTrial;			
extern BOOL				n_fWin95NotOSR2;	
extern BOOL				n_bReboot;			
extern LPSHAREDDLL		n_SharedDLL;		
extern HINSTANCE		n_hApplicationInst;	
extern HINSTANCE		n_hResourceInst;	
extern HINSTANCE		n_hScriptInst;		
extern HWND				n_hWndParent;		
extern WORD				n_wOS;				
extern EBURETCODE		n_EBUResultCode;	
extern int				n_nDirectXReturnCode;
extern int				n_nDirectPlayReturnCode;
extern BOOL				n_fForceDX;
extern BOOL				n_fNoDX;
extern EBUCALLBACK		n_lpfnAppCallback;		
extern VALIDATEPROC		n_lpfnValidateEntry;	
extern TCHAR			n_szAppDir[MAX_PATH];		
extern TCHAR			n_szAppTitle[128];			
extern TCHAR			n_szPatchPath[MAX_PATH];		
extern TCHAR			n_szResDLLPath[MAX_PATH];
extern TCHAR			n_szDiscIDPath[MAX_PATH];   // Used to verify old CDs
extern TCHAR			n_szSourcePath[MAX_PATH];	//Initialized by the command line.
extern TCHAR			n_szSetupExeName[MAX_PATH];	
extern TCHAR			n_szPid[MAX_PATH];				
extern TCHAR			n_szPlayerName[MAX_PATH];		
extern TCHAR			n_szProgram[MAX_PATH];			
extern TCHAR			n_szRegBase[MAX_PATH];			
extern TCHAR			n_szRegClassesMSGamesRoot[MAX_PATH]; 
extern TCHAR			n_szRegSharedDLLs[MAX_PATH];	
extern TCHAR			n_szRegUninstall[MAX_PATH];	
extern TCHAR			n_szSetupTitle[128];		
extern TCHAR			n_szUserLanguage[128];
extern TCHAR			n_szUserPath[MAX_PATH];	
extern HGDIOBJ			n_hOldFont;			// hOldFont
extern HGDIOBJ			n_hPrinterFont;		// hRomanFont
extern LPINSTALLLIST	n_ListEnd;			// ListEnd
extern LPINSTALLLIST	n_ListHead;			// ListHead
extern WORD				n_wDrtyBits;
extern BOOL				n_fWriteUninstall;
extern BOOL				n_bUseMCISound;				// use MCI for sound
extern BOOL				n_bNoSound;				// run silent

// new global vars as of 27.11.98 below here
extern int				n_nMaxDirLen;			// maximum directory length
extern int				n_nFilesInUninstall;
extern int				n_nFilesToDelete;
extern _int64			n_OldGroupList;			//Only used in maintenance mode to remember previous bits
extern BOOL				n_fAppDirExists;		// TRUE if AppDir exists, otherwise FALSE
extern DWORD			n_dwSystemBytesPerCluster;
extern DWORD			n_dwGameBytesPerCluster;
extern BOOL				n_fMaintMode;			// maintainence mode flag
extern __int64			n_GroupList;


// declaration of accessor functions
extern 	void	SetAlertFn(ALERTWINPROC fnAlert);
extern 	ALERTWINPROC GetAlertFn();			

extern 	void	SetBinaryResource(BOOL b);		
extern 	BOOL	GetBinaryResource();		

extern 	void	SetForceFreeSpace(BOOL b);
extern 	BOOL	GetForceFreeSpace();		

extern 	void	SetForceReinstall(BOOL b);
extern	BOOL	GetForceReinstall();	

extern 		void	SetIgnoreFileInfo(BOOL b);
extern 		BOOL	GetIgnoreFileInfo();			

extern 		void	SetInAutoRun(BOOL b);
extern 		BOOL	GetInAutoRun();

extern 		void    SetCommandFlags(DWORD dw);
extern 		DWORD   GetCommandFlags();	

extern 		void	SetBuild(DWORD dwBuild);
extern		DWORD	GetBuild();

extern 		void	SetExtraAppBytes(DWORD dwBytes);
extern 		DWORD	GetExtraAppBytes();

extern 		void	SetExtraSystemBytes(DWORD dwBytes);
extern 		DWORD	GetExtraSystemBytes();				

//	void	SetGroupSizes(UINT nGroup, DWORD dwSizes)	
//				{	assert((0<= nGroup) && (GROUPLISTSIZE >= nGroup));
//					m_dwGroupSizes[nGroup] = dwSizes;
//				};
//	DWORD	GetGroupSizes(UINT nGroup)	 
//				{	assert((0<= nGroup) && (GROUPLISTSIZE >= nGroup));
//					return m_dwGroupSizes[nGroup];
//				};
//	LPDWORD	GetLPDWGroupSizes(UINT nGroup)
//				{	assert((0<= nGroup) && (GROUPLISTSIZE >= nGroup));
//					return &m_dwGroupSizes[nGroup];	
//				};

extern 		void	SetDeleteSetup(BOOL b);
extern 		BOOL	GetDeleteSetup();		

extern 		void	SetDirCreated(BOOL b);
extern 		BOOL	GetDirCreated();			

extern 		void	SetIsThereAGetGroup(BOOL b);
extern 		BOOL	GetIsThereAGetGroup();				

extern 		void	SetJoystick(BOOL b);	
extern		BOOL	GetJoystick();		

extern 		void	SetPromptDelete(BOOL b);
extern 		BOOL	GetPromptDelete();	

extern 		void	SetBootstrapFlag(BOOL b);
extern 		BOOL	GetBootstrapFlag();		

extern 		void	SetRemovingApp(BOOL b);
extern 		BOOL	GetRemovingApp();	

extern 		void	SetRanUninstall(BOOL b);
extern		BOOL	GetRanUninstall();			

extern 		void	SetTrial(BOOL b);
extern 		BOOL	GetTrial();	

extern 		void	SetWin95NotOSR2(BOOL b);
extern 		BOOL	GetWin95NotOSR2();

extern 		void	SetAppInst(HINSTANCE hInst);
extern 		HINSTANCE	GetAppInst();

extern 		HINSTANCE	SetResourceInst(HINSTANCE hInst);
extern		HINSTANCE	GetResourceInst();		

extern 		void	SetScriptInst(HINSTANCE hInst);
extern 		HINSTANCE	GetScriptInst();			

extern 		void	SetWndParent(HWND hWnd);
extern 		HWND		GetWndParent();			

extern 		void	SetAppCallback(EBUCALLBACK lpfnEbuCB);
extern 		EBUCALLBACK		GetAppCallback();

extern 		void	SetValidateEntry(VALIDATEPROC lpfnEntry);
extern		VALIDATEPROC		GetValidateEntry();

extern 		void	SetDXReturnCode(int nCode);			
extern 		int				GetDXReturnCode();		

extern 		void	SetDPLAYReturnCode(int nCode);		
extern 		int				GetDPLAYReturnCode();

extern 		void    SetForceDXFlag(BOOL b);
extern 		BOOL	GetForceDXFlag();					

extern 		void    SetNoDXFlag(BOOL b);
extern		BOOL	GetNoDXFlag();	

extern 		void	SetSharedDLL(LPSHAREDDLL  lpDLL);
extern 		LPSHAREDDLL		GetSharedDLL();			

extern 		void	SetAppDir(LPTSTR	szStr);	
extern 		LPTSTR	GetAppDir();
					
extern 		LPTCH	GetLpChFromAppDir(UINT nIndex);
extern 		TCHAR	GetChFromAppDir(UINT	nIndex);

extern 		void	SetUserLanguage(LPTSTR szStr);
extern		LPTSTR	GetUserLanguage();	

extern 		void    SetResDLLPath(LPTSTR szStr);	
extern 		LPTSTR	GetResDLLPath();	

extern 		void	SetDiscIDPath(LPTSTR szStr);
extern 		LPTSTR	GetDiscIDPath();		

extern 		void	SetAppTitle(LPTSTR	szStr);
extern 		LPTSTR			GetAppTitle();	

extern 		void	SetPatchPath(LPTSTR  szStr);
extern		LPTSTR			GetPatchPath();	

extern 		void	SetSourcePath(LPTSTR  szStr);
extern 		LPTSTR			GetSourcePath();	

extern 		void	SetSetupExeName(LPTSTR  szStr);
extern 		LPTSTR			GetSetupExeName();

extern 		void	SetPid(LPTSTR  szStr);	
extern 		LPTSTR			GetPid();			

extern 		void	SetPlayerName(LPTSTR szStr);
extern		LPTSTR			GetPlayerName();

extern 		void	SetProgram(LPTSTR szStr);		
extern 		LPTSTR			GetProgram();

extern 		void	SetRegBase(LPTSTR szStr);
extern 		LPTSTR			GetRegBase();	

extern 		void	SetRCGameRoot(LPTSTR szStr);
extern 		LPTSTR			GetRCGameRoot();	

extern 		void	SetRegSharedDLLs(LPTSTR szStr);
extern		LPTSTR			GetRegSharedDLLs();	

extern 		void	SetRegUninstall(LPTSTR szStr);
extern 		LPTSTR			GetRegUninstall();	

extern 		void	SetSetupTitle(LPTSTR szStr);
extern 		LPTSTR			GetSetupTitle();		

extern 		void	SetUserPath(LPTSTR szStr);
extern 		LPTSTR			GetUserPath();

extern 		void	SetOS(WORD wOS);				
extern		WORD			GetOS();		

extern 		void	SetOldFont(HGDIOBJ hObj);
extern 		HGDIOBJ			GetOldFont();	

extern 		void	SetPrinterFont(HGDIOBJ hObj);
extern 		HGDIOBJ			GetPrinterFont();	

extern 		void	SetListEnd(LPINSTALLLIST lpList);
extern 		LPINSTALLLIST	GetListEnd();	

extern 		void	SetListHead(LPINSTALLLIST lpList);
extern		LPINSTALLLIST	GetListHead();

extern 		void	SetReboot(BOOL b);				
extern		BOOL			GetReboot();	

extern 		void	SetResultCode(EBURETCODE  retCode);
extern		EBURETCODE		GetResultCode();		

extern 		void	SetDirtyBits(WORD wDirtyBits);
extern		void	ClearDirtyBits();
extern		WORD	GetDirtyBits();			

extern 		void	SetCopyIncomplete(BOOL b);
extern		BOOL	GetCopyIncomplete();			

extern 		void	SetWriteUninstall(BOOL b);
extern		BOOL	GetWriteUninstall();					

extern 		void	SetMCISound(BOOL b);
extern		BOOL	GetMCISound();						

extern 		void	SetNoSound(BOOL b);
extern		BOOL	GetNoSound();					

extern 		void	SetMaxDirLen(int len);
extern		int		GetMaxDirLen();	

extern 		void	SetOldGroupList(_int64 nGroup);		
extern		_int64	GetOldGroupList();					

extern 		void	SetAppDirExists(BOOL fAppDir);		
extern		BOOL	GetAppDirExists();					

}

// Globals we didn't feel should go into the global class
// We didn't want to create a bunch of extra handlers to return and set each field
// of the data struct.
//UPDATEARRAY		gUpdates;  declared in dxinst.cpp to avoid multiple definitions

extern BOOL EnsureCDROMInserted(LPINSTALLLIST  traverse=NULL);

#endif
