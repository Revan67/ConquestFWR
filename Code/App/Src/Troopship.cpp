//--------------------------------------------------------------------------//
//                                                                          //
//                               Troopship.cpp                                //
//                                                                          //
//                  COPYRIGHT (C) 1997 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/Troopship.cpp 144   6/22/01 9:56a Tmauer $
*/			    
//------------------------------- #INCLUDES --------------------------------//
//--------------------------------------------------------------------------//

#include "pch.h"
#include <globals.h>

#include "ITroopship.h"
#include "TSpaceShip.h"
#include "Startup.h"
#include "ObjSet.h"

#include <DTroopship.h>
#include <DSpaceship.h>

#include <IConnection.h>
#include <DPlatform.h>

#include <EventSys.h>

#define SEND_OPERATION(x)   \
	{	TSHIP_NET_COMMANDS command = x;   \
		THEMATRIX->SendOperationData(attackAgentID, dwMissionID, &command, 1); }

#define POD_WAIT_PERIOD 4.0;

void __stdcall InitializeTroopPod (IBaseObject * _pod, IBaseObject * troopship, IBaseObject * target);
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
struct _NO_VTABLE Troopship : public SpaceShip<TROOPSHIP_SAVELOAD, TROOPSHIP_INIT>, ITroopship, BASE_TROOPSHIP_SAVELOAD
{
	BEGIN_MAP_INBOUND(Troopship)
	_INTERFACE_ENTRY(IBaseObject)
	_INTERFACE_ENTRY(IGotoPos)
	_INTERFACE_ENTRY(ISaveLoad)
	_INTERFACE_ENTRY(IQuickSaveLoad)
	_INTERFACE_ENTRY(IExplosionOwner)
	_INTERFACE_ENTRY(IMissionActor)
	_INTERFACE_ENTRY(IWeaponTarget)
	_INTERFACE_ENTRY(IBuild)
	_INTERFACE_ENTRY(IPhysicalObject)
	_INTERFACE_ENTRY(ICloak)
	_INTERFACE_ENTRY(IRepairee)
	_INTERFACE_ENTRY(ITroopship)
	_INTERFACE_ENTRY(IShipDamage)
	_INTERFACE_ENTRY(IExtent)
	_INTERFACE_ENTRY(IShipMove)
	_INTERFACE_ENTRY(IEffectTarget)
	END_MAP()
	//----------------------------------

	OBJPTR<IBaseObject> target;					// can now be enemy or friend

	PARCHETYPE pPodType;

	// sound bytes
	HSOUND hsndPodRelease;

	const BT_TROOPSHIP_DATA* troopData;

	UpdateNode			updateNode;
	SaveNode			saveNode;
	LoadNode			loadNode;
	ResolveNode			resolveNode;
	InitNode			initNode;
	ReceiveOpDataNode	receiveOpDataNode;
	OnOpCancelNode		onOpCancelNode;
	PreDestructNode		destructNode;

	struct TCallback : ITerrainSegCallback
	{
		GRIDVECTOR gridPos;

		TCallback (void)
		{
		}

		virtual bool TerrainCallback (const struct FootprintInfo & info, struct GRIDVECTOR & pos)
		{
			if (info.flags & (TERRAIN_IMPASSIBLE|TERRAIN_BLOCKLOS))
			{
				gridPos = pos;
				return false;
			}
			return true;
		}
	};
	//----------------------------------
	//----------------------------------
	
	Troopship (void);

	virtual ~Troopship (void);	

	/* IBaseObject methods */

	/* ITroopship methods */
	
	virtual void Capture (IBaseObject * victim, U32 agentID);	// moves to attack

	virtual bool IsCaptureOk (IBaseObject * victim);

	/* IWeaponTarget methods */


	/* IGotoPos methods */

	/* IMissionActor methods */

	virtual void OnOperationCreation (U32 agentID, void *buffer, U32 bufferSize);

	// called in response to OpAgent::SendOperationData()
	void receiveOperationData (U32 agentID, void *buffer, U32 bufferSize);

	virtual void OnMasterChange(bool bIsMaster)
	{
		repairMasterChange(bIsMaster);
	}

	/* SpaceShip methods */

	virtual const char * getSaveStructName (void) const		// must be overriden implemented by derived class
	{
		return "TROOPSHIP_SAVELOAD";
	}

	virtual void * getViewStruct (void) 	// must be overriden implemented by derived class
	{
		return static_cast<BASE_TROOPSHIP_SAVELOAD *>(this);
	}

	virtual bool testReadyForJump (void)
	{
		return true;
	}

	/* Troopship methods */

	void onOperationCancel (U32 agentID);

	BOOL32 updateTroopship (void);
	
	void save (TROOPSHIP_SAVELOAD & save);

	void load (TROOPSHIP_SAVELOAD & load);

	void resolve (void);

	void initTroopship (const TROOPSHIP_INIT & data);

	bool moveToTarget (bool bIsMaster);

	bool doAssault (bool bIsMaster);		// true if did the assault

	void localMovToPos (const Vector & _vec, U32 agentID=0)
	{
		GRIDVECTOR vec;
		vec = _vec;
		moveToPos(vec, agentID);
	}

	void preSelfDestruct (void);

	bool checkLOS (void);
};

//---------------------------------------------------------------------------
//
Troopship::Troopship (void) : 
			updateNode(this, UpdateProc(&Troopship::updateTroopship)),
			onOpCancelNode(this, OnOpCancelProc(&Troopship::onOperationCancel)),
			saveNode(this, CASTSAVELOADPROC(&Troopship::save)),
			loadNode(this, CASTSAVELOADPROC(&Troopship::load)),
			resolveNode(this, ResolveProc(&Troopship::resolve)),
			initNode(this, CASTINITPROC(&Troopship::initTroopship)),
			receiveOpDataNode(this, ReceiveOpDataProc(&Troopship::receiveOperationData)),
			destructNode(this, PreDestructProc(&Troopship::preSelfDestruct))
{
	// nothing to init?
}
//---------------------------------------------------------------------------
//
Troopship::~Troopship (void)
{
}
//---------------------------------------------------------------------------
// return TRUE if we have a clear LineOfSight with the target
//
bool Troopship::checkLOS (void)	// with targetPos
{
	COMPTR<ITerrainMap> map;
	TCallback callback;

	SECTOR->GetTerrainMap(GetSystemID(), map);
 
	if (map->TestSegment(GetGridPosition(), target.Ptr()->GetGridPosition(), &callback) == false)
	{
		return callback.gridPos.isMostlyEqual(target.Ptr()->GetGridPosition());
	}

	return true;
}
//---------------------------------------------------------------------------
//
BOOL32 Troopship::updateTroopship (void)
{
	if (attackAgentID)
	{
		bool bIsMaster = THEMATRIX->IsMaster();

		if (target==0 || target->GetSystemID() != systemID)
		{	
			if (bIsMaster)
			{
				SEND_OPERATION(TSHIP_CANCEL);
				target = 0;
				dwTargetID = 0;
				THEMATRIX->OperationCompleted2(attackAgentID, dwMissionID);
			}
		}
		else
		if ((rand() % 4) == 0 && moveToTarget(bIsMaster))		// close enough to takeover the target
		{
			if (bTakeoverApproved)		// work was already done on the host side
			{
				CQASSERT(bIsMaster==0);
				THEMATRIX->OperationCompleted2(attackAgentID, dwMissionID);
			}
			else
			if (bIsMaster)
			{
				// don't allow assault if unit is really dead
				if (hullPoints>0 && doAssault(bIsMaster))
				{
					THEMATRIX->FlushOpQueueForUnit(this);
					BANKER->FreeCommandPt(playerID,pInitData->resourceCost.commandPt);
					return 0;
				}
			}
		}
	}

	return 1;
}
//---------------------------------------------------------------------------
//
void Troopship::Capture (IBaseObject * victim, U32 agentID)
{
	CQASSERT(attackAgentID == 0);
	attackAgentID = agentID;
	bTakeoverApproved = 0;	

	if (victim)
		victim->QueryInterface(IBaseObjectID, target, playerID);
	else
		target=0;

	if (target != 0)
	{
		dwTargetID = victim->GetPartID();
		if (target->GetSystemID() == systemID)
			moveToPos(target->GetGridPosition());
	}
	else
		dwTargetID = 0;
}
//---------------------------------------------------------------------------
//
bool Troopship::IsCaptureOk (IBaseObject * victim)
{
	return MGlobals::CanTroopship(playerID,dwMissionID,victim->GetPartID());
}
//---------------------------------------------------------------------------
//
void Troopship::receiveOperationData (U32 agentID, void *buffer, U32 bufferSize)
{
	if (agentID == attackAgentID)
	{
		CQASSERT(bufferSize == 1);

		// get the command out of the buffer
		TSHIP_NET_COMMANDS command = static_cast<TSHIP_NET_COMMANDS>(((char*)buffer)[0]);

		switch (command)
		{
		case TSHIP_CANCEL:
			target = 0;
			dwTargetID = 0;
			THEMATRIX->OperationCompleted2(attackAgentID, dwMissionID);
			break;

		case TSHIP_APPROVED:
			bTakeoverApproved = true;		// complete operation when we get close
			break;

		case TSHIP_SUCCESS:
			bTakeoverApproved = false;
			doAssault(false);
			break;

		default:
			CQBOMB1("Received unknown data byte %d", command);
			break;
		}
	}
}
//---------------------------------------------------------------------------
//
bool Troopship::moveToTarget (bool bIsMaster)
{
	CQASSERT(target != 0 && attackAgentID);

	if (bIsMaster && target->IsVisibleToPlayer(playerID)==0 && isMoveActive()==0)
	{
		SEND_OPERATION(TSHIP_CANCEL);
		THEMATRIX->OperationCompleted2(attackAgentID, dwMissionID);
		return false;
	}
	else
	if (target->IsVisibleToPlayer(playerID) || bIsMaster==0)
	{
		GRIDVECTOR targetPos = target->GetGridPosition();
		moveToPos(targetPos);
		bool bLOS = checkLOS();
		// if move ended and we don't have a clear path to the target
		if (isMoveActive()==0 && bLOS==0)
		{
			if (bIsMaster)
			{
				SEND_OPERATION(TSHIP_CANCEL);
				THEMATRIX->OperationCompleted2(attackAgentID, dwMissionID);
				return false;
			}
		}
		else
		if (isMoveActive()==0 || (bLOS && targetPos - GetGridPosition() < troopData->assaultRange))
		{
			if (bIsMaster)
			{
				SEND_OPERATION(TSHIP_APPROVED);
				THEMATRIX->OperationCompleted2(attackAgentID, dwMissionID);
			}
			return true;
		}
	}

	return false;
}
//---------------------------------------------------------------------------
//	we're commited at this point
//
bool Troopship::doAssault (bool bIsMaster)
{
	CQASSERT(target != NULL && target->GetSystemID() == systemID && (bIsMaster==0 || attackAgentID==0));

	// do the takeover
	VOLPTR(IMissionActor) victim = target;
	CQASSERT(victim != 0);

	MPartNC part = target;

	if (bIsMaster && ((target->GetPlayerID() == playerID) || (!(part->bReady))) )
	{
		// fail, kill self
		THEMATRIX->ObjectTerminated(dwMissionID, 0);
		return false;
	}

	U32 newDwTargetID = (dwTargetID & ~0xF) | playerID;
	U32 targetPlayerID = MGlobals::GetPlayerFromPartID(dwTargetID);

	if (bIsMaster)
	{
		bool takeoverFailed = false;
		victim->PrepareTakeover(newDwTargetID, dwMissionID);
		if(THEMATRIX->GetWorkingOp(victim.Ptr()))
			takeoverFailed = true;
		else
		{
			THEMATRIX->FlushOpQueueForUnit(victim.Ptr());
			if (part->admiralID)
				THEMATRIX->FlushOpQueueForUnit(OBJLIST->FindObject(part->admiralID));
		}

		if(takeoverFailed)
		{
			// fail, kill self
			THEMATRIX->ObjectTerminated(dwMissionID, 0);
			return false;
		}
	}

	// about to scuttle?
	if (part->admiralID && (playerID == MGlobals::GetThisPlayer()))
	{
		FLAGSHIPSCUTTLE(OBJLIST->FindObject(part->admiralID), part->admiralID, scuttle, SUB_SCUTTLE, part.pInit->displayName);
	}

	OBJLIST->UnselectObject(target);

	if (bIsMaster)
	{
		ObjSet set;
		set.numObjects = 3;
		set.objectIDs[0] = dwMissionID;
		set.objectIDs[1] = dwTargetID;
		set.objectIDs[2] = newDwTargetID;
		if (part->admiralID)
			set.objectIDs[set.numObjects++] = part->admiralID;

		// get n'sync
		attackAgentID = THEMATRIX->CreateOperation(set, NULL, 0);
		bTakeoverApproved = false;
		SEND_OPERATION(TSHIP_SUCCESS);
	}
	else //on the clients the platfroms will need to change there build slots
	{
		if(victim.Ptr()->objClass == OC_PLATFORM)
		{
			OBJPTR<IPlatform> platform;
			victim.Ptr()->QueryInterface(IPlatformID,platform);
			if(platform)
			{
				platform->ClientSideTakeover(newDwTargetID);
			}
		}
	}
	
	if (bIsMaster)
	{
		THEMATRIX->OperationCompleted(attackAgentID, dwTargetID);
		THEMATRIX->OperationCompleted(attackAgentID, newDwTargetID);
		if (part->admiralID)
			THEMATRIX->OperationCompleted(attackAgentID, part->admiralID);
		THEMATRIX->OperationCompleted2(attackAgentID, dwMissionID);
		
		if (part->admiralID)
		{
			THEMATRIX->ObjectTerminated(part->admiralID, 0);
			THEMATRIX->ObjectTerminated(part->dwMissionID, 0);
		}
		else
		{
			// tell mission scripts that unit is captured
			MScript::RunProgramsWithEvent(CQPROGFLAG_TROOPSHIPED, part->dwMissionID);
			// tell playerAI that unit is captured
			EVENTSYS->Send(CQE_OBJECT_PRECAPTURE, (void *) part->dwMissionID);

			// change the player ID of the victim
			victim->TakeoverSwitchID(newDwTargetID);
			EVENTSYS->Send(CQE_OBJECT_POSTCAPTURE, (void *) newDwTargetID);
		}
	}
	else	// bIsMaster==0    (must do this in reverse order on client side)
	{
		if (part->admiralID==0)
		{
			victim->TakeoverSwitchID(newDwTargetID);
			THEMATRIX->SwitchPendingDeath(dwTargetID, newDwTargetID);
		}

		THEMATRIX->OperationCompleted(attackAgentID, dwTargetID);
		THEMATRIX->OperationCompleted(attackAgentID, newDwTargetID);
		if (part->admiralID)
			THEMATRIX->OperationCompleted(attackAgentID, part->admiralID);
		THEMATRIX->OperationCompleted2(attackAgentID, dwMissionID);
	}

	if (bIsMaster==0)
	{
		OBJLIST->DeferredDestruction(dwMissionID);
	}

	//
	// do some more scoring
	//

	if (victim.Ptr()->objClass == OC_PLATFORM)
	{
		// the victim loses the platform and we get a point for conversion
		MGlobals::SetPlatformsLost(targetPlayerID, MGlobals::GetPlatformsLost(targetPlayerID)+1);
		MGlobals::SetPlatformsConverted(playerID, MGlobals::GetPlatformsConverted(playerID)+1);
	}
	else 
	if (part->admiralID==0)
	{
		// the victim loses the unit and we get a point for unit conversion
		MGlobals::SetUnitsLost(targetPlayerID, MGlobals::GetUnitsLost(targetPlayerID)+1);
		MGlobals::SetUnitsConverted(playerID, MGlobals::GetUnitsConverted(playerID)+1);
	}

	if (hsndPodRelease==0)
		hsndPodRelease = SFXMANAGER->Open(troopData->sfxPodRelease);
	SFXMANAGER->Play(hsndPodRelease, systemID, &transform.translation);

	if (part->admiralID==0)
	{
		IBaseObject * pod = ARCHLIST->CreateInstance(pPodType);

		if (pod)
		{
			// pod code will change unit color when it completes
			InitializeTroopPod(pod, this, part.obj);
			OBJLIST->AddObject(pod);
		}
		else
		{
			part->playerID = (part->dwMissionID & PLAYERID_MASK);	// make sure color matches the missionID
		}
	}

	return true;
}
//---------------------------------------------------------------------------
//
void Troopship::OnOperationCreation (U32 agentID, void *buffer, U32 bufferSize)
{
	CQASSERT(attackAgentID == 0);

	attackAgentID = agentID;
	bTakeoverApproved = 0;		// really in second stage now
}
//---------------------------------------------------------------------------
//
void Troopship::onOperationCancel (U32 agentID)
{
	if (agentID == attackAgentID)
	{
		attackAgentID = 0;
		target = 0;
		dwTargetID = 0;
		bTakeoverApproved = 0;
	}
}
//---------------------------------------------------------------------------
//
void Troopship::preSelfDestruct (void)
{
	if (attackAgentID)
		onOperationCancel(attackAgentID);
}
//---------------------------------------------------------------------------
//
void Troopship::resolve (void)
{
	if (dwTargetID)
		OBJLIST->FindObject(dwTargetID, playerID, target);
}
//---------------------------------------------------------------------------
//
void Troopship::initTroopship (const TROOPSHIP_INIT & data)
{
	troopData = data.pData;
}
//---------------------------------------------------------------------------
//
void Troopship::save (TROOPSHIP_SAVELOAD & save)
{
	save.troopSaveLoad = *static_cast<BASE_TROOPSHIP_SAVELOAD *>(this);
}
//---------------------------------------------------------------------------
//
void Troopship::load (TROOPSHIP_SAVELOAD & load)
{
 	*static_cast<BASE_TROOPSHIP_SAVELOAD *>(this) = load.troopSaveLoad;
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//
inline struct Troopship * createTroopship (const TROOPSHIP_INIT & data)
{
	Troopship * obj = new ObjectImpl<Troopship>;

	obj->FRAME_init(data);
	return obj;
}
//------------------------------------------------------------------------------------------
//---------------------------Troopship Factory------------------------------------------------
//------------------------------------------------------------------------------------------
//
struct DACOM_NO_VTABLE TroopshipFactory : public IObjectFactory
{
	struct OBJTYPE : TROOPSHIP_INIT
	{
		PARCHETYPE podType;

		~OBJTYPE (void)
		{
			if (podType)
				ARCHLIST->Release(podType, OBJREFNAME);
		}
	};
	//------------------------------------------------------------------------------------------
	//
	U32 factoryHandle;		// handles to callback

	//
	// Interface mapping
	//

	BEGIN_DACOM_MAP_INBOUND(TroopshipFactory)
	DACOM_INTERFACE_ENTRY(IObjectFactory)
	END_DACOM_MAP()

	TroopshipFactory (void) { }

	~TroopshipFactory (void);

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

	/* TroopshipFactory methods */

	IDAComponent * getBase (void)
	{
		return static_cast<IObjectFactory *> (this);
	}
};
//--------------------------------------------------------------------------//
//
TroopshipFactory::~TroopshipFactory (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (OBJLIST && OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
		connection->Unadvise(factoryHandle);
}
//--------------------------------------------------------------------------//
//
void TroopshipFactory::init (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
		connection->Advise(getBase(), &factoryHandle);
}
//-----------------------------------------------------------------------------
//
HANDLE TroopshipFactory::CreateArchetype (const char *szArchname, OBJCLASS objClass, void *_data)
{
	OBJTYPE * result = 0;

	if (objClass == OC_SPACESHIP) 
	{
		BT_TROOPSHIP_DATA * data = (BT_TROOPSHIP_DATA *) _data;

		if (data->type == SSC_TROOPSHIP)	   
		{
			char buffer[GT_PATH];
			const char * ptr = strrchr(szArchname, '!');
			if (ptr)
				ptr++;
			else
				ptr = szArchname;
			strcpy(buffer, "TPOD!!");
			strcat(buffer, ptr);		// buffer should be the name of the pod type

			// pre-load the sounds
			SFXMANAGER->Preload(data->sfxPodRelease);

			result = new OBJTYPE();
			result->podType = ARCHLIST->LoadArchetype(buffer);
			if (result->podType)
				ARCHLIST->AddRef(result->podType, OBJREFNAME);

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
BOOL32 TroopshipFactory::DestroyArchetype (HANDLE hArchetype)
{
	OBJTYPE * objtype = (OBJTYPE *)hArchetype;

	EditorStopObjectInsertion(objtype->pArchetype);
	delete objtype;

	return 1;
}
//-------------------------------------------------------------------
//
IBaseObject * TroopshipFactory::CreateInstance (HANDLE hArchetype)
{
	OBJTYPE * objtype = (OBJTYPE *)hArchetype;
	Troopship * result = createTroopship(*objtype);
	result->pPodType = objtype->podType;

	return result;
}
//-------------------------------------------------------------------
//
void TroopshipFactory::EditorCreateInstance (HANDLE hArchetype, const MISSION_INFO & info)
{
	OBJTYPE * objtype = (OBJTYPE *)hArchetype;
	EditorStartObjectInsertion(objtype->pArchetype, info);
}
//---------------------------------------------------------------------------
//-------------------------GLOBAL STARTUP------------------------------------
//---------------------------------------------------------------------------
//
struct _troopship : GlobalComponent
{
	TroopshipFactory * sfactory;

	virtual void Startup (void)
	{
		sfactory = new DAComponent<TroopshipFactory>;
		AddToGlobalCleanupList((IDAComponent **) &sfactory);
	}

	virtual void Initialize (void)
	{
		sfactory->init();
	}
};

static _troopship __ship;

//---------------------------------------------------------------------------
//----------------------------End Troopship.cpp--------------------------------
//---------------------------------------------------------------------------