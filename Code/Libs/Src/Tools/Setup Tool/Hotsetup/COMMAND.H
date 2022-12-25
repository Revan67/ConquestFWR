//
// command.h
//

#ifndef __COMMAND_H
#define __COMMAND_H

EBURETCODE ExecuteAllCommands(LPRUNTIMECOMMAND prgRuntime, int cCommands, BOOL fFirstTime, UINT uFirstResID);
EBURETCODE ExecuteAllUninstallCommands(LPRUNTIMECOMMAND prgRuntime, int cCommands, BOOL fFirstTime );
EBURETCODE ExecuteGetInstallGroups(LPGETGROUP lpGroup,LPRUNTIMECOMMAND lpRuntime,WORD cCommands,LPRUNTIMECOMMAND prgRuntime,UINT uFirstResID,BOOL fFirstTime);

#endif //__COMMAND_H
