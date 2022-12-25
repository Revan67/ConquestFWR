//--------------------------------------------------------------------------//
//                                                                          //
//                                 UIAnim.cpp                                //
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


struct UIAnimArchetype
{
	const char *name;
	BT_UI_ANIM *data;
	PARCHETYPE pArchetype;
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

	UIAnimArchetype (void)
	{
		archIndex = -1;
		meshArch = NULL;
	}

	~UIAnimArchetype (void)
	{
		if (pArchetype)
			ARCHLIST->Release(pArchetype, OBJREFNAME);
	}

};

struct DUMMY_SAVESTRUCT			// required by template framework
{
	TRANS_SAVELOAD trans_SL;
};
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//

struct _NO_VTABLE UIAnim : public ObjectTransform<ObjectFrame<IBaseObject,struct DUMMY_SAVESTRUCT,UIAnimArchetype> >, IBlast
{

	BEGIN_MAP_INBOUND(UIAnim)
	_INTERFACE_ENTRY(IBaseObject)
	_INTERFACE_ENTRY(IBlast)
	_INTERFACE_ENTRY(IPhysicalObject)
	END_MAP()

	//------------------------------------------

	SINGLE timeToLive,totalTime;
	UIAnimArchetype *arch;
	HSOUND hSound;
	BT_UI_ANIM *data;
	S32 growSpeed;
	SINGLE animScale;
	U32 cnt;
	U32 systemID;
	U32 ownerID;
	OBJPTR<IBaseObject> owner;
	TRANSFORM trans;
	IBaseObject *animObj;
	SINGLE ui_animRadius;
	bool bInfinite:1;

	//------------------------------------------

	UIAnim (void)// : point(0)
	{
		timeToLive = 10;
		ui_animRadius = -1;
	}

    void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}

	virtual ~UIAnim (void);	// See ObjList.cpp

	/* IBaseObject methods */

	virtual void TestVisible (const USER_DEFAULTS & defaults, const U32 currentSystem, const U32 currentPlayer);

	virtual BOOL32 Update (void);

	virtual void PhysicalUpdate (SINGLE dt)
	{
		FRAME_physicalUpdate(dt);
		animObj->PhysicalUpdate(dt);
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

	BOOL32 EditorInitUIAnim();

	virtual void GetFireball(OBJPTR<IBaseObject> & pInterface) {}

	virtual void SetVisible (bool _bVisible)
	{
		bVisible = _bVisible;
	}

	virtual void SetRelativeTransform (const class TRANSFORM &_trans);

	bool UIAnim::get_projected_bounding_sphere(	float & cx,
										  float & cy,
										  float & radius,
										  float & depth);

	//IPhysicalObject

	virtual void SetSystemID (U32 newSystemID)
	{
		systemID = newSystemID;
	}

	virtual void SetPosition (const Vector & position, U32 newSystemID);

	virtual void SetTransform (const TRANSFORM & transform, U32 newSystemID);
};

//----------------------------------------------------------------------------------
//
UIAnim::~UIAnim (void)
{
	if (hSound)
		SFXMANAGER->CloseHandle(hSound);

	delete animObj;
}
//----------------------------------------------------------------------------------
//
void UIAnim::TestVisible (const USER_DEFAULTS & defaults, const U32 currentSystem, const U32 currentPlayer)
{
	float depth, center_x, center_y, radius;
	
	if (owner)
	{
		TRANSFORM ownerTrans = owner->GetTransform();
		transform = ownerTrans.multiply(trans);
	}
	
	bVisible = (GetSystemID() == currentSystem) && get_projected_bounding_sphere(center_x, center_y, radius, depth);
}
//----------------------------------------------------------------------------------
//
BOOL32 UIAnim::Update (void)
{
	if (bInfinite)
		return 1;

	CQASSERT(totalTime);
	BOOL32 allDone = TRUE;
	
	if (animObj->Update())
		allDone = FALSE;

	timeToLive -= (float)ELAPSED_TIME;

	return ((timeToLive > 0) || !allDone);
}
//----------------------------------------------------------------------------------
//
void UIAnim::Render (void)
{
	if (bVisible)
	{
		bHighlight = 0;
		BATCH->set_render_state(D3DRS_ZENABLE,FALSE);
		if (owner)
		{
			TRANSFORM ownerTrans = owner->GetTransform();
			transform = ownerTrans.multiply(trans);
		}
		
		
		animObj->Render();
	}
}
//----------------------------------------------------------------------------------
//
BOOL32 UIAnim::InitBlast (const class TRANSFORM & orientation, U32 _systemID, IBaseObject *_owner,SINGLE _animScale,SINGLE activationTime)
{
	BOOL32 result = 0;

	if (_owner)
	{
		_owner->QueryInterface(IBaseObjectID, owner, SYSVOLATILEPTR);
	}
	else
	{
		owner = NULL;
	}

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
		transform = orientation;
		transform.translation = pos;

		trans = transform;

		objClass = OC_UI_ANIM;
		if ((timeToLive = totalTime = data->totalTime) == 0)
			timeToLive = totalTime = 1;
		CQASSERT(totalTime);
	
		hSound = SFXMANAGER->Open(data->sfx);

		SFXMANAGER->Play(hSound, systemID, &pos);

		OBJPTR<IEffect> effect;
		animObj->QueryInterface(IEffectID,effect);
		CQASSERT(effect);
		effect->InitEffect(this,orientation,_animScale,totalTime);
		ui_animRadius = max(ui_animRadius,effect->GetRadius());

		result = 1;
	}

	animScale = _animScale;


	return result;

}

//----------------------------------------------------------------------------------
//
BOOL32 UIAnim::EditorInitUIAnim ()
{
	BOOL32 result = 0;

	CQASSERT(data);
	if (data)
	{
		trans = transform;

		objClass = OC_UI_ANIM;
		if ((timeToLive = totalTime = data->totalTime) == 0)
		{
			bInfinite = true;
		}
		CQASSERT(totalTime);
		
		hSound = SFXMANAGER->Open(data->sfx);
		
		OBJPTR<IEffect> effect;
		animObj->QueryInterface(IEffectID,effect);
		CQASSERT(effect);
		effect->EditorInitEffect(this,totalTime);
		ui_animRadius = max(ui_animRadius,effect->GetRadius());

		result = 1;
	}

	animScale = 1;


	return result;

}

void UIAnim::SetRelativeTransform(const TRANSFORM &_trans)
{
	trans = _trans;

}

bool UIAnim::get_projected_bounding_sphere(	float & cx,
										  float & cy,
										  float & radius,
										  float & depth)
{
	bool result = false;
//	CQASSERT(ui_animRadius != -1);

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
		
		if (center_distance >= ui_animRadius)
		{
			float dx = fabs(cx - screen_center_x);
			float dy = fabs(cy - screen_center_y);

			float outer_angle = asin(ui_animRadius / center_distance);
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

void UIAnim::SetPosition (const Vector & position, U32 newSystemID)
{
	systemID = newSystemID;
	trans.translation = position;
	if (owner == 0)
		transform.translation = position;
}

void UIAnim::SetTransform (const TRANSFORM &_trans, U32 newSystemID)
{
	systemID = newSystemID;
	trans = _trans;
	if (owner == 0)
		transform = trans;

}
//----------------------------------------------------------------------------------
//---------------------------------UIAnim Factory------------------------------------
//----------------------------------------------------------------------------------
//----------------------------------------------------------------------------------


struct DACOM_NO_VTABLE UIAnimManager : public IObjectFactory
{
	//
	// incoming interface map
	//
  
	BEGIN_DACOM_MAP_INBOUND(UIAnimManager)
	DACOM_INTERFACE_ENTRY(IObjectFactory)
	END_DACOM_MAP()

//	struct UIAnimNode *explosionList;
	U32 factoryHandle;

	//UIAnimManager methods

	UIAnimManager (void) 
	{
	}

	~UIAnimManager();
	
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
// UIAnimManager methods

UIAnimManager::~UIAnimManager()
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
void UIAnimManager::init()
{
	COMPTR<IDAConnectionPoint> connection;


	if (OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
	{
		connection->Advise(GetBase(), &factoryHandle);
	}
}
//--------------------------------------------------------------------------
//
HANDLE UIAnimManager::CreateArchetype(const char *szArchname, OBJCLASS objClass, void *data)
{
	if (objClass == OC_UI_ANIM)
	{
		UIAnimArchetype *newguy = new UIAnimArchetype;
		newguy->name = szArchname;
		newguy->data = (BT_UI_ANIM *)data;
		newguy->pArch = ARCHLIST->GetArchetype(szArchname);
		
		
		{
			HSOUND hSound = SFXMANAGER->Open(newguy->data->sfx);	// force the sfx to load early
			SFXMANAGER->CloseHandle(hSound);
		}
		
		if (newguy->data->effectType[0])
		{
			newguy->pArchetype = ARCHLIST->LoadArchetype(newguy->data->effectType);
			if (newguy->pArchetype)
				ARCHLIST->AddRef(newguy->pArchetype, OBJREFNAME);
			else
				CQTRACE11("Effect not loaded %s",newguy->data->effectType);
		}

		return newguy;
	}
	
	return 0;
}
//--------------------------------------------------------------------------
//
BOOL32 UIAnimManager::DestroyArchetype(HANDLE hArchetype)
{
	UIAnimArchetype *deadguy = (UIAnimArchetype *)hArchetype;

	delete deadguy;
	return 1;
}
//--------------------------------------------------------------------------
//
IBaseObject * UIAnimManager::CreateInstance(HANDLE hArchetype)
{
	UIAnimArchetype *pUIAnim = (UIAnimArchetype *)hArchetype;
	BT_UI_ANIM *objData = ((UIAnimArchetype *)hArchetype)->data;
	
	if (objData->objClass == OC_UI_ANIM)
	{
		UIAnim * obj = new ObjectImpl<UIAnim>;
		obj->objClass = OC_UI_ANIM;
		obj->FRAME_init(*pUIAnim);

		obj->data = objData;
		obj->arch = pUIAnim;
		obj->totalTime = objData->totalTime;

		if (pUIAnim->pArchetype)
		{
			IBaseObject *child = ARCHLIST->CreateInstance(pUIAnim->pArchetype);
			if (child)
				obj->animObj = child;
		}

		obj->EditorInitUIAnim();

		return obj;
	}
	
	return 0;
}
//--------------------------------------------------------------------------
//
void UIAnimManager::EditorCreateInstance(HANDLE hArchetype, const MISSION_INFO & info)
{
	// not supported
	UIAnimArchetype * objtype = (UIAnimArchetype *)hArchetype;
	EditorStartObjectInsertion(objtype->pArch, info);
}
//----------------------------------------------------------------------------------------------
//
struct IBaseObject * __stdcall CreateUIAnim (PARCHETYPE pArchetype, const class TRANSFORM &orientation, U32 systemID,SINGLE animScale)
{
	IBaseObject *result=0;

	if (pArchetype == 0)
		goto Done;
	
 	result = ARCHLIST->CreateInstance(pArchetype);
 	if (result)
 	{
		OBJPTR<IBlast> ui_anim;

		result->QueryInterface(IBlastID, ui_anim);
		CQASSERT(ui_anim!=0);
 		ui_anim->InitBlast(orientation,systemID,NULL,animScale);
 	}

Done:

	return result;
}
//----------------------------------------------------------------------------------------------
//
struct _anmbing : GlobalComponent
{
	struct UIAnimManager *ui_animMgr;

	virtual void Startup (void)
	{
		ui_animMgr = new DAComponent<UIAnimManager>;
		AddToGlobalCleanupList((IDAComponent **) &ui_animMgr);
	}

	virtual void Initialize (void)
	{
		ui_animMgr->init();
	}
};

static _anmbing anmbing;
//----------------------------------------------------------------------------------
//----------------------------------------------------------------------------------

//---------------------------------------------------------------------------
//--------------------------End UIAnim.cpp------------------------------------
//---------------------------------------------------------------------------
