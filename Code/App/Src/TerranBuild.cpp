//--------------------------------------------------------------------------//
//                                                                          //
//                                TerranBuild.cpp                              //
//                                                                          //
//                  COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/TerranBuild.cpp 45    10/04/00 8:35p Jasony $
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
#include "IBlast.h"
#include "TManager.h"
#include "MeshRender.h"

#include <DBuildObj.h>
#include <DBuildSave.h>
//temp
#include "TObject.h"

#include <TSmartPointer.h>
#include <IConnection.h>
//#include <ITextureLibrary.h>
#include <FileSys.h>
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//

struct TerranBuildArchetype
{
	U32 buildTex;
	PARCHETYPE pWeldType;
};


#define END_TIME 60
#define ONE_FACE 8
#define NEW_CHILD 26
#define START_TIME 64-NEW_CHILD
#define MAX_GIRDER_FACES 20
#define DRONE_UPDATE_TIME 2

struct FaceLookup
{
	struct IRenderMaterial *group;
	U16 index;
	U16 pos_offset; //hope this goes away
	U16 abs_index;
};

//------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------


struct GirderRender : IRenderChannel
{
	U32 girderTexID;

	GirderRender();

	~GirderRender();

	virtual void Render(SINGLE dt);

	virtual IRenderChannel *Clone();
};

GirderRender::GirderRender()
{
}

GirderRender::~GirderRender()
{}

void GirderRender::Render(SINGLE dt)
{
	BATCH->set_state(RPR_BATCH,TRUE);
	const Transform &trans = ENGINE->get_transform(ec->mi->instanceIndex);
	CAMERA->SetModelView(&trans);
	
	U32 color = 0xffffffff;
	ec->RenderWithTexture(girderTexID,color,false); //false for wrap
}

IRenderChannel * GirderRender::Clone()
{
	GirderRender *new_dr = new GirderRender;
	new_dr->girderTexID = girderTexID;

	return new_dr;
}
//------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------


struct _NO_VTABLE TerranBuild : public IBuildEffect
{
	BEGIN_MAP_INBOUND(TerranBuild)
	_INTERFACE_ENTRY(IBaseObject)
	_INTERFACE_ENTRY(IBuildEffect)
	END_MAP()

	//control variable
	SINGLE percent;

	//mesh variables
	S32 num_childs;
	S16 trigger;
	bool bPaused:1;
	SINGLE buildTimer;
	//only for visuals
	SINGLE rate;//,perFrame;
	S32 totalClicks,thisClicks,lastClicks;
	S32 *girderBuildAhead;

	Vector lastPos;

	SINGLE droneMoveTime;

	S32 nextFace;
	S32 lastChild,thisChild;

	MeshInfo *mc[MAX_CHILDS];
	FaceLookup *faceLookup[MAX_CHILDS];

	S32 *childTimes;
//	U32 systemID;

	TerranBuildArchetype *arch;
	
	U32 frames;

	S32 numFabWorking;
	U32 fabID[MAX_FAB_WORKING];

	U8 droneCnt,droneFab,numDrones;


	OBJPTR<IBaseObject> fabObj;
	OBJPTR<IBaseObject> buildObj;

	Transform trans;

	struct GirderRender *gr[MAX_CHILDS];

	TerranBuild (void) 
	{
		droneCnt = 0;
		droneFab = 0;
	}

	virtual ~TerranBuild (void);	// See ObjList.cpp

//	virtual void TestVisible (const USER_DEFAULTS & defaults, const U32 currentSystem, const U32 currentPlayer);	// set bVisible if possible for any part of object to appear

//	virtual SINGLE TestHighlight (const RECT & rect);

	virtual BOOL32 Update (void);

	virtual void SetupMesh (IBaseObject *fab,IBaseObject *obj,struct IMeshInfoTree *mesh_info,INSTANCE_INDEX _instanceIndex,SINGLE _totalTime);

	virtual void SetupMesh (IBaseObject *fab,IMeshInfoTree *mesh_info,SINGLE _totalTime);

	virtual void SetupMesh (IBaseObject *fab,IBaseObject *obj,SINGLE _totalTime)
	{
		CQBOMB0("Shield style build not supported");
	}

	void ReBuild ();

	virtual void SetBuildPercent (SINGLE newPercent);

	virtual void SetBuildRate (SINGLE percentPerSecond)
	{
		bPaused = false;
		rate = percentPerSecond;
	}

	virtual void AddFabricator (IFabricator *_fab);

	virtual void RemoveFabricator(IFabricator *_fab);

	virtual void SynchBuilderShips ();

	virtual void Done ();

//	virtual void Render (void);

	virtual void PhysicalUpdate(SINGLE dt);

	virtual void PrepareForExplode (void);

	virtual void PauseBuildEffect (bool bPause)
	{
		bPaused = bPause;
	}

	/* TerranBuild methods */

	void setupLookup();

	void setupRenderChannel();

	void composeGirders();

	void sendDrones(U32 child,U32 faceID);
	bool timeForDroneMove ();
	ITerranDrone * getNextDrone ();
	void resetDrones();
};

//---------------------------------------------------------------------------
//
TerranBuild::~TerranBuild (void)
{
	if (childTimes)
	{
		CQASSERT(girderBuildAhead);
		delete [] childTimes;
		delete [] girderBuildAhead;
		childTimes = 0;
		girderBuildAhead = 0;

		for (int i=0;i<MAX_CHILDS;i++)
			delete [] faceLookup[i];

		//nobody deletes these but ec
	//	delete gr;
	}
}

void TerranBuild::setupLookup()
{
	//create lookup table and hide mesh
	for (int k=0;k<num_childs;k++)
	{
		if (mc[k]->bHasMesh)
		{
			IRenderMaterial *irm=0;
			
			faceLookup[k] = new FaceLookup[mc[k]->mr->face_cnt];

			int pos_offset = 0;
			int f=0;
			while((irm = mc[k]->mr->GetNextFaceGroup(irm)) != 0)
			{
				U8 * faceRenders = &mc[k]->faceRenders[irm->face_offset];
				int a=0;
				while (a<irm->new_face_cnt)
				{
					faceLookup[k][f].group = irm;
					faceLookup[k][f].index = a;
					faceLookup[k][f].pos_offset = pos_offset;
					faceLookup[k][f].abs_index = f;
					faceRenders[a] |= FS__HIDDEN;
					a++;
					f++;
				}
				pos_offset += irm->vert_cnt;
			}

			int i;
			//sort lookup table
			for (i=0;i<mc[k]->mr->face_cnt-1;i++)
			{
				int j;
				FaceLookup &facei = faceLookup[k][i];
				for (j=i+1;j<mc[k]->mr->face_cnt;j++)
				{
					FaceLookup &facej = faceLookup[k][j];
					if (mc[k]->mr->pos_list[facei.pos_offset+facei.group->index_list[facei.index*3]].z >
						mc[k]->mr->pos_list[facej.pos_offset+facej.group->index_list[facej.index*3]].z)
					{
						FaceLookup temp = facei;
						facei = facej;
						facej = temp;
					}
				}
			}
		}
	}
}

void TerranBuild::setupRenderChannel()
{
	for (int c=0;c<num_childs;c++)
	{
		IEffectChannel *ec = mc[c]->GetNewEffectChannel();
		ec->idx_list = new U16[MAX_GIRDER_FACES*3];
		ec->idx_cnt = 0;//MAX_GIRDER_FACES*3;
		ec->tc = new TexCoord[MAX_GIRDER_FACES*3];
		ec->tc_cnt = 0;//tc_cnt;
		ec->tc_idx_list = new U16[MAX_GIRDER_FACES*3];
		ec->vert_cnt = 0;//vert_cnt;
		ec->face_ref_list = new U16[MAX_GIRDER_FACES+1];
		//make sure the "last" face has a really huge index for splitting
		gr[c] = new GirderRender;
		ec->irc = gr[c];
		gr[c]->girderTexID = arch->buildTex;
		gr[c]->ec = ec;
	}
}

void TerranBuild::SetupMesh (IBaseObject *fab,IBaseObject *obj,struct IMeshInfoTree *mesh_info,INSTANCE_INDEX _instanceIndex,SINGLE _totalTime)
{
	if(fab)
		fab->QueryInterface(IBaseObjectID,fabObj,NONSYSVOLATILEPTR);
	else
		fabObj = NULL;

	if(obj)
		obj->QueryInterface(IBaseObjectID,buildObj,NONSYSVOLATILEPTR);
	else
		buildObj = NULL;

	nextFace = 0;

	trigger = START_TIME;//32;
	
	
	lastChild = 0;
	
	totalClicks=START_TIME;

	num_childs = mesh_info->ListChildren(mc);
	girderBuildAhead = new S32[num_childs];

	CQASSERT( childTimes == 0);
	childTimes = new S32[num_childs];
	for (int c=0;c<num_childs;c++)
	{		
		childTimes[c] = totalClicks;
		if (mc[c]->bHasMesh)
		{
			mc[c]->bWhole = FALSE;
			//this depends on 100% LOD here
			girderBuildAhead[c] = min(MAX_GIRDER_FACES,mc[c]->mr->face_cnt);
			//every mesh now has the "new child" time associated with it
			totalClicks += NEW_CHILD;
			totalClicks += (girderBuildAhead[c]+mc[c]->mr->face_cnt)*ONE_FACE;
		}
		else
		{
		//	childTimes[c] = 0;
			girderBuildAhead[c] = 0;
		}
	}

	setupLookup();
	setupRenderChannel();
}

//---------------------------------------------------------------------------
//
void TerranBuild::SetupMesh (IBaseObject *fab,IMeshInfoTree *mesh_info,SINGLE _totalTime)//,IFabricator *_fab)
{
	if(fab)
		fab->QueryInterface(IBaseObjectID,fabObj,NONSYSVOLATILEPTR);
	else
		fabObj = NULL;

	nextFace = 0;

	trigger = START_TIME;//32;
	
	
	lastChild = 0;
	
	totalClicks=START_TIME;

	num_childs = mesh_info->ListChildren(mc);
	girderBuildAhead = new S32[num_childs];

	CQASSERT( childTimes == 0);
	childTimes = new S32[num_childs];
	for (int c=0;c<num_childs;c++)
	{		
		childTimes[c] = totalClicks;
		if (mc[c]->bHasMesh)
		{
			mc[c]->bWhole = FALSE;
			//this depends on 100% LOD here
			girderBuildAhead[c] = min(MAX_GIRDER_FACES,mc[c]->mr->face_cnt);
			//every mesh now has the "new child" time associated with it
			totalClicks += NEW_CHILD;
			totalClicks += (girderBuildAhead[c]+mc[c]->mr->face_cnt)*ONE_FACE;
		}
		else
		{
		//	childTimes[c] = 0;
			girderBuildAhead[c] = 0;
		}
	}

	setupLookup();
	setupRenderChannel();
}

//---------------------------------------------------------------------------
//
void TerranBuild::ReBuild ()
{
	thisClicks = percent*totalClicks;

	if (thisClicks > totalClicks)
		return;

	S32 c;

	if (thisClicks != lastClicks)
	{
		bool bForward = (thisClicks > lastClicks);
		
		c=0;
		while (c != num_childs-1 && thisClicks >= childTimes[c+1])
		{
			c++;
		}

		if (bForward)
		{
			while (thisChild != c)
			{
				//make a child mesh visible
				for (S32 i=0;i<mc[thisChild]->mr->face_cnt;i++)
				{
					FaceLookup &face = faceLookup[thisChild][i];
					mc[thisChild]->faceRenders[face.abs_index] &= ~FS__HIDDEN;
					mc[thisChild]->faceRenders[face.abs_index] &= ~FS__BUILDING;
				}
				thisChild++;
			}
			
			if (thisClicks < childTimes[c] + NEW_CHILD)
			{
				// in new child pause
				sendDrones(c,0);
			}
			else
			{
				//in actual faces or girders
				S32 nextFace = (thisClicks-childTimes[c]-NEW_CHILD-girderBuildAhead[c]*ONE_FACE)/ONE_FACE;
				CQASSERT(nextFace <= mc[c]->mr->face_cnt);
				S32 invisibleFace = min(nextFace+girderBuildAhead[c],mc[c]->mr->face_cnt);
				S32 i;
				for (i=0;i<nextFace;i++)
				{
					FaceLookup &face = faceLookup[c][i];
					mc[c]->faceRenders[face.abs_index] &= ~FS__HIDDEN;
					mc[c]->faceRenders[face.abs_index] &= ~FS__BUILDING;
				}
				for (i=max(nextFace,0);i<invisibleFace;i++)
				{
					FaceLookup &face = faceLookup[c][i];
					mc[c]->faceRenders[face.abs_index] &= ~FS__HIDDEN;
					mc[c]->faceRenders[face.abs_index] |= FS__BUILDING;
				}

				if(timeForDroneMove())
					sendDrones(c,max(0,invisibleFace-1));
			}
		}
		else
		{
			while (thisChild != c)
			{
				//make a child mesh invisible
				for (S32 i=0;i<mc[thisChild]->mr->face_cnt;i++)
				{
					FaceLookup &face = faceLookup[thisChild][i];
					mc[thisChild]->faceRenders[face.abs_index] |= FS__HIDDEN;
					mc[thisChild]->faceRenders[face.abs_index] &= ~FS__BUILDING;
				}
				thisChild--;
			}

			if (thisClicks < childTimes[c] + NEW_CHILD)
			{
				// in new child pause

				for (S32 i=0;i<mc[thisChild]->mr->face_cnt;i++)
				{
					FaceLookup &face = faceLookup[thisChild][i];
					mc[thisChild]->faceRenders[face.abs_index] |= FS__HIDDEN;
					mc[thisChild]->faceRenders[face.abs_index] &= ~FS__BUILDING;
				}
				sendDrones(thisChild,0);
			}
			else
			{
				//in actual faces or girder faces
				S32 nextFace = (thisClicks-childTimes[c]-NEW_CHILD-girderBuildAhead[c]*ONE_FACE)/ONE_FACE;
				
				//find first still invisible face
				S32 invisibleFace = min(nextFace+girderBuildAhead[c],mc[c]->mr->face_cnt);
				int i;
				for (i=max(0,nextFace);i<invisibleFace;i++)
				{
					FaceLookup &face = faceLookup[c][i];
					mc[c]->faceRenders[face.abs_index] &= ~FS__HIDDEN;
					mc[c]->faceRenders[face.abs_index] |= FS__BUILDING;
				}
				for (i=invisibleFace;i<mc[c]->mr->face_cnt;i++)
				{
					FaceLookup &face = faceLookup[c][i];
					mc[c]->faceRenders[face.abs_index] |= FS__HIDDEN;
					mc[c]->faceRenders[face.abs_index] &= ~FS__BUILDING;
				}
				if(timeForDroneMove())
					sendDrones(c,max(0,invisibleFace-1));
			}
		}
		lastChild = c;
		lastClicks = thisClicks;
	}
}

void TerranBuild::sendDrones(U32 child,U32 faceID)
{
	if (droneMoveTime > 0 && mc[child]->bHasMesh)
	{
		droneMoveTime -= DRONE_UPDATE_TIME;
		const FaceLookup &face = faceLookup[child][faceID];
		Vector bob = mc[child]->mr->pos_list[face.pos_offset+face.group->index_list[face.index*3]];
		bob = trans.rotate_translate(bob);
		Vector nBob = face.group->src_norms[face.group->index_list[face.index*3]];

		nBob = trans.rotate(nBob);
		
		ITerranDrone * drone = getNextDrone();
		if(drone)
		{
			drone->BuildAtPos(bob-300*nBob,nBob);
		}
		
		if (fabObj)
		{
			IBaseObject *poof = ARCHLIST->CreateInstance(arch->pWeldType);
			OBJPTR<IBlast> blast;
			if (poof->QueryInterface(IBlastID,blast) != 0)
			{
				blast->InitBlast(Transform(lastPos),fabObj->GetSystemID(),this);
			}
			
			OBJPTR<IExtent> extentObj;
			fabObj->QueryInterface(IExtentID,extentObj);
			CQASSERT(extentObj);
			extentObj->AddChildBlast(poof);
		}

		lastPos = bob;
	}
}

//---------------------------------------------------------------------------
//
bool TerranBuild::timeForDroneMove ()
{
	return DRONE_UPDATE_TIME < droneMoveTime;
}
//---------------------------------------------------------------------------
//
ITerranDrone * TerranBuild::getNextDrone ()
{
	OBJPTR<ITerranDrone> result;

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
			baseObj->QueryInterface(ITerranDroneID,result);
	}
	return result;
}
//---------------------------------------------------------------------------
//
void TerranBuild::resetDrones()
{
	droneMoveTime = DRONE_UPDATE_TIME;
	droneCnt = 0;
	droneFab = 0;
}
//---------------------------------------------------------------------------
//
void TerranBuild::SetBuildPercent (SINGLE newPercent)
{
	bPaused = false;
	CQASSERT(newPercent >= 0.0f && newPercent <= 1.0f);
	if (percent <= 1.0f)  //does this make sense for the reverse case?
	{
		frames++;
		
		if (!bVisible)
			ENGINE->update_instance(mc[0]->instanceIndex,0,0);
		
		//dangerous get_transform call!!!
		trans = ENGINE->get_transform(mc[lastChild]->instanceIndex);//GetTransform();
		
		percent = newPercent;

		ReBuild();

		composeGirders();
	}
}
//--------------------------------------------------------------------------//
//
BOOL32 TerranBuild::Update (void)
{
	droneMoveTime += ELAPSED_TIME;
	return 1;
}
//---------------------------------------------------------------------------
//
void TerranBuild::PhysicalUpdate(SINGLE dt)
{
	if (buildObj)
		bVisible = buildObj.Ptr()->bVisible;
	else
		bVisible = (fabObj && fabObj.Ptr()->bVisible);

	if (bPaused)
		return;

	if (bVisible)
	{
		percent += rate*dt;
		if (percent > 1.0f)
			percent = 1.0f;
		if (percent < 0.0f)
			percent = 0.0f;

		ReBuild();
		composeGirders();
	}
}
//---------------------------------------------------------------------------
//
#if 0
void TerranBuild::Render (void)
{
	if (bVisible)
	{
		percent += rate*OBJLIST->GetRealRenderTime();
		if (percent > 1.0f)
			percent = 1.0f;
		if (percent < 0.0f)
			percent = 0.0f;

		composeGirders();
	/*	TreeRender(mc,num_childs,Transform());
		BATCH->set_state(RPR_BATCH,1);
		BATCH->set_render_state(D3DRS_ALPHABLENDENABLE,TRUE);
		BATCH->set_render_state(D3DRS_ZENABLE,TRUE);
		BATCH->set_render_state(D3DRS_ZWRITEENABLE,FALSE);  ///?????
		BATCH->set_render_state(D3DRS_CULLMODE,D3DCULL_NONE);
		//BATCH->set_render_state(D3DRS_ZWRITEENABLE,TRUE);
		BATCH->set_render_state(D3DRS_SRCBLEND,D3DBLEND_SRCALPHA);
		BATCH->set_render_state(D3DRS_DESTBLEND,D3DBLEND_INVSRCALPHA);
		//BATCH->set_render_state(D3DRS_ZFUNC,D3DCMP_LESSEQUAL);
		renderMeshChunk();*/
	}
}
#endif
//---------------------------------------------------------------------------
//
void TerranBuild::composeGirders ()
{
	TexCoord temp_tc[MAX_GIRDER_FACES*3];
	U16 temp_tc_refs[MAX_GIRDER_FACES*3];
	U16 temp_refs[MAX_GIRDER_FACES*3];
	U16 temp_face_refs[MAX_GIRDER_FACES+1];

	U16 tc_re_index[MAX_GIRDER_FACES*3];
	
	Vector v[3];
	TexCoord tx[3];

//	SetupDiffuseBlend(arch->buildTex,FALSE);
//	BATCH->set_state(RPR_STATE_ID,arch->buildTex);

	for (int c=0;c<num_childs;c++)
	{
		if (mc[c]->bHasMesh)
		{
			int nextFace = 0;
			U16 tc_cnt=0,vert_cnt=0;
//			Transform tmp = ENGINE->get_transform(mc[c]->instanceIndex);
//			CAMERA->SetModelView(&tmp);
//			PB.Begin(PB_TRIANGLES);
			int i;
			for (i=0;i<mc[c]->mr->face_cnt;i++)
			{
				const FaceLookup &face = faceLookup[c][i];
				
				if ((mc[c]->faceRenders[face.abs_index] & FS__BUILDING) )
				{
					v[0] = mc[c]->mr->pos_list[face.pos_offset+face.group->index_list[face.index*3]];
					v[1] = mc[c]->mr->pos_list[face.pos_offset+face.group->index_list[face.index*3+1]];
					v[2] = mc[c]->mr->pos_list[face.pos_offset+face.group->index_list[face.index*3+2]];
					
					SINGLE dy,dx;
					dy = max(max(v[0].y,v[1].y),v[2].y)-min(min(v[0].y,v[1].y),v[2].y);
					dx = max(max(v[0].x,v[1].x),v[2].x)-min(min(v[0].x,v[1].x),v[2].x);
#define TEXFACTOR 0.003

					int tc_set;
					if (dy>dx)
					{
						tc_set=0;
						tx[0].u = v[0].z*TEXFACTOR;
						tx[0].v = v[0].y*TEXFACTOR;
						tx[1].u = v[1].z*TEXFACTOR;
						tx[1].v = v[1].y*TEXFACTOR;
						tx[2].u = v[2].z*TEXFACTOR;
						tx[2].v = v[2].y*TEXFACTOR;
					}
					else
					{
						tc_set =1;
						tx[0].u = v[0].z*TEXFACTOR;
						tx[0].v = v[0].x*TEXFACTOR;
						tx[1].u = v[1].z*TEXFACTOR;
						tx[1].v = v[1].x*TEXFACTOR;
						tx[2].u = v[2].z*TEXFACTOR;
						tx[2].v = v[2].x*TEXFACTOR;
					}
					
					for (int vv=0;vv<3;vv++)
					{
						U16 src_ref = face.pos_offset+face.group->index_list[face.index*3+vv];
						//this is a slow method but for LOW output poly counts should be ok
						//maybe not ok here
						U16 found_ref = 0xffff;
						for (int i=0;i<tc_cnt;i++)
						{
							if (tc_re_index[i] == (tc_set<<14)+src_ref)
							{
								found_ref = i;
								break;
							}
						}
						
						if (found_ref == 0xffff)
						{
							temp_tc[tc_cnt].u = tx[vv].u;
							temp_tc[tc_cnt].v = tx[vv].v;
							tc_re_index[tc_cnt] = (tc_set<<14)+src_ref;
							found_ref = tc_cnt;
							CQASSERT(tc_cnt < MAX_GIRDER_FACES*3);
							tc_cnt++;
						}
						temp_tc_refs[nextFace*3+vv] = found_ref;
						CQASSERT(tx[vv].u == temp_tc[found_ref].u);
						
						temp_refs[nextFace*3+vv] = src_ref;
					}
					CQASSERT(nextFace < MAX_GIRDER_FACES);
					temp_face_refs[nextFace] = face.abs_index;
					nextFace++;
				}
			}
			memcpy(gr[c]->ec->idx_list,temp_refs,sizeof(U16)*nextFace*3);
			gr[c]->ec->idx_cnt = nextFace*3;
			memcpy(gr[c]->ec->tc,temp_tc,sizeof(TexCoord)*tc_cnt);
			gr[c]->ec->tc_cnt = tc_cnt;
			memcpy(gr[c]->ec->tc_idx_list,temp_tc_refs,sizeof(U16)*nextFace*3);
			gr[c]->ec->vert_cnt = vert_cnt;

			//swap sort temp refs - i'd love something better
			for (i=0;i<nextFace-1;i++)
			{
				for (int j=i+1;j<nextFace;j++)
				{
					if (temp_face_refs[i] > temp_face_refs[j])
					{
						U16 temp = temp_face_refs[i];
						temp_face_refs[i] = temp_face_refs[j];
						temp_face_refs[j] = temp;
					}
				}
			}

			//make sure the "last" face has a really huge index for splitting
			temp_face_refs[nextFace] = 0xffff;
			memcpy(gr[c]->ec->face_ref_list,temp_face_refs,sizeof(U16)*(nextFace+1));
		}
	}

#undef TEXFACTOR
}

void TerranBuild::AddFabricator(IFabricator *_fab)
{
	OBJPTR<IFabricator> fab;
	_fab->QueryInterface(IFabricatorID,fab);
	fabID[numFabWorking] = fab.Ptr()->GetPartID();
	++numFabWorking;

	numDrones += fab->GetNumDrones();

	resetDrones();

	//dangerous get_transform call!!!
	trans = ENGINE->get_transform(mc[lastChild]->instanceIndex);
	//seemingly useful - not necessarily valid at this time - do in SynchBuilderShips
/*	OBJPTR<ITerranDrone> drone;
	for(unsigned int c = 0; c<fab->GetNumDrones() ; c++)
	{
		IBuildShip * obj;
		fab->GetDrone(&obj,c);
		obj->QueryInterface(ITerranDroneID,drone);
		Vector bob = mc[0]->buildMesh->object_vertex_list[mc[0]->buildMesh->vertex_batch_list[mc[0]->buildMesh->face_groups[mc[0]->myFaceArray[0].groupID].face_vertex_chain[mc[0]->myFaceArray[0].index*3] ]];
		bob.y += numFabWorking*300+c*100;
		TRANSFORM t = fab.ptr->GetTransform();
		bob = t.rotate_translate(bob);
		drone->SetPosition(bob);
		drone->IdleAtPos(bob);
	}*/
}

void TerranBuild::RemoveFabricator(IFabricator *_fab)
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
	numDrones -= fab->GetNumDrones();
	resetDrones();

	for(unsigned int c = 0; c<fab->GetNumDrones() ; c++)
	{
		IBuildShip * obj;
		fab->GetDrone(&obj,c);
		obj->Return();
	}
}

void TerranBuild::SynchBuilderShips()
{
	for (int i=0;i<numDrones;i++)
	{
		ITerranDrone *drone = getNextDrone();
		drone->SetTransform(trans);
		drone->IdleAtPos(trans.translation);
	}
}

void TerranBuild::Done ()
{
	//leaving this for the destructor.......
/*	delete [] childTimes;
	delete [] girderBuildAhead;
	childTimes = 0;
	girderBuildAhead = 0;*/

	percent = 1.0f;
	rate = 0.0f;

	ReBuild();

	composeGirders();
	
	for (int c=0;c<num_childs;c++)
	{
		mc[c]->RemoveEffectChannel(gr[c]->ec);
		delete gr[c]->ec;
	}
}

void TerranBuild::PrepareForExplode()
{
	composeGirders();
}
//------------------------------------------------------------------------------------------
//---------------------------TerranBuild Factory----------------------------------------------
//------------------------------------------------------------------------------------------
//
struct DACOM_NO_VTABLE TerranBuildFactory : public IObjectFactory
{
	struct OBJTYPE : TerranBuildArchetype
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
			TMANAGER->ReleaseTextureRef(buildTex);
			ARCHLIST->Release(pWeldType, OBJREFNAME);
		}
	};
	//------------------------------------------------------------------------------------------
	//
	U32 factoryHandle;		// handles to callback

	//
	// Interface mapping
	//

	BEGIN_DACOM_MAP_INBOUND(TerranBuildFactory)
	DACOM_INTERFACE_ENTRY(IObjectFactory)
	END_DACOM_MAP()

	TerranBuildFactory (void) { }

	~TerranBuildFactory (void);

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

	/* TerranBuildFactory methods */

	IDAComponent * getBase (void)
	{
		return static_cast<IObjectFactory *> (this);
	}
};
//--------------------------------------------------------------------------//
//
TerranBuildFactory::~TerranBuildFactory (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (OBJLIST && OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
		connection->Unadvise(factoryHandle);
}
//--------------------------------------------------------------------------//
//
void TerranBuildFactory::init (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
		connection->Advise(getBase(), &factoryHandle);
}
//-----------------------------------------------------------------------------
//
HANDLE TerranBuildFactory::CreateArchetype (const char *szArchname, OBJCLASS objClass, void *_data)
{
	OBJTYPE * result = 0;

	if (objClass == OC_BUILDOBJ)
	{
		BT_TERRAN_BUILD * data = (BT_TERRAN_BUILD *) _data;
		
		if (data->boClass == BO_TERRAN)
		{
			result = new OBJTYPE;
			char fname[64] = "tinnards.tga";
			CQASSERT(fname[0]);
			result->buildTex = TMANAGER->CreateTextureFromFile(fname, TEXTURESDIR, DA::TGA, PF_4CC_DAA4);
			
			result->pWeldType = ARCHLIST->LoadArchetype("BLAST!!Weld");
			CQASSERT(result->pWeldType);
			ARCHLIST->AddRef(result->pWeldType, OBJREFNAME);
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
BOOL32 TerranBuildFactory::DestroyArchetype (HANDLE hArchetype)
{
	delete (OBJTYPE *)hArchetype;
	return 1;
}
//-------------------------------------------------------------------
//
IBaseObject * TerranBuildFactory::CreateInstance (HANDLE hArchetype)
{
	OBJTYPE * objtype = (OBJTYPE *)hArchetype;
	TerranBuild * obj = new ObjectImpl<TerranBuild>;
	obj->arch = objtype;

	obj->objClass = OC_BUILDOBJ;

	return obj;

}
//-------------------------------------------------------------------
//
void TerranBuildFactory::EditorCreateInstance (HANDLE hArchetype, const MISSION_INFO & info)
{
	CQBOMB0("Editor insertion not supported");
}
//---------------------------------------------------------------------------
//-------------------------GLOBAL STARTUP------------------------------------
//---------------------------------------------------------------------------
//
struct _terranbuild : GlobalComponent
{
	TerranBuildFactory * sfactory;

	virtual void Startup (void)
	{
		sfactory = new DAComponent<TerranBuildFactory>;
		AddToGlobalCleanupList((IDAComponent **) &sfactory);
	}

	virtual void Initialize (void)
	{
		sfactory->init();
	}
};

static _terranbuild __tb;

//---------------------------------------------------------------------------
//-------------------------End TerranBuild.cpp-------------------------------
//---------------------------------------------------------------------------
