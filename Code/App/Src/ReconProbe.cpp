//--------------------------------------------------------------------------//
//                                                                          //
//                             ReconProbe.cpp                               //
//                                                                          //
//                  COPYRIGHT (C) 1997 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/ReconProbe.cpp 61    8/23/01 1:53p Tmauer $
*/			    
//------------------------------- #INCLUDES --------------------------------//
//--------------------------------------------------------------------------//

#include "pch.h"
#include <globals.h>

#include "TObjTrans.h"
#include "TObjControl.h"
#include "TObjFrame.h"
#include "TSpaceShip.h"
#include <DSpaceShip.h>
#include "TObject.h"
#include "Objlist.h"
#include "Startup.h"
#include "Mission.h"
#include "MPart.h"
#include "IRecon.h"
#include "CommPacket.h"

#include <TComponent.h>
#include <IConnection.h>
#include <TSmartPointer.h>

struct _NO_VTABLE ReconProbe : public SpaceShip<RECONPROBE_SAVELOAD, RECONPROBE_INIT>, IReconProbe, BASE_RECONPROBE_SAVELOAD
{
	BEGIN_MAP_INBOUND(ReconProbe)
	_INTERFACE_ENTRY(IBaseObject)
	_INTERFACE_ENTRY(IGotoPos)
	_INTERFACE_ENTRY(ISaveLoad)
	_INTERFACE_ENTRY(IQuickSaveLoad)
	_INTERFACE_ENTRY(IExplosionOwner)
	_INTERFACE_ENTRY(IMissionActor)
	_INTERFACE_ENTRY(IWeaponTarget)
	_INTERFACE_ENTRY(IBuild)
	_INTERFACE_ENTRY(IPhysicalObject)
	_INTERFACE_ENTRY(IReconProbe)
	_INTERFACE_ENTRY(IExtent)
	_INTERFACE_ENTRY(IShipMove)
	_INTERFACE_ENTRY(ICloak)
	END_MAP()

	typedef INITINFO RECONPROBEINITINFO;

	UpdateNode updateNode;
	SaveNode	saveNode;
	LoadNode	loadNode;
//	InitNode	initNode;
//	RenderNode  renderNode;
	OnOpCancelNode	onOpCancelNode;
	GeneralSyncNode  genSyncNode;
		
	OBJPTR<IReconLauncher> ownerLauncher;

	ReconProbe (void);

	virtual ~ReconProbe (void);

	virtual void CastVisibleArea (void);

	/*IReconProbe methods */

	virtual void InitRecon(IReconLauncher * _ownerLauncher, U32 _dwMissionID);

	virtual void ResolveRecon(IBaseObject * _ownerLauncher);

	virtual void LaunchProbe (IBaseObject * owner, const class TRANSFORM & orientation, const class Vector * pos,
		U32 targetSystemID, IBaseObject * jumpTarget);

	virtual void ExplodeProbe();

	virtual void DeleteProbe();

	virtual bool IsActive();

	virtual void ReconSwitchID(U32 newOwnerID)
	{
		CQASSERT(THEMATRIX->IsMaster() && "Can't do this multilayer");
		TakeoverSwitchID((dwMissionID & (~PLAYERID_MASK)) | (newOwnerID&PLAYERID_MASK));
	}

	/* IGotoPos methods */
	
	virtual void GotoPosition (const GRIDVECTOR & pos, U32 agentID, bool bSlowMove);

	virtual void PrepareForJump (IBaseObject * jumpgate, bool bUserMove, U32 agentID, bool bSlowMove);


	/* SpaceShip methods */
	
	virtual const char * getSaveStructName (void) const		// must be overriden implemented by derived class
	{
		return "RECONPROBE_SAVELOAD";
	}
	virtual void * getViewStruct (void) 	// must be overriden implemented by derived class
	{
		return static_cast<BASE_RECONPROBE_SAVELOAD *>(this);
	}

	/* IWeaponTarget methods */
	
	//returns true if a shield hit was created
	virtual BOOL32 ApplyDamage (IBaseObject * collider, U32 ownerID, const Vector & pos, const Vector &dir,U32 amount,BOOL32 bShieldHit=1);

	// move "amount" from the pending pile to the actual. (assumes complex formula has already been used)
	virtual void ApplyAOEDamage (U32 ownerID, U32 damageAmount);

	/* IMissionActor methods */

	virtual U32 GetPrioritySyncData (void * buffer)
	{
		if (!bGone)
			return FRAME_getPrioritySyncData(buffer);
		else
			return 0;
	}
	virtual U32 GetGeneralSyncData (void * buffer)
	{
		if (!bGone)
			return FRAME_getGeneralSyncData(buffer);
		else
			return 0;
	}

	void onOpCancel (U32 agentID);

	/*ISaveLoad */

	virtual void QuickSave (struct IFileSystem * file){};

	virtual void QuickLoad (const char *szSaveLoadType, const char *szInstanceName, void * buffer, U32 bufferSize){};

	virtual void QuickResolveAssociations (void){};

	/* ReconProbe methods */

	void saveProbe (RECONPROBE_SAVELOAD & save);
	void loadProbe (RECONPROBE_SAVELOAD & load);
	BOOL32 update (void);

	U32 getSyncData (void * buffer);
	void putSyncData (void * buffer, U32 bufferSize, bool bLateDelivery);
};
//---------------------------------------------------------------------------
//
ReconProbe::ReconProbe (void) : 
		saveNode(this, CASTSAVELOADPROC(&ReconProbe::saveProbe)),
		loadNode(this, CASTSAVELOADPROC(&ReconProbe::loadProbe)),
		updateNode(this, UpdateProc(&ReconProbe::update)),
		onOpCancelNode(this, OnOpCancelProc(&ReconProbe::onOpCancel)),
		genSyncNode(this, SyncGetProc(&ReconProbe::getSyncData), SyncPutProc(&ReconProbe::putSyncData))
{
}
//---------------------------------------------------------------------------
//
ReconProbe::~ReconProbe (void)
{
}
//----------------------------------------------------------------------------------
//
void ReconProbe::GotoPosition (const GRIDVECTOR & pos, U32 agentID, bool bSlowMove)
{
	goal = pos;

	goal.z = 2500;

	bMoving = true;
	bJumping = false;

	workingID = agentID;
}
//----------------------------------------------------------------------------------
//
void ReconProbe::PrepareForJump (IBaseObject * jumpgate, bool bUserMove, U32 agentID, bool bSloeMove)
{
	goal = jumpgate->GetPosition();

	goal.z = 2500;

	bJumping = true;
	bMoving = false;

	workingID = agentID;
}
//----------------------------------------------------------------------------------
//
void ReconProbe::onOpCancel (U32 agentID)
{
	if(agentID == workingID)
	{
		bMoving = false;
		bJumping = false;
		workingID = 0;
	}
}
//---------------------------------------------------------------------------
//
BOOL32 ReconProbe::ApplyDamage (IBaseObject * collider, U32 _ownerID, const Vector & pos, const Vector &dir,U32 amount,BOOL32 bShieldHit)
{
	SYSMAP->RegisterAttack(systemID,GetGridPosition(),playerID);
	BOOL32 result = 0;

	amount = MGlobals::GetEffectiveDamage(amount, OBJLIST->FindObject(_ownerID), this,_ownerID);

	if (MGlobals::AreAllies(MGlobals::GetPlayerFromPartID(_ownerID), playerID) == 0)
	{
		FLEETSHIP_UNDERATTACK;
		broadcastHelpMsg(_ownerID);
	}

	if (displayHull < 0)		// if uninitialized
		displayHull = hullPoints;

	if (THEMATRIX->IsMaster())
	{
		if (S32(hullPoints - amount) <= 0)
		{
			if (displayHull <= S32(amount))
			{
				if(ownerLauncher && !bGone)
					ownerLauncher->KillProbe(dwMissionID);
			}
			else
				hullPoints = 1;
		}
		else
			hullPoints -= amount;
	}
	
	Vector collide_pos(0,0,0);
	if (hullPoints < 0.3*hullPointsMax || !bShieldsUp)
	{
		BOOL32 bHit;
		Vector norm;
		bHit = GetCollisionPosition(collide_pos,norm,pos,dir);
		if(bHit)
			collide_pos = transform.inverse_rotate_translate(collide_pos);
	}
	else
	{
		//collide_pos is in object space coords
		if (bShieldHit)
		{
			CreateShieldHit(pos,dir,collide_pos,amount);
		}
		result = 1;
	}
	RegisterDamage(collide_pos,amount);

	return result;
}
//---------------------------------------------------------------------------
//
void ReconProbe::ApplyAOEDamage (U32 _ownerID, U32 amount)
{
	SYSMAP->RegisterAttack(systemID,GetGridPosition(),playerID);
	amount = MGlobals::GetEffectiveDamage(amount, OBJLIST->FindObject(_ownerID), this, _ownerID);

	if (_ownerID && MGlobals::AreAllies(MGlobals::GetPlayerFromPartID(_ownerID), playerID) == 0)
	{
		FLEETSHIP_UNDERATTACK;
		broadcastHelpMsg(_ownerID);
	}

	if (displayHull < 0)		// if uninitialized
		displayHull = hullPoints;

	if (THEMATRIX->IsMaster() && ownerLauncher && !bGone)
	{
		if (S32(hullPoints - amount) <= 0)
		{
			ownerLauncher->KillProbe(dwMissionID);
		}
		else
			hullPoints -= amount;
	}

	//unsatisfactory
	RegisterDamage(Vector(0,0,0),amount);
}
//----------------------------------------------------------------------------------
//
BOOL32 ReconProbe::update()
{
	if(bMoving)
	{
		Vector targPos = goal-transform.translation;
		SINGLE relYaw = get_angle(targPos.x,targPos.y) - transform.get_yaw();
		relYaw = fixAngle(relYaw);
		rotateShip(relYaw,0-transform.get_roll(),0-transform.get_pitch());
		if (setPosition(targPos))
		{
			THEMATRIX->OperationCompleted2(workingID,dwMissionID);
		}
	}
	else if (bJumping)
	{
		Vector targPos = goal-transform.translation;
		setPosition(targPos);

		SINGLE relYaw = get_angle(targPos.x,targPos.y) - transform.get_yaw();
		relYaw = fixAngle(relYaw);
		rotateShip(relYaw,0-transform.get_roll(),0-transform.get_pitch());

		SINGLE dist = targPos.magnitude();
		SINGLE outer = (boxRadius*4) + getDynamicsData().maxLinearVelocity*4;
		if (dist < outer)
		{
			bJumping = false;
			THEMATRIX->OperationCompleted2(workingID,dwMissionID);
		}
	}

	if(((probeTimer -= ELAPSED_TIME) <= 0) && (!bGone))
	{
		if(THEMATRIX->IsMaster())
		{
			bGone = true;
			ownerLauncher->KillProbe(dwMissionID);
		}
	}

	return 1;
}
//----------------------------------------------------------------------------------
//
U32 ReconProbe::getSyncData (void * buffer)
{
	if(bNoMoreSync)
		return 0;
	if(bLauncherDelete)
	{
		bNoMoreSync = true;
		bLauncherDelete = false;
		OBJLIST->DeferredDestruction(dwMissionID);		
		return 1;
	}
	return 0;
}
//----------------------------------------------------------------------------------
//
void ReconProbe::putSyncData (void * buffer, U32 bufferSize, bool bLateDelivery)
{
	if(bufferSize == 1)
	{
		bLauncherDelete = false;
		OBJLIST->DeferredDestruction(dwMissionID);
	}
}
//----------------------------------------------------------------------------------
//
void ReconProbe::CastVisibleArea (void)
{
	if(systemID)
	{
		SpaceShip<RECONPROBE_SAVELOAD, RECONPROBE_INIT>::CastVisibleArea();
	}
}
//----------------------------------------------------------------------------------
//
void ReconProbe::InitRecon(IReconLauncher * _ownerLauncher, U32 _dwMissionID)
{
	_ownerLauncher->QueryInterface(IReconLauncherID,ownerLauncher, NONSYSVOLATILEPTR);
	dwMissionID = _dwMissionID;
	
	playerID = MGlobals::GetPlayerFromPartID(dwMissionID);
	sprintf(partName,"ReconProbe 0x%x",dwMissionID);

	OBJLIST->AddPartID(this, dwMissionID);
	OBJLIST->AddObject(this);

	MGlobals::UpgradeMissionObj(this);
	bGone = true;
	SetReady(false);
	bNoMoreSync = false;
}
//----------------------------------------------------------------------------------
//
void ReconProbe::ResolveRecon(IBaseObject * _ownerLauncher)
{
	_ownerLauncher->QueryInterface(IReconLauncherID,ownerLauncher, NONSYSVOLATILEPTR);
}
//----------------------------------------------------------------------------------
//
void ReconProbe::LaunchProbe (IBaseObject * owner, const class TRANSFORM & orientation, const class Vector * pos,U32 targetSystemID, IBaseObject * jumpTarget)
{
	CQASSERT (pos);
	
	probeTimer = totalTime;

	Vector vel;
	TRANSFORM orient = orientation;

	bGone = false;

	systemID = owner->GetSystemID();
	playerID = owner->GetPlayerID();

	transform = orient;

	SetTransform(transform, systemID);//silly but needed for gridvector stuff I don't use

	hullPoints = hullPointsMax;

	SetReady(true);

	if(THEMATRIX->IsMaster())
	{
		USR_PACKET<USRMOVE> packet;

		packet.objectID[0] = GetPartID();
		packet.position.init(*pos,targetSystemID);
		packet.init(1);
		NETPACKET->Send(HOSTID,0,&packet);
	}
}
//----------------------------------------------------------------------------------
//
void ReconProbe::ExplodeProbe()
{
	if (bVisible)
	{
		RECONPROBE_INIT * rArch = (RECONPROBE_INIT *)(ARCHLIST->GetArchetypeHandle(pArchetype));
		if(rArch->pBlastType)
		{
			IBaseObject * blast = CreateBlast(rArch->pBlastType, transform, systemID);

			if (blast)
				OBJLIST->AddObject(blast);
		}
	}
	systemID = 0;
	OBJLIST->UnselectObject(this);
	if(bHighlight)
	{
		OBJLIST->FlushHighlightedList();
	}
	bGone = true;
	SetReady(false);
}
//----------------------------------------------------------------------------------
//
void ReconProbe::DeleteProbe()
{
	bLauncherDelete = true;
}
//----------------------------------------------------------------------------------
//
bool ReconProbe::IsActive()
{
	return !bGone;
}
//---------------------------------------------------------------------------
//
void ReconProbe::saveProbe (RECONPROBE_SAVELOAD & save)
{
	save.baseSaveLoad = *static_cast<BASE_RECONPROBE_SAVELOAD *>(this);
}
//---------------------------------------------------------------------------
//
void ReconProbe::loadProbe (RECONPROBE_SAVELOAD & load)
{
	*static_cast<BASE_RECONPROBE_SAVELOAD *>(this) = load.baseSaveLoad;
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//
struct IBaseObject * createReconProbe (const RECONPROBE_INIT & data)
{
	ReconProbe * obj = new ObjectImpl<ReconProbe>;

 	obj->FRAME_init(data);
	obj->totalTime = data.pData->lifeTime;

	MPartNC part(obj);

	part->mObjClass = data.pData->missionData.mObjClass;
	part->race = data.pData->missionData.race;
	part->caps = data.pData->missionData.caps;

	part->hullPointsMax   = data.pData->missionData.hullPointsMax  ;
	part->supplyPointsMax = data.pData->missionData.supplyPointsMax ;
	part->maxVelocity = data.pData->missionData.maxVelocity ;

	return obj;
}
//------------------------------------------------------------------------------------------
//--------------------------ReconProbe Factory----------------------------------------------
//------------------------------------------------------------------------------------------
//
struct DACOM_NO_VTABLE ReconProbeFactory : public IObjectFactory
{
	struct OBJTYPE : RECONPROBE_INIT
	{
		~OBJTYPE (void)
		{
			if(pBlastType)
			{
				ARCHLIST->Release(pBlastType, OBJREFNAME);
				pBlastType = NULL;
			}
		}
	};
	//------------------------------------------------------------------------------------------
	//
	U32 factoryHandle;		// handles to callback

	//
	// Interface mapping
	//

	BEGIN_DACOM_MAP_INBOUND(ReconProbeFactory)
	DACOM_INTERFACE_ENTRY(IObjectFactory)
	END_DACOM_MAP()

	ReconProbeFactory (void) { }

	~ReconProbeFactory (void);

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

	/* ReconProbeFactory methods */

	IDAComponent * getBase (void)
	{
		return static_cast<IObjectFactory *> (this);
	}
};
//--------------------------------------------------------------------------//
//
ReconProbeFactory::~ReconProbeFactory (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (OBJLIST && OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
		connection->Unadvise(factoryHandle);
}
//--------------------------------------------------------------------------//
//
void ReconProbeFactory::init (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
		connection->Advise(getBase(), &factoryHandle);
}
//-----------------------------------------------------------------------------
//
HANDLE ReconProbeFactory::CreateArchetype (const char *szArchname, OBJCLASS objClass, void *_data)
{
	OBJTYPE * result = 0;

	if (objClass == OC_SPACESHIP)
	{
		BT_RECONPROBE_DATA * data = (BT_RECONPROBE_DATA *) _data;

		if (data->type == SSC_RECONPROBE)	   
		{
			result = new OBJTYPE;
			if (result->loadSpaceshipArchetype(data, ARCHLIST->GetArchetype(szArchname)) == false)
				goto Error;

			if(data->blastType[0])
			{
				result->pBlastType = ARCHLIST->LoadArchetype(data->blastType);
				if(result->pBlastType)
					ARCHLIST->AddRef(result->pBlastType, OBJREFNAME);
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
BOOL32 ReconProbeFactory::DestroyArchetype (HANDLE hArchetype)
{
	OBJTYPE * objtype = (OBJTYPE *)hArchetype;

	EditorStopObjectInsertion(objtype->pArchetype);
	delete objtype;

	return 1;
}
//-------------------------------------------------------------------
//
IBaseObject * ReconProbeFactory::CreateInstance (HANDLE hArchetype)
{
	OBJTYPE * objtype = (OBJTYPE *)hArchetype;
	return createReconProbe(*objtype);
}
//-------------------------------------------------------------------
//
void ReconProbeFactory::EditorCreateInstance (HANDLE hArchetype, const MISSION_INFO & info)
{
	OBJTYPE * objtype = (OBJTYPE *)hArchetype;
	EditorStartObjectInsertion(objtype->pArchetype, info);
}
//---------------------------------------------------------------------------
//-------------------------GLOBAL STARTUP------------------------------------
//---------------------------------------------------------------------------
//
struct _rpfactory : GlobalComponent
{
	ReconProbeFactory * rpfactory;

	virtual void Startup (void)
	{
		rpfactory = new DAComponent<ReconProbeFactory>;
		AddToGlobalCleanupList((IDAComponent **) &rpfactory);
	}

	virtual void Initialize (void)
	{
		rpfactory->init();
	}
};

static _rpfactory __rpfactory;

//---------------------------------------------------------------------------
//--------------------------End ReconProbe.cpp--------------------------------
//---------------------------------------------------------------------------