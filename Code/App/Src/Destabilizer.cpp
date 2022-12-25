//--------------------------------------------------------------------------//
//                                                                          //
//                             Destabilizer.cpp                          //
//                                                                          //
//                  COPYRIGHT (C) 1997 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*

    $Author: Tmauer $

    $Header: /Conquest/App/Src/Destabilizer.cpp 32    10/20/00 3:27p Tmauer $
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

#define EFFECT_TIME 2.0
#define INNER_RING 1000.0
#define MID_RING ((INNER_RING+(data->explosionRange*GRIDSIZE))/2)
#define MID_INNER_DIFF (MID_RING-INNER_RING)

struct DESTABILIZER_INIT : RenderArch
{
	BT_DESTABILIZER_DATA * pData;
	S32 archIndex;
	IMeshArchetype * meshArch;
	PARCHETYPE pArchetype;
	AnimArchetype * animArch;
	PARCHETYPE pExplosion;
};

#define DESTABILIZER_SEGMENTS 40
#define EFFECT_RATE 3
#define SPARK_SIZE 3500
#define SPARK_SIZE_FINAL 1500
#define SPARK_SPACING 1500
#define NUM_SPARK_RING 16
#define DESTABILIZER_ANM_COUNT 15

struct DestabilizerMesh : IVertexBufferOwner
{
	Vector ringCenter[DESTABILIZER_SEGMENTS];
	Vector upperRing[DESTABILIZER_SEGMENTS];
	Vector sparks1[NUM_SPARK_RING];
	Vector sparks2[NUM_SPARK_RING];

	U32 baseTexID;
	U32 moveTexID;
	U32 vb_handle;

	virtual void RestoreVertexBuffers();

	DestabilizerMesh()
	{
		vb_handle = 0;
		vb_mgr->Add(this);
	}

	~DestabilizerMesh()
	{
		vb_mgr->Delete(this);
	}
	
}destabilizerMesh;

void DestabilizerMesh::RestoreVertexBuffers()
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

		result = PIPE->create_vertex_buffer( D3DFVF_RPVERTEX2, DESTABILIZER_SEGMENTS*2+2, IRP_VBF_SYSTEM, &vb_handle );
		CQASSERT(result == GR_OK);
		result = PIPE->lock_vertex_buffer( vb_handle, DDLOCK_WRITEONLY, (void **)&vb_data, &dwSize );
		CQASSERT(result == GR_OK);
		
		for(int i = 0; i < DESTABILIZER_SEGMENTS; ++i)
		{
			if(i%2)
			{
				vb_data[i*2].u = 0.16666666f;
				vb_data[i*2].v = 1.0f;
			}
			else
			{
				vb_data[i*2].u = 0.5f;
				vb_data[i*2].v = 1.0f;
			}
			vb_data[i*2].u2 = i;
			vb_data[i*2].pos = ringCenter[i];
			
			vb_data[i*2+1].color = 0x00ffffffff;
			if(i%2)
			{
				vb_data[i*2+1].u = 0.0f;
				vb_data[i*2+1].v = 0.5f;
			}
			else
			{
				vb_data[i*2+1].u = 0.33333333f;
				vb_data[i*2+1].v = 0.5f;
			}
			vb_data[i*2+1].u2 = i+0.5f;
			vb_data[i*2+1].pos = upperRing[i];
		}
		
		if(DESTABILIZER_SEGMENTS%2)
		{
			vb_data[DESTABILIZER_SEGMENTS*2].u = 0.16666666f;
			vb_data[DESTABILIZER_SEGMENTS*2].v = 1.0f;
		}
		else
		{
			vb_data[DESTABILIZER_SEGMENTS*2].u = 0.5f;
			vb_data[DESTABILIZER_SEGMENTS*2].v = 1.0f;
		}
		vb_data[DESTABILIZER_SEGMENTS*2].u2 = DESTABILIZER_SEGMENTS;
		vb_data[DESTABILIZER_SEGMENTS*2].pos = ringCenter[0];
		
		vb_data[DESTABILIZER_SEGMENTS*2+1].color = 0x00ffffffff;
		if(DESTABILIZER_SEGMENTS%2)
		{
			vb_data[DESTABILIZER_SEGMENTS*2+1].u = 0.0f;
			vb_data[DESTABILIZER_SEGMENTS*2+1].v = 0.5f;
		}
		else
		{
			vb_data[DESTABILIZER_SEGMENTS*2+1].u = 0.33333333f;
			vb_data[DESTABILIZER_SEGMENTS*2+1].v = 0.5f;
		}
		vb_data[DESTABILIZER_SEGMENTS*2+1].u2 = DESTABILIZER_SEGMENTS+0.5f;
		vb_data[DESTABILIZER_SEGMENTS*2+1].pos = upperRing[0];
		
		
		result = PIPE->unlock_vertex_buffer( vb_handle );
		CQASSERT(result == GR_OK);
	}
	else
		vb_handle = 0;
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//#define MESH_SPARKS 9
//#define MAX_SEGMENTS 35

struct _NO_VTABLE Destabilizer : public ObjectRender<ObjectMission<ObjectTransform<ObjectFrame<IBaseObject,struct DESTABILIZER_SAVELOAD,struct DESTABILIZER_INIT> > > > , ISaveLoad,
										BASE_DESTABILIZER_SAVELOAD,IReconProbe
{
	BEGIN_MAP_INBOUND(Destabilizer)
	_INTERFACE_ENTRY(IBaseObject)
	_INTERFACE_ENTRY(ISaveLoad)
	_INTERFACE_ENTRY(IMissionActor)
	_INTERFACE_ENTRY(IReconProbe)
	END_MAP()

	//------------------------------------------
	OBJPTR<IBaseObject> owner;
	
	const BT_DESTABILIZER_DATA * data;
	DESTABILIZER_INIT *arch;
	HSOUND hSound;

	U32 multiStages;
//	AnimInstance anm[DESTABILIZER_ANM_COUNT];
	OBJPTR<IWeaponTarget> targets[MAX_DESTABILIZER_TARGETS];

	GeneralSyncNode  genSyncNode;

	OBJPTR<IReconLauncher> ownerLauncher;
	OBJPTR<IBaseObject> target;

	U32 objMapSystemID;
	U32 objMapSquare;

	//------------------------------------------

	Destabilizer (void) :
		genSyncNode(this, SyncGetProc(&Destabilizer::getSyncData), SyncPutProc(&Destabilizer::putSyncData))
	{
		bDeleteRequested = true;
		bGone = true;
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

	virtual ~Destabilizer (void);

	virtual BOOL32 Update (void);

	virtual void PhysicalUpdate (SINGLE dt);

	virtual void Render (void);

	virtual U32 GetSystemID (void) const;

	virtual U32 GetPlayerID (void) const
	{
		return MGlobals::GetPlayerFromPartID(ownerID);
	}

	virtual void CastVisibleArea();


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

	void init (DESTABILIZER_INIT *initData);

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
Destabilizer::~Destabilizer (void)
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
U32 Destabilizer::GetSystemID (void) const
{
	return systemID;
}
//----------------------------------------------------------------------------------
//
void Destabilizer::CastVisibleArea()
{
	U32 tID = GetPlayerID();
	if(!tID)
		tID = playerID;
	SetVisibleToPlayer(tID);
}
//----------------------------------------------------------------------------------
//
void Destabilizer::PhysicalUpdate (SINGLE dt)
{
	if(time == -1)
	{
		SINGLE distSq = (targetPos-transform.translation).magnitude_squared();
		SINGLE moveDist = data->maxVelocity*dt;
		if(distSq < moveDist*moveDist)
		{
			TRANSFORM trans;
			trans.translation = transform.translation;
			IBaseObject * obj = CreateBlast(arch->pExplosion,trans, systemID);
			CQASSERT(obj);
			OBJLIST->AddObject(obj);
			time = 0;
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
	else
	{	
		/*if(bVisible)
		{
			Vector centerPos = targetPos;
			for(U32 anmCnt = 0; anmCnt < DESTABILIZER_ANM_COUNT; ++anmCnt)
			{
				TRANSFORM trans;
				if(anmCnt%2)
					trans.rotate_about_k(dt);
				else
					trans.rotate_about_k(-dt);
				if((anmCnt%4 > 1))
					trans.rotate_about_i(dt);
				else
					trans.rotate_about_i(-dt);

				if((anmCnt%8 > 3))
					trans.rotate_about_j(dt);
				else
					trans.rotate_about_j(-dt);

				Vector pos = anm[anmCnt].GetPosition() - centerPos;
				pos = trans*pos+centerPos;
				anm[anmCnt].SetPosition(pos);
				anm[anmCnt].update(dt);
			}
		}*/
		time += dt;
	}
}
//----------------------------------------------------------------------------------
//
BOOL32 Destabilizer::Update (void)
{
	if(numTargets && ((~targetsHeld) & (~(0xFFFFFFFF << numTargets))))
	{
		for(U32 i = 0; i < numTargets; ++ i)
		{
			if(!(targetsHeld & (0x01 << i)))
			{
				if (targets[i] && (!(targets[i].Ptr()->effectFlags.bDestabilizer)) && targets[i].Ptr()->GetSystemID() == systemID)
				{
					targets[i].Ptr()->effectFlags.bDestabilizer = true;
					targetsHeld |= (0x01 << i);
					VOLPTR(IShipMove) ship=targets[i].Ptr();
					if (ship)
					{
						ship->DestabilizeShip(dwMissionID);
						
						ENGINE->set_angular_velocity(targets[i].Ptr()->GetObjectIndex(),Vector((rand()%10)*0.1-0.5,(rand()%10)*0.1-0.5,(rand()%10)*0.2-1.0));
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
	BOOL32 result = 1;
	
	if (time >= 0 && time <= EFFECT_TIME && THEMATRIX->IsMaster() && !bFreeTargets)
	{
		SINGLE range = (time/EFFECT_TIME)*(data->explosionRange);
		ObjMapIterator it(systemID, transform.translation, range * GRIDSIZE);

		IBaseObject * posibleTargets[MAX_DESTABILIZER_TARGETS];
		U32 numPosibleTargets = 0;

		while (it && (numPosibleTargets < MAX_DESTABILIZER_TARGETS))
		{
			if ((it->flags & OM_UNTOUCHABLE) == 0)
			{
				if (it->obj && (!(it->obj->effectFlags.bDestabilizer))) 
				{
					SINGLE distance = (it->obj->GetPosition()-transform.translation).magnitude();
					
					if (distance < range*GRIDSIZE && (!it->obj->effectFlags.bDestabilizer))
					{
						posibleTargets[numPosibleTargets] = it->obj;
						++numPosibleTargets;
					}
				}
			}
			++it;
		}
		for(U32 i = 0; i < numPosibleTargets;++i)
		{
			if(numTargets < MAX_DESTABILIZER_TARGETS)
			{
				VOLPTR(IShipMove) ship=posibleTargets[i];
				if (ship && posibleTargets[i]->QueryInterface(IWeaponTargetID,targets[numTargets],NONSYSVOLATILEPTR))
				{
					posibleTargets[i]->effectFlags.bDestabilizer = true;
					targetIDs[numTargets] = posibleTargets[i]->GetPartID();
					targetsHeld |= (0x01 << numTargets);
				
					ship->DestabilizeShip(dwMissionID);
					
					ENGINE->set_angular_velocity(posibleTargets[i]->GetObjectIndex(),Vector((rand()%10)*0.1-0.5,(rand()%10)*0.1-0.5,(rand()%10)*0.2-1.0));
					++numTargets;
				}
			}
		}
	}
	/*if( time > EFFECT_TIME)
	{
		if(THEMATRIX->IsMaster())
		{
			COMPTR<ITerrainMap> map;
			SECTOR->GetTerrainMap(systemID,map);

			for(U32 count = 0; count < numSquares; ++count)
			{
				map->TestSegment(gvec[count],gvec[count],this);
			}
			}
}*/
	return result;
}

void Destabilizer::softwareRenderRing ()
{
	if(time <=EFFECT_TIME)
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
		
		SetupDiffuseBlend(destabilizerMesh.baseTexID,FALSE);
		
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
			for(int i = 0; i < DESTABILIZER_SEGMENTS; ++i)
			{
				PB.Color4ub(255,255,255,alpha);
				if(i%2)
					PB.TexCoord2f(0.16666666,1.0);
				else
					PB.TexCoord2f(0.5,1.0);
				PB.Vertex3f(destabilizerMesh.ringCenter[i].x,destabilizerMesh.ringCenter[i].y,destabilizerMesh.ringCenter[i].z);
				
				PB.Color4ub(255,255,255,0);
				if(i%2)
					PB.TexCoord2f(0,0.5);
				else
					PB.TexCoord2f(0.33333333,0.5);
				PB.Vertex3f(destabilizerMesh.upperRing[i].x,destabilizerMesh.upperRing[i].y,destabilizerMesh.upperRing[i].z);
			}
			PB.Color4ub(255,255,255,alpha);
			if(DESTABILIZER_SEGMENTS%2)
				PB.TexCoord2f(0.16666666,1.0);
			else
				PB.TexCoord2f(0.5,1.0);
			PB.Vertex3f(destabilizerMesh.ringCenter[0].x,destabilizerMesh.ringCenter[0].y,destabilizerMesh.ringCenter[0].z);
			
			PB.Color4ub(255,255,255,0);
			if(DESTABILIZER_SEGMENTS%2)
				PB.TexCoord2f(0,0.5);
			else
				PB.TexCoord2f(0.33333333,0.5);
			PB.Vertex3f(destabilizerMesh.upperRing[0].x,destabilizerMesh.upperRing[0].y,destabilizerMesh.upperRing[0].z);
			PB.End();
		}
	}
}
//----------------------------------------------------------------------------------
//
void Destabilizer::renderRing()
{
	if(time <=EFFECT_TIME)
	{
		if (CQRENDERFLAGS.bSoftwareRenderer)
		{
			softwareRenderRing();
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
			BATCH->set_texture_stage_texture( 0, destabilizerMesh.baseTexID );
			BATCH->set_texture_stage_texture( 1, destabilizerMesh.moveTexID );

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
			SetupDiffuseBlend(destabilizerMesh.baseTexID,FALSE);
		}
		
		Vertex2 *vb_data;
		U32 dwSize;
		GENRESULT result;
		result = PIPE->lock_vertex_buffer( destabilizerMesh.vb_handle, DDLOCK_WRITEONLY, (void **)&vb_data, &dwSize );
		CQASSERT(result == GR_OK);
		
		int i;
		for(i = 0; i < DESTABILIZER_SEGMENTS; ++i)
		{
			vb_data[i*2].color = (alpha<<24) | 0x00ffffff;
			vb_data[i*2].v2 = i+timeDif;
	
			vb_data[i*2+1].v2 = i+timeDif2;
		}
		vb_data[DESTABILIZER_SEGMENTS*2].color = (alpha<<24) | 0x00ffffff;
		vb_data[DESTABILIZER_SEGMENTS*2].v2 = DESTABILIZER_SEGMENTS+timeDif;
		vb_data[DESTABILIZER_SEGMENTS*2+1].v2 = DESTABILIZER_SEGMENTS+timeDif2;
		
		result = PIPE->unlock_vertex_buffer( destabilizerMesh.vb_handle );
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
			
			result = PIPE->draw_primitive_vb( D3DPT_TRIANGLESTRIP, destabilizerMesh.vb_handle, 0, DESTABILIZER_SEGMENTS*2+2, 0 );
			CQASSERT(result == GR_OK);
		}
	}
}

//----------------------------------------------------------------------------------
//
#if 0
void Destabilizer::renderAnm()
{
	SINGLE timeLeft = (data->duration-time);
	U8 anmAlpha = 255;
	SINGLE anmSize = INNER_RING*2;
	if(timeLeft < 1.0)
	{
		if(timeLeft < 0)
		{
			anmSize = 0;
			anmAlpha = 0;
		}
		else
		{
			anmAlpha = 255 * timeLeft;
			anmSize = INNER_RING*2*timeLeft;
		}
	}
	if(time < 1.0)
	{
		anmSize = INNER_RING*2*time;
	}
	BATCH->set_render_state(D3DRS_ZENABLE,TRUE);
	BATCH->set_render_state(D3DRS_ZWRITEENABLE,FALSE);
	BATCH->set_render_state(D3DRS_ALPHABLENDENABLE,TRUE);
	BATCH->set_render_state(D3DRS_SRCBLEND,D3DBLEND_ONE);
	BATCH->set_render_state(D3DRS_DESTBLEND,D3DBLEND_ONE);

	if(anmSize > 100)
	{
		for(U32 anmCnt = 0; anmCnt < DESTABILIZER_ANM_COUNT; ++anmCnt)
		{
			anm[anmCnt].SetWidth(anmSize);
			anm[anmCnt].SetColor(255,255,255,anmAlpha);
			ANIM2D->render(&(anm[anmCnt]));
		}
	}
}
#endif
//----------------------------------------------------------------------------------
//
void Destabilizer::renderSpark (void)
{
	SINGLE timeLeft = (data->duration-time);
	SINGLE ringODist;
	if(time*0.5 < EFFECT_TIME*2)
	{
		SINGLE dt =((time*0.5)-EFFECT_TIME)/EFFECT_TIME;
		ringODist = MID_RING-(dt*dt*MID_INNER_DIFF)-10000.0;
	}
	else
	{
		SINGLE dt = ((time*0.5)-(2*EFFECT_TIME))/EFFECT_TIME;
		ringODist = INNER_RING-(dt*MID_INNER_DIFF*2)-10000.0;
	}

	SINGLE outerRange;
	if(time < EFFECT_TIME)
	{
		outerRange = data->explosionRange*GRIDSIZE*(time/EFFECT_TIME);
	}
	else
	{
		outerRange = data->explosionRange*GRIDSIZE;
	}
	outerRange = outerRange*outerRange;


	Vector epos = targetPos;		
	
	Vector cpos (CAMERA->GetPosition());
	
	Vector look (epos - cpos);
	look.z *= 4;
	
	Vector k = look.normalize();

	Vector tmpUp(0,0,1);

	Vector j (cross_product(k,tmpUp));
	j.normalize();

	Vector i (cross_product(j,k));

	i.normalize();

	TRANSFORM baseTrans;
	baseTrans.set_i(i);
	baseTrans.set_j(j);
	baseTrans.set_k(k);

	SINGLE maxDist = data->explosionRange*GRIDSIZE;
	maxDist = maxDist*maxDist;

	BATCH->set_state(RPR_BATCH,TRUE);
	SetupDiffuseBlend(destabilizerMesh.baseTexID,FALSE);
	BATCH->set_render_state(D3DRS_ZENABLE,TRUE);
	BATCH->set_render_state(D3DRS_ZWRITEENABLE,FALSE);
	BATCH->set_render_state(D3DRS_ALPHABLENDENABLE,TRUE);
	BATCH->set_render_state(D3DRS_SRCBLEND,D3DBLEND_SRCALPHA);
	BATCH->set_render_state(D3DRS_DESTBLEND,D3DBLEND_INVSRCALPHA);

	TRANSFORM camTrans;
	camTrans.translation = transform.translation;
	CAMERA->SetModelView(&camTrans);
		
	BATCH->set_state(RPR_STATE_ID,destabilizerMesh.baseTexID);

	PB.Begin(PB_QUADS);

	U32 currentRow;
	if(ringODist < INNER_RING)
	{
		currentRow = ((-(ringODist-INNER_RING))/SPARK_SPACING)+1;
	}
	else
	{
		currentRow = 0;
	}
	SINGLE rowDist = currentRow*SPARK_SPACING+ringODist;

	while(rowDist*rowDist < outerRange)
	{
		SINGLE factor = (rowDist*rowDist)/outerRange;

		SINGLE size =(SPARK_SIZE-SPARK_SIZE_FINAL)*factor + SPARK_SIZE_FINAL;

		U32 alpha;

		if(factor > 0.2)
		{
			alpha = 128*((1-factor)*1.25); 
		}else if(factor < 0.1)
		{
			factor = __max(factor -0.05,0.0);
			alpha = 128*(factor*20);
		}else
		{
			alpha = 128;
		}

		if(timeLeft< 1.0)
		{
			if(timeLeft < 0)
				alpha = 0;
			else
				alpha = alpha*timeLeft;
		}
		PB.Color4ub(255,255,255,alpha);
		for(U32 spCount = 0; spCount < NUM_SPARK_RING; ++spCount)
		{
			TRANSFORM trans = baseTrans;
			trans.rotate_about_k(((currentRow*8)%21)*((spCount*100)%71));

			Vector realI = trans.get_i()*size;
			Vector realJ = trans.get_j()*size;

			Vector v[4];

			Vector base; 
			if(currentRow%2)
			{
				base = destabilizerMesh.sparks1[spCount]*rowDist;
			}
			else
			{
				base = destabilizerMesh.sparks2[spCount]*rowDist;
			}
			v[0] = base+realI+realJ;
			v[1] = base-realI+realJ;
			v[2] = base-realI-realJ;
			v[3] = base+realI-realJ;
			
			SINGLE addTexC = 0.5*(spCount%2);
			PB.TexCoord2f(0+addTexC,0);   PB.Vertex3f(v[0].x,v[0].y,v[0].z);
			PB.TexCoord2f(0.5+addTexC,0);   PB.Vertex3f(v[1].x,v[1].y,v[1].z);
			PB.TexCoord2f(0.5+addTexC,0.5);   PB.Vertex3f(v[2].x,v[2].y,v[2].z);
			PB.TexCoord2f(0+addTexC,0.5);   PB.Vertex3f(v[3].x,v[3].y,v[3].z);
		}

		rowDist += SPARK_SPACING;
		++currentRow;
	}

	PB.End();

	BATCH->set_state(RPR_STATE_ID,0);

}
//----------------------------------------------------------------------------------
//
void Destabilizer::Render (void)
{
	if (bVisible)
	{
		if(time == -1)
		{
//			ILight * lights[8];
			LIGHT->deactivate_all_lights();
//			U32 numLights = LIGHT->get_best_lights(lights,8, GetTransform().translation,4000);
//			LIGHT->activate_lights(lights,numLights);
			LIGHTS->ActivateBestLights(transform.translation,8,4000);

			TreeRender(mc);
			//ENGINE->render_instance(MAINCAM, instanceIndex,0,LODPERCENT,0,NULL);
		}
		else
		{
		//	renderSpark();

			renderRing();

		//	renderAnm();


			BATCH->set_state(RPR_BATCH,TRUE);
			CAMERA->SetModelView();
			
			BATCH->set_render_state(D3DRS_ALPHABLENDENABLE,TRUE);
			BATCH->set_render_state(D3DRS_SRCBLEND,D3DBLEND_SRCALPHA);
			BATCH->set_render_state(D3DRS_DESTBLEND,D3DBLEND_INVSRCALPHA);
			BATCH->set_render_state(D3DRS_ZENABLE,TRUE);
			BATCH->set_render_state(D3DRS_ZWRITEENABLE,FALSE);
			U8 alpha = 200;
			SINGLE timeLeft = (data->duration-time);
			if(timeLeft< 1.0)
			{
				if(timeLeft < 0)
					alpha = 0;
				else
					alpha = alpha*timeLeft;
			}
		}
	}
}
//----------------------------------------------------------------------------------
//
void Destabilizer::InitRecon(IReconLauncher * _ownerLauncher, U32 _dwMissionID)
{
	_ownerLauncher->QueryInterface(IReconLauncherID,ownerLauncher,NONSYSVOLATILEPTR);
	dwMissionID = _dwMissionID;
	
	playerID = MGlobals::GetPlayerFromPartID(dwMissionID);
	sprintf(partName,"Destabilizer 0x%x",dwMissionID);

	OBJLIST->AddPartID(this, dwMissionID);
	OBJLIST->AddObject(this);

	bGone = true;
	SetReady(false);
	lastSent = 0;
	numTargets = 0;
}
//----------------------------------------------------------------------------------
//
void Destabilizer::ResolveRecon(IBaseObject * _ownerLauncher)
{
	_ownerLauncher->QueryInterface(IReconLauncherID,ownerLauncher,NONSYSVOLATILEPTR);
}
//----------------------------------------------------------------------------------
//
void Destabilizer::LaunchProbe (IBaseObject * _owner, const class TRANSFORM & orientation, const class Vector * pos,
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
//	numSquares = 0;

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

	hSound = SFXMANAGER->Open(data->launchSfx);
	SFXMANAGER->Play(hSound,systemID,pos);

	Vector baseVect = targetPos;
	baseVect.x -= data->explosionRange*GRIDSIZE;
	baseVect.y -= data->explosionRange*GRIDSIZE;
/*	Vector testVect = baseVect;
	while(testVect.x < baseVect.x+(data->explosionRange*GRIDSIZE*2))
	{
		Vector testYVect = testVect;
		while(testYVect.y < baseVect.y+(data->explosionRange*GRIDSIZE*2))
		{
			GRIDVECTOR gridPos;
			gridPos = testYVect;
			if(gridPos-targetPos < data->explosionRange)
			{
				CQASSERT(numSquares < MAX_DESTABILIZER_SQUARES);
				gvec[numSquares] = gridPos;
				++numSquares;
			}
			testYVect.y += GRIDSIZE;
		}
		testVect.x += GRIDSIZE;
	}*/

/*	Vector anmPos = targetPos;
	for(U32 anmCnt = 0; anmCnt < DESTABILIZER_ANM_COUNT; ++anmCnt)
	{
		Vector pos(anmPos.x+(((rand()%2000)/1000.0)-1.0)*INNER_RING,
			anmPos.y+(((rand()%2000)/1000.0)-1.0)*INNER_RING,
			anmPos.z+(((rand()%2000)/1000.0)-1.0)*INNER_RING);
		anm[anmCnt].SetPosition(pos);
		anm[anmCnt].Randomize();
	}*/

	bGone = false;
	bDeleteRequested = false;

	objMapSystemID = systemID;
	objMapSquare = OBJMAP->GetMapSquare(systemID,transform.translation);
	OBJMAP->AddObjectToMap(this,objMapSystemID,objMapSquare);
}
//----------------------------------------------------------------------------------
//
void Destabilizer::ExplodeProbe()
{
	if(numTargets)
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
void Destabilizer::DeleteProbe()
{
	bLauncherDelete = true;
}
//----------------------------------------------------------------------------------
//
bool Destabilizer::IsActive()
{
	return !bGone;
}
//---------------------------------------------------------------------------
//
BOOL32 Destabilizer::Save (struct IFileSystem * inFile)
{
	DAFILEDESC fdesc = "DESTABILIZER_SAVELOAD";
	COMPTR<IFileSystem> file;
	BOOL32 result = 0;
	DWORD dwWritten;
	DESTABILIZER_SAVELOAD save;

	fdesc.lpImplementation = "DOS";
	fdesc.dwDesiredAccess = GENERIC_READ|GENERIC_WRITE;
	fdesc.dwShareMode = 0;  // no sharing
	fdesc.dwCreationDistribution = CREATE_ALWAYS;

	if (inFile->CreateInstance(&fdesc, file) != GR_OK)
		goto Done;

	memset(&save, 0, sizeof(save));

	memcpy(&save, static_cast<BASE_DESTABILIZER_SAVELOAD *>(this), sizeof(BASE_DESTABILIZER_SAVELOAD));

	FRAME_save(save);

	file->WriteFile(0, &save, sizeof(save), &dwWritten, 0);

	result = 1;

Done:	
	return result;
}
//---------------------------------------------------------------------------
//
BOOL32 Destabilizer::Load (struct IFileSystem * inFile)
{	
	DAFILEDESC fdesc = "DESTABILIZER_SAVELOAD";
	COMPTR<IFileSystem> file;
	BOOL32 result = 0;
	DWORD dwRead;
	DESTABILIZER_SAVELOAD load;
	U8 buffer[1024];
//	U32 anmCnt;
//	Vector anmPos;
	
	fdesc.lpImplementation = "DOS";
	if (inFile->CreateInstance(&fdesc, file) != GR_OK)
		goto Done;

	if (file->ReadFile(0, buffer, sizeof(buffer), &dwRead, 0) == 0)
		goto Done;
	MISSION->CorrelateSymbol("DESTABILIZER_SAVELOAD", buffer, &load);

	FRAME_load(load);

	*static_cast<BASE_DESTABILIZER_SAVELOAD *>(this) = load;

/*	anmPos = targetPos;

	for(anmCnt = 0; anmCnt < DESTABILIZER_ANM_COUNT; ++anmCnt)
	{
		Vector pos(anmPos.x+(((rand()%2000)/1000.0)-1.0)*INNER_RING,
			anmPos.y+(((rand()%2000)/1000.0)-1.0)*INNER_RING,
			anmPos.z+(((rand()%2000)/1000.0)-1.0)*INNER_RING);
		anm[anmCnt].SetPosition(pos);
		anm[anmCnt].Randomize();
	}*/

	result = 1;

Done:	
	return result;
}
//---------------------------------------------------------------------------
//
void Destabilizer::ResolveAssociations()
{
	U32 i;
	OBJLIST->FindObject(ownerID, NONSYSVOLATILEPTR, owner, IBaseObjectID);

	for(i = 0; i < numTargets; ++i)
	{
		IBaseObject * obj = OBJLIST->FindObject(targetIDs[i]);

		if(obj)
		{
			obj->QueryInterface(IWeaponTargetID, targets[i], NONSYSVOLATILEPTR);
		}
	}
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
U32 Destabilizer::getSyncData (void * buffer)
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
					if(targets[i])
					{
						targets[i].Ptr()->effectFlags.bDestabilizer=false;
						VOLPTR(IShipMove) ship=targets[i].Ptr();
						ship->ReleaseShipControl(dwMissionID);
					}
				}
			}
			lastSent = numTargets = 0;
		}
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
					if(targets[i])
					{
						targets[i].Ptr()->effectFlags.bDestabilizer=false;
						VOLPTR(IShipMove) ship=targets[i].Ptr();
						ship->ReleaseShipControl(dwMissionID);
					}
				}
			}
			lastSent = numTargets = 0;
		}
		bLauncherDelete = false;
		bNoMoreSync = true;
		OBJLIST->DeferredDestruction(dwMissionID);
		((U8*) buffer)[0] = 1;
		return 1;
	}
	if(bGone || bDeleteRequested)
		return 0;
	if(time > data->duration)
	{
		for(U32 i = 0; i < numTargets; ++i)
		{
			if(targetsHeld & (0x01 << i))
			{
				if(targets[i])
				{
					targets[i].Ptr()->effectFlags.bDestabilizer=false;
					VOLPTR(IShipMove) ship=targets[i].Ptr();
					ship->ReleaseShipControl(dwMissionID);
				}
			}
		}
		bDeleteRequested = true;
		lastSent = numTargets = 0;
		((U8*) buffer)[0] = 0;
		return 1;
	}
	U32 result = 0;
	if(numTargets > lastSent)
	{
		result = sizeof(U32)*(numTargets-lastSent);
		memcpy(buffer,(void *)(&(targetIDs[lastSent])),result);
		lastSent = numTargets;
	}
	return result;
}
//---------------------------------------------------------------------------
//
void Destabilizer::putSyncData (void * buffer, U32 bufferSize, bool bLateDelivery)
{
	if(bufferSize == 1)
	{
		U32 val = ((U8*) buffer)[0];
		if(val == 1)
		{
			if(numTargets)
			{
				for(U32 i = 0; i < numTargets; ++i)
				{
					if(targetsHeld & (0x01 << i))
					{
						if(targets[i])
						{
							targets[i].Ptr()->effectFlags.bDestabilizer=false;
							VOLPTR(IShipMove) ship=targets[i].Ptr();
							ship->ReleaseShipControl(dwMissionID);
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
						if(targets[i])
						{
							targets[i].Ptr()->effectFlags.bDestabilizer=false;
							VOLPTR(IShipMove) ship=targets[i].Ptr();
							ship->ReleaseShipControl(dwMissionID);
						}
					}
				}
				lastSent = numTargets = 0;
			}
			bFreeTargets = false;
		}
		else if(val == 0)
		{
			for(U32 i = 0; i < numTargets; ++i)
			{
				if(targets[i])
				{
					if(targetsHeld & (0x01 << i))
					{
						targets[i].Ptr()->effectFlags.bDestabilizer=false;
						VOLPTR(IShipMove) ship=targets[i].Ptr();
						ship->ReleaseShipControl(dwMissionID);
					}
				}
			}
			bDeleteRequested = true;
			lastSent = numTargets = 0;
		}
	}
	else
	{
		U32 * buf = (U32 *) buffer;
		U32 newTarg = bufferSize/sizeof(U32);
		CQASSERT(newTarg + numTargets <= MAX_DESTABILIZER_TARGETS);
		for(U32 count = 0; count < newTarg; ++count)
		{
			targetIDs[numTargets] = buf[count];

			IBaseObject * obj = OBJLIST->FindObject(buf[count]);

			if (obj)
			{
				obj->QueryInterface(IWeaponTargetID,targets[numTargets],NONSYSVOLATILEPTR);
				if((!obj->effectFlags.bDestabilizer) && obj->GetSystemID() == systemID)
				{
					obj->effectFlags.bDestabilizer = true;
					VOLPTR(IShipMove) ship=obj;
					ship->DestabilizeShip(dwMissionID);

					ENGINE->set_angular_velocity(obj->GetObjectIndex(),Vector((rand()%10)*0.1-0.5,(rand()%10)*0.1-0.5,(rand()%10)*0.2-1.0));
					targetsHeld |= (0x01 << (numTargets));
				}
				else
				{
					targetsHeld &= (~(0x01 << (numTargets)));
				}
			}
			++numTargets;
		}
		lastSent = numTargets;
	}
}
//---------------------------------------------------------------------------
//
void Destabilizer::init (DESTABILIZER_INIT *initData)
{
	FRAME_init(*initData);
	data = initData->pData;
	arch = initData;

	CQASSERT(data);
	CQASSERT(data->wpnClass == WPN_DESTABILIZER);
	CQASSERT(data->objClass == OC_WEAPON);

/*	for(U32 anmCnt = 0; anmCnt < DESTABILIZER_ANM_COUNT; ++anmCnt)
	{
		anm[anmCnt].Init(initData.animArch);
		anm[anmCnt].delay = 0;
		anm[anmCnt].SetRotation(((rand()%1000)/1000.0) * 2*PI);
		anm[anmCnt].SetWidth(INNER_RING*2);
		anm[anmCnt].loop = TRUE;
	}*/

	pArchetype = initData->pArchetype;
	objClass = OC_WEAPON;
	bNoMoreSync = false;
}
//------------------------------------------------------------------------------------------
//---------------------------Destabilizer Factory----------------------------------------------
//------------------------------------------------------------------------------------------
//
struct DACOM_NO_VTABLE DestabilizerFactory : public IObjectFactory
{
	struct OBJTYPE : DESTABILIZER_INIT
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
			if(animArch)
				delete animArch;
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

	BEGIN_DACOM_MAP_INBOUND(DestabilizerFactory)
	DACOM_INTERFACE_ENTRY(IObjectFactory)
	END_DACOM_MAP()

	DestabilizerFactory (void) { }

	~DestabilizerFactory (void);

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

	// DestabilizerFactory methods 

	IDAComponent * getBase (void)
	{
		return static_cast<IObjectFactory *> (this);
	}
};
//--------------------------------------------------------------------------//
//
DestabilizerFactory::~DestabilizerFactory (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (OBJLIST && OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
		connection->Unadvise(factoryHandle);
}
//--------------------------------------------------------------------------//
//
void DestabilizerFactory::init (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
		connection->Advise(getBase(), &factoryHandle);
}
//-----------------------------------------------------------------------------
//
HANDLE DestabilizerFactory::CreateArchetype (const char *szArchname, OBJCLASS objClass, void *_data)
{
	OBJTYPE * objArch = 0;

	if (objClass == OC_WEAPON)
	{
		BT_DESTABILIZER_DATA * data = (BT_DESTABILIZER_DATA *)_data;
		if (data->wpnClass == WPN_DESTABILIZER)
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

			//build the global mesh
			U32 i;
			for(i = 0; i < DESTABILIZER_SEGMENTS; ++i)
			{
				destabilizerMesh.ringCenter[i] = Vector(cos((2*PI*i)/DESTABILIZER_SEGMENTS),sin((2*PI*i)/DESTABILIZER_SEGMENTS),0);
				destabilizerMesh.upperRing[i] = Vector(cos((2*PI*(i+0.5))/DESTABILIZER_SEGMENTS)*0.6,sin((2*PI*(i+0.5))/DESTABILIZER_SEGMENTS)*0.6,0.4);
			}
			
			destabilizerMesh.RestoreVertexBuffers();

			SINGLE offAngle = (2*PI)/(NUM_SPARK_RING*2);
			for(i = 0; i < NUM_SPARK_RING; ++i)
			{
				destabilizerMesh.sparks1[i] = Vector(cos((2*PI*i)/NUM_SPARK_RING),sin((2*PI*i)/NUM_SPARK_RING),0);
				destabilizerMesh.sparks2[i] = Vector(cos((2*PI*i+offAngle)/NUM_SPARK_RING),sin((2*PI*i+offAngle)/NUM_SPARK_RING),0);
			}

			destabilizerMesh.baseTexID = TMANAGER->CreateTextureFromFile("destabilizer.tga", TEXTURESDIR, DA::TGA,PF_4CC_DAA4);
			if (CQRENDERFLAGS.bMultiTexture)
				destabilizerMesh.moveTexID = TMANAGER->CreateTextureFromFile("fractal_tile.tga", TEXTURESDIR, DA::TGA,PF_RGB5_A1);
			else
				destabilizerMesh.moveTexID = 0;

			if (data->explosionEffect[0])
			{
				objArch->pExplosion = ARCHLIST->LoadArchetype(data->explosionEffect);
				ARCHLIST->AddRef(objArch->pExplosion, OBJREFNAME);
			}
		//	objArch->animArch = ANIM2D->create_archetype("dstbexplo.anm");

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
BOOL32 DestabilizerFactory::DestroyArchetype (HANDLE hArchetype)
{
	OBJTYPE * objtype = (OBJTYPE *)hArchetype;

	TMANAGER->ReleaseTextureRef(destabilizerMesh.baseTexID);
	if (destabilizerMesh.moveTexID)
		TMANAGER->ReleaseTextureRef(destabilizerMesh.moveTexID);
	if (destabilizerMesh.vb_handle)
	{
		PIPE->destroy_vertex_buffer(destabilizerMesh.vb_handle);
		destabilizerMesh.vb_handle = 0;
	}
	delete objtype;

	return 1;
}
//-------------------------------------------------------------------
//
IBaseObject * DestabilizerFactory::CreateInstance (HANDLE hArchetype)
{
	OBJTYPE * objtype = (OBJTYPE *)hArchetype;

	Destabilizer * destabilizer = new ObjectImpl<Destabilizer>;

	destabilizer->init(objtype);

	return destabilizer;
}
//-------------------------------------------------------------------
//
void DestabilizerFactory::EditorCreateInstance (HANDLE hArchetype, const MISSION_INFO & info)
{
	// does not support this!
}
//---------------------------------------------------------------------------
//-------------------------GLOBAL STARTUP------------------------------------
//---------------------------------------------------------------------------
//
struct _destabilizer : GlobalComponent
{
	DestabilizerFactory * sfactory;

	virtual void Startup (void)
	{
		sfactory = new DAComponent<DestabilizerFactory>;
		AddToGlobalCleanupList((IDAComponent **) &sfactory);
	}

	virtual void Initialize (void)
	{
		sfactory->init();
	}
};

static _destabilizer __destabilizer;
//---------------------------------------------------------------------------
//------------------------End Destabilizer.cpp----------------------------------------
//---------------------------------------------------------------------------


