#ifndef TOBJARTIFACT_H
#define TOBJARTIFACT_H
//--------------------------------------------------------------------------//
//                                                                          //
//                              TObjArtifact.h                              //
//                                                                          //
//                  COPYRIGHT (C) 2004 BY WARTHOG, INC.                     //
//                                                                          //
//--------------------------------------------------------------------------//
//------------------------------- #INCLUDES --------------------------------//
//--------------------------------------------------------------------------//

#ifndef MESH_H
#include "Mesh.h"
#endif

#ifndef RENDERER_H
#include "Renderer.h"
#endif

//#ifndef MATERIAL_H
//#include "Material.h"
//#endif

#ifndef _INC_STDLIB
#include <stdlib.h>
#endif

#ifndef ENGINE_H
#include "Engine.h"
#endif

#ifndef ARCHHOLDER_H
#include "ArchHolder.h"
#endif

#ifndef IHARDPOINT_H
#include "IHardPoint.h"
#endif

#ifndef HPENUM_H
#include "HPEnum.h"
#endif

#include "IArtifact.h"

#define ObjectArtifact _Coa
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
template <class Base=IBaseObject> 
struct _NO_VTABLE ObjectArtifact : public Base, IArtifactHolder
{
	struct InitNode	     initNode;
	struct UpdateNode       updateNode;

	typename typedef Base::INITINFO ARTIFACTINITINFO;

	OBJPTR<IArtifact> artifact;

	ObjectArtifact (void);
	~ObjectArtifact (void);

	//IArtifactHolder
	virtual bool HasArtifact();

	virtual struct IBaseObject * GetArtifact();

	virtual void SetArtifact(const char * artifactName);

	virtual void DestroyArtifact();

	virtual void UseArtifactOn(IBaseObject * target, U32 agentID);

	virtual SINGLE GetWeaponRange();

	/* ObjectArtifact methods */
	void initArtifact (const ARTIFACTINITINFO & data);

	BOOL32 updateArtifact(void);
};
//---------------------------------------------------------------------------
//
template <class Base> 
ObjectArtifact< Base >::ObjectArtifact (void) :
					initNode(this, InitProc(CASTINITPROC(&ObjectArtifact::initArtifact))),
					updateNode(this, UpdateProc(&ObjectArtifact::updateArtifact))
{
}

template <class Base> 
ObjectArtifact< Base >::~ObjectArtifact (void) 
{
	if(artifact)
	{
		delete artifact.Ptr();
		artifact = NULL;
	}
}

//---------------------------------------------------------------------------
//
template <class Base>
void ObjectArtifact< Base >::initArtifact (const ARTIFACTINITINFO & data)
{
}
//---------------------------------------------------------------------------
//
template <class Base>
BOOL32 ObjectArtifact< Base >::updateArtifact(void)
{
	if(artifact.Ptr())
		artifact.Ptr()->Update();
	return 1;
}
//---------------------------------------------------------------------------
//
template <class Base>
bool ObjectArtifact< Base >::HasArtifact()
{
	return (artifact.Ptr() != 0);
}
//---------------------------------------------------------------------------
//
template <class Base>
IBaseObject * ObjectArtifact< Base >::GetArtifact()
{
	return artifact.Ptr();
}
//---------------------------------------------------------------------------
//
template <class Base>
void ObjectArtifact< Base >::SetArtifact(const char * artifactName)
{
	CQASSERT(!artifact);
	IBaseObject * obj = ARCHLIST->CreateInstance(artifactName);
	if(obj)
	{
		obj->QueryInterface(IArtifactID,artifact,NONSYSVOLATILEPTR);
		if(artifact.Ptr())
		{
			artifact->InitArtifact(this);
		}
	}
}
//---------------------------------------------------------------------------
//
template <class Base>
void ObjectArtifact< Base >::DestroyArtifact()
{
	CQASSERT(artifact);
	delete artifact.Ptr();
	artifact = NULL;
}
//---------------------------------------------------------------------------
//
template <class Base>
void ObjectArtifact< Base >::UseArtifactOn(IBaseObject * target, U32 agentID)
{
	//no action taken by default 
	THEMATRIX->OperationCompleted(agentID,GetPartID());
}
//---------------------------------------------------------------------------
//
template <class Base>
SINGLE ObjectArtifact< Base >::GetWeaponRange()
{
	return 0.0;
}


//---------------------------------------------------------------------------
//---------------------------End TObjGlow.h---------------------------------
//---------------------------------------------------------------------------
#endif