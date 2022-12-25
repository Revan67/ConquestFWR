//--------------------------------------------------------------------------//
//                                                                          //
//                              BlockadeRunner.cpp                          //
//                                                                          //
//                  COPYRIGHT (C) 2004 By Warthog TX, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header:
*/			    
//------------------------------- #INCLUDES --------------------------------//
//--------------------------------------------------------------------------//

#include "pch.h"
#include <globals.h>

#include "Startup.h"
#include "ArchHolder.h"
#include "MPart.h"
#include <DArtifacts.h>
#include "IArtifact.h"

#include <TSmartPointer.h>
#include <TComponent.h>
#include <IConnection.h> 

#include <stdlib.h>


struct BlockadeRunnerArchetype
{
};

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
struct _NO_VTABLE BlockadeRunner : IBaseObject, IArtifact
{
	BEGIN_MAP_INBOUND(BlockadeRunner)
	_INTERFACE_ENTRY(IBaseObject)
	_INTERFACE_ENTRY(IArtifact)
	END_MAP()

	BT_BLOCKADERUNNER_LAUNCHER * pData;

	OBJPTR<ILaunchOwner> owner;	// person who created us
	U32 ownerID;

	OBJPTR<IPlanet> planet;

	OBJPTR<IParticleCircle> visual;

	SINGLE supplyTimer;
	bool bNetToggleOn;

	//----------------------------------
	//----------------------------------
	
	BuffLauncher (void);

	~BuffLauncher (void);	

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

	virtual bool IsToggle();

	virtual bool CanToggle();

	virtual bool IsOn();

	// the following methods are for network synchronization of realtime objects
	virtual U32 GetSyncDataSize (void) const;

	virtual U32 GetSyncData (void * buffer);

	virtual void PutSyncData (void * buffer, U32 bufferSize);

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

	/* BuffLauncher methods */
	
	void init (PARCHETYPE pArchetype);

	bool checkSupplies();

	bool checkTargetType(IBaseObject  * obj);

	void setEffectFlag(IBaseObject  * obj, bool bSetting);

	bool checkEffectFlag(IBaseObject * obj);

	bool isTarget(IBaseObject * obj);

	IBaseObject * findPlanet();
};

//---------------------------------------------------------------------------
//
BuffLauncher::BuffLauncher (void) 
{
	supplyTimer = 0;
	bToggleOn = false;
}
//---------------------------------------------------------------------------
//
BuffLauncher::~BuffLauncher (void)
{
	if(numTargets)
	{
		for(U32 i = 0; i < numTargets; ++i)
		{
			OBJPTR<IBaseObject> obj;
			OBJLIST->FindObject(targetIDs[i],NONSYSVOLATILEPTR,obj,IBaseObjectID);
			if(obj)
			{
				setEffectFlag(obj,false);
			}
		}
		numTargets = 0;
	}
}
//---------------------------------------------------------------------------
//
void BuffLauncher::init (PARCHETYPE _pArchetype)
{
	pData = (BT_BUFF_LAUNCHER *) ARCHLIST->GetArchetypeData(_pArchetype);

	CQASSERT(pData);
	CQASSERT(pData->type == LC_BUFF_LAUNCHER);
	CQASSERT(pData->objClass == OC_LAUNCHER);

	pArchetype = _pArchetype;
	objClass = OC_LAUNCHER;

}
// the following methods are for network synchronization of realtime objects
U32 BuffLauncher::GetSyncDataSize (void) const
{
	if(bToggleOn != bNetToggleOn)
	{
		return 1;
	}
	return 0;
}

U32 BuffLauncher::GetSyncData (void * buffer)			// buffer points to use supplied memory
{
	if(bToggleOn != bNetToggleOn)
	{
		bNetToggleOn = bToggleOn ;
		*((U8 *)buffer) = bToggleOn ;
		return 1;
	}
	return 0;
}
//---------------------------------------------------------------------------
//
void BuffLauncher::PutSyncData (void * buffer, U32 bufferSize)
{
	bNetToggleOn = ((*((U8 *)buffer)) != 0 );
	if(bToggleOn  != bNetToggleOn)
	{
		bToggleOn  = bNetToggleOn;
		if(visual)
			visual->SetActive(bToggleOn);
	}
}
//---------------------------------------------------------------------------
//
void BuffLauncher::DoSpecialAbility (U32 specialID)
{
	if(THEMATRIX->IsMaster())
	{
		if(bToggleOn)
		{
			bToggleOn = false;
		}
		else
		{
			if(specialID == pData->launcherSpecialID)
			{
				MPart part(owner.Ptr()); 
				if(part->supplies > 0)
				{
					bToggleOn = true;
				}
			}
		}
		if(visual)
			visual->SetActive(bToggleOn);
	}
}
//---------------------------------------------------------------------------
//
S32 BuffLauncher::GetObjectIndex (void) const
{
	// not too sure about this...
	return owner.Ptr()->GetObjectIndex();
}
//---------------------------------------------------------------------------
//
U32 BuffLauncher::GetPartID (void) const
{
	return owner.Ptr()->GetPartID();
}
//---------------------------------------------------------------------------
//
BOOL32 BuffLauncher::Update (void)
{
	MPart part(owner.Ptr());
	if(part->bReady && checkSupplies())
	{
		if(pData->targetType == BT_BUFF_LAUNCHER::ALLIES_PLANET_ONLY)
		{
			if(!planet)
			{
				IBaseObject * pl = findPlanet();
				if(pl)
				{
					pl->QueryInterface(IPlanetID,planet,NONSYSVOLATILEPTR);
				}
				CQASSERT(planet);
			}
			if(planet)
			{
				for(U32 i = 0; i < 12; ++ i)
				{
					if(numTargets < MAX_BUFF_VICTIMS)
					{
						IBaseObject * obj = OBJLIST->FindObject(planet->GetSlotUser(0x01<<i));
						if(obj && MGlobals::AreAllies(obj->GetPlayerID(),owner.Ptr()->GetPlayerID()) && (!checkEffectFlag(obj)) && (!isTarget(obj)))
						{
							targetIDs[numTargets] = obj->GetPartID();
							++numTargets;
							setEffectFlag(obj,true);
						}
					}
					else
						break;
				}
			}
		}
		else
		{
			ObjMapIterator iter(part->systemID,owner.Ptr()->GetPosition(),pData->rangeRadius*GRIDSIZE);
			while(iter)
			{
				if(numTargets < MAX_BUFF_VICTIMS)
				{
					if ((iter->flags & OM_UNTOUCHABLE) == 0 && (iter->flags & OM_TARGETABLE))
					{
						MPart part = iter->obj;
						if(part.isValid() && part->bReady && (checkTargetType(iter->obj)) && 
							(!checkEffectFlag(iter->obj)) && (GetGridPosition() - iter->obj->GetGridPosition()) < pData->rangeRadius)
						{
							targetIDs[numTargets] = iter->obj->GetPartID();
							++numTargets;
							setEffectFlag(iter->obj,true);
						}
					}
				}
				else
				{
					break;//already full
				}
				++iter;
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
			else if(pData->targetType != BT_BUFF_LAUNCHER::ALLIES_PLANET_ONLY)
			{
				if(obj->GetSystemID() != owner.Ptr()->GetSystemID() || (GetGridPosition() - obj->GetGridPosition()) >= pData->rangeRadius)//if he outside of range
				{
					setEffectFlag(obj,false);
					if(i+1 != numTargets)
					{
						memmove(&(targetIDs[i]),&(targetIDs[i+1]),(numTargets-i-1) *sizeof(U32));
					}
					--numTargets;//this might skip people to remove but we will get them on the next update
				}
			}
		}
	}
	else
	{
		if(numTargets)
		{
			for(U32 i = 0; i < numTargets; ++i)
			{
				OBJPTR<IBaseObject> obj;
				OBJLIST->FindObject(targetIDs[i],NONSYSVOLATILEPTR,obj,IBaseObjectID);
				if(obj)
				{
					setEffectFlag(obj,false);
				}
			}
			numTargets = 0;
		}
	}
	return 1;
}
//---------------------------------------------------------------------------
//
void BuffLauncher::InitLauncher (IBaseObject * _owner, S32 ownerIndex, S32 animArcheIndex, SINGLE range)
{
	CQASSERT(_owner);
	_owner->QueryInterface(ILaunchOwnerID, owner, LAUNCHVOLATILEPTR);
	CQASSERT(owner!=0);
	ownerID = _owner->GetPartID();

	IBaseObject * obj = ARCHLIST->CreateInstance(pData->visualName);
	if(obj)
	{
		obj->QueryInterface(IParticleCircleID,visual,NONSYSVOLATILEPTR);
		if(visual)
		{
			visual->InitParticleCircle(_owner,pData->rangeRadius*GRIDSIZE);
			visual->SetActive(false);
			OBJLIST->AddObject(visual.Ptr());
		}
	}
}
//---------------------------------------------------------------------------
//
void BuffLauncher::InformOfCancel()
{
}
//---------------------------------------------------------------------------
//
void BuffLauncher::LauncherOpCreated(U32 agentID, void * buffer, U32 bufferSize)
{
}
//---------------------------------------------------------------------------
//
void BuffLauncher::LauncherReceiveOpData(U32 workingID, void * buffer, U32 bufferSize)
{
}
//---------------------------------------------------------------------------
//
void BuffLauncher::LauncherOpCompleted(U32 agentID)
{
}
//---------------------------------------------------------------------------
//
bool BuffLauncher::IsToggle()
{
	return pData->supplyUseType == BT_BUFF_LAUNCHER::SU_SHIP_SUPPLIES_TOGGLE;
}
//---------------------------------------------------------------------------
//
bool BuffLauncher::CanToggle()
{
	return pData->supplyUseType == BT_BUFF_LAUNCHER::SU_SHIP_SUPPLIES_TOGGLE;
}
//---------------------------------------------------------------------------
//
bool BuffLauncher::IsOn()
{
	return bToggleOn;
}
//---------------------------------------------------------------------------
//
void BuffLauncher::AttackObject (IBaseObject * obj)
{
}
//---------------------------------------------------------------------------
//
void BuffLauncher::DoCreateWormhole(U32 systemID)
{
}

//---------------------------------------------------------------------------
//
const TRANSFORM & BuffLauncher::GetTransform (void) const
{
	return owner.Ptr()->GetTransform();
}
//---------------------------------------------------------------------------
//
bool BuffLauncher::checkSupplies()
{
	switch(pData->supplyUseType)
	{
	case BT_BUFF_LAUNCHER::SU_PLATFORM_STANDARD:
		{
			return SECTOR->SystemInSupply(owner.Ptr()->GetSystemID(),owner.Ptr()->GetPlayerID());
		}
		break;
	case BT_BUFF_LAUNCHER::SU_SHIP_SUPPLIES_TOGGLE:
		{
			if(bToggleOn)
			{
				supplyTimer += ELAPSED_TIME;
				if(supplyTimer > 1.0)
				{
					supplyTimer -= 1.0;
					if(owner->UseSupplies(pData->supplyCost))
					{
						return true;
					}
					else
					{
						bToggleOn = false;
						if(visual)
							visual->SetActive(bToggleOn);
						return false;
					}
				}
				return true;
			}
			return false;
		}
		break;
	}
	return true;
}
//---------------------------------------------------------------------------
//
bool BuffLauncher::checkTargetType(IBaseObject * obj)
{
	if(!obj)
		return false;
	switch(pData->targetType)
	{
	case BT_BUFF_LAUNCHER::ALLIES_ONLY:
		return 	MGlobals::AreAllies(obj->GetPlayerID(),MGlobals::GetPlayerFromPartID(ownerID));
	case BT_BUFF_LAUNCHER::ENEMIES_ONLY:
		return 	!MGlobals::AreAllies(obj->GetPlayerID(),MGlobals::GetPlayerFromPartID(ownerID));
	case BT_BUFF_LAUNCHER::ALL_TARGETS:
		return true;
	case BT_BUFF_LAUNCHER::ALLIES_SHIPS_ONLY:
		return 	(obj->objClass == OC_SPACESHIP) && MGlobals::AreAllies(obj->GetPlayerID(),MGlobals::GetPlayerFromPartID(ownerID));
	case BT_BUFF_LAUNCHER::ALLIES_PLATS_ONLY:
		return 	(obj->objClass == OC_PLATFORM) && MGlobals::AreAllies(obj->GetPlayerID(),MGlobals::GetPlayerFromPartID(ownerID));
	case BT_BUFF_LAUNCHER::ENEMIES_SHIPS_ONLY:
		return 	(obj->objClass == OC_SPACESHIP) && (!MGlobals::AreAllies(obj->GetPlayerID(),MGlobals::GetPlayerFromPartID(ownerID)));
	case BT_BUFF_LAUNCHER::ENEMIES_PLATS_ONLY:
		return 	(obj->objClass == OC_PLATFORM) && (!MGlobals::AreAllies(obj->GetPlayerID(),MGlobals::GetPlayerFromPartID(ownerID)));
	case BT_BUFF_LAUNCHER::SHIPS_ONLY:
		return (obj->objClass == OC_SPACESHIP);
	case BT_BUFF_LAUNCHER::PLATS_ONLY:
		return (obj->objClass == OC_PLATFORM);
	}
	return false;
}
//---------------------------------------------------------------------------
//
void BuffLauncher::setEffectFlag(IBaseObject  * obj, bool bSetting)
{
	if(!obj)
		return;
	switch(pData->buffType)
	{
	case BT_BUFF_LAUNCHER::RESEARCH_EFFECT:
		{
			//there are up to four flags
			if(obj->effectFlags.bResearchBoost1 != bSetting)
				obj->effectFlags.bResearchBoost1 = bSetting;
			else if(obj->effectFlags.bResearchBoost2 != bSetting)
				obj->effectFlags.bResearchBoost2 = bSetting;
			else if(obj->effectFlags.bResearchBoost3 != bSetting)
				obj->effectFlags.bResearchBoost3 = bSetting;
			else if(obj->effectFlags.bResearchBoost4 != bSetting)
				obj->effectFlags.bResearchBoost4 = bSetting;
			else
			{
				CQASSERT(0);
			}
		}
		break;
	case BT_BUFF_LAUNCHER::CONSTRUCTION_EFFECT:
		{
			//there are up to four flags
			if(obj->effectFlags.bIndusrialBoost1 != bSetting)
				obj->effectFlags.bIndusrialBoost1 = bSetting;
			else if(obj->effectFlags.bIndusrialBoost2 != bSetting)
				obj->effectFlags.bIndusrialBoost2 = bSetting;
			else if(obj->effectFlags.bIndusrialBoost3 != bSetting)
				obj->effectFlags.bIndusrialBoost3 = bSetting;
			else if(obj->effectFlags.bIndusrialBoost4 != bSetting)
				obj->effectFlags.bIndusrialBoost4 = bSetting;
			else
			{
				CQASSERT(0);
			}
		}
		break;
	case BT_BUFF_LAUNCHER::SHIELD_JAM_EFFECT:
		{
			CQASSERT(obj->effectFlags.bShieldJam != bSetting);//if this trips it means that the list was corrupted, someone may be left buffed forever.
			obj->effectFlags.bShieldJam = bSetting;
		}
		break;
	case BT_BUFF_LAUNCHER::WEAPON_JAM_EFFECT:
		{
			CQASSERT(obj->effectFlags.bWeaponJam != bSetting);//if this trips it means that the list was corrupted, someone may be left buffed forever.
			obj->effectFlags.bWeaponJam = bSetting;
		}
		break;
	case BT_BUFF_LAUNCHER::SENSOR_JAM_EFFECT:
		{
			CQASSERT(obj->effectFlags.bSensorJam != bSetting);//if this trips it means that the list was corrupted, someone may be left buffed forever.
			obj->effectFlags.bSensorJam = bSetting;
		}
		break;
	case BT_BUFF_LAUNCHER::SINUATOR_EFFECT:
		CQASSERT(obj->effectFlags.bSinuator != bSetting);//if this trips it means that the list was corrupted, someone may be left buffed forever.
		obj->effectFlags.bSinuator = bSetting;
		break;
	case BT_BUFF_LAUNCHER::STASIS_EFFECT:
		CQASSERT(obj->effectFlags.bStasis != bSetting);//if this trips it means that the list was corrupted, someone may be left buffed forever.
		obj->effectFlags.bStasis = bSetting;
		break;
	case BT_BUFF_LAUNCHER::REPELLENT_EFFECT:
		CQASSERT(obj->effectFlags.bRepellent != bSetting);//if this trips it means that the list was corrupted, someone may be left buffed forever.
		obj->effectFlags.bRepellent = bSetting;
		break;
	case BT_BUFF_LAUNCHER::DESTABILIZER_EFFECT:
		CQASSERT(obj->effectFlags.bDestabilizer != bSetting);//if this trips it means that the list was corrupted, someone may be left buffed forever.
		obj->effectFlags.bDestabilizer = bSetting;
		break;
	case BT_BUFF_LAUNCHER::TALLORIAN_EFFECT:
		CQASSERT(obj->effectFlags.bTalorianShield != bSetting);//if this trips it means that the list was corrupted, someone may be left buffed forever.
		obj->effectFlags.bTalorianShield = bSetting;
		break;
	case BT_BUFF_LAUNCHER::AGIES_EFFECT:
		CQASSERT(obj->effectFlags.bAgeisShield != bSetting);//if this trips it means that the list was corrupted, someone may be left buffed forever.
		obj->effectFlags.bAgeisShield = bSetting;
		break;
	}
}
//---------------------------------------------------------------------------
//
bool BuffLauncher::checkEffectFlag(IBaseObject * obj)
{
	if(!obj)
		return false;
	switch(pData->buffType)
	{
	case BT_BUFF_LAUNCHER::RESEARCH_EFFECT:
		{
			if(obj->effectFlags.bResearchBoost1 || obj->effectFlags.bResearchBoost2 ||
                obj->effectFlags.bResearchBoost3 || obj->effectFlags.bResearchBoost4)
			{
				return true;
			}
			return false;
		}
		break;
	case BT_BUFF_LAUNCHER::CONSTRUCTION_EFFECT:
		{
			if(obj->effectFlags.bIndusrialBoost1 || obj->effectFlags.bIndusrialBoost2 ||
                obj->effectFlags.bIndusrialBoost3 || obj->effectFlags.bIndusrialBoost4)
			{
				return true;
			}
			return false;
		}
		break;
	case BT_BUFF_LAUNCHER::SHIELD_JAM_EFFECT:
		return obj->effectFlags.bShieldJam;
		break;
	case BT_BUFF_LAUNCHER::WEAPON_JAM_EFFECT:
		return obj->effectFlags.bWeaponJam;
		break;
	case BT_BUFF_LAUNCHER::SENSOR_JAM_EFFECT:
		return obj->effectFlags.bSensorJam;
		break;
	case BT_BUFF_LAUNCHER::SINUATOR_EFFECT:
		return obj->effectFlags.bSinuator;
		break;
	case BT_BUFF_LAUNCHER::STASIS_EFFECT:
		return obj->effectFlags.bStasis;
		break;
	case BT_BUFF_LAUNCHER::REPELLENT_EFFECT:
		return obj->effectFlags.bRepellent;
		break;
	case BT_BUFF_LAUNCHER::DESTABILIZER_EFFECT:
		return obj->effectFlags.bDestabilizer;
		break;
	case BT_BUFF_LAUNCHER::TALLORIAN_EFFECT:
		return obj->effectFlags.bTalorianShield;
		break;
	case BT_BUFF_LAUNCHER::AGIES_EFFECT:
		return obj->effectFlags.bAgeisShield;
		break;
	}
	return false;
}
//---------------------------------------------------------------------------
//
bool BuffLauncher::isTarget(IBaseObject * obj)
{
	for(U32 i = 0; i < numTargets; ++i)
	{
		if(targetIDs[i] == obj->GetPartID())
			return true;
	}
	return false;
}
//---------------------------------------------------------------------------
//
IBaseObject * BuffLauncher::findPlanet()
{
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
	return bestPlanet;
}
//---------------------------------------------------------------------------
//
inline IBaseObject * createBuffLauncher (PARCHETYPE pArchetype)
{
	BuffLauncher * buffLauncher = new ObjectImpl<BuffLauncher>;

	buffLauncher->init(pArchetype);

	return buffLauncher;
}
//------------------------------------------------------------------------------------------
//---------------------------BuffLauncher Factory------------------------------------------
//------------------------------------------------------------------------------------------
//
struct DACOM_NO_VTABLE BuffLauncherFactory : public IObjectFactory
{
	struct OBJTYPE : BuffLauncherArchetype
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
		}
	};
	//------------------------------------------------------------------------------------------
	//
	U32 factoryHandle;		// handles to callback

	//
	// Interface mapping
	//

	BEGIN_DACOM_MAP_INBOUND(BuffLauncherFactory)
	DACOM_INTERFACE_ENTRY(IObjectFactory)
	END_DACOM_MAP()

	BuffLauncherFactory (void) { }

	~BuffLauncherFactory (void);

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

	/* BuffLauncherFactory methods */

	IDAComponent * getBase (void)
	{
		return static_cast<IObjectFactory *> (this);
	}
};
//--------------------------------------------------------------------------//
//
BuffLauncherFactory::~BuffLauncherFactory (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (OBJLIST && OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
		connection->Unadvise(factoryHandle);
}
//--------------------------------------------------------------------------//
//
void BuffLauncherFactory::init (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
		connection->Advise(getBase(), &factoryHandle);
}
//-----------------------------------------------------------------------------
//
HANDLE BuffLauncherFactory::CreateArchetype (const char *szArchname, OBJCLASS objClass, void *_data)
{
	OBJTYPE * result = 0;

	if (objClass == OC_LAUNCHER)
	{
		BT_BUFF_LAUNCHER * data = (BT_BUFF_LAUNCHER *) _data;

		if (data->type == LC_BUFF_LAUNCHER)	   
		{
			result = new OBJTYPE;
			result->pArchetype = ARCHLIST->GetArchetype(szArchname);
		}
	}

	return (HANDLE) result;
}
//-------------------------------------------------------------------
//
BOOL32 BuffLauncherFactory::DestroyArchetype (HANDLE hArchetype)
{
	OBJTYPE * objtype = (OBJTYPE *)hArchetype;

//	EditorStopObjectInsertion(objtype->pArchetype);		// not supported
	delete objtype;

	return 1;
}
//-------------------------------------------------------------------
//
IBaseObject * BuffLauncherFactory::CreateInstance (HANDLE hArchetype)
{
	OBJTYPE * objtype = (OBJTYPE *)hArchetype;
	return createBuffLauncher(objtype->pArchetype);
}
//-------------------------------------------------------------------
//
void BuffLauncherFactory::EditorCreateInstance (HANDLE hArchetype, const MISSION_INFO & info)
{
	// not supported
}
//---------------------------------------------------------------------------
//-------------------------GLOBAL STARTUP------------------------------------
//---------------------------------------------------------------------------
//
struct _buffLauncher : GlobalComponent
{
	BuffLauncherFactory * sfactory;

	virtual void Startup (void)
	{
		sfactory = new DAComponent<BuffLauncherFactory>;
		AddToGlobalCleanupList((IDAComponent **) &sfactory);
	}

	virtual void Initialize (void)
	{
		sfactory->init();
	}
};

static _buffLauncher __ship;

//---------------------------------------------------------------------------------------------
//-------------------------------End BuffLauncher.cpp------------------------------------------------
//---------------------------------------------------------------------------------------------