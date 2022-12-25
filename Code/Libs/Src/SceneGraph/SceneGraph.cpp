//SceneGraph.cpp
//
//

#include "SceneGraph.h"
#include <3dMath.h>
#include <FDump.h>

SceneGraph::SceneGraph():
	mbInitialized(false)
{
}

SceneGraph::~SceneGraph()
{
	while(mFreeList.size())
	{
		delete mFreeList.front();
		mFreeList.pop_front();
	}
}

void SceneGraph::AddToFreeList(SceneNode * node)
{
	if(mbInitialized)
	{
		mFreeList.push_front(node);
	}
	else
	{
		delete node;
	}
}


GENRESULT COMAPI SceneGraph::InitSceneGraph()
{
	mbInitialized = true;
	return GR_OK;
}

ISceneNode * COMAPI SceneGraph::CreateSceneNode()
{
	ASSERT(mbInitialized);
	ISceneNode * node = NULL;
	if(mFreeList.size())
	{
		node = mFreeList.front();
		mFreeList.pop_front();
	}
	else
	{
		node = new SceneNode(this);
	}
	return node;
}

void COMAPI SceneGraph::RenderNode(ISceneNode * node)
{
	ASSERT(mbInitialized);
	mRenderList.push_back((SceneNode*)node);
}

void COMAPI SceneGraph::RenderScene()
{
	ASSERT(mbInitialized);

	//todo
}


GENRESULT COMAPI SceneGraph::Initialize(void)
{
	return GR_OK;
}

GENRESULT SceneGraph::init( AGGDESC *desc )
{
	return GR_OK;
}

//SceneNode

SceneNode::SceneNode(SceneGraph * parent) :
	mParent(parent)
{
	Clear();
}

SceneNode::~SceneNode()
{
}

void SceneNode::Clear()
{
	mWorld.set_identity();
	mVertexBuffer = 0;
	mIndexBuffer = 0;
	mMaterial = 0;
	mStartVertex = 0;
	mNumVerts = 0;
	mStartIndex = 0;
	mNumIndex = 0;
	mModList = 0;
	mTimer = 0.0f;
}

void SceneNode::SetRenderInfo(Transform world, U32 vertexBufferID, IDirect3DIndexBuffer9 * indexBuffer,
		IMaterial * material, U32 startVertex, U32 numVerts, U32 startIndex, U32 numIndex, IModifier * modList,
		float timer)
{
	mWorld = world;
	mVertexBuffer = vertexBufferID;
	mIndexBuffer = indexBuffer;
	mMaterial = material;
	mStartVertex = startVertex;
	mNumVerts = numVerts;
	mStartIndex = startIndex;
	mNumIndex = numIndex;
	mModList = modList;
	mTimer = timer;
}

void SceneNode::Release()
{
	Clear();
	mParent->AddToFreeList(this);
}
