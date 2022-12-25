//
// util.cpp
//
//		Utility functions
//

#include "prepstub.h"
#include <dos.h>
#include <stdarg.h>
#include <io.h>
#include <fcntl.h>
#include "util.h"
#include <stdio.h>
#include "shlobj.h"  // for FolderBrowse

extern BOOL g_bCommandLine;
extern char g_szLogFile[_MAX_PATH];

//****************************************************************************
// Procedure	DoesFileExist
//
// Purpose		Determines whether the given file exists on disk
//
// Parameters	lpszFilename		full pathname of the file to look for
//
// Returns		nonzero if the file exists, zero if not
//
// History		 1/26/95	KenSh		Created
//
BOOL DoesFileExist( LPCSTR lpszFilename )
{
	OFSTRUCT of;

	return( HFILE_ERROR != OpenFile( lpszFilename, &of, OF_EXIST ) );
}


//****************************************************************************
// Procedure	GetCurrentOperatingSystem
//
// Purpose		Determines what platform we're currently running on
//
// Parameters	none
//
// History		 1/31/95	KenSh		Created
//
WORD GetCurrentOperatingSystem()
{
	static WORD wResult = 0;

	if( !wResult )
	{
		OSVERSIONINFO 
			OSVersionInfo;

		OSVersionInfo.dwOSVersionInfoSize = sizeof(OSVersionInfo);
		GetVersionEx(&OSVersionInfo);

		switch(OSVersionInfo.dwPlatformId)
		{
		case VER_PLATFORM_WIN32_WINDOWS:
			wResult = (10 == OSVersionInfo.dwMinorVersion ? OS_WIN98 : OS_WIN95);

			break;

		case VER_PLATFORM_WIN32_NT:
			switch (OSVersionInfo.dwMajorVersion)
			{
			case 0:
			case 1:
			case 2:
			case 3:
				wResult = 0;  //NT 3.1, 3.51, etc. not supported...
				break;
				
			case 4:	// nt 4.0
				wResult = OS_NT40;
				break;

			case 5:	// nt 5.0
				wResult = OS_NT50;
				break;

			default:
				//unexpected NT version
				ASSERT(FALSE);
			}

			break;

		default:
			// unexpected platform ID
			ASSERT(FALSE);
		}
	}

	return wResult;
}

//****************************************************************************
// Procedure	LoadFile		Given a file name, returns a handle
//								pointing to the contents of that
//								file.
//
// Parameters	lpszFileName	Ye file name.
//				pFileSize		gets filled with file size, may be NULL
//
// Returns		NULL if unsuccessful, otherwise a handle of the file
//				contents.
//
// Assumptions	That the file is small enough to be entirely read into
//				memory.
//
// History		June 1992		Invented
//
HGLOBAL LoadFile( LPCSTR lpszFileName, DWORD* pFileSize /*=NULL*/ )
{
	HGLOBAL  hData;
	HPSTR	 lpData;
//	OFSTRUCT OFStruct;
	HANDLE	 hFile;
	DWORD	 lSize, lRead;
	DWORD highsize;

//	hFile = OpenFile ((LPSTR)lpszFileName, &OFStruct, OF_SHARE_DENY_WRITE | OF_READ);
    hFile = CreateFile((LPSTR)lpszFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		return NULL;
	}

	lSize = GetFileSize(hFile,&highsize);
	hData = GlobalAlloc (GMEM_MOVEABLE | GMEM_ZEROINIT, (DWORD) lSize + 1);

	if (!hData)
	{
		CloseHandle (hFile);
		return NULL;
	}

	lpData = (LPSTR) GlobalLock (hData);

	if (lpData == NULL)
	{
		CloseHandle (hFile);
		GlobalFree (hData);
		return NULL;
	}

	if (!ReadFile( hFile, (LPVOID)lpData, lSize,&lRead,NULL ) || lRead == 0)
	{
		CloseHandle (hFile);
		GlobalUnlock (hData);
		GlobalFree (hData);
		return NULL;
	}

	lpData [lSize] = '\0';
	CloseHandle (hFile);
	GlobalUnlock (hData);
	
	if( pFileSize )
	{
		*pFileSize = lSize;
	}
	
	return hData;
}


//****************************************************************************
// Procedure	Alert
//
// Purpose		Displays a message box using a format string in the resource
//				file.
//
// Parameters	hwndParent		the window to use as a parent, may be NULL
//				uType			MessageBox() style
//				uMessage		resource string to use
//				...				parameters for wvsprintf
//
// Returns		int				As returned by MessageBox()
//
int __cdecl Alert( HWND hwndParent, UINT uType, UINT uMessage, ... )
{
	va_list		vlShow;
	char		szBuffer[256];
	char		szText[1024];
	UINT		uStr;

	uStr = LoadString( g_hInst, uMessage, szBuffer, sizeof(szBuffer) );

	if (uStr)
	{
		va_start (vlShow, uMessage);
		wvsprintf (szText, szBuffer, vlShow);
		va_end (vlShow);
	}

	if( hwndParent )
		uType |= MB_APPLMODAL;
	else
		uType |= MB_TASKMODAL;
	
	if (g_bCommandLine)
	{
		FILE
			*pOutFile = fopen(g_szLogFile, "a+");
		
		fputs(szText, pOutFile);
		fputs("\n", pOutFile);
		printf ("[%s]\n", szText);	

		fclose(pOutFile);
		return IDOK;
	}else{
		MessageBeep( uType & MB_ICONMASK );
		return MessageBox( hwndParent, szText, g_szAppTitle, uType );
	}
}


int __cdecl Alert( HWND hwndParent, UINT uType, char szMessage[], ... )
{
	va_list		vlShow;
	char		szText[1024];

	if (szMessage)
	{
		va_start (vlShow, szMessage);
		wvsprintf (szText, szMessage, vlShow);
		va_end (vlShow);
	}

	if( hwndParent )
		uType |= MB_APPLMODAL;
	else
		uType |= MB_TASKMODAL;
	
	if (g_bCommandLine)
	{
		FILE
			*pOutFile = fopen(g_szLogFile, "a+");
		
		fputs(szText, pOutFile);

		fclose(pOutFile);
		return IDOK;
	}else{
		MessageBeep( uType & MB_ICONMASK );
		return MessageBox( hwndParent, szText, g_szAppTitle, uType );
	}
}



//****************************************************************************
// Procedure	StrToInt(LPCSTR)
//
// Purpose		Converts a LPSTR to an int; like C library atol() function
//
int StrToInt(LPCSTR lpstr)
{
	int lwRet;
	int lwDigit;
	LPCSTR pch;
	CHAR ch;
	
	lwRet = 0;
	pch = lpstr;
	
	//
	// skip preceeding space
	//
	while ( *pch == ' ' )
	{
		if ( *pch == '\0' )
			goto LError;
		pch = AnsiNext( pch );
	}
	
	//
	// parse the figure
	//
	for ( ;; )
	{
		ch = *pch;
		if ( ch < '0' || ch > '9' )
			goto LFigEnd;
		lwDigit = ch - '0';
		lwRet = lwRet * 10;
		lwRet += lwDigit;
		pch = AnsiNext( pch );
	}
LFigEnd:
	//
	// success
	//
	return lwRet;
	
LError:
	//
	// return zero in case of error
	//
	return 0;
}

BOOL UpdateDisplay(void)
{
	const int nMAX_BAR = 12;
	const char szBAR_CHAR[] = "X";

	MSG   msg;

	static char szBar[nMAX_BAR + 1] = "";
	static int  nBarLength = 1;

	while (PeekMessage (&msg, NULL, 0, 0, PM_REMOVE))
	{
		if( msg.message == WM_QUIT ||
			msg.message == WM_CLOSE ||
			msg.message == WM_DESTROY )
		{
			// Put the message back on the queue and get out of here.
			PostMessage(msg.hwnd, msg.message, msg.wParam, msg.lParam);
			return FALSE;
		}

		TranslateMessage (&msg);
		DispatchMessage (&msg);
	}

	if (nBarLength++ == nMAX_BAR)
	{
		lstrcpy(szBar, "");
		nBarLength = 1;
	}

	lstrcat(szBar, szBAR_CHAR);
	SendMessage(g_hwnd, WM_UPDATEDISPLAY, 0, (LPARAM) szBar);
			
	return TRUE;
}

BOOL EbuYield (void)
{
	MSG   msg;

	while (PeekMessage (&msg, NULL, 0, 0, PM_REMOVE))
	{
		if( msg.message == WM_QUIT ||
			msg.message == WM_CLOSE ||
			msg.message == WM_DESTROY )
		{
			// Put the message back on the queue and get out of here.
			PostMessage(msg.hwnd, msg.message, msg.wParam, msg.lParam);
			return FALSE;
		}

		TranslateMessage (&msg);
		DispatchMessage (&msg);
	}
	return TRUE;
}


bool __cdecl FormatString (char *szText, UINT uStr, ... )
{
	va_list		vlShow;
	char		szBuffer[256];

	uStr = LoadString( g_hInst, uStr, szBuffer, sizeof(szBuffer) );

	if (uStr)
	{
		va_start (vlShow, uStr);
		wvsprintf (szText, szBuffer, vlShow);
		va_end (vlShow);
	}
	
	return (uStr==0?false:true);
}


bool __cdecl fstprintf (FILE *fp, UINT uStr, ... )
{
	va_list		vlShow;
	char		szBuffer[256];
	char		szText[1024];

	uStr = LoadString( g_hInst, uStr, szBuffer, sizeof(szBuffer) );

	if (uStr)
	{
		va_start (vlShow, uStr);
		wvsprintf (szText, szBuffer, vlShow);
		va_end (vlShow);
		fprintf (fp, szText);
	}
	
	return (uStr==0?false:true);
}


bool __cdecl stprintf (UINT uStr, ... )
{
	va_list		vlShow;
	char		szBuffer[256];
	char		szText[1024];

	uStr = LoadString( g_hInst, uStr, szBuffer, sizeof(szBuffer) );

	if (uStr)
	{
		va_start (vlShow, uStr);
		wvsprintf (szText, szBuffer, vlShow);
		va_end (vlShow);
		printf (szText);
	}
	
	return (uStr==0?false:true);
}


void SplitPathAndFileName (char *pszIn, char *pszDirOut, char *pszFileOut)
{
	char *p, *pLast;

	if (NULL == pszIn)
		return;

	if (NULL == pszDirOut)
		return;

	ASSERT (NULL != pszFileOut);

	if (NULL == pszFileOut)
		return;

	p = pszIn;
	pLast = NULL;

	while (*p)
	{
		if ('\\' == *p)
			pLast = p;
		++p;
	}

	// Was there a directory?
	if (NULL != pLast)
	{
		while (pszIn <= pLast)
		{
			*(pszDirOut++) = *(pszIn++);
		}
		*pszDirOut = '\0';

		while (*pszIn)
		{
			*(pszFileOut++) = *(pszIn++);
		}
		*pszFileOut = '\0';
	}
	else
	{
		lstrcpy (pszFileOut, pszIn);
		*pszDirOut = '\0';
	}
}


bool IsDirectoryWritable (char *szPath)
{
	HANDLE hFile;
	char szTempFileName[MAX_PATH];

	GetTempFileName (szPath, "PS9", 0, szTempFileName);

	hFile = CreateFile (szTempFileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	if (INVALID_HANDLE_VALUE == hFile)
	{
		return false;
	}

	CloseHandle (hFile);
	DeleteFile (szTempFileName);
	return true;
}


bool IsFileWritable (char *szFileName)
{
	HANDLE hFile;
	
	hFile = CreateFile (szFileName, 
						GENERIC_WRITE, 
						0, 
						NULL, 
						CREATE_ALWAYS, 
						FILE_ATTRIBUTE_NORMAL, 
						NULL);

	if (INVALID_HANDLE_VALUE == hFile)
	{
		return false;
	}

	CloseHandle (hFile);

	return true;
}


void MakePathCompliant (char *szPath)
{
	if (szPath[strlen(szPath)-1] != '\\')
		lstrcat (szPath, "\\");
}


BOOL FileOpenDialog (HWND hWndParent, char *szDialogTitle, char *pszDirName, char *pszFileName, UINT uFilter)
{
	OPENFILENAME ofn;
	BOOL ret;
	int i;

	char szDirName[MAX_PATH];
	char szFile[255];
	char szFileTitle[255];
	char szFilterString[255];
	char szDefExt[4];
	
	//szDirName[0] = '\0';
	//szFile[0] = '\0';
	lstrcpy (szFile, pszFileName);
	lstrcpy (szDirName, pszDirName);

	memset (&ofn, 0 ,sizeof (ofn));

	// Extract the filter string and default ext from string.
	// Format: TypeFriendlyName|*.EXT|2ndTypeFriendlyName\|*.EXT|+XYZ
	// where XYZ is the default 3 char extension.

	LoadString( g_hInst, uFilter, szFilterString, sizeof(szFilterString) );

	lstrcpy (szDefExt, "");

	char *p = szFilterString;
	char *p2 = szDefExt;

	while (*p && *p!='+') 
		p++;

	*(p++)='\0';

	while (*p)
		*(p2++) = *(p++);

	// Convert all |'s in filter string to \0's.	
	for (i=0;szFilterString[i]!='\0';i++)
		if (szFilterString[i]=='|') 
			szFilterString[i]='\0';

	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = hWndParent;
	ofn.lpstrFilter = szFilterString;
	ofn.nFilterIndex = 1;
	ofn.lpstrFile = szFile;
	ofn.lpstrTitle = szDialogTitle;
	ofn.nMaxFile = sizeof(szFile);
	ofn.lpstrFileTitle = szFileTitle;
	ofn.nMaxFileTitle = sizeof(szFileTitle);
	ofn.lpstrInitialDir = szDirName;
	ofn.Flags =  OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;

	ret = GetOpenFileName(&ofn); 
	
	lstrcpy (pszFileName, ofn.lpstrFile);
	return (ret);
}


BOOL FolderBrowse (char *szRootPath, char *szInstructions)
{
	BROWSEINFO bi;

	ZeroMemory (&bi, sizeof (bi));

	bi.hwndOwner = g_hwnd;
	bi.pidlRoot = NULL;
	bi.lpszTitle = szInstructions;
	bi.ulFlags = BIF_RETURNONLYFSDIRS;
	bi.lpfn = NULL;
	bi.lParam = NULL;
	
	ITEMIDLIST *pItemList = SHBrowseForFolder (&bi);

	return (SHGetPathFromIDList(pItemList, szRootPath));
}


BOOL FileSaveDialog (HWND hWndParent, char *szFileName, UINT uFilter)
{
	OPENFILENAME ofn;
	BOOL ret;
	int i;
	
	char szDirName[255];
	char szFile[255];
	char szFileTitle[255];
	char szFilterString[255];
	char szDefExt[4];
	
	szDirName[0] = '\0';
	szFile[0] = '\0';

	memset (&ofn, 0 ,sizeof (ofn));
	
	// Extract the filter string and default ext from string.
	// Format: TypeFriendlyName|*.EXT|2ndTypeFriendlyName\|*.EXT|+XYZ
	// where XYZ is the default 3 char extension.

	LoadString( g_hInst, uFilter, szFilterString, sizeof(szFilterString) );

	lstrcpy (szDefExt, "");

	char *p = szFilterString;
	char *p2 = szDefExt;

	while (*p && *p!='+') 
		p++;

	*(p++)='\0';

	while (*p)
		*(p2++) = *(p++);

	// Convert all |'s in filter string to \0's.	
	for (i=0;szFilterString[i]!='\0';i++)
		if (szFilterString[i]=='|') 
			szFilterString[i]='\0';

	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = hWndParent;
	ofn.lpstrFilter = szFilterString;
	ofn.nFilterIndex = 1;
	ofn.lpstrFile = szFile;
	ofn.nMaxFile = sizeof(szFile);
	ofn.lpstrFileTitle = szFileTitle;
	ofn.nMaxFileTitle = sizeof(szFileTitle);
	ofn.lpstrInitialDir = szDirName;
	ofn.lpstrDefExt = szDefExt;
	ofn.Flags =  OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;

	ret = GetSaveFileName(&ofn); 
	
	strcpy (szFileName, ofn.lpstrFile);
	return (ret);
}

BOOL FileSaveDialog (HWND hWndParent, char *szFileName, char *szDefaultName, UINT uFilter)
{
	OPENFILENAME ofn;
	BOOL ret;
	int i;
	
	char szDirName[255];
	char szFile[255];
	char szFileTitle[255];
	char szFilterString[255];
	char szDefExt[4];
	
	szDirName[0] = '\0';
	szFile[0] = '\0';

	memset (&ofn, 0 ,sizeof (ofn));

	lstrcpy (szFile, szDefaultName);
	
	// Extract the filter string and default ext from string.
	// Format: TypeFriendlyName|*.EXT|2ndTypeFriendlyName\|*.EXT|+XYZ
	// where XYZ is the default 3 char extension.

	LoadString( g_hInst, uFilter, szFilterString, sizeof(szFilterString) );

	lstrcpy (szDefExt, "");

	char *p = szFilterString;
	char *p2 = szDefExt;

	while (*p && *p!='+') 
		p++;

	*(p++)='\0';

	while (*p)
		*(p2++) = *(p++);

	// Convert all |'s in filter string to \0's.	
	for (i=0;szFilterString[i]!='\0';i++)
		if (szFilterString[i]=='|') 
			szFilterString[i]='\0';

	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = hWndParent;
	ofn.lpstrFilter = szFilterString;
	ofn.nFilterIndex = 1;
	ofn.lpstrFile = szFile;
	ofn.nMaxFile = sizeof(szFile);
	ofn.lpstrFileTitle = szFileTitle;
	ofn.nMaxFileTitle = sizeof(szFileTitle);
	ofn.lpstrInitialDir = szDirName;
	ofn.lpstrDefExt = szDefExt;
	ofn.Flags =  OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;

	ret = GetSaveFileName(&ofn); 
	
	strcpy (szFileName, ofn.lpstrFile);
	return (ret);
}

BOOL FileSaveAsDialog (HWND hWndParent, char *szFileName, UINT uFilter, int *nType)
{
	OPENFILENAME ofn;
	BOOL ret;
	int i;
	
	char szDirName[255];
	char szFile[255];
	char szFileTitle[255];
	char szFilterString[255];
	char szDefExt[4];
	
	szDirName[0] = '\0';
	szFile[0] = '\0';

	memset (&ofn, 0 ,sizeof (ofn));
	
	// Extract the filter string and default ext from string.
	// Format: TypeFriendlyName|*.EXT|2ndTypeFriendlyName\|*.EXT|+XYZ
	// where XYZ is the default 3 char extension.

	LoadString( g_hInst, uFilter, szFilterString, sizeof(szFilterString) );

	lstrcpy (szDefExt, "");

	char *p = szFilterString;
	char *p2 = szDefExt;

	while (*p && *p!='+') 
		p++;

	*(p++)='\0';

	while (*p)
		*(p2++) = *(p++);

	// Convert all |'s in filter string to \0's.	
	for (i=0;szFilterString[i]!='\0';i++)
		if (szFilterString[i]=='|') 
			szFilterString[i]='\0';

	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = hWndParent;
	ofn.lpstrFilter = szFilterString;
	ofn.nFilterIndex = 1;
	ofn.lpstrFile = szFile;
	ofn.nMaxFile = sizeof(szFile);
	ofn.lpstrFileTitle = szFileTitle;
	ofn.nMaxFileTitle = sizeof(szFileTitle);
	ofn.lpstrInitialDir = szDirName;
	ofn.lpstrDefExt = szDefExt;
	ofn.Flags =  OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;

	ret = GetSaveFileName(&ofn); 

	*nType = ofn.nFilterIndex;

	/*
	if (ofn.Flags & OFN_EXTENSIONDIFFERENT)
	{
		strncpy (szFileName, ofn.lpstrFile, ofn.nFileExtension-1);
		szFileName[ofn.nFileExtension] = '\0';
	}
	*/
	strcpy (szFileName, ofn.lpstrFile);
	return (ret);
}

void __cdecl WriteLogFileMessage (char *szMessage, ...)
{
	va_list		vlShow;
	char szText[MAX_PATH*4];
	FILE *fp;
	fp = fopen (g_szLogFile, "a+");

	va_start (vlShow, szMessage);
	wvsprintf (szText, szMessage, vlShow);
	va_end (vlShow);

	fprintf (fp, "%s\n", szText);	
	printf ("%s\n", szText);	
	fclose (fp);
}

void __cdecl WriteLogFileMessage (UINT uMessage, ... )
{
	va_list		vlShow;
	char		szBuffer[1024];
	char		szText[2048];
	UINT		uStr;

	uStr = LoadString( g_hInst, uMessage, szBuffer, sizeof(szBuffer) );

	if (uStr)
	{
		va_start (vlShow, uMessage);
		wvsprintf (szText, szBuffer, vlShow);
		va_end (vlShow);
	}

	FILE *pOutFile = fopen(g_szLogFile, "a+");
		
	fputs(szText, pOutFile);
	fputs("\n", pOutFile);
	printf ("%s\n", szText);	

	fclose(pOutFile);
}
