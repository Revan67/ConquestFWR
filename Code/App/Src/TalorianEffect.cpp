//--------------------------------------------------------------------------//
//                                                                          //
//                             TalorianEffect.cpp                          //
//                                                                          //
//                  COPYRIGHT (C) 1997 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*

    $Author: Jasony $

    $Header: /Conquest/App/Src/TalorianEffect.cpp 6     10/27/00 11:47a Jasony $
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
#include "ITalorianEffect.h"
#include "Objlist.h"
#include "CQlight.h"
#include "Camera.h"
#include "Objmap.h"
#include "TManager.h"

#include <FileSys.h>

struct TalorianEffectArchetype
{
	INSTANCE_INDEX archIndex;
	IMeshArchetype * meshArch;
	BT_TALORIAN_EFFECT * pData;
	U32 textureID;
	
	TalorianEffectArchetype()
	{
		meshArch = NULL;
		archIndex = -1;
		textureID = 0;
	}

	~TalorianEffectArchetype()
	{
		if (textureID)
			TMANAGER->ReleaseTextureRef(textureID);
		textureID = 0;
	}
};

struct DUMMY_SAVESTRUCT			// required by template framework
{
	TRANS_SAVELOAD trans_SL;
};

#define MAX_ACTIVE_TIME 5.0

#define MAX_CIRCLE_SPOKES 32
#define SEGMENT_DELTA (5.0/MAX_CIRCLE_SPOKES) 

Vector TECircleOutVect[MAX_CIRCLE_SPOKES];
Vector TECircleInVect[MAX_CIRCLE_SPOKES];

struct _NO_VTABLE TalorianEffect : public ObjectTransform<ObjectFrame<IBaseObject,struct DUMMY_SAVESTRUCT,TalorianEffectArchetype> >, ITalorianEffect
{

	BEGIN_MAP_INBOUND(TalorianEffect)
	_INTERFACE_ENTRY(IBaseObject)
	_INTERFACE_ENTRY(ITalorianEffect)
	_INTERFACE_ENTRY(IPhysicalObject)
	END_MAP()

	U32 systemID;
	U32 playerID;
	bool bClosing;
	bool bActive;
	SINGLE time;
	SINGLE activeTime;

	U32 objMapSquare;
	U32 objMapSystemID;

	U32 textureID;

	TalorianEffect();

	~TalorianEffect();

	//IBaseObject

	virtual void Render();

	virtual U32 GetSystemID() const
	{
		return systemID;
	};

	BOOL32 Update ();

	virtual void CastVisibleArea();

	void PhysicalUpdate (SINGLE dt);

	void TestVisible (const USER_DEFAULTS & defaults, const U32 currentSystem, const U32 currentPlayer);

	//ITalorianEffect

	virtual void InitTalorianEffect(TRANSFORM & trans, U32 systemID, U32 _playerID);

	virtual void WarmUp();

	virtual void ShutDown();

	virtual bool IsActive();

	virtual void CloseUp();
	
};
//----------------------------------------------------------------------------------
//
TalorianEffect::TalorianEffect()
{
	bClosing = false;
}
//----------------------------------------------------------------------------------
//
TalorianEffect::~TalorianEffect()
{
	if (objMapSystemID)
		OBJMAP->RemoveObjectFromMap(this,objMapSystemID,objMapSquare);
}
//----------------------------------------------------------------------------------
//
void TalorianEffect::Render()
{
	time += OBJLIST->GetRealRenderTime();
	if(bVisible)
	{
		if(!activeTime)
		{	
			return;
		}
		U8 alpha = 255 * (activeTime/MAX_ACTIVE_TIME);

		BATCH->set_state(RPR_BATCH,FALSE);
		BATCH->set_state(RPR_STATE_ID,textureID);

		TRANSFORM trans;
		SINGLE scale = 5000;
		trans.scale(scale);
		trans.rotate_about_k(time*0.1);
		trans.set_position(transform.translation);

		CAMERA->SetModelView(&trans);
		SetupDiffuseBlend(textureID,FALSE);
		BATCH->set_render_state(D3DRS_ZENABLE,TRUE);
		BATCH->set_render_state(D3DRS_ZWRITEENABLE,FALSE);
		BATCH->set_render_state(D3DRS_ALPHABLENDENABLE,TRUE);
		BATCH->set_render_state(D3DRS_SRCBLEND,D3DBLEND_ONE);
		BATCH->set_render_state(D3DRS_DESTBLEND,D3DBLEND_ONE);

		U32 frame = F2LONG(time*4)%4;
		SINGLE highCord = frame*0.25;
		SINGLE lowCord = highCord+0.25;

		PB.Begin(PB_TRIANGLE_STRIP);
		PB.Color3ub(alpha,alpha,alpha);

		Vector point;
		for(U32 i = 0; i < MAX_CIRCLE_SPOKES; ++i)
		{
			point = TECircleOutVect[i];
			PB.TexCoord2f(i*SEGMENT_DELTA,highCord);
			PB.Vertex3f(point.x,point.y,point.z);					
			point = TECircleInVect[i];
			PB.TexCoord2f(i*SEGMENT_DELTA,lowCord);
			PB.Vertex3f(point.x,point.y,point.z);	
		}
		point = TECircleOutVect[0];
		PB.TexCoord2f(5.0,highCord);
		PB.Vertex3f(point.x,point.y,point.z);					
		point = TECircleInVect[0];
		PB.TexCoord2f(5.0,lowCord);
		PB.Vertex3f(point.x,point.y,point.z);	

		PB.End();
		BATCH->set_state(RPR_STATE_ID,0);
	}
}
//----------------------------------------------------------------------------------
//
void TalorianEffect::CastVisibleArea()
{
	const U32 mask = MGlobals::GetAllyMask(playerID);
	SetVisibleToAllies(mask);
}
//----------------------------------------------------------------------------------
//
BOOL32 TalorianEffect::Update ()
{
	return ((!bClosing) || (activeTime != 0) );
}
//----------------------------------------------------------------------------------
//
void TalorianEffect::PhysicalUpdate (SINGLE dt)
{
	if(bActive && !bClosing)
	{
		activeTime+= dt;
		if(activeTime > MAX_ACTIVE_TIME)
			activeTime = MAX_ACTIVE_TIME;
	}
	else
	{
		activeTime -= dt;
		if(activeTime < 0)
			activeTime = 0;
	}
}
//--------------------------------------------------------------------------//
// set bVisible if possible for any part of object to appear
//
void TalorianEffect::TestVisible (const USER_DEFAULTS & defaults, const U32 currentSystem, const U32 currentPlayer)
{
	IBaseObject::TestVisible(defaults,currentSystem,currentPlayer);
	if (bVisible)
	{
		SINGLE x,y,rad_2d,depth;
		bVisible = CAMERA->SphereInFrustrum(transform.translation,5500,x,y,rad_2d,depth);
	}
}
//----------------------------------------------------------------------------------
//
void TalorianEffect::InitTalorianEffect(TRANSFORM & trans, U32 _systemID, U32 _playerID)
{
	activeTime = 0;
	time = 0;
	transform = trans;
	systemID = _systemID;
	playerID = _playerID;
	bActive = false;
	bClosing = false;

	CQASSERT(objMapNode==0);
	objMapSquare = OBJMAP->GetMapSquare(systemID, trans.translation);
	objMapSystemID = systemID;
	objMapNode = OBJMAP->AddObjectToMap(this, objMapSystemID, objMapSquare, 0);
	CQASSERT(objMapNode);
}
//----------------------------------------------------------------------------------
//
void TalorianEffect::WarmUp()
{
	bActive = true;
}
//----------------------------------------------------------------------------------
//
void TalorianEffect::ShutDown()
{
	bActive = false;
}
//----------------------------------------------------------------------------------
//
bool TalorianEffect::IsActive()
{
	return bActive;
}
//----------------------------------------------------------------------------------
//
void TalorianEffect::CloseUp()
{
	bClosing = true;
}
//----------------------------------------------------------------------------------
//---------------------------------Blast Factory------------------------------------
//----------------------------------------------------------------------------------
//----------------------------------------------------------------------------------


struct DACOM_NO_VTABLE TalorianEffectManager : public IObjectFactory
{
	//
	// incoming interface map
	//
  
	BEGIN_DACOM_MAP_INBOUND(TalorianEffectManager)
	DACOM_INTERFACE_ENTRY(IObjectFactory)
	END_DACOM_MAP()

//	struct BlastNode *explosionList;
	U32 factoryHandle;

	//BlastManager methods

	TalorianEffectManager (void) 
	{
	}

	~TalorianEffectManager();
	
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
// TalorianEffectManager methods

TalorianEffectManager::~TalorianEffectManager()
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
void TalorianEffectManager::init()
{
	COMPTR<IDAConnectionPoint> connection;


	if (OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
	{
		connection->Advise(GetBase(), &factoryHandle);
	}
}
//--------------------------------------------------------------------------
//
HANDLE TalorianEffectManager::CreateArchetype(const char *szArchname, OBJCLASS objClass, void *data)
{
	if (objClass == OC_EFFECT)
	{
		BT_TALORIAN_EFFECT * objData = (BT_TALORIAN_EFFECT * )data;
		if (objData->fxClass == FX_TALORIAN_EFFECT)
		{
			for(U32 i = 0; i < MAX_CIRCLE_SPOKES; ++i)
			{
				TECircleOutVect[i] = Vector(cos(i*((2*PI)/MAX_CIRCLE_SPOKES)),sin(i*((2*PI)/MAX_CIRCLE_SPOKES)),0);
				TECircleInVect[i] = Vector(cos(i*((2*PI)/MAX_CIRCLE_SPOKES))*0.75,sin(i*((2*PI)/MAX_CIRCLE_SPOKES))*0.75,0);
			}

			TalorianEffectArchetype * handle = new TalorianEffectArchetype;
			handle->pData = objData;

			handle->textureID = TMANAGER->CreateTextureFromFile("talorianmatrix.tga", TEXTURESDIR, DA::TGA,PF_4CC_DAA4);

			return handle;
		}
	}

	return 0;
}
//--------------------------------------------------------------------------
//
BOOL32 TalorianEffectManager::DestroyArchetype(HANDLE hArchetype)
{
	TalorianEffectArchetype *deadguy = (TalorianEffectArchetype *)hArchetype;

	delete deadguy;
	return 1;
}
//--------------------------------------------------------------------------
//
IBaseObject * TalorianEffectManager::CreateInstance(HANDLE hArchetype)
{
	TalorianEffectArchetype *pBlast = (TalorianEffectArchetype *)hArchetype;

	TalorianEffect * obj = new ObjectImpl<TalorianEffect>;
	obj->objClass = OC_EFFECT;
	obj->textureID = pBlast->textureID;

	obj->FRAME_init(*pBlast);

	return obj;
}
//--------------------------------------------------------------------------
//
void TalorianEffectManager::EditorCreateInstance(HANDLE hArchetype, const MISSION_INFO & info)
{
	// not supported
}

//----------------------------------------------------------------------------------------------
//
struct _tebing : GlobalComponent
{
	struct TalorianEffectManager *blastMgr;

	virtual void Startup (void)
	{
		blastMgr = new DAComponent<TalorianEffectManager>;
		AddToGlobalCleanupList((IDAComponent **) &blastMgr);
	}

	virtual void Initialize (void)
	{
		blastMgr->init();
	}
};

static _tebing tebing;
//----------------------------------------------------------------------------------
//----------------------------------------------------------------------------------

//---------------------------------------------------------------------------
//--------------------------End TalorianEffect.cpp------------------------------------
//---------------------------------------------------------------------------
