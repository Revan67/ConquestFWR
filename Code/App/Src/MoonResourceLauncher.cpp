//--------------------------------------------------------------------------//
//                                                                          //
//                                MoonResourceLauncher.cpp                  //
//                                                                          //
//                  COPYRIGHT (C) 2003 By Warthog TX, INC.                  //
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
#include <DMoonResourceLauncher.h>
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

struct MoonResourceLauncherArchetype
{
};

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
struct _NO_VTABLE MoonResourceLauncher : IBaseObject, ILauncher, ISaveLoad, MOON_RESOURCE_LAUNCHER_SAVELOAD
{
	BEGIN_MAP_INBOUND(MoonResourceLauncher)
	_INTERFACE_ENTRY(IBaseObject)
	_INTERFACE_ENTRY(ISaveLoad)
	_INTERFACE_ENTRY(ILauncher)
	END_MAP()

	BT_MOON_RESOURCE_LAUNCHER * pData;

	OBJPTR<ILaunchOwner> owner;	// person who created us
	U32 ownerID;

	OBJPTR<IPlanet> planet;

	//----------------------------------
	//----------------------------------
	
	MoonResourceLauncher (void);

	~MoonResourceLauncher (void);	

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

	virtual BOOL32 Save (struct IFileSystem * file);

	virtual BOOL32 Load (struct IFileSystem * file);
	
	virtual void ResolveAssociations()
	{
	}

	/* MoonResourceLauncher methods */
	
	void init (PARCHETYPE pArchetype);

	bool checkSupplies();

	IBaseObject * findPlanet();
};

//---------------------------------------------------------------------------
//
MoonResourceLauncher::MoonResourceLauncher (void) 
{
}
//---------------------------------------------------------------------------
//
MoonResourceLauncher::~MoonResourceLauncher (void)
{
	if(!planet)
	{
		IBaseObject * pl = findPlanet();
		if(pl)
		{
			pl->QueryInterface(IPlanetID,planet,NONSYSVOLATILEPTR);
		}
	}
	if(THEMATRIX->IsMaster() && planet)
	{
		if(bHaveBoostedPlanet)
		{
			planet->BoostRegen(-pData->oreRegenRate,-pData->gasRegenRate,-pData->crewRegenRate);
			bHaveBoostedPlanet = false;
		}
	}
}
//---------------------------------------------------------------------------
//
void MoonResourceLauncher::init (PARCHETYPE _pArchetype)
{
	pData = (BT_MOON_RESOURCE_LAUNCHER *) ARCHLIST->GetArchetypeData(_pArchetype);

	CQASSERT(pData);
	CQASSERT(pData->type == LC_MOON_RESOURCE_LAUNCH);
	CQASSERT(pData->objClass == OC_LAUNCHER);

	pArchetype = _pArchetype;
	objClass = OC_LAUNCHER;

	bHaveBoostedPlanet = false;

}
//---------------------------------------------------------------------------
//
void MoonResourceLauncher::DoSpecialAbility (U32 specialID)
{

}
//---------------------------------------------------------------------------
//
S32 MoonResourceLauncher::GetObjectIndex (void) const
{
	// not too sure about this...
	return owner.Ptr()->GetObjectIndex();
}
//---------------------------------------------------------------------------
//
U32 MoonResourceLauncher::GetPartID (void) const
{
	return owner.Ptr()->GetPartID();
}
//---------------------------------------------------------------------------
//
BOOL32 MoonResourceLauncher::Update (void)
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
	if(THEMATRIX->IsMaster() && planet)
	{
		MPart part = owner.Ptr();
		if(part.isValid() && part->bReady)
		{
			if(bHaveBoostedPlanet)
			{
				if(!(SECTOR->SystemInSupply(owner.Ptr()->GetSystemID(),owner.Ptr()->GetPlayerID())))
				{
					planet->BoostRegen(-pData->oreRegenRate,-pData->gasRegenRate,-pData->crewRegenRate);
					bHaveBoostedPlanet = false;
				}
			}
			else
			{
				if(SECTOR->SystemInSupply(owner.Ptr()->GetSystemID(),owner.Ptr()->GetPlayerID()))
				{
					planet->BoostRegen(pData->oreRegenRate,pData->gasRegenRate,pData->crewRegenRate);
					bHaveBoostedPlanet = true;
				}
			}
		}
	}
	return 1;
}
//---------------------------------------------------------------------------
//
void MoonResourceLauncher::InitLauncher (IBaseObject * _owner, S32 ownerIndex, S32 animArcheIndex, SINGLE range)
{
	CQASSERT(_owner);
	_owner->QueryInterface(ILaunchOwnerID, owner, LAUNCHVOLATILEPTR);
	CQASSERT(owner!=0);
	ownerID = _owner->GetPartID();
}
//---------------------------------------------------------------------------
//
void MoonResourceLauncher::InformOfCancel()
{
}
//---------------------------------------------------------------------------
//
void MoonResourceLauncher::LauncherOpCreated(U32 agentID, void * buffer, U32 bufferSize)
{
}
//---------------------------------------------------------------------------
//
void MoonResourceLauncher::LauncherReceiveOpData(U32 workingID, void * buffer, U32 bufferSize)
{
}
//---------------------------------------------------------------------------
//
void MoonResourceLauncher::LauncherOpCompleted(U32 agentID)
{
}
//---------------------------------------------------------------------------
//
void MoonResourceLauncher::AttackObject (IBaseObject * obj)
{
}
//---------------------------------------------------------------------------
//
void MoonResourceLauncher::DoCreateWormhole(U32 systemID)
{
}

//---------------------------------------------------------------------------
//
const TRANSFORM & MoonResourceLauncher::GetTransform (void) const
{
	return owner.Ptr()->GetTransform();
}
//---------------------------------------------------------------------------
//
BOOL32 MoonResourceLauncher::Save (struct IFileSystem * outFile)
{
	DAFILEDESC fdesc = "MOON_RESOURCE_LAUNCHER_SAVELOAD";
	COMPTR<IFileSystem> file;
	BOOL32 result = 0;
	DWORD dwWritten;
	MOON_RESOURCE_LAUNCHER_SAVELOAD save;

	fdesc.lpImplementation = "DOS";
	fdesc.dwDesiredAccess = GENERIC_READ|GENERIC_WRITE;
	fdesc.dwShareMode = 0;  // no sharing
	fdesc.dwCreationDistribution = CREATE_ALWAYS;

	if (outFile->CreateInstance(&fdesc, file) != GR_OK)
		goto Done;

	memset(&save, 0, sizeof(save));
	memcpy(&save, static_cast<MOON_RESOURCE_LAUNCHER_SAVELOAD *>(this), sizeof(MOON_RESOURCE_LAUNCHER_SAVELOAD));

//	FRAME_save(save);
	
	file->WriteFile(0, &save, sizeof(save), &dwWritten, 0);
	
	result = 1;

Done:	
	return result;
}

//---------------------------------------------------------------------------
//
BOOL32 MoonResourceLauncher::Load (struct IFileSystem * inFile)
{
	DAFILEDESC fdesc = "MOON_RESOURCE_LAUNCHER_SAVELOAD";
	COMPTR<IFileSystem> file;
	BOOL32 result = 0;
	DWORD dwRead;
	MOON_RESOURCE_LAUNCHER_SAVELOAD load;
	U8 buffer[1024];

	fdesc.lpImplementation = "DOS";
	if (inFile->CreateInstance(&fdesc, file) != GR_OK)
		goto Done;

	file->ReadFile(0, buffer, sizeof(buffer), &dwRead, 0);
	MISSION->CorrelateSymbol("MOON_RESOURCE_LAUNCHER_SAVELOAD", buffer, &load);

	*static_cast<MOON_RESOURCE_LAUNCHER_SAVELOAD *>(this) = load;

//	FRAME_load(load);

	result = 1;

Done:	
	return result;
}
//---------------------------------------------------------------------------
//
bool MoonResourceLauncher::checkSupplies()
{
	return true;
}
//---------------------------------------------------------------------------
//
IBaseObject * MoonResourceLauncher::findPlanet()
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
inline IBaseObject * createMoonResourceLauncher (PARCHETYPE pArchetype)
{
	MoonResourceLauncher * moonResourceLauncher = new ObjectImpl<MoonResourceLauncher>;

	moonResourceLauncher->init(pArchetype);

	return moonResourceLauncher;
}
//------------------------------------------------------------------------------------------
//---------------------------MoonResourceLauncher Factory------------------------------------------
//------------------------------------------------------------------------------------------
//
struct DACOM_NO_VTABLE MoonResourceLauncherFactory : public IObjectFactory
{
	struct OBJTYPE : MoonResourceLauncherArchetype
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

	BEGIN_DACOM_MAP_INBOUND(MoonResourceLauncherFactory)
	DACOM_INTERFACE_ENTRY(IObjectFactory)
	END_DACOM_MAP()

	MoonResourceLauncherFactory (void) { }

	~MoonResourceLauncherFactory (void);

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

	/* MoonResourceLauncherFactory methods */

	IDAComponent * getBase (void)
	{
		return static_cast<IObjectFactory *> (this);
	}
};
//--------------------------------------------------------------------------//
//
MoonResourceLauncherFactory::~MoonResourceLauncherFactory (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (OBJLIST && OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
		connection->Unadvise(factoryHandle);
}
//--------------------------------------------------------------------------//
//
void MoonResourceLauncherFactory::init (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
		connection->Advise(getBase(), &factoryHandle);
}
//-----------------------------------------------------------------------------
//
HANDLE MoonResourceLauncherFactory::CreateArchetype (const char *szArchname, OBJCLASS objClass, void *_data)
{
	OBJTYPE * result = 0;

	if (objClass == OC_LAUNCHER)
	{
		BT_MOON_RESOURCE_LAUNCHER * data = (BT_MOON_RESOURCE_LAUNCHER *) _data;

		if (data->type == LC_MOON_RESOURCE_LAUNCH)	   
		{
			result = new OBJTYPE;
			result->pArchetype = ARCHLIST->GetArchetype(szArchname);
		}
	}

	return (HANDLE) result;
}
//-------------------------------------------------------------------
//
BOOL32 MoonResourceLauncherFactory::DestroyArchetype (HANDLE hArchetype)
{
	OBJTYPE * objtype = (OBJTYPE *)hArchetype;

//	EditorStopObjectInsertion(objtype->pArchetype);		// not supported
	delete objtype;

	return 1;
}
//-------------------------------------------------------------------
//
IBaseObject * MoonResourceLauncherFactory::CreateInstance (HANDLE hArchetype)
{
	OBJTYPE * objtype = (OBJTYPE *)hArchetype;
	return createMoonResourceLauncher(objtype->pArchetype);
}
//-------------------------------------------------------------------
//
void MoonResourceLauncherFactory::EditorCreateInstance (HANDLE hArchetype, const MISSION_INFO & info)
{
	// not supported
}
//---------------------------------------------------------------------------
//-------------------------GLOBAL STARTUP------------------------------------
//---------------------------------------------------------------------------
//
struct _moonResourceLauncher : GlobalComponent
{
	MoonResourceLauncherFactory * sfactory;

	virtual void Startup (void)
	{
		sfactory = new DAComponent<MoonResourceLauncherFactory>;
		AddToGlobalCleanupList((IDAComponent **) &sfactory);
	}

	virtual void Initialize (void)
	{
		sfactory->init();
	}
};

static _moonResourceLauncher __ship;

//---------------------------------------------------------------------------------------------
//-------------------------------End MoonResourceLauncher.cpp------------------------------------------------
//---------------------------------------------------------------------------------------------