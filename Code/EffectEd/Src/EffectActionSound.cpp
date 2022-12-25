//EffectAction.cpp

#include "stdafx.h"
#include "globals.h"
#include "IEffectEvent.h"
#include "IEffectAction.h"
#include "IEffectTarget.h"

#include "Archlist.h"
//#include <DSound.h>
#include <stdio.h>

#include <DACOM.h>
#include <filesys.h>
#include <TSmartPointer.h>

struct EffectActionSound: public EffectAction , IActionSound
{
	char soundName[64];

	SINGLE minSound;
	SINGLE maxSound;

	bool bLooping;

	IEffectTarget * soundTarget;
	char hpName[64];

	U32 context;

	EffectActionSound();

	~EffectActionSound();

	//IActionSound
	virtual void SetSoundEntry(const char * soundName);

	virtual const char * GetSoundEntry();

	virtual void SetTarget(struct IEffectTarget * target);

	virtual struct IEffectTarget * GetTarget();

	virtual SINGLE GetMinSound();

	virtual void SetMinSound(SINGLE setting);

	virtual SINGLE GetMaxSound();

	virtual void SetMaxSound(SINGLE setting);

	virtual bool IsLooping();

	virtual void SetLooping(bool bSetting);

	virtual void SetHpName(const char * setting);

	virtual const char * GetHpName();

	//IEffectAction
	virtual void Delete();

	virtual IActionSound * GetActionSound();

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
};

EffectActionSound::EffectActionSound()
{
	context = 0;
	strcpy(name,"newName");
	next = NULL;
	firstEvent = NULL;
	soundTarget = NULL;
	hpName[0] = 0;
	soundName[0] = 0;
	minSound = 1.0;
	maxSound = 30.0;
	bLooping = false;
}

EffectActionSound::~EffectActionSound()
{
	while(firstEvent)
	{
		IEffectEvent * tmp = firstEvent;
		firstEvent = firstEvent->GetNextEvent();
		delete tmp;
	}
}

void EffectActionSound::SetSoundEntry(const char * _soundName)
{
	strncpy(soundName,_soundName,63);
	soundName[63] = 0;
}

const char * EffectActionSound::GetSoundEntry()
{
	return soundName;
}

void EffectActionSound::SetTarget(struct IEffectTarget * target)
{
	soundTarget = target;
	hpName[0] = 0;
}

struct IEffectTarget * EffectActionSound::GetTarget()
{
	return soundTarget;
}

SINGLE EffectActionSound::GetMinSound()
{
	return minSound;
}

void EffectActionSound::SetMinSound(SINGLE setting)
{
	minSound = setting;
}

SINGLE EffectActionSound::GetMaxSound()
{
	return maxSound;
}

void EffectActionSound::SetMaxSound(SINGLE setting)
{
	maxSound = setting;
}

bool EffectActionSound::IsLooping()
{
	return bLooping;
}

void EffectActionSound::SetLooping(bool bSetting)
{
	bLooping = bSetting;
}

void EffectActionSound::SetHpName(const char * setting)
{
	strncpy(hpName,setting,63);
	hpName[63] = 0;
}

const char * EffectActionSound::GetHpName()
{
	return hpName;
}

void EffectActionSound::Delete()
{
	delete this;
}

IActionSound * EffectActionSound::GetActionSound()
{
	return this;
}

void EffectActionSound::StartAction()
{
	if(soundName)
	{
/*		BT_SOUNDENTRY * entry = (BT_SOUNDENTRY *)(ARCHLIST->GetArchetypeData(soundName));
		if(entry)
		{
			SoundDef def;
			def.priority = 0;
			def.dir = SOUNDDIR;
			strcpy(def.filename,entry->filename);
			def.bStreaming = entry->bStreaming;
			def.bUse3D = true;

			SoundHandle sHandle = SOUNDSYS->LoadSound(def);
			if(sHandle)
			{
				if(soundTarget)
				{
					U32 hpIndex = soundTarget->GetHardPointIndex(hpName);
					Transform trans;
					if(hpIndex != INVALID_HARD_POINT)
						soundTarget->GetHardPointTransform(hpIndex,trans);
					else
						trans = soundTarget->GetTransform();
					
					SoundInstance inst = SOUNDSYS->Play3D(sHandle,trans.translation,soundTarget->GetTargetID(),hpIndex,bLooping);
					SOUNDSYS->SetMinMaxDist(inst,minSound*FEET_TO_WORLD,maxSound*FEET_TO_WORLD);
				}
				else
				{
					SOUNDSYS->Play(sHandle,bLooping);
				}
				SOUNDSYS->ReleaseSound(sHandle);
			}
		}
*/	}
}

void EffectActionSound::TriggerEvent(const char * event)
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

void EffectActionSound::AnimationFinished()
{
}

S32 EffectActionSound::GetDrawWidth()
{
	return 0;
}

bool EffectActionSound::IsOpen()
{
	return true;
}

void EffectActionSound::SetOpen(bool setting)
{
}

void EffectActionSound::SaveCore(IFileSystem * outFile)
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
		aSave.actionType = ActionSaveHeader::AT_SOUND;
		actionFile->WriteFile(0,&aSave ,sizeof(ActionSaveHeader),&dwWritten);
	}

	DAFILEDESC fdesc2 = "SOUNDDATA";
	fdesc2.lpImplementation = "DOS";
	fdesc2.dwDesiredAccess = GENERIC_READ|GENERIC_WRITE;
//	fdesc2.dwShareMode = 0;  // no sharing
	fdesc2.dwCreationDistribution = CREATE_NEW;
	if (outFile->CreateInstance(&fdesc2, actionFile) == GR_OK)
	{
		ActionSoundSave soundSave;
		if(soundTarget)
			soundSave.targetID = soundTarget->GetTargetID();
		else
			soundSave.targetID = -1;
		strcpy(soundSave.hpName,hpName);
		strcpy(soundSave.soundName,soundName);
		soundSave.bLooping = bLooping;
		soundSave.minRange = minSound;
		soundSave.maxRange = maxSound;

		strcpy(soundSave.actionName,name);
		soundSave.xPos = iconXPos;
		soundSave.yPos = iconYPos;
		actionFile->WriteFile(0,&soundSave ,sizeof(ActionSoundSave),&dwWritten);
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

void EffectActionSound::LoadCore(struct IFileSystem * outFile, U32 _context, U32 version)
{
	context = _context;
	U32 dwWritten;
	DAFILEDESC fdesc = "SOUNDDATA";
	fdesc.dwDesiredAccess = GENERIC_READ;
	fdesc.dwShareMode = 0;  // no sharing
	
	IEffectAction * action = NULL;
	COMPTR<IFileSystem> actionData;
	if (outFile->CreateInstance(&fdesc, actionData) == GR_OK)
	{
		ActionSoundSave save;
		actionData->ReadFile(0,&(save) ,sizeof(ActionSoundSave),&dwWritten);
		strcpy(name,save.actionName);
		strcpy(hpName,save.hpName);
		strcpy(soundName,save.soundName);
		bLooping = save.bLooping;
		minSound = save.minRange;
		maxSound = save.maxRange;
		iconXPos = save.xPos;
		iconYPos = save.yPos;

		IEffectTarget * targ = EFFECTFILE->FindTargetByID(save.targetID,context);
		if(targ)
		{
			soundTarget = targ;
		}
	}
}

void EffectActionSound::NullTarget(struct IEffectTarget * target)
{
	if(target == soundTarget)
	{
		soundTarget = NULL;
		hpName[0] = 0;
	}
	IEffectEvent * search = firstEvent;
	while(search)
	{
		search->NullTarget(target);
		search = search->GetNextEvent();
	}
}

void EffectActionSound::NullTarget(struct ITargetAnim * target)
{
	IEffectEvent * search = firstEvent;
	while(search)
	{
		search->NullTarget(target);
		search = search->GetNextEvent();
	}
}

void EffectActionSound::NullTarget(struct ITargetHp * target)
{
	IEffectEvent * search = firstEvent;
	while(search)
	{
		search->NullTarget(target);
		search = search->GetNextEvent();
	}
}

IEffectAction * EffectActionSound::CreateCopy()
{
	EffectActionSound * action = new EffectActionSound();
	strcpy(action->name,name);
	if(firstEvent)
		action->firstEvent = firstEvent->CreateCopy(action);
	else
		action->firstEvent = NULL;
	if(next)
		action->next = next->CreateCopy();
	else
		action->next = NULL;
	action->soundTarget = soundTarget;
	action->maxSound = maxSound;
	action->minSound = minSound;
	action->bLooping = bLooping;
	strcpy(action->hpName,hpName);
	strcpy(action->soundName,soundName);
	return action;
}

IEffectAction * MakeEffectActionSound()
{
	return new EffectActionSound();
}

