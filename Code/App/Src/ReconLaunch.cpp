//--------------------------------------------------------------------------//
//                                                                          //
//                                ReconLaunch.cpp                                //
//                                                                          //
//                  COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/ReconLaunch.cpp 24    10/09/00 1:44p Tmauer $
*/			    
//------------------------------- #INCLUDES --------------------------------//
//--------------------------------------------------------------------------//

#include "pch.h"
#include <globals.h>

#include "ReconLaunch.h"
#include <DReconLaunch.h>
#include "IRecon.h"

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
#include "OpAgent.h"
#include "GridVector.h"

#include <Engine.h>
#include <TSmartPointer.h>
#include <IAnim.h>
#include <FileSys.h>
#include <TComponent.h>
#include <IConnection.h>

#include <stdlib.h>

//---------------------------------------------------------------------------
//
ReconLaunch::ReconLaunch (void) 
{
	fancyLaunchIndex = animIndex = barrelIndex = -1;
}
//---------------------------------------------------------------------------
//
ReconLaunch::~ReconLaunch (void)
{
	ANIM->release_script_inst(animIndex);
	animIndex = barrelIndex  = -1;
	if(probe)
	{
//		if(probe->IsActive())
//			probe->ExplodeProbe();
		probe->DeleteProbe();
//		OBJLIST->DeferredDestruction(probe.ptr->GetPartID());
		probe = NULL;
	}
}
//---------------------------------------------------------------------------
//
void ReconLaunch::init (PARCHETYPE _pArchetype, PARCHETYPE _pBoltType)
{
	const BT_RECON_LAUNCH * data = (const BT_RECON_LAUNCH *) ARCHLIST->GetArchetypeData(_pArchetype);

	CQASSERT(data);
	CQASSERT(data->type == LC_RECON_LAUNCH);
	CQASSERT(data->objClass == OC_LAUNCHER);

	pArchetype = _pArchetype;
	objClass = OC_LAUNCHER;

	pBoltType = _pBoltType;

	REFIRE_PERIOD = data->refirePeriod;
	CQASSERT(REFIRE_PERIOD!=0);
	supplyCost = data->supplyCost;
	animTime = data->animTime;
	effectDuration = data->effectDuration;
	refireDelay = (float(rand()) / RAND_MAX) * REFIRE_PERIOD + animTime;

	if (CQFLAGS.bLoadingObjlist==0)	// if not loading from disk, create the probes part ID now
	{
		probeID = MGlobals::CreateSubordinatePartID();
	}
	specialAbility = data->specialAbility;
	bWormWeapon = data->bWormWeapon;
}
//---------------------------------------------------------------------------
//
void ReconLaunch::InitLauncher (IBaseObject * _owner, S32 ownerIndex, S32 animArcheIndex, SINGLE range)
{
	const BT_RECON_LAUNCH * data = (const BT_RECON_LAUNCH *) ARCHLIST->GetArchetypeData(pArchetype);

	CQASSERT(_owner);
	_owner->QueryInterface(ILaunchOwnerID, owner, LAUNCHVOLATILEPTR);
	CQASSERT(owner!=0);

	if (data->hardpoint[0])
	{
		FindHardpoint(data->hardpoint, barrelIndex, hardpointinfo, ownerIndex);
	}
	else
	{
		hardpointinfo.orientation.set_identity();
		barrelIndex = ownerIndex;
	}

	CQASSERT(barrelIndex != -1);

	Vector pos = _owner->GetPosition();
	HSOUND hSound = SFXMANAGER->Open(data->warmupSound);
	SFXMANAGER->Play(hSound,_owner->GetSystemID(),&pos);

	if (data->animation[0])
		animIndex = ANIM->create_script_inst(animArcheIndex, ownerIndex, data->animation);

	bKillProbe = false;

	if(!probe)
	{
		IBaseObject * obj = OBJLIST->FindObject(probeID);
		if(obj)
		{
			obj->QueryInterface(IReconProbeID,probe,NONSYSVOLATILEPTR);
			probe->ResolveRecon(this);
		}
		else if(!CQFLAGS.bLoadingObjlist)
		{
			obj = ARCHLIST->CreateInstance(pBoltType);
			obj->QueryInterface(IReconProbeID,probe,NONSYSVOLATILEPTR);
			probe->InitRecon(this,probeID);
		}
	}
}
//---------------------------------------------------------------------------
//
const TRANSFORM & ReconLaunch::GetTransform (void) const
{
	getSighting(hardpointTransform);
	
	return hardpointTransform;
}
//---------------------------------------------------------------------------
//
void ReconLaunch::KillProbe (U32 dwMissionID)
{
	CQASSERT(THEMATRIX->IsMaster());
	CQASSERT(probe.Ptr()->GetPartID() == dwMissionID);
	bKillProbe = true;
}
//---------------------------------------------------------------------------
//
U32 ReconLaunch::hostShootTarget (U8 * packet)
{
	CQASSERT(refireDelay <= 0);
	U32 result = 0;
	
	getSighting(hardpointTransform);

	SINGLE RANGE = owner->GetWeaponRange();
	bool targetGood = false;
	if(attacking && ((RANGE <0) || ((owner.Ptr()->GetPosition() - targetPos).magnitude_squared() < RANGE*RANGE)))
		targetGood = true;

	if(targetGood)
	{
		if (owner->UseSupplies(supplyCost,true))
		{
			if(probe->IsActive())
			{
				probe->ExplodeProbe();
			}
			probe->LaunchProbe(owner.Ptr(), hardpointTransform, &targetPos, targetSystemID,target);
			packet[0] = 1;
			result = 1;// need to force a sync packet to be sent 
		}
		attacking = 4;
		refireDelay += REFIRE_PERIOD;
		refireDelay += animTime+effectDuration;
		effectTimer = effectDuration;
	}
	
	return result;
}
//---------------------------------------------------------------------------
//
void ReconLaunch::clientShootTarget ()
{
	getSighting(hardpointTransform);
	owner->UseSupplies(supplyCost);

	probe->LaunchProbe(owner.Ptr(), hardpointTransform, &targetPos, targetSystemID,target);

	attacking = 0;

	refireDelay += REFIRE_PERIOD;
	refireDelay += animTime+effectDuration;
	effectTimer = effectDuration;
}
//---------------------------------------------------------------------------
//
BOOL32 ReconLaunch::Update (void)
{
	if(attacking == 4)
	{
		if(owner)
			owner->LauncherCancelAttack();
		attacking = 0;
	}

	MPartNC part(owner.Ptr());
	if(part.isValid())
	{
		if((!(part->caps.probeOk)) || (!(part->caps.specialEOAOk)) || (!(part->caps.specialAttackWormOk)) || (!(part->caps.specialAbilityOk)))
		{
			BT_RECON_LAUNCH * data = (BT_RECON_LAUNCH *)(ARCHLIST->GetArchetypeData(pArchetype));
			if(data->neededTech.raceID)
			{
				if(MGlobals::GetCurrentTechLevel(owner.Ptr()->GetPlayerID()).HasTech(data->neededTech))
				{
/*					if (specialAbility == USA_PROBE)//  data->probe)
						part->caps.probeOk = true;
					else*/ if(data->selfTarget)
						part->caps.specialAbilityOk = true;
					else
						part->caps.specialEOAOk = true;

					if(bWormWeapon)
						part->caps.specialAttackWormOk = true;
				}
			}
			else
			{
/*				if (specialAbility == USA_PROBE)//  data->probe)
					part->caps.probeOk = true;
				else*/ if(data->selfTarget)
					part->caps.specialAbilityOk = true;
				else
					part->caps.specialEOAOk = true;
				if(bWormWeapon)
					part->caps.specialAttackWormOk = true;
			}
		}
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

	return 1;
}
//---------------------------------------------------------------------------
//
void ReconLaunch::AttackPosition (const struct GRIDVECTOR * pos, bool bSpecial)
{
	if (pos == 0)
		attacking = 0;
	else
	{
		targetPos = *pos;
		attacking = 1;
		targetID = 0;
		target = NULL;
	}
}
//---------------------------------------------------------------------------
//
void ReconLaunch::AttackMultiSystem (struct GRIDVECTOR * position, U32 targSystemID)
{
	if (position == 0)
		attacking = 0;
	else
	{
		targetSystemID = targSystemID;
		targetPos = *position;
		attacking = 1;
		targetID = 0;
		target = NULL;
	}
}
//---------------------------------------------------------------------------
//
void ReconLaunch::WormAttack (IBaseObject * obj)
{
	obj->QueryInterface(IBaseObjectID,target,NONSYSVOLATILEPTR);
	targetID = obj->GetPartID();
	targetPos = obj->GetGridPosition();
	attacking = 1;
}
//---------------------------------------------------------------------------
//
void ReconLaunch::AttackObject (IBaseObject * _target)
{
}
//---------------------------------------------------------------------------
//
void ReconLaunch::SpecialAttackObject (IBaseObject * obj)
{
}
//---------------------------------------------------------------------------
//
void ReconLaunch::DoSpecialAbility (U32 specialID)
{
	targetPos = owner.Ptr()->GetPosition();
	attacking = 1;
	targetID = 0;
	target = NULL;
}
//---------------------------------------------------------------------------
//
S32 ReconLaunch::GetObjectIndex (void) const
{
	return barrelIndex;
}
//---------------------------------------------------------------------------
//
U32 ReconLaunch::GetPartID (void) const
{
	return owner.Ptr()->GetPartID();
}
//---------------------------------------------------------------------------
//
BOOL32 ReconLaunch::Save (struct IFileSystem * inFile)
{
	DAFILEDESC fdesc = "RECON_LAUNCH_SAVELOAD";
	COMPTR<IFileSystem> file;
	BOOL32 result = 0;
	DWORD dwWritten;
	RECON_LAUNCH_SAVELOAD save;

	fdesc.lpImplementation = "DOS";
	fdesc.dwDesiredAccess = GENERIC_READ|GENERIC_WRITE;
	fdesc.dwShareMode = 0;  // no sharing
	fdesc.dwCreationDistribution = CREATE_ALWAYS;

	if (inFile->CreateInstance(&fdesc, file) != GR_OK)
		goto Done;

	save = *static_cast<RECON_LAUNCH_SAVELOAD *>(this);

	file->WriteFile(0, &save, sizeof(save), &dwWritten, 0);
	
	result = 1;

Done:	
	return result;
}
//---------------------------------------------------------------------------
//
BOOL32 ReconLaunch::Load (struct IFileSystem * inFile)
{
	DAFILEDESC fdesc = "RECON_LAUNCH_SAVELOAD";
	COMPTR<IFileSystem> file;
	BOOL32 result = 0;
	DWORD dwRead;
	RECON_LAUNCH_SAVELOAD load;
	U8 buffer[1024];

	fdesc.lpImplementation = "DOS";
	if (inFile->CreateInstance(&fdesc, file) != GR_OK)
		goto Done;

	file->ReadFile(0, buffer, sizeof(buffer), &dwRead, 0);
	MISSION->CorrelateSymbol("RECON_LAUNCH_SAVELOAD", buffer, &load);

	*static_cast<RECON_LAUNCH_SAVELOAD *>(this) = load;
		
	result = 1;

Done:	
	return result;
}

//---------------------------------------------------------------------------
//
void ReconLaunch::ResolveAssociations()
{
/*	owner.ptr = OBJLIST->FindObject(dwOwnerID);*/
	CQASSERT(owner.Ptr());
	/*owner.ptr->QueryInterface(ILaunchOwnerID, owner);
	CQASSERT(owner!=0);*/

	if (OBJLIST->FindObject(probeID, NONSYSVOLATILEPTR, probe, IReconProbeID))
	{
		probe->ResolveRecon(this);
	}

	if(targetID)
	{
		OBJLIST->FindObject(targetID, NONSYSVOLATILEPTR, target, IBaseObjectID);
	}
}
//---------------------------------------------------------------------------
// get the sighting vector of the barrel
//
void ReconLaunch::getSighting (TRANSFORM & result) const
{
	result.TRANSFORM::TRANSFORM(hardpointinfo.orientation, hardpointinfo.point);
	result = ENGINE->get_transform(barrelIndex).multiply(result);
}
//---------------------------------------------------------------------------
//
inline IBaseObject * createReconLaunch (PARCHETYPE pArchetype, PARCHETYPE pBoltType)
{
	ReconLaunch * reconLaunch = new ObjectImpl<ReconLaunch>;

	reconLaunch->init(pArchetype, pBoltType);

	return reconLaunch;
}
//------------------------------------------------------------------------------------------
//---------------------------ReconLaunch Factory----------------------------------------------
//------------------------------------------------------------------------------------------
//
struct DACOM_NO_VTABLE ReconLaunchFactory : public IObjectFactory
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

	BEGIN_DACOM_MAP_INBOUND(ReconLaunchFactory)
	DACOM_INTERFACE_ENTRY(IObjectFactory)
	END_DACOM_MAP()

	ReconLaunchFactory (void) { }

	~ReconLaunchFactory (void);

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

	/* ReconLaunchFactory methods */

	IDAComponent * getBase (void)
	{
		return static_cast<IObjectFactory *> (this);
	}
};
//--------------------------------------------------------------------------//
//
ReconLaunchFactory::~ReconLaunchFactory (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (OBJLIST && OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
		connection->Unadvise(factoryHandle);
}
//--------------------------------------------------------------------------//
//
void ReconLaunchFactory::init (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
		connection->Advise(getBase(), &factoryHandle);
}
//-----------------------------------------------------------------------------
//
HANDLE ReconLaunchFactory::CreateArchetype (const char *szArchname, OBJCLASS objClass, void *_data)
{
	OBJTYPE * result = 0;

	if (objClass == OC_LAUNCHER)
	{
		BT_RECON_LAUNCH * data = (BT_RECON_LAUNCH *) _data;

		if (data->type == LC_RECON_LAUNCH)	   
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
BOOL32 ReconLaunchFactory::DestroyArchetype (HANDLE hArchetype)
{
	OBJTYPE * objtype = (OBJTYPE *)hArchetype;

	delete objtype;

	return 1;
}
//-------------------------------------------------------------------
//
IBaseObject * ReconLaunchFactory::CreateInstance (HANDLE hArchetype)
{
	OBJTYPE * objtype = (OBJTYPE *)hArchetype;
	return createReconLaunch(objtype->pArchetype, objtype->pBoltType);
}
//-------------------------------------------------------------------
//
void ReconLaunchFactory::EditorCreateInstance (HANDLE hArchetype, const MISSION_INFO & info)
{
	// not supported
}
//---------------------------------------------------------------------------
//-------------------------GLOBAL STARTUP------------------------------------
//---------------------------------------------------------------------------
//
struct _reconLaunch : GlobalComponent
{
	ReconLaunchFactory * sfactory;

	virtual void Startup (void)
	{
		sfactory = new DAComponent<ReconLaunchFactory>;
		AddToGlobalCleanupList((IDAComponent **) &sfactory);
	}

	virtual void Initialize (void)
	{
		sfactory->init();
	}
};

static _reconLaunch __recLaunch;

//---------------------------------------------------------------------------------------------
//-------------------------------End ReconLaunch.cpp------------------------------------------------
//---------------------------------------------------------------------------------------------
