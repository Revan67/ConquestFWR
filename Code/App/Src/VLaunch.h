#ifndef VLAUNCH_H
#define VLAUNCH_H
//--------------------------------------------------------------------------//
//                                                                          //
//                                VLaunch.h                                  //
//                                                                          //
//                  COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/VLaunch.h 43    9/13/01 10:01a Tmauer $
*/			    
//------------------------------- #INCLUDES --------------------------------//
//--------------------------------------------------------------------------//

#ifndef TOBJECT_H
#include "TObject.h"
#endif

#ifndef DVLAUNCH_H
#include <DVLaunch.h>
#endif

#ifndef _3DMATH_H
#include <3DMath.h>
#endif

#ifndef _IHARDPOINT_H_
#include <IHardPoint.h>
#endif

#ifndef ILAUNCHER_H
#include "ILauncher.h"
#endif

#ifndef IWEAPON_H
#include "IWeapon.h"
#endif

#ifndef RANGEFINDER_H
#include "RangeFinder.h"
#endif


//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
struct _NO_VTABLE VLaunch : IBaseObject, ILauncher, VLAUNCH_SAVELOAD
{
	BEGIN_MAP_INBOUND(VLaunch)
	_INTERFACE_ENTRY(IBaseObject)
	_INTERFACE_ENTRY(ILauncher)
	END_MAP()

	//----------------------------------
	// hardpoint data
	//----------------------------------
	HardpointInfo  hardpointinfo[MAX_VERTICAL_TUBES];
	INSTANCE_INDEX ownerIndex[MAX_VERTICAL_TUBES];	// index of object that hardpoints are connected to
	//----------------------------------
	// randomizing data
	//----------------------------------
	int randomIndex[MAX_VERTICAL_TUBES];
	int numRandomEntries;
	//----------------------------------
	// rangeFinder
	//----------------------------------
	RangeFinder rangeFinder;
	SINGLE rangeError;
	//----------------------------------
	// Bolt data
	//----------------------------------
	PARCHETYPE pBoltType;
	SINGLE REFIRE_PERIOD;
	U32 MAX_SALVO, MINI_REFIRE;
	U32 SUPPLY_COST;		// per salvo
	//
	// target we are shooting at
	//
	OBJPTR<IBaseObject> target;
	OBJPTR<ILaunchOwner> owner;	// person who created us

	//----------------------------------
	//----------------------------------
	
	VLaunch (void);

	~VLaunch (void);	

	/* IBaseObject methods */

	virtual BOOL32 Update (void);	// returning FALSE causes destruction

	virtual S32 GetObjectIndex (void) const;

	virtual U32 GetPartID (void) const;

	virtual U32 GetSystemID (void) const
	{
		return owner.Ptr()->GetSystemID();
	}

	virtual U32 GetPlayerID (void) const
	{
		return owner.Ptr()->GetPlayerID();
	}

	const TRANSFORM & GetTransform (void) const
	{
		return owner.Ptr()->GetTransform();
	}

	/* ILauncher methods */

	virtual void InitLauncher (IBaseObject * owner, S32 ownerIndex, S32 animArcheIndex, SINGLE range);

	virtual void AttackPosition (const struct GRIDVECTOR * position, bool bSpecial);

	virtual void AttackObject (IBaseObject * obj);

	virtual void AttackMultiSystem (struct GRIDVECTOR * position, U32 targSystemID)
	{}

	virtual void WormAttack (IBaseObject * obj)
	{};

	virtual void DoCreateWormhole(U32 systemID)
	{}

	virtual const bool TestFightersRetracted (void) const 
	{ 
		return true;
	}

	virtual void SetFighterStance(FighterStance stance)
	{
	}

	virtual void HandlePreTakeover (U32 newMissionID, U32 troopID)
	{
		AttackObject(NULL);
	}

	virtual void TakeoverSwitchID (U32 newMissionID)
	{
	}

	virtual void DoSpecialAbility (U32 specialID)
	{
	}

	virtual void DoSpecialAbility (IBaseObject * obj)
	{
	}

	virtual void DoCloak (void)
	{
	}
	
	virtual void SpecialAttackObject (IBaseObject * obj)
	{
	}

	virtual void  GetSpecialAbility (UNIT_SPECIAL_ABILITY & ability, bool & bEnabled)
	{
	}

	virtual const U32 GetApproxDamagePerSecond (void) const
	{
		return 0;
	}

	virtual void InformOfCancel() {};

	virtual void LauncherOpCreated(U32 agentID, void * buffer, U32 bufferSize) {};

	virtual void LauncherReceiveOpData(U32 agentID, void * buffer, U32 bufferSize) {};

	virtual void LauncherOpCompleted(U32 agentID) {};

	virtual bool CanCloak(){return false;};

	virtual bool IsToggle() {return false;};

	virtual bool CanToggle(){return false;};

	virtual bool IsOn() {return false;};

	// the following methods are for network synchronization of realtime objects
	virtual U32 GetSyncDataSize (void) const
	{
		return 1;
	}

	virtual U32 GetSyncData (void * _buffer)
	{
		return 0;
	}

	virtual void PutSyncData (void * _buffer, U32 bufferSize)
	{
	}

	virtual void OnAllianceChange (U32 allyMask)
	{
		if (target)
		{
			// do we happen to be attacking a new friend?
			const U32 hisPlayerID = target->GetPlayerID() & PLAYERID_MASK;
							
			// if we are now allied with the target then stop shooting at him
			if (hisPlayerID && (((1 << (hisPlayerID-1)) & allyMask) != 0))
			{
				target = 0;
				dwTargetID = 0;
			}
		}
	}

	virtual IBaseObject * FindChildTarget(U32 childID){return NULL;};

	/* VLaunch methods */

	//static S32 findChild (const char * pathname, INSTANCE_INDEX parent);
	//static BOOL32 findHardpoint (const char * pathname, INSTANCE_INDEX & index, HardpointInfo & hardpointinfo, INSTANCE_INDEX parent);
	void getSighting (int whichHardpoint, class TRANSFORM & result);
	void init (PARCHETYPE _pArchetype, PARCHETYPE _pBoltType);
	int getRandomTube (void);
	bool checkSupplies (void);	// return true if ok to fire

	U32 findSalvo(const BT_VERTICAL_LAUNCH * data = NULL);

	Vector calculateErrorPos(const Vector & pos);
};

//---------------------------------------------------------------------------------------------
//-------------------------------End VLaunch.h-------------------------------------------------
//---------------------------------------------------------------------------------------------
#endif