#ifndef IGOTOPOS_H
#define IGOTOPOS_H
//--------------------------------------------------------------------------//
//                                                                          //
//                               IGotoPos.h                                 //
//                                                                          //
//                  COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/IGotoPos.h 24    8/23/01 1:53p Tmauer $
*/			    
//--------------------------------------------------------------------------//


#ifndef IOBJECT_H
#include "IObject.h"
#endif

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//

struct _NO_VTABLE IGotoPos : IObject
{
	virtual void GotoPosition (const struct GRIDVECTOR & pos, U32 agentID, bool bSlowMove) = 0;
	//
	// jumpgate usage
	//
	virtual void PrepareForJump (IBaseObject * jumpgate, bool bUserMove, U32 agentID, bool bSlowMove) = 0;
	virtual void UseJumpgate (IBaseObject * outgate, IBaseObject * ingate, const Vector& jumpToPosition, SINGLE heading, SINGLE speed, U32 agentID) = 0;
	virtual bool IsJumping (void) = 0;
	virtual bool IsHalfSquare() = 0;

	virtual void Patrol (const struct GRIDVECTOR & src, const struct GRIDVECTOR & dst, U32 agentID) = 0;

};

#endif