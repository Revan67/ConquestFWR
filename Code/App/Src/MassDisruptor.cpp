//--------------------------------------------------------------------------//
//                                                                          //
//                                 MassDisruptor.cpp                                 //
//                                                                          //
//                  COPYRIGHT (C) 1997 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*

    $Author: Jasony $

    $Header: /Conquest/App/Src/MassDisruptor.cpp 18    11/02/00 1:24p Jasony $
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
#include "IWeapon.h"
#include "IExplosion.h"
#include "TObjTrans.h"
#include "TObjFrame.h"
#include "Mesh.h"
#include "ArchHolder.h"
#include "Anim2d.h"
#include "FogOfWar.h"
#include "UserDefaults.h"
#include "Mission.h"
#include "OpAgent.h"
#include "IMissionActor.h"
#include <MGlobals.h>
#include "MPart.h"
#include "GridVector.h"
#include "DSpecial.h"
#include "MeshRender.h"
#include "CQLight.h"
#include "IBlast.h"

#include <Renderer.h>
#include <TComponent.h>
#include <Engine.h>
#include <Vector.h>
#include <Physics.h>
#include <Matrix.h>
#include <IHardpoint.h>
#include <stdlib.h>
#include <FileSys.h>
#include <ICamera.h>
#include <Collision.h>
#include <Pixel.h>
//#include <RPUL\PrimitiveBuilder.h>
#include <IRenderPrimitive.h>


//----------------------------------------------------------------------------

struct MASS_DISRUPTOR_INIT
{
	S32 archIndex;
	IMeshArchetype * meshArch;
	PARCHETYPE pArchetype;
	AnimArchetype *boltAnimArch,*warpAnimArch;
	PARCHETYPE pContactBlast;
	
	MASS_DISRUPTOR_INIT (void)
	{
		meshArch = NULL;
		archIndex = -1;
	}
		
	~MASS_DISRUPTOR_INIT (void)
	{
		if (boltAnimArch)
			delete boltAnimArch;
		if (warpAnimArch)
			delete warpAnimArch;
		if (pContactBlast)
			ARCHLIST->Release(pContactBlast, OBJREFNAME);
	}
};

struct _NO_VTABLE MassDisruptor : public ObjectTransform<ObjectFrame<IBaseObject,struct MASS_DISRUPTOR_SAVELOAD,struct MASS_DISRUPTOR_INIT> >, BASE_MASS_DISRUPTOR_SAVELOAD, IWeapon, ISaveLoad
{
	BEGIN_MAP_INBOUND(MassDisruptor)
	_INTERFACE_ENTRY(IBaseObject)
	_INTERFACE_ENTRY(IWeapon)
	_INTERFACE_ENTRY(ISaveLoad)
	END_MAP()

	//------------------------------------------
	U32 ownerID;
	U32 systemID;
	
	SINGLE time;
	const BT_MASS_DISRUPTOR_DATA * data;
	MASS_DISRUPTOR_INIT *arch;
	HSOUND hSound;
	S32 beamCounter;
	SINGLE timer;
	OBJPTR<IWeaponTarget> target;
	OBJPTR<ILaunchOwner> owner;
	MeshChain *mc;
	Vector scale;
	AnimInstance boltAnim,warpAnim;
	OBJPTR<IBlast> contactBlast;

	bool bDeleteRequested:1;
	
	//------------------------------------------

	MassDisruptor (void) 
	{
	}

    void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}

	virtual ~MassDisruptor (void);

	virtual BOOL32 Update (void);

	virtual void PhysicalUpdate (SINGLE dt);

	virtual void Render (void);

	virtual U32 GetSystemID (void) const;

	virtual U32 GetPlayerID (void) const
	{
		return MGlobals::GetPlayerFromPartID(ownerID);
	}

	/* IAOEWeapon methods */

	virtual void InitWeapon (IBaseObject * owner, const class TRANSFORM & orientation, IBaseObject * obj, U32 flags, const class Vector * pos=0);

	// weapon should determine who it will damage, and how much, then return the result to the caller
//	virtual U32 GetAffectedUnits (U32 partIDs[MAX_AOE_VICTIMS], U32 damage[MAX_AOE_VICTIMS]);

	// caller has determined who it will damage, and how much.
//	virtual void SetAffectedUnits (const U32 partIDs[MAX_AOE_VICTIMS], const U32 damage[MAX_AOE_VICTIMS]);

	/* ISaveLoad methods */

	virtual BOOL32 Save (struct IFileSystem * file);

	virtual BOOL32 Load (struct IFileSystem * file);

	virtual void ResolveAssociations();

	///////////////////

	void init (MASS_DISRUPTOR_INIT *initData);
	
//	void GenerateSparkMesh(INSTANCE_INDEX index);

//	void GenerateExtents(INSTANCE_INDEX index);

//	void GenerateSparks();
};

//----------------------------------------------------------------------------------
//
MassDisruptor::~MassDisruptor (void)
{
//	TXMLIB->release_texture(textureID);
/*	for (int i=0;i<3;i++)
		textureID[i] = 0;*/
	if (hSound)
		SFXMANAGER->CloseHandle(hSound);
	if (contactBlast)
		delete contactBlast.Ptr();
}
//----------------------------------------------------------------------------------
//
U32 MassDisruptor::GetSystemID (void) const
{
	return systemID;
}
//----------------------------------------------------------------------------------
//
void MassDisruptor::PhysicalUpdate (SINGLE dt)
{
	if ((!target) || (!owner))
	{
		bDeleteRequested = true;
		return;
	}

	timer += dt;
	if (stage == MD_SHOOT)
	{
//#define SHOOT_TIME 0.5
	//	if (timer >= SHOOT_TIME)

		dist -= data->boltSpeed*dt;
		
		if (dist < 400)
		{
			timer=0.0f;
			stage = MD_DISRUPT;
			IBaseObject * b_obj;
			b_obj = ARCHLIST->CreateInstance(arch->pContactBlast);
			if (b_obj)
			{
				b_obj->QueryInterface(IBlastID,contactBlast,NONSYSVOLATILEPTR);
				CQASSERT(contactBlast && "Not a blast!!");
				TRANSFORM trans;
				Vector look = CAMERA->GetPosition()-transform.translation;
				Vector i;
				i = cross_product(look,targetDir);
				
#define TOLERANCE 1e-3
				if (fabs (i.x) < TOLERANCE && fabs (i.y) < TOLERANCE)
				{
					i.x = 1.0f;
				}
				
				i.normalize ();
				Vector j = cross_product(i,targetDir);
				trans.set_i(i);
				trans.set_j(j);
				trans.set_k(targetDir);
				trans.translation = transform.translation;
				contactBlast->InitBlast(trans,owner.Ptr()->GetSystemID(),0);
				contactBlast->SetVisible(true);
			}
			//	target.ptr->bSpecialRender = true;
		}

	}

	if (stage == MD_DISRUPT)
	{
#define DISRUPT_TIME 3.0
		if (timer >= DISRUPT_TIME)
		{
			bDeleteRequested = true;
		//	target.ptr->bSpecialRender = false;
		}
	}

	if (contactBlast)
		contactBlast.Ptr()->PhysicalUpdate(dt);
}
//----------------------------------------------------------------------------------
//
BOOL32 MassDisruptor::Update (void)
{
	BOOL32 result = 1;

	if (bDeleteRequested)
	{
		result = 0;            //we are done!
		if(target.Ptr())
		{
			MPart part(target.Ptr());
			S32 damage = F2LONG(data->damagePercent*part->hullPointsMax);
			target->ApplyAOEDamage(ownerID,damage);
		}
	}

	if (contactBlast)
		contactBlast.Ptr()->Update();

	return result;
}
//----------------------------------------------------------------------------------
//
void MassDisruptor::Render (void)
{
	if(!target || !owner)
		return;
	
	if (systemID == SECTOR->GetCurrentSystem())
	{
		SINGLE dt = OBJLIST->GetRealRenderTime();
		if (stage == MD_SHOOT)
		{
			BATCH->set_state(RPR_BATCH,TRUE);
			BATCH->set_render_state(D3DRS_ZENABLE,TRUE);
			BATCH->set_render_state(D3DRS_ZWRITEENABLE,FALSE);
			BATCH->set_render_state(D3DRS_ALPHABLENDENABLE,TRUE);
			BATCH->set_render_state(D3DRS_SRCBLEND,D3DBLEND_ONE);
			BATCH->set_render_state(D3DRS_DESTBLEND,D3DBLEND_ONE);
			boltAnim.update(dt);
			transform.translation = target.Ptr()->GetPosition()-targetDir*dist;
			boltAnim.SetPosition(transform.translation);
			ANIM2D->render(&boltAnim);
		}
		
		if (stage == MD_DISRUPT)
		{
			LIGHTS->ActivateBestLights(target.Ptr()->GetTransform().translation,8,3000);
			
			puffViewPt[0].set(cos(8*timer),sin(7*timer),sin(6*timer));
			puffViewPt[0].normalize();
			warpAnim.update(dt);
			puffTexture = warpAnim.retrieve_current_frame()->texture;
			TreeRenderPuffy(mc->mi,mc->numChildren,scale,0.4);
		}
		
		if (contactBlast)
			contactBlast.Ptr()->Render();
	}
}
//----------------------------------------------------------------------------------
//
void MassDisruptor::InitWeapon (IBaseObject * _owner, const class TRANSFORM & orientation, IBaseObject * _target, U32 flags, const class Vector * pos)
{
	//Correct massDisruptor to fire at target regardless of gun barrel
//	SINGLE curPitch, desiredPitch, relPitch;
//	Vector goal = (_target)? _target->GetPosition() : *pos;

	systemID = _owner->GetSystemID();
	ownerID = _owner->GetPartID();
	U32 visFlags = _owner->GetTrueVisibilityFlags();

	//CQASSERT(_target);
	
	if (_target)
	{
		_target->QueryInterface(IWeaponTargetID, target, NONSYSVOLATILEPTR);
		CQASSERT(target!=0);
		visFlags |= _target->GetTrueVisibilityFlags();
		_owner->SetVisibleToAllies(1 << (_target->GetPlayerID()-1));
		_target->SetVisibleToAllies(1 << (_owner->GetPlayerID()-1));
		targetID = _target->GetPartID();
		
		VOLPTR(IExtent) extentObj = _target;
//		mc = &extentObj->GetMeshChain();
		float box[6];
		_target->GetObjectBox(box);
		scale.set(box[BBOX_MAX_X],box[BBOX_MAX_Y],box[BBOX_MAX_Z]);
		scale *= 1.9;
		
		_owner->QueryInterface(ILaunchOwnerID,owner,NONSYSVOLATILEPTR);
		SetVisibilityFlags(visFlags);
		
		stage = MD_SHOOT;
		
		Vector start = orientation.get_position();
		
		hSound = SFXMANAGER->Open(data->launchSfx);
		SFXMANAGER->Play(hSound,systemID,&start);
		
		targetDir = _target->GetPosition()-start;
		dist = targetDir.magnitude();
		dist += 1.0;
		targetDir /= dist;
	}
	else
		bDeleteRequested = true;
}
//---------------------------------------------------------------------------
//
/*U32 MassDisruptor::GetAffectedUnits (U32 partIDs[MAX_AOE_VICTIMS], U32 damage[MAX_AOE_VICTIMS])
{
	if (partIDs)
		partIDs[0] = targetID;
	//if (damage)
	//	damage[0] = damageDealt;

	return 1;
}
//---------------------------------------------------------------------------
//
void MassDisruptor::SetAffectedUnits (const U32 partIDs[MAX_AOE_VICTIMS], const U32 damage[MAX_AOE_VICTIMS])
{
	targetID = partIDs[0];
	//if (damage)
	//	damageDealt = damage[0];
}*/
//---------------------------------------------------------------------------
//
BOOL32 MassDisruptor::Save (struct IFileSystem * inFile)
{
	DAFILEDESC fdesc = "MASS_DISRUPTOR_SAVELOAD";
	COMPTR<IFileSystem> file;
	BOOL32 result = 0;
	DWORD dwWritten;
	MASS_DISRUPTOR_SAVELOAD save;

	fdesc.lpImplementation = "DOS";
	fdesc.dwDesiredAccess = GENERIC_READ|GENERIC_WRITE;
	fdesc.dwShareMode = 0;  // no sharing
	fdesc.dwCreationDistribution = CREATE_ALWAYS;

	if (inFile->CreateInstance(&fdesc, file) != GR_OK)
		goto Done;

	memset(&save, 0, sizeof(save));
	
	save.ownerID = ownerID;
	save.systemID = systemID;

	FRAME_save(save);

	file->WriteFile(0, &save, sizeof(save), &dwWritten, 0);
	
	result = 1;

Done:	

	return result;
}

BOOL32 MassDisruptor::Load (struct IFileSystem * inFile)
{	
	DAFILEDESC fdesc = "MASS_DISRUPTOR_SAVELOAD";
	COMPTR<IFileSystem> file;
	BOOL32 result = 0;
	DWORD dwRead;
	MASS_DISRUPTOR_SAVELOAD load;
	U8 buffer[1024];

	fdesc.lpImplementation = "DOS";
	if (inFile->CreateInstance(&fdesc, file) != GR_OK)
		goto Done;

	if (file->ReadFile(0, buffer, sizeof(buffer), &dwRead, 0) == 0)
		goto Done;
	MISSION->CorrelateSymbol("MASS_DISRUPTOR_SAVELOAD", buffer, &load);

	FRAME_load(load);

	ownerID = load.ownerID;
	systemID = load.systemID;

	result = 1;

Done:	
	return result;
}

void MassDisruptor::ResolveAssociations()
{
	OBJLIST->FindObject(targetID,NONSYSVOLATILEPTR,target,IWeaponTargetID);
	OBJLIST->FindObject(ownerID,NONSYSVOLATILEPTR,owner,ILaunchOwnerID);
	if(target && owner)
	{
		VOLPTR(IExtent) extentObj = target;
//		mc = &extentObj->GetMeshChain();
		float box[6];
		target.Ptr()->GetObjectBox(box);
		scale.set(box[BBOX_MAX_X],box[BBOX_MAX_Y],box[BBOX_MAX_Z]);
		scale *= 1.9;
	}
	else
		bDeleteRequested = true;
}

void MassDisruptor::init (MASS_DISRUPTOR_INIT *initData)
{
	FRAME_init(*initData);
	data = (const BT_MASS_DISRUPTOR_DATA *) ARCHLIST->GetArchetypeData(initData->pArchetype);
	arch = initData;

	CQASSERT(data);
	CQASSERT(data->wpnClass == WPN_MASS_DISRUPTOR);
	CQASSERT(data->objClass == OC_WEAPON);

	/*if ((textureID = TXMLIB->get_texture_id(data->fileName)) == 0)
	{
		CQASSERT(data->fileName[0]);-
		if ((textureID = CreateTextureFromFile(data->fileName, TEXTURESDIR, DA::TGA, 3)) != 0)
		{
			glBindTexture(GL_TEXTURE_2D,textureID);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glBindTexture(GL_TEXTURE_2D,0);
			TXMLIB->set_texture_id(data->fileName, textureID);
			TXMLIB->get_texture_id(data->fileName);	// add 1 reference
		}
	}*/
	
	if (initData->boltAnimArch)
	{
		boltAnim.Init(initData->boltAnimArch);
		boltAnim.SetWidth(data->animWidth);
	}
	
	if (initData->warpAnimArch)
	{
		warpAnim.Init(initData->warpAnimArch);
	}
	pArchetype = initData->pArchetype;
	objClass = OC_WEAPON;
}

//------------------------------------------------------------------------------------------
//---------------------------MassDisruptor Factory----------------------------------------------
//------------------------------------------------------------------------------------------
//
struct DACOM_NO_VTABLE MassDisruptorFactory : public IObjectFactory
{
	//------------------------------------------------------------------------------------------
	//
	U32 factoryHandle;		// handles to callback

	//
	// Interface mapping
	//

	BEGIN_DACOM_MAP_INBOUND(MassDisruptorFactory)
	DACOM_INTERFACE_ENTRY(IObjectFactory)
	END_DACOM_MAP()

	MassDisruptorFactory (void) { }

	~MassDisruptorFactory (void);

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

	// MassDisruptorFactory methods 

	IDAComponent * getBase (void)
	{
		return static_cast<IObjectFactory *> (this);
	}
};
//--------------------------------------------------------------------------//
//
MassDisruptorFactory::~MassDisruptorFactory (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (OBJLIST && OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
		connection->Unadvise(factoryHandle);
}
//--------------------------------------------------------------------------//
//
void MassDisruptorFactory::init (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
		connection->Advise(getBase(), &factoryHandle);
}
//-----------------------------------------------------------------------------
//
HANDLE MassDisruptorFactory::CreateArchetype (const char *szArchname, OBJCLASS objClass, void *_data)
{
	MASS_DISRUPTOR_INIT * result = 0;

	if (objClass == OC_WEAPON)
	{
		BT_MASS_DISRUPTOR_DATA * data = (BT_MASS_DISRUPTOR_DATA *)_data;
		if (data->wpnClass == WPN_MASS_DISRUPTOR)
		{
			result = new MASS_DISRUPTOR_INIT;
			
			
//			newguy->name = szArchname;
			//result->data = data;
			result->pArchetype = ARCHLIST->GetArchetype(szArchname);
			//
			// force preload of sound effect
			// 
			SFXMANAGER->Preload(data->launchSfx);
			
			result->boltAnimArch = ANIM2D->create_archetype(data->fileName);
			result->warpAnimArch = ANIM2D->create_archetype(data->warpAnim);

			if (data->contactBlastType[0])
			{
				result->pContactBlast = ARCHLIST->LoadArchetype(data->contactBlastType);
				CQASSERT(result->pContactBlast);
				ARCHLIST->AddRef(result->pContactBlast, OBJREFNAME);
			}

			goto Done;
		}
	}

	delete result;
	result = 0;
Done:
	return (HANDLE) result;
}
//-------------------------------------------------------------------
//
BOOL32 MassDisruptorFactory::DestroyArchetype (HANDLE hArchetype)
{
	MASS_DISRUPTOR_INIT * objtype = (MASS_DISRUPTOR_INIT *)hArchetype;

//	EditorStopObjectInsertion(objtype->pArchetype);		(does not allow for this)
	delete objtype;

	return 1;
}
//-------------------------------------------------------------------
//
IBaseObject * MassDisruptorFactory::CreateInstance (HANDLE hArchetype)
{
	MASS_DISRUPTOR_INIT * objtype = (MASS_DISRUPTOR_INIT *)hArchetype;

	MassDisruptor * massDisruptor = new ObjectImpl<MassDisruptor>;

	massDisruptor->init(objtype);

	return massDisruptor;
}
//-------------------------------------------------------------------
//
void MassDisruptorFactory::EditorCreateInstance (HANDLE hArchetype, const MISSION_INFO & info)
{
	// does not support this!
}
//---------------------------------------------------------------------------
//-------------------------GLOBAL STARTUP------------------------------------
//---------------------------------------------------------------------------
//
struct _massDisruptor : GlobalComponent
{
	MassDisruptorFactory * sfactory;

	virtual void Startup (void)
	{
		sfactory = new DAComponent<MassDisruptorFactory>;
		AddToGlobalCleanupList((IDAComponent **) &sfactory);
	}

	virtual void Initialize (void)
	{
		sfactory->init();
	}
};

static _massDisruptor __massDistruptor;
//---------------------------------------------------------------------------
//------------------------End MassDisruptor.cpp------------------------------
//---------------------------------------------------------------------------


