//--------------------------------------------------------------------------//
//                                                                          //
//                             LoadFont.cpp                                //
//                                                                          //
//               COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
   $Header: /Conquest/App/Src/LoadFont.cpp 2     10/28/98 1:26p Jasony $
*/
//--------------------------------------------------------------------------//

 
#include "pch.h"
#include <globals.h>

#include "Startup.h"
#include "CQTrace.h"

#include <IProfileParser.h>
#include <TSmartPointer.h>

//--------------------------------------------------------------------------//
//
struct _loadfont : GlobalComponent, IDAComponent
{
	U32 ref_count;
	static IDAComponent * _this;

	_loadfont (void)
	{
		ref_count = 1;
		_this = this;
	}
 
	/* GlobalComponent methods */

	virtual void Startup (void)
	{
		AddToGlobalCleanupList(&_this);
		load();
	}

	virtual void Initialize (void)
	{
	}

	/* IDAComponent methods */

	DEFMETHOD(QueryInterface) (const C8 *interface_name, void **instance)
	{
		*instance = 0;
		return GR_GENERIC;
	}
	
	DEFMETHOD_(U32,AddRef) (void)
	{
		ref_count++;
		return ref_count;
	}
	
	DEFMETHOD_(U32,Release) (void)
	{
		if (ref_count)
		{
			if (--ref_count==0)
			{
				unload();
			}
		}
		return ref_count;
	}

	/* _loadfont methods */

	void load (void);

	void unload (void);
};
//--------------------------------------------------------------------------//
//
void _loadfont::load (void)
{
	COMPTR<IProfileParser> parser;
	HANDLE hSection;
	U32 line;
	C8 buffer[MAX_PATH];

	if (DACOM->QueryInterface("IProfileParser", parser) != GR_OK)
		goto Done;

	if ((hSection = parser->CreateSection("FONTS")) == 0)
		goto Done;

	line=0;
	while (parser->ReadProfileLine(hSection, line++, buffer, sizeof(buffer)))
	{
		int i = AddFontResource(buffer);
		CQTRACE52("Added %d Font Resource(s) from \"%s\"", i, buffer);
	}

	parser->CloseSection(hSection);

Done:
	return;
}
//--------------------------------------------------------------------------//
//
void _loadfont::unload (void)
{
	COMPTR<IProfileParser> parser;
	HANDLE hSection;
	U32 line;
	C8 buffer[MAX_PATH];

	if (DACOM->QueryInterface("IProfileParser", parser) != GR_OK)
		goto Done;

	if ((hSection = parser->CreateSection("FONTS")) == 0)
		goto Done;

	line=0;
	while (parser->ReadProfileLine(hSection, line++, buffer, sizeof(buffer)))
	{
		RemoveFontResource(buffer);
	}

	parser->CloseSection(hSection);

Done:
	return;
}

static _loadfont loadfont;
IDAComponent * _loadfont::_this;

//--------------------------------------------------------------------------//
//------------------------------End LoadFont.cpp----------------------------//
//--------------------------------------------------------------------------//
