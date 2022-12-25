//--------------------------------------------------------------------------//
//                                                                          //
//                               EffectPlayer.cpp                           //
//                                                                          //
//                  COPYRIGHT (C) 2003 by Fever Pitch Studios, INC.         //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Author: Jthomas $

    $Header: /App/Src/EffectPlayer.cpp 27    11/24/03 3:50p Jthomas $
*/			    
//---------------------------------------------------------------------------
#include "pch.h"
#include <globals.h>

#include <IParticleManager.h>

#include "EffectPlayer.h"
#include "IObject.h"
#include "startup.h"
#include "objwatch.h"
#include "Camera.h"
#include "Sector.h"
#include "Objlist.h"

#include <DACOM.h>
#include <TSmartPointer.h>
#include <TConnPoint.h>
#include <TConnContainer.h>
#include <IHardpoint.h>
#include <IMeshManager.h>


//////////////////////////////////////////////////////////////////////////////////
// Save structures
////////////////////////////////////////////////////////////////////////
#define EFFECT_SAVE_VERSION 1 //should only need to change this if a structure changes size

struct EventSave
{
	enum EventType // always add to the end of this list, NEVER remove a value once it is in a version
	{
		START_TRIGGERED = 0,
		ANIM_CUE_TRIGGERED,
		ANIM_FINISHED,
		PARTICLE_CUE_TRIGGERED,
		SWITCH,
		LISTEN_EVENT,
	};
	EventType type;
	char stingParam[64];//the name of the cue... etc.
	U32 targetID;
	S32 xPos;
	S32 yPos;
};

#define ACTION_SAVE_VERSION 3 //should only need to change this if a structure changes size

struct ActionSaveHeader
{
	U32 version;
	enum ActionType
	{
		AT_ANIM = 0,
		AT_PARTICLE,
		AT_GAME_EVENT,
		AT_SOUND,
		AT_SWITCH,
		AT_HIDE_TARGET,
		AT_LISTEN,
		AT_JOINT_TRACK,
	};
	ActionType actionType;
};

struct ActionJointTrackSave
{
	U32 sourceID;
	U32 targetID;
	SINGLE angVelocity;
	char actionName[64];
	char jointName[64];
	S32 xPos;
	S32 yPos;
};

struct ActionListenSave
{
	U32 targetID;
	char actionName[64];
	char listenName[64];
	S32 xPos;
	S32 yPos;
};

struct ActionHideTargetSave
{
	U32 targetID;
	char actionName[64];
	bool bHide:1;
	S32 xPos;
	S32 yPos;
};

struct ActionAnimSave
{
	U32 targetID;
	char animName[64];
	char actionName[64];
	bool bLooping:1;
	bool bForced:1;
	U32 attachID;
	S32 xPos;
	S32 yPos;
};

struct ActionParticleSave
{
	char actionName[64];
	S32 xPos;
	S32 yPos;
};

struct ActionGameEventSave
{
	U32 targetID;
	char actionName[64];
	char eventName[64];
	bool bResponse:1;
	bool bDistanceDependant:1;
	bool bEndEffect:1;
	float speed;
	S32 xPos;
	S32 yPos;
};

struct ActionSoundSave
{
	U32 targetID;
	char hpName[64];
	char soundName[64];
	char actionName[64];
	bool bLooping;
	SINGLE minRange;
	SINGLE maxRange;
	S32 xPos;
	S32 yPos;
};

struct ActionSwitchSave
{
	char actionName[64];
	U32 numEvents;
	S32 xPos;
	S32 yPos;
};

#define FILTER_SAVE_VERSION 1 //should only need to change this if a structure changes size

struct FilterSaveHeader
{
	U32 version;
	enum ParticleEffectType type;
	U32 dataSize;
	U32 numOutput;
	U32 numInput;
	S32 xPos;
	S32 yPos;
	U32 linkID;
};

///////////////////////////////////////////////////
//event and action templates
///////////////////////////////////////////////////
struct ActionNode;

struct EventNode
{
	EventNode * next;
	ActionNode * firstAction;

	EventSave info;

	virtual ~EventNode();

	virtual bool GetTotalPlayTime(SINGLE & playTime,IBaseObject ** targets, U32 numTargets);

	virtual bool GetTimeToReactionCue(SINGLE & playTime, U32 targetID, IBaseObject ** targets, U32 numTargets);
};

struct FilterNode
{
	FilterNode * next;

	FilterSaveHeader info;

	FilterNode ** output;
	U32 * outputTargetID;

	FilterNode ** input;
	U32 * inputTargetID;

	U8 * data;
	virtual ~FilterNode()
	{
		delete [] output;
		delete [] outputTargetID;
		delete [] input;
		delete [] inputTargetID;
		delete [] data;
		delete next;
	}

	bool IsRoot()
	{
		for(U32 i = 0; i < info.numInput; ++i)
		{
			if(input[i])
				return false;
		}
		return true;
	}
};

struct ActionNode
{
	ActionNode * next;
	EventNode * firstEvent;

	ActionSaveHeader info;
	union 
	{
		ActionAnimSave animInfo;
		ActionParticleSave particleSave;
		ActionGameEventSave gameEventInfo;
		ActionSoundSave soundInfo;
		ActionSwitchSave switchInfo;
		ActionHideTargetSave hideInfo;
		ActionListenSave listenInfo;
		ActionJointTrackSave jointTrackInfo;
	};
	union
	{
		FilterNode * filterList;
	};

	virtual ~ActionNode()
	{
		delete firstEvent;
		delete next;
		if(info.actionType == ActionSaveHeader::AT_PARTICLE)
		{
			delete filterList;
		}
	}

	virtual bool GetTotalPlayTime(SINGLE & playTime,IBaseObject ** targets, U32 numTargets);

	virtual bool GetTimeToReactionCue(SINGLE & playTime, U32 targetID, IBaseObject ** targets, U32 numTargets);
};

bool ActionNode::GetTotalPlayTime(SINGLE & playTime,IBaseObject ** targets, U32 numTargets)
{
	if(info.actionType == ActionSaveHeader::AT_GAME_EVENT && gameEventInfo.bEndEffect)
	{
		playTime = 0;
		return true;
	}

	EventNode * search = firstEvent;
	while(search)
	{
		if(search->GetTotalPlayTime(playTime,targets,numTargets))
		{
			if(info.actionType == ActionSaveHeader::AT_ANIM)
			{
				if(numTargets > animInfo.targetID)
				{
					VOLPTR(IEffectTarget) targ = targets[animInfo.targetID];
					if(targ)
					{
						if(search->info.type == EventSave::ANIM_FINISHED)
						{
							playTime += targ->GetAnimationLength(animInfo.animName);
						}
						else if(search->info.type == EventSave::ANIM_CUE_TRIGGERED)
						{
							playTime += targ->GetAnmationCueTime(animInfo.animName,search->info.stingParam);
						}
					}
				}
			}
			return true;
		}
		search = search->next;
	}

	return false;
}

bool ActionNode::GetTimeToReactionCue(SINGLE & playTime, U32 targetID, IBaseObject ** targets, U32 numTargets)
{
	if(info.actionType == ActionSaveHeader::AT_GAME_EVENT && gameEventInfo.bResponse && gameEventInfo.targetID == targetID)
	{
		if(gameEventInfo.bDistanceDependant)
		{
			//assumes a parent did a linear motion to hit the target
			if(targets[0] && targets[targetID])
			{
				Vector dist = targets[0]->GetPosition()-targets[targetID]->GetPosition();
				playTime = ((dist.fast_magnitude()))/gameEventInfo.speed;
			}
		}
		else
		{
			playTime = 0;
		}
		return true;
	}

	EventNode * search = firstEvent;
	while(search)
	{
		if(search->GetTimeToReactionCue(playTime,targetID,targets,numTargets))
		{
			if(info.actionType == ActionSaveHeader::AT_ANIM)
			{
				if(numTargets > animInfo.targetID)
				{
					VOLPTR(IEffectTarget) targ = targets[animInfo.targetID];
					if(targ)
					{
						if(search->info.type == EventSave::ANIM_FINISHED)
						{
							playTime += targ->GetAnimationLength(animInfo.animName);
						}
						else if(search->info.type == EventSave::ANIM_CUE_TRIGGERED)
						{
							playTime += targ->GetAnmationCueTime(animInfo.animName,search->info.stingParam);
						}
					}
				}
			}
			return true;
		}
		search = search->next;
	}

	return false;
}


EventNode::~EventNode()
{
	delete firstAction;
	delete next;
}

bool EventNode::GetTotalPlayTime(SINGLE & playTime, IBaseObject ** targets, U32 numTargets)
{
	ActionNode * search = firstAction;
	while(search)
	{
		if(search->GetTotalPlayTime(playTime,targets,numTargets))
			return true;
		search =search->next;
	}
	return 0;
}

bool EventNode::GetTimeToReactionCue(SINGLE & playTime, U32 targetID, IBaseObject ** targets, U32 numTargets)
{
	ActionNode * search = firstAction;
	while(search)
	{
		if(search->GetTimeToReactionCue(playTime,targetID,targets,numTargets))
			return true;
		search =search->next;
	}
	return 0;
}



///////////////////////////////////////////////////
//internal interfaces
///////////////////////////////////////////////////

struct IInternalEffectPlayer
{
	virtual void AddInstance(struct EffectInstance * inst) = 0;

	virtual void AddParticleInst(IParticleInstance * inst, ActionNode * aNode, struct EffectInstance * effInst) = 0;
};

IInternalEffectPlayer * internalPlayer;

struct IInternalEffectHandle
{
	virtual EventNode * GetStartNode() = 0;
};

///////////////////////////////////////////////////

struct AnimListener
{
	AnimListener * next;
	EventNode * event;
	bool bAnimVolatile;//if true delete this if the animation ends or changes
};

struct EventListener
{
	EventListener * next;
	ActionNode * action;
};


struct TargetNode
{
	TargetNode * next;
	U32 id;
	DWORD context;
	OBJPTR<IEffectTarget> target;
	
	Vector position;
	bool bPositionOnly;

	AnimListener * animListener;
	EventListener * eventListener;

	TargetNode()
	{
		bPositionOnly = false;
		animListener = NULL;
		eventListener = NULL;
	}

	virtual ~TargetNode()
	{
		while(animListener)
		{
			AnimListener * tmp = animListener;
			animListener = animListener->next;
			delete tmp;
		}
		while(eventListener)
		{
			EventListener * tmp = eventListener;
			eventListener = eventListener->next;
			delete tmp;
		}
	}
};

//--------------------------------------------------------------------------//
//----------------------------EffectInstance Class--------------------------//
//--------------------------------------------------------------------------//

static int s_NextEffectInstanceUniqueId = 1;

struct UpdateTurretNode
{
	UpdateTurretNode * next;
	U32 targetID;
	U32 sourceID;
	INSTANCE_INDEX jointIndex;
	SINGLE angVelocity;
};

struct EffectInstance : public IEffectInstance
{
	EffectInstance * nextInstance;

	TargetNode * targetList;

	IInternalEffectHandle * effectHandle;

	EffectInstance(IInternalEffectHandle * base);

	U32 lifeCount;//when this hits zero durring an update I go away.

	U32 effectId;
	U32 systemID;

	IEffectCallback * callback;
	UpdateTurretNode * updateTurretList;

	bool bEnabled;
	bool bStoped;
	bool bMovieEffect;

	OBJPTR<IBaseObject> systemTarget;

/*	struct SoundInstEntry
	{
		SoundInstEntry * next;
		SoundInstance inst;
	};
	SoundInstEntry * soundInstList;
*/
	//IEffectInstance
	virtual ~EffectInstance();

	virtual void SetTarget(IBaseObject * obj, U32 id, DWORD context);

	virtual void SetTargetPositon(const Vector & pos, U32 id, DWORD context);

	virtual void SetSystemID(U32 systemID);

	virtual void SetSystemID(IBaseObject * sysObj);

	virtual U32 GetSystemID();

	virtual void TriggerStartEvent();

	virtual void AnimationEvent(IBaseObject * obj, const char * eventName);

	virtual void AnimationCompleted(IBaseObject * obj);

	virtual void GameEvent(IBaseObject * obj, const char * eventName);

	virtual void Enable();

	virtual void Disable();

	virtual void Stop();

	virtual U32 GetEffectId( void )
	{
		return effectId;
	}

	virtual BOOL32 ValidateTargets();

	virtual IEffectTarget* GetTarget(U32 id, DWORD& context);

	//EffectInastance
	void particleEvent(ActionNode * node, const char * eventName);

	void activateEvent(EventNode * node);

	void createAnimListener(EventNode * node);

	void activateAnimation(ActionNode * node);

	void activateParticle(ActionNode * node);

	void activateGameEvent(ActionNode * node);

	void activateSound(ActionNode * node);

	void activateSwitch(ActionNode * node);

	void activateHideTarget(ActionNode * node);
	
	void activateListen(ActionNode * node);

	void activateJointTrack(ActionNode * node);

	void addFilterToInst(IParticleInstance * inst, FilterNode * filter, IParticleEffectInstance * parentEff, U32 outputID, U32 inputID);

	TargetNode * findTarget(U32 id);

	bool update();

	void updateTurrets();

	void forcePlayCanceled(IBaseObject * obj);

//	void addSoundInst(SoundInstance sInst);
};
//--------------------------------------------------------------------------//
//
EffectInstance::EffectInstance(IInternalEffectHandle * base)
{
	effectHandle = base;
	targetList = NULL;
//	soundInstList = NULL;
	bEnabled = true;
	bStoped = false;
	bMovieEffect = false;
	callback = NULL;
	lifeCount = 1;
	systemID = 0;
	updateTurretList = NULL;
}
//--------------------------------------------------------------------------//
//
EffectInstance::~EffectInstance()
{
/*	while(soundInstList)
	{
		SoundInstEntry * tmp = soundInstList;
		soundInstList = soundInstList->next;
		if(SOUNDSYS)
			SOUNDSYS->Stop(tmp->inst);
		delete tmp;
	}
*/	
	while(updateTurretList)
	{
		UpdateTurretNode * tmp = updateTurretList;
		updateTurretList = updateTurretList->next;
		delete tmp;
	}
	while(targetList)
	{
		if(targetList->target)
			targetList->target->RemoveFromEffectListeners(this);

		TargetNode * tmp = targetList;
		targetList = targetList->next;
		delete tmp;
	}
}
//--------------------------------------------------------------------------//
//
void EffectInstance::SetTarget(IBaseObject * obj, U32 id, DWORD context)
{
	TargetNode * node = targetList;
	while(node)
	{
		if(node->id == id)
		{
			node->context = context;
			if(node->target)
				node->target->RemoveFromEffectListeners(this);
			node->bPositionOnly = false;
			if(obj)
			{
				obj->QueryInterface(IEffectTargetID,node->target,NONSYSVOLATILEPTR);
				if(node->target)
					node->target->AddToEffectListeners(this);
			}
			else
				node->target = NULL;
			return;
		}
		node = node->next;
	}
	
	node = new TargetNode;
	node->context = context;
	node->id = id;
	node->bPositionOnly = false;
	if(obj)
	{
		obj->QueryInterface(IEffectTargetID,node->target,NONSYSVOLATILEPTR);
		if(node->target)
			node->target->AddToEffectListeners(this);
	}
	else
		node->target = NULL;
	node->next = targetList;
	node->animListener = NULL;
	targetList = node;
}
//--------------------------------------------------------------------------//
//
void EffectInstance::SetTargetPositon(const Vector & pos, U32 id, DWORD context)
{
	TargetNode * node = targetList;
	while(node)
	{
		if(node->id == id)
		{
			if(node->target)
				node->target->RemoveFromEffectListeners(this);
			node->context = context;
			node->target.ptr = NULL;
			node->bPositionOnly = true;
			node->position = pos;
			return;
		}
		node = node->next;
	}
	
	node = new TargetNode;
	node->context = context;
	node->id = id;
	node->target = NULL;
	node->bPositionOnly = true;
	node->position = pos;
	node->next = targetList;
	node->animListener = NULL;
	targetList = node;
}
//--------------------------------------------------------------------------//
//
void EffectInstance::SetSystemID(U32 _systemID)
{
	systemID = _systemID;
}
//--------------------------------------------------------------------------//
//
void EffectInstance::SetSystemID(IBaseObject * sysObj)
{
	sysObj->QueryInterface(IBaseObjectID,systemTarget,NONSYSVOLATILEPTR);
}
//--------------------------------------------------------------------------//
//
U32 EffectInstance::GetSystemID()
{
	if(systemTarget)
		return systemTarget->GetSystemID();
	return systemID;
}
//--------------------------------------------------------------------------//
//
void EffectInstance::TriggerStartEvent()
{
	EFFECT_TRACE("   Starting Targets List\n");
#ifdef _DEBUG
	TargetNode * search = targetList;
	while(search)
	{
		if(search->bPositionOnly)
		{
			EFFECT_TRACE4("      Target:%d Location:(%f,%f,%f)\n",search->id,search->position.x,search->position.y,search->position.z);
		}
		else
		{
			if(search->target.ptr)
				EFFECT_TRACE2("      Target:%d UniqueID:%d\n",search->id,search->target.ptr->GetUniqueID());
		}
		search = search->next;
	}
#endif
	EFFECT_TRACE("   End Target List\n");

	EventNode * start = effectHandle->GetStartNode();
	if(start)
	{
		if(start->info.type == EventSave::START_TRIGGERED)
		{
			activateEvent(start);
		}
		else if(start->info.type == EventSave::ANIM_CUE_TRIGGERED)
		{
			createAnimListener(start);
		}
	}
}
//--------------------------------------------------------------------------//
//
void EffectInstance::activateAnimation(ActionNode * node)
{
	EFFECT_TRACE("Animation Action\n");
	EFFECT_TRACE1("   ID:%d \n",effectId);
	EFFECT_TRACE1("   Name:%s\n",node->animInfo.actionName);
	EFFECT_TRACE1("   Target:%d\n",node->animInfo.targetID);
	EFFECT_TRACE1("   Animation:%s\n",node->animInfo.animName);

	TargetNode * search = targetList;
	while(search)
	{
		if(search->id == node->animInfo.targetID)
		{
			AnimListener * animSearch = search->animListener;
			AnimListener * prev = NULL;
			while(animSearch)
			{
				if(animSearch->bAnimVolatile)
				{
					if(prev)
						prev->next = animSearch->next;
					else
						search->animListener = animSearch->next;
					--lifeCount;
					delete animSearch;
					if(prev)
						animSearch = prev->next;
					else
						animSearch = search->animListener;
				}
				else
				{
					prev = animSearch;
					animSearch = animSearch->next;
				}
			}
			bool bAddEvents = false;
			if(search->target)
			{
				bAddEvents = true;
				search->target->PlayAnimation(node->animInfo.animName,node->animInfo.bLooping);
			}
			if(bAddEvents)
			{
				EventNode * eNode = node->firstEvent;
				while(eNode)
				{
					if(eNode->info.type == EventSave::ANIM_CUE_TRIGGERED)
						createAnimListener(eNode);
					else if (eNode->info.type == EventSave::ANIM_FINISHED)
						createAnimListener(eNode);
					eNode = eNode->next;
				}
			}
			return;
		}
		search = search->next;
	}
}
//--------------------------------------------------------------------------//
//
void EffectInstance::activateParticle(ActionNode * node)
{
	EFFECT_TRACE("Particle Action\n");
	EFFECT_TRACE1("   ID:%d\n",effectId);
	EFFECT_TRACE1("   Name:%s\n",node->particleSave.actionName);
	
	IParticleInstance * inst = PARTMAN->CreateBaseParticleInstance();
	inst->SetContext((DWORD)this);
	FilterNode * filter = node->filterList;
	while(filter)
	{
		if(filter->IsRoot())
			addFilterToInst(inst,filter,NULL,0,0);
		filter = filter->next;
	}
	inst->Initialize((DWORD)this);
	internalPlayer->AddParticleInst(inst,node,this);
	
}
//--------------------------------------------------------------------------//
//
void EffectInstance::activateGameEvent(ActionNode * node)
{
	EFFECT_TRACE("GameEvent Action\n");
	EFFECT_TRACE1("   ID:%d\n",effectId);
	EFFECT_TRACE1("   Name:%s\n",node->gameEventInfo.actionName);
	EFFECT_TRACE1("   Target:%d\n",node->gameEventInfo.targetID);
	EFFECT_TRACE1("   EventString:%s\n",node->gameEventInfo.eventName);
	EFFECT_TRACE1("   Responce:%d\n",node->gameEventInfo.bResponse);
	EFFECT_TRACE1("   End:%d\n",node->gameEventInfo.bEndEffect);
	EFFECT_TRACE1("   Distance:%d\n",node->gameEventInfo.bDistanceDependant);
	EFFECT_TRACE1("   Speed:%f\n",node->gameEventInfo.speed);
	// apply flags to game event
	DWORD gameEventFlags = GEF_NONE;
	gameEventFlags |= node->gameEventInfo.bDistanceDependant ? GEF_DISTANCE     : GEF_NONE;
	gameEventFlags |= node->gameEventInfo.bEndEffect         ? GEF_EFFECT_END   : GEF_NONE;
	gameEventFlags |= node->gameEventInfo.bResponse          ? GEF_CUE_RESPONSE : GEF_NONE;

/*	if( node->gameEventInfo.eventName[0] == '@' )
	{
		const char* script = node->gameEventInfo.eventName + 1;
		
		// run a script for this event (if it exists)
		if( PYTHON->ScriptExists(script) )
		{
			PYTHON->RunScript( script );
		}
	}
*/
	TargetNode * search = findTarget(node->gameEventInfo.targetID);
	if(search)
	{
		if(search->target)
		{
			if(callback && node->gameEventInfo.bResponse)
			{
				callback->CueResponse(search->target.ptr,node->gameEventInfo.eventName,gameEventFlags,search->context);
			}
			else
			{
				search->target->EffectSystemEvent(search->context,node->gameEventInfo.eventName,gameEventFlags);
			}
		}
		else if(search->bPositionOnly && callback && node->gameEventInfo.bResponse)
		{
			callback->CuePositionResponse(search->position,node->gameEventInfo.eventName,gameEventFlags,search->context);
		}
/*		char * concus =strstr(node->gameEventInfo.eventName,"[concussion");
		if(concus)
		{
			SINGLE rad = atof(concus+11);
			if(rad > 0)
			{
				if(search->target)
					REACTOR->CreateEvent(search->target.ptr->GetPosition(),rad,REACTOR_CONCUSION);
				else if(search->bPositionOnly)
					REACTOR->CreateEvent(search->position,rad,REACTOR_CONCUSION);
			}
		}
*/	}
	if(node->gameEventInfo.bEndEffect)
	{
		if(callback)
		{
			callback->EffectInstanceEnded(effectId);
		}
	}
}
//--------------------------------------------------------------------------//
//
void EffectInstance::activateSound(ActionNode * node)
{
/*	EFFECT_TRACE("Sound Action\n");
	EFFECT_TRACE1("   ID:%d\n",effectId);
	EFFECT_TRACE1("   Name:%s\n",node->soundInfo.actionName);
	EFFECT_TRACE1("   Target:%d\n",node->soundInfo.targetID);
	EFFECT_TRACE1("   Sound:%s\n",node->soundInfo.soundName);
	if(node->soundInfo.soundName)
	{
		BT_SOUNDENTRY * entry = (BT_SOUNDENTRY *)(ARCHLIST->GetArchetypeData(node->soundInfo.soundName));
		if(entry)
		{
			SoundDef def;
			def.priority = SND_SFX_PRIORITY;
			def.dir = SFXDIR;
			STRNCPY(def.filename,entry->filename, 64);
			def.bStreaming = entry->bStreaming;
			def.bUse3D = true;

			SoundHandle sHandle = SOUNDSYS->LoadSound(def);
			if(sHandle)
			{
				IBaseObject * soundTarget = NULL;
				TargetNode * search = findTarget(node->gameEventInfo.targetID);
				if(search)
				{
					if(search->target)
					{
						soundTarget = search->target.ptr;
					}
				}
				
				if(soundTarget)
				{
					U32 hpIndex = INVALID_HARD_POINT;
					Transform trans;
					if(soundTarget->GetGrannyInstance())
					{
						soundTarget->GetGrannyInstance()->GetHardPointIndex(node->soundInfo.hpName);
						if(hpIndex != INVALID_HARD_POINT)
							soundTarget->GetGrannyInstance()->GetHardPointTransform(hpIndex,trans);
						else
							trans = soundTarget->GetTransform();
					}
					else
						trans = soundTarget->GetTransform();
					
					SoundInstance inst = SOUNDSYS->Play3D(sHandle,trans.translation,soundTarget->GetUniqueID(),hpIndex,node->soundInfo.bLooping);
					SOUNDSYS->SetMinMaxDist(inst,node->soundInfo.minRange,node->soundInfo.maxRange);
					if(node->soundInfo.bLooping)
						addSoundInst(inst);
				}
				else if(search && search->bPositionOnly)
				{
					SoundInstance inst = SOUNDSYS->Play3D(sHandle,search->position,0,0,node->soundInfo.bLooping);
					SOUNDSYS->SetMinMaxDist(inst,node->soundInfo.minRange,node->soundInfo.maxRange);
					if(node->soundInfo.bLooping)
						addSoundInst(inst);
				}
				else
				{
					SoundInstance inst =SOUNDSYS->Play(sHandle,node->soundInfo.bLooping);
					if(node->soundInfo.bLooping)
						addSoundInst(inst);
				}
				SOUNDSYS->ReleaseSound(sHandle);
			}
		}
	}*/
}
//--------------------------------------------------------------------------//
//
void EffectInstance::activateSwitch(ActionNode * node)
{
	EFFECT_TRACE("Switch Action\n");
	EFFECT_TRACE1("   ID:%d\n",effectId);
	EFFECT_TRACE1("   Name:%s\n",node->switchInfo.actionName);
	if(node->switchInfo.numEvents)
	{
		U32 num = rand()%node->switchInfo.numEvents;
		EventNode * eSearch = node->firstEvent;
		while(num && eSearch)
		{
			--num;
			eSearch = eSearch->next;
		}
		if(eSearch)
		{
			activateEvent(eSearch);
		}
	}
}
//--------------------------------------------------------------------------//
//
void EffectInstance::activateHideTarget(ActionNode * node)
{
	EFFECT_TRACE("Hide Action \n");
	EFFECT_TRACE1("   ID:%d\n",effectId);
	EFFECT_TRACE1("   Name:%s\n",node->hideInfo.actionName);
	EFFECT_TRACE1("   Target:%d\n",node->hideInfo.targetID);
	EFFECT_TRACE1("   Hide:%d\n",node->hideInfo.bHide);
	TargetNode * search = findTarget(node->hideInfo.targetID);
	if(search)
	{
		if(search->target)
		{
			search->target->EffectSystemHide(search->context,node->hideInfo.bHide);
		}
	}
}
//--------------------------------------------------------------------------//
//
void EffectInstance::activateListen(ActionNode * node)
{
	EFFECT_TRACE("LISTEN Action \n");
	EFFECT_TRACE1("   ID:%d\n",effectId);
	EFFECT_TRACE1("   Name:%s\n",node->listenInfo.actionName);
	EFFECT_TRACE1("   Target:%d\n",node->listenInfo.targetID);
	TargetNode * search = findTarget(node->listenInfo.targetID);
	if(search)
	{
		if(search->target)
		{
			//add to the targets event listener list
			EventListener * newListener = new EventListener;
			newListener->action = node;
			newListener->next = search->eventListener;
			search->eventListener = newListener;

			search->target->AddToEffectListeners(this);
		}
	}
}
//--------------------------------------------------------------------------//
//
void EffectInstance::activateJointTrack(ActionNode * node)
{
	EFFECT_TRACE("Joint Track Action \n");
	EFFECT_TRACE1("   ID:%d\n",effectId);
	EFFECT_TRACE1("   Name:%s\n",node->jointTrackInfo.actionName);
	TargetNode * search = findTarget(node->jointTrackInfo.sourceID);
	if(search)
	{
		if(search->target)
		{
			IMeshInstance * mesh = search->target->GetMesh();
			if(mesh)
			{
				INSTANCE_INDEX jointIndex = mesh->FindChild(node->jointTrackInfo.jointName);
				if(jointIndex != INVALID_INSTANCE_INDEX)
				{
					UpdateTurretNode * turret = new UpdateTurretNode;
					turret->next = updateTurretList;
					updateTurretList = turret;
					turret->sourceID = node->jointTrackInfo.sourceID;
					turret->targetID = node->jointTrackInfo.targetID;
					turret->angVelocity = node->jointTrackInfo.angVelocity;
					turret->jointIndex = jointIndex;
				}
			}
		}
	}
}
//--------------------------------------------------------------------------//
//
void EffectInstance::addFilterToInst(IParticleInstance * inst, FilterNode * filter, IParticleEffectInstance * parentEff, U32 outputID, U32 inputID)
{
	IParticleEffectInstance * effInst;
	if(parentEff)
		effInst = parentEff->AddFilter(outputID,inputID,filter->info.type,filter->data);
	else
		effInst = inst->AddFilter(filter->info.type,filter->data);

	for(U32 i = 0; i < filter->info.numOutput; ++i)
	{
		if(filter->output[i])
		{
			addFilterToInst(inst,filter->output[i],effInst,i,filter->outputTargetID[i]);	
		}
	}
}
//--------------------------------------------------------------------------//
//
TargetNode * EffectInstance::findTarget(U32 id)
{
	TargetNode * search = targetList;
	while(search)
	{
		if(search->id == id)
			return search;
		search = search->next;
	}
	return NULL;
}
//--------------------------------------------------------------------------//
//
bool EffectInstance::update()
{
	if(lifeCount == 0 || bStoped)
		return false;
	TargetNode * search = targetList;
	bool returnValue = false;
	while(search)
	{
		if(search->target || search->bPositionOnly)
		{
			returnValue= true;
			break;
		}
		search = search->next;
	}

	if(returnValue)
	{
		//update turrets
		updateTurrets();
	}

	return returnValue;
}
//--------------------------------------------------------------------------//
//
void EffectInstance::updateTurrets()
{
	UpdateTurretNode * search = updateTurretList;
	while(search)
	{
		TargetNode * source = findTarget(search->sourceID);
		TargetNode * target = findTarget(search->targetID);
		if(target && (target->target.Ptr() || target->bPositionOnly) && source && source->target.Ptr() && source->target.Ptr()->bVisible)
		{
			//compute target yaw
			SINGLE rot;
			TRANSFORM transform = ENGINE->get_transform(search->jointIndex);
			Vector pos = transform.get_position();
			SINGLE yaw = transform.get_yaw();
			Vector targetPos;
			if(target->target.Ptr())
				targetPos = target->target.Ptr()->GetTransform().translation;
			else
				targetPos = target->position;
			Vector goal = targetPos - pos;
			ENGINE->get_joint_state(search->jointIndex, IE_JST_BASIC, &rot);
			goal.z = 0;

			SINGLE relYaw = get_angle(goal.x,goal.y) - yaw;
			if (relYaw < -PI)
				relYaw += PI*2;
			else
			if (relYaw > PI)
				relYaw -= PI*2;

			//compute new yaw
			{
				//
				// is goal yaw in no-man's land?
				//
				SINGLE dt = OBJLIST->GetRealRenderTime();
				BOOL32 noman = 0;
				JointInfo const *jnt = ENGINE->get_joint_info(search->jointIndex);
				BOOL32 limited = (jnt->max0 - jnt->min0 < 359.0 * MUL_DEG_TO_RAD);
				SINGLE origRot = rot;

				if (limited)		// does joint really have a limit?
				{
					if (relYaw < 0)	// turn to the left needed
					{
						if (rot - relYaw > jnt->max0)
						{
							if (rot - (relYaw+PI*2) < jnt->min0)
								noman = 1;		// damned anyway
							else
								relYaw += PI*2;		// go the long way
						}

					}
					else // turn to the right needed
					{
						if (rot - relYaw < jnt->min0)
						{
							if (rot - (relYaw - PI*2) > jnt->max0)
								noman = 1;		// damned anyway
							else
								relYaw -= PI*2;	// go the long way
						}
					}
				}

				if (noman == 0)
				{
					if (relYaw < 0)			// turn to the left needed
					{
						SINGLE minVal = __min(search->angVelocity*MUL_DEG_TO_RAD*dt, -relYaw);
						rot += minVal;
						relYaw += minVal;
					}
					else
					{
						SINGLE minVal = -__min(search->angVelocity*MUL_DEG_TO_RAD*dt, relYaw);
						rot += minVal;
						relYaw += minVal;
					}

					if (limited)
					{
						if (rot > jnt->max0)
							rot = jnt->max0;
						else
						if (rot < jnt->min0)
							rot = jnt->min0;
					}
					else
					{
						if (rot < -PI)
							rot += PI*2;
						else
						if (rot > PI)
							rot -= PI*2;
					}

					if (rot != origRot)
					{
						ENGINE->set_joint_state(search->jointIndex, IE_JST_BASIC, &rot);
					}
				}
			}
		}
		search = search->next;
	}
}
//--------------------------------------------------------------------------//
//
void EffectInstance::forcePlayCanceled(IBaseObject * obj)
{
	//this guy has been prempted by something, Null out his pointer if he is critical
	TargetNode * search = targetList;
	while(search)
	{
		if(search->target.ptr == obj)
		{
			AnimListener * aSearch = search->animListener;
			while(aSearch)
			{
				if(aSearch->bAnimVolatile)//this guy is required for this effect to go off
				{
					if(search->target)
						search->target->RemoveFromEffectListeners(this);
					search->target = NULL;
				}
				aSearch = aSearch->next;
			}
		}
		search = search->next;
	}

}
//--------------------------------------------------------------------------//
//
/*void EffectInstance::addSoundInst(SoundInstance sInst)
{
	SoundInstEntry * node = new SoundInstEntry;
	node->inst = sInst;
	node->next = soundInstList;
	soundInstList = node;
}*/
//--------------------------------------------------------------------------//
//
void EffectInstance::AnimationEvent(IBaseObject * obj, const char * eventName)
{
	TargetNode * search = targetList;
	while(search)
	{
		if(search->target.ptr == obj)
		{
			AnimListener * lis = search->animListener;
			while(lis)
			{
				if(lis->event->info.type == EventSave::ANIM_CUE_TRIGGERED && strcmp(lis->event->info.stingParam,eventName) == 0)
				{
					activateEvent(lis->event);
					return;
				}
				lis = lis->next;
			}
			return;
		}
		search = search->next;
	}
}
//--------------------------------------------------------------------------//
//
void EffectInstance::AnimationCompleted(IBaseObject * obj)
{
	struct delayedEvent
	{
		EventNode * event;
		delayedEvent * next;
	};

	delayedEvent * delEvents = NULL;

	TargetNode * search = targetList;
	while(search)
	{
		if(search->target.ptr == obj)
		{
			AnimListener * animSearch = search->animListener;
			AnimListener * prev = NULL;
			while(animSearch)
			{
				if(animSearch->event->info.type == EventSave::ANIM_FINISHED)
				{
					delayedEvent * tmp = new delayedEvent;
					tmp->event = animSearch->event;
					tmp->next = delEvents;
					delEvents = tmp;
				}
				if(animSearch->bAnimVolatile)
				{
					if(prev)
						prev->next = animSearch->next;
					else
						search->animListener = animSearch->next;
					--lifeCount;
					delete animSearch;
					if(prev)
						animSearch = prev->next;
					else
						animSearch = search->animListener;
				}
				else
				{
					prev = animSearch;
					animSearch = animSearch->next;
				}
			}
		}
		search = search->next;
	}
	while(delEvents)
	{
		delayedEvent * tmp = delEvents;
		delEvents = delEvents->next;
		activateEvent(tmp->event);
		delete tmp;
	}
}
//--------------------------------------------------------------------------//
//
void EffectInstance::GameEvent(IBaseObject * obj, const char * eventName)
{
	TargetNode * search = targetList;
	while(search)
	{
		if(search->target.ptr == obj)
		{
			EventListener * lis = search->eventListener;
			while(lis)
			{
				if(lis->action->info.actionType == ActionSaveHeader::AT_LISTEN && strcmp(lis->action->listenInfo.listenName,eventName) == 0)
				{
					EventNode * events = lis->action->firstEvent;
					while(events)
					{
						activateEvent(events);
						events = events->next;
					}
				}
				lis = lis->next;
			}
			return;
		}
		search = search->next;
	}
}
//--------------------------------------------------------------------------//
//
void EffectInstance::Enable()
{
	bEnabled = true;
}
//--------------------------------------------------------------------------//
//
void EffectInstance::Disable()
{
	bEnabled = false;
}
//--------------------------------------------------------------------------//
//
void EffectInstance::Stop()
{
	bStoped = true;
}
//--------------------------------------------------------------------------//
//
BOOL32 EffectInstance::ValidateTargets()
{
	BOOL32 bAllTargetsValid = true;

	TargetNode * search = targetList;
	while(search)
	{
		// if there is not a valid target, and it used to be a valid target
		if(!search->target && !search->bPositionOnly)
		{
			bAllTargetsValid = false;
		}
		search = search->next;
	}

	return bAllTargetsValid;
}
//--------------------------------------------------------------------------//
//
IEffectTarget* EffectInstance::GetTarget(U32 id, DWORD& context)
{
	TargetNode* node = findTarget(id);
	if(node)
	{
		context = node->context;
		if(node->target)
		{
			return node->target;
		}
	}
	return NULL;
}
//--------------------------------------------------------------------------//
//
void EffectInstance::particleEvent(ActionNode * node, const char * eventName)
{
	EventNode * eSearch = node->firstEvent;
	while(eSearch)
	{
		if(eSearch->info.type == EventSave::PARTICLE_CUE_TRIGGERED && (strcmp(eSearch->info.stingParam,eventName) == 0))
		{
			activateEvent(eSearch);
			return;
		}
		eSearch = eSearch->next;
	}
}
//--------------------------------------------------------------------------//
//
void EffectInstance::activateEvent(EventNode * node)
{
#ifdef _DEBUG
	switch(node->info.type)
	{
	case EventSave::START_TRIGGERED:
		EFFECT_TRACE("EventActivated Type:START_TRIGGERED\n");
		EFFECT_TRACE1("   ID:%d\n",effectId);
		EFFECT_TRACE1("   Name:%s\n",node->info.stingParam);
		break;
	case EventSave::ANIM_CUE_TRIGGERED:
		EFFECT_TRACE("EventActivated Type:ANIM_CUE_TRIGGERED\n");
		EFFECT_TRACE1("   ID:%d\n",effectId);
		EFFECT_TRACE1("   Name:%s\n",node->info.stingParam);
		EFFECT_TRACE1("   Target:%d\n",node->info.targetID);
		break;
	case EventSave::ANIM_FINISHED:
		EFFECT_TRACE("EventActivated Type:ANIM_FINISHED\n");
		EFFECT_TRACE1("   ID:%d\n",effectId);
		EFFECT_TRACE1("   Target:%d\n",node->info.targetID);
		break;
	case EventSave::PARTICLE_CUE_TRIGGERED:
		EFFECT_TRACE("EventActivated Type:PARTICLE_CUE_TRIGGERED\n");
		EFFECT_TRACE1("ID:%d\n",effectId);
		EFFECT_TRACE1("Name:%s\n",node->info.stingParam);
		break;
	case EventSave::SWITCH:
		EFFECT_TRACE("EventActivated Type:SWITCH\n");
		EFFECT_TRACE1("   ID:%d\n",effectId);
		EFFECT_TRACE1("   Name:%s\n",node->info.stingParam);
		break;
	}
#endif

	ActionNode * aNode = node->firstAction;
	while(aNode)
	{
		switch(aNode->info.actionType)
		{
		case ActionSaveHeader::AT_ANIM:
			{
				activateAnimation(aNode);
			}
			break;
		case ActionSaveHeader::AT_PARTICLE:
			{
				activateParticle(aNode);
			}
			break;
		case ActionSaveHeader::AT_GAME_EVENT:
			{
				activateGameEvent(aNode);
			}
			break;
		case ActionSaveHeader::AT_SOUND:
			{
				activateSound(aNode);
			}
			break;
		case ActionSaveHeader::AT_SWITCH:
			{
				activateSwitch(aNode);
			}
			break;
		case ActionSaveHeader::AT_HIDE_TARGET:
			{
				activateHideTarget(aNode);
			}
			break;
		case ActionSaveHeader::AT_LISTEN:
			{
				activateListen(aNode);
			}
			break;
		case ActionSaveHeader::AT_JOINT_TRACK:
			{
				activateJointTrack(aNode);
			}
			break;
		}
		aNode = aNode->next;
	}
}
//--------------------------------------------------------------------------//
//
void EffectInstance::createAnimListener(EventNode * node)
{
	TargetNode * search = targetList;
	while(search)
	{
		if(search->id == node->info.targetID)
		{
			++lifeCount;
			AnimListener * aLis = new AnimListener;
			aLis->bAnimVolatile = true;
			aLis->event = node;
			aLis->next = search->animListener;
			search->animListener = aLis;
			return;
		}
		search = search->next;
	}
}

//--------------------------------------------------------------------------//
//------------------------------EffectHandle Class--------------------------//
//--------------------------------------------------------------------------//

struct EffectHandle : public IEffectHandle, IInternalEffectHandle
{
	EffectHandle * next;

	char fileName[64];

	EventNode * startNode;

	EffectHandle();

	virtual ~EffectHandle();

	//IEffectHandle
	virtual IEffectInstance * CreateInstance(DWORD, IEffectCallback * callback = NULL);

	virtual IEffectInstance * CreateMovieInstance();

	virtual SINGLE GetTotalPlayTime(IBaseObject ** targets, U32 numTargets);

	virtual SINGLE GetTimeToReactionCue(U32 targetID, IBaseObject ** targets, U32 numTargets);

	virtual const char * GetReactionType(U32 targetID);

	//IInternalEffectHandle
	virtual EventNode * GetStartNode();

	const char* getReactionTypeForTarget( U32 targetID, EventNode* _eventNode );
};
//--------------------------------------------------------------------------//
//
EffectHandle::EffectHandle()
{
	startNode = NULL;
}
//--------------------------------------------------------------------------//
//
EffectHandle::~EffectHandle()
{
	delete startNode;
}
//--------------------------------------------------------------------------//
//
IEffectInstance * EffectHandle::CreateInstance(DWORD effectId, IEffectCallback * callback)
{
	EffectInstance * inst = new EffectInstance(this);
	inst->callback = callback;
	internalPlayer->AddInstance(inst);

	if(effectId == 0XFFFFFFFF)
	{
		inst->effectId = s_NextEffectInstanceUniqueId;
		s_NextEffectInstanceUniqueId++;
	}
	else
	{
		inst->effectId = effectId;
	}
	EFFECT_TRACE("Instance Created\n");
	EFFECT_TRACE1("   File:%s\n",fileName);
	EFFECT_TRACE1("   ID:%d\n",inst->effectId);
	return inst;
};
//--------------------------------------------------------------------------//
//
IEffectInstance * EffectHandle::CreateMovieInstance()
{
	// todo(aaj): right now we believe there is no reason to give movie instances unique Ids

	EffectInstance * inst = new EffectInstance(this);
	inst->bMovieEffect = true;
	internalPlayer->AddInstance(inst);
	EFFECT_TRACE1("Movie Instance Created   File:%s\n",fileName);
	return inst;
}

SINGLE EffectHandle::GetTotalPlayTime(IBaseObject ** targets, U32 numTargets)
{
	if(startNode)
	{
		SINGLE playTime = 0;
		if(startNode->GetTotalPlayTime(playTime,targets,numTargets))
			return playTime;
		return 0;
	}
	return 0;
}

SINGLE EffectHandle::GetTimeToReactionCue(U32 targetID, IBaseObject ** targets, U32 numTargets)
{
	if(startNode)
	{
		SINGLE playTime = 0;
		if(startNode->GetTimeToReactionCue(playTime,targetID,targets,numTargets))
			return playTime;
		return 0;
	}
	return 0;
}

const char * EffectHandle::GetReactionType(U32 targetID)
{
	const char * returnString = getReactionTypeForTarget( targetID, startNode );

	if( !returnString )
	{
		return "default";
	}

	return returnString;
}

//--------------------------------------------------------------------------//
//
EventNode * EffectHandle::GetStartNode()
{
	return startNode;
}

const char* EffectHandle::getReactionTypeForTarget( U32 targetID, EventNode* _eventNode )
{
	if( !_eventNode ) return NULL;

	// go through each action node first

	ActionNode* actionNode = _eventNode->firstAction;

	while(actionNode)
	{
		// the action event needs to be a GAME_EVENT, match the target ID, and needs to be a response event

		if(actionNode->info.actionType == ActionSaveHeader::AT_GAME_EVENT)
		{
			ActionGameEventSave* gameEventInfo = &actionNode->gameEventInfo;

			if( gameEventInfo->targetID == targetID && gameEventInfo->bResponse )
			{
				return gameEventInfo->eventName;
			}
		}

		EventNode* eNode = actionNode->firstEvent;
		while( eNode )
		{
			const char * returnString = getReactionTypeForTarget( targetID, eNode );

			if( returnString )
			{
				return returnString;
			}

			eNode = eNode->next;
		}

		actionNode = actionNode->next;
	}

	return getReactionTypeForTarget( targetID, _eventNode->next );
}

//--------------------------------------------------------------------------//
//------------------------------EffectPlayer Class--------------------------//
//--------------------------------------------------------------------------//

struct DACOM_NO_VTABLE EffectPlayer : public IEffectPlayer, IInternalEffectPlayer,IGamePositionCallback

										   
{
	//
	// Interface mapping
	//
	BEGIN_DACOM_MAP_INBOUND(EffectPlayer)
	DACOM_INTERFACE_ENTRY(IEffectPlayer)
	END_DACOM_MAP()

	EffectInstance * instList;

	EffectHandle * handleList;

	struct ParticleNode :IParticleListener
	{
		ParticleNode * next;
		IParticleInstance * pInst;
		ActionNode * aNode;
		EffectInstance * effInst;

		~ParticleNode()
		{
			PARTMAN->ReleaseInstance(pInst);
		}

		//IParticleListener
		virtual void ParticalEvent(const char * eventName)
		{
			if(effInst)
				effInst->particleEvent(aNode,eventName);
		}
	};
	ParticleNode * particleList;
	ParticleNode * movieParticleList;

	EffectPlayer (void);

	virtual ~EffectPlayer (void);

    void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}

	/* IGamePositionCallback methods */

	virtual bool GetObjectTransform(U32 objectID, U32 hardpoint, U32 context, Transform & trans);

	virtual bool GetObjectTransform(U32 objectID, const char * hpName, U32 context, Transform & trans);

	virtual SINGLE GetParameter(const char * name, U32 objectID,DWORD context);

	virtual void ObjectPostionCallback(U32 objectID,DWORD context, char * string, Transform & trans);

	virtual bool TestCollision(Vector p1,Vector p2,Vector & collisionPoint,Vector & finalPoint,bool bTerrain,bool bTinkerToys,bool bWater,bool bUnits);

	virtual void ShakeCamera(SINGLE durration, SINGLE power);

	virtual IMeshInstance * GetObjectMesh(U32 objectID, DWORD context);

	/* IEffectPlayer methods */
	virtual IEffectHandle * LoadEffect(const char * effectFile);

	virtual void ReleaseEffect(IEffectHandle * handle);

	virtual void Update();

	virtual void Render();

	virtual void RenderMovie();

	virtual void Close();

	virtual SINGLE QueryEffectParam(GlobalEffectParam param);

	virtual void SetEffectParam(GlobalEffectParam param, SINGLE value);

	virtual void ForcePlayCanceled(IBaseObject * obj);

	virtual bool IsValidInstance(IEffectInstance * test);

	virtual IEffectInstance* GetEffectInstanceById(U32 effectId);

	virtual void ClearCallbacksTo(IEffectCallback * callback);

	//IInternalEffectPlayer

	virtual void AddInstance(struct EffectInstance * inst);

	virtual void AddParticleInst(IParticleInstance * inst, ActionNode * aNode, EffectInstance * effInst);

	// EffectPlayer

	EventNode * loadEventNode(IFileSystem * inFile);

	ActionNode * loadActionNode(IFileSystem * inFile);

	FilterNode * loadFilterNode(IFileSystem * inFile);

	IEffectHandle * findHandle(const char * name);

	inline void effectInstanceDied(EffectInstance* effInst)
	{
		ParticleNode * search = particleList;
		while(search)
		{
			if( search->effInst == effInst )
			{
				search->effInst = NULL;
			}
			search = search->next;
		}

		search = movieParticleList;
		while(search)
		{
			if( search->effInst == effInst )
			{
				search->effInst = NULL;
			}
			search = search->next;
		}
	}
};
//--------------------------------------------------------------------------//
//
EffectPlayer::EffectPlayer (void)
{
	instList = NULL;
	handleList = NULL;
	particleList = NULL;
	movieParticleList = NULL;
}
//--------------------------------------------------------------------------//
//
EffectPlayer::~EffectPlayer (void)
{
	while(instList)
	{
		EffectInstance * tmp = instList;
		instList = instList->nextInstance;
		delete tmp;
	}
	while(handleList)
	{
		EffectHandle * tmp = handleList;
		handleList = handleList->next;
		delete tmp;
	}
	while(particleList)
	{
		ParticleNode * tmp = particleList;
		particleList = particleList->next;
		delete tmp;
	}
	while(movieParticleList)
	{
		ParticleNode * tmp = movieParticleList;
		movieParticleList = movieParticleList->next;
		delete tmp;
	}

	EFFECTPLAYER = NULL;
	internalPlayer = NULL;
}
//--------------------------------------------------------------------------//
//
bool EffectPlayer::GetObjectTransform(U32 objectID, U32 hardpoint, U32 context, Transform & trans)
{
	if(context)
	{
		EffectInstance * inst = (EffectInstance *) context;
		TargetNode * tNode = inst->findTarget(objectID);
		if(tNode && tNode->target)
		{
			if(hardpoint != INVALID_HARD_POINT)
			{
				IMeshInstance * hpMesh = tNode->target->GetMesh() ;
				if(hpMesh)
				{
					hpMesh->GetHardPointTransform(hardpoint,trans);
					return true;
				}
			}
			trans = tNode->target.ptr->GetTransform();
			return true;
		}
		else if(tNode && tNode->bPositionOnly)
		{
			trans.translation = tNode->position;
			return true;
		}
	}
	return false;
}
//--------------------------------------------------------------------------//
//
bool EffectPlayer::GetObjectTransform(U32 objectID, const char * hpName, U32 context, Transform & trans)
{
	if(context)
	{
		EffectInstance * inst = (EffectInstance *) context;
		TargetNode * tNode = inst->findTarget(objectID);
		if(tNode && tNode->target)
		{
			IMeshInstance * hpMesh = tNode->target->GetMesh() ;
			if(hpName[0] && hpMesh)
			{
				U32 hpIndex = hpMesh->GetArchtype()->FindHardPointIndex(hpName);
				if(hpIndex != INVALID_HARD_POINT)
				{
					hpMesh->GetHardPointTransform(hpIndex,trans);
					return true;
				}
			}
			trans = tNode->target.ptr->GetTransform();
			return true;
		}
		else if(tNode && tNode->bPositionOnly)
		{
			trans.translation = tNode->position;
			return true;
		}
	}
	return false;
}
//--------------------------------------------------------------------------//
//
SINGLE EffectPlayer::GetParameter(const char * name, U32 objectID,DWORD context)
{
	if(objectID == -1)
	{
		return 0;//MODULE->GetValue(name,IModule::VT_EFFECT);
	}
	else if(context)
	{
		EffectInstance * inst = (EffectInstance *) context;
		TargetNode * tNode = inst->findTarget(objectID);
		if(tNode && tNode->target)
		{
			return tNode->target->QueryEffectParam(name);
		}
	}
	return 0;
}
//--------------------------------------------------------------------------//
//
void EffectPlayer::ObjectPostionCallback(U32 objectID,DWORD context, char * string, Transform & trans)
{
	if(context)
	{
		EffectInstance * inst = (EffectInstance *) context;
		TargetNode * tNode = inst->findTarget(objectID);
		if(tNode && tNode->target)
		{
			tNode->target->PositionCallback(string,trans);
		}
	}
}
//--------------------------------------------------------------------------//
//
bool EffectPlayer::TestCollision(Vector p1,Vector p2,Vector & collisionPoint,Vector & finalPoint,bool bTerrain,bool bTinkerToys,bool bWater,bool bUnits)
{
/*	if(bTerrain || bTinkerToys)
	{
		Vector normal;
		if(MAINMAP->ComplexCollisionCast(p1,p2,collisionPoint,normal,bTerrain,bTinkerToys))
		{
			Vector surfaceTangent = cross_product(normal,(p1-p2).fast_normalize());
			surfaceTangent = cross_product(surfaceTangent,normal);
			surfaceTangent.fast_normalize();
			
			finalPoint = (2 * ( collisionPoint + (dot_product((p2-collisionPoint),surfaceTangent)*surfaceTangent))) - p2;
			return true;
		}
	}
*/
	return false;
}
//--------------------------------------------------------------------------//
//
void EffectPlayer::ShakeCamera(SINGLE durration, SINGLE power)
{
	CAMERA->ShakeCamera(durration,power);
}
//--------------------------------------------------------------------------//
//
IMeshInstance * EffectPlayer::GetObjectMesh(U32 objectID, DWORD context)
{
	if(context)
	{
		EffectInstance * inst = (EffectInstance *) context;
		TargetNode * tNode = inst->findTarget(objectID);
		if(tNode && tNode->target)
		{
			return tNode->target->GetMesh();
		}
	}
	return NULL;
}
//--------------------------------------------------------------------------//
//
IEffectHandle * EffectPlayer::LoadEffect(const char * effectFile)
{
	IEffectHandle * oldHandle = findHandle(effectFile);
	if(oldHandle)
		return oldHandle;

	U32 dwWritten;
	DAFILEDESC fdesc = effectFile;
	COMPTR<IFileSystem> inFile;

	fdesc.lpImplementation = "UTF";
	if (OBJECTDIR->CreateInstance(&fdesc, inFile) == GR_OK)
	{
		DAFILEDESC fdesc = "BASEDATA";
		fdesc.dwDesiredAccess = GENERIC_READ;
		fdesc.dwShareMode = 0;  // no sharing
		
		COMPTR<IFileSystem> baseData;
		if (inFile->CreateInstance(&fdesc, baseData) == GR_OK)
		{
			U32 version;
			baseData->ReadFile(0,&version ,sizeof(U32),&dwWritten);
			CQASSERT(version == EFFECT_SAVE_VERSION && "Old Effect FileFormat");//may still be ok
		}

		inFile->SetCurrentDirectory("\\");
		
		if (inFile->SetCurrentDirectory("\\CORE_EFFECT") == 0)
			return NULL;

		EffectHandle * handle = new EffectHandle();
		handle->startNode = loadEventNode(inFile);
		strncpy(handle->fileName,effectFile, 64);
		handle->fileName[63] = 0;
		handle->next = handleList;
		handleList = handle;
		return handle;
	}
	return NULL;
}
//--------------------------------------------------------------------------//
//
void EffectPlayer::ReleaseEffect(IEffectHandle * handle)
{
}
//--------------------------------------------------------------------------//
//
void EffectPlayer::Update()
{
	EffectInstance * instUpdate = instList;
	EffectInstance * prev = NULL;
	while(instUpdate)
	{
		if(!instUpdate->update())
		{
			//delete this effect
			if(prev)
				prev->nextInstance = instUpdate->nextInstance;
			else
				instList = instUpdate->nextInstance;
			effectInstanceDied(instUpdate);
			delete instUpdate;
			if(prev)
				instUpdate = prev->nextInstance;
			else
				instUpdate = instList;
		}
		else
		{
			prev = instUpdate;
			instUpdate = instUpdate->nextInstance;
		}
	}
}
//--------------------------------------------------------------------------//
//
void EffectPlayer::Render()
{
	CAMERA->SetModelView();
	ParticleNode * search = particleList;
	ParticleNode * prev = NULL;
	U32 currentSystem = SECTOR->GetCurrentSystem();
	while(search)
	{
		if(search->pInst->IsFinished())
		{
			if(prev)
				prev->next = search->next;
			else 
				particleList = search->next;
			delete search;
			if(prev)
				search = prev->next;
			else
				search = particleList;
		}
		else
		{
			if(search->effInst && search->effInst->bEnabled && search->effInst->GetSystemID() == currentSystem)
			{
				Vector center;
				SINGLE radius;
				if(search->pInst->GetVisInfo(center,radius))
				{
					bool bVisTest = CAMERA->SphereInFrustrumFast(center,radius);
					search->pInst->Render(bVisTest);
				}
				else
					search->pInst->Render(false);
				prev = search;
				search = search->next;
			}
			else
			{
				prev = search;
				search = search->next;
			}
		}
	}
}
//--------------------------------------------------------------------------//
//
void EffectPlayer::RenderMovie()
{
	ParticleNode * search = movieParticleList;
	ParticleNode * prev = NULL;
	while(search)
	{
		if(search->pInst->IsFinished())
		{
			if(prev)
				prev->next = search->next;
			else 
				movieParticleList = search->next;
			PARTMAN->ReleaseInstance(search->pInst);
			delete search;
			if(prev)
				search = prev->next;
			else
				search = movieParticleList;
		}
		else
		{
			if(search->effInst && search->effInst->bEnabled)
			{
				Vector center;
				SINGLE radius;
				if(search->pInst->GetVisInfo(center,radius))
					search->pInst->Render(CAMERA->SphereInFrustrumFast(center,radius));
				else
					search->pInst->Render(false);
				prev = search;
				search = search->next;
			}
			else
			{
				prev = search;
				search = search->next;
			}
		}
	}
}
//--------------------------------------------------------------------------//
//
void EffectPlayer::Close()
{
	while(instList)
	{
		EffectInstance * tmp = instList;
		instList = instList->nextInstance;
		delete tmp;
	}
	while(handleList)
	{
		EffectHandle * tmp = handleList;
		handleList = handleList->next;
		delete tmp;
	}
	while(particleList)
	{
		ParticleNode * tmp = particleList;
		particleList = particleList->next;
		delete tmp;
	}
	while(movieParticleList)
	{
		ParticleNode * tmp = movieParticleList;
		movieParticleList = movieParticleList->next;
		delete tmp;
	}
}
//--------------------------------------------------------------------------//
//
SINGLE EffectPlayer::QueryEffectParam(GlobalEffectParam param)
{
	return 0;
}
//--------------------------------------------------------------------------//
//
void EffectPlayer::SetEffectParam(GlobalEffectParam param, SINGLE value)
{
}
//--------------------------------------------------------------------------//
//
void EffectPlayer::ForcePlayCanceled(IBaseObject * obj)
{
	EffectInstance * search = instList;
	while(search)
	{
		search->forcePlayCanceled(obj);
		search = search->nextInstance;
	}
}
//--------------------------------------------------------------------------//
//
bool EffectPlayer::IsValidInstance(IEffectInstance * test)
{
	EffectInstance * search = instList;
	while(search)
	{
		if(search == test)
			return true;
		search = search->nextInstance;
	}
	return false;
}
//--------------------------------------------------------------------------//
//
IEffectInstance* EffectPlayer::GetEffectInstanceById(U32 effectId)
{
	EffectInstance * inst = instList;
	while( inst )
	{
		if( inst->effectId == effectId )
		{
			return inst;
		}
		inst = inst->nextInstance;
	}
	return NULL;
}
//--------------------------------------------------------------------------//
//
void EffectPlayer::ClearCallbacksTo(IEffectCallback * callback)
{
	EffectInstance * inst = instList;
	while( inst )
	{
		if( inst->callback == callback)
		{
			inst->callback = NULL;
		}
		inst = inst->nextInstance;
	}
}
//--------------------------------------------------------------------------//
//
void EffectPlayer::AddInstance(struct EffectInstance * inst)
{
	inst->nextInstance = instList;
	instList = inst;
}
//--------------------------------------------------------------------------//
//
void EffectPlayer::AddParticleInst(IParticleInstance * inst, ActionNode * aNode, EffectInstance * effInst)
{
	ParticleNode * node = new ParticleNode;
	node->pInst = inst;
	node->aNode = aNode;
	node->effInst = effInst;
	inst->AddParticalListener(node);
	if(effInst->bMovieEffect)
	{
		node->next = movieParticleList;
		movieParticleList = node;
	}
	else
	{
		node->next = particleList;
		particleList = node;
	}
}
//--------------------------------------------------------------------------//
//
EventNode * EffectPlayer::loadEventNode(IFileSystem * inFile)
{
	U32 dwWritten;
	DAFILEDESC fdesc = "EVENTDATA";
	fdesc.dwDesiredAccess = GENERIC_READ;
	fdesc.dwShareMode = 0;  // no sharing
	
	COMPTR<IFileSystem> eventData;
	if (inFile->CreateInstance(&fdesc, eventData) == GR_OK)
	{
		EventNode * node = new EventNode;
		node->next = NULL;
		node->firstAction = NULL;
		eventData->ReadFile(0,&(node->info) ,sizeof(EventSave),&dwWritten);

		WIN32_FIND_DATA data;
		HANDLE handle;
		handle = inFile->FindFirstFile("*",&data);
		if(handle != INVALID_HANDLE_VALUE)
		{
			do
			{
				if((strncmp(data.cFileName,"Action",6)== 0) && (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
				{
					inFile->SetCurrentDirectory(data.cFileName);
					ActionNode * aNode = loadActionNode(inFile);
					aNode->next = node->firstAction;
					node->firstAction = aNode;
					inFile->SetCurrentDirectory("..");
				}
			}while(inFile->FindNextFile(handle,&data));
			inFile->FindClose(handle);
		}
		return node;
	}
	return NULL;
}
//--------------------------------------------------------------------------//
//
ActionNode * EffectPlayer::loadActionNode(IFileSystem * inFile)
{
	U32 dwWritten;
	DAFILEDESC fdesc = "ACTIONDATA";
	fdesc.dwDesiredAccess = GENERIC_READ;
	fdesc.dwShareMode = 0;  // no sharing
	
	COMPTR<IFileSystem> actionData;
	if (inFile->CreateInstance(&fdesc, actionData) == GR_OK)
	{
		ActionNode * node = new ActionNode;
		node->next = NULL;
		node->firstEvent = NULL;
		actionData->ReadFile(0,&(node->info) ,sizeof(ActionSaveHeader),&dwWritten);

		switch(node->info.actionType)
		{
		case ActionSaveHeader::AT_ANIM:
			{
				fdesc = "ANIMDATA";
				COMPTR<IFileSystem> animData;
				if (inFile->CreateInstance(&fdesc, animData) == GR_OK)
				{
					animData->ReadFile(0,&(node->animInfo) ,sizeof(ActionAnimSave),&dwWritten);
				}
			}
			break;
		case ActionSaveHeader::AT_PARTICLE:
			{
				fdesc = "PARTICLEDATA";
				COMPTR<IFileSystem> partData;
				if (inFile->CreateInstance(&fdesc, partData) == GR_OK)
				{
					partData->ReadFile(0,&(node->particleSave) ,sizeof(ActionParticleSave),&dwWritten);
				}
				node->filterList = NULL;
				WIN32_FIND_DATA data;
				HANDLE handle;
				handle = inFile->FindFirstFile("*",&data);
				if(handle != INVALID_HANDLE_VALUE)
				{
					do
					{
						if((strncmp(data.cFileName,"Filter",6)== 0) && (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
						{
							inFile->SetCurrentDirectory(data.cFileName);
							FilterNode * fNode = loadFilterNode(inFile);
							if(fNode)
							{
								fNode->next = node->filterList;
								node->filterList = fNode;
							}
							inFile->SetCurrentDirectory("..");
						}
					}while(inFile->FindNextFile(handle,&data));
					inFile->FindClose(handle);
				}

				//now that they are all loaded we need to relink them
				FilterNode * search = node->filterList;
				while(search)
				{
					U32 i;
					for(i = 0; i < search->info.numOutput; ++i)
					{
						FilterNode * linkNode = node->filterList;
						while(linkNode)
						{
							if(linkNode->info.linkID == (*((U32*)(&(search->output[i])))))
								break;
							linkNode = linkNode->next;
						}
						if(linkNode)
							search->output[i] = linkNode;
						else
							search->output[i] = NULL;
					}
					for(i = 0; i < search->info.numInput; ++i)
					{
						FilterNode * linkNode = node->filterList;
						while(linkNode)
						{
							if(linkNode->info.linkID == (*((U32*)(&(search->input[i])))))
								break;
							linkNode = linkNode->next;
						}
						if(linkNode)
							search->input[i] = linkNode;
						else
							search->input[i] = NULL;
					}
					search = search->next;
				}
			}
			break;
		case ActionSaveHeader::AT_GAME_EVENT:
			{
				fdesc = "GAMEEVENTDATA";
				COMPTR<IFileSystem> animData;
				if (inFile->CreateInstance(&fdesc, animData) == GR_OK)
				{
					animData->ReadFile(0,&(node->gameEventInfo) ,sizeof(ActionGameEventSave),&dwWritten);
				}
			}
			break;
		case ActionSaveHeader::AT_SOUND:
			{
				fdesc = "SOUNDDATA";
				COMPTR<IFileSystem> animData;
				if (inFile->CreateInstance(&fdesc, animData) == GR_OK)
				{
					animData->ReadFile(0,&(node->soundInfo) ,sizeof(ActionSoundSave),&dwWritten);
				}
			}
			break;
		case ActionSaveHeader::AT_SWITCH:
			{
				fdesc = "SWITCHDATA";
				COMPTR<IFileSystem> animData;
				if (inFile->CreateInstance(&fdesc, animData) == GR_OK)
				{
					animData->ReadFile(0,&(node->switchInfo) ,sizeof(ActionSwitchSave),&dwWritten);
				}
			}
			break;
		case ActionSaveHeader::AT_HIDE_TARGET:
			{
				fdesc = "HIDEDATA";
				COMPTR<IFileSystem> animData;
				if (inFile->CreateInstance(&fdesc, animData) == GR_OK)
				{
					animData->ReadFile(0,&(node->hideInfo) ,sizeof(ActionHideTargetSave),&dwWritten);
				}
			}
			break;
		case ActionSaveHeader::AT_LISTEN:
			{
				fdesc = "LISTENDATA";
				COMPTR<IFileSystem> animData;
				if (inFile->CreateInstance(&fdesc, animData) == GR_OK)
				{
					animData->ReadFile(0,&(node->listenInfo) ,sizeof(ActionListenSave),&dwWritten);
				}
			}
			break;
		case ActionSaveHeader::AT_JOINT_TRACK:
			{
				fdesc = "JOINTTRACKDATA";
				COMPTR<IFileSystem> animData;
				if (inFile->CreateInstance(&fdesc, animData) == GR_OK)
				{
					animData->ReadFile(0,&(node->jointTrackInfo) ,sizeof(ActionJointTrackSave),&dwWritten);
				}
			}
			break;
		}

		WIN32_FIND_DATA data;
		HANDLE handle;
		handle = inFile->FindFirstFile("*",&data);
		if(handle != INVALID_HANDLE_VALUE)
		{
			do
			{
				if((strncmp(data.cFileName,"Event",5)== 0) && (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
				{
					inFile->SetCurrentDirectory(data.cFileName);
					EventNode * eNode = loadEventNode(inFile);
					eNode->next = node->firstEvent;
					node->firstEvent = eNode;
					inFile->SetCurrentDirectory("..");
				}
			}while(inFile->FindNextFile(handle,&data));
			inFile->FindClose(handle);
		}
		return node;
	}
	return NULL;
}
//--------------------------------------------------------------------------//
//
FilterNode * EffectPlayer::loadFilterNode(IFileSystem * inFile)
{
	U32 dwWritten;
	DAFILEDESC fdesc = "FILTER";
	fdesc.dwDesiredAccess = GENERIC_READ;
	fdesc.dwShareMode = 0;  // no sharing
	
	COMPTR<IFileSystem> filterData;
	if (inFile->CreateInstance(&fdesc, filterData) == GR_OK)
	{
		FilterNode * node = new FilterNode;
		node->next = NULL;
		node->output = NULL;
		filterData->ReadFile(0,&(node->info) ,sizeof(FilterSaveHeader),&dwWritten);

		if(node->info.numOutput)
		{
			node->output = new FilterNode*[node->info.numOutput];
			memset(node->output,0,sizeof(FilterNode*)*node->info.numOutput);
			node->outputTargetID = new U32[node->info.numOutput];
		}
		else 
		{
			node->output = NULL;
			node->outputTargetID = NULL;
		}

		if(node->info.numInput)
		{
			node->input = new FilterNode*[node->info.numInput];
			memset(node->input,0,sizeof(FilterNode*)*node->info.numInput);
			node->inputTargetID = new U32[node->info.numInput];
		}
		else 
		{
			node->input = NULL;
			node->inputTargetID = NULL;
		}

		U32 i;
		for(i = 0 ; i < node->info.numOutput; ++i)
		{
			filterData->ReadFile(0,(void*)(&(node->output[i])) ,sizeof(U32),&dwWritten);//very hacky
		}

		for(i = 0 ; i < node->info.numOutput; ++i)
		{
			filterData->ReadFile(0,(&(node->outputTargetID[i])) ,sizeof(U32),&dwWritten);
		}

		for( i = 0 ; i < node->info.numInput; ++i)
		{
			filterData->ReadFile(0,(void*)(&(node->input[i])) ,sizeof(U32),&dwWritten);//very hacky
		}

		for(i = 0 ; i < node->info.numInput; ++i)
		{
			filterData->ReadFile(0,(&(node->inputTargetID[i])) ,sizeof(U32),&dwWritten);
		}

		node->data = new U8[node->info.dataSize];
		filterData->ReadFile(0,node->data ,node->info.dataSize,&dwWritten);
		return node;
	}
	return NULL;
}
//--------------------------------------------------------------------------//
//
IEffectHandle * EffectPlayer::findHandle(const char * name)
{
	EffectHandle * search = handleList;
	while(search)
	{
		if(strcmp(search->fileName,name) == 0)
			return search;
		search = search->next;
	}
	return NULL;
}

//-------------------------------------------------------------------
//-------------------------------------------------------------------
//-------------------------------------------------------------------
//
struct EffectPlayerComp : GlobalComponent
{
	EffectPlayer * eplayer;

	virtual void Startup (void)
	{
/*#ifdef _DEBUG
		FileLog::StartLog("EffectPlayer","EffectLog.txt");
		FileLog::LogLine("EffectPlayer","Effect Player Started\n");
		FileLog::Flush();
#endif*/
		EFFECTPLAYER = eplayer = new DAComponent<EffectPlayer>;
		internalPlayer = eplayer;
		GAMEPOSCALLBACK = eplayer;
		AddToGlobalCleanupList((IDAComponent **) &EFFECTPLAYER);
	}

	virtual void Initialize (void)
	{
	}
};

static EffectPlayerComp effectPlayerComp;

//-------------------------------------------------------------------
//-------------------------END EffectPlayer.cpp----------------------
//-------------------------------------------------------------------
