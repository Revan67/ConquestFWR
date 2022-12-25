//--------------------------------------------------------------------------//
//                                                                          //
//                               Planetoid.cpp                              //
//                                                                          //
//                  COPYRIGHT (C) 1997 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/Planetoid.cpp 239   5/10/01 1:56p Tmauer $
*/			    
//------------------------------- #INCLUDES --------------------------------//
//--------------------------------------------------------------------------//

#include "pch.h"
#include <globals.h>

#include "MGlobals.h"
#include "FogofWar.h"
#include "TComponent.h"
#include "TObjFrame.h"
#include "TObject.h"
#include "TObjSelect.h"
#include "TObjTrans.h"
#include "TObjMission.h"
#include "TObjEffectTarget.h"
#include "ObjList.h"
#include "Camera.h"
#include "Sector.h"
#include "SuperTrans.h"
#include "UserDefaults.h"
#include "Mission.h"
#include "IMissionActor.h"
#include "IPlanet.h"
#include "Startup.h"
#include "DrawAgent.h"
#include "TerrainMap.h"
#include "Anim2d.h"
#include "ILight.h"
#include "CQLight.h"
#include "MyVertex.h"
#include "OpAgent.h"
#include "FogOfWar.h"
#include "CQGame.h"
#include "SysMap.h"
#include "TManager.h"
#include <DQuickSave.h>
#include <DPlatform.h>
#include <DPlanet.h>
#include "ObjMap.h"
#include "IUnbornMeshList.h"
#include "IShapeLoader.h"
#include "IFabricator.h"
#include "MPart.h"
#include "IVertexBuffer.h"

//test stuff
#include <Mesh.h>
#include <Material.h>
#include <DEffectOpts.h>

#include <WindowManager.h>
#include <Engine.h>
#include <TSmartPointer.h>
#include <3DMath.h>
#include <FileSys.h>
#include <IConnection.h>
#include <IRenderPrimitive.h>
#include <IAnim.h>

#include "TObjRender.h"
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
#define GO_LEFT 0
#define GO_UP_LEFT 1
#define GO_UP 2
#define GO_UP_RIGHT 3
#define GO_RIGHT 4
#define GO_DOWN_RIGHT 5
#define GO_DOWN 6
#define GO_DOWN_LEFT 7

#define CLICK_TIME 0.2

static INSTANCE_INDEX slot = INVALID_INSTANCE_INDEX,circle = INVALID_INSTANCE_INDEX;
static TRANSFORM baseSlotTrans;
static Material *slot_mat,*circle_mat;
static S32 mat_cnt;
S32 camMoved=0;
static U32 PLANETTEXMEMUSED=0;

PGENTYPE pPlanetFontType;
PGENTYPE pPlanetFontType2;
COMPTR<IFontDrawAgent> pPlanetFont1;
COMPTR<IFontDrawAgent> pPlanetFont2;
COMPTR<IFontDrawAgent> pPlanetFont3;
COMPTR<IFontDrawAgent> pPlanetFont4;
COMPTR<IFontDrawAgent> pPlanetFont5;
COMPTR<IFontDrawAgent> pPlanetFont6;
COMPTR<IFontDrawAgent> pPlanetFont7;
COMPTR<IFontDrawAgent> pPlanetFont8;

PGENTYPE pPlanetShapeType;
COMPTR<IDrawAgent> planetShape[4];
COMPTR<IDrawAgent> metalShape;
COMPTR<IDrawAgent> gasShape;
COMPTR<IDrawAgent> crewShape;
COMPTR<IDrawAgent> t_backgroundShape, m_backgroundShape, s_backgroundShape;

SINGLE offGridX = 0;
SINGLE offGridY = 0;

struct PLANET_INIT
{
	const BT_PLANET_DATA * pData;
	PARCHETYPE pArchetype;
	IEffectHandle * ambientEffect;
	IMeshArchetype * meshArch;
	S32 archIndex;
	U32 mapTex;
	AnimArchetype *pointAnimArch;
	Vector rigidBodyArm;

	//render experiment
	RPVertex *st_lists[2];
	U16 index_lists[2][1092];
	Vector *vert_lists[2];
	Vector *norm_lists[2];
	Vector *face_norm_lists[2];
//	U32 refList[2][1092];
	S32 vert_cnt[2];

	U32 ownerTextID;

	U32 sysMapIconID;

	AnimArchetype * teraRingAnim;

	PARCHETYPE teraExplosion;

	~PLANET_INIT();

	void sortVertsOnZ();
};


PLANET_INIT::~PLANET_INIT()
{
	for (int m=0;m<2;m++)
	{
		delete st_lists[m];
		delete vert_lists[m];
		delete norm_lists[m];
		delete face_norm_lists[m];
	}
	TMANAGER->ReleaseTextureRef(ownerTextID);
}

void PLANET_INIT::sortVertsOnZ()
{
	U32 v_sort[400];
	U32 reindex_list[400];
	RPVertex *new_list;
	Vector *new_vert_list;
	Vector *new_norm_list;
	for (int m=0;m<2;m++)
	{
		if (vert_cnt[m])
		{
			new_list = new RPVertex[vert_cnt[m]];
			new_vert_list = new Vector[vert_cnt[m]];
			new_norm_list = new Vector[vert_cnt[m]];
			int v;
			for (v=0;v<vert_cnt[m];v++)
			{
				//massive ftols
				v_sort[v] = -(st_lists[m][v].pos.y+8000.0);//1000.0*atan2(st_lists[m][v].pos.z,st_lists[m][v].pos.x);
			}
			RadixSort::sort(v_sort,vert_cnt[m]);
			for (v=0;v<vert_cnt[m];v++)
			{
				new_list[v] = st_lists[m][RadixSort::index_list[v]];
				new_vert_list[v] = vert_lists[m][RadixSort::index_list[v]];
				new_norm_list[v] = norm_lists[m][RadixSort::index_list[v]];
				reindex_list[RadixSort::index_list[v]] = v;
			}
			for (int i=0;i<1092;i++)
			{
				if (index_lists[m][i] != 0xffff)
				{
					index_lists[m][i] = reindex_list[index_lists[m][i]];
				}
			}
			delete st_lists[m];
			st_lists[m] = new_list;
			delete vert_lists[m];
			vert_lists[m] = new_vert_list;
			delete norm_lists[m];
			norm_lists[m] = new_norm_list;
		}
	}
}

struct TeraPart
{
	Vector pos;
	SINGLE lifetime;
	SINGLE startTime;
	bool bAlive:1;
};

#define MAX_TERA_PARTICLES 1000
#define PLANET_RING_SIZE 2400

struct TeraformParticleRing
{
	bool bInitialize;
	SINGLE frequency;
	SINGLE maxTime;
	SINGLE time;
	SINGLE createTime;
	SINGLE teraExplosionTime;
	AnimArchetype * animArch;
	PARCHETYPE explosionArch;
	U32 systemID;

	U8 red;
	U8 green;
	U8 blue;

	TeraPart particles[MAX_TERA_PARTICLES];

	TeraformParticleRing()
	{
		bInitialize = false;
	}

	void Init(SINGLE freq, SINGLE _maxTime, AnimArchetype * _animArch, U8 _red, U8 _green,U8 _blue,PARCHETYPE _explosionArch, U32 _systemID);
	
	void Render(const Vector & hitDir, const Vector & position);

	void PhysUpdate(SINGLE dt,const Vector & hitDir, const Vector & position);
};

void TeraformParticleRing::Init(SINGLE freq, SINGLE _maxTime, AnimArchetype * _animArch, U8 _red, U8 _green,U8 _blue, PARCHETYPE _explosionArch, U32 _systemID)
{
	if(_animArch)
	{
		bInitialize  = true;
		systemID = _systemID;
		explosionArch = _explosionArch;
		red = _red;
		green = _green;
		blue = _blue;
		animArch = _animArch;
		frequency = freq;
		maxTime = _maxTime;
		time = maxTime*0.05;
		createTime = 0;
		teraExplosionTime = 0;
		for(U32 i = 0; i < MAX_TERA_PARTICLES; ++i)
		{
			particles[i].bAlive = false;
		}
	}
}

void TeraformParticleRing::Render(const Vector & hitDir, const Vector & position)
{
	if(!bInitialize )
		return;
	Vector j(0,0,1);
	Vector i = cross_product(j,hitDir);
	i.fast_normalize();
	j = cross_product(hitDir,i);
	j.fast_normalize();

	TRANSFORM trans;
	trans.set_i(i);
	trans.set_j(j);
	trans.set_k(hitDir);
	trans.translation = position;

	Vector cpos (CAMERA->GetPosition());

	Vector lookAt (CAMERA->GetLookAtPosition());

	Vector k = lookAt-cpos;
	k.fast_normalize();
	
	Vector tmpUp(0,0,1);

	j = Vector(cross_product(k,tmpUp));
	j.fast_normalize();

	i = Vector(cross_product(j,k));
	i.fast_normalize();

	CAMERA->SetPerspective();

	CAMERA->SetModelView();
	
	BATCH->set_state(RPR_BATCH,true);
	BATCH->set_render_state(D3DRS_CULLMODE,D3DCULL_NONE);
	BATCH->set_state(RPR_STATE_ID,animArch->frames[0].texture);
	
	BATCH->set_render_state(D3DRS_ZENABLE,TRUE);
	BATCH->set_render_state(D3DRS_ALPHABLENDENABLE,TRUE);
	BATCH->set_render_state(D3DRS_SRCBLEND,D3DBLEND_ONE);
	BATCH->set_render_state(D3DRS_DESTBLEND,D3DBLEND_ONE);
	BATCH->set_render_state(D3DRS_ZWRITEENABLE,FALSE);
	SetupDiffuseBlend(animArch->frames[0].texture,TRUE);

	PB.Begin(PB_QUADS);
	PB.Color4ub(red,green,blue,255);
	for(U32 counter = 0; counter < MAX_TERA_PARTICLES; ++counter)
	{
		if(particles[counter].bAlive)
		{
			Vector pos = trans.rotate_translate(particles[counter].pos);
			SINGLE size = 500*(1.0-((time - particles[counter].startTime)/(particles[counter].lifetime-particles[counter].startTime)));
	
			U32 totalTimeMS = (animArch->frame_cnt*1000)/animArch->capture_rate;
			U32 frameTimeMS = (time+particles[counter].startTime)*1000;
			frameTimeMS = (frameTimeMS+(F2LONG((counter*1000)*animArch->capture_rate)))%totalTimeMS;
			U32 frame = ((frameTimeMS*animArch->capture_rate)/1000);
			frame = frame%animArch->frame_cnt;//just in case
		
			const AnimFrame* anim = &(animArch->frames[frame]);

			Vector v0 = pos - i*size - j*size;
			Vector v1 = pos + i*size - j*size;
			Vector v2 = pos + i*size + j*size;
			Vector v3 = pos - i*size + j*size;

			PB.TexCoord2f (anim->x0, anim->y0);
			PB.Vertex3f (v0.x, v0.y, v0.z);
			PB.TexCoord2f (anim->x1, anim->y0);
			PB.Vertex3f (v1.x, v1.y, v1.z);
			PB.TexCoord2f (anim->x1, anim->y1);
			PB.Vertex3f (v2.x, v2.y, v2.z);
			PB.TexCoord2f (anim->x0, anim->y1);
			PB.Vertex3f (v3.x, v3.y, v3.z);

		}
	}
	PB.End();
	BATCH->set_state(RPR_BATCH, FALSE);
}

static SINGLE TERRA_EXPLOSION_TIME = 0.5;

void TeraformParticleRing::PhysUpdate(SINGLE dt,const Vector & hitDir, const Vector & position)
{
	if(!bInitialize )
		return;
	time += dt;
	createTime += dt;
	U32 numToCreate = 0;
	if(time < maxTime)
	{
		numToCreate = createTime*frequency;
		createTime = createTime - (numToCreate/frequency);
	}
	SINGLE delta = (((time/maxTime)*2)-1);
	bInitialize = false;
	for(U32 i = 0; i < MAX_TERA_PARTICLES; ++i)
	{
		if(particles[i].bAlive)
		{
			bInitialize = true;
			if(particles[i].lifetime < time)
			{
				particles[i].bAlive = false;
			}
		}
		if(numToCreate && (!particles[i].bAlive))
		{
			bInitialize = true;
			particles[i].bAlive = true;
			particles[i].startTime = time;
			particles[i].lifetime = 1.0f+(((SINGLE)(rand()%10000))/10000.0f)*1.0f+time;
			SINGLE angle = (((SINGLE)(rand()%10000))/10000.0f)*2*PI;
			particles[i].pos.z = PLANET_RING_SIZE *delta;
			SINGLE radius = sqrt((PLANET_RING_SIZE *PLANET_RING_SIZE )-(particles[i].pos.z*particles[i].pos.z));
			particles[i].pos.x = cos(angle)*radius;
			particles[i].pos.y = sin(angle)*radius;
			--numToCreate;
		}
	}

	if(time < maxTime)
	{
		teraExplosionTime += dt;

		while(teraExplosionTime > TERRA_EXPLOSION_TIME)
		{
			teraExplosionTime -= TERRA_EXPLOSION_TIME;
			if(explosionArch)
			{
				Vector j(0,0,1);
				Vector i = cross_product(j,hitDir);
				i.fast_normalize();
				j = cross_product(hitDir,i);
				j.fast_normalize();

				TRANSFORM trans;
				trans.set_i(i);
				trans.set_j(j);
				trans.set_k(hitDir);
				trans.translation = position;

				Vector pos;

				SINGLE angle = (((SINGLE)(rand()%10000))/10000.0f)*2*PI;
				pos.z = PLANET_RING_SIZE *delta;
				SINGLE radius = sqrt((PLANET_RING_SIZE *PLANET_RING_SIZE )-(pos.z*pos.z));
				pos.x = cos(angle)*radius;
				pos.y = sin(angle)*radius;
				pos = trans.rotate_translate(pos);
				trans.set_identity();
				trans.translation = pos;
				IBaseObject * obj = CreateBlast(explosionArch,trans, systemID);
				CQASSERT(obj);
				OBJLIST->AddObject(obj);
			}
		}
	}

}

#define HALO_SEGMENTS 40

struct HaloMesh : IVertexBufferOwner
{
	Vector ringCenter[HALO_SEGMENTS];
	Vector upperRing[HALO_SEGMENTS];
	U32 vb_handle;
	
	virtual void RestoreVertexBuffers();

	HaloMesh()
	{
		vb_handle = 0;
		vb_mgr->Add(this);
	}

	~HaloMesh()
	{
		vb_mgr->Delete(this);
	}

}haloMesh;

void HaloMesh::RestoreVertexBuffers()
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
		result = PIPE->create_vertex_buffer( D3DFVF_RPVERTEX2, HALO_SEGMENTS*2+2, IRP_VBF_SYSTEM, &haloMesh.vb_handle );
		CQASSERT(result == GR_OK);
		result = PIPE->lock_vertex_buffer( haloMesh.vb_handle, DDLOCK_WRITEONLY, (void **)&vb_data, &dwSize );
		CQASSERT(result == GR_OK);
		
		for(int i = 0; i < HALO_SEGMENTS; ++i)
		{
			vb_data[i*2].u2 = i;
			vb_data[i*2].u = i+1;
			vb_data[i*2].v = 1;
			vb_data[i*2].pos = haloMesh.ringCenter[i];
			
			vb_data[i*2+1].color = 0x00ffffff;
			vb_data[i*2+1].u2 = i+0.5f;
			vb_data[i*2+1].u = i+0.5f;
			vb_data[i*2+1].v = 0;
			vb_data[i*2+1].pos = haloMesh.upperRing[i];
		}
		
		vb_data[HALO_SEGMENTS*2].u = HALO_SEGMENTS+1;
		vb_data[HALO_SEGMENTS*2].v = 1;
		vb_data[HALO_SEGMENTS*2].u2 = HALO_SEGMENTS;
		vb_data[HALO_SEGMENTS*2].pos = haloMesh.ringCenter[0];
		
		vb_data[HALO_SEGMENTS*2+1].color = 0x00ffffff;
		vb_data[HALO_SEGMENTS*2+1].u = HALO_SEGMENTS+0.5f;
		vb_data[HALO_SEGMENTS*2+1].v = 0;
		vb_data[HALO_SEGMENTS*2+1].u2 = HALO_SEGMENTS+0.5f;
		vb_data[HALO_SEGMENTS*2+1].pos = haloMesh.upperRing[0];
		
		result = PIPE->unlock_vertex_buffer( haloMesh.vb_handle );
		CQASSERT(result == GR_OK);
	}
	else
		haloMesh.vb_handle = 0;
}

//------------------------------------------------------------------------------------------
//
struct _NO_VTABLE PlanetRing : public IBaseObject
{
	BEGIN_MAP_INBOUND(PlanetRing)
	_INTERFACE_ENTRY(IBaseObject)
	END_MAP()

	struct Planetoid * owner;

	//
	// IBaseObject methods
	//

	virtual const TRANSFORM & GetTransform (void) const;

	virtual Vector GetVelocity (void);

	virtual U32 GetSystemID (void) const;			// current system
	
	virtual bool GetObjectBox (OBJBOX & box) const;  // return false if not supported

	virtual U32 GetPartID (void) const;
};
//------------------------------------------------------------------------------------------
//
struct _NO_VTABLE Planetoid : public ObjectEffectTarget
										<ObjectSelection
											<ObjectMission
												<ObjectTransform
													<ObjectFrame<IBaseObject,PLANET_SAVELOAD,PLANET_INIT> >
												>
											>
										>, 
									ISaveLoad, IQuickSaveLoad, IPlanet, BASE_PLANET_SAVELOAD
{
	BEGIN_MAP_INBOUND(Planetoid)
	_INTERFACE_ENTRY(IBaseObject)
	_INTERFACE_ENTRY(ISaveLoad)
	_INTERFACE_ENTRY(IPlanet)
	_INTERFACE_ENTRY(IMissionActor)
	_INTERFACE_ENTRY(IPhysicalObject)
	_INTERFACE_ENTRY(IQuickSaveLoad)
	_INTERFACE_ENTRY(IEffectTarget)
	END_MAP()

	GeneralSyncNode  genSyncNode;

	//----------------------------------
	// animation index
	//----------------------------------
	S32 ambientAnimIndex;
	//----------------------------------
	S32 textureID;
	U32 highlightedSlot;
	U32 renderHighlightSlot;
//	AnimArchetype *pointAnimArch;
	SINGLE clickTime;
	S32 clickedSlot;
	bool bNoPhysics:1;
	bool bPlanetHighlight:1;
	bool bHasAllHighlightSlots:1;
	bool bMouseOver:1;
	bool bFirstUpdate:1;
	SINGLE alphaMod;
	U32 mapTex;
	U32 multiStages;

	//sync data
	S32 lastMetal,lastCrew,lastGas;

	//test highlight stuff
	float depth, center_x, center_y, radius;

	/////  Mineral planet stuff
	Mesh *sphereMesh;

	//render experiment
	S32 minV[2],maxV[2];
	bool faceRenders[2][364];
	LightRGB lit[2][250];
	S32 cameraMoveCnt;

	//override archtype for teraforming
	PLANET_INIT * renderArch1;
	PLANET_INIT * renderArch2;

	SINGLE teraformTime;
	SINGLE totalTeraformTime;
	Vector teraformHitDir;
	TeraformParticleRing teraRing;

	// planet ring object (for terrain map)
	PlanetRing * planetRing;

	//objmap
	int map_sys,map_square;

	Planetoid (void);

	virtual ~Planetoid (void);	// See ObjList.cpp

	virtual void CastVisibleArea (void);

	virtual SINGLE TestHighlight (const RECT &rect);

	virtual BOOL32 TestSlotHighlight (const S32 &mouseX,const S32 &mouseY);

	virtual void DrawFleetMoniker (bool bAllShips);

	virtual BOOL32 Update (void);

	virtual void PhysicalUpdate (SINGLE dt);

	virtual void Render (void);

//	void RenderPlanetoidWireFrame();

	virtual void MapRender (bool bPing);

	virtual void View (void);

	virtual void DEBUG_print (void) const;
	
	virtual void TestVisible (const USER_DEFAULTS & defaults, const U32 currentSystem, const U32 currentPlayer);	// set bVisible if possible for any part of object to appear

	virtual void UpdateVisibilityFlags (void);

	virtual void UpdateVisibilityFlags2 (void);

	/* ISaveLoad */

	virtual BOOL32 Save (struct IFileSystem * file);

	virtual BOOL32 Load (struct IFileSystem * file);

	virtual void ResolveAssociations();

	/* IQuickSaveLoad methods */
	
	virtual void QuickSave (struct IFileSystem * file);

	virtual void QuickLoad (const char *szSaveLoadType, const char *szInstanceName, void * buffer, U32 bufferSize);

	virtual void QuickResolveAssociations (void);

	/* IPlanet methods */

//	virtual void ActivateGrids(BOOL32 bGrid);

	virtual U32 GetHighlightedSlot();

	virtual TRANSFORM GetSlotTransform(U32 slotNumber);

	virtual void ClickSlot(U32 slotNumber);

	virtual U8 GetMaxSlots()
	{
		if(renderArch1->pData->bMoon)
			return 1;
		return M_MAX_SLOTS;
	}

	virtual U16 AllocateBuildSlot (U32 dwMissionID, U16 dwSlotHandle,bool bFail = false);

	virtual void DeallocateBuildSlot (U32 dwMissionID, U16 dwSlotHandle);

	virtual bool HasOpenSlots (void) const;

	virtual U32 GetSlotUser (U32 slotNumber);
	
	virtual void GetPlanetSlotMissionIDs(U32 * buffer, U32 slotID);

	virtual void CompleteSlotOperations(U32 workingID,U32 targetSlotID);

	virtual	U32 FindBestSlot(PARCHETYPE buildType, const Vector * preferedLoc = NULL);

	virtual bool IsBuildableBy(U32 playerID)
	{
		return true;
	}

	virtual bool MarkSlot(U32 playerID,U32 dwSlotHandle,U32 achtypeID);

	virtual void UnmarkSlot(U32 playerID,U32 dwSlotHandle);

	virtual U32 GetGas()
	{
		return gas;
	}

	virtual void SetGas(U32 newGas)
	{
		gas = newGas;
	}

	virtual U32 GetMetal()
	{
		return metal;
	}

	virtual void SetMetal(U32 newMetal)
	{
		metal = newMetal;
	}

	virtual U32 GetCrew()
	{
		return crew;
	}

	virtual void SetCrew(U32 newCrew)
	{
		crew = newCrew;
	}

	virtual bool IsMouseOver ()
	{
		return bMouseOver;
	}

	virtual bool IsRenderingFaded ()
	{
		return alphaMod < .9;
	}

	virtual bool IsHighlightingBuild();

	virtual void ChangePlayerRates(U32 playerID, SINGLE metalRate, SINGLE gasRate, SINGLE crewRate);

	virtual U32 GetEmptySlots();

	virtual void TeraformPlanet(const char * newArch, SINGLE changeTime, const Vector & hitDir);

	virtual bool IsMoon();

	virtual void BoostRegen(SINGLE boostOre, SINGLE boostGas, SINGLE boostCrew);

	/* IPhysicalObject methods */

	virtual void SetPosition (const Vector & position, U32 newSystemID)
	{
		systemID = newSystemID;
		CQASSERT(systemID);
		// 1st undo any previous footprints
		enableTerrainFootprint(false);

		// now set new footprints
		transform.translation = position;
		enableTerrainFootprint(true);
	}

	virtual void SetTransform (const TRANSFORM & trans, U32 newSystemID)
	{
		systemID = newSystemID;
		CQASSERT(systemID);
		// 1st undo any previous footprints
		enableTerrainFootprint(false);

		// now set new footprints
		transform = trans;
		enableTerrainFootprint(true);
	}

	/* IMissionActor methods */


	/* Planetoid methods */

	void setPhysicsState (bool bEnable)
	{
		if (bEnable == bNoPhysics)		// changing states
		{
			bNoPhysics = !bEnable;
		}
	}

	void enableTerrainFootprint (bool bEnable);

	void updateDisplayValues (void);

	U32 getSyncData (void * buffer);
	void putSyncData (void * buffer, U32 bufferSize, bool bLateDelivery);

	void drawResourceBar(S32 x1,S32 y1,S32 x2,S32 y2,U32 value,U32 max,U32 rate, U32 trueMax);

	void updateHarvestRates();

	void renderHalo(U8 red, U8 green,U8 blue,SINGLE sizeOuter,SINGLE sizeInner);

	void firstUpdate();
};

//---------------------------------------------------------------------------
//
Planetoid::Planetoid (void) :
	genSyncNode(this, SyncGetProc(&Planetoid::getSyncData), SyncPutProc(&Planetoid::putSyncData))
{
	ambientAnimIndex = -1;

	multiStages = 0xffffffff;

	planetRing = new ObjectImpl<PlanetRing>;
	planetRing->owner = this;
	planetRing->objClass = OC_BUILDRING;
	bFirstUpdate = false;
}
//---------------------------------------------------------------------------
//
Planetoid::~Planetoid (void)
{
	enableTerrainFootprint(false);

	ANIM->release_script_inst(ambientAnimIndex);
	ambientAnimIndex = -1;

	delete planetRing;
}
//---------------------------------------------------------------------------------------
//
void Planetoid::CastVisibleArea (void)
{
	if(DEFAULTS->GetDefaults()->fogMode == FOGOWAR_EXPLORED)
	{
		SetVisibleToAllies(0xFF);
	}
	// propogate visibility
//	SetVisibleToAllies(GetVisibilityFlags());
}
//--------------------------------------------------------------------------//
//
void Planetoid::DEBUG_print (void) const
{
#if 0
	const char * debugName;
	if (bHighlight && (debugName = GetDebugName()) != 0)
	{
		Vector point;
		S32 x, y;

		point.x = 0;
		point.y = box[2]+250.0F;	// maxY + 120
		point.z = 0;

		if (CAMERA->PointToScreen(point, &x, &y, &transform)==IN_PANE)
		{
			DEBUGFONT->StringDraw(0, x+20, y, debugName, (hPart.IsHost()) ? RGB_LOCAL : RGB_SHADOW);
		}
	}

#endif
}
//--------------------------------------------------------------------------//
// set bVisible if possible for any part of object to appear
//
void Planetoid::TestVisible (const USER_DEFAULTS & defaults, const U32 currentSystem, const U32 currentPlayer)
{
	bVisible = (GetSystemID() == currentSystem && ((GetVisibilityFlags()&MGlobals::GetAllyMask(currentPlayer))!=0 || defaults.bEditorMode || defaults.bVisibilityRulesOff));
	if (bVisible)
	{
	//	bVisible = REND->get_instance_projected_bounding_sphere(instanceIndex, MAINCAM, LODPERCENT, center_x, center_y, radius, depth);
		if(IsMoon())
            bVisible = CAMERA->SphereInFrustrum(transform.translation,1000,center_x, center_y, radius, depth);
		else
            bVisible = CAMERA->SphereInFrustrum(transform.translation,2500,center_x, center_y, radius, depth);

	}
}
//--------------------------------------------------------------------------
//
void Planetoid::UpdateVisibilityFlags (void)
{
	U8 newShadowVisibilityFlags;
	newShadowVisibilityFlags = GetVisibilityFlags() & (~GetPendingVisibilityFlags());
	if(newShadowVisibilityFlags != shadowVisibilityFlags)
	{
		U32 newFlags = newShadowVisibilityFlags & (~shadowVisibilityFlags);
		if(newFlags)
		{
			//record state
			for(U32 i = 0; i < MAX_PLAYERS; ++i)
			{
				if((newFlags >> i) & 0x01)
				{
					shadowGas[i] = gas;
					shadowMetal[i] = metal;
					shadowCrew[i] = crew;
				}
			}
		}
		U32 oldFlags = (~newShadowVisibilityFlags) & shadowVisibilityFlags;
		if(oldFlags)
		{
			//update marks
			for(U32 i = 0; i < MAX_PLAYERS; ++i)
			{
				if((oldFlags >> i) & 0x01)
				{
					slotMarks[i] = 0;
					for(U32 j = 0 ; j < M_MAX_SLOTS; ++j)
					{
						slotMarks[i] |= ((slotUser[j] != 0) << j);
					}
					trueMarks[i] = slotMarks[i]|mySlotMarks[i];
				}
			}
		}
		shadowVisibilityFlags = newShadowVisibilityFlags;
	}
	IBaseObject::UpdateVisibilityFlags2();//calling 2 on purpose;
}
//---------------------------------------------------------------------------
//
void Planetoid::UpdateVisibilityFlags2 (void)
{
	U8 newShadowVisibilityFlags;
	newShadowVisibilityFlags = GetVisibilityFlags() & (~GetPendingVisibilityFlags());
	if(newShadowVisibilityFlags != shadowVisibilityFlags)
	{
		U32 newFlags = newShadowVisibilityFlags & (~shadowVisibilityFlags);
		if(newFlags)
		{
			//record state
			for(U32 i = 0; i < MAX_PLAYERS; ++i)
			{
				if((newFlags >> i) & 0x01)
				{
					shadowGas[i] = gas;
					shadowMetal[i] = metal;
					shadowCrew[i] = crew;
				}
			}
		}
		U32 oldFlags = (~newShadowVisibilityFlags) & shadowVisibilityFlags;
		if(oldFlags)
		{
			//update marks
			for(U32 i = 0; i < MAX_PLAYERS; ++i)
			{
				if((oldFlags >> i) & 0x01)
				{
					slotMarks[i] = 0;
					for(U32 j = 0 ; j < M_MAX_SLOTS; ++j)
					{
						slotMarks[i] |= ((slotUser[j] != 0) << j);
					}
					trueMarks[i] = slotMarks[i]|mySlotMarks[i];
				}
			}
		}
		shadowVisibilityFlags = newShadowVisibilityFlags;
	}
	IBaseObject::UpdateVisibilityFlags2();//calling 2 on purpose;
}
//---------------------------------------------------------------------------
//
SINGLE Planetoid::TestHighlight (const RECT &rect)
{
	bHighlight = 0;
	bMouseOver = 0;
	if (renderArch1 && renderArch2)
	{
		return 0;
	}
	if (rect.left > 0 && rect.left==rect.right)
	{
		if (bVisible)
		{
			SINGLE diffx, diffy;

			diffx = rect.left - center_x;
			diffy = rect.top - center_y;

			bool result = ((diffy*diffy)+(diffx*diffx) < (radius*radius));

			bMouseOver = result;
//			if (DEFAULTS->GetDefaults()->bEditorMode)
				bHighlight = result;
		}
		
		if (BUILDARCHEID)
		{
			bHighlight = FALSE;
			if (TestSlotHighlight(rect.left,rect.top))
				bHighlight = TRUE;
		}
	}
	return 0.0f;
}
//---------------------------------------------------------------------------
//
#define GRID_RAD 3800
#define INSIDE_RAD 2100
//
//
void Planetoid::enableTerrainFootprint (bool bEnable)
{
	FootprintInfo fpi;
	fpi.flags = TERRAIN_FULLSQUARE | TERRAIN_PARKED | TERRAIN_BLOCKLOS | TERRAIN_IMPASSIBLE;
	fpi.height = boxRadius;
	fpi.missionID = dwMissionID;

	// KLUDGE! KLUDGE! KLUDGE!!!
	Vector cposition(transform.translation.x + 50.1f, transform.translation.y + 50.1f, transform.translation.z + 50.1f);
	GRIDVECTOR gv[4];
	gv[0] = cposition;
	gv[0].centerpos();
	gv[1].init(gv[0].getX() - 1, gv[0].getY());
	gv[2].init(gv[0].getX() - 1, gv[0].getY() - 1);
	gv[3].init(gv[0].getX(), gv[0].getY() - 1);

	// get the TerrainMap pointer
	COMPTR<ITerrainMap> map;
	SECTOR->GetTerrainMap(GetSystemID(), map);

	if (bEnable)
	{
		CQASSERT(map_sys==0);
		map_square = OBJMAP->GetMapSquare(systemID,transform.translation);
		map_sys = systemID;
		OBJMAP->AddObjectToMap(this,map_sys,map_square,OM_SYSMAP_FIRSTPASS);

		if(renderArch1->pData->bMoon)
			map->SetFootprint(gv, 1, fpi); 
		else
			map->SetFootprint(gv, 4, fpi); 
	}
	else
	if (map_sys)
	{
		OBJMAP->RemoveObjectFromMap(this,map_sys,map_square);
		map_sys = map_square = 0;
	
		if(renderArch1->pData->bMoon)
			map->UndoFootprint(gv, 1, fpi); 
		else
			map->UndoFootprint(gv, 4, fpi);
	}
}
//---------------------------------------------------------------------------
//
BOOL32 Planetoid::TestSlotHighlight (const S32 &mouseX,const S32 &mouseY)
{
	if(BUILDARCHEID)
	{
		BASE_PLATFORM_DATA * data = (BASE_PLATFORM_DATA *)ARCHLIST->GetArchetypeData(BUILDARCHEID);
		if(renderArch1->pData->bMoon && data->bMoonPlatform)
		{
			if(bMouseOver)
			{
				highlightedSlot = 0x00000001;
				bPlanetHighlight = false;
				bHasAllHighlightSlots = true;
				if(trueMarks[MGlobals::GetThisPlayer()-1] & (0x00000001))
				{
					bHasAllHighlightSlots = false;
				}
			}
			else
			{
				highlightedSlot = 0;
				bPlanetHighlight = false;
				bHasAllHighlightSlots = false;
			}
			renderHighlightSlot = highlightedSlot;
			return bHasAllHighlightSlots;
		}
		else if((!(renderArch1->pData->bMoon)) && (!(data->bMoonPlatform)))
		{
			highlightedSlot = 0;
			U32 centerSlot = -1;
			bPlanetHighlight = false;
			bHasAllHighlightSlots = false;
			if (bVisible)
			{
				SINGLE _mouseX = mouseX,_mouseY = mouseY;
				Vector pos = transform.get_position();
				
				CAMERA->ScreenToPoint(_mouseX,_mouseY);
				S32 dist = (_mouseX-pos.x)*(_mouseX-pos.x)+(_mouseY-pos.y)*(_mouseY-pos.y);
				
				if (dist < GRID_RAD*GRID_RAD)
				{
					if (dist > INSIDE_RAD*INSIDE_RAD)
					{
						SINGLE angle;
						angle = atan2(_mouseX-pos.x,_mouseY-pos.y);
						angle += 1.25*PI+PI/(M_MAX_SLOTS);
						if (angle > 2*PI)
							angle -= 2*PI;
						
						centerSlot = ((U32)(M_MAX_SLOTS*(angle)/(2*PI)))%M_MAX_SLOTS;
						
						U32 slotsWanted = data->slotsNeeded;
						if(slotsWanted)
						{
							bHasAllHighlightSlots = true;
							U32 slotsAttempted = 1;
							if(trueMarks[MGlobals::GetThisPlayer()-1] & (0x00000001 << centerSlot))
							{
								highlightedSlot = (0x00000001 << centerSlot);
								bHasAllHighlightSlots = false;
							}
							else 
							{
								highlightedSlot = (0x00000001 << centerSlot);
							}
							SINGLE centAngle = angle-(((centerSlot*2)+1)*((2*PI)/(M_MAX_SLOTS*2)));
							bool goLeft = (centAngle > 0);
							U32 leftSlot = centerSlot;
							U32 rightSlot = centerSlot;
							while(slotsAttempted < slotsWanted)
							{
								++slotsAttempted;
								bool isOdd = (slotsAttempted%2) != 0;
								if((isOdd && !goLeft) || ((!isOdd) && goLeft) )
								{
									if(leftSlot == M_MAX_SLOTS-1)
										leftSlot = 0;
									else
										++leftSlot;

									if(trueMarks[MGlobals::GetThisPlayer()-1] & (0x00000001 << leftSlot))
									{
										highlightedSlot |= (0x00000001 << leftSlot);
										bHasAllHighlightSlots = false;
									}
									else 
									{
										highlightedSlot |= (0x00000001 << leftSlot);
									}
								}
								else
								{
									if(rightSlot == 0)
										rightSlot = M_MAX_SLOTS-1;
									else
										--rightSlot;


									if(trueMarks[MGlobals::GetThisPlayer()-1] & (0x00000001 << rightSlot))
									{
										highlightedSlot |= (0x00000001 << rightSlot);
										bHasAllHighlightSlots = false;
									}
									else 
									{
										highlightedSlot |= (0x00000001 << rightSlot);
									}
								}
							}
						}
					}
				}
			}
			
		//	IBaseObject *builder = OBJLIST->GetSelectedList();
		//	if (builder && bHighlight && centerSlot == -1 && IsVisibleToPlayer(MGlobals::GetThisPlayer()))
		//	{
		//		bPlanetHighlight = true;
		//	}

			renderHighlightSlot = highlightedSlot;
			return bHasAllHighlightSlots;
		}
	}
	return false;
}
//---------------------------------------------------------------------------
//

#define IBASEWIDTH  220
#define BASEWIDTH IDEAL2REALX(IBASEWIDTH)
#define IBASEHEIGHT  119
#define BASEHEIGHT IDEAL2REALY(IBASEHEIGHT)
#define IBASEYOFF  50
#define BASEYOFF  IDEAL2REALY(IBASEYOFF)

#define LINESPACINGX IDEAL2REALX(20)
#define LINESPACINGY IDEAL2REALY(20)

#define TRUEMAX_GAS_RES 200
#define TRUEMAX_CREW_RES 1000
#define TRUEMAX_METAL_RES 200

void Planetoid::DrawFleetMoniker (bool bAllShips)
{
	if (bVisible==0 || bMouseOver==0 || renderArch1->pData->bMoon)
		return;

	S32 orgX,orgY;
	S32 screenX, screenY;
	S32 iScreenX,iScreenY;

	Vector pos = GetPosition();
	if(CAMERA->PointToScreen(pos,&orgX,&orgY))
	{
		BATCH->set_state(RPR_BATCH,false);
		iScreenX = REAL2IDEALX(orgX);
		if(iScreenX < 320)
			iScreenX += 100;
		else
			iScreenX -= 100+IBASEWIDTH;
		iScreenY = REAL2IDEALY(orgY);
		if(iScreenY < IBASEHEIGHT+IBASEYOFF)
			iScreenY += IBASEYOFF;
		else
			iScreenY -= IBASEHEIGHT+IBASEYOFF;

		screenX = IDEAL2REALX(iScreenX);
		screenY = IDEAL2REALY(iScreenY);

		M_RACE barRace =MGlobals::GetPlayerRace(MGlobals::GetThisPlayer());
		if(barRace == M_TERRAN)
			t_backgroundShape->Draw(CAMERA->GetPane(),screenX,screenY);
		else if(barRace == M_MANTIS)
			m_backgroundShape->Draw(CAMERA->GetPane(),screenX,screenY);
		else if(barRace == M_SOLARIAN)
			s_backgroundShape->Draw(CAMERA->GetPane(),screenX,screenY);

//		DA::RectangleHash(CAMERA->GetPane(),screenX+IDEAL2REALX(59),screenY+IDEAL2REALY(52),screenX+BASEWIDTH-IDEAL2REALX(4),screenY+IDEAL2REALY(71),RGB(128,128,128));
//		DA::RectangleHash(CAMERA->GetPane(),screenX+IDEAL2REALX(59),screenY+IDEAL2REALY(74),screenX+BASEWIDTH-IDEAL2REALX(4),screenY+IDEAL2REALY(93),RGB(128,128,128));
//		DA::RectangleHash(CAMERA->GetPane(),screenX+IDEAL2REALX(59),screenY+IDEAL2REALY(96),screenX+BASEWIDTH-IDEAL2REALX(4),screenY+IDEAL2REALY(115),RGB(128,128,128));
//		DA::RectangleHash(CAMERA->GetPane(),screenX+IDEAL2REALX(5),screenY+IDEAL2REALY(5),screenX+BASEWIDTH-IDEAL2REALX(4),screenY+IDEAL2REALY(30),RGB(128,128,128));
//		DA::RectangleHash(CAMERA->GetPane(),screenX+IDEAL2REALX(60),screenY+IDEAL2REALY(33),screenX+BASEWIDTH-IDEAL2REALX(4),screenY+IDEAL2REALY(49),RGB(128,128,128));

		wchar_t buffer[255];
		wchar_t buffer2[255];
		wchar_t buffer3[255];

		S32 metalValue, gasValue, crewValue;
		U32 playerIndex = MGlobals::GetThisPlayer()-1;
		if((0x01 << (playerIndex)) & shadowVisibilityFlags)
		{
			metalValue = shadowMetal[playerIndex];
			gasValue = shadowGas[playerIndex];
			crewValue = shadowCrew[playerIndex];
		}
		else
		{
			metalValue = metal;
			gasValue = gas;
			crewValue = crew;
		}

		wcscpy(buffer3, _localLoadStringW(IDS_PLANET_RATE));
		if(maxMetal)
		{
			wcscpy(buffer2, _localLoadStringW(IDS_METAL_SIGN));		
			swprintf(buffer,L"%s %d/%d",buffer2,metalValue*METAL_MULTIPLIER,maxMetal*METAL_MULTIPLIER);
			pPlanetFont1->StringDraw(CAMERA->GetPane(),screenX+IDEAL2REALX(70),screenY+IDEAL2REALY(26),buffer);

			U32 val = F2LONG(playerMetalRate[playerIndex]*METAL_MULTIPLIER*60);
			swprintf(buffer,L"%d%s",val,buffer3);
			pPlanetFont4->StringDraw(CAMERA->GetPane(),screenX+IDEAL2REALX(175),screenY+IDEAL2REALY(26),buffer);

			metalShape->Draw(CAMERA->GetPane(),screenX+IDEAL2REALX(60),screenY+IDEAL2REALY(28));

			drawResourceBar(screenX+IDEAL2REALX(75),screenY+IDEAL2REALY(37),screenX+BASEWIDTH-IDEAL2REALX(5),screenY+IDEAL2REALY(41),metalValue,maxMetal,playerMetalRate[playerIndex]*60,TRUEMAX_METAL_RES);
		}
		if(maxGas)
		{
			wcscpy(buffer2, _localLoadStringW(IDS_GAS_SIGN));		
			swprintf(buffer,L"%s %d/%d",buffer2,gasValue*GAS_MULTIPLIER,maxGas*GAS_MULTIPLIER);
			pPlanetFont2->StringDraw(CAMERA->GetPane(),screenX+IDEAL2REALX(70),screenY+IDEAL2REALY(42),buffer);

			U32 val = F2LONG(playerGasRate[playerIndex]*GAS_MULTIPLIER*60);
			swprintf(buffer,L"%d%s",val,buffer3);
			pPlanetFont5->StringDraw(CAMERA->GetPane(),screenX+IDEAL2REALX(175),screenY+IDEAL2REALY(42),buffer);

			gasShape->Draw(CAMERA->GetPane(),screenX+IDEAL2REALX(60),screenY+IDEAL2REALY(44));

			drawResourceBar(screenX+IDEAL2REALX(75),screenY+IDEAL2REALY(53),screenX+BASEWIDTH-IDEAL2REALX(5),screenY+IDEAL2REALY(57),gasValue,maxGas,playerGasRate[playerIndex]*60,TRUEMAX_GAS_RES);
		}

		if(maxCrew)
		{
			wcscpy(buffer2, _localLoadStringW(IDS_CREW_SIGN));		
			swprintf(buffer,L"%s %d/%d",buffer2,crewValue*CREW_MULTIPLIER,maxCrew*CREW_MULTIPLIER);
			pPlanetFont3->StringDraw(CAMERA->GetPane(),screenX+IDEAL2REALX(70),screenY+IDEAL2REALY(58),buffer);

			U32 val = F2LONG(playerCrewRate[playerIndex]*CREW_MULTIPLIER*60);
			swprintf(buffer,L"%d%s",val,buffer3);
			pPlanetFont6->StringDraw(CAMERA->GetPane(),screenX+IDEAL2REALX(175),screenY+IDEAL2REALY(58),buffer);

			crewShape->Draw(CAMERA->GetPane(),screenX+IDEAL2REALX(60),screenY+IDEAL2REALY(60));

			drawResourceBar(screenX+IDEAL2REALX(75),screenY+IDEAL2REALY(69),screenX+BASEWIDTH-IDEAL2REALX(5),screenY+IDEAL2REALY(73),crewValue,maxCrew,playerCrewRate[playerIndex]*60,TRUEMAX_CREW_RES);
		}
		
		_localAnsiToWide(partName, buffer, sizeof(buffer));
		pPlanetFont7->StringDraw(CAMERA->GetPane(),screenX+IDEAL2REALX(10),screenY+IDEAL2REALY(4),buffer);

		switch(renderArch1->pData->planetType)
		{
		case BT_PLANET_DATA::M_CLASS:
			{
				planetShape[1]->Draw(CAMERA->GetPane(),screenX+IDEAL2REALX(8),screenY+IDEAL2REALY(27));
				wcscpy(buffer, _localLoadStringW(IDS_PLANET_TYPE_HABITABLE));		
				pPlanetFont8->StringDraw(CAMERA->GetPane(),screenX+IDEAL2REALX(15),screenY+IDEAL2REALY(15),buffer);
				break;
			}
		case BT_PLANET_DATA::METAL_PLANET:
			{
				planetShape[3]->Draw(CAMERA->GetPane(),screenX+IDEAL2REALX(8),screenY+IDEAL2REALY(27));
				wcscpy(buffer, _localLoadStringW(IDS_PLANET_TYPE_METAL));		
				pPlanetFont8->StringDraw(CAMERA->GetPane(),screenX+IDEAL2REALX(15),screenY+IDEAL2REALY(15),buffer);
				break;
			}
		case BT_PLANET_DATA::GAS_PLANET:
			{
				planetShape[2]->Draw(CAMERA->GetPane(),screenX+IDEAL2REALX(8),screenY+IDEAL2REALY(27));
				wcscpy(buffer, _localLoadStringW(IDS_PLANET_TYPE_GAS));		
				pPlanetFont8->StringDraw(CAMERA->GetPane(),screenX+IDEAL2REALX(15),screenY+IDEAL2REALY(15),buffer);
				break;
			}
		case BT_PLANET_DATA::OTHER_PLANET:
			{
				planetShape[0]->Draw(CAMERA->GetPane(),screenX+IDEAL2REALX(8),screenY+IDEAL2REALY(27));
				wcscpy(buffer, _localLoadStringW(IDS_PLANET_TYPE_OTHER));		
				pPlanetFont8->StringDraw(CAMERA->GetPane(),screenX+IDEAL2REALX(15),screenY+IDEAL2REALY(15),buffer);
				break;
			}
		}

	}

}
#define RESBARWIDTH 4
//---------------------------------------------------------------------------
//
void Planetoid::drawResourceBar(S32 x1,S32 y1,S32 x2,S32 y2,U32 value,U32 max,U32 rate,U32 trueMax)
{
	if(max)
	{
		SINGLE totalMax = __max(trueMax,max);
		U32 barWidth = IDEAL2REALX(RESBARWIDTH);
		U32 fullBarWidth = barWidth+2;
		
		U32 distX = (max/totalMax)*(x2-x1);

		U32 totalBars = (distX)/fullBarWidth;
		x2 = x1+fullBarWidth*(totalBars)-2;
		DA::RectangleFill(CAMERA->GetPane(),x1,y1,x2,y2,RGB(0,0,0));
		
		SINGLE totalT = ((SINGLE)value)/((SINGLE)max);
		S32 postRate = value-rate;
		SINGLE partialT;
		if(postRate >0)
			partialT = ((SINGLE)postRate)/((SINGLE)max);
		else
			partialT = 0;
		
		U32 numBars = totalBars*totalT;
		U32 partialBars = totalBars*partialT;
		U32 i = 0;
		while(i < partialBars)
		{
			DA::RectangleFill(CAMERA->GetPane(),(fullBarWidth*i)+x1,y1,(fullBarWidth*i)+x1+barWidth,y2,RGB(35,150,120));		
			++i;
		}
		while(i < numBars)
		{
			DA::RectangleFill(CAMERA->GetPane(),(fullBarWidth*i)+x1,y1,(fullBarWidth*i)+x1+barWidth,y2,RGB(48,200,152));		
			++i;			
		}
	}
}
//---------------------------------------------------------------------------
//
void Planetoid::updateHarvestRates()
{
	if(MGlobals::IsUpdateFrame(dwMissionID))
	{
		for(U32 j = 0; j < MAX_PLAYERS; ++j)
		{
			playerGasRate[j] =0;
			playerMetalRate[j] =0;
			playerCrewRate[j] =0;
		}
		U32 firstID = 0;
		U32 lastSlotUser = 0;
		for(U32 i = 0; i < M_MAX_SLOTS; ++i)
		{
			U32 slotUser = GetSlotUser(0x01<<i);
			if(slotUser && slotUser !=lastSlotUser && slotUser != firstID)
			{
				lastSlotUser = slotUser;
				if(!firstID)
					firstID = slotUser;
				OBJPTR<IPlatform> platform;
				OBJLIST->FindObject(slotUser,NONSYSVOLATILEPTR,platform,IPlatformID);
				if(platform)
				{	
					MPart part(platform.Ptr());
					if(part->bReady)
					{
						platform->AddHarvestRates(playerGasRate[part->playerID-1],playerMetalRate[part->playerID-1],playerCrewRate[part->playerID-1]);
					}
				}
			}
		}
	}
}
//---------------------------------------------------------------------------
//
void Planetoid::renderHalo(U8 red, U8 green,U8 blue,SINGLE sizeOuter,SINGLE sizeInner)
{
	if(sizeOuter > 0.01)
	{
		Vector camPos = CAMERA->GetPosition();
		Vector camLook = CAMERA->GetLookAtPosition();
		Vector lookDir = -(camLook-camPos).fast_normalize();
		Vector up(0,0,1);
		Vector i = cross_product(up,lookDir);
		i.fast_normalize();
		Vector j = cross_product(lookDir,i);
		j.fast_normalize();

		Transform trans;
		trans.set_i(i);
		trans.set_j(j);
		trans.set_k(lookDir);
		trans.translation = GetPosition();

		BATCH->set_state(RPR_BATCH,false);
		CAMERA->SetModelView(&trans);
		BATCH->set_render_state(D3DRS_ZENABLE,TRUE);
		BATCH->set_render_state(D3DRS_ZWRITEENABLE,FALSE);
		BATCH->set_render_state(D3DRS_ALPHABLENDENABLE,TRUE);
		BATCH->set_render_state(D3DRS_SRCBLEND,D3DBLEND_ONE);
		BATCH->set_render_state(D3DRS_DESTBLEND,D3DBLEND_ONE);//INVSRCALPHA);
		BATCH->set_render_state(D3DRS_DITHERENABLE,TRUE);
		BATCH->set_render_state(D3DRS_CULLMODE,D3DCULL_NONE);

		if (multiStages != 1)
		{
			SetupDiffuseBlend(NULL,FALSE);
		}

		Vertex2 *vb_data;
		U32 dwSize;
		GENRESULT result;
		result = PIPE->lock_vertex_buffer( haloMesh.vb_handle, DDLOCK_WRITEONLY, (void **)&vb_data, &dwSize );
		CQASSERT(result == GR_OK);

		DWORD colorClear = 0;
		DWORD colorSolid = 0x00FF<<24 | RGB(blue*alphaMod,green*alphaMod,red*alphaMod);

		for(int count = 0; count < HALO_SEGMENTS; ++count)
		{
			vb_data[count*2].color = colorSolid;
			vb_data[count*2].pos = haloMesh.ringCenter[count]*sizeInner;
			vb_data[count*2].pos.z = -2000;
			
			vb_data[count*2+1].color = colorClear;
			vb_data[count*2+1].pos = haloMesh.upperRing[count]*sizeOuter;
		}
		
		vb_data[HALO_SEGMENTS*2].color = colorSolid;
		vb_data[HALO_SEGMENTS*2].pos = haloMesh.ringCenter[0]*sizeInner;
		
		vb_data[HALO_SEGMENTS*2+1].color = colorClear;
		vb_data[HALO_SEGMENTS*2+1].pos = haloMesh.upperRing[0]*sizeOuter;
		
		result = PIPE->unlock_vertex_buffer( haloMesh.vb_handle );
		CQASSERT(result == GR_OK);
			
		result = PIPE->draw_primitive_vb( D3DPT_TRIANGLESTRIP, haloMesh.vb_handle, 0, HALO_SEGMENTS*2+2, 0 );
		CQASSERT(result == GR_OK);
	}
}
//---------------------------------------------------------------------------
//
void Planetoid::firstUpdate()
{
	if(!bFirstUpdate)
	{
		bFirstUpdate = true;
		if(renderArch1->ambientEffect)
		{
			IEffectInstance * inst = renderArch1->ambientEffect->CreateInstance();
			inst->SetSystemID(GetSystemID());
			inst->SetTarget(this,0,0);
			inst->TriggerStartEvent();
		}
	}
}
//---------------------------------------------------------------------------
//
BOOL32 Planetoid::Update (void)
{
	firstUpdate();

	updateHarvestRates();

	planetRing->bVisible = bVisible;

	setPhysicsState(bVisible!=0);

	if(MGlobals::GetGameSettings().regenOn && THEMATRIX->IsMaster())
	{
		genGas += (gasRegen+gasBoost)*ELAPSED_TIME;
		S32 newGas = genGas;
		genGas -= newGas;
		gas = __min(maxGas,gas + newGas);

		genMetal += (metalRegen+oreBoost)*ELAPSED_TIME;
		S32 newMetal = genMetal;
		genMetal -=newMetal;
		metal = __min(maxMetal,metal + newMetal);

		genCrew += (crewRegen+crewBoost)*ELAPSED_TIME;
		S32 newCrew = genCrew;
		genCrew -=newCrew;
		crew = __min(maxCrew,crew+newCrew);
	}

	updateDisplayValues();
	FRAME_update();

	return 1;
}
//---------------------------------------------------------------------------
//
void Planetoid::PhysicalUpdate (SINGLE dt)
{
	FRAME_physicalUpdate(dt);

	if(instanceMesh)
		instanceMesh->Update(dt);

	if (clickTime > 0)
		clickTime -= dt;

	if(renderArch2)
	{
		teraformTime += dt;
		if(teraformTime > totalTeraformTime)
		{
			renderArch2 = NULL;
		}
	}
	teraRing.PhysUpdate(dt,teraformHitDir,GetPosition());
}
//---------------------------------------------------------------------------
//
void Planetoid::Render (void)
{
	if (bVisible)
	{
		LIGHTS->ActivateAmbientLight(transform.translation);
		
		const bool bEditorMode = DEFAULTS->GetDefaults()->bEditorMode;

		if(instanceMesh)
			instanceMesh->Render();

		//draw particle ring
		teraRing.Render(teraformHitDir,GetPosition());

		if(renderArch1 && renderArch2)
		{
			SINGLE delta = teraformTime/totalTeraformTime;
			if(delta < 0.5)
			{
				delta = 1.0-(delta*2);
				renderHalo(renderArch2->pData->halo.red*delta,renderArch2->pData->halo.green*delta,renderArch2->pData->halo.blue*delta,renderArch2->pData->halo.sizeOuter,renderArch2->pData->halo.sizeInner);
			}
			else
			{
				delta = (delta-0.5)*2;
				renderHalo(renderArch1->pData->halo.red*delta,renderArch1->pData->halo.green*delta,renderArch1->pData->halo.blue*delta,renderArch1->pData->halo.sizeOuter,renderArch1->pData->halo.sizeInner);
			}
		}
		else
		{
			renderHalo(renderArch1->pData->halo.red,renderArch1->pData->halo.green,renderArch1->pData->halo.blue,renderArch1->pData->halo.sizeOuter,renderArch1->pData->halo.sizeInner);
		}
		
		//  draw platform slots
		TRANSFORM trans = baseSlotTrans;
		trans.rotate_about_j(-PI/2);
		trans.set_position(transform.get_position() + (trans.get_k() * -2500));
		if (bEditorMode || BUILDARCHEID || clickTime > 0)
		{
			if(!(renderArch1->pData->bMoon))
			{
				int i;
				for (i=0;i<M_MAX_SLOTS;i++)
				{
					if (!(trueMarks[MGlobals::GetThisPlayer()-1] & (0x01 << i)))
					{
						if ((0x00000001 << i) & renderHighlightSlot)
						{
							if(bHasAllHighlightSlots)
							{
								for (int c=0;c<mat_cnt;c++)
								{
									slot_mat[c].emission.r = 0;
									slot_mat[c].emission.g = 200;
									slot_mat[c].emission.b = 0;
								}
							}
							else
							{
								for (int c=0;c<mat_cnt;c++)
								{
									slot_mat[c].emission.r = 200;
									slot_mat[c].emission.g = 0;
									slot_mat[c].emission.b = 0;
								}
							}
						}
						else
						{
							for (int c=0;c<mat_cnt;c++)
							{
								slot_mat[c].emission.r = 90;
								slot_mat[c].emission.g = 90;
								slot_mat[c].emission.b = 200;
							}
						}
						
						if (clickedSlot & (0x00000001 << i) && clickTime > 0)
						{
							TRANSFORM trans2 = trans;
							trans2.translation.z -= 100;
							ENGINE->set_transform(slot,trans2);
						}
						else
							ENGINE->set_transform(slot,trans);
						
						ENGINE->render_instance(MAINCAM, slot, 0, LODPERCENT, 0, NULL);
					}
					
					trans.set_position(trans.get_position() + (trans.get_k() * 2500));
					trans.rotate_about_j(2*PI/M_MAX_SLOTS);
					trans.set_position(trans.get_position() + (trans.get_k() * -2500));
				}
			}
			if(renderHighlightSlot)
			{
				if(bHasAllHighlightSlots)
					UNBORNMANAGER->RenderMeshAt(GetSlotTransform(renderHighlightSlot),BUILDARCHEID,MGlobals::GetThisPlayer(),0,255,0,128);
				else
					UNBORNMANAGER->RenderMeshAt(GetSlotTransform(renderHighlightSlot),BUILDARCHEID,MGlobals::GetThisPlayer(),255,0,0,128);
			}
			renderHighlightSlot = 0;
		}
	}
}
//---------------------------------------------------------------------------
//
void Planetoid::MapRender (bool bPing)
{
	if( ((GetVisibilityFlags()&MGlobals::GetAllyMask(MGlobals::GetThisPlayer()))!=0) || DEFAULTS->GetDefaults()->bEditorMode || DEFAULTS->GetDefaults()->bSpectatorModeOn)
	{
		if(renderArch1->sysMapIconID != -1)
			SYSMAP->DrawIcon(transform.translation,renderArch1->pData->bMoon ? GRIDSIZE : GRIDSIZE*2,renderArch1->sysMapIconID);
		else
			SYSMAP->DrawCircle(transform.translation,renderArch1->pData->bMoon ? GRIDSIZE : GRIDSIZE*2,RGB(255,255,255));
		
	}
}

//---------------------------------------------------------------------------
//
void Planetoid::View (void)
{
	PLANET_VIEW view;
	BASIC_INSTANCE data;

	view.rtData = &data;
	view.mission = this;
	CQASSERT(view.mission);

	Vector vec;
	Matrix matrix = get_orientation(instanceIndex);
	memset(&data, 0, sizeof(data));
	
	vec = get_position(instanceIndex);
	memcpy(&data.position, &vec, sizeof(data.position));

	vec = ang_velocity;
	vec = vec * matrix;		// transpose multiply (convert to object coordinates)

	memcpy(&data.rotation, &vec, sizeof(data.rotation));

	view.crew = crew;
	view.crewRegen = crewRegen;
	view.maxCrew = maxCrew;
	view.gas = gas;
	view.gasRegen = gasRegen;
	view.maxGas = maxGas;
	view.metal = metal;
	view.metalRegen = metalRegen;
	view.maxMetal = maxMetal;

	if (DEFAULTS->GetUserData("PLANET_VIEW", view.mission->partName, &view, sizeof(view)))
	{
		memcpy(&vec, &data.position, sizeof(data.position));
		set_position(instanceIndex, vec);

		memcpy(&vec, &data.rotation, sizeof(data.rotation));
		vec = matrix * vec;		// multiply (convert back to world coordinates)

		ang_velocity = vec;

		if (dwMissionID != ((dwMissionID & ~0xF) | playerID))	// playerID has changed
		{
			OBJLIST->RemovePartID(this, dwMissionID);
			dwMissionID = (dwMissionID & ~0xF) | playerID;
			OBJLIST->AddPartID(this, dwMissionID);
		}
		crew = view.crew;
		crewRegen = view.crewRegen;
		maxCrew = view.maxCrew;
		gas = view.gas;
		gasRegen = view.gasRegen;
		maxGas = view.maxGas;
		metal = view.metal;
		metalRegen = view.metalRegen;
		maxMetal = view.maxMetal;
	}
}
//---------------------------------------------------------------------------
//
BOOL32 Planetoid::Save (struct IFileSystem * inFile)
{
	DAFILEDESC fdesc = "PLANET_SAVELOAD";
	COMPTR<IFileSystem> file;
	BOOL32 result = 0;
	DWORD dwWritten;
	PLANET_SAVELOAD save;

	fdesc.lpImplementation = "DOS";
	fdesc.dwDesiredAccess = GENERIC_READ|GENERIC_WRITE;
	fdesc.dwShareMode = 0;  // no sharing
	fdesc.dwCreationDistribution = CREATE_ALWAYS;

	if (inFile->CreateInstance(&fdesc, file) != GR_OK)
		goto Done;

	memset(&save, 0, sizeof(save));

	FRAME_save(save);
	memcpy(&save, static_cast<BASE_PLANET_SAVELOAD *>(this), sizeof(BASE_PLANET_SAVELOAD));
	save.exploredFlags = GetVisibilityFlags();

	file->WriteFile(0, &save, sizeof(save), &dwWritten, 0);
	
	result = 1;

Done:	
	return result;
}
//---------------------------------------------------------------------------
//
BOOL32 Planetoid::Load (struct IFileSystem * inFile)
{
	DAFILEDESC fdesc = "PLANET_SAVELOAD";
	COMPTR<IFileSystem> file;
	BOOL32 result = 0;
	DWORD dwRead;
	PLANET_SAVELOAD load;
	U8 buffer[1024];

	fdesc.lpImplementation = "DOS";
	if (inFile->CreateInstance(&fdesc, file) != GR_OK)
		goto Done;

	file->ReadFile(0, buffer, sizeof(buffer), &dwRead, 0);
	MISSION->CorrelateSymbol("PLANET_SAVELOAD", buffer, &load);

	memcpy(static_cast<BASE_PLANET_SAVELOAD *>(this), &load, sizeof(BASE_PLANET_SAVELOAD));
	FRAME_load(load);
	SetJustVisibilityFlags(load.exploredFlags);

	UpdateVisibilityFlags();
	enableTerrainFootprint(true);

	lastMetal = metal;
	lastGas = gas;
	lastCrew = crew;

	result = 1;

Done:	
	return result;
}
//---------------------------------------------------------------------------
//
void Planetoid::ResolveAssociations()
{
}
//---------------------------------------------------------------------------
//
void Planetoid::QuickSave (struct IFileSystem * file)
{
	DAFILEDESC fdesc = partName;
	HANDLE hFile;

	file->CreateDirectory("MT_PLANET_QLOAD");
	if (file->SetCurrentDirectory("MT_PLANET_QLOAD") == 0)
		CQERROR0("QuickSave failed on directory 'MT_PLANET_QLOAD'");

	fdesc.dwDesiredAccess = GENERIC_READ|GENERIC_WRITE;
	fdesc.dwShareMode = 0;  // no sharing
	fdesc.dwCreationDistribution = CREATE_NEW;		// fail if file already exists

	if ((hFile = file->OpenChild(&fdesc)) == INVALID_HANDLE_VALUE)
	{
		CQERROR1("QuickSave failed on part '%s'", fdesc.lpFileName);
	}
	else
	{
		MT_PLANET_QLOAD qload;
		DWORD dwWritten;

		qload.pos.init(GetGridPosition(), systemID);

		file->WriteFile(hFile, &qload, sizeof(qload), &dwWritten, 0);
		file->CloseHandle(hFile);
	}
}
//---------------------------------------------------------------------------
//
void Planetoid::QuickLoad (const char *szSaveLoadType, const char *szInstanceName, void * buffer, U32 bufferSize)
{
	MT_PLANET_QLOAD qload;
	MISSION->CorrelateSymbol(szSaveLoadType, buffer, &qload);

	systemID = qload.pos.systemID;
	transform.translation = qload.pos;
	MGlobals::InitMissionData(this, MGlobals::CreateNewPartID(0));
	partName = szInstanceName;
	SetReady(true);

	genCrew = genGas = genCrew = 0;

	OBJLIST->AddPartID(this, dwMissionID);
	enableTerrainFootprint(true);
}
//---------------------------------------------------------------------------
//
void Planetoid::QuickResolveAssociations (void)
{
	// nothing to do here!?
}
//---------------------------------------------------------------------------
//
U32 Planetoid::GetHighlightedSlot()
{
	if(bPlanetHighlight)
		return 0;
	return highlightedSlot;		// return in range [1 to MAX_SLOTS]
}
//---------------------------------------------------------------------------
//
TRANSFORM Planetoid::GetSlotTransform (U32 slotNumber)
{
	CQASSERT(slotNumber && (!(slotNumber & (~0x00000FFF))));

	if(renderArch1->pData->bMoon)
	{
		return GetTransform();
	}
	else
	{
		TRANSFORM trans = baseSlotTrans;
		trans.set_position(transform.get_position());
				
		U32 startIndex = 0;
		while(slotNumber & (0x00000001 << startIndex))  //need to start at a zero to miss the overlap case
		{
			++startIndex;
		}

		SINGLE centerNumber = 0.0;
		U32 spotIndex = startIndex;
		U32 numSpots = 0;
		do
		{
			if(slotNumber & (0x00000001 << (spotIndex % M_MAX_SLOTS)))
			{
				centerNumber += spotIndex;
				++numSpots;
			}
			++spotIndex;
		}while(spotIndex != startIndex+M_MAX_SLOTS);
		
		centerNumber = centerNumber /numSpots;

		//the last PI/2 is to orient the slot.
		trans.rotate_about_j(centerNumber*2*PI/M_MAX_SLOTS - PI/2);
		Vector forward = -trans.get_k();
		trans.set_position(trans.get_position()+forward*2500);

		return trans;
	}
	return GetTransform();
}
//--------------------------------------------------------------------------//
//
void Planetoid::ClickSlot(U32 slotNumber)
{
	CQASSERT(slotNumber && (!(slotNumber & (~0x00000FFF))));
	clickedSlot = slotNumber;
	clickTime = CLICK_TIME;
}
//--------------------------------------------------------------------------//
//
U16 Planetoid::AllocateBuildSlot (U32 ownerMissionID, U16 dwSlotHandle,bool bFail)
{
	CQASSERT(dwSlotHandle);

	//two loops are needed because you don't want any slotUsers taken and still return 0
	int index;
	for(index = 0; index < M_MAX_SLOTS;++index)
	{
		if ( ((0x00000001 << index) & dwSlotHandle) && (slotUser[index] != 0))
		{
			if(THEMATRIX->IsMaster() || bFail)
			{
				return 0;
			}
//			else //this might be a bogus assert because the object could go away without a pending death.
//				CQASSERT(THEMATRIX->HasPendingDeath(slotUser[index]));
		}
	}
	for(index = 0; index < M_MAX_SLOTS;++index)
	{
		if ( ((0x00000001 << index) & dwSlotHandle)  )
		{
			CQASSERT(slotUser[index] == 0 || (!THEMATRIX->IsMaster()));
			slotUser[index] = ownerMissionID;
		}
		for(U32 i = 0; i < MAX_PLAYERS; ++i)
		{
			if(!((shadowVisibilityFlags >> i) & 0x01))
			{
				slotMarks[i] |= dwSlotHandle;
			}
			trueMarks[i] = slotMarks[i]|mySlotMarks[i];
		}
	}
	return dwSlotHandle;
}
//--------------------------------------------------------------------------//
//
void Planetoid::DeallocateBuildSlot (U32 ownerMissionID, U16 dwSlotHandle)
{
	CQASSERT(dwSlotHandle);
	for(int index = 0; index < M_MAX_SLOTS;++index)
	{
		U32 realSlotID = 0; //we could acualy be different because of client side overlap.
		if ( (0x00000001 << index) & dwSlotHandle)
		{
			if(ownerMissionID == slotUser[index])//might have come to be someone else if we are the client
			{
				slotUser[index] = 0;
				realSlotID |= (0x00000001 << index);
			}
		}
		for(U32 i = 0; i < MAX_PLAYERS; ++i)
		{
			if(!((shadowVisibilityFlags >> i) & 0x01))
			{
				slotMarks[i] &= (~realSlotID);
			}
			trueMarks[i] = slotMarks[i]|mySlotMarks[i];
		}
	}
}
//-------------------------------------------------------------------
//
bool Planetoid::HasOpenSlots (void) const
{
	int i;

	for (i = 0; i < M_MAX_SLOTS; i++)
	{
		if (slotUser[i] == 0)
			return true;
	}

	return false;
}
//-------------------------------------------------------------------
//
U32 Planetoid::GetSlotUser (U32 slotNumber)
{
	CQASSERT(slotNumber);
	
	for(int index = 0; index < M_MAX_SLOTS;++index)
	{
		if ( (0x00000001 << index) & slotNumber && slotUser[index]) 
		{
			return slotUser[index];
		}
	}
	return 0;
}
//-------------------------------------------------------------------
//
void Planetoid::GetPlanetSlotMissionIDs(U32 * buffer, U32 slotID)
{
	int offset = 0;
	for(int index = 0; index < M_MAX_SLOTS ; index++)
	{
		if(slotID & (0x00000001 << index))
		{
			buffer[offset] = dwMissionID | ((64+index)<<24);//this returns an ID in the subordinate space
			++offset;
		}
	}
}
//-------------------------------------------------------------------
//
void Planetoid::CompleteSlotOperations(U32 workingID,U32 targetSlotID)
{
	for(int index = 0; index < M_MAX_SLOTS ; index++)
	{
		if(targetSlotID & (0x00000001 << index))
		{
			U32 slotMissionID = dwMissionID | ((64+index)<<24);//this returns an ID in the subordinate space
			THEMATRIX->OperationCompleted(workingID,slotMissionID);
		}
	}
}
//-------------------------------------------------------------------
//
U32 Planetoid::FindBestSlot(PARCHETYPE buildType, const Vector * preferedLoc)
{
	BASE_PLATFORM_DATA * data = (BASE_PLATFORM_DATA *)ARCHLIST->GetArchetypeData(buildType);
	U32 bestSlot = 0;
	if(data->bMoonPlatform && renderArch1->pData->bMoon)
	{		
		if ( (slotUser[0] == 0))
		{
			return 0x00000001;
		}
	}
	else if((!(data->bMoonPlatform)) && (!(renderArch1->pData->bMoon)))
	{
		SINGLE bestDistance = 0;
		U32 numSlots = data->slotsNeeded;
		U32 slotMask = 0x00000001;
		
		while(!(slotMask & (0x00000001 << (numSlots-1))))
		{
			slotMask = (slotMask << 1) | 0x00000001;
		}
		for (U32 i=0; i < M_MAX_SLOTS; i++)
		{
			bool slotFree = true;
			for(int index = 0; index < M_MAX_SLOTS;++index)
			{
				if ( ((0x00000001 << index) & slotMask) && (slotUser[index] != 0))
				{
					slotFree = false;
				}
			}
			if (slotFree)
			{
				if(preferedLoc)
				{
					SINGLE distance;
					distance = ((*preferedLoc)-GetSlotTransform(slotMask).translation).magnitude();
					if ((!bestSlot) || distance < bestDistance)
					{
						bestDistance = distance;
						bestSlot = slotMask;
					}
				}
				else
				{
					return slotMask;
				}
			}
			slotMask = slotMask << 1;
			if(slotMask & (0x00000001 << M_MAX_SLOTS))
			{
				slotMask = (slotMask & (~(0x00000001 << M_MAX_SLOTS))) | 0x00000001; 
			}
		}
	}
	return bestSlot;
}
//-------------------------------------------------------------------
//
bool Planetoid::MarkSlot(U32 playerID,U32 dwSlotHandle,U32 achtypeID)
{
	for(U32 i = 0; i <M_MAX_SLOTS; ++i)
	{
		if(slotMarks[playerID-1] & (0x01 << i) && dwSlotHandle & (0x01 << i))
			return false;
	}
	mySlotMarks[playerID-1] |= dwSlotHandle;
	trueMarks[playerID-1] = slotMarks[playerID-1]|mySlotMarks[playerID-1];

	return true;
}
//-------------------------------------------------------------------
//
void Planetoid::UnmarkSlot(U32 playerID,U32 dwSlotHandle)
{
	mySlotMarks[playerID-1] &= (~dwSlotHandle);
	trueMarks[playerID-1] = slotMarks[playerID-1]|mySlotMarks[playerID-1];
}
//-------------------------------------------------------------------
//
bool Planetoid::IsHighlightingBuild()
{
	return highlightedSlot!=0;
}
//-------------------------------------------------------------------
//
void Planetoid::ChangePlayerRates(U32 playerID, SINGLE metalRate, SINGLE gasRate, SINGLE crewRate)
{
	U32 playerIndex = playerID -1;
	playerMetalRate[playerIndex] += metalRate;
	playerGasRate[playerIndex] += gasRate;
	playerCrewRate[playerIndex] += crewRate;
}
//-------------------------------------------------------------------
//
U32 Planetoid::GetEmptySlots()
{
	U32 retVal = M_MAX_SLOTS;
	for(U32 i = 0; i < M_MAX_SLOTS; ++i)
	{
		if(slotUser[i])
			--retVal;
	}
	return retVal;
}
//-------------------------------------------------------------------
//
void Planetoid::TeraformPlanet(const char * newArch, SINGLE changeTime, const Vector & hitDir)
{
	PARCHETYPE targetArch = ARCHLIST->LoadArchetype(newArch);

	if(targetArch)
	{
		renderArch2 = renderArch1;
		renderArch1 = (PLANET_INIT *)ARCHLIST->GetArchetypeHandle(targetArch);
		teraformTime = 0;
		totalTeraformTime= changeTime;
		teraformHitDir = hitDir;
		teraRing.Init(500,changeTime,renderArch1->teraRingAnim,renderArch1->pData->teraColor.red,renderArch1->pData->teraColor.green,renderArch1->pData->teraColor.blue,renderArch1->teraExplosion,systemID);

		crew = maxCrew = renderArch1->pData->maxCrew;
		gas = maxGas =  renderArch1->pData->maxGas;
		metal = maxMetal = renderArch1->pData->maxMetal;
		metalRegen =  renderArch1->pData->metalRegen;
		gasRegen = renderArch1->pData->gasRegen;
		crewRegen =  renderArch1->pData->crewRegen;
	}
}
//-------------------------------------------------------------------
//
bool Planetoid::IsMoon()
{
	return renderArch1->pData->bMoon;
}
//-------------------------------------------------------------------
//
void Planetoid::BoostRegen(SINGLE boostOre, SINGLE boostGas, SINGLE boostCrew)
{
	oreBoost += boostOre;
	gasBoost += boostGas;
	crewBoost += boostCrew;
}
//-------------------------------------------------------------------
//
/*
bool ObjectComm::hasOpenSlots (MPlanet & planet)
{
	bool result = false;
	OBJPTR<IPlanet> pPlanet;

	planet.obj->QueryInterface(IPlanetID, pPlanet);
	CQASSERT(pPlanet!=0);

	int i = pPlanet->GetMaxSlots();

	while (i-- > 0)
	{
		if ((result = (planet->slotUser[i] == 0)) != false)
			break;
	}

	return result;
}
*/
//---------------------------------------------------------------------------
//
void Planetoid::updateDisplayValues (void)
{
	//
	// update the hullpoints/supplies toward the true values.
	// Try to match values in 1 second of game time.
	//
}
//---------------------------------------------------------------------------
//
struct PlanetSync
{
	S32 gasDiff:10;
	S32 metalDiff:10;
	S32 crewDiff:10;
};
//---------------------------------------------------------------------------
//
U32 Planetoid::getSyncData (void * buffer)
{
	S32 realDif;
	PlanetSync sync;
	sync.crewDiff =0;
	sync.gasDiff =0;
	sync.metalDiff =0;

	bool bSync = false;
	if(lastGas != gas)
	{
		realDif = gas-lastGas;
		if(realDif > 511)
			realDif = 511;
		else if(realDif < -511)
			realDif = -511;
		sync.gasDiff = realDif;
		lastGas += realDif;
		bSync = true;
	}
	if(lastMetal != metal)
	{
		realDif = metal-lastMetal;
		if(realDif > 511)
			realDif = 511;
		else if(realDif < -511)
			realDif = -511;
		sync.metalDiff = realDif;
		lastMetal += realDif;
		bSync = true;
	}
	if(lastCrew != crew)
	{
		realDif = crew-lastCrew;
		if(realDif > 511)
			realDif = 511;
		else if(realDif < -511)
			realDif = -511;
		sync.crewDiff = realDif;
		lastCrew += realDif;
		bSync = true;
	}
	if(bSync)
	{
		*((PlanetSync *)buffer) = sync;
		return sizeof(PlanetSync);
	}
	return 0;
}
//---------------------------------------------------------------------------
//
void Planetoid::putSyncData (void * buffer, U32 bufferSize, bool bLateDelivery)
{
	CQASSERT(bLateDelivery==false);
	PlanetSync sync = *((PlanetSync *)buffer);
	gas += sync.gasDiff;
	lastGas = gas;
	metal += sync.metalDiff;
	lastMetal = metal;
	crew += sync.crewDiff;
	lastCrew = crew;
}
//---------------------------------------------------------------------------
//--------------------------PlanetRing methods-------------------------------
//---------------------------------------------------------------------------
//
const TRANSFORM & PlanetRing::GetTransform (void) const
{
	return owner->GetTransform();
}
//---------------------------------------------------------------------------
//
Vector PlanetRing::GetVelocity (void)
{
	return owner->GetVelocity();
}
//---------------------------------------------------------------------------
//
U32 PlanetRing::GetSystemID (void) const			// current system
{
	return owner->GetSystemID();
}
//---------------------------------------------------------------------------
//
bool PlanetRing::GetObjectBox (OBJBOX & box) const  // return false if not supported
{
	owner->GetObjectBox(box);
	box[0] = box[4] = GRID_RAD;
	box[1] = box[5] = -GRID_RAD;
	box[3] = -500;
	return true;
}
//---------------------------------------------------------------------------
//
U32 PlanetRing::GetPartID (void) const
{
	CQBOMB0("Attempt to get part id for planet ring");
	return 0;
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//
inline struct IBaseObject * createPlanetoid (const PLANET_INIT & data)
{
	Planetoid * obj = new ObjectImpl<Planetoid>;
	CQASSERT(data.meshArch);

	obj->renderArch1 = (PLANET_INIT *)&data;
	obj->renderArch2 = NULL;

	obj->FRAME_init(data);
	CQASSERT(obj->GetObjectIndex() != -1);
	 
	obj->pArchetype = data.pArchetype;
	obj->objClass = OC_PLANETOID;

	obj->lastCrew = obj->crew = obj->maxCrew = data.pData->maxCrew;
	obj->lastGas = obj->gas = obj->maxGas =  data.pData->maxGas;
	obj->lastMetal = obj->metal = obj->maxMetal = data.pData->maxMetal;
	obj->metalRegen =  data.pData->metalRegen;
	obj->gasRegen = data.pData->gasRegen;
	obj->crewRegen =  data.pData->crewRegen;

	obj->oreBoost = 0;
	obj->crewBoost = 0;
	obj->gasBoost = 0;

	obj->transform.rotate_about_i(90*MUL_DEG_TO_RAD);

	Vector rot (0,0.04,0);
	obj->ang_velocity=obj->transform*rot;
	obj->mapTex = data.mapTex;
	
	return obj;
}
//------------------------------------------------------------------------------------------
//
static void load_global_slots (void)
{
	Mesh *mesh;
	if (slot == INVALID_INSTANCE_INDEX)
	{
		COMPTR<IFileSystem> file;
		DAFILEDESC fdesc = "slotholder.3db";
		if (OBJECTDIR->CreateInstance(&fdesc,file) != GR_OK)
			CQFILENOTFOUND(fdesc.lpFileName);

		HARCH archeID;
		if (file!=0 && archeID.setArchetype(ENGINE->create_archetype("slotholder.3db",file)) != INVALID_ARCHETYPE_INDEX)
		{
			slot = ENGINE->create_instance2(archeID, NULL);
			baseSlotTrans.set_identity();
			baseSlotTrans.rotate_about_i(90*MUL_DEG_TO_RAD);
			baseSlotTrans.rotate_about_j(PI/4+180*MUL_DEG_TO_RAD);
	
			mesh = REND->get_instance_mesh(slot);
			slot_mat = mesh->material_list;
			mat_cnt = mesh->material_cnt;
			for (int c=0;c<mat_cnt;c++)
			{
				slot_mat[c].diffuse.r = 0;
				slot_mat[c].diffuse.g = 0;
				slot_mat[c].diffuse.b = 0;
				slot_mat[c].flags = MF_EMITTER;
			}

		}
	}
	if (circle == INVALID_INSTANCE_INDEX)
	{
		COMPTR<IFileSystem> file;
		DAFILEDESC fdesc = "planet_inside.3db";
		if (OBJECTDIR->CreateInstance(&fdesc,file) != GR_OK)
			CQFILENOTFOUND(fdesc.lpFileName);

		HARCH archeID;
		if (file!=0 && archeID.setArchetype(ENGINE->create_archetype("planet_inside.3db",file)) != INVALID_ARCHETYPE_INDEX)
		{
			circle = ENGINE->create_instance2(archeID, NULL);
			/*basecircleTrans.set_identity();
			basecircleTrans.rotate_about_i(90*MUL_DEG_TO_RAD);
			basecircleTrans.rotate_about_j(180*MUL_DEG_TO_RAD);*/

			Mesh *mesh = REND->get_instance_mesh(circle);
			circle_mat = mesh->material_list;
		}
	}
}
//------------------------------------------------------------------------------------------
//
static void unload_global_slots (void)
{
	if (slot != INVALID_INSTANCE_INDEX)
	{
		ENGINE->destroy_instance(slot);
		slot = INVALID_INSTANCE_INDEX;
	}
	if (circle != INVALID_INSTANCE_INDEX)
	{
		ENGINE->destroy_instance(circle);
		circle = INVALID_INSTANCE_INDEX;
	}
}
//------------------------------------------------------------------------------------------
//---------------------------Planetoid Factory----------------------------------------------
//------------------------------------------------------------------------------------------
//
//--------------------------------------------------------------------------//
//
struct DACOM_NO_VTABLE PlanetFactory : public IObjectFactory, IEventCallback
{
	struct OBJTYPE : PLANET_INIT
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
			meshArch = NULL;
			archIndex = -1;
			mapTex = 0;
		}
		
		~OBJTYPE (void)
		{
			if (meshArch)
				meshArch->Release();
			if(ambientEffect)
				EFFECTPLAYER->ReleaseEffect(ambientEffect);
			
			if (mapTex)
			{
				TMANAGER->ReleaseTextureRef(mapTex);
				mapTex = 0;
			}

			if(teraRingAnim)
			{
				delete teraRingAnim;
				teraRingAnim = NULL;
			}
		}
	};

	U32 factoryHandle;		// handles to callback
	AnimArchetype *pointAnimArch;
	U32 eventHandle;
	//
	// Interface mapping
	//

	BEGIN_DACOM_MAP_INBOUND(PlanetFactory)
	DACOM_INTERFACE_ENTRY(IObjectFactory)
	DACOM_INTERFACE_ENTRY(IEventCallback)
	END_DACOM_MAP()

	PlanetFactory (void) { }

	~PlanetFactory (void);

	IDAComponent * GetBase (void)
	{
		return (IEventCallback *) this;
	}

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

	DEFMETHOD(Notify) (U32 message, void *param);
	/* PlanetFactory methods */

	IDAComponent * getBase (void)
	{
		return static_cast<IObjectFactory *> (this);
	}

	void loadTextures (bool bLoad);
};
//--------------------------------------------------------------------------//
//
PlanetFactory::~PlanetFactory (void)
{
	COMPTR<IDAConnectionPoint> connection;

	unload_global_slots();

	if (OBJLIST && OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
		connection->Unadvise(factoryHandle);

	if (GS)
	{
		COMPTR<IDAConnectionPoint> connection;
		
		if (GS->QueryOutgoingInterface("IEventCallback", connection) == GR_OK)
			connection->Unadvise(eventHandle);
	}

	if (pointAnimArch)
		delete pointAnimArch;
}
//--------------------------------------------------------------------------//
//
void PlanetFactory::init (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
		connection->Advise(getBase(), &factoryHandle);

	if (GS->QueryOutgoingInterface("IEventCallback", connection) == GR_OK)
	{
		CQASSERT(eventHandle==0);
		connection->Advise(GetBase(), &eventHandle);
	}
}
//-----------------------------------------------------------------------------
//
HANDLE PlanetFactory::CreateArchetype (const char *szArchname, OBJCLASS objClass, void *_data)
{
	OBJTYPE * result = 0;

	if (objClass == OC_PLANETOID)
	{
		int lastTexMem = TEXMEMORYUSED;
		BT_PLANET_DATA * data = (BT_PLANET_DATA *) _data;
		result = new OBJTYPE;
		result->pData = data;
		result->pArchetype = ARCHLIST->GetArchetype(szArchname);
		result->pointAnimArch = pointAnimArch;

		result->meshArch = MESHMAN->CreateMeshArch(data->fileName);
 
		if (result->mapTex == 0)
			result->mapTex = TMANAGER->CreateTextureFromFile("mapearth.tga",TEXTURESDIR, DA::TGA,PF_4CC_DAA1);

		result->ownerTextID = TMANAGER->CreateTextureFromFile("PlanetOwner.tga", TEXTURESDIR, DA::TGA,PF_4CC_DAA4);

		if(data->sysMapIcon[0])
		{
			result->sysMapIconID = SYSMAP->RegisterIcon(data->sysMapIcon);
		}
		else
			result->sysMapIconID = -1;

		if(data->ambientEffect[0])
		{
			result->ambientEffect = EFFECTPLAYER->LoadEffect(data->ambientEffect);
		}
		else
			result->ambientEffect = NULL;
		
		if(!pPlanetFont1)
		{
			pPlanetFontType = GENDATA->LoadArchetype("Font!!PlanetInfo");
			CQASSERT(pPlanetFontType);
			GENDATA->AddRef(pPlanetFontType);

			COMPTR<IDAComponent> pBase;
			GENDATA->CreateInstance(pPlanetFontType, pBase);
			CQASSERT(pBase!=0);
			pBase->QueryInterface("IFontDrawAgent", pPlanetFont1);
			pPlanetFont1->SetFontColor(RGB(255,255,255)| 0xFF000000,0);

			GENDATA->CreateInstance(pPlanetFontType, pBase);
			CQASSERT(pBase!=0);
			pBase->QueryInterface("IFontDrawAgent", pPlanetFont2);
			pPlanetFont2->SetFontColor(RGB(255,255,255)| 0xFF000000,0);

			GENDATA->CreateInstance(pPlanetFontType, pBase);
			CQASSERT(pBase!=0);
			pBase->QueryInterface("IFontDrawAgent", pPlanetFont3);
			pPlanetFont3->SetFontColor(RGB(255,255,255)| 0xFF000000,0);

			GENDATA->CreateInstance(pPlanetFontType, pBase);
			CQASSERT(pBase!=0);
			pBase->QueryInterface("IFontDrawAgent", pPlanetFont4);
			pPlanetFont4->SetFontColor(RGB(255,255,255)| 0xFF000000,0);

			GENDATA->CreateInstance(pPlanetFontType, pBase);
			CQASSERT(pBase!=0);
			pBase->QueryInterface("IFontDrawAgent", pPlanetFont5);
			pPlanetFont5->SetFontColor(RGB(255,255,255)| 0xFF000000,0);

			GENDATA->CreateInstance(pPlanetFontType, pBase);
			CQASSERT(pBase!=0);
			pBase->QueryInterface("IFontDrawAgent", pPlanetFont6);
			pPlanetFont6->SetFontColor(RGB(255,255,255)| 0xFF000000,0);

			GENDATA->CreateInstance(pPlanetFontType, pBase);
			CQASSERT(pBase!=0);
			pBase->QueryInterface("IFontDrawAgent", pPlanetFont8);
			pPlanetFont8->SetFontColor(RGB(255,255,255)| 0xFF000000,0);

			pPlanetFontType2 = GENDATA->LoadArchetype("Font!!PlanetTitle");
			CQASSERT(pPlanetFontType2);
			GENDATA->AddRef(pPlanetFontType2);

			GENDATA->CreateInstance(pPlanetFontType2, pBase);
			CQASSERT(pBase!=0);
			pBase->QueryInterface("IFontDrawAgent", pPlanetFont7);
			pPlanetFont7->SetFontColor(RGB(65,212,228)| 0xFF000000,0);

			pPlanetShapeType = GENDATA->LoadArchetype("VFXShape!!PlanetBar");
			CQASSERT(pPlanetShapeType);
			GENDATA->AddRef(pPlanetShapeType);

			COMPTR<IShapeLoader> pPlanetShapeLoader;

			GENDATA->CreateInstance(pPlanetShapeType, pBase);
			pBase->QueryInterface("IShapeLoader", pPlanetShapeLoader);
			
			pPlanetShapeLoader->CreateDrawAgent(0, planetShape[0]);
			pPlanetShapeLoader->CreateDrawAgent(1, planetShape[1]);
			pPlanetShapeLoader->CreateDrawAgent(2, planetShape[2]);
			pPlanetShapeLoader->CreateDrawAgent(3, planetShape[3]);
			pPlanetShapeLoader->CreateDrawAgent(4, metalShape);
			pPlanetShapeLoader->CreateDrawAgent(5, gasShape);
			pPlanetShapeLoader->CreateDrawAgent(6, crewShape);
			pPlanetShapeLoader->CreateDrawAgent(7, t_backgroundShape);
			pPlanetShapeLoader->CreateDrawAgent(8, m_backgroundShape);
			pPlanetShapeLoader->CreateDrawAgent(9, s_backgroundShape);
		}

		PLANETTEXMEMUSED += (TEXMEMORYUSED-lastTexMem);

		if(data->teraParticle[0])
		{
			DAFILEDESC fdesc = data->teraParticle;
			
			fdesc.lpImplementation = "UTF";

			COMPTR<IFileSystem> file;
			
			if (OBJECTDIR->CreateInstance(&fdesc, file) == GR_OK)
			{
				 result->teraRingAnim = ANIM2D->create_archetype(file);
			}
		}

		if(data->teraExplosions[0])
		{
			result->teraExplosion = ARCHLIST->LoadArchetype(data->teraExplosions);
		}

		//build the global mesh
		for(U32 i = 0; i < HALO_SEGMENTS; ++i)
		{
			haloMesh.ringCenter[i] = Vector(cos((2*PI*i)/HALO_SEGMENTS),sin((2*PI*i)/HALO_SEGMENTS),-1);
			haloMesh.upperRing[i] = Vector(cos((2*PI*(i+0.5))/HALO_SEGMENTS),sin((2*PI*(i+0.5))/HALO_SEGMENTS),0);
		}
		
		haloMesh.RestoreVertexBuffers();

	}

	return (HANDLE) result;
}
//-------------------------------------------------------------------
//
BOOL32 PlanetFactory::DestroyArchetype (HANDLE hArchetype)
{
	OBJTYPE * objtype = (OBJTYPE *)hArchetype;

	EditorStopObjectInsertion(objtype->pArchetype);
	delete objtype;

	if(pPlanetFont1)
	{
		pPlanetFont1.free();
		pPlanetFont2.free();
		pPlanetFont3.free();
		pPlanetFont4.free();
		pPlanetFont5.free();
		pPlanetFont6.free();
		pPlanetFont7.free();
		pPlanetFont8.free();
		GENDATA->Release(pPlanetFontType);
		GENDATA->Release(pPlanetFontType2);

		planetShape[0].free();
		planetShape[1].free();
		planetShape[2].free();
		planetShape[3].free();
		metalShape.free();
		gasShape.free();
		crewShape.free();
		t_backgroundShape.free();
		m_backgroundShape.free();
		s_backgroundShape.free();
		GENDATA->Release(pPlanetShapeType);
	}

	PLANETTEXMEMUSED = 0;
	return 1;
}
//-------------------------------------------------------------------
//
IBaseObject * PlanetFactory::CreateInstance (HANDLE hArchetype)
{
	OBJTYPE * objtype = (OBJTYPE *)hArchetype;
	return createPlanetoid(*objtype);
}
//-------------------------------------------------------------------
//
void PlanetFactory::EditorCreateInstance (HANDLE hArchetype, const MISSION_INFO & info)
{
	OBJTYPE * objtype = (OBJTYPE *)hArchetype;
	EditorStartObjectInsertion(objtype->pArchetype, info);
}

//--------------------------------------------------------------------------//
// receive notifications from event system
//
GENRESULT PlanetFactory::Notify (U32 message, void *param)
{
//	MSG *msg = (MSG *) param;

	switch (message)
	{
	case CQE_ENTERING_INGAMEMODE:
		loadTextures(true);
		break;
	case CQE_LEAVING_INGAMEMODE:
		loadTextures(false);
		break;

	case CQE_CAMERA_MOVED:
		camMoved++;
		break;

/*	case WM_COMMAND:
		if (LOWORD(msg->wParam) == menuID)
		{
			viewer->set_display_state(1);
		}
		break;*/
	}

	return GR_OK;
}
//--------------------------------------------------------------------------
//
void PlanetFactory::loadTextures (bool bLoad)
{
	if (bLoad)
	{
		DAFILEDESC fdesc = "point.anm";
		COMPTR<IFileSystem> file;
		if (OBJECTDIR->CreateInstance(&fdesc, file) == GR_OK)
		{
			pointAnimArch = ANIM2D->create_archetype(file);
		}
		load_global_slots();
	}
	else
	{
		unload_global_slots();
		delete pointAnimArch;
		pointAnimArch = NULL;
	}
}
//---------------------------------------------------------------------------
//-------------------------GLOBAL STARTUP------------------------------------
//---------------------------------------------------------------------------
//
struct _planet : GlobalComponent
{
	PlanetFactory * planet;

	virtual void Startup (void)
	{
		planet = new DAComponent<PlanetFactory>;
		AddToGlobalCleanupList((IDAComponent **) &planet);
	}

	virtual void Initialize (void)
	{
		planet->init();
	}
};

static _planet planet;
//---------------------------------------------------------------------------
//--------------------------End Planetoid.cpp--------------------------------
//---------------------------------------------------------------------------
