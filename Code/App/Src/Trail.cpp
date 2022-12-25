//--------------------------------------------------------------------------//
//                                                                          //
//                                 Trail.cpp                                //
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
#include "DExplosion.h"
#include "Objlist.h"
#include "Anim2D.h"
#include "TObjTrans.h"
#include "TobjFrame.h"
#include "Startup.h"
#include "DTrail.h"

#include <FileSys.h>
#include <Engine.h>
#include <RendPipeline.h>
//#include <RPUL\PrimitiveBuilder.h>

struct TrailArchetype
{
	const char *name;
	BT_TRAIL_DATA *data;
	AnimArchetype *animArch;

    void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}

	TrailArchetype (void)
	{
	}

	~TrailArchetype (void)
	{
		delete animArch;
	}

};

#define MAX_ANIMS 8
#define ANIM_LENGTH 2000
#define FADE_RATE 24000

struct DUMMY_SAVESTRUCT			// required by template framework
{
	TRANS_SAVELOAD trans_SL;
};
//--------------------------------------------------------------------------//
//
struct TRAIL_INIT
{
	S32 archIndex;

	TRAIL_INIT (S32 _archIndex)
	{
		archIndex = _archIndex;
	}
};
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//

struct _NO_VTABLE Trail : public ObjectFrame<IBaseObject,struct DUMMY_SAVESTRUCT,struct TRAIL_INIT> //, ITrail
{

	BEGIN_MAP_INBOUND(Trail)
	_INTERFACE_ENTRY(IBaseObject)
//	_INTERFACE_ENTRY(ITrail)
	END_MAP()

	//------------------------------------------

	SINGLE timeToLive,lightTime,totalTime,lightTotalTime;
	TrailArchetype *arch;
//	AnimInstance *inst[MAX_ANIMS];
	U32 systemID, playerID;
	Vector source,target;
	OBJPTR<IBaseObject> jumper;
	U8 numSections;
	SINGLE sectionLength,totalLength;
	SINGLE extraLength,length;
	Vector dir,perp;
	BOOL32 bFading:1;
	U32 cnt;

	//------------------------------------------

	Trail (void)
	{
		timeToLive = 2;
	}

    void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}

	virtual ~Trail (void);	// See ObjList.cpp

	/* IBaseObject methods */

	virtual BOOL32 Update (void);

	virtual void PhysicalUpdate(SINGLE dt);

	virtual void Render (void);

	virtual U32 GetSystemID (void) const
	{
		return systemID;
	}

	virtual U32 GetPlayerID (void) const
	{
		return playerID;
	}

	virtual void TestVisible (const USER_DEFAULTS & defaults, const U32 currentSystem, const U32 currentPlayer);

	BOOL32 InitTrail (IBaseObject *_jumper,Vector _target, U32 _systemID);

};

//----------------------------------------------------------------------------------
//
Trail::~Trail (void)
{
	for (U32 i=0;i<MAX_ANIMS;i++)
	{
//		if (inst[i])
		//	delete inst[i];
	}
}
//----------------------------------------------------------------------------------
//
void Trail::PhysicalUpdate(SINGLE dt)
{
	if (bFading)
	{
		length -= FADE_RATE*dt;
	//	CQASSERT(length >= 0);
		source = target-dir*length;
	}
	else
	{
		if (jumper && jumper->GetSystemID() == systemID)
		{
			length = (jumper->GetPosition()-source).magnitude();
		//	CQASSERT(length >= 0);
			if (length > totalLength*0.95)
			{
				length = totalLength;
				bFading = TRUE;
			}
		}
		else
		{
			bFading = TRUE;
			length = totalLength;
		}
	}
	
	numSections = length/ANIM_LENGTH;
	extraLength = length-numSections*ANIM_LENGTH;
}

BOOL32 Trail::Update (void)
{
//	CQASSERT(totalTime);
	cnt = (cnt+1)%8;

	return (length >= 0);
}
//----------------------------------------------------------------------------------
//
void Trail::Render (void)
{
	if (bVisible && length > 0)
	{
		BATCH->set_state(RPR_BATCH,TRUE);
		BATCH->set_render_state(D3DRS_ALPHABLENDENABLE,TRUE);
		BATCH->set_render_state(D3DRS_ZENABLE,TRUE);
		BATCH->set_render_state(D3DRS_ZWRITEENABLE,FALSE);  ///?????
		BATCH->set_render_state(D3DRS_CULLMODE,D3DCULL_NONE);
		BATCH->set_render_state(D3DRS_SRCBLEND,D3DBLEND_SRCALPHA);
		BATCH->set_render_state(D3DRS_DESTBLEND,D3DBLEND_INVSRCALPHA);
		U8 i;
		AnimFrame frame = arch->animArch->frames[cnt/2];
		CAMERA->SetModelView();

		//PIPE->set_texture_stage_texture( 0,frame.texture);
		SetupDiffuseBlend(frame.texture,TRUE);

		BATCH->set_state(RPR_STATE_ID,frame.texture);

		PB.Begin(PB_QUADS);
		for (i=0;i<numSections;i++)
		{
			Vector v0,v1,v2,v3;
			v0 = source-perp*200+i*dir*ANIM_LENGTH;
			v1 = source+perp*200+i*dir*ANIM_LENGTH;
			v2 = source+perp*200+(i+1)*dir*ANIM_LENGTH;
			v3 = source-perp*200+(i+1)*dir*ANIM_LENGTH;
			if (i==0)
				PB.Color4ub(255,255,255,0);
			PB.TexCoord2f(frame.x0,frame.y0);		PB.Vertex3f(v0.x,v0.y,v0.z);
			PB.TexCoord2f(frame.x0,frame.y1);		PB.Vertex3f(v1.x,v1.y,v1.z);
				PB.Color4ub(255,255,255,255);
			PB.TexCoord2f(frame.x1,frame.y1);		PB.Vertex3f(v2.x,v2.y,v2.z);
			PB.TexCoord2f(frame.x1,frame.y0);		PB.Vertex3f(v3.x,v3.y,v3.z);
		}

		if (extraLength)
		{
			Vector v0,v1,v2,v3;
			v0 = source-perp*200+(numSections*ANIM_LENGTH)*dir;
			v1 = source+perp*200+(numSections*ANIM_LENGTH)*dir;
			v2 = source+perp*200+(numSections*ANIM_LENGTH+extraLength)*dir;
			v3 = source-perp*200+(numSections*ANIM_LENGTH+extraLength)*dir;
			if (numSections==0)
				PB.Color4ub(255,255,255,0);
			
			PB.TexCoord2f(frame.x0,frame.y0);		PB.Vertex3f(v0.x,v0.y,v0.z);
			PB.TexCoord2f(frame.x0,frame.y1);		PB.Vertex3f(v1.x,v1.y,v1.z);

				PB.Color4ub(255,255,255,255);
			PB.TexCoord2f(frame.x0+(frame.x1-frame.x0)*extraLength/ANIM_LENGTH,frame.y1);		PB.Vertex3f(v2.x,v2.y,v2.z);
			PB.TexCoord2f(frame.x0+(frame.x1-frame.x0)*extraLength/ANIM_LENGTH,frame.y0);		PB.Vertex3f(v3.x,v3.y,v3.z);
		}

		PB.End();
		BATCH->set_state(RPR_STATE_ID,0);
	}
}

//--------------------------------------------------------------------------//
// set bVisible if possible for any part of object to appear
//
void Trail::TestVisible (const USER_DEFAULTS & defaults, const U32 currentSystem, const U32 currentPlayer)
{
	IBaseObject::TestVisible(defaults,currentSystem,currentPlayer);
	if (bVisible)
	{
		SINGLE x,y,rad_2d,depth;
		bVisible = CAMERA->SphereInFrustrum(source+dir*(length/2),length/2,x,y,rad_2d,depth);
	}
}
//----------------------------------------------------------------------------------
//
BOOL32 Trail::InitTrail (IBaseObject *_jumper,Vector _target, U32 _systemID)
{
	BOOL32 result = 0;

	systemID = _systemID;
	playerID = _jumper->GetPlayerID();

	if (_jumper)
		_jumper->QueryInterface(IBaseObjectID, jumper, SYSVOLATILEPTR);
	else
		jumper = 0;

	source = jumper->GetPosition();
	target = _target;

	SetVisibilityFlags(jumper->GetVisibilityFlags());

	totalLength = (target-source).magnitude();
//	numSections = totalLength/ANIM_LENGTH;

	dir = target-source;
	if (dir.x || dir.y || dir.z)
		dir.normalize();
	else
		dir.set(1,0,0);
	perp = cross_product(dir,Vector(0,0,1));
	CQASSERT(perp.magnitude()-1.0 < 1e-3);
//	sectionLength = totalLength/numSections;

	return result;

}
//----------------------------------------------------------------------------------
//---------------------------------Trail Factory------------------------------------
//----------------------------------------------------------------------------------
//----------------------------------------------------------------------------------


struct DACOM_NO_VTABLE TrailManager : public IObjectFactory
{
	//
	// incoming interface map
	//
  
	BEGIN_DACOM_MAP_INBOUND(TrailManager)
	DACOM_INTERFACE_ENTRY(IObjectFactory)
	END_DACOM_MAP()

//	struct TrailNode *explosionList;
	U32 factoryHandle;


	//child object info
	TrailArchetype *pArchetype;

	//TrailManager methods

	TrailManager (void) 
	{
	}

	~TrailManager();
	
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
// TrailManager methods

TrailManager::~TrailManager()
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
void TrailManager::init()
{
	COMPTR<IDAConnectionPoint> connection;


	if (OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
	{
		connection->Advise(GetBase(), &factoryHandle);
	}
}
//--------------------------------------------------------------------------
//
HANDLE TrailManager::CreateArchetype(const char *szArchname, OBJCLASS objClass, void *data)
{
	if (objClass == OC_TRAIL)
	{
		TrailArchetype *newguy = new TrailArchetype;
		newguy->name = szArchname;
		newguy->data = (BT_TRAIL_DATA *)data;

		COMPTR<IFileSystem> objFile;
		DAFILEDESC fdesc;

		if (newguy->data->anim)
		{
			fdesc.lpFileName = newguy->data->anim;
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

	return 0;
}
//--------------------------------------------------------------------------
//
BOOL32 TrailManager::DestroyArchetype(HANDLE hArchetype)
{
	TrailArchetype *deadguy = (TrailArchetype *)hArchetype;

	delete deadguy;
	return 1;
}
//--------------------------------------------------------------------------
//
IBaseObject * TrailManager::CreateInstance(HANDLE hArchetype)
{
	TrailArchetype *pTrail = (TrailArchetype *)hArchetype;
	BT_TRAIL_DATA *objData = ((TrailArchetype *)hArchetype)->data;
	
	if (objData->objClass == OC_TRAIL)
	{
		Trail * obj = new ObjectImpl<Trail>;
		obj->objClass = OC_TRAIL;

//		obj->data = objData;
		obj->arch = pTrail;

		return obj;
	}
	
	return 0;
}
//--------------------------------------------------------------------------
//
void TrailManager::EditorCreateInstance(HANDLE hArchetype, const MISSION_INFO & info)
{
	// not supported
}
//----------------------------------------------------------------------------------------------
//
struct IBaseObject * __stdcall CreateTrail (PARCHETYPE pArchetype, IBaseObject *_jumper, Vector _target, U32 _systemID)
{
	Trail *result;
	
 	result = (Trail *)ARCHLIST->CreateInstance(pArchetype);
 	if (result)
 	{
 		result->InitTrail(_jumper,_target,_systemID);
 	}

	return result;
}
//----------------------------------------------------------------------------------------------
//
struct _trailFact : GlobalComponent
{
	struct TrailManager *trailMgr;

	virtual void Startup (void)
	{
		trailMgr = new DAComponent<TrailManager>;
		AddToGlobalCleanupList((IDAComponent **) &trailMgr);
	}

	virtual void Initialize (void)
	{
		trailMgr->init();
	}
};

static _trailFact trailfact;
//----------------------------------------------------------------------------------
//----------------------------------------------------------------------------------

//---------------------------------------------------------------------------
//--------------------------End Trail.cpp------------------------------------
//---------------------------------------------------------------------------
