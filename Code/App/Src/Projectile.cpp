//--------------------------------------------------------------------------//
//                                                                          //
//                             Projectile.cpp                               //
//                                                                          //
//                  COPYRIGHT (C) 1997 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*

    $Author: Jasony $

    $Header: /Conquest/App/Src/Projectile.cpp 97    11/10/00 10:17a Jasony $
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
#include "Mission.h"
#include "MGlobals.h"
#include "IMissionActor.h"
#include "IEngineTrail.h"
#include <DEffectOpts.h>
#include "MPart.h"
#include "ObjMap.h"
#include "CQExtent.h"
#include "Userdefaults.h"
#include "Sysmap.h"
#include "TObjRender.h"

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
#include <IParticleSystem.h>
#include <IAnim.h>
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//

static SINGLE accumulatedTime;
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//
struct ProjectileArchetype : RenderArch
{
	const char *name;
	BT_PROJECTILE_DATA *data;
	ARCHETYPE_INDEX archIndex;
	IMeshArchetype * meshArch;
	PARCHETYPE pArchetype;
	PARCHETYPE PBlastType,pEngineTrailType;
	Vector rigidBodyArm;

	void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}

	ProjectileArchetype (void)
	{
		archIndex = -1;
	}

	~ProjectileArchetype (void)
	{
		ENGINE->release_archetype(archIndex);
		if (PBlastType)
			ARCHLIST->Release(PBlastType, OBJREFNAME);
//		if (pSparkType)
//			ARCHLIST->Release(pSparkType);
		if(pEngineTrailType)
			ARCHLIST->Release(pEngineTrailType, OBJREFNAME);
	}
};

//---------------------------------------------------------------------------
//-----------------------------Class Bolt------------------------------------
//---------------------------------------------------------------------------

//static HSOUND hSound = 0;
//static HSOUND hSound2 = 0;
//static HSOUND sm_sound = 0;
//static int sound_users2 = 0;
//static int sound_users = 0;
//static int sm_sound_users = 0;

struct _NO_VTABLE Bolt : public ObjectRender<ObjectTransform<ObjectFrame<IBaseObject,BOLT_SAVELOAD,ProjectileArchetype> > >, IWeapon, BASE_BOLT_SAVELOAD
{

	BEGIN_MAP_INBOUND(Bolt)
	_INTERFACE_ENTRY(IBaseObject)
	_INTERFACE_ENTRY(IWeapon)
	END_MAP()

	//------------------------------------------

	const BT_PROJECTILE_DATA * data;
	HSOUND hSound;
	OBJPTR<IWeaponTarget> target;
	PARCHETYPE PBlastType;
	OBJPTR<IEngineTrail> trail;
	SINGLE range;
	int tech;
 
	//------------------------------------------

	Bolt (void)
	{
	}

	virtual ~Bolt (void);	// See ObjList.cpp

	/* IBaseObject methods */

	virtual BOOL32 Update (void);

	virtual void TestVisible (const USER_DEFAULTS & defaults, const U32 currentSystem, const U32 currentPlayer)
	{
		bVisible = (bDeleteRequested==0 && 
				    systemID == currentSystem &&
				   (IsVisibleToPlayer(currentPlayer) ||
					 defaults.bVisibilityRulesOff ||
					 defaults.bEditorMode) );

		if (bVisible)
		{
			S32 screenX,screenY;
			CAMERA->PointToScreen(transform.translation,&screenX,&screenY);
			
			RECT _rect;
			
			_rect.left  = screenX - 15;
			_rect.right	= screenX + 15;
			_rect.top = screenY - 15;
			_rect.bottom = screenY + 15;
			
			RECT screenRect = { 0, 0, SCREENRESX, SCREENRESY };
			
			bVisible = RectIntersects(_rect, screenRect);
		}
	}

	virtual void CastVisibleArea(void)
	{
		SetVisibleToAllies(GetVisibilityFlags());
	}

	virtual void PhysicalUpdate (SINGLE dt);

	virtual void Render (void);

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

	/* Bolt methods */

	BOOL32 update (SINGLE dt);

	void init (const ProjectileArchetype & data);
};
//---------------------------------------------------------------------------
//
static Bolt * CreateBolt (ProjectileArchetype & data)
{
	Bolt * bolt = new ObjectImpl<Bolt>;

	bolt->FRAME_init(data);
	bolt->init(data);

	return bolt;
}
//----------------------------------------------------------------------------------
//
void Bolt::init (const ProjectileArchetype & arch)
{
	CQASSERT(arch.data->wpnClass == WPN_BOLT);
	CQASSERT(arch.data->objClass == OC_WEAPON);

	objClass = OC_WEAPON;
	pArchetype = arch.pArchetype;
	PBlastType = arch.PBlastType;
	data = (BT_PROJECTILE_DATA *)arch.data;

//	if (data->MASS)
//		PHYSICS->set_mass(instanceIndex, data->MASS);

	if(arch.pEngineTrailType != 0)
	{
		IBaseObject * obj = ARCHLIST->CreateInstance(arch.pEngineTrailType);
		if(obj)
		{
			obj->QueryInterface(IEngineTrailID,trail,NONSYSVOLATILEPTR);
			if(trail != 0)
			{
				trail->InitEngineTrail(this,instanceIndex);
			}
		}

	}
}
//----------------------------------------------------------------------------------
//
void Bolt::InitWeapon (IBaseObject * owner, const class TRANSFORM & orientation, IBaseObject * _target, U32 flags, const class Vector * pos)
{
	Vector vel;
	TRANSFORM orient = orientation;
	U32 visFlags = owner->GetTrueVisibilityFlags();

	systemID = owner->GetSystemID();
	ownerID = owner->GetPartID();
	MPartNC part = owner;
	if (part)
		tech = part->techLevel.damage;

	launchFlags = flags;

	if (_target)
	{
		_target->QueryInterface(IWeaponTargetID, target, SYSVOLATILEPTR);

#ifndef FINAL_RELEASE
		if (target==0)
		{
			MPart part = _target;
			if (part.isValid())
			{
				CQBOMB1("Target \"%s\" doesn't support IWeaponTarget!", (const char *)part->partName);
			}
			else
				CQBOMB1("Target objClass=0x%X doesn't support IWeaponTarget!", _target->objClass);
		}
#endif

		targetID = _target->GetPartID();
		visFlags |= _target->GetTrueVisibilityFlags();
		owner->SetVisibleToAllies(1 << (_target->GetPlayerID()-1));
		_target->SetVisibleToAllies(1 << (owner->GetPlayerID()-1));
	}

	SetVisibilityFlags(visFlags);

	bVisible = 1;		// assume it's visible until proven otherwise

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
//----------------------------------------------------------------------------------
//
Bolt::~Bolt (void)
{
	OBJLIST->ReleaseProjectile();
	if (hSound)
		SFXMANAGER->CloseHandle(hSound);
	if(trail != 0)
	{
		delete trail.Ptr();
		trail = 0;
	}
}
//----------------------------------------------------------------------------------
//
BOOL32 Bolt::Update (void)
{
	if (bVisible==false && bDeleteRequested==0)
		update(ELAPSED_TIME);

	return (bDeleteRequested==0);
}
//----------------------------------------------------------------------------------
//
static bool bDealDamage = true;
BOOL32 Bolt::update (SINGLE dt)
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

		Vector norm_dir = direction;
		norm_dir.normalize();
		if (target->GetCollisionPosition(collide_point,dir,start,norm_dir))
		{
			SINGLE distance;
			Vector diff = collide_point - transform.translation;
			diff.z = 0;
			distance = diff.magnitude();
			if (distance < data->maxVelocity * dt)
			{
//				transform.set_position(collide_point);
				if (bDealDamage && (launchFlags & IWF_ALWAYS_MISS)==0)
				{
					//apply damage should take original position and direction for now
					if (target->ApplyVisualDamage(this, ownerID, start,norm_dir,data->damage) == 0)
					{
					//	IBaseObject * blast = CreateBlast(PBlastType, transform, systemID);

					//	if (blast)
					//		OBJLIST->AddObject(blast);

						if (bVisible)
						{
							collide_point = transform.translation;
							target->GetModelCollisionPosition(collide_point,dir,start,direction);
							target->AttachBlast(PBlastType,collide_point,-direction);
						}
					}
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
				if (bDealDamage && target->ApplyVisualDamage(this, ownerID, transform.translation, dir, data->damage) == 0)
				{
				//	IBaseObject * blast = CreateBlast(PBlastType, transform, systemID);
					
				//	if (blast)
				//		OBJLIST->AddObject(blast);
					
					if (bVisible)
					{
						collide_point = transform.translation;
						target->GetModelCollisionPosition(collide_point,dir,start,direction);
						target->AttachBlast(PBlastType,collide_point,-direction);
					}
				}
				bDeleteRequested = TRUE;
			}
		}
	}
	if(trail != 0)
		trail->Update();

	return (bDeleteRequested==0);
}
//---------------------------------------------------------------------------
//
void Bolt::PhysicalUpdate (SINGLE dt)
{
	transform.translation += (velocity * dt);
	if(trail != 0)
		trail->PhysicalUpdate(dt);

	if (bVisible && bDeleteRequested==0)
		update(dt);
}
//---------------------------------------------------------------------------
//
void Bolt::Render()
{
	if (bVisible)
	{

		Transform s;
		s.d[0][0] *= 1+0.1*tech;
		s.d[1][1] *= 1+0.1*tech;
		s.d[2][2] *= 1+0.4*tech;
		Transform trans = Transform(s.get_orientation(), -s.get_position()) * Transform (s.get_position());
		
		BATCH->set_state(RPR_BATCH,TRUE);
		BATCH->set_render_state(D3DRS_ZWRITEENABLE,TRUE);
		BATCH->set_render_state(D3DRS_ZENABLE,TRUE);
		//ENGINE->render_instance(MAINCAM, instanceIndex,0,LODPERCENT,RF_TRANSFORM_LOCAL,&trans);
		TreeRender(mc);
		if(trail != 0)
			trail->Render();
	}
}
//---------------------------------------------------------------------------
//-----------------------------Class Missile---------------------------------
//---------------------------------------------------------------------------
#define SPREAD	1.5f
#define UPDATE_INTERVAL 0.08f
#define FREQ	(UPDATE_INTERVAL*2.2f)

struct Ellipse missileBubble(Vector(0,0,0),50,Vector(1.0,1.0,1.0),Transform());

struct _NO_VTABLE Missile : public ObjectRender<ObjectPhysics<ObjectTransform<ObjectFrame<IBaseObject,MISSILE_SAVELOAD,ProjectileArchetype> > > >, IWeapon, IWeaponTarget, BASE_MISSILE_SAVELOAD
{
	BEGIN_MAP_INBOUND(Missile)
	_INTERFACE_ENTRY(IBaseObject)
	_INTERFACE_ENTRY(IWeapon)
	_INTERFACE_ENTRY(IWeaponTarget)
	END_MAP()

	HSOUND hSound;
	const BT_PROJECTILE_DATA * data;
	PARCHETYPE PBlastType;
	Vector relTargetPos;
	Vector ang_base;
	TRANSFORM startTrans;
	SINGLE range;
	SINGLE theta;

	//ROB'S NEW PHUNKY PHRESH PATH
	SINGLE pathLength,pathHeight;
	Vector base,up;
	SINGLE r,d;

	//render experiment

	ProjectileArchetype *arch;

	OBJPTR<IWeaponTarget> target;
	OBJPTR<IEngineTrail> trail;

	int map_square;
	SINGLE timer;

	//------------------------------------------

	Missile (void)
	{
		startTrans.rotate_about_i(90*MUL_DEG_TO_RAD);
		theta = (rand()%1000)*0.333*PI*0.001;
	}

	~Missile (void) 
	{
		OBJLIST->ReleaseProjectile();
		if (hSound)
			SFXMANAGER->CloseHandle(hSound);
		if(trail != 0)
		{
			delete trail.Ptr();
			trail = 0;
		}
			
		OBJMAP->RemoveObjectFromMap(this,systemID,map_square);
	}

	/* IBaseObject methods */

	virtual void SetTerrainFootprint (struct ITerrainMap * terrainMap);
	
	virtual BOOL32 Update (void)
	{
		if (bVisible==false && bDeleteRequested==0)
			update(ELAPSED_TIME);
		
		BOOL32 bTrailIsDone=1;
		//"EngineTrail"
		if(trail != 0)
		{
			if (bDeleteRequested)
				trail->SetDecayMode();

			bTrailIsDone = (trail->Update()==0 || (!CQEFFECTS.bWeaponTrails));
		}
		else
		{
			//particle trail
			timeToLive -= ELAPSED_TIME;
			
			if (timeToLive > 0)
				bTrailIsDone = FALSE;
		}
		
		return (bDeleteRequested==0 || bTrailIsDone==0);
	}

	BOOL32 update (SINGLE dt);

	virtual void PhysicalUpdate (SINGLE dt)
	{
		FRAME_physicalUpdate(dt);
		
		if(trail != 0)
			trail->PhysicalUpdate(dt);

		if (bVisible)
		{
			ANIM->update_instance(instanceIndex,dt);
		}

		if (bVisible && bDeleteRequested==0)
			update(dt);
	}

	virtual void TestVisible (const USER_DEFAULTS & defaults, const U32 currentSystem, const U32 currentPlayer)
	{
		bVisible = (systemID == currentSystem &&
				   (IsVisibleToPlayer(currentPlayer) ||
					 defaults.bVisibilityRulesOff ||
					 defaults.bEditorMode) );

		if (bVisible)
		{
			S32 screenX,screenY;
			CAMERA->PointToScreen(transform.translation,&screenX,&screenY);
			
			RECT _rect;
			
			_rect.left  = screenX - 15;
			_rect.right	= screenX + 15;
			_rect.top = screenY - 15;
			_rect.bottom = screenY + 15;
			
			RECT screenRect = { 0, 0, SCREENRESX, SCREENRESY };
			
			bVisible = RectIntersects(_rect, screenRect);
		}
	}

	virtual void CastVisibleArea(void)
	{
		SetVisibleToAllies(GetVisibilityFlags());
	}

	virtual void Render (void);

//	virtual void RenderMissile (void);

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

	/* IWeaponTarget */
	virtual void ApplyAOEDamage (U32 ownerID, U32 damageAmount);
	
	virtual BOOL32 ApplyDamage (IBaseObject * collider, U32 ownerID, const Vector & pos, const Vector &dir,U32 amount,BOOL32 shieldHit=1);

	virtual BOOL32 ApplyVisualDamage (IBaseObject * collider, U32 ownerID, const Vector & pos, const Vector &dir,U32 amount,BOOL32 shieldHit=1);

	virtual BOOL32 GetModelCollisionPosition (Vector &collide_point,Vector &dir,const Vector &start,const Vector &direction);

	virtual BOOL32 GetCollisionPosition (Vector &collide_point,Vector &dir,const Vector &start,const Vector &direction);

	virtual void AttachBlast(PARCHETYPE pBlast,const Vector &pos,const Vector &dir);

	virtual void AttachBlast(PARCHETYPE pBlast,const Transform & baseTrans);

//	virtual void GetExtentInfo (RECT **extents,SINGLE *z_step,SINGLE *z_min,U8 *slices);


	/* Missile methods */

	void init (const ProjectileArchetype & data);

	static Vector randomizeTargetPos (IBaseObject *target);

	BOOL32 setup_ang_velocity (SINGLE dt,SINGLE framerate_factor);

	void TurnOffTrail();
};
//---------------------------------------------------------------------------
//
static Missile * CreateMissile (ProjectileArchetype & data)
{
	Missile * missile = new ObjectImpl<Missile>;

	missile->FRAME_init(data);
	missile->init(data);
	missile->arch = &data;

	return missile;
}
//----------------------------------------------------------------------------------
//
void Missile::init (const ProjectileArchetype & arch)
{
	CQASSERT(arch.data->wpnClass == WPN_MISSILE);
	CQASSERT(arch.data->objClass == OC_WEAPON);

	objClass = OC_WEAPON;
	pArchetype = arch.pArchetype;
	PBlastType = arch.PBlastType;
	data = (BT_PROJECTILE_DATA *)arch.data;
//	if (data->MASS)
//		PHYSICS->set_mass(instanceIndex, data->MASS);

	wobble = (SINGLE(rand() % 256) / 256.0F) * 2*PI;
	if((arch.pEngineTrailType != 0) && CQEFFECTS.bWeaponTrails)
	{
		IBaseObject * obj = ARCHLIST->CreateInstance(arch.pEngineTrailType);
		if(obj)
		{
			obj->QueryInterface(IEngineTrailID,trail,NONSYSVOLATILEPTR);
			if(trail != 0)
			{
				trail->InitEngineTrail(this,instanceIndex);
			}
		}
	}	
}
//----------------------------------------------------------------------------------------------
//
void Missile::InitWeapon (IBaseObject * owner, const class TRANSFORM & orientation, IBaseObject * _target, U32 flags, const class Vector * pos)
{
	Vector vel;
	Vector targetPos;
	U32 visFlags = owner->GetTrueVisibilityFlags();

	systemID = owner->GetSystemID();
	ownerID =  owner->GetPartID();
	launchFlags = flags;
	bTargetingOff = false;

	initialPos = orientation.get_position();
	if (initialPos.z == 0)
		initialPos.z += 1;	// prevent later div by 0

	bVisible = 1;		// assume the best

//	bUp = (initialPos.z >= 0);

	/*if (pos)
		targetPos = *pos;
	else
		targetPos = randomizeTargetPos(_target);*/
	targetPos = randomizeTargetPos(_target);
//	targetPos = _target->GetPosition();
	if (pos)
	{
		targetPos -= _target->GetPosition();
		targetPos += *pos;
	}
	
	if (_target != 0)
	{
		_target->QueryInterface(IWeaponTargetID, target, SYSVOLATILEPTR);
#ifndef FINAL_RELEASE
		if (target==0)
		{
			MPart testing = _target;
			const char * name = (testing.isValid()) ? ((const char *)testing->partName) : "Unknown";
			CQBOMB2("Attempt to target non-targetable object. Target=%s, ObjClass=0x%X", name, _target->objClass);
		}
#endif // !FINAL_RELEASE

		targetID = _target->GetPartID();

		Transform trans = _target->GetTransform();
		relTargetPos = trans.inverse_rotate_translate(targetPos);
		visFlags |= _target->GetTrueVisibilityFlags();
		owner->SetVisibleToAllies(1 << (_target->GetPlayerID()-1));
		_target->SetVisibleToAllies(1 << (owner->GetPlayerID()-1));
	}

	SetVisibilityFlags(visFlags);
	transform = startTrans;

	Vector distance = targetPos - initialPos;
	SINGLE pitch = orientation.get_pitch();
	SINGLE yaw = get_angle(distance.x,distance.y)+0.75*SPREAD*cos(3*theta);

	transform.rotate_about_j(yaw);
  	ang_base = transform.get_i();

	pitch = __max((10*MUL_DEG_TO_RAD), pitch);
	transform.rotate_about_i(pitch);

	transform.set_position(initialPos);

	vel = -data->maxVelocity*transform.get_k();
	velocity = vel;

/*	SINGLE chord = distance.magnitude();
	range = chord + 1000;
	SINGLE sinA = distance.z/chord;
	chord = __min(chord, range) * 0.5F;
	chord = __max(chord, 1.0F);		// prevent div by 0

	SINGLE ang_vel = -data->maxVelocity * (sin(pitch)-sinA) / chord;

	ang_velocity = ang_base*ang_vel;*/

	//SETUP ROB'S NEW PHUNKY PHRESH PATH
	base = targetPos-initialPos;
	pathLength = base.magnitude();
	if (pathLength)
		base /= pathLength;

	range = pathLength+1000;

	Vector x(base.y,-base.x,0);
	up = cross_product(x,base);
	up.normalize();
	//missile will be dead when dot product of base and the position of the missile 
	//relative to the base point is greater than the pathLength(+some delta?)

	//do a geometry thing to figure out the chord
	SINGLE rel_pitch = orientation.get_pitch();
	rel_pitch -= TRANSFORM::get_pitch(base);
	SINGLE b = pathLength*0.5;
	SINGLE theta2 = PI/2-rel_pitch;
	if (theta2 > PI/2)
		theta2 = PI/2+rel_pitch;
	d = b*tan(theta2);
	r = sqrt(b*b+d*d);
	pathHeight = r-d;

 	hSound = SFXMANAGER->Open(data->launchSfx);
 	SFXMANAGER->Play(hSound,systemID,&orientation.translation);

	if(trail != 0)
		trail->Reset();
}

//---------------------------------------------------------------------------
//
BOOL32 Missile::ApplyDamage (IBaseObject * collider, U32 _owner, const Vector & pos, const Vector &dir,U32 amount,BOOL32 bShieldHit)
{
	SYSMAP->RegisterAttack(systemID,GetGridPosition(),MGlobals::GetPlayerFromPartID(ownerID));
	target = 0;
	bDeleteRequested = true;
	TurnOffTrail();

	return TRUE;  //don't want a hull hit
}
//---------------------------------------------------------------------------
//
BOOL32 Missile::ApplyVisualDamage (IBaseObject * collider, U32 ownerID, const Vector & pos, const Vector &dir,U32 amount,BOOL32 shieldHit)
{
	return true;
}
//---------------------------------------------------------------------------
//
void Missile::ApplyAOEDamage (U32 ownerID, U32 damageAmount)
{
	SYSMAP->RegisterAttack(systemID,GetGridPosition(),MGlobals::GetPlayerFromPartID(ownerID));
	target = 0;
	bDeleteRequested = true;
	TurnOffTrail();
}
//---------------------------------------------------------------------------
//
BOOL32 Missile::GetModelCollisionPosition (Vector &collide_point, Vector &dir, const Vector &start, const Vector &direction)
{
	return 0;
}
//---------------------------------------------------------------------------
//
BOOL32 Missile::GetCollisionPosition (Vector &collide_point, Vector &dir, const Vector &start, const Vector &direction)
{
	BOOL32 result = 0;

	missileBubble.transform(&missileBubble,transform.translation,transform);

	if (RayEllipse(collide_point,dir,start,direction,missileBubble))
		result = 1;

	return result;
}
//---------------------------------------------------------------------------
//
void Missile::AttachBlast(PARCHETYPE pBlast,const Vector &pos,const Vector &dir)
{
	return;
}
//---------------------------------------------------------------------------
//
void Missile::AttachBlast(PARCHETYPE pBlast,const Transform & baseTrans)
{
	return;
}
//---------------------------------------------------------------------------
//
/*void Missile::GetExtentInfo (RECT **extents,SINGLE *z_step,SINGLE *z_min,U8 *slices)
{
	CQASSERT(0 && "Missles don't have extents!");
	*extents=0;
	*z_step=0;
	*z_min=0;
	*slices=0;
}*/

//---------------------------------------------------------------------------
//  ROB'S NEW PHUNKY PHRESH PATH PHINDING
//
BOOL32 Missile::setup_ang_velocity (SINGLE dt,SINGLE framerate_factor)
{
	if (target!=0)
	{
		Transform trans = target.Ptr()->GetTransform();
		Vector targetPos = trans*relTargetPos;
	
		Vector relStart = targetPos-base*pathLength;
		Vector relPos = transform.translation-relStart;

		//detect if we've passed the target
		//if we have, give up seeking
		SINGLE progress = dot_product(relPos,base)+data->maxVelocity*dt;
		if (progress > pathLength+3000 || progress < 0 )
			return 0;
		else if (progress > pathLength)
		{
			//super special case - missile starts to expire once it has hit this condition
			pathLength -= 2000*dt;
			//
			if (ang_velocity.magnitude() > 0.35f)
			{
				ang_velocity.normalize();
				ang_velocity *= 0.3f;
			}
			//ang_velocity.set(0,0,0);
			return 1;
		}

		SINGLE pitch = transform.get_pitch();

		SINGLE x = r*r-pow(pathLength*0.5-progress,2);
		if (x < 0)
			return 0;

		SINGLE goalHeight = sqrt(x)-d;

		Vector goalPos;
	
		goalPos = relStart+(progress)*base+up*goalHeight;
//		CQASSERT(goalPos.z-transform.translation.z < 1000);

		Vector pos = transform.translation;
		SINGLE heading;
		SINGLE damper = __max(0.0f,1.7f*((pathLength-progress)/pathLength)-0.7f);
		damper *= framerate_factor;
		heading = atan2(goalPos.x-pos.x,goalPos.y-pos.y)+SPREAD*cos(3*theta)*damper;
		transform = startTrans;

		transform.rotate_about_j(heading);
		ang_base = transform.get_i();
		transform.rotate_about_i(pitch);
		transform.translation = pos;


		theta += (FREQ+FREQ*(rand()%1000)*0.001)*(progress)/pathLength;
		SINGLE desired_pitch = TRANSFORM::get_pitch(goalPos-transform.translation)+cos(4.83f*theta)*0.5*damper;

		SINGLE ang_vel = -4*(pitch-desired_pitch);

		if (ang_vel > 10)
			ang_vel = 10;
		if (ang_vel < -10)
			ang_vel = -10;

		ang_velocity = ang_vel*ang_base;

		return 1;
	}

	return 0;
}
//---------------------------------------------------------------------------------------------
// assumes that target is valid
//
Vector Missile::randomizeTargetPos (IBaseObject *target)
{
	OBJBOX tbox;
	const TRANSFORM & trans = target->GetTransform();
	Vector result;
	
	result.zero();
	if (target->GetObjectBox(tbox))
	{
		int num = rand() % 5;

		if (num)
		{
			switch (num)
			{
			case 1:
				result.x = tbox[0]*0.5F;	// maxx
				result.z = tbox[5]*0.5F;	// minz
				break;
			
			case 2:
				result.x = tbox[0]*0.5F;	// maxx
				result.z = tbox[4]*0.5F;	// maxz
				break;

			case 3:
				result.x = tbox[1]*0.5F;	// minx
				result.z = tbox[4]*0.5F;	// maxz
				break;

			case 4:
				result.x = tbox[1]*0.5F;	// minx
				result.z = tbox[5]*0.5F;	// minz
				break;
			}
			result = trans.rotate(result);
		}
	}

	result += trans.translation;
	return result;
}
//---------------------------------------------------------------------------
//
void Missile::SetTerrainFootprint (struct ITerrainMap * terrainMap)
{
	int new_map_square = OBJMAP->GetMapSquare(systemID,transform.translation);
	if (new_map_square != map_square)
	{
		OBJMAP->RemoveObjectFromMap(this,systemID,map_square);
		map_square = new_map_square;
		OBJMAP->AddObjectToMap(this,systemID,map_square,OM_AIR);
	}
}
//---------------------------------------------------------------------------
//
BOOL32 Missile::update (SINGLE dt)
{
	ENGINE->update_instance(instanceIndex,0, dt);		// update whole object, might have a particle effect

	Vector pos = transform.get_position();
	
	//if target has vanished, end missile
	if (target == 0)
		bDeleteRequested= true;
	
	pos -= initialPos;
	
	timer += dt;
	if (timer > UPDATE_INTERVAL)
	{
		SINGLE slice;
		if (dt > UPDATE_INTERVAL)
		{
			slice = timer;
			timer = 0.0f;
		}
		else
		{
			slice = UPDATE_INTERVAL;
			timer -= UPDATE_INTERVAL;
		}
		
		SINGLE ff = __min(__max((0.08-dt)/(0.08f-0.06f),0.0f),1.0f);

		if (setup_ang_velocity(slice,ff) == 0)
			bDeleteRequested = true;
	}
	
	Vector look = -transform.get_k();
	velocity = look * data->maxVelocity;
	
/*	if (!(launchFlags & IWF_ALWAYS_HIT))
	{
		//
		// check to make sure we haven't exceeded our max range
		//
		
		if (pos.magnitude() > range)
		bDeleteRequested = 1;
}*/
	
	Vector collide_point,dir;
	Vector backTrack = transform.translation-4000*look;
	
	if (target)
	{
		if (target->GetCollisionPosition(collide_point,dir,backTrack,look))
		{
			Vector diff = collide_point - transform.translation;
			
			if (dot_product(diff,look) < 0)
			{
				if ((launchFlags & IWF_ALWAYS_MISS)==0)
				{
					if (target->ApplyDamage(this, ownerID, backTrack,look,data->damage) == 0)
					{
						//	IBaseObject * blast = CreateBlast(PBlastType, transform, systemID);
						
						//	if (blast)
						//		OBJLIST->AddObject(blast);
						if (bVisible)
						{
							collide_point = transform.translation;
							target->GetModelCollisionPosition(collide_point,dir,backTrack,look);
							target->AttachBlast(PBlastType,collide_point,-look);
						}
					}
				}
				bDeleteRequested = TRUE;
			}
		}
	}
	
	if (bDeleteRequested)
	{
		velocity.set(0,0,0);
		ang_velocity.set(0,0,0);

		TurnOffTrail();
	}

/*	BOOL32 bTrailIsDone=1;
	//"EngineTrail"
	if(trail != 0)
	{
		bTrailIsDone = (trail->Update()==0);
	}
	else
	{
		//particle trail
		timeToLive -= ELAPSED_TIME;

		if (timeToLive > 0)
			bTrailIsDone = FALSE;
	}*/

	//return (bDeleteRequested==0 || bTrailIsDone==0);

	return 1;
}
//----------------------------------------------------------------------------------
//
void Missile::Render (void)
{
	if (bVisible)
	{
		if (bDeleteRequested == 0)
		{
			BATCH->set_render_state(D3DRS_ZWRITEENABLE,TRUE);
			BATCH->set_render_state(D3DRS_ZENABLE,TRUE);
			BATCH->set_render_state(D3DRS_ALPHABLENDENABLE,FALSE);
			//Vector pos = transform.get_position();
		//	RenderMissile();
			LIGHTS->DeactivateAllLights();
			TreeRender(mc,false);
		}

		if(trail != 0)
			trail->Render();
		/*else
		{
			INSTANCE_INDEX id=-1;
			id = ENGINE->get_instance_child_next(instanceIndex,EN_DONT_RECURSE,id);
			ENGINE->render_instance(MAINCAM,id,0,LODPERCENT,0,0);
		}*/
	}
}
#if 0
void Missile::RenderMissile()
{
	LIGHTS->ActivateBestLights(pos,8,4000);
	BATCH->set_render_state(D3DRS_ZENABLE,TRUE);
	BATCH->set_render_state(D3DRS_ZWRITEENABLE,TRUE);
	BATCH->set_render_state(D3DRS_ALPHABLENDENABLE,FALSE);
	
	INSTANCE_INDEX id,root_id;
	id = root_id = ENGINE->get_instance_root(instanceIndex);
	S32 mat_cnt=0;
	do
	{
		Mesh *mesh = REND->get_instance_mesh(id);
		
		if (mesh)
		{
			vis_state result = VS_UNKNOWN;
			
			const Transform &world_to_view = MAINCAM->get_inverse_transform() ;
			//
			// Don't bother with objects outside the view frustum.
			//
			// sphere center in world transformed to view
			Vector view_pos (world_to_view.rotate_translate(transform *
				mesh->sphere_center));
			
			result = MAINCAM->object_visibility(view_pos, mesh->radius);
			if (result == VS_NOT_VISIBLE || result == VS_SUB_PIXEL)
			{
				// Outside view frustum.
				return;
			}
			//!!!!!!! FIX
			/*	else if (result == VS_PARTIALLY_VISIBLE)
			{
			BATCH->set_render_state(D3DRS_CLIPPING,FALSE);
			}
			else // vs == ICamera::VS_FULLY_VISIBLE
			{
			BATCH->set_render_state(D3DRS_CLIPPING,TRUE);
		}*/
			
			
			LightRGB lit[100];
			
			Vector v[3],n[3];
			
			BATCH->set_render_state(D3DRS_CULLMODE,D3DCULL_CW);
			Transform inv = transform.get_inverse();
			S32 index_count=0;
			
			for (int fg_cnt=0;fg_cnt<mesh->face_group_cnt;fg_cnt++)
			{
				FaceGroup *fg = &mesh->face_groups[fg_cnt];
				index_count= fg->face_cnt*3;
				
				SetupDiffuseBlend(mesh->material_list[fg->material].texture_id,FALSE);
				BATCH->set_state(RPR_STATE_ID,mesh->material_list[fg->material].texture_id);
				CAMERA->SetModelView(&transform);
				unsigned int i;
				
				RPVertex *st_list = arch->st_lists[fg_cnt];
				U16 *index_list = arch->index_lists[fg_cnt];
				U16 id_list[120];
				LIGHT->light_vertices(lit,arch->vert_lists[fg_cnt],arch->norm_lists[fg_cnt],arch->vert_cnt[fg_cnt],&inv);
				for (i=0;i<arch->vert_cnt[fg_cnt];i++)
				{
					st_list[i].r = lit[i].r;
					st_list[i].g = lit[i].g;
					st_list[i].b = lit[i].b;
				}
				
#if 1
				index_count = 0;
				Vector cam_pos = CAMERA->GetPosition();
				Vector cam_pos_in_object_space = transform.inverse_rotate_translate(cam_pos);
				Vector *n;
				float cx = cam_pos_in_object_space.x;
				float cy = cam_pos_in_object_space.y;
				float cz = cam_pos_in_object_space.z;
				for (int j=0;j<fg->face_cnt;j++)
				{
					n = &arch->face_norm_lists[fg_cnt][j];
					SINGLE dot=cx*n->x+cy*n->y+cz*n->z;
					if (dot >= -fg->face_D_coefficient[j])
					{
						for (int vv=0;vv<3;vv++)
						{
							id_list[index_count] = index_list[j*3+vv];
							index_count++;
						}
					}
				}
#endif
				
				BATCH->draw_indexed_primitive( D3DPT_TRIANGLELIST, D3DFVF_RPVERTEX, st_list,
					arch->vert_cnt[fg_cnt], id_list, index_count,0);
			}
			BATCH->set_state(RPR_STATE_ID,0);
			mat_cnt++;
		}
		else if (CQEFFECTS.bWeaponTrails)
			ENGINE->render_instance(MAINCAM,id,0,LODPERCENT,0,0);
		
		if (id == root_id)
			id = INVALID_INSTANCE_INDEX;
		id = ENGINE->get_instance_child_next(root_id,EN_DONT_RECURSE,id);
	} while (id != INVALID_INSTANCE_INDEX);
}
#endif
//----------------------------------------------------------------------------------------------
//
/*
void Missile::OnCollision(IBaseObject *_target)
{
	if (_target && _target==target.ptr)
	{
		Vector pos;
		target->GetCollisionPosition(pos,transform.translation,-transform.get_k());
		target->ApplyDamage(this, pos, data->damage);
	}

	bDeleteRequested = TRUE;
}
*/

void Missile::TurnOffTrail()
{
	if (trail != 0)
		trail->SetDecayMode();

	INSTANCE_INDEX trailIndex = ENGINE->get_instance_child_next(instanceIndex,0,-1);
	
	if (trailIndex != -1)
	{
		timeToLive = 4.0f;
		COMPTR<IParticleSystem> IPS;
		
		if(ENGINE->query_instance_interface( trailIndex, IID_IParticleSystem, (IDAComponent **)(void **)IPS ) == GR_OK)
		{
			ParticleSystemParameters psp;
			IPS->get_parameters( &psp );
			psp.frequency = 0;
			IPS->set_parameters( &psp );
		}
	}
}
//----------------------------------------------------------------------------------------------
//-------------------------------class ProjectileManager--------------------------------------------
//----------------------------------------------------------------------------------------------
//
struct ArcCannon;
IBaseObject * CreateArcCannon (PARCHETYPE pArchetype, S32 archeIndex);
void CreateArcCannonArchetype();
void ReleaseArcCannonArchetype();
IBaseObject * CreateAutoCannon (PARCHETYPE pArchetype, S32 archeIndex, PARCHETYPE _PBlastType, PARCHETYPE _pSparkType);


struct DACOM_NO_VTABLE ProjectileManager : public IObjectFactory
{
	//
	// incoming interface map
	//
  
	BEGIN_DACOM_MAP_INBOUND(ProjectileManager)
	DACOM_INTERFACE_ENTRY(IObjectFactory)
	END_DACOM_MAP()

	U32 factoryHandle;

	//child object info
	ProjectileArchetype *pArchetype;

	//ProjectileManager methods

	ProjectileManager (void) 
	{
	}

	~ProjectileManager();
	
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
// ProjectileManager methods

ProjectileManager::~ProjectileManager()
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
void ProjectileManager::init()
{
	COMPTR<IDAConnectionPoint> connection;

	if (OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
		connection->Advise(GetBase(), &factoryHandle);
}
//--------------------------------------------------------------------------
//
HANDLE ProjectileManager::CreateArchetype(const char *szArchname, OBJCLASS objClass, void *_data)
{
	if (objClass == OC_WEAPON)
	{
		BT_PROJECTILE_DATA * data = (BT_PROJECTILE_DATA *)_data;
		switch (data->wpnClass)
		{
		case WPN_AUTOCANNON:
		case WPN_MISSILE:
		case WPN_BOLT:
			{
				ProjectileArchetype *newguy = new ProjectileArchetype;
			
				newguy->name = szArchname;
				newguy->data = data;
				newguy->pArchetype = ARCHLIST->GetArchetype(szArchname);
				if (data->blastType[0])
				{
					if ((newguy->PBlastType = ARCHLIST->LoadArchetype(data->blastType)) != 0)
						ARCHLIST->AddRef(newguy->PBlastType, OBJREFNAME);
				}
/*				if (data->sparkType[0])
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
				
			/*	if (file != 0 && (newguy->archIndex = ENGINE->create_archetype(((BT_PROJECTILE_DATA *)data)->fileName, file)) != INVALID_ARCHETYPE_INDEX)
				{
					//setup render
					INSTANCE_INDEX temp=ENGINE->create_instance2(newguy->archIndex,0);
					Mesh *mesh = REND->get_instance_mesh(temp);
					CQASSERT(mesh->face_group_cnt < 3);
					for (int fg_cnt=0;fg_cnt<mesh->face_group_cnt;fg_cnt++)
					{
						FaceGroup *fg = &mesh->face_groups[fg_cnt];
						
						int i;
						
						RPVertex *st_list = newguy->st_lists[fg_cnt];
						U16 *index_list = newguy->index_lists[fg_cnt];
						Vector *n = newguy->norm_lists[fg_cnt];
						Vector *v = newguy->vert_lists[fg_cnt];
						U16 refList[40];	
						memset(refList,0xff,sizeof(U16)*40);

						U16 currentRef=0,lastRef=0;
						for (i=0;i<fg->face_cnt;i++)
						{
							
							U16 ref[3],tref[3];
							
							newguy->face_norm_lists[fg_cnt][i] = mesh->normal_ABC[fg->face_normal[i]];

							ref[0] = mesh->vertex_batch_list[fg->face_vertex_chain[i*3]];
							ref[1] = mesh->vertex_batch_list[fg->face_vertex_chain[i*3+1]];
							ref[2] = mesh->vertex_batch_list[fg->face_vertex_chain[i*3+2]];
							
							tref[0] = mesh->texture_batch_list[fg->face_vertex_chain[i*3]];
							tref[1] = mesh->texture_batch_list[fg->face_vertex_chain[i*3+1]];
							tref[2] = mesh->texture_batch_list[fg->face_vertex_chain[i*3+2]];
							
							for (int vv = 0;vv<3;vv++)
							{
								CQASSERT(ref[vv] < 40);
								if (refList[ref[vv]] != 0xffff)
								{
									currentRef = refList[ref[vv]];
								//	CQASSERT(st_list[currentRef].u == mesh->texture_vertex_list[tref[vv]].u);
								//	CQASSERT(st_list[currentRef].v == mesh->texture_vertex_list[tref[vv]].v);
								}
								else
								{
									currentRef = lastRef;
									lastRef++;
									refList[ref[vv]] = currentRef;
								}
								st_list[currentRef].pos = mesh->object_vertex_list[ref[vv]];
								v[currentRef] = st_list[currentRef].pos;
								st_list[currentRef].u = mesh->texture_vertex_list[tref[vv]].u;
								st_list[currentRef].v = mesh->texture_vertex_list[tref[vv]].v;
								if (st_list[currentRef].u >= 0.97)
									st_list[currentRef].u -= 1.0;
								if (st_list[currentRef].v >= 0.97)
									st_list[currentRef].v -= 1.0;
								st_list[currentRef].a = 255;
								index_list[i*3+vv] = currentRef;
								n[currentRef] = mesh->normal_ABC[mesh->vertex_normal[ref[vv]]];
							}
						}
						newguy->vert_cnt[fg_cnt] = lastRef;
					}
					ENGINE->destroy_instance(temp);
				}
				else*/
				if (file==0 || (newguy->archIndex = ENGINE->create_archetype(((BT_PROJECTILE_DATA *)data)->fileName, file)) == INVALID_ARCHETYPE_INDEX)
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
BOOL32 ProjectileManager::DestroyArchetype(HANDLE hArchetype)
{
	ProjectileArchetype *deadguy = (ProjectileArchetype *)hArchetype;
//	BT_PROJECTILE_DATA *objData = deadguy->data;
	

	delete deadguy;

	return 1;
}
//--------------------------------------------------------------------------
//
IBaseObject * ProjectileManager::CreateInstance(HANDLE hArchetype)
{
	ProjectileArchetype *pWeapon = (ProjectileArchetype *)hArchetype;
	BT_PROJECTILE_DATA *objData = pWeapon->data;
//	INSTANCE_INDEX index=-1;
	
	if (objData->objClass == OC_WEAPON)
	{
		IBaseObject * obj = NULL;
		switch (objData->wpnClass)
		{
		case WPN_MISSILE:
			{
				obj = CreateMissile(*pWeapon);
			}
			break;
		case WPN_BOLT:
			{
				obj = CreateBolt(*pWeapon);
			}
			break;
		case WPN_AUTOCANNON:
			{
				obj = CreateAutoCannon(pWeapon->pArchetype,pWeapon->archIndex, pWeapon->PBlastType, NULL);
			}
			break;
		}

		return obj;
	}
	
	return 0;
}
//--------------------------------------------------------------------------
//
void ProjectileManager::EditorCreateInstance(HANDLE hArchetype,const MISSION_INFO & info)
{
	// not supported
}

static struct ProjectileManager *PROJECTILEMGR;
//----------------------------------------------------------------------------------------------
//
struct _projectiles : GlobalComponent
{
	virtual void Startup (void)
	{
		struct ProjectileManager *projectileMgr = new DAComponent<ProjectileManager>;
		PROJECTILEMGR = projectileMgr;
		AddToGlobalCleanupList((IDAComponent **) &PROJECTILEMGR);
	}

	virtual void Initialize (void)
	{
		PROJECTILEMGR->init();
	}
};

static _projectiles projectiles;

//--------------------------------------------------------------------------//
//------------------------------END Projectile.cpp--------------------------------//
//--------------------------------------------------------------------------//
