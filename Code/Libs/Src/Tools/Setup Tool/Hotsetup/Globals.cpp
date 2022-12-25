#include "stubpch.h"
#include "hotsetup.h"

/*
CGlobals::CGlobals()
{
	ZeroMemory(this, sizeof(CGlobals));
	n_EBUResultCode		= EBU_ERROR;	
	n_fWriteUninstall	= TRUE;
	n_GroupList			= 0;
	n_fMaintMode		= FALSE;
}

CGlobals::~CGlobals()
{
}
*/

namespace NGLOBALS
{
	ALERTWINPROC	n_fnAlert;	
	BOOL			n_bBinaryResource;
	BOOL			n_bForceFreeSpace;
	BOOL			n_fForceReinstall;
	BOOL			n_bIgnoreFileInfo;
	BOOL			n_bInAutoRun;
	DWORD			n_dwBuild;			
	DWORD			n_dwCommandFlags;
	DWORD			n_dwExtraAppBytes;
	DWORD			n_dwExtraSystemBytes;
	BOOL			n_bDeleteSetup;
	BOOL			n_bDirCreated;
	BOOL			n_bGetGroup;
	BOOL			n_bJoystick;
	BOOL			n_bCopyIncomplete;	
	BOOL			n_bPromptDelete;
	BOOL			n_bBootstrapped;
	BOOL			n_bRemovingApp;
	BOOL			n_bRanUninstall;
	BOOL			n_fTrial;
	BOOL			n_fWin95NotOSR2;
	BOOL			n_bReboot;			
	LPSHAREDDLL		n_SharedDLL;		
	HINSTANCE		n_hApplicationInst;	
	HINSTANCE		n_hResourceInst;	
	HINSTANCE		n_hScriptInst;		
	HWND			n_hWndParent;		
	WORD			n_wOS;				
	EBURETCODE		n_EBUResultCode	= EBU_ERROR;	
	int				n_nDirectXReturnCode;
	int				n_nDirectPlayReturnCode;
	BOOL			n_fForceDX;
	BOOL			n_fNoDX;
	EBUCALLBACK		n_lpfnAppCallback;	
	VALIDATEPROC	n_lpfnValidateEntry;
	TCHAR			n_szAppDir[MAX_PATH];
	TCHAR			n_szAppTitle[128];
	TCHAR			n_szPatchPath[MAX_PATH];
	TCHAR			n_szResDLLPath[MAX_PATH];
	TCHAR			n_szDiscIDPath[MAX_PATH];   // Used to verify old CDs
	TCHAR			n_szSourcePath[MAX_PATH];	//Initialized by the command line.
	TCHAR			n_szSetupExeName[MAX_PATH];	
	TCHAR			n_szPid[MAX_PATH];
	TCHAR			n_szPlayerName[MAX_PATH];
	TCHAR			n_szProgram[MAX_PATH];
	TCHAR			n_szRegBase[MAX_PATH];
	TCHAR			n_szRegClassesMSGamesRoot[MAX_PATH];
	TCHAR			n_szRegSharedDLLs[MAX_PATH];
	TCHAR			n_szRegUninstall[MAX_PATH];
	TCHAR			n_szSetupTitle[128];
	TCHAR			n_szUserLanguage[128];
	TCHAR			n_szUserPath[MAX_PATH];
	HGDIOBJ			n_hOldFont;					// hOldFont
	HGDIOBJ			n_hPrinterFont;				// hRomanFont
	LPINSTALLLIST	n_ListEnd;					// ListEnd
	LPINSTALLLIST	n_ListHead;					// ListHead
	WORD			n_wDirtyBits;
	BOOL			n_fWriteUninstall	= TRUE;
	BOOL			n_bUseMCISound;				// use MCI for sound
	BOOL			n_bNoSound;					// run silent

	int				n_nMaxDirLen;				// maximum directory length
	int				n_nFilesInUninstall;
	int				n_nFilesToDelete;
	_int64			n_OldGroupList;				// Only used in maintenance mode to remember previous bits
	BOOL			n_fAppDirExists;			// TRUE if AppDir exists, otherwise FALSE

	DWORD			n_dwSystemBytesPerCluster;
	DWORD			n_dwGameBytesPerCluster;
	BOOL			n_fMaintMode	= FALSE;	// maintainence mode flag
	__int64			n_GroupList		= 0;

	//accessor functions
	void	SetAlertFn(ALERTWINPROC fnAlert){ n_fnAlert = fnAlert;};
	ALERTWINPROC GetAlertFn()				{ return n_fnAlert;};

	void	SetBinaryResource(BOOL b)		{ n_bBinaryResource = b;};
	BOOL	GetBinaryResource()				{ return n_bBinaryResource;};

	void	SetForceFreeSpace(BOOL b)		{ n_bForceFreeSpace = b;};
	BOOL	GetForceFreeSpace()				{ return n_bForceFreeSpace;};

	void	SetForceReinstall(BOOL b)		{ n_fForceReinstall = b;};
	BOOL	GetForceReinstall()				{ return n_fForceReinstall;};

	void	SetIgnoreFileInfo(BOOL b)		{ n_bIgnoreFileInfo = b;};
	BOOL	GetIgnoreFileInfo()				{ return n_bIgnoreFileInfo;};

	void	SetInAutoRun(BOOL b)			{ n_bInAutoRun = b;};
	BOOL	GetInAutoRun()					{ return n_bInAutoRun;};

	void    SetCommandFlags(DWORD dw)		{ n_dwCommandFlags = dw;};
	DWORD   GetCommandFlags()				{ return n_dwCommandFlags;};

	void	SetBuild(DWORD dwBuild)			{ n_dwBuild = dwBuild;};
	DWORD	GetBuild()						{ return n_dwBuild;};

	void	SetExtraAppBytes(DWORD dwBytes)	{ n_dwExtraAppBytes = dwBytes;};
	DWORD	GetExtraAppBytes()				{ return n_dwExtraAppBytes;};

	void	SetExtraSystemBytes(DWORD dwBytes)	{ n_dwExtraSystemBytes = dwBytes;};
	DWORD	GetExtraSystemBytes()				{ return n_dwExtraSystemBytes;};

	void	SetDeleteSetup(BOOL b)		{ n_bDeleteSetup = b;};
	BOOL	GetDeleteSetup()			{ return n_bDeleteSetup;};

	void	SetDirCreated(BOOL b)	    { n_bDirCreated = b;};
	BOOL	GetDirCreated()				{ return n_bDirCreated;};

	void	SetGroup(BOOL b)			{ n_bGetGroup = b;};
	BOOL	GetGroup()					{ return n_bGetGroup;};

	void	SetJoystick(BOOL b)			{ n_bJoystick = b;};
	BOOL	GetJoystick()				{ return n_bJoystick;};

	void	SetPromptDelete(BOOL b)		{ n_bPromptDelete = b;};
	BOOL	GetPromptDelete()			{ return n_bPromptDelete;};

	void	SetBootstrapFlag(BOOL b)	{ n_bBootstrapped = b;};
	BOOL	GetBootstrapFlag()			{ return n_bBootstrapped;};

	void	SetRemovingApp(BOOL b)		{ n_bRemovingApp = b;};
	BOOL	GetRemovingApp()			{ return n_bRemovingApp;};

	void	SetRanUninstall(BOOL b)		{ n_bRanUninstall = b;};
	BOOL	GetRanUninstall()			{ return n_bRanUninstall;};

	void	SetTrial(BOOL b)			{ n_fTrial = b;};
	BOOL	GetTrial()					{ return n_fTrial;};

	void	SetWin95NotOSR2(BOOL b)		{ n_fWin95NotOSR2 = b;};
	BOOL	GetWin95NotOSR2()			{ return n_fWin95NotOSR2;};

	void	SetAppInst(HINSTANCE hInst)	{ n_hApplicationInst = hInst;};
	HINSTANCE	GetAppInst()			{ return n_hApplicationInst;};

	HINSTANCE	SetResourceInst(HINSTANCE hInst)	
			{ 
				n_hResourceInst = hInst;
				return n_hResourceInst;
			};
	HINSTANCE	GetResourceInst()				{ return n_hResourceInst;};

	void	SetScriptInst(HINSTANCE hInst)		{ n_hScriptInst = hInst;};
	HINSTANCE	GetScriptInst()					{ return n_hScriptInst;};

	void	SetWndParent(HWND hWnd)				{ n_hWndParent = hWnd;};
	HWND		GetWndParent()					{ return n_hWndParent;};

	void	SetAppCallback(EBUCALLBACK lpfnEbuCB)	{ n_lpfnAppCallback = lpfnEbuCB;};
	EBUCALLBACK		GetAppCallback()				{ return n_lpfnAppCallback;};

	void	SetValidateEntry(VALIDATEPROC lpfnEntry)	{ n_lpfnValidateEntry = lpfnEntry;};
	VALIDATEPROC		GetValidateEntry()				{ return n_lpfnValidateEntry;};

	void	SetDXReturnCode(int nCode)			{ n_nDirectXReturnCode = nCode;};
	int				GetDXReturnCode()			{ return n_nDirectXReturnCode;};

	void	SetDPLAYReturnCode(int nCode)			{ n_nDirectPlayReturnCode = nCode;};
	int				GetDPLAYReturnCode()			{ return n_nDirectPlayReturnCode;};

	void    SetForceDXFlag(BOOL b)				{ n_fForceDX = b; };
	BOOL	GetForceDXFlag()					{ return n_fForceDX; };

	void    SetNoDXFlag(BOOL b)					{ n_fNoDX = b; };
	BOOL	GetNoDXFlag()						{ return n_fNoDX; };

	void	SetSharedDLL(LPSHAREDDLL  lpDLL)	{ n_SharedDLL = lpDLL;};
	LPSHAREDDLL		GetSharedDLL()				{ return n_SharedDLL;};

	void	SetAppDir(LPTSTR	szStr)		
	{ 
		assert(sizeof(n_szAppDir) >= sizeof(szStr)); 
		lstrcpy( n_szAppDir, szStr);
	};
	LPTSTR	GetAppDir()						{ return (LPTSTR)n_szAppDir;};

	LPTCH	GetLpChFromAppDir(UINT nIndex)	{ return (&n_szAppDir[nIndex]);};
	TCHAR	GetChFromAppDir(UINT	nIndex)	{ return n_szAppDir[nIndex];};

	void	SetUserLanguage(LPTSTR szStr)	
	{ 
		assert(sizeof(n_szUserLanguage) >= sizeof(szStr));
		lstrcpy( n_szUserLanguage, szStr);
	};
	LPTSTR	GetUserLanguage()				{ return n_szUserLanguage;};

	void    SetResDLLPath(LPTSTR szStr)		
	{ 
		assert(sizeof(n_szResDLLPath) >= (strlen(szStr) * sizeof(TCHAR)));
		lstrcpy( n_szResDLLPath, szStr);
	};
	LPTSTR	GetResDLLPath()					{ return n_szResDLLPath;};

	void	SetDiscIDPath(LPTSTR szStr)		
	{ 
		assert(sizeof(n_szDiscIDPath) >= (strlen(szStr) * sizeof(TCHAR)));
		lstrcpy( n_szDiscIDPath, szStr);
	};
	LPTSTR	GetDiscIDPath()					{ return n_szDiscIDPath;};

	void	SetAppTitle(LPTSTR	szStr)		
	{ 
		assert(sizeof(n_szAppTitle) >= (strlen(szStr) * sizeof(TCHAR)));
		lstrcpy( n_szAppTitle, szStr);
	};
	LPTSTR			GetAppTitle()			{ return n_szAppTitle;};

	void	SetPatchPath(LPTSTR  szStr)		
	{ 
		assert(sizeof(n_szPatchPath) >= (strlen(szStr) * sizeof(TCHAR)));
		lstrcpy( n_szPatchPath, szStr);
	};
	LPTSTR			GetPatchPath()			{ return n_szPatchPath;};

	void	SetSourcePath(LPTSTR  szStr)	
	{ 
		assert(sizeof(n_szSourcePath) >= (strlen(szStr) * sizeof(TCHAR)));
		lstrcpy( n_szSourcePath, szStr);
	};
	LPTSTR			GetSourcePath()			{ return n_szSourcePath;};

	void	SetSetupExeName(LPTSTR  szStr)	
	{ 
		assert(sizeof(n_szSetupExeName) >= (strlen(szStr) * sizeof(TCHAR)));
		lstrcpy( n_szSetupExeName, szStr);
	};
	LPTSTR			GetSetupExeName()		{ return n_szSetupExeName;};

	void	SetPid(LPTSTR  szStr)			
	{ 
		assert(sizeof(n_szPid) >= (strlen(szStr) * sizeof(TCHAR)));
		lstrcpy( n_szPid, szStr);
	};
	LPTSTR			GetPid()				{ return n_szPid;};

	void	SetPlayerName(LPTSTR szStr)		
	{ 
		assert(sizeof(n_szPlayerName) >= (strlen(szStr) * sizeof(TCHAR)));
		lstrcpy( n_szPlayerName, szStr);
	};
	LPTSTR			GetPlayerName()			{ return n_szPlayerName;};

	void	SetProgram(LPTSTR szStr)		
	{ 
		assert(sizeof(n_szProgram) >= (strlen(szStr) * sizeof(TCHAR)));
		lstrcpy( n_szProgram, szStr);
	};
	LPTSTR			GetProgram()			{ return n_szProgram;};

	void	SetRegBase(LPTSTR szStr)		
	{ 
		assert(sizeof(n_szRegBase) >= (strlen(szStr) * sizeof(TCHAR)));
		lstrcpy( n_szRegBase, szStr);
	};
	LPTSTR			GetRegBase()			{ return n_szRegBase;};

	void	SetRCGameRoot(LPTSTR szStr)		
	{ 
		assert(sizeof(n_szRegClassesMSGamesRoot) >= (strlen(szStr) * sizeof(TCHAR)));
		lstrcpy( n_szRegClassesMSGamesRoot, szStr);
	};
	LPTSTR			GetRCGameRoot()			{ return n_szRegClassesMSGamesRoot;};

	void	SetRegSharedDLLs(LPTSTR szStr)	
	{ 
		assert(sizeof(n_szRegSharedDLLs) >= (strlen(szStr) * sizeof(TCHAR)));
		lstrcpy( n_szRegSharedDLLs, szStr);
	};
	LPTSTR			GetRegSharedDLLs()		{ return n_szRegSharedDLLs;};

	void	SetRegUninstall(LPTSTR szStr)	
	{ 
		assert(sizeof(n_szRegUninstall) >= (strlen(szStr) * sizeof(TCHAR)));
		lstrcpy( n_szRegUninstall, szStr);
	};
	LPTSTR			GetRegUninstall()		{ return n_szRegUninstall;};

	void	SetSetupTitle(LPTSTR szStr)		
	{ 
		assert(sizeof(n_szSetupTitle) >= (strlen(szStr) * sizeof(TCHAR)));
		lstrcpy( n_szSetupTitle, szStr);
	};
	LPTSTR			GetSetupTitle()			{ return n_szSetupTitle;};

	void	SetUserPath(LPTSTR szStr)	
	{ 
		assert(sizeof(n_szUserPath) >= (strlen(szStr) * sizeof(TCHAR)));
		lstrcpy( n_szUserPath, szStr);
	};
	LPTSTR			GetUserPath()		{ return n_szUserPath;};

	void	SetOS(WORD wOS)					{ n_wOS = wOS;};
	WORD			GetOS()					{ return n_wOS;};

	void	SetOldFont(HGDIOBJ hObj)		{ n_hOldFont = hObj; };
	HGDIOBJ			GetOldFont()			{ return n_hOldFont;};

	void	SetPrinterFont(HGDIOBJ hObj)		{ n_hPrinterFont = hObj; };
	HGDIOBJ			GetPrinterFont()			{ return n_hPrinterFont;};

	void	SetListEnd(LPINSTALLLIST lpList)	{ n_ListEnd = lpList;};
	LPINSTALLLIST	GetListEnd()				{ return n_ListEnd;};

	void	SetListHead(LPINSTALLLIST lpList)	{n_ListHead = lpList;};
	LPINSTALLLIST	GetListHead()				{ return n_ListHead;};

	void	SetReboot(BOOL b)					{ n_bReboot = b;};
	BOOL			GetReboot()					{ return n_bReboot;};

	void	SetResultCode(EBURETCODE  retCode)	{ n_EBUResultCode = retCode;};
	EBURETCODE		GetResultCode()				{ return n_EBUResultCode;};

	void	SetDirtyBits(WORD wDirtyBits)		{ n_wDirtyBits |= wDirtyBits;};
	void	ClearDirtyBits()					{ n_wDirtyBits = 0;};
	WORD	GetDirtyBits()						{ return n_wDirtyBits;};

	void	SetCopyIncomplete(BOOL b)			{ n_bCopyIncomplete = b;};
	BOOL	GetCopyIncomplete()					{ return n_bCopyIncomplete;};

	void	SetWriteUninstall(BOOL b)			{ n_fWriteUninstall = b;};
	BOOL	GetWriteUninstall()					{ return n_fWriteUninstall; };

	void	SetMCISound(BOOL b)					{ n_bUseMCISound = b;};
	BOOL	GetMCISound()						{ return n_bUseMCISound;};

	void	SetNoSound(BOOL b)					{ n_bNoSound = b;};
	BOOL	GetNoSound()						{ return n_bNoSound;};

	void	SetMaxDirLen(int len)				{ n_nMaxDirLen = len;};
	int		GetMaxDirLen()						{ return n_nMaxDirLen;};

	void	SetOldGroupList(_int64 nGroup)		{ n_OldGroupList = nGroup;};
	_int64	GetOldGroupList()					{ return n_OldGroupList;};

	void	SetAppDirExists(BOOL fAppDir)		{ n_fAppDirExists = fAppDir;};
	BOOL	GetAppDirExists()					{ return n_fAppDirExists;};


}

