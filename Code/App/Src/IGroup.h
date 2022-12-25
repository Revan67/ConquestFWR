#ifndef IGROUP_H
#define IGROUP_H
//--------------------------------------------------------------------------//
//                                                                          //
//                                IGroup.h                                 //
//                                                                          //
//                  COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/IGroup.h 5     8/25/99 2:11p Jasony $
*/			    
//------------------------------- #INCLUDES --------------------------------//
//--------------------------------------------------------------------------//

#ifndef IOBJECT_H
#include "IObject.h"
#endif

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
struct _NO_VTABLE IGroup : IObject 
{
	virtual void InitGroup (const U32 *pObjIDs, U32 numObjects, U32 dwMissionID) = 0;

	virtual bool TestGroup (const U32 *pObjIDs, U32 numObjects) const = 0;

	virtual U32 GetObjects (U32 pObjIDs[MAX_SELECTED_UNITS]) const = 0;

	virtual void EnableDeath (void) = 0;		// called on client machines when group has gone stale
};

//---------------------------------------------------------------------------
//------------------------END IGroup.h---------------------------------------
//---------------------------------------------------------------------------
#endif
