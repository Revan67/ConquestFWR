#include "stdafx.h"

#include "typelist.h"

#include <viewcnst.h>	// ViewConstructor
#include <document.h>
#include <viewer.h>

extern ICOManager *DACOM;

//---------------------------------------------------------------------------
// TypeList
//---------------------------------------------------------------------------

TypeList TheTypeList;

static IViewConstructor2* PARSER = NULL;
static HANDLE             SYMBOLS = 0;

BOOL32 __stdcall viewcnst_enum_proc2(struct IViewConstructor2 * cnst, const C8 *typeName, SYMBOL symbol, void *context)
{
	OutputDebugString(typeName);
	OutputDebugString("\n");
	return true;
}

void InitParser(const char *memory, unsigned int size)
{
	if( !PARSER )
	{
		DACOMDESC desc = "IViewConstructor";
		BOOL32 result = 0;
		IViewConstructor* parser = NULL;
	  
		if (DACOM->CreateInstance(&desc,(void**)&parser) == GR_OK)
		{
			parser->QueryInterface("IViewConstructor2", (void **) & PARSER);
			parser->Release();
		}		
	}

	if( PARSER )
	{
		if( SYMBOLS )
		{
			PARSER->DestroySymbols(SYMBOLS);
		}
		char* block = new char[ size ];
		memcpy( block, memory, size );
		SYMBOLS = PARSER->ParseNewMemory(block);
		delete block;

		PARSER->EnumerateTypes( viewcnst_enum_proc2 );
	}
}

//---------------------------------------------------------------------------
// PARSE
//---------------------------------------------------------------------------

BOOL32 __stdcall AddTypeName (struct IViewConstructor *cnst, const C8 *typeName, void *context)
{
	int size = cnst->GetTypeSize(typeName);
	TheTypeList.add(typeName,size);
	return TRUE;
}

void ParseMemory (const char *memory, unsigned int size)
{
	if( size )
		InitParser(memory,size);

	IViewConstructor *parser;
	DACOMDESC desc;
	desc.size = sizeof(desc);
	desc.interface_name = "IViewConstructor";
	if (DACOM->CreateInstance(&desc, (void **) &parser) == GR_OK)
	{
		parser->ParseMemory(memory);					// build viewers for my structures
		parser->EnumerateTypes(&AddTypeName,0);
		DACOM->RegisterComponent(parser,"IViewer");		// put viewers in master list
		parser->Release();
	}
}

//---------------------------------------------------------------------------

void ParseFile (const char *filename)
{
	ICOManager *DACOM = DACOM_Acquire();

	IViewConstructor *parser;
	DACOMDESC desc;
	desc.size = sizeof(desc);
	desc.interface_name = "IViewConstructor";
	if (DACOM->CreateInstance(&desc, (void **) &parser) == GR_OK)
	{
		if (parser->FullParseFile(filename) != GR_OK)	// build viewers for my structures
			parser->ParseFile(filename);

		TheTypeList.reset();
		parser->EnumerateTypes(&AddTypeName,0);

		DACOM->RegisterComponent(parser,"IViewer");		// put viewers in master list
		parser->Release();
	}
}

//---------------------------------------------------------------------------

CString GetTypeName (const char *name)
{
	CString type_name = name;

	const char *p = strstr(name,"::");
	if (p)
	{
		type_name = type_name.Left(p-name);
	}

	return type_name;
}

//--------------------------------------------------------------------------------------------------

void OpenViewer(struct IDocument *doc,const char *baseName,const char *instName)
{
	VIEWDESC vdesc;
	vdesc.className	= baseName;
	vdesc.doc		= doc;

	IViewer* viewer;
	if( DACOM->CreateInstance(&vdesc, (void **) &viewer) == GR_OK )
	{
//		doc->AddRef();
//		viewer->set_readonly(true);
//		doc->Release();
	}
}

//---------------------------------------------------------------------------

void ViewMemory(const char* type, void* memory)
{
	if( !PARSER ) return;

	SYMBOL s = PARSER->GetSymbol(SYMBOLS, type);

	if( s )
	{
		U32 memorySize = PARSER->GetTypeSize(s);

		if( memorySize )
		{
			DOCDESC desc;
			desc.size = sizeof(desc);
			desc.memory = memory;
			desc.memoryLength = memorySize;

			IDocument * doc = NULL;
			if (DACOM->CreateInstance(&desc, (void **) &doc) == GR_OK)
			{
				OpenViewer(doc, type, "instance");
				doc->Release();
			}
		}
	}
}

