//--------------------------------------------------------------------------//
//                                                                          //
//                             WormholeBlast.cpp                          //
//                                                                          //
//                  COPYRIGHT (C) 1997 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*

    $Author: Tmauer $

    $Header: /Conquest/App/Src/WormholeBlast.cpp 7     10/19/00 9:30a Tmauer $
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
#include "IWormholeBlast.h"
#include "Objlist.h"
#include "CQlight.h"
#include "Camera.h"
#include "Objmap.h"
#include "TManager.h"

#include <FileSys.h>

#define NUM_FLARES 4
#define FLAIR_CIRC_COUNT 45

static SINGLE flairCircleX[FLAIR_CIRC_COUNT];
static SINGLE flairCircleY[FLAIR_CIRC_COUNT];

struct WormholeBlastArchetype
{
	INSTANCE_INDEX archIndex;
	BT_WORMHOLE_EFFECT * pData;
	U32 textureID;
	IMeshArchetype * meshArch;
	
	WormholeBlastArchetype()
	{
		meshArch = NULL;
		archIndex = -1;
	}
};

struct DUMMY_SAVESTRUCT			// required by template framework
{
	TRANS_SAVELOAD trans_SL;
};

#define MAX_CLOUD_STRIPS 20
#define MIN_CS_TIME 1.0
#define MAX_CS_TIME 3.0
#define INTERVAL_CS_TIME (((MAX_CS_TIME+MIN_CS_TIME)/2)/MAX_CLOUD_STRIPS)
#define MAX_CS_ALTITUDE 500.0
#define MIN_CS_ALTITUDE -500.0
#define MAX_CS_SEGMENTS 24
#define MIN_CS_SEGMENTS 10
#define MAX_CS_SEG_DT 0.5
#define MIN_CS_SEG_DT 0.05

#define MAX_CLOUD_SPOKES 40
#define CLOUD_WIDTH 100

Vector WBCircleVect[MAX_CLOUD_SPOKES];

struct CloudStrip
{
	U32 startIndex;
	SINGLE time;
	SINGLE maxTime;
	U32 segments;
	SINGLE timeSegementDif;
	SINGLE altitude;
	bool bActive;
};

struct _NO_VTABLE WormholeBlast : public ObjectTransform<ObjectFrame<IBaseObject,struct DUMMY_SAVESTRUCT,WormholeBlastArchetype> >, IWormholeBlast
{

	BEGIN_MAP_INBOUND(WormholeBlast)
	_INTERFACE_ENTRY(IBaseObject)
	_INTERFACE_ENTRY(IWormholeBlast)
	_INTERFACE_ENTRY(IPhysicalObject)
	END_MAP()

	typedef INITINFO WORMBLASTINITINFO;

	SINGLE flashTime;
	U32 systemID;
	U32 playerID;
	SINGLE radius;
	bool bReverse;
	bool bClosing;

	U32 objMapSquare;
	U32 objMapSystemID;

	CloudStrip strips[MAX_CLOUD_STRIPS];
	U32 numStrips;
	SINGLE lastStripTime;
	U32 textureID;

	U32 scriptID;

	WormholeBlast();

	~WormholeBlast();

	//IBaseObject

	virtual void Render();

	virtual U32 GetSystemID() const
	{
		return systemID;
	};

	BOOL32 Update ();

	virtual void CastVisibleArea();

	void PhysicalUpdate (SINGLE dt);

	//IWormholeBlast
	
	virtual void InitWormholeBlast(TRANSFORM & trans, U32 systemID, SINGLE radius,U32 playerID, bool bReverse);

	virtual void Flash();

	virtual void CloseUp();

	virtual void SetScriptID(U32 scID)
	{
		scriptID = scID;
	};

	virtual U32 GetScriptID()
	{
		return scriptID;
	}

	void renderStrips();

};
//----------------------------------------------------------------------------------
//
WormholeBlast::WormholeBlast()
{
	bClosing = false;
	numStrips = 0;
	lastStripTime = 0;
	for(U32 i = 0; i < MAX_CLOUD_STRIPS; ++i)
	{
		strips[i].bActive = false;
	}
}
//----------------------------------------------------------------------------------
//
WormholeBlast::~WormholeBlast()
{
	if (objMapSystemID)
		OBJMAP->RemoveObjectFromMap(this,objMapSystemID,objMapSquare);
}
//----------------------------------------------------------------------------------
//
void WormholeBlast::renderStrips()
{
	if(!numStrips)
		return;

	BATCH->set_state(RPR_BATCH,TRUE);
	BATCH->set_state(RPR_STATE_ID,textureID);

	TRANSFORM trans;
	SINGLE scale = (radius)+1;
	trans.scale(scale);
	trans.set_position(transform.translation);

	CAMERA->SetModelView(&trans);
	SetupDiffuseBlend(textureID,FALSE);
	BATCH->set_state(RPR_STATE_ID,textureID);
	BATCH->set_render_state(D3DRS_ZENABLE,TRUE);
	BATCH->set_render_state(D3DRS_ZWRITEENABLE,FALSE);
	BATCH->set_render_state(D3DRS_ALPHABLENDENABLE,TRUE);
	BATCH->set_render_state(D3DRS_SRCBLEND,D3DBLEND_SRCALPHA);
	BATCH->set_render_state(D3DRS_DESTBLEND,D3DBLEND_INVSRCALPHA);

	PB.Begin(PB_QUADS);
	for(U32 i = 0; i < MAX_CLOUD_STRIPS; ++i)
	{
		if(strips[i].bActive)
		{
			SINGLE t = 1.0-(strips[i].time/strips[i].maxTime);
			if(bReverse)
				t= 1.0-t;
			Vector spoke = WBCircleVect[strips[i].startIndex];
			Vector lastPoint = spoke*t + Vector(0,0,strips[i].altitude);
			SINGLE width = (0.01)/((radius)+1);
			U8 lastAlpha;
			if(t < 0 || t > 1)
				lastAlpha = 0;
			else if(t < 0.4)
				lastAlpha = 255*(t/0.4);
			else if(t > 0.6)
				lastAlpha = 255*((1.0-t)/0.4);
			else
				lastAlpha = 255;
			Vector lastPoint1 = lastPoint + (spoke*(width*t));
			Vector lastPoint2 = lastPoint - (spoke*(width*t));
			for(U32 seg = 1; seg <= strips[i].segments; ++seg)
			{
				t = 1.0-((strips[i].time-(strips[i].timeSegementDif*seg))/strips[i].maxTime);
				if(bReverse)
					t= 1.0-t;
				spoke =  WBCircleVect[(strips[i].startIndex+seg)%MAX_CLOUD_SPOKES];
				Vector nextPoint = spoke*t+ Vector(0,0,strips[i].altitude);
				if(seg > (strips[i].segments >> 1))
				{
					width = (strips[i].segments -seg);
					width = (width*width*CLOUD_WIDTH+0.01)/((radius)+1);
				}
				else
				{
					width = (seg*seg*CLOUD_WIDTH+0.01)/((radius)+1);
				}

				U8 nextAlpha;
				if(t < 0 || t > 1)
					nextAlpha = 0;
				else if(t < 0.4)
					nextAlpha = 255*(t/0.4);
				else if(t > 0.6)
					nextAlpha = 255*((1.0-t)/0.4);
				else
					nextAlpha = 255;
				Vector dir = lastPoint-nextPoint;
				dir.normalize();
				dir = cross_product(dir,Vector(0,0,1));
				Vector nextPoint1 = nextPoint + dir*width*t;
				Vector nextPoint2 = nextPoint - dir*width*t;
				
				if(lastAlpha || nextAlpha)
				{
					PB.Color4ub(lastAlpha,lastAlpha,lastAlpha,lastAlpha);
					PB.TexCoord2f((seg-1)*0.3,1.0);
					PB.Vertex3f(lastPoint2.x,lastPoint2.y,lastPoint2.z);					
					PB.TexCoord2f((seg-1)*0.3,0.5);
					PB.Vertex3f(lastPoint.x,lastPoint.y,lastPoint.z);

					PB.Color4ub(nextAlpha,nextAlpha,nextAlpha,nextAlpha);
					PB.TexCoord2f(seg*0.3,0.5);
					PB.Vertex3f(nextPoint.x,nextPoint.y,nextPoint.z);
					PB.TexCoord2f(seg*0.3,1.0);
					PB.Vertex3f(nextPoint2.x,nextPoint2.y,nextPoint2.z);

					PB.Color4ub(lastAlpha,lastAlpha,lastAlpha,lastAlpha);
					PB.TexCoord2f((seg-1)*0.3,0);
					PB.Vertex3f(lastPoint1.x,lastPoint1.y,lastPoint1.z);					
					PB.TexCoord2f((seg-1)*0.3,0.5);
					PB.Vertex3f(lastPoint.x,lastPoint.y,lastPoint.z);

					PB.Color4ub(nextAlpha,nextAlpha,nextAlpha,nextAlpha);
					PB.TexCoord2f(seg*0.3,0.5);
					PB.Vertex3f(nextPoint.x,nextPoint.y,nextPoint.z);
					PB.TexCoord2f(seg*0.3,0);
					PB.Vertex3f(nextPoint1.x,nextPoint1.y,nextPoint1.z);
				}

				lastPoint1 = nextPoint1;
				lastPoint2 = nextPoint2;
				lastAlpha = nextAlpha;
				lastPoint = nextPoint;
			}
		}
	}
	PB.End();
	BATCH->set_state(RPR_STATE_ID,0);
}
//----------------------------------------------------------------------------------
//
void WormholeBlast::Render()
{
	if(bVisible)
	{

		renderStrips();

		if(flashTime <= 0)
			return;

		Vector pos = transform.translation;
			
		Vector cpos (CAMERA->GetPosition());
			
		Vector look (pos - cpos);
			
		Vector k = look.normalize();

		Vector tmpUp(pos.x,pos.y,pos.z+50000);

		Vector j (cross_product(k,tmpUp));
		if(j.magnitude())
		{
			j.normalize();

			Vector i (cross_product(j,k));

			i.normalize();

			TRANSFORM trans;
			trans.set_i(i);
			trans.set_j(j);
			trans.set_k(k);

			i = trans.get_i();
			j = trans.get_j();
			k = trans.get_k();




			BATCH->set_state(RPR_BATCH,false);
			DisableTextures();
			CAMERA->SetModelView();
			BATCH->set_state(RPR_STATE_ID,0);
			BATCH->set_render_state(D3DRS_ZENABLE,TRUE);
			BATCH->set_render_state(D3DRS_ZWRITEENABLE,FALSE);
			BATCH->set_render_state(D3DRS_ALPHABLENDENABLE,TRUE);
			BATCH->set_render_state(D3DRS_SRCBLEND,D3DBLEND_ONE);
			BATCH->set_render_state(D3DRS_DESTBLEND,D3DBLEND_ONE);
			BATCH->set_render_state(D3DRS_DITHERENABLE,TRUE);

			U8 alpha = 100*flashTime;
			PB.Begin(PB_TRIANGLES);
			for(U32 count = 0; count < NUM_FLARES; ++count)
			{	
				PB.Color3ub(alpha,alpha,alpha);
				PB.Vertex3f(pos.x,pos.y,pos.z);
				PB.Color3ub(0,0,0);
				U32 r1 = rand()%FLAIR_CIRC_COUNT;
				Vector final = pos+i*flairCircleX[r1]+j*flairCircleY[r1];
				PB.Vertex3f(final.x,final.y,final.z);
				r1 = (r1+(rand()%3)+1)%FLAIR_CIRC_COUNT;
				final = pos+i*flairCircleX[r1]+j*flairCircleY[r1];
				PB.Vertex3f(final.x,final.y,final.z);
			}
			PB.End();
		}
	}
}
//----------------------------------------------------------------------------------
//
void WormholeBlast::CastVisibleArea()
{
	const U32 mask = MGlobals::GetAllyMask(playerID);
	SetVisibleToAllies(mask);
}
//----------------------------------------------------------------------------------
//
void WormholeBlast::InitWormholeBlast(TRANSFORM & trans, U32 _systemID, SINGLE _radius,U32 _playerID, bool _bReverse)
{
	flashTime = 0;
	systemID = _systemID;
	transform = trans;
	radius = _radius;
	bReverse = _bReverse;
	playerID = _playerID;

	CQASSERT(objMapNode==0);
	objMapSquare = OBJMAP->GetMapSquare(systemID, trans.translation);
	objMapSystemID = systemID;
	objMapNode = OBJMAP->AddObjectToMap(this, objMapSystemID, objMapSquare, 0);
	CQASSERT(objMapNode);
}
//----------------------------------------------------------------------------------
//
void WormholeBlast::Flash()
{
	flashTime = 1.0;
}
//----------------------------------------------------------------------------------
//
void WormholeBlast::CloseUp()
{
	bClosing = true;
}
//----------------------------------------------------------------------------------
//
BOOL32 WormholeBlast::Update ()
{
	lastStripTime += ELAPSED_TIME;
	if(lastStripTime > INTERVAL_CS_TIME && numStrips < MAX_CLOUD_STRIPS && !bClosing)
	{
		lastStripTime = 0;
		for(U32 i = 0; i < MAX_CLOUD_STRIPS; ++i)
		{
			if(!strips[i].bActive)
			{
				strips[i].altitude = ((MAX_CS_ALTITUDE-MIN_CS_ALTITUDE)*((rand()%1000)/1000.0)+MIN_CS_ALTITUDE)/(radius);
				strips[i].maxTime = (MAX_CS_TIME-MIN_CS_TIME)*((rand()%1000)/1000.0)+MIN_CS_TIME;
				strips[i].segments = (MAX_CS_SEGMENTS-MIN_CS_SEGMENTS)*((rand()%1000)/1000.0)+MIN_CS_SEGMENTS;
				strips[i].startIndex = (MAX_CLOUD_SPOKES)*((rand()%1000)/1000.0);
				strips[i].timeSegementDif = (MAX_CS_SEG_DT-MIN_CS_SEG_DT)*((rand()%1000)/1000.0)+MIN_CS_SEG_DT;
				strips[i].bActive = true;
				strips[i].time = 0;
				++numStrips;
				break;
			}
		}
	}
	return ((!bClosing) || flashTime >0 || numStrips);
}
//----------------------------------------------------------------------------------
//
void WormholeBlast::PhysicalUpdate (SINGLE dt)
{
	for(U32 i = 0; i < MAX_CLOUD_STRIPS; ++i)
	{
		if(strips[i].bActive)
		{
			strips[i].time+= dt;
			if(strips[i].time-(strips[i].timeSegementDif*strips[i].segments) > strips[i].maxTime)
			{
				strips[i].bActive = false;
				--numStrips;
			}
		}
	}
	if(flashTime != 0)
	{
		flashTime -= dt;
		if(flashTime < 0)
			flashTime = 0;
	}
}
//----------------------------------------------------------------------------------
//---------------------------------Blast Factory------------------------------------
//----------------------------------------------------------------------------------
//----------------------------------------------------------------------------------


struct DACOM_NO_VTABLE WormholeBlastManager : public IObjectFactory
{
	//
	// incoming interface map
	//
  
	BEGIN_DACOM_MAP_INBOUND(WormholeBlastManager)
	DACOM_INTERFACE_ENTRY(IObjectFactory)
	END_DACOM_MAP()

//	struct BlastNode *explosionList;
	U32 factoryHandle;

	//BlastManager methods

	WormholeBlastManager (void) 
	{
	}

	~WormholeBlastManager();
	
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
// WormholeBlastManager methods

WormholeBlastManager::~WormholeBlastManager()
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
void WormholeBlastManager::init()
{
	COMPTR<IDAConnectionPoint> connection;


	if (OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
	{
		connection->Advise(GetBase(), &factoryHandle);
	}
}
//--------------------------------------------------------------------------
//
HANDLE WormholeBlastManager::CreateArchetype(const char *szArchname, OBJCLASS objClass, void *data)
{
	if (objClass == OC_EFFECT)
	{
		BT_WORMHOLE_EFFECT * objData = (BT_WORMHOLE_EFFECT * )data;
		if (objData->fxClass == FX_WORMHOLE_BLAST)
		{
			U32 i;
			for(i = 0; i < MAX_CLOUD_SPOKES; ++i)
			{
				WBCircleVect[i] = Vector(cos(i*((2*PI)/MAX_CLOUD_SPOKES)),sin(i*((2*PI)/MAX_CLOUD_SPOKES)),0);
			}
			for(i = 0; i < FLAIR_CIRC_COUNT; ++i)
			{
				flairCircleX[i] = cos(i*((PI*2)/FLAIR_CIRC_COUNT))*3*GRIDSIZE;
				flairCircleY[i] = sin(i*((PI*2)/FLAIR_CIRC_COUNT))*3*GRIDSIZE;
			}

			WormholeBlastArchetype * handle = new WormholeBlastArchetype;
			handle->pData = objData;

			handle->textureID = TMANAGER->CreateTextureFromFile("solarianworm_64.tga", TEXTURESDIR, DA::TGA,PF_4CC_DAA4);

			return handle;
		}
	}

	return 0;
}
//--------------------------------------------------------------------------
//
BOOL32 WormholeBlastManager::DestroyArchetype(HANDLE hArchetype)
{
	WormholeBlastArchetype *deadguy = (WormholeBlastArchetype *)hArchetype;

	delete deadguy;
	return 1;
}
//--------------------------------------------------------------------------
//
IBaseObject * WormholeBlastManager::CreateInstance(HANDLE hArchetype)
{
	WormholeBlastArchetype *pBlast = (WormholeBlastArchetype *)hArchetype;

	WormholeBlast * obj = new ObjectImpl<WormholeBlast>;
	obj->objClass = OC_EFFECT;
	obj->textureID = pBlast->textureID;

	obj->FRAME_init(*pBlast);

	return obj;
}
//--------------------------------------------------------------------------
//
void WormholeBlastManager::EditorCreateInstance(HANDLE hArchetype, const MISSION_INFO & info)
{
	// not supported
}

//----------------------------------------------------------------------------------------------
//
struct _wbing : GlobalComponent
{
	struct WormholeBlastManager *blastMgr;

	virtual void Startup (void)
	{
		blastMgr = new DAComponent<WormholeBlastManager>;
		AddToGlobalCleanupList((IDAComponent **) &blastMgr);
	}

	virtual void Initialize (void)
	{
		blastMgr->init();
	}
};

static _wbing wbing;
//----------------------------------------------------------------------------------
//----------------------------------------------------------------------------------

//---------------------------------------------------------------------------
//--------------------------End WormholeBlast.cpp------------------------------------
//---------------------------------------------------------------------------
