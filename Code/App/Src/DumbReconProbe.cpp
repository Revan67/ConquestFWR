//--------------------------------------------------------------------------//
//                                                                          //
//                             DumbReconProbe.cpp                          //
//                                                                          //
//                  COPYRIGHT (C) 1997 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*

    $Author: Jasony $

    $Header: /Conquest/App/Src/DumbReconProbe.cpp 6     10/13/00 12:05p Jasony $
*/			    
//--------------------------------------------------------------------------//

#include "pch.h"
#include <globals.h>

#include "TObject.h"
#include "Camera.h"
#include "Sector.h"
#include "Objlist.h"
#include "sfx.h"
#include "IConnection.h"
#include "Startup.h"
#include "SuperTrans.h"
#include "DSpecial.h"
#include "IWeapon.h"
#include "TObjTrans.h"
#include "TObjFrame.h"
#include "Mesh.h"
#include "ArchHolder.h"
#include "Anim2d.h"
#include "FogOfWar.h"
#include "UserDefaults.h"
#include "Mission.h"
#include "TerrainMap.h"
#include "CQLight.h"
#include "GridVector.h"
#include "TObjMission.h"
#include "OpAgent.h"
#include <MGlobals.h>
#include "TManager.h"
#include "IExplosion.h"
#include "IRecon.h"
#include "ObjMapIterator.h"
#include "stdio.h"
#include "IShipMove.h"
#include "CommPacket.h"
#include "IVertexBuffer.h"
#include "TObjRender.h"

#include <Renderer.h>
#include <TComponent.h>
#include <Engine.h>
#include <Vector.h>
#include <Matrix.h>
#include <IHardpoint.h>
#include <stdlib.h>
#include <FileSys.h>
#include <ICamera.h>
#include <Pixel.h>
//#include <RPUL\PrimitiveBuilder.h>
#include <IRenderPrimitive.h>

struct DUMBRECONPROBE_INIT : RenderArch
{
	BT_DUMBRECONPROBE_DATA * pData;
	S32 archIndex;
	IMeshArchetype * meshArch;
	PARCHETYPE pArchetype;
};

struct _NO_VTABLE DumbReconProbe : public ObjectRender<ObjectMission<ObjectTransform<ObjectFrame<IBaseObject,struct DUMBRECONPROBE_SAVELOAD,struct DUMBRECONPROBE_INIT> > > > , ISaveLoad,
										BASE_DUMBRECONPROBE_SAVELOAD,IReconProbe
{
	BEGIN_MAP_INBOUND(DumbReconProbe)
	_INTERFACE_ENTRY(IBaseObject)
	_INTERFACE_ENTRY(ISaveLoad)
	_INTERFACE_ENTRY(IMissionActor)
	_INTERFACE_ENTRY(IReconProbe)
	END_MAP()

	//------------------------------------------
	OBJPTR<IBaseObject> owner;
	
	const BT_DUMBRECONPROBE_DATA * data;
	DUMBRECONPROBE_INIT *arch;
	HSOUND hSound;

	GeneralSyncNode  genSyncNode;

	OBJPTR<IReconLauncher> ownerLauncher;
	OBJPTR<IBaseObject> target;

	U32 objMapSystemID;
	U32 objMapSquare;

	//------------------------------------------

	DumbReconProbe (void) :
		genSyncNode(this, SyncGetProc(&DumbReconProbe::getSyncData), SyncPutProc(&DumbReconProbe::putSyncData))
	{
		bDeleteRequested = true;
		bGone = true;
	}

    void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}

	virtual ~DumbReconProbe (void);

	virtual BOOL32 Update (void);

	virtual void PhysicalUpdate (SINGLE dt);

	virtual void Render (void);

	virtual U32 GetSystemID (void) const;

	virtual U32 GetPlayerID (void) const
	{
		return MGlobals::GetPlayerFromPartID(ownerID);
	}

	virtual void CastVisibleArea();

	virtual void RevealFog (const U32 currentSystem);


	/* IReconProbe */

	virtual void InitRecon(IReconLauncher * _ownerLauncher, U32 _dwMissionID);

	virtual void ResolveRecon(IBaseObject * _ownerLauncher);

	virtual void LaunchProbe (IBaseObject * owner, const class TRANSFORM & orientation, const class Vector * pos,
		U32 targetSystemID, IBaseObject * jumpTarget);

	virtual void ExplodeProbe();

	virtual void DeleteProbe();

	virtual bool IsActive();

	virtual void ReconSwitchID(U32 newOwnerID)
	{
	}

	/* ISaveLoad methods */

	virtual BOOL32 Save (struct IFileSystem * file);

	virtual BOOL32 Load (struct IFileSystem * file);

	virtual void ResolveAssociations();

	/* IMissionActor */

	///////////////////

	void init (DUMBRECONPROBE_INIT *initData);

	void softwareRenderRing ();

	void renderRing ();

	void renderSpark ();

//	void renderAnm();
	
	U32 getSyncData (void * buffer);
	void putSyncData (void * buffer, U32 bufferSize, bool bLateDelivery);

	IBaseObject * getBase (void)
	{
		return this;
	}
};

//----------------------------------------------------------------------------------
//
DumbReconProbe::~DumbReconProbe (void)
{
	if (hSound)
		SFXMANAGER->CloseHandle(hSound);
	if(objMapSystemID)
	{
		OBJMAP->RemoveObjectFromMap(this,objMapSystemID,objMapSquare);
		objMapSystemID = 0;
	}
}
//----------------------------------------------------------------------------------
//
U32 DumbReconProbe::GetSystemID (void) const
{
	return systemID;
}
//---------------------------------------------------------------------------
//
void DumbReconProbe::RevealFog (const U32 currentSystem)
{
	if (time > 0 && currentSystem == systemID)
	{
		if (MGlobals::AreAllies(GetPlayerID(), MGlobals::GetThisPlayer()))
			FOGOFWAR->RevealZone(this, sensorRadius, cloakedSensorRadius);
	}
}
//----------------------------------------------------------------------------------
//
void DumbReconProbe::CastVisibleArea()
{
	if(time >0 && systemID)
	{
		U32 tID = GetPlayerID();
		if(!tID)
			tID = playerID;
		SetVisibleToPlayer(tID);

		OBJLIST->CastVisibleArea(tID,systemID,GetPosition(),fieldFlags,sensorRadius,cloakedSensorRadius);
	}
}
//----------------------------------------------------------------------------------
//
void DumbReconProbe::PhysicalUpdate (SINGLE dt)
{
	if(time > 0)
	{
		time -= dt;
		SINGLE distSq = (targetPos-transform.translation).magnitude_squared();
		SINGLE moveDist = data->maxVelocity*dt;
		if(distSq < moveDist*moveDist)
		{
			if(target)
			{
				if(objMapSystemID)
				{
					OBJMAP->RemoveObjectFromMap(this,objMapSystemID,objMapSquare);
					objMapSystemID = 0;
				}
				IBaseObject * otherTarg = SECTOR->GetJumpgateDestination(target);
				systemID = otherTarg->GetSystemID();
				transform = otherTarg->GetTransform();
				targetPos = otherTarg->GetGridPosition();

				SECTOR->RevealSystem(systemID,GetPlayerID());
				objMapSystemID = systemID;
				objMapSquare = OBJMAP->GetMapSquare(systemID,transform.translation);
				OBJMAP->AddObjectToMap(this,objMapSystemID,objMapSquare);
			}
		}
		else if(systemID)
		{
			if(objMapSystemID)
			{
				OBJMAP->RemoveObjectFromMap(this,objMapSystemID,objMapSquare);
				objMapSystemID = 0;
			}
			transform.translation = transform.translation + ((targetPos-transform.translation).normalize())*moveDist;
			objMapSystemID = systemID;
			objMapSquare = OBJMAP->GetMapSquare(systemID,transform.translation);
			OBJMAP->AddObjectToMap(this,objMapSystemID,objMapSquare);
		}	
	}
}
//----------------------------------------------------------------------------------
//
BOOL32 DumbReconProbe::Update (void)
{
	if(bDeleteRequested && THEMATRIX->IsMaster())
	{
		if(!bGone)
		{
			if(ownerLauncher)
			{
				ownerLauncher->KillProbe(dwMissionID);
			}
		}
		return 1;
	}
	BOOL32 result = 1;
	
	return result;
}
//----------------------------------------------------------------------------------
//
void DumbReconProbe::Render (void)
{
	if (bVisible && (time > 0))
	{
		LIGHT->deactivate_all_lights();
		LIGHTS->ActivateBestLights(transform.translation,8,4000);
		TreeRender(mc);
	}
}
//----------------------------------------------------------------------------------
//
void DumbReconProbe::InitRecon(IReconLauncher * _ownerLauncher, U32 _dwMissionID)
{
	_ownerLauncher->QueryInterface(IReconLauncherID,ownerLauncher,NONSYSVOLATILEPTR);
	dwMissionID = _dwMissionID;
	
	playerID = MGlobals::GetPlayerFromPartID(dwMissionID);
	sprintf(partName,"DumbReconProbe 0x%x",dwMissionID);

	OBJLIST->AddPartID(this, dwMissionID);
	OBJLIST->AddObject(this);

	time = 0;
	bGone = true;
	SetReady(false);
	sensorRadius = data->missionData.sensorRadius;
	cloakedSensorRadius = data->missionData.cloakedSensorRadius;
}
//----------------------------------------------------------------------------------
//
void DumbReconProbe::ResolveRecon(IBaseObject * _ownerLauncher)
{
	_ownerLauncher->QueryInterface(IReconLauncherID,ownerLauncher,NONSYSVOLATILEPTR);
}
//----------------------------------------------------------------------------------
//
void DumbReconProbe::LaunchProbe (IBaseObject * _owner, const class TRANSFORM & orientation, const class Vector * pos,
		U32 targetSystemID, IBaseObject * jumpTarget)
{
	if(objMapSystemID)
	{
		OBJMAP->RemoveObjectFromMap(this,objMapSystemID,objMapSquare);
		objMapSystemID = 0;
	}
	
	CQASSERT(pos);
	targetPos = *pos;

	if(jumpTarget)
	{
		jumpTarget->QueryInterface(IBaseObjectID,target,NONSYSVOLATILEPTR);
		targetID = target->GetPartID();
	}
	else
	{
		target = NULL;
		targetID = NULL;
	}

	_owner->QueryInterface(IBaseObjectID, owner, NONSYSVOLATILEPTR);

	systemID = owner->GetSystemID();
	ownerID = owner->GetPartID();
	U32 visFlags = owner->GetVisibilityFlags();

	SetVisibilityFlags(visFlags);

	TRANSFORM orient = orientation;
	Vector start = orientation.get_position();

	//Correct bolt to fire at target regardless of gun barrel
	SINGLE curPitch, desiredPitch, relPitch,dist,curYaw,relYaw,desYaw;
	Vector goal = *pos; 

	curPitch = orient.get_pitch();
	curYaw = orient.get_yaw();
	//goal -= ENGINE->get_position(barrelIndex);
	goal -= orient.get_position();
	
	dist = sqrt(goal.x * goal.x  + goal.y * goal.y);
	desiredPitch = get_angle(goal.z, dist);
	desYaw = get_angle(goal.x,goal.y);

	relPitch = desiredPitch - curPitch;
	relYaw = desYaw-curYaw;

	orient.rotate_about_i(relPitch);
	orient.rotate_about_j(relYaw);
////------------------------
	transform = orient;

	hSound = SFXMANAGER->Open(data->launchSfx);
	SFXMANAGER->Play(hSound,systemID,pos);

	bGone = false;
	bDeleteRequested = false;

	objMapSystemID = systemID;
	objMapSquare = OBJMAP->GetMapSquare(systemID,transform.translation);
	OBJMAP->AddObjectToMap(this,objMapSystemID,objMapSquare);

	time = data->duration;
}
//----------------------------------------------------------------------------------
//
void DumbReconProbe::ExplodeProbe()
{
	if(objMapSystemID)
	{
		OBJMAP->RemoveObjectFromMap(this,objMapSystemID,objMapSquare);
		objMapSystemID = 0;
	}
	systemID = 0;
	bGone = true;
	SetReady(false);
}
//----------------------------------------------------------------------------------
//
void DumbReconProbe::DeleteProbe()
{
	bLauncherDelete = true;
}
//----------------------------------------------------------------------------------
//
bool DumbReconProbe::IsActive()
{
	return !bGone;
}
//---------------------------------------------------------------------------
//
BOOL32 DumbReconProbe::Save (struct IFileSystem * inFile)
{
	DAFILEDESC fdesc = "DUMBRECONPROBE_SAVELOAD";
	COMPTR<IFileSystem> file;
	BOOL32 result = 0;
	DWORD dwWritten;
	DUMBRECONPROBE_SAVELOAD save;

	fdesc.lpImplementation = "DOS";
	fdesc.dwDesiredAccess = GENERIC_READ|GENERIC_WRITE;
	fdesc.dwShareMode = 0;  // no sharing
	fdesc.dwCreationDistribution = CREATE_ALWAYS;

	if (inFile->CreateInstance(&fdesc, file) != GR_OK)
		goto Done;

	memset(&save, 0, sizeof(save));

	memcpy(&save, static_cast<BASE_DUMBRECONPROBE_SAVELOAD *>(this), sizeof(BASE_DUMBRECONPROBE_SAVELOAD));

	FRAME_save(save);

	file->WriteFile(0, &save, sizeof(save), &dwWritten, 0);

	result = 1;

Done:	
	return result;
}
//---------------------------------------------------------------------------
//
BOOL32 DumbReconProbe::Load (struct IFileSystem * inFile)
{	
	DAFILEDESC fdesc = "DUMBRECONPROBE_SAVELOAD";
	COMPTR<IFileSystem> file;
	BOOL32 result = 0;
	DWORD dwRead;
	DUMBRECONPROBE_SAVELOAD load;
	U8 buffer[1024];
	
	fdesc.lpImplementation = "DOS";
	if (inFile->CreateInstance(&fdesc, file) != GR_OK)
		goto Done;

	if (file->ReadFile(0, buffer, sizeof(buffer), &dwRead, 0) == 0)
		goto Done;
	MISSION->CorrelateSymbol("DUMBRECONPROBE_SAVELOAD", buffer, &load);

	FRAME_load(load);

	*static_cast<BASE_DUMBRECONPROBE_SAVELOAD *>(this) = load;

	result = 1;

Done:	
	return result;
}
//---------------------------------------------------------------------------
//
void DumbReconProbe::ResolveAssociations()
{
	OBJLIST->FindObject(ownerID, NONSYSVOLATILEPTR, owner, IBaseObjectID);

	if(targetID)
		OBJLIST->FindObject(targetID, NONSYSVOLATILEPTR, target, IBaseObjectID);

	objMapSystemID = systemID;
	if(systemID)
	{
		objMapSquare = OBJMAP->GetMapSquare(systemID,transform.translation);
		OBJMAP->AddObjectToMap(this,systemID,objMapSquare);
	}
}
//---------------------------------------------------------------------------
//
U32 DumbReconProbe::getSyncData (void * buffer)
{
	if(bNoMoreSync)
		return 0;
	if(bLauncherDelete && bDeleteRequested)
	{
		bLauncherDelete = false;
		bNoMoreSync = true;
		OBJLIST->DeferredDestruction(dwMissionID);
		((U8*) buffer)[0] = 1;
		return 1;
	}
	if(bGone || bDeleteRequested)
		return 0;
	if(time <= 0)
	{
		bDeleteRequested = true;
		((U8*) buffer)[0] = 0;
		return 1;
	}
	return 0;
}
//---------------------------------------------------------------------------
//
void DumbReconProbe::putSyncData (void * buffer, U32 bufferSize, bool bLateDelivery)
{
	if(bufferSize == 1)
	{
		U32 val = ((U8*) buffer)[0];
		if(val == 1)
		{
			bLauncherDelete = false;
			bNoMoreSync = true;
			OBJLIST->DeferredDestruction(dwMissionID);
		}
		else if(val == 0)
		{
			bDeleteRequested = true;
		}
	}
}
//---------------------------------------------------------------------------
//
void DumbReconProbe::init (DUMBRECONPROBE_INIT *initData)
{
	FRAME_init(*initData);
	data = initData->pData;
	arch = initData;

	CQASSERT(data);
	CQASSERT(data->wpnClass == WPN_DUMBRECONPROBE);
	CQASSERT(data->objClass == OC_WEAPON);

	pArchetype = initData->pArchetype;
	objClass = OC_WEAPON;
	bNoMoreSync = false;
	time = 0;
}
//------------------------------------------------------------------------------------------
//---------------------------DumbReconProbe Factory----------------------------------------------
//------------------------------------------------------------------------------------------
//
struct DACOM_NO_VTABLE DumbReconProbeFactory : public IObjectFactory
{
	struct OBJTYPE : DUMBRECONPROBE_INIT
	{
		void * operator new (size_t size)
		{
			return calloc(size, 1);
		}

		void   operator delete (void *ptr)
		{
			::free(ptr);
		}
		
		OBJTYPE (void)
		{
			archIndex = -1;
		}
		
		~OBJTYPE (void)
		{
			ENGINE->release_archetype(archIndex);
		}
	};
	//------------------------------------------------------------------------------------------
	//
	U32 factoryHandle;		// handles to callback

	//
	// Interface mapping
	//

	BEGIN_DACOM_MAP_INBOUND(DumbReconProbeFactory)
	DACOM_INTERFACE_ENTRY(IObjectFactory)
	END_DACOM_MAP()

	DumbReconProbeFactory (void) { }

	~DumbReconProbeFactory (void);

	void init (void);

    void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}

	// IObjectFactory methods 

	DEFMETHOD_(HANDLE,CreateArchetype) (const char *szArchname, OBJCLASS objClass, void *data);

	DEFMETHOD_(BOOL32,DestroyArchetype) (HANDLE hArchetype);

	DEFMETHOD_(IBaseObject *,CreateInstance) (HANDLE hArchetype);

	DEFMETHOD_(void,EditorCreateInstance) (HANDLE hArchetype, const MISSION_INFO & info);

	// DumbReconProbeFactory methods 

	IDAComponent * getBase (void)
	{
		return static_cast<IObjectFactory *> (this);
	}
};
//--------------------------------------------------------------------------//
//
DumbReconProbeFactory::~DumbReconProbeFactory (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (OBJLIST && OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
		connection->Unadvise(factoryHandle);
}
//--------------------------------------------------------------------------//
//
void DumbReconProbeFactory::init (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
		connection->Advise(getBase(), &factoryHandle);
}
//-----------------------------------------------------------------------------
//
HANDLE DumbReconProbeFactory::CreateArchetype (const char *szArchname, OBJCLASS objClass, void *_data)
{
	OBJTYPE * objArch = 0;

	if (objClass == OC_WEAPON)
	{
		BT_DUMBRECONPROBE_DATA * data = (BT_DUMBRECONPROBE_DATA *)_data;
		if (data->wpnClass == WPN_DUMBRECONPROBE)
		{
			objArch = new OBJTYPE;
			
			objArch->pArchetype = ARCHLIST->GetArchetype(szArchname);
			objArch->pData = data;
			//
			// force preload of sound effect
			// 
			SFXMANAGER->Preload(data->launchSfx);

			DAFILEDESC fdesc = data->fileName;
			COMPTR<IFileSystem> objFile;

			if (OBJECTDIR->CreateInstance(&fdesc, objFile) == GR_OK)
				TEXLIB->load_library(objFile, 0);
			else
				goto Error;

			if ((objArch->archIndex = ENGINE->create_archetype(fdesc.lpFileName, objFile)) == INVALID_ARCHETYPE_INDEX)
				goto Error;

			goto Done;
		}
	}

Error:
	delete objArch;
	objArch = 0;
Done:
	return (HANDLE) objArch;
}
//-------------------------------------------------------------------
//
BOOL32 DumbReconProbeFactory::DestroyArchetype (HANDLE hArchetype)
{
	OBJTYPE * objtype = (OBJTYPE *)hArchetype;

	delete objtype;

	return 1;
}
//-------------------------------------------------------------------
//
IBaseObject * DumbReconProbeFactory::CreateInstance (HANDLE hArchetype)
{
	OBJTYPE * objtype = (OBJTYPE *)hArchetype;

	DumbReconProbe * dumbReconProbe = new ObjectImpl<DumbReconProbe>;

	dumbReconProbe->init(objtype);

	return dumbReconProbe;
}
//-------------------------------------------------------------------
//
void DumbReconProbeFactory::EditorCreateInstance (HANDLE hArchetype, const MISSION_INFO & info)
{
	// does not support this!
}
//---------------------------------------------------------------------------
//-------------------------GLOBAL STARTUP------------------------------------
//---------------------------------------------------------------------------
//
struct _dumbReconProbe : GlobalComponent
{
	DumbReconProbeFactory * sfactory;

	virtual void Startup (void)
	{
		sfactory = new DAComponent<DumbReconProbeFactory>;
		AddToGlobalCleanupList((IDAComponent **) &sfactory);
	}

	virtual void Initialize (void)
	{
		sfactory->init();
	}
};

static _dumbReconProbe __dumbReconProbe;
//---------------------------------------------------------------------------
//------------------------End DumbReconProbe.cpp----------------------------------------
//---------------------------------------------------------------------------


