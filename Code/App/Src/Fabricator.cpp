//--------------------------------------------------------------------------//
//                                                                          //
//                             Fabricator.cpp                                //
//                                                                          //
//                  COPYRIGHT (C) 1997 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/Fabricator.cpp 222   7/09/01 11:02a Tmauer $
*/			    
//------------------------------- #INCLUDES --------------------------------//
//--------------------------------------------------------------------------//

#include "pch.h"
#include <globals.h>

#include "Sector.h"
//#include "TObjTrans.h"
//#include "TObjControl.h"
//#include "TObjFrame.h"
#include "TSpaceShip.h"
#include <DSpaceShip.h>
#include "TObject.h"
#include "Objlist.h"
#include "Iplanet.h"
#include "Startup.h"
#include "TFabricator.h"
#include "Mission.h"
#include "SysMap.h"
#include "CommPacket.h"
#include "MPart.h"
#include "ObjSet.h"
#include <DPlatform.h>
#include "DMTechNode.h"

#include <IConnection.h>
#include <TComponent.h>
#include <TSmartPointer.h>
#include <FileSys.h>
#include <EventSys.h>

#define NO_PATH_DIST 8000
#define FINAL_DIST 2000
#define DONE_DIST 50

//this is so I can add HasOldPackets easily.
#define HOST_CHECK THEMATRIX->IsMaster()

//#define NET_METRIC(opSize) {OBJLIST->DEBUG_AddNetworkBytes(IObjectList::FAB,opSize);}
#define NET_METRIC(opSize) ((void)0)

#define COMP_OP(operation) { U32 tempID = workingID; workingID = 0; THEMATRIX->OperationCompleted(tempID,operation);}

//FAB_C_AT_TARGET
//FAB_C_CANCEL_BUILD
//FAB_C_BUILD_FINISH
//FAB_C_STOP_ASSIST_BUILD
//FAB_C_STOP_BUILD
//FAB_C_REVERSE_BUILD
//FAB_C_IDLE_OP
//FAB_C_CANCEL_REPAIR
//FAB_C_AT_TARGET_REPAIR
//FAB_C_BEGIN_REPAIR
//FAB_C_REPAIR_FINISHED
//FAB_C_CANCEL_DISM
//FAB_C_AT_TARGET_DISM
//FAB_C_BEGIN_DISM
//FAB_C_DISM_FINISHED
//FAB_C_PRE_TAKEOVER
//FAB_C_BUILD_IMPOSIBLE
struct BaseFabCommand
{
	U8 command;
};

//FAB_C_BEGIN_CON_INFO
//FAB_C_BUILD_PLATFORM
#pragma pack(push,1)
struct FabCommandWHandle
{
	U8 command;
	U32 handle;
};

//FAB_C_REPAIR_BEG_PLANET
//FAB_C_DISM_BEG_PLANET
struct FabCommandW2Handle
{
	U8 command;
	U32 handle1;
	U32 handle2;
};

//FAB_C_REPAIR_BEG_POSITION
//FAB_C_DISM_BEG_POSITION
struct FabCommandWGridVect
{
	U8 command;
	GRIDVECTOR grid;
};
#pragma pack(pop)

struct _NO_VTABLE Fabricator : public TFabricator< SpaceShip<FABRICATOR_SAVELOAD, FABRICATOR_INIT> >, 
								ISpender, BASE_FABRICATOR_SAVELOAD
{
	BEGIN_MAP_INBOUND(Fabricator)
	_INTERFACE_ENTRY(IBaseObject)
	_INTERFACE_ENTRY(IGotoPos)
	_INTERFACE_ENTRY(ISaveLoad)
	_INTERFACE_ENTRY(IQuickSaveLoad)
	_INTERFACE_ENTRY(IExplosionOwner)
	_INTERFACE_ENTRY(IMissionActor)
	_INTERFACE_ENTRY(IWeaponTarget)
	_INTERFACE_ENTRY(IBuild)
	_INTERFACE_ENTRY(ISpender)
	_INTERFACE_ENTRY(IFabricator)
	_INTERFACE_ENTRY(IPhysicalObject)
	_INTERFACE_ENTRY(IRepairee)
	_INTERFACE_ENTRY(IShipDamage)
	_INTERFACE_ENTRY(IExtent)
	_INTERFACE_ENTRY(IBuildQueue)
	_INTERFACE_ENTRY(IShipMove)
	_INTERFACE_ENTRY(ICloak)
	_INTERFACE_ENTRY(IEffectTarget)
	END_MAP()

	UpdateNode updateNode;
	SaveNode	saveNode;
	LoadNode	loadNode;
	InitNode		initNode;
	ResolveNode		resolveNode;
	OnOpCancelNode	onOpCancelNode;
	PreTakeoverNode	preTakeoverNode;
	ReceiveOpDataNode receiveOpDataNode;
	PreDestructNode	destructNode;

	S32 mapDotCycle;

	OBJPTR<IBaseObject> workTarg;

	HSOUND hBuildSound;
	SFX::ID buildSoundID;

	U8 curDrone;

	SINGLE repairRate;

	enum FAB_NET_COMMANDS
	{
		FAB_C_BEGIN_CON_INFO,
		FAB_C_BUILD_CANCEL_EARLY,
		FAB_C_BUILD_CANCEL_LATE,
		FAB_C_AT_TARGET,
		FAB_C_BUILD_PLATFORM,
		FAB_C_CANCEL_BUILD,
		FAB_C_BUILD_FINISH,
		FAB_C_STOP_BUILD,
		FAB_C_REVERSE_BUILD,
		FAB_C_BUILD_START_FAILED,
		FAB_C_BUILD_IMPOSIBLE,

		FAB_C_BEGIN_CON_INFO_VECT,
		FAB_C_BEGIN_CON_INFO_JUMP,

		FAB_C_IDLE_OP,

		FAB_C_CANCEL_REPAIR,
		FAB_C_REPAIR_BEG_PLANET,
		FAB_C_REPAIR_BEG_POSITION,
		FAB_C_AT_TARGET_REPAIR,
		FAB_C_BEGIN_REPAIR,
		FAB_C_REPAIR_FINISHED,
		FAB_C_CANCEL_REPAIR_NOMONEY,

		FAB_C_CANCEL_DISM,
		FAB_C_DISM_BEG_PLANET,
		FAB_C_DISM_BEG_POSITION,
		FAB_C_DISM_CANCEL_LATE,
		FAB_C_AT_TARGET_DISM,
		FAB_C_BEGIN_DISM,
		FAB_C_DISM_FINISHED,
		FAB_C_STOP_DISM,
	};

		
	Fabricator (void);

	virtual ~Fabricator (void);

	/* IBaseObject */

//	virtual void TestVisible (const USER_DEFAULTS & defaults, const U32 currentSystem, const U32 currentPlayer);

//	virtual void Render (void);


	/* SpaceShip methods */
	
	virtual const char * getSaveStructName (void) const
	{
		return "FABRICATOR_SAVELOAD";
	}

	virtual void * getViewStruct (void)	
	{
		return static_cast<BASE_FABRICATOR_SAVELOAD *>(this);
	}
	// IFabricator method

	virtual void LocalStartBuild();

	virtual void LocalEndBuild(bool killAgent = false);

	virtual void BeginMove();

	virtual void BeginConstruction (U32 planetID, U32 slotID, U32 dwArchetypeDataID, U32 agentID);

	virtual void BeginConstruction (GRIDVECTOR position, U32 dwArchetypeDataID, U32 agentID);

	virtual void BeginConstruction (IBaseObject * jumpgate, U32 dwArchetypeDataID, U32 agentID);

	virtual void BeginRepair (U32 agentID, IBaseObject * repairTarget);

	virtual void BeginDismantle(U32 agentID, IBaseObject * dismantleTarget);

	virtual void ParkYourself (const TRANSFORM & _trans, U32 planetID, U32 slotID);

	virtual void BuildQueue (U32 dwArchetypeDataID, COMMANDS command);

	virtual U32 GetFabJobID ();

	virtual bool IsBuildingAgent (U32 agentID);

//	virtual const TRANSFORM & GetDroneTransform ();

	virtual U8 GetFabTab();

	virtual void SetFabTab(U8 tab);

	/* IMissionActor */

	// the following are process related
	// called in response to OpAgent::CreateOperation()
	virtual void OnOperationCreation (U32 agentID, void *buffer, U32 bufferSize);

	// called in response to OpAgent::SendOperationData()
	void receiveOperationData (U32 agentID, void *buffer, U32 bufferSize);

	// user has requested a different action
	void onOperationCancel (U32 agentID);
		
	void preSelfDestruct (void);

	virtual void OnStopRequest (U32 agentID);

	virtual void OnMasterChange (bool bIsMaster);

	virtual void OnAddToOperation (U32 agentID);

	//ISpender

	virtual ResourceCost GetAmountNeeded();
	
	virtual void UnstallSpender();
	
	virtual bool IsFabAtObj();

	/* Fabricator methods */

	BOOL32 updateFabricate (void);

//	bool initFabricator (PARCHETYPE pArchetype, S32 archIndex, S32 animIndex);
	void initFab (const FABRICATOR_INIT & data);

	void resolve (void);

	void save (FABRICATOR_SAVELOAD & save);
	void load (FABRICATOR_SAVELOAD & load);

	void preTakeover (U32 newMissionID, U32 troopID);

	void incrementScoringPlatformsBuilt (void)
	{
		MGlobals::SetNumPlatformsBuilt(playerID, MGlobals::GetNumPlatformsBuilt(playerID)+1);

		// if the platform is a jumpgate inhibitor than we update our jumpgate stats as well
		if (targetPlanetID)
		{
			IBaseObject * obj = OBJLIST->FindObject(targetPlanetID);
			if (obj->objClass == OC_JUMPGATE)	
			{
				MGlobals::SetNumJumpgatesControlled(playerID, MGlobals::GetNumJumpgatesControlled(playerID)+1);
			}
		}
	}

	bool moveToTarget();
};
//---------------------------------------------------------------------------
//
Fabricator::Fabricator (void) : 
	updateNode(this, UpdateProc(&Fabricator::updateFabricate)),
	saveNode(this, CASTSAVELOADPROC(&Fabricator::save)),
	loadNode(this, CASTSAVELOADPROC(&Fabricator::load)),
	initNode(this, CASTINITPROC(&Fabricator::initFab)),
	resolveNode(this, ResolveProc(&Fabricator::resolve)),
	onOpCancelNode(this, OnOpCancelProc(&Fabricator::onOperationCancel)),
	preTakeoverNode(this, PreTakeoverProc(&Fabricator::preTakeover)),
	receiveOpDataNode(this, ReceiveOpDataProc(&Fabricator::receiveOperationData)),
	destructNode(this, PreDestructProc(&Fabricator::preSelfDestruct))
{
}
//---------------------------------------------------------------------------
//
Fabricator::~Fabricator (void)
{
}
//---------------------------------------------------------------------------
//
void Fabricator::LocalStartBuild()
{
	Vector vect = GetPosition();

	if (hBuildSound==0)
		hBuildSound  = SFXMANAGER->Open(buildSoundID);
	SFXMANAGER->Play(hBuildSound,GetSystemID(),&vect,SFXPLAYF_LOOPING|SFXPLAYF_NORESTART);
}
//---------------------------------------------------------------------------
//
void Fabricator::LocalEndBuild(bool killAgent)
{
	SFXMANAGER->Stop(hBuildSound);
	OBJPTR<IBaseObject> obj;
	bBuilding = FALSE;
}
//---------------------------------------------------------------------------
//
void Fabricator::BeginRepair (U32 agentID, IBaseObject * repairTarget)
{
	CQASSERT((mode == FAB_IDLE) 
		|| (mode == FAB_WATING_TO_START_BUILD_CLIENT));
	CQASSERT(!workingID);
	workingID = agentID;

	if(HOST_CHECK)
	{
		if(repairTarget)
		{
			repairTarget->QueryInterface(IBaseObjectID,workTarg,SYSVOLATILEPTR);
			workTargID = repairTarget->GetPartID();
			//setting these for move code
			VOLPTR(IPlatform) plat=repairTarget;
			targetPlanetID = plat->GetPlanetID();
			if (targetPlanetID)
			{
				targetSlotID = plat->GetSlotID();
				FabCommandW2Handle buffer;
				buffer.command = FAB_C_REPAIR_BEG_PLANET;
				buffer.handle1 = targetPlanetID;
				buffer.handle2 = targetSlotID;
				THEMATRIX->SendOperationData(workingID,dwMissionID,&buffer,sizeof(buffer));
			}
			else
			{
				targetPosition = repairTarget->GetGridPosition();
				FabCommandWGridVect buffer;
				buffer.command = FAB_C_REPAIR_BEG_POSITION;
				buffer.grid = targetPosition;
				THEMATRIX->SendOperationData(workingID,dwMissionID,&buffer,sizeof(buffer));
			}

			//sent begining info to clients
			mode = FAB_MOVING_TO_TARGET_REPAIR;
			BeginMove();
		}
		else
		{
			BaseFabCommand buffer;
			buffer.command = FAB_C_CANCEL_REPAIR;
			NET_METRIC(sizeof(buffer));
			THEMATRIX->SendOperationData(workingID,dwMissionID,&buffer,sizeof(buffer));
			mode = FAB_IDLE;
			for(U32 count = 0; count<numDrones; ++count)
			{
				if(drone[count])
					drone[count]->Return();
			}
			COMP_OP(dwMissionID);
		}
	}
	else
	{
		if(repairTarget)
			workTargID = repairTarget->GetPartID();
		else
			workTargID = 0;

		mode = FAB_WAIT_REPAIR_INFO_CLIENT;
	}
	
}
//---------------------------------------------------------------------------
//
void Fabricator::BeginDismantle(U32 agentID, IBaseObject * dismantleTarget)
{
	CQASSERT((mode == FAB_IDLE) 
		|| (mode == FAB_WATING_TO_START_BUILD_CLIENT));
	CQASSERT(!workingID);
	workingID = agentID;

	if(HOST_CHECK)
	{
		if(dismantleTarget)
		{
			dismantleTarget->QueryInterface(IBaseObjectID,workTarg,SYSVOLATILEPTR);
			workTargID = dismantleTarget->GetPartID();
			//setting these for move code
			VOLPTR(IPlatform) plat=dismantleTarget;
			targetPlanetID = plat->GetPlanetID();
			if (targetPlanetID)
			{
				targetSlotID = plat->GetSlotID();
				FabCommandW2Handle buffer;
				buffer.command = FAB_C_DISM_BEG_PLANET;
				buffer.handle1 = targetPlanetID;
				buffer.handle2 = targetSlotID;
				THEMATRIX->SendOperationData(workingID,dwMissionID,&buffer,sizeof(buffer));
			}
			else
			{
				targetPosition = dismantleTarget->GetGridPosition();
				FabCommandWGridVect buffer;
				buffer.command = FAB_C_DISM_BEG_POSITION;
				buffer.grid = targetPosition;
				THEMATRIX->SendOperationData(workingID,dwMissionID,&buffer,sizeof(buffer));
			}

			//sent begining info to clients
			mode = FAB_MOVING_TO_TARGET_DISM;
			BeginMove();
		}
		else
		{
			BaseFabCommand buffer;
			buffer.command = FAB_C_CANCEL_DISM;
			NET_METRIC(sizeof(buffer));
			THEMATRIX->SendOperationData(workingID,dwMissionID,&buffer,sizeof(buffer));
			mode = FAB_IDLE;
			for(U32 count = 0; count<numDrones; ++count)
			{
				if(drone[count])
					drone[count]->Return();
			}
			COMP_OP(dwMissionID);
		}
	}
	else
	{
		if(dismantleTarget)
			workTargID = dismantleTarget->GetPartID();
		else
			workTargID = 0;

		mode = FAB_WAIT_DISM_INFO_CLIENT;
	}
}
//---------------------------------------------------------------------------
//
void Fabricator::BeginMove()
{
	IBaseObject * obj = OBJLIST->FindObject(targetPlanetID);
	
	moveStage = MS_PATHFIND;

	if (obj)
	{
		OBJPTR<IPlanet> planet;
		
		//get planet
		
		obj->QueryInterface(IPlanetID,planet);
		if (planet)
		{
			//planet case
			CQASSERT(targetSlotID);
			
			TRANSFORM trans;
			
			trans = planet->GetSlotTransform(targetSlotID);
			
			dir = -trans.get_k();
			GRIDVECTOR vec;
			vec = destPos = trans.translation+2000*dir;
			switch (race)
			{
			case M_TERRAN:
				destYaw = PI/2+TRANSFORM::get_yaw(dir);
				if (destYaw > PI)
					destYaw -= 2*PI;
				break;
			case M_MANTIS:
				destYaw = TRANSFORM::get_yaw(-dir);
				break;
			case M_SOLARIAN:
				destYaw = TRANSFORM::get_yaw(dir);
				break;
			}
			moveToPos(vec);
		}
		else
		{
			//jumpgate case - or repairing or dismantling existing plat
			destPos.set(0,0,0);
			moveToPos(obj->GetGridPosition());
		}
	}
	else
	{
		//deep space
		COMPTR<ITerrainMap> map;
		SECTOR->GetTerrainMap(systemID,map);
		FootprintInfo footprint;
		footprint.missionID = dwMissionID|0x10000000;
		footprint.height =box[2];
		footprint.flags = TERRAIN_DESTINATION | TERRAIN_HALFSQUARE;
		map->SetFootprint(&targetPosition,1,footprint);
		destPos = targetPosition;
		moveToPos(targetPosition);
	}

}
//---------------------------------------------------------------------------
//
void Fabricator::BeginConstruction (U32 planetID, U32 slotID, U32 dwArchetypeDataID, U32 agentID)
{
	CQASSERT((mode == FAB_IDLE) 
		|| (mode == FAB_WATING_TO_START_BUILD_CLIENT));
	CQASSERT(!workingID);
	workingID = agentID;

	targetPlanetID = planetID;
	targetSlotID = slotID;
	buildingID = dwArchetypeDataID;

	if(HOST_CHECK)
	{
		CQASSERT(dwArchetypeDataID);
		BASE_PLATFORM_DATA * baseData = (BASE_PLATFORM_DATA *)(ARCHLIST->GetArchetypeData(dwArchetypeDataID));
		CQASSERT(baseData);
		workingCost = baseData->missionData.resourceCost;
		M_RESOURCE_TYPE resType;
		if(BANKER->SpendMoney(playerID,workingCost,&resType))
		{
			if(BANKER->SpendCommandPoints(playerID,workingCost))
			{
				CQASSERT(targetSlotID);

				mode = FAB_MOVING_TO_TARGET;
				FabCommandWHandle buffer;
				buffer.command = FAB_C_BEGIN_CON_INFO;

				buffer.handle = targetSlotID;
				NET_METRIC(sizeof(buffer));
				THEMATRIX->SendOperationData(agentID,GetPartID(),&buffer,sizeof(buffer));

				BeginMove();
			}
			else
			{
				BANKER->AddResource(playerID,workingCost);
				FABRICATORCOMM(notEnoughCommandPoints,SUB_NO_CP);
				FabCommandWHandle buffer;
				buffer.command = FAB_C_BUILD_START_FAILED;
				buffer.handle = M_COMMANDPTS;
				NET_METRIC(sizeof(buffer));
				THEMATRIX->SendOperationData(workingID,dwMissionID,&buffer,sizeof(buffer));
				mode = FAB_IDLE;
				COMP_OP(dwMissionID);
				EVENTSYS->Post(CQE_FABRICATOR_FINISHED,(void *)dwMissionID);
			}
		}
		else
		{
			if(resType == M_GAS)
				FABRICATORCOMM(notEnoughGas,SUB_NO_GAS);
			else if(resType == M_METAL)
				FABRICATORCOMM(notEnoughMetal,SUB_NO_METAL);
			else if(resType == M_CREW)
				FABRICATORCOMM(notEnoughCrew,SUB_NO_CREW);

			FabCommandWHandle buffer;
			buffer.command = FAB_C_BUILD_START_FAILED;
			buffer.handle = resType;
			NET_METRIC(sizeof(buffer));
			THEMATRIX->SendOperationData(workingID,dwMissionID,&buffer,sizeof(buffer));
			mode = FAB_IDLE;
			COMP_OP(dwMissionID);
			EVENTSYS->Post(CQE_FABRICATOR_FINISHED,(void *)dwMissionID);
		}
	}
	else//client
	{
		mode = FAB_WAITING_INIT_CONS_CLIENT;
	}
}
//---------------------------------------------------------------------------
//
void Fabricator::BeginConstruction (GRIDVECTOR position, U32 dwArchetypeDataID, U32 agentID)
{
	CQASSERT((mode == FAB_IDLE) 
		|| (mode == FAB_WATING_TO_START_BUILD_CLIENT));
	CQASSERT(!workingID);
	workingID = agentID;

	targetPlanetID = 0;
	targetSlotID = 0;
	buildingID = dwArchetypeDataID;

	targetPosition = position;


	if(HOST_CHECK)
	{
		CQASSERT(dwArchetypeDataID);
		BASE_PLATFORM_DATA * baseData = (BASE_PLATFORM_DATA *)(ARCHLIST->GetArchetypeData(dwArchetypeDataID));
		CQASSERT(baseData);
		workingCost = baseData->missionData.resourceCost;
		M_RESOURCE_TYPE resType;
		if(BANKER->SpendMoney(playerID,workingCost,&resType))
		{
			if(BANKER->SpendCommandPoints(playerID,workingCost))
			{
				COMPTR<ITerrainMap> map;
				SECTOR->GetTerrainMap(systemID,map);
				if(map->IsGridEmpty(position,0,false))
				{

					//sent begining info to clients
					mode = FAB_MOVING_TO_TARGET;
					CQASSERT(dwArchetypeDataID);
					BaseFabCommand buffer;
					buffer.command = FAB_C_BEGIN_CON_INFO_VECT;

					NET_METRIC(sizeof(buffer));
					THEMATRIX->SendOperationData(agentID,GetPartID(),&buffer,sizeof(buffer));
					BeginMove();
				}
				else
				{
					BANKER->AddResource(playerID,workingCost);
					BANKER->FreeCommandPt(playerID,workingCost.commandPt);
					FABRICATORCOMM(buildImposible, SUB_NO_BUILD);
					FabCommandWHandle buffer;
					buffer.command = FAB_C_BUILD_START_FAILED;
					buffer.handle = 0xFFFFFFFF;
					NET_METRIC(sizeof(buffer));
					THEMATRIX->SendOperationData(workingID,dwMissionID,&buffer,sizeof(buffer));
					mode = FAB_IDLE;
					COMP_OP(dwMissionID);
					EVENTSYS->Post(CQE_FABRICATOR_FINISHED,(void *)dwMissionID);
				}
			}
			else
			{
				BANKER->AddResource(playerID,workingCost);
				FABRICATORCOMM(notEnoughCommandPoints, SUB_NO_CP);
				FabCommandWHandle buffer;
				buffer.command = FAB_C_BUILD_START_FAILED;
				buffer.handle = M_COMMANDPTS;
				NET_METRIC(sizeof(buffer));
				THEMATRIX->SendOperationData(workingID,dwMissionID,&buffer,sizeof(buffer));
				mode = FAB_IDLE;
				COMP_OP(dwMissionID);
				EVENTSYS->Post(CQE_FABRICATOR_FINISHED,(void *)dwMissionID);
			}
		}
		else
		{
			if(resType == M_GAS)
				FABRICATORCOMM(notEnoughGas, SUB_NO_GAS);
			else if(resType == M_METAL)
				FABRICATORCOMM(notEnoughMetal, SUB_NO_METAL);
			else if(resType == M_CREW)
				FABRICATORCOMM(notEnoughCrew, SUB_NO_CREW);

			FabCommandWHandle buffer;
			buffer.command = FAB_C_BUILD_START_FAILED;
			buffer.handle = resType;
			NET_METRIC(sizeof(buffer));
			THEMATRIX->SendOperationData(workingID,dwMissionID,&buffer,sizeof(buffer));
			mode = FAB_IDLE;
			COMP_OP(dwMissionID);
			EVENTSYS->Post(CQE_FABRICATOR_FINISHED,(void *)dwMissionID);
		}
	}
	else//client
	{
		mode = FAB_WAITING_INIT_CONS_CLIENT;
	}
}
//---------------------------------------------------------------------------
//
void Fabricator::BeginConstruction (IBaseObject * jumpgate, U32 dwArchetypeDataID, U32 agentID)
{
	CQASSERT((mode == FAB_IDLE) 
		|| (mode == FAB_WATING_TO_START_BUILD_CLIENT));
	CQASSERT(!workingID);
	workingID = agentID;

	targetPlanetID = jumpgate->GetPartID();
	targetSlotID = 0;
	buildingID = dwArchetypeDataID;

	if(HOST_CHECK)
	{
		CQASSERT(dwArchetypeDataID);
		BASE_PLATFORM_DATA * baseData = (BASE_PLATFORM_DATA *)(ARCHLIST->GetArchetypeData(dwArchetypeDataID));
		CQASSERT(baseData);
		workingCost = baseData->missionData.resourceCost;
		M_RESOURCE_TYPE resType;
		if(BANKER->SpendMoney(playerID,workingCost,&resType))
		{
			if(BANKER->SpendCommandPoints(playerID,workingCost))
			{
				OBJPTR<IJumpGate> gate;
				jumpgate->QueryInterface(IJumpGateID,gate);
				if(gate->CanIBuild(playerID))
				{
					//sent begining info to clients
					mode = FAB_MOVING_TO_TARGET;
					CQASSERT(dwArchetypeDataID);
					BaseFabCommand buffer;
					buffer.command = FAB_C_BEGIN_CON_INFO_JUMP;

					NET_METRIC(sizeof(buffer));
					THEMATRIX->SendOperationData(agentID,GetPartID(),&buffer,sizeof(buffer));
					BeginMove();
				}
				else
				{
					BANKER->AddResource(playerID,workingCost);
					BANKER->FreeCommandPt(playerID,workingCost.commandPt);
					FABRICATORCOMM(buildImposible, SUB_NO_BUILD);
					FabCommandWHandle buffer;
					buffer.command = FAB_C_BUILD_START_FAILED;
					buffer.handle = 0xFFFFFFFF;
					NET_METRIC(sizeof(buffer));
					THEMATRIX->SendOperationData(workingID,dwMissionID,&buffer,sizeof(buffer));
					mode = FAB_IDLE;
					COMP_OP(dwMissionID);
					EVENTSYS->Post(CQE_FABRICATOR_FINISHED,(void *)dwMissionID);
				}
			}
			else
			{
				BANKER->AddResource(playerID,workingCost);
				FABRICATORCOMM(notEnoughCommandPoints, SUB_NO_CP);
				FabCommandWHandle buffer;
				buffer.command = FAB_C_BUILD_START_FAILED;
				buffer.handle = M_COMMANDPTS;
				NET_METRIC(sizeof(buffer));
				THEMATRIX->SendOperationData(workingID,dwMissionID,&buffer,sizeof(buffer));
				mode = FAB_IDLE;
				COMP_OP(dwMissionID);
				EVENTSYS->Post(CQE_FABRICATOR_FINISHED,(void *)dwMissionID);
			}
		}
		else
		{
			if(resType == M_GAS)
				FABRICATORCOMM(notEnoughGas, SUB_NO_GAS);
			else if(resType == M_METAL)
				FABRICATORCOMM(notEnoughMetal, SUB_NO_METAL);
			else if(resType == M_CREW)
				FABRICATORCOMM(notEnoughCrew, SUB_NO_CREW);

			FabCommandWHandle buffer;
			buffer.command = FAB_C_BUILD_START_FAILED;
			buffer.handle = resType;
			NET_METRIC(sizeof(buffer));
			THEMATRIX->SendOperationData(workingID,dwMissionID,&buffer,sizeof(buffer));
			mode = FAB_IDLE;
			COMP_OP(dwMissionID);
			EVENTSYS->Post(CQE_FABRICATOR_FINISHED,(void *)dwMissionID);
		}
	}
	else//client
	{
		mode = FAB_WAITING_INIT_CONS_CLIENT;
	}

}
//---------------------------------------------------------------------------
//
void Fabricator::OnStopRequest (U32 agentID)
{
	if(agentID == workingID)
	{
		if(mode == FAB_BUILDING)
		{
			if(buildee)
			{
				if(buildee->NumFabOnBuild() > 1)
				{
					CQASSERT(0 && "Not Allowed");
/*					FabAssistBuildDone();
					BaseFabCommand buffer;
					buffer.command = FAB_C_STOP_ASSIST_BUILD;
					NET_METRIC(sizeof(buffer));
					THEMATRIX->SendOperationData(workingID,GetPartID(),&buffer,sizeof(buffer));
					mode = FAB_IDLE;
					COMP_OP(dwMissionID);
*/				}
				else
				{
					buildee->CancelBuild();

					BaseFabCommand buffer;
					buffer.command = FAB_C_REVERSE_BUILD;
					NET_METRIC(sizeof(buffer));
					THEMATRIX->SendOperationData(workingID,GetPartID(),&buffer,sizeof(buffer));
					mode = FAB_UNBUILD;
				}	
			}
		}
	}
}
//---------------------------------------------------------------------------
//
void Fabricator::OnMasterChange (bool bIsMaster)
{
	repairMasterChange(bIsMaster);
	if(bIsMaster)
	{
		switch(mode)
		{
		case FAB_IDLE:
		case FAB_MOVING_TO_TARGET:
		case FAB_MOVING_TO_READY_TARGET_HOST:
		case FAB_BUILDING:
		case FAB_UNBUILD:
		case FAB_EXPLODING:
		case FAB_MOVING_TO_TARGET_REPAIR:
		case FAB_MOVING_TO_READY_TARGET_REPAIR_HOST:
		case FAB_REPAIRING:
		case FAB_MOVING_TO_TARGET_DISM:
		case FAB_MOVING_TO_READY_TARGET_DISM_HOST:
		case FAB_DISMING:
			{
				//do nothing
				break;
			}
		case FAB_WAITING_INIT_CONS_CLIENT:
			{
				U32 agentID = workingID;
				workingID = 0;
				mode = FAB_IDLE;//I'm going to simualte the start of the build
				if(targetPlanetID)
				{
					if(targetSlotID)//building arround planet
					{
						BeginConstruction(targetPlanetID,targetSlotID,buildingID,agentID);
					}
					else//building at jumpgate
					{
						BeginConstruction(OBJLIST->FindObject(targetPlanetID),buildingID,agentID);
					}
				}
				else//building in space
				{
					BeginConstruction(targetPosition,buildingID,agentID);
				}
				break;
			}
		case FAB_AT_TARGET_CLIENT:
			{
				BaseFabCommand buffer;
				buffer.command = FAB_C_AT_TARGET;
				NET_METRIC(sizeof(buffer));
				THEMATRIX->SendOperationData(workingID,dwMissionID,&buffer,sizeof(buffer));	
				if(!targetPlanetID)
				{
					COMPTR<ITerrainMap> map;
					SECTOR->GetTerrainMap(systemID,map);
					FootprintInfo footprint;
					footprint.missionID = dwMissionID|0x10000000;
					footprint.height =box[2];
					footprint.flags = TERRAIN_DESTINATION | TERRAIN_HALFSQUARE;
					map->UndoFootprint(&targetPosition,1,footprint);
				}
				//no break;
			}
	
		case FAB_WATING_TO_START_BUILD_CLIENT:
			{	
				if(targetPlanetID)
				{
					//find the kind of operation I want to create
					//see if slot is taken
					IBaseObject * obj = OBJLIST->FindObject(targetPlanetID);
					CQASSERT(obj);
					if(obj->objClass == OC_JUMPGATE)
					{
						OBJPTR<IJumpGate> jumpgate;
						obj->QueryInterface(IJumpGateID,jumpgate);
						SINGLE range = GetGridPosition()-obj->GetGridPosition();
						if((jumpgate->GetPlayerOwner() != 0) || range > (sensorRadius*2))
						{
							FABRICATORCOMM(buildImposible, SUB_NO_BUILD);
							BaseFabCommand buffer;
							buffer.command = FAB_C_BUILD_IMPOSIBLE;
							NET_METRIC(sizeof(buffer));
							THEMATRIX->SendOperationData(workingID,dwMissionID,&buffer,sizeof(buffer));
							mode = FAB_IDLE;
							BANKER->AddResource(playerID,workingCost);
							COMP_OP(dwMissionID);
							EVENTSYS->Post(CQE_FABRICATOR_FINISHED,(void *)dwMissionID);
						}
						else
						{
							FabCommandWHandle buffer;
							buffer.command = FAB_C_BUILD_PLATFORM;
							buffer.handle = MGlobals::CreateNewPartID(playerID);
							THEMATRIX->AddObjectToOperation(workingID,obj->GetPartID());
							THEMATRIX->AddObjectToOperation(workingID,SECTOR->GetJumpgateDestination(obj)->GetPartID());
							THEMATRIX->AddObjectToOperation(workingID,buffer.handle);
							THEMATRIX->AddObjectToOperation(workingID,buffer.handle|0x10000000);
							NET_METRIC(sizeof(buffer));
							THEMATRIX->SendOperationData(workingID,dwMissionID,&buffer,sizeof(buffer));
							THEMATRIX->SetCancelState(workingID,false);

							IBaseObject * newPlat = MGlobals::CreateInstance(ARCHLIST->LoadArchetype(buildingID),buffer.handle);
							OBJPTR<IPlatform> platformPtr;
							newPlat->QueryInterface(IPlatformID,platformPtr);

							OBJLIST->AddObject(newPlat);
							OBJPTR<IPhysicalObject> miss;
							if (newPlat->QueryInterface(IPhysicalObjectID,miss))
							{
								miss->SetSystemID(GetSystemID());
							}
							platformPtr->ParkYourself(obj);
							ENGINE->update_instance(newPlat->GetObjectIndex(),0,0);
							OBJPTR<IBuild> targBuildee;
							newPlat->QueryInterface(IBuildID,targBuildee);
							targBuildee->SetProcessID(workingID);
							FabStartBuild(targBuildee);			
							mode = FAB_BUILDING;
						}
					}
					else//it is a planet
					{
						OBJPTR<IPlanet> planet;
						obj->QueryInterface(IPlanetID,planet);
						CQASSERT(planet != 0);

						SINGLE range = GetGridPosition()-obj->GetGridPosition();
						U32 slotHolder = planet->GetSlotUser(targetSlotID);
						if(slotHolder || (!(planet->IsBuildableBy(playerID))) || range > (sensorRadius*2))
						{
							FABRICATORCOMM(buildImposible,SUB_NO_BUILD);
							BaseFabCommand buffer;
							buffer.command = FAB_C_BUILD_IMPOSIBLE;
							NET_METRIC(sizeof(buffer));
							THEMATRIX->SendOperationData(workingID,dwMissionID,&buffer,sizeof(buffer));
							mode = FAB_IDLE;
							BANKER->AddResource(playerID,workingCost);
							COMP_OP(dwMissionID);
							EVENTSYS->Post(CQE_FABRICATOR_FINISHED,(void *)dwMissionID);
						}
						else //slot empty
						{
							//begin Construction Operation
							FabCommandWHandle buffer;
							buffer.command = FAB_C_BUILD_PLATFORM;
							buffer.handle = MGlobals::CreateNewPartID(playerID);
							U32 numSlots = ((BASE_PLATFORM_DATA *)(ARCHLIST->GetArchetypeData(buildingID)))->slotsNeeded;
							U32 slotIDs[12];
							planet->GetPlanetSlotMissionIDs(slotIDs,targetSlotID);
							
							for(U32 slotCount = 0; slotCount < numSlots; ++slotCount)
							{
								THEMATRIX->AddObjectToOperation(workingID,slotIDs[slotCount]);
							}

							THEMATRIX->AddObjectToOperation(workingID,buffer.handle);

							NET_METRIC(sizeof(buffer));
							THEMATRIX->SendOperationData(workingID,dwMissionID,&buffer,sizeof(buffer));
							THEMATRIX->SetCancelState(workingID,false);

							IBaseObject * newPlat = MGlobals::CreateInstance(ARCHLIST->LoadArchetype(buildingID),buffer.handle);
							OBJPTR<IPlatform> platformPtr;
							newPlat->QueryInterface(IPlatformID,platformPtr);

							OBJLIST->AddObject(newPlat);
							OBJPTR<IPhysicalObject> miss;
							if (newPlat->QueryInterface(IPhysicalObjectID,miss))
							{
								miss->SetSystemID(GetSystemID());
							}
							platformPtr->ParkYourself(planet->GetSlotTransform(targetSlotID),targetPlanetID,targetSlotID);
							ENGINE->update_instance(newPlat->GetObjectIndex(),0,0);
							OBJPTR<IBuild> targBuildee;
							newPlat->QueryInterface(IBuildID,targBuildee);
							targBuildee->SetProcessID(workingID);
							FabStartBuild(targBuildee);			
							mode = FAB_BUILDING;
						}
					}
				}
				else
				{
					//begin Construction Operation
					SINGLE range = GetGridPosition()-targetPosition;
					COMPTR<ITerrainMap> map;
					SECTOR->GetTerrainMap(systemID,map);
					if((range > (sensorRadius*2)) || (!(map->IsGridEmpty(targetPosition,0,false))))
					{
						FABRICATORCOMM(buildImposible,SUB_NO_BUILD);
						BaseFabCommand buffer;
						buffer.command = FAB_C_BUILD_IMPOSIBLE;
						NET_METRIC(sizeof(buffer));
						THEMATRIX->SendOperationData(workingID,dwMissionID,&buffer,sizeof(buffer));
						mode = FAB_IDLE;
						BANKER->AddResource(playerID,workingCost);
						COMP_OP(dwMissionID);
						EVENTSYS->Post(CQE_FABRICATOR_FINISHED,(void *)(void *)dwMissionID);
					}
					else
					{
						FabCommandWHandle buffer;
						buffer.command = FAB_C_BUILD_PLATFORM;
						buffer.handle = MGlobals::CreateNewPartID(playerID);
						THEMATRIX->AddObjectToOperation(workingID,buffer.handle);
						THEMATRIX->SendOperationData(workingID,dwMissionID,&buffer,sizeof(buffer));
						THEMATRIX->SetCancelState(workingID,false);

						IBaseObject * newPlat = MGlobals::CreateInstance(ARCHLIST->LoadArchetype(buildingID),buffer.handle);
						OBJPTR<IPlatform> platformPtr;
						newPlat->QueryInterface(IPlatformID,platformPtr);

						OBJLIST->AddObject(newPlat);
						VOLPTR(IPhysicalObject) miss;
						if ((miss = newPlat) != 0)
						{
							if(!platformPtr->IsHalfSquare())
								targetPosition.centerpos();
							miss->SetPosition(targetPosition, systemID);
						}
						ENGINE->update_instance(newPlat->GetObjectIndex(),0,0);
						OBJPTR<IBuild> targBuildee;
						newPlat->QueryInterface(IBuildID,targBuildee);
						targBuildee->SetProcessID(workingID);
						FabStartBuild(targBuildee);			
						mode = FAB_BUILDING;
					}
				}
				break;
			}
		case FAB_MOVING_TO_READY_TARGET_CLIENT:
			{
				mode = FAB_MOVING_TO_READY_TARGET_HOST;
				break;
			}
		case FAB_WAIT_REPAIR_INFO_CLIENT:
			{
				IBaseObject * repairTarget = OBJLIST->FindObject(workTargID);
				if(repairTarget)
				{
					repairTarget->QueryInterface(IBaseObjectID,workTarg,SYSVOLATILEPTR);
					workTargID = repairTarget->GetPartID();
					//setting these for move code
					VOLPTR(IPlatform) plat=repairTarget;
					targetPlanetID = plat->GetPlanetID();
					if (targetPlanetID)
					{
						targetSlotID = plat->GetSlotID();
						FabCommandW2Handle buffer;
						buffer.command = FAB_C_REPAIR_BEG_PLANET;
						buffer.handle1 = targetPlanetID;
						buffer.handle2 = targetSlotID;
						THEMATRIX->SendOperationData(workingID,dwMissionID,&buffer,sizeof(buffer));
					}
					else
					{
						targetPosition = repairTarget->GetGridPosition();
						FabCommandWGridVect buffer;
						buffer.command = FAB_C_REPAIR_BEG_POSITION;
						buffer.grid = targetPosition;
						THEMATRIX->SendOperationData(workingID,dwMissionID,&buffer,sizeof(buffer));
					}

					//sent begining info to clients
					mode = FAB_MOVING_TO_TARGET_REPAIR;
					BeginMove();
				}
				else
				{
					BaseFabCommand buffer;
					buffer.command = FAB_C_CANCEL_REPAIR;
					NET_METRIC(sizeof(buffer));
					THEMATRIX->SendOperationData(workingID,dwMissionID,&buffer,sizeof(buffer));
					mode = FAB_IDLE;
					for(U32 count = 0; count<numDrones; ++count)
					{
						if(drone[count])
							drone[count]->Return();
					}
					COMP_OP(dwMissionID);
				}
				break;
			}
		case FAB_AT_TARGET_REPAIR_CLIENT:
			{
				BaseFabCommand buffer;
				buffer.command = FAB_C_AT_TARGET_REPAIR;
				NET_METRIC(sizeof(buffer));
				THEMATRIX->SendOperationData(workingID,dwMissionID,&buffer,sizeof(buffer));	
				if(!targetPlanetID)
				{
					COMPTR<ITerrainMap> map;
					SECTOR->GetTerrainMap(systemID,map);
					FootprintInfo footprint;
					footprint.missionID = dwMissionID|0x10000000;
					footprint.height =box[2];
					footprint.flags = TERRAIN_DESTINATION | TERRAIN_HALFSQUARE;
					map->UndoFootprint(&targetPosition,1,footprint);
				}

				//no break;
			}
		case FAB_WATING_TO_START_REPAIR_CLIENT:
			{
				if(workTarg)
				{
					OBJPTR<IBuild> buildTarg;
					workTarg->QueryInterface(IBuildID,buildTarg);
					MPart repPart(workTarg);
					SINGLE dist = workTarg->GetGridPosition()-GetGridPosition();
					if(dist <= sensorRadius)
					{
						if(repPart->bReady && buildTarg->IsComplete())
						{
							MPart part(workTarg);
							ResourceCost cost;
							cost.commandPt = 0;
							cost.crew = (part.pInit->resourceCost.crew*(part->hullPointsMax - part->hullPoints))/(part->hullPointsMax*3);
							cost.metal = (part.pInit->resourceCost.metal*(part->hullPointsMax - part->hullPoints))/(part->hullPointsMax*3);
							cost.gas = (part.pInit->resourceCost.gas*(part->hullPointsMax - part->hullPoints))/(part->hullPointsMax*3);
							M_RESOURCE_TYPE failType;
							if(BANKER->SpendMoney(playerID,cost,&failType))
							{

								BaseFabCommand buffer;
								buffer.command = FAB_C_BEGIN_REPAIR;

								NET_METRIC(sizeof(buffer));
								THEMATRIX->SendOperationData(workingID,dwMissionID,&buffer,sizeof(buffer));
								mode = FAB_REPAIRING;

								oldHullPoints = repPart->hullPoints;

								Vector pos,dir;
								OBJPTR<IShipDamage> shipDamage;
								workTarg->QueryInterface(IShipDamageID,shipDamage);
								CQASSERT(shipDamage);
								shipDamage->GetNextDamageSpot(pos,dir);
								if(numDrones)
								{
									if(drone[curDrone])
									{
										VOLPTR(IShuttle) shuttle;
										shuttle = drone[curDrone];
										shuttle->WorkAtShipPos(workTarg,pos,dir,300);
									}
									curDrone = (curDrone+1)%numDrones;
								}
							}
							else
							{
								failSound(failType);
								FabCommandWHandle buffer;
								buffer.command = FAB_C_CANCEL_REPAIR_NOMONEY;
								buffer.handle = failType;
								NET_METRIC(sizeof(buffer));
								THEMATRIX->SendOperationData(workingID,dwMissionID,&buffer,sizeof(buffer));
								mode = FAB_IDLE;
								COMP_OP(dwMissionID);
							}				
						}
						else
						{
							BaseFabCommand buffer;
							buffer.command = FAB_C_CANCEL_REPAIR;
							NET_METRIC(sizeof(buffer));
							THEMATRIX->SendOperationData(workingID,dwMissionID,&buffer,sizeof(buffer));
							mode = FAB_IDLE;
							COMP_OP(dwMissionID);
						}
					}
					else
					{
						BaseFabCommand buffer;
						buffer.command = FAB_C_CANCEL_REPAIR;
						NET_METRIC(sizeof(buffer));
						THEMATRIX->SendOperationData(workingID,dwMissionID,&buffer,sizeof(buffer));
						mode = FAB_IDLE;
						COMP_OP(dwMissionID);
					}
				}
				else
				{
					BaseFabCommand buffer;
					buffer.command = FAB_C_CANCEL_REPAIR;
					NET_METRIC(sizeof(buffer));
					THEMATRIX->SendOperationData(workingID,dwMissionID,&buffer,sizeof(buffer));
					mode = FAB_IDLE;
					COMP_OP(dwMissionID);
				}
				break;
			}
		case FAB_MOVING_TO_READY_TARGET_REPAIR_CLIENT:
			{
				mode = FAB_MOVING_TO_READY_TARGET_REPAIR_HOST;
				break;
			}
		case FAB_WAIT_DISM_INFO_CLIENT:
			{
				IBaseObject * dismantleTarget = OBJLIST->FindObject(workTargID);
				if(dismantleTarget)
				{
					dismantleTarget->QueryInterface(IBaseObjectID,workTarg,SYSVOLATILEPTR);
					workTargID = dismantleTarget->GetPartID();
					//setting these for move code
					VOLPTR(IPlatform) plat=dismantleTarget;
					targetPlanetID = plat->GetPlanetID();
					if (targetPlanetID)
					{
						targetSlotID = plat->GetSlotID();
						FabCommandW2Handle buffer;
						buffer.command = FAB_C_DISM_BEG_PLANET;
						buffer.handle1 = targetPlanetID;
						buffer.handle2 = targetSlotID;
						THEMATRIX->SendOperationData(workingID,dwMissionID,&buffer,sizeof(buffer));
					}
					else
					{
						targetPosition = dismantleTarget->GetGridPosition();
						FabCommandWGridVect buffer;
						buffer.command = FAB_C_DISM_BEG_POSITION;
						buffer.grid = targetPosition;
						THEMATRIX->SendOperationData(workingID,dwMissionID,&buffer,sizeof(buffer));
					}

					//sent begining info to clients
					mode = FAB_MOVING_TO_TARGET_DISM;
					BeginMove();
				}
				else
				{
					BaseFabCommand buffer;
					buffer.command = FAB_C_CANCEL_DISM;
					NET_METRIC(sizeof(buffer));
					THEMATRIX->SendOperationData(workingID,dwMissionID,&buffer,sizeof(buffer));
					mode = FAB_IDLE;
					for(U32 count = 0; count<numDrones; ++count)
					{
						if(drone[count])
							drone[count]->Return();
					}
					COMP_OP(dwMissionID);
				}
				break;
			}
		case FAB_AT_TARGET_DISM_CLIENT:
			{
				BaseFabCommand buffer;
				buffer.command = FAB_C_AT_TARGET_DISM;
				NET_METRIC(sizeof(buffer));
				THEMATRIX->SendOperationData(workingID,dwMissionID,&buffer,sizeof(buffer));	
				if(!targetPlanetID)
				{
					COMPTR<ITerrainMap> map;
					SECTOR->GetTerrainMap(systemID,map);
					FootprintInfo footprint;
					footprint.missionID = dwMissionID|0x10000000;
					footprint.height =box[2];
					footprint.flags = TERRAIN_DESTINATION | TERRAIN_HALFSQUARE;
					map->UndoFootprint(&targetPosition,1,footprint);
				}
				//no break;
			}
		case FAB_WATING_TO_START_DISM_CLIENT:
			{
				if(workTarg)
				{
					OBJPTR<IBuild> buildTarg;
					workTarg->QueryInterface(IBuildID,buildTarg);
					MPart repPart(workTarg);
					SINGLE dist = GetGridPosition()-workTarg->GetGridPosition();
					if(dist <= sensorRadius && repPart->bReady && buildTarg->IsComplete())
					{
						OBJPTR<IMissionActor> actor;
						workTarg->QueryInterface(IMissionActorID,actor);
						actor->PrepareTakeover(workTargID, 0);
						if(!THEMATRIX->GetWorkingOp(workTarg))
						{
								THEMATRIX->FlushOpQueueForUnit(workTarg);

								FabCommandW2Handle buffer;
								buffer.command = FAB_C_BEGIN_DISM;

								workTarg->SetReady(false);
								OBJLIST->UnselectObject(workTarg);

								THEMATRIX->AddObjectToOperation(workingID,workTargID);

								OBJPTR<IPlatform> platform;
								workTarg->QueryInterface(IPlatformID,platform);
								if(platform->IsJumpPlatform())
								{
									OBJPTR<IJumpPlat> jplat;
									workTarg->QueryInterface(IJumpPlatID,jplat);
									targetPlanetID = jplat->GetJumpGate()->GetPartID();
									buffer.handle1 = targetPlanetID;
									buffer.handle2 = 0;
									THEMATRIX->AddObjectToOperation(workingID,jplat->GetJumpGate()->GetPartID());
									THEMATRIX->AddObjectToOperation(workingID,SECTOR->GetJumpgateDestination(jplat->GetJumpGate())->GetPartID());
								}
								else if(platform->IsDeepSpacePlatform())
								{
									targetPlanetID = 0;
									buffer.handle1 = 0;
									buffer.handle2 = 0;
								}
								else
								{
									targetPlanetID = platform->GetPlanetID();
									targetSlotID = platform->GetSlotID();
									OBJPTR<IPlanet> planet;
									OBJLIST->FindObject(targetPlanetID)->QueryInterface(IPlanetID,planet);
									U32 numSlots = ((BASE_PLATFORM_DATA *)(ARCHLIST->GetArchetypeData(workTarg->pArchetype)))->slotsNeeded;
									U32 slotIDs[12];
									planet->GetPlanetSlotMissionIDs(slotIDs,targetSlotID);
									buffer.handle1 = targetPlanetID;
									buffer.handle2 = targetSlotID;
									
									for(U32 slotCount = 0; slotCount < numSlots; ++slotCount)
									{
										THEMATRIX->AddObjectToOperation(workingID,slotIDs[slotCount]);
									}
								}

								NET_METRIC(sizeof(buffer));
								THEMATRIX->SendOperationData(workingID,dwMissionID,&buffer,sizeof(buffer));
								THEMATRIX->SetCancelState(workingID,false);

								mode = FAB_DISMING;

								OBJPTR<IBuild> targBuildee;
								workTarg->QueryInterface(IBuildID,targBuildee);
								FabStartDismantle(targBuildee);		
						}
						else
						{
							BaseFabCommand buffer;
							buffer.command = FAB_C_CANCEL_DISM;
							NET_METRIC(sizeof(buffer));
							THEMATRIX->SendOperationData(workingID,dwMissionID,&buffer,sizeof(buffer));
							mode = FAB_IDLE;
							for(U32 count = 0; count<numDrones; ++count)
							{
								if(drone[count])
									drone[count]->Return();
							}
							COMP_OP(dwMissionID);
						}
					}
					else
					{
						BaseFabCommand buffer;
						buffer.command = FAB_C_CANCEL_DISM;
						NET_METRIC(sizeof(buffer));
						THEMATRIX->SendOperationData(workingID,dwMissionID,&buffer,sizeof(buffer));
						mode = FAB_IDLE;
						for(U32 count = 0; count<numDrones; ++count)
						{
							if(drone[count])
								drone[count]->Return();
						}
						COMP_OP(dwMissionID);
					}
				}
				else
				{
					BaseFabCommand buffer;
					buffer.command = FAB_C_CANCEL_DISM;
					NET_METRIC(sizeof(buffer));
					THEMATRIX->SendOperationData(workingID,dwMissionID,&buffer,sizeof(buffer));
					mode = FAB_IDLE;
					for(U32 count = 0; count<numDrones; ++count)
					{
						if(drone[count])
							drone[count]->Return();
					}
					COMP_OP(dwMissionID);
				}
				break;
			}
		case FAB_MOVING_TO_READY_TARGET_DISM_CLIENT:
			{
				mode = FAB_MOVING_TO_READY_TARGET_DISM_HOST;
				break;
			}
		default:
			CQASSERT(0 && "Bad migration mode");
		}
	}else
	{
		switch(mode)
		{
		case FAB_IDLE:
		case FAB_MOVING_TO_TARGET:
		case FAB_WAITING_INIT_CONS_CLIENT:
		case FAB_AT_TARGET_CLIENT:
		case FAB_WATING_TO_START_BUILD_CLIENT:
		case FAB_MOVING_TO_READY_TARGET_CLIENT:
		case FAB_BUILDING:
		case FAB_UNBUILD:
		case FAB_EXPLODING:
		case FAB_MOVING_TO_TARGET_REPAIR:
		case FAB_WAIT_REPAIR_INFO_CLIENT:
		case FAB_AT_TARGET_REPAIR_CLIENT:
		case FAB_MOVING_TO_READY_TARGET_REPAIR_CLIENT:
		case FAB_WATING_TO_START_REPAIR_CLIENT:
		case FAB_REPAIRING:
		case FAB_MOVING_TO_TARGET_DISM:
		case FAB_WAIT_DISM_INFO_CLIENT:
		case FAB_AT_TARGET_DISM_CLIENT:
		case FAB_MOVING_TO_READY_TARGET_DISM_CLIENT:
		case FAB_WATING_TO_START_DISM_CLIENT:
		case FAB_DISMING:
			{
				//do nothing
				break;
			}
		case FAB_MOVING_TO_READY_TARGET_HOST:
			{
				mode = FAB_MOVING_TO_READY_TARGET_CLIENT;
				break;
			}
		case FAB_MOVING_TO_READY_TARGET_REPAIR_HOST:
			{
				mode = FAB_MOVING_TO_READY_TARGET_REPAIR_CLIENT;
				break;
			}
		case FAB_MOVING_TO_READY_TARGET_DISM_HOST:
			{
				mode = FAB_MOVING_TO_READY_TARGET_DISM_CLIENT;
			}
		default:
			CQASSERT(0 && "Bad migration mode");
		}
	}
}
//---------------------------------------------------------------------------
//
void Fabricator::BuildQueue (U32 dwArchetypeDataID, COMMANDS command)
{
/*	if(HOST_CHECK && command == USRBUILD::REMOVE && workingID && dwArchetypeDataID == buildingID)
	{
		if(mode == FAB_BUILDING)
		{
			if(buildee)
			{
				if(buildee->NumFabOnBuild() > 1)
				{
					CQASSERT(0 && "Not Allowed");
				}
				else
				{
					buildee->CancelBuild();//need to start revese non host first

					BaseFabCommand buffer;
					buffer.command = FAB_C_REVERSE_BUILD;
					NET_METRIC(sizeof(buffer));
					THEMATRIX->SendOperationData(workingID,GetPartID(),&buffer,sizeof(buffer));
					mode = FAB_UNBUILD;
				}	
			}
		}
		else if(mode != FAB_UNBUILD)
		{
			BaseFabCommand buffer;
			buffer.command = FAB_C_CANCEL_BUILD;
			NET_METRIC(sizeof(buffer));
			THEMATRIX->SendOperationData(workingID,dwMissionID,&buffer,sizeof(buffer));
			mode = FAB_IDLE;
			BANKER->AddResource(playerID,workingCost);
			COMP_OP(dwMissionID);
		}
	}*/
}
//---------------------------------------------------------------------------
//
U32 Fabricator::GetFabJobID ()
{
	if(bBuilding)
	{
		return buildingID;
	}
	return 0;
}
//---------------------------------------------------------------------------
//
bool Fabricator::IsBuildingAgent (U32 agentID)
{
	if((agentID == workingID) && (mode == FAB_BUILDING || mode == FAB_UNBUILD))
		return true;
	return false;
}
//---------------------------------------------------------------------------
//
U8 Fabricator::GetFabTab()
{
	return lastTab;
}
//---------------------------------------------------------------------------
//
void Fabricator::SetFabTab(U8 tab)
{
	lastTab = tab;
}
//---------------------------------------------------------------------------
//
void Fabricator::ParkYourself (const TRANSFORM & _transform, U32 planetID, U32 slotID)
{
	dir = -_transform.get_k();
//	GotoPosition(transform.translation-450*dir);
	disableAutoMovement();		// turn off rocking
}
/*const TRANSFORM & Fabricator::GetDroneTransform ()
{
	return transform;
}*/
//---------------------------------------------------------------------------
//

BOOL32 Fabricator::updateFabricate (void)
{
	if(!bExploding && bReady && (mode != FAB_EXPLODING))
	{
		TECHNODE currentNode = MGlobals::GetCurrentTechLevel(this->GetPlayerID());
		if(!(currentNode.HasSomeTech(M_TERRAN,0,0,TECHTREE::RESERVED_FABRICATOR,0,0,0)))
		{
			currentNode.race[race].common = (TECHTREE::COMMON)(((U32)currentNode.race[race].common) | TECHTREE::RESERVED_FABRICATOR);
			MGlobals::SetCurrentTechLevel(currentNode,this->GetPlayerID());
		}
	}

	if(HOST_CHECK)
	{
		CQASSERT((mode == FAB_IDLE) ||
			(mode == FAB_MOVING_TO_TARGET) ||
			(mode == FAB_BUILDING) ||
			(mode == FAB_UNBUILD) ||
			(mode == FAB_EXPLODING) ||
			(mode == FAB_MOVING_TO_TARGET_REPAIR) ||
			(mode == FAB_MOVING_TO_READY_TARGET_HOST) ||
			(mode == FAB_MOVING_TO_READY_TARGET_REPAIR_HOST) ||
			(mode == FAB_REPAIRING) ||
			(mode == FAB_MOVING_TO_TARGET_DISM)||
			(mode == FAB_MOVING_TO_READY_TARGET_DISM_HOST) ||
			(mode == FAB_DISMING));
		switch(mode)
		{
		case FAB_MOVING_TO_READY_TARGET_DISM_HOST:
		case FAB_MOVING_TO_TARGET_DISM:
			{
				if(moveToTarget())
				{
					if(mode != FAB_MOVING_TO_READY_TARGET_DISM_HOST)
					{
						BaseFabCommand buffer;
						buffer.command = FAB_C_AT_TARGET_DISM;
						NET_METRIC(sizeof(buffer));
						THEMATRIX->SendOperationData(workingID,dwMissionID,&buffer,sizeof(buffer));	
						if(!targetPlanetID)
						{
							COMPTR<ITerrainMap> map;
							SECTOR->GetTerrainMap(systemID,map);
							FootprintInfo footprint;
							footprint.missionID = dwMissionID|0x10000000;
							footprint.height =box[2];
							footprint.flags = TERRAIN_DESTINATION | TERRAIN_HALFSQUARE;
							map->UndoFootprint(&targetPosition,1,footprint);
						}
					}
					if(workTarg)
					{
						OBJPTR<IBuild> buildTarg;
						workTarg->QueryInterface(IBuildID,buildTarg);
						MPart repPart(workTarg);
						SINGLE dist = GetGridPosition()-workTarg->GetGridPosition();
						if(dist <= sensorRadius && repPart->bReady && buildTarg->IsComplete())
						{
							OBJPTR<IMissionActor> actor;
							workTarg->QueryInterface(IMissionActorID,actor);
							actor->PrepareTakeover(workTargID, 0);
							if(!THEMATRIX->GetWorkingOp(workTarg))
							{
								THEMATRIX->FlushOpQueueForUnit(workTarg);

								FabCommandW2Handle buffer;
								buffer.command = FAB_C_BEGIN_DISM;

								workTarg->SetReady(false);
								OBJLIST->UnselectObject(workTarg);

								THEMATRIX->AddObjectToOperation(workingID,workTargID);

								OBJPTR<IPlatform> platform;
								workTarg->QueryInterface(IPlatformID,platform);
								if(platform->IsJumpPlatform())
								{
									OBJPTR<IJumpPlat> jplat;
									workTarg->QueryInterface(IJumpPlatID,jplat);
									targetPlanetID = jplat->GetJumpGate()->GetPartID();
									buffer.handle1 = targetPlanetID;
									buffer.handle2 = 0;
									THEMATRIX->AddObjectToOperation(workingID,jplat->GetJumpGate()->GetPartID());
									THEMATRIX->AddObjectToOperation(workingID,SECTOR->GetJumpgateDestination(jplat->GetJumpGate())->GetPartID());
								}
								else if(platform->IsDeepSpacePlatform())
								{
									targetPlanetID = 0;
									buffer.handle1 = 0;
									buffer.handle2 = 0;
								}
								else
								{
									targetPlanetID = platform->GetPlanetID();
									targetSlotID = platform->GetSlotID();
									OBJPTR<IPlanet> planet;
									OBJLIST->FindObject(targetPlanetID)->QueryInterface(IPlanetID,planet);
									U32 numSlots = ((BASE_PLATFORM_DATA *)(ARCHLIST->GetArchetypeData(workTarg->pArchetype)))->slotsNeeded;
									U32 slotIDs[12];
									planet->GetPlanetSlotMissionIDs(slotIDs,targetSlotID);
									buffer.handle1 = targetPlanetID;
									buffer.handle2 = targetSlotID;
									
									for(U32 slotCount = 0; slotCount < numSlots; ++slotCount)
									{
										THEMATRIX->AddObjectToOperation(workingID,slotIDs[slotCount]);
									}
								}

								
								NET_METRIC(sizeof(buffer));
								THEMATRIX->SendOperationData(workingID,dwMissionID,&buffer,sizeof(buffer));
								THEMATRIX->SetCancelState(workingID,false);

								mode = FAB_DISMING;

								OBJPTR<IBuild> targBuildee;
								workTarg->QueryInterface(IBuildID,targBuildee);
								FabStartDismantle(targBuildee);
								fabStartAnims();
							}
							else
							{
								BaseFabCommand buffer;
								buffer.command = FAB_C_CANCEL_DISM;
								NET_METRIC(sizeof(buffer));
								THEMATRIX->SendOperationData(workingID,dwMissionID,&buffer,sizeof(buffer));
								mode = FAB_IDLE;
								for(U32 count = 0; count<numDrones; ++count)
								{
									if(drone[count])
										drone[count]->Return();
								}
								COMP_OP(dwMissionID);
							}
						}
						else
						{
							BaseFabCommand buffer;
							buffer.command = FAB_C_CANCEL_DISM;
							NET_METRIC(sizeof(buffer));
							THEMATRIX->SendOperationData(workingID,dwMissionID,&buffer,sizeof(buffer));
							mode = FAB_IDLE;
							for(U32 count = 0; count<numDrones; ++count)
							{
								if(drone[count])
									drone[count]->Return();
							}
							COMP_OP(dwMissionID);
						}
					}
					else
					{
						BaseFabCommand buffer;
						buffer.command = FAB_C_CANCEL_DISM;
						NET_METRIC(sizeof(buffer));
						THEMATRIX->SendOperationData(workingID,dwMissionID,&buffer,sizeof(buffer));
						mode = FAB_IDLE;
						for(U32 count = 0; count<numDrones; ++count)
						{
							if(drone[count])
								drone[count]->Return();
						}
						COMP_OP(dwMissionID);
					}
				}
				break;
			}
		case FAB_DISMING:
			{
				if(workTarg)
				{
					U32 stallType;
					if(FabGetProgress(stallType) >= 1.0)
					{
						BaseFabCommand buffer;
						buffer.command = FAB_C_DISM_FINISHED;
						NET_METRIC(sizeof(buffer));
						THEMATRIX->SendOperationData(workingID,dwMissionID,&buffer,sizeof(buffer));

						if(workTarg)//remove the workTarg from the operation we placed him in
						{
							ResourceCost cost = ((BASE_PLATFORM_DATA *)(ARCHLIST->GetArchetypeData(workTarg->pArchetype)))->missionData.resourceCost;
							cost.gas = cost.gas/2;
							cost.crew = cost.crew/2;
							cost.metal = cost.metal/2;
							BANKER->AddResource(playerID,cost);
//							BANKER->FreeCommandPt(playerID,cost.commandPt);

							OBJPTR<IBaseObject> obj;
							workTarg->QueryInterface(IBaseObjectID,obj);
							THEMATRIX->OperationCompleted(workingID,obj->GetPartID());	

							if(targetPlanetID)
							{
								IBaseObject * targ = OBJLIST->FindObject(targetPlanetID);
								OBJPTR<IPlanet> planet;
								targ->QueryInterface(IPlanetID,planet);
								if(planet)
								{
									planet->CompleteSlotOperations(workingID,targetSlotID);
								}
								else
								{
									THEMATRIX->OperationCompleted(workingID,targ->GetPartID());
									THEMATRIX->OperationCompleted(workingID,SECTOR->GetJumpgateDestination(targ)->GetPartID());
								}
							}

							OBJPTR<IPlatform> plat;
							workTarg->QueryInterface(IPlatformID,plat);
							plat->FreePlanetSlot();
							THEMATRIX->OperationCompleted(workingID,workTargID);
						}
						
						FabEnableCompletion();

						mode = FAB_IDLE;
						COMP_OP(dwMissionID);
					}
				}
				else //the buildee died or something
				{
					BaseFabCommand buffer;
					buffer.command = FAB_C_STOP_DISM;
					NET_METRIC(sizeof(buffer));
					THEMATRIX->SendOperationData(workingID,dwMissionID,&buffer,sizeof(buffer));

					for(U32 count = 0; count<numDrones; ++count)
					{
						if(drone[count])
							drone[count]->Return();
					}
					mode = FAB_IDLE;

					if(targetPlanetID)
					{
						IBaseObject * targ = OBJLIST->FindObject(targetPlanetID);
						OBJPTR<IPlanet> planet;
						targ->QueryInterface(IPlanetID,planet);
						if(planet)
						{
							planet->CompleteSlotOperations(workingID,targetSlotID);
						}
						else
						{
							THEMATRIX->OperationCompleted(workingID,targ->GetPartID());
							THEMATRIX->OperationCompleted(workingID,SECTOR->GetJumpgateDestination(targ)->GetPartID());
						}
					}
					
					COMP_OP(dwMissionID);
				}
				break;
			}
		case FAB_MOVING_TO_READY_TARGET_REPAIR_HOST:
		case FAB_MOVING_TO_TARGET_REPAIR:
			{
				if(moveToTarget())
				{
					if(mode != FAB_MOVING_TO_READY_TARGET_REPAIR_HOST)
					{
						BaseFabCommand buffer;
						buffer.command = FAB_C_AT_TARGET_REPAIR;
						NET_METRIC(sizeof(buffer));
						THEMATRIX->SendOperationData(workingID,dwMissionID,&buffer,sizeof(buffer));	
						if(!targetPlanetID)
						{
							COMPTR<ITerrainMap> map;
							SECTOR->GetTerrainMap(systemID,map);
							FootprintInfo footprint;
							footprint.missionID = dwMissionID|0x10000000;
							footprint.height =box[2];
							footprint.flags = TERRAIN_DESTINATION | TERRAIN_HALFSQUARE;
							map->UndoFootprint(&targetPosition,1,footprint);
						}
					}
					if(workTarg)
					{
						OBJPTR<IBuild> buildTarg;
						workTarg->QueryInterface(IBuildID,buildTarg);
						MPart repPart(workTarg);
						SINGLE dist = workTarg->GetGridPosition()-GetGridPosition();
						if(dist <= sensorRadius)
						{
							if(repPart->bReady && buildTarg->IsComplete())
							{
								MPart part(workTarg);
								ResourceCost cost;
								cost.commandPt = 0;
								cost.crew = (part.pInit->resourceCost.crew*(part->hullPointsMax - part->hullPoints))/(part->hullPointsMax*3);
								cost.metal = (part.pInit->resourceCost.metal*(part->hullPointsMax - part->hullPoints))/(part->hullPointsMax*3);
								cost.gas = (part.pInit->resourceCost.gas*(part->hullPointsMax - part->hullPoints))/(part->hullPointsMax*3);
								M_RESOURCE_TYPE failType;
								if(BANKER->SpendMoney(playerID,cost,&failType))
								{

									BaseFabCommand buffer;
									buffer.command = FAB_C_BEGIN_REPAIR;

									NET_METRIC(sizeof(buffer));
									THEMATRIX->SendOperationData(workingID,dwMissionID,&buffer,sizeof(buffer));
									mode = FAB_REPAIRING;

									oldHullPoints = repPart->hullPoints;

									Vector pos,dir;
									OBJPTR<IShipDamage> shipDamage;
									workTarg->QueryInterface(IShipDamageID,shipDamage);
									CQASSERT(shipDamage);
									shipDamage->GetNextDamageSpot(pos,dir);
									if(numDrones)
									{
										if(drone[curDrone])
										{
											VOLPTR(IShuttle) shuttle;
											shuttle = drone[curDrone];
											shuttle->WorkAtShipPos(workTarg,pos,dir,300);
										}
										curDrone = (curDrone+1)%numDrones;
									}
								}
								else
								{
									failSound(failType);
									FabCommandWHandle buffer;
									buffer.command = FAB_C_CANCEL_REPAIR_NOMONEY;
									buffer.handle = failType;
									NET_METRIC(sizeof(buffer));
									THEMATRIX->SendOperationData(workingID,dwMissionID,&buffer,sizeof(buffer));
									mode = FAB_IDLE;
									COMP_OP(dwMissionID);
								}
							}
							else
							{
								BaseFabCommand buffer;
								buffer.command = FAB_C_CANCEL_REPAIR;
								NET_METRIC(sizeof(buffer));
								THEMATRIX->SendOperationData(workingID,dwMissionID,&buffer,sizeof(buffer));
								mode = FAB_IDLE;
								COMP_OP(dwMissionID);
							}
						}
						else
						{
							BaseFabCommand buffer;
							buffer.command = FAB_C_CANCEL_REPAIR;
							NET_METRIC(sizeof(buffer));
							THEMATRIX->SendOperationData(workingID,dwMissionID,&buffer,sizeof(buffer));
							mode = FAB_IDLE;
							COMP_OP(dwMissionID);
						}
					}
					else
					{
						BaseFabCommand buffer;
						buffer.command = FAB_C_CANCEL_REPAIR;
						NET_METRIC(sizeof(buffer));
						THEMATRIX->SendOperationData(workingID,dwMissionID,&buffer,sizeof(buffer));
						mode = FAB_IDLE;
						COMP_OP(dwMissionID);
					}
				}
				break;
			}
		case FAB_REPAIRING:
			{
				if(HOST_CHECK)
				{
					if(workTarg)
					{
						MPartNC part(workTarg);
						U32 newHullPoints = part->hullPoints + (repairRate/REALTIME_FRAMERATE);
						if(newHullPoints > part->hullPointsMax)
							newHullPoints = part->hullPointsMax;
						part->hullPoints = newHullPoints;
						if(workTarg->objClass == OC_PLATFORM)
						{
							OBJPTR<IJumpPlat> plat;
							workTarg->QueryInterface(IJumpPlatID,plat);
							if(plat)
							{
								plat->SetSiblingHull(newHullPoints);
							}
						}
						if((((SINGLE)(newHullPoints-oldHullPoints))/part->hullPointsMax) >= 0.12f)
						{
							oldHullPoints = newHullPoints;
							OBJPTR<IShipDamage> shipDamage;
							workTarg->QueryInterface(IShipDamageID,shipDamage);
							CQASSERT(shipDamage);
							shipDamage->FixDamageSpot();
							Vector pos,dir;
							shipDamage->GetNextDamageSpot(pos,dir);
							if(numDrones)
							{
								if(drone[curDrone])
								{
									VOLPTR(IShuttle) shuttle;
									shuttle = drone[curDrone];
									shuttle->WorkAtShipPos(workTarg,pos,dir,300);
								}
								curDrone = (curDrone+1)%numDrones;
							}
						}
						if(newHullPoints == part->hullPointsMax)
						{
							BaseFabCommand buffer;
							buffer.command = FAB_C_REPAIR_FINISHED;
							NET_METRIC(sizeof(buffer));
							THEMATRIX->SendOperationData(workingID,dwMissionID,&buffer,sizeof(buffer));

							for(U32 count = 0; count<numDrones; ++count)
							{
								if(drone[count])
									drone[count]->Return();
							}

							mode = FAB_IDLE;
							COMP_OP(dwMissionID);
						}
					}
					else
					{
						BaseFabCommand buffer;
						buffer.command = FAB_C_CANCEL_REPAIR;
						NET_METRIC(sizeof(buffer));
						THEMATRIX->SendOperationData(workingID,dwMissionID,&buffer,sizeof(buffer));

						for(U32 count = 0; count<numDrones; ++count)
						{
							if(drone[count])
								drone[count]->Return();
						}
						mode = FAB_IDLE;
						COMP_OP(dwMissionID);
					}				
				}
				break;
			}
		case FAB_MOVING_TO_TARGET:
		case FAB_MOVING_TO_READY_TARGET_HOST:
			{
				if (moveToTarget())  //ship has reached target
				{
					if(mode != FAB_MOVING_TO_READY_TARGET_HOST) //if I am moving to a target
					{
						BaseFabCommand buffer;
						buffer.command = FAB_C_AT_TARGET;
						NET_METRIC(sizeof(buffer));
						THEMATRIX->SendOperationData(workingID,dwMissionID,&buffer,sizeof(buffer));	
						if(!targetPlanetID)
						{
							COMPTR<ITerrainMap> map;
							SECTOR->GetTerrainMap(systemID,map);
							FootprintInfo footprint;
							footprint.missionID = dwMissionID|0x10000000;
							footprint.height =box[2];
							footprint.flags = TERRAIN_DESTINATION | TERRAIN_HALFSQUARE;
							map->UndoFootprint(&targetPosition,1,footprint);
						}
						
					}
					
					if(targetPlanetID)  //I'm building on an object
					{
						//find the kind of operation I want to create
						//see if slot is taken
						IBaseObject * obj = OBJLIST->FindObject(targetPlanetID);
						CQASSERT(obj);
						if(obj->objClass == OC_JUMPGATE)
						{
							OBJPTR<IJumpGate> jumpgate;
							obj->QueryInterface(IJumpGateID,jumpgate);
							SINGLE range = GetGridPosition()-obj->GetGridPosition();
							if((jumpgate->GetPlayerOwner() != 0) || range > (sensorRadius*2))
							{
								FABRICATORCOMM(buildImposible, SUB_NO_BUILD);
								BaseFabCommand buffer;
								buffer.command = FAB_C_BUILD_IMPOSIBLE;
								NET_METRIC(sizeof(buffer));
								THEMATRIX->SendOperationData(workingID,dwMissionID,&buffer,sizeof(buffer));
								mode = FAB_IDLE;
								BANKER->AddResource(playerID,workingCost);
								COMP_OP(dwMissionID);
								EVENTSYS->Post(CQE_FABRICATOR_FINISHED,(void *)dwMissionID);
							}
							else
							{
								FabCommandWHandle buffer;
								buffer.command = FAB_C_BUILD_PLATFORM;
								buffer.handle = MGlobals::CreateNewPartID(playerID);
								THEMATRIX->AddObjectToOperation(workingID,buffer.handle);
								THEMATRIX->AddObjectToOperation(workingID,buffer.handle|0x10000000);
								THEMATRIX->AddObjectToOperation(workingID,obj->GetPartID());
								THEMATRIX->AddObjectToOperation(workingID,SECTOR->GetJumpgateDestination(obj)->GetPartID());
								NET_METRIC(sizeof(buffer));
								THEMATRIX->SendOperationData(workingID,dwMissionID,&buffer,sizeof(buffer));
								THEMATRIX->SetCancelState(workingID,false);
								
								IBaseObject * newPlat = MGlobals::CreateInstance(ARCHLIST->LoadArchetype(buildingID),buffer.handle);
								OBJPTR<IPlatform> platformPtr;
								newPlat->QueryInterface(IPlatformID,platformPtr);
								
								OBJLIST->AddObject(newPlat);
								OBJPTR<IPhysicalObject> miss;
								if (newPlat->QueryInterface(IPhysicalObjectID,miss))
								{
									miss->SetSystemID(GetSystemID());
								}
								platformPtr->ParkYourself(obj);
								ENGINE->update_instance(newPlat->GetObjectIndex(),0,0);
								OBJPTR<IBuild> targBuildee;
								newPlat->QueryInterface(IBuildID,targBuildee);
								targBuildee->SetProcessID(workingID);
								FabStartBuild(targBuildee);
								fabStartAnims();
								mode = FAB_BUILDING;
							}
						}
						else//if it's not a jumpgate, it is a planet
						{
							OBJPTR<IPlanet> planet;
							obj->QueryInterface(IPlanetID,planet);
							CQASSERT(planet != 0);
							
							SINGLE range = GetGridPosition()-obj->GetGridPosition();
							U32 slotHolder = planet->GetSlotUser(targetSlotID);
							if(slotHolder || (!(planet->IsBuildableBy(playerID))) || range > (sensorRadius*2))
							{
								FABRICATORCOMM(buildImposible, SUB_NO_BUILD);
								BaseFabCommand buffer;
								buffer.command = FAB_C_BUILD_IMPOSIBLE;
								NET_METRIC(sizeof(buffer));
								THEMATRIX->SendOperationData(workingID,dwMissionID,&buffer,sizeof(buffer));
								mode = FAB_IDLE;
								BANKER->AddResource(playerID,workingCost);
								COMP_OP(dwMissionID);
								EVENTSYS->Post(CQE_FABRICATOR_FINISHED,(void *)dwMissionID);
							}
							else //slot empty
							{
								//begin Construction Operation
								FabCommandWHandle buffer;
								buffer.command = FAB_C_BUILD_PLATFORM;
								buffer.handle = MGlobals::CreateNewPartID(playerID);
								THEMATRIX->AddObjectToOperation(workingID,buffer.handle);
								U32 numSlots = ((BASE_PLATFORM_DATA *)(ARCHLIST->GetArchetypeData(buildingID)))->slotsNeeded;
								U32 slotIDs[12];
								planet->GetPlanetSlotMissionIDs(slotIDs,targetSlotID);
								
								for(U32 slotCount = 0; slotCount < numSlots; ++slotCount)
								{
									THEMATRIX->AddObjectToOperation(workingID,slotIDs[slotCount]);
								}
								
								NET_METRIC(sizeof(buffer));
								THEMATRIX->SendOperationData(workingID,dwMissionID,&buffer,sizeof(buffer));
								THEMATRIX->SetCancelState(workingID,false);
								
								IBaseObject * newPlat = MGlobals::CreateInstance(ARCHLIST->LoadArchetype(buildingID),buffer.handle);
								OBJPTR<IPlatform> platformPtr;
								newPlat->QueryInterface(IPlatformID,platformPtr);
								
								OBJLIST->AddObject(newPlat);
								OBJPTR<IPhysicalObject> miss;
								if (newPlat->QueryInterface(IPhysicalObjectID,miss))
								{
									miss->SetSystemID(GetSystemID());
								}
								platformPtr->ParkYourself(planet->GetSlotTransform(targetSlotID),targetPlanetID,targetSlotID);
								ENGINE->update_instance(newPlat->GetObjectIndex(),0,0);
								OBJPTR<IBuild> targBuildee;
								newPlat->QueryInterface(IBuildID,targBuildee);
								targBuildee->SetProcessID(workingID);
								FabStartBuild(targBuildee);
								fabStartAnims();
								mode = FAB_BUILDING;
							}
						}
						}
						else  //I'm building in open space
						{
							//begin Construction Operation
							SINGLE range = GetGridPosition()-targetPosition;
							COMPTR<ITerrainMap> map;
							SECTOR->GetTerrainMap(systemID,map);
							if((range > (sensorRadius*2)) || (!(map->IsGridEmpty(targetPosition,0,false))))
							{
								FABRICATORCOMM(buildImposible, SUB_NO_BUILD);
								BaseFabCommand buffer;
								buffer.command = FAB_C_BUILD_IMPOSIBLE;
								NET_METRIC(sizeof(buffer));
								THEMATRIX->SendOperationData(workingID,dwMissionID,&buffer,sizeof(buffer));
								mode = FAB_IDLE;
								BANKER->AddResource(playerID,workingCost);
								COMP_OP(dwMissionID);
								EVENTSYS->Post(CQE_FABRICATOR_FINISHED,(void *)dwMissionID);
							}
							else
							{
								FabCommandWHandle buffer;
								buffer.command = FAB_C_BUILD_PLATFORM;
								buffer.handle = MGlobals::CreateNewPartID(playerID);
								THEMATRIX->AddObjectToOperation(workingID,buffer.handle);
								THEMATRIX->SendOperationData(workingID,dwMissionID,&buffer,sizeof(buffer));
								THEMATRIX->SetCancelState(workingID,false);
								
								IBaseObject * newPlat = MGlobals::CreateInstance(ARCHLIST->LoadArchetype(buildingID),buffer.handle);
								OBJPTR<IPlatform> platformPtr;
								newPlat->QueryInterface(IPlatformID,platformPtr);
								
								OBJLIST->AddObject(newPlat);
								VOLPTR(IPhysicalObject) miss;
								if ((miss = newPlat) != 0)
								{
									if(!platformPtr->IsHalfSquare())
										targetPosition.centerpos();
									miss->SetPosition(targetPosition, systemID);
								}
								ENGINE->update_instance(newPlat->GetObjectIndex(),0,0);
								OBJPTR<IBuild> targBuildee;
								newPlat->QueryInterface(IBuildID,targBuildee);
								targBuildee->SetProcessID(workingID);
								FabStartBuild(targBuildee);
								fabStartAnims();
								mode = FAB_BUILDING;
							}
						}
					}
					
					break;
			}
		case FAB_BUILDING:
		case FAB_UNBUILD:
			{
				moveTo(transform.translation);
				rotateShip(0,0,0);
				if(buildee)
				{
					U32 stallType;
					if(FabGetProgress(stallType) >= 1.0)
					{
						BaseFabCommand buffer;
						buffer.command = FAB_C_BUILD_FINISH;
						NET_METRIC(sizeof(buffer));
						THEMATRIX->SendOperationData(workingID,dwMissionID,&buffer,sizeof(buffer));
						
						if(buildee->NumFabOnBuild() == 1)
						{
							if(targetPlanetID)
							{
								IBaseObject * obj = OBJLIST->FindObject(targetPlanetID);
								if(obj->objClass == OC_JUMPGATE)
								{
									if(buildee)//remove the buildee from the operation we placed him in
									{
										OBJPTR<IBaseObject> obj;
										buildee->QueryInterface(IBaseObjectID,obj);
										THEMATRIX->OperationCompleted(workingID,obj->GetPartID());
										THEMATRIX->OperationCompleted(workingID,obj->GetPartID()|0x10000000);
										if(mode == FAB_UNBUILD)
										{
											BANKER->AddResource(playerID,workingCost);
//											BANKER->FreeCommandPt(playerID,workingCost.commandPt);
										}
									}
									
									THEMATRIX->OperationCompleted(workingID,obj->GetPartID());
									THEMATRIX->OperationCompleted(workingID,SECTOR->GetJumpgateDestination(obj)->GetPartID());
									FabEnableCompletion();
									if(mode == FAB_BUILDING)
									{
										incrementScoringPlatformsBuilt();
									}
								}
								else
								{
									if(buildee)//remove the buildee from the operation we placed him in
									{
										OBJPTR<IBaseObject> obj;
										buildee->QueryInterface(IBaseObjectID,obj);
										THEMATRIX->OperationCompleted(workingID,obj->GetPartID());	
										if(mode == FAB_UNBUILD)
										{
											OBJPTR<IPlatform> plat;
											buildee->QueryInterface(IPlatformID,plat);
											plat->FreePlanetSlot();
											BANKER->AddResource(playerID,workingCost);
//											BANKER->FreeCommandPt(playerID,workingCost.commandPt);
										}
									}
									
									OBJPTR<IPlanet> planet;
									IBaseObject * targetPlanet = OBJLIST->FindObject(targetPlanetID);
									targetPlanet->QueryInterface(IPlanetID,planet);
									planet->CompleteSlotOperations(workingID,targetSlotID);
									FabEnableCompletion();
									if(mode == FAB_BUILDING)
									{
										incrementScoringPlatformsBuilt();
									}
								}
							}
							else
							{
								if(buildee)//remove the buildee from the operation we placed him in
								{
									OBJPTR<IBaseObject> obj;
									buildee->QueryInterface(IBaseObjectID,obj);
									THEMATRIX->OperationCompleted(workingID,obj->GetPartID());	
									if(mode == FAB_UNBUILD)
									{
										BANKER->AddResource(playerID,workingCost);
//										BANKER->FreeCommandPt(playerID,workingCost.commandPt);
									}
								}
								
								FabEnableCompletion();
								if(mode == FAB_BUILDING)
								{
									incrementScoringPlatformsBuilt();
								}
							}
						}
						else
						{
							FabAssistBuildDone();
						}
						EVENTSYS->Post(CQE_FABRICATOR_FINISHED,(void *)dwMissionID);
						mode = FAB_IDLE;
						COMP_OP(dwMissionID);
						enableAutoMovement();
						SetPosition(transform.translation, systemID);
					}
				}
				else //the buildee died or something
				{
					BaseFabCommand buffer;
					buffer.command = FAB_C_STOP_BUILD;
					NET_METRIC(sizeof(buffer));
					THEMATRIX->SendOperationData(workingID,dwMissionID,&buffer,sizeof(buffer));
					if(targetPlanetID)
					{
						OBJPTR<IPlanet> planet;
						IBaseObject * targetPlanet = OBJLIST->FindObject(targetPlanetID);
						targetPlanet->QueryInterface(IPlanetID,planet);
						if(planet)
							planet->CompleteSlotOperations(workingID,targetSlotID);
						else//jumpgate
						{
							THEMATRIX->OperationCompleted(workingID,targetPlanet->GetPartID());
							THEMATRIX->OperationCompleted(workingID,SECTOR->GetJumpgateDestination(targetPlanet)->GetPartID());
						}
					}
					EVENTSYS->Post(CQE_FABRICATOR_FINISHED,(void *)dwMissionID);
					mode = FAB_IDLE;
					FabEmergencyStop();
					COMP_OP(dwMissionID);
					enableAutoMovement();
					SetPosition(transform.translation, systemID);
				}
				break;
			}
		}
	}
	else //I am the client
	{
		switch(mode)
		{
	
		case FAB_MOVING_TO_TARGET_DISM:
			{
				if(moveToTarget())
				{
					mode = FAB_AT_TARGET_DISM_CLIENT;
				}
				break;
			}
		case FAB_MOVING_TO_READY_TARGET_DISM_CLIENT:
			{
				moveToTarget();
				if(!(isMoveActive()))
				{
					mode = FAB_WATING_TO_START_DISM_CLIENT;
				}
				break;
			}

		case FAB_MOVING_TO_TARGET_REPAIR:
			{
				if(moveToTarget())
				{
					mode = FAB_AT_TARGET_REPAIR_CLIENT;
				}
				break;
			}
		case FAB_MOVING_TO_READY_TARGET_REPAIR_CLIENT:
			{
				moveToTarget();
				if(!(isMoveActive()))
				{
					mode = FAB_WATING_TO_START_REPAIR_CLIENT;
				}
				break;
			}
			
		case FAB_MOVING_TO_TARGET:
			{
				if(moveToTarget())
				{
					mode = FAB_AT_TARGET_CLIENT;
				}
				break;
			}
		case FAB_MOVING_TO_READY_TARGET_CLIENT:
			{
				moveToTarget();
				if(!(isMoveActive()))
				{
					mode = FAB_WATING_TO_START_BUILD_CLIENT;
				}
				break;
			}
		case FAB_BUILDING:
		case FAB_UNBUILD:
			if (moveToTarget())
				fabStartAnims();
			break;
		case FAB_REPAIRING:
			{
				if (moveToTarget())
					fabStartAnims();
				if(workTarg)
				{
					MPart part(workTarg);
					if((((SINGLE)(part->hullPoints-oldHullPoints))/part->hullPointsMax) >= 0.12)
					{
						oldHullPoints = part->hullPoints;
						OBJPTR<IShipDamage> shipDamage;
						workTarg->QueryInterface(IShipDamageID,shipDamage);
						CQASSERT(shipDamage);
						shipDamage->FixDamageSpot();
						Vector pos,dir;
						shipDamage->GetNextDamageSpot(pos,dir);
						if(numDrones)
						{
							if(drone[curDrone])
							{
								VOLPTR(IShuttle) shuttle;
								shuttle = drone[curDrone];
								shuttle->WorkAtShipPos(workTarg,pos,dir,300);
							}
							curDrone = (curDrone+1)%numDrones;
						}
					}
				}
				break;
			}
		}
	}


	return 1;
}
//---------------------------------------------------------------------------
//
void Fabricator::initFab (const FABRICATOR_INIT & data)
{
	repairRate = data.pData->repairRate;
	curDrone = 0;
	mode = FAB_IDLE;
	buildSoundID = data.pData->buildSound;
}
//---------------------------------------------------------------------------
//
void Fabricator::resolve (void)
{
	if(workTargID)
		OBJLIST->FindObject(workTargID,SYSVOLATILEPTR,workTarg,IBaseObjectID);
}
//---------------------------------------------------------------------------
//
void Fabricator::preTakeover (U32 newMissionID, U32 troopID)
{
	if(HOST_CHECK)
	{
		if(workingID)
		{
			if((mode == FAB_BUILDING) || (mode == FAB_UNBUILD))
			{
				BaseFabCommand buffer;
				buffer.command = FAB_C_BUILD_CANCEL_LATE;
				NET_METRIC(sizeof(buffer));
				THEMATRIX->SendOperationData(workingID,dwMissionID,&buffer,sizeof(buffer));

				if(targetPlanetID)
				{
					IBaseObject * obj = OBJLIST->FindObject(targetPlanetID);
					if(obj->objClass == OC_JUMPGATE)
					{
						if(buildee)//remove the buildee from the operation we placed him in
						{
							OBJPTR<IBaseObject> obj;
							buildee->QueryInterface(IBaseObjectID,obj);
							THEMATRIX->OperationCompleted(workingID,obj->GetPartID());
							THEMATRIX->OperationCompleted(workingID,obj->GetPartID()|0x10000000);
						}
									
						THEMATRIX->OperationCompleted(workingID,obj->GetPartID());
						THEMATRIX->OperationCompleted(workingID,SECTOR->GetJumpgateDestination(obj)->GetPartID());
						IBaseObject * targ = buildee.Ptr();			
						FabHaltBuild();
						if(targ)
							THEMATRIX->ObjectTerminated(targ->GetPartID(),0);
					}
					else
					{
						if(buildee)//remove the buildee from the operation we placed him in
						{
							OBJPTR<IBaseObject> obj;
							buildee->QueryInterface(IBaseObjectID,obj);
							THEMATRIX->OperationCompleted(workingID,obj->GetPartID());	
							if(mode == FAB_UNBUILD)
							{
								OBJPTR<IPlatform> plat;
								buildee->QueryInterface(IPlatformID,plat);
								plat->FreePlanetSlot();
							}
						}
								
						OBJPTR<IPlanet> planet;
						IBaseObject * targetPlanet = OBJLIST->FindObject(targetPlanetID);
						targetPlanet->QueryInterface(IPlanetID,planet);
						planet->CompleteSlotOperations(workingID,targetSlotID);
						IBaseObject * targ = buildee.Ptr();			
						FabHaltBuild();
						if(targ)
							THEMATRIX->ObjectTerminated(targ->GetPartID(),0);
					}
				}
				else
				{
					if(buildee)//remove the buildee from the operation we placed him in
					{
						OBJPTR<IBaseObject> obj;
						buildee->QueryInterface(IBaseObjectID,obj);
						THEMATRIX->OperationCompleted(workingID,obj->GetPartID());	
					}
					IBaseObject * targ = buildee.Ptr();			
					FabHaltBuild();
					if(targ)
						THEMATRIX->ObjectTerminated(targ->GetPartID(),0);
				}
				mode = FAB_IDLE;
				COMP_OP(dwMissionID);
				EVENTSYS->Post(CQE_FABRICATOR_FINISHED,(void *)dwMissionID);
			}
			else if(mode == FAB_MOVING_TO_TARGET)
			{
				BaseFabCommand buffer;
				buffer.command = FAB_C_BUILD_CANCEL_EARLY;
				NET_METRIC(sizeof(buffer));
				THEMATRIX->SendOperationData(workingID,dwMissionID,&buffer,sizeof(buffer));

				BANKER->AddResource(playerID,workingCost);
				if(!targetPlanetID)
				{
					COMPTR<ITerrainMap> map;
					SECTOR->GetTerrainMap(systemID,map);
					FootprintInfo footprint;
					footprint.missionID = dwMissionID|0x10000000;
					footprint.height =box[2];
					footprint.flags = TERRAIN_DESTINATION | TERRAIN_HALFSQUARE;
					map->UndoFootprint(&targetPosition,1,footprint);
				}
				mode = FAB_IDLE;
				COMP_OP(dwMissionID);
				EVENTSYS->Post(CQE_FABRICATOR_FINISHED,(void *)dwMissionID);
			}		
			else if(mode == FAB_DISMING)
			{
				BaseFabCommand buffer;
				buffer.command = FAB_C_DISM_CANCEL_LATE;
				NET_METRIC(sizeof(buffer));
				THEMATRIX->SendOperationData(workingID,dwMissionID,&buffer,sizeof(buffer));

				if(targetPlanetID)
				{
					IBaseObject * targ = OBJLIST->FindObject(targetPlanetID);
					OBJPTR<IPlanet> planet;
					targ->QueryInterface(IPlanetID,planet);
					if(planet)
					{
						planet->CompleteSlotOperations(workingID,targetSlotID);
					}
					else
					{
						THEMATRIX->OperationCompleted(workingID,targ->GetPartID());
						THEMATRIX->OperationCompleted(workingID,SECTOR->GetJumpgateDestination(targ)->GetPartID());
					}
				}

				if(workTarg)
				{
					FabHaltBuild();
					OBJPTR<IJumpPlat> jPlat;
					workTarg->QueryInterface(IJumpPlatID,jPlat);
					
					if (jPlat)
					{
						// jPlat->GetSibling can return NULL
						IBaseObject * sibling = jPlat->GetSibling();
						if (sibling)
						{
							THEMATRIX->ObjectTerminated(sibling->GetPartID(),0);
						}
					}
					THEMATRIX->ObjectTerminated(workTarg->GetPartID(),0);
				}
				COMP_OP(dwMissionID);
			}
			else if(mode == FAB_REPAIRING)
			{
				BaseFabCommand buffer;
				buffer.command = FAB_C_REPAIR_FINISHED;
				NET_METRIC(sizeof(buffer));
				THEMATRIX->SendOperationData(workingID,dwMissionID,&buffer,sizeof(buffer));

				for(U32 count = 0; count<numDrones; ++count)
				{
					if(drone[count])
						drone[count]->Return();
				}

				mode = FAB_IDLE;
				COMP_OP(dwMissionID);
			}
			else if(mode == FAB_MOVING_TO_TARGET_REPAIR)
			{
				BaseFabCommand buffer;
				buffer.command = FAB_C_CANCEL_REPAIR;
				NET_METRIC(sizeof(buffer));
				THEMATRIX->SendOperationData(workingID,dwMissionID,&buffer,sizeof(buffer));

				for(U32 count = 0; count<numDrones; ++count)
				{
					if(drone[count])
						drone[count]->Return();
				}
				mode = FAB_IDLE;
				COMP_OP(dwMissionID);
			}
			else
			{
				CQBOMB3("Fabrictor %x leaking agent %d on client, mode:%d",dwMissionID,workingID,mode);
				COMP_OP(dwMissionID);
			}
		}
	}

	for(U32 count = 0; count<numDrones; ++count)
	{
		if(drone[count])
			drone[count]->Return();
	}
	mode = FAB_IDLE;
}
//returns true if we're at our target build place
bool Fabricator::moveToTarget()
{
	if (moveStage == MS_PATHFIND)
	{
		//get planet
		IBaseObject * obj = OBJLIST->FindObject(targetPlanetID);
		Vector delta;

		VOLPTR(IPlanet) planet = obj;
		if (planet)
		{
			TRANSFORM trans;
			
			trans = planet->GetSlotTransform(targetSlotID);
			Vector delta = (trans.translation-transform.translation);
			//don't cut through the planet
			if (obj->GetSystemID() == systemID && (dot_product(delta,trans.get_k()) > 2000 && delta.magnitude() < NO_PATH_DIST) || !(isMoveActive()))
			{
				//we're close enough, beeline
				resetMoveVars();
				moveStage = MS_BEELINE;
			}
		}
		else
		{
			if (!(isMoveActive()))
			{
				//we're close enough, beeline
				moveStage = MS_BEELINE;
				if (obj)
				{
					//it's a jumpgate
					delta = transform.translation-obj->GetPosition();
					delta.normalize();
					destPos = obj->GetPosition()+delta*2000;
				}
				else
				{
					delta = transform.translation-destPos;
					delta.normalize();
					destPos = destPos+delta*2000;
				}
				
				switch (race)
				{
				case M_TERRAN:
					destYaw = PI/2+TRANSFORM::get_yaw(delta);
					if (destYaw > PI)
						destYaw -= 2*PI;
					break;
				case M_MANTIS:
					destYaw = TRANSFORM::get_yaw(-delta);
					break;
				case M_SOLARIAN:
					destYaw = TRANSFORM::get_yaw(delta);
					break;
				}
			}
		}
	}
	else if(moveStage == MS_BEELINE) //Pathfinding is over - get into position as fast as possible
	{
		CQASSERT(moveStage == MS_BEELINE);
		Vector delta = transform.translation-destPos;
		delta.z = 0;
		if(delta.magnitude() < FINAL_DIST)
		{
			SINGLE currentYaw = transform.get_yaw();
			if (delta.magnitude() < DONE_DIST && ((fabs(currentYaw-destYaw) < PI/12.0f) || (fabs(currentYaw-destYaw) > 23.0f*PI/12.0f)))
			{
				//dummy move - insufficient, must do this every frame
				//moveTo(transform.translation);
				//rotateShip(0,0,0);
				moveStage = MS_DONE;
				return true;
			}
			else
			{
				if (race == M_TERRAN)
				{
					SINGLE diff = fabs(currentYaw-destYaw);
					if (diff > PI/2 && diff < 3*PI/2)
					{
						destYaw = PI+destYaw;
						if (destYaw > PI)
							destYaw -= 2*PI;
					}
				}
				moveTo(destPos);
				rotateTo(destYaw,0,0);
			}
		}
		else
		{
			Vector delta = destPos-transform.translation;
			setPosition(delta, maxLinearVelocity);
			setThrustersOn();
			SINGLE yaw;
			//yaw = atan2(delta.y,delta.x);
			yaw = TRANSFORM::get_yaw(delta);
			rotateTo(yaw,0,0);
		}
	}
	else if (moveStage == MS_DONE)
	{
		moveTo(transform.translation);
		rotateShip(0,0,0);
	}

	return false;
};
//---------------------------------------------------------------------------
//
void Fabricator::OnOperationCreation (U32 agentID, void *buffer, U32 bufferSize)
{
	CQASSERT(workingID == 0);
	workingID = agentID;
	BaseFabCommand * buf = (BaseFabCommand *)buffer;
	switch(buf->command)
	{
	case FAB_C_IDLE_OP:
		{
			mode = FAB_IDLE;
			COMP_OP(dwMissionID);
			break;
		}
	default:
		{
			CQASSERT(0 && "bad data for create operation");
		}
	}
}
//---------------------------------------------------------------------------
//
void Fabricator::receiveOperationData (U32 agentID, void *buffer, U32 bufferSize)
{
	if(agentID != workingID)
		return;
	CQASSERT(workingID);
	BaseFabCommand * buf = (BaseFabCommand *)buffer;
	switch(buf->command)
	{
	case FAB_C_DISM_BEG_PLANET:
		{
			FabCommandW2Handle * myBuf = (FabCommandW2Handle *) buffer;
			targetPlanetID = myBuf->handle1;
			targetSlotID = myBuf->handle2;

			mode = FAB_MOVING_TO_TARGET_DISM;
			BeginMove();

			break;
		}
	case FAB_C_DISM_BEG_POSITION:
		{
			FabCommandWGridVect * myBuf = (FabCommandWGridVect *)buffer;
			targetPosition = myBuf->grid;
			targetPlanetID = NULL;

			mode = FAB_MOVING_TO_TARGET_DISM;
			BeginMove();

			break;
		}
	case FAB_C_DISM_CANCEL_LATE:
		{
			if(targetPlanetID)
			{
				IBaseObject * targ = OBJLIST->FindObject(targetPlanetID);
				OBJPTR<IPlanet> planet;
				targ->QueryInterface(IPlanetID,planet);
				if(planet)
				{
					planet->CompleteSlotOperations(workingID,targetSlotID);
				}
				else
				{
					THEMATRIX->OperationCompleted(workingID,targ->GetPartID());
					THEMATRIX->OperationCompleted(workingID,SECTOR->GetJumpgateDestination(targ)->GetPartID());
				}
			}

			mode = FAB_IDLE;
			COMP_OP(dwMissionID);
			break;
		}
	case FAB_C_BUILD_CANCEL_LATE:
		{
			if(targetPlanetID)
			{
				IBaseObject * obj = OBJLIST->FindObject(targetPlanetID);
				if(obj->objClass == OC_JUMPGATE)
				{
					if(buildee)//remove the buildee from the operation we placed him in
					{
						OBJPTR<IBaseObject> obj;
						buildee->QueryInterface(IBaseObjectID,obj);
						THEMATRIX->OperationCompleted(workingID,obj->GetPartID());
						THEMATRIX->OperationCompleted(workingID,obj->GetPartID()|0x10000000);
					}
								
					THEMATRIX->OperationCompleted(workingID,obj->GetPartID());
					THEMATRIX->OperationCompleted(workingID,SECTOR->GetJumpgateDestination(obj)->GetPartID());
					FabHaltBuild();
				}
				else
				{
					if(buildee)//remove the buildee from the operation we placed him in
					{
						OBJPTR<IBaseObject> obj;
						buildee->QueryInterface(IBaseObjectID,obj);
						THEMATRIX->OperationCompleted(workingID,obj->GetPartID());	
						if(mode == FAB_UNBUILD)
						{
							OBJPTR<IPlatform> plat;
							buildee->QueryInterface(IPlatformID,plat);
							plat->FreePlanetSlot();
						}
					}
							
					OBJPTR<IPlanet> planet;
					IBaseObject * targetPlanet = OBJLIST->FindObject(targetPlanetID);
					targetPlanet->QueryInterface(IPlanetID,planet);
					planet->CompleteSlotOperations(workingID,targetSlotID);
					FabHaltBuild();
				}
			}
			else
			{
				if(buildee)//remove the buildee from the operation we placed him in
				{
					OBJPTR<IBaseObject> obj;
					buildee->QueryInterface(IBaseObjectID,obj);
					THEMATRIX->OperationCompleted(workingID,obj->GetPartID());	
				}
				FabHaltBuild();
			}
			mode = FAB_IDLE;
			COMP_OP(dwMissionID);
			break;
		}
	case FAB_C_BUILD_CANCEL_EARLY:
		{
			if(!targetPlanetID)
			{
				COMPTR<ITerrainMap> map;
				SECTOR->GetTerrainMap(systemID,map);
				FootprintInfo footprint;
				footprint.missionID = dwMissionID|0x10000000;
				footprint.height =box[2];
				footprint.flags = TERRAIN_DESTINATION | TERRAIN_HALFSQUARE;
				map->UndoFootprint(&targetPosition,1,footprint);
			}
			mode = FAB_IDLE;
			COMP_OP(dwMissionID);
			break;
		}
	case FAB_C_BUILD_IMPOSIBLE:
		{
			FABRICATORCOMM(buildImposible, SUB_NO_BUILD);
			mode = FAB_IDLE;
			COMP_OP(dwMissionID);
			break;
		}

	case FAB_C_BUILD_START_FAILED:
		{
			FabCommandWHandle * myBuf = (FabCommandWHandle *) buffer;
			if(myBuf->handle == M_GAS)
				FABRICATORCOMM(notEnoughGas,SUB_NO_GAS);
			else if(myBuf->handle == M_METAL)
				FABRICATORCOMM(notEnoughMetal,SUB_NO_METAL);
			else if(myBuf->handle == M_CREW)
				FABRICATORCOMM(notEnoughCrew, SUB_NO_CREW);
			else if(myBuf->handle == M_COMMANDPTS)
				FABRICATORCOMM(notEnoughCommandPoints, SUB_NO_CP);
			else if(myBuf->handle == 0xFFFFFFFF)
				FABRICATORCOMM(buildImposible, SUB_NO_BUILD);

			mode = FAB_IDLE;
			COMP_OP(dwMissionID);			
			break;
		}
	case FAB_C_BEGIN_CON_INFO://Initialization data for the build, need to move to target first
		{
			CQASSERT(buildingID);
			BASE_PLATFORM_DATA * baseData = (BASE_PLATFORM_DATA *)(ARCHLIST->GetArchetypeData(buildingID));
			CQASSERT(baseData);
			workingCost = baseData->missionData.resourceCost;

			FabCommandWHandle * myBuf = (FabCommandWHandle *) buffer;
			CQASSERT(mode == FAB_WAITING_INIT_CONS_CLIENT);
			mode = FAB_MOVING_TO_TARGET;
			targetSlotID = myBuf->handle;

			BeginMove();
			break;
		}
	case FAB_C_BUILD_PLATFORM:
		{
			CQASSERT((mode == FAB_WATING_TO_START_BUILD_CLIENT) ||
				(mode == FAB_MOVING_TO_READY_TARGET_CLIENT));

			if (mode == FAB_WATING_TO_START_BUILD_CLIENT)
			{
				//I'm already at the platform and I just received the message that the build is beginning
				fabStartAnims();
			}

			FabCommandWHandle * myBuf = (FabCommandWHandle *) buffer;
			THEMATRIX->SetCancelState(workingID,false);
			if(targetPlanetID)
			{
				IBaseObject * obj = OBJLIST->FindObject(targetPlanetID);
				CQASSERT(obj);
				if(obj->objClass == OC_JUMPGATE)
				{
					IBaseObject * newPlat = MGlobals::CreateInstance(ARCHLIST->LoadArchetype(buildingID),myBuf->handle);
					OBJPTR<IPlatform> platformPtr;
					newPlat->QueryInterface(IPlatformID,platformPtr);

					OBJLIST->AddObject(newPlat);
					OBJPTR<IPhysicalObject> miss;
					if (newPlat->QueryInterface(IPhysicalObjectID,miss))
					{
						miss->SetSystemID(GetSystemID());
					}
					platformPtr->ParkYourself(obj);
					ENGINE->update_instance(newPlat->GetObjectIndex(),0,0);
					OBJPTR<IBuild> targBuildee;
					newPlat->QueryInterface(IBuildID,targBuildee);
					targBuildee->SetProcessID(workingID);
					FabStartBuild(targBuildee);			
					mode = FAB_BUILDING;
				}
				else
				{
					OBJPTR<IPlanet> planet;
					obj->QueryInterface(IPlanetID,planet);
					CQASSERT(planet != 0);

//					CQASSERT(!planet->GetSlotUser(targetSlotID));
		//			planet->AllocateBuildSlot(buf[1],slotID);

					IBaseObject * newPlat = MGlobals::CreateInstance(ARCHLIST->LoadArchetype(buildingID),myBuf->handle);
					OBJPTR<IPlatform> platformPtr;
					newPlat->QueryInterface(IPlatformID,platformPtr);
					OBJLIST->AddObject(newPlat);
					OBJPTR<IPhysicalObject> miss;
					if (newPlat->QueryInterface(IPhysicalObjectID,miss))
					{
						miss->SetSystemID(GetSystemID());
					}
					CQASSERT(targetSlotID && "No SlotID for build");
					platformPtr->ParkYourself(planet->GetSlotTransform(targetSlotID),targetPlanetID,targetSlotID);
					ENGINE->update_instance(newPlat->GetObjectIndex(),0,0);
					OBJPTR<IBuild> targBuildee;
					newPlat->QueryInterface(IBuildID,targBuildee);
					targBuildee->SetProcessID(workingID);
					FabStartBuild(targBuildee);			
					mode = FAB_BUILDING;
				}
			}
			else
			{
				IBaseObject * newPlat = MGlobals::CreateInstance(ARCHLIST->LoadArchetype(buildingID),myBuf->handle);
				OBJPTR<IPlatform> platformPtr;
				newPlat->QueryInterface(IPlatformID,platformPtr);
				OBJLIST->AddObject(newPlat);
				VOLPTR(IPhysicalObject) miss;
				if ((miss = newPlat) != 0)
				{
					if(!platformPtr->IsHalfSquare())
						targetPosition.centerpos();
					miss->SetPosition(targetPosition, systemID);
				}
				ENGINE->update_instance(newPlat->GetObjectIndex(),0,0);
				OBJPTR<IBuild> targBuildee;
				newPlat->QueryInterface(IBuildID,targBuildee);
				targBuildee->SetProcessID(workingID);
				FabStartBuild(targBuildee);			
				mode = FAB_BUILDING;
			}
			break;
		}
	case FAB_C_BEGIN_REPAIR:
		{
			CQASSERT((mode == FAB_WATING_TO_START_REPAIR_CLIENT) || (FAB_MOVING_TO_READY_TARGET_REPAIR_CLIENT));
			mode = FAB_REPAIRING;

			if (mode == FAB_WATING_TO_START_REPAIR_CLIENT)
			{
				//I'm already at the platform and I just received the message that the repair is beginning
				fabStartAnims();
			}

			if(workTarg)
			{
				MPart part = workTarg;
				oldHullPoints = part->hullPoints;

				Vector pos,dir;
				OBJPTR<IShipDamage> shipDamage;
				workTarg->QueryInterface(IShipDamageID,shipDamage);
				CQASSERT(shipDamage);
				shipDamage->GetNextDamageSpot(pos,dir);
				if(numDrones)
				{
					if(drone[curDrone])
					{
						VOLPTR(IShuttle) shuttle;
						shuttle = drone[curDrone];
						shuttle->WorkAtShipPos(workTarg,pos,dir,300);
					}
					curDrone = (curDrone+1)%numDrones;
				}
			}
			break;
		}
	case FAB_C_REPAIR_BEG_PLANET:
		{
			FabCommandW2Handle * myBuf = (FabCommandW2Handle *) buffer;
			targetPlanetID = myBuf->handle1;
			targetSlotID = myBuf->handle2;

			mode = FAB_MOVING_TO_TARGET_REPAIR;
			BeginMove();

			break;
		}
	case FAB_C_REPAIR_BEG_POSITION:
		{
			FabCommandWGridVect * myBuf = (FabCommandWGridVect *)buffer;
			targetPosition = myBuf->grid;
			targetPlanetID = NULL;

			mode = FAB_MOVING_TO_TARGET_REPAIR;
			BeginMove();

			break;
		}
	case FAB_C_BEGIN_CON_INFO_JUMP:
		{
			CQASSERT(mode == FAB_WAITING_INIT_CONS_CLIENT);
			mode = FAB_MOVING_TO_TARGET;
			BeginMove();
			break;
		}
	case FAB_C_BEGIN_CON_INFO_VECT:
		{
			CQASSERT(mode == FAB_WAITING_INIT_CONS_CLIENT);
			mode = FAB_MOVING_TO_TARGET;
			BeginMove();
			break;

		}
	case FAB_C_BEGIN_DISM:
		{
			CQASSERT((mode == FAB_WATING_TO_START_DISM_CLIENT) ||
				(mode == FAB_MOVING_TO_READY_TARGET_DISM_CLIENT));

			if (mode == FAB_WATING_TO_START_DISM_CLIENT)
			{
				//I'm already at the platform and I just received the message that the disassemble is beginning
				fabStartAnims();
			}

			FabCommandW2Handle * myBuf = (FabCommandW2Handle *)buffer;
			mode = FAB_DISMING;
			THEMATRIX->SetCancelState(workingID,false);

			if(!workTarg)
				OBJLIST->FindObject(workTargID,playerID,workTarg);
			OBJPTR<IMissionActor> actor;
			workTarg->QueryInterface(IMissionActorID,actor);
//			actor->PrepareTakeover(workTargID, 0);
			workTarg->SetReady(false);
			OBJLIST->UnselectObject(workTarg);

			targetPlanetID = myBuf->handle1;
			targetSlotID = myBuf->handle2;

			OBJPTR<IBuild> targBuildee;
			workTarg->QueryInterface(IBuildID,targBuildee);
			FabStartDismantle(targBuildee);			
			break;
		}
	case FAB_C_AT_TARGET_DISM:
		{
			CQASSERT((mode == FAB_MOVING_TO_TARGET_DISM) || (mode == FAB_AT_TARGET_DISM_CLIENT));
			if(mode == FAB_MOVING_TO_TARGET_REPAIR)//have not reached target on client
			{
				mode = FAB_MOVING_TO_READY_TARGET_DISM_CLIENT;
			}
			else // already at target
			{
				mode = FAB_WATING_TO_START_DISM_CLIENT;
			}
			if(!targetPlanetID)
			{
				COMPTR<ITerrainMap> map;
				SECTOR->GetTerrainMap(systemID,map);
				FootprintInfo footprint;
				footprint.missionID = dwMissionID|0x10000000;
				footprint.height =box[2];
				footprint.flags = TERRAIN_DESTINATION | TERRAIN_HALFSQUARE;
				map->UndoFootprint(&targetPosition,1,footprint);
			}
			break;
		}
	case FAB_C_DISM_FINISHED:
		{
			CQASSERT(mode == FAB_DISMING);
			if(workTarg)//remove the workTarg from the operation we placed him in
			{
				OBJPTR<IBaseObject> obj;
				workTarg->QueryInterface(IBaseObjectID,obj);
				OBJPTR<IPlatform> plat;
				workTarg->QueryInterface(IPlatformID,plat);
				plat->FreePlanetSlot();
				THEMATRIX->OperationCompleted(workingID,workTargID);
			}
			if(targetPlanetID)
			{
				IBaseObject * targ = OBJLIST->FindObject(targetPlanetID);
				OBJPTR<IPlanet> planet;
				targ->QueryInterface(IPlanetID,planet);
				if(planet)
				{
					planet->CompleteSlotOperations(workingID,targetSlotID);
				}
				else
				{
					THEMATRIX->OperationCompleted(workingID,targ->GetPartID());
					THEMATRIX->OperationCompleted(workingID,SECTOR->GetJumpgateDestination(targ)->GetPartID());
				}
			}

			
			FabEnableCompletion();

			mode = FAB_IDLE;
			COMP_OP(dwMissionID);
			break;
		}
	case FAB_C_STOP_DISM:
		{
			for(U32 count = 0; count<numDrones; ++count)
			{
				if(drone[count])
					drone[count]->Return();
			}
			mode = FAB_IDLE;

			if(targetPlanetID)
			{
				IBaseObject * targ = OBJLIST->FindObject(targetPlanetID);
				OBJPTR<IPlanet> planet;
				targ->QueryInterface(IPlanetID,planet);
				if(planet)
				{
					planet->CompleteSlotOperations(workingID,targetSlotID);
				}
				else
				{
					THEMATRIX->OperationCompleted(workingID,targ->GetPartID());
					THEMATRIX->OperationCompleted(workingID,SECTOR->GetJumpgateDestination(targ)->GetPartID());
				}
			}
			
			COMP_OP(dwMissionID);
			break;
		}
	case FAB_C_AT_TARGET_REPAIR:
		{
			CQASSERT((mode == FAB_MOVING_TO_TARGET_REPAIR) || (mode == FAB_AT_TARGET_REPAIR_CLIENT));
			if(mode == FAB_MOVING_TO_TARGET_REPAIR)//have not reached target on client
			{
				mode = FAB_MOVING_TO_READY_TARGET_REPAIR_CLIENT;
			}
			else // already at target
			{
				mode = FAB_WATING_TO_START_REPAIR_CLIENT;
			}
			if(!targetPlanetID)
			{
				COMPTR<ITerrainMap> map;
				SECTOR->GetTerrainMap(systemID,map);
				FootprintInfo footprint;
				footprint.missionID = dwMissionID|0x10000000;
				footprint.height =box[2];
				footprint.flags = TERRAIN_DESTINATION | TERRAIN_HALFSQUARE;
				map->UndoFootprint(&targetPosition,1,footprint);
			}
			break;
		}
	case FAB_C_REPAIR_FINISHED:
		{
			CQASSERT(mode == FAB_REPAIRING);
			mode = FAB_IDLE;
			for(U32 count = 0; count<numDrones; ++count)
			{
				if(drone[count])
					drone[count]->Return();
			}
			COMP_OP(dwMissionID);
			break;
		}
	case FAB_C_AT_TARGET:
		{
			CQASSERT((mode == FAB_MOVING_TO_POSITION) || (mode == FAB_MOVING_TO_TARGET) || (mode == FAB_AT_TARGET_CLIENT));
			if(mode == FAB_MOVING_TO_TARGET)//have not reached target on client
			{
				mode = FAB_MOVING_TO_READY_TARGET_CLIENT;
			}
			else // already at target
			{
				mode = FAB_WATING_TO_START_BUILD_CLIENT;
			}
			if(!targetPlanetID)
			{
				COMPTR<ITerrainMap> map;
				SECTOR->GetTerrainMap(systemID,map);
				FootprintInfo footprint;
				footprint.missionID = dwMissionID|0x10000000;
				footprint.height =box[2];
				footprint.flags = TERRAIN_DESTINATION | TERRAIN_HALFSQUARE;
				map->UndoFootprint(&targetPosition,1,footprint);
			}
			break;
		}
	case FAB_C_CANCEL_REPAIR_NOMONEY:
		{
			FabCommandWHandle * myBuf = (FabCommandWHandle *)buffer;
			failSound((M_RESOURCE_TYPE)(myBuf->handle));
		}
		//no break;
	case FAB_C_CANCEL_DISM:
	case FAB_C_CANCEL_REPAIR:
	case FAB_C_CANCEL_BUILD:
		{
			CQASSERT((mode == FAB_AT_TARGET_CLIENT) ||
					(mode == FAB_WATING_TO_START_BUILD_CLIENT) ||
					(mode == FAB_MOVING_TO_READY_TARGET_CLIENT) ||
					(mode == FAB_MOVING_TO_TARGET) ||
					(mode == FAB_MOVING_TO_POSITION) ||
					(mode == FAB_AT_TARGET_REPAIR_CLIENT) ||
					(mode == FAB_WATING_TO_START_REPAIR_CLIENT) ||
					(mode == FAB_MOVING_TO_READY_TARGET_REPAIR_CLIENT) ||
					(mode == FAB_MOVING_TO_TARGET_REPAIR) ||
					(mode == FAB_WAIT_REPAIR_INFO_CLIENT) ||
					(mode == FAB_WAIT_DISM_INFO_CLIENT) ||
					(mode == FAB_DISMING) ||
					(mode == FAB_REPAIRING)||
					(mode == FAB_MOVING_TO_TARGET_DISM)||
					(mode == FAB_MOVING_TO_READY_TARGET_DISM_CLIENT) ||
					(mode == FAB_WATING_TO_START_DISM_CLIENT));
			for(U32 count = 0; count<numDrones; ++count)
			{
				if(drone[count])
					drone[count]->Return();
			}
			mode = FAB_IDLE;
			COMP_OP(dwMissionID);
			break;
		}
	case FAB_C_BUILD_FINISH:
		{
			CQASSERT((mode == FAB_BUILDING) || (mode == FAB_UNBUILD));
			if(buildee)
			{
				if(buildee->NumFabOnBuild() == 1)
				{
					if(mode == FAB_BUILDING)
					{
						incrementScoringPlatformsBuilt();
					}
					if(targetPlanetID)
					{
						IBaseObject * obj = OBJLIST->FindObject(targetPlanetID);
						if(obj->objClass == OC_JUMPGATE)
						{
							if(buildee)//remove the buildee from the operation we placed him in
							{
								OBJPTR<IBaseObject> obj;
								buildee->QueryInterface(IBaseObjectID,obj);
								THEMATRIX->OperationCompleted(workingID,obj->GetPartID());
								THEMATRIX->OperationCompleted(workingID,obj->GetPartID()|0x10000000);
							}
							
							THEMATRIX->OperationCompleted(workingID,obj->GetPartID());
							THEMATRIX->OperationCompleted(workingID,SECTOR->GetJumpgateDestination(obj)->GetPartID());
							FabEnableCompletion();
						}
						else
						{
							if(buildee)//remove the buildee from the operation we placed him in
							{
								OBJPTR<IBaseObject> obj;
								buildee->QueryInterface(IBaseObjectID,obj);
								THEMATRIX->OperationCompleted(workingID,obj->GetPartID());	
								if(mode == FAB_UNBUILD)
								{
									OBJPTR<IPlatform> plat;
									buildee->QueryInterface(IPlatformID,plat);
									plat->FreePlanetSlot();
								}
							}
							OBJPTR<IPlanet> planet;
							IBaseObject * targetPlanet = OBJLIST->FindObject(targetPlanetID);
							targetPlanet->QueryInterface(IPlanetID,planet);
							planet->CompleteSlotOperations(workingID,targetSlotID);
							FabEnableCompletion();
						}
					}
					else
					{
						if(buildee)//remove the buildee from the operation we placed him in
						{
							OBJPTR<IBaseObject> obj;
							buildee->QueryInterface(IBaseObjectID,obj);
							THEMATRIX->OperationCompleted(workingID,obj->GetPartID());	
						}
						
						FabEnableCompletion();
					}
				}
				else
				{
					FabAssistBuildDone();
				}
			}
			mode = FAB_IDLE;
			COMP_OP(dwMissionID);
			enableAutoMovement();
			SetPosition(transform.translation, systemID);
			break;
		}
/*	case FAB_C_STOP_ASSIST_BUILD:
		{
			CQASSERT((mode == FAB_BUILDING) || (mode == FAB_WAIT_STOP_CLIENT));
			if(!(mode == FAB_WAIT_STOP_CLIENT))
			{
				if(buildee && (buildee->NumFabOnBuild() == 1) && (!(buildee->IsReversing())))
				{
					//if we get here then the build was completed by another fabricator before the cancel reached the client.
					//the otherr fabrcator thinks he had help so he will no make the platform.  Create it for him.
					FabEnableCompletion();
				} else if(buildee)
				{
					FabAssistBuildDone();
				}
			}
			mode = FAB_IDLE;
			COMP_OP(dwMissionID);
			break;
		}
*/	case FAB_C_REVERSE_BUILD:
		{
			CQASSERT(mode == FAB_BUILDING);
			buildee->CancelBuild();
			mode = FAB_UNBUILD;
			break;
		}
	case FAB_C_STOP_BUILD:
		{
			CQASSERT((mode == FAB_UNBUILD) || (mode == FAB_BUILDING));
			if(targetPlanetID)
			{
				OBJPTR<IPlanet> planet;
				IBaseObject * targetPlanet = OBJLIST->FindObject(targetPlanetID);
				targetPlanet->QueryInterface(IPlanetID,planet);
				if(planet)
					planet->CompleteSlotOperations(workingID,targetSlotID);
				else//jumpgate
				{
					THEMATRIX->OperationCompleted(workingID,targetPlanet->GetPartID());
					THEMATRIX->OperationCompleted(workingID,SECTOR->GetJumpgateDestination(targetPlanet)->GetPartID());
				}
			}
			mode = FAB_IDLE;
			FabEmergencyStop();
			COMP_OP(dwMissionID);
			enableAutoMovement();
			SetPosition(transform.translation, systemID);
			break;
		}
	default:
		{
			CQASSERT(0 && "Bad data sent to RecieveOperationData in Harvester");
		}
	}

}
//---------------------------------------------------------------------------
//
void Fabricator::OnAddToOperation (U32 agentID)
{
//	CQASSERT(!workingID);
//	workingID = agentID;
}
//---------------------------------------------------------------------------
//
void Fabricator::onOperationCancel (U32 agentID)
{
	if(workingID == agentID)
	{
		CQASSERT((mode != FAB_IDLE) || (mode != FAB_BUILDING) );
		
		for(U32 count = 0; count<numDrones; ++count)
		{
			if(drone[count])
				drone[count]->Return();
		}
		if(mode == 	FAB_MOVING_TO_TARGET || mode == FAB_MOVING_TO_POSITION)
		{
			if(HOST_CHECK)
			{
				BANKER->AddResource(playerID,workingCost);
				BANKER->FreeCommandPt(playerID,workingCost.commandPt);
			}
			if(!targetPlanetID)
			{
				COMPTR<ITerrainMap> map;
				SECTOR->GetTerrainMap(systemID,map);
				FootprintInfo footprint;
				footprint.missionID = dwMissionID|0x10000000;
				footprint.height =box[2];
				footprint.flags = TERRAIN_DESTINATION | TERRAIN_HALFSQUARE;
				map->UndoFootprint(&targetPosition,1,footprint);
			}
		}
		mode = FAB_IDLE;
		workingID = NULL;
	}

}
//---------------------------------------------------------------------------
//
void Fabricator::preSelfDestruct (void)
{
	//this is where we update the techtree of our loss....
	IBaseObject * obj = OBJLIST->GetObjectList();
	bool setNoPlatform = true;
	while(obj)
	{
		if((obj->objClass == OC_SPACESHIP) && (obj != ((IBaseObject *)this)) &&
			(obj->GetPlayerID() == GetPlayerID()))
		{
			MPart part(obj);
			if (part->mObjClass == M_FABRICATOR)
			{
				setNoPlatform = false;
				obj = NULL;
			}
			else
				obj = obj->next;
		}
		else
			obj = obj->next;
	}
	if(setNoPlatform)
	{
		TECHNODE oldTech = MGlobals::GetCurrentTechLevel(GetPlayerID());
		oldTech.race[M_TERRAN].build = (TECHTREE::BUILDNODE)(oldTech.race[M_TERRAN].build & (~(TECHTREE::RESERVED_FABRICATOR)));
		MGlobals::SetCurrentTechLevel(oldTech,GetPlayerID());
	}

	if(HOST_CHECK)
	{
		if(workingID)
		{
			if((mode == FAB_BUILDING) || (mode == FAB_UNBUILD))
			{
				BaseFabCommand buffer;
				buffer.command = FAB_C_BUILD_CANCEL_LATE;
				NET_METRIC(sizeof(buffer));
				THEMATRIX->SendOperationData(workingID,dwMissionID,&buffer,sizeof(buffer));

				if(targetPlanetID)
				{
					IBaseObject * obj = OBJLIST->FindObject(targetPlanetID);
					if(obj->objClass == OC_JUMPGATE)
					{
						if(buildee)//remove the buildee from the operation we placed him in
						{
							OBJPTR<IBaseObject> obj;
							buildee->QueryInterface(IBaseObjectID,obj);
							THEMATRIX->OperationCompleted(workingID,obj->GetPartID());
							THEMATRIX->OperationCompleted(workingID,obj->GetPartID()|0x10000000);
						}
									
						THEMATRIX->OperationCompleted(workingID,obj->GetPartID());
						THEMATRIX->OperationCompleted(workingID,SECTOR->GetJumpgateDestination(obj)->GetPartID());
						IBaseObject * targ = buildee.Ptr();			
						FabHaltBuild();
						if(targ)
						{
							THEMATRIX->ObjectTerminated(targ->GetPartID(),0);
							THEMATRIX->ObjectTerminated(targ->GetPartID()|0x10000000,0);
						}
					}
					else
					{
						if(buildee)//remove the buildee from the operation we placed him in
						{
							OBJPTR<IBaseObject> obj;
							buildee->QueryInterface(IBaseObjectID,obj);
							THEMATRIX->OperationCompleted(workingID,obj->GetPartID());	
							if(mode == FAB_UNBUILD)
							{
								OBJPTR<IPlatform> plat;
								buildee->QueryInterface(IPlatformID,plat);
								plat->FreePlanetSlot();
							}
						}
								
						OBJPTR<IPlanet> planet;
						IBaseObject * targetPlanet = OBJLIST->FindObject(targetPlanetID);
						targetPlanet->QueryInterface(IPlanetID,planet);
						planet->CompleteSlotOperations(workingID,targetSlotID);
						IBaseObject * targ = buildee.Ptr();			
						FabHaltBuild();
						if(targ)
							THEMATRIX->ObjectTerminated(targ->GetPartID(),0);
					}
				}
				else
				{
					if(buildee)//remove the buildee from the operation we placed him in
					{
						OBJPTR<IBaseObject> obj;
						buildee->QueryInterface(IBaseObjectID,obj);
						THEMATRIX->OperationCompleted(workingID,obj->GetPartID());	
					}
								
					IBaseObject * targ = buildee.Ptr();			
					FabHaltBuild();
					if(targ)
						THEMATRIX->ObjectTerminated(targ->GetPartID(),0);
				}
				mode = FAB_IDLE;
				COMP_OP(dwMissionID);
				EVENTSYS->Post(CQE_FABRICATOR_FINISHED,(void *)dwMissionID);
			}
			else if(mode == FAB_MOVING_TO_TARGET || mode == FAB_MOVING_TO_POSITION)
			{
				BaseFabCommand buffer;
				buffer.command = FAB_C_BUILD_CANCEL_EARLY;
				NET_METRIC(sizeof(buffer));
				THEMATRIX->SendOperationData(workingID,dwMissionID,&buffer,sizeof(buffer));

				BANKER->AddResource(playerID,workingCost);
				if(!targetPlanetID)
				{
					COMPTR<ITerrainMap> map;
					SECTOR->GetTerrainMap(systemID,map);
					FootprintInfo footprint;
					footprint.missionID = dwMissionID|0x10000000;
					footprint.height =box[2];
					footprint.flags = TERRAIN_DESTINATION | TERRAIN_HALFSQUARE;
					map->UndoFootprint(&targetPosition,1,footprint);
				}
				mode = FAB_IDLE;
				COMP_OP(dwMissionID);
				EVENTSYS->Post(CQE_FABRICATOR_FINISHED,(void *)dwMissionID);
			}
			else if(mode == FAB_DISMING)
			{
				BaseFabCommand buffer;
				buffer.command = FAB_C_DISM_CANCEL_LATE;
				NET_METRIC(sizeof(buffer));
				THEMATRIX->SendOperationData(workingID,dwMissionID,&buffer,sizeof(buffer));

				if(targetPlanetID)
				{
					IBaseObject * targ = OBJLIST->FindObject(targetPlanetID);
					OBJPTR<IPlanet> planet;
					targ->QueryInterface(IPlanetID,planet);
					if(planet)
					{
						planet->CompleteSlotOperations(workingID,targetSlotID);
					}
					else
					{
						THEMATRIX->OperationCompleted(workingID,targ->GetPartID());
						THEMATRIX->OperationCompleted(workingID,SECTOR->GetJumpgateDestination(targ)->GetPartID());
					}
				}

				if(workTarg)
				{
					FabHaltBuild();
					OBJPTR<IJumpPlat> jPlat;
					workTarg->QueryInterface(IJumpPlatID,jPlat);
					if (jPlat)
					{
						// the sibling can be NULL
						IBaseObject * sibling = jPlat->GetSibling();
						if (sibling)
						{
							THEMATRIX->ObjectTerminated(sibling->GetPartID(),0);
						}
					}
					THEMATRIX->ObjectTerminated(workTarg->GetPartID(),0);
				}
				COMP_OP(dwMissionID);
			}
			else if(mode == FAB_REPAIRING)
			{
				BaseFabCommand buffer;
				buffer.command = FAB_C_REPAIR_FINISHED;
				NET_METRIC(sizeof(buffer));
				THEMATRIX->SendOperationData(workingID,dwMissionID,&buffer,sizeof(buffer));

				for(U32 count = 0; count<numDrones; ++count)
				{
					if(drone[count])
						drone[count]->Return();
				}

				mode = FAB_IDLE;
				COMP_OP(dwMissionID);
			}
			else if(mode == FAB_MOVING_TO_TARGET_REPAIR)
			{
				BaseFabCommand buffer;
				buffer.command = FAB_C_CANCEL_REPAIR;
				NET_METRIC(sizeof(buffer));
				THEMATRIX->SendOperationData(workingID,dwMissionID,&buffer,sizeof(buffer));

				for(U32 count = 0; count<numDrones; ++count)
				{
					if(drone[count])
						drone[count]->Return();
				}
				mode = FAB_IDLE;
				COMP_OP(dwMissionID);
			}
			else
			{
				CQBOMB3("Fabrictor %x leaking agent %d on client, mode:%d",dwMissionID,workingID,mode);
				COMP_OP(dwMissionID);
			}
		}
	}
	mode = FAB_EXPLODING;
}
//---------------------------------------------------------------------------
//
ResourceCost Fabricator::GetAmountNeeded()
{
	return workingCost;
}
//---------------------------------------------------------------------------
//
void Fabricator::UnstallSpender()
{
}
//---------------------------------------------------------------------------
//
bool Fabricator::IsFabAtObj()
{
	return (moveStage == MS_DONE);
}
//---------------------------------------------------------------------------
//
/*void Fabricator::TestVisible (const USER_DEFAULTS & defaults, const U32 currentSystem, const U32 currentPlayer)
{
	SpaceShip<FABRICATOR_SAVELOAD, FABRICATOR_INIT>::TestVisible(defaults, currentSystem, currentPlayer);

	for(U32 count = 0; count < numDrones; ++count)
	{
		if(drone[count])
			drone[count].ptr->TestVisible(defaults, currentSystem, currentPlayer);
	}
}
//-------------------------------------------------------------------
//
void Fabricator::Render (void)
{
	SpaceShip<FABRICATOR_SAVELOAD, FABRICATOR_INIT>::Render();
	if(bReady)
	{
		for (U8 c=0;c<numDrones;c++)
		{
			if(drone[c])
				drone[c].ptr->Render();
		}
	}
}*/
//---------------------------------------------------------------------------
//
void Fabricator::save (FABRICATOR_SAVELOAD & save)
{
	save.baseSaveLoad = *static_cast<BASE_FABRICATOR_SAVELOAD *>(this);
}
//---------------------------------------------------------------------------
//
void Fabricator::load (FABRICATOR_SAVELOAD & load)
{
	*static_cast<BASE_FABRICATOR_SAVELOAD *>(this) = load.baseSaveLoad;
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//
inline struct IBaseObject * createFabricator (const FABRICATOR_INIT & data)
{
	Fabricator * obj = new ObjectImpl<Fabricator>;

	obj->FRAME_init(data);
	return obj;
}

//------------------------------------------------------------------------------------------
//---------------------------Fabricator Factory----------------------------------------------
//------------------------------------------------------------------------------------------
//
struct DACOM_NO_VTABLE FabricatorFactory : public IObjectFactory
{
	struct OBJTYPE : FABRICATOR_INIT
	{
		~OBJTYPE (void)
		{
			int i;
			if (pBuildEffect)
				ARCHLIST->Release(pBuildEffect, OBJREFNAME);
	
			for (i = 0; i < NUM_DRONE_RELEASE; i++)
				if (builderInfo[i].pBuilderType)
					ARCHLIST->Release(builderInfo[i].pBuilderType, OBJREFNAME);
		}
	};
	//------------------------------------------------------------------------------------------
	//
	U32 factoryHandle;		// handles to callback

	//
	// Interface mapping
	//

	BEGIN_DACOM_MAP_INBOUND(FabricatorFactory)
	DACOM_INTERFACE_ENTRY(IObjectFactory)
	END_DACOM_MAP()

	FabricatorFactory (void) { }

	~FabricatorFactory (void);

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

	/* FabricatorFactory methods */

	IDAComponent * getBase (void)
	{
		return static_cast<IObjectFactory *> (this);
	}
};
//--------------------------------------------------------------------------//
//
FabricatorFactory::~FabricatorFactory (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (OBJLIST && OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
		connection->Unadvise(factoryHandle);
}
//--------------------------------------------------------------------------//
//
void FabricatorFactory::init (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
		connection->Advise(getBase(), &factoryHandle);
}
//-----------------------------------------------------------------------------
//
HANDLE FabricatorFactory::CreateArchetype (const char *szArchname, OBJCLASS objClass, void *_data)
{
	OBJTYPE * result = 0;

	if (objClass == OC_SPACESHIP)
	{
		BT_FABRICATOR_DATA * data = (BT_FABRICATOR_DATA *) _data;

		if (data->type == SSC_FABRICATOR)	   
		{
			result = new OBJTYPE;

			SFXMANAGER->Preload(data->buildSound);
			
			if (result->loadSpaceshipArchetype(data, ARCHLIST->GetArchetype(szArchname)) == false)
				goto Error;
			
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

			switch (data->missionData.race)
			{
			case M_TERRAN:
				result->pBuildEffect = ARCHLIST->LoadArchetype("BUILD!!Terran");
				break;
			case M_MANTIS:
				result->pBuildEffect = ARCHLIST->LoadArchetype("BUILD!!Mantis");
				break;
			case M_SOLARIAN:
				result->pBuildEffect = ARCHLIST->LoadArchetype("BUILD!!Solarian");
			}
			if (result->pBuildEffect)
				ARCHLIST->AddRef(result->pBuildEffect, OBJREFNAME);
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
BOOL32 FabricatorFactory::DestroyArchetype (HANDLE hArchetype)
{
	OBJTYPE * objtype = (OBJTYPE *)hArchetype;

	EditorStopObjectInsertion(objtype->pArchetype);
	delete objtype;

	return 1;
}
//-------------------------------------------------------------------
//
IBaseObject * FabricatorFactory::CreateInstance (HANDLE hArchetype)
{
	OBJTYPE * objtype = (OBJTYPE *)hArchetype;
	return createFabricator(*objtype);
}
//-------------------------------------------------------------------
//
void FabricatorFactory::EditorCreateInstance (HANDLE hArchetype, const MISSION_INFO & info)
{
	OBJTYPE * objtype = (OBJTYPE *)hArchetype;
	EditorStartObjectInsertion(objtype->pArchetype, info);
}
//---------------------------------------------------------------------------
//-------------------------GLOBAL STARTUP------------------------------------
//---------------------------------------------------------------------------
//
struct _fabricatorship : GlobalComponent
{
	FabricatorFactory * sfactory;

	virtual void Startup (void)
	{
		sfactory = new DAComponent<FabricatorFactory>;
		AddToGlobalCleanupList((IDAComponent **) &sfactory);
	}

	virtual void Initialize (void)
	{
		sfactory->init();
	}
};

static _fabricatorship __ship;

//---------------------------------------------------------------------------
//--------------------------End Fabricator.cpp--------------------------------
//---------------------------------------------------------------------------