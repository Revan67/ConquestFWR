//--------------------------------------------------------------------------//
//                                                                          //
//                              MeshObj.cpp                                 //
//                                                                          //
//                  COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/MeshObj.cpp 3     10/08/00 9:45p Rmarr $
*/			    

#include "pch.h"
#include <globals.h>

#include "TObjRender.h"
#include "MeshRender.h"

IMeshObj::~IMeshObj()
{}

struct MeshObj : IMeshObj
{
	struct IMeshInfoTree *mesh_info;
	struct IMeshRender **mr;
		
	~MeshObj();

	void Init(RenderArch *renderArch,ARCHETYPE_INDEX archID);
};

MeshObj::~MeshObj()
{
	if (mesh_info)
		DestroyMeshInfoTree(mesh_info);
	ENGINE->destroy_instance(id);
}

void MeshObj::Init(RenderArch *renderArch,ARCHETYPE_INDEX archID)
{
	id = ENGINE->create_instance2(archID,0);
	CQASSERT( id != INVALID_INSTANCE_INDEX);

	mesh_info = CreateMeshInfoTree(id);
	mc.numChildren = mesh_info->ListChildren(mc.mi);
	mr = renderArch->mr;

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

		IMeshRender ***pmr = (IMeshRender ***)(void *)&renderArch->mr;
		int *nm = (int *)(void *)&renderArch->numChildren;
		*nm = mc.numChildren;
		*pmr = mr;
	}
		
	for (int i=0;i<mc.numChildren;i++)
	{
		mr[i]->SetupMeshInfo(mc.mi[i]);
	}
}

IMeshObj * CreateMeshObj(RenderArch *renderArch,ARCHETYPE_INDEX archID)
{
	MeshObj *result = new MeshObj;
	result->Init(renderArch,archID);

	return result;
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//
