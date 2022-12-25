//--------------------------------------------------------------------------//
//                                                                          //
//                               PRTTOKEN.cpp                               //
//                                                                          //
//                  COPYRIGHT (C) 2002 Fever Pitch Studios, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*

    $Author: Tmauer $
*/			    
//--------------------------------------------------------------------------//

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "HeapObj.h"

#include <ctype.h>
#include <malloc.h>

#include "prttoken.h"
#include "fdump.h"

char* opprnt[]  = {" ", "+", "-", "*", "/", "=", "==", "!=", "<", "<=",
                          ">=", ">",  "->", ".", "&", "|", "!", "/",
                          "%", "^", "&&", "||", "::", 0};
char *delprnt[] = { "  ", " ,", " ;", " :", " (", " )", " [", " ]",
                           "..", " {", " }", " \\", 0} ;
char *resprnt[] = { " ", "struct", "class", "public", "private", "protected",
                         "typedef", "signed", "unsigned", "enum", "const",
						 "union", "namespace", "__readonly", "__hexview", 
						 "__spellcheck", "__filename", "__color", 0 };



//-----------------------------------------------------------------
//-----------------------------------------------------------------

TOKEN talloc()           /* allocate a new token record */
{
#if !defined(_XBOX)
	return (TOKEN) HEAP->ClearAllocateMemory(sizeof(tokn), "DataViewer::Token struct");
#else
	return (TOKEN) calloc( 1, sizeof(tokn) );
#endif
}
#if DA_ERROR_LEVEL >= __SEV_TRACE_5
void printtoken(TOKEN tok)
{
    switch (tok->tokentype)
	{
	case OPERATOR:
		FDUMP(ErrorCode(ERR_PARSER, SEV_TRACE_5), "OPERATOR: which: %4d   %10s\n", tok->whichval, opprnt[tok->whichval] );
		break;
	case DELIMITER:
		FDUMP(ErrorCode(ERR_PARSER, SEV_TRACE_5), "DELIMITER: which: %4d   %10s\n", tok->whichval, delprnt[tok->whichval] );
		break;
	case RESERVED:
		FDUMP(ErrorCode(ERR_PARSER, SEV_TRACE_5), "RESERVED: which: %4d   %10s\n", tok->whichval, resprnt[tok->whichval] );
		break;
	case IDENTIFIERTOK:
		FDUMP(ErrorCode(ERR_PARSER, SEV_TRACE_5), "IDENTIFIERTOK:  value:  %16s\n", tok->stringval);
		break;
	case STRINGTOK:
		FDUMP(ErrorCode(ERR_PARSER, SEV_TRACE_5), "STRINGTOK: value:  %16s\n", tok->stringval);
		break;
	case NUMBERTOK:
		switch (tok->datatype)
		{
		case INTEGER:
			FDUMP(ErrorCode(ERR_PARSER, SEV_TRACE_5), "NUMBERTOK: type: %4d, Value = %12d\n", tok->datatype, tok->intval);
			break;
		case REAL:
			FDUMP(ErrorCode(ERR_PARSER, SEV_TRACE_5), "NUMBERTOK: type: %4d %d.%d\n", tok->datatype, (long)tok->realval, long((tok->realval-long(tok->realval))*100.0));
			break;
		}
	}

}
#else
void printtoken(TOKEN )
{
}
#endif

//--------------------------------------------------------------------------//
//-------------------------End PRTToken.cpp---------------------------------//
//--------------------------------------------------------------------------//











