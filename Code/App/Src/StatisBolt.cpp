//--------------------------------------------------------------------------//
//                                                                          //
//                             StasisBolt.cpp                          //
//                                                                          //
//                  COPYRIGHT (C) 1997 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*

    $Author: Jasony $

    $Header: /Conquest/App/Src/StatisBolt.cpp 64    10/18/00 1:43a Jasony $
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
#include "ObjMap.h"
#include "stdio.h"
#include "IVertexBuffer.h"
#include <DEffectOpts.h>
#include "TObjRender.H"

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
#define STASIS_TIMOUT 3.0

#define INNER_RING 800.0
#define MID_RING ((INNER_RING+(data->explosionRange*GRIDSIZE))/2)
#define MID_INNER_DIFF (MID_RING-INNER_RING)

struct STASISBOLT_INIT : RenderArch
{
	BT_STASISBOLT_DATA * pData;
	S32 archIndex;
	IMeshArchetype * meshArch;
	PARCHETYPE pArchetype;
	AnimArchetype * stasisAnm;
};

#define NUM_STATSIS_SPARKS 40
#define STASIS_SEGMENTS 40
#define EFFECT_RATE 3
#define SPARK_SIZE 3500
#define SPARK_SIZE_FINAL 1500
#define SPARK_SPACING 1500
#define NUM_SPARK_RING 16
#define STASIS_ANM_COUNT 10

struct StasisBoltMesh : IVertexBufferOwner, RenderArch
{
	Vector ringCenter[STASIS_SEGMENTS];
	Vector upperRing[STASIS_SEGMENTS];
	Vector sparks1[NUM_SPARK_RING];
	Vector sparks2[NUM_SPARK_RING];

	U32 baseTexID;
	U32 moveTexID;
	U32 vb_handle;

	virtual void RestoreVertexBuffers();

	StasisBoltMesh()
	{
		vb_handle = 0;
		vb_mgr->Add(this);
	}

	~StasisBoltMesh()
	{
		vb_mgr->Delete(this);
	}

}stasisBoltMesh;

void StasisBoltMesh::RestoreVertexBuffers()
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
		result = PIPE->create_vertex_buffer( D3DFVF_RPVERTEX2, STASIS_SEGMENTS*2+2, IRP_VBF_SYSTEM, &vb_handle );
		CQASSERT(result == GR_OK);
		result = PIPE->lock_vertex_buffer( vb_handle, DDLOCK_WRITEONLY, (void **)&vb_data, &dwSize );
		CQASSERT(result == GR_OK);
		
		for(int i = 0; i < STASIS_SEGMENTS; ++i)
		{
			if(i%2)
			{
				vb_data[i*2].u = 0.16666666f;
				vb_data[i*2].v = 0.75f;
			}
			else
			{
				vb_data[i*2].u = 0.5f;
				vb_data[i*2].v = 0.75f;
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
		
		if(STASIS_SEGMENTS%2)
		{
			vb_data[STASIS_SEGMENTS*2].u = 0.16666666f;
			vb_data[STASIS_SEGMENTS*2].v = 0.75f;
		}
		else
		{
			vb_data[STASIS_SEGMENTS*2].u = 0.5f;
			vb_data[STASIS_SEGMENTS*2].v = 0.75f;
		}
		vb_data[STASIS_SEGMENTS*2].u2 = STASIS_SEGMENTS;
		vb_data[STASIS_SEGMENTS*2].pos = ringCenter[0];
		
		vb_data[STASIS_SEGMENTS*2+1].color = 0x00ffffffff;
		if(STASIS_SEGMENTS%2)
		{
			vb_data[STASIS_SEGMENTS*2+1].u = 0.0f;
			vb_data[STASIS_SEGMENTS*2+1].v = 0.5f;
		}
		else
		{
			vb_data[STASIS_SEGMENTS*2+1].u = 0.33333333f;
			vb_data[STASIS_SEGMENTS*2+1].v = 0.5f;
		}
		vb_data[STASIS_SEGMENTS*2+1].u2 = STASIS_SEGMENTS+0.5f;
		vb_data[STASIS_SEGMENTS*2+1].pos = upperRing[0];
		
		
		result = PIPE->unlock_vertex_buffer( vb_handle );
		CQASSERT(result == GR_OK);
	}
	else
		vb_handle = 0;
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#define MESH_SPARKS 9
#define MAX_TENDRILS MAX_STASIS_TARGETS
#define MAX_SEGMENTS 35

struct StasisTendril
{
	Vector start,end;
	OBJPTR<IWeaponTarget> target;
	OBJPTR<IBaseObject> source;
	TRANSFORM tobject_to_world;
	S32 frame_ref[MAX_SEGMENTS];
	int segments;

	SINGLE length;
	S32 targetID;
	U8 segmentDrawn;

	//extent map stuff
	const RECT *extents;
	SINGLE z_step,z_min;
	U8 slices;
	U8 startSegment;
	SINGLE moveTime;

	void Render(U8 alpha);
	void Update(SINGLE dt);
	void Init();
};

void StasisTendril::Render(U8 alpha)
{
	if (target == 0 || source == 0)
		return;

	U32 systemID = SECTOR->GetCurrentSystem();

	if (target.Ptr()->GetSystemID() != systemID || source->GetSystemID() != systemID)
		return;

	start = source->GetPosition();
	end = target.Ptr()->GetPosition();
	Vector direction = end-start;
	length = direction.magnitude();
	if(length == 0)
	{
		direction = Vector(0,0,10);
		length = direction.magnitude();
	}
	direction /= length;
	segments = min(ceil(length*0.0008),MAX_SEGMENTS);

	if (FOGOFWAR->CheckVisiblePosition(start) || target.Ptr()->IsVisibleToPlayer(MGlobals::GetThisPlayer()) ||
	    (DEFAULTS->GetDefaults()->bVisibilityRulesOff || DEFAULTS->GetDefaults()->bEditorMode) )
	{
#define TOLERANCE 0.00001f
		
		Vector v0,v1,v2,v3;
		Vector cpos = MAINCAM->get_position();
		
#define BM_WDTH 240.0  // value was 255.0
		SINGLE width = BM_WDTH;
		//	width += (U8)(12*test);
		//	SINGLE mag,mag2;
		
		Vector look (cpos - start);
	
		//this is the draw code that makes the arcCannon always visible
		//look = direction;
		
		Vector i = cross_product(look,direction);//(look.y, -look.x, 0);
		
		if (fabs (i.x) < TOLERANCE && fabs (i.y) < TOLERANCE)
		{
			i.x = 1.0f;
		}
		i.normalize ();
		
			
		U8 c;

		//	PrimitiveBuilder PB(PIPE);
		PB.Color4ub(255,255,255,alpha);

		Vector control;
		control = start + direction*length/3;
		control.z += 1000;

		Vector ptBegin;
		Vector ptEnd;

		S32 posibleSeg = segments-segmentDrawn;
		for (c = max(posibleSeg,0) ; c <= segments; c++)
		{
			SINGLE t;
			if(c== 0)
				t = 0.0;
			else
				t = (c-moveTime)/float(segments);
			SINGLE u;
			if(c== segments)
				u = 1.0;
			else
				u = (c+1-moveTime)/float(segments);

			// quadratic bezier
			ptBegin = (1-t)*(1-t)*start + 2*(1-t)*t*control + t*t*end;
			ptEnd = (1-u)*(1-u)*start + 2*(1-u)*u*control + u*u*end;
			
			v0 = (ptBegin - (i * (width*t)));
			v1 = (ptBegin + (i * (width*t)));
			v2 = (ptEnd + (i * (width*u)));
			v3 = (ptEnd - (i * (width*u)));
			
			U8 frameNum = (startSegment+c)%MAX_SEGMENTS;
			SINGLE frameX1 = 0.01+0.50*((frame_ref[frameNum]+1)%2);
			SINGLE frameX2 = frameX1+0.48;
			if(c==segments)
				frameX2 = frameX1+moveTime*(frameX2-frameX1);

			SINGLE frameY1 = 0.51+0.25*(frame_ref[frameNum] >0);
			SINGLE frameY2 = frameY1+0.23;
////			BATCH->set_texture_stage_texture( 0,frame->texture);

			BATCH->set_state(RPR_STATE_ID,stasisBoltMesh.baseTexID+1);
			SetupDiffuseBlend(stasisBoltMesh.baseTexID,TRUE);

			//stupid correction factor here to eliminate seams
			PB.Begin(PB_QUADS);
			PB.TexCoord2f(frameX1,frameY1);     PB.Vertex3f(v0.x,v0.y,v0.z);
			PB.TexCoord2f(frameX1,frameY2);		PB.Vertex3f(v1.x,v1.y,v1.z);
			PB.TexCoord2f(frameX2,frameY2);		PB.Vertex3f(v2.x,v2.y,v2.z);
			PB.TexCoord2f(frameX2,frameY1);		PB.Vertex3f(v3.x,v3.y,v3.z);

			PB.End();
			BATCH->set_state(RPR_STATE_ID,0);
		}		
	}
#undef TOLERANCE
}

void StasisTendril::Update(SINGLE dt)
{
	if (target == 0 || source == 0)
		return;

	moveTime += dt;
	while(moveTime > 1.0)
	{
		moveTime -= 1.0;
		startSegment = (startSegment+1)%MAX_SEGMENTS;
		if(segmentDrawn < MAX_SEGMENTS)
			++segmentDrawn;
	}
}

void StasisTendril::Init ()
{
	segmentDrawn = 0;
	startSegment = 0;
	moveTime = 0;
	
	for (int i=0;i<MAX_SEGMENTS;i++)
	{
		frame_ref[i] = rand()%3;
	}
		
	OBJPTR<IExtent> extentObj;
	target->QueryInterface(IExtentID,extentObj);
	CQASSERT(extentObj);
	extentObj->GetExtentInfo(extents,&z_step,&z_min,&slices);
	CQASSERT(extents && "No extent info!?");
}

static struct StasisBolt * stasisBoltList = NULL;

struct _NO_VTABLE StasisBolt : public ObjectRender<ObjectMission<ObjectTransform<ObjectFrame<IBaseObject,struct STASISBOLT_SAVELOAD,struct STASISBOLT_INIT> > > >, ISaveLoad,
										BASE_STASISBOLT_SAVELOAD,ITerrainSegCallback,IReconProbe
{


	BEGIN_MAP_INBOUND(StasisBolt)
	_INTERFACE_ENTRY(IBaseObject)
	_INTERFACE_ENTRY(ISaveLoad)
	_INTERFACE_ENTRY(IMissionActor)
	_INTERFACE_ENTRY(IReconProbe)
	END_MAP()

	//------------------------------------------
	OBJPTR<IBaseObject> owner;
	OBJPTR<IBaseObject> target;
	
	const BT_STASISBOLT_DATA * data;
	HSOUND hSound;

	U32 objMapSquare;
	U32 objMapSystemID;

	U32 multiStages;
	AnimInstance anm[STASIS_ANM_COUNT];

	StasisTendril tendril[MAX_TENDRILS];

	GeneralSyncNode  genSyncNode;

	OBJPTR<IReconLauncher> ownerLauncher;

	StasisBolt * nextStasisBolt;

	//------------------------------------------

	StasisBolt (void) :
			genSyncNode(this, SyncGetProc(&StasisBolt::getSyncData), SyncPutProc(&StasisBolt::putSyncData))
	{
		nextStasisBolt = stasisBoltList;
		stasisBoltList = this;

		bDeleteRequested = false;
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

	virtual ~StasisBolt (void);

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

	void init (STASISBOLT_INIT &initData);

	void renderSoftwareRing();

	void renderRing();

	void renderSpark();

	void renderAnm();
	
	U32 getSyncData (void * buffer);
	void putSyncData (void * buffer, U32 bufferSize, bool bLateDelivery);

	IBaseObject * getBase (void)
	{
		return this;
	}

#define MAX_CLIP_POINTS 30

	bool clipsFields(const Vector pos, const U32 numClipPoints,Vector * clipPoints)
	{
		SINGLE dist = data->explosionRange*GRIDSIZE;
		dist = dist*dist;
		for(U32 i = 0; i < numClipPoints; ++i)
		{
			if((clipPoints[i]-pos).magnitude_squared() < dist)
				return true;
		}
		return false;
	}

	void findClipingFields(U32 & numClipPoints,Vector * clipPoints)
	{
		SINGLE dist = data->explosionRange*GRIDSIZE*2;
		dist = dist*dist;
		numClipPoints = 0;
		StasisBolt * search = nextStasisBolt;
		while(search && numClipPoints < MAX_CLIP_POINTS)
		{
			if(search->GetSystemID() == GetSystemID() && search->bVisible && search->time != -1 &&
				(transform.translation-search->transform.translation).magnitude_squared() < dist)
			{
				clipPoints[numClipPoints] = search->transform.translation;
				++numClipPoints;
			}
			search = search->nextStasisBolt;
		}
	}
};


//----------------------------------------------------------------------------------
//
StasisBolt::~StasisBolt (void)
{
	StasisBolt * search = stasisBoltList;
	StasisBolt * prev = NULL;
	while(search && search != this)
	{
		prev = search;
		search = search->nextStasisBolt;
	}
	if(search)
	{
		if(prev)
			prev->nextStasisBolt = nextStasisBolt;
		else
			stasisBoltList = nextStasisBolt;
	}
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
U32 StasisBolt::GetSystemID (void) const
{
	return systemID;
}
//----------------------------------------------------------------------------------
//
void StasisBolt::CastVisibleArea()
{
	U32 tID = GetPlayerID();
	if(!tID)
		tID = playerID;
	SetVisibleToPlayer(tID);
}
//----------------------------------------------------------------------------------
//
void StasisBolt::PhysicalUpdate (SINGLE dt)
{
	if(!bReady)
		return;
	if(time == -1)
	{
		SINGLE distSq = (targetPos-transform.translation).magnitude_squared();
		SINGLE moveDist = data->maxVelocity*dt;
		if(distSq < moveDist*moveDist)
		{
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
				OBJMAP->AddObjectToMap(this,systemID,objMapSquare);
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
			OBJMAP->AddObjectToMap(this,systemID,objMapSquare);
		}
	}
	else
	{	
		if(bVisible)
		{
			Vector centerPos = targetPos;
			for(U32 anmCnt = 0; anmCnt < STASIS_ANM_COUNT; ++anmCnt)
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
			for(U32 i = 0 ; i < numTargets; ++i)
			{
				tendril[i].Update(dt);
			}
		}
		time += dt;
	}
}
//---------------------------------------------------------------------------
//
bool StasisBolt::TerrainCallback (const struct FootprintInfo & info, struct GRIDVECTOR & pos)
{
	if(numTargets < MAX_STASIS_TARGETS)
	{
		if ((info.flags & (TERRAIN_PARKED|TERRAIN_MOVING)) != 0 && (info.flags & TERRAIN_DESTINATION)==0)
		{
			IBaseObject * obj = OBJLIST->FindObject(info.missionID);
			if (obj && (!(obj->effectFlags.bStasis))) 
			{
				if (obj->QueryInterface(IWeaponTargetID,tendril[numTargets].target,GetPlayerID()))
				{
//					CQASSERT(tendril[numTargets].target.ptr->GetSystemID() == systemID);
					obj->effectFlags.bStasis = true;
					targets[numTargets] = info.missionID;
					targetsHeld |= (0x01 << numTargets);
					//CQTRACE12("\nStasis add: ox%x target:0x%x\n", dwMissionID,obj->GetPartID());

					getBase()->QueryInterface(IBaseObjectID, tendril[numTargets].source, NONSYSVOLATILEPTR);

					tendril[numTargets].length = GetGridPosition()-obj->GetGridPosition();
					tendril[numTargets].end = obj->GetPosition();
					tendril[numTargets].start = GetPosition();;

					tendril[numTargets].targetID = info.missionID;

					tendril[numTargets].Init();

					++numTargets;
				}
			}
		}
		else if(info.flags & TERRAIN_FIELD)
		{
			IBaseObject * obj = OBJLIST->FindObject(info.missionID);
			if(obj && obj->objClass == OC_MINEFIELD)
			{
				OBJPTR<IWeaponTarget> targ;
				obj->QueryInterface(IWeaponTargetID,targ);
				if(targ)
				{
					targ->ApplyAOEDamage(ownerID,1);
				}
			}
		}
	}
	return true;
}
//----------------------------------------------------------------------------------
//
BOOL32 StasisBolt::Update (void)
{
	if(numTargets && ((~targetsHeld) & (~(0xFFFFFFFF << numTargets))))
	{
		for(U32 i = 0; i < numTargets; ++ i)
		{
			if(!(targetsHeld & (0x01 << i)))
			{
				IBaseObject * obj = OBJLIST->FindObject(targets[i]);
				if (obj && (!(obj->effectFlags.bStasis)) && obj->GetSystemID() == systemID)
				{
					obj->effectFlags.bStasis = true;
					targetsHeld |= (0x01 << i);
					//CQTRACE12("\nStasis add: ox%x target:0x%x\n", dwMissionID,obj->GetPartID());
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
	if( time > EFFECT_TIME && time <= STASIS_TIMOUT)
	{
		if(THEMATRIX->IsMaster() && !bFreeTargets)
		{
			COMPTR<ITerrainMap> map;
			SECTOR->GetTerrainMap(systemID,map);

			for(U32 count = 0; count < numSquares; ++count)
			{
				map->TestSegment(gvec[count],gvec[count],this);
			}
		}
	}
	return result;
}

void StasisBolt::renderSoftwareRing ()
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
		
		SetupDiffuseBlend(stasisBoltMesh.baseTexID,FALSE);
		
		for (int pass=0;pass<2;pass++)
		{
			if (pass)
			{
				trans.d[0][2] *= -1;
				trans.d[1][2] *= -1;
				trans.d[2][2] *= -1;
				CAMERA->SetModelView(&trans);
			}

			int i;
			for(i = 0; i < STASIS_SEGMENTS; ++i)
			{
				PB.Color4ub(255,255,255,alpha);
				if(i%2)
					PB.TexCoord2f(0.16666666,0.75);
				else
					PB.TexCoord2f(0.5,0.75);
				PB.Vertex3f(stasisBoltMesh.ringCenter[i].x,stasisBoltMesh.ringCenter[i].y,stasisBoltMesh.ringCenter[i].z);
				
				PB.Color4ub(255,255,255,0);
				if(i%2)
					PB.TexCoord2f(0,0.5);
				else
					PB.TexCoord2f(0.33333333,0.5);
				PB.Vertex3f(stasisBoltMesh.upperRing[i].x,stasisBoltMesh.upperRing[i].y,(stasisBoltMesh.upperRing[i].z));
			}
			PB.Color4ub(255,255,255,alpha);
			if(STASIS_SEGMENTS%2)
				PB.TexCoord2f(0.16666666,0.75);
			else
				PB.TexCoord2f(0.5,0.75);
			PB.Vertex3f(stasisBoltMesh.ringCenter[0].x,stasisBoltMesh.ringCenter[0].y,stasisBoltMesh.ringCenter[0].z);
			
			PB.Color4ub(255,255,255,0);
			if(STASIS_SEGMENTS%2)
				PB.TexCoord2f(0,0.5);
			else
				PB.TexCoord2f(0.33333333,0.5);
			PB.Vertex3f(stasisBoltMesh.upperRing[0].x,stasisBoltMesh.upperRing[0].y,(stasisBoltMesh.upperRing[0].z));
		}
	}
}
//----------------------------------------------------------------------------------
//
void StasisBolt::renderRing ()
{
	if(time <=EFFECT_TIME)
	{
		if (CQRENDERFLAGS.bSoftwareRenderer)
		{
			renderSoftwareRing();
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
			BATCH->set_texture_stage_texture( 0, stasisBoltMesh.baseTexID );
			BATCH->set_texture_stage_texture( 1, stasisBoltMesh.moveTexID );
			
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
			SetupDiffuseBlend(stasisBoltMesh.baseTexID,FALSE);
		}

		Vertex2 *vb_data;
		U32 dwSize;
		GENRESULT result;
		result = PIPE->lock_vertex_buffer( stasisBoltMesh.vb_handle, DDLOCK_WRITEONLY, (void **)&vb_data, &dwSize );
		CQASSERT(result == GR_OK);
		
		int i;
		for(i = 0; i < STASIS_SEGMENTS; ++i)
		{
			vb_data[i*2].color = (alpha<<24) | 0x00ffffff;
			vb_data[i*2].v2 = i+timeDif;
	
			vb_data[i*2+1].v2 = i+timeDif2;
		}
		vb_data[STASIS_SEGMENTS*2].color = (alpha<<24) | 0x00ffffff;
		vb_data[STASIS_SEGMENTS*2].v2 = STASIS_SEGMENTS+timeDif;
		vb_data[STASIS_SEGMENTS*2+1].v2 = STASIS_SEGMENTS+timeDif2;
		
		result = PIPE->unlock_vertex_buffer( stasisBoltMesh.vb_handle );
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
			
			result = PIPE->draw_primitive_vb( D3DPT_TRIANGLESTRIP, stasisBoltMesh.vb_handle, 0, STASIS_SEGMENTS*2+2, 0 );
			CQASSERT(result == GR_OK);
		}

	/*	PB.Begin(PB_TRIANGLE_STRIP);
		int i;
		if(multiStages == 1)
		{
			for(i = 0; i < STASIS_SEGMENTS; ++i)
			{
				PB.Color4ub(255,255,255,alpha);
				if(i%2)
					PB.MulCoord2f(0.16666666,0.75);
				else
					PB.MulCoord2f(0.5,0.75);
				PB.TexCoord2f(i,i+timeDif);
				PB.Vertex3f(stasisBoltMesh.ringCenter[i].x,stasisBoltMesh.ringCenter[i].y,stasisBoltMesh.ringCenter[i].z);

				PB.Color4ub(255,255,255,0.5);
				if(i%2)
					PB.MulCoord2f(0,0.5);
				else
					PB.MulCoord2f(0.33333333,0.5);
				PB.TexCoord2f(i+0.5,i+timeDif2);
				PB.Vertex3f(stasisBoltMesh.upperRing[i].x,stasisBoltMesh.upperRing[i].y,-(stasisBoltMesh.upperRing[i].z));
			}
			PB.Color4ub(255,255,255,alpha);
			if(STASIS_SEGMENTS%2)
				PB.MulCoord2f(0.16666666,0.75);
			else
				PB.MulCoord2f(0.5,0.75);
			PB.TexCoord2f(STASIS_SEGMENTS,STASIS_SEGMENTS+timeDif);
			PB.Vertex3f(stasisBoltMesh.ringCenter[0].x,stasisBoltMesh.ringCenter[0].y,stasisBoltMesh.ringCenter[0].z);

			PB.Color4ub(255,255,255,0);
			if(STASIS_SEGMENTS%2)
				PB.MulCoord2f(0,0.5);
			else
				PB.MulCoord2f(0.33333333,0.5);
			PB.TexCoord2f(STASIS_SEGMENTS+0.5,STASIS_SEGMENTS+timeDif2);
			PB.Vertex3f(stasisBoltMesh.upperRing[0].x,stasisBoltMesh.upperRing[0].y,-(stasisBoltMesh.upperRing[0].z));
		}
			PB.End();*/
	}
}
//----------------------------------------------------------------------------------
//
void StasisBolt::renderAnm()
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
		for(U32 anmCnt = 0; anmCnt < STASIS_ANM_COUNT; ++anmCnt)
		{
			anm[anmCnt].SetWidth(anmSize);
			anm[anmCnt].SetColor(255,255,255,anmAlpha);
			ANIM2D->render(&(anm[anmCnt]));
		}
	}

}
//----------------------------------------------------------------------------------
//
void StasisBolt::renderSpark (void)
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

	Vector clipPoints[MAX_CLIP_POINTS];
	U32 numClipPoints = 0;

	findClipingFields(numClipPoints,clipPoints);

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

	if(CQEFFECTS.bExpensiveTerrain==0)
	{
		BATCH->set_state(RPR_BATCH,false);
		DisableTextures();
		CAMERA->SetModelView();
		BATCH->set_render_state(D3DRS_ZENABLE,FALSE);
		BATCH->set_render_state(D3DRS_ZWRITEENABLE,FALSE);
		BATCH->set_render_state(D3DRS_ALPHABLENDENABLE,FALSE);
		TRANSFORM camTrans;
		camTrans.translation = transform.translation;
		CAMERA->SetModelView(&camTrans);
			
		PB.Begin(PB_POINTS);

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
			PB.Color3ub(100,255,100);
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
					base = stasisBoltMesh.sparks1[spCount]*rowDist;
				}
				else
				{
					base = stasisBoltMesh.sparks2[spCount]*rowDist;
				}
				Vector test = base+transform.translation;
				if(!numClipPoints || !clipsFields(test,numClipPoints,clipPoints))
				{
					v[0] = base+realI+realJ;
					v[1] = base-realI+realJ;
					v[2] = base-realI-realJ;
					v[3] = base+realI-realJ;
				
					PB.Vertex3f(v[0].x,v[0].y,v[0].z);
					PB.Vertex3f(v[1].x,v[1].y,v[1].z);
					PB.Vertex3f(v[2].x,v[2].y,v[2].z);
					PB.Vertex3f(v[3].x,v[3].y,v[3].z);
				}
			}

			rowDist += SPARK_SPACING;
			++currentRow;
		}

		PB.End();

	}
	else
	{
		BATCH->set_state(RPR_BATCH,TRUE);
		SetupDiffuseBlend(stasisBoltMesh.baseTexID,FALSE);
		BATCH->set_render_state(D3DRS_ZENABLE,TRUE);
		BATCH->set_render_state(D3DRS_ZWRITEENABLE,FALSE);
		BATCH->set_render_state(D3DRS_ALPHABLENDENABLE,TRUE);
		BATCH->set_render_state(D3DRS_SRCBLEND,D3DBLEND_SRCALPHA);
		BATCH->set_render_state(D3DRS_DESTBLEND,D3DBLEND_INVSRCALPHA);

		TRANSFORM camTrans;
		camTrans.translation = transform.translation;
		CAMERA->SetModelView(&camTrans);
			
		BATCH->set_state(RPR_STATE_ID,stasisBoltMesh.baseTexID);

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
					base = stasisBoltMesh.sparks1[spCount]*rowDist;
				}
				else
				{
					base = stasisBoltMesh.sparks2[spCount]*rowDist;
				}
				Vector test = base+transform.translation;
				if(!numClipPoints || !clipsFields(test,numClipPoints,clipPoints))
				{
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
			}

			rowDist += SPARK_SPACING;
			++currentRow;
		}

		PB.End();

		BATCH->set_state(RPR_STATE_ID,0);
	}
}
//----------------------------------------------------------------------------------
//
void StasisBolt::Render (void)
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

		}
		else
		{
			renderSpark();

			renderRing();

			renderAnm();


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
			for(U32 i = 0 ; i < numTargets; ++i)
			{
				tendril[i].Render(alpha);
			}
		}
	}
}
//----------------------------------------------------------------------------------
//
void StasisBolt::InitRecon(IReconLauncher * _ownerLauncher, U32 _dwMissionID)
{
	_ownerLauncher->QueryInterface(IReconLauncherID,ownerLauncher,NONSYSVOLATILEPTR);
	dwMissionID = _dwMissionID;
	
	playerID = MGlobals::GetPlayerFromPartID(dwMissionID);
	sprintf(partName,"StasisBolt 0x%x",dwMissionID);

	OBJLIST->AddPartID(this, dwMissionID);
	OBJLIST->AddObject(this);

	bGone = true;
	SetReady(false);
	numTargets = lastSent = 0;
}
//----------------------------------------------------------------------------------
//
void StasisBolt::ResolveRecon(IBaseObject * _ownerLauncher)
{
	_ownerLauncher->QueryInterface(IReconLauncherID,ownerLauncher,NONSYSVOLATILEPTR);
}
//----------------------------------------------------------------------------------
//
void StasisBolt::LaunchProbe (IBaseObject * _owner, const class TRANSFORM & orientation, const class Vector * pos,
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

	_owner->QueryInterface(IBaseObjectID, owner, NONSYSVOLATILEPTR);

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

	systemID = owner->GetSystemID();
	ownerID = owner->GetPartID();
	U32 visFlags = owner->GetVisibilityFlags();
	numSquares = 0;

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

	GRIDVECTOR baseVectPrime;
	if(target)
		baseVectPrime = SECTOR->GetJumpgateDestination(target)->GetGridPosition();
	else
		baseVectPrime = targetPos;
	Vector baseVect = baseVectPrime;
	baseVect.x -= data->explosionRange*GRIDSIZE;
	baseVect.y -= data->explosionRange*GRIDSIZE;
	Vector testVect = baseVect;
	while(testVect.x < baseVect.x+(data->explosionRange*GRIDSIZE*2))
	{
		Vector testYVect = testVect;
		while(testYVect.y < baseVect.y+(data->explosionRange*GRIDSIZE*2))
		{
			GRIDVECTOR gridPos;
			gridPos = testYVect;
			if(gridPos-baseVectPrime < data->explosionRange)
			{
				CQASSERT(numSquares < MAX_STASIS_SQUARES);
				gvec[numSquares] = gridPos;
				++numSquares;
			}
			testYVect.y += GRIDSIZE;
		}
		testVect.x += GRIDSIZE;
	}

	Vector anmPos;
	if(target)
		anmPos = SECTOR->GetJumpgateDestination(target)->GetGridPosition();
	else
		anmPos = targetPos;
	for(U32 anmCnt = 0; anmCnt < STASIS_ANM_COUNT; ++anmCnt)
	{
		Vector pos(anmPos.x+(((rand()%2000)/1000.0)-1.0)*INNER_RING,
			anmPos.y+(((rand()%2000)/1000.0)-1.0)*INNER_RING,
			anmPos.z+(((rand()%2000)/1000.0)-1.0)*INNER_RING);
		anm[anmCnt].SetPosition(pos);
		anm[anmCnt].Randomize();
	}

	bGone = false;
	bDeleteRequested = false;
	SetReady(true);

	objMapSystemID = systemID;
	objMapSquare = OBJMAP->GetMapSquare(objMapSystemID,transform.translation);
	OBJMAP->AddObjectToMap(this,systemID,objMapSquare);
}
//----------------------------------------------------------------------------------
//
void StasisBolt::ExplodeProbe()
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
void StasisBolt::DeleteProbe()
{
	bLauncherDelete = true;
}

//----------------------------------------------------------------------------------
//
bool StasisBolt::IsActive()
{
	return !bGone;
}
//---------------------------------------------------------------------------
//
BOOL32 StasisBolt::Save (struct IFileSystem * inFile)
{
	DAFILEDESC fdesc = "STASISBOLT_SAVELOAD";
	COMPTR<IFileSystem> file;
	BOOL32 result = 0;
	DWORD dwWritten;
	STASISBOLT_SAVELOAD save;

	fdesc.lpImplementation = "DOS";
	fdesc.dwDesiredAccess = GENERIC_READ|GENERIC_WRITE;
	fdesc.dwShareMode = 0;  // no sharing
	fdesc.dwCreationDistribution = CREATE_ALWAYS;

	if (inFile->CreateInstance(&fdesc, file) != GR_OK)
		goto Done;

	memset(&save, 0, sizeof(save));

	memcpy(&save, static_cast<BASE_STASISBOLT_SAVELOAD *>(this), sizeof(BASE_STASISBOLT_SAVELOAD));

	FRAME_save(save);

	file->WriteFile(0, &save, sizeof(save), &dwWritten, 0);

	result = 1;

Done:	
	return result;
}
//---------------------------------------------------------------------------
//
BOOL32 StasisBolt::Load (struct IFileSystem * inFile)
{	
	DAFILEDESC fdesc = "STASISBOLT_SAVELOAD";
	COMPTR<IFileSystem> file;
	BOOL32 result = 0;
	DWORD dwRead;
	STASISBOLT_SAVELOAD load;
	U8 buffer[1024];
	Vector anmPos;
	
	fdesc.lpImplementation = "DOS";
	if (inFile->CreateInstance(&fdesc, file) != GR_OK)
		goto Done;

	if (file->ReadFile(0, buffer, sizeof(buffer), &dwRead, 0) == 0)
		goto Done;
	MISSION->CorrelateSymbol("STASISBOLT_SAVELOAD", buffer, &load);

	FRAME_load(load);

	*static_cast<BASE_STASISBOLT_SAVELOAD *>(this) = load;

	result = 1;

Done:	
	return result;
}
//---------------------------------------------------------------------------
//
void StasisBolt::ResolveAssociations()
{
	U32 i;
	OBJLIST->FindObject(ownerID, NONSYSVOLATILEPTR, owner, IBaseObjectID);

	for(i = 0; i < numTargets; ++i)
	{
		IBaseObject * obj = OBJLIST->FindObject(targets[i]);

		if(obj)
		{
			if (obj->QueryInterface(IWeaponTargetID,tendril[ i].target, GetPlayerID()))
			{
	//			CQASSERT(tendril[i].target.ptr->GetSystemID() == systemID);

				getBase()->QueryInterface(IBaseObjectID, tendril[i].source, NONSYSVOLATILEPTR);

				tendril[i].length = GetGridPosition()-obj->GetGridPosition();
				tendril[i].end = obj->GetPosition();
				tendril[i].start = GetPosition();;

				tendril[i].targetID = targets[i];

				tendril[i].Init();
			}
		}
	}

	if(targetID)
		OBJLIST->FindObject(targetID, NONSYSVOLATILEPTR, target, IBaseObjectID);


	Vector anmPos;
	if(target)
		anmPos = SECTOR->GetJumpgateDestination(target)->GetGridPosition();
	else
		anmPos = targetPos;
	for(U32 anmCnt = 0; anmCnt < STASIS_ANM_COUNT; ++anmCnt)
	{
		Vector pos(anmPos.x+(((rand()%2000)/1000.0)-1.0)*INNER_RING,
			anmPos.y+(((rand()%2000)/1000.0)-1.0)*INNER_RING,
			anmPos.z+(((rand()%2000)/1000.0)-1.0)*INNER_RING);
		anm[anmCnt].SetPosition(pos);
		anm[anmCnt].Randomize();
	}

	objMapSystemID = systemID;
	if(systemID)
	{
		objMapSquare = OBJMAP->GetMapSquare(systemID,transform.translation);
		OBJMAP->AddObjectToMap(this,systemID,objMapSquare);
	}
}
//---------------------------------------------------------------------------
//
U32 StasisBolt::getSyncData (void * buffer)
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
					{
						targ->effectFlags.bStasis=false;
						//CQTRACE12("\nStasis remove: ox%x target:0x%x\n", dwMissionID,targ->GetPartID());
					}
				}
			}
			lastSent = numTargets = 0;
		}
		bFreeTargets = false;
		((U8*) buffer)[0] = 2;
		//CQTRACE11("Stasis Sending free: %x\n", dwMissionID);
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
					{
						targ->effectFlags.bStasis=false;
						//CQTRACE12("\nStasis remove: ox%x target:0x%x\n", dwMissionID,targ->GetPartID());
					}
				}
			}
			lastSent = numTargets = 0;
		}
		bLauncherDelete = false;
		bNoMoreSync = true;
		OBJLIST->DeferredDestruction(dwMissionID);
		((U8*) buffer)[0] = 1;
		//CQTRACE11("Stasis Sending delete: %x\n", dwMissionID);
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
				{
					targ->effectFlags.bStasis=false;
					//CQTRACE12("\nStasis remove: ox%x target:0x%x\n", dwMissionID,targ->GetPartID());
				}
			}
		}
		bDeleteRequested = true;
		lastSent = numTargets = 0;
		((U8*) buffer)[0] = 0;
		//CQTRACE11("Stasis Sending end: %x\n", dwMissionID);
		return 1;
	}
	U32 result = 0;
	if(numTargets > lastSent)
	{
		result = sizeof(U32)*(numTargets-lastSent);
		//CQTRACE14("Stasis Sending newTargts: %x numTargets: %d lastSent: %d size : %d\n", dwMissionID,numTargets,lastSent,result);
		memcpy(buffer,(void *)(&(targets[lastSent])),result);
		lastSent = numTargets;
	}
	return result;
}
//---------------------------------------------------------------------------
//
void StasisBolt::putSyncData (void * buffer, U32 bufferSize, bool bLateDelivery)
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
						IBaseObject * targ = OBJLIST->FindObject(targets[i]);
						if(targ)
						{
							targ->effectFlags.bStasis=false;
							//CQTRACE12("\nStasis remove: ox%x target:0x%x\n", dwMissionID,targ->GetPartID());
						}
					}
				}
				lastSent = numTargets = 0;
			}
			bLauncherDelete = false;
			bNoMoreSync = true;
			OBJLIST->DeferredDestruction(dwMissionID);
			//CQTRACE11("Stasis Recieving delete: %x\n", dwMissionID);
		}
		else if(val ==2)
		{
			if(numTargets)
			{
				for(U32 i = 0; i < numTargets; ++i)
				{
					if(targetsHeld & (0x01 << i))
					{
						IBaseObject * targ = OBJLIST->FindObject(targets[i]);
						if(targ)
						{
							targ->effectFlags.bStasis=false;
							//CQTRACE12("\nStasis remove: ox%x target:0x%x\n", dwMissionID,targ->GetPartID());
						}
					}
				}
				lastSent = numTargets = 0;
			}
			//CQTRACE11("Stasis Recieving free: %x\n", dwMissionID);
			bFreeTargets = false;
		}
		else if(val == 0)
		{
			if(numTargets)
			{
				for(U32 i = 0; i < numTargets; ++i)
				{
					if(targetsHeld & (0x01 << i))
					{
						IBaseObject * targ = OBJLIST->FindObject(targets[i]);
						if(targ)
						{
							targ->effectFlags.bStasis=false;
							//CQTRACE12("\nStasis remove: ox%x target:0x%x\n", dwMissionID,targ->GetPartID());
						}
					}
				}
			}
			lastSent = numTargets = 0;
			//CQTRACE11("Stasis Recieving end: %x\n", dwMissionID);
			bDeleteRequested = true;
		}
	}
	else
	{
		U32 * buf = (U32 *) buffer;
		U32 newTarg = bufferSize/sizeof(U32);
		//CQTRACE14("Stasis Recieving newTargets: %x  numTargets:%d newTargets:%d size:%d\n", dwMissionID,numTargets,newTarg,bufferSize);
		CQASSERT(newTarg + numTargets <= MAX_STASIS_TARGETS);
		for(U32 count = 0; count < newTarg; ++count)
		{
			targets[numTargets] = buf[count];

			IBaseObject * obj = OBJLIST->FindObject(buf[count]);
			if(obj)
			{
				if (obj->QueryInterface(IWeaponTargetID,tendril[numTargets].target, GetPlayerID()))
				{
	//				CQASSERT(tendril[numTargets].target.ptr->GetSystemID() == systemID);

					getBase()->QueryInterface(IBaseObjectID, tendril[numTargets].source, NONSYSVOLATILEPTR);

					tendril[numTargets].length = GetGridPosition()-obj->GetGridPosition();
					tendril[numTargets].end = obj->GetPosition();
					tendril[numTargets].start = GetPosition();;

					tendril[numTargets].targetID = buf[count];

					tendril[numTargets].Init();
				}
			}
	
			++numTargets;

			if (obj)
			{
				if((!obj->effectFlags.bStasis) && obj->GetSystemID() == systemID)
				{
					obj->effectFlags.bStasis = true;
					targetsHeld |= (0x01 << (numTargets-1));
					//CQTRACE12("\nStasis add: ox%x target:0x%x\n", dwMissionID,obj->GetPartID());

				}
				else
				{
					targetsHeld &= (~(0x01 << (numTargets-1)));
				}
			}
		}
		lastSent = numTargets;
	}
}
//---------------------------------------------------------------------------
//
void StasisBolt::init (STASISBOLT_INIT &initData)
{
	FRAME_init(initData);
	data = initData.pData;

	objMapSquare = 0;
	objMapSystemID = 0;

	CQASSERT(data);
	CQASSERT(data->wpnClass == WPN_STASISBOLT);
	CQASSERT(data->objClass == OC_WEAPON);

	for(U32 anmCnt = 0; anmCnt < STASIS_ANM_COUNT; ++anmCnt)
	{
		anm[anmCnt].Init(initData.stasisAnm);
		anm[anmCnt].delay = 0;
		anm[anmCnt].SetRotation(((rand()%1000)/1000.0) * 2*PI);
		anm[anmCnt].SetWidth(INNER_RING*2);
		anm[anmCnt].loop = TRUE;
	}

	pArchetype = initData.pArchetype;
	objClass = OC_WEAPON;
	bNoMoreSync = false;
	bDeleteRequested = true;
	bGone = true;
}
//------------------------------------------------------------------------------------------
//---------------------------StasisBolt Factory----------------------------------------------
//------------------------------------------------------------------------------------------
//
struct DACOM_NO_VTABLE StasisBoltFactory : public IObjectFactory
{
	struct OBJTYPE : STASISBOLT_INIT
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
			if(stasisAnm)
				delete stasisAnm;
		}
	};
	//------------------------------------------------------------------------------------------
	//
	U32 factoryHandle;		// handles to callback

	//
	// Interface mapping
	//

	BEGIN_DACOM_MAP_INBOUND(StasisBoltFactory)
	DACOM_INTERFACE_ENTRY(IObjectFactory)
	END_DACOM_MAP()

	StasisBoltFactory (void) { }

	~StasisBoltFactory (void);

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

	// StasisBoltFactory methods 

	IDAComponent * getBase (void)
	{
		return static_cast<IObjectFactory *> (this);
	}
};
//--------------------------------------------------------------------------//
//
StasisBoltFactory::~StasisBoltFactory (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (OBJLIST && OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
		connection->Unadvise(factoryHandle);
}
//--------------------------------------------------------------------------//
//
void StasisBoltFactory::init (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
		connection->Advise(getBase(), &factoryHandle);
}
//-----------------------------------------------------------------------------
//
HANDLE StasisBoltFactory::CreateArchetype (const char *szArchname, OBJCLASS objClass, void *_data)
{
	OBJTYPE * result = 0;

	if (objClass == OC_WEAPON)
	{
		BT_STASISBOLT_DATA * data = (BT_STASISBOLT_DATA *)_data;
		if (data->wpnClass == WPN_STASISBOLT)
		{
			result = new OBJTYPE;
			
			result->pArchetype = ARCHLIST->GetArchetype(szArchname);
			result->pData = data;
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

			if ((result->archIndex = ENGINE->create_archetype(fdesc.lpFileName, objFile)) == INVALID_ARCHETYPE_INDEX)
				goto Error;

			//build the global mesh
			U32 i;
			for(i = 0; i < STASIS_SEGMENTS; ++i)
			{
				stasisBoltMesh.ringCenter[i] = Vector(cos((2*PI*i)/STASIS_SEGMENTS),sin((2*PI*i)/STASIS_SEGMENTS),0);
				stasisBoltMesh.upperRing[i] = Vector(cos((2*PI*(i+0.5))/STASIS_SEGMENTS)*0.6,sin((2*PI*(i+0.5))/STASIS_SEGMENTS)*0.6,0.4);
			}

			stasisBoltMesh.RestoreVertexBuffers();

			SINGLE offAngle = (2*PI)/(NUM_SPARK_RING*2);
			for(i = 0; i < NUM_SPARK_RING; ++i)
			{
				stasisBoltMesh.sparks1[i] = Vector(cos((2*PI*i)/NUM_SPARK_RING),sin((2*PI*i)/NUM_SPARK_RING),0);
				stasisBoltMesh.sparks2[i] = Vector(cos((2*PI*i+offAngle)/NUM_SPARK_RING),sin((2*PI*i+offAngle)/NUM_SPARK_RING),0);
			}

			stasisBoltMesh.baseTexID = TMANAGER->CreateTextureFromFile("stasis_multimap.tga", TEXTURESDIR, DA::TGA,PF_4CC_DAA4);
			if (CQRENDERFLAGS.bMultiTexture)
				stasisBoltMesh.moveTexID = TMANAGER->CreateTextureFromFile("fractal_tile.tga", TEXTURESDIR, DA::TGA,PF_RGB5_A1);
			else
				stasisBoltMesh.moveTexID = 0;

			DAFILEDESC anmdesc;
			COMPTR<IFileSystem> anmFile;
			anmdesc.lpFileName = "stasis.anm";
			if (OBJECTDIR->CreateInstance(&anmdesc, anmFile) == GR_OK)
			{
				result->stasisAnm = ANIM2D->create_archetype(anmFile);
			}

			goto Done;
		}
	}

Error:
	delete result;
	result = 0;
Done:
	return (HANDLE) result;
}
//-------------------------------------------------------------------
//
BOOL32 StasisBoltFactory::DestroyArchetype (HANDLE hArchetype)
{
	OBJTYPE * objtype = (OBJTYPE *)hArchetype;

	TMANAGER->ReleaseTextureRef(stasisBoltMesh.baseTexID);
	if (stasisBoltMesh.moveTexID)
		TMANAGER->ReleaseTextureRef(stasisBoltMesh.moveTexID);
	if (stasisBoltMesh.vb_handle)
	{
		PIPE->destroy_vertex_buffer(stasisBoltMesh.vb_handle);
		stasisBoltMesh.vb_handle = 0;
	}

	delete objtype;

	return 1;
}
//-------------------------------------------------------------------
//
IBaseObject * StasisBoltFactory::CreateInstance (HANDLE hArchetype)
{
	OBJTYPE * objtype = (OBJTYPE *)hArchetype;

	StasisBolt * stasisBolt = new ObjectImpl<StasisBolt>;

	stasisBolt->init(*objtype);

	return stasisBolt;
}
//-------------------------------------------------------------------
//
void StasisBoltFactory::EditorCreateInstance (HANDLE hArchetype, const MISSION_INFO & info)
{
	// does not support this!
}
//---------------------------------------------------------------------------
//-------------------------GLOBAL STARTUP------------------------------------
//---------------------------------------------------------------------------
//
struct _stasisBolt : GlobalComponent
{
	StasisBoltFactory * sfactory;

	virtual void Startup (void)
	{
		sfactory = new DAComponent<StasisBoltFactory>;
		AddToGlobalCleanupList((IDAComponent **) &sfactory);
	}

	virtual void Initialize (void)
	{
		sfactory->init();
	}
};

static _stasisBolt __stasisBolt;
//---------------------------------------------------------------------------
//------------------------End StasisBolt.cpp----------------------------------------
//---------------------------------------------------------------------------


