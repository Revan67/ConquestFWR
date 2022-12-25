//--------------------------------------------------------------------------//
//                                                                          //
//                             MeshRender.h                                 //
//                                                                          //
//                  COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/MeshRender.h 34    10/21/00 6:18p Jasony $
*/			    
//------------------------------- #INCLUDES --------------------------------//
//--------------------------------------------------------------------------//

#ifndef MESHRENDER_H
#define MESHRENDER_H

#ifndef IEXPLOSION_H
#include "IExplosion.h"
#endif

#include "MyVertex.h"

#include <LightMan.h>

#include <DBaseData.h>

extern Vector puffViewPt[2];
extern U32 puffTexture;

/*enum FACE_STATE
{
	FS_HIDDEN,
	FS_BUILDING,
	FS_VISIBLE,
	FS_DAMAGED
};*/
#define MM_MULTITEX 0x01
#define MM_CHROME	0x02
#define MM_MARKINGS 0x04

struct MyFace
{
//	S32 groupID;
//	S32 index;
	struct IRenderMaterial *group;
	S32 index;
};

#define FS__NOT_VISIBLE 0x01
#define FS__HIDDEN		0x02
#define FS__DAMAGED		0x04
#define FS__BUILDING    0x08


struct FaceGroupInfo
{
	struct IMaterial * iMat;

	LightRGB_U8 diffuse;
	LightRGB_U8 emissive;
	U8 a;
	U32			texture_flags;
	U32			texture_id;
	U32			emissive_texture_flags;
	U32			emissive_texture_id;
	U32			unique_id;


	bool bCQ2Mat;
	U32 colorTex;
	U32 bumpTex;
	U32 specTex;
	U32 glowTex;
	

	FaceGroupInfo()
	{
		iMat = 0;
		bCQ2Mat = false;
		colorTex = bumpTex= specTex= glowTex=0;
	}

	~FaceGroupInfo();
	
};

struct IRenderChannel
{
	struct IEffectChannel *ec;

	virtual void Render(SINGLE dt) = 0;

	virtual IRenderChannel * Clone() = 0;

	virtual ~IRenderChannel()
	{
	}
};

struct IEffectChannel
{
//	struct IMeshRender *mr;
	struct MeshInfo *mi;
	U16 *idx_list;
	U16 *tc_idx_list;
	U16 *face_ref_list;
	TexCoord *tc;
	U16 idx_cnt,tc_cnt;
	U16 vert_cnt;
	U8 channelID;
	IEffectChannel *next;
	IRenderChannel *irc;

	virtual void RenderWithTexture(U32 textureID,U32 color,bool bClamp) = 0;

	virtual ~IEffectChannel()
	{
	}
};

IEffectChannel * CreateEffectChannel();
//void DeleteEffectChannel(IEffectChannel *ec);

//struct containing mesh data about a single instance
struct MeshInfo
{
	//for sorting faces
//	S32 *face_array;
	
	//archetype info for the mesh
	struct IMeshRender *mr;
	//for hiding or otherwise altering faces
//	FACE_STATE *hiddenArray;
	Vector sphere_center;
	SINGLE radius;
	//polymesh mesh
//	Mesh * buildMesh;
	//for stashing and restoring face properties
//	FACE_PROPERTY *face_props;
	//gather all faces into one array & sort?
//	MyFace *myFaceArray;
//	S32 face_cnt;
	//material properties - diffuse, emissive
	U8 *faceRenders;

	FaceGroupInfo *fgi;
	IEffectChannel *ec_list;
	struct IMeshInfoTree *parent;

	U8 faceGroupCnt;

	int cameraMoveCnt;

	INSTANCE_INDEX instanceIndex;

	int timer;

//	U16 unique:4;
	bool bWhole:1;
	bool bCacheValid:1;
	bool bHasMesh:1;
	bool bOwnsInstance:1;
	bool bSuppressEmissive:1;
	bool bBroken:1;

//	void Sort(bool bZ);

	virtual IEffectChannel * GetNewEffectChannel();

	virtual void RemoveEffectChannel(IEffectChannel *ec);

	virtual void CalculateSphere();

	virtual void GetBoundingBox(float *box);

	MeshInfo();
	
	~MeshInfo();

};

//interface to deal with a tree of instances
struct IMeshInfoTree
{
	virtual void AttachChild(INSTANCE_INDEX parentID,IMeshInfoTree *child_info)=0;

	virtual MeshInfo * GetMeshInfo()=0;

	virtual void DetachChild(INSTANCE_INDEX childID,IMeshInfoTree **child_info)=0;

	virtual void GetChildInfo(INSTANCE_INDEX childID,IMeshInfoTree **child_info)=0;
	
	virtual U32 ListChildren(MeshInfo **mesh_info)=0;

	virtual void LoseOwnershipOfMeshInfo()=0;

	virtual U32 ListFirstChildren(IMeshInfoTree **mesh_info)=0;
};

struct MeshChain
{
	MeshInfo *mi[MAX_CHILDS];
	U8 numChildren;
	bool bOwnsChildren;

	MeshChain()
	{
		bOwnsChildren = 0;
		numChildren = 0;
	}

	~MeshChain();
};

//function to create a tree of instances
IMeshInfoTree * CreateMeshInfoTree(INSTANCE_INDEX index);
void DestroyMeshInfoTree(IMeshInfoTree *mesh_info);

//void InitMeshInfo(MeshInfo & info,INSTANCE_INDEX id);
void CopyMeshInfo(MeshInfo & info,const MeshInfo &src,bool bCopyBuffers);
//I hereby define split_n to point in the direction of out1
void SplitMeshInfo(const MeshInfo &src,MeshInfo &out0,MeshInfo &out1,SINGLE split_d,const Vector &split_n);

struct ArchetypeFaceInfo
{
	Vector norm;
//	SINGLE D;
};

struct IRenderMaterial
{
	//Vector *pos_list;
	void *src_verts_buffer;
	U16 *index_list;
	Vector *src_norms;
	float *src_u2;
	float *src_v2;
	ArchetypeFaceInfo *faces;
	U16 vert_cnt;
	//TEMP!!!! ?????
	U16 fg_idx;
	U16 new_face_cnt;
	U16 face_offset;  //offset into faceRenders array

	U8 flags;

//	virtual void Render(Mesh *mesh,const Transform &trans)=0;

//	virtual void RenderPortion(Mesh *mesh,const Transform &inv,const Vector &split_plane)=0;

//	virtual void RenderPuffy(Mesh *mesh,const Transform &trans,const Transform &local_to_obj,const Vector &scale)=0;

//	virtual void RenderSolarian(Mesh *mesh,const Transform &trans,const Transform &obj_to_view)=0;

	virtual void Init(Mesh *mesh,int _fg_idx,struct MeshRender *_mr)=0;

	virtual void GetBuffers(void **vertices,U16 **indices,Vector **norms,ArchetypeFaceInfo **_faces,S32 *num_verts,S32 *num_faces) = 0;

//	virtual void SetBuffers(void *vertices,U16 *indices,Vector *norms,ArchetypeFaceInfo *_faces,S32 num_verts,S32 num_faces) = 0;

//	virtual void Clone(IRenderMaterial **rm,bool bCopyBuffers = true) = 0;

	virtual int GetVertexType() = 0;

	virtual int GetVertexSize() = 0;

	virtual ~IRenderMaterial()
	{
	}
};


struct IMeshRender
{
	Vector *pos_list;
	U16 pos_cnt;
	U16 face_cnt;

	virtual void Render (MeshInfo *mc,const Transform &world_to_view) = 0;

	virtual void RenderPortionZAlign (MeshInfo *mc,const Transform &world_to_view,const Vector &split_pt) = 0;

	virtual void RenderPortion (MeshInfo *mc,const Transform &world_to_view,const Vector &split_pt,const Vector &split_norm) = 0;

	virtual void RenderPuffy (MeshInfo *mc,const Transform &world_to_view,const Transform &obj_trans,const Vector &scale) = 0;

	virtual void Init (ARCHETYPE_INDEX idx) = 0;

	virtual void Init (MeshInfo *mc) = 0;

	virtual void SetupMeshInfo (MeshInfo *mc,bool bMakeBuffers=true, const char* materialOverride = 0) = 0;

//	virtual void SplitMesh (IRenderMaterial *out0,IRenderMaterial *out1,const Vector &split_d,const Vector &split_n) = 0;

	virtual IRenderMaterial * GetNextFaceGroup (IRenderMaterial *irm) = 0;

	virtual void AddRef() = 0;

	virtual void Release() = 0;

	virtual ~IMeshRender()
	{
	}

//	virtual void AddFaceGroup (IRenderMaterial *irm) = 0;
	//create a new MeshInfo struct, not necessarily connected to an engine mesh
//	void MakeMeshInfo(MeshInfo *mc);
};

/*struct InstanceRender
{
	MyFace *myFaceArray;
	S32 face_cnt;
	struct LightRGB *lit;
	FaceGroupInfo *fgi;
};*/

//void DeleteMeshRenders(IMeshRender **imr,int num);
//void AllocateMeshRenders(IMeshRender **imr,int num);
IMeshRender * CreateMeshRender();
//I hereby define split_n to point in the direction of out1
void SplitMesh(const MeshInfo &src,MeshInfo &out0,MeshInfo &out1,SINGLE split_d,const Vector &split_n);
void CopyMesh(IMeshRender *src,IMeshRender *dest,bool bCopyBuffers=true);
void TreeRender(MeshInfo **mi,int num_children);
void TreeRender(MeshInfo **mi,int num_children,const Transform &scaleTrans);
void TreeRender(MeshInfo *mi,const Transform &scaleTrans);
void TreeRender(MeshChain &mc,bool bLight=true, DWORD color = 0xFFFFFFFF);
void TreeRenderPortionZAlign(MeshInfo **mc,int num_children,const Vector &split_pt);
void TreeRenderPortion(MeshInfo **mc,int num_children,const Vector &split_pt,const Vector &split_norm);
void TreeRenderPuffy(MeshInfo **mc,int num_children,const Vector &scale,SINGLE puffPercent);
void StackRender(MeshInfo **mc,int num_children);
void AlphaStackRender(MeshInfo **mi,int num_children); //useful for aiding sorting
void EmptyStack();

#endif
//---------------------------------------------------------------------------
//---------------------------End MeshRender.h--------------------------------
//---------------------------------------------------------------------------
