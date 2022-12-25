#include "prepstub.h"
#include "textdoc.h"
#include "command.h"
#include "verutil.h"
#include "util.h"
#include "resource.h"
#include "stdio.h"
#include "diskinfo.h"
#include <time.h>

extern KEYWORD Keywords[];
extern DISKKEYWORD g_DiskKeywords[MAX_DISK_LABELS + 1];
// 
// This file contains Prepstub98 only functions.
//

BOOL CCommand::TextWrite( HFILE hFile)
{
    char szBuf[_MAX_PATH*2] = "";
	
	//
	// Write the token text
	//

	if (m_eCommandType == TOK_NEWLINE)
	{
		lstrcpy (szBuf, "\r\n");
	    _lwrite( hFile, (LPCSTR)szBuf, lstrlen (szBuf));
		return TRUE;
	}

	if (Keywords[m_eCommandType].uFlags == TF_BRACKETED)
	{
		wsprintf (szBuf, "[%s]\t", Keywords[m_eCommandType].pszKeyword);
	}
	else
	{
		if (m_eCommandType != TOK_COMMENT)
		{
			wsprintf (szBuf, "%s\t", Keywords[m_eCommandType].pszKeyword);
		}
		else
		{
			wsprintf (szBuf, "%s", Keywords[m_eCommandType].pszKeyword);
		}
	}

    _lwrite( hFile, (LPCSTR)szBuf, lstrlen (szBuf));

	//
    // Write Command Specific Data
    //

    switch( m_eCommandType )
    {
        //case TOK_INSTALL:
		case TOK_INSTFONT:		
        case TOK_INSTALLLIST:
            TextWriteInstallFile (hFile);
			break;

        case TOK_GETNAME:
        case TOK_GETGROUP:
        case TOK_GETPID:
		case TOK_INSTALLGO:
		case TOK_BEGINFILELIST:
		case TOK_ENDFILELIST:
		case TOK_BEGINSTRINGLIST:
		case TOK_ENDSTRINGLIST:
		case TOK_BEGINSTATICSTRINGLIST:
		case TOK_ENDSTATICSTRINGLIST:
			// these have no command specific params.
			break;

		case TOK_ACTION:
			TextWriteAction (hFile);
			break;

		case TOK_READFILELIST:
			TextWriteReadFile (hFile);
			break;

        case TOK_MKDIR:
            TextWriteMkDir (hFile);
			break;

		case TOK_COMMENT:
			TextWriteComment (hFile);
			return TRUE;
			break;

		case TOK_RULE:
			TextWriteRule (hFile);
			break;

        case TOK_INIVALUE:
            TextWriteIniValue (hFile);
			break;

		case TOK_SHELLEXECUTE:
			TextWriteShellExecute (hFile);
			break;

		case TOK_REGWIZ:
            TextWriteRegWiz (hFile);
			break;

		case TOK_CABGO:
			TextWriteCabGo (hFile);
			break;

        case TOK_CDSPEED:
            TextWriteCDSpeed (hFile);
			break;

        case TOK_INSTICON:
            TextWriteInstIcon (hFile);
			break;

        case TOK_INSTDX:
            TextWriteInstDX (hFile);
			break;

		case TOK_INSTDPLAY:
			TextWriteInstDPlay (hFile);
			break;

        case TOK_DELETEFILE:
            TextWriteDeleteFile (hFile);
			break;

        case TOK_MKROOT:
            TextWriteMkRoot (hFile);
			break;

		case TOK_PROPERTY:
			TextWriteProperty (hFile);
			break;

		case TOK_STRINGVAR:
			TextWriteString (hFile);
			break;

		default:
			Alert( g_hwnd, MB_ICONSTOP | MB_OK, "The token '%s' is not implemented in CCommand::TextWrite().\r\nToken information can not be saved.", Keywords[m_eCommandType].pszKeyword);
			lstrcpy (szBuf, "<!!! CCommand::TextWrite() - Token Not Implemented !!!>\r\n");
			_lwrite( hFile, (LPCSTR)szBuf, lstrlen (szBuf));
			return true;
			break;
    }

    //
    // Write OS/Build Type flags
    //

	lstrcpy (szBuf, "\t");

    if (m_fWin95 && m_fWin98)
	{
		lstrcat (szBuf, "ALLWIN\t");
	}
	else
	{
		if( m_fWin95 )
		{
			lstrcat (szBuf, Keywords[TOK_WIN95].pszKeyword);
			lstrcat (szBuf, "\t");
	    }

		if( m_fWin98 )
		{
			lstrcat (szBuf, Keywords[TOK_WIN98].pszKeyword);
			lstrcat (szBuf, "\t");
		}
	}

	if (m_fNT40 && m_fNT50)
	{
		lstrcat (szBuf, "ALLNT\t");
	}
	else
	{
		if( m_fNT40 )
		{
			lstrcat (szBuf, Keywords[TOK_NT40].pszKeyword);
			lstrcat (szBuf, "\t");
		}

		if( m_fNT50 )
		{
			lstrcat (szBuf, Keywords[TOK_NT50].pszKeyword);
			lstrcat (szBuf, "\t");
		}
	}

	if (m_fDBCS)
	{
		lstrcat (szBuf, Keywords[TOK_DBCS].pszKeyword);
		lstrcat (szBuf, "\t");
	}

	if (m_fOEM)
	{
		lstrcat (szBuf, Keywords[TOK_OEM].pszKeyword);
		lstrcat (szBuf, "\t");
	}

	if (m_fRTL)
	{
		lstrcat (szBuf, Keywords[TOK_RTL].pszKeyword);
		lstrcat (szBuf, "\t");
	}

	if (m_fJPN)
	{
		lstrcat (szBuf, Keywords[TOK_JPN].pszKeyword);
		lstrcat (szBuf, "\t");
	}

	if (m_fGER)
	{
		lstrcat (szBuf, Keywords[TOK_GER].pszKeyword);
		lstrcat (szBuf, "\t");
	}

	if (m_fFRA)
	{
		lstrcat (szBuf, Keywords[TOK_FRA].pszKeyword);
		lstrcat (szBuf, "\t");
	}

	if (m_fSPA)
	{
		lstrcat (szBuf, Keywords[TOK_SPA].pszKeyword);
		lstrcat (szBuf, "\t");
	}

	if (m_fUSA)
	{
		lstrcat (szBuf, Keywords[TOK_USA].pszKeyword);
		lstrcat (szBuf, "\t");
	}

	if (m_fAPP1)
	{
		lstrcat (szBuf, Keywords[TOK_APP1].pszKeyword);
		lstrcat (szBuf, "\t");
	}

	if (m_fAPP2)
	{
		lstrcat (szBuf, Keywords[TOK_APP2].pszKeyword);
		lstrcat (szBuf, "\t");
	}

	if (m_fAPP3)
	{
		lstrcat (szBuf, Keywords[TOK_APP3].pszKeyword);
		lstrcat (szBuf, "\t");
	}

    if( m_fIMEENABLE )
    {
		lstrcat (szBuf, Keywords[TOK_IMEENABLE].pszKeyword);
		lstrcat (szBuf, "\t");
    }

    if( m_fIMEON )
    {
		lstrcat (szBuf, Keywords[TOK_IMEON].pszKeyword);
		lstrcat (szBuf, "\t");
    }

	if( m_fCabPreCopy )
    {
		lstrcat (szBuf, Keywords[TOK_PRECOPY].pszKeyword);
		lstrcat (szBuf, "\t");
    }

    _lwrite( hFile, (LPCSTR)szBuf, lstrlen (szBuf));

    return TRUE;
}

BOOL CCommand::TextWriteInstallFile( HFILE hFile)
{
    char szBuf[_MAX_PATH*4];

    // other flags

	lstrcpy (szBuf, "");

    if( m_fWindowsDir ) 
    {
		lstrcat (szBuf, Keywords[TOK_WINDOWS].pszKeyword);
		lstrcat (szBuf, "\t");
    }
    else if (m_fSystemDir)
    {
		lstrcat (szBuf, Keywords[TOK_SYSTEM].pszKeyword);
		lstrcat (szBuf, "\t");
    }
    else if (m_fAppDir)
    {
		lstrcat (szBuf, Keywords[TOK_APP].pszKeyword);
		lstrcat (szBuf, "\t");
    }

    if( m_fSysFile ) 
    {
		lstrcat (szBuf, Keywords[TOK_CHECKVER].pszKeyword);
		lstrcat (szBuf, "\t");
    }

    if( m_fSharedFile ) 
    {
		lstrcat (szBuf, Keywords[TOK_SHARED].pszKeyword);
		lstrcat (szBuf, "\t");
    }

    if( m_fDLLRegister )
    {
		lstrcat (szBuf, Keywords[TOK_DLLREGISTER].pszKeyword);
		lstrcat (szBuf, "\t");
	}

    if( m_fUninstallFile )
    {
		lstrcat (szBuf, Keywords[TOK_UNINSTALL].pszKeyword);
		lstrcat (szBuf, "\t");
    }

    if( m_fCab ) 
    {
		lstrcat (szBuf, Keywords[TOK_CAB].pszKeyword);
		lstrcat (szBuf, "\t");
    }

	if( m_fFont )
	{
		lstrcat (szBuf, Keywords[TOK_FONT].pszKeyword);
		lstrcat (szBuf, "\t");
    }

	if( m_fUninstOnly )
	{
		lstrcat (szBuf, Keywords[TOK_UNINSTALLONLY].pszKeyword);
		lstrcat (szBuf, "\t");
	}

	_lwrite( hFile, (LPCSTR)szBuf, lstrlen(szBuf) );

    // Dest Name, tripple quoted

	wsprintf (szBuf, "\"\"\"%s\"\"\"\t\t\t\t", m_pszDestName);
	_lwrite( hFile, (LPCSTR)szBuf, lstrlen(szBuf) );

    // Source Name, tripple quoted

	wsprintf (szBuf, "\"\"\"%s\"\"\"\t\t\t\t", m_pszSourceName);
	_lwrite( hFile, (LPCSTR)szBuf, lstrlen(szBuf) );


	// DiskID, only if explictly specified
	if (m_eCommandType == TOK_INSTALLLIST)
	{
		if (m_nDiskId != DISK_NOT_SPECIFIED)
		{
			wsprintf (szBuf, "%s\t\t", g_DiskKeywords[m_nDiskId].pszKeyword);
			_lwrite( hFile, (LPCSTR)szBuf, lstrlen(szBuf) );
		}
	}

	// group
	if (m_eCommandType != TOK_INSTFONT)
	{
		sprintf (szBuf, "0x%I64X\t", m_cGroup);
		_lwrite( hFile, (LPCSTR)szBuf, lstrlen(szBuf) );
	}

    return TRUE;
}


BOOL CCommand::TextWriteMkDir( HFILE hFile )
{
    char szBuf[_MAX_PATH*2];

	lstrcpy (szBuf, "");

    if( m_fUninstallFile )
    {
		lstrcat (szBuf, Keywords[TOK_UNINSTALL].pszKeyword);
		lstrcat (szBuf, "\t");
    }
    if( m_fUninstallAll )
    {
		lstrcat (szBuf, Keywords[TOK_UNINSTALLALL].pszKeyword);
		lstrcat (szBuf, "\t");
    }

	if (szBuf[0])
	{
		_lwrite( hFile, (LPCSTR)szBuf, lstrlen(szBuf) );
	}

	wsprintf (szBuf, "\"\"\"%s\"\"\"\t", m_pszMkDirValue );
	_lwrite( hFile, (LPCSTR)szBuf, lstrlen(szBuf) );

	// group
	sprintf (szBuf, "0x%I64X\t", m_cDirGroup);
	_lwrite( hFile, (LPCSTR)szBuf, lstrlen(szBuf) );
   
    return TRUE;
}

BOOL CCommand::TextWriteComment( HFILE hFile )
{
    char szBuf[_MAX_PATH*2];

	lstrcpy (szBuf, "");

	wsprintf (szBuf, "%s", m_pszComment);
	_lwrite( hFile, (LPCSTR)szBuf, lstrlen(szBuf) );
  
    return TRUE;
}

BOOL CCommand::TextWriteProperty( HFILE hFile )
{
    char szBuf[_MAX_PATH*2];

	wsprintf (szBuf, "\"\"\"%s\"\"\"\t", m_pszProperty);
	_lwrite( hFile, (LPCSTR)szBuf, lstrlen(szBuf) );

	wsprintf (szBuf, "\"\"\"%s\"\"\"\t", m_pszPropertyValue);
	_lwrite( hFile, (LPCSTR)szBuf, lstrlen(szBuf) );
  
    return TRUE;
}

BOOL CCommand::TextWriteRule ( HFILE hFile )
{
    char szBuf[_MAX_PATH*2];

	lstrcpy (szBuf, "");

	// action
	wsprintf (szBuf, "\"\"\"%s\"\"\"\t", m_pszRuleAction);
	_lwrite( hFile, (LPCSTR)szBuf, lstrlen(szBuf) );

	// pattern
	wsprintf (szBuf, "\"\"\"%s\"\"\"\t", m_pszRulePattern);
	_lwrite( hFile, (LPCSTR)szBuf, lstrlen(szBuf) );

	// group
	sprintf (szBuf, "0x%I64X\t", m_cGroup);
	_lwrite( hFile, (LPCSTR)szBuf, lstrlen(szBuf) );

	// write optional flags

	lstrcpy (szBuf, "");

    if( m_fWindowsDir ) 
    {
		lstrcat (szBuf, Keywords[TOK_WINDOWS].pszKeyword);
		lstrcat (szBuf, "\t");
    }

    if (m_fSystemDir)
    {
		lstrcat (szBuf, Keywords[TOK_SYSTEM].pszKeyword);
		lstrcat (szBuf, "\t");
    }

    if (m_fAppDir)
    {
		lstrcat (szBuf, Keywords[TOK_APP].pszKeyword);
		lstrcat (szBuf, "\t");
    }

    if( m_fSysFile ) 
    {
		lstrcat (szBuf, Keywords[TOK_CHECKVER].pszKeyword);
		lstrcat (szBuf, "\t");
    }

    if( m_fSharedFile ) 
    {
		lstrcat (szBuf, Keywords[TOK_SHARED].pszKeyword);
		lstrcat (szBuf, "\t");
    }

    if( m_fDLLRegister )
    {
		lstrcat (szBuf, Keywords[TOK_DLLREGISTER].pszKeyword);
		lstrcat (szBuf, "\t");
	}

    if( m_fUninstallFile )
    {
		lstrcat (szBuf, Keywords[TOK_UNINSTALL].pszKeyword);
		lstrcat (szBuf, "\t");
    }

	if( m_fUninstallAll )
    {
		lstrcat (szBuf, Keywords[TOK_UNINSTALLALL].pszKeyword);
		lstrcat (szBuf, "\t");
    }

    if( m_fCab ) 
    {
		lstrcat (szBuf, Keywords[TOK_CAB].pszKeyword);
		lstrcat (szBuf, "\t");
    }

	_lwrite( hFile, (LPCSTR)szBuf, lstrlen(szBuf) );


    return TRUE;
}

BOOL CCommand::TextWriteMkRoot( HFILE hFile)
{
	char szBuf[_MAX_PATH*2];

    if( m_fUninstallFile )
    {
		lstrcpy (szBuf, Keywords[TOK_UNINSTALL].pszKeyword);
		lstrcat (szBuf, "\t");
		_lwrite( hFile, (LPCSTR)szBuf, lstrlen(szBuf) );
    }
    return TRUE;
}

BOOL CCommand::TextWriteDeleteFile( HFILE hFile)
{
    char szBuf[_MAX_PATH];

	if (m_fUninstallFile)
	{
		lstrcpy (szBuf, "silent\t");
		_lwrite( hFile, (LPCSTR)szBuf, lstrlen(szBuf) );
	}

	// source name
	wsprintf (szBuf, "\"\"\"%s\"\"\"\t", m_pszDeleteFileValue);
	_lwrite( hFile, (LPCSTR)szBuf, lstrlen(szBuf) );

    return TRUE;
}

BOOL CCommand::TextWriteInstDX( HFILE hFile)
{
    char szBuf[_MAX_PATH*3];

	wsprintf (szBuf, "\"\"\"%s\"\"\"\t", m_pszInstDXValue);
	_lwrite( hFile, (LPCSTR)szBuf, lstrlen(szBuf) );

	wsprintf (szBuf, "\"\"\"%s\"\"\"\t", m_pszInstDXNameValue);
	_lwrite( hFile, (LPCSTR)szBuf, lstrlen(szBuf) );

	wsprintf (szBuf, "\"\"\"%s\"\"\"\t", m_pszInstDXMinVersion);
	_lwrite( hFile, (LPCSTR)szBuf, lstrlen(szBuf) );

	wsprintf (szBuf, "\"\"\"%s\"\"\"\t", m_pszInstDXFlagsValue);
	_lwrite( hFile, (LPCSTR)szBuf, lstrlen(szBuf) );

    return TRUE;
}


BOOL CCommand::TextWriteInstDPlay( HFILE hFile)
{
    char szBuf[_MAX_PATH*3];

	wsprintf (szBuf, "\"\"\"%s\"\"\"\t", m_pszInstDPLAYNameValue);
	_lwrite( hFile, (LPCSTR)szBuf, lstrlen(szBuf) );

	wsprintf (szBuf, "\"\"\"%s\"\"\"\t", m_pszInstDPLAYMinVersion);
	_lwrite( hFile, (LPCSTR)szBuf, lstrlen(szBuf) );

    return TRUE;
}


BOOL CCommand::TextWriteInstIcon( HFILE hFile)
{
    char szBuf[_MAX_PATH*2];

	wsprintf (szBuf, "\"\"\"%s\"\"\"\t", m_pszInstIconNameValue);
	_lwrite( hFile, (LPCSTR)szBuf, lstrlen(szBuf) );

	wsprintf (szBuf, "\"\"\"%s\"\"\"\t", m_pszInstIconNameIconValue);
	_lwrite( hFile, (LPCSTR)szBuf, lstrlen(szBuf) );

	wsprintf (szBuf, "\"\"\"%s\"\"\"\t", m_pszInstIconDescriptionValue);
	_lwrite( hFile, (LPCSTR)szBuf, lstrlen(szBuf) );

	wsprintf (szBuf, "\"\"\"%s\"\"\"\t", m_pszInstIconDestinationValue);
	_lwrite( hFile, (LPCSTR)szBuf, lstrlen(szBuf) );

	wsprintf (szBuf, "\"\"\"%s\"\"\"\t", m_pszInstIconIndexValue);
	_lwrite( hFile, (LPCSTR)szBuf, lstrlen(szBuf) );

	sprintf (szBuf, "0x%I64X ", m_cGroup);
	_lwrite( hFile, (LPCSTR)szBuf, lstrlen(szBuf) );

    return TRUE;
}

BOOL CCommand::TextWriteCDSpeed( HFILE hFile)
{
    char szBuf[_MAX_PATH*2];

	wsprintf (szBuf, "\"\"\"%s\"\"\"\t", m_pszCDSpeedMinCDValue);
	_lwrite( hFile, (LPCSTR)szBuf, lstrlen(szBuf) );

	wsprintf (szBuf, "\"\"\"%s\"\"\"\t", m_pszCDSpeedMaxCPUValue);
	_lwrite( hFile, (LPCSTR)szBuf, lstrlen(szBuf) );

	wsprintf (szBuf, "\"\"\"%s\"\"\"\t", m_pszCDSpeedFileNameValue);
	_lwrite( hFile, (LPCSTR)szBuf, lstrlen(szBuf) );

    return TRUE;
}

BOOL CCommand::TextWriteCabGo( HFILE hFile)
{
    char szBuf[_MAX_PATH*2];

	wsprintf (szBuf, "\"\"\"%s\"\"\"\t", m_pszCabName);
	_lwrite( hFile, (LPCSTR)szBuf, lstrlen(szBuf) );

    return TRUE;
}

BOOL CCommand::TextWriteReadFile ( HFILE hFile)
{
    char szBuf[_MAX_PATH*2];

	wsprintf (szBuf, "\"\"\"%s\"\"\"\t", m_pszDestName);
	_lwrite( hFile, (LPCSTR)szBuf, lstrlen(szBuf) );

    return TRUE;
}

BOOL CCommand::TextWriteRegWiz( HFILE hFile)
{
    char szBuf[_MAX_PATH*2];

	wsprintf (szBuf, "\"\"\"%s\"\"\"\t", m_pszRegWizRegName);
	_lwrite( hFile, (LPCSTR)szBuf, lstrlen(szBuf) );

    return TRUE;
}

BOOL CCommand::TextWriteShellExecute( HFILE hFile)
{
    char szBuf[_MAX_PATH*2] = "";

    if( m_fUninstallFile )
    {
		lstrcat (szBuf, Keywords[TOK_UNINSTALL].pszKeyword);
		lstrcat (szBuf, "\t");
    }

	if (m_fShellExecuteWait)
	{
		lstrcat (szBuf, Keywords[TOK_WAIT].pszKeyword);
		lstrcat (szBuf, "\t");
	}

	_lwrite( hFile, (LPCSTR)szBuf, lstrlen(szBuf) );

	wsprintf (szBuf, "\"\"\"%s\"\"\"\t", m_pszShellExecuteFileName);
	_lwrite( hFile, (LPCSTR)szBuf, lstrlen(szBuf) );
	
	wsprintf (szBuf, "\"\"\"%s\"\"\"\t", m_pszShellExecuteDirectory);
	_lwrite( hFile, (LPCSTR)szBuf, lstrlen(szBuf) );

	wsprintf (szBuf, "\"\"\"%s\"\"\"\t", m_pszShellExecuteParameters);
	_lwrite( hFile, (LPCSTR)szBuf, lstrlen(szBuf) );

	wsprintf (szBuf, "\"\"\"%d\"\"\"\t", m_nShellExecuteShow);
	_lwrite( hFile, (LPCSTR)szBuf, lstrlen(szBuf) );

	// group
	sprintf (szBuf, "0x%I64X\t", m_cGroup);
	_lwrite( hFile, (LPCSTR)szBuf, lstrlen(szBuf) );

    return TRUE;
}

BOOL CCommand::TextWriteIniValue( HFILE hFile)
{
    char szBuf[_MAX_PATH*2];

	lstrcpy (szBuf, "");
 
    if( m_fUninstallFile )
    {
		lstrcat (szBuf, Keywords[TOK_UNINSTALL].pszKeyword);
		lstrcat (szBuf, "\t");
    }
	
    if( m_fUninstallAll )
    {
		lstrcat (szBuf, Keywords[TOK_UNINSTALLALL].pszKeyword);
		lstrcat (szBuf, "\t");
    }
	if( m_fMap )
    {
		lstrcat (szBuf, Keywords[TOK_MAP].pszKeyword);
		lstrcat (szBuf, "\t");
	}
 
	_lwrite( hFile, (LPCSTR)szBuf, lstrlen(szBuf) );

    // ini Filename, assume no backslashes
    if( !m_fMap )
    {
		wsprintf (szBuf, "\"\"\"%s\"\"\"\t", m_pszIniFilename);
		_lwrite( hFile, (LPCSTR)szBuf, lstrlen(szBuf) );
    }

    // ini Section
	wsprintf (szBuf, "\"\"\"%s\"\"\"\t", m_pszIniSection);
	_lwrite( hFile, (LPCSTR)szBuf, lstrlen(szBuf) );

    // ini Entry
	wsprintf (szBuf, "\"\"\"%s\"\"\"\t", m_pszIniEntry);
	_lwrite( hFile, (LPCSTR)szBuf, lstrlen(szBuf) );

    // ini Value
	wsprintf (szBuf, "\"\"\"%s\"\"\"\t", m_pszIniValue);
	_lwrite( hFile, (LPCSTR)szBuf, lstrlen(szBuf) );

	// ini Type
	{
		wsprintf (szBuf, "\"\"\"%s\"\"\"\t", m_pszIniType);
		_lwrite( hFile, (LPCSTR)szBuf, lstrlen(szBuf) );
	} 

	// group  
	sprintf (szBuf, "0x%I64X\t", m_cGroup);
	_lwrite( hFile, (LPCSTR)szBuf, lstrlen(szBuf) );

    return TRUE;
}


BOOL CCommand::TextWriteString ( HFILE hFile )
{
    char szBuf[_MAX_PATH*2];

	lstrcpy (szBuf, "");

	// StringID
	wsprintf (szBuf, "%d\t", m_nStringID);
	_lwrite( hFile, (LPCSTR)szBuf, lstrlen(szBuf) );

	// StringValue
	wsprintf (szBuf, "\"\"\"%s\"\"\"\t", m_pszStringValue);
	_lwrite( hFile, (LPCSTR)szBuf, lstrlen(szBuf) );

    return TRUE;
}


BOOL CCommand::TextWriteAction ( HFILE hFile )
{
    char szBuf[_MAX_PATH*2];

	lstrcpy (szBuf, "");

	// Command
	
	wsprintf (szBuf, "\"\"\"%s\"\"\"\t", m_pszActionCommand);
	_lwrite( hFile, (LPCSTR)szBuf, lstrlen(szBuf) );
	
	// Param 1 (optional)
	if (m_pszActionParam1)
	{
		wsprintf (szBuf, "\"\"\"%s\"\"\"\t", m_pszActionParam1);
		_lwrite( hFile, (LPCSTR)szBuf, lstrlen(szBuf) );
	}

	// Param 2 (optional)
	if (m_pszActionParam2)
	{
		wsprintf (szBuf, "\"\"\"%s\"\"\"\t", m_pszActionParam2);
		_lwrite( hFile, (LPCSTR)szBuf, lstrlen(szBuf) );
	}

	// Param 3 (optional)
	if (m_pszActionParam3)
	{
		wsprintf (szBuf, "\"\"\"%s\"\"\"\t", m_pszActionParam3);
		_lwrite( hFile, (LPCSTR)szBuf, lstrlen(szBuf) );
	}

	// Param 4 (optional)
	if (m_pszActionParam4)
	{
		wsprintf (szBuf, "\"\"\"%s\"\"\"\t", m_pszActionParam4);
		_lwrite( hFile, (LPCSTR)szBuf, lstrlen(szBuf) );
	}

	if (m_bRecurseFlag)
	{
		lstrcpy (szBuf, "\t\tRECURSE\t");
		_lwrite( hFile, (LPCSTR)szBuf, lstrlen(szBuf) );
	}

    return TRUE;
}
