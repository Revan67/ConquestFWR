#ifndef FANCYLAUCH_H
#define FANCYLAUCH_H
//--------------------------------------------------------------------------//
//                                                                          //
//                                FancyLaunch.h                                  //
//                                                                          //
//                  COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/FancyLaunch.h 46    9/13/01 10:01a Tmauer $
*/			    
//------------------------------- #INCLUDES --------------------------------//
//--------------------------------------------------------------------------//

#ifndef TOBJECT_H
#include "TObject.h"
#endif

#ifndef DFANCYLAUCH_H
#include "DFancyLaunch.h"
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

#ifndef MPART_H
#include "MPart.h"
#endif

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
struct _NO_VTABLE FancyLaunch : IBaseObject, ILauncher, ISaveLoad, FANCY_LAUNCH_SAVELOAD
{
	BEGIN_MAP_INBOUND(FancyLaunch)
	_INTERFACE_ENTRY(IBaseObject)
	_INTERFACE_ENTRY(ISaveLoad)
	_INTERFACE_ENTRY(ILauncher)
	END_MAP()

	//----------------------------------
	// animation index
	//----------------------------------
	S32 animIndex;
	//----------------------------------
	// hardpoint data
	//----------------------------------
	HardpointInfo  hardpointinfo;
	INSTANCE_INDEX barrelIndex;
	mutable TRANSFORM	   hardpointTransform;
	//----------------------------------
	// Joint data
	//----------------------------------
	S32 fancyLaunchIndex;
	//----------------------------------
	// Bolt data
	//----------------------------------
	PARCHETYPE pBoltType;
	SINGLE REFIRE_PERIOD;
	U32 supplyCost;
	SINGLE animTime,effectDuration,effectTimer;
	//
	// target we are shooting at
	//
	OBJPTR<IBaseObject> target;
	OBJPTR<ILaunchOwner> owner;	// person who created us

	bool bSpecialWeapon:1;
	bool bTargetRequired:1;
	bool bWormWeapon:1;
	UNIT_SPECIAL_ABILITY specialAbility;

	//
	// sync info
	// 
	struct SYNC_PACKET
	{
		U32 objectIDs[MAX_AOE_VICTIMS];
	};
		
	//----------------------------------
	//----------------------------------
	
	FancyLaunch (void);

	~FancyLaunch (void);	

	/* IBaseObject methods */

	virtual const TRANSFORM & GetTransform (void) const;

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

	/* ILauncher methods */

	virtual void InitLauncher (IBaseObject * owner, S32 ownerIndex, S32 animArcheIndex, SINGLE range);

	virtual void AttackPosition (const struct GRIDVECTOR * position, bool bSpecial);

	virtual void AttackObject (IBaseObject * obj);

	virtual void AttackMultiSystem (struct GRIDVECTOR * position, U32 targSystemID)
	{}

	virtual void WormAttack (IBaseObject * obj);

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
		attacking = 0;
	}


	virtual void TakeoverSwitchID (U32 newMissionID)
	{
	}

	virtual void DoSpecialAbility (U32 specialID)
	{
	}

	virtual void DoSpecialAbility (IBaseObject *obj)
	{
	}

	virtual void DoCloak (void)
	{
	}

	virtual void SpecialAttackObject (IBaseObject * obj);

	virtual void  GetSpecialAbility (UNIT_SPECIAL_ABILITY & ability, bool & bSpecialEnabled)
	{
		if (specialAbility != USA_NONE)
		{
			MPart part = owner.Ptr();

			ability = specialAbility;
			if (bTargetRequired)
			{
				bSpecialEnabled = part->caps.specialAttackOk && (supplyCost <= part->supplies);
			}
			else
			{
				bSpecialEnabled = part->caps.specialEOAOk && (supplyCost <= part->supplies);
			}
		}
	}

	virtual const U32 GetApproxDamagePerSecond (void) const
	{
		return 0;
	}

	virtual void InformOfCancel();

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
		return sizeof(SYNC_PACKET);
	}

	virtual U32 GetSyncData (void * buffer)
	{
		if (((attacking && target) || (attacking == 1)) && refireDelay <= 0 && (owner.Ptr()->effectFlags.canShoot()) && (!owner.Ptr()->fieldFlags.suppliesLocked()))
		{
			SYNC_PACKET * data = (SYNC_PACKET *) buffer;
			return hostShootTarget(data);
		}

		return 0;
	}

	virtual void PutSyncData (void * buffer, U32 bufferSize)
	{
		SYNC_PACKET data;
		memset((void *)(&data),0,sizeof(SYNC_PACKET));
		if(bufferSize != 1)
			memcpy((void *)(data.objectIDs),buffer,bufferSize);

		clientShootTarget(&data);
	}

	virtual void OnAllianceChange (U32 allyMask)
	{
		// don't do anything
	}

	virtual IBaseObject * FindChildTarget(U32 childID){return NULL;};

	/* ISaveLoad methods */

	virtual BOOL32 Save (struct IFileSystem * file);

	virtual BOOL32 Load (struct IFileSystem * file);
	
	virtual void ResolveAssociations();

	/* FancyLaunch methods */

//	static S32 findChild (const char * pathname, INSTANCE_INDEX parent);
//	static BOOL32 findHardpoint (const char * pathname, INSTANCE_INDEX & index, HardpointInfo & hardpointinfo, INSTANCE_INDEX parent);
	void getSighting (class TRANSFORM & result) const;
	void init (PARCHETYPE pArchetype, PARCHETYPE _pBoltType);
	void recalculateOffsetVectorForDistance (void);
	void recalculateOffsetVectorForAngle (const Vector & barrelPos, SINGLE yaw, SINGLE rotAngle);
	static void getPadPoints (VFX_POINT points[4], const TRANSFORM & transform, const OBJBOX & _box);
	U32 hostShootTarget (SYNC_PACKET * packet);
	void clientShootTarget (const SYNC_PACKET * packet);
	SINGLE getDistanceToTarget (void);
};

//---------------------------------------------------------------------------------------------
//-------------------------------End FancyLaunch.h--------------------------------------------------
//---------------------------------------------------------------------------------------------
#endif