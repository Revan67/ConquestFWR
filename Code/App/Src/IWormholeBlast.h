#ifndef IWORMHOLEBLAST_H
#define IWORMHOLEBLAST_H
//--------------------------------------------------------------------------//
//                                                                          //
//                              IWormholeBlast.h                                    //
//                                                                          //
//                  COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/IWormholeBlast.h 3     9/30/00 6:00p Tmauer $
*/			    
//------------------------------- #INCLUDES --------------------------------//
//--------------------------------------------------------------------------//
#ifndef IOBJECT_H
#include "IObject.h"
#endif

//typedef S32 INSTANCE_INDEX;

struct _NO_VTABLE IWormholeBlast : IObject
{
	virtual void InitWormholeBlast(TRANSFORM & trans, U32 systemID, SINGLE radius, U32 playerID, bool bReverse) = 0;

	virtual void Flash() = 0;

	virtual void CloseUp() = 0;

	virtual void SetScriptID(U32 scID) = 0;

	virtual U32 GetScriptID() = 0;
};


//---------------------------------------------------------------------------
//-------------------------END IWormholeBlast.h--------------------------------------
//---------------------------------------------------------------------------
#endif
