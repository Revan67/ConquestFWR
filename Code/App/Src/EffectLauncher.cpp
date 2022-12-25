//--------------------------------------------------------------------------//
//                                                                          //
//                       Effect Launcher.cpp                                //
//                                                                          //
//                  COPYRIGHT (C) 2004 Warthog TX, INC.                     //
//                                                                          //
//--------------------------------------------------------------------------//
//------------------------------- #INCLUDES --------------------------------//
//--------------------------------------------------------------------------//

#include "pch.h"
#include <globals.h>

#include "TObject.h"
#include <DTurret.h>

#include <3DMath.h>
#include <IHardPoint.h>
#include "ILauncher.h"
#include "TerrainMap.h"
#include "EffectPlayer.h"

#include <DWeapon.h>

#include "SuperTrans.h"
#include "ObjList.h"

#include "Mission.h"
#include "IMissionActor.h"
#include "Startup.h"
#include "ArchHolder.h"
#include "Sector.h"
#include "TerrainMap.h"
#include "Camera.h"
#include "MPart.h"
#include "GridVector.h"
#include "TManager.h"
#include "EventScheduler.h"

#include <Engine.h>
#include <TSmartPointer.h>
#include <IAnim.h>
#include <FileSys.h>
#include <TComponent.h>
#include <IConnection.h>

#include <stdlib.h>

struct EffectLauncherArchetype 
{
	PARCHETYPE pArchetype;
	BT_EFFECT_LAUNCHER *data;

	IEffectHandle * effect;

	void * operator new (size_t size)
	{
		return HEAP->ClearAllocateMemory(size, "EffectLauncherArchetype");
	}

	void   operator delete (void *ptr)
	{
		HEAP->FreeMemory(ptr);
	}

	EffectLauncherArchetype (void)
	{
		effect = NULL;
	}

	~EffectLauncherArchetype (void)
	{
		if(effect)
		{
			EFFECTPLAYER->ReleaseEffect(effect);
			effect = NULL;
		}
	}
};


struct _NO_VTABLE EffectLauncher : IBaseObject, ILauncher, BASE_EFFECT_LAUNCHER_SAVELOAD
{
	BEGIN_MAP_INBOUND(EffectLauncher)
	_INTERFACE_ENTRY(IBaseObject)
	_INTERFACE_ENTRY(ILauncher)
	END_MAP()

	//----------------------------------
	// EffectLauncherArchetype Handle
	//----------------------------------
	EffectLauncherArchetype * hArchetype;

	//
	// target we are shooting at
	//
	OBJPTR<IBaseObject> target;
	OBJPTR<ILaunchOwner> owner;	// person who created us

	IEffectInstance * effectInst;

	//----------------------------------
	//----------------------------------
	
	EffectLauncher (void);

	~EffectLauncher (void);	

	/* IBaseObject methods */

	virtual const TRANSFORM & GetTransform (void) const;

	virtual BOOL32 Update (void);	// returning FALSE causes destruction

	virtual void PhysicalUpdate (SINGLE dt);

	virtual void Render(void);

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
		ability = USA_NONE;
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

	virtual void OnAllianceChange (U32 allyMask);

	virtual IBaseObject * FindChildTarget(U32 childID){return NULL;};

	/* Turret methods */
	void init (EffectLauncherArchetype * _hArchetype);
	bool checkLOS (void);	// with targetPos
	SINGLE getDistanceToTarget (void);
};

//---------------------------------------------------------------------------
//
EffectLauncher::EffectLauncher (void) 
{
}
//---------------------------------------------------------------------------
//
EffectLauncher::~EffectLauncher (void)
{
	if(effectInst)
	{
		if(EFFECTPLAYER->IsValidInstance(effectInst))
			effectInst->Stop();
		effectInst = NULL;
	}
}
//---------------------------------------------------------------------------
//
void EffectLauncher::init (EffectLauncherArchetype * _hArchetype)
{
	hArchetype = _hArchetype;
	const BT_EFFECT_LAUNCHER * data = _hArchetype->data;

	CQASSERT(data);
	CQASSERT(data->type == LC_EFFECT_LAUNCHER);
	CQASSERT(data->objClass == OC_LAUNCHER);

	pArchetype = hArchetype->pArchetype;
	objClass = OC_LAUNCHER;
}
//---------------------------------------------------------------------------
//
void EffectLauncher::InitLauncher (IBaseObject * _owner, S32 ownerIndex, S32 animArcheIndex, SINGLE range)
{
	CQASSERT(_owner);
	_owner->QueryInterface(ILaunchOwnerID, owner, LAUNCHVOLATILEPTR);
	CQASSERT(owner!=0);

	if(hArchetype->effect)
	{
		effectInst = hArchetype->effect->CreateInstance();
		
		effectInst->SetTarget(owner.Ptr(),0,0);
		effectInst->SetTarget(target.Ptr(),1,0);
		effectInst->SetSystemID(owner.Ptr());
		effectInst->TriggerStartEvent();
	}
}
//---------------------------------------------------------------------------
//
const TRANSFORM & EffectLauncher::GetTransform (void) const
{
	return owner.Ptr()->GetTransform();
}
//---------------------------------------------------------------------------
//
SINGLE EffectLauncher::getDistanceToTarget (void)
{
	SINGLE result = 0;

	if (target)
		result = target->GetGridPosition() - owner.Ptr()->GetGridPosition();

	return result;
}
//---------------------------------------------------------------------------
//
static bool bUpdateSwivel = true;
static bool bRangeError = true;
BOOL32 EffectLauncher::Update (void)
{
	if (target != 0)
	{
		effectInst->SetTarget(target.Ptr(),1,0);

		targetPos = target->GetTransform().get_position();

		if (refireDelay <= 0)
		{
			if ((target==0 || (target->GetSystemID()==GetSystemID() && target->IsVisibleToPlayer(owner.Ptr()->GetPlayerID()))) && 
				getDistanceToTarget() < (owner->GetWeaponRange()/GRIDSIZE) && checkLOS())
			{
				if((owner.Ptr()->effectFlags.canShoot()) && (!owner.Ptr()->fieldFlags.suppliesLocked())) // either we had supplies
				{
					if (owner->UseSupplies(hArchetype->data->supplyCost))
					{
						//do the real damage
						if(hArchetype->data->weaponDamage)
						{
							SINGLE dist = (targetPos-owner.Ptr()->GetTransform().translation).fast_magnitude();

							SINGLE time = (dist/hArchetype->data->weaponVelocity) + hArchetype->data->weaponFireDelay;
							SCHEDULER->QueueDamageEvent(time,owner.Ptr()->GetPartID(), target.Ptr()->GetPartID(), hArchetype->data->weaponDamage);
						}		
						//do the visual
						if(bVisible || target.Ptr()->bVisible)
						{
							//fire 
							VOLPTR(IEffectTarget) effTarg = owner.Ptr();
							if(effTarg)
								effTarg->TriggerGameEvent("shoot");
						}
					}
				}
			}
			refireDelay +=  hArchetype->data->refirePeriod;
		}
		else
		{
			refireDelay -= ELAPSED_TIME;
		}
	}
	else
	{
		effectInst->SetTarget(NULL,1,0);
	}

	return 1;
}

void EffectLauncher::PhysicalUpdate (SINGLE dt)
{
}

//---------------------------------------------------------------------------
//
void EffectLauncher::Render(void)
{
}

//---------------------------------------------------------------------------
//
void EffectLauncher::AttackPosition (const struct GRIDVECTOR * pos, bool bSpecial)
{
}
//---------------------------------------------------------------------------
//
void EffectLauncher::AttackObject (IBaseObject * _target)
{
	if  (_target == 0)
	{
		target = 0;
		dwTargetID = 0;
	}
	else
	{
		dwTargetID = _target->GetPartID();
		_target->QueryInterface(IBaseObjectID, target, GetPlayerID());
	}
}
//---------------------------------------------------------------------------
//
S32 EffectLauncher::GetObjectIndex (void) const
{
	return INVALID_INSTANCE_INDEX;
}
//---------------------------------------------------------------------------
//
U32 EffectLauncher::GetPartID (void) const
{
	return owner.Ptr()->GetPartID();
}
//---------------------------------------------------------------------------
//
void EffectLauncher::OnAllianceChange (U32 allyMask)
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
//---------------------------------------------------------------------------
// return TRUE if we have a clear LineOfSight with the target
//
bool EffectLauncher::checkLOS (void)	// with targetPos
{
	CQASSERT(target);
	GRIDVECTOR vec;
	vec = target->GetGridPosition();
	return owner->TestLOS(vec);
}
//---------------------------------------------------------------------------
//
inline IBaseObject * createEffectLauncher (EffectLauncherArchetype * hArchetype)
{
	EffectLauncher * effectLauncher = new ObjectImpl<EffectLauncher>;

	effectLauncher->init(hArchetype);

	return effectLauncher;
}
//------------------------------------------------------------------------------------------
//---------------------------EffectLauncher Factory----------------------------------------------
//------------------------------------------------------------------------------------------
//
struct DACOM_NO_VTABLE EffectLauncherFactory : public IObjectFactory
{
	//------------------------------------------------------------------------------------------
	//
	U32 factoryHandle;		// handles to callback

	//
	// Interface mapping
	//

	BEGIN_DACOM_MAP_INBOUND(EffectLauncherFactory)
	DACOM_INTERFACE_ENTRY(IObjectFactory)
	END_DACOM_MAP()

	EffectLauncherFactory (void) { }

	~EffectLauncherFactory (void);

	void init (void);

    void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}

	/* IObjectFactory methods */

	DEFMETHOD_(HANDLE,CreateArchetype) (const char *szArchname, OBJCLASS objClass, void *data);

	DEFMETHOD_(BOOL32,DestroyArchetype) (HANDLE hArchetype);

	DEFMETHOD_(IBaseObject *,CreateInstance) (HANDLE hArchetype);

	DEFMETHOD_(void,EditorCreateInstance) (HANDLE hArchetype, const MISSION_INFO & info);

	/* EffectLauncherFactory methods */

	IDAComponent * getBase (void)
	{
		return static_cast<IObjectFactory *> (this);
	}
};
//--------------------------------------------------------------------------//
//
EffectLauncherFactory::~EffectLauncherFactory (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (OBJLIST && OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
		connection->Unadvise(factoryHandle);
}
//--------------------------------------------------------------------------//
//
void EffectLauncherFactory::init (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
		connection->Advise(getBase(), &factoryHandle);
}
//-----------------------------------------------------------------------------
//
HANDLE EffectLauncherFactory::CreateArchetype (const char *szArchname, OBJCLASS objClass, void *_data)
{
	EffectLauncherArchetype * result = 0;

	if (objClass == OC_LAUNCHER)
	{
		BT_EFFECT_LAUNCHER * data = (BT_EFFECT_LAUNCHER *) _data;

		if (data->type == LC_EFFECT_LAUNCHER)	   
		{
			result = new EffectLauncherArchetype;
			result->pArchetype = ARCHLIST->GetArchetype(szArchname);

			if(data->weaponType[0])
				result->effect = EFFECTPLAYER->LoadEffect(data->weaponType);
			else
				result->effect = NULL;
 
			result->data = data;	
		}
	}

	return (HANDLE) result;
}
//-------------------------------------------------------------------
//
BOOL32 EffectLauncherFactory::DestroyArchetype (HANDLE hArchetype)
{
	EffectLauncherArchetype * objtype = (EffectLauncherArchetype *)hArchetype;

	delete objtype;

	return 1;
}
//-------------------------------------------------------------------
//
IBaseObject * EffectLauncherFactory::CreateInstance (HANDLE hArchetype)
{
	return createEffectLauncher((EffectLauncherArchetype *)hArchetype);
}
//-------------------------------------------------------------------
//
void EffectLauncherFactory::EditorCreateInstance (HANDLE hArchetype, const MISSION_INFO & info)
{
	// not supported
}
//---------------------------------------------------------------------------
//-------------------------GLOBAL STARTUP------------------------------------
//---------------------------------------------------------------------------
//
struct _effectLauncher : GlobalComponent
{
	EffectLauncherFactory * sfactory;

	virtual void Startup (void)
	{
		sfactory = new DAComponent<EffectLauncherFactory>;
		AddToGlobalCleanupList((IDAComponent **) &sfactory);
	}

	virtual void Initialize (void)
	{
		sfactory->init();
	}
};

static _effectLauncher __effectLauncher;

//---------------------------------------------------------------------------------------------
//-------------------------------End EffectLauncher.cpp------------------------------------------------
//---------------------------------------------------------------------------------------------
