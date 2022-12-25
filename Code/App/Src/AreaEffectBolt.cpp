//--------------------------------------------------------------------------//
//                                                                          //
//                             AreaEffectBlolt.cpp                          //
//                                                                          //
//                  COPYRIGHT (C) 1997 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*

    $Author: Jasony $

    $Header: /Conquest/App/Src/AreaEffectBolt.cpp 44    10/13/00 12:04p Jasony $
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

struct AEBOLT_INIT : RenderArch
{
	S32 archIndex;
	IMeshArchetype * meshArch;
	PARCHETYPE pArchetype;
	PARCHETYPE pExplosion;
};

#define AEBOLT_SEGMENTS 20

struct AEBoltMesh : IVertexBufferOwner
{
	Vector ringCenter[AEBOLT_SEGMENTS];
	Vector upperRing[AEBOLT_SEGMENTS];
	U32 ringTexID;
	U32 moveTexID;
	U32 vb_handle;
	
	virtual void RestoreVertexBuffers();

	AEBoltMesh()
	{
		vb_handle = 0;
		vb_mgr->Add(this);
	}

	~AEBoltMesh()
	{
		vb_mgr->Delete(this);
	}

}aeBoltMesh;

void AEBoltMesh::RestoreVertexBuffers()
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
		result = PIPE->create_vertex_buffer( D3DFVF_RPVERTEX2, AEBOLT_SEGMENTS*2+2, IRP_VBF_SYSTEM, &aeBoltMesh.vb_handle );
		CQASSERT(result == GR_OK);
		result = PIPE->lock_vertex_buffer( aeBoltMesh.vb_handle, DDLOCK_WRITEONLY, (void **)&vb_data, &dwSize );
		CQASSERT(result == GR_OK);
		
		for(int i = 0; i < AEBOLT_SEGMENTS; ++i)
		{
			vb_data[i*2].u2 = i;
			vb_data[i*2].u = i+1;
			vb_data[i*2].v = 1;
			vb_data[i*2].pos = aeBoltMesh.ringCenter[i];
			
			vb_data[i*2+1].color = 0x00ffffff;
			vb_data[i*2+1].u2 = i+0.5f;
			vb_data[i*2+1].u = i+0.5f;
			vb_data[i*2+1].v = 0;
			vb_data[i*2+1].pos = aeBoltMesh.upperRing[i];
		}
		
		vb_data[AEBOLT_SEGMENTS*2].u = AEBOLT_SEGMENTS+1;
		vb_data[AEBOLT_SEGMENTS*2].v = 1;
		vb_data[AEBOLT_SEGMENTS*2].u2 = AEBOLT_SEGMENTS;
		vb_data[AEBOLT_SEGMENTS*2].pos = aeBoltMesh.ringCenter[0];
		
		vb_data[AEBOLT_SEGMENTS*2+1].color = 0x00ffffff;
		vb_data[AEBOLT_SEGMENTS*2+1].u = AEBOLT_SEGMENTS+0.5f;
		vb_data[AEBOLT_SEGMENTS*2+1].v = 0;
		vb_data[AEBOLT_SEGMENTS*2+1].u2 = AEBOLT_SEGMENTS+0.5f;
		vb_data[AEBOLT_SEGMENTS*2+1].pos = aeBoltMesh.upperRing[0];
		
		result = PIPE->unlock_vertex_buffer( aeBoltMesh.vb_handle );
		CQASSERT(result == GR_OK);
	}
	else
		aeBoltMesh.vb_handle = 0;
}

struct _NO_VTABLE AEBolt : public ObjectRender<ObjectTransform<ObjectFrame<IBaseObject,struct AEBOLT_SAVELOAD,struct AEBOLT_INIT> > >, IAOEWeapon, ISaveLoad
{
	BEGIN_MAP_INBOUND(AEBolt)
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
	
	const BT_AEBOLT_DATA * data;
	HSOUND hSound;

	Vector targetPos;

	U32 targUnitID[MAX_AOE_VICTIMS];
	U32 distances[MAX_AOE_VICTIMS];
	U32 numFound;
	SINGLE time,lastTime;

	U32 	multiStages;
	PARCHETYPE pExplosion;

	//------------------------------------------

	AEBolt (void) 
	{
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

	virtual ~AEBolt (void);

	virtual BOOL32 Update (void);

	virtual void PhysicalUpdate (SINGLE dt);

	virtual void SoftwareRender (void);

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

	void init (AEBOLT_INIT &initData);
	
};

//----------------------------------------------------------------------------------
//
AEBolt::~AEBolt (void)
{
	if (hSound)
		SFXMANAGER->CloseHandle(hSound);
}
//----------------------------------------------------------------------------------
//
U32 AEBolt::GetSystemID (void) const
{
	return systemID;
}
//----------------------------------------------------------------------------------
//
void AEBolt::CastVisibleArea()
{
	SetVisibleToAllies(GetVisibilityFlags());
}
//----------------------------------------------------------------------------------
//
void AEBolt::PhysicalUpdate (SINGLE dt)
{
	if(time == -1)
	{
		SINGLE distSq = (targetPos-transform.translation).magnitude_squared();
		SINGLE moveDist = data->maxVelocity*dt;
		if(distSq < moveDist*moveDist)
		{
			time = 0;
			if(target)
			{
				IBaseObject * otherTarget = SECTOR->GetJumpgateDestination(target);
				systemID = otherTarget->GetSystemID();
				transform = otherTarget->GetTransform();
				targetPos = otherTarget->GetGridPosition();
			}
			if (bVisible)
			{
				IBaseObject * obj = CreateBlast(pExplosion,transform, systemID);
				CQASSERT(obj);
				OBJLIST->AddObject(obj);
			}
		}
		else
		{
			transform.translation = transform.translation + ((targetPos-transform.translation).normalize())*moveDist;
		}
	}
	else
	{
		time += dt;
	}
}
//----------------------------------------------------------------------------------
//
BOOL32 AEBolt::Update (void)
{
	BOOL32 result = 1;
	if( time != -1)
	{
		SINGLE start = (lastTime/EFFECT_TIME)*data->explosionRange;
		start = start*start;
		SINGLE stop = (time/EFFECT_TIME)*data->explosionRange;
		stop = stop * stop;
		OBJPTR<IWeaponTarget> target;
		for(U32 i = 0; i < numFound; ++i)
		{
			if((distances[i] >= start) && (distances[i] < stop))
			{
				distances[i] = 0;
				if(targUnitID[i])
				{
					IBaseObject *obj = OBJLIST->FindObject(targUnitID[i]);
					targUnitID[i] = 0;
					if (obj && obj->QueryInterface(IWeaponTargetID,target) != 0)
					{
						if (target)
						{
							SINGLE t = 1.0 - (distances[i]/(data->explosionRange));
							SINGLE t2 = 1.0;
							MPart part(target.Ptr());
							if(part.pInit->armorData.myArmor == NO_ARMOR)
								t2 = data->armorData.damageTable.noArmor+1.0;
							else if(part.pInit->armorData.myArmor == LIGHT_ARMOR)
								t2 = data->armorData.damageTable.lightArmor+1.0;
							else if(part.pInit->armorData.myArmor == MEDIUM_ARMOR)
								t2 = data->armorData.damageTable.mediumArmor+1.0;
							else if(part.pInit->armorData.myArmor == HEAVY_ARMOR)
								t2 = data->armorData.damageTable.heavyArmor+1.0;
							target->ApplyAOEDamage(ownerID,data->damage*t*t2);
						}
					}
				}

			}
		}
		if(time > EFFECT_TIME)
			result = 0;
	}
	return result;
}

void AEBolt::SoftwareRender (void)
{
	TRANSFORM trans;
	trans.set_identity();
	SINGLE scale = (time/EFFECT_TIME)*(data->explosionRange*GRIDSIZE)+1;
	trans.scale(scale);
	trans.set_position(targetPos);
	
	
	U32 alpha;
	SINGLE t = time/EFFECT_TIME;
	if(time < EFFECT_TIME)
		alpha = ((1.0 - (t*t))) *255;
	else
		alpha = 0;
	
	BATCH->set_state(RPR_BATCH,false);
	CAMERA->SetModelView(&trans);
	BATCH->set_render_state(D3DRS_ZENABLE,TRUE);
	BATCH->set_render_state(D3DRS_ZWRITEENABLE,FALSE);
	BATCH->set_render_state(D3DRS_ALPHABLENDENABLE,TRUE);
	BATCH->set_render_state(D3DRS_SRCBLEND,D3DBLEND_SRCALPHA);
	BATCH->set_render_state(D3DRS_DESTBLEND,D3DBLEND_ONE);//INVSRCALPHA);
	BATCH->set_render_state(D3DRS_DITHERENABLE,TRUE);
	
	SetupDiffuseBlend(aeBoltMesh.ringTexID,FALSE);
	
	for (int pass=0;pass<2;pass++)
	{
		if (pass)
		{
			trans.d[0][2] *= -1;
			trans.d[1][2] *= -1;
			trans.d[2][2] *= -1;
			CAMERA->SetModelView(&trans);
		}
		
		PB.Begin(PB_TRIANGLE_STRIP);
		int i;
		
		for(i = 0; i < AEBOLT_SEGMENTS; ++i)
		{
			PB.Color4ub(255,255,255,alpha);
			PB.TexCoord2f(i+1,1);
			PB.Vertex3f(aeBoltMesh.ringCenter[i].x,aeBoltMesh.ringCenter[i].y,aeBoltMesh.ringCenter[i].z);
			
			PB.Color4ub(255,255,255,0);
			PB.TexCoord2f(i+0.5,0);
			PB.Vertex3f(aeBoltMesh.upperRing[i].x,aeBoltMesh.upperRing[i].y,aeBoltMesh.upperRing[i].z);
		}
		PB.Color4ub(255,255,255,alpha);
		PB.TexCoord2f(AEBOLT_SEGMENTS+1,1);
		PB.Vertex3f(aeBoltMesh.ringCenter[0].x,aeBoltMesh.ringCenter[0].y,aeBoltMesh.ringCenter[0].z);
		
		PB.Color4ub(255,255,255,0);
		PB.TexCoord2f(AEBOLT_SEGMENTS+0.5,0);
		PB.Vertex3f(aeBoltMesh.upperRing[0].x,aeBoltMesh.upperRing[0].y,aeBoltMesh.upperRing[0].z);
		
		PB.End();
	}
}
//----------------------------------------------------------------------------------
//
void AEBolt::Render (void)
{
	if (bVisible)
	{
		if(time == -1)
		{
//			ILight * lights[8];
			LIGHT->deactivate_all_lights();
			//U32 numLights = LIGHT->get_best_lights(lights,8, GetTransform().translation,4000);
			//LIGHT->activate_lights(lights,numLights);
			LIGHTS->ActivateBestLights(transform.translation,8,4000);

			TreeRender(mc);
		//	ENGINE->render_instance(MAINCAM, instanceIndex,0,LODPERCENT,0,NULL);
		}
		else
		{
			if (CQRENDERFLAGS.bSoftwareRenderer)
			{
				SoftwareRender();
				return;
			}

			TRANSFORM trans;
			trans.set_identity();
			SINGLE scale = (time/EFFECT_TIME)*(data->explosionRange*GRIDSIZE)+1;
			trans.scale(scale);
			trans.set_position(targetPos);

			U32 alpha;
			SINGLE t = time/EFFECT_TIME;
			SINGLE timeDif = (t)*EFFECT_RATE;
			SINGLE timeDif2 = timeDif-1;
			if(time < EFFECT_TIME)
				alpha = ((1.0 - (t*t))) *255;
			else
				alpha = 0;

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
				BATCH->set_texture_stage_texture( 0, aeBoltMesh.ringTexID );
				BATCH->set_texture_stage_texture( 1, aeBoltMesh.moveTexID );
				
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
				SetupDiffuseBlend(aeBoltMesh.ringTexID,FALSE);
			}

			Vertex2 *vb_data;
			U32 dwSize;
			GENRESULT result;
			result = PIPE->lock_vertex_buffer( aeBoltMesh.vb_handle, DDLOCK_WRITEONLY, (void **)&vb_data, &dwSize );
			CQASSERT(result == GR_OK);

			int i;
			for(i = 0; i < AEBOLT_SEGMENTS; ++i)
			{
				vb_data[i*2].color = alpha<<24 | 0x00ffffff;
				vb_data[i*2].v2 = i+timeDif;
				vb_data[i*2+1].v2 = i+timeDif2;				
			}

			vb_data[AEBOLT_SEGMENTS*2].color = alpha<<24 | 0x00ffffff;
			vb_data[AEBOLT_SEGMENTS*2].v2 = AEBOLT_SEGMENTS+timeDif;
			vb_data[AEBOLT_SEGMENTS*2+1].v2 = AEBOLT_SEGMENTS+timeDif2;

			result = PIPE->unlock_vertex_buffer( aeBoltMesh.vb_handle );
			CQASSERT(result == GR_OK);
			
			for (int pass=0;pass<2;pass++)
			{
				if (pass)
				{
					trans.d[0][2] *= -1;
					trans.d[1][2] *= -1;
					trans.d[2][2] *= -1;
					CAMERA->SetModelView(&trans);
				}

				result = PIPE->draw_primitive_vb( D3DPT_TRIANGLESTRIP, aeBoltMesh.vb_handle, 0, AEBOLT_SEGMENTS*2+2, 0 );
				CQASSERT(result == GR_OK);
			}
		}
	}
}

//----------------------------------------------------------------------------------
//
void AEBolt::InitWeapon (IBaseObject * _owner, const class TRANSFORM & orientation, IBaseObject * _target, U32 flags, const class Vector * pos)
{
	CQASSERT(pos);
	targetPos = *pos;

	if(_target && _target->objClass == OC_JUMPGATE)
	{
		_target->QueryInterface(IBaseObjectID,target,NONSYSVOLATILEPTR);
		targetID = _target->GetPartID();
		targetPos = target->GetPosition();
	}
	else
	{
		target = NULL;
		targetID = 0;
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

	time = -1;
	lastTime = 0;

	hSound = SFXMANAGER->Open(data->launchSfx);
	SFXMANAGER->Play(hSound,systemID,pos);
}
//---------------------------------------------------------------------------
//
U32 AEBolt::GetAffectedUnits (U32 partIDs[MAX_AOE_VICTIMS], U32 damage[MAX_AOE_VICTIMS])
{
	memset(partIDs,0,sizeof(partIDs));
	numFound = 0;
	
	SINGLE range = data->explosionRange;
	U32 sysID;
	GRIDVECTOR myPos;
	if(target)
	{
		IBaseObject * otherTarget = SECTOR->GetJumpgateDestination(target);
		sysID = otherTarget->GetSystemID();
		myPos = otherTarget->GetGridPosition();
	}
	else
	{
		myPos = targetPos;
		sysID = systemID;
	}
	ObjMapIterator it(sysID, myPos, range * GRIDSIZE);

	while (it)
	{
		if (((it->flags & OM_UNTOUCHABLE) == 0) && ((it->flags & OM_TARGETABLE) || (it->obj->objClass == OC_MINEFIELD)) &&
			(!MGlobals::AreAllies(MGlobals::GetPlayerFromPartID(it->dwMissionID),MGlobals::GetPlayerFromPartID(ownerID))))
		{
			U32 visFlags = GetVisibilityFlags();
			visFlags |= it->obj->GetVisibilityFlags();

			SINGLE distance = it->obj->GetGridPosition()-myPos;

			if(owner)
			{
				owner->SetVisibleToAllies(1 << (it->obj->GetPlayerID()-1));
			}
			SetVisibilityFlags(visFlags);

			for(S32 i = 0; i < MAX_AOE_VICTIMS; ++i)
			{
				if(numFound >((U32)i))
				{
					if(distances[i] > distance)
					{
						for(S32 k = numFound; k > i; --k)
						{
							if(k != MAX_AOE_VICTIMS)
							{
								distances[k] = distances[k-1];
								targUnitID[k] = targUnitID[k-1];
							}
							else
							{
								--numFound;
							}
						}
						distances[i] = distance;
						targUnitID[i] = it->obj->GetPartID();
						SetVisibilityFlags(GetVisibilityFlags()|it->obj->GetTrueVisibilityFlags());
						++numFound;
						break;
					}
				}
				else
				{
					distances[i] = distance;
					targUnitID[i] = it->obj->GetPartID();
					++numFound;
					break;
				}
			}
		}
		++it;
	}

	if(numFound)
	{
		memcpy(partIDs,targUnitID,sizeof(U32)*numFound);
		if(damage)
		{
			for(U32 i = 0 ; i < numFound; ++ i)
			{
				SINGLE t = 1.0 - (distances[i]/(data->explosionRange));
				damage[i] = data->damage*t;
			}
		}
	}
	return numFound;
}
//---------------------------------------------------------------------------
//
void AEBolt::SetAffectedUnits (const U32 partIDs[MAX_AOE_VICTIMS], const U32 damage[MAX_AOE_VICTIMS])
{
	numFound = 0;
	for(int i = 0; i < MAX_AOE_VICTIMS; ++i)
	{
		if(partIDs[i])
		{
			++numFound;
			targUnitID[i] = partIDs[i];
			if(damage)
			{
				SINGLE t = damage[i]/data->damage;
				distances[i] = 1.0-t*(data->explosionRange);
			}
			IBaseObject * obj = OBJLIST->FindObject(partIDs[i]);
			if(obj)
			{
				U32 visFlags = GetVisibilityFlags();
				visFlags |= obj->GetVisibilityFlags();
				if(owner)
					owner->SetVisibleToAllies(1 << (obj->GetPlayerID()-1));
				SetVisibilityFlags(visFlags);
			}
		}
		else
			break;
	}
}
//---------------------------------------------------------------------------
//
BOOL32 AEBolt::Save (struct IFileSystem * inFile)
{
	DAFILEDESC fdesc = "AEBOLT_SAVELOAD";
	COMPTR<IFileSystem> file;
	BOOL32 result = 0;
	DWORD dwWritten;
	AEBOLT_SAVELOAD save;

	fdesc.lpImplementation = "DOS";
	fdesc.dwDesiredAccess = GENERIC_READ|GENERIC_WRITE;
	fdesc.dwShareMode = 0;  // no sharing
	fdesc.dwCreationDistribution = CREATE_ALWAYS;

	if (inFile->CreateInstance(&fdesc, file) != GR_OK)
		goto Done;

	memset(&save, 0, sizeof(save));
	
	save.ownerID = ownerID;
	save.time = time;
	save.lastTime = lastTime;
	save.targetPos = targetPos;
	save.numFound = numFound;
	save.systemID = systemID;
	save.targetID = targetID;

	FRAME_save(save);

	file->WriteFile(0, &save, sizeof(save), &dwWritten, 0);

	fdesc.lpFileName = "TARGETS";
	if (inFile->CreateInstance(&fdesc, file) != GR_OK)
		goto Done;

	file->WriteFile(0,targUnitID,sizeof(U32)*numFound,&dwWritten, 0);
	file->WriteFile(0,distances,sizeof(U32)*numFound,&dwWritten, 0);

	result = 1;

Done:	
	return result;
}

BOOL32 AEBolt::Load (struct IFileSystem * inFile)
{	
	DAFILEDESC fdesc = "AEBOLT_SAVELOAD";
	COMPTR<IFileSystem> file;
	BOOL32 result = 0;
	DWORD dwRead;
	AEBOLT_SAVELOAD load;
	U8 buffer[1024];

	fdesc.lpImplementation = "DOS";
	if (inFile->CreateInstance(&fdesc, file) != GR_OK)
		goto Done;

	if (file->ReadFile(0, buffer, sizeof(buffer), &dwRead, 0) == 0)
		goto Done;
	MISSION->CorrelateSymbol("AEBOLT_SAVELOAD", buffer, &load);

	FRAME_load(load);

	ownerID = load.ownerID;
	time = load.time;
	lastTime = load.lastTime;
	targetPos = load.targetPos;
	numFound = load.numFound;
	systemID = load.systemID;
	targetID = load.targetID;

	fdesc.lpFileName = "TARGETS";
	if (inFile->CreateInstance(&fdesc, file) != GR_OK)
		goto Done;

	if (file->ReadFile(0,targUnitID,sizeof(U32)*numFound,&dwRead, 0) == 0)
		goto Done;
	if (file->ReadFile(0,distances,sizeof(U32)*numFound,&dwRead, 0) == 0)
		goto Done;

	result = 1;

Done:	
	return result;
}

void AEBolt::ResolveAssociations()
{
	OBJLIST->FindObject(ownerID,NONSYSVOLATILEPTR,owner,IBaseObjectID);
	if(targetID)
		OBJLIST->FindObject(targetID,NONSYSVOLATILEPTR,target,IBaseObjectID);
}

void AEBolt::init (AEBOLT_INIT &initData)
{
	FRAME_init(initData);
	data = (const BT_AEBOLT_DATA *) ARCHLIST->GetArchetypeData(initData.pArchetype);

	CQASSERT(data);
	CQASSERT(data->wpnClass == WPN_AEBOLT);
	CQASSERT(data->objClass == OC_WEAPON);

	pArchetype = initData.pArchetype;
	objClass = OC_WEAPON;
	pExplosion = initData.pExplosion;
}
//------------------------------------------------------------------------------------------
//---------------------------ArcCannon Factory----------------------------------------------
//------------------------------------------------------------------------------------------
//
struct DACOM_NO_VTABLE AEBoltFactory : public IObjectFactory
{
	struct OBJTYPE : AEBOLT_INIT
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
		}
	};
	//------------------------------------------------------------------------------------------
	//
	U32 factoryHandle;		// handles to callback

	//
	// Interface mapping
	//

	BEGIN_DACOM_MAP_INBOUND(AEBoltFactory)
	DACOM_INTERFACE_ENTRY(IObjectFactory)
	END_DACOM_MAP()

	AEBoltFactory (void) { }

	~AEBoltFactory (void);

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

	// AEBoltFactory methods 

	IDAComponent * getBase (void)
	{
		return static_cast<IObjectFactory *> (this);
	}
};
//--------------------------------------------------------------------------//
//
AEBoltFactory::~AEBoltFactory (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (OBJLIST && OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
		connection->Unadvise(factoryHandle);
}
//--------------------------------------------------------------------------//
//
void AEBoltFactory::init (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
		connection->Advise(getBase(), &factoryHandle);
}
//-----------------------------------------------------------------------------
//
HANDLE AEBoltFactory::CreateArchetype (const char *szArchname, OBJCLASS objClass, void *_data)
{
	OBJTYPE * objArch = 0;

	if (objClass == OC_WEAPON)
	{
		BT_AEBOLT_DATA * data = (BT_AEBOLT_DATA *)_data;
		if (data->wpnClass == WPN_AEBOLT)
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
			for(U32 i = 0; i < AEBOLT_SEGMENTS; ++i)
			{
				aeBoltMesh.ringCenter[i] = Vector(cos((2*PI*i)/AEBOLT_SEGMENTS),sin((2*PI*i)/AEBOLT_SEGMENTS),0);
				aeBoltMesh.upperRing[i] = Vector(cos((2*PI*(i+0.5))/AEBOLT_SEGMENTS)*0.7,sin((2*PI*(i+0.5))/AEBOLT_SEGMENTS)*0.7,0.2);
			}
			aeBoltMesh.ringTexID = TMANAGER->CreateTextureFromFile("shockwave_color.tga", TEXTURESDIR, DA::TGA,PF_RGB5_A1);
			if (CQRENDERFLAGS.bMultiTexture)
				aeBoltMesh.moveTexID = TMANAGER->CreateTextureFromFile("fractal_tile.tga", TEXTURESDIR, DA::TGA,PF_RGB5_A1);
			else
				aeBoltMesh.moveTexID = 0;
			
			aeBoltMesh.RestoreVertexBuffers();
			
			if (data->explosionEffect[0])
			{
				objArch->pExplosion = ARCHLIST->LoadArchetype(data->explosionEffect);
				ARCHLIST->AddRef(objArch->pExplosion, OBJREFNAME);
			}
		
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
BOOL32 AEBoltFactory::DestroyArchetype (HANDLE hArchetype)
{
	OBJTYPE * objtype = (OBJTYPE *)hArchetype;

	TMANAGER->ReleaseTextureRef(aeBoltMesh.ringTexID);
	if (aeBoltMesh.moveTexID)
		TMANAGER->ReleaseTextureRef(aeBoltMesh.moveTexID);
	if (aeBoltMesh.vb_handle)
	{
		PIPE->destroy_vertex_buffer(aeBoltMesh.vb_handle);
		aeBoltMesh.vb_handle = 0;
	}
	delete objtype;

	return 1;
}
//-------------------------------------------------------------------
//
IBaseObject * AEBoltFactory::CreateInstance (HANDLE hArchetype)
{
	OBJTYPE * objtype = (OBJTYPE *)hArchetype;

	AEBolt * aeBolt = new ObjectImpl<AEBolt>;

	aeBolt->init(*objtype);

	return aeBolt;
}
//-------------------------------------------------------------------
//
void AEBoltFactory::EditorCreateInstance (HANDLE hArchetype, const MISSION_INFO & info)
{
	// does not support this!
}
//---------------------------------------------------------------------------
//-------------------------GLOBAL STARTUP------------------------------------
//---------------------------------------------------------------------------
//
struct _aeBolt : GlobalComponent
{
	AEBoltFactory * sfactory;

	virtual void Startup (void)
	{
		sfactory = new DAComponent<AEBoltFactory>;
		AddToGlobalCleanupList((IDAComponent **) &sfactory);
	}

	virtual void Initialize (void)
	{
		sfactory->init();
	}
};

static _aeBolt __aeBolt;
//---------------------------------------------------------------------------
//------------------------End Arc.cpp----------------------------------------
//---------------------------------------------------------------------------


