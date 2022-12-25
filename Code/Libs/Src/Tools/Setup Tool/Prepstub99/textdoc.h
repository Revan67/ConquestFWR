//
// textdoc.h
//
//		03/15/97 update timestamp

#ifndef __TEXTDOC_H
#define __TEXTDOC_H

//*** Token flags
//
#define TF_SINGLE       0x0001      //these divide tokens
#define TF_BRACKETED    0x0002      //[KEYWORD] comes in as a single token
#define TF_KEYWORD      0x0004      //any other keyword

//possible compiler tokens.  Order must match the Keywords defined
//in textdoc.cpp.
enum ETOKEN
{
    TOK_EOF = 0,
    TOK_BEGIN,
    TOK_WHITESPACE,
    TOK_DOUBLEQUOTE,
    TOK_SINGLEQUOTE,
    TOK_NEWLINE,
    TOK_COMMA,
    TOK_LEFTBRACKET,
    TOK_RIGHTBRACKET,
    TOK_HYPHEN,
    TOK_COMMENT,
    TOK_WIN95,
	TOK_WIN98,
	TOK_ALLWIN,
    TOK_NT40,
	TOK_NT50,
	TOK_ALLNT,
	TOK_DBCS,
	TOK_OEM,
	TOK_RTL,
	TOK_JPN,
	TOK_GER,
	TOK_FRA,
	TOK_SPA,
	TOK_USA,
	TOK_APP1,
	TOK_APP2,
	TOK_APP3,
    TOK_IMEENABLE,
    TOK_IMEON,
    TOK_INSTALL,
    TOK_INIVALUE,
    TOK_MAP,
    TOK_WINDOWS,
    TOK_SYSTEM,
	TOK_CHECKVER,
//    TOK_SYSTEM32,
    TOK_APP,
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
	TOK_WAIT,
	TOK_SILENT,
    TOK_CAB,
    TOK_INSTALLLIST,
    TOK_INSTALLGO,
	TOK_CABGO,
    TOK_REGWIZ,
	TOK_SHELLEXECUTE,
    TOK_SHARED,
	TOK_DLLREGISTER,
    TOK_INSTFONT,
	TOK_READFILELIST,
	TOK_PROPERTY,
	TOK_RULE,
	TOK_LOCALIZE,
	TOK_HISTORY,
	TOK_STRINGVAR,
	TOK_UNINSTALLLINK,
	TOK_BEGINFILELIST,
	TOK_ENDFILELIST,
	TOK_BEGINSTRINGLIST,
	TOK_ENDSTRINGLIST,
	TOK_BEGINSTATICSTRINGLIST,
	TOK_ENDSTATICSTRINGLIST,
	TOK_DISK_01,
	TOK_DISK_02,
	TOK_DISK_03,
	TOK_DISK_04,
	TOK_DISK_05,
	TOK_DISK_06,
	TOK_DISK_07,
	TOK_DISK_08,
	TOK_PRECOPY,
	TOK_PERSIST,
	TOK_RECURSE,
	TOK_DELFILEINSTALL,
	TOK_FONT,
	TOK_UNINSTALLONLY,
	TOK_ACTION,

    TOK_STRING,
    TOK_NUMBER,
    DEFAULT_ACTION, //not a token but indicates what to do in default case

    NUM_TOKENS
};


class CToken
{
public:
    ETOKEN          eToken;
    LPCSTR          pszValue;
    BOOL            fQuoted;        //Were there double quotes around token
    BOOL            fBracketed;     //Were there brackets around token

public:
    CToken( ETOKEN tok, LPCSTR pVal, BOOL quot, BOOL brac )
        { eToken = tok; pszValue = pVal; fQuoted = quot; fBracketed = brac; }
    CToken()
        { fQuoted = FALSE; fBracketed = FALSE; }
};

//contents of a keyword
typedef struct tagKEYWORD
{
    UINT    uToken;
    char*   pszKeyword;
    UINT    uFlags;
} KEYWORD, *LPKEYWORD;

//possible compiler transition actions.  Order is not important.
enum EACTION
{
    actNOP,             actJump,            actPush,            actPop,
    actError
};

//possible "Major" compiler states to be in.  Order is not important.
enum EMAJOR
{
    jDontCare,          jSetupDoc,          jInstall,           jDeleteFile,
    jAddShare,          jIniValue,          jGetGroup,          jReadFileList,
    jCabGo,             jInstFont,          jMkDir,             jMkRoot,
    jGetName,           jGetPID,            jInstDX,            jInstIcon,
    jCDSpeed,           jInstallList,       jInstallGo,			jRegWiz,
	jShellExecute,		
	jComment,			jBeginFileList,		jEndFileList,		jProperty,
	jRule,				jBeginStringList,	jEndStringList,		jString,
	jBeginStaticStringList,	jEndStaticStringList, jInstDPLAY,	jAction
};

//possible "Minor" compiler states to be in.  Order is not important.
enum EMINOR
{
    nDontCare,          nStart,				nPersist,			nRecurse,
    nOS,                nDestDir,           nSysFile,           nSharedFile,
    nDestName,          nSrcName,           nGroup,             nGroup2,
	nFilename,          nSection,			nDLLRegister,		nSilent,
    nEntry,             nValue,             nParam,             nModule,	nType,
    nMap,               nUninstall,			nUninstallAll,      nMkDir,             
    nGetName,			nDirectory,			nParameters,		nShow,
    nInstDX,			nInstDXName,        nInstDXFlags,		nWait,
    nInstDXMinVersion,	nInstIconName,      nInstIconPath,		nInstIconNameIcon,
    nInstIconIndex,     nInstIconDest,		nFont,				nUninstOnly,
    nCDSpeedCDMin,      nCDSpeedCPUMax,     nSysDir,			nAppDir,
    nCDSpeedName,       nGo,                nGoDone,			nWinDir,
    nReadFileList,		nReadFileListName,	nRegWiz,
	nCabGo,				nCabName,			nCab,
	nProperty,			nPropertyValue,		nPattern, nAction, nCD,
	nDiskId,			nInstall,			nStringID,	nStringValue, nUninstallLink,
	nInstDPLAYName,		nInstDPLAYMinVersion,					nPrecopy,
	nActionCommand,		nActionParam1,		nActionParam2,		nActionParam3, nActionParam4
};

//description of a compiler state transition
typedef struct tagEXPECTED {

    EMAJOR      majFrom;        //major state being examined
    EMINOR      minFrom;        //minor state being examined
    ETOKEN      eToken;         //token causing an action
    EACTION     eAction;        //action to perform when token is found in state majFrom:minFrom
    EMAJOR      majTo;          //major state to transition to (for actJump & actError)
    EMINOR      minTo;          //minor state to transition to (for actJump & actError)

} EXPECTED, *LPEXPECTED;



/////////////////////////////////////////////////////////////////////////////
// CTextDocument document

class CTextDocument
{
// Attributes
protected:
    HGLOBAL     m_hFileData;        //global handle to the file's contents
    LPSTR       m_lpFileData;       //pointer to the file's contents
    DWORD       m_cchFileData;      //size of the file, in bytes
    HPSTR       m_hpFilePtr;        //huge pointer where we are now
    DWORD       m_ichFilePtr;       //pointer into the data buffer
    char        m_chT;              //char that was overwritten with '\0'
    char        m_szPathName[_MAX_PATH];    //full pathname of the file
    BOOL        m_fExcel;           //Excel work-around (hack)

public:
    BOOL        OpenFile( const char* pszFileName );
    void        EnableExcelWorkaround()
                    { m_fExcel = TRUE; }
    void        DisableExcelWorkaround()
                    { m_fExcel = FALSE; }

protected:
    BOOL        ReadNextToken( CToken* pToken );
    BOOL        DispatchToken( ETOKEN eToken, const char*pszValue, BOOL fQuoted=FALSE, BOOL fBracketed=FALSE )
                    { CToken tok( eToken, pszValue, fQuoted, fBracketed );
                      return OnAddToken( tok ); }
    void        SetPathName( const char* pszNewPath )
                    { strncpy( m_szPathName, pszNewPath, sizeof(m_szPathName) ); }

//Should be overridden by derived classes.
protected:
    virtual BOOL    OnAddToken( const CToken& Token );
};


#endif //__TEXTDOC_H
