#ifndef MESHARCH_H
#define MESHARCH_H

//--------------------------------------------------------------------------//
//                                                                          //
//                              MeshArch.H                                  //
//                                                                          //
//               COPYRIGHT (C) 2004 By Warthog, INC.                        //
//                                                                          //
//--------------------------------------------------------------------------//

#include <IMeshManager.h>
#include "IInternalMeshManager.h"
#include <d3d9.h>
#include <IMaterialManager.h>

struct MeshFace
{
	IDirect3DIndexBuffer9 * indexBuffer;
	U32 startVertex;
    U32 numVerts;
	U32 startIndex;
	U32 numIndex;
	IMaterial * mat;
	U32 archIndex;//archetype index the has the transform
	Vector maxBox;
	Vector minBox;
};

struct MeshVertex
{
	Vector pos;
	Vector normal;
	SINGLE u,v,u2,v2;
	Vector tangent;
	Vector binormal;
	U8 red,green,blue,alpha;
};

struct VertexBufferNode
{
	VertexBufferNode * next;
	CQ_VertexType vType;
	U32 vertexBuffer;
	bool bValid;
};

struct MeshArch: public IMeshArchetype
{
	MeshArch * next;
	U32 refCount;
	IInternalMeshManager * owner;

	bool bDynamic:1;
	bool bRef:1;
	bool bRefOverrideVerts:1;
	bool bRefOverrideFaces:1;

	U32 allocatedVerts;
private:
	MeshArch * refArch;
	IMaterial * dynamicMaterial;

	ARCHETYPE_INDEX archIndex;
	SCRIPT_SET_ARCH animArchIndex;

	VertexBufferNode * vertexBuffers;
	MeshVertex * vertexList;//all vertex data for the mesh
	U32 num_verts;//number of verticies to make

	MeshFace * faceArray;
	U32 numFaceGroups;

public:


	//IMeshArchetype

	virtual ARCHETYPE_INDEX GetEngineArchtype();

	virtual SCRIPT_SET_ARCH GetAnimArchtype();

	virtual bool FindFirstHardpoint(HardPointDef & hp);

	virtual bool FindNextHardpoint(HardPointDef & hp);

	virtual U32 FindHardPointIndex(const char * name);

	virtual bool FindHardPontFromIndex(U32 index, HardPointDef & hp);

	virtual bool GetFirstAnimScript(AnimScriptEntry & entry);

	virtual bool GetNextAnimScript(AnimScriptEntry & entry);

	virtual bool GetFirstAnimCue(AnimScriptEntry & entry, AnimCueEntry & cue);

	virtual bool GetNextAnimCue(AnimScriptEntry & entry, AnimCueEntry & cue);

	virtual SINGLE GetAnimationDurration(const char * animName);

	virtual U32 GetNumFaceGroups();

	virtual IMaterial * GetFaceGroupMaterial(U32 fgIndex);

	virtual void ResetRef();

	virtual void MeshOperationSlice(SINGLE sliceDelta, Vector sliceDir, IMeshInstance * targetMesh);

	virtual void MeshOperationCut(SINGLE sliceDelta, Vector sliceDir, IMeshInstance * targetMesh);

	virtual void AddRef();

	virtual void Release();

	//MeshArch

	void ReinitDynamic(IMaterial * mat, U32 numVerts);

	MeshFace * GetFaceArray();

	U32 GetNumVerts();

	MeshVertex * GetVertexList();

	U32 GetArchIndex();

	void Realize();

	void InvalidateBuffers();

	U32 GetVertexBufferForType(CQ_VertexType vType);


	////////////////////////////////////////////
	void buildFullMesh();

	U32 computeNumFaceGroups();

	void buildMeshForIndex(U32 baseIndex,U32 & faceOffset);

	struct IChannel2 * findEventChannel(char * animName);

	bool findHardpoint(struct HpSearch * search, ARCHETYPE_INDEX baseIndex);

	void overrideFaces();

	void overrideVerts();

	MeshArch(ARCHETYPE_INDEX archIndex, SCRIPT_SET_ARCH animArchIndex);

	MeshArch(IMaterial * mat, U32 numVerts);//dynamic mesh contructor

	MeshArch(MeshArch * refSource);//reference arch

	~MeshArch();
};

#endif