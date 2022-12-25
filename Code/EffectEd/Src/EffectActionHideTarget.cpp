//ActionHideTarget.cpp

#include "stdafx.h"
#include "globals.h"
#include "IEffectEvent.h"
#include "IEffectAction.h"
#include "IEffectTarget.h"

#include <DACOM.h>
#include <filesys.h>
#include <TSmartPointer.h>
#include <stdio.h>

struct ActionHideTarget: public EffectAction, IActionHideTarget
{
	bool bHide;
	IEffectTarget * effectTarget;

	U32 context;

	ActionHideTarget();

	~ActionHideTarget();

	//IEffectAction
	virtual void Delete();

	virtual IActionHideTarget * GetActionHideTarget();

	virtual void StartAction();

	virtual void TriggerEvent(const char * event);

	virtual void AnimationFinished();

	virtual S32 GetDrawWidth();

	virtual bool IsOpen();

	virtual void SetOpen(bool setting);

	virtual void SaveCore(IFileSystem * outFile);

	virtual void LoadCore(struct IFileSystem * inFile, U32 _context, U32 version);

	virtual void NullTarget(struct IEffectTarget * target);

	virtual void NullTarget(struct ITargetAnim * target);

	virtual void NullTarget(struct ITargetHp * target);

	virtual IEffectAction * CreateCopy();

	//IActionHideTarget
	virtual bool GetHide();

	virtual void SetHide(bool bSetting);

	virtual void SetTarget(struct IEffectTarget * target);

	virtual struct IEffectTarget * GetTarget();
};

ActionHideTarget::ActionHideTarget()
{
	context = 0;
	strcpy(name,"newName");
	next = NULL;
	firstEvent = NULL;
	bHide = true;
	effectTarget = NULL;
}

ActionHideTarget::~ActionHideTarget()
{
	while(firstEvent)
	{
		IEffectEvent * tmp = firstEvent;
		firstEvent = firstEvent->GetNextEvent();
		delete tmp;
	}
}

void ActionHideTarget::Delete()
{
	delete this;
}

IActionHideTarget * ActionHideTarget::GetActionHideTarget()
{
	return this;
}

void ActionHideTarget::StartAction()
{
	if(effectTarget)
	{
		effectTarget->Hide(bHide);
	}
}

void ActionHideTarget::TriggerEvent(const char * event)
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

void ActionHideTarget::AnimationFinished()
{
}

S32 ActionHideTarget::GetDrawWidth()
{
	return 0;
}

bool ActionHideTarget::IsOpen()
{
	return true;
}

void ActionHideTarget::SetOpen(bool setting)
{
}

void ActionHideTarget::SaveCore(IFileSystem * outFile)
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
		aSave.actionType = ActionSaveHeader::AT_HIDE_TARGET;
		actionFile->WriteFile(0,&aSave ,sizeof(ActionSaveHeader),&dwWritten);
	}

	DAFILEDESC fdesc2 = "HIDEDATA";
	fdesc2.lpImplementation = "DOS";
	fdesc2.dwDesiredAccess = GENERIC_READ|GENERIC_WRITE;
//	fdesc2.dwShareMode = 0;  // no sharing
	fdesc2.dwCreationDistribution = CREATE_NEW;
	if (outFile->CreateInstance(&fdesc2, actionFile) == GR_OK)
	{
		ActionHideTargetSave hideSave;
		hideSave.bHide = bHide;
		if(effectTarget)
			hideSave.targetID = effectTarget->GetTargetID();
		strcpy(hideSave.actionName,name);
		hideSave.xPos = iconXPos;
		hideSave.yPos = iconYPos;
		actionFile->WriteFile(0,&hideSave ,sizeof(hideSave),&dwWritten);
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

void ActionHideTarget::LoadCore(struct IFileSystem * inFile, U32 _context, U32 version)
{
	context = _context;
	U32 dwWritten;
	DAFILEDESC fdesc = "HIDEDATA";
	fdesc.dwDesiredAccess = GENERIC_READ;
	fdesc.dwShareMode = 0;  // no sharing
	
	IEffectAction * action = NULL;
	COMPTR<IFileSystem> actionData;
	if (inFile->CreateInstance(&fdesc, actionData) == GR_OK)
	{
		ActionHideTargetSave hideSave;
		actionData->ReadFile(0,&(hideSave) ,sizeof(ActionHideTargetSave),&dwWritten);
		bHide = hideSave.bHide;
		strcpy(name,hideSave.actionName);
		iconXPos = hideSave.xPos;
		iconYPos = hideSave.yPos;
		effectTarget = EFFECTFILE->FindTargetByID(hideSave.targetID,context);
	}
}

void ActionHideTarget::NullTarget(struct IEffectTarget * target)
{
	if(effectTarget == target)
		effectTarget = NULL;
	IEffectEvent * search = firstEvent;
	while(search)
	{
		search->NullTarget(target);
		search = search->GetNextEvent();
	}
}

void ActionHideTarget::NullTarget(struct ITargetAnim * target)
{
	IEffectEvent * search = firstEvent;
	while(search)
	{
		search->NullTarget(target);
		search = search->GetNextEvent();
	}
}

void ActionHideTarget::NullTarget(struct ITargetHp * target)
{
	IEffectEvent * search = firstEvent;
	while(search)
	{
		search->NullTarget(target);
		search = search->GetNextEvent();
	}
}

IEffectAction * ActionHideTarget::CreateCopy()
{
	ActionHideTarget * action = new ActionHideTarget();
	strcpy(action->name,name);
	if(firstEvent)
		action->firstEvent = firstEvent->CreateCopy(action);
	else
		action->firstEvent = NULL;
	if(next)
		action->next = next->CreateCopy();
	else
		action->next = NULL;
	action->bHide = bHide;
	action->effectTarget = effectTarget;
	return action;
}

bool ActionHideTarget::GetHide()
{
	return bHide;
}

void ActionHideTarget::SetHide(bool bSetting)
{
	bHide = bSetting;
}

void ActionHideTarget::SetTarget(struct IEffectTarget * target)
{
	effectTarget = target;
}

struct IEffectTarget * ActionHideTarget::GetTarget()
{
	return effectTarget;
}

IEffectAction * MakeEffectActionHideTarget()
{
	return new ActionHideTarget();
}


