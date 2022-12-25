//--------------------------------------------------------------------------//
//                                                                          //
//                                 AutoCannon.cpp                                 //
//                                                                          //
//                  COPYRIGHT (C) 1997 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*

    $Author: Jasony $

    $Header: /Conquest/App/Src/AutoCannon.cpp 24    10/18/00 11:14a Jasony $
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
#include "TObjTrans.h"
#include "Mesh.h"
#include "IBlast.h"
#include "ArchHolder.h"
#include "CQTrace.h"

#include <Renderer.h>
#include <TComponent.h>
#include <Engine.h>
#include <Vector.h>
#include <Matrix.h>
#include <IHardpoint.h>
#include <stdlib.h>
#include <FileSys.h>
#include <ICamera.h>

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
/*static void Transform_to_4x4 (float m[16], const Transform &t)
{
        m[ 0] = t.element[0][0];
        m[ 1] = t.element[1][0];
        m[ 2] = t.element[2][0];
        m[ 3] = 0;

        m[ 4] = t.element[0][1];
        m[ 5] = t.element[1][1];
        m[ 6] = t.element[2][1];
        m[ 7] = 0;

        m[ 8] = t.element[0][2];
        m[ 9] = t.element[1][2];
        m[10] = t.element[2][2];
        m[11] = 0;

        m[12] = t.element[0][3];
        m[13] = t.element[1][3];
        m[14] = t.element[2][3];
        m[15] = 1;
}*/

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#define NUM_STREAKS 4

struct _NO_VTABLE AutoCannon : public IBaseObject, IWeapon
{
	BEGIN_MAP_INBOUND(AutoCannon)
	_INTERFACE_ENTRY(IBaseObject)
	_INTERFACE_ENTRY(IWeapon)
	END_MAP()

	//------------------------------------------
	U32 systemID;
	Vector start, direction;
	SINGLE length;
	const BT_PROJECTILE_DATA * data;
	OBJPTR<IWeaponTarget> target;
	S32 time;
	HSOUND hSound;
	BOOL32 hit;
	Vector collide_point,collide_dir;
	INSTANCE_INDEX index[NUM_STREAKS];
	U8 cnt;
	Mesh *enemyMesh;
	ARCHETYPE_INDEX archeIndex;
	TRANSFORM transform;
	PARCHETYPE pSparkType, pBlastType;
	U32 ownerID;
	//------------------------------------------

    void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}

	AutoCannon (void);

	virtual ~AutoCannon (void);

	virtual BOOL32 Update (void);

	virtual void Render (void);

	virtual U32 GetSystemID (void) const;

	/* IWeapon methods */

	virtual void InitWeapon (IBaseObject * owner, const class TRANSFORM & orientation, IBaseObject * obj, U32 flags, const class Vector * pos=0);

	void init (PARCHETYPE _pArchetype);
	
};

AutoCannon::AutoCannon (void) 
{
	U8 i;
	for (i=0;i<NUM_STREAKS;i++)
	{
		index[i] = INVALID_INSTANCE_INDEX;
	}
	time = 40;
}
//----------------------------------------------------------------------------------
//
AutoCannon::~AutoCannon (void)
{
	if (hSound)
		SFXMANAGER->CloseHandle(hSound);
	U8 i;
	for (i=0;i<NUM_STREAKS;i++)
	{
		if (index[i] != INVALID_INSTANCE_INDEX)
			ENGINE->destroy_instance(index[i]);
	}
}
//----------------------------------------------------------------------------------
//
U32 AutoCannon::GetSystemID (void) const
{
	return systemID;
}
//----------------------------------------------------------------------------------
//
BOOL32 AutoCannon::Update (void)
{
	if (target==0)
		return 0;

	BOOL32 result=1;

	time--;
	if (time >= 20 && time%5==4)
//	if (time==39)
	{
		if (bVisible)
		{
			IBaseObject * blast = CreateBlast(pSparkType, transform, systemID);

			if (blast)
				OBJLIST->AddObject(blast);
		}

		if (cnt < NUM_STREAKS)
		{
			if ((index[cnt] = ENGINE->create_instance2(archeIndex,NULL)) != INVALID_INSTANCE_INDEX)
			{
				Transform trans = transform;
				trans.translation.x += (80*(SINGLE)rand()/RAND_MAX)-40;
				trans.translation.z += (80*(SINGLE)rand()/RAND_MAX)-40;
				ENGINE->set_transform(index[cnt],trans);
				ENGINE->set_velocity(index[cnt],16000*direction);
				cnt++;
			}
		}

	}

	if (time >= 20 && time%5==0)
	{
	//	Vector collide_point,dir;
		if (target->GetCollisionPosition(collide_point,collide_dir,start,direction))
		{
			if (bVisible)
			{
				Transform trans = transform;
				trans.set_position(collide_point);
				IBaseObject * blast = CreateBlast(pBlastType, trans, systemID);

				if (blast)
					OBJLIST->AddObject(blast);
			}
			
		}
	}

	if (time<=0)
	{
	//	target->ApplyDamage(this, ownerID, collide_point, collide_dir, data->damage);
		target->ApplyDamage(this, ownerID, start,direction, data->damage);

		result =0;
	}

	return result;
}
//----------------------------------------------------------------------------------
//
void AutoCannon::Render (void)
{
	if (target==0)
		return;
	
//	CAMERA->SetGLRenderVolume();
	U8 i;
	for (i=0;i<NUM_STREAKS;i++)
	{
		if (index[i] != INVALID_INSTANCE_INDEX)
			ENGINE->render_instance(MAINCAM,index[i],0,LODPERCENT,0,NULL);
	}

}

//----------------------------------------------------------------------------------
//
void AutoCannon::InitWeapon (IBaseObject * owner, const class TRANSFORM & orientation, IBaseObject * _target, U32 flags, const class Vector * pos)
{
	//Correct AutoCannon to fire at target regardless of gun barrel
	SINGLE curPitch, desiredPitch, relPitch;
	Vector goal = (_target)? _target->GetPosition() : *pos;

	systemID = owner->GetSystemID();
	ownerID = owner->GetPartID();
	U32 visFlags = owner->GetTrueVisibilityFlags();

	if (_target)
	{
		_target->QueryInterface(IWeaponTargetID, target, SYSVOLATILEPTR);
		CQASSERT(target!=0);
		visFlags |= _target->GetTrueVisibilityFlags();
		owner->SetVisibleToAllies(1 << (_target->GetPlayerID()-1));
	}
	SetVisibilityFlags(visFlags);
	curPitch = orientation.get_pitch();
	goal -= orientation.get_position();

	desiredPitch = sqrt(goal.x * goal.x  + goal.y * goal.y);
	desiredPitch = get_angle(goal.z, desiredPitch);

	relPitch = desiredPitch - curPitch;

	transform = orientation;
	transform.rotate_about_i(relPitch);

	start = orientation.get_position();
	direction = -transform.get_k();
	//transform.set_k(direction);

	hSound = SFXMANAGER->Open(data->launchSfx);
	SFXMANAGER->Play(hSound,systemID,&start);

	length = 5000;

	if (target!=0)
	{
		Vector norm;
		if (target->GetModelCollisionPosition(collide_point,norm,start,direction))
		{
			Vector len = collide_point;
			len -= start;
			SINGLE collide_dist = len.magnitude();
			
			if (length >= collide_dist)		// we have a winner!
			{
				length = collide_dist;
				hit = 1;
			}
		}
		
		HARCH archeIndex = target.Ptr()->GetObjectIndex();
		enemyMesh = REND->get_archetype_mesh(archeIndex);
	}

}
//---------------------------------------------------------------------------
//
void AutoCannon::init (PARCHETYPE _pArchetype)
{
	data = (const BT_PROJECTILE_DATA *) ARCHLIST->GetArchetypeData(_pArchetype);

	CQASSERT(data);
	CQASSERT(data->wpnClass == WPN_AUTOCANNON);
	CQASSERT(data->objClass == OC_WEAPON);

	pArchetype = _pArchetype;
	objClass = OC_WEAPON;
}
//---------------------------------------------------------------------------
//
IBaseObject * CreateAutoCannon (PARCHETYPE pArchetype, S32 archeIndex, PARCHETYPE _pBlastType, PARCHETYPE _pSparkType)
{
	AutoCannon * autoCannon = new ObjectImpl<AutoCannon>;

	autoCannon->init(pArchetype);
	autoCannon->archeIndex = archeIndex;
	autoCannon->pBlastType = _pBlastType;
	autoCannon->pSparkType = _pSparkType;

	return autoCannon;
}

