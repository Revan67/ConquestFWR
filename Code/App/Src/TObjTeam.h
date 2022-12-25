#ifndef TOBJTEAM_H
#define TOBJTEAM_H
//--------------------------------------------------------------------------//
//                                                                          //
//                              TObjTeam.h                                 //
//                                                                          //
//                  COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/TObjTeam.h 45    8/23/01 9:12a Tmauer $
*/			    
//------------------------------- #INCLUDES --------------------------------//
//--------------------------------------------------------------------------//

#ifndef MESH_H
#include "Mesh.h"
#endif

#ifndef RENDERER_H
#include "Renderer.h"
#endif

//#ifndef MATERIAL_H
//#include "Material.h"
//#endif

#ifndef _INC_STDLIB
#include <stdlib.h>
#endif

#ifndef ENGINE_H
#include "Engine.h"
#endif

#ifndef ARCHHOLDER_H
#include "ArchHolder.h"
#endif

#ifndef IBLINKERS_H
#include "IBlinkers.h"
#endif

#include <IMeshManager.h>
#include <IMaterialManager.h>

#define MAX_MATS 30

#define ObjectTeam _Cot
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
template <class Base=IBaseObject> 
struct _NO_VTABLE ObjectTeam : public Base
{
	struct PreRenderNode preRenderNode;
	struct PostRenderNode postRenderNode;
	struct PhysUpdateNode physUpdateNode;
	struct InitNode      initNode;

	typename typedef Base::INITINFO TEAMINITINFO;

	COLORREF currentColor;

	IModifier * colorMod;

	//BLINKER STUFF
	COMPTR<IBlinkers> blinkers;
	U32 blinkerTexID;

	U32 billboardThreshhold;//stored here becasue this is the lowest point it is used.

	ObjectTeam (void);
	~ObjectTeam (void);

	/* ObjectTeam methods */
	void initTeam (const TEAMINITINFO & data);
	void preRenderTeam (void);
	void postRenderTeam (void);
	void physUpdateTeam (SINGLE dt);
	virtual void SetColors();
//	void markChild (INSTANCE_INDEX parentIdx);
	void GetAllChildren(INSTANCE_INDEX instanceIndex,INSTANCE_INDEX *array,S32 &last,S32 arraySize);

	void renderBlinkers();

};
//---------------------------------------------------------------------------
//
template <class Base> 
ObjectTeam< Base >::ObjectTeam (void) :
					preRenderNode(this, RenderProc(&ObjectTeam::preRenderTeam)),
					postRenderNode(this, RenderProc(&ObjectTeam::postRenderTeam)),
					physUpdateNode(this, PhysUpdateProc(&ObjectTeam::physUpdateTeam)),
					initNode(this, InitProc(&ObjectTeam::initTeam))
{
	blinkerTexID = 0;
	colorMod = 0;
}

template <class Base> 
ObjectTeam< Base >::~ObjectTeam (void) 
{
	if(colorMod)
	{
		colorMod->Release();
	}
}

//---------------------------------------------------------------------------
//
template <class Base>
void ObjectTeam< Base >::initTeam (const TEAMINITINFO & data)
{
//	currentColor = COLORTABLE[MGlobals::GetColorID(playerID)];
	//SetColors();

	//blinkers
	if (data.blink_arch)
		CreateBlinkers(blinkers,data.blink_arch,instanceIndex);

	blinkerTexID = data.blinkTex;
}
//---------------------------------------------------------------------------
//
template <class Base> 
void ObjectTeam< Base >::preRenderTeam (void)
{
	
	COLORREF color = COLORTABLE[MGlobals::GetColorID(playerID)];

	if (color != currentColor)
	{
		SetColors();
		currentColor = color;
	}	

}
//---------------------------------------------------------------------------
//
template <class Base> 
void ObjectTeam< Base >::postRenderTeam (void)
{
	if (blinkers != 0 && bReady && bSpecialRender == 0 && ( (!billboardThreshhold) || 
		((OBJLIST->GetShipsToRender() <= billboardThreshhold * CQEFFECTS.nFlatShipScale)|| CQEFFECTS.nFlatShipScale == 4) 	))
	{
		renderBlinkers();
	}
}
//---------------------------------------------------------------------------
//
template <class Base> 
void ObjectTeam< Base >::physUpdateTeam (SINGLE dt)
{
	if (blinkers)
		blinkers->Update(dt);
}
//---------------------------------------------------------------------------
//
/*template <class Base> 
void ObjectTeam< Base >::SetColors (void)
{	
	S32 children[30];
	S32 num_children=0;

	Material *mat[MAX_MATS];
//	int mat_cnt=0;

	memset(mat, 0, sizeof(mat));

	GetAllChildren(instanceIndex,children,num_children,30);
	COLORREF color = COLORTABLE[MGlobals::GetColorID(playerID)];

	for (int c=0;c<num_children;c++)
	{
		//Mesh *mesh = REND->get_unique_instance_mesh(children[c]);
		Mesh *mesh = REND->get_instance_mesh(children[c]);

		if (mesh)
		{
			Material *tmat = mesh->material_list;
			for (int i = 0; i < mesh->material_cnt; i++, tmat++)
			{
				if (strstr(tmat->name, "markings") != 0)
				{
					if (CQRENDERFLAGS.bSoftwareRenderer)
					{
						tmat->texture_id = 0;
					}
					
					tmat->specular.r = tmat->diffuse.r = GetRValue(color);
					tmat->specular.g = tmat->diffuse.g = GetGValue(color);
					tmat->specular.b = tmat->diffuse.b = GetBValue(color);

					tmat->emission.r = 0.3*tmat->diffuse.r;
					tmat->emission.g = 0.3*tmat->diffuse.g;
					tmat->emission.b = 0.3*tmat->diffuse.b;
				}
			}
		}
	}
}*/
//---------------------------------------------------------------------------
//
template <class Base> 
void ObjectTeam< Base >::SetColors (void)
{	
	if (bExploding)
		return;
	
	if(instanceMesh)
	{
		COLORREF color = COLORTABLE[MGlobals::GetColorID(playerID)];
		U32 maxFG = instanceMesh->GetArchtype()->GetNumFaceGroups();
		for(U32 fg = 0; fg < maxFG; ++fg)
		{
			IMaterial * mat = instanceMesh->GetArchtype()->GetFaceGroupMaterial(fg);
			if(mat)
			{
				IModifier * modSearch = colorMod;
				while(modSearch)
				{
					if(modSearch->GetMaterial() == mat)
						break;
					modSearch = modSearch->GetNextModifier();
				}
				if(modSearch)
					modSearch = mat->CreateModifierColor("TeamColor",GetRValue(color),GetGValue(color),GetBValue(color),modSearch);
				else
				{
					modSearch = mat->CreateModifierColor("TeamColor",GetRValue(color),GetGValue(color),GetBValue(color),modSearch);
					modSearch->SetNextModifier(colorMod);
					colorMod = modSearch;
				}

			}
		}
		instanceMesh->SetModifierList(colorMod);
	}

//	S32 children[30];
//	S32 num_children=0;

//	Material *mat[MAX_MATS];
//	int mat_cnt=0;

//	memset(mat, 0, sizeof(mat));

//	GetAllChildren(instanceIndex,children,num_children,30);
//	COLORREF color = COLORTABLE[MGlobals::GetColorID(playerID)];
	
/*	for (int c=0;c<mc.numChildren;c++)
	{	
		//Mesh *mesh = REND->get_unique_instance_mesh(children[c]);
		//Mesh *mesh = REND->get_instance_mesh(children[c]);
		
		if (mc.mi[c]->bHasMesh)
		{
			IRenderMaterial *irm=0;
			
			while((irm = mc.mi[c]->mr->GetNextFaceGroup(irm)) != 0)
			{
				FaceGroupInfo *fgi = &mc.mi[c]->fgi[irm->fg_idx];
				//Material *tmat = &mesh->material_list[mesh->face_groups[fg_cnt].material];
				
				//if (strstr(tmat->name, "markings") != 0)
				
				if (irm->flags & MM_MARKINGS)
				{
					color = COLORTABLE[MGlobals::GetColorID(playerID)];
					
					fgi->diffuse.r = GetRValue(color);
					fgi->diffuse.g = GetGValue(color);
					fgi->diffuse.b = GetBValue(color);
					
					fgi->emissive.r = 0.3*fgi->diffuse.r;
					fgi->emissive.g = 0.3*fgi->diffuse.g;
					fgi->emissive.b = 0.3*fgi->diffuse.b;
				}
			}
		}
	}
	CQASSERT(HEAP->EnumerateBlocks());*/
}
//---------------------------------------------------------------------------
//
template <class Base>
void ObjectTeam< Base >::GetAllChildren (INSTANCE_INDEX instanceIndex,INSTANCE_INDEX *array,S32 &last,S32 arraySize)
{
	if (last < arraySize)
	{
		array[last] = instanceIndex;
		last++;
		INSTANCE_INDEX lastChild = INVALID_INSTANCE_INDEX,child;
		while ((child = ENGINE->get_instance_child_next(instanceIndex,EN_DONT_RECURSE,lastChild)) != INVALID_INSTANCE_INDEX)
		{
			GetAllChildren(child,array,last,arraySize);
			lastChild = child;
		}
	}
}
//---------------------------------------------------------------------------
//
template <class Base> 
void ObjectTeam< Base >::renderBlinkers()
{
	Vector points[32];
	SINGLE intensity[32];

	S32 numBlinkers = blinkers->GetBlinkers(points,intensity,32);

	U8 i;
	
//	BATCH->set_texture_stage_texture( 0,blinkerTexID);
	SetupDiffuseBlend(blinkerTexID,TRUE);
	BATCH->set_state(RPR_STATE_ID,blinkerTexID);
	BATCH->set_render_state(D3DRS_ALPHABLENDENABLE,TRUE);
	BATCH->set_render_state(D3DRS_CULLMODE,D3DCULL_NONE);
	BATCH->set_render_state(D3DRS_ZWRITEENABLE,FALSE);
	BATCH->set_render_state(D3DRS_ZENABLE,TRUE);
	BATCH->set_render_state(D3DRS_DITHERENABLE,TRUE);
	BATCH->set_render_state(D3DRS_SRCBLEND,D3DBLEND_SRCALPHA);
	BATCH->set_render_state(D3DRS_DESTBLEND,D3DBLEND_INVSRCALPHA);
	
	//render blinkers
	CAMERA->SetModelView();

	PB.Begin(PB_QUADS);
	
	Vector cpos (CAMERA->GetPosition());
	for (i=0;i<numBlinkers;i++)
	{
			PB.Color4ub(255,255,255,intensity[i]*255);
			//set dynamic lights
	/*		blinkLight[i].color.r = cosB*255;
			blinkLight[i].setColor(cosB*255,0,0);
			blinkLight[i].set_On(TRUE);*/
				

			
			Vector t = points[i];//transform.rotate_translate(points[i]);
			//TEMPORARY
	//		blinkLight[i].set_position(Vector(t.x,t.y,t.z+400));
	//		blinkLight[i].setSystem(systemID);

			Vector look (t - cpos);
			
			Vector i (look.y, -look.x, 0);
			
			if (fabs (i.x) < 1e-4 && fabs (i.y) < 1e-4)
			{
				i.x = 1.0f;
			}
			
			i.normalize ();
			
			Vector k (-look);
			k.normalize ();
			Vector j (cross_product (k, i));
			
#define BLINKER_SIZE 60

			i *= BLINKER_SIZE;
			j *= BLINKER_SIZE;
			Vector v0,v1,v2,v3;
			v0 = t-i-j;
			v1 = t+i-j;
			v2 = t+i+j;
			v3 = t-i+j;
			
			PB.TexCoord2f(0,0);  PB.Vertex3f(v0.x,v0.y,v0.z+30);
			PB.TexCoord2f(1,0);  PB.Vertex3f(v1.x,v1.y,v1.z+30);
			PB.TexCoord2f(1,1);  PB.Vertex3f(v2.x,v2.y,v2.z+30);
			PB.TexCoord2f(0,1);  PB.Vertex3f(v3.x,v3.y,v3.z+30);
	}

	PB.End();
	BATCH->set_state(RPR_STATE_ID,0);
}
//---------------------------------------------------------------------------
//---------------------------End TObjTeam.h---------------------------------
//---------------------------------------------------------------------------
#endif