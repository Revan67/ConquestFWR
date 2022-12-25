//SceneGraph.h
//

#ifndef _SCENE_GRAPH_H_
#define _SCENE_GRAPH_H_

#include <ISceneGraph.h>
#include <TComponent.h>
#include <xform.h>
#include <list>

struct SceneGraph;

struct SceneNode : public ISceneNode
{
	SceneNode(SceneGraph * parent);
	~SceneNode();

	void Clear();

	//ISceneNode
	virtual void SetRenderInfo(Transform world, U32 vertexBufferID, IDirect3DIndexBuffer9 * indexBuffer,
		IMaterial * material, U32 startVertex, U32 numVerts, U32 startIndex, U32 numIndex, IModifier * modList,
		float timer);

	virtual void Release();

private:
	Transform				mWorld;
	U32						mVertexBuffer;
	IDirect3DIndexBuffer9 * mIndexBuffer;
	IMaterial *				mMaterial;
	U32						mStartVertex;
	U32						mNumVerts;
	U32						mStartIndex;
	U32						mNumIndex;
	IModifier *				mModList;
	float					mTimer;

	SceneGraph *			mParent;
};

struct SceneGraph : ISceneGraph,
				    IAggregateComponent

{
	BEGIN_DACOM_MAP_INBOUND(SceneGraph)
	DACOM_INTERFACE_ENTRY(ISceneGraph)
	DACOM_INTERFACE_ENTRY2(IID_ISceneGraph,ISceneGraph)
	DACOM_INTERFACE_ENTRY(IAggregateComponent)
	DACOM_INTERFACE_ENTRY2(IID_IAggregateComponent,IAggregateComponent)
	END_DACOM_MAP()

	SceneGraph();
	~SceneGraph();

	void AddToFreeList(SceneNode * node);

	//ISceneGraph

	virtual GENRESULT COMAPI InitSceneGraph();

	virtual ISceneNode * COMAPI CreateSceneNode();

	virtual void COMAPI RenderNode(ISceneNode * node);

	virtual void COMAPI RenderScene();

	// IAggregateComponent 
	virtual GENRESULT COMAPI Initialize(void);

	virtual GENRESULT init( AGGDESC *desc );

private:
	bool mbInitialized;

	std::list<SceneNode *> mFreeList;

	std::list<SceneNode *> mRenderList;
};
#endif