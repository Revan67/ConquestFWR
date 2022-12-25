//--------------------------------------------------------------------------//
//                                                                          //
//                                NovaLauncher.cpp                          //
//                                                                          //
//                  COPYRIGHT (C) 2004 WARTHOG, INC.                        //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header:
*/			    
//------------------------------- #INCLUDES --------------------------------//
//--------------------------------------------------------------------------//

#include "pch.h"
#include <globals.h>

#include "SuperTrans.h"
#include "ObjList.h"
#include "sfx.h"

#include "Mission.h"
#include "IMissionActor.h"
#include "Startup.h"
#include "ArchHolder.h"
#include "TerrainMap.h"
#include "MPart.h"
#include "ILauncher.h"
#include "IObject.h"
#include "objwatch.h"
#include "TObject.h"
#include "Sector.h"
#include "OpAgent.h"
#include "ObjMapIterator.h"
#include "IWeapon.h"
#include "UnitComm.h"
#include "INovaExplosion.h"

#include <DNovaLauncher.h>

#include <Engine.h>
#include <TSmartPointer.h>
#include <IAnim.h>
#include <FileSys.h>
#include <TComponent.h>
#include <IConnection.h> 
#include <MGlobals.h>
#include <DMTechNode.h>

#include <stdlib.h>

// so that we can use the techtree stuff
using namespace TECHTREE;

struct _NO_VTABLE NovaLaunch : IBaseObject, ILauncher, ISaveLoad, NOVA_LAUNCHER_SAVELOAD
{
	BEGIN_MAP_INBOUND(NovaLaunch)
	_INTERFACE_ENTRY(IBaseObject)
	_INTERFACE_ENTRY(ISaveLoad)
	_INTERFACE_ENTRY(ILauncher)
	END_MAP()

	OBJPTR<ILaunchOwner> owner;	// person who created us
	const BT_NOVA_LAUNCHER * pData;

	U32 nextWarningTime;
	bool bExploded;

	//----------------------------------
	//----------------------------------
	
	NovaLaunch (void);

	~NovaLaunch (void);	

	/* IBaseObject methods */

	virtual const TRANSFORM & GetTransform (void) const;

	virtual BOOL32 Update (void);	// returning FALSE causes destruction

	virtual void PhysicalUpdate(SINGLE dt);

	virtual void Render();

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

		ability = USA_NOVA;
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
		return 0;
	}

	virtual U32 GetSyncData (void * buffer)
	{
		return 0;
	}

	virtual void PutSyncData (void * buffer, U32 bufferSize)
	{
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


	/* NovaLauncher methods */
	
	void init (PARCHETYPE pArchetype);

	bool checkSupplies();

};
//---------------------------------------------------------------------------
//
NovaLaunch::NovaLaunch (void) 
{
	bNovaOn = false;
	bExploded = false;
	timer = 0;
}
//---------------------------------------------------------------------------
//
NovaLaunch::~NovaLaunch (void)
{
}
//---------------------------------------------------------------------------
//
void NovaLaunch::init (PARCHETYPE _pArchetype)
{
	const BT_NOVA_LAUNCHER * data = (const BT_NOVA_LAUNCHER *) ARCHLIST->GetArchetypeData(_pArchetype);

	CQASSERT(data);
	CQASSERT(data->type == LC_NOVA_LAUNCH);
	CQASSERT(data->objClass == OC_LAUNCHER);

	pArchetype = _pArchetype;
	objClass = OC_LAUNCHER;

}
//---------------------------------------------------------------------------
//
void NovaLaunch::DoSpecialAbility (U32 specialID)
{
	if(bNovaOn)
		return;
	MPart part = owner.Ptr();
	if (part.isValid() && part->hullPoints==0)
		return;		// don't do this ability if already dead!

	if (checkSupplies())
	{
		timer = 0;
		bNovaOn = true;
		nextWarningTime = 0;
		MPartNC partNC = owner.Ptr();
		partNC->caps.specialAbilityOk = false;
	}
}
//---------------------------------------------------------------------------
//
S32 NovaLaunch::GetObjectIndex (void) const
{
	// not too sure about this...
	return owner.Ptr()->GetObjectIndex();
}
//---------------------------------------------------------------------------
//
U32 NovaLaunch::GetPartID (void) const
{
	return owner.Ptr()->GetPartID();
}
//---------------------------------------------------------------------------
//
#define EXPLOSION_GRID_PER_SEC 5

BOOL32 NovaLaunch::Update (void)
{
	if(bNovaOn && THEMATRIX->IsMaster())
	{
		if(timer > nextWarningTime)
		{
			if(nextWarningTime == 0)
			{
				COMM_NOVA_ALLERT(owner.Ptr(),owner.Ptr()->GetPartID(),"warning.wav",NOVA_WARNING_30);
			}
			else if(nextWarningTime == 5)
			{
				COMM_NOVA_ALLERT(owner.Ptr(),owner.Ptr()->GetPartID(),"warning.wav",NOVA_WARNING_25);
			}
			else if(nextWarningTime == 10)
			{
				COMM_NOVA_ALLERT(owner.Ptr(),owner.Ptr()->GetPartID(),"warning.wav",NOVA_WARNING_20);
			}
			else if(nextWarningTime == 15)
			{
				COMM_NOVA_ALLERT(owner.Ptr(),owner.Ptr()->GetPartID(),"warning.wav",NOVA_WARNING_15);
			}
			else if(nextWarningTime == 20)
			{
				COMM_NOVA_ALLERT(owner.Ptr(),owner.Ptr()->GetPartID(),"warning.wav",NOVA_WARNING_10);
			}
			else if(nextWarningTime == 25)
			{
				COMM_NOVA_ALLERT(owner.Ptr(),owner.Ptr()->GetPartID(),"warning.wav",NOVA_WARNING_5);
			}
			nextWarningTime+= 5;
			if(nextWarningTime > pData->chargeTime)
			{
				nextWarningTime += 10000;
			}
		}
		if(timer > pData->chargeTime)
		{
			SINGLE radius = (timer-pData->chargeTime)*EXPLOSION_GRID_PER_SEC*GRIDSIZE;
			ObjMapIterator iter(owner.Ptr()->GetSystemID(),owner.Ptr()->GetPosition(),radius);
			while(iter)
			{
				if ((iter->obj != owner.Ptr()) && (iter->flags & OM_UNTOUCHABLE) == 0 && (iter->flags & OM_TARGETABLE))
				{
					if(iter->obj->GetGridPosition()-owner.Ptr()->GetGridPosition() < radius)
					{
						VOLPTR(IWeaponTarget) target = iter->obj;
						target->ApplyAOEDamage(owner.Ptr()->GetPartID(),10000);
					}
				}

				++iter;
			}
		}

		if(timer > pData->chargeTime+(64/EXPLOSION_GRID_PER_SEC))
		{
			VOLPTR(IWeaponTarget) target = owner.Ptr();
			target->ApplyAOEDamage(owner.Ptr()->GetPartID(),10000);
		}
	}
	return 1;
}
//---------------------------------------------------------------------------
//
void NovaLaunch::PhysicalUpdate(SINGLE dt)
{
	if(bNovaOn)
	{
		SECTOR->AddNovaBomb(owner.Ptr()->GetSystemID());
		timer += dt;
		if(timer > pData->chargeTime && (!bExploded))
		{
			bExploded = true;
			IBaseObject * obj = ARCHLIST->CreateInstance(pData->novaExplosion);
			if(obj)
			{
				VOLPTR(INovaExplosion) nova = obj;
				if(nova)
					nova->InitNovaExplosion(owner.Ptr());
				OBJLIST->AddObject(obj);
			}
		}
	}
}
//---------------------------------------------------------------------------
//
void NovaLaunch::Render()
{
}
//---------------------------------------------------------------------------
//
void NovaLaunch::InitLauncher (IBaseObject * _owner, S32 ownerIndex, S32 animArcheIndex, SINGLE range)
{
	const BT_NOVA_LAUNCHER * data = (const BT_NOVA_LAUNCHER *) ARCHLIST->GetArchetypeData(pArchetype);
	pData = data;
	CQASSERT(_owner);
	_owner->QueryInterface(ILaunchOwnerID, owner, LAUNCHVOLATILEPTR);
	CQASSERT(owner!=0);
}
//---------------------------------------------------------------------------
//
const TRANSFORM & NovaLaunch::GetTransform (void) const
{
	return owner.Ptr()->GetTransform();
}
//---------------------------------------------------------------------------
//
bool NovaLaunch::checkSupplies()
{
	return SECTOR->SystemInSupply(owner.Ptr()->GetSystemID(),owner.Ptr()->GetPlayerID());
}
//---------------------------------------------------------------------------
//
inline IBaseObject * createNovaLaunch (PARCHETYPE pArchetype)
{
	NovaLaunch * novaLaunch = new ObjectImpl<NovaLaunch>;

	novaLaunch->init(pArchetype);

	return novaLaunch;
}
//------------------------------------------------------------------------------------------
//---------------------------NovaLaunch Factory------------------------------------------
//------------------------------------------------------------------------------------------
//
struct DACOM_NO_VTABLE NovaLaunchFactory : public IObjectFactory
{
	struct OBJTYPE 
	{
		PARCHETYPE pArchetype;
		PARCHETYPE pBoltType;

		void * operator new (size_t size)
		{
			return calloc(size,1);
		}

		OBJTYPE (void)
		{
		}

		~OBJTYPE (void)
		{
			if (pBoltType)
				ARCHLIST->Release(pBoltType, OBJREFNAME);
		}
	};
	//------------------------------------------------------------------------------------------
	//
	U32 factoryHandle;		// handles to callback

	//
	// Interface mapping
	//

	BEGIN_DACOM_MAP_INBOUND(NovaLaunchFactory)
	DACOM_INTERFACE_ENTRY(IObjectFactory)
	END_DACOM_MAP()

	NovaLaunchFactory (void) { }

	~NovaLaunchFactory (void);

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

	/* NovaLaunchFactory methods */

	IDAComponent * getBase (void)
	{
		return static_cast<IObjectFactory *> (this);
	}
};
//--------------------------------------------------------------------------//
//
NovaLaunchFactory::~NovaLaunchFactory (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (OBJLIST && OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
		connection->Unadvise(factoryHandle);
}
//--------------------------------------------------------------------------//
//
void NovaLaunchFactory::init (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
		connection->Advise(getBase(), &factoryHandle);
}
//-----------------------------------------------------------------------------
//
HANDLE NovaLaunchFactory::CreateArchetype (const char *szArchname, OBJCLASS objClass, void *_data)
{
	OBJTYPE * result = 0;

	if (objClass == OC_LAUNCHER)
	{
		BT_NOVA_LAUNCHER * data = (BT_NOVA_LAUNCHER *) _data;

		if (data->type == LC_NOVA_LAUNCH)	   
		{
			result = new OBJTYPE;
			result->pArchetype = ARCHLIST->GetArchetype(szArchname);
		}
	}

	return (HANDLE) result;
}
//-------------------------------------------------------------------
//
BOOL32 NovaLaunchFactory::DestroyArchetype (HANDLE hArchetype)
{
	OBJTYPE * objtype = (OBJTYPE *)hArchetype;

//	EditorStopObjectInsertion(objtype->pArchetype);		// not supported
	delete objtype;

	return 1;
}
//-------------------------------------------------------------------
//
IBaseObject * NovaLaunchFactory::CreateInstance (HANDLE hArchetype)
{
	OBJTYPE * objtype = (OBJTYPE *)hArchetype;
	return createNovaLaunch(objtype->pArchetype);
}
//-------------------------------------------------------------------
//
void NovaLaunchFactory::EditorCreateInstance (HANDLE hArchetype, const MISSION_INFO & info)
{
	// not supported
}
//---------------------------------------------------------------------------
//-------------------------GLOBAL STARTUP------------------------------------
//---------------------------------------------------------------------------
//
struct _novaLaunch : GlobalComponent
{
	NovaLaunchFactory * sfactory;

	virtual void Startup (void)
	{
		sfactory = new DAComponent<NovaLaunchFactory>;
		AddToGlobalCleanupList((IDAComponent **) &sfactory);
	}

	virtual void Initialize (void)
	{
		sfactory->init();
	}
};

static _novaLaunch __ship;

//---------------------------------------------------------------------------------------------
//-------------------------------End NovaLaunch.cpp------------------------------------------------
//---------------------------------------------------------------------------------------------