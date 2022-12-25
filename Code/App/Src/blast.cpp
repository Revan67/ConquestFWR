//--------------------------------------------------------------------------//
//                                                                          //
//                                 Blast.cpp                                //
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
#include "IConnection.h"
#include "Camera.h"
#include "DExplosion.h"
#include "Objlist.h"
#include "Anim2D.h"
#include "CQlight.h"
#include "Sfx.h"
#include "TObjTrans.h"
#include "TobjFrame.h"
#include "Startup.h"
#include "IBlast.h"
#include "ICamera.h"
#include "UserDefaults.h"
#include <MGlobals.h>
#include "Fogofwar.h"

#include <View2D.h>
#include <FileSys.h>
#include <Engine.h>
#include <IRenderPrimitive.h>


struct BlastArchetype
{
	const char *name;
	BT_BLAST *data;
	PARCHETYPE pArchetype[3];
	PARCHETYPE pArch;
	S32 archIndex;
	IMeshArchetype * meshArch;

    void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}

	BlastArchetype (void)
	{
		meshArch = NULL;
		archIndex = -1;
	}

	~BlastArchetype (void)
	{
		for (int i=0;i<3;i++)
		{
			if (pArchetype[i])
				ARCHLIST->Release(pArchetype[i], OBJREFNAME);
		}
	}

};

struct DUMMY_SAVESTRUCT			// required by template framework
{
	TRANS_SAVELOAD trans_SL;
};
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
struct EffectNode
{
	IBaseObject *obj;
	EffectNode *next;
};

struct _NO_VTABLE Blast : public ObjectTransform<ObjectFrame<IBaseObject,struct DUMMY_SAVESTRUCT,BlastArchetype> >, IBlast
{

	BEGIN_MAP_INBOUND(Blast)
	_INTERFACE_ENTRY(IBaseObject)
	_INTERFACE_ENTRY(IBlast)
	_INTERFACE_ENTRY(IPhysicalObject)
	END_MAP()

	//------------------------------------------

	SINGLE timeToLive,lightTime,totalTime,lightTotalTime;
	CQLight *light;
	BlastArchetype *arch;
	HSOUND hSound;
	SINGLE lightRange;
	BT_BLAST *data;
	S32 growSpeed;
	SINGLE animScale;
	U32 cnt;
	U32 systemID;
	U32 ownerID;
	OBJPTR<IBaseObject> owner;
	TRANSFORM trans;
	EffectNode *effectList;
	BOOL32 bSounded:1;
	BOOL32 bLit:1;
	SINGLE delay;
	SINGLE blastRadius;

	//------------------------------------------

	Blast (void)// : point(0)
	{
		timeToLive = 10;
		blastRadius = -1;
	}

    void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}

	virtual ~Blast (void);	// See ObjList.cpp

	/* IBaseObject methods */

	virtual void TestVisible (const USER_DEFAULTS & defaults, const U32 currentSystem, const U32 currentPlayer);

	virtual BOOL32 Update (void);

	virtual void PhysicalUpdate (SINGLE dt)
	{
		if (delay <= 0)
		{
			FRAME_physicalUpdate(dt);
			EffectNode *pos = effectList;
			while (pos)
			{
				pos->obj->PhysicalUpdate(dt);
				pos = pos->next;
			}
			

			if (bVisible && light && light->range && lightTime > 0)
			{
				LightRGB color;
				light->range = lightRange*(lightTime/lightTotalTime);
				color.r = data->flash.red*(lightTime/lightTotalTime);
				color.g = data->flash.green*(lightTime/lightTotalTime);
				color.b = data->flash.blue*(lightTime/lightTotalTime);
				light->setColor(color.r,color.g,color.b);
				light->UpdateLight(true);
				lightTime -= dt;
			}
		}
	}

	virtual void Render (void);

	virtual U32 GetSystemID (void) const
	{
		return systemID;
	}
	
	virtual U32 GetPlayerID (void) const
	{
		return MGlobals::GetPlayerFromPartID(ownerID);
	}

	BOOL32 InitBlast (const class TRANSFORM & orientation, U32 systemID, IBaseObject *owner,SINGLE _animScale=1.0,SINGLE activationTime=0);

	BOOL32 EditorInitBlast();

	virtual void SetRelativeTransform (const class TRANSFORM &_trans);

	virtual void AddEffect(IBaseObject *obj);

	virtual void GetFireball(OBJPTR<IBaseObject> & pInterface)
	{
		EffectNode *pos = effectList;
		OBJPTR<IFireball> fire;
		while (pos)
		{
			if (pos->obj->QueryInterface(IFireballID,pInterface, SYSVOLATILEPTR))
			{
				return;
			}
			pos = pos->next;
		}
	}

	virtual void SetVisible (bool _bVisible)
	{
		bVisible = _bVisible;
	}

	bool get_projected_bounding_sphere(	float & cx,
										  float & cy,
										  float & radius,
										  float & depth);

	//IPhysicalObject

	virtual void SetSystemID (U32 newSystemID)
	{
		systemID = newSystemID;
		if (light)
			light->setSystem(systemID);
	}

	virtual void SetPosition (const Vector & position, U32 newSystemID);

	virtual void SetTransform (const TRANSFORM & transform, U32 newSystemID);
};

//----------------------------------------------------------------------------------
//
Blast::~Blast (void)
{
	if (hSound)
		SFXMANAGER->CloseHandle(hSound);

	
	EffectNode *pos = effectList;

	while (pos)
	{
		effectList = pos->next;
		delete pos->obj;
		delete pos;
		pos = effectList;
	}

	if (light)
		delete light;
}
//----------------------------------------------------------------------------------
//
void Blast::TestVisible (const USER_DEFAULTS & defaults, const U32 currentSystem, const U32 currentPlayer)
{
	if (delay > 0)
	{
		bVisible = FALSE;
		return;
	}
	
	float depth, center_x, center_y, radius;
	
	if (owner)
	{
		TRANSFORM ownerTrans = owner->GetTransform();
		transform = ownerTrans.multiply(trans);
	}
	
	bVisible = data->bDrawThroughFog || (GetSystemID() == currentSystem && get_projected_bounding_sphere(center_x, center_y, radius, depth) &&
		((FOGOFWAR->CheckVisiblePosition(transform.translation)) || defaults.bVisibilityRulesOff) );
}
//----------------------------------------------------------------------------------
//
BOOL32 Blast::Update (void)
{
	CQASSERT(totalTime);
	BOOL32 allDone = TRUE;
	
	if (delay <= 0)
	{
		EffectNode *pos = effectList;
		while (pos)
		{
			if (pos->obj->Update())
				allDone = FALSE;
			pos = pos->next;
		}

		if (!bVisible && light && light->range && lightTime > 0)
		{
			LightRGB color;
			light->range = lightRange*(lightTime/lightTotalTime);
			color.r = data->flash.red*(lightTime/lightTotalTime);
			color.g = data->flash.green*(lightTime/lightTotalTime);
			color.b = data->flash.blue*(lightTime/lightTotalTime);
			light->setColor(color.r,color.g,color.b);
			light->UpdateLight(true);
			lightTime -= ELAPSED_TIME;
		}
		timeToLive -= (float)ELAPSED_TIME;
	}
	else
	{
		delay -= ELAPSED_TIME;
		if (delay < 0 && !bLit)
		{
			if (owner)
			{
				TRANSFORM ownerTrans = owner->GetTransform();
				transform = ownerTrans.multiply(trans);
			}
			else
				transform = trans;
			Vector pos = trans.translation;
			if (owner)
				pos += owner->GetPosition();
			if (light)
			{
				light->range = data->flash.range;
				lightRange = light->range;
				if (light->range)
				{
					light->setColor(data->flash.red,data->flash.green,data->flash.blue);
					light->setSystem(systemID);
					Vector light_pos = pos;
					light_pos.z += 200.0;
					light->set_position(light_pos);
					light->enable();
				}
			}
			bLit = TRUE;
			EffectNode *fxpos = effectList;
			while (fxpos)
			{
				OBJPTR<IEffect> effect;
				fxpos->obj->QueryInterface(IEffectID,effect);
				CQASSERT(effect);
				effect->InitEffect(this,trans,animScale,totalTime);
				blastRadius = max(blastRadius,effect->GetRadius());
				fxpos = fxpos->next;
			}
		}

	}

	if (!bSounded && delay <= data->leadTime)
	{
		Vector pos = trans.translation;
		if (owner)
			pos += owner->GetPosition();
		SFXMANAGER->Play(hSound, systemID, &pos);
		bSounded = TRUE;
	}

	return ((timeToLive > 0) || !allDone);
}
//----------------------------------------------------------------------------------
//
void Blast::Render (void)
{
	if (bVisible)
	{
		if (delay <= 0)
		{
			bHighlight = 0;
			BATCH->set_render_state(D3DRS_ZWRITEENABLE,FALSE);
			if (owner)
			{
				TRANSFORM ownerTrans = owner->GetTransform();
				transform = ownerTrans.multiply(trans);
			}


			EffectNode *pos = effectList;
			while (pos)
			{
				pos->obj->Render();
				pos = pos->next;
			}
		}
	}
}
//----------------------------------------------------------------------------------
//
BOOL32 Blast::InitBlast (const class TRANSFORM & orientation, U32 _systemID, IBaseObject *_owner,SINGLE _animScale,SINGLE activationTime)
{
	BOOL32 result = 0;

	if (_owner)
		_owner->QueryInterface(IBaseObjectID, owner, SYSVOLATILEPTR);
	else
		owner = 0;

	Vector pos = orientation.get_position();

	if (owner)
	{
		pos += owner->GetPosition();
		ownerID = owner->GetPartID();
		SetVisibilityFlags(owner->GetVisibilityFlags());
	}

	systemID = _systemID;

	CQASSERT(data);
	if (data)
	{
		bLit = 0;

		trans = orientation;
		if (owner)
		{
			TRANSFORM ownerTrans = owner->GetTransform();
			transform = ownerTrans.multiply(trans);
		}
		else
			transform = trans;

		objClass = OC_BLAST;
		if ((timeToLive = totalTime = data->totalTime) == 0)
			timeToLive = totalTime = 1;
		CQASSERT(totalTime);
		lightTime = lightTotalTime = data->flash.lifeTime;
	
		delay = activationTime;
		if (hSound==0)
			hSound = SFXMANAGER->Open(data->sfx);
		if (activationTime <= data->leadTime)
		{
			
			SFXMANAGER->Play(hSound, systemID, &pos);
			bSounded = TRUE;
		}
		
		if (data->flash.lifeTime && light==0)
			light = new CQLight;

		if (light)
			light->disable();
		if (activationTime == 0)
		{
			bLit = TRUE;
			if (light)
			{
				light->range = data->flash.range;
				lightRange = light->range;
				if (light->range)
				{
					//				light->color.r = data->flash.red;
					//				light->color.g = data->flash.green;
					//				light->color.b = data->flash.blue;
					
					light->setColor(data->flash.red,data->flash.green,data->flash.blue);
					
					light->setSystem(systemID);
					Vector light_pos = pos;
					light_pos.z += 200.0;
					light->set_position(light_pos);
					light->enable();
				}
			}
			EffectNode *fxpos = effectList;
			while (fxpos)
			{
				OBJPTR<IEffect> effect;
				fxpos->obj->QueryInterface(IEffectID,effect);
				CQASSERT(effect);
				effect->InitEffect(this,orientation,_animScale,totalTime);
				blastRadius = max(blastRadius,effect->GetRadius());
				fxpos = fxpos->next;
			}
		}
		result = 1;
	}

	animScale = _animScale;


	return result;

}

//----------------------------------------------------------------------------------
//
BOOL32 Blast::EditorInitBlast ()
{
	BOOL32 result = 0;

	CQASSERT(data);
	if (data)
	{
		trans = transform;

		objClass = OC_BLAST;
		if ((timeToLive = totalTime = data->totalTime) == 0)
			timeToLive = totalTime = 1;
		CQASSERT(totalTime);
		lightTime = lightTotalTime = data->flash.lifeTime;
		
		delay = 0;
		hSound = SFXMANAGER->Open(data->sfx);
		
		bLit = TRUE;
		if (data->flash.lifeTime)
		{
			if (light==0)
				light = new CQLight;
			if (light)
			{
				light->range = data->flash.range;
				lightRange = light->range;
				if (light->range)
				{
					light->setColor(data->flash.red,data->flash.green,data->flash.blue);
					
					light->setSystem(systemID);
					Vector light_pos = transform.translation;
					light_pos.z += 200.0;
					light->set_position(light_pos);
					light->enable();
				}
			}
		}
		EffectNode *fxpos = effectList;
		while (fxpos)
		{
			VOLPTR(IEffect) effect=fxpos->obj;
			CQASSERT(effect);
			effect->EditorInitEffect(this,totalTime);
			blastRadius = max(blastRadius,effect->GetRadius());
			fxpos = fxpos->next;
		}
		result = 1;
	}

	animScale = 1;


	return result;

}

void Blast::SetRelativeTransform(const TRANSFORM &_trans)
{
	trans = _trans;
	if (owner == 0)
		transform = trans;
	if (light && light->range)
	{
		Vector light_pos = trans.translation;
		if (owner)
			light_pos += owner->GetPosition();
		light_pos.z += 200.0;
		light->set_position(light_pos);
	}
}

void Blast::AddEffect(IBaseObject *obj)
{
	EffectNode *node = new EffectNode;
	node->obj = obj;
	node->next = effectList;
	effectList = node;
}

bool Blast::get_projected_bounding_sphere(	float & cx,
										  float & cy,
										  float & radius,
										  float & depth)
{
	bool result = false;
//	CQASSERT(blastRadius != -1);

	Transform cam2world = MAINCAM->get_transform();
	Vector wcenter = trans.translation;
	if (owner)
		wcenter += owner->GetPosition();
	Vector vcenter = cam2world.inverse_rotate_translate(wcenter);
				
	// Make sure object is in front of near plane.
	if (vcenter.z < -1.0)
	{
		const struct ViewRect * pane = MAINCAM->get_pane();
		
		float x_screen_center = float(pane->x1 - pane->x0) * 0.5f;
		float y_screen_center = float(pane->y1 - pane->y0) * 0.5f;
		float screen_center_x = pane->x0 + x_screen_center;
		float screen_center_y = pane->y0 + y_screen_center;
		
		float w = -1.0 / vcenter.z;
		float sphere_center_x = vcenter.x * w;
		float sphere_center_y = vcenter.y * w;
		
		cx = screen_center_x + sphere_center_x * MAINCAM->get_hpc()*MAINCAM->get_znear();
		cy = screen_center_y + sphere_center_y * MAINCAM->get_vpc()*MAINCAM->get_znear();
		
		Matrix Rc = cam2world.get_orientation();
		
		float center_distance = vcenter.magnitude();
		
		if (center_distance >= blastRadius)
		{
			float dx = fabs(cx - screen_center_x);
			float dy = fabs(cy - screen_center_y);

			float outer_angle = asin(blastRadius / center_distance);
			sphere_center_x = fabs(sphere_center_x);
			float inner_angle = atan(sphere_center_x);
			
			float near_plane_radius = tan(inner_angle - outer_angle);
			near_plane_radius = sphere_center_x-near_plane_radius;
			radius = near_plane_radius * MAINCAM->get_hpc()*MAINCAM->get_znear();

			int view_w = (pane->x1 - pane->x0 + 1) >> 1;
			int view_h = (pane->y1 - pane->y0 + 1) >> 1;
			
			if ((dx < (view_w + radius)) && (dy < (view_h + radius)))
			{
				depth = -vcenter.z;
				result = true;
			}
		}
		else
			result = true;
	}
	
	return result;
}

void Blast::SetPosition (const Vector & position, U32 newSystemID)
{
	SetSystemID(newSystemID);
	trans.translation = position;
	if (owner == 0)
		transform.translation = position;
	if (light && light->range)
	{
		Vector light_pos = position;
		if (owner)
			light_pos += owner->GetPosition();
		light_pos.z += 200.0;
		light->set_position(light_pos);
	}
/*	EffectNode *fxpos = effectList;
	while (fxpos)
	{
		OBJPTR<IEffect> effect;
		fxpos->obj->QueryInterface(IEffectID,effect);
		CQASSERT(effect);
		effect->SetPosition(position);
		fxpos = fxpos->next;
	}*/
}

void Blast::SetTransform (const TRANSFORM &_trans, U32 newSystemID)
{
	SetSystemID(newSystemID);
	trans = _trans;
	if (owner == 0)
		transform = trans;
	if (light && light->range)
	{
		Vector light_pos = trans.translation;
		light_pos.z += 200.0;
		if (owner)
			light_pos += owner->GetPosition();
		light->set_position(light_pos);
	}
/*	EffectNode *fxpos = effectList;
	while (fxpos)
	{
		OBJPTR<IEffect> effect;
		fxpos->obj->QueryInterface(IEffectID,effect);
		CQASSERT(effect);
		effect->SetTransform(trans);
		fxpos = fxpos->next;
	}*/
}
//----------------------------------------------------------------------------------
//---------------------------------Blast Factory------------------------------------
//----------------------------------------------------------------------------------
//----------------------------------------------------------------------------------


struct DACOM_NO_VTABLE BlastManager : public IObjectFactory
{
	//
	// incoming interface map
	//
  
	BEGIN_DACOM_MAP_INBOUND(BlastManager)
	DACOM_INTERFACE_ENTRY(IObjectFactory)
	END_DACOM_MAP()

//	struct BlastNode *explosionList;
	U32 factoryHandle;

	//BlastManager methods

	BlastManager (void) 
	{
	}

	~BlastManager();
	
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
// BlastManager methods

BlastManager::~BlastManager()
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
void BlastManager::init()
{
	COMPTR<IDAConnectionPoint> connection;


	if (OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
	{
		connection->Advise(GetBase(), &factoryHandle);
	}
}
//--------------------------------------------------------------------------
//
HANDLE BlastManager::CreateArchetype(const char *szArchname, OBJCLASS objClass, void *data)
{
	if (objClass == OC_BLAST)
	{
		BlastArchetype *newguy = new BlastArchetype;
		newguy->name = szArchname;
		newguy->data = (BT_BLAST *)data;
		newguy->pArch = ARCHLIST->GetArchetype(szArchname);

	
		SFXMANAGER->Preload(newguy->data->sfx);	// force the sfx to load early

		/*COMPTR<IFileSystem> objFile;
		DAFILEDESC fdesc = newguy->data->fileName;
		if (OBJECTDIR->CreateInstance(&fdesc, objFile) == GR_OK)
			newguy->archIndex = ENGINE->create_archetype(fdesc.lpFileName,objFile);

		if (newguy->data->animName)
		{
			fdesc.lpFileName = newguy->data->animName;
			if (OBJECTDIR->CreateInstance(&fdesc, objFile) == GR_OK)
			{
				newguy->animArch = ANIM2D->create_archetype(objFile);
			}
			else 
			{
				CQFILENOTFOUND(fdesc.lpFileName);
				newguy->animArch =0;
			}
		}*/

		for (int i=0;i<3;i++)
		{
			if (newguy->data->effectType[i][0])
			{
				newguy->pArchetype[i] = ARCHLIST->LoadArchetype(newguy->data->effectType[i]);
				if (newguy->pArchetype[i])
					ARCHLIST->AddRef(newguy->pArchetype[i], OBJREFNAME);
				else
					CQTRACE11("Effect not loaded %s",newguy->data->effectType[i]);
			}
		}	
		return newguy;
	}

	return 0;
}
//--------------------------------------------------------------------------
//
BOOL32 BlastManager::DestroyArchetype(HANDLE hArchetype)
{
	BlastArchetype *deadguy = (BlastArchetype *)hArchetype;

	delete deadguy;
	return 1;
}
//--------------------------------------------------------------------------
//
IBaseObject * BlastManager::CreateInstance(HANDLE hArchetype)
{
	BlastArchetype *pBlast = (BlastArchetype *)hArchetype;
	BT_BLAST *objData = ((BlastArchetype *)hArchetype)->data;
	
	if (objData->objClass == OC_BLAST)
	{
		Blast * obj = new ObjectImpl<Blast>;
		obj->objClass = OC_BLAST;
		obj->FRAME_init(*pBlast);

		obj->data = objData;
		obj->arch = pBlast;
		obj->totalTime = objData->totalTime;

		for (int i=0;i<3;i++)
		{
			if (pBlast->pArchetype[i])
			{
				IBaseObject *child = ARCHLIST->CreateInstance(pBlast->pArchetype[i]);
				if (child)
					obj->AddEffect(child);
			}
		}

		obj->EditorInitBlast();

		return obj;
	}
	
	return 0;
}
//--------------------------------------------------------------------------
//
void BlastManager::EditorCreateInstance(HANDLE hArchetype, const MISSION_INFO & info)
{
	// not supported
	BlastArchetype * objtype = (BlastArchetype *)hArchetype;
	EditorStartObjectInsertion(objtype->pArch, info);
}
//----------------------------------------------------------------------------------------------
//
struct IBaseObject * __stdcall CreateBlast (PARCHETYPE pArchetype, const class TRANSFORM &orientation, U32 systemID,SINGLE animScale)
{
	IBaseObject *result=0;

	if (pArchetype == 0)
		goto Done;
	
 	result = ARCHLIST->CreateInstance(pArchetype);
 	if (result)
 	{
		OBJPTR<IBlast> blast;

		result->QueryInterface(IBlastID, blast);
		CQASSERT(blast!=0);
 		blast->InitBlast(orientation,systemID,NULL,animScale);
 	}

Done:

	return result;
}
//----------------------------------------------------------------------------------------------
//
struct _bing : GlobalComponent
{
	struct BlastManager *blastMgr;

	virtual void Startup (void)
	{
		blastMgr = new DAComponent<BlastManager>;
		AddToGlobalCleanupList((IDAComponent **) &blastMgr);
	}

	virtual void Initialize (void)
	{
		blastMgr->init();
	}
};

static _bing bing;
//----------------------------------------------------------------------------------
//----------------------------------------------------------------------------------

//---------------------------------------------------------------------------
//--------------------------End Blast.cpp------------------------------------
//---------------------------------------------------------------------------
