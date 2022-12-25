#ifndef STRINGDATA_H
#define STRINGDATA_H
//--------------------------------------------------------------------------//
//                                                                          //
//                               StringData.h                               //
//                                                                          //
//                  COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/StringData.h 3     12/02/99 7:01p Sbarton $
*/			    
//---------------------------------------------------------------------------

#ifndef DACOM_H
#include <Dacom.h>
#endif

//----------------------------------------------------------------------------
//
struct DACOM_NO_VTABLE IStringData : public IDAComponent
{
	virtual void * GetArchetypeData (const C8 * name) = 0;

	virtual void * GetArchetypeData (const C8 * name, U32 & dataSize) = 0;		// also returns data size
};

//-------------------------------------------------------------------
//-------------------------END StringData.h---------------------------
//-------------------------------------------------------------------


#endif