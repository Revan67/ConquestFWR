//ActionDetachTarget.cpp

#include "stdafx.h"
#include "globals.h"
#include "IEffectEvent.h"
#include "IEffectAction.h"
#include "IEffectTarget.h"

#include <DACOM.h>
#include <filesys.h>
#include <TSmartPointer.h>
#include <stdio.h>

struct ActionDetachTarget: public EffectAction, IActionDetachTarget
{
	IEffectTarget * effectTarget;
	U32 attachID;
	bool bDetachAll;

	U32 context;

	ActionDetachTarget();

	~ActionDetachTarget();

	//IEffectAction
	virtual void Delete();

	virtual IActionDetachTarget * GetActionDetachTarget();

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

	//IActionDetachTarget
	virtual void SetTarget(struct IEffectTarget * target);

	virtual struct IEffectTarget * GetTarget();

	virtual void SetAttachID(U32 id);

	virtual U32 GetAttachID();

	virtual bool GetAllFlag();

	virtual void SetAllFlag(bool bSetting);
};

ActionDetachTarget::ActionDetachTarget()
{
	context = 0;
	strcpy(name,"newName");
	next = NULL;
	firstEvent = NULL;
	effectTarget = NULL;
	bDetachAll = false;
	attachID = 0;
}

ActionDetachTarget::~ActionDetachTarget()
{
	while(firstEvent)
	{
		IEffectEvent * tmp = firstEvent;
		firstEvent = firstEvent->GetNextEvent();
		delete tmp;
	}
}

void ActionDetachTarget::Delete()
{
	delete this;
}

IActionDetachTarget * ActionDetachTarget::GetActionDetachTarget()
{
	return this;
}

void ActionDetachTarget::StartAction()
{
	if(effectTarget)
	{
		if(bDetachAll)
			effectTarget->DetatchAllMeshes();
		else
			effectTarget->DetatchMesh(attachID);
	}
}

void ActionDetachTarget::TriggerEvent(const char * event)
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

void ActionDetachTarget::AnimationFinished()
{
}

S32 ActionDetachTarget::GetDrawWidth()
{
	return 0;
}

bool ActionDetachTarget::IsOpen()
{
	return true;
}

void ActionDetachTarget::SetOpen(bool setting)
{
}

void ActionDetachTarget::SaveCore(IFileSystem * outFile)
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
		aSave.actionType = ActionSaveHeader::AT_DETACH_TARGET;
		actionFile->WriteFile(0,&aSave ,sizeof(ActionSaveHeader),&dwWritten);
	}

	DAFILEDESC fdesc2 = "DETACHDATA";
	fdesc2.lpImplementation = "DOS";
	fdesc2.dwDesiredAccess = GENERIC_READ|GENERIC_WRITE;
//	fdesc2.dwShareMode = 0;  // no sharing
	fdesc2.dwCreationDistribution = CREATE_NEW;
	if (outFile->CreateInstance(&fdesc2, actionFile) == GR_OK)
	{
		ActionDetatchTargetSave detatchSave;
		detatchSave.attachIndex = attachID;
		if(effectTarget)
			detatchSave.targetID = effectTarget->GetTargetID();
		else
			detatchSave.targetID = 0;
		detatchSave.bDetachAll = bDetachAll;
		strcpy(detatchSave.actionName,name);
		detatchSave.xPos = iconXPos;
		detatchSave.yPos = iconYPos;
		actionFile->WriteFile(0,&detatchSave ,sizeof(detatchSave),&dwWritten);
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

void ActionDetachTarget::LoadCore(struct IFileSystem * inFile, U32 _context, U32 version)
{
	context = _context;
	U32 dwWritten;
	DAFILEDESC fdesc = "DETACHDATA";
	fdesc.dwDesiredAccess = GENERIC_READ;
	fdesc.dwShareMode = 0;  // no sharing
	
	IEffectAction * action = NULL;
	COMPTR<IFileSystem> actionData;
	if (inFile->CreateInstance(&fdesc, actionData) == GR_OK)
	{
		ActionDetatchTargetSave detachSave;
		actionData->ReadFile(0,&(detachSave) ,sizeof(ActionDetatchTargetSave),&dwWritten);
		attachID = detachSave.attachIndex;
		bDetachAll = detachSave.bDetachAll;
		strcpy(name,detachSave.actionName);
		iconXPos = detachSave.xPos;
		iconYPos = detachSave.yPos;
		effectTarget = EFFECTFILE->FindTargetByID(detachSave.targetID,context);
	}
}

void ActionDetachTarget::NullTarget(struct IEffectTarget * target)
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

void ActionDetachTarget::NullTarget(struct ITargetAnim * target)
{
	IEffectEvent * search = firstEvent;
	while(search)
	{
		search->NullTarget(target);
		search = search->GetNextEvent();
	}
}

void ActionDetachTarget::NullTarget(struct ITargetHp * target)
{
	IEffectEvent * search = firstEvent;
	while(search)
	{
		search->NullTarget(target);
		search = search->GetNextEvent();
	}
}

IEffectAction * ActionDetachTarget::CreateCopy()
{
	ActionDetachTarget * action = new ActionDetachTarget();
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
	action->bDetachAll = bDetachAll;
	action->attachID = attachID;
	return action;
}

void ActionDetachTarget::SetTarget(struct IEffectTarget * target)
{
	effectTarget = target;
}

struct IEffectTarget * ActionDetachTarget::GetTarget()
{
	return effectTarget;
}


void ActionDetachTarget::SetAttachID(U32 id)
{
	attachID = id;
}

U32 ActionDetachTarget::GetAttachID()
{
	return attachID;
}

bool ActionDetachTarget::GetAllFlag()
{
	return bDetachAll;
}

void ActionDetachTarget::SetAllFlag(bool bSetting)
{
	bDetachAll = bSetting;
}


IEffectAction * MakeEffectActionDetachTarget()
{
	return new ActionDetachTarget();
}


