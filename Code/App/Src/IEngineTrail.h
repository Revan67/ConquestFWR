#ifndef IENGINETRAIL_H
#define IENGINETRAIL_H
//--------------------------------------------------------------------------//
//                                                                          //
//                              IEngineTrail.h                                    //
//                                                                          //
//                  COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $ $
*/			    
//------------------------------- #INCLUDES --------------------------------//
//--------------------------------------------------------------------------//
#ifndef IOBJECT_H
#include "IObject.h"
#endif

//--------------------------------------------------------------------------//
//
struct _NO_VTABLE IEngineTrail : IObject 
{
	virtual BOOL32 InitEngineTrail (IBaseObject * owner, S32 ownerIndex) = 0;

	virtual void SetLengthModifier(SINGLE percent) = 0;

	virtual void Render (void) = 0;

	virtual void BatchRender (U32 stage) = 0;

	virtual void SetupBatchRender(U32 stage) = 0;

	virtual void FinishBatchRender(U32 stage) = 0;

	virtual U32 GetBatchRenderStateNumber() = 0;

	virtual BOOL32 Update (void) = 0;

	virtual void PhysicalUpdate (SINGLE dt) = 0;

//	virtual void PhysicalTimeUpdate (SINGLE dt) = 0;

	virtual void Reset (void) = 0;

	virtual void SetDecayMode(void) = 0;
};
//---------------------------------------------------------------------------
//----------------------END IEngineTrail.h-----------------------------------
//---------------------------------------------------------------------------
#endif
