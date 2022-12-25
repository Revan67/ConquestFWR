#ifndef TOBJEFFECTTARGET_H
#define TOBJEFFECTTARGET_H
//--------------------------------------------------------------------------//
//                                                                          //
//                              TObjEffectTarget.h                          //
//                                                                          //
//                  COPYRIGHT (C) 2004 WARTHOG TX, INC.                     //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/TObjCloak.h 35    9/13/01 10:01a Tmauer $
*/			    
//------------------------------- #INCLUDES --------------------------------//
//--------------------------------------------------------------------------//

#include "EffectPlayer.h"
#include <IMeshManager.h>

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
#define ObjectEffectTarget _Cet

struct EffectListenerNode
{
	EffectListenerNode * next;
	IEffectInstance * listener;
};

template <class Base=IBaseObject> 
struct _NO_VTABLE ObjectEffectTarget : public Base, IEffectTarget
{
	typename typedef Base::INITINFO EFFTARGINITINFO;

	struct InitNode       initNode;
//	struct PhysUpdateNode	physUpdateNode;

	EffectListenerNode * listenerList;

	SINGLE thrustParam;
	
	ObjectEffectTarget (void);
	~ObjectEffectTarget (void);

	/* IEffectTarget methods */
	virtual void PlayAnimation(const char * animName, bool bLooping);

	virtual SINGLE QueryEffectParam(const char* targetEffectParam);

	virtual void AddToEffectListeners(struct IEffectInstance * inst);

	virtual void RemoveFromEffectListeners(struct IEffectInstance * inst);

	virtual U32 GetHardPointIndex(const char * hpName);

	virtual void EffectSystemEvent(U32 context, const char * eventString, DWORD gameEventFlags);

	virtual void PositionCallback(const char * string, Transform & trans);

	virtual void EffectSystemHide(U32 context, bool bSetting);

	virtual SINGLE GetAnimationLength(const char * animName);

	virtual SINGLE GetAnmationCueTime(const char * animName, const char * cue);

	virtual IMeshInstance * GetMesh();

	virtual void TriggerGameEvent(const char * eventName);

	/* IMeshCallback */
	virtual void AnimationCue(struct IMeshInstance * meshInstance, const char * cueName);

	/* ObjectEffectTarget methods */
	void initEffectTarg (const EFFTARGINITINFO & data);

//	void physUpdateEffectTarg (SINGLE dt);

};

//---------------------------------------------------------------------------
//
template <class Base> 
ObjectEffectTarget< Base >::ObjectEffectTarget (void) :
					initNode(this, InitProc(&ObjectEffectTarget::initEffectTarg))
//					physUpdateNode(this, PhysUpdateProc(physUpdateEffectTarg))
{
	listenerList = NULL;
	thrustParam = 0;
}

template <class Base> 
ObjectEffectTarget< Base >::~ObjectEffectTarget (void) 
{
	while(listenerList)
	{
		EffectListenerNode * tmp = listenerList;
		listenerList = listenerList->next;
		delete tmp;
	}
}

//---------------------------------------------------------------------------
//
template <class Base>
void ObjectEffectTarget< Base >::PlayAnimation(const char * animName, bool bLooping)
{
	if(instanceMesh)
		instanceMesh->PlayAnimation(animName,bLooping);
}

//---------------------------------------------------------------------------
//
#define EP_HULL "Hull"
#define EP_SUPPLIES "Supplies"
#define EP_INSUPPLY "In Supply"
#define EP_SELECTED "Selected"
#define EP_HIGHLIGHT "Highlight"
#define EP_THRUST "Thrust"
#define EP_ENGINE_POWER "Engine Power"

template <class Base>
SINGLE ObjectEffectTarget< Base >::QueryEffectParam(const char* targetEffectParam)
{
	if(strcmp(targetEffectParam,EP_HULL) == 0)
	{
		return hullPoints/hullPointsMax;
	}
	else if(strcmp(targetEffectParam,EP_SUPPLIES) == 0)
	{
		return supplies/supplyPointsMax;
	}
	else if(strcmp(targetEffectParam,EP_INSUPPLY) == 0)
	{
		if(SECTOR->SystemInSupply(systemID,playerID))
			return 1.0f;
		return 0.0f;
	}
	else if(strcmp(targetEffectParam,EP_SELECTED) == 0)
	{
		if(bSelected)
			return 1.0f;
		return 0.0f;
	}
	else if(strcmp(targetEffectParam,EP_HIGHLIGHT) == 0)
	{
		if(bHighlight)
			return 1.0f;
		return 0.0f;
	}
	else if(strcmp(targetEffectParam,EP_THRUST) == 0)
	{
		return thrustParam;
	}
	else if(strcmp(targetEffectParam,EP_ENGINE_POWER) == 0)
	{
		return techLevel.engine;
	}
	return 0.0f;
}

//---------------------------------------------------------------------------
//
template <class Base>
void ObjectEffectTarget< Base >::AddToEffectListeners(struct IEffectInstance * inst)
{
	EffectListenerNode * node = new EffectListenerNode;
	node->listener = inst;
	node->next = listenerList;
	listenerList = node;
}

//---------------------------------------------------------------------------
//
template <class Base>
void ObjectEffectTarget< Base >::RemoveFromEffectListeners(struct IEffectInstance * inst)
{
	EffectListenerNode * search = listenerList;
	EffectListenerNode * prev = NULL;
	while(search)
	{
		if(search->listener == inst)
		{
			if(prev)
				prev->next = search->next;
			else 
				listenerList = search->next;
			delete search;
			return;
		}
		prev = search;
		search = search->next;
	}
}

//---------------------------------------------------------------------------
//
template <class Base>
U32 ObjectEffectTarget< Base >::GetHardPointIndex(const char * hpName)
{
	if(instanceMesh)
		return instanceMesh->GetArchtype()->FindHardPointIndex(hpName);
	return INVALID_HARD_POINT;
}

//---------------------------------------------------------------------------
//
template <class Base>
void ObjectEffectTarget< Base >::EffectSystemEvent(U32 context, const char * eventString, DWORD gameEventFlags)
{
}

//---------------------------------------------------------------------------
//
template <class Base>
void ObjectEffectTarget< Base >::PositionCallback(const char * string, Transform & trans)
{
}

//---------------------------------------------------------------------------
//
template <class Base>
void ObjectEffectTarget< Base >::EffectSystemHide(U32 context, bool bSetting)
{
	bSpecialRender = bSetting;
}

//---------------------------------------------------------------------------
//
template <class Base>
SINGLE ObjectEffectTarget< Base >::GetAnimationLength(const char * animName)
{
	return 0;
}

//---------------------------------------------------------------------------
//
template <class Base>
SINGLE ObjectEffectTarget< Base >::GetAnmationCueTime(const char * animName, const char * cue)
{
	return 0;
}

//---------------------------------------------------------------------------
//
template <class Base>
IMeshInstance * ObjectEffectTarget< Base >::GetMesh()
{
	return instanceMesh;
}

//---------------------------------------------------------------------------
//
template <class Base>
void ObjectEffectTarget< Base >::AnimationCue(struct IMeshInstance * meshInstance, const char * cueName)
{
	EffectListenerNode * search = listenerList;
	while(search)
	{
		search->listener->AnimationEvent(this,cueName);
		search = search->next;
	}
}

//---------------------------------------------------------------------------
//
template <class Base>
void ObjectEffectTarget< Base >::TriggerGameEvent(const char * eventName)
{
	EffectListenerNode * search = listenerList;
	while(search)
	{
		search->listener->GameEvent(this,eventName);
		search = search->next;
	}
}

//---------------------------------------------------------------------------
//
template <class Base>
void ObjectEffectTarget< Base >::initEffectTarg (const EFFTARGINITINFO & data)
{
}
//---------------------------------------------------------------------------
//
//template <class Base>
//void ObjectEffectTarget< Base >::physUpdateEffectTarg (SINGLE dt)
//{
//}
//---------------------------------------------------------------------------
//---------------------------End TObjCloak.h---------------------------------
//---------------------------------------------------------------------------
#endif