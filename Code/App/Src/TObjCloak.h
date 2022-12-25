#ifndef TOBJCLOAK_H
#define TOBJCLOAK_H
//--------------------------------------------------------------------------//
//                                                                          //
//                              TObjCloak.h                                 //
//                                                                          //
//                  COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/TObjCloak.h 35    9/13/01 10:01a Tmauer $
*/			    
//------------------------------- #INCLUDES --------------------------------//
//--------------------------------------------------------------------------//

#ifndef MESH_H
#include "Mesh.h"
#endif

#ifndef RENDERER_H
#include "Renderer.h"
#endif

#ifndef CLOAK_H
#include "ICloak.h"
#endif

#ifndef _INC_STDLIB
#include <stdlib.h>
#endif

#ifndef ENGINE_H
#include "Engine.h"
#endif

#ifndef MYVERTEX_H
#include "MyVertex.h"
#endif

#ifndef IATTACK_H
#include "IAttack.h"
#endif

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
#define CLOAK_TIME 1.0
#define ObjectCloak _Coc

template <class Base=IBaseObject> 
struct _NO_VTABLE ObjectCloak : public Base, ICloak, CLOAK_SAVELOAD_BASE
{
	typename typedef Base::SAVEINFO CLOAKSAVEINFO;
	typename typedef Base::INITINFO CLOAKINITINFO;

	struct PreRenderNode  preRenderNode;
//	struct PostRenderNode postRenderNode;
	struct InitNode       initNode;
	struct PhysUpdateNode physUpdateNode;
	struct SaveNode			saveNode;
	struct LoadNode         loadNode;
	struct PreTakeoverNode	preTakeoverNode;
	
//	Mesh *cmesh;

	U32 cloakTex,cloakTex2;
	PARCHETYPE pCloakEffect;
	SINGLE cloakShift;

	ObjectCloak (void);
	~ObjectCloak (void);

	/* ICloak methods */

	virtual void EnableCloak(bool bEnable);

/*	virtual void SetCloakType(bool _bDrawCloaking)
	{
		bDrawCloaking = _bDrawCloaking;
	}*/

	virtual bool CanCloak() {return false;};

	virtual bool IsToggle() {return false;};

	virtual bool CanToggle(){return false;};

	virtual bool IsOn() {return false;};

	/* ObjectCloak methods */
	void initCloak (const CLOAKINITINFO & data);
	void preRenderCloak (void);
//	void cloakPostRender (void);
	void physUpdateCloak (SINGLE dt);
	void cloak (void);
	void uncloak (void);
	void preTakeoverCloak (U32 newMissionID, U32 troopID);

	void saveCloak (CLOAKSAVEINFO & save);
	void loadCloak (CLOAKSAVEINFO & load);

	void RenderCloaked(U32 texID);
};

//---------------------------------------------------------------------------
//
template <class Base> 
ObjectCloak< Base >::ObjectCloak (void) :
					preRenderNode(this, RenderProc(&ObjectCloak::preRenderCloak)),
				//	postRenderNode(this, RenderProc(cloakPostRender)),
					initNode(this, InitProc(&ObjectCloak::initCloak)),
					physUpdateNode(this, PhysUpdateProc(&ObjectCloak::physUpdateCloak)),
					saveNode(this, SaveLoadProc(&ObjectCloak::saveCloak)),
					loadNode(this, SaveLoadProc(&ObjectCloak::loadCloak)),
					preTakeoverNode(this, PreTakeoverProc(&ObjectCloak::preTakeoverCloak))
{
//	cloaking = 0;
//	bDrawCloaking=TRUE;
}

template <class Base> 
ObjectCloak< Base >::~ObjectCloak (void) 
{

}

//---------------------------------------------------------------------------
//
template <class Base>
void ObjectCloak< Base >::initCloak (const CLOAKINITINFO & data)
{
	cloakTex = data.cloakTex;
	cloakTex2 = data.cloakTex2;
	bCloakPending = data.pData->cloak.bAutoCloak;

}
//---------------------------------------------------------------------------
//
template <class Base>
void ObjectCloak< Base >::preRenderCloak()
{
	if (bCloaked || (cloakTimer <= 1.0 && cloakTimer > 0))
	{
		BATCH->set_state(RPR_BATCH,TRUE);

		BATCH->set_render_state(D3DRS_ZENABLE,TRUE);
		BATCH->set_render_state(D3DRS_ZWRITEENABLE,TRUE);
		
		BATCH->set_render_state(D3DRS_ALPHABLENDENABLE,TRUE);
		BATCH->set_render_state(D3DRS_SRCBLEND,D3DBLEND_SRCALPHA);
		BATCH->set_render_state(D3DRS_DESTBLEND,D3DBLEND_INVSRCALPHA);
		BATCH->set_render_state(D3DRS_ZFUNC,D3DCMP_LESSEQUAL);

		SetupDiffuseBlend(cloakTex,FALSE);
		BATCH->set_state(RPR_DELAY,1);
		BATCH->set_state(RPR_STATE_ID,cloakTex);
		CAMERA->SetModelView(&transform);

		//this needs to be updated
		RenderCloaked(cloakTex);
		BATCH->set_state(RPR_STATE_ID,0);
		BATCH->set_state(RPR_DELAY,0);
	}
}
//---------------------------------------------------------------------------
//
template <class Base> 
void ObjectCloak< Base >::EnableCloak (bool bEnable)
{
	if (bEnable)
	{
		++cloakCount;
		if(!bCloaking)
			cloak();
	}
	else
	{
		CQASSERT(cloakCount);
		--cloakCount;
		if(!cloakCount)
			uncloak();
	}
}
//---------------------------------------------------------------------------
//
template <class Base> 
void ObjectCloak< Base >::cloak (void)
{
	bCloaking = true;
	cloakTimer = CLOAK_TIME+0.5;
	if (pCloakEffect)
	{
		IBaseObject *obj = ARCHLIST->CreateInstance(pCloakEffect);
		if (obj)
		{
			OBJLIST->AddObject(obj);
			OBJPTR<ICloakEffect> effect;
			if (obj->QueryInterface(ICloakEffectID,effect))
				effect->Init(this);
		}
	}

/*	// if we are cloaking, then set the units stance to stop (peace)
	OBJPTR<IAttack> attack;
	if (static_cast<IBaseObject*>(this)->QueryInterface(IAttackID, attack))
	{
		attack->SetUnitStance(US_STOP);
	}
*/
}
//---------------------------------------------------------------------------
//
template <class Base> 
void ObjectCloak< Base >::uncloak (void)
{
	bCloaking = false;
	cloakTimer = CLOAK_TIME;
	bCloaked = false;
	bSpecialRender = false;
}
//---------------------------------------------------------------------------
//
template <class Base> 
void ObjectCloak< Base >::preTakeoverCloak (U32 newMissionID, U32 troopID)
{
	if(dwMissionID != newMissionID)
	{
		if(cloakCount)
		{
			cloakCount = 0;
			uncloak();
		}
	}
}
/*static bDummy()
{
	int h=3;
	h++;
}*/
//---------------------------------------------------------------------------
//
template <class Base> 
void ObjectCloak< Base >::physUpdateCloak(SINGLE dt)
{
		// are we within range of other ships?
/*	U8 enemyMask = ~(MGlobals::GetAllyMask(dwMissionID & PLAYERID_MASK));
	U8 visMask   = GetVisibilityFlags();
	U8 visible = enemyMask & visMask;*/

	if (bCloakPending && !building)
	{
		cloak();
		bCloakPending = 0;
	}

	if (cloakTimer > 0)
	{
		cloakTimer -= dt;
		if (cloakTimer <= 0)
		{
			if (bCloaking)
			{
				bCloaked = 1;
			}
			else
				bCloaked = 0;
			bSpecialRender = bCloaked;
			cloakTimer = 0;
		}

		if (cloakTimer < CLOAK_TIME)
		{
			if (bCloaking)
				cloakPercent = (cloakTimer/CLOAK_TIME);
			else
				cloakPercent = ((CLOAK_TIME-cloakTimer)/CLOAK_TIME);
			
/*			for (int j=0;j<mc.numChildren;j++)
			{
				for (int i=0;i<mc.mi[j]->faceGroupCnt;i++)
				{
					mc.mi[j]->fgi[i].a = 255*cloakPercent;
				}
			}
*/		}
	}
}

template <class Base> 
void ObjectCloak< Base >::RenderCloaked(U32 texID)
{
/*	U16 re_index_list[1000];

	BATCH->set_render_state(D3DRS_CULLMODE,D3DCULL_CW);//NONE);
	
	BATCH->set_render_state(D3DRS_DITHERENABLE,TRUE);
	
	SINGLE shiftX,shiftY;
	Vector v[3];
	Vector pos = transform.translation*0.001;
	SINGLE heading = transform.get_yaw();
	pos.x = fmod(pos.x,1.0)-0.5;
	pos.y = fmod(pos.y,1.0)-0.5;
	shiftX = pos.x+0.3*cos(1+2*cloakShift);
	cloakShift+= 0.01;
	shiftY = pos.y+0.3*cos(cloakShift);

	U32 color;
	if (cloakTimer <= 1.0 && cloakTimer > 0)
	{
		color = U8((1.0-cloakPercent)*255)<<24 | 0x00ffffff;
	}
	else
		color = 0xffffffff;

	const Transform *world_to_view =CAMERA->GetInverseTransform();
	Transform Tov = (*world_to_view)*transform;
	
	int f=0;
	int base_v;
	for (int c=0;c<mc.numChildren;c++)
	{
		base_v=0;
		if (mc.mi[c]->bHasMesh && mc.mi[c]->mr->pos_cnt)
		{
			Vector *pos_list = mc.mi[c]->mr->pos_list;

			TRANSFORM trans = ENGINE->get_transform(mc.mi[c]->instanceIndex);
			CAMERA->SetModelView(&trans);
			
			TRANSFORM inv = trans.get_inverse();
			
			BATCHDESC desc;
			desc.type = D3DPT_TRIANGLELIST;
			desc.vertex_format = D3DFVF_RPVERTEX;
			desc.num_verts = mc.mi[c]->mr->pos_cnt;
			desc.num_indices = mc.mi[c]->mr->face_cnt*3;
			CQASSERT(mc.mi[c]->mr->pos_cnt < 1000);
			memset(re_index_list,0xff,sizeof(U16)*mc.mi[c]->mr->pos_cnt);
			CQBATCH->GetPrimBuffer(&desc);
			RPVertex *dest_verts = (RPVertex *)desc.verts;
			U16 *id_list = desc.indices;
			
			IRenderMaterial *irm=0;
			
			int v_out=0;
			int i_out=0;

			while((irm = mc.mi[c]->mr->GetNextFaceGroup(irm)) != 0)
			{
				U8 *faceRenders = &mc.mi[c]->faceRenders[irm->face_offset];
				U16 *index_list = irm->index_list;
				int i=0;
				while (i<irm->new_face_cnt)
				{
					Vector n[3];
					TexCoord t[3];
					SINGLE tu,tv;
					
					if ((faceRenders[i] & (FS__HIDDEN | FS__BUILDING)) == 0)
					{
						Vector v[3];
						n[0] = v[0]=pos_list[base_v+index_list[i*3]];
						n[1] = v[1]=pos_list[base_v+index_list[i*3+1]];
						n[2] = v[2]=pos_list[base_v+index_list[i*3+2]];
						
						n[0].fast_normalize();
						n[1].fast_normalize();
						n[2].fast_normalize();
						
						for (int j=0;j<3;j++)
						{
							n[j] = Tov.rotate(n[j]);
							
							int src_ref = base_v+index_list[i*3+j];
							if (re_index_list[src_ref] == 0xffff)
							{
								tu = v[j].x*0.0012;
								tv = v[j].z*0.0012;
								t[j].u = -tu*cos(heading)-tv*sin(heading);
								t[j].v = -tu*(-sin(heading))-tv*cos(heading);
								dest_verts[v_out].pos = v[j];
								dest_verts[v_out].u = t[j].u+shiftX;
								dest_verts[v_out].v = t[j].v+shiftY;
								dest_verts[v_out].color = color;
								re_index_list[src_ref] = v_out;
								v_out++;
							}
							id_list[i_out++] = re_index_list[src_ref];
						}
					}

					f++;
					i++;
				}
				base_v += irm->vert_cnt;  //this will change if I ever stop making pos_list parallel
			}
			desc.num_verts = v_out;
			desc.num_indices = i_out;
			CQBATCH->ReleasePrimBuffer(&desc);
		}
	}*/
}
//---------------------------------------------------------------------------
//
template <class Base> 
void ObjectCloak< Base >::saveCloak (CLOAKSAVEINFO & save)
{
	save.cloak_SL.baseCloak = *static_cast<CLOAK_SAVELOAD_BASE *>(this);
	save.cloak_SL.bCloaked = bCloaked;
}
//---------------------------------------------------------------------------
//
template <class Base> 
void ObjectCloak< Base >::loadCloak (CLOAKSAVEINFO & load)
{
	*static_cast<CLOAK_SAVELOAD_BASE *>(this) = load.cloak_SL.baseCloak;
	bCloaked = load.cloak_SL.bCloaked;
	if(bCloaked)
		bSpecialRender = true;
}
//---------------------------------------------------------------------------
//---------------------------End TObjCloak.h---------------------------------
//---------------------------------------------------------------------------
#endif