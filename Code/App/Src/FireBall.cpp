//--------------------------------------------------------------------------//
//                                                                          //
//                                 Fireball.cpp                                //
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
//#include "TObjConnect.h"
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

//#include <connect.h>
//#include <TComponent.h>
#include <FileSys.h>
#include <Engine.h>
#include <IRenderPrimitive.h>


struct FireballArchetype
{
	const char *name;
//	BT_Fireball *data;
	AnimArchetype *animArch;

    void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}

	FireballArchetype (void)
	{
	}

	~FireballArchetype (void)
	{
		delete animArch;
	}

};

#define MAX_ANIMS 12

struct DUMMY_SAVESTRUCT			// required by template framework
{
	TRANS_SAVELOAD trans_SL;
};
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//

struct _NO_VTABLE Fireball : public IBaseObject, IFireball, IEffect
{

	BEGIN_MAP_INBOUND(Fireball)
	_INTERFACE_ENTRY(IBaseObject)
	_INTERFACE_ENTRY(IFireball)
	_INTERFACE_ENTRY(IEffect)
	END_MAP()

	//------------------------------------------

	SINGLE timeToLive,totalTime;
	FireballArchetype *arch;
	AnimInstance *inst[MAX_ANIMS];
//	S32 growSpeed;
	SINGLE animWidth;
	Vector grow1,grow2,pos1,pos2;
	SINGLE animScale;
	U32 cnt;
	U32 spreadFactor;
	U32 spreadStage;
	IBaseObject *owner;
	TRANSFORM trans;

	//------------------------------------------

	Fireball (void)// : point(0)
	{
		timeToLive = 10;
	}

	void * operator new (size_t size)
	{
		return HEAP->ClearAllocateMemory(size, "Fireball");
	}

	void   operator delete (void *ptr)
	{
		HEAP->FreeMemory(ptr);
	}


	virtual ~Fireball (void);	// See ObjList.cpp

	/* IBaseObject methods */

	virtual BOOL32 Update (void);

	virtual void PhysicalUpdate (SINGLE dt);

	virtual void Render (void);

	//  IEffect

	virtual BOOL32 InitEffect ( IBaseObject *_owner,const class TRANSFORM & orientation,SINGLE _animScale,SINGLE lifeTime);

	virtual BOOL32 EditorInitEffect ( IBaseObject *_owner,SINGLE lifeTime);

	virtual SINGLE GetRadius ();


	/////////////////////////
	void SetSpread(Vector _grow1, Vector _grow2, U8 _spreadFactor);

	virtual AnimInstance *GetAnim() 
	{
		return inst[0];
	}
};

//----------------------------------------------------------------------------------
//
Fireball::~Fireball (void)
{
	for (int i=0;i<MAX_ANIMS;i++)
	{
		if (inst[i])
			delete inst[i];
	}
}
//----------------------------------------------------------------------------------
//
void Fireball::PhysicalUpdate (SINGLE dt)
{


	int i;

	CQASSERT(arch->animArch);
	for (i=0;i<MAX_ANIMS;i++)
	{
		if (inst[i])
		{
			inst[i]->update(dt);
		/*	if (growSpeed)
			{
				animWidth += growSpeed;
				inst[i]->SetWidth(animWidth);
				U8 alpha = (timeToLive/totalTime)*255.0;
				inst[i]->SetColor(alpha,alpha,alpha,255);
			}*/
		}
	}
}

BOOL32 Fireball::Update()
{
	if (timeToLive < 0)
		return 0;
	
	CQASSERT(totalTime);
	if (cnt > spreadFactor*10+1)
	{
		pos1.set(0,0,0);
		pos2.set(0,0,0);
		cnt -= spreadFactor*10;
		spreadStage = 0;
	}

	if (timeToLive > totalTime*0.5)
	{
		cnt++;
		if (cnt%10 == 0)
		{
			pos1 += grow1;
			
			
			U32 i=0;
			U32 j=0;
			while (!i && j < MAX_ANIMS)
			{
				if (!inst[j])
					i=j;
				j++;
			}
			
			if (j < MAX_ANIMS)
			{
				inst[i] = new AnimInstance;
				if (inst[i])
				{
					inst[i]->Init(arch->animArch);
					Vector pos = pos1;
					pos.z += 400*(SINGLE)rand()/RAND_MAX-200;
					inst[i]->SetPosition(pos);
					//animWidth = 1000*animScale*(timeToLive/totalTime);
					animWidth = 1000*animScale*(1-0.4*(spreadStage/spreadFactor));
					inst[i]->SetRotation(2*PI*(SINGLE)rand()/RAND_MAX);
					inst[i]->SetWidth(animWidth);
					inst[i]->loop = FALSE;
				}
			}
		}
	/*	if (cnt == 23)
		{
			i=0;
			U32 j=0;
			while (!i && j < MAX_ANIMS)
			{
				if (!inst[j])
					i=j;
				j++;
			}
			
			if (j < MAX_ANIMS)
			{
				inst[i] = new AnimInstance;
				if (inst[i])
				{
					inst[i]->Init(arch->animArch);
					Vector pos(0,0,0);
					pos.z += 400*(SINGLE)rand()/RAND_MAX-200;
					inst[i]->SetPosition(pos);
					animWidth = 1000*animScale;
					inst[i]->SetRotation(2*PI*(SINGLE)rand()/RAND_MAX);
					inst[i]->SetWidth(animWidth);
					inst[i]->loop = FALSE;
				}
			}
		}*/
		if (cnt%10 == 5)
		{
			pos2 += grow2;
			
			U32 i=0;
			U32 j=0;
			while (!i && j < MAX_ANIMS)
			{
				if (!inst[j])
					i=j;
				j++;
			}
			
			if (j < MAX_ANIMS)
			{
				inst[i] = new AnimInstance;
				if (inst[i])
				{
					inst[i]->Init(arch->animArch);
					Vector pos = pos2;
					pos.z += 400*(SINGLE)rand()/RAND_MAX-200;
					pos.x += pos.z/10.0;
					inst[i]->SetPosition(pos);
					//animWidth = 1000*animScale*(timeToLive/totalTime);
					animWidth = 1000*animScale*(1-0.4*(spreadStage/spreadFactor));
					inst[i]->SetRotation(2*PI*(SINGLE)rand()/RAND_MAX);
					inst[i]->SetWidth(animWidth);
					inst[i]->loop = FALSE;
				}
			}
			spreadStage++;
		}
	}
	return ((timeToLive -= (float)ELAPSED_TIME) > 0);
}
//----------------------------------------------------------------------------------
//
void Fireball::Render (void)
{
	if (timeToLive < 0)
		return;
	TRANSFORM transform;
	
	BATCH->set_render_state(D3DRS_ZWRITEENABLE,FALSE);
	BATCH->set_render_state(D3DRS_ALPHABLENDENABLE,TRUE);
	BATCH->set_render_state(D3DRS_SRCBLEND,D3DBLEND_ONE);
	BATCH->set_render_state(D3DRS_DESTBLEND,D3DBLEND_ONE);
//	BATCH->set_state(RPR_BATCH_TRANSLUCENT_MODE, RPR_TRANSLUCENT_UNSORTED );
	if (owner)
	{
		TRANSFORM ownerTrans = owner->GetTransform();
		transform = ownerTrans.multiply(trans);
	}
	
	for (U8 i=0;i<MAX_ANIMS;i++)
	{
		if (inst[i])
		{

			ANIM2D->render(inst[i],&transform);
		}
	}
//	PIPE->set_pipeline_state(RP_BATCH_TRANSLUCENT_MODE, RP_TRANSLUCENT_UNSORTED );
}
//----------------------------------------------------------------------------------
//
BOOL32 Fireball::InitEffect ( IBaseObject *_owner,const class TRANSFORM & orientation,SINGLE _animScale,SINGLE lifeTime)
{
	BOOL32 result = 0;

	owner = _owner;

	//Vector pos = orientation.get_position();
	//pos1 = pos;
	//pos2 = pos;

	//pos1 = pos2 = Vector(0,0,0);
	
	animScale = _animScale;
/*	if (arch->animArch)
	{
		inst[0] = new AnimInstance;
		if (inst[0])
		{
			inst[0]->Init(arch->animArch);
		//	inst[0]->transform = orientation;
			animWidth = 1000*animScale;
			inst[0]->SetRotation(2*PI*(SINGLE)rand()/RAND_MAX);
			inst[0]->SetWidth(animWidth);
			inst[0]->loop = FALSE;
		}
	}
	
	spreadFactor = 1;*/
	//	transform = orientation;
	
//	objClass = OC_EFFECT;
	if ((timeToLive = totalTime = lifeTime) == 0)
		timeToLive = totalTime = 1;
	CQASSERT(totalTime);
	
	result = 1;

	return result;
}

//----------------------------------------------------------------------------------
//
BOOL32 Fireball::EditorInitEffect ( IBaseObject *_owner,SINGLE lifeTime)
{
	BOOL32 result = 0;

	owner = _owner;

	//Vector pos = orientation.get_position();
	//pos1 = pos;
	//pos2 = pos;

	pos1 = pos2 = Vector(0,0,0);
	
	animScale = 1;
	if (arch->animArch)
	{
		inst[0] = new AnimInstance;
		if (inst[0])
		{
			inst[0]->Init(arch->animArch);
		//	inst[0]->transform = orientation;
			animWidth = 1000*animScale;
			inst[0]->SetRotation(2*PI*(SINGLE)rand()/RAND_MAX);
			inst[0]->SetWidth(animWidth);
			inst[0]->loop = FALSE;
		}
	}
	
	spreadFactor = 1;
	//	transform = orientation;
	
	objClass = OC_EFFECT;
	if ((timeToLive = totalTime = lifeTime) == 0)
		timeToLive = totalTime = 1;
	CQASSERT(totalTime);
	
	result = 1;

	return result;

}

SINGLE Fireball::GetRadius()
{
	return max(grow1.magnitude()*spreadFactor,grow2.magnitude()*spreadFactor)+animWidth*1.414;
}

void Fireball::SetSpread(Vector _grow1,Vector _grow2,U8 _spreadFactor)
{
	grow1 = _grow1;
	grow2 = _grow2;
	spreadFactor = _spreadFactor;
}

//----------------------------------------------------------------------------------
//---------------------------------Fireball Factory------------------------------------
//----------------------------------------------------------------------------------
//----------------------------------------------------------------------------------


struct DACOM_NO_VTABLE FireballManager : public IObjectFactory
{
	//
	// incoming interface map
	//
  
	BEGIN_DACOM_MAP_INBOUND(FireballManager)
	DACOM_INTERFACE_ENTRY(IObjectFactory)
	END_DACOM_MAP()

//	struct FireballNode *explosionList;
	U32 factoryHandle;


	//child object info
	FireballArchetype *pArchetype;

	//FireballManager methods

	FireballManager (void) 
	{
	}

	~FireballManager();
	
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
// FireballManager methods

FireballManager::~FireballManager()
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
void FireballManager::init()
{
	COMPTR<IDAConnectionPoint> connection;


	if (OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
	{
		connection->Advise(GetBase(), &factoryHandle);
	}
}
//--------------------------------------------------------------------------
//
HANDLE FireballManager::CreateArchetype(const char *szArchname, OBJCLASS objClass, void *data)
{
	if (objClass == OC_EFFECT)
	{
		BT_FIREBALL_DATA *objData = (BT_FIREBALL_DATA *)data;
		if (objData->fxClass == FX_FIREBALL)
		{
			FireballArchetype *newguy = new FireballArchetype;
			newguy->name = szArchname;
		//	newguy->data = objData;
			
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
				}
			}
			return newguy;
		}
	}

	return 0;
}
//--------------------------------------------------------------------------
//
BOOL32 FireballManager::DestroyArchetype(HANDLE hArchetype)
{
	FireballArchetype *deadguy = (FireballArchetype *)hArchetype;

	delete deadguy;
	return 1;
}
//--------------------------------------------------------------------------
//
IBaseObject * FireballManager::CreateInstance(HANDLE hArchetype)
{
	FireballArchetype *pFireball = (FireballArchetype *)hArchetype;
	//BT_Fireball *objData = ((FireballArchetype *)hArchetype)->data;
	
	Fireball * obj = new ObjectImpl<Fireball>;
	obj->objClass = OC_EFFECT;

//	obj->data = objData;
	obj->arch = pFireball;
	return obj;
	
}
//--------------------------------------------------------------------------
//
void FireballManager::EditorCreateInstance(HANDLE hArchetype, const MISSION_INFO & info)
{
	// not supported
}
//----------------------------------------------------------------------------------------------
//
/*struct IBaseObject * __stdcall CreateFireball (PARCHETYPE pArchetype)
{
	IBaseObject *result=0;

	if (pArchetype == 0)
		goto Done;
	
	result = FireballMGR->CreateInstance(pArchetype->hArchetype);
 	if (result)
 	{
		OBJPTR<IFireball> Fireball;

		result->QueryInterface(IFireballID, Fireball);
		CQASSERT(Fireball!=0);
 	}

Done:

	return result;
}*/
//----------------------------------------------------------------------------------------------
//
struct _fball : GlobalComponent
{
	struct FireballManager *FireballMgr;

	virtual void Startup (void)
	{
		FireballMgr = new DAComponent<FireballManager>;
		AddToGlobalCleanupList((IDAComponent **) &FireballMgr);
	}

	virtual void Initialize (void)
	{
		FireballMgr->init();
	}
};

static _fball fball;
//----------------------------------------------------------------------------------
//----------------------------------------------------------------------------------

//---------------------------------------------------------------------------
//--------------------------End Fireball.cpp------------------------------------
//---------------------------------------------------------------------------
