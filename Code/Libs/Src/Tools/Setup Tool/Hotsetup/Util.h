//
// util.h
//

#ifndef __UTIL_H
#define __UTIL_H

#define DO_INSTALL 0
#define DO_UNINSTALL 1

BOOL DoesFileExist( LPCSTR lpszFilename );
BOOL DoesFileExistNoCriticalErrors( LPCSTR lpszFilename );
LPTSTR EBUlstrstri(LPTSTR lpSearch, LPTSTR lpFind);
UINT MyGetWindowsDirectory( LPSTR lpszBuf, UINT cbBuf );
UINT MyGetSystemDirectory( LPSTR lpszBuf, UINT cbBuf );
void GetWindowsDriveRoot( LPSTR lpszBuf, UINT cbBuf );
UINT GetModuleDirectory( LPSTR lpszBuf, UINT cbBuf );
VOID Append5C( char *szPath );
void DelTree(char P_szPath[]);
void DelFileInTree(char P_szPath[]);

LONG GetRegisteredOwner(char *buf, DWORD cbuf);

BOOL AppGetFileSizeRequirements(TCHAR tcDrive, 
								__int64 i64Group, 
								DWORD *pdwGameFreeSpace,
								DWORD *pdwGameNeeded,
								DWORD *pdwSystemFreeSpace,
								DWORD *pdwSystemNeeded);

HRSRC EBUFindResource(HMODULE hModule, LPCTSTR lpName, LPCTSTR lpType);
HGLOBAL EBULoadResource(HMODULE hModule, HRSRC hResInfo);
int EBULoadString(HINSTANCE hResInst, UINT uID, LPTSTR lpBuffer, int nBufferMax);
HANDLE EBULoadImage(HINSTANCE hInst, LPCTSTR lpszName, UINT uType, int cxDesired, int cyDesired, UINT fuLoad);
HCURSOR EBULoadCursor(HINSTANCE hInst, LPCTSTR lpCursorName);
HICON EBULoadIcon(HINSTANCE hInst, LPCTSTR lpIconName);
HBITMAP EBULoadBitmap(HINSTANCE hInst, LPCTSTR lpBitmapName);
BOOL EBUPlaySound(LPCSTR pszSound, HMODULE hmod, DWORD fdwSound);
DWORD GetBuildCharacteristics(void);
BOOL ExecOnThisPlatform(DWORD dwBuildFlags);
BOOL SetApp1Flag(BOOL fSet);
void DisplaySystemError(DWORD dwError, UINT uiType);

BOOL IsPreviewEnabled(void);
BOOL IsApp1(void);
BOOL IsApp2(void);
BOOL IsApp3(void);
BOOL IsRetail(void);
BOOL IsDBCS(void);
BOOL IsANSI(void);
BOOL IsOEM(void);
BOOL IsIMEEnabled(void);
BOOL IsIMEOn(void);
BOOL IsUSA(void);
BOOL IsJapan(void);
BOOL IsFrench(void);
BOOL IsGerman(void);
BOOL IsSpanish(void);

LPSTR pszGetLast5C( LPCSTR psz );
BOOL FIsFullPath( LPCSTR lpszFileName );

int __cdecl Alert(HWND hwndParent, UINT uType, UINT uMessage, ...);
int __cdecl Alert(HWND hwndParent, UINT uType, LPTSTR tcsMessage, ...);
int __cdecl Alert(HWND hwndParent, UINT uType, LPTSTR tcsMessage, va_list args);
BOOL ForwardMessages();
BYTE GetServicePack();
WORD GetCurrentOperatingSystem();
VOID DLLRegister(const char *sz, BYTE bProcessType);
VOID ReplaceStringTokens(char *sz, size_t wBuf);
LONG GetProgramFilesLocation(char *buf, DWORD cbuf);
LONG GetCommonFilesLocation(char *buf, DWORD cbuf);
BOOL IsAdmin(void);
BOOL AddSharedDLL(LPSTR szFileName);
BOOL RemoveSharedDLL(LPSTR szFileName);
void ClearSharedDLL();
BOOL IsMinVersionInstalled(LPSTR lpFilePath, WORD wMajor, WORD wMinor, WORD wBuildHi, WORD wBuildLo);
BOOL MyGetDiskFreeSpace(LPCTSTR lpRootPathName, LPDWORD lpBytesPerCluster, LPDWORD lpNumberOfFreeClusters);
DIRECT_X_VERSION CheckDXVersion( char *pszDXVerString, char *pszDXDllName );
DIRECT_X_VERSION CheckDPLAYVersion( char *pszDPLAYVerString );
WORD GetDefaultButtonResponse(DWORD dwMsgType);

extern void DeleteBootstrapper(void);
extern LONG MyGetUniversalIniString( HKEY Win32Key, LPSTR lpszKey, LPCSTR szValue, LPCSTR lpszDefault, LPSTR lpszBuf, int cbBuf );

LPSTR  EBUstrcpyn(TCHAR *pszDest, TCHAR *pszSrc, int n);
LPTSTR EBUstrstr(LPTSTR szSearchString, LPTSTR szSearchFor);

HANDLE EBUCreateFile(LPCTSTR lpFileName, 
					 DWORD dwDesiredAccess, 
					 DWORD dwShareMode, 
					 LPSECURITY_ATTRIBUTES lpSecurityAttributes, 
					 DWORD dwCreationDistribution, 
					 DWORD dwFlagsAndAttributes, 
					 HANDLE hTemplateFile);
BOOL EBUReadFile(HANDLE hFile, 
				 LPVOID lpBuffer, 
				 DWORD nNumberOfBytesToRead, 
				 LPDWORD lpNumberOfBytesRead, 
				 LPOVERLAPPED lpOverlapped);
BOOL EBUCopyFile(LPCTSTR lpExistingFileName, 
				 LPCTSTR lpNewFileName, 
				 BOOL bFailIfExists);
LONG EBULZCopy(INT hfSource, 
			   INT hfDest);
INT EBULZOpenFile(LPTSTR lpFileName, 
				  LPOFSTRUCT lpReOpenBuf, 
				  WORD wStyle);

enum keyboardType
{
	kbError   = -1,
	kbUnknown = 0,
	kb101     = 101,
	kb106     = 106,
	kb98      = 98,
};

__int64 addKeyboardTypeFlag();
__int64 addKeyboardTypeFlag( __int64 i64TempMask );
__int64 removeKeyboardTypeFlag( __int64 i64TempMask );

#define DIRTY_INSTALLFILE	0x0001
#define DIRTY_INSTALLFONT	0x0002
#define DIRTY_INSTALLLIST	0x0004
#define DIRTY_ADDINIVALUE	0x0008
#define DIRTY_MKDIR			0x0010
#define DIRTY_INSTICON		0x0020

BOOL EBUIsDirty();

void TRACE(LPCTSTR pszMessage, ... );

void TraceLastError(DWORD LastError, BOOL bAssert=TRUE);

void TraceMciError(MCIERROR LastError, BOOL bAssert=TRUE);

void TraceMessage(DWORD dwMessageID);

BOOL EBUCreateDirectory(char * pszPath, BOOL fGameRootDir);

#endif //__UTIL_H
