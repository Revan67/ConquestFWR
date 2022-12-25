#ifndef IJUMPGATE_H
#define IJUMPGATE_H
//--------------------------------------------------------------------------//
//                                                                          //
//                                IBuild.h                                 //
//                                                                          //
//                  COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/IJumpGate.h 22    11/06/00 1:26p Jasony $
*/			    
//------------------------------- #INCLUDES --------------------------------//
//--------------------------------------------------------------------------//
#ifndef IOBJECT_H
#include "IObject.h"
#endif

#define MAX_WARP_DISTANCE 7000
#define JUMP_TIME 0.9
#define PRE_JUMP_TIME 1.0
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//

struct JumpgateInfo
{
	U32 gate_id,exit_gate_id;
};

struct _NO_VTABLE IJumpGate : IObject
{
	//returns time it will take to warp
	virtual SINGLE JumpOut (IBaseObject *obj,SINGLE time) = 0;

	//arrivalTime is the time that the ship will hit the jumpgate
	//returns time that the ship will be released in the new system
	virtual SINGLE JumpIn (IBaseObject *obj,SINGLE arrivalTime,const Vector &dir) = 0;

	virtual void InitWith (U32 systemID,Vector pos,U32 gateID,U32 exit_gateID) = 0;

	virtual bool IsJumpAllowed (void) = 0;

	virtual void SetJumpAllowed (bool bAllowed) = 0;

	virtual bool PlayerCanJump (U32 playerID) = 0;

	virtual void SetOwner (U32 dwMissionID) = 0;

	virtual void UnsetOwner (U32 dwMissionID) = 0;

	virtual U32 GetPlayerOwner (void) = 0;	// returns the playerID of the owner

	virtual struct JumpgateInfo *GetJumpgateInfo() = 0;

	virtual void Mark (U32 playerID) = 0;

	virtual void Unmark (U32 playerID) = 0;

	virtual bool IsHighlightingBuild (void) = 0;

	virtual void Lock (void) = 0;

	virtual void Unlock (void) = 0;

	virtual bool IsLocked (void) = 0;

	virtual bool CanIBuild (U32 playerID) = 0;

	virtual bool IsOwnershipKnownToPlayer (U32 playerID) = 0;

	virtual void SetJumpgateInvisible (bool bEnable) = 0;
};

//---------------------------------------------------------------------------
//------------------------END IJumpGate.h------------------------------------
//---------------------------------------------------------------------------
#endif
