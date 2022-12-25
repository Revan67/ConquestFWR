//--------------------------------------------------------------------------//
//                                                                          //
//                                FancyLaunch.cpp                                //
//                                                                          //
//                  COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/FancyLaunch.cpp 43    10/04/00 8:35p Jasony $
*/			    
//------------------------------- #INCLUDES --------------------------------//
//--------------------------------------------------------------------------//

#include "pch.h"
#include <globals.h>

#include "FancyLaunch.h"
#include <DFancyLaunch.h>

#include "SuperTrans.h"
#include "ObjList.h"
#include "sfx.h"

#include "Mission.h"
#include "IMissionActor.h"
#include "Startup.h"
#include "ArchHolder.h"
#include "TerrainMap.h"
#include "MPart.h"
#include "MGlobals.h"
#include "GridVector.h"
#include "ObjMapIterator.h"
#include "OpAgent.h"

#include <Engine.h>
#include <TSmartPointer.h>
#include <IAnim.h>
#include <FileSys.h>
#include <TComponent.h>
#include <IConnection.h>

#include <stdlib.h>

#define MAX_TENDRILS 5 

//---------------------------------------------------------------------------
//
FancyLaunch::FancyLaunch (void) 
{
	fancyLaunchIndex = animIndex = barrelIndex = -1;
}
//---------------------------------------------------------------------------
//
FancyLaunch::~FancyLaunch (void)
{
	ANIM->release_script_inst(animIndex);
	animIndex = barrelIndex = -1;
}
//---------------------------------------------------------------------------
//
void FancyLaunch::init (PARCHETYPE _pArchetype, PARCHETYPE _pBoltType)
{
	const BT_FANCY_LAUNCH * data = (const BT_FANCY_LAUNCH *) ARCHLIST->GetArchetypeData(_pArchetype);

	CQASSERT(data);
	CQASSERT(data->type == LC_FANCY_LAUNCH);
	CQASSERT(data->objClass == OC_LAUNCHER);

	pArchetype = _pArchetype;
	objClass = OC_LAUNCHER;

	pBoltType = _pBoltType;
//	pBoltData = (BT_WEAPON_DATA *) ARCHLIST->GetArchetypeData(pBoltType);

	bSpecialWeapon = data->bSpecialWeapon;
	bTargetRequired = data->bTargetRequired;
	bWormWeapon = data->bWormHole;

	REFIRE_PERIOD = data->refirePeriod;
	CQASSERT(REFIRE_PERIOD!=0);
	supplyCost = data->supplyCost;
	animTime = data->animTime;
	effectDuration = data->effectDuration;
	refireDelay = (float(rand()) / RAND_MAX) * REFIRE_PERIOD + animTime;
	specialAbility = data->specialAbility;
}
//---------------------------------------------------------------------------
//
void FancyLaunch::InitLauncher (IBaseObject * _owner, S32 ownerIndex, S32 animArcheIndex, SINGLE range)
{
	const BT_FANCY_LAUNCH * data = (const BT_FANCY_LAUNCH *) ARCHLIST->GetArchetypeData(pArchetype);

	CQASSERT(_owner);
	_owner->QueryInterface(ILaunchOwnerID, owner, LAUNCHVOLATILEPTR);
	CQASSERT(owner!=0);

	if (data->hardpoint[0])
	{
		FindHardpoint(data->hardpoint, barrelIndex, hardpointinfo, ownerIndex);
	}
	
	if (barrelIndex == -1)
	{
		hardpointinfo.orientation.set_identity();
		barrelIndex = ownerIndex;
	}

	CQASSERT(barrelIndex != -1);

/*	if (data->joint[0])
	{
		jointIndex = findJoint(data->joint, ownerIndex);
		CQASSERT(jointIndex != -1);
	
		const Joint* jnt = MODEL->get_joint(jointIndex);
		CQASSERT (jnt);
		fancyLaunchIndex = jnt->child;
		CQASSERT (fancyLaunchIndex != -1);
	}*/

	Vector pos = _owner->GetPosition();
	HSOUND hSound = SFXMANAGER->Open(data->warmupSound);
	SFXMANAGER->Play(hSound,_owner->GetSystemID(),&pos);

	if (data->animation[0])
		animIndex = ANIM->create_script_inst(animArcheIndex, ownerIndex, data->animation);

	/*if (jointIndex != -1)
		ANG_VELOCITY = data->info.angVelocity * ELAPSED_TIME;
	else
		MAX_ANG_DEFLECTION = data->info.maxAngDeflection;*/
}
//---------------------------------------------------------------------------
//
const TRANSFORM & FancyLaunch::GetTransform (void) const
{
	getSighting(hardpointTransform);
	
	return hardpointTransform;
}
//---------------------------------------------------------------------------
//
SINGLE FancyLaunch::getDistanceToTarget (void)
{
	SINGLE result = 0;

	if (target)
		result = TRANSFORM::getDistance2D(target->GetTransform().get_position(), owner.Ptr()->GetPosition());

	return result;
}
//---------------------------------------------------------------------------
//
U32 FancyLaunch::hostShootTarget (SYNC_PACKET * packet)
{
	CQASSERT(refireDelay <= 0);
	bool bUseSlowRefire = false;
	IBaseObject * obj;
	U32 result = 0;
	
	getSighting(hardpointTransform);

	SINGLE RANGE = owner->GetWeaponRange();
	bool targetGood = false;
	if((attacking  == 1) &&((RANGE <0) || ((owner.Ptr()->GetPosition() - targetPos).magnitude_squared() < RANGE*RANGE)))
		targetGood = true;
	else if ((attacking == 2) && (target==0 || (target.Ptr()->IsVisibleToPlayer(GetPlayerID()) && target->GetSystemID() == GetSystemID() && getDistanceToTarget() < RANGE)))
	{
		targetGood = true;
	}
	
	if(targetGood)
	{
		if ((bUseSlowRefire = !owner->UseSupplies(supplyCost,bSpecialWeapon))==true)
		{
			// do not shoot when out of supplies
			bUseSlowRefire = false;
		}
		else // either we had supplies, or using slow refire rate
		{
			obj = ARCHLIST->CreateInstance(pBoltType);
			if (obj)
			{
				VOLPTR(IWeapon) bolt;
				VOLPTR(IAOEWeapon) aoeWeapon;

				if ((bolt = obj) != 0)
				{
					bolt->InitWeapon(owner.Ptr(), hardpointTransform, target, 0, &targetPos);
					result = 1;// need to force a sync packet to be sent 
					//sizeof(SYNC_PACKET);
				}
				else
				if ((aoeWeapon=obj) != 0)
				{
					aoeWeapon->InitWeapon(owner.Ptr(), hardpointTransform, target, 0, &targetPos);
					result = aoeWeapon->GetAffectedUnits(packet->objectIDs, NULL) *sizeof(U32);
					if(!result) result = 1;//need to force a sync packet
//					result = sizeof(SYNC_PACKET);
				}
				OBJLIST->AddObject(obj);
			}
		}
		if(bSpecialWeapon)
		{
			target = 0;
			attacking = 4;
		}

		refireDelay += REFIRE_PERIOD;
		refireDelay += animTime+effectDuration;
		effectTimer = effectDuration;
	}
	
	return result;
}
//---------------------------------------------------------------------------
//
void FancyLaunch::clientShootTarget (const SYNC_PACKET * packet)
{
	IBaseObject * obj;
	
	getSighting(hardpointTransform);
	owner->UseSupplies(supplyCost,bSpecialWeapon);

	obj = ARCHLIST->CreateInstance(pBoltType);
	if (obj)
	{
		VOLPTR(IWeapon) bolt;
		VOLPTR(IAOEWeapon) aoeWeapon;

		OBJLIST->AddObject(obj);
		if ((bolt = obj) != 0)
		{
			bolt->InitWeapon(owner.Ptr(), hardpointTransform, target, 0, &targetPos);
		}
		else
		if ((aoeWeapon=obj) != 0)
		{
			aoeWeapon->InitWeapon(owner.Ptr(), hardpointTransform, target, 0, &targetPos);
			aoeWeapon->SetAffectedUnits(packet->objectIDs, NULL);
		}
	}

	refireDelay += REFIRE_PERIOD;
	refireDelay += animTime+effectDuration;
	effectTimer = effectDuration;
	if(bSpecialWeapon)
		target = 0;
}
//---------------------------------------------------------------------------
//
BOOL32 FancyLaunch::Update (void)
{
	if(attacking ==4)
	{
		if(owner)
			owner->LauncherCancelAttack();
		attacking = 0;
	}

	if(bSpecialWeapon)
	{
		MPartNC part(owner.Ptr());
		if(part.isValid())
		{
			if((!(part->caps.specialAttackOk)) && (!(part->caps.specialEOAOk)) && (!(part->caps.specialAttackWormOk)))
			{
				BT_FANCY_LAUNCH * data = (BT_FANCY_LAUNCH *)(ARCHLIST->GetArchetypeData(pArchetype));
				if(data->neededTech.raceID)
				{
					if(MGlobals::GetCurrentTechLevel(owner.Ptr()->GetPlayerID()).HasTech(data->neededTech))
					{
						if(bTargetRequired)
							part->caps.specialAttackOk = true;
						else
							part->caps.specialEOAOk = true;

						if(bWormWeapon)
							part->caps.specialAttackWormOk = true;
					}
				}
				else
				{
					if(bTargetRequired)
						part->caps.specialAttackOk = true;
					else
						part->caps.specialEOAOk = true;
					if(bWormWeapon)
						part->caps.specialAttackWormOk = true;
				}
			}
		}
	}
	if (attacking == 2  && target == 0)
	{
		target = 0;
		attacking = 0;
	}
	if (attacking == 2)
		targetPos = target->GetTransform().get_position() + offsetVector;
	if (attacking)
	{
	/*	if (jointIndex != -1)
			swivelToTarget(targetPos);
		else*/
		if (target)
			recalculateOffsetVectorForDistance();
	}

	if (refireDelay > 0)
	{
		refireDelay -= ELAPSED_TIME;

		//start warm-up anim
		if (refireDelay < animTime && refireDelay+ELAPSED_TIME > animTime)
		{
			if (animIndex != -1)
				ANIM->script_start(animIndex, Animation::FORWARD, Animation::BEGIN);
		}

		//start warm-down anim
		if (effectTimer > 0)
		{
			effectTimer -= ELAPSED_TIME;
			if (effectTimer <= 0 && animIndex != -1)
			{
				ANIM->script_start(animIndex, Animation::BACKWARDS, Animation::END);
			}
		}
	}

	//attempt to force the sync data update if I should fire
	if(THEMATRIX->IsMaster())
	{
		if (((attacking && target) || (attacking == 1)) && refireDelay <= 0 && (owner.Ptr()->effectFlags.canShoot()) && (!owner.Ptr()->fieldFlags.suppliesLocked()))
		{
			SINGLE RANGE = owner->GetWeaponRange();
			bool targetGood = false;
			if((attacking  == 1) &&((RANGE <0) || ((owner.Ptr()->GetPosition() - targetPos).magnitude_squared() < RANGE*RANGE)))
				targetGood = true;
			else if ((attacking == 2) && (target==0 || (target.Ptr()->IsVisibleToPlayer(GetPlayerID()) && target->GetSystemID() == GetSystemID() && getDistanceToTarget() < RANGE)))
			{
				targetGood = true;
			}
			if(targetGood)
			{
				//the only down side is that if this unit does not fire for some reason durring the synce we will be thrashing the sync data call every update
				THEMATRIX->ForceSyncData(owner.ptr); //note: this may not update the launcher properly if a higher priority launcher steals the sync... see gunboat::getSyncLaunchers
			}
		}
	}
	return 1;
}
//---------------------------------------------------------------------------
//
void FancyLaunch::AttackPosition (const struct GRIDVECTOR * pos, bool bSpecial)
{
	if(bSpecial)
	{
		if (pos == 0)
			attacking = 0;
		else
		{
			targetPos = *pos;
			attacking = 1;
			target = 0;
			dwTargetID = 0;
			target = NULL;
		}
	}
}
//---------------------------------------------------------------------------
//
void FancyLaunch::AttackObject (IBaseObject * _target)
{
	if(!bSpecialWeapon)
	{
		if  (_target == 0)
			attacking = 0;
		else
		{
			dwTargetID = _target->GetPartID();
			_target->QueryInterface(IBaseObjectID, target, GetPlayerID());
			attacking = 2;
		}
	}
}
//---------------------------------------------------------------------------
//
void FancyLaunch::SpecialAttackObject (IBaseObject * obj)
{
	if(bSpecialWeapon)
	{
		if  (obj == 0)
			attacking = 0;
		else
		{
			dwTargetID = obj->GetPartID();
			obj->QueryInterface(IBaseObjectID, target, GetPlayerID());
			attacking = 2;
		}
	}
}
//---------------------------------------------------------------------------
//
void FancyLaunch::InformOfCancel()
{
	if(bSpecialWeapon && attacking)
	{
		attacking = 0;
	}
}
//---------------------------------------------------------------------------
//
void FancyLaunch::WormAttack (IBaseObject * obj)
{
	if(bWormWeapon)
	{
		if  (obj == 0)
			attacking = 0;
		else
		{
			dwTargetID = obj->GetPartID();
			obj->QueryInterface(IBaseObjectID, target, GetPlayerID());
			attacking = 2;
		}
	}
}
//---------------------------------------------------------------------------
//
S32 FancyLaunch::GetObjectIndex (void) const
{
	return barrelIndex;
}
//---------------------------------------------------------------------------
//
U32 FancyLaunch::GetPartID (void) const
{
	return owner.Ptr()->GetPartID();
}
//---------------------------------------------------------------------------
//
BOOL32 FancyLaunch::Save (struct IFileSystem * inFile)
{
	DAFILEDESC fdesc = "FANCY_LAUNCH_SAVELOAD";
	COMPTR<IFileSystem> file;
	BOOL32 result = 0;
	DWORD dwWritten;
	FANCY_LAUNCH_SAVELOAD save;

	fdesc.lpImplementation = "DOS";
	fdesc.dwDesiredAccess = GENERIC_READ|GENERIC_WRITE;
	fdesc.dwShareMode = 0;  // no sharing
	fdesc.dwCreationDistribution = CREATE_ALWAYS;

	if (inFile->CreateInstance(&fdesc, file) != GR_OK)
		goto Done;

	ENGINE->get_joint_state(fancyLaunchIndex, IE_JST_BASIC, &currentRot);
//	dwOwnerID = owner.ptr->GetPartID();
	if (target==0)
		dwTargetID =0;

	save = *static_cast<FANCY_LAUNCH_SAVELOAD *>(this);

	file->WriteFile(0, &save, sizeof(save), &dwWritten, 0);
	
	result = 1;

Done:	
	return result;
}
//---------------------------------------------------------------------------
//
BOOL32 FancyLaunch::Load (struct IFileSystem * inFile)
{
	DAFILEDESC fdesc = "FANCY_LAUNCH_SAVELOAD";
	COMPTR<IFileSystem> file;
	BOOL32 result = 0;
	DWORD dwRead;
	FANCY_LAUNCH_SAVELOAD load;
	U8 buffer[1024];

	fdesc.lpImplementation = "DOS";
	if (inFile->CreateInstance(&fdesc, file) != GR_OK)
		goto Done;

	file->ReadFile(0, buffer, sizeof(buffer), &dwRead, 0);
	MISSION->CorrelateSymbol("FANCY_LAUNCH_SAVELOAD", buffer, &load);

	ENGINE->set_joint_state(fancyLaunchIndex, IE_JST_BASIC, &load.currentRot);

	*static_cast<FANCY_LAUNCH_SAVELOAD *>(this) = load;
		
	result = 1;

Done:	
	return result;
}

//---------------------------------------------------------------------------
//
void FancyLaunch::ResolveAssociations()
{
	if (dwTargetID)
	{
		OBJLIST->FindObject(dwTargetID, GetPlayerID(), target);
//		CQASSERT(target!=0);
	}
/*	owner.ptr = OBJLIST->FindObject(dwOwnerID);*/
	CQASSERT(owner.Ptr());
	/*owner.ptr->QueryInterface(ILaunchOwnerID, owner);
	CQASSERT(owner!=0);*/
}
//---------------------------------------------------------------------------
// get the sighting vector of the barrel
//
void FancyLaunch::getSighting (TRANSFORM & result) const
{


	result.TRANSFORM::TRANSFORM(hardpointinfo.orientation, hardpointinfo.point);
	result = ENGINE->get_transform(barrelIndex).multiply(result);

/*	if (jointIndex == -1)	// if 360 degree fancyLaunch
	{
		SINGLE angle = TRANSFORM::get_yaw(targetPos - result.translation) - result.get_yaw();
		if (angle < -PI)
			angle += PI*2;
		else
		if (angle > PI)
			angle -= PI*2;

		if (fabs(angle) <= MAX_ANG_DEFLECTION)
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
	}*/
}

//---------------------------------------------------------------------------
//
/*S32 FancyLaunch::findChild (const char * pathname, INSTANCE_INDEX parent)
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

	while ((child = MODEL->get_child(parent, child)) != -1)
	{
		if (MODEL->is_named(child, buffer))
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
}
//----------------------------------------------------------------------------------------
//
BOOL32 FancyLaunch::findHardpoint (const char * pathname, INSTANCE_INDEX & index, HardpointInfo & hardpointinfo, INSTANCE_INDEX parent)
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
}*/

//---------------------------------------------------------------------------
//
void FancyLaunch::getPadPoints (VFX_POINT points[4], const TRANSFORM & transform, const OBJBOX & _box)
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
void FancyLaunch::recalculateOffsetVectorForDistance (void)
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
	}

	Vector goal = targetPos+offsetVector-position;
	goal.z = 0;
}
//---------------------------------------------------------------------------------------------
// assumes that target is valid
// minimize fabs(relYaw + rotAngle)
// where 'rotAngle' is angle that fancyLaunch has traveled already
//
void FancyLaunch::recalculateOffsetVectorForAngle (const Vector & barrelPos, SINGLE yaw, SINGLE rotAngle)
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
	}
}
//---------------------------------------------------------------------------
// return TRUE if we have a clear LineOfSight with the target
//
/*bool FancyLaunch::checkLOS (void)	// with targetPos
{
	VFX_POINT points[2];
	IBaseObject * colliderArray[16];
	Vector positionArray[16];
	int i, result;
	COMPTR<ITerrainMap> map;

	if (jointIndex == -1)	// no joint, check against max angle
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

	SECTOR->GetTerrainMap(GetSystemID(), map);
	points[0].x = hardpointTransform.translation.x;
	points[0].y = hardpointTransform.translation.y;
	points[1].x = targetPos.x;
	points[1].y = targetPos.y;
	result = map->TestSegment(points, colliderArray, positionArray, this);
	CQASSERT(result <= 16);

	i = result-1;
	while (i >= 0)
	{
		if ((colliderArray[i]->objClass & OC_PLANETOID) == 0)
			result--;
		i--;
	}

	return (result==0);
}*/
//---------------------------------------------------------------------------
//
inline IBaseObject * createFancyLaunch (PARCHETYPE pArchetype, PARCHETYPE pBoltType)
{
	FancyLaunch * fancyLaunch = new ObjectImpl<FancyLaunch>;

	fancyLaunch->init(pArchetype, pBoltType);

	return fancyLaunch;
}
//------------------------------------------------------------------------------------------
//---------------------------FancyLaunch Factory----------------------------------------------
//------------------------------------------------------------------------------------------
//
struct DACOM_NO_VTABLE FancyLaunchFactory : public IObjectFactory
{
	struct OBJTYPE 
	{
		PARCHETYPE pArchetype;
		PARCHETYPE pBoltType;

		void * operator new (size_t size)
		{
			return calloc(size,1);
		}

		OBJTYPE (void)
		{
		}

		~OBJTYPE (void)
		{
			if (pBoltType)
				ARCHLIST->Release(pBoltType, OBJREFNAME);
		}
	};
	//------------------------------------------------------------------------------------------
	//
	U32 factoryHandle;		// handles to callback

	//
	// Interface mapping
	//

	BEGIN_DACOM_MAP_INBOUND(FancyLaunchFactory)
	DACOM_INTERFACE_ENTRY(IObjectFactory)
	END_DACOM_MAP()

	FancyLaunchFactory (void) { }

	~FancyLaunchFactory (void);

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

	/* FancyLaunchFactory methods */

	IDAComponent * getBase (void)
	{
		return static_cast<IObjectFactory *> (this);
	}
};
//--------------------------------------------------------------------------//
//
FancyLaunchFactory::~FancyLaunchFactory (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (OBJLIST && OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
		connection->Unadvise(factoryHandle);
}
//--------------------------------------------------------------------------//
//
void FancyLaunchFactory::init (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
		connection->Advise(getBase(), &factoryHandle);
}
//-----------------------------------------------------------------------------
//
HANDLE FancyLaunchFactory::CreateArchetype (const char *szArchname, OBJCLASS objClass, void *_data)
{
	OBJTYPE * result = 0;

	if (objClass == OC_LAUNCHER)
	{
		BT_FANCY_LAUNCH * data = (BT_FANCY_LAUNCH *) _data;

		if (data->type == LC_FANCY_LAUNCH)	   
		{
			result = new OBJTYPE;
			result->pArchetype = ARCHLIST->GetArchetype(szArchname);
 
			result->pBoltType = ARCHLIST->LoadArchetype(data->weaponType);
			CQASSERT(result->pBoltType);
			ARCHLIST->AddRef(result->pBoltType, OBJREFNAME);
		}
	}

	return (HANDLE) result;
}
//-------------------------------------------------------------------
//
BOOL32 FancyLaunchFactory::DestroyArchetype (HANDLE hArchetype)
{
	OBJTYPE * objtype = (OBJTYPE *)hArchetype;

//	EditorStopObjectInsertion(objtype->pArchetype);		// not supported
	delete objtype;

	return 1;
}
//-------------------------------------------------------------------
//
IBaseObject * FancyLaunchFactory::CreateInstance (HANDLE hArchetype)
{
	OBJTYPE * objtype = (OBJTYPE *)hArchetype;
	return createFancyLaunch(objtype->pArchetype, objtype->pBoltType);
}
//-------------------------------------------------------------------
//
void FancyLaunchFactory::EditorCreateInstance (HANDLE hArchetype, const MISSION_INFO & info)
{
	// not supported
}
//---------------------------------------------------------------------------
//-------------------------GLOBAL STARTUP------------------------------------
//---------------------------------------------------------------------------
//
struct _fancyLaunch : GlobalComponent
{
	FancyLaunchFactory * sfactory;

	virtual void Startup (void)
	{
		sfactory = new DAComponent<FancyLaunchFactory>;
		AddToGlobalCleanupList((IDAComponent **) &sfactory);
	}

	virtual void Initialize (void)
	{
		sfactory->init();
	}
};

static _fancyLaunch __ship;

//---------------------------------------------------------------------------------------------
//-------------------------------End FancyLaunch.cpp------------------------------------------------
//---------------------------------------------------------------------------------------------
