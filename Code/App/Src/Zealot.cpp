//--------------------------------------------------------------------------//
//                                                                          //
//                                 Zealot.cpp                                //
//                                                                          //
//                  COPYRIGHT (C) 1997 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/Zealot.cpp 48    9/13/01 10:01a Tmauer $
*/			    
//------------------------------- #INCLUDES --------------------------------//
//--------------------------------------------------------------------------//

#include "pch.h"
#include <globals.h>

#include "TObject.h"
#include "IConnection.h"
#include "Camera.h"
#include "DEffect.h"
#include "Objlist.h"
#include "Sfx.h"
#include "TObjTrans.h"
#include "TObjFrame.h"
#include "TObjPhys.h"
#include "Startup.h"
#include "IWeapon.h"
#include "IFighter.h"
#include "ILauncher.h"
#include "Mission.h"
#include "ICloak.h"
#include "ArchHolder.h"
#include "MGlobals.h"
#include "MPart.h"
#include "Sector.h"
#include "CQLight.h"
#include "OpAgent.h"
#include "IUnbornMeshList.h"
#include <DSpecial.h>
#include <DMBaseData.h>
#include "MeshRender.h"

#include <TSmartPointer.h>
#include <Mesh.h>
#include <FileSys.h>
#include <Engine.h>
#include <IRenderPrimitive.h>
#include <Renderer.h>



struct ZealotArchetype
{
	const char *name;
	BT_ZEALOT_DATA *data;
	S32 archIndex;
	IMeshArchetype * meshArch;
	PARCHETYPE pImpactBlast;
	Vector rigidBodyArm;

    void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}

	ZealotArchetype (void)
	{
		meshArch = NULL;
		archIndex = INVALID_INSTANCE_INDEX;
	}

	~ZealotArchetype (void)
	{
		ENGINE->release_archetype(archIndex);
		if (pImpactBlast)
			ARCHLIST->Release(pImpactBlast, OBJREFNAME);
	}

};

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//

struct _NO_VTABLE Zealot : public ObjectPhysics<ObjectTransform<ObjectFrame<IBaseObject,ZEALOT_SAVELOAD,ZealotArchetype> > >,  ISaveLoad, ILauncher, BASE_ZEALOT_SAVELOAD
{

	BEGIN_MAP_INBOUND(Zealot)
	_INTERFACE_ENTRY(IBaseObject)
	_INTERFACE_ENTRY(ISaveLoad)
	_INTERFACE_ENTRY(ILauncher)
	END_MAP()

	//------------------------------------------

	BT_ZEALOT_DATA *data;
	OBJPTR<IWeaponTarget> target;
	ZealotArchetype *arch;

	OBJPTR<ILaunchOwner> owner;

//	U32 time;  //temp

	bool bDeleteRequested:1;

	//
	// sync info
	// 
	struct SYNC_PACKET
	{
		U32 targetID;
	};
	
	//------------------------------------------

	Zealot (void)
	{
		transform = TRANSFORM::WORLD;
	}

    void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}

	virtual ~Zealot (void);	// See ObjList.cpp

	/* IBaseObject methods */

	virtual void CastVisibleArea (void);

	virtual void PhysicalUpdate (SINGLE dt);
	
	virtual BOOL32 Update ();
	
	virtual void Render (void);

	//---------------------------------------------------------------------------
	//
	virtual U32 GetPartID (void) const
	{
		if (owner)
		{
			return owner.Ptr()->GetPartID();
		}
		else
			return dwMissionID;
	}

	virtual U32 GetPlayerID () const
	{
		return MGlobals::GetPlayerFromPartID(dwMissionID);
	}

	virtual U32 GetSystemID() const
	{
		return systemID;
	}
	
	// ILauncher

	virtual void InitLauncher (IBaseObject * owner, S32 ownerIndex, S32 animArcheIndex, SINGLE _range);

	virtual void AttackPosition (const struct GRIDVECTOR * position, bool bSpecial);

	virtual void AttackObject (IBaseObject * obj);

	virtual void AttackMultiSystem (struct GRIDVECTOR * position, U32 targSystemID);

	virtual void WormAttack (IBaseObject * obj)
	{};

	virtual void DoCreateWormhole(U32 systemID)
	{}

	virtual const bool TestFightersRetracted (void) const; // return true if fighters are retracted

	virtual void SetFighterStance(FighterStance stance)
	{
	}

	virtual void HandlePreTakeover (U32 newMissionID, U32 troopID)
	{
		CQASSERT(stage == Z_NONE && "See Rob");
	}

	virtual void TakeoverSwitchID (U32 newMissionID)
	{
	}

	// the following methods are for network synchronization of realtime objects
	virtual U32 GetSyncDataSize (void) const;

	virtual U32 GetSyncData (void * buffer);			// buffer points to use supplied memory

	virtual void PutSyncData (void * buffer, U32 bufferSize);

	virtual void DoSpecialAbility (U32 specialID);

	virtual void DoSpecialAbility (IBaseObject * obj);

	virtual void DoCloak (void)
	{
	}
	
	virtual void SpecialAttackObject (IBaseObject * obj);

	virtual void  GetSpecialAbility (UNIT_SPECIAL_ABILITY & ability, bool & bEnabled)
	{
		MPart part(owner.Ptr());
		ability = USA_FURYRAM;
		bEnabled = part->caps.specialAttackOk;
	}

	virtual const U32 GetApproxDamagePerSecond (void) const
	{
		return 0;
	}

	virtual void InformOfCancel() {};

	virtual void LauncherOpCreated(U32 agentID, void * buffer, U32 bufferSize) {};

	virtual void LauncherReceiveOpData(U32 agentID, void * buffer, U32 bufferSize) {};

	virtual void LauncherOpCompleted(U32 agentID) {};

	virtual bool CanCloak(){return false;};

	virtual bool IsToggle() {return false;};

	virtual bool CanToggle(){return false;};

	virtual bool IsOn() {return false;};

	virtual void OnAllianceChange (U32 allyMask)
	{
	}

	virtual IBaseObject * FindChildTarget(U32 childID){return NULL;};

	/* ISaveLoad methods */

	virtual BOOL32 Save (struct IFileSystem * file);

	virtual BOOL32 Load (struct IFileSystem * file);

	virtual void ResolveAssociations ();

	// Zealot methods

//	bool checkSupplies ();

	void init (ZealotArchetype *arch);

	void shootTarget();

	BOOL32 update(SINGLE dt);
};

//----------------------------------------------------------------------------------
//
Zealot::~Zealot (void)
{
}
//---------------------------------------------------------------------------------------
//
void Zealot::CastVisibleArea (void)
{
	if (stage != Z_NONE)
	{
		// propogate visibility
		SetVisibleToAllies(GetVisibilityFlags());
	}
}
//----------------------------------------------------------------------------------
//
void Zealot::PhysicalUpdate (SINGLE dt)
{
	if (bDeleteRequested)
		return;

//	if (instanceIndex != INVALID_INSTANCE_INDEX)
//		ENGINE->update_instance(instanceIndex,0,dt);
//	if (mc != 0)
//		ENGINE->update_instance(mc->mi[0]->instanceIndex,0,dt);
	FRAME_physicalUpdate(dt);

	if (bVisible)
		bDeleteRequested = (update(dt) == 0);
}

BOOL32 Zealot::Update ()
{
	MPartNC part(owner.Ptr());
	if(part.isValid())
	{
		if(!(part->caps.specialAttackOk))
		{
			BT_ZEALOT_DATA * data = (BT_ZEALOT_DATA *)(ARCHLIST->GetArchetypeData(pArchetype));
			if(data->neededTech.raceID)
			{
				if(MGlobals::GetCurrentTechLevel(owner.Ptr()->GetPlayerID()).HasTech(data->neededTech))
				{
					part->caps.specialAttackOk = true;
				}
			}
			else
			{
				part->caps.specialAttackOk = true;
			}
		}
	}

	if (bVisible == 0)
		bDeleteRequested = (update(ELAPSED_TIME) == 0);

	return (bDeleteRequested==0);
}

BOOL32 Zealot::update(SINGLE dt)
{
	if (stage != Z_NONE)
	{
		if (target == 0)
			return 0;

		if (systemID != target.Ptr()->GetSystemID())
		{
			if (bVisible)
			{
				IBaseObject *obj = CreateBlast(arch->pImpactBlast,transform,systemID);
				OBJLIST->AddObject(obj);
			}

			stage = Z_NONE;
			target = 0;
			return 0;
		}
		
		Vector targetPos = target.Ptr()->GetPosition();
		Vector diff = targetPos-transform.translation;

		if (stage == Z_ROTATE)
		{
			SINGLE yaw = transform.get_yaw();
			SINGLE relYaw = get_angle(diff.x, diff.y) - yaw;
			
			if (relYaw < -PI)
				relYaw += PI*2;
			else if (relYaw > PI)
				relYaw -= PI*2;
//			rotateShip(transform, relYaw, 0,0);//relRoll,relPitch);
			ang_velocity.set(0,0,-4*relYaw);

			if (fabs(relYaw) < 0.1)
			{
				ang_velocity.set(0,0,0);
				stage = Z_THRUST;
			}
		} 
		else if (stage == Z_THRUST)
		{
			SINGLE mag;
			Vector pt,dir;
			if (target->GetCollisionPosition(pt,dir,transform.translation,diff))
			{
				diff = pt-transform.translation;
				mag = diff.magnitude();
				if (mag < data->kamikazeSpeed*dt)
				{
				//hit!!
					U32 damage = 0;
					if(MGlobals::GetCurrentTechLevel(MGlobals::GetPlayerFromPartID(dwMissionID)).HasTech(M_MANTIS,TECHTREE::M_RES_EXPLODYRAM2,0,0,0,0,0))
						damage = data->damageAmount[2];
					else if(MGlobals::GetCurrentTechLevel(MGlobals::GetPlayerFromPartID(dwMissionID)).HasTech(M_MANTIS,TECHTREE::M_RES_EXPLODYRAM1,0,0,0,0,0))
						damage = data->damageAmount[1];
					else
						damage = data->damageAmount[0];

					target->ApplyDamage(this,dwMissionID,transform.translation,diff,damage,0);
					if (bVisible)
					{
						IBaseObject *obj = CreateBlast(arch->pImpactBlast,transform,systemID);
						OBJLIST->AddObject(obj);
					}

					stage = Z_NONE;
					target = 0;

					return 0;
				}
			}
			else
				mag = diff.magnitude();
			
			velocity = diff*data->kamikazeSpeed/mag;
		}
		/*time -= 1;

		if (time ==0)
			return 0;*/

	}

	FRAME_update();

	return 1;
}
//----------------------------------------------------------------------------------
//
void Zealot::Render (void)
{
	if (bDeleteRequested)
		return;

	if (owner.Ptr() == 0 && systemID == SECTOR->GetCurrentSystem() && bVisible)
	{
		BATCH->set_state(RPR_BATCH,TRUE);
//		updateChildPositions();
		if (zealotArchetypeID)
			UNBORNMANAGER->RenderMeshAt(transform,zealotArchetypeID,dwMissionID & PLAYERID_MASK);
	}
}

// ILauncher

void Zealot::InitLauncher (IBaseObject * _owner, S32 ownerIndex, S32 animArcheIndex, SINGLE _range)
{
	CQASSERT(_owner);
	//ownerID = _owner->GetPartID();
	_owner->QueryInterface(ILaunchOwnerID,owner,LAUNCHVOLATILEPTR);
}

void Zealot::AttackPosition (const struct GRIDVECTOR * position, bool bSpecial)
{
//	CQBOMB0("AttackPosition not supported");
}

void Zealot::AttackObject (IBaseObject * obj)
{
//	CQBOMB0("AttackObject not supported");
}
 
void Zealot::AttackMultiSystem (struct GRIDVECTOR * position, U32 targSystemID)
{
}

const bool Zealot::TestFightersRetracted (void) const
{
	return true;
}  // return true or else

// the following methods are for network synchronization of realtime objects
U32 Zealot::GetSyncDataSize (void) const
{
	return sizeof(SYNC_PACKET);
}

U32 Zealot::GetSyncData (void * buffer)
{
	if (target && (owner.Ptr()->effectFlags.canShoot()) && (!owner.Ptr()->fieldFlags.suppliesLocked()))
	{
		if ((target.Ptr()->GetPosition()-owner.Ptr()->GetPosition()).magnitude() > owner->GetWeaponRange())
			return 0;

		if (target.Ptr()->GetSystemID() != owner.Ptr()->GetSystemID())
			return 0;

		SYNC_PACKET * data = (SYNC_PACKET *) buffer;
		data->targetID = targetID;
		shootTarget();
		return sizeof(*data);
	}

	return 0;
}

void Zealot::PutSyncData (void * buffer, U32 bufferSize)
{
	SYNC_PACKET data;
	memset((void *)(&data),0,sizeof(SYNC_PACKET));
	if(bufferSize != 1)
		memcpy(&data,buffer,bufferSize);

	targetID = data.targetID;
	shootTarget();
}
//---------------------------------------------------------------------------
//
void Zealot::DoSpecialAbility (U32 specialID)
{
}
//---------------------------------------------------------------------------
//
void Zealot::DoSpecialAbility (IBaseObject * obj)
{
}
//---------------------------------------------------------------------------
//
void Zealot::SpecialAttackObject (IBaseObject * obj)
{
	if (obj == 0)
		target = 0;

	if (owner == 0)
	{
		CQBOMB0("Bring to the attention of Rob Marr - ignorable");
		return;
	}

	{
		MPart part = owner.Ptr();
		if (part.isValid() && part->hullPoints==0)
			return;		// don't do this attack if already dead!
	}

	if(obj)
	{
		obj->QueryInterface(IWeaponTargetID,target,GetPlayerID());
		targetID = obj->GetPartID();
	}

	zealotArchetypeID = ARCHLIST->GetArchetypeDataID(owner.Ptr()->pArchetype);
}

void Zealot::shootTarget()
{
	if (owner)
		owner->LauncherCancelAttack();

	IBaseObject *obj = OBJLIST->FindObject(targetID);
	if(obj)
	{
		obj->QueryInterface(IWeaponTargetID,target,GetPlayerID());
		CQASSERT(target);

		U32 visFlags = owner.Ptr()->GetTrueVisibilityFlags();
		visFlags |= obj->GetTrueVisibilityFlags();
		SetVisibilityFlags(visFlags);

		dwMissionID = owner.Ptr()->GetPartID();
		owner.Ptr()->SetReady(false);

		UnregisterWatchersForObject(this);
		OBJLIST->AddObject(this);
		//danger to debuggers!

		ENGINE->set_instance_handler(owner.Ptr()->GetObjectIndex(),this);
	//	OBJLIST->DeferredDestruction(dwMissionID);
	//	EVENTSYS->Send(CQE_OBJECT_DESTROYED, (void *) dwMissionID);
		if (THEMATRIX->IsMaster())
			THEMATRIX->ObjectTerminated(dwMissionID);

		stage = Z_ROTATE;

		systemID = owner.Ptr()->GetSystemID();
		owner = 0;
	}
	if (THEMATRIX->IsMaster())
		THEMATRIX->ObjectTerminated(dwMissionID);
	//temp
//	time = 100;
}
//---------------------------------------------------------------------------
//
BOOL32 Zealot::Save (struct IFileSystem * outFile)
{
	DAFILEDESC fdesc = "ZEALOT_SAVELOAD";
	COMPTR<IFileSystem> file;
	BOOL32 result = 0;
	DWORD dwWritten;
	ZEALOT_SAVELOAD save;

	if (bDeleteRequested)
		goto Done;

	fdesc.lpImplementation = "DOS";
	fdesc.dwDesiredAccess = GENERIC_READ|GENERIC_WRITE;
	fdesc.dwShareMode = 0;  // no sharing
	fdesc.dwCreationDistribution = CREATE_ALWAYS;

	if (outFile->CreateInstance(&fdesc, file) != GR_OK)
		goto Done;

	memset(&save, 0, sizeof(save));
	memcpy(&save, static_cast<BASE_ZEALOT_SAVELOAD *>(this), sizeof(BASE_ZEALOT_SAVELOAD));

	FRAME_save(save);

	save.visibilityFlags = GetVisibilityFlags();
	
	file->WriteFile(0, &save, sizeof(save), &dwWritten, 0);
	
	result = 1;

Done:	
	return result;
}
//---------------------------------------------------------------------------
//
BOOL32 Zealot::Load (struct IFileSystem * inFile)
{
	DAFILEDESC fdesc = "ZEALOT_SAVELOAD";
	COMPTR<IFileSystem> file;
	BOOL32 result = 0;
	DWORD dwRead;
	ZEALOT_SAVELOAD load;
	U8 buffer[1024];

	fdesc.lpImplementation = "DOS";
	if (inFile->CreateInstance(&fdesc, file) != GR_OK)
		goto Done;

	file->ReadFile(0, buffer, sizeof(buffer), &dwRead, 0);
	MISSION->CorrelateSymbol("ZEALOT_SAVELOAD", buffer, &load);

	// temporarily removed - returned, but why was it removed?
	*static_cast<BASE_ZEALOT_SAVELOAD *>(this) = load;

	FRAME_load(load);

	SetVisibilityFlags(load.visibilityFlags);

	result = 1;

Done:	
	return result;
}
//---------------------------------------------------------------------------
//
void Zealot::ResolveAssociations ()
{
	if (targetID)
	{
		IBaseObject *obj = OBJLIST->FindObject(targetID);
		obj->QueryInterface(IWeaponTargetID,target,GetPlayerID());
		CQASSERT(target);
	}
}
//---------------------------------------------------------------------------
//
/*bool Zealot::checkSupplies ()
{
	MDATA mdata;
	owner.ptr->GetMissionData(mdata);
	SINGLE maxPts = mdata.pInitData->supplyPointsMax;
	SINGLE curPts = mdata.pSaveData->supplies;

	if (curPts/maxPts <= data->shutoff)
	{
		return false;
	}

	return true;
}*/
//---------------------------------------------------------------------------
//
void Zealot::init (ZealotArchetype *_arch)
{
	arch = _arch;
	data = arch->data;
}
//----------------------------------------------------------------------------------
//---------------------------------Zealot Factory------------------------------------
//----------------------------------------------------------------------------------
//----------------------------------------------------------------------------------

struct DACOM_NO_VTABLE ZealotManager : public IObjectFactory
{
	//
	// incoming interface map
	//
  
	BEGIN_DACOM_MAP_INBOUND(ZealotManager)
	DACOM_INTERFACE_ENTRY(IObjectFactory)
	END_DACOM_MAP()

	U32 factoryHandle;


	//child object info
	ZealotArchetype *pArchetype;

	//ZealotManager methods

	ZealotManager (void) 
	{
	}

	~ZealotManager();
	
    void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}

	IDAComponent * GetBase (void)
	{
		return (IObjectFactory *) this;
	}

	void init();

	//IObjectFactory
	DEFMETHOD_(HANDLE,CreateArchetype) (const char *szArchname, OBJCLASS objClass, void *data);

	DEFMETHOD_(BOOL32,DestroyArchetype) (HANDLE hArchetype);

	DEFMETHOD_(IBaseObject *,CreateInstance) (HANDLE hArchetype);

	DEFMETHOD_(void,EditorCreateInstance) (HANDLE hArchetype, const MISSION_INFO & info);

};

//--------------------------------------------------------------------------
// ZealotManager methods

ZealotManager::~ZealotManager()
{
	COMPTR<IDAConnectionPoint> connection;
	if (OBJLIST)
	{
		if (OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
			connection->Unadvise(factoryHandle);
	}
}
//--------------------------------------------------------------------------
//
void ZealotManager::init()
{
	COMPTR<IDAConnectionPoint> connection;


	if (OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
	{
		connection->Advise(GetBase(), &factoryHandle);
	}
}
//--------------------------------------------------------------------------
//
HANDLE ZealotManager::CreateArchetype(const char *szArchname, OBJCLASS objClass, void *data)
{
	ZealotArchetype *newguy = 0;
	if (objClass == OC_WEAPON)
	{
		BT_ZEALOT_DATA *objData = (BT_ZEALOT_DATA *)data;
		if (objData->wpnClass == WPN_ZEALOT)
		{
			newguy = new ZealotArchetype;
			newguy->name = szArchname;
			newguy->data = objData;
			newguy->rigidBodyArm.set(0,0,0);
			
			if (objData->impactBlastType[0])
			{
				newguy->pImpactBlast = ARCHLIST->LoadArchetype(objData->impactBlastType);
				if (newguy->pImpactBlast)
					ARCHLIST->AddRef(newguy->pImpactBlast, OBJREFNAME);
			}
			
			
		/*	DAFILEDESC fdesc;
			COMPTR<IFileSystem> objFile;

			if (objData->objectName)
			{
				fdesc.lpFileName = objData->objectName;
				if (OBJECTDIR->CreateInstance(&fdesc, objFile) == GR_OK)
				{
					TXMLIB->load_library(objFile);
					if ((newguy->archIndex = ENGINE->create_archetype(fdesc.lpFileName, objFile)) == INVALID_ARCHETYPE_INDEX)
					{
						goto Error;
					}
				}
				else 
				{
					CQFILENOTFOUND(fdesc.lpFileName);
					goto Error;
				}
			}
			else
			{
				CQERROR0("No object file specified"); 
				goto Error;
			}*/

			goto Done;
		}
	}
//Error:
	delete newguy;
	newguy = 0;

Done:
	return newguy;
}
//--------------------------------------------------------------------------
//
BOOL32 ZealotManager::DestroyArchetype(HANDLE hArchetype)
{
	ZealotArchetype *deadguy = (ZealotArchetype *)hArchetype;

	delete deadguy;
	return 1;
}
//--------------------------------------------------------------------------
//
IBaseObject * ZealotManager::CreateInstance(HANDLE hArchetype)
{
	ZealotArchetype *pZealot = (ZealotArchetype *)hArchetype;
	
	Zealot * obj = new ObjectImpl<Zealot>;
	obj->objClass = OC_WEAPON;
//	obj->instanceIndex = ENGINE->create_instance(pZealot->archIndex);
//	obj->renderArchIndex = pZealot->archIndex;
//	obj->archIndex = pZealot->archIndex;
//	ENGINE->hold_archetype(pZealot->archIndex);
	obj->init(pZealot);

	return obj;
	
}
//--------------------------------------------------------------------------
//
void ZealotManager::EditorCreateInstance(HANDLE hArchetype, const MISSION_INFO & info)
{
	// not supported
}

struct ZealotManager *ZealotMgr;
//----------------------------------------------------------------------------------------------
//
struct _zzc : GlobalComponent
{


	virtual void Startup (void)
	{
		ZealotMgr = new DAComponent<ZealotManager>;
		AddToGlobalCleanupList((IDAComponent **) &ZealotMgr);
	}

	virtual void Initialize (void)
	{
		ZealotMgr->init();
	}
};

static _zzc zzc;
//----------------------------------------------------------------------------------
//----------------------------------------------------------------------------------

//---------------------------------------------------------------------------
//--------------------------End Zealot.cpp------------------------------------
//---------------------------------------------------------------------------
