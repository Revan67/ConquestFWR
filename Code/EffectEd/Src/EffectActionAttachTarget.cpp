//ActionAttachTarget.cpp

#include "stdafx.h"
#include "globals.h"
#include "IEffectEvent.h"
#include "IEffectAction.h"
#include "IEffectTarget.h"

#include <DACOM.h>
#include <filesys.h>
#include <TSmartPointer.h>
#include <stdio.h>

struct ActionAttachTarget: public EffectAction, IActionAttachTarget
{
	IEffectTarget * effectTarget;
	char hpName[64];
	char meshName[64];
	char meshHpName[64];
	U32 attachID;

	U32 context;

	ActionAttachTarget();

	~ActionAttachTarget();

	//IEffectAction
	virtual void Delete();

	virtual IActionAttachTarget * GetActionAttachTarget();

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

	//IActionAttachTarget
	virtual const char * GetMeshName();

	virtual void SetMeshName(const char * newMeshName);

	virtual void SetTarget(struct IEffectTarget * target);

	virtual struct IEffectTarget * GetTarget();

	virtual void SetMeshHpName(const char * setting);

	virtual const char * GetMeshHpName();

	virtual void SetHpName(const char * setting);

	virtual const char * GetHpName();

	virtual void SetAttachID(U32 id);

	virtual U32 GetAttachID();
};

ActionAttachTarget::ActionAttachTarget()
{
	context = 0;
	strcpy(name,"newName");
	next = NULL;
	firstEvent = NULL;
	effectTarget = NULL;
	hpName[0] = 0;
	meshName[0] = 0;
	meshHpName[0] = 0;
	attachID = 0;

	IEffectEvent * event = MakeEffectEvent();
	event->SetEventType(EventSave::ATTACH_FINISHED);
	event->SetName("Attach Finished");
	event->SetOffset(10);
	event->SetNextEvent(firstEvent);
	SetFirstEvent(event);
	event->SetIconPos(200,0);
}

ActionAttachTarget::~ActionAttachTarget()
{
	while(firstEvent)
	{
		IEffectEvent * tmp = firstEvent;
		firstEvent = firstEvent->GetNextEvent();
		delete tmp;
	}
}

void ActionAttachTarget::Delete()
{
	delete this;
}

IActionAttachTarget * ActionAttachTarget::GetActionAttachTarget()
{
	return this;
}

void ActionAttachTarget::StartAction()
{
	if(effectTarget)
	{
		effectTarget->AttachMesh(meshName,meshHpName,hpName,attachID);
	}
	IEffectEvent * search = firstEvent;
	while(search)
	{
		if(search->GetEventType() == EventSave::ATTACH_FINISHED)
		{
			search->TriggerEvent();
		}
		search = search->GetNextEvent();
	}
}

void ActionAttachTarget::TriggerEvent(const char * event)
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

void ActionAttachTarget::AnimationFinished()
{
}

S32 ActionAttachTarget::GetDrawWidth()
{
	return 0;
}

bool ActionAttachTarget::IsOpen()
{
	return true;
}

void ActionAttachTarget::SetOpen(bool setting)
{
}

void ActionAttachTarget::SaveCore(IFileSystem * outFile)
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
		aSave.actionType = ActionSaveHeader::AT_ATTACH_TARGET;
		actionFile->WriteFile(0,&aSave ,sizeof(ActionSaveHeader),&dwWritten);
	}

	DAFILEDESC fdesc2 = "ATTACHDATA";
	fdesc2.lpImplementation = "DOS";
	fdesc2.dwDesiredAccess = GENERIC_READ|GENERIC_WRITE;
//	fdesc2.dwShareMode = 0;  // no sharing
	fdesc2.dwCreationDistribution = CREATE_NEW;
	if (outFile->CreateInstance(&fdesc2, actionFile) == GR_OK)
	{
		ActionAttachTargetSave attachSave;
		attachSave.attachIndex = attachID;
		if(effectTarget)
			attachSave.targetID = effectTarget->GetTargetID();
		else
			attachSave.targetID = 0;
		strcpy(attachSave.hpName,hpName);
		strcpy(attachSave.meshfile,meshName);
		strcpy(attachSave.meshHpName,meshHpName);
		strcpy(attachSave.actionName,name);
		attachSave.xPos = iconXPos;
		attachSave.yPos = iconYPos;
		actionFile->WriteFile(0,&attachSave ,sizeof(attachSave),&dwWritten);
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

void ActionAttachTarget::LoadCore(struct IFileSystem * inFile, U32 _context, U32 version)
{
	context = _context;
	U32 dwWritten;
	DAFILEDESC fdesc = "ATTACHDATA";
	fdesc.dwDesiredAccess = GENERIC_READ;
	fdesc.dwShareMode = 0;  // no sharing
	
	IEffectAction * action = NULL;
	COMPTR<IFileSystem> actionData;
	if (inFile->CreateInstance(&fdesc, actionData) == GR_OK)
	{
		ActionAttachTargetSave attachSave;
		actionData->ReadFile(0,&(attachSave) ,sizeof(ActionAttachTargetSave),&dwWritten);
		attachID = attachSave.attachIndex;
		strcpy(name,attachSave.actionName);
		strcpy(hpName,attachSave.hpName);
		strcpy(meshName,attachSave.meshfile);
		strcpy(meshHpName,attachSave.meshHpName);
		iconXPos = attachSave.xPos;
		iconYPos = attachSave.yPos;
		effectTarget = EFFECTFILE->FindTargetByID(attachSave.targetID,context);
	}
}

void ActionAttachTarget::NullTarget(struct IEffectTarget * target)
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

void ActionAttachTarget::NullTarget(struct ITargetAnim * target)
{
	IEffectEvent * search = firstEvent;
	while(search)
	{
		search->NullTarget(target);
		search = search->GetNextEvent();
	}
}

void ActionAttachTarget::NullTarget(struct ITargetHp * target)
{
	IEffectEvent * search = firstEvent;
	while(search)
	{
		search->NullTarget(target);
		search = search->GetNextEvent();
	}
}

IEffectAction * ActionAttachTarget::CreateCopy()
{
	ActionAttachTarget * action = new ActionAttachTarget();
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
	strcpy(action->hpName,hpName);
	strcpy(action->meshName,meshName);
	strcpy(action->meshHpName,meshHpName);
	action->attachID = attachID;
	return action;
}

const char * ActionAttachTarget::GetMeshName()
{
	return meshName;
}

void ActionAttachTarget::SetMeshName(const char * newMeshName)
{
	strncpy(meshName,newMeshName,63);
	meshName[63] = 0;
}

void ActionAttachTarget::SetTarget(struct IEffectTarget * target)
{
	effectTarget = target;
}

struct IEffectTarget * ActionAttachTarget::GetTarget()
{
	return effectTarget;
}

void ActionAttachTarget::SetMeshHpName(const char * setting)
{
	strncpy(meshHpName,setting,63);
	meshHpName[63] = 0;
}

const char * ActionAttachTarget::GetMeshHpName()
{
	return meshHpName;
}

void ActionAttachTarget::SetHpName(const char * setting)
{
	strncpy(hpName,setting,63);
	hpName[63] = 0;
}

const char * ActionAttachTarget::GetHpName()
{
	return hpName;
}

void ActionAttachTarget::SetAttachID(U32 id)
{
	attachID = id;
}

U32 ActionAttachTarget::GetAttachID()
{
	return attachID;
}

IEffectAction * MakeEffectActionAttachTarget()
{
	return new ActionAttachTarget();
}


