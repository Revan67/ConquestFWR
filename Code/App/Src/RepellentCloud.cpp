//--------------------------------------------------------------------------//
//                                                                          //
//                             RepellentCloud.cpp                          //
//                                                                          //
//                  COPYRIGHT (C) 1997 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*

    $Author: Tmauer $

    $Header: /Conquest/App/Src/RepellentCloud.cpp 44    10/20/00 4:16p Tmauer $
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

struct REPELLENTCLOUD_INIT
{
	BT_REPELLENTCLOUD_DATA * pData;
	S32 archIndex;
	IMeshArchetype * meshArch;
	PARCHETYPE pArchetype;
};

struct RepelMesh
{
	U32 tex1ID;
	U32 tex2ID;
	PARCHETYPE sprayType;
	PARCHETYPE dammageType;
}repelMesh;

struct _NO_VTABLE RepellentCloud : public ObjectMission<ObjectTransform<ObjectFrame<IBaseObject,struct REPELLENTCLOUD_SAVELOAD,struct REPELLENTCLOUD_INIT> > >, ISaveLoad,
										BASE_REPELLENTCLOUD_SAVELOAD,ITerrainSegCallback,IReconProbe
{
	BEGIN_MAP_INBOUND(RepellentCloud)
	_INTERFACE_ENTRY(IBaseObject)
	_INTERFACE_ENTRY(IReconProbe)
	_INTERFACE_ENTRY(ISaveLoad)
	_INTERFACE_ENTRY(IMissionActor)
	END_MAP()

	GeneralSyncNode  genSyncNode;

	//------------------------------------------
	OBJPTR<IBaseObject> owner;
	
	const BT_REPELLENTCLOUD_DATA * data;
	HSOUND hSound;

	SINGLE angle;
	U32 multiStages;
	
	U8 clAlpha[MAX_REPEL_SQUARES];
	U32 removeTargets[MAX_REPEL_TARGETS];
	U8 numToRemove;

	OBJPTR<IReconLauncher> ownerLauncher;

	U32 objMapSquare[MAX_REPEL_TARGETS];
	U32 objMapSystemID;
	U32 objMapNum;

	//------------------------------------------

	RepellentCloud (void) :
	genSyncNode(this, SyncGetProc(&RepellentCloud::getSyncData), SyncPutProc(&RepellentCloud::putSyncData))
	{
		angle = 0;
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

	virtual ~RepellentCloud (void);

	virtual BOOL32 Update (void);

	virtual void PhysicalUpdate (SINGLE dt);

	virtual void Render (void);

	virtual U32 GetSystemID (void) const;

	virtual U32 GetPlayerID (void) const
	{
		return MGlobals::GetPlayerFromPartID(ownerID);
	}

	virtual void CastVisibleArea();

	bool TerrainCallback (const struct FootprintInfo & info, struct GRIDVECTOR & pos);

	virtual void SetReady(bool _bReady)
	{
		bReady = _bReady;
	}

	virtual struct GRIDVECTOR GetGridPosition (void) const
	{
		GRIDVECTOR tmp;
		tmp = transform.translation + sprayDir*GRIDSIZE*2.5 ;
		return tmp;	
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

	void init (REPELLENTCLOUD_INIT &initData);
	

	U32 getSyncData (void * buffer);
	void putSyncData (void * buffer, U32 bufferSize, bool bLateDelivery);
};

//----------------------------------------------------------------------------------
//
RepellentCloud::~RepellentCloud (void)
{
	for(U32 i = 0; i < objMapNum; ++i)
	{
		OBJMAP->RemoveObjectFromMap(this,objMapSystemID,objMapSquare[i]);
	}
	objMapNum = 0;
}
//----------------------------------------------------------------------------------
//
U32 RepellentCloud::GetSystemID (void) const
{
	return systemID;
}
//----------------------------------------------------------------------------------
//
void RepellentCloud::CastVisibleArea()
{
	U32 tID = GetPlayerID();
	if(!tID)
		tID = playerID;
	SetVisibleToPlayer(tID);
}
//----------------------------------------------------------------------------------
//
void RepellentCloud::PhysicalUpdate (SINGLE dt)
{
	if(!bReady)
		return;
	angle += dt*0.1;
	if(angle > 2*PI)
		angle -= 2*PI;
	time += dt;

	if(time < data->duration-2.0)
	{
		for(U32 i = 0; i < numSquares; ++i)
		{
			clAlpha[i] = __min(clAlpha[i]+(64*dt),128);
		}
	}
	else
	{
		for(U32 i = 0; i < numSquares; ++i)
		{
			clAlpha[i] = __max(clAlpha[i]-(64*dt),0);
		}
	}

	if(numSquares == 1)
	{
		if(time > 0.5)
		{
			gvec[1] = transform.translation + sprayDir*GRIDSIZE*2;
			clAlpha[1] = 0;
			++numSquares;

			CQASSERT(objMapSystemID == systemID);
			objMapSquare[objMapNum] = OBJMAP->GetMapSquare(systemID,gvec[1]);
			OBJMAP->AddObjectToMap(this,objMapSystemID,objMapSquare[objMapNum]);
			++objMapNum;
		}
	}
	else if(numSquares == 2)
	{
		if(time > 1.0)
		{
			gvec[2] = transform.translation + sprayDir*GRIDSIZE*2+ sideDir*GRIDSIZE;
			clAlpha[2] = 0;
			gvec[3] = transform.translation + sprayDir*GRIDSIZE*2- sideDir*GRIDSIZE;
			clAlpha[3] = 0;
			gvec[4] = transform.translation + sprayDir*GRIDSIZE*3;
			clAlpha[4] = 0;
			numSquares += 3;

			CQASSERT(objMapSystemID == systemID);
			objMapSquare[objMapNum] = OBJMAP->GetMapSquare(systemID,gvec[2]);
			OBJMAP->AddObjectToMap(this,objMapSystemID,objMapSquare[objMapNum]);
			++objMapNum;

			CQASSERT(objMapSystemID == systemID);
			objMapSquare[objMapNum] = OBJMAP->GetMapSquare(systemID,gvec[3]);
			OBJMAP->AddObjectToMap(this,objMapSystemID,objMapSquare[objMapNum]);
			++objMapNum;

			CQASSERT(objMapSystemID == systemID);
			objMapSquare[objMapNum] = OBJMAP->GetMapSquare(systemID,gvec[4]);
			OBJMAP->AddObjectToMap(this,objMapSystemID,objMapSquare[objMapNum]);
			++objMapNum;
		}
	}
	else if(numSquares == 5)
	{
		if(time > 1.5)
		{
			gvec[5] = transform.translation + sprayDir*GRIDSIZE*3+ sideDir*GRIDSIZE;
			clAlpha[5] = 0;
			gvec[6] = transform.translation + sprayDir*GRIDSIZE*3- sideDir*GRIDSIZE;
			clAlpha[6] = 0;
			gvec[7] = transform.translation + sprayDir*GRIDSIZE*4;
			clAlpha[7] = 0;
			numSquares += 3;
			
			CQASSERT(objMapSystemID == systemID);
			objMapSquare[objMapNum] = OBJMAP->GetMapSquare(systemID,gvec[5]);
			OBJMAP->AddObjectToMap(this,objMapSystemID,objMapSquare[objMapNum]);
			++objMapNum;

			CQASSERT(objMapSystemID == systemID);
			objMapSquare[objMapNum] = OBJMAP->GetMapSquare(systemID,gvec[6]);
			OBJMAP->AddObjectToMap(this,objMapSystemID,objMapSquare[objMapNum]);
			++objMapNum;

			CQASSERT(objMapSystemID == systemID);
			objMapSquare[objMapNum] = OBJMAP->GetMapSquare(systemID,gvec[7]);
			OBJMAP->AddObjectToMap(this,objMapSystemID,objMapSquare[objMapNum]);
			++objMapNum;
		}
	}else if(numSquares == 8)
	{
		if(time > 2.0)
		{
			gvec[8] = transform.translation + sprayDir*GRIDSIZE*3+ sideDir*GRIDSIZE*2;
			clAlpha[8] = 0;
			gvec[9] = transform.translation + sprayDir*GRIDSIZE*3- sideDir*GRIDSIZE*2;
			clAlpha[9] = 0;
			gvec[10] = transform.translation + sprayDir*GRIDSIZE*4+ sideDir*GRIDSIZE;
			clAlpha[10] = 0;
			gvec[11] = transform.translation + sprayDir*GRIDSIZE*4- sideDir*GRIDSIZE;
			clAlpha[11] = 0;
			numSquares += 4;

			CQASSERT(objMapSystemID == systemID);
			objMapSquare[objMapNum] = OBJMAP->GetMapSquare(systemID,gvec[8]);
			OBJMAP->AddObjectToMap(this,objMapSystemID,objMapSquare[objMapNum]);
			++objMapNum;

			CQASSERT(objMapSystemID == systemID);
			objMapSquare[objMapNum] = OBJMAP->GetMapSquare(systemID,gvec[9]);
			OBJMAP->AddObjectToMap(this,objMapSystemID,objMapSquare[objMapNum]);
			++objMapNum;

			CQASSERT(objMapSystemID == systemID);
			objMapSquare[objMapNum] = OBJMAP->GetMapSquare(systemID,gvec[10]);
			OBJMAP->AddObjectToMap(this,objMapSystemID,objMapSquare[objMapNum]);
			++objMapNum;

			CQASSERT(objMapSystemID == systemID);
			objMapSquare[objMapNum] = OBJMAP->GetMapSquare(systemID,gvec[11]);
			OBJMAP->AddObjectToMap(this,objMapSystemID,objMapSquare[objMapNum]);
			++objMapNum;
		}
	}

}
//---------------------------------------------------------------------------
//
bool RepellentCloud::TerrainCallback (const struct FootprintInfo & info, struct GRIDVECTOR & pos)
{
	if(numTargets < MAX_REPEL_TARGETS)
	{
		if ((info.flags & (TERRAIN_PARKED|TERRAIN_MOVING)) != 0 && (info.flags & TERRAIN_DESTINATION)==0)
		{
			IBaseObject * obj = OBJLIST->FindObject(info.missionID);
			if (obj && (!(obj->effectFlags.bRepellent)))
			{
				obj->effectFlags.bRepellent = true;
				targets[numTargets] = info.missionID;
				targetsHeld |= (0x01 << numTargets);
				++numTargets;
			}
		}
	}
	return true;
}
//----------------------------------------------------------------------------------
//
BOOL32 RepellentCloud::Update (void)
{
	if(numTargets && ((~targetsHeld) & (~(0xFFFFFFFF << numTargets))))
	{
		for(U32 i = 0; i < numTargets; ++ i)
		{
			if(!(targetsHeld & (0x01 << i)))
			{
				IBaseObject * obj = OBJLIST->FindObject(targets[i]);
				if (obj && (!(obj->effectFlags.bRepellent)) && obj->GetSystemID() == systemID)
				{
					obj->effectFlags.bRepellent = true;
					targetsHeld |= (0x01 << i);
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
		COMPTR<ITerrainMap> map;
		SECTOR->GetTerrainMap(systemID,map);

		for(U32 count = 0; count < numSquares; ++count)
		{
			map->TestSegment(gvec[count],gvec[count],this);
		}

		if(lastTime+1.0 < time)
		{
			lastTime = time;
			for(U32 count = 0; count < numTargets; ++count)
			{
				IBaseObject * obj = OBJLIST->FindObject(targets[count]);
				if(obj)
				{
					OBJPTR<IWeaponTarget> targ;
					obj->QueryInterface(IWeaponTargetID,targ);
					if(targ)
					{
						if(THEMATRIX->IsMaster())
						{
							GRIDVECTOR loc = obj->GetGridPosition();
							bool bDamage = false;
							for(U32 i = 0 ; i < numSquares; ++ i)
							{
								if(loc.isMostlyEqual(gvec[i]))
								{
									bDamage = true;
									targ->AttachBlast(repelMesh.dammageType,obj->GetTransform());
									targ->ApplyAOEDamage(ownerID,data->damagePerSec);
									i = numSquares;
								}
							}
							if(!bDamage && numToRemove < MAX_REPEL_TARGETS)
							{
								if(targetsHeld & (0x01 << count))
								{
									obj->effectFlags.bRepellent=false;
								}
								removeTargets[numToRemove] = obj->GetPartID();
								++numToRemove;
								if(count < lastSent)
									--lastSent;
								--numTargets;
								if(count != numTargets)
								{
									memmove(targets+count,targets+count+1,sizeof(U32)*(numTargets-count));
									U32 upperMask = ((0xFFFFFFFF << (count+1))& targetsHeld) >> 1;
									targetsHeld = (targetsHeld & (~(0xFFFFFFFF<< count))) | upperMask;
								}
								--count;
							}
						}
						else
						{
							targ->AttachBlast(repelMesh.dammageType,obj->GetTransform());
						}
					}
				}else if(THEMATRIX->IsMaster() && numToRemove < MAX_REPEL_TARGETS)
				{
					removeTargets[numToRemove] = targets[count];
					++numToRemove;
					if(count < lastSent)
						--lastSent;
					--numTargets;
					if(count != numTargets)
					{
						memmove(targets+count,targets+count+1,sizeof(U32)*(numTargets-count));
						U32 upperMask = ((0xFFFFFFFF << (count+1))& targetsHeld) >> 1;
						targetsHeld = (targetsHeld & (~(0xFFFFFFFF<< count))) | upperMask;
					}
					--count;
				}
			};
		}
	}
	return result;
}
//----------------------------------------------------------------------------------
//
void RepellentCloud::Render (void)
{
	if (bVisible)
	{					
		if(CQEFFECTS.bExpensiveTerrain==0)
		{
			BATCH->set_state(RPR_BATCH,false);
			DisableTextures();
			CAMERA->SetModelView();
			BATCH->set_render_state(D3DRS_ZENABLE,FALSE);
			BATCH->set_render_state(D3DRS_ZWRITEENABLE,FALSE);
			BATCH->set_render_state(D3DRS_ALPHABLENDENABLE,FALSE);

			Vector epos = gvec[0];		
		
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

			SINGLE size = GRIDSIZE*0.6;

			Vector xOff = sprayDir*GRIDSIZE*0.25;
			Vector yOff = sideDir*GRIDSIZE*0.25;

			PB.Begin(PB_POINTS);

			PB.Color3ub(200,255,0);
			for(U32 count = 0; count < numSquares; ++count)
			{
				TRANSFORM trans ;

				Vector realI;
				Vector realJ;

				Vector v[4];

				Vector base ; 

				Vector offBase;


				trans = baseTrans;

				base = gvec[count]; 

				if(count != 10)
				{
					trans.rotate_about_k(time+count);

					realI = trans.get_i()*size;
					realJ = trans.get_j()*size;

					offBase = base+xOff+yOff;

					v[0] = offBase+realI+realJ;
					v[1] = offBase-realI+realJ;
					v[2] = offBase-realI-realJ;
					v[3] = offBase+realI-realJ;
					
					PB.Vertex3f(v[0].x,v[0].y,v[0].z);
					PB.Vertex3f(v[1].x,v[1].y,v[1].z);
					PB.Vertex3f(v[2].x,v[2].y,v[2].z);
					PB.Vertex3f(v[3].x,v[3].y,v[3].z);
				}
				if(count != 2 && count != 7)
				{
					trans.rotate_about_k(1.0);

					realI = trans.get_i()*size;
					realJ = trans.get_j()*size;

					offBase = base-xOff+yOff;

					v[0] = offBase+realI+realJ;
					v[1] = offBase-realI+realJ;
					v[2] = offBase-realI-realJ;
					v[3] = offBase+realI-realJ;
					
					PB.Vertex3f(v[0].x,v[0].y,v[0].z);
					PB.Vertex3f(v[1].x,v[1].y,v[1].z);
					PB.Vertex3f(v[2].x,v[2].y,v[2].z);
					PB.Vertex3f(v[3].x,v[3].y,v[3].z);
				}

				if(count != 3 && count != 8)
				{
					trans.rotate_about_k(1.0);

					realI = trans.get_i()*size;
					realJ = trans.get_j()*size;

					offBase = base-xOff-yOff;

					v[0] = offBase+realI+realJ;
					v[1] = offBase-realI+realJ;
					v[2] = offBase-realI-realJ;
					v[3] = offBase+realI-realJ;
					
					PB.Vertex3f(v[0].x,v[0].y,v[0].z);
				    PB.Vertex3f(v[1].x,v[1].y,v[1].z);
					PB.Vertex3f(v[2].x,v[2].y,v[2].z);
					PB.Vertex3f(v[3].x,v[3].y,v[3].z);
				}

				if(count != 11)
				{
					trans.rotate_about_k(1.0);

					realI = trans.get_i()*size;
					realJ = trans.get_j()*size;

					offBase = base+xOff-yOff;

					v[0] = offBase+realI+realJ;
					v[1] = offBase-realI+realJ;
					v[2] = offBase-realI-realJ;
					v[3] = offBase+realI-realJ;
					
					PB.Vertex3f(v[0].x,v[0].y,v[0].z);
					PB.Vertex3f(v[1].x,v[1].y,v[1].z);
					PB.Vertex3f(v[2].x,v[2].y,v[2].z);
					PB.Vertex3f(v[3].x,v[3].y,v[3].z);
				}

			}
		
			PB.End();		
		}
		else
		{
			BATCH->set_state(RPR_BATCH,true);
			BATCH->set_state(RPR_STATE_ID,repelMesh.tex1ID);

			CAMERA->SetModelView();

			BATCH->set_render_state(D3DRS_ZWRITEENABLE,FALSE);
			BATCH->set_render_state(D3DRS_ZENABLE,0);
			BATCH->set_render_state(D3DRS_ALPHABLENDENABLE,TRUE);
			BATCH->set_render_state(D3DRS_SRCBLEND,D3DBLEND_SRCALPHA);
			BATCH->set_render_state(D3DRS_DESTBLEND,D3DBLEND_INVSRCALPHA);
			BATCH->set_render_state(D3DRS_CULLMODE,D3DCULL_NONE);

			if ((multiStages == 1 || multiStages == 0xffffffff))
			{

				BATCH->set_texture_stage_texture( 0, repelMesh.tex1ID );
				BATCH->set_texture_stage_texture( 1, repelMesh.tex2ID );
				
				// blending - This is the same as D3DTMAPBLEND_MODULATEALPHA
				BATCH->set_texture_stage_state( 0, D3DTSS_COLOROP, D3DTOP_MODULATE );
				BATCH->set_texture_stage_state( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
				BATCH->set_texture_stage_state( 0, D3DTSS_COLORARG2, D3DTA_DIFFUSE );
				BATCH->set_texture_stage_state( 0, D3DTSS_ALPHAOP, D3DTOP_MODULATE );
				BATCH->set_texture_stage_state( 0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE );
				BATCH->set_texture_stage_state( 0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE );
				
				// filtering - bilinear with mips
				//	BATCH->set_sampler_state( 0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR );
				//	BATCH->set_sampler_state( 0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR );
				//	BATCH->set_sampler_state( 0, D3DSAMP_MIPFILTER, D3DTEXF_POINT );
				
				// addressing - clamped
				BATCH->set_sampler_state( 0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP );
				BATCH->set_sampler_state( 0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP );
				
				BATCH->set_texture_stage_state( 1, D3DTSS_TEXCOORDINDEX, 1);
				// blending - This is the same as D3DTMAPBLEND_MODULATEALPHA
				BATCH->set_texture_stage_state( 1, D3DTSS_COLOROP, D3DTOP_MODULATE );
				BATCH->set_texture_stage_state( 1, D3DTSS_COLORARG1, D3DTA_TEXTURE );
				BATCH->set_texture_stage_state( 1, D3DTSS_COLORARG2, D3DTA_CURRENT );
				BATCH->set_texture_stage_state( 1, D3DTSS_ALPHAOP, D3DTOP_MODULATE );
				BATCH->set_texture_stage_state( 1, D3DTSS_ALPHAARG1, D3DTA_TEXTURE );
				BATCH->set_texture_stage_state( 1, D3DTSS_ALPHAARG2, D3DTA_CURRENT );
				
				// filtering - bilinear with mips
				//	BATCH->set_sampler_state( 1, D3DSAMP_MINFILTER, D3DTEXF_LINEAR );
				//	BATCH->set_sampler_state( 1, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR );
				//	BATCH->set_sampler_state( 1, D3DSAMP_MIPFILTER, D3DTEXF_POINT );
				
				// addressing - clamped
				BATCH->set_sampler_state( 1, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP );
				BATCH->set_sampler_state( 1, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP );
			}
			
			if (multiStages == 0xffffffff)
			{
				multiStages = (BATCH->verify_state() == GR_OK);
			}

			if (multiStages != 1)
			{
				SetupDiffuseBlend(repelMesh.tex1ID,TRUE);
			}


			BATCH->set_render_state(D3DRS_DITHERENABLE,true);

			Vector epos = gvec[0];		
		
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

			SINGLE size = GRIDSIZE*0.6;

			Vector xOff = sprayDir*GRIDSIZE*0.25;
			Vector yOff = sideDir*GRIDSIZE*0.25;

			PB.Begin(PB_QUADS);

			for(U32 count = 0; count < numSquares; ++count)
			{
				TRANSFORM trans ;

				Vector realI;
				Vector realJ;

				Vector v[4];

				Vector base ; 

				Vector offBase;

				PB.Color4ub(255,255,255,clAlpha[count]);

				trans = baseTrans;

				base = gvec[count]; 

				SINGLE width, height;
				if(count != 10)
				{
					trans.rotate_about_k(time+count);

					realI = trans.get_i()*size;
					realJ = trans.get_j()*size;

					offBase = base+xOff+yOff;

					v[0] = offBase+realI+realJ;
					v[1] = offBase-realI+realJ;
					v[2] = offBase-realI-realJ;
					v[3] = offBase+realI-realJ;
					
					height = 0.5*((count%4)>1);
					width = 0.5*(count%2);
					PB.TexCoord2f(width,height);   PB.Vertex3f(v[0].x,v[0].y,v[0].z);
					PB.TexCoord2f(0.5+width,height);   PB.Vertex3f(v[1].x,v[1].y,v[1].z);
					PB.TexCoord2f(0.5+width,0.5+height);   PB.Vertex3f(v[2].x,v[2].y,v[2].z);
					PB.TexCoord2f(width,0.5+height);   PB.Vertex3f(v[3].x,v[3].y,v[3].z);
				}
				if(count != 2 && count != 7)
				{
					trans.rotate_about_k(1.0);

					realI = trans.get_i()*size;
					realJ = trans.get_j()*size;

					offBase = base-xOff+yOff;

					v[0] = offBase+realI+realJ;
					v[1] = offBase-realI+realJ;
					v[2] = offBase-realI-realJ;
					v[3] = offBase+realI-realJ;
					
					height = 0.5*(((count+1)%4)>1);
					width = 0.5*((count+1)%2);
					PB.TexCoord2f(width,height);   PB.Vertex3f(v[0].x,v[0].y,v[0].z);
					PB.TexCoord2f(0.5+width,height);   PB.Vertex3f(v[1].x,v[1].y,v[1].z);
					PB.TexCoord2f(0.5+width,0.5+height);   PB.Vertex3f(v[2].x,v[2].y,v[2].z);
					PB.TexCoord2f(width,0.5+height);   PB.Vertex3f(v[3].x,v[3].y,v[3].z);
				}

				if(count != 3 && count != 8)
				{
					trans.rotate_about_k(1.0);

					realI = trans.get_i()*size;
					realJ = trans.get_j()*size;

					offBase = base-xOff-yOff;

					v[0] = offBase+realI+realJ;
					v[1] = offBase-realI+realJ;
					v[2] = offBase-realI-realJ;
					v[3] = offBase+realI-realJ;
					
					height = 0.5*(((count+2)%4)>1);
					width = 0.5*((count+2)%2);
					PB.TexCoord2f(width,height);   PB.Vertex3f(v[0].x,v[0].y,v[0].z);
					PB.TexCoord2f(0.5+width,height);   PB.Vertex3f(v[1].x,v[1].y,v[1].z);
					PB.TexCoord2f(0.5+width,0.5+height);   PB.Vertex3f(v[2].x,v[2].y,v[2].z);
					PB.TexCoord2f(width,0.5+height);   PB.Vertex3f(v[3].x,v[3].y,v[3].z);
				}

				if(count != 11)
				{
					trans.rotate_about_k(1.0);

					realI = trans.get_i()*size;
					realJ = trans.get_j()*size;

					offBase = base+xOff-yOff;

					v[0] = offBase+realI+realJ;
					v[1] = offBase-realI+realJ;
					v[2] = offBase-realI-realJ;
					v[3] = offBase+realI-realJ;
					
					height = 0.5*(((count+3)%4)>1);
					width = 0.5*((count+3)%2);
					PB.TexCoord2f(width,height);   PB.Vertex3f(v[0].x,v[0].y,v[0].z);
					PB.TexCoord2f(0.5+width,height);   PB.Vertex3f(v[1].x,v[1].y,v[1].z);
					PB.TexCoord2f(0.5+width,0.5+height);   PB.Vertex3f(v[2].x,v[2].y,v[2].z);
					PB.TexCoord2f(width,0.5+height);   PB.Vertex3f(v[3].x,v[3].y,v[3].z);
				}

			}
		
			PB.End();

			BATCH->set_state(RPR_STATE_ID,0);
		}
	}

}
//----------------------------------------------------------------------------------
//
void RepellentCloud::InitRecon(IReconLauncher * _ownerLauncher, U32 _dwMissionID)
{
	_ownerLauncher->QueryInterface(IReconLauncherID,ownerLauncher,NONSYSVOLATILEPTR);
	dwMissionID = _dwMissionID;
	
	playerID = MGlobals::GetPlayerFromPartID(dwMissionID);
	sprintf(partName,"RepellentCloud 0x%x",dwMissionID);

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
void RepellentCloud::ResolveRecon(IBaseObject * _ownerLauncher)
{
	_ownerLauncher->QueryInterface(IReconLauncherID,ownerLauncher, NONSYSVOLATILEPTR);
}
//----------------------------------------------------------------------------------
//
void RepellentCloud::LaunchProbe (IBaseObject * _owner, const class TRANSFORM & orientation, const class Vector * pos,
		U32 targetSystemID, IBaseObject * jumpTarget)
{
	CQASSERT(objMapNum == 0);
	for(U32 i = 0; i < objMapNum; ++i)
	{
		OBJMAP->RemoveObjectFromMap(this,objMapSystemID,objMapSquare[i]);
	}
	objMapNum = 0;

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

	time = 0;
	lastTime = 0;


	sprayDir = -transform.get_k();
	sideDir = transform.get_i();

	Vector check = transform.translation- *pos;
	if(check.x != 0 || check.y != 0)
	{
		SINGLE dist = check.fast_magnitude();
		if( dist > data->centerRange*GRIDSIZE)
		{
			transform.translation += sprayDir*(dist-data->centerRange*GRIDSIZE);
		}
	}

	gvec[0] = transform.translation + sprayDir*GRIDSIZE;
	clAlpha[0] = 0;

	numSquares = 1;

	OBJPTR<IWeaponTarget> ownerWT;
	owner->QueryInterface(IWeaponTargetID,ownerWT);

	TRANSFORM sprayTrans = transform;
	sprayTrans.rotate_about_j(PI);
	if (ownerWT)
		ownerWT->AttachBlast(repelMesh.sprayType,sprayTrans);

	if (hSound==0)
		hSound = SFXMANAGER->Open(data->launchSfx);
	SFXMANAGER->Play(hSound,systemID,pos);

	
	CQASSERT(!objMapNum);
	objMapSystemID = systemID;
	objMapSquare[objMapNum] = OBJMAP->GetMapSquare(systemID,gvec[0]);
	OBJMAP->AddObjectToMap(this,objMapSystemID,objMapSquare[objMapNum]);
	++objMapNum;
}
//----------------------------------------------------------------------------------
//
void RepellentCloud::ExplodeProbe()
{
	if(numTargets || numToRemove)
		bFreeTargets = true;
	else
		bFreeTargets = false;

	for(U32 i = 0; i < objMapNum; ++i)
	{
		OBJMAP->RemoveObjectFromMap(this,objMapSystemID,objMapSquare[i]);
	}
	objMapNum = 0;
	systemID = 0;
	bGone = true;
	SetReady(false);
}
//----------------------------------------------------------------------------------
//
void RepellentCloud::DeleteProbe()
{
	bLauncherDelete = true;
}
//----------------------------------------------------------------------------------
//
bool RepellentCloud::IsActive()
{
	return !bGone;
}
//---------------------------------------------------------------------------
//
BOOL32 RepellentCloud::Save (struct IFileSystem * inFile)
{
	DAFILEDESC fdesc = "REPELLENTCLOUD_SAVELOAD";
	COMPTR<IFileSystem> file;
	BOOL32 result = 0;
	DWORD dwWritten;
	REPELLENTCLOUD_SAVELOAD save;

	fdesc.lpImplementation = "DOS";
	fdesc.dwDesiredAccess = GENERIC_READ|GENERIC_WRITE;
	fdesc.dwShareMode = 0;  // no sharing
	fdesc.dwCreationDistribution = CREATE_ALWAYS;

	if (inFile->CreateInstance(&fdesc, file) != GR_OK)
		goto Done;

	memset(&save, 0, sizeof(save));

	memcpy(&save, static_cast<BASE_REPELLENTCLOUD_SAVELOAD *>(this), sizeof(BASE_REPELLENTCLOUD_SAVELOAD));

	FRAME_save(save);

	file->WriteFile(0, &save, sizeof(save), &dwWritten, 0);

	result = 1;

Done:	
	return result;
}
//---------------------------------------------------------------------------
//
BOOL32 RepellentCloud::Load (struct IFileSystem * inFile)
{	
	DAFILEDESC fdesc = "REPELLENTCLOUD_SAVELOAD";
	COMPTR<IFileSystem> file;
	BOOL32 result = 0;
	DWORD dwRead;
	REPELLENTCLOUD_SAVELOAD load;
	U8 buffer[1024];

	fdesc.lpImplementation = "DOS";
	if (inFile->CreateInstance(&fdesc, file) != GR_OK)
		goto Done;

	if (file->ReadFile(0, buffer, sizeof(buffer), &dwRead, 0) == 0)
		goto Done;
	MISSION->CorrelateSymbol("REPELLENTCLOUD_SAVELOAD", buffer, &load);

	FRAME_load(load);

	*static_cast<BASE_REPELLENTCLOUD_SAVELOAD *>(this) = load;

	result = 1;

Done:	
	return result;
}
//---------------------------------------------------------------------------
//
void RepellentCloud::ResolveAssociations()
{
	OBJLIST->FindObject(ownerID, NONSYSVOLATILEPTR, owner);

	objMapSystemID = systemID;
	if(objMapSystemID)
	{
		for(U32 i = 0; i < numSquares; ++i)
		{
			objMapSquare[objMapNum] = OBJMAP->GetMapSquare(systemID,gvec[i]);
			OBJMAP->AddObjectToMap(this,objMapSystemID,objMapSquare[objMapNum]);
			++objMapNum;
		}
	}
}
//---------------------------------------------------------------------------
//
U32 RepellentCloud::getSyncData (void * buffer)
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
					IBaseObject * targ = OBJLIST->FindObject(targets[i]);
					if(targ)
						targ->effectFlags.bRepellent=false;
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
					IBaseObject * targ = OBJLIST->FindObject(targets[i]);
					if(targ)
						targ->effectFlags.bRepellent=false;
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
				IBaseObject * targ = OBJLIST->FindObject(targets[i]);
				if(targ)
					targ->effectFlags.bRepellent=false;
			}
		}
		bDeleteRequested = true;
		((U8*) buffer)[0] = 0;
		lastSent = numTargets = 0;
		return 1;
	}
	if(numTargets == lastSent && !numToRemove)
		return 0;
	U32 result = 1;
	bool bSend = false;

	*((U8 *)(buffer)) = numToRemove;

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
void RepellentCloud::putSyncData (void * buffer, U32 bufferSize, bool bLateDelivery)
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
						IBaseObject * targ = OBJLIST->FindObject(targets[i]);
						if(targ)
							targ->effectFlags.bRepellent=false;
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
						IBaseObject * targ = OBJLIST->FindObject(targets[i]);
						if(targ)
							targ->effectFlags.bRepellent=false;
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
					IBaseObject * targ = OBJLIST->FindObject(targets[i]);
					if(targ)
						targ->effectFlags.bRepellent=false;
				}
			}
			bDeleteRequested = true;
			lastSent = numTargets = 0;
		}
	}
	else
	{
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
							IBaseObject * targ = OBJLIST->FindObject(targets[targCount]);
							if(targ)
								targ->effectFlags.bRepellent=false;
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
		CQASSERT(numToGet + numTargets <= MAX_REPEL_TARGETS);
		for(U32 count = 0; count < numToGet; ++count)
		{
			targets[numTargets] = buf[count];
			++numTargets;

			IBaseObject * obj = OBJLIST->FindObject(buf[count]);
			if (obj)
			{
				if((!obj->effectFlags.bRepellent) && obj->GetSystemID() == systemID)
				{
					obj->effectFlags.bRepellent = true;
					targetsHeld |= (0x01 << (numTargets-1));
				}
				else
					targetsHeld &= (~(0x01 << (numTargets-1)));
			}
		}
		lastSent = numTargets;
	}
}
//---------------------------------------------------------------------------
//
void RepellentCloud::init (REPELLENTCLOUD_INIT &initData)
{
	FRAME_init(initData);
	data = initData.pData;

	CQASSERT(data);
	CQASSERT(data->wpnClass == WPN_REPELLENTCLOUD);
	CQASSERT(data->objClass == OC_WEAPON);

	pArchetype = initData.pArchetype;
	objClass = OC_WEAPON;
	bNoMoreSync = false;
}
//------------------------------------------------------------------------------------------
//---------------------------RepellentCloud Factory----------------------------------------------
//------------------------------------------------------------------------------------------
//
struct DACOM_NO_VTABLE RepellentCloudFactory : public IObjectFactory
{
	struct OBJTYPE : REPELLENTCLOUD_INIT
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

	BEGIN_DACOM_MAP_INBOUND(RepellentCloudFactory)
	DACOM_INTERFACE_ENTRY(IObjectFactory)
	END_DACOM_MAP()

	RepellentCloudFactory (void) { }

	~RepellentCloudFactory (void);

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

	// RepellentCloudFactory methods 

	IDAComponent * getBase (void)
	{
		return static_cast<IObjectFactory *> (this);
	}
};
//--------------------------------------------------------------------------//
//
RepellentCloudFactory::~RepellentCloudFactory (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (OBJLIST && OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
		connection->Unadvise(factoryHandle);
}
//--------------------------------------------------------------------------//
//
void RepellentCloudFactory::init (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
		connection->Advise(getBase(), &factoryHandle);
}
//-----------------------------------------------------------------------------
//
HANDLE RepellentCloudFactory::CreateArchetype (const char *szArchname, OBJCLASS objClass, void *_data)
{
	OBJTYPE * result = 0;

	if (objClass == OC_WEAPON)
	{
		BT_REPELLENTCLOUD_DATA * data = (BT_REPELLENTCLOUD_DATA *)_data;
		if (data->wpnClass == WPN_REPELLENTCLOUD)
		{
			result = new OBJTYPE;
			
			result->pArchetype = ARCHLIST->GetArchetype(szArchname);
			result->pData = data;
			//
			// force preload of sound effect
			// 
			SFXMANAGER->Preload(data->launchSfx);

			repelMesh.tex1ID = TMANAGER->CreateTextureFromFile("cloud_weapon3.tga", TEXTURESDIR, DA::TGA,PF_4CC_DAA4);
			repelMesh.tex2ID = TMANAGER->CreateTextureFromFile("neb1.tga", TEXTURESDIR, DA::TGA,PF_4CC_DAA4);
			
			repelMesh.sprayType = ARCHLIST->LoadArchetype("BLAST!!RepellentCloud");
			repelMesh.dammageType = ARCHLIST->LoadArchetype("BLAST!!RepellentCloudDamage");

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
BOOL32 RepellentCloudFactory::DestroyArchetype (HANDLE hArchetype)
{
	OBJTYPE * objtype = (OBJTYPE *)hArchetype;

	delete objtype;

	return 1;
}
//-------------------------------------------------------------------
//
IBaseObject * RepellentCloudFactory::CreateInstance (HANDLE hArchetype)
{
	OBJTYPE * objtype = (OBJTYPE *)hArchetype;

	RepellentCloud * repClound = new ObjectImpl<RepellentCloud>;

	repClound->init(*objtype);

	return repClound;
}
//-------------------------------------------------------------------
//
void RepellentCloudFactory::EditorCreateInstance (HANDLE hArchetype, const MISSION_INFO & info)
{
	// does not support this!
}
//---------------------------------------------------------------------------
//-------------------------GLOBAL STARTUP------------------------------------
//---------------------------------------------------------------------------
//
struct _repelBolt : GlobalComponent
{
	RepellentCloudFactory * sfactory;

	virtual void Startup (void)
	{
		sfactory = new DAComponent<RepellentCloudFactory>;
		AddToGlobalCleanupList((IDAComponent **) &sfactory);
	}

	virtual void Initialize (void)
	{
		sfactory->init();
	}
};

static _repelBolt __repelBolt;
//---------------------------------------------------------------------------
//------------------------End RepellentCloud.cpp----------------------------------------
//---------------------------------------------------------------------------


