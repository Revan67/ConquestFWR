#ifndef ILAUNCHER_H
#define ILAUNCHER_H
//--------------------------------------------------------------------------//
//                                                                          //
//                                ILauncher.h                               //
//                                                                          //
//                  COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/ILauncher.h 43    9/13/01 10:01a Tmauer $
*/			    
//------------------------------- #INCLUDES --------------------------------//
//--------------------------------------------------------------------------//

#ifndef IOBJECT_H
#include "IObject.h"
#endif

struct ILauncher;

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
struct _NO_VTABLE ILaunchOwner : IObject
{
	virtual bool UseSupplies (U32 amount,bool bAbsolute = false) = 0;	// return true if supplies were available

	virtual bool TestLOS (const struct GRIDVECTOR & pos) = 0;

	virtual void GetLauncher (const U32 index, OBJPTR<ILauncher> & objLauncher) = 0;

	virtual void LauncherCancelAttack (void) = 0;

	virtual void GotoLauncherPosition (GRIDVECTOR pos) = 0;

	virtual void LauncherSendOpData (U32 agentID, void * buffer,U32 bufferSize) = 0;

	virtual void LaunchOpCompleted (ILauncher * launcher,U32 agentID) = 0 ;

	virtual U32 CreateLauncherOp (ILauncher * launcher,struct ObjSet & set,void * buffer,U32 bufferSize) = 0;

	virtual SINGLE GetWeaponRange (void) = 0;

	virtual SINGLE GetOptimalWeaponRange (void) = 0;

	virtual IBaseObject * GetTarget (void) = 0;

	virtual bool HasAttackAgent (void) = 0;

	virtual IBaseObject * FindChildTarget(U32 childID) = 0;
};

//--------------------------------------------------------------------------//
//
struct _NO_VTABLE ILauncher : IObject
{
	virtual void InitLauncher (IBaseObject * owner, S32 ownerIndex, S32 animArcheIndex, SINGLE range) = 0;

	virtual void AttackPosition (const struct GRIDVECTOR * position, bool bSpecial) = 0;

	virtual void AttackObject (IBaseObject * obj) = 0;

	virtual void AttackMultiSystem (struct GRIDVECTOR * position, U32 targSystemID) = 0;

	virtual void WormAttack (IBaseObject * obj) = 0;

	virtual void DoCreateWormhole(U32 systemID) = 0;

	virtual const bool TestFightersRetracted (void) const = 0;   // return true if fighters are retracted

	virtual void SetFighterStance(enum FighterStance stance) = 0;

	// stop attacking, kill off probes, etc. switch over to the new missionID
	virtual void HandlePreTakeover (U32 newMissionID, U32 troopID) = 0;

	virtual void TakeoverSwitchID (U32 newMissionID) = 0;

	// the following methods are for network synchronization of realtime objects
	virtual U32 GetSyncDataSize (void) const = 0;

	virtual U32 GetSyncData (void * buffer) = 0;			// buffer points to use supplied memory

	virtual void PutSyncData (void * buffer, U32 bufferSize) = 0;

	virtual void DoSpecialAbility (U32 specialID) = 0;

	virtual void DoSpecialAbility (IBaseObject *obj) = 0;

	virtual void SpecialAttackObject (IBaseObject * obj) = 0;

	virtual void DoCloak (void) = 0;

	virtual bool IsToggle() = 0;

	virtual bool CanToggle() = 0;

	virtual bool IsOn() = 0;

	/* what special attack (if any) does this unit have, and can we use it?  Variables get overwritten */

	virtual void  GetSpecialAbility (enum UNIT_SPECIAL_ABILITY & ability, bool & bEnabled) = 0; 

	virtual const U32 GetApproxDamagePerSecond (void) const = 0;

	virtual void InformOfCancel (void) = 0;

	virtual void LauncherOpCreated (U32 agentID, void * buffer, U32 bufferSize) = 0;

	virtual void LauncherReceiveOpData (U32 agentID, void * buffer, U32 bufferSize) = 0;

	virtual void LauncherOpCompleted (U32 agentID) = 0;

	virtual bool CanCloak (void) = 0;

	// stop attacking allies!
	virtual void OnAllianceChange (U32 allyMask) = 0;

	virtual IBaseObject * FindChildTarget(U32 childID) = 0;
};

struct _NO_VTABLE IMimic : IObject
{
	virtual void SetDiscoveredToAllies (U32 allyMask) = 0;

	virtual bool IsDiscoveredTo (U32 allyMask) = 0;

	virtual void UpdateDiscovered (void) = 0;
};

struct _NO_VTABLE ISystemSupplier : IObject
{
	virtual bool IsTempSupply () = 0;
};
//---------------------------------------------------------------------------------------------
//-------------------------------End ILauncher.h-----------------------------------------------
//---------------------------------------------------------------------------------------------
#endif