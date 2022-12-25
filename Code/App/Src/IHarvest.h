#ifndef IHARVEST_H
#define IHARVEST_H
//--------------------------------------------------------------------------//
//                                                                          //
//                                 IHarvest.h                               //
//                                                                          //
//                  COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/IHarvest.h 7     4/18/00 3:42p Tmauer $
*/			    
//--------------------------------------------------------------------------//

#ifndef IOBJECT_H
#include "IObject.h"
#endif

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//

struct _NO_VTABLE IHarvest : IObject
{
	virtual void BeginHarvest (IBaseObject * victim, U32 agentID, bool autoSelected) = 0;

	virtual void DockTaken (void) = 0;

	virtual U32 GetGas() = 0;

	virtual U32 GetMetal() = 0;

	virtual bool IsIdle() = 0;

	virtual void SetAutoHarvest(enum M_NUGGET_TYPE nType) = 0;
};

//-----------------------------------------------------------------------------------
//---------------------------END IHarvest.h------------------------------------------
//-----------------------------------------------------------------------------------
#endif