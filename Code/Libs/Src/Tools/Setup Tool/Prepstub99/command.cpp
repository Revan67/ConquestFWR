//
// command.cpp
//
//      PREPSTUB's method of keeping track of what's in a setup command.
//      not the same as the stub's command.cpp!!
//
// History:
//
//       2/02/95    KenSh       Created
//		03/15/97 update timestamp
//

#include "prepstub.h"
#include "textdoc.h"
#include "command.h"
#include "verutil.h"
#include "util.h"
#include <tchar.h>
#include <time.h>
#include "resource.h"
#include "stdio.h"
#include "diskinfo.h"

BOOL	g_fFileError = FALSE;

//
// A small fuction to copy string
//
VOID StrCpy_double5C( LPSTR& pszDst, LPCSTR pszSrc )
{
    LPSTR pszTmp;
    while( *pszSrc )
    {
        if( *pszSrc == '\\' )
        {
            *pszDst++ = '\\';
        }
        pszTmp = AnsiNext( pszSrc );
        while ( pszSrc != pszTmp )
            *pszDst++ = *pszSrc++;
    }
}


CCommand::CCommand()
{
	ZeroMemory(this, sizeof(CCommand));
	m_nDiskId = DISK_NOT_SPECIFIED;
}

CCommand::~CCommand()
{
    if( m_pszSourceName )
    {
        free( m_pszSourceName );
    }
	
    if( m_pszDestName )
    {
        free( m_pszDestName );
    }
	
    if( m_pszIniFilename )
    {
        free( m_pszIniFilename );
    }
	
    if( m_pszIniSection )
    {
        free( m_pszIniSection );
    }
	
    if( m_pszIniEntry )
    {
        free( m_pszIniEntry );
    }
	
    if( m_pszIniValue )
    {
        free( m_pszIniValue );
    }
	
	if (m_pszIniType)
	{
		free (m_pszIniType);
	}
	
    if( m_pszMkDirValue )
    {
        free( m_pszMkDirValue );
    }
    if( m_pszDeleteFileValue )
    {
        free( m_pszDeleteFileValue );
    }
    if( m_pszInstDXValue )
    {
        free( m_pszInstDXValue );
    }
    if( m_pszInstDXFlagsValue )
    {
        free( m_pszInstDXFlagsValue );
    }
    if( m_pszInstDXNameValue )
    {
        free( m_pszInstDXNameValue );
    }
	
	if( m_pszInstDXMinVersion )
    {
        free( m_pszInstDXMinVersion );
    }
	
    if( m_pszInstDPLAYNameValue )
    {
        free( m_pszInstDPLAYNameValue );
    }
	
	if( m_pszInstDPLAYMinVersion )
    {
        free( m_pszInstDPLAYMinVersion );
    }
	
    if ( m_pszInstIconNameValue)
    {
        free ( m_pszInstIconNameValue);
    }
    if ( m_pszInstIconNameIconValue)
    {
        free ( m_pszInstIconNameIconValue);
    }
    if ( m_pszInstIconDescriptionValue)
    {
        free ( m_pszInstIconDescriptionValue);
    }
    if ( m_pszInstIconIndexValue)
    {
        free ( m_pszInstIconIndexValue);
    }
    if (m_pszInstIconDestinationValue)
        free (m_pszInstIconDestinationValue);
	
	if(m_pszCDSpeedFileNameValue)
		free(m_pszCDSpeedFileNameValue);
	
    if(m_pszCDSpeedMinCDValue)
		free(m_pszCDSpeedMinCDValue);
	
    if(m_pszCDSpeedMaxCPUValue)
		free(m_pszCDSpeedMaxCPUValue);
	
	if(m_pszRegWizRegName)
		free (m_pszRegWizRegName);
	
	if (m_pszShellExecuteFileName)
		free (m_pszShellExecuteFileName);
	if (m_pszShellExecuteDirectory)
		free (m_pszShellExecuteDirectory);
	if (m_pszShellExecuteParameters)
		free (m_pszShellExecuteParameters);
	
	if (m_pszCabName)
		free (m_pszCabName);
	
	if (m_pszComment)
		free (m_pszComment);
	
	if (m_pszProperty)
		free (m_pszProperty);
	
	if (m_pszPropertyValue)
		free (m_pszPropertyValue);
	
	if (m_pszRulePattern)
		free (m_pszRulePattern);

	if (m_pszRuleAction)
		free (m_pszRuleAction);

	if (m_pszStringValue)
		free (m_pszStringValue);

	if (m_pszActionCommand)
		free (m_pszActionCommand);

	if (m_pszActionParam1)
		free (m_pszActionParam1);

	if (m_pszActionParam2)
		free (m_pszActionParam2);

	if (m_pszActionParam3)
		free (m_pszActionParam3);

	if (m_pszActionParam4)
		free (m_pszActionParam4);

	m_nStringID = -1;
}

BOOL CCommand::IsValidToken ()
{
	if (m_eCommandType == TOK_COMMENT  || 
		m_eCommandType == TOK_PROPERTY ||
		m_eCommandType == TOK_BEGINFILELIST ||
		m_eCommandType == TOK_ENDFILELIST ||
		m_eCommandType == TOK_RULE ||
		m_eCommandType == TOK_NEWLINE ||
		m_eCommandType == TOK_STRINGVAR ||
		m_eCommandType == TOK_BEGINSTRINGLIST ||
		m_eCommandType == TOK_ENDSTRINGLIST ||
		m_eCommandType == TOK_BEGINSTATICSTRINGLIST ||
		m_eCommandType == TOK_ENDSTATICSTRINGLIST ||
		m_eCommandType == TOK_ACTION)
		return false;
	else
		return true;
}


void CCommand::SetActionCommand (LPCSTR lpsz)
{
    int cch = lstrlen(lpsz);
    m_pszActionCommand = (char*)malloc( cch+1 );
    CopyMemory(m_pszActionCommand, lpsz, cch+1);
}

void CCommand::SetActionParam1 (LPCSTR lpsz)
{
    int cch = lstrlen(lpsz);
    m_pszActionParam1 = (char*)malloc( cch+1 );
    CopyMemory(m_pszActionParam1, lpsz, cch+1);
}

void CCommand::SetActionParam2 (LPCSTR lpsz)
{
    int cch = lstrlen(lpsz);
    m_pszActionParam2 = (char*)malloc( cch+1 );
    CopyMemory(m_pszActionParam2, lpsz, cch+1);
}

void CCommand::SetActionParam3 (LPCSTR lpsz)
{
    int cch = lstrlen(lpsz);
    m_pszActionParam3 = (char*)malloc( cch+1 );
    CopyMemory(m_pszActionParam3, lpsz, cch+1);
}

void CCommand::SetActionParam4 (LPCSTR lpsz)
{
    int cch = lstrlen(lpsz);
    m_pszActionParam4 = (char*)malloc( cch+1 );
    CopyMemory(m_pszActionParam4, lpsz, cch+1);
}

void CCommand::SetFileInfo (FILEINFO *fi)
{
	CopyMemory (&m_FileInfo, fi, sizeof (FILEINFO));
}

void CCommand::SetDestName( LPCSTR lpsz )
{
    int cch = lstrlen(lpsz);
	if (m_pszDestName)
	{
		free (m_pszDestName);
		m_pszDestName=NULL;
	}
    m_pszDestName = (char*)malloc( cch+1 );
    CopyMemory(m_pszDestName, lpsz, cch+1);
}

void CCommand::SetSourceName( LPCSTR lpsz )
{
    int cch = lstrlen(lpsz);
	if (m_pszSourceName)
	{
		free (m_pszSourceName);
		m_pszSourceName = NULL;
	}
    m_pszSourceName = (char*)malloc( cch+1 );
    CopyMemory(m_pszSourceName, lpsz, cch+1);
}

void CCommand::SetIniFilename( LPCSTR lpsz )
{
    int cch = lstrlen(lpsz);
    m_pszIniFilename = (char*)malloc( cch+1 );
    CopyMemory(m_pszIniFilename, lpsz, cch+1);
}

void CCommand::SetIniSection( LPCSTR lpsz )
{
    int cch = lstrlen(lpsz);
    m_pszIniSection = (char*)malloc( cch+1 );
    CopyMemory(m_pszIniSection, lpsz, cch+1);
}

void CCommand::SetIniEntry( LPCSTR lpsz )
{
    int cch = lstrlen(lpsz);
    m_pszIniEntry = (char*)malloc( cch+1 );
    CopyMemory(m_pszIniEntry, lpsz, cch+1);
}

void CCommand::SetIniValue( LPCSTR lpsz )
{
    int cch = lstrlen(lpsz);
    m_pszIniValue = (char*)malloc( cch+1 );
    CopyMemory(m_pszIniValue, lpsz, cch+1);
}

void CCommand::SetIniType( LPCSTR lpsz )
{
	int cch = lstrlen(lpsz);
	m_pszIniType = (char *) malloc( cch+1 );
	CopyMemory(m_pszIniType, lpsz, cch+1);
}

void CCommand::SetMkDirValue( LPCSTR lpsz )
{
    int cch = lstrlen(lpsz);
    m_pszMkDirValue = (char*)malloc( cch+1 );
    CopyMemory(m_pszMkDirValue, lpsz, cch+1);
}
void CCommand::SetDeleteFileValue( LPCSTR lpsz )
{
    int cch = lstrlen(lpsz);
    m_pszDeleteFileValue = (char*)malloc( cch+1 );
    CopyMemory(m_pszDeleteFileValue, lpsz, cch+1);
}

void CCommand::SetInstDXValue( LPCSTR lpsz )
{
    int cch = lstrlen(lpsz);
    m_pszInstDXValue = (char*)malloc( cch+1 );
    CopyMemory(m_pszInstDXValue, lpsz, cch+1);
}

void CCommand::SetInstDXFlagsValue( LPCSTR lpsz )
{
    int cch = lstrlen(lpsz);
    m_pszInstDXFlagsValue = (char*)malloc( cch+1 );
    CopyMemory(m_pszInstDXFlagsValue, lpsz, cch+1);
}

void CCommand::SetInstDXNameValue( LPCSTR lpsz )
{
    int cch = lstrlen(lpsz);
    m_pszInstDXNameValue = (char*)malloc( cch+1 );
    CopyMemory(m_pszInstDXNameValue, lpsz, cch+1);
}

void CCommand::SetInstDXMinVersionValue( LPCSTR lpsz )
{
    int cch = lstrlen(lpsz);
    m_pszInstDXMinVersion = (char*)malloc( cch+1 );
    CopyMemory(m_pszInstDXMinVersion, lpsz, cch+1);
}

void CCommand::SetInstDPLAYNameValue( LPCSTR lpsz )
{
    int cch = lstrlen(lpsz);
    m_pszInstDPLAYNameValue = (char*)malloc( cch+1 );
    CopyMemory(m_pszInstDPLAYNameValue, lpsz, cch+1);
}

void CCommand::SetInstDPLAYMinVersionValue( LPCSTR lpsz )
{
    int cch = lstrlen(lpsz);
    m_pszInstDPLAYMinVersion = (char*)malloc( cch+1 );
    CopyMemory(m_pszInstDPLAYMinVersion, lpsz, cch+1);
}


void CCommand::SetInstIconNameValue( LPCSTR lpsz )
{
    int cch = lstrlen(lpsz);
    m_pszInstIconNameValue = (char*)malloc( cch+1 );
    CopyMemory(m_pszInstIconNameValue, lpsz, cch+1);
}

void CCommand::SetInstIconNameIconValue( LPCSTR lpsz )
{
    int cch = lstrlen(lpsz);
    m_pszInstIconNameIconValue = (char*)malloc( cch+1 );
    CopyMemory(m_pszInstIconNameIconValue, lpsz, cch+1);
}

void CCommand::SetInstIconDescriptionValue( LPCSTR lpsz )
{
    int cch = lstrlen(lpsz);
    m_pszInstIconDescriptionValue = (char*)malloc( cch+1 );
    CopyMemory(m_pszInstIconDescriptionValue, lpsz, cch+1);
}
void CCommand::SetInstIconIndexValue( LPCSTR lpsz )
{
    int cch = lstrlen(lpsz);
    m_pszInstIconIndexValue = (char*)malloc( cch+1 );
    CopyMemory(m_pszInstIconIndexValue, lpsz, cch+1);
}


void CCommand::SetInstIconDestinationValue( LPCSTR lpsz )
{
    int cch = lstrlen(lpsz);
    m_pszInstIconDestinationValue = (char*)malloc( cch+1 );
    CopyMemory(m_pszInstIconDestinationValue, lpsz, cch+1);
}

void CCommand::SetCDSpeedFileNameValue( LPCSTR lpsz )
{
    int cch = lstrlen(lpsz);
    m_pszCDSpeedFileNameValue = (char*)malloc( cch+1 );
    CopyMemory(m_pszCDSpeedFileNameValue, lpsz, cch+1);
}

void CCommand::SetCDSpeedMinCDValue( LPCSTR lpsz )
{
    int cch = lstrlen(lpsz);
    m_pszCDSpeedMinCDValue = (char*)malloc( cch+1 );
    CopyMemory(m_pszCDSpeedMinCDValue, lpsz, cch+1);
}

void CCommand::SetCDSpeedMaxCPUValue( LPCSTR lpsz )
{
    int cch = lstrlen(lpsz);
    m_pszCDSpeedMaxCPUValue = (char*)malloc( cch+1 );
    CopyMemory(m_pszCDSpeedMaxCPUValue, lpsz, cch+1);
}

void CCommand::SetCabName( LPCSTR lpsz )
{
    int cch = lstrlen(lpsz);
    m_pszCabName = (char*)malloc( cch+1 );
    CopyMemory(m_pszCabName, lpsz, cch+1);
}

void CCommand::SetRegWizRegName( LPCSTR lpsz )
{
    int cch = lstrlen(lpsz);
    m_pszRegWizRegName = (char*)malloc( cch+1 );
    CopyMemory(m_pszRegWizRegName, lpsz, cch+1);
}

void CCommand::SetShellExecuteFileName( LPCSTR lpsz )
{
    int cch = lstrlen(lpsz);
    m_pszShellExecuteFileName = (char*)malloc( cch+1 );
    CopyMemory(m_pszShellExecuteFileName, lpsz, cch+1);
}

void CCommand::SetShellExecuteDirectory( LPCSTR lpsz )
{
    int cch = lstrlen(lpsz);
    m_pszShellExecuteDirectory = (char*)malloc( cch+1 );
    CopyMemory(m_pszShellExecuteDirectory, lpsz, cch+1);
}

void CCommand::SetShellExecuteParameters( LPCSTR lpsz )
{
    int cch = lstrlen(lpsz);
    m_pszShellExecuteParameters = (char*)malloc( cch+1 );
    CopyMemory(m_pszShellExecuteParameters, lpsz, cch+1);
}

void CCommand::SetReadFileListName( LPCSTR lpsz )
{
    int cch = lstrlen(lpsz);
    m_pszDestName = (char*)malloc( cch+1 );
    CopyMemory(m_pszDestName, lpsz, cch+1);
}

void CCommand::SetComment( LPCSTR lpsz )
{
    int cch = lstrlen(lpsz);
    m_pszComment = (char*)malloc( cch+1 );
    CopyMemory(m_pszComment, lpsz, cch+1);
}

void CCommand::SetProperty( LPCSTR lpsz )
{
    int cch = lstrlen(lpsz);
    m_pszProperty = (char*)malloc( cch+1 );
    CopyMemory(m_pszProperty, lpsz, cch+1);
}

void CCommand::SetPropertyValue ( LPCSTR lpsz )
{
    int cch = lstrlen(lpsz);
    m_pszPropertyValue = (char*)malloc( cch+1 );
    CopyMemory(m_pszPropertyValue, lpsz, cch+1);
}

void CCommand::SetRuleAction (LPCSTR lpsz)
{
    int cch = lstrlen(lpsz);
    m_pszRuleAction = (char*)malloc( cch+1 );
    CopyMemory(m_pszRuleAction, lpsz, cch+1);
}
void CCommand::SetRulePattern (LPCSTR lpsz)
{
    int cch = lstrlen(lpsz);
    m_pszRulePattern = (char*)malloc( cch+1 );
    CopyMemory(m_pszRulePattern, lpsz, cch+1);
}

void CCommand::SetStringID (int nStringID)
{
	m_nStringID = nStringID;
}

void CCommand::SetStringValue (LPCSTR lpsz)
{
    int cch = lstrlen(lpsz);
    m_pszStringValue = (char*)malloc( cch+1 );
    CopyMemory(m_pszStringValue, lpsz, cch+1);
}

BOOL CCommand::GetSourceVersionInfo( LPCSTR lpszStubPath )
{
    char szPath[_MAX_PATH];
    UINT uResult;
    WORD wOS = GetCurrentOperatingSystem();
	
    // build file path
    lstrcpy( szPath, lpszStubPath );
    lstrcat( szPath, m_pszSourceName );
	
TryAgain:
    uResult = EBUFileInfo( szPath, &m_FileInfo );
	
	//
	//If there was an error AND if the first char of the source file was not a %
	//sign (indicating a substitution token) then report and error...
	//
    if( uResult & FI_ERRORMASK )
    { 
		if (_tcschr(m_pszSourceName, '%'))
		{
			ZeroMemory(&m_FileInfo, sizeof(FILEINFO));
			return TRUE;
		}
		
        if( uResult & FI_ERR_NOEXIST )
        {
            int iErr = Alert( g_hwnd, MB_ICONSTOP | MB_ABORTRETRYIGNORE, STR_FILENOTFOUND, (LPCSTR)szPath );
			
			if ( iErr == IDIGNORE )
			{	g_fFileError = TRUE;
			return TRUE;
			}
			if ( iErr == IDRETRY )
				goto TryAgain;
        }
        else if( uResult & FI_ERR_CANTOPEN )
        {
            int iErr = Alert( g_hwnd, MB_ICONSTOP | MB_ABORTRETRYIGNORE, STR_CANTOPENSOURCE, (LPCSTR)szPath );
			if ( iErr == IDIGNORE )
			{	g_fFileError = TRUE;
			return TRUE;
			}
			if ( iErr == IDRETRY )
				goto TryAgain;
        }
        else
        {
            Alert( g_hwnd, MB_ICONSTOP | MB_OK, STR_NOMEMORY );
        }
		
        return FALSE;
    }
    else
    {
        return TRUE;
    }
}

DWORD CCommand::GetBuildFlags ()
{
	DWORD dwBuildFlags = 0;
	
	if (m_fWin95)
		dwBuildFlags |= OS_WIN95;
	
	if (m_fWin98)
		dwBuildFlags |= OS_WIN98;
	
	if (m_fNT40)
		dwBuildFlags |= OS_NT40;
	
	if (m_fNT50)
		dwBuildFlags |= OS_NT50;
	
	if (m_fDBCS)
		dwBuildFlags |= BLD_DBCS;
	
	if (m_fOEM)
		dwBuildFlags |= BLD_OEM;
	
	if (m_fRTL)
		dwBuildFlags |= BLD_RTL;
	
	if (m_fJPN)
		dwBuildFlags |= BLD_JPN;
	
	if (m_fGER)
		dwBuildFlags |= BLD_GER;
	
	if (m_fFRA)
		dwBuildFlags |= BLD_FRA;
	
	if (m_fSPA)
		dwBuildFlags |= BLD_SPA;
	
	if (m_fUSA)
		dwBuildFlags |= BLD_USA;
	
	if (m_fAPP1)
		dwBuildFlags |= BLD_APP1;

	if (m_fAPP2)
		dwBuildFlags |= BLD_APP2;

	if (m_fAPP3)
		dwBuildFlags |= BLD_APP3;
	
	if (m_fIMEON)
		dwBuildFlags |= SCF_IME_ON;
	
	if (m_fIMEENABLE)
		dwBuildFlags |= SCF_IME_ENABLE;

	if (m_fCabPreCopy)
		dwBuildFlags |= BLD_CAB_PRECOPY;
	
	return dwBuildFlags;
}


void CCommand::SetBuildFlags (DWORD dwBuildFlags)
{
	if (dwBuildFlags & OS_WIN95)
		m_fWin95 = true;
	else
		m_fWin95 = false;
	
	if (dwBuildFlags & OS_WIN98)
		m_fWin98 = true;
	else
		m_fWin98 = false;
	
	if (dwBuildFlags & OS_NT40)
		m_fNT40 = true;
	else
		m_fNT40 = false;
	
	if (dwBuildFlags & OS_NT50)
		m_fNT50 = true;
	else
		m_fNT50 = false;
	
	if (dwBuildFlags & BLD_DBCS)
		m_fDBCS = true;
	else
		m_fDBCS = false;
	
	if (dwBuildFlags & BLD_OEM)
		m_fOEM = true;
	else
		m_fOEM = false;
	
	if (dwBuildFlags & BLD_RTL)
		m_fRTL = true;
	else
		m_fRTL = false;
	
	if (dwBuildFlags & BLD_JPN)
		m_fJPN = true;
	else
		m_fJPN = false;
	
	if (dwBuildFlags & BLD_GER)
		m_fGER = true;
	else
		m_fGER = false;
	
	if (dwBuildFlags & BLD_FRA)
		m_fFRA = true;
	else
		m_fFRA = false;
	
	if (dwBuildFlags & BLD_SPA)
		m_fSPA = true;
	else
		m_fSPA = false;
	
	if (dwBuildFlags & BLD_USA)
		m_fUSA = true;
	else
		m_fUSA = false;
	
	if (dwBuildFlags & BLD_APP1)
		m_fAPP1 = true;
	else
		m_fAPP1 = false;

	if (dwBuildFlags & BLD_APP2)
		m_fAPP2 = true;
	else
		m_fAPP2 = false;

	if (dwBuildFlags & BLD_APP3)
		m_fAPP3 = true;
	else
		m_fAPP3 = false;
	
	if (dwBuildFlags & SCF_IME_ON)
		m_fIMEON = true;
	else
		m_fIMEON = false;
	
	if (dwBuildFlags & SCF_IME_ENABLE)
		m_fIMEENABLE = true;
	else
		m_fIMEENABLE = false;

	m_fCabPreCopy = (dwBuildFlags & BLD_CAB_PRECOPY)?true:false;

		
}


DWORD CCommand::GetInstallFlags ()
{
	DWORD dwInstallFlags = 0;

	if (m_fWindowsDir)
    {
        dwInstallFlags |= IF_WINDOWSDIR;
    }
    else if (m_fSystemDir)
    {
        dwInstallFlags |= IF_SYSTEMDIR;
    }
    else if (m_fAppDir)
    {
        dwInstallFlags |= IF_APPDIR;
    }
	/*   7/17/98 - CNH - IF_SYSTEM32DIR appears to have vanished!
    else if (m_fSystem32Dir)
	{
        dwInstallFlags |= IF_SYSTEM32DIR;
    }
	*/
	
    if (m_fSysFile)
    {
        dwInstallFlags |= IF_SYSTEMFILE;
    }
    if (m_fSharedFile)
    {
        dwInstallFlags |= IF_SHAREDFILE;
    }
    if (m_fDLLRegister)
    {
        dwInstallFlags |= IF_DLLREGISTER;
    }
    if (m_fUninstallFile)
    {
        dwInstallFlags |= IF_UNINSTALL;
    }
	if (m_fUninstallAll)
	{
		dwInstallFlags |= IF_UNINSTALLALL;
	}
    if (m_fCab)
    {
        dwInstallFlags |= IF_CAB;
    }

	return dwInstallFlags;
}


void CCommand::SetInstallFlags (DWORD dwInstallFlags)
{
	if (dwInstallFlags & IF_WINDOWSDIR)
		m_fWindowsDir = true;
	else
		m_fWindowsDir = false;

	if (dwInstallFlags & IF_SYSTEMDIR)
		m_fSystemDir = true;
	else	
		m_fSystemDir = false;

	if (dwInstallFlags & IF_APPDIR)
		m_fAppDir = true;
	else
		m_fAppDir = false;

	/*   7/17/98 - CNH - IF_SYSTEM32DIR appears to have vanished! */

	if (dwInstallFlags & IF_SYSTEMFILE)
		m_fSysFile = true;
	else
		m_fSysFile = false;
	
	if (dwInstallFlags & IF_SHAREDFILE)
		m_fSharedFile = true;
	else
		m_fSharedFile = false;
	
    if (dwInstallFlags & IF_DLLREGISTER)
		m_fDLLRegister = true;
	else
		m_fDLLRegister = false;
	
	if (dwInstallFlags & IF_UNINSTALL)
		m_fUninstallFile =  true;
	else
		m_fUninstallFile = false;

	if (dwInstallFlags & IF_UNINSTALLALL)
		m_fUninstallAll = true;
	else
		m_fUninstallAll = false;
	
	if (dwInstallFlags & IF_CAB)
		m_fCab = true;
	else
		m_fCab = false;

}

void	CCommand::SetUninstallFileFlag()
{
	m_fUninstallFile = TRUE;
	if ( m_fUninstallAll )
		MessageBox ( NULL, "Set either Uninstall or Uninstall_All, not both", g_szAppTitle, MB_OK);
}

void	CCommand::SetUninstallAllFlag()
{
	m_fUninstallAll = TRUE;
	if ( m_fUninstallFile )
		MessageBox ( NULL, "Set either Uninstall or Uninstall_All, not both", g_szAppTitle, MB_OK);
}

#if 0
BOOL CCommand::Write( HFILE hFile, int nBaseID )
{
    char szBuf[255];
	
    //
    // SETUPCOMMAND header
    //
    wsprintf( szBuf, "%d SETUPCOMMAND MOVEABLE\r\nBEGIN\r\n", nBaseID + GetCommandID() );
    _lwrite( hFile, (LPCSTR)szBuf, lstrlen(szBuf) );
	
    //
    // constant representing command
    //
	
    switch( m_eCommandType )
    {
	case TOK_INSTALL:
		lstrcpy( szBuf, "\tSC_INSTALLFILE\r\n" );
		break;
		
	case TOK_INIVALUE:
		lstrcpy( szBuf, "\tSC_ADDINIVALUE\r\n" );
		break;
		
	case TOK_INSTFONT:
		lstrcpy (szBuf, "\tSC_INSTALLFONT\r\n");
		break;
		
	case TOK_READFILELIST:
		lstrcpy (szBuf, "\tSC_READFILELIST\r\n");
		break;
		
	case TOK_MKDIR:
		lstrcpy (szBuf, "\tSC_MKDIR\r\n");
		break;
		
	case TOK_MKROOT:
		lstrcpy (szBuf, "\tSC_MKROOT\r\n");
		break;
		
	case TOK_GETNAME:
		lstrcpy (szBuf, "\tSC_GETNAME\r\n");
		break;
		
	case TOK_GETPID:
		lstrcpy (szBuf, "\tSC_GETPID\r\n");
		break;
		
	case TOK_INSTDX:
		lstrcpy (szBuf, "\tSC_INSTDX\r\n");
		break;
		
	case TOK_INSTICON:
		lstrcpy (szBuf, "\tSC_INSTICON\r\n");
		break;
		
	case TOK_CDSPEED:
		lstrcpy (szBuf, "\tSC_CDSPEED\r\n");
		break;
		
	case TOK_INSTALLLIST:
		lstrcpy (szBuf, "\tSC_INSTALLLIST\r\n");
		break;
		
	case TOK_INSTALLGO:
		lstrcpy (szBuf, "\tSC_INSTALLGO\r\n");
		break;
		
	case TOK_REGWIZ:
		lstrcpy (szBuf, "\tSC_REGWIZ\r\n");
		break;
		
	case TOK_SHELLEXECUTE:
		lstrcpy(szBuf, "\tSC_SHELLEXECUTE\r\n");
		break;
		
	case TOK_DELETEFILE :
		lstrcpy(szBuf,"\tSC_DELETEFILE\r\n");
		break;
		
	case TOK_GETGROUP:
		lstrcpy (szBuf, "\tSC_GETGROUP\r\n");
		break;
		
	case TOK_CABGO:
		lstrcpy (szBuf, "\tSC_CABGO\r\n");
		break;
		
	default:
		ASSERT(FALSE);
		return FALSE;
    }
	
    _lwrite( hFile, (LPCSTR)szBuf, lstrlen(szBuf) );
	
    //
    // resource ID of actual command
    //
    wsprintf( szBuf, "\t%d\t\t\t// resource ID of associated COMMANDDATA\r\n", nBaseID + GetCommandID() );
    _lwrite( hFile, (LPCSTR)szBuf, lstrlen(szBuf) );
	
	
    BOOL fFirst = TRUE;
    lstrcpy( szBuf, "// " );
	
    //
    // Write OS flags
    //
	
	BOOL dwBuildFlags = 0;
	
    if( m_fWin95 )
    {
        if( !fFirst )
        {
            lstrcat( szBuf, "|" );
        }
        lstrcat( szBuf, "OS_WIN95" );
        fFirst = FALSE;
		
		dwBuildFlags |= OS_WIN95;
    }
    if( m_fWin98 )
    {
        if( !fFirst )
        {
            lstrcat( szBuf, "|" );
        }
        lstrcat( szBuf, "OS_WIN98" );
        fFirst = FALSE;
		
		dwBuildFlags |= OS_WIN98;
    }
    if( m_fNT40 )
    {
        if( !fFirst )
        {
            lstrcat( szBuf, "|" );
        }
        lstrcat( szBuf, "OS_NT40" );
        fFirst = FALSE;
		
		dwBuildFlags |= OS_NT40;
    }
    if( m_fNT50 )
    {
        if( !fFirst )
        {
            lstrcat( szBuf, "|" );
        }
        lstrcat( szBuf, "OS_NT50" );
        fFirst = FALSE;
		
		dwBuildFlags |= OS_NT50;
    }
    if( m_fDBCS )
    {
        if( !fFirst )
        {
            lstrcat( szBuf, "|" );
        }
        lstrcat( szBuf, "BLD_DBCS" );
        fFirst = FALSE;
		
		dwBuildFlags |= BLD_DBCS;
    }
    if( m_fOEM )
    {
        if( !fFirst )
        {
            lstrcat( szBuf, "|" );
        }
        lstrcat( szBuf, "BLD_OEM" );
        fFirst = FALSE;
		
		dwBuildFlags |= BLD_OEM;
    }
    if( m_fRTL)
    {
        if( !fFirst )
        {
            lstrcat( szBuf, "|" );
        }
        lstrcat( szBuf, "BLD_RTL" );
        fFirst = FALSE;
		
		dwBuildFlags |= BLD_RTL;
    }
    if( m_fJPN )
    {
        if( !fFirst )
        {
            lstrcat( szBuf, "|" );
        }
        lstrcat( szBuf, "BLD_JPN" );
        fFirst = FALSE;
		
		dwBuildFlags |= BLD_JPN;
    }
    if( m_fGER )
    {
        if( !fFirst )
        {
            lstrcat( szBuf, "|" );
        }
        lstrcat( szBuf, "BLD_GER" );
        fFirst = FALSE;
		
		dwBuildFlags |= BLD_GER;
    }
    if( m_fFRA )
    {
        if( !fFirst )
        {
            lstrcat( szBuf, "|" );
        }
        lstrcat( szBuf, "BLD_FRA" );
        fFirst = FALSE;
		
		dwBuildFlags |= BLD_FRA;
    }
    if( m_fSPA )
    {
        if( !fFirst )
        {
            lstrcat( szBuf, "|" );
        }
        lstrcat( szBuf, "BLD_SPA" );
        fFirst = FALSE;
		
		dwBuildFlags |= BLD_SPA;
    }
    if( m_fUSA )
    {
        if( !fFirst )
        {
            lstrcat( szBuf, "|" );
        }
        lstrcat( szBuf, "BLD_USA" );
        fFirst = FALSE;
		
		dwBuildFlags |= BLD_USA;
    }
    if( m_fAPP1 )
    {
        if( !fFirst )
        {
            lstrcat( szBuf, "|" );
        }
        lstrcat( szBuf, "BLD_APP1" );
        fFirst = FALSE;
		
		dwBuildFlags |= BLD_APP1;
    }
    if( m_fAPP2 )
    {
        if( !fFirst )
        {
            lstrcat( szBuf, "|" );
        }
        lstrcat( szBuf, "BLD_APP2" );
        fFirst = FALSE;
		
		dwBuildFlags |= BLD_APP2;
    }
    if( m_fAPP3 )
    {
        if( !fFirst )
        {
            lstrcat( szBuf, "|" );
        }
        lstrcat( szBuf, "BLD_APP3" );
        fFirst = FALSE;
		
		dwBuildFlags |= BLD_APP3;
    }
	if( m_fIMEENABLE )
    {
        if( !fFirst )
        {
            lstrcat( szBuf, "|" );
        }
        lstrcat( szBuf, "SCF_IME_ENABLE" );
        fFirst = FALSE;
		
		dwBuildFlags |= SCF_IME_ENABLE;
    }
    if( m_fIMEON )
    {
        if( !fFirst )
        {
            lstrcat( szBuf, "|" );
        }
        lstrcat( szBuf, "SCF_IME_ON" );
        fFirst = FALSE;
		
		dwBuildFlags |= SCF_IME_ON;
    }
    if( fFirst )
    {
        Alert( g_hwnd, MB_ICONSTOP | MB_OK, STR_MUSTHAVEOSFLAG, GetCommandID() );
        return FALSE;
    }
	
	TCHAR szTmp[MAX_PATH];
	
	wsprintf(szTmp, "\t0x%04x, 0x%04x\t%s\r\n",
		(int) LOWORD(dwBuildFlags),
		(int) HIWORD(dwBuildFlags),
		szBuf);
    _lwrite( hFile, (LPCSTR)szTmp, lstrlen(szTmp) );
	
    // Finish it off
	
    lstrcpy( szBuf, "END\r\n\r\n" );
    _lwrite( hFile, (LPCSTR)szBuf, lstrlen(szBuf) );
	
    //
    // Now write the command-specific data
    //
	
    switch( m_eCommandType )
    {
	case TOK_INSTALL:
	case TOK_INSTALLLIST:
		return WriteInstallFile( hFile, nBaseID + GetCommandID() );
		
	case TOK_INIVALUE:
		return WriteIniValue( hFile, nBaseID + GetCommandID() );
		
	case TOK_INSTFONT:
		return WriteInstallFile(hFile, nBaseID + GetCommandID ());
		
	case TOK_READFILELIST:
		return WriteReadFileList (hFile, nBaseID + GetCommandID ());
		
	case TOK_MKDIR:
		return WriteMkDir (hFile, nBaseID + GetCommandID ());
		
	case TOK_MKROOT:
		return WriteMkRoot (hFile, nBaseID + GetCommandID ());
		
	case TOK_GETNAME:
		return WriteGetName (hFile, nBaseID + GetCommandID ());
		
	case TOK_GETGROUP:
		return WriteGetGroup (hFile, nBaseID + GetCommandID ());
		
	case TOK_GETPID:
		return WriteGetPID (hFile, nBaseID + GetCommandID ());
		
	case TOK_INSTDX:
		return WriteInstDX (hFile, nBaseID + GetCommandID ());
		
	case TOK_INSTDPLAY:
		return WriteInstDPLAY (hFile, nBaseID + GetCommandID ());
		
	case TOK_INSTICON:
		return WriteInstIcon (hFile, nBaseID + GetCommandID ());
		
	case TOK_CDSPEED:
		return WriteCDSpeed (hFile, nBaseID + GetCommandID ());
		
	case TOK_INSTALLGO:
		return WriteInstallGo (hFile, nBaseID + GetCommandID ());
		
	case TOK_CABGO:
		return WriteCabGo (hFile, nBaseID + GetCommandID ());
		
	case TOK_REGWIZ:
		return WriteRegWiz (hFile, nBaseID + GetCommandID ());
		
	case TOK_SHELLEXECUTE:
		return WriteShellExecute (hFile, nBaseID + GetCommandID ());
		
	case TOK_DELETEFILE:
		return WriteDeleteFile (hFile, nBaseID + GetCommandID ());
    }
	
    return TRUE;
}

BOOL CCommand::WriteInstallFile( HFILE hFile, int nCmdID )
{
    char szBuf[255];
    LPSTR pchSrc;
    LPSTR pchBuf;
	
    //
    // Header
    //
	
    wsprintf( szBuf, "%d COMMANDDATA MOVEABLE\r\nBEGIN\r\n", nCmdID );
    _lwrite( hFile, (LPCSTR)szBuf, lstrlen(szBuf) );
	
    //
    // FileInfo
    //
	
    //note that Intel processors are little endian, meaning we should write
    //each half of the version backwards.
    wsprintf( szBuf, "\t%d,%d,%d,%d\t\t// Version (a.b.c.d ==> b,a,d,c)\r\n", (int)LOWORD(m_FileInfo.dwFileVersionMS),
		(int)HIWORD(m_FileInfo.dwFileVersionMS), (int)LOWORD(m_FileInfo.dwFileVersionLS),
		(int)HIWORD(m_FileInfo.dwFileVersionLS) );
    _lwrite( hFile, (LPCSTR)szBuf, lstrlen(szBuf) );
	
    wsprintf( szBuf, "\t0x%04x, 0x%04x\t\t// Language\r\n",
		(int)LOWORD(m_FileInfo.dwLanguage), (int)HIWORD(m_FileInfo.dwLanguage) );
    _lwrite( hFile, (LPCSTR)szBuf, lstrlen(szBuf) );
	
    wsprintf( szBuf, "\t0x%04x, 0x%04x\t\t// File size (%lu)\r\n",
		(int)LOWORD(m_FileInfo.dwFileSize), (int)HIWORD(m_FileInfo.dwFileSize),
		(ULONG) m_FileInfo.dwFileSize);
    _lwrite( hFile, (LPCSTR)szBuf, lstrlen(szBuf) );
	
    wsprintf( szBuf, "\t0x%04x, 0x%04x\t\t// Timestamp\r\n",
		(int)LOWORD(m_FileInfo.FileTime), (int)HIWORD(m_FileInfo.FileTime) );
    _lwrite( hFile, (LPCSTR)szBuf, lstrlen(szBuf) );
	
    //
    // other flags
    //
	
    lstrcpy( szBuf, "\t" );

	BOOL bPrev = FALSE;

    if( m_fWindowsDir )
    {
        lstrcat( szBuf, "IF_WINDOWSDIR" );
		bPrev = TRUE;
    }
    else if( m_fSystemDir )
    {
        lstrcat( szBuf, "IF_SYSTEMDIR" );
		bPrev = TRUE;
    }
    else if( m_fAppDir )
    {
        lstrcat( szBuf, "IF_APPDIR" );
		bPrev = TRUE;
    }
/* SYSTEM32DIR is long gone
    else
    {
        lstrcat( szBuf, "IF_SYSTEM32DIR" );
    }
*/	
    if( m_fSysFile )
    {
		if ( bPrev )
        {
			lstrcat( szBuf, " | " );
		}
		else
			bPrev = TRUE;
        lstrcat( szBuf, "IF_SYSTEMFILE" );
    }
    if( m_fSharedFile )
    {
		if ( bPrev )
        {
			lstrcat( szBuf, " | " );
		}
		else
			bPrev = TRUE;
        lstrcat( szBuf, "IF_SHAREDFILE" );
    }
    if( m_fDLLRegister )
    {
		if ( bPrev )
        {
			lstrcat( szBuf, " | " );
		}
		else
			bPrev = TRUE;
        lstrcat( szBuf, "IF_DLLREGISTER" );
    }
    if( m_fUninstallFile )
    {
		if ( bPrev )
        {
			lstrcat( szBuf, " | " );
		}
		else
			bPrev = TRUE;
        lstrcat( szBuf, "IF_UNINSTALL" );
    }
    if( m_fCab )
    {
		if ( bPrev )
        {
			lstrcat( szBuf, " | " );
		}
		else
			bPrev = TRUE;
        lstrcat( szBuf, "IF_CAB" );
    }
    lstrcat( szBuf, "\r\n" );
    _lwrite( hFile, (LPCSTR)szBuf, lstrlen(szBuf) );
	
    //
    // Offset to dest name == length of source name + 1
    //
	
    wsprintf( szBuf, "\t%d\t\t\t// Length of Source + 1\r\n", lstrlen(m_pszSourceName)+1 );
    _lwrite( hFile, (LPCSTR)szBuf, lstrlen(szBuf) );
	
    //
    // Source Name, must change single backslashes to double
    //
	
    szBuf[0] = '\t';
    szBuf[1] = '\"';
    pchSrc = m_pszSourceName;
    pchBuf = szBuf+2;
    StrCpy_double5C( pchBuf, pchSrc );
    lstrcpy( pchBuf, "\\0\"\r\n" );
    _lwrite( hFile, (LPCSTR)szBuf, lstrlen(szBuf) );
	
    //
    // Dest Name, must change single backslashes to double
    //
	
    szBuf[0] = '\t';
    szBuf[1] = '\"';
    pchSrc = m_pszDestName;
    pchBuf = szBuf+2;
    StrCpy_double5C( pchBuf, pchSrc );
    lstrcpy( pchBuf, "\\0\"\r\n" );
    _lwrite( hFile, (LPCSTR)szBuf, lstrlen(szBuf) );
	
	// GroupId
	szBuf[0] = '\t';
    szBuf[1] = '\"';
    sprintf((szBuf+2),"%I64X",m_cGroup);
	pchBuf=szBuf+lstrlen(szBuf);
    lstrcpy( pchBuf, "\\0\"\r\n" );
    _lwrite( hFile, (LPCSTR)szBuf, lstrlen(szBuf) );
	
	// DiskId
	szBuf[0] = '\t';
    szBuf[1] = '\"';
    sprintf((szBuf+2),"%d",m_nDiskId);
	pchBuf=szBuf+lstrlen(szBuf);
    lstrcpy( pchBuf, "\\0\"\t //DiskID \r\n" );
    _lwrite( hFile, (LPCSTR)szBuf, lstrlen(szBuf) );
	
    // Finish up
	
    wsprintf( szBuf, "END\r\n\r\n" );
    _lwrite( hFile, (LPCSTR)szBuf, lstrlen(szBuf) );
	
    return TRUE;
}


BOOL CCommand::WriteMkDir( HFILE hFile, int nCmdID )
{
    char szBuf[255];
    LPSTR pchSrc;
    LPSTR pchBuf;
	
    //
    // Header
    //
	
    wsprintf( szBuf, "%d COMMANDDATA MOVEABLE\r\nBEGIN\r\n", nCmdID );
    _lwrite( hFile, (LPCSTR)szBuf, lstrlen(szBuf) );
	
    //
    // FileInfo
    //
	
    // other flags
    //
	
    lstrcpy( szBuf, "\t" );
	
    if( m_fUninstallFile )
    {
        lstrcat( szBuf, "IF_UNINSTALL" );
    }
    else if( m_fUninstallAll )
    {
        lstrcat( szBuf, "IF_UNINSTALLALL" );
    }
	else
        lstrcat( szBuf, "0" );
	
    lstrcat( szBuf, "\r\n" );
    _lwrite( hFile, (LPCSTR)szBuf, lstrlen(szBuf) );
	
    //
    // Source Name, must change single backslashes to double
    //
	
    szBuf[0] = '\t';
    szBuf[1] = '\"';
    pchSrc = m_pszMkDirValue;
    pchBuf = szBuf+2;
    StrCpy_double5C( pchBuf, pchSrc );
    lstrcpy( pchBuf, "\\0\"\r\n" );
    _lwrite( hFile, (LPCSTR)szBuf, lstrlen(szBuf) );
	
	szBuf[0] = '\t';
    szBuf[1] = '\"';
    sprintf((szBuf+2),"%I64X",m_cDirGroup);
	pchBuf=szBuf+lstrlen(szBuf);
    lstrcpy( pchBuf, "\\0\"\r\n" );
    _lwrite( hFile, (LPCSTR)szBuf, lstrlen(szBuf) );
    // Finish up
	
    wsprintf( szBuf, "END\r\n\r\n" );
    _lwrite( hFile, (LPCSTR)szBuf, lstrlen(szBuf) );
	
    return TRUE;
}
BOOL CCommand::WriteMkRoot( HFILE hFile, int nCmdID )
{
    char szBuf[255];
	
    //
    // Header
    //
	
    wsprintf( szBuf, "%d COMMANDDATA MOVEABLE\r\nBEGIN\r\n", nCmdID );
    _lwrite( hFile, (LPCSTR)szBuf, lstrlen(szBuf) );
	
    //
    // FileInfo
    //
	
    // other flags
    //
	
    lstrcpy( szBuf, "\t" );
	
    if( m_fUninstallFile )
    {
        lstrcat( szBuf, "IF_UNINSTALL" );
    }
	else
		lstrcat( szBuf, "0" );
	
    lstrcat( szBuf, "\r\n" );
    _lwrite( hFile, (LPCSTR)szBuf, lstrlen(szBuf) );
	
    // Finish up
	
    wsprintf( szBuf, "END\r\n\r\n" );
    _lwrite( hFile, (LPCSTR)szBuf, lstrlen(szBuf) );
	
    return TRUE;
}
BOOL CCommand::WriteGetName( HFILE hFile, int nCmdID )
{
    char szBuf[255];
	
    //
    // Header
    //
	
    wsprintf( szBuf, "%d COMMANDDATA MOVEABLE\r\nBEGIN\r\n", nCmdID );
    _lwrite( hFile, (LPCSTR)szBuf, lstrlen(szBuf) );
	
    //
    // FileInfo
    //
	
    // other flags
    //
	
    lstrcpy( szBuf, "\t" );
	
    lstrcat( szBuf, "0" );
    lstrcat( szBuf, "\r\n" );
    _lwrite( hFile, (LPCSTR)szBuf, lstrlen(szBuf) );
	
    // Finish up
	
    wsprintf( szBuf, "END\r\n\r\n" );
    _lwrite( hFile, (LPCSTR)szBuf, lstrlen(szBuf) );
	
    return TRUE;
}
BOOL CCommand::WriteGetGroup( HFILE hFile, int nCmdID )
{
    char szBuf[255];
	
    //
    // Header
    //
	
    wsprintf( szBuf, "%d COMMANDDATA MOVEABLE\r\nBEGIN\r\n", nCmdID );
    _lwrite( hFile, (LPCSTR)szBuf, lstrlen(szBuf) );
	
    //
    // FileInfo
    //
	
    // other flags
    //
	
    lstrcpy( szBuf, "\t" );
	
    lstrcat( szBuf, "0" );
    lstrcat( szBuf, "\r\n" );
    _lwrite( hFile, (LPCSTR)szBuf, lstrlen(szBuf) );
	
    // Finish up
	
    wsprintf( szBuf, "END\r\n\r\n" );
    _lwrite( hFile, (LPCSTR)szBuf, lstrlen(szBuf) );
	
    return TRUE;
}
BOOL CCommand::WriteDeleteFile( HFILE hFile, int nCmdID )
{
    char szBuf[255];
    LPSTR pchSrc;
    LPSTR pchBuf;
	
    //
    // Header
    //
	
    wsprintf( szBuf, "%d COMMANDDATA MOVEABLE\r\nBEGIN\r\n", nCmdID );
    _lwrite( hFile, (LPCSTR)szBuf, lstrlen(szBuf) );
	
    // other flags
    //
    lstrcpy( szBuf, "\t" );
	
    lstrcat( szBuf, m_fUninstallFile ? "IF_UNINSTALL\t//silent delete" : "0");
    lstrcat( szBuf, "\r\n" );
    _lwrite( hFile, (LPCSTR)szBuf, lstrlen(szBuf) );
	
    //
    // Source Name, must change single backslashes to double
    //
	
    szBuf[0] = '\t';
    szBuf[1] = '\"';
    pchSrc = m_pszDeleteFileValue;
    pchBuf = szBuf+2;
    StrCpy_double5C( pchBuf, pchSrc );
    lstrcpy( pchBuf, "\\0\"\r\n" );
    _lwrite( hFile, (LPCSTR)szBuf, lstrlen(szBuf) );
	
    // Finish up
	
    wsprintf( szBuf, "END\r\n\r\n" );
    _lwrite( hFile, (LPCSTR)szBuf, lstrlen(szBuf) );
	
    return TRUE;
}
BOOL CCommand::WriteGetPID( HFILE hFile, int nCmdID )
{
    char szBuf[255];
	
    //
    // Header
    //
	
    wsprintf( szBuf, "%d COMMANDDATA MOVEABLE\r\nBEGIN\r\n", nCmdID );
    _lwrite( hFile, (LPCSTR)szBuf, lstrlen(szBuf) );
	
    //
    // FileInfo
    //
	
    // other flags
    //
	
    lstrcpy( szBuf, "\t" );
	
    lstrcat( szBuf, "0" );
    lstrcat( szBuf, "\r\n" );
    _lwrite( hFile, (LPCSTR)szBuf, lstrlen(szBuf) );
	
    wsprintf( szBuf, "END\r\n\r\n" );
    _lwrite( hFile, (LPCSTR)szBuf, lstrlen(szBuf) );
	
    return TRUE;
}
BOOL CCommand::WriteInstDX( HFILE hFile, int nCmdID )
{
    char szBuf[255];
    LPSTR pchSrc;
    LPSTR pchBuf;
	
    //
    // Header
    //
	
    wsprintf( szBuf, "%d COMMANDDATA MOVEABLE\r\nBEGIN\r\n", nCmdID );
    _lwrite( hFile, (LPCSTR)szBuf, lstrlen(szBuf) );
	
    //
    // FileInfo
    //
	
    // other flags
    //
	
    lstrcpy( szBuf, "\t" );
	
    lstrcat( szBuf, "0" );
    lstrcat( szBuf, "\r\n" );
    _lwrite( hFile, (LPCSTR)szBuf, lstrlen(szBuf) );
	
    //
    // Source Name, must change single backslashes to double
    //
	
    szBuf[0] = '\t';
    szBuf[1] = '\"';
    pchSrc = m_pszInstDXValue;
    pchBuf = szBuf+2;
    lstrcpy(pchBuf,pchSrc);
    lstrcat( pchBuf, "\\0\"\r\n" );
    _lwrite( hFile, (LPCSTR)szBuf, lstrlen(szBuf) );
	
    szBuf[0] = '\t';
    szBuf[1] = '\"';
    pchSrc = m_pszInstDXNameValue;
    pchBuf = szBuf+2;
    lstrcpy(pchBuf,pchSrc);
    lstrcat( pchBuf, "\\0\"\r\n" );
    _lwrite( hFile, (LPCSTR)szBuf, lstrlen(szBuf) );
	
    szBuf[0] = '\t';
    szBuf[1] = '\"';
    pchSrc = m_pszInstDXMinVersion;
    pchBuf = szBuf+2;
    lstrcpy(pchBuf,pchSrc);
    lstrcat( pchBuf, "\\0\"\r\n" );
    _lwrite( hFile, (LPCSTR)szBuf, lstrlen(szBuf) );
	
    szBuf[0] = '\t';
    pchSrc = m_pszInstDXFlagsValue;
    pchBuf = szBuf+1;
    lstrcpy(pchBuf,pchSrc);
    lstrcat( pchBuf, " + 0L\r\n" );
    _lwrite( hFile, (LPCSTR)szBuf, lstrlen(szBuf) );
	
	
	
	// Finish up
	
    wsprintf( szBuf, "END\r\n\r\n" );
    _lwrite( hFile, (LPCSTR)szBuf, lstrlen(szBuf) );
	
    return TRUE;
}
BOOL CCommand::WriteInstDPLAY( HFILE hFile, int nCmdID )
{
    char szBuf[255];
    LPSTR pchSrc;
    LPSTR pchBuf;
	
    //
    // Header
    //
	
    wsprintf( szBuf, "%d COMMANDDATA MOVEABLE\r\nBEGIN\r\n", nCmdID );
    _lwrite( hFile, (LPCSTR)szBuf, lstrlen(szBuf) );
	
    //
    // FileInfo
    //
	
    // other flags
    //
	
    lstrcpy( szBuf, "\t" );
	
    lstrcat( szBuf, "0" );
    lstrcat( szBuf, "\r\n" );
    _lwrite( hFile, (LPCSTR)szBuf, lstrlen(szBuf) );
	
    //
    // Source Name, must change single backslashes to double
    //
	
/*    szBuf[0] = '\t';
    szBuf[1] = '\"';
    pchSrc = m_pszInstDPLAYValue;
    pchBuf = szBuf+2;
    lstrcpy(pchBuf,pchSrc);
    lstrcat( pchBuf, "\\0\"\r\n" );
    _lwrite( hFile, (LPCSTR)szBuf, lstrlen(szBuf) );
	
*/	szBuf[0] = '\t';
    szBuf[1] = '\"';
    pchSrc = m_pszInstDPLAYNameValue;
    pchBuf = szBuf+2;
    lstrcpy(pchBuf,pchSrc);
    lstrcat( pchBuf, "\\0\"\r\n" );
    _lwrite( hFile, (LPCSTR)szBuf, lstrlen(szBuf) );
	
    szBuf[0] = '\t';
    szBuf[1] = '\"';
    pchSrc = m_pszInstDPLAYMinVersion;
    pchBuf = szBuf+2;
    lstrcpy(pchBuf,pchSrc);
    lstrcat( pchBuf, "\\0\"\r\n" );
    _lwrite( hFile, (LPCSTR)szBuf, lstrlen(szBuf) );
	
    szBuf[0] = '\t';
    pchSrc = m_pszInstDPLAYFlagsValue;
    pchBuf = szBuf+1;
    lstrcpy(pchBuf,pchSrc);
    lstrcat( pchBuf, " + 0L\r\n" );
    _lwrite( hFile, (LPCSTR)szBuf, lstrlen(szBuf) );
	
	
	
	// Finish up
	
    wsprintf( szBuf, "END\r\n\r\n" );
    _lwrite( hFile, (LPCSTR)szBuf, lstrlen(szBuf) );
	
    return TRUE;
}
BOOL CCommand::WriteInstIcon( HFILE hFile, int nCmdID )
{
    char szBuf[255];
    LPSTR pchSrc;
    LPSTR pchBuf;
	
    //
    // Header
    //
	
    wsprintf( szBuf, "%d COMMANDDATA MOVEABLE\r\nBEGIN\r\n", nCmdID );
    _lwrite( hFile, (LPCSTR)szBuf, lstrlen(szBuf) );
	
    //
    // FileInfo
    //
	
    // other flags
    //
	
    lstrcpy( szBuf, "\t" );
	
    lstrcat( szBuf, "0" );
    lstrcat( szBuf, "\r\n" );
    _lwrite( hFile, (LPCSTR)szBuf, lstrlen(szBuf) );
	
    //
    // Source Name, must change single backslashes to double
    //
	
    szBuf[0] = '\t';
    szBuf[1] = '\"';
    pchSrc = m_pszInstIconNameValue;
    pchBuf = szBuf+2;
    StrCpy_double5C( pchBuf, pchSrc );
    lstrcpy( pchBuf, "\\0\"\r\n" );
    _lwrite( hFile, (LPCSTR)szBuf, lstrlen(szBuf) );
	
    szBuf[0] = '\t';
    szBuf[1] = '\"';
    pchSrc = m_pszInstIconNameIconValue;
    pchBuf = szBuf+2;
    StrCpy_double5C( pchBuf, pchSrc );
    lstrcpy( pchBuf, "\\0\"\r\n" );
    _lwrite( hFile, (LPCSTR)szBuf, lstrlen(szBuf) );
	
    szBuf[0] = '\t';
    szBuf[1] = '\"';
    pchSrc = m_pszInstIconDescriptionValue;
    pchBuf = szBuf+2;
    StrCpy_double5C( pchBuf, pchSrc );
    lstrcpy( pchBuf, "\\0\"\r\n" );
    _lwrite( hFile, (LPCSTR)szBuf, lstrlen(szBuf) );
	
    szBuf[0] = '\t';
    szBuf[1] = '\"';
    pchSrc = m_pszInstIconDestinationValue;
    pchBuf = szBuf+2;
    StrCpy_double5C( pchBuf, pchSrc );
    lstrcpy( pchBuf, "\\0\"\r\n" );
    _lwrite( hFile, (LPCSTR)szBuf, lstrlen(szBuf) );
	
	
    szBuf[0] = '\t';
	
    pchBuf = szBuf+1;
    pchSrc = m_pszInstIconIndexValue;
    lstrcpy(pchBuf,pchSrc);
    lstrcat( pchBuf, "\r\n" );
    _lwrite( hFile, (LPCSTR)szBuf, lstrlen(szBuf) );
	
	szBuf[0] = '\t';
    szBuf[1] = '\"';
    sprintf((szBuf+2),"%I64X",m_cGroup);
	pchBuf=szBuf+lstrlen(szBuf);
    lstrcpy( pchBuf, "\\0\"\r\n" );
    _lwrite( hFile, (LPCSTR)szBuf, lstrlen(szBuf) );
	
	
    // Finish up
	
    wsprintf( szBuf, "END\r\n\r\n" );
    _lwrite( hFile, (LPCSTR)szBuf, lstrlen(szBuf) );
	
    return TRUE;
}
BOOL CCommand::WriteCDSpeed( HFILE hFile, int nCmdID )
{
    char szBuf[255];
    LPSTR pchSrc;
    LPSTR pchBuf;
	
    //
    // Header
    //
	
    wsprintf( szBuf, "%d COMMANDDATA MOVEABLE\r\nBEGIN\r\n", nCmdID );
    _lwrite( hFile, (LPCSTR)szBuf, lstrlen(szBuf) );
	
    //
    // FileInfo
    //
	
    // other flags
    //
	
    lstrcpy( szBuf, "\t" );
	
    lstrcat( szBuf, "0" );
    lstrcat( szBuf, "\r\n" );
    _lwrite( hFile, (LPCSTR)szBuf, lstrlen(szBuf) );
	
    szBuf[0] = '\t';
    pchSrc = m_pszCDSpeedMinCDValue;
    pchBuf = szBuf+1;
    lstrcpy(pchBuf,pchSrc);
    pchBuf+=lstrlen(pchSrc);
    lstrcpy( pchBuf, "\r\n" );
    _lwrite( hFile, (LPCSTR)szBuf, lstrlen(szBuf) );
	
    szBuf[0] = '\t';
    pchSrc = m_pszCDSpeedMaxCPUValue;
    pchBuf = szBuf+1;
    lstrcpy(pchBuf,pchSrc);
    pchBuf+=lstrlen(pchSrc);
    lstrcpy( pchBuf, "\r\n" );
    _lwrite( hFile, (LPCSTR)szBuf, lstrlen(szBuf) );
	
    szBuf[0] = '\t';
    szBuf[1] = '\"';
    pchSrc = m_pszCDSpeedFileNameValue;
    pchBuf = szBuf+2;
    StrCpy_double5C( pchBuf, pchSrc );
    lstrcpy( pchBuf, "\\0\"\r\n" );
    _lwrite( hFile, (LPCSTR)szBuf, lstrlen(szBuf) );
    // Finish up
	
    wsprintf( szBuf, "END\r\n\r\n" );
    _lwrite( hFile, (LPCSTR)szBuf, lstrlen(szBuf) );
	
    return TRUE;
}


BOOL CCommand::WriteInstallGo( HFILE hFile, int nCmdID )
{
    char szBuf[255];
	
	// Bail out if any file access problems ocurred during installlist scan
	// Note: We leave a crappy output file behind on exit
	if ( g_fFileError )
	{
		g_fFileError = FALSE;
		return FALSE;
	}
    //
    // Header
    //
	
    wsprintf( szBuf, "%d COMMANDDATA MOVEABLE\r\nBEGIN\r\n", nCmdID );
    _lwrite( hFile, (LPCSTR)szBuf, lstrlen(szBuf) );
	
    lstrcpy( szBuf, "\t" );
	
    lstrcat( szBuf, "0" );
    lstrcat( szBuf, "\r\n" );
    _lwrite( hFile, (LPCSTR)szBuf, lstrlen(szBuf) );
	
    // Finish up
	
    wsprintf( szBuf, "END\r\n\r\n" );
    _lwrite( hFile, (LPCSTR)szBuf, lstrlen(szBuf) );
	
    return TRUE;
}

BOOL CCommand::WriteCabGo( HFILE hFile, int nCmdID )
{
    char szBuf[255];
    LPSTR pchSrc;
    LPSTR pchBuf;
    //
    // Header
    //
	
    wsprintf( szBuf, "%d COMMANDDATA MOVEABLE\r\nBEGIN\r\n", nCmdID );
    _lwrite( hFile, (LPCSTR)szBuf, lstrlen(szBuf) );
	
    //
    // FileInfo
    //
	
    // other flags
    //
    szBuf[0] = '\t';
    szBuf[1] = '\"';
    pchSrc = m_pszCabName;
    pchBuf = szBuf+2;
    StrCpy_double5C( pchBuf, pchSrc );
    lstrcpy( pchBuf, "\\0\"\r\n" );
    _lwrite( hFile, (LPCSTR)szBuf, lstrlen(szBuf) );
	
    // Finish up
	
    wsprintf( szBuf, "END\r\n\r\n" );
    _lwrite( hFile, (LPCSTR)szBuf, lstrlen(szBuf) );
	
    return TRUE;
}
BOOL CCommand::WriteRegWiz( HFILE hFile, int nCmdID )
{
    char szBuf[255];
    LPSTR pchSrc;
    LPSTR pchBuf;
    //
    // Header
    //
	
    wsprintf( szBuf, "%d COMMANDDATA MOVEABLE\r\nBEGIN\r\n", nCmdID );
    _lwrite( hFile, (LPCSTR)szBuf, lstrlen(szBuf) );
	
    //
    // FileInfo
    //
	
    // other flags
    //
    szBuf[0] = '\t';
    szBuf[1] = '\"';
    pchSrc = m_pszRegWizRegName;
    pchBuf = szBuf+2;
    StrCpy_double5C( pchBuf, pchSrc );
    lstrcpy( pchBuf, "\\0\"\r\n" );
    _lwrite( hFile, (LPCSTR)szBuf, lstrlen(szBuf) );
	
    // Finish up
	
    wsprintf( szBuf, "END\r\n\r\n" );
    _lwrite( hFile, (LPCSTR)szBuf, lstrlen(szBuf) );
	
    return TRUE;
}

BOOL CCommand::WriteShellExecute( HFILE hFile, int nCmdID )
{
    char szBuf[255];
    LPSTR pchSrc;
    LPSTR pchBuf;
    //
    // Header
    //
	
    wsprintf( szBuf, "%d COMMANDDATA MOVEABLE\r\nBEGIN\r\n", nCmdID );
    _lwrite( hFile, (LPCSTR)szBuf, lstrlen(szBuf) );
	
    //
    // FileInfo
    //
	
    // other flags
    //
    lstrcpy( szBuf, "\t" );
	
    if( m_fUninstallFile )
    {
        lstrcat( szBuf, "IF_UNINSTALL" );
    }
	else
	{
        lstrcat( szBuf, "0" );
	}
	
    lstrcat( szBuf, "\r\n" );
    _lwrite( hFile, (LPCSTR)szBuf, lstrlen(szBuf) );
    
	szBuf[0] = '\t';
    szBuf[1] = '\"';
    pchSrc = m_pszShellExecuteFileName;
    pchBuf = szBuf+2;
    StrCpy_double5C( pchBuf, pchSrc );
    lstrcpy( pchBuf, "\\0\"\r\n" );
    _lwrite( hFile, (LPCSTR)szBuf, lstrlen(szBuf) );
	
    szBuf[0] = '\t';
    szBuf[1] = '\"';
    pchSrc = m_pszShellExecuteDirectory;
    pchBuf = szBuf+2;
    StrCpy_double5C( pchBuf, pchSrc );
    lstrcpy( pchBuf, "\\0\"\r\n" );
    _lwrite( hFile, (LPCSTR)szBuf, lstrlen(szBuf) );
	
    szBuf[0] = '\t';
    szBuf[1] = '\"';
    pchSrc = m_pszShellExecuteParameters;
    pchBuf = szBuf+2;
    StrCpy_double5C( pchBuf, pchSrc );
    lstrcpy( pchBuf, "\\0\"\r\n" );
    _lwrite( hFile, (LPCSTR)szBuf, lstrlen(szBuf) );
	
    wsprintf( szBuf, "\t%dL\t\t// Show Flags\r\n", m_nShellExecuteShow);
    _lwrite( hFile, (LPCSTR)szBuf, lstrlen(szBuf) );
	
    szBuf[0] = '\t';
    szBuf[1] = '\"';
    pchBuf = szBuf+2;
	*pchBuf = m_fShellExecuteWait ? '1' : '0';
    lstrcpy( pchBuf + sizeof('1'), "\\0\"\r\n" );
    _lwrite( hFile, (LPCSTR)szBuf, lstrlen(szBuf) );
	
	szBuf[0] = '\t';
    szBuf[1] = '\"';
    sprintf((szBuf+2),"%I64X",m_cGroup);
	pchBuf=szBuf+lstrlen(szBuf);
    lstrcpy( pchBuf, "\\0\"\r\n" );
    _lwrite( hFile, (LPCSTR)szBuf, lstrlen(szBuf) );
	
	//
    // Finish up
	
    wsprintf( szBuf, "END\r\n\r\n" );
    _lwrite( hFile, (LPCSTR)szBuf, lstrlen(szBuf) );
	
    return TRUE;
}

BOOL CCommand::WriteIniValue(HFILE hFile, int nCmdID)
{
    char szBuf[255];
    LPSTR pchBuf;
    LPSTR pchSrc;
    int nTotalLength;
	
    //
    //Header
    //
    wsprintf( szBuf, "%d COMMANDDATA MOVEABLE\r\nBEGIN\r\n", nCmdID );
    _lwrite( hFile, (LPCSTR)szBuf, lstrlen(szBuf) );
	
	//
    //Section Offset == len(Filename) + 1
    //It is zero if we are supposed to use ini file mapping.
	//
    if( m_fMap )
    {
        nTotalLength = 0;
        wsprintf( szBuf, "\t0\t\t\t// Use ini-file mapping\r\n" );
    }
    else
    {
        nTotalLength = lstrlen(m_pszIniFilename) + 1;
        wsprintf( szBuf, "\t%d\t\t\t// Offset of Section Name\r\n", nTotalLength );
    }
	
    _lwrite( hFile, (LPCSTR)szBuf, lstrlen(szBuf) );
	
	//
    //Entry Offset == len(Filename) + len(Section) + 2
	//
    nTotalLength += lstrlen(m_pszIniSection) + 1;
    wsprintf( szBuf, "\t%d\t\t\t// Offset of Entry\r\n", nTotalLength );
    _lwrite( hFile, (LPCSTR)szBuf, lstrlen(szBuf) );
	
	//
    //Value offset == len(Filename) + len(Section) + len(Entry) + 3
	//
    nTotalLength += lstrlen(m_pszIniEntry) + 1;
    wsprintf( szBuf, "\t%d\t\t\t// Offset of Value\r\n", nTotalLength );
    _lwrite( hFile, (LPCSTR)szBuf, lstrlen(szBuf) );
	
	//
    //Type offset == len(Filename) + len(Section) + len(Entry) + len(Value) + 4
	//
    nTotalLength += lstrlen(m_pszIniValue) + 1;
	
	{  // look for paired double quotes that will turn into single double quotes
		LPCTSTR pBack = m_pszIniValue;
		LPCTSTR pFront = AnsiNext(pBack);
		while( *pFront )
		{
			if( *pBack == '"' && *pFront == '"' )
			{
				nTotalLength--;
				pBack = AnsiNext(pFront);
				pFront = AnsiNext(pBack);
			}
			else
			{
				pBack = pFront;
				pFront = AnsiNext(pBack);
			}
		}
	}
	
    wsprintf( szBuf, "\t%d\t\t\t// Offset of Type\r\n", nTotalLength );
    _lwrite( hFile, (LPCSTR)szBuf, lstrlen(szBuf) );
	
	//
    //Uninstall offset == len(Filename) + len(Section) + len(Entry) + len(Value) + sizeof(DWORD) + 4
	//
    nTotalLength += sizeof(DWORD);
    wsprintf( szBuf, "\t%d\t\t\t// Offset of UninstallFlag\r\n", nTotalLength );
    _lwrite( hFile, (LPCSTR)szBuf, lstrlen(szBuf));
	
	//
    //Group offset == len(Filename) + len(Section) + len(Entry) + len(Value) + sizeof(DWORD) + sizeof('1') + 4
	//
    nTotalLength += sizeof('1');
    wsprintf( szBuf, "\t%d\t\t\t// Offset of Group Flag\r\n", nTotalLength );
    _lwrite( hFile, (LPCSTR)szBuf, lstrlen(szBuf));
	
    //
    //ini Filename, assume no backslashes
    //
    if( !m_fMap )
    {
        wsprintf( szBuf, "\t\"%s\\0\"\r\n", (LPCSTR)m_pszIniFilename );
        _lwrite( hFile, (LPCSTR)szBuf, lstrlen(szBuf) );
    }
	
    //
    //ini Section
    //
    szBuf[0] = '\t';
    szBuf[1] = '\"';
    pchSrc = m_pszIniSection;
    pchBuf = szBuf+2;
    StrCpy_double5C( pchBuf, pchSrc );
    lstrcpy( pchBuf, "\\0\"\r\n" );
    _lwrite( hFile, (LPCSTR)szBuf, lstrlen(szBuf) );
	
    //
    //ini Entry
    //
    szBuf[0] = '\t';
    szBuf[1] = '\"';
    pchSrc = m_pszIniEntry;
    pchBuf = szBuf+2;
    StrCpy_double5C( pchBuf, pchSrc );
    lstrcpy( pchBuf, "\\0\"\r\n" );
    _lwrite( hFile, (LPCSTR)szBuf, lstrlen(szBuf) );
	
    //
    //ini Value
    //
    szBuf[0] = '\t';
    szBuf[1] = '\"';
    pchSrc = m_pszIniValue;
    pchBuf = szBuf+2;
    StrCpy_double5C( pchBuf, pchSrc );
    lstrcpy( pchBuf, "\\0\"\r\n" );
    _lwrite( hFile, (LPCSTR)szBuf, lstrlen(szBuf) );
	
    //
    //ini Type
    //
	DWORD dwType;
	TCHAR tcsType[64];
	
	//
	//Determine and store the type of data being written
	//
	if (0 == lstrcmpi(m_pszIniType, "REG_DWORD"))
	{
		dwType = REG_DWORD;
		lstrcpy(tcsType, "REG_DWORD");
	}
	else if (0 == lstrcmpi(m_pszIniType, "REG_SZ"))
	{
		dwType = REG_SZ;
		lstrcpy(tcsType, "REG_SZ");
	}
	else if (0 == lstrcmpi(m_pszIniType, "REG_EXPAND_SZ"))
	{
		dwType = REG_EXPAND_SZ;
		lstrcpy(tcsType, "REG_EXPAND_SZ");
	}
	else
	{
		//
		//The following are all codes that flag a REG_BINARY type.  The string
		//passed in is simply used to set the size of the binary data being
		//written... The size is written as a negative number to avoid conflict
		//with the standard REG_* types...
		//
		if (0 == lstrcmpi("DWORD", m_pszIniType))
		{
			dwType = (DWORD) -((LONG) sizeof(DWORD));
			lstrcpy(tcsType, "REG_BINARY (DWORD)");
		}
		else if (0 == lstrcmpi("INT", m_pszIniType))
		{
			dwType = (DWORD) -((LONG) sizeof(int));
			lstrcpy(tcsType, "REG_BINARY (INT)");
		}
		else if (0 == lstrcmpi("BYTE", m_pszIniType))
		{
			dwType = (DWORD) -((LONG) sizeof(BYTE));
			lstrcpy(tcsType, "REG_BINARY (BYTE)");
			
		}
		else if (0 == lstrcmpi("STRLEN", m_pszIniType))
		{
			//
			//lstrlen m_pszIniValue is correct because we're setting type
			//equal to the length of the value string
			//
			dwType = (DWORD) -lstrlen(m_pszIniValue);
			lstrcpy(tcsType, "REG_BINARY (STRLEN)");
			
		}
		else
		{
			//
			//Assume that the size was passed in as a hard-coded number...
			//
			dwType = (DWORD) -atol(m_pszIniType);
			lstrcpy(tcsType, "REG_BINARY (USER-SIZED)");
			
		}
	}
	
	//
	//ini type of data...
	//
    wsprintf( szBuf, "\t%luL\t\t\t// %s\r\n", dwType, tcsType );
    _lwrite( hFile, (LPCSTR)szBuf, lstrlen(szBuf) );
	
    //
    //ini UninstallFlag
    //
    szBuf[0] = '\t';
    szBuf[1] = '\"';
    pchBuf = szBuf+2;
	*pchBuf = m_fUninstallFile ? '1' : '0';
	if ( m_fUninstallAll )
		*pchBuf = '2';
    //lstrcpy( pchBuf + sizeof('1'), "\\0\"\r\n" );
    lstrcpy( pchBuf + sizeof('1'), "\"\r\n" );
    _lwrite( hFile, (LPCSTR)szBuf, lstrlen(szBuf) );
	
	//
	//ini Group flags
	//
	szBuf[0] = '\t';
    szBuf[1] = '\"';
    sprintf((szBuf+2),"%I64X",m_cGroup);
	pchBuf=szBuf+lstrlen(szBuf);
    lstrcpy( pchBuf, "\\0\"\r\n" );
    _lwrite( hFile, (LPCSTR)szBuf, lstrlen(szBuf) );
	
	
	//
    //Finish up
	//
    wsprintf( szBuf, "END\r\n\r\n" );
    _lwrite( hFile, (LPCSTR)szBuf, lstrlen(szBuf) );
	
    return TRUE;
}

BOOL CCommand::WriteReadFileList( HFILE hFile, int nCmdID )
{
    char szBuf[255];
    LPSTR pchSrc;
    LPSTR pchBuf;
	
    //
    // Header
    //
	
    wsprintf( szBuf, "%d COMMANDDATA MOVEABLE\r\nBEGIN\r\n", nCmdID );
    _lwrite( hFile, (LPCSTR)szBuf, lstrlen(szBuf) );
	
    //
    // Source Name, must change single backslashes to double
    //
	
    szBuf[0] = '\t';
    szBuf[1] = '\"';
    pchSrc = m_pszDestName;
    pchBuf = szBuf+2;
    StrCpy_double5C( pchBuf, pchSrc );
    lstrcpy( pchBuf, "\\0\"\r\n" );
    _lwrite( hFile, (LPCSTR)szBuf, lstrlen(szBuf) );
	
    // Finish up
	
    wsprintf( szBuf, "END\r\n\r\n" );
    _lwrite( hFile, (LPCSTR)szBuf, lstrlen(szBuf) );
	
    return TRUE;
}
#endif
