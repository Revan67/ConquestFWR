//--------------------------------------------------------------------------//
//                                                                          //
//                               JumpPlat.cpp                                //
//                                                                          //
//                  COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/JumpPlat.cpp 30    6/22/01 9:56a Tmauer $
*/			    
//--------------------------------------------------------------------------//

#include "pch.h"
#include <globals.h>

#include "TPlatform.h"
#include "Startup.h"
#include "MGlobals.h"
#include "ObjList.h"
#include "GenData.h"
#include "IJumpPlat.h"
#include "IJumpGate.h"

#include <DJumpPlat.h>

#include <Physics.h>
#include <3DMath.h>
#include <FileSys.h>
#include <IConnection.h>
#include <TComponent.h>
#include <HeapObj.h>

#define HOST_CHECK THEMATRIX->IsMaster()

#define COMP_OP(operation) { U32 tempID = workingID; workingID = 0; THEMATRIX->OperationCompleted(tempID,operation);}

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//

struct _NO_VTABLE JumpPlat :Platform<JUMPPLAT_SAVELOAD, JUMPPLAT_INIT>, IJumpPlat, 
											BASE_JUMPPLAT_SAVELOAD
{
	BEGIN_MAP_INBOUND(JumpPlat)
	_INTERFACE_ENTRY(IBaseObject)
	_INTERFACE_ENTRY(ISaveLoad)
	_INTERFACE_ENTRY(IWeaponTarget)
	_INTERFACE_ENTRY(IExplosionOwner)
	_INTERFACE_ENTRY(IMissionActor)
	_INTERFACE_ENTRY(IBuild)
	_INTERFACE_ENTRY(IPlatform)
	_INTERFACE_ENTRY(IPhysicalObject)
	_INTERFACE_ENTRY(IShipDamage)
	_INTERFACE_ENTRY(IQuickSaveLoad)
	_INTERFACE_ENTRY(IJumpPlat)
	_INTERFACE_ENTRY(IExtent)
	_INTERFACE_ENTRY(IEffectTarget)
	END_MAP()

	SaveNode		saveNode;
	LoadNode		loadNode;
	ResolveNode		resolveNode;
	ExplodeNode		explodeNode;

	OBJPTR<IJumpPlat> sibling;

	JumpPlat (void);

	virtual ~JumpPlat (void);	// See ObjList.cpp

	virtual SINGLE TestHighlight (const RECT &rect);

	virtual void SetReady (bool _bReady);

	/* IExplosionOwner methods */

	virtual void OnChildDeath (INSTANCE_INDEX child)
	{
	}
	// IJumpPlat

	virtual void SetSibling (IBaseObject * baseSib);

	virtual void SetHull (U32 amount, bool terminate);

	virtual IBaseObject * GetJumpGate (void);

	virtual void SetSiblingHull (U32 amount);

	virtual IBaseObject * GetSibling (void);

	// IPlatform method

	virtual bool IsDeepSpacePlatform (void)
	{
		return false;
	}

	virtual bool IsJumpPlatform (void)
	{
		return true;
	}

	virtual U32 GetPlanetID (void)
	{
		return 0;
	};

	virtual U32 GetSlotID (void)
	{
		return 0;
	};

	virtual TRANSFORM GetShipTransform (void);

	virtual TRANSFORM GetDockTransform (void)
	{
		return transform;
	}

	virtual void SetRallyPoint (const struct NETGRIDVECTOR & point)
	{
		CQBOMB0("What!?");
	}

	virtual bool GetRallyPoint (struct NETGRIDVECTOR & point)
	{
		return false;
	}

	virtual bool IsDockLocked (void)
	{
		return false;
	}

	virtual void LockDock (IBaseObject * locker)
	{
	}

	virtual void UnlockDock (IBaseObject * locker)
	{
	}

	virtual void FreePlanetSlot (void)
	{
	}

	virtual void ParkYourself (const TRANSFORM & _trans, U32 planetID, U32 slotID)
	{
		CQASSERT(0 && "Not supported for jump platforms");
	}
	
	virtual void ParkYourself (IBaseObject * jumpgate);

	virtual U32 GetNumDocking (void)
	{
		CQBOMB0("What!?");
		return 0;
	}

	virtual void IncNumDocking (void)
	{
		CQBOMB0("What!?");
	}

	virtual void DecNumDocking (void)
	{
		CQBOMB0("What!?");
	}

	/* IWeaponTarget */

	virtual void ApplyAOEDamage (U32 ownerID, U32 damageAmount);
	
	virtual BOOL32 ApplyDamage (IBaseObject * collider, U32 ownerID, const Vector & pos, const Vector &dir,U32 amount,BOOL32 shieldHit=1);

	/* IQuickSaveLoad methods */

	virtual void QuickSave (struct IFileSystem * file);

	virtual void QuickResolveAssociations (void);

	/* IMissionActor */

	virtual void TakeoverSwitchID (U32 newMissionID);

	/* Platform methods */


	virtual const char * getSaveStructName (void) const		// must be overriden implemented by derived class
	{
		return "JUMPPLAT_SAVELOAD";
	}

	virtual void * getViewStruct (void)				// must be overriden implemented by derived class
	{
		return static_cast<BASE_JUMPPLAT_SAVELOAD *>(this);
	}

	/* BuildPlat methods */

	void EndBuild (bool bAborting);

	bool initJumpPlat (const JUMPPLAT_INIT & data);

	void save (JUMPPLAT_SAVELOAD & save);

	void load (JUMPPLAT_SAVELOAD & load);

	void explodePlat (bool bExplode);

	void resolve (void);

	virtual void killPlatform();
};
//---------------------------------------------------------------------------
//
JumpPlat::JumpPlat (void) :
			saveNode(this, CASTSAVELOADPROC(&JumpPlat::save)),
			loadNode(this, CASTSAVELOADPROC(&JumpPlat::load)),
			explodeNode(this, ExplodeProc(&JumpPlat::explodePlat)),
			resolveNode(this, ResolveProc(&JumpPlat::resolve))
{
}
//---------------------------------------------------------------------------
//
JumpPlat::~JumpPlat (void)
{
	IBaseObject * jumpgate = OBJLIST->FindObject(jumpGateID);
	if(jumpgate && bOwnGate)
	{
		OBJPTR<IJumpGate> gate;
		jumpgate->QueryInterface(IJumpGateID,gate);
		gate->UnsetOwner(dwMissionID);	
		bOwnGate = false;
	}
}
//---------------------------------------------------------------------------
//
SINGLE JumpPlat::TestHighlight (const RECT &rect)
{
	SINGLE closness = Platform<JUMPPLAT_SAVELOAD, JUMPPLAT_INIT>::TestHighlight(rect);
	if(bHighlight)
	{
		IBaseObject * jumpgate = OBJLIST->FindObject(jumpGateID);
		jumpgate->TestHighlight(rect);
		if(jumpgate->bHighlight)
		{
			bHighlight = false;
			return 999999.0f;;
		}
	}
	return closness;
}
//---------------------------------------------------------------------------
//
void JumpPlat::SetReady (bool _bReady)
{
	bool oldReady = bReady;

	Platform<JUMPPLAT_SAVELOAD, JUMPPLAT_INIT>::SetReady(_bReady);
	if(oldReady && (!_bReady) && jumpGateID && bLockGate)
	{
		bLockGate = false;
		IBaseObject * jumpgate = OBJLIST->FindObject(jumpGateID);
		OBJPTR<IJumpGate> gate;
		if(jumpgate)
		{
			jumpgate->QueryInterface(IJumpGateID,gate);
			gate->Unlock();
		}
		if(mObjClass == M_JUMPPLAT || mObjClass == M_HQ || mObjClass == M_COCOON || mObjClass == M_ACROPOLIS || mObjClass == M_LOCUS)
		{
			SECTOR->ComputeSupplyForAllPlayers();
		}
	}
	else if((!oldReady) && bReady && jumpGateID && !bLockGate)
	{
		bLockGate = true;
		IBaseObject * jumpgate = OBJLIST->FindObject(jumpGateID);
		OBJPTR<IJumpGate> gate;
		if(jumpgate)
		{
			jumpgate->QueryInterface(IJumpGateID,gate);
			gate->Lock();
		}

		if(mObjClass == M_JUMPPLAT || mObjClass == M_HQ || mObjClass == M_COCOON || mObjClass == M_ACROPOLIS || mObjClass == M_LOCUS)
		{
			SECTOR->ComputeSupplyForAllPlayers();
		}
	}
	if(sibling)
	{
		MPart sibPart(sibling.Ptr());
		if(sibPart->bReady != _bReady)
			sibling.Ptr()->SetReady(_bReady);
	}
}
//---------------------------------------------------------------------------
//
void JumpPlat::SetSibling (IBaseObject * baseSib)
{
	baseSib->QueryInterface(IJumpPlatID,sibling, NONSYSVOLATILEPTR);
}
//---------------------------------------------------------------------------
//
void JumpPlat::SetHull (U32 amount, bool terminate)
{
	if(THEMATRIX->IsMaster())
	{
		hullPoints = amount;
		if(hullPoints <= 0 && terminate)
			THEMATRIX->ObjectTerminated(dwMissionID, 0);
	}
}
//---------------------------------------------------------------------------
//
IBaseObject * JumpPlat::GetJumpGate (void)
{
	return OBJLIST->FindObject(jumpGateID);
}
//---------------------------------------------------------------------------
//
void JumpPlat::SetSiblingHull (U32 amount)
{
	if(sibling)
		sibling->SetHull(amount,false);
}
//---------------------------------------------------------------------------
//
IBaseObject * JumpPlat::GetSibling (void)
{
    if (!sibling)
    {
        if(dwMissionID & 0x10000000)
        {
	        IBaseObject * obj = OBJLIST->FindObject(dwMissionID & 0xEFFFFFFF);
			if (obj)
			{
				obj->QueryInterface(IJumpPlatID,sibling,NONSYSVOLATILEPTR);
			}
        }
        else
        {
	        IBaseObject * obj = OBJLIST->FindObject(dwMissionID | 0x10000000);
			if (obj)
			{
				obj->QueryInterface(IJumpPlatID,sibling,NONSYSVOLATILEPTR);
			}
        }
    }

    return sibling.Ptr();
}
//---------------------------------------------------------------------------
//
void JumpPlat::ParkYourself (IBaseObject * jumpgate)
{
	//create other platform
	if((!(dwMissionID & 0x10000000)) && (!sibling))
	{
		IBaseObject * baseSib = MGlobals::CreateInstance(pArchetype,dwMissionID|0x10000000);
		OBJLIST->AddObject(baseSib);
		baseSib->QueryInterface(IJumpPlatID,sibling,NONSYSVOLATILEPTR);
		
		MPartNC part = baseSib;


		sibling->SetSibling(this);


		IBaseObject * jump2 = SECTOR->GetJumpgateDestination(jumpgate);
		sibling->ParkYourself(jump2);
		ENGINE->update_instance(sibling.Ptr()->GetObjectIndex(),0,0);

		// the object has to initialize its footprint
		COMPTR<ITerrainMap> map;
		SECTOR->RevealSystem(jump2->GetSystemID(),playerID);
		SECTOR->GetTerrainMap(jump2->GetSystemID(), map);
		if (map)
		{
			baseSib->SetTerrainFootprint(map);
		}

		if (part.isValid())
		{
			part->hullPoints = hullPoints;
			baseSib->SetReady(bReady);

			if (part->playerID != 0)
			{
				baseSib->SetVisibleToPlayer(part->playerID);
				baseSib->UpdateVisibilityFlags();
			}
		}
	}

	//park myself
	SetSystemID(jumpgate->GetSystemID());
	transform.translation = jumpgate->GetPosition();
	jumpGateID = jumpgate->GetPartID();

	OBJPTR<IJumpGate> gate;
	if(jumpgate && !bOwnGate)
	{
		jumpgate->QueryInterface(IJumpGateID,gate);
		gate->SetOwner(dwMissionID);
		bOwnGate = true;
	}
	if(bReady)
	{
		if(!bLockGate)
		{
			bLockGate = true;
			gate->Lock();
			if(mObjClass == M_JUMPPLAT || mObjClass == M_HQ || mObjClass == M_COCOON || mObjClass == M_ACROPOLIS || mObjClass == M_LOCUS)
			{
				SECTOR->ComputeSupplyForAllPlayers();
			}
		}
	}
}
//---------------------------------------------------------------------------
//
TRANSFORM JumpPlat::GetShipTransform (void)
{
	return TRANSFORM();
}
//---------------------------------------------------------------------------
//
void JumpPlat::ApplyAOEDamage (U32 ownerID, U32 damageAmount)
{
	Platform<JUMPPLAT_SAVELOAD, JUMPPLAT_INIT>::ApplyAOEDamage(ownerID,damageAmount);
	if(sibling)
		sibling->SetHull(hullPoints,true);
}
//---------------------------------------------------------------------------
//
BOOL32 JumpPlat::ApplyDamage (IBaseObject * collider, U32 ownerID, const Vector & pos, const Vector &dir,U32 amount,BOOL32 shieldHit)
{
	BOOL32 result = Platform<JUMPPLAT_SAVELOAD, JUMPPLAT_INIT>::ApplyDamage (collider, ownerID, pos, dir,amount,shieldHit);
	if(sibling)
		sibling->SetHull(hullPoints,true);

	return result;
}
//---------------------------------------------------------------------------
//
void JumpPlat::EndBuild (bool bAborting)
{
	if (building)
	{
		building = FALSE;
		whole = !bAborting;
		
		if (bAborting == FALSE)
		{
			if((mObjClass != M_HARVEST) 
				&&	(mObjClass != M_GALIOT) &&(mObjClass != M_SIPHON))
				supplies = supplyPointsMax;
			SetReady(true);
			emitterPause = 2.0;
			PLATFORM_ALERT(this, dwMissionID, constructComplete,SUB_CONSTRUCTION_COMP, pInitData->displayName, ALERT_PLATFORM_BUILD);

			MScript::RunProgramsWithEvent(CQPROGFLAG_OBJECTCONSTRUCTED, dwMissionID);

			if (buildEffectObj)
			{
				buildEffectObj->SetBuildPercent(1.0);
				buildEffectObj->Done();
				bSpecialRender = false;
				//hang around for unbuild right now
			//	delete buildEffectObj;
			}
			
		}
		else
		{
			if(THEMATRIX->IsMaster())
				THEMATRIX->ObjectTerminated(GetPartID());
		}
		fabID = 0;

	}
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//

bool JumpPlat::initJumpPlat (const JUMPPLAT_INIT & data)
{
	bOwnGate = false;
	return true;
}
//---------------------------------------------------------------------------
//
void JumpPlat::save (JUMPPLAT_SAVELOAD & save)
{
	save.jumpPlatSaveload = *static_cast<BASE_JUMPPLAT_SAVELOAD *>(this);
}
//---------------------------------------------------------------------------
//
void JumpPlat::load (JUMPPLAT_SAVELOAD & load)
{
 	*static_cast<BASE_JUMPPLAT_SAVELOAD *>(this) = load.jumpPlatSaveload;
}
//---------------------------------------------------------------------------
//
void JumpPlat::explodePlat (bool bExplode)
{
	IBaseObject * jumpgate = OBJLIST->FindObject(jumpGateID);
	if(jumpgate && bOwnGate)
	{
		OBJPTR<IJumpGate> gate;
		jumpgate->QueryInterface(IJumpGateID,gate);
		if(dwMissionID & 0x10000000 && bLockGate)
			MGlobals::SetNumJumpgatesControlled(playerID, MGlobals::GetNumJumpgatesControlled(playerID)-1);
		if(bOwnGate)
		{
			gate->UnsetOwner(dwMissionID);	
			bOwnGate = false;
		}
		if(bLockGate)
		{
			gate->Unlock();
			bLockGate = false;
		}
	}
}
//---------------------------------------------------------------------------
//
void JumpPlat::resolve (void)
{
	if(dwMissionID & 0x10000000)
	{
		IBaseObject * obj = OBJLIST->FindObject(dwMissionID & 0xEFFFFFFF);
		if(obj)
			obj->QueryInterface(IJumpPlatID,sibling,NONSYSVOLATILEPTR);
	}
	else
	{
		IBaseObject * obj = OBJLIST->FindObject(dwMissionID | 0x10000000);
		if(obj)
			obj->QueryInterface(IJumpPlatID,sibling,NONSYSVOLATILEPTR);
	}
}
//---------------------------------------------------------------------------
//
void JumpPlat::killPlatform (void)
{
	if(!(dwMissionID & 0x10000000))
	{
		THEMATRIX->SendPlatformDeath(dwMissionID);
		bPlatRealyDead = true;
		COMPTR<ITerrainMap> map;
		SECTOR->GetTerrainMap(systemID, map);
		undoFootprintInfo(map);
		OBJLIST->DeferredDestruction(dwMissionID);
	}
}
//---------------------------------------------------------------------------
//
void JumpPlat::QuickSave (struct IFileSystem * file)
{
	if(dwMissionID & 0x10000000)
		return;
	Platform<JUMPPLAT_SAVELOAD, JUMPPLAT_INIT>::QuickSave (file);
}
//---------------------------------------------------------------------------
//
void JumpPlat::QuickResolveAssociations (void)
{
	Platform<JUMPPLAT_SAVELOAD, JUMPPLAT_INIT>::QuickResolveAssociations();
}
//---------------------------------------------------------------------------
//
void JumpPlat::TakeoverSwitchID (U32 newMissionID)
{
	Platform<JUMPPLAT_SAVELOAD, JUMPPLAT_INIT>::TakeoverSwitchID(newMissionID);
	if(newMissionID  & 0x10000000)
	{
		IBaseObject * obj = OBJLIST->FindObject(dwMissionID & 0xEFFFFFFF);
		if(obj)
		{
			obj->QueryInterface(IJumpPlatID,sibling,NONSYSVOLATILEPTR);
			sibling->SetSibling(this);
		}
	}
	else
	{
		IBaseObject * obj = OBJLIST->FindObject(dwMissionID | 0x10000000);
		if(obj)
		{	
			obj->QueryInterface(IJumpPlatID,sibling,NONSYSVOLATILEPTR);
			sibling->SetSibling(this);
		}
	}
	SECTOR->ComputeSupplyForAllPlayers();
}
//---------------------------------------------------------------------------
//
inline struct IBaseObject * createPlatform (const JUMPPLAT_INIT & data)
{
	JumpPlat * obj = new ObjectImpl<JumpPlat>;

	obj->FRAME_init(data);
	if (obj->initJumpPlat(data) == false)
	{
		delete obj;
		obj = 0;
	}

	return obj;
}
//------------------------------------------------------------------------------------------
//
JUMPPLAT_INIT::~JUMPPLAT_INIT (void)					// free archetype references
{
	// nothing yet!?
}
//------------------------------------------------------------------------------------------
//---------------------------JumpPlat Factory----------------------------------------------
//------------------------------------------------------------------------------------------
//
struct DACOM_NO_VTABLE JumpPlatFactory : public IObjectFactory
{
	U32 factoryHandle;		// handles to callback

	//
	// Interface mapping
	//

	BEGIN_DACOM_MAP_INBOUND(JumpPlatFactory)
	DACOM_INTERFACE_ENTRY(IObjectFactory)
	END_DACOM_MAP()

	JumpPlatFactory (void) { }

	~JumpPlatFactory (void);

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

	/* JumpPlatFactory methods */

	IDAComponent * getBase (void)
	{
		return static_cast<IObjectFactory *> (this);
	}
};
//--------------------------------------------------------------------------//
//
JumpPlatFactory::~JumpPlatFactory (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (OBJLIST && OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
		connection->Unadvise(factoryHandle);
}
//--------------------------------------------------------------------------//
//
void JumpPlatFactory::init (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
		connection->Advise(getBase(), &factoryHandle);
}
//-----------------------------------------------------------------------------
//
HANDLE JumpPlatFactory::CreateArchetype (const char *szArchname, OBJCLASS objClass, void *_data)
{
	JUMPPLAT_INIT * result = 0;

	if (objClass == OC_PLATFORM)
	{
		BT_PLAT_JUMP_DATA * data = (BT_PLAT_JUMP_DATA *) _data;
		
		if (data->type == PC_JUMPPLAT)
		{
			result = new JUMPPLAT_INIT;

			if (result->loadPlatformArchetype(data, ARCHLIST->GetArchetype(szArchname)))
			{
				// do something here...
			}
			else
				goto Error;
		}

		goto Done;
	}



Error:
	delete result;
	result = 0;
Done:
	return (HANDLE) result;
}
//-------------------------------------------------------------------
//
BOOL32 JumpPlatFactory::DestroyArchetype (HANDLE hArchetype)
{
	JUMPPLAT_INIT * objtype = (JUMPPLAT_INIT *)hArchetype;

	EditorStopObjectInsertion(objtype->pArchetype);
	delete objtype;

	return 1;
}
//-------------------------------------------------------------------
//
IBaseObject * JumpPlatFactory::CreateInstance (HANDLE hArchetype)
{
	JUMPPLAT_INIT * objtype = (JUMPPLAT_INIT *)hArchetype;
	return createPlatform(*objtype);
}
//-------------------------------------------------------------------
//
void JumpPlatFactory::EditorCreateInstance (HANDLE hArchetype, const MISSION_INFO & info)
{
	JUMPPLAT_INIT * objtype = (JUMPPLAT_INIT *)hArchetype;
	EditorStartObjectInsertion(objtype->pArchetype, info);
}
//---------------------------------------------------------------------------
//-------------------------GLOBAL STARTUP------------------------------------
//---------------------------------------------------------------------------
//
struct _jumpplatfactory : GlobalComponent
{
	JumpPlatFactory * sfactory;

	virtual void Startup (void)
	{
		sfactory = new DAComponent<JumpPlatFactory>;
		AddToGlobalCleanupList((IDAComponent **) &sfactory);
	}

	virtual void Initialize (void)
	{
		sfactory->init();
	}
};

static _jumpplatfactory __plat;

//--------------------------------------------------------------------------//
//----------------------------End JumpPlat.cpp-------------------------------//
//--------------------------------------------------------------------------//
