#ifndef ITALORIANEFFECT_H
#define ITALORIANEFFECT_H
//--------------------------------------------------------------------------//
//                                                                          //
//                              ITalorianEffect.h                                    //
//                                                                          //
//                  COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/ITalorianEffect.h 2     7/14/00 3:50p Tmauer $
*/			    
//------------------------------- #INCLUDES --------------------------------//
//--------------------------------------------------------------------------//
#ifndef IOBJECT_H
#include "IObject.h"
#endif


struct _NO_VTABLE ITalorianEffect : IObject
{
	virtual void InitTalorianEffect(TRANSFORM & trans, U32 systemID, U32 playerID) = 0;

	virtual void WarmUp() = 0;

	virtual void ShutDown() = 0;

	virtual bool IsActive() = 0;

	virtual void CloseUp() = 0;
};


//---------------------------------------------------------------------------
//-------------------------END IWormholeBlast.h--------------------------------------
//---------------------------------------------------------------------------
#endif
