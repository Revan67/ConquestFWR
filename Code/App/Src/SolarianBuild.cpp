//--------------------------------------------------------------------------//
//                                                                          //
//                            SolarianBuild.cpp                             //
//                                                                          //
//                  COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/SolarianBuild.cpp 36    10/18/00 1:43a Jasony $
*/			    
//------------------------------- #INCLUDES --------------------------------//
//--------------------------------------------------------------------------//

#include "pch.h"
#include <globals.h>

#include "IBuild.h"
#include "ObjList.h"
#include "Startup.h"
#include "IExplosion.h"
#include "Camera.h"
#include "IBuildShip.h"
#include "SimpleMesh.h"
#include "IShipDamage.h"
#include "ILight.h"
#include "TManager.h"
#include "MeshRender.h"
#include "CQBatch.h"
#include "HPEnum.h"

#include <DBuildObj.h>
#include <DBuildSave.h>
//temp?
#include "TObject.h"

#include <Renderer.h>
#include <IHardPoint.h>
#include <LightMan.h>
#include <TSmartPointer.h>
#include <IConnection.h>
#include <ITextureLibrary.h>
#include <FileSys.h>
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//

//MeshRender.cpp
Vertex2 *GetScratchList2();
U16 *GetIndexScratchList();

struct SolarianBuildArchetype
{
	U32 gridTex;
	U32 beamTex;
};

#define DRONE_UPDATE_TIME 2
//#define GROW_START_TIME 2.0
//#define GROW_END_TIME 5.3
#define GRID_FADE_TIME 0.8
#define BEAM_FADE_TIME 0.2
#define BEAM_END_TIME (GRID_FADE_TIME-BEAM_FADE_TIME)

#define MAX_DRONE_CNT 12



struct FaceGroupDork
{
	//stored off real buffers
	U16 *index_list_rl;
	void *src_verts_rl;
	Vector *src_norms_rl;
	ArchetypeFaceInfo *faces_rl;
	S32 vert_cnt_rl;
	S32 face_cnt_rl;

	//local buffers
/*	U16 *index_list;
	void *src_verts;
	Vector *src_norms;
	ArchetypeFaceInfo *faces;
	S32 vert_cnt;
	S32 face_cnt;*/

	//z's stored for wipe
	Vector *vs;

	S32 MAX_vert_cnt;
	S32 MAX_face_cnt;

	int vert_type;

	IRenderMaterial *fgr;

	FaceGroupDork *next;

	FaceGroupDork()
	{
//		src_verts = 0;
		next = 0;
	}

	~FaceGroupDork()
	{
/*		delete [] index_list;
		delete [] src_verts;
		delete [] src_norms;
		delete [] faces;*/
		delete [] vs;
	}

	void Init(IRenderMaterial *src_fgr)
	{
		//store off handy pointers to the original mesh
		src_fgr->GetBuffers(&src_verts_rl,&index_list_rl,&src_norms_rl,&faces_rl,&vert_cnt_rl,&face_cnt_rl);
		vert_type = src_fgr->GetVertexType();
	}

	void MakeBuffers(int _vert_cnt,int _face_cnt,const Transform &trans,SINGLE &min_z,SINGLE &max_z)
	{
		//presumably the new mesh has more verts than the original
		CQASSERT(_vert_cnt > vert_cnt_rl);
		CQASSERT(_face_cnt > face_cnt_rl);

		MAX_vert_cnt = _vert_cnt;
		MAX_face_cnt = _face_cnt;

		fgr->index_list = new U16[_face_cnt*3];

		fgr->src_norms = new Vector[_vert_cnt];
		fgr->faces = new ArchetypeFaceInfo[_face_cnt];

		memcpy(fgr->index_list,index_list_rl,sizeof(U16)*face_cnt_rl*3);
		if (vert_type == D3DFVF_RPVERTEX)
		{
			CQASSERT(fgr->src_verts_buffer==0); 
			fgr->src_verts_buffer = new RPVertex[_vert_cnt];
			memcpy(fgr->src_verts_buffer,src_verts_rl,sizeof(RPVertex)*vert_cnt_rl);
			vs = new Vector[vert_cnt_rl];
			RPVertex *verts = (RPVertex *)fgr->src_verts_buffer;
			for (int i=0;i<vert_cnt_rl;i++)
			{
				vs[i] = (trans*verts[i].pos);
				if (vs[i].z > max_z)
					max_z = vs[i].z;
				if (vs[i].z < min_z)
					min_z = vs[i].z;
			}
		}
		else
		{
			fgr->src_verts_buffer = new Vertex2[_vert_cnt];
			memcpy(fgr->src_verts_buffer,src_verts_rl,sizeof(Vertex2)*vert_cnt_rl);
			vs = new Vector[vert_cnt_rl];
			Vertex2 *verts2 = (Vertex2 *)fgr->src_verts_buffer;
			for (int i=0;i<vert_cnt_rl;i++)
			{
				vs[i] = (trans*verts2[i].pos);
				if (vs[i].z > max_z)
					max_z = vs[i].z;
				if (vs[i].z < min_z)
					min_z = vs[i].z;
			}
		}
		memcpy(fgr->src_norms,src_norms_rl,sizeof(Vector)*vert_cnt_rl);
		memcpy(fgr->faces,faces_rl,sizeof(ArchetypeFaceInfo)*face_cnt_rl);


	}
/*
	void SetLocal()
	{
		CQASSERT(vert_cnt <= MAX_vert_cnt);
		CQASSERT(face_cnt <= MAX_face_cnt);
		fgr->SetBuffers(src_verts,index_list,src_norms,faces,vert_cnt,face_cnt);
	}
	
	void SetReal()
	{
		fgr->SetBuffers(src_verts_rl,index_list_rl,src_norms_rl,faces_rl,vert_cnt_rl,face_cnt_rl);
	}*/
};

struct MeshDork
{
	FaceGroupDork *fg_dork;
	IMeshRender *imr;

	void InitFaceGroupBuffers(struct MeshInfo *src_mc,const Transform &trans,SINGLE &min_z,SINGLE &max_z)
	{
		IRenderMaterial *fgr = imr->GetNextFaceGroup(0);
		IRenderMaterial *src_fgr = src_mc->mr->GetNextFaceGroup(0);
		FaceGroupDork *new_fg,*last_fg=0;

		while (fgr)
		{
			if ((src_mc->fgi[src_fgr->fg_idx].texture_flags & TF_F_HAS_ALPHA) == 0)
			{
				new_fg = new FaceGroupDork;
				new_fg->fgr = fgr;
				new_fg->Init(src_fgr);
				new_fg->MakeBuffers(new_fg->vert_cnt_rl*4,new_fg->face_cnt_rl*3,trans,min_z,max_z);
				
				if (fg_dork)
				{
					last_fg->next = new_fg;
					last_fg = new_fg;
				}
				else
					fg_dork = last_fg = new_fg;
			}

			fgr->face_offset *= 3;  //change me so I don't overlap

			fgr = imr->GetNextFaceGroup(fgr);
			src_fgr = src_mc->mr->GetNextFaceGroup(src_fgr);
		}
	}

	~MeshDork()
	{
		if (fg_dork)
		{
			FaceGroupDork *next;
			while (fg_dork)
			{
				next = fg_dork->next;
				delete fg_dork;
				fg_dork = next;
			}
		}
	}
};

struct EdgeSplit
{
//	Vector v1,v2,
	U16 ref1,ref2;
	U16 out_ref;
	U16 grid_ref;
	U16 goop_ref[2];
	U16 new_edge_ref;

	EdgeSplit *next;
};

struct NewEdge
{
	Vector v1,v2;
};

#define ES_BANK_SIZE 300

struct _NO_VTABLE SolarianBuild : public IBuildEffect, IHPEnumerator
{
	BEGIN_MAP_INBOUND(SolarianBuild)
	_INTERFACE_ENTRY(IBaseObject)
	_INTERFACE_ENTRY(IBuildEffect)
	END_MAP()

	//control variable
	SINGLE percent;

/*	U8 droneCnt;
	U8 droneFab;
	U8 numDrones;
	SINGLE droneMoveTime;*/

	SolarianBuildArchetype *arch;
	
	S32 numFabWorking;
	U32 fabID[MAX_FAB_WORKING];

	OBJPTR<IBaseObject> buildObj;
	OBJPTR<IFabricator> fabObj;
	INSTANCE_INDEX instanceIndex;

	Transform trans;

//	S32 totalClicks,thisClicks;
	SINGLE smoothTimer,gridTimer,totalTime,timePassed;
	//percent per second for visual neatness
	SINGLE rate;

	SMesh *smesh;
	Vector shieldScale;
	U32 lastReveal;

	SINGLE obj_length;
	SINGLE z_plane;
//	TRANSFORM fabTrans;

	//flaky stuff yet
	MeshInfo mc[MAX_CHILDS];
	int numChildren;
	MeshDork md[MAX_CHILDS];
	IMeshRender *imr[MAX_CHILDS];

	//beam stuff
	Vector beam_source_pos[3];
	Vector beam_pts[2];
	int num_beams;
	SINGLE min_x,max_x;
	SINGLE max_z,min_z;
	SINGLE sweep_x[3];
	SINGLE delta;

	//meshsplit stuff
	EdgeSplit *edgeSplitHash[8];
	Vector v_out[400];
#define NEW_EDGE_MAX 600
	U16 new_edge[NEW_EDGE_MAX];
	U16 new_edge_ref;
	U16 total_out_verts;
	//why aren't these 2 local?
	U16 re_index_list1[1500];
	U16 re_index_list2[1500];
	int vert_cnt1,vert_cnt2;
	RPVertex *v_list1,*v_list2;
	void *verts;

	int next_es;
	EdgeSplit es_bank[ES_BANK_SIZE];

	bool bBuildFromBack:1;
	bool bPaused:1;

	SolarianBuild (void) 
	{
		sweep_x[2] = 600;
	}

	virtual ~SolarianBuild (void);	// See ObjList.cpp

	virtual BOOL32 Update (void);

	virtual void PhysicalUpdate (SINGLE dt);

	virtual void SetupMesh (IBaseObject *fab,IBaseObject *obj,struct IMeshInfoTree *mesh_info,INSTANCE_INDEX _instanceIndex,SINGLE _totalTime);

	virtual void SetupMesh (IBaseObject *fab,struct IMeshInfoTree *mesh_info,SINGLE _totalTime)
	{}

	virtual void SetupMesh (IBaseObject *fab,IBaseObject *obj,SINGLE _totalTime);

	virtual void SetBuildPercent (SINGLE newPercent);

	//hint for visuals
	virtual void SetBuildRate (SINGLE percentPerSecond);

	virtual void AddFabricator (IFabricator *_fab);

	virtual void RemoveFabricator (IFabricator *_fab);

	virtual void SynchBuilderShips ();

	virtual void Done ();

	virtual void Render (void);

	virtual void PrepareForExplode (void)
	{}

	virtual void PauseBuildEffect (bool bPause)
	{
		bPaused = bPause;
	}

	/* SolarianBuild methods */

	void renderBeams ();

	void revealShip ();

	void splitEdge(FaceGroupDork *fg_dork,U16 ref1,U16 ref2,EdgeSplit **es);

	void splitEdge2(FaceGroupDork *fg_dork,U16 ref1,U16 ref2,EdgeSplit **es);

	void renderRevealed ();

	bool EnumerateHardpoint (const HPENUMINFO & info);

//	bool timeForDroneMove ();
//	ISpiderDrone * getNextDrone ();
//	void resetDrones();
};

//---------------------------------------------------------------------------
//
SolarianBuild::~SolarianBuild (void)
{
//	delete [] hidden;
//	DeleteMeshRenders(imr,numChildren);
	for (int i=0;i<numChildren;i++)
	{
		if (imr[i])
			imr[i]->Release();
	}
}

void SolarianBuild::SetupMesh (IBaseObject *fab,IBaseObject *obj,struct IMeshInfoTree *mesh_info,INSTANCE_INDEX _instanceIndex,SINGLE _totalTime)
{
	instanceIndex = _instanceIndex;
	CQASSERT(instanceIndex != -1);

	if (obj)
	{
		obj->QueryInterface(IBaseObjectID,buildObj,NONSYSVOLATILEPTR);
		obj->bSpecialRender = true;
	}
	
	MeshInfo *src_mc[MAX_CHILDS];
	numChildren = mesh_info->ListChildren(src_mc);

	totalTime = _totalTime;

	TRANSFORM fabTrans;
	if (fab)
	{
		fab->QueryInterface(IFabricatorID,fabObj, NONSYSVOLATILEPTR);
		fabTrans = fab->GetTransform();
		EnumerateHardpoints(fab->GetObjectIndex(),"hp_beam*",this);
	}
	else
		fabTrans = ENGINE->get_transform(instanceIndex);

	if (num_beams == 1)
	{
		beam_source_pos[num_beams] = beam_source_pos[0];
		num_beams++;
	}

//	Vector beam_dir = -fabTrans.get_k();
//	Vector beam_norm(beam_dir.y,-beam_dir.z,0);

//	beam_source_pos = Vector(0,0,0);
//	beam_pts[0] = beam_source_pos+beam_dir*1000+beam_norm*1000;
//	beam_pts[1] = beam_source_pos+beam_dir*1000-beam_norm*1000;

//	OBJBOX box;
//	buildObj->GetObjectBox(box);
//	obj_length = max(box[BBOX_MAX_Z],-box[BBOX_MIN_Z])*2;

	
	TRANSFORM bo_trans = ENGINE->get_transform(instanceIndex);
	TRANSFORM inv = bo_trans.get_inverse();

	max_z = -999999;
	min_z = 999999;

	/*if (dot_product(bo_trans.get_k(),bo_trans.translation-fabTrans.translation) > 0)
	{
		//sweep the other way
		inv.d[0][0] *= -1;
		inv.d[1][0] *= -1;
		inv.d[2][0] *= -1;
		inv.translation.z *= -1;
		inv.d[0][2] *= -1;
		inv.d[1][2] *= -1;
		inv.d[2][2] *= -1;
		inv.translation.x *= -1;
	}*/

	bBuildFromBack = false;
	if (dot_product(bo_trans.get_k(),bo_trans.translation-fabTrans.translation) > 0)
		bBuildFromBack = true;

	//Setup the mesh mangling data
//	AllocateMeshRenders(imr,numChildren);

	for (int k=0;k<numChildren;k++)
	{
		if (src_mc[k]->bHasMesh)
		{
			TRANSFORM trans = inv*ENGINE->get_transform(src_mc[k]->instanceIndex);
			if (bBuildFromBack)
			{
				//sweep the other way
				trans.d[0][0] *= -1;
				trans.d[1][0] *= -1;
				trans.d[2][0] *= -1;
				trans.translation.z *= -1;
				trans.d[0][2] *= -1;
				trans.d[1][2] *= -1;
				trans.d[2][2] *= -1;
				trans.translation.x *= -1;
			}

			//create my local MeshRender - but don't copy the buffers
			imr[k] = CreateMeshRender();
			imr[k]->AddRef();
			CopyMesh(src_mc[k]->mr,imr[k],false);
			//Tell the MeshDork class where its data will be
			md[k].imr = imr[k];
			//Make the new JUMBO sized facegroups I'll need
			md[k].InitFaceGroupBuffers(src_mc[k],trans,min_z,max_z);
			//make a local mc with jumbo buffers
			
			//mc[k].instanceIndex = src_mc[k]->instanceIndex;
			//mc[k].buildMesh = src_mc[k]->buildMesh;
			CopyMeshInfo(mc[k],*src_mc[k],0);
			mc[k].mr = md[k].imr;
			mc[k].mr->AddRef();
			mc[k].bBroken = true;
			//hide the mesh here - if necessary
			///////////////////
			mc[k].faceRenders = new U8[src_mc[k]->mr->face_cnt*3+1];
			memset(mc[k].faceRenders,0x00,sizeof(U8)*(src_mc[k]->mr->face_cnt*3+1));
			for (int f=0;f<src_mc[k]->mr->face_cnt*3;f++)
				mc[k].faceRenders[f] |= FS__HIDDEN;
		}
		else
		{
			mc[k].bHasMesh = 0;
			mc[k].instanceIndex = src_mc[k]->instanceIndex;
		}
	}

	obj_length = max_z-min_z;
}
//---------------------------------------------------------------------------
//  FOR SHIPS
//
void SolarianBuild::SetupMesh (IBaseObject *fab,IBaseObject *obj,SINGLE _totalTime)
{
	CQASSERT(0 && "No longer supported");
}
//---------------------------------------------------------------------------
//  FOR MORPHING
//
/*void SolarianBuild::SetupMesh (IBaseObject *fab,struct IMeshInfo *mesh_info,SINGLE _totalTime)
{
	OBJPTR<IFabricator> fabo;
	fab->QueryInterface(IFabricatorID,fabo);

	num_childs = mesh_info->ListChildren(mc);

	int vert_cnt=0;
	int face_cnt=0;
	for (int c=0;c<num_childs;c++)
	{
		vert_cnt += mc[c]->buildMesh->object_vertex_cnt;
		face_cnt += mc[c]->buildMesh->face_cnt;
	}

	pellet_verts = new Vector[vert_cnt];
	hidden = new bool[face_cnt];
	memset(hidden,0,sizeof(bool)*face_cnt);
	int i=0;
	for (c=0;c<num_childs;c++)
	{
		for (int v=0;v<mc[c]->buildMesh->object_vertex_cnt;v++)
		{
			pellet_verts[i] = mc[c]->buildMesh->object_vertex_list[v];
			pellet_verts[i].normalize();
			pellet_verts[i] *= 50;
		
			i++;
		}
	}

//	if (smesh->e_list == 0)
//		smesh->MakeEdges();

	numDrones = fabo->GetNumDrones();
	CQASSERT(numDrones <= MAX_DRONE_CNT);
	
	for (i=0;i<numDrones;i++)
	{
		droneRef[i] = rand()%smesh->v_cnt;
	}
	
	//danger!!
	Transform trans = fab->GetTransform();

	buildObj->bSpecialRender = true;

	totalTime = _totalTime;
	deployTime = GROW_START_TIME;

  //	end_pos = obj->GetPosition();
  
}*/
//---------------------------------------------------------------------------
//
void SolarianBuild::SetBuildPercent (SINGLE newPercent)
{
	bPaused = false;
	//timePassed = newPercent*totalTime;
	if (newPercent != percent)
		smoothTimer = newPercent;//timePassed;
	percent = newPercent;
	
	//thisClicks = percent*totalClicks;
}
//--------------------------------------------------------------------------//
//
void SolarianBuild::SetBuildRate (SINGLE percentPerSecond)
{
	bPaused = false;
	rate = percentPerSecond;
}
//--------------------------------------------------------------------------//
//
BOOL32 SolarianBuild::Update (void)
{
/*	if (smesh)
	{
		droneMoveTime += ELAPSED_TIME;
		
		if (totalTime-timePassed < GROW_END_TIME)
		{
			SINGLE ratio;
			ratio = -(totalTime-timePassed-GROW_END_TIME)/GROW_END_TIME;
			CQASSERT(ratio <= 1.0);
			
			S32 reveal = smesh->f_cnt*ratio;
			
			for (int f=lastReveal;f<reveal;f++)
			{
				hidden[f] = true;
			}
			
			lastReveal = reveal;
		}
	}*/
	
	return 1;
}
//--------------------------------------------------------------------------//
//
void SolarianBuild::PhysicalUpdate (SINGLE dt)
{
	if (bPaused)
		return;

	delta = dt;
	smoothTimer += dt*rate;
	gridTimer -= dt;
	if (gridTimer < 0)
		gridTimer = GRID_FADE_TIME;

	if (smesh)
	{
//		deployTimer += dt;

		if (smoothTimer > 1.0)
			smoothTimer = 1.0;
		if (smoothTimer < 0.0)
			smoothTimer = 0.0;
	//	thisClicks = percent*totalClicks;
/*		if (deployTimer < deployTime)
		{
			SINGLE ratio = deployTimer/deployTime;
			ENGINE->set_position(buildObj.ptr->GetObjectIndex(),ratio*end_pos+(1-ratio)*start_pos);
		}*/
	}
}
//---------------------------------------------------------------------------
//
void SolarianBuild::Render (void)
{
	revealShip();

	renderRevealed();

	renderBeams();
}

void SolarianBuild::renderBeams()
{
	if (num_beams == 0)
		return;

	int v_cnt=0;
	int id_cnt=0;
	
	Transform oTrans = ENGINE->get_transform(instanceIndex);//buildObj->GetTransform();
	TRANSFORM fabTrans;
	if (fabObj)
	{
		if (!fabObj->IsFabAtObj())
			return;

		fabTrans = fabObj.Ptr()->GetTransform();
	}
	else
		fabTrans = oTrans;

	Transform inv = fabTrans.get_inverse();

	if (bBuildFromBack)
	{
		//sweep the other way
		oTrans.d[0][0] *= -1;
		oTrans.d[1][0] *= -1;
		oTrans.d[2][0] *= -1;
	//	oTrans.translation.z *= -1;
		oTrans.d[0][2] *= -1;
		oTrans.d[1][2] *= -1;
		oTrans.d[2][2] *= -1;
	//	oTrans.translation.x *= -1;
	}
	Transform objTrans = inv*oTrans;

	SINGLE dt = OBJLIST->GetRealRenderTime();
	static SINGLE rrtime = 0;
	rrtime -= 2*dt;
	
	//		Vector beam_end = beam_pos[i];
	/*	if (beam_dir[i].x > 0)
	{
	beam_end.z = -z_plane;
	z_pos = min_z_n+(i%5)*((max_z_n-min_z_n)*0.24);
	}
	else
	{
	beam_end.z = z_plane;
	z_pos = min_z_p+(i%5)*((max_z_p-min_z_p)*0.24);
}*/
	
	BATCH->set_state(RPR_BATCH,TRUE);
	
	BATCH->set_render_state(D3DRS_ALPHABLENDENABLE,TRUE);
	BATCH->set_render_state(D3DRS_ZENABLE,TRUE);
	BATCH->set_render_state(D3DRS_ZWRITEENABLE,FALSE);
	BATCH->set_render_state(D3DRS_SRCBLEND,D3DBLEND_ONE);
	BATCH->set_render_state(D3DRS_DESTBLEND,D3DBLEND_ONE);
	BATCH->set_render_state(D3DRS_CULLMODE,D3DCULL_NONE);
	
	SetupDiffuseBlend(arch->beamTex,FALSE);
	
	CAMERA->SetModelView(&fabTrans);
	BATCH->set_state(RPR_STATE_ID,arch->beamTex);
	
	BATCHDESC desc;
	desc.type = D3DPT_TRIANGLELIST;
	desc.vertex_format = D3DFVF_RPVERTEX;
	desc.num_verts = 4*num_beams;
	desc.num_indices = 6*num_beams;
	CQBATCH->GetPrimBuffer(&desc);
	RPVertex *v_list = (RPVertex *)desc.verts;
	U16 *id_list = desc.indices;
	
	for (int bb=0;bb<num_beams;bb++)
	{
		bool bSweep=(bb%2 == 0);
		if (bSweep)
		{
			sweep_x[bb] += 800*delta;
			if (sweep_x[bb] > max_x)
				sweep_x[bb] = 0;
		}
		else
		{
			sweep_x[bb] -= 800*delta;
			if (sweep_x[bb] < min_x)
				sweep_x[bb] = 0;
		}

		int closest=-1;
		int found=-1;
		SINGLE dist=99999.0f;
		
		CQASSERT(new_edge_ref%2 == 0);
		
		int a=0;
		while (a<new_edge_ref && found == -1)
		{
			//swap to put points in order
			if (v_out[new_edge[a]].x > v_out[new_edge[a+1]].x)
			{
				int temp=new_edge[a];
				new_edge[a] = new_edge[a+1];
				new_edge[a+1] = temp;
			}
			
			if (sweep_x[bb] > v_out[new_edge[a]].x)
			{
				if (sweep_x[bb] < v_out[new_edge[a+1]].x)
				{
					found = a;
				}
				else if (bSweep==0 && fabs(v_out[new_edge[a]].x-sweep_x[bb]) < dist)
				{
					closest = a;
					dist = fabs(v_out[new_edge[a]].x-sweep_x[bb]);
				}
			}
			else
			{
				if ((bSweep) && fabs(v_out[new_edge[a+1]].x-sweep_x[bb]) < dist)
				{
					closest = a;
					dist = fabs(v_out[new_edge[a+1]].x-sweep_x[bb]);
				}
			}
			
			a+=2;
		}
		
		if (found == -1 && closest != -1)
		{
			found = closest;
			sweep_x[bb] = v_out[new_edge[found]].x;
		}
		
		if (found != -1)
		{

			
			Vector temp;
			
			Vector up,down;
			
			SINGLE diff = (v_out[new_edge[found+1]].x-v_out[new_edge[found]].x);
			Vector beam_end;
			if (diff != 0)
			{
				beam_end = objTrans*(v_out[new_edge[found]]+(sweep_x[bb]-v_out[new_edge[found]].x)*(v_out[new_edge[found+1]]-v_out[new_edge[found]])/diff);
			}
			else
				beam_end = objTrans*v_out[new_edge[found]];
			
			const Transform * camTrans = CAMERA->GetTransform();
			Vector norm = cross_product(inv.rotate(camTrans->get_k()),beam_source_pos[bb]-beam_end);
			norm.normalize();
			norm *= 100;
			
			up = beam_end+norm;
			
			down = beam_end-norm;
			
			v_list[v_cnt].pos = beam_source_pos[bb]-norm;
			v_list[v_cnt].u = rrtime;
			v_list[v_cnt].v = 0.75;
			v_list[v_cnt].color = 0xffffffff;
			v_cnt++;
			v_list[v_cnt].pos = beam_source_pos[bb]+norm;
			v_list[v_cnt].u = rrtime;
			v_list[v_cnt].v = 1.0;
			v_list[v_cnt].color = 0xffffffff;
			v_cnt++;
			v_list[v_cnt].pos = up;
			v_list[v_cnt].u = rrtime+1.0;
			v_list[v_cnt].v = 1.0;
			v_list[v_cnt].color = 0xffffffff;
			v_cnt++;
			v_list[v_cnt].pos = down;
			v_list[v_cnt].u = rrtime+1.0;
			v_list[v_cnt].v = 0.75;
			v_list[v_cnt].color = 0xffffffff;
			v_cnt++;
			
			id_list[id_cnt++] = v_cnt-4;
			id_list[id_cnt++] = v_cnt-3;
			id_list[id_cnt++] = v_cnt-2;
			id_list[id_cnt++] = v_cnt-4;
			id_list[id_cnt++] = v_cnt-2;
			id_list[id_cnt++] = v_cnt-1;
			
		}
	}
	desc.num_verts = v_cnt;
	desc.num_indices = id_cnt;
	CQBATCH->ReleasePrimBuffer(&desc);
	BATCH->set_state(RPR_STATE_ID,0);
	delta = 0;
}

void SolarianBuild::revealShip()
{
	SINGLE progress = smoothTimer*1.1f-0.1f;
	if (progress > 1.0f)
		progress = 1.0f;
	if (progress < 0.0f)
		progress = 0.0f;
//	z_plane = progress*obj_length-0.5*obj_length;
	z_plane = min_z+progress*(max_z-min_z);
}

typedef void (EDGER)(FaceGroupDork *fg_dork,U16 ref1,U16 ref2,EdgeSplit **es,SolarianBuild *that);

void GsplitEdge(FaceGroupDork *fg_dork,U16 ref1,U16 ref2,EdgeSplit **es,SolarianBuild *that)
{
	that->splitEdge(fg_dork,ref1,ref2,es);
}

void GsplitEdge2(FaceGroupDork *fg_dork,U16 ref1,U16 ref2,EdgeSplit **es,SolarianBuild *that)
{
	that->splitEdge2(fg_dork,ref1,ref2,es);
}

void SolarianBuild::splitEdge(FaceGroupDork *fg_dork,U16 ref1,U16 ref2,EdgeSplit **es)
{
	int hash = (ref1+ref2)%8;
	
	EdgeSplit *pos = edgeSplitHash[hash];

	while ( pos && (ref1 != pos->ref1 || ref2 != pos->ref2) )
	{
		pos = pos->next;
	}

	if (pos)
	{
		*es = pos;
	}
	else
	{
		EdgeSplit *new_es = &es_bank[next_es++];
		CQASSERT(next_es < ES_BANK_SIZE);
		new_es->next = edgeSplitHash[hash];
		edgeSplitHash[hash] = new_es;
		*es = new_es;
		
		new_es->ref1 = ref1;
		new_es->ref2 = ref2;
					
		Vector *vec1 = &fg_dork->vs[ref1];
		Vector *vec2 = &fg_dork->vs[ref2];

		new_es->goop_ref[0] = 0xffff;
		new_es->goop_ref[1] = 0xffff;
		new_es->grid_ref = 0xffff;
		new_es->out_ref = 0xffff;
		int goop=0;

#define GOOP_WIDTH 200

		if (vec1->z <= z_plane-GOOP_WIDTH) //one of the points is in the "done area"
		{
			new_es->goop_ref[goop] = vert_cnt1++;
			
			SINGLE ratio = (z_plane-GOOP_WIDTH-vec1->z)/(vec2->z-vec1->z);
			Vector vec = *vec1 + (*vec2-*vec1)*ratio;
			v_list1[new_es->goop_ref[goop]].pos = vec;
			v_list1[new_es->goop_ref[goop]].color = 0;//0xffffffff;
		//	v_list1[new_es->goop_ref[goop]].a = 0;//255*(GOOP_WIDTH-(vec.z-z_plane))/GOOP_WIDTH;
			goop++;
		}

		if (vec2->z >= z_plane) //one of the points is in the "grid area"
		{
			//goop edge
			SINGLE ratio = (z_plane-vec1->z)/(vec2->z-vec1->z);
			Vector vec = *vec1 + (*vec2-*vec1)*ratio;
			new_es->goop_ref[goop] = vert_cnt1++;
			v_list1[new_es->goop_ref[goop]].pos = vec;
			v_list1[new_es->goop_ref[goop]].color = 0xffffffff;

			new_es->out_ref = fg_dork->fgr->vert_cnt++;
			re_index_list2[new_es->out_ref] = vert_cnt2;
			new_es->grid_ref = vert_cnt2++;
			v_list2[new_es->grid_ref].pos = vec;

			RPVertex *vert1 = &(((RPVertex *)verts)[ref1]);
			RPVertex *vert2 = &(((RPVertex *)verts)[ref2]);
			RPVertex *new_vert = &(((RPVertex *)verts)[new_es->out_ref]);
				
			if (vec.x < min_x)
				min_x = vec.x;
			if (vec.x > max_x)
				max_x = vec.x;
			
			new_es->new_edge_ref = total_out_verts;
			v_out[total_out_verts++] = vec;
			CQASSERT(total_out_verts < 400);
			new_vert->pos = vert1->pos + (vert2->pos-vert1->pos)*ratio;
			new_vert->u = vert1->u + (vert2->u-vert1->u)*ratio;
			new_vert->v = vert1->v + (vert2->v-vert1->v)*ratio;


		}
	}
}

void SolarianBuild::splitEdge2(FaceGroupDork *fg_dork,U16 ref1,U16 ref2,EdgeSplit **es)
{
	int hash = (ref1+ref2)%8;
	
	EdgeSplit *pos = edgeSplitHash[hash];

	while ( pos && (ref1 != pos->ref1 || ref2 != pos->ref2) )
	{
		pos = pos->next;
	}

	if (pos)
	{
		*es = pos;
	}
	else
	{
		EdgeSplit *new_es = &es_bank[next_es++];
		CQASSERT(next_es < ES_BANK_SIZE);
		new_es->next = edgeSplitHash[hash];
		edgeSplitHash[hash] = new_es;
		*es = new_es;

		new_es->ref1 = ref1;
		new_es->ref2 = ref2;
		Vector *vec1 = &fg_dork->vs[ref1];
		Vector *vec2 = &fg_dork->vs[ref2];

		new_es->goop_ref[0] = 0xffff;
		new_es->goop_ref[1] = 0xffff;
		new_es->grid_ref = 0xffff;
		new_es->out_ref = 0xffff;
		int goop=0;
		
		if (vec1->z <= z_plane-GOOP_WIDTH) //one of the points is in the "done area"
		{
			new_es->goop_ref[goop] = vert_cnt1++;
			
			SINGLE ratio = (z_plane-GOOP_WIDTH-vec1->z)/(vec2->z-vec1->z);
			Vector vec = *vec1 + (*vec2-*vec1)*ratio;
			v_list1[new_es->goop_ref[goop]].pos = vec;
			v_list1[new_es->goop_ref[goop]].color = 0;//0xffffffff;
		//	v_list1[new_es->goop_ref[goop]].a = 0;//255*(GOOP_WIDTH-(vec.z-z_plane))/GOOP_WIDTH;
			goop++;
		}

		if (vec2->z >= z_plane) //one of the points is in the "grid area"
		{
			//goop edge
			SINGLE ratio = (z_plane-vec1->z)/(vec2->z-vec1->z);
			Vector vec = *vec1 + (*vec2-*vec1)*ratio;
			new_es->goop_ref[goop] = vert_cnt1++;
			v_list1[new_es->goop_ref[goop]].pos = vec;
			v_list1[new_es->goop_ref[goop]].color = 0xffffffff;

			new_es->out_ref = fg_dork->fgr->vert_cnt++;
			re_index_list2[new_es->out_ref] = vert_cnt2;
			new_es->grid_ref = vert_cnt2++;
			v_list2[new_es->grid_ref].pos = vec;

			Vertex2 *vert1 = &(((Vertex2 *)verts)[ref1]);
			Vertex2 *vert2 = &(((Vertex2 *)verts)[ref2]);
			Vertex2 *new_vert = &(((Vertex2 *)verts)[new_es->out_ref]);
				
			if (vec.x < min_x)
				min_x = vec.x;
			if (vec.x > max_x)
				max_x = vec.x;
			
			new_es->new_edge_ref = total_out_verts;
			v_out[total_out_verts++] = vec;
			CQASSERT(total_out_verts < 400);
			new_vert->pos = vert1->pos + (vert2->pos-vert1->pos)*ratio;
			new_vert->u = vert1->u + (vert2->u-vert1->u)*ratio;
			new_vert->v = vert1->v + (vert2->v-vert1->v)*ratio;
			new_vert->u2 = vert1->u2 + (vert2->u2-vert1->u2)*ratio;
			new_vert->v2 = vert1->v2 + (vert2->v2-vert1->v2)*ratio;
		}
	}
}

void SolarianBuild::renderRevealed()
{
	//setup grid pulsing
	U8 r=200,g=200,b=200,a=255;
	SINGLE ramp=1.0f;
	if (smoothTimer<=0.1f)
		ramp = smoothTimer/0.1f;

	ramp *= 0.3+7.0f*fabs(0.01-fmod(smoothTimer,0.02f));
	
	r = F2LONG(240.0f*ramp);
	b = F2LONG(240.0f*ramp);
	g = F2LONG(240.0f*ramp);
	
	int gridColor = a<<24|r<<16|g<<8|b;
	
	max_x = min_x = 0;
	
	vert_cnt1=0;
	int old_vert_cnt2=0;
	vert_cnt2=0;
	int id_cnt2=0;
	int id_cnt1=0;
	total_out_verts = 0;
	new_edge_ref = 0;
	
	EDGER *edger;
	edger=GsplitEdge;
	
	for (int c=0;c<numChildren;c++)
	{
		/*for (int i=0;i<8;i++)
		{
			edgeSplitHash[i] = 0;
		}
		next_es = 0;*/
		vert_cnt1=0;
		old_vert_cnt2 = vert_cnt2 = 0;
		id_cnt1 = id_cnt2 = 0;
	//	Mesh *m = REND->get_instance_mesh(mc[c].instanceIndex);
		if (mc[c].instanceIndex != INVALID_INSTANCE_INDEX && mc[c].bHasMesh)
		{
			BATCH->set_state(RPR_BATCH,TRUE);
			
			U16 *id_list1=0,*id_list2=0;
			BATCHDESC desc,desc2;
			bool bGotBuffers=false;

			while (!bGotBuffers)
			{
				BATCH->set_render_state(D3DRS_ALPHABLENDENABLE,TRUE);
				BATCH->set_render_state(D3DRS_ZENABLE,TRUE);
				BATCH->set_render_state(D3DRS_CULLMODE,D3DCULL_NONE);
				
				trans = ENGINE->get_transform(mc[0].instanceIndex);
				if (bBuildFromBack)
				{
					//sweep the other way
					trans.d[0][0] *= -1;
					trans.d[1][0] *= -1;
					trans.d[2][0] *= -1;
					trans.d[0][2] *= -1;
					trans.d[1][2] *= -1;
					trans.d[2][2] *= -1;
				}
				CAMERA->SetModelView(&trans);
				
				BATCH->set_render_state(D3DRS_ZWRITEENABLE,FALSE);
				BATCH->set_render_state(D3DRS_SRCBLEND,D3DBLEND_ONE);
				BATCH->set_render_state(D3DRS_DESTBLEND,D3DBLEND_ONE);
				BATCH->set_render_state(D3DRS_ZFUNC,D3DCMP_LESSEQUAL);
				SetupDiffuseBlend(arch->gridTex,FALSE);
				BATCH->set_state(RPR_STATE_ID,arch->gridTex);
				
				desc2.type = D3DPT_TRIANGLELIST;
				desc2.vertex_format = D3DFVF_RPVERTEX;
				desc2.num_verts = mc[c].mr->face_cnt*5;
				desc2.num_indices = mc[c].mr->face_cnt*7;
				CQBATCH->GetPrimBuffer(&desc2);
				v_list2 = (RPVertex *)desc2.verts;
				id_list2 = desc2.indices;
				
				TRANSFORM camtrans(false);
				camtrans = *CAMERA->GetInverseTransform();
				camtrans.translation.z += 10;
				trans = camtrans*trans;
				BATCH->set_modelview(trans);
				DisableTextures();
				BATCH->set_state(RPR_STATE_ID,arch->gridTex+1);
				BATCH->set_state(RPR_DELAY,1);
				
				desc.type = D3DPT_TRIANGLELIST;
				desc.vertex_format = D3DFVF_RPVERTEX;
				desc.num_verts = mc[c].mr->face_cnt*3;
				desc.num_indices = mc[c].mr->face_cnt*6;
				bGotBuffers = CQBATCH->GetPrimBuffer(&desc,true);  //theoretically, failure can only occur on the second buffer to open
				v_list1 = (RPVertex *)desc.verts;
				id_list1 = desc.indices;
			}
			
			BATCH->set_state(RPR_DELAY,0);
			
			FaceGroupDork * fg_dork = md[c].fg_dork;
			int fg_cnt=0;
			while (fg_dork)
			{
				IRenderMaterial *fgr = fg_dork->fgr;

				//for now, fgi is parallel to Polymesh facegroups
				fg_cnt = fgr->fg_idx;
				if ((mc[c].fgi[fg_cnt].texture_flags & TF_F_HAS_ALPHA) == 0)
				{
					U8 * faceRenders = &mc[c].faceRenders[fgr->face_offset];
					for (int i=0;i<8;i++)
					{
						edgeSplitHash[i] = 0;
					}
					next_es = 0;
					
					
					//setup list to be used for packing
					memset(re_index_list1,0xff,1500*2);
					memset(re_index_list2,0xff,1500*2);
					
					fgr->vert_cnt = fg_dork->vert_cnt_rl;
					fgr->new_face_cnt = fg_dork->face_cnt_rl;
					
					verts = fgr->src_verts_buffer;
					
					for (S32 f=0;f<fg_dork->face_cnt_rl;f++)
					{
						//wouldn't checking for hidden here be counter-productive?
						if (1) //(faceRenders[f] & FS__HIDDEN == 0) || rate < 0)
						{
							if (fg_dork->vert_type == D3DFVF_RPVERTEX)
							{
								edger = GsplitEdge;
							}
							else
								edger = GsplitEdge2;
							
							//	RPVertex *verts=(RPVertex *)fgr->src_verts_buffer;
							int ref[3];
							//Vector v_out1[4],v_out2[4];
							int split[4],in1[4],in2[4],out1[5],out2[4];
							//							NewEdge * ne1,ne2;
							
							ref[0] = fgr->index_list[f*3];
							ref[1] = fgr->index_list[f*3+1];
							ref[2] = fgr->index_list[f*3+2];
							
							//split the polygon  eek!
							int pts=0;
							int pts1=0;
							int pts2=0;
							
							int i;
							for (i=0;i<3;i++)
							{
								if (fg_dork->vs[ref[i]].z < z_plane)
								{
									if (fg_dork->vs[ref[i]].z > z_plane-GOOP_WIDTH) //leading edge
									{
										//gooey stuff
										if (re_index_list1[ref[i]] == 0xffff)
										{
											v_list1[vert_cnt1].pos = fg_dork->vs[ref[i]];
											U8 fade = 255*(v_list1[vert_cnt1].pos.z-(z_plane-GOOP_WIDTH))/GOOP_WIDTH;
											v_list1[vert_cnt1].color = fade<<16|fade<<8|fade;
											re_index_list1[ref[i]] = vert_cnt1++;
										}
										
										out1[pts1] = re_index_list1[ref[i]];
										in1[pts1] = ref[i];
										pts1++;
									}
									else
									{
										//Revealed already
										split[pts] = ref[i];
										pts++;
									}
								}
								else
								{
									//grid stuff
									if (re_index_list2[ref[i]] == 0xffff)
									{
										v_list2[vert_cnt2].pos = fg_dork->vs[ref[i]];
										re_index_list2[ref[i]] = vert_cnt2++;
									}
									
									out2[pts2] = re_index_list2[ref[i]];
									in2[pts2] = ref[i];
									pts2++;
								}
							}
							
							int final_pts = pts;
							int final_pts1 = pts1;
							int final_pts2 = pts2;
							
							//copy goop area points into "done" area
							for (i=0;i<pts1;i++)
							{
								split[final_pts++] = in1[i];
								CQASSERT(fgr->src_norms[in1[i]].x+7);
							}
							
							for (i=0;i<pts;i++)
							{
								for (int j=0;j<pts1;j++)
								{
									EdgeSplit *es;
									
									edger(fg_dork,split[i],in1[j],&es,this);
									
									/*	if (es->out_ref != 0xffff)
									{
									split[final_pts] = es->out_ref;
									fgr->src_norms[split[final_pts]] = fgr->faces[f].norm;
									final_pts++;
								}*/
									
									//CQASSERT(es->out_ref == 0xffff);
									out1[final_pts1++] = es->goop_ref[0];
									//CQASSERT (es->goop_ref[1] == 0xffff);
									//		out1[final_pts1++] = es->goop_ref[1];
									
									
								}
							}
							
							CQASSERT(new_edge_ref%2 == 0);
						//	CQASSERT(new_edge_ref != 268 || pts2!=2);
							
							for (i=0;i<pts;i++)
							{
								for (int j=0;j<pts2;j++)
								{
									EdgeSplit *es;
									
									edger(fg_dork,split[i],in2[j],&es,this);
									
									CQASSERT(es->out_ref < fgr->vert_cnt);
									split[final_pts] = es->out_ref;
									fgr->src_norms[split[final_pts]] = fgr->faces[f].norm;
									final_pts++;
									
									CQASSERT(es->grid_ref < vert_cnt2);
									out1[final_pts1++] = es->goop_ref[0];
									//CQASSERT (es->goop_ref[1] != 0xffff)
									out1[final_pts1++] = es->goop_ref[1];
									new_edge[new_edge_ref++] = es->new_edge_ref;
									CQASSERT(es->goop_ref[0] < vert_cnt1);
									out2[final_pts2++] = es->grid_ref;
									CQASSERT(es->grid_ref < vert_cnt2);
									
									
								}
							}
							
							int j;
							//flip is necessary to make coherent polygons
							for (i=0;i<pts1;i++)
							{
								for (j=0;j<pts2;j++)
								{
									EdgeSplit *es;
									
									edger(fg_dork,in1[i],in2[j],&es,this);
									
									//CQASSERT(es->out_ref != 0xffff);
									CQASSERT(es->out_ref < fgr->vert_cnt);
									split[final_pts] = es->out_ref;
									fgr->src_norms[split[final_pts]] = fgr->faces[f].norm;
									final_pts++;
									
									CQASSERT(es->goop_ref[0] < vert_cnt1);
									//CQASSERT(es->goop_ref[1] == 0xffff);
									out1[final_pts1++] = es->goop_ref[0];
									CQASSERT(new_edge_ref < NEW_EDGE_MAX);
									new_edge[new_edge_ref++] = es->new_edge_ref;
									CQASSERT(es->grid_ref < vert_cnt2);
									out2[final_pts2++] = es->grid_ref;
									
								}
							}

							CQASSERT(new_edge_ref%2 == 0);
							
							CQASSERT(final_pts != 1 && final_pts != 2);
							

							
							U16 * id_list = fgr->index_list;
							int id_cnt = fgr->new_face_cnt*3;
							
							if (final_pts)
							{
								id_list[id_cnt] = split[0];
								id_list[id_cnt+1] = split[1];
								id_list[id_cnt+2] = split[2];
								
								for (int a=0;a<final_pts;a++)
								{
									CQASSERT(fgr->src_norms[split[a]].x+7);
								}
								
								CQASSERT(split[2] < fgr->vert_cnt);
								
								fgr->faces[fgr->new_face_cnt] = fgr->faces[f];
								faceRenders[fgr->new_face_cnt] &= ~FS__HIDDEN;   //!!!!!fix
								fgr->new_face_cnt++;
								
								id_cnt += 3;
								
								if (final_pts == 4)
								{
									id_list[id_cnt-1] = split[3];
									id_list[id_cnt] = split[0];
									id_list[id_cnt+1] = split[3];
									id_list[id_cnt+2] = split[2];
									id_cnt += 3;
									
									CQASSERT(split[3] < fgr->vert_cnt);
									
									fgr->faces[fgr->new_face_cnt] = fgr->faces[f];
									faceRenders[fgr->new_face_cnt] &= ~FS__HIDDEN; // !!!!fix
									fgr->new_face_cnt++;
								}
								
								
							}
							
#define TEX 0.002
							//outside polys
							for (i=old_vert_cnt2;i<vert_cnt2;i++)
							{
								v_list2[i].u = v_list2[i].pos.z*TEX;
								v_list2[i].v = v_list2[i].pos.x*TEX;
								v_list2[i].color = gridColor;
							}
							
							/*	for (i=0;i<vert_cnt1;i++)
							{
							v_list1[i].color = color;
						}*/
							
							old_vert_cnt2 = vert_cnt2;
							
							CQASSERT(final_pts1 != 1 && final_pts1 != 2);
							
							if (final_pts1)
							{
								id_list1[id_cnt1] = out1[0];
								id_list1[id_cnt1+1] = out1[1];
								id_list1[id_cnt1+2] = out1[2];
								
								id_cnt1 += 3;
								
								if (final_pts1 == 4)
								{
									//first poly was 0,1,2  make it 0,1,3
									id_list1[id_cnt1-1] = out1[3];
									id_list1[id_cnt1] = out1[0];
									id_list1[id_cnt1+1] = out1[3];
									id_list1[id_cnt1+2] = out1[2];
									
									CQASSERT(out1[2] < vert_cnt1);
									
									id_cnt1 += 3;
								}
								
								if (final_pts1 == 5)
								{
								/*	id_list1[id_cnt-3] = out1[0];
								id_list1[id_cnt1-2] = out1[4];
									id_list1[id_cnt1-1] = out1[2];*/
									id_list1[id_cnt1]   = out1[0];
									id_list1[id_cnt1+1] = out1[2];
									id_list1[id_cnt1+2] = out1[3];
									
									id_list1[id_cnt1+3] = out1[0];
									id_list1[id_cnt1+4] = out1[3];
									id_list1[id_cnt1+5] = out1[4];
									id_cnt1 += 6;
									
								}
							}
							
							if (final_pts2)
							{
								id_list2[id_cnt2] = out2[0];
								id_list2[id_cnt2+1] = out2[1];
								id_list2[id_cnt2+2] = out2[2];
								
								id_cnt2 += 3;
								
								if (final_pts2 == 4)
								{
									//first poly was 0,1,2  make it 0,1,3
									id_list2[id_cnt2-1] = out2[3];
									id_list2[id_cnt2] = out2[0];
									id_list2[id_cnt2+1] = out2[3];
									id_list2[id_cnt2+2] = out2[2];
									
									CQASSERT(out2[2] < vert_cnt2);
									
									id_cnt2 += 3;
								}
							}
							
							CQASSERT(f < fg_dork->face_cnt_rl*3);
							
							//FIX THIS!!!!!!!!!!
							faceRenders[f] |= FS__HIDDEN;
							
							if (pts1 == 3 || pts2 == 3)
							{
								faceRenders[f] |= FS__HIDDEN;
							}
							
							if (pts == 3)
							{
								faceRenders[f] &= ~FS__HIDDEN;
							}
						}
					}
				}
				//				fg_dork->SetLocal();
				fg_dork = fg_dork->next;
				//	fg_cnt++;
			}
			
			desc2.num_verts = vert_cnt2;
			desc2.num_indices = id_cnt2;
			CQBATCH->ReleasePrimBuffer(&desc2);

			desc.num_verts = vert_cnt1;
			desc.num_indices = id_cnt1;
			CQBATCH->ReleasePrimBuffer(&desc);

			
			BATCH->set_state(RPR_STATE_ID,0);

			TreeRender(&mc[c],Transform());

			/*fg_dork = md[c].fg_dork;
			while (fg_dork)
			{
				fg_dork->SetReal();
				fg_dork = fg_dork->next;
			}*/
		}
	}
}

//---------------------------------------------------------------------------
//
void SolarianBuild::AddFabricator(IFabricator *_fab)
{
	OBJPTR<IFabricator> fab;
	_fab->QueryInterface(IFabricatorID,fab);
	fabID[numFabWorking] = fab.Ptr()->GetPartID();
	++numFabWorking;

//	resetDrones();
}
//---------------------------------------------------------------------------
//
void SolarianBuild::RemoveFabricator(IFabricator *_fab)
{
	OBJPTR<IFabricator> fab;
	_fab->QueryInterface(IFabricatorID,fab);
	U32 rFab = fab.Ptr()->GetPartID();

	for(int index = 0; index < numFabWorking ; ++index)
	{
		if(fabID[index] == rFab)
		{
			--numFabWorking;
			while(index < numFabWorking)
			{
				fabID[index] = fabID[index+1];
				++index;
			}
		}
	}

	//drone stuff
//	resetDrones();

	//OBJPTR<IBuildShip> drone;
	for(unsigned int c = 0; c<fab->GetNumDrones() ; c++)
	{
		IBuildShip * obj;
		fab->GetDrone(&obj,c);
	//	obj->QueryInterface(IBuildShipID,drone);
		obj->Return();
	}
}

void SolarianBuild::SynchBuilderShips()
{
/*	for (int i=0;i<numDrones;i++)
	{
		Vector v;
		S32 vert = rand()%3;
		S32 face = droneRef[i];
		SINGLE morph=min(max(0,(smoothTimer-GROW_START_TIME)/(totalTime-GROW_END_TIME-GROW_START_TIME)),1.0);
		SINGLE mmorph = 1.0 - 13.0*morph*morph + 12.0*morph;
		v = trans*(smesh->v_list[smesh->f_list[face].v[vert]].pt*morph+pellet_verts[smesh->f_list[face].v[vert]]*mmorph);
		ISpiderDrone *drone = getNextDrone();
		drone->IdleAt(v);
	}*/

}

void SolarianBuild::Done()
{
/*	for (int i=0;i<numChildren;i++)
	{
		mc[i]->unique = 0;
	}*/
	
	if (buildObj)
	{
		buildObj->bSpecialRender = false;
		buildObj = 0;
	}

	//make ship visible
	for (int k=0;k<numChildren;k++)
	{
		if (mc[k].bHasMesh)
		{
			//make a child mesh visible
			for (S32 i=0;i<mc[k].mr->face_cnt;i++)
			{
				mc[k].faceRenders[i] &= ~FS__HIDDEN;
			}
		}
	}
}

bool SolarianBuild::EnumerateHardpoint (const HPENUMINFO & info)
{
//	beam_pos[num_beams] = info.hardpointinfo.point;
//	beam_dir[num_beams] = info.hardpointinfo.orientation.get_k();
	beam_source_pos[num_beams] = info.hardpointinfo.point;
	num_beams++;

	return num_beams < 3;
}
//------------------------------------------------------------------------------------------
//---------------------------SolarianBuild Factory----------------------------------------------
//------------------------------------------------------------------------------------------
//
struct DACOM_NO_VTABLE SolarianBuildFactory : public IObjectFactory
{
	struct OBJTYPE : SolarianBuildArchetype
	{
		void * operator new (size_t size)
		{
			return calloc(size,1);
		}
		
		void   operator delete (void *ptr)
		{
			::free(ptr);
		}
		
		OBJTYPE (void)
		{
		}

		~OBJTYPE (void)
		{
			TMANAGER->ReleaseTextureRef(gridTex);
			TMANAGER->ReleaseTextureRef(beamTex);
		}
	};
	//------------------------------------------------------------------------------------------
	//
	U32 factoryHandle;		// handles to callback

	//
	// Interface mapping
	//

	BEGIN_DACOM_MAP_INBOUND(SolarianBuildFactory)
	DACOM_INTERFACE_ENTRY(IObjectFactory)
	END_DACOM_MAP()

	SolarianBuildFactory (void) { }

	~SolarianBuildFactory (void);

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

	/* SolarianBuildFactory methods */

	IDAComponent * getBase (void)
	{
		return static_cast<IObjectFactory *> (this);
	}
};
//--------------------------------------------------------------------------//
//
SolarianBuildFactory::~SolarianBuildFactory (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (OBJLIST && OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
		connection->Unadvise(factoryHandle);
}
//--------------------------------------------------------------------------//
//
void SolarianBuildFactory::init (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
		connection->Advise(getBase(), &factoryHandle);
}
//-----------------------------------------------------------------------------
//
HANDLE SolarianBuildFactory::CreateArchetype (const char *szArchname, OBJCLASS objClass, void *_data)
{
	OBJTYPE * result = 0;

	if (objClass == OC_BUILDOBJ)
	{
		BT_SOLARIAN_BUILD * data = (BT_SOLARIAN_BUILD *) _data;
		
		if (data->boClass == BO_SOLARIAN)
		{
			result = new OBJTYPE;
			char fname[32];
			strcpy(fname,"grid.tga");
			if (fname [0])
			{
				result->gridTex = TMANAGER->CreateTextureFromFile(fname, TEXTURESDIR, DA::TGA, PF_4CC_DAA4);
			}

			strcpy(fname,"SolarianBuild.tga");
			if (fname [0])
			{
				result->beamTex = TMANAGER->CreateTextureFromFile(fname, TEXTURESDIR, DA::TGA, PF_4CC_DAA4);
			}
		}
	}

	goto Done;

//Error:
	delete result;
	result = 0;
Done:
	return (HANDLE) result;
}
//-------------------------------------------------------------------
//
BOOL32 SolarianBuildFactory::DestroyArchetype (HANDLE hArchetype)
{
	delete (OBJTYPE *)hArchetype;
	return 1;
}
//-------------------------------------------------------------------
//
IBaseObject * SolarianBuildFactory::CreateInstance (HANDLE hArchetype)
{
	OBJTYPE * objtype = (OBJTYPE *)hArchetype;
	SolarianBuild * obj = new ObjectImpl<SolarianBuild>;
	obj->arch = objtype;

	obj->objClass = OC_BUILDOBJ;

	return obj;

}
//-------------------------------------------------------------------
//
void SolarianBuildFactory::EditorCreateInstance (HANDLE hArchetype, const MISSION_INFO & info)
{
	CQBOMB0("Editor insertion not supported");
}


//---------------------------------------------------------------------------
//-------------------------GLOBAL STARTUP------------------------------------
//---------------------------------------------------------------------------
//
struct _solarianbuild : GlobalComponent
{
	SolarianBuildFactory * sfactory;

	virtual void Startup (void)
	{
		sfactory = new DAComponent<SolarianBuildFactory>;
		AddToGlobalCleanupList((IDAComponent **) &sfactory);
	}

	virtual void Initialize (void)
	{
		sfactory->init();
	}
};

static _solarianbuild __sb;

//---------------------------------------------------------------------------
//-------------------------End SolarianBuild.cpp-----------------------------
//---------------------------------------------------------------------------
