//--------------------------------------------------------------------------//
//                                                                          //
//                             DataParser.cpp                              //
//                                                                          //
//                  COPYRIGHT (C) 2002 Fever Pitch Studios, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
	DataParser objects are created by the View Constructor. (See ViewCnst.h)
  
	$Header: /Libs/Src/Viewer/Docuview/DataParser.cpp 2     7/14/03 12:11p Ajackson $
*/			    
//--------------------------------------------------------------------------//
//
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

#include <DataParser.h>
#include "SymTable.h"
#include <Document.h>
#include "DataView.h"
//#include "TokenDef.h"

#include <HeapObj.h>
#include <TComponent.h>

void __stdcall CreateDataParser (SYMBOL symbol, IDataParser ** pParser);
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//  
struct DACOM_NO_VTABLE DataParser : public IDataParser
{
	SYMBOL root;

	BEGIN_DACOM_MAP_INBOUND(DataParser)
	DACOM_INTERFACE_ENTRY(IDataParser)
	DACOM_INTERFACE_ENTRY2(IID_IDataParser,IDataParser)
	END_DACOM_MAP()


	DataParser (void)
	{
	}

	~DataParser (void)
	{
		root->Release();
		root = 0;
	}

	void * operator new (size_t size)
	{
		return HEAP->ClearAllocateMemory(size, "DataParser");
	}

	/* IDataParser methods */

	virtual BOOL32 __stdcall Enumerate (IParserCallback * callback);

	
	/* DataParser methods */

	void init (SYMBOL symbol)
	{
		root = symbol;
		root->AddRef();
	}

	IDAComponent * getBase (void)
	{
		return this;
	}
};
//--------------------------------------------------------------------------//
//
BOOL32 DataParser::Enumerate (IParserCallback * callback)
{
	BOOL32 result=1;
	VARIABLEDESC vdesc;
	SYMBOL symbol = root->datatype;
	COMPTR<IDataParser> parser;

	if (root->kind == ARRAYSYM)
	{
		int count = root->arrayval;

		vdesc.varName = 0;
		vdesc.typeName = symbol->name;

		switch (symbol->kind)
		{
		case ARRAYSYM:
			vdesc.kind = VARIABLEDESC::ARRAY;
			break;

		case RECORDSYM:
			vdesc.kind = VARIABLEDESC::RECORD;
			break;

		default:
			vdesc.kind = VARIABLEDESC::BASIC;
			break;
		}

		vdesc.offset = 0;
		vdesc.size = symbol->size;
		vdesc.arraySize = symbol->arrayval;
		vdesc.bitOffset = symbol->bit_offset;
		vdesc.bitCount = symbol->bit_count;

		CreateDataParser(symbol, parser);

		while (count-- > 0)
		{
			if ((result = callback->VarInstance(parser, vdesc)) == 0)
				break;
			vdesc.offset += vdesc.size;
		}
	}
	else
	if (root->kind == RECORDSYM)
	{
		SYMBOL oldType = 0;
		//
		// symbol is a VARSYM
		//
		while (symbol)
		{
			vdesc.varName = symbol->name;
			vdesc.typeName = symbol->datatype->name;

			switch (symbol->datatype->kind)
			{
			case ARRAYSYM:
				vdesc.kind = VARIABLEDESC::ARRAY;
				break;

			case RECORDSYM:
				vdesc.kind = VARIABLEDESC::RECORD;
				break;

			default:
				vdesc.kind = VARIABLEDESC::BASIC;
				break;
			}

			vdesc.offset = symbol->offset;
			vdesc.size = symbol->size;
			vdesc.arraySize = DataViewer::GetType(symbol)->arrayval;
			vdesc.bitOffset = symbol->bit_offset;
			vdesc.bitCount = symbol->bit_count;

			if (oldType != symbol->datatype)
				CreateDataParser(symbol, parser);

			if ((result = callback->VarInstance(parser, vdesc)) == 0)
				break;

			oldType = symbol->datatype;
			symbol = symbol->link;
		}
	}

	return result;
}

//--------------------------------------------------------------------------//
//
void __stdcall CreateDataParser (SYMBOL symbol, IDataParser ** pParser)
{
	symbol = DataViewer::GetType(symbol);

	if (symbol->kind == ARRAYSYM || symbol->kind == RECORDSYM)
	{
		DataParser * parser = new DAComponent<DataParser>;
		parser->init(symbol);
		*pParser = parser;
	}
	else
		*pParser = 0;
}
//--------------------------------------------------------------------------//
//--------------------------End DataParser.cpp------------------------------//
//--------------------------------------------------------------------------//
