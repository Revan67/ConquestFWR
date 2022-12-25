//--------------------------------------------------------------------------//
//                                                                          //
//                               BuildSupPlat.cpp                                //
//                                                                          //
//                  COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/BuildSupPlat.cpp 108   7/27/01 10:29a Tmauer $
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
#include "ISupplier.h"
#include "IRepairee.h"
#include "ObjMapIterator.h"
#include "SoundManager.h"

#include <DBuildSupPlat.h>
#include <DSupplyShipSave.h>

#include <Physics.h>
#include <3DMath.h>
#include <FileSys.h>
#include <IConnection.h>
#include <TComponent.h>
#include <HeapObj.h>

#define HOST_CHECK THEMATRIX->IsMaster()

#define COMP_OP(operation) { U32 tempID = workingID; workingID = 0; THEMATRIX->OperationCompleted(tempID,operation);}

#define RESUPPLY_TIME 10.0

#define CIRC_TIME 4.0

//P_OP_BUILD_FINISH
//P_OP_RES_BEGIN
//P_OP_RES_FINISH
//P_OP_STOP
//P_OP_BUILD_REMOVE_DELAY
#pragma pack(push,1)
struct BaseBuildCommand
{
	U8 command:4;
};

//P_OP_QUEUE_ADD
//P_OP_QUEUE_REMOVE
//P_OP_BUILD_BEGIN
//P_OP_BUILD_NO_ROOM
struct BuildCommandWHandle
{
	U8 command:4;
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

struct _NO_VTABLE BuildSupPlat : public TFabricator<
									Platform<BUILDSUPPLAT_SAVELOAD, BUILDSUPPLAT_INIT>
								>,
											ISpender,ISupplier, BASE_BUILDSUPPLAT_SAVELOAD
{
	BEGIN_MAP_INBOUND(BuildSupPlat)
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
	_INTERFACE_ENTRY(ISpender)
	_INTERFACE_ENTRY(IShipDamage)
	_INTERFACE_ENTRY(ISupplier)
	_INTERFACE_ENTRY(IExtent)
	_INTERFACE_ENTRY(IBuildQueue)
	_INTERFACE_ENTRY(IEffectTarget)
	END_MAP()

	UpdateNode		updateNode;
	PhysUpdateNode  physUpdateNode;
	PreDestructNode	destructNode;
	SaveNode		saveNode;
	LoadNode		loadNode;
	ResolveNode		resolveNode;
	OnOpCancelNode	onOpCancelNode;
	PreTakeoverNode	preTakeoverNode;
	ReceiveOpDataNode	receiveOpDataNode;

	//
	// research data
	//
	SINGLE_TECHNODE currentResearch;
	U32 workTime;
	ResourceCost workingCost;

	//bools
	bool bDelayed:1;

	HardpointInfo  shippointinfo;
	INSTANCE_INDEX shipPointIndex;

	//supply data
	SINGLE supplyRange;
	SINGLE supplyPerSecond;

	SINGLE circleTime;

	enum COMMUNICATION
	{
		P_OP_QUEUE_ADD,
		P_OP_QUEUE_REMOVE,
		P_OP_POP_QUEUE,
		P_OP_BUILD_BEGIN,
		P_OP_BUILD_FINISH,
		P_OP_BUILD_HALT_CLEARQ,
		P_OP_BUILD_HALT,
		P_OP_RES_BEGIN,
		P_OP_RES_FINISH,
		P_OP_STOP,
		P_OP_DELAY,
		P_OP_REMOVE_DELAY,
		P_OP_QUEUE_FAIL
	};


	BuildSupPlat (void);

	virtual ~BuildSupPlat (void);	// See ObjList.cpp

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

	virtual void TakeoverSwitchID (U32 newMissionID);

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
	
	/* ISupplier methods */

	virtual void SupplyTarget(U32 agentID, IBaseObject * obj);

	virtual void SetSupplyStance (enum SUPPLY_SHIP_STANCE _bAutoSupply)
	{};

	virtual enum SUPPLY_SHIP_STANCE GetSupplyStance ()
	{
		return SUP_STANCE_NONE;
	};

	virtual void SetSupplyEscort (U32 agentID, IBaseObject * target)
	{
		CQBOMB0("Not implemented! (ignorable)");
		THEMATRIX->OperationCompleted(agentID, dwMissionID);
	}

	/*IMissionActor methods */


	/* IBaseObject */

	virtual void TestVisible (const USER_DEFAULTS & defaults, const U32 currentSystem, const U32 currentPlayer);

	virtual void Render (void);

	virtual void MapRender(bool bPing);
	
	/* Platform methods */

	virtual const char * getSaveStructName (void) const		// must be overriden implemented by derived class
	{
		return "BUILDSUPPLAT_SAVELOAD";
	}

	virtual void * getViewStruct (void)				// must be overriden implemented by derived class
	{
		return static_cast<BASE_BUILDSUPPLAT_SAVELOAD *>(this);
	}

	/* BuildPlat methods */

	BOOL32 updateBuild (void);

	void physUpdateBuild (SINGLE dt);

	void render (void);

	bool initBuildPlat (const BUILDSUPPLAT_INIT & data);

	void save (BUILDSUPPLAT_SAVELOAD & save);

	void load (BUILDSUPPLAT_SAVELOAD & load);

	void resolve (void);

	void destruct (void);

	void preTakeover (U32 newMissionID, U32 troopID);

	bool isAdmiral(U32 archID);

	void clearQueue();

};
//---------------------------------------------------------------------------
//
BuildSupPlat::BuildSupPlat (void) :
			updateNode(this, UpdateProc(&BuildSupPlat::updateBuild)),
			physUpdateNode(this, PhysUpdateProc(&BuildSupPlat::physUpdateBuild)),
			saveNode(this, CASTSAVELOADPROC(&BuildSupPlat::save)),
			loadNode(this, CASTSAVELOADPROC(&BuildSupPlat::load)),
			resolveNode(this, ResolveProc(&BuildSupPlat::resolve)),
			destructNode(this, PreDestructProc(&BuildSupPlat::destruct)),
			onOpCancelNode(this, OnOpCancelProc(&BuildSupPlat::onOperationCancel)),
			preTakeoverNode(this, PreTakeoverProc(&BuildSupPlat::preTakeover)),
			receiveOpDataNode(this, ReceiveOpDataProc(&BuildSupPlat::receiveOperationData))
{
	circleTime = 0;
}
//---------------------------------------------------------------------------
//
BuildSupPlat::~BuildSupPlat (void)
{
}
//---------------------------------------------------------------------------
//
void BuildSupPlat::LocalStartBuild()
{
	fabStartAnims();
	return;
}
//---------------------------------------------------------------------------
//
void BuildSupPlat::LocalEndBuild(bool killAgent)
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
		if(buildee)
		{
			THEMATRIX->OperationCompleted(workingID,buildee.ptr->GetPartID());	
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
void BuildSupPlat::BuildQueue (U32 dwArchetypeDataID, COMMANDS command)
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
							BT_RESEARCH * btResData = (BT_RESEARCH *)buildType;
							TECHNODE workTech = MGlobals::GetWorkingTechLevel(playerID);
							if((!isInQueue(dwArchetypeDataID)) && (!(workTech.HasTech(btResData->researchTech))))
							{
								M_RESOURCE_TYPE failType;
								if(BANKER->SpendMoney(playerID,btResData->cost,&failType))
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
							BT_RESEARCH * btResData = (BT_RESEARCH *)buildType;
							TECHNODE workTech = MGlobals::GetWorkingTechLevel(playerID);
							if((!isInQueue(dwArchetypeDataID)) && (!(workTech.HasTech(btResData->researchTech))))
							{
								M_RESOURCE_TYPE failType;
								if(BANKER->SpendMoney(playerID,btResData->cost,&failType))
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
							if(resType->type == RESEARCH_TECH)
							{
								BT_RESEARCH * btResData = (BT_RESEARCH *)resType;
								TECHNODE curTech;
								curTech = MGlobals::GetWorkingTechLevel(playerID);
								curTech.RemoveFromNode(btResData->researchTech);
								MGlobals::SetWorkingTechLevel(curTech,playerID);
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
							CQASSERT((!bResearching) && (!bBuilding));
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
SINGLE BuildSupPlat::FabGetProgress(U32 & stallType)
{
	if(bDelayed)
	{
		stallType = IActiveButton::NO_MONEY;
		return 0;
	}
	else if(bResearching)
	{
		stallType = IActiveButton::NO_STALL;
		return (buildTimeSpent/ workTime);
	}
	else if(bBuilding)
	{
	 	return buildee->GetBuildProgress(stallType);
	}
	return 0.0;
}
//---------------------------------------------------------------------------
//
SINGLE BuildSupPlat::FabGetDisplayProgress(U32 & stallType)
{
	if(bDelayed)
	{
		stallType = IActiveButton::NO_MONEY;
		return 0.0;
	}
	else if(bResearching)
	{
		stallType = IActiveButton::NO_STALL;
		return (buildTimeSpent/ workTime);
	}
	else if(bBuilding)
	{
	 	return buildee->GetBuildDisplayProgress(stallType);
	}
	return 0.0;
}
//---------------------------------------------------------------------------
//
ResourceCost BuildSupPlat::GetAmountNeeded()
{
	return workingCost;
}
//---------------------------------------------------------------------------
//	
void BuildSupPlat::UnstallSpender()
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
void BuildSupPlat::OnOperationCreation (U32 agentID, void *buffer, U32 bufferSize)
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
	case P_OP_QUEUE_ADD:
		{
			BuildCommandWHandleIndex * myBuf = (BuildCommandWHandleIndex *) buffer;
			addToQueue(myBuf->handle,myBuf->index);
			BASIC_DATA * buildType = (BASIC_DATA *)(ARCHLIST->GetArchetypeData(myBuf->handle));
			if(buildType->objClass == OC_RESEARCH)
			{
				BASE_RESEARCH_DATA * resType = (BASE_RESEARCH_DATA *)buildType;
				if(resType->type == RESEARCH_TECH)
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
				if(resType->type == RESEARCH_TECH)
				{
					BT_RESEARCH * btResData = (BT_RESEARCH *)resType;
					TECHNODE workTech = MGlobals::GetWorkingTechLevel(playerID);
					workTech.RemoveFromNode(btResData->researchTech);
					MGlobals::SetWorkingTechLevel(workTech,playerID);
				}
			}

			removeIndexFromQueue(myBuf->index);//should never delete the working project because we don't have a real agentID
			CQASSERT((!bResearching) && (!bBuilding));
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
					buildTimeSpent = 0;
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
void BuildSupPlat::receiveOperationData (U32 agentID, void *buffer, U32 bufferSize)
{
	if(workingID != agentID)
		return;
	BaseBuildCommand * buf = (BaseBuildCommand *)buffer;
	switch(buf->command)
	{
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
	case P_OP_QUEUE_ADD:
		{
			BuildCommandWHandleIndex * myBuf = (BuildCommandWHandleIndex *) buffer;
			addToQueue(myBuf->handle,myBuf->index);
			BASIC_DATA * buildType = (BASIC_DATA *)(ARCHLIST->GetArchetypeData(myBuf->handle));
			if(buildType->objClass == OC_RESEARCH)
			{
				BASE_RESEARCH_DATA * resType = (BASE_RESEARCH_DATA *)buildType;
				if(resType->type == RESEARCH_TECH)
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
				if(resType->type == RESEARCH_TECH)
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
	case P_OP_BUILD_FINISH:
		{
			if(agentID == workingID)
			{
				if(buildee)
				{
					if(!buildee->IsReversing())
					{
						MGlobals::SetNumUnitsBuilt(playerID,MGlobals::GetNumUnitsBuilt(playerID)+1);
						MPartNC part(buildee.Ptr());
						part->bUnderCommand = true;
					}
					THEMATRIX->OperationCompleted(workingID,buildee.Ptr()->GetPartID());	
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
void BuildSupPlat::onOperationCancel (U32 agentID)
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

	clearQueue();
}
//---------------------------------------------------------------------------
//
void BuildSupPlat::OnStopRequest (U32 agentID)
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
				if(resType->type == RESEARCH_TECH)
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
void BuildSupPlat::preTakeover(U32 newMissionID, U32 troopID)
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
	}
	CQASSERT(!workingID);
	clearQueue();
}
//---------------------------------------------------------------------------
//
void BuildSupPlat::OnMasterChange (bool bIsMaster)
{
	TFabricator<
		Platform<BUILDSUPPLAT_SAVELOAD, BUILDSUPPLAT_INIT>
		>::OnMasterChange(bIsMaster);
	//platfroms seem to have no host specific or client specific modes
}
//---------------------------------------------------------------------------
//
void BuildSupPlat::TakeoverSwitchID (U32 newMissionID)
{
	TFabricator<Platform<BUILDSUPPLAT_SAVELOAD, BUILDSUPPLAT_INIT> >::TakeoverSwitchID(newMissionID);
	SECTOR->ComputeSupplyForAllPlayers();
}
//---------------------------------------------------------------------------
//
void BuildSupPlat::ParkYourself (const TRANSFORM & _transform, U32 planetID, U32 slotID)
{
	IBaseObject * obj = OBJLIST->FindObject(planetID);
	VOLPTR(IPlanet) planet;

	if (obj && (planet=obj)!=0)
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
TRANSFORM BuildSupPlat::GetShipTransform()
{
	TRANSFORM trans;
	trans.set_orientation(shippointinfo.orientation);
	trans.set_position(shippointinfo.point);
	trans = transform.multiply(trans);
	return trans;
}
//---------------------------------------------------------------------------
//
void BuildSupPlat::SupplyTarget(U32 agentID, IBaseObject * obj)
{
	CQASSERT((!bPlatDead) && "Tried to order a Dead plat to attack");

	THEMATRIX->OperationCompleted(agentID,dwMissionID);
}
//---------------------------------------------------------------------------
//
void BuildSupPlat::SetRallyPoint (const struct NETGRIDVECTOR & point)
{
	rallyPoint = point;
}
//---------------------------------------------------------------------------
//
bool BuildSupPlat::GetRallyPoint (struct NETGRIDVECTOR & point)
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
void BuildSupPlat::TestVisible (const USER_DEFAULTS & defaults, const U32 currentSystem, const U32 currentPlayer)
{
	Platform<BUILDSUPPLAT_SAVELOAD, BUILDSUPPLAT_INIT>::TestVisible(defaults, currentSystem, currentPlayer);
}
//-------------------------------------------------------------------
//
void BuildSupPlat::Render (void)
{
	Platform<BUILDSUPPLAT_SAVELOAD, BUILDSUPPLAT_INIT>::Render();

	render();
}
//-------------------------------------------------------------------
//
void BuildSupPlat::MapRender(bool bPing)
{
	Platform<BUILDSUPPLAT_SAVELOAD, BUILDSUPPLAT_INIT>::MapRender(bPing);
}
//-------------------------------------------------------------------
//
bool BuildSupPlat::isAdmiral (U32 archID)
{
	BASE_SPACESHIP_DATA	* data  = (BASE_SPACESHIP_DATA	*) ARCHLIST->GetArchetypeData(archID);

	return (data->objClass == OC_SPACESHIP && data->type == SSC_FLAGSHIP);
}
//---------------------------------------------------------------------------
//
void BuildSupPlat::clearQueue()
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
BOOL32 BuildSupPlat::updateBuild (void)
{
	if(HOST_CHECK && (!bExploding) && bReady && SECTOR->SystemInSupply(systemID,playerID))
	{
		supplyTimer += ELAPSED_TIME;
		if(supplyTimer > RESUPPLY_TIME)
		{
			supplyTimer -=RESUPPLY_TIME;
			U32 resupplyAmount = RESUPPLY_TIME*supplyPerSecond;
			if(resupplyAmount && !fieldFlags.suppliesLocked())
			{
				IBaseObject * searchObj = NULL;
				SINGLE realRange = supplyRange*MGlobals::GetTenderUpgrade(this);
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

	//building stuff
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

					IBaseObject * builtShip = buildee.Ptr();
					if(buildee)
					{
						if(!buildee->IsReversing())
						{
							MGlobals::SetNumUnitsBuilt(playerID,MGlobals::GetNumUnitsBuilt(playerID)+1);
							MPartNC part(buildee.Ptr());
							part->bUnderCommand = true;
						}
						THEMATRIX->OperationCompleted(workingID,buildee.Ptr()->GetPartID());	
					}
					FabEnableCompletion();
					popFabQueue();
					COMP_OP(dwMissionID);
					if(builtShip!=0 && (rallyPoint.systemID != MAX_SYSTEMS+1))
					{
						USR_PACKET<USRMOVE> packet;

						packet.objectID[0] = builtShip->GetPartID();
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
			buildTimeSpent += ELAPSED_TIME*effectFlags.getResearchBoost();
			if(buildTimeSpent >= workTime)
			{
				BaseBuildCommand buffer;
				buffer.command = P_OP_RES_FINISH;
				THEMATRIX->SendOperationData(workingID,dwMissionID,&buffer,sizeof(buffer));

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
		}
		else if(queueSize && (!bDelayed) && (!bBuilding) && (!bResearching) && (!workingID))
		{
			U32 buildArchID = peekFabQueue();
			BASIC_DATA * buildType = (BASIC_DATA *)(ARCHLIST->GetArchetypeData(buildArchID));
			if(buildType)
			{
				if(buildType->objClass == OC_RESEARCH)//is it research
				{
					BASE_RESEARCH_DATA * bResData = (BASE_RESEARCH_DATA *) buildType;
					if(bResData->type == RESEARCH_TECH)
					{
						BT_RESEARCH * resData = (BT_RESEARCH *) buildType;
						currentResearch = resData->researchTech;
						
						TECHNODE haveTech = MGlobals::GetCurrentTechLevel(playerID);
						if(!(haveTech.HasSomeTech(currentResearch)) )//I am not working on this tech and I don't have this tech
						{
							workingCost = resData->cost;
							bResearching = true;
							
							BaseBuildCommand buffer;
							buffer.command = P_OP_RES_BEGIN;

							workingID = THEMATRIX->CreateOperation(dwMissionID,&buffer,sizeof(buffer));

							constructionID = buildArchID;
							workTime = resData->time*(1.0-MGlobals::GetAIBonus(playerID));
							buildTimeSpent = 0;
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
		if(bResearching && workingID && bReady)
		{
			buildTimeSpent += ELAPSED_TIME*effectFlags.getResearchBoost();
		}
	}
	return true;
}
//---------------------------------------------------------------------------
//
void BuildSupPlat::physUpdateBuild (SINGLE dt)
{
}
//---------------------------------------------------------------------------
//
void BuildSupPlat::render (void)
{
	if((systemID == SECTOR->GetCurrentSystem()) && (bHighlight || bSelected || BUILDARCHEID) && bReady && MGlobals::AreAllies(playerID,MGlobals::GetThisPlayer()))
	{
		circleTime += 0.03;
		while(circleTime >CIRC_TIME)
		{
			circleTime -= CIRC_TIME;
		}
		BATCH->set_state(RPR_BATCH,true);
		DisableTextures();
		CAMERA->SetModelView();
		BATCH->set_state(RPR_STATE_ID,0);
		BATCH->set_render_state(D3DRS_ZENABLE,FALSE);
		BATCH->set_render_state(D3DRS_ZWRITEENABLE,FALSE);
		BATCH->set_render_state(D3DRS_ALPHABLENDENABLE,TRUE);
		BATCH->set_render_state(D3DRS_SRCBLEND,D3DBLEND_ONE);
		BATCH->set_render_state(D3DRS_DESTBLEND,D3DBLEND_ONE);
		PB.Begin(PB_LINES);
		PB.Color3ub(0,128,0);
		SINGLE supRange = supplyRange*MGlobals::GetTenderUpgrade(this)*GRIDSIZE;
		Vector relDir =Vector(cos((2*PI*circleTime)/CIRC_TIME)*supRange,
					sin((2*PI*circleTime)/CIRC_TIME)*supRange,0);
		Vector oldVect = transform.translation+relDir;
		PB.Vertex3f(oldVect.x,oldVect.y,oldVect.z);
		oldVect = transform.translation-relDir;
		PB.Vertex3f(oldVect.x,oldVect.y,oldVect.z);
		PB.End();

		drawRangeCircle(supplyRange*MGlobals::GetTenderUpgrade(this),RGB(0,128,0));
	}
}
//---------------------------------------------------------------------------
//
bool BuildSupPlat::initBuildPlat (const BUILDSUPPLAT_INIT & data)
{
	bDelayed = false;

	const BT_PLAT_BUILDSUP_DATA * objData = data.pData;

	if (objData->ship_hardpoint[0])
		FindHardpoint(objData->ship_hardpoint, shipPointIndex, shippointinfo, instanceIndex);

	supplyRange = objData->supplyRadius;
	supplyPerSecond = objData->supplyPerSecond;

	rallyPoint.systemID = MAX_SYSTEMS+1;
	
	return true;
}
//---------------------------------------------------------------------------
//
void BuildSupPlat::save (BUILDSUPPLAT_SAVELOAD & save)
{
	save.buildPlatSaveload = *static_cast<BASE_BUILDSUPPLAT_SAVELOAD *>(this);
}
//---------------------------------------------------------------------------
//
void BuildSupPlat::load (BUILDSUPPLAT_SAVELOAD & load)
{
 	*static_cast<BASE_BUILDSUPPLAT_SAVELOAD *>(this) = load.buildPlatSaveload;
}
//---------------------------------------------------------------------------
//
void BuildSupPlat::resolve (void)
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
void BuildSupPlat::destruct (void)
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
	}
	CQASSERT(!workingID);
	clearQueue();
}
//---------------------------------------------------------------------------
//
inline struct IBaseObject * createPlatform (const BUILDSUPPLAT_INIT & data)
{
	BuildSupPlat * obj = new ObjectImpl<BuildSupPlat>;

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
BUILDSUPPLAT_INIT::~BUILDSUPPLAT_INIT (void)					// free archetype references
{
	for (int i = 0; i < NUM_DRONE_RELEASE; i++)
		if (builderInfo[i].pBuilderType)
			ARCHLIST->Release(builderInfo[i].pBuilderType, OBJREFNAME);
}
//------------------------------------------------------------------------------------------
//---------------------------BuildSupPlatFactory----------------------------------------------
//------------------------------------------------------------------------------------------
//
struct DACOM_NO_VTABLE BuildSupPlatFactory : public IObjectFactory
{
	U32 factoryHandle;		// handles to callback

	//
	// Interface mapping
	//

	BEGIN_DACOM_MAP_INBOUND(BuildSupPlatFactory)
	DACOM_INTERFACE_ENTRY(IObjectFactory)
	END_DACOM_MAP()

	BuildSupPlatFactory (void) { }

	~BuildSupPlatFactory (void);

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
BuildSupPlatFactory::~BuildSupPlatFactory (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (OBJLIST && OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
		connection->Unadvise(factoryHandle);
}
//--------------------------------------------------------------------------//
//
void BuildSupPlatFactory::init (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
		connection->Advise(getBase(), &factoryHandle);
}
//-----------------------------------------------------------------------------
//
HANDLE BuildSupPlatFactory::CreateArchetype (const char *szArchname, OBJCLASS objClass, void *_data)
{
	BUILDSUPPLAT_INIT * result = 0;

	if (objClass == OC_PLATFORM)
	{
		BT_PLAT_BUILDSUP_DATA * data = (BT_PLAT_BUILDSUP_DATA *) _data;
		
		if (data->type == PC_BUILDSUPPLAT)
		{
			result = new BUILDSUPPLAT_INIT;

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
BOOL32 BuildSupPlatFactory::DestroyArchetype (HANDLE hArchetype)
{
	BUILDSUPPLAT_INIT * objtype = (BUILDSUPPLAT_INIT *)hArchetype;

	EditorStopObjectInsertion(objtype->pArchetype);
	delete objtype;

	return 1;
}
//-------------------------------------------------------------------
//
IBaseObject * BuildSupPlatFactory::CreateInstance (HANDLE hArchetype)
{
	BUILDSUPPLAT_INIT * objtype = (BUILDSUPPLAT_INIT *)hArchetype;
	return createPlatform(*objtype);
}
//-------------------------------------------------------------------
//
void BuildSupPlatFactory::EditorCreateInstance (HANDLE hArchetype, const MISSION_INFO & info)
{
	BUILDSUPPLAT_INIT * objtype = (BUILDSUPPLAT_INIT *)hArchetype;
	EditorStartObjectInsertion(objtype->pArchetype, info);
}
//---------------------------------------------------------------------------
//-------------------------GLOBAL STARTUP------------------------------------
//---------------------------------------------------------------------------
//
struct _buildsupplatfactory : GlobalComponent
{
	BuildSupPlatFactory * sfactory;

	virtual void Startup (void)
	{
		sfactory = new DAComponent<BuildSupPlatFactory>;
		AddToGlobalCleanupList((IDAComponent **) &sfactory);
	}

	virtual void Initialize (void)
	{
		sfactory->init();
	}
};

static _buildsupplatfactory __plat;

//--------------------------------------------------------------------------//
//----------------------------End GunPlat.cpp-------------------------------//
//--------------------------------------------------------------------------//
