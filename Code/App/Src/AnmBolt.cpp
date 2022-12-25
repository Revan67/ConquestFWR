//--------------------------------------------------------------------------//
//                                                                          //
//                                 AnmBolt.cpp                                 //
//                                                                          //
//                  COPYRIGHT (C) 1997 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*

    $Author: Jasony $

    $Header: /Conquest/App/Src/AnmBolt.cpp 20    11/02/00 1:24p Jasony $
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
#include "DWeapon.h"
#include "IWeapon.h"
#include "CQLight.h"
#include "TObjTrans.h"
#include "TObjFrame.h"
#include "TObjPhys.h"
#include "MGlobals.h"
#include "Mission.h"
#include "IMissionActor.h"
#include "IEngineTrail.h"
#include "anim2d.h"

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
#include <ITextureLibrary.h>
//#include <RPUL\PrimitiveBuilder.h>
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//
struct AnmBoltArchetype
{
	const char *name;
	BT_ANMBOLT_DATA *data;
	ARCHETYPE_INDEX archIndex;
	IMeshArchetype * meshArch;
	PARCHETYPE pArchetype;
	PARCHETYPE PBlastType,pEngineTrailType;
	AnimArchetype * animArch;
	Vector rigidBodyArm;

	void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}

	AnmBoltArchetype (void)
	{
		meshArch = NULL;
		archIndex = -1;
	}

	~AnmBoltArchetype (void)
	{
		ENGINE->release_archetype(archIndex);
		if (PBlastType)
			ARCHLIST->Release(PBlastType, OBJREFNAME);
//		if (pSparkType)
//			ARCHLIST->Release(pSparkType);
		if(pEngineTrailType)
			ARCHLIST->Release(pEngineTrailType, OBJREFNAME);
		if(animArch)
			delete animArch;
	}
};

struct _NO_VTABLE AnmBolt : public ObjectPhysics<ObjectTransform<ObjectFrame<IBaseObject,ANMBOLT_SAVELOAD,AnmBoltArchetype> > >, IWeapon, BASE_ANMBOLT_SAVELOAD
{
	BEGIN_MAP_INBOUND(AnmBolt)
	_INTERFACE_ENTRY(IBaseObject)
	_INTERFACE_ENTRY(IWeapon)
	END_MAP()

	//------------------------------------------

	const BT_ANMBOLT_DATA * data;
	HSOUND hSound;
	OBJPTR<IWeaponTarget> target;
	PARCHETYPE PBlastType;
	OBJPTR<IEngineTrail> trail;
	SINGLE range;
	AnimInstance * boltAnim;
	//------------------------------------------

	AnmBolt (void);
	virtual ~AnmBolt (void);	// See ObjList.cpp

	virtual BOOL32 Update (void);

	virtual void PhysicalUpdate (SINGLE dt);

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

	virtual void InitWeapon (IBaseObject * owner, const class TRANSFORM & orientation, IBaseObject * obj, U32 flags, const class Vector * pos=0);

	void init (const AnmBoltArchetype & data);
};

//----------------------------------------------------------------------------------
//
AnmBolt::AnmBolt (void) 
{
}

//----------------------------------------------------------------------------------
//
AnmBolt::~AnmBolt (void)
{
	OBJLIST->ReleaseProjectile();
	if (hSound)
		SFXMANAGER->CloseHandle(hSound);
	if(trail != 0)
	{
		delete trail.Ptr();
		trail = 0;
	}
	if(boltAnim)
		delete boltAnim;
}
//----------------------------------------------------------------------------------
//
BOOL32 AnmBolt::Update (void)
{
	Vector pos;
	//
	// check to make sure we haven't exceeded our max range
	//
	
	pos = transform.get_position();
	pos -= initialPos;
	pos.z = 0;

	if (pos.magnitude() > range)
		bDeleteRequested = 1;
	
	if (target!=0)
	{
		Vector collide_point,dir;

		if (target->GetCollisionPosition(collide_point,dir,start,direction))
		{
			SINGLE distance;
			Vector diff = collide_point - transform.translation;
			diff.z = 0;
			distance = diff.magnitude();
			if (distance < data->maxVelocity * ELAPSED_TIME)
			{
//				transform.set_position(collide_point);
				if (bVisible)
				{
					IBaseObject * blast = CreateBlast(PBlastType, transform, systemID);

					if (blast)
						OBJLIST->AddObject(blast);
				}

				if ((launchFlags & IWF_ALWAYS_MISS)==0)
				{
					//apply damage should take original position and direction for now
					target->ApplyVisualDamage(this, ownerID, start,direction,data->damage);//collide_point, dir, data->damage);
				}
				bDeleteRequested = TRUE;
			}
		}
		else
		if (launchFlags & IWF_ALWAYS_HIT)
		{
			//
			// are we heading towards the target?
			//
			dir = target.Ptr()->GetTransform().translation - transform.translation;
			SINGLE relYaw = get_angle(dir.x, dir.y) - transform.get_yaw();

			if (relYaw < -PI)
				relYaw += PI*2;
			else
			if (relYaw > PI)
				relYaw -= PI*2;

			if (fabs(relYaw) > 60*MUL_DEG_TO_RAD)	// we've gone past
			{
				if (bVisible)
				{
					IBaseObject * blast = CreateBlast(PBlastType, transform, systemID);

					if (blast)
						OBJLIST->AddObject(blast);
				}

				target->ApplyVisualDamage(this, ownerID, transform.translation, dir, data->damage);
				bDeleteRequested = TRUE;
			}
		}
	}
	if(trail != 0)
		trail->Update();

	return (bDeleteRequested==0);
}

void AnmBolt::PhysicalUpdate (SINGLE dt)
{
	FRAME_physicalUpdate(dt);
	if(trail != 0)
		trail->PhysicalUpdate(dt);
	if(boltAnim)
		boltAnim->update(dt);
}

//----------------------------------------------------------------------------------
//
void AnmBolt::Render (void)
{
	if (bVisible)
	{
//		BATCH->set_render_state(D3DRS_ZWRITEENABLE,TRUE);
//		BATCH->set_render_state(D3DRS_ZENABLE,TRUE);
//		ENGINE->render_instance(MAINCAM, instanceIndex);
		if(boltAnim)
		{		
			BATCH->set_render_state(D3DRS_ZWRITEENABLE,FALSE);
			BATCH->set_render_state(D3DRS_ALPHABLENDENABLE,TRUE);
			BATCH->set_render_state(D3DRS_SRCBLEND,D3DBLEND_ONE);
			BATCH->set_render_state(D3DRS_DESTBLEND,D3DBLEND_ONE);
			ANIM2D->render(boltAnim,&(GetTransform()));
		}
		if(trail != 0)
			trail->Render();
	}
}
//----------------------------------------------------------------------------------
//
void AnmBolt::InitWeapon (IBaseObject * owner, const class TRANSFORM & orientation, IBaseObject * _target, U32 flags, const class Vector * pos)
{
	Vector vel;
	TRANSFORM orient = orientation;
	U32 visFlags = owner->GetVisibilityFlags();

	systemID = owner->GetSystemID();
	ownerID = owner->GetPartID();
	launchFlags = flags;

	if (_target)
	{
		_target->QueryInterface(IWeaponTargetID, target, SYSVOLATILEPTR);
		CQASSERT(target!=0);

		targetID = _target->GetPartID();
		visFlags |= _target->GetVisibilityFlags();
		owner->SetVisibleToAllies(1 << (_target->GetPlayerID()-1));
		_target->SetVisibleToAllies(1 << (owner->GetPlayerID()-1));
	}

	SetVisibilityFlags(visFlags);
	start = orientation.get_position();

	//Correct bolt to fire at target regardless of gun barrel
	SINGLE curPitch, desiredPitch, relPitch;
	Vector goal = (pos)? *pos : _target->GetPosition();

	curPitch = orient.get_pitch();
	//goal -= ENGINE->get_position(barrelIndex);
	goal -= orient.get_position();

	range = goal.magnitude() + 1000;
	
	desiredPitch = sqrt(goal.x * goal.x  + goal.y * goal.y);
	desiredPitch = get_angle(goal.z, desiredPitch);

	relPitch = desiredPitch - curPitch;

	orient.rotate_about_i(relPitch);
////------------------------
	transform = orient;
	initialPos = orientation.get_position();
	//shouldn't be necessary
//	if (initialPos.z == 0)
//		initialPos.z += 1;	// prevent later div by 0
	
	vel = -orient.get_k();
	vel *= data->maxVelocity;
	velocity = vel;
	direction = vel;

	hSound = SFXMANAGER->Open(data->launchSfx);
	SFXMANAGER->Play(hSound,systemID,&start);

}
//---------------------------------------------------------------------------
//
void AnmBolt::init (const AnmBoltArchetype & arch)
{
	CQASSERT(arch.data->wpnClass == WPN_ANMBOLT);
	CQASSERT(arch.data->objClass == OC_WEAPON);

	objClass = OC_WEAPON;
	pArchetype = arch.pArchetype;
	PBlastType = arch.PBlastType;
	data = (BT_ANMBOLT_DATA *)arch.data;

//	if (data->MASS)
//		PHYSICS->set_mass(instanceIndex, data->MASS);

	if(arch.pEngineTrailType != 0)
	{
		IBaseObject * obj = ARCHLIST->CreateInstance(arch.pEngineTrailType);
		if(obj)
		{
			obj->QueryInterface(IEngineTrailID,trail, NONSYSVOLATILEPTR);
			if(trail != 0)
			{
				trail->InitEngineTrail(this,instanceIndex);
			}
		}

	}
	if(arch.animArch)
	{
		boltAnim = new AnimInstance;
		if (boltAnim)
		{
			boltAnim->Init(arch.animArch);
			boltAnim->SetRotation(2*PI*(SINGLE)rand()/RAND_MAX);
			boltAnim->SetWidth(data->boltSize);
			boltAnim->loop = TRUE;
		}
	}

}
//---------------------------------------------------------------------------
//
static AnmBolt * CreateAnmBolt (AnmBoltArchetype & data)
{
	AnmBolt * bolt = new ObjectImpl<AnmBolt>;

	bolt->FRAME_init(data);
	bolt->init(data);

	return bolt;
}
//----------------------------------------------------------------------------------------------
//-------------------------------class AnmBoltManager--------------------------------------------
//----------------------------------------------------------------------------------------------
//

struct DACOM_NO_VTABLE AnmBoltManager : public IObjectFactory
{
	//
	// incoming interface map
	//
  
	BEGIN_DACOM_MAP_INBOUND(AnmBoltManager)
	DACOM_INTERFACE_ENTRY(IObjectFactory)
	END_DACOM_MAP()

	U32 factoryHandle;

	//child object info
	AnmBoltArchetype *pArchetype;

	//BeamManager methods

	AnmBoltManager (void) 
	{
	}

	~AnmBoltManager();
	
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
// AnmBoltManager methods

AnmBoltManager::~AnmBoltManager()
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
void AnmBoltManager::init()
{
	COMPTR<IDAConnectionPoint> connection;

	if (OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
		connection->Advise(GetBase(), &factoryHandle);
}
//--------------------------------------------------------------------------
//
HANDLE AnmBoltManager::CreateArchetype(const char *szArchname, OBJCLASS objClass, void *_data)
{
	if (objClass == OC_WEAPON)
	{
		BT_ANMBOLT_DATA * data = (BT_ANMBOLT_DATA *)_data;
		if (data->wpnClass == WPN_ANMBOLT)
		{
			AnmBoltArchetype *newguy = new AnmBoltArchetype;
		
			newguy->name = szArchname;
			newguy->data = data;
			newguy->pArchetype = ARCHLIST->GetArchetype(szArchname);
			if (data->blastType[0])
			{
				if ((newguy->PBlastType = ARCHLIST->LoadArchetype(data->blastType)) != 0)
					ARCHLIST->AddRef(newguy->PBlastType, OBJREFNAME);
			}
/*			if (data->sparkType[0])
			{
				if ((newguy->pSparkType = ARCHLIST->LoadArchetype(data->sparkType)) != 0)
					ARCHLIST->AddRef(newguy->pSparkType);
			}*/
			if (data->engineTrailType[0])
			{
				if ((newguy->pEngineTrailType = ARCHLIST->LoadArchetype(data->engineTrailType)) != 0)
					ARCHLIST->AddRef(newguy->pEngineTrailType, OBJREFNAME);
			}
			//
			// force preload of sound effect
			// 
			SFXMANAGER->Preload(data->launchSfx);
			
			DAFILEDESC fdesc = data->fileName;
			COMPTR<IFileSystem> file;
			if (OBJECTDIR->CreateInstance(&fdesc,file) == GR_OK)
				TEXLIB->load_library(file, 0);
			

			if (data->animFile[0])
			{
				DAFILEDESC fdesc;
				COMPTR<IFileSystem> objFile;
				fdesc.lpFileName = data->animFile;
				if (OBJECTDIR->CreateInstance(&fdesc, objFile) == GR_OK)
				{
					newguy->animArch = ANIM2D->create_archetype(objFile);
				}
				else 
				{
					CQFILENOTFOUND(fdesc.lpFileName);
					newguy->animArch =0;
				}
			}

			if (file != 0 && (newguy->archIndex = ENGINE->create_archetype(((BT_PROJECTILE_DATA *)data)->fileName, file)) != INVALID_ARCHETYPE_INDEX)
			{
			}
			else
			{
				delete newguy;
				newguy = 0;
			}

			return newguy;
		}
	}
	
	return 0;
}
//--------------------------------------------------------------------------
//
BOOL32 AnmBoltManager::DestroyArchetype(HANDLE hArchetype)
{
	AnmBoltArchetype *deadguy = (AnmBoltArchetype *)hArchetype;	

	delete deadguy;

	return 1;
}
//--------------------------------------------------------------------------
//
IBaseObject * AnmBoltManager::CreateInstance(HANDLE hArchetype)
{
	AnmBoltArchetype *pWeapon = (AnmBoltArchetype *)hArchetype;
	BT_ANMBOLT_DATA *objData = pWeapon->data;
//	INSTANCE_INDEX index=-1;
	
	if (objData->objClass == OC_WEAPON)
	{
		IBaseObject * obj = NULL;
		switch (objData->wpnClass)
		{
		case WPN_ANMBOLT:
			{
				obj = CreateAnmBolt(*pWeapon);
			}
			break;
		}

		return obj;
	}
	
	return 0;
}
//--------------------------------------------------------------------------
//
void AnmBoltManager::EditorCreateInstance(HANDLE hArchetype,const MISSION_INFO & info)
{
	// not supported
}

struct _AnmBoltManager : GlobalComponent
{
	AnmBoltManager * sfactory;

	virtual void Startup (void)
	{
		sfactory = new DAComponent<AnmBoltManager>;
		AddToGlobalCleanupList((IDAComponent **) &sfactory);
	}

	virtual void Initialize (void)
	{
		sfactory->init();
	}
};

static _AnmBoltManager __aBolt;
//--------------------------------------------------------------------------//
//------------------------------END AnmBolt.cpp--------------------------------//
//--------------------------------------------------------------------------//
