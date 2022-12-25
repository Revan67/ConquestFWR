//--------------------------------------------------------------------------//
//                                                                          //
//                               Jumpgate.cpp                              //
//                                                                          //
//                  COPYRIGHT (C) 1997 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/jumpgate.cpp 158   9/10/01 2:05p Tmauer $
*/			    
//------------------------------- #INCLUDES --------------------------------//
//--------------------------------------------------------------------------//

#include "pch.h"
#include <globals.h>

#include "TObjFrame.h"
#include "TDocClient.h"
#include "TObject.h"
#include "ObjList.h"
#include "Camera.h"
#include "SuperTrans.h"
#include "Sector.h"
#include "UserDefaults.h"
#include "ANIM2D.h"
//next 4 for factory
#include "TResclient.h"
#include "IConnection.h"
#include "Startup.h"
#include "resource.h"
#include "TObjTrans.h"
#include "TObjSelect.h"
#include "TObjMission.h"
#include <DJumpGate.h>
#include "IJumpGate.h"
#include "Mission.h"
#include "CQLight.h"
#include "SFX.h"
#include "ICamera.h"
#include "MyVertex.h"
#include "Anim2d.h"
#include "IMorphMesh.h"
#include "BBMesh.h"
#include "FogOfWar.h"
#include "SysMap.h"
#include "GridVector.h"
#include "TerrainMap.h"
#include <DQuickSave.h>
#include <DJumpSave.h>
#include "TManager.h"
#include "ObjMap.h"
#include "IUnbornMeshList.h"
#include "DPlatform.h"

#include <Engine.h>
#include <TSmartPointer.h>
#include <HKEvent.h>
#include <3DMath.h>
#include <Physics.h>
#include <FileSys.h>
#include <WindowManager.h>
#include <ITextureLibrary.h>
#include <Renderer.h>
#include <Mesh.h>
#include <EventSys.h>

#include <stdlib.h>

#define NUM_LINE_SEGS 30

Vector jumpGateCircle[NUM_LINE_SEGS];

#define NUM_DOTS 20
#define RIM_DOTS 200
#define NUM_FLARES 50
#define RADIUS 1600

struct ColorRGB
{
	U8 r,g,b;
};

struct SortVertex
{
	Vector p;
	U32 v_idx;
	U32 m_idx;
	Parameters *params;
};
/*
// BILLBOARD MESH CLASS
struct Billboard
{
	S32 size;
	SINGLE rotation;
	SINGLE frameOffset;
};

struct BB_Mesh
{
	U32 bb_txm;
	AnimArchetype *animArch;
	Billboard *bb_data;
	U32 *v_sort;
	Vector *p_sort;
	Parameters params;
	SINGLE timer;
	COMPTR<IMorphMesh> morphMesh;
	U32 numVerts;
	bool bAdditive;

	void * operator new (size_t size)
	{
		return HEAP->ClearAllocateMemory(size, "BB_Mesh");
	}

	void Update(SINGLE dt);

	void SetMesh(MorphMeshArchetype *mm_arch,BILLBOARD_MESH *data);

	void SetMesh(ARCHETYPE_INDEX id,BILLBOARD_MESH *data);

	ARCHETYPE_INDEX GetMeshID();

	SINGLE GetMeshes(Mesh **mesh1,Mesh **mesh2);

	S32 GetVertices(Vector *vertices,Parameters *params);


	BB_Mesh();

	~BB_Mesh (void);
	
protected:
	ARCHETYPE_INDEX bb_mesh;
};*/

BB_Mesh::BB_Mesh()
{
	bb_mesh = INVALID_ARCHETYPE_INDEX;
	numVerts = 0;
}

BB_Mesh::~BB_Mesh (void)
{
	delete [] bb_data;
	bb_data = 0;
	delete [] v_sort;
	v_sort = 0;
	delete [] p_sort;
	p_sort = 0;
}

void BB_Mesh::Update(SINGLE dt)
{
	timer += dt;
	if (morphMesh)
		morphMesh->Update(dt);
}

void BB_Mesh::SetMesh(MorphMeshArchetype *mm_arch,BILLBOARD_MESH *data)
{
	CreateMorphMesh(morphMesh,mm_arch);
	Mesh *mesh = morphMesh->GetMesh0();
	CQASSERT(mesh);
	if (bb_data)
	{
		delete [] v_sort;
		delete [] p_sort;
		delete [] bb_data;
	}
	bb_data = new Billboard[mesh->object_vertex_cnt];
	v_sort = new U32[mesh->object_vertex_cnt];
	p_sort = new Vector[mesh->object_vertex_cnt];
	numVerts = mesh->object_vertex_cnt;
	for (int i=0;i<mesh->object_vertex_cnt;i++)
	{
		//for convenience the size is cut to a "radius" type size
		if (data->size_max > data->size_min)
			bb_data[i].size = 0.5*(data->size_min+rand()%(data->size_max-data->size_min));
		else
			bb_data[i].size = 0.5*data->size_min;
		bb_data[i].rotation = (rand()%360)*PI/180.0;
		if (animArch)
		{
			CQASSERT(animArch->frame_cnt);
			bb_data[i].frameOffset = (rand()%(animArch->frame_cnt*100))/100.0;
		}
	}
}

void BB_Mesh::SetMesh(ARCHETYPE_INDEX id,BILLBOARD_MESH *data)
{
	bb_mesh = id;
	Mesh *mesh = REND->get_archetype_mesh(id);
	CQASSERT(mesh);
	if (bb_data)
	{
		delete [] v_sort;
		delete [] p_sort;
		delete [] bb_data;
	}
	bb_data = new Billboard[mesh->object_vertex_cnt];
	v_sort = new U32[mesh->object_vertex_cnt];
	p_sort = new Vector[mesh->object_vertex_cnt];
	numVerts = mesh->object_vertex_cnt;
	for (int i=0;i<mesh->object_vertex_cnt;i++)
	{
		//for convenience the size is cut to a "radius" type size
		if (data->size_max > data->size_min)
			bb_data[i].size = 0.5*(data->size_min+rand()%(data->size_max-data->size_min));
		else
			bb_data[i].size = 0.5*data->size_min;
		bb_data[i].rotation = (rand()%360)*PI/180.0;
		if (animArch)
		{
			CQASSERT(animArch->frame_cnt);
			bb_data[i].frameOffset = (rand()%(animArch->frame_cnt*100))/100.0;
		}
	}
}

ARCHETYPE_INDEX BB_Mesh::GetMeshID()
{
	return bb_mesh;
}

SINGLE BB_Mesh::GetMeshes(Mesh **mesh1,Mesh **mesh2)
{
	if (morphMesh)
		return morphMesh->GetMorphPos(mesh1,mesh2);
	else
	{
		*mesh1 = REND->get_archetype_mesh(bb_mesh);
		*mesh2 = 0;
		return 0;
	}
}

S32 BB_Mesh::GetVertices(Vector *vertices,Parameters *params)
{
	if (morphMesh)
		return morphMesh->GetVertices(vertices,params);
	else
	{
		Mesh *mesh = REND->get_archetype_mesh(bb_mesh);
		if (mesh)
		{
			for (int i=0;i<mesh->object_vertex_cnt;i++)
			{
				vertices[i] = mesh->object_vertex_list[i];	
			}
				
			params->scale = 1.0;

			return mesh->object_vertex_cnt;
		}

		return 0;
	}
}

S32 BB_Mesh::GetMesh0(Vector *vertices,Parameters *params)
{
	Mesh *mesh;
	if (morphMesh)
		 mesh = morphMesh->GetMesh0();
	else
		mesh = REND->get_archetype_mesh(bb_mesh);
	
	if (mesh)
	{
		for (int i=0;i<mesh->object_vertex_cnt;i++)
		{
			vertices[i] = mesh->object_vertex_list[i];	
		}
		
		params->scale = 1.0;
		
		return mesh->object_vertex_cnt;
	}
	
	return 0;
}
///////////////////////////////////////////////
struct Jumper
{
	OBJPTR<IBaseObject> obj;
	U32 playerID;
	SINGLE tick;
	SINGLE jumpTime;
	SINGLE aim;
	Jumper *next;

	Jumper::Jumper()
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

	void destroy (void)
	{
		delete this;
	}

protected: // can only create heap Jumper objects
	~Jumper (void)
	{
	}
};


	
U32 flashTexID=0;
static Vector *ring;
static Vector *sphere;
static Vector *cylinder;
static TexCoord *tex;
static ColorRGB *ringColors;
static SINGLE maxY,minY;

struct JumpgateArchetype
{
	const char *name;
	BT_JUMPGATE_DATA *pData;
	ARCHETYPE_INDEX archIndex,arche2;
	IMeshArchetype * meshArch;
	PARCHETYPE pArchetype;
	AnimArchetype *dormantArch;
	U32 mapTex;
	ARCHETYPE_INDEX bb_mesh_archID[MAX_BB_MESHES];
	U32 bb_txm[MAX_BB_MESHES];
	AnimArchetype *bb_anim_arch[MAX_BB_MESHES];
	MorphMeshArchetype *mm_arch[MAX_BB_MESHES];
	U32 windowTexID;

    void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}

	JumpgateArchetype (void)
	{
		meshArch = NULL;
		archIndex = -1;
		arche2 = -1;
		mapTex = -1;
		windowTexID = 0;
		for (int i=0;i<MAX_BB_MESHES;i++)
		{
			bb_txm[i] = 0;
			bb_mesh_archID[i] = INVALID_ARCHETYPE_INDEX;
		}
	}


	~JumpgateArchetype (void)
	{
		if (dormantArch)
		{
			delete dormantArch;
			dormantArch = 0;
		}

		ENGINE->release_archetype(archIndex);
		ENGINE->release_archetype(arche2);
		for (int i=0;i<MAX_BB_MESHES;i++)
		{
			if (bb_anim_arch[i])
			{
				delete bb_anim_arch[i];
				bb_anim_arch[i] = 0;
			}

			ENGINE->release_archetype(bb_mesh_archID[i]);
			if (mm_arch[i])
			{
				DestroyMorphMeshArchetype(mm_arch[i]);
				mm_arch[i] = 0;
			}
		}

		TMANAGER->ReleaseTextureRef(windowTexID);
	}

};

void Render3db(Mesh *mesh,struct ColorRGB *color,const Transform &trans,SINGLE colorDamp);
void vector_rotate_about_k (Vector *vec,SINGLE angle);
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
enum JUMP_STAGE
{
	JS_IDLE,
	JS_OPENING,
	JS_OPEN,
	JS_CLOSING
};

#define OPENING_TIME 1.0
#define	OPEN_TIME 3.0

struct _NO_VTABLE Jumpgate : public ObjectSelection
										<ObjectMission
												<ObjectTransform<ObjectFrame<IBaseObject,JUMPGATE_SAVELOAD,JumpgateArchetype> >
												>
										>, 
									ISaveLoad, IJumpGate, IQuickSaveLoad
{
	BEGIN_MAP_INBOUND(Jumpgate)
	_INTERFACE_ENTRY(IBaseObject)
	_INTERFACE_ENTRY(IMissionActor)
	_INTERFACE_ENTRY(ISaveLoad)
	_INTERFACE_ENTRY(IJumpGate)
	_INTERFACE_ENTRY(IQuickSaveLoad)
	_INTERFACE_ENTRY(IPhysicalObject)
	END_MAP()


	//BILLBOARD MESH STUFF
	BB_Mesh bb[MAX_BB_MESHES];
	SortVertex *s_sort;
	U32 *v_sort;
	U32 v_pos;
	U32 sort_size;
	int numMeshes;

	//NEW STAGE TIMING STUFF
	JUMP_STAGE jumpStage;
	SINGLE timer;
	//time for gate to stay open
	SINGLE hangTime;

	//NEW AUXILIARY EFFECTS
	U32 windowTexID;

	//DISORGANIZED CRAP
	CQLight lights[2];
	S32 offsetX[2],offsetY[2];
	SINGLE dirX[2],dirY[2];
	BOOL32 lActive[2];
	SINGLE timeToLive[2];
	U32 texture,rim_tex,flare_tex;
	SINGLE uCount;
	SINGLE jumpTimer;
	Jumper *jumper;
	U32 mapTex;
	BT_JUMPGATE_DATA *data;
	SINGLE time_until_last_jump;
	ARCHETYPE_INDEX arche2;

	AnimInstance anim;

	BOOL32 bDeleteRequested:1;
	bool bJumpAllowed:1;
	bool bRenderBuild:1;

	INSTANCE_INDEX ringIdx;
	SINGLE baseAim;
	Jumper *last_jumper,*next_jumper;
	BOOL32 bAim:1;
	SINGLE baseTick;
	JumpgateInfo info;
	SINGLE ick;
	SINGLE turnAngle;
	SINGLE emissionTimer;
	SINGLE theta;
	BOOL32 bDormant:1;
	HSOUND hAmbientSound;
	HSOUND hEnterSound[4];
	U32 enterSoundCnt;
	HSOUND hArriveSound[4];
	U32 arriveSoundCnt;

	U32 ownerID;
	bool bLocked;
	U8 marks;
	U8  visMarks;
	U8	shadowVisibilityFlags;
	bool bInvisible;


	//objmap
	U32 map_sys,map_square;


	//----------------------------------
	
	Jumpgate (void);

    void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}

	virtual ~Jumpgate (void);	

	/* IBaseObject methods */

	virtual BOOL32 Update (void);

	virtual void PhysicalUpdate (SINGLE dt);

	virtual void CastVisibleArea (void);

	void StashVertices(U32 mesh_id);

	void Billboard3db(BLENDS blendMode);

	virtual void Render (void);

	virtual void MapRender (bool bPing);

	virtual void View (void);

	virtual void UpdateVisibilityFlags (void);
	
	virtual void UpdateVisibilityFlags2 (void);

	virtual void init (JumpgateArchetype *data);

	virtual SINGLE TestHighlight (const RECT & rect);	// set bHighlight if possible for any part of object to appear within rect

	void TestVisible (const USER_DEFAULTS & defaults, const U32 currentSystem, const U32 currentPlayer)
	{
		bVisible = (GetSystemID() == currentSystem && ((GetVisibilityFlags()&MGlobals::GetAllyMask(currentPlayer))!=0 || defaults.bEditorMode || defaults.bVisibilityRulesOff));
		if (bVisible && (bVisible = (instanceIndex != INVALID_INSTANCE_INDEX)) != 0)
		if (bVisible)
		{
			float depth, center_x, center_y, radius;
			bVisible = REND->get_instance_projected_bounding_sphere(instanceIndex, MAINCAM, LODPERCENT, center_x, center_y, radius, depth);		
		}
	}


	virtual void DrawHighlighted (void);

	/* ISaveLoad */

	virtual BOOL32 Save (struct IFileSystem * file);

	virtual BOOL32 Load (struct IFileSystem * file);

	virtual void ResolveAssociations();

	/* IQuickSaveLoad methods */
	
	virtual void QuickSave (struct IFileSystem * file);

	virtual void QuickLoad (const char *szSaveLoadType, const char *szInstanceName, void * buffer, U32 bufferSize);

	virtual void QuickResolveAssociations (void);

	/* Jumpgate methods */

	void renderCircle();
	
	void createLights ();

//	void CylinderToRing (void);
//void ConeToSphere (void);

	virtual SINGLE JumpOut(IBaseObject *obj,SINGLE time);

	virtual SINGLE JumpIn(IBaseObject *obj,SINGLE arrivalTime,const Vector &dir);

	virtual void InitWith(U32 systemID,Vector pos,U32 gateID,U32 exit_gateID);

	virtual bool IsJumpAllowed();
	
	virtual void SetJumpAllowed(bool bAllowed);

	virtual bool PlayerCanJump(U32 playerID);

	virtual void Mark(U32 playerID);

	virtual void Unmark(U32 playerID);

	virtual bool IsHighlightingBuild();

	virtual void Lock();

	virtual void Unlock();

	virtual bool IsLocked();	

	virtual bool CanIBuild(U32 playerID);
	
	virtual void SetOwner(U32 missionID);

	virtual void UnsetOwner(U32 missionID);

	virtual U32 GetPlayerOwner (void);

	virtual struct JumpgateInfo *GetJumpgateInfo()
	{
		return &info;
	}

	virtual bool IsOwnershipKnownToPlayer (U32 playerID);

	virtual void SetJumpgateInvisible (bool bEnable);

	virtual void SetPosition (const Vector & position, U32 newSystemID)
	{
		enableTerrainFootprint(false);
		systemID = newSystemID;
		transform.translation = position;
		lights[0].setSystem(newSystemID);
		lights[0].set_position(Vector(position.x,position.y,position.z+2000));
		anim.SetPosition(position);
		SFXMANAGER->Play(hAmbientSound,systemID,&transform.translation,SFXPLAYF_LOOPING|SFXPLAYF_NOFOG);
		enableTerrainFootprint(true);
	}

	virtual void SetTransform (const TRANSFORM & trans, U32 newSystemID)
	{
		enableTerrainFootprint(false);
		systemID = newSystemID;
		transform = trans;
		Vector t=trans.translation;
		lights[0].setSystem(newSystemID);
		lights[0].set_position(Vector(t.x,t.y,t.z+2000));
		anim.SetPosition(t);
		SFXMANAGER->Play(hAmbientSound,systemID,&transform.translation,SFXPLAYF_LOOPING|SFXPLAYF_NOFOG);
		enableTerrainFootprint(true);
	}

	void enableTerrainFootprint (bool bEnable);

	bool containsAlliedUnit (void);
};

//---------------------------------------------------------------------------
//
Jumpgate::Jumpgate (void) 
{
	baseTick = -1;
	mapTex = 0;
}
//---------------------------------------------------------------------------
//
Jumpgate::~Jumpgate (void)
{
	enableTerrainFootprint(false);

	if (ringIdx)
		ENGINE->destroy_instance(ringIdx);

	SFXMANAGER->CloseHandle(hAmbientSound);
	SFXMANAGER->CloseHandle(hEnterSound[0]);
	SFXMANAGER->CloseHandle(hEnterSound[1]);
	SFXMANAGER->CloseHandle(hEnterSound[2]);
	SFXMANAGER->CloseHandle(hEnterSound[3]);
	SFXMANAGER->CloseHandle(hArriveSound[0]);
	SFXMANAGER->CloseHandle(hArriveSound[1]);
	SFXMANAGER->CloseHandle(hArriveSound[2]);
	SFXMANAGER->CloseHandle(hArriveSound[3]);

	if (s_sort)
		delete [] s_sort;
	if (v_sort)
		delete [] v_sort;

	while(jumper)
	{
		Jumper * j = jumper;
		jumper = jumper->next;
		j->destroy();
	}

//	ENGINE->destroy_instance(flash_index[1]);
}
//---------------------------------------------------------------------------
//
void Jumpgate::enableTerrainFootprint (bool bEnable)
{
	FootprintInfo fpi;
	fpi.flags = TERRAIN_FULLSQUARE | TERRAIN_PARKED;
	fpi.height = 400;
	fpi.missionID = dwMissionID;

	// KLUDGE! KLUDGE! KLUDGE!!!
	Vector cposition(transform.translation.x + 50.1f, transform.translation.y + 50.1f, transform.translation.z + 50.1f);
	GRIDVECTOR vec;
	vec = cposition;
	vec.centerpos();

	// get the TerrainMap pointer
	COMPTR<ITerrainMap> map;
	SECTOR->GetTerrainMap(GetSystemID(), map);

	if (bEnable)
	{
		map->SetFootprint(&vec, 1, fpi); 
		CQASSERT(map_sys==0);		// assume that we removed old node first
		map_square = OBJMAP->GetMapSquare(systemID,transform.translation);
		map_sys = systemID;
		OBJMAP->AddObjectToMap(this,map_sys,map_square,OM_SYSMAP_FIRSTPASS);
	}
	else
	{	
		map->UndoFootprint(&vec, 1, fpi);

		if (map_sys)
		{
			OBJMAP->RemoveObjectFromMap(this,map_sys,map_square);
			map_sys = map_square = 0;
		}
	}
}
//---------------------------------------------------------------------------
//
void Jumpgate::DrawHighlighted (void)
{
	if (nextHighlighted==0 && OBJLIST->GetHighlightedList()==this)
	{
		Vector point;
		S32 x, y;

		point.x = 0;
		point.y = box[2]+250.0;
		point.z = 0;


		COMPTR<IFontDrawAgent> pFont;
		if(DEFAULTS->GetDefaults()->bEditorMode)
		{
			CAMERA->PointToScreen(point, &x, &y, &transform);
			DEBUGFONT->StringDraw(CAMERA->GetPane(), x-20, y+10, partName, RGB(180,180,180) | 0xFF000000);		// testing!!! fix this GetDisplayName()
		}
		else
		{
			if (OBJLIST->GetUnitFont(pFont) == GR_OK)
			{
				WM->GetCursorPos(x, y);
				y += IDEAL2REALY(24);
				pFont->SetFontColor(RGB(140,140,180) | 0xFF000000, 0);
				wchar_t buffer[128];
				wchar_t buffer2[128];
				wchar_t nameBuffer[128];
				wcsncpy(buffer, _localLoadStringW(IDS_JUMPGATE_NAME), sizeof(buffer)/sizeof(wchar_t));
				SECTOR->GetSystemName(nameBuffer,sizeof(nameBuffer),SECTOR->GetJumpgateDestination(this)->GetSystemID());
				swprintf(buffer2,buffer,nameBuffer);
				pFont->StringDraw(CAMERA->GetPane(), x-20, y+10, buffer2);		// testing!!! fix this GetDisplayName()
			}
		}
	}
}
//---------------------------------------------------------------------------
//  returns total warp time
//
SINGLE Jumpgate::JumpOut (IBaseObject *obj,SINGLE time)
{
	if (jumpStage == JS_OPEN)
	{
		timer = max(OPEN_TIME,timer);
	}
	else
		hangTime = max(OPEN_TIME,hangTime);

	Vector dir = transform.translation-obj->GetPosition();
	Jumper * newJumper = new Jumper;
	obj->QueryInterface(IBaseObjectID, newJumper->obj, NONSYSVOLATILEPTR);
	newJumper->playerID = obj->GetPlayerID();
	if (time == 0)
	{
		//	newJumper->jumpTime = dir.magnitude()*(JUMP_TIME/MAX_WARP_DISTANCE);
		//		if (newJumper->jumpTime < 3)
		//			newJumper->jumpTime = 3;
		newJumper->jumpTime = JUMP_TIME+PRE_JUMP_TIME;
	}
	else
		newJumper->jumpTime = time;
	
	CQASSERT(newJumper->jumpTime > 1e-2);
	
	newJumper->tick = jumpTimer+newJumper->jumpTime;
	newJumper->aim = atan2(dir.x,dir.y);
	//HALF OF WHAT WE NEED
/*	if (newJumper->aim > PI*0.5)
		newJumper->aim -= PI;
	if (newJumper->aim < -PI*0.5)
		newJumper->aim += PI;*/
	//WHAT WE NEED
	if (newJumper->aim > PI)
		newJumper->aim -= PI*2;
	if (newJumper->aim < -PI)
		newJumper->aim += PI*2;
	
	//add to list
	if (!jumper)
	{
		jumper = newJumper;
	}
	else
	{
		Jumper *pos;
		pos = jumper;
		while (pos->next)
		{
			pos = pos->next;
		}
		pos->next = newJumper;
	}
	
	bool bAllied = containsAlliedUnit();
	bool bRevealed = (bAllied || (FOGOFWAR->CheckVisiblePosition(transform.translation) != 0));
	U32 flags = bRevealed ? (SFXPLAYF_NODROP | SFXPLAYF_NOFOG) : 0;
	enterSoundCnt = (enterSoundCnt+1)%4;
	SFXMANAGER->Play(hEnterSound[enterSoundCnt],systemID,&transform.translation, flags);


	if (jumpStage == JS_IDLE || jumpStage == JS_CLOSING)
	{
		for (int i=0;i<MAX_BB_MESHES;i++)
		{
			if (bb[i].morphMesh != 0)
			{
				bb[i].morphMesh->MorphTo(1,0.3,FALSE);
				bb[i].morphMesh->QueueAnim(2,0.5,TRUE);
			}
		}
		jumpStage = JS_OPENING;
		timer = OPENING_TIME;
		//adjust theta for smooth funneling
		theta += baseAim-newJumper->aim;
		//set new funnel target
		baseAim = newJumper->aim;
	}

	return newJumper->jumpTime;

}
//---------------------------------------------------------------------------
//
SINGLE Jumpgate::JumpIn (IBaseObject *obj,SINGLE arrivalTime,const Vector &dir)
{
	SINGLE result;
		
	result = max(data->min_hold_time+arrivalTime,time_until_last_jump+data->min_stagger_time);

	time_until_last_jump = result;
	
	//timer = OPEN_TIME*0.5+time_until_last_jump;
	//jumpStage = JS_OPEN;

	if (jumpStage == JS_OPEN)
	{
		timer = max(OPEN_TIME*0.5+result,timer);
	}
	else
		hangTime = max(OPEN_TIME*0.5+result,hangTime);

//	Vector dir = transform.translation-obj->GetPosition();
	Jumper * newJumper = new Jumper;
	obj->QueryInterface(IBaseObjectID, newJumper->obj, NONSYSVOLATILEPTR);
	newJumper->playerID = obj->GetPlayerID();
	
	newJumper->jumpTime = result;//time_until_last_jump;
	
	CQASSERT(newJumper->jumpTime > 1e-2);
	
	newJumper->tick = jumpTimer+newJumper->jumpTime;
	newJumper->aim = atan2(-dir.x,-dir.y);
	//HALF OF WHAT WE NEED
/*	if (newJumper->aim > PI*0.5)
		newJumper->aim -= PI;
	if (newJumper->aim < -PI*0.5)
		newJumper->aim += PI;*/
	//WHAT WE NEED
	if (newJumper->aim > PI)
		newJumper->aim -= PI*2;
	if (newJumper->aim < -PI)
		newJumper->aim += PI*2;
	
	//add to list
	if (!jumper)
	{
		jumper = newJumper;
	}
	else
	{
		Jumper *pos;
		pos = jumper;
		while (pos->next)
		{
			pos = pos->next;
		}
		pos->next = newJumper;
	}


	bool bAllied = containsAlliedUnit();
	bool bRevealed = (bAllied || (FOGOFWAR->CheckVisiblePosition(transform.translation) != 0));
	U32 flags = bRevealed ? (SFXPLAYF_NODROP | SFXPLAYF_NOFOG) : 0;

	arriveSoundCnt = (arriveSoundCnt+1)%4;
	SFXMANAGER->Play(hArriveSound[arriveSoundCnt],systemID,&transform.translation, flags);

	if (jumpStage == JS_IDLE || jumpStage == JS_CLOSING)
	{
		for (int i=0;i<MAX_BB_MESHES;i++)
		{
			if (bb[i].morphMesh != 0)
			{
				bb[i].morphMesh->MorphTo(1,0.3,FALSE);
				bb[i].morphMesh->QueueAnim(2,0.5,TRUE);
			}
		}
		jumpStage = JS_OPENING;
		timer = OPENING_TIME;
		theta += baseAim-newJumper->aim;
		//set new funnel target
		baseAim = newJumper->aim;
	}
	
	return result-arrivalTime;//newJumper->jumpTime;
}
//---------------------------------------------------------------------------
//
bool Jumpgate::containsAlliedUnit (void)
{
	Jumper * node = jumper;
	bool result = false;
	const U32 allyMask = MGlobals::GetAllyMask(MGlobals::GetThisPlayer()) << 1;

	while (node)
	{
		if (((1 << node->playerID) & allyMask) != 0)
		{
			result = true;
			break;
		}
		node = node->next;
	}

	return result;
}
//---------------------------------------------------------------------------
//
void Jumpgate::InitWith (U32 _systemID,Vector pos,U32 gateID,U32 exit_gateID)
{
	SetPosition(pos, _systemID);
	info.gate_id = gateID;
	info.exit_gate_id = exit_gateID;
}
//---------------------------------------------------------------------------
//
bool Jumpgate::IsJumpAllowed (void)
{
	return bJumpAllowed;
}
//---------------------------------------------------------------------------
//
void Jumpgate::SetJumpAllowed (bool bAllowed)
{
	bJumpAllowed = bAllowed;
}
//---------------------------------------------------------------------------
//
bool Jumpgate::PlayerCanJump (U32 playerID)
{
	if(ownerID && IsLocked())
		return MGlobals::AreAllies(playerID,(ownerID&PLAYERID_MASK));
	return true;
}
//---------------------------------------------------------------------------
//
void Jumpgate::Mark (U32 playerID)
{
	marks |= (0x01 << (playerID-1));
}
//---------------------------------------------------------------------------
//
void Jumpgate::Unmark (U32 playerID)
{
	marks &= ~(0x01 << (playerID-1));
}
//---------------------------------------------------------------------------
//
bool Jumpgate::IsHighlightingBuild (void)
{
	return bRenderBuild;
}
//---------------------------------------------------------------------------
//
void Jumpgate::Lock (void)
{
	bLocked = true;
}
//---------------------------------------------------------------------------
//
void Jumpgate::Unlock (void)
{
	bLocked = false;
}
//---------------------------------------------------------------------------
//
bool Jumpgate::IsLocked (void)
{
	return bLocked;
}
//---------------------------------------------------------------------------
//
bool Jumpgate::CanIBuild (U32 playerID)
{
	if(ownerID)
	{
		IBaseObject * obj;
		obj = OBJLIST->FindObject(ownerID);
		if(obj && obj->IsVisibleToPlayer(playerID))
		{
			return false;
		}
	}
	return true;
}
//---------------------------------------------------------------------------
//
bool Jumpgate::IsOwnershipKnownToPlayer (U32 playerID)
{
	bool result = true;

	if (ownerID)
	{
		IBaseObject * obj;
		obj = OBJLIST->FindObject(ownerID);
		if(obj && obj->IsVisibleToPlayer(playerID)==0)
		{
			return false;
		}
	}

	return result;
}
//---------------------------------------------------------------------------
//
void Jumpgate::SetJumpgateInvisible (bool bEnable)
{
	bInvisible = bEnable;
}
//---------------------------------------------------------------------------
//
void Jumpgate::SetOwner (U32 missionID)
{
	ownerID = missionID;
}
//---------------------------------------------------------------------------
//
void Jumpgate::UnsetOwner (U32 missionID)
{
	if(ownerID == missionID)
	{
		ownerID = 0;
		for(U32 i = 0; i < MAX_PLAYERS;++i)
		{
			if(!(shadowVisibilityFlags &(0x01 << i)))
			{
				visMarks &= ~(0x01 << i);
			}
		}
	}
}
//---------------------------------------------------------------------------
//
U32 Jumpgate::GetPlayerOwner (void)
{
	return (ownerID&PLAYERID_MASK);
}
//---------------------------------------------------------------------------
//
void Jumpgate::PhysicalUpdate(SINGLE dt)
{
//	bool bRevealed = (FOGOFWAR->CheckVisiblePosition(transform.translation) != 0);
	
/*	if (bRevealed == 0)
	{
		jumpStage = JS_IDLE;
		for (int i=0;i<MAX_BB_MESHES;i++)
		{
			if (bb[i].morphMesh != 0)
			{
				bb[i].morphMesh->RunAnim(0,true);
			}
		}
		goto UpdateOnly;
	}*/
	
	switch (jumpStage)
	{
	case JS_IDLE:
		theta += 0.2*dt;
		break;
	case JS_OPENING:
		{
			timer -= dt;
			if (timer < 0)
			{
				jumpStage = JS_OPEN;
				timer = hangTime;
			}
		}
		break;
	case JS_CLOSING:
		{
			timer -= dt;
			if (timer < 0)
			{
				jumpStage = JS_IDLE;
				hangTime = 0;
			}
		}
		break;
	case JS_OPEN:
		theta += 1.0*dt;
		timer -= dt;
		if (timer < 0)
		{
			for (int i=0;i<MAX_BB_MESHES;i++)
			{
				if (bb[i].morphMesh != 0)
				{
					bb[i].morphMesh->MorphTo(3,0.3,FALSE);
					bb[i].morphMesh->QueueAnim(0,0.5,TRUE);
				}
			}
			jumpStage = JS_CLOSING;
			timer = OPENING_TIME;
		}
		break;
	}

//UpdateOnly:
	FRAME_physicalUpdate(dt);
		
	anim.update(dt);

	int i;

	for (i=0;i<MAX_BB_MESHES;i++)
		bb[i].Update(dt);


	uCount += 0.03*dt;
	time_until_last_jump -= dt;

	jumpTimer += dt;
	emissionTimer += dt;

	Jumper *jumperPos = jumper;
	while (jumperPos)
	{
		if (jumpTimer-jumperPos->tick > jumperPos->jumpTime)
		{
			if (jumperPos == last_jumper)
				last_jumper = NULL;
			if (jumperPos == jumper)
			{
				jumper = jumperPos->next;
				jumperPos->destroy();
				jumperPos = NULL;
			}
			else
			{
				Jumper *pos2 = jumper;
				while (pos2->next != jumperPos)
				{
					pos2 = pos2->next;
				}
				pos2->next = jumperPos->next;
				
				jumperPos->destroy();
				jumperPos = pos2;
			}
		}

		if (jumperPos)
			jumperPos = jumperPos->next;
	}
}
//---------------------------------------------------------------------------------------
//
void Jumpgate::CastVisibleArea (void)
{
	if(DEFAULTS->GetDefaults()->fogMode == FOGOWAR_EXPLORED)
	{
		SetVisibleToAllies(0xFF);
	}
	else
		SetVisibleToAllies(GetVisibilityFlags());
}
//---------------------------------------------------------------------------------------
//
#define GRID_RAD 3800
#define INSIDE_RAD 2100
//
//
//---------------------------------------------------------------------------------------
//
BOOL32 Jumpgate::Update()
{
	CQASSERT(map_sys==systemID);		// must be on the OBJMAP!
	bool bRevealed = (FOGOFWAR->CheckVisiblePosition(transform.translation) != 0);

	if (bRevealed == 0)
		goto Done;

	FRAME_update();

	/*Vector pos;
	pos = GetTransform().get_position();

	Vector target;*/

//	if (warper == 0) bWarping = FALSE;
//	jumpTimer++;
//	emissionTimer++;
	if (fmod(jumpTimer,4) < ELAPSED_TIME && bDormant)
	{
		anim.SetRotation(2*PI*rand()/RAND_MAX);
		anim.Restart();
		emissionTimer = 0;
	}

Done:
	return (bDeleteRequested == 0);
}

void Jumpgate::StashVertices(U32 mesh_id)
{
//	U32 *v_sort = bb[mesh_id].v_sort;
	Vector cpos (MAINCAM->get_position());

	Vector *p_sort = bb[mesh_id].p_sort;
	Parameters *params = &bb[mesh_id].params;
	S32 numVerts;

	bool bRevealed = (FOGOFWAR->CheckVisiblePosition(transform.translation) != 0);

	if (bRevealed)
		numVerts = bb[mesh_id].GetVertices(p_sort,params);
	else
		numVerts = bb[mesh_id].GetMesh0(p_sort,params);

	int v_cnt;
	for (v_cnt=0;v_cnt<numVerts;v_cnt++)
	{
		//Vector p;
	//	p = (mesh1->object_vertex_list[v_cnt]+data->billboardMesh[mesh_id].offset)*(1-share);

	//	if (mesh2)
	//		p += (mesh2->object_vertex_list[v_cnt]+data->billboardMesh[mesh_id].offset)*(share);

		s_sort[v_pos].v_idx = v_cnt;
		s_sort[v_pos].m_idx = mesh_id;
		s_sort[v_pos].p = transform*(p_sort[v_cnt]+data->billboardMesh[mesh_id].offset);
		s_sort[v_pos].params = params;
	//	v_sort[v_pos] = (s_sort[v_cnt].p-cpos).magnitude();
		v_sort[v_pos] = -s_sort[v_cnt].p.y;
		v_pos++;
	}
}

void Jumpgate::Billboard3db(BLENDS blendMode)
{
	BATCH->set_render_state(D3DRS_ZENABLE,FALSE);
	BATCH->set_render_state(D3DRS_ZWRITEENABLE,FALSE);


/*	if (bb[0].animArch)
	{
//		BATCH->set_texture_stage_texture( 0,bb[0].animArch->frames[0].texture);
		SetupDiffuseBlend(bb[0].animArch->frames[0].texture,TRUE);
	}
	else
	//	BATCH->set_texture_stage_texture( 0,bb[0].bb_txm);
		SetupDiffuseBlend(bb[0].bb_txm,TRUE);*/

	BATCH->set_render_state(D3DRS_ALPHABLENDENABLE,TRUE);
	BATCH->set_render_state(D3DRS_DITHERENABLE,FALSE);
	if (blendMode == ONE_ONE || blendMode == ONE_INVSRC)
		BATCH->set_render_state(D3DRS_SRCBLEND,D3DBLEND_ONE);
	else
		BATCH->set_render_state(D3DRS_SRCBLEND,D3DBLEND_SRCALPHA);
	
	if (blendMode == ONE_ONE || blendMode == SRC_ONE)
		BATCH->set_render_state(D3DRS_DESTBLEND,D3DBLEND_ONE);
	else
		BATCH->set_render_state(D3DRS_DESTBLEND,D3DBLEND_INVSRCALPHA);

//	Vector a = CAMERA->GetPosition()-trans.translation;
//	a.normalize();
//	a= trans.inverse_rotate(a);

	Vector epos = transform.translation;

	Vector cpos (MAINCAM->get_position());
	
	Vector look (epos - cpos);
	
	Vector i (look.y, -look.x, 0);
	
	if (fabs (i.x) < 1e-5 && fabs (i.y) < 1e-5)
	{
		i.x = 1.0f;
	}
	
	i.normalize ();
	
	Vector k (-look);
	k.normalize ();
	Vector j (cross_product (k, i));
	
	Vector base_i,base_j;
	base_i = i;
	base_j = j;
//	i *= data->billboardMesh->size*0.5;
//	j *= data->billboardMesh->size*0.5;
	
	TRANSFORM trans;
//	trans.set_i(i);
//	trans.set_j(j);
	trans.set_k(k);
	//trans.translation = epos;
	
	CAMERA->SetModelView();
	
//	PB.Color4ub(100,100,100,255);
	PB.Color4ub(255,255,255,255);
	PB.Begin(PB_QUADS);
	
	RadixSort::sort(v_sort,v_pos);//sort_size);

	SINGLE alpha=0.4;
	for (int v_cnt=v_pos-1;v_cnt>=0;v_cnt--)
	{
		U32 idx = RadixSort::index_list[v_cnt];
		BB_Mesh *bbm = &bb[s_sort[idx].m_idx];
		Billboard *b = &bbm->bb_data[s_sort[idx].v_idx];
		Vector p;
	//	p = transform*(mesh->object_vertex_list[idx]+data->billboardMesh[mesh_id].offset);
		p = s_sort[idx].p;

		trans.set_i(base_i);
		trans.set_j(base_j);
		trans.rotate_about_k(b->rotation);
		i = trans.get_i();
		j = trans.get_j();

		Vector v[4];
		v[0] = p+b->size*s_sort[idx].params->scale*(-i-j);
		v[1] = p+b->size*s_sort[idx].params->scale*(i-j);
		v[2] = p+b->size*s_sort[idx].params->scale*(i+j);
		v[3] = p+b->size*s_sort[idx].params->scale*(-i+j);
		
		if (blendMode == ONE_ONE)
			PB.Color3ub(alpha*255,alpha*255,alpha*255);
		else
			PB.Color4ub(255,255,255,alpha*255);

		if (bbm->animArch)
		{
			AnimFrame * frame = &bbm->animArch->frames[S32(b->frameOffset+bbm->timer*bbm->animArch->capture_rate)%bbm->animArch->frame_cnt];
			PB.TexCoord2f (frame->x0,frame->y0);
			PB.Vertex3f (v[0].x, v[0].y, v[0].z);
			PB.TexCoord2f (frame->x1,frame->y0);
			PB.Vertex3f (v[1].x, v[1].y, v[1].z);
			PB.TexCoord2f (frame->x1,frame->y1);
			PB.Vertex3f (v[2].x, v[2].y, v[2].z);
			PB.TexCoord2f (frame->x0,frame->y1);
			PB.Vertex3f (v[3].x, v[3].y, v[3].z);
		}
		else
		{
			PB.TexCoord2f (0,0);
			PB.Vertex3f (v[0].x, v[0].y, v[0].z);
			PB.TexCoord2f (1,0);
			PB.Vertex3f (v[1].x, v[1].y, v[1].z);
			PB.TexCoord2f (1,1);
			PB.Vertex3f (v[2].x, v[2].y, v[2].z);
			PB.TexCoord2f (0,1);
			PB.Vertex3f (v[3].x, v[3].y, v[3].z);		
		}
		alpha+= 0.005;
	}
	PB.End();
}

void Jumpgate::renderCircle()
{
	COLORREF color = SECTORCOLORTABLE[SECTOR->GetJumpgateDestination(this)->GetSystemID()-1];

	BATCH->set_state(RPR_BATCH,true);
	DisableTextures();
	CAMERA->SetModelView();
	BATCH->set_render_state(D3DRS_ZENABLE,FALSE);
	BATCH->set_render_state(D3DRS_ZWRITEENABLE,FALSE);
	BATCH->set_render_state(D3DRS_ALPHABLENDENABLE,TRUE);
	BATCH->set_render_state(D3DRS_SRCBLEND,D3DBLEND_ONE);
	BATCH->set_render_state(D3DRS_DESTBLEND,D3DBLEND_ONE);
	PB.Begin(PB_LINES);
	PB.Color3ub(GetRValue(color),GetGValue(color),GetBValue(color));
	Vector oldVect = jumpGateCircle[0]+transform.translation;
	for(U32 i = 1 ; i <= NUM_LINE_SEGS; ++i)
	{
		PB.Vertex3f(oldVect.x,oldVect.y,oldVect.z);
		oldVect = jumpGateCircle[i%NUM_LINE_SEGS]+transform.translation;
		PB.Vertex3f(oldVect.x,oldVect.y,oldVect.z);
	}
	PB.End();

}

void Jumpgate::Render (void)
{
	if (bVisible)
	{
//		if(bHighlight)
//			renderCircle();
		//ORIENTOBJECT
	
		bool bRevealed = (FOGOFWAR->CheckVisiblePosition(transform.translation) != 0);

		Jumper *pos = jumper;
		Jumper *first = NULL,*nearest = NULL;
		
		//this check should neutralize the whole jumpgate anim
		if (bRevealed)
		{
			while (pos)
			{
				
				{
					//maybe could do this smarter and faster
					if (!nearest || fabs(pos->tick-jumpTimer) < fabs(nearest->tick-jumpTimer))
					{
						nearest = pos;
					}
					if ((!first || (pos->tick<first->tick)) && (pos->tick > jumpTimer))
					{
						first = pos;
					}
					if (pos->tick <= jumpTimer)
					{
						last_jumper = pos;
						//	baseAim = last_jumper->aim;
						baseTick = last_jumper->tick;
					}
				}
				
				if (pos)
					pos = pos->next;
			}
		}
		TRANSFORM trans;
		Vector i(1,0,0);
		
		
		Vector k(0,0,1);
		Vector k0,k1;
	/*	if (next_jumper)
		{
			SINGLE aim = (baseAim*fabs(jumpTimer-next_jumper->tick)+next_jumper->aim*fabs(jumpTimer-baseTick))/(next_jumper->tick-baseTick);
			k.set(-sin(aim),-cos(aim),0);
			ick = ((turnAngle)/(2*PI))*(jumpTimer-baseTick)/(SINGLE)(next_jumper->tick-baseTick);
		}
		else */
		if (bRevealed)
		{
			switch (jumpStage)
			{
			case JS_OPEN:
				k.set(-sin(baseAim),-cos(baseAim),0);
				baseTick = jumpTimer;
				break;
			case JS_OPENING:
				{
					k0.set(-sin(baseAim),-cos(baseAim),0);
					k1.set(0,0,1);
					SINGLE morph=timer/OPENING_TIME;
					k = morph*k1+(1-morph)*k0;
				}
				break;
			case JS_IDLE:
				k.set(0,0,1);
				break;
			case JS_CLOSING:
				{
					k0.set(-sin(baseAim),-cos(baseAim),0);
					k1.set(0,0,1);
					SINGLE morph=timer/OPENING_TIME;
					k = (1-morph)*k1+(morph)*k0;
				}
				break;
			}
		}
		else
			k.set(0,0,1);
		
		i.set(cos(baseAim),-sin(baseAim),0);
		
		trans.set_i(i);
		trans.set_k(k);
		Vector j = cross_product(k,i);
		trans.set_j(j);
		trans.translation = transform.translation;
		trans.rotate_about_k(theta);
		transform = trans;

		//RENDER BILLBOARD MESHES
	
		int meshesRendered=0;
	
		BLENDS stage;
		U32 nextStartMesh=0;
		U32 currentTex;

		static bool bRendered[MAX_BB_MESHES];
		memset(bRendered,0,sizeof(bool)*MAX_BB_MESHES);

		while (meshesRendered < numMeshes)
		{
			v_pos = 0;

			while (bRendered[nextStartMesh] || bb[nextStartMesh].numVerts==0)
			{
				nextStartMesh++;
				CQASSERT(nextStartMesh < MAX_BB_MESHES);
			}

			currentTex = bb[nextStartMesh].bb_txm;
			stage = bb[nextStartMesh].blendMode;
			for (int c=nextStartMesh;c<MAX_BB_MESHES;c++)
			{
				if (bb[c].numVerts && bb[c].blendMode == stage && bb[c].bb_txm == currentTex)
				{
					StashVertices(c);
					meshesRendered++;
					bRendered[c] = true;
				}
			}

			BATCH->set_state(RPR_BATCH,FALSE);
			SetupDiffuseBlend(currentTex,TRUE);
			if (v_pos)
				Billboard3db(stage);
		}

		//render open stage effects
		if (0)//jumpStage == JS_OPEN)
		{
			Vector t = transform.translation-k*100;
			
//			BATCH->set_texture_stage_texture( 0,windowTexID);
			SetupDiffuseBlend(windowTexID,TRUE);
			BATCH->set_render_state(D3DRS_ALPHABLENDENABLE,TRUE);
			BATCH->set_render_state(D3DRS_CULLMODE,D3DCULL_NONE);
			BATCH->set_render_state(D3DRS_ZENABLE,FALSE);
			BATCH->set_render_state(D3DRS_SRCBLEND,D3DBLEND_ONE);
			BATCH->set_render_state(D3DRS_DESTBLEND,D3DBLEND_ONE);
#define NUM2 600
			
			SINGLE morph = 0;
			Vector v0,v1,v2,v3;
			v0 = t-i*NUM2*(1-2.5*morph)-j*NUM2*(1-2.5*morph);
			v1 = t+i*NUM2*(1-2.5*morph)-j*NUM2*(1-2.5*morph);
			v2 = t+i*NUM2*(1-2.5*morph)+j*NUM2*(1-2.5*morph);
			v3 = t-i*NUM2*(1-2.5*morph)+j*NUM2*(1-2.5*morph);
			
			PB.Color3ub(100,130,150);
			PB.Begin(PB_QUADS);
			PB.TexCoord2f(0,0);  PB.Vertex3f(v0.x,v0.y,v0.z);
			PB.TexCoord2f(1,0);  PB.Vertex3f(v1.x,v1.y,v1.z);
			PB.TexCoord2f(1,1);  PB.Vertex3f(v2.x,v2.y,v2.z);
			PB.TexCoord2f(0,1);  PB.Vertex3f(v3.x,v3.y,v3.z);

			PB.Color3ub(50,80,100);
			morph = -0.13;
			t = transform.translation+k*100;
			v0 = t-i*NUM2*(1-2.5*morph)-j*NUM2*(1-2.5*morph);
			v1 = t+i*NUM2*(1-2.5*morph)-j*NUM2*(1-2.5*morph);
			v2 = t+i*NUM2*(1-2.5*morph)+j*NUM2*(1-2.5*morph);
			v3 = t-i*NUM2*(1-2.5*morph)+j*NUM2*(1-2.5*morph);
			PB.TexCoord2f(0,0);  PB.Vertex3f(v0.x,v0.y,v0.z);
			PB.TexCoord2f(1,0);  PB.Vertex3f(v1.x,v1.y,v1.z);
			PB.TexCoord2f(1,1);  PB.Vertex3f(v2.x,v2.y,v2.z);
			PB.TexCoord2f(0,1);  PB.Vertex3f(v3.x,v3.y,v3.z);
			PB.End();
		}

		//render jump flash
		if (nearest )
		{
			SINGLE timeToJump = fabs(nearest->tick-jumpTimer);
			//render warp flash
			if (timeToJump < 0.36)//morph < 0.4)
			{
				SINGLE morph = timeToJump;
				CAMERA->SetModelView();
				
				Vector cpos (CAMERA->GetPosition());
				Vector look (transform.translation - cpos);
				
				Vector i (look.y, -look.x, 0);
				
				if (fabs (i.x) < 1e-4 && fabs (i.y) < 1e-4)
				{
					i.x = 1.0f;
				}
				
				i.normalize ();

				vector_rotate_about_k(&i,(rand()%100)*0.01*6);//theta*6);

				Vector k (-look);
				k.normalize ();
				Vector j (cross_product (k, i));
				
			//	BATCH->set_texture_stage_texture( 0,flashTexID);
				SetupDiffuseBlend(flashTexID,TRUE);
				BATCH->set_render_state(D3DRS_ALPHABLENDENABLE,TRUE);
				BATCH->set_render_state(D3DRS_CULLMODE,D3DCULL_NONE);
				BATCH->set_render_state(D3DRS_ZENABLE,FALSE);
				BATCH->set_render_state(D3DRS_ZWRITEENABLE,FALSE);
				BATCH->set_render_state(D3DRS_SRCBLEND,D3DBLEND_ONE);
				BATCH->set_render_state(D3DRS_DESTBLEND,D3DBLEND_ONE);
				PB.Color3ub(145*(1-2.5*morph),145*(1-2.5*morph),145*(1-2.5*morph));
				PB.Begin(PB_QUADS);
				Vector t = transform.translation-transform.get_k()*750;
				
				Vector test (transform.translation - cpos);
				SINGLE flashSize = test.magnitude();
				flashSize *= 0.35;

				Vector v0,v1,v2,v3;
				
				if (1)//rand()%4==0)
				{
					v0 = t-i*flashSize*(1-2.5*morph)-j*flashSize*(1-2.5*morph);
					v1 = t+i*flashSize*(1-2.5*morph)-j*flashSize*(1-2.5*morph);
					v2 = t+i*flashSize*(1-2.5*morph)+j*flashSize*(1-2.5*morph);
					v3 = t-i*flashSize*(1-2.5*morph)+j*flashSize*(1-2.5*morph);
					
					PB.TexCoord2f(0,0);  PB.Vertex3f(v0.x,v0.y,v0.z);
					PB.TexCoord2f(1,0);  PB.Vertex3f(v1.x,v1.y,v1.z);
					PB.TexCoord2f(1,1);  PB.Vertex3f(v2.x,v2.y,v2.z);
					PB.TexCoord2f(0,1);  PB.Vertex3f(v3.x,v3.y,v3.z);
				}
				
				vector_rotate_about_k(&i,-theta*12);

				j = cross_product (k, i);
				
				if (1)//rand()%4==0)
				{
					v0 = t-i*flashSize*(1-2.5*morph)-j*flashSize*(1-2.5*morph);
					v1 = t+i*flashSize*(1-2.5*morph)-j*flashSize*(1-2.5*morph);
					v2 = t+i*flashSize*(1-2.5*morph)+j*flashSize*(1-2.5*morph);
					v3 = t-i*flashSize*(1-2.5*morph)+j*flashSize*(1-2.5*morph);
					
					
					PB.TexCoord2f(0,0);  PB.Vertex3f(v0.x,v0.y,v0.z);
					PB.TexCoord2f(1,0);  PB.Vertex3f(v1.x,v1.y,v1.z);
					PB.TexCoord2f(1,1);  PB.Vertex3f(v2.x,v2.y,v2.z);
					PB.TexCoord2f(0,1);  PB.Vertex3f(v3.x,v3.y,v3.z);
				}

				PB.End();
			}
		}

		if(BUILDARCHEID && bRenderBuild)
		{
			TRANSFORM platTrans;
			platTrans.rotate_about_i(PI/2);
			platTrans.translation = GetPosition();
			UNBORNMANAGER->RenderMeshAt(platTrans,BUILDARCHEID,MGlobals::GetThisPlayer(),0,255,0,128);
		}

		return;
	}
	
}
//---------------------------------------------------------------------------
//
void Jumpgate::MapRender (bool bPing)
{
	if(((GetVisibilityFlags()&MGlobals::GetAllyMask(MGlobals::GetThisPlayer()))!=0) || DEFAULTS->GetDefaults()->bEditorMode || DEFAULTS->GetDefaults()->bSpectatorModeOn)
	{
		if(mapTex != -1)
			SYSMAP->DrawIcon(transform.translation,GRIDSIZE,mapTex);
		else
			SYSMAP->DrawCircle(transform.translation,GRIDSIZE,RGB(0,0,255));
		
	}
}
//---------------------------------------------------------------------------
//
void Jumpgate::init (JumpgateArchetype *arch)
{
	data = arch->pData;

	createLights();

	hAmbientSound = SFXMANAGER->Open(data->ambience);
	hEnterSound[0] = SFXMANAGER->Open(data->enter1);
	hEnterSound[1] = SFXMANAGER->Open(data->enter2);
	hEnterSound[2] = SFXMANAGER->Open(data->enter1);
	hEnterSound[3] = SFXMANAGER->Open(data->enter2);
	hArriveSound[0] = SFXMANAGER->Open(data->arrive1);
	hArriveSound[1] = SFXMANAGER->Open(data->arrive2);
	hArriveSound[2] = SFXMANAGER->Open(data->arrive1);
	hArriveSound[3] = SFXMANAGER->Open(data->arrive2);
	
	mapTex = arch->mapTex;
	windowTexID = arch->windowTexID;

	int i;
	for (i=0;i<MAX_BB_MESHES;i++)
	{
		bb[i].bb_txm = arch->bb_txm[i];
		bb[i].animArch = arch->bb_anim_arch[i];
		if (bb[i].animArch)
			bb[i].bb_txm = bb[i].animArch->frames[0].texture;
	}

	if (arch->dormantArch)
	{
		anim.Init(arch->dormantArch);
		anim.rate = 25;
		anim.loop = FALSE;
	//	anim.SetPosition(Vector(x,y,0));
		anim.SetWidth(1000);
	//	anim.Randomize();
	}

	sort_size = 0;
	//INIT BILLBOARDS
	for (i=0;i<MAX_BB_MESHES;i++)
	{
		if (arch->bb_mesh_archID[i] != INVALID_ARCHETYPE_INDEX)
		//	bb[i].SetMesh(ENGINE->create_instance(arch->bb_mesh_archID[i]),&arch->pData->billboardMesh[i]);
			bb[i].SetMesh(arch->bb_mesh_archID[i],&arch->pData->billboardMesh[i]);
		if (arch->mm_arch[i])
		{
			bb[i].SetMesh(arch->mm_arch[i],&arch->pData->billboardMesh[i]);
			bb[i].morphMesh->RunAnim(0,TRUE);
		}

		bb[i].blendMode = arch->pData->billboardMesh[i].blendMode;

		sort_size += bb[i].numVerts;

		if (bb[i].numVerts)
			numMeshes++;
	}

	s_sort = new SortVertex[sort_size];
	v_sort = new U32[sort_size];

	arche2 = arch->arche2;

	bJumpAllowed = true;
	bRenderBuild = false;
	bLocked = false;
}
//---------------------------------------------------------------------------
//
SINGLE Jumpgate::TestHighlight (const RECT & rect)	// set bHighlight if possible for any part of object to appear within rect
{
	SINGLE closeness = 999999.0f;

	bHighlight = 0;
	if (bVisible)
	{
		if ((bVisible = (instanceIndex != INVALID_INSTANCE_INDEX)) != 0)
		{
			float depth=0, center_x=0, center_y=0, radius=0;

			if ((bVisible = get_pbs(center_x, center_y, radius, depth)) != 0)
			{
				RECT _rect;

				_rect.left  = center_x - radius;
				_rect.right	= center_x + radius;
				_rect.top = center_y - radius;
				_rect.bottom = center_y + radius;

				RECT screenRect = { 0, 0, SCREENRESX, SCREENRESY };

				if ((bVisible = RectIntersects(_rect, screenRect)) != 0)
				{
					if (RectIntersects(rect, _rect))
					{
						ViewPoint points[64];
						int numVerts = sizeof(points) / sizeof(ViewPoint);
						if (REND->get_instance_projected_bounding_polygon(instanceIndex, MAINCAM, LODPERCENT, numVerts, points, numVerts, depth))
						{
							bHighlight = RectIntersects(rect, points, numVerts);

							if (rect.left == rect.right && rect.top == rect.bottom)
								closeness = fabs(rect.left - center_x) * fabs(rect.top - center_y);
							else
								closeness = 0.0f;
						}
					}
				}
			}
		}
	}
	bRenderBuild = false;
	if(bHighlight && BUILDARCHEID)
	{
		if (!((visMarks|marks) & (0x01 << (MGlobals::GetThisPlayer()-1)) ))
		{
			BASE_PLATFORM_DATA * data = (BASE_PLATFORM_DATA *)(ARCHLIST->GetArchetypeData(BUILDARCHEID));
			if(data->type == PC_JUMPPLAT)
			{
				bRenderBuild = true;
			}
		}
	}

	return closeness;
}
//---------------------------------------------------------------------------
//
void Jumpgate::createLights()
{
	Vector vec;

	lights[0].color.r = lights[0].color.g = lights[0].color.b = 0;
	lights[0].range = 4000;
	lights[0].set_On(true);
	lights[0].setSystem(systemID);
}
//---------------------------------------------------------------------------
//
BOOL32 Jumpgate::Save (struct IFileSystem * outFile)
{
	BOOL32 result = 0;
	U32 dwWritten;
	COMPTR<IFileSystem> file;
	DAFILEDESC fdesc = "JUMPGATE_SAVELOAD";
	JUMPGATE_SAVELOAD d;

	fdesc.dwDesiredAccess = GENERIC_READ|GENERIC_WRITE;
	fdesc.dwShareMode = 0;  // no sharing
	fdesc.dwCreationDistribution = CREATE_ALWAYS;
	if (outFile->CreateInstance(&fdesc,file) != GR_OK)
		goto Done;

	memset(&d, 0, sizeof(d));
	d.id = info.gate_id;
	d.exit_gate_id = info.exit_gate_id;
	d.time_until_last_jump = time_until_last_jump;
	d.exploredFlags = GetVisibilityFlags();
	d.bJumpAllowed = bJumpAllowed;
	d.ownerID = ownerID;
	d.bLocked = bLocked;
	d.marks = marks;
	d.visMarks = visMarks;
	d.shadowVisabilityFlags = shadowVisibilityFlags;
	d.bInvisible = bInvisible;

	FRAME_save(d);
		
	if (file->WriteFile(0,&d,sizeof(d),&dwWritten,0) == 0)
		goto Done;

	result = 1;

Done:	
	return result;
}
//---------------------------------------------------------------------------
//
BOOL32 Jumpgate::Load (struct IFileSystem * inFile)
{
	BOOL32 result = 0;
	U32 dwRead;
	COMPTR<IFileSystem> file;
	DAFILEDESC fdesc = "JUMPGATE_SAVELOAD";
	U8 buffer[1024];
	JUMPGATE_SAVELOAD d;

	if (inFile->CreateInstance(&fdesc,file) != GR_OK)
		goto Done;

	file->ReadFile(0, buffer, sizeof(buffer), &dwRead, 0);
	MISSION->CorrelateSymbol(fdesc.lpFileName, buffer, &d);

	CQASSERT(d.mission.dwMissionID != 0);

	FRAME_load(d);
		
	info.gate_id = d.id;
	info.exit_gate_id = d.exit_gate_id;
	time_until_last_jump = d.time_until_last_jump;
	ownerID = d.ownerID;
	bLocked = d.bLocked;
	visMarks = d.visMarks;
	shadowVisibilityFlags = d.shadowVisabilityFlags;
	bInvisible = d.bInvisible;

	SetVisibleToAllies(d.exploredFlags);
	UpdateVisibilityFlags();

	SECTOR->RegisterJumpgate(this,d.id);
	
	SetPosition(transform.translation, systemID);
	bJumpAllowed = d.bJumpAllowed;
	marks = d.marks;

	result = 1;

Done:	
	
	return result;
}
//---------------------------------------------------------------------------
//
void Jumpgate::ResolveAssociations()
{
	FRAME_resolve();
	Vector t = transform.translation;
	lights[0].set_position(Vector(t.x,t.y,t.z+2000));
}
//---------------------------------------------------------------------------
//
void Jumpgate::QuickSave (struct IFileSystem * file)
{
	DAFILEDESC fdesc = partName;
	HANDLE hFile;

	file->CreateDirectory("MT_QJGATELOAD");
	if (file->SetCurrentDirectory("MT_QJGATELOAD") == 0)
		CQERROR0("QuickSave failed on directory 'MT_QJGATELOAD'");

	fdesc.dwDesiredAccess = GENERIC_READ|GENERIC_WRITE;
	fdesc.dwShareMode = 0;  // no sharing
	fdesc.dwCreationDistribution = CREATE_NEW;		// fail if file already exists

	if ((hFile = file->OpenChild(&fdesc)) == INVALID_HANDLE_VALUE)
	{
		CQERROR1("QuickSave failed on part '%s'", fdesc.lpFileName);
	}
	else
	{
		MT_QJGATELOAD qload;
		DWORD dwWritten;

		qload.pos.init(GetGridPosition(), systemID);
		qload.gate_id = info.gate_id;
		qload.exit_gate_id = info.exit_gate_id;

		file->WriteFile(hFile, &qload, sizeof(qload), &dwWritten, 0);
		file->CloseHandle(hFile);
	}
}
//---------------------------------------------------------------------------
//
void Jumpgate::QuickLoad (const char *szSaveLoadType, const char *szInstanceName, void * buffer, U32 bufferSize)
{
	MT_QJGATELOAD qload;
	GENRESULT gr = MISSION->CorrelateSymbol(szSaveLoadType, buffer, &qload);
	CQASSERT(gr == GR_OK);

	info.gate_id = qload.gate_id;
	info.exit_gate_id = qload.exit_gate_id;
	time_until_last_jump = 0;
	MGlobals::InitMissionData(this, MGlobals::CreateNewPartID(0));

	SetPosition(qload.pos, qload.pos.systemID);
	SECTOR->RegisterJumpgate(this,qload.gate_id);
	partName = szInstanceName;

	OBJLIST->AddPartID(this, dwMissionID);

	bJumpAllowed = 1;
}
//---------------------------------------------------------------------------
//
void Jumpgate::QuickResolveAssociations (void)
{
	// nothing to do here!?
}
//---------------------------------------------------------------------------
//
void Jumpgate::View (void)
{
	JUMPGATE_VIEW view;

	view.mission = this;
	view.rtData = NULL;

	if (DEFAULTS->GetUserData("JUMPGATE_VIEW", view.mission->partName, &view, sizeof(view)))
	{
		bJumpAllowed = view.bJumpAllowed;
	}
}
//--------------------------------------------------------------------------
//
void Jumpgate::UpdateVisibilityFlags (void)
{
	if(bInvisible)
	{
		SetVisibilityFlags(0);
		return;
	}
	U8 newShadowVisibilityFlags;
	newShadowVisibilityFlags = GetVisibilityFlags() & (~GetPendingVisibilityFlags());
	if(newShadowVisibilityFlags != shadowVisibilityFlags)
	{
		U32 oldFlags = (~newShadowVisibilityFlags) & shadowVisibilityFlags;
		if(oldFlags)
		{
			//update marks
			for(U32 i = 0; i < MAX_PLAYERS; ++i)
			{
				if((oldFlags >> i) & 0x01)
				{
					visMarks &= ~(0x01 << i);
					visMarks |= (((ownerID&PLAYERID_MASK) != 0) << i);
				}
			}
		}
		shadowVisibilityFlags = newShadowVisibilityFlags;
	}
	IBaseObject::UpdateVisibilityFlags2();//calling 2 on purpose;
}
//---------------------------------------------------------------------------
//
void Jumpgate::UpdateVisibilityFlags2 (void)
{
	if(bInvisible)
	{
		SetVisibilityFlags(0);
		return;
	}
	U8 newShadowVisibilityFlags;
	newShadowVisibilityFlags = GetVisibilityFlags() & (~GetPendingVisibilityFlags());
	if(newShadowVisibilityFlags != shadowVisibilityFlags)
	{
		U32 oldFlags = (~newShadowVisibilityFlags) & shadowVisibilityFlags;
		if(oldFlags)
		{
			//update marks
			for(U32 i = 0; i < MAX_PLAYERS; ++i)
			{
				if((oldFlags >> i) & 0x01)
				{
					visMarks &= ~(0x01 << i);
					visMarks |= (((ownerID&PLAYERID_MASK) != 0) << i);
				}
			}
		}
		shadowVisibilityFlags = newShadowVisibilityFlags;
	}
	IBaseObject::UpdateVisibilityFlags2();//calling 2 on purpose;
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//
struct IBaseObject * CreateJumpgate (const Transform &trans,JumpgateArchetype *arch)
{
	Jumpgate * obj = new ObjectImpl<Jumpgate>;

	obj->FRAME_init(*arch);

	obj->pArchetype = 0;
	obj->objClass = OC_JUMPGATE;
	obj->systemID = SECTOR->GetCurrentSystem();
	obj->transform = trans;
	obj->init(arch);

	return obj;
}

enum CURSOR_MODE
{
	NOT_OWNED=0,
	INSERT,
	BAN
};

struct JumpgateNode
{
	IBaseObject *Jumpgate;
	JumpgateNode *next;
};



struct DACOM_NO_VTABLE JumpgateManager : public IObjectFactory
{
	//
	// incoming interface map
	//
  
	BEGIN_DACOM_MAP_INBOUND(JumpgateManager)
	DACOM_INTERFACE_ENTRY(IObjectFactory)
	END_DACOM_MAP()

	// structure
//	struct JumpgateNode *JumpgateList;
	U32 factoryHandle;
	TRANSFORM editorOrientation;	// initial orientation of objects

	//child object info
	JumpgateArchetype *pArchetype;

	//JumpgateManager methods

	JumpgateManager (void) 
	{
		//editorOrientation.rotate_about_k(45*MUL_DEG_TO_RAD);
		editorOrientation.rotate_about_j(PI/2);
	}

	~JumpgateManager();
	
    void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}

	IDAComponent * GetBase (void)
	{
		return static_cast<IObjectFactory *>(this);
	}

	BOOL32 Update();

	void Init();

	void ConeToSphere(ARCHETYPE_INDEX archeID);

	void SphereToCylinder(ARCHETYPE_INDEX archeID);

	void DarkenRing(Mesh *mesh);

//	void setCursorMode (CURSOR_MODE newMode);

	//IObjectFactory
	DEFMETHOD_(HANDLE,CreateArchetype) (const char *szArchname, OBJCLASS objClass, void *data);

	DEFMETHOD_(BOOL32,DestroyArchetype) (HANDLE hArchetype);

	DEFMETHOD_(IBaseObject *,CreateInstance) (HANDLE hArchetype);

	DEFMETHOD_(void,EditorCreateInstance) (HANDLE hArchetype, const MISSION_INFO & info);
};


//--------------------------------------------------------------------------
// JumpgateManager methods

JumpgateManager::~JumpgateManager()
{
	COMPTR<IDAConnectionPoint> connection;
	if (OBJLIST)
	{
		if (OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
			connection->Unadvise(factoryHandle);
	}

/*	if (arche2 != INVALID_ARCHETYPE_INDEX)
	{
		ENGINE->release_archetype(arche2);
	}*/
}

BOOL32 JumpgateManager::Update()
{
	return 1;
}

void JumpgateManager::Init()
{
	COMPTR<IDAConnectionPoint> connection;

	if (OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
	{
		connection->Advise(GetBase(), &factoryHandle);
	}
}
//--------------------------------------------------------------------------//
//
void JumpgateManager::ConeToSphere(ARCHETYPE_INDEX archeID)
{
	CQASSERT(0);
	Mesh *mesh = REND->get_archetype_mesh(archeID);
	SINGLE tanT;
	Vector *v = mesh->object_vertex_list;
	
	SINGLE neg;
	SINGLE cen,size,scale,cone_scale,shift;

	//TEMP
	Vector *cone;
	cone = new Vector[mesh->object_vertex_cnt];
	sphere = new Vector[mesh->object_vertex_cnt];
	scale = 0.4;
	cone_scale = 1.8;
	shift = 1470;

	memcpy(cone,v,sizeof(Vector)*mesh->object_vertex_cnt);
	S32 i;
	for (i=0;i < mesh->object_vertex_cnt;i++)
	{
		if (v[i].y > maxY)
			maxY = v[i].y;
		if (v[i].y < minY)
			minY = v[i].y;
	}
	cen = (maxY+minY)*0.5;
	size = (maxY-minY)*0.5+1;
	for (i=0;i < mesh->object_vertex_cnt;i++)
	{
		tanT = v[i].x/v[i].z;
		neg = 1;
		if (v[i].z < 0)
			neg = -1;
	
		v[i].z = sqrt((size*size-(v[i].y-cen)*(v[i].y-cen))/(tanT*tanT+1))*neg*scale;
		neg = 1;
		if (v[i].x < 0)
			neg = -1;
		v[i].x = fabs(v[i].z*tanT)*neg;
		v[i].y -= cen;
		v[i].y *= scale;
		cone[i] *= cone_scale;
		cone[i].y -= shift;
	}
	memcpy(sphere,v,sizeof(Vector)*mesh->object_vertex_cnt);

	tex = new TexCoord[mesh->texture_vertex_cnt];
	memcpy(tex,mesh->texture_vertex_list,sizeof(TexCoord)*mesh->texture_vertex_cnt);
}
//--------------------------------------------------------------------------//
//
void JumpgateManager::SphereToCylinder(ARCHETYPE_INDEX archeID)
{
	Mesh *mesh = REND->get_archetype_mesh(archeID);
	Vector *v = mesh->object_vertex_list;
	
	SINGLE neg;
	SINGLE cen,size,scale,shift=0,cyl_rad;

	sphere = new Vector[mesh->object_vertex_cnt];
	cylinder = new Vector[mesh->object_vertex_cnt];
	scale = 1.2;
//	shift = 1470;
	cyl_rad = 800;

	memcpy(sphere,v,sizeof(Vector)*mesh->object_vertex_cnt);
	S32 i;
	for (i=0;i < mesh->object_vertex_cnt;i++)
	{
		if (v[i].y > maxY)
			maxY = v[i].y;
		if (v[i].y < minY)
			minY = v[i].y;
	}
	cen = (maxY+minY)*0.5;
	size = (maxY-minY)*0.5+1;
	for (i=0;i < mesh->object_vertex_cnt;i++)
	{
		SINGLE ratio = cyl_rad/sqrt(v[i].x*v[i].x+v[i].z*v[i].z);
		neg = 1;
		if (v[i].z < 0)
			neg = -1;

		v[i].x *= ratio;
		v[i].z *= ratio;
	
		v[i].y -= cen;
		v[i].y *= scale;
		sphere[i].y -= shift;
	}
	memcpy(cylinder,v,sizeof(Vector)*mesh->object_vertex_cnt);

	tex = new TexCoord[mesh->texture_vertex_cnt];
	memcpy(tex,mesh->texture_vertex_list,sizeof(TexCoord)*mesh->texture_vertex_cnt);
}
//--------------------------------------------------------------------------//
//
void JumpgateManager::DarkenRing(Mesh *mesh)
{
	Vector *v = mesh->object_vertex_list;
	SINGLE size;
	SINGLE maxX=0,minX=0;

	S32 i;
	for (i=0;i < mesh->object_vertex_cnt;i++)
	{
		if (v[i].x > maxX)
			maxX = v[i].x;
		if (v[i].x < minX)
			minX = v[i].x;
	}
//	cen = (maxY+minY)*0.5;
	size = (maxX-minX)*0.5;

	for (i=0;i < mesh->object_vertex_cnt;i++)
	{
		if (v[i].magnitude() <250)// size-60)
		{
			ringColors[i].r = ringColors[i].g = ringColors[i].b = 0;
		}
		else
			ringColors[i].r = ringColors[i].g = ringColors[i].b = 200;

	//	ringColors[i].r = ringColors[i].g = ringColors[i].b = ((size-v[i].magnitude())/size)*255;
	}
}
//--------------------------------------------------------------------------//
//
HANDLE JumpgateManager::CreateArchetype(const char *szArchname, OBJCLASS objClass, void *data)
{
	if (objClass == OC_JUMPGATE)
	{
		for(int j = 0; j < NUM_LINE_SEGS; ++j)
		{
			jumpGateCircle[j] = Vector(cos((2*PI*j)/NUM_LINE_SEGS)*1000,
				sin((2*PI*j)/NUM_LINE_SEGS)*1000,0);
		}

		JumpgateArchetype *newguy = new JumpgateArchetype;
		newguy->name = szArchname;
		newguy->pData = (BT_JUMPGATE_DATA *)data;
		newguy->pArchetype = ARCHLIST->GetArchetype(szArchname);

		COMPTR<IFileSystem> file;
		DAFILEDESC fdesc;

		// LOAD BILLBOARD MESHES
		for (int i=0;i<MAX_BB_MESHES;i++)
		{
			if (newguy->pData->billboardMesh[i].mesh_name[0] && newguy->pData->billboardMesh[i].tex_name[0])
			{
				//Get mesh or animating mesh
				if (strstr(newguy->pData->billboardMesh[i].mesh_name,".3db"))
				{
					fdesc = newguy->pData->billboardMesh[i].mesh_name;
					if (OBJECTDIR->CreateInstance(&fdesc, file) == GR_OK)
						newguy->bb_mesh_archID[i] = ENGINE->create_archetype(fdesc.lpFileName, file);
					else
						CQFILENOTFOUND(fdesc.lpFileName);
				}
				else if (strstr(newguy->pData->billboardMesh[i].mesh_name,".ini"))
				{
					newguy->mm_arch[i] = CreateMorphMeshArchetype(newguy->pData->billboardMesh[i].mesh_name);
				//	newguy->morphMesh[i]->ReadINI(newguy->pData->billboardMesh[i].mesh_name);
				//	newguy->morphMesh[i]->RunAnim(0,TRUE);
				}

				//Get texture or animating texture
				if (strstr(newguy->pData->billboardMesh[i].tex_name,".tga"))
					newguy->bb_txm[i] = TMANAGER->CreateTextureFromFile(newguy->pData->billboardMesh[i].tex_name,TEXTURESDIR, DA::TGA,PF_4CC_DAA8);
				else if (strstr(newguy->pData->billboardMesh[i].tex_name,".anm"))
				{
					fdesc = newguy->pData->billboardMesh[i].tex_name;
					if (OBJECTDIR->CreateInstance(&fdesc, file) == GR_OK)
						newguy->bb_anim_arch[i] = ANIM2D->create_archetype(file);
					else
						CQFILENOTFOUND(fdesc.lpFileName);
				}
				else
					CQERROR1("Don't know how to load %s",newguy->pData->billboardMesh[i].tex_name);
			}
		}
		// LOAD REMAINING DISORGANIZED CRAP

		fdesc = "Jring.3db";
		if (OBJECTDIR->CreateInstance(&fdesc, file) == GR_OK)
			TEXLIB->load_library(file, 0);

		if ((newguy->arche2 = ENGINE->create_archetype("TheRing", file)) != INVALID_ARCHETYPE_INDEX)
		{
			Mesh *mesh = REND->get_archetype_mesh(newguy->arche2);
			ring = new Vector[mesh->object_vertex_cnt];
			ringColors = new ColorRGB[mesh->object_vertex_cnt];
			DarkenRing(mesh);
			memcpy(ring,mesh->object_vertex_list,sizeof(Vector)*mesh->object_vertex_cnt);
		}

		//fdesc = newguy->pData->file_3db;
		fdesc = "Jgate_Sphere.3db";
		if (OBJECTDIR->CreateInstance(&fdesc, file) == GR_OK)
			TEXLIB->load_library(file, 0);
		if ((newguy->archIndex = ENGINE->create_archetype(fdesc.lpFileName, file)) != INVALID_ARCHETYPE_INDEX)
		{
		//	archeIndex = newguy->archeIndex;
		//	ConeToSphere(archeIndex);
			SphereToCylinder(newguy->archIndex);
		}
		else
		{
			delete newguy;
			newguy = 0;
		}
		
		/*fdesc = "jgateflash.tga";
		
		if (TEXTURESDIR->CreateInstance(&fdesc, file) != GR_OK)
		{
			CQERROR1("Failed to open file %s", fdesc.lpFileName);
			return;
		}*/

		flashTexID = TMANAGER->CreateTextureFromFile("dragonball_additive_flash.tga", TEXTURESDIR,DA::TGA,PF_4CC_DAA4);
		newguy->windowTexID = TMANAGER->CreateTextureFromFile("WarpWindow.tga", TEXTURESDIR,DA::TGA,PF_4CC_DAA4);

		newguy->mapTex = SYSMAP->RegisterIcon("SysMap\\Jumpgate.bmp");
		fdesc = "dormant.anm";
		if (OBJECTDIR->CreateInstance(&fdesc, file) == GR_OK)
		{
			newguy->dormantArch = ANIM2D->create_archetype(file);
		}

		// preload sounds

		SFXMANAGER->Preload(newguy->pData->ambience);
		SFXMANAGER->Preload(newguy->pData->enter1);
		SFXMANAGER->Preload(newguy->pData->enter2);
		SFXMANAGER->Preload(newguy->pData->enter1);
		SFXMANAGER->Preload(newguy->pData->enter2);
		SFXMANAGER->Preload(newguy->pData->arrive1);
		SFXMANAGER->Preload(newguy->pData->arrive2);
		SFXMANAGER->Preload(newguy->pData->arrive1);
		SFXMANAGER->Preload(newguy->pData->arrive2);

		return newguy;
	}
	else
		return 0;
}

BOOL32 JumpgateManager::DestroyArchetype(HANDLE hArchetype)
{
	if (sphere)
	{
		delete [] sphere;
		sphere = 0;
	}
	if (ring)
	{
		delete [] ring;
		ring = 0;
	}
	if (ringColors)
	{
		delete [] ringColors;
		ringColors = 0;
	}
	if (cylinder)
	{
		delete [] cylinder;
		cylinder = 0;
	}
	if (tex)
	{
		delete [] tex;
		tex = 0;
	}

	TMANAGER->ReleaseTextureRef(flashTexID);

	JumpgateArchetype *deadguy = (JumpgateArchetype *)hArchetype;
	EditorStopObjectInsertion(deadguy->pArchetype);
	delete deadguy;

	return 1;
}

IBaseObject * JumpgateManager::CreateInstance(HANDLE hArchetype)
{
	JumpgateArchetype *pJumpgate = (JumpgateArchetype *)hArchetype;
	BT_JUMPGATE_DATA *objData = ((JumpgateArchetype *)hArchetype)->pData;
	TRANSFORM & trans = editorOrientation;
	
	if (objData->objClass == OC_JUMPGATE)
	{
	//	if ((index = ENGINE->create_instance(pJumpgate->archeIndex)) != INVALID_INSTANCE_INDEX)
	//	{
		/*	if (!JumpgateList)
			{
				JumpgateList = new JumpgateNode;
				JumpgateList->next = NULL;
			}
			else
			{
				JumpgateNode *node;
				node = new JumpgateNode;
				node->next = JumpgateList;
				JumpgateList = node;
			}*/
			
			
			
		/*	IBaseObject * obj = new ObjectImpl<Jumpgate>;
			obj->instanceIndex = index;
			obj->pArchetype = 0;
			obj->objClass = OC_JUMPGATE;
			obj->systemID = SECTOR->GetCurrentSystem();
			obj->dwOwnerID = -1;
			
			JumpgateList->Jumpgate = obj;*/

			IBaseObject *obj = CreateJumpgate(trans,pJumpgate);
			

			return obj;
//		}
	}

	
	
	return 0;
}

void JumpgateManager::EditorCreateInstance(HANDLE hArchetype,const MISSION_INFO & _info)
{
	JumpgateArchetype * objtype = (JumpgateArchetype *) hArchetype;
	EditorStartObjectInsertion(objtype->pArchetype, _info);
}

static struct JumpgateManager *JUMPGATEMGR;
//----------------------------------------------------------------------------------------------
//
struct _jump : GlobalComponent
{
	virtual void Startup (void)
	{
		struct JumpgateManager *JumpgateMgr = new DAComponent<JumpgateManager>;
		JUMPGATEMGR = JumpgateMgr;
		AddToGlobalCleanupList((IDAComponent **) &JUMPGATEMGR);
	}

	virtual void Initialize (void)
	{
		JUMPGATEMGR->Init();
	}
};

static _jump jump;
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
void Render3db(Mesh *mesh,ColorRGB *color,const Transform &trans,SINGLE colorDamp)
{
	FaceGroup *fg = mesh->face_groups;
	//PIPE->set_render_state(D3DRS_ZENABLE,TRUE);
//	PIPE->set_render_state(D3DRS_ZWRITEENABLE,TRUE);

//	PIPE->set_texture_stage_texture( 0,mesh->material_list->texture_id);
	SetupDiffuseBlend(mesh->material_list->texture_id,TRUE);
//	PIPE->set_render_state(D3DRS_ALPHABLENDENABLE,FALSE);
	PIPE->set_render_state(D3DRS_DITHERENABLE,TRUE);

	Vector a = CAMERA->GetPosition()-trans.translation;
	a.normalize();
	a= trans.inverse_rotate(a);

	PB.Begin(PB_TRIANGLES);
	int i;
	for (i=0;i<mesh->face_cnt;i++)
	{
		U32 ref[3];
		U32 tref[3];
	
		ref[0] = mesh->vertex_batch_list[fg->face_vertex_chain[i*3]];
		ref[1] = mesh->vertex_batch_list[fg->face_vertex_chain[i*3+1]];
		ref[2] = mesh->vertex_batch_list[fg->face_vertex_chain[i*3+2]];

		tref[0] = mesh->texture_batch_list[fg->face_vertex_chain[i*3]];
		tref[1] = mesh->texture_batch_list[fg->face_vertex_chain[i*3+1]];
		tref[2] = mesh->texture_batch_list[fg->face_vertex_chain[i*3+2]];

		Vector v0,v1,v2;
		v0 = mesh->object_vertex_list[ref[0]];
		v1 = mesh->object_vertex_list[ref[1]];
		v2 = mesh->object_vertex_list[ref[2]];

		SINGLE f = fabs(dot_product(a,*(mesh->normal_ABC+mesh->vertex_normal[ref[0]])));
		f = colorDamp*max(f*1.5-0.5,0);

		PB.TexCoord2f(mesh->texture_vertex_list[tref[0]].u,mesh->texture_vertex_list[tref[0]].v);
		PB.Color3ub(f*color[ref[0]].r,f*color[ref[0]].g,f*color[ref[0]].b);   PB.Vertex3f(v0.x,v0.y,v0.z);
		f = fabs(dot_product(a,*(mesh->normal_ABC+mesh->vertex_normal[ref[1]])));
		f = colorDamp*max(f*1.5-0.5,0);
		PB.TexCoord2f(mesh->texture_vertex_list[tref[1]].u,mesh->texture_vertex_list[tref[1]].v);
		PB.Color3ub(f*color[ref[1]].r,f*color[ref[1]].g,f*color[ref[1]].b);   PB.Vertex3f(v1.x,v1.y,v1.z);
		f = fabs(dot_product(a,*(mesh->normal_ABC+mesh->vertex_normal[ref[2]])));
		f = colorDamp*max(f*1.5-0.5,0);
		PB.TexCoord2f(mesh->texture_vertex_list[tref[2]].u,mesh->texture_vertex_list[tref[2]].v);
		PB.Color3ub(f*color[ref[2]].r,f*color[ref[2]].g,f*color[ref[2]].b);   PB.Vertex3f(v2.x,v2.y,v2.z);
		
	}
	PB.End();
}

void vector_rotate_about_k (Vector *vec,SINGLE angle)
{
	SINGLE cosine, sine;
	Vector temp;

	angle = -angle;
	cosine = cos(angle);
	sine = sin(angle);

	temp.x = (cosine * vec->x) + (sine * vec->y);
	temp.y = (cosine * vec->y) - (sine * vec->x);

	vec->x = temp.x;
	vec->y = temp.y;
}
//---------------------------------------------------------------------------
//------------------------End Jumpgate.cpp----------------------------------
//---------------------------------------------------------------------------
