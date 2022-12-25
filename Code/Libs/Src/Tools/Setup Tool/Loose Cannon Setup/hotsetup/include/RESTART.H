//
// restart.h
//

#ifndef __RESTART_H
#define __RESTART_H

BOOL GetRebootFlag();
BOOL AddFileToExitWindowsList(LPCSTR lpszDest, LPCSTR lpszTemp);
void DeleteRestartFiles();
void SetupSetRebootFlag();

#endif //__RESTART_H
