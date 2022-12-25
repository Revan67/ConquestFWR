//--------------------------------------------------------------------------//
//                                                                          //
//                                 Tractor.cpp                                 //
//                                                                          //
//                  COPYRIGHT (C) 1997 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/Tractor.cpp 61    9/13/01 10:01a Tmauer $
*/			    
//--------------------------------------------------------------------------//

#include "pch.h"
#include <globals.h>

#include "TObject.h"
#include "Camera.h"
#include "Objlist.h"
#include "sfx.h"
#include "IConnection.h"
#include "Startup.h"
#include "SuperTrans.h"
#include "DSpecial.h"
#include "ILauncher.h"
#include "IWeapon.h"
#include "CQLight.h"
#include "TObjTrans.h"
#include "TObjFrame.h"
#include "Mission.h"
#include "MGlobals.h"
#include "IMissionActor.h"
#include "Anim2d.h"
#include "UserDefaults.h"
#include "IBlast.h"
#include "MPart.h"
#include "IShipMove.h"

#include <TComponent.h>
#include <lightman.h>
#include <Engine.h>
#include <Vector.h>
#include <Matrix.h>
#include <IHardpoint.h>
#include <stdlib.h>
#include <FileSys.h>
#include <ICamera.h>
#include <Pixel.h>
//#include <RPUL\PrimitiveBuilder.h>
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//
struct TractorArchetype
{
	const char *name;
	BT_TRACTOR_DATA *data;
	PARCHETYPE pArchetype;
	PARCHETYPE pContactBlast;
	AnimArchetype *animArch;

	TractorArchetype (void)
	{
		memset(this, 0, sizeof(*this));
	}

	~TractorArchetype (void)
	{
		if (animArch)
			delete animArch;
		if (pContactBlast)
			ARCHLIST->Release(pContactBlast, OBJREFNAME);
	}
};


struct _NO_VTABLE Tractor : IBaseObject, ISaveLoad, ILauncher, TRACTOR_SAVELOAD
{
	BEGIN_MAP_INBOUND(Tractor)
	_INTERFACE_ENTRY(IBaseObject)
	_INTERFACE_ENTRY(ILauncher)
	_INTERFACE_ENTRY(ISaveLoad)
	END_MAP()

	//------------------------------------------
	unsigned int textureID;
	TractorArchetype *arch;
	BT_TRACTOR_DATA *data;
	OBJPTR<IWeaponTarget> target;
	OBJPTR<ILaunchOwner> owner;
	OBJPTR<IBlast> contactBlast;
	HSOUND hSound;

	SINGLE timer;
	U32 ownerID;

	bool bPushing:1;

	//----------------------------------
	// hardpoint data
	//----------------------------------
	HardpointInfo  hardpointinfo;
	INSTANCE_INDEX barrelIndex;
	//------------------------------------------

	struct SYNC_PACKET
	{
		U32 targetID;
	};

	Tractor (void);
	virtual ~Tractor (void);	// See ObjList.cpp

	virtual BOOL32 Update (void);

	virtual void PhysicalUpdate (SINGLE dt);

//	virtual void TestVisible (const USER_DEFAULTS & defaults, const U32 currentSystem, const U32 currentPlayer);

	virtual void Render (void);

	virtual U32 GetSystemID (void) const
	{
		return systemID;
	}

	virtual U32 GetPartID (void) const
	{
		return ownerID;
	}

	virtual U32 GetPlayerID (void) const
	{
		return MGlobals::GetPlayerFromPartID(ownerID);
	}

	/* IWeapon methods */

//	virtual void InitWeapon (IBaseObject * owner, const class TRANSFORM & orientation, IBaseObject * obj, U32 flags, const class Vector * pos=0);

	// IRecon

	virtual void InitLauncher (IBaseObject * owner, S32 ownerIndex, S32 animArcheIndex, SINGLE range);

	virtual void AttackPosition (const struct GRIDVECTOR * position, bool bSpecial) {}

	virtual void AttackObject (IBaseObject * obj) {}

	virtual void AttackMultiSystem (struct GRIDVECTOR * position, U32 targSystemID) {}

	virtual void WormAttack (IBaseObject * obj)
	{};

	virtual void DoCreateWormhole(U32 systemID)
	{}

	virtual const bool TestFightersRetracted (void) const {return true;}   // return true if fighters are retracted

	virtual void SetFighterStance(FighterStance stance)
	{
	}

	virtual void HandlePreTakeover (U32 newMissionID, U32 troopID)
	{
		target = 0;
		targetID = 0;
	}

	virtual void TakeoverSwitchID (U32 newMissionID)
	{
		ownerID = newMissionID;
	}

	// the following methods are for network synchronization of realtime objects
	virtual U32 GetSyncDataSize (void) const;

	virtual U32 GetSyncData (void * buffer);		// buffer points to use supplied memory

	virtual void PutSyncData (void * buffer, U32 bufferSize);

	virtual void DoSpecialAbility (U32 specialID) {}

	virtual void DoSpecialAbility (IBaseObject *obj) {}

	virtual void DoCloak (void)
	{
	}
	
	virtual void SpecialAttackObject (IBaseObject * obj);

	/* what special attack (if any) does this unit have, and can we use it?  Variables get overwritten */

	virtual void  GetSpecialAbility (UNIT_SPECIAL_ABILITY & ability, bool & bEnabled)
	{
		MPart part(owner.Ptr());
		ability = USA_TRACTOR;
		bEnabled = (part->caps.specialAttackOk && (part->supplies >= data->supplyCost));
	} 

	virtual void InformOfCancel() 
	{
		if(targetID && !target)
		{
			targetID = 0;
			time = 0.0f;
		}
	};
	
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

	void init (TractorArchetype * data);

	/* ISaveLoad */

	virtual BOOL32 Save (struct IFileSystem * file);

	virtual BOOL32 Load (struct IFileSystem * file);

	virtual void ResolveAssociations();

	virtual const U32 GetApproxDamagePerSecond (void) const
	{ return data->damagePerSecond; }

	void shootTarget();
};

//----------------------------------------------------------------------------------
//
Tractor::Tractor (void) 
{
	time = 30;
}

//----------------------------------------------------------------------------------
//
Tractor::~Tractor (void)
{
	textureID = 0;
	if (hSound)
		SFXMANAGER->CloseHandle(hSound);

	if (target)
	{
		VOLPTR(IShipMove) smove = target.Ptr();
		if (smove)
			smove->ReleaseShipControl(ownerID);
	}

	delete contactBlast.Ptr();
}

//---------------------------------------------------------------------------
//
BOOL32 Tractor::Save (struct IFileSystem * inFile)
{
	DAFILEDESC fdesc = "TRACTOR_SAVELOAD";
	COMPTR<IFileSystem> file;
	BOOL32 result = 0;
	DWORD dwWritten;
	TRACTOR_SAVELOAD save;

	fdesc.lpImplementation = "DOS";
	fdesc.dwDesiredAccess = GENERIC_READ|GENERIC_WRITE;
	fdesc.dwShareMode = 0;  // no sharing
	fdesc.dwCreationDistribution = CREATE_ALWAYS;

	if (inFile->CreateInstance(&fdesc, file) != GR_OK)
		goto Done;

	if (target==0)
		targetID = 0;
	memset(&save, 0, sizeof(save));
	memcpy(&save, static_cast<TRACTOR_SAVELOAD *>(this), sizeof(TRACTOR_SAVELOAD));

	file->WriteFile(0, &save, sizeof(save), &dwWritten, 0);
	
	result = 1;

Done:	
	return result;
}
//---------------------------------------------------------------------------
//
BOOL32 Tractor::Load (struct IFileSystem * inFile)
{
	DAFILEDESC fdesc = "TRACTOR_SAVELOAD";
	COMPTR<IFileSystem> file;
	BOOL32 result = 0;
	DWORD dwRead;
	TRACTOR_SAVELOAD load;
	U8 buffer[1024];

	fdesc.lpImplementation = "DOS";
	if (inFile->CreateInstance(&fdesc, file) != GR_OK)
		goto Done;

	file->ReadFile(0, buffer, sizeof(buffer), &dwRead, 0);
	MISSION->CorrelateSymbol("TRACTOR_SAVELOAD", buffer, &load);

	*static_cast<TRACTOR_SAVELOAD *>(this) = load;

	result = 1;

Done:	
	return result;
}

//---------------------------------------------------------------------------
//

void Tractor::ResolveAssociations()
{
	if (targetID)
	{
		IBaseObject* targetObject = OBJLIST->FindObject(targetID);
		CQASSERT(targetObject!=0);

		if (targetObject) targetObject->QueryInterface(IWeaponTargetID, target,NONSYSVOLATILEPTR);
		CQASSERT(target!=0);
	}
}
//----------------------------------------------------------------------------------
//
BOOL32 Tractor::Update (void)
{
	BOOL32 result=1;
	MPartNC part(owner.Ptr());
	if(part.isValid())
	{
		if(!(part->caps.specialAttackOk))
		{
			BT_TRACTOR_DATA * data = (BT_TRACTOR_DATA *)(ARCHLIST->GetArchetypeData(pArchetype));
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
	if (target)
	{
		if (!bPushing)
		{
			systemID = owner.Ptr()->GetSystemID();
			U32 targetSystemID = target.Ptr()->GetSystemID();
			if (systemID == targetSystemID)
			{
				bPushing = true;
				VOLPTR(IShipMove) smove=target.Ptr();
				if (smove)
					smove->PushShipTo(ownerID,target.Ptr()->GetPosition(),50);
			}
		}
		/*if (mass > data->minimumMass && target!=0)
		{
			IBaseObject *owner = OBJLIST->FindObject(ownerID);
			if (owner == 0)
				return 0;
			
			Vector dir = owner->GetPosition()-target.ptr->GetPosition();//pushPos;
			
			dir.z = 0;
			if (fabs(dir.x) < 1e-3 && fabs(dir.y) < 1e-3)
			{
				dir.set(1,0,0);
			}
			else
				dir.normalize();
			
			SINGLE speed = (data->basePushPower+data->pushPerMass*mass);
		//	ENGINE->set_velocity(target.ptr->GetObjectIndex(),vel);
			target->PushShip(ownerID,dir,0);
		}*/

		if ((time-ELAPSED_TIME) <= 0.0f)
		{
			if (target && bPushing)
			{
				VOLPTR(IShipMove) smove = target.Ptr();
				if (smove)
				{
					Vector dir = owner.Ptr()->GetPosition()-target.Ptr()->GetPosition();
					target->ApplyDamage(this,ownerID,owner.Ptr()->GetPosition(),dir,time*data->damagePerSecond,0);
					smove->ReleaseShipControl(ownerID);
				}
			}
			target = 0;
			targetID = 0;
			time = 0.0f;
			if(owner)
				owner->LauncherCancelAttack();
			if (contactBlast)
			{
				delete contactBlast.Ptr();
				contactBlast = 0;
			}
		}
		else if (target)
		{
			time -= ELAPSED_TIME;
			Vector dir = owner.Ptr()->GetPosition()-target.Ptr()->GetPosition();
			target->ApplyDamage(this,ownerID,owner.Ptr()->GetPosition(),dir,ELAPSED_TIME*data->damagePerSecond,0);
		}
	}

	if (contactBlast)
	{
		contactBlast.Ptr()->Update();
	}

	return result;
}

void Tractor::PhysicalUpdate(SINGLE dt)
{
	if (refireDelay > 0)
		refireDelay -= dt;
}
//----------------------------------------------------------------------------------
//
/*void Tractor::TestVisible (const USER_DEFAULTS & defaults, const U32 currentSystem, const U32 currentPlayer)
{
	IBaseObject *owner = OBJLIST->FindObject(ownerID);
	
	bVisible = 0;

	if (owner && target!=0)
	{
		bVisible = (GetSystemID() == currentSystem &&
			   (IsVisibleToPlayer(currentPlayer) ||
			   (target.ptr->IsVisibleToPlayer(currentPlayer)) ||
			     defaults.bVisibilityRulesOff ||
			     defaults.bEditorMode) );
	}
}*/
//----------------------------------------------------------------------------------
//
void Tractor::Render (void)
{
	if (target == 0)
		return;

	SINGLE dt = OBJLIST->GetRealRenderTime();
	timer -= dt;
	while (timer < 0.0f)
		timer += 1.0f;
	CAMERA->SetModelView();
	SINGLE test = (SINGLE)rand()/RAND_MAX;
	
	//visibility code
	//Vector epos (ENGINE->get_position (instance));
	#define TOLERANCE 0.00001f

	Vector v0,v1,v2,v3;
	Vector cpos = MAINCAM->get_position();
	
#define BM_WDTH 200
#define SEG_LENGTH (16*BM_WDTH)

	int width = BM_WDTH;
	width += (U8)(12*test);
//	SINGLE mag,mag2;


	IBaseObject *owner = OBJLIST->FindObject(ownerID);
	if (owner == 0)
		return;

	bVisible = owner->bVisible || target.Ptr()->bVisible;
	if (!bVisible)
		return;

	bVisible = (target.Ptr()->GetSystemID() == owner->GetSystemID());

	if (!bVisible)
		return;

	Vector start = owner->GetPosition();
	if (barrelIndex != -1)
	{
		start = ENGINE->get_transform(barrelIndex)*hardpointinfo.point;
	}
	Vector end = target.Ptr()->GetPosition();
	SINGLE length = (end-start).magnitude();
	Vector direction = (end-start)/length;
//	int numSegments = 0.5*length/SEG_LENGTH;

	Vector look (cpos - start);

	//this is the draw code that makes the beam always visible
	
	Vector i;// (look.y, -look.x, 0);
	i = cross_product(look,direction);

	if (fabs (i.x) < TOLERANCE && fabs (i.y) < TOLERANCE)
	{
		i.x = 1.0f;
	}
	
	i.normalize ();
	
	BATCH->set_state(RPR_BATCH,TRUE);
	CAMERA->SetModelView();
	BATCH->set_render_state(D3DRS_ZENABLE,FALSE);
	BATCH->set_render_state(D3DRS_ZWRITEENABLE,FALSE);
	BATCH->set_render_state(D3DRS_ALPHABLENDENABLE,TRUE);
	BATCH->set_render_state(D3DRS_SRCBLEND,D3DBLEND_ONE);
	BATCH->set_render_state(D3DRS_DESTBLEND,D3DBLEND_ONE);
	BATCH->set_render_state(D3DRS_CULLMODE,D3DCULL_NONE);

//	BATCH->set_texture_stage_texture(0,animArch->frames[0].texture);
	SetupDiffuseBlend(arch->animArch->frames[0].texture,FALSE);
	BATCH->set_state(RPR_STATE_ID,arch->animArch->frames[0].texture);

	PB.Color3ub(255,255,255);	
	PB.Begin(PB_QUADS);


//	SINGLE step = 1.0/numSegments;
		
	v0 = start - (i *width);
	v1 = start + (i *width);
	v2 = end + (i * width);
	v3 = end - (i * width);

	AnimFrame *frame = &arch->animArch->frames[F2LONG(timer*8)%4];
		
	//stupid correction factor here to eliminate seams  !!!clamp???
	PB.TexCoord2f(-5*timer,frame->y0);     PB.Vertex3f(v0.x,v0.y,v0.z);
	PB.TexCoord2f(-5*timer,frame->y1);		PB.Vertex3f(v1.x,v1.y,v1.z);
	PB.TexCoord2f(length/SEG_LENGTH-5*timer,frame->y1);		PB.Vertex3f(v2.x,v2.y,v2.z);
	PB.TexCoord2f(length/SEG_LENGTH-5*timer,frame->y0);		PB.Vertex3f(v3.x,v3.y,v3.z);

	PB.End();  //GL_QUADS

	BATCH->set_state(RPR_STATE_ID,0);

	if (contactBlast)
	{
		Vector j = cross_product(i,-direction);
		Transform t;
		t.set_i(i);
		t.set_j(j);
		t.set_k(-direction);
		t.translation = end;
		contactBlast.Ptr()->PhysicalUpdate(dt);
		contactBlast->SetRelativeTransform(t);
		contactBlast.Ptr()->Render();
	}

}

// the following methods are for network synchronization of realtime objects
U32 Tractor::GetSyncDataSize (void) const
{
	return 1;
}

U32 Tractor::GetSyncData (void * buffer)
{
	SINGLE RANGE = owner->GetWeaponRange();
	IBaseObject* targetObject = OBJLIST->FindObject(targetID);
	if (targetObject == 0)
	{
		targetID = 0;
		return 0;
	}

	if (((owner.Ptr()->GetPosition() - targetObject->GetPosition()).magnitude_squared() < RANGE*RANGE) && targetID && 
		refireDelay <= 0 && (owner.Ptr()->effectFlags.canShoot()) && (!owner.Ptr()->fieldFlags.suppliesLocked()))
	{
		if (owner->UseSupplies(data->supplyCost,true))
		{
			SYNC_PACKET * data = (SYNC_PACKET *) buffer;
			data->targetID = targetID;
			shootTarget();
		}
		return sizeof(SYNC_PACKET);
	}
	
	return 0;
}

void Tractor::PutSyncData (void * buffer, U32 bufferSize)
{
	SYNC_PACKET * data = (SYNC_PACKET *) buffer;
	if(data->targetID)
	{
		targetID = data->targetID;
		shootTarget();
	}
}

//----------------------------------------------------------------------------------
//
void Tractor::SpecialAttackObject(IBaseObject *obj)
{
	if (obj)
	{
		targetID = obj->GetPartID();
	}
}

void Tractor::shootTarget()
{
	IBaseObject *obj = OBJLIST->FindObject(targetID);
	if (obj==0)
		return;

	refireDelay = data->refirePeriod;
	refireDelay += data->duration;
	
	time = data->duration;
	systemID = owner.Ptr()->GetSystemID();
	U32 targetSystemID = obj->GetSystemID();
	
	obj->QueryInterface(IWeaponTargetID, target, NONSYSVOLATILEPTR);
#ifndef FINAL_RELEASE
		if (target==0)
		{
			MPart part = obj;
			if (part.isValid())
			{
				CQBOMB1("Target \"%s\" doesn't support IWeaponTarget!", (const char *)part->partName);
			}
			else
				CQBOMB1("Target objClass=0x%X doesn't support IWeaponTarget!", obj->objClass);
		}
#endif
	if(target == 0)
		return;
	if (systemID == targetSystemID)
	{
		bPushing = true;
		VOLPTR(IShipMove) smove=target.Ptr();
		if(smove)
		{
			smove->PushShipTo(ownerID,target.Ptr()->GetPosition(),50);
		}
	}
	
	U32 visFlags = owner.Ptr()->GetVisibilityFlags();
	visFlags |= obj->GetVisibilityFlags();
	owner.Ptr()->SetVisibleToAllies(1 << (obj->GetPlayerID()-1));
	
	SetVisibilityFlags(visFlags);
	
	Vector pos = obj->GetPosition();
	
	hSound = SFXMANAGER->Open(data->launchSfx);
	SFXMANAGER->Play(hSound,systemID,&pos);
	
	//	mass = data->minimumMass*2+200;
	
	IBaseObject * b_obj;
	b_obj = ARCHLIST->CreateInstance(arch->pContactBlast);
	if (b_obj)
	{
		if (contactBlast)
		{
			delete contactBlast.Ptr();
			contactBlast = 0;
		}
		
		b_obj->QueryInterface(IBlastID,contactBlast,NONSYSVOLATILEPTR);
		CQASSERT(contactBlast && "Not a blast!!");
		TRANSFORM ownerTrans = owner.Ptr()->GetTransform();
		//!!!! might want to set the orientation here someday
		TRANSFORM trans;
		trans.translation = target.Ptr()->GetPosition();
		contactBlast->InitBlast(trans,owner.Ptr()->GetSystemID(),0);
		contactBlast->SetVisible(true);
	}
}

void Tractor::InitLauncher(IBaseObject * _owner, S32 ownerIndex, S32 animArcheIndex, SINGLE range)
{
	_owner->QueryInterface(ILaunchOwnerID,owner,LAUNCHVOLATILEPTR);

	ownerID = owner.Ptr()->GetPartID();

	barrelIndex = -1;
	if (data->hardpoint[0])
	{
		FindHardpoint(data->hardpoint, barrelIndex, hardpointinfo, ownerIndex);
		if (barrelIndex == -1)
			CQERROR1("Hardpoint '%s' not found!", data->hardpoint);
	}
}
//---------------------------------------------------------------------------
//
void Tractor::init (TractorArchetype * _arch)
{
	arch = _arch;
	CQASSERT(arch->data->wpnClass == WPN_TRACTOR);
	CQASSERT(arch->data->objClass == OC_WEAPON);

	data = (BT_TRACTOR_DATA *)arch->data;

	pArchetype = arch->pArchetype;
	objClass = OC_WEAPON;
//	animArch = arch->animArch;
}
//---------------------------------------------------------------------------
//
static Tractor * CreateTractor (TractorArchetype * data)
{
	Tractor * beam = new ObjectImpl<Tractor>;

//	beam->FRAME_init(data);
	beam->init(data);

	return beam;
}
//----------------------------------------------------------------------------------------------
//-------------------------------class TractorManager--------------------------------------------
//----------------------------------------------------------------------------------------------
//
struct DACOM_NO_VTABLE TractorManager : public IObjectFactory
{
	//
	// incoming interface map
	//
  
	BEGIN_DACOM_MAP_INBOUND(TractorManager)
	DACOM_INTERFACE_ENTRY(IObjectFactory)
	END_DACOM_MAP()

	U32 factoryHandle;

	//child object info
	TractorArchetype *pArchetype;

	//TractorManager methods

	TractorManager (void) 
	{
	}

	~TractorManager();
	
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
// TractorManager methods

TractorManager::~TractorManager()
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
void TractorManager::init()
{
	COMPTR<IDAConnectionPoint> connection;

	if (OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
		connection->Advise(GetBase(), &factoryHandle);
}
//--------------------------------------------------------------------------
//
HANDLE TractorManager::CreateArchetype(const char *szArchname, OBJCLASS objClass, void *_data)
{
	TractorArchetype *newguy=0;
	if (objClass == OC_WEAPON)
	{
		BT_TRACTOR_DATA * data = (BT_TRACTOR_DATA *)_data;
		if (data->wpnClass == WPN_TRACTOR)
		{
			
			newguy = new TractorArchetype;
			
			newguy->name = szArchname;
			newguy->data = data;
			newguy->pArchetype = ARCHLIST->GetArchetype(szArchname);
				
			COMPTR<IFileSystem> file;
			DAFILEDESC fdesc = data->fileName;
			
			fdesc.lpImplementation = "UTF";
			
			
			if (OBJECTDIR->CreateInstance(&fdesc, file) != GR_OK)
			{
				CQERROR1("Failed to open file %s", fdesc.lpFileName);
				goto Error;
			}
			
			if ((newguy->animArch = ANIM2D->create_archetype(file)) == 0)
			{
				CQERROR0("Failed to create tractor archetype");
				goto Error;
			}
			
			if (data->contactBlastType[0])
			{
				newguy->pContactBlast = ARCHLIST->LoadArchetype(data->contactBlastType);
				CQASSERT(newguy->pContactBlast);
				ARCHLIST->AddRef(newguy->pContactBlast, OBJREFNAME);
			}

			//
			// force preload of sound effect
			// 
			SFXMANAGER->Preload(data->launchSfx);
		
			goto Done;
		}
	}

Error:
	if (newguy)
		delete newguy;
	newguy = 0;

Done:
	return newguy;
}
//--------------------------------------------------------------------------
//
BOOL32 TractorManager::DestroyArchetype(HANDLE hArchetype)
{
	TractorArchetype *pWeapon = (TractorArchetype *)hArchetype;
	BT_TRACTOR_DATA *objData = pWeapon->data;
	
	if (objData->objClass == OC_WEAPON)
	{
//		IBaseObject * obj = NULL;
	/*	switch (objData->wpnClass)
		{
		case WPN_ARC:
			ReleaseArcCannonArchetype();
			break;
		}*/
	}
	TractorArchetype *deadguy = (TractorArchetype *)hArchetype;
	delete deadguy;

	return 1;
}
//--------------------------------------------------------------------------
//
IBaseObject * TractorManager::CreateInstance(HANDLE hArchetype)
{
	TractorArchetype *pWeapon = (TractorArchetype *)hArchetype;
	BT_TRACTOR_DATA *objData = pWeapon->data;
//	INSTANCE_INDEX index=-1;
	
	if (objData->objClass == OC_WEAPON)
	{
		IBaseObject * obj = NULL;
		switch (objData->wpnClass)
		{
		case WPN_TRACTOR:
			{
				obj = CreateTractor(pWeapon);
			}
			break;
		}

		return obj;
	}
	
	return 0;
}
//--------------------------------------------------------------------------
//
void TractorManager::EditorCreateInstance(HANDLE hArchetype,const MISSION_INFO & info)
{
	// not supported
}

static struct TractorManager *TRACTORMGR;
//----------------------------------------------------------------------------------------------
//
struct _tractors : GlobalComponent
{
	virtual void Startup (void)
	{
		struct TractorManager *tractorMgr = new DAComponent<TractorManager>;
		TRACTORMGR = tractorMgr;
		AddToGlobalCleanupList((IDAComponent **) &TRACTORMGR);
	}

	virtual void Initialize (void)
	{
		TRACTORMGR->init();
	}
};

static _tractors tractors;

//--------------------------------------------------------------------------//
//------------------------------END Tractor.cpp--------------------------------//
//--------------------------------------------------------------------------//
