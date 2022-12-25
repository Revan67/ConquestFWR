//
// binary.cpp
//
//      PREPSTUB's method of writing the commands as a binary object
//
//
// History:
//
//		5-16-97 - created by craigh
//

#include "prepstub.h"
#include "textdoc.h"
#include "command.h"
#include "verutil.h"
#include "util.h"
#include "resource.h"
#include "stdio.h"
#include <time.h>

extern BOOL	g_fFileError;

extern VOID StrCpy_double5C( LPSTR pszDst, LPCSTR pszSrc );

BOOL CCommand::BinaryWrite( HFILE hFile)
{
    char szBuf[_MAX_PATH*2];

    switch( m_eCommandType )
    {
//        case TOK_INSTALL:
//            *(WORD *)szBuf = SC_INSTALLFILE;
//            break;

        case TOK_INIVALUE:
			*(WORD *)szBuf = SC_ADDINIVALUE;
            break;

        case TOK_INSTFONT:
			*(WORD *)szBuf = SC_INSTALLFONT;
            break;

        case TOK_MKDIR:
			*(WORD *)szBuf = SC_MKDIR;
            break;

        case TOK_MKROOT:
			*(WORD *)szBuf = SC_MKROOT;
            break;

        case TOK_GETNAME:
			*(WORD *)szBuf = SC_GETNAME;
            break;

        case TOK_GETPID:
			*(WORD *)szBuf = SC_GETPID;
            break;

        case TOK_INSTDX:
			*(WORD *)szBuf = SC_INSTDX;
            break;

        case TOK_INSTDPLAY:
			*(WORD *)szBuf = SC_INSTDPLAY;
            break;

        case TOK_INSTICON:
			*(WORD *)szBuf = SC_INSTICON;
            break;

        case TOK_CDSPEED:
			*(WORD *)szBuf = SC_CDSPEED;
            break;

        case TOK_INSTALLLIST:
			*(WORD *)szBuf = SC_INSTALLLIST;
            break;

        case TOK_INSTALLGO:
			*(WORD *)szBuf = SC_INSTALLGO;
            break;

        case TOK_REGWIZ:
			*(WORD *)szBuf = SC_REGWIZ;
            break;

		case TOK_SHELLEXECUTE:
			*(WORD *)szBuf = SC_SHELLEXECUTE;
			break;

		case TOK_DELETEFILE :
			*(WORD *)szBuf = SC_DELETEFILE;
			break;

        case TOK_GETGROUP:
			*(WORD *)szBuf = SC_GETGROUP;
            break;

		case TOK_CABGO:
			*(WORD *)szBuf = SC_CABGO;
            break;

		case TOK_READFILELIST:
			*(WORD *)szBuf = SC_READFILELIST;
            break;

		default:
            ASSERT(FALSE);
            return FALSE;
    }

    _lwrite( hFile, (LPCSTR)szBuf, sizeof(WORD) );

	// Resource ID (index in BLOBS)
	*(WORD *)szBuf=(WORD)GetCommandID()-1;
    _lwrite( hFile, (LPCSTR)szBuf, sizeof(WORD) );

    BOOL fFirst = TRUE;
	*(DWORD *)szBuf = 0;

    //
    // Write OS/Build Type flags
    //

    if( m_fWin95 )
    {
        *(DWORD *)szBuf |= OS_WIN95;
		fFirst = FALSE;
    }
    if( m_fWin98 )
    {
        *(DWORD *)szBuf |= OS_WIN98;
		fFirst = FALSE;
    }
    if( m_fNT40 )
    {
         *(DWORD *)szBuf |= OS_NT40;
		 fFirst = FALSE;
    }
    if( m_fNT50 )
    {
         *(DWORD *)szBuf |= OS_NT50;
		 fFirst = FALSE;
    }

	if (m_fDBCS)
	{
		*(DWORD *) szBuf |= BLD_DBCS;
		fFirst = FALSE;
	}
	if (m_fOEM)
	{
		*(DWORD *) szBuf |= BLD_OEM;
		fFirst = FALSE;
	}
	if (m_fRTL)
	{
		*(DWORD *) szBuf |= BLD_RTL;
		fFirst = FALSE;
	}
	if (m_fJPN)
	{
		*(DWORD *) szBuf |= BLD_JPN;
		fFirst = FALSE;
	}
	if (m_fGER)
	{
		*(DWORD *) szBuf |= BLD_GER;
		fFirst = FALSE;
	}
	if (m_fFRA)
	{
		*(DWORD *) szBuf |= BLD_FRA;
		fFirst = FALSE;
	}
	if (m_fSPA)
	{
		*(DWORD *) szBuf |= BLD_SPA;
		fFirst = FALSE;
	}
	if (m_fUSA)
	{
		*(DWORD *) szBuf |= BLD_USA;
		fFirst = FALSE;
	}
	if (m_fAPP1)
	{
		*(DWORD *) szBuf |= BLD_APP1;
		fFirst = FALSE;
	}
	if (m_fAPP2)
	{
		*(DWORD *) szBuf |= BLD_APP2;
		fFirst = FALSE;
	}
	if (m_fAPP3)
	{
		*(DWORD *) szBuf |= BLD_APP3;
		fFirst = FALSE;
	}
    if( m_fIMEENABLE )
    {
         *(DWORD *)szBuf |= SCF_IME_ENABLE;
		 fFirst = FALSE;
    }
    if( m_fIMEON )
    {
         *(DWORD *)szBuf |= SCF_IME_ON;
		 fFirst = FALSE;
    }
    if( fFirst )
    {
        Alert( g_hwnd, MB_ICONSTOP | MB_OK, STR_MUSTHAVEOSFLAG, GetCommandID() );
        return FALSE;
    }

    _lwrite( hFile, (LPCSTR)szBuf, sizeof(DWORD) );

    //
    // Now write the command-specific data
    //

    switch( m_eCommandType )
    {
        case TOK_INSTALL:
        case TOK_INSTALLLIST:
            return BinaryWriteInstallFile( hFile, GetCommandID() );

        case TOK_INIVALUE:
            return BinaryWriteIniValue( hFile, GetCommandID() );

        case TOK_INSTFONT:
            return BinaryWriteInstallFile(hFile, GetCommandID ());

        case TOK_MKDIR:
            return BinaryWriteMkDir (hFile, GetCommandID ());

        case TOK_MKROOT:
            return BinaryWriteMkRoot (hFile, GetCommandID ());

        case TOK_GETNAME:
            return BinaryWriteGetName (hFile, GetCommandID ());

        case TOK_GETGROUP:
            return BinaryWriteGetGroup (hFile, GetCommandID ());

        case TOK_GETPID:
            return BinaryWriteGetPID (hFile, GetCommandID ());

        case TOK_INSTDX:
            return BinaryWriteInstDX (hFile, GetCommandID ());

        case TOK_INSTDPLAY:
            return BinaryWriteInstDPLAY (hFile, GetCommandID ());

        case TOK_INSTICON:
            return BinaryWriteInstIcon (hFile, GetCommandID ());

        case TOK_CDSPEED:
            return BinaryWriteCDSpeed (hFile, GetCommandID ());

        case TOK_INSTALLGO:
            return BinaryWriteInstallGo (hFile, GetCommandID ());

        case TOK_CABGO:
            return BinaryWriteCabGo (hFile, GetCommandID ());

        case TOK_REGWIZ:
            return BinaryWriteRegWiz (hFile, GetCommandID ());

		case TOK_SHELLEXECUTE:
			return BinaryWriteShellExecute (hFile, GetCommandID ());

        case TOK_DELETEFILE:
            return BinaryWriteDeleteFile (hFile, GetCommandID ());

        case TOK_READFILELIST:
            return BinaryWriteReadFileList (hFile, GetCommandID ());
    }

    return TRUE;
}

BOOL CCommand::BinaryWriteInstallFile( HFILE hFile, int nCmdID )
{
    char szBuf[_MAX_PATH*4];
	DWORD dwOffset=0;

    //
    // Header
    //


    //
    // FileInfo
    //

	// Version data
	*(DWORD *)szBuf = m_FileInfo.dwFileVersionMS;
	dwOffset+=sizeof(DWORD);
	
	*(DWORD *)(szBuf+dwOffset) = m_FileInfo.dwFileVersionLS;
	dwOffset+=sizeof(DWORD);
	
	// Language
	*(DWORD *)(szBuf+dwOffset) = m_FileInfo.dwLanguage;
	dwOffset+=sizeof(DWORD);

	// filesize
	*(DWORD *)(szBuf+dwOffset) = m_FileInfo.dwFileSize;
	dwOffset+=sizeof(DWORD);

	// date/time
	*(time_t *)(szBuf+dwOffset) = m_FileInfo.FileTime;
	dwOffset+=sizeof(time_t);

    //
    // other flags
    //
	*(WORD *)(szBuf+dwOffset) = 0;

    if( m_fWindowsDir )
    {
        *(WORD *)(szBuf+dwOffset) |= IF_WINDOWSDIR;
    }
    else if( m_fSystemDir )
    {
		*(WORD *)(szBuf+dwOffset) |= IF_SYSTEMDIR;
    }
    else
    {
		*(WORD *)(szBuf+dwOffset) |= IF_APPDIR;
    }

    if( m_fSysFile )
    {
		*(WORD *)(szBuf+dwOffset) |=  IF_SYSTEMFILE;
    }
    if( m_fSharedFile )
    {
		*(WORD *)(szBuf+dwOffset) |=  IF_SHAREDFILE;
    }
    if( m_fDLLRegister )
    {
		*(WORD *)(szBuf+dwOffset) |=  IF_DLLREGISTER;
    }
    if( m_fUninstallFile )
    {
		*(WORD *)(szBuf+dwOffset) |=  IF_UNINSTALL;
    }
    if( m_fCab )
    {
		*(WORD *)(szBuf+dwOffset) |=  IF_CAB;
    }
    if( m_fFont )
    {
		*(WORD *)(szBuf+dwOffset) |=  IF_FONTFILE;
    }
    if( m_fUninstOnly )
    {
		*(WORD *)(szBuf+dwOffset) |=  IF_UNINSTONLY;
    }
	dwOffset += sizeof(WORD);


    //
    // Offset to dest name == length of source name + 1
    //

	*(WORD *)(szBuf+dwOffset) = lstrlen(m_pszSourceName) + 1;
	dwOffset += sizeof(WORD);

    //
    // Source Name, must change single backslashes to double
    //

    lstrcpy( (LPSTR)(szBuf+dwOffset), m_pszSourceName );
	dwOffset+= lstrlen(m_pszSourceName) + 1;

    //
    // Dest Name, must change single backslashes to double
    //

    lstrcpy( (LPSTR)(szBuf+dwOffset), m_pszDestName);
	dwOffset += lstrlen(m_pszDestName)+1;

	//
	//Write group with NUL terminator
	//
	dwOffset+=sprintf((szBuf+dwOffset),"%I64X%c",m_cGroup,'\0');

	//
	//Write DiskId with NUL terminator
	//
	dwOffset+=sprintf((szBuf+dwOffset), "%d%c", m_nDiskId, '\0');

	
	// write length of data structure
	_lwrite(hFile,(LPCSTR)&dwOffset,sizeof(DWORD));
	// write data
	_lwrite(hFile,szBuf,dwOffset);



    return TRUE;
}


BOOL CCommand::BinaryWriteMkDir( HFILE hFile, int nCmdID )
{
    char szBuf[_MAX_PATH*2];
	DWORD dwOffset=0;

    //
    // Header
    //
    if( m_fUninstallFile )
    {
        *(WORD *) szBuf = IF_UNINSTALL;
    }
    else if( m_fUninstallAll )
    {
        *(WORD *) szBuf = IF_UNINSTALLALL;
    }
	else
        *(WORD *) szBuf = 0;
	dwOffset+=sizeof(WORD);

    //
    // Source Name, must change single backslashes to double
    //

    lstrcpy( szBuf+dwOffset, m_pszMkDirValue );
	dwOffset += lstrlen(m_pszMkDirValue)+1;

    dwOffset+=sprintf((szBuf+dwOffset),"%I64X",m_cDirGroup);
//	dwOffset--;

	// write length of data
    _lwrite( hFile, (LPCSTR)&dwOffset, sizeof(DWORD));
	// write data
    _lwrite( hFile, (LPCSTR)szBuf, dwOffset);
    // Finish up

    return TRUE;
}
BOOL CCommand::BinaryWriteMkRoot( HFILE hFile, int nCmdID )
{
    char szBuf[_MAX_PATH*2];
	DWORD dwOffset = 0;

    if( m_fUninstallFile )
    {
        *(WORD *) szBuf = IF_UNINSTALL;
    }
	else
        *(WORD *) szBuf = 0;

	dwOffset+=sizeof(WORD);

    // write length of data
    _lwrite( hFile, (LPCSTR)&dwOffset, sizeof(DWORD));
	// write data
    _lwrite( hFile, (LPCSTR)szBuf, dwOffset);

    return TRUE;
}
BOOL CCommand::BinaryWriteGetName( HFILE hFile, int nCmdID )
{
    char szBuf[255];
	DWORD dwOffset = 0;

    //
    // Header
    //
	*(WORD *)szBuf = 0;
	dwOffset += sizeof(WORD);

	// write length of data
    _lwrite( hFile, (LPCSTR)&dwOffset, sizeof(DWORD));
	// write data
    _lwrite( hFile, (LPCSTR)szBuf, dwOffset);

    return TRUE;
}
BOOL CCommand::BinaryWriteGetGroup( HFILE hFile, int nCmdID )
{
    char szBuf[_MAX_PATH*2];
	DWORD dwOffset=0;

	*(WORD *)szBuf = 0;
	dwOffset += sizeof(WORD);

	// write length of data
    _lwrite( hFile, (LPCSTR)&dwOffset, sizeof(DWORD));
	// write data
    _lwrite( hFile, (LPCSTR)szBuf, dwOffset);

    return TRUE;
}
BOOL CCommand::BinaryWriteDeleteFile( HFILE hFile, int nCmdID )
{
    char szBuf[_MAX_PATH];
	DWORD dwOffset = 0;

	szBuf[0] = 0;
	szBuf[1] = 0;

	//
	//If silent delete requested, set silent delete flag
	//
	if ( m_fUninstallFile)
	{
		*(WORD *)szBuf = IF_UNINSTALL;
    }

	//
	//If recurse requested, set recurse flag
	//
	if( m_fUninstallAll )
    {
        *(WORD *) szBuf |= IF_UNINSTALLALL;
    }

	//
	//If not during uninstall requested, set appropriate flag
	//
	if( m_fNotUninstall )
    {
        *(WORD *) szBuf |= IF_NOTUNINSTALL;
    }
	//
	//If during install requested, set appropriate flag
	//
	if( m_fInstall )
    {
        *(WORD *) szBuf |= IF_INSTALL;
    }
	dwOffset += sizeof(WORD);

	//
    // Source Name, must change single backslashes to double
    //

    lstrcpy(szBuf+dwOffset, m_pszDeleteFileValue);
	dwOffset += lstrlen(m_pszDeleteFileValue)+1;

	// write length of data
    _lwrite( hFile, (LPCSTR)&dwOffset, sizeof(DWORD));
	// write data
    _lwrite( hFile, (LPCSTR)szBuf, dwOffset);

    return TRUE;
}
BOOL CCommand::BinaryWriteGetPID( HFILE hFile, int nCmdID )
{
    char szBuf[_MAX_PATH * 2];
	DWORD dwOffset=0;

	*(WORD *)szBuf = 0;
	dwOffset += sizeof(WORD);

	// write length of data
    _lwrite( hFile, (LPCSTR)&dwOffset, sizeof(DWORD));
	// write data
	//BUGBUG:REVIEW:Is this whole function correct now?? reizen
    _lwrite( hFile, (LPCSTR)szBuf, dwOffset);

    return TRUE;
}
BOOL CCommand::BinaryWriteInstDX( HFILE hFile, int nCmdID )
{
    char szBuf[_MAX_PATH*3];
	DWORD dwOffset = 0;

	*(WORD *)szBuf = 0;
	dwOffset += sizeof(WORD);

    lstrcpy(szBuf+dwOffset, m_pszInstDXValue);
	dwOffset+=lstrlen(m_pszInstDXValue)+1;

    lstrcpy(szBuf+dwOffset, m_pszInstDXNameValue);
	dwOffset+=lstrlen(m_pszInstDXNameValue)+1;

	lstrcpy(szBuf+dwOffset, m_pszInstDXMinVersion);
	dwOffset+=lstrlen(m_pszInstDXMinVersion)+1;

	if (0L == atol(m_pszInstDXFlagsValue))
	{
		//
		//Set DXFlags as default (DSETUP_DIRECTX) DWORD
		//
		*(DWORD *)(szBuf+dwOffset) = (DWORD) DSETUP_DIRECTX;
	}
	else
	{
		//
		//Convert DX Flags to DWORD
		//
		*(DWORD *)(szBuf+dwOffset) = (DWORD) atol(m_pszInstDXFlagsValue);
	}
	
	dwOffset+=sizeof(DWORD);

	// write length of data
    _lwrite( hFile, (LPCSTR)&dwOffset, sizeof(DWORD));
	// write data
    _lwrite( hFile, (LPCSTR)szBuf, dwOffset);


    return TRUE;
}
BOOL CCommand::BinaryWriteInstDPLAY( HFILE hFile, int nCmdID )
{
    char szBuf[_MAX_PATH*3];
	DWORD dwOffset = 0;

	*(WORD *)szBuf = 0;
	dwOffset += sizeof(WORD);

    lstrcpy(szBuf+dwOffset, m_pszInstDPLAYNameValue);
	dwOffset+=lstrlen(m_pszInstDPLAYNameValue)+1;

	lstrcpy(szBuf+dwOffset, m_pszInstDPLAYMinVersion);
	dwOffset+=lstrlen(m_pszInstDPLAYMinVersion)+1;

	// write length of data
    _lwrite( hFile, (LPCSTR)&dwOffset, sizeof(DWORD));
	// write data
    _lwrite( hFile, (LPCSTR)szBuf, dwOffset);


    return TRUE;
}
BOOL CCommand::BinaryWriteInstIcon( HFILE hFile, int nCmdID )
{
    char szBuf[_MAX_PATH*2];
	DWORD dwOffset=0;


	// Write The uninstallLink flag
	if ( m_fUninstallLink )
	{
		*(WORD *)szBuf = IF_UNINSTALLLINK;
    }
	else
	{
		*(WORD *)szBuf = 0;
		dwOffset += sizeof(WORD);
	}

    lstrcpy(szBuf+dwOffset, m_pszInstIconNameValue);
	dwOffset+=lstrlen(m_pszInstIconNameValue)+1;

	lstrcpy(szBuf+dwOffset, m_pszInstIconNameIconValue);
	dwOffset+=lstrlen(m_pszInstIconNameIconValue)+1;

    lstrcpy(szBuf+dwOffset, m_pszInstIconDescriptionValue);
	dwOffset+=lstrlen(m_pszInstIconDescriptionValue)+1;

    lstrcpy(szBuf+dwOffset, m_pszInstIconDestinationValue);
	dwOffset+=lstrlen(m_pszInstIconDestinationValue)+1;

	*(WORD *)(szBuf+dwOffset) = (WORD)atoi(m_pszInstIconIndexValue);
	dwOffset+= sizeof(WORD);

	dwOffset+=sprintf((szBuf+dwOffset),"%I64X%c",m_cGroup,'\0');

	// write length of data
    _lwrite( hFile, (LPCSTR)&dwOffset, sizeof(DWORD));
	// write data
    _lwrite( hFile, (LPCSTR)szBuf, dwOffset);

    return TRUE;
}
BOOL CCommand::BinaryWriteCDSpeed( HFILE hFile, int nCmdID )
{
    char szBuf[_MAX_PATH*2];
	DWORD dwOffset=0;

	*(WORD *)szBuf = 0;
	dwOffset += sizeof(WORD);

	*(WORD *)(szBuf+dwOffset) = (WORD)atoi(m_pszCDSpeedMinCDValue);
	dwOffset+=sizeof(WORD);

    *(WORD *)(szBuf+dwOffset) = (WORD)atoi(m_pszCDSpeedMaxCPUValue);
	dwOffset+=sizeof(WORD);

    lstrcpy(szBuf+dwOffset, m_pszCDSpeedFileNameValue);
	dwOffset+=lstrlen(m_pszCDSpeedFileNameValue)+1;

	// write length of data
    _lwrite( hFile, (LPCSTR)&dwOffset, sizeof(DWORD));
	// write data
    _lwrite( hFile, (LPCSTR)szBuf, dwOffset);

    return TRUE;
}


BOOL CCommand::BinaryWriteInstallGo( HFILE hFile, int nCmdID )
{
    char szBuf[_MAX_PATH*2];
	DWORD dwOffset=0;

// Bail out if any file access problems ocurred during installlist scan
// Note: We leave a crappy output file behind on exit
	if ( g_fFileError )
	{
		g_fFileError = FALSE;
		return FALSE;
	}

	// write length of data
    _lwrite( hFile, (LPCSTR)&dwOffset, sizeof(DWORD));
	// write data
    _lwrite( hFile, (LPCSTR)szBuf, dwOffset);

    return TRUE;
}

BOOL CCommand::BinaryWriteCabGo( HFILE hFile, int nCmdID )
{
    char szBuf[_MAX_PATH*2];
	DWORD dwOffset=0;

    lstrcpy(szBuf+dwOffset, m_pszCabName);
	dwOffset+=lstrlen(m_pszCabName)+1;

	// write length of data
    _lwrite( hFile, (LPCSTR)&dwOffset, sizeof(DWORD));
	// write data
    _lwrite( hFile, (LPCSTR)szBuf, dwOffset);

    return TRUE;
}
BOOL CCommand::BinaryWriteRegWiz( HFILE hFile, int nCmdID )
{
    char szBuf[_MAX_PATH*2];
	DWORD dwOffset=0;

    lstrcpy(szBuf+dwOffset, m_pszRegWizRegName);
	dwOffset+=lstrlen(m_pszRegWizRegName)+1;

	// write length of data
    _lwrite( hFile, (LPCSTR)&dwOffset, sizeof(DWORD));
	// write data
    _lwrite( hFile, (LPCSTR)szBuf, dwOffset);


    return TRUE;
}

BOOL CCommand::BinaryWriteShellExecute( HFILE hFile, int nCmdID )
{
    char szBuf[_MAX_PATH * 2];
	DWORD dwOffset=0;

    *(WORD *) szBuf = m_fUninstallFile ? IF_UNINSTALL : 0;
	dwOffset+=sizeof(WORD);

    lstrcpy(szBuf+dwOffset, m_pszShellExecuteFileName);
	dwOffset+=lstrlen(m_pszShellExecuteFileName) + 1;

	lstrcpy(szBuf+dwOffset, m_pszShellExecuteDirectory);
	dwOffset+=lstrlen(m_pszShellExecuteDirectory) + 1;

	lstrcpy(szBuf+dwOffset, m_pszShellExecuteParameters);
	dwOffset+=lstrlen(m_pszShellExecuteParameters) + 1;

	*(int *)(szBuf+dwOffset) = m_nShellExecuteShow;
	dwOffset += sizeof(int);

	dwOffset+=sprintf((szBuf+dwOffset),"%c%c", (m_fShellExecuteWait ? '1' : '0'),'\0');
	//*(szBuf+dwOffset) = m_fShellExecuteWait ? '1' : '0';
	//dwOffset += sizeof('1');

    dwOffset+=sprintf((szBuf+dwOffset),"%I64X%c",m_cGroup, '\0');

	// write length of data
    _lwrite( hFile, (LPCSTR)&dwOffset, sizeof(DWORD));
	// write data
    _lwrite( hFile, (LPCSTR)szBuf, dwOffset);


    return TRUE;
}

BOOL CCommand::BinaryWriteIniValue( HFILE hFile, int nCmdID )
{
    char szBuf[_MAX_PATH*2];
	DWORD dwOffset=0;
	DWORD dwType;
    int nTotalLength;

    // Section Offset == len(Filename) + 1
    //   It is zero if we are supposed to use ini file mapping.
    if( m_fMap )
    {
        *(WORD *)(szBuf+dwOffset) = 0;
		dwOffset += sizeof(WORD);
		nTotalLength = 0;
    }
    else
    {
		nTotalLength = lstrlen(m_pszIniFilename)+1;
        *(WORD *)(szBuf+dwOffset) = (WORD)nTotalLength;
		dwOffset += sizeof(WORD);
    }
    // Entry Offset == len(Filename) + len(Section) + 2
    nTotalLength += lstrlen(m_pszIniSection) + 1;
	*(WORD *)(szBuf+dwOffset) = (WORD)nTotalLength;
	dwOffset += sizeof(WORD);

    // Value offset == len(Filename) + len(Section) + len(Entry) + 3
    nTotalLength += lstrlen(m_pszIniEntry) + 1;
	*(WORD *)(szBuf+dwOffset) = (WORD)nTotalLength;
	dwOffset += sizeof(WORD);

    // Offset to type
	nTotalLength += lstrlen(m_pszIniValue) + 1;
	*(WORD *)(szBuf+dwOffset) = (WORD)nTotalLength;
	dwOffset += sizeof(WORD);
	
	// Uninstall offset
	nTotalLength += sizeof(DWORD); //type is always a DWORD value
	*(WORD *)(szBuf+dwOffset) = (WORD)nTotalLength;
	dwOffset += sizeof(WORD);
	
	// Group offset
	nTotalLength += sizeof('1');  //Uninstall flag is a '1' or a '0'
	*(WORD *)(szBuf+dwOffset) = (WORD)nTotalLength;
	dwOffset += sizeof(WORD);
	
    //
    // ini Filename, assume no backslashes
    //
    if( !m_fMap )
    {
        lstrcpy(szBuf+dwOffset,m_pszIniFilename );
		dwOffset+=lstrlen(m_pszIniFilename)+1;
    }

    //
    // ini Section
    //
	lstrcpy(szBuf+dwOffset,m_pszIniSection );
	dwOffset+=lstrlen(m_pszIniSection)+1;

	//
    // ini Entry
    //
	lstrcpy(szBuf+dwOffset,m_pszIniEntry );
	dwOffset+=lstrlen(m_pszIniEntry)+1;

    //
    // ini Value
    //
	lstrcpy(szBuf+dwOffset,m_pszIniValue );
	dwOffset+=lstrlen(m_pszIniValue)+1;

	//
	//Determine and store the type of data being written
	//
	if (0 == lstrcmpi(m_pszIniType, "REG_DWORD"))
	{
		dwType = REG_DWORD;
	}
	else if (0 == lstrcmpi(m_pszIniType, "REG_SZ"))
	{
		dwType = REG_SZ;
	}
	else if (0 == lstrcmpi(m_pszIniType, "REG_EXPAND_SZ"))
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
		if (0 == lstrcmpi("DWORD", m_pszIniType))
		{
			dwType = (DWORD) -((LONG) sizeof(DWORD));
		}
		else if (0 == lstrcmpi("INT", m_pszIniType))
		{
			dwType = (DWORD) -((LONG) sizeof(int));
		}
		else if (0 == lstrcmpi("BYTE", m_pszIniType))
		{
			dwType = (DWORD) -((LONG) sizeof(BYTE));
		}
		else if (0 == lstrcmpi("STRLEN", m_pszIniType))
		{
			//
			//lstrlen m_pszIniValue is correct because we're setting type
			//equal to the length of the value string
			//
			dwType = (DWORD) -lstrlen(m_pszIniValue);
		}
		else
		{
			//
			//Assume that the size was passed in as a hard-coded number...
			//
			dwType = (DWORD) -atol(m_pszIniType);
		}
	}

	//
	//Key type
	//
	*(DWORD *) (szBuf+dwOffset) = dwType;
	dwOffset += sizeof(DWORD);

	//
	//Uninstall flag...
	//
	*(szBuf+dwOffset) = m_fUninstallFile ? '1' : '0';
	if ( m_fUninstallAll )
		*(szBuf+dwOffset) = '2';
	dwOffset += sizeof('1');

	//
	//Write group with NUL terminator
	//
	dwOffset+=sprintf((szBuf+dwOffset),"%I64X%c",m_cGroup,'\0');

	// write length of data
    _lwrite( hFile, (LPCSTR)&dwOffset, sizeof(DWORD));
	// write data
    _lwrite( hFile, (LPCSTR)szBuf, dwOffset);

    return TRUE;
}

BOOL CCommand::BinaryWriteReadFileList( HFILE hFile, int nCmdID )
{
    char szBuf[_MAX_PATH * 2];
	DWORD dwOffset=0;

    lstrcpy(szBuf+dwOffset, m_pszDestName);
	dwOffset+=lstrlen(m_pszDestName) + 1;

	// write length of data
    _lwrite( hFile, (LPCSTR)&dwOffset, sizeof(DWORD));
	// write data
    _lwrite( hFile, (LPCSTR)szBuf, dwOffset);


    return TRUE;
}

