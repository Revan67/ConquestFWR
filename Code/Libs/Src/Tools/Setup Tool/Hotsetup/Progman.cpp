#include <windows.h>
#include <windowsx.h>
//#include "initguid.h"
#define INITGUID
#include "objbase.h"
extern "C" {
#include "intshcut.h"
#include "shlguid.h"
#include <shlobj.h>
}
#include "setup.h"
#include "HotSetupRC.h"
#include "util.h"
 
#include "progman.h"

using namespace NGLOBALS;

/**************************************************************************

   CreateLink()

   uses the shell's IShellLink and IPersistFile interfaces to create and
   store a shortcut to the specified object.

   Returns the result of calling the member functions of the interfaces.

   lpszPathObj - address of a buffer containing the path of the object

   lpszPathLink - address of a buffer containing the path where the shell
      link is to be stored

   lpszDesc - address of a buffer containing the description of the shell
      link

**************************************************************************/

HRESULT CreateLink(LPCSTR lpszSource,
				   LPSTR lpszTarget,
				   LPSTR lpszDesc,
				   LPSTR lpszWorkDir,
				   LPSTR lpszArgs,
				   LPSTR lpszIconSource,
				   int icon)
{
	HRESULT hres;
	IShellLink* pShellLink;
	LPSTR pszIconSourceFile;

	CoInitialize(NULL);

	//CoInitialize must be called before this
	// Get a pointer to the IShellLink interface.
	char http[5];
	boolean fURL = FALSE;
	IUniformResourceLocator *pUrl;

	lstrcpyn(http,lpszSource,5);
	if(!lstrcmpi(http,"http"))
		fURL = TRUE;

	if(!fURL)
	{
		hres = CoCreateInstance(CLSID_ShellLink,
								NULL,
								CLSCTX_INPROC_SERVER,
								IID_IShellLink,
								(LPVOID*)&pShellLink);

	}
	else
	{
		hres = CoCreateInstance(CLSID_InternetShortcut,
								NULL,
								CLSCTX_INPROC_SERVER,
								IID_IUniformResourceLocator,
								(LPVOID*)&pUrl);
	}
	if (SUCCEEDED(hres))
	{
		if(fURL)
		{
			hres = pUrl->QueryInterface(IID_IShellLink,(LPVOID *)&pShellLink);
			if(!SUCCEEDED(hres))
			{
				pUrl->Release();
				return hres;
			}

			pUrl->SetURL(lpszSource,IURL_SETURL_FL_GUESS_PROTOCOL);
		}
		else
		{
			pShellLink->SetPath(lpszSource);
			pShellLink->SetWorkingDirectory(lpszWorkDir);
			pShellLink->SetArguments(lpszArgs);

			//
			//If an alternate icon source file was specified, use it for finding the icon,
			//otherwise use the source file the link will point to... 
			//
			pszIconSourceFile = (lstrcmpi(lpszIconSource, "NULL")) ? lpszIconSource : (LPSTR) lpszSource;
			pShellLink->SetIconLocation(pszIconSourceFile,icon);
		}

		IPersistFile* pPersistFile;

		// Set the path to the shortcut target, and add the description.
		pShellLink->SetDescription(lpszDesc);

		// Query IShellLink for the IPersistFile interface for saving the
		// shortcut in persistent storage.
		hres = pShellLink->QueryInterface(IID_IPersistFile, (LPVOID*)&pPersistFile);

		if (SUCCEEDED(hres))
		{
			WCHAR wsz[MAX_PATH];

			// Ensure that the string is ANSI.
			MultiByteToWideChar(CP_ACP,
								0,
								lpszTarget,
								-1,
								wsz,
								MAX_PATH);

			// Save the link by calling IPersistFile::Save.
			hres = pPersistFile->Save(wsz, TRUE);

			pPersistFile->Release();
		}

		pShellLink->Release();
		if(fURL)
			pUrl->Release();
	}

	CoUninitialize();

	return hres;
}

BOOL AddGroupToStartMenu(char *GroupName)
{
LPITEMIDLIST   pidlStartMenu;
char           szPath[MAX_PATH];

//get the pidl for the start menu - this will be used to intialize the folder browser
SHGetSpecialFolderLocation(NULL, 
						   GetOS() & OS_WINMASK ? CSIDL_PROGRAMS : CSIDL_COMMON_PROGRAMS,
						   &pidlStartMenu);

//get the path for the folder
SHGetPathFromIDList(pidlStartMenu, szPath);

lstrcat(szPath,"\\");
lstrcat(szPath,GroupName);

//create the folder
CreateFolder(szPath);


return TRUE;
}

/**************************************************************************

   CreateFolder()

**************************************************************************/

BOOL CreateFolder(LPSTR lpszFolder)
{
//create the folder
//CreateDirectory(lpszFolder, NULL);
   char working[512];
   char *ptr, *ptrPrev;
   char ch;
   int numDirs=1,count,x;

   // check directory name
   if(lstrlen(lpszFolder) == 0)
   {
        Alert( GetWndParent(), MB_ICONSTOP | MB_OK, STR_ERROR_MUSTGETDIR );
        return(IDCANCEL);
   }
   // check directory name length
   if( lstrlen(lpszFolder) > 512 - 1 )
   {
        Alert( GetWndParent(), MB_ICONSTOP | MB_OK, STR_ERROR_BADDIR );
        return(IDCANCEL);
   }
   lstrcpy(working,lpszFolder);

   ptr = AnsiNext( working );
   if( *ptr == ':')
   {
	   ch = working[0];
	   if( !isalpha(ch) )
		   return(IDCANCEL);
	   ptr = AnsiNext( ptr );
	   ch = *ptr;
	   ptr = AnsiNext( ptr );
	   if( ch == '\\' && *ptr == '\\')
	   {
           if(Alert( GetWndParent(), MB_ICONEXCLAMATION | MB_OKCANCEL, STR_ERROR_BADDIR )==IDCANCEL)
		   {
			   if(Alert(GetWndParent(), MB_ICONEXCLAMATION | MB_OKCANCEL, STR_ABORT_SETUP)==IDOK)
			   {
				   return(IDABORT);
			   }
			   else
				   return(IDCANCEL);
		   }
		   else 
			   return(IDCANCEL);
	   }
   }

   //
   // check the last char
   // if ended by '\\' put NUL
   //
   ptr = (LPSTR) CharPrev( working, working + lstrlen(working));
   if( *ptr == '\\' )
   {
	   if( *AnsiNext(ptr) == '\\' && ptr != working )
	   {
           if(Alert( GetWndParent(), MB_ICONEXCLAMATION | MB_OKCANCEL, STR_ERROR_BADDIR )==IDCANCEL)
		   {
            if(Alert(GetWndParent(), MB_ICONEXCLAMATION | MB_OKCANCEL, STR_ABORT_SETUP)==IDOK)
            {
               return(IDABORT);
            }
		    else
		      return(IDCANCEL);
		   }
		   else return(IDCANCEL);
	   }
	   *ptr = '\0';
   }

   //
   // count Dirs
   //
   ptr = working;
   ptrPrev = ptr;
   while( *ptr )
   {
	   if( *ptr == '\\' )
		   numDirs++;
       ptrPrev = ptr;
	   ptr = AnsiNext( ptr );
   }
   if( FIsFullPath(working) )
	   numDirs--;
   if(numDirs <= 0)
	   numDirs = 1;

   count = 0;
   for(x=0;x<numDirs;x++)
   {
       count = 0;
	   lstrcpy(working,lpszFolder);
	   ptr = working;
	   while( *ptr != '\0' )
	   {
		   ptrPrev = ptr;
		   if( *ptr == '\\' )
		   {
			   if( ptr != working && *ptrPrev != ':' && count > x)
			   {
				   *ptr = '\0';
				   break;
			   }
			   count++;
		   }
		   ptr = AnsiNext( ptr );
	   }
	   
  
	   // see if this directory already exists
	   DWORD dwfa = GetFileAttributes(working);
	   
	   if (0xFFFFFFFF == dwfa)
	   {
		   // nothing exists so try to create it
		   if(CreateDirectory(working,NULL) == FALSE)
		   {
			   if(GetLastError()==ERROR_INVALID_NAME)
			   {
				   if(Alert( GetWndParent(), MB_ICONEXCLAMATION | MB_OKCANCEL, STR_ERROR_BADDIR )==IDCANCEL)
				   {
					   if(Alert(GetWndParent(), MB_ICONEXCLAMATION | MB_OKCANCEL, STR_ABORT_SETUP)==IDOK)
					   {
						   return IDABORT;
					   }
				   }
				   return(IDCANCEL);
			   }
		   }
		   
	   }
	   else
	   {
		   // something exits now figure out what it is.
		   if (dwfa & FILE_ATTRIBUTE_DIRECTORY)
		   {
			   // dir already exists.  Nothing to do
		   }
		   else
		   {
			   // something exits but its not a dir this is bad
			   if(Alert( GetWndParent(), MB_ICONEXCLAMATION | MB_OKCANCEL, STR_ERROR_BADDIR )==IDCANCEL)
			   {
				   if(Alert(GetWndParent(), MB_ICONEXCLAMATION | MB_OKCANCEL, STR_ABORT_SETUP)==IDOK)
				   {
					   return IDABORT;
				   }
			   }
			   return IDCANCEL;
		   }
	   }
   }

//notify the shell that you made a change
// BoundsChecker throws bogus alert on the following call.  -- checked by a-melodh 21.9.98
// SHChangeNotify.  The second argument below is valid.
SHChangeNotify(SHCNE_MKDIR, SHCNF_PATH | SHCNF_FLUSH, lpszFolder, (LPCVOID)NULL);

return TRUE;
}


/**************************************************************************

   AddItemToStartMenu()

**************************************************************************/
BOOL AddItemToStartMenu(char *szLink, char *szProgram, char *szIconSource, char *szWorkingDir, char *szCmdArgs, int icon)
{
char   szPath[MAX_PATH];
LPITEMIDLIST   pidlStartMenu;

//get the pidl for the start menu
SHGetSpecialFolderLocation(NULL, 
						   GetOS() & OS_WINMASK ? CSIDL_PROGRAMS : CSIDL_COMMON_PROGRAMS,
						   &pidlStartMenu);

SHGetPathFromIDList(pidlStartMenu, szPath);
lstrcat(szPath,"\\");
lstrcat(szPath,szLink);
char http[5];
lstrcpyn(http,szProgram,5);
if(!lstrcmpi(http,"http"))   // is it a url?
//add .LNK to the shortcut file name
   lstrcat(szPath, ".url");
else
//add .LNK to the shortcut file name
   lstrcat(szPath, ".lnk");

//create the shortcut
CreateLink(szProgram, szPath, "",szWorkingDir,szCmdArgs,szIconSource,icon);

return TRUE;
}


/**************************************************************************

   AddItemToDesktop()

**************************************************************************/
BOOL AddItemToDesktop(char *szLink, char *szProgram, char *szIconSource, char *szWorkingDir, char *szCmdArgs, int icon)
{
char   szPath[MAX_PATH];
LPITEMIDLIST   pidlStartMenu;

//get the pidl for the start menu
SHGetSpecialFolderLocation(NULL, CSIDL_DESKTOP, &pidlStartMenu);

SHGetPathFromIDList(pidlStartMenu, szPath);
lstrcat(szPath,"\\");
lstrcat(szPath,szLink);

char http[5];
lstrcpyn(http,szProgram,5);
if(!lstrcmpi(http,"http"))   // is it a url?
//add .LNK to the shortcut file name
   lstrcat(szPath, ".url");
else
//add .LNK to the shortcut file name
lstrcat(szPath, ".lnk");

//create the shortcut
CreateLink(szProgram, szPath, "",szWorkingDir,szCmdArgs,szIconSource,icon);

return TRUE;
}

/**************************************************************************

   DeleteGroupFromStartMenu()

**************************************************************************/
BOOL DeleteGroupFromStartMenu(char *GroupName,BOOL bClearAll)
{
LPITEMIDLIST   pidlPrograms;
char           szPath[MAX_PATH] = "";

	//get the pidl for the programs group - this will be used to initialize the folder browser
	if (NOERROR == SHGetSpecialFolderLocation(NULL, 
							   ((GetOS() & OS_WINMASK) ? 
							     CSIDL_PROGRAMS : 
								 CSIDL_COMMON_PROGRAMS),
							   &pidlPrograms))
	{
		//get the path for the chosen group/folder
		if (SHGetPathFromIDList(pidlPrograms, szPath))
		{
			lstrcat(szPath,"\\");
			lstrcat(szPath, GroupName);

			//delete the group/folder
			if (DeleteFolder(szPath,bClearAll))
			{
				return TRUE;
			}
		}
	}

	return FALSE;
}


//	DeleteLinkFromStartMenu()
BOOL DeleteLinkFromStartMenu(char *FileName,BOOL bClearAll)
{
LPITEMIDLIST   pidlPrograms;
char           szPath[MAX_PATH] = "";

	//get the pidl for the programs group - this will be used to initialize the folder browser
	if (NOERROR == SHGetSpecialFolderLocation(NULL, 
							   ((GetOS() & OS_WINMASK) ? 
							     CSIDL_PROGRAMS : 
								 CSIDL_COMMON_PROGRAMS),
							   &pidlPrograms))
	{
		//get the path for the chosen group/folder
		if (SHGetPathFromIDList(pidlPrograms, szPath))
		{
			lstrcat(szPath,"\\");
			lstrcat(szPath, FileName);

			//delete the file
			if (!DeleteSingleLink(szPath,bClearAll))
			{
				TRACE(STR_HARDCODE_CANTDELETE_STARTUPMENULINK);
			}
			return TRUE;
		}
	}
// your system screwed up if you can't get the special folder location
	return FALSE; 
}

/**************************************************************************

  DeleteLinkFromDesktop()
  
**************************************************************************/
BOOL DeleteLinkFromDesktop(char *LinkName,BOOL bUrl)
{
	LPITEMIDLIST   pidlPrograms;
	char           szPath[MAX_PATH];
	
	//get the pidl for the programs group - this will be used to initialize the folder browser
	if (NOERROR == SHGetSpecialFolderLocation(NULL, CSIDL_DESKTOP, &pidlPrograms))
   {

	   //get the path for the chosen group/folder
	   if (SHGetPathFromIDList(pidlPrograms, szPath))
	   {
		   // MEMO : Jul.15,1997 21:02 by yutaka.
		   // Check last char of szPath and top char of the LinkName for safe.
		   ASSERT( *CharPrev(szPath,szPath+lstrlen(szPath))!='\\' );
		   ASSERT( LinkName[0]!='\\' );
   
		   lstrcat(szPath,"\\");
		   lstrcat(szPath,LinkName);
		   if(bUrl)
		      lstrcat(szPath,".url");
		   else
		      lstrcat(szPath,".lnk");

		   //delete the link
		   if (DeleteFile(szPath))
		   {
			   // BoundsChecker throws bogus alert on the following call.  -- checked by a-melodh 21.9.98
			   // SHChangeNotify.  The second argument below is valid.
			   SHChangeNotify(SHCNE_DELETE, SHCNF_PATH | SHCNF_FLUSH, szPath, (LPCVOID)NULL);
			   return TRUE;
		   }
	   }
   }

   return FALSE;
}

/**************************************************************************

   DeleteFolder()

**************************************************************************/

BOOL DeleteFolder(LPSTR lpszFolder,BOOL bClearAll)
{
	char              szFile[_MAX_PATH];
	WIN32_FIND_DATA   FindData;
	HANDLE            hFind;
	BOOL              bFindFile = TRUE;
	
	//we can't remove a directory that is not empty, so we need to empty this one
	
	lstrcpy(szFile, lpszFolder);
	lstrcat(szFile, "\\*.*");
		
	hFind = FindFirstFile(szFile, &FindData);
	while((INVALID_HANDLE_VALUE != hFind) && bFindFile)
	{
		if(*(FindData.cFileName) != '.' && bClearAll)
		{
			//copy the path and file name to our temp buffer
			if (0 == GetShortPathName(lpszFolder, szFile, _MAX_PATH))
			{
				lstrcpy(szFile, lpszFolder);
			}

			lstrcat(szFile, "\\");

			//
			//If the filename found wasn't an LFN or a name with spaces in it, then
			//cAlternateFileName will == "".  In this case, you cFilename which will
			//specify the name correctly.  Oh, by the way, this is only a problem
			//under Windows NT 4.0 *WORKSTATION* - cAlternateFileName is set to the
			//same value as cFilename in this case for all other operating systems...
			//Woohoo!  Gotta love it!  reizen 04/29/98.
			//
			lstrcat(szFile, *(FindData.cAlternateFileName) ? 
				FindData.cAlternateFileName : FindData.cFileName);

			//add a second NULL because SHFileOperation is looking for this
			lstrcat(szFile, "\0");
			
			//delete the file
			DeleteFile(szFile);
			// BoundsChecker throws bogus alert on the following call.  -- checked by a-melodh 21.9.98
			// SHChangeNotify.  The second argument below is valid.
			SHChangeNotify(SHCNE_DELETE, SHCNF_PATH | SHCNF_FLUSH, szFile, (LPCVOID)NULL);
		}
		
		//find the next file
		bFindFile = FindNextFile(hFind, &FindData);
	}
	if(hFind != INVALID_HANDLE_VALUE)
		FindClose(hFind);
	
	DWORD dwfa = GetFileAttributes(lpszFolder);
	
	if ((0xFFFFFFFF != dwfa) && (dwfa & FILE_ATTRIBUTE_DIRECTORY))
		return RemoveDirectory(lpszFolder);
	else
		return TRUE;
}


BOOL DeleteSingleLink(LPSTR lpszFolder,BOOL bClearAll)
{
	TCHAR             szFile[MAX_PATH];
	
	//we can't remove a directory that is not empty, so we need to empty this one
	
	lstrcpy(szFile, lpszFolder);
	// add the extension
	lstrcat(szFile, ".lnk");
		
	//copy the path and file name to our temp buffer
	if (0 == GetShortPathName(szFile, szFile, MAX_PATH))
	{
		LPVOID lpMsgBuf;
		FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
					  NULL,
					  GetLastError(),
					  MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
					  (LPTSTR) &lpMsgBuf,
					  0,
					  NULL);
		TRACE((LPCTSTR)lpMsgBuf);
		LocalFree(lpMsgBuf);

		lstrcpy(szFile, lpszFolder);
	}

	//add a second NULL because SHFileOperation is looking for this
	lstrcat(szFile, "\0");
	
	//delete the file
	BOOL test = DeleteFile(szFile);
	// BoundsChecker throws bogus alert on the following call.  -- checked by a-melodh 21.9.98
	// SHChangeNotify.  The second argument below is valid.
	SHChangeNotify(SHCNE_DELETE, SHCNF_PATH | SHCNF_FLUSH, szFile, (LPCVOID)NULL);
	
	return TRUE;	
}
