//--------------------------------------------------------------------------//
//                                                                          //
//                            ArtifactLauncher.cpp                          //
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
#include <DArtifactLauncher.h>
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
#include "IParticleCircle.h"
#include "IArtifact.h"

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

struct ArtifactLauncherArchetype
{
};

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
struct _NO_VTABLE ArtifactLauncher : IBaseObject, ILauncher, ISaveLoad, ARTIFACT_LAUNCHER_SAVELOAD
{
	BEGIN_MAP_INBOUND(ArtifactLauncher)
	_INTERFACE_ENTRY(IBaseObject)
	_INTERFACE_ENTRY(ISaveLoad)
	_INTERFACE_ENTRY(ILauncher)
	END_MAP()

	BT_ARTIFACT_LAUNCHER * pData;

	OBJPTR<ILaunchOwner> owner;	// person who created us
	U32 ownerID;

	bool bForceDelete;

	OBJPTR<IWeaponTarget> target;

	//----------------------------------
	//----------------------------------
	
	ArtifactLauncher (void);

	~ArtifactLauncher (void);	

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
		if(obj)
		{
			obj->QueryInterface(IWeaponTargetID,target,GetPlayerID());
			targetID = obj->GetPartID();
		}
	}

	virtual void  GetSpecialAbility (UNIT_SPECIAL_ABILITY & ability, bool & bSpecialEnabled)
	{
		ability = USA_DOCK;
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

	/* ArtifactLauncher methods */
	
	void init (PARCHETYPE pArchetype);
};

//---------------------------------------------------------------------------
//
ArtifactLauncher::ArtifactLauncher (void) 
{
	target = NULL;
	targetID = 0;
	bForceDelete = false;
}
//---------------------------------------------------------------------------
//
ArtifactLauncher::~ArtifactLauncher (void)
{
}
//---------------------------------------------------------------------------
//
void ArtifactLauncher::init (PARCHETYPE _pArchetype)
{
	pData = (BT_ARTIFACT_LAUNCHER *) ARCHLIST->GetArchetypeData(_pArchetype);

	CQASSERT(pData);
	CQASSERT(pData->type == LC_ARTIFACT_LAUNCHER);
	CQASSERT(pData->objClass == OC_LAUNCHER);

	pArchetype = _pArchetype;
	objClass = OC_LAUNCHER;
}
// the following methods are for network synchronization of realtime objects
U32 ArtifactLauncher::GetSyncDataSize (void) const
{
	if(bForceDelete)
	{
		return 1;
	}
	return 0;
}

U32 ArtifactLauncher::GetSyncData (void * buffer)			// buffer points to use supplied memory
{
	if(bForceDelete)
	{
		return 1;
	}
	return 0;
}
//---------------------------------------------------------------------------
//
void ArtifactLauncher::PutSyncData (void * buffer, U32 bufferSize)
{
	OBJLIST->DeferredDestruction(ownerID);
}
//---------------------------------------------------------------------------
//
void ArtifactLauncher::DoSpecialAbility (U32 specialID)
{
}
//---------------------------------------------------------------------------
//
S32 ArtifactLauncher::GetObjectIndex (void) const
{
	// not too sure about this...
	return owner.Ptr()->GetObjectIndex();
}
//---------------------------------------------------------------------------
//
U32 ArtifactLauncher::GetPartID (void) const
{
	return owner.Ptr()->GetPartID();
}
//---------------------------------------------------------------------------
//
BOOL32 ArtifactLauncher::Update (void)
{
	if(THEMATRIX->IsMaster())
	{
		if(target)
		{
			if((target.ptr->GetGridPosition()-owner.ptr->GetGridPosition()) < 1.0)
			{
				VOLPTR(IArtifactHolder) aHolder = target.Ptr();
				if(aHolder && (aHolder->GetArtifact() == NULL))
				{
					aHolder->SetArtifact(pData->artifactName);
					bForceDelete = true;
					THEMATRIX->ForceSyncData(owner.ptr);
					OBJLIST->DeferredDestruction(ownerID);
				}
			}
		}
	}
	return 1;
}
//---------------------------------------------------------------------------
//
void ArtifactLauncher::InitLauncher (IBaseObject * _owner, S32 ownerIndex, S32 animArcheIndex, SINGLE range)
{
	CQASSERT(_owner);
	_owner->QueryInterface(ILaunchOwnerID, owner, LAUNCHVOLATILEPTR);
	CQASSERT(owner!=0);
	ownerID = _owner->GetPartID();
}
//---------------------------------------------------------------------------
//
void ArtifactLauncher::InformOfCancel()
{
	target = NULL;
	targetID = 0;
}
//---------------------------------------------------------------------------
//
void ArtifactLauncher::LauncherOpCreated(U32 agentID, void * buffer, U32 bufferSize)
{
}
//---------------------------------------------------------------------------
//
void ArtifactLauncher::LauncherReceiveOpData(U32 workingID, void * buffer, U32 bufferSize)
{
}
//---------------------------------------------------------------------------
//
void ArtifactLauncher::LauncherOpCompleted(U32 agentID)
{
}
//---------------------------------------------------------------------------
//
bool ArtifactLauncher::IsToggle()
{
	return false;
}
//---------------------------------------------------------------------------
//
bool ArtifactLauncher::CanToggle()
{
	return false;
}
//---------------------------------------------------------------------------
//
bool ArtifactLauncher::IsOn()
{
	return false;
}
//---------------------------------------------------------------------------
//
void ArtifactLauncher::AttackObject (IBaseObject * obj)
{
}
//---------------------------------------------------------------------------
//
void ArtifactLauncher::DoCreateWormhole(U32 systemID)
{
}

//---------------------------------------------------------------------------
//
const TRANSFORM & ArtifactLauncher::GetTransform (void) const
{
	return owner.Ptr()->GetTransform();
}
//---------------------------------------------------------------------------
//
inline IBaseObject * createArtifactLauncher (PARCHETYPE pArchetype)
{
	ArtifactLauncher * artifactLauncher = new ObjectImpl<ArtifactLauncher>;

	artifactLauncher->init(pArchetype);

	return artifactLauncher;
}
//------------------------------------------------------------------------------------------
//---------------------------ArtifactLauncher Factory------------------------------------------
//------------------------------------------------------------------------------------------
//
struct DACOM_NO_VTABLE ArtifactLauncherFactory : public IObjectFactory
{
	struct OBJTYPE : ArtifactLauncherArchetype
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

	BEGIN_DACOM_MAP_INBOUND(ArtifactLauncherFactory)
	DACOM_INTERFACE_ENTRY(IObjectFactory)
	END_DACOM_MAP()

	ArtifactLauncherFactory (void) { }

	~ArtifactLauncherFactory (void);

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

	/* ArtifactLauncherFactory methods */

	IDAComponent * getBase (void)
	{
		return static_cast<IObjectFactory *> (this);
	}
};
//--------------------------------------------------------------------------//
//
ArtifactLauncherFactory::~ArtifactLauncherFactory (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (OBJLIST && OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
		connection->Unadvise(factoryHandle);
}
//--------------------------------------------------------------------------//
//
void ArtifactLauncherFactory::init (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
		connection->Advise(getBase(), &factoryHandle);
}
//-----------------------------------------------------------------------------
//
HANDLE ArtifactLauncherFactory::CreateArchetype (const char *szArchname, OBJCLASS objClass, void *_data)
{
	OBJTYPE * result = 0;

	if (objClass == OC_LAUNCHER)
	{
		BT_ARTIFACT_LAUNCHER * data = (BT_ARTIFACT_LAUNCHER *) _data;

		if (data->type == LC_ARTIFACT_LAUNCHER)	   
		{
			result = new OBJTYPE;
			result->pArchetype = ARCHLIST->GetArchetype(szArchname);
		}
	}

	return (HANDLE) result;
}
//-------------------------------------------------------------------
//
BOOL32 ArtifactLauncherFactory::DestroyArchetype (HANDLE hArchetype)
{
	OBJTYPE * objtype = (OBJTYPE *)hArchetype;

//	EditorStopObjectInsertion(objtype->pArchetype);		// not supported
	delete objtype;

	return 1;
}
//-------------------------------------------------------------------
//
IBaseObject * ArtifactLauncherFactory::CreateInstance (HANDLE hArchetype)
{
	OBJTYPE * objtype = (OBJTYPE *)hArchetype;
	return createArtifactLauncher(objtype->pArchetype);
}
//-------------------------------------------------------------------
//
void ArtifactLauncherFactory::EditorCreateInstance (HANDLE hArchetype, const MISSION_INFO & info)
{
	// not supported
}
//---------------------------------------------------------------------------
//-------------------------GLOBAL STARTUP------------------------------------
//---------------------------------------------------------------------------
//
struct _artifactLauncher : GlobalComponent
{
	ArtifactLauncherFactory * sfactory;

	virtual void Startup (void)
	{
		sfactory = new DAComponent<ArtifactLauncherFactory>;
		AddToGlobalCleanupList((IDAComponent **) &sfactory);
	}

	virtual void Initialize (void)
	{
		sfactory->init();
	}
};

static _artifactLauncher __ship;

//---------------------------------------------------------------------------------------------
//-------------------------------End ArtifactLauncher.cpp------------------------------------------------
//---------------------------------------------------------------------------------------------