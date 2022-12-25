// setupdoc.h : header file
//

#include "textdoc.h"        //include base class



//forward declaration
class CCommand;


typedef int (* SORTCOMPAREPROC) (const void *pItem1, const void *pItem2);

/////////////////////////////////////////////////////////////////////////////
// CSetupDoc document

class CSetupDoc : public CTextDocument
{
private:
    int         m_cCommands;
    CCommand**  m_papCommands;
    int         m_maxCommands;
	int			m_iCommandPtr;

    LPCSTR      m_lpszStubPath;

public:
	void		SetCommandPtr (int iPos) {m_iCommandPtr = iPos; }
    int         GetNumCommands()
                    { return m_cCommands; }
	int			GetNumValidCommands();
    CCommand*   GetNthCommand( int n )
                    { return m_papCommands[n]; }
	void		SetNthCommand (int n, CCommand *pCmd)
					{ m_papCommands[n] = pCmd; }
    CCommand*   AddCommand();
	void		AttachCommand(CCommand *pCmd);
	void		InsertCommand(CCommand *pCmd);
	bool		DeleteCommand ();
    CCommand*   GetLastCommand()
                    { return m_papCommands[m_cCommands-1]; }
    LPCSTR      GetStubPath()
                    { return m_lpszStubPath; }
	int			FindFirstToken (ETOKEN eToken);
	int			FindFirstToken (ETOKEN eToken, int iStart, int iEnd);
	bool		GetFileVersionInfo(const char *szFilePath);
	void		SortCommandsInRange (int nStart, int nEnd, SORTCOMPAREPROC SortCompareProc);

public:
    CSetupDoc();
    ~CSetupDoc();
    BOOL PrepareStub( const char* pszInput, const char* pszOutput, LPCSTR lpszStubPath );

	// begin prepstub98 only functions
	void ReNumberCommands();
	BOOL WriteTextFile( const char* pszOutput );
	// end prepstub98 only functions

	// bool ReadSetupScript ( const char* pszInput);
	// BOOL WriteSetupScript(const char* pszOutput, const char *szFilePath);


//private:
    BOOL    Write( const char* pszOutput );


//compiler stuff
private:
    virtual BOOL OnAddToken( const CToken& Token );
    BOOL    OnStateChanging(		EMAJOR eMajor, EMINOR eMinor, ETOKEN eToken, LPCSTR pszValue );
    BOOL    OnSetupDocToken(		EMAJOR eMajor, EMINOR eMinor, ETOKEN eToken, LPCSTR pszValue );
    BOOL    OnSetupInstallToken(	EMAJOR eMajor, EMINOR eMinor, ETOKEN eToken, LPCSTR pszValue );
    BOOL    OnSetupMkDirToken(		EMAJOR eMajor, EMINOR eMinor, ETOKEN eToken, LPCSTR pszValue );
    BOOL    OnSetupMkRootToken(		EMAJOR eMajor, EMINOR eMinor, ETOKEN eToken, LPCSTR pszValue );
    BOOL    OnSetupGetNameToken(	EMAJOR eMajor, EMINOR eMinor, ETOKEN eToken, LPCSTR pszValue );
    BOOL    OnSetupGetGroupToken(	EMAJOR eMajor, EMINOR eMinor, ETOKEN eToken, LPCSTR pszValue );
    BOOL    OnSetupGetPIDToken(		EMAJOR eMajor, EMINOR eMinor, ETOKEN eToken, LPCSTR pszValue );
    BOOL    OnSetupInstDXToken(		EMAJOR eMajor, EMINOR eMinor, ETOKEN eToken, LPCSTR pszValue );
    BOOL    OnSetupInstDPLAYToken(	EMAJOR eMajor, EMINOR eMinor, ETOKEN eToken, LPCSTR pszValue );
    BOOL    OnSetupInstIcon(		EMAJOR eMajor, EMINOR eMinor, ETOKEN eToken, LPCSTR pszValue );
    BOOL    OnSetupIniValueToken(	EMAJOR eMajor, EMINOR eMinor, ETOKEN eToken, LPCSTR pszValue );
    BOOL    OnSetupIniDWordToken(	EMAJOR eMajor, EMINOR eMinor, ETOKEN eToken, LPCSTR pszValue );
    BOOL    OnSetupInstFontToken(	EMAJOR eMajor, EMINOR eMinor, ETOKEN eToken, LPCSTR pszValue );
    BOOL    OnSetupCDSpeed(			EMAJOR eMajor, EMINOR eMinor, ETOKEN eToken, LPCSTR pszValue );
    BOOL    OnSetupInstallList(		EMAJOR eMajor, EMINOR eMinor, ETOKEN eToken, LPCSTR pszValue );
    BOOL    OnSetupInstallGo(		EMAJOR eMajor, EMINOR eMinor, ETOKEN eToken, LPCSTR pszValue );
    BOOL    OnSetupRegWiz(			EMAJOR eMajor, EMINOR eMinor, ETOKEN eToken, LPCSTR pszValue );
    BOOL    OnSetupShellExecute(	EMAJOR eMajor, EMINOR eMinor, ETOKEN eToken, LPCSTR pszValue );
    BOOL    OnSetupDeleteFile(		EMAJOR eMajor, EMINOR eMinor, ETOKEN eToken, LPCSTR pszValue );
	BOOL	OnSetupReadFileList(	EMAJOR eMajor, EMINOR eMinor, ETOKEN eToken, LPCSTR pszValue );
	BOOL	OnSetupPropertyToken(	EMAJOR eMajor, EMINOR eMinor, ETOKEN eToken, LPCSTR pszValue );
	BOOL	OnSetupBeginFileList(	EMAJOR eMajor, EMINOR eMinor, ETOKEN eToken, LPCSTR pszValue );
	BOOL	OnSetupEndFileList(		EMAJOR eMajor, EMINOR eMinor, ETOKEN eToken, LPCSTR pszValue );
	BOOL	OnSetupCommentToken(	EMAJOR eMajor, EMINOR eMinor, ETOKEN eToken, LPCSTR pszValue );
	BOOL	OnSetupRuleToken(		EMAJOR eMajor, EMINOR eMinor, ETOKEN eToken, LPCSTR pszValue );
	BOOL	OnSetupBeginStringList(	EMAJOR eMajor, EMINOR eMinor, ETOKEN eToken, LPCSTR pszValue );
	BOOL	OnSetupEndStringList(	EMAJOR eMajor, EMINOR eMinor, ETOKEN eToken, LPCSTR pszValue );
	BOOL	OnSetupStringToken(		EMAJOR eMajor, EMINOR eMinor, ETOKEN eToken, LPCSTR pszValue );
	BOOL	OnSetupBeginStaticStrings ( EMAJOR eMajor, EMINOR eMinor, ETOKEN eToken, LPCSTR pszValue );
	BOOL	OnSetupEndStaticStrings(EMAJOR eMajor, EMINOR eMinor, ETOKEN eToken, LPCSTR pszValue );
	BOOL	OnSetupActionToken(		EMAJOR eMajor, EMINOR eMinor, ETOKEN eToken, LPCSTR pszValue );

    BOOL    OnFinishedParsing( void );

    int         m_nLine;            // what line we're in the middle of parsing
    int         m_nCmdID;           // current command ID (while parsing)

};

