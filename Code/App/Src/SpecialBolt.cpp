//--------------------------------------------------------------------------//
//                                                                          //
//                             SpecialBolt.cpp                               //
//                                                                          //
//                  COPYRIGHT (C) 1997 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*

    $Author: Jasony $

    $Header: /Conquest/App/Src/SpecialBolt.cpp 33    11/02/00 1:24p Jasony $
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
#include "TObjRender.h"
#include "Mission.h"
#include "MGlobals.h"
#include "IMissionActor.h"
#include "TManager.h"

#include <TComponent.h>
#include <lightman.h>
#include <Engine.h>
#include <Vector.h>
#include <Matrix.h>
#include <IHardpoint.h>
#include <ITextureLibrary.h>
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
struct SpecialBoltArchetype : RenderArch
{
	const char *name;
	BT_SPECIALBOLT_DATA *data;
	ARCHETYPE_INDEX archIndex;
	IMeshArchetype * meshArch;
	PARCHETYPE pArchetype;
	PARCHETYPE PBlastType, pSparkType;

	void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}

	SpecialBoltArchetype (void)
	{
		meshArch = NULL;
		archIndex = -1;
	}

	~SpecialBoltArchetype (void)
	{
		ENGINE->release_archetype(archIndex);
		if (PBlastType)
			ARCHLIST->Release(PBlastType, OBJREFNAME);
		if (pSparkType)
			ARCHLIST->Release(pSparkType, OBJREFNAME);
	}
};

//---------------------------------------------------------------------------
//-----------------------------Class SpecialBolt------------------------------------
//---------------------------------------------------------------------------
#define FLASH_RANGE 2000

struct _NO_VTABLE SpecialBolt : public ObjectRender<ObjectTransform<ObjectFrame<IBaseObject,BOLT_SAVELOAD,SpecialBoltArchetype> > >, IWeapon, BASE_BOLT_SAVELOAD
{

	BEGIN_MAP_INBOUND(SpecialBolt)
	_INTERFACE_ENTRY(IBaseObject)
	_INTERFACE_ENTRY(IWeapon)
	END_MAP()

	//------------------------------------------

	CQLight flash;
	const BT_SPECIALBOLT_DATA * data;
	HSOUND hSound;
	OBJPTR<IWeaponTarget> target;
	PARCHETYPE PBlastType;
	SINGLE range;
 
	//------------------------------------------

	SpecialBolt (void)
	{
	/*	flashTime = 15;

		flash.color.r = 0xFF;
		flash.color.g = 0xC0;
		flash.color.b = 0x40;
		flash.range = FLASH_RANGE;
		flash.set_On(1);*/
	}

	virtual ~SpecialBolt (void);	// See ObjList.cpp

	/* IBaseObject methods */

	virtual BOOL32 Update (void);

	virtual void PhysicalUpdate (SINGLE dt)
	{
		transform.translation += (velocity * dt);
	}

	virtual void Render (void)
	{
		TreeRender(mc);
	}

	virtual U32 GetPartID (void) const
	{
		return ownerID;
	}

	virtual U32 GetSystemID (void) const
	{
		return systemID;
	}

	virtual U32 GetPlayerID (void) const
	{
		return MGlobals::GetPlayerFromPartID(ownerID);
	}

	/* IWeapon methods */

	void InitWeapon (IBaseObject * owner, const class TRANSFORM & orientation, IBaseObject * target, U32 flags, const class Vector * pos);

	/* SpecialBolt methods */

	void init (const SpecialBoltArchetype & data);

};
//---------------------------------------------------------------------------
//
static SpecialBolt * CreateSpecialBolt (SpecialBoltArchetype & data)
{
	SpecialBolt * bolt = new ObjectImpl<SpecialBolt>;

	bolt->FRAME_init(data);
	bolt->init(data);

	return bolt;
}
//----------------------------------------------------------------------------------
//
void SpecialBolt::init (const SpecialBoltArchetype & arch)
{
	CQASSERT(arch.data->wpnClass == WPN_BOLT);
	CQASSERT(arch.data->objClass == OC_WEAPON);

	objClass = OC_WEAPON;
	pArchetype = arch.pArchetype;
	PBlastType = arch.PBlastType;
	data = arch.data;
}
//----------------------------------------------------------------------------------
//
void SpecialBolt::InitWeapon (IBaseObject * owner, const class TRANSFORM & orientation, IBaseObject * _target, U32 flags, const class Vector * pos)
{
	Vector vel;
	TRANSFORM orient = orientation;

	systemID = owner->GetSystemID();
	ownerID = owner->GetPartID();
	U32 visFlags = owner->GetTrueVisibilityFlags();

	if (_target)
	{
		_target->QueryInterface(IWeaponTargetID, target, SYSVOLATILEPTR);
		CQASSERT(target!=0);

		targetID = _target->GetPartID();
		visFlags |= _target->GetTrueVisibilityFlags();
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
	flash.set_transform(orient);
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

	flashTime = data->flash.lifeTime/ELAPSED_TIME;

//	flash.color.r = data->flash.red;
//	flash.color.g = data->flash.green;
//	flash.color.b = data->flash.blue;
	flash.setColor(data->flash.red,data->flash.green,data->flash.blue);

	flash.range = data->flash.range;
	flash.setSystem(systemID);
	flash.set_On(1);
}
//----------------------------------------------------------------------------------
//
SpecialBolt::~SpecialBolt (void)
{
}
//----------------------------------------------------------------------------------
//
BOOL32 SpecialBolt::Update (void)
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
	
//	flash.range = FLASH_RANGE*(flashTime/15.0);
	if (flashTime > 0)
	{
		LightRGB color;
		color.r = data->flash.red*(flashTime/(REALTIME_FRAMERATE*data->flash.lifeTime));
		color.g = data->flash.green*(flashTime/(REALTIME_FRAMERATE*data->flash.lifeTime));
		color.b = data->flash.blue*(flashTime/(REALTIME_FRAMERATE*data->flash.lifeTime));
		flash.setColor(color.r,color.g,color.b);
		if (--flashTime < 0)
			flash.disable();
	}

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
				if (bVisible)
				{
					IBaseObject * blast = CreateBlast(PBlastType, transform, systemID);

					if (blast)
						OBJLIST->AddObject(blast);
				}

//				target->ApplyDamage(this, ownerID, collide_point, dir, data->damage);
				target->ApplyDamage(this, ownerID, start,direction, data->damage);
				//MStructure ship(target.ptr);
				//ship->ApplyEffect(data->special);
				bDeleteRequested = TRUE;
			}
		}
	}

	return (bDeleteRequested==0);
}

struct DACOM_NO_VTABLE SpecialBoltManager : public IObjectFactory
{
	//
	// incoming interface map
	//
  
	BEGIN_DACOM_MAP_INBOUND(SpecialBoltManager)
	DACOM_INTERFACE_ENTRY(IObjectFactory)
	END_DACOM_MAP()

	U32 factoryHandle;

	//child object info
	SpecialBoltArchetype *pArchetype;

	//SpecialBoltManager methods

	SpecialBoltManager (void) 
	{
	}

	~SpecialBoltManager();
	
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
// SpecialBoltManager methods

SpecialBoltManager::~SpecialBoltManager()
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
void SpecialBoltManager::init()
{
	COMPTR<IDAConnectionPoint> connection;

	if (OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
		connection->Advise(GetBase(), &factoryHandle);
}
//--------------------------------------------------------------------------
//
HANDLE SpecialBoltManager::CreateArchetype(const char *szArchname, OBJCLASS objClass, void *_data)
{
	if (objClass == OC_WEAPON)
	{
		BT_SPECIALBOLT_DATA * data = (BT_SPECIALBOLT_DATA *)_data;
		switch (data->wpnClass)
		{
		case WPN_SPECIALBOLT:
			{
				SpecialBoltArchetype *newguy = new SpecialBoltArchetype;
			
				newguy->name = szArchname;
				newguy->data = data;
				newguy->pArchetype = ARCHLIST->GetArchetype(szArchname);
				if (data->blastType[0])
				{
					if ((newguy->PBlastType = ARCHLIST->LoadArchetype(data->blastType)) != 0)
						ARCHLIST->AddRef(newguy->PBlastType, OBJREFNAME);
				}
				if (data->sparkType[0])
				{
					if ((newguy->pSparkType = ARCHLIST->LoadArchetype(data->sparkType)) != 0)
						ARCHLIST->AddRef(newguy->pSparkType, OBJREFNAME);
				}
				//
				// force preload of sound effect
				// 
				SFXMANAGER->Preload(data->launchSfx);
				
				DAFILEDESC fdesc = data->fileName;
				COMPTR<IFileSystem> file;
				if (OBJECTDIR->CreateInstance(&fdesc,file) == GR_OK)
					TEXLIB->load_library(file, 0);
				
				if (file != 0 && (newguy->archIndex = ENGINE->create_archetype(((BT_SPECIALBOLT_DATA *)data)->fileName, file)) != INVALID_ARCHETYPE_INDEX)
				{
				}
				else
				{
					delete newguy;
					newguy = 0;
				}

				return newguy;
			}
			break;
			
		default:
			break;
		}
	}

	return 0;
}
//--------------------------------------------------------------------------
//
BOOL32 SpecialBoltManager::DestroyArchetype(HANDLE hArchetype)
{
	SpecialBoltArchetype *deadguy = (SpecialBoltArchetype *)hArchetype;

	delete deadguy;

	return 1;
}
//--------------------------------------------------------------------------
//
IBaseObject * SpecialBoltManager::CreateInstance(HANDLE hArchetype)
{
	SpecialBoltArchetype *pWeapon = (SpecialBoltArchetype *)hArchetype;
	BT_SPECIALBOLT_DATA *objData = pWeapon->data;
	
	if (objData->objClass == OC_WEAPON)
	{
		IBaseObject * obj = NULL;
		switch (objData->wpnClass)
		{
		case WPN_SPECIALBOLT:
			{
				obj = CreateSpecialBolt(*pWeapon);
			}
			break;
		}

		return obj;
	}
	
	return 0;
}
//--------------------------------------------------------------------------
//
void SpecialBoltManager::EditorCreateInstance(HANDLE hArchetype,const MISSION_INFO & info)
{
	// not supported
}

static struct SpecialBoltManager *SPUB;
//----------------------------------------------------------------------------------------------
//
struct _specbolt : GlobalComponent
{
	virtual void Startup (void)
	{
		struct SpecialBoltManager *spubMgr = new DAComponent<SpecialBoltManager>;
		SPUB = spubMgr;
		AddToGlobalCleanupList((IDAComponent **) &SPUB);
	}

	virtual void Initialize (void)
	{
		SPUB->init();
	}
};

static _specbolt specbolt;

//--------------------------------------------------------------------------//
//------------------------------END SpecialBolt.cpp--------------------------------//
//--------------------------------------------------------------------------//
