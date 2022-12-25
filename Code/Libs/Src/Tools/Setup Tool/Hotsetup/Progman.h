#ifndef __PROGMAN_H__
#define __PROGMAN_H__

//extern "C"
//{
HRESULT CreateLink(LPCSTR, LPSTR, LPSTR,LPSTR,LPSTR,LPSTR,int);
BOOL AddGroupToStartMenu(char *szGroupName);
BOOL CreateFolder(LPSTR lpszFolder) ;
BOOL AddItemToStartMenu(char *szLink, char *szProgram, char *szIconSource, char *szWorkingDir, char *szCmdArgs, int icon);
BOOL AddItemToDesktop(char *szLink, char *szProgram, char *szIconSource, char *szWorkingDir, char *szCmdArgs, int icon);
BOOL DeleteGroupFromStartMenu(char *GroupName,BOOL bClearAll);
BOOL DeleteLinkFromStartMenu(char *FileName, BOOL BClearAll);
BOOL DeleteLinkFromDesktop(char *GroupName,BOOL bUrl);
BOOL DeleteFolder(LPSTR lpszFolder,BOOL bClearAll);
BOOL DeleteSingleLink(LPSTR lpszFolder,BOOL bClearAll);
//}

#endif