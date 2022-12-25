#ifndef IJUMPPLAT_H
#define IJUMPPLAT_H
//--------------------------------------------------------------------------//
//                                                                          //
//                                IJumpPlat.h                                 //
//                                                                          //
//                  COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/IJumpPlat.h 3     3/17/00 8:43p Tmauer $
*/			    
//------------------------------- #INCLUDES --------------------------------//
//--------------------------------------------------------------------------//
#ifndef IOBJECT_H
#include "IObject.h"
#endif

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//

struct _NO_VTABLE IJumpPlat : IObject
{
	virtual void ParkYourself(IBaseObject * jumpgate) = 0;

	virtual void SetSibling(IBaseObject * baseSib) = 0;

	virtual void SetHull(U32 amount, bool terminate) = 0;

	virtual IBaseObject * GetJumpGate() = 0;

	virtual void SetSiblingHull(U32 amount) = 0;

	virtual IBaseObject * GetSibling() = 0;
};


//---------------------------------------------------------------------------
//-----------------------END IJumpPlat.h---------------------------------------
//---------------------------------------------------------------------------
#endif
