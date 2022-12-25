//--------------------------------------------------------------------------//
//                                                                          //
//                               RefinePlat.cpp                                //
//                                                                          //
//                  COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/RefinePlat.cpp 113   7/27/01 10:29a Tmauer $
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
#include "IHarvest.h"
#include "ObjSet.h"
#include "TFabricator.h"
#include "CommPacket.h"
#include "SoundManager.h"
#include "DMTechNode.h"

#include <DRefinePlat.h>
#include <DResearch.h>

#include <Physics.h>
#include <3DMath.h>
#include <FileSys.h>
#include <IConnection.h>
#include <TComponent.h>
#include <HeapObj.h>

#define HOST_CHECK THEMATRIX->IsMaster()

#define COMP_OP(operation) { U32 tempID = workingID; workingID = 0; THEMATRIX->OperationCompleted(tempID,operation);}

//P_OP_BUILD_FINISH
//P_OP_RES_BEGIN
//P_OP_RES_FINISH
//P_OP_STOP
//P_OP_BUILD_REMOVE_DELAY
#pragma pack(push,1)
struct BaseBuildCommand
{
	U8 command;
};

//P_OP_QUEUE_ADD
//P_OP_QUEUE_REMOVE
//P_OP_BUILD_BEGIN
//P_OP_BUILD_NO_ROOM
struct BuildCommandWHandle
{
	U8 command;
	U32 handle;
};

struct BuildCommandWIndex
{
	U8 command;
	U8 index;
};

struct BuildCommandWHandleIndex
{
	U8 command;
	U8 index;
	U32 handle;
};

#pragma pack(pop)

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//

struct _NO_VTABLE RefinePlat :TFabricator<Platform<REFINEPLAT_SAVELOAD, REFINEPLAT_INIT>
								>, ISpender, IHarvestBuilder,
											BASE_REFINEPLAT_SAVELOAD
{
	BEGIN_MAP_INBOUND(RefinePlat)
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
	_INTERFACE_ENTRY(IFabricator)
	_INTERFACE_ENTRY(IBuildQueue)
	_INTERFACE_ENTRY(ISpender)
	_INTERFACE_ENTRY(IShipDamage)
	_INTERFACE_ENTRY(IUpgrade)
	_INTERFACE_ENTRY(IHarvestBuilder)
	_INTERFACE_ENTRY(IEffectTarget)
	END_MAP()

	SaveNode		saveNode;
	LoadNode		loadNode;
	ResolveNode		resolveNode;
	UpdateNode		updateNode;
	PreTakeoverNode preTakeoverNode;
	UpgradeNode		upgradeNode;

	PhysUpdateNode  physUpdateNode;
	RenderNode		renderNode;
	PreDestructNode	destructNode;
	OnOpCancelNode	onOpCancelNode;
	ReceiveOpDataNode	receiveOpDataNode;
	GeneralSyncNode  genSyncNode;

	//
	// research data
	//
	BT_PLAT_REFINE_DATA * objData;

	SINGLE_TECHNODE currentResearch;

	SINGLE workTime;
	ResourceCost workingCost;
	U32 upgradeToLevel;

	HardpointInfo  shippointinfo;
	INSTANCE_INDEX shipPointIndex;

	HardpointInfo  dockpointinfo;
	INSTANCE_INDEX dockPointIndex;

	U32 freeShipArchID;	
	OBJPTR<IBaseObject> dockLocker;

	HARVEST_STANCE netHarvestStance;

	bool bDelayed;

	enum COMMUNICATION
	{
		P_OP_QUEUE_ADD,
		P_OP_QUEUE_REMOVE,
		P_OP_POP_QUEUE,

		P_OP_BUILD_BEGIN,
		P_OP_BUILD_FINISH,
		P_OP_BUILD_HALT,
		P_OP_BUILD_HALT_CLEARQ,

		P_OP_RES_BEGIN,
		P_OP_RES_FINISH,
		P_OP_ADMIRAL_FINISHED,

		P_OP_UPGRADE_BEGIN,
		P_OP_UPGRADE_FINISH,
		P_OP_UPGRADE_HALT,

		P_OP_DELAY,
		P_OP_REMOVE_DELAY,

		P_OP_STOP,

		P_OP_FREE_SHIP,
		P_OP_QUEUE_FAIL
	};

	

	RefinePlat (void);

	virtual ~RefinePlat (void);	// See ObjList.cpp

	virtual void SetReady(bool _bReady);

	// IFabricator methods

	virtual void LocalStartBuild();

	virtual void LocalEndBuild(bool killAgent = false);

	virtual void ParkYourself (const TRANSFORM & _transform, U32 planetID, U32 slotID);

	virtual void BuildQueue (U32 dwArchetypeDataID, COMMANDS command); 

	virtual void BeginConstruction (U32 planetID, U32 slotID, U32 dwArchetypeDataID, U32 agentID)
	{
		CQBOMB0("Platforms do not support this method");
	}

	virtual void BeginConstruction (GRIDVECTOR position, U32 dwArchetypeDataID, U32 agentID)
	{
		CQBOMB0("Platforms do not support this method");
	}

	virtual void BeginConstruction (IBaseObject * jumpgate, U32 dwArchetypeDataID, U32 agentID)
	{
		CQBOMB0("Platforms do not support this method");
	}

	virtual void BeginRepair (U32 agentID, IBaseObject * repairTarget)
	{
		CQBOMB0("Platforms do not support this method");
	};

	virtual void BeginDismantle(U32 agentID, IBaseObject * dismantleTarget)
	{
		CQBOMB0("Platforms do not support this method");
	};

	virtual SINGLE FabGetProgress(U32 & stallType);

	virtual SINGLE FabGetDisplayProgress(U32 & stallType);

	virtual bool IsBuildingAgent (U32 agentID)
	{
		CQBOMB0("Platforms do not support this method");
		return false;
	}

	virtual U8 GetFabTab()
	{return 0;};

	virtual void SetFabTab(U8 tab)
	{};

	virtual bool IsFabAtObj()
	{
		return true;
	}

	//ISpender

	virtual ResourceCost GetAmountNeeded();
	
	virtual void UnstallSpender();

	/* IExplosionOwner methods */

	virtual void OnChildDeath (INSTANCE_INDEX child)
	{
	}


	/* IMissionActor */

	// the following are process related
	// called in response to OpAgent::CreateOperation()
	virtual void OnOperationCreation (U32 agentID, void *buffer, U32 bufferSize);

	void receiveOperationData (U32 agentID, void *buffer, U32 bufferSize);

	void onOperationCancel (U32 agentID);
	
	virtual void OnStopRequest (U32 agentID);

	virtual void OnMasterChange (bool bIsMaster);

	// IHarvestBuilder methods
	virtual enum HARVEST_STANCE GetHarvestStance();

	virtual void SetHarvestStance(enum HARVEST_STANCE hStance);

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
		TRANSFORM trans;
		trans.set_orientation(dockpointinfo.orientation);
		trans.set_position(dockpointinfo.point);
		trans = transform.multiply(trans);
		return trans;
	}

	virtual void SetRallyPoint (const struct NETGRIDVECTOR & point);

	virtual bool GetRallyPoint (struct NETGRIDVECTOR & point);

	virtual bool IsDockLocked()
	{
		return (dockLockerID != 0);
	}

	virtual void LockDock (IBaseObject * locker)
	{
		if(THEMATRIX->IsMaster())
			CQASSERT(!dockLockerID);
		if(!dockLockerID)
		{
			locker->QueryInterface(IBaseObjectID, dockLocker, playerID);
			dockLockerID = locker->GetPartID();
		}
	}

	virtual void UnlockDock (IBaseObject * locker)
	{
		CQASSERT(dockLockerID);
		if(THEMATRIX->IsMaster())
			CQASSERT(locker->GetPartID() == dockLockerID);
		if(locker->GetPartID() == dockLockerID)
		{
			dockLocker = NULL;
			dockLockerID = 0;
		}
	}

	virtual void FreePlanetSlot (void)
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
	
	virtual U32 GetNumDocking ()
	{
		return numDocking;
	}

	virtual void IncNumDocking ()
	{
		++numDocking;
	}

	virtual void DecNumDocking ()
	{
		CQASSERT(numDocking);
		--numDocking;
	}

	virtual void AddHarvestRates(SINGLE & gas, SINGLE & metal, SINGLE & crew);

	/* IBaseObject */

	/*virtual void TestVisible (const USER_DEFAULTS & defaults, const U32 currentSystem, const U32 currentPlayer);

	virtual void Render (void);*/
	
	/* Platform methods */


	virtual const char * getSaveStructName (void) const		// must be overriden implemented by derived class
	{
		return "REFINEPLAT_SAVELOAD";
	}

	virtual void * getViewStruct (void)				// must be overriden implemented by derived class
	{
		return static_cast<BASE_REFINEPLAT_SAVELOAD *>(this);
	}

	/* BuildPlat methods */

	bool initRefinePlat (const REFINEPLAT_INIT & data);

	void save (REFINEPLAT_SAVELOAD & save);

	void load (REFINEPLAT_SAVELOAD & load);

	void resolve (void);

	BOOL32 update (void);

	void preTakeover (U32 newMissionID, U32 troopID);

	void physUpdateBuild (SINGLE dt);

	void render (void);

	void destruct (void);

	bool isAdmiral(U32 archID);

	void clearQueue();

	void upgradeRefine(const REFINEPLAT_INIT & data);

	U32 getGasUpgrade();

	U32 getMetalUpgrade();

	U32 getCrewUpgrade();

	U32 getSyncStance (void * buffer);
	void putSyncStance (void * buffer, U32 bufferSize, bool bLateDelivery);
};
//---------------------------------------------------------------------------
//
RefinePlat::RefinePlat (void) :
			saveNode(this, CASTSAVELOADPROC(&RefinePlat::save)),
			loadNode(this, CASTSAVELOADPROC(&RefinePlat::load)),
			resolveNode(this, ResolveProc(&RefinePlat::resolve)),
			updateNode(this, UpdateProc(&RefinePlat::update)),
			preTakeoverNode(this, PreTakeoverProc(&RefinePlat::preTakeover)),
			physUpdateNode(this, PhysUpdateProc(&RefinePlat::physUpdateBuild)),
			renderNode(this,RenderProc(&RefinePlat::render)),
			destructNode(this, PreDestructProc(&RefinePlat::destruct)),
			onOpCancelNode(this, OnOpCancelProc(&RefinePlat::onOperationCancel)),
			receiveOpDataNode(this, ReceiveOpDataProc(&RefinePlat::receiveOperationData)),
			upgradeNode(this,UpgradeProc(CASTINITPROC(&RefinePlat::upgradeRefine))),
			genSyncNode(this, SyncGetProc(&RefinePlat::getSyncStance), SyncPutProc(&RefinePlat::putSyncStance))
{
}
//---------------------------------------------------------------------------
//
RefinePlat::~RefinePlat (void)
{
}
//---------------------------------------------------------------------------
//
void RefinePlat::SetReady(bool _bReady)
{
	TFabricator<Platform<REFINEPLAT_SAVELOAD, REFINEPLAT_INIT>
								>::SetReady(_bReady);
}
//---------------------------------------------------------------------------
//
void RefinePlat::LocalStartBuild()
{
	fabStartAnims();
	return;
}
//---------------------------------------------------------------------------
//
void RefinePlat::LocalEndBuild(bool killAgent)
{
	if(killAgent)
	{
		if(THEMATRIX->IsMaster())
		{
			BaseBuildCommand buffer;
			buffer.command = P_OP_BUILD_HALT;
			THEMATRIX->SendOperationData(workingID,dwMissionID,&buffer,sizeof(buffer));

			if(buildee)
			{
				THEMATRIX->OperationCompleted(workingID,constructionID);
			}
			bBuilding = false;
			if(bDelayed)
			{
				BANKER->RemoveStalledSpender(playerID,this);
				bDelayed = false;
			}
			popFabQueue();
			COMP_OP(dwMissionID);
		}
		else
		{
			popFabQueue();
		}


/*		CQASSERT(workingID);
		OBJPTR<IBaseObject> obj;
		if(buildee)
		{
			buildee->QueryInterface(IBaseObjectID,obj);
			THEMATRIX->OperationCompleted(workingID,obj->GetPartID());	
		}
		popFabQueue();
		COMP_OP(dwMissionID);
*/	}
	return;
}

#define CreateQueueBuffer2(NAME, COMMAND,HANDLE,INDEX) \
		BuildCommandWHandleIndex NAME;\
		buffer.command = COMMAND;\
		buffer.handle = HANDLE;\
		buffer.index = INDEX;

#define CreateQueueBuffer1(NAME, COMMAND,INDEX) \
		BuildCommandWIndex NAME;\
		buffer.command = COMMAND;\
		buffer.index = INDEX;

#define NetworkFailSound(RESTYPE) \
	{\
		BuildCommandWHandle buffer;\
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
void RefinePlat::BuildQueue (U32 dwArchetypeDataID, COMMANDS command)
{
	if(HOST_CHECK)
	{
		switch(command)
		{
		case COMMANDS::ADDIFEMPTY:		// note: fix this!!!
			if (buildQueueEmpty() == false)
				break;
			//fall through intentional
		case COMMANDS::ADD:
			{
				if((!buildQueueFull()) && bReady)
				{
					if(!workingID)
					{
						BASIC_DATA * buildType = (BASIC_DATA *)(ARCHLIST->GetArchetypeData(dwArchetypeDataID));
						if(buildType->objClass == OC_RESEARCH)
						{
							BASE_RESEARCH_DATA * resType = (BASE_RESEARCH_DATA *)buildType;
							if(resType->type == RESEARCH_UPGRADE)
							{
								if(!queueHasUpgrade())
								{
									M_RESOURCE_TYPE failType;
									if(BANKER->SpendMoney(playerID,resType->cost,&failType))
									{
										if(BANKER->SpendCommandPoints(playerID,resType->cost))
										{
											CreateQueueBuffer2(buffer,P_OP_QUEUE_ADD,dwArchetypeDataID,getNextQueueIndex());
											workingID = THEMATRIX->CreateOperation(dwMissionID,&buffer,sizeof(buffer));
											COMP_OP(dwMissionID);
											addToQueue(dwArchetypeDataID);
										}
										else
										{
											BANKER->AddResource(playerID,resType->cost);
											NetworkFailSound(M_COMMANDPTS);
										}
									}
									else
										NetworkFailSound(failType);
								}
							}
							else //admiral or tech
							{
								BT_RESEARCH * btResData = (BT_RESEARCH *)resType;
								TECHNODE workTech = MGlobals::GetWorkingTechLevel(playerID);
								if((!isInQueue(dwArchetypeDataID)) && (!(workTech.HasTech(btResData->researchTech))))
								{
									M_RESOURCE_TYPE failType;
									if(BANKER->SpendMoney(playerID,resType->cost,&failType))
									{
										workTech.AddToNode(btResData->researchTech);
										MGlobals::SetWorkingTechLevel(workTech,playerID);
										CreateQueueBuffer2(buffer,P_OP_QUEUE_ADD,dwArchetypeDataID,getNextQueueIndex());
										workingID = THEMATRIX->CreateOperation(dwMissionID,&buffer,sizeof(buffer));
										COMP_OP(dwMissionID);
										addToQueue(dwArchetypeDataID);
									}
									else
										NetworkFailSound(failType);
								}
							}
						}
						else
						{
							M_RESOURCE_TYPE failType;
							BASE_SPACESHIP_DATA * shipData = (BASE_SPACESHIP_DATA * )buildType;
							if(BANKER->SpendMoney(playerID,shipData->missionData.resourceCost,&failType))
							{
								CreateQueueBuffer2(buffer,P_OP_QUEUE_ADD,dwArchetypeDataID,getNextQueueIndex());
								workingID = THEMATRIX->CreateOperation(dwMissionID,&buffer,sizeof(buffer));
								addToQueue(dwArchetypeDataID);
								COMP_OP(dwMissionID);
							}
							else
								NetworkFailSound(failType);
						}

					}else
					{
						BASIC_DATA * buildType = (BASIC_DATA *)(ARCHLIST->GetArchetypeData(dwArchetypeDataID));
						if(buildType->objClass == OC_RESEARCH)
						{
							BASE_RESEARCH_DATA * resType = (BASE_RESEARCH_DATA *)buildType;
							if(resType->type == RESEARCH_UPGRADE)
							{
								if(!queueHasUpgrade())
								{
									M_RESOURCE_TYPE failType;
									if(BANKER->SpendMoney(playerID,resType->cost,&failType))
									{
										if(BANKER->SpendCommandPoints(playerID,resType->cost))
										{
											CreateQueueBuffer2(buffer,P_OP_QUEUE_ADD,dwArchetypeDataID,getNextQueueIndex());
											THEMATRIX->SendOperationData(workingID,dwMissionID,&buffer,sizeof(buffer));
											addToQueue(dwArchetypeDataID);
										}
										else
										{
											BANKER->AddResource(playerID,resType->cost);
											NetworkFailSound(M_COMMANDPTS);
										}
									}
									else
										NetworkFailSound(failType);
								}
							}
							else //admiral or tech
							{
								BT_RESEARCH * btResData = (BT_RESEARCH *)resType;
								TECHNODE workTech = MGlobals::GetWorkingTechLevel(playerID);
								if((!isInQueue(dwArchetypeDataID)) && (!(workTech.HasTech(btResData->researchTech))))
								{
									M_RESOURCE_TYPE failType;
									if(BANKER->SpendMoney(playerID,resType->cost,&failType))
									{
										workTech.AddToNode(btResData->researchTech);
										MGlobals::SetWorkingTechLevel(workTech,playerID);
										CreateQueueBuffer2(buffer,P_OP_QUEUE_ADD,dwArchetypeDataID,getNextQueueIndex());
										THEMATRIX->SendOperationData(workingID,dwMissionID,&buffer,sizeof(buffer));
										addToQueue(dwArchetypeDataID);
									}
									else
										NetworkFailSound(failType);
								}
							}
						}
						else
						{
							M_RESOURCE_TYPE failType;
							BASE_SPACESHIP_DATA * shipData = (BASE_SPACESHIP_DATA * )buildType;
							if(BANKER->SpendMoney(playerID,shipData->missionData.resourceCost,&failType))
							{
								CreateQueueBuffer2(buffer,P_OP_QUEUE_ADD,dwArchetypeDataID,getNextQueueIndex());
								THEMATRIX->SendOperationData(workingID,dwMissionID,&buffer,sizeof(buffer));
								addToQueue(dwArchetypeDataID);
							}
							else
								NetworkFailSound(failType);
						}
					}
				}
				break;
			}
		case COMMANDS::REMOVE:
			{
				if(bReady)
				{
					U32 slotID = dwArchetypeDataID;//realy the slotID in disguise
					if(slotID == getFirstQueueIndex() && bBuilding && buildee && buildee->IsReversing())
						return;
					U32 realArchID = findQueueValue(slotID);
					if(realArchID != -1)
					{
						BASIC_DATA * buildType = (BASIC_DATA *)(ARCHLIST->GetArchetypeData(realArchID));
						if(buildType->objClass == OC_RESEARCH)
						{
							BASE_RESEARCH_DATA * resType = (BASE_RESEARCH_DATA *)buildType;
							BANKER->AddResource(playerID,resType->cost);
							if(resType->type == RESEARCH_TECH|| resType->type == RESEARCH_ADMIRAL)
							{
								BT_RESEARCH * btResData = (BT_RESEARCH *)resType;
								TECHNODE curTech;
								curTech = MGlobals::GetWorkingTechLevel(playerID);
								curTech.RemoveFromNode(btResData->researchTech);
								MGlobals::SetWorkingTechLevel(curTech,playerID);
							}
							else if(resType->type == RESEARCH_UPGRADE)
							{
								BANKER->FreeCommandPt(playerID,resType->cost.commandPt);
							}
						}
						else
						{
							BASE_SPACESHIP_DATA * shipData = (BASE_SPACESHIP_DATA * )buildType;
							BANKER->AddResource(playerID,shipData->missionData.resourceCost);
						}
						if(!workingID)
						{
							CreateQueueBuffer1(buffer,P_OP_QUEUE_REMOVE,slotID);
							workingID = THEMATRIX->CreateOperation(dwMissionID,&buffer,sizeof(buffer));
							removeIndexFromQueue(slotID);//should never delete the working project because we don't have a real agentID
							CQASSERT((!bResearching) && (!bBuilding) && (!bUpgrading));
							COMP_OP(dwMissionID);
						}else
						{
							U32 queuePos = getQueuePositionFromIndex(slotID);
							CreateQueueBuffer1(buffer,P_OP_QUEUE_REMOVE,slotID);
							THEMATRIX->SendOperationData(workingID,dwMissionID,&buffer,sizeof(buffer));
							if(bResearching)
							{
								removeIndexFromQueue(slotID);
								if(queuePos == 0)
								{
									TECHNODE curTech;
									curTech = MGlobals::GetWorkingTechLevel(playerID);
									curTech.RemoveFromNode(currentResearch);
									MGlobals::SetWorkingTechLevel(curTech,playerID);
									bResearching = false;
									if(bDelayed)
									{
										bDelayed = false;
										BANKER->RemoveStalledSpender(playerID,this);
									}
									COMP_OP(dwMissionID);
								}
							}
							else if(bBuilding)
							{
								if(queuePos == 0)
								{
									if(bDelayed)
									{
										bDelayed = false;
										BANKER->RemoveStalledSpender(playerID,this);
									}
									buildee->CancelBuild();
								}
								else
								{
									removeIndexFromQueue(slotID);
								}
							}
							else if(bUpgrading)
							{
								if(queuePos == 0)
								{
									CancelUpgrade();
									bUpgrading = false;
									removeIndexFromQueue(slotID);
									COMP_OP(dwMissionID);
									if(bDelayed)
									{
										bDelayed = false;
										BANKER->RemoveStalledSpender(playerID,this);
									}
								}
								else
								{
									removeIndexFromQueue(slotID);
								}
							}
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
}
//---------------------------------------------------------------------------
//
SINGLE RefinePlat::FabGetProgress(U32 & stallType)
{
	if(bDelayed)
	{
		stallType = IActiveButton::NO_MONEY;
		return 0;
	}
	else if(bResearching)
	{
		stallType = IActiveButton::NO_STALL;
		return (buidTimeSpent/ workTime);
	}
	else if(bUpgrading)
	{
		stallType = IActiveButton::NO_STALL;
		return (buidTimeSpent/ workTime);
	}
	else if(bBuilding)
	{
	 	return buildee->GetBuildProgress(stallType);
	}
	return 0.0;
}
//---------------------------------------------------------------------------
//
SINGLE RefinePlat::FabGetDisplayProgress(U32 & stallType)
{
	if(bDelayed)
	{
		stallType = IActiveButton::NO_MONEY;
		return 0.0;
	}
	else if(bResearching)
	{
		stallType = IActiveButton::NO_STALL;
		return (buidTimeSpent/ workTime);
	}
	else if(bUpgrading)
	{
		stallType = IActiveButton::NO_STALL;
		return (buidTimeSpent/ workTime);
	}
	else if(bBuilding)
	{
	 	return buildee->GetBuildDisplayProgress(stallType);
	}
	return 0.0;
}
//---------------------------------------------------------------------------
//
ResourceCost RefinePlat::GetAmountNeeded()
{
	return workingCost;
}
//---------------------------------------------------------------------------
//	
void RefinePlat::UnstallSpender()
{
	bDelayed = false;
	BaseBuildCommand buffer;
	buffer.command = P_OP_REMOVE_DELAY;
	if(workingID)
	{
		THEMATRIX->SendOperationData(workingID,dwMissionID,&buffer,sizeof(buffer));
	}
	else
	{
		workingID = THEMATRIX->CreateOperation(dwMissionID,&buffer,sizeof(buffer));
		COMP_OP(dwMissionID);
	}
}
//---------------------------------------------------------------------------
//
void RefinePlat::ParkYourself (const TRANSFORM & _transform, U32 planetID, U32 slotID)
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
	if(mObjClass == M_JUMPPLAT || mObjClass == M_HQ || mObjClass == M_COCOON || mObjClass == M_ACROPOLIS)
	{
		SECTOR->ComputeSupplyForAllPlayers();
	}
	rallyPoint.init(GetGridPosition(),systemID);
}
//---------------------------------------------------------------------------
//
TRANSFORM RefinePlat::GetShipTransform()
{
	TRANSFORM trans;
	trans.set_orientation(shippointinfo.orientation);
	trans.set_position(shippointinfo.point);
	trans = transform.multiply(trans);
	return trans;
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//

bool RefinePlat::initRefinePlat (const REFINEPLAT_INIT & data)
{
	objData = (BT_PLAT_REFINE_DATA *)(data.pData);
	dockLockerID = 0;
	bFreeShipMade = false;
	netHarvestStance = defaultHarvestStance = HS_NO_STANCE;

	if (objData->ship_hardpoint[0])
		FindHardpoint(objData->ship_hardpoint, shipPointIndex, shippointinfo, instanceIndex);
	if (objData->dock_hardpoint[0])
		FindHardpoint(objData->dock_hardpoint, dockPointIndex, dockpointinfo, instanceIndex);
	if (objData->harvesterArchetype[0])
		freeShipArchID = ARCHLIST->GetArchetypeDataID(objData->harvesterArchetype);
	else
		freeShipArchID = 0;

	gasHarvested = 0;
	metalHarvested = 0;
	crewHarvested = 0;

	rallyPoint.systemID = MAX_SYSTEMS+1;

	return true;
}
//---------------------------------------------------------------------------
//
void RefinePlat::save (REFINEPLAT_SAVELOAD & save)
{
	save.refinePlatSaveload = *static_cast<BASE_REFINEPLAT_SAVELOAD *>(this);
}
//---------------------------------------------------------------------------
//
void RefinePlat::load (REFINEPLAT_SAVELOAD & load)
{
 	*static_cast<BASE_REFINEPLAT_SAVELOAD *>(this) = load.refinePlatSaveload;
}
//---------------------------------------------------------------------------
//
void RefinePlat::resolve (void)
{
	if(bResearching && constructionID)
	{
		BASIC_DATA * buildType = (BASIC_DATA *)(ARCHLIST->GetArchetypeData(constructionID));
		CQASSERT(buildType->objClass == OC_RESEARCH);
		BT_RESEARCH * resData = (BT_RESEARCH *) buildType;
		workingCost = resData->cost;
		workTime = resData->time*(1.0-MGlobals::GetAIBonus(playerID));
		currentResearch = resData->researchTech;
	}
	if(bUpgrading && constructionID)
	{
		BASIC_DATA * buildType = (BASIC_DATA *)(ARCHLIST->GetArchetypeData(constructionID));
		CQASSERT(buildType->objClass == OC_RESEARCH);
		BT_UPGRADE * upgradeData = (BT_UPGRADE *) buildType;
		workingCost = upgradeData->cost;
		workTime = upgradeData->time*(1.0-MGlobals::GetAIBonus(playerID));
		upgradeToLevel = upgradeData->extensionID;
	}
	if(bBuilding)
	{
		IBaseObject * obj = OBJLIST->FindObject(constructionID);
		if(obj)
		{
			BASE_SPACESHIP_DATA * shipData = (BASE_SPACESHIP_DATA *)(ARCHLIST->GetArchetypeData(obj->pArchetype));
			workingCost = shipData->missionData.resourceCost;
		}
	}
	if(dockLockerID)
		OBJLIST->FindObject(dockLockerID, playerID, dockLocker, IBaseObjectID);
}
//---------------------------------------------------------------------------
//
void RefinePlat::preTakeover(U32 newMissionID, U32 troopID)
{
	if(HOST_CHECK)
	{
		if(bResearching && workingID)
		{
			BaseBuildCommand buffer;
			buffer.command = P_OP_STOP;
			THEMATRIX->SendOperationData(workingID,dwMissionID,&buffer,sizeof(buffer));

			TECHNODE curTech;
			curTech = MGlobals::GetWorkingTechLevel(playerID);
			curTech.RemoveFromNode(currentResearch);
			MGlobals::SetWorkingTechLevel(curTech,playerID);
			bResearching = false;	
			COMP_OP(dwMissionID);
		} 
		else if(bBuilding && workingID)
		{
			BaseBuildCommand buffer;
			buffer.command = P_OP_BUILD_HALT_CLEARQ;
			THEMATRIX->SendOperationData(workingID,dwMissionID,&buffer,sizeof(buffer));

			if(buildee)
			{
				THEMATRIX->OperationCompleted(workingID,constructionID);
				FabHaltBuild();
				THEMATRIX->ObjectTerminated(constructionID,0);
			}
			bBuilding = false;
			if(bDelayed)
			{
				BANKER->RemoveStalledSpender(playerID,this);
				bDelayed = false;
			}
			COMP_OP(dwMissionID);
		}
		else if(bUpgrading && workingID)
		{
			BaseBuildCommand buffer;
			buffer.command = P_OP_UPGRADE_HALT;
			THEMATRIX->SendOperationData(workingID,dwMissionID,&buffer,sizeof(buffer));

			CancelUpgrade();
			BANKER->FreeCommandPt(playerID,workingCost.commandPt);
			bUpgrading = false;

			COMP_OP(dwMissionID);
		}
	}
	CQASSERT(!workingID);
	clearQueue();
	if(dockLocker)
	{
		OBJPTR<IHarvest> harvester;
		dockLocker->QueryInterface(IHarvestID,harvester);
		if(harvester)
		{
			harvester->DockTaken();
		}
	}
}
//---------------------------------------------------------------------------
//
void RefinePlat::upgradeRefine(const REFINEPLAT_INIT & data)
{
	OBJPTR<IPlanet> planet;
	if(buildPlanetID)
	{
		OBJLIST->FindObject(buildPlanetID,NONSYSVOLATILEPTR,planet,IPlanetID);
		if(planet)
		{
//			planet->ChangePlayerRates(playerID,-objData->metalRate,-objData->gasRate,-objData->crewRate);
		}
	}
	objData = (BT_PLAT_REFINE_DATA *)(data.pData);
	if(planet)
//		planet->ChangePlayerRates(playerID,objData->metalRate,objData->gasRate,objData->crewRate);

	if (objData->ship_hardpoint[0])
		FindHardpoint(objData->ship_hardpoint, shipPointIndex, shippointinfo, instanceIndex);
	if (objData->dock_hardpoint[0])
		FindHardpoint(objData->dock_hardpoint, dockPointIndex, dockpointinfo, instanceIndex);
}
//---------------------------------------------------------------------------
//
void RefinePlat::SetRallyPoint (const struct NETGRIDVECTOR & point)
{
	rallyPoint = point;
}
//---------------------------------------------------------------------------
//
bool RefinePlat::GetRallyPoint (struct NETGRIDVECTOR & point)
{
//	CQASSERT(rallyPoint.systemID != MAX_SYSTEMS+1);

	NETGRIDVECTOR testGrid;
	testGrid.init(GetGridPosition(), systemID);
	
	if (rallyPoint == testGrid || (rallyPoint.systemID == MAX_SYSTEMS+1))
	{
		// we haven't set a rally point yet
		return false;
	}

	point = rallyPoint;
	return true;
}
//---------------------------------------------------------------------------
//
void RefinePlat::AddHarvestRates(SINGLE & gas, SINGLE & metal, SINGLE & crew)
{
	gas += objData->gasRate[getGasUpgrade()];
	metal += objData->metalRate[getMetalUpgrade()];
	crew += objData->crewRate[getCrewUpgrade()];
}

//---------------------------------------------------------------------------
//
/*void RefinePlat::TestVisible (const USER_DEFAULTS & defaults, const U32 currentSystem, const U32 currentPlayer)
{
	Platform<REFINEPLAT_SAVELOAD, REFINEPLAT_INIT>::TestVisible(defaults, currentSystem, currentPlayer);

	for(U32 count = 0; count < numDrones; ++count)
	{
		if(drone[count])
			drone[count].ptr->TestVisible(defaults, currentSystem, currentPlayer);
	}
}
//-------------------------------------------------------------------
//
void RefinePlat::Render (void)
{
	Platform<REFINEPLAT_SAVELOAD, REFINEPLAT_INIT>::Render();
	if(bReady)
	{
		for (U8 c=0;c<numDrones;c++)
		{
			if(drone[c])
				drone[c].ptr->Render();
		}
	}
}*/
//-------------------------------------------------------------------
//
bool RefinePlat::isAdmiral (U32 archID)
{
	BASE_SPACESHIP_DATA	* data  = (BASE_SPACESHIP_DATA	*) ARCHLIST->GetArchetypeData(archID);

	return (data->objClass == OC_SPACESHIP && data->type == SSC_FLAGSHIP);
}
//---------------------------------------------------------------------------
//
void RefinePlat::clearQueue()
{
	bool working = true;
	while(queueSize)
	{
		U32 archID = peekFabQueue();
		BASIC_DATA * buildType = (BASIC_DATA *)(ARCHLIST->GetArchetypeData(archID));
		if(buildType->objClass == OC_RESEARCH)
		{
			BASE_RESEARCH_DATA * resType = (BASE_RESEARCH_DATA *)buildType;
			if(resType->type == RESEARCH_TECH || resType->type == RESEARCH_ADMIRAL)
			{
				BT_RESEARCH * btResData = (BT_RESEARCH *)resType;
				TECHNODE workTech = MGlobals::GetWorkingTechLevel(playerID);
				workTech.RemoveFromNode(btResData->researchTech);
				MGlobals::SetWorkingTechLevel(workTech,playerID);
			}
			if(THEMATRIX->IsMaster()&& (!working))
				BANKER->AddResource(playerID,resType->cost);
		}
		else if(buildType->objClass == OC_SPACESHIP)
		{
			BASE_SPACESHIP_DATA * shipData = (BASE_SPACESHIP_DATA *)buildType;
			if(THEMATRIX->IsMaster() && (!working))
				BANKER->AddResource(playerID,shipData->missionData.resourceCost);
		}

		popFabQueue();
		working = false;
	}
}
//---------------------------------------------------------------------------
//
U32 RefinePlat::getGasUpgrade()
{
	TECHNODE node = MGlobals::GetCurrentTechLevel(playerID);
	if(node.HasTech(race,0,TECHTREE::RES_REFINERY_GAS3,0,0,0,0))
		return 3;
	else if(node.HasTech(race,0,TECHTREE::RES_REFINERY_GAS2,0,0,0,0))
		return 2;
	else if(node.HasTech(race,0,TECHTREE::RES_REFINERY_GAS1,0,0,0,0))
		return 1;
	return 0;
}
//---------------------------------------------------------------------------
//
U32 RefinePlat::getMetalUpgrade()
{
	TECHNODE  node = MGlobals::GetCurrentTechLevel(playerID);
	if(node.HasTech(race,0,TECHTREE::RES_REFINERY_METAL3,0,0,0,0))
		return 3;
	else if(node.HasTech(race,0,TECHTREE::RES_REFINERY_METAL2,0,0,0,0))
		return 2;
	else if(node.HasTech(race,0,TECHTREE::RES_REFINERY_METAL1,0,0,0,0))
		return 1;
	return 0;
}
//---------------------------------------------------------------------------
//
U32 RefinePlat::getCrewUpgrade()
{
	TECHNODE  node = MGlobals::GetCurrentTechLevel(playerID);
	if(node.HasTech(race,0,TECHTREE::RES_REFINERY_CREW3,0,0,0,0))
		return 3;
	else if(node.HasTech(race,0,TECHTREE::RES_REFINERY_CREW2,0,0,0,0))
		return 2;
	else if(node.HasTech(race,0,TECHTREE::RES_REFINERY_CREW1,0,0,0,0))
		return 1;
	return 0;
}
//---------------------------------------------------------------------------
//
U32 RefinePlat::getSyncStance (void * buffer)
{
	U32 result = 0;
	if (netHarvestStance != defaultHarvestStance)
	{
		((U8*)buffer)[result++] = defaultHarvestStance;
		netHarvestStance = defaultHarvestStance;
	}
	return result;
}
//---------------------------------------------------------------------------
//
void RefinePlat::putSyncStance (void * buffer, U32 bufferSize, bool bLateDelivery)
{
	U8 stance = ((U8*)buffer)[--bufferSize];
	netHarvestStance = defaultHarvestStance = HARVEST_STANCE(stance);
}
//---------------------------------------------------------------------------
//
BOOL32 RefinePlat::update (void)
{
	if((!HOST_CHECK) && (!bExploding) && bReady)
	{
		//client approximate harvesting code
		if(buildPlanetID)
		{
			OBJPTR<IPlanet> planet;
			OBJLIST->FindObject(buildPlanetID)->QueryInterface(IPlanetID,planet);
			gasHarvested += ELAPSED_TIME*objData->gasRate[getGasUpgrade()]*(1.0+MGlobals::GetAIBonus(playerID));
			U32 newHarvest = gasHarvested;
			if(newHarvest)
			{
				if(newHarvest > planet->GetGas())
					newHarvest = planet->GetGas();

				gasHarvested -= newHarvest;
				MGlobals::SetGasGained(playerID,MGlobals::GetGasGained(playerID)+newHarvest);
			}

			metalHarvested += ELAPSED_TIME*objData->metalRate[getMetalUpgrade()]*(1.0+MGlobals::GetAIBonus(playerID));
			newHarvest = metalHarvested;
			if(newHarvest)
			{
				if(newHarvest > planet->GetMetal())
					newHarvest = planet->GetMetal();
				metalHarvested -= newHarvest;
				MGlobals::SetMetalGained(playerID,MGlobals::GetMetalGained(playerID)+newHarvest);
			}
			
			crewHarvested += ELAPSED_TIME*objData->crewRate[getCrewUpgrade()]*(1.0+MGlobals::GetAIBonus(playerID));
			newHarvest = crewHarvested;
			if(newHarvest)
			{
				if(newHarvest > planet->GetCrew())
					newHarvest = planet->GetCrew();
				crewHarvested -= newHarvest;
				MGlobals::SetCrewGained(playerID,MGlobals::GetCrewGained(playerID)+newHarvest);
			}
		}
	}

	if(HOST_CHECK && (!bExploding) && bReady)
	{
		//harvesting code
		if(buildPlanetID)
		{
			OBJPTR<IPlanet> planet;
			OBJLIST->FindObject(buildPlanetID)->QueryInterface(IPlanetID,planet);
			gasHarvested += ELAPSED_TIME*objData->gasRate[getGasUpgrade()]*(1.0+MGlobals::GetAIBonus(playerID));
			U32 newHarvest = gasHarvested;
			if(newHarvest)
			{
				if(newHarvest > planet->GetGas())
					newHarvest = planet->GetGas();

				gasHarvested -= newHarvest;
				planet->SetGas(planet->GetGas()-newHarvest);
				BANKER->AddGas(playerID,newHarvest);
				MGlobals::SetGasGained(playerID,MGlobals::GetGasGained(playerID)+newHarvest);
			}

			metalHarvested += ELAPSED_TIME*objData->metalRate[getMetalUpgrade()]*(1.0+MGlobals::GetAIBonus(playerID));
			newHarvest = metalHarvested;
			if(newHarvest)
			{
				if(newHarvest > planet->GetMetal())
					newHarvest = planet->GetMetal();
				planet->SetMetal(planet->GetMetal()-newHarvest);
				metalHarvested -= newHarvest;
				BANKER->AddMetal(playerID,newHarvest);
				MGlobals::SetMetalGained(playerID,MGlobals::GetMetalGained(playerID)+newHarvest);
			}
			
			crewHarvested += ELAPSED_TIME*objData->crewRate[getCrewUpgrade()]*(1.0+MGlobals::GetAIBonus(playerID));
			newHarvest = crewHarvested;
			if(newHarvest)
			{
				if(newHarvest > planet->GetCrew())
					newHarvest = planet->GetCrew();
				planet->SetCrew(planet->GetCrew()-newHarvest);
				crewHarvested -= newHarvest;
				BANKER->AddCrew(playerID,newHarvest);
				MGlobals::SetCrewGained(playerID,MGlobals::GetCrewGained(playerID)+newHarvest);
			}

		}

		//building code
		if(bBuilding && workingID)
		{
			U32 stallType;
			if((FabGetProgress(stallType) >= 1.0) )
			{
				if((buildee && buildee->IsReversing()) || ((!bDelayed) && BANKER->SpendCommandPoints(playerID,workingCost)))
				{
					BaseBuildCommand buffer;
					buffer.command = P_OP_BUILD_FINISH;
					THEMATRIX->SendOperationData(workingID,dwMissionID,&buffer,sizeof(buffer));
					bBuilding = false;

					bool bOrderAutoHarvest = false;

					OBJPTR<IBaseObject> baseBuild;
					VOLPTR(IHarvest) harvest = buildee;
					if(buildee)
					{
						if(!buildee->IsReversing())
						{
							MGlobals::SetNumUnitsBuilt(playerID,MGlobals::GetNumUnitsBuilt(playerID)+1);
							MPartNC part(buildee.Ptr());
							part->bUnderCommand = true;
							if(defaultHarvestStance != HS_NO_STANCE)
							{
								if(harvest)
								{
									bOrderAutoHarvest = true;
								}
							}
						}

						buildee->QueryInterface(IBaseObjectID,baseBuild);
						THEMATRIX->OperationCompleted(workingID,baseBuild->GetPartID());	
					}
					FabEnableCompletion();
					popFabQueue();
					COMP_OP(dwMissionID);
					if(baseBuild && (rallyPoint.systemID != MAX_SYSTEMS+1))
					{
						USR_PACKET<USRMOVE> packet;

						packet.objectID[0] = baseBuild->GetPartID();
						packet.position = rallyPoint;
						packet.init(1);
				
						NETPACKET->Send(HOSTID,0,&packet);
					}
					if(harvest && bOrderAutoHarvest)
					{
						USR_PACKET<HARVEST_AUTOMODE_PACKET> packet;
						packet.objectID[0] = harvest.Ptr()->GetPartID();
						if(defaultHarvestStance == HS_GAS_HARVEST)
							packet.nuggetType = M_GAS_NUGGETS;
						else
							packet.nuggetType = M_METAL_NUGGET;
						packet.userBits = 1;//delay the packet till after the move.
						packet.init(1);
						NETPACKET->Send(HOSTID,0,&packet);
					}
				}
				else if(!bDelayed)
				{
					bDelayed = true;
					BANKER->AddStalledSpender(playerID,this);

					BaseBuildCommand buffer;
					buffer.command = P_OP_DELAY;
					THEMATRIX->SendOperationData(workingID,dwMissionID,&buffer,sizeof(buffer));
					failSound(M_COMMANDPTS);
				}
			}
		}
		else if(bResearching && workingID)
		{
			if(SECTOR->SystemInSupply(systemID,playerID))
			{
				buidTimeSpent += ELAPSED_TIME*effectFlags.getResearchBoost();

				if(buidTimeSpent >= workTime)
				{
					BASE_RESEARCH_DATA * buildType = (BASE_RESEARCH_DATA *)(ARCHLIST->GetArchetypeData(constructionID));
					if(buildType->type == RESEARCH_TECH)
					{
						BaseBuildCommand buffer;
						buffer.command = P_OP_RES_FINISH;
						THEMATRIX->SendOperationData(workingID,dwMissionID,&buffer,sizeof(buffer));

						BT_RESEARCH * resData = (BT_RESEARCH *)buildType;
						COMM_RESEARCH_COMPLETED(((char *)(resData->resFinishedSound)),resData->resFinishSubtitle,pInitData->displayName);

						TECHNODE curTech = MGlobals::GetCurrentTechLevel(playerID);
						curTech.AddToNode(currentResearch);
						MGlobals::SetCurrentTechLevel(curTech,playerID);
						curTech = MGlobals::GetWorkingTechLevel(playerID);
						curTech.RemoveFromNode(currentResearch);
						MGlobals::SetWorkingTechLevel(curTech,playerID);
						popFabQueue();
						constructionID = 0;
						bResearching = false;				
						COMP_OP(dwMissionID);
					}
					else//admiral
					{
						BuildCommandWHandle buffer;
						buffer.command = P_OP_ADMIRAL_FINISHED;
						buffer.handle = MGlobals::CreateNewPartID(playerID) | ADMIRAL_MASK;

						THEMATRIX->AddObjectToOperation(workingID,buffer.handle);
						THEMATRIX->SendOperationData(workingID,dwMissionID,&buffer,sizeof(buffer));
						THEMATRIX->OperationCompleted(workingID,buffer.handle);

						// add the admiral creation to the scoreboard
						MGlobals::SetNumAdmiralsBuilt(playerID, MGlobals::GetNumAdmiralsBuilt(playerID)+1);

						TECHNODE curTech = MGlobals::GetCurrentTechLevel(playerID);
						curTech.AddToNode(currentResearch);
						MGlobals::SetCurrentTechLevel(curTech,playerID);
						curTech = MGlobals::GetWorkingTechLevel(playerID);
						curTech.RemoveFromNode(currentResearch);
						MGlobals::SetWorkingTechLevel(curTech,playerID);
						popFabQueue();
						constructionID = 0;
						bResearching = false;				

						IBaseObject *admiral;
						BT_ADMIRAL_RES * admiralRes = (BT_ADMIRAL_RES *) buildType;
						admiral = MGlobals::CreateInstance(ARCHLIST->LoadArchetype(admiralRes->flagshipType),buffer.handle);
						OBJLIST->AddObject(admiral);

						TRANSFORM trans= GetTransform();
						trans.set_position(trans.get_position()-Vector(0,0,100));
							
						VOLPTR(IPhysicalObject) miss;
						if ((miss = admiral) != 0)
						{
							miss->SetTransform(trans, systemID);
							ENGINE->update_instance(admiral->GetObjectIndex(),0,0);
						}

						MPartNC part(admiral);
						admiral->SetReady(true);
						part->hullPoints = part->hullPointsMax;
						part->supplies = 0;

						SPACESHIP_ALERT(admiral, admiral->GetPartID(), constructComplete,SUB_CONSTRUCTION_COMP, part.pInit->displayName,ALERT_UNIT_BUILT);

						USR_PACKET<USRMOVE> packet;

						packet.objectID[0] = admiral->GetPartID();
						if(rallyPoint.systemID ==17)
						{
							NETGRIDVECTOR ngv;
							ngv.init(GetGridPosition(),systemID);
							packet.position = ngv;
						}
						else
							packet.position = rallyPoint;
						packet.init(1);
						NETPACKET->Send(HOSTID,0,&packet);

						MScript::RunProgramsWithEvent(CQPROGFLAG_OBJECTCONSTRUCTED, admiral->GetPartID());

						COMP_OP(dwMissionID);
					}
				}
			}
		}
		else if(bUpgrading && workingID )
		{
			if(SECTOR->SystemInSupply(systemID,playerID))
			{
				buidTimeSpent += ELAPSED_TIME;
				if(buidTimeSpent >= workTime)
				{
					BT_UPGRADE * resData = (BT_UPGRADE *)(ARCHLIST->GetArchetypeData(constructionID));
					COMM_MUTATION_COMPLETED(((char *)(resData->resFinishedSound)),resData->resFinishSubtitle,pInitData->displayName);

					SetUpgrade(upgradeToLevel);
					popFabQueue();
					
					bUpgrading = false;

					constructionID = 0;

					BaseBuildCommand buffer;
					buffer.command = P_OP_UPGRADE_FINISH;
					THEMATRIX->SendOperationData(workingID,dwMissionID,&buffer,sizeof(buffer));
					COMP_OP(dwMissionID);	
				}
				if(buidTimeSpent >= 0 && buidTimeSpent <= workTime)
					SetUpgradePercent(buidTimeSpent/workTime);			
			}
		}
		else if(queueSize && (!bDelayed) && (!bBuilding) && (!bResearching) && (!bUpgrading) && (!workingID))
		{
			U32 buildArchID = peekFabQueue();
			BASIC_DATA * buildType = (BASIC_DATA *)(ARCHLIST->GetArchetypeData(buildArchID));
			if(buildType)
			{
				if(buildType->objClass == OC_RESEARCH)//is it research
				{
					BASE_RESEARCH_DATA * bResData = (BASE_RESEARCH_DATA *) buildType;
					if(bResData->type == RESEARCH_TECH|| bResData->type == RESEARCH_ADMIRAL)
					{
						BT_RESEARCH * resData = (BT_RESEARCH *) buildType;
						currentResearch = resData->researchTech;
						
						TECHNODE haveTech = MGlobals::GetCurrentTechLevel(playerID);
						if((!(haveTech.HasSomeTech(currentResearch))) )//I am not working on this tech and I don't have this tech
						{
							workingCost = resData->cost;
							bResearching = true;
							
							BaseBuildCommand buffer;
							buffer.command = P_OP_RES_BEGIN;

							workingID = THEMATRIX->CreateOperation(dwMissionID,&buffer,sizeof(buffer));

							constructionID = buildArchID;
							workTime = resData->time*(1.0-MGlobals::GetAIBonus(playerID));
							buidTimeSpent = 0;
						}
						else
						{
							BaseBuildCommand buffer;
							buffer.command = P_OP_POP_QUEUE;
							workingID = THEMATRIX->CreateOperation(dwMissionID,&buffer,sizeof(buffer));
							COMP_OP(dwMissionID);
							popFabQueue();
						}
					}
					else if(bResData->type == RESEARCH_UPGRADE)
					{
						BT_UPGRADE * upData = (BT_UPGRADE *)buildType;
						workingCost = upData->cost;

						bUpgrading = true;

						workTime = upData->time*(1.0-MGlobals::GetAIBonus(playerID));
						upgradeToLevel = upData->extensionID;
						constructionID = buildArchID;

						StartUpgrade(upgradeToLevel,upData->time*(1.0-MGlobals::GetAIBonus(playerID)));
						SetUpgradePercent(0.0);

						BaseBuildCommand buffer;
						buffer.command = P_OP_UPGRADE_BEGIN;
						buidTimeSpent = 0;

						workingID = THEMATRIX->CreateOperation(dwMissionID,&buffer,sizeof(buffer));
					}
				}
				else  //or is it a space ship
				{
					BASE_SPACESHIP_DATA * shipData =  (BASE_SPACESHIP_DATA *)buildType;
					workingCost = shipData->missionData.resourceCost;
					BuildCommandWHandle buffer;
					buffer.command = P_OP_BUILD_BEGIN;
					constructionID = MGlobals::CreateNewPartID(GetPlayerID());

					if(isAdmiral(buildArchID))
						constructionID |= ADMIRAL_MASK;

					buffer.handle = constructionID;

					ObjSet set;
					set.numObjects = 2;
					set.objectIDs[0] = dwMissionID;//my ID
					set.objectIDs[1] = buffer.handle; //ship ID
					workingID = THEMATRIX->CreateOperation(set,&buffer,sizeof(buffer));

					THEMATRIX->SetCancelState(workingID,false);
					IBaseObject *buildee;
					buildee = MGlobals::CreateInstance(ARCHLIST->LoadArchetype(buildArchID),buffer.handle);
					OBJLIST->AddObject(buildee);
					OBJPTR<IBuild> build;
					if (buildee->QueryInterface(IBuildID,build))
					{
						TRANSFORM trans= GetShipTransform();
						
						VOLPTR(IPhysicalObject) miss;
						if ((miss = buildee) != 0)
						{
							miss->SetTransform(trans, systemID);
							ENGINE->update_instance(buildee->GetObjectIndex(),0,0);
						}
						FabStartBuild(build);
					}		
				}
			}
			else
			{
				BaseBuildCommand buffer;
				buffer.command = P_OP_POP_QUEUE;
				workingID = THEMATRIX->CreateOperation(dwMissionID,&buffer,sizeof(buffer));
				COMP_OP(dwMissionID);
				popFabQueue();
			}
		}
	}
	else
	{
		if(!bExploding && bReady)
		{
			//harvesting code
			if(buildPlanetID)
			{
				OBJPTR<IPlanet> planet;
				OBJLIST->FindObject(buildPlanetID)->QueryInterface(IPlanetID,planet);
				gasHarvested += ELAPSED_TIME*objData->gasRate[getGasUpgrade()];
				U32 newHarvest = gasHarvested;
				if(newHarvest)
				{
					if(newHarvest > planet->GetGas())
						newHarvest = planet->GetGas();

					gasHarvested -= newHarvest;
				}

				metalHarvested += ELAPSED_TIME*objData->metalRate[getMetalUpgrade()];
				newHarvest = metalHarvested;
				if(newHarvest)
				{
					if(newHarvest > planet->GetMetal())
						newHarvest = planet->GetMetal();
					metalHarvested -= newHarvest;
				}
				
				crewHarvested += ELAPSED_TIME*objData->crewRate[getCrewUpgrade()];
				newHarvest = crewHarvested;
				if(newHarvest)
				{
					if(newHarvest > planet->GetCrew())
						newHarvest = planet->GetCrew();
					crewHarvested -= newHarvest;
				}
			}
		}
		if(SECTOR->SystemInSupply(systemID,playerID))
		{
			if((bUpgrading || bResearching) && workingID && bReady)
			{
				if(bResearching)
					buidTimeSpent += ELAPSED_TIME*effectFlags.getResearchBoost();
				else
					buidTimeSpent += ELAPSED_TIME;
			}
		}
	}

	if(bReady && !bFreeShipMade && HOST_CHECK && MGlobals::IsUpdateFrame(dwMissionID) && !workingID)
	{
		bFreeShipMade = true;
		if(freeShipArchID)
		{
			BuildCommandWHandle buffer;
			buffer.command = P_OP_FREE_SHIP;
			buffer.handle = MGlobals::CreateNewPartID(playerID);
			ObjSet set;
			set.numObjects = 2;
			set.objectIDs[0] = dwMissionID;
			set.objectIDs[1] = buffer.handle;
			U32 workingID = THEMATRIX->CreateOperation(set,&buffer,sizeof(buffer));
			THEMATRIX->OperationCompleted(workingID,buffer.handle);
			THEMATRIX->OperationCompleted(workingID,dwMissionID);

			IBaseObject *freeShip;
			freeShip = MGlobals::CreateInstance(ARCHLIST->LoadArchetype(freeShipArchID),buffer.handle);
			OBJLIST->AddObject(freeShip);

			TRANSFORM trans= GetTransform();
			trans.set_position(trans.get_position()-Vector(0,0,100));
				
			VOLPTR(IPhysicalObject) miss;
			if ((miss = freeShip) != 0)
			{
				miss->SetTransform(trans, systemID);
				ENGINE->update_instance(freeShip->GetObjectIndex(),0,0);
			}

			MPartNC part(freeShip);
			freeShip->SetReady(true);
			part->hullPoints = part->hullPointsMax;
			part->supplies = 0;

			USR_PACKET<USRMOVE> packet;

			packet.objectID[0] = freeShip->GetPartID();
			packet.position.init(GetGridPosition(),systemID);
			packet.init(1);
			NETPACKET->Send(HOSTID,0,&packet);

			MScript::RunProgramsWithEvent(CQPROGFLAG_OBJECTCONSTRUCTED, freeShip->GetPartID());
		}
	}
	return true;
}
//---------------------------------------------------------------------------
//
void RefinePlat::physUpdateBuild (SINGLE dt)
{
}
//---------------------------------------------------------------------------
//
void RefinePlat::render (void)
{
}
//---------------------------------------------------------------------------
//
void RefinePlat::destruct (void)
{
	if(HOST_CHECK)
	{
		if(bResearching && workingID)
		{
			BaseBuildCommand buffer;
			buffer.command = P_OP_STOP;
			THEMATRIX->SendOperationData(workingID,dwMissionID,&buffer,sizeof(buffer));

			TECHNODE curTech;
			curTech = MGlobals::GetWorkingTechLevel(playerID);
			curTech.RemoveFromNode(currentResearch);
			MGlobals::SetWorkingTechLevel(curTech,playerID);
			bResearching = false;	
			COMP_OP(dwMissionID);
		} 
		else if(bBuilding && workingID)
		{
			BaseBuildCommand buffer;
			buffer.command = P_OP_BUILD_HALT_CLEARQ;
			THEMATRIX->SendOperationData(workingID,dwMissionID,&buffer,sizeof(buffer));

			if(buildee)
			{
				THEMATRIX->OperationCompleted(workingID,constructionID);
				FabHaltBuild();
				THEMATRIX->ObjectTerminated(constructionID,0);
			}
			bBuilding = false;
			if(bDelayed)
			{
				BANKER->RemoveStalledSpender(playerID,this);
				bDelayed = false;
			}
			COMP_OP(dwMissionID);
		}
		else if(bUpgrading && workingID)
		{
			BaseBuildCommand buffer;
			buffer.command = P_OP_UPGRADE_HALT;
			THEMATRIX->SendOperationData(workingID,dwMissionID,&buffer,sizeof(buffer));

			CancelUpgrade();
			BANKER->FreeCommandPt(playerID,workingCost.commandPt);
			bUpgrading = false;

			COMP_OP(dwMissionID);
		}
	}
	CQASSERT(!workingID);
	clearQueue();
}
//---------------------------------------------------------------------------
//
void RefinePlat::OnOperationCreation (U32 agentID, void *buffer, U32 bufferSize)
{
	BaseBuildCommand * buf = (BaseBuildCommand *)buffer;
	switch(buf->command)
	{
	case P_OP_POP_QUEUE:
		{
			popFabQueue();
			THEMATRIX->OperationCompleted(agentID,dwMissionID);
			break;
		}
	case P_OP_QUEUE_FAIL:
		{
			BuildCommandWHandle * com = (BuildCommandWHandle *)buf;
			failSound((M_RESOURCE_TYPE)(com->handle));
			THEMATRIX->OperationCompleted(agentID,dwMissionID);
			break;
		}
	case P_OP_DELAY:
		{
			failSound(M_COMMANDPTS);
			bDelayed = true;
			BANKER->AddStalledSpender(playerID,this);
			THEMATRIX->OperationCompleted(agentID,dwMissionID);
			break;
		}
	case P_OP_REMOVE_DELAY:
		{
			bDelayed = false;
			BANKER->RemoveStalledSpender(playerID,this);
			THEMATRIX->OperationCompleted(agentID,dwMissionID);
			break;
		}
	case P_OP_UPGRADE_BEGIN:
		{
			U32 buildArchID = peekFabQueue();
			BASIC_DATA * buildType = (BASIC_DATA *)(ARCHLIST->GetArchetypeData(buildArchID));
			
			CQASSERT(buildType->objClass == OC_RESEARCH);

			BT_UPGRADE * upData = (BT_UPGRADE *)buildType;
			CQASSERT(upData->type == RESEARCH_UPGRADE);

			bUpgrading = true;

			workingCost = upData->cost;
			workTime = upData->time*(1.0-MGlobals::GetAIBonus(playerID));
			upgradeToLevel = upData->extensionID;
			constructionID = buildArchID;
			buidTimeSpent = 0;

			StartUpgrade(upgradeToLevel,upData->time*(1.0-MGlobals::GetAIBonus(playerID)));
			SetUpgradePercent(0.0);

			workingID = agentID;
			break;
		}
	case P_OP_QUEUE_ADD:
		{
			BuildCommandWHandleIndex * myBuf = (BuildCommandWHandleIndex *) buffer;
			addToQueue(myBuf->handle,myBuf->index);
			BASIC_DATA * buildType = (BASIC_DATA *)(ARCHLIST->GetArchetypeData(myBuf->handle));
			if(buildType->objClass == OC_RESEARCH)
			{
				BASE_RESEARCH_DATA * resType = (BASE_RESEARCH_DATA *)buildType;
				if(resType->type == RESEARCH_TECH|| resType->type == RESEARCH_ADMIRAL)
				{
					BT_RESEARCH * btResData = (BT_RESEARCH *)resType;
					TECHNODE workTech = MGlobals::GetWorkingTechLevel(playerID);
					workTech.AddToNode(btResData->researchTech);
					MGlobals::SetWorkingTechLevel(workTech,playerID);
				}
			}
			THEMATRIX->OperationCompleted(agentID,dwMissionID);
			break;
		}
	case P_OP_QUEUE_REMOVE:
		{
			BuildCommandWIndex * myBuf = (BuildCommandWIndex *) buffer;
			BASIC_DATA * buildType = (BASIC_DATA *)(ARCHLIST->GetArchetypeData(findQueueValue(myBuf->index)));
			if(buildType->objClass == OC_RESEARCH)
			{
				BASE_RESEARCH_DATA * resType = (BASE_RESEARCH_DATA *)buildType;
				if(resType->type == RESEARCH_TECH|| resType->type == RESEARCH_ADMIRAL)
				{
					BT_RESEARCH * btResData = (BT_RESEARCH *)resType;
					TECHNODE workTech = MGlobals::GetWorkingTechLevel(playerID);
					workTech.RemoveFromNode(btResData->researchTech);
					MGlobals::SetWorkingTechLevel(workTech,playerID);
				}
			}

			removeIndexFromQueue(myBuf->index);//should never delete the working project because we don't have a real agentID
			CQASSERT((!bResearching) && (!bBuilding) && (!bUpgrading));
			THEMATRIX->OperationCompleted(agentID,dwMissionID);
			break;
		}
	case P_OP_RES_BEGIN:
		{
			if(queueSize) //make sure the queue is still full
			{
				U32 buildArchID = peekFabQueue();
				BASIC_DATA * buildType = (BASIC_DATA *)(ARCHLIST->GetArchetypeData(buildArchID));
				if(buildType->objClass == OC_RESEARCH)
				{
					BT_RESEARCH * resData = (BT_RESEARCH *) buildType;
					constructionID = buildArchID;
					workTime = resData->time*(1.0-MGlobals::GetAIBonus(playerID));
					workingCost = resData->cost;
					buidTimeSpent = 0;
					currentResearch = resData->researchTech;
					bResearching = true;
					workingID = agentID;
				}
				else //it is a space ship
				{
					CQASSERT( 0 && "should be a research");
				}
			}
			else
			{
				CQASSERT(0 && "Empty Queue for Build");
			}
			break;
		}
	case P_OP_BUILD_BEGIN:
		{
			workingID = agentID;
			THEMATRIX->SetCancelState(workingID,false);
			if(queueSize)
			{
				U32 buildArchID = peekFabQueue();
				BASIC_DATA * buildType = (BASIC_DATA *)(ARCHLIST->GetArchetypeData(buildArchID));
				if(buildType->objClass == OC_SPACESHIP)
				{
					BASE_SPACESHIP_DATA * shipData =  (BASE_SPACESHIP_DATA *)buildType;
					workingCost = shipData->missionData.resourceCost;

					IBaseObject *buildee;
					BuildCommandWHandle * myBuf = (BuildCommandWHandle *) buffer;
					constructionID = myBuf->handle;
					buildee = MGlobals::CreateInstance(ARCHLIST->LoadArchetype(buildArchID),constructionID);
					OBJLIST->AddObject(buildee);
					OBJPTR<IBuild> build;
					if (buildee->QueryInterface(IBuildID,build))
					{
						TRANSFORM trans= GetShipTransform();
						
						VOLPTR(IPhysicalObject) miss;
						if ((miss = buildee) != 0)
						{
							miss->SetTransform(trans, systemID);
							ENGINE->update_instance(buildee->GetObjectIndex(),0,0);
						}
						FabStartBuild(build);
					}
					else
					{
						CQASSERT(0 && "Tried to build soething that has no IBuild interface");
					}
					
				}
				else
				{
					CQASSERT(0 && "Bad Queue Data for Build");
				}
			}
			else
			{
				CQASSERT(0 && "Empty Queue for Build");
			}

			break;
		}
	case P_OP_FREE_SHIP:
		{
			bFreeShipMade = true;

			BuildCommandWHandle * myBuf = (BuildCommandWHandle *) buffer;
			IBaseObject *freeShip;
			freeShip = MGlobals::CreateInstance(ARCHLIST->LoadArchetype(freeShipArchID),myBuf->handle);
			OBJLIST->AddObject(freeShip);

			TRANSFORM trans= GetTransform();
			trans.set_position(trans.get_position()-Vector(0,0,100));
				
			VOLPTR(IPhysicalObject) miss;
			if ((miss = freeShip) != 0)
			{
				miss->SetTransform(trans, systemID);
				ENGINE->update_instance(freeShip->GetObjectIndex(),0,0);
			}

			MPartNC part(freeShip);
			freeShip->SetReady(true);
			part->hullPoints = part->hullPointsMax;
			part->supplies = 0;

			MScript::RunProgramsWithEvent(CQPROGFLAG_OBJECTCONSTRUCTED, freeShip->GetPartID());

			THEMATRIX->OperationCompleted(agentID,myBuf->handle);
			THEMATRIX->OperationCompleted(agentID,dwMissionID);
			break;
		}

	}
}
//---------------------------------------------------------------------------
//
void RefinePlat::receiveOperationData (U32 agentID, void *buffer, U32 bufferSize)
{
	if(workingID != agentID)
		return;
	BaseBuildCommand * buf = (BaseBuildCommand *)buffer;
	switch(buf->command)
	{
	case P_OP_UPGRADE_HALT:
		{
			CancelUpgrade();
			bUpgrading = false;

			COMP_OP(dwMissionID);
			break;
		}
	case P_OP_BUILD_HALT_CLEARQ:
	case P_OP_BUILD_HALT:
		{
			if(buildee)
			{
				THEMATRIX->OperationCompleted(workingID,constructionID);
				FabHaltBuild();
			}
			bBuilding = false;
			if(buf->command == P_OP_BUILD_HALT_CLEARQ)
				clearQueue();
			COMP_OP(dwMissionID);
			if(bDelayed)
			{
				BANKER->RemoveStalledSpender(playerID,this);
				bDelayed = false;
			}
			break;
		}
	case P_OP_DELAY:
		{
			failSound(M_COMMANDPTS);
			bDelayed = true;
			BANKER->AddStalledSpender(playerID,this);
			break;
		}
	case P_OP_REMOVE_DELAY:
		{
			bDelayed = false;
			BANKER->RemoveStalledSpender(playerID,this);
			break;
		}
	case P_OP_QUEUE_FAIL:
		{
			BuildCommandWHandle * com = (BuildCommandWHandle *)buf;
			failSound((M_RESOURCE_TYPE)(com->handle));
			break;
		}
	case P_OP_UPGRADE_FINISH:
		{
			BT_UPGRADE * resData = (BT_UPGRADE *)(ARCHLIST->GetArchetypeData(constructionID));
			COMM_MUTATION_COMPLETED(((char *)(resData->resFinishedSound)),resData->resFinishSubtitle,pInitData->displayName);

			SetUpgrade(upgradeToLevel);
			popFabQueue();
							
			bUpgrading = false;
			constructionID = 0;

			COMP_OP(dwMissionID);	
			break;
		}

	case P_OP_QUEUE_ADD:
		{
			BuildCommandWHandleIndex * myBuf = (BuildCommandWHandleIndex *) buffer;
			addToQueue(myBuf->handle,myBuf->index);
			BASIC_DATA * buildType = (BASIC_DATA *)(ARCHLIST->GetArchetypeData(myBuf->handle));
			if(buildType->objClass == OC_RESEARCH)
			{
				BASE_RESEARCH_DATA * resType = (BASE_RESEARCH_DATA *)buildType;
				if(resType->type == RESEARCH_TECH|| resType->type == RESEARCH_ADMIRAL)
				{
					BT_RESEARCH * btResData = (BT_RESEARCH *)resType;
					TECHNODE workTech = MGlobals::GetWorkingTechLevel(playerID);
					workTech.AddToNode(btResData->researchTech);
					MGlobals::SetWorkingTechLevel(workTech,playerID);
				}
			}
			break;
		}
	case P_OP_STOP:
		{
			U32 current = 0;
			U32 currentIndex =0;
			if(bResearching)
			{
				TECHNODE curTech;
				bResearching = false;	
				COMP_OP(dwMissionID);
			}
			else if(bBuilding)
			{
				buildee->CancelBuild();			
				current = peekFabQueue();
				currentIndex = getFirstQueueIndex();
			}
			if(bDelayed)
			{
				bDelayed = false;
				BANKER->RemoveStalledSpender(playerID,this);
			}
			clearQueue();
			if(current)
				addToQueue(current,currentIndex);	
			break;
		}
	case P_OP_QUEUE_REMOVE:
		{
			BuildCommandWIndex * myBuf = (BuildCommandWIndex *) buffer;
			S32 index = myBuf->index;
			U32 queuePos = getQueuePositionFromIndex(index);
			BASIC_DATA * buildType = (BASIC_DATA *)(ARCHLIST->GetArchetypeData(findQueueValue(index)));
			if(buildType->objClass == OC_RESEARCH)
			{
				BASE_RESEARCH_DATA * resType = (BASE_RESEARCH_DATA *)buildType;
				if(resType->type == RESEARCH_TECH|| resType->type == RESEARCH_ADMIRAL)
				{
					BT_RESEARCH * btResData = (BT_RESEARCH *)resType;
					TECHNODE workTech = MGlobals::GetWorkingTechLevel(playerID);
					workTech.RemoveFromNode(btResData->researchTech);
					MGlobals::SetWorkingTechLevel(workTech,playerID);
				}
			}
			if(bResearching)
			{
				removeIndexFromQueue(index);
				if(queuePos == 0)
				{
					TECHNODE curTech;
					curTech = MGlobals::GetWorkingTechLevel(playerID);
					curTech.RemoveFromNode(currentResearch);
					MGlobals::SetWorkingTechLevel(curTech,playerID);
					bResearching = false;	
					if(bDelayed)
					{
						bDelayed = false;
						BANKER->RemoveStalledSpender(playerID,this);
					}
					COMP_OP(dwMissionID);
				}
			}
			else if(bBuilding)
			{
				if(queuePos == 0)
				{
					buildee->CancelBuild();			
					if(bDelayed)
					{
						bDelayed = false;
						BANKER->RemoveStalledSpender(playerID,this);
					}
				}
				else
				{
					removeIndexFromQueue(index);
				}
			}
			else if(bUpgrading)
			{
				if(queuePos == 0)
				{
					if(bDelayed)
					{
						bDelayed = false;
						BANKER->RemoveStalledSpender(playerID,this);
					}
					CancelUpgrade();
					bUpgrading = false;
					removeIndexFromQueue(index);
					COMP_OP(dwMissionID);
				}
				else
				{
					removeIndexFromQueue(index);
				}
			}
			break;
		}
	case P_OP_RES_FINISH:
		{
			if(agentID == workingID)
			{
				BT_RESEARCH * resData = (BT_RESEARCH *)(ARCHLIST->GetArchetypeData(constructionID));
				COMM_RESEARCH_COMPLETED(((char *)(resData->resFinishedSound)),resData->resFinishSubtitle,pInitData->displayName);

				TECHNODE curTech = MGlobals::GetCurrentTechLevel(playerID);
				curTech.AddToNode(currentResearch);
				MGlobals::SetCurrentTechLevel(curTech,playerID);
				curTech = MGlobals::GetWorkingTechLevel(playerID);
				curTech.RemoveFromNode(currentResearch);
				MGlobals::SetWorkingTechLevel(curTech,playerID);
				popFabQueue();
				constructionID = 0;
				bResearching = false;
				COMP_OP(dwMissionID);
			}
			break;
		}
	case P_OP_ADMIRAL_FINISHED:
		{
			if(agentID == workingID)
			{
				BuildCommandWHandle * myBuf = (BuildCommandWHandle *) buffer;
				THEMATRIX->OperationCompleted(workingID,myBuf->handle);

				TECHNODE curTech = MGlobals::GetCurrentTechLevel(playerID);
				curTech.AddToNode(currentResearch);
				MGlobals::SetCurrentTechLevel(curTech,playerID);
				curTech = MGlobals::GetWorkingTechLevel(playerID);
				curTech.RemoveFromNode(currentResearch);
				MGlobals::SetWorkingTechLevel(curTech,playerID);
				popFabQueue();
				bResearching = false;

				// add the admiral creation to the scoreboard
				MGlobals::SetNumAdmiralsBuilt(playerID, MGlobals::GetNumAdmiralsBuilt(playerID)+1);

				IBaseObject *admiral;
				BT_ADMIRAL_RES * admiralRes = (BT_ADMIRAL_RES *) (ARCHLIST->GetArchetypeData(constructionID));
;
				admiral = MGlobals::CreateInstance(ARCHLIST->LoadArchetype(admiralRes->flagshipType),myBuf->handle);
				OBJLIST->AddObject(admiral);

				TRANSFORM trans= GetTransform();
				trans.set_position(trans.get_position()-Vector(0,0,100));
						
				VOLPTR(IPhysicalObject) miss;
				if ((miss = admiral) != 0)
				{
					miss->SetTransform(trans, systemID);
					ENGINE->update_instance(admiral->GetObjectIndex(),0,0);
				}

				MPartNC part(admiral);
				admiral->SetReady(true);
				part->hullPoints = part->hullPointsMax;
				part->supplies = 0;
		
				SPACESHIP_ALERT(admiral, admiral->GetPartID(), constructComplete,SUB_CONSTRUCTION_COMP, part.pInit->displayName,ALERT_UNIT_BUILT);

				constructionID = 0;
				COMP_OP(dwMissionID);
			}
			break;
		}
	case P_OP_BUILD_FINISH:
		{
			if(agentID == workingID)
			{
				OBJPTR<IBaseObject> baseBuild;
				if(buildee)
				{
					if(!buildee->IsReversing())
					{
						MGlobals::SetNumUnitsBuilt(playerID,MGlobals::GetNumUnitsBuilt(playerID)+1);
						MPartNC part(buildee.Ptr());
						part->bUnderCommand = true;
					}
					buildee->QueryInterface(IBaseObjectID,baseBuild);
					THEMATRIX->OperationCompleted(workingID,baseBuild->GetPartID());	
				}
				FabEnableCompletion();
				popFabQueue();
				COMP_OP(dwMissionID);
			}
		}
	}
}
//---------------------------------------------------------------------------
//
void RefinePlat::onOperationCancel (U32 agentID)
{
	if(bResearching)
	{
		TECHNODE curTech;
		curTech = MGlobals::GetWorkingTechLevel(playerID);
		curTech.RemoveFromNode(currentResearch);
		MGlobals::SetWorkingTechLevel(curTech,playerID);
		if(THEMATRIX->IsMaster())
			BANKER->AddResource(GetPlayerID(),workingCost);
		bResearching = false;	
		if(bDelayed)
		{
			bDelayed = false;
			BANKER->RemoveStalledSpender(playerID,this);
		}
		workingID = NULL;
	}
	else if(bUpgrading)
	{
		if(THEMATRIX->IsMaster())
			BANKER->AddResource(GetPlayerID(),workingCost);
		if(bDelayed)
		{
			bDelayed = false;
			BANKER->RemoveStalledSpender(playerID,this);
		}
		CancelUpgrade();
		if(THEMATRIX->IsMaster())
		{
			BANKER->FreeCommandPt(playerID,workingCost.commandPt);
		}
		bUpgrading = false;
		workingID = NULL;
	}
	
	clearQueue();
}
//---------------------------------------------------------------------------
//
void RefinePlat::OnStopRequest (U32 agentID)
{
	if(workingID == agentID)
	{
		BaseBuildCommand buffer;
		buffer.command = P_OP_STOP;
		THEMATRIX->SendOperationData(workingID,dwMissionID,&buffer,sizeof(buffer));

		CQASSERT(bBuilding);
		if(buildee)
		{
			if(!buildee->IsReversing())
			{
				buildee->CancelBuild();		
				BANKER->AddResource(playerID,workingCost);
			}
		}
		U32 current = peekFabQueue();
		U32 currentIndex = getFirstQueueIndex();
		popFabQueue();
		while(queueSize)
		{
			BASIC_DATA * buildType = (BASIC_DATA *)(ARCHLIST->GetArchetypeData(peekFabQueue()));
			if(buildType->objClass == OC_RESEARCH)
			{
				BASE_RESEARCH_DATA * resType = (BASE_RESEARCH_DATA *)buildType;
				BANKER->AddResource(playerID,resType->cost);
				if(resType->type == RESEARCH_TECH|| resType->type == RESEARCH_ADMIRAL)
				{
					BT_RESEARCH * btResData = (BT_RESEARCH *)resType;
					TECHNODE workTech = MGlobals::GetWorkingTechLevel(playerID);
					workTech.RemoveFromNode(btResData->researchTech);
					MGlobals::SetWorkingTechLevel(workTech,playerID);
				}
			}
			else
			{
				BASE_SPACESHIP_DATA * shipData = (BASE_SPACESHIP_DATA * )buildType;
				BANKER->AddResource(playerID,shipData->missionData.resourceCost);
			}
			popFabQueue();
		}
		addToQueue(current,currentIndex);	
		if(bDelayed)
		{
			bDelayed = false;
			BANKER->RemoveStalledSpender(playerID,this);
		}
	}
}
//---------------------------------------------------------------------------
//
void RefinePlat::OnMasterChange (bool bIsMaster)
{
	TFabricator<Platform<REFINEPLAT_SAVELOAD, REFINEPLAT_INIT>
								>::OnMasterChange(bIsMaster);
	//platfroms seem to have no host specific or client specific modes
}
//---------------------------------------------------------------------------
//
enum HARVEST_STANCE RefinePlat::GetHarvestStance()
{
	return defaultHarvestStance;
}
//---------------------------------------------------------------------------
//
void RefinePlat::SetHarvestStance(enum HARVEST_STANCE hStance)
{
	defaultHarvestStance = hStance;
}
//---------------------------------------------------------------------------
//
inline struct IBaseObject * createPlatform (const REFINEPLAT_INIT & data)
{
	RefinePlat * obj = new ObjectImpl<RefinePlat>;

	obj->FRAME_init(data);
	if (obj->initRefinePlat(data) == false)
	{
		delete obj;
		obj = 0;
	}

	return obj;
}
//------------------------------------------------------------------------------------------
//
REFINEPLAT_INIT::~REFINEPLAT_INIT (void)					// free archetype references
{
	for (int i = 0; i < NUM_DRONE_RELEASE; i++)
		if (builderInfo[i].pBuilderType)
			ARCHLIST->Release(builderInfo[i].pBuilderType, OBJREFNAME);
}
//------------------------------------------------------------------------------------------
//---------------------------RefinePlat Factory----------------------------------------------
//------------------------------------------------------------------------------------------
//
struct DACOM_NO_VTABLE RefinePlatFactory : public IObjectFactory
{
	U32 factoryHandle;		// handles to callback

	//
	// Interface mapping
	//

	BEGIN_DACOM_MAP_INBOUND(RefinePlatFactory)
	DACOM_INTERFACE_ENTRY(IObjectFactory)
	END_DACOM_MAP()

	RefinePlatFactory (void) { }

	~RefinePlatFactory (void);

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

	/* RefinePlatFactory methods */

	IDAComponent * getBase (void)
	{
		return static_cast<IObjectFactory *> (this);
	}
};
//--------------------------------------------------------------------------//
//
RefinePlatFactory::~RefinePlatFactory (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (OBJLIST && OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
		connection->Unadvise(factoryHandle);
}
//--------------------------------------------------------------------------//
//
void RefinePlatFactory::init (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
		connection->Advise(getBase(), &factoryHandle);
}
//-----------------------------------------------------------------------------
//
HANDLE RefinePlatFactory::CreateArchetype (const char *szArchname, OBJCLASS objClass, void *_data)
{
	REFINEPLAT_INIT * result = 0;

	if (objClass == OC_PLATFORM)
	{
		BT_PLAT_REFINE_DATA * data = (BT_PLAT_REFINE_DATA *) _data;
		
		if (data->type == PC_REFINERY)
		{
			result = new REFINEPLAT_INIT;

			if (result->loadPlatformArchetype(data, ARCHLIST->GetArchetype(szArchname)))
			{
				int i;
				for (i = 0; i < NUM_DRONE_RELEASE; i++)
				{
					const DRONE_RELEASE * drone =  &data->drone_release[i];
	
					if ((result->builderInfo[i].pBuilderType = ARCHLIST->LoadArchetype(drone->builderType)) != 0)
						ARCHLIST->AddRef(result->builderInfo[i].pBuilderType, OBJREFNAME);

						result->builderInfo[i].numDrones = drone->numDrones;
				}
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
BOOL32 RefinePlatFactory::DestroyArchetype (HANDLE hArchetype)
{
	REFINEPLAT_INIT * objtype = (REFINEPLAT_INIT *)hArchetype;

	EditorStopObjectInsertion(objtype->pArchetype);
	delete objtype;

	return 1;
}
//-------------------------------------------------------------------
//
IBaseObject * RefinePlatFactory::CreateInstance (HANDLE hArchetype)
{
	REFINEPLAT_INIT * objtype = (REFINEPLAT_INIT *)hArchetype;
	return createPlatform(*objtype);
}
//-------------------------------------------------------------------
//
void RefinePlatFactory::EditorCreateInstance (HANDLE hArchetype, const MISSION_INFO & info)
{
	REFINEPLAT_INIT * objtype = (REFINEPLAT_INIT *)hArchetype;
	EditorStartObjectInsertion(objtype->pArchetype, info);
}
//---------------------------------------------------------------------------
//-------------------------GLOBAL STARTUP------------------------------------
//---------------------------------------------------------------------------
//
struct _refineplatfactory : GlobalComponent
{
	RefinePlatFactory * sfactory;

	virtual void Startup (void)
	{
		sfactory = new DAComponent<RefinePlatFactory>;
		AddToGlobalCleanupList((IDAComponent **) &sfactory);
	}

	virtual void Initialize (void)
	{
		sfactory->init();
	}
};

static _refineplatfactory __plat;

//--------------------------------------------------------------------------//
//----------------------------End RefinePlat.cpp-------------------------------//
//--------------------------------------------------------------------------//
