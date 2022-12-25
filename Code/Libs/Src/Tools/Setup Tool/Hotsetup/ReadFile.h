#ifndef __READFILE_H__
#define __READFILE_H__

#ifdef READFILE

EBURETCODE ExecuteReadFileList(LPREADFILELIST lpReadFileList,
	LPRUNTIMECOMMAND lpRuntime, WORD cCommands, LPRUNTIMECOMMAND prgRuntime,
	UINT uFirstResID, BOOL fFirstTime, BYTE bProcessType );

BOOL FProcessPlatformToken(char **pszToken, DWORD *pdwInstallBuild);
BOOL FProcessStringAndPlatformTokens(HANDLE hFile, int cStrings, char *pBuf,
	DWORD *pdwInstallBuild);
EBURETCODE FReadToEOL( HANDLE hFile );
EBURETCODE FBackupOneLine(HANDLE hFile, int *pwFilePos);

#define INSTALL_FILE 0
#define INSTALL_LIST 1
EBURETCODE FPreExecuteInstallTemplate(HANDLE hFile, BYTE bInstallType,
	BYTE bProcessType );

EBURETCODE FPreExecuteInstallGo(HANDLE hFile );

EBURETCODE FPreExecuteCabGo(HANDLE hFile );

EBURETCODE FPreExecuteMkRoot(HANDLE hFile, LPRUNTIMECOMMAND lpRuntime,
	WORD cCommands, LPRUNTIMECOMMAND prgRuntime, UINT uFirstResID, BOOL fFirstTime,
	BYTE bProcessType );

EBURETCODE FPreExecuteMkDir(HANDLE hFile, BYTE bProcessType);

EBURETCODE FPreExecuteAddIniTemplate(HANDLE hFile, BYTE bProcessType);

EBURETCODE FPreExecuteShellExecute(HANDLE hFile, BYTE bProcessType);

#define GET_NAME 0
#define GET_GROUP 1
EBURETCODE FPreExecuteGetTemplate(HANDLE hFile, BYTE bGetType, LPRUNTIMECOMMAND lpRuntime,
	WORD cCommands, LPRUNTIMECOMMAND prgRuntime, UINT uFirstResID, BOOL fFirstTime );

EBURETCODE FPreExecuteInstIcon(HANDLE hFile, BYTE bProcessType );

EBURETCODE FPreExecuteGetPid(HANDLE hFile );

EBURETCODE FPreExecuteInstDX(HANDLE hFile );
EBURETCODE FPreExecuteInstDPLAY(HANDLE hFile );

EBURETCODE FPreExecuteCDSpeed(HANDLE hFile );

EBURETCODE FPreExecuteDeleteFile(HANDLE hFile );

EBURETCODE FPreExecuteRegWiz(HANDLE hFile);

#endif //READFILE
#endif //__READFILE_H__
