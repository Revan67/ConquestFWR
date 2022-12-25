//--------------------------------------------------------------------------//
//                                                                          //
//                               BuildPlat.cpp                                //
//                                                                          //
//                  COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/BuildPlat.cpp 105   7/27/01 10:29a Tmauer $
*/			    
//--------------------------------------------------------------------------//

#include "pch.h"
#include <globals.h>

#include "TPlatform.h"
#include "Startup.h"
#include "MGlobals.h"
#include "ObjList.h"
#include "GenData.h"
#include "CQLight.h"
#include "TFabricator.h"
#include "CommPacket.h"
#include "DResearch.h"
#include "ObjSet.h"
#include "SoundManager.h"

#include <DBuildPlat.h>

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

struct _NO_VTABLE BuildPlat : public TFabricator<
									Platform<BUILDPLAT_SAVELOAD, BUILDPLAT_INIT>
								>, ISpender,
											 BASE_BUILDPLAT_SAVELOAD
{
	BEGIN_MAP_INBOUND(BuildPlat)
	_INTERFACE_ENTRY(IBaseObject)
	_INTERFACE_ENTRY(ISaveLoad)
	_INTERFACE_ENTRY(IQuickSaveLoad)
	_INTERFACE_ENTRY(IWeaponTarget)
	_INTERFACE_ENTRY(IExplosionOwner)
	_INTERFACE_ENTRY(IMissionActor)
	_INTERFACE_ENTRY(IBuild)
	_INTERFACE_ENTRY(IPlatform)
	_INTERFACE_ENTRY(IPhysicalObject)
	_INTERFACE_ENTRY(IFabricator)
	_INTERFACE_ENTRY(IBuildQueue)
	_INTERFACE_ENTRY(ISpender)
	_INTERFACE_ENTRY(IShipDamage)
	_INTERFACE_ENTRY(IExtent)
	_INTERFACE_ENTRY(IUpgrade)
	_INTERFACE_ENTRY(IEffectTarget)
	END_MAP()

	UpdateNode		updateNode;
	PhysUpdateNode  physUpdateNode;
	RenderNode		renderNode;
	PreDestructNode	destructNode;
	SaveNode		saveNode;
	LoadNode		loadNode;
	ResolveNode		resolveNode;
	OnOpCancelNode	onOpCancelNode;
	PreTakeoverNode	preTakeoverNode;
	ReceiveOpDataNode	receiveOpDataNode;
	UpgradeNode		upgradeNode;


	ResourceCost workingCost;
	U32 workTime;

	//
	// research data
	//
	SINGLE_TECHNODE currentResearch;

	//
	// upgrade data
	//
	U32 upgradeToLevel;

	//bools
	bool bDelayed:1;

	HardpointInfo  shippointinfo;
	INSTANCE_INDEX shipPointIndex;

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
		P_OP_QUEUE_FAIL
	};


	BuildPlat (void);

	virtual ~BuildPlat (void);	// See ObjList.cpp

	/* IExplosionOwner methods */

	virtual void OnChildDeath (INSTANCE_INDEX child)
	{
	}

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

	// IMissionActor
	virtual void OnOperationCreation (U32 agentID, void *buffer, U32 bufferSize);

	void receiveOperationData (U32 agentID, void *buffer, U32 bufferSize);

	void onOperationCancel (U32 agentID);
	
	virtual void OnStopRequest (U32 agentID);

	virtual void OnMasterChange (bool bIsMaster);

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

	virtual void SetRallyPoint (const struct NETGRIDVECTOR & point);

	virtual bool GetRallyPoint (struct NETGRIDVECTOR & point);

	virtual bool IsDockLocked()
	{
		CQBOMB0("What!?");
		return true;
	}

	virtual void LockDock(IBaseObject * locker)
	{
		CQBOMB0("What!?");
	}

	virtual void UnlockDock(IBaseObject * locker)
	{
		CQBOMB0("What!?");
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
	
	/* IBaseObject */

//	virtual void TestVisible (const USER_DEFAULTS & defaults, const U32 currentSystem, const U32 currentPlayer);

//	virtual void Render (void);
	
	/* Platform methods */

	virtual const char * getSaveStructName (void) const		// must be overriden implemented by derived class
	{
		return "BUILDPLAT_SAVELOAD";
	}

	virtual void * getViewStruct (void)				// must be overriden implemented by derived class
	{
		return static_cast<BASE_BUILDPLAT_SAVELOAD *>(this);
	}

	/* BuildPlat methods */

	BOOL32 updateBuild (void);

	void physUpdateBuild (SINGLE dt);

	void render (void);

	bool initBuildPlat (const BUILDPLAT_INIT & data);

	void save (BUILDPLAT_SAVELOAD & save);

	void load (BUILDPLAT_SAVELOAD & load);

	void resolve (void);

	void destruct (void);

	void preTakeover (U32 newMissionID, U32 troopID);

	void upgradeBuild(const BUILDPLAT_INIT & data);

	bool isAdmiral(U32 archID);

	void clearQueue();
};
//---------------------------------------------------------------------------
//
BuildPlat::BuildPlat (void) :
			updateNode(this, UpdateProc(&BuildPlat::updateBuild)),
			physUpdateNode(this, PhysUpdateProc(&BuildPlat::physUpdateBuild)),
			renderNode(this,RenderProc(&BuildPlat::render)),
			saveNode(this, CASTSAVELOADPROC(&BuildPlat::save)),
			loadNode(this, CASTSAVELOADPROC(&BuildPlat::load)),
			resolveNode(this, ResolveProc(&BuildPlat::resolve)),
			destructNode(this, PreDestructProc(&BuildPlat::destruct)),
			onOpCancelNode(this, OnOpCancelProc(&BuildPlat::onOperationCancel)),
			preTakeoverNode(this, PreTakeoverProc(&BuildPlat::preTakeover)),
			receiveOpDataNode(this, ReceiveOpDataProc(&BuildPlat::receiveOperationData)),
			upgradeNode(this, UpgradeProc(CASTINITPROC(&BuildPlat::upgradeBuild)))
{
}
//---------------------------------------------------------------------------
//
BuildPlat::~BuildPlat (void)
{
}
//---------------------------------------------------------------------------
//
void BuildPlat::LocalStartBuild()
{
	fabStartAnims();
	return;
}
//---------------------------------------------------------------------------
//
void BuildPlat::LocalEndBuild(bool killAgent)
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
void BuildPlat::BuildQueue (U32 dwArchetypeDataID, COMMANDS command)
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
							else//admiral or tech
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
							else//admiral or tech
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
							if(resType->type == RESEARCH_TECH || resType->type == RESEARCH_ADMIRAL)
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
SINGLE BuildPlat::FabGetProgress(U32 & stallType)
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
SINGLE BuildPlat::FabGetDisplayProgress(U32 & stallType)
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
ResourceCost BuildPlat::GetAmountNeeded()
{
	return workingCost;
}
//---------------------------------------------------------------------------
//	
void BuildPlat::UnstallSpender()
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
void BuildPlat::OnOperationCreation (U32 agentID, void *buffer, U32 bufferSize)
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
				if(resType->type == RESEARCH_TECH || resType->type == RESEARCH_ADMIRAL)
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
	}
}
//---------------------------------------------------------------------------
//
void BuildPlat::receiveOperationData (U32 agentID, void *buffer, U32 bufferSize)
{
	if(workingID != agentID)
		return;
	BaseBuildCommand * buf = (BaseBuildCommand *)buffer;
	switch(buf->command)
	{
	case P_OP_UPGRADE_HALT:
		{
			clearQueue();
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
			if(bDelayed)
			{
				BANKER->RemoveStalledSpender(playerID,this);
				bDelayed = false;
			}
			if(buf->command == P_OP_BUILD_HALT_CLEARQ)
				clearQueue();
			COMP_OP(dwMissionID);
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
				if(resType->type == RESEARCH_TECH || resType->type == RESEARCH_ADMIRAL)
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
				if(resType->type == RESEARCH_TECH || resType->type == RESEARCH_ADMIRAL)
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
				constructionID = 0;
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

				SPACESHIP_ALERT(admiral, admiral->GetPartID(), constructComplete,SUB_CONSTRUCTION_COMP, part.pInit->displayName, ALERT_UNIT_BUILT);

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
void BuildPlat::onOperationCancel (U32 agentID)
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
void BuildPlat::OnStopRequest (U32 agentID)
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
				if(resType->type == RESEARCH_TECH || resType->type == RESEARCH_ADMIRAL)
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
void BuildPlat::preTakeover(U32 newMissionID, U32 troopID)
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
			while(queueSize)
			{
				popFabQueue();
			}
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
			bUpgrading = false;
			BANKER->FreeCommandPt(playerID,workingCost.commandPt);

			COMP_OP(dwMissionID);
		}
	}
	CQASSERT(!workingID);
	clearQueue();
}
//---------------------------------------------------------------------------
//
void BuildPlat::upgradeBuild(const BUILDPLAT_INIT & data)
{
	const BT_PLAT_BUILD_DATA * objData = data.pData;

	if (objData->ship_hardpoint[0])
		FindHardpoint(objData->ship_hardpoint, shipPointIndex, shippointinfo, instanceIndex);
}
//---------------------------------------------------------------------------
//
void BuildPlat::OnMasterChange (bool bIsMaster)
{
	TFabricator<
		Platform<BUILDPLAT_SAVELOAD, BUILDPLAT_INIT>
		>::OnMasterChange(bIsMaster);
	//platfroms seem to have no host specific or client specific modes
}
//---------------------------------------------------------------------------
//
void BuildPlat::ParkYourself (const TRANSFORM & _transform, U32 planetID, U32 slotID)
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
	rallyPoint.init(GetGridPosition(),systemID);

	if(mObjClass == M_JUMPPLAT || mObjClass == M_HQ || mObjClass == M_COCOON || mObjClass == M_ACROPOLIS || mObjClass == M_LOCUS)
	{
		SECTOR->ComputeSupplyForAllPlayers();
	}
}
//---------------------------------------------------------------------------
//
TRANSFORM BuildPlat::GetShipTransform()
{
	TRANSFORM trans;
	trans.set_orientation(shippointinfo.orientation);
	trans.set_position(shippointinfo.point);
	trans = transform.multiply(trans);
	return trans;
}
//---------------------------------------------------------------------------
//
void BuildPlat::SetRallyPoint (const struct NETGRIDVECTOR & point)
{
	rallyPoint = point;
}
//---------------------------------------------------------------------------
//
bool BuildPlat::GetRallyPoint (struct NETGRIDVECTOR & point)
{
	//CQASSERT(rallyPoint.systemID != MAX_SYSTEMS+1);

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
/*void BuildPlat::TestVisible (const USER_DEFAULTS & defaults, const U32 currentSystem, const U32 currentPlayer)
{
	Platform<BUILDPLAT_SAVELOAD, BUILDPLAT_INIT>::TestVisible(defaults, currentSystem, currentPlayer);

	for(U32 count = 0; count < numDrones; ++count)
	{
		if(drone[count])
			drone[count].ptr->TestVisible(defaults, currentSystem, currentPlayer);
	}
}
//-------------------------------------------------------------------
//
void BuildPlat::Render (void)
{
	Platform<BUILDPLAT_SAVELOAD, BUILDPLAT_INIT>::Render();
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
bool BuildPlat::isAdmiral (U32 archID)
{
	BASE_SPACESHIP_DATA	* data  = (BASE_SPACESHIP_DATA	*) ARCHLIST->GetArchetypeData(archID);

	return (data->objClass == OC_SPACESHIP && data->type == SSC_FLAGSHIP);
}
//---------------------------------------------------------------------------
//
void BuildPlat::clearQueue()
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
BOOL32 BuildPlat::updateBuild (void)
{
	if(HOST_CHECK && (!bExploding) && bReady)
	{
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
					if(baseBuild && (rallyPoint.systemID != MAX_SYSTEMS+1))
					{
						USR_PACKET<USRMOVE> packet;

						packet.objectID[0] = baseBuild->GetPartID();
						packet.position = rallyPoint;
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
						buffer.handle = MGlobals::CreateNewPartID(playerID);
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

						SPACESHIP_ALERT(admiral, admiral->GetPartID(), constructComplete,SUB_CONSTRUCTION_COMP, part.pInit->displayName, ALERT_UNIT_BUILT);

						USR_PACKET<USRMOVE> packet;

						packet.objectID[0] = admiral->GetPartID();
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
			if(SECTOR->SystemInSupply(systemID,playerID))
			{
				U32 buildArchID = peekFabQueue();
				BASIC_DATA * buildType = (BASIC_DATA *)(ARCHLIST->GetArchetypeData(buildArchID));
				if(buildType)
				{
					if(buildType->objClass == OC_RESEARCH)//is it research
					{
						BASE_RESEARCH_DATA * bResData = (BASE_RESEARCH_DATA *) buildType;
						if(bResData->type == RESEARCH_TECH || bResData->type == RESEARCH_ADMIRAL)
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
	}
	else
	{
		if((bUpgrading || bResearching) && workingID && bReady)
		{
			if(SECTOR->SystemInSupply(systemID,playerID))
			{
				if(bResearching)
					buidTimeSpent += ELAPSED_TIME*effectFlags.getResearchBoost();
				else
					buidTimeSpent += ELAPSED_TIME;
			}
		}
	}
	return true;
}
//---------------------------------------------------------------------------
//
void BuildPlat::physUpdateBuild (SINGLE dt)
{
}
//---------------------------------------------------------------------------
//
void BuildPlat::render (void)
{
}
//---------------------------------------------------------------------------
//
bool BuildPlat::initBuildPlat (const BUILDPLAT_INIT & data)
{
	const BT_PLAT_BUILD_DATA * objData = data.pData;

	bDelayed = false;
	if (objData->ship_hardpoint[0])
		FindHardpoint(objData->ship_hardpoint, shipPointIndex, shippointinfo, instanceIndex);

	rallyPoint.systemID = MAX_SYSTEMS+1;

	return true;
}
//---------------------------------------------------------------------------
//
void BuildPlat::save (BUILDPLAT_SAVELOAD & save)
{
	save.buildPlatSaveload = *static_cast<BASE_BUILDPLAT_SAVELOAD *>(this);
}
//---------------------------------------------------------------------------
//
void BuildPlat::load (BUILDPLAT_SAVELOAD & load)
{
 	*static_cast<BASE_BUILDPLAT_SAVELOAD *>(this) = load.buildPlatSaveload;
}
//---------------------------------------------------------------------------
//
void BuildPlat::resolve (void)
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
}
//---------------------------------------------------------------------------
//
void BuildPlat::destruct (void)
{
	if(HOST_CHECK)
	{
		if(bResearching && workingID)
		{
			BaseBuildCommand buffer;
			buffer.command = P_OP_STOP;
			THEMATRIX->SendOperationData(workingID,dwMissionID,&buffer,sizeof(buffer));

			clearQueue();
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
			bUpgrading = false;
			BANKER->FreeCommandPt(playerID,workingCost.commandPt);

			COMP_OP(dwMissionID);
		}
	}
	clearQueue();
}
//---------------------------------------------------------------------------
//
inline struct IBaseObject * createPlatform (const BUILDPLAT_INIT & data)
{
	BuildPlat * obj = new ObjectImpl<BuildPlat>;

	obj->FRAME_init(data);
	if (obj->initBuildPlat(data) == false)
	{
		delete obj;
		obj = 0;
	}

	return obj;
}
//------------------------------------------------------------------------------------------
//
BUILDPLAT_INIT::~BUILDPLAT_INIT (void)					// free archetype references
{
	for (int i = 0; i < NUM_DRONE_RELEASE; i++)
		if (builderInfo[i].pBuilderType)
			ARCHLIST->Release(builderInfo[i].pBuilderType, OBJREFNAME);
}
//------------------------------------------------------------------------------------------
//---------------------------BuildPlat Factory----------------------------------------------
//------------------------------------------------------------------------------------------
//
struct DACOM_NO_VTABLE BuildPlatFactory : public IObjectFactory
{
	U32 factoryHandle;		// handles to callback

	//
	// Interface mapping
	//

	BEGIN_DACOM_MAP_INBOUND(BuildPlatFactory)
	DACOM_INTERFACE_ENTRY(IObjectFactory)
	END_DACOM_MAP()

	BuildPlatFactory (void) { }

	~BuildPlatFactory (void);

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

	/* GunplatFactory methods */

	IDAComponent * getBase (void)
	{
		return static_cast<IObjectFactory *> (this);
	}
};
//--------------------------------------------------------------------------//
//
BuildPlatFactory::~BuildPlatFactory (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (OBJLIST && OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
		connection->Unadvise(factoryHandle);
}
//--------------------------------------------------------------------------//
//
void BuildPlatFactory::init (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
		connection->Advise(getBase(), &factoryHandle);
}
//-----------------------------------------------------------------------------
//
HANDLE BuildPlatFactory::CreateArchetype (const char *szArchname, OBJCLASS objClass, void *_data)
{
	BUILDPLAT_INIT * result = 0;

	if (objClass == OC_PLATFORM)
	{
		BT_PLAT_BUILD_DATA * data = (BT_PLAT_BUILD_DATA *) _data;
		
		if (data->type == PC_BUILDPLAT)
		{
			result = new BUILDPLAT_INIT;

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
BOOL32 BuildPlatFactory::DestroyArchetype (HANDLE hArchetype)
{
	BUILDPLAT_INIT * objtype = (BUILDPLAT_INIT *)hArchetype;

	EditorStopObjectInsertion(objtype->pArchetype);
	delete objtype;

	return 1;
}
//-------------------------------------------------------------------
//
IBaseObject * BuildPlatFactory::CreateInstance (HANDLE hArchetype)
{
	BUILDPLAT_INIT * objtype = (BUILDPLAT_INIT *)hArchetype;
	return createPlatform(*objtype);
}
//-------------------------------------------------------------------
//
void BuildPlatFactory::EditorCreateInstance (HANDLE hArchetype, const MISSION_INFO & info)
{
	BUILDPLAT_INIT * objtype = (BUILDPLAT_INIT *)hArchetype;
	EditorStartObjectInsertion(objtype->pArchetype, info);
}
//---------------------------------------------------------------------------
//-------------------------GLOBAL STARTUP------------------------------------
//---------------------------------------------------------------------------
//
struct _buildplatfactory : GlobalComponent
{
	BuildPlatFactory * sfactory;

	virtual void Startup (void)
	{
		sfactory = new DAComponent<BuildPlatFactory>;
		AddToGlobalCleanupList((IDAComponent **) &sfactory);
	}

	virtual void Initialize (void)
	{
		sfactory->init();
	}
};

static _buildplatfactory __plat;

//--------------------------------------------------------------------------//
//----------------------------End GunPlat.cpp-------------------------------//
//--------------------------------------------------------------------------//
