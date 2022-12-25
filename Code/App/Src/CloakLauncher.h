#ifndef CLOAKLAUCHER_H
#define CLOAKLAUCHER_H
//--------------------------------------------------------------------------//
//                                                                          //
//                                CloackLauncher.h                          //
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

#ifndef DCLOAKLAUNCHER_H
#include <DCloakLauncher.h>
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
struct _NO_VTABLE CloakLauncher : IBaseObject, ILauncher, ISaveLoad, CLOAK_LAUNCHER_SAVELOAD
{
	BEGIN_MAP_INBOUND(CloakLauncher)
	_INTERFACE_ENTRY(IBaseObject)
	_INTERFACE_ENTRY(ISaveLoad)
	_INTERFACE_ENTRY(ILauncher)
	END_MAP()

	OBJPTR<ILaunchOwner> owner;	// person who created us		

	S32 cloakLaunchIndex;

	SINGLE cloakSupplyUse;
	SINGLE cloakShutoff;
	SINGLE_TECHNODE techNode;

	//----------------------------------
	//----------------------------------
	
	CloakLauncher (void);

	~CloakLauncher (void);	

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

	virtual void SpecialAttackObject (IBaseObject * obj)
	{
	}

	virtual void DoCloak (void);

	virtual void  GetSpecialAbility (UNIT_SPECIAL_ABILITY & ability, bool & bSpecialEnabled)
	{
		MPart part = owner.Ptr();

		ability = USA_CLOAK;
		bSpecialEnabled = part->caps.cloakOk && checkSupplies();
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

	virtual U32 GetSyncData (void * buffer)
	{
		if(bPrepareToggle)
		{
			bPrepareToggle = false;
			if(!bCloakEnabled)
			{
				enableCloak(true);
				((U8 *) buffer)[0] = 1;
				return 1;
			}
			else
			{
				enableCloak(false);
				((U8 *) buffer)[0] = 2;
				return 1;
			}
		}
		return 0;
	}

	virtual void PutSyncData (void * buffer, U32 bufferSize)
	{
		U8 command = ((U8*)buffer)[0];
		if(command == 1)
		{
			CQASSERT(!bCloakEnabled);
			enableCloak(true);
		} else if(command == 2)
		{
			CQASSERT(bCloakEnabled);
			enableCloak(false);
		}
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

	/* CloakLauncher methods */
	
	void init (PARCHETYPE pArchetype);

	bool checkSupplies();

	void enableCloak(bool bEnable);

	void checkTechLevel();

};

//---------------------------------------------------------------------------------------------
//-------------------------------End CloakLauncher.h-------------------------------------------
//---------------------------------------------------------------------------------------------
#endif