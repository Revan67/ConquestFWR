//ActionListen.cpp

#include "stdafx.h"
#include "globals.h"
#include "IEffectEvent.h"
#include "IEffectAction.h"
#include "IEffectTarget.h"

#include <DACOM.h>
#include <filesys.h>
#include <TSmartPointer.h>
#include <stdio.h>

struct ActionListen: public EffectAction, IActionListen
{
	char listenName[64];
	IEffectTarget * effectTarget;

	U32 context;

	ActionListen();

	~ActionListen();

	//IEffectAction
	virtual void Delete();

	virtual IActionListen * GetActionListen();

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

	//IActionListen
	virtual void SetListenName(const char * newListenName);

	virtual char * GetListenName();

	virtual void SetTarget(struct IEffectTarget * target);

	virtual struct IEffectTarget * GetTarget();
};

ActionListen::ActionListen()
{
	context = 0;
	strcpy(name,"newName");
	next = NULL;
	firstEvent = NULL;
	listenName[0] = 0;
	effectTarget = NULL;

	IEffectEvent * search = MakeEffectEvent();
	search->SetEventType(EventSave::LISTEN_EVENT);
	search->SetName("On Hearing");
	search->SetOffset(10.0f);
	search->SetNextEvent(firstEvent);
	search->SetParent(this);
	search->SetIconPos(200,0);
	SetFirstEvent(search);
}

ActionListen::~ActionListen()
{
	while(firstEvent)
	{
		IEffectEvent * tmp = firstEvent;
		firstEvent = firstEvent->GetNextEvent();
		delete tmp;
	}
}

void ActionListen::Delete()
{
	delete this;
}

IActionListen * ActionListen::GetActionListen()
{
	return this;
}

void ActionListen::StartAction()
{
	IEffectEvent * search = firstEvent;
	while(search)
	{
		search->TriggerEvent();
		search = search->GetNextEvent();
	}
}

void ActionListen::TriggerEvent(const char * event)
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

void ActionListen::AnimationFinished()
{
}

S32 ActionListen::GetDrawWidth()
{
	return 0;
}

bool ActionListen::IsOpen()
{
	return true;
}

void ActionListen::SetOpen(bool setting)
{
}

void ActionListen::SaveCore(IFileSystem * outFile)
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
		aSave.actionType = ActionSaveHeader::AT_LISTEN_ACTION;
		actionFile->WriteFile(0,&aSave ,sizeof(ActionSaveHeader),&dwWritten);
	}

	DAFILEDESC fdesc2 = "LISTENDATA";
	fdesc2.lpImplementation = "DOS";
	fdesc2.dwDesiredAccess = GENERIC_READ|GENERIC_WRITE;
//	fdesc2.dwShareMode = 0;  // no sharing
	fdesc2.dwCreationDistribution = CREATE_NEW;
	if (outFile->CreateInstance(&fdesc2, actionFile) == GR_OK)
	{
		ActionListenSave listenSave;
		strcpy(listenSave.listenName,listenName);
		if(effectTarget)
			listenSave.targetID = effectTarget->GetTargetID();
		else
			listenSave.targetID = 0;
		strcpy(listenSave.actionName,name);
		listenSave.xPos = iconXPos;
		listenSave.yPos = iconYPos;
		actionFile->WriteFile(0,&listenSave ,sizeof(listenSave),&dwWritten);
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

void ActionListen::LoadCore(struct IFileSystem * inFile, U32 _context, U32 version)
{
	context = _context;
	U32 dwWritten;
	DAFILEDESC fdesc = "LISTENDATA";
	fdesc.dwDesiredAccess = GENERIC_READ;
	fdesc.dwShareMode = 0;  // no sharing
	
	IEffectAction * action = NULL;
	COMPTR<IFileSystem> actionData;
	if (inFile->CreateInstance(&fdesc, actionData) == GR_OK)
	{
		ActionListenSave listenSave;
		actionData->ReadFile(0,&(listenSave) ,sizeof(ActionListenSave),&dwWritten);
		strcpy(listenName,listenSave.listenName);
		strcpy(name,listenSave.actionName);
		iconXPos = listenSave.xPos;
		iconYPos = listenSave.yPos;
		effectTarget = EFFECTFILE->FindTargetByID(listenSave.targetID,context);
	}
}

void ActionListen::NullTarget(struct IEffectTarget * target)
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

void ActionListen::NullTarget(struct ITargetAnim * target)
{
	IEffectEvent * search = firstEvent;
	while(search)
	{
		search->NullTarget(target);
		search = search->GetNextEvent();
	}
}

void ActionListen::NullTarget(struct ITargetHp * target)
{
	IEffectEvent * search = firstEvent;
	while(search)
	{
		search->NullTarget(target);
		search = search->GetNextEvent();
	}
}

IEffectAction * ActionListen::CreateCopy()
{
	ActionListen * action = new ActionListen();
	strcpy(action->name,name);
	if(firstEvent)
		action->firstEvent = firstEvent->CreateCopy(action);
	else
		action->firstEvent = NULL;
	if(next)
		action->next = next->CreateCopy();
	else
		action->next = NULL;
	strcpy(action->listenName,listenName);
	action->effectTarget = effectTarget;
	return action;
}

void ActionListen::SetListenName(const char * newListenName)
{
	strcpy(listenName,newListenName);
}

char * ActionListen::GetListenName()
{
	return listenName;
}

void ActionListen::SetTarget(struct IEffectTarget * target)
{
	effectTarget = target;
}

struct IEffectTarget * ActionListen::GetTarget()
{
	return effectTarget;
}

IEffectAction * MakeEffectActionListen()
{
	return new ActionListen();
}


