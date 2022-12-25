//EffectAction.cpp

#include "stdafx.h"
#include "globals.h"
#include "IEffectEvent.h"
#include "IEffectAction.h"
#include "IEffectTarget.h"
#include "IEffectFile.h"

#include <DACOM.h>
#include <filesys.h>
#include <TSmartPointer.h>

#include <stdio.h>

struct ActionGameEvent: public EffectAction,IActionGameEvent
{
	IEffectTarget * effectTarget;
	char eventName[64];

	bool bResponce:1;
	bool bDistanceDependant:1;
	bool bEndEffect:1;
	float speed;

	U32 context;

	ActionGameEvent();

	~ActionGameEvent();

	//IEffectAction
	virtual void Delete();

	virtual IActionGameEvent * GetActionGameEvent();

	virtual void StartAction();

	virtual void TriggerEvent(const char * event);

	virtual void AnimationFinished();

	virtual S32 GetDrawWidth();

	virtual bool IsOpen();

	virtual void SetOpen(bool setting);

	virtual void SaveCore(IFileSystem * outFile);

	virtual void LoadCore(IFileSystem * outFile, U32 _context, U32 version);

	virtual void NullTarget(struct IEffectTarget * target);

	virtual void NullTarget(struct ITargetAnim * target);

	virtual void NullTarget(struct ITargetHp * target);

	virtual IEffectAction * CreateCopy();

	//IActionGameEvent
	virtual void SetGameEventName(const char * eventName);

	virtual const char * GetGameEventName();

	virtual void SetTarget(struct IEffectTarget * target);

	virtual struct IEffectTarget * GetTarget();

	virtual void SetResponceFlag(bool bSetting);

	virtual bool GetResponceFlag();

	virtual void SetDistanceFlag(bool bSetting);

	virtual bool GetDistanceFlag();

	virtual void SetEndEffectFlag(bool bSetting);

	virtual bool GetEndEffectFlag();

	virtual void SetSpeed(SINGLE newSpeed);

	virtual SINGLE GetSpeed();
};

ActionGameEvent::ActionGameEvent()
{
	strcpy(name,"newName");
	next = NULL;
	firstEvent = NULL;
	effectTarget = NULL;
	eventName[0] = 0;
	context = 0;
	bResponce = false;
	bDistanceDependant = false;
	bEndEffect = false;
	speed = 0;
}

ActionGameEvent::~ActionGameEvent()
{
	while(firstEvent)
	{
		IEffectEvent * tmp = firstEvent;
		firstEvent = firstEvent->GetNextEvent();
		delete tmp;
	}
}

void ActionGameEvent::Delete()
{
	delete this;
}

IActionGameEvent * ActionGameEvent::GetActionGameEvent()
{
	return this;
}

void ActionGameEvent::StartAction()
{
}

void ActionGameEvent::TriggerEvent(const char * event)
{
	IEffectEvent * search = firstEvent;
	while(search)
	{
		if(strcmp(search->GetName(),event) == 0)
		{
			search->TriggerEvent();
		}
		search = search->GetNextEvent();
	}
}

void ActionGameEvent::AnimationFinished()
{
}

S32 ActionGameEvent::GetDrawWidth()
{
	return 0;
}

bool ActionGameEvent::IsOpen()
{
	return true;
}

void ActionGameEvent::SetOpen(bool setting)
{
}

void ActionGameEvent::SaveCore(IFileSystem * outFile)
{
	U32 dwWritten;
	DAFILEDESC fdesc = "ACTIONDATA";
	fdesc.lpImplementation = "DOS";
	fdesc.dwDesiredAccess = GENERIC_READ|GENERIC_WRITE;
//	fdesc.dwShareMode = 0;  // no sharing
	fdesc.dwCreationDistribution = CREATE_NEW;
	
	COMPTR<IFileSystem> actionFile;
	if (outFile->CreateInstance(&fdesc, actionFile) == GR_OK)
	{
		ActionSaveHeader aSave;
		aSave.version = ACTION_SAVE_VERSION;
		aSave.actionType = ActionSaveHeader::AT_GAME_EVENT;
		actionFile->WriteFile(0,&aSave ,sizeof(ActionSaveHeader),&dwWritten);
	}

	DAFILEDESC fdesc2 = "GAMEEVENTDATA";
	fdesc2.lpImplementation = "DOS";
	fdesc2.dwDesiredAccess = GENERIC_READ|GENERIC_WRITE;
//	fdesc2.dwShareMode = 0;  // no sharing
	fdesc2.dwCreationDistribution = CREATE_NEW;
	if (outFile->CreateInstance(&fdesc2, actionFile) == GR_OK)
	{
		ActionGameEventSave eventSave;
		if(effectTarget)
			eventSave.targetID = effectTarget->GetTargetID();
		else
			eventSave.targetID = -1;
		eventSave.bDistanceDependant = bDistanceDependant;
		eventSave.bResponce = bResponce;
		eventSave.bEndEffect = bEndEffect;
		eventSave.speed = speed;
		strcpy(eventSave.eventName,eventName);
		strcpy(eventSave.actionName,name);
		eventSave.xPos = iconXPos;
		eventSave.yPos = iconYPos;
		actionFile->WriteFile(0,&eventSave ,sizeof(ActionGameEventSave),&dwWritten);
	}

	U32 fileId = 0;
	IEffectEvent * search = firstEvent;
	while(search)
	{
		char buffer[255];
		sprintf(buffer,"Event%d",fileId);
		outFile->CreateDirectory(buffer);
		
		if (outFile->SetCurrentDirectory(buffer) == 0)
			return;

		search->SaveCore(outFile);
		
		if (outFile->SetCurrentDirectory("..") == 0)
			return;
		++fileId;
		search = search->GetNextEvent();
	}
}

void ActionGameEvent::LoadCore(IFileSystem * outFile, U32 _context, U32 version)
{
	context = _context;
	U32 dwWritten;
	DAFILEDESC fdesc = "GAMEEVENTDATA";
	fdesc.dwDesiredAccess = GENERIC_READ;
	fdesc.dwShareMode = 0;  // no sharing
	
	IEffectAction * action = NULL;
	COMPTR<IFileSystem> actionData;
	if (outFile->CreateInstance(&fdesc, actionData) == GR_OK)
	{
		ActionGameEventSave save;
		actionData->ReadFile(0,&(save) ,sizeof(ActionGameEventSave),&dwWritten);

		bResponce = save.bResponce;
		bDistanceDependant = save.bDistanceDependant;
		bEndEffect = save.bEndEffect;
		speed = save.speed;
		strcpy(name,save.actionName);
		strcpy(eventName,save.eventName);
		iconXPos = save.xPos;
		iconYPos = save.yPos;
		IEffectTarget * targ = EFFECTFILE->FindTargetByID(save.targetID,context);
		if(targ)
		{
			effectTarget = targ;
		}
	}
}

void ActionGameEvent::NullTarget(struct IEffectTarget * target)
{
	if(effectTarget == target)
	{
		effectTarget = NULL;
	}
	IEffectEvent * search = firstEvent;
	while(search)
	{
		search->NullTarget(target);
		search = search->GetNextEvent();
	}
}

void ActionGameEvent::NullTarget(struct ITargetAnim * target)
{
	IEffectEvent * search = firstEvent;
	while(search)
	{
		search->NullTarget(target);
		search = search->GetNextEvent();
	}
}

void ActionGameEvent::NullTarget(struct ITargetHp * target)
{
	IEffectEvent * search = firstEvent;
	while(search)
	{
		search->NullTarget(target);
		search = search->GetNextEvent();
	}
}

IEffectAction * ActionGameEvent::CreateCopy()
{
	ActionGameEvent * action = new ActionGameEvent();
	strcpy(action->name,name);
	if(firstEvent)
		action->firstEvent = firstEvent->CreateCopy(action);
	else
		action->firstEvent = NULL;
	if(next)
		action->next = next->CreateCopy();
	else
		action->next = NULL;
	action->effectTarget = effectTarget;

	action->bResponce = bResponce;
	action->bDistanceDependant = bDistanceDependant;
	action->bEndEffect = bEndEffect;
	action->speed = speed;

	strcpy(action->eventName,eventName);
	return action;
}

void ActionGameEvent::SetGameEventName(const char * _eventName)
{
	strncpy(eventName,_eventName,63);
	name[63] = 0;
}

const char * ActionGameEvent::GetGameEventName()
{
	return eventName;
}

void ActionGameEvent::SetTarget(struct IEffectTarget * target)
{
	effectTarget = target;
}

IEffectTarget * ActionGameEvent::GetTarget()
{
	return effectTarget;
}

void ActionGameEvent::SetResponceFlag(bool bSetting)
{
	bResponce = bSetting;
}

bool ActionGameEvent::GetResponceFlag()
{
	return bResponce;
}

void ActionGameEvent::SetDistanceFlag(bool bSetting) 
{
	bDistanceDependant = bSetting;
}

bool ActionGameEvent::GetDistanceFlag() 
{
	return bDistanceDependant;
}

void ActionGameEvent::SetEndEffectFlag(bool bSetting) 
{
	bEndEffect = bSetting;
}

bool ActionGameEvent::GetEndEffectFlag()
{
	return bEndEffect;
}

void ActionGameEvent::SetSpeed(SINGLE newSpeed)
{
	speed = newSpeed;
}

SINGLE ActionGameEvent::GetSpeed()
{
	return speed;
}




IEffectAction * MakeEffectActionGameEvent()
{
	return new ActionGameEvent();
}

