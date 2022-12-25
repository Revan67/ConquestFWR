#ifndef MULTICLOAKLAUCHER_H
#define MULTICLOAKLAUCHER_H
//--------------------------------------------------------------------------//
//                                                                          //
//                                MultiCloakLauncher.h                          //
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

#ifndef DMULTICLOAKLAUNCHER_H
#include <DMultiCloakLauncher.h>
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

#ifndef ICLOAK_H
#include "ICloak.h"
#endif

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
struct _NO_VTABLE MultiCloakLauncher : IBaseObject, ILauncher, ISaveLoad, MULTICLOAK_LAUNCHER_SAVELOAD
{
	BEGIN_MAP_INBOUND(MultiCloakLauncher)
	_INTERFACE_ENTRY(IBaseObject)
	_INTERFACE_ENTRY(ISaveLoad)
	_INTERFACE_ENTRY(ILauncher)
	END_MAP()

	OBJPTR<ILaunchOwner> owner;	// person who created us
	OBJPTR<ICloak> cloakTarget;
		
//	bool bStandby;		// turn on the cloaking as soon as we can (ie. no longer visible to enemies)
	S32 cloakLaunchIndex;

	SINGLE cloakSupplyUse;
	SINGLE targetSupplyCostPerHull;
	SINGLE cloakShutoff;
	SINGLE_TECHNODE techNode;

	//----------------------------------
	//----------------------------------
	
	MultiCloakLauncher (void);

	~MultiCloakLauncher (void);	

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
		// undesired behavior
		/*
		bStandby = false;
		if (bCloakEnabled != false)
		{
			bCloakEnabled = false;
			enableCloak(bCloakEnabled);
		}
		*/
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
		decloakTime = 0;
		if(bCloakingTarget)
		{
			if(cloakTarget)
			{
				cloakTarget->EnableCloak(false);
			}
		}
		cloakTargetID = 0;
		cloakTarget = NULL;
		bCloakingTarget = false;
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

	virtual void SpecialAttackObject (IBaseObject * obj);

	virtual void DoCloak (void);

	virtual void  GetSpecialAbility (UNIT_SPECIAL_ABILITY & ability, bool & bSpecialEnabled)
	{
		MPart part = owner.Ptr();

		ability = USA_MULTICLOAK;
		bSpecialEnabled = part->caps.synthesisOk && checkSupplies();
	}

	virtual const U32 GetApproxDamagePerSecond (void) const
	{
		return 0;
	}

	virtual void InformOfCancel() {};

	virtual void LauncherOpCreated(U32 agentID, void * buffer, U32 bufferSize) {};

	virtual void LauncherReceiveOpData(U32 agentID, void * buffer, U32 bufferSize) {};

	virtual void LauncherOpCompleted(U32 agentID) {};

	virtual bool CanCloak();

	virtual bool IsToggle() {return false;};

	virtual bool CanToggle(){return false;};

	virtual bool IsOn() {return false;};

	// the following methods are for network synchronization of realtime objects
	virtual U32 GetSyncDataSize (void) const
	{
		return 1;
	}

#define F_CLOAK_OFF 0x01
#define F_CLOAK_ON  0x02
#define F_CLOAK_TARG_ON 0x04
#define F_CLOAK_TARG_OFF 0x08

#define SHROUD_RANGE_MOD 3.0

	virtual U32 GetSyncData (void * buffer)
	{
		U8 command = 0;
		if(bPrepareToggle)
		{
			bPrepareToggle = false;
			if(!bCloakEnabled)
			{
				enableCloak(true);
				command = F_CLOAK_ON;
			}
			else
			{
				enableCloak(false);
				command = F_CLOAK_OFF;
				if(cloakTarget && bCloakingTarget)
				{
					cloakTarget->EnableCloak(false);
					cloakTarget = NULL;
					cloakTargetID = 0;
					bCloakingTarget = false;
					command |= F_CLOAK_TARG_OFF;
				}
			}
		}

		if(cloakTarget)
		{
			if(bCloakingTarget)
			{
				if((cloakTarget.Ptr()->GetSystemID() != owner.Ptr()->GetSystemID()) || (cloakTarget.Ptr()->GetGridPosition()-owner.Ptr()->GetGridPosition() > (owner->GetWeaponRange()*SHROUD_RANGE_MOD)/GRIDSIZE))	
				{
					if(decloakTime)
					{
						if(decloakTime+(5.0/ELAPSED_TIME) < MGlobals::GetUpdateCount())
						{
							cloakTarget->EnableCloak(false);
							cloakTarget = NULL;
							cloakTargetID = 0;
							bCloakingTarget = false;
							command |= F_CLOAK_TARG_OFF;
						}
					}
					else
					{
						decloakTime = MGlobals::GetUpdateCount();
					}
				}
				else
				{
					decloakTime = 0;
				}
			}
			else
			{
				if((cloakTarget.Ptr()->GetSystemID() == owner.Ptr()->GetSystemID()) && (cloakTarget.Ptr()->GetGridPosition()-owner.Ptr()->GetGridPosition() <= owner->GetWeaponRange()/GRIDSIZE))	
				{
					cloakTarget->EnableCloak(true);
					bCloakingTarget = true;
					command |= F_CLOAK_TARG_ON;
				}
			}
		}
		else if(cloakTargetID)
		{
			cloakTarget = NULL;
			cloakTargetID = 0;
			bCloakingTarget = false;
			command |= F_CLOAK_TARG_OFF;
		}
		if(command)
		{
			((U8*)buffer)[0] = command;
			return 1;
		}
		return 0;
	}

	virtual void PutSyncData (void * buffer, U32 bufferSize)
	{
		U8 command = ((U8 *)buffer)[0];

		if(command & F_CLOAK_ON)
		{
			CQASSERT(!bCloakEnabled);
			enableCloak(true);
		} else if(command & F_CLOAK_OFF)
		{
			CQASSERT(bCloakEnabled);
			enableCloak(false);
		}
		
		if(command & F_CLOAK_TARG_ON)
		{
			if(cloakTarget)
			{
				cloakTarget->EnableCloak(true);
			}
			bCloakingTarget = true;
		}else if(command & F_CLOAK_TARG_OFF)
		{
			if(cloakTarget && bCloakingTarget)
			{
				cloakTarget->EnableCloak(false);
			}
			cloakTarget = NULL;
			cloakTargetID = 0;
			bCloakingTarget = false;
		}
	}

	virtual void OnAllianceChange (U32 allyMask)
	{
	}

	virtual IBaseObject * FindChildTarget(U32 childID){return NULL;};

	/* ISaveLoad methods */

	virtual BOOL32 Save (struct IFileSystem * file);

	virtual BOOL32 Load (struct IFileSystem * file);
	
	virtual void ResolveAssociations();

	/* MultiCloakLauncher methods */
	
	void init (PARCHETYPE pArchetype);

	bool checkSupplies();

	void enableCloak(bool bCloakEnabled);

	void checkTechLevel();

};

//---------------------------------------------------------------------------------------------
//-------------------------------End MultiCloakLauncher.h-------------------------------------------
//---------------------------------------------------------------------------------------------
#endif