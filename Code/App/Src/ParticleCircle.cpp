//--------------------------------------------------------------------------//
//                                                                          //
//                             ParticleCircle.cpp                           //
//                                                                          //
//                  COPYRIGHT (C) 2004 WARTHOG, INC.                        //
//                                                                          //
//--------------------------------------------------------------------------//
/*
*/			    
//--------------------------------------------------------------------------//

#include "pch.h"
#include <globals.h>

#include "TObject.h"
#include "IConnection.h"
#include "DEffect.h"
#include "TObjTrans.h"
#include "TobjFrame.h"
#include "Startup.h"
#include "IParticleCircle.h"
#include "Objlist.h"
#include "CQlight.h"
#include "Camera.h"
#include "Objmap.h"
#include "TManager.h"
#include "Anim2d.h"

#include <FileSys.h>

struct DUMMY_SAVESTRUCT			// required by template framework
{
	TRANS_SAVELOAD trans_SL;
};

struct ParticleCircleArchetype
{
	INSTANCE_INDEX archIndex;
	IMeshArchetype * meshArch;
	BT_PARTICLE_CIRCLE * pData;
	
	ParticleCircleArchetype()
	{
		meshArch = NULL;
		archIndex = -1;
	}

	~ParticleCircleArchetype()
	{
	}
};

#define VECTOR_SPEED 0.3

struct _NO_VTABLE ParticleCircle : public ObjectTransform<ObjectFrame<IBaseObject,struct DUMMY_SAVESTRUCT,ParticleCircleArchetype> >, IParticleCircle
{
	BEGIN_MAP_INBOUND(ParticleCircle)
	_INTERFACE_ENTRY(IBaseObject)
	_INTERFACE_ENTRY(IParticleCircle)
	_INTERFACE_ENTRY(IPhysicalObject)
	END_MAP()

	typedef INITINFO PARTCIRCLEINITINFO;
	BT_PARTICLE_CIRCLE * pData;

	OBJPTR<IBaseObject> owner;

	SINGLE radius;

	bool bActive;
	SINGLE timer;

	ParticleCircle();

	~ParticleCircle();

	//IBaseObject

	virtual void Render();

	virtual U32 GetSystemID() const
	{
		if(owner)
			return owner.Ptr()->GetSystemID();
		return 0xFFFFFFFF;
	};

	BOOL32 Update ();

	virtual void CastVisibleArea();

	void PhysicalUpdate (SINGLE dt);

	//IParticleCircle

	virtual void InitParticleCircle(IBaseObject * owner,SINGLE radius);

	virtual void SetActive(bool bSetting);

};
//----------------------------------------------------------------------------------
//
ParticleCircle::ParticleCircle()
{
	bActive = false;
	timer = 0;
}
//----------------------------------------------------------------------------------
//
ParticleCircle::~ParticleCircle()
{
}
//----------------------------------------------------------------------------------
//
void ParticleCircle::Render()
{
	if(bVisible)
	{
		if(bActive && owner && owner.Ptr()->bSelected)
		{
			drawRangeCircle(radius/GRIDSIZE,RGB(pData->color.red,pData->color.green,pData->color.blue));
			drawRangeCircle((radius*0.99)/GRIDSIZE,RGB(pData->color.red,pData->color.green,pData->color.blue));
			
			drawPartialRangeCircle((radius*0.75)/GRIDSIZE,timer*VECTOR_SPEED,RGB(pData->color.red,pData->color.green,pData->color.blue));
			drawPartialRangeCircle((radius*0.5)/GRIDSIZE,-timer*VECTOR_SPEED,RGB(pData->color.red,pData->color.green,pData->color.blue));
			drawPartialRangeCircle((radius*0.25)/GRIDSIZE,timer*VECTOR_SPEED,RGB(pData->color.red,pData->color.green,pData->color.blue));

			SINGLE moveVar1 = fmod(timer*VECTOR_SPEED,1.0);
			drawRangeCircle((radius*moveVar1)/GRIDSIZE,RGB(pData->color.red*moveVar1,pData->color.green*moveVar1,pData->color.blue*moveVar1));

			if(moveVar1 > 0.5)
				moveVar1 = moveVar1-0.5;
			else
				moveVar1 = moveVar1+0.5;

			drawRangeCircle((radius*moveVar1)/GRIDSIZE,RGB(pData->color.red*moveVar1,pData->color.green*moveVar1,pData->color.blue*moveVar1));
		}
	}
}
//----------------------------------------------------------------------------------
//
void ParticleCircle::CastVisibleArea()
{
	if(owner)
	{
		const U32 mask = MGlobals::GetAllyMask(owner.Ptr()->GetPlayerID());
		SetVisibleToAllies(mask);
	}
}
//----------------------------------------------------------------------------------
//
BOOL32 ParticleCircle::Update ()
{
	if(!owner)
		return 0;
	return 1;
}
//----------------------------------------------------------------------------------
//
void ParticleCircle::PhysicalUpdate (SINGLE dt)
{
	if(owner.Ptr())
        SetTransform(owner->GetTransform(),owner.Ptr()->GetSystemID());
	if(bActive && bVisible)
	{
		timer += dt;
	}
}
//----------------------------------------------------------------------------------
//
void ParticleCircle::InitParticleCircle(IBaseObject * _owner,SINGLE _radius)
{
	_owner->QueryInterface(IBaseObjectID,owner,NONSYSVOLATILEPTR);
	radius = _radius;
}
//----------------------------------------------------------------------------------
//
void ParticleCircle::SetActive(bool bSetting)
{
	bActive = bSetting;
}
//----------------------------------------------------------------------------------
//---------------------------------ParticleCircle Factory------------------------------------
//----------------------------------------------------------------------------------
//----------------------------------------------------------------------------------


struct DACOM_NO_VTABLE ParticleCircleManager : public IObjectFactory
{
	//
	// incoming interface map
	//
  
	BEGIN_DACOM_MAP_INBOUND(ParticleCircleManager)
	DACOM_INTERFACE_ENTRY(IObjectFactory)
	END_DACOM_MAP()

//	struct BlastNode *explosionList;
	U32 factoryHandle;

	//BlastManager methods

	ParticleCircleManager (void) 
	{
	}

	~ParticleCircleManager();
	
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
// ParticleCircleManager methods

ParticleCircleManager::~ParticleCircleManager()
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
void ParticleCircleManager::init()
{
	COMPTR<IDAConnectionPoint> connection;


	if (OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
	{
		connection->Advise(GetBase(), &factoryHandle);
	}
}
//--------------------------------------------------------------------------
//
HANDLE ParticleCircleManager::CreateArchetype(const char *szArchname, OBJCLASS objClass, void *data)
{
	if (objClass == OC_EFFECT)
	{
		BT_PARTICLE_CIRCLE* objData = (BT_PARTICLE_CIRCLE * )data;
		if (objData->fxClass == FX_PARTICLE_CIRCLE)
		{
			ParticleCircleArchetype * handle = new ParticleCircleArchetype;
			handle->pData = objData;
			return handle;
		}
	}

	return 0;
}
//--------------------------------------------------------------------------
//
BOOL32 ParticleCircleManager::DestroyArchetype(HANDLE hArchetype)
{
	ParticleCircleArchetype *deadguy = (ParticleCircleArchetype *)hArchetype;

	delete deadguy;
	return 1;
}
//--------------------------------------------------------------------------
//
IBaseObject * ParticleCircleManager::CreateInstance(HANDLE hArchetype)
{
	ParticleCircleArchetype *pBlast = (ParticleCircleArchetype *)hArchetype;

	ParticleCircle * obj = new ObjectImpl<ParticleCircle>;
	obj->objClass = OC_EFFECT;
	obj->pData = pBlast->pData;

	obj->FRAME_init(*pBlast);

	return obj;
}
//--------------------------------------------------------------------------
//
void ParticleCircleManager::EditorCreateInstance(HANDLE hArchetype, const MISSION_INFO & info)
{
	// not supported
}

//----------------------------------------------------------------------------------------------
//
struct PartCircGlobal : GlobalComponent
{
	struct ParticleCircleManager *blastMgr;

	virtual void Startup (void)
	{
		blastMgr = new DAComponent<ParticleCircleManager>;
		AddToGlobalCleanupList((IDAComponent **) &blastMgr);
	}

	virtual void Initialize (void)
	{
		blastMgr->init();
	}
};

static PartCircGlobal _PartCircGlobal;
//----------------------------------------------------------------------------------
//----------------------------------------------------------------------------------

//---------------------------------------------------------------------------
//--------------------------End ParticleCircle.cpp------------------------------------
//---------------------------------------------------------------------------
