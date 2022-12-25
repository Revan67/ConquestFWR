//--------------------------------------------------------------------------//
//                                                                          //
//                             MeshRender.cpp                               //
//                                                                          //
//                  COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/MeshRender.cpp 105   10/22/01 2:17p Tmauer $
*/			    
//------------------------------- #INCLUDES --------------------------------//
//--------------------------------------------------------------------------//

#include "pch.h"
#include <globals.h>

#include <stdio.h>
 
#include "MeshRender.h"
#include "IExplosion.h"//MeshInfo.h?
#include "Camera.h"
#include "CQBatch.h"
#include "Archholder.h"
#include "GridVector.h"
#include "Objlist.h"
#include <DEffectOpts.h>
#include "CQLight.h"
#include "IVertexBuffer.h"

#include <Renderer.h>
#include <ICamera.h>
#include <LightMan.h>

#include "TManager.h"

#include "iMaterialManager.h"


inline DWORD F2DW( FLOAT f ) { return *((DWORD*)&f); }

static DWORD colorMod = 0xFFFFFFFF;

struct SftVertex
{
	Vector			pos;
	SINGLE dummy;
	union {
		struct {
		unsigned char	b, g, r, a;
		};
		unsigned int color;
	};
	unsigned int specular;
	float			u,v;
};

struct VBVertex
{
	Vector			pos;
	Vector			norm;
	float			u,v;
	float			u2,v2;
	float			tanX, tanY;
	float			binX, binY;
	float			tanZ, binZ;
};

//#define D3DFVF_VBVERTEX (D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_TEX4 | D3DFVF_TEXCOORDSIZE2(0) | D3DFVF_TEXCOORDSIZE2(1) | D3DFVF_TEXCOORDSIZE3(2) | D3DFVF_TEXCOORDSIZE3(3) )
#define D3DFVF_VBVERTEX (D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_TEX5)

//global for fun
Vector cam_pos;
Vector cam_pos_in_object_space;
//static LightRGB lit[HUGE];
/*struct LightRGBA
{
	int r,g,b,a;
};
LightRGBA *lit;*/

#define HUGE 5000

VBVertex tmp_VBVertex[HUGE];
struct LightRGB_U8 lit[HUGE];
FaceGroupInfo *fgi;
MeshInfo *active_mi;
RPVertex tmp_verts1[HUGE];
float tmp_float1[HUGE];
float tmp_float2[HUGE];
Vertex2 tmp_verts2[HUGE];
U16 indexScratchList[HUGE];
Vector tmp_n[HUGE];
Vector tmp_v[HUGE];
Vector tmp_v1[HUGE/2];
int tmp_v_cnt,tmp_v_cnt1;
Vector dest_vec[HUGE];
Vector dest_norm[HUGE];
U16 re_index_list[HUGE];
bool bCacheValid=false;
bool bScaleTrans;
Transform scaleTrans;
int unique = 1;
//Transform inv;
SINGLE puffPercent;
Vector puffViewPt[2];
U32 puffTexture;
SINGLE chromeRot=0.0f;
TRANSFORM chromeTrans;
bool bChromeDirty=true;
U8 chosenAngle;

//U32 testLightCurves = 0;

ID3DXEffect** emissiveEffect = 0;
ID3DXEffect** nonEmissiveEffect = 0;

extern S32 camMoved;

FaceGroupInfo::~FaceGroupInfo()
	{
//		if (colorTex) TMANAGER->ReleaseTextureRef(colorTex);
//		if (bumpTex) TMANAGER->ReleaseTextureRef(bumpTex);
//		if (specTex) TMANAGER->ReleaseTextureRef(specTex);
//		if (glowTex) TMANAGER->ReleaseTextureRef(glowTex);
	}

D3DXMATRIX* getEnvMatrix()
	{
		static SINGLE lastRot = 999;	
		static D3DXMATRIX envMat;
		SINGLE worldRot = CAMERA->GetWorldRotation();

		// if camera pitch becomes changeable, we need to test against that also
		if (worldRot != lastRot)
		{
			lastRot = worldRot;
			SINGLE pitch, roll, yaw;
			CAMERA->GetOrientation(&pitch,&roll,&yaw);
			D3DXMATRIX matTrans;
			D3DXMATRIX matTmp;
			D3DXMatrixIdentity(&matTrans);
			D3DXMatrixRotationX(&matTmp, 3.14159 * -pitch / 180.0); 
			D3DXMatrixMultiply(&matTrans, &matTrans, &matTmp);
			D3DXMatrixRotationY(&matTmp, 3.14159 * (worldRot - 180.0) / 180.0);
			D3DXMatrixMultiply(&matTrans, &matTrans, &matTmp);
			D3DXMatrixRotationZ(&matTmp, 3.14159 * -.1); 
			D3DXMatrixMultiply(&matTrans, &matTrans, &matTmp);
			D3DXMatrixScaling(&matTmp, .5,-.5,.5);
			D3DXMatrixMultiply(&matTrans, &matTrans, &matTmp);
			D3DXMatrixTranspose(&envMat, &matTrans);
		}
		return &envMat;
	}

D3DXMATRIX* getSpecMatrix()
	{
		static SINGLE lastRot = 999;	
		static D3DXMATRIX envMat;
		SINGLE worldRot = CAMERA->GetWorldRotation();

		// if camera pitch becomes changeable, we need to test against that also
		if (worldRot != lastRot)
		{
			lastRot = worldRot;
			SINGLE pitch, roll, yaw;
			CAMERA->GetOrientation(&pitch,&roll,&yaw);
			D3DXMATRIX matTrans;
			D3DXMATRIX matTmp;
			D3DXMatrixIdentity(&matTrans);

			D3DXMatrixRotationX(&matTmp, 3.14159 * -pitch / 180.0); 
			D3DXMatrixMultiply(&matTrans, &matTrans, &matTmp);
			D3DXMatrixRotationY(&matTmp, 3.14159 * (worldRot - 90.0) / 180.0);
			D3DXMatrixMultiply(&matTrans, &matTrans, &matTmp);
			D3DXMatrixRotationZ(&matTmp, 3.14159 * -.1); 
			D3DXMatrixMultiply(&matTrans, &matTrans, &matTmp);

			D3DXMatrixScaling(&matTmp, .5,.5,.5);
			D3DXMatrixMultiply(&matTrans, &matTrans, &matTmp);
			D3DXMatrixTranslation(&matTmp, .5,.5,.5);
			D3DXMatrixMultiply(&matTrans, &matTrans, &matTmp);
			

			D3DXMatrixTranspose(&envMat, &matTrans);
		}
		return &envMat;
	}



struct SplitPoly
{
	U16 split_face;
	U16 out_split_face0,out_split_face1;
	U16 in_ref0[3],in_ref1[3];
	U16 out_ref0[4],out_ref1[4];
	U16 in_cnt0,in_cnt1,out_cnt0,out_cnt1;
	SINGLE ratios[2];
};

struct SplitStruct
{
	U16 * tc_re_index0;
	U16 * tc_re_index1;
	struct EffectChannel *out0;
	struct EffectChannel *out1;
	int next_face_ref;

	SplitStruct()
	{
		tc_re_index0 = tc_re_index1 = 0;
	}

	void Reset()
	{
		if (tc_re_index0)
		{
			delete [] tc_re_index0;
			tc_re_index0 = 0;
		}
		if (tc_re_index1)
		{
			delete [] tc_re_index1;
			tc_re_index1 = 0;
		}
	}
};

struct MeshSplit
{
	U8 dest_face_render0[HUGE/2];
	U8 dest_face_render1[HUGE/2];
	U16 out_face_cnt0,out_face_cnt1;

	MeshSplit()
	{
		out_face_cnt0 = out_face_cnt1 = 0;
	}
};

#define MAX_EFFECT_CHANNELS 10
SplitStruct ss[MAX_EFFECT_CHANNELS];
U16 face_index_offset;

struct EffectChannel : IEffectChannel
{
	//EffectChannel *next;

	EffectChannel()
	{
		next = 0;
	}

	~EffectChannel();

	virtual void AddSplitPoly(SplitPoly &sp,SplitStruct &ss);

	void RenderWithTexture(U32 textureID,U32 color,bool bClamp);
};

struct RenderMaterial : IRenderMaterial
{
	U8 *faces_ptr;
	struct MeshRender *mr;
	struct FaceGroup *fg;
	U16 *index_list4[4];
	IDirect3DIndexBuffer9* IB4[4];
	U32 maxVert[4];
	RenderMaterial *next;

	virtual void Clone(RenderMaterial **rm,bool bCopyBuffers = true) = 0;

	//I hereby define split_n to point in the direction of out1
	virtual void SplitFaceGroup (const MeshInfo &src,MeshInfo &out0,MeshInfo &out1,RenderMaterial *fg_out0,RenderMaterial *fg_out1,SINGLE split_d,const Vector &split_n,struct MeshSplit &meshSplit) = 0;

	virtual void Render(const Transform &inv) = 0;

	virtual void RenderPortionZAlign(const Transform &inv,const Vector &split_plane) = 0;

	virtual void RenderPortion(const Transform &inv,const Vector &split_plane,const Vector &split_normal) = 0;

	virtual void RenderPuffy(const Transform &inv,const Transform &local_to_obj,const Vector & scale) = 0;

	virtual void RenderSolarian(const Transform &trans,const Transform &obj_to_view) = 0;

	virtual void setRenderStates() = 0;
//	virtual void RenderFacesWithProperty(U32 textureID,U8 property) = 0;
};

struct MeshRender : IMeshRender, IVertexBufferOwner
{
	bool bSolarianRenderstateKludge;
	bool vertexBuffersBuilt;
	U32 vb_handle;
	U32 vb_handle4[4];
	struct RenderMaterial *fgr;
//	Vector *face_norms[4];
	U32 ref_cnt;

	//IVertexBufferOwner
	void RestoreVertexBuffers();

	//
	void BuildTangentSpace(VBVertex * pVertices, int c);

	void Render (MeshInfo *mc,const Transform &world_to_view);

	void RenderPortionZAlign (MeshInfo *mc,const Transform &world_to_view,const Vector &split_pt);

	void RenderPortion (MeshInfo *mc,const Transform &world_to_view,const Vector &split_pt,const Vector &split_norm);

	void RenderPuffy (MeshInfo *mc,const Transform &world_to_view,const Transform &obj_trans,const Vector &scale);

	void Init (ARCHETYPE_INDEX idx);

	void Init (MeshInfo *mc);

	void SetupMeshInfo (MeshInfo *mc,bool bMakeBuffers=true, const char* materialOverride = 0);

	//virtual void SplitMesh (IRenderMaterial *out0,IRenderMaterial *out1,const Vector &split_d,const Vector &split_n);

	virtual IRenderMaterial * GetNextFaceGroup (IRenderMaterial *irm);
	//create a new MeshInfo struct, not necessarily connected to an engine mesh
//	void MakeMeshInfo(MeshInfo *mc);

	virtual void AddRef()
	{
		ref_cnt++;
	}

	virtual void Release()
	{
		ref_cnt--;
		if (ref_cnt == 0)
			delete this;
	}

	MeshRender();

	~MeshRender();

};

template <class VertexStruct,U32 VERTEX_FORMAT>
struct MMaterial : RenderMaterial
{
	U32 vb_offset;
	U32 hintID;
	
	virtual U32 getStateID() = 0;

	virtual void setRenderStates() = 0;

	virtual void Render(const Transform &inv);

	static void DRender(RenderMaterial *that,const Transform &inv);

	virtual void RenderPortionZAlign(const Transform &inv,const Vector &split_plane);

	virtual void RenderPortion(const Transform &inv,const Vector &split_plane,const Vector &split_normal);

	virtual void RenderPuffy(const Transform &inv,const Transform &local_to_obj,const Vector & scale);

	virtual void RenderSolarian(const Transform &trans,const Transform &obj_to_view);

	virtual void splitEdgeZ(const VertexStruct & v1,const VertexStruct & v2,VertexStruct & v_out,SINGLE z) = 0;
	
	virtual void splitEdge(const VertexStruct & v1,const VertexStruct & v2,VertexStruct & v_out,SINGLE split_d,const Vector &split_n,SINGLE & ratio) = 0;

	virtual void splitEdgePlane(const VertexStruct & v1,const VertexStruct & v2,VertexStruct & v_out,const Vector & planePoint,const Vector &split_n) = 0;
	
	virtual void GetBuffers(void **vertices,U16 **indices,Vector **norms,ArchetypeFaceInfo **_faces,S32 *num_verts,S32 *num_faces);

	//I hereby define split_n to point in the direction of out1
	virtual void SplitFaceGroup (const MeshInfo &src,MeshInfo &out0,MeshInfo &out1,RenderMaterial *fg_out0,RenderMaterial *fg_out1,SINGLE split_d,const Vector &split_n,struct MeshSplit &meshSplit);

	virtual int GetVertexType()
	{
		return VERTEX_FORMAT;
	}

	virtual int GetVertexSize()
	{
		return sizeof(VertexStruct);
	}

	virtual void cleanupRenderStates()
	{
		return;
	}
	

};
  
struct FaceGroupRender : MMaterial<RPVertex,D3DFVF_RPVERTEX>
{
	D3DXMATRIX matFlipY;
	
	void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}

	virtual U32 getStateID();

	virtual void setRenderStates();

	virtual void Render(const Transform &inv);

	virtual void Init(Mesh *mesh,int _fg_idx,MeshRender *_mr);


	virtual void Clone(RenderMaterial **rm,bool bCopyBuffers);

	virtual void splitEdgeZ(const RPVertex & v1,const RPVertex & v2,RPVertex & v_out,SINGLE z);

	virtual void splitEdge(const RPVertex & v1,const RPVertex & v2,RPVertex & v_out,SINGLE split_d,const Vector &split_n,SINGLE &ratio);

	virtual void splitEdgePlane(const RPVertex & v1,const RPVertex & v2,RPVertex & v_out,const Vector & planePoint,const Vector &split_n);

	void sortVertices(int c,U16 *sort_vert);

	FaceGroupRender();

	~FaceGroupRender();


	virtual void cleanupRenderStates()
	{
			BATCH->set_render_state(D3DRS_SRCBLEND,D3DBLEND_SRCALPHA);
			BATCH->set_render_state(D3DRS_DESTBLEND,D3DBLEND_INVSRCALPHA);
			BATCH->set_render_state(D3DRS_ALPHABLENDENABLE,FALSE);
			BATCH->set_texture_stage_state(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
				for (int i = 0; i < 4; i++)
			{
				PIPE->set_sampler_state( i, D3DSAMP_ADDRESSU, MAT_GET_ADDR_MODE(fgi[fg_idx].texture_flags,0) );
				PIPE->set_sampler_state( i, D3DSAMP_ADDRESSV, MAT_GET_ADDR_MODE(fgi[fg_idx].texture_flags,1) );
				PIPE->set_sampler_state( i, D3DSAMP_MINFILTER,		D3DTEXF_LINEAR );
				PIPE->set_sampler_state( i, D3DSAMP_MAGFILTER,		D3DTEXF_LINEAR );
				PIPE->set_sampler_state( i, D3DSAMP_MIPFILTER,		D3DTEXF_LINEAR);
			}
			PIPE->set_render_state(D3DRS_ALPHATESTENABLE,FALSE);
			PIPE->set_render_state(D3DRS_CULLMODE,D3DCULL_NONE);
			PIPE->set_render_state(D3DRS_ZENABLE,FALSE);
			
			PIPE->set_texture_stage_state( 0, D3DTSS_TEXCOORDINDEX, 0);
			PIPE->set_texture_stage_state( 1, D3DTSS_TEXCOORDINDEX, 1);
			
	}

};

struct SoftwareFGRender : FaceGroupRender
{
	void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}

	virtual void Init(Mesh *mesh,int _fg_idx,MeshRender *_mr);

	virtual void Render(const Transform &inv);

	virtual void Clone(RenderMaterial **rm,bool bCopyBuffers);

	virtual void setRenderStates();

	SoftwareFGRender()
	{
		hintID = U32(-1);
	}
};

struct FaceGroupRender2 : MMaterial<Vertex2,D3DFVF_RPVERTEX2>
{
	void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}

	virtual U32 getStateID();

	virtual void setRenderStates();

	void Init(Mesh *mesh,int _fg_idx,MeshRender *_mr);//MeshInfo *mc);

	virtual void Clone(RenderMaterial **rm,bool bCopyBuffers);

	virtual void splitEdgeZ(const Vertex2 & v1,const Vertex2 & v2,Vertex2 & v_out,SINGLE z);

	virtual void splitEdge(const Vertex2 & v1,const Vertex2 & v2,Vertex2 & v_out,SINGLE split_d,const Vector &split_n,SINGLE &ratio);

	virtual void splitEdgePlane(const Vertex2 & v1,const Vertex2 & v2,Vertex2 & v_out,const Vector & planePoint,const Vector &split_n);
	
	FaceGroupRender2();

	~FaceGroupRender2();
};

struct RenderQueue
{
	//MeshRender *mr;
	MeshInfo *mi;
//	int num_children;
	bool bRendered:1;

};

struct RenderQueue renderQueue[200];
struct RenderQueue alphaQueue[100];
int stack=0,alphaStack=0;

void StackRender(MeshInfo **mi,int num_children)
{
	for (int c=0;c<num_children;c++)
	{
		renderQueue[stack].mi = mi[c];
		renderQueue[stack].bRendered = false;
		stack++;

		if (stack==200)
			EmptyStack();
	}
};

void AlphaStackRender(MeshInfo **mi,int num_children)
{
	for (int c=0;c<num_children;c++)
	{
		alphaQueue[alphaStack].mi = mi[c];
		alphaQueue[alphaStack].bRendered = false;
		alphaStack++;

		if (alphaStack==100)
			EmptyStack();
	}
};

void EmptyStack()
{
	if (stack == 0 && alphaStack == 0)
		return;

	for (int i=0;i<stack;i++)
	{
		for (int j=i;j<stack;j++)
		{
			if (renderQueue[i].mi->mr == renderQueue[j].mi->mr && renderQueue[j].bRendered == false)
			{
				TreeRender(&renderQueue[j].mi,1);
				renderQueue[j].bRendered = true;
			}
		}
	}
	stack=0;

	for (i=0;i<alphaStack;i++)
	{
		for (int j=i;j<alphaStack;j++)
		{
			if (alphaQueue[i].mi->mr == alphaQueue[j].mi->mr && alphaQueue[j].bRendered == false)
			{
				TreeRender(&alphaQueue[j].mi,1);
				alphaQueue[j].bRendered = true;
			}
		}
	}
	alphaStack=0;
};

void SetCommonStates()
{
	cam_pos = CAMERA->GetPosition();
	return;

	BATCH->set_state(RPR_BATCH,TRUE);

	BATCH->set_render_state(D3DRS_ZENABLE,TRUE);
	BATCH->set_render_state(D3DRS_ALPHATESTENABLE,FALSE);

	BATCH->set_render_state(D3DRS_SRCBLEND,D3DBLEND_SRCALPHA);
	BATCH->set_render_state(D3DRS_DESTBLEND,D3DBLEND_INVSRCALPHA);

	BATCH->set_texture_stage_state( 0, D3DTSS_COLOROP, D3DTOP_MODULATE );
	BATCH->set_texture_stage_state( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
	BATCH->set_texture_stage_state( 0, D3DTSS_COLORARG2, D3DTA_DIFFUSE );
	BATCH->set_texture_stage_state( 0, D3DTSS_ALPHAOP, D3DTOP_MODULATE );
	BATCH->set_texture_stage_state( 0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE );
	BATCH->set_texture_stage_state( 0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE );

	// filtering - bilinear with mips
	BATCH->set_sampler_state( 0, D3DSAMP_MINFILTER,		D3DTEXF_LINEAR );
	BATCH->set_sampler_state( 0, D3DSAMP_MAGFILTER,		D3DTEXF_LINEAR );
//	BATCH->set_sampler_state( 0, D3DSAMP_MIPFILTER,		D3DTEXF_POINT );


	BATCH->set_texture_stage_state( 1, D3DTSS_COLORARG1, D3DTA_TEXTURE );
	BATCH->set_texture_stage_state( 1, D3DTSS_COLORARG2, D3DTA_CURRENT );
	BATCH->set_texture_stage_state( 1, D3DTSS_ALPHAARG1, D3DTA_TEXTURE );
	BATCH->set_texture_stage_state( 1, D3DTSS_ALPHAARG2, D3DTA_CURRENT );

	// filtering - bilinear with mips
	BATCH->set_sampler_state( 1, D3DSAMP_MINFILTER,		D3DTEXF_LINEAR );
	BATCH->set_sampler_state( 1, D3DSAMP_MAGFILTER,		D3DTEXF_LINEAR );
	//	BATCH->set_sampler_state( 1, D3DSAMP_MIPFILTER,		D3DTEXF_POINT );
}

void TreeRender(MeshChain &mc,bool bLight, DWORD color)
{
	bLight = false;
	if (mc.mi[0]->bHasMesh && bLight)
	{
		Vector pos = ENGINE->get_position(mc.mi[0]->instanceIndex);
		//LIGHTS->ActivateBestLights(pos,4,max(mc.mi[0]->radius*2,500));
		LIGHTS->ActivateAmbientLight(pos);
	}
	
	bScaleTrans = false;
	
	SetCommonStates();

	colorMod = color;
	const Transform &world_to_view = MAINCAM->get_inverse_transform() ;
	for (int x=0;x<mc.numChildren;x++)
	{
		mc.mi[x]->mr->Render(mc.mi[x],world_to_view);
	}

	colorMod = 0xFFFFFFFF;
	//BATCH->set_state(RPR_BATCH,TRUE);
}

void TreeRender(MeshInfo **mi,int num_children)
{
	if (mi[0]->bHasMesh)
	{
		Vector pos = ENGINE->get_position(mi[0]->instanceIndex);
//		LIGHTS->ActivateBestLights(pos,4,max(mi[0]->radius*2,500));
		LIGHTS->ActivateAmbientLight(pos);
	}
	bScaleTrans = false;
	
	SetCommonStates();

	const Transform &world_to_view = MAINCAM->get_inverse_transform() ;
	for (int x=0;x<num_children;x++)
	{
		mi[x]->mr->Render(mi[x],world_to_view);
	}

//	BATCH->set_state(RPR_BATCH,TRUE);
}

void TreeRender(MeshInfo **mi,int num_children,const Transform &_scaleTrans)
{
	if (mi[0]->bHasMesh)
	{
		Vector pos = ENGINE->get_position(mi[0]->instanceIndex);
		//LIGHTS->ActivateBestLights(pos,4,max(mi[0]->radius*2,500));
		LIGHTS->ActivateAmbientLight(pos);
	}
	bScaleTrans = true;

	SetCommonStates();

	const Transform &world_to_view = MAINCAM->get_inverse_transform();
	scaleTrans = _scaleTrans;
	for (int x=0;x<num_children;x++)
	{
		mi[x]->mr->Render(mi[x],world_to_view);
	}

	BATCH->set_state(RPR_BATCH,TRUE);
}

void TreeRender(MeshInfo *mi,const Transform &_scaleTrans)
{
	if (mi->bHasMesh)
	{
		Vector pos = ENGINE->get_position(mi->instanceIndex);
		//LIGHTS->ActivateBestLights(pos,4,max(mi->radius*2,500));
		LIGHTS->ActivateAmbientLight(pos);
	}
	bScaleTrans = true;

	SetCommonStates();

	const Transform &world_to_view = MAINCAM->get_inverse_transform();
	scaleTrans = _scaleTrans;

	mi->mr->Render(mi,world_to_view);
	

	BATCH->set_state(RPR_BATCH,TRUE);
}

void TreeRenderPortionZAlign(MeshInfo **mi,int num_children,const Vector &split_pt)
{
	if (mi[0]->bHasMesh)
	{
		Vector pos = ENGINE->get_position(mi[0]->instanceIndex);
		//LIGHTS->ActivateBestLights(pos,4,max(mi[0]->radius*2,500));
		LIGHTS->ActivateAmbientLight(pos);
	}
	bScaleTrans = false;
	
	SetCommonStates();

	const Transform &world_to_view = MAINCAM->get_inverse_transform();
	for (int x=0;x<num_children;x++)
	{
		mi[x]->mr->RenderPortionZAlign(mi[x],world_to_view,split_pt);
	}

	BATCH->set_state(RPR_BATCH,TRUE);
}

void TreeRenderPortion(MeshInfo **mi,int num_children,const Vector &split_pt,const Vector &split_norm)
{
	if (mi[0]->bHasMesh)
	{
		Vector pos = ENGINE->get_position(mi[0]->instanceIndex);
		//LIGHTS->ActivateBestLights(pos,4,max(mi[0]->radius*2,500));
		LIGHTS->ActivateAmbientLight(pos);
	}
	bScaleTrans = false;
	
	SetCommonStates();

	const Transform &world_to_view = MAINCAM->get_inverse_transform();
	for (int x=0;x<num_children;x++)
	{
		mi[x]->mr->RenderPortion(mi[x],world_to_view,split_pt,split_norm);
	}

	BATCH->set_state(RPR_BATCH,TRUE);
}

void TreeRenderPuffy(MeshInfo **mi,int num_children,const Vector &scale,SINGLE _puffPercent)
{
	Vector pos = ENGINE->get_position(mi[0]->instanceIndex);
	//LIGHTS->ActivateBestLights(pos,4,max(mi[0]->radius*2,500));
	LIGHTS->ActivateAmbientLight(pos);
	puffPercent = _puffPercent;
	bScaleTrans = false;
	
	SetCommonStates();

	const Transform &world_to_view = MAINCAM->get_inverse_transform() ;
	Transform obj_trans = ENGINE->get_transform(mi[0]->instanceIndex);
	CAMERA->SetModelView(&obj_trans);
	for (int x=0;x<num_children;x++)
	{
		if (mi[x]->mr)
			mi[x]->mr->RenderPuffy(mi[x],world_to_view,obj_trans,scale);
	}

	BATCH->set_state(RPR_BATCH,TRUE);
}

MeshRender::MeshRender()
{
	bSolarianRenderstateKludge = false;
	vertexBuffersBuilt = false;
	fgr = 0;
	ref_cnt = 0;
	pos_list = 0;
	face_cnt = pos_cnt = 0;
	vb_handle = 0;
	for (int i=0;i<4;i++)
		vb_handle4[i] = 0;
	vb_mgr->Add(this);
}

MeshRender::~MeshRender()
{
/*		for (int i=0;i<num_fg;i++)
{
delete fg[i];
}*/
	RenderMaterial *pos = fgr;
	while (pos)
	{
		pos = pos->next;
		delete fgr;
		fgr = pos;
	}
	delete [] pos_list;
	if (vb_handle)
	{
		PIPE->destroy_vertex_buffer(vb_handle);
	}
	for (int i=0;i<4;i++)
		if (vb_handle4[i])
			PIPE->destroy_vertex_buffer(vb_handle4[i]);
	vb_mgr->Delete(this);
}

BOOL MungeFPCW( WORD *pwOldCW )
{
    BOOL ret = FALSE;
    WORD wTemp, wSave;
 
    __asm fstcw wSave
    if (wSave & 0x300)// ||            // Not single mode
//        0x3f != (wSave & 0x3f) ||   // Exceptions enabled
  //      wSave & 0xC00)              // Not round to nearest mode
    {
        __asm
        {
            mov ax, wSave
            and ax, not 300h    ;; single mode
     //       or  ax, 3fh         ;; disable all exceptions
       //     and ax, not 0xC00   ;; round to nearest mode
            mov wTemp, ax
            fldcw   wTemp
        }
        ret = TRUE;
    }
    *pwOldCW = wSave;
    return ret;
}


void MeshRender::Render(MeshInfo *mc,const Transform &world_to_view)
{
//	WORD wOldCW;
//	BOOL bChangedFPCW = MungeFPCW( &wOldCW );


	unique = 0;//mc->unique;

	Transform world_adjust_tr(false);
	Transform inv(false);
	
	if (mc->cameraMoveCnt != camMoved)
	{
		mc->cameraMoveCnt = camMoved;
		mc->bCacheValid=false;
	}
	else
	{
		mc->timer++;
		if ((mc->timer&1)==1)
			mc->bCacheValid = false;
	}



	const Transform &transform = ENGINE->get_transform(mc->instanceIndex);

	if (mc->bHasMesh == 0)
	{
		//BATCH->set_state(RPR_BATCH,TRUE);
		//BATCH->set_state(RPR_DELAY,1);
		ENGINE->render_instance(MAINCAM,mc->instanceIndex,0,LODPERCENT,0,0);
		//BATCH->set_state(RPR_DELAY,0);
		return;
	}

	if (bScaleTrans)
		world_adjust_tr = transform*scaleTrans;
	else
		world_adjust_tr = transform;

	vis_state result = VS_UNKNOWN;
	//
	// Don't bother with objects outside the view frustum.
	//
	// sphere center in world transformed to view
	Vector view_pos (world_to_view.rotate_translate(world_adjust_tr *
		mc->sphere_center));
	
	result = MAINCAM->object_visibility(view_pos, mc->radius);
	if (result == VS_NOT_VISIBLE || result == VS_SUB_PIXEL)
	{
		// Outside view frustum.
		goto Done;
	}else //if (result == VS_PARTIALLY_VISIBLE)
	{
		BATCH->set_render_state(D3DRS_CLIPPING,TRUE);
	}
/*	else // vs == ICamera::VS_FULLY_VISIBLE
	{
		BATCH->set_render_state(D3DRS_CLIPPING,FALSE);
	}*/

	BATCH->set_render_state(D3DRS_CULLMODE,D3DCULL_NONE);

	//CAMERA->SetModelView(&transform);
	
	inv = world_adjust_tr.get_inverse();

	//{
		TRANSFORM mv = world_to_view*world_adjust_tr;
		//.rotate_about_j(1);
	//BATCH->set_modelview(mv);
	PIPE->set_modelview(mv);
		//PIPE->set_default_constants(mv, CAMERA->GetPosition());
	//}
//	S32 index_count;

	cam_pos_in_object_space = inv.rotate_translate(cam_pos);
	
	PIPE->set_default_constants(world_adjust_tr,cam_pos_in_object_space);
	

	chosenAngle = 0;
/*	SINGLE bleah;
	bleah = atan2(cam_pos_in_object_space.z,cam_pos_in_object_space.x);
	if (bleah > 0.25*PI)
	{
		if (bleah > 0.75*PI)
			chosenAngle = 2;
		else
			chosenAngle = 1;
	}
	else if (bleah < -0.25*PI)
	{
		if (bleah < -0.75*PI)
			chosenAngle = 2;
			else
			chosenAngle = 3;
}*/
	int q;
	for (q=0;q<1;q++)
	{
		RenderMaterial *pos;
		pos = fgr;
		//lit = mc->lit;
		fgi = mc->fgi;
		active_mi = mc;
		
		bCacheValid = mc->bCacheValid;
		
		while (pos)
		{
			if (fgi[pos->fg_idx].bCQ2Mat || (pos->flags & MM_CHROME) == 0)
			{
				pos->Render(inv);
			}
			else
				pos->RenderSolarian(inv,world_to_view*world_adjust_tr);
			
			pos = pos->next;
		}
	}

	mc->bCacheValid = true;

Done:

	BATCH->set_render_state(D3DRS_CLIPPING,TRUE);

	SINGLE dt = OBJLIST->GetRealRenderTime();
	IEffectChannel *ec = mc->ec_list;
	while (ec)
	{
		ec->irc->Render(dt);
		ec = ec->next;
	}

	return;
}

void MeshRender::RenderPortionZAlign(MeshInfo *mc,const Transform &world_to_view,const Vector &split_pt)
{
	Vector local_split_point;

	unique = 0;//mc->unique;

	Transform world_adjust_tr(false);
	Transform inv(false);
	
	if (mc->cameraMoveCnt != camMoved)
	{
		mc->cameraMoveCnt = camMoved;
		mc->bCacheValid=false;
	}
	else
	{
		mc->timer++;
		if ((mc->timer&1)==1)
			mc->bCacheValid = false;
	}

	const Transform &transform = ENGINE->get_transform(mc->instanceIndex);

	if (mc->bHasMesh == 0)
	{
		ENGINE->render_instance(MAINCAM,mc->instanceIndex,0,LODPERCENT,0,0);
		return;
	}

	if (bScaleTrans)
		world_adjust_tr = transform*scaleTrans;
	else
		world_adjust_tr = transform;

	vis_state result = VS_UNKNOWN;
	//
	// Don't bother with objects outside the view frustum.
	//
	// sphere center in world transformed to view
	Vector view_pos (world_to_view.rotate_translate(world_adjust_tr *
		mc->sphere_center));
	
	result = MAINCAM->object_visibility(view_pos, mc->radius);
	if (result == VS_NOT_VISIBLE || result == VS_SUB_PIXEL)
	{
		// Outside view frustum.
		goto Done;
	}else if (result == VS_PARTIALLY_VISIBLE)
	{
		BATCH->set_render_state(D3DRS_CLIPPING,TRUE);
	}
	else // vs == ICamera::VS_FULLY_VISIBLE
	{
		BATCH->set_render_state(D3DRS_CLIPPING,FALSE);
	}

	BATCH->set_render_state(D3DRS_CULLMODE,D3DCULL_NONE);

	inv = world_adjust_tr.get_inverse();
	BATCH->set_modelview(world_to_view*world_adjust_tr);

//	S32 index_count;

	cam_pos_in_object_space = inv.rotate_translate(cam_pos);

	PIPE->set_default_constants(world_adjust_tr,cam_pos_in_object_space);

	//try to light all the vertices here - unnecessary ?
//	if (mc->bCacheValid == false)
//		LIGHT->light_vertices_U8(mc->lit,vert_list,norm_list,vert_cnt+vert_cnt2,&inv);

	RenderMaterial *pos;
	pos = fgr;
	fgi = mc->fgi;
	bCacheValid = mc->bCacheValid;

	local_split_point = inv.rotate_translate(split_pt);

	while (pos)
	{
		pos->RenderPortionZAlign(inv,local_split_point);
		pos = pos->next;
	}

	mc->bCacheValid = true;

Done:

	BATCH->set_render_state(D3DRS_CLIPPING,TRUE);
	return;
}

void MeshRender::RenderPortion(MeshInfo *mc,const Transform &world_to_view,const Vector &split_pt,const Vector &split_norm)
{
	Vector local_split_point;
	Vector local_split_norm;

	unique = 0;//mc->unique;

	Transform world_adjust_tr(false);
	Transform inv(false);
	
	if (mc->cameraMoveCnt != camMoved)
	{
		mc->cameraMoveCnt = camMoved;
		mc->bCacheValid=false;
	}
	else
	{
		mc->timer++;
		if ((mc->timer&1)==1)
			mc->bCacheValid = false;
	}

	const Transform &transform = ENGINE->get_transform(mc->instanceIndex);

	if (mc->bHasMesh == 0)
	{
		ENGINE->render_instance(MAINCAM,mc->instanceIndex,0,LODPERCENT,0,0);
		return;
	}

	if (bScaleTrans)
		world_adjust_tr = transform*scaleTrans;
	else
		world_adjust_tr = transform;

	vis_state result = VS_UNKNOWN;
	//
	// Don't bother with objects outside the view frustum.
	//
	// sphere center in world transformed to view
	Vector view_pos (world_to_view.rotate_translate(world_adjust_tr *
		mc->sphere_center));
	
	result = MAINCAM->object_visibility(view_pos, mc->radius);
	if (result == VS_NOT_VISIBLE || result == VS_SUB_PIXEL)
	{
		// Outside view frustum.
		goto Done;
	}else if (result == VS_PARTIALLY_VISIBLE)
	{
		BATCH->set_render_state(D3DRS_CLIPPING,TRUE);
	}
	else // vs == ICamera::VS_FULLY_VISIBLE
	{
		BATCH->set_render_state(D3DRS_CLIPPING,FALSE);
	}

	BATCH->set_render_state(D3DRS_CULLMODE,D3DCULL_NONE);

	inv = world_adjust_tr.get_inverse();
	BATCH->set_modelview(world_to_view*world_adjust_tr);

//	S32 index_count;

	cam_pos_in_object_space = inv.rotate_translate(cam_pos);

	//try to light all the vertices here - unnecessary ?
//	if (mc->bCacheValid == false)
//		LIGHT->light_vertices_U8(mc->lit,vert_list,norm_list,vert_cnt+vert_cnt2,&inv);
	
	PIPE->set_default_constants(world_adjust_tr,cam_pos_in_object_space);
	


	RenderMaterial *pos;
	pos = fgr;
	fgi = mc->fgi;
	bCacheValid = mc->bCacheValid;

	local_split_point = inv.rotate_translate(split_pt);
	local_split_norm = inv.rotate(split_norm);

	while (pos)
	{
		pos->RenderPortion(inv,local_split_point,local_split_norm);
		pos = pos->next;
	}

	mc->bCacheValid = true;

Done:

	BATCH->set_render_state(D3DRS_CLIPPING,TRUE);
	return;
}

void MeshRender::RenderPuffy(MeshInfo *mc,const Transform &world_to_view,const Transform &obj_trans,const Vector &scale)
{
	unique = 0;//mc->unique;

	Transform local_to_obj(false);
	Transform world_adjust_tr(false);
	Transform inv(false);
	
	if (mc->cameraMoveCnt != camMoved)
	{
		mc->cameraMoveCnt = camMoved;
		mc->bCacheValid=false;
	}
	else
	{
		mc->timer++;
		if ((mc->timer&1)==1)
			mc->bCacheValid = false;
	}



	const Transform &transform = ENGINE->get_transform(mc->instanceIndex);

	if (mc->bHasMesh==0)
	{
		ENGINE->render_instance(MAINCAM,mc->instanceIndex,0,LODPERCENT,0,0);
		return;
	}

	if (bScaleTrans)
		world_adjust_tr = transform*scaleTrans;
	else
		world_adjust_tr = transform;

	vis_state result = VS_UNKNOWN;
	//
	// Don't bother with objects outside the view frustum.
	//
	// sphere center in world transformed to view
	Vector view_pos (world_to_view.rotate_translate(world_adjust_tr *
		mc->sphere_center));
	
	result = MAINCAM->object_visibility(view_pos, mc->radius);
	if (result == VS_NOT_VISIBLE || result == VS_SUB_PIXEL)
	{
		// Outside view frustum.
		goto Done;
	}else if (result == VS_PARTIALLY_VISIBLE)
	{
		BATCH->set_render_state(D3DRS_CLIPPING,TRUE);
	}
	else // vs == ICamera::VS_FULLY_VISIBLE
	{
		BATCH->set_render_state(D3DRS_CLIPPING,FALSE);
	}

	BATCH->set_render_state(D3DRS_CULLMODE,D3DCULL_NONE);

	inv = world_adjust_tr.get_inverse();
//	BATCH->set_modelview(world_to_view*world_adjust_tr);

	cam_pos_in_object_space = inv.rotate_translate(cam_pos);

	//try to light all the vertices here - unnecessary ?
//	if (mc->bCacheValid == false)
//		LIGHT->light_vertices_U8(mc->lit,vert_list,norm_list,vert_cnt+vert_cnt2,&inv);

	RenderMaterial *pos;
	pos = fgr;
	//lit = mc->lit;
	fgi = mc->fgi;
	bCacheValid = mc->bCacheValid;

	local_to_obj = obj_trans.get_inverse()*world_adjust_tr;

	while (pos)
	{
		pos->RenderPuffy(inv,local_to_obj,scale);
		pos = pos->next;
	}

	mc->bCacheValid = true;

Done:

	BATCH->set_render_state(D3DRS_CLIPPING,TRUE);
	return;
}

void MeshRender::Init(MeshInfo *mc)
{
	HARCH archIndex(mc->instanceIndex);
	Init(archIndex);
}

void MeshRender::Init(ARCHETYPE_INDEX idx)
{
	bool bPromoteUVs;
	Mesh *mesh = REND->get_archetype_mesh(idx);
	//Mesh *mesh = mc->buildMesh;
	
	if (mesh)
	{


		tmp_v_cnt = 0;
		//	fgr = new FaceGroupRender[mesh->face_group_cnt];
		RenderMaterial *pos=0;
		//	vert_cnt = 0;
		//	vert_cnt2 = 0;
		face_cnt = 0;
		
		face_index_offset=0;
		for (int a=0;a<2;a++)
		{
			for (int fg_cnt=0;fg_cnt<mesh->face_group_cnt;fg_cnt++)
			{
				if (mesh->imaterial_list)
				{
					FaceGroup *fg = &mesh->face_groups[fg_cnt];
					IMaterial *mat = mesh->imaterial_list[fg->material];
					RenderMaterial *rm=0;

					if (a==0) rm = new FaceGroupRender();
					if (rm)
					{
						if (pos)
						{
							pos->next = rm;
							pos = pos->next;
						}
						else
							fgr = pos = rm;
						
						rm->Init(mesh,fg_cnt,this);
						face_index_offset += rm->new_face_cnt;
						face_cnt += rm->new_face_cnt;
					}
				}
				else if (mesh->material_list)
				{
					FaceGroup *fg = &mesh->face_groups[fg_cnt];
					Material *mat = &mesh->material_list[fg->material];
					RenderMaterial *rm=0;

					bPromoteUVs = false;
					if (strstr(mat->name,"chrome"))
						bPromoteUVs = true;

					if (CQRENDERFLAGS.bSoftwareRenderer == 0)
					{
						if (true || (CQRENDERFLAGS.bMultiTexture == 0 || mesh->texture_batch_list2 == 0 || GET_TC_WRAP_MODE(mat->emissive_texture_flags) != TC_WRAP_UV_1) && (bPromoteUVs==0 || mat->emissive_texture_id == 0))
						{
							if (a==0)
								rm = new FaceGroupRender;
						}
						else if (a==1)
						{
							rm = new FaceGroupRender2;
						}
					}
					else if (a==0)
						rm = new SoftwareFGRender;
					
					if (rm)
					{
						if (pos)
						{
							pos->next = rm;
							pos = pos->next;
						}
						else
							fgr = pos = rm;
						
						//the rm's get created here in an arbitrary order thanks to the a thing
						//however the a thing is sorting them.  That's good right?
						if (bPromoteUVs)
							rm->flags |= MM_CHROME;
						if (strstr(mat->name,"markings"))
							rm->flags |= MM_MARKINGS;

						rm->Init(mesh,fg_cnt,this);
						face_index_offset += rm->new_face_cnt;
						face_cnt += rm->new_face_cnt;
					}
				}
				else
				{
					ASSERT(false);	
				}
			}
		}
		
		if (tmp_v_cnt)
		{
			pos_list = new Vector[tmp_v_cnt];
			memcpy(pos_list,tmp_v,sizeof(Vector)*tmp_v_cnt);
			pos_cnt = tmp_v_cnt;
#if (defined(_ROB) || defined(_DREW))
			CQTRACE11("Loaded mesh with %d vertices",pos_cnt);
#endif
			RestoreVertexBuffers();
		}
	}
}

void MeshRender::BuildTangentSpace(VBVertex * pVertices, int c)
{
	static float smoothAmt = 0.5;
	if (smoothAmt != 0.0)
	{
		Vector* scratchNormal = new Vector[pos_cnt];
		// Find duplicate vertices in the mesh, and average their tangent spaces
		// together. This is necessary to avoid discontinuities at the seams.
		for(int i=0; i < pos_cnt; i++ )
		{
			Vector vN(0,0,0);
			for(int j=0; j < pos_cnt; j++ )
			{
				FLOAT dist1 = D3DXVec3LengthSq( (D3DXVECTOR3*)&(pVertices[i].pos - pVertices[j].pos) );
				if( dist1 < 1.0e-8f)
				{
					vN += pVertices[j].norm;
				}
			}
			vN.fast_normalize();
			scratchNormal[i] = vN;
		}
		for( i=0; i < pos_cnt; i++ )
		{
			pVertices[i].norm = smoothAmt * scratchNormal[i]
							  + (1 - smoothAmt) * pVertices[i].norm;
		}
	}


	U32 total_index_count = 0;
	RenderMaterial *pos = fgr;
	while (pos)
	{
		total_index_count += pos->new_face_cnt * 3;
		pos = pos->next;
	}
	
	U16* pIndices = new U16[total_index_count];
	
	int ind = 0;
	pos = fgr;
	while (pos)
	{
		if (pos->index_list4[c])
		{
			for (int i = 0; i < pos->new_face_cnt * 3 ; i++)
			{
				pIndices[ind] = pos->index_list4[c][i] + ((FaceGroupRender *)pos)->vb_offset;
				ind++;
			}
		}
		pos = pos->next;
	}
    
    U32 i,j;
	D3DXVECTOR3* scratchTangent = new D3DXVECTOR3[pos_cnt];
	D3DXVECTOR3* scratchBinormal = new D3DXVECTOR3[pos_cnt];
	
	for (i = 0; i < pos_cnt; i++)
	{
		scratchTangent[i] = D3DXVECTOR3(0,0,0);
		scratchBinormal[i] = D3DXVECTOR3(0,0,0);
	}

    // Loop through all triangles, accumulating du and dv offsets to build
    // basis vectors
    for( i = 0; (S32)i < total_index_count; i += 3 )
    {       
        WORD i0 = pIndices[i+0];
        WORD i1 = pIndices[i+1];
        WORD i2 = pIndices[i+2];

        if( i0<pos_cnt && i1<pos_cnt  && i2<pos_cnt )
		{
            VBVertex* v0 = &pVertices[i0];
            VBVertex* v1 = &pVertices[i1];
            VBVertex* v2 = &pVertices[i2];
            D3DXVECTOR3   du, dv;
            D3DXVECTOR3   cp;

            // Skip degenerate triangles
            if( fabs(v0->pos.x-v1->pos.x)<1e-6 && fabs(v0->pos.y-v1->pos.y)<1e-6 && fabs(v0->pos.z-v1->pos.z)<1e-6 )
                continue;
            if( fabs(v1->pos.x-v2->pos.x)<1e-6 && fabs(v1->pos.y-v2->pos.y)<1e-6 && fabs(v1->pos.z-v2->pos.z)<1e-6 )
                continue;
            if( fabs(v2->pos.x-v0->pos.x)<1e-6 && fabs(v2->pos.y-v0->pos.y)<1e-6 && fabs(v2->pos.z-v0->pos.z)<1e-6 )
                continue;

			D3DXVECTOR2 UV1, UV2, UV0;

			UV0.x = v0->u;
			UV0.y = v0->v;
			UV1.x = v1->u;
			UV1.y = v1->v;
			UV2.x = v2->u;
			UV2.y = v2->v;

            D3DXVECTOR3 edge01( v1->pos.x - v0->pos.x, UV1.x - UV0.x, UV1.y - UV0.y );
            D3DXVECTOR3 edge02( v2->pos.x - v0->pos.x, UV2.x - UV0.x, UV2.y - UV0.y );
            D3DXVec3Cross( &cp, &edge01, &edge02 );
            if( fabs(cp.x) > 1e-8 )
            {
                du.x = -cp.y / cp.x;        
                dv.x = -cp.z / cp.x;
            }
            edge01 = D3DXVECTOR3( v1->pos.y - v0->pos.y, UV1.x - UV0.x, UV1.y - UV0.y );
            edge02 = D3DXVECTOR3( v2->pos.y - v0->pos.y, UV2.x - UV0.x, UV2.y - UV0.y );
            D3DXVec3Cross( &cp, &edge01, &edge02 );
            if( fabs(cp.x) > 1e-8 )
            {
                du.y = -cp.y / cp.x;
                dv.y = -cp.z / cp.x;
            }
			edge01 = D3DXVECTOR3( v1->pos.z - v0->pos.z, UV1.x - UV0.x, UV1.y - UV0.y );
            edge02 = D3DXVECTOR3( v2->pos.z - v0->pos.z, UV2.x - UV0.x, UV2.y - UV0.y );
            D3DXVec3Cross( &cp, &edge01, &edge02 );
            if( fabs(cp.x) > 1e-8 )
            {
                du.z = -cp.y / cp.x;
                dv.z = -cp.z / cp.x;
            }
            scratchTangent[i0] += du;
            scratchTangent[i1] += du;
            scratchTangent[i2] += du;

			
            scratchBinormal[i0] += dv;
            scratchBinormal[i1] += dv;
            scratchBinormal[i2] += dv;
        }	
		else
		{
			int i = 42;
		}
    }


    for( i = 0; i < pos_cnt; i++)
    {    
        D3DXVec3Normalize( &scratchTangent[i], &scratchTangent[i] );
		D3DXVec3Normalize( &scratchBinormal[i], &scratchBinormal[i] );
        D3DXVec3Normalize( (D3DXVECTOR3*)&pVertices[i].norm, (D3DXVECTOR3*)&pVertices[i].norm);
        //D3DXVec3Cross( &scratchBinormal[i], &scratchTangent[i], 
        //               (D3DXVECTOR3*)&pVertices[i].norm);
    }

	for( i=0; i < pos_cnt; i++ )
    {
		pVertices[i].tanX = scratchTangent[i].x;
		pVertices[i].tanY = scratchTangent[i].y;
		pVertices[i].tanZ = scratchTangent[i].z;
		pVertices[i].binX = scratchBinormal[i].x;
		pVertices[i].binY = scratchBinormal[i].y;
		pVertices[i].binZ = scratchBinormal[i].z;
    }
	delete[] pIndices;
	delete[] scratchTangent;
	delete[] scratchBinormal;
}


void MeshRender::RestoreVertexBuffers()
{
	static bool debugVertexBuffers = false;
	if (vertexBuffersBuilt && !debugVertexBuffers) return;

	vertexBuffersBuilt = true;
	CQASSERT(CQFLAGS.b3DEnabled);
	if (CQRENDERFLAGS.bSoftwareRenderer==0 && pos_cnt)
	{
		U32 flags=0;
		if (CQRENDERFLAGS.bHardwareGeometry==0)
			flags = IRP_VBF_SYSTEM;
		
		GENRESULT result;		
		for (int c=0;c<1;c++)  //only make one extra buffer
		{
			U16 vert_sort[HUGE];
			U16 inv_vert_sort[HUGE];
			
			if (vb_handle4[c])
			{
				result = PIPE->destroy_vertex_buffer( vb_handle4[c] );
				CQASSERT(result == GR_OK);
			}
			CQASSERT(pos_cnt && pos_cnt != 0xffff);
			result = PIPE->create_vertex_buffer( D3DFVF_VBVERTEX, pos_cnt, flags, &vb_handle4[c] );
			CQASSERT(result == GR_OK);
			///TEST
			VBVertex *vbuffer;
			U32 size;
			result = PIPE->lock_vertex_buffer(vb_handle4[c],DDLOCK_WRITEONLY | DDLOCK_DISCARDCONTENTS,(void **)&vbuffer,&size);
			CQASSERT(result == GR_OK);
			
			RenderMaterial *pos = fgr;
			while (pos)
			{
				if (pos->GetVertexType() == D3DFVF_RPVERTEX && pos->faces)
				{
					((FaceGroupRender *)pos)->sortVertices(c,vert_sort);
					for (int v=0;v<pos->vert_cnt;v++)
						inv_vert_sort[vert_sort[v]] = v;
					
					RPVertex *src_verts = (RPVertex *)pos->src_verts_buffer;
					int v_cnt = ((FaceGroupRender *)pos)->vb_offset;
					//	memcpy(&vbuffer[((FaceGroupRender *)pos)->vb_offset],pos->src_verts_buffer,pos->vert_cnt*sizeof(RPVertex));
					for (int p=0;p<pos->vert_cnt;p++)
					{
						vbuffer[v_cnt].pos = src_verts[vert_sort[p]].pos;
						vbuffer[v_cnt].u = src_verts[vert_sort[p]].u;
						vbuffer[v_cnt].v = src_verts[vert_sort[p]].v;
						if (vbuffer[v_cnt].v > 5 || vbuffer[v_cnt].v < -5
							|| vbuffer[v_cnt].u > 5 || vbuffer[v_cnt].u < -5
							&& (vbuffer[v_cnt].u != 41.742188 && vbuffer[v_cnt].u != -41.742188))
						{
							int i = 42;
							i++;
						}
						if (src_verts[vert_sort[p]].b != 255)
						{
							int i = 42;
							i++;
						}
						if (pos->src_u2)
						{
							vbuffer[v_cnt].u2 = pos->src_u2[vert_sort[p]];
							vbuffer[v_cnt].v2 = pos->src_v2[vert_sort[p]];
						}
						else
						{
							vbuffer[v_cnt].u2 = src_verts[vert_sort[p]].u;
							vbuffer[v_cnt].v2 = src_verts[vert_sort[p]].v;
						}

						vbuffer[v_cnt].v = 1.0 - vbuffer[v_cnt].v;
						vbuffer[v_cnt].v2 = 1.0 - vbuffer[v_cnt].v2;

						vbuffer[v_cnt].norm = pos->src_norms[vert_sort[p]];
						vbuffer[v_cnt].norm.normalize();
						v_cnt++;
					}
					
					if (pos->index_list4[c] == 0)
					{
						PIPE->create_index_buffer(pos->new_face_cnt*3*sizeof(U16), &pos->IB4[c]);
						pos->index_list4[c] = new U16[pos->new_face_cnt*3];
						pos->maxVert[c] = 0;
						U16* lockedInds;
						pos->IB4[c]->Lock(0,0,(void**)&lockedInds,0);
						for (int i=0;i<pos->new_face_cnt*3;i++)
						{
							lockedInds[i] = pos->index_list4[c][i] = inv_vert_sort[pos->index_list[i]];
							if (pos->index_list4[c][i] > pos->maxVert[c])
								pos->maxVert[c] = pos->index_list4[c][i];
						}
						pos->IB4[c]->Unlock();
					}
				}
				pos = pos->next;
			}
			BuildTangentSpace(vbuffer, c);
			result = PIPE->unlock_vertex_buffer(vb_handle4[c]);
			CQASSERT(result == GR_OK);
//			result = PIPE->optimize_vertex_buffer(vb_handle4[c]);
			CQASSERT(result == GR_OK);
		}
	}
	
}



void LoadFromTexLib(const char* name, U32 *tex)
{
	if (TEXLIB->has_texture_id(name) != GR_OK)		// name is not present yet
		TEXLIB->load_texture(TEXTURESDIR, name);
	TEXLIB->get_texture_id(name, tex);
}

void MeshRender::SetupMeshInfo(MeshInfo *mc,bool bMakeBuffers, const char* materialOverride)
{
	if (!emissiveEffect)
		{
			emissiveEffect = PIPE->load_effect("cq2\\shaders\\emissive.fx",(IComponentFactory *)TEXTURESDIR);
			nonEmissiveEffect = PIPE->load_effect("cq2\\shaders\\noEmissive.fx",(IComponentFactory *)TEXTURESDIR);
		}

	mc->mr = this;
	AddRef();

	if (mc->bHasMesh == 0)
		return;

	Mesh *mesh = REND->get_instance_mesh(mc->instanceIndex);
	//pad for goofy readahead behavior
	//	mc->lit = new LightRGB_U8[vert_cnt+vert_cnt2+1];
	CQASSERT(mc->fgi == 0);
	mc->fgi = new FaceGroupInfo[mesh->face_group_cnt];
	mc->faceGroupCnt = mesh->face_group_cnt;
	mc->bCacheValid = false;
	
	if (bMakeBuffers)
	{
		//added padding for cache opt....
		mc->faceRenders = new U8[mc->mr->face_cnt+1];//mesh->face_groups[fg_cnt].face_cnt];
		memset(mc->faceRenders,0x00,sizeof(U8)*(mc->mr->face_cnt+1));//mesh->face_groups[fg_cnt].face_cnt);
	}
	
	RenderMaterial *pos;
	pos = fgr;

	while (pos)
	{
		FaceGroup *fg = &mesh->face_groups[pos->fg_idx];
		if (mesh->imaterial_list)
		{
			mc->fgi[pos->fg_idx].iMat = mesh->imaterial_list[fg->material];
			mc->fgi[pos->fg_idx].bCQ2Mat = false;
			mc->fgi[pos->fg_idx].glowTex = 0;
			mc->fgi[pos->fg_idx].colorTex = 0;
			mc->fgi[pos->fg_idx].bumpTex = 0;
		}
		else
		if (mesh->material_list)
		{
			Material *mat = &mesh->material_list[fg->material];
			mc->fgi[pos->fg_idx].diffuse.r = mat->diffuse.r;
			mc->fgi[pos->fg_idx].diffuse.g = mat->diffuse.g;
			mc->fgi[pos->fg_idx].diffuse.b = mat->diffuse.b;
			mc->fgi[pos->fg_idx].emissive.r = mat->emission.r;
			mc->fgi[pos->fg_idx].emissive.g = mat->emission.g;
			mc->fgi[pos->fg_idx].emissive.b = mat->emission.b;
			mc->fgi[pos->fg_idx].a = 255;
			mc->fgi[pos->fg_idx].unique_id = (U32)mat;
			
			char texName[512];
			sprintf(texName, "cq2\\");
			//if (mat->texturelib)
			//{
				//mat->texturelib->get_texture_name(fgi[fg_idx].texture_id, texName, 512);
			U32 id = 0;
			TEXLIB->get_texture_ref_texture_id( mat->diffuse_texture_ref, &id);
			TEXLIB->get_texture_name(id, texName + 4, 512);
			//}
			
			if (!texName[6])
			//if (true) // for debugging
			{
				mc->fgi[pos->fg_idx].bCQ2Mat = false;
			}
			else
			{
				strlwr(texName);
				bool bSolarian = false;
				if (strstr(texName, "environ"))
				{
					this->bSolarianRenderstateKludge = true;
					bSolarian = true;
					U32 id = 0;
					texName[6] = 0;
					TEXLIB->get_texture_ref_texture_id( mat->emissive_texture_ref, &id);
					TEXLIB->get_texture_name(id, texName +4, 512);
				}

				char* tmp = strstr(texName, "_color.");
				if (!tmp)
				{
					tmp = strchr(texName, '.');
				}
				if (!tmp)
				{
					tmp = strstr(texName, "_merge");
				}
				if (!tmp) 
				{
					mc->fgi[pos->fg_idx].bCQ2Mat = false;
				}
				else
				{
					sprintf(tmp, "_color.tga");
					mc->fgi[pos->fg_idx].colorTex = TMANAGER->CreateTextureFromFile(texName, (IComponentFactory *)TEXTURESDIR, DA::TGA, PF_4CC_DAA8);
					sprintf(tmp, "_bump.tga");
					mc->fgi[pos->fg_idx].bumpTex = TMANAGER->CreateTextureFromFile(texName, (IComponentFactory *)TEXTURESDIR, DA::TGA, PF_4CC_DAA8);
					if (mc->fgi[pos->fg_idx].colorTex && mc->fgi[pos->fg_idx].bumpTex)
					{
						mc->fgi[pos->fg_idx].bCQ2Mat = true;
					}
					else
					{
						mc->fgi[pos->fg_idx].bCQ2Mat = false;
					}
					mc->fgi[pos->fg_idx].glowTex = 0;
					if (strstr(texName, "solarian_lm"))
					{
						mc->fgi[pos->fg_idx].glowTex = mc->fgi[pos->fg_idx].colorTex;
					}
					if (mc->fgi[pos->fg_idx].bCQ2Mat)
					{
						if (mat->emissive_texture_id && mat->emissive_texture_ref)
						{
							U32 id = 0;
							texName[6] = 0;
							TEXLIB->get_texture_ref_texture_id( mat->emissive_texture_ref, &id);
							TEXLIB->get_texture_name(id, texName +4, 512);
						}

						if (texName[6])
						{
							char* tmp = strchr(texName, '.');
							if (!(mat->emissive_texture_id && mat->emissive_texture_ref))
							{
								tmp -= 5;
							}
							if (tmp)
							{
								sprintf(tmp, "_glow.tga");
								mc->fgi[pos->fg_idx].glowTex = TMANAGER->CreateTextureFromFile(texName, (IComponentFactory *)TEXTURESDIR, DA::TGA, PF_4CC_DAA8);
							}
						}
					}
				}
			}
			if (!mc->fgi[pos->fg_idx].bCQ2Mat)
			{
				mc->fgi[pos->fg_idx].colorTex =	mat->texture_id;
			}
			mc->fgi[pos->fg_idx].texture_flags = mat->texture_flags;
			mc->fgi[pos->fg_idx].texture_id = mat->texture_id;
			mc->fgi[pos->fg_idx].emissive_texture_flags = mat->emissive_texture_flags;
			mc->fgi[pos->fg_idx].emissive_texture_id = mat->emissive_texture_id;
		}

		
		
		pos = pos->next;
	}
}

IRenderMaterial * MeshRender::GetNextFaceGroup (IRenderMaterial *irm)
{
	if (irm == 0)
		return fgr;
	else
		return ((RenderMaterial *)irm)->next;
}

//?
/*void MeshRender::AddFaceGroup (IRenderMaterial *irm)
{
	if (pos)
	{
		pos->next = rm;
		pos = pos->next;
	}
	else
		dest->fgr = pos = rm;
}*/
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//



FaceGroupRender::FaceGroupRender()
{
	hintID = U32(-1);

	D3DXMATRIX matTmp;
	D3DXMatrixScaling(&matTmp, 1,-1,1);
	D3DXMatrixIdentity(&matTmp);
	D3DXMatrixTranspose(&matFlipY, &matTmp);
}

FaceGroupRender::~FaceGroupRender()
{
//	delete [] pos_list;
	delete [] src_verts_buffer;
	delete [] src_norms;
	delete [] src_u2;
	delete [] src_v2;
	delete [] index_list;
//	delete [] nref_list;
	if (faces_ptr)
		free(faces_ptr);
	else if (faces)
		delete [] faces;
	for (int i=0;i<4;i++)
	{
		if (index_list4[i])
			delete [] index_list4[i];
	}
}

U32 FaceGroupRender::getStateID()
{
	U32 stateID;
	unique = 1;
	if (fgi[fg_idx].texture_flags & TF_F_HAS_ALPHA) 
	{
		unique = 2;
	}
	else if (fgi[fg_idx].a != 255) //cloaking or something
	{
		unique = 3;
	}
	
	if ((flags & MM_MULTITEX) && CQEFFECTS.bEmissiveTextures && (active_mi->bSuppressEmissive==0))
	{
		stateID = U32(fgi[fg_idx].unique_id)+unique;
	}
	else
	{
		stateID = U32(fgi[fg_idx].unique_id)+unique+4;
	}

	return stateID;
}

void FaceGroupRender::setRenderStates()
{
	//Extended Material Members:
	//fgi[fg_idx].renderType
	//fgi[fg_idx].colorTex
	//fgi[fg_idx].bumpTex
	//fgi[fg_idx].glowTex
	
	
	
	
	{
		if (fgi[fg_idx].texture_flags & TF_F_HAS_ALPHA)
		{
			BATCH->set_render_state(D3DRS_ALPHABLENDENABLE,TRUE);
			BATCH->set_render_state(D3DRS_ZWRITEENABLE,FALSE);
		}
		else if (fgi[fg_idx].a != 255)
		{
			BATCH->set_render_state(D3DRS_ALPHABLENDENABLE,TRUE);
			BATCH->set_render_state(D3DRS_ZWRITEENABLE,TRUE);
		}
		else
		{
			BATCH->set_render_state(D3DRS_ALPHABLENDENABLE,FALSE);
			BATCH->set_render_state(D3DRS_ZWRITEENABLE,TRUE);
		}
		
		BATCH->set_texture_stage_texture( 0, fgi[fg_idx].texture_id );

		
		// addressing - clamped
		BATCH->set_sampler_state( 0, D3DSAMP_ADDRESSU, MAT_GET_ADDR_MODE(fgi[fg_idx].texture_flags,0) );
		BATCH->set_sampler_state( 0, D3DSAMP_ADDRESSV, MAT_GET_ADDR_MODE(fgi[fg_idx].texture_flags,1) );
		
		BATCH->set_texture_stage_state( 0, D3DTSS_TEXCOORDINDEX, 0);
		
		if ((flags & MM_MULTITEX) && CQEFFECTS.bEmissiveTextures && (active_mi->bSuppressEmissive==0))
		{
			BATCH->set_texture_stage_state( 1, D3DTSS_COLOROP, D3DTOP_SELECTARG1 );
			BATCH->set_texture_stage_state( 1, D3DTSS_COLORARG1, D3DTA_TEXTURE);
			BATCH->set_texture_stage_state( 1, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1 );
			BATCH->set_texture_stage_state( 1, D3DTSS_ALPHAARG1, D3DTA_CURRENT);
			BATCH->set_texture_stage_texture(1,fgi[fg_idx].emissive_texture_id);
			
			// addressing - clamped
			BATCH->set_sampler_state( 1, D3DSAMP_ADDRESSU, MAT_GET_ADDR_MODE(fgi[fg_idx].emissive_texture_flags,0) );
			BATCH->set_sampler_state( 1, D3DSAMP_ADDRESSV, MAT_GET_ADDR_MODE(fgi[fg_idx].emissive_texture_flags,1) );
			BATCH->set_sampler_state( 1, D3DSAMP_ADDRESSV,D3DTADDRESS_WRAP);
			BATCH->set_sampler_state( 1, D3DSAMP_ADDRESSU,D3DTADDRESS_WRAP);
			
			BATCH->set_texture_stage_state( 1, D3DTSS_TEXCOORDINDEX, 1);
		}
		else
		{
			BATCH->set_texture_stage_state( 1, D3DTSS_COLOROP, D3DTOP_DISABLE );
			BATCH->set_texture_stage_state( 1, D3DTSS_ALPHAOP, D3DTOP_DISABLE );
		}
			BATCH->set_texture_stage_state( 2, D3DTSS_COLOROP, D3DTOP_DISABLE );
			BATCH->set_texture_stage_state( 3, D3DTSS_COLOROP, D3DTOP_DISABLE );
			BATCH->set_texture_stage_state( 4, D3DTSS_COLOROP, D3DTOP_DISABLE );
	}	
}


void FaceGroupRender::Init(Mesh *mesh,int _fg_idx,MeshRender *_mr)//MeshInfo *mc)
{
	face_offset = face_index_offset;
	vb_offset = tmp_v_cnt;

	float* tmp_u2 = tmp_float1;
	float* tmp_v2 = tmp_float2;
	RPVertex *tmp_verts = tmp_verts1;
	CQASSERT(faces == 0);//_norm_list == 0);
	mr = _mr;
	fg_idx = _fg_idx;
	fg = &mesh->face_groups[fg_idx];
	int i;

	if (mesh->material_list && mesh->material_list[fg->material].emissive_texture_id != 0)
		flags |= MM_MULTITEX;
	
	U16 id_list[HUGE];
	ArchetypeFaceInfo afi[1500];
	
	U16 refList[HUGE][14];
	memset(refList,0xff,sizeof(U16)*HUGE*14);
	U16 currentRef,lastRef;
	currentRef=lastRef=0;//=mr->vert_cnt;
	int ref_cnt=0;
	new_face_cnt=0;
	for (i=0;i<fg->face_cnt;i++)
	{
		U32 ref[3],tref[3], tref2[3];
		
		//face_norm_list[i] = mesh->normal_ABC[fg->face_normal[i]];
		
		ref[0] = mesh->vertex_batch_list[fg->face_vertex_chain[i*3]];
		ref[1] = mesh->vertex_batch_list[fg->face_vertex_chain[i*3+1]];
		ref[2] = mesh->vertex_batch_list[fg->face_vertex_chain[i*3+2]];
		
		tref2[0] = tref[0] = mesh->texture_batch_list[fg->face_vertex_chain[i*3]];
		tref2[1] = tref[1] = mesh->texture_batch_list[fg->face_vertex_chain[i*3+1]];
		tref2[2] = tref[2] = mesh->texture_batch_list[fg->face_vertex_chain[i*3+2]];

		CQASSERT(tref[2] < (U32)mesh->texture_vertex_cnt);

		bool bSecondSet;
		if (mesh->material_list)
		{
			bSecondSet = GET_TC_WRAP_MODE(mesh->material_list[fg->material].emissive_texture_flags) == TC_WRAP_UV_1;
			bSecondSet = bSecondSet && mesh->texture_batch_list2 && ((flags & MM_CHROME) == 0);
		}
		else
		{
			bSecondSet = mesh->texture_batch_list2;
		}

		if (bSecondSet)
		{
			tref2[0] = mesh->texture_batch_list2[fg->face_vertex_chain[i*3]];
			tref2[1] = mesh->texture_batch_list2[fg->face_vertex_chain[i*3+1]];
			tref2[2] = mesh->texture_batch_list2[fg->face_vertex_chain[i*3+2]];
		}

		int num_sides=1;
		if (fg->face_properties[i] & TWO_SIDED)
			num_sides = 2;
		
		for (int sides=0;sides<num_sides;sides++)
		{
			Vector face_norm = mesh->normal_ABC[fg->face_normal[i]];
			if (sides)
				face_norm *= -1;
			for (int vv = 0;vv<3;vv++)
			{
				Vector this_norm;
				if (fg->face_properties[i] & FLAT_SHADED)
					this_norm = face_norm;
				else
				{
					this_norm = mesh->normal_ABC[mesh->vertex_normal[ref[vv]]];

					if (sides)
						this_norm *= -1;
				}

				afi[new_face_cnt].norm = face_norm;
//				afi[new_face_cnt].D = fg->face_D_coefficient[i];

				//broken
				//nref[ref_cnt] = mesh->vertex_normal[ref[vv]];
				int idx = 0;
				CQASSERT(ref[vv] < HUGE);
				if (refList[ref[vv]][0] != 0xffff)
				{
					currentRef = refList[ref[vv]][idx];
					while (refList[ref[vv]][idx] != 0xffff && 
						(tmp_verts[currentRef].u != mesh->texture_vertex_list[tref[vv]].u || tmp_verts[currentRef].v != mesh->texture_vertex_list[tref[vv]].v || (tmp_n[currentRef] - this_norm).magnitude_squared() > 1e-5))
					{
						idx++;
#if (defined(_DREW) || defined(_ROB))
						CQASSERT(idx != 8);
#endif
						CQASSERT(idx < 14);
						currentRef = refList[ref[vv]][idx];
					}
				}
				
				if (refList[ref[vv]][idx] == 0xffff)
				{
					currentRef = lastRef;
					lastRef++;
					CQASSERT(lastRef < HUGE);
					refList[ref[vv]][idx] = currentRef;
					//do this if pos_list should be parallel to src_verts
					tmp_v[tmp_v_cnt++] = mesh->object_vertex_list[ref[vv]];
					//
				}
				
				tmp_verts[currentRef].pos = mesh->object_vertex_list[ref[vv]];
				tmp_verts[currentRef].u = mesh->texture_vertex_list[tref[vv]].u;
				tmp_verts[currentRef].v = mesh->texture_vertex_list[tref[vv]].v;
				tmp_u2[currentRef] = mesh->texture_vertex_list[tref2[vv]].u;
				tmp_v2[currentRef] = mesh->texture_vertex_list[tref2[vv]].v;
				//tmp_verts[currentRef].a = 255;
				//temp?
				tmp_verts[currentRef].color = 0xffffffff;
				id_list[ref_cnt] = currentRef;
				
				tmp_n[currentRef] = this_norm;

				ref_cnt++;

				CQASSERT(ref_cnt < HUGE);

				//do this if pos_list is optimized
				/*
				if (re_index_list[ref[vv]] == 0xffff)
				{
					re_index_list[ref[vv]] = tmp_v_cnt;
					tmp_v[tmp_v_cnt++] = tmp_verts[currentRef].pos;
				}
				something = something
				*/

				CQASSERT(currentRef < 1400);
			}

			//if this is the second side of a double-sided poly, rewind.
			if (sides)
			{
				//do norms here too if re-enabled
				int temp=id_list[ref_cnt-2];
				id_list[ref_cnt-2] = id_list[ref_cnt-1];
				id_list[ref_cnt-1] = temp;
			}
			new_face_cnt++;
		}
	}
	vert_cnt = lastRef;
	src_u2 = new float[lastRef];
	memcpy(src_u2,tmp_u2,lastRef*sizeof(float));
	src_v2 = new float[lastRef];
	memcpy(src_v2,tmp_v2,lastRef*sizeof(float));
	src_verts_buffer = (void *)(new RPVertex[lastRef]);
	memcpy(src_verts_buffer,tmp_verts,lastRef*sizeof(RPVertex));
	//pos_list = new Vector[lastRef];
	//memcpy(pos_list,tmp_v,lastRef*sizeof(Vector));
	src_norms = new Vector[lastRef];
	memcpy(src_norms,tmp_n,lastRef*sizeof(Vector));
//	nref_list = new U32[ref_cnt];
//	memcpy(nref_list,nref,ref_cnt*sizeof(U32));
	index_list = new U16[ref_cnt];
	memcpy(index_list,id_list,ref_cnt*sizeof(U16));
	//faces = new ArchetypeFaceInfo[new_face_cnt];

	//pad for cache read-ahead
	faces_ptr = (U8 *)malloc(sizeof(ArchetypeFaceInfo)*(new_face_cnt+1)+31);
	faces = (ArchetypeFaceInfo *)((U32(faces_ptr)+31) & ~31);
	memcpy(faces,afi,new_face_cnt*sizeof(ArchetypeFaceInfo));
	//	result->sortVertsOnZ();
	//CQASSERT(HEAP->EnumerateBlocks());

	///TEST
/*	RPVertex *vbuffer;
	U32 size;
	PIPE->lock_vertex_buffer(vb_handle,DDLOCK_WRITEONLY | DDLOCK_NOOVERWRITE,(void **)&vbuffer,&size);
	memcpy(vbuffer,src_verts_buffer,vert_cnt*sizeof(RPVertex));
	PIPE->unlock_vertex_buffer(vb_handle);
	PIPE->optimize_vertex_buffer(vb_handle);*/
}

void FaceGroupRender::Clone(RenderMaterial **rm,bool bCopyBuffers)
{
	FaceGroupRender *fgr;
	*rm = fgr = new FaceGroupRender;
	fgr->mr = mr;
	fgr->fg_idx = fg_idx;
	fgr->fg = fg;
	fgr->flags = flags;
	fgr->new_face_cnt=new_face_cnt;
	fgr->vert_cnt = vert_cnt;
	fgr->face_offset = face_offset;
	if (bCopyBuffers)
	{
		fgr->src_verts_buffer = (void *)(new RPVertex[vert_cnt]);
		memcpy(fgr->src_verts_buffer,src_verts_buffer,vert_cnt*sizeof(RPVertex));
//		fgr->pos_list = new Vector[vert_cnt];
//		memcpy(fgr->pos_list,pos_list,vert_cnt*sizeof(Vector));
		fgr->src_u2 = new float[vert_cnt];
		memcpy(fgr->src_u2,src_u2,vert_cnt*sizeof(float));
		fgr->src_v2 = new float[vert_cnt];
		memcpy(fgr->src_v2,src_v2,vert_cnt*sizeof(float));
		fgr->src_norms = new Vector[vert_cnt];
		memcpy(fgr->src_norms,src_norms,vert_cnt*sizeof(Vector));
		fgr->index_list = new U16[new_face_cnt*3];
		memcpy(fgr->index_list,index_list,new_face_cnt*3*sizeof(U16));
	

		//pad for cache read-ahead
		fgr->faces_ptr = (U8 *)malloc(sizeof(ArchetypeFaceInfo)*(new_face_cnt+1)+31);
		fgr->faces = (ArchetypeFaceInfo *)((U32(faces_ptr)+31) & ~31);
		memcpy(fgr->faces,faces,new_face_cnt*sizeof(ArchetypeFaceInfo));
	}

//	CQASSERT(HEAP->EnumerateBlocks());

}

void FaceGroupRender::splitEdgeZ(const RPVertex & v1,const RPVertex & v2,RPVertex & v_out,SINGLE z)
{
	SINGLE ratio = (z-v1.pos.z)/(v2.pos.z-v1.pos.z);

	v_out.pos = v1.pos+ratio*(v2.pos-v1.pos);
	v_out.u = v1.u+ratio*(v2.u-v1.u);
	v_out.v = v1.v+ratio*(v2.v-v1.v);
}

void FaceGroupRender::splitEdge(const RPVertex & v1,const RPVertex & v2,RPVertex & v_out,SINGLE split_d,const Vector &split_n,SINGLE &ratio)
{
	Vector diff = v2.pos-v1.pos;
	Vector delta = v2.pos-split_d*split_n;

	ratio = dot_product(split_n,delta)/dot_product(split_n,diff);

	v_out.pos = v1.pos+ratio*(v2.pos-v1.pos);
	v_out.u = v1.u+ratio*(v2.u-v1.u);
	v_out.v = v1.v+ratio*(v2.v-v1.v);
}

void FaceGroupRender::splitEdgePlane(const RPVertex & v1,const RPVertex & v2,RPVertex & v_out,const Vector & planePoint,const Vector &split_n)
{
	Vector tmp1 = planePoint-v1.pos;
	Vector tmp2 = v2.pos-v1.pos;

	SINGLE den = dot_product(split_n,tmp2);

	SINGLE ratio = 0;

	if(den != 0)
		ratio = dot_product(split_n,tmp1)/den;

	v_out.pos = v1.pos+ratio*(v2.pos-v1.pos);
	v_out.u = v1.u+ratio*(v2.u-v1.u);
	v_out.v = v1.v+ratio*(v2.v-v1.v);
}

void FaceGroupRender::sortVertices(int c,U16 *sort_vert)
{
	Vector look(0,1,0);
/*	switch (c)
	{
	case 0:
		look.set(0.766,0.642,0);
		break;
	case 1:
		look.set(0,0.642,0.766);
		break;
	case 2:
		look.set(-0.766,0.642,0);
		break;
	case 3:
		look.set(0,0.642,-0.766);
		break;
	}*/

	SINGLE dots[HUGE];

	for (int i=0;i<vert_cnt;i++)
	{
		dots[i] = -444.0f;
		sort_vert[i] = i;
	}

	for (i=0;i<new_face_cnt;i++)
	{
		SINGLE new_dot = dot_product(look,faces[i].norm);
		for (int v=0;v<3;v++)
		{
			if (new_dot > dots[index_list[i*3+v]])
			{
				dots[index_list[i*3+v]] = new_dot;
			}
		}
	}

	for (i=0;i<vert_cnt-1;i++)
	{
		for (int j=i+1;j<vert_cnt;j++)
		{
			if (dots[i] < dots[j])
			{
				SINGLE temp = dots[i];
				dots[i] = dots[j];
				dots[j] = temp;
				
				U16 temp2 = sort_vert[i];
				sort_vert[i] = sort_vert[j];
				sort_vert[j] = temp2;
			}
		}
	}
}

FaceGroupRender2::FaceGroupRender2()
{
	hintID = U32(-1);
	src_u2 = src_v2 = 0;
}

FaceGroupRender2::~FaceGroupRender2()
{
//	delete [] pos_list;
	delete [] src_verts_buffer;
	delete [] src_norms;
	delete [] index_list;
//	delete [] nref_list;
	if (faces_ptr)
		free(faces_ptr);
	else if (faces)
		delete [] faces;
}

U32 FaceGroupRender2::getStateID()
{
	U32 stateID;
	unique = 1;
	if (fgi[fg_idx].texture_flags & TF_F_HAS_ALPHA)
	{
		unique = 2;
	}
	else if (fgi[fg_idx].a != 255)
	{
		unique = 3;
	}
	
	if (CQEFFECTS.bEmissiveTextures && (active_mi->bSuppressEmissive==0))
	{
		stateID = U32(fgi[fg_idx].unique_id)+unique;
	}
	else
	{
		stateID = U32(fgi[fg_idx].unique_id)+unique+4;
	}

	return stateID;
}

void FaceGroupRender2::setRenderStates()
{
	if (fgi[fg_idx].texture_flags & TF_F_HAS_ALPHA)
	{
		BATCH->set_render_state(D3DRS_ALPHABLENDENABLE,TRUE);
		BATCH->set_render_state(D3DRS_ZWRITEENABLE,FALSE);
	}
	else if (fgi[fg_idx].a != 255)
	{
		BATCH->set_render_state(D3DRS_ALPHABLENDENABLE,TRUE);
		BATCH->set_render_state(D3DRS_ZWRITEENABLE,TRUE);
	}
	else
	{
		BATCH->set_render_state(D3DRS_ALPHABLENDENABLE,FALSE);
		BATCH->set_render_state(D3DRS_ZWRITEENABLE,TRUE);
	}

	BATCH->set_texture_stage_texture( 0, fgi[fg_idx].texture_id );
	

	// addressing - clamped
	BATCH->set_sampler_state( 0, D3DSAMP_ADDRESSU, MAT_GET_ADDR_MODE(fgi[fg_idx].texture_flags,0) );
	BATCH->set_sampler_state( 0, D3DSAMP_ADDRESSV, MAT_GET_ADDR_MODE(fgi[fg_idx].texture_flags,1) );

	BATCH->set_texture_stage_state( 0, D3DTSS_TEXCOORDINDEX, 0);

	if (CQEFFECTS.bEmissiveTextures && (active_mi->bSuppressEmissive==0))
	{
		BATCH->set_texture_stage_state( 1, D3DTSS_COLOROP, D3DTOP_ADD );
		BATCH->set_texture_stage_state( 1, D3DTSS_ALPHAOP, D3DTOP_SELECTARG2 );
		BATCH->set_texture_stage_texture(1,fgi[fg_idx].emissive_texture_id);

		// addressing - clamped
		BATCH->set_sampler_state( 1, D3DSAMP_ADDRESSU, MAT_GET_ADDR_MODE(fgi[fg_idx].emissive_texture_flags,0) );
		BATCH->set_sampler_state( 1, D3DSAMP_ADDRESSV, MAT_GET_ADDR_MODE(fgi[fg_idx].emissive_texture_flags,1) );

		BATCH->set_texture_stage_state( 1, D3DTSS_TEXCOORDINDEX, 1);
	//	BATCH->set_state(RPR_STATE_ID,U32(fgi[fg_idx].unique_id)+unique);
	}
	else
	{
		BATCH->set_texture_stage_state( 1, D3DTSS_COLOROP, D3DTOP_DISABLE );
		BATCH->set_texture_stage_state( 1, D3DTSS_ALPHAOP, D3DTOP_DISABLE );
	//	BATCH->set_state(RPR_STATE_ID,U32(fgi[fg_idx].unique_id)+unique+4);
	}
}


void FaceGroupRender2::Init(Mesh *mesh,int _fg_idx,MeshRender *_mr)//MeshInfo *mc)
{
	face_offset = face_index_offset;
	vb_offset = tmp_v_cnt;

	Vertex2 *tmp_verts = tmp_verts2;
	
	CQASSERT(faces == 0);//_norm_list == 0);

#define MULTI 24
	mr = _mr;
	fg_idx = _fg_idx;
	fg = &mesh->face_groups[fg_idx];
	int i;
	
	U16 id_list[HUGE];
	ArchetypeFaceInfo afi[500];
	
	U16 refList[HUGE][MULTI];
	memset(refList,0xff,sizeof(U16)*HUGE*MULTI);
	U16 currentRef,lastRef;
	currentRef=lastRef=0;//mr->vert_cnt2;
	int ref_cnt=0;
	new_face_cnt=0;
	for (i=0;i<fg->face_cnt;i++)
	{
		U32 ref[3],tref[3],tref2[3];
		
	//	face_norm_list[i] = mesh->normal_ABC[fg->face_normal[i]];
		
		ref[0] = mesh->vertex_batch_list[fg->face_vertex_chain[i*3]];
		ref[1] = mesh->vertex_batch_list[fg->face_vertex_chain[i*3+1]];
		ref[2] = mesh->vertex_batch_list[fg->face_vertex_chain[i*3+2]];
		
		tref2[0] = tref[0] = mesh->texture_batch_list[fg->face_vertex_chain[i*3]];
		tref2[1] = tref[1] = mesh->texture_batch_list[fg->face_vertex_chain[i*3+1]];
		tref2[2] = tref[2] = mesh->texture_batch_list[fg->face_vertex_chain[i*3+2]];

		CQASSERT(tref[2] < (U32)mesh->texture_vertex_cnt);

		if (mesh->texture_batch_list2 && ((flags & MM_CHROME) == 0))
		{
			tref2[0] = mesh->texture_batch_list2[fg->face_vertex_chain[i*3]];
			tref2[1] = mesh->texture_batch_list2[fg->face_vertex_chain[i*3+1]];
			tref2[2] = mesh->texture_batch_list2[fg->face_vertex_chain[i*3+2]];
		}

		int num_sides=1;
		if (fg->face_properties[i] & TWO_SIDED)
			num_sides = 2;
		
		for (int sides=0;sides<num_sides;sides++)
		{
			Vector face_norm = mesh->normal_ABC[fg->face_normal[i]];
			if (sides)
				face_norm *= -1;

			for (int vv = 0;vv<3;vv++)
			{
				Vector this_normal;
				if (fg->face_properties[i] & FLAT_SHADED)
					this_normal = face_norm;
				else
				{
					this_normal = mesh->normal_ABC[mesh->vertex_normal[ref[vv]]];

					if (sides)
						this_normal *= -1;
				}

				afi[new_face_cnt].norm = face_norm;
//				afi[new_face_cnt].D = fg->face_D_coefficient[i];

				int idx = 0;

				//broken
			//	nref[ref_cnt] = mesh->vertex_normal[ref[vv]];
				if (refList[ref[vv]][0] != 0xffff)
				{
					currentRef = refList[ref[vv]][idx];
					while (currentRef != 0xffff && 
						(tmp_verts[currentRef].u != mesh->texture_vertex_list[tref[vv]].u || tmp_verts[currentRef].v != mesh->texture_vertex_list[tref[vv]].v || 
						(tmp_verts[currentRef].u2 != mesh->texture_vertex_list[tref2[vv]].u || tmp_verts[currentRef].v2 != mesh->texture_vertex_list[tref2[vv]].v) ||
						(tmp_n[currentRef] - this_normal).magnitude_squared() > 1e-5))
					{
						idx++;
#if (defined(_DREW) || defined(_ROB))
						CQASSERT(idx != 8);
#endif
						CQASSERT(idx < MULTI);
						currentRef = refList[ref[vv]][idx];
					}
				}
				
				if (refList[ref[vv]][idx] == 0xffff)
				{
					if (idx >= 14)
						CQBOMB1("One vertex becomes %i - ignorable",idx);
					currentRef = lastRef;
					lastRef++;
					refList[ref[vv]][idx] = currentRef;
					//do this if pos_list should be parallel to src_verts
					tmp_v[tmp_v_cnt++] = mesh->object_vertex_list[ref[vv]];
					//
				}

				tmp_verts[currentRef].pos = mesh->object_vertex_list[ref[vv]];
				tmp_verts[currentRef].u = mesh->texture_vertex_list[tref[vv]].u;
				tmp_verts[currentRef].v = mesh->texture_vertex_list[tref[vv]].v;
				tmp_verts[currentRef].u2 = mesh->texture_vertex_list[tref2[vv]].u;
				tmp_verts[currentRef].v2 = mesh->texture_vertex_list[tref2[vv]].v;
				tmp_verts[currentRef].a = 255;
				id_list[ref_cnt] = currentRef;
				
				tmp_n[currentRef] = this_normal;

				CQASSERT(currentRef < HUGE);
				CQASSERT(ref_cnt < HUGE);

				ref_cnt++;
			}
			
			//if this is the second side of a double-sided poly, rewind.
			if (sides)
			{

				//do norms here too if re-enabled
				int temp=id_list[ref_cnt-2];
				id_list[ref_cnt-2] = id_list[ref_cnt-1];
				id_list[ref_cnt-1] = temp;
			}
			new_face_cnt++;
		}
	}

	vert_cnt = lastRef;
	src_norms = new Vector[lastRef];
	memcpy(src_norms,tmp_n,lastRef*sizeof(Vector));
	src_verts_buffer = (void *)(new Vertex2[lastRef]);
	memcpy(src_verts_buffer,tmp_verts,lastRef*sizeof(Vertex2));
//	pos_list = new Vector[lastRef];
//	memcpy(pos_list,tmp_v,lastRef*sizeof(Vector));
	index_list = new U16[ref_cnt];
	memcpy(index_list,id_list,ref_cnt*sizeof(U16));
//	faces = new ArchetypeFaceInfo[new_face_cnt];
	//pad for cache read-ahead
	faces_ptr = (U8 *)malloc(sizeof(ArchetypeFaceInfo)*(new_face_cnt+1)+31);
	faces = (ArchetypeFaceInfo *)((U32(faces_ptr)+31) & ~31);
	memcpy(faces,afi,new_face_cnt*sizeof(ArchetypeFaceInfo));

//	CQASSERT(HEAP->EnumerateBlocks());
	//	result->sortVertsOnZ();

	///TEST
/*	Vertex2 *vbuffer;
	U32 size;
	PIPE->lock_vertex_buffer(mr->vb_handle,DDLOCK_WRITEONLY | DDLOCK_NOOVERWRITE,(void **)&vbuffer,&size);
	memcpy(&vbuffer[vb_offset],src_verts_buffer,vert_cnt*sizeof(Vertex2));
	PIPE->unlock_vertex_buffer(mr->vb_handle);*/
}

void FaceGroupRender2::Clone(RenderMaterial **rm,bool bCopyBuffers)
{
	FaceGroupRender2 *fgr;
	*rm = fgr = new FaceGroupRender2;
	fgr->mr = mr;
	fgr->fg_idx = fg_idx;
	fgr->fg = fg;
	fgr->flags = flags;
	fgr->new_face_cnt=new_face_cnt;
	fgr->vert_cnt = vert_cnt;
	fgr->face_offset = face_offset;
	if (bCopyBuffers)
	{
		fgr->src_verts_buffer = (void *)(new Vertex2[vert_cnt]);
		memcpy(fgr->src_verts_buffer,src_verts_buffer,vert_cnt*sizeof(Vertex2));
//		fgr->pos_list = new Vector[vert_cnt];
//		memcpy(fgr->pos_list,pos_list,vert_cnt*sizeof(Vector));
		fgr->src_norms = new Vector[vert_cnt];
		memcpy(fgr->src_norms,src_norms,vert_cnt*sizeof(Vector));
		fgr->index_list = new U16[new_face_cnt*3];
		memcpy(fgr->index_list,index_list,new_face_cnt*3*sizeof(U16));
		
		//pad for cache read-ahead
		fgr->faces_ptr = (U8 *)malloc(sizeof(ArchetypeFaceInfo)*(new_face_cnt+1)+31);
		fgr->faces = (ArchetypeFaceInfo *)((U32(faces_ptr)+31) & ~31);
		memcpy(fgr->faces,faces,new_face_cnt*sizeof(ArchetypeFaceInfo));
	}

//	CQASSERT(HEAP->EnumerateBlocks());

}

void FaceGroupRender2::splitEdgeZ(const Vertex2 & v1,const Vertex2 & v2,Vertex2 & v_out,SINGLE z)
{
	SINGLE ratio = (z-v1.pos.z)/(v2.pos.z-v1.pos.z);

	v_out.pos = v1.pos+ratio*(v2.pos-v1.pos);
	v_out.u = v1.u+ratio*(v2.u-v1.u);
	v_out.v = v1.v+ratio*(v2.v-v1.v);
	v_out.u2 = v1.u2+ratio*(v2.u2-v1.u2);
	v_out.v2 = v1.v2+ratio*(v2.v2-v1.v2);
}

void FaceGroupRender2::splitEdge(const Vertex2 & v1,const Vertex2 & v2,Vertex2 & v_out,SINGLE split_d,const Vector &split_n,SINGLE &ratio)
{
	Vector diff = v2.pos-v1.pos;
	Vector delta = v2.pos-split_d*split_n;

	ratio = dot_product(split_n,delta)/dot_product(split_n,diff);

	v_out.pos = v1.pos+ratio*(v2.pos-v1.pos);
	v_out.u = v1.u+ratio*(v2.u-v1.u);
	v_out.v = v1.v+ratio*(v2.v-v1.v);
	v_out.u2 = v1.u2+ratio*(v2.u2-v1.u2);
	v_out.v2 = v1.v2+ratio*(v2.v2-v1.v2);
}

void FaceGroupRender2::splitEdgePlane(const Vertex2 & v1,const Vertex2 & v2,Vertex2 & v_out,const Vector & planePoint,const Vector &split_n)
{
	Vector tmp1 = planePoint-v1.pos;
	Vector tmp2 = v2.pos-v1.pos;

	SINGLE den = dot_product(split_n,tmp2);

	SINGLE ratio = 0;

	if(den != 0)
		ratio = dot_product(split_n,tmp1)/den;

	v_out.pos = v1.pos+ratio*(v2.pos-v1.pos);
	v_out.u = v1.u+ratio*(v2.u-v1.u);
	v_out.v = v1.v+ratio*(v2.v-v1.v);
}

/*
void FaceGroupRender2::GetBuffers(void **vertices,U16 **indices,Vector **norms,ArchetypeFaceInfo **_faces,S32 *num_verts,S32 *num_faces)
{
	*vertices = src_verts;
	*indices = index_list;
	*norms = src_norms;
	*_faces = faces;
	*num_verts = vert_cnt;
	*num_faces = new_face_cnt;

}

void FaceGroupRender2::SetBuffers(void *vertices,U16 *indices,Vector *norms,ArchetypeFaceInfo *_faces,S32 num_verts,S32 num_faces)
{
	src_verts = (Vertex2 *)vertices;
	index_list = indices;
	src_norms = norms;
	faces = _faces;
	vert_cnt = num_verts;
	new_face_cnt = num_faces;
}*/


template <class VertexStruct,U32 VERTEX_FORMAT>
void MMaterial<VertexStruct,VERTEX_FORMAT>::Render(const Transform &inv)
{
	if (vert_cnt==0)
		return;

	BATCH->set_state(RPR_BATCH,TRUE);

	U8 *faceRenders = &active_mi->faceRenders[face_offset];

	S32 index_count;
	U32 stateID = getStateID();
	
	if (hintID == U32(-1) || CQBATCH->ActivateMaterial(stateID,hintID) == false)
	{
		setRenderStates();
		hintID = CQBATCH->CreateMaterial(stateID,D3DPT_TRIANGLELIST,VERTEX_FORMAT);
		bool result = CQBATCH->ActivateMaterial(stateID,hintID);
		CQASSERT(result);
	}

	BATCHDESC desc;
	desc.type = D3DPT_TRIANGLELIST;
	desc.vertex_format = VERTEX_FORMAT;
	desc.num_verts = vert_cnt;
	desc.num_indices = new_face_cnt*3;
	CQBATCH->GetPrimBuffer(&desc);
	VertexStruct *dest_verts = (VertexStruct *)desc.verts;
	U16 *id_list = desc.indices;
	VertexStruct *src_verts = (VertexStruct *)src_verts_buffer;
	

	int i;
	
	index_count = 0;

	
	int v_src=0;
	int v_dst=0;
	CQASSERT(new_face_cnt < HUGE);
	memset(re_index_list,0xff,sizeof(U16)*vert_cnt);//new_face_cnt*3);
	{
		//useless copy to hit cache
		//memcpy(faces,faces,sizeof(ArchetypeFaceInfo)*new_face_cnt);

		float cx = cam_pos_in_object_space.x;
		float cy = cam_pos_in_object_space.y;
		float cz = cam_pos_in_object_space.z;
		for (int j=0;j<new_face_cnt;j++)
		{
			if ((faceRenders[j] & (FS__HIDDEN | FS__BUILDING)) == 0)
			{
				ArchetypeFaceInfo &afi = faces[j];
				SINGLE dot=cx*afi.norm.x+cy*afi.norm.y+cz*afi.norm.z;
				if (dot >= 0)//fg->face_D_coefficient[j])
				{
					for (i=0;i<3;i++)
					{
						v_src = index_list[j*3+i];
						if (re_index_list[v_src] == 0xffff)
						{
							re_index_list[v_src] = v_dst;
							dest_vec[v_dst] = src_verts[v_src].pos;
							dest_verts[v_dst] = src_verts[v_src];
							dest_norm[v_dst] = src_norms[v_src];
							v_dst++;
						}

						id_list[index_count] = re_index_list[v_src];
						index_count++;
					}

					faceRenders[j] &= ~FS__NOT_VISIBLE;
					//memcpy(&id_list[index_count],&index_list[j*3],sizeof(U16)*3);

					//index_count+=3;
				}
				else
					faceRenders[j] |= FS__NOT_VISIBLE;
			}
		}

		CQASSERT(v_dst < HUGE);
		LIGHT->light_vertices_U8(lit,dest_vec,dest_norm,v_dst,&inv);
	}
	
	U32 r,g,b;
	int x;
	FaceGroupInfo &fgi2 = fgi[fg_idx];
	//pseudo assembly
	//for (i=minV;i<=maxV;i++)
	//{
	i=0;
Loop:
		const LightRGB_U8 & lit_p = lit[i];
		VertexStruct & vert = dest_verts[i];

		if (i>=v_dst)
			goto Done;

		x=lit[i+1].r;
		//not 100% accurate, but fast
		r = ((fgi2.diffuse.r*(lit_p.r)) >> 8)+fgi2.emissive.r;
		g = ((fgi2.diffuse.g*(lit_p.g)) >> 8)+fgi2.emissive.g;
		b = ((fgi2.diffuse.b*(lit_p.b)) >> 8)+fgi2.emissive.b;

		if (r>255 || g>255 || b >255)
		{
			r = min(r,255);
			g = min(g,255);
			b = min(b,255);
		}

		vert.r = r;
		vert.g = g;
		vert.b = b;
		vert.a = fgi2.a;
		i++;

		goto Loop;

	//}

Done:

	desc.num_verts = v_dst;
	desc.num_indices = index_count;
	CQBATCH->ReleasePrimBuffer(&desc);

	BATCH->set_state(RPR_STATE_ID,0);
}	


void FaceGroupRender::Render(const Transform &inv)
{
	static bool bSkipRender = 0;
	if (bSkipRender) return;
	if (vert_cnt==0)
		return;
	
	if (false && (active_mi->bBroken || CQRENDERFLAGS.bSoftwareRenderer || index_list4[0]==0 || fgi[fg_idx].texture_flags & TF_F_HAS_ALPHA || fgi[fg_idx].a != 255 || vert_cnt < 50))
	{
		MMaterial<RPVertex,D3DFVF_RPVERTEX>::Render(inv);
		return;
	}

	//return;

	//setRenderStates();
	
	//return;
	/*
	U32 val;
	BATCH->get_state(RPR_BATCH,&val);
	if (val)
	{
		CQBATCH->CommitState();
		BATCH->set_state(RPR_BATCH,FALSE);
	}
	return;
	*/

	U8 *faceRenders = &active_mi->faceRenders[face_offset];

	S32 index_count;
	
	U16 *id_list = indexScratchList;
//	bool vert_rendered[4000];
//	memset(vert_rendered,0,sizeof(bool)*4000);

	U16 *index_list = index_list4[chosenAngle];
	if (index_list == 0) 
		return;
	U32 vb_handle = mr->vb_handle4[chosenAngle];

	int i;
	
	index_count = 0;
	
	int v_src=0;
	int v_dst=0;
	CQASSERT(new_face_cnt < HUGE);
	//return;
//	memset(re_index_list,0xff,sizeof(U16)*new_face_cnt*3);
	/*int v_max=0;
	
	{

		//useless copy to hit cache
		//memcpy(faces,faces,sizeof(ArchetypeFaceInfo)*new_face_cnt);


		float cx = cam_pos_in_object_space.x;
		float cy = cam_pos_in_object_space.y;
		float cz = cam_pos_in_object_space.z;
		for (int j=0;j<new_face_cnt;j++)
		{
			if ((faceRenders[j] & (FS__HIDDEN | FS__BUILDING)) == 0)
			{
				ArchetypeFaceInfo &afi = faces[j];
				//SINGLE dot=cx*afi.norm.x+cy*afi.norm.y+cz*afi.norm.z;
				//if (dot >= 0)//-afi.D)//fg->face_D_coefficient[j])
				{
					for (i=0;i<3;i++)
					{
						v_src = index_list[j*3+i];
	
						id_list[index_count] = v_src;
						index_count++;
						if (v_src > v_max)
							v_max = v_src;
					}

					faceRenders[j] &= ~FS__NOT_VISIBLE;
					//memcpy(&id_list[index_count],&index_list[j*3],sizeof(U16)*3);
					//index_count+=3;
				}
				//else
					//faceRenders[j] |= FS__NOT_VISIBLE;
			}
		}

		CQASSERT(v_dst < HUGE);
	//	LIGHT->light_vertices_U8(lit,dest_vec,dest_norm,v_dst,&inv);
	}*/
	//return;
	//if (index_count)
	{
	//	CQASSERT(v_max < vert_cnt);
		//PIPE->process_vertices(mr->vb_handle

		BATCH->set_render_state(D3DRS_NORMALIZENORMALS,TRUE);
		
		if (fgi[fg_idx].iMat)
		{
			PIPE->set_texture_stage_texture( 0, fgi[fg_idx].colorTex );
			PIPE->set_render_state(D3DRS_LIGHTING,FALSE);
			PIPE->set_render_state(D3DRS_ALPHABLENDENABLE,TRUE);
			PIPE->set_render_state(D3DRS_ALPHATESTENABLE,FALSE);
			PIPE->set_render_state(D3DRS_ALPHAREF,0x0000000A);
			PIPE->set_render_state(D3DRS_SRCBLEND,D3DBLEND_SRCALPHA);
			PIPE->set_render_state(D3DRS_DESTBLEND,D3DBLEND_INVSRCALPHA);
			PIPE->set_render_state(D3DRS_CULLMODE,D3DCULL_CW);
			PIPE->set_render_state(D3DRS_ZENABLE,TRUE);
			PIPE->set_render_state(D3DRS_ZWRITEENABLE,TRUE);
			PIPE->set_render_state(D3DRS_ZFUNC,D3DCMP_LESSEQUAL);
			PIPE->set_texture_stage_state( 0, D3DTSS_TEXCOORDINDEX, 0);
			
			BATCH->set_texture_stage_state(0,D3DTSS_COLOROP,D3DTOP_SELECTARG1);
			BATCH->set_texture_stage_state(0,D3DTSS_COLORARG1,D3DTA_TFACTOR);
			BATCH->set_texture_stage_state(0,D3DTSS_ALPHAOP,D3DTOP_SELECTARG1 );
			BATCH->set_texture_stage_state(0,D3DTSS_ALPHAARG1,D3DTA_TFACTOR);
			BATCH->set_texture_stage_state(1,D3DTSS_COLOROP,D3DTOP_DISABLE);
			
			//GENRESULT result = PIPE->draw_indexed_primitive_vb( D3DPT_TRIANGLELIST, vb_handle, vb_offset, maxVert[0], IB4[0],new_face_cnt*3 , 0 );
			//CQASSERT(result == GR_OK);
			fgi[fg_idx].iMat->DrawVB((IDirect3DVertexBuffer9*)vb_handle, IB4[0], vb_offset, maxVert[0], 0, new_face_cnt*3, NULL);
			cleanupRenderStates();
		}
		else if (/*fgi[fg_idx].colorTex &&*/ !fgi[fg_idx].bCQ2Mat)
		{
			if (this->mr->bSolarianRenderstateKludge) return;

			PIPE->set_texture_stage_texture( 0, fgi[fg_idx].colorTex );
			PIPE->set_render_state(D3DRS_LIGHTING,FALSE);
			PIPE->set_render_state(D3DRS_ALPHABLENDENABLE,FALSE);
			PIPE->set_render_state(D3DRS_ALPHATESTENABLE,FALSE);
			PIPE->set_render_state(D3DRS_ALPHAREF,0x0000000A);
			PIPE->set_render_state(D3DRS_SRCBLEND,D3DBLEND_SRCALPHA);
			PIPE->set_render_state(D3DRS_DESTBLEND,D3DBLEND_INVSRCALPHA);
			PIPE->set_render_state(D3DRS_CULLMODE,D3DCULL_CW);
			PIPE->set_render_state(D3DRS_ZENABLE,TRUE);
			PIPE->set_render_state(D3DRS_ZWRITEENABLE,TRUE);
			PIPE->set_render_state(D3DRS_ZFUNC,D3DCMP_LESSEQUAL);
			PIPE->set_texture_stage_state( 0, D3DTSS_TEXCOORDINDEX, 0);
			
			
			BATCH->set_texture_stage_state(0,D3DTSS_COLOROP,D3DTOP_SELECTARG1);
			BATCH->set_texture_stage_state(0,D3DTSS_COLORARG1,D3DTA_TFACTOR);
			BATCH->set_texture_stage_state(0,D3DTSS_ALPHAOP,D3DTOP_SELECTARG1 );
			BATCH->set_texture_stage_state(0,D3DTSS_ALPHAARG1,D3DTA_TFACTOR);
			BATCH->set_texture_stage_state(1,D3DTSS_COLOROP,D3DTOP_DISABLE);
			// light renders here.  todo: fix it
			GENRESULT result = PIPE->draw_indexed_primitive_vb( D3DPT_TRIANGLELIST, vb_handle, vb_offset, maxVert[0], IB4[0],new_face_cnt*3 , 0 );
			CQASSERT(result == GR_OK);
			cleanupRenderStates();

			return;
		}
		else if (fgi[fg_idx].colorTex)
		{
			for (int i = 0; i < 4; i++)
			{
				PIPE->set_sampler_state( i, D3DSAMP_ADDRESSU, MAT_GET_ADDR_MODE(fgi[fg_idx].texture_flags,0) );
				PIPE->set_sampler_state( i, D3DSAMP_ADDRESSV, MAT_GET_ADDR_MODE(fgi[fg_idx].texture_flags,1) );
				PIPE->set_sampler_state( i, D3DSAMP_MINFILTER,		D3DTEXF_LINEAR );
				PIPE->set_sampler_state( i, D3DSAMP_MAGFILTER,		D3DTEXF_LINEAR );
				PIPE->set_sampler_state( i, D3DSAMP_MIPFILTER,		D3DTEXF_LINEAR);
			}

			//PIPE->set_texture_stage_texture( 3, testLightCurves	);
			PIPE->set_render_state(D3DRS_LIGHTING,FALSE);
			PIPE->set_render_state(D3DRS_ALPHATESTENABLE,true);
			PIPE->set_render_state(D3DRS_ALPHAREF,0x0000000A);
			PIPE->set_render_state(D3DRS_CULLMODE,D3DCULL_CW);
			PIPE->set_render_state(D3DRS_ZENABLE,TRUE);
			PIPE->set_render_state(D3DRS_ZWRITEENABLE,TRUE);
			PIPE->set_render_state(D3DRS_ZFUNC,D3DCMP_LESSEQUAL);
			
			static U32 count = 0;
			static bool bDebugShader = false;
			if (bDebugShader && count%300 == 0) 
			{
				emissiveEffect = PIPE->load_effect("cq2\\shaders\\emissive.fx",(IComponentFactory *)TEXTURESDIR);
				nonEmissiveEffect = PIPE->load_effect("cq2\\shaders\\noEmissive.fx",(IComponentFactory *)TEXTURESDIR);

			}
			float emissiveColor [] = { fgi[fg_idx].emissive.r/255.0, 
									   fgi[fg_idx].emissive.g/255.0, 
									   fgi[fg_idx].emissive.b/255.0,
									   0};
			if ((emissiveColor[0] > .7 && emissiveColor[1] > .7 && emissiveColor[2] > .7)
				|| (emissiveColor[0] <= .2 && emissiveColor[1]<= .2 && emissiveColor[2]<= .2))
			{
				emissiveColor[0] = emissiveColor[1] = emissiveColor[2] = 0;
			}


			PIPE->set_ps_constants(2, (float*)&emissiveColor,1);
			if (emissiveColor[0] <= 0 && emissiveColor[1] == 0 && emissiveColor[2] == 0)
			{
				emissiveColor[0] = emissiveColor[1] = emissiveColor[2] = 1;
			}
			else
			{
				emissiveColor[0] *= 2;
				emissiveColor[1] *= 2;
				emissiveColor[2] *= 2;
			}
			//return;
			int alphaMod = (colorMod & 0x000000FF);

			alphaMod *= fgi[fg_idx].a;
			alphaMod /= 255;
			emissiveColor[3] = alphaMod / 255.0;

			PIPE->set_ps_constants(3, (float*)&emissiveColor,1);
			PIPE->set_render_state(D3DRS_NORMALIZENORMALS,TRUE);

			//return;
			count++;
			UINT passes;
			ID3DXEffect * chosenEffect;
			
			if (bDebugShader) chosenEffect = *nonEmissiveEffect;
			else chosenEffect = fgi[fg_idx].glowTex ? *emissiveEffect : *nonEmissiveEffect;

			//ID3DXEffect * chosenEffect = *nonEmissiveEffect;
			chosenEffect->Begin(&passes, 0);
			PIPE->set_render_state(D3DRS_ALPHABLENDENABLE,TRUE);

			PIPE->set_render_state(D3DRS_SRCBLEND,D3DBLEND_SRCALPHA);
			PIPE->set_render_state(D3DRS_DESTBLEND,D3DBLEND_INVSRCALPHA);
	
			if (passes == 1)
			{
				PIPE->set_texture_stage_texture( 0, fgi[fg_idx].colorTex );
				PIPE->set_texture_stage_texture( 1, fgi[fg_idx].bumpTex );
				PIPE->set_texture_stage_texture( 2, fgi[fg_idx].glowTex	);
				PIPE->set_texture_stage_state( 0, D3DTSS_TEXCOORDINDEX, 0);
				PIPE->set_texture_stage_state( 1, D3DTSS_TEXCOORDINDEX, 0);
				PIPE->set_texture_stage_state( 2, D3DTSS_TEXCOORDINDEX, 1);
				chosenEffect->Pass(0);
				GENRESULT result = PIPE->draw_indexed_primitive_vb( D3DPT_TRIANGLELIST, vb_handle, vb_offset, maxVert[0], IB4[0],new_face_cnt*3 , 0 );
				CQASSERT(result == GR_OK);
			}
			else if (passes == 2)  // for the pixelshader-poor
			{
				//PIPE->set_render_state(D3DRS_ALPHAREF,0x00000001);
				PIPE->set_render_state(D3DRS_TEXTUREFACTOR,D3DCOLOR_XRGB(fgi[fg_idx].emissive.r,fgi[fg_idx].emissive.g,fgi[fg_idx].emissive.b));
				PIPE->set_texture_stage_texture( 0, fgi[fg_idx].bumpTex );
				PIPE->set_texture_stage_texture( 1, fgi[fg_idx].colorTex );
				
				PIPE->set_texture_stage_state( 0, D3DTSS_TEXCOORDINDEX, 0);
				PIPE->set_texture_stage_state( 1, D3DTSS_TEXCOORDINDEX, 0);
				chosenEffect->Pass(0);
				GENRESULT result = PIPE->draw_indexed_primitive_vb( D3DPT_TRIANGLELIST, vb_handle, vb_offset, maxVert[0], IB4[0],new_face_cnt*3 , 0 );
				CQASSERT(result == GR_OK);

				PIPE->set_render_state(D3DRS_TEXTUREFACTOR,D3DCOLOR_XRGB((int)(emissiveColor[0]*160),(int)(emissiveColor[1]*160),(int)(emissiveColor[2]*160)));
				
				PIPE->set_render_state(D3DRS_SRCBLEND,D3DBLEND_ONE);
				PIPE->set_render_state(D3DRS_DESTBLEND,D3DBLEND_ONE);
				PIPE->set_texture_stage_state( 1, D3DTSS_TEXCOORDINDEX, 1);
				PIPE->set_texture_stage_texture( 1, fgi[fg_idx].glowTex	);

				//PIPE->set_render_state(D3DRS_ZFUNC,D3DCMP_EQUAL);
				chosenEffect->Pass(1);
				result = PIPE->draw_indexed_primitive_vb( D3DPT_TRIANGLELIST, vb_handle, vb_offset, maxVert[0], IB4[0],new_face_cnt*3 , 0 );
				CQASSERT(result == GR_OK);
			}
			chosenEffect->End();

			//return;
			cleanupRenderStates();
			return;
		}
	}
}

template <class VertexStruct,U32 VERTEX_FORMAT>
void MMaterial<VertexStruct,VERTEX_FORMAT>::RenderPortionZAlign(const Transform &inv,const Vector &split_pt)
{
	if (vert_cnt==0)
		return;

	S32 index_count;

	setRenderStates();
	U32 val;
	BATCH->get_state(RPR_BATCH,&val);
	if (val)
	{
		CQBATCH->CommitState();
		BATCH->set_state(RPR_BATCH,FALSE);
	}

	//BATCHDESC desc;
	//desc.type = D3DPT_TRIANGLELIST;
	//desc.vertex_format = VERTEX_FORMAT;
	//desc.num_verts = vert_cnt*2;
	//desc.num_indices = new_face_cnt*6;
	//CQBATCH->GetPrimBuffer(&desc);
	VBVertex *dest_verts = tmp_VBVertex;
	U16 *id_list = indexScratchList;
	VertexStruct *src_verts = (VertexStruct *)src_verts_buffer;

	int ref1[3],ref2[3];
	int out_ref[4];
	int cnt1,cnt2;
	

	int i;
	
	index_count = 0;
	
	int v_src=0;
	int v_dst=0;
	CQASSERT(new_face_cnt < HUGE);
	memset(re_index_list,0xff,sizeof(U16)*vert_cnt);//new_face_cnt*3);
	{
		//useless copy to hit cache
		//memcpy(faces,faces,sizeof(ArchetypeFaceInfo)*new_face_cnt);

		float cx = cam_pos_in_object_space.x;
		float cy = cam_pos_in_object_space.y;
		float cz = cam_pos_in_object_space.z;
		for (int j=0;j<new_face_cnt;j++)
		{
			ArchetypeFaceInfo &afi = faces[j];
			SINGLE dot=cx*afi.norm.x+cy*afi.norm.y+cz*afi.norm.z;
			if (dot >= 0)//-afi.D)//fg->face_D_coefficient[j])
			{
				//put the vertices on either side
				cnt1 = cnt2 = 0;
				for (i=0;i<3;i++)
				{
					v_src = index_list[j*3+i];
					if (src_verts[v_src].pos.z > split_pt.z)
						ref1[cnt1++] = v_src;
					else
						ref2[cnt2++] = v_src;
				}

				if (cnt1)
				{
					int out=0;
					for (i=0;i<cnt1;i++)
					{
						v_src = ref1[i];
						if (re_index_list[v_src] == 0xffff)
						{
							re_index_list[v_src] = v_dst;
							dest_vec[v_dst] = src_verts[v_src].pos;
							dest_verts[v_dst].pos = src_verts[v_src].pos;
							dest_verts[v_dst].u = src_verts[v_src].u;
							dest_verts[v_dst].v = 1.0-src_verts[v_src].v;
							dest_verts[v_dst].u2 = src_verts[v_src].u;
							dest_verts[v_dst].v2 = 1.0-src_verts[v_src].v;
							//dest_verts[v_dst] = src_verts[v_src];
							dest_verts[v_dst].norm = dest_norm[v_dst] = src_norms[v_src];
							v_dst++;
						}
						out_ref[out++] = re_index_list[v_src];
						for (int k=0;k<cnt2;k++)
						{
							VertexStruct tmp;
							splitEdgeZ(src_verts[v_src],src_verts[ref2[k]],tmp,split_pt.z);
							dest_verts[v_dst].pos = tmp.pos;
							dest_verts[v_dst].u = tmp.u;
							dest_verts[v_dst].v = 1.0-tmp.v;
							dest_verts[v_dst].u2 = tmp.u;
							dest_verts[v_dst].v2 = 1.0-tmp.v;
							dest_vec[v_dst] = dest_verts[v_dst].pos;
							dest_verts[v_dst].norm = dest_norm[v_dst] = afi.norm;
							
							out_ref[out++] = v_dst;
							v_dst++;
						}
					}
					
					id_list[index_count++] = out_ref[0];
					id_list[index_count++] = out_ref[1];
					id_list[index_count++] = out_ref[2];
					
					if (out == 4)
					{
						id_list[index_count++] = out_ref[0];
						id_list[index_count++] = out_ref[2];
						id_list[index_count++] = out_ref[3];
					}
				}
				
			}
		}

		CQASSERT(v_dst < HUGE);
//		LIGHT->light_vertices_U8(lit,dest_vec,dest_norm,v_dst,&inv);
	}
	

	if (fgi[fg_idx].colorTex)
		{
			for (int i = 0; i < 4; i++)
			{
				PIPE->set_sampler_state( i, D3DSAMP_ADDRESSU, MAT_GET_ADDR_MODE(fgi[fg_idx].texture_flags,0) );
				PIPE->set_sampler_state( i, D3DSAMP_ADDRESSV, MAT_GET_ADDR_MODE(fgi[fg_idx].texture_flags,1) );
				PIPE->set_sampler_state( i, D3DSAMP_MINFILTER,		D3DTEXF_LINEAR );
				PIPE->set_sampler_state( i, D3DSAMP_MAGFILTER,		D3DTEXF_LINEAR );
				PIPE->set_sampler_state( i, D3DSAMP_MIPFILTER,		D3DTEXF_LINEAR);
			}

			PIPE->set_texture_stage_texture( 0, fgi[fg_idx].colorTex );
			PIPE->set_texture_stage_texture( 1, fgi[fg_idx].bumpTex );
			PIPE->set_texture_stage_texture( 2, fgi[fg_idx].glowTex	);
			PIPE->set_render_state(D3DRS_LIGHTING,FALSE);
			PIPE->set_render_state(D3DRS_ALPHABLENDENABLE,TRUE);
			PIPE->set_render_state(D3DRS_ALPHATESTENABLE,true);
			PIPE->set_render_state(D3DRS_ALPHAREF,0x0000000A);
			PIPE->set_render_state(D3DRS_SRCBLEND,D3DBLEND_SRCALPHA);
			PIPE->set_render_state(D3DRS_DESTBLEND,D3DBLEND_INVSRCALPHA);
			PIPE->set_render_state(D3DRS_CULLMODE,D3DCULL_CW);
			PIPE->set_render_state(D3DRS_ZENABLE,TRUE);
			PIPE->set_render_state(D3DRS_ZWRITEENABLE,TRUE);
			PIPE->set_render_state(D3DRS_ZFUNC,D3DCMP_LESSEQUAL);
			
			float emissiveColor [] = { fgi[fg_idx].emissive.r/255.0, 
									   fgi[fg_idx].emissive.g/255.0, 
									   fgi[fg_idx].emissive.b/255.0,
									   0};
			if ((emissiveColor[0] > .7 && emissiveColor[1] > .7 && emissiveColor[2] > .7)
				|| (emissiveColor[0] <= .2 && emissiveColor[1]<= .2 && emissiveColor[2]<= .2))
			{
				emissiveColor[0] = emissiveColor[1] = emissiveColor[2] = 0;
			}

			emissiveColor[3] = 1.0;

			PIPE->set_ps_constants(2, (float*)&emissiveColor,1);
			if (emissiveColor[0] <= 0 && emissiveColor[1] == 0 && emissiveColor[2] == 0)
			{
				emissiveColor[0] = emissiveColor[1] = emissiveColor[2] = 1;
			}
			else
			{
				emissiveColor[0] *= 2;
				emissiveColor[1] *= 2;
				emissiveColor[2] *= 2;
			}
			int alphaMod = (colorMod & 0x000000FF);

			alphaMod *= fgi[fg_idx].a;
			alphaMod /= 255;
			emissiveColor[3] = 1.0;

			PIPE->set_ps_constants(3, (float*)&emissiveColor,1);
			PIPE->set_render_state(D3DRS_NORMALIZENORMALS,TRUE);

			PIPE->set_texture_stage_state( 0, D3DTSS_TEXCOORDINDEX, 0);
			PIPE->set_texture_stage_state( 1, D3DTSS_TEXCOORDINDEX, 0);
			PIPE->set_texture_stage_state( 2, D3DTSS_TEXCOORDINDEX, 1);
			UINT passes;
			ID3DXEffect * chosenEffect = fgi[fg_idx].glowTex ? *emissiveEffect : *nonEmissiveEffect;
			chosenEffect->Begin(&passes, 0);
			if (passes == 1)
			{
				PIPE->set_texture_stage_texture( 0, fgi[fg_idx].colorTex );
				PIPE->set_texture_stage_texture( 1, fgi[fg_idx].bumpTex );
				PIPE->set_texture_stage_texture( 2, fgi[fg_idx].glowTex	);
				PIPE->set_texture_stage_state( 0, D3DTSS_TEXCOORDINDEX, 0);
				PIPE->set_texture_stage_state( 1, D3DTSS_TEXCOORDINDEX, 0);
				PIPE->set_texture_stage_state( 2, D3DTSS_TEXCOORDINDEX, 1);
				chosenEffect->Pass(0);
				PIPE->draw_indexed_primitive(D3DPT_TRIANGLELIST , D3DFVF_VBVERTEX, dest_verts, v_dst, id_list, index_count,0 );		
			}
			else if (passes == 2)  // for the pixelshader-poor
			{
				//PIPE->set_render_state(D3DRS_ALPHAREF,0x00000001);
				PIPE->set_render_state(D3DRS_TEXTUREFACTOR,D3DCOLOR_XRGB(fgi[fg_idx].emissive.r,fgi[fg_idx].emissive.g,fgi[fg_idx].emissive.b));
				PIPE->set_texture_stage_texture( 0, fgi[fg_idx].bumpTex );
				PIPE->set_texture_stage_texture( 1, fgi[fg_idx].colorTex );
				
				PIPE->set_texture_stage_state( 0, D3DTSS_TEXCOORDINDEX, 0);
				PIPE->set_texture_stage_state( 1, D3DTSS_TEXCOORDINDEX, 0);
				chosenEffect->Pass(0);
				PIPE->draw_indexed_primitive(D3DPT_TRIANGLELIST , D3DFVF_VBVERTEX, dest_verts, v_dst, id_list, index_count,0 );		
				PIPE->set_render_state(D3DRS_TEXTUREFACTOR,D3DCOLOR_XRGB((int)(emissiveColor[0]*160),(int)(emissiveColor[1]*160),(int)(emissiveColor[2]*160)));
				
				PIPE->set_render_state(D3DRS_SRCBLEND,D3DBLEND_ONE);
				PIPE->set_render_state(D3DRS_DESTBLEND,D3DBLEND_ONE);
				PIPE->set_texture_stage_state( 1, D3DTSS_TEXCOORDINDEX, 1);
				PIPE->set_texture_stage_texture( 1, fgi[fg_idx].glowTex	);

				//PIPE->set_render_state(D3DRS_ZFUNC,D3DCMP_EQUAL);
				chosenEffect->Pass(1);
				PIPE->draw_indexed_primitive(D3DPT_TRIANGLELIST , D3DFVF_VBVERTEX, dest_verts, v_dst, id_list, index_count,0 );		
			}
			chosenEffect->End();
			//return;
			cleanupRenderStates();
			return;
	}
}


//VBVertex tmp_VBVertex
//U16 indexScratchList

template <class VertexStruct,U32 VERTEX_FORMAT>
void MMaterial<VertexStruct,VERTEX_FORMAT>::RenderPortion(const Transform &inv,const Vector &split_pt, const Vector & split_normal)
{
	if (vert_cnt==0)
		return;

	S32 index_count;

	setRenderStates();
	U32 val;
	BATCH->get_state(RPR_BATCH,&val);
	if (val)
	{
		CQBATCH->CommitState();
		BATCH->set_state(RPR_BATCH,FALSE);
	}

	VBVertex *dest_verts = tmp_VBVertex;
	U16 *id_list = indexScratchList;
	VertexStruct *src_verts = (VertexStruct *)src_verts_buffer;

	int ref1[3],ref2[3];
	int out_ref[4];
	int cnt1,cnt2;
	

	int i;
	
	index_count = 0;
	
	int v_src=0;
	int v_dst=0;
	CQASSERT(new_face_cnt < HUGE);
	memset(re_index_list,0xff,sizeof(U16)*vert_cnt);//new_face_cnt*3);
	{
		//useless copy to hit cache
		//memcpy(faces,faces,sizeof(ArchetypeFaceInfo)*new_face_cnt);

		float cx = cam_pos_in_object_space.x;
		float cy = cam_pos_in_object_space.y;
		float cz = cam_pos_in_object_space.z;
		for (int j=0;j<new_face_cnt;j++)
		{
			ArchetypeFaceInfo &afi = faces[j];
			SINGLE dot=cx*afi.norm.x+cy*afi.norm.y+cz*afi.norm.z;
			if (dot >= 0)//-afi.D)//fg->face_D_coefficient[j])
			{
				//put the vertices on either side
				cnt1 = cnt2 = 0;
				for (i=0;i<3;i++)
				{
					v_src = index_list[j*3+i];
					Vector srcDir = (split_pt-src_verts[v_src].pos).fast_normalize();
					if (dot_product(srcDir,split_normal) < 0)
						ref1[cnt1++] = v_src;
					else
						ref2[cnt2++] = v_src;
				}

				if (cnt1)
				{
					int out=0;
					for (i=0;i<cnt1;i++)
					{
						v_src = ref1[i];
						if (re_index_list[v_src] == 0xffff)
						{
							re_index_list[v_src] = v_dst;
							dest_vec[v_dst] = src_verts[v_src].pos;

							dest_verts[v_dst].pos = src_verts[v_src].pos;
							dest_verts[v_dst].u = src_verts[v_src].u;
							dest_verts[v_dst].v = 1.0-src_verts[v_src].v;
							dest_verts[v_dst].u2 = src_verts[v_src].u;
							dest_verts[v_dst].v2 = 1.0-src_verts[v_src].v;
						
							//dest_verts[v_dst] = src_verts[v_src];
							dest_verts[v_dst].norm = dest_norm[v_dst] = src_norms[v_src];
							v_dst++;
						}
						out_ref[out++] = re_index_list[v_src];
						for (int k=0;k<cnt2;k++)
						{
							VertexStruct tmp;
							splitEdgePlane(src_verts[v_src],src_verts[ref2[k]],tmp, split_pt, split_normal);
							dest_verts[v_dst].pos = tmp.pos;
							dest_verts[v_dst].u = tmp.u;
							dest_verts[v_dst].v = tmp.v;
							dest_verts[v_dst].u2 = tmp.u;
							dest_verts[v_dst].v2 = tmp.v;
							dest_vec[v_dst] = dest_verts[v_dst].pos;
							dest_verts[v_dst].norm = dest_norm[v_dst] = afi.norm;
							
							out_ref[out++] = v_dst;
							v_dst++;
						}
					}
					
					id_list[index_count++] = out_ref[0];
					id_list[index_count++] = out_ref[1];
					id_list[index_count++] = out_ref[2];
					
					if (out == 4)
					{
						id_list[index_count++] = out_ref[0];
						id_list[index_count++] = out_ref[2];
						id_list[index_count++] = out_ref[3];
					}
				}
				
			}
		}

		CQASSERT(v_dst < HUGE);
	}
	


	if (fgi[fg_idx].colorTex)
		{
			for (int i = 0; i < 4; i++)
			{
				PIPE->set_sampler_state( i, D3DSAMP_ADDRESSU, MAT_GET_ADDR_MODE(fgi[fg_idx].texture_flags,0) );
				PIPE->set_sampler_state( i, D3DSAMP_ADDRESSV, MAT_GET_ADDR_MODE(fgi[fg_idx].texture_flags,1) );
				PIPE->set_sampler_state( i, D3DSAMP_MINFILTER,		D3DTEXF_LINEAR );
				PIPE->set_sampler_state( i, D3DSAMP_MAGFILTER,		D3DTEXF_LINEAR );
				PIPE->set_sampler_state( i, D3DSAMP_MIPFILTER,		D3DTEXF_LINEAR);
			}

			PIPE->set_texture_stage_texture( 0, fgi[fg_idx].colorTex );
			PIPE->set_texture_stage_texture( 1, fgi[fg_idx].bumpTex );
			PIPE->set_texture_stage_texture( 2, fgi[fg_idx].glowTex	);
			PIPE->set_render_state(D3DRS_LIGHTING,FALSE);
			PIPE->set_render_state(D3DRS_ALPHABLENDENABLE,TRUE);
			PIPE->set_render_state(D3DRS_ALPHATESTENABLE,true);
			PIPE->set_render_state(D3DRS_ALPHAREF,0x0000000A);
			PIPE->set_render_state(D3DRS_SRCBLEND,D3DBLEND_SRCALPHA);
			PIPE->set_render_state(D3DRS_DESTBLEND,D3DBLEND_INVSRCALPHA);
			PIPE->set_render_state(D3DRS_CULLMODE,D3DCULL_CW);
			PIPE->set_render_state(D3DRS_ZENABLE,TRUE);
			PIPE->set_render_state(D3DRS_ZWRITEENABLE,TRUE);
			PIPE->set_render_state(D3DRS_ZFUNC,D3DCMP_LESSEQUAL);
			
			float emissiveColor [] = { fgi[fg_idx].emissive.r/255.0, 
									   fgi[fg_idx].emissive.g/255.0, 
									   fgi[fg_idx].emissive.b/255.0,
									   0};
			if ((emissiveColor[0] > .7 && emissiveColor[1] > .7 && emissiveColor[2] > .7)
				|| (emissiveColor[0] <= .2 && emissiveColor[1]<= .2 && emissiveColor[2]<= .2))
			{
				emissiveColor[0] = emissiveColor[1] = emissiveColor[2] = 0;
			}

			emissiveColor[3] = 1.0;

			PIPE->set_ps_constants(2, (float*)&emissiveColor,1);
			if (emissiveColor[0] <= 0 && emissiveColor[1] == 0 && emissiveColor[2] == 0)
			{
				emissiveColor[0] = emissiveColor[1] = emissiveColor[2] = 1;
			}
			else
			{
				emissiveColor[0] *= 2;
				emissiveColor[1] *= 2;
				emissiveColor[2] *= 2;
			}
			int alphaMod = (colorMod & 0x000000FF);

			alphaMod *= fgi[fg_idx].a;
			alphaMod /= 255;
			emissiveColor[3] = 1.0;

			PIPE->set_ps_constants(3, (float*)&emissiveColor,1);
			PIPE->set_render_state(D3DRS_NORMALIZENORMALS,TRUE);

			PIPE->set_texture_stage_state( 0, D3DTSS_TEXCOORDINDEX, 0);
			PIPE->set_texture_stage_state( 1, D3DTSS_TEXCOORDINDEX, 0);
			PIPE->set_texture_stage_state( 2, D3DTSS_TEXCOORDINDEX, 1);
			UINT passes;
			ID3DXEffect * chosenEffect = fgi[fg_idx].glowTex ? *emissiveEffect : *nonEmissiveEffect;
			chosenEffect->Begin(&passes, 0);
			if (passes == 1)
			{
				PIPE->set_texture_stage_texture( 0, fgi[fg_idx].colorTex );
				PIPE->set_texture_stage_texture( 1, fgi[fg_idx].bumpTex );
				PIPE->set_texture_stage_texture( 2, fgi[fg_idx].glowTex	);
				PIPE->set_texture_stage_state( 0, D3DTSS_TEXCOORDINDEX, 0);
				PIPE->set_texture_stage_state( 1, D3DTSS_TEXCOORDINDEX, 0);
				PIPE->set_texture_stage_state( 2, D3DTSS_TEXCOORDINDEX, 1);
				chosenEffect->Pass(0);
				PIPE->draw_indexed_primitive(D3DPT_TRIANGLELIST , D3DFVF_VBVERTEX, dest_verts, v_dst, id_list, index_count,0 );		
			}
			else if (passes == 2)  // for the pixelshader-poor
			{
				//PIPE->set_render_state(D3DRS_ALPHAREF,0x00000001);
				PIPE->set_render_state(D3DRS_TEXTUREFACTOR,D3DCOLOR_XRGB(fgi[fg_idx].emissive.r,fgi[fg_idx].emissive.g,fgi[fg_idx].emissive.b));
				PIPE->set_texture_stage_texture( 0, fgi[fg_idx].bumpTex );
				PIPE->set_texture_stage_texture( 1, fgi[fg_idx].colorTex );
				
				PIPE->set_texture_stage_state( 0, D3DTSS_TEXCOORDINDEX, 0);
				PIPE->set_texture_stage_state( 1, D3DTSS_TEXCOORDINDEX, 0);
				chosenEffect->Pass(0);
				PIPE->draw_indexed_primitive(D3DPT_TRIANGLELIST , D3DFVF_VBVERTEX, dest_verts, v_dst, id_list, index_count,0 );		
				PIPE->set_render_state(D3DRS_TEXTUREFACTOR,D3DCOLOR_XRGB((int)(emissiveColor[0]*160),(int)(emissiveColor[1]*160),(int)(emissiveColor[2]*160)));
				
				PIPE->set_render_state(D3DRS_SRCBLEND,D3DBLEND_ONE);
				PIPE->set_render_state(D3DRS_DESTBLEND,D3DBLEND_ONE);
				PIPE->set_texture_stage_state( 1, D3DTSS_TEXCOORDINDEX, 1);
				PIPE->set_texture_stage_texture( 1, fgi[fg_idx].glowTex	);

				//PIPE->set_render_state(D3DRS_ZFUNC,D3DCMP_EQUAL);
				chosenEffect->Pass(1);
				PIPE->draw_indexed_primitive(D3DPT_TRIANGLELIST , D3DFVF_VBVERTEX, dest_verts, v_dst, id_list, index_count,0 );		
			}
			chosenEffect->End();



			//return;
			cleanupRenderStates();
			return;
	}
	//BATCH->set_state(RPR_STATE_ID,0);
}

template <class VertexStruct,U32 VERTEX_FORMAT>
void MMaterial<VertexStruct,VERTEX_FORMAT>::RenderPuffy(const Transform &inv,const Transform &local_to_obj,const Vector & scale)
{
	if (vert_cnt==0)
		return;

	BATCH->set_state(RPR_BATCH,TRUE);

	S32 index_count;
	
	U32 stateID = getStateID();
	

	CQASSERT(vert_cnt < HUGE/2);
	BATCHDESC desc,desc2;
	U16 *id_list=0,*id_list2=0;
	VertexStruct *src_verts=0,*dest_verts=0;
	RPVertex * dest_verts2=0;
	bool bHasBuffers=0;
	
	while (!bHasBuffers)
	{
		BATCH->set_state(RPR_STATE_ID,stateID-1);  //corresponds to alpha stateID
		setRenderStates();
		//Material *mat = &mesh->material_list[fg->material];
		BATCH->set_render_state(D3DRS_ALPHABLENDENABLE,TRUE);
		BATCH->set_render_state(D3DRS_ZWRITEENABLE,FALSE);
		
		desc.type = D3DPT_TRIANGLELIST;
		desc.vertex_format = VERTEX_FORMAT;
		desc.num_verts = vert_cnt;
		desc.num_indices = new_face_cnt*3;
		CQBATCH->GetPrimBuffer(&desc);
		dest_verts = (VertexStruct *)desc.verts;
		id_list = desc.indices;
		src_verts = (VertexStruct *)src_verts_buffer;
		
		BATCH->set_render_state(D3DRS_SRCBLEND,D3DBLEND_SRCALPHA);
		BATCH->set_render_state(D3DRS_DESTBLEND,D3DBLEND_INVSRCALPHA);
		BATCH->set_texture_stage_texture(0,puffTexture);
		BATCH->set_texture_stage_state( 1, D3DTSS_COLOROP, D3DTOP_DISABLE );
		BATCH->set_texture_stage_state( 1, D3DTSS_ALPHAOP, D3DTOP_DISABLE );
		BATCH->set_state(RPR_DELAY,1);
		BATCH->set_state(RPR_STATE_ID,puffTexture+1);
		
		desc2.type = D3DPT_TRIANGLELIST;
		desc2.vertex_format = D3DFVF_RPVERTEX;
		desc2.num_verts = vert_cnt;
		desc2.num_indices = new_face_cnt*3;
		bHasBuffers = CQBATCH->GetPrimBuffer(&desc2,true);
		dest_verts2 = (RPVertex *)desc2.verts;
		id_list2 = desc2.indices;
	}

	static U8 alphas[3000];
	Vector fake_look = puffViewPt[0];

	int i;
	
	index_count = 0;
	int index_count2 = 0;
	
	int v_src=0;
	int v_dst=0;
	int v_dst2=0;
	CQASSERT(new_face_cnt < HUGE);
	memset(re_index_list,0xff,sizeof(U16)*vert_cnt);//new_face_cnt*3);
	U16 *re_index_list2 = &re_index_list[HUGE/2];
	memset(re_index_list2,0xff,sizeof(U16)*vert_cnt);//new_face_cnt*3);

	//useless copy to hit cache
	//memcpy(faces,faces,sizeof(ArchetypeFaceInfo)*new_face_cnt);
	
	float cx = cam_pos_in_object_space.x;
	float cy = cam_pos_in_object_space.y;
	float cz = cam_pos_in_object_space.z;
	for (int j=0;j<new_face_cnt;j++)
	{
		ArchetypeFaceInfo &afi = faces[j];
		SINGLE dot=cx*afi.norm.x+cy*afi.norm.y+cz*afi.norm.z;
		if (dot >= 0)//-afi.D)//fg->face_D_coefficient[j])
		{
			bool bShows=0;
			for (i=0;i<3;i++)
			{
				v_src = index_list[j*3+i];
				if (re_index_list[v_src] == 0xffff)
				{
					Vector puff_vert;
					Vector v = local_to_obj*src_verts[v_src].pos;
					puff_vert.set(v.x/scale.x,v.y/scale.y,v.z/scale.z);
					puff_vert.normalize();
					puff_vert.x *= scale.x;
					puff_vert.y *= scale.y;
					puff_vert.z *= scale.z;
					
					Vector puff_norm = puff_vert;
					puff_norm.normalize();
//					SINGLE dot = dot_product(puff_norm,fake_look);
					puff_norm.normalize();
					SINGLE s=max(0.0,min(1.0f,-2.0f+3.0f*dot_product(puff_norm,fake_look)));
					alphas[v_dst] = F2LONG(255.0*s);
					
					//SINGLE d=fabs(fmod(dot,400));
					//d=fabs((200.0f-d)*0.005f);
					SINGLE d=s;
					if (d>0)
						bShows=1;
					
					puff_vert *= puffPercent*d;
					puff_vert += (1.0f-puffPercent*d)*v;
					
					re_index_list[v_src] = v_dst;
					dest_vec[v_dst] = puff_vert;
					dest_verts[v_dst] = src_verts[v_src];
					dest_verts[v_dst].pos = puff_vert;
					dest_norm[v_dst] = src_norms[v_src];
					
					v_dst++;
				}
				
				id_list[index_count] = re_index_list[v_src];
				index_count++;
			}

			if (bShows)
			{
				for (i=0;i<3;i++)
				{
					v_src = index_list[j*3+i];
					if (re_index_list2[v_src] == 0xffff)
					{
						re_index_list2[v_src] = v_dst2;
						dest_verts2[v_dst2].pos = dest_verts[id_list[index_count-3+i]].pos;
						dest_verts2[v_dst2].u = dest_verts[id_list[index_count-3+i]].u;
						dest_verts2[v_dst2].v = dest_verts[id_list[index_count-3+i]].v;
						dest_verts2[v_dst2].color = 0xffffffff;
					//	dest_verts2[v_dst2].r = alphas[id_list[index_count-3+i]];
					//	dest_verts2[v_dst2].g = alphas[id_list[index_count-3+i]];
					//	dest_verts2[v_dst2].b = alphas[id_list[index_count-3+i]];
						dest_verts2[v_dst2].a = alphas[id_list[index_count-3+i]];
						
						v_dst2++;
					}

					id_list2[index_count2] = re_index_list2[v_src];
					index_count2++;
				}
			}
		}
	}
	
	CQASSERT(v_dst < HUGE);
	LIGHT->light_vertices_U8(lit,dest_vec,dest_norm,v_dst,&inv);
	
	U32 r,g,b;
	int x;
	FaceGroupInfo &fgi2 = fgi[fg_idx];

	i=0;
Loop:
		const LightRGB_U8 & lit_p = lit[i];
		VertexStruct & vert = dest_verts[i];

		if (i>=v_dst)
			goto Done;

		x=lit[i+1].r;
		r = ((fgi2.diffuse.r*(lit_p.r)) >> 8)+fgi2.emissive.r;
		g = ((fgi2.diffuse.g*(lit_p.g)) >> 8)+fgi2.emissive.g;
		b = ((fgi2.diffuse.b*(lit_p.b)) >> 8)+fgi2.emissive.b;

		if (r>255 || g>255 || b >255)
		{
			r = min(r,255);
			g = min(g,255);
			b = min(b,255);
		}

		vert.r = r;
		vert.g = g;
		vert.b = b;
		vert.a = alphas[i];
		i++;

		goto Loop;

Done:

	desc.num_verts = v_dst;
	desc.num_indices = index_count;
	CQBATCH->ReleasePrimBuffer(&desc);

	desc2.num_verts = v_dst2;
	desc2.num_indices = index_count2;
	CQBATCH->ReleasePrimBuffer(&desc2);

	BATCH->set_state(RPR_STATE_ID,0);
	BATCH->set_state(RPR_DELAY,0);
}

template <class VertexStruct,U32 VERTEX_FORMAT>
void MMaterial<VertexStruct,VERTEX_FORMAT>::RenderSolarian(const Transform &inv,const Transform &obj_to_view)
{
	if (vert_cnt==0)
		return;

	active_mi->bSuppressEmissive = false; //solarians don't flicker

	BATCH->set_state(RPR_BATCH,TRUE);
	U8 *faceRenders = &active_mi->faceRenders[face_offset];

	if (bChromeDirty)
	{
		chromeTrans.set_identity();
		chromeTrans.rotate_about_j(chromeRot);
		bChromeDirty = false;
	}

	S32 index_count;
	
	U32 stateID = getStateID();
	BATCH->set_state(RPR_STATE_ID,stateID);
	setRenderStates();

	BATCHDESC desc;
	desc.type = D3DPT_TRIANGLELIST;
	desc.vertex_format = VERTEX_FORMAT;
	desc.num_verts = vert_cnt;
	desc.num_indices = new_face_cnt*3;
	CQBATCH->GetPrimBuffer(&desc);
	VertexStruct *dest_verts = (VertexStruct *)desc.verts;
	U16 *id_list = desc.indices;
	VertexStruct *src_verts = (VertexStruct *)src_verts_buffer;
	
	int i;
	
	index_count = 0;

	int v_src=0;
	int v_dst=0;
	CQASSERT(new_face_cnt < HUGE);
	memset(re_index_list,0xff,sizeof(U16)*vert_cnt);//new_face_cnt*3);
	{
		//useless copy to hit cache
		//memcpy(faces,faces,sizeof(ArchetypeFaceInfo)*new_face_cnt);

		float cx = cam_pos_in_object_space.x;
		float cy = cam_pos_in_object_space.y;
		float cz = cam_pos_in_object_space.z;
		Vector view = cam_pos_in_object_space;
		view.normalize();
		for (int j=0;j<new_face_cnt;j++)
		{
			if ((faceRenders[j] & (FS__HIDDEN | FS__BUILDING)) == 0)
			{
				ArchetypeFaceInfo &afi = faces[j];
				SINGLE dot=cx*afi.norm.x+cy*afi.norm.y+cz*afi.norm.z;
				if (dot >= 0)//-afi.D)//fg->face_D_coefficient[j])
				{
					for (i=0;i<3;i++)
					{
						v_src = index_list[j*3+i];
						if (re_index_list[v_src] == 0xffff)
						{
							re_index_list[v_src] = v_dst;
							dest_vec[v_dst] = src_verts[v_src].pos;
							dest_verts[v_dst] = src_verts[v_src];
							Vector norm = chromeTrans.rotate(src_norms[v_src]);
							dest_verts[v_dst].u = 0.5+0.5*norm.x;
							dest_verts[v_dst].v = 0.5+0.5*norm.y;
							dest_norm[v_dst] = src_norms[v_src];
							v_dst++;
						}

						id_list[index_count] = re_index_list[v_src];
						index_count++;
					}

					faceRenders[j] &= ~FS__NOT_VISIBLE;
				}
				else
					faceRenders[j] |= FS__NOT_VISIBLE;
			}
		}

		CQASSERT(v_dst < HUGE);
		LIGHT->light_vertices_U8(lit,dest_vec,dest_norm,v_dst,&inv);
	}
	
	U32 r,g,b;
	int x;
	FaceGroupInfo &fgi2 = fgi[fg_idx];

	i=0;
Loop:
		const LightRGB_U8 & lit_p = lit[i];
		VertexStruct & vert = dest_verts[i];

		if (i>=v_dst)
			goto Done;

		x=lit[i+1].r;
		r = ((fgi2.diffuse.r*(lit_p.r)) >> 8)+fgi2.emissive.r;
		g = ((fgi2.diffuse.g*(lit_p.g)) >> 8)+fgi2.emissive.g;
		b = ((fgi2.diffuse.b*(lit_p.b)) >> 8)+fgi2.emissive.b;

		if (r>255 || g>255 || b >255)
		{
			r = min(r,255);
			g = min(g,255);
			b = min(b,255);
		}

		vert.r = r;
		vert.g = g;
		vert.b = b;
		vert.a = fgi2.a;
		i++;

		goto Loop;

Done:

	desc.num_verts = v_dst;
	desc.num_indices = index_count;
	CQBATCH->ReleasePrimBuffer(&desc);

	BATCH->set_state(RPR_STATE_ID,0);
}

template <class VertexStruct,U32 VERTEX_FORMAT>
void MMaterial<VertexStruct,VERTEX_FORMAT>::GetBuffers(void **vertices,U16 **indices,Vector **norms,ArchetypeFaceInfo **_faces,S32 *num_verts,S32 *num_faces)
{
	*vertices = src_verts_buffer;
	*indices = index_list;
	*norms = src_norms;
	*_faces = faces;
	*num_verts = vert_cnt;
	*num_faces = new_face_cnt;

}


//I hereby define split_n to point in the direction of out1
template <class VertexStruct,U32 VERTEX_FORMAT>
void MMaterial<VertexStruct,VERTEX_FORMAT>::SplitFaceGroup (const MeshInfo &src,MeshInfo &out0,MeshInfo &out1,RenderMaterial *fg_out0,RenderMaterial *fg_out1,SINGLE split_d,const Vector &split_n,struct MeshSplit &meshSplit)
{
	U8 *dest_faceRender0 = &meshSplit.dest_face_render0[meshSplit.out_face_cnt0];
	U8 *dest_faceRender1 = &meshSplit.dest_face_render1[meshSplit.out_face_cnt1];
	int pos_list_index_offset0 = tmp_v_cnt;
	int pos_list_index_offset1 = tmp_v_cnt1;

	((FaceGroupRender*)fg_out0)->vb_offset = tmp_v_cnt;
	((FaceGroupRender*)fg_out1)->vb_offset = tmp_v_cnt1;

	CQASSERT(face_offset < src.mr->face_cnt);
	//source buffers
	U8 *faceRenders = &src.faceRenders[face_offset];
	VertexStruct *src_verts = (VertexStruct *)src_verts_buffer;

	//destination buffers
	VertexStruct *dest_verts0=(VertexStruct *)tmp_verts1,*dest_verts1=(VertexStruct *)&tmp_verts2;
	Vector *dest_norm0=tmp_n,*dest_norm1=&tmp_n[HUGE/2];
	U16 *id_list0=indexScratchList,*id_list1=&indexScratchList[HUGE/2];
	ArchetypeFaceInfo afi0[HUGE/2];
	ArchetypeFaceInfo afi1[HUGE/2];

	//counters for output vertices generated
	U16 dst_ref0=0,dst_ref1=0;
	U16 index_count0=0,index_count1=0;

	U16 *re_index_list1 = &re_index_list[HUGE/2];
	CQASSERT(new_face_cnt < HUGE/2);
	
	memset(re_index_list,0xff,sizeof(U16)*vert_cnt);//new_face_cnt*3);
	memset(re_index_list1,0xff,sizeof(U16)*vert_cnt);//new_face_cnt*3);

	for (int f=0;f<new_face_cnt;f++)
	{
		if ((faceRenders[f] & FS__HIDDEN) == 0)
		{
			//should be temp, we're grabbing the face norm
			ArchetypeFaceInfo &afi = faces[f];

			//put the vertices on either side
			int cnt0,cnt1;
			cnt0 = cnt1 = 0;

			U16 ref0[3];
			U16 ref1[3];

			SINGLE ratios[2];
			SplitPoly sp;

			int i;
			for (i=0;i<3;i++)
			{
				int src_ref = index_list[f*3+i];

				//find the delta from this point to the split plane
				Vector delta = src_verts[src_ref].pos-split_d*split_n;
				//find out which side we're on
				if (dot_product(split_n,delta) < 0)
				{
					ref0[cnt0] = src_ref;
					sp.in_ref0[cnt0] = i;
					cnt0++;
				}
				else
				{
					ref1[cnt1] = src_ref;
					sp.in_ref1[cnt1] = i;
					cnt1++;
				}
			}

			CQASSERT(cnt0+cnt1 == 3);
			
			sp.split_face = f+face_offset;
			//ok what I'm doing here is creating the new face_ref_lists
			sp.out_split_face0 = meshSplit.out_face_cnt0+index_count0/3;
			sp.out_split_face1 = meshSplit.out_face_cnt1+index_count1/3;
			
			U16 out_ref0[4],out_ref1[4];
			
			int out0=0;
			int out1=0;
			
			//stash half 0
			for (i=0;i<cnt0;i++)
			{
				int src_ref = ref0[i];
				if (re_index_list[src_ref] == 0xffff)
				{
					re_index_list[src_ref] = dst_ref0;
					dest_verts0[dst_ref0] = src_verts[src_ref];
					dest_norm0[dst_ref0] = src_norms[src_ref];
					tmp_v[tmp_v_cnt++] = src_verts[src_ref].pos;
					dst_ref0++;
				}
				out_ref0[out0++] = re_index_list[src_ref];
			}
			
			//stash half 1
			for (i=0;i<cnt1;i++)
			{
				int src_ref = ref1[i];
				if (re_index_list1[src_ref] == 0xffff)
				{
					re_index_list1[src_ref] = dst_ref1;
					dest_verts1[dst_ref1] = src_verts[src_ref];
					dest_norm1[dst_ref1] = src_norms[src_ref];
					tmp_v1[tmp_v_cnt1++] = src_verts[src_ref].pos;
					dst_ref1++;
				}
				out_ref1[out1++] = re_index_list1[src_ref];
			}
			
			int rcnt=0;
			//split edges
			for (i=0;i<cnt0;i++)
			{
				int j;
				for (j=0;j<cnt1;j++)
				{
					VertexStruct out_vert;
					splitEdge(src_verts[ref0[i]],src_verts[ref1[j]],out_vert,split_d,split_n,ratios[rcnt]);
					rcnt++;
					//mesh 0
					dest_verts0[dst_ref0] = out_vert;
					dest_norm0[dst_ref0] = afi.norm;  //this should be temp
					tmp_v[tmp_v_cnt++] = out_vert.pos;
					out_ref0[out0++] = dst_ref0;
					dst_ref0++;
					CQASSERT(dst_ref0 < HUGE/2);
					
					//mesh 1
					dest_verts1[dst_ref1] = out_vert;
					dest_norm1[dst_ref1] = afi.norm;  //temp
					tmp_v1[tmp_v_cnt1++] = out_vert.pos;
					out_ref1[out1++] = dst_ref1;
					dst_ref1++;
					CQASSERT(dst_ref1 < HUGE/2);
				}
			}
			CQASSERT(rcnt == 0 || rcnt == 2);
			
			// re-index half 0
			if (out0)
			{
				afi0[index_count0/3] = afi;
				dest_faceRender0[index_count0/3] = faceRenders[f];
				
				id_list0[index_count0++] = out_ref0[0];
				id_list0[index_count0++] = out_ref0[1];
				id_list0[index_count0++] = out_ref0[2];
				
				if (out0 == 4)
				{
					afi0[index_count0/3] = afi;
					dest_faceRender0[index_count0/3] = faceRenders[f];
					id_list0[index_count0++] = out_ref0[0];
					id_list0[index_count0++] = out_ref0[2];
					id_list0[index_count0++] = out_ref0[3];
				}
				
				CQASSERT(index_count0 < HUGE/2);
			}
			
			// re-index half 1
			if (out1)
			{
				afi1[index_count1/3] = afi;
				dest_faceRender1[index_count1/3] = faceRenders[f];
				id_list1[index_count1++] = out_ref1[0];
				id_list1[index_count1++] = out_ref1[1];
				id_list1[index_count1++] = out_ref1[2];
				
				if (out1 == 4)
				{
					afi1[index_count1/3] = afi;
					dest_faceRender1[index_count1/3] = faceRenders[f];
					id_list1[index_count1++] = out_ref1[0];
					id_list1[index_count1++] = out_ref1[2];
					id_list1[index_count1++] = out_ref1[3];
				}
				
				CQASSERT(index_count1 < HUGE/2);
			}
			
			for (i=0;i<out0;i++)
				sp.out_ref0[i] = out_ref0[i]+pos_list_index_offset0;
			for (i=0;i<out1;i++)
				sp.out_ref1[i] = out_ref1[i]+pos_list_index_offset1;
			
			sp.in_cnt0 = cnt0;
			sp.in_cnt1 = cnt1;
			sp.out_cnt0 = out0;
			sp.out_cnt1 = out1;
			if (out0 && out1)
			{
				sp.ratios[0] = ratios[0];
				sp.ratios[1] = ratios[1];
			}
			
			
			int ec_cnt=0;
			IEffectChannel *ec = src.ec_list;
			while (ec)
			{
				if (ec->idx_cnt)  //implicitly linked to code in SplitMesh
				{
					static_cast<EffectChannel *>(ec)->AddSplitPoly(sp,ss[ec_cnt]);
					ec_cnt++;
				}
				ec = ec->next;
			}
		}
	}

	fg_out0->vert_cnt = dst_ref0;
	fg_out0->new_face_cnt = index_count0/3;
	if (dst_ref0)
	{
		//mesh 0
		fg_out0->src_verts_buffer = (void *)(new VertexStruct[dst_ref0]);
		memcpy(fg_out0->src_verts_buffer,dest_verts0,dst_ref0*sizeof(VertexStruct));
		fg_out0->src_norms = new Vector[dst_ref0];
		memcpy(fg_out0->src_norms,dest_norm0,dst_ref0*sizeof(Vector));
		fg_out0->index_list = new U16[index_count0];
		memcpy(fg_out0->index_list,id_list0,index_count0*sizeof(U16));
		
		//pad for cache read-ahead
		fg_out0->faces_ptr = (U8 *)malloc(sizeof(ArchetypeFaceInfo)*(fg_out0->new_face_cnt+1)+31);
		fg_out0->faces = (ArchetypeFaceInfo *)((U32(fg_out0->faces_ptr)+31) & ~31);
		memcpy(fg_out0->faces,afi0,fg_out0->new_face_cnt*sizeof(ArchetypeFaceInfo));
		fg_out0->face_offset = meshSplit.out_face_cnt0;
		out0.mr->face_cnt += fg_out0->new_face_cnt;
		meshSplit.out_face_cnt0 += fg_out0->new_face_cnt;
	}

	fg_out1->vert_cnt = dst_ref1;
	fg_out1->new_face_cnt = index_count1/3;
	if (dst_ref1)
	{
		//mesh 1
		fg_out1->src_verts_buffer = (void *)(new VertexStruct[dst_ref1]);
		memcpy(fg_out1->src_verts_buffer,dest_verts1,dst_ref1*sizeof(VertexStruct));
		fg_out1->src_norms = new Vector[dst_ref1];
		memcpy(fg_out1->src_norms,dest_norm1,dst_ref1*sizeof(Vector));
		fg_out1->index_list = new U16[index_count1];
		memcpy(fg_out1->index_list,id_list1,index_count1*sizeof(U16));
		
		//pad for cache read-ahead
		fg_out1->faces_ptr = (U8 *)malloc(sizeof(ArchetypeFaceInfo)*(fg_out1->new_face_cnt+1)+31);
		fg_out1->faces = (ArchetypeFaceInfo *)((U32(fg_out1->faces_ptr)+31) & ~31);
		memcpy(fg_out1->faces,afi1,fg_out1->new_face_cnt*sizeof(ArchetypeFaceInfo));
		fg_out1->face_offset = meshSplit.out_face_cnt1;
		out1.mr->face_cnt += fg_out1->new_face_cnt;
		meshSplit.out_face_cnt1 += fg_out1->new_face_cnt;
	}
}


//---------------------------------------------------------------------------
//
void PremultiplyTexture (U32 texID,S16 bias,SINGLE contrast)
{
	RPLOCKDATA data;
	unsigned long level=0;

	unsigned long width,height,num_levels;
	num_levels = 0;
	PIPE->get_texture_dim(texID,&width,&height,&num_levels);
	if(num_levels == 0)
		return;
	PIPE->lock_texture(texID,num_levels-1,&data);
	PIPE->unlock_texture(texID,num_levels-1);
	if (data.pf.num_bits() != 16)
		return;

	U16 *pixel = (U16 *)((U8 *)data.pixels);
	if (pixel[0] == 0xf81f)
		return;

	while (level < num_levels)
	{
		PIPE->lock_texture(texID,level,&data);
//		RGB colors[256];
		
	//	if (data.pf.num_bits() == 16)
	//	{
			U8 r,g,b;
			for (unsigned int y=0;y<data.height;y++)
			{
				for (unsigned int x=0;x<data.width;x++)
				{
					U16 *pixel = (U16 *)((U8 *)data.pixels+(y*data.pitch+x*2));
					r = (*pixel >> data.pf.rl) << data.pf.rr;
					g = (*pixel >> data.pf.gl) << data.pf.gr;
					b = (*pixel >> data.pf.bl) << data.pf.br;
					r = min(bias+r*contrast,255);
					g = min(bias+g*contrast,255);
					b = min(bias+b*contrast,255);
					*pixel = (r >> data.pf.rr) << data.pf.rl;
					*pixel |= (g >> data.pf.gr) << data.pf.gl;
					*pixel |= (b >> data.pf.br) << data.pf.bl;
				}
			}
	/*	}
		else 
		{
			PIPE->get_texture_palette(texID,0,256,colors);
			S16 r,g,b;
			for (int i=0;i<256;i++)
			{
				r = colors[i].r*contrast+bias;
				g = colors[i].g*contrast+bias;
				b = colors[i].b*contrast+bias;
				
				colors[i].r = max(0,min(r,255));
				colors[i].g = max(0,min(g,255));
				colors[i].b = max(0,min(b,255));
			}
			PIPE->set_texture_palette(texID,0,256,colors);
		}*/
		PIPE->unlock_texture(texID,level);

		level++;
	}

	PIPE->lock_texture(texID,num_levels-1,&data);
	pixel = (U16 *)((U8 *)data.pixels);
	pixel[0] = 0xf81f;
	PIPE->unlock_texture(texID,num_levels-1);
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
void SoftwareFGRender::setRenderStates()
{
	unique = 1;
	if (fgi[fg_idx].texture_flags & TF_F_HAS_ALPHA || fgi[fg_idx].a != 255)
	{
		BATCH->set_render_state(D3DRS_ALPHABLENDENABLE,TRUE);
		BATCH->set_render_state(D3DRS_ZWRITEENABLE,FALSE);
		unique = 2;
	}
	else
	{
		BATCH->set_render_state(D3DRS_ALPHABLENDENABLE,FALSE);
		BATCH->set_render_state(D3DRS_ZWRITEENABLE,TRUE);
	}
	
	BATCH->set_texture_stage_texture( 0, fgi[fg_idx].texture_id );
	
	// addressing - clamped
	BATCH->set_sampler_state( 0, D3DSAMP_ADDRESSU, MAT_GET_ADDR_MODE(fgi[fg_idx].texture_flags,0) );
	BATCH->set_sampler_state( 0, D3DSAMP_ADDRESSV, MAT_GET_ADDR_MODE(fgi[fg_idx].texture_flags,1) );
	
	BATCH->set_texture_stage_state( 0, D3DTSS_TEXCOORDINDEX, 0);

	if ((flags & MM_MARKINGS) == 0)
	{
		BATCH->set_texture_stage_state(0,D3DTSS_COLOROP,D3DTOP_SELECTARG1);
		BATCH->set_texture_stage_state(0,D3DTSS_ALPHAOP,D3DTOP_DISABLE );
		BATCH->set_texture_stage_state(0,D3DTSS_COLORARG1,D3DTA_TEXTURE);
		BATCH->set_state(RPR_STATE_ID,U32(fgi[fg_idx].unique_id)+unique);
	}
	else
	{
		BATCH->set_texture_stage_texture( 0, 0 );
		BATCH->set_texture_stage_state( 0, D3DTSS_COLOROP, D3DTOP_SELECTARG1 );
		BATCH->set_texture_stage_state(0,D3DTSS_ALPHAOP,D3DTOP_DISABLE );
		BATCH->set_texture_stage_state( 0, D3DTSS_COLORARG1, D3DTA_DIFFUSE );
		BATCH->set_texture_stage_state( 0, D3DTSS_COLORARG2, D3DTA_DIFFUSE );
		BATCH->set_state(RPR_STATE_ID,U32(fgi[fg_idx].unique_id)+unique+4);
	}
	
}

void SoftwareFGRender::Init(Mesh *mesh,int _fg_idx,MeshRender *_mr)
{
	FaceGroupRender::Init(mesh,_fg_idx,_mr);
	FaceGroup *fg = &mesh->face_groups[fg_idx];

	PremultiplyTexture(	mesh->material_list[fg->material].texture_id,0.0f,0.75f);
}

void SoftwareFGRender::Render(const Transform &inv)
{
	if (vert_cnt==0)
		return;

	U8 *faceRenders = &active_mi->faceRenders[face_offset];

	S32 index_count;
	
	setRenderStates();

	BATCHDESC desc;
	desc.type = D3DPT_TRIANGLELIST;
	//desc.vertex_format = D3DFVF_LVERTEX;
	desc.num_verts = new_face_cnt*3;
	desc.num_indices = new_face_cnt*3;
	CQBATCH->GetPrimBuffer(&desc);
	SftVertex *dest_verts = (SftVertex *)desc.verts;
	U16 *id_list = desc.indices;
	RPVertex *src_verts = (RPVertex *)src_verts_buffer;
	

	int i;
	
	index_count = 0;
#if 0
	if (bCacheValid)
	{
		//useless copy to hit cache
		memcpy(index_list,index_list,sizeof(U16)*new_face_cnt*3);
	//	maxV=-1;
		for (int j=0;j<new_face_cnt;j++)
		{
		//	if ((index_list[j*3] & (FS__HIDDEN | FS__NOT_VISIBLE)) == 0)
			if ((faceRenders[j] & (FS__HIDDEN | FS__NOT_VISIBLE)) == 0)
			{
				memcpy(&id_list[index_count],&index_list[j*3],sizeof(U16)*3);
	//			maxV = max(maxV,max(id_list[index_count],max(id_list[index_count+1],id_list[index_count+2])));
			/*	*id_list_pos++ = *index_list_pos++;
				*id_list_pos++ = *index_list_pos++;
				*id_list_pos++ = *index_list_pos++;*/
				index_count+=3;
			}
		//	else
		//		index_list_pos += 3;
		}
	//	maxV += minV;
	}
	else
#endif
	
	int v_src=0;
	int v_dst=0;
	CQASSERT(new_face_cnt < HUGE);
	memset(re_index_list,0xff,sizeof(U16)*vert_cnt);//new_face_cnt*3);
	{
		//useless copy to hit cache
		//memcpy(faces,faces,sizeof(ArchetypeFaceInfo)*new_face_cnt);

		float cx = cam_pos_in_object_space.x;
		float cy = cam_pos_in_object_space.y;
		float cz = cam_pos_in_object_space.z;
		for (int j=0;j<new_face_cnt;j++)
		{
			if ((faceRenders[j] & (FS__HIDDEN | FS__BUILDING)) == 0)
			{
				ArchetypeFaceInfo &afi = faces[j];
				SINGLE dot=cx*afi.norm.x+cy*afi.norm.y+cz*afi.norm.z;
				if (dot >= 0)//-afi.D)//fg->face_D_coefficient[j])
				{
					for (i=0;i<3;i++)
					{
						v_src = index_list[j*3+i];
						if (re_index_list[v_src] == 0xffff)
						{
							re_index_list[v_src] = v_dst;
							//dest_vec[v_dst] = src_verts[v_src].pos;
							dest_verts[v_dst].pos = src_verts[v_src].pos;
							dest_verts[v_dst].u = src_verts[v_src].u;
							dest_verts[v_dst].v = src_verts[v_src].v;
						//	dest_norm[v_dst] = src_norms[v_src];
							v_dst++;
						}

						id_list[index_count] = re_index_list[v_src];
						index_count++;
					}

					faceRenders[j] &= ~FS__NOT_VISIBLE;
					//memcpy(&id_list[index_count],&index_list[j*3],sizeof(U16)*3);

					//index_count+=3;
				}
				else
					faceRenders[j] |= FS__NOT_VISIBLE;
			}
		}

		CQASSERT(v_dst < HUGE);
	//	LIGHT->light_vertices_U8(lit,dest_vec,dest_norm,v_dst,&inv);
	}
	
	if ((flags & MM_MARKINGS))
	{
		U32 r,g,b;
		FaceGroupInfo &fgi2 = fgi[fg_idx];
		r = fgi2.diffuse.r*0.7;
		g = fgi2.diffuse.g*0.7;
		b = fgi2.diffuse.b*0.7;
		i=0;

Loop:

		SftVertex & vert = dest_verts[i];
		
		if (i>=v_dst)
			goto Done;
		
		vert.r = r;
		vert.g = g;
		vert.b = b;
		vert.a = fgi2.a;
		i++;
		
		goto Loop;

Done:
		;
	}

	desc.num_verts = v_dst;
	desc.num_indices = index_count;
	CQBATCH->ReleasePrimBuffer(&desc);

	BATCH->set_state(RPR_STATE_ID,0);
}

void SoftwareFGRender::Clone(RenderMaterial **rm,bool bCopyBuffers)
{
	SoftwareFGRender *fgr;
	*rm = fgr = new SoftwareFGRender;

	fgr->mr = mr;
	fgr->fg_idx = fg_idx;
	fgr->fg = fg;
	fgr->flags = flags;
	fgr->new_face_cnt=new_face_cnt;
	fgr->vert_cnt = vert_cnt;
	fgr->face_offset = face_offset;
	if (bCopyBuffers)
	{
		fgr->src_verts_buffer = (void *)(new RPVertex[vert_cnt]);
		memcpy(fgr->src_verts_buffer,src_verts_buffer,vert_cnt*sizeof(RPVertex));
//		fgr->pos_list = new Vector[vert_cnt];
//		memcpy(fgr->pos_list,pos_list,vert_cnt*sizeof(Vector));
		fgr->src_norms = new Vector[vert_cnt];
		memcpy(fgr->src_norms,src_norms,vert_cnt*sizeof(Vector));
		fgr->index_list = new U16[new_face_cnt*3];
		memcpy(fgr->index_list,index_list,new_face_cnt*3*sizeof(U16));
	

		//pad for cache read-ahead
		fgr->faces_ptr = (U8 *)malloc(sizeof(ArchetypeFaceInfo)*(new_face_cnt+1)+31);
		fgr->faces = (ArchetypeFaceInfo *)((U32(faces_ptr)+31) & ~31);
		memcpy(fgr->faces,faces,new_face_cnt*sizeof(ArchetypeFaceInfo));
	}

//	CQASSERT(HEAP->EnumerateBlocks());

}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
void CopyMesh(IMeshRender *src,IMeshRender *dest,bool bCopyBuffers)
{
	dest->pos_list = new Vector[src->pos_cnt];
	memcpy(dest->pos_list,src->pos_list,src->pos_cnt*sizeof(Vector));
	dest->pos_cnt = src->pos_cnt;
	dest->face_cnt = src->face_cnt;

	RenderMaterial *pos=0,*src_pos=(static_cast<MeshRender *>(src))->fgr;

	while (src_pos)
	{
		RenderMaterial *rm;
	
		src_pos->Clone(&rm,bCopyBuffers);

		//dest->AddFaceGroup(rm);

		if (pos)
		{
			pos->next = rm;
			pos = pos->next;
		}
		else
			static_cast<MeshRender *>(dest)->fgr = pos = rm;
	
		src_pos = src_pos->next;
	}


}

//I hereby define split_n to point in the direction of out1
void SplitMesh(const MeshInfo &src,MeshInfo &out0,MeshInfo &out1,SINGLE split_d,const Vector &split_n)
{
	MeshSplit meshSplit;

	out0.mr = CreateMeshRender();
	out0.mr->AddRef();
	out1.mr = CreateMeshRender();
	out1.mr->AddRef();
	int num_effect_channels = 0;

	//create the new effect channels
	IEffectChannel *ec = src.ec_list;
	IEffectChannel *last_ec0=0,*last_ec1=0;
	while (ec)
	{
		CQASSERT(num_effect_channels < MAX_EFFECT_CHANNELS);
		
		if (ec->idx_cnt) //implicitly linked to code in splitfacegroup
		{
			//out 0
			IEffectChannel *new_ec = CreateEffectChannel();
			new_ec->irc = ec->irc->Clone();
			new_ec->irc->ec = new_ec;
			new_ec->idx_cnt = 0;
			new_ec->idx_list = new U16[ec->idx_cnt*2];
			new_ec->tc_cnt = 0;
			new_ec->tc = new TexCoord[ec->tc_cnt*3];
			new_ec->tc_idx_list = new U16[ec->idx_cnt*2];
			new_ec->face_ref_list = new U16[(ec->idx_cnt/3)*2+1];
			memset(new_ec->face_ref_list,0xff,sizeof(U16)*((ec->idx_cnt/3)*2+1));
			new_ec->mi = &out0;
			
			if (last_ec0)
				last_ec0->next = new_ec;
			else
				out0.ec_list = new_ec;
			last_ec0 = new_ec;
			//out 1
			new_ec = CreateEffectChannel();
			new_ec->irc = ec->irc->Clone();
			new_ec->irc->ec = new_ec;
			new_ec->idx_cnt = 0;
			new_ec->idx_list = new U16[ec->idx_cnt*2];
			new_ec->tc_cnt = 0;
			new_ec->tc = new TexCoord[ec->tc_cnt*3];
			new_ec->tc_idx_list = new U16[ec->idx_cnt*2];
			new_ec->face_ref_list = new U16[(ec->idx_cnt/3)*2+1];
			memset(new_ec->face_ref_list,0xff,sizeof(U16)*((ec->idx_cnt/3)*2+1));
			new_ec->mi = &out1;
			
			if (last_ec1)
				last_ec1->next = new_ec;
			else
				out1.ec_list = new_ec;
			last_ec1 = new_ec;
			
			ss[num_effect_channels].tc_re_index0 = new U16[ec->tc_cnt];
			memset(ss[num_effect_channels].tc_re_index0,0xff,sizeof(U16)*ec->tc_cnt);
			ss[num_effect_channels].tc_re_index1 = new U16[ec->tc_cnt];
			memset(ss[num_effect_channels].tc_re_index1,0xff,sizeof(U16)*ec->tc_cnt);
			ss[num_effect_channels].next_face_ref = 0;
			ss[num_effect_channels].out0 = static_cast<EffectChannel *>(last_ec0);
			ss[num_effect_channels].out1 = static_cast<EffectChannel *>(last_ec1);
			num_effect_channels++;
		}

		ec = ec->next;
	}

	tmp_v_cnt = tmp_v_cnt1 = 0;
	//split the facegroups
	RenderMaterial *pos0=0,*pos1=0,*src_pos=(static_cast<MeshRender *>(src.mr))->fgr;

	while (src_pos)
	{
		if (src_pos->vert_cnt) //cull out silly degenerate meshes
		{
			RenderMaterial *rm0,*rm1;
			
			src_pos->Clone(&rm0,0); //don't copy the buffers
			src_pos->Clone(&rm1,0);
			
			if (pos0)
			{
				pos0->next = rm0;
				pos0 = pos0->next;
			}
			else
				static_cast<MeshRender *>(out0.mr)->fgr = pos0 = rm0;
			
			if (pos1)
			{
				pos1->next = rm1;
				pos1 = pos1->next;
			}
			else
				static_cast<MeshRender *>(out1.mr)->fgr = pos1 = rm1;
			
			rm0->new_face_cnt = 0;
			rm0->vert_cnt = 0;
			rm0->mr = static_cast<MeshRender *>(out0.mr);
			rm1->new_face_cnt = 0;
			rm1->vert_cnt = 0;
			rm1->mr = static_cast<MeshRender *>(out1.mr);
			src_pos->SplitFaceGroup(src,out0,out1,rm0,rm1,split_d,split_n,meshSplit);
		}
		src_pos = src_pos->next;
	}

	if (tmp_v_cnt)
	{
		out0.mr->pos_list = new Vector[tmp_v_cnt];
		memcpy(out0.mr->pos_list,tmp_v,sizeof(Vector)*tmp_v_cnt);
		out0.mr->pos_cnt = tmp_v_cnt;
		out0.mr->face_cnt = meshSplit.out_face_cnt0;
		out0.faceRenders = new U8[meshSplit.out_face_cnt0];
		memcpy(out0.faceRenders,meshSplit.dest_face_render0,meshSplit.out_face_cnt0);
	}

	if (tmp_v_cnt1)
	{
		out1.mr->pos_list = new Vector[tmp_v_cnt1];
		memcpy(out1.mr->pos_list,tmp_v1,sizeof(Vector)*tmp_v_cnt1);
		out1.mr->pos_cnt = tmp_v_cnt1;
		out1.mr->face_cnt = meshSplit.out_face_cnt1;
		out1.faceRenders = new U8[meshSplit.out_face_cnt1];
		memcpy(out1.faceRenders,meshSplit.dest_face_render1,meshSplit.out_face_cnt1);
	}

	for (int i=0;i<MAX_EFFECT_CHANNELS;i++)
	{
		ss[i].Reset();
	}

	//terminate the face_ref_lists
	ec = out0.ec_list;
	while (ec)
	{
		ec->face_ref_list[ec->idx_cnt/3] = 0xffff;
		ec = ec->next;
	}
	ec = out1.ec_list;
	while (ec)
	{
		ec->face_ref_list[ec->idx_cnt/3] = 0xffff;
		ec = ec->next;
	}

	((MeshRender*)out0.mr)->RestoreVertexBuffers();
	((MeshRender*)out1.mr)->RestoreVertexBuffers();
	//out0.mr->RestoreVertexBuffers();
}

IMeshRender * CreateMeshRender()
{
	MeshRender *new_mr = new MeshRender;
	return new_mr;
}
/*void AllocateMeshRenders(IMeshRender **imr,int num)
{
	MeshRender *block = new MeshRender[num];
	for (int i=0;i<num;i++)
	{
		imr[i] = &block[i];
	}
}

void DeleteMeshRenders(IMeshRender **imr,int num)
{
	if (imr)
	{
		MeshRender * mr = static_cast<MeshRender *>(imr[0]);
		delete [] mr;
	}
}*/

void UpdateRenderEffects(SINGLE dt)
{
	bChromeDirty = true;
	chromeRot += dt;
	if (chromeRot > 2*PI)
		chromeRot -= 2*PI;
}

//---------------------------------------------------------------------------
//  Effects Channels
//

EffectChannel::~EffectChannel()
{
	delete [] idx_list;
	delete [] tc;
	delete [] tc_idx_list;
	if (face_ref_list)
		delete [] face_ref_list;
	delete irc;
}

void EffectChannel::RenderWithTexture(U32 textureID,U32 color,bool bClamp)
{
	if (tc_cnt == 0)
		return;

	U32 adjusted_color = color & 0x00ffffff;
	adjusted_color |= (mi->fgi[0].a<<24);  //this is exceptionally kludgey!!!!

	S32 index_count;
	
	BATCH->set_state(RPR_BATCH,TRUE);
	
	BATCH->set_render_state(D3DRS_ZENABLE,TRUE);
	BATCH->set_render_state(D3DRS_ZWRITEENABLE,FALSE);
	BATCH->set_render_state(D3DRS_CULLMODE,D3DCULL_NONE);
	BATCH->set_render_state(D3DRS_ALPHABLENDENABLE,TRUE);
	BATCH->set_render_state(D3DRS_SRCBLEND,D3DBLEND_SRCALPHA);
	BATCH->set_render_state(D3DRS_DESTBLEND,D3DBLEND_INVSRCALPHA);
	BATCH->set_render_state(D3DRS_ZFUNC,D3DCMP_LESSEQUAL);
	BATCH->set_render_state(D3DRS_WRAP0,0);
	
//	BATCH->set_sampler_state( 0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP );
//	BATCH->set_sampler_state( 0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP );
	BATCH->set_state(RPR_STATE_ID,textureID);
	SetupDiffuseBlend(textureID,bClamp);
	
	BATCHDESC desc;
	desc.type = D3DPT_TRIANGLELIST;
	desc.vertex_format = D3DFVF_RPVERTEX;
	desc.num_verts = tc_cnt;  //worried about this
	desc.num_indices = idx_cnt;
	CQBATCH->GetPrimBuffer(&desc);
	RPVertex *dest_verts = (RPVertex *)desc.verts;
	U16 *id_list = desc.indices;

	Vector *pos_list = mi->mr->pos_list;
	

	int i;
	
	index_count = 0;
	
	int v_src=0;
	int v_dst=0;
	{
		int face_cnt = idx_cnt/3;
		for (int j=0;j<face_cnt;j++)
		{
			if ((mi->faceRenders[face_ref_list[j]] & FS__HIDDEN) == 0)
			{
				for (i=0;i<3;i++)
				{
					v_src = idx_list[j*3+i];
					v_dst = tc_idx_list[j*3+i];
					
					dest_verts[v_dst].pos = pos_list[v_src];
					dest_verts[v_dst].u = tc[v_dst].u;
					dest_verts[v_dst].v = tc[v_dst].v;
					dest_verts[v_dst].color = adjusted_color;
					id_list[index_count] = v_dst;
					index_count++;
				}
			}
		}
	}
	
	desc.num_indices = index_count;
	CQBATCH->ReleasePrimBuffer(&desc);

	BATCH->set_state(RPR_STATE_ID,0);
}

/*void EffectChannel::StartSplit()
{

}*/

void EffectChannel::AddSplitPoly(SplitPoly &sp,SplitStruct &ss)
{
	//this is because polygons with effects on them can be hidden and therefore won't be
	//considered for splitting
	while (face_ref_list[ss.next_face_ref] < sp.split_face)
		ss.next_face_ref++;

	if (face_ref_list[ss.next_face_ref] == sp.split_face)
	{
		int i;
		U16 tc_out_ref0[4],tc_out_ref1[4];
		U16 tc_out_cnt0=0,tc_out_cnt1=0;
		
		//the variables on the following line correspond to the internal face count of this struct
		int split_face_start_vert = ss.next_face_ref*3;
		ss.next_face_ref++;
		
		for (i=0;i<sp.in_cnt0;i++)
		{
			//in_ref0 is 0..2
			int src_ref = tc_idx_list[split_face_start_vert+sp.in_ref0[i]];
			if (ss.tc_re_index0[src_ref] == 0xffff)
			{
				ss.tc_re_index0[src_ref] = ss.out0->tc_cnt;
				ss.out0->tc[ss.out0->tc_cnt] = tc[src_ref];
				ss.out0->tc_cnt++;
				CQASSERT(ss.out0->tc_cnt < tc_cnt*3);
			}
			tc_out_ref0[tc_out_cnt0++] = ss.tc_re_index0[src_ref];
		}
		for (i=0;i<sp.in_cnt1;i++)
		{
			//in_ref1 is 0..2
			int src_ref = tc_idx_list[split_face_start_vert+sp.in_ref1[i]];
			if (ss.tc_re_index1[src_ref] == 0xffff)
			{
				ss.tc_re_index1[src_ref] = ss.out1->tc_cnt;
				ss.out1->tc[ss.out1->tc_cnt] = tc[src_ref];
				ss.out1->tc_cnt++;
				CQASSERT(ss.out1->tc_cnt < tc_cnt*3);
			}
			tc_out_ref1[tc_out_cnt1++] = ss.tc_re_index1[src_ref];
		}
		
		//create the new texture coords on the split
		int tc_made=0;
		for (i=0;i<sp.in_cnt0;i++)
		{
			int j;
			for (j=0;j<sp.in_cnt1;j++)
			{
				TexCoord newTex;
				newTex.u = ss.out0->tc[tc_out_ref0[i]].u+sp.ratios[tc_made]*(ss.out1->tc[tc_out_ref1[j]].u-ss.out0->tc[tc_out_ref0[i]].u);
				newTex.v = ss.out0->tc[tc_out_ref0[i]].v+sp.ratios[tc_made]*(ss.out1->tc[tc_out_ref1[j]].v-ss.out0->tc[tc_out_ref0[i]].v);
				ss.out0->tc[ss.out0->tc_cnt] = newTex;
				CQASSERT(tc_out_cnt0 < 4);
				tc_out_ref0[tc_out_cnt0++] = ss.out0->tc_cnt++;
				CQASSERT(ss.out0->tc_cnt < tc_cnt*3);
				ss.out1->tc[ss.out1->tc_cnt] = newTex;
				tc_out_ref1[tc_out_cnt1++] = ss.out1->tc_cnt++;
				CQASSERT(ss.out1->tc_cnt < tc_cnt*3);
			}
		}
		
		CQASSERT(tc_made == 0 || tc_made == 2);
		
		// re-idx half 0
		if (sp.out_cnt0)
		{
			ss.out0->face_ref_list[ss.out0->idx_cnt/3] = sp.out_split_face0;
			ss.out0->idx_list[ss.out0->idx_cnt] = sp.out_ref0[0];
			ss.out0->idx_list[ss.out0->idx_cnt+1] = sp.out_ref0[1];
			ss.out0->idx_list[ss.out0->idx_cnt+2] = sp.out_ref0[2];
			ss.out0->tc_idx_list[ss.out0->idx_cnt++] = tc_out_ref0[0];
			ss.out0->tc_idx_list[ss.out0->idx_cnt++] = tc_out_ref0[1];
			ss.out0->tc_idx_list[ss.out0->idx_cnt++] = tc_out_ref0[2];
			
			if (sp.out_cnt0 == 4)
			{
				ss.out0->face_ref_list[ss.out0->idx_cnt/3] = sp.out_split_face0+1;
				ss.out0->idx_list[ss.out0->idx_cnt] = sp.out_ref0[0];
				ss.out0->idx_list[ss.out0->idx_cnt+1] = sp.out_ref0[2];
				ss.out0->idx_list[ss.out0->idx_cnt+2] = sp.out_ref0[3];
				
				CQASSERT(tc_out_cnt0 == 4);
				
				ss.out0->tc_idx_list[ss.out0->idx_cnt++] = tc_out_ref0[0];
				ss.out0->tc_idx_list[ss.out0->idx_cnt++] = tc_out_ref0[2];
				ss.out0->tc_idx_list[ss.out0->idx_cnt++] = tc_out_ref0[3];
			}
		}
		
		// re-idx half 1
		if (sp.out_cnt1)
		{
			ss.out1->face_ref_list[ss.out1->idx_cnt/3] = sp.out_split_face1;
			ss.out1->idx_list[ss.out1->idx_cnt] = sp.out_ref1[0];
			ss.out1->idx_list[ss.out1->idx_cnt+1] = sp.out_ref1[1];
			ss.out1->idx_list[ss.out1->idx_cnt+2] = sp.out_ref1[2];
			ss.out1->tc_idx_list[ss.out1->idx_cnt++] = tc_out_ref1[0];
			ss.out1->tc_idx_list[ss.out1->idx_cnt++] = tc_out_ref1[1];
			ss.out1->tc_idx_list[ss.out1->idx_cnt++] = tc_out_ref1[2];
			
			if (sp.out_cnt1 == 4)
			{
				ss.out1->face_ref_list[ss.out1->idx_cnt/3] = sp.out_split_face1+1;
				ss.out1->idx_list[ss.out1->idx_cnt] = sp.out_ref1[0];
				ss.out1->idx_list[ss.out1->idx_cnt+1] = sp.out_ref1[2];
				ss.out1->idx_list[ss.out1->idx_cnt+2] = sp.out_ref1[3];
				
				ss.out1->tc_idx_list[ss.out1->idx_cnt++] = tc_out_ref1[0];
				ss.out1->tc_idx_list[ss.out1->idx_cnt++] = tc_out_ref1[2];
				ss.out1->tc_idx_list[ss.out1->idx_cnt++] = tc_out_ref1[3];
			}
		}
	}
	else
		CQASSERT(face_ref_list[ss.next_face_ref] > sp.split_face);
}

IEffectChannel * CreateEffectChannel()
{
	return new EffectChannel;
}

/*void DeleteEffectChannel(IEffectChannel *ec)
{
	EffectChannel *_ec = static_cast<EffectChannel *>(ec);
	delete _ec;
}*/

//---------------------------------------------------------------------------
//---------------------End MeshRender.cpp------------------------------------
//---------------------------------------------------------------------------
