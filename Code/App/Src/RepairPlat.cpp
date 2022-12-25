//--------------------------------------------------------------------------//
//                                                                          //
//                               RepairPlat.cpp                                //
//                                                                          //
//                  COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/RepairPlat.cpp 117   6/22/01 9:56a Tmauer $
*/			    
//--------------------------------------------------------------------------//

#include "pch.h"
#include <globals.h>

#include "TPlatform.h"
#include "Startup.h"
#include "MGlobals.h"
#include "ObjList.h"
#include "GenData.h"
#include "IHardPoint.h"
#include "IRepairPlatform.h"
#include "ObjSet.h"
#include "IRepairee.h"
#include "CQLight.h"
#include "Isupplier.h"
#include "IFabricator.h"
#include "DSupplyShipSave.h"
#include "ObjMapIterator.h"

#include <DResearch.h>
#include <DRepairPlat.h>

#include <Physics.h>
#include <3DMath.h>
#include <FileSys.h>
#include <IConnection.h>
#include <TComponent.h>
#include <HeapObj.h>

#define RESUPPLY_TIME 10.0

#define HOST_CHECK THEMATRIX->IsMaster()

#define COMP_OP(operation) { U32 tempID = workingID; workingID = 0; THEMATRIX->OperationCompleted(tempID,operation);}

//REP_C_REPAIR_PAID
//REP_C_REPAIR_FINISHED
//REP_C_REPAIR_FINISHED_CLEAN
//REP_C_TARGET_LOST
//REC_C_UPGRADE_START
struct BaseRepairCommand
{
	U8 command;
};

//REP_C_REPAIR_FINISHED_NOMONEY
//REP_C_BEGIN
struct RepairCommandWHandle : public BaseRepairCommand
{
	U32 handle;
};

#define NUM_LINE_SEGS 40
#define CIRC_TIME 4.0

#define SUP_C_FORGETTARGET	0x00000000
#define SUP_C_NEWTARGET		0x00000001
#define SUP_C_UPGRADEEND	0x00000002

#pragma pack(push,1)
struct SupplySyncCommand
{
	U8 command:2;
};
struct SupplySyncNewCommand 
{
	U8 command:2;
	U32 target;
};
#pragma pack(pop)

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//

struct _NO_VTABLE RepairPlat :Platform<REPAIRPLAT_SAVELOAD, REPAIRPLAT_INIT> , IBuildQueue,
											ISpender, ISupplier, IRepairPlatform, BASE_REPAIRPLAT_SAVELOAD
{
	BEGIN_MAP_INBOUND(RepairPlat)
	_INTERFACE_ENTRY(IBaseObject)
	_INTERFACE_ENTRY(ISaveLoad)
	_INTERFACE_ENTRY(IQuickSaveLoad)
	_INTERFACE_ENTRY(IWeaponTarget)
	_INTERFACE_ENTRY(IExtent)
	_INTERFACE_ENTRY(IExplosionOwner)
	_INTERFACE_ENTRY(IMissionActor)
	_INTERFACE_ENTRY(IBuild)
	_INTERFACE_ENTRY(IPlatform)
	_INTERFACE_ENTRY(IPhysicalObject)
	_INTERFACE_ENTRY(IRepairPlatform)
	_INTERFACE_ENTRY(ISpender)
	_INTERFACE_ENTRY(IShipDamage)
	_INTERFACE_ENTRY(ISupplier)
	_INTERFACE_ENTRY(IBuildQueue)
	_INTERFACE_ENTRY(IUpgrade)
	_INTERFACE_ENTRY(IEffectTarget)
	END_MAP()

	enum COMMUNICATION
	{
		REP_C_BEGIN,
		REP_C_REPAIR_FINISHED,
		REP_C_REPAIR_FINISHED_CLEAN,
		REP_C_TARGET_LOST,
		REC_C_UPGRADE_START,
		REC_C_UPGRADE_END,
		REC_C_UPGRADE_DELAY_START,
		REC_C_UPGRADE_FORGET,
		P_OP_QUEUE_FAIL,
		REP_C_REPAIR_PAID,
		REP_C_REPAIR_FINISHED_NOMONEY,
		REP_C_TARGET_TAKEN
	};

	SaveNode		saveNode;
	LoadNode		loadNode;
	ResolveNode		resolveNode;
	UpdateNode		updateNode;
	PhysUpdateNode	physUpdateNode;
	OnOpCancelNode	onOpCancelNode;
	PreTakeoverNode preTakeoverNode;
	ReceiveOpDataNode	receiveOpDataNode;
	PreDestructNode	destructNode;
	UpgradeNode		upgradeNode;

	const BT_PLAT_REPAIR_DATA * pData;

	OBJPTR<IShuttle> repairShip;

	OBJPTR<IBaseObject> repairTarget;

	SINGLE circleTime;

	ResourceCost workCost;
	SINGLE workTime;

	HardpointInfo  shippointinfo;
	INSTANCE_INDEX shipPointIndex;
	
	bool bUpgradeCancel;

	RepairPlat (void);

	virtual ~RepairPlat (void);	// See ObjList.cpp

	/* IExplosionOwner methods */

	virtual void OnChildDeath (INSTANCE_INDEX child)
	{
	}

	// IBuildQueue methods
	virtual void BuildQueue (U32 dwArchetypeDataID, COMMANDS command); 
	virtual SINGLE FabGetProgress(U32 & stallType);

	virtual SINGLE FabGetDisplayProgress(U32 & stallType);

	virtual U32 GetFabJobID ();

	virtual U32 GetNumInQueue (U32 value)
	{
		CQBOMB0("Repair Platforms do not support this method");
		return 0;
	};

	virtual bool IsInQueue (U32 value) 
	{
		if(bUpgrading)
			return true;
		return false;
	};

	virtual bool IsUpgradeInQueue ()
	{
		return bUpgrading;
	}

	virtual U32 GetQueue(U32 * queueCopy,U32 * slotIDs = NULL) 
	{
		if(bUpgrading)
		{
			queueCopy[0] = GetFabJobID();
			if(slotIDs)
				slotIDs[0] = 0;
			return 1;
		}
		return 0;
	};

	// IPlatform method

	virtual U32 GetPlanetID ()
	{
		return buildPlanetID;
	};

	virtual U32 GetSlotID ()
	{
		return buildSlot;
	};

	virtual TRANSFORM GetShipTransform ();

	virtual TRANSFORM GetDockTransform ()
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

	virtual bool IsDockLocked()
	{
		if(!bReady)
			return true;
		return bDockLocked;
	}

	virtual void LockDock(IBaseObject * locker)
	{
		CQASSERT(!bDockLocked);
		bDockLocked = true;
	}

	virtual void UnlockDock(IBaseObject * locker)
	{
		CQASSERT(bDockLocked);
		bDockLocked = false;
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

	virtual void StopActions();
	
	/* IRepairPlatform methods */
	
	virtual void BeginRepairOperation(U32 agentID, IBaseObject * repairee);

	virtual void RepaireeDocked ();

	virtual void RepaireeDestroyed ();

	virtual void RepaireeTakeover ();

	virtual void UpgradeToRepair();

	/* ISupplier methods */

	virtual void SupplyTarget(U32 agentID, IBaseObject * obj);

	virtual void SetSupplyStance (enum SUPPLY_SHIP_STANCE _bAutoSupply)
	{
	};

	virtual enum SUPPLY_SHIP_STANCE GetSupplyStance ()
	{
		return SUP_STANCE_NONE;
	};

	virtual void SetSupplyEscort (U32 agentID, IBaseObject * target)
	{
		CQBOMB0("Not implemented! (ignorable)");
		THEMATRIX->OperationCompleted(agentID, dwMissionID);
	}

	// ISpender methods
	virtual ResourceCost GetAmountNeeded ();
	
	virtual void UnstallSpender ();

	/* IMissionActor */

	// the following are process related
	// called in response to OpAgent::CreateOperation()
	virtual void OnOperationCreation (U32 agentID, void *buffer, U32 bufferSize);

	// called in response to OpAgent::SendOperationData()
	void receiveOperationData (U32 agentID, void *buffer, U32 bufferSize);

	// user has requested a different action
	void onOperationCancel (U32 agentID);
		
	void preSelfDestruct (void);

	virtual void OnStopRequest (U32 agentID)
	{
	};

	virtual void OnMasterChange (bool bIsMaster);

	virtual void OnAddToOperation (U32 agentID);

	/* IBaseObject */

	virtual void TestVisible (const USER_DEFAULTS & defaults, const U32 currentSystem, const U32 currentPlayer);

	virtual void Render (void);

	virtual void MapRender (bool bPing);

	/* Platform methods */

	virtual const char * getSaveStructName (void) const		// must be overriden implemented by derived class
	{
		return "REPAIRPLAT_SAVELOAD";
	}

	virtual void * getViewStruct (void)				// must be overriden implemented by derived class
	{
		return static_cast<BASE_REPAIRPLAT_SAVELOAD *>(this);
	}

	/* BuildPlat methods */

	bool initRepairPlat (const REPAIRPLAT_INIT & data);

	void upgradeRep (const REPAIRPLAT_INIT & data);

	void save (REPAIRPLAT_SAVELOAD & save);

	void load (REPAIRPLAT_SAVELOAD & load);

	void resolve (void);

	void render (void);

	BOOL32 update (void);

	void physUpdate (SINGLE dt);

	void preTakeover (U32 newMissionID, U32 troopID);
};
//---------------------------------------------------------------------------
//
RepairPlat::RepairPlat (void) :
			saveNode(this, CASTSAVELOADPROC(&RepairPlat::save)),
			loadNode(this, CASTSAVELOADPROC(&RepairPlat::load)),
			resolveNode(this, ResolveProc(&RepairPlat::resolve)),
			updateNode(this,UpdateProc(&RepairPlat::update)),
			physUpdateNode(this,PhysUpdateProc(&RepairPlat::physUpdate)),
			onOpCancelNode(this, OnOpCancelProc(&RepairPlat::onOperationCancel)),
			preTakeoverNode(this, PreTakeoverProc(&RepairPlat::preTakeover)),
			receiveOpDataNode(this, ReceiveOpDataProc(&RepairPlat::receiveOperationData)),
			destructNode(this, PreDestructProc(&RepairPlat::preSelfDestruct)),
			upgradeNode(this, UpgradeProc(CASTINITPROC(&RepairPlat::upgradeRep)))
{
	circleTime = 0;
}
//---------------------------------------------------------------------------
//
RepairPlat::~RepairPlat (void)
{
	if(repairShip)
		delete repairShip.Ptr(); //not in obj list
}

#define NetworkFailSound(RESTYPE) \
	{\
		RepairCommandWHandle buffer;\
		buffer.command = P_OP_QUEUE_FAIL;\
		buffer.handle = RESTYPE;\
		if(workingID)\
		{\
			THEMATRIX->SendOperationData(workingID,dwMissionID,&buffer,sizeof(buffer));\
		}\
		else\
		{\
			workingID = THEMATRIX->CreateOperation(dwMissionID,&buffer,sizeof(buffer));\
			COMP_OP(dwMissionID);\
		}\
		failSound(RESTYPE);\
	}

//---------------------------------------------------------------------------
//
void RepairPlat::BuildQueue (U32 dwArchetypeDataID, COMMANDS command)
{
	switch(command)
	{
	case COMMANDS::ADDIFEMPTY:
			break;
		// fall through intentional
	case COMMANDS::ADD:
		{
			if(!bUpgrading)
			{
				upgradeID = dwArchetypeDataID;
				BT_UPGRADE * buildType = (BT_UPGRADE *)(ARCHLIST->GetArchetypeData(upgradeID));
				CQASSERT(buildType->objClass == OC_RESEARCH && buildType->type == RESEARCH_UPGRADE);

				workCost = buildType->cost;
				workTime = buildType->time;
				upgradeTimeSpent = 0;

				M_RESOURCE_TYPE failType;
				if(BANKER->SpendMoney(playerID,workCost,&failType))
				{
					if(BANKER->SpendCommandPoints(playerID,workCost))
					{
						bUpgrading = true;
						bUpgradeCancel = false;
						bUpgradeDelay = false;
						RepairCommandWHandle buffer;
						buffer.command = REC_C_UPGRADE_START;
						buffer.handle = upgradeID;
						if(!workingID)
						{
							workingID = THEMATRIX->CreateOperation(dwMissionID,&buffer,sizeof(buffer));

							UpgradeToRepair();

							COMP_OP(dwMissionID);
						}else
						{
							THEMATRIX->SendOperationData(workingID,dwMissionID,&buffer,sizeof(buffer));
							UpgradeToRepair();
						}
					}
					else
					{
						BANKER->AddResource(playerID,workCost);
						NetworkFailSound(M_COMMANDPTS);
					}
				}
				else
				{
					NetworkFailSound(failType);
				}
			}
			break;
		}
	case COMMANDS::REMOVE:
		{
			if(bUpgrading)
			{
				if(dwArchetypeDataID==0)
				{
					upgradeID = 0;
					CancelUpgrade();
					bUpgrading = false;
					if(!bUpgradeDelay)
					{
						BANKER->AddResource(playerID,workCost);
						BANKER->FreeCommandPt(playerID,workCost.commandPt);
					}

					BaseRepairCommand buffer;
					buffer.command = REC_C_UPGRADE_FORGET;
					if(!workingID)
					{
						workingID = THEMATRIX->CreateOperation(dwMissionID,&buffer,sizeof(buffer));

						COMP_OP(dwMissionID);
					}else
					{
						THEMATRIX->SendOperationData(workingID,dwMissionID,&buffer,sizeof(buffer));
					}			
				}
			}
			break;
		}
	case COMMANDS::PAUSE:
		{
			break;
		}
	}
}
//---------------------------------------------------------------------------
//
SINGLE RepairPlat::FabGetProgress(U32 & stallType)
{
	if (!bUpgrading)
		return 0.0f;

	if(bUpgradeDelay)
		stallType = IActiveButton::NO_MONEY;
	else
		stallType = IActiveButton::NO_STALL;
	return (upgradeTimeSpent/workTime);
}
//---------------------------------------------------------------------------
//
SINGLE RepairPlat::FabGetDisplayProgress(U32 & stallType)
{
	if (!bUpgrading)
		return 0.0f;

	if(bUpgradeDelay)
		stallType = IActiveButton::NO_MONEY;
	else
		stallType = IActiveButton::NO_STALL;
	return (upgradeTimeSpent/workTime);
}
//---------------------------------------------------------------------------
//
U32 RepairPlat::GetFabJobID ()
{
	if(workTime > 0)
		return upgradeID;
	else return 0;
}
//---------------------------------------------------------------------------
//
void RepairPlat::ParkYourself (const TRANSFORM & _transform, U32 planetID, U32 slotID)
{
	IBaseObject * obj = OBJLIST->FindObject(planetID);
	OBJPTR<IPlanet> planet;

	if (obj && obj->QueryInterface(IPlanetID, planet))
	{
		U32 slotUser = planet->GetSlotUser(slotID);
		if(slotUser != dwMissionID)
			planet->AllocateBuildSlot(dwMissionID, slotID);
		systemID = obj->GetSystemID();
	}

	SetTransform(_transform,systemID);
	
	buildPlanetID = planetID;
	buildSlot = slotID;

	if(repairShip)
	{
		repairShip->SetSystemID(systemID);
	}

	if(mObjClass == M_JUMPPLAT || mObjClass == M_HQ || mObjClass == M_COCOON || mObjClass == M_ACROPOLIS)
	{
		SECTOR->ComputeSupplyForAllPlayers();
	}
}
//---------------------------------------------------------------------------
//
void RepairPlat::StopActions()
{
	if(THEMATRIX->IsMaster() && bUpgrading)
	{
		bUpgradeCancel = true;
	}
}
//---------------------------------------------------------------------------
//
TRANSFORM RepairPlat::GetShipTransform()
{
	if(shipPointIndex)
	{
		TRANSFORM trans;
		trans.set_orientation(shippointinfo.orientation);
		trans.set_position(shippointinfo.point);
		trans = transform.multiply(trans);
		return trans;
	}
	return transform;
}
//---------------------------------------------------------------------------
//
void RepairPlat::SupplyTarget(U32 agentID, IBaseObject * obj)
{
	CQASSERT((!bPlatDead) && "Tried to order a Dead plat to repair");

	THEMATRIX->OperationCompleted(agentID,dwMissionID);
}
//---------------------------------------------------------------------------
//
void RepairPlat::BeginRepairOperation(U32 agentID,IBaseObject * repairee)
{
	CQASSERT((!bPlatDead) && "Tried to order a Dead plat to attack");
	if(bPlatDead)
	{
		THEMATRIX->OperationCompleted(agentID,dwMissionID);
		return;
	}
	if(HOST_CHECK)
	{
		CQASSERT(agentID);
		repairee->QueryInterface(IBaseObjectID, repairTarget, playerID);
		repairTargetID = repairee->GetPartID();
		bTakenCost = false;

		mode = REP_WAIT_FOR_DOCK;

		RepairCommandWHandle buffer;
		buffer.command = REP_C_BEGIN;
		buffer.handle = repairTargetID;

		workingID = agentID;
		THEMATRIX->AddObjectToOperation(workingID,dwMissionID);
		THEMATRIX->SendOperationData(workingID,dwMissionID,&buffer,sizeof(buffer));
		THEMATRIX->SetCancelState(workingID,false);
	}
}
//---------------------------------------------------------------------------
//
void RepairPlat::RepaireeDocked ()
{
	mode = REP_REPAIRING;

	MPart part = repairTarget;
	oldHullPoints = part->hullPoints;

	Vector pos,dir;
	OBJPTR<IShipDamage> shipDamage;
	repairTarget->QueryInterface(IShipDamageID,shipDamage);
	CQASSERT(shipDamage);
	shipDamage->GetNextDamageSpot(pos,dir);
	if(repairShip)
		repairShip->WorkAtShipPos(repairTarget,pos,dir,300);
}
//---------------------------------------------------------------------------
//
void RepairPlat::RepaireeDestroyed ()
{
	if(THEMATRIX->IsMaster())
	{
		if(repairTarget)
		{
			OBJPTR<IRepairee> repTarg;
			repairTarget->QueryInterface(IRepaireeID,repTarg);
			repTarg->RepairCompleted();

			BaseRepairCommand buffer;
			buffer.command = REP_C_REPAIR_FINISHED;
			THEMATRIX->SendOperationData(workingID,dwMissionID,&buffer,sizeof(buffer));

			mode = REP_NONE;
			if(repairShip)
				repairShip->Return();

			THEMATRIX->OperationCompleted(workingID,repairTargetID);
			COMP_OP(dwMissionID);
		}
		else
		{
			BaseRepairCommand buffer;
			buffer.command = REP_C_TARGET_LOST;
			THEMATRIX->SendOperationData(workingID,dwMissionID,&buffer,sizeof(buffer));

			mode = REP_NONE;
			COMP_OP(dwMissionID);
		}
	}
}
//---------------------------------------------------------------------------
//
void RepairPlat::RepaireeTakeover()
{
	if(THEMATRIX->IsMaster())
	{
		if(repairTarget)
		{
			OBJPTR<IRepairee> repTarg;
			repairTarget->QueryInterface(IRepaireeID,repTarg);
			repTarg->RepairCompleted();
		}
		if(repairShip)
			repairShip->Return();
		mode = REP_NONE;
		BaseRepairCommand buffer;
		buffer.command = REP_C_TARGET_TAKEN;
		THEMATRIX->SendOperationData(workingID,dwMissionID,&buffer,sizeof(buffer));
		THEMATRIX->OperationCompleted(workingID,repairTargetID);
		COMP_OP(dwMissionID);
	}
}
//---------------------------------------------------------------------------
//
void RepairPlat::UpgradeToRepair()
{
	BT_UPGRADE * buildType = (BT_UPGRADE *)(ARCHLIST->GetArchetypeData(upgradeID));
	StartUpgrade(buildType->extensionID,buildType->time);
}
//---------------------------------------------------------------------------
//
ResourceCost RepairPlat::GetAmountNeeded ()
{
	return workCost;
}
//---------------------------------------------------------------------------
//
void RepairPlat::UnstallSpender ()
{
	if(bUpgradeDelay)
	{
		upgradeTimeSpent = 0;
		bUpgradeDelay = false;
		RepairCommandWHandle buffer;
		buffer.command = REC_C_UPGRADE_START;
		buffer.handle = upgradeID;
		if(!workingID)
		{
			workingID = THEMATRIX->CreateOperation(dwMissionID,&buffer,sizeof(buffer));
			UpgradeToRepair();

			COMP_OP(dwMissionID);
		}else
		{
			THEMATRIX->SendOperationData(workingID,dwMissionID,&buffer,sizeof(buffer));
			UpgradeToRepair();
		}
	}
}
//---------------------------------------------------------------------------
//
void RepairPlat::OnOperationCreation (U32 agentID, void *buffer, U32 bufferSize)
{
	CQASSERT(!workingID);
	workingID = agentID;
	BaseRepairCommand * buf = (BaseRepairCommand *) buffer;
	switch(buf->command)
	{
	case P_OP_QUEUE_FAIL:
		{
			RepairCommandWHandle * com = (RepairCommandWHandle *) buf;
			failSound((M_RESOURCE_TYPE)(com->handle));
			COMP_OP(dwMissionID);
			break;
		}
	case REC_C_UPGRADE_FORGET:
		{
			upgradeID = 0;
			CancelUpgrade();
			bUpgrading = false;
			COMP_OP(dwMissionID);
			break;
		}
	case REC_C_UPGRADE_END:
		{
			bUpgrading = false;
			caps.repairOk = true;
			BT_UPGRADE * buildType = (BT_UPGRADE *)(ARCHLIST->GetArchetypeData(upgradeID));
			COMM_MUTATION_COMPLETED(((char *)(buildType->resFinishedSound)),buildType->resFinishSubtitle,pInitData->displayName);
			SetUpgrade(buildType->extensionID);	
			COMP_OP(dwMissionID);
			break;
		}
	case REC_C_UPGRADE_DELAY_START:
		{
			RepairCommandWHandle * myBuf = (RepairCommandWHandle *) buffer;
			
			upgradeID = myBuf->handle;

			BT_UPGRADE * buildType = (BT_UPGRADE *)(ARCHLIST->GetArchetypeData(upgradeID));
			CQASSERT(buildType->objClass == OC_RESEARCH && buildType->type == RESEARCH_UPGRADE);

			workCost = buildType->cost;
			workTime = buildType->time;
			upgradeTimeSpent = 0;

			bUpgrading = true;
			bUpgradeCancel = false;
			bUpgradeDelay = true;
			BANKER->AddStalledSpender(playerID,this);

			COMP_OP(dwMissionID);

			break;
		}
	case REC_C_UPGRADE_START:
		{
			if(bUpgradeDelay)
				BANKER->RemoveStalledSpender(playerID,this);
			RepairCommandWHandle * myBuf = (RepairCommandWHandle *) buffer;
			
			upgradeID = myBuf->handle;

			BT_UPGRADE * buildType = (BT_UPGRADE *)(ARCHLIST->GetArchetypeData(upgradeID));
			CQASSERT(buildType->objClass == OC_RESEARCH && buildType->type == RESEARCH_UPGRADE);

			workCost = buildType->cost;
			workTime = buildType->time;
			upgradeTimeSpent = 0;

			bUpgrading = true;
			bUpgradeCancel = false;
			bUpgradeDelay = false;

			UpgradeToRepair();

			COMP_OP(dwMissionID);
		}
		break;
	default:
		{
			CQASSERT(0 && "Bad data");
		}
	}
}
//---------------------------------------------------------------------------
//
void RepairPlat::receiveOperationData (U32 agentID, void *buffer, U32 bufferSize)
{
	if(potentialWorkingID == agentID)
	{
		workingID = potentialWorkingID;
	}
	if(workingID != agentID)
		return;
	BaseRepairCommand * buf = (BaseRepairCommand *) buffer;
	switch(buf->command)
	{
	case P_OP_QUEUE_FAIL:
		{
			RepairCommandWHandle * com = (RepairCommandWHandle *) buf;
			failSound((M_RESOURCE_TYPE)(com->handle));
			COMP_OP(dwMissionID);
			break;
		}
	case REC_C_UPGRADE_FORGET:
		{
			upgradeID = 0;
			CancelUpgrade();
			bUpgrading = false;
			break;
		}
	case REC_C_UPGRADE_END:
		{
			bUpgrading = false;
			caps.repairOk = true;
			BT_UPGRADE * buildType = (BT_UPGRADE *)(ARCHLIST->GetArchetypeData(upgradeID));
			COMM_MUTATION_COMPLETED(((char *)(buildType->resFinishedSound)),buildType->resFinishSubtitle,pInitData->displayName);
			SetUpgrade(buildType->extensionID);	
			break;
		}
	case REC_C_UPGRADE_DELAY_START:
		{
			RepairCommandWHandle * myBuf = (RepairCommandWHandle *) buffer;
			
			upgradeID = myBuf->handle;

			BT_UPGRADE * buildType = (BT_UPGRADE *)(ARCHLIST->GetArchetypeData(upgradeID));
			CQASSERT(buildType->objClass == OC_RESEARCH && buildType->type == RESEARCH_UPGRADE);

			workCost = buildType->cost;
			workTime = buildType->time;
			upgradeTimeSpent = 0;

			BANKER->AddStalledSpender(playerID,this);

			bUpgrading = true;
			bUpgradeCancel = false;
			bUpgradeDelay = true;

			break;
		}
	case REC_C_UPGRADE_START:
		{
			RepairCommandWHandle * myBuf = (RepairCommandWHandle *) buffer;
			
			upgradeID = myBuf->handle;

			BT_UPGRADE * buildType = (BT_UPGRADE *)(ARCHLIST->GetArchetypeData(upgradeID));
			CQASSERT(buildType->objClass == OC_RESEARCH && buildType->type == RESEARCH_UPGRADE);

			if(bUpgradeDelay)
				BANKER->RemoveStalledSpender(playerID,this);

			workCost = buildType->cost;
			workTime = buildType->time;
			upgradeTimeSpent = 0;

			bUpgrading = true;
			bUpgradeCancel = false;
			bUpgradeDelay = false;

			UpgradeToRepair();

		}
		break;
	case REP_C_REPAIR_PAID:
		{
			bTakenCost = true;
		}
		break;
	case REP_C_REPAIR_FINISHED_CLEAN:
		{
			if(repairTarget)
			{
				Vector pos,dir;
				OBJPTR<IShipDamage> shipDamage;
				repairTarget->QueryInterface(IShipDamageID,shipDamage);
				CQASSERT(shipDamage);
				while(shipDamage->GetNumDamageSpots())
				{
					shipDamage->GetNextDamageSpot(pos,dir);
					shipDamage->FixDamageSpot();
				}
			}

			THEMATRIX->OperationCompleted(workingID,repairTargetID);
			if(repairTarget)
			{
				OBJPTR<IRepairee> repTarg;
				repairTarget->QueryInterface(IRepaireeID,repTarg);
				repTarg->RepairCompleted();
			}
			repairTargetID = 0;
			if(repairShip)
				repairShip->Return();
			mode = REP_NONE;
			COMP_OP(dwMissionID);
			break;
		}
	case REP_C_REPAIR_FINISHED_NOMONEY:
		{
			RepairCommandWHandle * buffer = (RepairCommandWHandle *) buf;
			failSound((M_RESOURCE_TYPE)(buffer->handle));
		}
			//no break
	case REP_C_REPAIR_FINISHED:
		{
			THEMATRIX->OperationCompleted(workingID,repairTargetID);
			if(repairTarget)
			{
				OBJPTR<IRepairee> repTarg;
				repairTarget->QueryInterface(IRepaireeID,repTarg);
				repTarg->RepairCompleted();
			}
			repairTargetID = 0;
			if(repairShip)
				repairShip->Return();
			mode = REP_NONE;
			COMP_OP(dwMissionID);
			break;
		}
	case REP_C_TARGET_TAKEN:
		{
			if(repairTarget)
			{
				OBJPTR<IRepairee> repTarg;
				repairTarget->QueryInterface(IRepaireeID,repTarg);
				repTarg->RepairCompleted();
			}
			if(repairShip)
				repairShip->Return();
			mode = REP_NONE;
			THEMATRIX->OperationCompleted(workingID,repairTargetID);
			COMP_OP(dwMissionID);
			break;
		}
	case REP_C_TARGET_LOST:
		{
			if(repairShip)
				repairShip->Return();
			mode = REP_NONE;
			COMP_OP(dwMissionID);
			break;
		}
	case REP_C_BEGIN:
		{
			RepairCommandWHandle * myBuf = (RepairCommandWHandle *) buffer;
			repairTargetID = myBuf->handle;
			OBJLIST->FindObject(repairTargetID, playerID, repairTarget);
			mode = REP_WAIT_FOR_DOCK;
			bTakenCost = false;
			OBJPTR<IRepairee> repTarg;
			repairTarget->QueryInterface(IRepaireeID,repTarg);
			repTarg->RepairStartReceived(this);
			THEMATRIX->SetCancelState(workingID,false);
			break;
		}
	default:
		{
			CQASSERT(0 && "Bad data");
		}
	}
}
//---------------------------------------------------------------------------
//
void RepairPlat::onOperationCancel (U32 agentID)
{
}
//---------------------------------------------------------------------------
//
void RepairPlat::preTakeover (U32 newMissionID, U32 troopID)
{
	if(HOST_CHECK)
	{
		if(bUpgrading)
		{
			upgradeID = 0;
			CancelUpgrade();
			bUpgrading = false;
			BANKER->FreeCommandPt(playerID,workCost.commandPt);

			BaseRepairCommand buffer;
			buffer.command = REC_C_UPGRADE_FORGET;
			if(!workingID)
			{
				workingID = THEMATRIX->CreateOperation(dwMissionID,&buffer,sizeof(buffer));

				COMP_OP(dwMissionID);
			}else
			{
				THEMATRIX->SendOperationData(workingID,dwMissionID,&buffer,sizeof(buffer));
			}			
		}
		if(workingID)
		{
			if(repairTarget)
			{
				OBJPTR<IRepairee> repTarg;
				repairTarget->QueryInterface(IRepaireeID,repTarg);
				repTarg->RepairCompleted();

				BaseRepairCommand buffer;
				buffer.command = REP_C_REPAIR_FINISHED;
				THEMATRIX->SendOperationData(workingID,dwMissionID,&buffer,sizeof(buffer));

				mode = REP_NONE;
				if(repairShip)
					repairShip->Return();

				THEMATRIX->OperationCompleted(workingID,repairTargetID);
				COMP_OP(dwMissionID);
			}
			else
			{
				BaseRepairCommand buffer;
				buffer.command = REP_C_TARGET_LOST;
				THEMATRIX->SendOperationData(workingID,dwMissionID,&buffer,sizeof(buffer));

				mode = REP_NONE;
				COMP_OP(dwMissionID);
			}
		}
	}
}
//---------------------------------------------------------------------------
//
void RepairPlat::preSelfDestruct (void)
{
	if(HOST_CHECK)
	{
		if(workingID)
		{
			if(repairTarget)
			{
				OBJPTR<IRepairee> repTarg;
				repairTarget->QueryInterface(IRepaireeID,repTarg);
				repTarg->RepairCompleted();

				BaseRepairCommand buffer;
				buffer.command = REP_C_REPAIR_FINISHED;
				THEMATRIX->SendOperationData(workingID,dwMissionID,&buffer,sizeof(buffer));

				mode = REP_NONE;
				if(repairShip)
					repairShip->Return();

				THEMATRIX->OperationCompleted(workingID,repairTargetID);
				COMP_OP(dwMissionID);
			}
			else
			{
				BaseRepairCommand buffer;
				buffer.command = REP_C_TARGET_LOST;
				THEMATRIX->SendOperationData(workingID,dwMissionID,&buffer,sizeof(buffer));

				mode = REP_NONE;
				COMP_OP(dwMissionID);
			}
		}
	}
}
//---------------------------------------------------------------------------
//
void RepairPlat::OnMasterChange (bool bIsMaster)
{
	Platform<REPAIRPLAT_SAVELOAD, REPAIRPLAT_INIT>::OnMasterChange(bIsMaster);
}
//---------------------------------------------------------------------------
//
void RepairPlat::OnAddToOperation (U32 agentID)
{
	potentialWorkingID = agentID;
}
//---------------------------------------------------------------------------
//
void RepairPlat::TestVisible (const USER_DEFAULTS & defaults, const U32 currentSystem, const U32 currentPlayer)
{
	Platform<REPAIRPLAT_SAVELOAD, REPAIRPLAT_INIT>::TestVisible(defaults, currentSystem, currentPlayer);

	if(repairShip)
		repairShip.Ptr()->TestVisible(defaults, currentSystem, currentPlayer);
}
//-------------------------------------------------------------------
//
void RepairPlat::Render (void)
{
	Platform<REPAIRPLAT_SAVELOAD, REPAIRPLAT_INIT>::Render();

	if(!bPlatDead)
	{
		if(repairShip)
			repairShip.Ptr()->Render();
	}
	render();
}
//-------------------------------------------------------------------
//
void RepairPlat::MapRender(bool bPing)
{
	Platform<REPAIRPLAT_SAVELOAD, REPAIRPLAT_INIT>::MapRender(bPing);
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//

bool RepairPlat::initRepairPlat (const REPAIRPLAT_INIT & data)
{
	pData = data.pData;

	FindHardpoint("\\hp_repairship", shipPointIndex, shippointinfo, instanceIndex);

	workTime = 0;
	timeSpent = 0;
	bUpgrading = false;
	bUpgradeDelay = false;

	bDockLocked = false;
	mode = REP_NONE;

	//create resupply drone
	if(pData->missionData.caps.repairOk && pData->repairDroneType[0])
	{
		IBaseObject *obj = ARCHLIST->CreateInstance(data.repairDroneType);
		if (obj)
		{
			obj->QueryInterface(IShuttleID,repairShip,playerID);
			CQASSERT(repairShip != 0);
			repairShip->InitBuildShip(this);
			repairShip->SetSystemID(systemID);
			if (pData->repairDroneHardpoint[0])
			{
				INSTANCE_INDEX engIndex;
				HardpointInfo hpInfo;
				FindHardpoint(pData->repairDroneHardpoint, engIndex, hpInfo, instanceIndex);
				if(repairShip)
				{
					repairShip->SetStartHardpoint(hpInfo,engIndex);
				}
			}
		}		
	}

	return true;
}
//---------------------------------------------------------------------------
//
void RepairPlat::upgradeRep (const REPAIRPLAT_INIT & data)
{
	pData = data.pData;
}
//---------------------------------------------------------------------------
//
void RepairPlat::save (REPAIRPLAT_SAVELOAD & save)
{
	save.repairPlatSaveload = *static_cast<BASE_REPAIRPLAT_SAVELOAD *>(this);

	if(repairShip)
		save.repairTransform = repairShip.Ptr()->GetTransform();
}
//---------------------------------------------------------------------------
//
void RepairPlat::load (REPAIRPLAT_SAVELOAD & load)
{
 	*static_cast<BASE_REPAIRPLAT_SAVELOAD *>(this) = load.repairPlatSaveload;
	if(repairShip)
	{
		repairShip->SetSystemID(systemID);
		repairShip->SetTransform(load.repairTransform);
	}
}
//---------------------------------------------------------------------------
//
void RepairPlat::resolve (void)
{
	if(bUpgrading)
	{
		BT_UPGRADE * buildType = (BT_UPGRADE *)(ARCHLIST->GetArchetypeData(upgradeID));
		CQASSERT(buildType->objClass == OC_RESEARCH && buildType->type == RESEARCH_UPGRADE);

		workCost = buildType->cost;
		workTime = buildType->time;
		if(bUpgradeDelay)
		{
			BANKER->AddStalledSpender(playerID,this);
		}
	}
	if(repairTargetID)
	{
		OBJLIST->FindObject(repairTargetID, playerID, repairTarget);
	}
}
//---------------------------------------------------------------------------
//
void RepairPlat::render (void)
{
	if((systemID == SECTOR->GetCurrentSystem()) && (bHighlight || bSelected || BUILDARCHEID) && bReady&& MGlobals::AreAllies(playerID,MGlobals::GetThisPlayer()))
	{
		circleTime += 0.03;
		while(circleTime >CIRC_TIME)
		{
			circleTime -= CIRC_TIME;
		}

		BATCH->set_state(RPR_BATCH,true);
		DisableTextures();
		CAMERA->SetModelView();
		BATCH->set_render_state(D3DRS_ZENABLE,FALSE);
		BATCH->set_render_state(D3DRS_ZWRITEENABLE,FALSE);
		BATCH->set_render_state(D3DRS_ALPHABLENDENABLE,TRUE);
		BATCH->set_render_state(D3DRS_SRCBLEND,D3DBLEND_ONE);
		BATCH->set_render_state(D3DRS_DESTBLEND,D3DBLEND_ONE);
		PB.Begin(PB_LINES);
		COLORREF color;
		if(SECTOR->SystemInSupply(systemID,playerID))
		{
			PB.Color3ub(64,128,64);
			color = RGB(64,128,64);
		}
		else
		{
			PB.Color3ub(128,64,64);
			color = RGB(128,64,64);
		}
		SINGLE realRange = pData->supplyRange*MGlobals::GetTenderUpgrade(this);
		Vector relDir =Vector(cos((2*PI*circleTime)/CIRC_TIME)*realRange*GRIDSIZE,
					sin((2*PI*circleTime)/CIRC_TIME)*realRange*GRIDSIZE,0);
		Vector oldVect = transform.translation+relDir;
		PB.Vertex3f(oldVect.x,oldVect.y,oldVect.z);
		oldVect = transform.translation-relDir;
		PB.Vertex3f(oldVect.x,oldVect.y,oldVect.z);
		PB.End();
		drawRangeCircle(realRange,color);
	}
}
//---------------------------------------------------------------------------
//
BOOL32 RepairPlat::update (void)
{
	if(bUpgrading && (!bExploding) && (!bUpgradeDelay))
	{
		upgradeTimeSpent += ELAPSED_TIME;
		if(HOST_CHECK)
		{
			if(bUpgradeCancel)
			{
				bUpgradeCancel = false;
				BANKER->FreeCommandPt(playerID,workCost.commandPt);
				upgradeID = 0;
				CancelUpgrade();
				bUpgrading = false;
				if(!bUpgradeDelay)
					BANKER->AddResource(playerID,workCost);

				BaseRepairCommand buffer;
				buffer.command = REC_C_UPGRADE_FORGET;
				if(!workingID)
				{
					workingID = THEMATRIX->CreateOperation(dwMissionID,&buffer,sizeof(buffer));

					COMP_OP(dwMissionID);
				}else
				{
					THEMATRIX->SendOperationData(workingID,dwMissionID,&buffer,sizeof(buffer));
				}			
			}
			else if(upgradeTimeSpent > workTime)
			{
				BaseRepairCommand buffer;
				buffer.command = REC_C_UPGRADE_END;
				if(!workingID)
				{
					workingID = THEMATRIX->CreateOperation(dwMissionID,&buffer,sizeof(buffer));

					COMP_OP(dwMissionID);
				}else
				{
					THEMATRIX->SendOperationData(workingID,dwMissionID,&buffer,sizeof(buffer));
				}
				caps.repairOk = true;
				BT_UPGRADE * buildType = (BT_UPGRADE *)(ARCHLIST->GetArchetypeData(upgradeID));
				COMM_MUTATION_COMPLETED(((char *)(buildType->resFinishedSound)),buildType->resFinishSubtitle,pInitData->displayName);
				SetUpgrade(buildType->extensionID);	
				bUpgrading = false;
			}
		}
		if(upgradeTimeSpent >= 0 && upgradeTimeSpent <= workTime)
			SetUpgradePercent(upgradeTimeSpent/workTime);
	}

	if(HOST_CHECK && (!bExploding) && bReady && SECTOR->SystemInSupply(systemID,playerID))
	{
		supplyTimer += ELAPSED_TIME;
		if(supplyTimer > RESUPPLY_TIME)
		{
			supplyTimer -=RESUPPLY_TIME;
			U32 resupplyAmount = RESUPPLY_TIME*pData->supplyPerSecond;
			if(resupplyAmount && !fieldFlags.suppliesLocked())
			{
				IBaseObject * searchObj = NULL;
				SINGLE realRange = pData->supplyRange*MGlobals::GetTenderUpgrade(this);
				ObjMapIterator iter(systemID,GetPosition(),realRange*GRIDSIZE,playerID);
				while(iter)
				{
					U32 hisPlayerID=iter.GetApparentPlayerID(MGlobals::GetAllyMask(playerID));
					searchObj = iter->obj;
					if(hisPlayerID && MGlobals::AreAllies(hisPlayerID,playerID))
					{
						MPartNC part(searchObj);
						if(part.isValid() && part->bReady && (part->mObjClass != M_FABRICATOR) && (part->mObjClass != M_HARVEST) && 
							(part->mObjClass != M_GALIOT) &&(part->mObjClass != M_SIPHON)
							&& (part->supplyPointsMax != 0) && (searchObj->GetGridPosition()-GetGridPosition() < realRange) &&
							part->supplies < part->supplyPointsMax)
						{
							U32 need = part->supplyPointsMax - part->supplies;
							if(need > resupplyAmount)
								need = resupplyAmount;
							part->supplies += need;
						}
					}
					++iter;
				}
			}// end if mode == idle or idle op
		}//end if is update frame		
	}

	if(repairShip)
		repairShip.Ptr()->Update();
	if((mode == REP_REPAIRING) && HOST_CHECK)
	{
		if(repairTarget)
		{
			if(SECTOR->SystemInSupply(systemID,playerID))
			{
				if(!bTakenCost)
				{
					MPart part(repairTarget);
					ResourceCost cost;
					cost.commandPt = 0;
					cost.crew = (part.pInit->resourceCost.crew*(part->hullPointsMax - part->hullPoints))/(part->hullPointsMax*3);
					cost.metal = (part.pInit->resourceCost.metal*(part->hullPointsMax - part->hullPoints))/(part->hullPointsMax*3);
					cost.gas = (part.pInit->resourceCost.gas*(part->hullPointsMax - part->hullPoints))/(part->hullPointsMax*3);
					M_RESOURCE_TYPE failType;
					if(BANKER->SpendMoney(playerID,cost,&failType))
					{
						bTakenCost = true;
						BaseRepairCommand buffer;
						buffer.command = REP_C_REPAIR_PAID;
						THEMATRIX->SendOperationData(workingID,dwMissionID,&buffer,sizeof(buffer));						
					}
					else
					{
						failSound(failType);
						OBJPTR<IRepairee> repTarg;
						repairTarget->QueryInterface(IRepaireeID,repTarg);
						repTarg->RepairCompleted();

						RepairCommandWHandle buffer;
						buffer.command = REP_C_REPAIR_FINISHED_NOMONEY;
						buffer.handle = failType;
						THEMATRIX->SendOperationData(workingID,dwMissionID,&buffer,sizeof(buffer));

						mode = REP_NONE;
						if(repairShip)
							repairShip->Return();

						THEMATRIX->OperationCompleted(workingID,repairTargetID);
						COMP_OP(dwMissionID);
					}
				}
				if(bTakenCost)
				{
					MPartNC part(repairTarget);
					bool hullDone = true;
					if(caps.repairOk)
					{
						U32 newHullPoints = part->hullPoints + (pData->repairRate/REALTIME_FRAMERATE);
						if(newHullPoints > part->hullPointsMax)
							newHullPoints = part->hullPointsMax;
						part->hullPoints = newHullPoints;
						if((((SINGLE)(newHullPoints-oldHullPoints))/part->hullPointsMax) >= 0.12)
						{
							oldHullPoints = newHullPoints;
							OBJPTR<IShipDamage> shipDamage;
							repairTarget->QueryInterface(IShipDamageID,shipDamage);
							CQASSERT(shipDamage);
							shipDamage->FixDamageSpot();
							Vector pos,dir;
							shipDamage->GetNextDamageSpot(pos,dir);
							if(repairShip)
								repairShip->WorkAtShipPos(repairTarget,pos,dir,300);
						}
						if(newHullPoints != part->hullPointsMax)
							hullDone = false;
					}

					bool supplyDone = true;
					if((part->mObjClass != M_HARVEST) && 
						(part->mObjClass != M_GALIOT) &&(part->mObjClass != M_SIPHON))
					{
						U32 newSupplyPoints = part->supplies + (pData->supplyRate/REALTIME_FRAMERATE);
						if(newSupplyPoints > part->supplyPointsMax)
							newSupplyPoints = part->supplyPointsMax;
						part->supplies = newSupplyPoints;
						if(newSupplyPoints != part->supplyPointsMax)
							supplyDone = false;
					}

					if(supplyDone && hullDone)
					{
						OBJPTR<IShipDamage> shipDamage;
						if(caps.repairOk)
						{
							repairTarget->QueryInterface(IShipDamageID,shipDamage);
							CQASSERT(shipDamage);
							while(shipDamage->GetNumDamageSpots())
							{
								Vector pos,dir;
								shipDamage->GetNextDamageSpot(pos,dir);
								shipDamage->FixDamageSpot();
							}
						}
						OBJPTR<IRepairee> repTarg;
						repairTarget->QueryInterface(IRepaireeID,repTarg);
						repTarg->RepairCompleted();

						BaseRepairCommand buffer;
						buffer.command = REP_C_REPAIR_FINISHED_CLEAN;
						THEMATRIX->SendOperationData(workingID,dwMissionID,&buffer,sizeof(buffer));

						mode = REP_NONE;
						if(repairShip)
							repairShip->Return();
						THEMATRIX->OperationCompleted(workingID,repairTargetID);
						COMP_OP(dwMissionID);
					}
				}
			}
			else
			{
				OBJPTR<IRepairee> repTarg;
				repairTarget->QueryInterface(IRepaireeID,repTarg);
				repTarg->RepairCompleted();

				BaseRepairCommand buffer;
				buffer.command = REP_C_REPAIR_FINISHED;
				THEMATRIX->SendOperationData(workingID,dwMissionID,&buffer,sizeof(buffer));

				mode = REP_NONE;
				if(repairShip)
					repairShip->Return();

				THEMATRIX->OperationCompleted(workingID,repairTargetID);
				COMP_OP(dwMissionID);
			}
		}
		else
		{
			BaseRepairCommand buffer;
			buffer.command = REP_C_TARGET_LOST;
			THEMATRIX->SendOperationData(workingID,dwMissionID,&buffer,sizeof(buffer));

			mode = REP_NONE;
			COMP_OP(dwMissionID);
		}
	}
	return true;
}
//---------------------------------------------------------------------------
//
void RepairPlat::physUpdate( SINGLE dt)
{
	if(repairShip)
		repairShip.Ptr()->PhysicalUpdate(dt);
}
//---------------------------------------------------------------------------
//
inline struct IBaseObject * createPlatform (const REPAIRPLAT_INIT & data)
{
	RepairPlat * obj = new ObjectImpl<RepairPlat>;

	obj->FRAME_init(data);
	if (obj->initRepairPlat(data) == false)
	{
		delete obj;
		obj = 0;
	}

	return obj;
}
//------------------------------------------------------------------------------------------
//
REPAIRPLAT_INIT::~REPAIRPLAT_INIT (void)					// free archetype references
{
	if(repairDroneType)
		ARCHLIST->Release(repairDroneType, OBJREFNAME);
}
//------------------------------------------------------------------------------------------
//---------------------------RepairPlat Factory----------------------------------------------
//------------------------------------------------------------------------------------------
//
struct DACOM_NO_VTABLE RepairPlatFactory : public IObjectFactory
{
	U32 factoryHandle;		// handles to callback

	//
	// Interface mapping
	//

	BEGIN_DACOM_MAP_INBOUND(RepairPlatFactory)
	DACOM_INTERFACE_ENTRY(IObjectFactory)
	END_DACOM_MAP()

	RepairPlatFactory (void) { }

	~RepairPlatFactory (void);

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

	/* RepairPlatFactory methods */

	IDAComponent * getBase (void)
	{
		return static_cast<IObjectFactory *> (this);
	}
};
//--------------------------------------------------------------------------//
//
RepairPlatFactory::~RepairPlatFactory (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (OBJLIST && OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
		connection->Unadvise(factoryHandle);
}
//--------------------------------------------------------------------------//
//
void RepairPlatFactory::init (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
		connection->Advise(getBase(), &factoryHandle);
}
//-----------------------------------------------------------------------------
//
HANDLE RepairPlatFactory::CreateArchetype (const char *szArchname, OBJCLASS objClass, void *_data)
{
	REPAIRPLAT_INIT * result = 0;

	if (objClass == OC_PLATFORM)
	{
		BT_PLAT_REPAIR_DATA * data = (BT_PLAT_REPAIR_DATA *) _data;
		
		if (data->type == PC_REPAIRPLAT)
		{
			result = new REPAIRPLAT_INIT;

			if((result->repairDroneType = ARCHLIST->LoadArchetype(data->repairDroneType)) != 0)
				ARCHLIST->AddRef(result->repairDroneType, OBJREFNAME);

			if (!result->loadPlatformArchetype(data, ARCHLIST->GetArchetype(szArchname)))
			{
				goto Error;
			}
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
BOOL32 RepairPlatFactory::DestroyArchetype (HANDLE hArchetype)
{
	REPAIRPLAT_INIT * objtype = (REPAIRPLAT_INIT *)hArchetype;

	EditorStopObjectInsertion(objtype->pArchetype);
	delete objtype;

	return 1;
}
//-------------------------------------------------------------------
//
IBaseObject * RepairPlatFactory::CreateInstance (HANDLE hArchetype)
{
	REPAIRPLAT_INIT * objtype = (REPAIRPLAT_INIT *)hArchetype;
	return createPlatform(*objtype);
}
//-------------------------------------------------------------------
//
void RepairPlatFactory::EditorCreateInstance (HANDLE hArchetype, const MISSION_INFO & info)
{
	REPAIRPLAT_INIT * objtype = (REPAIRPLAT_INIT *)hArchetype;
	EditorStartObjectInsertion(objtype->pArchetype, info);
}
//---------------------------------------------------------------------------
//-------------------------GLOBAL STARTUP------------------------------------
//---------------------------------------------------------------------------
//
struct _repairplatfactory : GlobalComponent
{
	RepairPlatFactory * sfactory;

	virtual void Startup (void)
	{
		sfactory = new DAComponent<RepairPlatFactory>;
		AddToGlobalCleanupList((IDAComponent **) &sfactory);
	}

	virtual void Initialize (void)
	{
		sfactory->init();
	}
};

static _repairplatfactory __plat;

//--------------------------------------------------------------------------//
//----------------------------End RepairPlat.cpp-------------------------------//
//--------------------------------------------------------------------------//
