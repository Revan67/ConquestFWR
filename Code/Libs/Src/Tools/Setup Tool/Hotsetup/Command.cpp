//
// command.cpp
//
//    Code to implement the various setup commands.
//
// History:
//
//     1/31/95 KenSh    Created
//     8/01/95 AjayJ    Changed TABs to spaces (Whoops!)
//                      ExecuteAddIniValue: When MapFlag is specified,
//                      examine IniSectionName to determine which
//                      registry HKEY to add string to.
//     8/03/95 KenSh    Don't restart windows in Win95/NT even if a system
//                      file was updated.
//     8/15/95 a-DenSo  Added SC_INSTALLFONT command support
//     9/02/95 a-DenSo  Added CustomInstall call after regular installation
//		03/15/97 update timestamp
//     03/28/97 v-richei  Don't use SHFileOperation, use O.S. CopyFile to avoid err dialogs

#include "stubpch.h"
#include "setup.h"
#include "hotsetup.h"
#include "command.h"
#include "util.h"
#include "uninstal.h"
#include "HotSetupRC.h"
#include "restart.h"
#include "registry.h"
#include "pid.h"
#include "progman.h"
#include "readfile.h"
#include "string.h"
#include <shlobj.h>
#include <ctype.h>
#include <olectl.h>
#include "block.h"
#include "mem.h"

// for font installation stuff
#include "FontUtils.h"
#include <fcntl.h>
#include <io.h>

typedef int (WINAPI * WINFARPROC) (HWND,LPSTR,DWORD);
using namespace NGLOBALS;

extern void InstDPLAYRegApp();


EBURETCODE extract_files(char *cabinet_name);
EBURETCODE InstDX(LPINSTDX, DIRECT_X_VERSION *);
EBURETCODE InstDPLAY( LPINSTDPLAY,DIRECT_X_VERSION *);
EBURETCODE UnInstDPLAY( LPINSTDPLAY,DIRECT_X_VERSION *);
//
//*** Local function declarations, some also used by ReadFile.Cpp
//
EBURETCODE ExecuteSetupCommand(LPRUNTIMECOMMAND lpRuntime,WORD cCommands,BOOL fFirstTime,UINT uFirstResID,LPRUNTIMECOMMAND prgRuntime  );
EBURETCODE ExecuteUninstallCommand( LPRUNTIMECOMMAND lpRuntime );
EBURETCODE ExecuteMaintainenceCommands(LPRUNTIMECOMMAND lpRuntime,WORD cCommands,UINT uFirstResID,LPRUNTIMECOMMAND prgRuntime  );
EBURETCODE ExecuteInstallFile( LPINSTALLFILE lpInstallFile );
EBURETCODE ExecuteInstallList( LPINSTALLFILE lpInstallFile );
EBURETCODE ExecuteInstallGo(void);
EBURETCODE ExecuteCabGo( LPCABGO lpCabGo);
EBURETCODE ExecuteRegWiz(LPREGWIZ lpRegWiz);
EBURETCODE ExecuteUnInstallFile( LPINSTALLFILE lpInstallFile );
EBURETCODE ExecuteAddIniValue(LPADDINIVALUE lpAddIniValue, BYTE bProcessType);
EBURETCODE ExecuteShellExecute(LPSHELLEXECUTE lpShellExecute, BYTE bProcessType);
EBURETCODE PreloadFont( LPINSTALLFONT lpInstallFont );
EBURETCODE ExecuteMkDir( LPMKDIR lpMkDir );
EBURETCODE ExecuteRdDir( LPMKDIR lpMkDir );
EBURETCODE ExecuteMkRoot( LPMKROOT lpMkRoot,LPRUNTIMECOMMAND lpRuntime, WORD cCommands, LPRUNTIMECOMMAND prgRuntime, UINT uFirstResID, BOOL fFirstTime);
EBURETCODE ExecuteGetName( LPGETNAME lpGetName );
EBURETCODE ExecuteGetPID( LPGETPID lpGetPid );
EBURETCODE ExecuteInstIcon( LPINSTICON lpInstIcon );
EBURETCODE ExecuteRemoveIcon( LPINSTICON lpInstIcon );
EBURETCODE ExecuteDeleteFile(LPDELETEFILE lpDeleteFile);
EBURETCODE MaintModeRemoveIcon(LPINSTICON  lpInstIcon);
EBURETCODE ExecuteInstDX( LPINSTDX lpInstDX );
EBURETCODE ExecuteInstDPLAY(LPINSTDPLAY lpInstDPLAY );
EBURETCODE ExecuteUnInstDPLAY(LPINSTDPLAY lpInstDPLAY, BYTE bProcessType );
void CountUninstallCommand( LPRUNTIMECOMMAND lpRuntime );
void MakeDestFontName (LPSTR lpPath, LPINT lpLength, LPSTR lpszName);
extern EBURETCODE ExecuteCDSpeed( LPCDSPEED lpCDSpeed );

EBURETCODE WINAPI FValidCDKey ( PTSTR szCDKey );
EBURETCODE FValidPidSerial ( LPSTR rgchSerial7 );
BOOL NEAR ReadTTFInfo(PSTR pszFile, LPSTR lpszVersion, LPSTR lpszFontName);

EBURETCODE WINAPI ValidateDirectoryAndResolvePath(PTSTR pszPath);
VOID NukeDirectPlayRemnants(void);
static VOID DeleteDirectPlayFile(TCHAR *szFile);
void RemoveWhiteSpaceFromFilename(LPSTR pStr);

BOOL GetFileSizeRequirements(LPRUNTIMECOMMAND prgRuntime, WORD cCommands, TCHAR *pszGameDrive, DWORD *dwGameKBytesFree, DWORD *dwGameKBytesNeeded, DWORD *dwSystemKBytesFree, DWORD *dwSystemKBytesNeeded,  __int64 filegroup, BOOL fFirstTime);
extern BOOL EBUGetFileTimeAndSize(LPCSTR lpszFile, time_t *pFileTime, DWORD *pdwFileSize);
void MakeStrLowercase(PTCHAR pszLowerStr);

typedef struct tagFileList {
	int CommandID;
	LPINSTALLFILE	plInstallFile;
	int	PassNo;
	int	ClustersFreed;
	int	Need;
	struct tagFileList *p[63];
}FILELIST, *LPFILELIST;

static char v_szLastIniFile[50];
static int v_nLastIniResult;
static FILELIST sStaticFiles, sAppFiles;
static char	szOldAppPath[MAX_PATH];


//****************************************************************************
// Procedure   ExecuteAllCommands
//
// Purpose     Runs through the command list prepared by GetFileSizeRequirements and
//             executes those commands which are needed.
//
// Parameters  
//             prgRuntime     array of runtime info about the commands
//             cCommands      size of the prgRuntime array
//             fFirstTime     flag for first time setup has been run
//
// Returns     zero to abort setup; nonzero to continue
//
// History      1/31/95 KenSh    Created
//
EBURETCODE ExecuteAllCommands(LPRUNTIMECOMMAND prgRuntime, int cCommands, BOOL fFirstTime, UINT uFirstResID)
{
	CALLBACKDATA cbd;
	int i;
	EBURETCODE
		nResult = EBU_OK;
	
	SetResultCode(nResult);
	
	*v_szLastIniFile = 0;
	cbd.nID = SS_BEGININSTALL;
	
	//
	//Give the setup app a status callback...
	//
	if(!n_fMaintMode) // call back status if not in maintainence mode, (we do that in another place in maint)
		(*(GetAppCallback())) ((void *) &cbd);
	
	// Run through all the commands and execute those that we need to
	for( i = 0; i < cCommands; i++ )
	{
		ForwardMessages();
		
		if( prgRuntime[i].lpSetupCommand )
		{
			if(n_fMaintMode) // if in maintainence mode only do maintainence commands
				nResult = ExecuteMaintainenceCommands(&prgRuntime[i], cCommands,
				uFirstResID, prgRuntime );
			else
				nResult = ExecuteSetupCommand(&prgRuntime[i], cCommands,
				fFirstTime, uFirstResID, prgRuntime );
			
			SetResultCode(nResult);
		}

		switch (nResult)
		{
		case EBU_ABORT:
		case EBU_ERROR:
			{
				goto Done;
			}break;
			
		case EBU_BACK:
			{
				if (0 == i)
				{
					// on first command with back so break out
					goto Done;
				}else{
					i -= 2;
				}
			}break;
		}
	}
	
Done:
#ifdef _DEBUG
	// check to make sure we're not losing any memory
	ClearMemoryRefs();
	CheckMemoryRefs();
#endif //_DEBUG
	cbd.nID = SS_ENDINSTALL;
	if(!n_fMaintMode) // call back status if not in maintainence mode
		(*(GetAppCallback())) ((void *) &cbd);
	
	//
	//Don't allow a cancel return from the engine. Cancel just means that 
	//the user or setup app chose to bypass a script command, not that
	//there was a failure...
	//
	return EBU_CANCEL == nResult ? EBU_OK : nResult;
}


//****************************************************************************
// Procedure   ExecuteSetupCommand
//
// Purpose     Executes a single setup command (without checking whether or
//             not it needs to be executed).
//
// Parameters  
//             lpRuntime      pointer to info about the command
//
// History      1/31/95 KenSh    Created
//
EBURETCODE ExecuteSetupCommand(LPRUNTIMECOMMAND lpRuntime,WORD cCommands,BOOL fFirstTime,UINT uFirstResID,LPRUNTIMECOMMAND prgRuntime  )
{
	LPSETUPCOMMAND lpCommand = lpRuntime->lpSetupCommand;
	ASSERT(lpCommand);
	
	HGLOBAL    hglbCommandData;
	LPVOID     lpvCommandData;
	EBURETCODE
		nResult = EBU_OK;

	if(!GetBinaryResource())
	{
		
		hglbCommandData = lpCommand->LoadCommandResource();
		ASSERT(hglbCommandData);
		
		lpvCommandData = LockResource( hglbCommandData );
		ASSERT(lpvCommandData);
	}
	else
	{
		lpvCommandData = (LPVOID)(((BYTE *)lpCommand) + sizeof(SETUPCOMMAND)+sizeof(DWORD));
		ASSERT(lpvCommandData);
	}

	//
	//Expose the flags passed in for this command...
	//
	SetCommandFlags(lpCommand->dwBuildFlags);

	switch( lpCommand->wCommandID )
	{
//	case SC_INSTALLFILE:
//		nResult = ExecuteInstallFile((LPINSTALLFILE)lpvCommandData );
//		break;
//		
	case SC_INSTALLLIST:
		nResult = ExecuteInstallList((LPINSTALLFILE)lpvCommandData );
		break;
		
	case SC_INSTALLGO:
		nResult = ExecuteInstallGo();
		break;
		
	case SC_CABGO:
		nResult = ExecuteCabGo((LPCABGO) lpvCommandData);
		break;
		
	case SC_REGWIZ:
		nResult = ExecuteRegWiz((LPREGWIZ)lpvCommandData );
		break;
		
		//Add a line to the given ini file
	case SC_ADDINIVALUE:
		nResult = ExecuteAddIniValue((LPADDINIVALUE)lpvCommandData, DO_INSTALL);
		break;
		
	case SC_SHELLEXECUTE:
		nResult = ExecuteShellExecute((LPSHELLEXECUTE) lpvCommandData, DO_INSTALL);
		break;

	case SC_MKDIR:
		nResult = ExecuteMkDir((LPMKDIR)lpvCommandData );
		break;
		
	case SC_MKROOT:
		nResult = ExecuteMkRoot((LPMKROOT)lpvCommandData,
			lpRuntime, cCommands, prgRuntime, uFirstResID, fFirstTime);
		break;
		
	case SC_GETGROUP:
		nResult = ExecuteGetInstallGroups((LPGETGROUP)lpvCommandData,lpRuntime,cCommands,prgRuntime,uFirstResID,fFirstTime);
		break;
		
#ifdef READFILE
	case SC_READFILELIST:
		nResult = ExecuteReadFileList((LPREADFILELIST)lpvCommandData,
			lpRuntime, cCommands, prgRuntime, uFirstResID, fFirstTime, DO_INSTALL);
		break;
#endif
		
	case SC_DELETEFILE:
		nResult = ExecuteDeleteFile((LPDELETEFILE)lpvCommandData);
		break;
		
	case SC_GETNAME:
		nResult = ExecuteGetName((LPGETNAME)lpvCommandData );
		break;
		
	case SC_GETPID:
		nResult = ExecuteGetPID((LPGETPID)lpvCommandData );
		break;
		
	case SC_INSTDX:
		{
			//
			//BUGBUG:REVIEW: 
			// This special EBU_BACK handler skips this engine command when EBU_BACK is returned by a engine command 
			// proceeding it.  The handler basically nullifies the command and either backs up 
			// another command or position to the start of the engine script.
			// Consequently, it prevents DirectX callbacks from occuring until EBU_BACK finds a command 
			// that can be re-executed. (a-petere)
			// It presumes that on a successful execution of this command that a subsequent pass will find
			// SAME or NEW DIRECT_X_VERSION check result and there for nullify the command.  This works as long 
			// as the /DXFORCE command is not specified from the command line.  In which case you can get 
			// many executions of this command. (a-petere)

			if (EBU_BACK == GetResultCode())
				return EBU_BACK;

			nResult = ExecuteInstDX((LPINSTDX)lpvCommandData );

			if (EBU_ERROR == nResult)
			{
				//
				// Failure to install direct X may not be fatal depending on the already installed
				// drivers and the version of the OS.  We are going to give them the benefit of the doubt
				//
				Alert(GetWndParent(), MB_OK | MB_ICONWARNING, STR_ERROR_DIRECTXSETUPFAILED, "%SETUPTITLE", "%SETUPTITLE");

				nResult = EBU_OK;
			}
		}
		break;
	case SC_INSTDPLAY:
		{
			//
			//BUGBUG:REVIEW: 
			// This special EBU_BACK handler skips this engine command when EBU_BACK is returned by a engine command 
			// proceeding it.  The handler basically nullifies the command and either backs up 
			// another command or position to the start of the engine script.
			// Consequently, it prevents DirectX callbacks from occuring until EBU_BACK finds a command 
			// that can be re-executed. (a-petere)
			// It presumes that on a successful execution of this command that a subsequent pass will find
			// SAME or NEW DIRECT_X_VERSION check result and there for nullify the command.  This works as long 
			// as the /DXFORCE command is not specified from the command line.  In which case you can get 
			// many executions of this command. (a-petere)

			if (EBU_BACK == GetResultCode())
				return EBU_BACK;

			nResult = ExecuteInstDPLAY((LPINSTDPLAY)lpvCommandData );

			if (EBU_ERROR == nResult)
			{
				//
				// Failure to install direct PLAY might not be fatal depending on the already installed
				// drivers and the version of the OS.  We are going to give them the benefit of the doubt
				//
				Alert(GetWndParent(), MB_OK | MB_ICONWARNING, STR_ERROR_DIRECTPLAYSETUPFAILED, "%SETUPTITLE", "%SETUPTITLE");

				nResult = EBU_OK;
			}
		}
		break;
		
	case SC_INSTICON:
		nResult = ExecuteInstIcon((LPINSTICON)lpvCommandData );
		break;
		
	case SC_CDSPEED:
		nResult = ExecuteCDSpeed((LPCDSPEED)lpvCommandData );
		break;
		
	default:
		Alert(GetWndParent(), MB_OK | MB_ICONSTOP, STR_ERROR_INVALIDCOMMAND);

#ifdef _DEBUG
		ASSERT(FALSE);
		nResult = EBU_OK;
#else
		nResult = EBU_ABORT;
#endif
		break;
	}

	return nResult;
}
//****************************************************************************
// Procedure   ExecuteMaintainenceCommands
//
// Purpose     Executes a single setup command used in maintainence mode
//             (without checking whether or not it needs to be executed).
//
// Parameters  
//             lpRuntime      pointer to info about the command
//
// History      1/31/95 KenSh    Created
//
EBURETCODE ExecuteMaintainenceCommands(LPRUNTIMECOMMAND lpRuntime,WORD cCommands,UINT uFirstResID,LPRUNTIMECOMMAND prgRuntime  )
{
	LPSETUPCOMMAND lpCommand = lpRuntime->lpSetupCommand;
	ASSERT(lpCommand);
	
	HGLOBAL    hglbCommandData;
	LPVOID     lpvCommandData;
	EBURETCODE
		nResult = EBU_OK;

	if(!GetBinaryResource())
	{
		
		hglbCommandData = lpCommand->LoadCommandResource();
		ASSERT(hglbCommandData);
		
		lpvCommandData = LockResource( hglbCommandData );
		ASSERT(lpvCommandData);
	}
	else
	{
		lpvCommandData = (LPVOID)(((BYTE *)lpCommand) + sizeof(SETUPCOMMAND)+sizeof(DWORD));
		ASSERT(lpvCommandData);
	}
	
	switch( lpCommand->wCommandID )
	{
	case SC_INSTALLLIST:
		nResult = ExecuteInstallList((LPINSTALLFILE)lpvCommandData );
		break;
		
	case SC_INSTALLGO:
		nResult = ExecuteInstallGo();
		break;
	
	case SC_CABGO:
		nResult = ExecuteCabGo((LPCABGO) lpvCommandData);
		break;
		
//    case SC_INSTALLFILE:
//		nResult = ExecuteInstallFile((LPINSTALLFILE)lpvCommandData );
//		break;

	case SC_INSTICON:
		nResult = ExecuteInstIcon((LPINSTICON)lpvCommandData );
		break;

	case SC_ADDINIVALUE:
        nResult = ExecuteAddIniValue((LPADDINIVALUE)lpvCommandData, DO_INSTALL);
		break;

	case SC_REGWIZ:
		nResult = ExecuteRegWiz((LPREGWIZ) lpvCommandData);
		break;

    case SC_MKDIR:
		nResult = ExecuteMkDir((LPMKDIR)lpvCommandData );
		break;

	case SC_INSTDX:
		{
			//
			//BUGBUG:REVIEW: 
			// This special EBU_BACK handler skips this engine command when EBU_BACK is returned by a engine command 
			// proceeding it.  The handler basically nullifies the command and either backs up 
			// another command or position to the start of the engine script.
			// Consequently, it prevents DirectX callbacks from occuring until EBU_BACK finds a command 
			// that can be re-executed. (a-petere)
			// It presumes that on a successful execution of this command that a subsequent pass will find
			// SAME or NEW DIRECT_X_VERSION check result and there for nullify the command.  This works as long 
			// as the /DXFORCE command is not specified from the command line.  In which case you can get 
			// many executions of this command. (a-petere)

			if (EBU_BACK == GetResultCode())
				return EBU_BACK;

			nResult = ExecuteInstDX((LPINSTDX)lpvCommandData );

			if (EBU_ERROR == nResult)
			{
				//
				// Failure to install direct X may not be fatal depending on the already installed
				// drivers and the version of the OS.  We are going to give them the benefit of the doubt
				//
				Alert(GetWndParent(), MB_OK | MB_ICONWARNING, STR_ERROR_DIRECTXSETUPFAILED, "%SETUPTITLE", "%SETUPTITLE");

				nResult = EBU_OK;
			}
		}
		break;
	case SC_INSTDPLAY:
		{
			//
			//BUGBUG:REVIEW: 
			// This special EBU_BACK handler skips this engine command when EBU_BACK is returned by a engine command 
			// proceeding it.  The handler basically nullifies the command and either backs up 
			// another command or position to the start of the engine script.
			// Consequently, it prevents DirectX callbacks from occuring until EBU_BACK finds a command 
			// that can be re-executed. (a-petere)
			// It presumes that on a successful execution of this command that a subsequent pass will find
			// SAME or NEW DIRECT_X_VERSION check result and there for nullify the command.  This works as long 
			// as the /DXFORCE command is not specified from the command line.  In which case you can get 
			// many executions of this command. (a-petere)

			if (EBU_BACK == GetResultCode())
				return EBU_BACK;

			nResult = ExecuteInstDPLAY((LPINSTDPLAY)lpvCommandData );

			if (EBU_ERROR == nResult)
			{
				//
				// Failure to install direct X may not be fatal depending on the already installed
				// drivers and the version of the OS.  We are going to give them the benefit of the doubt
				//
				Alert(GetWndParent(), MB_OK | MB_ICONWARNING, STR_ERROR_DIRECTPLAYSETUPFAILED, "%SETUPTITLE", "%SETUPTITLE");

				nResult = EBU_OK;
			}
		}
		break;

#ifdef READFILE
	case SC_READFILELIST:
		nResult = ExecuteReadFileList((LPREADFILELIST)lpvCommandData,
			lpRuntime, cCommands, prgRuntime, uFirstResID, FALSE, DO_INSTALL);
		break;
#endif
	case SC_DELETEFILE:
		nResult = ExecuteDeleteFile((LPDELETEFILE)lpvCommandData);
		break;

	case SC_SHELLEXECUTE:
//	case SC_INSTALLFONT:
    case SC_MKROOT:
    case SC_GETNAME:
	case SC_GETPID:
	case SC_CDSPEED:
	case SC_GETGROUP:
		nResult = EBU_OK;
		break;
	
   default:
		Alert(GetWndParent(), MB_OK | MB_ICONSTOP, STR_ERROR_INVALIDCOMMAND);

#ifdef _DEBUG
		ASSERT(FALSE);
		nResult = EBU_OK;
#else
		nResult = EBU_ABORT;
#endif
		break;
	}

	return nResult;
}

//****************************************************************************
// Procedure   ExecuteAllUninstallCommands
//
// Purpose     Runs through the command list prepared by GetFileSizeRequirements and
//             executes those commands which are needed.
//
// Parameters  
//             prgRuntime     array of runtime info about the commands
//             cCommands      size of the prgRuntime array
//             fFirstTime     flag for first time setup has been run
//
// Returns     zero to abort setup; nonzero to continue
//
// History      1/31/95 KenSh    Created
//
EBURETCODE ExecuteAllUninstallCommands(LPRUNTIMECOMMAND prgRuntime,
 int cCommands, BOOL fFirstTime )
{
	int		i;
	EBURETCODE nResult = EBU_OK;

	SetResultCode(nResult);

	CALLBACKDATA cbd;
	
	cbd.nID = SS_BEGINUNINSTALL;
	cbd.fUninstall = TRUE;

	//
	//Give the setup app a status callback...
	//
	if(!n_fMaintMode) // call back status if not in maintainence mode
	   (*(GetAppCallback())) ((void *) &cbd);


	// Run through all the commands and count file deletions
	for( i = cCommands-1; i >=0; i-- )
	{
		if( prgRuntime[i].lpSetupCommand )
		{
			CountUninstallCommand(&prgRuntime[i] );
		}
	}

	// Run through all the commands and execute those that we need to
	for( i = cCommands-1; i >=0; i-- )
	{
		if( prgRuntime[i].lpSetupCommand )
		{
			nResult = ExecuteUninstallCommand(&prgRuntime[i] );

			SetResultCode(nResult);

			if (EBU_ERROR == nResult || EBU_ABORT == nResult)
			{
				break;
			}
		}
	}
	cbd.nID = SS_ENDUNINSTALL;
	if(!n_fMaintMode) // callback end status if not in maintainence mode
	   (*(GetAppCallback())) ((void *) &cbd);

	//
	//Don't allow a cancel return from the engine. Cancel just means that 
	//the user or setup app chose to bypass a script command, not that
	//there was a failure...
	//
	return EBU_CANCEL == nResult ? EBU_OK : nResult;
}


//****************************************************************************
// Procedure   ExecuteUninstallCommand
//
// Purpose     Executes a single setup command (without checking whether or
//             not it needs to be executed).
//
// Parameters  
//             lpRuntime      pointer to info about the command
//
// Returns     zero to abort setup; nonzero to continue
//
// History      1/31/95 KenSh    Created
//
EBURETCODE ExecuteUninstallCommand(LPRUNTIMECOMMAND lpRuntime )
{
	LPSETUPCOMMAND lpCommand = lpRuntime->lpSetupCommand;
	ASSERT(lpCommand);

	HGLOBAL hglbCommandData;
	LPVOID lpvCommandData;
   if(!GetBinaryResource())
	{

		hglbCommandData = lpCommand->LoadCommandResource();
		ASSERT(hglbCommandData);
		
		lpvCommandData = LockResource( hglbCommandData );
		ASSERT(lpvCommandData);
	}
	else
	{
		lpvCommandData = (LPVOID)(((BYTE *)lpCommand) + sizeof(SETUPCOMMAND)+sizeof(DWORD));
		ASSERT(lpvCommandData);
	}

	EBURETCODE
		nResult = EBU_OK;

   if (n_fMaintMode) // if in maintainence mode skip all commands but install list and
   {				// readfile 
      switch( lpCommand->wCommandID )
      {
//		 case SC_INSTALLFILE:
	     case SC_INSTALLLIST:
		 case SC_READFILELIST:
         case SC_INSTICON:
         case SC_ADDINIVALUE:
         case SC_MKDIR:
         case SC_DELETEFILE:
		 case SC_INSTDPLAY:
			 break;
         case SC_MKROOT:
		 case SC_SHELLEXECUTE:
//         case SC_INSTALLFONT:
         case SC_GETNAME:
         case SC_GETPID:
         case SC_INSTDX:
         case SC_CDSPEED:
         case SC_INSTALLGO:
         case SC_REGWIZ:
         case SC_GETGROUP:
         case SC_CABGO:
            return EBU_OK;
         default:
            ASSERT(FALSE);
            nResult = EBU_ERROR;
            break;
      }
   }
	switch( lpCommand->wCommandID )
	{
		//Copy a file, version checking is already done
//		case SC_INSTALLFILE:
		case SC_INSTALLLIST:
			nResult = ExecuteUnInstallFile((LPINSTALLFILE)lpvCommandData );
			break;

		case SC_MKDIR:
			nResult = ExecuteRdDir((LPMKDIR)lpvCommandData );
			break;

		case SC_MKROOT:
			nResult = ExecuteRdRoot((LPMKROOT)lpvCommandData );
			break;

		case SC_INSTICON:  
			nResult = ExecuteRemoveIcon((LPINSTICON)lpvCommandData );
			break;

		case SC_DELETEFILE:
			nResult = ExecuteDeleteFile((LPDELETEFILE)lpvCommandData);
			break;

#ifdef READFILE
		case SC_READFILELIST:
			nResult = ExecuteReadFileList((LPREADFILELIST)lpvCommandData,
				NULL, 0, NULL, 0, FALSE, DO_UNINSTALL);
			break;
#endif //READFILE

         case SC_ADDINIVALUE:
            nResult = ExecuteAddIniValue((LPADDINIVALUE)lpvCommandData, DO_UNINSTALL);
			break;

		 case SC_SHELLEXECUTE:
			 nResult = ExecuteShellExecute((LPSHELLEXECUTE) lpvCommandData, DO_UNINSTALL);
			 break;

		 case SC_INSTDPLAY:
			 nResult = ExecuteUnInstDPLAY((LPINSTDPLAY) lpvCommandData, DO_UNINSTALL);
			 break;

//         case SC_INSTALLFONT:
         case SC_GETNAME:
         case SC_GETPID:
         case SC_INSTDX:
         case SC_CDSPEED:
		 case SC_INSTALLGO:
	     case SC_REGWIZ:
		 case SC_GETGROUP:
		 case SC_CABGO:
          nResult = EBU_OK;
            break;

		default:
			ASSERT(FALSE);
			nResult = EBU_ERROR;
			break;
	}

	return nResult;
}

//****************************************************************************
// Procedure   CountUninstallCommand
//
// Purpose     If the command will delete a file, increment the two global counters
//
// Parameters  
//             lpRuntime      pointer to info about the command

//
// History      9/11/98 a-nigelh    Created
//
void CountUninstallCommand(LPRUNTIMECOMMAND lpRuntime )
{
	LPSETUPCOMMAND lpCommand = lpRuntime->lpSetupCommand;
	ASSERT(lpCommand);

	HGLOBAL hglbCommandData;
	LPVOID lpvCommandData;

	if(!GetBinaryResource())
	{
		hglbCommandData = lpCommand->LoadCommandResource();
		ASSERT(hglbCommandData);
		
		lpvCommandData = LockResource( hglbCommandData );
		ASSERT(lpvCommandData);
	}
	else
	{
		lpvCommandData = (LPVOID)(((BYTE *)lpCommand) + sizeof(SETUPCOMMAND)+sizeof(DWORD));
		ASSERT(lpvCommandData);
	}

	switch( lpCommand->wCommandID )
	{
		//Copy a file, version checking is already done
//		case SC_INSTALLFILE:
		case SC_INSTALLLIST:
			{
			n_nFilesInUninstall++;
			n_nFilesToDelete++;
			}
            break;
	}

	return ;
}
// GetFileSizeRequirements
// NOTICE:  If the game is being installed to the same drive as the system drive, then 
//			dwSystemKBytesFree will come back as 0.  dwSystemKBytesFree is only set when the
//			system drive is not the same as the drive on which the game/app is being installed.
// New design
//	If sStaticFiles or sAppFiles empty (PassNo = 0)
//		scan filelist creating and linking a cell for each file
//  Then
//		get free space and cluster size for system and app drives
//		scan lists, according to filegroup. Use PassNo to eliminate
//		files already counted.
BOOL GetFileSizeRequirements(LPRUNTIMECOMMAND prgRuntime, WORD cCommands, TCHAR *pszGameDrive,
							 DWORD *dwGameKBytesFree, DWORD *dwGameKBytesNeeded,
							 DWORD *dwSystemKBytesFree, DWORD *dwSystemKBytesNeeded,
							 __int64 filegroup, BOOL fFirstTime)
{
    int             i;
    LPSETUPCOMMAND  lpCommand;
	time_t			sFileTime;
	DWORD			sFileSize;

	LPINSTALLFILE   pInstallFile;
	HGLOBAL         hglbInstallFile;
	BOOL			fSystem;
	clock_t			tStart, tEnd;
	
	int	AppClustersFreed = 0;
	int SysClustersFreed = 0;

	char szDestPath[MAX_PATH];

    *dwGameKBytesFree		= 0;    //total disk space available on the game drive
    *dwGameKBytesNeeded		= 0;    //total disk space needed for the game files
    *dwSystemKBytesFree		= 0;    //total disk space available on the sytem drive
    *dwSystemKBytesNeeded	= 0;    //total disk space needed for the system files

	//
	//Give the setup app a status callback...
	//
	CALLBACKDATA cbd;
	cbd.nID = SS_PREPARINGFILELIST;
	
	(*(GetAppCallback())) ((void *) &cbd);

/*timing profile*/	tStart = clock();
	
	n_dwSystemBytesPerCluster = 0;
	n_dwGameBytesPerCluster = 0;
	DWORD dwClustersFree;     //free disk space (clusters)


	//
	//Initialize to include space for drive, : and \
	//
	TCHAR szDrive[_MAX_PATH] = " :\\";
	
	char app[2],drv[2];

	ASSERT(pszGameDrive);
	ASSERT(*pszGameDrive);

	GetSystemDirectory(szDrive, _MAX_PATH);
	szDrive[3] = '\0';
	
	app[0] = *pszGameDrive;
	app[1] = '\0';
	drv[0] = szDrive[0];
	drv[1] = '\0';

	if (IsDBCS() && IsJapan())
	{
		// just for safe but...
		
		// Japanese Windows's CharUpper()/CharLower() converts
		// "\x??\x00" to "\x81\x45" (DBCS MIDDLE DOT)
		// ?? = 0x81-0x9F,0xE0-0xFC (DBCS Lead Byte)
		
		if ( ! IsDBCSLeadByte(*(unsigned char *)drv) ){
			CharUpper(drv);
		}
		if ( ! IsDBCSLeadByte(*(unsigned char *)app) ){
			CharUpper(app);
		}
	}
	else
	{
		CharUpper(drv);
		CharUpper(app);
	}

	// Get the amount of free space available on the system drive
	if (MyGetDiskFreeSpace(szDrive, 
						&n_dwSystemBytesPerCluster,
						&dwClustersFree))
	{
		if ( n_dwSystemBytesPerCluster >= 1024 )
			*dwSystemKBytesFree = (n_dwSystemBytesPerCluster >> 10) * dwClustersFree;
		else
			*dwSystemKBytesFree = dwClustersFree / ( 1024 / n_dwSystemBytesPerCluster);
	}
	else
	{
		*dwSystemKBytesFree = 0xFFFFFFFF;
	}

	// Set counts for game drive
	if(app[0] == drv[0]) // system and game drive are same;
	{
		// same as system drive so copy numbers
		*dwGameKBytesFree = *dwSystemKBytesFree;
		n_dwGameBytesPerCluster    = n_dwSystemBytesPerCluster;
	}
	else
	{
		// otherwise get the amount of free space available on the game destination drive
		szDrive[0] = *pszGameDrive;
	
		if( MyGetDiskFreeSpace(szDrive, &n_dwGameBytesPerCluster,
			&dwClustersFree) )
		{
			if ( n_dwGameBytesPerCluster >= 1024 )
				*dwGameKBytesFree = (n_dwGameBytesPerCluster >> 10) * dwClustersFree;
			else
				*dwGameKBytesFree = dwClustersFree / ( 1024 / n_dwGameBytesPerCluster);
		}
		else
			*dwGameKBytesFree = 0xFFFFFFFF;
	}	
	
	ASSERT ((LONG) n_dwGameBytesPerCluster > 0);
	ASSERT ((LONG) n_dwSystemBytesPerCluster > 0);
	
	// if PassNo is 0, we need to build the file list from the command list
	if ( sAppFiles.PassNo == 0 )
	{	
		LPFILELIST lpFileEntry;

		for (i = 0; i < cCommands; i++)
		{
			ForwardMessages();
		
			//
			//If not using a binary blob but instead using individual 
			//setup resources...
			//
			if (!GetBinaryResource())
			{
				//
				//a null hglbCommand means that this command is not used
				//on the current OS
				//things like not installing Direct X on NT 4.0
				//
				if (!prgRuntime[i].hglbSetupCommand) 	
				{
					//
					//Next iteration of for loop if NULL command...
					//
					continue;		
				}
			}
			else 
			{
				if (!prgRuntime[i].lpSetupCommand)
				{
					//
					//Next iteration of for loop if NULL command...
					//
					continue;
				}
			}
// a bulk malloc would be more efficient but would require a get cell function
			lpFileEntry = (LPFILELIST)malloc(sizeof(FILELIST));		//boundschecked reizen
			lpFileEntry->CommandID = i;
			lpFileEntry->PassNo = 0;
			lpFileEntry->ClustersFreed = 0;
			lpFileEntry->Need = 0;

			lpCommand = prgRuntime[i].lpSetupCommand;
			
			//
			//At this point we know the command must be at least looked at.
			//
			switch (lpCommand->wCommandID)
			{
//			case SC_INSTALLFILE:
			case SC_INSTALLLIST:
//			case SC_INSTALLFONT:
				if (!GetBinaryResource())
				{
					hglbInstallFile = lpCommand->LoadCommandResource();
					pInstallFile = (LPINSTALLFILE) LockResource(hglbInstallFile);
				}
				else
				{
					pInstallFile = (LPINSTALLFILE)(((BYTE *)lpCommand)+sizeof(SETUPCOMMAND)+sizeof(DWORD));
				}
				// I assume the pInstallFile is at a constant address
				lpFileEntry->plInstallFile = pInstallFile;
	
				__int64 group = pInstallFile->GetGroup();

				fSystem = pInstallFile->fCopyToWindowsDir() ||
						  pInstallFile->fCopyToSystemDir() ||
						  pInstallFile->fIsSystemFile() ||
						  pInstallFile->fIsSharedFile() ||
						  pInstallFile->fIsFontFile();

				// For every group that the file is a member of,
				// link the FileEntry to that groups list
				__int64 grouplist = group;

				// This while is controlled by left shift grouplist
				// We increment x to keep track of which bit we are currently
				// working with. 
				int x = 0;
				while ( grouplist )
				{
					if ( grouplist & 1 )
					{
						if (!fSystem) // game drive file
						{
							// Add file to sAppFiles
							lpFileEntry->p[x] = sAppFiles.p[x];
							sAppFiles.p[x] = lpFileEntry;
						}
						else
						{
							// For system files, we can do the 'size' and
							// 'Should we install' checks here, once.
							int cch = 0;

		
							// So find the destination
							if (pInstallFile->fIsFontFile())
							{
								cch = MAX_PATH;
								MakeDestFontName(szDestPath, &cch, pInstallFile->szName );
							}
							else if (pInstallFile->fCopyToSystemDir())
							{
								// \windows\system
								cch = MyGetSystemDirectory(szDestPath, sizeof(szDestPath));
							}
							else if (pInstallFile->fCopyToWindowsDir())
							{
									// \windows
								cch = MyGetWindowsDirectory(szDestPath, sizeof(szDestPath));
							}
							else
							{
								cch = 0;
							}
		
							//now append the destination file name to the path
							// but not for font files since that has been done
							if ( !pInstallFile->fIsFontFile())
								lstrcpy(&szDestPath[cch], pInstallFile->GetDestFileName());
							ReplaceStringTokens(szDestPath, _MAX_PATH);
							
							// Should we install
							if ( lpFileEntry->Need ) // we have already checked
							{
								// Add file to sStaticFiles
								lpFileEntry->p[x] = sStaticFiles.p[x];
								sStaticFiles.p[x] = lpFileEntry;

							}
							else if ( pInstallFile->fIsFontFile() )
							{
								char	lpszVersion[256];
								char	lpszFontname[256];
								if (0xffffffff != GetFileAttributes(szDestPath))
								{
									LPSTR lpStr = pszGetLast5C(szDestPath);
		
									//
									//convert filename to lowercase so can match against lowercase .ttf.  BUGBUG:REVIEW:
									//will hardcoded string .ttf be the same in other languages?
									//
									CharLower(lpStr);
									if (NULL != strstr(lpStr, ".ttf"))
									{
										char szDestVer[256];
			
										// Compare our font version with the already installed font version
										// First check the version info of the font we wish to copy
										if (!ReadTTFInfo(pInstallFile->szName, lpszVersion, lpszFontname))
										{
											if ( pInstallFile->wFlags & IF_CAB )
												goto QueueCopy;
											ASSERT(false);
											break;
										}
			
										// Then check version info of existing font in dest path
										if (!ReadTTFInfo(szDestPath, szDestVer, NULL))
										{
											goto QueueCopy; // because existing version is corrupt
										}
			
										TCHAR *pSrcVer;
										TCHAR *pDestVer;
			
										//
										//Look for the first numeric digit in the version string and assume that it
										//is the version number...
										//
										for (pSrcVer = lpszVersion; *pSrcVer; pSrcVer = CharNext(pSrcVer))
										{
											if (_istdigit(*pSrcVer))
											{
												break;
											}
										}
			
										for (pDestVer = szDestVer; *pDestVer; pDestVer = CharNext(pDestVer))
										{
											if (_istdigit(*pDestVer))
											{
												break;
											}
										}
			
										// if our font is an older version, skip it
										if (atof(pSrcVer) <= atof(pDestVer))
											// it exists in as good a version or better
										{
											break;
										}
										// need to copy it
									}
									// else it's not a ttf file so skip the version checks and copy it
								}
								// else the font does not exist already so copy it
								goto QueueCopy;
							}
							else if ( ShouldFileBeInstalled ( pInstallFile, &prgRuntime[i].FileInfo, fFirstTime ) == IVF_SUCCESS_COPY )
							{
QueueCopy:								lpFileEntry->Need = (pInstallFile->FileInfo.dwFileSize / 
									(n_dwSystemBytesPerCluster)) + 1;

								if ( EBUGetFileTimeAndSize(szDestPath, &sFileTime, &sFileSize ) )
								{
									lpFileEntry->ClustersFreed = 1 + sFileSize / n_dwGameBytesPerCluster;
								}
								// Add file to sStaticFiles
								lpFileEntry->p[x] = sStaticFiles.p[x];
								sStaticFiles.p[x] = lpFileEntry;
							}
							else
							{
								//SetDirtyBits(DIRTY_INSTALLFILE);

								if(pInstallFile->fIsSharedFile() && AddSharedDLL((LPSTR)szDestPath) )
								{
									MyRefCountSharedDll(szDestPath, TRUE);
								}

								//
								//Always DLL register flagged files, even if they were already
								//installed...
								//
								if (pInstallFile->fDLLRegister())
								{
									DLLRegister(szDestPath, DO_INSTALL);
								}

								prgRuntime[i].hglbSetupCommand = NULL;
								prgRuntime[i].lpSetupCommand = NULL;

								free ( lpFileEntry );
								break;
							}
						}
					}
					grouplist = grouplist >>1;
					x++;
				}
			}   // switch
		}
	}

	// See if the AppDir has changed. If so, we must check all of the app files
	// to see if they are overwriting existing files

	if ( lstrcmp ( GetAppDir(), szOldAppPath ) != 0 )
	{
		// save the new AppDir, the existence flag is maintained in ExecuteMkRoot
		// and the last PassNo used with the old AppDir
		lstrcpy ( szOldAppPath, GetAppDir() );
	}

	//
	//Flag whether the root directory still/already exists...
	//
    SetAppDirExists(0xFFFFFFFF != GetFileAttributes(GetAppDir()));

	sAppFiles.PassNo++;

	// Find down each list for the supplied filegroup
	__int64 grouplist = filegroup | 0x1;
	int x = 0;
	while ( grouplist )
	{
		if ( grouplist & 1 )
		{
			LPFILELIST	lpGroup = sAppFiles.p[x];

			// spin down a filelist
			while (lpGroup)
			{
				// Get the file size if we haven't counted this file already
				if (lpGroup->PassNo != sAppFiles.PassNo )
				{
					lpGroup->ClustersFreed = 0;

					// if the AppDir exists we need to test for
					// existing files being overwritten
					// if not, space freed has to be 0
					if ( GetAppDirExists() )
					{
						// The saved size may be invalid so recalculate
						// Get the destination
						lstrcpy(szDestPath, lpGroup->plInstallFile->GetDestPath());
						ReplaceStringTokens(szDestPath, _MAX_PATH);
	
						//
						//If there were no tokens in the specified dest dir string, then we assume
						//that the directory path specified is relative to the appdir (g_szAppDir)
						//so build the new dir name as appdir+specified dir
						//
						if (0 == lstrcmpi(szDestPath, lpGroup->plInstallFile->GetDestPath()))
						{
							wsprintf(szDestPath, "%s\\%s", GetAppDir(), lpGroup->plInstallFile->GetDestPath());
						}
						// Get the attributes/size of the destination, if any
						if ( EBUGetFileTimeAndSize(szDestPath, &sFileTime, &sFileSize ) )
						{
							// convert bytes to clusters
							lpGroup->ClustersFreed = 1 + sFileSize / n_dwGameBytesPerCluster;
						}
					}

					// Get source file clusters
					lpGroup->Need = (lpGroup->plInstallFile->FileInfo.dwFileSize / 
							   ( n_dwGameBytesPerCluster)) + 1;

					// Sum both requirements and space freed
					// Don't combine here because handling negative DWORD's is messy
					AppClustersFreed += lpGroup->ClustersFreed;
					*dwGameKBytesNeeded += lpGroup->Need;
					lpGroup->PassNo = sAppFiles.PassNo;
				}
				lpGroup = lpGroup->p[x];
			}
		}
		grouplist = grouplist>>1;
		x++;
	}

	// Repeat for the system files. Exactly the same logic but no need to
	// worry about changes in file sizes.
	sStaticFiles.PassNo++;

	grouplist = filegroup | 0x1;
	x = 0;
	while ( grouplist )
	{
		if ( grouplist & 1 )
		{
			LPFILELIST	lpGroup = sStaticFiles.p[x];

			while (lpGroup)
			{
				// Get the file size if we haven't counted this file already
				if (lpGroup->PassNo != sStaticFiles.PassNo )
				{
					lpGroup->PassNo = sStaticFiles.PassNo;
					*dwSystemKBytesNeeded += lpGroup->Need;
					SysClustersFreed += lpGroup->ClustersFreed;
				}
				lpGroup = lpGroup->p[x];
			}
		}
		grouplist = grouplist>>1;
		x++;
	}

	// normalize sizes into K (from clusters)
	// subtract space freed, ignoring negative answers

	if ( (int)*dwGameKBytesNeeded > AppClustersFreed )
		*dwGameKBytesNeeded -= AppClustersFreed;
	else
		*dwGameKBytesNeeded = 0;

	if ( (int)*dwSystemKBytesNeeded > SysClustersFreed )
		*dwSystemKBytesNeeded -= SysClustersFreed;
	else
		*dwSystemKBytesNeeded = 0;

	*dwGameKBytesNeeded = ((*dwGameKBytesNeeded * n_dwGameBytesPerCluster ) >>10) + (GetExtraAppBytes()>>10) + 1;
	*dwSystemKBytesNeeded = ((*dwSystemKBytesNeeded * n_dwSystemBytesPerCluster ) >>10) + (GetExtraSystemBytes()>>10) + 1;
	
	if(app[0] == drv[0]) // system and game drive are same;
	{
		*dwGameKBytesNeeded += *dwSystemKBytesNeeded;
		*dwSystemKBytesNeeded = 0;
	}

/**/	tEnd = clock();
/**/	tEnd -= tStart;
/**/	TRACE("GFSR:: Groups: %x  Time to execute: %d ticks\n", (int) filegroup, tEnd);

	// This is the only return so the value is irrelevant.
	return TRUE;
}

EBURETCODE ExecuteGetInstallGroups(LPGETGROUP lpGroup,LPRUNTIMECOMMAND lpRuntime,WORD cCommands,LPRUNTIMECOMMAND prgRuntime,UINT uFirstResID,BOOL fFirstTime)
{
	EBURETCODE retc = EBU_OK;
	GETGROUPDATA gp;
	
	//
	//Initialize group to last install type (if any)
	//
	gp.group = removeKeyboardTypeFlag(n_GroupList);
	
	//
	//Populate group size array before calling back setup app's InstallType screen...
	//
    GetFileSizeRequirements(prgRuntime, 
							cCommands, 
							GetAppDir(),
							&gp.dwGameFreeSpace, 
							&gp.dwGameNeeded, 
							&gp.dwSystemFreeSpace, 
							&gp.dwSystemNeeded, 
							gp.group, 
							fFirstTime);
	
	BOOL
		bDone = FALSE;
	while (!bDone)
	{
		//
		//Callback the setup app's InstallType handler...
		//
		retc = (*(GetAppCallback())) ((void *) &gp);
		
		switch (retc)
		{
			
		case EBU_OK:
			{
				SetOldGroupList(n_GroupList);
				//g_OldGroupList = g_GroupList;
				n_GroupList = addKeyboardTypeFlag(gp.group);

				retc = EBU_OK;

				if (!GetForceFreeSpace())
				{
				//
//				//BUGBUG:REVIEW:Is g_szAppDir *always* set when we call here.  If so, can get
//				//BUGBUG:REVIEW:rid of if (szDestDrive...) check below..  For testing this,
//				//BUGBUG:REVIEW:I put in the ASSERT that follows - reizen:12/18/97
//				//
//				ASSERT(GetChFromAppDir(0));
//				//xxGlob		ASSERT(g_szAppDir[0]);
//				
					char szDestDrive[_MAX_PATH];
					char szSysDrive[_MAX_PATH];
					GetSystemDirectory(szSysDrive, _MAX_PATH);
					lstrcpy(szDestDrive, GetAppDir());
				
					if(szDestDrive[1] == ':')
					{
						szDestDrive[1] = '\0';
					}
					if(szSysDrive[1] == ':')
					{
						szSysDrive[1] = '\0';
					}
					
				    GetFileSizeRequirements(prgRuntime, 
							cCommands, 
							GetAppDir(),
							&gp.dwGameFreeSpace, 
							&gp.dwGameNeeded, 
							&gp.dwSystemFreeSpace, 
							&gp.dwSystemNeeded, 
							n_GroupList, 
							fFirstTime);

					if (gp.dwGameFreeSpace < gp.dwGameNeeded  || gp.dwSystemFreeSpace < gp.dwSystemNeeded)
					{
						// Make the parent window non-topmost, so that the user can
						// look at other stuff on their system.  -ks 4/7/95
						SetWindowPos( GetWndParent(), HWND_NOTOPMOST, 0, 0, 0, 0,
									  SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE );

						char
							szTemp[25];
						sprintf(szTemp, "%.2f", gp.dwSystemNeeded / 1024.0);

						if (gp.dwSystemFreeSpace < gp.dwSystemNeeded)
							  Alert( GetWndParent(), MB_OK | MB_ICONEXCLAMATION | MB_DEFBUTTON1,
								  STR_ERROR_NEEDSYSDISKSPACE, szSysDrive, szTemp);

						sprintf(szTemp, "%.2f", gp.dwGameNeeded / 1024.0);
						if (gp.dwGameFreeSpace < gp.dwGameNeeded)
						   Alert( GetWndParent(), MB_OK | MB_ICONEXCLAMATION | MB_DEFBUTTON1,
							   STR_ERROR_NEEDDISKSPACE, szDestDrive, szTemp);

						retc = EBU_RETRY;
					}

					if ((EBU_OK == retc) || (EBU_ERROR == retc))
						bDone = TRUE;	// if we are either ok on size or there is an engine error
				}
			}break;
			
		case EBU_RETRY:
			{
				GetFileSizeRequirements(prgRuntime, cCommands, GetAppDir(), &gp.dwGameFreeSpace, &gp.dwGameNeeded, &gp.dwSystemFreeSpace, &gp.dwSystemNeeded, gp.group, fFirstTime);
			}break;
			
		case EBU_BACK:
			{
				//
				//Before returning, remove the directory we may have previously created
				//during ExecuteMkRoot...
				//
				ExecuteRdRoot(NULL);
				bDone = TRUE;
			}break;
			
		case EBU_ABORT:
			{
				bDone = TRUE;
			}break;
			
		default:
			{
				n_GroupList = 0xFFFFFFFFFFFFFFFF;
			}
		}
	}

	return retc;
}

//****************************************************************************
// Procedure   ExecuteInstallList
//
// Purpose     Executes the InstallList setup command.  Queues files to be copied
//					after setting up source and destination filenames. Copying is
//					done by ExecuteInstallGo
//
// Parameters  
//             lpInstallFile  pointer to an INSTALLFILE struct describing
//                            the source and destination files.
//
// Returns     nonzero if successful, zero if setup was aborted.
//

EBURETCODE ExecuteInstallList(LPINSTALLFILE lpInstallFile)
{
	char		  szSource[_MAX_PATH] = "";
	char		  szDest[_MAX_PATH];
	int			  cch = 0;
	LPCSTR		  ptr = NULL;
	INSTALLLISTDATA ild;
	LPINSTALLLIST NewElement = NULL;

	__int64 group = lpInstallFile->GetGroup();
	//
	//(If we aren't in maintenance mode or if we are in maintenance mode but force reinstall is set
	// AND
	//If group isn't group 1 and group isn't in user-selected group mask and group mask has been set)
	// OR
	//If we ARE in maintenance mode and group is 1 or group is in group mask
	if (n_fMaintMode)
	{
		if ((group & 0x1 || group & GetOldGroupList()) && !GetForceReinstall())
		{
			return EBU_OK;
		}
	}
	if (!(0x1 & group) && !(n_GroupList & group) && removeKeyboardTypeFlag(n_GroupList) > 0)
	{
		return EBU_OK;
	}

	// if this is a system file, there is a chance that we need not copy it
	// there is also a chance that we MUST not copy it, so check again before we add it to the copy list
	if ( lpInstallFile->fCopyToWindowsDir() ||
		 lpInstallFile->fCopyToSystemDir() ||
		 lpInstallFile->fIsSystemFile() ||
		 lpInstallFile->fIsSharedFile() ||
		 lpInstallFile->fIsFontFile())
	{
		int x = 0;
		BOOL	fCopyFile = FALSE;
		
		while ( group )
		{
			if ( group & 1 )
			{

				LPFILELIST	lpGroup = sStaticFiles.p[x];

				while (lpGroup)
				{
					if ( lpGroup->plInstallFile == lpInstallFile )
					{
						fCopyFile = TRUE;
						break;
					}
					lpGroup = lpGroup->p[x];
				}
			}

			if ( fCopyFile )
				break;

			group = group>>1;
			x++;
		}

		if ( ! fCopyFile )
		{
			return EBU_OK;
		}
	}


	// Build the dest filename
	if( lpInstallFile->fCopyToWindowsDir() )
	{
		cch = MyGetWindowsDirectory( szDest, sizeof(szDest) );
	}
	else if( lpInstallFile->fCopyToSystemDir() )
	{
		cch = MyGetSystemDirectory( szDest, sizeof(szDest) );
	}

	if (lpInstallFile->fIsFontFile())
	{
		cch = _MAX_PATH;
		MakeDestFontName (szDest, &cch, lpInstallFile->szName);
	}
	else if(!lpInstallFile->fCopyToAppDir())
	{
        lstrcpy( &szDest[cch], lpInstallFile->GetDestFileName() );
		ReplaceStringTokens(szDest, _MAX_PATH);
	}
	else
	{
		ptr = lpInstallFile->GetDestFileName();
#if defined( FIXFORUS ) // Jun.05,1997 14:20 by yutaka. o
		// MEMO : szName[] will have "Source\0Destination\0"
		// and GetDestFileName() will points 'D'.
		// I fixed this to points middle '\0'.
		if ( *ptr!='\0' && ptr!=&lpInstallFile->szName[0] ){
			ptr--; // points '\0'
		}
#endif
		ptr = AnsiPrev( &lpInstallFile->szName[0], ptr );
		while(*ptr != '\\' && ptr != &lpInstallFile->szName[0])
			ptr = AnsiPrev( &lpInstallFile->szName[0], ptr );
#if defined( FIXFORUS ) // Jun.05,1997 14:20 by yutaka. o
		// MEMO : szName[0] might have '\\'.
		if(*ptr == '\\')
#else
		if(ptr != &lpInstallFile->szName[0])
#endif
			ptr = AnsiNext(ptr);

		//
		//Copy the destination dir into the dest buffer and expand any tokens
		//
		lstrcpy(szDest, lpInstallFile->GetDestPath());
		ReplaceStringTokens(szDest, _MAX_PATH);

		//
		//If there were no tokens in the specified dest dir string, then we assume
		//that the directory path specified is relative to the appdir (g_szAppDir)
		//so build the new dir name as appdir+specified dir
		//
		if (0 == lstrcmpi(szDest, lpInstallFile->GetDestPath()))
		{
			wsprintf(szDest, "%s\\%s", GetAppDir(), lpInstallFile->GetDestPath());
		}
	}
	
	// Build the source filename
    lstrcpy(szSource, lpInstallFile->GetSourceFileName());
	ReplaceStringTokens(szSource, _MAX_PATH);
	// Build the source filename
	if (*(szSource+1) != ':') // full path is already present, don't prepend current path: cjh 08/08/97
	{
		GetModuleDirectory(szSource, sizeof(szSource));
	    lstrcat(szSource, lpInstallFile->GetSourceFileName());
		ReplaceStringTokens(szSource, _MAX_PATH);
	}

	char FileTo[_MAX_PATH];

	LPSTR from = szDest;
	LPSTR to = FileTo;
	LPSTR lpTmp1;
	LPSTR lpTmp2;
	while(*from != '\0')
	{
		if(*from == '\\')
		{
			lpTmp1 = AnsiNext(from);
			lpTmp2 = AnsiNext(lpTmp1);
			if(*lpTmp1 == '.' && *lpTmp2 == '\\')
				from = lpTmp2;
		}
		lpTmp1 = AnsiNext(from);
		while( from != lpTmp1 )
			*to++ = *from++;
	}
	*to = '\0';
	*(to+1) = '\0';
	if (IsDBCS())
	{
		CharPrev( FileTo, to );
	}
	else
	{
		to--;
	}

	while(to != FileTo)
	{
		if(*to == '\\')
		{
			*to++ = '\0';
			*to = '\0';
			break;
		}
		to = AnsiPrev( FileTo, to );
	}

	ild.pszFileName = szSource;
	ild.wFlags = lpInstallFile->wFlags;

	//
	//Callback the setup app's InstallList handler...
	//
	EBURETCODE retc = (*(GetAppCallback())) ((void *) &ild);
	ASSERT(EBU_OK == retc || EBU_CANCEL == retc || EBU_ABORT == retc);

	if (EBU_OK != retc)
	{
		return retc;
	}

	if (n_fMaintMode && !GetForceReinstall()) // Maintainence mode, only put file in list if it's not there
	{				 // you won't recopy files in multiple groups
		if(GetFileAttributes(szDest) != 0xFFFFFFFF)
			return EBU_OK;
	}
	
	// Allocate all the needed memory up front
	if (!FNewMemory((void **)&NewElement, sizeof(INSTALLLIST)))
	{
OOM:
		Alert(GetWndParent(), MB_ICONSTOP | MB_OK, STR_ERROR_NOMEMORY );
		return EBU_ERROR;
	}
	ZeroMemory(NewElement, sizeof(INSTALLLIST));

	if (!FNewMemory((void **)&NewElement->szSource, lstrlen(szSource)+1))
	{
		FreeMemory(NewElement);
		goto OOM;
	}

	if (!FNewMemory((void **)&NewElement->szDest, lstrlen(szDest)+1))
	{
		FreeMemory(NewElement->szSource);
		FreeMemory(NewElement);
		goto OOM;
	}

	if(NULL == GetListHead())
	{
		SetListHead(NewElement);
		SetListEnd(NewElement);
		NewElement->nodenum = 1;
		NewElement->nDiskID = lpInstallFile->GetDiskID();
	}
	else
	{
		GetListEnd()->nextElement = NewElement;
		NewElement->nodenum = GetListEnd()->nodenum+1;
		NewElement->nDiskID = lpInstallFile->GetDiskID();
		SetListEnd(NewElement);
	}

	lstrcpy(NewElement->szSource,szSource);
	lstrcpy(NewElement->szDest,szDest);

	NewElement->dwFileSize = lpInstallFile->FileInfo.dwFileSize;
	NewElement->wFlags = lpInstallFile->wFlags;

	return EBU_OK;
}

EBURETCODE ExecuteRegWiz(LPREGWIZ lpRegWiz)
{
	TCHAR			    szURL[_MAX_PATH];
	REGWIZDATA          rwd;

	lstrcpy(szURL, lpRegWiz->GetRegURL());
	
	ReplaceStringTokens(szURL, _MAX_PATH);
	
	EBURETCODE retc = EBU_OK;
	
	rwd.pszURL = szURL;

	while (TRUE)
	{
		retc = (*(GetAppCallback())) ((void *) &rwd);

		ASSERT(retc == EBU_ABORT || retc == EBU_OK || retc == EBU_BACK || retc == EBU_CANCEL);

		switch (retc)
		{
		case EBU_CANCEL:
		case EBU_ABORT:
			return EBU_OK;

		case EBU_OK:
			//
			//If the user clicked the Register Online button, fire up the web browser
			//to allow them to register...
			//
			retc = EBUShellExecute(GetWndParent(),
								   szURL,
								   NULL,
								   NULL,
								   SW_SHOWNORMAL,
								   EBUENGINE_SHELLEXECUTE,
								   0,
								   FALSE,
								   NULL);
			
			Alert(GetWndParent(),
				  (MB_OK | (EBU_ERROR == retc ? MB_ICONWARNING : MB_ICONINFORMATION)),
				  (EBU_ERROR == retc ? STR_ERROR_CANTSTARTREGWIZ : STR_REGWIZPROMPT),
				  szURL);

			return retc;

		case EBU_BACK:
			return retc;

		default:
			ASSERT(FALSE);
		}
	}
}

EBURETCODE ExecuteInstallGo(void)
{
	UINT           cch = 0;
	EBURETCODE     nCopyResult = EBU_OK;
	char		   FileTo[_MAX_PATH];
	char		   szSource[_MAX_PATH];
	int			   sourcelength=0;
	int			   destlength=0;
	int			   count = 0;
	DWORD		   dwTotalSize=0;
	LPINSTALLLIST  traverse;
	FILECOPYSTATUS fs;
	TCHAR		szSavedCurrentDirectory[_MAX_PATH];
	TCHAR		szWindowsDirectory[_MAX_PATH];
	LPSTR		pszFileName=NULL;
	DWORD		dwResult=0;
	BOOL		fRestoreDirectory=FALSE;

	// Need to set to a know valid temp directory for disk swaps.
	// But Save Current Directory
	dwResult = GetCurrentDirectory(sizeof(szSavedCurrentDirectory), szSavedCurrentDirectory);
	if (0 > dwResult || sizeof(szSavedCurrentDirectory) < dwResult)
	{
		return EBU_ABORT;
	}
	dwResult = GetWindowsDirectory(szWindowsDirectory, sizeof(szWindowsDirectory));
	if (0 > dwResult || sizeof(szSavedCurrentDirectory) < dwResult)
	{
		return EBU_ABORT;
	}
	if (!SetCurrentDirectory(szWindowsDirectory))
	{
		return EBU_ABORT;
	}
	else
	{
		fRestoreDirectory = TRUE;
	}

	// Register app with DPLAY lobby if INSTDPLAY already encountered
	// done here because we are now committed to do the copy
	InstDPLAYRegApp();

	if(NULL == GetListHead())
	{
		return EBU_OK;
	}

	traverse = GetListHead();
	do
	{
		dwTotalSize+=traverse->dwFileSize;
		traverse=traverse->nextElement;
		count++;

	} while(traverse != NULL);

	fs.dwTotalSize = dwTotalSize;
	fs.fDone = FALSE;
	fs.fCancelled = FALSE;
	traverse = GetListHead();
	SetListHead(NULL);
	SetListEnd(NULL);
	char *destptr = FileTo;
	char *sourceptr = szSource;
	LPINSTALLLIST lpLast;

	do
	{
		WORD	wFlags;
		//
		//Allow calling setup app to update its U.I., process messages, etc.
		//
		ForwardMessages();

		// This is the only place that disk swaps should occur at from script code.
		// don't ensure CDROM if this is not multi-disk - enhance performance ALOT
		if ((traverse->nDiskID != -1)&&(!EnsureCDROMInserted(traverse)))
		{
			nCopyResult = EBU_ABORT;
			goto cleanup;
		}

		destptr = FileTo;
		sourceptr = szSource;
		lstrcpy(sourceptr,traverse->szSource);
		sourceptr+=lstrlen(sourceptr)+1;
		*sourceptr='\0';
		lstrcpy(destptr,traverse->szDest);
		destptr+=lstrlen(destptr)+1;
		*destptr = '\0';
		fs.dwLastFile = traverse->dwFileSize;
		lpLast = traverse;
		traverse=traverse->nextElement;
		lstrcpy(fs.szSource,lpLast->szSource);
		lstrcpy(fs.szDest,lpLast->szDest);
		wFlags = lpLast->wFlags;
		FreeMemory(lpLast->szSource);
		FreeMemory(lpLast->szDest);
		FreeMemory(lpLast);
		lpLast = NULL;

		if ( wFlags & IF_FONTFILE )
		{
			// free the font so that the file copy can complete successfully
			RemoveFontResource (FileTo);
		}


		nCopyResult = MyCopyFile(szSource, FileTo, 
			(wFlags & IF_SHAREDFILE ? TRUE : FALSE),
			(wFlags & ( IF_WINDOWSDIR | IF_SYSTEMDIR | IF_SYSTEMFILE | IF_SHAREDFILE | IF_FONTFILE) ? TRUE : FALSE), &fs);

		if ( wFlags & IF_FONTFILE && EBU_OK == nCopyResult )
		{
			char	lpszVersion[256];
			char	lpszFontname[256] = " ";
			char	szTemp[16];
			LPSTR	lpStr = CharNext(pszGetLast5C(FileTo));

			SetDirtyBits(DIRTY_INSTALLFONT);

			//
			//convert filename to lowercase so can match against lowercase .ttf.  BUGBUG:REVIEW:
			//will hardcoded string .ttf be the same in other languages?
			//
			CharLower(lpStr);
			if (NULL != strstr(lpStr, ".ttf"))
			{
				// get fontName so we can store it in the registry later
				if (!ReadTTFInfo(szSource, lpszVersion, lpszFontname))
				{
#ifdef _DEBUG
					TRACE(STR_HARDCODE_NOSOURCEFONT,szSource);
#endif
					nCopyResult = EBU_ERROR;
					break;
				}

				// If we didn't retrieve our fontName from the file, then let's use the fontFile's name
				if (' ' == lpszFontname[0])   
				{
					lstrcpy(lpszFontname, &lpStr[1]);
				}
	
				// find the registry information for fonts
				char szRegistryKey [_MAX_PATH];
				if (GetOS() & OS_NTMASK)
				{
					//NT gets font information from different key than WIN40
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
					KEY_SET_VALUE, &hkFonts) != ERROR_SUCCESS)
				{
					nCopyResult = EBU_ERROR;
					break;
				}
		
				EBULoadString(GetResourceInst(), STR_TRUETYPE, szTemp, 16);
				lstrcat(lpszFontname, szTemp);
		
				// add this font to the registry
				RegSetValueEx (hkFonts, lpszFontname, 0, REG_SZ,
					(BYTE FAR *) lpStr, lstrlen (lpStr) + 1);
				RegCloseKey (hkFonts);
		
				// also install the font for immediate use
				AddFontResource (FileTo);
				SendMessage(HWND_BROADCAST, WM_FONTCHANGE, 0, 0);
			}
			else	// not .ttf file
				ASSERT (false);
		}
		else if ( wFlags & ( IF_WINDOWSDIR | IF_SYSTEMDIR | IF_SYSTEMFILE | IF_SHAREDFILE ) )
		{
			if (EBU_OK == nCopyResult)
			{
				SetDirtyBits(DIRTY_INSTALLLIST);

				if (wFlags & IF_DLLREGISTER)
				{
					// Register the DLL if the copy was successful and if flagged to do so...
					DLLRegister(FileTo, DO_INSTALL);
				}
			}
		}

		if (EBU_OK == nCopyResult)
		{
			SetDirtyBits(DIRTY_INSTALLLIST);
		}

	} while (traverse != NULL && nCopyResult == EBU_OK);

cleanup:
	if (nCopyResult != EBU_ABORT && fRestoreDirectory)
	{	
		//Make Sure a disk was reinserted before restoring the directory
		if (EnsureCDROMInserted(NULL))
		{
			//Verify we could restore the directory
			if (!SetCurrentDirectory(szSavedCurrentDirectory))
			{
				DWORD dwLastError = GetLastError();

				if (NO_ERROR != dwLastError)
				{
					DisplaySystemError(dwLastError, MB_OK | MB_ICONINFORMATION);
				}

				nCopyResult = EBU_ABORT;
			}
		}
		else
		{
			nCopyResult = EBU_ABORT;
		}
	}
	//
	//Ensure list gets cleaned up in an abort situation
	//
	while (traverse)
	{
		ForwardMessages();

		lpLast = traverse;
		FreeMemory(traverse->szSource);
		FreeMemory(traverse->szDest);
		traverse = traverse->nextElement;
		FreeMemory(lpLast);
		lpLast = NULL;
		// Clean-Up the global
		SetListHead(NULL);
	}

	//
	//If we canceled as a result of a user response to an error code in MyCopyFile(),
	//call back the application one last time with the total copied equal to the total
	//size.  Several setup apps use this flag to determine that the copy process is
	//complete...
	//
	fs.dwTotalCopied = fs.dwTotalSize;
	fs.fDone = TRUE;
	fs.fCancelled = (EBU_OK == nCopyResult ? FALSE : TRUE);
	(*(GetAppCallback())) ((void *) &fs);

	return nCopyResult;
}

EBURETCODE ExecuteCabGo(LPCABGO lpCabGo)
{
	LPINSTALLLIST  traverse = NULL;
	traverse = GetListHead();
	if (NULL == traverse)
	{
		return EBU_OK;
	}
	else
	    return extract_files(lpCabGo->GetCabName());
}

EBURETCODE ExecuteDeleteFile(LPDELETEFILE lpDeleteFile)
{
	WIN32_FIND_DATA FindData;
	HANDLE          hFind;
	BOOL            bFindFile = TRUE;
	char			szFileName[_MAX_PATH];
	char			szDir[_MAX_PATH];
	DELETEFILEDATA  dfp;

	if ( GetRemovingApp() && lpDeleteFile->fIsPersistFile() )
	{
		return EBU_OK;
	}

	if ( !GetRemovingApp() && !lpDeleteFile->fIsInstallFile() )
	{
		return EBU_OK;
	}

	lstrcpy(szFileName, lpDeleteFile->GetDeleteFileName());
	ReplaceStringTokens(szFileName, _MAX_PATH);
	if(szFileName[1] != ':')
	{
		wsprintf(szDir,"%s\\%s",GetAppDir(),szFileName);
		lstrcpy(szFileName,szDir);
	}

	lstrcpy(szDir,szFileName);
	// MEMO : Jun.11,1997 19:26 by yutaka. o
	// Currently *to points '\0'. This is okay also for DBCS.
	// Please do not 'lstrlen()-1'.
	// If you want to do, please use CharPrev().
	char *to = szDir+lstrlen(szDir);
	while(to != szDir)
	{
		if(*to == '\\')
		{
			*to++ = '\0';
			*to = '\0';
			break;
		}
		to = AnsiPrev( szDir, to );
	}

	//
	//If the script supplied Uninstall flag is set, then that means no prompt for delete...
	//
	dfp.fPromptToDelete = lpDeleteFile->GetDeleteFileSilentFlag() ? FALSE : TRUE;

	dfp.fUninstall = TRUE;
	dfp.pszPathName = szFileName;

	EBURETCODE retc = (*(GetAppCallback())) ((void *) &dfp);

	ASSERT(EBU_OK == retc || EBU_ABORT == retc || EBU_CANCEL == retc);

	if (EBU_OK != retc)
	{
		return retc;
	}
	
	if ( lpDeleteFile->fIsRecurseFile() )
	{
		DelFileInTree ( szFileName );
	}
	else
	{
		hFind = FindFirstFile(szFileName, &FindData);
		while((INVALID_HANDLE_VALUE != hFind) && bFindFile)
		{
			if(*(FindData.cFileName) != '.')
			{
				char szFile[_MAX_PATH];
				lstrcpy(szFile,szDir);
				lstrcat(szFile,"\\");
				if(*FindData.cAlternateFileName != '\0')
					lstrcat(szFile,FindData.cAlternateFileName);
				else
					lstrcat(szFile,FindData.cFileName);
				SetFileAttributes(szFile,FILE_ATTRIBUTE_NORMAL);
	
				//delete the file
				DeleteFile(szFile);
			}
			//find the next file
			bFindFile = FindNextFile(hFind, &FindData);
		}

		if(hFind != INVALID_HANDLE_VALUE)
			FindClose(hFind);
	}

	return EBU_OK;
}

//****************************************************************************
// Procedure   ExecuteUnInstallFile
//
// Purpose     Executes the InstallFile setup command.  Copies the file
//             to its destination without checking version or anything (that
//             should have already been done by now.)  If the file cannot
//             be opened for writing, this function will take the appropriate
//             action -- either adding the file to the list of things to
//             copy on reboot, or prompting the user to close the file.
//
// Parameters  
//             lpInstallFile  pointer to an INSTALLFILE struct describing
//                            the source and destination files.
//
// Returns     nonzero if successful, zero if setup was aborted.
//
// History      1/31/95 KenSh    Created
//              8/03/95 KenSh    Don't restart windows in Win95/NT
//              8/13/95 KenSh    Disable system error dialogs while copying
//                               the file.
//              8/24/95 a-DenSo  Allow full path specification passed in
//				9/11/98 a-nigelh Use fcs to return percentage progress
EBURETCODE ExecuteUnInstallFile(LPINSTALLFILE lpInstallFile )
{
	FILECOPYSTATUS fcs;
	char     szDest[_MAX_PATH];
	UINT     cch = 0;

	ForwardMessages();

	// Overload TotalSize and TotalCopied to return progress percentage
	// Use TotalSize for number of files to delete
	// Use TotalCopied for number of files to be deleted
	fcs.dwTotalSize = n_nFilesInUninstall;
	fcs.dwTotalCopied = n_nFilesInUninstall - (n_nFilesToDelete > 0 ? n_nFilesToDelete-- : 0 );

	if (n_fMaintMode) // only delete files not in the current group, don't ever delete
	{				 // group 1 files
		__int64 group = lpInstallFile->GetGroup();
		if ((group & 0x1) || (group & n_GroupList))
			return EBU_OK;
	}

	// Build the dest filename
	if( lpInstallFile->fCopyToWindowsDir() )
	{
		cch = MyGetWindowsDirectory( szDest, sizeof(szDest) );
	}
	else if( lpInstallFile->fCopyToSystemDir() )
	{
		cch = MyGetSystemDirectory( szDest, sizeof(szDest) );
	}
	else if( lpInstallFile->fIsFontFile() )
	{
		// Since we can't hope to uninstall a font file, bail out here
		return EBU_OK;
	}

    if(!lpInstallFile->fCopyToAppDir())
	{
        lstrcpy( &szDest[cch], lpInstallFile->GetDestFileName() );
		ReplaceStringTokens(szDest, _MAX_PATH);
	}
    else
	{
		ASSERT(0 == cch);
		ASSERT(*(char *)lpInstallFile->GetDestFileName() != 0);
		ASSERT(!lpInstallFile->fCopyToWindowsDir());
		ASSERT(!lpInstallFile->fCopyToSystemDir());

		//
		//Copy the destination dir into the dest buffer and expand any tokens
		//
		lstrcpy(szDest, lpInstallFile->GetDestPath());
		ReplaceStringTokens(szDest, _MAX_PATH);

		//
		//If there were no tokens in the specified dest dir string, then we assume
		//that the directory path specified is relative to the appdir (g_szAppDir)
		//so build the new dir name as appdir+specified dir
		//
		if (0 == lstrcmpi(szDest, lpInstallFile->GetDestPath()))
		{
			wsprintf(szDest, "%s\\%s", GetAppDir(), lpInstallFile->GetDestPath());
		}
	}

	fcs.fUninstall = TRUE;
	lstrcpy(fcs.szDest, szDest);

	EBURETCODE retc = (*(GetAppCallback())) ((void *) &fcs);
    // xxGlob EBURETCODE retc = (*g_lpfnAppCallback) ((void *) &fcs);

	ASSERT(EBU_OK == retc || EBU_ABORT == retc || EBU_CANCEL == retc);

	if (EBU_OK != retc)
	{
		return retc;
	}

	// if the file is marked with the uninstall flag
	if (lpInstallFile->fIsUninstallFile())
	{
		// if it is shared then release share ref
		if (lpInstallFile->fIsSharedFile() && RemoveSharedDLL(szDest) )
		{
			// Decrement the reference count, and maybe even delete it!
			MyReleaseSharedDll(szDest, lpInstallFile->fDLLRegister(), TRUE);
		}
		else
		{
			if (lpInstallFile->fDLLRegister())
				DLLRegister(szDest, DO_UNINSTALL);

			SetFileAttributes(szDest, FILE_ATTRIBUTE_NORMAL);
			DeleteFile(szDest);
		}
	}
	else
	{
		//
		//Doesn't need to be uninstalled, but dec ref count if we incremented it
		//
		if (lpInstallFile->fIsSharedFile() && RemoveSharedDLL(szDest) )
		{
			MyReleaseSharedDll(szDest, FALSE, FALSE);
		}
	}

	return EBU_OK;   //file was copied successfully
}

//****************************************************************************
// Procedure   ExecuteMkDir
//
// Purpose     Executes the MkDir setup command.  Creates the directory, using
//             the root dir input before this instruction
//
// Parameters  
//             lpMkDir  pointer to an MKDIR struct describing
//                            the directory.
//
// Returns     nonzero if successful, zero if setup was aborted.
//
// History      4/19/96 craigh    Created
//
EBURETCODE ExecuteMkDir(LPMKDIR lpMkDir )
{
	char    szDest[_MAX_PATH];
	UINT    cch = 0;
	int     nResult = 0;
	int     nAlertResult;
	MKDIRDATA mkd;
	
	__int64 group = lpMkDir->GetDirGroup();

	//
	//(If we aren't in maintenance mode or if we are in maintenance mode but force reinstall is set
	// AND
	//If group isn't group 1 and group isn't in user-selected group mask and group mask has been set)
	// OR
	//If we ARE in maintenance mode and group is 1 or group is in group mask
	//
	if (n_fMaintMode)
	{
		if ((group & 0x1 || group & GetOldGroupList()) && !GetForceReinstall())
		{
			return EBU_OK;
		}
	}
	if (!(0x1 & group) && !(n_GroupList & group) && removeKeyboardTypeFlag(n_GroupList) > 0)
	{
		return EBU_OK;
	}

	ASSERT(lpMkDir->GetMkDir());

	//
	//Copy the destination dir into the dest buffer and expand any tokens
	//
	lstrcpy(szDest, lpMkDir->GetMkDir());
	ReplaceStringTokens(szDest, _MAX_PATH);

	//
	//If there were no tokens in the specified dest dir string, then we assume
	//that the directory path specified is relative to the appdir (g_szAppDir)
	//so build the new dir name as appdir+specified dir
	//
	if (0 == lstrcmpi(szDest, lpMkDir->GetMkDir()))
	{
		wsprintf(szDest, "%s\\%s", GetAppDir(), lpMkDir->GetMkDir());
	}

	mkd.pszDirName = szDest;

	EBURETCODE retc = (*(GetAppCallback())) ((void *) &mkd);

	if (EBU_OK != retc)
	{
		return retc;
	}

	DWORD dwfa = GetFileAttributes(szDest);

	//
	//If directory already exists, buh-bye.
	//
	if ((0xFFFFFFFF != dwfa) && (dwfa & FILE_ATTRIBUTE_DIRECTORY))
	{
		return EBU_OK;
	}

	while (0 == nResult)
	{
		nResult = CreateDirectory(szDest, NULL);

		if (0 == nResult)
		{
			nAlertResult = Alert( GetWndParent(), MB_ICONEXCLAMATION | MB_ABORTRETRYIGNORE | MB_DEFBUTTON2, STR_ERROR_CANTCREATEDIRECTORY, (LPCSTR)szDest );

			if (IDABORT == nAlertResult)
			{
				return EBU_ABORT;
			}
			else if (IDIGNORE == nAlertResult)
			{
				return EBU_OK;
			}
		}
		else
		{
			SetDirtyBits(DIRTY_MKDIR);
		}

	}

    return EBU_OK;
}


//****************************************************************************
// Procedure   ExecuteRDDir
//
// Purpose     Executes the MkDir uninstall command.  Deletes the directory, using
//             the root dir input before this instruction
//
// Parameters  
//             lpInstallFile  pointer to an INSTALLFILE struct describing
//                            the source and destination files.
//
// Returns     nonzero if successful, zero if setup was aborted.
//
// History      4/19/96 craigh    Created
//
EBURETCODE ExecuteRdDir(LPMKDIR lpMkDir )
{
	char     szDest[_MAX_PATH];
	UINT     cch = 0;
	MKDIRDATA mkd;
	EBURETCODE retc = EBU_OK;

	ASSERT(lpMkDir->GetMkDir() != 0);

	__int64 group = lpMkDir->GetDirGroup();

	// if maintMode AND (group==1 OR group==FileGroup)
	if (n_fMaintMode && ((group & 0x1) || (n_GroupList & group)))
	{
		return EBU_OK;
	}

	//
	//Copy the destination dir into the dest buffer and expand any tokens
	//
	lstrcpy(szDest, lpMkDir->GetMkDir());
	ReplaceStringTokens(szDest, _MAX_PATH);

	//
	//If there were no tokens in the specified dest dir string, then we assume
	//that the directory path specified is relative to the appdir (g_szAppDir)
	//so build the new dir name as appdir+specified dir
	//
	if (0 == lstrcmpi(szDest, lpMkDir->GetMkDir()))
	{
		wsprintf(szDest, "%s\\%s", GetAppDir(), lpMkDir->GetMkDir());
	}

	DWORD dwfa = GetFileAttributes(szDest);
	if ((0xFFFFFFFF != dwfa) && (dwfa & FILE_ATTRIBUTE_DIRECTORY))
    {
		mkd.pszDirName = szDest;
		mkd.fUninstall = TRUE;
	    retc = (*(GetAppCallback())) ((void *) &mkd);

		ASSERT(EBU_OK == retc || EBU_ABORT == retc || EBU_CANCEL == retc);

		if (EBU_OK == retc)
		{
			if ( lpMkDir->wFlags & IF_UNINSTALL )
				RemoveDirectory(szDest);
			else if ( lpMkDir->wFlags & IF_UNINSTALLALL )
			{
				DelTree(szDest);
			}
		}
    }

    return retc;
}

//****************************************************************************
// Procedure   ExecuteMkRoot
//
// Purpose     Executes the MkRoot setup command.  Creates the directory, using
//             the root dir input before this instruction
//
// Parameters  
//             lpInstallFile  pointer to an INSTALLFILE struct describing
//                            the source and destination files.
//
// Returns     nonzero if successful, zero if setup was aborted.
//
// History      4/19/96 craigh    Created
//
EBURETCODE ExecuteMkRoot(LPMKROOT lpMkRoot,LPRUNTIMECOMMAND lpRuntime,WORD cCommands,LPRUNTIMECOMMAND prgRuntime,UINT uFirstResID,BOOL fFirstTime)
{
	EBURETCODE retc = EBU_OK;
	MKROOTDATA mk;
	DWORD dwGameFreeSpace;
	DWORD dwGameNeeded;
	DWORD dwSystemFreeSpace;
	DWORD dwSystemNeeded;
	BOOL
		bRecomputeUsage = FALSE;
	
	lstrcpy(mk.szAppDir, GetAppDir());
	mk.lpfnValidateEntry = ValidateDirectoryAndResolvePath;
	
	BOOL
		bDone = FALSE;

	// because we might need the install size to display on this screen compute it if we don't have it
	if(!GetIsThereAGetGroup())
	{
		if (FALSE == GetFileSizeRequirements(prgRuntime, 
							cCommands, 
							GetAppDir(),
							&dwGameFreeSpace, 
							&dwGameNeeded, 
							&dwSystemFreeSpace, 
							&dwSystemNeeded, 
							GetIsThereAGetGroup(), 
							fFirstTime) )
	
		{
			return EBU_ERROR;	// engine error game over
		}
	}

	
	while(!bDone)
	{
		retc = (*(GetAppCallback())) ((void *) &mk);
		// now that we have the directory that the user wants us to install to do some checks on it
		
		switch(retc)
		{
		case EBU_ABORT:
			{
				// the user has exited setup
				// time to clear out of the engine
				bDone = TRUE;
			}break;
			
		case EBU_CANCEL:
			{
				// this command has been canceled we should quit and continue with engine ?
                TRACE("EBU_CANCEL on ExecuteMkRoot.\n");
				bDone = TRUE;
			}break;
			
		case EBU_RETRY:
			{
				// Being asked to verify that we can install to the requested directory.
				// pretend that there is no group information so that we recompute disk usage needs on the new drive
				bRecomputeUsage = TRUE;
			}	// fall thru to EBU_OK case
			
		case EBU_OK:
			{
				// validate and verify the drive / directory
				SetAppDirExists(true);
                SetAppDir(mk.UserRootEntry);
				
				char 	szSysDrive[_MAX_PATH];
				char 	szDestDrive[_MAX_PATH];
				lstrcpy(szDestDrive,mk.UserRootEntry);
				GetSystemDirectory(szSysDrive, _MAX_PATH);

				// now that we have the directory (and it has been verified and ok'd by the user) create it.


                if (EBUCreateDirectory(GetAppDir(), TRUE))    // creates full root path
				{
					SetAppDirExists(false);
				}

				
				if(!GetIsThereAGetGroup())
				{
					bRecomputeUsage = TRUE;
				}
				
				if (bRecomputeUsage && !GetForceFreeSpace())
				{
					if (FALSE == GetFileSizeRequirements(prgRuntime, 
							cCommands, 
							GetAppDir(),
							&dwGameFreeSpace, 
							&dwGameNeeded, 
							&dwSystemFreeSpace, 
							&dwSystemNeeded, 
							GetIsThereAGetGroup(), 
							fFirstTime))
					{
			            Alert(GetWndParent(), MB_OK | MB_ICONSTOP, STR_ERROR_RESOURCEFAILURE);
						return EBU_ERROR;	// engine error game over
					}

					// For our error message, make the drive "C" instead of "C:\"
					szSysDrive[1] = 0;
					szDestDrive[1] = 0;
					if ( dwGameFreeSpace < dwGameNeeded && dwSystemFreeSpace < dwSystemNeeded )
					{

						// Make the parent window non-topmost, so that the user can
						// look at other stuff on their system.  -ks 4/7/95
						SetWindowPos( GetWndParent(), HWND_NOTOPMOST, 0, 0, 0, 0,
									  SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE );

						char
							szTemp[25];
						sprintf(szTemp, "%.2f", dwSystemNeeded / 1024.0);

						if (dwSystemFreeSpace < dwSystemNeeded)
							  Alert( GetWndParent(), MB_OK | MB_ICONEXCLAMATION | MB_DEFBUTTON2,
								  STR_ERROR_NEEDSYSDISKSPACE, szSysDrive, szTemp);

						sprintf(szTemp, "%.2f", dwGameNeeded / 1024.0);
						if (dwGameFreeSpace < dwGameNeeded)
						   Alert( GetWndParent(), MB_OK | MB_ICONEXCLAMATION | MB_DEFBUTTON2,
							   STR_ERROR_NEEDDISKSPACE, szDestDrive, szTemp);

						retc = EBU_RETRY;
					}
					else
						retc = EBU_OK;
				}

				if (EBU_RETRY != retc)
					bDone = TRUE;

			}break;	// case EBU_OK

		case EBU_BACK:
			{
				bDone = TRUE;
			}break;
			

		default:
			{
				ASSERT(!"Unexpected return code from user callback.\n");
			}
			
		} // (switch retc)

	} // while (!bDone)
	
    return retc;
}

//****************************************************************************
// Procedure   ExecuteRdRoot
//
// Purpose     Executes the MkRoot setup command.  Creates the directory, using
//             the root dir input before this instruction
//
// Parameters  
//             lpInstallFile  pointer to an INSTALLFILE struct describing
//                            the source and destination files.
//
// Returns     nonzero if successful, zero if setup was aborted.
//
// History      4/19/96 craigh    Created
//				5/20/97 v-richei  Rewrote to remove full root path
//
EBURETCODE ExecuteRdRoot(LPMKROOT lpMkRoot )
{
	TCHAR             szFile[_MAX_PATH];
	TCHAR             *pszFile;
	
	if (0 == GetShortPathName(GetAppDir(),szFile,_MAX_PATH))
	{
		lstrcpy(szFile, GetAppDir());
	}

	pszFile = szFile + lstrlen(szFile);

	while (pszFile > szFile)
	{
		// MEMO : Jul.15,1997 20:04 by yutaka.
		// We should strip before RemoveDirectory() ?
		// g_szAppDir might have '\\' in the last byte.
		ASSERT( *CharPrev(szFile, pszFile)!='\\' );
		
		RemoveDirectory(szFile);

		while (pszFile > szFile)
		{
			pszFile = CharPrev(szFile, pszFile);

			if (*pszFile == '\\')
			{
				*pszFile = '\0';

				break;
			}
		}
	}

	SetDirCreated(FALSE);

	return EBU_OK;
}

EBURETCODE WINAPI ValidateDirectoryAndResolvePath(PTSTR pszPath)
{
	//
	//Define max floppy space, taking into account drivespaced 2.88mb floppies...
	//
	const int nMAX_FLOPPY_DISK_SPACE = 6000000;

	DWORD  dwRc;

	TCHAR  chPrevChar = '\0';
	TCHAR  chNextChar;

	PTCHAR pch1;
	PTCHAR pch2;
	PTCHAR pch3;
	PTCHAR pch4;

	PTCHAR pszPathWalk;
	PTCHAR pszFirstCreated = NULL;

	DWORD BytesPerSector;			// bytes per sector
	DWORD SectorsPerCluster;		// sectors per cluster
	DWORD NumberOfFreeClusters;		// number of free clusters
	DWORD TotalNumberOfClusters;	// size of partition

	_int64 nTotalDiskSize = 0;
	
	TCHAR	szSysPath[MAX_PATH];
	PTCHAR	pszLowerSysPath, pSysTmp;
	PTCHAR	pszLowerUserPath,pUserTmp;
	BOOL	bMatch = TRUE;
	DWORD	dwErr = 0;
	DWORD   dwPathLen = 0;

	TCHAR	szTempBuffer[MAX_PATH];
	TCHAR*	pszTempBuffer = szTempBuffer;

	// First copy this to a buffer whose address we can manipulate, too
	lstrcpy(szTempBuffer, pszPath);
	//
	//Eat leading white space and quotation marks...
	//
	while (' ' == *pszTempBuffer || '\t' == *pszTempBuffer || '\"' == *pszTempBuffer)
	{
		pszTempBuffer = CharNext(pszTempBuffer);
	}

	// Remove any leading or trailing white space from each file/folder name
	RemoveWhiteSpaceFromFilename(pszTempBuffer);
	//
	//Invalid if blank pathname was passed in...
	//
	if ('\0' == *pszTempBuffer)
	{
		goto PathError;
	}

	//
	//Walk to the end of the path string...
	//
	pszPathWalk = pszTempBuffer + lstrlen(pszTempBuffer);

	//
	//Now back up and eat any trailing white space or quotation marks...
	//
	while ('\0' == *pszPathWalk)
	{
		pszPathWalk = CharPrev(pszTempBuffer, pszPathWalk);

		if (' ' == *pszPathWalk || '\t' == *pszPathWalk || '\"' == *pszPathWalk)
		{
			*pszPathWalk = '\0';
		}
	}

	//
	//Save first four non white space characters...
	//
	pch1 = pszTempBuffer;
	pch2 = CharNext(pch1);
	pch3 = CharNext(pch2);
	pch4 = CharNext(pch3);

	//
	//Check for drive letter, colon, and backslash at beginning of path...  This test 
	//also prevents the user from specifying a relative path at the beginning of
	//the path name...
	//
	if (0 < _istalpha(*pch1) &&
		':'  == *pch2 &&
		'\\' == *pch3)
	{
		//
		//If all we have is a drive letter, colon, and backslash, that's no good...
		//
		if ('\0' == *pch4)
		{
			goto PathError;
		}
	}
	else
	{
		//
		//Also an error, duh, if we don't have letter, colon, and backslash...
		//
		goto PathError;
	}

	//
	//Before trying to use the drive spec in any calls, set to ignore any I/O critical
	//errors (blue screens).  Instead, the error will be passed back...
	//
	SetErrorMode(SEM_FAILCRITICALERRORS);

	//
	//Temporarily replace 4th character with terminating NUL so just drive, colon,
	//and backslash get passed to GetDriveType() and MyGetDiskFreeSpace() calls...
	//
	chPrevChar = *pch4;
	*pch4 = '\0';
	dwRc = (DWORD) GetDriveType(pszTempBuffer);

	//
	//Invalid if it's any of these return values...
	//
	if (dwRc == DRIVE_UNKNOWN || dwRc == DRIVE_NO_ROOT_DIR || dwRc == DRIVE_CDROM)
	{
		*pch4 = chPrevChar;
		goto PathError;
	}

	//
	//If we can't get free space, this must be a removable with no disk in it...
	//Using Windows API because its reliable for total size on floppies
	//
	if (FALSE == GetDiskFreeSpace(pszTempBuffer, 
									&SectorsPerCluster, 
									&BytesPerSector, 
									&NumberOfFreeClusters, 
									&TotalNumberOfClusters))
	{
		*pch4 = chPrevChar;
		goto PathError;
	}

	//
	//Restore rest of path specification...
	//
	*pch4 = chPrevChar;

	nTotalDiskSize = TotalNumberOfClusters * BytesPerSector * SectorsPerCluster;

	//
	//If this is a floppy disk drive - we don't allow it
	//
	if (nTotalDiskSize < nMAX_FLOPPY_DISK_SPACE) 
	{
		goto PathError;
	}

	//
	//Init a walk ptr to the backslash char of (drive-colon-backslash, 
	//ex., of C:\) -- we've already checked the drive and colon part --
	//and now we'll walk through the rest of the path string looking for 
	//various characters and character combos/states...
	//
	pszPathWalk = pch3;
	while (*pszPathWalk)
	{
		//
		//Get a copy of the next character for use in switch state comparisons...
		//
		chNextChar = *(CharNext(pszPathWalk));

		switch (*pszPathWalk)
		{
		//
		//Check for an invalid character...
		//
		case '/':
		case '\"':
		case ':':
		case '*':
		case '?':
		case '<':
		case '>':
		case '|':
			goto PathError;

		//
		//If we have two backslashes in a row, that's invalid.  Otherwise, check
		//to see if this is a trailing backslash and remove it if so...
		//
		case '\\':
			if ('\\' == chPrevChar)
			{
				goto PathError;
			}

			//
			//Remove any trailing backslash...
			//
			if ('\0' == chNextChar)
			{
				*pszPathWalk = '\0';
			}

			break;

		//
		//Periods are valid unless they are being used to specify relative paths;
		//we're not going to allow that (just like Office 97).  In any sequence
		//of periods (1, 2, or 3+ of them), it's invalid:
		//  IF the character immediately preceding the first period is a backslash
		//  AND if the character immediately following the first period is 
		//  a backslash, period, or NUL.  Really.
		//
		case '.':
			if ('\\' == chPrevChar)
			{
				switch (chNextChar)
				{
				case '\\':
				case '.':
				case '\0':
					//
					//Relative path specified...
					//
					goto PathError;
				}
			}
			else
			{
				if ('.' == chPrevChar && '\0' == chNextChar)
				{
					//
					//Trailing periods, only one allowed...
					//
					goto PathError;
				}
			}

			break;

			//
			//It's invalid to have whitespace before a backslash.  This code prevents
			//occurrences such as C:\  \TMP, C:\Dir1 \Dir2, etc.
			//
		case ' ':
		case '\t':
			if ('\\' == chNextChar)
			{
				goto PathError;
			}
		}

		//
		//Save previous character, get next character and continue walking...
		//
		chPrevChar = *pszPathWalk;
		pszPathWalk = CharNext(pszPathWalk);
	}

	//
	//Check to see if the specified path is some file system construct other than
	//a directory...
	//
	dwRc = GetFileAttributes(pszTempBuffer);
	if (!(FILE_ATTRIBUTE_DIRECTORY & dwRc) && 0xFFFFFFFF != dwRc)
	{
		goto PathError;
	}

	// Check to see if it's in some other system-marked folder
	if ((FILE_ATTRIBUTE_SYSTEM & dwRc) && (0xFFFFFFFF != dwRc) )
	{
		goto PathError;
	}
	// Check to see if we are trying to install into the windows folder
	GetWindowsDirectory(szSysPath, MAX_PATH);
	pUserTmp = pszLowerUserPath = (TCHAR*) malloc(lstrlen(pszTempBuffer) + sizeof(TCHAR)); // add one char for null terminator
	pSysTmp = pszLowerSysPath = (TCHAR*) malloc(lstrlen(szSysPath) + sizeof(TCHAR)); // add one char for null terminator
	CopyMemory(pszLowerUserPath, pszTempBuffer, lstrlen(pszTempBuffer)+1);
	CopyMemory(pszLowerSysPath, szSysPath, lstrlen(szSysPath)+1);
	MakeStrLowercase(pszLowerUserPath);
	MakeStrLowercase(pszLowerSysPath);
	
	//CharLowerBuff(szSysPath, MAX_PATH);
	// TODO:  need to convert copy of pSysPath to all lowerCase, too
	while ('\0' != *pszLowerSysPath)
	{
		if (*pszLowerSysPath == *pszLowerUserPath)
		{
			pszLowerSysPath = CharNext(pszLowerSysPath);
			pszLowerUserPath = CharNext(pszLowerUserPath);
		}
		else
		{
			bMatch = FALSE;
			break;
			
		}
	}
	if (bMatch) // then we have a system file
	{
		goto PathError;
	}
	// we won't need these anymore, so clean up
	free(pUserTmp);
	free(pSysTmp);
	
	dwRc = 0;
	pszPathWalk = pszFirstCreated = pszTempBuffer;

	while (TRUE)
	{
		//
		//If we found a new subdir level
		//
		if ('\\' == *pszPathWalk || '\0' == *pszPathWalk)
		{
			//
			//If we're pointing to the terminating zero, set done flag...
			//
			dwRc = *pszPathWalk ? 0 : 1;

			//
			//Temporarily Replace the backslash with a NUL so we can create the directory.  
			//If we were already at the end, then the following statement doesn't hurt...
			//
			*pszPathWalk = '\0';

			//
			//Try to create the directory up to this level...
			//
			if (((dwPathLen = EBUStrlen(pszTempBuffer)) + GetMaxDirLen() > _MAX_PATH ) || 0 == CreateDirectory(pszTempBuffer, NULL))
			{
				//
				//If the directory create failed for a reason other than:
				//  the subdirectory already existed -OR-
				//  Error 267, invalid directory (COM1,CON,AUX,LPT1, etc.)
				//then that's an error...
				//
				//
				dwErr = GetLastError();
				if (0xFFFFFFFF == GetFileAttributes(pszTempBuffer) || (dwErr == ERROR_DIRECTORY) || dwErr == ERROR_DISK_FULL || dwPathLen + GetMaxDirLen() > _MAX_PATH)
				{
					//
					//If we weren't yet at the end of the path...
					//
					if (0 == dwRc)
					{
						//
						//Restore original backslash that we replaced with a NUL up above...
						//
						*pszPathWalk = '\\';
					}

					//
					//Break out of the loop if there was an error.  Post loop
					//logic will check state of dwRc to see if the "test directory
					//creation" phase completed successfully.  Since we failed, reset
					//this to FALSE...
					//
					dwRc = 0;

					break;
				}
				else
				{
					//
					//If we failed because the directory already existed, then we'll
					//reset the pointer indicating where in the path the first
					//subdir was actually created by us.  It'll get set again if another
					//subdir candidate is found...
					//
					pszFirstCreated = NULL;
				}
			}

			//
			//Restore original backslash if we're not yet at the end of the path...
			//Also, set the next subdir candidate for "first subdir created"... if
			//we haven't already determined one...
			//
			if (0 == dwRc)
			{
				*pszPathWalk = '\\';

				if (NULL == pszFirstCreated)
				{
					pszFirstCreated = pszPathWalk;
				}
			}
			else
			{
				//
				//Get out of here because we're at the end of the path...
				//
				break;
			}
		}

		//
		//Get next character
		//
		pszPathWalk = CharNext(pszPathWalk);
	}

	//
	//If we actually created any of the subdirs...
	//
	if (pszFirstCreated)
	{
		//
		//Point to the end of the path
		//
		pszPathWalk = pszTempBuffer + lstrlen(pszTempBuffer);

		//
		//While we haven't backed up past the level that we actually first created...
		//
		while (pszPathWalk > pszFirstCreated)
		{
			//
			//Save either a backslash or the zero terminator (first loop pass)
			//and replace it with a NUL
			//
			chPrevChar = *pszPathWalk;
			*pszPathWalk = '\0';

			//
			//Remove the subdir that we created...
			//
			RemoveDirectory(pszTempBuffer);

			//
			//This will restore either the backslash that was here or the zero
			//terminator will get "replaced" with one just like it!
			//
			*pszPathWalk = chPrevChar;

			//
			//Back up to the backslash that terminates the next subdir level...
			//
			
			do
			{
				pszPathWalk = CharPrev(pszTempBuffer, pszPathWalk);
			} while ('\\' != *pszPathWalk);
		}
	}

	//
	//If, for some reason, we couldn't finish "test" creating the subdirs, then
	//we've got an error...
	//
	if (0 == dwRc)
	{
		goto PathError;
	}

	//
	//Restore normal error handling...
	//
	SetErrorMode(0);

	lstrcpy(pszPath, pszTempBuffer);
	return EBU_OK;

PathError:
	//
	//Restore normal error handling in case we haven't yet...
	//
	SetErrorMode(0);

	if (ERROR_DISK_FULL == dwErr)
	{
		Alert(GetWndParent(), MB_ICONEXCLAMATION | MB_OK, STR_ERROR_NODISKSPACE, szTempBuffer);
	}
	else
	{
		Alert(GetWndParent(), MB_ICONEXCLAMATION | MB_OK, STR_ERROR_BADPATH, szTempBuffer, GetAppDir());
	}

	return EBU_CANCEL;
}

void RemoveWhiteSpaceFromFilename(LPSTR pStr)
{
	TCHAR		szTempBuf[MAX_PATH];
	TCHAR*		pSpaceSeeker;
	TCHAR*		pCurrent;

	lstrcpy(szTempBuf, pStr);
	pSpaceSeeker = pCurrent = szTempBuf;

	// First get rid of all white spaces following a wack ('\')
	while (NULL != *pSpaceSeeker)
	{
		if ('\\' == *pSpaceSeeker)
		{
			pCurrent = pSpaceSeeker;
			pSpaceSeeker = CharNext(pSpaceSeeker);
			while ((' ' == *pSpaceSeeker) || ('\t' == *pSpaceSeeker))
			{
				pSpaceSeeker = CharNext(pSpaceSeeker);
			}
			pCurrent = CharNext(pCurrent);
			if (pCurrent != pSpaceSeeker)
			{
				lstrcpy(pCurrent, pSpaceSeeker);
				pSpaceSeeker = pCurrent;
			}

		}
		pSpaceSeeker = CharNext(pSpaceSeeker);
	}

	// Then get rid of all white spaces preceding a wack ('\')
	pSpaceSeeker = pCurrent = szTempBuf;
	while (NULL != *pSpaceSeeker)
	{
		if ('\\' == *pSpaceSeeker)
		{
			pCurrent = pSpaceSeeker;
			pSpaceSeeker = CharPrev(szTempBuf,pSpaceSeeker);
			// no reason test for whiteSpace prior to first char in string
			if (pCurrent == pSpaceSeeker)
			{
				pCurrent = pSpaceSeeker = CharNext(pSpaceSeeker);
				continue;
			}
			while ((' ' == *pSpaceSeeker) || ('\t' == *pSpaceSeeker))
			{
				pSpaceSeeker = CharPrev(szTempBuf, pSpaceSeeker);
			}
			pSpaceSeeker = CharNext(pSpaceSeeker);
			if ( pSpaceSeeker != pCurrent)
			{
				lstrcpy(pSpaceSeeker, pCurrent);
				pSpaceSeeker = CharNext(pSpaceSeeker);
			}
		}
		pSpaceSeeker = CharNext(pSpaceSeeker);
	}
	lstrcpy(pStr, szTempBuf);

}

//****************************************************************************
// Procedure   ExecuteGetName
//
// Purpose     Executes the GetName setup command.  Creates the directory, using
//             the root dir input before this instruction
//
// Parameters  
//             lpInstallFile  pointer to an INSTALLFILE struct describing
//                            the source and destination files.
//
// Returns     nonzero if successful, zero if setup was aborted.
//
// History      4/19/96 craigh    Created
//
EBURETCODE ExecuteGetName(LPGETNAME lpGetName )
{
	EBURETCODE retc = EBU_OK;
	GETNAMEDATA gnd;

	gnd.pszPlayerName = GetPlayerName();

	if (!lstrlen(gnd.pszPlayerName))
	{
		GetRegisteredOwner(gnd.pszPlayerName, MAX_PATH);
	}

	retc = (*(GetAppCallback())) ((void *) &gnd);

	ASSERT(retc == EBU_OK || retc == EBU_ABORT || retc == EBU_BACK);

	return retc;
}

EBURETCODE ExecuteGetPID(LPGETPID lpGetPID )
{
	GETPIDDATA pid;
	EBURETCODE retc = EBU_OK;

	//
	//Set validate proc and callback U.I.  The U.I. may chose to have the user
	//enter a PID.  The U.I. can test for this first callback by testing if
	//NULL == pid.pszPID
	//
	pid.lpfnValidateEntry = (VALIDATEPROC) FValidCDKey;
	pid.pszPID = NULL;
	retc = (*(GetAppCallback())) ((void *) &pid);

	ASSERT(EBU_OK == retc || EBU_ABORT == retc || EBU_BACK == retc || EBU_CANCEL == retc);
	
	if (retc != EBU_OK)
	{
		return retc;
	}
	
	//
	//Generate and store PID - use PID portions (if any) that the user entered...
	//
	GenerateAndStorePID(&pid);

	//
	//Callback the U.I. again with the final formatted PID so that a PID verification
	//dialog, for instance, can be displayed...
	//
	pid.pszPID = GetPid();
	retc = (*(GetAppCallback())) ((void *) &pid);

	ASSERT(EBU_OK == retc || EBU_ABORT == retc || EBU_BACK == retc || EBU_CANCEL == retc);

    return retc;
}


EBURETCODE ExecuteInstIcon(LPINSTICON lpInstIcon )
{
	TCHAR WorkDir[_MAX_PATH * 2];
	TCHAR szArgs[_MAX_PATH * 2];
	TCHAR szGroupName[_MAX_PATH];
	TCHAR szExeName[_MAX_PATH * 2];
	TCHAR szIconSource[_MAX_PATH];
	TCHAR *pchSrc;
	TCHAR *pchDest;
	TCHAR *pchTemp;
	INSTICONDATA icinfo;
	TCHAR szIconDesc[_MAX_PATH*2];

	
	__int64 group = lpInstIcon->GetGroup();

	if (n_fMaintMode)
	{
		if ((group & 0x1 || group & GetOldGroupList()) && !GetForceReinstall())
		{
			return EBU_OK;
		}
	}
	if (!(0x1 & group) && !(n_GroupList & group) && removeKeyboardTypeFlag(n_GroupList) > 0)
	{
		return EBU_OK;
	}


	// get the params for this instruction from the lpInstIcon.
	DWORD index = lpInstIcon->GetIconIndex();
	char *szProgName = lpInstIcon->GetIconName();
	lstrcpy(szIconSource, lpInstIcon->GetIconSource());
	lstrcpy(szIconDesc,lpInstIcon->GetIconDescription());
	char *szIconDest = lpInstIcon->GetIconDestination(); //Start (M)enu, (D)esktop, (B)oth
	
	ReplaceStringTokens(szIconSource, _MAX_PATH);
	ReplaceStringTokens(szIconDesc, _MAX_PATH*2);
	// copy the full app name into a temp to be used for the start menu goup name
	lstrcpyn(szGroupName,szIconDesc,sizeof(szGroupName));
	// cut the exe name (and last \) off
	char *ptr = pszGetLast5C(szGroupName);
	
	// MEMO : Jul.15,1997 20:22 by yutaka.
	// We need to care when ptr equals NULL.
	ASSERT( ptr );
	
	char *DesktopLinkName = ptr+1; // for the desktop link we just want the file description
	if( ptr!= NULL )
		*ptr = '\0';
	
	if ('B' == *szIconDest || 'M' == *szIconDest)
	{
		// add this group series to the start menu
		AddGroupToStartMenu(szGroupName);
		SetDirtyBits(DIRTY_INSTICON);
	}

	//
	//Build the exe path
	//
	lstrcpyn(szGroupName, szIconDesc, sizeof(szGroupName));
	lstrcpy(szExeName, szProgName);
	ReplaceStringTokens(szExeName, _MAX_PATH * 2);

	//
	//Build the ArgList
	//
	pchDest = szArgs;
	pchSrc = szExeName;

	//
	//Skip extension
	//
	while (*pchSrc && *pchSrc != '.')
	{
		pchSrc = AnsiNext(pchSrc);
	}

	ASSERT(*pchSrc);  //EXE name must have an extension...

	while (*pchSrc != ' ' && *pchSrc != '\0')
	{
		pchSrc = AnsiNext(pchSrc);
	}

	pchTemp = pchSrc;

	while (*pchSrc == ' ')
	{
		pchSrc = AnsiNext(pchSrc);
	}
	
	//
	//Remove any trailing space from EXE name...
	//
	*pchTemp = '\0';

	//
	//Set the app arglist for the link
	//
	if (*pchSrc != '\0')
	{
		lstrcpy(szArgs,pchSrc);
		*pchSrc = '\0';
	}
	else
	{
		*szArgs = '\0';
	}

	//
	//Extract the program file name.  pchSrc now points to right before the arg list...
	//
	pchDest = WorkDir;

	//
	//Find the last backslash in the Executable pathname (not including the arg list)
	//
	while (*pchSrc != '\\' && *pchSrc != ':')
	{
		pchSrc = AnsiPrev(szExeName, pchSrc);
	}
	
	//
	//Build the working directory...
	//
	ptr = szExeName;
	while(ptr != pchSrc)
		*pchDest++ = *ptr++;
	*pchDest = '\0';
	
	switch(*szIconDest)
	{
	case 'B':
		icinfo.icontype = ICON_MENU | ICON_DESKTOP;
		break;
	case 'M':
		icinfo.icontype = ICON_MENU;
		break;
	case 'D':
		icinfo.icontype = ICON_DESKTOP;
		break;
	}

	EBURETCODE retc = EBU_OK;

	icinfo.szIconDesc = szIconDesc;
	icinfo.szExeName = szExeName;
	icinfo.szIconSource = szIconSource;
	icinfo.WorkDir = WorkDir;
	icinfo.szArgs = szArgs;
	icinfo.index = index;

	retc = (*(GetAppCallback())) ((void *) &icinfo);

	ASSERT(retc == EBU_OK || retc == EBU_ABORT || retc == EBU_BACK || retc == EBU_CANCEL);

	if (retc != EBU_OK)
	{
		return retc;
	}
	
	// now create the link item in the start menu if needed
	if ('B' == *szIconDest || 'M' == *szIconDest)
		AddItemToStartMenu(szIconDesc,szExeName,szIconSource,WorkDir,szArgs,index);
	
	// and on the desktop if needed
	if ('B' == *szIconDest || 'D' == *szIconDest)
		AddItemToDesktop(DesktopLinkName,szExeName,szIconSource,WorkDir,szArgs,index);
	
	SetDirtyBits(DIRTY_INSTICON);

    return EBU_OK;
}

EBURETCODE ExecuteRemoveIcon(LPINSTICON lpInstIcon )
{
	INSTICONDATA iid;
	EBURETCODE   retc = EBU_OK;
	TCHAR szProgName[_MAX_PATH*2];
	TCHAR szGroupName[_MAX_PATH];


	__int64 group = lpInstIcon->GetGroup();
	
	lstrcpy(szProgName,lpInstIcon->GetIconDescription());
	ReplaceStringTokens(szProgName, _MAX_PATH*2);

	lstrcpy(szGroupName,szProgName);
	TCHAR *ptr = pszGetLast5C(szGroupName);
	
	// MEMO : Jul.15,1997 20:30 by yutaka.
	// We need to check if ptr equals NULL.
	// When ptr equals NULL, DesktopLinkName is corrupt and will be GPF.
	ASSERT( ptr );
	
	char *DesktopLinkName = ptr+1;
	
	// Format IconDescription String for callback
	if ( !n_fMaintMode && !lpInstIcon->fIsUninstallLink() )
	{
		if (ptr != NULL)
		{
			*ptr = '\0';
		}
	}	

	iid.szIconDesc = szGroupName;
	iid.icontype = ICON_MENU;
	iid.fUninstall = TRUE;
	
	retc = (*(GetAppCallback())) ((void *) &iid);
	
	ASSERT(EBU_OK == retc || EBU_ABORT == retc || EBU_CANCEL == retc);
	
	if (EBU_OK != retc)
	{
		return retc;
	}
	
	// Delete the link only when told to or when in maintenance mode.
	if ( n_fMaintMode || lpInstIcon->fIsUninstallLink() )
	{
		DeleteLinkFromStartMenu(szGroupName,TRUE);
	}
	// Otherwise delete the Folder
	else
	{
		DeleteGroupFromStartMenu(szGroupName,TRUE);
	}

	//
	//If this icon isn't a start "M"enu only icon, delete the desktop icon too...
	//
	if (lstrcmpi("M", lpInstIcon->GetIconDestination()))
	{
		iid.szIconDesc = DesktopLinkName;
		iid.icontype = ICON_DESKTOP;
		
		retc = (*(GetAppCallback())) ((void *) &iid);
		
		ASSERT(EBU_OK == retc || EBU_ABORT == retc || EBU_CANCEL == retc);
		
		if (EBU_OK != retc)
		{
			return retc;
		}

		TCHAR http[5];
		TCHAR exename[_MAX_PATH*2];

		lstrcpy(exename, lpInstIcon->GetIconName());
		ReplaceStringTokens(exename, _MAX_PATH*2);
		lstrcpyn(http, exename, 5);

		if(!lstrcmp(http,"http"))
		{
			DeleteLinkFromDesktop(DesktopLinkName, TRUE);
		}
		else
		{
			DeleteLinkFromDesktop(DesktopLinkName, FALSE);
		}
	}
	
    return EBU_OK;
}


EBURETCODE ExecuteInstDX(LPINSTDX lpInstDX )
{
	EBURETCODE  retc = EBU_OK;
	DIRECT_X_VERSION uExistingVersion;
	//
	//if /NODX specified on command line, return as if user canceled DX installation...
	//
	if (GetNoDXFlag())
	{
		return EBU_CANCEL;
	}

	// DirectPlay only callback if DLL name == NULL
	// MEMO : lpInstDX->GetInstDX() points InstDX's first arg : Sep.30,1997 01:18 by yutaka.
	// Return an error if an old script resource is used.
	if (lstrcmpi("NULL", lpInstDX->GetInstDX()) ? FALSE : TRUE)
	{
		return EBU_ERROR; 
	}
	//
	//If command line specified /DXFORCE, then always return EV_EXISTING_OLDER, else
	//do real version check...
	//
	if (FALSE == GetForceDXFlag())
	{
		CHAR szVersion[MAX_PATH],szDllName[MAX_PATH];
		lstrcpy(szVersion,lpInstDX->GetInstDXMinVersion());
		ReplaceStringTokens(szVersion,MAX_PATH);
		lstrcpy(szDllName,lpInstDX->GetInstDX());
		ReplaceStringTokens(szDllName,MAX_PATH);
		uExistingVersion = CheckDXVersion( szVersion, szDllName);
	}
	else
	{
		uExistingVersion = EV_EXISTING_OLDER;
	}

	retc = InstDX(lpInstDX, &uExistingVersion);

	return retc;
}

EBURETCODE ExecuteInstDPLAY(LPINSTDPLAY lpInstDPLAY )
{
	EBURETCODE  retc = EBU_OK;
	DIRECT_X_VERSION uExistingVersion;

	//
	//if /NODX specified on command line, return as if user canceled DX installation...
	//
	if (GetNoDXFlag())
	{
		return EBU_CANCEL;
	}
	//
	//If command line specified /DXFORCE, then always return EV_EXISTING_OLDER, else
	//do real version check...
	//
	if (FALSE == GetForceDXFlag())
	{
		uExistingVersion = CheckDPLAYVersion(lpInstDPLAY->GetInstDPLAYMinVersion() );
	}
	else
	{
		uExistingVersion = EV_EXISTING_OLDER;
	}

	retc = InstDPLAY( lpInstDPLAY, &uExistingVersion );

	return retc;

}

//****************************************************************************
// Procedure ExecuteUnInstDPLAY
//
// The sole function of this routine is to unregister the app from the dplay lobby
// UnInstDPLAY does a callback to get the GUID so we can uninstall the right app 
EBURETCODE ExecuteUnInstDPLAY(LPINSTDPLAY lpInstDPLAY, BYTE bProcessType )
{
	EBURETCODE  retc = EBU_OK;
	DIRECT_X_VERSION uExistingVersion;

	if (DO_UNINSTALL == bProcessType)
	{
		retc = UnInstDPLAY( lpInstDPLAY, &uExistingVersion );
	}
	return retc;

}

//****************************************************************************
// Procedure   ExecuteShellExecute
//
EBURETCODE ExecuteShellExecute(LPSHELLEXECUTE lpShellExecute, BYTE bProcessType)
{
	TCHAR szParameters[MAX_PATH];
	TCHAR szDirectory[MAX_PATH];

	ASSERT(DO_INSTALL == bProcessType || DO_UNINSTALL == bProcessType);

	if (DO_INSTALL == bProcessType)
	{
		__int64 group = lpShellExecute->GetGroup();
		
		//
		//If ShellExecute command doesn't specify group one AND it doesn't specify a group
		//selected from the GetGroup command AND if the group list (n_GroupList) has already
		//been set (like in the GetGroup command), then don't process this ini/reg write...
		//
		if (!(group & 0x1) && !(group & n_GroupList) && removeKeyboardTypeFlag(n_GroupList) > 0)
		{
			return GetResultCode();
		}
	}

	//
	//If Uninstall flag was specified, then ONLY run this during uninstall...
	//
	if (lpShellExecute->RunDuringUninstall())
	{
		if (DO_INSTALL == bProcessType)
		{
			return GetResultCode();
		}
	}
	else
	{
		if (DO_UNINSTALL == bProcessType)
		{
			return GetResultCode();
		}
	}



	lstrcpy(szDirectory, lpShellExecute->GetDirectory());
	if (0 == lstrcmpi(szDirectory, "NULL"))
	{
		szDirectory[0] = '\0';
	}

	lstrcpy(szParameters, lpShellExecute->GetParameters());
	if (0 == lstrcmpi(szParameters, "NULL"))
	{
		szParameters[0] = '\0';
	}

	return EBUShellExecute(GetWndParent(),
						   lpShellExecute->GetFileName(),
						   *szParameters ? szParameters : NULL,
						   *szDirectory ? szDirectory : NULL,
						   lpShellExecute->GetShowFlags(),
						   EBUSCRIPT_SHELLEXECUTE,
						   NULL,
						   lpShellExecute->GetWait(),
						   NULL);
}

//****************************************************************************
// Procedure   ExecuteAddIniValue
//
// Purpose	   Add a Sz of DWord to the Registry or INI file (sz only).
//
// Parameters	
//				lpAddIniValue	pointer to ADDINIVALUE command data
//				bDataType		specifies whether the data is a sz or DWORD
//				bProcessType	Install or delete
//
// Returns		nonzero if successful, zero if not.
//
// History		4/15/97 a-drews		Created
//
EBURETCODE ExecuteAddIniValue(LPADDINIVALUE lpAddIniValue, BYTE bProcessType)
{
#define MAX_INI_BUF	256
	char szKey[MAX_INI_BUF];
	char szValue[MAX_INI_BUF];
	char sz[MAX_INI_BUF];
	BOOL fChangedAccess = FALSE;
	ADDINIVALUEDATA aivd;
	
	ASSERT(DO_INSTALL == bProcessType || DO_UNINSTALL == bProcessType);
	
	if (DO_INSTALL == bProcessType)
	{
		__int64 group = lpAddIniValue->GetGroup();
		
		if (n_fMaintMode)
		{
			if ((group & 0x1 || group & GetOldGroupList()) && !GetForceReinstall())
			{
				return EBU_OK;
			}
		}
		if (!(0x1 & group) && !(n_GroupList & group) && removeKeyboardTypeFlag(n_GroupList) > 0)
		{
			return EBU_OK;
		}
	}
	

	if ((DO_UNINSTALL == bProcessType) && n_fMaintMode)
	{
		__int64 group = lpAddIniValue->GetGroup();

		// if maintMode AND (group==1 OR group==FileGroup)
		if ((group & 0x1) || (n_GroupList & group))
		{
			return EBU_OK;
		}
	}
	
	lstrcpy(szKey, lpAddIniValue->GetIniKeyName());
	ReplaceStringTokens(szKey, MAX_INI_BUF);
	lstrcpy(szValue, lpAddIniValue->GetIniValue());
	ReplaceStringTokens(szValue, MAX_INI_BUF);
	
	aivd.pszFileName = lpAddIniValue->GetIniFileName();
	aivd.pszSectionName = lpAddIniValue->GetIniSectionName();
	aivd.pszKeyName = szKey;
	aivd.pszValue = szValue;
	aivd.fUninstall = DO_INSTALL == bProcessType ? FALSE : TRUE;
	
	EBURETCODE retc = (*(GetAppCallback())) ((void *) &aivd);
	
	ASSERT(retc == EBU_OK || retc == EBU_CANCEL || retc == EBU_ABORT || retc == EBU_BACK);
	
	if (retc != EBU_OK)
	{
		return retc;
	}
	
	// Check out if we're supposed to map to either registry or ini file
	if( lpAddIniValue->GetMapFlag() )
	{
		HKEY hkey = NULL;
		
		if (lstrcmpi(lpAddIniValue->GetIniSectionName(), "universal") == 0 ||
			lstrcmpi(lpAddIniValue->GetIniSectionName(), "HKEY_LOCAL_MACHINE") == 0 )
			hkey = HKEY_LOCAL_MACHINE;
		else if (lstrcmpi(lpAddIniValue->GetIniSectionName(), "HKEY_CURRENT_USER") == 0)
			hkey = HKEY_CURRENT_USER;
		else if (lstrcmpi(lpAddIniValue->GetIniSectionName(), "HKEY_CLASSES_ROOT") == 0)
			hkey = HKEY_CLASSES_ROOT;
		else if (lstrcmpi(lpAddIniValue->GetIniSectionName(), "HKEY_USERS") == 0)
			hkey = HKEY_USERS;
		else if (lstrcmpi(lpAddIniValue->GetIniSectionName(), "HKEY_CURRENT_CONFIG") == 0)
			hkey = HKEY_CURRENT_CONFIG;
		else if (lstrcmpi(lpAddIniValue->GetIniSectionName(), "HKEY_DYN_DATA") == 0)
			hkey = HKEY_DYN_DATA;
		
		// ignore write failures, we don't care that much
		if (NULL != hkey)
		{
			if (DO_INSTALL == bProcessType)
			{
				MyWriteUniversalRegType(hkey, szKey, szValue, lpAddIniValue->GetIniType());
				
				SetDirtyBits(DIRTY_ADDINIVALUE);
			}
			else
			{
				//
				//Don't remove value if uninstall flag is not set
				//
				if (FALSE == lpAddIniValue->GetIniUninstall() &&
					FALSE == lpAddIniValue->GetIniUninstallAll())
				{
					return EBU_OK;
				}
				
				HKEY hkeyDelete;
				char *pchValue;
				
				// parse szKey into the real key name and value name
				pchValue = szKey;
				// run to the end of the string
				while (*pchValue && pchValue < szKey + MAX_INI_BUF)
					pchValue = CharNext(pchValue);
				if (pchValue == szKey + MAX_INI_BUF)
					return EBU_ERROR;
				// back un until we hit a '\'
				ASSERT('\\' != *pchValue);
				while ('\\' != *pchValue && pchValue > szKey)
					pchValue = CharPrev(szKey, pchValue);
				// break szKey into 2 null terminated strings
				*pchValue = 0;
				pchValue++;

				if (ERROR_SUCCESS == RegOpenKeyEx(hkey, szKey,
					NULL,
					KEY_READ | KEY_WRITE,
					&hkeyDelete))
				{
					if (FALSE == lpAddIniValue->GetIniUninstallAll())
					{
						long lRc;
					
						if (!lstrcmpi(pchValue, "NULL"))
							*pchValue = NULL;
					
						lRc = RegDeleteValue(hkeyDelete, pchValue);
						RegCloseKey(hkeyDelete);
					
						if (ERROR_SUCCESS == lRc)
						{
							// remove the whole key if it's empty
							MyRegDeleteEmptyKey(hkey, szKey, FALSE);
						}
						else
						{
#ifdef _DEBUG
							TRACE(STR_HARDCODE_ERROR_REMOVEREGKEY, szKey, pchValue);
#endif	
							return EBU_OK;
						}
					}
					else	// delete this key and all subkeys
					{
						MyRegDeleteSubKeys ( hkey, szKey );
						RegDeleteKey ( hkey, szKey );
					}
				}
				else
				{
					// couldn't open the key.  Probably was not a complete install so just continue
					return EBU_OK;
				}
			}
		}
		else
		{
			switch (lpAddIniValue->GetIniType())
			{
			case REG_SZ:
			case REG_EXPAND_SZ:
				MyWritePrivateProfileString(szKey, szValue);
				SetDirtyBits(DIRTY_ADDINIVALUE);
				
				break;
				
			default:
				return EBU_ERROR;
			}
		}
		return EBU_OK;
	}
	else  // we must be an actual .ini file...
	{
		char szFile[MAX_INI_BUF];
		
		// Build the full pathname of the ini file
		lstrcpy( szFile, lpAddIniValue->GetIniFileName() );
		ReplaceStringTokens(szFile, MAX_INI_BUF);
		
		if (DO_UNINSTALL == bProcessType)
		{
			//
			//Don't remove value if uninstall flag is not set
			//
			if (FALSE == lpAddIniValue->GetIniUninstall())
			{
				return EBU_OK;
			}
			
			// we need to remove the entry if it's an .ini file
			if (!WritePrivateProfileString(
				lpAddIniValue->GetIniSectionName(),
				szKey,
				NULL,
				szFile ))
			{
#ifdef _DEBUG
				DisplaySystemError(GetLastError(), MB_OK | MB_ICONWARNING);
#endif
				return EBU_ERROR;
			}
			
			return EBU_OK;
		}
		else
		{
			if (REG_DWORD == lpAddIniValue->GetIniType())
				return EBU_ERROR;
			
            // gonna write the string into a file
			// Check if the value is already there -ks 7/11/95
			GetPrivateProfileString(
				lpAddIniValue->GetIniSectionName(),
				szKey,
				"",
				sz,
				sizeof(char) * MAX_INI_BUF,
				szFile );
			
			if( 0 != lstrcmpi( sz, szValue ) )
				// We need to modify the string
			{
				
				// Check for read-only file
				if( 0 != _access( szFile, 06 ) )
					// 06 means read and write permission.	Go figure.
				{
					int nResult;
					
					if( !lstrcmpi( szFile, v_szLastIniFile ) )
					{
						// Already asked about this file, don't ask again.
						nResult = v_nLastIniResult;
					}
					else
					{
						nResult = IDYES;
						
						v_nLastIniResult = nResult;
						lstrcpyn( v_szLastIniFile, szFile, sizeof(szFile) );
					}
					
					if( IDNO == nResult )
						// They don't want us to dink with the .ini file, exit w/ success
						return EBU_OK;
					else
					{
						// Make it writable.
						_chmod( szFile, _S_IREAD | _S_IWRITE );
						fChangedAccess = TRUE;
						
						// Flush the cache
						WritePrivateProfileString( NULL, NULL, NULL, szFile );
					}
				}
				
				if (!WritePrivateProfileString(
					lpAddIniValue->GetIniSectionName(),
					szKey,
					szValue,
					szFile ))
				{
					DisplaySystemError(GetLastError(), MB_OK | MB_ICONWARNING);
				}
				
				if( fChangedAccess )
					// We should change the file back to read-only.
				{
					// Flush the cache
					WritePrivateProfileString( NULL, NULL, NULL, szFile );
					
					_chmod( szFile, _S_IREAD );
				}
				
				// ignore failures, we don't care that much
				return EBU_OK;
			}
			else
				return EBU_OK;	   // didn't need to change anything.
		}
   }
}


// ReadTTFInfo()
// Params:     pszFile -- name of TTF fontFile from which we are trying to retrieve info.
//             lpVersionStr -- version info will be retrieved into here
//             lpNameStr -- Font family and subfamily names will be retrieved into here.  If we
//                         don't care about the fontName, then passing in NULL for this parameter
//                         will skip the lookup of the fontName.
BOOL NEAR ReadTTFInfo(PSTR pszFile, LPSTR lpszVersionStr, LPSTR lpszFontName)
{   
  unsigned              i;
  int                   fp;
  unsigned short        numNames;
  unsigned              cTables;
  sfnt_OffsetTable      OffsetTable;
  sfnt_DirectoryEntry   Table;
  sfnt_NamingTable      NamingTable;
  sfnt_NameRecord       NameRecord;

  lpszVersionStr[0] = '\0';
  if ((fp = open (pszFile, O_RDONLY | O_BINARY)) == -1)
  {
#ifdef _DEBUG
      TRACE(STR_HARDCODE_CANTOPENFONTFILE, pszFile);
#endif
      return FALSE;
  }

  // First off, read the initial directory header on the TTF.  We're only
  // interested in the "numOffsets" variable to tell us how many tables
  // are present in this file.  
  //
  // Remember to always convert from Motorola format (Big Endian to 
  // Little Endian).
  //
  read (fp, &OffsetTable, sizeof (OffsetTable) - sizeof (sfnt_DirectoryEntry));
  cTables = (int) SWAPW (OffsetTable.numOffsets);

   for ( i = 0; i < cTables && i < 40; i++)
   {
      if ((read (fp, &Table, sizeof (Table))) != sizeof(Table)) 
         return FALSE;
      if (Table.tag == tag_NamingTable)	/* defined in sfnt_en.h */
      {
         // Now that we've found the entry for the name table, seek to that
         // position in the file and read in the initial header for this
         // particular table.  See "True Type Font Files" for information
         // on this record layout.
         lseek (fp, SWAPL (Table.offset), SEEK_SET);

         int nRead = read (fp, &NamingTable, sizeof (NamingTable));
		 ASSERT(nRead > 0);

         numNames = SWAPW(NamingTable.count);
         // Find NameID=5 in Naming Table; this gets us version info
         while (numNames--) 
         {
	         nRead = read (fp, &NameRecord, sizeof (NameRecord));
			 ASSERT(nRead > 0);

	         if (SWAPW(NameRecord.platformID) == 1 && SWAPW(NameRecord.nameID) == 5) 
            {
		         lseek (fp, SWAPW (NameRecord.offset) + SWAPW(NamingTable.stringOffset) + 
				               SWAPL(Table.offset), SEEK_SET);
		         nRead = read (fp, lpszVersionStr, SWAPW(NameRecord.length));
				 ASSERT(nRead > 0);

               lpszVersionStr[SWAPW(NameRecord.length)]  = '\0';

			   //
			   //After we've got version info, no need to keep reading so
			   //break out of the loop. a-richei:09/06/97
			   //
			   break;
	         }
         }
         if (NULL != lpszFontName)  // if we don't care about the name, let's skip this part
         {
            // reset to beginning of NamingTable
            lseek (fp, SWAPL (Table.offset), SEEK_SET);
            read (fp, &NamingTable, sizeof (NamingTable));
            numNames = SWAPW(NamingTable.count);
            // Now let's look for NameID=4, which is the Font family and subfamily name
            while (numNames--) 
            {
	            nRead = read (fp, &NameRecord, sizeof (NameRecord));
				ASSERT(nRead > 0);

	            if (SWAPW(NameRecord.platformID) == 1 && SWAPW(NameRecord.nameID) == 4) 
               {
		            lseek (fp, SWAPW (NameRecord.offset) + SWAPW(NamingTable.stringOffset) + 
				                  SWAPL(Table.offset), SEEK_SET);
		            nRead = read (fp, lpszFontName, SWAPW(NameRecord.length));
					ASSERT(nRead > 0);

                  lpszFontName[SWAPW(NameRecord.length)]  = '\0';

				   //
				   //After we've got font names, no need to keep reading so
				   //break out of the loop. a-richei:09/06/97
				   //
				   break;
               }
            }
        }
         close (fp);
         return TRUE;
      }
   }
   close (fp);
   return FALSE;
}

//----------------------------------------------------------------------------
// Procedure    MakeDestFontName
//
// Purpose      Constructs the path for the destination font file.
//
// Parameters   lpPath          destination path string buffer
//              lpLength        pointer to buffer length, returned path length
//
// Returns      none
//
// History       8/22/95    a-DenSo     Created
//

void MakeDestFontName (LPSTR lpPath, LPINT lpLength, LPSTR lpszName)
{
    LPSTR psz;
	
    // form path to destination directory
    int cch;
	
	cch = MyGetWindowsDirectory (lpPath, *lpLength);
	psz = lpPath + cch;
	if (cch < *lpLength)
	{
		UINT nchars = EBULoadString(GetResourceInst(), STR_DIRECTORY_FONTS, psz,
            *lpLength - cch);
		if (nchars != 0)
		{
			if (IsDBCS() && IsJapan())
			{
				for ( LPSTR p=psz ; *p ; p=CharNext(p) ){
               		if ( islower( *p ) ){
						*p = toupper( *p );
               		}
				}
			}
			else
			{
				AnsiUpper( psz );
			}

			cch += nchars;
			psz = lpPath + cch;
			if (cch < *lpLength)
			{
				ASSERT( *psz == '\0' );
				*psz = '\\';
				cch++;
			}
		}
	}
	
    // find the file name (sans path)
    LPCSTR name = pszGetLast5C( lpszName );
    if( name == NULL )
    {
        name = lpszName;
    }else{
        name = AnsiNext( name );
    }
	
    // append file name to the destination path
    lstrcpy (lpPath + cch, name);

	ReplaceStringTokens(lpPath, *lpLength);

    *lpLength = lstrlen (lpPath);
}

//****************************************************************************
// Procedure   PreloadFontCopy
//
// Purpose     Copies the file to its destination. 
// Parameters  
//             lpInstallFile  pointer to an INSTALLFILE struct describing
//                            the source and destination files.
//
// Returns     nonzero if successful, zero if setup was aborted.
//
//
EBURETCODE PreloadFontCopy(LPINSTALLFILE lpInstallFile )
{
	EBURETCODE nCopyResult = EBU_OK;
	
	char FileTo[_MAX_PATH];

	//
	//Setup pathnames for file copy
	//
	lstrcpy(FileTo,lpInstallFile->szName+lstrlen(lpInstallFile->szName)+1);
	assert(FileTo[0]);
/*	LPSTR from = lpInstallFile->szName;
	LPSTR to = FileTo;
	LPSTR lpTmp1;
	LPSTR lpTmp2;
	while(*from != '\0')
	{
		if(*from == '\\')
		{
			lpTmp1 = AnsiNext(from);
			lpTmp2 = AnsiNext(lpTmp1);
			if(*lpTmp1 == '.' && *lpTmp2 == '\\')
				from = lpTmp2;
		}
		lpTmp1 = AnsiNext(from);
		while( from != lpTmp1 )
			*to++ = *from++;
	}

	*to = '\0';
	*(to+1) = '\0';

	if (IsDBCS())
	{
		to = CharPrev( FileTo, to );
	}
	else
	{
		to--;
	}

	while(to != FileTo)
	{
		if(*to == '\\')
		{
			*to++ = '\0';
			*to = '\0';
			break;
		}
		to = AnsiPrev( FileTo, to );
	}
*/
	//Attempt to copy the file, this code (internally) will loop and retry as requested
	nCopyResult = MyCopyFile(lpInstallFile->szName, FileTo,
								TRUE, TRUE, NULL);

	return nCopyResult;
}

//****************************************************************************
// Procedure   PreloadFont
//
// Purpose     Executes the InstallFont setup command. If this is
//             successful, the font is added to the system.
//
// Parameters  
//             lpInstallFont  pointer to an INSTALLFONT struct describing
//                            the font and definition file.
//
// Returns     nonzero if successful, zero if setup was aborted.
//
// History      8/15/95 a-DenSo  Created
//
EBURETCODE PreloadFont(LPINSTALLFONT pInstallFont )
{
	char       szDestPath [MAX_PATH];
	int        dlen = MAX_PATH;
	char       lpszVersion[256];
	char       lpszFontname[256] = " ";
	char       szTemp[16];
	BOOL       fIsTrueTypeFont= FALSE;
	char       szCurrPath[MAX_PATH];
	char       szModDir[MAX_PATH];
	EBURETCODE nCopied        = EBU_CANCEL;
	EBURETCODE nReturn    = EBU_CANCEL;

	DWORD      dwWritten = GetCurrentDirectory(MAX_PATH, szCurrPath);
	GetModuleDirectory(szModDir, MAX_PATH);
	SetCurrentDirectory(szModDir);
	
	char szSourceFileName[_MAX_PATH];
	lstrcpy(szSourceFileName,pInstallFont->GetSourceFileName());
	ReplaceStringTokens(szSourceFileName, _MAX_PATH);
	
	// allocate memory for INSTALLFILE structure
	int slen = lstrlen (szSourceFileName);
	LPINSTALLFILE lpInstallFile = (LPINSTALLFILE)GlobalAllocPtr (GMEM_MOVEABLE, sizeof (INSTALLFILE) + slen + dlen + 1);
	if (lpInstallFile == NULL)
	{
		Alert (GetWndParent(), MB_OK | MB_ICONSTOP, STR_ERROR_NOMEMORY);
		nReturn = EBU_ERROR;
		goto Done;
	}
	MakeDestFontName (szDestPath, &dlen, pInstallFont->szName);
	
	// initialize INSTALLFILE structure with font file information
	lpInstallFile->FileInfo = *pInstallFont->GetSourceFileInfo ();
	lpInstallFile->wFlags = IF_FONTFILE;
	lpInstallFile->wDestOffset = slen + 1;
	lstrcpy (lpInstallFile->szName, szSourceFileName);
	lstrcpy (lpInstallFile->szName + slen + 1, szDestPath);
	
	// HACKHACK: we must append the group to make it always get copied; since PrepStub & engine
	//           didn't require a group for InstallFont script command, we don't have one. so hardcode it in.
	lstrcpy(lpInstallFile->GetDestPath() + lstrlen(szDestPath) +1, "1");
	// Append DISK_NOT_SPECIFIED to this command lpInstallFile structure 
	lstrcpy(lpInstallFile->GetDestPath() + lstrlen(szDestPath) +1 + 2, "-1");
	
	// if dest filename already exists, then we only need to copy fontfile if version is newer
	if (0xffffffff != GetFileAttributes(szDestPath))
	{
		LPSTR lpStr = pszGetLast5C(szDestPath);
		
		//
		//convert filename to lowercase so can match against lowercase .ttf.  BUGBUG:REVIEW:
		//will hardcoded string .ttf be the same in other languages?
		//
		CharLower(lpStr);
		if (NULL != strstr(lpStr, ".ttf"))
		{
			char szDestVer[256];
			
			// Compare our font version with the already installed font version
			// First check the version info of the font we wish to copy
			if (!ReadTTFInfo(lpInstallFile->szName, lpszVersion, lpszFontname))
			{
				nReturn = EBU_ERROR;
				goto Done;
			}
			
			// Then check version info of existing font in dest path
			if (!ReadTTFInfo(szDestPath, szDestVer, NULL))
			{
				nReturn = EBU_ERROR;
				goto Done;
			}
			
			TCHAR *pSrcVer;
			TCHAR *pDestVer;
			
			//
			//Look for the first numeric digit in the version string and assume that it
			//is the version number...
			//
			for (pSrcVer = lpszVersion; *pSrcVer; pSrcVer = CharNext(pSrcVer))
			{
				if (_istdigit(*pSrcVer))
				{
					break;
				}
			}
			
			for (pDestVer = szDestVer; *pDestVer; pDestVer = CharNext(pDestVer))
			{
				if (_istdigit(*pDestVer))
				{
					break;
				}
			}
			
			// if our font is a newer version, then copy file across
			if (atof(pSrcVer) > atof(pDestVer))
			{
				// free the font so that the file copy can complete successfully
				RemoveFontResource (szDestPath);
				
				// copy the font file across
				nCopied = PreloadFontCopy (lpInstallFile);

				
				if (EBU_OK == nCopied)
				{
					SetDirtyBits(DIRTY_INSTALLFONT);
				}
			}
			else
			{
				nCopied = EBU_OK;   // a copy of the font already exists
			}
		}
		else
		{
			// free the font so that the file copy can complete successfully
			RemoveFontResource (szDestPath);
			
			// Not TTF file; so copy it across without checking versions.
			nCopied = PreloadFontCopy (lpInstallFile);
			
			if (EBU_OK == nCopied)
			{
				SetDirtyBits(DIRTY_INSTALLFONT);
			}
		}
	}
	else
	{
		// font doesn't exist so copy the font file across
		nCopied = PreloadFontCopy (lpInstallFile);
		
		if (EBU_OK == nCopied)
		{
			SetDirtyBits(DIRTY_INSTALLFONT);
		}
		
		LPSTR lpStr = pszGetLast5C(szDestPath);
		//
		//convert filename to lowercase so can match against lowercase .ttf.  BUGBUG:REVIEW:
		//will hardcoded string .ttf be the same in other languages?
		//
		CharLower(lpStr);
		if (NULL != strstr(lpStr, ".ttf"))
		{
			// get fontName so we can store it in the registry later
			if (!ReadTTFInfo(lpInstallFile->szName, lpszVersion, lpszFontname))
			{
#ifdef _DEBUG
				TRACE(STR_HARDCODE_NOSOURCEFONT,lpInstallFile->szName);
#endif
				nReturn = EBU_ERROR;
				goto Done;
			}
		}
	}
	
	// If we didn't retrieve our fontName from the file, then let's use the fontFile's name
	if (' ' == lpszFontname[0])   
	{
		char * pChr = pszGetLast5C(szDestPath);
		
		ASSERT(pChr);
		
		lstrcpy(lpszFontname, ++pChr);
	}
	
	GlobalFreePtr (lpInstallFile);
	
	if (EBU_OK == nCopied)
	{
		// find the registry information for fonts
		char szRegistryKey [_MAX_PATH];
		if (GetOS() & OS_NTMASK)
		{
			//NT gets font information from different key than WIN40
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
			KEY_SET_VALUE, &hkFonts) != ERROR_SUCCESS)
		{
			nReturn = EBU_ERROR;
			goto Done;
		}
		
		//
		//convert filename to lowercase so can match against lowercase .ttf.  BUGBUG:REVIEW:
		//will hardcoded string .ttf be the same in other languages?
		//
		LPSTR lpStr = pszGetLast5C(szDestPath);
		CharLower(lpStr);
		if (NULL != strstr(lpStr, ".ttf"))
		{
			EBULoadString(GetResourceInst(), STR_TRUETYPE, szTemp, 16);
			lstrcat(lpszFontname, szTemp);
		}
		
		// add this font to the registry
		RegSetValueEx (hkFonts, lpszFontname, 0, REG_SZ,
			(BYTE FAR *) pInstallFont->GetFontName(), lstrlen (lpszFontname) + 1);
		RegCloseKey (hkFonts);
		
		// also install the font for immediate use
		AddFontResource (szDestPath);
		SendMessage(HWND_BROADCAST, WM_FONTCHANGE, 0, 0);
		nReturn = EBU_OK;
	}
	else
	{
		nReturn = nCopied;
	}
Done:
	// Reset current directory back
	SetCurrentDirectory(szCurrPath);
	return nReturn;
}

//
//Calculate and validate serial number checksum
//
EBURETCODE FValidPidSerial (LPSTR rgchSerial7 )
{
    INT i, wSum, ch;
	
    ASSERT(rgchSerial7 != NULL);
	
    for (i = wSum = 0; i < 7; i++)
	{
        ch = rgchSerial7[i];

#ifdef _DEBUG
		if (IsDBCS())
		{
	        // MEMO : if assert is failed, need to support DBCS.
		    ASSERT( !IsDBCSLeadByte(ch) );
		}
#endif
        if (ch == 'x')
            ch = 0;
        if (!isdigit(ch))
        {
			Alert( GetWndParent(), MB_ICONEXCLAMATION | MB_OK, STR_ERROR_BADCDKEY);
			return EBU_CANCEL;
        }
        wSum += ch - '0';
	}

    if (wSum == 0 || wSum % 7 != 0)
    {
		Alert( GetWndParent(), MB_ICONEXCLAMATION | MB_OK, STR_ERROR_BADCDKEY);
		return EBU_CANCEL;
    }
    return EBU_OK;
}

/*
**  Purpose:
**      Validate PID CD Key string.
**  Arguments:
**      sz: string to validate
**  Returns:
**      fTrue if valid.
****************************************************************************/
EBURETCODE WINAPI FValidCDKey (PTSTR PidData)
{
    INT i;
    TCHAR szmyCDKey[PID20LENGTH];

    ASSERT(PidData != NULL);
	LPGETPIDDATA pid = (LPGETPIDDATA) PidData;

	if (IsOEM())
	{
    	wsprintf(szmyCDKey, "%s%s%s", pid->SiteCode, pid->ProductID, pid->SerialNumber);
	}
    else
	{
		wsprintf(szmyCDKey,"%s%s", pid->SiteCode, pid->ProductID);
	}

	for (i = 0; i < (IsOEM() ? 17 : 10); i++)
	{
        if (!isdigit(szmyCDKey[i]))
        {
			Alert(GetWndParent(), MB_ICONEXCLAMATION | MB_OKCANCEL, STR_ERROR_BADCDKEY);
			return EBU_CANCEL;
        }
	}
	
	return FValidPidSerial(pid->ProductID);
}

static VOID DeleteDirectPlayFile(TCHAR *szFile)
{
	HANDLE hFile;

	//
	//If the file specified does exist...
	//
	if (0xFFFFFFFF != GetFileAttributes(szFile))
	{
		//
		//If DeleteFile API fails for whatever reason...
		//
		if (0 == DeleteFile(szFile))
		{
			//
			//Create the file with DELETE_ON_CLOSE flag so that the
			//operating system can delete it once the last handle
			//is closed...
			//
			hFile = CreateFile(szFile,
								  GENERIC_READ,
								  FILE_SHARE_READ,
								  NULL,
								  OPEN_EXISTING,
								  FILE_ATTRIBUTE_NORMAL | FILE_FLAG_DELETE_ON_CLOSE,
								  NULL);

			//
			//If we can't open the file with DELETE_ON_CLOSE for
			//any reason, make one last attempt to delete the file
			//
			if (INVALID_HANDLE_VALUE == hFile)
			{
				DeleteFile(szFile);
			}
			else
			{
				//
				//Close the handle to the file so the O.S. can delete
				//it.  The actual delete will occur when all other
				//processes accessing the file (if any) release their 
				//hold on it too...
				//
				CloseHandle(hFile);
			}
		}
	}
}


void MakeStrLowercase(PTCHAR pszLowerStr)
{
	// MEMO : ::CharLower() also converts DBCS alphabet.
	//        DBCS uppered alphabet and DBCS lowered alphabet is not
	//        treat as 'A' and 'a' under the file system.
	//        They are treated as the different character.
	for ( LPSTR p=pszLowerStr ; *p ; p=CharNext(p) ){
		if ( isupper( *p ) ){
			*p = tolower( *p );
		}
	}
}
