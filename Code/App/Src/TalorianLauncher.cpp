//--------------------------------------------------------------------------//
//                                                                          //
//                                TalorianLauncher.cpp                           //
//                                                                          //
//                  COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.               //
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
#include "TObject.h"
#include <DTalorianLauncher.h>
#include "ILauncher.h"
#include "IWeapon.h"
#include "DSpaceShip.h"
#include "IGotoPos.h"
#include "OpAgent.h"
#include "IShipMove.h"
#include "CommPacket.h"
#include "sector.h"
#include "ObjSet.h"
#include "IFabricator.h"
#include "IPlanet.h"
#include "ITalorianEffect.h"
#include "ObjMapIterator.h"

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

struct TalorianLauncherArchetype
{
	PARCHETYPE pTalorianEffectType;
};

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
struct _NO_VTABLE TalorianLauncher : IBaseObject, ILauncher, ISaveLoad, TALORIAN_LAUNCHER_SAVELOAD
{
	BEGIN_MAP_INBOUND(TalorianLauncher)
	_INTERFACE_ENTRY(IBaseObject)
	_INTERFACE_ENTRY(ISaveLoad)
	_INTERFACE_ENTRY(ILauncher)
	END_MAP()

	BT_TALORIAN_LAUNCHER * pData;

	OBJPTR<ILaunchOwner> owner;	// person who created us
	OBJPTR<ITalorianEffect> shield;

	OBJPTR<IPlanet> planetCach;

	//----------------------------------
	//----------------------------------
	
	TalorianLauncher (void);

	~TalorianLauncher (void);	

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

	virtual void AttackObject (IBaseObject * obj);

	virtual void AttackMultiSystem (struct GRIDVECTOR * position, U32 targSystemID)
	{}

	virtual void WormAttack (IBaseObject * obj)
	{};

	virtual void DoCreateWormhole(U32 systemID);

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
		ability = USA_NONE;
	}

	virtual const U32 GetApproxDamagePerSecond (void) const
	{
		return 0;
	}

	virtual void InformOfCancel();

	virtual void LauncherOpCreated(U32 agentID, void * buffer, U32 bufferSize);

	virtual void LauncherReceiveOpData(U32 agentID, void * buffer, U32 bufferSize);

	virtual void LauncherOpCompleted(U32 agentID);

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

	/* TalorianLauncher methods */
	
	void init (PARCHETYPE pArchetype);

	bool checkSupplies();

	IBaseObject * findPlanet();
};

//---------------------------------------------------------------------------
//
TalorianLauncher::TalorianLauncher (void) 
{
}
//---------------------------------------------------------------------------
//
TalorianLauncher::~TalorianLauncher (void)
{
	if(shield)
		shield->CloseUp();
	if(numTargets)
	{
		for(U32 i = 0; i < numTargets; ++i)
		{
			OBJPTR<IBaseObject> obj;
			OBJLIST->FindObject(targetIDs[i],NONSYSVOLATILEPTR,obj,IBaseObjectID);
			if(obj)
			{
				obj->effectFlags.bTalorianShield = false;
			}
		}
		numTargets = 0;
	}
}
//---------------------------------------------------------------------------
//
void TalorianLauncher::init (PARCHETYPE _pArchetype)
{
	pData = (BT_TALORIAN_LAUNCHER *) ARCHLIST->GetArchetypeData(_pArchetype);

	CQASSERT(pData);
	CQASSERT(pData->type == LC_TALORIAN_LAUNCH);
	CQASSERT(pData->objClass == OC_LAUNCHER);

	pArchetype = _pArchetype;
	objClass = OC_LAUNCHER;

}
//---------------------------------------------------------------------------
//
void TalorianLauncher::DoSpecialAbility (U32 specialID)
{

}
//---------------------------------------------------------------------------
//
S32 TalorianLauncher::GetObjectIndex (void) const
{
	// not too sure about this...
	return owner.Ptr()->GetObjectIndex();
}
//---------------------------------------------------------------------------
//
U32 TalorianLauncher::GetPartID (void) const
{
	return owner.Ptr()->GetPartID();
}
//---------------------------------------------------------------------------
//
BOOL32 TalorianLauncher::Update (void)
{
	if(!shield)
	{
		TalorianLauncherArchetype * archData = (TalorianLauncherArchetype *)(ARCHLIST->GetArchetypeHandle(pArchetype));
		if(archData->pTalorianEffectType)
		{
			IBaseObject * obj = ARCHLIST->CreateInstance(archData->pTalorianEffectType);
			CQASSERT(obj);
			obj->QueryInterface(ITalorianEffectID,shield,NONSYSVOLATILEPTR);
			OBJLIST->AddObject(obj);
			CQASSERT(shield);

			VOLPTR(IPlanet) planet = findPlanet();
			TRANSFORM trans;
			if(planet)
				trans = planet.Ptr()->GetTransform();
			else
				trans = owner.Ptr()->GetTransform();
			shield->InitTalorianEffect(trans,owner.Ptr()->GetSystemID(),owner.Ptr()->GetPlayerID());
		}
	}
	MPart part(owner.Ptr());
	if(part->bReady && SECTOR->SystemInSupply(owner.Ptr()->GetSystemID(),owner.Ptr()->GetPlayerID()))
	{
		if(shield && !shield->IsActive())
			shield->WarmUp();
		VOLPTR(IPlanet) planet = findPlanet();
		if(planet)
		{
			for(U32 i = 0; i < 12; ++i)
			{
				U32 slotOwner = planet->GetSlotUser(0x01 << i);
				if(slotOwner && numTargets < MAX_SHIELD_VICTIMS && MGlobals::AreAllies(owner.Ptr()->GetPlayerID(),MGlobals::GetPlayerFromPartID(slotOwner)))
				{
					OBJPTR<IBaseObject> obj;
					OBJLIST->FindObject(slotOwner,NONSYSVOLATILEPTR,obj,IBaseObjectID);
					if(obj && (!obj->effectFlags.bTalorianShield))
					{
						targetIDs[numTargets] = obj->GetPartID();
						++numTargets;
						obj->effectFlags.bTalorianShield = true;
					}
				}
			}
		}
		for(U32 i = 0; i < numTargets; ++i)
		{
			OBJPTR<IBaseObject> obj;
			OBJLIST->FindObject(targetIDs[i],NONSYSVOLATILEPTR,obj,IBaseObjectID);
			if(!obj)
			{
				if(i+1 != numTargets)
				{
					memmove(&(targetIDs[i]),&(targetIDs[i+1]),(numTargets-i-1)*sizeof(U32));
				}
				--numTargets;//this might skip people to remove but we will get them on the next update
			}
		}
	}
	else
	{
		if(shield && shield->IsActive())
			shield->ShutDown();
		if(numTargets)
		{
			for(U32 i = 0; i < numTargets; ++i)
			{
				OBJPTR<IBaseObject> obj;
				OBJLIST->FindObject(targetIDs[i],NONSYSVOLATILEPTR,obj,IBaseObjectID);
				if(obj)
				{
					obj->effectFlags.bTalorianShield = false;
				}
			}
			numTargets = 0;
		}
	}
	return 1;
}
//---------------------------------------------------------------------------
//
void TalorianLauncher::InitLauncher (IBaseObject * _owner, S32 ownerIndex, S32 animArcheIndex, SINGLE range)
{
	CQASSERT(_owner);
	_owner->QueryInterface(ILaunchOwnerID, owner, LAUNCHVOLATILEPTR);
	CQASSERT(owner!=0);
}
//---------------------------------------------------------------------------
//
void TalorianLauncher::InformOfCancel()
{
}
//---------------------------------------------------------------------------
//
void TalorianLauncher::LauncherOpCreated(U32 agentID, void * buffer, U32 bufferSize)
{
}
//---------------------------------------------------------------------------
//
void TalorianLauncher::LauncherReceiveOpData(U32 workingID, void * buffer, U32 bufferSize)
{
}
//---------------------------------------------------------------------------
//
void TalorianLauncher::LauncherOpCompleted(U32 agentID)
{
}
//---------------------------------------------------------------------------
//
void TalorianLauncher::AttackObject (IBaseObject * obj)
{
}
//---------------------------------------------------------------------------
//
void TalorianLauncher::DoCreateWormhole(U32 systemID)
{
}

//---------------------------------------------------------------------------
//
const TRANSFORM & TalorianLauncher::GetTransform (void) const
{
	return owner.Ptr()->GetTransform();
}
//---------------------------------------------------------------------------
//
bool TalorianLauncher::checkSupplies()
{
	return true;
}
//---------------------------------------------------------------------------
//
IBaseObject * TalorianLauncher::findPlanet()
{
	if(planetCach)
		return planetCach.Ptr();
	IBaseObject * bestPlanet = NULL;
	SINGLE bestDist = 0;
	ObjMapIterator iter(owner.Ptr()->GetSystemID(),owner.Ptr()->GetPosition(),7*GRIDSIZE);
	while(iter)
	{
		if(iter->obj->objClass == OC_PLANETOID)
		{
			VOLPTR(IPlanet) testPlanet = iter->obj;
			if(testPlanet && (!(testPlanet->IsMoon())))
			{
				if(bestPlanet)
				{
					SINGLE testDist = owner.Ptr()->GetGridPosition()-iter->obj->GetGridPosition();
					if(testDist < bestDist)
					{
						bestPlanet = iter->obj;
						bestDist = testDist;
					}
				}
				else
				{
					bestPlanet = iter->obj;
					bestDist = owner.Ptr()->GetGridPosition()-iter->obj->GetGridPosition();
				}
			}
		}
		++iter;
	}
	if(bestPlanet)
	{
		bestPlanet->QueryInterface(IPlanetID,planetCach,NONSYSVOLATILEPTR);
	}
	else
	{
		planetCach = NULL;
	}
	return bestPlanet;
}
//---------------------------------------------------------------------------
//
inline IBaseObject * createTalorianLauncher (PARCHETYPE pArchetype)
{
	TalorianLauncher * talorianLauncher = new ObjectImpl<TalorianLauncher>;

	talorianLauncher->init(pArchetype);

	return talorianLauncher;
}
//------------------------------------------------------------------------------------------
//---------------------------TalorianLauncher Factory------------------------------------------
//------------------------------------------------------------------------------------------
//
struct DACOM_NO_VTABLE TalorianLauncherFactory : public IObjectFactory
{
	struct OBJTYPE : TalorianLauncherArchetype
	{
		PARCHETYPE pArchetype;

		void * operator new (size_t size)
		{
			return calloc(size,1);
		}

		OBJTYPE (void)
		{
		}

		~OBJTYPE (void)
		{
			if(pTalorianEffectType)
				ARCHLIST->Release(pTalorianEffectType, OBJREFNAME);

		}
	};
	//------------------------------------------------------------------------------------------
	//
	U32 factoryHandle;		// handles to callback

	//
	// Interface mapping
	//

	BEGIN_DACOM_MAP_INBOUND(TalorianLauncherFactory)
	DACOM_INTERFACE_ENTRY(IObjectFactory)
	END_DACOM_MAP()

	TalorianLauncherFactory (void) { }

	~TalorianLauncherFactory (void);

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

	/* TalorianLauncherFactory methods */

	IDAComponent * getBase (void)
	{
		return static_cast<IObjectFactory *> (this);
	}
};
//--------------------------------------------------------------------------//
//
TalorianLauncherFactory::~TalorianLauncherFactory (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (OBJLIST && OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
		connection->Unadvise(factoryHandle);
}
//--------------------------------------------------------------------------//
//
void TalorianLauncherFactory::init (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
		connection->Advise(getBase(), &factoryHandle);
}
//-----------------------------------------------------------------------------
//
HANDLE TalorianLauncherFactory::CreateArchetype (const char *szArchname, OBJCLASS objClass, void *_data)
{
	OBJTYPE * result = 0;

	if (objClass == OC_LAUNCHER)
	{
		BT_TALORIAN_LAUNCHER * data = (BT_TALORIAN_LAUNCHER *) _data;

		if (data->type == LC_TALORIAN_LAUNCH)	   
		{
			result = new OBJTYPE;
			result->pArchetype = ARCHLIST->GetArchetype(szArchname);
			result->pTalorianEffectType = ARCHLIST->LoadArchetype("TALORIANEFFECT!!S_SHIELD");
			if(result->pTalorianEffectType)
				ARCHLIST->AddRef(result->pTalorianEffectType, OBJREFNAME);
		}
	}

	return (HANDLE) result;
}
//-------------------------------------------------------------------
//
BOOL32 TalorianLauncherFactory::DestroyArchetype (HANDLE hArchetype)
{
	OBJTYPE * objtype = (OBJTYPE *)hArchetype;

//	EditorStopObjectInsertion(objtype->pArchetype);		// not supported
	delete objtype;

	return 1;
}
//-------------------------------------------------------------------
//
IBaseObject * TalorianLauncherFactory::CreateInstance (HANDLE hArchetype)
{
	OBJTYPE * objtype = (OBJTYPE *)hArchetype;
	return createTalorianLauncher(objtype->pArchetype);
}
//-------------------------------------------------------------------
//
void TalorianLauncherFactory::EditorCreateInstance (HANDLE hArchetype, const MISSION_INFO & info)
{
	// not supported
}
//---------------------------------------------------------------------------
//-------------------------GLOBAL STARTUP------------------------------------
//---------------------------------------------------------------------------
//
struct _talorianLauncher : GlobalComponent
{
	TalorianLauncherFactory * sfactory;

	virtual void Startup (void)
	{
		sfactory = new DAComponent<TalorianLauncherFactory>;
		AddToGlobalCleanupList((IDAComponent **) &sfactory);
	}

	virtual void Initialize (void)
	{
		sfactory->init();
	}
};

static _talorianLauncher __ship;

//---------------------------------------------------------------------------------------------
//-------------------------------End TalorianLauncher.cpp------------------------------------------------
//---------------------------------------------------------------------------------------------