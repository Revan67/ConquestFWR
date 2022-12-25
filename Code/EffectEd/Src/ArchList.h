#ifndef ARCHLIST_H
#define ARCHLIST_H
//--------------------------------------------------------------------------//
//                                                                          //
//                               ArchList.H                                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Author: Tmauer $

    $Header: /EffectEd/Src/ArchList.h 2     10/14/03 4:06p Tmauer $
*/			    
//-------------------------------------------------------------------
/*
	Management of the archetype list
*/
//-------------------------------------------------------------------
//-------------------------------------------------------------------
#ifndef DACOM_H
#include <DACOM.h>
#endif

#ifndef OBJCLASS_H
#include "ObjClass.h"
#endif

#ifndef DBASEDATA_H
#include "DBaseData.h"
#endif

#pragma warning(disable : 4200)

struct BASIC_DATA;
struct IBaseObject;
//--------------------------------------------------------------------------//
//
struct ARCHDATATYPE
{
	C8 name[GT_PATH];
	BASIC_DATA * objData;
	U32 dataSize;		// size of data chunk in bytes
};

//--------------------------------------------------------------------------//
//
struct ARCHDATA
{
	U32 numArchetypes;
	ARCHDATATYPE type[];
};

//----------------------------------------------------------------------------
//

struct DACOM_NO_VTABLE IArchetypeList : public IDAComponent
{
	virtual	void * GetArchetypeData (const C8 * name) = 0;

	virtual	BOOL32 EnumerateArchetypeData (struct IArchetypeEnum * enumerator) = 0;

	virtual	BOOL32 EnumerateArchetypeDataBySubArch (struct IArchetypeEnum * enumerator, const char* subarch) = 0;

	virtual void Close (void) = 0;
};

//----------------------------------------------------------------------------
//
struct IArchetypeEnum
{
	virtual	BOOL32 ArchetypeEnum (const char * name, void *data, U32 size) = 0;
};

#endif
