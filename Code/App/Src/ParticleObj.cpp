//--------------------------------------------------------------------------//
//                                                                          //
//                                 ParticleObj.cpp                                //
//                                                                          //
//                  COPYRIGHT (C) 1997 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Author: Rmarr $
*/			    
//------------------------------- #INCLUDES --------------------------------//
//--------------------------------------------------------------------------//

#include "pch.h"
#include <globals.h>

#include "TObject.h"
#include "IConnection.h"
#include "Camera.h"
#include "DEffect.h"
#include "Objlist.h"
#include "Anim2D.h"
#include "CQlight.h"
#include "Sfx.h"
#include "TObjTrans.h"
#include "TobjFrame.h"
#include "Startup.h"
#include "IBlast.h"
#include "IParticleSystem.h"


#include <FileSys.h>
#include <Engine.h>
#include <RendPipeline.h>


struct ParticleObjArchetype
{
	const char *name;
//	BT_ParticleObj *data;
	ARCHETYPE_INDEX archIndex;

    void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}

	ParticleObjArchetype (void)
	{
		archIndex = -1;
	}

	~ParticleObjArchetype (void)
	{
		ENGINE->release_archetype(archIndex);
	}

};

struct DUMMY_SAVESTRUCT			// required by template framework
{
	TRANS_SAVELOAD trans_SL;
};
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//

struct _NO_VTABLE ParticleObj : public IBaseObject, IEffect //,IParticleObj
{

	BEGIN_MAP_INBOUND(ParticleObj)
	_INTERFACE_ENTRY(IBaseObject)
//	_INTERFACE_ENTRY(IParticleObj)
	_INTERFACE_ENTRY(IEffect)
	END_MAP()

	//------------------------------------------

	SINGLE timeToLive,totalTime;
	ParticleObjArchetype *arch;
	INSTANCE_INDEX instanceIndex;
	U32 cnt;
	IBaseObject *owner;

	//------------------------------------------

	ParticleObj (void)// : point(0)
	{
		timeToLive = 10;
	}

    void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}

	virtual ~ParticleObj (void);	// See ObjList.cpp

	/* IBaseObject methods */

	virtual BOOL32 Update ();

	virtual void PhysicalUpdate (SINGLE dt)
	{
		ENGINE->update_instance(instanceIndex,0,dt);
	}

	virtual void Render (void);

	// IEffect Methods
	
	virtual BOOL32 InitEffect ( IBaseObject *_owner,const class TRANSFORM & orientation,SINGLE _animScale,SINGLE lifeTime);

	virtual BOOL32 EditorInitEffect ( IBaseObject *_owner,SINGLE lifeTime);

	virtual SINGLE GetRadius ();


};

//----------------------------------------------------------------------------------
//
ParticleObj::~ParticleObj (void)
{
	ENGINE->destroy_instance(instanceIndex);
}
//----------------------------------------------------------------------------------
//
BOOL32 ParticleObj::Update (void)
{
	return ((timeToLive -= (float)ELAPSED_TIME) > 0);
}
//----------------------------------------------------------------------------------
//
void ParticleObj::Render (void)
{
	TRANSFORM transform;

	if (owner)
	{
		TRANSFORM ownerTrans = owner->GetTransform();
		transform = ownerTrans;
	}

	ENGINE->set_transform(instanceIndex,transform);
	ENGINE->render_instance(MAINCAM, instanceIndex, 0, LODPERCENT, 0, NULL);
}
//----------------------------------------------------------------------------------
//
BOOL32 ParticleObj::InitEffect ( IBaseObject *_owner,const class TRANSFORM & orientation,SINGLE _animScale,SINGLE lifeTime)
{
	BOOL32 result = 0;

//	instanceIndex = ENGINE->create_instance(arch->archIndex);
//	CQASSERT(instanceIndex != INVALID_INSTANCE_INDEX);

	owner = _owner;
	CQASSERT(owner);

	//THE ORIENTATION PARAMETER IS IGNORED

	TRANSFORM transform;
	if (owner)
	{
		TRANSFORM ownerTrans = owner->GetTransform();
		transform = ownerTrans;
	}

	ENGINE->set_transform(instanceIndex,transform);
	ENGINE->update_instance(instanceIndex,0,0);

	
	//	transform = orientation;
	
//	objClass = OC_EFFECT;
	//HACK!!! Ugly hack!!
	if ((timeToLive = totalTime = lifeTime*2) == 0)
		timeToLive = totalTime = 1;
	CQASSERT(totalTime);
	
	result = 1;

	return result;

}

//----------------------------------------------------------------------------------
//
BOOL32 ParticleObj::EditorInitEffect ( IBaseObject *_owner,SINGLE lifeTime)
{
	BOOL32 result = 0;

	instanceIndex = ENGINE->create_instance2(arch->archIndex, NULL);
	CQASSERT(instanceIndex != INVALID_INSTANCE_INDEX);

	owner = _owner;

	TRANSFORM transform;
	if (owner)
		transform = owner->GetTransform();

	ENGINE->set_transform(instanceIndex,transform);
	//	transform = orientation;
	
	objClass = OC_EFFECT;
	//HACK!!! Ugly hack!!
	if ((timeToLive = totalTime = lifeTime*2) == 0)
		timeToLive = totalTime = 1;
	CQASSERT(totalTime);
	
	result = 1;

	return result;

}

SINGLE ParticleObj::GetRadius()
{
	CQASSERT(instanceIndex != INVALID_INSTANCE_INDEX);

	COMPTR<IParticleSystem> IPS;

	if(ENGINE->query_instance_interface( instanceIndex, IID_IParticleSystem, (IDAComponent **)(void **)IPS ) == GR_OK)
	{
		ParticleSystemParameters psp;
		IPS->get_parameters( &psp );

//		CQASSERT(psp.bounding_sphere_radius);
		return psp.bounding_sphere_radius;
	}
	else
		CQBOMB0("query_instance_interface() failed");

	return 0;
}
//----------------------------------------------------------------------------------
//---------------------------------ParticleObj Factory------------------------------------
//----------------------------------------------------------------------------------
//----------------------------------------------------------------------------------


struct DACOM_NO_VTABLE ParticleObjManager : public IObjectFactory
{
	//
	// incoming interface map
	//
  
	BEGIN_DACOM_MAP_INBOUND(ParticleObjManager)
	DACOM_INTERFACE_ENTRY(IObjectFactory)
	END_DACOM_MAP()

//	struct ParticleObjNode *explosionList;
	U32 factoryHandle;


	//child object info
	ParticleObjArchetype *pArchetype;

	//ParticleObjManager methods

	ParticleObjManager (void) 
	{
	}

	~ParticleObjManager();
	
    void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}

	IDAComponent * GetBase (void)
	{
		return (IObjectFactory *) this;
	}

	void init();

	//IObjectFactory
	DEFMETHOD_(HANDLE,CreateArchetype) (const char *szArchname, OBJCLASS objClass, void *data);

	DEFMETHOD_(BOOL32,DestroyArchetype) (HANDLE hArchetype);

	DEFMETHOD_(IBaseObject *,CreateInstance) (HANDLE hArchetype);

	DEFMETHOD_(void,EditorCreateInstance) (HANDLE hArchetype, const MISSION_INFO & info);
};


//--------------------------------------------------------------------------
// ParticleObjManager methods

ParticleObjManager::~ParticleObjManager()
{
	COMPTR<IDAConnectionPoint> connection;
	if (OBJLIST)
	{
		if (OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
			connection->Unadvise(factoryHandle);
	}
}
//--------------------------------------------------------------------------
//
void ParticleObjManager::init()
{
	COMPTR<IDAConnectionPoint> connection;


	if (OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
	{
		connection->Advise(GetBase(), &factoryHandle);
	}
}
//--------------------------------------------------------------------------
//
HANDLE ParticleObjManager::CreateArchetype(const char *szArchname, OBJCLASS objClass, void *data)
{
	if (objClass == OC_EFFECT)
	{
		BT_PARTICLE_DATA *objData = (BT_PARTICLE_DATA *)data;
		if (objData->fxClass == FX_PARTICLE)
		{
			ParticleObjArchetype *newguy = new ParticleObjArchetype;
			newguy->name = szArchname;
		//	newguy->data = objData;
			
			COMPTR<IFileSystem> objFile;
			DAFILEDESC fdesc = objData->fileName;
			if (OBJECTDIR->CreateInstance(&fdesc, objFile) == GR_OK)
				newguy->archIndex = ENGINE->create_archetype(fdesc.lpFileName,objFile);

			if (newguy->archIndex == INVALID_ARCHETYPE_INDEX)
			{
				CQBOMB1("Failed to load particle effect - %s",objData->fileName);
				delete newguy;
				goto Error;
			}

			return newguy;
		}
	}

Error:

	return 0;
}
//--------------------------------------------------------------------------
//
BOOL32 ParticleObjManager::DestroyArchetype(HANDLE hArchetype)
{
	ParticleObjArchetype *deadguy = (ParticleObjArchetype *)hArchetype;

	delete deadguy;
	return 1;
}
//--------------------------------------------------------------------------
//
IBaseObject * ParticleObjManager::CreateInstance(HANDLE hArchetype)
{
	ParticleObjArchetype *pParticleObj = (ParticleObjArchetype *)hArchetype;
	//BT_ParticleObj *objData = ((ParticleObjArchetype *)hArchetype)->data;
	
	ParticleObj * obj = new ObjectImpl<ParticleObj>;
	obj->objClass = OC_EFFECT;
//	obj->instanceIndex = ENGINE->create_instance(pParticleObj->archIndex);
//	CQASSERT(obj->instanceIndex != INVALID_INSTANCE_INDEX);

//	obj->data = objData;
	obj->arch = pParticleObj;
	return obj;
	
}
//--------------------------------------------------------------------------
//
void ParticleObjManager::EditorCreateInstance(HANDLE hArchetype, const MISSION_INFO & info)
{
	// not supported
}
//----------------------------------------------------------------------------------------------
//
/*struct IBaseObject * __stdcall CreateParticleObj (PARCHETYPE pArchetype)
{
	IBaseObject *result=0;

	if (pArchetype == 0)
		goto Done;
	
	result = ParticleObjMGR->CreateInstance(pArchetype->hArchetype);
 	if (result)
 	{
		OBJPTR<IParticleObj> ParticleObj;

		result->QueryInterface(IParticleObjID, ParticleObj);
		CQASSERT(ParticleObj!=0);
 	}

Done:

	return result;
}*/
//----------------------------------------------------------------------------------------------
//
struct _wang : GlobalComponent
{
	struct ParticleObjManager *ParticleObjMgr;

	virtual void Startup (void)
	{
		ParticleObjMgr = new DAComponent<ParticleObjManager>;
		AddToGlobalCleanupList((IDAComponent **) &ParticleObjMgr);
	}

	virtual void Initialize (void)
	{
		ParticleObjMgr->init();
	}
};

static _wang wang;
//----------------------------------------------------------------------------------
//----------------------------------------------------------------------------------

//---------------------------------------------------------------------------
//--------------------------End ParticleObj.cpp------------------------------------
//---------------------------------------------------------------------------
