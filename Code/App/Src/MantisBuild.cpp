//--------------------------------------------------------------------------//
//                                                                          //
//                                MantisBuild.cpp                           //
//                                                                          //
//                  COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/MantisBuild.cpp 47    10/24/00 3:37p Jasony $
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
//#include "IShipMove.h"
#include "CQBatch.h"
#include "CQLight.h"

#include <DBuildObj.h>
#include <DBuildSave.h>
//temp
#include "TObject.h"

#include <renderer.h>
#include <IHardPoint.h>
#include <LightMan.h>
#include <TSmartPointer.h>
#include <IConnection.h>
#include <ITextureLibrary.h>
#include <FileSys.h>
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//

struct MantisBuildArchetype
{
	U32 cocoonTex;
};

#define DRONE_UPDATE_TIME 2
#define GROW_START_TIME 2.0f
#define GROW_END_TIME 5.3f

#define MAX_DRONE_CNT 12

struct _NO_VTABLE MantisBuild : public IBuildEffect
{
	BEGIN_MAP_INBOUND(MantisBuild)
	_INTERFACE_ENTRY(IBaseObject)
	_INTERFACE_ENTRY(IBuildEffect)
	END_MAP()

	//control variable
	SINGLE percent;

	U8 droneCnt;
	U8 droneFab;
	U8 numDrones;
	SINGLE droneMoveTime;

	//special morph stuff
	MeshInfo *mc[MAX_CHILDS];
	int num_childs;
	MeshInfo *rootChild;

	//

	MantisBuildArchetype *arch;
	
	S32 numFabWorking;
	U32 fabID[MAX_FAB_WORKING];

	OBJPTR<IBaseObject> buildObj;
	INSTANCE_INDEX instanceIndex;

	Transform trans;

//	S32 totalClicks,thisClicks;
	SINGLE smoothTimer,deployTime,deployTimer,totalTime,timePassed;
	//percent per second for visual neatness
	SINGLE rate;

	//cocoon render stuff
//	LightRGB *lit;
//	RPVertex *v_list;
	SMesh *smesh;
	Vector *pellet_verts;
	int pellet_face_cnt;

	bool *hidden;
	Vector shieldScale;
	U32 lastReveal;
	Vector start_pos,end_pos;

	//spider drone stuff
	S32 droneRef[MAX_DRONE_CNT];

//	OBJPTR<IFabricator> fabo;

	bool bCocoonMove:1;
	bool bPaused:1;
	bool bGrowsAway:1;

	MantisBuild (void) 
	{
		droneCnt = 0;
		droneFab = 0;
	}

	virtual ~MantisBuild (void);	// See ObjList.cpp

	virtual BOOL32 Update (void);

	virtual void PhysicalUpdate (SINGLE dt);

	virtual void SetupMesh (IBaseObject *fab,IBaseObject *obj,struct IMeshInfoTree *mesh_info,INSTANCE_INDEX _instanceIndex,SINGLE _totalTime);

	virtual void SetupMesh (IBaseObject *fab,struct IMeshInfoTree *mesh_info,SINGLE _totalTime);

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

	/* MantisBuild methods */

	void renderCocoon (void);

	void renderGrowth (void);

//	void renderMeshChunk();

	bool timeForDroneMove ();
	ISpiderDrone * getNextDrone ();
	void resetDrones();
};

//---------------------------------------------------------------------------
//
MantisBuild::~MantisBuild (void)
{
/*	if (fabo)
	{
		VOLPTR(IShipMove) sm=fabo;
		if (sm)
			sm->ReleaseShipControl(fabo.ptr->GetPartID());
	}*/
	delete [] pellet_verts;
	delete [] hidden;
}


//---------------------------------------------------------------------------
//
bool MantisBuild::timeForDroneMove ()
{
	return DRONE_UPDATE_TIME < droneMoveTime;
}
//---------------------------------------------------------------------------
//
ISpiderDrone * MantisBuild::getNextDrone ()
{
	OBJPTR<ISpiderDrone> result;
	droneMoveTime -= DRONE_UPDATE_TIME;
	++droneCnt;
	IBaseObject * obj = OBJLIST->FindObject(fabID[droneFab]);
	if(obj)
	{
		OBJPTR<IFabricator> fab;
		obj->QueryInterface(IFabricatorID,fab);
		if(droneCnt >= fab->GetNumDrones())
		{
			droneCnt = 0;
			droneFab = (droneFab +1)%numFabWorking;
			IBaseObject * obj = OBJLIST->FindObject(fabID[droneFab]);
			obj->QueryInterface(IFabricatorID,fab);
		}
		IBuildShip * baseObj;
		fab->GetDrone(&baseObj,droneCnt);
		if(baseObj)
		{
			baseObj->QueryInterface(ISpiderDroneID,result);
			CQASSERT(result);
		}
	}
	return result;
}
//---------------------------------------------------------------------------
//
void MantisBuild::resetDrones()
{
	droneMoveTime = DRONE_UPDATE_TIME;
	droneCnt = 0;
	droneFab = 0;
}

void MantisBuild::SetupMesh (IBaseObject *fab,IBaseObject *obj,struct IMeshInfoTree *mesh_info,INSTANCE_INDEX _instanceIndex,SINGLE _totalTime)
{
	instanceIndex = _instanceIndex;

	//default end position?
	end_pos = ENGINE->get_position(instanceIndex);

	if (obj)
	{
		obj->QueryInterface(IBaseObjectID, buildObj, NONSYSVOLATILEPTR);
		VOLPTR(IShipDamage) shipDamage=obj;
		CQASSERT(shipDamage);
		smesh = shipDamage->GetShieldMesh();
		if (obj->objClass == OC_SPACESHIP)
		{
			bGrowsAway = true;
			//VOLPTR(IExtent) extent=obj;
			//extent->
			trans=obj->GetTransform();
			SINGLE box[6];
			obj->GetObjectBox(box);
			Vector dir = -trans.get_k();
			SINGLE length = box[BBOX_MAX_Z]-box[BBOX_MIN_Z];
			start_pos = trans.translation;
			end_pos = start_pos+dir*(length/2);
		}
	}
	else
		buildObj=0;

	if (fab)
	{
//		fab->QueryInterface(IFabricatorID,fabo);

	/*	VOLPTR(IShipMove) sm=fab;
		if (sm)
		{
			sm->DestabilizeShip(fabo.ptr->GetPartID());
			ENGINE->set_angular_velocity(fab->GetObjectIndex(),Vector(0,0,0));
			ENGINE->set_velocity(fab->GetObjectIndex(),Vector(0,0,0));
		}*/
	}

	if (smesh)
	{
		bCocoonMove = true;

		CQASSERT(pellet_verts == 0 && "Call Done when done");

		pellet_verts = new Vector[smesh->v_cnt];
		hidden = new bool[smesh->f_cnt];
		pellet_face_cnt = smesh->f_cnt;
		int i;
		for (i=0;i<smesh->v_cnt;i++)
		{
			pellet_verts[i] = smesh->v_list[i].pt;
			pellet_verts[i].normalize();
			pellet_verts[i] *= 50;
		}

		if (smesh->e_list == 0)
			smesh->MakeEdges();

		for (i=0;i<numDrones;i++)
		{
			droneRef[i] = rand()%smesh->v_cnt;
		}
	}
	
	if (fab && bGrowsAway==false)  //this is awfully confusing
	{
		//danger!!
		trans = fab->GetTransform();

		char *fname = "\\mouth";
		INSTANCE_INDEX childIndex;
		HardpointInfo hardpointinfo;

		if (FindHardpoint(fname, childIndex, hardpointinfo, fab->GetObjectIndex()))
			start_pos = trans*hardpointinfo.point;
		else
			start_pos = trans.translation;
	}

	totalTime = _totalTime;
	deployTime = GROW_START_TIME;


	VOLPTR(IPlatform) plat=obj;
	if (plat)
	{
		rootChild = plat->GetRoots();
	}
}

//---------------------------------------------------------------------------
//  FOR SHIPS
//
void MantisBuild::SetupMesh (IBaseObject *fab,IBaseObject *obj,SINGLE _totalTime)
{
	

}
//---------------------------------------------------------------------------
//  FOR MORPHING
//
void MantisBuild::SetupMesh (IBaseObject *fab,struct IMeshInfoTree *mesh_info,SINGLE _totalTime)
{
	OBJPTR<IFabricator> fabo;
	fab->QueryInterface(IFabricatorID,fabo);

	num_childs = mesh_info->ListChildren(mc);

	int vert_cnt=0;
	pellet_face_cnt=0;
	int c;
	for (c=0;c<num_childs;c++)
	{
		if (mc[c]->bHasMesh)
		{
			vert_cnt += mc[c]->mr->pos_cnt;
			pellet_face_cnt += mc[c]->mr->face_cnt;
		}
	}

	CQASSERT(pellet_verts == 0 && "Call Done when done");
	pellet_verts = new Vector[vert_cnt];
	hidden = new bool[pellet_face_cnt];
	int i=0;
	for (c=0;c<num_childs;c++)
	{
		if (mc[c]->bHasMesh)
		{
			for (int v=0;v<mc[c]->mr->pos_cnt;v++)
			{
				pellet_verts[i] = mc[c]->mr->pos_list[v];
				pellet_verts[i].normalize();
				pellet_verts[i] *= 50;
			
				i++;
			}
		}
	}

//	if (smesh->e_list == 0)
//		smesh->MakeEdges();

	if(fabo)
		numDrones = fabo->GetNumDrones();
	else
		numDrones = 0;
	CQASSERT(numDrones <= MAX_DRONE_CNT);
	
	for (i=0;i<numDrones;i++)
	{
		droneRef[i] = rand()%vert_cnt;
	}
	
	//danger!!
	Transform trans = fab->GetTransform();

	totalTime = _totalTime;
	deployTime = GROW_START_TIME;

}
//---------------------------------------------------------------------------
//
void MantisBuild::SetBuildPercent (SINGLE newPercent)
{
	bPaused = false;
	CQASSERT(newPercent >= 0.0f);
	if (newPercent > 1.0f)
	{
		CQBOMB0("Build at over 100% done - ignorable");
		newPercent = 1.0f;
	}
	timePassed = newPercent*totalTime;
	CQASSERT(timePassed <= totalTime);
	if (newPercent != percent)
		smoothTimer = timePassed;
	percent = newPercent;

	if (rate < 0)
	{
		deployTimer = -newPercent/rate;
	}
	//thisClicks = percent*totalClicks;
}
//--------------------------------------------------------------------------//
//
void MantisBuild::SetBuildRate (SINGLE percentPerSecond)
{
	bPaused = false;
	rate = percentPerSecond;
	if (rate > 0)
	{
		memset(hidden,0,sizeof(bool)*pellet_face_cnt);
		if (buildObj)
			buildObj->bSpecialRender = true;
	}
	else if (rate < 0)
	{
		if (lastReveal == 0)
		{
			memset(hidden,0xff,sizeof(bool)*pellet_face_cnt);
			lastReveal = pellet_face_cnt;
		}
		deployTimer = -1.0f/rate;
		
	}
}
//--------------------------------------------------------------------------//
//
BOOL32 MantisBuild::Update (void)
{
	droneMoveTime += ELAPSED_TIME;
		
	return 1;
}
//--------------------------------------------------------------------------//
//
void MantisBuild::PhysicalUpdate (SINGLE dt)
{
	if (bPaused)
		return;

	smoothTimer += dt*rate*totalTime;
	if (smoothTimer > totalTime)
		smoothTimer = totalTime;
	if (smoothTimer < 0.0f)
		smoothTimer = 0.0f;
	if (bCocoonMove)
	{
		if (rate > 0)
			deployTimer += dt;
		else if (rate < 0)
			deployTimer -= dt;

		if (deployTimer < deployTime)
		{
			SINGLE ratio = deployTimer/deployTime;
			
			ENGINE->set_position(instanceIndex,ratio*end_pos+(1-ratio)*start_pos);
		}
		else
			ENGINE->set_position(instanceIndex,end_pos);
	}

	if (bGrowsAway)
	{
		SINGLE ratio = smoothTimer/totalTime;
		ENGINE->set_position(instanceIndex,ratio*end_pos+(1-ratio)*start_pos);
	}
	
	CQASSERT(timePassed <= totalTime);
	if (totalTime-smoothTimer < GROW_END_TIME || (rate < 0 && lastReveal != 0))  //check if we really have unrevealed all the faces
	{
		SINGLE ratio;
		ratio = (GROW_END_TIME-(totalTime-smoothTimer))/GROW_END_TIME;
		
		U32 reveal = max(0,pellet_face_cnt*ratio);
		
		if (rate > 0)
		{
			for (U32 f=lastReveal;f<reveal;f++)
			{
				hidden[f] = true;
			}
			
			lastReveal = reveal;
		}
		else
		{
			for (U32 f=reveal;f<lastReveal;f++)
			{
				hidden[f] = false;
			}
			
			lastReveal = reveal;
		}
	}
}
//---------------------------------------------------------------------------
//
void MantisBuild::Render (void)
{
		trans = ENGINE->get_transform(instanceIndex);
	if (smesh)
	{
	/*	thisClicks = (percent+rate*smoothTimer)*totalClicks;
		if (thisClicks > totalClicks)
			thisClicks = totalClicks;*/
		LIGHTS->ActivateBestLights(trans.translation,8,4000);
		renderCocoon();

		if (num_childs == 0 && buildObj)
		{
			if (totalTime-timePassed < GROW_END_TIME)
				buildObj->bSpecialRender = false;
			else
				buildObj->bSpecialRender = true;
		}
		
		if (rootChild)
		{
			SINGLE scale=min(max(0,(smoothTimer-GROW_START_TIME)/(totalTime-GROW_END_TIME-GROW_START_TIME)),1.0f);
			TRANSFORM scaleTrans;
			scaleTrans.d[0][0] = min(scale*1.5f,1.0f);
			scaleTrans.d[1][1] = min(scale*1.5f,1.0f);
			scaleTrans.d[2][2] = scale;
			
			//TRANSFORM trans = transform*scaleTrans;
			TreeRender(&rootChild,1,scaleTrans);
		}
	}
	else
	{
	//	trans = buildObj->GetTransform();
		renderGrowth();

		/*if (totalTime-timePassed < GROW_END_TIME)
			buildObj->bSpecialRender = false;
		else
			buildObj->bSpecialRender = true;*/
	}

	if (num_childs && totalTime-timePassed < GROW_END_TIME)
	{
		//ENGINE->set_transform(mc[0]->instanceIndex,trans);
		TreeRender(mc,num_childs);
	}

/*	if (1)//bVisible)
	{
		BATCH->set_state(RPR_BATCH,1);
		BATCH->set_render_state(D3DRS_ALPHABLENDENABLE,TRUE);
		BATCH->set_render_state(D3DRS_ZENABLE,TRUE);
		//BATCH->set_render_state(D3DRS_ZWRITEENABLE,TRUE);
		BATCH->set_render_s6tate(D3DRS_SRCBLEND,D3DBLEND_SRCALPHA);
		BATCH->set_render_state(D3DRS_DESTBLEND,D3DBLEND_INVSRCALPHA);
		//BATCH->set_render_state(D3DRS_ZFUNC,D3DCMP_LESSEQUAL);
		renderMeshChunk();//buildTex,FS_BUILDING);
	}*/
}
//---------------------------------------------------------------------------
//
void MantisBuild::renderCocoon()
{
	Vector v[3];
	SINGLE morph=min(max(0,(smoothTimer-GROW_START_TIME)/(totalTime-GROW_END_TIME-GROW_START_TIME)),1.0f);
	SINGLE mmorph;
//	if (morph < 0.5)
//		mmorph = 1.0+8*morph;
//	else
//		mmorph = 10.0-10*morph;

	mmorph = 1.0f - 13.0f*morph*morph + 12.0f*morph;
	Transform inv;
	
/*	TRANSFORM scaleTrans;
	scaleTrans.d[0][0] = shieldScale.x;//SHIELD_SCALE;
	scaleTrans.d[1][1] = shieldScale.y;//SHIELD_SCALE;
	scaleTrans.d[2][2] = shieldScale.z;//SHIELD_SCALE;

	DisableTextures();

	TRANSFORM trans = transform*scaleTrans;*/
	CAMERA->SetModelView(&trans);

	BATCH->set_render_state(D3DRS_ALPHABLENDENABLE,TRUE);
	BATCH->set_render_state(D3DRS_ZENABLE,TRUE);
	BATCH->set_render_state(D3DRS_ZWRITEENABLE,TRUE);
	BATCH->set_render_state(D3DRS_SRCBLEND,D3DBLEND_SRCALPHA);
	BATCH->set_render_state(D3DRS_DESTBLEND,D3DBLEND_INVSRCALPHA);

	LightRGB lit[3];
	Vector n[3];

	SetupDiffuseBlend(arch->cocoonTex,FALSE);
	BATCH->set_state(RPR_STATE_ID,arch->cocoonTex);
	inv = trans.get_inverse();

	PB.Color3ub(255,255,255);
	PB.Begin(PB_TRIANGLES,smesh->f_cnt*3);
	for (int f=0;f<smesh->f_cnt;f++)
	{
		if (hidden[f] == 0)
		{
			v[0] = smesh->v_list[smesh->f_list[f].v[0]].pt*morph+pellet_verts[smesh->f_list[f].v[0]]*mmorph;
			v[1] = smesh->v_list[smesh->f_list[f].v[1]].pt*morph+pellet_verts[smesh->f_list[f].v[1]]*mmorph;
			v[2] = smesh->v_list[smesh->f_list[f].v[2]].pt*morph+pellet_verts[smesh->f_list[f].v[2]]*mmorph;
			
			//0.02 is the inverse of 50 referenced elsewhere in the code
			n[0] = smesh->v_list[smesh->f_list[f].v[0]].n*morph+pellet_verts[smesh->f_list[f].v[0]]*mmorph*0.02;
			n[1] = smesh->v_list[smesh->f_list[f].v[1]].n*morph+pellet_verts[smesh->f_list[f].v[1]]*mmorph*0.02;
			n[2] = smesh->v_list[smesh->f_list[f].v[2]].n*morph+pellet_verts[smesh->f_list[f].v[2]]*mmorph*0.02;
			
			n[0].fast_normalize();
			n[1].fast_normalize();
			n[2].fast_normalize();
			
			LIGHT->light_vertices(lit,v,n,3,&inv);
#define TEX 0.0017
			PB.Color3ub(lit[0].r,lit[0].g,lit[0].b);
			PB.TexCoord2f(v[0].x*TEX,v[0].z*TEX);
			PB.Vertex3f_NC(v[0].x,v[0].y,v[0].z);
			PB.Color3ub(lit[1].r,lit[1].g,lit[1].b);
			PB.TexCoord2f(v[1].x*TEX,v[1].z*TEX);
			PB.Vertex3f_NC(v[1].x,v[1].y,v[1].z);
			PB.Color3ub(lit[2].r,lit[2].g,lit[2].b);
			PB.TexCoord2f(v[2].x*TEX,v[2].z*TEX);
			PB.Vertex3f_NC(v[2].x,v[2].y,v[2].z);
		}
	}
	PB.End();
	BATCH->set_state(RPR_STATE_ID,0);

	if (smesh)
	{
		//???????
		ISpiderDrone *drone = getNextDrone();
		
		if (drone)
		{
			S32 face = smesh->GetNeighborOnEdge(droneRef[droneCnt],rand()%3);
			S32 vert = rand()%3;
			v[0] = trans*(smesh->v_list[smesh->f_list[face].v[vert]].pt*morph+pellet_verts[smesh->f_list[face].v[vert]]*mmorph);
			n[0] = trans.rotate(smesh->v_list[smesh->f_list[face].v[vert]].n*morph+pellet_verts[smesh->f_list[face].v[vert]]*mmorph*0.02f);
		
			if (rand()%30 == 0)
				drone->SpinTo(v[0]+50*n[0]);
			else if (rand()%3 == 0)
				drone->ScuttleTo(v[0]+50*n[0]);

			droneRef[droneCnt] = face;
		}
	}
}
//---------------------------------------------------------------------------
//
void MantisBuild::renderGrowth()
{
	BATCH->set_state(RPR_DELAY,1);
	//create a render mesh with texture function

	Vector v[3];
	SINGLE morph=min(max(0,(smoothTimer)/(totalTime-GROW_END_TIME)),1.0f);
	SINGLE mmorph;
//	if (morph < 0.5)
//		mmorph = 1.0+8*morph;
//	else
//		mmorph = 10.0-10*morph;

	mmorph = 1.0f - 13.0f*morph*morph + 12.0f*morph;
	Transform inv;
	
/*	TRANSFORM scaleTrans;
	scaleTrans.d[0][0] = shieldScale.x;//SHIELD_SCALE;
	scaleTrans.d[1][1] = shieldScale.y;//SHIELD_SCALE;
	scaleTrans.d[2][2] = shieldScale.z;//SHIELD_SCALE;

	DisableTextures();

	TRANSFORM trans = transform*scaleTrans;*/
	CAMERA->SetModelView(&trans);

	BATCH->set_render_state(D3DRS_ZFUNC,D3DCMP_LESSEQUAL);
	BATCH->set_render_state(D3DRS_ALPHABLENDENABLE,FALSE);
	BATCH->set_render_state(D3DRS_ZENABLE,TRUE);
	BATCH->set_render_state(D3DRS_ZWRITEENABLE,TRUE);
	BATCH->set_render_state(D3DRS_SRCBLEND,D3DBLEND_SRCALPHA);
	BATCH->set_render_state(D3DRS_DESTBLEND,D3DBLEND_INVSRCALPHA);

	LightRGB lit[3];
	Vector n[3];

	SetupDiffuseBlend(arch->cocoonTex,FALSE);
	BATCH->set_state(RPR_STATE_ID,arch->cocoonTex+1);

	int f=0;
	int base_v;
	for (int c=0;c<num_childs;c++)
	{
		base_v=0;
		if (mc[c]->bHasMesh)
		{
			Vector *pos_list = mc[c]->mr->pos_list;

			trans = ENGINE->get_transform(mc[c]->instanceIndex);
			CAMERA->SetModelView(&trans);
			
			inv = trans.get_inverse();
			PB.Begin(PB_TRIANGLES,mc[c]->mr->face_cnt*3);
			
			IRenderMaterial *irm=0;
			
			while((irm = mc[c]->mr->GetNextFaceGroup(irm)) != 0)
			{
				U16 *index_list = irm->index_list;
				int i=0;
				while (i<irm->new_face_cnt)
				{
					if (hidden[f] == 0)
					{
						Vector v[3];
						n[0] = v[0]=pos_list[base_v+index_list[i*3]];
						n[1] = v[1]=pos_list[base_v+index_list[i*3+1]];
						n[2] = v[2]=pos_list[base_v+index_list[i*3+2]];

						n[0].fast_normalize();
						n[1].fast_normalize();
						n[2].fast_normalize();
						
						v[0] = v[0]*morph+pellet_verts[base_v+index_list[i*3]]*mmorph;
						v[1] = v[1]*morph+pellet_verts[base_v+index_list[i*3+1]]*mmorph;
						v[2] = v[2]*morph+pellet_verts[base_v+index_list[i*3+2]]*mmorph;
						
						
						LIGHT->light_vertices(lit,v,n,3,&inv);
#define TEX 0.0017
						PB.Color3ub(lit[0].r,lit[0].g,lit[0].b);
						PB.TexCoord2f(v[0].x*TEX,v[0].z*TEX);
						PB.Vertex3f_NC(v[0].x,v[0].y,v[0].z);
						PB.Color3ub(lit[1].r,lit[1].g,lit[1].b);
						PB.TexCoord2f(v[1].x*TEX,v[1].z*TEX);
						PB.Vertex3f_NC(v[1].x,v[1].y,v[1].z);
						PB.Color3ub(lit[2].r,lit[2].g,lit[2].b);
						PB.TexCoord2f(v[2].x*TEX,v[2].z*TEX);
						PB.Vertex3f_NC(v[2].x,v[2].y,v[2].z);
						
					}
					f++;
					i++;
				}
				base_v += irm->vert_cnt;  //this will change if I ever stop making pos_list parallel
			}
			PB.End();  //PB_TRIANGLES

			//base_v will change here instead
		}
	}
	BATCH->set_state(RPR_STATE_ID,0);
	BATCH->set_state(RPR_DELAY,0);
#undef TEX

	if (smesh)
	{
		//???????
		ISpiderDrone *drone = getNextDrone();
		
		if (drone)
		{
			S32 face = smesh->GetNeighborOnEdge(droneRef[droneCnt],rand()%3);
			S32 vert = rand()%3;
			v[0] = trans*(smesh->v_list[smesh->f_list[face].v[vert]].pt*morph+pellet_verts[smesh->f_list[face].v[vert]]*mmorph);
			n[0] = trans.rotate(smesh->v_list[smesh->f_list[face].v[vert]].n*morph+pellet_verts[smesh->f_list[face].v[vert]]*mmorph*0.02f);
		
			if (rand()%30 == 0)
				drone->SpinTo(v[0]+50*n[0]);
			else if (rand()%3 == 0)
				drone->ScuttleTo(v[0]+50*n[0]);

			droneRef[droneCnt] = face;
		}
	}
}


void MantisBuild::AddFabricator(IFabricator *_fab)
{
	OBJPTR<IFabricator> fab;
	_fab->QueryInterface(IFabricatorID,fab);
	fabID[numFabWorking] = fab.Ptr()->GetPartID();
	++numFabWorking;
	
	numDrones += fab->GetNumDrones();

	resetDrones();


}

void MantisBuild::RemoveFabricator(IFabricator *_fab)
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

	numDrones -= fab->GetNumDrones();
	//drone stuff
	resetDrones();

	//OBJPTR<IBuildShip> drone;
	for(unsigned int c = 0; c<fab->GetNumDrones() ; c++)
	{
		IBuildShip * obj;
		fab->GetDrone(&obj,c);
	//	obj->QueryInterface(IBuildShipID,drone);
		obj->Return();
	}
}

void MantisBuild::SynchBuilderShips()
{
	if (smesh)
	{
		for (int i=0;i<numDrones;i++)
		{
			Vector v;
			S32 vert = rand()%3;
			S32 face = droneRef[i];
			SINGLE morph=min(max(0,(smoothTimer-GROW_START_TIME)/(totalTime-GROW_END_TIME-GROW_START_TIME)),1.0f);
			SINGLE mmorph = 1.0f - 13.0f*morph*morph + 12.0f*morph;
			v = trans*(smesh->v_list[smesh->f_list[face].v[vert]].pt*morph+pellet_verts[smesh->f_list[face].v[vert]]*mmorph);
			ISpiderDrone *drone = getNextDrone();
			drone->IdleAt(v);
		}
	}
}

void MantisBuild::Done()
{
	/*if (fabo)
	{
		VOLPTR(IShipMove) sm=fabo;
		if (sm)
			sm->ReleaseShipControl(fabo.ptr->GetPartID());
	}*/
	if (bCocoonMove)
	{
	  ENGINE->set_position(instanceIndex,end_pos);
	  ENGINE->update_instance(instanceIndex,0,0);
	}
	if (buildObj)
	{
		buildObj->bSpecialRender = false;
	}
	delete [] pellet_verts;
	delete [] hidden;
	pellet_verts = 0;
	hidden = 0;
}
//------------------------------------------------------------------------------------------
//---------------------------MantisBuild Factory----------------------------------------------
//------------------------------------------------------------------------------------------
//
struct DACOM_NO_VTABLE MantisBuildFactory : public IObjectFactory
{
	struct OBJTYPE : MantisBuildArchetype
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
			TMANAGER->ReleaseTextureRef(cocoonTex);
		}
	};
	//------------------------------------------------------------------------------------------
	//
	U32 factoryHandle;		// handles to callback

	//
	// Interface mapping
	//

	BEGIN_DACOM_MAP_INBOUND(MantisBuildFactory)
	DACOM_INTERFACE_ENTRY(IObjectFactory)
	END_DACOM_MAP()

	MantisBuildFactory (void) { }

	~MantisBuildFactory (void);

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

	/* MantisBuildFactory methods */

	IDAComponent * getBase (void)
	{
		return static_cast<IObjectFactory *> (this);
	}
};
//--------------------------------------------------------------------------//
//
MantisBuildFactory::~MantisBuildFactory (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (OBJLIST && OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
		connection->Unadvise(factoryHandle);
}
//--------------------------------------------------------------------------//
//
void MantisBuildFactory::init (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
		connection->Advise(getBase(), &factoryHandle);
}
//-----------------------------------------------------------------------------
//
HANDLE MantisBuildFactory::CreateArchetype (const char *szArchname, OBJCLASS objClass, void *_data)
{
	OBJTYPE * result = 0;

	if (objClass == OC_BUILDOBJ)
	{
		BT_MANTIS_BUILD * data = (BT_MANTIS_BUILD *) _data;
		
		if (data->boClass == BO_MANTIS)
		{
			result = new OBJTYPE;
			char * fname = data->cocoonTextureName;
			if (fname [0])
			{
				result->cocoonTex = TMANAGER->CreateTextureFromFile(fname, TEXTURESDIR, DA::TGA, PF_4CC_DAA4);
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
BOOL32 MantisBuildFactory::DestroyArchetype (HANDLE hArchetype)
{
	delete (OBJTYPE *)hArchetype;
	return 1;
}
//-------------------------------------------------------------------
//
IBaseObject * MantisBuildFactory::CreateInstance (HANDLE hArchetype)
{
	OBJTYPE * objtype = (OBJTYPE *)hArchetype;
	MantisBuild * obj = new ObjectImpl<MantisBuild>;
	obj->arch = objtype;

	obj->objClass = OC_BUILDOBJ;

	return obj;

}
//-------------------------------------------------------------------
//
void MantisBuildFactory::EditorCreateInstance (HANDLE hArchetype, const MISSION_INFO & info)
{
	CQBOMB0("Editor insertion not supported");
}
//---------------------------------------------------------------------------
//-------------------------GLOBAL STARTUP------------------------------------
//---------------------------------------------------------------------------
//
struct _mantisbuild : GlobalComponent
{
	MantisBuildFactory * sfactory;

	virtual void Startup (void)
	{
		sfactory = new DAComponent<MantisBuildFactory>;
		AddToGlobalCleanupList((IDAComponent **) &sfactory);
	}

	virtual void Initialize (void)
	{
		sfactory->init();
	}
};

static _mantisbuild __mb;

//---------------------------------------------------------------------------
//-------------------------End MantisBuild.cpp-------------------------------
//---------------------------------------------------------------------------
