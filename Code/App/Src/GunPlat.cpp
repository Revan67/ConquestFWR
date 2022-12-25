//--------------------------------------------------------------------------//
//                                                                          //
//                               GunPlat.cpp                                //
//                                                                          //
//                  COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/GunPlat.cpp 147   6/22/01 2:09p Tmauer $
*/			    
//--------------------------------------------------------------------------//

#include "pch.h"
#include <globals.h>

#include "TPlatform.h"
#include "Startup.h"
#include "MGlobals.h"
#include "MPart.h"
#include "ObjList.h"
#include "GenData.h"
#include "IAttack.h"
#include "ILauncher.h"
#include "CommPacket.h"
#include "DResearch.h"
#include "TerrainMap.h"
#include "ObjMapIterator.h"

#include <DGunPlat.h>

#include <Physics.h>
#include <3DMath.h>
#include <FileSys.h>
#include <IConnection.h>
#include <TComponent.h>
#include <HeapObj.h>

#define GUNP_C_UPGRADE_FINISH		0
#define GUNP_C_UPGRADE_START		1
#define GUNP_C_UPGRADE_START_DELAY	2
#define GUNP_C_UPGRADE_REMOVE_DELAY 3
#define P_OP_QUEUE_FAIL				4
#define GUNP_C_UPGRADE_FORGET		5
#define GUNP_C_LAUNCHER_OP			6

#define LOP_SEND_DATA	1
#define LOP_END			2
#define LOP_CANCEL		3

struct GunPlatCommand
{
	U8 command;
};

struct GunPlatCommandWHandle
{
	U8 command;
	U32 handle;
};

#define HOST_CHECK THEMATRIX->IsMaster()

#define COMP_OP(operation) { U32 tempID = workingID; workingID = 0; THEMATRIX->OperationCompleted(tempID,operation);}

#define LOSE_TARGET_TIMER		U32(1+ 5 * DEF_REALTIME_FRAMERATE)
#define EXT_IDLE_TIMER			U32(1+ 3 * DEF_REALTIME_FRAMERATE)
#define IDLE_TIMER				U32(1+ 3 * DEF_REALTIME_FRAMERATE)

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//

struct _NO_VTABLE Gunplat : Platform<GUNPLAT_SAVELOAD, GUNPLAT_INIT>,
									ISpender, IAttack, ILaunchOwner, IBuildQueue,  BASE_GUNPLAT_SAVELOAD
{
	BEGIN_MAP_INBOUND(Gunplat)
	_INTERFACE_ENTRY(IBaseObject)
	_INTERFACE_ENTRY(ISaveLoad)
	_INTERFACE_ENTRY(IQuickSaveLoad)
	_INTERFACE_ENTRY(IWeaponTarget)
	_INTERFACE_ENTRY(IExplosionOwner)
	_INTERFACE_ENTRY(IMissionActor)
	_INTERFACE_ENTRY(IBuild)
	_INTERFACE_ENTRY(IPlatform)
	_INTERFACE_ENTRY(IPhysicalObject)
	_INTERFACE_ENTRY(IAttack)
	_INTERFACE_ENTRY(ILaunchOwner)
	_INTERFACE_ENTRY(ISpender)
	_INTERFACE_ENTRY(IShipDamage)
	_INTERFACE_ENTRY(IExtent)
	_INTERFACE_ENTRY(IBuildQueue)
	_INTERFACE_ENTRY(IUpgrade)
	_INTERFACE_ENTRY(IEffectTarget)
	END_MAP()

	//---------------------------------------------------------------------------
	//
	struct TCallback : ITerrainSegCallback
	{
		U32 buildPlanetID;
		GRIDVECTOR gridPos;
		U32 blockingMissionID;

		TCallback (U32 planetID)
		{
			buildPlanetID = planetID;
			blockingMissionID = 0;
		}

		virtual bool TerrainCallback (const struct FootprintInfo & info, struct GRIDVECTOR & pos)
		{
			if (info.flags & TERRAIN_BLOCKLOS)
			{
				if (info.missionID != buildPlanetID)		// something else is in the way
				{
					if (info.missionID && info.missionID == blockingMissionID)
					{
						// we've already checked out this dude, now we fail
						return false;
					}

					// is this thing that's blocking our los a planet
					// record the position of what's blocking our los
					blockingMissionID = info.missionID;
					gridPos = pos;
				}
			}
			return true;
		}
	};


	OBJPTR<IBaseObject> target;					// can now be enemy or friend
	OBJPTR<ILauncher> launcher[MAX_GUNPLAT_LAUNCHERS];	
	OBJPTR<ILauncher> specialLauncher;
	SINGLE outerWeaponRange;

	ResourceCost workingCost;
	SINGLE workTime;

	bool bNoLineOfSight:1;		// true means we don't line of sight

	PhysUpdateNode  physUpdateNode;
	UpdateNode		updateNode1;
	UpdateNode		updateNode2;
	ExplodeNode		explodeNode;
	SaveNode		saveNode;
	LoadNode		loadNode;
	ResolveNode		resolveNode;
	PreTakeoverNode preTakeoverNode;
	ReceiveOpDataNode receiveOpDataNode;
	PreDestructNode	destructNode;
	OnOpCancelNode	onOpCancelNode;
	UpgradeNode		upgradeNode;
	GeneralSyncNode  genSyncNode1;
	GeneralSyncNode  genSyncNode2;
	GeneralSyncNode  genSyncNode3;
	GeneralSyncNode  genSyncNode4;

	UNIT_STANCE netUnitStance;
	FighterStance netFighterStance;
	U32			netTargetID;
	int loseTargetTimer;
	int idleTimer, extIdleTimer, moveIdleTimer;

	const GUNPLAT_INIT * ginitData;

	Gunplat (void);

	virtual ~Gunplat (void);	// See ObjList.cpp

	virtual void COMAPI destroy_instance (INSTANCE_INDEX index)
	{
		CQASSERT(index == instanceIndex);
		destroyLaunchers();
		instanceIndex = -1;
	}


	/* IBaseObject methods */

	virtual void Render (void);

	virtual void RevealFog (const U32 currentSystem);

	virtual void CastVisibleArea (void);			

	virtual void DrawHighlighted (void);

	/* IExplosionOwner methods */

	virtual void OnChildDeath (INSTANCE_INDEX child);


	//ISpender

	virtual ResourceCost GetAmountNeeded();
	
	virtual void UnstallSpender();

	// IMissionActor
	virtual void OnOperationCreation (U32 agentID, void *buffer, U32 bufferSize);

	void receiveOperationData (U32 agentID, void *buffer, U32 bufferSize);

	void onOperationCancel (U32 agentID);
	
	virtual void OnStopRequest (U32 agentID);

	virtual void OnMasterChange (bool bIsMaster);

	/* IBuildQueue */

	virtual void BuildQueue (U32 dwArchetypeDataID, COMMANDS command); 

	virtual U32 GetNumInQueue (U32 value);

	virtual bool IsInQueue (U32 value);

	virtual bool IsUpgradeInQueue ();

	virtual U32 GetQueue(U32 * queueCopy,U32 * slotIDs = NULL);

	virtual SINGLE FabGetProgress(U32 & stallType);

	virtual SINGLE FabGetDisplayProgress(U32 & stallType);

	virtual U32 GetFabJobID ();

	/* IAttack methods */
	
	virtual void Attack (IBaseObject * victim, U32 agentID, bool bUserGenerated);	// moves to attack

	virtual void AttackPosition(const struct GRIDVECTOR & position, U32 agentID); 

	virtual void CancelAttack (void);

	virtual void SpecialAttack (IBaseObject * victim, U32 agentID);

	virtual void SpecialAOEAttack (const struct GRIDVECTOR & position, U32 agentID);

	virtual void WormAttack (IBaseObject * victim, U32 agentID);

	virtual void ReportKill (U32 partID);

	virtual void Escort (IBaseObject * target, U32 agentID)
	{
		// does nothing
	}

	virtual void DoSpecialAbility (U32 specialID);

	virtual void DoSpecialAbility (IBaseObject *obj)
	{
	}

	virtual void DoCloak (void) {};

	virtual void MultiSystemAttack (struct GRIDVECTOR & position, U32 targSystemID, U32 agentID);

	virtual void DoCreateWormhole(U32 systemID, U32 agentID);

	virtual void SetUnitStance (const UNIT_STANCE stance)
	{
		unitStance = stance;
	}

	virtual const UNIT_STANCE GetUnitStance () const
	{
		return unitStance;
	}

	virtual void SetFighterStance (const FighterStance stance)
	{
		fighterStance = stance;
		int i;
		for (i = 0; i < MAX_GUNBOAT_LAUNCHERS; i++)
		{
			if (launcher[i])
			{
				launcher[i]->SetFighterStance(fighterStance);
			}
		}

	}

	virtual const FighterStance GetFighterStance (void)
	{
		return fighterStance;
	}

	void GetTarget(IBaseObject* & targObj, U32 targID)
	{
		targObj = target;
		targID = dwTargetID;
	}

	virtual void  GetSpecialAbility (UNIT_SPECIAL_ABILITY & ability, bool & bSpecialEnabled)
	{
		ability = USA_NONE;
		bSpecialEnabled = false;
	}

	virtual void OnAllianceChange (U32 allyMask)
	{
		if (THEMATRIX->IsMaster())
		{
			// if we are attacking someone, then see if they are a new ally
			if (attackAgentID && dwTargetID && target)
			{
				U32 dummy;
				U8 hisPlayerID=0;
				
				VOLPTR(IExtent) extentObj = target;
				
				if (extentObj)
				{
					extentObj->GetAliasData(dummy,hisPlayerID);
				}

				if (hisPlayerID == 0)
				{
					hisPlayerID = dwTargetID & PLAYERID_MASK;
				}
						
				// if we are now allied with the target then end the operation
				if (hisPlayerID && (((1 << (hisPlayerID-1)) & allyMask) != 0))
				{
					// send the message indicated we have accomplished our task
					cancelAttack();
				}
			}

			// go through all of our launchers and tell them to stop attacking any new allies
			int i;
			for (i = 0; i < MAX_GUNBOAT_LAUNCHERS; i++)
			{
				if (launcher[i])
				{
					launcher[i]->OnAllianceChange(allyMask);
				}
			}
		}
	}

	/* IMissionActor methods */

	virtual void InitActor (void);

	/* ILauchOwner methods */

	virtual bool UseSupplies (U32 amount,bool bAbsolute = false);	// return true if supplies were available

	virtual bool TestLOS (const struct GRIDVECTOR & pos);

	virtual void GetLauncher (const U32 index, OBJPTR<ILauncher> & objLauncher)
	{
		CQASSERT(index < MAX_GUNBOAT_LAUNCHERS);
		
		if (launcher[index])
		{
			launcher[index]->QueryInterface(ILauncherID, objLauncher);
		}
		else
		{
			objLauncher = NULL;
		}
	}

	virtual void LauncherCancelAttack();

	virtual void GotoLauncherPosition(GRIDVECTOR pos);

	virtual void LauncherSendOpData(U32 agentID, void * buffer,U32 bufferSize);

	virtual void LaunchOpCompleted(ILauncher * launcher,U32 agentID);

	virtual U32 CreateLauncherOp(ILauncher * launcher,struct ObjSet & set,void * buffer,U32 bufferSize);

	virtual void EnableCloak (bool bEnable);

	virtual SINGLE GetWeaponRange (void);

	virtual SINGLE GetOptimalWeaponRange (void);

	virtual IBaseObject * GetTarget (void)
	{
		return target;
	}

	virtual bool HasAttackAgent (void)
	{
		return (attackAgentID != 0);
	}

	virtual IBaseObject * FindChildTarget(U32 childID);

	/* ISaveLoad methods */

	virtual BOOL32 Save (struct IFileSystem * file);

	virtual BOOL32 Load (struct IFileSystem * file);


	
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

	void destroyLaunchers (void);

	virtual void ParkYourself (const TRANSFORM & _trans, U32 planetID, U32 slotID);

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

	virtual bool IsTempSupply();
	
	/* Platform methods */

	virtual const char * getSaveStructName (void) const		// must be overriden implemented by derived class
	{
		return "GUNPLAT_SAVELOAD";
	}

	virtual void * getViewStruct (void)				// must be overriden implemented by derived class
	{
		return static_cast<BASE_GUNPLAT_SAVELOAD *>(this);
	}

	/* Gunplat methods */

	BOOL32 updatePlat (void);

	BOOL32 updateTargeting (void);

	bool checkObjectInRange(U32 dwObjectID);
	
	void physUpdatePlat (SINGLE dt);

	void explodePlat (bool bExplode);

	void preSelfDestruct (void);

	bool initGunplat (const GUNPLAT_INIT & data);

	void findAutoTarget (void);

	bool checkLOS (TCallback & callback);

	void save (GUNPLAT_SAVELOAD & save);

	void load (GUNPLAT_SAVELOAD & load);

	void resolve (void);

	void preTakeover (U32 newMissionID, U32 troopID);

	void initLaunchers (void);

	void upgradePlat (const GUNPLAT_INIT & data);

	// sync methods
	U32 getSyncStance (void * buffer);
	void putSyncStance (void * buffer, U32 bufferSize, bool bLateDelivery);

	U32 getSyncFighterStance (void * buffer);
	void putSyncFighterStance (void * buffer, U32 bufferSize, bool bLateDelivery);

	U32 getSyncLaunchers (void * buffer);
	void putSyncLaunchers (void * buffer, U32 bufferSize, bool bLateDelivery);

	U32 getSyncTarget (void * buffer);
	void putSyncTarget (void * buffer, U32 bufferSize, bool bLateDelivery);

	void setAttackTarget (IBaseObject * victim);		// tells launchers to attack

	void sendAttackPacket (void)
	{
		CQASSERT(target && dwTargetID);

		USR_PACKET<USRATTACK> packet;
		packet.objectID[0] = dwMissionID;
		packet.targetID = dwTargetID;
		packet.bUserGenerated = 0;
		packet.init(1);
		NETPACKET->Send(HOSTID, 0, &packet);
	}

	void cancelAttack (void)
	{
		CQASSERT(THEMATRIX->IsMaster());
		if (attackAgentID)
		{
			// send the message indicated we have accomplished our task
			if (bSpecialAttack)
			{
				int i;
				for (i = 0; i < MAX_GUNBOAT_LAUNCHERS; i++)
				{
					if (launcher[i])
					{
						launcher[i]->InformOfCancel();
					}
				}
			}
			setAttackTarget(0);		// make sure launchers turn off
			THEMATRIX->SendOperationData(attackAgentID, dwMissionID, NULL,0);
			THEMATRIX->OperationCompleted2(attackAgentID, dwMissionID);
			bSpecialAttack = bUserGenerated = false;
			idleTimer = IDLE_TIMER;
			netTargetID = dwTargetID = 0;
			target = 0;
			extIdleTimer = EXT_IDLE_TIMER;
		}
	}

	bool canAutoTarget (void) const
	{
		return (bReady && unitStance != US_STOP && bUpgrading==false &&
			caps.attackOk && (DEFAULTS->GetDefaults()->bNoAutoTarget==0||DEFAULTS->GetDefaults()->bCheatsEnabled==0));
	}

};
//---------------------------------------------------------------------------
//
Gunplat::Gunplat (void) :
			updateNode1(this, UpdateProc(&Gunplat::updatePlat)),
			updateNode2(this, UpdateProc(&Gunplat::updateTargeting)),
			explodeNode(this, ExplodeProc(&Gunplat::explodePlat)),
			saveNode(this, CASTSAVELOADPROC(&Gunplat::save)),
			loadNode(this, CASTSAVELOADPROC(&Gunplat::load)),
			resolveNode(this, ResolveProc(&Gunplat::resolve)),
			physUpdateNode(this, PhysUpdateProc(&Gunplat::physUpdatePlat)),
			preTakeoverNode(this, PreTakeoverProc(&Gunplat::preTakeover)),
			receiveOpDataNode(this, ReceiveOpDataProc(&Gunplat::receiveOperationData)),
			destructNode(this, PreDestructProc(&Gunplat::preSelfDestruct)),
			onOpCancelNode(this, OnOpCancelProc(&Gunplat::onOperationCancel)),
			upgradeNode(this, UpgradeProc(CASTINITPROC(&Gunplat::upgradePlat))),
			genSyncNode1(this, SyncGetProc(&Gunplat::getSyncStance), SyncPutProc(&Gunplat::putSyncStance)),
			genSyncNode2(this, SyncGetProc(&Gunplat::getSyncTarget), SyncPutProc(&Gunplat::putSyncTarget)),
			genSyncNode3(this, SyncGetProc(&Gunplat::getSyncLaunchers), SyncPutProc(&Gunplat::putSyncLaunchers)),
			genSyncNode4(this, SyncGetProc(&Gunplat::getSyncFighterStance), SyncPutProc(&Gunplat::putSyncFighterStance))
{
	idleTimer = IDLE_TIMER;
	loseTargetTimer = LOSE_TARGET_TIMER;
	extIdleTimer = EXT_IDLE_TIMER;
}
//---------------------------------------------------------------------------
//
Gunplat::~Gunplat (void)
{
	destroyLaunchers();
}
//---------------------------------------------------------------------------
//
void Gunplat::destroyLaunchers (void)
{
	int i;

	for (i = 0; i < MAX_GUNPLAT_LAUNCHERS; i++)
	{
		delete launcher[i].Ptr();
		launcher[i] = 0;
	}

	delete specialLauncher.Ptr();
	specialLauncher = 0;
}
//---------------------------------------------------------------------------
// NOTE: we have to be careful about when we send the Operation_Complete message, 
// because the target is not synchronized with us. He may be doing differnt things on a
// other machines. If the host machine completes early (e.g. the target is not in the system yet),
// then the client machines can get stuck.
// Therefore, we only complete when the target is dead (or has death pending).
//
BOOL32 Gunplat::updatePlat (void)
{
	if(bPlatDead)
		return 1;
	if(bUpgrading && bReady && SECTOR->SystemInSupply(systemID,playerID))
	{
		if(!bDelayed)
		{
			buildTimeSpent += ELAPSED_TIME ;
			if(buildTimeSpent > workTime)
			{
				if(THEMATRIX->IsMaster())
				{
					BT_UPGRADE * buildType = (BT_UPGRADE *)(ARCHLIST->GetArchetypeData(upgradeID));
					COMM_MUTATION_COMPLETED(((char *)(buildType->resFinishedSound)),buildType->resFinishSubtitle,pInitData->displayName);

					SetUpgrade(buildType->extensionID);
					bUpgrading = false;
					caps.attackOk = true;
					GunPlatCommand buffer;
					buffer.command = GUNP_C_UPGRADE_FINISH;
					upgradeID = 0;
					THEMATRIX->SendOperationData(workingID,dwMissionID,&buffer,sizeof(buffer));
					COMP_OP(dwMissionID);
				}
			}
			if(buildTimeSpent >= 0 && buildTimeSpent <= workTime)
				SetUpgradePercent(buildTimeSpent/workTime);
		}
	}

	if (bReady)
	{
		for (int i = 0; i < MAX_GUNPLAT_LAUNCHERS; i++)
		{
			if (launcher[i].Ptr())
			{
				launcher[i].Ptr()->Update();
			}
		}

		if (specialLauncher.Ptr())
		{
			specialLauncher.Ptr()->Update();
		}
	}

	return 1;
}
//---------------------------------------------------------------------------
//
BOOL32 Gunplat::updateTargeting (void)
{
	if (bExploding)
		return 1;
	if (THEMATRIX->IsMaster() == 0)
		return 1;

	bool bLostSight = false;		// if not visible or don't have LOS

	if (target!=0)
	{
		if (target->IsVisibleToPlayer(playerID) == 0)
			bLostSight = true;
		else
		{
			TCallback callback(buildPlanetID);
			if (!checkObjectInRange(dwTargetID) || (bNoLineOfSight && !checkLOS(callback)))
				bLostSight = true;
		}
	}

	//
	// do the targeting code...
	//

	if (bUserGenerated)
	{
		CQASSERT(attackAgentID);

		MPart part = target;
		if (part.isValid()==0 || part->hullPoints==0)
			cancelAttack();
		else
		{
			if (bLostSight==false)
				loseTargetTimer = LOSE_TARGET_TIMER;
			else
			{
				if (--loseTargetTimer < 0)
				{
					cancelAttack();
				}
			}
		}
	}
	else
	if (canAutoTarget())
	{
		if (--idleTimer < 0)
		{
			idleTimer = IDLE_TIMER;

			if (attackAgentID!=0)
			{
				findAutoTarget();
			}
			else
			if (THEMATRIX->HasPendingOp(this))	// working on something else
			{
				findAutoTarget();
			}
			// NOTE: Do not do any targeting if we have no agent!
		}
	}
	else  // not allowed to target, and have no agent
	if (attackAgentID==0)
	{
		// officially stop attacking if we loose sight of the target
		if (bLostSight)
		{
			target = 0;
			dwTargetID = 0;
		}
	}

	//
	// agent management 
	//

	if (attackAgentID!=0)
	{
		if (bUserGenerated == 0)	// already handled the other case above
		{
			if (target==0)
			{
				if (--loseTargetTimer < 0)		// don't cancel attack immediately
					cancelAttack();
			}
			else
			{
				if (bLostSight==false)
					loseTargetTimer = LOSE_TARGET_TIMER;
				else
				{
					if (--loseTargetTimer < 0)
					{
						cancelAttack();
					}
				}
			}
		}
	}
	else
	if (canAutoTarget() && THEMATRIX->HasPendingOp(this) == 0)		// if totally idle
	{
		if (--extIdleTimer < 0)
		{
			extIdleTimer = EXT_IDLE_TIMER;

			target = 0;			// clear current target, if there is one
			dwTargetID = 0;	

			findAutoTarget();

			// if we found a target, initiate an attack command
			if (target!=0)
			{
				sendAttackPacket();
				target = 0;			// clear this target until we get the word
				dwTargetID = 0;	
			}
		}
	}
	
	return 1;
}
//---------------------------------------------------------------------------
//
void Gunplat::physUpdatePlat (SINGLE dt)
{
	int i;

	for (i = 0; i < MAX_GUNPLAT_LAUNCHERS; i++)
		if (launcher[i].Ptr())
			launcher[i].Ptr()->PhysicalUpdate(dt);

	if (specialLauncher.Ptr())
		specialLauncher.Ptr()->PhysicalUpdate(dt);
}
//---------------------------------------------------------------------------
//
void Gunplat::explodePlat (bool bExplode)
{
	destroyLaunchers();
}
//---------------------------------------------------------------------------
//
void Gunplat::preSelfDestruct (void)
{
	if (attackAgentID)
	{
		attackAgentID = 0;
	}
	if(THEMATRIX->IsMaster() && bUpgrading)
	{
		BANKER->FreeCommandPt(playerID,workingCost.commandPt);
	}
	if(launcherAgentID)
	{
		launcher[launcherID]->LauncherOpCompleted(launcherAgentID);

		U8 buffer = LOP_CANCEL;
		U32 bufferSize = 1;
		THEMATRIX->SendOperationData(launcherAgentID,dwMissionID,&buffer,bufferSize);
		THEMATRIX->OperationCompleted(launcherAgentID,dwMissionID);
		launcherAgentID = 0;
	}
}
//---------------------------------------------------------------------------
//
void Gunplat::ParkYourself (const TRANSFORM & _transform, U32 planetID, U32 slotID)
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
	if(mObjClass == M_JUMPPLAT || mObjClass == M_HQ || mObjClass == M_COCOON || mObjClass == M_ACROPOLIS || mObjClass == M_LOCUS)
	{
		SECTOR->ComputeSupplyForAllPlayers();
	}
}
//---------------------------------------------------------------------------
//
bool Gunplat::IsTempSupply()
{
	for (U32 i = 0; i < MAX_GUNPLAT_LAUNCHERS; i++)
	{
		VOLPTR(ISystemSupplier) sup = launcher[i].Ptr();
		if (sup)
		{
			return sup->IsTempSupply();
		}
	}
	return false;
}
//---------------------------------------------------------------------------
//
void Gunplat::InitActor (void)
{
	Platform<GUNPLAT_SAVELOAD, GUNPLAT_INIT>::InitActor();
	initLaunchers();
}
//---------------------------------------------------------------------------
//
U32 Gunplat::getSyncStance (void * buffer)
{
	U32 result = 0;
	if (netUnitStance != unitStance)
	{
		((U8*)buffer)[result++] = unitStance;
		netUnitStance = unitStance;
	}
	return result;
}
//---------------------------------------------------------------------------
//
void Gunplat::putSyncFighterStance (void * buffer, U32 bufferSize, bool bLateDelivery)
{
	U8 stance = ((U8*)buffer)[--bufferSize];
	netUnitStance = unitStance = UNIT_STANCE(stance);
}
//---------------------------------------------------------------------------
//
U32 Gunplat::getSyncFighterStance (void * buffer)
{
	U32 result = 0;
	if (netFighterStance != fighterStance)
	{
		((U8*)buffer)[result++] = fighterStance;
		netFighterStance = fighterStance;
	}
	return result;
}
//---------------------------------------------------------------------------
//
void Gunplat::putSyncStance (void * buffer, U32 bufferSize, bool bLateDelivery)
{
	U8 stance = ((U8*)buffer)[--bufferSize];
	netFighterStance = fighterStance = FighterStance(stance);
}
//---------------------------------------------------------------------------
//
U32 Gunplat::getSyncLaunchers (void * buffer)
{
	U32 result = 0;

	if (bPlatDead==0)
	{
		int i;
		
		for (i = 0; i < MAX_GUNPLAT_LAUNCHERS; i++)
		{
			if (launcher[i].Ptr() && (result = launcher[i]->GetSyncData(buffer)) != 0)
			{
				((U8*)buffer)[result++] = i+1;
				break;
			}
		}

		if (result==0 && specialLauncher.Ptr() && (result = specialLauncher->GetSyncData(buffer)) != 0)
		{
			((U8*)buffer)[result++] = MAX_GUNPLAT_LAUNCHERS+1;
		}
	}

	return result;
}
//---------------------------------------------------------------------------
// gunplats CAN receive late data because they are sometimes involved in moves, and jumps do to the UI. (jy)
//
void Gunplat::putSyncLaunchers (void * buffer, U32 bufferSize, bool bLateDelivery)
{
	U8 a = ((U8*)buffer)[--bufferSize];

	if (a <= MAX_GUNPLAT_LAUNCHERS)
		launcher[a-1]->PutSyncData(buffer, bufferSize);
	else
	if (a == MAX_GUNPLAT_LAUNCHERS+1)
		specialLauncher->PutSyncData(buffer, bufferSize);
	else
		CQBOMB0("Invalid sync data");
}
//---------------------------------------------------------------------------
//
U32 Gunplat::getSyncTarget (void * _buffer)
{
	dwTargetID = (target!=0) ? target->GetPartID() : 0;

	if (netTargetID == dwTargetID)
		return 0;
	else
	{
		U32 * buffer = (U32 *) _buffer;
		*buffer = dwTargetID;
		netTargetID = dwTargetID;
		setAttackTarget(target);
		return sizeof(U32);
	}
}
//---------------------------------------------------------------------------
//
void Gunplat::putSyncTarget (void * _buffer, U32 bufferSize, bool bLateDelivery)
{
	CQASSERT(bufferSize == sizeof(U32));
	U32 * buffer = (U32 *) _buffer;

	netTargetID = dwTargetID = *buffer;
	OBJLIST->FindObject(dwTargetID, playerID, target, IBaseObjectID);
	setAttackTarget(target);
}
//---------------------------------------------------------------------------
//
TRANSFORM Gunplat::GetShipTransform()
{
	CQBOMB0("Who's calling this?");
	return TRANSFORM();
}
//---------------------------------------------------------------------------
//
void Gunplat::initLaunchers (void)
{
	int i;
	IBaseObject * tmp;

	for (i = 0; i < MAX_GUNPLAT_LAUNCHERS; i++)
	{
		if (ginitData->launchers[i])
		{
			if ((tmp = ARCHLIST->CreateInstance(ginitData->launchers[i])) != 0)
			{
				if (tmp->QueryInterface(ILauncherID, launcher[i], NONSYSVOLATILEPTR))
					launcher[i]->InitLauncher(this, instanceIndex, ginitData->animArchetype, outerWeaponRange);
				else
				{
					delete tmp;
					break;
				}
			}
			else
				break;
		}
		else
			break;
	}

	if (ginitData->specialLauncher)
	{
		if ((tmp = ARCHLIST->CreateInstance(ginitData->specialLauncher)) != 0)
		{
			if (tmp->QueryInterface(ILauncherID, specialLauncher, NONSYSVOLATILEPTR))
				specialLauncher->InitLauncher(this, instanceIndex, ginitData->animArchetype, outerWeaponRange);
			else
			{
				delete tmp;
			}
		}
	}
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//
bool Gunplat::initGunplat (const GUNPLAT_INIT & data)
{
	outerWeaponRange   = data.pData->outerWeaponRange * GRIDSIZE;
	bNoLineOfSight = data.pData->bNoLineOfSight;

	ginitData = &data;
	unitStance = netUnitStance = US_DEFEND;
	fighterStance = netFighterStance = FS_NORMAL;

	return true;
}
//---------------------------------------------------------------------------
//
void Gunplat::upgradePlat (const GUNPLAT_INIT & data)
{
	outerWeaponRange   = data.pData->outerWeaponRange * GRIDSIZE;
	bNoLineOfSight = data.pData->bNoLineOfSight;

	ginitData = &data;

	destroyLaunchers();
	initLaunchers();
}
//---------------------------------------------------------------------------
//
void Gunplat::save (GUNPLAT_SAVELOAD & save)
{
	save.gunplatSaveload = *static_cast<BASE_GUNPLAT_SAVELOAD *>(this);
}
//---------------------------------------------------------------------------
//
void Gunplat::load (GUNPLAT_SAVELOAD & load)
{
 	*static_cast<BASE_GUNPLAT_SAVELOAD *>(this) = load.gunplatSaveload;
	netUnitStance = unitStance;
	netFighterStance = fighterStance;
	netTargetID = dwTargetID;
}
//---------------------------------------------------------------------------
//
void Gunplat::resolve (void)
{
	int i;
	OBJPTR<ISaveLoad> obj;

	if (dwTargetID)
		OBJLIST->FindObject(dwTargetID, playerID, target);

	for (i = 0; i < MAX_GUNPLAT_LAUNCHERS; i++)
		if (launcher[i].Ptr() && launcher[i]->QueryInterface(ISaveLoadID, obj))
			obj->ResolveAssociations();

	if (specialLauncher.Ptr() && specialLauncher->QueryInterface(ISaveLoadID, obj))
		obj->ResolveAssociations();

	if (target && attackAgentID)
		setAttackTarget(target);		// make sure launchers turn off
	else
		setAttackTarget(NULL);		// make sure launchers turn off

}
//---------------------------------------------------------------------------
//
void Gunplat::preTakeover(U32 newMissionID, U32 troopID)
{
	if (THEMATRIX->IsMaster())
	{
		if (attackAgentID)
		{
			// send the message indicated we have accomplished our task
			cancelAttack();
		}

		if (bUpgrading)
		{
			if(THEMATRIX->IsMaster() && bUpgrading)
			{
				upgradeID = 0;
				CancelUpgrade();
				bUpgrading = false;
				caps.attackOk = true;
				BANKER->FreeCommandPt(playerID,workingCost.commandPt);
				GunPlatCommand buffer;
				buffer.command = GUNP_C_UPGRADE_FORGET;
				THEMATRIX->SendOperationData(workingID,dwMissionID,&buffer,sizeof(buffer));
				COMP_OP(dwMissionID);
			}	
		}
	}

	int i;
	for (i = 0; i < MAX_GUNPLAT_LAUNCHERS; i++)
	{
		if (launcher[i].Ptr())
			launcher[i]->HandlePreTakeover(newMissionID, troopID);
	}

	if (specialLauncher.Ptr())
		specialLauncher->HandlePreTakeover(newMissionID, troopID);
}
//---------------------------------------------------------------------------
//
BOOL32 Gunplat::Save (struct IFileSystem * file)
{
	int i;
	OBJPTR<ISaveLoad> obj;

	Platform<GUNPLAT_SAVELOAD, GUNPLAT_INIT>::Save(file);

	char buffer[64];

	for (i = 0; i < MAX_GUNPLAT_LAUNCHERS; i++)
	{
		if (launcher[i].Ptr() && launcher[i]->QueryInterface(ISaveLoadID, obj))
		{
			sprintf(buffer, "Turret-%d", i);

			file->CreateDirectory(buffer);

			if (file->SetCurrentDirectory(buffer) == 0) 
				return 0;

			obj->Save(file);

			if (file->SetCurrentDirectory("..") == 0) 
				return 0;
		}
	}

	if (specialLauncher.Ptr() && specialLauncher->QueryInterface(ISaveLoadID, obj))
	{
		sprintf(buffer, "Special-Launcher");
		
		file->CreateDirectory(buffer);
		
		if (file->SetCurrentDirectory(buffer) == 0) 
			return 0;
		
		obj->Save(file);
		
		if (file->SetCurrentDirectory("..") == 0) 
			return 0;
	}

	return 1;
}
//---------------------------------------------------------------------------
//
BOOL32 Gunplat::Load (struct IFileSystem * file)
{
	int i;
	OBJPTR<ISaveLoad> obj;
	char buffer[64];

	Platform<GUNPLAT_SAVELOAD, GUNPLAT_INIT>::Load(file);

	initLaunchers();

	for (i = 0; i < MAX_GUNPLAT_LAUNCHERS; i++)
	{
		if (launcher[i].Ptr() && launcher[i]->QueryInterface(ISaveLoadID, obj))
		{
			sprintf(buffer, "Turret-%d", i);

			if (file->SetCurrentDirectory(buffer) == 0) 
				return 0;

		 	obj->Load(file);

			if (file->SetCurrentDirectory("..") == 0) 
				return 0;
		}
	}

	if (specialLauncher.Ptr() && specialLauncher->QueryInterface(ISaveLoadID, obj))
	{
		sprintf(buffer, "Special-Launcher");
		
		if (file->SetCurrentDirectory(buffer) == 0) 
			return 0;
		
		obj->Load(file);
		
		if (file->SetCurrentDirectory("..") == 0) 
			return 0;
	}

	return 1;
}
//---------------------------------------------------------------------------
// find closest target to us
//
void Gunplat::findAutoTarget (void)
{
	const U32 allyMask = MGlobals::GetAllyMask(dwMissionID & PLAYERID_MASK);

	ObjMapIterator it(systemID, transform.translation, GetWeaponRange(),playerID);

	IBaseObject *bestTarget = 0;
	MPart part;
	bool isPreferred = false;

	while (it)
	{
		if ((it->flags & OM_UNTOUCHABLE) == 0 && (it->flags & OM_TARGETABLE))
		{
			const U32 hisPlayerID = it.GetApparentPlayerID(allyMask);//it->dwMissionID & PLAYERID_MASK;
			
			// if we are enemies
			if (hisPlayerID && (((1 << (hisPlayerID-1)) & allyMask) == 0))
			{
				MPart part = it->obj;

				// don't shoot things that are building, or are already dead
				if (part.isValid() && part->bReady && part->hullPoints>0 && part->bNoAutoTarget==0)
				{
					// can we see the enemy, and is there a proper LOS to the enemy
					if (it->obj->IsVisibleToPlayer(dwMissionID & PLAYERID_MASK) && TestLOS(it->obj->GetGridPosition()))
					{
						// find the preferred target
						part = it->obj;
						
						if (part.isValid() && (MGlobals::IsObjectThreatening(part->mObjClass)))
						{
							isPreferred = true;
						}

						if (bestTarget==0 || isPreferred)
						{
							if (isPreferred)
							{
								// we've found our best target
								bestTarget = it->obj;
								break;
							}
							else
							{
								bestTarget = it->obj;
							}
						}
					}
				}
				isPreferred = false;
			}
		}
		++it;
	}

	if (bestTarget && bestTarget != target)
	{
		bestTarget->QueryInterface(IBaseObjectID, target, playerID);
		dwTargetID = bestTarget->GetPartID();
		loseTargetTimer = LOSE_TARGET_TIMER;

		// is our best target a gunboat or troopship?
		isPreferredTarget = isPreferred;
	}
}
//---------------------------------------------------------------------------
//
bool Gunplat::checkObjectInRange(U32 dwObjectID)
{
	IBaseObject *obj = OBJLIST->FindObject(dwObjectID);

	if (obj)
	{
		if (obj->GetSystemID() == systemID && obj->IsVisibleToPlayer(dwMissionID & PLAYERID_MASK))
		{
			Vector vec = obj->GetTransform().translation - transform.translation;
			SINGLE mag = vec.x * vec.x + vec.y * vec.y;

			if (mag < GetWeaponRange()*GetWeaponRange())
			{
				return true;
			}
		}
	}

	return false;
}
//---------------------------------------------------------------------------
//
void Gunplat::Render (void)
{
	int i;

	Platform<GUNPLAT_SAVELOAD, GUNPLAT_INIT>::Render();

	for (i = 0; i < MAX_GUNPLAT_LAUNCHERS; i++)
		if (launcher[i].Ptr())
			launcher[i].Ptr()->Render();

	if (specialLauncher.Ptr())
		specialLauncher.Ptr()->Render();

	if(DEFAULTS->GetDefaults()->bInfoHighlights && bHighlight)
	{
		if(GetWeaponRange())
		{
			drawRangeCircle(GetWeaponRange()/GRIDSIZE,RGB(255,0,0));
		}
	}
}
//---------------------------------------------------------------------------
//
void Gunplat::RevealFog (const U32 currentSystem)
{
	int i;

	Platform<GUNPLAT_SAVELOAD, GUNPLAT_INIT>::RevealFog(currentSystem);

	for (i = 0; i < MAX_GUNPLAT_LAUNCHERS; i++)
		if (launcher[i].Ptr())
			launcher[i].Ptr()->RevealFog(currentSystem);

	if (specialLauncher.Ptr())
		specialLauncher.Ptr()->RevealFog(currentSystem);
}
//---------------------------------------------------------------------------
//
void Gunplat::CastVisibleArea (void)
{
	int i;

	Platform<GUNPLAT_SAVELOAD, GUNPLAT_INIT>::CastVisibleArea();

	for (i = 0; i < MAX_GUNPLAT_LAUNCHERS; i++)
		if (launcher[i].Ptr())
			launcher[i].Ptr()->CastVisibleArea();

	if (specialLauncher.Ptr())
		specialLauncher.Ptr()->CastVisibleArea();
}
//---------------------------------------------------------------------------
//
void Gunplat::DrawHighlighted (void)
{
	Platform<GUNPLAT_SAVELOAD, GUNPLAT_INIT>::DrawHighlighted();

	for (U32 i = 0; i < MAX_GUNPLAT_LAUNCHERS; i++)
	{
		if (launcher[i].Ptr())
		{
			launcher[i].Ptr()->DrawHighlighted();
		}
	}
}
//---------------------------------------------------------------------------
//
static bool __fastcall isRelated (INSTANCE_INDEX parent, INSTANCE_INDEX child)
{
	while (child != parent)
	{
		if ((child = ENGINE->get_instance_parent(child)) == -1)
			return false;
	}

	return true;
}
//---------------------------------------------------------------------------
//
void Gunplat::OnChildDeath (INSTANCE_INDEX child)
{
	int i;

	for (i = 0; i < MAX_GUNPLAT_LAUNCHERS; i++)
		if (launcher[i].Ptr() && isRelated(child, launcher[i].Ptr()->GetObjectIndex()))
		{
			delete launcher[i].Ptr();
			launcher[i] = 0;
		}

	if (specialLauncher.Ptr() && isRelated(child, specialLauncher.Ptr()->GetObjectIndex()))
	{
		delete specialLauncher.Ptr();
		specialLauncher = 0;
	}
}
//---------------------------------------------------------------------------
//
ResourceCost Gunplat::GetAmountNeeded()
{
	return workingCost;
}	
//---------------------------------------------------------------------------
//
void Gunplat::UnstallSpender()
{
	if(bUpgrading && bDelayed)
	{
		bDelayed = false;
		BT_UPGRADE * buildType = (BT_UPGRADE *)(ARCHLIST->GetArchetypeData(upgradeID));
		StartUpgrade(buildType->extensionID,buildType->time*(1.0-MGlobals::GetAIBonus(playerID)));
		SetUpgradePercent(0.0);
		GunPlatCommand buffer;
		buffer.command = GUNP_C_UPGRADE_REMOVE_DELAY;

		workingID = THEMATRIX->CreateOperation(dwMissionID,&buffer,sizeof(buffer));
	}
}
//---------------------------------------------------------------------------
//
void Gunplat::OnOperationCreation (U32 agentID, void *buffer, U32 bufferSize)
{
	GunPlatCommand * buf = (GunPlatCommand *)buffer;
	switch(buf->command)
	{
	case GUNP_C_LAUNCHER_OP:
		{
			launcherAgentID = agentID;
			GunPlatCommandWHandle * com = (GunPlatCommandWHandle *) buf;
			launcherID = com->handle;
			launcher[launcherID]->LauncherOpCreated(agentID,&(((U8 *)buffer)[sizeof(GunPlatCommandWHandle)]),bufferSize-sizeof(GunPlatCommandWHandle));
			break;
		}
	case P_OP_QUEUE_FAIL:
		{
			GunPlatCommandWHandle * com = (GunPlatCommandWHandle *) buf;
			failSound((M_RESOURCE_TYPE)(com->handle));
			THEMATRIX->OperationCompleted(agentID,dwMissionID);
			break;
		}
	case GUNP_C_UPGRADE_START_DELAY:
		{
			GunPlatCommandWHandle *buf = (GunPlatCommandWHandle *) buffer;
				
			upgradeID = buf->handle;
			BT_UPGRADE * buildType = (BT_UPGRADE *)(ARCHLIST->GetArchetypeData(upgradeID));
			CQASSERT(buildType->objClass == OC_RESEARCH && buildType->type == RESEARCH_UPGRADE);

			workingCost = buildType->cost;
			workTime = buildType->time*(1.0-MGlobals::GetAIBonus(playerID));
			buildTimeSpent = 0;
			bUpgrading = true;

			bDelayed = true;

			break;
		}
	case GUNP_C_UPGRADE_START:
		{
			target = 0;
			dwTargetID = 0;
			setAttackTarget(NULL);

			GunPlatCommandWHandle *buf = (GunPlatCommandWHandle *) buffer;
				
			upgradeID = buf->handle;
			BT_UPGRADE * buildType = (BT_UPGRADE *)(ARCHLIST->GetArchetypeData(upgradeID));
			CQASSERT(buildType->objClass == OC_RESEARCH && buildType->type == RESEARCH_UPGRADE);

			workingCost = buildType->cost;
			workTime = buildType->time*(1.0-MGlobals::GetAIBonus(playerID));
			bUpgrading = true;

			bDelayed = false;

			StartUpgrade(buildType->extensionID,workTime);
			SetUpgradePercent(0.0);

			caps.attackOk = false;
			workingID = agentID;
			THEMATRIX->SetCancelState(workingID,false);
			break;
		}
	}
}
//---------------------------------------------------------------------------
//
void Gunplat::receiveOperationData (U32 agentID, void *buffer, U32 bufferSize)
{
	if(agentID == launcherAgentID)
	{
		U8 command = ((U8 *)buffer)[bufferSize-1];
		if(command == LOP_SEND_DATA)
		{
			launcher[launcherID]->LauncherReceiveOpData(agentID,buffer,bufferSize-1);
		}
		else if(command == LOP_END)
		{
			launcher[launcherID]->LauncherOpCompleted(agentID);
			THEMATRIX->OperationCompleted(agentID,dwMissionID);
			launcherAgentID = 0;
		}else if(command == LOP_CANCEL)
		{
			launcher[launcherID]->LauncherOpCompleted(agentID);
			THEMATRIX->OperationCompleted(agentID,dwMissionID);
			launcherAgentID = 0;
		}
	}
	if (agentID == attackAgentID)
	{			
		CQASSERT(bufferSize == 0);
		if (bSpecialAttack)
		{
			int i;
			for (i = 0; i < MAX_GUNBOAT_LAUNCHERS; i++)
			{
				if (launcher[i])
				{
					launcher[i]->InformOfCancel();
				}
			}
		}
		setAttackTarget(0);		// make sure launchers turn off
		bSpecialAttack = bUserGenerated = false;
		idleTimer = IDLE_TIMER;
		netTargetID = dwTargetID = 0;
		target = 0;
		extIdleTimer = EXT_IDLE_TIMER;

		THEMATRIX->OperationCompleted2(attackAgentID, dwMissionID);
	}
	if(workingID != agentID)
		return;
	GunPlatCommand * buf = (GunPlatCommand *)buffer;
	switch(buf->command)
	{
	case P_OP_QUEUE_FAIL:
		{
			GunPlatCommandWHandle * com = (GunPlatCommandWHandle *) buf;
			failSound((M_RESOURCE_TYPE)(com->handle));
			break;
		}
	case GUNP_C_UPGRADE_FORGET:
		{
			upgradeID = 0;
			CancelUpgrade();
			bUpgrading = false;
			caps.attackOk = true;
			COMP_OP(dwMissionID);
			break;
		}
	case GUNP_C_UPGRADE_FINISH:
		{
			BT_UPGRADE * buildType = (BT_UPGRADE *)(ARCHLIST->GetArchetypeData(upgradeID));
			COMM_MUTATION_COMPLETED(((char *)(buildType->resFinishedSound)),buildType->resFinishSubtitle,pInitData->displayName);

			SetUpgrade(buildType->extensionID);
			bUpgrading = false;
			caps.attackOk = true;
			upgradeID =0;

			COMP_OP(dwMissionID);
			break;
		}
	case GUNP_C_UPGRADE_REMOVE_DELAY:
		{
			bDelayed = false;
			BT_UPGRADE * buildType = (BT_UPGRADE *)(ARCHLIST->GetArchetypeData(upgradeID));
			StartUpgrade(buildType->extensionID,buildType->time*(1.0-MGlobals::GetAIBonus(playerID)));
			SetUpgradePercent(0.0);
			break;
		}
	}
}
//---------------------------------------------------------------------------
//
void Gunplat::onOperationCancel (U32 agentID)
{
	if (agentID == attackAgentID)
	{
		attackAgentID = 0;
	}
	// gunplats need to stop attacking on any cancel
	bUserGenerated = false;
	target = 0;
	netTargetID = dwTargetID = 0;
	setAttackTarget(0);
}	
//---------------------------------------------------------------------------
//
void Gunplat::OnStopRequest (U32 agentID)
{
	if (bUpgrading && workingID)
	{
		upgradeID = 0;
		CancelUpgrade();
		bUpgrading = false;
		caps.attackOk = true;
		BANKER->AddResource(playerID,workingCost);
		BANKER->FreeCommandPt(playerID,workingCost.commandPt);
		GunPlatCommand buffer;
		buffer.command = GUNP_C_UPGRADE_FORGET;
		THEMATRIX->SendOperationData(workingID,dwMissionID,&buffer,sizeof(buffer));
		COMP_OP(dwMissionID);
	}
}
//---------------------------------------------------------------------------
//
void Gunplat::OnMasterChange (bool bIsMaster)
{
	Platform<GUNPLAT_SAVELOAD, GUNPLAT_INIT>::OnMasterChange(bIsMaster);
}

#define NetworkFailSound(RESTYPE) \
	{\
		GunPlatCommandWHandle buffer;\
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
void Gunplat::BuildQueue (U32 dwArchetypeDataID, COMMANDS command)
{
	switch(command)
	{
	case COMMANDS::ADDIFEMPTY:
			break;
		// fall through intentional
	case COMMANDS::ADD:
		{
			if(HOST_CHECK && !bUpgrading)
			{
				cancelAttack();

				upgradeID = dwArchetypeDataID;
				BT_UPGRADE * buildType = (BT_UPGRADE *)(ARCHLIST->GetArchetypeData(upgradeID));
				CQASSERT(buildType->objClass == OC_RESEARCH && buildType->type == RESEARCH_UPGRADE);

				workingCost = buildType->cost;
				workTime = buildType->time*(1.0-MGlobals::GetAIBonus(playerID));
				buildTimeSpent = 0;

				M_RESOURCE_TYPE failType;
				if(BANKER->SpendMoney(playerID,workingCost,&failType))
				{
					if(BANKER->SpendCommandPoints(playerID,workingCost))
					{
						bUpgrading = true;
						bDelayed = false;
						caps.attackOk = false;
						StartUpgrade(buildType->extensionID,workTime);
						SetUpgradePercent(0.0);

						GunPlatCommandWHandle buffer;
						buffer.command = GUNP_C_UPGRADE_START;
						buffer.handle = upgradeID;

						workingID = THEMATRIX->CreateOperation(dwMissionID,&buffer,sizeof(buffer));
						THEMATRIX->SetCancelState(workingID,false);
					}
					else
					{
						upgradeID = 0;
						BANKER->AddResource(playerID,workingCost);
						NetworkFailSound(M_COMMANDPTS);
					}
				}
				else
				{
					upgradeID = 0;
					NetworkFailSound(failType);
				}
			}
			else
			{
				CQTRACE10("OldStylePlatform Add thrown away");
			}
			break;
		}
	case COMMANDS::REMOVE:
		{
			if(bUpgrading && workingID)
			{
				if(dwArchetypeDataID==0)
				{
					upgradeID = 0;
					CancelUpgrade();
					bUpgrading = false;
					caps.attackOk = true;

					BANKER->AddResource(playerID,workingCost);
					BANKER->FreeCommandPt(playerID,workingCost.commandPt);

					GunPlatCommand buffer;
					buffer.command = GUNP_C_UPGRADE_FORGET;
					THEMATRIX->SendOperationData(workingID,dwMissionID,&buffer,sizeof(buffer));

					COMP_OP(dwMissionID);
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
U32 Gunplat::GetNumInQueue (U32 value)
{
	return 0;
}
//---------------------------------------------------------------------------
//
bool Gunplat::IsInQueue (U32 value)
{
	if(bUpgrading)
		return true;
	return false;
}
//---------------------------------------------------------------------------
//
bool Gunplat::IsUpgradeInQueue ()
{
	return bUpgrading;
}
//---------------------------------------------------------------------------
//
U32 Gunplat::GetQueue(U32 * queueCopy,U32 * slotIDs)
{
	if(bUpgrading)
	{
		queueCopy[0] = upgradeID;
		if(slotIDs)
			slotIDs[0] = 0;
		return 1;
	}
	return 0;
}
//---------------------------------------------------------------------------
//
SINGLE Gunplat::FabGetProgress(U32 & stallType)
{
	if(!bUpgrading)
		return 0;
	if(bDelayed)
		stallType = IActiveButton::NO_MONEY;
	else
		stallType = IActiveButton::NO_STALL;
	return (buildTimeSpent/ ((SINGLE)(workTime)));
}
//---------------------------------------------------------------------------
//
SINGLE Gunplat::FabGetDisplayProgress(U32 & stallType)
{
	if(!bUpgrading)
		return 0;
	if(bDelayed)
		stallType = IActiveButton::NO_MONEY;
	else
		stallType = IActiveButton::NO_STALL;
	return (buildTimeSpent/ ((SINGLE)(workTime)));
}
//---------------------------------------------------------------------------
//
U32 Gunplat::GetFabJobID ()
{
	return upgradeID;
}
//---------------------------------------------------------------------------
//
void Gunplat::Attack (IBaseObject * victim, U32 agentID, bool _bUserGenerated)
{
//	CQASSERT((!bPlatDead) && "Tried to order a Dead plat to attack");
	if(bPlatDead)
	{
		THEMATRIX->OperationCompleted(agentID,dwMissionID);
		return;
	}
	bRecallFighters = false;
	bUserGenerated = _bUserGenerated;

	if (victim)
	{
		dwTargetID = victim->GetPartID();

		if (bUserGenerated)
		{
			const U32 hisID = victim->GetPlayerID();
			
			// we are purposely attacking an ally
			if (playerID != hisID && MGlobals::AreAllies(playerID, hisID))
			{
				MPart part = victim;
				COMM_ALLIED_ATTACK(victim, victim->GetPartID(), SUB_ALLIED_ATTACK, part.pInit->displayName);
			}
		}
	}
	else
	{
		dwTargetID = 0;
	}

	if (victim)
		victim->QueryInterface(IBaseObjectID, target, playerID);
	else
		target = 0;

	attackAgentID = agentID;

	// is the object we are attacking a gunboat?
	if (victim)
	{
		MPart part;
		part = victim;
		isPreferredTarget = MGlobals::IsObjectThreatening(part->mObjClass);
	}

	setAttackTarget(target);
	netTargetID = dwTargetID;		// already sync'ed up
	loseTargetTimer = LOSE_TARGET_TIMER;
	idleTimer = IDLE_TIMER;
}
//---------------------------------------------------------------------------
//
void Gunplat::AttackPosition(const struct GRIDVECTOR & position, U32 agentID)
{
	CQASSERT((!bPlatDead) && "Tried to order a Dead plat to attack");
	if(bPlatDead)
	{
		THEMATRIX->OperationCompleted(agentID,dwMissionID);
		return;
	}
	bRecallFighters = false;
	bSpecialAttack = true;

	int i;
	GRIDVECTOR pos = position;
	for (i = 0; i < MAX_GUNPLAT_LAUNCHERS; i++)
	{
		if (launcher[i])
		{
			launcher[i]->AttackPosition(&pos,false);
		}
	}
}
//---------------------------------------------------------------------------
//
void Gunplat::CancelAttack (void)
{
	CQASSERT(attackAgentID==0);
	setAttackTarget(NULL);
	target = 0;
	dwTargetID = netTargetID = 0;
}
//---------------------------------------------------------------------------
//
void Gunplat::setAttackTarget (IBaseObject * victim)
{
	int i;

	bRecallFighters = false;

	for (i = 0; i < MAX_GUNPLAT_LAUNCHERS; i++)
		if (launcher[i])
			launcher[i]->AttackObject(victim);
}
//---------------------------------------------------------------------------
//
void Gunplat::ReportKill (U32 partID)
{
	if (partID == dwTargetID)
	{
		target = 0;
	}

	IBaseObject * obj = OBJLIST->FindObject(partID);
	if (obj)
	{
		if (obj->objClass == OC_SPACESHIP)
			MGlobals::SetUnitsDestroyed(playerID, MGlobals::GetUnitsDestroyed(playerID)+1);
		else
		if (obj->objClass == OC_PLATFORM)
			MGlobals::SetPlatformsDestroyed(playerID, MGlobals::GetPlatformsDestroyed(playerID)+1);
		else
			CQERROR1("Unknown victim class: 0x%X", obj->objClass);
	}

	numKills++;
	techLevel.experience = static_cast<MISSION_SAVELOAD::InstanceTechLevel::UPGRADE>(MGlobals::GetIndExperienceLevel(numKills));
}
//---------------------------------------------------------------------------
//
void Gunplat::SpecialAttack (IBaseObject * victim, U32 agentID)
{
	CQASSERT((!bPlatDead) && "Tried to order a Dead plat to attack");
	if(bPlatDead)
	{
		THEMATRIX->OperationCompleted(agentID,dwMissionID);
		return;
	}
	CQASSERT(attackAgentID==0);

	attackAgentID = agentID;
	bRecallFighters = false;
	bUserGenerated = true;
	bSpecialAttack = true;

	for (int i = 0; i < MAX_GUNBOAT_LAUNCHERS; i++)
	{
		if (launcher[i])
		{
			launcher[i]->SpecialAttackObject(victim);
		}
	}

	if (victim)
	{
		netTargetID = dwTargetID = victim->GetPartID();
	}
	else
	{
		dwTargetID = 0;
	}

	if (victim)
	{
		victim->QueryInterface(IBaseObjectID, target, playerID);
	}
	else
	{
		target = 0;
	}
}
//---------------------------------------------------------------------------
//
void Gunplat::SpecialAOEAttack (const struct GRIDVECTOR & position, U32 agentID)
{
	CQASSERT((!bPlatDead) && "Tried to order a Dead plat to attack");
	if(bPlatDead)
	{
		THEMATRIX->OperationCompleted(agentID,dwMissionID);
		return;
	}
	bRecallFighters = false;
	bSpecialAttack = true;

	int i;
	GRIDVECTOR pos = position;
	for (i = 0; i < MAX_GUNPLAT_LAUNCHERS; i++)
	{
		if (launcher[i])
		{
			launcher[i]->AttackPosition(&pos,true);
		}
	}
}
//---------------------------------------------------------------------------
//
void Gunplat::WormAttack (IBaseObject * victim, U32 agentID)
{
}
//---------------------------------------------------------------------------
//
void Gunplat::DoSpecialAbility (U32 specialID)
{
	int i;
	for (i = 0; i < MAX_GUNBOAT_LAUNCHERS; i++)
	{
		if (launcher[i])
		{
			launcher[i]->DoSpecialAbility(specialID);
		}
	}
}
//---------------------------------------------------------------------------
//
void Gunplat::MultiSystemAttack (struct GRIDVECTOR & position, U32 targSystemID, U32 agentID)
{
	CQASSERT((!bPlatDead) && "Tried to order a Dead plat to attack");
	if(bPlatDead)
	{
		THEMATRIX->OperationCompleted(agentID,dwMissionID);
		return;
	}
	bRecallFighters = false;
	SetUnitStance(US_STOP);
	int i;
	GRIDVECTOR pos = position;
	for (i = 0; i < MAX_GUNPLAT_LAUNCHERS; i++)
	{
		if (launcher[i])
		{
			launcher[i]->AttackMultiSystem(&pos,targSystemID);
		}
	}
	THEMATRIX->OperationCompleted2(agentID,dwMissionID);
}
//---------------------------------------------------------------------------
//
void Gunplat::DoCreateWormhole(U32 systemID, U32 agentID)
{
	THEMATRIX->OperationCompleted(agentID,dwMissionID);
	CQASSERT((!bPlatDead) && "Tried to order a Dead plat to attack");
	if(bPlatDead)
	{
		return;
	}
	bRecallFighters = false;
	SetUnitStance(US_STOP);
	int i;
	for (i = 0; i < MAX_GUNPLAT_LAUNCHERS; i++)
	{
		if (launcher[i])
		{
			launcher[i]->DoCreateWormhole(systemID);
		}
	}
}
//---------------------------------------------------------------------------
//
bool Gunplat::UseSupplies (U32 amount,bool bAbsolute)
{
	if(fieldFlags.suppliesLocked())
		return false;
	USER_DEFAULTS * const defaults = DEFAULTS->GetDefaults();
	if (defaults->bCheatsEnabled && defaults->bNoSupplies)
		return true;

	if(bAbsolute)
	{
		if(supplies >= amount)
		{
			if (THEMATRIX->IsMaster())
			{
				if (supplies <= S32(amount))
				{
					supplies = 0;
				}
				else
					supplies -= amount;
			}
			return true;
		}
	}
	else if (supplies > 0)
	{
		if (THEMATRIX->IsMaster())
		{
			if (supplies <= S32(amount))
			{
				supplies = 0;
			}
			else
				supplies -= amount;
		}

		return true;
	}

	return false;
}
//---------------------------------------------------------------------------
//
bool Gunplat::TestLOS (const struct GRIDVECTOR & pos)
{
	TCallback callback(buildPlanetID);
	COMPTR<ITerrainMap> map;
	bool bResult;

	SECTOR->GetTerrainMap(GetSystemID(), map);
	callback.buildPlanetID = buildPlanetID;

	if ((bResult = map->TestSegment(GetGridPosition(), pos, &callback)) == false)
	{
		bResult = callback.gridPos.isMostlyEqual(pos);
	}

	return bResult;
}
//---------------------------------------------------------------------------
// return TRUE if we have a clear LineOfSight with the target
//
bool Gunplat::checkLOS (TCallback & callback)	// with targetPos
{
	if (target.Ptr() == NULL)
		return FALSE;

	COMPTR<ITerrainMap> map;
	SECTOR->GetTerrainMap(GetSystemID(), map);

	if (map->TestSegment(GetGridPosition(), target.Ptr()->GetGridPosition(), &callback) == false)
	{
		return callback.gridPos.isMostlyEqual(target.Ptr()->GetGridPosition());
	}

	return true;
}
//---------------------------------------------------------------------------
//
void Gunplat::LauncherCancelAttack()
{
}
//---------------------------------------------------------------------------
//
void Gunplat::GotoLauncherPosition(GRIDVECTOR pos)
{
}
//---------------------------------------------------------------------------
//
void Gunplat::LauncherSendOpData(U32 agentID, void * buffer,U32 bufferSize)
{
	CQASSERT(agentID == launcherAgentID);
	((U8 *)buffer)[bufferSize] = LOP_SEND_DATA;
	++bufferSize;
	THEMATRIX->SendOperationData(agentID,dwMissionID,buffer,bufferSize);
}
//---------------------------------------------------------------------------
//
void Gunplat::LaunchOpCompleted(ILauncher * launcher,U32 agentID)
{
	CQASSERT(agentID == launcherAgentID);
	U8 buffer = LOP_END;
	U32 bufferSize = 1;
	THEMATRIX->SendOperationData(agentID,dwMissionID,&buffer,bufferSize);
	THEMATRIX->OperationCompleted(agentID,dwMissionID);
	launcherAgentID = 0;
}
//---------------------------------------------------------------------------
//
U32 Gunplat::CreateLauncherOp(ILauncher * _launcher,struct ObjSet & set,void * buffer,U32 bufferSize)
{
	for(U32 i = 0; i < MAX_GUNBOAT_LAUNCHERS; ++ i)
	{
		if(launcher[i] == _launcher)
		{
			launcherID = i;
			U8 newBuffer[256];
			memcpy(&(((U8 *)newBuffer)[sizeof(GunPlatCommandWHandle)]),buffer,bufferSize);
			GunPlatCommandWHandle * myCom = (GunPlatCommandWHandle *)newBuffer;
			myCom->command = GUNP_C_LAUNCHER_OP;
			myCom->handle = launcherID;
			launcherAgentID = THEMATRIX->CreateOperation(set,&newBuffer,bufferSize+sizeof(GunPlatCommandWHandle));
			return launcherAgentID;
		}
	}
	CQASSERT(0 && "CreateLauncherOp failed");
	return 0;
}
//---------------------------------------------------------------------------
//
SINGLE Gunplat::GetWeaponRange()
{
	SINGLE fighterRangeMod = 1.0;
	if(mObjClass == M_SPACESTATION ||
		mObjClass == M_PLASMAHIVE )
	{
		if(race == M_TERRAN)
		{
			fighterRangeMod = 1.0+0.1*((techLevel.classSpecific/3.0)-0.5);
		}
		if(race == M_MANTIS)
		{
			fighterRangeMod = 1.0+0.1*((techLevel.classSpecific/5.0)-0.5);
		}
	}
	return outerWeaponRange*fighterRangeMod;
}
//---------------------------------------------------------------------------
//
SINGLE Gunplat::GetOptimalWeaponRange (void)
{
	return outerWeaponRange;
}
//---------------------------------------------------------------------------
//
IBaseObject * Gunplat::FindChildTarget(U32 childID)
{
	for(U32 i = 0; i < MAX_GUNBOAT_LAUNCHERS; ++ i)
	{
		if(launcher[i])
		{
			IBaseObject * child = launcher[i]->FindChildTarget(childID);
			if(child)
				return child;
		}
	}
	return NULL;
}
//---------------------------------------------------------------------------
//
void Gunplat::EnableCloak (bool bEnable)
{
}
//---------------------------------------------------------------------------
//
inline struct IBaseObject * createPlatform (const GUNPLAT_INIT & data)
{
	Gunplat * obj = new ObjectImpl<Gunplat>;

	obj->FRAME_init(data);
	if (obj->initGunplat(data) == false)
	{
		delete obj;
		obj = 0;
	}

	return obj;
}
//------------------------------------------------------------------------------------------
//
GUNPLAT_INIT::~GUNPLAT_INIT (void)					// free archetype references
{
	// nothing yet!?
}
//------------------------------------------------------------------------------------------
//---------------------------Gunplat Factory----------------------------------------------
//------------------------------------------------------------------------------------------
//
struct DACOM_NO_VTABLE GunplatFactory : public IObjectFactory
{
	struct OBJTYPE : GUNPLAT_INIT
	{
		~OBJTYPE (void)
		{
			int i;

			for (i = 0; i < MAX_GUNPLAT_LAUNCHERS; i++)
				if (launchers[i])
					ARCHLIST->Release(launchers[i], OBJREFNAME);
			
			if (specialLauncher)
				ARCHLIST->Release(specialLauncher, OBJREFNAME);
		}
	};
	//------------------------------------------------------------------------------------------
	//
	U32 factoryHandle;		// handles to callback

	//
	// Interface mapping
	//

	BEGIN_DACOM_MAP_INBOUND(GunplatFactory)
	DACOM_INTERFACE_ENTRY(IObjectFactory)
	END_DACOM_MAP()

	GunplatFactory (void) { }

	~GunplatFactory (void);

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
GunplatFactory::~GunplatFactory (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (OBJLIST && OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
		connection->Unadvise(factoryHandle);
}
//--------------------------------------------------------------------------//
//
void GunplatFactory::init (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
		connection->Advise(getBase(), &factoryHandle);
}
//-----------------------------------------------------------------------------
//
HANDLE GunplatFactory::CreateArchetype (const char *szArchname, OBJCLASS objClass, void *_data)
{
	OBJTYPE * result = 0;
	int i;

	if (objClass == OC_PLATFORM)
	{
		BT_PLAT_GUN * data = (BT_PLAT_GUN *) _data;
		
		if (data->type == PC_GUNPLAT)
		{
			result = new OBJTYPE();

			if (result->loadPlatformArchetype(data, ARCHLIST->GetArchetype(szArchname)) == false)
				goto Error;

			for (i = 0; i < MAX_GUNPLAT_LAUNCHERS; i++)
			{
				if (data->launcherType[i][0])
				{
					if ((result->launchers[i] = ARCHLIST->LoadArchetype(data->launcherType[i])) == 0)
						break;
					else
						ARCHLIST->AddRef(result->launchers[i], OBJREFNAME);
				}
				else
					break;
			}

			if (data->specialLauncherType[0])
			{
				if ((result->specialLauncher = ARCHLIST->LoadArchetype(data->specialLauncherType)) != 0)
					ARCHLIST->AddRef(result->specialLauncher, OBJREFNAME);
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
BOOL32 GunplatFactory::DestroyArchetype (HANDLE hArchetype)
{
//	GUNPLAT_INIT * objtype = (GUNPLAT_INIT *)hArchetype;
	OBJTYPE * objtype = (OBJTYPE *)hArchetype;

	EditorStopObjectInsertion(objtype->pArchetype);
	delete objtype;

	return 1;
}
//-------------------------------------------------------------------
//
IBaseObject * GunplatFactory::CreateInstance (HANDLE hArchetype)
{
//	GUNPLAT_INIT * objtype = (GUNPLAT_INIT *)hArchetype;
	OBJTYPE * objtype = (OBJTYPE *)hArchetype;

	return createPlatform(*objtype);
}
//-------------------------------------------------------------------
//
void GunplatFactory::EditorCreateInstance (HANDLE hArchetype, const MISSION_INFO & info)
{
//	GUNPLAT_INIT * objtype = (GUNPLAT_INIT *)hArchetype;
	OBJTYPE * objtype = (OBJTYPE *)hArchetype;
	EditorStartObjectInsertion(objtype->pArchetype, info);
}
//---------------------------------------------------------------------------
//-------------------------GLOBAL STARTUP------------------------------------
//---------------------------------------------------------------------------
//
struct _gunplatfactory : GlobalComponent
{
	GunplatFactory * sfactory;

	virtual void Startup (void)
	{
		sfactory = new DAComponent<GunplatFactory>;
		AddToGlobalCleanupList((IDAComponent **) &sfactory);
	}

	virtual void Initialize (void)
	{
		sfactory->init();
	}
};

static _gunplatfactory __plat;

//--------------------------------------------------------------------------//
//----------------------------End GunPlat.cpp-------------------------------//
//--------------------------------------------------------------------------//
