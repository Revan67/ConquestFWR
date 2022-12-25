//
// command.h
//
//		03/15/97 update timestamp

#ifndef __COMMAND_H
#define __COMMAND_H

#include "verutil.h"


//////////////////////////////////////////////////////////////////////////////
// CCommand
//
class CCommand
{
private:
	char*	m_pszSourceName;
	char*	m_pszDestName;			//also used as module name for WinExec and file name for ReadFileList
	__int64		m_cGroup;
	int		m_nDiskId;
	
	// AddIniValue vars
	char*	m_pszIniFilename;
	char*	m_pszIniSection;
	char*	m_pszIniEntry;
	char*	m_pszIniValue;
	char*   m_pszIniType;

	// MkDir vars
	char*	m_pszMkDirValue;
    __int64 m_cDirGroup;
	
	// DeleteFile vars
	char*	m_pszDeleteFileValue;
	
	// InstDX variables
	char*	m_pszInstDXValue;
	char*	m_pszInstDXNameValue;
	char*	m_pszInstDXFlagsValue;
	char*	m_pszInstDXMinVersion;
	
	// InstDPLAY variables
	char*	m_pszInstDPLAYNameValue;
	char*	m_pszInstDPLAYMinVersion;
	
	// InstIcon vars
	char*	m_pszInstIconNameValue;
	char*   m_pszInstIconNameIconValue;
	char*	m_pszInstIconDescriptionValue;
	char*	m_pszInstIconIndexValue;
	char*	m_pszInstIconDestinationValue;
	
	// CDSpeed vars
	char*	m_pszCDSpeedMinCDValue;
	char*	m_pszCDSpeedMaxCPUValue;
	char*	m_pszCDSpeedFileNameValue;
	
	// CabList vars
	char*   m_pszCabName;
	
	// RegWiz vars
	char*	m_pszRegWizRegName;

	// ShellExecute vars
	char*  m_pszShellExecuteFileName;
	char*  m_pszShellExecuteDirectory;
	char*  m_pszShellExecuteParameters;
	int    m_nShellExecuteShow;
	BOOL   m_fShellExecuteWait;

	// Comment string
	char*  m_pszComment;

	// Property strings
	char*  m_pszProperty;
	char*  m_pszPropertyValue;

	// Rule vars
	char*  m_pszRuleAction;		// Action to take when rule matches pattern
	char*  m_pszRulePattern;	// pattern to match

	// String vars
	int	   m_nStringID;
	char*  m_pszStringValue;

	// Action command vars
	char*  m_pszActionCommand;
	char*  m_pszActionParam1;
	char*  m_pszActionParam2;
	char*  m_pszActionParam3;
	char*  m_pszActionParam4;
	bool   m_bRecurseFlag;
	
	bool   m_fCabPreCopy;

	// other
	ETOKEN		m_eCommandType;		//which command is it, TOK_INSTALL etc.
	int			m_nCmdID;			//command ID
	BOOL		m_fMap;				// 'map' flag for AddIniValue
	BOOL		m_fWin95;
	BOOL		m_fNT40;
	BOOL		m_fWin98;
	BOOL		m_fNT50;
	BOOL		m_fDBCS;
	BOOL		m_fOEM;
	BOOL		m_fRTL;
	BOOL		m_fJPN;
	BOOL		m_fGER;
	BOOL		m_fFRA;
	BOOL		m_fSPA;
	BOOL		m_fUSA;
	BOOL		m_fAPP1;
	BOOL		m_fAPP2;
	BOOL		m_fAPP3;
	BOOL		m_fIMEENABLE;
	BOOL		m_fIMEON;
	BOOL		m_fSysFile;
	BOOL		m_fUninstallFile;	// should file be uninstalled?
	BOOL		m_fUninstallAll;	// should uninstall be recursive?
	BOOL		m_fUninstallLink;	// remove only the link instead of the folder for InstIcon Commands.
	BOOL		m_fCab;				// is source file in a cab file
	BOOL		m_fSharedFile;
	BOOL		m_fDLLRegister;
	BOOL		m_fWindowsDir;
	BOOL		m_fSystemDir;
	BOOL		m_fNotUninstall;	// ignore during uninstall
	BOOL		m_fInstall;			// valid during install/reinstall
	BOOL		m_fFont;			// for installlist
	BOOL		m_fUninstOnly;		// ignore during Install/Maint (installlist)
	BOOL     m_fAppDir;
	BOOL     m_fMkDir;
	BOOL     m_fMkRoot;
	BOOL     m_fGetName;
	BOOL     m_fGetPID;
	FILEINFO	m_FileInfo;			//version, etc. for INSTALLFILE
	
public:
	CCommand();
	~CCommand();
	
	BOOL	IsValidToken ();
	int		GetCommandID()
				{ return m_nCmdID; }
	void	SetCommandID( int nCmdID )
				{ m_nCmdID = nCmdID; }
//	BOOL	Write( HFILE hFile, int nBaseID );
	BOOL	BinaryWrite( HFILE hFile );
	BOOL	TextWrite (HFILE hFile);
	void	SetMapFlag()
				{ m_fMap = TRUE; }
	BOOL	GetMapFlag ()
				{ return m_fMap; }
	void	SetWin95Flag()
				{ m_fWin95 = TRUE; }
	void	SetWin98Flag()
				{ m_fWin98 = TRUE; }
	void	SetNT40Flag()
				{ m_fNT40 = TRUE; }
	void	SetNT50Flag()
				{ m_fNT50 = TRUE; }
	void	SetDBCSFlag()
				{ m_fDBCS = TRUE; }
	void	SetOEMFlag()
				{ m_fOEM = TRUE; }
	void	SetRTLFlag()
				{ m_fRTL = TRUE; }
	void	SetJPNFlag()
				{ m_fJPN = TRUE; }
	void	SetGERFlag()
				{ m_fGER = TRUE; }
	void	SetFRAFlag()
				{ m_fFRA = TRUE; }
	void	SetSPAFlag()
				{ m_fSPA = TRUE; }
	void	SetUSAFlag()
				{ m_fUSA = TRUE; }
	void	SetAPP1Flag()
				{ m_fAPP1 = TRUE; }
	void	SetAPP2Flag()
				{ m_fAPP2 = TRUE; }
	void	SetAPP3Flag()
				{ m_fAPP3 = TRUE; }
	void	SetIMEENABLEFlag()
				{ m_fIMEENABLE = TRUE; }
	void	SetIMEONFlag()
				{ m_fIMEON = TRUE; }
	void	SetWindowsDirFlag()
				{ m_fWindowsDir = TRUE; }
	BOOL	GetWindowsDirFlag()
				{ return m_fWindowsDir; }
	void	SetSystemDirFlag()
				{ m_fSystemDir = TRUE; }
	BOOL	GetSystemDirFlag()
				{ return m_fSystemDir; }
	void	SetAppDirFlag()
				{ m_fAppDir = TRUE; }
	BOOL	GetAppDirFlag()
				{ return m_fAppDir; }
	void	SetMkDirFlag()
				{ m_fMkDir = TRUE; }
	void	SetMkRootFlag()
				{ m_fMkRoot = TRUE; }
	void	SetDirGroup(__int64 cGroupValue)
				{ m_cDirGroup = cGroupValue; }
	void	SetGetNameFlag()
				{ m_fGetName = TRUE; }
	void	SetGetPIDFlag()
				{ m_fGetPID = TRUE; }
//	void	SetSystem32DirFlag()
//				{ m_fSystem32Dir = TRUE; }
	void	SetSysFileFlag()
				{ m_fSysFile = TRUE; }
	BOOL	GetSysFileFlag()
				{ return m_fSysFile; }
	void	SetUninstallFileFlag();
	void	SetUninstallAllFlag();
	void	SetUninstallLinkFlag()
				{ m_fUninstallLink = TRUE; }
	BOOL	GetUninstallFileFlag()
				{ return m_fUninstallFile; }
	BOOL	GetUninstallAllFlag()
				{ return m_fUninstallAll; }
	BOOL	GetUninstallLinkFlag()
				{ return m_fUninstallLink; }
	void	SetCabFlag()
				{ m_fCab = TRUE; }
	BOOL	GetCabFlag()
				{ return m_fCab; }
	void	SetSharedFileFlag()
				{ m_fSharedFile = TRUE; }
	BOOL	GetSharedFileFlag()
				{ return m_fSharedFile; }

	void	SetDLLRegisterFlag()
				{ m_fDLLRegister = TRUE; }
	BOOL	GetDLLRegisterFlag()
				{ return m_fDLLRegister; }
	void	SetCommandType( ETOKEN tokType )
				{ m_eCommandType = tokType; }
	void	SetGroup(__int64 cGroupValue)
				{ m_cGroup = cGroupValue; }
	void	SetDiskId(int nDiskIdValue)
				{ m_nDiskId = nDiskIdValue; }

	ETOKEN	GetCommandType()
				{ return m_eCommandType; }
	
	void	SetDestName( LPCSTR lpszDestName );
	
	void	SetSourceName( LPCSTR lpszSourceName );
	
    void	SetIniFilename( LPCSTR lpszIniFilename );
	void	SetIniSection( LPCSTR lpszIniSection );
	void	SetIniEntry( LPCSTR lpszIniEntry );
	void	SetIniValue( LPCSTR lpszIniValue );
	void    SetIniType( LPCSTR lpszIniType );
	
	void	SetMkDirValue( LPCSTR lpszMkDirValue );
	
	void	SetDeleteFileValue( LPCSTR lpszDeleteFileValue );
	void	SetDeleteFileSilentFlag()
				{ m_fUninstallFile = TRUE; }  //overloaded for silent delete...
	BOOL	GetDeleteFileSilentFlag()
				{ return m_fUninstallFile; }
	void	SetDeleteFilePersistFlag()
				{ m_fNotUninstall = TRUE; }
	BOOL	GetDeleteFilePersistFlag()
				{ return m_fNotUninstall; }
	void	SetDeleteFileRecurseFlag()
				{ m_fUninstallAll = TRUE; }  //overloaded for recursive delete
	BOOL	GetDeleteFileRecurseFlag()
				{ return m_fUninstallAll; }
	void	SetDeleteFileInstallFlag()
				{ m_fInstall = TRUE; } 
	BOOL	GetDeleteFileInstallFlag()
				{ return m_fInstall; }
	void	SetFontFlag()
				{ m_fFont = TRUE; } 
	BOOL	GetFontFlag()
				{ return m_fFont; }
	void	SetUninstOnlyFlag()
				{ m_fUninstOnly = TRUE; } 
	BOOL	GetUninstOnlyFlag()
				{ return m_fUninstOnly; }
	void	SetSharedFlag()
				{ m_fSharedFile = TRUE; } 
	BOOL	GetSharedFlag()
				{ return m_fSharedFile; }

	void	SetInstDXValue( LPCSTR lpszInstDXValue );
	void	SetInstDXNameValue( LPCSTR lpszInstDXNameValue );
	void	SetInstDXFlagsValue( LPCSTR lpszInstDXFlagsValue );
	void	SetInstDXMinVersionValue( LPCSTR );
	
	void	SetInstDPLAYNameValue( LPCSTR lpszInstDPLAYNameValue );
	void	SetInstDPLAYMinVersionValue( LPCSTR );
	
	void	SetInstIconNameValue( LPCSTR lpszInstIconNameValue );
	void    SetInstIconNameIconValue( LPCSTR lpszInstIconNameIconValue );
	void	SetInstIconDescriptionValue( LPCSTR lpszInstIconDescriptionValue );
	void	SetInstIconIndexValue( LPCSTR lpszInstIconIndexValue);
	void	SetInstIconDestinationValue( LPCSTR lpszInstIconDestinationValue);
	
	void	SetCDSpeedFileNameValue( LPCSTR lpszCDSpeedFileNameValue );
	void	SetCDSpeedMinCDValue( LPCSTR lpszCDSpeedMinCDValue  );
	void	SetCDSpeedMaxCPUValue( LPCSTR lpszCDSpeedMaxCPUValue  );
	
	void	SetCabName( LPCSTR lpszCabName );

    void	SetRegWizRegName(LPCSTR lpszRegWizRegName);

	void	SetShellExecuteFileName(LPCSTR lpszShellExecuteFileName);
	void	SetShellExecuteDirectory(LPCSTR lpszShellExecuteDirectoryName);
	void	SetShellExecuteParameters(LPCSTR lpszShellExecuteParameters);
	void	SetShellExecuteShowFlag(int nShowFlags)
				{m_nShellExecuteShow = nShowFlags; }
	void	SetShellExecuteWaitFlag()
				{m_fShellExecuteWait = TRUE; }
	BOOL	GetShellExecuteWaitFlag()
				{return m_fShellExecuteWait; }

	void	SetReadFileListName(LPCSTR lpszFileListName);

	void	SetComment( LPCSTR lpsz );
	void	SetProperty( LPCSTR lpsz );
	void	SetPropertyValue( LPCSTR lpsz );
	void	SetRulePattern (LPCSTR lpsz);
	void	SetRuleAction (LPCSTR lpsz);

	void	SetStringID (int nStringID);
	void	SetStringValue (LPCSTR lpsz);
	void	SetFileInfo (FILEINFO *fi);

	void	SetActionCommand (LPCSTR lpsz);
	void	SetActionParam1 (LPCSTR lpsz);
	void	SetActionParam2 (LPCSTR lpsz);
	void	SetActionParam3 (LPCSTR lpsz);
	void	SetActionParam4 (LPCSTR lpsz);
	void	SetActionRecurseFlag (bool bRecurseFlag)
		{ m_bRecurseFlag = bRecurseFlag; }
	bool	GetActionRecurseFlag (void)
		{ return m_bRecurseFlag; }
	
	
	// General
		
	__int64 GetGroup(void)
				{ return m_cGroup; }
	LPCSTR	GetReadFileListName()
				{	return m_pszDestName; }
	int GetDiskId(void)
				{ return m_nDiskId; }

	// InstallList, etc
	LPCSTR	GetSourceName ()
				{	return m_pszSourceName; }
	LPCSTR	GetDestName ()
				{	return m_pszDestName; }

	// AddIniValue
	LPCSTR	GetIniFilename()
				{	return m_pszIniFilename; }
	LPCSTR	GetIniSection ()
				{	return m_pszIniSection; }
	LPCSTR	GetIniEntry ()
				{	return m_pszIniEntry; }
	LPCSTR	GetIniValue ()
				{	return m_pszIniValue; }
	LPCSTR	GetIniType ()
				{	return m_pszIniType; }

	// MkDir
	LPCSTR	GetDirName ()
				{	return m_pszMkDirValue; }

	__int64 GetDirGroup(void)
				{ return m_cDirGroup; }

	// DeleteFile
	LPCSTR	GetDeleteFileValue ()
				{	return m_pszDeleteFileValue; }

	// InstDx
	LPCSTR	GetInstDxValue ()
				{	return m_pszInstDXValue; }
	LPCSTR	GetInstDxNameValue ()
				{	return m_pszInstDXNameValue; }
	LPCSTR	GetInstDxFlagsValue ()
				{	return m_pszInstDXFlagsValue; }
	LPCSTR	GetInstDxMinVersion ()
				{	return m_pszInstDXMinVersion; }

	// InstIcon
	LPCSTR	GetInstIconNameValue ()
				{	return m_pszInstIconNameValue; }
	LPCSTR	GetInstIconNameIconValue ()
				{	return m_pszInstIconNameIconValue; }
	LPCSTR	GetInstIconDescriptionValue ()
				{	return m_pszInstIconDescriptionValue; }
	LPCSTR	GetInstIconIndexValue ()
				{	return m_pszInstIconIndexValue; }
	LPCSTR	GetInstIconDestinationValue ()
				{	return m_pszInstIconDestinationValue; }

	// CDSpeed	
	LPCSTR	GetCDSpeedMinCDValue ()
				{	return m_pszCDSpeedMinCDValue; }
	LPCSTR	GetCDSpeedMaxCPUValue ()
				{	return m_pszCDSpeedMaxCPUValue; }
	LPCSTR	GetCDSpeedFileNameValue ()
				{	return m_pszCDSpeedFileNameValue; }
	// CabList
	LPCSTR	GetCabName ()
				{	return m_pszCabName; }

	// RegWiz
	LPCSTR	GetRegWizRegName ()
				{	return m_pszRegWizRegName; }

	// ShellExecute
	LPCSTR	GetShellExecuteFileName ()
				{	return m_pszShellExecuteFileName; }
	LPCSTR	GetShellExecuteDirectory ()
				{	return m_pszShellExecuteDirectory; }
	LPCSTR	GetShellExecuteParameters ()
				{	return m_pszShellExecuteParameters; }
	int		GetShellExecuteShow ()
				{	return m_nShellExecuteShow; }
	BOOL	GetShellExecuteWait ()
				{	return m_fShellExecuteWait; }

	// Comment
	LPCSTR	GetComment ()
				{	return m_pszComment; }

	
	// PrepStub98: Rule, Property, Localize, History, String, etc
	LPCSTR	GetRuleAction ()
				{	return m_pszRuleAction; }
	LPCSTR	GetRulePattern ()
				{	return m_pszRulePattern; }
	LPCSTR	GetProperty ()
				{	return m_pszProperty; }
	LPCSTR	GetPropertyValue ()
				{	return m_pszPropertyValue; }
	int		GetStringID ()
				{	return m_nStringID; }
	LPCSTR	GetStringValue ()
				{	return m_pszStringValue; }

	LPCSTR	GetActionCommand ()
				{	return m_pszActionCommand; }

	LPCSTR	GetActionParam1 ()
				{	return m_pszActionParam1; }

	LPCSTR	GetActionParam2 ()
				{	return m_pszActionParam2; }

	LPCSTR	GetActionParam3 ()
				{	return m_pszActionParam3; }

	LPCSTR	GetActionParam4 ()
				{	return m_pszActionParam4; }

	void	SetCabPreCopy (bool fCabPreCopy)
				{	m_fCabPreCopy = fCabPreCopy; }

	bool	GetCabPreCopy (void)
				{	return m_fCabPreCopy; }

	DWORD	GetBuildFlags ();
	BOOL 	GetSourceVersionInfo( LPCSTR lpszStubPath );
	//BOOL	GetReadFileListName(LPCSTR lpszFileListName);

	void	SetBuildFlags (DWORD dwBuildFlags);
	DWORD	GetInstallFlags ();
	void	SetInstallFlags (DWORD dwInstallFlags);
	
private:
/*	BOOL 	WriteInstallFile( HFILE hFile, int nCmdID );
	BOOL 	WriteMkDir( HFILE hFile, int nCmdID );
	BOOL	WriteMkRoot( HFILE hFile, int nCmdID );
	BOOL	WriteGetName( HFILE hFile, int nCmdID );
	BOOL	WriteGetGroup( HFILE hFile, int nCmdID );
	BOOL	WriteGetPID( HFILE hFile, int nCmdID );
	BOOL	WriteInstDX( HFILE hFile, int nCmdID );
	BOOL	WriteInstDPLAY( HFILE hFILE, int nCmdID );
	BOOL	WriteInstIcon( HFILE hFile, int nCmdID );
	BOOL 	WriteIniValue( HFILE hFile, int nCmdID );
	BOOL 	WriteCDSpeed( HFILE hFile, int nCmdID );
	BOOL	WriteInstallGo(HFILE hFile, int nCmdID);
    BOOL	WriteCabGo( HFILE hFile, int nCmdID );
	BOOL	WriteRegWiz(HFILE hFile, int nCmdID);
	BOOL	WriteShellExecute(HFILE hFile, int nCmdID);
    BOOL	WriteDeleteFile(HFILE hFile, int nCmdID);
	BOOL	WriteReadFileList(HFILE hFile, int nCmdID);
*/	BOOL 	BinaryWriteInstallFile( HFILE hFile, int nCmdID );
	BOOL 	BinaryWriteMkDir( HFILE hFile, int nCmdID );
	BOOL	BinaryWriteMkRoot( HFILE hFile, int nCmdID );
	BOOL	BinaryWriteGetName( HFILE hFile, int nCmdID );
	BOOL	BinaryWriteGetGroup( HFILE hFile, int nCmdID );
	BOOL	BinaryWriteGetPID( HFILE hFile, int nCmdID );
	BOOL	BinaryWriteInstDX( HFILE hFile, int nCmdID );
	BOOL	BinaryWriteInstDPLAY( HFILE hFile, int nCmdID );
	BOOL	BinaryWriteInstIcon( HFILE hFile, int nCmdID );
	BOOL 	BinaryWriteIniValue( HFILE hFile, int nCmdID );
	BOOL 	BinaryWriteCDSpeed( HFILE hFile, int nCmdID );
	BOOL	BinaryWriteInstallGo(HFILE hFile, int nCmdID);
    BOOL	BinaryWriteCabGo( HFILE hFile, int nCmdID );
	BOOL	BinaryWriteRegWiz(HFILE hFile, int nCmdID);
	BOOL	BinaryWriteShellExecute(HFILE hFile, int nCmdID);
	BOOL	BinaryWriteDeleteFile(HFILE hFile, int nCmdID);
	BOOL	BinaryWriteReadFileList(HFILE hFile, int nCmdID);

	// Begin Prepstub98 only functions.
	BOOL 	TextWriteInstallFile (HFILE hFile);
	BOOL 	TextWriteMkDir (HFILE hFile);
	BOOL	TextWriteMkRoot (HFILE hFile);
	BOOL	TextWriteGetName (HFILE hFile);
	BOOL	TextWriteGetGroup (HFILE hFile);
	BOOL	TextWriteGetPID (HFILE hFile);
	BOOL	TextWriteInstDX (HFILE hFile );
	BOOL	TextWriteInstDPlay (HFILE hFile);
	BOOL	TextWriteInstIcon (HFILE hFile);
	BOOL 	TextWriteIniValue (HFILE hFile);
	BOOL 	TextWriteCDSpeed (HFILE hFile);
    BOOL	TextWriteCabGo (HFILE hFile);
	BOOL	TextWriteRegWiz (HFILE hFile);
	BOOL	TextWriteShellExecute (HFILE hFile);
    BOOL	TextWriteDeleteFile (HFILE hFile);
	BOOL	TextWriteComment (HFILE hFile);
	BOOL	TextWriteRule (HFILE hFile);
	BOOL	TextWriteProperty (HFILE hFile);
	BOOL	TextWriteReadFile (HFILE hFile);
	BOOL	TextWriteString (HFILE hFile);
	BOOL	TextWriteAction (HFILE hFile);
	// End Prepstub98 only functions
};


#endif //__COMMAND_H
