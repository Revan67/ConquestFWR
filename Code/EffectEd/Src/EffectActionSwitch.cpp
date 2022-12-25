//EffectActionSwitch.cpp

#include "stdafx.h"
#include "globals.h"
#include "IEffectEvent.h"
#include "IEffectAction.h"

#include <DACOM.h>
#include <filesys.h>
#include <TSmartPointer.h>

#include <stdio.h>
#include <stdlib.h>

struct EffectActionSwitch: public EffectAction, IActionSwitch
{
	U32 numEvents;

	bool bOpen;

	U32 context;

	EffectActionSwitch();

	~EffectActionSwitch();

	//IActionSwitch

	virtual U32 GetSwitchNumber();

	virtual void SetSwitchNumber(U32 setting);

	//IEffectAction
	virtual void Delete();

	virtual IActionSwitch * GetActionSwitch();

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

	//EffectActionSwitch

	void updateEvents();
};

EffectActionSwitch::EffectActionSwitch()
{
	context = 0;
	bOpen = false;
	strcpy(name,"newName");
	next = NULL;
	firstEvent = NULL;
	numEvents = 2;

	updateEvents();
}

EffectActionSwitch::~EffectActionSwitch()
{
	while(firstEvent)
	{
		IEffectEvent * tmp = firstEvent;
		firstEvent = firstEvent->GetNextEvent();
		delete tmp;
	}
}

U32 EffectActionSwitch::GetSwitchNumber()
{
	return numEvents;
}

void EffectActionSwitch::SetSwitchNumber(U32 setting)
{
	numEvents = setting;
	updateEvents();
}

void EffectActionSwitch::Delete()
{
	delete this;
}

IActionSwitch * EffectActionSwitch::GetActionSwitch()
{
	return this;
}

void EffectActionSwitch::StartAction()
{
	if(numEvents > 0)
	{
		U32 num = rand()%numEvents;
		IEffectEvent * search = firstEvent;
		while(num)
		{
			--num;
			search = search->GetNextEvent();
		}
		search->TriggerEvent();
	}
}

void EffectActionSwitch::TriggerEvent(const char * event)
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

void EffectActionSwitch::AnimationFinished()
{
}

S32 EffectActionSwitch::GetDrawWidth()
{
	return 0;
}

bool EffectActionSwitch::IsOpen()
{
	return bOpen;
}

void EffectActionSwitch::SetOpen(bool setting)
{
	bOpen = setting;
}

void EffectActionSwitch::SaveCore(IFileSystem * outFile)
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
		aSave.actionType = ActionSaveHeader::AT_SWITCH;
		actionFile->WriteFile(0,&aSave ,sizeof(ActionSaveHeader),&dwWritten);
	}

	DAFILEDESC fdesc2 = "SWITCHDATA";
	fdesc2.lpImplementation = "DOS";
	fdesc2.dwDesiredAccess = GENERIC_READ|GENERIC_WRITE;
//	fdesc2.dwShareMode = 0;  // no sharing
	fdesc2.dwCreationDistribution = CREATE_NEW;
	if (outFile->CreateInstance(&fdesc2, actionFile) == GR_OK)
	{
		ActionSwitchSave switchSave;
		switchSave.numEvents = numEvents;

		strcpy(switchSave.actionName,name);
		switchSave.xPos = iconXPos;
		switchSave.yPos = iconYPos;
		actionFile->WriteFile(0,&switchSave ,sizeof(ActionSwitchSave),&dwWritten);
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

void EffectActionSwitch::LoadCore(struct IFileSystem * outFile, U32 _context, U32 version)
{
	context = _context;
	U32 dwWritten;
	DAFILEDESC fdesc = "SWITCHDATA";
	fdesc.dwDesiredAccess = GENERIC_READ;
	fdesc.dwShareMode = 0;  // no sharing
	
	IEffectAction * action = NULL;
	COMPTR<IFileSystem> actionData;
	if (outFile->CreateInstance(&fdesc, actionData) == GR_OK)
	{
		ActionSwitchSave save;
		actionData->ReadFile(0,&(save) ,sizeof(ActionSwitchSave),&dwWritten);
		strcpy(name,save.actionName);
		numEvents = save.numEvents;
		iconXPos = save.xPos;
		iconYPos = save.yPos;

		updateEvents();
	}
}

void EffectActionSwitch::NullTarget(struct IEffectTarget * target)
{
	IEffectEvent * search = firstEvent;
	while(search)
	{
		search->NullTarget(target);
		search = search->GetNextEvent();
	}
}

void EffectActionSwitch::NullTarget(struct ITargetAnim * target)
{
	IEffectEvent * search = firstEvent;
	while(search)
	{
		search->NullTarget(target);
		search = search->GetNextEvent();
	}
}

void EffectActionSwitch::NullTarget(struct ITargetHp * target)
{
	IEffectEvent * search = firstEvent;
	while(search)
	{
		search->NullTarget(target);
		search = search->GetNextEvent();
	}
}

IEffectAction * EffectActionSwitch::CreateCopy()
{
	EffectActionSwitch * action = new EffectActionSwitch();
	strcpy(action->name,name);
	if(firstEvent)
		action->firstEvent = firstEvent->CreateCopy(action);
	else
		action->firstEvent = NULL;
	if(next)
		action->next = next->CreateCopy();
	else
		action->next = NULL;
	return action;
}

void EffectActionSwitch::updateEvents()
{
	IEffectEvent * oldList = firstEvent;

	firstEvent = NULL;
	for(U32 i = 0; i < numEvents; ++i)
	{
		char buffer[256];
		sprintf(buffer,"Choice%d",i);
		IEffectEvent * prev = NULL;
		IEffectEvent * search = oldList;
		while(search)
		{
			if(strcmp(buffer,search->GetName()) == 0)
			{
				break;
			}
			prev = search;
			search = search->GetNextEvent();
		}
		if(search)
		{
			if(prev)
				prev->SetNextEvent(search->GetNextEvent());
			else
				oldList = search->GetNextEvent();
			search->SetNextEvent(firstEvent);
			firstEvent = search;
		}
		else
		{
			search = MakeEffectEvent();
			search->SetEventType(EventSave::SWITCH);
			search->SetName(buffer);
			search->SetOffset((SINGLE)(i*10));
			search->SetNextEvent(firstEvent);
			search->SetParent(this);
			search->SetIconPos(200,i*50);
			SetFirstEvent(search);
		}
	}

	while(oldList)
	{
		IEffectEvent * tmp = oldList;
		oldList = oldList->GetNextEvent();
		delete tmp;
	}
}

IEffectAction * MakeEffectActionSwitch()
{
	return new EffectActionSwitch();
}


