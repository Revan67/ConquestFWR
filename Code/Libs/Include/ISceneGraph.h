//ISceneGraph.h
//
//

#ifndef _ISCENE_GRAPH_H_
#define _ISCENE_GRAPH_H_

#include "DACOM.h"

#define IID_ISceneGraph MAKE_IID("ISceneGraph",1)
#define CLSID_SceneGraph "SceneGraph"

class Transform;
struct IMaterial;
struct IModifier;
struct IDirect3DIndexBuffer9;

struct ISceneNode
{
	//This should contain all the data needed to call draw primitive and to sort the object...
	virtual void SetRenderInfo(Transform world, U32 vertexBufferID, IDirect3DIndexBuffer9 * indexBuffer,
		IMaterial * material, U32 startVertex, U32 numVerts, U32 startIndex, U32 numIndex, IModifier * modList,
		float timer) = 0;

	//returns this node to the free pool for reallocation
	virtual void Release() = 0;
};

struct DACOM_NO_VTABLE ISceneGraph : public IDAComponent
{
	virtual GENRESULT COMAPI InitSceneGraph() = 0;

	virtual ISceneNode * COMAPI CreateSceneNode() = 0;

	virtual void COMAPI RenderNode(ISceneNode * node) = 0;

	virtual void COMAPI RenderScene() = 0;
};

#endif