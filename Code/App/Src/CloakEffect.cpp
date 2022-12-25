//--------------------------------------------------------------------------//
//                                                                          //
//                                 CloakEffect.cpp                                //
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

#include <Mesh.h>
#include <FileSys.h>
#include <Engine.h>
#include <IRenderPrimitive.h>
#include <Renderer.h>

#define FPS 3
#define LIFETIME 1.25

struct CloakEffectArchetype
{
	const char *name;
//	BT_CLOAKEFFECT_DATA *data;
	AnimArchetype *animArch;
	S32 archIndex;

    void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}

	CloakEffectArchetype (void)
	{
		archIndex = INVALID_INSTANCE_INDEX;
	}

	~CloakEffectArchetype (void)
	{
		delete animArch;
		if (archIndex != INVALID_INSTANCE_INDEX)
			ENGINE->release_archetype(archIndex);
	}

};

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//

struct _NO_VTABLE CloakEffect : public IBaseObject, ICloakEffect
{

	BEGIN_MAP_INBOUND(CloakEffect)
	_INTERFACE_ENTRY(IBaseObject)
	_INTERFACE_ENTRY(ICloakEffect)
	END_MAP()

	//------------------------------------------

	SINGLE timeToLive;
	AnimArchetype *animArch;
	U32 ownerID;
	INSTANCE_INDEX instanceIndex;
	bool bPing;
	SINGLE frameCounter;

	//------------------------------------------

	CloakEffect (void)
	{
		timeToLive = LIFETIME;
		frameCounter = 2.5;
	}

    void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}

	virtual ~CloakEffect (void);	// See ObjList.cpp

	/* IBaseObject methods */

	virtual void PhysicalUpdate (SINGLE dt);

	virtual BOOL32 Update ();

	virtual void Render (void);

	// CloakEffect methods

	void Init(IBaseObject *_owner);
};

//----------------------------------------------------------------------------------
//
CloakEffect::~CloakEffect (void)
{
}
//----------------------------------------------------------------------------------
//
void CloakEffect::PhysicalUpdate (SINGLE dt)
{
	if (bPing)
	{
		frameCounter += dt*FPS;
		if (frameCounter > 2.5f)
			bPing = FALSE;
		frameCounter = min(frameCounter,3.9f);
	}
	else
	{
		frameCounter -= dt*FPS;
		if (frameCounter < 0.5f)
			bPing = TRUE;
		frameCounter = max(frameCounter,0.0f);
	}
}

BOOL32 CloakEffect::Update ()
{
	IBaseObject *owner = OBJLIST->FindObject(ownerID);

	if (owner == 0)
		return 0;

	return ((timeToLive -= ELAPSED_TIME) > 0);
}
//----------------------------------------------------------------------------------
//
void CloakEffect::Render (void)
{

}

void CloakEffect::Init(IBaseObject *_owner)
{
	ownerID = _owner->GetPartID();
}
//----------------------------------------------------------------------------------
//

//----------------------------------------------------------------------------------
//---------------------------------CloakEffect Factory------------------------------------
//----------------------------------------------------------------------------------
//----------------------------------------------------------------------------------


struct DACOM_NO_VTABLE CloakEffectManager : public IObjectFactory
{
	//
	// incoming interface map
	//
  
	BEGIN_DACOM_MAP_INBOUND(CloakEffectManager)
	DACOM_INTERFACE_ENTRY(IObjectFactory)
	END_DACOM_MAP()

//	struct CloakEffectNode *explosionList;
	U32 factoryHandle;


	//child object info
	CloakEffectArchetype *pArchetype;

	//CloakEffectManager methods

	CloakEffectManager (void) 
	{
	}

	~CloakEffectManager();
	
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
// CloakEffectManager methods

CloakEffectManager::~CloakEffectManager()
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
void CloakEffectManager::init()
{
	COMPTR<IDAConnectionPoint> connection;


	if (OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
	{
		connection->Advise(GetBase(), &factoryHandle);
	}
}
//--------------------------------------------------------------------------
//
HANDLE CloakEffectManager::CreateArchetype(const char *szArchname, OBJCLASS objClass, void *data)
{
	CloakEffectArchetype *newguy = 0;
	if (objClass == OC_EFFECT)
	{
		BT_CLOAKEFFECT_DATA *objData = (BT_CLOAKEFFECT_DATA *)data;
		if (objData->fxClass == FX_CLOAKEFFECT)
		{
			newguy = new CloakEffectArchetype;
			newguy->name = szArchname;
//			newguy->data = objData;
			
			DAFILEDESC fdesc;
			COMPTR<IFileSystem> objFile;

			if (objData->animName)
			{
				fdesc.lpFileName = objData->animName;//"cloak_ripple.anm";
				if (OBJECTDIR->CreateInstance(&fdesc, objFile) == GR_OK)
				{
					newguy->animArch = ANIM2D->create_archetype(objFile);
				}
				else 
				{
					CQFILENOTFOUND(fdesc.lpFileName);
					newguy->animArch =0;
					goto Error;
				}
			}
			else
			{
				CQERROR0("No animation file specified"); 
				goto Error;
			}

			if (objData->objectName)
			{
				fdesc.lpFileName = objData->objectName;//"cloakshape.3db";
				if (OBJECTDIR->CreateInstance(&fdesc, objFile) == GR_OK)
				{
					TEXLIB->load_library(objFile, 0);
					if ((newguy->archIndex = ENGINE->create_archetype(fdesc.lpFileName, objFile)) == INVALID_ARCHETYPE_INDEX)
					{
						goto Error;
					}
				}
				else 
				{
					delete newguy->animArch;
					newguy->animArch =0;
					CQFILENOTFOUND(fdesc.lpFileName);
					goto Error;
				}
			}
			else
			{
				CQERROR0("No object file specified"); 
				goto Error;
			}

			goto Done;
		}
	}
Error:
	delete newguy;
	newguy = 0;

Done:
	return newguy;
}
//--------------------------------------------------------------------------
//
BOOL32 CloakEffectManager::DestroyArchetype(HANDLE hArchetype)
{
	CloakEffectArchetype *deadguy = (CloakEffectArchetype *)hArchetype;

	delete deadguy;
	return 1;
}
//--------------------------------------------------------------------------
//
IBaseObject * CloakEffectManager::CreateInstance(HANDLE hArchetype)
{
	CloakEffectArchetype *pCloakEffect = (CloakEffectArchetype *)hArchetype;
	
	CloakEffect * obj = new ObjectImpl<CloakEffect>;
	obj->objClass = OC_EFFECT;
	obj->animArch = pCloakEffect->animArch;
	obj->instanceIndex = ENGINE->create_instance2(pCloakEffect->archIndex,0);

//	obj->arch = pCloakEffect;

	return obj;
	
}
//--------------------------------------------------------------------------
//
void CloakEffectManager::EditorCreateInstance(HANDLE hArchetype, const MISSION_INFO & info)
{
	// not supported
}

//----------------------------------------------------------------------------------------------
//
struct _ag : GlobalComponent
{
	struct CloakEffectManager *CloakEffectMgr;

	virtual void Startup (void)
	{
		CloakEffectMgr = new DAComponent<CloakEffectManager>;
		AddToGlobalCleanupList((IDAComponent **) &CloakEffectMgr);
	}

	virtual void Initialize (void)
	{
		CloakEffectMgr->init();
	}
};

static _ag ag;
//----------------------------------------------------------------------------------
//----------------------------------------------------------------------------------

//---------------------------------------------------------------------------
//--------------------------End CloakEffect.cpp------------------------------------
//---------------------------------------------------------------------------
