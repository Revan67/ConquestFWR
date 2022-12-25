//--------------------------------------------------------------------------//
//                                                                          //
//                                 Beam.cpp                                 //
//                                                                          //
//                  COPYRIGHT (C) 1997 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*

    $Author: Jasony $

    $Header: /Conquest/App/Src/GattlingBeam.cpp 9     11/02/00 1:24p Jasony $
*/			    
//--------------------------------------------------------------------------//

#include "pch.h"
#include <globals.h>

#include "TObject.h"
#include "Camera.h"
#include "Objlist.h"
#include "sfx.h"
#include "IConnection.h"
#include "Startup.h"
#include "SuperTrans.h"
#include "DWeapon.h"
#include "IWeapon.h"
#include "CQLight.h"
#include "TObjTrans.h"
#include "TObjFrame.h"
#include "MGlobals.h"
#include "Mission.h"
#include "IMissionActor.h"
#include "TManager.h"
#include "IBlast.h"
#include "OpAgent.h"

#include <TComponent.h>
#include <lightman.h>
#include <Engine.h>
#include <Vector.h>
#include <Physics.h>
#include <Matrix.h>
#include <IHardpoint.h>
#include <stdlib.h>
#include <FileSys.h>
#include <ICamera.h>
#include <Collision.h>
#include <Pixel.h>
//#include <RPUL\PrimitiveBuilder.h>
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//
struct GattlingBeamArchetype
{
	const char *name;
	BT_GATTLINGBEAM_DATA *data;
	ARCHETYPE_INDEX archIndex;
	IMeshArchetype * meshArch;
	PARCHETYPE pArchetype;
	PARCHETYPE pContactBlast;
	U32 textureID;

	GattlingBeamArchetype (void)
	{
		memset(this, 0, sizeof(*this));
		archIndex = -1;
		meshArch = 0;
	}

	~GattlingBeamArchetype (void)
	{
		ENGINE->release_archetype(archIndex);
		if (pContactBlast)
			ARCHLIST->Release(pContactBlast, OBJREFNAME);
		if(textureID)
			TMANAGER->ReleaseTextureRef(textureID);
	}
};

#define MAX_ENDS 20

struct _NO_VTABLE GattlingBeam : public ObjectTransform<ObjectFrame<IBaseObject,GATTLINGBEAM_SAVELOAD,GattlingBeamArchetype> >, IWeapon, BASE_GATTLINGBEAM_SAVELOAD
{
	BEGIN_MAP_INBOUND(GattlingBeam)
	_INTERFACE_ENTRY(IBaseObject)
	_INTERFACE_ENTRY(IWeapon)
	END_MAP()

	//------------------------------------------
	unsigned int textureID;
	const BT_GATTLINGBEAM_DATA * data;
	OBJPTR<IWeaponTarget> target;
	OBJPTR<IBaseObject> owner;

	HSOUND hSound;

	Vector lastEnd[MAX_ENDS];
	Vector lastStart[MAX_ENDS];
	SINGLE endAge[MAX_ENDS];
	U32 numEnds;
	U32 firstEnd;

	SINGLE range;
	//------------------------------------------

	GattlingBeam (void);
	virtual ~GattlingBeam (void);	// See ObjList.cpp

	virtual BOOL32 Update (void);

	virtual void PhysicalUpdate (SINGLE dt);

	virtual void Render (void);

	virtual U32 GetSystemID (void) const
	{
		return systemID;
	}

	virtual U32 GetPartID (void) const
	{
		return ownerID;
	}

	virtual U32 GetPlayerID (void) const
	{
		return MGlobals::GetPlayerFromPartID(ownerID);
	}

	/* IWeapon methods */

	virtual void InitWeapon (IBaseObject * _owner, const class TRANSFORM & orientation, IBaseObject * obj, U32 flags, const class Vector * pos=0);

	void init (const GattlingBeamArchetype & data);
};

//----------------------------------------------------------------------------------
//
GattlingBeam::GattlingBeam (void) 
{
}

//----------------------------------------------------------------------------------
//
GattlingBeam::~GattlingBeam (void)
{
	OBJLIST->ReleaseProjectile();
	textureID = 0;
}

//----------------------------------------------------------------------------------
//
BOOL32 GattlingBeam::Update (void)
{
	BOOL32 result;

	result = (time+2) > 0;

	return result;
}

void GattlingBeam::PhysicalUpdate (SINGLE dt)
{
	time -= dt;
	if(!owner)
		return;
	while(lastTime > 0 && lastTime > time)
	{	
		SINGLE t = time/data->lifetime;
		Vector targEnd = (target1-target2)*t+target2;

		Vector start = owner->GetTransform()*startOffset;
		Vector realDir = targEnd-start;
		Vector end = realDir;
		end = start+end.normalize()*range;
		SINGLE wobble = data->maxSweepDist/4;
		end += Vector(((rand()%1000)/500.0)*wobble-wobble,
							((rand()%1000)/500.0)*wobble-wobble,
							((rand()%1000)/500.0)*wobble-wobble);


		if (target!=0 && lastTime > 0 && owner)
		{
			Vector collide_point,dir;

			if (target->GetCollisionPosition(collide_point,dir,start,realDir))
			{
				Vector len = collide_point;
				len -= start;
				SINGLE collide_dist = len.fast_magnitude();

				if (range >= collide_dist)		// we have a winner!
				{
					end = collide_point;
					target->ApplyVisualDamage(this, ownerID, start,realDir, 0);
					if(data->contactBlast[0])
					{
						if (bVisible)
						{
							Vector direction = (end-start).normalize();

							Vector cpos = MAINCAM->get_position();
		
							Vector look (cpos - start);
							Vector i = cross_product(look,-direction);

							Vector j = cross_product(i,-direction);
							Transform t;
							t.set_i(i);
							t.set_j(j);
							t.set_k(-direction);
							t.translation = end;
							IBaseObject * obj = CreateBlast(
								((GattlingBeamArchetype *)(ARCHLIST->GetArchetypeHandle(pArchetype)))->pContactBlast,
								t, systemID);
							CQASSERT(obj);
							OBJLIST->AddObject(obj);
						}
					}
				}
			}
		}

		lastEnd[(firstEnd+numEnds)%MAX_ENDS] = end;
		lastStart[(firstEnd+numEnds)%MAX_ENDS] = start;
		endAge[(firstEnd+numEnds)%MAX_ENDS] = lastTime;
		if(numEnds != MAX_ENDS)
			++numEnds;
		else
			firstEnd = (firstEnd+1)%MAX_ENDS;
		lastTime -= 0.25;
	}
}

//----------------------------------------------------------------------------------
//
void GattlingBeam::Render (void)
{
	if (bVisible==0)
		return;
	if(!owner)
		return;

	//visibility code
	#define TOLERANCE 0.00001f

	for(U32 i = 0; i < numEnds; ++i)
	{
		SINGLE age = endAge[(firstEnd+i)%MAX_ENDS]-time;
		if(age < 0.5)
		{
			Vector v0,v1,v2,v3;
			Vector cpos = MAINCAM->get_position();
			
			SINGLE width = data->beamWidth;

			Vector start = lastStart[(firstEnd+i)%MAX_ENDS];
			Vector look (cpos - start);
			Vector end = lastEnd[(firstEnd+i)%MAX_ENDS];
			Vector direction = end-start;
			SINGLE length = direction.fast_magnitude();
			direction.normalize();

			Vector i;
			i = cross_product(look,direction);

			if (fabs (i.x) < TOLERANCE && fabs (i.y) < TOLERANCE)
			{
				i.x = 1.0f;
			}
			
			i.normalize ();

			BATCH->set_state(RPR_BATCH,TRUE);
			BATCH->set_state(RPR_STATE_ID,textureID);

			BATCH->set_render_state(D3DRS_ZENABLE,TRUE);
			BATCH->set_render_state(D3DRS_ZWRITEENABLE,FALSE);
			BATCH->set_render_state(D3DRS_ALPHABLENDENABLE,TRUE);
			BATCH->set_render_state(D3DRS_SRCBLEND,D3DBLEND_ONE);
			BATCH->set_render_state(D3DRS_DESTBLEND,D3DBLEND_ONE);

			BATCH->set_render_state(D3DRS_CULLMODE,D3DCULL_NONE);

			SetupDiffuseBlend(textureID,TRUE);

			CAMERA->SetModelView();

			v0 = (start - (i * width));
			v1 = (start + (i * width));
			v2 = (end + (i * width));
			v3 = (end - (i * width));

			SINGLE alphaMod = (1.0-(age*2))/1.0;
			PB.Color3ub(255*alphaMod,255*alphaMod,255*alphaMod);
			U8 alpha = 255*(1.0-((length+1)/(range+500)))*alphaMod;
			PB.Begin(PB_QUADS);

			PB.TexCoord2f(0,0);     PB.Vertex3f(v0.x,v0.y,v0.z);
			PB.TexCoord2f(1,0);		PB.Vertex3f(v1.x,v1.y,v1.z);
			PB.Color3ub(alpha,alpha,alpha);
			PB.TexCoord2f(1,length*0.002);		PB.Vertex3f(v2.x,v2.y,v2.z);
			PB.TexCoord2f(0,length*0.002);		PB.Vertex3f(v3.x,v3.y,v3.z);

			PB.End();  //GL_QUADS
		}
	}

	BATCH->set_state(RPR_STATE_ID,0);
}
//----------------------------------------------------------------------------------
//
void GattlingBeam::InitWeapon (IBaseObject * _owner, const class TRANSFORM & orientation, IBaseObject * _target, U32 flags, const class Vector * pos)
{
	numEnds = 0;
	firstEnd = 0;
	Vector goal = (pos)? *pos : _target->GetPosition();

	systemID = _owner->GetSystemID();
	ownerID = _owner->GetPartID();
	U32 visFlags = _owner->GetTrueVisibilityFlags();

	if (_target)
	{
		_target->QueryInterface(IWeaponTargetID, target,SYSVOLATILEPTR);
		CQASSERT(target!=0);

		targetID = _target->GetPartID();
		visFlags |= _target->GetTrueVisibilityFlags();
		_owner->SetVisibleToAllies(1 << (_target->GetPlayerID()-1));
		_target->SetVisibleToAllies(1 << (_owner->GetPlayerID()-1));
	}

	if(_owner)
	{
		_owner->QueryInterface(IBaseObjectID,owner,SYSVOLATILEPTR);
	}

	SetVisibilityFlags(visFlags);

	target1 = Vector(((rand()%1000)/500.0)*data->maxSweepDist-data->maxSweepDist,
							((rand()%1000)/500.0)*data->maxSweepDist-data->maxSweepDist,
							((rand()%1000)/500.0)*data->maxSweepDist-data->maxSweepDist);

	target2 = goal+target1;
	target1 = goal-target1;

	startOffset =  _owner->GetTransform().get_inverse() * orientation.get_position();

	range = (goal-orientation.get_position()).fast_magnitude()+4000;

	hSound = SFXMANAGER->Open(data->launchSfx);

	Vector playPos = orientation.get_position();
	SFXMANAGER->Play(hSound,systemID,&playPos);

	lastTime = time = data->lifetime;
}
//---------------------------------------------------------------------------
//
void GattlingBeam::init (const GattlingBeamArchetype & arch)
{
	CQASSERT(arch.data->wpnClass == WPN_GATTLINGBEAM);
	CQASSERT(arch.data->objClass == OC_WEAPON);

	data = (BT_GATTLINGBEAM_DATA *)arch.data;

	CQASSERT(data->fileName[0]);
	textureID = arch.textureID;

	pArchetype = arch.pArchetype;
	objClass = OC_WEAPON;

}
//---------------------------------------------------------------------------
//
static GattlingBeam * CreateGattlingBeam (GattlingBeamArchetype & data)
{
	GattlingBeam * beam = new ObjectImpl<GattlingBeam>;

	beam->FRAME_init(data);
	beam->init(data);

	return beam;
}
//----------------------------------------------------------------------------------------------
//-------------------------------class GattlingBeamManager--------------------------------------------
//----------------------------------------------------------------------------------------------
//
struct ArcCannon;
IBaseObject * CreateArcCannon (PARCHETYPE pArchetype, S32 archeIndex);
void CreateArcCannonArchetype();
void ReleaseArcCannonArchetype();
IBaseObject * CreateAutoCannon (PARCHETYPE pArchetype, S32 archeIndex, PARCHETYPE _PBlastType, PARCHETYPE _pSparkType);


struct DACOM_NO_VTABLE GattlingBeamManager : public IObjectFactory
{
	//
	// incoming interface map
	//
  
	BEGIN_DACOM_MAP_INBOUND(GattlingBeamManager)
	DACOM_INTERFACE_ENTRY(IObjectFactory)
	END_DACOM_MAP()

	U32 factoryHandle;

	//child object info
	GattlingBeamArchetype *pArchetype;

	//BeamManager methods

	GattlingBeamManager (void) 
	{
	}

	~GattlingBeamManager();
	
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
// GattlingBeamManager methods

GattlingBeamManager::~GattlingBeamManager()
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
void GattlingBeamManager::init()
{
	COMPTR<IDAConnectionPoint> connection;

	if (OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
		connection->Advise(GetBase(), &factoryHandle);
}
//--------------------------------------------------------------------------
//
HANDLE GattlingBeamManager::CreateArchetype(const char *szArchname, OBJCLASS objClass, void *_data)
{
	if (objClass == OC_WEAPON)
	{
		BT_GATTLINGBEAM_DATA * data = (BT_GATTLINGBEAM_DATA *)_data;
		if (data->wpnClass == WPN_GATTLINGBEAM)
		{
			
			GattlingBeamArchetype *newguy = new GattlingBeamArchetype;
			
			newguy->name = szArchname;
			newguy->data = data;
			newguy->pArchetype = ARCHLIST->GetArchetype(szArchname);
			if(data->contactBlast[0])
			{
				newguy->pContactBlast = ARCHLIST->LoadArchetype(data->contactBlast);
				if(newguy->pContactBlast)
					ARCHLIST->AddRef(newguy->pContactBlast, OBJREFNAME);
			}
			
			newguy->textureID = TMANAGER->CreateTextureFromFile(data->fileName, TEXTURESDIR, DA::TGA, PF_4CC_DAA4);
			//
			// force preload of sound effect
			// 
			SFXMANAGER->Preload(data->launchSfx);
			
			return newguy;
		}
	}
	
	return 0;
}
//--------------------------------------------------------------------------
//
BOOL32 GattlingBeamManager::DestroyArchetype(HANDLE hArchetype)
{
	GattlingBeamArchetype *pWeapon = (GattlingBeamArchetype *)hArchetype;
	BT_GATTLINGBEAM_DATA *objData = pWeapon->data;
	
	if (objData->objClass == OC_WEAPON)
	{
//		IBaseObject * obj = NULL;
	/*	switch (objData->wpnClass)
		{
		case WPN_ARC:
			ReleaseArcCannonArchetype();
			break;
		}*/
	}
	GattlingBeamArchetype *deadguy = (GattlingBeamArchetype *)hArchetype;
	delete deadguy;

	return 1;
}
//--------------------------------------------------------------------------
//
IBaseObject * GattlingBeamManager::CreateInstance(HANDLE hArchetype)
{
	GattlingBeamArchetype *pWeapon = (GattlingBeamArchetype *)hArchetype;
	BT_GATTLINGBEAM_DATA *objData = pWeapon->data;
//	INSTANCE_INDEX index=-1;
	
	if (objData->objClass == OC_WEAPON)
	{
		IBaseObject * obj = NULL;
		switch (objData->wpnClass)
		{
		case WPN_GATTLINGBEAM:
			{
				obj = CreateGattlingBeam(*pWeapon);
			}
			break;
		}

		return obj;
	}
	
	return 0;
}
//--------------------------------------------------------------------------
//
void GattlingBeamManager::EditorCreateInstance(HANDLE hArchetype,const MISSION_INFO & info)
{
	// not supported
}

//----------------------------------------------------------------------------------------------
//
struct _gguns : GlobalComponent
{
	GattlingBeamManager * weaponMgr;

	virtual void Startup (void)
	{
		weaponMgr = new DAComponent<GattlingBeamManager>;
		AddToGlobalCleanupList((IDAComponent **) &weaponMgr);
	}

	virtual void Initialize (void)
	{
		weaponMgr->init();
	}
};

static _gguns guns;

//--------------------------------------------------------------------------//
//------------------------------END Beam.cpp--------------------------------//
//--------------------------------------------------------------------------//
