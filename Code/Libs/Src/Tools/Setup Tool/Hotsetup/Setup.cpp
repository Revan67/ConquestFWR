//
// Setup.cpp
//
// Functions to determine which files we need to install and
// actually install them.
//
// History:
//  1/26/95 KenSh       Created
//  ?/??/95 StephHer    Added uninstall code
//  8/13/95 KenSh       Write DLL reference-counts into the registry
//                      as DWORDs instead of strings.
//  8/22/95 a-DenSo     Add support for SC_INSTALLFONT
//  8/24/95 KenSh       Delete VER.DLL from the Windows directory
//  8/09/96 a-melodh    Add documentation
//  8/20/96 a-melodh    Divide RunSetup() up into InstallApp() and UnistallApp()
//  8/26/96 a-melodh    Put some of BuildCopyList() code into other functions. (modularize it a bit)
//		03/15/97 a-dashoe		update timestamp
//  8/21/98 a-nigelh	BuildCopyList functionality moved to GetFileSizeRequirements
//
#include "stubpch.h"
#include "setup.h"
#include "hotsetup.h"
#include "HotSetupRC.h"
#include "vercopy.h"
#include "command.h"
#include "restart.h"
#include "registry.h"
#include "progman.h"
#include "pid.h"
#include "uninstal.h"
#include "hotsetup.h"

//
//Wild card mask to check for to see if SETUPxxx.DLL exists...
//
#define strSETUPRESDLL "SETUP???.DLL"

using namespace NGLOBALS;

extern DWORD	g_dwGameBytesPerCluster, g_dwSystemBytesPerCluster;
//BOOL   n_fMaintMode = FALSE;			// Maintainence mode flag, set and reset in MaintainApp()
//NGLOBALS::n_fMaintMode = FALSE;

static int g_nLanguageDefault = 0;
static LPRUNTIMECOMMAND g_prgRuntime;
static int g_cCommands;


//*** Local function declarations
//
BOOL GetBootstrapFileName(TCHAR *pszExePath, TCHAR *pszTempExePath, TCHAR *pszExtension);
static BOOL CopySetupToHardDriveAndRunFromThere(LPSTR lpszCmdLine);
void    ExecuteSCInstallFile(HGLOBAL* phglbCommand, LPRUNTIMECOMMAND lprgRuntime, LPSETUPCOMMAND  lpCommand, int nIndex, DWORD* pdwGameClustersNeeded, DWORD* pdwSystemClustersNeeded, BOOL fFirstTime);
void    ExecuteSCInstallFont(HGLOBAL* phglbCommand, LPRUNTIMECOMMAND lprgRuntime, LPSETUPCOMMAND  lpCommand, int nIndex, DWORD* pdwGameClustersNeeded, DWORD* pdwSystemClustersNeeded, BOOL fFirstTime);
static  UINT ShouldFontBeInstalled (LPINSTALLFONT pInstallFont, LPRUNTIMECOMMAND pRunCommand, BOOL fFirstTime);
static  int NEAR AskLanguageOverwrite( DWORD dwLangCur, DWORD dwLangNew );
static  BOOL MySetupUninstall();
static  DWORD BytesToClusters (DWORD dwBytes, BOOL fSystemDrive);
static  void StampVersionType(BOOL fbTrial);
static  void ParseCommandLine(LPSTR lpszCmdLine, BOOL *fDontBootstrap);
extern  BOOL FLoadResourceDLL(LCID lcid, ALERTWINPROC AlertWinProc = NULL);
extern BOOL BootstrapResourceDll(TCHAR *pszResourceDllName);
extern BOOL GetFileSizeRequirements(LPRUNTIMECOMMAND prgRuntime, WORD cCommands, TCHAR *pszGameDrive, DWORD *dwGameKBytesFree, DWORD *dwGameKBytesNeeded, DWORD *dwSystemKBytesFree, DWORD *dwSystemKBytesNeeded,  __int64 filegroup, BOOL fFirstTime);

BOOL LoadTokenFile(HRSRC hRsrc, LPUINT uFirstResID, LPINT cCommands);
EBURETCODE CreateScriptList(LPRUNTIMECOMMAND lprgRuntime, int cCommands, UINT uFirstResID);

//
//Defined in command.cpp, used in ShouldFileBeInstalled()
//
extern VOID ReplaceStringTokens(char *sz, size_t wBuf);

static void FreeRuntimeResources( LPRUNTIMECOMMAND lpRuntime, int cCommands );

//****************************************************************************
// Procedure   ParseCommandLine
//
// Purpose     Splits the command-line into separate arguments.  Arguments
//          are separated by spaces, unless the argument is quoted.  If
//          quotes (") are used then spaces can occur within the argument.
//          Note that the string passed to this function is mutated.
//          Pointers to the args are stored in rglpArg[].
//
// Parameters  lpszCmdLine    the command line to split into arguments
//
// Returns     nothing
//
// History      1/30/95 KenSh    Created
//
void ParseCommandLine(LPSTR lpszCmdLine, BOOL *fDontBootstrap)
{
	ASSERT(lpszCmdLine);
	TRACE("Command line: %s\n", lpszCmdLine);

	int
		nArgs = 0;       // number of command-line args

	TCHAR 
		*pSrcChar = NULL,		// working pointer into the command line
		*pDestChar = NULL;		// working pointer into the destination point

	BOOL  bMidQuote = FALSE; //are we in the middle of a quoted word

	
	DWORD dwBuildBits = 0;


	struct SCommandLineParam
	{
		TCHAR
			szSwitch[128],
			szParam[MAX_PATH];
		SCommandLineParam() {szSwitch[0] = NULL; szParam[0] = NULL;};
	};

	enum PARAMS {AUTORUN, FORCE, PATCH, RUNTEMP, UNINSTALL, DBCS, ANSI, OEM, RTL, LANG, DLLPATH, NOVER, DXFORCE, NODX, 
		INSTALLDIR, SETUPDIR, INSTALLTYPE, LOG, LOGLEVEL, QUIET, REBOOT, USERPATH, NOSOUND, MCISOUND, NOREBOOT, HELP, NOOP, NUM_PARAMS};

	SCommandLineParam
		Params[NUM_PARAMS + MAX_ARG];

	struct
	{
		int   nArgID;
		TCHAR *pszArg;
		TCHAR *pszParamHelp;
		TCHAR *pszHelp;
		TCHAR *pszError;
	} rgArgs[NUM_PARAMS] = 
		{
			AUTORUN,		"autorun",		"",					"Run setup in autorun mode",				"",
			FORCE,			"force",		"",					"Ignores disk space requirements",			"",
			PATCH,			"patch",		":CDSetupDir",		"Use this setup to overide cd version",		"Can't find CD Setup.",
			SETUPDIR,		"setupdir",		":CDSetupDir",		"Specify setup source directory",		"Can't find CD Setup.",
			INSTALLDIR,		"installdir",	":InstallDir",		"Folder to install game to",				"",
			RUNTEMP,		"runtemp",		"[:SetupSrcPath]",	"Don't bootstrap setup, param shows the original setup location.", "error.",
			UNINSTALL,		"uninstall",	"",					"Run setup in uninstall mode",				"",
			DBCS,			"dbcs",			"",					"Turn on DBCS option",						"",
			ANSI,			"ansi",			"",					"Turn on ANSI option",						"",
			OEM,			"oem",			"",					"Use OEM options",							"",
			RTL,			"rtl",			"",					"Use retail options",						"",
			LANG,			"lang",			":LangID",			"Use LandID script options",				"",
			DLLPATH,		"dllpath",		":DLLPath",			"Full path to setup dll",					"Can't find setup dll",
			NOVER,			"nover",		"",					"No destination file version checking",		"",
			DXFORCE,		"dxforce",		"",					"Force DX install if applicable to OS",		"",
			NODX,			"nodx",			"",					"Ignore DX install requirement",			"",
			REBOOT,			"reboot",		"",					"Force reboot after setup completion",		"",
			USERPATH,		"userpath",		"",					"XXXX??????",								"",
			NOSOUND,		"nosound",		"",					"Turn off all sound effects in setup",		"",
			MCISOUND,		"mcisound",		"",					"Use the alternate setup sound method",		"",
			NOREBOOT,		"noreboot",		"",					"Ignore reboot after setup completion",		"",
			HELP,			"?",			"",					"This help text",							"",
			INSTALLTYPE,	"InstallType",	":####",			"Group id",									"",
			LOG,			"log",			"[:FileName[+]]",	"Setup log file name + appends to log",		"Can't create file",
			LOGLEVEL,		"loglevel",		":n",				"0: Errors only,  1: Normal,  2: Full",		"",
			QUIET,			"quiet",		"",					"Quiet mode no UI",			"",
			NOOP,			"", "", ""
		};
	

	SetRanUninstall(FALSE);
	
	pSrcChar = lpszCmdLine;
	pDestChar = Params[0].szSwitch;


	enum STATE {WHITE_SPACE, SWITCH, PARAM} state = WHITE_SPACE;

	//
	//tokenize the command line and get an array full of command line args...
	//
	while (*pSrcChar)
	{
		switch (state)
		{
		case WHITE_SPACE:
			{
				// trim off leading white space
				if ((' ' == *pSrcChar) || ('\t' == *pSrcChar))
				{
					break;
				}else{
					state = SWITCH;	// now looking for a switch marker '/'
				}
			}


		case SWITCH:
			{
				// two things make us change state here. Either we find the param marker ':' or white space
				switch(*pSrcChar)
				{
				case ':':
					{
						// starting the param
						// truncate the option switch section here
						*pDestChar = NULL;

						// now set the destination to the param section of this option
						pDestChar = Params[nArgs-1].szParam;

						state = PARAM;
					}break;

				case ' ':
				case '\t':
					{
						// starting a white space section
						// truncate the switch here
						*pDestChar = NULL;

						state = WHITE_SPACE;
					}break;

				case '/':
					{
						// starting a new param
						pDestChar = Params[nArgs].szSwitch;
						nArgs++;
						if (FALSE) //(MAX_ARG == nArgs)
						{
							TRACE("Exceded max command line args.\n");
							nArgs = MAX_ARG -1;
						}
					}break;

				default:
					{
						// copy the char over
						*pDestChar = *pSrcChar;
					}

				}
			}break;


		case PARAM:
			{
				// we walk the param until we hit a white space delimiter (quoted spaces don't count)
				switch (*pSrcChar)
				{
					case ' ':
					case '\t':
						{
							if (bMidQuote)
							{
								*pDestChar = *pSrcChar;
							}else{
								// truncate the param string
								*pDestChar = NULL;
								state = WHITE_SPACE;
							}
						}break;

					case '"':
						{
							bMidQuote = !bMidQuote;
						}break;

					default:
						{
							*pDestChar = *pSrcChar;
						}
				}
			}break;	// PARAM
		}

		if (*pDestChar)
			pDestChar++;

		pSrcChar = CharNext(pSrcChar);
	}

	if (pDestChar)
	{
		*pDestChar = NULL;
	}

#ifdef _DEBUG
	TRACE("Command line params:\n");
	for (int i = 0; i < nArgs; i++)
	{
		TRACE("  %d -> Switch:[%s] Param:[%s]\n", i, Params[i].szSwitch, Params[i].szParam);
	}
	TRACE("\n\n");
#endif

	// Check command line parms.  This is a strict lstrcmpi with the switch keywords against the option switch,
	// partial matches don't count.

	int nArgIdx = 0;
	while (0 < nArgs--)
	{
		nArgIdx = 0;

		for (nArgIdx = 0; (nArgIdx < NUM_PARAMS-1) && lstrcmpi(Params[nArgs].szSwitch, rgArgs[nArgIdx].pszArg); nArgIdx++);

		switch (rgArgs[nArgIdx].nArgID)
		{
		case SETUPDIR:
			{
				TRACE("[SETUPDIR] OEM switch NYI.\n");
			}break;

		case INSTALLDIR:
			{
				TRACE("[INSTALLDIR] OEM switch NYI.\n");
			}break;

		case INSTALLTYPE:
			{
				TRACE("[INSTALLTYPE] OEM switch NYI.\n");
			}break;

		case QUIET:
			{
				TRACE("[QUIET] OEM switch NYI.\n");
			}break;

		case LOG:
			{
				TRACE("[LOG] OEM switch NYI.\n");
			}break;

		case LOGLEVEL:
			{
				TRACE("[LOGLEVEL] OEM switch NYI.\n");
			}break;

		case AUTORUN:
			{
				TRACE("[AUTORUN] Setup in AUTORUN mode.\n");
				SetInAutoRun(TRUE);
			}break;

		case FORCE:
			{
				TRACE("[FORCE] Disk space requirements ignored.\n");
				SetForceFreeSpace(TRUE);
			}break;

		case PATCH:
			{
				TRACE("[PATCH] Patching setup at: %s\n", Params[nArgs].szParam);

				SetPatchPath(Params[nArgs].szParam);
			}break;

		case RUNTEMP:
			{
				TRACE("[RUNTEMP] ");

				if (*Params[nArgs].szParam && (FALSE == *fDontBootstrap))
				{
					TRACE("Source path: %s", Params[nArgs].szParam);
					SetSourcePath(Params[nArgs].szParam);
				}
				else
				{
					TRACE("Not Bootstraping");
					*fDontBootstrap = TRUE;
				}

				TRACE(".\n");
			}break;

		case UNINSTALL:
			{
				TRACE("[UNINSTALL] Setup in UNINSTALL mode.\n");

				SetRemovingApp(TRUE);
				SetPromptDelete(TRUE);
				SetRanUninstall(TRUE);
			}break;

		case DBCS:
			TRACE("[DBCS].\n");
			dwBuildBits |= BLD_DBCS;

			break;

		case ANSI:
			TRACE("[ANSI].\n");
			dwBuildBits |= BLD_ANSI;

			break;

		case OEM:
			TRACE("[OEM].\n");
			dwBuildBits |= BLD_OEM;

			break;

		case RTL:
			TRACE("[RTL].\n");
			dwBuildBits |= BLD_RTL;

			break;

		case LANG:
			TRACE("[LANG] Langauge set as: %s\n", Params[nArgs].szParam);

			SetUserLanguage(Params[nArgs].szParam);

			break;

		case DLLPATH:
			TRACE("[DLLPATH] DLL at: %s", Params[nArgs].szParam);

			SetResDLLPath(Params[nArgs].szParam);

			break;

		case NOVER:
			TRACE("[NOVER] Dest version checking turned off.\n");
			SetIgnoreFileInfo(TRUE);

			break;

		case DXFORCE:
			TRACE("[DXFORCE] DX install forced on (if os applicable).\n");
			SetForceDXFlag(TRUE);

			break;
		
		case NODX:
			TRACE("[NODX] DX install forced off (if os applicable).\n");
			SetNoDXFlag(TRUE);

			break;

		case REBOOT:
			{
				TRACE("[REBOOT] Reboot required after install.\n");
				SetReboot(TRUE);
			}break;

		case USERPATH:
			{
				TRACE("[USERPATH] at: %s\n", Params[nArgs].szParam);

				SetUserPath(Params[nArgs].szParam);
			}break;

		case NOSOUND:
			{
				TRACE("[NOSOUND] Setup sounds turned off.\n");

				SetNoSound(TRUE);
			}break;

		case MCISOUND:
			{
				TRACE("[MCISOUND] Alternate Setup sounds player turned on.\n");
				SetMCISound(TRUE);
			}break;

		case NOREBOOT:
			{
				TRACE("[NOREBOOT] Noreboot required after install.\n");
			}break;

		case HELP:
			{
				TRACE("Command line params:\n");
				for (nArgIdx = 0; (nArgIdx < NUM_PARAMS-1); nArgIdx++)
				{
					TRACE("  /%-15s %s\n", rgArgs[nArgIdx].pszArg, rgArgs[nArgIdx].pszHelp);
				}
				TRACE("\n\n");
			}break;

		default:
			{
				// unexpected param
			}break;
		}
	}

	//
	//Set any build characteristics specified on command line...
	//
	SetBuild(dwBuildBits);
}

//****************************************************************************
// Procedure    InitEBUSetup
//
// Purpose      Initializes engine, loads resource DLL, parses command line,
//              sets global app callback proc and messagebox (Alert) proc
//
// Parameters   lpszCmdLine			Windows command line
//				AppCallbackProc		Setup app callback function
//				AlertWinProc		Setup apps' MessageBox callback proc 

// Returns      EBU_OK if successful, EBU_ERROR if not...
//
EBURETCODE InitEBUSetup(LPSTR lpszCmdLine, EBUCALLBACK AppCallbackProc, ALERTWINPROC AlertWinProc, HWND hWndParent, BOOL fDontBootstrap, BOOL fZone)
{
	TCHAR  *pszFileName;

	//
	//Determine current operating system...
	//
    SetOS(GetCurrentOperatingSystem());

	//
	//Get command line parms
	//
	ParseCommandLine(lpszCmdLine, &fDontBootstrap);

	//
	//Never bootstrap on unsupported OS's
	//
	if (OS_NOTSUPPORTED == GetOS())
	{
		fDontBootstrap = TRUE;
	}

	if (TRUE == fDontBootstrap)
	{
		//
		//Not bootstrapping, so use .EXE module file name as source path...
		//
		GetModuleFileName(NULL, GetSourcePath(), MAX_PATH);
	}
	else
	{
		//
		//If we were run WITHOUT the /RunTemp switch then we haven't been bootstrapped yet
		//so call the function to bootstrap our setup/uninstall .EXE
		//
		if ('\0' == *(GetSourcePath()))
		{
			if (TRUE == CopySetupToHardDriveAndRunFromThere(lpszCmdLine))
			{
				//
				//Not really an error, just signaling that we're aborting initialization
				//
				return EBU_ERROR;
			}
			else
			{
				// If user told us to abort during a CD Check then do not proceed.
				if (EBU_ABORT == GetResultCode())
					return EBU_ABORT;
				//
				//Bootstrap failed for some reason so use .EXE module file name...
				//
				GetModuleFileName(NULL, GetSourcePath(), MAX_PATH);

				SetBootstrapFlag(FALSE);
			}
		}
		else
		{
			//
			//Set flag so we know whether we need to delete ourself on the way out.
			//It we bootstrapped, we did, else we don't. see DeleteMyself() function...
			//
			SetBootstrapFlag(TRUE);
		}
	}

	//
	//Use this global variable as the window for MessageBoxes...
	//
	SetWndParent(hWndParent);

	//
	//RUNTEMP switch always passes in full path/file name...
	//
	pszFileName = pszGetLast5C(GetSourcePath());
	ASSERT(pszFileName);

	//
	//Point to beginning of .EXE file name portion
	//
	SetSetupExeName(CharNext(pszFileName));

	//
	//Remove file name from .EXE path by nulling out last backslash
	//
	*pszFileName = '\0';

	//
	//Now point to the directory where original setup was run from...
	//
	SetCurrentDirectory(GetSourcePath());

	if (FALSE == FLoadResourceDLL(LOCALE_USER_DEFAULT, AlertWinProc))
	{
		//
		//We get in here if this is a /PATCH setup and there was no
		//resource .DLL in the directory where /PATCH setup was initialized...
		//
		SetCurrentDirectory(GetPatchPath());

		if (FALSE == FLoadResourceDLL(LOCALE_USER_DEFAULT, AlertWinProc))
		{
			return EBU_ERROR;
		}
		else
		{
			SetSourcePath(GetPatchPath());
		}
	}
	
	//
	//If /PATCH switch specified, always set the source path to the specified
	//source directory...
	//
	if (*(GetPatchPath()))
	{
		SetCurrentDirectory(GetPatchPath());
	}

	//
	//Determine current build characteristics as specified in resource DLL
	//
	SetBuild(GetBuildCharacteristics());

	//
	//Set global callback function pointer
	//
	SetAppCallback(AppCallbackProc);

	ASSERT((GetAppCallback()));

	InitRegistry(fZone);
	
	return EBU_OK;
}

//****************************************************************************
// Procedure    UninstallApp
//
// Purpose      Checks what needs to be uninstalled and uninstalls whatever's necessary.
//
// Parameters   g_hWndParent      Parent window for MessageBox()
//              bTrial          TRUE if we are installing trial version.
//
// Returns      nonzero to continue running the stub; zero to abort for
//              any reason.
//
// History
//  8/20/96 a-melodh    Created from RunSetup() for case when we are UNinstalling app.
//
EBURETCODE UninstallApp(HWND hWndParent, BOOL bTrial)
{
    HRSRC               hRsrc =  NULL;
    LPRUNTIMECOMMAND    prgRuntime;
    char                szDestDrive[_MAX_PATH];     //destination drive, e.g. "C:\"
    EBURETCODE          nResult = EBU_OK;

    int cCommands;      // number of commands in setup script
    UINT uFirstResID;   // resource ID of first command

    // Store some globals
	ASSERT(hWndParent);
    SetWndParent(hWndParent);
    SetRemovingApp(TRUE);

    // we are uninstall, therefore we aren't trying to overwrite language system
    g_nLanguageDefault = IDNO;

    if (!LoadTokenFile(hRsrc, &uFirstResID, &cCommands))
        return EBU_ERROR;

    if( cCommands == 0 )
    {
        return EBU_OK;
    }

    // Calculate required disk space, build copy list
    prgRuntime = (LPRUNTIMECOMMAND)GlobalAllocPtr( GMEM_MOVEABLE | GMEM_ZEROINIT,
                            sizeof(RUNTIMECOMMAND) * cCommands );
    if( !prgRuntime )
    {
        Alert (GetWndParent(), MB_OK | MB_ICONSTOP, STR_ERROR_NOMEMORY);
        return EBU_ERROR;
    }

    if (EBU_ERROR == CreateScriptList(prgRuntime, cCommands, uFirstResID))
    {
        //Unlock and free resources in prgRuntime
        FreeRuntimeResources( prgRuntime, cCommands );

        GlobalFreePtr(prgRuntime);

		Alert(GetWndParent(), MB_OK | MB_ICONSTOP, STR_ERROR_RESOURCEFAILURE);

        return EBU_ERROR;
    }

    //
    // This confusing block of code does two things: 1) Gets the destination drive of the AppDir
    // 2) if g_szAppDir specifies a path without the driveName, then insert driveName into g_szAppDir.

    // if AppDir does not contain driveName, then default to WindowsDriveRoot
    szDestDrive[0] = '\0';
    LPSTR pSrcChar;
    CHAR szTmp[_MAX_PATH];
    CHAR ch0,ch1;

    ch0 = GetChFromAppDir(0);
    ch1 = GetChFromAppDir(1);
    if ( isalpha( ch0 ) && ch1 == ':' )
    {
        szDestDrive[0] = ch0;
        szDestDrive[1] = ch1;
        szDestDrive[2] = '\\';
        szDestDrive[3] = '\0';
        pSrcChar = GetLpChFromAppDir(2);
        if ( *pSrcChar == '\\' )
            pSrcChar = AnsiNext( pSrcChar );
    }
    else
    {
        // get drive name where Windows installed
        GetWindowsDirectory( szDestDrive, sizeof(szDestDrive) );
        ASSERT( isalpha(szDestDrive[0]) );
        pSrcChar = AnsiNext( szDestDrive );
        ASSERT( *pSrcChar == ':' );
        pSrcChar = AnsiNext( pSrcChar );
        ASSERT( *pSrcChar == '\\' );
        *pSrcChar = '\0';
        // szTmp is like that "C:"
        pSrcChar = GetAppDir();
        if ( *pSrcChar == '\\' )
            pSrcChar = AnsiNext( pSrcChar );
    }
    lstrcpy( szTmp, szDestDrive );
    lstrcat( szTmp, pSrcChar );
    SetAppDir(szTmp);
    // Now, g_szAppDir is like that
    // "C:\Program files\Microsoft Games\A New Game"

    nResult = ExecuteAllUninstallCommands(prgRuntime, cCommands, TRUE );

	ASSERT(EBU_OK == nResult || EBU_ERROR == nResult || EBU_ABORT == nResult || EBU_CANCEL == nResult);

    PerformUninstallCommands();
	
    //Unlock and free resources in prgRuntime
    FreeRuntimeResources(prgRuntime, cCommands);
    GlobalFreePtr(prgRuntime);

    if (EBU_OK == nResult)
    {
        ExecuteRdRoot(NULL);
        SetDirCreated(FALSE);
    }

	SetRemovingApp(FALSE);

	//
	//Set flag that the resource dll and uninstaller should be deleted only
	//if uninstall was successful and if "setup /uninstall" or "uninstal.exe"
	//was run...
	//
	SetDeleteSetup(EBU_OK == nResult && GetRanUninstall() ? TRUE : FALSE);

	ClearDirtyBits();

    return nResult;
}

//****************************************************************************
// Procedure    InstallApp
//
// Purpose      Checks what needs to be installed and installs whatever's
//              necessary.  Restarts windows if necessary.
//
// Parameters   hWndParent      Parent window for MessageBox()
//              fFirstTime      Is this the 1st time the app's been launched?
//              bTrial          TRUE if we are installing trial version.
//
// Returns      nonzero to continue running the stub; zero to abort for
//              any reason.
//
// History
//  8/20/96 a-melodh    Created from RunSetup() for case when we are installing app.
//
EBURETCODE InstallApp( HWND hWndParent, BOOL fFirstTime, BOOL bTrial)
{
    HRSRC               hRsrc = NULL;
    LPRUNTIMECOMMAND    prgRuntime;
    char                szDestDrive[_MAX_PATH];     //destination drive, e.g. "C:\"
    EBURETCODE          nResult = EBU_OK;

    int cCommands;      // number of commands in setup script
    UINT uFirstResID;   // resource ID of first command

    // Store some globals
	ASSERT(hWndParent);
    SetWndParent(hWndParent);
	SetTrial(bTrial);
	SetCopyIncomplete(FALSE);
	SetMaxDirLen(0);
	n_nFilesInUninstall = 0;
	n_nFilesToDelete = 0;
	SetOldGroupList(0);

    // If this is not the first time the app has been run, then assume "NO"
    // to the question: 'do you want to overwrite your xxx-language system
    // files with yyy-language?'
    if( !fFirstTime )
    {
      g_nLanguageDefault = IDNO;
    }

    if (!LoadTokenFile(hRsrc, &uFirstResID, &cCommands))
        return EBU_ERROR;

    if (cCommands == 0)
    {
      return EBU_OK;
    }

    prgRuntime = (LPRUNTIMECOMMAND)GlobalAllocPtr( GMEM_MOVEABLE | GMEM_ZEROINIT,
                                 sizeof(RUNTIMECOMMAND) * cCommands );
    if (!prgRuntime)
    {
      Alert(GetWndParent(), MB_OK | MB_ICONSTOP, STR_ERROR_NOMEMORY);
      return EBU_ERROR;
    }

    if (EBU_ERROR == CreateScriptList(prgRuntime, cCommands, uFirstResID))
    {
        //Unlock and free resources in prgRuntime
        FreeRuntimeResources( prgRuntime, cCommands );

        GlobalFreePtr( prgRuntime );

        Alert(GetWndParent(), MB_OK | MB_ICONSTOP, STR_ERROR_RESOURCEFAILURE);
        
        return EBU_ERROR;
    }

    // Forward messages and check for Alt-F4
    if( !ForwardMessages() )
	{
		//
		//How do we ever get into here?  Do we need this if test? a-richei
		//
		ASSERT(FALSE);
        return EBU_ABORT;
	}

	//
	//Save these for use in AppGetFileSizeRequirements() function...
	//
	g_prgRuntime = prgRuntime;
	g_cCommands = cCommands;

    //
    // This confusing block of code does two things: 1) Gets the destination drive of the AppDir
    // 2) if g_szAppDir specifies a path without the driveName, then insert driveName into g_szAppDir.

    szDestDrive[0] = '\0';
    // if AppDir does not contain driveName, then default to WindowsDriveRoot
    LPSTR pSrcChar;
    CHAR szTmp[_MAX_PATH];
    CHAR ch0,ch1;

    ch0 = GetChFromAppDir(0);
    ch1 = GetChFromAppDir(1);
    if ( isalpha( ch0 ) && ch1 == ':' )
    {
        szDestDrive[0] = ch0;
        szDestDrive[1] = ch1;
        szDestDrive[2] = '\\';
        szDestDrive[3] = '\0';
        pSrcChar = GetLpChFromAppDir(2);
        if ( *pSrcChar == '\\' )
            pSrcChar = AnsiNext( pSrcChar );
    }
    else
    {
        // get drive name where Windows installed
        GetWindowsDirectory( szDestDrive, sizeof(szDestDrive) );
        ASSERT( isalpha(szDestDrive[0]) );
        pSrcChar = AnsiNext( szDestDrive );
        ASSERT( *pSrcChar == ':' );
        pSrcChar = AnsiNext( pSrcChar );
        ASSERT( *pSrcChar == '\\' );
        *pSrcChar = '\0';
        // szTmp is like that "C:"
        pSrcChar = GetAppDir();
        if ( *pSrcChar == '\\' )
            pSrcChar = AnsiNext( pSrcChar );
    }
    lstrcpy( szTmp, szDestDrive );
    lstrcat( szTmp, pSrcChar );
    lstrcpy( GetAppDir(), szTmp );
    // Now, g_szAppDir is like that
    // "C:\Program files\Microsoft Games\A New Game"

    nResult = ExecuteAllCommands(prgRuntime, cCommands, fFirstTime,uFirstResID );

    //Unlock and free resources in prgRuntime
    FreeRuntimeResources( prgRuntime, cCommands );
    GlobalFreePtr( prgRuntime );

    if (EBU_OK == nResult)
    {
        TCHAR szBuf[MAX_PATH];
        if(GetWriteUninstall() == TRUE)
           MySetupUninstall();

        EBULoadString(GetResourceInst(), STR_REGKEY_VAL_LAUNCHED, szBuf, sizeof(szBuf) );
        MyWritePrivateProfileString( szBuf, "1");

		//
		//Write PID...
		//
		EBULoadString(GetResourceInst(), STR_REGKEY_VAL_PID, szBuf, sizeof(szBuf));
        MyWritePrivateProfileString( szBuf, GetPid());

		//
		//Write PlayerName if there is one...
		//
		if (*(GetPlayerName()))
		{
			EBULoadString(GetResourceInst(), STR_REGKEY_VAL_PLAYERNAME, szBuf, sizeof(szBuf));
			MyWritePrivateProfileString( szBuf, GetPlayerName());
		}

		//
		//Write the root game install directory to the InstalledPath regkey
		//
        EBULoadString(GetResourceInst(), STR_REGKEY_VAL_APPPATH, szBuf, sizeof(szBuf) );
        MyWritePrivateProfileString( szBuf, GetAppDir());

        StampVersionType(bTrial);
		// force write of group flag, it's critical for maintainence mode
        char tempGroup[64];
        EBULoadString(GetResourceInst(), STR_REGKEY_VAL_GROUP, szBuf, sizeof(szBuf) );
        sprintf(tempGroup,"%I64u",removeKeyboardTypeFlag(n_GroupList));
        MyWritePrivateProfileString(szBuf,tempGroup);
		
        // force write of LANGID for language we installed under
        {
           WORD LangID = LANG_ENGLISH;
           if (BLD_JPN & GetBuild())
           {
              LangID = LANG_JAPANESE;
           }
           else if (BLD_FRA & GetBuild())
           {
              LangID = LANG_FRENCH;
           }
           else if (BLD_GER & GetBuild())
           {
              LangID = LANG_GERMAN;
           }
           else if (BLD_USA & GetBuild())
           {
              LangID = LANG_ENGLISH;
           }
           else if (BLD_SPA & GetBuild())
           {
              LangID = LANG_SPANISH;
           }
           TCHAR temp[MAX_PATH];
           EBULoadString(GetResourceInst(), STR_REGKEY_VAL_LANGID, szBuf, sizeof(szBuf) );
           lstrcpy(temp,GetRegBase());
           lstrcat(temp,"\\");
           lstrcat(temp,szBuf);
           itoa(LangID,szBuf,10);
           MyWriteUniversalRegType(HKEY_LOCAL_MACHINE, temp, szBuf, REG_DWORD);
        }
    }
	//If reboot flag was set, but install wasn't successful, delete restart list...
    if (EBU_OK != nResult && GetRebootFlag())
    {
		DeleteRestartFiles();
    }

    if (EBU_ERROR == nResult || EBU_ABORT == nResult || EBU_BACK == nResult)
    {
        if (GetDirCreated())
        {
            ExecuteRdRoot(NULL);
            SetDirCreated(FALSE);
        }
    }

	if (nResult == EBU_OK)
	{
		ClearDirtyBits();

		ClearSharedDLL();
	}

    return nResult;
}

//****************************************************************************
// Procedure    MaintainApp
//
// Purpose      Put's setup into maintainence mode.  Allows user to change
//              file groups, MUST have a group callback handler in UI.
//
// Parameters   hWndParent      Parent window for MessageBox()
//              fFirstTime      Is this the 1st time the app's been launched?
//              bTrial          TRUE if we are installing trial version.
//
// Returns      nonzero to continue running the stub; zero to abort for
//              any reason.
//
// History
//  2/17/97 craigh - created from InstallApp
//
EBURETCODE MaintainApp(HWND hWndParent, BOOL bTrial, BOOL fForceReinstall)
{
    HRSRC               hRsrc = NULL;
    LPRUNTIMECOMMAND    prgRuntime;
    char                szDestDrive[_MAX_PATH];     //destination drive, e.g. "C:\"
    EBURETCODE          nResult = EBU_OK;

    int cCommands;      // number of commands in setup script
    UINT uFirstResID;   // resource ID of first command

    // Store some globals
	ASSERT(hWndParent);
    SetWndParent(hWndParent);
	SetTrial(bTrial);
	SetCopyIncomplete(FALSE);
    n_fMaintMode = TRUE;			// set maintainence mode flag, this is a critical flag
	SetForceReinstall(fForceReinstall);

    // If this is not the first time the app has been run, then assume "NO"
    // to the question: 'do you want to overwrite your xxx-language system
    // files with yyy-language?'
    g_nLanguageDefault = IDNO;

    if (!LoadTokenFile(hRsrc, &uFirstResID, &cCommands))
        return EBU_ERROR;

    if (cCommands == 0)
    {
      return EBU_OK;
    }

    // Calculate required disk space, build copy list
    prgRuntime = (LPRUNTIMECOMMAND)GlobalAllocPtr( GMEM_MOVEABLE | GMEM_ZEROINIT,
                                 sizeof(RUNTIMECOMMAND) * cCommands );
    if (!prgRuntime)
    {
      Alert(GetWndParent(), MB_OK | MB_ICONSTOP, STR_ERROR_NOMEMORY);
      return EBU_ERROR;
    }

    if (EBU_ERROR == CreateScriptList(prgRuntime, cCommands, uFirstResID))
    {
        //Unlock and free resources in prgRuntime
        FreeRuntimeResources( prgRuntime, cCommands );

        GlobalFreePtr( prgRuntime );

        Alert(GetWndParent(), MB_OK | MB_ICONSTOP, STR_ERROR_RESOURCEFAILURE);
        
        return EBU_ERROR;
    }

	//
	//Save these for use in AppGetFileSizeRequirements() function...
	//
	g_prgRuntime = prgRuntime;
	g_cCommands = cCommands;

    //
    // This confusing block of code does two things: 1) Gets the destination drive of the AppDir
    // 2) if g_szAppDir specifies a path without the driveName, then insert driveName into g_szAppDir.

    szDestDrive[0] = '\0';
    // if AppDir does not contain driveName, then default to WindowsDriveRoot
    LPSTR pSrcChar;
    CHAR szTmp[_MAX_PATH];
    CHAR ch0,ch1;

    ch0 = GetChFromAppDir(0);
    ch1 = GetChFromAppDir(1);
    if ( isalpha( ch0 ) && ch1 == ':' )
    {
        szDestDrive[0] = ch0;
        szDestDrive[1] = ch1;
        szDestDrive[2] = '\\';
        szDestDrive[3] = '\0';
        pSrcChar = GetLpChFromAppDir(2);
        if ( *pSrcChar == '\\' )
            pSrcChar = AnsiNext( pSrcChar );
    }
    else
    {
        // get drive name where Windows installed
        GetWindowsDirectory( szDestDrive, sizeof(szDestDrive) );
        ASSERT( isalpha(szDestDrive[0]) );
        pSrcChar = AnsiNext( szDestDrive );
        ASSERT( *pSrcChar == ':' );
        pSrcChar = AnsiNext( pSrcChar );
        ASSERT( *pSrcChar == '\\' );
        *pSrcChar = '\0';
        // szTmp is like that "C:"
        pSrcChar = GetAppDir();
        if ( *pSrcChar == '\\' )
            pSrcChar = AnsiNext( pSrcChar );
    }
    lstrcpy( szTmp, szDestDrive );
    lstrcat( szTmp, pSrcChar );
    SetAppDir(szTmp);
    // Now, g_szAppDir is like that
    // "C:\Program files\Microsoft Games\A New Game"

	int		i=0;
	CALLBACKDATA cbd;
	cbd.nID = SS_BEGINMAINTAIN;

	LPGETGROUP pGetGroup = new CGetGroup();

	//
	//Give the setup app a status callback...
	//
	(*(GetAppCallback())) ((void *) &cbd);
	// execute the group handler to update the groups selection

	nResult = ExecuteGetInstallGroups(pGetGroup,&prgRuntime[i],cCommands,prgRuntime,uFirstResID,FALSE);

	if (nResult == EBU_OK)
	{
		// first delete all files that are no longer in the selected groups
	    nResult = ExecuteAllUninstallCommands(prgRuntime, cCommands, FALSE);

		if(nResult == EBU_OK) // next install files that are not installed, but part of new group
		{
			ClearDirtyBits();

			nResult = ExecuteAllCommands(prgRuntime, cCommands, FALSE, uFirstResID);
		}
	}
	n_fMaintMode = FALSE; // to get engine out of maintainence mode this flag must be reset
    //Unlock and free resources in prgRuntime
    FreeRuntimeResources( prgRuntime, cCommands );
    GlobalFreePtr( prgRuntime );

    if (EBU_OK == nResult)
    {
        TCHAR szBuf[MAX_PATH];

		//
		//Write PlayerName if there is one...
		//
		if (*(GetPlayerName()))
		{
			EBULoadString(GetResourceInst(), STR_REGKEY_VAL_PLAYERNAME, szBuf, sizeof(szBuf));
			MyWritePrivateProfileString( szBuf, GetPlayerName());
		}

        char tempGroup[64];
        EBULoadString(GetResourceInst(), STR_REGKEY_VAL_GROUP, szBuf, sizeof(szBuf) );
        sprintf(tempGroup,"%I64u",removeKeyboardTypeFlag(n_GroupList));
        MyWritePrivateProfileString(szBuf,tempGroup);

		ClearDirtyBits();
    }

	cbd.nID = SS_ENDMAINTAIN; // do callback to tell UI we are done with this execution
    (*(GetAppCallback())) ((void *) &cbd);

    return nResult;
}

#if 0
EBURETCODE CheckFileSpaceRequirements()
{
	// This returns one of three things
	// EBU_OK		There was enough space
	// EBU_RETRY	There was not enough space but we were able to check ok
	// EBU_ERROR	There was an error getting space, engine unstable

	DWORD               dwClustersFree;     //free disk space (clusters)
    DWORD               dwBytesPerCluster;

	EBURETCODE
		ReturnCode = EBU_OK;
	//
	//Give the setup app a status callback...
	//
	CALLBACKDATA cbd;
	cbd.nID = SS_CHECKDISKSPACE;
	(*(GetAppCallback())) ((void *) &cbd);

	// if the user has used the /FORCE command line switch then we ignore free space checking and just go for it
	if (GetForceFreeSpace())
		return EBU_OK;
	
    // Sep.10,1997 02:45 by yutaka.
	// MEMO : I met the situation that the installed MFC42.DLL(e.g.1721344) is
	//        bigger than the CD's(e.g.94184). When this value
	//        (dwSystemClustersNeeded) become minus(e.g.-19), 
	//        dwSysKBNeeded will be underflow and will be a very big value 
	//        like 4GBytes...
	// 
	//        Anyway, I don't know the MFC42.DLL(size=1721344) is regal or not.
	// 
	if ((LONG) dwSystemClustersNeeded < 0 )
	{
		dwSystemClustersNeeded = 0;
	}

    // Check if we have enough free disk space
    if( (LONG) dwClustersNeeded > 0)
    {
        if (!MyGetDiskFreeSpace(szDestDrive, &dwBytesPerCluster,
                                &dwClustersFree))
        {
            Alert(GetWndParent(), MB_OK | MB_ICONSTOP, STR_ERROR_RESOURCEFAILURE);
            ReturnCode = EBU_ERROR;
        }else{

			DWORD dwBytesFree = dwClustersFree * dwBytesPerCluster;
			DWORD dwKBFree = dwBytesFree >> 10;
			DWORD dwBytesNeeded = dwClustersNeeded * dwBytesPerCluster;
			DWORD dwKBNeeded = dwBytesNeeded >> 10;

			char szSysDrive[_MAX_PATH];
			GetSystemDirectory(szSysDrive, _MAX_PATH);
			szSysDrive[3] = '\0';

			if(!MyGetDiskFreeSpace(szSysDrive, &dwBytesPerCluster,
								   &dwClustersFree))
			{
				Alert( GetWndParent(), MB_OK | MB_ICONSTOP, STR_ERROR_RESOURCEFAILURE);
				ReturnCode = EBU_ERROR;
			}else{
				dwBytesFree = dwClustersFree * dwBytesPerCluster;
				DWORD dwSysKBFree = dwBytesFree >> 10;
				DWORD dwSysBytesNeeded = dwSystemClustersNeeded * dwBytesPerCluster;
				DWORD dwSysKBNeeded = dwSysBytesNeeded >> 10;

				// TODO:  is it really possible for dwKBNeeded to be < 0? cuz that's what dwKBDiskRequired is set to at this point
				if ((LONG) dwKBNeeded < 0)
				{
					dwKBNeeded = 0;
				}

				if ((LONG) dwSysKBNeeded < 0)
				{
					dwSysKBNeeded = 0;
				}

				char app[2],drv[2];
				app[0] = szDestDrive[0];
				app[1] = '\0';
				drv[0] = szSysDrive[0];
				drv[1] = '\0';
				CharUpper(drv);
				CharUpper(app);
				if(app[0] == drv[0]) // system and game drive are same;
				{
					dwKBNeeded += dwSysKBNeeded;
					dwSysKBNeeded = 0;
				}
					
				if (dwKBFree < dwKBNeeded  || dwSysKBFree < dwSysKBNeeded)
				{
					// For our error message, make the drive "C" instead of "C:\"
					char szDrive[_MAX_PATH];
					if(!szDestDrive)
					{
						GetCurrentDirectory(_MAX_PATH,szDrive);
						szDrive[1] = 0;
					}
					else
					{
						ASSERT( isalpha(szDestDrive[0]) );
						szDestDrive[1] = 0;
					}

					// Make the parent window non-topmost, so that the user can
					// look at other stuff on their system.  -ks 4/7/95
					SetWindowPos( GetWndParent(), HWND_NOTOPMOST, 0, 0, 0, 0,
								  SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE );

					char
						szTemp[25];
					sprintf(szTemp, "%.2f", dwKBNeeded / 1000.0);

					if (dwSysKBFree < dwSysKBNeeded)
						  Alert( GetWndParent(), MB_OK | MB_ICONEXCLAMATION | MB_DEFBUTTON2,
							  STR_ERROR_NEEDSYSDISKSPACE, szSysDrive, szTemp);

					sprintf(szTemp, "%.2f", dwKBNeeded / 1000.0);
					if (dwKBFree < dwKBNeeded)
					   Alert( GetWndParent(), MB_OK | MB_ICONEXCLAMATION | MB_DEFBUTTON2,
						   STR_ERROR_NEEDDISKSPACE, ((szDestDrive) ? (LPCSTR)szDestDrive : szDrive), szTemp);

					ReturnCode = EBU_RETRY;
				}
			}
		}
	}

	return ReturnCode;

}
#endif

LPSETUPCOMMAND FindCommand(BYTE * lpBlob,LPSETUPCOMMAND * lpList, int i)
{
	int x=0;
	BYTE *lpCommand=NULL;
	lpBlob += sizeof(SETUPINFO);

	lpCommand = lpBlob;
	for(x=0;x< i;x++)
	{
		lpList[x] = (LPSETUPCOMMAND)lpCommand;
		lpCommand += sizeof(SETUPCOMMAND);
		lpCommand += *(DWORD *)(lpCommand);
		lpCommand += sizeof(DWORD);
	}
	return (LPSETUPCOMMAND)lpCommand;

}

void CheckMaxDirLength (LPMKDIR lpMkDir)
{
	char    szDest[_MAX_PATH];
	int		nLen;
	
	//
	//Copy the destination dir into the dest buffer and expand any tokens
	//
	lstrcpy(szDest, lpMkDir->GetMkDir());
	ReplaceStringTokens(szDest, _MAX_PATH);

	//
	//If there were no tokens in the specified dest dir string, then we assume
	//that the directory path specified is relative to the appdir (g_szAppDir)
	//so check the length of the specified dir name.
	//
	if (0 == lstrcmpi(szDest, lpMkDir->GetMkDir()))
	{
		nLen = lstrlen ( szDest );
		if ( nLen > GetMaxDirLen())
		{
			SetMaxDirLen(nLen);
		}
	}
}

EBURETCODE CreateScriptList(LPRUNTIMECOMMAND lprgRuntime, int cCommands, UINT uFirstResID)
{
    int            i;
    HRSRC          hRsrc;
    HGLOBAL        hglbCommand= (HGLOBAL) NULL;
    LPSETUPCOMMAND lpCommand;
	BYTE           *lpBlob;
	LPSETUPCOMMAND *lpCommandList;

    // Loop through each command and load its resource.  The command is added
    // to the "copy list" if:
    //      1) its OS flag matches the current OS, and...
    //      2) its version is more recent than the dest (for an INSTALLFILE), and...
    //
    // Adding the command to the copy list consists of making the lpSetupCommand
    // pointer of the appropriate entry in lprgRuntime point to the command's
    // data.
    //
	if (GetBinaryResource())
	{
		if( !(hRsrc = EBUFindResource(GetScriptInst(), "SETUPDATA", (LPCSTR)"SETUPBINARY" ))
			|| !(hglbCommand = EBULoadResource(GetScriptInst(), hRsrc )) )
		{
			//couldn't find or load the resource - this may not be an error
			//if we allow null commands.  do we??
			return EBU_ERROR;
		}

		lpBlob = (BYTE *)LockResource(hglbCommand);
		lpCommandList = new LPSETUPCOMMAND[cCommands];
		FindCommand(lpBlob,lpCommandList,cCommands);
	}

    for (i = 0; i < cCommands; i++)
    {
		ForwardMessages();

		if(!GetBinaryResource())
		{
           if( !(hRsrc = EBUFindResource(GetScriptInst(), MAKEINTRESOURCE(uFirstResID + (UINT)i), (LPCSTR)"SETUPCOMMAND" ))
             || !(hglbCommand = EBULoadResource(GetScriptInst(), hRsrc )) )
           {
               //couldn't find or load the resource - this may not be an error
               //if we allow null commands.  do we??
               return EBU_ERROR;
           }

           if( !(lpCommand = (LPSETUPCOMMAND)LockResource( hglbCommand )) )
           {
            //this really is an error
               return EBU_ERROR;
           }
		}
		else
		{
			lpCommand = lpCommandList[i];
		}
		
        //check if the command should be executed under the current OS and build
        if (ExecOnThisPlatform(lpCommand->dwBuildFlags))
        {
            if (lpCommand->wCommandID == SC_GETGROUP)
				SetGroup(TRUE);

			if (lpCommand->wCommandID == SC_MKDIR)
				CheckMaxDirLength ((LPMKDIR)(((BYTE *)lpCommand) + sizeof(SETUPCOMMAND)+sizeof(DWORD)) );

			lprgRuntime[i].hglbSetupCommand = hglbCommand;
			lprgRuntime[i].lpSetupCommand = lpCommand;
		}
	}

	if (GetBinaryResource())
	{
	   delete lpCommandList;
	}

	return EBU_OK;
}

//****************************************************************************
// Procedure    LoadTokenFile    [private]
//
// Purpose
//
// Parameters   
//              hRsrc-
//              ufirstResID-
//              cCommands-
//
// Returns      nothing
//
BOOL LoadTokenFile(HRSRC hRsrc, LPUINT uFirstResID, LPINT cCommands)
{
    HGLOBAL             hglbSetupInfo;
    LPSETUPINFO         lpSetupInfo;

	if(((hRsrc = EBUFindResource(GetScriptInst(),"SETUPDATA",(LPCSTR)"SETUPBINARY")) != NULL)
		&& (hglbSetupInfo = EBULoadResource( GetScriptInst(), hRsrc )) )
	{
		SetBinaryResource(TRUE);
    }
	else
	{
           Alert( GetWndParent(), MB_OK | MB_ICONSTOP, STR_ERROR_RESOURCEFAILURE);
           return FALSE;
	}
    if( !(lpSetupInfo = (LPSETUPINFO)LockResource( hglbSetupInfo )) )
    {
//        FreeResource( hglbSetupInfo );  // (optional)Win95 & NT auto free resources
        Alert( GetWndParent(), MB_OK | MB_ICONSTOP, STR_ERROR_RESOURCEFAILURE);
        return FALSE;
    }
    *cCommands = (int)(UINT)lpSetupInfo->wNumCommand;
    *uFirstResID = (UINT)lpSetupInfo->wFirstResID;
//    FreeResource( hglbSetupInfo );  // (optional)Win95 & NT auto free resources
    return TRUE;
}

//****************************************************************************
// Procedure    FreeRuntimeResources    [private]
//
// Purpose      Unlocks and frees resources locked by BuildCopyList.  Does
//              not free the array of RUNTIMECOMMANDs itself.  (This is not
//              necessary under Win32 but it still is for Win16.)  Each entry
//              in the array with a non-null hglbSetupCommand will be both
//              unlocked and freed.
//
// Parameters   lpRuntime       pointer to array of RUNTIMECOMMAND's.
//              cCommands       size of this array
//
// Returns      nothing
//
// History       1/31/95    KenSh       Created
//

void FreeRuntimeResources( LPRUNTIMECOMMAND lpRuntime, int cCommands )
{
#if 0
    int i;

    for( i = 0; i < cCommands; i++ )
    {
        if( lpRuntime[i].hglbSetupCommand )
        {
            FreeResource( lpRuntime[i].hglbSetupCommand );
        }
    }
#endif

    return;
}


//****************************************************************************
// Procedure    ShouldFileBeInstalled   [private]
//
// Purpose      Determines whether or not a particular file needs to get
//              installed by looking at the version/language info and the
//              file's timestamp.
//
// Parameters   pInstallFile        pointer to INSTALLFILE describing the file
//              pDestFileInfo       pointer to destination's file info, will be
//                                  filled if that file exists.  If not, the file
//                                  size is guaranteed to be zero.
//
// Returns      One of these values:
//
//              IVF_ERR_USERABORT       If the user clicked Abort
//              IVF_ERR_USERIGNORE      If the user clicked Ignore
//              IVF_ERR_NOMEMORY        Ran out of memory getting version info
//              IVF_SUCCESS_COPY        File should be copied
//              IVF_SUCCESS_NOCOPY      File should not be copied
//
// History       1/30/95    KenSh       Created
//               ?/??/95    StephHer    Added uninstall functionality
//               6/21/95    KenSh       Uninstall files on IF_UNINSTALL flag only
//           6/25/95 KenSh    Added detailed debug messages
//
UINT ShouldFileBeInstalled(LPINSTALLFILE pInstallFile, LPFILEINFO lpDestFileInfo, BOOL fFirstTime)
{
    BOOL fCopy = FALSE;
    BOOL fDone;
	BOOL fSystem = FALSE;
    UINT uVersionCompare;
    char szDestPath[_MAX_PATH];
    UINT cch;
    int  nAlertResult;
	
	//
    //Prepare the destination directory
	//
    if (!pInstallFile->fCopyToAppDir())
	{
		//if it is going to be copied to a system directory
		fSystem = TRUE;
		
		if (pInstallFile->fCopyToSystemDir())
		{
			// \windows\system
			cch = MyGetSystemDirectory(szDestPath, sizeof(szDestPath));
		}
		else
		{
			if (pInstallFile->fCopyToWindowsDir())
			{
				// \windows
				cch = MyGetWindowsDirectory(szDestPath, sizeof(szDestPath));
			}
			else
			{
				// if may be copied to a non-app dir, like the common dir.
				cch = 0;

				// ASSERT(FALSE);
				// used to be \windows\system32 but now SYSTEM32 directive is gone
			}
		}
		
		//now append the destination file name to the path
		lstrcpy(&szDestPath[cch], pInstallFile->GetDestFileName());
		ReplaceStringTokens(szDestPath, _MAX_PATH);
	}
	else
	{
		//
		//If we ever want version checking for non system files, change #if 1 to #if 0
		//
#if 1
		// non system files we will always install.
		return IVF_SUCCESS_COPY;
#else		
		//
		//application directory file
		//
		  lstrcpy(szDestPath, pInstallFile->GetDestPath());
		  ReplaceStringTokens(szDestPath, _MAX_PATH);
		  
			//
			//If there were no tokens in the specified dest dir string, then we assume
			//that the directory path specified is relative to the appdir (g_szAppDir)
			//so build the new dir name as appdir+specified dir
			//
			if (0 == lstrcmpi(szDestPath, pInstallFile->GetDestPath()))
			{
			wsprintf(szDestPath, "%s\\%s", GetAppDir(), pInstallFile->GetDestPath());
			}
#endif
    }
	
    fDone = FALSE;

	//
	//Now try to load the version information for the source file
	//Build the source filename
	//
	TCHAR szSource[_MAX_PATH];
    lstrcpy(szSource, pInstallFile->GetSourceFileName());  // added here 6/3/98 cjh
	ReplaceStringTokens(szSource, _MAX_PATH);
	// Build the source filename
	if (*(szSource+1) != ':') // full path is already present, don't prepend current path: cjh 08/08/97
	{
		GetModuleDirectory(szSource, sizeof(szSource));
	    lstrcat(szSource, pInstallFile->GetSourceFileName());
	}

	
//	GetModuleDirectory(szSource, sizeof(szSource)); // assumed file was always relative to setup, not anymore
	//lstrcat(szSource, pInstallFile->szName);
	
    while (!fDone)
    {
        uVersionCompare = VersionCompare(szSource,
										 &pInstallFile->FileInfo, 
										 szDestPath, 
										 lpDestFileInfo,
										 TRUE); // use the cached info, if any
		
        fDone = TRUE;   //may be set to FALSE below...
		
        if (uVersionCompare & VC_ERRORMASK)
        {
			//
            //Don't need to check for VC_ERR_NOEXIST2 or VC_ERR_CANTOPEN2
            //because we've already pre-determined the info that would
            //require lpszSrc to be opened (that's what lpSrcFileInfo is)
			//
            if (uVersionCompare & VC_ERR_NOEXIST)
            {
                lpDestFileInfo->dwFileSize = 0;
				
                //dest file doesn't exist, overwrite it
                fCopy = TRUE;
            }
            else if (uVersionCompare & VC_ERR_NOMEMORY)
            {
                return IVF_ERR_NOMEMORY;
            }
            else if (uVersionCompare & VC_ERR_CANTOPEN)
            {
                //It's hard to see how we would get this error.  This can only
                //happen if another task has the file open for writing, pretty
                //darn unlikely.
                nAlertResult = Alert(GetWndParent(), 
									 MB_ABORTRETRYIGNORE | MB_ICONEXCLAMATION | MB_DEFBUTTON2,
									 STR_ERROR_CANTREADDEST, 
									 (LPSTR) szDestPath);
				
                if (nAlertResult == IDABORT)
                {
                    return IVF_ERR_USERABORT;
                }
                else if (nAlertResult == IDIGNORE)
                {
                    return IVF_ERR_USERIGNORE;
                }
                else
                {
                    fDone = FALSE;  //force a retry
                }
            }
            else
            {
                //Just to be safe we'll avoid the copy if something else is wrong
                fCopy = FALSE;
            }
        }
    } //end while()
	
    if (!(uVersionCompare & VC_ERRORMASK))
    {
        if (uVersionCompare & VC_VER_GREATER)
        {
			//
			//Destination version is newer than source, don't overwrite it...
			//
			fCopy = FALSE;
        }
        else if (uVersionCompare & VC_VER_LESS)
        {
			//
            //destination version is older, overwrite it
			//
            fCopy = TRUE;
        }
        else if ((uVersionCompare & VC_VER_NONE) &&
			(uVersionCompare & VC_DATE_OLDER))
        {
			//
            //no version info but destination is older, overwrite it
			//
            fCopy = TRUE;
        }
        else if (uVersionCompare & VC_LANG_DIFFERENT)
        {
            ASSERT(uVersionCompare & VC_VER_EQUAL);
			
            //Languages don't match, ask the user if they want to overwrite (maybe)
			
			//
            //Don't ask if our version has no language info
			//
            if (!pInstallFile->FileInfo.dwLanguage)
            {
                fCopy = FALSE;
            }
            else if (IDYES == AskLanguageOverwrite(lpDestFileInfo->dwLanguage, 
				pInstallFile->FileInfo.dwLanguage))
            {
                fCopy = TRUE;
            }
            else
            {
                fCopy = FALSE;
            }
        }
        else if (uVersionCompare & VC_DATE_OLDER)
        {
            ASSERT(uVersionCompare & VC_VER_EQUAL);
			
			//
            //version info matches but destination is older, overwrite it
			//
            fCopy = TRUE;
        }
        else
        {	
			//
			//During reinstalls (fFirstTime == FALSE), then we do overwrite files with
			//equal version and equal date...
			//
			if (!fFirstTime &&
				uVersionCompare & VC_DATE_EQUAL &&
				(uVersionCompare & VC_VER_EQUAL || uVersionCompare & VC_VER_NONE))
			{
				fCopy = TRUE;
			}
			else
			{
				//Otherwise, don't overwrite equal or newer file
				fCopy = FALSE;
			}
        }
    }
	
    if (fCopy)
    {
        return IVF_SUCCESS_COPY;
    }
    else
    {
        if (GetRemovingApp() && pInstallFile->fCopyToAppDir() && pInstallFile->fIsUninstallFile())
		{
			return IVF_SUCCESS_COPY;
		}
        else
		{
			return IVF_SUCCESS_NOCOPY;
		}
    }
}


//****************************************************************************
// Procedure    ShouldFontBeInstalled   [private]
//
// Purpose      Determines whether or not a particular font needs to get
//              installed by looking at the font file and registry information.
//
// Parameters   pInstallFont        pointer to INSTALLFONT describing the font
//              pRunCommand         pointer to RUNTIMECOMMAND information for
//                                  this command.
//
// Returns      One of these values:
//
//              IVF_ERR_USERABORT       If the user clicked Abort
//              IVF_ERR_USERIGNORE      If the user clicked Ignore
//              IVF_ERR_NOMEMORY        Ran out of memory getting version info
//              IVF_SUCCESS_COPY        File should be copied
//              IVF_SUCCESS_NOCOPY      File should not be copied
//
// Note         This does not follow the recommended procedure for installing
//              fonts on Windows 95 and NT, partially because of the complexity
//              of that procedure and it's orientation toward office products
//              and partially because it has logical flaws for our purposes.
//              We assume the font files will always have the same name.
//              It is only implemented for Win32.
//
// History       8/22/95    a-DenSo     Created based on ShouldFileBeInstalled
//               1/12/96    WilliamW    Made NT font installation work &
//                                      copy file when either file or registry
//                                      info is missing.
//
UINT ShouldFontBeInstalled (LPINSTALLFONT pInstallFont, LPRUNTIMECOMMAND pRunCommand, BOOL fFirstTime)
{
    // check for file already present on system
    char szDestPath [_MAX_PATH];
    int cch = sizeof (szDestPath);
    pInstallFont->MakeDestFileName(szDestPath, &cch);

	BOOL
		bDestExists = (0xFFFFFFFF != GetFileAttributes(szDestPath));

    // find the registry information for fonts
    char szRegistryKey [256];
    if (GetOS() & OS_NTMASK) //NT gets font information from different key than WIN40
    {
        EBULoadString(GetResourceInst(), STR_REGKEY_NT_FONTS, szRegistryKey,
                    sizeof (szRegistryKey));
    }
    else
    {
        EBULoadString(GetResourceInst(), STR_REGKEY_WIN40_FONTS, szRegistryKey,
                    sizeof (szRegistryKey));
    }

    HKEY hkFonts;

    if (RegOpenKeyEx (HKEY_LOCAL_MACHINE, szRegistryKey, 0,
                      KEY_QUERY_VALUE, &hkFonts) != ERROR_SUCCESS)
    {
        // don't try copy if font information missing
        return IVF_SUCCESS_NOCOPY;
    }

    // check for this font installed
    char szBuff [256];
    DWORD cb = sizeof (szBuff);
    DWORD type;
    LPSTR lpszName = &pInstallFont->szName [pInstallFont->wNameOffset];

    LONG result = RegQueryValueEx (hkFonts, lpszName, 0, &type,
								   (BYTE FAR *) szBuff, &cb);

    RegCloseKey (hkFonts);

	//
	//BUGBUG:REVIEW:This code never checks the FONT version, why is that?  reizen...
	//

    //
    //Reinstall font if either file or registry info missing
    //
    if ((result == ERROR_SUCCESS) && (bDestExists))
    {
        // don't copy if both font file and registry present
        return IVF_SUCCESS_NOCOPY;
    }
    else
    {
        // copy and install font
        return IVF_SUCCESS_COPY;
    }
}


//****************************************************************************
// Procedure    AskLanguageOverwrite    [private]
//
// Purpose      Determines whether we should overwrite when two files have
//              the same version number but different language info.  If a
//              value was set in InitSetup(), that will be returned; otherwise
//              the user will be asked once per language if they want that
//              language overwritten by the new files.
//
// Parameters   dwLangCur       the language that might be overwritten
//              dwLangNew       the language that we're installing
//
// Returns      IDYES       If the new file should be installed
//              IDNO        If the old file should be left in place
//
// History       1/27/95    KenSh       Created
//
int NEAR AskLanguageOverwrite( DWORD dwLangCur, DWORD dwLangNew )
{
    // We keep track of up to 10 languages that are being replaced and
    // ask separately about separate languages.  We don't bother to
    // keep track of which languages are being copied, since those
    // will probably all be the same anyway.

    static DWORD rgdwAskedLanguage[10]; //languages we've asked about
    static int   rgnAnswers[10];        //answers to these 10 languages
    static BOOL  fInit = FALSE;
    int i;
    char szLangCur[128];
    char szLangNew[128];
    int nResult;

    //A default answer to this question will prevent the question from
    //being asked.
    if( g_nLanguageDefault )
    {
        return g_nLanguageDefault;
    }

    if( !fInit )
    {
        ZeroMemory( &rgdwAskedLanguage, sizeof(rgdwAskedLanguage) );
        fInit = TRUE;
    }

    //Check if we've already asked about this language overwrite
    for( i = 0; i < 10 && rgdwAskedLanguage[i]; i++ )
    {
        if( rgdwAskedLanguage[i] == dwLangCur )
        {
            //We've already asked; return the previous result
            return rgnAnswers[i];
        }
    }

    //This is the first time we've encountered this particular language,
    //so we'll ask the user.

    VerLanguageName( LOWORD(dwLangCur), szLangCur, sizeof(szLangCur) );
    VerLanguageName( LOWORD(dwLangNew), szLangNew, sizeof(szLangNew) );

    nResult = Alert(GetWndParent(),
                    MB_YESNO | MB_ICONEXCLAMATION,
                    STR_OVERWRITELANGUAGE,
                    (LPSTR)szLangCur,
                    (LPSTR)szLangNew );

	//
	//Find the first free element in the asked language list...
	//
    for( i = 0; i < 10 && rgdwAskedLanguage[i]; i++ );

    //Store this answer if we have room
    if( i < 10 )
    {
        rgdwAskedLanguage[i] = dwLangCur;
        rgnAnswers[i] = nResult;
    }

    return nResult;
}


//****************************************************************************
// Class        CSetupCommand
//
// Procedure    LoadCommandResource
//
// Purpose      Returns an HGLOBAL to the resource associated with the command
//              data for this command - which may be an INSTALLFILE, an
//              ADDINISTRING, or whatever.  The result of this func can be
//              passed to LockResource and cast to whatever type you want.
//
// Parameters   none
//
// Returns      HGLOBAL from EBULoadResource, or NULL if resource not found
//
// History       2/01/95    KenSh       Created
//
HGLOBAL CSetupCommand::LoadCommandResource()
{
    HRSRC hRsrc;

    if( (hRsrc = EBUFindResource( GetScriptInst(), MAKEINTRESOURCE(GetCommandResID()), (LPCSTR)"COMMANDDATA" )) )
    {
        return EBULoadResource( GetScriptInst(), hRsrc );
    }
    else
    {
        return NULL;
    }
}


BOOL HasAppEverBeenLaunched(BOOL bCheckApp)
{
    char szBuf[256];
    char szLaunched[256];
     BOOL fLaunched = FALSE;

    EBULoadString(GetResourceInst(), STR_REGKEY_VAL_LAUNCHED, szLaunched, sizeof(szLaunched) );

    MyGetPrivateProfileString( szLaunched, "0", szBuf, sizeof(szBuf) );

    fLaunched =  (BOOL)( szBuf[0] == '1' );
    if(!fLaunched)
       return(FALSE);
    if(bCheckApp)
    {
        char AppPath[256];
       EBULoadString(GetResourceInst(), STR_REGKEY_VAL_APPPATH, szLaunched, sizeof(szLaunched) );
       MyGetPrivateProfileString( szLaunched, "", AppPath, sizeof(AppPath) );
       EBULoadString(GetResourceInst(),STR_LAUNCHEXE, szLaunched, sizeof(szLaunched) );
       wsprintf(szBuf,"%s\\%s",AppPath,szLaunched);

       if(GetFileAttributes(szBuf) != 0xFFFFFFFF && fLaunched)
          return(TRUE);
       return(FALSE);
    }
    return(TRUE);
}



//****************************************************************************
// Procedure    MySetupUninstall
//
// Purpose      Adds the "Uninstall" string to the registry, as required
//              by the Windows 95 logo requirements.  Does nothing except
//              in Windows 95 or NT 4.0.
//
// Parameters   none
//
// Returns      nonzero if successful, zero if not.
//
// History       ?/??/95    StephHer    Created
//               6/22/95    KenSh       Removed hardcoded uninstall path
//               6/22/95    KenSh       DBCS-enabled
//
BOOL MySetupUninstall()
{
    DWORD   dwResult;
    DWORD   dwDisposition;
    HKEY    hkeyRoot;
    char    sz[260];
    char    szModFName[260];
    int     iLen;
    LPSTR   pchBackslash = NULL;

	wsprintf(szModFName, "\"%s\\", GetAppDir());

    //
    // Open Root Key
    //
    lstrcpy(sz, GetRegUninstall());
    lstrcat(sz, "\\");
	int
		iStringLen = 0;

	char
		szAppRegRoot[MAX_PATH] = "";

	iStringLen = EBULoadString(GetResourceInst(), STR_REGKEY_APP_REG_ROOT, szAppRegRoot, sizeof(szAppRegRoot) );
	ASSERT(iStringLen);

    
	while (pchBackslash = pszGetLast5C( szAppRegRoot ))
		*pchBackslash = ' ';	// turn the \'s into a space

	lstrcat(sz, szAppRegRoot);

    dwResult = RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                                sz,
                                0,
                                NULL,
                                REG_OPTION_NON_VOLATILE,
	                            KEY_WRITE,
                                NULL,
                                &hkeyRoot,
                                &dwDisposition);
    if (ERROR_SUCCESS != dwResult)
    {
        return FALSE;
    }

    EBULoadString(GetResourceInst(), STR_REGKEY_UNINSTALL_DISPLAY_NAME, sz, sizeof(sz));

    dwResult = RegSetValueEx(hkeyRoot,
                               (LPSTR)"DisplayName",
                               0,
                               REG_SZ,
                               (BYTE FAR *)sz,
                               lstrlen(sz) + 1);


    // Find the last backslash in the module filename
    pchBackslash = pszGetLast5C( szModFName );
    ASSERT( pchBackslash );

    // Append the uninstall program's name (after the backslash)
    pchBackslash = AnsiNext(pchBackslash);
    iLen = (int)(UINT)((DWORD)pchBackslash - (DWORD)(LPSTR)szModFName);
    if( EBULoadString(GetResourceInst(), STR_UNSETUPEXENAME, &szModFName[iLen], sizeof(szModFName) - iLen ) )
    {
		lstrcat(szModFName, "\" /runtemp");
        dwResult = RegSetValueEx(hkeyRoot,
                                   (LPSTR)"UninstallString",
                                   0,
                                   REG_SZ,
                                   (BYTE FAR *)szModFName,
                                   lstrlen(szModFName) + 1);
    }
    else
    {
        dwResult = ERROR_INVALID_PARAMETER;
    }

    RegCloseKey(hkeyRoot);

    return( ERROR_SUCCESS == dwResult );
}

//----------------------------------------------------------------------------
// Procedure    MyRefCountSharedDll
//
// Purpose      Increments or decrements the reference count of the given DLL.  Does
//              nothing if not Windows 95 or NT 4.0, and does nothing if the
//              given filename does not end in ".DLL".
//
// Parameters   lpszValue       filename of the DLL to ref count
//              fBumpRefCount          increment if true, decrement if false
//
// Returns      nonzero if successful, zero if the registry could not be
//              modified.
//
// History       ?/??/95    StephHer    Created
//               6/21/95    KenSh       Always change the reference count,
//                                      no matter what the file's extension is.
//               8/13/95    KenSh       Write DWORD instead of string to registry
//               3/28/97    v-richei    Changed to allow inc or dec of ref count
//
BOOL MyRefCountSharedDll(LPCSTR lpszValue, BOOL fBumpRefCount)
{
    char        szValue[20];    // plenty big for even a REG_SZ number
    HKEY        hkeyRoot;
    DWORD       dwRefCount = 0; // the reference count we write back
    DWORD       dwResult;       // result of reg functions
    DWORD       dwDisposition;
    DWORD       dwSize;
    DWORD       dwType;

    //
    // Open/Create SharedDlls Key
    //
    dwResult = RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                                GetRegSharedDLLs(),
                                0,
                                NULL,
                                REG_OPTION_NON_VOLATILE,
                                KEY_ALL_ACCESS,
                                NULL,
                                &hkeyRoot,
                                &dwDisposition);
    if (ERROR_SUCCESS != dwResult)
    {
        return FALSE;
    }

    //
    // check to see if there is a subkey by the name of the passed in value
    // already in the SharedDlls name space
    //
    dwSize = sizeof(szValue);
    dwResult = RegQueryValueEx(hkeyRoot,
                                 (LPSTR)lpszValue,
                                 NULL,
                                 (LPDWORD)&dwType,
                                 (LPBYTE)szValue,
                                 (LPDWORD)&dwSize);

    //
    // if the Key was located extract the number located there an increment it one
    // then write the results back to the szKey
    //
    if (dwResult == ERROR_SUCCESS)
    {
		//
        //Parse the result differently depending on the data type
		//
        if( dwType == REG_DWORD )
            // Little-endian DWORD, use its value directly
        {
            dwRefCount = *(LPDWORD)szValue;
        }
        else if( dwType == REG_SZ )
            // String representation of number, parse it.
        {
            dwRefCount = (DWORD)(UINT)atoi(szValue);
        }
        else if( dwType == REG_DWORD_BIG_ENDIAN )
            // most significant byte first.
        {
            dwRefCount = (BYTE)szValue[3] +
                         ((BYTE)szValue[2] << 8) +
                         ((BYTE)szValue[1] << 16) +
                         ((BYTE)szValue[0] << 24);
        }
        else if( dwType == REG_BINARY )
            // ooh boy.  Assume binary has least-significant byte first
        {
            dwRefCount = (BYTE)szValue[0] +
                         ((BYTE)szValue[1] << 8) +
                         ((BYTE)szValue[2] << 16) +
                         ((BYTE)szValue[3] << 24);
        }
        else
        {
            ASSERT(FALSE);  // what case did we miss?
            dwRefCount = 0;
        }

    }

    // Change the reference count.  Never let it go below zero though.
	dwRefCount += TRUE == fBumpRefCount ? 1 : (0 == dwRefCount ? 0 : -1);

    //
    // Write the updated value back to the registry
    //
    dwResult = RegSetValueEx(hkeyRoot,
                               lpszValue,
                               0,
                               REG_DWORD,
                               (LPBYTE)&dwRefCount,
                               sizeof(dwRefCount) );

    RegCloseKey(hkeyRoot);

    return TRUE;
}


//----------------------------------------------------------------------------
// Procedure    BytesToClusters
//
// Purpose      Converts the number of bytes in a file to the number of clusters
//              the file would use on either the system drive or the game drive
//
// Parameters   dwBytes         number of bytes in the file
//
// Returns      corresponding cluster count.
//
// History       8/22/95    a-DenSo     Created
//			     6/02/97    a-richei	Different for system and game drive
//
DWORD BytesToClusters (DWORD dwBytes, BOOL fSystemDrive)
{
	DWORD dwDummy;
	TCHAR szCurrentGameDrive[_MAX_PATH] = "";
	TCHAR szSystemDrive[_MAX_PATH] = "";
	static TCHAR szLastGameDrive[_MAX_PATH] = "";

	if ( fSystemDrive )
	{
		// going to system drive
		if ( !g_dwSystemBytesPerCluster )
		{
			// get size info for system drive
			GetSystemDirectory(szSystemDrive, _MAX_PATH);
			szSystemDrive[3] = '\0';

			MyGetDiskFreeSpace(szSystemDrive, &g_dwSystemBytesPerCluster, &dwDummy);
			ASSERT ((LONG) g_dwSystemBytesPerCluster > 0);
		}
		return (dwBytes / g_dwSystemBytesPerCluster) + 1;
	}
	else
	{
		// going to game drive (may be same drive as system but we don't worry about that)
		if ( !n_dwGameBytesPerCluster )
		{
		
			ASSERT('\0' != GetChFromAppDir(0));

			// MEMO : CopyMemory() requires byte length : Sep.05,1997 11:23 by yutaka.
			CopyMemory(szCurrentGameDrive, GetAppDir(), sizeof(TCHAR) * 3);

			// Get cluster size if we haven't already or if the game drive has changed from last time through
			if (!lstrlen(szLastGameDrive) || lstrcmpi(szLastGameDrive, szCurrentGameDrive))
			{
				lstrcpy(szLastGameDrive, szCurrentGameDrive);

				// get size info for game drive
				MyGetDiskFreeSpace(szCurrentGameDrive, &n_dwGameBytesPerCluster, &dwDummy);
				ASSERT ((LONG) n_dwGameBytesPerCluster > 0);
			}
		}

		// Calculate the cluster count required for byte count passed.
		return (dwBytes / n_dwGameBytesPerCluster) + 1;
	}
}



//****************************************************************************
// Procedure   LaunchApplication
//
// Purpose     Splits the command-line into separate arguments.  Arguments
//
// Parameters  
//
// Returns      nonzero if successful, zero if not
//
// History		v-richei 3/20/97 now accepts command line parameters too...
BOOL LaunchApplication(UINT uiExeStringID, UINT uiParamStringID)
{
    char szLaunched[256];
	char szAppName[256];
    char AppDirectory[256];
	char szParams[MAX_PATH];

    if (GetRebootFlag())
    {
        Alert(GetWndParent(), MB_OK | MB_ICONSTOP, STR_MUSTRESTART);
        return FALSE;
    }

	//
	//Get Directory where app was installed into AppDirectory
	//
    EBULoadString(GetResourceInst(), STR_REGKEY_VAL_APPPATH, szLaunched, sizeof(szLaunched) );
    MyGetPrivateProfileString( szLaunched, "", AppDirectory, sizeof(AppDirectory) );

	//
	//Append the EXE name to the AppDirectory
	//
    EBULoadString(GetResourceInst(), uiExeStringID, szLaunched, sizeof(szLaunched) );
    wsprintf(szAppName, "%s\\%s", AppDirectory, szLaunched);

	// if the exe string includes path information, Isolate it and tack it on to the
	// path in AppDirectory
	int iLen = strlen ( szLaunched ) - 1;
	while ( iLen )
	{
		if ( szLaunched[iLen] == '\\' )
		{
			szLaunched[iLen] = 0;
			break;
		}
		iLen--;
	}
	if ( iLen )
	{
		lstrcat ( AppDirectory, "\\" );
		lstrcat ( AppDirectory, szLaunched );
	}

	//
	//Get any command line parameters
	//
	EBULoadString(GetResourceInst(), uiParamStringID, szParams, MAX_PATH);

	if (GetFileAttributes(szAppName) != 0xFFFFFFFF)
	{
		//
		//Launch game.  If parameter string was empty, then EBULoadString would have returned a 
		//RESERR... message.  In that case, ensure that NULL is passed for the parm member instead
		//of RESERR...
		//
		if (EBU_ERROR != EBUShellExecute(GetWndParent(),
										 szAppName,
										 (strncmp(szParams, "RESERR", 6)) ? szParams : NULL,
										 AppDirectory,
										 SW_SHOWNORMAL,
										 EBUENGINE_SHELLEXECUTE,
										 STR_ERROR_INITFAILURE,
										 FALSE,  //don't wait
										 NULL))
		{
			return TRUE;
		}
	}

	return FALSE;
}

//****************************************************************************
// Procedure    StampVersionType
//
// Purpose      Writes value to VersionType Registry key; either RetailVersion or TrialVersion.
//
// Returns      nothing.
//
void StampVersionType(BOOL fbTrial)
{
    char vertype[50];
    char trial[50];
    DWORD dwResult;
    HKEY hkResult;

	// verify that our application registry key has been properly built
	ASSERT(GetRegBase());

    EBULoadString(GetResourceInst(),((fbTrial) ? STR_REGKEY_VAL_TRIALVERSION : STR_REGKEY_VAL_RETAILVERSION),vertype,sizeof(vertype));

    EBULoadString(GetResourceInst(),STR_REGKEY_VAL_VERSIONTYPE,trial,sizeof(trial));
    dwResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                          (LPSTR)GetRegBase(),
                          NULL,
                          KEY_SET_VALUE,
                          (HKEY FAR *)&hkResult);

    if( ERROR_SUCCESS == dwResult )
    {
        dwResult = RegSetValueEx(hkResult,
                      trial,
                      0,
                      REG_SZ,
                      (LPBYTE)vertype,
                      lstrlen(vertype)+1);
        RegCloseKey(hkResult);
    }
}

static BOOL CopySetupToHardDriveAndRunFromThere(LPSTR lpszCmdLine)
{
	TCHAR  szExePath[MAX_PATH];
	TCHAR  szTempExePath[MAX_PATH];
	TCHAR  szCmdLine[MAX_PATH];

	STARTUPINFO si;
	PROCESS_INFORMATION pi;

	//
	//Get the name of the current process .EXE (usually setup.exe or uninstal.exe)
	//
	GetModuleFileName(NULL, szTempExePath, sizeof(szTempExePath));
	
	if (0 == GetShortPathName(szTempExePath, szExePath, sizeof(szExePath)))
	{
		lstrcpy(szExePath, szTempExePath);
	}
	ASSERT(*szExePath);

	GetBootstrapFileName(szExePath, szTempExePath, "EXE");

	//
	//Make a copy of the current process .EXE
	//
	if (0 == EBUCopyFile(szExePath, szTempExePath, FALSE))
	{
		return FALSE;
	}

	//
	//Remove any read-only bit from our temp .EXE
	//
	SetFileAttributes(szTempExePath, FILE_ATTRIBUTE_NORMAL);

	//
	//Write a command line of the form: OriginalCommandLine /RunTemp:Path\Setup.Exe
	//  
	wsprintf(szCmdLine, 
			 __TEXT("\"%s\" %s /RUNTEMP:%s"), 
			 szTempExePath,
			 lpszCmdLine,
			 szExePath);

	//
	//Initialize data for the CreateProcess call...
	//
	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);

	//
	//Launch the Temporary .EXE which will run and be deleted by the batch file...
	//
	if (0 == CreateProcess(NULL, szCmdLine, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi))
	{
		DeleteFile(szTempExePath);

		return FALSE;
	}

	CloseHandle(pi.hThread);
	CloseHandle(pi.hProcess);

	return TRUE;
}

BOOL AppGetFileSizeRequirements(TCHAR tcDrive, 
								__int64 i64Group, 
								DWORD *pdwGameFreeSpace,
								DWORD *pdwGameNeeded,
								DWORD *pdwSystemFreeSpace,
								DWORD *pdwSystemNeeded)
{
	if (NULL == g_prgRuntime || 0 == g_cCommands)
	{
		return FALSE;
	}

	ASSERT(tcDrive);

	GetFileSizeRequirements(g_prgRuntime,
							g_cCommands,
							&tcDrive,
							pdwGameFreeSpace,
							pdwGameNeeded,
							pdwSystemFreeSpace,
							pdwSystemNeeded,
							i64Group,
							FALSE);

	return TRUE;
}


BOOL GetBootstrapFileName(TCHAR *pszSrcPath, TCHAR *pszDestPath, TCHAR *pszExtension)
{
	TCHAR szTryDrive[4];
	TCHAR szCurrDir[MAX_PATH];
	UINT  uiDriveType;
	DWORD dwBytesPerCluster;
	DWORD dwNumberOfFreeClusters;
	TCHAR cNextDrive;
	BOOL  fFirstPass = TRUE;
	BOOL  fGotDrive = FALSE;
	HANDLE hFile;

	struct _tstat ss;
	WIN32_FIND_DATA wfd;

	//
	//Get file size of file that we want to bootstrap...
	//
	if (0 != _tstat(pszSrcPath, &ss))
	{
		DisplaySystemError(GetLastError(), MB_OK | MB_ICONSTOP);
		return FALSE;
	}

	//
	//Save the current working directory...
	//
	GetCurrentDirectory(MAX_PATH, szCurrDir);

	//
	//Try temp directory first...
	//
	GetTempPath(_MAX_PATH, pszDestPath);
	
	//
	//Copy drive:\ plus NUL terminator...
	//
	EBUstrcpyn(szTryDrive, pszDestPath, 4);

	//
	//Disable operating system popups...
	//
	SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX);

	//
	//Check for fixed drives first, then removable drives...
	//
	for (int nDriveType = 0; nDriveType < 2 && FALSE == fGotDrive; nDriveType++)
	{
		cNextDrive = 'A';

		//
		//If temp path fails, check drives "A" through "Z"...
		//
		while ('Z' >= cNextDrive)
		{
			uiDriveType = GetDriveType(szTryDrive);
			
			//
			//First time only accept FIXED drives, second time removables are ok...
			//
			if ((UINT) (0 == nDriveType ? DRIVE_FIXED : DRIVE_REMOVABLE) == uiDriveType)
			{
				//
				//The first pass, we're trying the temp directory, after that we
				//try the root of the drive we're checking...
				//
				SetCurrentDirectory(FALSE == fFirstPass ? szTryDrive : pszDestPath);

				dwBytesPerCluster = dwNumberOfFreeClusters = 0;
				MyGetDiskFreeSpace(szTryDrive, 
								   &dwBytesPerCluster, 
								   &dwNumberOfFreeClusters);

				//
				//If there's enough free space on the drive...
				//
				if (dwBytesPerCluster * dwNumberOfFreeClusters > (DWORD) ss.st_size)
				{
					//
					//If there is a file named SETUP???.DLL...
					//
					hFile = FindFirstFile(strSETUPRESDLL, &wfd);
					
					if (INVALID_HANDLE_VALUE != hFile)
					{
						SetFileAttributes(wfd.cFileName, FILE_ATTRIBUTE_NORMAL);
						DeleteFile(wfd.cFileName);
						FindClose(hFile);
					}

					//
					//Ensure that we correctly deleted the existing SETUP???.DLL file...
					//
					hFile = FindFirstFile(strSETUPRESDLL, &wfd);
					
					if (INVALID_HANDLE_VALUE == hFile)
					{
						//
						//If we have write permissions to create a temp file in the directory...
						//
						if (0 != GetTempFileName(FALSE == fFirstPass ? szTryDrive : pszDestPath,
												 __TEXT("EBU"), 
												 0, 
												 pszDestPath))
						{
							//
							//Delete the temp file we created, because we want a file with
							//a different extension...
							//
							DeleteFile(pszDestPath);

							fGotDrive = TRUE;
							break;
						}
					}
					else
					{

						FindClose(hFile);
					}
				}
			}

			//
			//Now, let's check the next drive...
			//
			*szTryDrive = cNextDrive++;

			fFirstPass = FALSE;
		}
	}

	if (TRUE == fGotDrive && pszExtension)
	{
		TCHAR *pszPtr = pszDestPath;

		//
		//Change .TMP extension to specified extension...
		//
		while (*pszPtr)
		{
			pszPtr = CharNext(pszPtr);
		}
		pszPtr = CharPrev(pszDestPath, pszPtr);
		while (*pszPtr != '.')
		{
			pszPtr = CharPrev(pszDestPath, pszPtr);
		}
		pszPtr = CharNext(pszPtr);

		lstrcpy(pszPtr, pszExtension);
	}

	//
	//Restore original working directory...
	//
	SetCurrentDirectory(szCurrDir);

	SetErrorMode(0);

	return fGotDrive;
}
