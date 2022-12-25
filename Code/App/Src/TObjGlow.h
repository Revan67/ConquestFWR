#ifndef TOBJGLOW_H
#define TOBJGLOW_H
//--------------------------------------------------------------------------//
//                                                                          //
//                              TObjGlow.h                                 //
//                                                                          //
//                  COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/TObjGlow.h 42    8/23/01 9:12a Tmauer $
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

#ifndef IHARDPOINT_H
#include "IHardPoint.h"
#endif

#ifndef HPENUM_H
#include "HPEnum.h"
#endif

#define ObjectGlow _Cog
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
template <class Base=IBaseObject> 
struct _NO_VTABLE ObjectGlow : public Base
{
//	struct PreRenderNode preRenderNode;
	struct PostRenderNode postRenderNode;
	struct InitNode	     initNode;
//	struct UpdateNode	updateNode;

	typename typedef Base::INITINFO GLOWINITINFO;

	//	Mesh *mesh;
	//Material *mat;
	SINGLE glow;
	S32 matID[2];
	INSTANCE_INDEX eng_id[2];

	INSTANCE_INDEX engine_child_index[MAX_ENGINE_GLOWS];
	Vector glowPt[MAX_ENGINE_GLOWS];
	Vector glowDir[MAX_ENGINE_GLOWS];
	S32 glowSize[MAX_ENGINE_GLOWS];
	U32 engineTex;

	U8 r,g,b;
	U8 num_hp;
	U8 numEngMats;

	ObjectGlow (void);
	~ObjectGlow (void);

	/* ObjectGlow methods */
	void initGlow (const GLOWINITINFO & data);
//	BOOL32 updateGlow();
	void glowPreRender (void);
	void glowPostRender (void);

private:

	struct HPEnumerator : IHPEnumerator
	{
		ObjectGlow<Base> *that;
		virtual bool EnumerateHardpoint (const HPENUMINFO & info)
		{
			int idx = atol(&info.name[8]);
			CQASSERT(idx >=0 && idx <= MAX_ENGINE_GLOWS);
			that->glowPt[idx] = info.hardpointinfo.point;
			that->glowDir[idx] = -info.hardpointinfo.orientation.get_k();
			that->glowSize[idx] = 1; //set a flag saying we found this hardpoint
			that->num_hp++;
			return (that->num_hp<MAX_ENGINE_GLOWS);
		}
	};

};

/*static void GDummy()
{
};*/
//---------------------------------------------------------------------------
//
template <class Base> 
ObjectGlow< Base >::ObjectGlow (void) :
				//	updateNode(this, UpdateProc(updateGlow)),
				//	preRenderNode(this, RenderProc(glowPreRender)),
					postRenderNode(this, RenderProc(&ObjectGlow::glowPostRender)),
					initNode(this, InitProc(&ObjectGlow::initGlow))
{
	for (int c=0;c<MAX_ENGINE_GLOWS;c++)
		engine_child_index[c] = INVALID_INSTANCE_INDEX;

//	matID[0] = matID[1] = -1;
}

template <class Base> 
ObjectGlow< Base >::~ObjectGlow (void) 
{

}

//---------------------------------------------------------------------------
//
template <class Base>
void ObjectGlow< Base >::initGlow (const GLOWINITINFO & data)
{
	BASE_SPACESHIP_DATA *pData = (BASE_SPACESHIP_DATA *)data.pData;//& ((const GLOWINITINFO *) &_data)->pData
//	glowPt[0].z = box[MAXZ]*0.95;
//	numGlows = 1;

	//I don't think we want this anymore
	/*for (int j=0;j<mc.numChildren;j++)
	{
		Mesh * mesh = REND->get_archetype_mesh(HARCH(mc.mi[j]->instanceIndex));
		
		if (mesh)
		{
			Material *tmat = mesh->material_list;
			for (int i = 0; i < mesh->material_cnt; i++, tmat++)
			{
				if (strstr(tmat->name, "eng") != 0)
				{
					//mat = tmat;
					CQASSERT(numEngMats < 2 && "Three engines - must update code");
					matID[numEngMats] = i;
					eng_id[numEngMats++] = mc.mi[j]->instanceIndex;
					tmat->diffuse.r = 100;
					tmat->diffuse.g = 100;
					tmat->diffuse.b = 100;
					break;
				}
			}
		}
	}*/


	HPEnumerator hardpointEnum;
	hardpointEnum.that = this;
//	GDummy();
	EnumerateHardpoints(instanceIndex,"hp_eglow*",&hardpointEnum);
	for (int i=0;i<MAX_ENGINE_GLOWS;i++)
	{
		if (glowSize[i])
		{
			glowSize[i] = pData->engineGlow.size[i];
			if (glowSize[i] == 1)  //flag saying we found this hardpoint
				glowSize[i] = 200;
			r = pData->engineGlow.r;
			g = pData->engineGlow.g;
			b = pData->engineGlow.b;
		}
		else
			glowSize[i] = 0;
	}

/*	const char *fname = "EngineGlow.tga";
	if ((engineTex = TXMLIB->get_texture_id(fname)) == 0)
	{
		if ((engineTex = CreateTextureFromFile(fname, TEXTURESDIR, DA::TGA, PF_4CC_DAA4)) != 0)
		{
			TXMLIB->set_texture_id(fname, engineTex);
			TXMLIB->get_texture_id(fname);	// add 1 reference
		}
	}*/
	engineTex = data.engineTex;
}
/*
template <class Base>
void ObjectGlow< Base >::physUpdateGlow (SINGLE dt)
{


	return TRUE;
}
*/
//---------------------------------------------------------------------------
//
#if 0

template <class Base> 
void ObjectGlow< Base >::glowPreRender (void)
{
	Mesh *mesh;
	Material *mat;
	
	

/*	if (mesh->material_cnt <= matID)
	{
		matID = 0;
		Material *tmat = mesh->material_list;
		for (int i = 0; i < mesh->material_cnt; i++, tmat++)
		{
			if (strstr(tmat->name, "eng") != 0)
			{
				//mat = tmat;
				matID = i;
				break;
			}
		}
	}*/

	if (!bExploding)
	{
		for (int i=0;i<numEngMats;i++)
		{
			mesh = REND->get_instance_mesh(eng_id[i]);
			if (mesh)
			{
				mat = &mesh->material_list[matID[i]];

				mat->emission.r = glow;
				mat->emission.g = glow;
				mat->emission.b = glow;
			}
		}
	}
}
#endif
//---------------------------------------------------------------------------
//
template <class Base> 
void ObjectGlow< Base >::glowPostRender (void)
{
	if (bExploding == 0 && bCloaked == 0 && bSpecialRender == 0 && ( (!billboardThreshhold) || 
		((OBJLIST->GetShipsToRender() <= billboardThreshhold * CQEFFECTS.nFlatShipScale  ) || CQEFFECTS.nFlatShipScale == 4)
		))
	{
	//	GDummy();
		SINGLE dt = OBJLIST->GetRealRenderTime();
		if (areThrustersOn())
		{
			glow+=0.3*dt;
			if (glow > 1.0f)
				glow = 1.0f;
		}
		else
		{
			glow -= dt*0.15f;
			if (glow < 0.0f)
				glow = 0.0f;
		}
		
		if (glow >0.01)
		{
			//	BATCH->set_render_state(D3DRS_TEXTUREMAPBLEND,D3DTBLEND_MODULATEALPHA);
			//BATCH->set_texture_stage_texture( 0,engineTex);
			BATCH->set_state(RPR_BATCH,TRUE);
			BATCH->set_render_state(D3DRS_ZENABLE,TRUE);
			BATCH->set_render_state(D3DRS_ZWRITEENABLE,FALSE);
			SetupDiffuseBlend(engineTex,TRUE);
			//	BATCH->set_state(RPR_STATE_ID,engineTex);
			BATCH->set_render_state(D3DRS_ALPHABLENDENABLE,TRUE);
			BATCH->set_render_state(D3DRS_SRCBLEND,D3DBLEND_ONE);
			BATCH->set_render_state(D3DRS_DESTBLEND,D3DBLEND_ONE);
			
			int tech = techLevel.engine;
			
			CAMERA->SetModelView();
			PB.Color3ub(r*glow,g*glow,b*glow);
			
			//	PB.Begin(PB_QUADS);
			for (int c=0;c<MAX_ENGINE_GLOWS;c++)
			{
				if (glowSize[c])
				{
					Vector cpos (CAMERA->GetPosition());
					
					Vector look (transform*glowPt[c] - cpos);
					
					Vector worldDir;// (look.y, -look.x, 0);
					worldDir = transform.rotate(glowDir[c]);
					
					/*#define TOLERANCE 1e-5
					if (fabs (i.x) < TOLERANCE && fabs (i.y) < TOLERANCE)
					{
					i.x = 1.0f;
				}*/
					
					worldDir.normalize ();
					
					Vector k (-look);
					k.normalize ();
					Vector j (cross_product (k, worldDir));
					j.normalize();
					Vector i = cross_product (j,k);
					
					TRANSFORM trans;
					trans.set_i(i);
					trans.set_j(j);
					trans.set_k(k);
					
					Vector p[4],p2[4];
					
					SINGLE size = glowSize[c]*glow;
					
					SINGLE u0,v0,u1,v1;
					u0 = 0.5;
					u1 = 1.0;
					v0 = 0;
					v1 = 1;
					
					trans.translation = transform*glowPt[c];
					
					//long part
					p[0].set(0,-size,0);
					p[1].set(size*(1+0.75*tech),-size,0);
					p[2].set(size*(1+0.75*tech),size,0);
					p[3].set(0,size,0);
					
					p2[0] = trans*p[0];
					p2[1] = trans*p[1];
					p2[2] = trans*p[2];
					p2[3] = trans*p[3];
					
					BATCH->set_render_state(D3DRS_ZENABLE,TRUE);
					BATCH->set_state(RPR_STATE_ID,engineTex);
					PB.Begin(PB_QUADS);
					PB.TexCoord2f (u0, v0);
					PB.Vertex3f (p2[0].x, p2[0].y, p2[0].z);
					PB.TexCoord2f (u1, v0);
					PB.Vertex3f (p2[1].x, p2[1].y, p2[1].z);
					PB.TexCoord2f (u1, v1);
					PB.Vertex3f (p2[2].x, p2[2].y, p2[2].z);
					PB.TexCoord2f (u0, v1);
					PB.Vertex3f (p2[3].x, p2[3].y, p2[3].z);
					PB.End();
					
					
					k = -worldDir;
					i = cross_product (j,k);
					
					if (dot_product(worldDir,look) < 0)
					{
						//we need this one to be FALSE, but we need to check for face occlusion like FL?
						//BATCH->set_render_state(D3DRS_ZENABLE,FALSE);
					}
					else
					{
						j= -j;
					}
					
					i = cross_product (j,k);
					
					trans.set_i(i);
					trans.set_j(j);
					trans.set_k(k);
					
					//short part
					p[1].set(size,-size,0);
					p[2].set(size,size,0);
					
					p2[0] = trans*p[0];
					p2[1] = trans*p[1];
					p2[2] = trans*p[2];
					p2[3] = trans*p[3];
					
					BATCH->set_state(RPR_STATE_ID,engineTex+1);
					PB.Begin(PB_QUADS);
					PB.TexCoord2f (u0, v0);
					PB.Vertex3f (p2[0].x, p2[0].y, p2[0].z);
					PB.TexCoord2f (u1, v0);
					PB.Vertex3f (p2[1].x, p2[1].y, p2[1].z);
					PB.TexCoord2f (u1, v1);
					PB.Vertex3f (p2[2].x, p2[2].y, p2[2].z);
					PB.TexCoord2f (u0, v1);
					PB.Vertex3f (p2[3].x, p2[3].y, p2[3].z);
					PB.End();
				}
			}
		}
		BATCH->set_state(RPR_STATE_ID,0);
	}
}
//---------------------------------------------------------------------------
//---------------------------End TObjGlow.h---------------------------------
//---------------------------------------------------------------------------
#endif