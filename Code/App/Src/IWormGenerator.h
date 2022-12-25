#ifndef IWORMGENERATOR_H
#define IWORMGENERATOR_H
//--------------------------------------------------------------------------//
//                                                                          //
//                               IWormGenerator.h                                 //
//                                                                          //
//                  COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/IWormGenerator.h 2     6/07/00 9:42p Tmauer $
*/			    
//--------------------------------------------------------------------------//


#ifndef IOBJECT_H
#include "IObject.h"
#endif

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//

struct _NO_VTABLE IWormGenerator : IObject
{
	virtual void SetSibling(IBaseObject * sibling) = 0;

	virtual IBaseObject * GetOwner() = 0;
};

struct _NO_VTABLE IWormholeSync : IObject
{
	virtual void CreateAt(IBaseObject * targ1, IBaseObject * targ2, U32 targID);

	virtual void ResolveWormhole(IBaseObject * owner);

	virtual void InitWormhole(IBaseObject * owner,U32 partID);
};

#endif