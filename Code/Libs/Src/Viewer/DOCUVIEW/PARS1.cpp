//--------------------------------------------------------------------------//
//                                                                          //
//                                PARS1.cpp                                 //
//                                                                          //
//                  COPYRIGHT (C) 2002 Fever Pitch Studios, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*

    $Author: Ajackson $
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
#include "filestrm.h"
#include "lxtables.h"
#include "symtable.h"
#include "ferror.h"
#include "parserrs.h"
#include "fdump.h"

#include <malloc.h>

#ifndef __max
#	define __max(x,y) ((x > y)? x : y)
#endif

SymbolManager SymbolTable;

TOKEN gettoken(void);
extern int lineno, EOFFLG;
extern FileStream stream;

TOKEN savedtoken;

#define DEBUG         0             /* set bits here for debugging, 0 = off  */

SYMBOL datatype_declaration (bool & bNewType, int & modifier);
SYMBOL parse_type_definitions (void);
SYMBOL find_type_in_namespace (SYMBOL name);


//----------------------------------------------------------------
//---------------------------------------------------------------------
//
TOKEN gettok(void)       /* Get the next token; works with peektok. */
{  
	TOKEN tok;
    if (savedtoken != NULL)
    { 
		tok = savedtoken;
		savedtoken = NULL; 
	}
    else 
		tok = gettoken();

	if (EOFFLG)
	{
		FDUMP(ErrorCode(ERR_PARSER, SEV_FATAL), "Unexpected EOF reached.");
	}
	return(tok);
}
//---------------------------------------------------------------------
//
TOKEN peektok(void)       /* Peek at the next token */
{ 
	if (savedtoken == NULL)
    	savedtoken = gettoken();
	return(savedtoken);
}
//--------------------------------------------------------------------------
//
static SYMBOL get_type (SYMBOL sym)
{
	switch (sym->kind)
	{
	case TYPESYM:
	case VARSYM:
		if (sym->datatype)
			return get_type(sym->datatype);
	}

	return sym;
}
//---------------------------------------------------------------------
//
struct TALLYSTRUCT
{
	struct TALLYSTRUCT *pNext;
	SYMBOL sym;
};
//---------------------------------------------------------------------
//
int add_to_tally (TALLYSTRUCT **list, SYMBOL sym)
{
	int result = 0;
	TALLYSTRUCT *tlist;

	tlist = *list;
	while (tlist)
	{
		if (tlist->sym == sym)
			goto Done;
		tlist = tlist->pNext;
	}

	tlist = new TALLYSTRUCT;
	tlist->pNext = *list;
	*list = tlist;
	tlist->sym = sym;
	result++;
Done:
	return result;
}
//---------------------------------------------------------------------
//
int tally_circular_dependencies (SYMBOL sym)
{
	TALLYSTRUCT *list = 0, *tlist;
	SYMBOL tmp=sym->datatype, tmp2;
	int result=0;
	
	while (tmp)
	{
		if ((tmp2 = tmp->datatype) != 0)
		do
		{
			if (tmp2->datatype == sym)
			{
				result += add_to_tally(&list, tmp2);
				break;
			}
		} while ((tmp2 = tmp2->datatype) != 0);

		tmp = tmp->link;
	}

	while (list)
	{
		tlist = list->pNext;
		delete list;
		list = tlist;
	}

	return result;
}
//---------------------------------------------------------------------
//
void find_next_semicolon(void)
{
 	TOKEN tok=0;

	do
	{
		if (EOFFLG)
			break;
		free(tok);
		tok = gettok();
	} while (tok->tokentype != DELIMITER || tok->whichval != SEMICOLON-DELIMITER_BIAS);

	free(tok);
}
//------------------------------------------------------------------------------//
//
SYMBOL inheritance_element (SYMBOL dataType, int * currentOffset, int & currentAlignment)
{
	SYMBOL result;

	// add anonymous member
	result = new Symbol;
	result->kind = VARSYM;
	result->size = dataType->size;
	result->alignment = dataType->alignment;
	result->datatype = dataType;
	if (currentOffset)
	{
		result->offset = (*currentOffset + result->alignment - 1) & ~(result->alignment-1);
		*currentOffset = (result->offset + result->size);
	}
	dataType->AddRef();

 	currentAlignment = __max(dataType->alignment, currentAlignment);

	return result;
}
//---------------------------------------------------------------------
// returns a list of annonymous VARSYMS
//
SYMBOL parse_inheritance (void)
{
	TOKEN tok = peektok();
	SYMBOL result=0, last=0;
	int currentOffset=0, currentAlignment=1;

	if (tok->tokentype == DELIMITER && tok->whichval == COLON-DELIMITER_BIAS)
	{
		free(gettok());		// eat the ':'

		tok = peektok();
		while (1)
		{
			if (EOFFLG)
				break;

			if (tok->tokentype == IDENTIFIERTOK)
			{
				SYMBOL tmp;

				if ((tmp = SymbolTable.get_symbol(tok->stringval)) == 0)
				{
					fatal(UNDEFINED_SYMBOL, 0, tok->stringval);
					free(gettok());
					tok = peektok();
					continue;
				}
				free(gettok());
				//
				// check for scoping operator
				//
				tok = peektok();
				if (tok->tokentype == OPERATOR && tok->whichval == SCOPEOP)		// scoping operator
				{
					free(gettok());
					if ((tmp = find_type_in_namespace(tmp)) == 0)
					{
						fatal(BAD_PARSE);
						return 0;
					}
				}
				if (tmp->datatype && get_type(tmp)->kind == RECORDSYM)
				{
					// add to the list
					tmp = inheritance_element(tmp, &currentOffset, currentAlignment);
					if (last==0)
					{
						last = result = tmp;
					}
					else
					{
						last->link = tmp;
						last = last->link;
					}
				}
			}
			else
			{
				if (tok->tokentype == DELIMITER)
				{
					if (tok->whichval == SEMICOLON-DELIMITER_BIAS || tok->whichval == LBRACE-DELIMITER_BIAS)
						break;	
				}
				free(gettok());
			}
			tok = peektok();
		}
	}
	return result;
}
//---------------------------------------------------------------------
//
void skip_to_closing_brace (void)
{
	TOKEN tok = gettok();

	while (1)
	{
		if (tok->tokentype == DELIMITER)
		{
			if (tok->whichval == LBRACE-DELIMITER_BIAS)
				skip_to_closing_brace();
			else
			if (tok->whichval == RBRACE-DELIMITER_BIAS)
				break;
		}
		free(tok);
		tok = gettok();
	}

	free(tok);
}
//---------------------------------------------------------------------
// assume that current token is '(' of the param list, skip the entire function definition
//
void skip_function_definition (void)
{
	TOKEN tok;

	tok = gettok();
	while (tok->tokentype != DELIMITER || tok->whichval != LBRACE-DELIMITER_BIAS)
	{
		if (tok->tokentype == DELIMITER && tok->whichval == SEMICOLON-DELIMITER_BIAS)
		{
			free(tok);
			return;			// just a function declaration
		}
		free(tok);
		tok = gettok();
	}

	free(tok);
	skip_to_closing_brace();
}
//---------------------------------------------------------------------
//
int reserved(TOKEN tok,int n)          /* Test for a reserved word */
{ 
	return ( tok->tokentype == RESERVED
	    && (tok->whichval + RESERVED_BIAS ) == n);
}
//---------------------------------------------------------------------
//
int typesym(SYMBOL sym)
{
	if (!sym ||
		 sym->kind == VARSYM ||
		 sym->kind == ENUMSYM ||
		 sym->kind == FUNCTIONSYM)
	{
		return 0;
	}

	return 1;
}
//-------------------------------------------------------------//
//
SYMBOL parse_array (SYMBOL dataType, int modifier)
{
	SYMBOL result=0, tmp;
	TOKEN tok = peektok();
	int arrayval;

	if (tok->tokentype == DELIMITER && tok->whichval == LBRACKET-DELIMITER_BIAS)
	{
		free(gettok());		// eat the '['
		tok = gettok();
		if (tok->tokentype != NUMBERTOK)
		{
			// is this a ']'  ([] case means same as [0])
			if (tok->tokentype != DELIMITER || tok->whichval != RBRACKET-DELIMITER_BIAS)
				fatal(NUMBER_EXPECTED);
			arrayval=0;
			free(tok); // free the invalid thing
		}
		else
		{
			arrayval = tok->intval;
			free(tok);
			free(gettok());	// eat the ']'
		}

		if ((tmp = parse_array(dataType, modifier)) != 0)
		{
			dataType = tmp;
			modifier = 0;		// modifier already assigned to array type
		}
		
		result = new Symbol;
		result->kind = ARRAYSYM;
		result->size = (dataType->size + dataType->alignment - 1) & ~(dataType->alignment - 1);
		result->size *= arrayval;
		result->alignment = dataType->alignment;
		result->arrayval = arrayval;
		result->datatype = dataType;
		result->modifiers = modifier;
		dataType->AddRef();
	}

	return result;
}
//-------------------------------------------------------------//
//
SYMBOL declaration_member (SYMBOL dataType, SYMKIND kind, int modifier)
{
 	TOKEN tok=gettok();
	SYMBOL result=0, tmp;
	
	while (tok->tokentype == OPERATOR && tok->whichval == TIMES-OPERATOR_BIAS)
	{
		tmp = new Symbol;
		tmp->kind = POINTERSYM;
		tmp->size = 4;
		tmp->alignment = 4;
		if (result == 0)
		{
			result = tmp;
			result->datatype = dataType;
			dataType->AddRef();
		}
		else
		{
			tmp->datatype = result;
			result->AddRef();
			result = tmp;
		}
		free(tok);
		tok = gettok();
	}

	if (tok->tokentype != IDENTIFIERTOK)
	{
	 	fatal(BAD_PARSE); 
		if (result)
			result->Release();
		free(tok);
		return 0;	
	}

	if ((tmp = SymbolTable.get_symbol(tok->stringval)) != 0)
	{
		//
		// if we got here, symbol had better be on another level
		//
		
		if (tmp->blocklevel == SymbolTable.get_current_level())
		{
			if (kind == TYPESYM)
			{
#if DA_ERROR_LEVEL >= __SEV_TRACE_5
				FDUMP(ErrorCode(ERR_PARSER, SEV_TRACE_5), "PARSER: [LINE %d] Ignoring duplicate typedef of symbol: \"%s\"\n", lineno, tok->stringval);
#endif
			}
			else
		 		fatal(DUPLICATE_SYMBOL, 0, tok->stringval); 
			if (result)
				result->Release();
			free(tok);
			return 0;	
		}
	}

	modifier |= dataType->modifiers;
	if ((tmp = parse_array((result)?result:dataType, modifier)) != 0)
	{
		result = tmp;
		modifier = 0;	// modifier was already assigned to array type
	}

	tmp = SymbolTable.create_symbol(tok->stringval);
	tmp->kind = kind;
	tmp->size = (result)?result->size:dataType->size;
	tmp->alignment = (result)?result->alignment:dataType->alignment;
	tmp->modifiers = modifier;

	if (result == 0)
	{
		result = tmp;
		result->datatype = dataType;
		dataType->AddRef();
	}
	else
	{
		tmp->datatype = result;
	 	result->AddRef();
		result = tmp;
	}

	free(tok);

	return result;
}
//-------------------------------------------------------------//
//
SYMBOL type_declarators (SYMBOL dataType, int modifier)
{
	TOKEN tok=peektok();
	SYMBOL result = 0, tmp, last=0;

	while (tok->tokentype != DELIMITER || tok->whichval != SEMICOLON-DELIMITER_BIAS)
	{
		if (tok->tokentype == DELIMITER && tok->whichval == LPAREN-DELIMITER_BIAS)	// function call!
		{
			// remove items from list, returned to caller
			while (result)
			{
				tmp = result->link;
				result->Release();
				result = tmp;
			}
			return 0;
		}

		if ((tmp = declaration_member(dataType, TYPESYM, modifier)) != 0)
		{
			if (result==0)
				result = last = tmp;
			else
			{
				last->link = tmp;
				last = tmp;
				tmp->AddRef();
			}
		}

		tok=peektok();

		if (tok->tokentype == DELIMITER && tok->whichval == COMMA-DELIMITER_BIAS)
		{
			free(gettok());		// eat the comma
			tok = peektok();
		}
	}


	return result;
}
//-------------------------------------------------------------//
//
SYMBOL var_declaratators (SYMBOL dataType, int modifier, int * currentOffset, int * extraBits)
{
	TOKEN tok=peektok();
	SYMBOL result = 0, tmp, last=0;
	int bitwidth;
	int cO=0, eB=0;

	if (currentOffset == 0)		// parsing a union
		currentOffset = &cO;
	if (extraBits == 0)
		extraBits = &eB;

	while (tok->tokentype != DELIMITER || tok->whichval != SEMICOLON-DELIMITER_BIAS)
	{
		if (tok->tokentype == DELIMITER && tok->whichval == LPAREN-DELIMITER_BIAS)	// function call!
		{
			// remove items from list, returned to caller
			while (result)
			{
				tmp = result->link;
				result->Release();
				result = tmp;
			}
			return 0;
		}

		tmp = declaration_member(dataType, VARSYM, modifier);

		tok = peektok();
		if (tmp == 0 || tok->tokentype != DELIMITER || tok->whichval == LPAREN-DELIMITER_BIAS)	// function call!
		{
			// remove items from list, returned to caller
			// note: probably should remove the 'tmp' symbol from the SymbolTable, 
			// but there is no remove method supplied by the SymbolTable manager
			while (result)
			{
				tmp = result->link;
				result->Release();
				result = tmp;
			}
			return 0;
		}

		bitwidth = 0;
		
		if (tok->tokentype == DELIMITER && tok->whichval == COLON-DELIMITER_BIAS)	// bit field
		{
			SYMBOL local;

			free(gettok());			// eat the ':'
			tok = gettok();			// read the number

			if (tok->tokentype == IDENTIFIERTOK && (local = SymbolTable.get_symbol(tok->stringval)) != 0 && local->kind == CONSTSYM)
				bitwidth = local->constval.intnum;
			else
				bitwidth = tok->intval;

			free(tok);
			tok = peektok();
		}
		
		if (bitwidth == 0)
		{
			tmp->offset = (*currentOffset + tmp->alignment - 1) & ~(tmp->alignment-1);
			*currentOffset = (tmp->offset + tmp->size);
			*extraBits = 0;
		}
		else
		{
			tmp->offset = (*currentOffset + tmp->alignment - 1) & ~(tmp->alignment-1);
			if (tmp->offset != *currentOffset)	// not aligned
			{
				fatal(NONALIGNED_BITFIELD, 1);
				*extraBits = 0;
				*currentOffset = tmp->offset;
			}
			// VC++ doesn't allow a bitfield to spill into the next element
			if (*extraBits && bitwidth > *extraBits)
			{
				fatal(BITFIELD_OVERFLOW, 1);
				*extraBits = 0;
				*currentOffset = tmp->offset;
			}

			if (*extraBits)
				tmp->offset -= tmp->size;		// use left-over bits
			else
			{
				*extraBits += (tmp->size * 8);
				*currentOffset += tmp->size;
			}

			if (bitwidth > *extraBits)
			{
				fatal(BITFIELD_TOO_LARGE);		// bitfield exceeds size of the basic type
				bitwidth = *extraBits;
			}

			tmp->bit_offset = ((tmp->size * 8) - *extraBits) & ((tmp->size * 8)-1);
			tmp->bit_count  = bitwidth;

			*extraBits -= bitwidth;
		}
		if (result==0)
			result = last = tmp;
		else
		{
			last->link = tmp;
			last = tmp;
			tmp->AddRef();
		}

		tok=peektok();

		if (tok->tokentype == DELIMITER && tok->whichval == COMMA-DELIMITER_BIAS)
		{
			free(gettok());		// eat the comma
			tok = peektok();
		}
	}

	return result;
}
//------------------------------------------------------------------------------//
//
SYMBOL member_element (int * currentOffset, int * extraBits, int & currentAlignment, int prevTypeSize)
{
	SYMBOL result, dataType;
	bool bNewType;
	int modifier = 0;

	if ((dataType = datatype_declaration(bNewType, modifier)) == 0)
		return 0;
	if (extraBits && prevTypeSize && dataType->size != prevTypeSize)
		*extraBits = 0;
	
	result = var_declaratators(dataType, modifier, currentOffset, extraBits);

	TOKEN tok = peektok();
	if (tok->tokentype != DELIMITER || tok->whichval == LPAREN-DELIMITER_BIAS)	// function call!
	{
		skip_function_definition();
		return 0;
	}

	free(gettok());		// absorb the ';'

	// check for anonymous case
	if (result == 0 && (bNewType==0 || dataType->name == 0))
	{
		result = new Symbol;
		result->kind = VARSYM;
		result->size = dataType->size;
		result->alignment = dataType->alignment;
		result->datatype = dataType;
		if (currentOffset)
		{
			result->offset = (*currentOffset + result->alignment - 1) & ~(result->alignment-1);
			*currentOffset = (result->offset + result->size);
		}
		dataType->AddRef();
	}

	if (result)
		currentAlignment = __max(dataType->alignment, currentAlignment);

	return result;
}
//------------------------------------------------------------------------------//
//
SYMBOL member_list (SYMBOL dataType, int * currentOffset, int * extraBits, SYMBOL inheritance = 0)
{
	TOKEN tok;
	SYMBOL result = 0, tmp, last=0;
	int currentAlignment=1;
	int typeSize=0;

	SymbolTable.inc_current_level();

	free(gettok());	// absorb the '{'
	tok = peektok();

	if (inheritance)
	{
		result = last = inheritance;

		while (1)
		{
			currentAlignment = __max(last->datatype->alignment, currentAlignment);

			if (last->link == 0)
				break;
			last = last->link;
		}

		if (currentOffset)
			*currentOffset = (last->offset + last->size);
	}

	while (tok->tokentype != DELIMITER || tok->whichval != RBRACE-DELIMITER_BIAS)
	{
		if ((tmp = member_element(currentOffset, extraBits, currentAlignment, typeSize)) != 0)
		{
			typeSize = tmp->size;
			if (currentOffset == 0)	// if this is a union
				dataType->size = __max(dataType->size, tmp->size);
			
			if (result==0)
				result = last = tmp;
			else
			{
				last->link = tmp;
				last = tmp;
				tmp->AddRef();
			}
			while (last->link)
				last = last->link;
		}

		tok=peektok();
	}

	free(gettok());		// absorb the '}'

	if ((dataType->datatype = result) != 0)
		result->AddRef();
	
	if (currentOffset)	// if this is a struct
		dataType->size = (*currentOffset + currentAlignment - 1) & ~(currentAlignment - 1);
	dataType->alignment = currentAlignment;

	dataType->internal_refs = tally_circular_dependencies(dataType);

	SymbolTable.dec_current_level();

	return result;
}
//------------------------------------------------------------------------------//
//
SYMBOL enum_member_element (int & enumVal)
{
	TOKEN tok;
	SYMBOL result = 0;

	tok = gettok();

	if (tok->tokentype == IDENTIFIERTOK)
	{
		if (SymbolTable.get_symbol(tok->stringval) == 0)
		{
			result = SymbolTable.create_symbol(tok->stringval);
			result->kind = CONSTSYM;
			result->size = result->alignment = sizeof(result->kind);

			free(tok);
			tok = peektok();
			if (tok->tokentype == OPERATOR && tok->whichval == ASSIGN-OPERATOR_BIAS)
			{
				bool bNegative = false;
				free(gettok());			// eat the '='
				tok = gettok();
				if (tok->tokentype == OPERATOR && tok->whichval == MINUS-OPERATOR_BIAS)
				{
					free(tok);
					tok = gettok();
					bNegative = true;	// negative number
				}
				if (tok->tokentype == NUMBERTOK)
				{
					result->constval.intnum =  tok->intval;
					if (bNegative)
						result->constval.intnum = -result->constval.intnum;
					enumVal = result->constval.intnum + 1;
					free(tok);
					tok = 0;
				}
				else
				if (tok->tokentype == IDENTIFIERTOK)
				{
					SYMBOL tmp;
					if ((tmp = SymbolTable.get_symbol(tok->stringval)) != 0)
					{
						if (tmp->kind == CONSTSYM)
						{
							result->constval.intnum =  tmp->constval.intnum;
							if (bNegative)
								result->constval.intnum = -result->constval.intnum;
							enumVal = result->constval.intnum + 1;
						}
					}
					free(tok);
					tok = 0;
				}

			}
			else
				result->constval.intnum =  enumVal++;

			tok = 0;
		}
	}


	free(tok);

	return result;
}
//------------------------------------------------------------------------------//
//
SYMBOL enum_member_list (SYMBOL dataType)
{
	TOKEN tok;
	SYMBOL result = 0, tmp, last=0;
	int enumVal = 0;

	SymbolTable.inc_current_level();

	free(gettok());	// absorb the '{'
	tok = peektok();

	while (tok->tokentype != DELIMITER || tok->whichval != RBRACE-DELIMITER_BIAS)
	{
		if ((tmp = enum_member_element(enumVal)) != 0)
		{
			if (result==0)
				result = last = tmp;
			else
			{
				last->link = tmp;
				last = tmp;
				tmp->AddRef();
			}
			while (last->link)
				last = last->link;
		}

		tok=peektok();
		if (tok->tokentype == DELIMITER && tok->whichval == COMMA-DELIMITER_BIAS)
		{
			free(gettok());		// eat the ','
			tok = peektok();
		}
	}

	free(gettok());		// absorb the '}'

	if ((dataType->datatype = result) != 0)
		result->AddRef();
	dataType->size = dataType->alignment = sizeof(dataType->kind);
	
	SymbolTable.dec_current_level();

	return result;
}
//------------------------------------------------------------------------------//
//  struct/class keyword already parsed
//
SYMBOL parse_struct (bool & bNewType, int modifiers)
{
	SYMBOL result;
	TOKEN tok;
	// see if there is a tag; if so, is this defined already?

	tok = peektok();

	if (tok->tokentype == IDENTIFIERTOK)
	{
		gettok();
		if ((result = SymbolTable.get_symbol(tok->stringval)) == 0 || result->datatype == 0)
		{
			if (result == 0)
				result = SymbolTable.create_symbol(tok->stringval);
			free(tok);
			SYMBOL inheritance = parse_inheritance();

			tok = peektok();
			result->kind = RECORDSYM;
			if (tok->tokentype == DELIMITER && tok->whichval == LBRACE-DELIMITER_BIAS)
			{
				int currentOffset = 0, extraBits = 0;
				member_list(result, & currentOffset, & extraBits, inheritance);
				result->modifiers = modifiers;
				bNewType=1;
				return result;
			}
			// else forward reference
			return result;
		}
		else
		{
			free(tok);
			return result;
		}
	}
	else // no tag
	{
		int currentOffset = 0, extraBits=0;

		result = new Symbol;
		result->kind = RECORDSYM;
		result->modifiers = modifiers;

		member_list(result, & currentOffset, & extraBits);
		return result;
	}
}
//------------------------------------------------------------------------------//
//  union keyword already parsed
//
SYMBOL parse_union (bool & bNewType, int modifiers)
{
	SYMBOL result;
	TOKEN tok;
	// see if there is a tag; if so, is this defined already?

	tok = peektok();

	if (tok->tokentype == IDENTIFIERTOK)
	{
		gettok();
		if ((result = SymbolTable.get_symbol(tok->stringval)) == 0 || result->datatype == 0)
		{
			if (result == 0)
				result = SymbolTable.create_symbol(tok->stringval);
			free(tok);
			tok = peektok();
			result->kind = RECORDSYM;
			if (tok->tokentype == DELIMITER && tok->whichval == LBRACE-DELIMITER_BIAS)
			{
				member_list(result, 0, 0);
				result->modifiers = modifiers;
				bNewType=1;
				return result;
			}
			// else forward reference
			return result;
		}
		else
		{
			free(tok);
			return result;
		}
	}
	else // no tag
	{
		result = new Symbol;
		result->kind = RECORDSYM;
		result->modifiers = modifiers;

		member_list(result, 0, 0);
		return result;
	}
}
//------------------------------------------------------------------------------//
//  enum keyword already parsed
//
SYMBOL parse_enum (bool & bNewType, int modifiers)
{
	SYMBOL result;
	TOKEN tok;
	// see if there is a tag; if so, is this defined already?

	tok = peektok();

	if (tok->tokentype == IDENTIFIERTOK)
	{
		gettok();
		if ((result = SymbolTable.get_symbol(tok->stringval)) == 0 || result->datatype == 0)
		{
			if (result == 0)
				result = SymbolTable.create_symbol(tok->stringval);
			free(tok);
			tok = peektok();
			result->kind = ENUMSYM;
			if (tok->tokentype == DELIMITER && tok->whichval == LBRACE-DELIMITER_BIAS)
			{
				enum_member_list(result);
				result->modifiers = modifiers;
				bNewType=1;
				return result;
			}
			// else forward reference
			return result;
		}
		else
		{
			free(tok);
			return result;
		}
	}
	else // no tag
	{
		result = new Symbol;
		result->kind = ENUMSYM;
		result->modifiers = modifiers;

		enum_member_list(result);
		return result;
	}
}
//------------------------------------------------------------------------------//
// namespace keyword already parsed
//
SYMBOL parse_namespace (bool & bNewType)
{
	SYMBOL result;
	TOKEN tok;
	// see if there is a tag; if so, is this defined already?

	tok = peektok();
	bNewType = 0;

	if (tok->tokentype == IDENTIFIERTOK)
	{
		gettok();
		if ((result = SymbolTable.get_symbol(tok->stringval)) == 0)
		{
			result = SymbolTable.create_symbol(tok->stringval);
			bNewType = 1;
		}

		free(tok);
		tok = peektok();
		result->kind = NAMESYM;
		if (tok->tokentype == DELIMITER && tok->whichval == LBRACE-DELIMITER_BIAS)
		{
			gettok();		// eat the '{'
			free(tok);

			SymbolTable.push_namespace(result);

			SYMBOL tmp, newtype = parse_type_definitions();

			tok = peektok();
			if (tok->tokentype == DELIMITER && tok->whichval == RBRACE-DELIMITER_BIAS)
			{
				gettok();		// eat the '}'
				free(tok);
			}

			if ((tmp = result->datatype) != 0)	// if we have parsed this datatype before
			{
				// stick new parsed data at end of list
				while (tmp->link)
					tmp = tmp->link;
				if ((tmp->link = newtype) != 0)
					newtype->AddRef();
			}
			else
			{
				result->datatype = newtype;
				newtype->AddRef();
			}

			SymbolTable.pop_namespace(result);

			return result;
		}
		// else forward reference
		return result;
	}
	else // no tag
	{
		fatal(DEFAULT_ERROR);
		return 0;
	}
}
//------------------------------------------------------------------------------//
//
SYMBOL find_type_in_namespace (SYMBOL name)
{
	SYMBOL result=0;
	TOKEN tok = gettok();
	
	if (tok->tokentype != IDENTIFIERTOK)
		goto Done;	// error

	while (name->kind == TYPESYM)
		name = name->datatype;

	if (name->kind == RECORDSYM)
	{
		result = name->datatype;
		// find the name list of variables in record
		while (result)
		{
			if (result->datatype->name && result->datatype->name==tok->stringval)
				break;
			result = result->link;
		}

	}
	else
	if (name->kind == NAMESYM)
	{
		result = name->datatype;
		// result is now a list of types

		while (result)
		{
			if (result->name && result->name==tok->stringval)
				break;
			result = result->link;
		}
	}

Done:
	free(tok);
	return result;
}
//------------------------------------------------------------------------------//
//
SYMBOL datatype_declaration (bool & bNewType, int & modifier)
{
	SYMBOL result = 0, tmp;
	TOKEN tok=gettok();
 
	bNewType=0;

	if (tok->tokentype == DELIMITER)
	{
		if (tok->whichval == SEMICOLON-DELIMITER_BIAS)
		{
			free(tok);
			return 0;
		}
		fatal(BAD_PARSE);
		goto Error;
	}
	else
	if (tok->tokentype == IDENTIFIERTOK)
	{
		if ((result = SymbolTable.get_symbol(tok->stringval)) == 0)
		{
			fatal(UNDEFINED_SYMBOL, 0, tok->stringval);
			goto Error;
		}
		free(tok);
		//
		// check for scoping operator
		//
		tok = peektok();
		if (tok->tokentype == OPERATOR && tok->whichval == SCOPEOP)		// scoping operator
		{
			free(gettok());
			if ((result = find_type_in_namespace(result)) == 0)
			{
				fatal(BAD_PARSE);
				goto Error;
			}
		}
		
		return result;		
	}
	else
	if (tok->tokentype == RESERVED)
	{
		switch (tok->whichval)
		{
		case _TYPEDEF-RESERVED_BIAS:
			free(tok);
			if ((tmp = datatype_declaration(bNewType, modifier)) != 0)
			{
				type_declarators(tmp, modifier);
				tok = peektok();
				if (tok->tokentype == DELIMITER && tok->whichval == LPAREN-DELIMITER_BIAS)	// function call!
					skip_function_definition();
				tok = peektok();
				if (tok->tokentype == DELIMITER && tok->whichval == SEMICOLON-DELIMITER_BIAS)	
					free(gettok());		// eat the ';'
			}
			modifier=0;
			return datatype_declaration(bNewType, modifier);

		case _HEXVIEW-RESERVED_BIAS:
			modifier |= MODIFIER_HEXVIEW;
			free(tok);
			return datatype_declaration(bNewType, modifier);

		case _SPELLCHECK-RESERVED_BIAS:
			modifier |= MODIFIER_SPELLCHECK;
			free(tok);
			return datatype_declaration(bNewType, modifier);

		case _FILENAMECHECK-RESERVED_BIAS:
			modifier |= MODIFIER_FILENAME;
			free(tok);
			return datatype_declaration(bNewType, modifier);

		case _COLORCHECK-RESERVED_BIAS:
			modifier |= MODIFIER_COLOR;
			free(tok);
			return datatype_declaration(bNewType, modifier);

		case _READONLY-RESERVED_BIAS:
			modifier |= MODIFIER_READONLY;
			free(tok);
			return datatype_declaration(bNewType, modifier);
		
		case _UNSIGNED-RESERVED_BIAS:
			modifier |= MODIFIER_UNSIGNED;

			// fall through intentional

		case _SIGNED-RESERVED_BIAS:
			free(tok);
			tok = peektok();
			if (tok->tokentype == IDENTIFIERTOK)
			{
				if ((result = SymbolTable.get_symbol(tok->stringval)) == 0)		// assume "int" type
					return SymbolTable.get_symbol("int");
			}
			return datatype_declaration(bNewType, modifier);

		case _PUBLIC-RESERVED_BIAS:
		case _PRIVATE-RESERVED_BIAS:
		case _PROTECTED-RESERVED_BIAS:
			free(tok);
			free(gettok());		// eat the ':'
			return datatype_declaration(bNewType, modifier);

		case _CLASS-RESERVED_BIAS:
		case _STRUCT-RESERVED_BIAS:
			free(tok);
			return parse_struct(bNewType, modifier);

		case _UNION-RESERVED_BIAS:
			free(tok);
			return parse_union(bNewType, modifier);

		case _CONST-RESERVED_BIAS:
			free(tok);
			return datatype_declaration(bNewType, modifier);

		case _ENUM-RESERVED_BIAS:
			free(tok);
			return parse_enum(bNewType, modifier);
		}
	}

Error:
	free(tok);
	find_next_semicolon();
	return 0;
}
//------------------------------------------------------------------------------//
//
SYMBOL parse_type_definitions (void)
{
	bool bNewType;
	SYMBOL result = 0, tmp, last=0;
	int currentOffset, modifier, extraBits;
	TOKEN tok=peektok();

	while (EOFFLG == 0)
	{
		currentOffset = modifier = extraBits = 0;

		if (tok->tokentype == DELIMITER && tok->whichval == RBRACE-DELIMITER_BIAS)
			break;
		else
		if (tok->tokentype == RESERVED && tok->whichval == _TYPEDEF-RESERVED_BIAS)
		{
			free(gettok());
			if ((tmp = datatype_declaration(bNewType, modifier)) != 0)
			{
				if ((tmp = type_declarators(tmp, modifier)) != 0)
				{
					if (result == 0)
						result = last = tmp;
					else
					{
						last->link = tmp;
						last = tmp;
						tmp->AddRef();
					}
					while (last->link)
						last = last->link;
				}
				else
				{
					tok = peektok();
					if (tok->tokentype == DELIMITER && tok->whichval == LPAREN-DELIMITER_BIAS)	// function call!
						skip_function_definition();
				}
				tok = peektok();
				if (tok->tokentype == DELIMITER && tok->whichval == SEMICOLON-DELIMITER_BIAS)	
					free(gettok());		// eat the ';'
			}
		}
		else
		if (tok->tokentype == RESERVED && tok->whichval == _NAMESPACE-RESERVED_BIAS)
		{
			free(gettok());
			tmp = parse_namespace(bNewType);
			if (bNewType && tmp)
			{
				if (result == 0)
					result = last = tmp;
				else
				{
					last->link = tmp;
					last = tmp;
					tmp->AddRef();
				}
			}
		}
		else
		if ((tmp = datatype_declaration(bNewType, modifier)) != 0)
		{
			var_declaratators(tmp, modifier, & currentOffset, & extraBits);		// eat the variables

			tok = peektok();
			if (bNewType)
			{
				if (tok->tokentype != DELIMITER || tok->whichval == LPAREN-DELIMITER_BIAS)	// function call!
				{
					skip_function_definition();
				}
				else
				{
					if (result == 0)
						result = last = tmp;
					else
					{
						last->link = tmp;
						last = tmp;
						tmp->AddRef();
					}
				}
			}
			tok = peektok();
			if (tok->tokentype == DELIMITER && tok->whichval == SEMICOLON-DELIMITER_BIAS)	
				free(gettok());		// eat the ';'
		}
		tok = peektok();		// are we at the end of the file?
	}

	return result;
}

extern int g_availableISpace;		// available identifier space
extern int g_totalISpace;

//------------------------------------------------------------------------------//
//
static SYMBOL _parse (void)
{
	SYMBOL list;
	
	lineno = 1;
	EOFFLG = 0;
	savedtoken = NULL;
	InitTables();

#if DA_ERROR_LEVEL >= __SEV_TRACE_2
	FDUMP(ErrorCode(ERR_PARSER, SEV_TRACE_2), "Started Parser.\n");
#endif

	list = parse_type_definitions();

	free(savedtoken);
	savedtoken=0;

	if (list)
		list->AddRef();

#if DA_ERROR_LEVEL >= __SEV_TRACE_2
	FDUMP(ErrorCode(ERR_PARSER, SEV_TRACE_2), "Parse Complete. %d of %d bytes available identifier space.\n", g_availableISpace, g_totalISpace);
#endif

	return list;
}
//------------------------------------------------------------------------------//
//
SYMBOL parse_file (const char *name)
{
	SYMBOL list;
	if (stream.open(name) == 0)
	{
#if DA_ERROR_LEVEL >= __SEV_ERROR
		FDUMP(ErrorCode(ERR_PARSER, SEV_ERROR), "Parser::Could not open %s\n.", name);
#endif
		return 0;
	}
	
	list = _parse();

	stream.close();	

	return list;
}
//------------------------------------------------------------------------------//
//
SYMBOL parse_memory (const char *memory)
{
	SYMBOL list;

	stream.buffer = (const unsigned char *) memory;
	stream.buffer_index = 0;
	stream.bOwnMemory = false;

	list = _parse();

	stream.buffer = 0;

	return list;
}

//--------------------------------------------------------------------------//
//
#if defined(_XBOX)

#include "HeapObj.h"

extern HINSTANCE hInstance;
extern ICOManager * DACOM;

void RegisterViewConstructor (ICOManager * DACOM);
void RegisterDocument (ICOManager * DACOM);
void RegisterStringSet (ICOManager * DACOM);

BOOL WINAPI DllMain_Parser(HINSTANCE hinstance, DWORD fdwReason, LPVOID lpvReserved)
{
	switch (fdwReason)
	{
		case DLL_PROCESS_ATTACH:
		{
			hInstance = hinstance;
			HEAP = HEAP_Acquire();
			
			if ((DACOM = DACOM_Acquire()) != 0)
			{
				RegisterViewConstructor(DACOM);
				RegisterDocument(DACOM);
				RegisterStringSet(DACOM);
			}
		}
	}

	return 1;
}
#endif
//--------------------------------------------------------------------------//
//--------------------------END Pars1.cpp-----------------------------------//
//--------------------------------------------------------------------------//
