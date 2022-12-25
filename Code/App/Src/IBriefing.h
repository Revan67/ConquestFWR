#ifndef IBRIEFING_H
#define IBRIEFING_H
//--------------------------------------------------------------------------//
//                                                                          //
//                              IBriefing.h                                 //
//                                                                          //
//                  COPYRIGHT (C) 2000 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/IBriefing.h 6     9/18/00 5:01p Jasony $
*/			    
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
#ifndef DACOM_H
#include <DACOM.h>
#endif

struct CQBRIEFINGITEM;
struct CQBRIEFINGTELETYPE;

//--------------------------------------------------------------------------//
//
struct DACOM_NO_VTABLE IBriefing : IDAComponent
{
	virtual U32 PlayAnimatedMessage(const CQBRIEFINGITEM & item) = 0;  

	virtual U32 PlayTeletype(const CQBRIEFINGTELETYPE & item) = 0;

	virtual U32 PlayAnimation(const CQBRIEFINGITEM & item) = 0;

	virtual void FreeSlot (const U32 slotID) = 0;

	virtual const bool IsAnimationPlaying (const U32 slotID) = 0;
};

#endif