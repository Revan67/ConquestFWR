#ifndef TOBJRENDER_H
#define TOBJRENDER_H
//--------------------------------------------------------------------------//
//                                                                          //
//                              TObjRender.h                                //
//                                                                          //
//                  COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/TObjRender.h 4     10/08/00 9:45p Rmarr $
*/			    
//------------------------------- #INCLUDES --------------------------------//
//--------------------------------------------------------------------------//

#ifndef MESH_H
#include <Mesh.h>
#endif

#ifndef ARCHHOLDER_H
#include "ArchHolder.h"
#endif

#ifndef IEXPLOSION_H
#include "IExplosion.h"
#endif

#ifndef SECTOR_H
#include "Sector.h"
#endif

#ifndef MESHRENDER_H
#include "MeshRender.h"
#endif

#ifndef RENDERER_H
#include <Renderer.h>
#endif

#ifndef OBJMAP_H
#include "ObjMap.h"
#endif

#ifndef USERDEFAULTS_H
#include "UserDefaults.h"
#endif


//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//

struct RenderArch
{
	int numChildren;
	struct IMeshInfoTree *mesh_info;
	struct IMeshRender **mr;

	RenderArch()
	{
		mr = 0;
	}

	virtual ~RenderArch()
	{
		for (int i=0;i<numChildren;i++)
		{
			mr[i]->Release();
		}
		delete [] mr;
	}
};

struct IMeshObj
{
	MeshChain mc;
	INSTANCE_INDEX id;

	virtual ~IMeshObj();
};

IMeshObj * CreateMeshObj(RenderArch *renderArch,ARCHETYPE_INDEX archID);

template <class Base=IBaseObject> 
struct _NO_VTABLE ObjectRender : public Base
{
	IMeshInfoTree *mesh_info;//[MAX_CHILDS];
	MeshChain mc;
	IMeshRender **mr;//[MAX_CHILDS];

	typename typedef Base::INITINFO RENDERINITINFO;
	struct InitNode			initNode;
	struct PostRenderNode	postRenderNode;

	//child blasts
	IBaseObject *childBlastList;
	
	ObjectRender (void) : initNode(this,InitProc(&ObjectRender::initRender)),
		postRenderNode(this,RenderProc(&ObjectRender::postRenderExtent))
	{
	}

	~ObjectRender();

	void initRender (const RENDERINITINFO & data);

	virtual MeshChain & GetMeshChain()
	{
		return mc;
	}

	void postRenderExtent();

	virtual IMeshInfoTree *GetMeshInfoTree();

	virtual void SetMeshInfoTree(IMeshInfoTree *_mesh_info);
};

//---------------------------------------------------------------------------
//
template <class Base>
ObjectRender< Base >::~ObjectRender()
{
	IBaseObject *pos = childBlastList;
	while (childBlastList)
	{
		childBlastList = pos->next;
		//delete pos->obj;
		delete pos;
		pos = childBlastList;
	}

	if (mesh_info)
		DestroyMeshInfoTree(mesh_info);
}

//----------------------------------------------------------------------------------
//
template <class Base>
void ObjectRender< Base >::initRender (const RENDERINITINFO & data)
{

	HARCH archIndex = instanceIndex;

	mesh_info = CreateMeshInfoTree(instanceIndex);
	mc.numChildren = mesh_info->ListChildren(mc.mi);
	mr = data.mr;

	if (mr == 0)
	{

		CQASSERT(mc.numChildren);
		typedef IMeshRender * booga;
		mr = new booga[mc.numChildren];
		for (int i=0;i<mc.numChildren;i++)
		{
			mr[i] = CreateMeshRender();
			mr[i]->AddRef();
			mr[i]->Init(mc.mi[i]);
		}

		IMeshRender ***pmr = (IMeshRender ***)(void *)&data.mr;
		int *nm = (int *)(void *)&data.numChildren;
		*nm = mc.numChildren;
		*pmr = mr;
	}
		
	for (int i=0;i<mc.numChildren;i++)
	{
		mr[i]->SetupMeshInfo(mc.mi[i]);
	}

}
//---------------------------------------------------------------------------
//
template <class Base>
void ObjectRender< Base >::postRenderExtent()
{
	IBaseObject *pos = childBlastList;
	const U32 currentSystem = SECTOR->GetCurrentSystem();
	const U32 currentPlayer = MGlobals::GetThisPlayer();
	const USER_DEFAULTS & defaults = *DEFAULTS->GetDefaults();

	while (pos)
	{
		pos->TestVisible(defaults, currentSystem, currentPlayer);
		pos->Render();
		pos = pos->next;
	}
}
//---------------------------------------------------------------------------
//
template <class Base>
IMeshInfoTree * ObjectRender< Base >::GetMeshInfoTree()
{
	return mesh_info;
}
//---------------------------------------------------------------------------
//
template <class Base>
void ObjectRender< Base >::SetMeshInfoTree(IMeshInfoTree *_mesh_info)
{
	mesh_info = _mesh_info;
	if (_mesh_info)
	{
		mc.numChildren = mesh_info->ListChildren(mc.mi);
	}
	else
		mc.numChildren = 0;
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//


#endif
