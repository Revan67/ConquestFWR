//--------------------------------------------------------------------------//
//                                                                          //
//                             RepulsorWave.cpp                          //
//                                                                          //
//                  COPYRIGHT (C) 1997 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*

    $Author: Jasony $

    $Header: /Conquest/App/Src/RepulsorWave.cpp 15    10/26/00 3:33p Jasony $
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
#include "IRecon.h"
#include "ObjMap.h"
#include "IShipMove.h"
#include "ObjMapIterator.h"
#include "MPart.h"
#include "IVertexBuffer.h"
#include <stdio.h>
#include <DEffectOpts.h>

#include <Renderer.h>
#include <TComponent.h>
#include <Engine.h>
#include <Vector.h>
#include <Matrix.h>
#include <IHardpoint.h>
//#include <ITextureLibrary.h>
#include <stdlib.h>
#include <FileSys.h>
#include <ICamera.h>
#include <Pixel.h>
//#include <RPUL\PrimitiveBuilder.h>
#include <IRenderPrimitive.h>

#define EFFECT_RATE 3

struct REPULSORWAVE_INIT
{
	BT_REPULSORWAVE_DATA * pData;
	S32 archIndex;
	IMeshArchetype * meshArch;
	PARCHETYPE pArchetype;
};

#define RWBOLT_SEGMENTS 20

struct RWBoltMesh : IVertexBufferOwner
{
	Vector ringCenter[RWBOLT_SEGMENTS];
	Vector upperRing[RWBOLT_SEGMENTS];
	U32 ringTexID;
	U32 moveTexID;
	U32 vb_handle;
	
	virtual void RestoreVertexBuffers();

	RWBoltMesh()
	{
		vb_handle = 0;
		vb_mgr->Add(this);
	}

	~RWBoltMesh()
	{
		vb_mgr->Delete(this);
	}

}rwBoltMesh;

void RWBoltMesh::RestoreVertexBuffers()
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
		result = PIPE->create_vertex_buffer( D3DFVF_RPVERTEX2, RWBOLT_SEGMENTS*2+2, IRP_VBF_SYSTEM, &rwBoltMesh.vb_handle );
		CQASSERT(result == GR_OK);
		result = PIPE->lock_vertex_buffer( rwBoltMesh.vb_handle, DDLOCK_WRITEONLY, (void **)&vb_data, &dwSize );
		CQASSERT(result == GR_OK);
		
		for(int i = 0; i < RWBOLT_SEGMENTS; ++i)
		{
			vb_data[i*2].u2 = i;
			vb_data[i*2].u = i+1;
			vb_data[i*2].v = 1;
			vb_data[i*2].pos = rwBoltMesh.ringCenter[i];
			
			vb_data[i*2+1].color = 0x00ffffff;
			vb_data[i*2+1].u2 = i+0.5f;
			vb_data[i*2+1].u = i+0.5f;
			vb_data[i*2+1].v = 0;
			vb_data[i*2+1].pos = rwBoltMesh.upperRing[i];
		}
		
		vb_data[RWBOLT_SEGMENTS*2].u = RWBOLT_SEGMENTS+1;
		vb_data[RWBOLT_SEGMENTS*2].v = 1;
		vb_data[RWBOLT_SEGMENTS*2].u2 = RWBOLT_SEGMENTS;
		vb_data[RWBOLT_SEGMENTS*2].pos = rwBoltMesh.ringCenter[0];
		
		vb_data[RWBOLT_SEGMENTS*2+1].color = 0x00ffffff;
		vb_data[RWBOLT_SEGMENTS*2+1].u = RWBOLT_SEGMENTS+0.5f;
		vb_data[RWBOLT_SEGMENTS*2+1].v = 0;
		vb_data[RWBOLT_SEGMENTS*2+1].u2 = RWBOLT_SEGMENTS+0.5f;
		vb_data[RWBOLT_SEGMENTS*2+1].pos = rwBoltMesh.upperRing[0];
		
		result = PIPE->unlock_vertex_buffer( rwBoltMesh.vb_handle );
		CQASSERT(result == GR_OK);
	}
	else
		rwBoltMesh.vb_handle = 0;
}

struct _NO_VTABLE RepulsorWave : public ObjectMission<ObjectTransform<ObjectFrame<IBaseObject,struct REPULSORWAVE_SAVELOAD,struct REPULSORWAVE_INIT> > >, ISaveLoad,
										BASE_REPULSORWAVE_SAVELOAD,IReconProbe
{
	BEGIN_MAP_INBOUND(RepulsorWave)
	_INTERFACE_ENTRY(IBaseObject)
	_INTERFACE_ENTRY(IReconProbe)
	_INTERFACE_ENTRY(ISaveLoad)
	_INTERFACE_ENTRY(IMissionActor)
	END_MAP()

	GeneralSyncNode  genSyncNode;

	//------------------------------------------
	OBJPTR<IBaseObject> owner;
	
	const BT_REPULSORWAVE_DATA * data;
	HSOUND hSound;

	U32 multiStages;
	
	U32 removeTargets[MAX_REPULSORWAVE_TARGETS];
	U8 numToRemove;

	OBJPTR<IReconLauncher> ownerLauncher;
	
	U32 objMapSystemID;
	U32 objMapSquare;

	//------------------------------------------

	RepulsorWave (void) :
		genSyncNode(this, SyncGetProc(&RepulsorWave::getSyncData), SyncPutProc(&RepulsorWave::putSyncData))
	{
		multiStages = 0xFFFFFFFF;
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

	virtual ~RepulsorWave (void);

	virtual BOOL32 Update (void);

	virtual void PhysicalUpdate (SINGLE dt);

	virtual void Render (void);

	void SoftwareRender ();

	virtual U32 GetSystemID (void) const;

	virtual U32 GetPlayerID (void) const
	{
		return MGlobals::GetPlayerFromPartID(ownerID);
	}

	virtual void CastVisibleArea();

	virtual void SetReady(bool _bReady)
	{
		bReady = _bReady;
	}

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

	///////////////////

	void init (REPULSORWAVE_INIT &initData);
	

	U32 getSyncData (void * buffer);
	void putSyncData (void * buffer, U32 bufferSize, bool bLateDelivery);
};

//----------------------------------------------------------------------------------
//
RepulsorWave::~RepulsorWave (void)
{
	if(objMapSystemID)
	{
		OBJMAP->RemoveObjectFromMap(this,objMapSystemID,objMapSquare);
		objMapSystemID = 0;
	}
}
//----------------------------------------------------------------------------------
//
U32 RepulsorWave::GetSystemID (void) const
{
	return systemID;
}
//----------------------------------------------------------------------------------
//
void RepulsorWave::CastVisibleArea()
{
	U32 tID = GetPlayerID();
	if(!tID)
		tID = playerID;
	SetVisibleToPlayer(tID);
}
//----------------------------------------------------------------------------------
//
void RepulsorWave::PhysicalUpdate (SINGLE dt)
{
	if(!bReady)
		return;
	time = time+dt;
}
//----------------------------------------------------------------------------------
//
BOOL32 RepulsorWave::Update (void)
{
	if(numTargets && ((~targetsHeld) & (~(0xFFFFFFFF << numTargets))))
	{
		for(U32 i = 0; i < numTargets; ++ i)
		{
			if(!(targetsHeld & (0x01 << i)))
			{
				IBaseObject * obj = OBJLIST->FindObject(targets[i]);
				if(obj && obj->GetSystemID() == systemID && (!obj->effectFlags.bRepulsorWave))
				{
					OBJPTR<IShipMove> shipTarg;
					if (obj->QueryInterface(IShipMoveID,shipTarg))
					{	
						Vector dir = -(transform.translation-shipTarg.Ptr()->GetPosition());
						if(dir.x == 0 && dir.y == 0)
							dir = Vector(1,0,0);
						dir.normalize();
						Vector position = transform.translation;

						for(U32 j = 0; j < ((data->range+1)*2); ++j)
						{
							position += (dir*GRIDSIZE)/2;
							U32 _x = ((F2LONG(position.x) * 4) + ((GRIDSIZE-1)/2)) / GRIDSIZE;
							U32 _y = ((F2LONG(position.y) * 4) + ((GRIDSIZE-1)/2)) / GRIDSIZE;
		
							if (_x > 255 || _y > 255)
							{
								position -= (dir*GRIDSIZE)/2;
								break;
							}
						}
						CQTRACE42("Ship Grabed - Ship:%x  owner:%x",targets[i],ownerID); 
						shipTarg->PushShipTo(ownerID,position,700);
						CQASSERT(!(obj->effectFlags.bRepulsorWave));
						obj->effectFlags.bRepulsorWave = true;
						targetsHeld |= (0x01 << i);
					}
				}
			}
		}
	}
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
	if(!bReady)
		return 1;
	BOOL32 result = 1;
	if(THEMATRIX->IsMaster() && !bFreeTargets) 
	{
		ObjMapIterator iter(systemID,transform.translation,data->range*GRIDSIZE);

		IBaseObject * posibleTargets[MAX_REPULSORWAVE_TARGETS];
		U32 numPosibleTargets = 0;

		while(iter && numPosibleTargets <MAX_REPULSORWAVE_TARGETS)
		{
			if ((iter->flags & OM_UNTOUCHABLE) == 0 && (iter->flags & OM_TARGETABLE))
			{
				MPart part = iter->obj;
				if(part.isValid() && part->bReady && (!MGlobals::AreAllies(iter->obj->GetPlayerID(),MGlobals::GetPlayerFromPartID(ownerID))) && 
					(!iter->obj->effectFlags.bRepulsorWave) && (GetGridPosition() - iter->obj->GetGridPosition()) < data->range)
				{
					posibleTargets[numPosibleTargets] = iter->obj;
					++numPosibleTargets;

				}
			}
			++iter;
		}	

		U32 i;
		for(i = 0; i < numPosibleTargets;++i)
		{
			if(numTargets < MAX_REPULSORWAVE_TARGETS)
			{
				IBaseObject * obj = posibleTargets[i];
				OBJPTR<IShipMove> shipTarg;
				if (obj->QueryInterface(IShipMoveID,shipTarg) )
				{	
					Vector dir = -(transform.translation-shipTarg.Ptr()->GetPosition());
					if(dir.x == 0 && dir.y == 0)
						dir = Vector(1,0,0);
					dir.normalize();
					Vector position = transform.translation;

					for(U32 j = 0; j < (data->range+1)*2; ++j)
					{
						position += (dir*GRIDSIZE)/2;
						U32 _x = ((F2LONG(position.x) * 4) + ((GRIDSIZE-1)/2)) / GRIDSIZE;
						U32 _y = ((F2LONG(position.y) * 4) + ((GRIDSIZE-1)/2)) / GRIDSIZE;
	
						if (_x > 255 || _y > 255)
						{
							position -= (dir*GRIDSIZE)/2;
							break;
						}
					}
					CQTRACE42("Ship Grabed - Ship:%x  owner:%x",obj->GetPartID(),ownerID); 
					shipTarg->PushShipTo(ownerID,position,700);
					targets[numTargets] = obj->GetPartID();
					CQASSERT(!(obj->effectFlags.bRepulsorWave));
					obj->effectFlags.bRepulsorWave = true;
					targetsHeld |= (0x01 << numTargets);
					++numTargets;
				}
			}
		}

		for(i = 0; i < numTargets; ++i)
		{
			if(numToRemove < MAX_REPULSORWAVE_TARGETS)
			{
				IBaseObject * obj = OBJLIST->FindObject(targets[i]);
				if(!obj)
				{
					removeTargets[numToRemove] = targets[i];
					++numToRemove;
					if(i < lastSent)
						--lastSent;
					--numTargets;
					if(i != numTargets)
					{
						memmove(targets+i,targets+i+1,sizeof(U32)*(numTargets-i));
						U32 upperMask = ((0xFFFFFFFF << (i+1))& targetsHeld) >> 1;
						targetsHeld = (targetsHeld & (~(0xFFFFFFFF<< i))) | upperMask;
					}
					--i;
				}
				else if((obj->GetGridPosition() - GetGridPosition()) >data->range)
				{
					if(targetsHeld & (0x01 << i))
					{
						OBJPTR<IShipMove> shipTarg;
						if (obj && obj->QueryInterface(IShipMoveID,shipTarg))
						{	
							CQASSERT(obj->effectFlags.bRepulsorWave);
							CQTRACE42("Ship Released - Ship:%x  owner:%x",targets[i],ownerID); 
							shipTarg->ReleaseShipControl(ownerID);
							obj->effectFlags.bRepulsorWave = false;
						}
					}
					removeTargets[numToRemove] = obj->GetPartID();
					++numToRemove;
					if(i < lastSent)
						--lastSent;
					--numTargets;
					if(i != numTargets)
					{
						memmove(targets+i,targets+i+1,sizeof(U32)*(numTargets-i));
						U32 upperMask = ((0xFFFFFFFF << (i+1))& targetsHeld) >> 1;
						targetsHeld = (targetsHeld & (~(0xFFFFFFFF<< i))) | upperMask;
					}
					--i;			
				}
			}
		}
	}
	return result;
}
//----------------------------------------------------------------------------------
//
void RepulsorWave::SoftwareRender (void)
{
	SINGLE tempTime;
	if(time < data->ringTime)
		tempTime = time;
	else
		tempTime = time - data->interRingTime * ceil((time-data->ringTime)/data->interRingTime);

	while(tempTime > 0)
	{
		TRANSFORM trans;
		trans.set_identity();
		SINGLE scale = (tempTime/data->ringTime)*(data->range*GRIDSIZE)+1;
		trans.scale(scale);
		trans.set_position(targetPos);
		
		
		U32 alpha;
		SINGLE t = tempTime/data->ringTime;
		if(tempTime < data->ringTime)
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
		
		SetupDiffuseBlend(rwBoltMesh.ringTexID,FALSE);
		
		PB.Begin(PB_TRIANGLE_STRIP);
		int i;
		
		for(i = 0; i < RWBOLT_SEGMENTS; ++i)
		{
			PB.Color4ub(255,255,255,alpha);
			PB.TexCoord2f(i+1,1);
			PB.Vertex3f(rwBoltMesh.ringCenter[i].x,rwBoltMesh.ringCenter[i].y,rwBoltMesh.ringCenter[i].z);
			
			PB.Color4ub(255,255,255,0);
			PB.TexCoord2f(i+0.5,0);
			PB.Vertex3f(rwBoltMesh.upperRing[i].x,rwBoltMesh.upperRing[i].y,rwBoltMesh.upperRing[i].z);
		}
		PB.Color4ub(255,255,255,alpha);
		PB.TexCoord2f(RWBOLT_SEGMENTS+1,1);
		PB.Vertex3f(rwBoltMesh.ringCenter[0].x,rwBoltMesh.ringCenter[0].y,rwBoltMesh.ringCenter[0].z);
		
		PB.Color4ub(255,255,255,0);
		PB.TexCoord2f(RWBOLT_SEGMENTS+0.5,0);
		PB.Vertex3f(rwBoltMesh.upperRing[0].x,rwBoltMesh.upperRing[0].y,rwBoltMesh.upperRing[0].z);
		
		PB.End();
	}
}
//----------------------------------------------------------------------------------
//
void RepulsorWave::Render (void)
{
	if (bVisible && (THEMATRIX->IsMaster() || bHasBeenInit))
	{					
		if (CQRENDERFLAGS.bSoftwareRenderer)
		{
			SoftwareRender();
			return;
		}

		SINGLE tempTime;
		if(time < data->ringTime)
			tempTime = time;
		else
			tempTime = time - data->interRingTime * ceil((time-data->ringTime)/data->interRingTime);

		while(tempTime > 0)
		{
			TRANSFORM trans;
			trans.set_identity();
			SINGLE scale = (tempTime/data->ringTime)*(data->range*GRIDSIZE)+1;
			trans.scale(scale);
			trans.set_position(targetPos);

			U32 alpha;
			SINGLE t = tempTime/data->ringTime;
			SINGLE timeDif = (t)*EFFECT_RATE;
			SINGLE timeDif2 = timeDif-1;
			if(tempTime < data->ringTime)
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
				BATCH->set_texture_stage_texture( 0, rwBoltMesh.ringTexID );
				BATCH->set_texture_stage_texture( 1, rwBoltMesh.moveTexID );
				
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
				SetupDiffuseBlend(rwBoltMesh.ringTexID,FALSE);
			}

			Vertex2 *vb_data;
			U32 dwSize;
			GENRESULT result;
			result = PIPE->lock_vertex_buffer( rwBoltMesh.vb_handle, DDLOCK_WRITEONLY, (void **)&vb_data, &dwSize );
			CQASSERT(result == GR_OK);

			int i;
			for(i = 0; i < RWBOLT_SEGMENTS; ++i)
			{
				vb_data[i*2].color = alpha<<24 | 0x00ffffff;
				vb_data[i*2].v2 = i+timeDif;
				vb_data[i*2+1].v2 = i+timeDif2;				
			}

			vb_data[RWBOLT_SEGMENTS*2].color = alpha<<24 | 0x00ffffff;
			vb_data[RWBOLT_SEGMENTS*2].v2 = RWBOLT_SEGMENTS+timeDif;
			vb_data[RWBOLT_SEGMENTS*2+1].v2 = RWBOLT_SEGMENTS+timeDif2;

			result = PIPE->unlock_vertex_buffer( rwBoltMesh.vb_handle );
			CQASSERT(result == GR_OK);
			
			result = PIPE->draw_primitive_vb( D3DPT_TRIANGLESTRIP, rwBoltMesh.vb_handle, 0, RWBOLT_SEGMENTS*2+2, 0 );
			CQASSERT(result == GR_OK);
			tempTime -= data->interRingTime;
		}
	}
}
//----------------------------------------------------------------------------------
//
void RepulsorWave::InitRecon(IReconLauncher * _ownerLauncher, U32 _dwMissionID)
{
	_ownerLauncher->QueryInterface(IReconLauncherID,ownerLauncher,NONSYSVOLATILEPTR);
	dwMissionID = _dwMissionID;
	
	playerID = MGlobals::GetPlayerFromPartID(dwMissionID);
	sprintf(partName,"RepulsorWave 0x%x",dwMissionID);

	OBJLIST->AddPartID(this, dwMissionID);
	OBJLIST->AddObject(this);

	numTargets = 0;
	lastSent = 0;
	numToRemove = 0;

	bGone = true;
	SetReady(false);
}
//----------------------------------------------------------------------------------
//
void RepulsorWave::ResolveRecon(IBaseObject * _ownerLauncher)
{
	_ownerLauncher->QueryInterface(IReconLauncherID,ownerLauncher, NONSYSVOLATILEPTR);
}
//----------------------------------------------------------------------------------
//
void RepulsorWave::LaunchProbe (IBaseObject * _owner, const class TRANSFORM & orientation, const class Vector * pos,
		U32 targetSystemID, IBaseObject * jumpTarget)
{
	CQASSERT(objMapSystemID == 0);
	if(objMapSystemID)
	{
		OBJMAP->RemoveObjectFromMap(this,objMapSystemID,objMapSquare);
		objMapSystemID = 0;
	}

	CQASSERT(pos);
	targetPos = *pos;

	bDeleteRequested = false;
	bGone = false;

	SetReady(true);

	_owner->QueryInterface(IBaseObjectID, owner, NONSYSVOLATILEPTR);

	systemID = owner->GetSystemID();
	ownerID = owner->GetPartID();
	U32 visFlags = owner->GetVisibilityFlags();

	SetVisibilityFlags(visFlags);

	transform = orientation;

	transform.translation = targetPos;

	time = 0;

	if (hSound==0)
		hSound = SFXMANAGER->Open(data->launchSfx);
	SFXMANAGER->Play(hSound,systemID,pos);

	objMapSystemID = systemID;
	objMapSquare = OBJMAP->GetMapSquare(systemID,transform.translation);
	OBJMAP->AddObjectToMap(this,systemID,objMapSquare);

	bHasBeenInit = false;
}
//----------------------------------------------------------------------------------
//
void RepulsorWave::ExplodeProbe()
{
	if(numTargets || numToRemove)
		bFreeTargets = true;
	else
		bFreeTargets = false;

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
void RepulsorWave::DeleteProbe()
{
	bLauncherDelete = true;
}
//----------------------------------------------------------------------------------
//
bool RepulsorWave::IsActive()
{
	return !bGone;
}
//---------------------------------------------------------------------------
//
BOOL32 RepulsorWave::Save (struct IFileSystem * inFile)
{
	DAFILEDESC fdesc = "REPULSORWAVE_SAVELOAD";
	COMPTR<IFileSystem> file;
	BOOL32 result = 0;
	DWORD dwWritten;
	REPULSORWAVE_SAVELOAD save;

	fdesc.lpImplementation = "DOS";
	fdesc.dwDesiredAccess = GENERIC_READ|GENERIC_WRITE;
	fdesc.dwShareMode = 0;  // no sharing
	fdesc.dwCreationDistribution = CREATE_ALWAYS;

	if (inFile->CreateInstance(&fdesc, file) != GR_OK)
		goto Done;

	memset(&save, 0, sizeof(save));

	memcpy(&save, static_cast<BASE_REPULSORWAVE_SAVELOAD *>(this), sizeof(BASE_REPULSORWAVE_SAVELOAD));

	FRAME_save(save);

	file->WriteFile(0, &save, sizeof(save), &dwWritten, 0);

	result = 1;

Done:	
	return result;
}
//---------------------------------------------------------------------------
//
BOOL32 RepulsorWave::Load (struct IFileSystem * inFile)
{	
	DAFILEDESC fdesc = "REPULSORWAVE_SAVELOAD";
	COMPTR<IFileSystem> file;
	BOOL32 result = 0;
	DWORD dwRead;
	REPULSORWAVE_SAVELOAD load;
	U8 buffer[1024];

	fdesc.lpImplementation = "DOS";
	if (inFile->CreateInstance(&fdesc, file) != GR_OK)
		goto Done;

	if (file->ReadFile(0, buffer, sizeof(buffer), &dwRead, 0) == 0)
		goto Done;
	MISSION->CorrelateSymbol("REPULSORWAVE_SAVELOAD", buffer, &load);

	FRAME_load(load);

	*static_cast<BASE_REPULSORWAVE_SAVELOAD *>(this) = load;

	result = 1;

Done:	
	return result;
}
//---------------------------------------------------------------------------
//
void RepulsorWave::ResolveAssociations()
{
	OBJLIST->FindObject(ownerID, NONSYSVOLATILEPTR, owner);
	
	objMapSystemID = systemID;
	if(systemID)
	{
		objMapSquare = OBJMAP->GetMapSquare(systemID,transform.translation);
		OBJMAP->AddObjectToMap(this,systemID,objMapSquare);
	}
}
//---------------------------------------------------------------------------
//

#define REPULS_INIT 1
#define REPULS_UPDATE 2

//---------------------------------------------------------------------------
//
U32 RepulsorWave::getSyncData (void * buffer)
{
	if(bNoMoreSync)
		return 0;
	if(bFreeTargets)
	{
		if(numTargets)
		{
			for(U32 i = 0; i < numTargets; ++i)
			{
				if(targetsHeld & (0x01 << i))
				{
					IBaseObject * obj = OBJLIST->FindObject(targets[i]);
					OBJPTR<IShipMove> shipTarg;
					if (obj && obj->QueryInterface(IShipMoveID,shipTarg))
					{	
						CQASSERT(obj->effectFlags.bRepulsorWave);
						CQTRACE42("Ship Released - Ship:%x  owner:%x",targets[i],ownerID); 
						shipTarg->ReleaseShipControl(ownerID);
						obj->effectFlags.bRepulsorWave = false;
					}
				}
			}
		}
		numToRemove = lastSent = numTargets = 0;
		bFreeTargets = false;
		((U8*) buffer)[0] = 2;
		return 1;
	}
	if(bLauncherDelete && bDeleteRequested)
	{
		if(numTargets)
		{
			for(U32 i = 0; i < numTargets; ++i)
			{
				if(targetsHeld & (0x01 << i))
				{
					IBaseObject * obj = OBJLIST->FindObject(targets[i]);
					OBJPTR<IShipMove> shipTarg;
					if (obj && obj->QueryInterface(IShipMoveID,shipTarg))
					{	
						CQASSERT(obj->effectFlags.bRepulsorWave);
						CQTRACE42("Ship Released - Ship:%x  owner:%x",targets[i],ownerID); 
						shipTarg->ReleaseShipControl(ownerID);
						obj->effectFlags.bRepulsorWave = false;
					}
				}
			}
			lastSent = numTargets = 0;
		}
		bLauncherDelete = false;
		bNoMoreSync = true;
		numToRemove = 0;
		OBJLIST->DeferredDestruction(dwMissionID);
		((U8*) buffer)[0] = 1;
		return 1;
	}
	if(bGone || bDeleteRequested)
		return 0;
	if(!bHasBeenInit)
	{
		bHasBeenInit = true;
		*((U8 *)buffer) = REPULS_INIT;
		NETGRIDVECTOR * initPos = (NETGRIDVECTOR *)(((U8 *)buffer)+1);
		initPos->init(transform.translation,systemID);
		return sizeof(U8)+sizeof(NETGRIDVECTOR);
	}
	if(time > data->duration)
	{

		for(U32 i = 0; i < numTargets; ++i)
		{
			if(targetsHeld & (0x01 << i))
			{
				IBaseObject * obj = OBJLIST->FindObject(targets[i]);
				OBJPTR<IShipMove> shipTarg;
				if (obj && obj->QueryInterface(IShipMoveID,shipTarg))
				{	
					CQASSERT(obj->effectFlags.bRepulsorWave);
					CQTRACE42("Ship Released - Ship:%x  owner:%x",targets[i],ownerID); 
					shipTarg->ReleaseShipControl(ownerID);
					obj->effectFlags.bRepulsorWave = false;
				}
			}
		}
		bDeleteRequested = true;
		((U8*) buffer)[0] = 0;
		numToRemove = 0;
		lastSent = numTargets = 0;
		return 1;
	}
	if(numTargets == lastSent && !numToRemove)
		return 0;
	*((U8 *)buffer) = REPULS_UPDATE;
	U32 result = 2;
	bool bSend = false;

	*(((U8 *)(buffer))+1) = numToRemove;

	if(numToRemove)
	{
		memcpy((((U8 *)(buffer))+result),removeTargets,numToRemove*sizeof(U32));
		result += sizeof(U32)*(numToRemove);
		numToRemove = 0;
		bSend = true;
	}
	if(numTargets > lastSent)
	{
		memcpy(((U8 *)(buffer))+result,(void *)(&(targets[lastSent])),(numTargets-lastSent)*sizeof(U32));
		result += sizeof(U32)*(numTargets-lastSent);
		lastSent = numTargets;
		bSend = true;
	}

	if(bSend)
		return result;
	return 0;
}
//---------------------------------------------------------------------------
//
void RepulsorWave::putSyncData (void * buffer, U32 bufferSize, bool bLateDelivery)
{
	if(bufferSize == 1)
	{
		U8 val = ((U8*) buffer)[0];
		if(val == 1)
		{
			if(numTargets)
			{
				for(U32 i = 0; i < numTargets; ++i)
				{
					if(targetsHeld & (0x01 << i))
					{
						IBaseObject * obj = OBJLIST->FindObject(targets[i]);
						OBJPTR<IShipMove> shipTarg;
						if (obj && obj->QueryInterface(IShipMoveID,shipTarg))
						{	
							CQASSERT(obj->effectFlags.bRepulsorWave);
							CQTRACE42("Ship Released - Ship:%x  owner:%x",targets[i],ownerID); 
							shipTarg->ReleaseShipControl(ownerID);
							obj->effectFlags.bRepulsorWave = false;
						}
					}
				}
				lastSent = numTargets = 0;
			}
			bLauncherDelete = false;
			bNoMoreSync = true;
			OBJLIST->DeferredDestruction(dwMissionID);
		}
		else if(val == 2)
		{
			if(numTargets)
			{
				for(U32 i = 0; i < numTargets; ++i)
				{
					if(targetsHeld & (0x01 << i))
					{
						IBaseObject * obj = OBJLIST->FindObject(targets[i]);
						OBJPTR<IShipMove> shipTarg;
						if (obj && obj->QueryInterface(IShipMoveID,shipTarg))
						{	
							CQASSERT(obj->effectFlags.bRepulsorWave);
							CQTRACE42("Ship Released - Ship:%x  owner:%x",targets[i],ownerID); 
							shipTarg->ReleaseShipControl(ownerID);
							obj->effectFlags.bRepulsorWave = false;
						}
					}
				}
			}
			numToRemove = lastSent = numTargets = 0;
			bFreeTargets = false;
		}
		else if(val == 0)
		{
			for(U32 i = 0; i < numTargets; ++i)
			{
				if(targetsHeld & (0x01 << i))
				{
					IBaseObject * obj = OBJLIST->FindObject(targets[i]);
					OBJPTR<IShipMove> shipTarg;
					if (obj && obj->QueryInterface(IShipMoveID,shipTarg))
					{	
						CQASSERT(obj->effectFlags.bRepulsorWave);
						CQTRACE42("Ship Released - Ship:%x  owner:%x",targets[i],ownerID); 
						shipTarg->ReleaseShipControl(ownerID);
						obj->effectFlags.bRepulsorWave = false;
					}
				}
			}
			bDeleteRequested = true;
			lastSent = numTargets = 0;
		}
	}
	else
	{
		U8 command = ((U8*)(buffer))[0];
		if(command == REPULS_INIT)
		{
			NETGRIDVECTOR * initPos = (NETGRIDVECTOR *)(((U8 *)buffer)+1);

			if(objMapSystemID)
			{
				OBJMAP->RemoveObjectFromMap(this,objMapSystemID,objMapSquare);
				objMapSystemID = 0;
			}

			targetPos = *initPos;

			systemID = initPos->systemID;

			transform.translation = targetPos;

			objMapSystemID = systemID;
			objMapSquare = OBJMAP->GetMapSquare(systemID,transform.translation);
			OBJMAP->AddObjectToMap(this,systemID,objMapSquare);

			bHasBeenInit = true;
		}
		else if(command == REPULS_UPDATE)
		{
			buffer = (((U8 *)buffer)+1);
			--bufferSize;
			U32 numToGet = *((U8 *)buffer);
			U32 * buf = (U32 *) (((U8 *)buffer)+1);
			if(numToGet)
			{
				for(U32 count = 0; count < numToGet; ++count)
				{
					for(U32 targCount = 0; targCount < numTargets; ++ targCount)
					{
						if(targets[targCount] == buf[count])
						{
							if(targetsHeld & (0x01 << targCount))
							{
								IBaseObject * obj = OBJLIST->FindObject(targets[targCount]);
								OBJPTR<IShipMove> shipTarg;
								if (obj && obj->QueryInterface(IShipMoveID,shipTarg))
								{	
									CQASSERT(obj->effectFlags.bRepulsorWave);
									CQTRACE42("Ship Released - Ship:%x  owner:%x",targets[targCount],ownerID); 
									shipTarg->ReleaseShipControl(ownerID);
									obj->effectFlags.bRepulsorWave = false;
								}
							}
							--lastSent;
							--numTargets;
							if(targCount != numTargets)
							{
								memmove(targets+targCount,targets+targCount+1,sizeof(U32)*(numTargets-targCount));
								U32 upperMask = ((0xFFFFFFFF << (targCount+1))& targetsHeld) >> 1;
								targetsHeld = (targetsHeld & (~(0xFFFFFFFF<< targCount))) | upperMask;
							}
							targCount=numTargets;
						}
					}
				}
			}
			buf = (U32 *) (((U8 *)buffer)+1+sizeof(U32)*numToGet);
			numToGet = (bufferSize -(1+sizeof(U32)*numToGet))/sizeof(U32);
			CQASSERT(numToGet + numTargets <= MAX_REPULSORWAVE_TARGETS);
			for(U32 count = 0; count < numToGet; ++count)
			{
				targets[numTargets] = buf[count];
				++numTargets;

				IBaseObject * obj = OBJLIST->FindObject(buf[count]);
				if (obj)
				{
					if(obj->GetSystemID() == systemID && (!obj->effectFlags.bRepulsorWave))
					{
						OBJPTR<IShipMove> shipTarg;
						if (obj->QueryInterface(IShipMoveID,shipTarg))
						{	
							Vector dir = -(transform.translation-shipTarg.Ptr()->GetPosition());
							if(dir.x == 0 && dir.y == 0)
								dir = Vector(1,0,0);
							dir.normalize();
							Vector position = transform.translation;

							for(U32 j = 0; j < (data->range+1)*2; ++j)
							{
								position += (dir*GRIDSIZE)/2;
								U32 _x = ((F2LONG(position.x) * 4) + ((GRIDSIZE-1)/2)) / GRIDSIZE;
								U32 _y = ((F2LONG(position.y) * 4) + ((GRIDSIZE-1)/2)) / GRIDSIZE;
			
								if (_x > 255 || _y > 255)
								{
									position -= (dir*GRIDSIZE)/2;
									break;
								}
							}
							CQTRACE42("Ship Grabed - Ship:%x  owner:%x",buf[count],ownerID); 
							shipTarg->PushShipTo(ownerID,position,700);
							CQASSERT(!(obj->effectFlags.bRepulsorWave));
							obj->effectFlags.bRepulsorWave = true;
							targetsHeld |= (0x01 << (numTargets-1));
						}
					}
					else
						targetsHeld &= (~(0x01 << (numTargets-1)));
				}
			}
			lastSent = numTargets;
		}
	}
}
//---------------------------------------------------------------------------
//
void RepulsorWave::init (REPULSORWAVE_INIT &initData)
{
	FRAME_init(initData);
	data = initData.pData;

	CQASSERT(data);
	CQASSERT(data->wpnClass == WPN_REPULSORWAVE);
	CQASSERT(data->objClass == OC_WEAPON);

	pArchetype = initData.pArchetype;
	objClass = OC_WEAPON;
	bNoMoreSync = false;
}
//------------------------------------------------------------------------------------------
//---------------------------RepulsorWave Factory----------------------------------------------
//------------------------------------------------------------------------------------------
//
struct DACOM_NO_VTABLE RepulsorWaveFactory : public IObjectFactory
{
	struct OBJTYPE : REPULSORWAVE_INIT
	{
		void * operator new (size_t size)
		{
			return calloc(size,1);
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

	BEGIN_DACOM_MAP_INBOUND(RepulsorWaveFactory)
	DACOM_INTERFACE_ENTRY(IObjectFactory)
	END_DACOM_MAP()

	RepulsorWaveFactory (void) { }

	~RepulsorWaveFactory (void);

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

	// RepulsorWaveFactory methods 

	IDAComponent * getBase (void)
	{
		return static_cast<IObjectFactory *> (this);
	}
};
//--------------------------------------------------------------------------//
//
RepulsorWaveFactory::~RepulsorWaveFactory (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (OBJLIST && OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
		connection->Unadvise(factoryHandle);
}
//--------------------------------------------------------------------------//
//
void RepulsorWaveFactory::init (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
		connection->Advise(getBase(), &factoryHandle);
}
//-----------------------------------------------------------------------------
//
HANDLE RepulsorWaveFactory::CreateArchetype (const char *szArchname, OBJCLASS objClass, void *_data)
{
	OBJTYPE * result = 0;

	if (objClass == OC_WEAPON)
	{
		BT_REPULSORWAVE_DATA * data = (BT_REPULSORWAVE_DATA *)_data;
		if (data->wpnClass == WPN_REPULSORWAVE)
		{
			result = new OBJTYPE;
			
			result->pArchetype = ARCHLIST->GetArchetype(szArchname);
			result->pData = data;
			//
			// force preload of sound effect
			// 
			SFXMANAGER->Preload(data->launchSfx);
			//build the global mesh
			for(U32 i = 0; i < RWBOLT_SEGMENTS; ++i)
			{
				rwBoltMesh.ringCenter[i] = Vector(cos((2*PI*i)/RWBOLT_SEGMENTS),sin((2*PI*i)/RWBOLT_SEGMENTS),0);
				rwBoltMesh.upperRing[i] = Vector(cos((2*PI*(i+0.5))/RWBOLT_SEGMENTS)*0.7,sin((2*PI*(i+0.5))/RWBOLT_SEGMENTS)*0.7,0.1);
			}
			rwBoltMesh.ringTexID = TMANAGER->CreateTextureFromFile("shockwave_rw_color.tga", TEXTURESDIR, DA::TGA,PF_RGB5_A1);
			if (CQRENDERFLAGS.bMultiTexture)
				rwBoltMesh.moveTexID = TMANAGER->CreateTextureFromFile("fractal_tile.tga", TEXTURESDIR, DA::TGA,PF_RGB5_A1);
			else
				rwBoltMesh.moveTexID = 0;
			
			rwBoltMesh.RestoreVertexBuffers();

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
BOOL32 RepulsorWaveFactory::DestroyArchetype (HANDLE hArchetype)
{
	OBJTYPE * objtype = (OBJTYPE *)hArchetype;

	TMANAGER->ReleaseTextureRef(rwBoltMesh.ringTexID);
	if (rwBoltMesh.moveTexID)
		TMANAGER->ReleaseTextureRef(rwBoltMesh.moveTexID);
	if (rwBoltMesh.vb_handle)
	{
		PIPE->destroy_vertex_buffer(rwBoltMesh.vb_handle);
		rwBoltMesh.vb_handle = 0;
	}

	delete objtype;

	return 1;
}
//-------------------------------------------------------------------
//
IBaseObject * RepulsorWaveFactory::CreateInstance (HANDLE hArchetype)
{
	OBJTYPE * objtype = (OBJTYPE *)hArchetype;

	RepulsorWave * repClound = new ObjectImpl<RepulsorWave>;

	repClound->init(*objtype);

	return repClound;
}
//-------------------------------------------------------------------
//
void RepulsorWaveFactory::EditorCreateInstance (HANDLE hArchetype, const MISSION_INFO & info)
{
	// does not support this!
}
//---------------------------------------------------------------------------
//-------------------------GLOBAL STARTUP------------------------------------
//---------------------------------------------------------------------------
//
struct _repelWave : GlobalComponent
{
	RepulsorWaveFactory * sfactory;

	virtual void Startup (void)
	{
		sfactory = new DAComponent<RepulsorWaveFactory>;
		AddToGlobalCleanupList((IDAComponent **) &sfactory);
	}

	virtual void Initialize (void)
	{
		sfactory->init();
	}
};

static _repelWave __repelWave;
//---------------------------------------------------------------------------
//------------------------End RepulsorWave.cpp----------------------------------------
//---------------------------------------------------------------------------


