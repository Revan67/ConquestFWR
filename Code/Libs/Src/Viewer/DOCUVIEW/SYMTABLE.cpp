//--------------------------------------------------------------------------//
//                                                                          //
//                                SymTable.cpp                              //
//                                                                          //
//                  COPYRIGHT (C) 2002 Fever Pitch Studios, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*

    $Author: Tmauer $
*/			    
//--------------------------------------------------------------------------//
//------------------------------- #INCLUDES --------------------------------//

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

#include "symtable.h"
#include "tokendef.h"
#include "ferror.h"
#include "fdump.h"

#include "HeapObj.h"

//#include <memory.h>
//#include <string.h>
//#include <stdio.h>
#include <stdlib.h>

//--------------------------------------------------------------------------//
void printsymbol(SYMBOL node)
{
	// testing!!!

}

char *SymbolManager::search_name=0;
unsigned SymbolManager::search_hashval=0;

void * Symbol::operator new (size_t size)
{
	return HEAP->AllocateMemory(size, "DataViewer:Symbol struct");
}
//--------------------------------------------------------------------------//
//
unsigned long Symbol::Release (void)
{
	if (dwRefs > 0)
		dwRefs--;
	else
	{
		dwRefs=internal_refs;	// testing!!
	}
	if (dwRefs==internal_refs)
	{
		// if it's not one these kinds, it should have at most one datatype
		if(kind==RECORDSYM||kind==ENUMSYM||kind==NAMESYM)
		{
			SYMBOL tmp;

			while (datatype)
			{
				tmp = datatype->link;
				datatype->Release();
				datatype = tmp;
			}
		}
		else
		{
			if (datatype)
				datatype->Release();
		}

		delete this;
		return 0;
	}
	return dwRefs;
}
//--------------------------------------------------------------------------//
//
unsigned long Symbol::AddRef (void)
{
	dwRefs++;
	return dwRefs;
}
//--------------------------------------------------------------------------//
//
SymbolManager::SymbolManager(void)
{
	current_level = 0;
	memset(SymbolArray, 0, sizeof(SymbolArray));
}
//--------------------------------------------------------------------------//
//
void SymbolManager::free(void)
{
	int i;

	for (i = 0; i < MAX_LEVELS; i++)
		remove_tree(i);

	current_level = 0;
}
//--------------------------------------------------------------------------//
//
void SymbolManager::remove_node(SYMBOL sym)
{
	if (sym->left)
		remove_node(sym->left);
	if (sym->right)
		remove_node(sym->right);
	sym->Release();
}
//--------------------------------------------------------------------------//
//
void SymbolManager::remove_tree(int level)
{
	if (level < MAX_LEVELS)
	{
		SYMBOL sym = SymbolArray[level];	
		
		if (sym)
			remove_node(sym);
		SymbolArray[level] = 0;
	}
}
//--------------------------------------------------------------------------//
//
unsigned SymbolManager::get_hash(char *_name)
{
	if (_name == 0)
		return unsigned(rand() * rand());

	unsigned result[2];
	int len = strlen(_name);

	if (len >= 8)
	{
		memcpy(result, _name, 8);
	}
	else
	if (len >= 4)
	{
		memcpy(result, _name, 4);
		memcpy(((char*)result)+4, _name, 4);
	}
	else
	if (len >= 2)
	{
		memcpy(result, _name, 2);
		memcpy(((char*)result)+2, _name, 2);
		memcpy(((char*)result)+4, _name, 2);
		memcpy(((char*)result)+6, _name, 2);
	}
	else
	{
	 	memset(result, *_name, 8);
	}	

	return (result[0] * result[1]);
}
//--------------------------------------------------------------------------//
//
int SymbolManager::set_current_level(int new_level)
{
	int i = MAX_LEVELS - 1;

	while (i > new_level)
	{
		remove_tree(i--);
	}

	current_level = new_level;
	return new_level;
}
//--------------------------------------------------------------------------//
//
SYMBOL SymbolManager::get_symbol(char *_name, unsigned hashval, char create)
{
	int i = current_level;
	SYMBOL result=0;

	search_name = _name;
	search_hashval = hashval;

	while (i >= 0)
	{
		if ((result = SymbolArray[i]) != 0)
		{
			if ((result = find_node(result)) != 0)
			{
				break;
			}
		}
		i--;
	}

	if (result == 0 && create)
	{
		result = create_symbol(_name, hashval, current_level);
	}

	return result;
}
//--------------------------------------------------------------------------//
//
SYMBOL SymbolManager::find_node(SYMBOL sym)
{
	while (sym)
	{
		if (sym->hash == search_hashval && sym->name && !strcmp(sym->name, search_name))
			break;
		if (sym->hash <= search_hashval)
		{
			sym = sym->left;
			continue;
		}
		if (sym->hash > search_hashval)
			sym = sym->right;
	}
	
	return sym;
}
//--------------------------------------------------------------------------//
//
SYMBOL SymbolManager::create_symbol(char *_name, unsigned hashval, int level)
{
	SYMBOL result=0;
	SYMBOL node;

	if (level >= MAX_LEVELS)
	{
		fatal(TOO_MANY_LEVELS);
		level = MAX_LEVELS - 1;
	}

	if ((node = SymbolArray[level]) == 0)
	{
	 	SymbolArray[level] = result = new Symbol;
		result->dwRefs=1;
	}
	else  // search for insertion point
	{
	 	while (1)
		{
		 	if (node->hash == hashval && node->name && !strcmp(node->name, _name))
			{
			 	fatal(DOUBLE_DEFINE_VAR);
				result = node;
				break;
			}
			if (node->hash <= hashval)
			{
			 	if (node->left)
					node = node->left;
				else
				{
				 	node->left = result = new Symbol;
					result->dwRefs=1;
					break;
				}
			}
			else
			{
			 	if (node->right)
					node = node->right;
				else
				{
				 	node->right = result = new Symbol;
					result->dwRefs=1;
					break;
				}
			}
		}
	}
		
	result->name = _name;
	result->hash = hashval;
	result->blocklevel = level;

	return result;
}
//--------------------------------------------------------------------------//
//
void SymbolManager::add_node (SYMBOL sym)
{
	SYMBOL node;
	const UINT hashval = sym->hash;

	sym->left = sym->right = 0;

	if ((node = SymbolArray[current_level]) == 0)
	{
	 	SymbolArray[current_level] = sym;
		sym->AddRef();
	}
	else  // search for insertion point
	{
	 	while (1)
		{
		 	if (node->hash == hashval && node->name && node->name==sym->name)
			{
			 	fatal(DOUBLE_DEFINE_VAR);
				break;
			}
			if (node->hash <= hashval)
			{
			 	if (node->left)
					node = node->left;
				else
				{
				 	node->left = sym;
					sym->AddRef();
					break;
				}
			}
			else
			{
			 	if (node->right)
					node = node->right;
				else
				{
				 	node->right = sym;
					sym->AddRef();
					break;
				}
			}
		}
	}
}
//--------------------------------------------------------------------------//
//
void SymbolManager::push_namespace (SYMBOL name)
{
	SYMBOL sym = name->datatype;

	inc_current_level();

	while (sym)
	{
		add_node(sym);
		sym = sym->link;
	}
}
//--------------------------------------------------------------------------//
//
SYMBOL SymbolManager::insertfn(char *_name, SYMBOL resulttp, SYMBOL argtp)
{ 
	SYMBOL sym, res, arg;

	sym = create_symbol(_name);
	sym->kind = FUNCTIONSYM;
	res = new Symbol;
	res->dwRefs=1;
	res->kind = ARGSYM;
	res->datatype = resulttp;
	if (resulttp != 0) 
		res->basicdt = resulttp->basicdt;
	arg = new Symbol;
	arg->dwRefs=1;
	arg->kind = ARGSYM;
	arg->datatype = argtp;
	if (argtp != 0) 
		arg->basicdt = argtp->basicdt;
	arg->link = 0;
	res->link = arg;
	sym->datatype = res;
	return sym;
}
//--------------------------------------------------------------------------//
//
SYMBOL SymbolManager::insertbt(char *_name, int basictp, int siz)
{ 
	SYMBOL sym;
    sym = create_symbol(_name);
    sym->kind = BASICTYPE;
    sym->basicdt = basictp;
    sym->size = siz;
	sym->alignment = siz;
    return sym;
}
//--------------------------------------------------------------------------//
//
void SymbolManager::print_node(SYMBOL node)
{
	if (node == 0)
		return;
	print_node(node->left);
	if (node->name)
		printsymbol(node);
	print_node(node->right);
}
//--------------------------------------------------------------------------//
//
void SymbolManager::print_table(void)
{
	int i;
	
	for (i = 1; i < MAX_LEVELS; i++)
	{
		if (SymbolArray[i])
		{
#if DA_ERROR_LEVEL >= __SEV_TRACE_5
	FDUMP(ErrorCode(ERR_PARSER, SEV_TRACE_5), "Symbol Table level %d:\n", i);
#endif
			print_node(SymbolArray[i]);
		}
	}	
}
//--------------------------------------------------------------------------//
//
SYMBOL SymbolManager::CreateBOOL (void)
{
	SYMBOL result=0, tmp;

    result = create_symbol("bool");
	result->kind = ENUMSYM;
	result->basicdt = INTEGER;
	result->size = sizeof(bool);
	result->alignment = result->size;

	tmp = new Symbol;
	tmp->kind = CONSTSYM;
	tmp->name = "false";
	tmp->basicdt = INTEGER;
	tmp->size = sizeof(bool);
	tmp->alignment = tmp->size;
	tmp->constval.intnum = false;

	result->datatype = tmp;
	tmp->AddRef();

	tmp = new Symbol;
	tmp->kind = CONSTSYM;
	tmp->name = "true";
	tmp->basicdt = INTEGER;
	tmp->size = sizeof(bool);
	tmp->alignment = tmp->size;
	tmp->constval.intnum = true;

	result->datatype->link = tmp;
	tmp->AddRef();

	return result;
}
//--------------------------------------------------------------------------//
//
SYMBOL SymbolManager::set_reserved(void)
{  
	SYMBOL result, last;
	current_level = 0;               /* Put compiler symbols in block 0 */
	if (SymbolArray[0])
		return 0;

	result = last = insertbt("double", REAL, sizeof(double));
	last->link = insertbt("float", REAL, sizeof(float));
	last->link->AddRef();
	last = last->link;
	last->link = insertbt("__int64", INTEGER, sizeof(__int64));
	last->link->AddRef();
	last = last->link;
	last->link = insertbt("int", INTEGER, sizeof(int));
	last->link->AddRef();
	last = last->link;
	last->link = insertbt("long", INTEGER, sizeof(long));
	last->link->AddRef();
	last = last->link;
	last->link = insertbt("short", INTEGER, sizeof(short));
	last->link->AddRef();
	last = last->link;
	last->link = insertbt("char", INTEGER, sizeof(char));
	last->link->AddRef();
	last = last->link;
	last->link = insertbt("void", INTEGER, 0);
	last->link->AddRef();
	last = last->link;
	last->link = CreateBOOL();
	last->link->AddRef();

	return result;

/*
	realsym = insertbt("REAL", REAL, 8);
	intsym  = insertbt("INTEGER", INTEGER, 4);
	charsym = insertbt("CHAR", STRINGTYPE, 1);
	boolsym = insertbt("BOOLEAN", BOOLETYPE, 4);

    sym = insertfn("EXP", realsym, realsym);
	sym = insertfn("SIN", realsym, realsym);
	sym = insertfn("COS", realsym, realsym);
	sym = insertfn("SQRT", realsym, realsym);
	sym = insertfn("ROUND", intsym, realsym);
	sym = insertfn("ORD", intsym, intsym);
	sym = insertfn("WRITE", NULL, NULL);
	sym = insertfn("WRITELN", NULL, NULL);
	sym = insertfn("WRITELNF", NULL, realsym);
	sym = insertfn("READ", NULL, NULL);
	sym = insertfn("READLN", NULL, NULL);
	sym = insertfn("EOF", boolsym, NULL);
*/
}

//--------------------------------------------------------------------------//
//--------------------------END SYMTABLE.CPP--------------------------------//
//--------------------------------------------------------------------------//
