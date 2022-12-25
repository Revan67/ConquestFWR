//--------------------------------------------------------------------------//
//                                                                          //
//                               Scanner.cpp                                //
//                                                                          //
//                  COPYRIGHT (C) 2002 Fever Pitch Studios, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*

    $Author: Tmauer $
*/			    
//--------------------------------------------------------------------------//

// 4514: unused inline function
// 4201: nonstandard no-name structure use
// 4100: formal parameter was not used
// 4512: assignment operator could not be generated
// 4245: conversion from signed to unsigned
// 4127: constant condition expression
// 4355: 'this' used in member initializer
// 4244: conversion from int to unsigned char, possible loss of data
// 4200: zero sized struct member
// 4710: inline function not expanded
// 4702: unreachable code
// 4786: truncating function name (255 chars) in browser info
#pragma warning (disable : 4514 4201 4100 4512 4245 4127 4355 4244 4710 4702 4786)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#pragma warning (disable : 4355 4201)

#include "FileSys.h"

#include "tokendef.h"
#include "lxtables.h"
#include "filestrm.h"
#include "ferror.h"
#include "prttoken.h"

#include <malloc.h>

#define DEBUGGETTOKEN 0  /* to print each token as it is parsed */

TOKEN gettok(void);       /* Get the next token; works with peektok. */
TOKEN peektok(void);       /* Get the next token; works with peektok. */
TOKEN gettoken(void);


int lineno, EOFFLG;
FileStream stream;

void testscanner (void)
  { TOKEN tok;
    while (EOFFLG == 0)
      {
      tok = gettoken();
      if (EOFFLG == 0) printtoken(tok);
      }
  }

//----------------------------------------------------------------
void skipblanks (void)     /* skip blanks, tab, newline, comments */
{
	UBYTE c, cc;

	while (1)
	{
		c = stream.peek_char();
		cc = stream.peek2_char();	

		if (c == '/' && cc == '*') // comment found
		{
			stream.get_char();
			stream.get_char();
			c = stream.peek_char();
			cc = stream.peek2_char();	

			while (c != '*' || cc != '\\')
			{
				if (c == '/' && cc == '/')		// comment found
				{
					stream.get_char();
					stream.get_char();
					c = stream.peek_char();

					while (c != '\n' && c)
					{
						stream.get_char();
						c = stream.peek_char();
					}
					
					if (!c)
					{
						fatal(UNEXPECTED_EOF);
						return;
					}
					lineno++;
					stream.get_char();	// skip the EOLN character
					c = stream.peek_char();
					cc = stream.peek2_char();
				}
		
				if (!c)
				{
					fatal(UNEXPECTED_EOF);
					return;
				}

				if (c == '/' && cc == '*') // comment found
					skipblanks();
				else
					stream.get_char();
				c = stream.peek_char();
				cc = stream.peek2_char();	
				if (c == '\n')
					lineno++;
			}

			if (!c)
			{
				fatal(UNEXPECTED_EOF);
				return;
			}

			stream.get_char();	// skip the comment characters
			stream.get_char();
		}
		else
		if (c == '/' && cc == '/')		// comment found
		{
			stream.get_char();
			stream.get_char();
			c = stream.peek_char();

			while (c != '\n' && c)
			{
				stream.get_char();
				c = stream.peek_char();
			}
			
			if (!c)
			{
				break;
			}
			lineno++;
			stream.get_char();	// skip the EOLN character
		}
		else  // search for first non-white space character
		{
			if (c == 0 || CHARCLASS[c] != NOTHING || c == '\"')
				break;
			if (c == '\n')
				lineno++;

			stream.get_char();	// skip the white space character
		}
	}
}
//--------------------------------------------------------------------------//
//
void skip_compiler_directive (void)
{
	int _lineno = lineno;	// save the current line number
	TOKEN tok;
	
	stream.get_char();		// eat the '#' character

	while (EOFFLG == 0)
	{
		tok = peektok();

		if (tok->tokentype == DELIMITER && tok->whichval == LCONTINUE - DELIMITER_BIAS)
			_lineno = lineno+1;
		else
		if (lineno > _lineno)
			break;

		free(gettok());		// eat the token
	}
}
//--------------------------------------------------------------------------//
/*
	int DetermineTokenType(char *name, int & ttype, int & which);
	INPUT:
		Null terminated string. For Pascal, name should be in all
		capital letters.
	RETURNS:
		ttype = the type of token: (OPERATOR, IDENTIFIERTOK, RESERVED)
		which = (if ttype == OPERATOR or RESERVED) which token value.
*/
//--------------------------------------------------------------------------//
//
void DetermineTokenType(char *name, int & ttype, int & which)
{
	char a = *name;
	char **compare;

	ttype = IDENTIFIERTOK;	// assume identifier
	// check for reserved words 
	compare = resprnt;
	while (*compare)
	{
		if (**compare == a && !strcmp(*compare, name))
		{
		 	ttype = RESERVED;
			which = compare - resprnt;
			return;
		}
		compare++;
	}
	// check for operators 
	compare = opprnt;
	while (*compare)
	{
		if (**compare == a && !strcmp(*compare, name))
		{
		 	ttype = OPERATOR;
			which = compare - opprnt;
			return;
		}
		compare++;
	}
}

//----------------------------------------------------------------
//
int findfirstname (const char *_bank, const char *name)
{
	const char *bank = _bank;
	int len;

	while (bank < name)
	{
		len = strlen(bank);
		if (strcmp(bank, name) == 0)
			break;
		bank += len+1;
	}

	return bank - _bank;
}
//----------------------------------------------------------------
// // _bank & name are counted strings
//
int findfirstname2 (const char *_bank, const char *name)
{
	const char *bank = _bank;
	const int nameLen = U8(*name++);
	int len = U8(*bank++);
	const char letter = *name;

	while (bank < name)
	{
		int diff = len-nameLen;
		if (diff>=0 && letter == bank[diff])
		{
			if (memcmp(bank+diff, name, nameLen) == 0)
			{
				bank+=diff;
				break;
			}
		}
		// no match, skip to the next string
		bank += len;
		len = U8(*bank++);
	}

	return bank - _bank;
}
//----------------------------------------------------------------
// find either an indentifier or a reserved token, or an operator
//
int g_availableISpace, g_totalISpace;

TOKEN identifier (TOKEN tok)
{
	#define MAX_TBANK_SIZE 0x20000
	static char char_bank[MAX_TBANK_SIZE];	
	static int  bank_index = 0;

	int i=0;
	UBYTE c;
	char error = 0;		// send WARNING if this becomes true

		// assume that we have an identifier
	tok->tokentype = IDENTIFIERTOK;
	tok->datatype = STRINGTYPE;
	tok->stringval = &char_bank[bank_index+1];
	
	c = stream.peek_char();	

	if (bank_index < MAX_TBANK_SIZE-1)
	{
		char_bank[i+bank_index] = 0;	// leave room for count byte
		i++;
	}
	
	while (CHARCLASS[c] == ALPHA || CHARCLASS[c] == NUMERIC || c == '_') 
	{
// C is case sensitive
//		c = CONVERTCASE[stream.get_char()];
		c = stream.get_char();

		if (bank_index+i < MAX_TBANK_SIZE-1)
		{
			if (i < 255)
			{
		 		char_bank[bank_index+i] = c;
				i++;
			}
		}
		else
		{
		 	error = 1;
		}

		c = stream.peek_char();	
	} // end while()

	char_bank[i+bank_index] = 0;
	if (error)
	{
	 	fatal(OUT_OF_TSTRINGS);
	}
	else
	{
		char_bank[bank_index] = i++;		// count byte = length of string + NULL byte
	}


#ifdef _DEBUG
	if (strcmp("__DEBUG", char_bank+bank_index+1) == 0)
	{
		skipblanks();     /* and comments */
		return 0;	// identifier(tok);	// get next token
	}
#endif
	DetermineTokenType(char_bank+bank_index+1, tok->tokentype, tok->whichval);
	if (tok->tokentype != IDENTIFIERTOK)
		tok->datatype = INTEGER;
	else
	{
		int j;

		if ((j = findfirstname2(char_bank, char_bank+bank_index)) == bank_index+1)
			bank_index += i;
		else
			tok->stringval = &char_bank[j];
	}

	g_availableISpace = MAX_TBANK_SIZE - bank_index;
	g_totalISpace = MAX_TBANK_SIZE;

	return tok;
}

//----------------------------------------------------------------
//
TOKEN getstring (TOKEN tok)
{
	#define MAX_BANK_SIZE 4096
	static char char_bank[MAX_BANK_SIZE];	
	static int  bank_index = 0;

	int i=0;
	UBYTE c,cc;
	char error = 0;		// send WARNING if this becomes true

	tok->tokentype = STRINGTOK;
	tok->datatype = STRINGTYPE;
	tok->stringval = &char_bank[bank_index];
	
	c = stream.get_char();	// get the \" character
	
	while (1) 
	{
		c = stream.get_char();
		cc = stream.peek_char();
		
		if (c == '\"')
		{
		 	if (c != cc)
				break;			// end of string
			stream.get_char();
		}
		else
		if (c == '\\')
		{
			c = cc;
			switch (c)
			{
				case 'n':
					c = '\n';
					break;
				case 't':
					c = '\t';
					break;
				case 'r':
					c = '\r';
					break;
			}
			stream.get_char();
		}
		if (!c || c == '\n')
		{
			fatal(BAD_STRING_FORMAT);
			if (c == '\n')
				lineno++;
			break;
		}
		
		if (bank_index+i < MAX_BANK_SIZE-1)
		{
		 	char_bank[bank_index+i] = c;
			i++;
		}
		else
		{
		 	error = 1;
		}
	} // end while()

	char_bank[i+bank_index] = 0;
	if (error)
	{
	 	fatal(OUT_OF_STRINGS);
	}
	else
		i++;

	{
		int j;

		if ((j = findfirstname(char_bank, char_bank+bank_index)) == bank_index)
			bank_index += i;
		else
			tok->stringval = &char_bank[j];
	}

	return tok;
}
//----------------------------------------------------------------
// find either an operator or a delimiter
//
TOKEN special (TOKEN tok)
{
	UBYTE c,cc;

	// tok->tokentype = STRINGTOK;
	tok->datatype = INTEGER;
	// tok->stringval = &char_bank[bank_index];

	c  = stream.get_char();
	cc = stream.peek_char();
	switch (c)
	{
		case '+':
			tok->tokentype = OPERATOR;
			tok->whichval = PLUSOP;
			break;
		case '-':
			if (cc == '>')	// pointer op
			{
				tok->tokentype = OPERATOR;
				tok->whichval = POINTEROP;
				stream.get_char();
			}
			else
			{
				tok->tokentype = OPERATOR;
				tok->whichval = MINUSOP;
			}
			break;
		case '*':
			tok->tokentype = OPERATOR;
			tok->whichval = TIMESOP;
			break;
		case '/':
			tok->tokentype = OPERATOR;
			tok->whichval = DIVIDEOP;
			break;
		case '=':
			if (cc == '=')	// equal op
			{
				tok->whichval = EQOP;
				stream.get_char();
			}
			else
				tok->whichval = ASSIGNOP;
			tok->tokentype = OPERATOR;
			break;
		case '^':
			tok->tokentype = OPERATOR;
			tok->whichval = XOROP;
			break;
		case '%':
			tok->tokentype = OPERATOR;
			tok->whichval = MODOP;
			break;
		case ':':
			if (cc == ':')	// SCOPE OPERATOR
			{
				tok->tokentype = OPERATOR;
				tok->whichval = SCOPEOP;
				stream.get_char();
			}
			else
			{
			 	tok->tokentype = DELIMITER;
				tok->whichval = COLON - DELIMITER_BIAS;
			}
			break;
		case '!':
			if (cc == '=')	// NOT EQUAL
			{
				tok->tokentype = OPERATOR;
				tok->whichval = NEOP;
				stream.get_char();
			}
			else	// NOT OPERATOR
			{
				tok->tokentype = OPERATOR;
				tok->whichval = NOTOP;
			}
			break;
		case '<':
			if (cc == '=')	// <= LE
			{
				tok->tokentype = OPERATOR;
				tok->whichval = LEOP;
				stream.get_char();
			}
			else
			{
				tok->tokentype = OPERATOR;
				tok->whichval = LTOP;
			}
			break;
		case '>':
			if (cc == '=')
			{
				tok->tokentype = OPERATOR;
				tok->whichval = GEOP;
				stream.get_char();
			}
			else
			{
				tok->tokentype = OPERATOR;
				tok->whichval = GTOP;
			}
			break;
		case '.':
			if (cc == '.')
			{
			 	tok->tokentype = DELIMITER;
				tok->whichval = DOTDOT - DELIMITER_BIAS;
				stream.get_char();
			}
			else
			{
				tok->tokentype = OPERATOR;
				tok->whichval = DOTOP;
			}
			break;
		case '&':
			if (cc == '&')		// logical and
			{
				tok->whichval = LOGANDOP;
				stream.get_char();
			}
			else
			{
				tok->whichval = ANDOP;
			}
			tok->tokentype = OPERATOR;
			break;
		case '|':
			if (cc == '|')		// logical and
			{
				tok->whichval = LOGOROP;
				stream.get_char();
			}
			else
			{
				tok->whichval = OROP;
			}
			tok->tokentype = OPERATOR;
			break;
		case ',':
			tok->tokentype = DELIMITER;
			tok->whichval = COMMA - DELIMITER_BIAS;
			break;
		case ';':
			tok->tokentype = DELIMITER;
			tok->whichval = SEMICOLON - DELIMITER_BIAS;
			break;
		case '(':
			tok->tokentype = DELIMITER;
			tok->whichval = LPAREN - DELIMITER_BIAS;
			break;
		case ')':
			tok->tokentype = DELIMITER;
			tok->whichval = RPAREN - DELIMITER_BIAS;
			break;
		case '[':
			tok->tokentype = DELIMITER;
			tok->whichval = LBRACKET - DELIMITER_BIAS;
			break;
		case ']':
			tok->tokentype = DELIMITER;
			tok->whichval = RBRACKET - DELIMITER_BIAS;
			break;
		case '{':
			tok->tokentype = DELIMITER;
			tok->whichval = LBRACE - DELIMITER_BIAS;
			break;
		case '}':
			tok->tokentype = DELIMITER;
			tok->whichval = RBRACE - DELIMITER_BIAS;
			break;
		case '\\':
			tok->tokentype = DELIMITER;
			tok->whichval = LCONTINUE - DELIMITER_BIAS;
			break;
		default:
			fatal(DEFAULT_ERROR);
			tok->tokentype = DELIMITER;
			tok->whichval = RBRACKET - DELIMITER_BIAS;
	} // end of monster switch
	
	return tok;		
}
//----------------------------------------------------------------
//----------------------------------------------------------------
TOKEN number (TOKEN tok);

/* To use this file:
   A function gettoken() is provided; it will work if you provide the
   functions it calls.

   To run by itself, use a main program as shown below. */

TOKEN gettoken()
{
	TOKEN tok;
	int c, cclass;

	tok = talloc();   /* allocate a token */
	skipblanks();     /* and comments */
Top:
	if ((c = stream.peek_char()) != 0)
	{
		cclass = CHARCLASS[c];
		if (cclass == ALPHA)
		{
			if (identifier(tok) == 0)
				goto Top;		// this only happens for debugging!
		}
		else if (cclass == NUMERIC)
			number(tok);
		else if (c == '\"')
			getstring(tok);
		else
		if (c == '#')
		{
			free(tok);
			skip_compiler_directive();
			return gettok();
		}
		else
			special(tok);
	}
	else 
		EOFFLG = 1;

	if (DEBUGGETTOKEN != 0) 
		printtoken(tok);

	return(tok);
}

//-------------------------------------------------------------//
//-------------------------------------------------------------//

#if 0
//-------------------------------------------------------------//
char banner[] = "Lexical Analyzer test\n";
char help_msg[] = "USAGE: lextest [filename]\n";
//-------------------------------------------------------------//

int main(int argc, char *argv[])
{
	printf(banner);
	if (argc < 2)
	{
		printf(help_msg);
		return 1;
	}

	stream.open(argv[1]);	

	lineno = 1;
	EOFFLG = 0;
	InitTables();
	printf("Started scanner test.\n");
	testscanner();
	return 0;
}
#endif

//--------------------------------------------------------------------------//
//--------------------------End Scanner.cpp---------------------------------//
//--------------------------------------------------------------------------//
