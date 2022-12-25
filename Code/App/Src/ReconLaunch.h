#ifndef RECONLAUNCH_H
#define RECONLAUNCH_H
//--------------------------------------------------------------------------//
//                                                                          //
//                                ReconLaunch.h                                  //
//                                                                          //
//                  COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/ReconLaunch.h 33    9/13/01 10:01a Tmauer $
*/			    
//------------------------------- #INCLUDES --------------------------------//
//--------------------------------------------------------------------------//

#ifndef TOBJECT_H
#include "TObject.h"
#endif

#ifndef DRECONLAUCH_H
#include "DReconLaunch.h"
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

#ifndef IRECON_H
#include "IRecon.h"
#endif

#ifndef OBJLIST_H
#include "ObjList.h"
#endif

#ifndef MPART_H
#include "MPart.h"
#endif

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
struct _NO_VTABLE ReconLaunch : IBaseObject, ILauncher, IReconLauncher, ISaveLoad, RECON_LAUNCH_SAVELOAD
{
	BEGIN_MAP_INBOUND(ReconLaunch)
	_INTERFACE_ENTRY(IBaseObject)
	_INTERFACE_ENTRY(ISaveLoad)
	_INTERFACE_ENTRY(ILauncher)
	_INTERFACE_ENTRY(IReconLauncher)
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
	bool bWormWeapon;

	OBJPTR<IBaseObject> target;
	OBJPTR<ILaunchOwner> owner;	// person who created us
	OBJPTR<IReconProbe> probe;

	UNIT_SPECIAL_ABILITY specialAbility;
		
	//----------------------------------
	//----------------------------------
	
	ReconLaunch (void);

	~ReconLaunch (void);	

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

	/* IReconLauncher */

	virtual void KillProbe (U32 dwMissionID);

	/* ILauncher methods */

	virtual void InitLauncher (IBaseObject * owner, S32 ownerIndex, S32 animArcheIndex, SINGLE range);

	virtual void AttackPosition (const struct GRIDVECTOR * position, bool bSpecial);

	virtual void AttackObject (IBaseObject * obj);

	virtual void AttackMultiSystem (struct GRIDVECTOR * position, U32 targSystemID);

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
		if(probe && probe->IsActive())
		{
			probe->ExplodeProbe();
		}
	}

	virtual void TakeoverSwitchID (U32 newMissionID)
	{
		if(probe)
		{
			probe->ReconSwitchID(newMissionID);
			if(!probe)
			{
				probeID = (probeID & (~PLAYERID_MASK)) | (newMissionID&PLAYERID_MASK);
				IBaseObject * obj = OBJLIST->FindObject(probeID);
				if(obj)
				{
					obj->QueryInterface(IReconProbeID,probe,NONSYSVOLATILEPTR);
					CQASSERT(probe!=0);
				}		
			}
		}
	}

	virtual void DoSpecialAbility (U32 specialID);

	virtual void DoSpecialAbility (IBaseObject * obj)
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
			bSpecialEnabled = (part->caps.specialEOAOk || part->caps.specialAbilityOk || part->caps.probeOk) && (supplyCost <= part->supplies);
		}
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
		if((attacking == 1) && refireDelay <= 0 && (owner.Ptr()->effectFlags.canShoot()) && (!owner.Ptr()->fieldFlags.suppliesLocked()))
		{
			U8 * data = (U8 *) buffer;
			U32 result = hostShootTarget(data);
			if((!result) && bKillProbe)
			{
				*data = 2;
				bKillProbe = false;
				probe->ExplodeProbe();
				return 1;//kill the probe;
			}
			bKillProbe = false;
			return result;
		}
		else if(bKillProbe)
		{
			probe->ExplodeProbe();
			bKillProbe = false;
			U8 * data = (U8 *)buffer;
			data[0] = 2;
			return 1;//kill the probe;
		}

		return 0;
	}

	virtual void PutSyncData (void * buffer, U32 bufferSize)
	{
		U8 * data = (U8 *) buffer;
		if(probe->IsActive())
			probe->ExplodeProbe();
		if(data[0] == 1)
			clientShootTarget();
	}

	virtual void OnAllianceChange (U32 allyMask)
	{
	}

	virtual IBaseObject * FindChildTarget(U32 childID){return NULL;};

	/* ISaveLoad methods */

	virtual BOOL32 Save (struct IFileSystem * file);

	virtual BOOL32 Load (struct IFileSystem * file);
	
	virtual void ResolveAssociations();

	/* FancyLaunch methods */

	void getSighting (class TRANSFORM & result) const;
	void init (PARCHETYPE pArchetype, PARCHETYPE _pBoltType);
	U32 hostShootTarget (U8 * packet);
	void clientShootTarget ();
};

//---------------------------------------------------------------------------------------------
//-------------------------------End FancyLaunch.h--------------------------------------------------
//---------------------------------------------------------------------------------------------
#endif