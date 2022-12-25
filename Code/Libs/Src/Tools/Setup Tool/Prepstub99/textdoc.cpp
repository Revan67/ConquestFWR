//
// textdoc.cpp
//
//      Implementation of the CTextDocument class, which allows a file to
//      be easily parsed in.  Non-MFC version.
//
// History:
//
//      10/??/94    KenSh       Created
//       1/31/95    KenSh       Stripped down and added to stub project
//       8/10/95    a-DenSo     Added InstallFont command
//

#include "prepstub.h"
#include "textdoc.h"
#include "util.h"
#include "resource.h"
#include "diskinfo.h"
//extern DISKLABEL DISKS[];

//NOTE that these must be in the same order as the enum in textdoc.h
KEYWORD Keywords[] =
{
	{ TOK_EOF,				"",							0				},	//sent at end of compile
	{ TOK_BEGIN,			"",							0				},	//sent at beginning of compile
	{ TOK_WHITESPACE,		" \t",						0				},	//special case
	{ TOK_DOUBLEQUOTE,		"\"",						0				},	//special case
	{ TOK_SINGLEQUOTE,		"'",						0				},	//special case
	{ TOK_NEWLINE,			"\r\n",						TF_SINGLE		},	//special case
	{ TOK_COMMA,			",",						TF_SINGLE		},
	{ TOK_LEFTBRACKET,		"[",						TF_SINGLE		},
	{ TOK_RIGHTBRACKET,		"]",						TF_SINGLE		},
	{ TOK_HYPHEN,			"-",						TF_SINGLE		},

	{ TOK_COMMENT,			";",						TF_KEYWORD		},
	{ TOK_WIN95,			"WIN95",					TF_KEYWORD		},
	{ TOK_WIN98,			"WIN98",					TF_KEYWORD		},
	{ TOK_ALLWIN,			"ALLWIN",					TF_KEYWORD		},
	{ TOK_NT40,				"NT40",						TF_KEYWORD		},
	{ TOK_NT50,				"NT50",						TF_KEYWORD		},
	{ TOK_ALLNT,			"ALLNT",					TF_KEYWORD		},
	{ TOK_DBCS,				"DBCS",						TF_KEYWORD		},
	{ TOK_OEM,				"OEM",						TF_KEYWORD		},
	{ TOK_RTL,				"RTL",						TF_KEYWORD		},
	{ TOK_JPN,				"JPN",						TF_KEYWORD		},
	{ TOK_GER,				"GER",						TF_KEYWORD		},
	{ TOK_FRA,				"FRA",						TF_KEYWORD		},
	{ TOK_SPA,				"SPA",						TF_KEYWORD		},
	{ TOK_USA,				"USA",						TF_KEYWORD		},
	{ TOK_APP1,				"APP1",						TF_KEYWORD		},
	{ TOK_APP2,				"APP2",						TF_KEYWORD		},
	{ TOK_APP3,				"APP3",						TF_KEYWORD		},
	{ TOK_IMEENABLE,		"IMEENABLE",				TF_KEYWORD		},
	{ TOK_IMEON,			"IMEON",					TF_KEYWORD		},
	{ TOK_INSTALL,			"INSTALLFILE",				TF_KEYWORD		},
	{ TOK_INIVALUE,			"ADDINIVALUE",				TF_KEYWORD		},
	{ TOK_MAP,				"MAP",						TF_KEYWORD		},
	{ TOK_WINDOWS,			"WINDOWS",					TF_KEYWORD		},
	{ TOK_SYSTEM,			"SYSTEM",					TF_KEYWORD		},
	{ TOK_CHECKVER,			"CHECKVER",					TF_KEYWORD		},
//	{ TOK_SYSTEM32,			"SYSTEM32",					TF_KEYWORD		},
	{ TOK_APP,				"APP",						TF_KEYWORD		},
	{ TOK_MKDIR,			"MKDIR",					TF_KEYWORD		},
	{ TOK_MKROOT,			"MKROOT",					TF_KEYWORD		},
	{ TOK_GETNAME,			"GETNAME",					TF_KEYWORD		},
	{ TOK_GETPID,			"GETPID",					TF_KEYWORD		},
	{ TOK_INSTDX,			"INSTDX",					TF_KEYWORD		},
	{ TOK_INSTDPLAY,		"INSTDPLAY",				TF_KEYWORD		},
	{ TOK_INSTICON,			"INSTICON",					TF_KEYWORD		},
	{ TOK_CDSPEED,			"CDSPEED",					TF_KEYWORD		},
	{ TOK_DELETEFILE,		"DELETEFILE",				TF_KEYWORD		},
	{ TOK_GETGROUP, 		"GETGROUP",					TF_KEYWORD		},
	{ TOK_UNINSTALL,		"UNINSTALL",				TF_KEYWORD		},
	{ TOK_UNINSTALLALL,		"UNINSTALL_ALL",			TF_KEYWORD		},
	{ TOK_WAIT,				"WAIT",						TF_KEYWORD		},
	{ TOK_SILENT,			"SILENT",					TF_KEYWORD		},
	{ TOK_CAB,				"CAB",						TF_KEYWORD		},
	{ TOK_INSTALLLIST,		"INSTALLLIST",				TF_KEYWORD		},
	{ TOK_INSTALLGO,		"INSTALLGO",				TF_KEYWORD		},
	{ TOK_CABGO,			"CABGO",					TF_KEYWORD		},
	{ TOK_REGWIZ,			"REGWIZ",					TF_KEYWORD		},
	{ TOK_SHELLEXECUTE,		"SHELLEXECUTE",				TF_KEYWORD		},
	{ TOK_SHARED,			"SHARED",					TF_KEYWORD		},
	{ TOK_DLLREGISTER,		"DLLREGISTER",				TF_KEYWORD		},
	{ TOK_INSTFONT,			"INSTALLFONT",				TF_KEYWORD		},
	{ TOK_READFILELIST,		"READFILELIST",				TF_KEYWORD		},
	{ TOK_PROPERTY,			"PROPERTY",					TF_KEYWORD		},
	{ TOK_RULE,				"RULE",						TF_KEYWORD		},
	{ TOK_LOCALIZE,			"LOCALIZE",					TF_KEYWORD		},
	{ TOK_HISTORY,			"HISTORY",					TF_KEYWORD		},
	{ TOK_STRINGVAR,		"STRING",					TF_KEYWORD		},
	{ TOK_UNINSTALLLINK,	"UNINSTALLLINK",			TF_KEYWORD		},
	{ TOK_BEGINFILELIST,	"BEGIN_UPDATE_FILELIST",	TF_BRACKETED	},
	{ TOK_ENDFILELIST,		"END_UPDATE_FILELIST",		TF_BRACKETED	},
	{ TOK_BEGINSTRINGLIST,	"BEGIN_UPDATE_STRINGLIST",	TF_BRACKETED	},
	{ TOK_ENDSTRINGLIST,	"END_UPDATE_STRINGLIST",	TF_BRACKETED	},
	{ TOK_BEGINSTATICSTRINGLIST,	"BEGIN_STATIC_STRINGS",	TF_BRACKETED	},
	{ TOK_ENDSTATICSTRINGLIST,		"END_STATIC_STRINGS",	TF_BRACKETED	},
	{ TOK_DISK_01,			g_DiskKeywords[DISK_01].pszKeyword,	TF_KEYWORD		},
	{ TOK_DISK_02,			g_DiskKeywords[DISK_02].pszKeyword,	TF_KEYWORD		},
	{ TOK_DISK_03,			g_DiskKeywords[DISK_03].pszKeyword,	TF_KEYWORD		},
	{ TOK_DISK_04,			g_DiskKeywords[DISK_04].pszKeyword,	TF_KEYWORD		},
	{ TOK_DISK_05,			g_DiskKeywords[DISK_05].pszKeyword,	TF_KEYWORD		},
	{ TOK_DISK_06,			g_DiskKeywords[DISK_06].pszKeyword,	TF_KEYWORD		},
	{ TOK_DISK_07,			g_DiskKeywords[DISK_07].pszKeyword,	TF_KEYWORD		},
	{ TOK_DISK_08,			g_DiskKeywords[DISK_08].pszKeyword,	TF_KEYWORD		},
	{ TOK_PRECOPY,			"PRECOPY",					TF_KEYWORD		},
	{ TOK_PERSIST,			"PERSIST",					TF_KEYWORD		},
	{ TOK_RECURSE,			"RECURSE",					TF_KEYWORD		},
	{ TOK_DELFILEINSTALL,	"INSTALL",					TF_KEYWORD		},
	{ TOK_FONT,				"FONT",						TF_KEYWORD		},
	{ TOK_UNINSTALLONLY,	"UNINSTALLONLY",			TF_KEYWORD		},
	{ TOK_ACTION,			"ACTION",					TF_KEYWORD		},
	
};

#define FIRST_SINGLE_TOKEN      ((UINT)TOK_NEWLINE)
#define LAST_SINGLE_TOKEN       ((UINT)TOK_HYPHEN)
#define FIRST_KEYWORD_TOKEN     ((UINT)TOK_COMMENT)
#define LAST_KEYWORD_TOKEN      ((UINT)TOK_ACTION)

#define IS_WHITESPACE(ch) ((ch)==' ' || (ch)=='\t')


/////////////////////////////////////////////////////////////////////////////
// CTextDocument commands

BOOL CTextDocument::OpenFile( const char* pszFileName )
{
    BOOL fBetweenBrackets = FALSE;  //are we reading a potential PF_BRACKETED keyword?
    CToken token;   //current token

    SetPathName( pszFileName );

    m_hFileData = ::LoadFile( pszFileName, &m_cchFileData );

    if( !m_hFileData )
    {
        Alert( g_hwnd, MB_ICONSTOP | MB_OK, STR_FILENOTFOUND, (LPCSTR)pszFileName );
        return FALSE;
    }

    m_lpFileData = (LPSTR)GlobalLock( m_hFileData );

    if( !m_lpFileData )
    {
        GlobalFree( m_hFileData );
        m_hFileData = NULL;
        Alert( g_hwnd, MB_ICONSTOP | MB_OK, STR_NOMEMORY );
        return FALSE;
    }

    // we are at the beginning of the file.
    m_ichFilePtr = 0;
    m_hpFilePtr = (HPSTR)m_lpFileData;

    // notify the callback that the token stream is about to start
    if( !DispatchToken( TOK_BEGIN, (const char*)"" ) )
    {
        GlobalUnlock( m_hFileData );
        GlobalFree( m_hFileData );
        m_hFileData = NULL;
        m_lpFileData = NULL;
        return FALSE;
    }

    while( ReadNextToken( &token ) )
    {
        //Instead of passing left brackets immediately, we'll wait until
        //we get the next token to see if we should pass the 3 tokens as one.
        if( token.eToken == TOK_LEFTBRACKET && !fBetweenBrackets )
        {
            fBetweenBrackets = TRUE;
            continue;
        }
        else if( fBetweenBrackets && (UINT)token.eToken <= LAST_KEYWORD_TOKEN )
            //We've read in a token which *may* be a bracketed token
        {
            if( Keywords[(UINT)token.eToken].uFlags & TF_BRACKETED )
                //We got a bracketed token - now just make sure there's a
                //closing bracket following.
            {
                CToken tokenNext;

                if( ReadNextToken( &tokenNext ) )
                {
                    if( tokenNext.eToken == TOK_RIGHTBRACKET )
                    {
                        //Send just one (bracketed) token
                        char chT = *tokenNext.pszValue;
                        *((LPSTR)tokenNext.pszValue) = 0;

                        token.fBracketed = TRUE;

                        if( !OnAddToken( token ) )
                        {
                            GlobalUnlock( m_hFileData );
                            GlobalFree( m_hFileData );
                            m_hFileData = NULL;
                            m_lpFileData = NULL;
                            return FALSE;
                        }

                        *((LPSTR)tokenNext.pszValue) = chT;
                    }
                    else
                        //A token besides a right bracket was found.  We must
                        //send all 3 separately.
                    {
                        //Send the left bracket.
                        if( !DispatchToken( TOK_LEFTBRACKET, (const char*)Keywords[(UINT)TOK_LEFTBRACKET].pszKeyword ) )
                        {
                            GlobalUnlock( m_hFileData );
                            GlobalFree( m_hFileData );
                            m_hFileData = NULL;
                            m_lpFileData = NULL;
                            return FALSE;
                        }

                        //Send the keyword we thought might be bracketed.
                        if( !OnAddToken( token ) )
                        {
                            GlobalUnlock( m_hFileData );
                            GlobalFree( m_hFileData );
                            m_hFileData = NULL;
                            m_lpFileData = NULL;
                            return FALSE;
                        }

                        //Send the most recent (non-rightbracket) token
                        if( !OnAddToken( tokenNext ) )
                        {
                            GlobalUnlock( m_hFileData );
                            GlobalFree( m_hFileData );
                            m_hFileData = NULL;
                            m_lpFileData = NULL;
                            return FALSE;
                        }
                    }

                    //Move on to the next token.
                    fBetweenBrackets = FALSE;
                    continue;
                }
            }
        }

        //If we've gotten here, we know it's not a bracketed token.  So if we're
        //between brackets, send the opening bracket as a separate token.
        if( fBetweenBrackets )
        {
            if( !DispatchToken( TOK_LEFTBRACKET, (const char*)Keywords[(UINT)TOK_LEFTBRACKET].pszKeyword ) )
            {
                GlobalUnlock( m_hFileData );
                GlobalFree( m_hFileData );
                m_hFileData = NULL;
                m_lpFileData = NULL;
                return FALSE;
            }
            fBetweenBrackets = FALSE;
        }

        //Send the token we've most recently read.
        if( !OnAddToken( token ) )
        {
            GlobalUnlock( m_hFileData );
            GlobalFree( m_hFileData );
            m_hFileData = NULL;
            m_lpFileData = NULL;
            return FALSE;
        }
    }

    // notify callback that the token stream has ended.
    BOOL fResult = DispatchToken( TOK_EOF, "" );

    // clean up.
    GlobalUnlock( m_hFileData );
    GlobalFree( m_hFileData );
    m_hFileData = NULL;
    m_lpFileData = NULL;

    return fResult;
}


//****************************************************************************
// Class        CTextDocument
//
// Procedure    ReadNextToken
//
// Purpose      Pulls the next token out of the text file and stuffs it
//              into pToken.
//
// Parameters   pToken      where to stick the token
//
// Returns      zero at EOF, else nonzero
//
// History      10/??/94    KenSh       Created
//               2/02/95    KenSh       Added Excel workaround
//
BOOL CTextDocument::ReadNextToken( CToken* pToken )
{
    DWORD cchUsed;
    UINT iToken;
    ETOKEN type = TOK_EOF;      //an illegal value
    ETOKEN stringType;          //either TOK_SINGLEQUOTE or TOK_DOUBLEQUOTE for strings only
    BOOL fNegative = FALSE;
    HPSTR hpFirst;
    HPSTR hpTmp;

    if( m_ichFilePtr == m_cchFileData )
    {
        return FALSE;       //eof
    }

    pToken->fQuoted = FALSE;
    pToken->fBracketed = FALSE;

    hpFirst = m_hpFilePtr;

    //Replace the munged character from last time we were called
    if( m_ichFilePtr > 0 )
    {
        hpFirst[0] = m_chT;
    }

	// If we find a line that begins with a ';', we treat the entire
	// line as the token value and return
	if (hpFirst[0] == ';')
	{
	    HPSTR hpCur;

		// get 1st char of the current token
		hpCur = hpFirst;
		hpTmp = AnsiNext( hpCur );
		cchUsed = hpTmp - hpCur;
		hpCur = hpTmp;
		
		while(( m_ichFilePtr + cchUsed < m_cchFileData)
			&& (hpFirst[cchUsed] != 13) && (hpFirst[cchUsed] != 10) )
		{
			hpTmp = AnsiNext( hpCur );
			cchUsed += hpTmp - hpCur;
			hpCur = hpTmp;
		}
		
		m_hpFilePtr += cchUsed;
		m_ichFilePtr += cchUsed;

		m_chT = m_hpFilePtr[0];

		hpFirst[cchUsed]=0;		

		pToken->pszValue = (LPCSTR)AnsiNext(hpFirst);
		pToken->eToken = TOK_COMMENT;

		return true;
	}


    // In Excel when you Save As Text, the double-quote character
    // gets replaced with 3 of them, and a single double-quote character
    // is inserted in places where Excel wants them.  So we ignore a
    // double quote by itself and treat three of them like just one.
    if( m_fExcel )
    {
        if( hpFirst[0] == '\"' )
        {
            //Not currently handling case of two quotes (not one or three).

            if( hpFirst[1] == '\"' && hpFirst[2] == '\"' )
            {
                //skip 2 of them
                hpFirst += 2;
                m_hpFilePtr += 2;
                m_ichFilePtr += 2;
            }
            else
            {
                //skip the solitary quote
                hpFirst++;
                m_hpFilePtr++;
                m_ichFilePtr++;
            }
        }
    }

    //Figure out what type of token this could be... if it's a single token...
    if( IS_WHITESPACE( hpFirst[0] ) )
    {
        type = TOK_WHITESPACE;
    }
    else if( hpFirst[0] == Keywords[(UINT)TOK_SINGLEQUOTE].pszKeyword[0] )
    {
        pToken->fQuoted = TRUE;
        type = TOK_STRING;
        stringType = TOK_SINGLEQUOTE;
    }
    else if( hpFirst[0] == Keywords[(UINT)TOK_DOUBLEQUOTE].pszKeyword[0] )
    {
        pToken->fQuoted = TRUE;
        type = TOK_STRING;
        stringType = TOK_DOUBLEQUOTE;
    }
    else if( hpFirst[0] == '-' )
    {
        //we'll hypothesize that this is a negative number -- we'll check later
        //whether this is actually the case and change type if necessary.
        fNegative = TRUE;
    }
    else
    {

        //
        // parse single token
        //
        for( iToken = FIRST_SINGLE_TOKEN; iToken <= LAST_SINGLE_TOKEN; iToken++ )
        {
            if( hpFirst[0] == Keywords[iToken].pszKeyword[0] )
            {
                //Found a single-character token; return immediately
                pToken->eToken = (ETOKEN)iToken;

                //Check for special-case (2-character) newline
                if( (ETOKEN)iToken == TOK_NEWLINE &&
                    m_ichFilePtr < m_cchFileData &&
                    hpFirst[1] == Keywords[TOK_NEWLINE].pszKeyword[1] )
                {
                    //found 2 chars, skip both
                    // assume token is ASCII
                    m_hpFilePtr += 2;
                    m_ichFilePtr += 2;
                }
                else
                {
                    // assume token is ASCII
                    m_hpFilePtr += 1;
                    m_ichFilePtr += 1;
                }

                //zero out the character following the token, it will be re-added later
                if( m_ichFilePtr < m_cchFileData )
                {
                    m_chT = m_hpFilePtr[0];
                    m_hpFilePtr[0] = 0;
                }

                pToken->pszValue = (LPCSTR)hpFirst;
                return TRUE;
            }
        }
    }

    //Skip the first character for quoted strings
    if( type == TOK_STRING )
    {
        hpTmp = AnsiNext(hpFirst);
        while ( hpTmp != hpFirst )
        {
            hpFirst++;
            m_hpFilePtr++;
            m_ichFilePtr++;
        }
        ASSERT( m_ichFilePtr < m_cchFileData );
        if( m_ichFilePtr == m_cchFileData )
        {
            return FALSE;
        }
    }

    //At this point we have a multi-character token.  Move through the
    //rest of the characters until we find a separator.]
    //
    // cchUsed is # of char for multi-char token.
    // token can include DBCS character
    //

    // pointer to current char
    // hpCur points hpFirst[cch]
    HPSTR hpCur;

    // get 1st char of the current token
    hpCur = hpFirst;
    hpTmp = AnsiNext( hpCur );
    cchUsed = hpTmp - hpCur;
    hpCur = hpTmp;

    while( m_ichFilePtr + cchUsed < m_cchFileData )
    {
        //
        // White Space
        //  may be "the start of TOK_WHITESPACE",
        //  "the end of Token" or "TOK_STRING"
        //
        if( IS_WHITESPACE( hpFirst[cchUsed] ) )
        {
            if( !(type == TOK_WHITESPACE || type == TOK_STRING) )
            {
                //found the end of a token
                goto LEndOfToken;
            }
        }
        //
        // not White Space
        //
        else if( type == TOK_WHITESPACE )
        {
            //found the end of the whitespace
            goto LEndOfToken;
        }
        //
        // Single quote
        //  may be "the start of TOK_STRING" or "the end of TOK_STRING"
        //
        else if( hpFirst[cchUsed] == Keywords[(UINT)TOK_SINGLEQUOTE].pszKeyword[0] )
        {
            if( type == TOK_STRING )
            {
                if( stringType == TOK_SINGLEQUOTE )
                    //found the end of the string
                {
                    // skip the closing quote
                    // assume quote is 1 byte char
                    m_ichFilePtr++;
                    m_hpFilePtr++;
                    goto LEndOfToken;
                }
            }
            else
            {
                //found the beginning of a string, i.e. end of current token
                goto LEndOfToken;
            }
        }
        //
        // Double quote
        //  may be "the start of TOK_STRING" or "the end of TOK_STRING"
        //
        else if( hpFirst[cchUsed] == Keywords[(UINT)TOK_DOUBLEQUOTE].pszKeyword[0] )
        {
            if( type == TOK_STRING )
            {
                //found the end of a string
                if( stringType == TOK_DOUBLEQUOTE )
                {
                    // skip the closing quote
                    // assume quote is 1 byte char
                    m_ichFilePtr++;
                    m_hpFilePtr++;
                    if( m_fExcel )
                    {
                        //skip the extraneous 2 quotes
                        m_ichFilePtr += 2;
                        m_hpFilePtr += 2;
                    }
                    goto LEndOfToken;
                }
            }
            else
            {
                //found the beginning of a string, i.e. end of current token
                goto LEndOfToken;
            }
        }
        else if( type == TOK_STRING && hpFirst[cchUsed] == Keywords[(UINT)TOK_NEWLINE].pszKeyword[0] )
        {
            //hit end of line during a string; terminate the string
            goto LEndOfToken;
        }
        else if( type != TOK_STRING )   //we always add tokens to strings
        {
            //See if this character is a "single" separator.
            for( iToken = FIRST_SINGLE_TOKEN; iToken <= LAST_SINGLE_TOKEN; iToken++ )
            {
                //found a separator, finish the token
                if( hpFirst[cchUsed] == Keywords[iToken].pszKeyword[0] )
                    goto LEndOfToken;
            }
        }
        //Just a normal character, it gets automatically added by AnsiNext
        hpTmp = AnsiNext( hpCur );
        cchUsed += hpTmp - hpCur;
    }
LEndOfToken:

    // proceed file pointers
    m_hpFilePtr += cchUsed;
    m_ichFilePtr += cchUsed;

    //m_hpFilePtr and m_ichFilePtr should be pointing to where the next token
    //will start (not necessarily immediately following the current token).

    //hpFirst should be pointing to the beginning of the current token.
    //cchUsed should be the number of characters to use in the current token.

    //Save the character following the string and zero it out
    if( m_ichFilePtr < m_cchFileData )
    {
        //We don't necessarily save the save character that we zero out -
        //we just save what we know we'll try to replace next time around.
        m_chT = m_hpFilePtr[0];
        hpFirst[cchUsed] = 0;
    }

    //Try to identify the type of token
    if( type == TOK_EOF )   //i.e. type has not yet been determined
    {
        for( iToken = FIRST_KEYWORD_TOKEN; iToken <= LAST_KEYWORD_TOKEN; iToken++ )
        {
            if( !lstrcmpi( Keywords[iToken].pszKeyword, hpFirst ) )
            {
                type = (ETOKEN)iToken;
                break;
            }
        }
    }

    if( type == TOK_EOF )   //we *still* don't know what kind of token this is!
    {
        DWORD i;

        //if it's all digits, it's a number; else it's a string
        // assume every digits char is 1 byte char
        //  no need to use AnsiNext ?
        for( i = (fNegative)? 1 : 0; i < cchUsed; i++ )
        {
            if( !isdigit( hpFirst[i] ) )
                break;
        }

        type = (i < cchUsed) ? TOK_STRING : TOK_NUMBER;
    }

    //check if what we thought might be a negative number was actually just
    //a hyphen
    if( fNegative && type != TOK_NUMBER )
    {
        type = TOK_HYPHEN;
        hpFirst[cchUsed] = m_chT;   //restore the character we nuked earlier

        // assume hyphen is 1 byte char
        //   no need to use AnsiNext ?
        m_ichFilePtr -= cchUsed - 1;
        m_hpFilePtr = hpFirst+1;

        // assume hyphen is 1 byte char
        //   no need to use AnsiNext ?
        m_chT = hpFirst[1];
        hpFirst[1] = 0;
    }

    pToken->eToken = type;
    pToken->pszValue = (LPCSTR)hpFirst;

    return TRUE;
}

BOOL CTextDocument::OnAddToken( const CToken& token )
{
    return TRUE;
}
