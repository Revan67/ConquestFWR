#ifndef IFIGHTER_H
#define IFIGHTER_H
//--------------------------------------------------------------------------//
//                                                                          //
//                                IFighter.h                                //
//                                                                          //
//                  COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/IFighter.h 27    11/14/00 12:29p Jasony $
*/			    
//------------------------------- #INCLUDES --------------------------------//
//--------------------------------------------------------------------------//
#ifndef IOBJECT_H
#include "IObject.h"
#endif

#ifndef DFIGHTER_H
#include <DFighter.h>
#endif

#ifndef IENGINETRAIL_H
#include "IEngineTrail.h"
#endif

#ifndef CQBATCH_H
#include "CQBatch.h"
#endif

//
// A fighter is the tiny individual ship
//
struct FighterInfo
{
	U32 systemID;
	U32 playerID;
	bool bIsLeader;
	const BT_FIGHTER_DATA * pFighterData;
};
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
struct _NO_VTABLE IFighterOwner : IObject
{
	virtual void OnFighterDestruction (S32 fighterIndex) = 0;	 // fighter hit by weapon

	virtual struct IFighter * GetFighter (S32 fighterIndex) = 0;

	virtual void GetLandingClearance (void) = 0;

	virtual void OnFighterLanding (S32 fighterIndex) = 0;

	virtual struct IBaseObject * FindFighter (U32 fighterMissionID) = 0;
};
//--------------------------------------------------------------------------//
//
struct _NO_VTABLE IFighter : IObject 
{
	//The batch render now works like this
	//  step 1 - ask how many different batches there are with GetBatchRenderStateNumber()
	//			this will return the total number of states for this object and it's children
	//  step 2 - loop over the number of states and do ->
	//		2.1 - Call SetupBatchRender(stage) - this will set up the render flags for the batch render.
	//				note - if stage is greater than this objects number of states then it is the state of one of you children
	//		2.2 - loop through all objects calling BatchRender(stage) 
	//		2.3 - Call FinishBatchRender(stage)

	virtual void BatchRender (U32 stage,BATCHDESC &desc) = 0;

	virtual void SetupBatchRender(U32 stage,BATCHDESC &desc,int fighter_cnt) = 0;

	virtual void FinishBatchRender(U32 stage,BATCHDESC &desc,int fighter_cnt) = 0;

	virtual U32 GetBatchRenderStateNumber() = 0;

	virtual IEngineTrail * GetTrail (void) = 0;

	virtual void InitFighter (IBaseObject * owner, S32 index, SINGLE _baseAirAccuracy, SINGLE _baseGroundAccuracy) = 0;

	virtual void LaunchFighter (const class TRANSFORM & orientation, const class Vector & initialVelocity, U32 formationIndex, S32 fighterIndex, S32 parentIndex, S32 childIndex) = 0;

	virtual void ReturnToCarrier (void) = 0;

	virtual void SetFighterSupplies (U32 supplies) = 0;

	virtual U32  GetFighterSupplies (void) const = 0;

	virtual FighterState GetState (void) const = 0;

	virtual void SetTarget (IBaseObject * target) = 0;

	virtual void SetFormationState (U8 state) = 0;

	virtual U8 GetFormationState (void) = 0;

	virtual void SetPatrolState (U8 state) = 0;

	virtual bool IsLeader (void) const = 0;  // true if has no leader itself

	virtual bool IsRelated (S32 fighterIndex) const = 0;	// return true if "fighterIndex" is parent or child

	virtual void GetFighterInfo (FighterInfo & info) const = 0;

	virtual void * __fastcall GetChildFighter (OBJPTR<IFighter> & pInterface) = 0;

	virtual U32 GetFighterOwner (void) const = 0;	// return part of carrier

	// add object to the OBJ_MAP
	virtual void SetRadarSigniture (void) = 0;

	virtual bool IsVisible (void) const = 0;

	// duplicated from IBaseObject for convenience
	virtual U32 GetPlayerID (void) const = 0;	// return ID of player who owns fighter

	// duplicated from IBaseObject for convenience
	virtual BOOL32 Update (void) = 0;	

	// duplicated from IBaseObject for convenience
	virtual void PhysicalUpdate (SINGLE dt) = 0;	

	// duplicated from IBaseObject for convenience
//	virtual void Render (void) = 0;

	// duplicated from IBaseObject for convenience
	virtual Vector GetVelocity (void) = 0;
	
	// duplicated from IBaseObject for convenience
	virtual const TRANSFORM & GetTransform (void) const = 0;

	// duplicated from IBaseObject for convenience
	virtual void RevealFog (const U32 currentSystem) = 0;

	// duplicated from IBaseObject for convenience
	virtual void CastVisibleArea (void) = 0;

	// duplicated from IBaseObject for convenience
	virtual void TestVisible (const USER_DEFAULTS & defaults, const U32 currentSystem, const U32 currentPlayer) = 0;

	virtual void SetMissionID (U32 dwMissionID) = 0;

	static IFighter * FindFighter (U32 dwMissionID);

	virtual S32 GetHullPoints (void) = 0;
};

//---------------------------------------------------------------------------
//---------------------------END IFighter.h----------------------------------
//---------------------------------------------------------------------------
#endif
