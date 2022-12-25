#ifdef READFILE

#pragma message( "READFILE is defined." ) // Jul.18,1997 19:04 by yutaka.

#include <dsetup.h>				// required for DSETUP_DIRECTX define


#include "stubpch.h"
#include "setup.h"
#include "hotsetup.h"
#include "command.h"
#include "util.h"
#include "readfile.h"
#include "HotSetupRC.h"
#include "diskinfo.h"
// Keyword tokens we currently recognize when doing an on the fly setup.
// Order must match array which follows
enum CMDTOKEN
{
	TOK_INSTALL,
	TOK_INIVALUE,
	TOK_SHELLEXECUTE,
	TOK_MKDIR,
	TOK_MKROOT,
	TOK_GETNAME,
	TOK_GETPID,
	TOK_INSTDX,
	TOK_INSTDPLAY,
	TOK_INSTICON,
	TOK_CDSPEED,
	TOK_DELETEFILE,
	TOK_GETGROUP,
	TOK_UNINSTALL,
	TOK_UNINSTALLALL,
	TOK_INSTALLLIST,
	TOK_INSTALLGO,
	TOK_CABGO,
	TOK_REGWIZ,
	TOK_INSTFONT,
	TOK_PERSIST,
	TOK_RECURSE,
	TOK_DELFILEINSTALL,
	NUM_CMDTOKENS
};

//contents of a keyword
typedef struct tagCMDKEYWORD
{
	CMDTOKEN CmdToken;
	char *pszKeyword;
} CMDKEYWORD, *CMDLPKEYWORD;

//NOTE that these must be in the same order as the enum above
CMDKEYWORD CmdKeywords[] =
{
	{ TOK_INSTALL,		"INSTALLFILE" },
	{ TOK_INIVALUE,		"ADDINIVALUE" },
	{ TOK_SHELLEXECUTE, "SHELLEXECUTE"},
	{ TOK_MKDIR,		"MKDIR" },
	{ TOK_MKROOT,		"MKROOT" },
	{ TOK_GETNAME,		"GETNAME" },
	{ TOK_GETPID,		"GETPID" },
	{ TOK_INSTDX,		"INSTDX" },
	{ TOK_INSTDPLAY,	"INSTDPLAY" },
	{ TOK_INSTICON,		"INSTICON" },
	{ TOK_CDSPEED,		"CDSPEED" },
	{ TOK_DELETEFILE,	"DELETEFILE" },
	{ TOK_GETGROUP,		"GETGROUP" },
	{ TOK_UNINSTALL,	"UNINSTALL" },
	{ TOK_UNINSTALLALL,	"UNINSTALL_ALL" },
	{ TOK_INSTALLLIST,	"INSTALLLIST" },
	{ TOK_INSTALLGO,	"INSTALLGO" },
	{ TOK_CABGO,		"CABGO" },
	{ TOK_REGWIZ,		"REGWIZ" },
	{ TOK_INSTFONT,		"INSTALLFONT" },
	{ TOK_PERSIST,		"PERSIST" },
	{ TOK_RECURSE,		"RECURSE" },
	{ TOK_DELFILEINSTALL,"INSTALL" },
};

enum TOKEN_STATE {TOKEN_NONE, TOKEN_OK, TOKEN_ERROR, TOKEN_EOL, TOKEN_EOF};

//xxGlob extern LPSETUPCOMMAND g_lpCommand; // current setup command.

EBURETCODE FExecuteReadFileListCommand(HANDLE hFile, CMDTOKEN cmdToken,
	LPREADFILELIST lpReadFileList,LPRUNTIMECOMMAND lpRuntime, WORD cCommands,
	LPRUNTIMECOMMAND prgRuntime, UINT uFirstResID, BOOL fFirstTime, BYTE bProcessType );

TOKEN_STATE ReadToken( HANDLE hFile, char *(&PToken) );
BOOL FReadToken( HANDLE hFile, char *(&PToken), TOKEN_STATE *pTokenState);
extern BOOL FindSpaceCreateTempFileAndCopyToIt(char *pszSrcFileName, char *pszBootStrappedName);

extern BOOL NEAR ExecuteUninstallCommand(LPRUNTIMECOMMAND lpRuntime );
extern EBURETCODE ExecuteUnInstallFile(LPINSTALLFILE lpInstallFile );
extern EBURETCODE ExecuteInstallList(LPINSTALLFILE lpInstallFile );
extern EBURETCODE ExecuteInstallGo(void);
extern EBURETCODE ExecuteCabGo(LPCABGO lpCabGo);
extern EBURETCODE ExecuteAddIniValue(LPADDINIVALUE lpAddIniValue, BYTE bProcessType);
extern EBURETCODE ExecuteShellExecute(LPSHELLEXECUTE lpShellExecute, BYTE bProcessType);
extern EBURETCODE ExecuteMkDir(LPMKDIR lpMkDir );
extern EBURETCODE ExecuteRdDir(LPMKDIR lpMkDir );
extern EBURETCODE ExecuteMkRoot(LPMKROOT lpMkRoot,LPRUNTIMECOMMAND lpRuntime, WORD cCommands, LPRUNTIMECOMMAND prgRuntime, UINT uFirstResID, BOOL fFirstTime);
extern EBURETCODE ExecuteGetName(LPGETNAME lpGetName );
extern EBURETCODE ExecuteGetPID(LPGETPID lpGetPid );
extern EBURETCODE ExecuteInstIcon(LPINSTICON lpInstIcon );
extern EBURETCODE ExecuteRemoveIcon(LPINSTICON lpInstIcon );
extern EBURETCODE ExecuteCDSpeed(LPCDSPEED lpCDSpeed );
extern EBURETCODE ExecuteGetInstallGroups(LPGETGROUP lpGroup,LPRUNTIMECOMMAND lpRuntime,WORD cCommands,LPRUNTIMECOMMAND prgRuntime,UINT uFirstResID,BOOL fFirstTime);
extern EBURETCODE ExecuteDeleteFile(LPDELETEFILE lpDeleteFile);
extern EBURETCODE ExecuteInstDX(LPINSTDX lpInstDX );
extern EBURETCODE ExecuteRegWiz(LPREGWIZ lpRegWiz);
extern EBURETCODE ExecuteInstDPLAY(LPINSTDPLAY lpInstDPLAY);

using namespace NGLOBALS;

//****************************************************************************
// Procedure   ExecuteReadFileList
//
// Purpose     Executes the ReadFileList setup command.  Executes all cmds found
//				the specified file.
//
// Parameters  
//             lpInstallFile  pointer to an INSTALLFILE struct describing
//                            the source and destination files.
//				bProcessType  Used to indicate whether this is an INSTALL or UNINSTALL.
//
// Returns     nonzero if successful, zero if setup was aborted.
//
// History     2/24/97 a-drews    Created
//
EBURETCODE ExecuteReadFileList(LPREADFILELIST lpReadFileList,
							   LPRUNTIMECOMMAND lpRuntime, WORD cCommands, LPRUNTIMECOMMAND prgRuntime,
							   UINT uFirstResID, BOOL fFirstTime, BYTE bProcessType )
{
	UINT iToken;
	CMDTOKEN cmdtoken;
	EBURETCODE nReturn = EBU_ERROR;
	char *szToken = NULL;
	TOKEN_STATE TokenState = TOKEN_NONE;
	HANDLE hFile = NULL;
	char szFileName[_MAX_PATH + 1];
	char szTmpFileName[_MAX_PATH + 1];
	BOOL fRemarkLine = FALSE;
	BOOL bInIDBACK = FALSE;
	int cFileLine = 1;
	int wFilePos = -1;
	DWORD nResult = 0;

	ASSERT(DO_INSTALL == bProcessType || DO_UNINSTALL == bProcessType);
	
	GetModuleDirectory(szFileName, sizeof(szFileName));
	lstrcat(szFileName, lpReadFileList->ReadFileListName());
	
	//Check that file exists and is not a directory
	nResult = GetFileAttributes(szFileName);
	if (0xFFFFFFFF == nResult ||  FILE_ATTRIBUTE_DIRECTORY == nResult)
	{
		return EBU_ERROR;
	}

	//Get a short path name for this file if possible
	if (0 == GetShortPathName(szFileName, szTmpFileName, sizeof(szTmpFileName)))
	{	
		// Revert to using LongPathName if ShortPath unavailable
		lstrcpy(szTmpFileName, szFileName);
	}
	ASSERT(*szTmpFileName);

	//If this succeeds the szFileName is our full path filename now.
	if (!FindSpaceCreateTempFileAndCopyToIt(szTmpFileName, szFileName))
	{
		return EBU_ABORT;
	};
	//Just use the OS version since Its been copied for readfile builds
	hFile = CreateFile(szFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (INVALID_HANDLE_VALUE == hFile)
	{
		char	szMessageBoxMessage[256];
		
		// unable to open the file
		wsprintf( szMessageBoxMessage, STR_HARDCODE_CANTOPENREADFILELIST, szFileName);
 		MessageBox( GetWndParent(), szMessageBoxMessage, GetSetupTitle(), MB_OK|MB_ICONINFORMATION );
		ASSERT(FALSE);
		return EBU_ERROR;
	}
	
	while ((TOKEN_EOF != TokenState) && (TOKEN_ERROR != TokenState))
	{
		if (DO_UNINSTALL == bProcessType)
		{	
			// We need execute the commands in reverse order for everything to uninstall right
			// so we need to ge backwards through the file
			int wLastPos = wFilePos;

			nReturn = FBackupOneLine(hFile, &wFilePos);
			
			if (EBU_ERROR == nReturn)
			{
				if(n_fMaintMode && wLastPos == 0) // if in maintainence mode and at start of file
					nReturn = EBU_OK;			  // return ok so we can continue
				goto Done;
			}
		}
		
		if (bInIDBACK)
		{
			// we need to backup a command so we are going to start by reading backwards through the file until we hit
			// the begining of the file or the begining of the previous line
			
			int
				iCurrFilePos = SetFilePointer(hFile, 0, NULL, FILE_CURRENT);	// get current position
			
			BOOL
				bFoundEndOfOurLine = FALSE,
				bFoundEndOfPrev = FALSE;
			
			char
				ch;
			
			DWORD
				bRead;
			
			
			do{
				if (!iCurrFilePos || (0xFFFFFFFF == iCurrFilePos))
				{
					// Hit begining of file give up and pass back to engine
					nReturn = EBU_BACK;
					goto Done;
				}
				
				// backup a character
				iCurrFilePos = SetFilePointer(hFile, -1, NULL, FILE_CURRENT);
				
				// read that char
				if (!ReadFile(hFile, &ch, 1, &bRead, NULL))
				{
					nReturn = EBU_ERROR;
					goto Done;
				}
				
				// see if this is the end of a line
				if (13 == ch)
				{
					if (!bFoundEndOfOurLine)
					{
						// Found end of our line
						bFoundEndOfOurLine = TRUE;
					}else{
						// Found the end of the previous line
						if (!bFoundEndOfPrev)
						{
							bFoundEndOfPrev = TRUE;
						}else{
							// already did the prev now on begining of the line we want
							
							fRemarkLine = FALSE;
							--cFileLine;
							// backup over the char we just checked.  We need to process the CR
							//							iCurrFilePos = SetFilePointer(hFile, -1, NULL, FILE_CURRENT);
							
							break;
						}
					}
				}
				
				// backup over the char we just checked
				iCurrFilePos = SetFilePointer(hFile, -1, NULL, FILE_CURRENT);
			} while (TRUE);
		}
		
		
		TokenState = ReadToken(hFile, szToken);
		if (TOKEN_OK != TokenState)
		{
			if (TOKEN_EOL == TokenState)
			{
				// New line; Clear Remark file and increment line count
				fRemarkLine = FALSE;
				if (bInIDBACK)
				{
					--cFileLine;
				}else{
					++cFileLine;
				}
			}
			
			if (szToken)
			{
				FreeMemory(szToken);
				szToken = NULL;
			}
			continue;
		}
		
		ASSERT(szToken);
		
		// Check for a Remark line
		if (fRemarkLine || ';' == szToken[0])
		{
			FreeMemory(szToken);
			szToken = NULL;
			if (DO_INSTALL == bProcessType)
				fRemarkLine = TRUE;
			continue;
		}
		
		// figure out which token we are processing
		for( iToken = 0; iToken < NUM_CMDTOKENS; iToken++ )
		{
			if( !lstrcmpi( CmdKeywords[iToken].pszKeyword, szToken ) )
			{
				cmdtoken = CmdKeywords[iToken].CmdToken;
				break;
			}
		}
		if (NUM_CMDTOKENS == iToken)
			// we ran off the end of the token list
		{
			Alert(GetWndParent(), MB_ICONEXCLAMATION | MB_OK, STR_ERROR_BOGUSTOKEN, szToken, cFileLine);
			FreeMemory(szToken);
			szToken = NULL;
			nReturn = EBU_ERROR;
			goto Done;
		}
		
		FreeMemory(szToken);
		szToken = NULL;
		
		// we have a valid line so turn off the bInIDBACK flag
		bInIDBACK = FALSE;
		
		
		nReturn = FExecuteReadFileListCommand(hFile, cmdtoken, lpReadFileList,
			lpRuntime, cCommands, prgRuntime, uFirstResID, fFirstTime, bProcessType);
		
		SetResultCode(nReturn);

		switch (nReturn)
		{
		case EBU_ERROR:
			{
				Alert( GetWndParent(), MB_ICONSTOP | MB_OK, STR_ERROR_READFILEERROR, cFileLine);
				goto Done;
			}break;
			
		case EBU_OK:
		case EBU_CANCEL:
			{
				++cFileLine;
			}break;
			
		case EBU_BACK:
			{
				bInIDBACK = TRUE;
				--cFileLine;
			}break;
			
		default:
			// installation was aborted.
			goto Done;
		}
	}//while
	
	nReturn = TOKEN_ERROR != TokenState ? EBU_OK : EBU_ERROR;
	
Done:
	// close the list file
	CloseHandle(hFile);
	hFile = NULL;

	//Clean-up the temporary Readfile
	DeleteFile(szFileName);
	
	return nReturn;
}

//****************************************************************************
// Procedure   ExecuteReadFileListCommand
//
// Purpose     Executes a single ReadFileList setup command.
//
// Parameters  
//             lpInstallFile  pointer to an INSTALLFILE struct describing
//                            the source and destination files.
//				bProcessType  Used to indicate whether this is an INSTALL or UNINSTALL.
//
// Returns     nonzero if successful, zero if setup was aborted.
//
// History     2/24/97 a-drews    Created
//
EBURETCODE FExecuteReadFileListCommand( HANDLE hFile, CMDTOKEN cmdtoken,
	LPREADFILELIST lpReadFileList, LPRUNTIMECOMMAND lpRuntime, WORD cCommands,
	LPRUNTIMECOMMAND prgRuntime, UINT uFirstResID, BOOL fFirstTime, BYTE bProcessType )
{
	EBURETCODE nReturn = EBU_OK;
	if (n_fMaintMode)		// if in maintainence mode skip lines with commands not executed
	{
	  switch(cmdtoken)
	  {
		case TOK_UNINSTALL:
		case TOK_UNINSTALLALL:
		//case TOK_INSTALL:
		case TOK_MKROOT:
		//case TOK_MKDIR:
		//case TOK_INIVALUE:
		case TOK_GETNAME:
		case TOK_INSTFONT:
		//case TOK_INSTICON:
		case TOK_GETPID:
		case TOK_INSTDX:
		case TOK_INSTDPLAY:
		case TOK_CDSPEED:
		//case TOK_DELETEFILE:
		case TOK_REGWIZ:
		case TOK_GETGROUP:
			nReturn = FReadToEOL(hFile);
			return nReturn;
	  }
	}

	switch(cmdtoken)
	{
	default:
	case TOK_UNINSTALLALL:
	case TOK_UNINSTALL:
		ASSERT(FALSE);
		nReturn = EBU_CANCEL;
		break;

//	case TOK_INSTALL:
//		nReturn = FPreExecuteInstallTemplate(hFile, INSTALL_FILE,
//					bProcessType);
//		break;

	case TOK_INSTALLLIST:
		nReturn = FPreExecuteInstallTemplate(hFile, INSTALL_LIST,
					bProcessType);
		break;

	case TOK_INSTALLGO:
		if (DO_INSTALL == bProcessType)
			nReturn = FPreExecuteInstallGo(hFile);
		else
			nReturn = FReadToEOL(hFile);
		break;

	case TOK_CABGO:
		if (DO_INSTALL == bProcessType)
			nReturn = FPreExecuteCabGo(hFile);
		else
			nReturn = FReadToEOL(hFile);
			break;

	case TOK_MKROOT:
		nReturn = FPreExecuteMkRoot(hFile, lpRuntime, cCommands,
				prgRuntime, uFirstResID, fFirstTime, bProcessType);
		break;

	case TOK_MKDIR:
		nReturn = FPreExecuteMkDir(hFile, bProcessType);
		break;

	case TOK_INIVALUE:
		nReturn = FPreExecuteAddIniTemplate(hFile, bProcessType);
		break;

	case TOK_SHELLEXECUTE:
		nReturn = FPreExecuteShellExecute(hFile, bProcessType);
		break;

	case TOK_GETNAME:
		if (DO_INSTALL == bProcessType)
			nReturn = FPreExecuteGetTemplate(hFile, GET_NAME, lpRuntime, cCommands,
				prgRuntime, uFirstResID, fFirstTime );
		else
			nReturn = FReadToEOL(hFile);
		break;

	case TOK_GETGROUP:
		if (DO_INSTALL == bProcessType) 
			nReturn = FPreExecuteGetTemplate(hFile, GET_GROUP, lpRuntime, cCommands,
				prgRuntime, uFirstResID, fFirstTime );
		else
			nReturn = FReadToEOL(hFile);
		break;

	case TOK_INSTICON:
		nReturn = FPreExecuteInstIcon(hFile, bProcessType);
		break;

	case TOK_GETPID:
		if (DO_INSTALL == bProcessType)
			nReturn = FPreExecuteGetPid(hFile);
		else
			nReturn = FReadToEOL(hFile);
		break;

	case TOK_INSTDX:
		if (DO_INSTALL == bProcessType)
			nReturn = FPreExecuteInstDX(hFile);
		else
			nReturn = FReadToEOL(hFile);
		break;
	case TOK_INSTDPLAY:
		if (DO_INSTALL == bProcessType)
			nReturn = FPreExecuteInstDPLAY(hFile);
		else
			nReturn = FReadToEOL(hFile);
		break;

	case TOK_CDSPEED:
		if (DO_INSTALL == bProcessType)
			nReturn = FPreExecuteCDSpeed(hFile);
		else
			nReturn = FReadToEOL(hFile);
		break;

	case TOK_DELETEFILE:
		nReturn = FPreExecuteDeleteFile(hFile);
		break;

	case TOK_REGWIZ:
		if (DO_INSTALL == bProcessType)
		{
			nReturn = FPreExecuteRegWiz(hFile);
		}
		else
		{
		   nReturn = FReadToEOL(hFile);
		}

	    break;
	}

	ForwardMessages();

	ASSERT(EBU_OK == nReturn || EBU_CANCEL == nReturn || EBU_ERROR == nReturn ||
		   EBU_ABORT == nReturn || EBU_BACK == nReturn || EBU_RETRY == nReturn);

	return nReturn;
}

//****************************************************************************
// Procedure	FProcessStringAndPlatformTokens
//
// Purpose		Process N string tokens and then the Platform params
//
// Parameters	hFile			handle to file we're reading from
//				cStrings		Count of string tokens to process
//				pBuf			Buffer to put strings into
//				pdwInstallBuild		Where to set platform flags
//
// Returns		nonzero if successful, zero if error
//
// History		3/26/97 a-drews    Created
//
BOOL FProcessStringAndPlatformTokens(HANDLE hFile, int cStrings, char *pBuf,
	DWORD *pdwInstallBuild)
{
#define MAX_NUMBER_OF_STRINGS 10
	char *rgsz[MAX_NUMBER_OF_STRINGS];
	char *szToken;
	int c = 0;
	char *pch;
	TOKEN_STATE TokenState;

	ASSERT(cStrings <= MAX_NUMBER_OF_STRINGS);

	*pdwInstallBuild = 0;
	do
	{
		if (!FReadToken(hFile, szToken, &TokenState))
			continue;
		ASSERT(szToken);
	
		if (pBuf && c < cStrings)
			// string token
			rgsz[c++] = szToken;
		else
			//Platform Token
			if (!FProcessPlatformToken(&szToken, pdwInstallBuild))
					goto Failed;

	} while (TOKEN_OK == TokenState);

	if (TOKEN_EOL != TokenState || c < cStrings || 0 == *pdwInstallBuild)
		goto Failed;

	pch = pBuf;

	if (pBuf)
	{
		for (c = 0; c < cStrings; c++)
		{
			ASSERT(rgsz[c]);
			lstrcpy(pch, rgsz[c]);
			pch += lstrlen(rgsz[c])+1;
			FreeMemory(rgsz[c]);
			rgsz[c] = NULL;
		}
	}

	return TRUE;

Failed:
	while (c > 0)
	{
		FreeMemory(rgsz[--c]);
		rgsz[--c] = NULL;
	}
	return(FALSE);
}
//****************************************************************************
// Procedure	FProcessDiskKeywords
//
// Purpose		Check Token against valid Disk Keywords for Multi Disk stuff.
//
// Parameters	hFile			File to read from.
//
// Returns     TRUE if token matched the keyword, FALSE otherwise.
//
// History      6/09/98		a-petere - created
//
BOOL FProcessDiskKeywords(char **pszToken, char *szDiskID)
{
	BOOL fFoundDiskID =FALSE;
	int nDiskID=0;

	// Check DiskID Keywords
	for (nDiskID = DISK_01; nDiskID <= MAX_DISK_LABELS; nDiskID++)
	{
		if (!lstrcmpi(*pszToken, g_DiskKeywords[nDiskID].pszKeyword))
		{
			fFoundDiskID = TRUE;
			break;
		}
	}
	if (fFoundDiskID)
	{
		//Consume the Token
		FreeMemory(*pszToken);
		*pszToken = NULL;
		itoa(nDiskID,szDiskID,10);
		return TRUE;
	}
	else
	{	
		//Default to DISK_01
		itoa(DISK_NOT_SPECIFIED,szDiskID,10);
		return FALSE;
	}
}




//****************************************************************************
// Procedure	FReadToken
//
// Purpose		Call ReadToken. If error (ie !TOKEN_OK || !TOKEN_EOL) free szToken
//				and return FALSE;
//
// Parameters	hFile			handle to file we're reading from
//				PToken			string from file placed here.
//				pTokenState		TokenState placed here
//
// Returns		nonzero if successful, zero if error
//
// History		3/24/97 a-drews    Created
//
BOOL FReadToken(HANDLE hFile, char *(&PToken), TOKEN_STATE *pTokenState)
{
	*pTokenState = ReadToken(hFile, PToken);
	if (TOKEN_OK != *pTokenState && TOKEN_EOL != *pTokenState)
	{
		if (PToken)
		{
			FreeMemory(PToken);
			PToken = NULL;
		}
		return(FALSE);
	}
	else
		return(TRUE);
}

TOKEN_STATE ReadToken(HANDLE hFile, char *(&PToken))
{

	ASSERT(hFile);

	int
		iCurTokenPos = 0;
	ULONG
		ulBytesRead = 0;

	BOOL
		bDone = FALSE,
		bTemp = TRUE,
		bInQuotes = FALSE,
		bStartedToken = FALSE,
		fEatingWhiteSpace = FALSE;

	TOKEN_STATE
		eRetVal = TOKEN_NONE;

	char
		szCurrentLine[2048] = "";

	// read a token in
	do
	{
		bTemp = ReadFile(hFile, (LPVOID) (szCurrentLine + iCurTokenPos), 1, &ulBytesRead, NULL);

		if (!ulBytesRead)
		{
			eRetVal = TOKEN_EOF;
			continue;
		}

		if ( IsDBCS() && bTemp )
		{
			// MEMO : Trapping all DBCS characters.
			if ( IsDBCSLeadByte( szCurrentLine[iCurTokenPos] ) ){
				// we have a real char here
				bStartedToken = TRUE;

				iCurTokenPos++;

				bTemp = ReadFile(hFile, (LPVOID) (szCurrentLine + iCurTokenPos), 1, &ulBytesRead, NULL);

				if (!ulBytesRead)
				{
					eRetVal = TOKEN_EOF;
					continue;
				}
				if ( bTemp ){
					iCurTokenPos++;
					continue; // Trap until SBCS came.
				}
			}
		}

		if (!bTemp)
		{
			// some sort of read error
			bDone = TRUE;

			DisplaySystemError(GetLastError(), MB_OK | MB_ICONSTOP);

			eRetVal = TOKEN_ERROR;
		}

		if (9 == szCurrentLine[iCurTokenPos] || 32 == szCurrentLine[iCurTokenPos])
		// a space or tab
		{
			if (!bStartedToken) // leading space
			{
				continue;
			}

			if (!bInQuotes)
			{
				// a non quoted space or tab marks the end of a token
				eRetVal = TOKEN_OK;
				// don't leave the loop until all the whitespace after a token has been
				// eaten
				fEatingWhiteSpace = TRUE;
				continue;
			}
		}

		if (13 == szCurrentLine[iCurTokenPos])
		{
			fEatingWhiteSpace = FALSE;
			eRetVal = TOKEN_EOL;
			continue;
		}

		if (fEatingWhiteSpace)
		{
			// we've found a non-whitespace char which isn't a EOL so push the file pointer
			// back one char a exit the loop
			SetFilePointer(hFile, -1, NULL, FILE_CURRENT);
			fEatingWhiteSpace = FALSE;
			continue;
		}

		if ('\"' == szCurrentLine[iCurTokenPos])
		{
			bInQuotes = !bInQuotes;
			continue;
		}

		if (!isprint(szCurrentLine[iCurTokenPos])) // eat control chars (LF, TAB, ETC.)
			continue;

		// we have a real char here
		bStartedToken = TRUE;

		iCurTokenPos++;

	}while (TOKEN_NONE == eRetVal || fEatingWhiteSpace);

	// kill the last space or cr read in
	szCurrentLine[iCurTokenPos] = NULL;
	if (FNewMemory((void **)&PToken, lstrlen(szCurrentLine)+1))
		lstrcpy(PToken, szCurrentLine);
	else
	// Out of Memory
	{
		Alert( GetWndParent(), MB_ICONSTOP | MB_OK, STR_ERROR_NOMEMORY );
		eRetVal = TOKEN_ERROR;
	}
	return eRetVal;


}

//****************************************************************************
// Procedure	FProcessPlatformToken
//
// Purpose		Set OS, platform, IME, etc. flags based on string
//
// Parameters	szToken			OS or IME flag
//				pdwInstallBuild		Flags to set.
//
// Returns     TRUE if we see a platform token we expect, FALSE otherwise.
//
// History      3/24/97		a-drews - created
//
BOOL FProcessPlatformToken(char **pszToken, DWORD *pdwInstallBuild)
{
	BOOL fReturn = FALSE;
	static CSetupCommand command;

	//
	//Reset command flags...
	//
	SetCommandFlags(0);

	struct
	{
		TCHAR *pszBuildFlag;
		DWORD dwBuildFlag;
	} sOSList[] = {"WIN95",     OS_WIN95,
				   "WIN98",     OS_WIN98,
				   "ALLWIN",    OS_WIN95 | OS_WIN98,
				   "NT40",      OS_NT40,
				   "NT50",      OS_NT50,
				   "ALLNT",     OS_NT40 | OS_NT50,
				   "DBCS",		BLD_DBCS,
				   "OEM",		BLD_OEM,
				   "RTL",		BLD_RTL,
				   "JPN",		BLD_JPN,
				   "GER",		BLD_GER,
				   "FRA",		BLD_FRA,
				   "SPA",		BLD_SPA,
				   "USA",		BLD_USA,
				   "APP1",		BLD_APP1,
				   "IMEENABLE", SCF_IME_ENABLE,
				   "IMEON",     SCF_IME_ON,
				   "",			0xFFFFFFFF};

	for (int nIdx = 0; 0xFFFFFFFF != sOSList[nIdx].dwBuildFlag; nIdx++)
	{
		if (0 == lstrcmpi(*pszToken, sOSList[nIdx].pszBuildFlag))
		{
			fReturn = TRUE;

			*pdwInstallBuild |= sOSList[nIdx].dwBuildFlag;

			break;
		}
	}

	FreeMemory(*pszToken);
	*pszToken = NULL;

	SetCommandFlags(*pdwInstallBuild);

	return fReturn;
}
//****************************************************************************
// Procedure	FReadToEOL
//
// Purpose		Eat all the token until we hit an EOL
//
// Parameters	hFile			File to read from.
//
// Returns     TRUE if we reach the EOL with no errors, FALSE otherwise.
//
// History      3/27/97		a-drews - created
//
EBURETCODE FReadToEOL(HANDLE hFile)
{
	TOKEN_STATE TokenState;
	char *szToken = NULL;

	do
	{
		TokenState = ReadToken(hFile, szToken);

		if (szToken)
		{
			FreeMemory(szToken);
			szToken = NULL;
		}
	} while (TOKEN_OK == TokenState);

	return TOKEN_EOL == TokenState ? EBU_OK : EBU_ERROR;
}

//****************************************************************************
// Procedure	FBackupOneLine
//
// Purpose		Starting from *pwFilePos. Read backwards one line.
//
// Parameters	hFile			File to read from.
//				*pwFilePos		File position to start from (EOF if -1).
//
// Returns     TRUE if we reach the EOL with no errors, FALSE otherwise.
//
// History      4/3/97		a-drews - created
//
EBURETCODE FBackupOneLine(HANDLE hFile, int *pwFilePos)
{
	char ch;
	DWORD bRead;

	// set file pointer to last known start of line
	if (-1 == *pwFilePos)
		SetFilePointer(hFile, -1, NULL, FILE_END);
	else
		SetFilePointer(hFile, *pwFilePos, NULL, FILE_BEGIN);
	//can't backup past the beginning of the file
	if (0 == *pwFilePos)
		return EBU_ERROR;
	//backup 2 chars to get past the EOL
	if (SetFilePointer(hFile, -2, NULL, FILE_CURRENT) < 0)
		// Hit the begginning of the file
		return EBU_ERROR;
	do
	{
		if (!ReadFile(hFile, &ch, 1, &bRead, NULL))
			return EBU_ERROR;

		if (13 == ch)
		{
			// Found the end of the previous line
			*pwFilePos = SetFilePointer(hFile, 0, NULL, FILE_CURRENT);
			return EBU_OK;
		}

		if (0xFFFFFFFF == SetFilePointer(hFile, -2, NULL, FILE_CURRENT))
		{
			// ran into the front of the file
			*pwFilePos = SetFilePointer(hFile, 0, NULL, FILE_BEGIN);
			return EBU_OK;
		}
	} while (TRUE);
}

//****************************************************************************
// Procedure	FPreExecuteInstallTemplate
//
// Purpose		Setup data strucs to call ExecuteInstallList or ExecuteInstallFile.
//				Data is read at runtime from a text file.
//
// Parameters	
//				hFile			handle to file we're reading from
//				bInstallType	INSTALL_FILE or INSTALL_LIST
//				bProcessType	INSTALL || UNINSTALL
//
// Returns		nonzero if successful, zero if setup was aborted.
//
// History		2/24/97 a-drews    Created
//
EBURETCODE FPreExecuteInstallTemplate(HANDLE hFile, BYTE bInstallType,
	BYTE bProcessType )
{
	; // I am not sure why but the compiler is choking on the enum if I don't have this ; here
	enum LINE_STATES {LOC_MODIFIER=0, INSTALL_FLAG, DEST_NAME, SOURCE_NAME, DISK_ID , FILE_GROUP, PLATFORM};
	LINE_STATES LineState = LOC_MODIFIER;
	char *szToken = NULL;
	char *szUninstallModifier = NULL;
	char *szDestName = NULL;
	char *szSourceName = NULL;
	char *szFileGroup = NULL;
	
	char szDiskID[128];
	int	nDiskID=DISK_NOT_SPECIFIED;

	TOKEN_STATE TokenState = TOKEN_NONE;
	DWORD dwInstallBuild = 0;
	CInstallFile *pInstallFile = new CInstallFile();
	EBURETCODE nReturn = EBU_OK;
	BOOL bSystem;
	static BOOL bFirstInstallListFile=TRUE;

	ASSERT(INSTALL_LIST == bInstallType);
	ASSERT(DO_INSTALL == bProcessType || DO_UNINSTALL == bProcessType); 		

	do
	{
		if (!FReadToken(hFile, szToken, &TokenState))
			continue;
		
		ASSERT(szToken);
		
Skipped:		
		switch (LineState)
		{
		default:
			ASSERT(FALSE);
			break;

		case LOC_MODIFIER:
			if (!strcmpi(szToken, "app")) // application directory
			{
				pInstallFile->wFlags |= IF_APPDIR;
				LineState = (LINE_STATES)((int)LineState + 1);
				goto FoundFlag;
			}
			if (!strcmpi(szToken, "WINDOWS")) // application directory
			{
				pInstallFile->wFlags |= IF_WINDOWSDIR;
				LineState = (LINE_STATES)((int)LineState + 1);
				goto FoundFlag;
			}
			if (!strcmpi(szToken, "SYSTEM"))
			{
				pInstallFile->wFlags |= IF_SYSTEMDIR;
				LineState = (LINE_STATES)((int)LineState + 1);
				goto FoundFlag;
			}

			LineState = (LINE_STATES)((int)LineState + 1);
			goto Skipped;
			break;
			
		case INSTALL_FLAG:
			//Look for optional arguments
			if (!lstrcmpi(szToken, "UnInstall"))
			{
				pInstallFile->wFlags |= IF_UNINSTALL;
				goto FoundFlag;
			}
			else if (!lstrcmpi(szToken, "Shared"))
			{
				pInstallFile->wFlags |= IF_SHAREDFILE;
				goto FoundFlag;
			}
			else if (!lstrcmpi(szToken, "DLLRegister"))
			{
				pInstallFile->wFlags |= IF_DLLREGISTER;
				goto FoundFlag;
			}
			else if (!lstrcmpi(szToken, "cab"))
			{
				pInstallFile->wFlags |= IF_CAB;
				goto FoundFlag;
			}
			else if (!lstrcmpi(szToken, "Font"))
			{
				pInstallFile->wFlags |= IF_FONTFILE;
				goto FoundFlag;
			}
			else if (!lstrcmpi(szToken, "UninstallOnly"))
			{
				pInstallFile->wFlags |= IF_UNINSTONLY;
				goto FoundFlag;
			}
			// didn't find a valid flag so we're gonna move to next test state DEST_NAME.
			LineState = (LINE_STATES)((int)LineState + 1);
			goto Skipped;

FoundFlag:
			FreeMemory(szToken);
			szToken = NULL;
			// notice the LineState is not changed. We keep looking for flags as long as
			// we see a valid one.
			break;
//DestName:
		case DEST_NAME:
			// now the destination name
			szDestName = szToken;
			LineState = (LINE_STATES)((int)LineState + 1);
			break;
			
		case SOURCE_NAME:
			// now the source name
			szSourceName = szToken;
			LineState = (LINE_STATES)((int)LineState + 1);
			break;
			
		case DISK_ID:
			//Set to next LineState and check for new token required.
			LineState = (LINE_STATES)((int)LineState + 1);
			// Check DiskID Keywords
			if (!FProcessDiskKeywords(&szToken, szDiskID))
			{
				goto Skipped;  //didn't consume the token.
			}
			break;			

		case FILE_GROUP:
			// now the file group
			szFileGroup = szToken;
			LineState = (LINE_STATES)((int)LineState + 1);
			break;
		
		case PLATFORM:
			// now the platform flags until end of line
			if (!FProcessPlatformToken(&szToken, &dwInstallBuild))
			{
				nReturn = EBU_ERROR;
				goto Done;
			}	
			break;
		}
	} while (TOKEN_OK == TokenState);

	if (TOKEN_EOL != TokenState )
	{
		nReturn = EBU_ERROR;
		goto Done;
	}

	nReturn = EBU_OK;

	// call ExecuteInstallList for this file if it is supposed to be installed on the curent os
	if (ExecOnThisPlatform(dwInstallBuild))
	{
		if (DO_INSTALL == bProcessType)
		// only need version info if we're doing an install
		{		
			char
				szTempFileName[MAX_PATH],
				szExecDir[MAX_PATH],
				*szTempSource = szSourceName;
			
			GetModuleDirectory( szExecDir, sizeof(szExecDir) );

			// make sure the source name doesn't have a leading '\'
			if ('\\' == szTempSource[0])
				szTempSource++;
				wsprintf(szTempFileName, "%s%s", szExecDir, szTempSource);
				ReplaceStringTokens(szTempFileName, _MAX_PATH);

			ASSERT(szSourceName);
			// get the file size, it's needed for copy gauges, and for size calculations
			struct _stat st;

			// Build an InstallList structure to send to EnsureCDRomInserted
			// It only needs the diskID and the sourcefilename, so that alternate network paths can be 
			// searched for file info.
			INSTALLLIST ilMultiCD;
			ilMultiCD.szSource=szTempFileName;
			ilMultiCD.nDiskID = atoi(szDiskID);
			if (!EnsureCDROMInserted(&ilMultiCD))
			{
				nReturn = EBU_ABORT;
				goto Done;
			}
			
			if(_stat(szTempFileName,&st) == 0)
				pInstallFile->FileInfo.dwFileSize = (DWORD)st.st_size;

		   bSystem =	pInstallFile->fCopyToWindowsDir()  || pInstallFile->fCopyToSystemDir() ||
						pInstallFile->fIsSystemFile() ||
						pInstallFile->fIsSharedFile();

			// get the file info
		   if (bSystem)
		   {
				// We only care about the file info for system files.
			   UINT
				uResult = EBUFileInfo(szTempFileName, &pInstallFile->FileInfo);

				if( uResult & FI_ERRORMASK )
				{
					if (pInstallFile->wFlags & IF_CAB)
						goto CopyFile;
					else if( uResult & FI_ERR_NOEXIST)
						Alert( GetWndParent(), MB_ICONSTOP | MB_OK, STR_ERROR_FILENOTFOUND, (LPCSTR)szSourceName);
					else if( uResult & FI_ERR_CANTOPEN )
						Alert( GetWndParent(), MB_ICONSTOP | MB_OK, STR_ERROR_CANTOPENSOURCE, (LPCSTR)szSourceName);
					else
						Alert( GetWndParent(), MB_ICONSTOP | MB_OK, STR_ERROR_NOMEMORY );
					goto Done;
				}
		   }
		}
CopyFile:

		// build the path string which includes the source name, destination name, and the group id, diskid
		ASSERT(szSourceName);
		ASSERT(szDestName);
		ASSERT(lstrlen(szSourceName)+1 + lstrlen(szDestName)+1 + lstrlen(szFileGroup)+1 <= MAX_DATA_LENGTH);

		//
		//Allow substitution tokens in source file names
		//
		lstrcpy(pInstallFile->szName, szSourceName);
		ReplaceStringTokens(pInstallFile->szName, _MAX_PATH);

		pInstallFile->wDestOffset = lstrlen(pInstallFile->szName) + sizeof(TCHAR);

		//
		//4 null terminated strings, sourcename, destname, filegroup, diskid...
		//
		if (bFirstInstallListFile && bInstallType == INSTALL_LIST)
		{
			bFirstInstallListFile = FALSE;
			// If there isn't a explicit disk_XX for the first readfile list command set it to disk_01
			// This will make the second pass of the filename through EnsureCDROMInserted benign.
			// Without the explicit disk_01 at the beginning the installgo pass will not get reset to the implied
			// disk_01 on startup.  If there is an explicit disk then nothing happens.
			if ( !lstrcmpi(szDiskID, "-1") )
			{

				wsprintf(pInstallFile->szName, "%s%c%s%c%s%c%s",
						 pInstallFile->szName,
						 '\0',
						 szDestName,
						 '\0',
						 szFileGroup,
						 '\0',
						 "0");
			}
			else
			{
				wsprintf(pInstallFile->szName, "%s%c%s%c%s%c%s",
						 pInstallFile->szName,
						 '\0',
						 szDestName,
						 '\0',
						 szFileGroup,
						 '\0',
						 szDiskID);
			}
		}
		else
		{
			wsprintf(pInstallFile->szName, "%s%c%s%c%s%c%s",
					 pInstallFile->szName,
					 '\0',
					 szDestName,
					 '\0',
					 szFileGroup,
					 '\0',
					 szDiskID);
		}

		if (DO_INSTALL == bProcessType)
		{
			nReturn = ExecuteInstallList(pInstallFile);
		}
		else
			nReturn = ExecuteUnInstallFile(pInstallFile);
	}
	else
		nReturn = GetResultCode();	// use whatever the last code was

Done:
	if (szDestName)
		FreeMemory(szDestName);
	if (szSourceName)
		FreeMemory(szSourceName);
	if (szFileGroup)
		FreeMemory(szFileGroup);

	delete pInstallFile;
	return nReturn;
}

//****************************************************************************
// Procedure	FPreExecuteInstallGo
//
// Purpose		Setup data strucs to call ExecuteInstallGo. Data is read at runtime from
//				a text file.
//
// Parameters	
//				hFile			handle to file we're reading from
//
// Returns		nonzero if successful, zero if setup was aborted.
//
// History		2/25/97 a-drews    Created
//
EBURETCODE FPreExecuteInstallGo(HANDLE hFile )
{
	EBURETCODE nReturn = EBU_OK;
	DWORD dwInstallBuild = 0;

	if (!FProcessStringAndPlatformTokens(hFile, 0, NULL, &dwInstallBuild))
	{
		nReturn = EBU_ERROR;
		goto Done;
	}

	if (ExecOnThisPlatform(dwInstallBuild))
		nReturn = ExecuteInstallGo();
	else
		nReturn = GetResultCode();	// use whatever the last code was

Done:
	return nReturn;
}
//****************************************************************************
// Procedure	FPreExecuteCabGo
//
// Purpose		Setup data strucs to call ExecuteCabGo. Data is read at runtime from
//				a text file.
//
// Parameters	
//				hFile			handle to file we're reading from
//
// Returns		nonzero if successful, zero if setup was aborted.
//
// History		2/25/97 a-drews    Created
//
EBURETCODE FPreExecuteCabGo(HANDLE hFile )
{
	EBURETCODE nReturn = EBU_OK;
	CCabGo *pCabGo = new CCabGo();
	enum LINE_STATES {CAB_NAME=0, PLATFORM};
	LINE_STATES LineState = CAB_NAME;
	char *szToken;
	TOKEN_STATE TokenState;
	DWORD dwInstallBuild = 0;

	ZeroMemory(pCabGo->szName, MAX_DATA_LENGTH);

	do
	{
		if (!FReadToken(hFile, szToken, &TokenState))
			continue;
		
		ASSERT(szToken);
		
		switch (LineState)
		{
		default:
			ASSERT(FALSE);
			break;

		case CAB_NAME:
			ASSERT(MAX_DATA_LENGTH > lstrlen(szToken));
			lstrcpy(pCabGo->szName, szToken);
			LineState = PLATFORM;
			FreeMemory(szToken);
			szToken = NULL;
			break;
			
		case PLATFORM:
			// now the platform flags until end of line
			if (!FProcessPlatformToken(&szToken, &dwInstallBuild))
			{
				nReturn = EBU_ERROR;
				goto Done;
			}	

			break;
		}
	} while (TOKEN_OK == TokenState);
	
	if (TOKEN_EOL == TokenState)
	{
		if (ExecOnThisPlatform(dwInstallBuild))
				nReturn = ExecuteCabGo(pCabGo);
	else
		nReturn = GetResultCode();	// use whatever the last code was
	}
	else
		nReturn = EBU_ERROR;

Done:
	delete pCabGo;

	return nReturn;
}

//****************************************************************************
// Procedure	FPreExecuteMkRoot
//
// Purpose		Setup data strucs to call ExecuteMkRoot. Data is read at runtime from
//				a text file.
//
// Parameters	
//				hFile			handle to file we're reading from
//				rest of params are passed down to ExecuteMkRoot
//
// Returns		nonzero if successful, zero if setup was aborted.
//
// History		2/27/97 a-drews    Created
//
EBURETCODE FPreExecuteMkRoot(HANDLE hFile, LPRUNTIMECOMMAND lpRuntime,
	WORD cCommands, LPRUNTIMECOMMAND prgRuntime,UINT uFirstResID, BOOL fFirstTime,
	BYTE bProcessType )
{
	enum LINE_STATES {UNINSTALL_FLAG=0, PLATFORM};
	LINE_STATES LineState = UNINSTALL_FLAG;
	EBURETCODE nReturn = EBU_OK;
	char *szToken;
	TOKEN_STATE TokenState;
	CMkRoot *pMkRoot = new CMkRoot();
	DWORD dwInstallBuild = 0;

	ASSERT(DO_INSTALL == bProcessType || DO_UNINSTALL == bProcessType);

	do
	{
		if (!FReadToken(hFile, szToken, &TokenState))
			continue;
		
		ASSERT(szToken);
		
		switch (LineState)
		{
		default:
			ASSERT(FALSE);
			break;

		case UNINSTALL_FLAG:
			if (!strcmpi(szToken, "UnInstall"))
			{
				// set the uninstall flag
				pMkRoot->wFlags |= IF_UNINSTALL;
				LineState = PLATFORM;
				FreeMemory(szToken);
				szToken = NULL;

				break;
			}

			//
			// we will now fall thorugh to PLATFORM
			//
						
		case PLATFORM:
			// now the platform flags until end of line
			if (!FProcessPlatformToken(&szToken, &dwInstallBuild))
			{
				nReturn = EBU_ERROR;
				goto Done;
			}	
			break;
		}
	} while (TOKEN_OK == TokenState);

	if (TOKEN_EOL == TokenState)
	{
		if (ExecOnThisPlatform(dwInstallBuild))
			if (DO_INSTALL == bProcessType)
				nReturn = ExecuteMkRoot(pMkRoot, lpRuntime, cCommands,
					prgRuntime, uFirstResID, fFirstTime);
			else
				nReturn = ExecuteRdRoot(pMkRoot);
		else
			nReturn = GetResultCode();	// use whatever the last code was
	}
	else
		nReturn = EBU_ERROR;

Done:
	delete pMkRoot;

	return nReturn;
}

//****************************************************************************
// Procedure	FPreExecuteMkDir
//
// Purpose		Setup data strucs to call ExecuteMkDir. Data is read at runtime from
//				a text file.
//
// Parameters	
//				hFile			handle to file we're reading from
//
// Returns		nonzero if successful, zero if setup was aborted.
//
// History		2/27/97 a-drews    Created
//
EBURETCODE FPreExecuteMkDir(HANDLE hFile, BYTE bProcessType )
{
	enum LINE_STATES {UNINSTALL_FLAG=0, DIR_NAME, FILE_GROUP, PLATFORM};
	LINE_STATES LineState = UNINSTALL_FLAG;
	EBURETCODE nReturn = EBU_OK;
	char *szToken = NULL;
	char *szDirName = NULL;
	char *szFileGroup = NULL;
	TOKEN_STATE TokenState;
	CMkDir *pMkDir = new CMkDir();
	DWORD dwInstallBuild = 0;
	
	ASSERT(DO_INSTALL == bProcessType || DO_UNINSTALL == bProcessType);

	do
	{
		if (!FReadToken(hFile, szToken, &TokenState))
			continue;
		
		ASSERT(szToken);
		
		switch (LineState)
		{
		default:
			ASSERT(FALSE);
			break;

		case UNINSTALL_FLAG:
			if (!strcmpi(szToken, "UnInstall"))
			{
				// set the uninstall flag
				pMkDir->wFlags |= IF_UNINSTALL;
				LineState = (LINE_STATES)((int)LineState + 1);
				FreeMemory(szToken);
				szToken = NULL;
				break;
			} 
			else if (!strcmpi(szToken, "UnInstall_All"))
			{
				// set the uninstall flag
				pMkDir->wFlags |= IF_UNINSTALLALL;
				LineState = (LINE_STATES)((int)LineState + 1);
				FreeMemory(szToken);
				szToken = NULL;
				break;
			} 

			//
			//we will now fall thorugh to DIR_NAME
			//
			
		case DIR_NAME:
			szDirName = szToken;
			LineState = FILE_GROUP;
			break;

		case FILE_GROUP:
			// now the file group
			szFileGroup = szToken;
			LineState = (LINE_STATES)((int)LineState + 1);
			break;

		case PLATFORM:
			// now the platform flags until end of line
			if (!FProcessPlatformToken(&szToken, &dwInstallBuild))
			{
				nReturn = EBU_ERROR;
				goto Done;
			}	
			break;
		}
	} while (TOKEN_OK == TokenState);
	
	if (TOKEN_EOL != TokenState)
	{
		nReturn = EBU_ERROR;
		goto Done;
	}

	if (ExecOnThisPlatform(dwInstallBuild))
	{
		// build the name string which includes the directory name, and group id
		ASSERT(szDirName);
		ASSERT(szFileGroup);
		ASSERT(lstrlen(szDirName)+1 + lstrlen(szFileGroup)+1 <= MAX_DATA_LENGTH);
		wsprintf(pMkDir->szName, "%sX%s", szDirName, szFileGroup);

		// chop szName into 2 null terminated strings
		pMkDir->szName[lstrlen(szDirName)] = NULL;

		if (DO_INSTALL == bProcessType)
			nReturn = ExecuteMkDir(pMkDir);
		else
			nReturn = ExecuteRdDir(pMkDir);
	}
	else
		nReturn = GetResultCode();	// use whatever the last code was


Done:
	if (szDirName)
		FreeMemory(szDirName);
	if (szFileGroup)
		FreeMemory(szFileGroup);

	delete pMkDir;

	return nReturn;
}

//****************************************************************************
// Procedure	FPreExecuteAddIniTemplate
//
// Purpose		Setup data strucs to call ExecuteAddIniValue
//				Data is read at runtime from a text file.
//
// Parameters	
//				hFile			handle to file we're reading from
//				bDataType		INI_STRING or INI_DWORD
//
// Returns		nonzero if successful, zero if setup was aborted.
//
// History		3/17/97 a-drews    Created
//
EBURETCODE FPreExecuteAddIniTemplate(HANDLE hFile, BYTE bProcessType)
{
	enum LINE_STATES {UNINSTALL_FLAG=0, MAP_FLAG, FILE_NAME, SECTION_NAME, KEY_NAME, VALUE, DATA_TYPE, INSTALL_GROUP, PLATFORM};
	LINE_STATES LineState = UNINSTALL_FLAG;
	EBURETCODE nReturn = EBU_OK;
	char *szToken = NULL;
	char *szFileName = NULL;
	char *szSectionName = NULL;
	char *szKeyName = NULL;
	char *szValue = NULL;
	char *szType = NULL;
	char *szInstallGroup = NULL;
	DWORD dwType;
	char  cUninstall = '0';
	TOKEN_STATE TokenState;
	CAddIniValue *pAddIniValue = new CAddIniValue();
	DWORD dwInstallBuild = 0;
	char *pch = NULL;

	ASSERT(DO_INSTALL == bProcessType || DO_UNINSTALL == bProcessType);

	do
	{
		if (!FReadToken(hFile, szToken, &TokenState))
			continue;
		
		ASSERT(szToken);
		
		switch (LineState)
		{
		default:
			ASSERT(FALSE);
			break;

			case UNINSTALL_FLAG:
				if (0 == lstrcmpi(szToken, "uninstall"))
				{
					cUninstall = '1';
					LineState = MAP_FLAG;
					FreeMemory(szToken);
					szToken = NULL;
					break;
				}
				else if (0 == lstrcmpi(szToken, "uninstall_all"))
				{
					cUninstall = '2';
					LineState = MAP_FLAG;
					FreeMemory(szToken);
					szToken = NULL;
					break;
				}

				//
				//else fall through to MAP_FLAG...
				//

			case MAP_FLAG:
				if (!lstrcmpi(szToken, "map"))
				{
					// if "map" appears then FILE_NAME is omitted
					LineState = SECTION_NAME;
					FreeMemory(szToken);
					szToken = NULL;
					break;
				} else
				{
					// we will now fall thorugh to FILE_NAME
				}
				
		case FILE_NAME:
			szFileName = szToken;
			LineState = SECTION_NAME;
			break;

		case SECTION_NAME:
			szSectionName = szToken;
			LineState = (LINE_STATES)((int)LineState + 1);
			break;

		case KEY_NAME:
			szKeyName = szToken;
			LineState = (LINE_STATES)((int)LineState + 1);
			break;

		case VALUE:
			szValue = szToken;
			LineState = (LINE_STATES)((int)LineState + 1);
			break;

		case DATA_TYPE:
			szType = szToken;
			LineState = (LINE_STATES) ((int) LineState + 1);
			break;

		case INSTALL_GROUP:
			// now the install group
			szInstallGroup = szToken;
			LineState = (LINE_STATES)((int)LineState + 1);
			break;
			
		case PLATFORM:
			// now the platform flags until end of line
			if (!FProcessPlatformToken(&szToken, &dwInstallBuild))
			{
				nReturn = EBU_ERROR;
				goto Done;
			}	
			break;
		}
	} while (TOKEN_OK == TokenState);
	
	if (TOKEN_EOL != TokenState)
	{
		nReturn = EBU_ERROR;
		goto Done;
	}

	if (ExecOnThisPlatform(dwInstallBuild))
	{
		// build the name string which may includes the filename and always includes the
		//	section name and key name
		ASSERT(szSectionName);
		ASSERT(szKeyName);
		ASSERT(szValue);
		ASSERT(szType);
		ASSERT(szInstallGroup);

		ASSERT(((NULL != szFileName) ? lstrlen(szFileName)+1 : 0) + lstrlen(szSectionName)+1
			+ lstrlen(szKeyName)+1 + lstrlen(szValue)+1 + lstrlen(szInstallGroup)+1 + sizeof(DWORD) + 1 <= MAX_DATA_LENGTH);
		pch = pAddIniValue->szData;
		//Add filename if it exists
		if (szFileName)
		{
			lstrcpy(pch, szFileName);
			pAddIniValue->wSectionOffset = lstrlen(szFileName)+1;
			pch += pAddIniValue->wSectionOffset;
		}
		//Add section name
		lstrcpy(pch, szSectionName);
		pch += lstrlen(szSectionName)+1;
		//Add Key Name
		pAddIniValue->wKeyOffset = pAddIniValue->wSectionOffset + lstrlen(szSectionName)+1;
		lstrcpy(pch, szKeyName);
		pch += lstrlen(szKeyName)+1;
		//Add Value
		pAddIniValue->wValueOffset = pAddIniValue->wKeyOffset + lstrlen(szKeyName)+1;
		lstrcpy(pch, szValue);
		pch += lstrlen(szValue)+1;
		//Add Data Type
		pAddIniValue->wTypeOffset = pAddIniValue->wValueOffset + lstrlen(szValue)+1;
		
		//
		//Determine and store the type of data being written
		//
		if (0 == lstrcmpi(szType, "REG_DWORD"))
		{
			dwType = REG_DWORD;
		}
		else if (0 == lstrcmpi(szType, "REG_SZ"))
		{
			dwType = REG_SZ;
		}
		else if (0 == lstrcmpi(szType, "REG_EXPAND_SZ"))
		{
			dwType = REG_EXPAND_SZ;
		}
		else
		{
			//
			//The following are all codes that flag a REG_BINARY type.  The string
			//passed in is simply used to set the size of the binary data being
			//written... The size is written as a negative number to avoid conflict
			//with the standard REG_* types...
			//
			if (0 == lstrcmpi("DWORD", szType))
			{
				dwType = (DWORD) -((LONG) sizeof(DWORD));
			}
			else if (0 == lstrcmpi("INT", szType))
			{
				dwType = (DWORD) -((LONG) sizeof(int));
			}
			else if (0 == lstrcmpi("BYTE", szType))
			{
				dwType = (DWORD) -((LONG) sizeof(BYTE));
			}
			else if (0 == lstrcmpi("STRLEN", szType))
			{
				//
				//lstrlen szValue is correct because we're setting type
				//equal to the length of the value string
				//
				dwType = (DWORD) -lstrlen(szValue);
			}
			else
			{
				//
				//Assume that the size was passed in as a hard-coded number...
				//
				dwType = (DWORD) -atol(szType);
			}
		}

		*((DWORD *) pch) = dwType;
		pch += sizeof(DWORD);

		pAddIniValue->wUninstallOffset = pAddIniValue->wTypeOffset + sizeof(DWORD);
		*pch = cUninstall;
		pch += sizeof('1');

		pAddIniValue->wGroupOffset = pAddIniValue->wUninstallOffset + sizeof('1');
		lstrcpy(pch, szInstallGroup);

		nReturn = ExecuteAddIniValue(pAddIniValue, bProcessType);
	}
	else
		nReturn = GetResultCode();	// use whatever the last code was


Done:
	if (szFileName)
		FreeMemory(szFileName);
	if (szSectionName)
		FreeMemory(szSectionName);
	if (szKeyName)
		FreeMemory(szKeyName);
	if (szValue)
		FreeMemory(szValue);
	if (szType)
		FreeMemory(szType);
	if (szInstallGroup)
		FreeMemory(szInstallGroup);
	delete pAddIniValue;

	return nReturn;
}

//****************************************************************************
// Procedure	FPreExecuteShellExecute
//
// History		5/26/98 reizen    Created
//
EBURETCODE FPreExecuteShellExecute(HANDLE hFile, BYTE bProcessType)
{
	enum LINE_STATES {FLAGS=0, FILE_NAME, DIR_NAME, PARAMETERS, SHOWFLAGS, INSTALL_GROUP, PLATFORM};
	LINE_STATES LineState = FLAGS;
	EBURETCODE nReturn = EBU_OK;
	char *szToken = NULL;
	char *szFileName = NULL;
	char *szDirectoryName = NULL;
	char *szParameters = NULL;
	char *szShowFlags = NULL;
	char *szInstallGroup = NULL;

	BOOL fWait = FALSE;

	TOKEN_STATE TokenState;

	CShellExecute *pShellExecute = new CShellExecute();
	DWORD dwInstallBuild = 0;
	char *pch = NULL;

	ASSERT(DO_INSTALL == bProcessType || DO_UNINSTALL == bProcessType);
		
	do
	{
		if (!FReadToken(hFile, szToken, &TokenState))
			continue;
		
		ASSERT(szToken);

		switch (LineState)
		{
		default:
			ASSERT(FALSE);
			break;

		case FLAGS:
			if (0 == lstrcmpi(szToken, "uninstall"))
			{
				pShellExecute->wFlags |= IF_UNINSTALL;
				FreeMemory(szToken);
				szToken = NULL;
				break;
			}
			else
			{
				if (0 == lstrcmpi(szToken, "wait"))
				{
					fWait = TRUE;
					FreeMemory(szToken);
					szToken = NULL;
					break;
				}
			}

			//
			//Fall through to FILE_NAME if no flags found...
			//

		case FILE_NAME:
			szFileName = szToken;
			LineState = DIR_NAME;
			break;

		case DIR_NAME:
			szDirectoryName = szToken;
			LineState = (LINE_STATES)((int)LineState + 1);
			break;

		case PARAMETERS:
			szParameters = szToken;
			LineState = (LINE_STATES)((int)LineState + 1);
			break;

		case SHOWFLAGS:
			szShowFlags = szToken;
			LineState = (LINE_STATES)((int)LineState + 1);
			break;

		case INSTALL_GROUP:
			// now the install group
			szInstallGroup = szToken;
			LineState = (LINE_STATES)((int)LineState + 1);
			break;
			
		case PLATFORM:
			// now the platform flags until end of line
			if (!FProcessPlatformToken(&szToken, &dwInstallBuild))
			{
				nReturn = EBU_ERROR;
				goto Done;
			}	
			break;
		}
	} while (TOKEN_OK == TokenState);
	
	if (TOKEN_EOL != TokenState)
	{
		nReturn = EBU_ERROR;
		goto Done;
	}

	if (ExecOnThisPlatform(dwInstallBuild))
	{
		ASSERT(szFileName);
		ASSERT(szDirectoryName);
		ASSERT(szParameters);
		ASSERT(szShowFlags);
		ASSERT(szInstallGroup);
		ASSERT(lstrlen(szFileName)+ 1 + lstrlen(szDirectoryName) + 1 +
			   lstrlen(szParameters) + 1 + sizeof(int) + sizeof(WORD) + 
			   lstrlen(szInstallGroup) + 1 <= MAX_DATA_LENGTH);

		pch = pShellExecute->szName;

		lstrcpy(pch, szFileName);
		pch += lstrlen(szFileName) + 1;

		lstrcpy(pch, szDirectoryName);
		pch += lstrlen(szDirectoryName) + 1;

		lstrcpy(pch, szParameters);
		pch += lstrlen(szParameters) + 1;

		* (int *) pch = atoi(szShowFlags);
		pch += sizeof(int);

		lstrcpy(pch, fWait ? "1" : "0");
		pch += lstrlen("1") + 1;

		lstrcpy(pch, szInstallGroup);
		pch += lstrlen(szInstallGroup) + 1;

		nReturn = ExecuteShellExecute(pShellExecute, bProcessType);
	}
	else
		nReturn = GetResultCode();	// use whatever the last code was

Done:
	if (szFileName)
		FreeMemory(szFileName);
	if (szDirectoryName)
		FreeMemory(szDirectoryName);
	if (szParameters)
		FreeMemory(szParameters);
	if (szShowFlags)
		FreeMemory(szShowFlags);
	if (szInstallGroup)
		FreeMemory(szInstallGroup);
	delete pShellExecute;

	return nReturn;
}

//****************************************************************************
// Procedure	FPreExecuteGetTemplate
//
// Purpose		Setup data strucs to call ExecuteGetName or ExecuteGetInstallGroups.
//				Data is read at runtime from a text file.
//
// Parameters	
//				hFile			handle to file we're reading from
//
// Returns		nonzero if successful, zero if setup was aborted.
//
// History		3/19/97 a-drews    Created
//
EBURETCODE FPreExecuteGetTemplate(HANDLE hFile, BYTE bGetType,
	LPRUNTIMECOMMAND lpRuntime, WORD cCommands, LPRUNTIMECOMMAND prgRuntime,UINT uFirstResID, BOOL fFirstTime )
{
	enum LINE_STATES {UNINSTALL_FLAG, PLATFORM};
	LINE_STATES LineState = UNINSTALL_FLAG;
	char *szToken = NULL;
	EBURETCODE nReturn = EBU_OK;
	TOKEN_STATE TokenState;
	CGetName *pGetName = new CGetName();
	CGetGroup *pGetGroup = new CGetGroup();
	DWORD dwInstallBuild = 0;

	ASSERT(GET_NAME == bGetType || GET_GROUP == bGetType);

	do
	{
		if (!FReadToken(hFile, szToken, &TokenState))
			continue;
		
		ASSERT(szToken);
		
		switch (LineState)
		{
		default:
			ASSERT(FALSE);
			break;

		case UNINSTALL_FLAG:
			if (!strcmpi(szToken, "UnInstall"))
			{
				// set the uninstall flag
				if (GET_NAME == bGetType)
					pGetName->wFlags |= IF_UNINSTALL;
				else
					pGetGroup->wFlags |= IF_UNINSTALL;
				LineState = (LINE_STATES) ((int) LineState + 1);
				FreeMemory(szToken);
				szToken = NULL;
				break;
			}

			//
			//we will now fall thorugh to PLATFORM
			//

		case PLATFORM:
			// now the platform flags until end of line
			if (!FProcessPlatformToken(&szToken, &dwInstallBuild))
			{
				nReturn = EBU_ERROR;
				goto Done;
			}	
			break;
		}
	} while (TOKEN_OK == TokenState);
	
	if (TOKEN_EOL != TokenState)
	{
		nReturn = EBU_ERROR;
		goto Done;
	}

	if (ExecOnThisPlatform(dwInstallBuild))
	{
		if (GET_NAME == bGetType)
			nReturn = ExecuteGetName(pGetName);
		else
			nReturn = ExecuteGetInstallGroups(pGetGroup, lpRuntime, cCommands,
				prgRuntime, uFirstResID, fFirstTime );
	}
	else
		nReturn = GetResultCode();	// use whatever the last code was

Done:
	delete pGetName;
	delete pGetGroup;

	return nReturn;
}


//****************************************************************************
// Procedure	FPreExecuteInstIcon
//
// Purpose		Setup data strucs to call ExecuteInstIcon.
//				Data is read at runtime from a text file.
//
// Parameters	
//				hFile			handle to file we're reading from
//
// Returns		nonzero if successful, zero if setup was aborted.
//
// History		3/17/97 a-drews    Created
//
EBURETCODE FPreExecuteInstIcon(HANDLE hFile, BYTE bProcessType)
{
	EBURETCODE nReturn = EBU_OK;
	CInstIcon *pInstIcon = new CInstIcon();
	DWORD dwInstallBuild = 0;
	
	ASSERT(DO_INSTALL == bProcessType || DO_UNINSTALL == bProcessType);

const int cStrings = 6;
char *rgsz[cStrings];
char *szToken;
int c = 0;
char *pch;
BOOL bFirst=TRUE;
TOKEN_STATE TokenState;

	dwInstallBuild = 0;
	do
	{
		if (!FReadToken(hFile, szToken, &TokenState))
			continue;
		ASSERT(szToken);

		// Check for optional UnInstalLink
		if  (bFirst)	
		{
			bFirst = FALSE;
			if (!strcmpi(szToken, "UnInstallLink"))
			{
				FreeMemory(szToken);
				szToken = NULL;
				pInstIcon->wFlags = IF_UNINSTALLLINK;
				continue;
			}
		}
			
		if (pInstIcon->szName && c < cStrings)
			// string token
			rgsz[c++] = szToken;
		else
			//Platform Token
			if (!FProcessPlatformToken(&szToken, &dwInstallBuild))
			{
				nReturn = EBU_ERROR;
				goto ParseDone;
			}
	} while (TOKEN_OK == TokenState);

	if (TOKEN_EOL != TokenState || c < cStrings || 0 == dwInstallBuild)
	{
		nReturn = EBU_ERROR;
		goto ParseDone;
	}

	pch = pInstIcon->szName;

	if (pInstIcon->szName)
	{
		for (c = 0; c < cStrings; c++)
		{
			ASSERT(rgsz[c]);
			lstrcpy(pch, rgsz[c]);
			pch += lstrlen(rgsz[c])+1;
			FreeMemory(rgsz[c]);
			rgsz[c] = NULL;
		}
	}

	if (ExecOnThisPlatform(dwInstallBuild))
	{
		char *pch;

		//convert IconIndex from CHAR to WORD
		pch = pInstIcon->GetIconDestination()+lstrlen(pInstIcon->GetIconDestination())+1;
		*(WORD *) pch = (WORD) atoi(pch);

		if (DO_INSTALL == bProcessType)
			nReturn = ExecuteInstIcon(pInstIcon);
		else
			nReturn = ExecuteRemoveIcon(pInstIcon);
	}
	else
		nReturn = GetResultCode();	// use whatever the last code was
	goto ExecuteDone;
// This Memory will be leaked if parsing fails
ParseDone:
	while (c > 0)
	{
		FreeMemory(rgsz[--c]);
		rgsz[--c] = NULL;
	}
ExecuteDone:
	delete pInstIcon;

	return nReturn;
}

//****************************************************************************
// Procedure	FPreExecuteGetPid
//
// Purpose		Setup data strucs to call ExecuteGetPid.
//				Data is read at runtime from a text file.
//
// Parameters	
//				hFile			handle to file we're reading from
//
// Returns		nonzero if successful, zero if setup was aborted.
//
// History		3/24/97 a-drews    Created
//
EBURETCODE FPreExecuteGetPid(HANDLE hFile )
{
	DWORD       dwInstallBuild = 0;
	CGetPID    *pGetPID = new CGetPID();
	EBURETCODE nReturn = EBU_OK;

	if (!FProcessStringAndPlatformTokens(hFile, 0, NULL, &dwInstallBuild))
	{
		nReturn = EBU_ERROR;
		goto Done;
	}

	nReturn = EBU_OK;

	if (ExecOnThisPlatform(dwInstallBuild))
	{
		nReturn = ExecuteGetPID(pGetPID);
	}
	else
		nReturn = GetResultCode();	// use whatever the last code was

Done:
	delete pGetPID;

	return nReturn;
}

//****************************************************************************
// Procedure	FPreExecuteRegWiz
//
// Purpose		Setup data strucs to call ExecuteRegWiz
//				Data is read at runtime from a text file.
//
// Parameters	
//				hFile			handle to file we're reading from
//
// Returns		nonzero if successful, zero if setup was aborted.
//
// History		12/1/97 reizen    Created
//
EBURETCODE FPreExecuteRegWiz(HANDLE hFile)
{
	EBURETCODE nReturn = EBU_OK;
	LPREGWIZ   pRegWiz = new CRegWiz();
	DWORD      dwInstallBuild = 0;

	if (!FProcessStringAndPlatformTokens(hFile, 1, pRegWiz->szURL, &dwInstallBuild))
	{
		nReturn = EBU_ERROR;
		goto Done;
	}

	if (ExecOnThisPlatform(dwInstallBuild))
		nReturn = ExecuteRegWiz(pRegWiz);
	else
		nReturn = GetResultCode();	// use whatever the last code was

Done:
	delete pRegWiz;

	return nReturn;
}

//****************************************************************************
// Procedure	FPreExecuteInstDX
//
// Purpose		Setup data strucs to call ExecuteInstDX.
//				Data is read at runtime from a text file.
//
// Parameters	
//				hFile			handle to file we're reading from
//
// Returns		nonzero if successful, zero if setup was aborted.
//
// History		3/25/97 a-drews    Created
//
EBURETCODE FPreExecuteInstDX(HANDLE hFile)
{
	EBURETCODE  nReturn = EBU_OK;
	CInstDX     *pInstDX = new CInstDX();
	DWORD       dwInstallBuild = 0;
	int	cStrings = 4;
	char		*pBuf;


	#define		MAX_NUMBER_OF_STRINGS 10
	char		*rgsz[MAX_NUMBER_OF_STRINGS];
	char		*szToken;
	int			c = 0;
	char		*pch;
	TOKEN_STATE TokenState;

	//Get the pointer to our target memory
	pBuf = pInstDX->szName;
	dwInstallBuild = 0;

	ASSERT(cStrings <= MAX_NUMBER_OF_STRINGS);

	//
	// Process the first cStrings cout of TOKENS as strings
	// Then look for the ONE or MORE Platform TOKENS
	//
	do
	{
		if (!FReadToken(hFile, szToken, &TokenState))
			continue;
		ASSERT(szToken);
		//Read first cStrings arguments as strings
		if (pBuf && c < cStrings)
		{
			//
			// if the first string is equal to the old DPLAY signal then error.
			//
			if ( ( c == 0) && (lstrcmpi("NULL", szToken) ? FALSE : TRUE) )
			{
				FreeMemory(szToken);
				goto Failed;
			}
			// string token
			rgsz[c++] = szToken;
		}
		else	
		{
				//Platform Token
				if (!FProcessPlatformToken(&szToken, &dwInstallBuild))
						goto Failed;
		}
	} while (TOKEN_OK == TokenState);

	if (TOKEN_EOL != TokenState || c < cStrings || 0 == dwInstallBuild)
		goto Failed;
 	

	pch = pBuf;

	//Put the values into the buffer
	if (pBuf)
	{
		for (c = 0; c < cStrings; c++)
		{
			ASSERT(rgsz[c]);
			//
			// The last string is the TOKEN for the DXFLAGS so convert it here
			// into a DWORD in the memory with no terminating char '\0'.
			//
			if (c == cStrings - 1)
            {
                DWORD dwFlags;

				//
				//At this point, the DX Flags are in string format, like "DSETUP_DIRECTX" or "65034"
				//
				if (0L == atol(rgsz[c]))
				{
					//
					//Set DXFlags as default (DSETUP_DIRECTX) DWORD
                    //

                    dwFlags = DSETUP_DIRECTX;
				}
				else
				{
					//
					//Convert DX Flags to DWORD
                    //
                    dwFlags = (DWORD) atol(rgsz[c]);
					
                }

                CopyMemory((void *) pch, (void *) &dwFlags, sizeof(DWORD));
                pch += sizeof(DWORD);
			}
			else
			{
				lstrcpy(pch, rgsz[c]);
				pch += lstrlen(rgsz[c])+1;
			}
			FreeMemory(rgsz[c]);
			rgsz[c] = NULL;
		}
	}

	if (ExecOnThisPlatform(dwInstallBuild))
		nReturn = ExecuteInstDX(pInstDX);
	else
		nReturn = GetResultCode();	// use whatever the last code was
	goto Done;

Failed:
	while (c > 0)
	{
		FreeMemory(rgsz[--c]);
		rgsz[--c] = NULL;
	}
	nReturn = EBU_ERROR;

Done:
	delete pInstDX;
	return nReturn;
}

//****************************************************************************
// Procedure	FPreExecuteInstDPLAY
//
// Purpose		Setup data strucs to call ExecuteInstDPLAY.
//				Data is read at runtime from a text file.
//
// Parameters	
//				hFile			handle to file we're reading from
//
// Returns		nonzero if successful, zero if setup was aborted.
//
// History		9/14/98 a-petere	Created
//
EBURETCODE FPreExecuteInstDPLAY(HANDLE hFile)
{
	EBURETCODE	nReturn = EBU_OK;
	CInstDPLAY	*pInstDPLAY = new CInstDPLAY();
	DWORD		dwInstallBuild = 0;
	char		*pBuf;
	const int	cStrings = 2;
	char		*rgsz[cStrings];
	char		*szToken;
	int			c = 0;
	char		*pch;
	TOKEN_STATE TokenState;

	//Get the pointer to our target memory
	pBuf = pInstDPLAY->szName;

	//
	// Process the first cStrings cout of TOKENS as strings
	// Then look for the ONE or MORE Platform TOKENS
	//
	do
	{
		if (!FReadToken(hFile, szToken, &TokenState))
			continue;
		ASSERT(szToken);
		//Read first cStrings arguments as strings
		if (pBuf && c < cStrings)
			// string token
			rgsz[c++] = szToken;
		else	
		{
				//Platform Token
				if (!FProcessPlatformToken(&szToken, &dwInstallBuild))
						goto Failed;
		}
	} while (TOKEN_OK == TokenState);

	if (TOKEN_EOL != TokenState || c < cStrings || 0 == dwInstallBuild)
		goto Failed;
 	

	pch = pBuf;

	//Put the values into the buffer
	if (pBuf)
	{
		for (c = 0; c < cStrings; c++)
		{
			ASSERT(rgsz[c]);
			lstrcpy(pch, rgsz[c]);
			pch += lstrlen(rgsz[c])+1;
			FreeMemory(rgsz[c]);
			rgsz[c] = NULL;
		}
	}

	if (ExecOnThisPlatform(dwInstallBuild))
		nReturn = ExecuteInstDPLAY(pInstDPLAY);
	else
		nReturn = GetResultCode();	// use whatever the last code was
	goto Done;

Failed:
	while (c > 0)
	{
		FreeMemory(rgsz[--c]);
		rgsz[--c] = NULL;
	}
	nReturn = EBU_ERROR;

Done:
	delete pInstDPLAY;
	return nReturn;
}

//****************************************************************************
// Procedure	FPreExecuteCDSpeed
//
// Purpose		Setup data strucs to call ExecuteCDSpeed.
//				Data is read at runtime from a text file.
//
// Parameters	
//				hFile			handle to file we're reading from
//
// Returns		nonzero if successful, zero if setup was aborted.
//
// History		3/26/97 a-drews    Created
//
EBURETCODE FPreExecuteCDSpeed(HANDLE hFile)
{
	enum LINE_STATES {MIN_CD=0, MAX_CPU, DATA_NAME, PLATFORM};
	LINE_STATES LineState = MIN_CD;
	EBURETCODE nReturn = EBU_OK;
	char *szToken = NULL;
	char *szMinCD = NULL;
	char *szMaxCPU = NULL;
	char *szDataName = NULL;
	TOKEN_STATE TokenState;
	CCDSpeed *pCDSpeed = new CCDSpeed();
	DWORD dwInstallBuild = 0;
	char *pch = NULL;

	do
	{
		if (!FReadToken(hFile, szToken, &TokenState))
			continue;
		
		ASSERT(szToken);
		
		switch (LineState)
		{
		default:
			ASSERT(FALSE);
			break;

		case MIN_CD:
			szMinCD = szToken;
			LineState = (LINE_STATES)((int)LineState + 1);
			break;

		case MAX_CPU:
			szMaxCPU = szToken;
			LineState = (LINE_STATES)((int)LineState + 1);
			break;

		case DATA_NAME:
			szDataName = szToken;
			LineState = (LINE_STATES)((int)LineState + 1);
			break;

		case PLATFORM:
			if (!FProcessPlatformToken(&szToken, &dwInstallBuild))
			{
				nReturn = EBU_ERROR;
				goto Done;
			}
			break;
		}
	} while (TOKEN_OK == TokenState);
	
	if (TOKEN_EOL != TokenState)
	{
		nReturn = EBU_ERROR;
		goto Done;
	}

	if (ExecOnThisPlatform(dwInstallBuild))
	{
		// build the name string which may includes the filename and always includes the
		//	section name and key name
		ASSERT(szMinCD);
		ASSERT(szMaxCPU);
		ASSERT(szDataName);
		ASSERT(sizeof(WORD) + sizeof(WORD) + lstrlen(szDataName)+1<= MAX_DATA_LENGTH);
			
		pch = pCDSpeed->szName;
		//convert szMinCD to WORD and add it.
		*(WORD *)pch = (WORD)atoi(szMinCD);
		pch += sizeof(WORD);
		//convert szMaxCPU to WORD and add it.
		*(WORD *)pch = (WORD)atoi(szMaxCPU);
		pch += sizeof(WORD);
		//Add szDataName
		lstrcpy(pch, szDataName);

		nReturn = ExecuteCDSpeed(pCDSpeed);
	}
	else
		nReturn = GetResultCode();	// use whatever the last code was

Done:
	if (szMinCD)
		FreeMemory(szMinCD);
	if (szMaxCPU)
		FreeMemory(szMaxCPU);
	if (szDataName)
		FreeMemory(szDataName);
	delete pCDSpeed;

	return nReturn;
}

//****************************************************************************
// Procedure	FPreExecuteDeleteFile
//
// Purpose		Setup data strucs to call ExecuteDeleteFile.
//				Data is read at runtime from a text file.
//
// Parameters	
//				hFile			handle to file we're reading from
//
// Returns		nonzero if successful, zero if setup was aborted.
//
// History		3/26/97 a-drews    Created
//
EBURETCODE FPreExecuteDeleteFile(HANDLE hFile)
{
	enum LINE_STATES {UNINSTALL_FLAG, PATHNAME, PLATFORM};
	LINE_STATES LineState = UNINSTALL_FLAG;

	char *szToken = NULL;
	TOKEN_STATE TokenState;
	EBURETCODE nReturn = EBU_OK;
	CDeleteFile *pDeleteFile = new CDeleteFile();
	DWORD dwInstallBuild = 0;

	do
	{
		if (!FReadToken(hFile, szToken, &TokenState))
			continue;
		
		ASSERT(szToken);
		
		switch (LineState)
		{
		default:
			ASSERT(FALSE);
			break;

		case UNINSTALL_FLAG:
			if (!lstrcmpi(szToken, "silent"))
			{
				// set the uninstall flag
				pDeleteFile->wFlags |= IF_UNINSTALL;
				FreeMemory(szToken);
				szToken = NULL;
				break;
			} 
			else if (!lstrcmpi(szToken, "persist"))
			{
				// set the uninstall flag
				pDeleteFile->wFlags |= IF_NOTUNINSTALL;
				FreeMemory(szToken);
				szToken = NULL;
				break;
			} 
			else if (!lstrcmpi(szToken, "recurse"))
			{
				// set the uninstall flag
				pDeleteFile->wFlags |= IF_UNINSTALLALL;
				FreeMemory(szToken);
				szToken = NULL;
				break;
			} 
			else if (!lstrcmpi(szToken, "install"))
			{
				// set the uninstall flag
				pDeleteFile->wFlags |= IF_INSTALL;
				FreeMemory(szToken);
				szToken = NULL;
				break;
			} 
			else
			{
				// we will now fall through to FILENAME
			}
			
		case PATHNAME:

			// now the destination name
			lstrcpy(pDeleteFile->szName, szToken);
			LineState = PLATFORM;
			FreeMemory(szToken);
			szToken = NULL;
			break;

		case PLATFORM:
			// now the platform flags until end of line
			if (!FProcessPlatformToken(&szToken, &dwInstallBuild))
			{
				nReturn = EBU_ERROR;
				goto Done;
			}	

			break;
		}
	} while (TOKEN_OK == TokenState);
	
	if (TOKEN_EOL != TokenState)
	{
		nReturn = EBU_ERROR;
		goto Done;
	}

	if (ExecOnThisPlatform(dwInstallBuild))
		nReturn = ExecuteDeleteFile(pDeleteFile);
	else
		nReturn = GetResultCode();	// use whatever the last code was

Done:
	delete pDeleteFile;

	return nReturn;
}
#endif //READFILE
