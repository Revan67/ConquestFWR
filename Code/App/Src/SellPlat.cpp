//--------------------------------------------------------------------------//
//                                                                          //
//                               SellPlat.cpp                                //
//                                                                          //
//                  COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/SellPlat.cpp 17    10/17/00 6:15p Tmauer $
*/			    
//--------------------------------------------------------------------------//

#include "pch.h"
#include <globals.h>

#include "TPlatform.h"
#include "Startup.h"
#include "MGlobals.h"
#include "ObjList.h"
#include "GenData.h"
#include "IRepairPlatform.h"
#include "TFabricator.h"
#include "IRepairee.h"

#include <DSellPlat.h>

#include <Physics.h>
#include <3DMath.h>
#include <FileSys.h>
#include <IConnection.h>
#include <TComponent.h>
#include <HeapObj.h>

#define HOST_CHECK THEMATRIX->IsMaster()

#define COMP_OP(operation) { U32 tempID = workingID; workingID = 0; THEMATRIX->OperationCompleted(tempID,operation);}

struct BaseSellCommand
{
	U8 command;
};

struct SellCommandWHandle : public BaseSellCommand
{
	U32 handle;
};


//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//

struct _NO_VTABLE SellPlat :TFabricator<
									Platform<SELLPLAT_SAVELOAD, SELLPLAT_INIT>
								>,IRepairPlatform, BASE_SELLPLAT_SAVELOAD
{
	BEGIN_MAP_INBOUND(SellPlat)
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
	_INTERFACE_ENTRY(IRepairPlatform)
	_INTERFACE_ENTRY(IFabricator)
	_INTERFACE_ENTRY(IExtent)
	_INTERFACE_ENTRY(IEffectTarget)
	END_MAP()

	enum COMMUNICATION
	{
		SEL_C_BEGIN,
		SEL_C_SELL_FINISHED,
		SEL_C_SELL_FINISHED_CLEAN,
		SEL_C_TARGET_LOST,
		SEL_C_TARGET_TAKEN,
	};

	SaveNode		saveNode;
	LoadNode		loadNode;
	ResolveNode		resolveNode;
	UpdateNode		updateNode;
	ReceiveOpDataNode receiveOpNode;
	PreTakeoverNode	preTakeoverNode;
	PreDestructNode preDestructNode;

	OBJPTR<IBaseObject> sellTarget;

	HardpointInfo  shippointinfo;
	INSTANCE_INDEX shipPointIndex;

	SellPlat (void);

	virtual ~SellPlat (void);	// See ObjList.cpp

	/* IExplosionOwner methods */

	virtual void OnChildDeath (INSTANCE_INDEX child)
	{
	}

	// IRepairPlatform

	virtual TRANSFORM GetShipTransform ();

	virtual TRANSFORM GetDockTransform ()
	{
		return transform;
	}

	virtual bool IsDockLocked();

	virtual void LockDock(IBaseObject * locker);

	virtual void UnlockDock(IBaseObject * locker);

	virtual void BeginRepairOperation(U32 agentID, IBaseObject * repairee);

	virtual void RepaireeDocked ();

	virtual void RepaireeDestroyed ();

	virtual void RepaireeTakeover (); 

	virtual void UpgradeToRepair();

	//IFabricator

	virtual void BeginConstruction (U32 planetID, U32 slotID, U32 dwArchetypeDataID, U32 agentID) ;

	virtual void BeginConstruction (GRIDVECTOR position, U32 dwArchetypeDataID, U32 agentID);

	virtual void BeginConstruction (IBaseObject * jumpgate, U32 dwArchetypeDataID, U32 agentID);

	virtual void BeginRepair (U32 agentID, IBaseObject * repairTarget);

	virtual void BeginDismantle(U32 agentID, IBaseObject * dismantleTarget);

	virtual bool IsBuildingAgent (U32 agentID);

	virtual U8 GetFabTab()
	{return 0;};

	virtual void SetFabTab(U8 tab)
	{};

	virtual void BuildQueue (U32 dwArchetypeDataID, COMMANDS command);

	virtual void LocalStartBuild();

	virtual void LocalEndBuild(bool killAgent = false);

	virtual bool IsFabAtObj()
	{
		return true;
	}

	//IMissionActors

	virtual void OnMasterChange (bool bIsMaster);

	virtual void OnAddToOperation (U32 agentID);
	
	// IPlatform method

	virtual U32 GetPlanetID ()
	{
		return buildPlanetID;
	};

	virtual U32 GetSlotID ()
	{
		return buildSlot;
	};

	virtual void SetRallyPoint (const struct NETGRIDVECTOR & point)
	{
		CQBOMB0("What!?");
	}

	virtual bool GetRallyPoint (struct NETGRIDVECTOR & point)
	{
		return false;
	}

	virtual void FreePlanetSlot()
	{
		if (buildPlanetID && buildSlot)
		{
			IBaseObject * obj = OBJLIST->FindObject(buildPlanetID);
			OBJPTR<IPlanet> planet;

			if (obj && obj->QueryInterface(IPlanetID, planet))
				planet->DeallocateBuildSlot(dwMissionID, buildSlot);
			buildPlanetID = buildSlot = 0;
		}
	}

	virtual void ParkYourself (const TRANSFORM & _trans, U32 planetID, U32 slotID);
	
	virtual U32 GetNumDocking ()
	{
		CQBOMB0("What!?");
		return 0;
	}

	virtual void IncNumDocking ()
	{
		CQBOMB0("What!?");
	}

	virtual void DecNumDocking ()
	{
		CQBOMB0("What!?");
	}

	/* Platform methods */


	virtual const char * getSaveStructName (void) const		// must be overriden implemented by derived class
	{
		return "SELLPLAT_SAVELOAD";
	}

	virtual void * getViewStruct (void)				// must be overriden implemented by derived class
	{
		return static_cast<BASE_SELLPLAT_SAVELOAD *>(this);
	}

	/* BuildPlat methods */

	bool initSellPlat (const SELLPLAT_INIT & data);

	void save (SELLPLAT_SAVELOAD & save);

	void load (SELLPLAT_SAVELOAD & load);

	void resolve (void);

	BOOL32 updatePlat(void);

	void receiveOpData(U32 agentID,void * buffer, U32 bufferSize);

	void preDestructPlat(void);

	void preTakeoverPlat(U32 newMissionID,U32 troopID);
};
//---------------------------------------------------------------------------
//
SellPlat::SellPlat (void) :
			saveNode(this, CASTSAVELOADPROC(&SellPlat::save)),
			loadNode(this, CASTSAVELOADPROC(&SellPlat::load)),
			resolveNode(this, ResolveProc(&SellPlat::resolve)),
			updateNode(this, UpdateProc(&SellPlat::updatePlat)),
			receiveOpNode(this, ReceiveOpDataProc(&SellPlat::receiveOpData)),
			preTakeoverNode(this, PreTakeoverProc(&SellPlat::preTakeoverPlat)),
			preDestructNode(this, PreDestructProc(&SellPlat::preDestructPlat))
{
}
//---------------------------------------------------------------------------
//
SellPlat::~SellPlat (void)
{
}
//---------------------------------------------------------------------------
//
void SellPlat::ParkYourself (const TRANSFORM & _transform, U32 planetID, U32 slotID)
{
	transform = _transform;

	IBaseObject * obj = OBJLIST->FindObject(planetID);
	OBJPTR<IPlanet> planet;

	if (obj && obj->QueryInterface(IPlanetID, planet))
	{
		U32 slotUser = planet->GetSlotUser(slotID);
		if(slotUser != dwMissionID)
			planet->AllocateBuildSlot(dwMissionID, slotID);
		systemID = obj->GetSystemID();
	}
	
	buildPlanetID = planetID;
	buildSlot = slotID;
	if(mObjClass == M_JUMPPLAT || mObjClass == M_HQ || mObjClass == M_COCOON || mObjClass == M_ACROPOLIS || mObjClass == M_LOCUS)
	{
		SECTOR->ComputeSupplyForAllPlayers();
	}
}
//---------------------------------------------------------------------------
//
TRANSFORM SellPlat::GetShipTransform()
{
	TRANSFORM trans;
	trans.set_orientation(shippointinfo.orientation);
	trans.set_position(shippointinfo.point);
	trans = transform.multiply(trans);
	return trans;
}
//---------------------------------------------------------------------------
//
bool SellPlat::IsDockLocked()
{
	if(!bReady)
		return true;
	return bDockLocked;
}
//---------------------------------------------------------------------------
//
void SellPlat::LockDock(IBaseObject * locker)
{
	bDockLocked = true;
}
//---------------------------------------------------------------------------
//
void SellPlat::UnlockDock(IBaseObject * locker)
{
	bDockLocked = false;
}
//---------------------------------------------------------------------------
//
void SellPlat::BeginRepairOperation(U32 agentID, IBaseObject * repairee)
{
	CQASSERT((!bPlatDead) && "Tried to order a Dead plat to sell");
	if(bPlatDead)
	{
		THEMATRIX->OperationCompleted(agentID,dwMissionID);
		return;
	}
	if(HOST_CHECK)
	{
		CQASSERT(agentID);
		repairee->QueryInterface(IBaseObjectID, sellTarget, playerID);
		sellTargetID = repairee->GetPartID();

		mode = SEL_WAIT_FOR_DOCK;

		SellCommandWHandle buffer;
		buffer.command = SEL_C_BEGIN;
		buffer.handle = sellTargetID;

		workingID = agentID;
		THEMATRIX->AddObjectToOperation(workingID,dwMissionID);
		THEMATRIX->SendOperationData(workingID,dwMissionID,&buffer,sizeof(buffer));
		THEMATRIX->SetCancelState(workingID,false);
	}
}
//---------------------------------------------------------------------------
//
void SellPlat::RepaireeDocked ()
{
	mode = SEL_SELLING;
	CQASSERT(sellTarget);
	OBJPTR<IBuild> buildPtr;
	sellTarget->QueryInterface(IBuildID,buildPtr);
	FabStartDismantle(buildPtr);
}
//---------------------------------------------------------------------------
//
void SellPlat::RepaireeDestroyed ()
{
	if(workingID && THEMATRIX->IsMaster())
	{
		BaseSellCommand buffer;
		buffer.command = SEL_C_SELL_FINISHED;
		THEMATRIX->SendOperationData(workingID,dwMissionID,&buffer,sizeof(buffer));

		THEMATRIX->OperationCompleted(workingID,sellTargetID);
		if(sellTarget)
		{
			FabHaltBuild();
	//		THEMATRIX->ObjectTerminated(sellTargetID,0);
		}
		sellTargetID = 0;
		mode = SEL_NONE;
		THEMATRIX->OperationCompleted2(workingID,dwMissionID);
	}
}
//---------------------------------------------------------------------------
//
void SellPlat::RepaireeTakeover ()
{
	if(workingID && THEMATRIX->IsMaster())
	{
		if(sellTarget)
		{
			OBJPTR<IRepairee> repTarg;
			sellTarget->QueryInterface(IRepaireeID,repTarg);
			repTarg->RepairCompleted();
		}
		mode = SEL_NONE;
		BaseSellCommand buffer;
		buffer.command = SEL_C_TARGET_TAKEN;
		THEMATRIX->SendOperationData(workingID,dwMissionID,&buffer,sizeof(buffer));
		THEMATRIX->OperationCompleted(workingID,sellTargetID);
		COMP_OP(dwMissionID);
	}
}
//---------------------------------------------------------------------------
//
void SellPlat::UpgradeToRepair()
{
	CQASSERT(0 && "Tried to upgrade a sellPlat");
}
//---------------------------------------------------------------------------
//
void SellPlat::BeginConstruction (U32 planetID, U32 slotID, U32 dwArchetypeDataID, U32 agentID)
{
}
//---------------------------------------------------------------------------
//
void SellPlat::BeginConstruction (GRIDVECTOR position, U32 dwArchetypeDataID, U32 agentID)
{
}
//---------------------------------------------------------------------------
//
void SellPlat::BeginConstruction (IBaseObject * jumpgate, U32 dwArchetypeDataID, U32 agentID)
{
}
//---------------------------------------------------------------------------
//
void SellPlat::BeginRepair (U32 agentID, IBaseObject * repairTarget)
{
}
//---------------------------------------------------------------------------
//
void SellPlat::BeginDismantle(U32 agentID, IBaseObject * dismantleTarget)
{
}
//---------------------------------------------------------------------------
//
bool SellPlat::IsBuildingAgent (U32 agentID)
{
	CQASSERT(0 && "IsBuildingAgent not supported here.");
	return false;
}
//---------------------------------------------------------------------------
//
void SellPlat::BuildQueue (U32 dwArchetypeDataID, COMMANDS command)
{
}
//---------------------------------------------------------------------------
//
void SellPlat::LocalStartBuild()
{
}
//---------------------------------------------------------------------------
//
void SellPlat::LocalEndBuild(bool killAgent)
{
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//

bool SellPlat::initSellPlat (const SELLPLAT_INIT & data)
{
	workingID = 0;
	sellTargetID = 0;
	mode=SEL_NONE ;

	bDockLocked = false;

	if (data.pData->ship_hardpoint[0])
		FindHardpoint(data.pData->ship_hardpoint, shipPointIndex, shippointinfo, instanceIndex);

	return true;
}
//---------------------------------------------------------------------------
//
void SellPlat::save (SELLPLAT_SAVELOAD & save)
{
	save.sellPlatSaveload = *static_cast<BASE_SELLPLAT_SAVELOAD *>(this);
}
//---------------------------------------------------------------------------
//
void SellPlat::load (SELLPLAT_SAVELOAD & load)
{
 	*static_cast<BASE_SELLPLAT_SAVELOAD *>(this) = load.sellPlatSaveload;
}
//---------------------------------------------------------------------------
//
void SellPlat::resolve (void)
{
	if(sellTargetID)
	{
		OBJLIST->FindObject(sellTargetID,playerID,sellTarget);
	}
}
//---------------------------------------------------------------------------
//
BOOL32 SellPlat::updatePlat(void)
{
	if((!bExploding) && bReady && mode == SEL_SELLING)
	{
		if(THEMATRIX->IsMaster())
		{
			if(sellTarget)
			{
				OBJPTR<IBuild> buildPtr;
				sellTarget->QueryInterface(IBuildID,buildPtr);
				U32 dummy;
				if(buildPtr->GetBuildProgress(dummy) >= 1.0)
				{
					BaseSellCommand buffer;
					buffer.command = SEL_C_SELL_FINISHED_CLEAN;
					THEMATRIX->SendOperationData(workingID,dwMissionID,&buffer,sizeof(buffer));

					THEMATRIX->OperationCompleted(workingID,sellTargetID);
					FabEnableCompletion();
					ResourceCost cost = ((BASE_SPACESHIP_DATA *)(ARCHLIST->GetArchetypeData(sellTarget->pArchetype)))->missionData.resourceCost;
					BANKER->AddResource(playerID,cost);
//					BANKER->FreeCommandPt(playerID,cost.commandPt);
//					THEMATRIX->ObjectTerminated(sellTargetID);
//					UnlockDock(sellTarget);
					sellTargetID = 0;
					mode = SEL_NONE;
					THEMATRIX->OperationCompleted2(workingID,dwMissionID);
				}				
			}
			else
			{
				BaseSellCommand buffer;
				buffer.command = SEL_C_TARGET_LOST;
				THEMATRIX->SendOperationData(workingID,dwMissionID,&buffer,sizeof(buffer));

				THEMATRIX->OperationCompleted(workingID,sellTargetID);
				UnlockDock(NULL);
				sellTargetID = 0;
				mode = SEL_NONE;
				THEMATRIX->OperationCompleted2(workingID,dwMissionID);
			}
		}
	}
	return 1;
}
//---------------------------------------------------------------------------
//
void SellPlat::OnMasterChange (bool bIsMaster)
{
	TFabricator<
			Platform<SELLPLAT_SAVELOAD, SELLPLAT_INIT>
		>::OnMasterChange(bIsMaster);
}
//---------------------------------------------------------------------------
//
void SellPlat::OnAddToOperation (U32 agentID)
{
	potentialWorkingID = agentID;
}
//---------------------------------------------------------------------------
//
void SellPlat::receiveOpData(U32 agentID, void * buffer, U32 bufferSize)
{
	if(potentialWorkingID == agentID)
	{
		workingID = potentialWorkingID;
		potentialWorkingID = 0;
	}
	if(workingID == agentID)
	{
		BaseSellCommand * myBuf = (BaseSellCommand *) buffer;
		switch(myBuf->command)
		{
		case SEL_C_TARGET_TAKEN:
			{
				if(sellTarget)
				{
					OBJPTR<IRepairee> repTarg;
					sellTarget->QueryInterface(IRepaireeID,repTarg);
					repTarg->RepairCompleted();
				}
				mode = SEL_NONE;
				THEMATRIX->OperationCompleted(workingID,sellTargetID);
				COMP_OP(dwMissionID);
				break;
			}
		case SEL_C_BEGIN:
			{
				SellCommandWHandle * sellBuf = (SellCommandWHandle *) buffer;
				OBJLIST->FindObject(sellBuf->handle,playerID,sellTarget);
				sellTargetID = sellBuf->handle;

				OBJPTR<IRepairee> repTarg;
				sellTarget->QueryInterface(IRepaireeID,repTarg);
				repTarg->RepairStartReceived(this);

				mode = SEL_WAIT_FOR_DOCK;
				THEMATRIX->SetCancelState(workingID,false);

				break;
			}
		case SEL_C_SELL_FINISHED_CLEAN:
			{
				THEMATRIX->OperationCompleted(workingID,sellTargetID);
				FabEnableCompletion();
//				UnlockDock(sellTarget);
				sellTargetID = 0;
				mode = SEL_NONE;
				THEMATRIX->OperationCompleted2(workingID,dwMissionID);
				break;
			}
		case SEL_C_SELL_FINISHED:
			{
				THEMATRIX->OperationCompleted(workingID,sellTargetID);
				if(sellTarget)
				{
					FabHaltBuild();
					OBJPTR<IRepairee> repTarg;
					sellTarget->QueryInterface(IRepaireeID,repTarg);
					repTarg->RepairCompleted();
				}
				sellTargetID = 0;
				mode = SEL_NONE;
				THEMATRIX->OperationCompleted2(workingID,dwMissionID);
				break;
			}
		case SEL_C_TARGET_LOST:
			{
				THEMATRIX->OperationCompleted(workingID,sellTargetID);
				UnlockDock(NULL);
				sellTargetID = 0;
				mode = SEL_NONE;
				THEMATRIX->OperationCompleted2(workingID,dwMissionID);
				break;
			}
		}
	}
}
//---------------------------------------------------------------------------
//
void SellPlat::preDestructPlat(void)
{
	if(workingID && THEMATRIX->IsMaster())
	{
		BaseSellCommand buffer;
		buffer.command = SEL_C_SELL_FINISHED;
		THEMATRIX->SendOperationData(workingID,dwMissionID,&buffer,sizeof(buffer));

		THEMATRIX->OperationCompleted(workingID,sellTargetID);
		if(sellTarget)
		{
			FabHaltBuild();
			THEMATRIX->ObjectTerminated(sellTargetID,0);
		}
		UnlockDock(NULL);
		sellTargetID = 0;
		mode = SEL_NONE;
		THEMATRIX->OperationCompleted2(workingID,dwMissionID);
	}
	CQASSERT(!workingID);
}
//---------------------------------------------------------------------------
//
void SellPlat::preTakeoverPlat(U32 newMissionID,U32 troopID)
{
	if(workingID && THEMATRIX->IsMaster())
	{
		BaseSellCommand buffer;
		buffer.command = SEL_C_SELL_FINISHED;
		THEMATRIX->SendOperationData(workingID,dwMissionID,&buffer,sizeof(buffer));

		THEMATRIX->OperationCompleted(workingID,sellTargetID);
		if(sellTarget)
		{
			FabHaltBuild();

			OBJPTR<IRepairee> repTarg;
			sellTarget->QueryInterface(IRepaireeID,repTarg);
			repTarg->RepairCompleted();

			if(mode ==SEL_SELLING)
				THEMATRIX->ObjectTerminated(sellTargetID,0);
		}
		sellTargetID = 0;
		mode = SEL_NONE;
		THEMATRIX->OperationCompleted2(workingID,dwMissionID);
	}
	CQASSERT(!workingID);
}
//---------------------------------------------------------------------------
//
inline struct IBaseObject * createPlatform (const SELLPLAT_INIT & data)
{
	SellPlat * obj = new ObjectImpl<SellPlat>;

	obj->FRAME_init(data);
	if (obj->initSellPlat(data) == false)
	{
		delete obj;
		obj = 0;
	}

	return obj;
}
//------------------------------------------------------------------------------------------
//
SELLPLAT_INIT::~SELLPLAT_INIT (void)					// free archetype references
{
	// nothing yet!?
}
//------------------------------------------------------------------------------------------
//---------------------------SellPlat Factory----------------------------------------------
//------------------------------------------------------------------------------------------
//
struct DACOM_NO_VTABLE SellPlatFactory : public IObjectFactory
{
	U32 factoryHandle;		// handles to callback

	//
	// Interface mapping
	//

	BEGIN_DACOM_MAP_INBOUND(SellPlatFactory)
	DACOM_INTERFACE_ENTRY(IObjectFactory)
	END_DACOM_MAP()

	SellPlatFactory (void) { }

	~SellPlatFactory (void);

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

	/* SellPlatFactory methods */

	IDAComponent * getBase (void)
	{
		return static_cast<IObjectFactory *> (this);
	}
};
//--------------------------------------------------------------------------//
//
SellPlatFactory::~SellPlatFactory (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (OBJLIST && OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
		connection->Unadvise(factoryHandle);
}
//--------------------------------------------------------------------------//
//
void SellPlatFactory::init (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
		connection->Advise(getBase(), &factoryHandle);
}
//-----------------------------------------------------------------------------
//
HANDLE SellPlatFactory::CreateArchetype (const char *szArchname, OBJCLASS objClass, void *_data)
{
	SELLPLAT_INIT * result = 0;

	if (objClass == OC_PLATFORM)
	{
		BT_PLAT_SELL_DATA * data = (BT_PLAT_SELL_DATA *) _data;
		
		if (data->type == PC_SELL)
		{
			result = new SELLPLAT_INIT;

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
BOOL32 SellPlatFactory::DestroyArchetype (HANDLE hArchetype)
{
	SELLPLAT_INIT * objtype = (SELLPLAT_INIT *)hArchetype;

	EditorStopObjectInsertion(objtype->pArchetype);
	delete objtype;

	return 1;
}
//-------------------------------------------------------------------
//
IBaseObject * SellPlatFactory::CreateInstance (HANDLE hArchetype)
{
	SELLPLAT_INIT * objtype = (SELLPLAT_INIT *)hArchetype;
	return createPlatform(*objtype);
}
//-------------------------------------------------------------------
//
void SellPlatFactory::EditorCreateInstance (HANDLE hArchetype, const MISSION_INFO & info)
{
	SELLPLAT_INIT * objtype = (SELLPLAT_INIT *)hArchetype;
	EditorStartObjectInsertion(objtype->pArchetype, info);
}
//---------------------------------------------------------------------------
//-------------------------GLOBAL STARTUP------------------------------------
//---------------------------------------------------------------------------
//
struct _sellplatfactory : GlobalComponent
{
	SellPlatFactory * sfactory;

	virtual void Startup (void)
	{
		sfactory = new DAComponent<SellPlatFactory>;
		AddToGlobalCleanupList((IDAComponent **) &sfactory);
	}

	virtual void Initialize (void)
	{
		sfactory->init();
	}
};

static _sellplatfactory __splat;

//--------------------------------------------------------------------------//
//----------------------------End SellPlat.cpp-------------------------------//
//--------------------------------------------------------------------------//
