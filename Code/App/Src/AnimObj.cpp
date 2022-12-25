//--------------------------------------------------------------------------//
//                                                                          //
//                                 AnimObj.cpp                                //
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

#include <FileSys.h>
#include <Engine.h>
#include <IRenderPrimitive.h>


struct AnimObjArchetype
{
	const char *name;
	BT_ANIMOBJ_DATA *data;
	AnimArchetype *animArch;

    void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}

	AnimObjArchetype (void)
	{
	}

	~AnimObjArchetype (void)
	{
		delete animArch;
	}

};

#define MAX_ANIMS 8

struct DUMMY_SAVESTRUCT			// required by template framework
{
	TRANS_SAVELOAD trans_SL;
};
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//

struct _NO_VTABLE AnimObj : public IBaseObject, IEffect //,IAnimObj
{

	BEGIN_MAP_INBOUND(AnimObj)
	_INTERFACE_ENTRY(IBaseObject)
//	_INTERFACE_ENTRY(IAnimObj)
	_INTERFACE_ENTRY(IEffect)
	END_MAP()

	//------------------------------------------

	SINGLE timeToLive,totalTime;
	AnimObjArchetype *arch;
	AnimInstance *inst;
	HSOUND hSound;
	SINGLE growSpeed; // world units/sec
	SINGLE animWidth;
//	SINGLE animScale;
	SINGLE timePassed;
	U32 cnt;
	U32 spreadFactor;
	IBaseObject *owner;
	TRANSFORM trans;
	bool bFadeOut:1;
	bool bFaceFront:1;
	bool bSmooth:1;
	bool bLooping:1;

	//------------------------------------------

	AnimObj (void)// : point(0)
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

	virtual ~AnimObj (void);	// See ObjList.cpp

	/* IBaseObject methods */

	virtual void PhysicalUpdate (SINGLE dt);

	virtual BOOL32 Update ();

	virtual void Render (void);

/*	virtual U32 GetSystemID (void) const
	{
		return systemID;
	}*/

	//IEffect
	
	virtual BOOL32 InitEffect ( IBaseObject *_owner,const class TRANSFORM & orientation,SINGLE _animScale,SINGLE lifeTime);

	virtual BOOL32 EditorInitEffect ( IBaseObject *_owner,SINGLE lifeTime);

	virtual SINGLE GetRadius ();

	//?????????????

	virtual AnimInstance *GetAnim() 
	{
		return inst;
	}
};

//----------------------------------------------------------------------------------
//
AnimObj::~AnimObj (void)
{
	if (inst)
		delete inst;
}
//----------------------------------------------------------------------------------
//
void AnimObj::PhysicalUpdate (SINGLE dt)
{
	CQASSERT(totalTime);

	timePassed += dt;

	inst->update(dt);

	animWidth += growSpeed*dt;
	inst->SetWidth(animWidth);
	
	if (bFadeOut)
	{
		U8 alpha = (timeToLive/totalTime)*255.0;
		inst->SetColor(alpha,alpha,alpha,255);
		//inst->SetColor(255,255,255,alpha);
	}
}

BOOL32 AnimObj::Update ()
{
	SINGLE updateTime = totalTime-timeToLive;
	
	if (updateTime-timePassed > ELAPSED_TIME)
	{
		PhysicalUpdate(ELAPSED_TIME);
	}

	return ((timeToLive -= ELAPSED_TIME) > 0 && animWidth > 0);
}
//----------------------------------------------------------------------------------
//
void AnimObj::Render (void)
{
	TRANSFORM transform;

//	PIPE->set_pipeline_state(RP_BATCH_TRANSLUCENT_MODE, RP_TRANSLUCENT_DEPTH_SORTED );
	if (owner)
	{
		TRANSFORM ownerTrans = owner->GetTransform();
		transform = ownerTrans.multiply(trans);
	}
	
	if (inst)
	{
		BATCH->set_render_state(D3DRS_ZWRITEENABLE,FALSE);
		BATCH->set_render_state(D3DRS_ALPHABLENDENABLE,TRUE);
		if (arch->data->bHasAlphaChannel == 0)
		{
			BATCH->set_render_state(D3DRS_SRCBLEND,D3DBLEND_ONE);
			BATCH->set_render_state(D3DRS_DESTBLEND,D3DBLEND_ONE);
		}
		else
		{
			BATCH->set_render_state(D3DRS_SRCBLEND,D3DBLEND_SRCALPHA);
			BATCH->set_render_state(D3DRS_DESTBLEND,D3DBLEND_INVSRCALPHA);
		}

		if (bSmooth && CQRENDERFLAGS.bMultiTexture)
			ANIM2D->render_smooth(inst,&transform);
		else
			ANIM2D->render(inst,&transform);
	}
	//	PIPE->set_pipeline_state(RP_BATCH_TRANSLUCENT_MODE, RP_TRANSLUCENT_UNSORTED );
}
//----------------------------------------------------------------------------------
//
BOOL32 AnimObj::InitEffect ( IBaseObject *_owner,const class TRANSFORM & orientation,SINGLE _animScale,SINGLE lifeTime)
{
	BOOL32 result = 0;

	owner = _owner;

	SINGLE animScale = _animScale;
	if (inst)
	{
		animWidth *= animScale;
		if (animWidth == 0)
			animWidth = 1;
		inst->SetWidth(animWidth);
	}

/*	if (arch->animArch)
	{
		inst = new AnimInstance;
		if (inst)
		{
			inst->Init(arch->animArch);
			animWidth *= animScale;
			if (animWidth == 0)
				animWidth = 1;
			inst->SetRotation(2*PI*(SINGLE)rand()/RAND_MAX);
			inst->SetWidth(animWidth);
			inst->loop = FALSE;
			inst->alwaysFront = bFaceFront;
		}
	}*/
	
	//	transform = orientation;
	
//	objClass = OC_EFFECT;
	if (lifeTime != 0)
	{
		timeToLive = totalTime = lifeTime;
	}
	else if (timeToLive == 0)
	{
		timeToLive = totalTime = 1;
	}
	CQASSERT(totalTime);
	
	result = 1;

	return result;

}

//----------------------------------------------------------------------------------
//
BOOL32 AnimObj::EditorInitEffect ( IBaseObject *_owner,SINGLE lifeTime)
{
	BOOL32 result = 0;

	owner = _owner;

	SINGLE animScale = 1;
	if (arch->animArch)
	{
		inst = new AnimInstance;
		if (inst)
		{
			inst->Init(arch->animArch);
			animWidth *= animScale;
			if (animWidth == 0)
				animWidth = 1;
			inst->SetRotation(2*PI*(SINGLE)rand()/RAND_MAX);
			inst->SetWidth(animWidth);
			inst->loop = bLooping;
			inst->alwaysFront = bFaceFront;
		}
	}
	
	//	transform = orientation;
	
	objClass = OC_EFFECT;
	if (lifeTime != 0)
	{
		timeToLive = totalTime = lifeTime;
	}
	else if (timeToLive == 0)
	{
		timeToLive = totalTime = 1;
	}
	CQASSERT(totalTime);
	
	result = 1;

	return result;

}
//----------------------------------------------------------------------------------
//
SINGLE AnimObj::GetRadius()
{
	SINGLE result = animWidth*1.414;

	if (growSpeed > 0)
	{
		result = (animWidth+totalTime*growSpeed)*1,414;
	}

	return result;
}

//----------------------------------------------------------------------------------
//---------------------------------AnimObj Factory------------------------------------
//----------------------------------------------------------------------------------
//----------------------------------------------------------------------------------


struct DACOM_NO_VTABLE AnimObjManager : public IObjectFactory
{
	//
	// incoming interface map
	//
  
	BEGIN_DACOM_MAP_INBOUND(AnimObjManager)
	DACOM_INTERFACE_ENTRY(IObjectFactory)
	END_DACOM_MAP()

//	struct AnimObjNode *explosionList;
	U32 factoryHandle;


	//child object info
	AnimObjArchetype *pArchetype;

	//AnimObjManager methods

	AnimObjManager (void) 
	{
	}

	~AnimObjManager();
	
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
// AnimObjManager methods

AnimObjManager::~AnimObjManager()
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
void AnimObjManager::init()
{
	COMPTR<IDAConnectionPoint> connection;


	if (OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
	{
		connection->Advise(GetBase(), &factoryHandle);
	}
}
//--------------------------------------------------------------------------
//
HANDLE AnimObjManager::CreateArchetype(const char *szArchname, OBJCLASS objClass, void *data)
{
	if (objClass == OC_EFFECT)
	{
		BT_ANIMOBJ_DATA *objData = (BT_ANIMOBJ_DATA *)data;
		if (objData->fxClass == FX_ANIMOBJ)
		{
			AnimObjArchetype *newguy = new AnimObjArchetype;
			newguy->name = szArchname;
			newguy->data = objData;
			
			if (objData->animName)
			{
				DAFILEDESC fdesc;
				COMPTR<IFileSystem> objFile;
				fdesc.lpFileName = objData->animName;
				if (OBJECTDIR->CreateInstance(&fdesc, objFile) == GR_OK)
				{
					newguy->animArch = ANIM2D->create_archetype(objFile);
				}
				else 
				{
					CQFILENOTFOUND(fdesc.lpFileName);
					newguy->animArch =0;
					delete newguy;
					return 0;
				}
			}
			return newguy;
		}
	}

	return 0;
}
//--------------------------------------------------------------------------
//
BOOL32 AnimObjManager::DestroyArchetype(HANDLE hArchetype)
{
	AnimObjArchetype *deadguy = (AnimObjArchetype *)hArchetype;

	delete deadguy;
	return 1;
}
//--------------------------------------------------------------------------
//
IBaseObject * AnimObjManager::CreateInstance(HANDLE hArchetype)
{
	AnimObjArchetype *pAnimObj = (AnimObjArchetype *)hArchetype;
	//BT_AnimObj *objData = ((AnimObjArchetype *)hArchetype)->data;
	
	AnimObj * obj = new ObjectImpl<AnimObj>;
	obj->objClass = OC_EFFECT;
	obj->growSpeed = pAnimObj->data->sizeVel;
	obj->totalTime = obj->timeToLive = pAnimObj->data->lifeTime;
	obj->bFadeOut = pAnimObj->data->fadeOut;
	obj->bFaceFront = pAnimObj->data->faceFront;
	obj->animWidth = pAnimObj->data->animSize;
	obj->bSmooth = pAnimObj->data->bSmooth;
	obj->bLooping = pAnimObj->data->bLooping;

//	obj->data = objData;
	obj->arch = pAnimObj;
	return obj;
	
}
//--------------------------------------------------------------------------
//
void AnimObjManager::EditorCreateInstance(HANDLE hArchetype, const MISSION_INFO & info)
{
	// not supported
}
//----------------------------------------------------------------------------------------------
//
/*struct IBaseObject * __stdcall CreateAnimObj (PARCHETYPE pArchetype)
{
	IBaseObject *result=0;

	if (pArchetype == 0)
		goto Done;
	
	result = AnimObjMGR->CreateInstance(pArchetype->hArchetype);
 	if (result)
 	{
		OBJPTR<IAnimObj> AnimObj;

		result->QueryInterface(IAnimObjID, AnimObj);
		CQASSERT(AnimObj!=0);
 	}

Done:

	return result;
}*/
//----------------------------------------------------------------------------------------------
//
struct _ung : GlobalComponent
{
	struct AnimObjManager *AnimObjMgr;

	virtual void Startup (void)
	{
		AnimObjMgr = new DAComponent<AnimObjManager>;
		AddToGlobalCleanupList((IDAComponent **) &AnimObjMgr);
	}

	virtual void Initialize (void)
	{
		AnimObjMgr->init();
	}
};

static _ung ung;
//----------------------------------------------------------------------------------
//----------------------------------------------------------------------------------

//---------------------------------------------------------------------------
//--------------------------End AnimObj.cpp------------------------------------
//---------------------------------------------------------------------------
