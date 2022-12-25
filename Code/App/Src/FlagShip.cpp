//--------------------------------------------------------------------------//
//                                                                          //
//                               FlagShip.cpp                               //
//                                                                          //
//                  COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/FlagShip.cpp 197   8/23/01 1:53p Tmauer $
*/			    
//------------------------------- #INCLUDES --------------------------------//
//--------------------------------------------------------------------------//

#include "pch.h"
#include <globals.h>

#include "TSpaceShip.h"
#include "ObjList.h"
#include "Camera.h"
#include "SuperTrans.h"
#include "UserDefaults.h"
#include "Startup.h"
#include "IAdmiral.h"
#include <DFlagship.h>
#include "CommPacket.h"
#include "MPart.h"
#include "ObjSet.h"
#include "IAttack.h"
#include "ILauncher.h"
#include "DMTechNode.h"
#include "MPart.h"
#include "ObjMapIterator.h"
#include "Mission.h"
#include "TBuildQueue.h"

#include <TComponent.h>
#include <IConnection.h>
#include <Engine.h>
#include <TSmartPointer.h>
#include <HKEvent.h>
#include <3DMath.h>
#include <Physics.h>
#include <FileSys.h>
#include <WindowManager.h>
#include <IAnim.h>
#include <IHardPoint.h>

#include <stdlib.h>
#include <stdio.h>

#define IDLE_TICKS		U32(1+ 4 * DEF_REALTIME_FRAMERATE)
#define FORECAST_TICKS	(20)		// seconds

#define NUM_TARGETS		2
#define ATTACK_RANGE	6.0f * GRIDSIZE	// looking for stuff within 3 grids of the admiral
#define SEEK_RANGE		64.0f * GRIDSIZE 

#define INVALID_GROUP_ID 0xFFFFFFFF
//fleet group members
struct FleetMember
{
	FleetMember * next;
	U32 partID;
};

struct FleetGroup
{
	FleetGroup * next;
	FleetGroupDef * pData;
	FleetMember * memberList;
	U32 groupID;//the id of the group in the BT_FORMATION structure
	U32 placementID;
	SINGLE radius;

	S32 postX;
	S32 postY;
	bool bPostValid:1;

	struct AI
	{
		bool bPostMoved;
		OBJPTR<IBaseObject> targets[NUM_TARGETS];
		NETGRIDVECTOR lastPos;
		NETGRIDVECTOR rovingPost;
		bool bRoving:1;
	}ai;

	FleetGroup()
	{
		next = NULL;
		memberList = NULL;
		bPostValid= false;
		ai.bPostMoved = false;
		ai.targets[0] = NULL;
		ai.targets[1] = NULL;
		ai.bRoving = false;
	};

	void AddMember(U32 member);
};


FleetMember * freeFleetMemberList = NULL;
FleetGroup * freeFleetGroupList = NULL;

FleetMember * getNewFleetMember()
{
	if(freeFleetMemberList)
	{
		FleetMember * ret = freeFleetMemberList;
		freeFleetMemberList = ret->next;
		ret->next = NULL;
		return ret;
	}
	return new FleetMember;
}

void releaseFleetMember(FleetMember * member)
{
	member->next = freeFleetMemberList;
	freeFleetMemberList = member;
}

void clearFleetMemberList()
{
	while(freeFleetMemberList)
	{
		FleetMember * tmp = freeFleetMemberList;
		freeFleetMemberList = freeFleetMemberList->next;
		delete tmp;
	}
}

FleetGroup * getNewFleetGroup()
{
	if(freeFleetGroupList)
	{
		FleetGroup * ret = freeFleetGroupList;
		freeFleetGroupList = ret->next;
		ret->next = NULL;
		ret->bPostValid = false;
		return ret;
	}
	return new FleetGroup;
}

//this will also realse any fleet members inside the group.
void releaseFleetGroup(FleetGroup * member)
{
	while(member->memberList)
	{
		FleetMember * tmp = member->memberList;
		member->memberList = member->memberList->next;
		releaseFleetMember(tmp);
	}
	member->ai.targets[0] = NULL;
	member->ai.targets[1] = NULL;
	member->ai.bRoving = false;
	member->next = freeFleetGroupList;
	freeFleetGroupList = member;
}

void clearFleetGroupList()
{
	while(freeFleetGroupList)
	{
		FleetGroup * tmp = freeFleetGroupList;
		freeFleetGroupList = freeFleetGroupList->next;
		delete tmp;
	}
}

void FleetGroup::AddMember(U32 member)
{
	FleetMember * newMem = getNewFleetMember();
	newMem->partID = member;
	newMem->next = memberList;
	memberList = newMem;
}

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
struct _NO_VTABLE Flagship : public TBuildQueue<SpaceShip<FLAGSHIP_SAVELOAD, FLAGSHIP_INIT> >, BASE_FLAGSHIP_SAVELOAD, IAdmiral
{
	BEGIN_MAP_INBOUND(Flagship)
	_INTERFACE_ENTRY(IBaseObject)
	_INTERFACE_ENTRY(IGotoPos)
	_INTERFACE_ENTRY(ISaveLoad)
	_INTERFACE_ENTRY(IQuickSaveLoad)
	_INTERFACE_ENTRY(IExplosionOwner)
	_INTERFACE_ENTRY(IMissionActor)
	_INTERFACE_ENTRY(IWeaponTarget)
	_INTERFACE_ENTRY(IBuild)
	_INTERFACE_ENTRY(IPhysicalObject)
	_INTERFACE_ENTRY(IAdmiral)
	_INTERFACE_ENTRY(IRepairee)
	_INTERFACE_ENTRY(IShipDamage)
	_INTERFACE_ENTRY(IExtent)
	_INTERFACE_ENTRY(IShipMove)
	_INTERFACE_ENTRY(ICloak)
	_INTERFACE_ENTRY(IBuildQueue)
	_INTERFACE_ENTRY(IEffectTarget)
	END_MAP()
	//----------------------------------

	PhysUpdateNode  physUpdateNode;
	UpdateNode		updateNode;
	ExplodeNode		explodeNode;
	SaveNode		saveNode;
	LoadNode		loadNode;
	ResolveNode		resolveNode;
	InitNode		initNode;
	OnOpCancelNode	onOpCancelNode;
	ReceiveOpDataNode	receiveOpDataNode;
	PreDestructNode	destructNode;
	GeneralSyncNode  genSyncNode1;
	GeneralSyncNode  genSyncNode2;
	GeneralSyncNode  genSyncNode3;
	RenderNode		renderNode;

	OBJPTR<IFleetShip> dockship;
	OBJPTR<IBaseObject> target;
	OBJPTR<IBaseObject> defend;

	U32 fleetStrength;
	SINGLE attackRadius;

	U32 nIdleShips;
	U32 idleShipArray[MAX_SELECTED_UNITS];	// these can be any ships in the fleet

	U32 nTotalGunboats;
	U32 nAvailableGunboats;
	U32 idleGunboatArray[MAX_SELECTED_UNITS];	// these are gunboats only
	U32 nTargetUpdateGunboats;
	U32 targetUpdateGunboatArray[MAX_SELECTED_UNITS];	// these are gunboats only

	U32 nSupplyShips;
	U32 supplyShipArray[MAX_SELECTED_UNITS];	// supply ships only

	ObjSet fleetSet;
	OBJPTR<IBaseObject> targets[NUM_TARGETS];	

	ADMIRAL_TACTIC netAdmiralTactic;

	BT_FORMATION * pFormation;
	U32 netFormationID;
	bool bReformFormation:1;//set true if the groups need to be reallocated.
	bool bForceUpdateFormationPost:1;
	FleetGroup * fleetGroups;
	OBJPTR<IBaseObject> formationTarget;
	OBJPTR<IJumpGate> targetFormationGate;
	BT_COMMAND_KIT * knownKits[MAX_KNOWN_KITS];

	//builf sync data
	bool bSyncQueueAdd:1;
	bool bSyncQueueRemove:1;
	bool bSyncQueueSound:1;
	bool bSyncBuildFinished:1;
	U32 syncQueueAddData;
	U32 syncQueueRemoveData;
	U32 syncFailSound;
	U32 syncFinsihedID;

	//----------------------------------
	//----------------------------------
	
	Flagship (void);

	virtual ~Flagship (void);	

	/* IBaseObject methods */

	virtual void TestVisible (const USER_DEFAULTS & defaults, const U32 currentSystem, const U32 currentPlayer)
	{
		if ((bVisible = !bAttached) != 0)
			SpaceShip<FLAGSHIP_SAVELOAD, FLAGSHIP_INIT>::TestVisible(defaults, currentSystem, currentPlayer);
	}

	virtual void SetTerrainFootprint (struct ITerrainMap * terrainMap)
	{
		if (bAttached==false)
			SpaceShip<FLAGSHIP_SAVELOAD, FLAGSHIP_INIT>::SetTerrainFootprint(terrainMap);
		else
			undoFootprintInfo(terrainMap);
	}

	/* IAdmiral methods */

	virtual void DockFlagship (IBaseObject * target, U32 agentID);

	virtual void UndockFlagship (U32 agentID, U32 _dockshipID);

	virtual void OnFleetShipDamaged (IBaseObject * victim, U32 attackerID);

	virtual void OnFleetShipDestroyed (IBaseObject * victim);

	virtual void OnFleetShipTakeover (IBaseObject * victim);

	virtual IBaseObject * FleetShipTargetDestroyed (IBaseObject * ship);		// admiral should return a new target

	virtual U32  GetFleetMembers (U32 objectIDs[MAX_SELECTED_UNITS]);

	virtual void SetFleetMembers (U32 objectIDs[MAX_SELECTED_UNITS], U32 numUnits);

	virtual void RemoveShipFromFleet (U32 shipID);

	virtual U32 GetAdmiralHotkey (void)
	{
		return admiralHotkey;
	}

	virtual void SetAdmiralHotkey(U32 hotkey)
	{
		admiralHotkey = (BASE_FLAGSHIP_SAVELOAD::AdmiralHotkey)hotkey;
	}

	virtual BOOL32 IsDocked (void)
	{
		return bAttached;
	}

	virtual IBaseObject * GetDockship (void)
	{
		if (dockship) 
		{
			return dockship.Ptr();
		}
		else
		{
			return NULL;
		}
	}


	virtual SINGLE GetDamageBonus(M_OBJCLASS sourceMObjClass,ARMOR_TYPE sourceArmor,OBJCLASS targetObjClass,M_RACE targetRace, ARMOR_TYPE targetArmor);

	virtual SINGLE GetDefenceBonus(M_OBJCLASS targetMObjClass, ARMOR_TYPE targetArmor);

	virtual SINGLE GetFighterDamageBonus();

	virtual SINGLE GetSpeedBonus(M_OBJCLASS shipMObjClass, ARMOR_TYPE shipArmor);

	virtual U32 ConvertSupplyBonus(M_OBJCLASS shipMObjClass, ARMOR_TYPE shipArmor, U32 suppliesUsed);

	virtual SINGLE GetSensorBonus(M_OBJCLASS shipMObjClass, ARMOR_TYPE shipArmor);

	virtual SINGLE GetFighterTargetingBonus();

	virtual SINGLE GetFighterDodgeBonus();

	virtual SINGLE GetRangeBonus(M_OBJCLASS shipMObjClass, ARMOR_TYPE shipArmor);

	virtual bool IsInFleetRepairMode();

	virtual U32 GetToolbarImage();

	virtual HBTNTXT::BUTTON_TEXT GetToolbarText();

	virtual HBTNTXT::HOTBUTTONINFO GetToolbarStatusText();
	
	virtual HBTNTXT::MULTIBUTTONINFO GetToolbarHintbox ();

	virtual void SetAdmiralTactic (ADMIRAL_TACTIC tactic);

	virtual ADMIRAL_TACTIC GetAdmiralTactic ();

	virtual void SetFormation (U32 formationArchID);

	virtual U32 GetFormation ();

	virtual U32 GetKnownFormation(U32 index);

	virtual void LearnCommandKit(const char * kitName);

	virtual void RenderFleetInfo();

	virtual void HandleMoveCommand(const NETGRIDVECTOR &vec);

	virtual void HandleAttackCommand(U32 targetID, U32 destSystemID);

	virtual bool IsInLockedFormation();

	virtual Vector GetFormationDir();

	virtual void MoveDoneHint(IBaseObject * ship);

	virtual struct BT_COMMAND_KIT * GetAvailibleCommandKit(U32 index);

	virtual U32 GetAvailibleCommandKitID(U32 index);
	
	virtual bool IsKnownKit(struct BT_COMMAND_KIT * comKit);

	virtual struct BT_COMMAND_KIT * GetKnownCommandKit(U32 index);

	virtual bool CanResearchKits();

	/* ISaveLoad methods */

	virtual BOOL32 Save (struct IFileSystem * file);

	virtual BOOL32 Load (struct IFileSystem * file);

	/* IGotoPos methods */

	virtual void GotoPosition (const GRIDVECTOR & pos, U32 agentID, bool bSlowMove);

	virtual void PrepareForJump (IBaseObject * jumpgate, bool bUserMove, U32 agentID, bool bSlowMove);

	virtual void UseJumpgate (IBaseObject * outgate, IBaseObject * ingate, const Vector& jumpToPosition, SINGLE heading, SINGLE speed, U32 agentID);

	virtual void RepairYourselfAt (IBaseObject * platform, U32 agentID);

	virtual void Patrol (const GRIDVECTOR & src, const GRIDVECTOR & dst, U32 agentID)
	{
		SpaceShip<FLAGSHIP_SAVELOAD, FLAGSHIP_INIT>::Patrol(src, dst, agentID);
	}


	/* IMissionActor methods */

	virtual void OnMasterChange(bool bIsMaster)
	{
		repairMasterChange(bIsMaster);
	};

	/* IBuildQueue */
	virtual void BuildQueue (U32 dwArchetypeDataID, COMMANDS command);

	virtual SINGLE FabGetDisplayProgress(U32 & stallType);

	/* IFabricator */ //not realy used but needed for template

	virtual void LocalStartBuild(){};

	virtual void LocalEndBuild(bool killAgent = false){};

	virtual void BeginConstruction (U32 planetID, U32 slotID, U32 dwArchetypeDataID, U32 agentID){};

	virtual void BeginConstruction (GRIDVECTOR position, U32 dwArchetypeDataID, U32 agentID){};

	virtual void BeginConstruction (IBaseObject * jumpgate, U32 dwArchetypeDataID, U32 agentID){};

	virtual void BeginRepair (U32 agentID, IBaseObject * repairTarget){};

	virtual void BeginDismantle(U32 agentID, IBaseObject * dismantleTarget){};

	virtual void ParkYourself (const TRANSFORM & _trans, U32 planetID, U32 slotID){};

	virtual bool IsBuildingAgent (U32 agentID){return false;};

	virtual U8 GetFabTab(){return 0;};

	virtual void SetFabTab(U8 tab){};

	virtual bool IsFabAtObj(){return true;};

	/* IBaseObject methods */


	// called in response to OpAgent::SendOperationData()
	void receiveOperationData (U32 agentID, void *buffer, U32 bufferSize);

	// user has requested a different action
	void onOperationCancel (U32 agentID);

	/* SpaceShip methods */
	
	virtual const char * getSaveStructName (void) const		// must be overriden implemented by derived class
	{
		return "FLAGSHIP_SAVELOAD";
	}
	virtual void * getViewStruct (void) 	// must be overriden implemented by derived class
	{
		return static_cast<BASE_FLAGSHIP_SAVELOAD *>(this);
	}

	/* Flagship methods */

	BOOL32 updateFlagship (void);
	
	void physUpdateFlagship (SINGLE dt);

	void renderFormationInfo (void);

	void explodeFlagship (bool bExplode);

	void destroyLaunchers (void);

	void save (FLAGSHIP_SAVELOAD & save);

	void load (FLAGSHIP_SAVELOAD & load);

	void resolve (void);

	void initFlagship (const FLAGSHIP_INIT & data);

	void doMove (void);

	bool findAutoTarget (U32 *pTarget=0);

	void updateSpeech (void);

	void doDefend (void);

	void resetMode (void)
	{
		mode = NOMODE;
		idleTimer = IDLE_TICKS;
		targetID = 0;
		target = 0;
	}

	void onTargetCancelledOp (void)
	{
		bool bIsMaster = THEMATRIX->IsMaster();

		// dockship is doing something else
		if (bIsMaster)
		{
			THEMATRIX->SendOperationData(dockAgentID, dwMissionID, &bIsMaster, sizeof(bIsMaster));	// signal cancel
			THEMATRIX->OperationCompleted(dockAgentID, dockshipID);
			dockshipID = 0;
			THEMATRIX->OperationCompleted2(dockAgentID, dwMissionID);
		}
		
		if (playerID == MGlobals::GetThisPlayer())
		{
			FLAGSHIPALERT(transferfailed, SUB_ADMIRAL_TRANS_FAIL);
		}
		
		if (bIsMaster && MISSION->IsComputerControlled(playerID) == false)
		{
			findBestDockship();		// jump on some other ship
		}
	}

	bool findBestDockship (IBaseObject * objIgnore = NULL);

	void doSyncData (void * data, U32 dataSize);

	void clearFleet (void);

	void attachToDockShip (const MPartNC & part);

	void doIdleBehavior (void);

	void smartSupplyShips (void);

	void smartFleetTargetSelection (void);

	void smartUnitStances (void);

	void queryIdleGunboats (void);

	void queryIdleGunboats (FleetGroup * group);

	void queryIdleShips (void);

	void querySupplyShips (void);

	void moveIdleShips (bool bUpdate = true);

	void preSelfDestruct (void);

	U32 getSyncStance (void * buffer);
	void putSyncStance (void * buffer, U32 bufferSize, bool bLateDelivery);

	U32 getSyncFormation (void * buffer);
	void putSyncFormation (void * buffer, U32 bufferSize, bool bLateDelivery);

	U32 getSyncKit (void * buffer);
	void putSyncKit (void * buffer, U32 bufferSize, bool bLateDelivery);

	void checkTargetPointers (void);

	void addUnitToFleet (U32 unitID);

	void findBestTargets (OBJPTR<IBaseObject> * targetList, Vector searchPos, bool bSeekRange = false);

	bool findPersonalTarget (IBaseObject * gunboat);

	bool findPersonalUpdateTarget (IBaseObject * gunboat);

	GRIDVECTOR findSeekPoint (GRIDVECTOR searchCenter, Vector * avoidList, U32 numAvoidPoints);

	GRIDVECTOR findSpotterPoint (GRIDVECTOR searchCenter, Vector * avoidList, U32 numAvoidPoints);

	void queryGunboats (void);

	void computeDefaultFormationPost(void);

	void reformFormation(void);

	void computeGroupSizes(void);

	void createGroupPosts(void);

	void findBasePost(FleetGroup * search);

	void orderGroupPositions(bool bSlow);

	FleetGroup * findGroupByTypeID(U32 id);

	void deleteFormation(void);

	void fillGroupMin(IBaseObject ** unassignedShips, U32 numShips,FleetGroup * newGroup);
	
	U32 countValidShips(FleetGroup * newGroup,DWORD positive_filter, DWORD negative_filter);

	S32 getNextValidShipIndex(IBaseObject ** unassignedShips, U32 numShips, DWORD positiveFilterBits, DWORD negativeFilterBits);

	U32 findHighestPriorityGroup(IBaseObject ** unassignedShips, U32 numShips,bool parentTest = true);

	U32 countExistingGroup(U32 groupDefCount);

	void findBestAssignment(IBaseObject * ship);

	void updateFormationAI();

	void updateGroupAI();

	void updateGroupNormal(FleetGroup * group);

	void updateGroupRover(FleetGroup * group);

	void updateGroupSpotter(FleetGroup * group);

	void updateGroupStriker(FleetGroup * group);

	bool isGroupAtTarget(FleetGroup * group);

	void moveGroupTo(FleetGroup * search, NETGRIDVECTOR position,Vector faceDir);

	FleetGroup * findFleetGroupFor(U32 dwMissionID);

	void findNextJumpGate(U32 sourceSystemID);

	bool isFormationForming(bool bPartial = false);

	void jumpOutGate();

	NETGRIDVECTOR getCurrentFleetPosition();

	void updateFormationPost();

	SINGLE findShortestOptimalRange(FleetGroup * group);
};

//---------------------------------------------------------------------------
//
Flagship::Flagship (void) : 
			updateNode(this, UpdateProc(&Flagship::updateFlagship)),
			explodeNode(this, ExplodeProc(&Flagship::explodeFlagship)),
			saveNode(this, CASTSAVELOADPROC(&Flagship::save)),
			loadNode(this, CASTSAVELOADPROC(&Flagship::load)),
			resolveNode(this, ResolveProc(&Flagship::resolve)),
			initNode(this, CASTINITPROC(&Flagship::initFlagship)),
			physUpdateNode(this, PhysUpdateProc(&Flagship::physUpdateFlagship)),
			onOpCancelNode(this, OnOpCancelProc(&Flagship::onOperationCancel)),
			receiveOpDataNode(this, ReceiveOpDataProc(&Flagship::receiveOperationData)),
			destructNode(this, PreDestructProc(&Flagship::preSelfDestruct)),
			genSyncNode1(this, SyncGetProc(&Flagship::getSyncStance), SyncPutProc(&Flagship::putSyncStance)),
			genSyncNode2(this, SyncGetProc(&Flagship::getSyncFormation), SyncPutProc(&Flagship::putSyncFormation)),
			genSyncNode3(this, SyncGetProc(&Flagship::getSyncKit), SyncPutProc(&Flagship::putSyncKit)),
			renderNode(this, RenderProc(&Flagship::renderFormationInfo))
{
}
//---------------------------------------------------------------------------
//
Flagship::~Flagship (void)
{
	// cannot explode the flagship here! we don't want to run explosion code when the 
	// mission is ending!

	MPartNC part = dockship.Ptr();
	if (part)
		part->admiralID = 0;		// break the connection
	bAttached = false;

	clearFleet();

	deleteFormation();
}
//---------------------------------------------------------------------------
//
void Flagship::GotoPosition (const GRIDVECTOR & pos,  U32 agentID, bool bSlowMove)
{
	if (bAttached)
	{
		CQASSERT(THEMATRIX->IsMaster()==0);
		UndockFlagship(0, dockshipID);
	}

	SpaceShip<FLAGSHIP_SAVELOAD, FLAGSHIP_INIT>::GotoPosition(pos, agentID, bSlowMove);
}
//---------------------------------------------------------------------------
//
void Flagship::PrepareForJump (IBaseObject * jumpgate, bool bUserMove, U32 agentID, bool bSlowMove)
{
	if (bAttached)
	{
		CQASSERT(THEMATRIX->IsMaster()==0);
		UndockFlagship(0, dockshipID);
	}

	SpaceShip<FLAGSHIP_SAVELOAD, FLAGSHIP_INIT>::PrepareForJump(jumpgate, bUserMove, agentID, bSlowMove);
}
//---------------------------------------------------------------------------
//
void Flagship::UseJumpgate (IBaseObject * outgate, IBaseObject * ingate, const Vector& jumpToPosition, SINGLE heading, SINGLE speed, U32 agentID)
{
	if (bAttached)
	{
		CQASSERT(THEMATRIX->IsMaster()==0);
		UndockFlagship(0, dockshipID);
	}

	SpaceShip<FLAGSHIP_SAVELOAD, FLAGSHIP_INIT>::UseJumpgate(outgate, ingate, jumpToPosition, heading, speed, agentID);
}
//---------------------------------------------------------------------------
//
void Flagship::RepairYourselfAt (IBaseObject * platform, U32 agentID)
{
	if (bAttached)
	{
		CQASSERT(THEMATRIX->IsMaster()==0);
		UndockFlagship(0, dockshipID);
	}

	SpaceShip<FLAGSHIP_SAVELOAD, FLAGSHIP_INIT>::RepairYourselfAt(platform, agentID);
}
//---------------------------------------------------------------------------
//
void Flagship::BuildQueue (U32 dwArchetypeDataID, COMMANDS command)
{
	if(THEMATRIX->IsMaster())
	{
		switch(command)
		{
		case IBuildQueue::COMMANDS::ADDIFEMPTY:		
			if (buildQueueEmpty() == false)
				break;
			//fall through intentional
		case IBuildQueue::COMMANDS::ADD:
			{
				if(CanResearchKits())
				{
					BASIC_DATA * buildType = (BASIC_DATA *)(ARCHLIST->GetArchetypeData(dwArchetypeDataID));
					if(buildType->objClass == OC_RESEARCH)
					{
						BASE_RESEARCH_DATA * resType = (BASE_RESEARCH_DATA *)buildType;
						if(resType->type == RESEARCH_COMMAND_KIT)
						{
							BT_COMMAND_KIT * btKitData = (BT_COMMAND_KIT *)resType;
							if((!isInQueue(dwArchetypeDataID)) && (!(IsKnownKit(btKitData))))
							{
								M_RESOURCE_TYPE failType;
								if(BANKER->SpendMoney(playerID,resType->cost,&failType))
								{
									syncQueueAddData = dwArchetypeDataID;
									bSyncQueueAdd = true;
									addToQueue(dwArchetypeDataID);
									THEMATRIX->ForceSyncData(this);
								}
								else
								{
									failSound(failType);
									bSyncQueueSound = true;
									syncFailSound = failType;
									THEMATRIX->ForceSyncData(this);
								}
							}
						}
					}
				}
				break;
			}
		case IBuildQueue::COMMANDS::REMOVE:
			{
				if(bReady)
				{
					U32 slotID = dwArchetypeDataID;//realy the slotID in disguise
					U32 realArchID = findQueueValue(slotID);
					if(realArchID != -1)
					{
						BASIC_DATA * buildType = (BASIC_DATA *)(ARCHLIST->GetArchetypeData(realArchID));
						if(buildType->objClass == OC_RESEARCH)
						{
							BASE_RESEARCH_DATA * resType = (BASE_RESEARCH_DATA *)buildType;
							BANKER->AddResource(playerID,resType->cost);
						}
						U32 queuePos = getQueuePositionFromIndex(slotID);
						if(queuePos ==0)
							buildTime = 0;
						bSyncQueueRemove = true;
						syncQueueRemoveData = slotID;
						removeIndexFromQueue(slotID);//should never delete the working project because we don't have a real agentID
					}
				}
				break;
			}
		case IBuildQueue::COMMANDS::PAUSE:
			{
				break;
			}
		}
	}
}
//---------------------------------------------------------------------------
//
SINGLE Flagship::FabGetDisplayProgress(U32 & stallType)
{
	stallType = IActiveButton::NO_STALL;
	if(!buildQueueEmpty())
	{
		U32 buildID = peekFabQueue();
		BT_COMMAND_KIT * kit = (BT_COMMAND_KIT *)(ARCHLIST->GetArchetypeData(buildID));
		if(kit)
		{
			return buildTime/kit->time;
		}
	}
	return 0.0f;
}
//---------------------------------------------------------------------------
//
void Flagship::DockFlagship (IBaseObject * _target, U32 agentID)
{
	if (bAttached)
		UndockFlagship(0, dockshipID);
	dockAgentID = agentID;
	CQASSERT(bAttached == false);

	if (_target == 0)
	{
		onTargetCancelledOp();
	}
	else
	{
		_target->QueryInterface(IFleetShipID, dockship, playerID);
		CQASSERT(dockship!=0);
		dockshipID = dockship.Ptr()->GetPartID();

		if (dockship.Ptr()->GetSystemID() != systemID)
		{
			onTargetCancelledOp();
		}
		else
		{
			GRIDVECTOR vec;
			vec = dockship.Ptr()->GetTransform().translation;
			moveToPos(vec, 0);

			dockship->WaitForAdmiral(true);

			// we are going to dock unto a ship, do we need to set in the fleet?
			addUnitToFleet(dockshipID);
		}
	}
}
//------------------------------------------------------------------------------------------
//
void Flagship::addUnitToFleet (U32 unitID)
{
	// make sure we have both the flagshipID and the missionID added to the fleetset
	// the add will return false if the object is already in the list
	if (fleetSet.addObject(dwMissionID))
	{
		fleetID = dwMissionID;
	}
	fleetSet.addObject(unitID);
}
//------------------------------------------------------------------------------------------
//
void Flagship::UndockFlagship (U32 agentID, U32 _dockshipID)
{
	CQASSERT(dockAgentID==0);

	MPartNC part = dockship.Ptr();
	if (part)
		part->admiralID = 0;		// break the connection
	bAttached = false;
	bMoveDisabled = false;
	disableAutoMovement();
	dockship = 0;
	THEMATRIX->OperationCompleted(agentID, _dockshipID);
	dockshipID = 0;
	THEMATRIX->OperationCompleted2(agentID, dwMissionID);

	moveToPos(GetGridPosition());
}
//------------------------------------------------------------------------------------------
//
void Flagship::OnFleetShipDamaged (IBaseObject * victim, U32 attackerID)
{
	if (targetID == 0 && mode != MOVING)
	{
		MPart part = targets[0];
		if (part.isValid() == 0)
		{
			idleTimer = -1;
		}
	}
}
//---------------------------------------------------------------------------
//
void Flagship::OnFleetShipDestroyed (IBaseObject * victim)
{
	numDeadShips++;

	MPart part = victim;

	// update the fleet set
	fleetSet.removeObject(victim->GetPartID());

	// has the dockship been destroyed?
	if (bAttached)
	{
		if (part->admiralID == dwMissionID)
		{
			// aaaay! The dockship has been destroyed, find another one
			UndockFlagship(0, dockshipID);
			if (THEMATRIX->IsMaster() && MISSION->IsComputerControlled(playerID) == false)
			{
				findBestDockship();
			}
		}
	}
	else
	{
		// were we heading towards a dockship that's been destroyed
		if (dockship.Ptr())
		{
			if (dockship.Ptr()->GetPartID() == dockshipID)
			{
				// find a new dockship
				if (THEMATRIX->IsMaster() && MISSION->IsComputerControlled(playerID) == false)
				{
					findBestDockship(dockship.Ptr());
				}
			}
		}
	}

	//remove him from the formation group list if any.
	FleetGroup * search = fleetGroups;
	bool bFound = false;
	while(search)
	{
		FleetMember * member = search->memberList;
		FleetMember * prevMember = NULL;
		while(member)
		{
			if(member->partID == victim->GetPartID())
			{
				if(prevMember)
					prevMember->next = member->next;
				else
					search->memberList = member->next;
				releaseFleetMember(member);
				bFound = true;
				break;
			}
			prevMember = member;
			member = member->next;
		}
		if(bFound)
			break;
		search = search->next;
	}
}
//---------------------------------------------------------------------------
//
void Flagship::OnFleetShipTakeover (IBaseObject * victim)
{
	numDeadShips++;
	MPartNC part = victim;

	// update the fleet set
	fleetSet.removeObject(victim->GetPartID());

	// zero out the victims fleetID and groupID
	part->groupID = 0;
	part->fleetID = 0;

	// has the dockship been taken over?
	if (bAttached)
	{
		if (part->admiralID == dwMissionID)
		{
			// there is an attempt to takeover the dockship, blow up the dockship and the flagship
			if (THEMATRIX->IsMaster())
			{
				// send a comm message telling us we're going to scuttle the ship
				// the troopship code will handle the deaths
			}
		}
	}
}
//---------------------------------------------------------------------------
//
void Flagship::queryGunboats (void)
{
	U32 i = 0;
	MPart part;
	
	nTotalGunboats = 0;

	for (i = 0; i < fleetSet.numObjects; i++)
	{
		part = OBJLIST->FindObject(fleetSet.objectIDs[i]);
		
		if (part.isValid() && MGlobals::IsGunboat(part->mObjClass))
		{
			nTotalGunboats++;
		}
	}
}
//---------------------------------------------------------------------------
//
void Flagship::computeDefaultFormationPost(void)
{
	Vector average(0,0,0);
	U32 counter = 0;
	for(U32 i = 0; i < fleetSet.numObjects; ++ i)
	{
		if(fleetSet.objectIDs[i] == GetPartID() && dockshipID)
		{
			continue;
		}
		IBaseObject * ship = OBJLIST->FindObject(fleetSet.objectIDs[i]);
		if(ship)
		{
			if(ship->GetSystemID() == GetSystemID())
			{
				average += ship->GetPosition();
				++counter;
			}
		}
	}
	if(counter == 0)
	{
		targetFormationPost.init(GetPosition(),GetSystemID());
	}
	else
	{
		targetFormationPost.init(average/counter,GetSystemID());
	}

	formationPost = targetFormationPost;

	RECT rect;
	SECTOR->GetSystemRect(GetSystemID(),&rect);
	Vector centerSys((rect.right-rect.left)/2,(rect.top-rect.bottom)/2,0);
	Vector dir = centerSys-formationPost;
	dir.fast_normalize();
	targetFormationDirX = formationDirX = (S8)(dir.x*127);
	targetFormationDirY = formationDirY = (S8)(dir.y*127);
	bHaveFormationPost = true;
}
//---------------------------------------------------------------------------
//
void Flagship::reformFormation(void)
{
//clean up
	deleteFormation();
//allocation and assignment of groups
	//get all ships but the admiral ship and the ship he is on into the unassigned list.
	IBaseObject * unassignedShips[MAX_SELECTED_UNITS];
	U32 numShips = 0;
	U32 i;
	for(i = 0; i < fleetSet.numObjects; ++i)
	{
		if(fleetSet.objectIDs[i] != GetPartID() && ((!bAttached) || fleetSet.objectIDs[i] != dockshipID))
		{
			unassignedShips[numShips] = OBJLIST->FindObject(fleetSet.objectIDs[i]);
			++numShips;
		}
	}

	//put the admiral and his docked ship into the main group.
	FleetGroup * newGroup = getNewFleetGroup();
	fleetGroups = newGroup;
	newGroup->next = NULL;
	newGroup->pData = &(pFormation->groups[0]);
	newGroup->groupID = 0;
	newGroup->AddMember(GetPartID());
	if(dockshipID)
		newGroup->AddMember(dockshipID);

	//attempt to fill the main groups min values

	fillGroupMin(unassignedShips,numShips,newGroup);

	//now fill out the groups (repeat this until no more groups can be created
		//find the group with the highest priority and all prerequisits met
		//fill it with the minimum ships
		//repeat if posible
	U32 priorityGroup = findHighestPriorityGroup(unassignedShips,numShips);
	while(priorityGroup != INVALID_GROUP_ID)
	{
		FleetGroup * newGroup = getNewFleetGroup();
		newGroup->next = fleetGroups;
		newGroup->pData = &(pFormation->groups[priorityGroup]);
		newGroup->groupID = priorityGroup;
		fleetGroups = newGroup;
		fillGroupMin(unassignedShips,numShips,newGroup);
		priorityGroup = findHighestPriorityGroup(unassignedShips,numShips);
	}

	//now try to create groups with the bOverflowCreation flag set
		//find the group with the highest priority and all prerequisits met (excuding the parent requirement)
		//fill it with the minimum ships
		//repeat if posible
	priorityGroup = findHighestPriorityGroup(unassignedShips,numShips,false);
	while(priorityGroup != INVALID_GROUP_ID)
	{
		FleetGroup * newGroup = getNewFleetGroup();
		newGroup->next = fleetGroups;
		newGroup->pData = &(pFormation->groups[priorityGroup]);
		newGroup->groupID = priorityGroup;
		fillGroupMin(unassignedShips,numShips,newGroup);
		priorityGroup = findHighestPriorityGroup(unassignedShips,numShips,false);
	}

	//now try to place the left over ships into groups evenly.  The overflow ships flag is a low priority to fill.
	for(i = 0; i < numShips; ++i)
	{
		if(unassignedShips[i])
		{
			findBestAssignment(unassignedShips[i]);
		}
	}

//placement of groups

	//compute teh bounding box for each group;
	computeGroupSizes();

	createGroupPosts();

// new orders

	orderGroupPositions(false);
}
//---------------------------------------------------------------------------
//
void Flagship::computeGroupSizes(void)
{
	FleetGroup * search = fleetGroups;
	while(search)
	{
		FleetMember * member = search->memberList;
		SINGLE area = 0;
		while(member)
		{
			IBaseObject * obj = OBJLIST->FindObject(member->partID);
			if(obj)
			{
				VOLPTR(IGotoPos) go = obj;
				if(go)
				{
					if(go->IsHalfSquare())
						area += 1;
					else
						area += 4;
				}
			}
			member = member->next;
		}
		area = (area/4);
		search->radius = sqrt(area/PI)+1;
		search->postX = 0;
		search->postY = 0;
		search->bPostValid = false;
		search = search->next;
	}
}
//---------------------------------------------------------------------------
//
void Flagship::createGroupPosts(void)
{
	bool bChange = true;
	while(bChange)
	{
		bChange = false;
		FleetGroup * search = fleetGroups;
		while(search)
		{
			if(!(search->bPostValid))
			{
				switch(search->pData->advancedPlacement.placementType)
				{
				case FleetGroupDef::AdvancedPlacement::PT_NORMAL:
					{
						findBasePost(search);
						if(search->bPostValid)
							bChange = true;
					}
					break;
				case FleetGroupDef::AdvancedPlacement::PT_LINE:
					{
						// placementID order  
						// 4 2 0 1 3 .. etc
						FleetGroup * pSearch = fleetGroups;
						FleetGroup * sibling = NULL;
						FleetGroup * highest = NULL;
						//look for our sibling
						while(pSearch)
						{
							if(pSearch->bPostValid && pSearch->groupID == search->groupID)
							{
								if(!highest)
									highest = pSearch;
								else if(highest->placementID < pSearch->placementID) 
								{
									if(!sibling)
										sibling = highest;
									else if(sibling->placementID < highest->placementID)
										sibling = highest;
									highest = pSearch;
								}
								else if(!sibling)
									sibling = highest;
								else if(sibling->placementID < highest->placementID)
									sibling = highest;
							}
							pSearch = pSearch->next;
						}
						if(!highest)//there is no sibling, we are the first one
						{
							findBasePost(search);
							if(search->bPostValid)
							{
								search->placementID = 0;
								bChange = true;
							}
						}
						else
						{
							if(!sibling)//for the sibling 0 case
								sibling = highest;
							Vector dir(search->pData->advancedPlacement.placementDirX,search->pData->advancedPlacement.placementDirY,0);
							dir.fast_normalize();
							if((highest->placementID == 0) || (sibling->placementID%2))//do I go to the right
							{
								dir = -dir;
							}
							search->postX = sibling->postX+(dir.x*(search->radius+sibling->radius));
							search->postY = sibling->postY+(dir.y*(search->radius+sibling->radius));
							search->bPostValid = true;
							search->placementID = highest->placementID+1;
							bChange = true;
						}
					}
					break;
				case FleetGroupDef::AdvancedPlacement::PT_LINE_END:
					{
						// placementID order  
						// 0 1 2 3 4 .. etc
						FleetGroup * pSearch = fleetGroups;
						FleetGroup * highest = NULL;
						//look for our sibling
						while(pSearch)
						{
							if(pSearch->bPostValid && pSearch->groupID == search->groupID)
							{
								if(!highest)
									highest = pSearch;
								else if(highest->placementID < pSearch->placementID) 
								{
									highest = pSearch;
								}
							}
							pSearch = pSearch->next;
						}
						if(!highest)//there is no sibling, we are the first one
						{
							findBasePost(search);
							if(search->bPostValid)
							{
								search->placementID = 0;
								bChange = true;
							}
						}
						else
						{
							Vector dir(search->pData->advancedPlacement.placementDirX,search->pData->advancedPlacement.placementDirY,0);
							dir.fast_normalize();
							search->postX = highest->postX+(dir.x*(search->radius+highest->radius));
							search->postY = highest->postY+(dir.y*(search->radius+highest->radius));
							search->bPostValid = true;
							search->placementID = highest->placementID+1;
							bChange = true;
						}
					}
					break;
				case FleetGroupDef::AdvancedPlacement::PT_V:
					{
						// placementID order  
						//      0
						//     1 2
						//    3   4
						//
						FleetGroup * pSearch = fleetGroups;
						FleetGroup * sibling = NULL;
						FleetGroup * highest = NULL;
						//look for our sibling
						while(pSearch)
						{
							if(pSearch->bPostValid && pSearch->groupID == search->groupID)
							{
								if(!highest)
									highest = pSearch;
								else if(highest->placementID < pSearch->placementID) 
								{
									if(!sibling)
										sibling = highest;
									else if(sibling->placementID < highest->placementID)
										sibling = highest;
									highest = pSearch;
								}
								else if(!sibling)
									sibling = highest;
								else if(sibling->placementID < highest->placementID)
									sibling = highest;
							}
							pSearch = pSearch->next;
						}
						if(!highest)//there is no sibling, we are the first one
						{
							findBasePost(search);
							if(search->bPostValid)
							{
								search->placementID = 0;
								bChange = true;
							}
						}
						else
						{
							if(!sibling)//for the sibling 0 case
								sibling = highest;
							Vector dir(search->pData->advancedPlacement.placementDirX,search->pData->advancedPlacement.placementDirY,0);
							dir.fast_normalize(); //forward on the V
							Vector side = cross_product(dir,Vector(0,0,1));
							side.fast_normalize();
							if((highest->placementID == 0) || (sibling->placementID%2))//do I go to the right
							{
								side = -side;
							}
							dir = (-dir)+side;
							dir.fast_normalize();
							search->postX = sibling->postX+(dir.x*(search->radius+sibling->radius));
							search->postY = sibling->postY+(dir.y*(search->radius+sibling->radius));
							search->bPostValid = true;
							search->placementID = highest->placementID+1;
							bChange = true;
						}
					}
					break;
				}
			}
			search = search->next;
		}
	}
}
//---------------------------------------------------------------------------
//
void Flagship::findBasePost(FleetGroup * search)
{
	if(search->pData->relativeTo == FleetGroupDef::RT_CENTER)
	{
		switch(search->pData->relation)
		{
		case FleetGroupDef::RE_CENTER:
			{
				search->postX = 0;
				search->postY = 0;
				search->bPostValid = true;
			}
			break;
		case FleetGroupDef::RE_EDGE:
			{
				Vector dir(search->pData->relDirX,search->pData->relDirY,0);
				dir.fast_normalize();
				search->postX = dir.x*search->radius;
				search->postY = dir.y*search->radius;
				search->bPostValid = true;
			}
			break;
		}

	}
	else if(search->pData->relativeTo == FleetGroupDef::RT_GROUP)
	{
		FleetGroup * parent = findGroupByTypeID(search->pData->relativeGroupID);
		if(parent && parent->bPostValid)
		{
			switch(search->pData->relation)
			{
			case FleetGroupDef::RE_CENTER:
				{
					search->postX = parent->postX;
					search->postY = parent->postY;
					search->bPostValid = true;
				}
				break;
			case FleetGroupDef::RE_EDGE:
				{
					Vector dir(search->pData->relDirX,search->pData->relDirY,0);
					dir.fast_normalize();
					search->postX = parent->postX+(dir.x*(search->radius+parent->radius));
					search->postY = parent->postY+(dir.y*(search->radius+parent->radius));
					search->bPostValid = true;
				}
				break;
			}				
		}
	}
}
//---------------------------------------------------------------------------
//
FleetGroup * Flagship::findGroupByTypeID(U32 id)
{
	FleetGroup * search = fleetGroups;
	while(search)
	{
		if(search->groupID == id)
			return search;
		search = search->next;
	}
	return NULL;
}
//---------------------------------------------------------------------------
//
void Flagship::orderGroupPositions(bool bSlow)
{
	TRANSFORM postTrans;
	postTrans.translation = formationPost;
	Vector i(formationDirX,formationDirY,0);
	i.fast_normalize();
	Vector k(0,0,1);
	Vector j = cross_product(k,i);
	postTrans.set_i(i);
	postTrans.set_j(j);
	postTrans.set_k(k);
	FleetGroup * search = fleetGroups;
	while(search)
	{
		Vector groupCenter(search->postX*GRIDSIZE,search->postY*GRIDSIZE,0);
		if(flipFormationPost)
			groupCenter.y *= -1.0;
		groupCenter = postTrans.rotate_translate(groupCenter);
		GRIDVECTOR grid;
		if(groupCenter.x < 0)
			groupCenter.x = 0;
		if(groupCenter.y < 0)
			groupCenter.y = 0;

		grid =groupCenter;
		FleetMember * member = search->memberList;
		while(member)
		{
			if(dockshipID && member->partID == GetPartID())
			{
				member = member->next;
			}
			else
			{
				search->ai.lastPos = formationPost;
				USR_PACKET<USRMOVE> packet;
				packet.objectID[0] = member->partID;
				packet.position.init(grid, formationPost.systemID);
				packet.userBits = 0;
				packet.ctrlBits = bSlow;
				packet.init(1);
				NETPACKET->Send(HOSTID, 0, &packet);
				member = member->next;
			}
		}
		search = search->next;
	}
}
//---------------------------------------------------------------------------
//
void Flagship::deleteFormation(void)
{
	while(fleetGroups)
	{
		FleetGroup * tmp = fleetGroups;
		fleetGroups =  fleetGroups->next;
		releaseFleetGroup(tmp);
	}
}
//---------------------------------------------------------------------------
//
void Flagship::fillGroupMin(IBaseObject ** unassignedShips, U32 numShips,FleetGroup * newGroup)
{
	for(U32 filterCount = 0; filterCount < MAX_SHIP_FILTERS; ++filterCount)
	{
		ShipFilters & filter = newGroup->pData->filters[filterCount];
		if((!(filter.bOverflowOnly)) && filter.min)
		{
			for(U32 shipCount = countValidShips(newGroup,filter.positiveFilterBits,filter.negativeFilterBits); shipCount < filter.min; ++ shipCount)
			{
				S32 shipIndex = getNextValidShipIndex(unassignedShips,numShips,filter.positiveFilterBits,filter.negativeFilterBits);
				if(shipIndex != -1)
				{
					newGroup->AddMember(unassignedShips[shipIndex]->GetPartID());
					unassignedShips[shipIndex] = NULL;
				}
				else
				{
					break;
				}
			}
		}
	}
}
//---------------------------------------------------------------------------
//
U32 Flagship::countValidShips(FleetGroup * newGroup,DWORD positive_filter, DWORD negative_filter)
{
	U32 count = 0;
	FleetMember * search = newGroup->memberList;
	while(search)
	{
		IBaseObject * ship = OBJLIST->FindObject(search->partID);
		if(ship)
		{
			if(ship->MatchesSomeFilter(positive_filter) && (!(ship->MatchesSomeFilter(negative_filter))))
			{
				++count;
			}
		}
		search = search->next;
	}
	return count;
}
//---------------------------------------------------------------------------
//
S32 Flagship::getNextValidShipIndex(IBaseObject ** unassignedShips, U32 numShips, DWORD positiveFilterBits, DWORD negativeFilterBits)
{
	for(S32 i = 0; i < (S32)numShips; ++i)
	{
		if(unassignedShips[i])
		{
			if(unassignedShips[i]->MatchesSomeFilter(positiveFilterBits) && (!(unassignedShips[i]->MatchesSomeFilter(negativeFilterBits))))
			{
				return i;
			}
		}
	}
	return -1;
}
//---------------------------------------------------------------------------
//
U32 Flagship::findHighestPriorityGroup(IBaseObject ** unassignedShips, U32 numShips,bool parentTest)
{
	U32 bestIndex = INVALID_GROUP_ID;
	for(U32 groupDefCount = 0; groupDefCount < MAX_GROUP_DEFS; ++ groupDefCount)
	{
		if(pFormation->groups[groupDefCount].bActive)
		{
			//compare to best priority
			if(bestIndex != INVALID_GROUP_ID)
			{
				if(pFormation->groups[groupDefCount].priority < pFormation->groups[bestIndex].priority)
					continue;//the best is better than me.
			}

			//test if I have too many of this group
			if(countExistingGroup(groupDefCount) >= pFormation->groups[groupDefCount].creationNumber)
				continue;

			//test for parent existence
			if(parentTest  && countExistingGroup(pFormation->groups[groupDefCount].parentGroup) < pFormation->groups[groupDefCount].parentGroupNumber)
				continue;

			//test for min ships for group
			bool bInsuficientShips = false;
			for(U32 filterCount = 0; filterCount < MAX_SHIP_FILTERS; ++filterCount)
			{
				if(pFormation->groups[groupDefCount].filters[filterCount].positiveFilterBits &&
					pFormation->groups[groupDefCount].filters[filterCount].min)
				{
					U32 shipAvail = 0;
					for(U32 shipIndexCount = 0; shipIndexCount < numShips; ++ shipIndexCount)
					{
						if(unassignedShips[shipIndexCount])
						{
							if(unassignedShips[shipIndexCount]->MatchesSomeFilter(pFormation->groups[groupDefCount].filters[filterCount].positiveFilterBits) &&
								(!(unassignedShips[shipIndexCount]->MatchesSomeFilter(pFormation->groups[groupDefCount].filters[filterCount].negativeFilterBits))))
							{
								++shipAvail;
							}
						}
					}
					if(shipAvail < pFormation->groups[groupDefCount].filters[filterCount].min)
					{
						bInsuficientShips = true;
						break;
					}
				}
			}
			if(!bInsuficientShips)
			{
				bestIndex = groupDefCount;
			}
		}
	}
	return bestIndex;
}
//---------------------------------------------------------------------------
//
U32 Flagship::countExistingGroup(U32 groupDefCount)
{
	U32 count = 0;
	FleetGroup * search = fleetGroups;
	while(search)
	{
		if(search->groupID == groupDefCount)
			++count;
		search = search->next;
	}
	return count;
}
//---------------------------------------------------------------------------
//
void Flagship::findBestAssignment(IBaseObject * ship)
{
	//find the group with the least number of matches for a matching filter

	U32 currentMin = 0;
	FleetGroup * bestGroup = NULL;

	FleetGroup * search = fleetGroups;
	while(search)
	{
		for(U32 filterCount = 0; filterCount < MAX_SHIP_FILTERS; ++filterCount)
		{
			if(search->pData->filters[filterCount].positiveFilterBits)
			{
				if(ship->MatchesSomeFilter(search->pData->filters[filterCount].positiveFilterBits) && (!(search->pData->filters[filterCount].negativeFilterBits)))
				{
					U32 currentCount = countValidShips(search,search->pData->filters[filterCount].positiveFilterBits,search->pData->filters[filterCount].negativeFilterBits);
					if(currentCount < search->pData->filters[filterCount].max)
					{
						if(bestGroup)
						{
							if(currentCount < currentMin)
							{
								bestGroup = search;
								currentMin = currentCount;
							}
						}
						else
						{
							bestGroup = search;
							currentMin = currentCount;
						}
					}
				}
			}
		}
		if(search->groupID == 0 && ! bestGroup)
		{
			currentMin = 10000;
			bestGroup = search;
		}
		search = search->next;
	}
	CQASSERT(bestGroup);//we should have at least found the main group
	if(bestGroup)
	{
		bestGroup->AddMember(ship->GetPartID());
	}
}
//---------------------------------------------------------------------------
//
void Flagship::updateFormationAI()
{
	if(pFormation)
	{
		//update cloaking usage
		//fill up the packet
		USR_PACKET<USRCLOAK> cloakPacket;
		U32 count = 0;
		switch(pFormation->cloakUsage)
		{
		case BT_FORMATION::CU_NONE:
			break;//do nothing
		case BT_FORMATION::CU_ALL:
			{
				for(U32 i = 0; i < fleetSet.numObjects; ++i)
				{
					IBaseObject * obj = OBJLIST->FindObject(fleetSet.objectIDs[i]);
					if(obj)
					{
						MPart part = obj;
						if(part->caps.cloakOk && part->supplies != 0 && (!(obj->bCloaked)))
						{
							cloakPacket.objectID[count] = fleetSet.objectIDs[i];
							++count;
						}
					}
				}
			}
			break;
		case BT_FORMATION::CU_RECON_ONLY:
			{
				FleetGroup * group = fleetGroups;
				while(group)
				{
					if(group->pData->aiType == FleetGroupDef::ROVER)
					{
						FleetMember * member = group->memberList;
						while(member)
						{
							IBaseObject * obj = OBJLIST->FindObject(member->partID);
							if(obj)
							{
								MPart part = obj;
								if(part->caps.cloakOk && part->supplies != 0 && (!(obj->bCloaked)))
								{
									cloakPacket.objectID[count] = member->partID;
									++count;
								}
							}
							member = member->next;
						}
					}
					group = group->next;
				}
			}
			break;
		case BT_FORMATION::CU_RECON_AND_SPOTTERS:
			{
				FleetGroup * group = fleetGroups;
				while(group)
				{
					if(group->pData->aiType == FleetGroupDef::ROVER || group->pData->aiType == FleetGroupDef::SPOTTER)
					{
						FleetMember * member = group->memberList;
						while(member)
						{
							IBaseObject * obj = OBJLIST->FindObject(member->partID);
							if(obj)
							{
								MPart part = obj;
								if(part->caps.cloakOk && part->supplies != 0 && (!(obj->bCloaked)))
								{
									cloakPacket.objectID[count] = member->partID;
									++count;
								}
							}
							member = member->next;
						}
					}
					group = group->next;
				}
			}
			break;
		}

		if(count)
		{
			cloakPacket.init(count);
			NETPACKET->Send(HOSTID, 0, &cloakPacket);
		}
	}

	//update supply usage

	S32 totalAvailibleSupplies = 0;
	S32 totalSupplyRoom = 0;
	FleetGroup * group = fleetGroups;
	while(group)
	{
		if(!group->ai.bRoving)
		{
			FleetMember * member = group->memberList;
			while(member)
			{
				IBaseObject * obj = OBJLIST->FindObject(member->partID);
				if(obj)
				{
					MPartNC part = obj;
					if(part.isValid() && MGlobals::IsSupplyShip(part->mObjClass))
					{
						totalAvailibleSupplies += part->supplies;
						totalSupplyRoom += part->supplyPointsMax;
					}
				}
				member= member->next;
			}
		}
		group = group->next;
	}
	if(totalAvailibleSupplies)
	{
		group = fleetGroups;
		while(group)
		{
			if(!group->ai.bRoving)
			{
				FleetMember * member = group->memberList;
				while(member)
				{
					IBaseObject * obj = OBJLIST->FindObject(member->partID);
					if(obj)
					{
						MPartNC part = obj;
						if(part.isValid() && (!(MGlobals::IsSupplyShip(part->mObjClass))))
						{
							S32 need = part->supplyPointsMax-part->supplies;
							if(need)
							{
								if(need > totalAvailibleSupplies)
								{
									need = totalAvailibleSupplies;
								}
								if(need)
								{
									totalAvailibleSupplies-= need;
									part->supplies += need;
								}
							}
						}
					}
					member= member->next;
				}
			}
			group = group->next;
		}
		FleetGroup * group = fleetGroups;
		while(group)
		{
			if(!group->ai.bRoving)
			{
				FleetMember * member = group->memberList;
				while(member)
				{
					IBaseObject * obj = OBJLIST->FindObject(member->partID);
					if(obj)
					{
						MPartNC part = obj;
						if(part.isValid() && MGlobals::IsSupplyShip(part->mObjClass))
						{
							SINGLE precent = part->supplyPointsMax;
							precent = precent/totalSupplyRoom;
							part->supplies = totalAvailibleSupplies*precent;
						}
					}
					member= member->next;
				}
			}
			group = group->next;
		}
	}

	//update formation attack target
	if(pFormation->bAdmiralAttackControl && (!formationTarget))
	{
		IBaseObject * bestTarg = NULL;
		SINGLE bestDist = 0;
		ObjMapIterator iter(formationPost.systemID,formationPost,64*GRIDSIZE,playerID);
		const U32 allyMask = MGlobals::GetAllyMask(playerID);
		while(iter)
		{
			if(iter->flags & OM_TARGETABLE && (!(iter->flags &OM_UNTOUCHABLE)))
			{
				const U32 hisPlayerID = iter.GetApparentPlayerID(allyMask);//it->dwMissionID & PLAYERID_MASK;
				// if we are enemies
				if (hisPlayerID && (((1 << (hisPlayerID-1)) & allyMask) == 0))
				{
					// as long as the enemy is visible, let's go for it!
					if (iter->obj->IsVisibleToPlayer(playerID))
					{
						if(bestTarg)
						{
							SINGLE newDist = iter->obj->GetGridPosition()-formationPost;
							if(newDist < bestDist)
							{
								bestDist = newDist;
								bestTarg = iter->obj;
							}
						}
						else
						{
							bestTarg = iter->obj;
							bestDist = iter->obj->GetGridPosition()-formationPost;
						}
					}
				}
			}
			if(bestTarg && bestDist < pFormation->moveTargetRange)
				break;//break early
			++iter;
		}
		if(bestTarg && bestDist > pFormation->moveTargetRange)
		{
			//order the attack
			USR_PACKET<FORMATIONATTACK> packet;

			packet.targetID = bestTarg->GetPartID();
			packet.destSystemID = bestTarg->GetSystemID();
			packet.objectID[0] = GetPartID();
			packet.init(1);

			NETPACKET->Send(HOSTID, 0, &packet);
		}

	}
}
//---------------------------------------------------------------------------
//
void Flagship::updateGroupAI()
{
	//set the stances for all formation members
	for(U32 i = 0; i <fleetSet.numObjects; ++i)
	{
		VOLPTR(IAttack) attacker = OBJLIST->FindObject(fleetSet.objectIDs[i]);
		if(attacker)
		{
			switch(admiralTactic)
			{
			case AT_PEACE:
				attacker->SetUnitStance(US_STOP);
				attacker->SetFighterStance(FS_NORMAL);
				break;
			case AT_DEFEND:
				attacker->SetUnitStance(US_STAND);
				attacker->SetFighterStance(FS_NORMAL);
				break;
			case AT_HOLD:
				attacker->SetUnitStance(US_STAND);
				attacker->SetFighterStance(FS_NORMAL);
				break;
			case AT_SEEK:
				attacker->SetUnitStance(US_STAND);
				attacker->SetFighterStance(FS_PATROL);
				break;
			}
		}
	}
	//do I need to move the whole fleet closer to our target
	if(formationTarget && ((formationTarget->GetSystemID() != targetFormationPost.systemID) || targetFormationPost-formationTarget->GetGridPosition() > pFormation->moveTargetRange))
	{
		if(formationTarget->GetSystemID() != formationPost.systemID)//move right to the target
		{
			if((formationTarget->GetSystemID() & HYPER_SYSTEM_MASK) != 0)
			{
				NETGRIDVECTOR vec;
				vec.init(formationTarget->GetGridPosition(),formationTarget->GetSystemID());

				USR_PACKET<USRFORMATIONMOVE> packet;

				packet.position = vec;
				packet.objectID[0] = GetPartID();
				packet.init(1);

				NETPACKET->Send(HOSTID, 0, &packet);
			}
		}
		else//move near the target
		{
			Vector currentPos;
			currentPos = getCurrentFleetPosition();
			Vector targetPos;
			targetPos = formationTarget->GetGridPosition();
			Vector dir = currentPos-targetPos;
			SINGLE mag = dir.fast_magnitude();
			Vector finalPos;
			if(mag < pFormation->moveTargetRange*GRIDSIZE)
			{
				finalPos = currentPos;
			}
			else
			{
				dir /= mag;
				finalPos = targetPos+(dir*pFormation->moveTargetRange*GRIDSIZE);
			}
			NETGRIDVECTOR vec;
			vec.init(finalPos,formationTarget->GetSystemID());

			USR_PACKET<USRFORMATIONMOVE> packet;

			packet.position = vec;
			packet.objectID[0] = GetPartID();
			packet.init(1);

			NETPACKET->Send(HOSTID, 0, &packet);
		}
	}
	else//go through each ships AI and update it
	{
		if((!isFormationForming(true)) || bForceUpdateFormationPost)
		{
			bForceUpdateFormationPost = false;
			updateFormationPost();
		}
		if(targetFormationPost.systemID == GetSystemID())
			bRovingAllowed = true;
		else
			bRovingAllowed = false;

		FleetGroup * search = fleetGroups;
		while(search)
		{
			switch(search->pData->aiType)
			{
			case FleetGroupDef::NORMAL:
				{
					updateGroupNormal(search);
				}
				break;
			case FleetGroupDef::ROVER:
				{
					updateGroupRover(search);
				}
				break;
			case FleetGroupDef::SPOTTER:
				{
					updateGroupSpotter(search);
				}
				break;
			case FleetGroupDef::STRIKER:
				{
					updateGroupStriker(search);
				}
				break;
			}
			search = search->next;
		}
	}
}
//---------------------------------------------------------------------------
//
void Flagship::updateGroupNormal(FleetGroup * group)
{
	if(group->ai.bPostMoved)//update our position
	{
		group->ai.bPostMoved = false;

		Vector i(formationDirX,formationDirY,0);
		i.fast_normalize();

		moveGroupTo(group,formationPost,i);
	}
	else //update our attack targets.
	{
		queryIdleGunboats(group);

		if (nAvailableGunboats != 0 || nTargetUpdateGunboats != 0)
		{
			findBestTargets(group->ai.targets,group->ai.lastPos);

			//update the targets
			if (group->ai.targets[0])
			{
				USR_PACKET<USRATTACK> packet1;
				USR_PACKET<USRATTACK> packet2;
				int cnt1 = 0;
				int cnt2 = 0;
				IBaseObject * targ = NULL;
				MPart part1 = group->ai.targets[1];

				for (U32 i = 0; i < nAvailableGunboats; i++)
				{
					// only assign attack commands if the ship is in the same system as the admiral
					IBaseObject * gunboat = OBJLIST->FindObject(idleGunboatArray[i]);
					targ = NULL;
					if (gunboat && gunboat->GetSystemID() == systemID)
					{
						if (part1.isValid() && nTotalGunboats > 3 && part1->caps.attackOk)
						{
							VOLPTR(ILaunchOwner) launchOwner = gunboat;
							if(launchOwner)
							{
								bool targ0Range = (launchOwner->GetWeaponRange()/GRIDSIZE > group->ai.targets[0]->GetGridPosition()-gunboat->GetGridPosition()) 
									&& launchOwner->TestLOS(group->ai.targets[0]->GetGridPosition());
								bool targ1Range = (launchOwner->GetWeaponRange()/GRIDSIZE > group->ai.targets[1]->GetGridPosition()-gunboat->GetGridPosition())
									&& launchOwner->TestLOS(group->ai.targets[1]->GetGridPosition());
								if(targ0Range && targ1Range)
									targ = group->ai.targets[i%2];
								else if(targ0Range)
									targ = group->ai.targets[0];
								else if(targ1Range)
									targ = group->ai.targets[1];
								else 
									findPersonalTarget(gunboat);
							}
							else
								targ = group->ai.targets[i%2];
						}
						else 
						{
							VOLPTR(ILaunchOwner) launchOwner = gunboat;
							if(launchOwner)
							{
								if(launchOwner->GetWeaponRange()/GRIDSIZE > group->ai.targets[0]->GetGridPosition()-gunboat->GetGridPosition()
									&& launchOwner->TestLOS(group->ai.targets[0]->GetGridPosition()))
									targ = group->ai.targets[0];
								else
									findPersonalTarget(gunboat);
							}
							else
								targ = group->ai.targets[0];
						}

						if (targ && targ == group->ai.targets[0])
						{
							packet1.objectID[cnt1++] = idleGunboatArray[i];
						}
						else if(targ && targ == group->ai.targets[1])
						{
							packet2.objectID[cnt2++] = idleGunboatArray[i];
						}
					}
				}

				if (cnt1)
				{
					packet1.targetID = group->ai.targets[0]->GetPartID();
					packet1.bUserGenerated = false;
					packet1.init(cnt1);
					NETPACKET->Send(HOSTID, 0, &packet1);
				}
				if (cnt2)
				{
					packet2.targetID = group->ai.targets[1]->GetPartID();
					packet2.bUserGenerated = false;
					packet2.init(cnt2);
					NETPACKET->Send(HOSTID, 0, &packet2);
				}

				//update working gunboats on server only, will propagate to clients
				for (U32 i = 0; i < nTargetUpdateGunboats; i++)
				{
					// only assign attack commands if the ship is in the same system as the admiral
					IBaseObject * gunboat = OBJLIST->FindObject(targetUpdateGunboatArray[i]);
					targ = NULL;
					if (gunboat && gunboat->GetSystemID() == systemID)
					{
						if (part1.isValid() && nTotalGunboats > 3 && part1->caps.attackOk)
						{
							VOLPTR(ILaunchOwner) launchOwner = gunboat;
							if(launchOwner.ptr)
							{
								bool targ0Range = (launchOwner->GetWeaponRange()/GRIDSIZE > group->ai.targets[0]->GetGridPosition()-gunboat->GetGridPosition())
									&& launchOwner->TestLOS(group->ai.targets[0]->GetGridPosition());
								bool targ1Range = (launchOwner->GetWeaponRange()/GRIDSIZE > group->ai.targets[1]->GetGridPosition()-gunboat->GetGridPosition())
									&& launchOwner->TestLOS(group->ai.targets[1]->GetGridPosition());
								if(targ0Range && targ1Range)
									targ = group->ai.targets[i%2];
								else if(targ0Range)
									targ = group->ai.targets[0];
								else if(targ1Range)
									targ = group->ai.targets[1];
								else 
									findPersonalUpdateTarget(gunboat);
							}
							else
								targ = group->ai.targets[i%2];
						}
						else 
						{
							VOLPTR(ILaunchOwner) launchOwner = gunboat;
							if(launchOwner)
							{
								if(launchOwner->GetWeaponRange()/GRIDSIZE > group->ai.targets[0]->GetGridPosition()-gunboat->GetGridPosition()
									&& launchOwner->TestLOS(group->ai.targets[0]->GetGridPosition()))
									targ = group->ai.targets[0];
								else
									findPersonalUpdateTarget(gunboat);
							}
							else
								targ = group->ai.targets[0];
						}
						if(targ)
						{
							VOLPTR(IFleetShip) ship = gunboat;
							if(ship)
								ship->SetFleetshipTarget(targ);
						}
					}
				}
			}
		}
	}
}
//---------------------------------------------------------------------------
//
void Flagship::updateGroupRover(FleetGroup * group)
{
	//update move info
	bool bUpdateMove = false;
	if(group->ai.bPostMoved)//update our position
	{
		group->ai.bPostMoved = false;
		if(!bRovingAllowed)
		{
			bUpdateMove = true;
		}
	}

	if(bUpdateMove)
	{
		group->ai.bRoving = false;
		Vector i(formationDirX,formationDirY,0);
		i.fast_normalize();

		moveGroupTo(group,formationPost,i);
	}
	else if(bRovingAllowed && ((!(group->ai.bRoving)) || isGroupAtTarget(group)))
	{
		Vector avoidList[16];
		U32 numAvoid = 0;
		FleetGroup * gSearch = fleetGroups;
		while(gSearch && numAvoid < 16)
		{
			if(gSearch->ai.bRoving && gSearch != group)
			{
				avoidList[numAvoid] = gSearch->ai.rovingPost;
				++numAvoid;
			}
			gSearch = gSearch->next;
		}
		if(group->ai.bRoving)
			group->ai.rovingPost.init(findSeekPoint(group->ai.rovingPost,avoidList,numAvoid),GetSystemID());
		else
			group->ai.rovingPost.init(findSeekPoint(group->ai.lastPos,avoidList,numAvoid),GetSystemID());

		group->ai.bRoving = true;

		FleetMember * member = group->memberList;
		USR_PACKET<USRMOVE> packet;
		packet.position = group->ai.rovingPost;
		packet.userBits = 0;
		packet.ctrlBits = true;
		U32 count = 0;
		while(member)
		{
			if(dockshipID && member->partID == GetPartID())
			{
				member = member->next;
			}
			else
			{
				packet.objectID[count] = member->partID;
				++count;
				member = member->next;
			}
		}
		if(count)
		{
			packet.init(count);
			NETPACKET->Send(HOSTID, 0, &packet);
		}
	}
	else //update our attack targets.
	{
		queryIdleGunboats(group);

		if (nAvailableGunboats != 0)
		{
			findBestTargets(group->ai.targets,group->ai.lastPos);

			//update the targets
			if (group->ai.targets[0])
			{
				USR_PACKET<USRATTACK> packet1;
				USR_PACKET<USRATTACK> packet2;
				int cnt1 = 0;
				int cnt2 = 0;
				IBaseObject * targ = NULL;
				MPart part1 = group->ai.targets[1];

				for (U32 i = 0; i < nAvailableGunboats; i++)
				{
					// only assign attack commands if the ship is in the same system as the admiral
					IBaseObject * gunboat = OBJLIST->FindObject(idleGunboatArray[i]);
					targ = NULL;
					if (gunboat && gunboat->GetSystemID() == systemID)
					{
						if (part1.isValid() && nTotalGunboats > 5 && part1->caps.attackOk)
						{
							VOLPTR(ILaunchOwner) launchOwner = gunboat;
							if(launchOwner)
							{
								bool targ0Range = (launchOwner->GetWeaponRange()/GRIDSIZE > group->ai.targets[0]->GetGridPosition()-gunboat->GetGridPosition())
									&& launchOwner->TestLOS(group->ai.targets[0]->GetGridPosition());
								bool targ1Range = (launchOwner->GetWeaponRange()/GRIDSIZE > group->ai.targets[1]->GetGridPosition()-gunboat->GetGridPosition())
									&& launchOwner->TestLOS(group->ai.targets[1]->GetGridPosition());
								if(targ0Range && targ1Range)
									targ = group->ai.targets[i%2];
								else if(targ0Range)
									targ = group->ai.targets[0];
								else if(targ1Range)
									targ = group->ai.targets[1];
								else 
									findPersonalTarget(gunboat);
							}
							else
								targ = group->ai.targets[i%2];
						}
						else 
						{
							VOLPTR(ILaunchOwner) launchOwner = gunboat;
							if(launchOwner)
							{
								if(launchOwner->GetWeaponRange()/GRIDSIZE > group->ai.targets[0]->GetGridPosition()-gunboat->GetGridPosition()
									&& launchOwner->TestLOS(group->ai.targets[0]->GetGridPosition()))
									targ = group->ai.targets[0];
								else
									findPersonalTarget(gunboat);
							}
							else
								targ = group->ai.targets[0];
						}

						if (targ && targ == group->ai.targets[0])
						{
							packet1.objectID[cnt1++] = idleGunboatArray[i];
						}
						else if(targ && targ == group->ai.targets[1])
						{
							packet2.objectID[cnt2++] = idleGunboatArray[i];
						}
					}
				}

				if (cnt1)
				{
					packet1.targetID = group->ai.targets[0]->GetPartID();
					packet1.bUserGenerated = false;
					packet1.init(cnt1);
					NETPACKET->Send(HOSTID, 0, &packet1);
				}
				if (cnt2)
				{
					packet2.targetID = group->ai.targets[1]->GetPartID();
					packet2.bUserGenerated = false;
					packet2.init(cnt2);
					NETPACKET->Send(HOSTID, 0, &packet2);
				}
			}
		}
	}
}
//---------------------------------------------------------------------------
//
void Flagship::updateGroupSpotter(FleetGroup * group)
{
	//update move info
	bool bUpdateMove = false;
	if(group->ai.bPostMoved)//update our position
	{
		group->ai.bPostMoved = false;
		if(!bRovingAllowed)
		{
			bUpdateMove = true;
		}
	}

	if(bUpdateMove)
	{
		group->ai.bRoving = false;
		Vector i(formationDirX,formationDirY,0);
		i.fast_normalize();

		moveGroupTo(group,formationPost,i);
	}
	else if(bRovingAllowed && ((!(group->ai.bRoving)) || isGroupAtTarget(group)))
	{
		Vector avoidList[16];
		U32 numAvoid = 0;
		FleetGroup * gSearch = fleetGroups;
		while(gSearch && numAvoid < 16)
		{
			if(gSearch->ai.bRoving && gSearch != group)
			{
				avoidList[numAvoid] = gSearch->ai.rovingPost;
				++numAvoid;
			}
			gSearch = gSearch->next;
		}
		if(group->ai.bRoving)
			group->ai.rovingPost.init(findSpotterPoint(group->ai.rovingPost,avoidList,numAvoid),GetSystemID());
		else
			group->ai.rovingPost.init(findSpotterPoint(group->ai.lastPos,avoidList,numAvoid),GetSystemID());

		group->ai.bRoving = true;

		FleetMember * member = group->memberList;
		USR_PACKET<USRMOVE> packet;
		packet.position = group->ai.rovingPost;
		packet.userBits = 0;
		packet.ctrlBits = true;
		U32 count = 0;
		while(member)
		{
			if(dockshipID && member->partID == GetPartID())
			{
				member = member->next;
			}
			else
			{
				packet.objectID[count] = member->partID;
				++count;
				member = member->next;
			}
		}
		if(count)
		{
			packet.init(count);
			NETPACKET->Send(HOSTID, 0, &packet);
		}
	}
	else //update our attack targets.
	{
		queryIdleGunboats(group);

		if (nAvailableGunboats != 0)
		{
			findBestTargets(group->ai.targets,group->ai.lastPos);

			//update the targets
			if (group->ai.targets[0])
			{
				USR_PACKET<USRATTACK> packet1;
				USR_PACKET<USRATTACK> packet2;
				int cnt1 = 0;
				int cnt2 = 0;
				IBaseObject * targ = NULL;
				MPart part1 = group->ai.targets[1];

				for (U32 i = 0; i < nAvailableGunboats; i++)
				{
					// only assign attack commands if the ship is in the same system as the admiral
					IBaseObject * gunboat = OBJLIST->FindObject(idleGunboatArray[i]);
					targ = NULL;
					if (gunboat && gunboat->GetSystemID() == systemID)
					{
						if (part1.isValid() && nTotalGunboats > 5 && part1->caps.attackOk)
						{
							VOLPTR(ILaunchOwner) launchOwner = gunboat;
							if(launchOwner)
							{
								bool targ0Range = (launchOwner->GetWeaponRange()/GRIDSIZE > group->ai.targets[0]->GetGridPosition()-gunboat->GetGridPosition())
									&& launchOwner->TestLOS(group->ai.targets[0]->GetGridPosition());
								bool targ1Range = (launchOwner->GetWeaponRange()/GRIDSIZE > group->ai.targets[1]->GetGridPosition()-gunboat->GetGridPosition())
									&& launchOwner->TestLOS(group->ai.targets[1]->GetGridPosition());
								if(targ0Range && targ1Range)
									targ = group->ai.targets[i%2];
								else if(targ0Range)
									targ = group->ai.targets[0];
								else if(targ1Range)
									targ = group->ai.targets[1];
								else 
									findPersonalTarget(gunboat);
							}
							else
								targ = group->ai.targets[i%2];
						}
						else 
						{
							VOLPTR(ILaunchOwner) launchOwner = gunboat;
							if(launchOwner)
							{
								if(launchOwner->GetWeaponRange()/GRIDSIZE > group->ai.targets[0]->GetGridPosition()-gunboat->GetGridPosition()
									&& launchOwner->TestLOS(group->ai.targets[0]->GetGridPosition()))
									targ = group->ai.targets[0];
								else
									findPersonalTarget(gunboat);
							}
							else
								targ = group->ai.targets[0];
						}

						if (targ && targ == group->ai.targets[0])
						{
							packet1.objectID[cnt1++] = idleGunboatArray[i];
						}
						else if(targ && targ == group->ai.targets[1])
						{
							packet2.objectID[cnt2++] = idleGunboatArray[i];
						}
					}
				}

				if (cnt1)
				{
					packet1.targetID = group->ai.targets[0]->GetPartID();
					packet1.bUserGenerated = false;
					packet1.init(cnt1);
					NETPACKET->Send(HOSTID, 0, &packet1);
				}
				if (cnt2)
				{
					packet2.targetID = group->ai.targets[1]->GetPartID();
					packet2.bUserGenerated = false;
					packet2.init(cnt2);
					NETPACKET->Send(HOSTID, 0, &packet2);
				}
			}
		}
	}
}
//---------------------------------------------------------------------------
//
#define MAX_STRIKER_RANGE 6

void Flagship::updateGroupStriker(FleetGroup * group)
{
	if(group->ai.bPostMoved)//update our position
	{
		//this will force the strike team to back off for a momement if they are attacking a target.
		group->ai.bPostMoved = false;
		group->ai.bRoving = false;

		Vector i(formationDirX,formationDirY,0);
		i.fast_normalize();

		moveGroupTo(group,formationPost,i);
	}
	else //update our attack targets.
	{
		queryIdleGunboats(group);

		if (nAvailableGunboats != 0 || nTargetUpdateGunboats != 0)
		{
			findBestTargets(group->ai.targets,group->ai.lastPos);

			//update the targets
			if (group->ai.targets[0])
			{
				USR_PACKET<USRATTACK> packet1;
				USR_PACKET<USRATTACK> packet2;
				USR_PACKET<USRMOVE> movePacket;
				int cnt1 = 0;
				int cnt2 = 0;
				int cnt3 = 0;
				IBaseObject * targ = NULL;
				MPart part1 = group->ai.targets[1];

				for (U32 i = 0; i < nAvailableGunboats; i++)
				{
					// only assign attack commands if the ship is in the same system as the admiral
					IBaseObject * gunboat = OBJLIST->FindObject(idleGunboatArray[i]);
					targ = NULL;
					if (gunboat && gunboat->GetSystemID() == systemID)
					{
						bool bFoundPersonal = false;
						if (part1.isValid() && nTotalGunboats > 5 && part1->caps.attackOk)
						{
							VOLPTR(ILaunchOwner) launchOwner = gunboat;
							if(launchOwner)
							{
								bool targ0Range = (launchOwner->GetWeaponRange()/GRIDSIZE > group->ai.targets[0]->GetGridPosition()-gunboat->GetGridPosition())
									&& launchOwner->TestLOS(group->ai.targets[0]->GetGridPosition());
								bool targ1Range = (launchOwner->GetWeaponRange()/GRIDSIZE > group->ai.targets[1]->GetGridPosition()-gunboat->GetGridPosition())
									&& launchOwner->TestLOS(group->ai.targets[1]->GetGridPosition());
								if(targ0Range && targ1Range)
									targ = group->ai.targets[i%2];
								else if(targ0Range)
									targ = group->ai.targets[0];
								else if(targ1Range)
									targ = group->ai.targets[1];
								else 
									bFoundPersonal = findPersonalTarget(gunboat);
							}
							else
								targ = group->ai.targets[i%2];
						}
						else 
						{
							VOLPTR(ILaunchOwner) launchOwner = gunboat;
							if(launchOwner)
							{
								if(launchOwner->GetWeaponRange()/GRIDSIZE > group->ai.targets[0]->GetGridPosition()-gunboat->GetGridPosition()
									&& launchOwner->TestLOS(group->ai.targets[0]->GetGridPosition()))
									targ = group->ai.targets[0];
								else
									bFoundPersonal = findPersonalTarget(gunboat);
							}
							else
								targ = group->ai.targets[0];
						}

						if (targ && targ == group->ai.targets[0])
						{
							packet1.objectID[cnt1++] = idleGunboatArray[i];
						}
						else if(targ && targ == group->ai.targets[1])
						{
							packet2.objectID[cnt2++] = idleGunboatArray[i];
						}
						else if(!bFoundPersonal)
						{
							movePacket.objectID[cnt3++] = idleGunboatArray[i];
						}
					}
				}

				if (cnt1)
				{
					packet1.targetID = group->ai.targets[0]->GetPartID();
					packet1.bUserGenerated = false;
					packet1.init(cnt1);
					NETPACKET->Send(HOSTID, 0, &packet1);
				}
				if (cnt2)
				{
					packet2.targetID = group->ai.targets[1]->GetPartID();
					packet2.bUserGenerated = false;
					packet2.init(cnt2);
					NETPACKET->Send(HOSTID, 0, &packet2);
				}

				//ok now we will see if we chould move off of our point
				if(cnt3)
				{
					//ok we need to get closer to the target[0] or target[1]
					U32 targetIndex = 0;
					SINGLE rangeToTarget = group->ai.targets[0]->GetGridPosition()-group->ai.lastPos;
					if(rangeToTarget > MAX_STRIKER_RANGE && group->ai.targets[1])
					{
						targetIndex = 1;
						rangeToTarget = group->ai.targets[1]->GetGridPosition()-group->ai.lastPos;
					}

					if(rangeToTarget < MAX_STRIKER_RANGE)
					{
						//ok we need to rove to the target
						SINGLE optimalRange = findShortestOptimalRange(group);
						Vector startPos = group->ai.lastPos;
						Vector endPos = group->ai.targets[targetIndex]->GetGridPosition();
						Vector dir = startPos-endPos;
						dir.fast_normalize();
						Vector final = endPos+(dir*optimalRange*GRIDSIZE);

						group->ai.rovingPost.init(final,GetSystemID());

						group->ai.bRoving = true;

						movePacket.position = group->ai.rovingPost;
						movePacket.userBits = 0;
						movePacket.ctrlBits = false;//move as fast as posible
						movePacket.init(cnt3);
						NETPACKET->Send(HOSTID, 0, &movePacket);
					}
				}

				//update working gunboats on server only, will propagate to clients
				for (U32 i = 0; i < nTargetUpdateGunboats; i++)
				{
					// only assign attack commands if the ship is in the same system as the admiral
					IBaseObject * gunboat = OBJLIST->FindObject(targetUpdateGunboatArray[i]);
					targ = NULL;
					if (gunboat && gunboat->GetSystemID() == systemID)
					{
						if (part1.isValid() && nTotalGunboats > 3 && part1->caps.attackOk)
						{
							VOLPTR(ILaunchOwner) launchOwner = gunboat;
							if(launchOwner)
							{
								bool targ0Range = (launchOwner->GetWeaponRange()/GRIDSIZE > group->ai.targets[0]->GetGridPosition()-gunboat->GetGridPosition()
									&& launchOwner->TestLOS(group->ai.targets[0]->GetGridPosition()));
								bool targ1Range = (launchOwner->GetWeaponRange()/GRIDSIZE > group->ai.targets[1]->GetGridPosition()-gunboat->GetGridPosition())
									&& launchOwner->TestLOS(group->ai.targets[1]->GetGridPosition());
								if(targ0Range && targ1Range)
									targ = group->ai.targets[i%2];
								else if(targ0Range)
									targ = group->ai.targets[0];
								else if(targ1Range)
									targ = group->ai.targets[1];
								else 
									findPersonalUpdateTarget(gunboat);
							}
							else
								targ = group->ai.targets[i%2];
						}
						else 
						{
							VOLPTR(ILaunchOwner) launchOwner = gunboat;
							if(launchOwner)
							{
								if(launchOwner->GetWeaponRange()/GRIDSIZE > group->ai.targets[0]->GetGridPosition()-gunboat->GetGridPosition()
									&& launchOwner->TestLOS(group->ai.targets[0]->GetGridPosition()))
									targ = group->ai.targets[0];
								else
									findPersonalUpdateTarget(gunboat);
							}
							else
								targ = group->ai.targets[0];
						}
						if(targ)
						{
							VOLPTR(IFleetShip) ship = gunboat;
							if(ship)
								ship->SetFleetshipTarget(targ);
						}
					}
				}
			}
			else if(group->ai.bRoving)//I have no targets, if I am roving I should return
			{
				group->ai.bPostMoved = false;
				group->ai.bRoving = false;

				Vector i(formationDirX,formationDirY,0);
				i.fast_normalize();

				moveGroupTo(group,formationPost,i);
			}
		}
	}
}
//---------------------------------------------------------------------------
//
bool Flagship::isGroupAtTarget(FleetGroup * group)
{
	GRIDVECTOR center;
	if(group->ai.bRoving)
		center = group->ai.rovingPost;
	else
		center = group->ai.lastPos;
	FleetMember * search = group->memberList;
	while(search)
	{
		IBaseObject * obj = OBJLIST->FindObject(search->partID);
		if(obj)
		{
			if(obj->GetGridPosition()-center > group->radius)
				return false;
		}
		search = search->next;
	}
	return true;
}
//---------------------------------------------------------------------------
//
void Flagship::moveGroupTo(FleetGroup * group, NETGRIDVECTOR position,Vector i)
{
	TRANSFORM postTrans;
	postTrans.translation = position;
	Vector k(0,0,1);
	Vector j = cross_product(k,i);
	postTrans.set_i(i);
	postTrans.set_j(j);
	postTrans.set_k(k);
	Vector groupCenter(group->postX*GRIDSIZE,group->postY*GRIDSIZE,0);
	if(flipFormationPost)
		groupCenter.y *= -1.0;
	groupCenter = postTrans.rotate_translate(groupCenter);
	if(groupCenter.x < 0)
		groupCenter.x = 0;
	if(groupCenter.y < 0)
		groupCenter.y = 0;
	GRIDVECTOR grid;
	grid =groupCenter;
	FleetMember * member = group->memberList;
	USR_PACKET<USRMOVE> packet;
	packet.position.init(grid, position.systemID);
	group->ai.lastPos = packet.position;
	packet.userBits = 0;
	packet.ctrlBits = true;
	U32 count = 0;
	while(member)
	{
		if(dockshipID && member->partID == GetPartID())
		{
			member = member->next;
		}
		else
		{
			if(OBJLIST->FindObject(member->partID))
			{
				packet.objectID[count] = member->partID;
				++count;
			}
			member = member->next;
		}
	}
	if(count)
	{
		packet.init(count);
		NETPACKET->Send(HOSTID, 0, &packet);
	}
}
//---------------------------------------------------------------------------
//
FleetGroup * Flagship::findFleetGroupFor(U32 dwMissionID)
{
	FleetGroup * search = fleetGroups;
	while(search)
	{
		FleetMember * member = search->memberList;
		while(member)
		{
			if(member->partID == dwMissionID)
				return search;
			member = member->next;
		}
		search = search->next;
	}
	return NULL;
}
//---------------------------------------------------------------------------
//
void Flagship::findNextJumpGate(U32 sourceSystemID)
{
	targetFormationGate = NULL;
	targetFormationGateID = 0;
	U32 list[16];
	U32 numJumps = SECTOR->GetShortestPath(sourceSystemID, targetFormationPost.systemID, list, GetPlayerID());
	if(numJumps && numJumps != 0xFFFFFFFF)
	{
		IBaseObject * jump = SECTOR->GetJumpgateTo(list[0], list[1],getCurrentFleetPosition());
		if(jump)
		{
			jump->QueryInterface(IJumpGateID,targetFormationGate,NONSYSVOLATILEPTR);
			targetFormationGateID = jump->GetPartID();
		}
	}
}
//---------------------------------------------------------------------------
//
bool Flagship::isFormationForming(bool bPartial)
{
	U32 count = 0;
	for(U32 i = 0;i < fleetSet.numObjects; ++i)
	{
		VOLPTR(IShipMove) fleetShip = OBJLIST->FindObject(fleetSet.objectIDs[i]);
		if(fleetShip && fleetShip->IsMoving())
		{
			if(bPartial)
				++count;
			else
				return true;
		}
	}
	if(bPartial)
	{
		if(count > fleetSet.numObjects/2)
			return true;
	}
	return false;
}
//---------------------------------------------------------------------------
//
void Flagship::jumpOutGate()
{
	if(targetFormationGate)
	{
		USR_PACKET<USRJUMP> packet;
		packet.jumpgateID = targetFormationGate.Ptr()->GetPartID();
		U32 count = 0;
		for(U32 i = 0; i < fleetSet.numObjects; ++i)
		{
			IBaseObject * obj = OBJLIST->FindObject(fleetSet.objectIDs[i]);
			if(obj)
			{
				if(obj->GetSystemID() == GetSystemID())
				{
					MPart part = obj;
					if(part.isValid() && part->caps.jumpOk)
					{
						packet.objectID[count] = fleetSet.objectIDs[i];
						++count;
					}
				}
			}
		}
		if(count)
		{
			packet.init(count);
			NETPACKET->Send(HOSTID, 0, &packet);
		}
	}
}
//---------------------------------------------------------------------------
//
NETGRIDVECTOR Flagship::getCurrentFleetPosition()
{
	if(pFormation && (!(pFormation->bFreeForm)))
	{
		NETGRIDVECTOR grid;
		grid.init(GetGridPosition(),systemID);
		return grid;
	}
	else
	{
		FleetGroup * search = fleetGroups;
		while(search)
		{
			if(search->groupID == 0)
			{
				Vector pos(0,0,0);
				U32 count = 0;
				FleetMember * member = search->memberList;
				while(member)
				{
					if(member->partID != GetPartID() || (!bAttached))
					{
						IBaseObject * obj = OBJLIST->FindObject(member->partID);
						if(obj && obj->GetSystemID() == systemID)
						{
							++count;
							pos += obj->GetPosition();
						} 
					}
					member = member->next;
				}
				if(count)
				{
					pos = pos/count;
					NETGRIDVECTOR grid;
					grid.init(pos,systemID);
					return grid;
				}
				else
				{
					NETGRIDVECTOR grid;
					grid.init(GetGridPosition(),systemID);
					return grid;
				}
			}
			search = search->next;
		}
	}
	NETGRIDVECTOR grid;
	grid.init(GetGridPosition(),systemID);
	return grid;
}
//---------------------------------------------------------------------------
//
#define MAX_MOVE_DIST 5
#define MAX_TURN_ANG (PI/2.0f)

void Flagship::updateFormationPost()
{
	if(formationPost != targetFormationPost || formationDirX != targetFormationDirX || formationDirY != targetFormationDirY)
	{
		if(formationPost.systemID != targetFormationPost.systemID)//we are trying to jump
		{
			findNextJumpGate(formationPost.systemID);
			if(targetFormationGate)
			{
				Vector pos1 = formationPost;
				Vector pos2 = targetFormationGate.Ptr()->GetPosition();
				Vector dir = pos2-pos1;
				SINGLE dist = dir.fast_magnitude();
				if(dist < MAX_MOVE_DIST*GRIDSIZE)
				{
					//we are here order the jump.
					jumpOutGate();

					//update the formation post to the other side.
					IBaseObject * otherGate = SECTOR->GetJumpgateDestination(targetFormationGate.Ptr());
					Vector entryPoint = otherGate->GetPosition();
					Vector targetPoint = entryPoint+Vector(1,0,0);//just in case
					if(otherGate->GetSystemID() == targetFormationPost.systemID)
					{
						targetPoint = targetFormationPost;
					}
					else//we need to look at our path find to get the next jump gate
					{
						U32 list[16];
						U32 numJumps = SECTOR->GetShortestPath(otherGate->GetSystemID(), targetFormationPost.systemID, list, GetPlayerID());
						if(numJumps)
						{
							IBaseObject * jump = SECTOR->GetJumpgateTo(list[0], list[1],entryPoint);
							if(jump)
							{
								targetPoint = jump->GetPosition();
							}
						}
					}

					Vector dir = targetPoint-entryPoint;
					dir.fast_normalize();
					formationPost.init(entryPoint+(dir*GRIDSIZE*3),otherGate->GetSystemID());
					targetFormationDirX = formationDirX = (S8)(dir.x*127);
					targetFormationDirY = formationDirY = (S8)(dir.y*127);
				}
				else//just move closer to the gate
				{
					dir /= dist;
					formationPost.init(pos1+(dir*MAX_MOVE_DIST*GRIDSIZE),formationPost.systemID);

					Vector formationDir(formationDirX,formationDirY,0);
					formationDir.fast_normalize();
					SINGLE dot = dot_product(formationDir,dir);
					SINGLE angle = acos(dot);
					formationDirX = (S8)(dir.x*127);
					formationDirY = (S8)(dir.y*127);
					if(angle > MAX_TURN_ANG)
					{
						flipFormationPost = !flipFormationPost;
					}
				}

			}
		}
		else
		{
			if(formationPost != targetFormationPost)
			{
				Vector pos1 = formationPost;
				Vector pos2 = targetFormationPost;

				Vector dir = pos2-pos1;
				SINGLE dist = dir.fast_magnitude();
				if(dist < MAX_MOVE_DIST*GRIDSIZE)
					formationPost = targetFormationPost;
				else
				{
					dir /= dist;
					formationPost.init(pos1+(dir*MAX_MOVE_DIST*GRIDSIZE),formationPost.systemID);
				}
			}
			if(formationDirX != targetFormationDirX || formationDirY != targetFormationDirY)
			{
				Vector targetDir(targetFormationDirX,targetFormationDirY,0);
				targetDir.fast_normalize();
				Vector formationDir(formationDirX,formationDirY,0);
				formationDir.fast_normalize();
				SINGLE dot = dot_product(formationDir,targetDir);
				SINGLE angle = acos(dot);
				formationDirX = targetFormationDirX;
				formationDirY = targetFormationDirY;
				if(angle > MAX_TURN_ANG)
				{
					flipFormationPost = !flipFormationPost;
				}
			}
		}

		moveDoneHint = 0;

		FleetGroup * search = fleetGroups;
		while(search)
		{
			search->ai.bPostMoved = true;
			search = search->next;
		}
	}
}
//---------------------------------------------------------------------------
//
SINGLE Flagship::findShortestOptimalRange(FleetGroup * group)
{
	SINGLE shortest = 64*GRIDSIZE;
	FleetMember * member = group->memberList;
	while(member)
	{
		VOLPTR(ILaunchOwner) lo = OBJLIST->FindObject(member->partID);
		if(lo && lo->GetOptimalWeaponRange() < shortest)
		{
			shortest = lo->GetOptimalWeaponRange();
		}
		member = member->next;
	}
	return shortest;
}
//---------------------------------------------------------------------------
// give me a new target!
//
IBaseObject * Flagship::FleetShipTargetDestroyed (IBaseObject * ship)
{
	// the ship has to be in the same system as the admiral
	if (ship == 0 || ship->GetSystemID() != systemID)
	{
		return NULL;
	}

	if((!pFormation) || (pFormation->bFreeForm))
	{
		// ask for a new target, the fleet ship should have a couple of targets selected
		findBestTargets(targets,transform.translation);

		if(admiralTactic == AT_DEFEND || admiralTactic == AT_SEEK)
		{
			MPart part1 = targets[0];
			MPart part2 = targets[1];

			if (part1.isValid() == 0 && part2.isValid() == 0 && admiralTactic == AT_SEEK)
			{
				findBestTargets(targets,transform.translation,true);
			}

			part1 = targets[0];
			part2 = targets[1];

			if (part1.isValid() == 0 && part2.isValid() == 0)
			{
				// no new targets!  return NULL
				return NULL;
			}

			int whichTarget = rand()%2;

			if (whichTarget == 1 && part2.isValid())
			{
				// if we have more than 5 gunboats in our fleet than spread the attack out
				// but only if the second target is a threat
				queryGunboats();

				// is the secondary target a threat?
				if (part2->caps.admiralOk || part2->caps.attackOk || part2->caps.captureOk)
				{
					return part2.obj;
				}
			}

			// if we've gotten here, than we are going to attack the primary target
			return part1.obj;
		}
		else if(admiralTactic == AT_HOLD)
		{
			MPart part1 = targets[0];
			MPart part2 = targets[1];

			if (part1.isValid() == 0 && part2.isValid() == 0)
			{
				// no new targets!  return NULL
				return NULL;
			}

			bool targ0Range = false;
			bool targ1Range = false;
			VOLPTR(ILaunchOwner) launchOwner = ship;
			if(launchOwner)
			{
				if(part1.isValid())
					targ0Range = (launchOwner->GetWeaponRange()/GRIDSIZE > targets[0]->GetGridPosition()-ship->GetGridPosition());
				if(part2.isValid())
					targ1Range = (launchOwner->GetWeaponRange()/GRIDSIZE > targets[1]->GetGridPosition()-ship->GetGridPosition());
			}
			int whichTarget = rand()%2;

			if (whichTarget == 1 && part2.isValid() && targ1Range)
			{
				// if we have more than 5 gunboats in our fleet than spread the attack out
				// but only if the second target is a threat
				queryGunboats();

				// is the secondary target a threat?
				if (part2->caps.admiralOk || part2->caps.attackOk || part2->caps.captureOk)
				{
					return part2.obj;
				}
			}

			if(targ0Range)
			{
				// if we've gotten here, than we are going to attack the primary target
				return part1.obj;
			}
			return NULL;
		}
	}
	else//formation Targeting rules
	{
		// ask for a new target, the fleet ship should have a couple of targets selected
		FleetGroup * group = findFleetGroupFor(ship->GetPartID());
		if(group)
		{
			findBestTargets(group->ai.targets,group->ai.lastPos);
			if (group->ai.targets[0])
			{
				USR_PACKET<USRATTACK> packet1;
				USR_PACKET<USRATTACK> packet2;
				int cnt1 = 0;
				int cnt2 = 0;
				IBaseObject * targ = NULL;
				MPart part1 = group->ai.targets[1];

				// only assign attack commands if the ship is in the same system as the admiral
				targ = NULL;
				if (part1.isValid() && part1->caps.attackOk)
				{
					VOLPTR(ILaunchOwner) launchOwner = ship;
					if(launchOwner)
					{
						bool targ0Range = (launchOwner->GetWeaponRange()/GRIDSIZE > group->ai.targets[0]->GetGridPosition()-ship->GetGridPosition());
						bool targ1Range = (launchOwner->GetWeaponRange()/GRIDSIZE > group->ai.targets[1]->GetGridPosition()-ship->GetGridPosition());
						if(targ0Range)
							targ = group->ai.targets[0];
						else if(targ1Range)
							targ = group->ai.targets[1];
						else 
							findPersonalTarget(ship);
					}
					else
						targ = group->ai.targets[0];
				}
				else 
				{
					VOLPTR(ILaunchOwner) launchOwner = ship;
					if(launchOwner)
					{
						if(launchOwner->GetWeaponRange()/GRIDSIZE > group->ai.targets[0]->GetGridPosition()-ship->GetGridPosition())
							targ = group->ai.targets[0];
						else
							findPersonalTarget(ship);
					}
					else
						targ = group->ai.targets[0];
				}

				if (targ && targ == group->ai.targets[0])
				{
					packet1.objectID[cnt1++] = ship->GetPartID();
				}
				else if(targ && targ == group->ai.targets[1])
				{
					packet2.objectID[cnt2++] = ship->GetPartID();
				}

				if (cnt1)
				{
					packet1.targetID = group->ai.targets[0]->GetPartID();
					packet1.bUserGenerated = false;
					packet1.init(cnt1);
					NETPACKET->Send(HOSTID, 0, &packet1);
				}
				if (cnt2)
				{
					packet2.targetID = group->ai.targets[1]->GetPartID();
					packet2.bUserGenerated = false;
					packet2.init(cnt2);
					NETPACKET->Send(HOSTID, 0, &packet2);
				}
			}
		}
	}
	return NULL;
}
//---------------------------------------------------------------------------
//
U32  Flagship::GetFleetMembers (U32 objectIDs[MAX_SELECTED_UNITS])
{
	memcpy(objectIDs, fleetSet.objectIDs, sizeof(fleetSet.objectIDs));
	return fleetSet.numObjects;
}
//---------------------------------------------------------------------------
//
void Flagship::SetFleetMembers (U32 objectIDs[MAX_SELECTED_UNITS], U32 numUnits)
{
	const bool bIsMaster = THEMATRIX->IsMaster();

	CQASSERT(objectIDs[0] == dwMissionID);

	// clear out the old fleet
	clearFleet();

	// if the number of units is one, than we are disbanding the fleet, undock the admiral if you have to
	if (numUnits == 1)
	{
		if (bIsMaster)
		{
			if (bAttached)
			{
				USR_PACKET<USRUNDOCKFLAGSHIP> packet;

				packet.objectID[0] = dwMissionID;
				packet.objectID[1] = dockshipID;
				packet.position.init(GetGridPosition(), systemID);
				packet.init(2);
				NETPACKET->Send(HOSTID, 0, &packet);
			}
		}
		return;
	}
	
	// now set the new fleetID's for the objects in the fleet
	U32 i;
	IBaseObject * obj = NULL;
	MPartNC part;

	for (i = 0; i < numUnits; i++)
	{
		fleetSet.objectIDs[i] = objectIDs[i];

		obj = OBJLIST->FindObject(objectIDs[i]);
		part = obj;

		if (part.isValid())
		{
			// do you already have a fleet admiral
			if (part->fleetID && part->fleetID != dwMissionID)
			{
				OBJPTR<IAdmiral> admiral;
				if(OBJLIST->FindObject(part->fleetID, playerID, admiral, IAdmiralID))
				{
					admiral->RemoveShipFromFleet(part->dwMissionID);
				}
			}

			part->fleetID = dwMissionID;
		}
	}
	
	fleetSet.numObjects = numUnits;


	// if the flagship is not already docked, then we need to dock it on the most powerful ship in the fleet
	if (dockship == NULL && MISSION->IsComputerControlled(playerID) == false)
	{
		findBestDockship();
	}

	bReformFormation = true;
}
//---------------------------------------------------------------------------
//
void Flagship::RemoveShipFromFleet (U32 shipID)
{
	fleetSet.removeObject(shipID);
}
//---------------------------------------------------------------------------
//
bool hasMObjClass(M_OBJCLASS mObjClass, M_OBJCLASS * classList)
{
	for(U32 i = 0; i < NUM_ADMIRAL_BONUS_SHIPS; ++i)
	{
		if(mObjClass == classList[i])
			return true;
	}
	return false;
}
//---------------------------------------------------------------------------
//
bool hasArmorClass(ARMOR_TYPE sourceArmorClass,AdmiralBonuses & data)
{
	switch(sourceArmorClass)
	{
	case NO_ARMOR:
		return data.dNoArmorFavored;
	case LIGHT_ARMOR:
		return data.dLightArmorFavored;
	case MEDIUM_ARMOR:
		return data.dMediumArmorFavored;
	case HEAVY_ARMOR:
		return data.dHeavyArmorFavored;
	}
	return false;
}
//---------------------------------------------------------------------------
//
SINGLE getLocalDamageBonus(AdmiralBonuses::BonusValues & data,OBJCLASS targetObjClass,M_RACE targetRace, ARMOR_TYPE targetArmor)
{
	SINGLE bonus = 0;
	//base
	bonus += data.damage;
	//hated race
	if(targetRace == M_TERRAN)
		bonus += data.hatedRaceDamage.terran;
	else if(targetRace == M_MANTIS)
		bonus += data.hatedRaceDamage.mantis;
	else if(targetRace == M_SOLARIAN)
		bonus += data.hatedRaceDamage.celareon;
	else if(targetRace == M_VYRIUM)
		bonus += data.hatedRaceDamage.vyrium;
	//hated platform
	if(targetObjClass == OC_PLATFORM)
		bonus += data.platformDamage;
	//hated armor
	if(targetArmor == NO_ARMOR)
		bonus += data.hatedArmorDamage.noArmor;
	else if(targetArmor == LIGHT_ARMOR)
		bonus += data.hatedArmorDamage.lightArmor;
	else if(targetArmor == MEDIUM_ARMOR)
		bonus += data.hatedArmorDamage.mediumArmor;
	else if(targetArmor == HEAVY_ARMOR)
		bonus += data.hatedArmorDamage.heavyArmor;

	return bonus;
}
//---------------------------------------------------------------------------
//
SINGLE getFeatureDamageBonus(AdmiralBonuses & data,M_OBJCLASS sourceMObjClass,ARMOR_TYPE sourceArmor,OBJCLASS targetObjClass,M_RACE targetRace, ARMOR_TYPE targetArmor)
{
	SINGLE bonus = 0;

	//base
	bonus += getLocalDamageBonus(data.baseBonuses,targetObjClass,targetRace,targetArmor);
	//favored ship
	if(hasMObjClass(sourceMObjClass,data.bonusShips))
	{
		bonus += getLocalDamageBonus(data.favoredShipBonus,targetObjClass,targetRace,targetArmor);
	}

	//favoredArmor
	if(hasArmorClass(sourceArmor,data))
	{
		bonus += getLocalDamageBonus(data.favoredArmor,targetObjClass,targetRace,targetArmor);
	}

	return bonus;
}
//---------------------------------------------------------------------------
//
SINGLE Flagship::GetDamageBonus(M_OBJCLASS sourceMObjClass,ARMOR_TYPE sourceArmor,OBJCLASS targetObjClass,M_RACE targetRace, ARMOR_TYPE targetArmor)
{
	BT_FLAGSHIP_DATA * flagData = (BT_FLAGSHIP_DATA *) (ARCHLIST->GetArchetypeData(pArchetype));
	SINGLE bonus = 0;
	
	//first the base bonuses
	bonus += getFeatureDamageBonus(flagData->admiralBonueses,sourceMObjClass,sourceArmor,targetObjClass,targetRace, targetArmor);

	//formation bonuses
	if(pFormation)
	{
		bonus += getFeatureDamageBonus(pFormation->formationBonuses,sourceMObjClass,sourceArmor,targetObjClass,targetRace, targetArmor);
	}

	//kit bonuses
	for(U32 count = 0; count < MAX_KNOWN_KITS; ++count)
	{
		if(knownKits[count])
		{
			bonus += getFeatureDamageBonus(knownKits[count]->kitBonuses,sourceMObjClass,sourceArmor,targetObjClass,targetRace, targetArmor);
		}
	}

	return bonus;
}
//---------------------------------------------------------------------------
//
SINGLE getLocalDefenceBonus(AdmiralBonuses::BonusValues & data)
{
	SINGLE bonus = 0;
	//base
	bonus += data.defence;

	return bonus;
}
//---------------------------------------------------------------------------
//
SINGLE getFeatureDefenceBonus(AdmiralBonuses & data,M_OBJCLASS targetMObjClass, ARMOR_TYPE targetArmor)
{
	SINGLE bonus = 0;

	//base
	bonus += getLocalDefenceBonus(data.baseBonuses);
	//favored ship
	if(hasMObjClass(targetMObjClass,data.bonusShips))
	{
		bonus += getLocalDefenceBonus(data.favoredShipBonus);
	}

	//favoredArmor
	if(hasArmorClass(targetArmor,data))
	{
		bonus += getLocalDefenceBonus(data.favoredArmor);
	}

	return bonus;
}
//---------------------------------------------------------------------------
//
SINGLE Flagship::GetDefenceBonus(M_OBJCLASS targetMObjClass, ARMOR_TYPE targetArmor)
{
	BT_FLAGSHIP_DATA * flagData = (BT_FLAGSHIP_DATA *) (ARCHLIST->GetArchetypeData(pArchetype));

	SINGLE bonus = 0;

	//first the base bonuses
	bonus += getFeatureDefenceBonus(flagData->admiralBonueses,targetMObjClass, targetArmor);

	//formation bonuses
	if(pFormation)
	{
		bonus += getFeatureDefenceBonus(pFormation->formationBonuses,targetMObjClass, targetArmor);
	}

	//kit bonuses
	for(U32 count = 0; count < MAX_KNOWN_KITS; ++count)
	{
		if(knownKits[count])
		{
			bonus += getFeatureDefenceBonus(knownKits[count]->kitBonuses,targetMObjClass, targetArmor);
		}
	}

	return bonus;
}
//---------------------------------------------------------------------------
//
SINGLE Flagship::GetFighterDamageBonus()
{
	return 0.0;
}
//---------------------------------------------------------------------------
//
SINGLE getLocalSpeedBonus(AdmiralBonuses::BonusValues & data)
{
	SINGLE bonus = 0;
	//base
	bonus += data.speed;

	return bonus;
}
//---------------------------------------------------------------------------
//
SINGLE getFeatureSpeedBonus(AdmiralBonuses & data,M_OBJCLASS shipMObjClass, ARMOR_TYPE shipArmor)
{
	SINGLE bonus = 0;

	//base
	bonus += getLocalSpeedBonus(data.baseBonuses);
	//favored ship
	if(hasMObjClass(shipMObjClass,data.bonusShips))
	{
		bonus += getLocalSpeedBonus(data.favoredShipBonus);
	}

	//favoredArmor
	if(hasArmorClass(shipArmor,data))
	{
		bonus += getLocalSpeedBonus(data.favoredArmor);
	}

	return bonus;
}
//---------------------------------------------------------------------------
//
SINGLE Flagship::GetSpeedBonus(M_OBJCLASS shipMObjClass, ARMOR_TYPE shipArmor)
{
	BT_FLAGSHIP_DATA * flagData = (BT_FLAGSHIP_DATA *) (ARCHLIST->GetArchetypeData(pArchetype));

	SINGLE bonus = 0;

	//first the base bonuses
	bonus += getFeatureSpeedBonus(flagData->admiralBonueses,shipMObjClass, shipArmor);

	//formation bonuses
	if(pFormation)
	{
		bonus += getFeatureSpeedBonus(pFormation->formationBonuses,shipMObjClass, shipArmor);
	}

	//kit bonuses
	for(U32 count = 0; count < MAX_KNOWN_KITS; ++count)
	{
		if(knownKits[count])
		{
			bonus += getFeatureSpeedBonus(knownKits[count]->kitBonuses,shipMObjClass, shipArmor);
		}
	}

	return bonus;
}
//---------------------------------------------------------------------------
//
SINGLE getLocalSupplyBonus(AdmiralBonuses::BonusValues & data)
{
	SINGLE bonus = 0;
	//base
	bonus += data.supplyUsage;

	return bonus;
}
//---------------------------------------------------------------------------
//
SINGLE getFeatureSupplyBonus(AdmiralBonuses & data,M_OBJCLASS shipMObjClass, ARMOR_TYPE shipArmor)
{
	SINGLE bonus = 0;

	//base
	bonus += getLocalSupplyBonus(data.baseBonuses);
	//favored ship
	if(hasMObjClass(shipMObjClass,data.bonusShips))
	{
		bonus += getLocalSupplyBonus(data.favoredShipBonus);
	}

	//favoredArmor
	if(hasArmorClass(shipArmor,data))
	{
		bonus += getLocalSupplyBonus(data.favoredArmor);
	}

	return bonus;
}
//---------------------------------------------------------------------------
//
U32 Flagship::ConvertSupplyBonus(M_OBJCLASS shipMObjClass, ARMOR_TYPE shipArmor, U32 suppliesUsed)
{
	BT_FLAGSHIP_DATA * flagData = (BT_FLAGSHIP_DATA *) (ARCHLIST->GetArchetypeData(pArchetype));

	SINGLE bonus = 0;

	//first the base bonuses
	bonus += getFeatureSupplyBonus(flagData->admiralBonueses,shipMObjClass, shipArmor);

	//formation bonuses
	if(pFormation)
	{
		bonus += getFeatureSupplyBonus(pFormation->formationBonuses,shipMObjClass, shipArmor);
	}

	//kit bonuses
	for(U32 count = 0; count < MAX_KNOWN_KITS; ++count)
	{
		if(knownKits[count])
		{
			bonus += getFeatureSupplyBonus(knownKits[count]->kitBonuses,shipMObjClass, shipArmor);
		}
	}


	suppliesUsed = suppliesUsed * (1.0+bonus);
	if((!suppliesUsed) && (((rand() %1000)/1000.0) >bonus ))
		suppliesUsed = 1;

	return suppliesUsed;
}
//---------------------------------------------------------------------------
//
SINGLE getLocalSensorBonus(AdmiralBonuses::BonusValues & data)
{
	SINGLE bonus = 0;
	//base
	bonus += data.sensors;

	return bonus;
}
//---------------------------------------------------------------------------
//
SINGLE getFeatureSensorBonus(AdmiralBonuses & data,M_OBJCLASS shipMObjClass, ARMOR_TYPE shipArmor)
{
	SINGLE bonus = 0;

	//base
	bonus += getLocalSensorBonus(data.baseBonuses);
	//favored ship
	if(hasMObjClass(shipMObjClass,data.bonusShips))
	{
		bonus += getLocalSensorBonus(data.favoredShipBonus);
	}

	//favoredArmor
	if(hasArmorClass(shipArmor,data))
	{
		bonus += getLocalSensorBonus(data.favoredArmor);
	}

	return bonus;
}
//---------------------------------------------------------------------------
//
SINGLE Flagship::GetSensorBonus(M_OBJCLASS shipMObjClass, ARMOR_TYPE shipArmor)
{
	BT_FLAGSHIP_DATA * flagData = (BT_FLAGSHIP_DATA *) (ARCHLIST->GetArchetypeData(pArchetype));

	SINGLE bonus = 0;

	//first the base bonuses
	bonus += getFeatureSensorBonus(flagData->admiralBonueses,shipMObjClass, shipArmor);

	//formation bonuses
	if(pFormation)
	{
		bonus += getFeatureSensorBonus(pFormation->formationBonuses,shipMObjClass, shipArmor);
	}

	//kit bonuses
	for(U32 count = 0; count < MAX_KNOWN_KITS; ++count)
	{
		if(knownKits[count])
		{
			bonus += getFeatureSensorBonus(knownKits[count]->kitBonuses,shipMObjClass, shipArmor);
		}
	}

	return bonus;
}
//---------------------------------------------------------------------------
//
SINGLE Flagship::GetFighterTargetingBonus()
{
	return 0.0;
}
//---------------------------------------------------------------------------
//
SINGLE Flagship::GetFighterDodgeBonus()
{
	return 0.0;
}
//---------------------------------------------------------------------------
//
SINGLE getLocalRangeBonus(AdmiralBonuses::BonusValues & data)
{
	SINGLE bonus = 0;
	//base
	bonus += data.rangeModifier;

	return bonus;
}
//---------------------------------------------------------------------------
//
SINGLE getFeatureRangeBonus(AdmiralBonuses & data,M_OBJCLASS shipMObjClass, ARMOR_TYPE shipArmor)
{
	SINGLE bonus = 0;

	//base
	bonus += getLocalRangeBonus(data.baseBonuses);
	//favored ship
	if(hasMObjClass(shipMObjClass,data.bonusShips))
	{
		bonus += getLocalRangeBonus(data.favoredShipBonus);
	}

	//favoredArmor
	if(hasArmorClass(shipArmor,data))
	{
		bonus += getLocalRangeBonus(data.favoredArmor);
	}

	return bonus;
}
//---------------------------------------------------------------------------
//
SINGLE Flagship::GetRangeBonus(M_OBJCLASS shipMObjClass, ARMOR_TYPE shipArmor)
{
	BT_FLAGSHIP_DATA * flagData = (BT_FLAGSHIP_DATA *) (ARCHLIST->GetArchetypeData(pArchetype));

	SINGLE bonus = 0;

	//first the base bonuses
	bonus += getFeatureRangeBonus(flagData->admiralBonueses,shipMObjClass, shipArmor);

	//formation bonuses
	if(pFormation)
	{
		bonus += getFeatureRangeBonus(pFormation->formationBonuses,shipMObjClass, shipArmor);
	}

	//kit bonuses
	for(U32 count = 0; count < MAX_KNOWN_KITS; ++count)
	{
		if(knownKits[count])
		{
			bonus += getFeatureRangeBonus(knownKits[count]->kitBonuses,shipMObjClass, shipArmor);
		}
	}

	return bonus;
}
//---------------------------------------------------------------------------
//
bool Flagship::IsInFleetRepairMode()
{
	if(pFormation)
	{
		return pFormation->formationSpecials.bRepairWholeFleet;
	}
	return false;
}
//---------------------------------------------------------------------------
//
U32 Flagship::GetToolbarImage()
{
	BT_FLAGSHIP_DATA * flagData = (BT_FLAGSHIP_DATA *) (ARCHLIST->GetArchetypeData(pArchetype));
	return flagData->toolbarInfo.baseImage;
}
//---------------------------------------------------------------------------
//
HBTNTXT::BUTTON_TEXT Flagship::GetToolbarText()
{
	BT_FLAGSHIP_DATA * flagData = (BT_FLAGSHIP_DATA *) (ARCHLIST->GetArchetypeData(pArchetype));
	return flagData->toolbarInfo.buttonText;
}
//---------------------------------------------------------------------------
//
HBTNTXT::HOTBUTTONINFO Flagship::GetToolbarStatusText()
{
	BT_FLAGSHIP_DATA * flagData = (BT_FLAGSHIP_DATA *) (ARCHLIST->GetArchetypeData(pArchetype));
	return flagData->toolbarInfo.buttonStatus;
}
//---------------------------------------------------------------------------
//
HBTNTXT::MULTIBUTTONINFO Flagship::GetToolbarHintbox ()
{
	BT_FLAGSHIP_DATA * flagData = (BT_FLAGSHIP_DATA *) (ARCHLIST->GetArchetypeData(pArchetype));
	return flagData->toolbarInfo.buttonHintbox;
}
//---------------------------------------------------------------------------
//
void Flagship::SetAdmiralTactic (ADMIRAL_TACTIC tactic)
{
	admiralTactic = tactic;
}
//---------------------------------------------------------------------------
//
ADMIRAL_TACTIC Flagship::GetAdmiralTactic ()
{
	return admiralTactic;
}
//---------------------------------------------------------------------------
//
void Flagship::SetFormation (U32 formationArchID)
{
	bReformFormation = true;
	formationID = formationArchID;
	if(formationID)
		pFormation = (BT_FORMATION * )(ARCHLIST->GetArchetypeData(formationID));
	else
		pFormation = NULL;
}
//---------------------------------------------------------------------------
//
U32 Flagship::GetFormation ()
{
	return formationID;
}
//---------------------------------------------------------------------------
//
U32 Flagship::GetKnownFormation(U32 index)
{
	if(index < MAX_FORMATIONS)
	{
		return knownFormations[index];
	}
	return 0;
}
//---------------------------------------------------------------------------
//
void Flagship::LearnCommandKit(const char * kitName)
{
	U32 kitID = ARCHLIST->GetArchetypeDataID(kitName);
	if(kitID)
	{
		BT_COMMAND_KIT * kit = (BT_COMMAND_KIT*)(ARCHLIST->GetArchetypeData(kitID));
		U32 count;
		for(count = 0; count < MAX_KNOWN_KITS; ++count)
		{
			if(!(commandKitsArchID[count]))
			{
				commandKitsArchID[count] = kitID;
				knownKits[count] = kit;
				break;
			}
		}
		for(count = 0; count < MAX_COMMAND_FORMATIONS; ++count)
		{
			if(kit->formations[count][0])
			{
				U32 formID = ARCHLIST->GetArchetypeDataID(kit->formations[count]);
				if(formID)
				{
					for(U32 formsIndex = 0; formsIndex <MAX_FORMATIONS; ++formsIndex)
					{
						if(knownFormations[formsIndex] == formID)
						{
							break;//I already know this formation
						}
						else if(knownFormations[formsIndex] == 0)
						{
							knownFormations[formsIndex] = formID;
							break;
						}
					}
				}
			}
		}
	}
}
//---------------------------------------------------------------------------
//
void Flagship::RenderFleetInfo()
{
	renderFormationInfo();
}
//---------------------------------------------------------------------------
//
void Flagship::HandleMoveCommand(const NETGRIDVECTOR &vec)
{
	if(!bHaveFormationPost)
		computeDefaultFormationPost();

	idleTimer = 0;//force and ai update
	formationTarget = NULL;
	formationTargetID = 0;

	targetFormationDirX = formationDirX;
	targetFormationDirY = formationDirY;

	//move spoke
	CQASSERT(bHaveFormationPost);
	if(vec.systemID == systemID)
	{
		Vector end1;
		end1 = vec;
		Vector end2;
		end2 = getCurrentFleetPosition();
		Vector dir = end1-end2;
		if(dir.magnitude_squared() > 0.000001)
		{
			dir.fast_normalize();
			targetFormationDirX = (S8)(dir.x*127);
			targetFormationDirY = (S8)(dir.y*127);
		}
	}
	else if(vec.systemID == formationPost.systemID)
	{
		Vector end1;
		end1 = vec;
		Vector end2;
		end2 = formationPost;
		Vector dir = end1-end2;
		if(dir.magnitude_squared() > 0.000001)
		{
			dir.fast_normalize();
			targetFormationDirX = (S8)(dir.x*127);
			targetFormationDirY = (S8)(dir.y*127);
		}
	}
	else
	{
		RECT rect;
		SECTOR->GetSystemRect(vec.systemID,&rect);
		Vector centerSys((rect.right-rect.left)/2,(rect.top-rect.bottom)/2,0);
		Vector dir = centerSys-vec;
		if(dir.magnitude_squared() > 0.000001)
		{
			dir.fast_normalize();
			targetFormationDirX = (S8)(dir.x*127);
			targetFormationDirY = (S8)(dir.y*127);
		}
	}
	targetFormationPost = vec;

	bForceUpdateFormationPost = true;
}
//---------------------------------------------------------------------------
//
void Flagship::HandleAttackCommand(U32 targetID, U32 destSystemID)
{
	idleTimer = 0;//force and ai update
	formationTargetID = targetID;
	OBJLIST->FindObject(targetID,playerID,formationTarget);
	if(!formationTarget)
		formationTargetID = 0;
}
//---------------------------------------------------------------------------
//
bool Flagship::IsInLockedFormation()
{
	if(pFormation && (!(pFormation->bFreeForm)))
	{
		return true;
	}
	return false;
}
//---------------------------------------------------------------------------
//
Vector Flagship::GetFormationDir()
{
	Vector ret(formationDirX,formationDirY,0);
	ret.fast_normalize();
	return ret;
}
//---------------------------------------------------------------------------
//
void Flagship::MoveDoneHint(IBaseObject * ship)
{
	if(ship->GetSystemID() == formationPost.systemID)
	{
		moveDoneHint++;
		if(moveDoneHint > (fleetSet.numObjects/2))
		{
			idleTimer = 0;//force and ai update
			bForceUpdateFormationPost = true;
		}
	}
}
//---------------------------------------------------------------------------
//
struct BT_COMMAND_KIT * Flagship::GetAvailibleCommandKit(U32 index)
{
	BT_FLAGSHIP_DATA * flagData = (BT_FLAGSHIP_DATA *) (ARCHLIST->GetArchetypeData(pArchetype));
	if(flagData->commandKits[index][0])
	{
		return (BT_COMMAND_KIT *)(ARCHLIST->GetArchetypeData(flagData->commandKits[index]));
	}
	return NULL;
}
//---------------------------------------------------------------------------
//
U32 Flagship::GetAvailibleCommandKitID(U32 index)
{
	BT_FLAGSHIP_DATA * flagData = (BT_FLAGSHIP_DATA *) (ARCHLIST->GetArchetypeData(pArchetype));
	if(flagData->commandKits[index][0])
	{
		return ARCHLIST->GetArchetypeDataID(flagData->commandKits[index]);
	}
	return 0;
}
//---------------------------------------------------------------------------
//
bool Flagship::IsKnownKit(struct BT_COMMAND_KIT * comKit)
{
	for(U32 count = 0; count < MAX_KNOWN_KITS; ++count)
	{
		if(knownKits[count] == comKit)
			return true;
	}
	return false;
}
//---------------------------------------------------------------------------
//
struct BT_COMMAND_KIT * Flagship::GetKnownCommandKit(U32 index)
{
	return knownKits[index];
}
//---------------------------------------------------------------------------
//
bool Flagship::CanResearchKits()
{
	U32 numberTaken = 0;
	for(U32 count = 0; count < MAX_KNOWN_KITS; ++count)
	{
		if(knownKits[count])
		{
			numberTaken++;
		}
	}
	numberTaken += getQueueSize();
	if(numberTaken < MAX_KNOWN_KITS && (!buildQueueFull()) && bReady)
	{
		return true;
	}
	return false;
}
//---------------------------------------------------------------------------
//
BOOL32 Flagship::Save (struct IFileSystem * inFile)
{
	SpaceShip<FLAGSHIP_SAVELOAD, FLAGSHIP_INIT>::Save(inFile);
	//
	// save the group
	//
	DAFILEDESC fdesc = "FLEET";
	COMPTR<IFileSystem> file;
	DWORD dwWritten;

	fdesc.lpImplementation = "DOS";
	fdesc.dwDesiredAccess = GENERIC_READ|GENERIC_WRITE;
	fdesc.dwShareMode = 0;  // no sharing
	fdesc.dwCreationDistribution = CREATE_ALWAYS;

	if (inFile->CreateInstance(&fdesc, file) == GR_OK)
	{
		if (fleetSet.numObjects)
			file->WriteFile(0, fleetSet.objectIDs, fleetSet.numObjects * sizeof(U32), &dwWritten, 0);
	}

	return 1;
}
//---------------------------------------------------------------------------
//
BOOL32 Flagship::Load (struct IFileSystem * inFile)
{
	SpaceShip<FLAGSHIP_SAVELOAD, FLAGSHIP_INIT>::Load(inFile);
	//
	// load the group, if it's not loaded already
	//
	DAFILEDESC fdesc = "FLEET";
	COMPTR<IFileSystem> file;
	DWORD dwRead;

	if (inFile->CreateInstance(&fdesc, file) == GR_OK)
	{
		fleetSet.numObjects = file->GetFileSize() / sizeof(U32);
		file->ReadFile(0, fleetSet.objectIDs, fleetSet.numObjects * sizeof(U32), &dwRead, 0);
	}

	netAdmiralTactic = admiralTactic;
	netFormationID = formationID;
	if(formationID)
		pFormation = (BT_FORMATION * )(ARCHLIST->GetArchetypeData(formationID));
	else
		pFormation = NULL;

	for(U32 count = 0; count < MAX_KNOWN_KITS; ++count)
	{
		if(commandKitsArchID[count])
		{
			knownKits[count] = (BT_COMMAND_KIT *)(ARCHLIST->GetArchetypeData(commandKitsArchID[count]));
		}
		else
		{
			knownKits[count] = NULL;
		}
	}
	return 1;
}
//------------------------------------------------------------------------------------------
// called in response to OpAgent::SendOperationData()
//
void Flagship::receiveOperationData (U32 agentID, void *buffer, U32 bufferSize)
{
	if (agentID == dockAgentID)
	{
		// buffer==NULL means operation completed successfully!
		if (buffer==0)
		{
			if (dockship)
				attachToDockShip(dockship.Ptr());
		}
		else
		{
			if (dockship)
				dockship->WaitForAdmiral(false);
		}
		THEMATRIX->OperationCompleted(dockAgentID, dockshipID);
		if(buffer)
			dockshipID = 0;
		THEMATRIX->OperationCompleted2(dockAgentID, dwMissionID);
		if (buffer==0 && playerID == MGlobals::GetThisPlayer())
			FLAGSHIPCOMM(admiralondeck,SUB_ADMIRAL_ON_DECK);
	}
}
//------------------------------------------------------------------------------------------
// user has requested a different action
//
void Flagship::onOperationCancel (U32 agentID)
{
	if (agentID == dockAgentID)
	{
		if (dockship)
			dockship->WaitForAdmiral(false);
		THEMATRIX->OperationCompleted(dockAgentID, dockshipID);
		dockAgentID = 0;
		dockship = NULL;
		dockshipID = 0;
	}
//	SpaceShip<FLAGSHIP_SAVELOAD, FLAGSHIP_INIT>::OnOperationCancel(agentID);
}
//------------------------------------------------------------------------------------------
//	
void Flagship::checkTargetPointers (void)
{
	// have our tagets moved out of range?
	bool bNullTargets = false;

	U32 range = ATTACK_RANGE/GRIDSIZE;

	if (targets[0] != 0)
	{
		// are the targets in the same system as the flagship?
		if (targets[0]->GetSystemID() != GetSystemID())
		{
			bNullTargets = true;
		}
		else
		{
			SINGLE mag = targets[0]->GetGridPosition() - GetGridPosition();
			if (mag > range)
			{
				bNullTargets = true;
			}
		}
	}

	if (bNullTargets)
	{
		targets[0] = NULL;
		targets[1] = NULL;
	}
}
//------------------------------------------------------------------------------------------
//	
void Flagship::moveIdleShips (bool bUpdate)
{
	// make sure we;re not in hyperspace!
	if (systemID && systemID <= MAX_SYSTEMS)
	{
		if (bUpdate)
		{
			queryIdleShips();
		}

		GRIDVECTOR grid = GetGridPosition();
		IBaseObject * obj;
		U32 i;

		USR_PACKET<USRMOVE> packet;
		int cnt = 0;

		for (i = 0; i < nIdleShips; i++)
		{
			obj = OBJLIST->FindObject(idleShipArray[i]);

			if (obj != NULL)
			{
				// do any of the available ships need to be told to move closer?
				if (obj->GetSystemID() != systemID)
				{
					// this fleetship is not in the same system as the admiral
					// send a move packet
					packet.objectID[cnt++] = idleShipArray[i];
				}

			}
		}

		if (cnt)
		{
			packet.position.init(grid, systemID);
			packet.userBits = 0;
			packet.init(cnt);
			NETPACKET->Send(HOSTID, 0, &packet);
		}
		if(admiralTactic == AT_SEEK)
		{
			GRIDVECTOR pos = findSeekPoint(GetGridPosition(),NULL,0);
			USR_PACKET<USRMOVE> seekPacket;
			cnt = 0;

			for (i = 0; i < nIdleShips; i++)
			{
				obj = OBJLIST->FindObject(idleShipArray[i]);

				if (obj != NULL)
				{
					if (obj->GetSystemID() == systemID)
					{
						seekPacket.objectID[cnt++] = idleShipArray[i];
					}
				}
			}

			if (cnt)
			{
				seekPacket.position.init(pos, systemID);
				seekPacket.userBits = 0;
				seekPacket.init(cnt);
				NETPACKET->Send(HOSTID, 0, &seekPacket);
			}

		}
	}
}
//------------------------------------------------------------------------------------------
//
void Flagship::querySupplyShips (void)
{
	U32 i;
	nSupplyShips = 0;
	MPart part;

	for (i = 0; i < fleetSet.numObjects; i++)
	{
		part = OBJLIST->FindObject(fleetSet.objectIDs[i]);
		
		if ((part.isValid() && part->caps.supplyOk == true) && (THEMATRIX->HasPendingOp(fleetSet.objectIDs[i]) == false))
		{
			supplyShipArray[nSupplyShips++] = fleetSet.objectIDs[i];
		}
	}
}
//------------------------------------------------------------------------------------------
//	
void Flagship::queryIdleShips (void)
{
	U32 i;
	nIdleShips = 0;
	MPart part;

	for (i = 0; i < fleetSet.numObjects; i++)
	{
		part = OBJLIST->FindObject(fleetSet.objectIDs[i]);
		
		if (part.isValid() && part->caps.moveOk && (THEMATRIX->HasPendingOp(fleetSet.objectIDs[i]) == false))
		{
			idleShipArray[nIdleShips++] = fleetSet.objectIDs[i];
		}
	}
}
//------------------------------------------------------------------------------------------
//	
void Flagship::queryIdleGunboats (void)
{
	U32 i;
	nAvailableGunboats = 0;
	nTotalGunboats = 0;
	MPart part;
	OBJPTR<IFleetShip> fleetShip;

	for (i = 0; i < fleetSet.numObjects; i++)
	{
		part = OBJLIST->FindObject(fleetSet.objectIDs[i]);
		if (part.isValid())
		{
			// is the object a gunboat, and is that gunboat available for admiral instructions
			part.obj->QueryInterface(IFleetShipID, fleetShip);

			if (fleetShip)
			{
				nTotalGunboats++;
				if (fleetShip->IsAvailableForAdmiral() != 0)
				{
					idleGunboatArray[nAvailableGunboats++] = fleetSet.objectIDs[i];
				}
			}
		}
	}
}
//------------------------------------------------------------------------------------------
//	
void Flagship::queryIdleGunboats (FleetGroup * group)
{
	nAvailableGunboats = 0;
	nTotalGunboats = 0;
	nTargetUpdateGunboats = 0;
	MPart part;
	OBJPTR<IFleetShip> fleetShip;

	FleetMember * search = group->memberList;

	while(search)
	{
		part = OBJLIST->FindObject(search->partID);
		if (part.isValid())
		{
			// is the object a gunboat, and is that gunboat available for admiral instructions
			part.obj->QueryInterface(IFleetShipID, fleetShip);

			if (fleetShip)
			{
				nTotalGunboats++;
				if (fleetShip->IsAvailableForAdmiral() != 0)
				{
					idleGunboatArray[nAvailableGunboats++] = search->partID;
				}
				else
				{
					IBaseObject * target = fleetShip->GetFleetshipTarget();
					if((target == 0) || (group->ai.targets[0] != target && group->ai.targets[1] != target))
						targetUpdateGunboatArray[nTargetUpdateGunboats++] = search->partID;
				}
			}
		}
		search = search->next;
	}
}
//------------------------------------------------------------------------------------------
//	
void Flagship::doIdleBehavior (void)
{
	if(pFormation == NULL || pFormation->bFreeForm)
	{
		smartUnitStances();
		smartFleetTargetSelection();
		smartSupplyShips();

		if (targetID)
		{
			if (target==0 || target->GetSystemID()!=systemID || target->IsVisibleToPlayer(playerID)==0)
			{
				targetID = 0;
				target = 0;

				if (mode == ATTACKING)
					mode = NOMODE;
			}
		}

		if (THEMATRIX->HasActiveJump(fleetSet) == false)
		{
			// if defending, and our defending ship has left, then go after it
			if (mode != ATTACKING && mode != MOVING && (mode != DEFENDING || defend==0 || defend->GetSystemID()==systemID))
			{
				//findAutoTarget();
				//smartFleetTargetSelection();
			}
		}
	}
	else //we are in a formation
	{
		if(bReformFormation && (dockshipID == 0 || bAttached))
		{
			if(!bHaveFormationPost)
				computeDefaultFormationPost();
			reformFormation();
			bReformFormation = false;
		}

		if(THEMATRIX->IsMaster() && ((systemID & HYPER_SYSTEM_MASK)==0))
		{
			updateFormationAI();
			updateGroupAI();
		}
	}
}
//------------------------------------------------------------------------------------------
//	
void Flagship::smartSupplyShips (void)
{
	// what we need to know....

	// 1) what ships need supplies the most
	// 2) what ships already have a supply ship assigned to them

	querySupplyShips();

	if (nSupplyShips)
	{
		U32 i;

		// assign a supply ship to a fleet member
		// find the top num = nSupplyShips in the fleet that would benefit from supply ships

		// for now, have supply ships escort the admiral
		USR_PACKET<USRESCORT> packet;
		for (i = 0; i < nSupplyShips; i++)
		{
			packet.objectID[i] = supplyShipArray[i]; 
		}
		packet.targetID = dwMissionID;
		packet.init(i);
		NETPACKET->Send(HOSTID, 0, &packet);
	}
}
//------------------------------------------------------------------------------------------
//	find the best target's for each ship in the fleet
void Flagship::findBestTargets (OBJPTR<IBaseObject> * targetList, Vector searchPos, bool bSeekRange)
{
	if (targetList[0] == NULL || targetList[1] == NULL)
	{
		const U32 allyMask = MGlobals::GetAllyMask(playerID);

		// get all the objects that are potential targets
		ObjMapIterator it(systemID, searchPos, (bSeekRange) ? SEEK_RANGE : ATTACK_RANGE, playerID);
		MPart part;

		IBaseObject * bestTargets[NUM_TARGETS];
		U32 betterHullMax = 0;
		U32 bestHullMax = 0;
		U32 testHullMax;
		U32 bestHull = 10000000;
		U32 betterHull = 10000000;
		U32 testHull;
		SINGLE bestDist = 10000000.0f;
		SINGLE betterDist = 10000000.0f;
		SINGLE testDist;
		S32 bestThreat = -1;
		S32 betterThreat = -1;
		S32 testThreat;
		bool bBetter;
		bool bBest;
		memset(bestTargets, 0, sizeof(bestTargets));
		GRIDVECTOR searchGridPos;
		searchGridPos = searchPos;

		while (it)
		{
			if ((it->flags & OM_UNTOUCHABLE) == 0)
			{
				const U32 hisPlayerID = it.GetApparentPlayerID(allyMask);//it->dwMissionID & PLAYERID_MASK;
					
				// if we are enemies
				if (hisPlayerID && (((1 << (hisPlayerID-1)) & allyMask) == 0))
				{
					// as long as the enemy is visible, let's go for it!
					if ((it->flags & OM_TARGETABLE) && it->obj->IsVisibleToPlayer(playerID))
					{
						part = it->obj;
						if (part.isValid() && part->hullPoints>0)
						{
							testHullMax = part->hullPointsMax;
							testHull = part->hullPoints;
							testDist = part.obj->GetGridPosition() - searchGridPos;
							testThreat = 0;//part->caps.admiralOk;
							bBetter = false;
							bBest = false;

							// set up the test threat
							if (part->caps.admiralOk)
							{
								testThreat += 100;
							}
							if (part->caps.captureOk)
							{
								testThreat += 60;
							}
							if (part->caps.attackOk)
							{
								testThreat += 50;
							}
							if (MGlobals::IsFabricator(part->mObjClass))
							{
								testThreat += 40;
							}
							if(it->dwMissionID == formationTargetID)
							{
								testThreat += 1000;
							}
							
							if (testThreat > bestThreat)
							{
								// found a better target based on its threat potential
								bBest = true;
							}
							else if (testThreat == bestThreat)
							{
								if (testHullMax > bestHullMax)
								{
									// found a better target on hull max alone
									bBest = true;
								}
								else if (testHullMax == bestHullMax)
								{
									// is this target weaker?
									if (testHull < bestHull)
									{
										// found a better target with the same hull max, but a weaker hull
										bBest = true;
									}
									else if (testHull == bestHull)
									{
										// is the target closer?
										if (testDist < bestDist)
										{
											// found a better target with the same hull max and the same current hull but it's closer
											bBest = true;
										}
									}
								}
							}

							if(!bBest)
							{
								if (testThreat > betterThreat)
								{
									// found a better target based on its threat potential
									bBetter = true;
								}
								else if (testThreat == betterThreat)
								{
									if (testHullMax > betterHullMax)
									{
										// found a better target on hull max alone
										bBetter = true;
									}
									else if (testHullMax == betterHullMax)
									{
										// is this target weaker?
										if (testHull < betterHull)
										{
											// found a better target with the same hull max, but a weaker hull
											bBetter = true;
										}
										else if (testHull == betterHull)
										{
											// is the target closer?
											if (testDist < betterDist)
											{
												// found a better target with the same hull max and the same current hull but it's closer
												bBetter = true;
											}
										}
									}
								}

							}

							if (bBetter)
							{
								// found the second best target
							
								bestTargets[1] = part.obj;
								betterHullMax = testHullMax;
								betterHull = testHull;
								betterDist = testDist;
								betterThreat = testThreat;
							}
							else if(bBest)
							{
								// found the best target
								IBaseObject * tmp = bestTargets[0];
								
								bestTargets[0] = part.obj;
								bestTargets[1] = tmp;
								betterHullMax = bestHullMax;
								betterHull = bestHull;
								betterDist = bestDist;
								betterThreat = bestThreat;
								bestHullMax = testHullMax;
								bestHull = testHull;
								bestDist = testDist;
								bestThreat = testThreat;
							}
						}
					}
				}
			}
			++it;
		}

		for (U32 i = 0; i < NUM_TARGETS; i++)
		{
			if (bestTargets[i])
				bestTargets[i]->QueryInterface(IBaseObjectID, targetList[i], playerID);
			else
				targetList[i] = 0;
		}
	}
}
//------------------------------------------------------------------------------------------
//	
bool Flagship::findPersonalTarget (IBaseObject * gunboat)
{
	VOLPTR(ILaunchOwner) lauchOwner = gunboat;
	if(lauchOwner)
	{
		const U32 allyMask = MGlobals::GetAllyMask(playerID);

		// get all the objects that are potential targets
		ObjMapIterator it(systemID, gunboat->GetGridPosition(), lauchOwner->GetWeaponRange(), playerID);
		MPart part;

		IBaseObject * bestTarget = NULL;
		U32 bestHullMax = 0;
		U32 testHullMax;
		U32 bestHull = 10000000;
		U32 testHull;
		SINGLE bestDist = 10000000.0f;
		SINGLE testDist;
		S32 bestThreat = -1;
		S32 testThreat;
		bool bBetter;

		while (it)
		{
			if ((it->flags & OM_UNTOUCHABLE) == 0)
			{
				const U32 hisPlayerID = it.GetApparentPlayerID(allyMask);//it->dwMissionID & PLAYERID_MASK;
					
				// if we are enemies
				if (hisPlayerID && (((1 << (hisPlayerID-1)) & allyMask) == 0))
				{
					// as long as the enemy is visible, let's go for it!
					if ((it->flags & OM_TARGETABLE) && it->obj->IsVisibleToPlayer(playerID))
					{
						part = it->obj;
						if (part.isValid() && part->hullPoints>0)
						{
							if(lauchOwner->GetWeaponRange() >= (it->obj->GetGridPosition()-gunboat->GetGridPosition())*GRIDSIZE)
							{
								testHullMax = part->hullPointsMax;
								testHull = part->hullPoints;
								testDist = part.obj->GetGridPosition() - gunboat->GetGridPosition();
								testThreat = 0;//part->caps.admiralOk;
								bBetter = false;

								// set up the test threat
								if (part->caps.admiralOk)
								{
									testThreat += 100;
								}
								if (part->caps.captureOk)
								{
									testThreat += 60;
								}
								if (part->caps.attackOk)
								{
									testThreat += 50;
								}
								if (MGlobals::IsFabricator(part->mObjClass))
								{
									testThreat += 40;
								}
								
								if (testThreat > bestThreat)
								{
									// found a better target based on its threat potential
									bBetter = true;
								}
								else if (testThreat == bestThreat)
								{
									if (testHullMax > bestHullMax)
									{
										// found a better target on hull max alone
										bBetter = true;
									}
									else if (testHullMax == bestHullMax)
									{
										// is this target weaker?
										if (testHull < bestHull)
										{
											// found a better target with the same hull max, but a weaker hull
											bBetter = true;
										}
										else if (testHull == bestHull)
										{
											// is the target closer?
											if (testDist < bestDist)
											{
												// found a better target with the same hull max and the same current hull but it's closer
												bBetter = true;
											}
										}
									}
								}

								if (bBetter)
								{
									bestTarget = part.obj;
									bestHullMax = testHullMax;
									bestHull = testHull;
									bestDist = testDist;
									bestThreat = testThreat;
								}
							}
						}
					}
				}
			}
			++it;
		}

		if(bestTarget)
		{
			USR_PACKET<USRATTACK> packet1;
			packet1.targetID = bestTarget->GetPartID();
			packet1.bUserGenerated = false;
			packet1.objectID[0] = gunboat->GetPartID();
			packet1.init(1);
			NETPACKET->Send(HOSTID, 0, &packet1);
			return true;
		}
	}
	return false;
}
//------------------------------------------------------------------------------------------
//	
bool Flagship::findPersonalUpdateTarget (IBaseObject * gunboat)
{
	VOLPTR(ILaunchOwner) lauchOwner = gunboat;
	if(lauchOwner)
	{
		const U32 allyMask = MGlobals::GetAllyMask(playerID);

		// get all the objects that are potential targets
		ObjMapIterator it(systemID, gunboat->GetGridPosition(), lauchOwner->GetWeaponRange(), playerID);
		MPart part;

		IBaseObject * bestTarget = NULL;
		U32 bestHullMax = 0;
		U32 testHullMax;
		U32 bestHull = 10000000;
		U32 testHull;
		SINGLE bestDist = 10000000.0f;
		SINGLE testDist;
		S32 bestThreat = -1;
		S32 testThreat;
		bool bBetter;

		while (it)
		{
			if ((it->flags & OM_UNTOUCHABLE) == 0)
			{
				const U32 hisPlayerID = it.GetApparentPlayerID(allyMask);//it->dwMissionID & PLAYERID_MASK;
					
				// if we are enemies
				if (hisPlayerID && (((1 << (hisPlayerID-1)) & allyMask) == 0))
				{
					// as long as the enemy is visible, let's go for it!
					if ((it->flags & OM_TARGETABLE) && it->obj->IsVisibleToPlayer(playerID))
					{
						part = it->obj;
						if (part.isValid() && part->hullPoints>0)
						{
							if(lauchOwner->GetWeaponRange() >= (it->obj->GetGridPosition()-gunboat->GetGridPosition())*GRIDSIZE)
							{
								testHullMax = part->hullPointsMax;
								testHull = part->hullPoints;
								testDist = part.obj->GetGridPosition() - gunboat->GetGridPosition();
								testThreat = 0;//part->caps.admiralOk;
								bBetter = false;

								// set up the test threat
								if (part->caps.admiralOk)
								{
									testThreat += 100;
								}
								if (part->caps.captureOk)
								{
									testThreat += 60;
								}
								if (part->caps.attackOk)
								{
									testThreat += 50;
								}
								if (MGlobals::IsFabricator(part->mObjClass))
								{
									testThreat += 40;
								}
								
								if (testThreat > bestThreat)
								{
									// found a better target based on its threat potential
									bBetter = true;
								}
								else if (testThreat == bestThreat)
								{
									if (testHullMax > bestHullMax)
									{
										// found a better target on hull max alone
										bBetter = true;
									}
									else if (testHullMax == bestHullMax)
									{
										// is this target weaker?
										if (testHull < bestHull)
										{
											// found a better target with the same hull max, but a weaker hull
											bBetter = true;
										}
										else if (testHull == bestHull)
										{
											// is the target closer?
											if (testDist < bestDist)
											{
												// found a better target with the same hull max and the same current hull but it's closer
												bBetter = true;
											}
										}
									}
								}

								if (bBetter)
								{
									bestTarget = part.obj;
									bestHullMax = testHullMax;
									bestHull = testHull;
									bestDist = testDist;
									bestThreat = testThreat;
								}
							}
						}
					}
				}
			}
			++it;
		}

		if(bestTarget)
		{
			VOLPTR(IFleetShip) ship = gunboat;
			if(ship)
				ship->SetFleetshipTarget(bestTarget);
			return true;
		}
	}
	return false;
}
//------------------------------------------------------------------------------------------
//	
GRIDVECTOR Flagship::findSeekPoint (GRIDVECTOR searchCenter, Vector * avoidList, U32 numAvoidPoints)
{
	GRIDVECTOR retVal = searchCenter;//worst case target the amiral;
	SINGLE percent = FOGOFWAR->GetPercentageFogCleared(playerID,systemID);
	if(percent != 1.0) //find a spot of hard fog
	{
		Vector pos = FOGOFWAR->FindHardFog(playerID,systemID,searchCenter,avoidList,numAvoidPoints);
		retVal = pos;
		return retVal;
	}
	
	//create a list of planets and jumpgates
#define MAX_SEEK_OBJECTS 20
	IBaseObject * seekList[MAX_SEEK_OBJECTS];
	U32 numObjects = 0;
	ObjMapIterator it(systemID, transform.translation, 64*GRIDSIZE, playerID);
	while(it && numObjects < MAX_SEEK_OBJECTS)
	{
		if((!(it->flags & OM_TARGETABLE)) && (it->flags & OM_SYSMAP_FIRSTPASS)) //planets and jumpgates are not targetable and are on the first pass
		{
			if(it->obj->objClass == OC_PLANETOID || it->obj->objClass == OC_JUMPGATE)
			{
				seekList[numObjects] = it->obj;
				++numObjects;
			}
		}
		++it;
	}
	//pick a random object
	if(numObjects)
	{
		return seekList[rand()%numObjects]->GetGridPosition();
	}
	return retVal;
}
//------------------------------------------------------------------------------------------
//	
#define SPOT_SCORE_DIST (4*GRIDSIZE)

GRIDVECTOR Flagship::findSpotterPoint (GRIDVECTOR searchCenter, Vector * avoidList, U32 numAvoidPoints)
{
	GRIDVECTOR retVal = searchCenter;//worst case target the amiral;
	SINGLE percent = FOGOFWAR->GetPercentageFogCleared(playerID,systemID);
	if(percent != 1.0) //find a spot of hard fog
	{
		Vector pos = FOGOFWAR->FindHardFogFiltered(playerID,systemID,searchCenter,avoidList,numAvoidPoints,formationPost,pFormation->spotterRange*GRIDSIZE);
		retVal = pos;
		if(retVal-formationPost <= pFormation->spotterRange)
            return retVal;
		else
			retVal = searchCenter;
	}
	//ok there is no hard fog to go to
	// goto a point allong the boarder preferring a point near our location toward the front
	Vector testPoint = searchCenter;
	Vector center = formationPost;
	Vector dir;
	Vector formationDir = Vector(formationDirX,formationDirY,0);
	formationDir.fast_normalize();

	if(searchCenter == formationPost)
	{
		dir = formationDir;
	}
	else
	{
		dir = testPoint-center;
		dir.fast_normalize();
	}
	//now rotate the dir
	TRANSFORM trans;
	trans.rotate_about_k(25*MUL_DEG_TO_RAD);
	Vector dir1 = trans.rotate(dir);
	trans.set_identity();
	trans.rotate_about_k(-25*MUL_DEG_TO_RAD);
	Vector dir2 = trans.rotate(dir);
	Vector boarderPoint1 = center + (dir1*(pFormation->spotterRange*GRIDSIZE));
	Vector boarderPoint2 = center + (dir2*(pFormation->spotterRange*GRIDSIZE));

	//now score the two points;
	SINGLE score1 = dot_product(dir1,formationDir);
	SINGLE score2 = dot_product(dir2,formationDir);
	if(score1 >score2)
	{
		score1 = GRIDSIZE;
		score2 = GRIDSIZE*5;
	}
	else
	{
		score1 = GRIDSIZE*5;
		score2 = GRIDSIZE;
	}
	
	for(U32 i = 0; i < numAvoidPoints; ++i)
	{
		SINGLE dist1 = (boarderPoint1-avoidList[i]).magnitude_squared();
		if(dist1 < SPOT_SCORE_DIST*SPOT_SCORE_DIST)//7*7
		{
			score1 += (SPOT_SCORE_DIST*SPOT_SCORE_DIST)-dist1;
		}
		SINGLE dist2 = (boarderPoint2-avoidList[i]).magnitude_squared();
		if(dist2 < SPOT_SCORE_DIST*SPOT_SCORE_DIST)//7*7
		{
			score2 += (SPOT_SCORE_DIST*SPOT_SCORE_DIST)-dist2;
		}
	}
	SINGLE totalScore = score1+score2;
	SINGLE val = ((SINGLE)(rand()%10000))/10000.0f;
	if(totalScore*val > score1)
	{
		if(boarderPoint1.x < 0)
			boarderPoint1.x =0;
		if(boarderPoint1.y < 0)
			boarderPoint1.y =0;
		retVal = boarderPoint1;
	}
	else
	{
		if(boarderPoint2.x < 0)
			boarderPoint2.x =0;
		if(boarderPoint2.y < 0)
			boarderPoint2.y =0;
		retVal = boarderPoint2;
	}
	return retVal;
}
//------------------------------------------------------------------------------------------
//	
void Flagship::smartFleetTargetSelection (void)
{
	// do not set targets if targetting is off
	if (admiralTactic == AT_PEACE || fleetSet.numObjects == 0 || (DEFAULTS->GetDefaults()->bNoAutoTarget != 0 && DEFAULTS->GetDefaults()->bCheatsEnabled))
	{
		if (fleetSet.numObjects)
		{
			moveIdleShips();
		}
		return;
	}

	U32 i;
	
	// PART ONE: FIND OUT WHICH FLEET SHIPS ARE NOT IDLE
	//
	//
	queryIdleGunboats();

	// if all of the fleet ships are busy, then exit
	if (nAvailableGunboats == 0)
	{
		return;
	}

	// PART TWO: FIND PRIMARY AND SECONDARY TARGETS IF THEY ARE NOT YET DEFINDED
	//
	//
	findBestTargets(targets,transform.translation);
	if(admiralTactic == AT_SEEK && (!(targets[0])))
	{
		findBestTargets(targets,transform.translation,true);
	}

	if(admiralTactic == AT_DEFEND || admiralTactic == AT_SEEK)
	{
		// PART THREE: Split up the units to attack either the primary or secondary target
		//	if there are no targets, then check to see if any of your fleet members are too far away
		//
		if (targets[0])
		{
			USR_PACKET<USRATTACK> packet1;
			USR_PACKET<USRATTACK> packet2;
			int cnt1 = 0;
			int cnt2 = 0;
			IBaseObject * targ = NULL;
			MPart part1 = targets[1];

			for (i = 0; i < nAvailableGunboats; i++)
			{
				// only assign attack commands if the ship is in the same system as the admiral
				IBaseObject * gunboat = OBJLIST->FindObject(idleGunboatArray[i]);
				if (gunboat && gunboat->GetSystemID() == systemID)
				{
					if (part1.isValid() && nTotalGunboats > 5 && part1->caps.attackOk)
					{
						targ = targets[i%2];
					}
					else 
					{
						targ = targets[0];
					}

					if (targ == targets[0])
					{
						packet1.objectID[cnt1++] = idleGunboatArray[i];
					}
					else
					{
						packet2.objectID[cnt2++] = idleGunboatArray[i];
					}
				}
			}

			if (cnt1)
			{
				packet1.targetID = targets[0]->GetPartID();
				packet1.bUserGenerated = false;
				packet1.init(cnt1);
				NETPACKET->Send(HOSTID, 0, &packet1);
			}
			if (cnt2)
			{
				packet2.targetID = targets[1]->GetPartID();
				packet2.bUserGenerated = false;
				packet2.init(cnt2);
				NETPACKET->Send(HOSTID, 0, &packet2);
			}
		}
		else
		{
			// nothing to shoot at, but let's see if we need to move any fleet members closer to the admiral
			moveIdleShips(true);
		}
	}
	else if(admiralTactic == AT_HOLD)
	{
		if (targets[0])
		{
			USR_PACKET<USRATTACK> packet1;
			USR_PACKET<USRATTACK> packet2;
			int cnt1 = 0;
			int cnt2 = 0;
			IBaseObject * targ = NULL;
			MPart part1 = targets[1];

			for (i = 0; i < nAvailableGunboats; i++)
			{
				// only assign attack commands if the ship is in the same system as the admiral
				IBaseObject * gunboat = OBJLIST->FindObject(idleGunboatArray[i]);
				targ = NULL;
				if (gunboat && gunboat->GetSystemID() == systemID)
				{
					if (part1.isValid() && nTotalGunboats > 5 && part1->caps.attackOk)
					{
						VOLPTR(ILaunchOwner) launchOwner = gunboat;
						if(launchOwner)
						{
							bool targ0Range = (launchOwner->GetWeaponRange()/GRIDSIZE > targets[0]->GetGridPosition()-gunboat->GetGridPosition());
							bool targ1Range = (launchOwner->GetWeaponRange()/GRIDSIZE > targets[1]->GetGridPosition()-gunboat->GetGridPosition());
							if(targ0Range && targ1Range)
								targ = targets[i%2];
							else if(targ0Range)
								targ = targets[0];
							else if(targ1Range)
								targ = targets[1];
							else 
								findPersonalTarget(gunboat);
						}
						else
							targ = targets[i%2];
					}
					else 
					{
						VOLPTR(ILaunchOwner) launchOwner = gunboat;
						if(launchOwner)
						{
							if(launchOwner->GetWeaponRange()/GRIDSIZE > targets[0]->GetGridPosition()-gunboat->GetGridPosition())
								targ = targets[0];
							else
								findPersonalTarget(gunboat);
						}
						else
							targ = targets[0];
					}

					if (targ && targ == targets[0])
					{
						packet1.objectID[cnt1++] = idleGunboatArray[i];
					}
					else if(targ && targ == targets[1])
					{
						packet2.objectID[cnt2++] = idleGunboatArray[i];
					}
				}
			}

			if (cnt1)
			{
				packet1.targetID = targets[0]->GetPartID();
				packet1.bUserGenerated = false;
				packet1.init(cnt1);
				NETPACKET->Send(HOSTID, 0, &packet1);
			}
			if (cnt2)
			{
				packet2.targetID = targets[1]->GetPartID();
				packet2.bUserGenerated = false;
				packet2.init(cnt2);
				NETPACKET->Send(HOSTID, 0, &packet2);
			}
		}
		else
		{
			// nothing to shoot at, but let's see if we need to move any fleet members closer to the admiral
			moveIdleShips(true);
		}
	}
}
//------------------------------------------------------------------------------------------
//
void Flagship::smartUnitStances()
{
	for(U32 i = 0; i <fleetSet.numObjects; ++i)
	{
		VOLPTR(IAttack) attacker = OBJLIST->FindObject(fleetSet.objectIDs[i]);
		if(attacker)
		{
			switch(admiralTactic)
			{
			case AT_PEACE:
				attacker->SetUnitStance(US_STOP);
				attacker->SetFighterStance(FS_NORMAL);
				break;
			case AT_DEFEND:
				attacker->SetUnitStance(US_DEFEND);
				attacker->SetFighterStance(FS_NORMAL);
				break;
			case AT_HOLD:
				attacker->SetUnitStance(US_STAND);
				attacker->SetFighterStance(FS_NORMAL);
				break;
			case AT_SEEK:
				attacker->SetUnitStance(US_ATTACK);
				attacker->SetFighterStance(FS_PATROL);
				break;
			}
		}
	}

}
//------------------------------------------------------------------------------------------
//
BOOL32 Flagship::updateFlagship (void)
{
	if (fleetID == 0)
	{
		fleetID = dwMissionID;
	}

	if (bAttached)
	{
		disableAutoMovement();
		DEBUG_resetInputCounter();
	}

	if(!bExploding && bReady)
	{
		TECHNODE currentNode = MGlobals::GetCurrentTechLevel(this->GetPlayerID());
		if(!(currentNode.HasTech(M_TERRAN,0,0,TECHTREE::RESERVED_ADMIRAL,0,0,0)))
		{
			currentNode.race[race].common = (TECHTREE::COMMON)(((U32)currentNode.race[race].common)| TECHTREE::RESERVED_ADMIRAL);
			MGlobals::SetCurrentTechLevel(currentNode,this->GetPlayerID());
		}
	}

	if (dockAgentID)
	{
		if (dockship == 0 || (dockship.Ptr()->GetSystemID() != systemID))
		{
			// we may want to keep on trying to dock another ship
			onTargetCancelledOp();
		}
		else
		{
			MPartNC part = dockship.Ptr();
			if (part->admiralID != 0)
			{
				onTargetCancelledOp();
			}
			else
			{
				if (isMoveActive()==false)
				{
					if (THEMATRIX->IsMaster())
					{
						THEMATRIX->SendOperationData(dockAgentID, dwMissionID, NULL, 0);
						attachToDockShip(part);
						THEMATRIX->OperationCompleted(dockAgentID, dockshipID);
						THEMATRIX->OperationCompleted2(dockAgentID, dwMissionID);
						if (playerID == MGlobals::GetThisPlayer())
							FLAGSHIPCOMM(admiralondeck,SUB_ADMIRAL_ON_DECK);
					}
				}
			}
		}
	}
	else
	{
		if (MGlobals::IsUpdateFrame(dwMissionID))
			updateSpeech();

		if (bAttached)
		{
			ClearVisibilityFlags();
		}

		if (idleTimer > 0)
		{
			idleTimer--;
		}

		checkTargetPointers();

		if (mode == MOVING)
		{
			doMove();
		}
		else if (mode == DEFENDING)
		{
			doDefend();
		}

		if (idleTimer <= 0)
		{
			if (THEMATRIX->IsMaster() && MISSION->IsComputerControlled(playerID) == false)
			{
				doIdleBehavior();
			}
			idleTimer = IDLE_TICKS;
		}
	}

	//update command kit reasearch
	if(!buildQueueEmpty())
	{
		buildTime += ELAPSED_TIME;
		if(THEMATRIX->IsMaster())
		{
			U32 buildID = peekFabQueue();
			BT_COMMAND_KIT * kit = (BT_COMMAND_KIT *)(ARCHLIST->GetArchetypeData(buildID));
			if(kit)
			{
				if(kit->time < buildTime)
				{
					//I have finished
					U32 count;
					for(U32 count = 0; count < MAX_KNOWN_KITS; ++count)
					{
						if(!(commandKitsArchID[count]))
						{
							commandKitsArchID[count] = buildID;
							knownKits[count] = kit;
							break;
						}
					}
					for(count = 0; count < MAX_COMMAND_FORMATIONS; ++count)
					{
						if(kit->formations[count][0])
						{
							U32 formID = ARCHLIST->GetArchetypeDataID(kit->formations[count]);
							if(formID)
							{
								for(U32 formsIndex = 0; formsIndex <MAX_FORMATIONS; ++formsIndex)
								{
									if(knownFormations[formsIndex] == formID)
									{
										break;//I already know this formation
									}
									else if(knownFormations[formsIndex] == 0)
									{
										knownFormations[formsIndex] = formID;
										break;
									}
								}
							}
						}
					}
					
					popFabQueue();
					bSyncBuildFinished = true;
					syncFinsihedID = buildID;
					buildTime = 0;
					THEMATRIX->ForceSyncData(this);
				}
			}
		}
	}
	
	return 1;
}
//---------------------------------------------------------------------------
//
void Flagship::physUpdateFlagship (SINGLE dt)
{
	if (bAttached && dockship!=0 && ((dockship.Ptr()->GetSystemID() & HYPER_SYSTEM_MASK) == 0))
	{
		SetTransform(dockship.Ptr()->GetTransform(), dockship.Ptr()->GetSystemID());
		velocity = dockship.Ptr()->GetVelocity();
	}
}
//---------------------------------------------------------------------------
//
void Flagship::renderFormationInfo (void)
{
	if(DEFAULTS->GetDefaults()->bInfoHighlights)
	{
		if(pFormation && (!(pFormation->bFreeForm)) && formationPost.systemID == SECTOR->GetCurrentSystem())
		{
			if(targetFormationPost.systemID == SECTOR->GetCurrentSystem())
			{
				Vector center = targetFormationPost;
				drawCircle(center,2200,RGB(255,128,128));
				Vector dir(targetFormationDirX,targetFormationDirY,0);
				dir.normalize();
				Vector pos = center + dir*2700;
				drawLine(pos,center,RGB(255,128,128));
			}

			Vector center = formationPost;
			drawCircle(center,2000,RGB(255,255,255));
			Vector dir = Vector(formationDirX,formationDirY,0);
			dir.normalize();
			Vector pos = center + dir*2500;
			drawLine(pos,center,RGB(255,255,255));

			TRANSFORM postTrans;
			postTrans.translation = formationPost;
			Vector i(formationDirX,formationDirY,0);
			i.fast_normalize();
			Vector k(0,0,1);
			Vector j = cross_product(k,i);
			postTrans.set_i(i);
			postTrans.set_j(j);
			postTrans.set_k(k);

			FleetGroup * search = fleetGroups;
			while(search)
			{
				Vector groupCenter(search->postX*GRIDSIZE,search->postY*GRIDSIZE,0);
				if(flipFormationPost)
					groupCenter.y *= -1.0;
				groupCenter = postTrans.rotate_translate(groupCenter);
				drawCircle(groupCenter,search->radius*GRIDSIZE,RGB(0,128,255));
				search = search->next;
			}
		}
	}
}
//---------------------------------------------------------------------------
//
void Flagship::resolve (void)
{
	OBJLIST->FindObject(dockshipID, playerID, dockship, IFleetShipID);
	if (targetID)
		OBJLIST->FindObject(targetID, playerID, target );
}
//---------------------------------------------------------------------------
//
void Flagship::initFlagship (const FLAGSHIP_INIT & data)
{
	BT_FLAGSHIP_DATA * flagData = (BT_FLAGSHIP_DATA *) (ARCHLIST->GetArchetypeData(pArchetype));
	attackRadius = data.pData->attackRadius;
	admiralHotkey = BASE_FLAGSHIP_SAVELOAD::NO_HOTKEY;
	admiralTactic = netAdmiralTactic = AT_DEFEND;
	memset(knownFormations,0,sizeof(U32)*6);
	
	U32 count;
	for(count = 0; count < 2; ++count)
	{
		if(flagData->startingFormations[count][0])
		{
			U32 formID = ARCHLIST->GetArchetypeDataID(flagData->startingFormations[count]);
			if(formID)
				knownFormations[count] = formID;
		}
	}
	formationID = netFormationID = knownFormations[0];
	if(formationID)
        pFormation = (BT_FORMATION *)( ARCHLIST->GetArchetypeData(formationID));
	else
		pFormation = NULL;
	bReformFormation = false;
	fleetGroups = NULL;
	bHaveFormationPost = false;
	flipFormationPost = false;
	bRovingAllowed = false;
	for(count = 0; count < MAX_KNOWN_KITS; ++count)
	{
		commandKitsArchID[count] = 0;
		knownKits[count] = NULL;
	}
	bSyncQueueAdd = false;
	bSyncQueueRemove = false;
	bSyncQueueSound = false;
	bSyncBuildFinished = false;
	buildTime = 0;
}
//---------------------------------------------------------------------------
//
void Flagship::save (FLAGSHIP_SAVELOAD & save)
{
 	save.flagshipSaveLoad = *static_cast<BASE_FLAGSHIP_SAVELOAD *>(this);
}
//---------------------------------------------------------------------------
//
void Flagship::load (FLAGSHIP_SAVELOAD & load)
{
 	*static_cast<BASE_FLAGSHIP_SAVELOAD *>(this) = load.flagshipSaveLoad;
}
//---------------------------------------------------------------------------
//
void Flagship::explodeFlagship (bool bExplode)
{ 
	// we should not explode if we are attached, capeche?
//	CQASSERT(!bAttached && "Flagship is attached, it should not be exploding");

	// why should we bother moving the flagship around?  It's dying! -sb
//	if (bAttached)
//	{
//		UndockFlagship(0);
//	}

	//this is where we update the techtree of our loss....
	IBaseObject * obj = OBJLIST->GetObjectList();
	bool setNoPlatform = true;
	while(obj)
	{
		if((obj->GetPartID() & 0x80000000) && (obj != ((IBaseObject *)this)) &&
			(obj->GetPlayerID() == GetPlayerID()))
		{
			setNoPlatform = false;
			obj = NULL;
		}
		else
			obj = obj->next;
	}
	if(setNoPlatform)
	{
		TECHNODE oldTech = MGlobals::GetCurrentTechLevel(GetPlayerID());
		oldTech.race[M_TERRAN].build = (TECHTREE::BUILDNODE)(oldTech.race[M_TERRAN].build & (~(TECHTREE::RESERVED_ADMIRAL)));
		MGlobals::SetCurrentTechLevel(oldTech,GetPlayerID());
	}
	
	if (playerID == MGlobals::GetThisPlayer() && bExplode)
		SHIPCOMMDEATH(true);

	MPartNC part = dockship.Ptr();
	if (part)
		part->admiralID = 0;		// break the connection
	bAttached = false;
	dockship = 0;

	clearFleet();
}
//---------------------------------------------------------------------------
//
void Flagship::doMove (void)
{
}
//---------------------------------------------------------------------------
//
void Flagship::doDefend (void)
{
	//
	// move close to the defend ship
	// 
	if (defend != 0 && bAttached==false && MGlobals::IsUpdateFrame(dwMissionID))
	{
		SINGLE dist = defend->GetGridPosition()-GetGridPosition();

		if (dist*9 > attackRadius)
		{
			moveToPos(defend->GetGridPosition());

		}
	}
}
//---------------------------------------------------------------------------
//
void Flagship::updateSpeech (void)
{
	if (bReady && bExploding==false && playerID == MGlobals::GetThisPlayer())
	{
		MPart part;

		{
			U32 health = (hullPoints * 100) / hullPointsMax;

			if (health >= 35 && (dockshipID && (part=OBJLIST->FindObject(dockshipID)).isValid()))
			{
				health = (part->hullPoints * 100) / part->hullPointsMax;
			}

			if (dmessages.inTrouble==false && health < 35)
			{
				FLAGSHIPALERT(flagshipintrouble, SUB_ADMIRAL_FLAGSHIP_DANGER);
				dmessages.inTrouble=true;
			}
			else
			if (health > 75)
				dmessages.inTrouble=false;
		}

		{
			U32 health=0, supply=0;
			U32 i;
			U32 numObjects=0;
			U32 supplyObjects=0;
			
			fleetStrength = 0;

			for (i = 0; i < fleetSet.numObjects; i++)
			{
				if ((part = OBJLIST->FindObject(fleetSet.objectIDs[i])).isValid())
				{
					health   += (part->hullPoints * 100) / part->hullPointsMax;
					if (part->supplyPointsMax)
					{
						U32 percent = (part->supplies * 100) / part->supplyPointsMax;
						supply += percent;
						supplyObjects++;
						if (percent > 33)
						{
							switch (part.pInit->armorData.myArmor)
							{
							case NO_ARMOR:
							default:
								fleetStrength += part->hullPoints/2;
								break;
							case LIGHT_ARMOR:
								fleetStrength += part->hullPoints;
								break;
							case MEDIUM_ARMOR:
								fleetStrength += part->hullPoints*2;
								break;
							case HEAVY_ARMOR:
								fleetStrength += part->hullPoints*4;
								break;
							}
						}
						fleetStrength++;
					}
					numObjects++;
				}
			}

			if (numObjects>1)		// don't talk about fleet health unless you have more than one ship
			{
				if (numObjects+numDeadShips)
					health /= (numObjects+numDeadShips);			// counts dead people too

				if (health > 75)
				{
					dmessages.damage75 = dmessages.damage50 = false;
				}

				if (health < 25 && dmessages.damage75==false)
				{
					FLAGSHIPALERT(fleetdamage75, SUB_FLEET_75_DAMAGE);
					dmessages.damage75 = dmessages.damage50 = true;
				}

				if (health < 50 && dmessages.damage50==false)
				{
					FLAGSHIPALERT(fleetdamage50, SUB_FLEET_50_DAMAGE);
					dmessages.damage50 = true;
				}
			}

			if (supplyObjects)
			{
				if (supplyObjects)
					supply /= supplyObjects;

				if (supply > 75)
					smessages.suppliesOut = smessages.suppliesLow = false;

				
				if (supply < 15 && smessages.suppliesOut==false)
				{
					FLAGSHIP_NOSUPPLIES;
					smessages.suppliesOut = smessages.suppliesLow = true;
				}

				if (supply < 50 && smessages.suppliesLow ==false)
				{
					FLAGSHIPALERT(supplieslow, SUB_FLEET_LOW_SUPPLIES);
					smessages.suppliesLow = true;
				}
			}
		}

		if (forecastTimer > 0)
			forecastTimer--;
		else
		{
			forecastTimer = FORECAST_TICKS;

			if (target && target->IsVisibleToPlayer(playerID) && fleetStrength)		// are we attacking?
			{
				//
				// get sum of enemy strength
				//
				U32 enemyStrength = 0;
				IBaseObject *obj = OBJLIST->GetTargetList();
				const U32 allyMask = MGlobals::GetAllyMask(playerID);
				MPart part;

				while (obj)
				{
					if (obj->objMapNode != 0)
					{
						const U32 hisPlayerID = obj->GetPlayerID();
						
						// if we are enemies
						if (hisPlayerID && (((1 << (hisPlayerID-1)) & allyMask) == 0))
						{
							if (obj->GetSystemID() == systemID && obj->IsVisibleToPlayer(playerID))
							{
								part = obj;
								if (part->caps.attackOk)
								{
									if (part->supplies)
									{
										switch (part.pInit->armorData.myArmor)
										{
										case NO_ARMOR:
										default:
											enemyStrength += part->hullPoints/2;
											break;
										case LIGHT_ARMOR:
											enemyStrength += part->hullPoints;
											break;
										case MEDIUM_ARMOR:
											enemyStrength += part->hullPoints*2;
											break;
										case HEAVY_ARMOR:
											enemyStrength += part->hullPoints*4;
											break;
										}
									}
								}
							}
						}
					}

					obj = obj->nextTarget;
				}


				S32 diff = (enemyStrength * 100) / fleetStrength;

				if (fleetStrength <= MAX_SELECTED_UNITS)
					diff = __max(diff, 100);

				if (diff < 50)
				{
					if (lastForecast != GOOD)
						FLAGSHIPALERT(battlegood, SUB_BATTLE_GOOD);
					lastForecast = GOOD;
				}
				else
				if (diff > 200)
				{
					if (lastForecast != UGLY)
						FLAGSHIPALERT(battlebleak, SUB_BATTLE_BLEAK);
					lastForecast = UGLY;
				}
				else
				{
					if (lastForecast != BAD)
						FLAGSHIPALERT(battlemoderate, SUB_BATTLE_MODERATE);
					lastForecast = BAD;
				}
			}
		}
	}
}
//---------------------------------------------------------------------------
// find closest dockship to us, we're in trouble
bool Flagship::findBestDockship (IBaseObject * objIgnore)
{

	// make sure we are the host
	if (THEMATRIX->IsMaster() == false)
	{
		return false;
	}

	// attach to biggest ship we have
	//
	// find biggest ship that is defined in our fleet
	//
	IBaseObject * obj = OBJLIST->GetTargetList();     // mission objects
	MPart bestShip;
	MPart other;
	
	for (U32 i = 0; i < fleetSet.numObjects; i++)
	{
		obj = OBJLIST->FindObject(fleetSet.objectIDs[i]);
		other = obj;
		if (other.isValid())
		{
			if (bestShip.isValid())
			{
				if ((other->hullPoints > bestShip->hullPoints) && MGlobals::IsGunboat(other->mObjClass) && (other.obj != objIgnore))
				{
					bestShip = other;
				}
			}
			else if ((other->systemID == systemID) && (MGlobals::IsGunboat(other->mObjClass) == true) && (other.obj != objIgnore))
			{
				bestShip = other;
			}
		}
	}

	if (bestShip.isValid())
	{
		// fixit - look at this for example
		if (bAttached)
		{
			CQASSERT(dockship!=0);
			USR_PACKET<USRUNDOCKFLAGSHIP> packet;
			packet.objectID[0] = dwMissionID;
			packet.objectID[1] = dockshipID;
			packet.userBits = 0;	// clear out the queue - important
			packet.position.init(GetGridPosition(), systemID);
			packet.init(2, false);		// don't track this command
			NETPACKET->Send(HOSTID, 0, &packet);
		}
		
		USR_PACKET<USRDOCKFLAGSHIP> packet;

		packet.objectID[0] = dwMissionID;
		packet.objectID[1] = bestShip->dwMissionID;
		packet.userBits = bAttached;		// queue this (if we sent an UNDOCK command) - important
		packet.init(2);
		NETPACKET->Send(HOSTID, 0, &packet);
		return true;
	}

	return false;
}
//---------------------------------------------------------------------------
// find closest target to us
// 
bool Flagship::findAutoTarget (U32 * pTarget)
{
	bool result = false;

	if (DEFAULTS->GetDefaults()->bNoAutoTarget == 0 || DEFAULTS->GetDefaults()->bCheatsEnabled==0)
	{
		const U32 allyMask = MGlobals::GetAllyMask(playerID);
		SINGLE minDistance = 0;
		IBaseObject *obj = OBJLIST->GetTargetList();
		IBaseObject *bestTarget = 0;
		MISSION_DATA::M_CAPS caps;
		memset(&caps, 0, sizeof(caps));
		const SINGLE outerRange = attackRadius;
		MPart part;

		while (obj)
		{
			if (obj->objMapNode != 0)
			{
				const U32 hisPlayerID = obj->GetPlayerID();
				
				// if we are enemies
				if (hisPlayerID && (((1 << (hisPlayerID-1)) & allyMask) == 0))
				{
					if (obj->GetSystemID() == systemID && obj->IsVisibleToPlayer(playerID))
					{
						Vector relVec = obj->GetTransform().translation - transform.translation;
						SINGLE mag = obj->GetGridPosition() - GetGridPosition();
						part = obj;

						if (part.isValid() && part->hullPoints>0)
						{
							if (obj == target && mag < outerRange)
							{
								if ((part->caps.attackOk||part->caps.captureOk))
								{
									bestTarget = obj;
									minDistance = mag;
									break;
								}
							}

							if (mag < outerRange)
							{
								if (part.isValid()==0 || bestTarget==0 || (mag<minDistance&&caps.attackOk==0&&caps.captureOk==0) || (part->caps.attackOk||part->caps.captureOk))
								{
									bestTarget = obj;
									minDistance = mag;
									caps = part->caps;
								}
							}
						}
					}
				}
			}

			obj = obj->nextTarget;
		}

		if (bestTarget)
		{
			if (idleTimer < 0 || minDistance < outerRange)
			{
				// if we were attacked, or enemy within gun range, then return fire!
				if (mode == MOVING)
				{
					/*
					USR_PACKET<AISETTARGET> packet;
					U32 numObjects = OBJLIST->GetGroupMembers(groupID, packet.objectID);
					packet.init(numObjects);
					packet.targetID = bestTarget->GetPartID();
					// don't set target pointer while moving
					if (numObjects>0)
					{
						NETPACKET->Send(HOSTID, 0, &packet);
					}
					*/
					// we must be called through sync callback
					CQASSERT(pTarget!=0);
					*pTarget = bestTarget->GetPartID();
				}
				else
				{
					CQASSERT(pTarget==0);
					USR_PACKET<USRATTACK> packet;
					if ((packet.targetID = bestTarget->GetPartID()) != targetID)
					{
						memcpy(packet.objectID, fleetSet.objectIDs, sizeof(packet.objectID));
						packet.init(fleetSet.numObjects);
						targetID = packet.targetID;
						bestTarget->QueryInterface(IBaseObjectID, target, playerID);
						if (fleetSet.numObjects>0)
						{
							packet.bUserGenerated = false;
							NETPACKET->Send(HOSTID, 0, &packet);
						}
					}
				}

				result = true;
			}
		}
	}

	return result;
}
//---------------------------------------------------------------------------
//
void Flagship::clearFleet (void)
{
	// go through the fleet set and zero them out
	U32 i;
	IBaseObject * obj;
	MPartNC part;

	for (i = 0; i < fleetSet.numObjects; i++)
	{
		obj = OBJLIST->FindObject(fleetSet.objectIDs[i]);
		part = obj;
		
		if (part.isValid() && part->dwMissionID != dwMissionID)
		{
			// this ship is no longer part of a fleet
			part->fleetID = 0;
		}
	}

	fleetSet.numObjects = 0;
}
//---------------------------------------------------------------------------
//
void Flagship::attachToDockShip (const MPartNC & part)
{
	bAttached = true;
	bMoveDisabled = true;
	CQASSERT(part->admiralID==0);		// should not be already attached
	part->admiralID = dwMissionID;
	part->fleetID = dwMissionID;
	OBJLIST->UnselectObject(this);
	dockship->WaitForAdmiral(false);
	targetID = 0;
	UnregisterWatchersForObject(this);
	fieldFlags.zero();

	COMPTR<ITerrainMap> map;

	SECTOR->GetTerrainMap(systemID, map);
	undoFootprintInfo(map);
}
//---------------------------------------------------------------------------
//
void Flagship::preSelfDestruct (void)
{
	if (dockAgentID)
	{
		bool bIsMaster = THEMATRIX->IsMaster();

		// dockship is doing something else
		if (bIsMaster)
		{
			THEMATRIX->SendOperationData(dockAgentID, dwMissionID, &bIsMaster, sizeof(bIsMaster));	// signal cancel
			THEMATRIX->OperationCompleted(dockAgentID, dockshipID);
			dockshipID = 0;
			THEMATRIX->OperationCompleted2(dockAgentID, dwMissionID);
		}
	}
}
//---------------------------------------------------------------------------
//
U32 Flagship::getSyncStance (void * buffer)
{
	U32 result = 0;
	if (netAdmiralTactic != admiralTactic)
	{
		((U8*)buffer)[result++] = admiralTactic;
		netAdmiralTactic = admiralTactic;
	}
	return result;
}
//---------------------------------------------------------------------------
//
void Flagship::putSyncStance (void * buffer, U32 bufferSize, bool bLateDelivery)
{
	U8 stance = ((U8*)buffer)[--bufferSize];
	netAdmiralTactic = admiralTactic = ADMIRAL_TACTIC(stance);
}
//---------------------------------------------------------------------------
//
U32 Flagship::getSyncFormation (void * buffer)
{
	U32 result = 0;
	if (netFormationID != formationID)
	{
		((U32*)buffer)[result++] = formationID;
		netFormationID = formationID;
	}
	return result*sizeof(U32);
}
//---------------------------------------------------------------------------
//
void Flagship::putSyncFormation (void * buffer, U32 bufferSize, bool bLateDelivery)
{
	formationID = *((U32*)buffer);
	netFormationID = formationID;
	pFormation = (BT_FORMATION * )(ARCHLIST->GetArchetypeData(formationID));
}
//---------------------------------------------------------------------------
//
#define ADD_TO_QUEUE 1
#define REMOVE_TO_QUEUE 2
#define SOUND_QUEUE 3
#define FINSHIED_COMMAND_KIT 4
U32 Flagship::getSyncKit (void * buffer)
{
	U32 size = 0;
	if(bSyncQueueAdd)
	{
		bSyncQueueAdd = false;
		((U8*)buffer)[size] = ADD_TO_QUEUE;
		size += 1;
		*((U32*)(((U8*)buffer)+size)) = syncQueueAddData;
		size += sizeof(U32);
	}
	if(bSyncQueueRemove)
	{
		bSyncQueueRemove = false;
		((U8*)buffer)[size] = REMOVE_TO_QUEUE;
		size += 1;
		*((U32*)(((U8*)buffer)+size)) = syncQueueRemoveData;
		size += sizeof(U32);
	}
	if(bSyncQueueSound)
	{
		bSyncQueueSound = false;
		((U8*)buffer)[size] = SOUND_QUEUE;
		size += 1;
		*((U32*)(((U8*)buffer)+size)) = syncFailSound;
		size += sizeof(U32);
	}
	if(bSyncBuildFinished)
	{
		bSyncBuildFinished = false;
		((U8*)buffer)[size] = FINSHIED_COMMAND_KIT;
		size += 1;
		*((U32*)(((U8*)buffer)+size)) = syncFinsihedID;
		size += sizeof(U32);
	}
	
	return size;
}
//---------------------------------------------------------------------------
//
void Flagship::putSyncKit (void * buffer, U32 bufferSize, bool bLateDelivery)
{
	if(bufferSize)
	{
		U32 read = 0;
		while(read < bufferSize)
		{
			switch(((U8*)buffer)[read])
			{
			case ADD_TO_QUEUE:
				addToQueue(*((U32*)(((U8*)buffer)+read+1)));
				break;
			case REMOVE_TO_QUEUE:
				{
					U32 slotID = *((U32*)(((U8*)buffer)+read+1));
					U32 queuePos = getQueuePositionFromIndex(slotID);
					if(queuePos ==0)
						buildTime = 0;
					removeIndexFromQueue(slotID);
				}
				break;
			case SOUND_QUEUE:
				failSound((M_RESOURCE_TYPE)(*((U32*)(((U8*)buffer)+read+1))));
				break;
			case FINSHIED_COMMAND_KIT:
				{
					U32 buildID = *((U32*)(((U8*)buffer)+read+1));
					BT_COMMAND_KIT * kit = (BT_COMMAND_KIT *)(ARCHLIST->GetArchetypeData(buildID));
					U32 count;
					for(count = 0; count < MAX_KNOWN_KITS; ++count)
					{
						if(!(commandKitsArchID[count]))
						{
							commandKitsArchID[count] = buildID;
							knownKits[count] = kit;
							break;
						}
					}
					for(count = 0; count < MAX_COMMAND_FORMATIONS; ++count)
					{
						if(kit->formations[count][0])
						{
							U32 formID = ARCHLIST->GetArchetypeDataID(kit->formations[count]);
							if(formID)
							{
								for(U32 formsIndex = 0; formsIndex <MAX_FORMATIONS; ++formsIndex)
								{
									if(knownFormations[formsIndex] == formID)
									{
										break;//I already know this formation
									}
									else if(knownFormations[formsIndex] == 0)
									{
										knownFormations[formsIndex] = formID;
										break;
									}
								}
							}
						}
					}
					popFabQueue();
				}
				break;
			}
			read += sizeof(U32)+1;
		}
	}
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//
inline struct IBaseObject * createFlagship (const FLAGSHIP_INIT & data)
{
	Flagship * obj = new ObjectImpl<Flagship>;

	obj->FRAME_init(data);
	return obj;
}
//------------------------------------------------------------------------------------------
//---------------------------Flagship Factory------------------------------------------------
//------------------------------------------------------------------------------------------
//
struct DACOM_NO_VTABLE FlagshipFactory : public IObjectFactory
{
	struct OBJTYPE : FLAGSHIP_INIT
	{
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

	BEGIN_DACOM_MAP_INBOUND(FlagshipFactory)
	DACOM_INTERFACE_ENTRY(IObjectFactory)
	END_DACOM_MAP()

	FlagshipFactory (void) { }

	~FlagshipFactory (void);

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

	/* FlagshipFactory methods */

	IDAComponent * getBase (void)
	{
		return static_cast<IObjectFactory *> (this);
	}
};
//--------------------------------------------------------------------------//
//
FlagshipFactory::~FlagshipFactory (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (OBJLIST && OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
		connection->Unadvise(factoryHandle);
}
//--------------------------------------------------------------------------//
//
void FlagshipFactory::init (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
		connection->Advise(getBase(), &factoryHandle);
}
//-----------------------------------------------------------------------------
//
HANDLE FlagshipFactory::CreateArchetype (const char *szArchname, OBJCLASS objClass, void *_data)
{
	OBJTYPE * result = 0;

	if (objClass == OC_SPACESHIP)
	{
		BT_FLAGSHIP_DATA * data = (BT_FLAGSHIP_DATA *) _data;

		if (data->type == SSC_FLAGSHIP)	   
		{
			result = new OBJTYPE();
			if (result->loadSpaceshipArchetype(data, ARCHLIST->GetArchetype(szArchname)) == false)
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
BOOL32 FlagshipFactory::DestroyArchetype (HANDLE hArchetype)
{
	OBJTYPE * objtype = (OBJTYPE *)hArchetype;

	EditorStopObjectInsertion(objtype->pArchetype);
	delete objtype;

	return 1;
}
//-------------------------------------------------------------------
//
IBaseObject * FlagshipFactory::CreateInstance (HANDLE hArchetype)
{
	OBJTYPE * objtype = (OBJTYPE *)hArchetype;
	return createFlagship(*objtype);
}
//-------------------------------------------------------------------
//
void FlagshipFactory::EditorCreateInstance (HANDLE hArchetype, const MISSION_INFO & info)
{
	OBJTYPE * objtype = (OBJTYPE *)hArchetype;
	EditorStartObjectInsertion(objtype->pArchetype, info);
}
//---------------------------------------------------------------------------
//-------------------------GLOBAL STARTUP------------------------------------
//---------------------------------------------------------------------------
//
struct _flagship : GlobalComponent
{
	FlagshipFactory * sfactory;

	virtual void Startup (void)
	{
		sfactory = new DAComponent<FlagshipFactory>;
		AddToGlobalCleanupList((IDAComponent **) &sfactory);
	}

	virtual void Initialize (void)
	{
		sfactory->init();
	}
};

static _flagship __flag;

//-----------------------------------------------------------------------------
//-----------------------------End FlagShip.cpp--------------------------------
//-----------------------------------------------------------------------------
