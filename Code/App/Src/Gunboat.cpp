//--------------------------------------------------------------------------//
//                                                                          //
//                               Gunboat.cpp                                //
//                                                                          //
//                  COPYRIGHT (C) 1997 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/Gunboat.cpp 350   9/13/01 10:01a Tmauer $
*/			    

//--------------------------------------------------------------------------//
//
/*
 * gotoPivot should only be valid when we have an escortAgentID or attackAgentID.
 * 		set when we have an agent, clear it when operation cancel.
 *	If we need to do a move, do it while we still have an agent.
 * 
 *	Use sync data to set/clear targets for the launchers. This should be the ONLY way to change the launcher's target,
 *      except during an Attack command...(make sure to set netTarget=target)
 *
 *  Stances must be completely independent of commands. Use sync data to transmit stance changes
 * 
 *  Stances: Attack - wide range, may move to attack
 *           Defend - medium range,	may move to attack
 *			 Stand  - gun range, do not move to attack
 *			 Peace  - do not auto target, do not move
 *     
 *  Targeting: 
 *		Function picks a target based on state, then sets a class variable. Implement as a sync method.
 *		Method sends new target to client, and to launchers.
 *		If an attack was a user command, or !bReady, or peace stance, or cloaked or mimic'ed, or being repaired; do not auto target. 
 *		Else if attackAgent!=0, choose a target according to stance.
 *      Else if (attackAgent==0 && hasPendingOp()) choose a target within gun range.
 *
 *  Attack Agent management
 *		If user command and extended timeout and target is out of range, complete attack. (Send Op Data)
 *		If no attack agent and idle for 3 second timeout, issue an attack command. (if target available)
 *		If agent and target==0 and idle for timeout, complete the attack (Send Op Data)
 *
 *  Escort Agent management
 *		Move to escort unit position (if too far away), complete operation when unit dies. (Send Op Data)
 *		If unit moves to another system, re-issue Escort command.
 * 
 *	Moving to attack
 *		If Attack or Defend stance and attackAgent!=0, set pivot to setGotoPivot().
 *
 *  moveSync() if gotoPivot, moveToPos(gotoPivot), send pivot across the network.
 *
 *  targetingSync()  if target != netTarget,  set target for launchers, send target to client
 *
 *  resetAttackVars()
 *    bUserCommand, gotoPivot, 
 *
 */

//------------------------------- #INCLUDES --------------------------------//
//--------------------------------------------------------------------------//

#include "pch.h"
#include <globals.h>

#include "TSpaceShip.h"
#include "TobjArtifact.h"
#include "ObjList.h"
#include "Camera.h"
#include "SuperTrans.h"
#include "Sector.h"
#include "UserDefaults.h"
#include "DSpaceship.h"
#include "DInstance.h"
#include "ILauncher.h"
#include <MGlobals.h>
#include "Startup.h"
#include "IAttack.h"
#include "MPart.h"
#include "CommPacket.h"
#include "IAdmiral.h"
#include "ObjMapIterator.h"
#include "Mission.h"

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

#define EXT_MOVE_IDLE_TIMER		U32(1+ 5 * DEF_REALTIME_FRAMERATE)
#define MOVE_IDLE_TIMER			U32(1+ 1 * DEF_REALTIME_FRAMERATE)
#define SUPPLIES_DISPLAY_PERIOD U32(1+ 2 * DEF_REALTIME_FRAMERATE)
#define LOSE_TARGET_TIMER		U32(1+ 5 * DEF_REALTIME_FRAMERATE)
#define EXT_IDLE_TIMER			U32(1+ 3 * DEF_REALTIME_FRAMERATE)
#define IDLE_TIMER				U32(1+ 3 * DEF_REALTIME_FRAMERATE)


#define LANCER_RANGE	3.0f * GRIDSIZE

static U32 GUNBOATTEXMEMUSED=0;

#define GUNBOAT_C_LAUNCHER_OP			6

#define LOP_SEND_DATA	1
#define LOP_END			2
#define LOP_CANCEL		3

struct GunBoatCommand
{
	U8 command;
};

struct GunBoatCommandWHandle
{
	U8 command;
	U32 handle;
};

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
struct _NO_VTABLE Gunboat : public ObjectArtifact< SpaceShip<GUNBOAT_SAVELOAD, GUNBOAT_INIT> >, IAttack, IToggle, ILaunchOwner, IFleetShip, BASE_GUNBOAT_SAVELOAD
{
	BEGIN_MAP_INBOUND(Gunboat)
	_INTERFACE_ENTRY(IBaseObject)
	_INTERFACE_ENTRY(IAttack)
	_INTERFACE_ENTRY(IGotoPos)
	_INTERFACE_ENTRY(ISaveLoad)
	_INTERFACE_ENTRY(IQuickSaveLoad)
	_INTERFACE_ENTRY(ILaunchOwner)
	_INTERFACE_ENTRY(IExplosionOwner)
	_INTERFACE_ENTRY(IMissionActor)
	_INTERFACE_ENTRY(IWeaponTarget)
	_INTERFACE_ENTRY(IBuild)
	_INTERFACE_ENTRY(IPhysicalObject)
	_INTERFACE_ENTRY(ICloak)
	_INTERFACE_ENTRY(IFleetShip)
	_INTERFACE_ENTRY(IRepairee)
	_INTERFACE_ENTRY(IShipDamage)
	_INTERFACE_ENTRY(IExtent)
	_INTERFACE_ENTRY(IShipMove)
	_INTERFACE_ENTRY(IToggle)
	_INTERFACE_ENTRY(IArtifactHolder)
	_INTERFACE_ENTRY(IEffectTarget)
	END_MAP()
	//----------------------------------

	//---------------------------------------------------------------------------
	//
	enum SPECIALTYPE
	{
		SPECIAL_NONE,
		SPECIAL_SELF,
		SPECIAL_TARGET,
		SPECIAL_CLOAK
	};
	struct SPECIALQUEUENODE
	{
		SPECIALTYPE type;
		U32			dwTargetID;
	};
#define MAX_SPECIAL_NODES 4
	struct SPECIALQUEUE
	{
		SPECIALQUEUENODE node[MAX_SPECIAL_NODES];
		int reader, writer;		// index to read / write to next

		SPECIALQUEUENODE * getNext (void)
		{
			SPECIALQUEUENODE * result = 0;

			if (isEmpty() == 0)
			{
				result = node+reader;
				reader = (reader + 1) % MAX_SPECIAL_NODES;
			}

			return result;
		}

		void addTo (SPECIALTYPE type, U32 dwTargetID=0)
		{
			if (isFull() == 0)
			{
				node[writer].type = type;
				node[writer].dwTargetID = dwTargetID;
				writer = (writer + 1) % MAX_SPECIAL_NODES;
			}
		}

		bool isEmpty (void) const
		{
			return reader == writer;
		}

		bool isFull (void) const 
		{
			return (((writer+1) % MAX_SPECIAL_NODES) == reader);		// would adding one make us empty?
		}
	} specialQueue;

	//---------------------------------------------------------------------------
	//
	struct TCallback : ITerrainSegCallback
	{
		GRIDVECTOR gridPos;
		U32 blockingMissionID;

		TCallback (void)
		{
			blockingMissionID = 0;
		}

		virtual bool TerrainCallback (const struct FootprintInfo & info, struct GRIDVECTOR & pos)
		{
			if (info.flags & TERRAIN_BLOCKLOS)
			{
				if (info.missionID && info.missionID == blockingMissionID)
				{
					// we've already checked out this dude, now we fail
					return false;
				}

				blockingMissionID = info.missionID;
				gridPos = pos;
			}
			return true;
		}
	};

	OBJPTR<ILauncher> launcher[MAX_GUNBOAT_LAUNCHERS];
	OBJPTR<IBaseObject> target;					// can now be enemy or friend
	OBJPTR<IBaseObject> escort;

	SINGLE optimalWeaponRange, optimalFacingAngle, outerWeaponRange;
	bool bNoLineOfSight;		// true means we don't need line of sight
	int loseTargetTimer;
	int idleTimer, extIdleTimer, moveIdleTimer;

	U32				netTargetID;
	UNIT_STANCE		netUnitStance;
	FighterStance	netFighterStance;
	GRIDVECTOR		gotoPivot, oldGotoPivot;
	GRIDVECTOR		targetPos;

	S32  suppliesDisplayDecay;

	//----------------------------------
	//----------------------------------

	PhysUpdateNode  physUpdateNode;
	UpdateNode		updateNode;
	UpdateNode		updateNode2;
	ExplodeNode		explodeNode;
	PreDestructNode	destructNode;
	SaveNode		saveNode;
	LoadNode		loadNode;
	ResolveNode		resolveNode;
	InitNode		initNode;
	OnOpCancelNode	onOpCancelNode;
	PreTakeoverNode preTakeoverNode;
	ReceiveOpDataNode receiveOpDataNode;
	GeneralSyncNode  genSyncNode1;
	GeneralSyncNode  genSyncNode2;
	GeneralSyncNode  genSyncNode3;
	GeneralSyncNode  genSyncNode4;
	GeneralSyncNode  genSyncNode5;

	const GUNBOAT_INIT * ginitData;

	//----------------------------------
	//----------------------------------
	
	Gunboat (void);

	virtual ~Gunboat (void);	

	/* IBaseObject methods */

	virtual void Render (void);

	virtual void RevealFog (const U32 currentSystem);

	virtual void CastVisibleArea (void);			

	virtual void DrawFleetMoniker (bool bAllShips);

	virtual void SetTerrainFootprint ( ITerrainMap *map);

	virtual void UpdateVisibilityFlags (void);
	
	/* IGotoPos methods */

	virtual void GotoPosition (const struct GRIDVECTOR & pos, U32 agentID, bool bSlowMove);

	virtual void UseJumpgate (IBaseObject * outgate, IBaseObject * ingate, const Vector& jumpToPosition, SINGLE heading, SINGLE speed, U32 agentID);

	/* IRepairee methods  */

	virtual void RepairYourselfAt (IBaseObject * platform, U32 agentID);

	virtual void RepairCompleted (void);

	/* ICloak */
	virtual bool CanCloak();

	/* IToggle */

	virtual bool IsToggle();

	virtual bool CanToggle();

	virtual bool IsOn();

	/* IArtifactHolder */

	virtual void UseArtifactOn(IBaseObject * target, U32 agentID);

	/* IAttack methods */

	virtual void Attack (IBaseObject * victim, U32 agentID, bool bUserGenerated);

	virtual void AttackPosition(const struct GRIDVECTOR & position, U32 agentID); 

	virtual void CancelAttack (void);

	virtual void ReportKill (U32 partID);

	virtual void SpecialAttack (IBaseObject * victim, U32 agentID);

	virtual void SpecialAOEAttack (const struct GRIDVECTOR & position, U32 agentID);

	virtual void WormAttack (IBaseObject * victim, U32 agentID);

	virtual void Escort (IBaseObject * target, U32 agentID);

	virtual void DoSpecialAbility (U32 specialID);

	virtual void DoSpecialAbility (IBaseObject *obj);

	virtual void DoCloak (void);

	virtual void MultiSystemAttack (struct GRIDVECTOR & position, U32 targSystemID, U32 agentID);

	virtual void DoCreateWormhole(U32 systemID, U32 agentID);

	virtual void SetUnitStance (const UNIT_STANCE stance);

	virtual const UNIT_STANCE GetUnitStance (void) const;

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

	virtual void  GetSpecialAbility (UNIT_SPECIAL_ABILITY & ability, bool & bSpecialEnabled);

	virtual void OnAllianceChange (U32 allyMask);

	virtual void GetTarget(IBaseObject* & targObj, U32 targID)
	{
		targObj = target;
		targID = dwTargetID;
	}

	/* IFleetShip methods */

	virtual void WaitForAdmiral (bool bWait);

	virtual bool IsAvailableForAdmiral (void);

	virtual void SetFleetshipTarget (IBaseObject * _target)
	{
		CQASSERT(fleetID);
		CQASSERT(THEMATRIX->IsMaster());
		loseTargetTimer = LOSE_TARGET_TIMER;
		moveIdleTimer = MOVE_IDLE_TIMER;
		if (_target != target)
		{
			if (_target)
			{
				dwTargetID = _target->GetPartID();
				_target->QueryInterface(IBaseObjectID, target, playerID);

				if(!isMoveActive())
					setgotoPivot();
			}
			else
			{
				dwTargetID = 0;
				target = 0;
			}
		}
	}

	virtual IBaseObject * GetFleetshipTarget();

	virtual bool IsInMove (void);

	/* IExplosionOwner methods */

	virtual void OnChildDeath (INSTANCE_INDEX child);

	/* ILaunchOwner */

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
	
	virtual SINGLE GetWeaponRange();

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

	virtual void COMAPI destroy_instance (INSTANCE_INDEX index)
	{
		CQASSERT(index == instanceIndex);
		//this code being called AFTER destructor???
	//	destroyLaunchers();
		instanceIndex = -1;
	}

	/* ISaveLoad methods */

	virtual BOOL32 Save (struct IFileSystem * file);

	virtual BOOL32 Load (struct IFileSystem * file);

	/* IMissionActor methods */

	virtual void InitActor (void);

	void onOperationCancel (U32 agentID);

	virtual void OnOperationCreation (U32 agentID, void *buffer, U32 bufferSize)
	{
		GunBoatCommand * buf = (GunBoatCommand *)buffer;
		switch(buf->command)
		{
			case GUNBOAT_C_LAUNCHER_OP:
				{
					launcherAgentID = agentID;
					GunBoatCommandWHandle * com = (GunBoatCommandWHandle *) buf;
					launcherID = com->handle;
					launcher[launcherID]->LauncherOpCreated(agentID,&(((U8 *)buffer)[sizeof(GunBoatCommandWHandle)]),bufferSize-sizeof(GunBoatCommandWHandle));
					break;
				}
		}
	}

	virtual void TakeoverSwitchID (U32 newMissionID);

	virtual void OnMasterChange(bool bIsMaster)
	{
		repairMasterChange(bIsMaster);
	}

	/* IWeaponTarget methods */
	virtual BOOL32 ApplyDamage (IBaseObject * collider, U32 ownerID, const Vector & pos, const Vector &dir,U32 amount,BOOL32 bShieldHit);

	// move "amount" from the pending pile to the actual. (assumes complex formula has already been used)
	virtual void ApplyAOEDamage (U32 ownerID, U32 damageAmount);

	/* SpaceShip methods */

	virtual void broadcastHelpMsg (U32 attackerID);
	
	virtual const char * getSaveStructName (void) const		// must be overriden implemented by derived class
	{
		return "GUNBOAT_SAVELOAD";
	}

	virtual void * getViewStruct (void) 	// must be overriden implemented by derived class
	{
		return static_cast<BASE_GUNBOAT_SAVELOAD *>(this);
	}

	virtual bool testReadyForJump (void)
	{
		int i;

		for (i = 0; i < MAX_GUNBOAT_LAUNCHERS; i++)
		{
			if (launcher[i].Ptr() && launcher[i]->TestFightersRetracted()==false)
				return false;
		}
		return true;
	}

	/* Gunboat methods */

	BOOL32 updateBoat (void);

	BOOL32 updateBoatClient (void);
	
	void physUpdateBoat (SINGLE dt);

	void explodeBoat (bool bExplode);

	void preSelfDestruct (void);

	void destroyLaunchers (void);

	bool setgotoPivot (void);

	bool setgotoPivotPosition(GRIDVECTOR & pos);

	void save (GUNBOAT_SAVELOAD & save);

	void load (GUNBOAT_SAVELOAD & load);

	void resolve (void);

	void preTakeover (U32 newMissionID, U32 troopID);

	bool checkLOS (TCallback & callback);

	void initGunboat (const GUNBOAT_INIT & data);

	void reportKillToAdmiral (void);

	void notifyAdmiralOfDamage (U32 attackerID);

	void notifyAdmiralOfDestruction (void);

	void initLaunchers (void);

	void findClosestTarget (const GRIDVECTOR & pivot, SINGLE fDist);

	void rotateToTarget (void);

	void rotateToTargetPos (void);

//	void findBetterTarget (void);

	void setAttackTarget (IBaseObject * victim);	// do not move to attack

	// sync methods
	U32 getSyncStance (void * buffer);
	void putSyncStance (void * buffer, U32 bufferSize, bool bLateDelivery);

	U32 getSyncMove (void * buffer);
	void putSyncMove (void * buffer, U32 bufferSize, bool bLateDelivery);

	U32 getSyncLaunchers (void * buffer);
	void putSyncLaunchers (void * buffer, U32 bufferSize, bool bLateDelivery);

	U32 getSyncSpecial (void * buffer);
	void putSyncSpecial (void * buffer, U32 bufferSize, bool bLateDelivery);

	U32 getSyncTarget (void * buffer);
	void putSyncTarget (void * buffer, U32 bufferSize, bool bLateDelivery);

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
			if(artifact && bArtifactUse)
				artifact->SetTarget(NULL);

			setAttackTarget(0);		// make sure launchers turn off
			THEMATRIX->SendOperationData(attackAgentID, dwMissionID, NULL,0);
			THEMATRIX->OperationCompleted2(attackAgentID, dwMissionID);
			bArtifactUse = bSpecialAttack = bUserGenerated = false;
			idleTimer = IDLE_TIMER;
			netTargetID = dwTargetID = 0;
			target = 0;
			gotoPivot.zero();
			oldGotoPivot.zero();
			extIdleTimer = EXT_IDLE_TIMER;
		}
	}

	void cancelEscort (void)
	{
		CQASSERT(THEMATRIX->IsMaster());
		if (escortAgentID)
		{
			// send the message indicated we have accomplished our task
			THEMATRIX->SendOperationData(escortAgentID, dwMissionID, NULL,0);
			THEMATRIX->OperationCompleted2(escortAgentID, dwMissionID);
			idleTimer = IDLE_TIMER;
			gotoPivot.zero();
			oldGotoPivot.zero();
			extIdleTimer = EXT_IDLE_TIMER;
			dwEscortID = 0;
			escort = 0;
		}
	}

	// called in response to OpAgent::SendOperationData()
	void receiveOperationData (U32 agentID, void *buffer, U32 bufferSize)
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
			gotoPivot.zero();
			oldGotoPivot.zero();
			extIdleTimer = EXT_IDLE_TIMER;

			THEMATRIX->OperationCompleted2(attackAgentID, dwMissionID);
		}
		else if (agentID == escortAgentID)
		{
			CQASSERT(bufferSize == 0);
			idleTimer = IDLE_TIMER;
			gotoPivot.zero();
			oldGotoPivot.zero();
			extIdleTimer = EXT_IDLE_TIMER;
			dwEscortID = 0;
			escort = 0;
			
			THEMATRIX->OperationCompleted2(escortAgentID, dwMissionID);
		}
	}

	bool notifyAdmiralOfTargetLoss (void);

	// if a jump is blocked, bRecallFighters can be true, but there will be no pending op
	bool canAutoTarget (void) const
	{
		return (bReady && unitStance != US_STOP && bCloaked==0 && bMimic==0 && bRepairUnderway==0 && (bRecallFighters==0 || THEMATRIX->HasPendingOp(const_cast<Gunboat *>(this))==0) &&
			(fleetID==0||MISSION->IsComputerControlled(playerID)) && caps.attackOk && (DEFAULTS->GetDefaults()->bNoAutoTarget==0||DEFAULTS->GetDefaults()->bCheatsEnabled==0));
	}
};

//---------------------------------------------------------------------------
//
Gunboat::Gunboat (void) : 
			updateNode(this, UpdateProc(&Gunboat::updateBoat)),
			updateNode2(this, UpdateProc(&Gunboat::updateBoatClient)),
			explodeNode(this, ExplodeProc(&Gunboat::explodeBoat)),
			saveNode(this, CASTSAVELOADPROC(&Gunboat::save)),
			loadNode(this, CASTSAVELOADPROC(&Gunboat::load)),
			resolveNode(this, ResolveProc(&Gunboat::resolve)),
			initNode(this, CASTINITPROC(&Gunboat::initGunboat)),
			physUpdateNode(this, PhysUpdateProc(&Gunboat::physUpdateBoat)),
			onOpCancelNode(this, OnOpCancelProc(&Gunboat::onOperationCancel)),
			preTakeoverNode(this, PreTakeoverProc(&Gunboat::preTakeover)),
			receiveOpDataNode(this, ReceiveOpDataProc(&Gunboat::receiveOperationData)),
			destructNode(this, PreDestructProc(&Gunboat::preSelfDestruct)),
			genSyncNode1(this, SyncGetProc(&Gunboat::getSyncStance), SyncPutProc(&Gunboat::putSyncStance)),
			genSyncNode2(this, SyncGetProc(&Gunboat::getSyncMove), SyncPutProc(&Gunboat::putSyncMove)),
			genSyncNode3(this, SyncGetProc(&Gunboat::getSyncTarget), SyncPutProc(&Gunboat::putSyncTarget)),
			genSyncNode4(this, SyncGetProc(&Gunboat::getSyncSpecial), SyncPutProc(&Gunboat::putSyncSpecial)),
			genSyncNode5(this, SyncGetProc(&Gunboat::getSyncLaunchers), SyncPutProc(&Gunboat::putSyncLaunchers))

{
	idleTimer = IDLE_TIMER;
	loseTargetTimer = LOSE_TARGET_TIMER;
	extIdleTimer = EXT_IDLE_TIMER;
	moveIdleTimer = MOVE_IDLE_TIMER;
}
//---------------------------------------------------------------------------
//
Gunboat::~Gunboat (void)
{
	destroyLaunchers();
}
//---------------------------------------------------------------------------
//
static bool bUpdateBoat = true;
BOOL32 Gunboat::updateBoat (void)
{
	if(!bUpdateBoat)
		return 1;
	if (bExploding)
		return 1;
	if (THEMATRIX->IsMaster() == 0)
		return 1;

	//
	// do the targeting code...
	//

	if (bUserGenerated)
	{
		CQASSERT(attackAgentID);

		if (bSpecialAttack==0 && bArtifactUse==0)		// don't cancel attacks for special attacks
		{
			MPart part = target;
			if (part.isValid()==0 || part->hullPoints==0)
				cancelAttack();
			else
			{
				if (target->IsVisibleToPlayer(playerID))
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
	if (canAutoTarget())
	{
		if (--idleTimer < 0)
		{
			idleTimer = IDLE_TIMER;

			if (attackAgentID!=0)
			{
				if (unitStance == US_ATTACK)
				{
/*					if (target!=0 && target->GetSystemID() == systemID)
						findBetterTarget();
					else
*/
					findClosestTarget(GetGridPosition(), sensorRadius);
				}
				else
				if (unitStance == US_DEFEND)
				{
					if (bDefendPivotValid)
						findClosestTarget(defendPivot, sensorRadius);
					else
						findClosestTarget(GetGridPosition(), sensorRadius);
				}
				else
				{
					findClosestTarget(GetGridPosition(), (GetWeaponRange()/GRIDSIZE));
				}
			}
			else
			if (escortAgentID!=0 || THEMATRIX->HasPendingOp(this))	// working on something else
			{
				findClosestTarget(GetGridPosition(), (GetWeaponRange()/GRIDSIZE));
			}
			// NOTE: Do not do any targeting if we have no agent!
		}
	}
	else  // not allowed to target, and have no agent
	if (attackAgentID==0)
	{
		// officially stop attacking if we loose sight of the target
		if (target && target->IsVisibleToPlayer(playerID)==0)
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
				if (--loseTargetTimer < 0 || fleetID)		// don't cancel attack immediately (unless part of a fleet)
				{
					if (fleetID==0 || notifyAdmiralOfTargetLoss()==0)
						cancelAttack();
				}
			}
			else
			{
				if (target->IsVisibleToPlayer(playerID))
					loseTargetTimer = LOSE_TARGET_TIMER;
				else
				{
					if (--loseTargetTimer < 0)
					{
						if (fleetID==0 || notifyAdmiralOfTargetLoss()==0)
							cancelAttack();
					}
				}
			}
		}
	}
	else
	if (escortAgentID!=0)
	{
		if (escort==0)
			cancelEscort();
		else
		if ((escort->GetSystemID() & ~HYPER_SYSTEM_MASK) != systemID)
		{
			U32 dwOldEscortID = dwEscortID;

			cancelEscort();			// cancel current command

			//
			// re-issue escort command
			//
			if (dwOldEscortID)
			{
				USR_PACKET<USRESCORT> packet;
				packet.objectID[0] = dwMissionID;
				packet.targetID = dwOldEscortID;
				packet.init(1);
				NETPACKET->Send(HOSTID, 0, &packet);
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

			if (unitStance == US_ATTACK)
			{
				findClosestTarget(GetGridPosition(), sensorRadius);
			}
			else
			if (unitStance == US_DEFEND)
			{
				if (bDefendPivotValid)
					findClosestTarget(defendPivot, sensorRadius);
				if (target==0)	// we didn't find anything at the pivot point
					findClosestTarget(GetGridPosition(), sensorRadius);
			}
			else
			{
				findClosestTarget(GetGridPosition(), (GetWeaponRange()/GRIDSIZE));
			}
			
			// if we found a target, initiate an attack command
			if (target!=0)
			{
				sendAttackPacket();
				target = 0;			// clear this target until we get the word
				dwTargetID = 0;	
			}
		}
	}
	
	//
	// update unit movement
	//

	if (attackAgentID!=0)
	{
		if (unitStance == US_DEFEND || unitStance == US_ATTACK)
		{
			if (target!=0 && target->GetSystemID() == systemID)
			{
				if (--moveIdleTimer < 0)
				{
					moveIdleTimer = MOVE_IDLE_TIMER;
					setgotoPivot();
				}
			}
			//
			// go back to defend pivot
			//
			if (target == 0 && bDefendPivotValid && unitStance == US_DEFEND)
			{
				if (--moveIdleTimer < 0)
				{
					moveIdleTimer = MOVE_IDLE_TIMER;
					gotoPivot = defendPivot;		// delay the move until a network sync

					if (oldGotoPivot == gotoPivot)
						moveIdleTimer = EXT_MOVE_IDLE_TIMER;	// wait a long time next time if this happens
					else
						oldGotoPivot = gotoPivot;
				}
			}
		}
	}
	else
	if (escortAgentID!=0)
	{
		if (--moveIdleTimer < 0)
		{
			moveIdleTimer = MOVE_IDLE_TIMER;
			if (escort!=0 && escort->GetSystemID() == systemID)
			{
				gotoPivot = escort->GetGridPosition();
				if (oldGotoPivot == gotoPivot)
					moveIdleTimer = EXT_MOVE_IDLE_TIMER;	// wait a long time next time if this happens
				else
					oldGotoPivot = gotoPivot;
			}
		}
	}
	
	return 1;
}
//---------------------------------------------------------------------------
// do update code that happens on both client and host machine
//
static bool bUpdateBoatClient = true;

BOOL32 Gunboat::updateBoatClient (void)
{
	if(!bUpdateBoatClient)
		return 1;

	if (bExploding)
		return 1;

	if (suppliesDisplayDecay > 0)
		suppliesDisplayDecay--;

	if (attackAgentID != 0 && isMoveActive() == 0)
	{
		if(target!= 0 && target->GetSystemID() == systemID)
		{
			rotateToTarget();
		}
		else
		{
			rotateToTargetPos();
		}
	}
	
	if (bReady)
	{
		int i;

		for (i = 0; i < MAX_GUNBOAT_LAUNCHERS; i++)
		{
			if (launcher[i].Ptr())
			{
				launcher[i].Ptr()->Update();
			}
		}
	}

	return 1;
}
//---------------------------------------------------------------------------
//
void Gunboat::destroyLaunchers (void)
{
	int i;

	for (i = 0; i < MAX_GUNBOAT_LAUNCHERS; i++)
	{
		delete launcher[i].Ptr();
		launcher[i] = 0;
	}
}
//---------------------------------------------------------------------------
// 
void Gunboat::rotateToTarget (void)
{
	CQASSERT(target);

	if (isMoveActive()==0 && optimalFacingAngle >= 0)    // if has valid facing angle
	{
		SINGLE yaw = transform.get_yaw();
		SINGLE relYaw;
		Vector dir = target->GetPosition();
		dir -= transform.get_position();	// vector to target
		dir.z = 0;

		relYaw = fixAngle(get_angle(dir.x, dir.y) - yaw);
		if (relYaw <= 0)
			relYaw += optimalFacingAngle;
		else
			relYaw -= optimalFacingAngle;

		if (fabs(relYaw) > 2.0F * MUL_DEG_TO_RAD || fabs(ang_velocity.z) > 2.0F * MUL_DEG_TO_RAD)
		{
			rotateShip(relYaw, 0 - transform.get_roll(), 0 - transform.get_pitch());
			moveTo(transform.translation);
		}
	}
}
//---------------------------------------------------------------------------
// 
void Gunboat::rotateToTargetPos (void)
{
	if (isMoveActive()==0 && optimalFacingAngle >= 0 && (targetPos.isZero()==0))    // if has valid facing angle
	{
		SINGLE yaw = transform.get_yaw();
		SINGLE relYaw;
		Vector dir = targetPos;
		dir -= transform.get_position();	// vector to target
		dir.z = 0;

		relYaw = fixAngle(get_angle(dir.x, dir.y) - yaw);
		if (relYaw <= 0)
			relYaw += optimalFacingAngle;
		else
			relYaw -= optimalFacingAngle;

		if (fabs(relYaw) > 2.0F * MUL_DEG_TO_RAD || fabs(ang_velocity.z) > 2.0F * MUL_DEG_TO_RAD)
		{
			rotateShip(relYaw, 0 - transform.get_roll(), 0 - transform.get_pitch());
			moveTo(transform.translation);
		}
	}
}
//---------------------------------------------------------------------------
//
void Gunboat::physUpdateBoat (SINGLE dt)
{
	int i;

	for (i = 0; i < MAX_GUNBOAT_LAUNCHERS; i++)
		if (launcher[i].Ptr())
			launcher[i].Ptr()->PhysicalUpdate(dt);
}
//---------------------------------------------------------------------------
//
void Gunboat::SetTerrainFootprint ( ITerrainMap *map)
{
	SpaceShip<GUNBOAT_SAVELOAD, GUNBOAT_INIT>::SetTerrainFootprint(map);

	for (int i = 0; i < MAX_GUNBOAT_LAUNCHERS; i++)
		if (launcher[i].Ptr())
			launcher[i].Ptr()->SetTerrainFootprint(map);
}
//---------------------------------------------------------------------------
//
void Gunboat::UpdateVisibilityFlags (void)
{
	IBaseObject::UpdateVisibilityFlags();
	if (bMimic)
	{
		OBJPTR<ILauncher> launcher;
		GetLauncher(2,launcher);
		if (launcher)
		{
			VOLPTR(IMimic) mimic=launcher.Ptr();
			mimic->UpdateDiscovered();
		}
	}
}
//---------------------------------------------------------------------------
//
void Gunboat::Render (void)
{
	int i;

	SpaceShip<GUNBOAT_SAVELOAD, GUNBOAT_INIT>::Render();

	for (i = 0; i < MAX_GUNBOAT_LAUNCHERS; i++)
		if (launcher[i].Ptr())
			launcher[i].Ptr()->Render();

	if (DEFAULTS->GetDefaults()->bInfoHighlights && bHighlight)
	{
		if((GetWeaponRange()/GRIDSIZE) > 0)
		{
			drawRangeCircle((GetWeaponRange()/GRIDSIZE), RGB(255,0,0));
		}
		if(optimalWeaponRange > 0)
		{
			drawRangeCircle(optimalWeaponRange, RGB(128,0,0));
		}
	}
}
//---------------------------------------------------------------------------
//
void Gunboat::RevealFog (const U32 currentSystem)
{
	int i;

	SpaceShip<GUNBOAT_SAVELOAD, GUNBOAT_INIT>::RevealFog(currentSystem);

	for (i = 0; i < MAX_GUNBOAT_LAUNCHERS; i++)
		if (launcher[i].Ptr())
			launcher[i].Ptr()->RevealFog(currentSystem);
}
//---------------------------------------------------------------------------
//
void Gunboat::CastVisibleArea (void)
{
	int i;

	SpaceShip<GUNBOAT_SAVELOAD, GUNBOAT_INIT>::CastVisibleArea();

	for (i = 0; i < MAX_GUNBOAT_LAUNCHERS; i++)
		if (launcher[i].Ptr())
			launcher[i].Ptr()->CastVisibleArea();
}
//---------------------------------------------------------------------------
//
void Gunboat::DrawFleetMoniker (bool bAllShips)
{
	if (bVisible && (dwMissionID & PLAYERID_MASK) == MGlobals::GetThisPlayer())
	{
		if (suppliesDisplayDecay > 0 && bHighlight==0 && supplyPointsMax && DBHOTKEY->GetHotkeyState(IDH_HIGHLIGHT_ALL)==0)
		{
			U32 percent = (supplies * 100) / supplyPointsMax;

			if (percent <= 50)
			{
				Vector point;
				COMPTR<IDebugFontDrawAgent> pFont = DEBUGFONT;
				S32 x, y;

//				if (OBJLIST->GetUnitFont(pFont) == GR_OK)
				{
					point.x = 0;
					point.y = H2+250.0;
					point.z = 0;

					CAMERA->PointToScreen(point, &x, &y, &transform);
					PANE * pane = CAMERA->GetPane();
					

//					pFont->SetFontColor(RGB(0,128,255) | 0xFF000000, 0);
					char temp[M_MAX_STRING];
					sprintf(temp, "%d%%", percent);
					pFont->StringDraw(pane, x-20, y+10, temp, RGB(0,128,255) | 0xFF000000);	
				}
			}
		}
	}

	SpaceShip<GUNBOAT_SAVELOAD, GUNBOAT_INIT>::DrawFleetMoniker(bAllShips);
}
//---------------------------------------------------------------------------
//
void Gunboat::GotoPosition (const struct GRIDVECTOR & pos, U32 agentID, bool bSlowMove)
{
	bDefendPivotValid = true;
	defendPivot = pos;

	SpaceShip<GUNBOAT_SAVELOAD, GUNBOAT_INIT>::GotoPosition(pos, agentID, bSlowMove);
}
//---------------------------------------------------------------------------
//
void Gunboat::UseJumpgate (IBaseObject * outgate, IBaseObject * ingate, const Vector& jumpToPosition, SINGLE heading, SINGLE speed, U32 agentID)
{
	bDefendPivotValid = false;
	SpaceShip<GUNBOAT_SAVELOAD, GUNBOAT_INIT>::UseJumpgate(outgate, ingate, jumpToPosition, heading, speed, agentID);
}
//---------------------------------------------------------------------------
//
void Gunboat::RepairYourselfAt (IBaseObject * platform, U32 agentID)
{
	CQASSERT(attackAgentID == 0);		// should have gotten a cancel before this
	CQASSERT(escortAgentID == 0);

	idleTimer = IDLE_TIMER;
	bRepairUnderway = true;
	gotoPivot.zero();
	oldGotoPivot.zero();

	SpaceShip<GUNBOAT_SAVELOAD, GUNBOAT_INIT>::RepairYourselfAt(platform, agentID);
}
//---------------------------------------------------------------------------
//
void Gunboat::RepairCompleted (void)
{
	bRepairUnderway = false;
	gotoPivot.zero();
	oldGotoPivot.zero();
	SpaceShip<GUNBOAT_SAVELOAD, GUNBOAT_INIT>::RepairCompleted();
}
//---------------------------------------------------------------------------
//
bool Gunboat::CanCloak()
{
	for (U32 i = 0; i < MAX_GUNBOAT_LAUNCHERS; i++)
	{
		if (launcher[i])
		{
			if(launcher[i]->CanCloak())
				return true;
		}
	}
	return false;
}
//---------------------------------------------------------------------------
//
bool Gunboat::IsToggle()
{
	for (U32 i = 0; i < MAX_GUNBOAT_LAUNCHERS; i++)
	{
		if (launcher[i])
		{
			if(launcher[i]->IsToggle())
				return true;
		}
	}
	return false;	
}
//---------------------------------------------------------------------------
//
bool Gunboat::CanToggle()
{
	for (U32 i = 0; i < MAX_GUNBOAT_LAUNCHERS; i++)
	{
		if (launcher[i])
		{
			if(launcher[i]->CanToggle())
				return true;
		}
	}
	return false;	
}
//---------------------------------------------------------------------------
//
bool Gunboat::IsOn()
{
	for (U32 i = 0; i < MAX_GUNBOAT_LAUNCHERS; i++)
	{
		if (launcher[i])
		{
			if(launcher[i]->IsToggle())
				return launcher[i]->IsOn();
		}
	}
	return false;
}
//---------------------------------------------------------------------------
//
void Gunboat::Escort (IBaseObject * _target, U32 agentID)
{
	CQASSERT(attackAgentID == 0);		// should have gotten a cancel before this
	CQASSERT(escortAgentID == 0);

	idleTimer = IDLE_TIMER;
	gotoPivot.zero();
	oldGotoPivot.zero();
	escortAgentID = agentID;
	bDefendPivotValid = false;

	if (_target)
	{
		_target->QueryInterface(IBaseObjectID, escort, playerID);
	}
	else
	{
		escort = 0;
	}

	if (escort)
		dwEscortID = escort->GetPartID();
	else
		dwEscortID = 0;
}
//---------------------------------------------------------------------------
//
void Gunboat::DoSpecialAbility (U32 specialID)
{
	specialQueue.addTo(SPECIAL_SELF, specialID);
	/*
	int i;
	for (i = 0; i < MAX_GUNBOAT_LAUNCHERS; i++)
	{
		if (launcher[i])
		{
			launcher[i]->DoSpecialAbility();
		}
	}
	*/
}
//---------------------------------------------------------------------------
//
void Gunboat::DoSpecialAbility (IBaseObject *obj)
{
	if (obj)
		specialQueue.addTo(SPECIAL_TARGET, obj->GetPartID());

	/*
	int i;
	for (i = 0; i < MAX_GUNBOAT_LAUNCHERS; i++)
	{
		if (launcher[i])
		{
			launcher[i]->DoSpecialAbility(obj);
		}
	}
	*/
}
//---------------------------------------------------------------------------
//
void Gunboat::DoCloak (void)
{
	specialQueue.addTo(SPECIAL_CLOAK, 0);
}
//---------------------------------------------------------------------------
//
void Gunboat::OnAllianceChange (U32 allyMask)
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
//---------------------------------------------------------------------------
//
void Gunboat::WaitForAdmiral (bool bWait)
{
	bWaitingForAdmiral = bWait;
	gotoPivot.zero();
	oldGotoPivot.zero();
}
//---------------------------------------------------------------------------
//
bool Gunboat::IsAvailableForAdmiral (void)
{
	// the gunboat is available for action by the admiral as long as it doesn't have a command
	// that's user generated, its part of a fleet, and it doesn't have any pending ops
	return (bUserGenerated == false && fleetID && THEMATRIX->HasPendingOp(this) == false && bCloaked == false && bMimic == false);
}
//---------------------------------------------------------------------------
//
IBaseObject * Gunboat::GetFleetshipTarget()
{
	return target;
}
//---------------------------------------------------------------------------
//
bool Gunboat::IsInMove (void)
{
	return isMoveActive();
}
//---------------------------------------------------------------------------
//
void Gunboat::UseArtifactOn(IBaseObject * victim, U32 agentID)
{
	CQASSERT(attackAgentID==0);
	CQASSERT(escortAgentID==0);
	CQASSERT(agentID);			// can't have no agent+usergenerated

	oldGotoPivot.zero();
	bool _bUserGenerated = true;

	// if the user generated this attack, then we *always* have to move to position
	if (_bUserGenerated || unitStance != US_STAND)
	{
		if (victim && victim->IsVisibleToPlayer(playerID) == false && victim->GetSystemID() == systemID)
		{
			gotoPivot = victim->GetGridPosition();
		}
		else
		{
			gotoPivot.zero();
			
			// force a move right off the bat so as to not look foolish
			if (victim && victim->GetSystemID() == systemID)
			{
				victim->QueryInterface(IBaseObjectID, target, playerID);
				setgotoPivot();
		
				if (gotoPivot.isZero()==0)
				{
					// it's okay to do the moveToPos since we just got an attack
					moveToPos(gotoPivot);
				}
			}
		}
	}

	if (victim && _bUserGenerated)
	{
		const U32 hisID = victim->GetPlayerID();
	
		// we are purposely attacking an ally
		if (hisID && playerID != hisID && MGlobals::AreAllies(playerID, hisID))
		{
			MPart part = victim;
			COMM_ALLIED_ATTACK(victim, victim->GetPartID(),SUB_ALLIED_ATTACK,part.pInit->displayName);
		}

		bDefendPivotValid = true;
		defendPivot = victim->GetGridPosition();
	}

	// reset the idle timer
	idleTimer = IDLE_TIMER;

	bRecallFighters = false;
	
	if (victim)
	{
		dwTargetID = victim->GetPartID();
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

	if(artifact)
		artifact->SetTarget(target);

	netTargetID = dwTargetID;		// already sync'ed up
	attackAgentID = agentID;
	bUserGenerated = _bUserGenerated;
	bArtifactUse = true;
	bRecallFighters = false;
	bSpecialAttack = false;
	loseTargetTimer = LOSE_TARGET_TIMER;
}
//---------------------------------------------------------------------------
//
void Gunboat::Attack (IBaseObject * victim, U32 agentID, bool _bUserGenerated)
{
	CQASSERT(attackAgentID==0);
	CQASSERT(escortAgentID==0);
	CQASSERT(agentID);			// can't have no agent+usergenerated

	oldGotoPivot.zero();

	// if the user generated this attack, then we *always* have to move to position
	if (_bUserGenerated || unitStance != US_STAND)
	{
		if (victim && victim->IsVisibleToPlayer(playerID) == false && victim->GetSystemID() == systemID)
		{
			gotoPivot = victim->GetGridPosition();
		}
		else
		{
			gotoPivot.zero();
			
			// force a move right off the bat so as to not look foolish
			if (victim && victim->GetSystemID() == systemID)
			{
				victim->QueryInterface(IBaseObjectID, target, playerID);
				setgotoPivot();
		
				if (gotoPivot.isZero()==0)
				{
					// it's okay to do the moveToPos since we just got an attack
					moveToPos(gotoPivot);
				}
			}
		}
	}

	if (victim && _bUserGenerated)
	{
		const U32 hisID = victim->GetPlayerID();
	
		// we are purposely attacking an ally
		if (hisID && playerID != hisID && MGlobals::AreAllies(playerID, hisID))
		{
			MPart part = victim;
			COMM_ALLIED_ATTACK(victim, victim->GetPartID(),SUB_ALLIED_ATTACK,part.pInit->displayName);
		}

		bDefendPivotValid = true;
		defendPivot = victim->GetGridPosition();
	}

	// reset the idle timer
	idleTimer = IDLE_TIMER;

	bRecallFighters = false;
	
	if (victim)
	{
		dwTargetID = victim->GetPartID();
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

	setAttackTarget(target);

	netTargetID = dwTargetID;		// already sync'ed up
	attackAgentID = agentID;
	bUserGenerated = _bUserGenerated;
	loseTargetTimer = LOSE_TARGET_TIMER;
}
//---------------------------------------------------------------------------
//
void Gunboat::AttackPosition(const struct GRIDVECTOR & position, U32 agentID)
{
	bRecallFighters = false;
	bSpecialAttack = true;
	bUserGenerated = true;
	bArtifactUse = false;
	GRIDVECTOR pos = position;

	attackAgentID = agentID;
	dwTargetID = netTargetID = 0;
	target = 0;

	for (int i = 0; i < MAX_GUNBOAT_LAUNCHERS; i++)
	{
		if (launcher[i])
		{
			launcher[i]->AttackPosition(&pos,false);
		}
	}
	targetPos = position;

	setgotoPivotPosition(pos);

	bDefendPivotValid = true;
	defendPivot = gotoPivot;
}
//---------------------------------------------------------------------------
//
void Gunboat::CancelAttack (void)
{
	CQASSERT(attackAgentID==0);
	setAttackTarget(NULL);
	if(artifact)
		artifact->SetTarget(NULL);
	dwTargetID = netTargetID = 0;
	target = 0;
}
//---------------------------------------------------------------------------
//
void Gunboat::setAttackTarget (IBaseObject * victim)
{
	int i;

	bRecallFighters = false;

	for (i = 0; i < MAX_GUNBOAT_LAUNCHERS; i++)
	{
		if (launcher[i])
		{
			launcher[i]->AttackObject(victim);
		}
	}
}
//---------------------------------------------------------------------------
//
void Gunboat::reportKillToAdmiral (void)
{
	if (fleetID)
	{
		MPartNC part = OBJLIST->FindObject(fleetID);
		if (part.isValid())
		{
			part->numKills++;
			techLevel.experience = static_cast<MISSION_SAVELOAD::InstanceTechLevel::UPGRADE>(MGlobals::GetAdmiralExperienceLevel(part->numKills));
		}
		else
		{
			fleetID = 0;
		}
	}
}	
//---------------------------------------------------------------------------
// return TRUE if admiral gave us a new target
//
bool Gunboat::notifyAdmiralOfTargetLoss (void)
{
	bool result = false;
	if (fleetID)
	{
		VOLPTR(IAdmiral) admiral = OBJLIST->FindObject(fleetID);

		if (admiral)
		{
			IBaseObject * newtarget = admiral->FleetShipTargetDestroyed(this);

			if (newtarget)
			{
				SetFleetshipTarget(newtarget);
				result = true;
			}
		}
	}

	return result;
}
//---------------------------------------------------------------------------
//
void Gunboat::ReportKill (U32 partID)
{
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

	reportKillToAdmiral();
}
//---------------------------------------------------------------------------
//
void Gunboat::SpecialAttack (IBaseObject * victim, U32 agentID)
{
	CQASSERT(attackAgentID==0);
	CQASSERT(escortAgentID==0);

	oldGotoPivot.zero();

	attackAgentID = agentID;
	bRecallFighters = false;
	bUserGenerated = true;
	bSpecialAttack = true;
	bArtifactUse = false;

	if (victim && victim->IsVisibleToPlayer(playerID) == false && victim->GetSystemID() == systemID)
	{
		gotoPivot = victim->GetGridPosition();
	}
	else
	{
		gotoPivot.zero();
		
		// force a move right off the bat so as to not look foolish
		if (victim && victim->GetSystemID() == systemID)
		{
			victim->QueryInterface(IBaseObjectID, target, playerID);
			setgotoPivot();
	
			if (gotoPivot.isZero()==0)
			{
				// it's okay to do the moveToPos since we just got an attack
				moveToPos(gotoPivot);
			}
		}
	}

	for (int i = 0; i < MAX_GUNBOAT_LAUNCHERS; i++)
	{
		if (launcher[i])
		{
			launcher[i]->SpecialAttackObject(victim);
		}
	}

	if (victim)
	{
		bDefendPivotValid = true;
		defendPivot = victim->GetGridPosition();
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
void Gunboat::SpecialAOEAttack (const struct GRIDVECTOR & position, U32 agentID)
{
	bRecallFighters = false;
	bSpecialAttack = true;
	bUserGenerated = true;
	bArtifactUse = false;
	GRIDVECTOR pos = position;

	attackAgentID = agentID;
	dwTargetID = netTargetID = 0;
	target = 0;

	for (int i = 0; i < MAX_GUNBOAT_LAUNCHERS; i++)
	{
		if (launcher[i])
		{
			launcher[i]->AttackPosition(&pos,true);
		}
	}

	setgotoPivotPosition(pos);

	bDefendPivotValid = true;
	defendPivot = gotoPivot;
}
//---------------------------------------------------------------------------
//
void Gunboat::WormAttack (IBaseObject * victim, U32 agentID)
{
	bRecallFighters = false;
	bSpecialAttack = true;
	bUserGenerated = true;
	bArtifactUse = false;

	attackAgentID = agentID;
	dwTargetID = netTargetID = 0;
	target = 0;

	int i;
	GRIDVECTOR pos;
	pos = victim->GetPosition();
	for (i = 0; i < MAX_GUNBOAT_LAUNCHERS; i++)
	{
		if (launcher[i])
		{
			launcher[i]->WormAttack(victim);
		}
	}

	moveToPos(pos);

	bDefendPivotValid = true;
	defendPivot = pos;
}
//---------------------------------------------------------------------------
//
void Gunboat::MultiSystemAttack (struct GRIDVECTOR & position, U32 targSystemID, U32 agentID)
{
	bRecallFighters = false;
	int i;
	GRIDVECTOR pos = position;
	for (i = 0; i < MAX_GUNBOAT_LAUNCHERS; i++)
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
void Gunboat::DoCreateWormhole(U32 systemID, U32 agentID)
{
	THEMATRIX->OperationCompleted(agentID,dwMissionID);
	bRecallFighters = false;
	int i;
	for (i = 0; i < MAX_GUNBOAT_LAUNCHERS; i++)
	{
		if (launcher[i])
		{
			launcher[i]->DoCreateWormhole(systemID);
		}
	}
}
//---------------------------------------------------------------------------
//
void Gunboat::SetUnitStance (const UNIT_STANCE stance)
{
	unitStance = stance;
}
//---------------------------------------------------------------------------
//
const UNIT_STANCE Gunboat::GetUnitStance (void) const
{
	return unitStance;
}
//---------------------------------------------------------------------------
//
void Gunboat::GetSpecialAbility (UNIT_SPECIAL_ABILITY & ability, bool & bSpecialEnabled)
{
	// okay, what's the special ability here?
	ability = USA_NONE;
	bSpecialEnabled = false;

	int i;
	for (i = 0; i < MAX_GUNBOAT_LAUNCHERS; i++)
	{
		if (launcher[i])
		{
			launcher[i]->GetSpecialAbility(ability, bSpecialEnabled);
			if (ability != USA_NONE)
			{
				// we've found our special ability, return
				return;
			}
		}
	}
}
//---------------------------------------------------------------------------
//
BOOL32 Gunboat::Save (struct IFileSystem * file)
{
	int i;
	OBJPTR<ISaveLoad> obj;

	SpaceShip<GUNBOAT_SAVELOAD, GUNBOAT_INIT>::Save(file);

	char buffer[64];

	for (i = 0; i < MAX_GUNBOAT_LAUNCHERS; i++)
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

	return 1;
}
//---------------------------------------------------------------------------
//
BOOL32 Gunboat::Load (struct IFileSystem * file)
{
	int i;
	OBJPTR<ISaveLoad> obj;
	char buffer[64];

	SpaceShip<GUNBOAT_SAVELOAD, GUNBOAT_INIT>::Load(file);

	initLaunchers();

	for (i = 0; i < MAX_GUNBOAT_LAUNCHERS; i++)
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

	return 1;
}
//---------------------------------------------------------------------------
//
void Gunboat::InitActor (void)
{
	SpaceShip<GUNBOAT_SAVELOAD, GUNBOAT_INIT>::InitActor();
	initLaunchers();
}
//---------------------------------------------------------------------------
//
void Gunboat::initLaunchers (void)
{
	int i;
	IBaseObject * tmp;

	CQASSERT(launcher[0] == 0);

	for (i = 0; i < MAX_GUNBOAT_LAUNCHERS; i++)
	{
		if (ginitData->launchers[i])
		{
			if ((tmp = ARCHLIST->CreateInstance(ginitData->launchers[i])) != 0)
			{
				if (tmp->QueryInterface(ILauncherID, launcher[i], NONSYSVOLATILEPTR))
				{
					launcher[i]->InitLauncher(this, instanceIndex, ginitData->animArchetype, outerWeaponRange*GRIDSIZE);
				}
				else
				{
					delete tmp;
					break;
				}
			}
			else
			{
				break;
			}
		}
		else
		{
			break;
		}
	}
}
//---------------------------------------------------------------------------
//
U32 Gunboat::getSyncStance (void * buffer)
{
	U32 result = 0;
	if (netFighterStance != fighterStance)
	{
		((U8*)buffer)[result++] = unitStance;
		((U8*)buffer)[result++] = fighterStance;
		netUnitStance = unitStance;
		netFighterStance = fighterStance;
	}
	else if (netUnitStance != unitStance)
	{
		((U8*)buffer)[result++] = unitStance;
		netUnitStance = unitStance;
	}
	return result;
}
//---------------------------------------------------------------------------
//
void Gunboat::putSyncStance (void * buffer, U32 bufferSize, bool bLateDelivery)
{
	U8 stance = ((U8*)buffer)[0];
	netUnitStance = unitStance = UNIT_STANCE(stance);
	if(bufferSize == 2)
	{
		stance = ((U8*)buffer)[1];
		netFighterStance = fighterStance = FighterStance(stance);
	}
}
//---------------------------------------------------------------------------
//
U32 Gunboat::getSyncSpecial (void * _buffer)
{
	U32 result = 0;

	SPECIALQUEUENODE * node = specialQueue.getNext();
	if (node)
	{
		U8 * buffer = (U8 *) _buffer;
		*buffer = (U8) node->type;
		result++;
		if (node->type == SPECIAL_TARGET || node->type == SPECIAL_SELF)
		{
			U32 *pBuf = (U32 *) (buffer + 1);
			*pBuf = node->dwTargetID;
			result += sizeof(U32);
		}

		putSyncSpecial(_buffer, result, false);
	}

	return result;
}
//---------------------------------------------------------------------------
//
void Gunboat::putSyncSpecial (void * buffer, U32 bufferSize, bool bLateDelivery)
{
	CQASSERT(bufferSize > 0);
	SPECIALTYPE type = static_cast<SPECIALTYPE>(((U8 *)buffer)[0]);

	if (type == SPECIAL_TARGET)
	{
		CQASSERT(bufferSize == 5);
		U32 *pBuf = (U32 *) (((char *)buffer) + 1);
		// do special ability + target
		IBaseObject * obj = OBJLIST->FindObject(*pBuf);

		if (obj)
		{
			int i;
			for (i = 0; i < MAX_GUNBOAT_LAUNCHERS; i++)
			{
				if (launcher[i])
				{
					launcher[i]->DoSpecialAbility(obj);
				}
			}
		}		
	}
	else
	if (type == SPECIAL_SELF)
	{
		U32 *pBuf = (U32 *) (((char *)buffer) + 1);
		int i;
		for (i = 0; i < MAX_GUNBOAT_LAUNCHERS; i++)
		{
			if (launcher[i])
			{
				launcher[i]->DoSpecialAbility(*pBuf);
			}
		}
	}
	else
	if (type == SPECIAL_CLOAK)
	{
		int i;
		for (i = 0; i < MAX_GUNBOAT_LAUNCHERS; i++)
		{
			if (launcher[i])
			{
				launcher[i]->DoCloak();
			}
		}
	}
	else
	{
		CQBOMB1("Invalid type=%d received.", type);
	}
}
//---------------------------------------------------------------------------
//
U32 Gunboat::getSyncTarget (void * _buffer)
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
void Gunboat::putSyncTarget (void * _buffer, U32 bufferSize, bool bLateDelivery)
{
	CQASSERT(bufferSize == sizeof(U32));
	U32 * buffer = (U32 *) _buffer;

	netTargetID = dwTargetID = *buffer;
	OBJLIST->FindObject(dwTargetID, playerID, target, IBaseObjectID);
	setAttackTarget(target);
}
//---------------------------------------------------------------------------
//
U32 Gunboat::getSyncMove (void * buffer)
{
	U32 result = 0;
	if (gotoPivot.isZero() == 0)		// we have valid position to go to
	{
		GRIDVECTOR * pPos = (GRIDVECTOR *) buffer;
		result = sizeof(GRIDVECTOR);
		*pPos = gotoPivot;
		moveToPos(gotoPivot);
		gotoPivot.zero();
	}
	return result;
}
//---------------------------------------------------------------------------
//
void Gunboat::putSyncMove (void * buffer, U32 bufferSize, bool bLateDelivery)
{
	GRIDVECTOR * pPos = (GRIDVECTOR *) buffer;
	CQASSERT(bufferSize == sizeof(GRIDVECTOR));

	// don't do this if the packet came in late
	if (bLateDelivery == 0)
	{
		//	CQASSERT(attackAgentID != 0);
		moveToPos(*pPos);
		gotoPivot.zero();
	}
}
//---------------------------------------------------------------------------
//
U32 Gunboat::getSyncLaunchers (void * buffer)
{
	U32 result = 0;
	int i;

	if(artifact)
	{
		if(artifact->GetSyncDataSize())
		{
			result = artifact->GetSyncData(buffer);
			((U8*)buffer)[result++] = MAX_GUNBOAT_LAUNCHERS;
			return result;
		}
	}

	for (i = 0; i < MAX_GUNBOAT_LAUNCHERS; i++)
	{
		if (launcher[i].Ptr() && (result = launcher[i]->GetSyncData(buffer)) != 0)
		{
			((U8*)buffer)[result++] = i;
			break;
		}
	}

	return result;
}
//---------------------------------------------------------------------------
//
void Gunboat::putSyncLaunchers (void * buffer, U32 bufferSize, bool bLateDelivery)
{
	U8 a = ((U8*)buffer)[--bufferSize];
	if(a == MAX_GUNBOAT_LAUNCHERS)//is it an artifact
	{	
		if(artifact)
			artifact->PutSyncData(buffer,bufferSize);
	}
	else
	{
		launcher[a]->PutSyncData(buffer, bufferSize);
	}
}
//---------------------------------------------------------------------------
//
void Gunboat::TakeoverSwitchID (U32 newMissionID)
{
	SpaceShip<GUNBOAT_SAVELOAD, GUNBOAT_INIT>::TakeoverSwitchID(newMissionID);
	for (U32 i = 0; i < MAX_GUNBOAT_LAUNCHERS; i++)
	{
		if (launcher[i].Ptr())
		{
			launcher[i]->TakeoverSwitchID(newMissionID);
		}
	}
}
//---------------------------------------------------------------------------
//
void Gunboat::onOperationCancel (U32 agentID)
{
	targetPos.zero();
	if (agentID == attackAgentID)
	{
		attackAgentID = 0;
		int i;
		for (i = 0; i < MAX_GUNBOAT_LAUNCHERS; i++)
		{
			if (launcher[i])
			{
				launcher[i]->InformOfCancel();
			}
		}
		extIdleTimer = EXT_IDLE_TIMER;
		moveIdleTimer = MOVE_IDLE_TIMER;
		gotoPivot.zero();
		oldGotoPivot.zero();
	}
	else
	if (agentID == escortAgentID)
	{
		escortAgentID = 0;
		escort = NULL;
		dwEscortID = 0;
		extIdleTimer = EXT_IDLE_TIMER;
		moveIdleTimer = MOVE_IDLE_TIMER;
		gotoPivot.zero();
		oldGotoPivot.zero();
	}

	bUserGenerated = false;
	bWaitingForAdmiral = false;
	bRepairUnderway = false;
	bSpecialAttack = false;
	bDefendPivotValid = false;
	bArtifactUse = false;
}
//---------------------------------------------------------------------------
//
void Gunboat::preTakeover (U32 newMissionID, U32 troopID)
{
	if (THEMATRIX->IsMaster())
	{
		if (attackAgentID)
		{
			cancelAttack();
		}

		if (escortAgentID)
		{
			cancelEscort();
		}
	}

	targetPos.zero();
	gotoPivot.zero();
	oldGotoPivot.zero();
	bWaitingForAdmiral = false;
	bRepairUnderway = false;
	bDefendPivotValid = false;

	int i;

	for (i = 0; i < MAX_GUNBOAT_LAUNCHERS; i++)
	{
		if (launcher[i].Ptr())
			launcher[i]->HandlePreTakeover(newMissionID, troopID);
	}
}
//---------------------------------------------------------------------------
//
void Gunboat::notifyAdmiralOfDamage (U32 attackerID)
{
	if (fleetID)
	{
		OBJPTR<IAdmiral> admiral;
		IBaseObject * obj = OBJLIST->FindObject(fleetID);
		if (obj && obj->QueryInterface(IAdmiralID, admiral))
		{
			admiral->OnFleetShipDamaged(this, attackerID);
		}
		else
		{
			fleetID = 0;
		}
	}
}
//---------------------------------------------------------------------------
//
void Gunboat::notifyAdmiralOfDestruction (void)
{
	if (fleetID)
	{
		OBJPTR<IAdmiral> admiral;
		if (OBJLIST->FindObject(fleetID, TOTALLYVOLATILEPTR, admiral, IAdmiralID))
		{
			admiral->OnFleetShipDestroyed(this);
		}
		else
		{
			fleetID = 0;
		}
	}
}
//---------------------------------------------------------------------------
//
BOOL32 Gunboat::ApplyDamage (IBaseObject * collider, U32 _ownerID, const Vector & pos, const Vector &dir,U32 amount,BOOL32 bShieldHit)
{
	BOOL32 result;
	// only look away if nothing else to do
	if (dwTargetID==0 && isMoveActive()==0 && THEMATRIX->HasPendingOp(this)==0 && idleTimer > IDLE_TIMER/2)
		extIdleTimer /= 2;
	result = SpaceShip<GUNBOAT_SAVELOAD, GUNBOAT_INIT>::ApplyDamage(collider, _ownerID, pos, dir, amount, bShieldHit);

	notifyAdmiralOfDamage(_ownerID);

	return result;
}
//---------------------------------------------------------------------------
//
void Gunboat::ApplyAOEDamage (U32 _ownerID, U32 damageAmount)
{
	// only look away if nothing else to do
	if (dwTargetID==0 && isMoveActive()==0 && THEMATRIX->HasPendingOp(this)==0 && idleTimer > IDLE_TIMER/2)
		extIdleTimer /= 2;
	SpaceShip<GUNBOAT_SAVELOAD, GUNBOAT_INIT>::ApplyAOEDamage(_ownerID, damageAmount);

	notifyAdmiralOfDamage(_ownerID);
}
//---------------------------------------------------------------------------
//
void Gunboat::broadcastHelpMsg (U32 attackerID)
{
	// if we didn't already take care of this in the gunboat ApplyDamage() section
	if (fleetID==0)
		SpaceShip<GUNBOAT_SAVELOAD, GUNBOAT_INIT>::broadcastHelpMsg(attackerID);
}
//---------------------------------------------------------------------------
//
void Gunboat::resolve (void)
{
	int i;
	OBJPTR<ISaveLoad> obj;

	if (dwTargetID)
	{
		OBJLIST->FindObject(dwTargetID, playerID, target);
	}
	else if (dwEscortID)
	{
		OBJLIST->FindObject(dwEscortID, playerID, escort);
	}

	for (i = 0; i < MAX_GUNBOAT_LAUNCHERS; i++)
		if (launcher[i].Ptr() && launcher[i]->QueryInterface(ISaveLoadID, obj))
			obj->ResolveAssociations();

	if (target && attackAgentID && (!bSpecialAttack) && (!bArtifactUse))
		setAttackTarget(target);		// make sure launchers turn off
	else
		setAttackTarget(NULL);		// make sure launchers turn off
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
void Gunboat::OnChildDeath (INSTANCE_INDEX child)
{
	int i;

	for (i = 0; i < MAX_GUNBOAT_LAUNCHERS; i++)
		if (launcher[i].Ptr() && isRelated(child, launcher[i].Ptr()->GetObjectIndex()))
		{
			delete launcher[i].Ptr();
			launcher[i] = 0;
		}
}
//---------------------------------------------------------------------------
//
bool Gunboat::UseSupplies (U32 amount,bool bAbsolute)
{
	if(fieldFlags.suppliesLocked())
		return false;
	USER_DEFAULTS * const defaults = DEFAULTS->GetDefaults();
	if (defaults->bCheatsEnabled && defaults->bNoSupplies)
		return true;

	suppliesDisplayDecay = SUPPLIES_DISPLAY_PERIOD;

	if(fleetID && amount)
	{
		VOLPTR(IAdmiral) flagship;
		OBJLIST->FindObject(fleetID,TOTALLYVOLATILEPTR,flagship,IAdmiralID);
		if(flagship.Ptr())
		{
			MPart part(this); 
			amount = flagship->ConvertSupplyBonus(mObjClass,part.pInit->armorData.myArmor,amount);
		}				
	}

	if(amount)
	{
		SINGLE supplyMod = 1.0-SECTOR->GetSectorEffects(playerID,systemID)->getSupplyMod();
		amount *= supplyMod;
		amount = __max(1,amount);
	}

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
bool Gunboat::TestLOS (const struct GRIDVECTOR & pos)
{
	TCallback callback;
	COMPTR<ITerrainMap> map;
	bool result;

	SECTOR->GetTerrainMap(GetSystemID(), map);

	if ((result=map->TestSegment(GetGridPosition(), pos, &callback)) == false)
		result = callback.gridPos.isMostlyEqual(pos);

	return result;
}
//---------------------------------------------------------------------------
//
void Gunboat::LauncherCancelAttack (void)
{
	if (attackAgentID)
	{
		resetMoveVars();
		moveToPos(GetGridPosition());		// find a valid landing place
		if (THEMATRIX->IsMaster())
			cancelAttack();
	}
}
//---------------------------------------------------------------------------
//
void Gunboat::GotoLauncherPosition (GRIDVECTOR pos)
{
	moveToPos(pos);
}
//---------------------------------------------------------------------------
//
void Gunboat::LauncherSendOpData (U32 agentID, void * buffer,U32 bufferSize)
{
	CQASSERT(agentID == launcherAgentID);
	((U8 *)buffer)[bufferSize] = LOP_SEND_DATA;
	++bufferSize;
	THEMATRIX->SendOperationData(agentID,dwMissionID,buffer,bufferSize);
}
//---------------------------------------------------------------------------
//
void Gunboat::LaunchOpCompleted (ILauncher * launcher,U32 agentID)
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
U32 Gunboat::CreateLauncherOp (ILauncher * _launcher,struct ObjSet & set,void * buffer,U32 bufferSize)
{
	for(U32 i = 0; i < MAX_GUNBOAT_LAUNCHERS; ++ i)
	{
		if(launcher[i] == _launcher)
		{
			launcherID = i;
			U8 newBuffer[256];
			memcpy(&(((U8 *)newBuffer)[sizeof(GunBoatCommandWHandle)]),buffer,bufferSize);
			GunBoatCommandWHandle * myCom = (GunBoatCommandWHandle *)newBuffer;
			myCom->command = GUNBOAT_C_LAUNCHER_OP;
			myCom->handle = launcherID;
			launcherAgentID = THEMATRIX->CreateOperation(set,&newBuffer,bufferSize+sizeof(GunBoatCommandWHandle));
			return launcherAgentID;
		}
	}
	CQASSERT(0 && "CreateLauncherOp failed");
	return 0;
}
//---------------------------------------------------------------------------
//
SINGLE Gunboat::GetWeaponRange()
{
	SINGLE admiralMod = 1.0;
	if(fleetID)
	{
		VOLPTR(IAdmiral) flagship;
		OBJLIST->FindObject(fleetID,TOTALLYVOLATILEPTR,flagship,IAdmiralID);
		if(flagship.Ptr())
		{
			MPart part(this);
			admiralMod = 1 + flagship->GetRangeBonus(mObjClass,part.pInit->armorData.myArmor);
		}				
	}
	SINGLE fighterRangeMod = 1.0;;
	if(MGlobals::IsCarrier(mObjClass))
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
	SINGLE sectorRangeMod = 1.0+SECTOR->GetSectorEffects(playerID,systemID)->getWeaponRangeMod();
	return outerWeaponRange*admiralMod*fighterRangeMod*sectorRangeMod*GRIDSIZE;
}
//---------------------------------------------------------------------------
//
SINGLE Gunboat::GetOptimalWeaponRange (void)
{
	return optimalWeaponRange;
}
//---------------------------------------------------------------------------
//
IBaseObject * Gunboat::FindChildTarget(U32 childID)
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
void Gunboat::initGunboat (const GUNBOAT_INIT & data)
{
	outerWeaponRange   = data.pData->outerWeaponRange * GRIDSIZE;
	optimalFacingAngle = data.pData->optimalFacingAngle;
	bNoLineOfSight = data.pData->bNoLineOfSight;

	if ((optimalWeaponRange = outerWeaponRange) > 0)
	{
		SINGLE diff = box[4]-box[5];
		SINGLE maxTime = 2;

		if (optimalFacingAngle > 0)
			maxTime += (optimalFacingAngle/getDynamicsData().maxAngVelocity)*4;		// factor in turning time
		
		diff = __max(diff, getDynamicsData().maxLinearVelocity*maxTime);

		optimalWeaponRange -= diff;

		if (optimalWeaponRange <= 0)
			optimalWeaponRange = 0;
	}

	optimalWeaponRange /= GRIDSIZE;
	outerWeaponRange /= GRIDSIZE;
	ginitData = &data;

	// without this check here, the game looks like catshit when fighting with corvettes
	if (optimalWeaponRange < 0.51f && optimalWeaponRange >= 0)
	{
		optimalWeaponRange = 0.51f;
	}

	// we are in defend stance by default
	unitStance = netUnitStance = US_DEFEND;
	fighterStance = netFighterStance = FS_NORMAL;
}
//---------------------------------------------------------------------------
// return TRUE if moving into position
bool Gunboat::setgotoPivot (void)
{
	if (optimalWeaponRange > 0)
	{
		//
		// set pivot point for attacks
		//
		Vector dir = transform.get_position();
		TCallback callback;
		bool bLOS = (bNoLineOfSight || checkLOS(callback));     // true if we have lineOfSight on target
		SINGLE dirMag;

		dir -= (target->GetTransform().translation);	// vector from him to us
		dir.z = 0;
		dirMag = target->GetGridPosition() - GetGridPosition();

		if (bLOS && dirMag < optimalWeaponRange)
		{
			if (isMoveActive())
			{
				resetMoveVars();
				gotoPivot.zero();
			}
		}
		else if (bLOS==0 || dirMag > (GetWeaponRange()/GRIDSIZE) || (optimalFacingAngle <= 0 && dirMag >= optimalWeaponRange))
		{
			Vector pivot;
			if (bLOS)
			{
				pivot = dir;
				pivot *= ((optimalWeaponRange + box[5]/GRIDSIZE)/dirMag);
				pivot += target->GetTransform().translation;
			}
			else
			{
				pivot = target->GetTransform().translation;
			}

			gotoPivot = pivot;		// delay the move until a net work sync

			if (oldGotoPivot == gotoPivot)
				moveIdleTimer = EXT_MOVE_IDLE_TIMER;	// wait a long time next time if this happens
			else
				oldGotoPivot = gotoPivot;
			return true;
		}
	}

	return false;
}
//---------------------------------------------------------------------------
//
bool Gunboat::setgotoPivotPosition(GRIDVECTOR & pos)
{
	if (optimalWeaponRange > 0)
	{
		//
		// set pivot point for attacks
		//
		Vector dir = transform.get_position();
		bool bLOS = (bNoLineOfSight || TestLOS(pos));     // true if we have lineOfSight on target
		SINGLE dirMag;

		dir -= (pos);	// vector from him to us
		dir.z = 0;
		dirMag = pos - GetGridPosition();

		if (bLOS && dirMag < optimalWeaponRange)
		{
			if (isMoveActive())
			{
				resetMoveVars();
				gotoPivot.zero();
			}
		}
		else if (bLOS==0 || dirMag > (GetWeaponRange()/GRIDSIZE) || (optimalFacingAngle <= 0 && dirMag >= optimalWeaponRange))
		{
			Vector pivot;
			if (bLOS)
			{
				pivot = dir;
				pivot *= ((optimalWeaponRange + box[5]/GRIDSIZE)/dirMag);
				pivot += pos;
			}
			else
			{
				pivot = pos;
			}

			gotoPivot = pivot;		// delay the move until a net work sync

			if (oldGotoPivot == gotoPivot)
				moveIdleTimer = EXT_MOVE_IDLE_TIMER;	// wait a long time next time if this happens
			else
				oldGotoPivot = gotoPivot;
			return true;
		}
	}

	return false;
}
//---------------------------------------------------------------------------
//
void Gunboat::save (GUNBOAT_SAVELOAD & save)
{
 	save.gunSaveLoad = *static_cast<BASE_GUNBOAT_SAVELOAD *>(this);
}
//---------------------------------------------------------------------------
//
void Gunboat::load (GUNBOAT_SAVELOAD & load)
{
 	*static_cast<BASE_GUNBOAT_SAVELOAD *>(this) = load.gunSaveLoad;
	netUnitStance = unitStance;
	netFighterStance = fighterStance;
	netTargetID = dwTargetID;
}
//---------------------------------------------------------------------------
//
void Gunboat::explodeBoat (bool bExplode)
{
	notifyAdmiralOfDestruction();
	destroyLaunchers();
}
//---------------------------------------------------------------------------
//
void Gunboat::preSelfDestruct (void)
{
	if (THEMATRIX->IsMaster())
		cancelAttack();		// must tell client side to cancel before death
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
// return TRUE if we have a clear LineOfSight with the target
//
bool Gunboat::checkLOS (TCallback & callback)	// with targetPos
{
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
/*
void Gunboat::findBetterTarget (void)
{
	// is there a better target?
	CQASSERT(!bUserGenerated);
	CQASSERT(target);
	CQASSERT(dwTargetID);


	// is our current target still close enough to attack?
	GRIDVECTOR targetGrid = target->GetGridPosition();
	GRIDVECTOR gridPos = GetGridPosition();
	SINGLE fDist = targetGrid - gridPos;

	// we want to take out the MPart part = it->obj section 
	// do it the gunplat too

	if (fDist > outerWeaponRange)
	{
		// see if a better target can be found
		IBaseObject * betterTarget = NULL;
		const U32 allyMask = MGlobals::GetAllyMask(dwMissionID & PLAYERID_MASK);
		SINGLE bestDistance = fDist + 1000.0f;
		SINGLE testDistance;

		ObjMapIterator it(systemID, transform.translation, fDist * GRIDSIZE,playerID);

		bool bChangeTarget;

		while (it)
		{
			MPart part = it->obj;

			if ((it->flags & OM_UNTOUCHABLE) == 0 && (it->flags & OM_TARGETABLE) && part.isValid() && part->bReady)
			{
				const U32 hisPlayerID = it.GetApparentPlayerID(allyMask);
					
				// if we are enemies
				if (hisPlayerID && (((1 << (hisPlayerID-1)) & allyMask) == 0))
				{
					// as long as the enemy is visible, let's go for it!
					if (it->obj->IsVisibleToPlayer(dwMissionID & PLAYERID_MASK))
					{
						const bool bLOS = (bNoLineOfSight || TestLOS(it->obj->GetGridPosition())); // true if we have lineOfSight on target

						// we need to have LOS
						if (bLOS)
						{
							testDistance = it->obj->GetGridPosition() - GetGridPosition();

							if (betterTarget)
							{
								// do not change target if the old target is a military ship and the new one is not
								MPart oldTarget = betterTarget;
								MPart newTarget = it->obj;

								// if the new target is a military ship with a better distance, then change targets
								if (MGlobals::IsMilitaryShip(newTarget->mObjClass) && testDistance < bestDistance)
								{
									bChangeTarget = true;
								}
								// if the old target is not a military ship and the new one is, then update target
								else if (!MGlobals::IsMilitaryShip(oldTarget->mObjClass) && MGlobals::IsMilitaryShip(newTarget->mObjClass))
								{
									bChangeTarget = true;
								}
								else
								{
									bChangeTarget = false;
								}

								if (bChangeTarget)
								{
									bestDistance = testDistance;
									betterTarget = it->obj;
								}
							}
							else
							{
								bestDistance = testDistance;
								betterTarget = it->obj;
							}
						}
					}
				}
			}
			++it;
		}

		// have we found a target that is closer!?
		if (betterTarget && betterTarget != target && bExploding == false)
		{
			betterTarget->QueryInterface(IBaseObjectID, target, playerID);
			dwTargetID = betterTarget->GetPartID();
			loseTargetTimer = LOSE_TARGET_TIMER;
		}
	}
}
*/
//---------------------------------------------------------------------------
//
void Gunboat::findClosestTarget (const GRIDVECTOR & pivot, SINGLE fDist)
{
	if (fDist <= 0)
		fDist = 64;		// a large number
	// attack the closest thing to you within the radius
	const U32 allyMask = MGlobals::GetAllyMask(dwMissionID & PLAYERID_MASK);
	IBaseObject * bestTarget = NULL;

	// make that for the attack stance we are looking for a target within *at least* 5 squares
	SINGLE distance = fDist * GRIDSIZE;
	if (unitStance == US_ATTACK && fDist < 5.0f)
	{
		distance = 5.0f * GRIDSIZE;
	}

	SINGLE bestDistance = distance;
	SINGLE testDistance;

	ObjMapIterator it(systemID, pivot, distance, playerID);

	bool bChangeTarget = true;

	while (it)
	{
		if ((it->flags & OM_UNTOUCHABLE) == 0 && (it->flags & OM_TARGETABLE))
		{
			const U32 hisPlayerID = it.GetApparentPlayerID(allyMask);

			// if we are enemies
			if (hisPlayerID && (((1 << (hisPlayerID-1)) & allyMask) == 0))
			{
				// as long as the enemy is visible, let's go for it!
				if (it->obj->IsVisibleToPlayer(dwMissionID & PLAYERID_MASK))
				{
					MPart part = it->obj;

					// don't auto-attack units that are under construction or are dead
					if (part.isValid() && part->bReady && part->hullPoints>0 && part->bNoAutoTarget==0)
					{
						const bool bLOS = (bNoLineOfSight || TestLOS(it->obj->GetGridPosition())); // true if we have lineOfSight on target

						// we need to have LOS
						if (bLOS)
						{
							testDistance = it->obj->GetGridPosition() - pivot;

							if (bestTarget)
							{
								// do not change target if the old target is a military ship and the new one is not
								MPart oldTarget = bestTarget;
								MPart newTarget = it->obj;


								// if the new target is a military ship with a better distance, then change targets
								if (MGlobals::IsMilitaryShip(newTarget->mObjClass) && testDistance < bestDistance)
								{
									bChangeTarget = true;
								}
								// if the old target is not a military ship and the new one is, then update target
								else if (!MGlobals::IsMilitaryShip(oldTarget->mObjClass) && MGlobals::IsMilitaryShip(newTarget->mObjClass))
								{
									bChangeTarget = true;
								}
								else
								{
									bChangeTarget = false;
								}

								if(MISSION->IsComputerControlled(playerID))//turn on smart ageis targeting......
								{
									if(bestTarget->effectFlags.bAgeisShield && !it->obj->effectFlags.bAgeisShield)
										bChangeTarget = true;
									else if((!bestTarget->effectFlags.bAgeisShield) && it->obj->effectFlags.bAgeisShield)
										bChangeTarget = false;
								}

								if (bChangeTarget)
								{
									bestDistance = testDistance;
									bestTarget = it->obj;
								}
							}
							else
							if (testDistance <= bestDistance)	// don't attack things out of range
							{
								bestDistance = testDistance;
								bestTarget = it->obj;
							}
						}
					}
				}
			}
		}
		++it;
	}

	// found a good target, hurray!?
	if (bestTarget && bestTarget != target && bExploding == false)
	{
		bestTarget->QueryInterface(IBaseObjectID, target, playerID);
		dwTargetID = bestTarget->GetPartID();
		loseTargetTimer = LOSE_TARGET_TIMER;
	}
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//
inline struct IBaseObject * createGunboat (const GUNBOAT_INIT & data)
{
	Gunboat * obj = new ObjectImpl<Gunboat>;

	obj->FRAME_init(data);
	return obj;
}
//------------------------------------------------------------------------------------------
//---------------------------Gunboat Factory------------------------------------------------
//------------------------------------------------------------------------------------------
//
struct DACOM_NO_VTABLE GunboatFactory : public IObjectFactory
{
	struct OBJTYPE : GUNBOAT_INIT
	{
		~OBJTYPE (void)
		{
			int i;

			for (i = 0; i < MAX_GUNBOAT_LAUNCHERS; i++)
				if (launchers[i])
					ARCHLIST->Release(launchers[i], OBJREFNAME);
		}
	};
	//------------------------------------------------------------------------------------------
	//
	U32 factoryHandle;		// handles to callback

	//
	// Interface mapping
	//

	BEGIN_DACOM_MAP_INBOUND(GunboatFactory)
	DACOM_INTERFACE_ENTRY(IObjectFactory)
	END_DACOM_MAP()

	GunboatFactory (void) { }

	~GunboatFactory (void);

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

	/* GunboatFactory methods */

	IDAComponent * getBase (void)
	{
		return static_cast<IObjectFactory *> (this);
	}
};
//--------------------------------------------------------------------------//
//
GunboatFactory::~GunboatFactory (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (OBJLIST && OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
		connection->Unadvise(factoryHandle);
}
//--------------------------------------------------------------------------//
//
void GunboatFactory::init (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
		connection->Advise(getBase(), &factoryHandle);
}
//-----------------------------------------------------------------------------
//
HANDLE GunboatFactory::CreateArchetype (const char *szArchname, OBJCLASS objClass, void *_data)
{
	OBJTYPE * result = 0;
	int i;

	if (objClass == OC_SPACESHIP)
	{
		BT_GUNBOAT_DATA * data = (BT_GUNBOAT_DATA *) _data;

		if (data->type == SSC_GUNBOAT)	   
		{ 
			int lastTexMem=0;
			result = new OBJTYPE();
			if (result->loadSpaceshipArchetype(data, ARCHLIST->GetArchetype(szArchname)) == false)
				goto Error;

			for (i = 0; i < MAX_GUNBOAT_LAUNCHERS; i++)
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
			GUNBOATTEXMEMUSED += (TEXMEMORYUSED-lastTexMem);
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
BOOL32 GunboatFactory::DestroyArchetype (HANDLE hArchetype)
{
	OBJTYPE * objtype = (OBJTYPE *)hArchetype;

	EditorStopObjectInsertion(objtype->pArchetype);
	delete objtype;

	GUNBOATTEXMEMUSED = 0;

	return 1;
}
//-------------------------------------------------------------------
//
IBaseObject * GunboatFactory::CreateInstance (HANDLE hArchetype)
{
	OBJTYPE * objtype = (OBJTYPE *)hArchetype;
	return createGunboat(*objtype);
}
//-------------------------------------------------------------------
//
void GunboatFactory::EditorCreateInstance (HANDLE hArchetype, const MISSION_INFO & info)
{
	OBJTYPE * objtype = (OBJTYPE *)hArchetype;
	EditorStartObjectInsertion(objtype->pArchetype, info);
}
//---------------------------------------------------------------------------
//-------------------------GLOBAL STARTUP------------------------------------
//---------------------------------------------------------------------------
//
struct _gunship : GlobalComponent
{
	GunboatFactory * sfactory;

	virtual void Startup (void)
	{
		sfactory = new DAComponent<GunboatFactory>;
		AddToGlobalCleanupList((IDAComponent **) &sfactory);
	}

	virtual void Initialize (void)
	{
		sfactory->init();
	}
};

static _gunship __ship;

//---------------------------------------------------------------------------
//----------------------------End Gunboat.cpp--------------------------------
//---------------------------------------------------------------------------

