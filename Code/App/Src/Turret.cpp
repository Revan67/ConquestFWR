//--------------------------------------------------------------------------//
//                                                                          //
//                                Turret.cpp                                //
//                                                                          //
//                  COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/Turret.cpp 103   9/13/01 10:01a Tmauer $
*/			    
//------------------------------- #INCLUDES --------------------------------//
//--------------------------------------------------------------------------//

#include "pch.h"
#include <globals.h>

#include "TObject.h"
#include <DTurret.h>

#include <3DMath.h>
#include <IHardPoint.h>
#include "ILauncher.h"
#include "IWeapon.h"
#include "TerrainMap.h"
#include "RangeFinder.h"
#include "IBlast.h"
#include "EffectPlayer.h"

#include <DWeapon.h>

#include "SuperTrans.h"
#include "ObjList.h"

#include "Mission.h"
#include "IMissionActor.h"
#include "Startup.h"
#include "ArchHolder.h"
#include "Sector.h"
#include "TerrainMap.h"
#include "Camera.h"
#include "MPart.h"
#include "GridVector.h"
#include "TManager.h"
#include "EventScheduler.h"

#include <Engine.h>
#include <TSmartPointer.h>
#include <IAnim.h>
#include <FileSys.h>
#include <TComponent.h>
#include <IConnection.h>

#include <stdlib.h>

struct TurretArchetype 
{
	PARCHETYPE pArchetype;
	PARCHETYPE pBoltType;
	PARCHETYPE pWarmUpType;
//	AnimArchetype * animBlastArch;
	INSTANCE_INDEX textureID;
	SINGLE muzzleFlashTime;
	SINGLE muzzleFlashSize;
	U8 flashRed,flashGreen,flashBlue;
	BT_TURRET *data;

	void * operator new (size_t size)
	{
		return HEAP->ClearAllocateMemory(size, "TurretArchetype");
	}

	void   operator delete (void *ptr)
	{
		HEAP->FreeMemory(ptr);
	}

	TurretArchetype (void)
	{
	}

	~TurretArchetype (void)
	{
		if (pBoltType)
			ARCHLIST->Release(pBoltType, OBJREFNAME);
		if (pWarmUpType)
			ARCHLIST->Release(pWarmUpType, OBJREFNAME);
		TMANAGER->ReleaseTextureRef(textureID);
//		if(animBlastArch)
//			delete animBlastArch;
	}
};


struct _NO_VTABLE Turret : IBaseObject, ILauncher, BASE_TURRET_SAVELOAD
{
	BEGIN_MAP_INBOUND(Turret)
	_INTERFACE_ENTRY(IBaseObject)
	_INTERFACE_ENTRY(ILauncher)
	END_MAP()

	//----------------------------------
	// TurretArchetype Handle
	//----------------------------------
	TurretArchetype * hArchetype;
	//----------------------------------
	// animation index
	//----------------------------------
	S32 animIndex;
	//----------------------------------
	// hardpoint data
	//----------------------------------
	HardpointInfo  hardpointinfo;
	INSTANCE_INDEX barrelIndex;
	mutable TRANSFORM	   hardpointTransform;
	//----------------------------------
	// Joint data
	//----------------------------------
	S32 turretIndex;
	//----------------------------------
	// Bolt data
	//----------------------------------
	union {
	SINGLE ANG_VELOCITY;
	SINGLE MAX_ANG_DEFLECTION;
	};
	SINGLE REFIRE_PERIOD;
	U32 supplyCost;
	SINGLE weaponVelocity;
	//----------------------------------
	// rangeFinder
	//----------------------------------
	RangeFinder rangeFinder;
	//----------------------------------
	// muzzle flash animation instance
	//----------------------------------
//	AnimInstance * muzzleAnim;
	SINGLE flashedFor;

	//
	// target we are shooting at
	//
	OBJPTR<IBaseObject> target;
	OBJPTR<ILaunchOwner> owner;	// person who created us

	OBJPTR<IBlast> warmUpBlast;

	bool bStartedFiring;

	//----------------------------------
	//----------------------------------
	
	Turret (void);

	~Turret (void);	

	/* IBaseObject methods */

	virtual const TRANSFORM & GetTransform (void) const;

	virtual BOOL32 Update (void);	// returning FALSE causes destruction

	virtual void PhysicalUpdate (SINGLE dt);

	virtual void Render(void);

	virtual S32 GetObjectIndex (void) const;

	virtual U32 GetPartID (void) const;

	virtual U32 GetSystemID (void) const
	{
		return owner.Ptr()->GetSystemID();
	}

	virtual U32 GetPlayerID (void) const
	{
		return owner.Ptr()->GetPlayerID();
	}

	/* ILauncher methods */

	virtual void InitLauncher (IBaseObject * owner, S32 ownerIndex, S32 animArcheIndex, SINGLE range);

	virtual void AttackPosition (const struct GRIDVECTOR * position, bool bSpecial);

	virtual void AttackObject (IBaseObject * obj);

	virtual void AttackMultiSystem (struct GRIDVECTOR * position, U32 targSystemID)
	{}

	virtual void WormAttack (IBaseObject * obj)
	{};

	virtual void DoCreateWormhole(U32 systemID)
	{}

	virtual const bool TestFightersRetracted (void) const 
	{ 
		return true;
	}

	virtual void SetFighterStance(FighterStance stance)
	{
	}

	virtual void HandlePreTakeover (U32 newMissionID, U32 troopID)
	{
		AttackObject(NULL);
	}

	virtual void TakeoverSwitchID (U32 newMissionID)
	{
	}

	virtual void DoSpecialAbility (U32 specialID)
	{
	}

	virtual void DoSpecialAbility (IBaseObject * obj)
	{
	}

	virtual void DoCloak (void)
	{
	}
	
	virtual void SpecialAttackObject (IBaseObject * obj)
	{
	}

	virtual void  GetSpecialAbility (UNIT_SPECIAL_ABILITY & ability, bool & bEnabled)
	{
		ability = USA_NONE;
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

	// the following methods are for network synchronization of realtime objects
	virtual U32 GetSyncDataSize (void) const
	{
		return 1;
	}

	virtual U32 GetSyncData (void * _buffer)
	{
		return 0;
	}

	virtual void PutSyncData (void * _buffer, U32 bufferSize)
	{
	}

	virtual void OnAllianceChange (U32 allyMask);

	virtual IBaseObject * FindChildTarget(U32 childID){return NULL;};

	/* Turret methods */

//	static S32 findChild (const char * pathname, INSTANCE_INDEX parent);
//	static S32 findJoint (const char * pathname, INSTANCE_INDEX parent);
//	static BOOL32 findHardpoint (const char * pathname, INSTANCE_INDEX & index, HardpointInfo & hardpointinfo, INSTANCE_INDEX parent);
	void getSighting (class TRANSFORM & result) const;
	void swivelToTarget (const Vector & target);
	void swivelToTargetRT (SINGLE dt);		// uses relYaw
	void init (TurretArchetype * _hArchetype);
	void recalculateOffsetVectorForDistance (void);
	void recalculateOffsetVectorForAngle (const Vector & barrelPos, SINGLE yaw, SINGLE rotAngle);
	static void getPadPoints (VFX_POINT points[4], const TRANSFORM & transform, const OBJBOX & _box);
	bool checkLOS (void);	// with targetPos
	SINGLE getDistanceToTarget (void);
};

//---------------------------------------------------------------------------
//
Turret::Turret (void) 
{
	turretIndex = animIndex = barrelIndex = -1;
}
//---------------------------------------------------------------------------
//
Turret::~Turret (void)
{
	if (ANIM)
		ANIM->release_script_inst(animIndex);
	animIndex = barrelIndex = -1;
	if (warmUpBlast)
		delete warmUpBlast.Ptr();
//	if(muzzleAnim)
//		delete muzzleAnim;
}
//---------------------------------------------------------------------------
//
void Turret::init (TurretArchetype * _hArchetype)
{
	hArchetype = _hArchetype;
	const BT_TURRET * data = _hArchetype->data;

	CQASSERT(data);
	CQASSERT(data->type == LC_TURRET);
	CQASSERT(data->objClass == OC_LAUNCHER);

	pArchetype = hArchetype->pArchetype;
	objClass = OC_LAUNCHER;

	BASE_WEAPON_DATA * pBaseWeapon = (BASE_WEAPON_DATA *) ARCHLIST->GetArchetypeData(hArchetype->pBoltType);

	switch (pBaseWeapon->wpnClass)
	{
	case WPN_MISSILE:
	case WPN_BOLT:
	case WPN_PLASMABOLT:
	case WPN_ANMBOLT:
		weaponVelocity = ((BT_PROJECTILE_DATA *)pBaseWeapon)->maxVelocity;
		break;
	case WPN_GATTLINGBEAM:
	case WPN_BEAM:
		weaponVelocity = 20000000;//((BT_BEAM_DATA *)pBaseWeapon)->velocity;
		break;
	case WPN_SPECIALBOLT:
		weaponVelocity = ((BT_SPECIALBOLT_DATA *)pBaseWeapon)->maxVelocity;
		break;
	case WPN_LASERSPRAY:
		weaponVelocity = ((BT_LASERSPRAY_DATA *)pBaseWeapon)->velocity;
		break;
	default:
		CQBOMB0("Unhandled weapon type");
		weaponVelocity = 1000;
		break;
	}

	REFIRE_PERIOD = data->refirePeriod;
	CQASSERT(REFIRE_PERIOD!=0);
	supplyCost = data->supplyCost;
	refireDelay = max((float(rand()) / RAND_MAX) * REFIRE_PERIOD,-data->warmUpBlast.triggerTime);
	CQASSERT(REFIRE_PERIOD >= -data->warmUpBlast.triggerTime && "Warmup anim trigger time is longer than refire");

	flashedFor = 0;
}
//---------------------------------------------------------------------------
//
void Turret::InitLauncher (IBaseObject * _owner, S32 ownerIndex, S32 animArcheIndex, SINGLE range)
{
	const BT_TURRET * data = (const BT_TURRET *) ARCHLIST->GetArchetypeData(pArchetype);

	CQASSERT(_owner);
	_owner->QueryInterface(ILaunchOwnerID, owner, LAUNCHVOLATILEPTR);
	CQASSERT(owner!=0);

	if (data->hardpoint[0])
		FindHardpoint(data->hardpoint, barrelIndex, hardpointinfo, ownerIndex);
	if (barrelIndex == -1)
		CQERROR1("Hardpoint '%s' not found!", data->hardpoint);
	if (data->joint[0])
	{
		turretIndex = FindChild(data->joint,ownerIndex);
		if (turretIndex == -1)
			CQERROR1("Child object '%s' not found!", data->joint);
	}

	if (data->animation[0])
		animIndex = ANIM->create_script_inst(animArcheIndex, ownerIndex, data->animation);

	if (turretIndex != -1)
		ANG_VELOCITY = data->info.angVelocity;
	else
		MAX_ANG_DEFLECTION = data->info.maxAngDeflection;
}
//---------------------------------------------------------------------------
//
const TRANSFORM & Turret::GetTransform (void) const
{
	getSighting(hardpointTransform);
	
	return hardpointTransform;
}
//---------------------------------------------------------------------------
//
SINGLE Turret::getDistanceToTarget (void)
{
	SINGLE result = 0;

	if (target)
		result = target->GetGridPosition() - owner.Ptr()->GetGridPosition();

	return result;
}
//---------------------------------------------------------------------------
//
static bool bUpdateSwivel = true;
static bool bRangeError = true;
BOOL32 Turret::Update (void)
{
	if (warmUpBlast)
	{
		if (warmUpBlast.Ptr()->Update() == 0)
		{
			delete warmUpBlast.Ptr();
			warmUpBlast = 0;
		}
	}

	if (target != 0)
	{
		targetPos = target->GetTransform().get_position() + offsetVector;
		if (bUpdateSwivel && (turretIndex != -1 && (refireDelay <= 0 || (rand() & 3)==0)))
		{
			if (target && target->GetSystemID() == GetSystemID())
				swivelToTarget(targetPos);
		}

		if (refireDelay <= 0)
		{
			bool bUseSlowRefire = false;
			if (fabs(relYaw) < 5 * MUL_DEG_TO_RAD)
			{
				IBaseObject * obj;
				VOLPTR(IWeapon) bolt;

				if (target && turretIndex == -1)
				{
					recalculateOffsetVectorForDistance();
					targetPos = target->GetTransform().get_position() + offsetVector;
				}

				getSighting(hardpointTransform);
				if ((target==0 || (target->GetSystemID()==GetSystemID() && target->IsVisibleToPlayer(owner.Ptr()->GetPlayerID()))) && 
					getDistanceToTarget() < (owner->GetWeaponRange()/GRIDSIZE) && checkLOS())
				{
					if (bStartedFiring == false && hArchetype->pWarmUpType)
					{
						// need start-up animation first
					}
					else
					if ((bUseSlowRefire = !owner->UseSupplies(supplyCost))==true)
					{
						// do not shoot when out of supplies
						bUseSlowRefire = false;
					}
					else if((owner.Ptr()->effectFlags.canShoot()) && (!owner.Ptr()->fieldFlags.suppliesLocked())) // either we had supplies, or using slow refire rate
					{
						//do the real damage
						U32 damage = 0;
						U32 weaponVelocity = 0;
						BASE_WEAPON_DATA * pBaseWeapon = (BASE_WEAPON_DATA *) ARCHLIST->GetArchetypeData(hArchetype->pBoltType);

						switch (pBaseWeapon->wpnClass)
						{
						case WPN_MISSILE:
						case WPN_BOLT:
						case WPN_PLASMABOLT:
						case WPN_ANMBOLT:
							damage = ((BT_PROJECTILE_DATA *)pBaseWeapon)->damage;
							weaponVelocity = ((BT_PROJECTILE_DATA *)pBaseWeapon)->maxVelocity;
							break;
						case WPN_GATTLINGBEAM:
						case WPN_BEAM:
							damage = ((BT_BEAM_DATA *)pBaseWeapon)->damage;
							weaponVelocity = 20000000;//((BT_BEAM_DATA *)pBaseWeapon)->velocity;
							break;
						case WPN_SPECIALBOLT:
							damage = ((BT_SPECIALBOLT_DATA *)pBaseWeapon)->damage;
							weaponVelocity = ((BT_SPECIALBOLT_DATA *)pBaseWeapon)->maxVelocity;
							break;
						case WPN_LASERSPRAY:
							weaponVelocity = ((BT_LASERSPRAY_DATA *)pBaseWeapon)->velocity;
							damage = ((BT_LASERSPRAY_DATA *)pBaseWeapon)->damage;
							break;
						default:
							CQBOMB0("Unhandled weapon type");
							damage = 0;
							break;
						}
						if(damage)
						{
							SINGLE dist = (targetPos-hardpointTransform.translation).fast_magnitude();

							SINGLE time = dist/weaponVelocity;
							SCHEDULER->QueueDamageEvent(time,owner.Ptr()->GetPartID(), target.Ptr()->GetPartID(), damage);
						}		
						//do the visual
						if(bVisible || target.Ptr()->bVisible)
						{
							if(OBJLIST->CreateProjectile())
							{
								obj = ARCHLIST->CreateInstance(hArchetype->pBoltType);
								if (obj)
								{
									OBJLIST->AddObject(obj);
									if ((bolt = obj) != 0)
									{
										Vector temp = targetPos;
										if (bRangeError && target)
										{
											rangeFinder.calcRangeError(this, target, owner.Ptr());
											temp = rangeFinder.calculateErrorPosition(targetPos);
										}
										bolt->InitWeapon(owner.Ptr(), hardpointTransform, target, 0, &temp);
										flashedFor = hArchetype->muzzleFlashTime;
		//								if(muzzleAnim)
		//									muzzleAnim->Restart();
									}
									if (animIndex != -1)
										ANIM->script_start(animIndex, Animation::FORWARD, Animation::BEGIN);
								}
								else
								{
									OBJLIST->ReleaseProjectile();//need to dec the ref count
								}
							}
						}
					}
					bStartedFiring = true;
				}
				else  // did not fire
					bStartedFiring = false;
			}
			refireDelay += REFIRE_PERIOD;
		}
		else
		{
			if (owner.Ptr()->bVisible && bStartedFiring && refireDelay+hArchetype->data->warmUpBlast.triggerTime <= 0 && warmUpBlast==0 && owner->UseSupplies(0,0))
			{
				refireDelay = max(refireDelay,-hArchetype->data->warmUpBlast.triggerTime);

				//do the visual
				if(bVisible || target.Ptr()->bVisible)
				{
					IBaseObject * obj;
					obj = ARCHLIST->CreateInstance(hArchetype->pWarmUpType);
					if (obj)
					{
						obj->QueryInterface(IBlastID,warmUpBlast,NONSYSVOLATILEPTR);
						CQASSERT(warmUpBlast && "Not a blast!!");
						TRANSFORM ownerTrans = owner.Ptr()->GetTransform();
						getSighting(hardpointTransform);
						TRANSFORM trans = (ownerTrans.get_inverse())*hardpointTransform;
						warmUpBlast->InitBlast(trans,owner.Ptr()->GetSystemID(),owner.Ptr());
					}
					else
					{
						OBJLIST->ReleaseProjectile();//need to dec the ref count
					}
				}
			}
			refireDelay -= ELAPSED_TIME;
		}
	}

	return 1;
}

void Turret::PhysicalUpdate (SINGLE dt)
{
	if(flashedFor != 0)
	{
		flashedFor -= dt;
		if(flashedFor < 0)
			flashedFor = 0;
	}

	if (turretIndex != -1 && target!=0)
		swivelToTargetRT(dt);

	if (warmUpBlast)
		warmUpBlast.Ptr()->PhysicalUpdate(dt);
}

//---------------------------------------------------------------------------
//
void Turret::Render(void)
{
	if (warmUpBlast)
		warmUpBlast->SetVisible(owner.Ptr()->bVisible != 0);

	if (owner.Ptr()->bVisible)
	{
		if(flashedFor != 0)
		{
			MPartNC part = owner.Ptr();
			int tech = part->techLevel.damage;
			
			TurretArchetype * tArch = hArchetype;
			SINGLE t = flashedFor/(tArch->muzzleFlashTime);
			SINGLE size;
			U8 alpha;
			//		if(t > 0.5)//getting bigger
			//		{
			//			size = (tArch->muzzleFlashSize)*(1-t);
			//			alpha = 255;
			//		}
			//		else//getting smaller an fading
			//		{
			size = (1+0.3*tech)*(tArch->muzzleFlashSize)*t;
			alpha = (t*256);
			//		}
			
			
			getSighting(hardpointTransform);
			Vector epos = hardpointTransform.translation;		
			
			Vector cpos (CAMERA->GetPosition());
			
			Vector look (epos - cpos);
			
			Vector k = look.normalize();
			
			Vector tmpUp(epos.x,epos.y,epos.z+50000);
			
			Vector j (cross_product(k,tmpUp));
			j.normalize();
			
			Vector i (cross_product(j,k));
			
			i.normalize();
			
			TRANSFORM trans;
			trans.set_i(i);
			trans.set_j(j);
			trans.set_k(k);
			
			i = trans.get_i();
			j = trans.get_j();
			
			Vector v[4];
			v[0] = epos - i*size - j*size;
			v[1] = epos + i*size - j*size;
			v[2] = epos + i*size + j*size;
			v[3] = epos - i*size + j*size;
			
			BATCH->set_state(RPR_BATCH,true);
			CAMERA->SetModelView();
			//		BATCH->set_texture_stage_texture( 0,((TurretArchetype *)hArchetype)->textureID);
			SetupDiffuseBlend(hArchetype->textureID,TRUE);
			BATCH->set_state(RPR_STATE_ID,hArchetype->textureID);
			BATCH->set_render_state(D3DRS_ZENABLE,TRUE);
			BATCH->set_render_state(D3DRS_ZWRITEENABLE,TRUE);
			BATCH->set_render_state(D3DRS_ALPHABLENDENABLE,TRUE);
			BATCH->set_render_state(D3DRS_SRCBLEND,D3DBLEND_ONE);
			BATCH->set_render_state(D3DRS_DESTBLEND,D3DBLEND_ONE);
			BATCH->set_render_state(D3DRS_CULLMODE,D3DCULL_NONE);
			
			PB.Begin(PB_QUADS);
			
			PB.Color4ub(tArch->flashRed,tArch->flashGreen,tArch->flashBlue,alpha);
			PB.TexCoord2f(0,0);   PB.Vertex3f(v[0].x,v[0].y,v[0].z);
			PB.TexCoord2f(1,0);   PB.Vertex3f(v[1].x,v[1].y,v[1].z);
			PB.TexCoord2f(1,1);   PB.Vertex3f(v[2].x,v[2].y,v[2].z);
			PB.TexCoord2f(0,1);   PB.Vertex3f(v[3].x,v[3].y,v[3].z);
			
			PB.End();
			
			BATCH->set_state(RPR_STATE_ID,0);
		}
		
		if (warmUpBlast)
			warmUpBlast.Ptr()->Render();
	}
/*	if((owner.ptr->bVisible) && (GetSystemID()== SECTOR->GetCurrentSystem()) && muzzleAnim && muzzleAnim->retrieve_current_frame())
	{		
		getSighting(hardpointTransform);
		BATCH->set_render_state(D3DRS_ZWRITEENABLE,FALSE);
		BATCH->set_render_state(D3DRS_ALPHABLENDENABLE,TRUE);
		BATCH->set_render_state(D3DRS_SRCBLEND,D3DBLEND_ONE);
		BATCH->set_render_state(D3DRS_DESTBLEND,D3DBLEND_ONE);
		ANIM2D->render(muzzleAnim,&hardpointTransform);

	}
*/
}

//---------------------------------------------------------------------------
//
void Turret::AttackPosition (const struct GRIDVECTOR * pos, bool bSpecial)
{
/*	if (pos == 0)
		attacking = 0;
	else
	{
		targetPos = *pos;
		attacking = 1;
		target = 0;
	}
*/
}
//---------------------------------------------------------------------------
//
void Turret::AttackObject (IBaseObject * _target)
{
	if  (_target == 0)
	{
		if (warmUpBlast)
		{
			delete warmUpBlast.Ptr();
			warmUpBlast = 0;
		}
		target = 0;
		dwTargetID = 0;
	}
	else
	{
		dwTargetID = _target->GetPartID();
		_target->QueryInterface(IBaseObjectID, target, GetPlayerID());

		if (bStartedFiring == 0)
			refireDelay = max(refireDelay,-hArchetype->data->warmUpBlast.triggerTime);
	}
}
//---------------------------------------------------------------------------
//
S32 Turret::GetObjectIndex (void) const
{
	if (turretIndex != -1)
		return turretIndex;
	return barrelIndex;
}
//---------------------------------------------------------------------------
//
U32 Turret::GetPartID (void) const
{
	return owner.Ptr()->GetPartID();
}
//---------------------------------------------------------------------------
//
void Turret::OnAllianceChange (U32 allyMask)
{
	if (target)
	{
		// do we happen to be attacking a new friend?
		const U32 hisPlayerID = target->GetPlayerID() & PLAYERID_MASK;
						
		// if we are now allied with the target then stop shooting at him
		if (hisPlayerID && (((1 << (hisPlayerID-1)) & allyMask) != 0))
		{
			target = 0;
			dwTargetID = 0;
		}
	}
}
//---------------------------------------------------------------------------
// get the sighting vector of the barrel
//
void Turret::getSighting (TRANSFORM & result) const
{
	/*
	const Matrix matrix = ENGINE->get_orientation(index);

	pos = matrix * pos;
	pos += ENGINE->get_position(index);

	orientation = matrix * orientation;
	*/

	result.TRANSFORM::TRANSFORM(hardpointinfo.orientation, hardpointinfo.point);
	result = ENGINE->get_transform(barrelIndex).multiply(result);

	if (turretIndex == -1)	// if 360 degree turret
	{
		SINGLE angle = TRANSFORM::get_yaw(targetPos - result.translation) - result.get_yaw();
		if (angle < -PI)
			angle += PI*2;
		else
		if (angle > PI)
			angle -= PI*2;

		if (MAX_ANG_DEFLECTION<0 || fabs(angle) <= MAX_ANG_DEFLECTION)
		{
			SINGLE pitch = result.get_pitch();		// preserve pitch
			Vector pos = result.get_position();
			Vector distance = targetPos - pos;

			SINGLE yaw = get_angle(distance.x,distance.y);

			result.set_identity();
			result.rotate_about_i(90 * MUL_DEG_TO_RAD);
			result.rotate_about_j(yaw);
			result.rotate_about_i(pitch);
			result.set_position(pos);
		}
	}
}
//---------------------------------------------------------------------------
// sets relYaw as a side-effect.
//
void Turret::swivelToTarget (const Vector & targetPos)
{
	SINGLE rot;
	TRANSFORM transform = ENGINE->get_transform(turretIndex);
	Vector pos = transform.get_position();
	SINGLE yaw = transform.get_yaw();
	Vector goal = targetPos - pos;
	ENGINE->get_joint_state(turretIndex, IE_JST_BASIC, &rot);
	goal.z = 0;

	recalculateOffsetVectorForAngle(pos, yaw, -rot);		// moves targetPos for next time
	relYaw = get_angle(goal.x, goal.y) - yaw;
	if (relYaw < -PI)
		relYaw += PI*2;
	else
	if (relYaw > PI)
		relYaw -= PI*2;
}
//---------------------------------------------------------------------------
//
void Turret::swivelToTargetRT (SINGLE dt)
{
	//
	// is goal yaw in no-man's land?
	//
	BOOL32 noman = 0;
	JointInfo const *jnt = ENGINE->get_joint_info(turretIndex);
	BOOL32 limited = (jnt->max0 - jnt->min0 < 359.0 * MUL_DEG_TO_RAD);
	SINGLE rot, origRot;
	ENGINE->get_joint_state(turretIndex, IE_JST_BASIC, &rot);
	origRot = rot;

	if (limited)		// does joint really have a limit?
	{
		if (relYaw < 0)	// turn to the left needed
		{
			if (rot - relYaw > jnt->max0)
			{
				if (rot - (relYaw+PI*2) < jnt->min0)
					noman = 1;		// damned anyway
				else
					relYaw += PI*2;		// go the long way
			}

		}
		else // turn to the right needed
		{
			if (rot - relYaw < jnt->min0)
			{
				if (rot - (relYaw - PI*2) > jnt->max0)
					noman = 1;		// damned anyway
				else
					relYaw -= PI*2;	// go the long way
			}
		}
	}

	if (noman == 0)
	{
		if (relYaw < 0)			// turn to the left needed
		{
			SINGLE min = __min(ANG_VELOCITY*dt, -relYaw);
			rot += min;
			relYaw += min;
		}
		else
		{
			SINGLE min = -__min(ANG_VELOCITY*dt, relYaw);
			rot += min;
			relYaw += min;
		}

		if (limited)
		{
			if (rot > jnt->max0)
				rot = jnt->max0;
			else
			if (rot < jnt->min0)
				rot = jnt->min0;
		}
		else
		{
			if (rot < -PI)
				rot += PI*2;
			else
			if (rot > PI)
				rot -= PI*2;
		}

		if (rot != origRot)
		{
			ENGINE->set_joint_state(turretIndex, IE_JST_BASIC, &rot);
			if (owner.Ptr()->bVisible==0 || LODPERCENT==0)		// won't move the turret otherwise!
				ENGINE->update_instance(owner.Ptr()->GetObjectIndex(), 0,0);
		}
	}
}
//---------------------------------------------------------------------------
//
/*
S32 Turret::findChild (const char * pathname, INSTANCE_INDEX parent)
{
	S32 index = -1;
	char buffer[MAX_PATH];
	const char *ptr=pathname, *ptr2;
	INSTANCE_INDEX child=-1;

	if (ptr[0] == '\\')
		ptr++;

	if ((ptr2 = strchr(ptr, '\\')) == 0)
	{
		strcpy(buffer, ptr);
	}
	else
	{
		memcpy(buffer, ptr, ptr2-ptr);
		buffer[ptr2-ptr] = 0;		// null terminating
	}

	while ((child = ENGINE->get_child(parent, child)) != -1)
	{
		const char *name = MODEL->get_name(child);
		if (strcmp(name, buffer) == 0)
		{
			if (ptr2)
			{
				// found the child, go deeper if needed
				parent = child;
				child = -1;
				ptr = ptr2+1;
				if ((ptr2 = strchr(ptr, '\\')) == 0)
				{
					strcpy(buffer, ptr);
				}
				else
				{
					memcpy(buffer, ptr, ptr2-ptr);
					buffer[ptr2-ptr] = 0;		// null terminating
				}
			}
			else
			{
				index = child;
				break;
			}
		}
	}

	return index;
}*/
//----------------------------------------------------------------------------------------
//
/*BOOL32 Turret::findHardpoint (const char * pathname, INSTANCE_INDEX & index, HardpointInfo & hardpointinfo, INSTANCE_INDEX parent)
{
	BOOL32 result=0;
	char buffer[MAX_PATH];
	char *ptr=buffer;

	strcpy(buffer, pathname);
	if ((ptr = strrchr(ptr, '\\')) == 0)
		goto Done;

	*ptr++ = 0;
	if (buffer[0])
		index = findChild(buffer, parent);
	else
		index = parent;

Done:	
	if (index != -1)
	{
		HARCH arch = index;

		if ((result = HARDPOINT->retrieve_hardpoint_info(arch, ptr, hardpointinfo)) == false)
			index = -1;		// invalidate result
	}

	return result;
}
//---------------------------------------------------------------------------
//
S32 Turret::findJoint (const char * pathname, INSTANCE_INDEX parent)
{
	S32 result;
	S32 child;
	char buffer[MAX_PATH];
	char *ptr=buffer;

	result = child = -1;
	if (pathname[0] == '\\')
		pathname++;
	strcpy(buffer, pathname);
	if ((ptr = strrchr(ptr, '\\')) == 0)
	{
		// parent is the root
		child  = findChild(buffer, parent);
	}
	else
	{
		*ptr++ = 0;
		if ((parent = findChild(buffer, parent)) != -1)
			child = findChild(ptr, parent);
	}

	if (parent != -1 && child != -1)
		if ((result = MODEL->find_joint(parent, child)) != -1)
		{
			CQASSERT(MODEL->get_joint_data_size(MODEL->get_joint(result)->type) == 1 && "Bad DOF data for joint");		// sizeof 1 SINGLE
		}
	
	return result;
}*/
//---------------------------------------------------------------------------
//
void Turret::getPadPoints (VFX_POINT points[4], const TRANSFORM & transform, const OBJBOX & _box)
{
	//
	// calculate points (in clock-wise order) for terrain foot-pad system
	//
	Vector pt, wpt;

	pt.y = 0;
	pt.x = _box[0];	// maxx
	pt.z = _box[5];	// minz

	wpt = transform.rotate_translate(pt);
	points[0].x = wpt.x;
	points[0].y = wpt.y;

	pt.z = _box[4];	// maxz

	wpt = transform.rotate_translate(pt);
	points[1].x = wpt.x;
	points[1].y = wpt.y;

	pt.x = _box[1];	// minx

	wpt = transform.rotate_translate(pt);
	points[2].x = wpt.x;
	points[2].y = wpt.y;

	pt.z = _box[5];	// minz

	wpt = transform.rotate_translate(pt);
	points[3].x = wpt.x;
	points[3].y = wpt.y;
}
//---------------------------------------------------------------------------------------------
// assumes that target is valid
//
void Turret::recalculateOffsetVectorForDistance (void)
{
	OBJBOX tbox;
	VFX_POINT points[4];
	const TRANSFORM & trans = target->GetTransform();
	const Vector position = ENGINE->get_position(barrelIndex);
	
	offsetVector.zero();
	if (target->GetObjectBox(tbox))
	{
		SINGLE minDist;
		int i, j=-1;

		getPadPoints(points, trans, tbox);

		minDist =  (trans.translation.x - position.x) * (trans.translation.x - position.x) + 
				   (trans.translation.y - position.y) * (trans.translation.y - position.y);

		for (i = 0; i < 4; i++)
		{
			SINGLE tmin;

			tmin =  (points[i].x - position.x) * (points[i].x - position.x) + 
					(points[i].y - position.y) * (points[i].y - position.y);

			if (tmin < minDist)
			{
				j = i;
				minDist = tmin;
			}
		}

		if (j >= 0)
		{
			offsetVector.x = (points[j].x - trans.translation.x) * 0.5F;
			offsetVector.y = (points[j].y - trans.translation.y) * 0.5F;
		}

		//
		// lead the target
		//
		offsetVector += RangeFinder::leadTargetFast(position, trans.translation, target.Ptr()->GetVelocity(), weaponVelocity);
	}

	Vector goal = targetPos+offsetVector-position;
	goal.z = 0;
}
//---------------------------------------------------------------------------------------------
// assumes that target is valid
// minimize fabs(relYaw + rotAngle)
// where 'rotAngle' is angle that turret has traveled already
//
void Turret::recalculateOffsetVectorForAngle (const Vector & barrelPos, SINGLE yaw, SINGLE rotAngle)
{
	OBJBOX tbox;
	VFX_POINT points[4];
	const TRANSFORM & trans = target->GetTransform();
	SINGLE angle;
	Vector goal;
	
	offsetVector.zero();
	if (target->GetObjectBox(tbox))
	{
		SINGLE minAngle;
		int i, j=-1;

		getPadPoints(points, trans, tbox);

		goal = trans.translation - barrelPos;
		angle = get_angle(goal.x, goal.y) - yaw;
		if (angle < -PI)
			angle += PI*2;
		else
		if (angle > PI)
			angle -= PI*2;

		minAngle = fabs(angle + rotAngle);

		for (i = 0; i < 4; i++)
		{
			angle = get_angle(points[i].x-barrelPos.x, points[i].y-barrelPos.y) - yaw;
			if (angle < -PI)
				angle += PI*2;
			else
			if (angle > PI)
				angle -= PI*2;
			angle = fabs(angle + rotAngle);

			if (angle < minAngle)
			{
				j = i;
				minAngle = angle;
			}
		}

		if (j >= 0)
		{
			offsetVector.x = (points[j].x - trans.translation.x) * 0.5F;
			offsetVector.y = (points[j].y - trans.translation.y) * 0.5F;
		}

		//
		// lead the target
		//
		offsetVector += RangeFinder::leadTargetFast(barrelPos, trans.translation, target.Ptr()->GetVelocity(), weaponVelocity);
	}
}
//---------------------------------------------------------------------------
// return TRUE if we have a clear LineOfSight with the target
//
bool Turret::checkLOS (void)	// with targetPos
{
	if (turretIndex == -1 && MAX_ANG_DEFLECTION >= 0)	// no joint, check against max angle
	{
		SINGLE angle = TRANSFORM::get_yaw(targetPos - hardpointTransform.translation) - hardpointTransform.get_yaw();
		if (angle < -PI)
			angle += PI*2;
		else
		if (angle > PI)
			angle -= PI*2;

		if (fabs(angle) > MAX_ANG_DEFLECTION)
			return false;
	}

	CQASSERT(target);
	GRIDVECTOR vec;
	vec = target->GetGridPosition();
	return owner->TestLOS(vec);
}
//---------------------------------------------------------------------------
//
inline IBaseObject * createTurret (TurretArchetype * hArchetype)
{
	Turret * turret = new ObjectImpl<Turret>;

	turret->init(hArchetype);

	return turret;
}
//------------------------------------------------------------------------------------------
//---------------------------Turret Factory----------------------------------------------
//------------------------------------------------------------------------------------------
//
struct DACOM_NO_VTABLE TurretFactory : public IObjectFactory
{
	//------------------------------------------------------------------------------------------
	//
	U32 factoryHandle;		// handles to callback

	//
	// Interface mapping
	//

	BEGIN_DACOM_MAP_INBOUND(TurretFactory)
	DACOM_INTERFACE_ENTRY(IObjectFactory)
	END_DACOM_MAP()

	TurretFactory (void) { }

	~TurretFactory (void);

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

	/* TurretFactory methods */

	IDAComponent * getBase (void)
	{
		return static_cast<IObjectFactory *> (this);
	}
};
//--------------------------------------------------------------------------//
//
TurretFactory::~TurretFactory (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (OBJLIST && OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
		connection->Unadvise(factoryHandle);
}
//--------------------------------------------------------------------------//
//
void TurretFactory::init (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
		connection->Advise(getBase(), &factoryHandle);
}
//-----------------------------------------------------------------------------
//
HANDLE TurretFactory::CreateArchetype (const char *szArchname, OBJCLASS objClass, void *_data)
{
	TurretArchetype * result = 0;

	if (objClass == OC_LAUNCHER)
	{
		BT_TURRET * data = (BT_TURRET *) _data;

		if (data->type == LC_TURRET)	   
		{
			result = new TurretArchetype;
			result->pArchetype = ARCHLIST->GetArchetype(szArchname);
 
			result->pBoltType = ARCHLIST->LoadArchetype(data->weaponType);
			CQASSERT(result->pBoltType);
			ARCHLIST->AddRef(result->pBoltType, OBJREFNAME);

			if (data->warmUpBlast.blastType[0])
			{
				result->pWarmUpType = ARCHLIST->LoadArchetype(data->warmUpBlast.blastType);
				CQASSERT(result->pWarmUpType);
				ARCHLIST->AddRef(result->pWarmUpType, OBJREFNAME);
			}

			result->muzzleFlashSize = data->muzzleFlashWidth;
			result->muzzleFlashTime = data->muzzleFlashTime;
			result->flashRed = data->colorMod.red;
			result->flashGreen = data->colorMod.green;
			result->flashBlue = data->colorMod.blue;
			result->data = data;
			if (data->animMuzzleFlash[0])
			{
				result->textureID = TMANAGER->CreateTextureFromFile(data->animMuzzleFlash, TEXTURESDIR, DA::TGA,PF_4CC_DAA4);

/*				DAFILEDESC fdesc;
				COMPTR<IFileSystem> objFile;
				fdesc.lpFileName = data->animMuzzleFlash;
				if (OBJECTDIR->CreateInstance(&fdesc, objFile) == GR_OK)
				{
					result->animBlastArch = ANIM2D->create_archetype(objFile);
				}
				else 
				{
					CQFILENOTFOUND(fdesc.lpFileName);
					result->animBlastArch =0;
				}
*/			}
			else
			{
				result->textureID = 0;
			}
			
		}
	}

	return (HANDLE) result;
}
//-------------------------------------------------------------------
//
BOOL32 TurretFactory::DestroyArchetype (HANDLE hArchetype)
{
	TurretArchetype * objtype = (TurretArchetype *)hArchetype;

//	EditorStopObjectInsertion(objtype->pArchetype);		// not supported
	delete objtype;

	return 1;
}
//-------------------------------------------------------------------
//
IBaseObject * TurretFactory::CreateInstance (HANDLE hArchetype)
{
	return createTurret((TurretArchetype *)hArchetype);
}
//-------------------------------------------------------------------
//
void TurretFactory::EditorCreateInstance (HANDLE hArchetype, const MISSION_INFO & info)
{
	// not supported
}
//---------------------------------------------------------------------------
//-------------------------GLOBAL STARTUP------------------------------------
//---------------------------------------------------------------------------
//
struct _turret : GlobalComponent
{
	TurretFactory * sfactory;

	virtual void Startup (void)
	{
		sfactory = new DAComponent<TurretFactory>;
		AddToGlobalCleanupList((IDAComponent **) &sfactory);
	}

	virtual void Initialize (void)
	{
		sfactory->init();
	}
};

static _turret __ship;

//---------------------------------------------------------------------------------------------
//-------------------------------End Turret.cpp------------------------------------------------
//---------------------------------------------------------------------------------------------
