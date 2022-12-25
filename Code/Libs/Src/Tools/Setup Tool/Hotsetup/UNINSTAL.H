//
// uninstal.h
//
//		Function prototypes for uninstall commands
//

#ifndef __UININSTAL_H
#define __UNINSTALL_H

BOOL PerformUninstallCommands(void);
BOOL MyReleaseSharedDll(LPCSTR lpszValue, BOOL fDLLRegister, BOOL fDeleteWhenZeroRefCount);
BOOL RemoveBaseKey(HKEY hkeyBase, LPCSTR lpsz, BOOL IgnoreRegValues);

#endif // !__UNINSTALL_H
