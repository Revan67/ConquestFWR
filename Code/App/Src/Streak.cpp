//--------------------------------------------------------------------------//
//                                                                          //
//                                 Streak.cpp                                //
//                                                                          //
//                  COPYRIGHT (C) 1997 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Author: Jasony $
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
#include <RendPipeline.h>


struct StreakArchetype
{
	const char *name;
	BT_STREAK_DATA *data;
	ARCHETYPE_INDEX archIndex;
	AnimArchetype *animArch;

    void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}

	StreakArchetype (void)
	{
		archIndex = -1;
	}

	~StreakArchetype (void)
	{
		delete animArch;
		ENGINE->release_archetype(archIndex);
	}

};

#define MAX_LINES 8

struct DUMMY_SAVESTRUCT			// required by template framework
{
	TRANS_SAVELOAD trans_SL;
};
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
struct Line
{
	Vector dir;
	SINGLE length;
	SINGLE maxLength;
	SINGLE lineTime,time;
};

struct _NO_VTABLE Streak : public IBaseObject, IEffect//,IStreak
{

	BEGIN_MAP_INBOUND(Streak)
	_INTERFACE_ENTRY(IBaseObject)
//	_INTERFACE_ENTRY(IStreak)
	_INTERFACE_ENTRY(IEffect)
	END_MAP()

	//------------------------------------------

	SINGLE timeToLive,totalTime;
	StreakArchetype *arch;
	SINGLE animWidth;
	SINGLE animScale;
	U32 cnt;
	IBaseObject *owner;
	TRANSFORM trans;
	Line line[MAX_LINES];
	U8 numLines,maxLines;
	U8 tempCnt;

	//------------------------------------------

	Streak (void)// : point(0)
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

	virtual ~Streak (void);	// See ObjList.cpp

	/* IBaseObject methods */

	virtual BOOL32 Update (void);

	virtual void Render (void);

	//IEffect
	
	virtual BOOL32 InitEffect ( IBaseObject *_owner,const class TRANSFORM & orientation,SINGLE _animScale,SINGLE lifeTime);
	
	virtual BOOL32 EditorInitEffect ( IBaseObject *_owner,SINGLE lifeTime);

	virtual SINGLE GetRadius();

};

//----------------------------------------------------------------------------------
//
Streak::~Streak (void)
{

}
//----------------------------------------------------------------------------------
//
BOOL32 Streak::Update (void)
{
	CQASSERT(totalTime);

//	CQASSERT(arch->animArch);

	int i;
	for (i=0;i<numLines;i++)
	{
		line[i].length += 0.2*(line[i].maxLength-line[i].length);
		line[i].time -= ELAPSED_TIME;
		/*if (line[i].time < 0)
		{
			line[i].length = 50;
			line[i].maxLength = 400+2.0*(rand()%100);
			line[i].time = line[i].lineTime = 0.5*arch->data->lineTime;
		}*/
	}
	if (numLines < maxLines && rand()%50==0)
	{
		line[numLines].length = 50;
		line[numLines].maxLength = 800+2.0*(rand()%100);
		line[numLines].time = line[i].lineTime = arch->data->lineTime;
		numLines++;
	}

	
	tempCnt = (tempCnt+1)%20;
	
	return ((timeToLive -= (float)ELAPSED_TIME) > 0);
}
//----------------------------------------------------------------------------------
//
void Streak::Render (void)
{
		TRANSFORM transform;
		bHighlight = 0;
		
		PIPE->set_render_state(D3DRS_ZWRITEENABLE,FALSE);
//		PIPE->set_texture_stage_texture( 0,arch->animArch->frames[0].texture);
		SetupDiffuseBlend(arch->animArch->frames[0].texture,TRUE);
		PIPE->set_render_state(D3DRS_ALPHABLENDENABLE,TRUE);
		PIPE->set_render_state(D3DRS_SRCBLEND,D3DBLEND_SRCALPHA);
		PIPE->set_render_state(D3DRS_DESTBLEND,D3DBLEND_INVSRCALPHA);
		if (owner)
		{
		//	TRANSFORM ownerTrans = owner->GetTransform();
		//	transform = ownerTrans.multiply(trans);
			transform = owner->GetTransform();
		}

		Vector look = -CAMERA->GetTransform()->get_k();
		
		CAMERA->SetModelView();


		
		PB.Begin(PB_QUADS);
		for (int i=0;i<numLines;i++)
		{
			Vector dir = transform.rotate(line[i].dir);
			
		//	Vector test = CAMERA->GetPosition()-transform.translation;
		//	test.normalize();
			Vector n = cross_product(look,dir);
			n.normalize();
			Vector v0,v1,v2,v3;

		
			AnimFrame *frame = &arch->animArch->frames[tempCnt/5];

			SINGLE l=0;

	#define STR_SIZE 200
			
			Vector p1,p2 = dir*STR_SIZE;
			
			while (l<line[i].length)
			{
				if (line[i].time > 0)
				{
					
					SINGLE factor = 1.0;
					if (line[i].time < 0.4*line[i].lineTime)
					{
						factor = 2.5*line[i].time/line[i].lineTime;
						if (factor < 0)
							factor = 0;
					}
					factor *= (line[i].length-l)/line[i].length;
					PB.Color4ub(255,255,255,255*factor);
					
					p1 = p2;//dir * l;
					p2 = dir * (l+STR_SIZE);
					v0 = transform.translation+p1-70*n;
					v1 = transform.translation+p1+70*n;
					v2 = transform.translation+p2+70*n;
					v3 = transform.translation+p2-70*n;
					
					
					PB.TexCoord2f(frame->x0,frame->y0);					PB.Vertex3f(v0.x,v0.y,v0.z);		
					PB.TexCoord2f(frame->x1,frame->y0);					PB.Vertex3f(v1.x,v1.y,v1.z);
					PB.TexCoord2f(frame->x1,frame->y1);					PB.Vertex3f(v2.x,v2.y,v2.z);
					PB.TexCoord2f(frame->x0,frame->y1);					PB.Vertex3f(v3.x,v3.y,v3.z);
				}
				l += STR_SIZE;
			}

		}
		PB.End();
}
//----------------------------------------------------------------------------------
//
BOOL32 Streak::InitEffect ( IBaseObject *_owner,const class TRANSFORM & orientation,SINGLE _animScale,SINGLE lifeTime)
{
	BOOL32 result = 0;

	owner = _owner;

//	objClass = OC_EFFECT;
	if ((timeToLive = totalTime = 0.9*lifeTime) == 0)
		timeToLive = totalTime = 1;
	CQASSERT(totalTime);

/*	for (int i=0;i<maxLines;i++)
	{
		line[i].dir.set(rand()%100-50,rand()%100-50,rand()%100-50);
		if (line[i].dir.x==0 && line[i].dir.y==0 && line[i].dir.z==0)
			line[i].dir.x=1;
		line[i].dir.normalize();
//		line[i].length = 50;
//		line[i].maxLength = 800+2.0*(rand()%100);
//		line[i].time = line[i].lineTime = arch->data->lineTime;
	}*/
	
	result = 1;

	return result;

}

//----------------------------------------------------------------------------------
//
BOOL32 Streak::EditorInitEffect ( IBaseObject *_owner,SINGLE lifeTime)
{
	BOOL32 result = 0;

	owner = _owner;

	objClass = OC_EFFECT;
	if ((timeToLive = totalTime = 0.9*lifeTime) == 0)
		timeToLive = totalTime = 1;
	CQASSERT(totalTime);

	for (int i=0;i<maxLines;i++)
	{
		line[i].dir.set(rand()%100-50,rand()%100-50,rand()%100-50);
		if (line[i].dir.x==0 && line[i].dir.y==0 && line[i].dir.z==0)
			line[i].dir.x=1;
		line[i].dir.normalize();
	}
	
	result = 1;

	return result;

}
//----------------------------------------------------------------------------------
//
SINGLE Streak::GetRadius()
{
	return 1000;
}
//----------------------------------------------------------------------------------
//---------------------------------Streak Factory------------------------------------
//----------------------------------------------------------------------------------
//----------------------------------------------------------------------------------


struct DACOM_NO_VTABLE StreakManager : public IObjectFactory
{
	//
	// incoming interface map
	//
  
	BEGIN_DACOM_MAP_INBOUND(StreakManager)
	DACOM_INTERFACE_ENTRY(IObjectFactory)
	END_DACOM_MAP()

//	struct StreakNode *explosionList;
	U32 factoryHandle;


	//child object info
	StreakArchetype *pArchetype;

	//StreakManager methods

	StreakManager (void) 
	{
	}

	~StreakManager();
	
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
// StreakManager methods

StreakManager::~StreakManager()
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
void StreakManager::init()
{
	COMPTR<IDAConnectionPoint> connection;


	if (OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
	{
		connection->Advise(GetBase(), &factoryHandle);
	}
}
//--------------------------------------------------------------------------
//
HANDLE StreakManager::CreateArchetype(const char *szArchname, OBJCLASS objClass, void *data)
{
	if (objClass == OC_EFFECT)
	{
		BT_STREAK_DATA *objData = (BT_STREAK_DATA *)data;
		if (objData->fxClass == FX_STREAK)
		{
			StreakArchetype *newguy = new StreakArchetype;
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
				}
			}
			return newguy;
		}
	}

	return 0;
}
//--------------------------------------------------------------------------
//
BOOL32 StreakManager::DestroyArchetype(HANDLE hArchetype)
{
	StreakArchetype *deadguy = (StreakArchetype *)hArchetype;

	delete deadguy;
	return 1;
}
//--------------------------------------------------------------------------
//
IBaseObject * StreakManager::CreateInstance(HANDLE hArchetype)
{
	StreakArchetype *pStreak = (StreakArchetype *)hArchetype;
	BT_STREAK_DATA *objData = ((StreakArchetype *)hArchetype)->data;
	
	Streak * obj = new ObjectImpl<Streak>;
	obj->objClass = OC_EFFECT;

//	obj->data = objData;
	obj->arch = pStreak;
	CQASSERT(obj->numLines < MAX_LINES);
	obj->maxLines = objData->numLines;
	return obj;
	
}
//--------------------------------------------------------------------------
//
void StreakManager::EditorCreateInstance(HANDLE hArchetype, const MISSION_INFO & info)
{
	// not supported
}
//----------------------------------------------------------------------------------------------
//
/*struct IBaseObject * __stdcall CreateStreak (PARCHETYPE pArchetype)
{
	IBaseObject *result=0;

	if (pArchetype == 0)
		goto Done;
	
	result = StreakMGR->CreateInstance(pArchetype->hArchetype);
 	if (result)
 	{
		OBJPTR<IStreak> Streak;

		result->QueryInterface(IStreakID, Streak);
		CQASSERT(Streak!=0);
 	}

Done:

	return result;
}*/
//----------------------------------------------------------------------------------------------
//
struct _streak : GlobalComponent
{
	struct StreakManager *StreakMgr;

	virtual void Startup (void)
	{
		StreakMgr = new DAComponent<StreakManager>;
		AddToGlobalCleanupList((IDAComponent **) &StreakMgr);
	}

	virtual void Initialize (void)
	{
		StreakMgr->init();
	}
};

static _streak streak;
//----------------------------------------------------------------------------------
//----------------------------------------------------------------------------------

//---------------------------------------------------------------------------
//--------------------------End Streak.cpp------------------------------------
//---------------------------------------------------------------------------
