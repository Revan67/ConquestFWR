//--------------------------------------------------------------------------//
//                                                                          //
//                             PlanetKillerBolt.cpp                         //
//                                                                          //
//                  COPYRIGHT (C) 2004 Warthog, INC.                        //
//                                                                          //
//--------------------------------------------------------------------------//
/*

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
#include <MGlobals.h>
#include "TManager.h"
#include "ObjMapIterator.h"
#include "MPart.h"
#include "IVertexBuffer.h"
#include "TObjRender.h"
#include "IPlanet.h"
#include "IEngineTrail.h"

#include <Renderer.h>
#include <TComponent.h>
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
#include <IRenderPrimitive.h>

#define EFFECT_TIME 1.5
#define EFFECT_RATE 3

struct PKBOLT_INIT : RenderArch
{
	S32 archIndex;
	IMeshArchetype * meshArch;
	PARCHETYPE pArchetype;
	PARCHETYPE pExplosion;
	PARCHETYPE pEngineTrailType;
};

#define PKBOLT_SEGMENTS 40

struct PKBoltMesh : IVertexBufferOwner
{
	Vector ringCenter[PKBOLT_SEGMENTS];
	Vector upperRing[PKBOLT_SEGMENTS];
	U32 ringTexID;
	U32 moveTexID;
	U32 vb_handle;
	
	virtual void RestoreVertexBuffers();

	PKBoltMesh()
	{
		vb_handle = 0;
		vb_mgr->Add(this);
	}

	~PKBoltMesh()
	{
		vb_mgr->Delete(this);
	}

}pkBoltMesh;

void PKBoltMesh::RestoreVertexBuffers()
{
	if (CQRENDERFLAGS.bSoftwareRenderer==0)
	{
		Vertex2 *vb_data;
		U32 dwSize;
		GENRESULT result;
		if (vb_handle)
		{
			result = PIPE->destroy_vertex_buffer(vb_handle);
			CQASSERT(result == GR_OK);
		}
		result = PIPE->create_vertex_buffer( D3DFVF_RPVERTEX2, PKBOLT_SEGMENTS*2+2, IRP_VBF_SYSTEM, &pkBoltMesh.vb_handle );
		CQASSERT(result == GR_OK);
		result = PIPE->lock_vertex_buffer( pkBoltMesh.vb_handle, DDLOCK_WRITEONLY, (void **)&vb_data, &dwSize );
		CQASSERT(result == GR_OK);
		
		for(int i = 0; i < PKBOLT_SEGMENTS; ++i)
		{
			vb_data[i*2].u2 = i;
			vb_data[i*2].u = i+1;
			vb_data[i*2].v = 1;
			vb_data[i*2].pos = pkBoltMesh.ringCenter[i];
			
			vb_data[i*2+1].color = 0x00ffffff;
			vb_data[i*2+1].u2 = i+0.5f;
			vb_data[i*2+1].u = i+0.5f;
			vb_data[i*2+1].v = 0;
			vb_data[i*2+1].pos = pkBoltMesh.upperRing[i];
		}
		
		vb_data[PKBOLT_SEGMENTS*2].u = PKBOLT_SEGMENTS+1;
		vb_data[PKBOLT_SEGMENTS*2].v = 1;
		vb_data[PKBOLT_SEGMENTS*2].u2 = PKBOLT_SEGMENTS;
		vb_data[PKBOLT_SEGMENTS*2].pos = pkBoltMesh.ringCenter[0];
		
		vb_data[PKBOLT_SEGMENTS*2+1].color = 0x00ffffff;
		vb_data[PKBOLT_SEGMENTS*2+1].u = PKBOLT_SEGMENTS+0.5f;
		vb_data[PKBOLT_SEGMENTS*2+1].v = 0;
		vb_data[PKBOLT_SEGMENTS*2+1].u2 = PKBOLT_SEGMENTS+0.5f;
		vb_data[PKBOLT_SEGMENTS*2+1].pos = pkBoltMesh.upperRing[0];
		
		result = PIPE->unlock_vertex_buffer( pkBoltMesh.vb_handle );
		CQASSERT(result == GR_OK);
	}
	else
		pkBoltMesh.vb_handle = 0;
}

struct _NO_VTABLE PKBolt : public ObjectRender<ObjectTransform<ObjectFrame<IBaseObject,struct PKBOLT_SAVELOAD,struct PKBOLT_INIT> > >, IAOEWeapon, ISaveLoad
{
	BEGIN_MAP_INBOUND(PKBolt)
	_INTERFACE_ENTRY(IBaseObject)
	_INTERFACE_ENTRY(IAOEWeapon)
	_INTERFACE_ENTRY(ISaveLoad)
	END_MAP()

	//------------------------------------------
	OBJPTR<IBaseObject> owner;
	OBJPTR<IBaseObject> target;

	U32 ownerID;
	U32 targetID;
	U32 systemID;
	
	const BT_PKBOLT_DATA * data;
	HSOUND hSound;

	PARCHETYPE pExplosion;
	bool bReadyToDelete;
	bool bDamagePlats;

	OBJPTR<IEngineTrail> trail;

	//shockwave
	SINGLE shockTime;
	bool bShockOn;
	U32 multiStages;

	//------------------------------------------

	PKBolt (void) 
	{
		bReadyToDelete = false;
		bDamagePlats = false;
		bShockOn = false;
		if (CQRENDERFLAGS.bMultiTexture)
			multiStages = 0xffffffff;
	}

    void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}

	virtual ~PKBolt (void);

	virtual BOOL32 Update (void);

	virtual void PhysicalUpdate (SINGLE dt);

	virtual void Render (void);

	virtual U32 GetSystemID (void) const;

	virtual U32 GetPlayerID (void) const
	{
		return MGlobals::GetPlayerFromPartID(ownerID);
	}

	virtual void CastVisibleArea();

	/* IAOEWeapon methods */

	virtual void InitWeapon (IBaseObject * owner, const class TRANSFORM & orientation, IBaseObject * obj, U32 flags, const class Vector * pos=0);

	// weapon should determine who it will damage, and how much, then return the result to the caller
	virtual U32 GetAffectedUnits (U32 partIDs[MAX_AOE_VICTIMS], U32 damage[MAX_AOE_VICTIMS]);

	// caller has determined who it will damage, and how much.
	virtual void SetAffectedUnits (const U32 partIDs[MAX_AOE_VICTIMS], const U32 damage[MAX_AOE_VICTIMS]);

	/* ISaveLoad methods */

	virtual BOOL32 Save (struct IFileSystem * file);

	virtual BOOL32 Load (struct IFileSystem * file);

	virtual void ResolveAssociations();

	///////////////////

	void init (PKBOLT_INIT &initData);
	
};

//----------------------------------------------------------------------------------
//
PKBolt::~PKBolt (void)
{
	if(trail != 0)
	{
		delete trail.Ptr();
		trail = 0;
	}
	if (hSound)
		SFXMANAGER->CloseHandle(hSound);
}
//----------------------------------------------------------------------------------
//
U32 PKBolt::GetSystemID (void) const
{
	return systemID;
}
//----------------------------------------------------------------------------------
//
void PKBolt::CastVisibleArea()
{
	SetVisibleToAllies(GetVisibilityFlags());
}
//----------------------------------------------------------------------------------
//
static const SINGLE SHOCK_TIME = 2.0f;
static const SINGLE SHOCK_SCALE = 10000.0f;

void PKBolt::PhysicalUpdate (SINGLE dt)
{
	if(!target)
		return;
	SINGLE distSq = (target->GetPosition()-transform.translation).magnitude_squared();
	SINGLE moveDist = data->maxVelocity*dt;
	if(distSq < 2500*2500)
	{
		if (!bReadyToDelete && !bDamagePlats)
		{
			IBaseObject * obj = CreateBlast(pExplosion,transform, systemID);
			CQASSERT(obj);
			OBJLIST->AddObject(obj);
			bDamagePlats = true;
			bShockOn = true;
			shockTime = 0;
			VOLPTR(IPlanet) planet = target;
			if(planet)
			{
				Vector hitDir = target->GetPosition()-GetPosition();
				hitDir.fast_normalize();
				planet->TeraformPlanet(data->newPlanetType,data->changeTime,hitDir);
			}
		}
	}
	else
	{
		transform.translation = transform.translation + ((target->GetPosition()-transform.translation).normalize())*moveDist;
	}

	if(bShockOn)
	{
		shockTime += dt;
		if(shockTime > SHOCK_TIME)
			bShockOn = false;
	}

	if(trail != 0)
		trail->Update();
	if(trail != 0)
		trail->PhysicalUpdate(dt);
}
//----------------------------------------------------------------------------------
//
BOOL32 PKBolt::Update (void)
{
	if(!target)
		return false;
	if(bDamagePlats)
	{
		if(!bShockOn)
			bReadyToDelete = true;
		VOLPTR(IPlanet) planet = target;
		if(planet)
		{
			for(U32 i = 0; i < 12; ++i)
			{
				U32 id = planet->GetSlotUser(0x00000001 << i);
				IBaseObject * obj = OBJLIST->FindObject( id);
				if(obj)
				{
					VOLPTR(IWeaponTarget) targ = obj;
					if(targ)
					{
						bReadyToDelete = false;
						targ->ApplyAOEDamage(1,100);
					}
				}
			}
		}
	}
	return !bReadyToDelete;
}

//----------------------------------------------------------------------------------
//
void PKBolt::Render (void)
{
	if (bVisible)
	{
		if(!bReadyToDelete)
		{
			LIGHT->deactivate_all_lights();
			LIGHTS->ActivateBestLights(transform.translation,8,4000);

			if(!bDamagePlats)
			{
				TreeRender(mc);

				if(trail != 0)
					trail->Render();
			}

			//render shock ring
			if(bShockOn)
			{
				SINGLE delta = shockTime/SHOCK_TIME;
				SINGLE scale = delta*SHOCK_SCALE;
				U32 alpha = (1.0f-delta)*255;

				TRANSFORM trans = transform;
				trans.scale(scale);

				BATCH->set_state(RPR_BATCH,false);
				CAMERA->SetModelView(&trans);
				BATCH->set_render_state(D3DRS_ZENABLE,TRUE);
				BATCH->set_render_state(D3DRS_ZWRITEENABLE,FALSE);
				BATCH->set_render_state(D3DRS_ALPHABLENDENABLE,TRUE);
				BATCH->set_render_state(D3DRS_SRCBLEND,D3DBLEND_SRCALPHA);
				BATCH->set_render_state(D3DRS_DESTBLEND,D3DBLEND_ONE);//INVSRCALPHA);
				BATCH->set_render_state(D3DRS_DITHERENABLE,TRUE);

				if(multiStages == 1 || multiStages == 0xffffffff)
				{
				//	BATCH->set_state(RPR_BATCH,false);
					BATCH->set_texture_stage_texture( 0, pkBoltMesh.ringTexID );
					BATCH->set_texture_stage_texture( 1, pkBoltMesh.moveTexID );
					
					// blending - This is the same as D3DTMAPBLEND_MODULATEALPHA
					BATCH->set_texture_stage_state( 0, D3DTSS_COLOROP, D3DTOP_MODULATE );
					BATCH->set_texture_stage_state( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE  );
					BATCH->set_texture_stage_state( 0, D3DTSS_COLORARG2, D3DTA_DIFFUSE );
					BATCH->set_texture_stage_state( 0, D3DTSS_ALPHAOP, D3DTOP_MODULATE );
					BATCH->set_texture_stage_state( 0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE );
					BATCH->set_texture_stage_state( 0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE );
								
					// addressing - clamped
					BATCH->set_sampler_state( 0, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP );
					BATCH->set_sampler_state( 0, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP );
					
					BATCH->set_texture_stage_state( 1, D3DTSS_TEXCOORDINDEX, 1);
					// blending - This is the same as D3DTMAPBLEND_MODULATEALPHA
					BATCH->set_texture_stage_state( 1, D3DTSS_COLOROP, D3DTOP_MODULATE );
					BATCH->set_texture_stage_state( 1, D3DTSS_COLORARG1, D3DTA_TEXTURE );
					BATCH->set_texture_stage_state( 1, D3DTSS_COLORARG2, D3DTA_CURRENT);
					BATCH->set_texture_stage_state( 1, D3DTSS_ALPHAOP, D3DTOP_MODULATE );
					BATCH->set_texture_stage_state( 1, D3DTSS_ALPHAARG1, D3DTA_TEXTURE );
					BATCH->set_texture_stage_state( 1, D3DTSS_ALPHAARG2, D3DTA_CURRENT);
								
					// addressing - clamped
					BATCH->set_sampler_state( 1, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP );
					BATCH->set_sampler_state( 1, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP );
				}
				
				if (multiStages == 0xffffffff)
				{
					multiStages = (BATCH->verify_state() == GR_OK);
				}
				
				if (multiStages != 1)
				{
					SetupDiffuseBlend(pkBoltMesh.ringTexID,FALSE);
				}

				Vertex2 *vb_data;
				U32 dwSize;
				GENRESULT result;
				result = PIPE->lock_vertex_buffer( pkBoltMesh.vb_handle, DDLOCK_WRITEONLY, (void **)&vb_data, &dwSize );
				CQASSERT(result == GR_OK);

				SINGLE timeDif = delta*3;
				SINGLE timeDif2 = timeDif-1;
				int i;
				for(i = 0; i < PKBOLT_SEGMENTS; ++i)
				{
					vb_data[i*2].color = alpha<<24 | 0x00ffffff;
					vb_data[i*2].v2 = i+timeDif;
					vb_data[i*2+1].v2 = i+timeDif2;				
				}

				vb_data[PKBOLT_SEGMENTS*2].color = alpha<<24 | 0x00ffffff;
				vb_data[PKBOLT_SEGMENTS*2].v2 = PKBOLT_SEGMENTS+timeDif;
				vb_data[PKBOLT_SEGMENTS*2+1].v2 = PKBOLT_SEGMENTS+timeDif2;

				result = PIPE->unlock_vertex_buffer( pkBoltMesh.vb_handle );
				CQASSERT(result == GR_OK);
			
				int pass;
				for (pass=0;pass<2;pass++)
				{
					if (pass)
					{
						trans.d[0][2] *= -1;
						trans.d[1][2] *= -1;
						trans.d[2][2] *= -1;
						CAMERA->SetModelView(&trans);
					}

					result = PIPE->draw_primitive_vb( D3DPT_TRIANGLESTRIP, pkBoltMesh.vb_handle, 0, PKBOLT_SEGMENTS*2+2, 0 );
					CQASSERT(result == GR_OK);
				}
				trans.scale(0.8);
				trans.translation+= -transform.get_k()*1000;
				CAMERA->SetModelView(&trans);
				for (pass=0;pass<2;pass++)
				{
					if (pass)
					{
						trans.d[0][2] *= -1;
						trans.d[1][2] *= -1;
						trans.d[2][2] *= -1;
						CAMERA->SetModelView(&trans);
					}

					result = PIPE->draw_primitive_vb( D3DPT_TRIANGLESTRIP, pkBoltMesh.vb_handle, 0, PKBOLT_SEGMENTS*2+2, 0 );
					CQASSERT(result == GR_OK);
				}
				trans.scale(0.8);
				trans.translation+= -transform.get_k()*1000;
				CAMERA->SetModelView(&trans);
				for (pass=0;pass<2;pass++)
				{
					if (pass)
					{
						trans.d[0][2] *= -1;
						trans.d[1][2] *= -1;
						trans.d[2][2] *= -1;
						CAMERA->SetModelView(&trans);
					}

					result = PIPE->draw_primitive_vb( D3DPT_TRIANGLESTRIP, pkBoltMesh.vb_handle, 0, PKBOLT_SEGMENTS*2+2, 0 );
					CQASSERT(result == GR_OK);
				}
			}
		}
	}
}

//----------------------------------------------------------------------------------
//
void PKBolt::InitWeapon (IBaseObject * _owner, const class TRANSFORM & orientation, IBaseObject * _target, U32 flags, const class Vector * pos)
{
	if(_target && _target->objClass == OC_PLANETOID)
	{
		_target->QueryInterface(IBaseObjectID,target,NONSYSVOLATILEPTR);
		targetID = _target->GetPartID();
	}
	else
	{
		return;
	}

	_owner->QueryInterface(IBaseObjectID,owner,NONSYSVOLATILEPTR);

	systemID = owner->GetSystemID();
	ownerID = owner->GetPartID();
	U32 visFlags = owner->GetTrueVisibilityFlags();

	SetVisibilityFlags(visFlags);

	TRANSFORM orient = orientation;
	Vector start = orientation.get_position();

	//Correct bolt to fire at target regardless of gun barrel
	SINGLE curPitch, desiredPitch, relPitch,dist,curYaw,relYaw,desYaw;
	Vector goal = _target->GetPosition(); 

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
}
//---------------------------------------------------------------------------
//
U32 PKBolt::GetAffectedUnits (U32 partIDs[MAX_AOE_VICTIMS], U32 damage[MAX_AOE_VICTIMS])
{
	return 0;
}
//---------------------------------------------------------------------------
//
void PKBolt::SetAffectedUnits (const U32 partIDs[MAX_AOE_VICTIMS], const U32 damage[MAX_AOE_VICTIMS])
{
}
//---------------------------------------------------------------------------
//
BOOL32 PKBolt::Save (struct IFileSystem * inFile)
{
	DAFILEDESC fdesc = "PKBOLT_SAVELOAD";
	COMPTR<IFileSystem> file;
	BOOL32 result = 0;
	DWORD dwWritten;
	PKBOLT_SAVELOAD save;

	fdesc.lpImplementation = "DOS";
	fdesc.dwDesiredAccess = GENERIC_READ|GENERIC_WRITE;
	fdesc.dwShareMode = 0;  // no sharing
	fdesc.dwCreationDistribution = CREATE_ALWAYS;

	if (inFile->CreateInstance(&fdesc, file) != GR_OK)
		goto Done;

	memset(&save, 0, sizeof(save));
	
	save.ownerID = ownerID;
	save.systemID = systemID;
	save.targetID = targetID;

	FRAME_save(save);

	file->WriteFile(0, &save, sizeof(save), &dwWritten, 0);

	result = 1;

Done:	
	return result;
}

BOOL32 PKBolt::Load (struct IFileSystem * inFile)
{	
	DAFILEDESC fdesc = "PKBOLT_SAVELOAD";
	COMPTR<IFileSystem> file;
	BOOL32 result = 0;
	DWORD dwRead;
	PKBOLT_SAVELOAD load;
	U8 buffer[1024];

	fdesc.lpImplementation = "DOS";
	if (inFile->CreateInstance(&fdesc, file) != GR_OK)
		goto Done;

	if (file->ReadFile(0, buffer, sizeof(buffer), &dwRead, 0) == 0)
		goto Done;
	MISSION->CorrelateSymbol("PKBOLT_SAVELOAD", buffer, &load);

	FRAME_load(load);

	ownerID = load.ownerID;
	systemID = load.systemID;
	targetID = load.targetID;

	result = 1;

Done:	
	return result;
}

void PKBolt::ResolveAssociations()
{
	OBJLIST->FindObject(ownerID,NONSYSVOLATILEPTR,owner,IBaseObjectID);
	if(targetID)
		OBJLIST->FindObject(targetID,NONSYSVOLATILEPTR,target,IBaseObjectID);
}

void PKBolt::init (PKBOLT_INIT &initData)
{
	FRAME_init(initData);
	data = (const BT_PKBOLT_DATA *) ARCHLIST->GetArchetypeData(initData.pArchetype);

	CQASSERT(data);
	CQASSERT(data->wpnClass == WPN_PKBOLT);
	CQASSERT(data->objClass == OC_WEAPON);

	pArchetype = initData.pArchetype;
	objClass = OC_WEAPON;
	pExplosion = initData.pExplosion;

	if(initData.pEngineTrailType != 0)
	{
		IBaseObject * obj = ARCHLIST->CreateInstance(initData.pEngineTrailType);
		if(obj)
		{
			obj->QueryInterface(IEngineTrailID,trail,NONSYSVOLATILEPTR);
			if(trail != 0)
			{
				trail->InitEngineTrail(this,instanceIndex);
			}
		}
	}

	if(trail != 0)
		trail->Reset();
}
//------------------------------------------------------------------------------------------
//---------------------------ArcCannon Factory----------------------------------------------
//------------------------------------------------------------------------------------------
//
struct DACOM_NO_VTABLE PKBoltFactory : public IObjectFactory
{
	struct OBJTYPE : PKBOLT_INIT
	{
		void * operator new (size_t size)
		{
			return calloc(size,1);
		}
		
		OBJTYPE (void)
		{
			archIndex = -1;
			pExplosion = 0;
		}
		
		~OBJTYPE (void)
		{
			ENGINE->release_archetype(archIndex);
			if (pExplosion)
				ARCHLIST->Release(pExplosion, OBJREFNAME);
			if (pEngineTrailType)
				ARCHLIST->Release(pEngineTrailType, OBJREFNAME);
			
		}
	};
	//------------------------------------------------------------------------------------------
	//
	U32 factoryHandle;		// handles to callback

	//
	// Interface mapping
	//

	BEGIN_DACOM_MAP_INBOUND(PKBoltFactory)
	DACOM_INTERFACE_ENTRY(IObjectFactory)
	END_DACOM_MAP()

	PKBoltFactory (void) { }

	~PKBoltFactory (void);

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

	// PKBoltFactory methods 

	IDAComponent * getBase (void)
	{
		return static_cast<IObjectFactory *> (this);
	}
};
//--------------------------------------------------------------------------//
//
PKBoltFactory::~PKBoltFactory (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (OBJLIST && OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
		connection->Unadvise(factoryHandle);
}
//--------------------------------------------------------------------------//
//
void PKBoltFactory::init (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
		connection->Advise(getBase(), &factoryHandle);
}
//-----------------------------------------------------------------------------
//
HANDLE PKBoltFactory::CreateArchetype (const char *szArchname, OBJCLASS objClass, void *_data)
{
	OBJTYPE * objArch = 0;

	if (objClass == OC_WEAPON)
	{
		BT_PKBOLT_DATA * data = (BT_PKBOLT_DATA *)_data;
		if (data->wpnClass == WPN_PKBOLT)
		{
			objArch = new OBJTYPE;
			
			objArch->pArchetype = ARCHLIST->GetArchetype(szArchname);
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
		
			//build the global mesh
			for(U32 i = 0; i < PKBOLT_SEGMENTS; ++i)
			{
				pkBoltMesh.ringCenter[i] = Vector(cos((2*PI*i)/PKBOLT_SEGMENTS),sin((2*PI*i)/PKBOLT_SEGMENTS),0);
				pkBoltMesh.upperRing[i] = Vector(cos((2*PI*(i+0.5))/PKBOLT_SEGMENTS)*0.7,sin((2*PI*(i+0.5))/PKBOLT_SEGMENTS)*0.7,0.2);
			}
			pkBoltMesh.ringTexID = TMANAGER->CreateTextureFromFile("shockwave_color_green.tga", TEXTURESDIR, DA::TGA,PF_RGB5_A1);
			if (CQRENDERFLAGS.bMultiTexture)
				pkBoltMesh.moveTexID = TMANAGER->CreateTextureFromFile("fractal_tile.tga", TEXTURESDIR, DA::TGA,PF_RGB5_A1);
			else
				pkBoltMesh.moveTexID = 0;
			
			pkBoltMesh.RestoreVertexBuffers();

			if (data->explosionEffect[0])
			{
				objArch->pExplosion = ARCHLIST->LoadArchetype(data->explosionEffect);
				ARCHLIST->AddRef(objArch->pExplosion, OBJREFNAME);
			}

			if (data->engineTrailType[0])
			{
				if ((objArch->pEngineTrailType = ARCHLIST->LoadArchetype(data->engineTrailType)) != 0)
					ARCHLIST->AddRef(objArch->pEngineTrailType, OBJREFNAME);
			}

			ARCHLIST->LoadArchetype(data->newPlanetType);
		
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
BOOL32 PKBoltFactory::DestroyArchetype (HANDLE hArchetype)
{
	OBJTYPE * objtype = (OBJTYPE *)hArchetype;

	delete objtype;

	return 1;
}
//-------------------------------------------------------------------
//
IBaseObject * PKBoltFactory::CreateInstance (HANDLE hArchetype)
{
	OBJTYPE * objtype = (OBJTYPE *)hArchetype;

	PKBolt * pkBolt = new ObjectImpl<PKBolt>;

	pkBolt->init(*objtype);

	return pkBolt;
}
//-------------------------------------------------------------------
//
void PKBoltFactory::EditorCreateInstance (HANDLE hArchetype, const MISSION_INFO & info)
{
	// does not support this!
}
//---------------------------------------------------------------------------
//-------------------------GLOBAL STARTUP------------------------------------
//---------------------------------------------------------------------------
//
struct _pkBolt : GlobalComponent
{
	PKBoltFactory * sfactory;

	virtual void Startup (void)
	{
		sfactory = new DAComponent<PKBoltFactory>;
		AddToGlobalCleanupList((IDAComponent **) &sfactory);
	}

	virtual void Initialize (void)
	{
		sfactory->init();
	}
};

static _pkBolt __pkBolt;
//---------------------------------------------------------------------------
//------------------------End PlanetKillerBolt.cpp----------------------------------------
//---------------------------------------------------------------------------


