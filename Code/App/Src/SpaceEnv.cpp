//--------------------------------------------------------------------------//
//                                                                          //
//                               SpaceEnv.cpp                               //
//                                                                          //
//                  COPYRIGHT (C) 1997 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*

    $Author: Tmauer $

    $Header: /Conquest/App/Src/SpaceEnv.cpp 68    11/01/01 12:46p Tmauer $
*/			    
//--------------------------------------------------------------------------//

#include "pch.h"
#include <globals.h>

#include "Menu.h"
#include "Resource.h"
//#include "SysMap.h"
#include "Camera.h"
#include "Startup.h"
#include "Sector.h"
#include "CQTrace.h"
#include "TManager.h"
#include "IBackground.h"
#include "MyVertex.h"
#include <DEffectOpts.h>
#include "IVertexBuffer.h"

#include <3DMath.h>
#include <TComponent.h>
#include <FileSys.h>
#include <TSmartPointer.h>
#include <Heapobj.h>
#include <EventSys.h>
#include <IConnection.h>
//#include <RPUL\PrimitiveBuilder.h>
#include <renderer.h>
#include <mesh.h>

#include <malloc.h>
#include <stdlib.h>

static U32 BACKGROUNDTEXMEMUSED=0;
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
#define ND 10
#define SW 0.75e4
#define LEFT -1e4
#define NEBDEPTH -8e3

static const SINGLE textureFloats [4] = { 0.0, 0.25, 0.5, 0.75 };
//static void Render3dbNoTex(Mesh *mesh,struct ColorRGB *color);

const char *nebNames[4] = {
	"nebula.3db",
	"nebula2.3db",
	"nebula3.3db",
	"nebula4.3db"
};

struct ColorRGB
{
	U8 r,g,b;
};

#define MAX_VERTS 100
#define WHIT_SCALE 10

#define MAX_BG_CHILD 4

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//
struct DACOM_NO_VTABLE SpaceEnvironment : public IEventCallback, IBackground, IVertexBufferOwner
{
	unsigned int textureID;
	U32 	multiStages;

	U32 vb_handle[MAX_BG_CHILD];
	S32 vb_triangles[MAX_BG_CHILD];
	S32 vb_max_triangles[MAX_BG_CHILD];
	Mesh * vbMesh[MAX_BG_CHILD];

	U32 eventHandle;

	BOOL32 bCameraMoved;

	ARCHETYPE_INDEX nebShapeArcheIndex[MAX_SYSTEMS];
//	ColorRGB * nebColor[MAX_SYSTEMS];
	Mesh * nebMesh[MAX_SYSTEMS];
	BOOL32 bHasFocus;
		
	//
	// incoming interface map
	//
  
	BEGIN_DACOM_MAP_INBOUND(SpaceEnvironment)
	DACOM_INTERFACE_ENTRY(IEventCallback)
	DACOM_INTERFACE_ENTRY(IBackground)
	END_DACOM_MAP()

    void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}
	
	SpaceEnvironment (void)
	{
		vb_mgr->Add(this);
	}

	~SpaceEnvironment (void);
	
	void __stdcall RenderNeb (void);

	IDAComponent * getBase (void)
	{
		return static_cast<IEventCallback *>(this);
	}

	void Render3db(Mesh *mesh);

	void TestVertexBuffer(Mesh * mesh, U32 fg_cnt);

	void sortMesh(Mesh *mesh);

	/* IBackground methods */

	void LoadBackground (char * filename,U32 systemID);
	
	/* IEventCallback methods */

	DEFMETHOD(Notify) (U32 message, void *param);

	void cleanUp (void);

	virtual void RestoreVertexBuffers();
};

void SpaceEnvironment::RestoreVertexBuffers()
{
	for(U32 i = 0; i < MAX_BG_CHILD; ++ i)
	{
		if(vb_handle[i])
		{
			PIPE->destroy_vertex_buffer(vb_handle[i]);
			vb_handle[i] = 0;
		}
	}
}
//------------------------------------------------------------------------
//------------------------------------------------------------------------
//
SpaceEnvironment::~SpaceEnvironment (void)
{
/*	for (int i=0;i<MAX_SYSTEMS;i++)
	{
		free (nebColor[i]);
		nebColor[i] = 0;
	}*/
	vb_mgr->Delete(this);
	TMANAGER->ReleaseTextureRef(textureID);
	textureID = 0;
	
	if (GS)
	{
		COMPTR<IDAConnectionPoint> connection;
		
		if (GS->QueryOutgoingInterface("IEventCallback", connection) == GR_OK)
			connection->Unadvise(eventHandle);
	}

	for (int i=0;i<MAX_SYSTEMS;i++)
	{
		if(nebShapeArcheIndex[i])
		{
			ENGINE->release_archetype(nebShapeArcheIndex[i]);
			nebShapeArcheIndex[i] = 0;
		}
	}
}
void SpaceEnvironment::RenderNeb (void)
{
	if (CQRENDERFLAGS.bSoftwareRenderer)
		return;
	if (CQEFFECTS.bBackground==0)
		return;

	static SINGLE Z_DEPTH = -25000;
	static SINGLE SIDE_STRETCH_ADD = 170000;
	static SINGLE SIDE_STRETCH_DIV = 2300;
	static SINGLE Z_STRETCH = 60;

	PANE * pane = CAMERA->GetPane();
	PIPE->set_render_state(D3DRS_ZENABLE,FALSE);

	PIPE->set_viewport(pane->x0,pane->y0,pane->x1-pane->x0+1,pane->y1-pane->y0+1);
	PIPE->set_perspective(17,1.8, 1e3,1e6);
	const Transform *to_view = CAMERA->GetInverseTransform();
	Transform bob;
	bob = *to_view;
	RECT rect;
	SECTOR->GetSystemRect(SECTOR->GetCurrentSystem(),&rect);

	SINGLE width = (rect.right-rect.left);
	SINGLE halfwidth = width /2;
	Transform nebTrans(Vector(halfwidth,halfwidth,Z_DEPTH));//Vector(30000,30000,-25000));
	nebTrans.d[0][0] = nebTrans.d[1][1] = ((width+SIDE_STRETCH_ADD)/SIDE_STRETCH_DIV); //60;//WHIT_SCALE;
	nebTrans.d[2][2] = Z_STRETCH;
	BATCH->set_state(RPR_BATCH,false);
	Transform mv((bob)*nebTrans);
	SINGLE s = 1/(mv.d[2][2]);
	mv.scale(s);
	mv.translation *= s;
	BATCH->set_modelview(mv);


	U32 systemID = SECTOR->GetCurrentSystem()-1;
	if (nebMesh[systemID])
		Render3db(nebMesh[systemID]);
}

#if 0
static void Render3dbNoTex(Mesh *mesh,ColorRGB *color)
{
	CQASSERT(mesh);

	PIPE->set_render_state(D3DRS_ZENABLE,FALSE);
	PIPE->set_render_state(D3DRS_ZWRITEENABLE,FALSE);
	PIPE->set_render_state(D3DRS_CULLMODE,D3DCULL_NONE);

	DisableTextures();
	//
	PIPE->set_render_state(D3DRS_ALPHABLENDENABLE,FALSE);
	PIPE->set_render_state(D3DRS_DITHERENABLE,TRUE);

	PB.Begin(PB_TRIANGLES);
	int i;
	for (i=0;i<mesh->face_cnt;i++)
	{
		U32 ref[3];
		FaceGroup *fg = mesh->face_groups;
		ref[0] = mesh->vertex_batch_list[fg->face_vertex_chain[i*3]];
		ref[1] = mesh->vertex_batch_list[fg->face_vertex_chain[i*3+1]];
		ref[2] = mesh->vertex_batch_list[fg->face_vertex_chain[i*3+2]];

		Vector v0,v1,v2;
		v0 = mesh->object_vertex_list[ref[0]];
		v1 = mesh->object_vertex_list[ref[1]];
		v2 = mesh->object_vertex_list[ref[2]];

		PB.Color3ub(color[ref[0]].r,color[ref[0]].g,color[ref[0]].b);   PB.Vertex3f(v0.x,v0.y,v0.z);
		PB.Color3ub(color[ref[1]].r,color[ref[1]].g,color[ref[1]].b);   PB.Vertex3f(v1.x,v1.y,v1.z);
		PB.Color3ub(color[ref[2]].r,color[ref[2]].g,color[ref[2]].b);   PB.Vertex3f(v2.x,v2.y,v2.z);

	}
	PB.End();
}
#endif

void SpaceEnvironment::Render3db(Mesh *mesh)
{
	CQASSERT(mesh);
	
	BATCH->set_state(RPR_BATCH,false);
	BATCH->set_render_state(D3DRS_ZENABLE,FALSE);
	BATCH->set_render_state(D3DRS_ZWRITEENABLE,FALSE);
	BATCH->set_render_state(D3DRS_CULLMODE,D3DCULL_NONE);
	
	BATCH->set_render_state(D3DRS_ALPHABLENDENABLE,TRUE);
	BATCH->set_render_state(D3DRS_ALPHATESTENABLE,TRUE);
	BATCH->set_render_state(D3DRS_ALPHAREF,8);
	BATCH->set_render_state(D3DRS_SRCBLEND,D3DBLEND_SRCALPHA);
	BATCH->set_render_state(D3DRS_DESTBLEND,D3DBLEND_INVSRCALPHA);
	//dither is very valuable for art that has dark colors
	BATCH->set_render_state(D3DRS_DITHERENABLE,TRUE);	
	
	for (int fg_cnt=0;fg_cnt<mesh->face_group_cnt;fg_cnt++)
	{
		FaceGroup *fg = &mesh->face_groups[fg_cnt];

/*		if(CQEFFECTS.bHighBackground)
		{
			if(multiStages == 1 || multiStages == 0xffffffff)
			{
			//	BATCH->set_state(RPR_BATCH,false);
				BATCH->set_texture_stage_texture( 0, mesh->material_list[fg->material].texture_id );
				BATCH->set_texture_stage_texture( 1, textureID );
				
				// blending - This is the same as D3DTMAPBLEND_MODULATEALPHA
				BATCH->set_texture_stage_state( 0, D3DTSS_COLOROP, D3DTOP_SELECTARG1 );
				BATCH->set_texture_stage_state( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE  );
				BATCH->set_texture_stage_state( 0, D3DTSS_COLORARG2, D3DTA_DIFFUSE );
				BATCH->set_texture_stage_state( 0, D3DTSS_ALPHAOP, D3DTOP_DISABLE );
				BATCH->set_texture_stage_state( 0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE );
				BATCH->set_texture_stage_state( 0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE );
							
				// addressing - clamped
				BATCH->set_sampler_state( 0, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP );
				BATCH->set_sampler_state( 0, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP );
				
				BATCH->set_texture_stage_state( 1, D3DTSS_TEXCOORDINDEX, 1);
				// blending - This is the same as D3DTMAPBLEND_MODULATEALPHA
				BATCH->set_texture_stage_state( 1, D3DTSS_COLOROP, D3DTOP_ADD );
				BATCH->set_texture_stage_state( 1, D3DTSS_COLORARG1, D3DTA_TEXTURE );
				BATCH->set_texture_stage_state( 1, D3DTSS_COLORARG2, D3DTA_CURRENT);
				BATCH->set_texture_stage_state( 1, D3DTSS_ALPHAOP, D3DTOP_DISABLE );
				BATCH->set_texture_stage_state( 1, D3DTSS_ALPHAARG1, D3DTA_CURRENT );
				BATCH->set_texture_stage_state( 1, D3DTSS_ALPHAARG2, D3DTA_CURRENT);
							
				// addressing - clamped
				BATCH->set_sampler_state( 1, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP );
				BATCH->set_sampler_state( 1, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP );
			}
			
			if (multiStages == 0xffffffff)
			{
				multiStages = (BATCH->verify_state() == GR_OK);
			}
			
			if (multiStages == 1)
			{
				bRendered = true;
				
				TestVertexBuffer(mesh,fg_cnt);

				U32 result = PIPE->draw_primitive_vb( D3DPT_TRIANGLELIST, vb_handle[fg_cnt], 0, vb_triangles[fg_cnt]*3, 0 );
				CQASSERT(result == GR_OK);
			}
		}
*/	
		SetupDiffuseBlend(mesh->material_list[fg->material].texture_id,TRUE);
		BATCH->set_texture_stage_state(0,D3DTSS_COLOROP,D3DTOP_SELECTARG1);
		BATCH->set_texture_stage_state(0,D3DTSS_ALPHAOP,D3DTOP_SELECTARG1 );
		BATCH->set_texture_stage_state(0,D3DTSS_COLORARG1,D3DTA_TEXTURE);
		BATCH->set_texture_stage_state(0,D3DTSS_ALPHAARG1, D3DTA_TEXTURE);

		PB.Begin(PB_TRIANGLES);
		int i;
		for (i=0;i<fg->face_cnt;i++)
		{
			U32 ref[3],tref[3];
			
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
			
			PB.Color3ub(255,255,255);
			PB.TexCoord2f(mesh->texture_vertex_list[tref[0]].u,mesh->texture_vertex_list[tref[0]].v);
			PB.Vertex3f(v0.x,v0.y,v0.z);
			PB.TexCoord2f(mesh->texture_vertex_list[tref[1]].u,mesh->texture_vertex_list[tref[1]].v);
			PB.Vertex3f(v1.x,v1.y,v1.z);
			PB.TexCoord2f(mesh->texture_vertex_list[tref[2]].u,mesh->texture_vertex_list[tref[2]].v);
			PB.Vertex3f(v2.x,v2.y,v2.z);
			
		}
		PB.End();
	}
}
//--------------------------------------------------------------------------//
//
void SpaceEnvironment::TestVertexBuffer(Mesh * mesh, U32 fg_cnt)
{
	if(!(vb_handle[fg_cnt]) || vb_max_triangles[fg_cnt] < mesh->face_groups[fg_cnt].face_cnt)
	{
		if(vb_handle[fg_cnt])
		{
			PIPE->destroy_vertex_buffer(vb_handle[fg_cnt]);
			vb_handle[fg_cnt] = 0;
		}
		vbMesh[fg_cnt] = NULL;
		U32 flags = 0;
		if (CQRENDERFLAGS.bHardwareGeometry==0)
			flags = IRP_VBF_SYSTEM;
		U32 result = PIPE->create_vertex_buffer( D3DFVF_RPVERTEX2, mesh->face_groups[fg_cnt].face_cnt*3, flags, &(vb_handle[fg_cnt]) );
		vb_max_triangles[fg_cnt] = mesh->face_groups[fg_cnt].face_cnt;
		CQASSERT(result == GR_OK);
	}

	if(vbMesh[fg_cnt] != mesh)
	{
		vbMesh[fg_cnt] = mesh;
		vb_triangles[fg_cnt] = mesh->face_groups[fg_cnt].face_cnt;
		Vertex2 *vb_data;
		U32 dwSize;
		U32 result = PIPE->lock_vertex_buffer( vb_handle[fg_cnt], DDLOCK_WRITEONLY |DDLOCK_DISCARDCONTENTS, (void **)&vb_data, &dwSize );
		CQASSERT(result == GR_OK);
		FaceGroup *fg = &mesh->face_groups[fg_cnt];

#define TEX_SCALE 0.008
		for (S32 i=0;i<fg->face_cnt;i++)
		{
			U32 ref[3],tref[3];
				
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
			
			S32 vc = i*3;
			vb_data[vc].pos = v0;
			vb_data[vc].color = 0x00C8C8C8;
			vb_data[vc].u = mesh->texture_vertex_list[tref[0]].u;
			vb_data[vc].v = mesh->texture_vertex_list[tref[0]].v;
			vb_data[vc].u2 = v0.x*TEX_SCALE;
			vb_data[vc].v2 = v0.y*TEX_SCALE;

			vc++;
			vb_data[vc].pos = v1;
			vb_data[vc].color = 0x00C8C8C8;
			vb_data[vc].u = mesh->texture_vertex_list[tref[1]].u;
			vb_data[vc].v = mesh->texture_vertex_list[tref[1]].v;
			vb_data[vc].u2 = v1.x*TEX_SCALE;
			vb_data[vc].v2 = v1.y*TEX_SCALE;

			vc++;
			vb_data[vc].pos = v2;
			vb_data[vc].color = 0x00C8C8C8;
			vb_data[vc].u = mesh->texture_vertex_list[tref[2]].u;
			vb_data[vc].v = mesh->texture_vertex_list[tref[2]].v;
			vb_data[vc].u2 = v2.x*TEX_SCALE;
			vb_data[vc].v2 = v2.y*TEX_SCALE;
		}
		result = PIPE->unlock_vertex_buffer( vb_handle[fg_cnt] );
		CQASSERT(result == GR_OK);	
	}
}
//--------------------------------------------------------------------------//
//
Mesh * sortMeshPtr = NULL;
FaceGroup *sortFG = NULL;

int faceZComp( const void *arg1, const void *arg2 )
{
	int * index1 = (int*)arg1;
	int * index2 = (int*)arg2;
	SINGLE z1_0 = sortMeshPtr->object_vertex_list[sortMeshPtr->vertex_batch_list[index1[0]]].z;
	SINGLE z1_1 = sortMeshPtr->object_vertex_list[sortMeshPtr->vertex_batch_list[index1[1]]].z;
	SINGLE z1_2 = sortMeshPtr->object_vertex_list[sortMeshPtr->vertex_batch_list[index1[2]]].z;
	SINGLE z1 = __min(z1_0,z1_1);
	z1 = __min(z1,z1_2);

	SINGLE z2_0 = sortMeshPtr->object_vertex_list[sortMeshPtr->vertex_batch_list[index2[0]]].z;
	SINGLE z2_1 = sortMeshPtr->object_vertex_list[sortMeshPtr->vertex_batch_list[index2[1]]].z;
	SINGLE z2_2 = sortMeshPtr->object_vertex_list[sortMeshPtr->vertex_batch_list[index2[2]]].z;
	SINGLE z2 = __min(z2_0,z2_1);
	z2 = __min(z2,z2_2);
	if(z1 < z2)
		return -1;
	else if(z1 == z2)
		return 0;
	return 1;
}
//--------------------------------------------------------------------------//
//
int groupZComp( const void *arg1, const void *arg2 )
{
	FaceGroup * fg1 = (FaceGroup *)arg1;
	FaceGroup * fg2 = (FaceGroup *)arg2;
	SINGLE fg1Z = 0;
	bool bHasZ = false;
	for (int i=0;i<fg1->face_cnt;i++)
	{
		U32 ref[3];
		
		ref[0] = sortMeshPtr->vertex_batch_list[fg1->face_vertex_chain[i*3]];
		ref[1] = sortMeshPtr->vertex_batch_list[fg1->face_vertex_chain[i*3+1]];
		ref[2] = sortMeshPtr->vertex_batch_list[fg1->face_vertex_chain[i*3+2]];
		
		SINGLE z0,z1,z2;
		z0 = sortMeshPtr->object_vertex_list[ref[0]].z;
		z1 = sortMeshPtr->object_vertex_list[ref[1]].z;
		z2 = sortMeshPtr->object_vertex_list[ref[2]].z;
		SINGLE minZ = __min(z0,z1);
		minZ = __min(minZ,z2);
		
		if(bHasZ)
		{
			if(minZ < fg1Z)
				fg1Z = minZ;
		}
		else
		{
			fg1Z = minZ;
			bHasZ = true;
		}
	}
	SINGLE fg2Z = 0;
	bHasZ = false;
	for (int i=0;i<fg2->face_cnt;i++)
	{
		U32 ref[3];
		
		ref[0] = sortMeshPtr->vertex_batch_list[fg2->face_vertex_chain[i*3]];
		ref[1] = sortMeshPtr->vertex_batch_list[fg2->face_vertex_chain[i*3+1]];
		ref[2] = sortMeshPtr->vertex_batch_list[fg2->face_vertex_chain[i*3+2]];
		
		SINGLE z0,z1,z2;
		z0 = sortMeshPtr->object_vertex_list[ref[0]].z;
		z1 = sortMeshPtr->object_vertex_list[ref[1]].z;
		z2 = sortMeshPtr->object_vertex_list[ref[2]].z;
		SINGLE minZ = __min(z0,z1);
		minZ = __min(minZ,z2);
		
		if(bHasZ)
		{
			if(minZ < fg2Z)
				fg2Z = minZ;
		}
		else
		{
			fg2Z = minZ;
			bHasZ = true;
		}
	}
	if(fg1Z < fg2Z)
		return -1;
	else if(fg1Z == fg2Z)
		return 0;
	return 1;
}
//--------------------------------------------------------------------------//
//
void SpaceEnvironment::sortMesh(Mesh *mesh)
{
	sortMeshPtr = mesh;
	//this will sort the face groups based on the Z value of the faces
	for (int fg_cnt=0;fg_cnt<mesh->face_group_cnt;fg_cnt++)
	{
		FaceGroup *fg = &mesh->face_groups[fg_cnt];
		sortFG = fg;
		qsort(fg->face_vertex_chain,fg->face_cnt,sizeof(int)*3,faceZComp);
	}
	qsort(mesh->face_groups,mesh->face_group_cnt,sizeof(FaceGroup),groupZComp);
}
//--------------------------------------------------------------------------//
//
void SpaceEnvironment::LoadBackground(char * filename,U32 systemID)
{
	if (CQRENDERFLAGS.bSoftwareRenderer)
		return;
	if (CQEFFECTS.bExpensiveTerrain==0)
		return;

	int lastTexMem=TEXMEMORYUSED;
	systemID -= 1;
	if(filename[0])
	{
		DAFILEDESC fdesc = filename;
		COMPTR<IFileSystem> file;
				
		if (OBJECTDIR->CreateInstance(&fdesc, file) == GR_OK)
		{
			if(nebShapeArcheIndex[systemID])
			{
				ENGINE->release_archetype(nebShapeArcheIndex[systemID]);
				nebShapeArcheIndex[systemID] = 0;
				nebMesh[systemID] = NULL;
			/*	if (nebColor[systemID])
					free(nebColor[systemID]);
				nebColor[systemID] = 0;*/
			}
			if ((nebShapeArcheIndex[systemID] = ENGINE->create_archetype(filename, file)) != INVALID_ARCHETYPE_INDEX)
			{
				nebMesh[systemID] = REND->get_archetype_mesh(nebShapeArcheIndex[systemID]);
				CQASSERT(nebMesh[systemID]);
				if(nebMesh[systemID])
					sortMesh(nebMesh[systemID]);
				/*if (nebColor[systemID])
					free(nebColor[systemID]);
				nebColor[systemID] = (ColorRGB *)calloc(nebMesh[systemID]->object_vertex_cnt,sizeof(ColorRGB));
				memset(nebColor[systemID],0xff,nebMesh[systemID]->object_vertex_cnt*sizeof(ColorRGB));*/
			}
		}
		else
			CQFILENOTFOUND(fdesc.lpFileName);
	}
	else
	{
		if(nebShapeArcheIndex[systemID])
		{
			ENGINE->release_archetype(nebShapeArcheIndex[systemID]);
			nebShapeArcheIndex[systemID] = 0;
			nebMesh[systemID] = NULL;
		/*	if (nebColor[systemID])
				free(nebColor[systemID]);
			nebColor[systemID] = 0;*/
		}
	}
	BACKGROUNDTEXMEMUSED += (TEXMEMORYUSED-lastTexMem);
}
//--------------------------------------------------------------------------//
// receive notifications from event system
//
GENRESULT SpaceEnvironment::Notify (U32 message, void *param)
{
//	MSG *msg = (MSG *) param;

	switch (message)
	{
	case CQE_ENTERING_INGAMEMODE:
		textureID = TMANAGER->CreateTextureFromFile("StarBk.tga", TEXTURESDIR, DA::TGA, PF_RGB5_A1);
		if (CQRENDERFLAGS.bMultiTexture)
			multiStages = 0xffffffff;
		else
			multiStages = 0;
		break;
	case CQE_LEAVING_INGAMEMODE:
		for(U32 i = 0; i < MAX_BG_CHILD; ++ i)
		{
			if(vb_handle[i])
			{
				PIPE->destroy_vertex_buffer(vb_handle[i]);
				vb_handle[i] = 0;
			}
		}
		TMANAGER->ReleaseTextureRef(textureID);
		textureID = 0;
		for (U32 i=0;i<MAX_SYSTEMS;i++)
		{
			if(nebShapeArcheIndex[i])
			{
				ENGINE->release_archetype(nebShapeArcheIndex[i]);
				nebShapeArcheIndex[i] = 0;
			}
		}
		break;

	case CQE_CAMERA_MOVED:
		bCameraMoved = 1;
		break;

	case CQE_KILL_FOCUS:
		bHasFocus = 0;
		break;

	case CQE_SET_FOCUS:
		bHasFocus = 1;
		break;

	case CQE_MISSION_CLOSE:
		cleanUp();
		break;
	}

	return GR_OK;
}
//----------------------------------------------------------------------------------------------
//
void SpaceEnvironment::cleanUp (void)
{
	for (U32 i=0;i<MAX_SYSTEMS;i++)
	{
		if(nebShapeArcheIndex[i])
		{
			ENGINE->release_archetype(nebShapeArcheIndex[i]);
			nebShapeArcheIndex[i] = 0;
			nebMesh[i] = NULL;
		}
		/*if (nebColor[i])
			free(nebColor[i]);
		nebColor[i] = 0;*/
	}
}
//----------------------------------------------------------------------------------------------
//
struct _stars : GlobalComponent
{
	SpaceEnvironment *spaceEnv;

	virtual void Startup (void)
	{
		BACKGROUND = spaceEnv = new DAComponent<SpaceEnvironment>;
		AddToGlobalCleanupList(&BACKGROUND);
	}

	virtual void Initialize (void)
	{
		COMPTR<IDAConnectionPoint> connection;

		if (GS->QueryOutgoingInterface("IEventCallback", connection) == GR_OK)
			connection->Advise(spaceEnv->getBase(), &spaceEnv->eventHandle);
	}
};

static _stars stars;

//--------------------------------------------------------------------------//
//----------------------------END SpaceEnv.cpp------------------------------//
//--------------------------------------------------------------------------//
