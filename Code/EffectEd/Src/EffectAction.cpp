//EffectAction.cpp

#include "stdafx.h"
#include "globals.h"
#include "IEffectEvent.h"
#include "IEffectAction.h"
#include "EventGraph.h"

#include <DACOM.h>
#include <filesys.h>
#include <TSmartPointer.h>

EffectAction::EffectAction()
{
	strcpy(name,"newName");
	next = NULL;
	firstEvent = NULL;

	iconXPos = 0;
	iconYPos = 0;
	iconWidth = 0;
	iconHeight = 0;
}

EffectAction::~EffectAction()
{
	while(firstEvent)
	{
		IEffectEvent * tmp = firstEvent;
		firstEvent = firstEvent->GetNextEvent();
		delete tmp;
	}
}

void EffectAction::Delete()
{
	delete this;
}

IEffectAction * EffectAction::GetNextAction()
{
	return next;
}

void EffectAction::SetNextAction(IEffectAction * target)
{
	next = target;
}

IEffectEvent * EffectAction::GetFirstEvent()
{
	return firstEvent;
}

void EffectAction::SetFirstEvent(struct IEffectEvent * target)
{
	firstEvent = target;

	S32 testPosX = 200;
	S32 testPosY = 0;
	IEffectEvent * search = firstEvent;
	while(search)
	{
		if(search->GetParent() != this)
		{
			search->SetParent(this);
			search->SetIconPos(testPosX,testPosY);
		}
		testPosY += (iconHeight +5);
		search = search->GetNextEvent();
	}
	EventGraph::InvalidateEventGraph();
}

const char * EffectAction::GetName()
{
	return name;
}

void EffectAction::SetName(const char * newName)
{
	strncpy(name,newName,63);
	name[63] = 0;
}

IActionAnimation * EffectAction::GetActionAnimation()
{
	return NULL;
}

IActionParticle * EffectAction::GetActionParticle()
{
	return NULL;
}

IActionGameEvent * EffectAction::GetActionGameEvent()
{
	return NULL;
}

IActionSound * EffectAction::GetActionSound()
{
	return NULL;
}

IActionSwitch * EffectAction::GetActionSwitch()
{
	return NULL;
}

IActionHideTarget * EffectAction::GetActionHideTarget()
{
	return NULL;
}

IActionListen * EffectAction::GetActionListen()
{
	return NULL;
}

IActionJointTrack * EffectAction::GetActionJointTrack()
{
	return NULL;
}

void EffectAction::StartAction()
{
}

void EffectAction::TriggerEvent(const char * event)
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

void EffectAction::AnimationFinished()
{
}

S32 EffectAction::GetDrawWidth()
{
	return 0;
}

bool EffectAction::IsOpen()
{
	return true;
}

void EffectAction::SetOpen(bool setting)
{
}

void EffectAction::SaveCore(IFileSystem * outFile)
{
}

void EffectAction::LoadCore(struct IFileSystem * inFile, U32 _context, U32 version)
{
}

void EffectAction::NullTarget(struct IEffectTarget * target)
{
	IEffectEvent * search = firstEvent;
	while(search)
	{
		search->NullTarget(target);
		search = search->GetNextEvent();
	}
}

void EffectAction::NullTarget(struct ITargetAnim * target)
{
	IEffectEvent * search = firstEvent;
	while(search)
	{
		search->NullTarget(target);
		search = search->GetNextEvent();
	}
}

void EffectAction::NullTarget(struct ITargetHp * target)
{
	IEffectEvent * search = firstEvent;
	while(search)
	{
		search->NullTarget(target);
		search = search->GetNextEvent();
	}
}

IEffectAction * EffectAction::CreateCopy()
{
	EffectAction * action = new EffectAction();
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

S32 EffectAction::GetIconXPos()
{
	if(parent)
		return parent->GetIconXPos()+iconXPos;
	return iconXPos;
}

S32 EffectAction::GetIconYPos()
{
	if(parent)
		return parent->GetIconYPos()+iconYPos;
	return iconYPos;
}

IEffectEvent * EffectAction::GetParent()
{
	return parent;
}

void EffectAction::SetParent(IEffectEvent* event)
{
	parent = event;
}

void EffectAction::SetDimentions(S32 newWidth, S32 newHeight)
{
	iconWidth = newWidth;
	iconHeight = newHeight;
}

bool EffectAction::HitTest(S32 x, S32 y)
{
	if(x >= GetIconXPos() && x <= GetIconXPos()+iconWidth && y >= GetIconYPos() && y <= GetIconYPos()+iconHeight)
		return true;
	return false;
}

void EffectAction::RealitveIconMove(S32 x, S32 y)
{
	iconXPos += x;
	iconYPos += y;
}

void EffectAction::SetIconPos(S32 x, S32 y)
{
	iconXPos = x;
	iconYPos = y;
}

void EffectAction::UpdateAction()
{
}

IEffectAction * MakeEffectAction()
{
	return new EffectAction();
}

IEffectAction * LoadActionCore(struct IFileSystem * inFile, U32 context)
{
	U32 dwWritten;
	DAFILEDESC fdesc = "ACTIONDATA";
	fdesc.dwDesiredAccess = GENERIC_READ;
	fdesc.dwShareMode = 0;  // no sharing
	
	IEffectAction * action = NULL;
	COMPTR<IFileSystem> actionData;
	if (inFile->CreateInstance(&fdesc, actionData) == GR_OK)
	{
		ActionSaveHeader save;
		actionData->ReadFile(0,&(save) ,sizeof(ActionSaveHeader),&dwWritten);

		switch(save.actionType)
		{
		case ActionSaveHeader::AT_ANIM:
			{
				action = MakeEffectActionAnim();
				action->LoadCore(inFile,context,save.version);
			}
			break;
		case ActionSaveHeader::AT_PARTICLE:
			{
				action = MakeEffectActionParticle();
				action->LoadCore(inFile,context,save.version);
			}
			break;
		case ActionSaveHeader::AT_GAME_EVENT:
			{
				action = MakeEffectActionGameEvent();
				action->LoadCore(inFile,context,save.version);
			}
			break;
		case ActionSaveHeader::AT_SOUND:
			{
				action = MakeEffectActionSound();
				action->LoadCore(inFile,context,save.version);
			}
			break;
		case ActionSaveHeader::AT_SWITCH:
			{
				action = MakeEffectActionSwitch();
				action->LoadCore(inFile,context,save.version);
			}
			break;
		case ActionSaveHeader::AT_HIDE_TARGET:
			{
				action = MakeEffectActionHideTarget();
				action->LoadCore(inFile,context,save.version);
			}
			break;
		case ActionSaveHeader::AT_LISTEN_ACTION:
			{
				action = MakeEffectActionListen();
				action->LoadCore(inFile,context,save.version);
			}
			break;
		case ActionSaveHeader::AT_JOINT_TRACK:
			{
				action = MakeEffectActionJointTrack();
				action->LoadCore(inFile,context,save.version);
			}
			break;
		default:
			return NULL;
		}

		WIN32_FIND_DATA data;
		HANDLE handle;
		handle = inFile->FindFirstFile("Event*",&data);
		if(handle != INVALID_HANDLE_VALUE)
		{
			do
			{
				if(data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				{
					inFile->SetCurrentDirectory(data.cFileName);
					DAFILEDESC fdesc = "EVENTDATA";
					fdesc.dwDesiredAccess = GENERIC_READ;
					fdesc.dwShareMode = 0;  // no sharing
						
					COMPTR<IFileSystem> baseData;
					if (inFile->CreateInstance(&fdesc, baseData) == GR_OK)
					{
						EventSave save;
						U32 dwWritten;
						baseData->ReadFile(0,&save ,sizeof(EventSave),&dwWritten);

						//EventSave::ANIM_CUE_TRIGGERED
						if(save.type == EventSave::ANIM_CUE_TRIGGERED)
						{
							IEffectEvent * event = action->GetFirstEvent();
							while(event)
							{
								if(event->GetEventType() == save.type && (strcmp(save.stingParam,event->GetName()) == 0))
								{
									event->LoadCoreLate(inFile,context);
									break;
								}
								event = event->GetNextEvent();
							}
						}
						if(save.type == EventSave::ANIM_FINISHED)
						{
							IEffectEvent * event = action->GetFirstEvent();
							while(event)
							{
								if(event->GetEventType() == save.type)
								{
									event->LoadCoreLate(inFile,context);
									break;
								}
								event = event->GetNextEvent();
							}
						}
						if(save.type == EventSave::PARTICLE_CUE_TRIGGERED)
						{
							IEffectEvent * event = action->GetFirstEvent();
							while(event)
							{
								if(event->GetEventType() == save.type && (strcmp(save.stingParam,event->GetName()) == 0))
								{
									event->LoadCoreLate(inFile,context);
									break;
								}
								event = event->GetNextEvent();
							}
						}
						if(save.type == EventSave::SWITCH)
						{
							IEffectEvent * event = action->GetFirstEvent();
							while(event)
							{
								if(event->GetEventType() == save.type && (strcmp(save.stingParam,event->GetName()) == 0))
								{
									event->LoadCoreLate(inFile,context);
									break;
								}
								event = event->GetNextEvent();
							}
						}
						if(save.type == EventSave::LISTEN_EVENT)
						{
							IEffectEvent * event = action->GetFirstEvent();
							while(event)
							{
								if(event->GetEventType() == save.type)
								{
									event->LoadCoreLate(inFile,context);
									break;
								}
								event = event->GetNextEvent();
							}
						}
					}
					inFile->SetCurrentDirectory("..");
				}
			}while(inFile->FindNextFile(handle,&data));
			inFile->FindClose(handle);
		}
	}
	return action;
};


