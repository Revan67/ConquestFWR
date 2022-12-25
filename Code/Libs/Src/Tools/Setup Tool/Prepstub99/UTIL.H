//
// util.h
//

#ifndef __UTIL_H
#define __UTIL_H
#include "stdio.h"

BOOL DoesFileExist( LPCSTR lpszFilename );

#define WM_UPDATEDISPLAY WM_USER + 1500

UINT MyGetWindowsDirectory( LPSTR lpszBuf, UINT cbBuf );
UINT MyGetSystemDirectory( LPSTR lpszBuf, UINT cbBuf );
void GetWindowsDriveRoot( LPSTR lpszBuf, UINT cbBuf );
UINT GetModuleDirectory( LPSTR lpszBuf, UINT cbBuf );

WORD GetCurrentOperatingSystem();

BOOL IsRangeLockingInstalled( char* pszTestFile );
HGLOBAL LoadFile( LPCSTR lpszFileName, DWORD* pFileSize = NULL );
int __cdecl Alert( HWND hwndParent, UINT uType, UINT uMessage, ... );
int __cdecl Alert( HWND hwndParent, UINT uType, char szMessage[], ... );

int StrToInt(LPCSTR lpstr);
BOOL UpdateDisplay(void);
BOOL EbuYield(void);

bool __cdecl stprintf (UINT uStr, ... );
bool __cdecl fstprintf (FILE *fp, UINT uStr, ... );
bool __cdecl FormatString (char *szText, UINT uStr, ... );

bool IsDirectoryWritable (char *szPath);
void SplitPathAndFileName (char *pszIn, char *pszDirOut, char *pszFileOut);
bool IsFileWritable (char *szFileName);


BOOL FileSaveAsDialog (HWND hWndParent, char *szFileName, UINT uFilter, int *nType);
BOOL FileSaveDialog (HWND hWndParent, char *szFileName, char *szDefaultName, UINT uFilter);
BOOL FileSaveDialog (HWND hWndParent, char *szFileName, UINT uFilter);
BOOL FolderBrowse (char *szRootPath, char *szInstructions);
BOOL FileOpenDialog (HWND hWndParent, char *szDialogTitle, char *pszDirName, char *pszFileName, UINT uFilter);

void __cdecl WriteLogFileMessage (char *szMessage, ...);
void __cdecl WriteLogFileMessage (UINT uMessage, ... );


#endif //__UTIL_H
