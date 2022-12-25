#ifndef BBMESH_H
#define BBMESH_H
//--------------------------------------------------------------------------//
//                                                                          //
//                                BBMesh.h                                  //
//                                                                          //
//                  COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/BBMesh.h 3     6/14/00 4:06p Rmarr $
*/			    
//------------------------------- #INCLUDES --------------------------------//
//--------------------------------------------------------------------------//
#ifndef DBASEDATA_H
#include "DBaseData.h"
#endif

#ifndef MORPHMESH_H
#include "IMorphMesh.h"
#endif

#ifndef DJUMPGATE_H
#include "DJumpgate.h"
#endif

#ifndef HEAPOBJ_H
#include "HeapObj.h"
#endif

// BILLBOARD MESH CLASS
struct Billboard
{
	S32 size;
	SINGLE rotation;
	SINGLE frameOffset;
};

struct BB_Mesh
{
	U32 bb_txm;
	struct AnimArchetype *animArch;
	struct Billboard *bb_data;
	U32 *v_sort;
	Vector *p_sort;
	Parameters params;
	SINGLE timer;
	COMPTR<IMorphMesh> morphMesh;
	U32 numVerts;
//	bool bAdditive;
	BLENDS blendMode;

	void * operator new (size_t size)
	{
		return HEAP->ClearAllocateMemory(size, "BB_Mesh");
	}

	void Update(SINGLE dt);

	void SetMesh(MorphMeshArchetype *mm_arch,BILLBOARD_MESH *data);

	void SetMesh(ARCHETYPE_INDEX id,BILLBOARD_MESH *data);

	ARCHETYPE_INDEX GetMeshID();

	SINGLE GetMeshes(Mesh **mesh1,Mesh **mesh2);

	S32 GetVertices(Vector *vertices,Parameters *params);

	S32 GetMesh0(Vector *vertices,Parameters *params);


	BB_Mesh();

	~BB_Mesh (void);
	
protected:
	ARCHETYPE_INDEX bb_mesh;
};

#endif