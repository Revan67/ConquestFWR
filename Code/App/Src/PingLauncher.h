#ifndef PINGLAUCHER_H
#define PINGLAUCHER_H
//--------------------------------------------------------------------------//
//                                                                          //
//                                PingLauncher.h                          //
//                                                                          //
//                  COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header:
*/			    
//------------------------------- #INCLUDES --------------------------------//
//--------------------------------------------------------------------------//

#ifndef TOBJECT_H
#include "TObject.h"
#endif

#ifndef DPINGLAUNCHER_H
#include <DPingLauncher.h>
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

#ifndef SYSMAP_H
#include "Sysmap.h"
#endif

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
struct _NO_VTABLE PingLaunch : IBaseObject, ILauncher, ISaveLoad, PING_LAUNCHER_SAVELOAD
{
	BEGIN_MAP_INBOUND(PingLaunch)
	_INTERFACE_ENTRY(IBaseObject)
	_INTERFACE_ENTRY(ISaveLoad)
	_INTERFACE_ENTRY(ILauncher)
	END_MAP()

	OBJPTR<ILaunchOwner> owner;	// person who created us
	//
	// sync info
	// 
	struct SYNC_PACKET
	{
		U32 objectIDs[MAX_AOE_VICTIMS];
		U32 damage[MAX_AOE_VICTIMS];
	};
		

	S32 pingLaunchIndex;

	SINGLE pingSupplyCost;
	SINGLE_TECHNODE techNode;

	bool bPingIt;

	//----------------------------------
	//----------------------------------
	
	PingLaunch (void);

	~PingLaunch (void);	

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
	

	virtual void AttackPosition (const struct GRIDVECTOR * position, bool bSpecial)
	{
	}

	virtual void AttackObject (IBaseObject * obj)
	{
	}

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
	}

	virtual void TakeoverSwitchID (U32 newMissionID)
	{
	}

	virtual void DoSpecialAbility (U32 specialID);

	virtual void DoSpecialAbility (IBaseObject *obj)
	{
	}

	virtual void DoCloak (void)
	{
	}
	
	virtual void SpecialAttackObject (IBaseObject * obj)
	{
	}

	virtual void  GetSpecialAbility (UNIT_SPECIAL_ABILITY & ability, bool & bSpecialEnabled)
	{
		MPart part = owner.Ptr();

		ability = USA_PING;
		bSpecialEnabled = part->caps.specialAbilityOk && checkSupplies();
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

	virtual U32 GetSyncData (void * buffer)
	{
		if(bPingIt)
		{
			bPingIt = false;
			SYSMAP->PingSystem(owner.Ptr()->GetSystemID(),owner.Ptr()->GetPosition(),owner.Ptr()->GetPlayerID());
			return 1;
		}
		return 0;
	}

	virtual void PutSyncData (void * buffer, U32 bufferSize)
	{
		SYSMAP->PingSystem(owner.Ptr()->GetSystemID(),owner.Ptr()->GetPosition(),owner.Ptr()->GetPlayerID());
	}

	virtual void OnAllianceChange (U32 allyMask)
	{
	}

	virtual IBaseObject * FindChildTarget(U32 childID){return NULL;};

	/* ISaveLoad methods */

	virtual BOOL32 Save (struct IFileSystem * file)
	{
		return TRUE;
	}

	virtual BOOL32 Load (struct IFileSystem * file)
	{
		return FALSE;
	}
	
	virtual void ResolveAssociations()
	{
	}


	/* PingLauncher methods */
	
	void init (PARCHETYPE pArchetype);

	bool checkSupplies();

	void checkTechLevel();

};

//---------------------------------------------------------------------------------------------
//-------------------------------End PingLauncher.h-------------------------------------------
//---------------------------------------------------------------------------------------------
#endif