//--------------------------------------------------------------------------//
//                                                                          //
//                              AirDefense.cpp                              //
//                                                                          //
//                  COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/AirDefense.cpp 56    9/13/01 10:01a Tmauer $
*/			    
//------------------------------- #INCLUDES --------------------------------//
//--------------------------------------------------------------------------//

#include "pch.h"
#include <globals.h>


#include "TObject.h"
#include "DAirDefense.h"
#include <IHardPoint.h>
#include "ILauncher.h"
#include "IWeapon.h"

#include <DWeapon.h>
#include "IFighter.h"

#include "SuperTrans.h"
#include "ObjList.h"
#include "Startup.h"
#include "Mission.h"
#include "MGlobals.h"
#include "ArchHolder.h"
#include "Camera.h"
#include "sfx.h"
#include "UserDefaults.h"
#include "TManager.h"
#include "EventScheduler.h"

#include "ObjMapIterator.h"

#include <Engine.h>
#include <TSmartPointer.h>
#include <IAnim.h>
#include <FileSys.h>
#include <TComponent.h>
#include <IConnection.h>

#include <stdlib.h>

struct AirDefenseArchetype
{
	PARCHETYPE pArchetype;
	PARCHETYPE pBoltType;
	const struct BT_PROJECTILE_DATA * pBoltData;
	BT_AIR_DEFENSE * pData;
	S32 realRefirePeriod;

	INSTANCE_INDEX	flashTextureID;

	void * operator new (size_t size)
	{
		return calloc(size,1);
	}
		
	void   operator delete (void *ptr)
	{
		::free(ptr);
	}

	~AirDefenseArchetype()
	{
		if (pBoltType)
			ARCHLIST->Release(pBoltType,OBJREFNAME);
		TMANAGER->ReleaseTextureRef(flashTextureID);
		flashTextureID=0;
	}
};

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
struct _NO_VTABLE AirDefense : IBaseObject, ILauncher, AIRDEFENSE_SAVELOAD
{
	BEGIN_MAP_INBOUND(AirDefense)
	_INTERFACE_ENTRY(IBaseObject)
	_INTERFACE_ENTRY(ILauncher)
	END_MAP()

	//----------------------------------
	// hardpoint data
	//----------------------------------
	HardpointInfo  hardpointinfo;
	INSTANCE_INDEX pointIndex;
	mutable TRANSFORM hardpointTransform;

	AirDefenseArchetype * archetype;

	SINGLE flashCounter;
	bool bHaveTarget:1;
	HSOUND hSound;
	//
	// target we are shooting at
	//
	OBJPTR<ILaunchOwner> owner;	// person who created us

	//----------------------------------
	//----------------------------------
	
	AirDefense (void);

	~AirDefense (void);	

	/* IBaseObject methods */

	virtual const TRANSFORM & GetTransform (void) const;

	virtual U32 GetSystemID (void) const;

	virtual BOOL32 Update (void);	// returning FALSE causes destruction

	virtual void PhysicalUpdate(SINGLE dt)
	{
		flashCounter += dt;
		if(flashCounter > 2*archetype->pData->flashFrequency)
			flashCounter -= 2*archetype->pData->flashFrequency;
	}

	virtual void Render (void);
	
	virtual S32 GetObjectIndex (void) const;

	virtual U32 GetPartID (void) const;

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

	virtual void HandlePreTakeover (U32 newMissionID, U32 troopID)
	{
	}

	virtual void TakeoverSwitchID (U32 newMissionID)
	{
	}

	virtual const bool TestFightersRetracted (void) const 
	{ 
		return true;
	}

	virtual void SetFighterStance(FighterStance stance)
	{
	}

	virtual void DoSpecialAbility (U32 specialID)
	{
	}

	virtual void DoSpecialAbility (IBaseObject *obj)
	{
	}

	virtual void SpecialAttackObject (IBaseObject * obj)
	{
	}

	virtual void DoCloak (void)
	{
	};

	virtual void  GetSpecialAbility (UNIT_SPECIAL_ABILITY & ability, bool & bEnabled)
	{
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
		return 0;
	}

	virtual U32 GetSyncData (void * buffer)
	{
		return 0;
	}

	virtual void PutSyncData (void * buffer, U32 bufferSize)
	{
	}

	virtual void OnAllianceChange (U32 allyMask)
	{
		// don't do anything
	}

	virtual IBaseObject * FindChildTarget(U32 childID){return NULL;};

	/* AirDefense methods */

//	static S32 findChild (const char * pathname, INSTANCE_INDEX parent);
//	static BOOL32 findHardpoint (const char * pathname, INSTANCE_INDEX & index, HardpointInfo & hardpointinfo, INSTANCE_INDEX parent);
	bool getSighting (TRANSFORM & result, IBaseObject * target=0, Vector *newPos=0, bool * pbAlwaysHit = 0) const;
	void init (AirDefenseArchetype * _archetype);
	void findTarget (OBJPTR<struct IWeaponTarget> & result);
};


//---------------------------------------------------------------------------
//
AirDefense::AirDefense (void) 
{
	pointIndex = -1;
}
//---------------------------------------------------------------------------
//
AirDefense::~AirDefense (void)
{
	pointIndex = -1;
	if(hSound)
		SFXMANAGER->CloseHandle(hSound);
}
//---------------------------------------------------------------------------
//
void AirDefense::init (AirDefenseArchetype * _archetype)
{
	CQASSERT(_archetype);
	CQASSERT(_archetype->pData->type == LC_AIR_DEFENSE);
	CQASSERT(_archetype->pData->objClass == OC_LAUNCHER);

	objClass = OC_LAUNCHER;
	archetype = _archetype;
}
//---------------------------------------------------------------------------
//
void AirDefense::InitLauncher (IBaseObject * _owner, S32 ownerIndex, S32 animArcheIndex, SINGLE _range)
{
	const BT_AIR_DEFENSE * data = (const BT_AIR_DEFENSE *) ARCHLIST->GetArchetypeData(pArchetype);

	CQASSERT(_owner);
	_owner->QueryInterface(ILaunchOwnerID, owner, LAUNCHVOLATILEPTR);
	CQASSERT(owner!=0);

	if (data->hardpoint[0])
		FindHardpoint(data->hardpoint, pointIndex, hardpointinfo, ownerIndex);
	CQASSERT(pointIndex != -1);
	flashCounter = 0;
	bHaveTarget = false;
}
//---------------------------------------------------------------------------
//
const TRANSFORM & AirDefense::GetTransform (void) const
{
	getSighting(hardpointTransform);
	
	return hardpointTransform;
}
//---------------------------------------------------------------------------
//
U32 AirDefense::GetSystemID (void) const
{
	return owner.Ptr()->GetSystemID();
}
//---------------------------------------------------------------------------
//
BOOL32 AirDefense::Update (void)
{
	if (--refireTime <= 0 && owner.Ptr()->effectFlags.canShoot())
	{
		OBJPTR<IWeaponTarget> target;

		refireTime = archetype->realRefirePeriod;
		findTarget(target);
		if (target)
		{
			Vector pos; 
			bool bAlwaysHit=false;

			if (getSighting(hardpointTransform, target.Ptr(), &pos, &bAlwaysHit))
			{
				if (owner->UseSupplies(archetype->pData->supplyCost) == 0)
					refireTime = archetype->realRefirePeriod;
					
				if (refireTime > 0)
				{
					//do the real damage
					U32 weaponVelocity = 10000;
					BASE_WEAPON_DATA * pBaseWeapon = (BASE_WEAPON_DATA *) ARCHLIST->GetArchetypeData(archetype->pBoltType);

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
						break;
					}
					SINGLE dist = (target.Ptr()->GetPosition(),-hardpointTransform.translation).fast_magnitude();

					SINGLE time = dist/weaponVelocity;
					SCHEDULER->QueueDamageEvent(__min(1,time),owner.Ptr()->GetPartID(), target.Ptr()->GetPartID(), 1);
					//do the visual
					if(bVisible || target.Ptr()->bVisible)
					{
						if(OBJLIST->CreateProjectile())
						{
							IBaseObject * obj = ARCHLIST->CreateInstance(archetype->pBoltType);
							if (obj)
							{
								OBJPTR<IWeapon> bolt;

								OBJLIST->AddObject(obj);
								if (obj->QueryInterface(IWeaponID,bolt) != 0)
								{
									bolt->InitWeapon(owner.Ptr(), hardpointTransform, target.Ptr(), (bAlwaysHit)?IWF_ALWAYS_HIT:IWF_ALWAYS_MISS);
								}
							}
							else
							{
								OBJLIST->ReleaseProjectile();
							}
						}
					}
					// updates the sound's position
					if (hSound==0)
						hSound = SFXMANAGER->Open(archetype->pData->soundFx);
					SFXMANAGER->Play(hSound,GetSystemID(),&hardpointTransform.translation,SFXPLAYF_LOOPING|SFXPLAYF_NORESTART);
					bHaveTarget = true;
				}
				else
				{
					refireTime = archetype->realRefirePeriod;
					if(bHaveTarget)
					{
						SFXMANAGER->Stop(hSound);
						bHaveTarget = false;
					}
				}
			}
			else
			if(bHaveTarget)
			{
				SFXMANAGER->Stop(hSound);
				bHaveTarget = false;
			}
		}
		else
		if(bHaveTarget)
		{
			SFXMANAGER->Stop(hSound);
			bHaveTarget = false;
		}
		else
		{
			refireTime *= 10;		// extra long time when nothing is happening
		}
	}

	return 1;
}
//---------------------------------------------------------------------------
//
void AirDefense::Render (void)
{
	if(owner.Ptr()->bVisible && bHaveTarget)
	{
		if(archetype->flashTextureID)
		{
			if(flashCounter < archetype->pData->flashFrequency)
			{
				Vector epos = ENGINE->get_transform(pointIndex).rotate_translate(hardpointinfo.point);
				
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
				SINGLE size = archetype->pData->flashWidth;
				v[0] = epos - i*size - j*size;
				v[1] = epos + i*size - j*size;
				v[2] = epos + i*size + j*size;
				v[3] = epos - i*size + j*size;

				SetupDiffuseBlend(archetype->flashTextureID,TRUE);
				BATCH->set_render_state(D3DRS_ZENABLE,TRUE);
				BATCH->set_render_state(D3DRS_ZWRITEENABLE,TRUE);
				BATCH->set_render_state(D3DRS_ALPHABLENDENABLE,TRUE);
				BATCH->set_render_state(D3DRS_SRCBLEND,D3DBLEND_SRCALPHA);
				BATCH->set_render_state(D3DRS_DESTBLEND,D3DBLEND_INVSRCALPHA);
				
				PB.Begin(PB_QUADS);

				PB.Color4ub(archetype->pData->flashColor.red,archetype->pData->flashColor.green,
					archetype->pData->flashColor.blue,archetype->pData->flashColor.alpha);
				PB.TexCoord2f(0,0);   PB.Vertex3f(v[0].x,v[0].y,v[0].z);
				PB.TexCoord2f(1,0);   PB.Vertex3f(v[1].x,v[1].y,v[1].z);
				PB.TexCoord2f(1,1);   PB.Vertex3f(v[2].x,v[2].y,v[2].z);
				PB.TexCoord2f(0,1);   PB.Vertex3f(v[3].x,v[3].y,v[3].z);
				
				PB.End();

			}
		}
	}
}
//---------------------------------------------------------------------------
//
void AirDefense::AttackPosition (const struct GRIDVECTOR * pos, bool bSpecial)
{
}
//---------------------------------------------------------------------------
//
void AirDefense::AttackObject (IBaseObject * obj)
{
}
//---------------------------------------------------------------------------
//
S32 AirDefense::GetObjectIndex (void) const
{
	return pointIndex;
}
//---------------------------------------------------------------------------
//
U32 AirDefense::GetPartID (void) const
{
	return owner.Ptr()->GetPartID();
}
//---------------------------------------------------------------------------
// get the sighting vector of the barrel
//
bool AirDefense::getSighting (TRANSFORM & result, IBaseObject * target, Vector *newPos, bool * pbAlwaysHit) const
{
	/*
	const Matrix matrix = ENGINE->get_orientation(index);

	pos = matrix * pos;
	pos += ENGINE->get_position(index);

	orientation = matrix * orientation;
	*/

	Vector pos = ENGINE->get_transform(pointIndex).rotate_translate(hardpointinfo.point);
	bool inRange = false;

	result.set_identity();
	result.rotate_about_i(90 * MUL_DEG_TO_RAD);

	if (target)
	{
		Vector relVec = target->GetTransform().translation - pos;
		SINGLE distance = relVec.magnitude();

		if (distance < owner->GetWeaponRange())
		{
			//
			// calculate lead
			//
			SINGLE time = distance / archetype->pBoltData->maxVelocity;
			Vector diff = (target->GetVelocity() * time);
			// testing!!!
			diff.z = 0;
			relVec += diff;
			SINGLE yaw = get_angle(relVec.x, relVec.y);

			int num = rand() & 255;

			if (num > int(archetype->pData->baseAccuracy * 256))	// failed our competency check
			{
				if (num & 1)
					yaw += 10 * MUL_DEG_TO_RAD;
				else
					yaw -= 10 * MUL_DEG_TO_RAD;
			}
			else
			if (pbAlwaysHit)
				*pbAlwaysHit = true;
			
			if (newPos)
				*newPos = target->GetTransform().translation + diff;

			if (relVec.magnitude() < owner->GetWeaponRange())	// still within range?
			{
				SINGLE x = sqrt(relVec.x * relVec.x  + relVec.y * relVec.y);
				SINGLE pitch;
			
				pitch = get_angle(relVec.z, x);

				result.rotate_about_j(yaw);
				result.rotate_about_i(pitch);
				inRange = true;
			}
		}
	}
	
	result.set_position(pos);
	return inRange;
}
//---------------------------------------------------------------------------
//
/*S32 AirDefense::findChild (const char * pathname, INSTANCE_INDEX parent)
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
BOOL32 AirDefense::findHardpoint (const char * pathname, INSTANCE_INDEX & index, HardpointInfo & hardpointinfo, INSTANCE_INDEX parent)
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
		HARCH hArch = index;

		if ((result = HARDPOINT->retrieve_hardpoint_info(hArch, ptr, hardpointinfo)) == false)
			index = -1;		// invalidate result
	}

	return result;
}*/
//---------------------------------------------------------------------------
//
void AirDefense::findTarget (OBJPTR<IWeaponTarget> & target)
{
	VOLPTR(IFighter) fighter;
	SINGLE minDistance = owner->GetWeaponRange();
	minDistance *= minDistance;
	IBaseObject *bestTarget = 0;
	U32 systemID = GetSystemID();
	CQASSERT(systemID);
	const GRIDVECTOR pos = owner.Ptr()->GetGridPosition();
	const U32 allyMask = MGlobals::GetAllyMask(owner.Ptr()->GetPlayerID());

	ObjMapIterator iter(systemID,owner.Ptr()->GetPosition(),owner->GetWeaponRange());
	while (iter)
	{
		if (iter->flags & OM_AIR)
		{
			if ((allyMask & (1 << ((iter->dwMissionID&PLAYERID_MASK)-1))) == 0)		// if not allies
			{
				fighter = iter->obj;
				if (fighter == 0 || fighter->IsLeader())
				{
					SINGLE mag = iter->obj->GetGridPosition() - pos;
		
					if (mag < minDistance)
					{
						bestTarget = iter->obj;
						minDistance = mag;
					}
				}
			}
		}
		++iter;
	}

	if (bestTarget)
		bestTarget->QueryInterface(IWeaponTargetID, target);
	else
		target = 0;
}

//---------------------------------------------------------------------------
//
inline IBaseObject * createAirDefense (AirDefenseArchetype * archetype)
{
	AirDefense * air = new ObjectImpl<AirDefense>;

	air->init(archetype);

	return air;
}
//------------------------------------------------------------------------------------------
//---------------------------AirDefense Factory----------------------------------------------
//------------------------------------------------------------------------------------------
//
struct DACOM_NO_VTABLE AirDefenseFactory : public IObjectFactory
{
	//------------------------------------------------------------------------------------------
	//
	U32 factoryHandle;		// handles to callback

	//
	// Interface mapping
	//

	BEGIN_DACOM_MAP_INBOUND(AirDefenseFactory)
	DACOM_INTERFACE_ENTRY(IObjectFactory)
	END_DACOM_MAP()

	AirDefenseFactory (void) { }

	~AirDefenseFactory (void);

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

	/* AirDefenseFactory methods */

	IDAComponent * getBase (void)
	{
		return static_cast<IObjectFactory *> (this);
	}
};
//--------------------------------------------------------------------------//
//
AirDefenseFactory::~AirDefenseFactory (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (OBJLIST && OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
		connection->Unadvise(factoryHandle);
}
//--------------------------------------------------------------------------//
//
void AirDefenseFactory::init (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
		connection->Advise(getBase(), &factoryHandle);
}
//-----------------------------------------------------------------------------
//
HANDLE AirDefenseFactory::CreateArchetype (const char *szArchname, OBJCLASS objClass, void *_data)
{
	AirDefenseArchetype * result = 0;

	if (objClass == OC_LAUNCHER)
	{
		BT_AIR_DEFENSE * data = (BT_AIR_DEFENSE *) _data;

		if (data->type == LC_AIR_DEFENSE)	   
		{
			result = new AirDefenseArchetype;
			result->pArchetype = ARCHLIST->GetArchetype(szArchname);
 
			result->pBoltType = ARCHLIST->LoadArchetype(data->weaponType);
			CQASSERT(result->pBoltType);
			ARCHLIST->AddRef(result->pBoltType, OBJREFNAME);

			result->pBoltData =	(BT_PROJECTILE_DATA *) ARCHLIST->GetArchetypeData(result->pBoltType);
			result->pData = data;
			result->realRefirePeriod = F2LONG(data->refirePeriod*REALTIME_FRAMERATE);
			if (data->flashTextureName[0])
			{
				result->flashTextureID = TMANAGER->CreateTextureFromFile(data->flashTextureName, TEXTURESDIR, DA::TGA,PF_4CC_DAA4);
			}

			SFXMANAGER->Preload(data->soundFx);
		}
	}

	return (HANDLE) result;
}
//-------------------------------------------------------------------
//
BOOL32 AirDefenseFactory::DestroyArchetype (HANDLE hArchetype)
{
	AirDefenseArchetype * objtype = (AirDefenseArchetype *)hArchetype;

	delete objtype;

	return 1;
}
//-------------------------------------------------------------------
//
IBaseObject * AirDefenseFactory::CreateInstance (HANDLE hArchetype)
{
	AirDefenseArchetype * objtype = (AirDefenseArchetype *)hArchetype;
	return createAirDefense(objtype);
}
//-------------------------------------------------------------------
//
void AirDefenseFactory::EditorCreateInstance (HANDLE hArchetype, const MISSION_INFO & info)
{
	// not supported
}
//---------------------------------------------------------------------------
//-------------------------GLOBAL STARTUP------------------------------------
//---------------------------------------------------------------------------
//
struct _airdefense : GlobalComponent
{
	AirDefenseFactory * sfactory;

	virtual void Startup (void)
	{
		sfactory = new DAComponent<AirDefenseFactory>;
		AddToGlobalCleanupList((IDAComponent **) &sfactory);
	}

	virtual void Initialize (void)
	{
		sfactory->init();
	}
};

static _airdefense __ship;

//---------------------------------------------------------------------------------------------
//-------------------------------End AirDefense.cpp------------------------------------------------
//---------------------------------------------------------------------------------------------
