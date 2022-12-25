//EffectActionAnim.cpp

#include "stdafx.h"
#include "globals.h"
#include "IEffectEvent.h"
#include "IEffectAction.h"
#include "IEffectTarget.h"
#include "ITargetCue.h"
#include "ITargetAnim.h"
#include "EventGraph.h"

#include <DACOM.h>
#include <filesys.h>
#include <TSmartPointer.h>

#include <stdio.h>

struct EffectActionAnim: public EffectAction, IActionAnimation
{
	bool bLooping;
	bool bForced;
	bool bOpen;
	bool bUseAnimName;

	char animName[64];

	U32 context;

	IEffectTarget * effectTarget;

	ITargetAnim * targetAnim;

	EffectActionAnim();

	~EffectActionAnim();

	//IEffectAction
	virtual void Delete();

	virtual IActionAnimation * GetActionAnimation();

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

	//IActionAnimation

	virtual void SetAnimation(struct ITargetAnim * anim);

	virtual struct ITargetAnim * GetAnimation();

	virtual void SetTarget(struct IEffectTarget * target);

	virtual struct IEffectTarget * GetTarget();

	virtual void SetLooping(bool bSetting);

	virtual bool IsLooping();

	virtual void SetForced(bool bSetting);

	virtual bool IsForced();

	virtual bool IsNamedAnim();

	virtual void SetNamedAnim(const char * newName);

	virtual const char * GetAnimName();
};

EffectActionAnim::EffectActionAnim()
{
	strcpy(name,"Animation");
	next = NULL;
	firstEvent = NULL;
	effectTarget = NULL;
	targetAnim = NULL;
	bLooping = false;
	bForced = false;
	bOpen = false;
	animName[0] = 0;
	bUseAnimName = false;

	context = 0;
}

EffectActionAnim::~EffectActionAnim()
{
	while(firstEvent)
	{
		IEffectEvent * tmp = firstEvent;
		firstEvent = firstEvent->GetNextEvent();
		delete tmp;
	}
}

void EffectActionAnim::Delete()
{
	delete this;
}

IActionAnimation * EffectActionAnim::GetActionAnimation()
{
	return this;
}

void EffectActionAnim::StartAction()
{
	if(effectTarget && targetAnim)
	{
		effectTarget->SetCurrentAction(this);
		effectTarget->PlayAnimation(targetAnim,bLooping);
	}
	else if(effectTarget && bUseAnimName)
	{
		effectTarget->SetCurrentAction(this);
		effectTarget->PlayNamedAnimation(animName,bLooping);
	}
}

void EffectActionAnim::TriggerEvent(const char * event)
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

void EffectActionAnim::AnimationFinished()
{
	IEffectEvent * search = firstEvent;
	while(search)
	{
		if(search->GetEventType() == EventSave::ANIM_FINISHED)
		{
			search->TriggerEvent();
		}
		search = search->GetNextEvent();
	}
}


S32 EffectActionAnim::GetDrawWidth()
{
	if(targetAnim)
	{
		return (S32)(targetAnim->GetPlayTime());
	}
	return 0;
}

bool EffectActionAnim::IsOpen()
{
	return bOpen;
}

void EffectActionAnim::SetOpen(bool setting)
{
	bOpen = setting;
}

void EffectActionAnim::SaveCore(IFileSystem * outFile)
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
		aSave.actionType = ActionSaveHeader::AT_ANIM;
		actionFile->WriteFile(0,&aSave ,sizeof(ActionSaveHeader),&dwWritten);
	}

	DAFILEDESC fdesc2 = "ANIMDATA";
	fdesc2.lpImplementation = "DOS";
	fdesc2.dwDesiredAccess = GENERIC_READ|GENERIC_WRITE;
//	fdesc2.dwShareMode = 0;  // no sharing
	fdesc2.dwCreationDistribution = CREATE_NEW;
	if (outFile->CreateInstance(&fdesc2, actionFile) == GR_OK)
	{
		ActionAnimSave animSave;
		animSave.bLooping = bLooping;
		animSave.bForced = bForced;
		if(effectTarget)
			animSave.targetID = effectTarget->GetTargetID();
		if(targetAnim)
			strcpy(animSave.animName,targetAnim->GetName());
		else
			strcpy(animSave.animName,animName);
		strcpy(animSave.actionName,name);
		animSave.xPos = iconXPos;
		animSave.yPos = iconYPos;
		actionFile->WriteFile(0,&animSave ,sizeof(animSave),&dwWritten);
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

void EffectActionAnim::LoadCore(struct IFileSystem * inFile, U32 _context, U32 version)
{
	context = _context;
	U32 dwWritten;
	DAFILEDESC fdesc = "ANIMDATA";
	fdesc.dwDesiredAccess = GENERIC_READ;
	fdesc.dwShareMode = 0;  // no sharing
	
	IEffectAction * action = NULL;
	COMPTR<IFileSystem> actionData;
	if (inFile->CreateInstance(&fdesc, actionData) == GR_OK)
	{
		ActionAnimSave save;
		actionData->ReadFile(0,&(save) ,sizeof(ActionAnimSave),&dwWritten);
		bLooping = save.bLooping;
		bForced = save.bForced;
		strcpy(name,save.actionName);
		iconXPos = save.xPos;
		iconYPos = save.yPos;

		IEffectTarget * targ = EFFECTFILE->FindTargetByID(save.targetID,context);
		if(targ)
		{
			effectTarget = targ;
			ITargetAnim * anim = targ->GetFirstAnim();
			while(anim)
			{
				if(strcmp(save.animName,anim->GetName())==0)
				{
					targetAnim = anim;
					ITargetCue * cue = targetAnim->GetFirstCue();
					while(cue)
					{
						IEffectEvent * event = MakeEffectEvent();
						event->SetEventType(EventSave::ANIM_CUE_TRIGGERED);
						event->SetName(cue->GetName());
						event->SetOffset(cue->GetTime());
						event->SetParent(this);
						event->SetFileDependant(true);

						IEffectEvent * search = firstEvent;
						IEffectEvent * prev = NULL;
						while(search)
						{
							if(search->GetOffset() < event->GetOffset())
							{
								break;
							}
							prev = search;
							search = search->GetNextEvent();
						}
						event->SetNextEvent(search);
						if(prev)
							prev->SetNextEvent(event);
						else
							firstEvent = event;

						cue = cue->GetNextCue();
					}
					break;
				}
				anim = anim->GetNextAnim();
			}
			if(!targetAnim && save.animName[0])
			{
				bUseAnimName = true;
				strcpy(animName,save.animName);
			}
		}
	}
}

void EffectActionAnim::NullTarget(struct IEffectTarget * target)
{
	if(effectTarget == target)
	{
		effectTarget = NULL;
		targetAnim = NULL;
	}
	IEffectEvent * search = firstEvent;
	while(search)
	{
		search->NullTarget(target);
		search = search->GetNextEvent();
	}
}

void EffectActionAnim::NullTarget(struct ITargetAnim * target)
{
	if(targetAnim == target)
	{
		targetAnim = NULL;
	}
	IEffectEvent * search = firstEvent;
	while(search)
	{
		search->NullTarget(target);
		search = search->GetNextEvent();
	}
}

void EffectActionAnim::NullTarget(struct ITargetHp * target)
{
	IEffectEvent * search = firstEvent;
	while(search)
	{
		search->NullTarget(target);
		search = search->GetNextEvent();
	}
}

IEffectAction * EffectActionAnim::CreateCopy()
{
	EffectActionAnim * action = new EffectActionAnim();
	strcpy(action->name,name);
	if(firstEvent)
		action->firstEvent = firstEvent->CreateCopy(action);
	else
		action->firstEvent = NULL;
	if(next)
		action->next = next->CreateCopy();
	else
		action->next = NULL;
	action->bForced = bForced;
	action->bLooping = bLooping;
	action->bOpen = bOpen;
	action->effectTarget = effectTarget;
	action->targetAnim = targetAnim;
	action->bUseAnimName = bUseAnimName;
	strcpy(action->animName,animName);
	return action;
}

void EffectActionAnim::SetAnimation(struct ITargetAnim * anim)
{
	bUseAnimName = false;
	IEffectEvent * search = firstEvent;
	IEffectEvent * prev = NULL;
	while(search)
	{
		if(search->IsFileDependant())
		{
			if(prev)
				prev->SetNextEvent(search->GetNextEvent());
			else
				firstEvent = search->GetNextEvent();
			delete search;
			if(prev)
				search = prev->GetNextEvent();
			else
				search = firstEvent;
		}
		else
		{
			prev = search;
			search = search->GetNextEvent();
		}
	}
	targetAnim= anim;
	if(targetAnim)
	{
		ITargetCue * cue = targetAnim->GetFirstCue();
		S32 yOffset = 0;
		while(cue)
		{
			yOffset += 50;
			cue = cue->GetNextCue();
		}
		cue = targetAnim->GetFirstCue();
		while(cue)
		{
			yOffset -= 50;
			IEffectEvent * event = MakeEffectEvent();
			event->SetEventType(EventSave::ANIM_CUE_TRIGGERED);
			event->SetName(cue->GetName());
			event->SetOffset(cue->GetTime());
			event->SetParent(this);
			event->SetFileDependant(true);
			event->SetIconPos(200,yOffset);

			IEffectEvent * search = firstEvent;
			IEffectEvent * prev = NULL;
			while(search)
			{
				if(search->GetOffset() < event->GetOffset())
				{
					break;
				}
				prev = search;
				search = search->GetNextEvent();
			}
			event->SetNextEvent(search);
			if(prev)
				prev->SetNextEvent(event);
			else
				firstEvent = event;
	

			cue = cue->GetNextCue();
		}
	}
	EventGraph::InvalidateEventGraph();
	InvalidateRect(mainWindow,NULL,false);
}

ITargetAnim * EffectActionAnim::GetAnimation()
{
	return targetAnim;
}

void EffectActionAnim::SetTarget(struct IEffectTarget * target)
{
	effectTarget = target;
}

IEffectTarget * EffectActionAnim::GetTarget()
{
	return effectTarget;
}

void EffectActionAnim::SetLooping(bool bSetting)
{
	bLooping = bSetting;
}

bool EffectActionAnim::IsLooping()
{
	return bLooping;
}

void EffectActionAnim::SetForced(bool bSetting)
{
	bForced = bSetting;
}

bool EffectActionAnim::IsForced()
{
	return bForced;
}

bool EffectActionAnim::IsNamedAnim()
{
	return bUseAnimName;
}

void EffectActionAnim::SetNamedAnim(const char * newName)
{
	bUseAnimName = true;
	targetAnim = NULL;
	strncpy(animName,newName,63);
	animName[63] = 0;
}

const char * EffectActionAnim::GetAnimName()
{
	return animName;
}

IEffectAction * MakeEffectActionAnim()
{
	return new EffectActionAnim();
}

