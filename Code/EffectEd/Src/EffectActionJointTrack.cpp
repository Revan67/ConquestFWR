//ActionJointTrack.cpp

#include "stdafx.h"
#include "globals.h"
#include "IEffectEvent.h"
#include "IEffectAction.h"
#include "IEffectTarget.h"
#include "PreviewWin.h"
#include "SuperTrans.h"

#include <DACOM.h>
#include <filesys.h>
#include <TSmartPointer.h>
#include <IMeshManager.h>
#include <ENGINE.h>
#include <stdio.h>
#include <stdlib.h>

struct ActionJointTrack: public EffectAction, IActionJointTrack
{
	char jointName[64];
	IEffectTarget * effectSource;
	IEffectTarget * effectTarget;
	SINGLE angVelocity;

	U32 context;

	ActionJointTrack();

	~ActionJointTrack();

	//IEffectAction
	virtual void Delete();

	virtual IActionJointTrack * GetActionJointTrack();

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

	virtual void UpdateAction();

	//IActionJointTrack
	virtual void SetJointName(const char * newJointName);

	virtual char * GetJointName();

	virtual void SetTarget(struct IEffectTarget * target);

	virtual struct IEffectTarget * GetTarget();

	virtual void SetSource(struct IEffectTarget * source);

	virtual struct IEffectTarget * GetSource();

	virtual void SetAngVelocity(SINGLE newVel);

	virtual SINGLE GetAngVelocity();
};

ActionJointTrack::ActionJointTrack()
{
	context = 0;
	strcpy(name,"newName");
	next = NULL;
	firstEvent = NULL;
	jointName[0] = 0;
	effectSource = NULL;
	effectTarget = NULL;
	angVelocity = 0.0f;
}

ActionJointTrack::~ActionJointTrack()
{
	while(firstEvent)
	{
		IEffectEvent * tmp = firstEvent;
		firstEvent = firstEvent->GetNextEvent();
		delete tmp;
	}
}

void ActionJointTrack::Delete()
{
	delete this;
}

IActionJointTrack * ActionJointTrack::GetActionJointTrack()
{
	return this;
}

void ActionJointTrack::StartAction()
{
	if(effectTarget)
	{
		effectTarget->AddToUpdateList(this);
	}
}

void ActionJointTrack::TriggerEvent(const char * event)
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

void ActionJointTrack::AnimationFinished()
{
}

S32 ActionJointTrack::GetDrawWidth()
{
	return 0;
}

bool ActionJointTrack::IsOpen()
{
	return true;
}

void ActionJointTrack::SetOpen(bool setting)
{
}

void ActionJointTrack::SaveCore(IFileSystem * outFile)
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
		aSave.actionType = ActionSaveHeader::AT_JOINT_TRACK;
		actionFile->WriteFile(0,&aSave ,sizeof(ActionSaveHeader),&dwWritten);
	}

	DAFILEDESC fdesc2 = "JOINTTRACKDATA";
	fdesc2.lpImplementation = "DOS";
	fdesc2.dwDesiredAccess = GENERIC_READ|GENERIC_WRITE;
//	fdesc2.dwShareMode = 0;  // no sharing
	fdesc2.dwCreationDistribution = CREATE_NEW;
	if (outFile->CreateInstance(&fdesc2, actionFile) == GR_OK)
	{
		ActionJointTrackSave jointTrackSave;
		strcpy(jointTrackSave.jointName,jointName);
		if(effectTarget)
			jointTrackSave.targetID = effectTarget->GetTargetID();
		else
			jointTrackSave.targetID = 0;
		if(effectSource)
			jointTrackSave.sourceID = effectSource->GetTargetID();
		else
			jointTrackSave.sourceID = 0;
		jointTrackSave.angVelocity = angVelocity;
		strcpy(jointTrackSave.actionName,name);
		jointTrackSave.xPos = iconXPos;
		jointTrackSave.yPos = iconYPos;
		actionFile->WriteFile(0,&jointTrackSave ,sizeof(jointTrackSave),&dwWritten);
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

void ActionJointTrack::LoadCore(struct IFileSystem * inFile, U32 _context, U32 version)
{
	context = _context;
	U32 dwWritten;
	DAFILEDESC fdesc = "JOINTTRACKDATA";
	fdesc.dwDesiredAccess = GENERIC_READ;
	fdesc.dwShareMode = 0;  // no sharing
	
	IEffectAction * action = NULL;
	COMPTR<IFileSystem> actionData;
	if (inFile->CreateInstance(&fdesc, actionData) == GR_OK)
	{
		ActionJointTrackSave jointTrackSave;
		actionData->ReadFile(0,&(jointTrackSave) ,sizeof(ActionJointTrackSave),&dwWritten);
		strcpy(jointName,jointTrackSave.jointName);
		strcpy(name,jointTrackSave.actionName);
		iconXPos = jointTrackSave.xPos;
		iconYPos = jointTrackSave.yPos;
		effectTarget = EFFECTFILE->FindTargetByID(jointTrackSave.targetID,context);
		effectSource = EFFECTFILE->FindTargetByID(jointTrackSave.sourceID,context);
		angVelocity = jointTrackSave.angVelocity;
	}
}

void ActionJointTrack::NullTarget(struct IEffectTarget * target)
{
	if(effectTarget == target)
		effectTarget = NULL;
	if(effectSource == target)
		effectSource = NULL;
	IEffectEvent * search = firstEvent;
	while(search)
	{
		search->NullTarget(target);
		search = search->GetNextEvent();
	}
}

void ActionJointTrack::NullTarget(struct ITargetAnim * target)
{
	IEffectEvent * search = firstEvent;
	while(search)
	{
		search->NullTarget(target);
		search = search->GetNextEvent();
	}
}

void ActionJointTrack::NullTarget(struct ITargetHp * target)
{
	IEffectEvent * search = firstEvent;
	while(search)
	{
		search->NullTarget(target);
		search = search->GetNextEvent();
	}
}

IEffectAction * ActionJointTrack::CreateCopy()
{
	ActionJointTrack * action = new ActionJointTrack();
	strcpy(action->name,name);
	if(firstEvent)
		action->firstEvent = firstEvent->CreateCopy(action);
	else
		action->firstEvent = NULL;
	if(next)
		action->next = next->CreateCopy();
	else
		action->next = NULL;
	strcpy(action->jointName,jointName);
	action->effectTarget = effectTarget;
	action->effectSource = effectSource;
	action->angVelocity = angVelocity;
	return action;
}

void ActionJointTrack::UpdateAction()
{
	if(effectTarget && effectSource)
	{
		IMeshInstance * mesh = effectSource->GetMesh();
		if(mesh)
		{
			ARCHETYPE_INDEX jointIndex = mesh->FindChild(jointName);
			if(jointIndex != INVALID_ARCHETYPE_INDEX)
			{
				//compute target yaw
				SINGLE rot;
				TRANSFORM transform = ENGINE->get_transform(jointIndex);
				Vector pos = transform.get_position();
				SINGLE yaw = transform.get_yaw();
				Vector targetPos = effectTarget->GetTransform().translation;
				Vector goal = targetPos - pos;
				ENGINE->get_joint_state(jointIndex, IE_JST_BASIC, &rot);
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
					SINGLE dt = PreviewWin::GetRenderTime();
					BOOL32 noman = 0;
					JointInfo const *jnt = ENGINE->get_joint_info(jointIndex);
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
							SINGLE minVal = __min(angVelocity*MUL_DEG_TO_RAD*dt, -relYaw);
							rot += minVal;
							relYaw += minVal;
						}
						else
						{
							SINGLE minVal = -__min(angVelocity*MUL_DEG_TO_RAD*dt, relYaw);
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
							ENGINE->set_joint_state(jointIndex, IE_JST_BASIC, &rot);
						}
					}
				}
			}
		}
	}
}

void ActionJointTrack::SetJointName(const char * newJointName)
{
	strcpy(jointName,newJointName);
}

char * ActionJointTrack::GetJointName()
{
	return jointName;
}

void ActionJointTrack::SetTarget(struct IEffectTarget * target)
{
	effectTarget = target;
}

struct IEffectTarget * ActionJointTrack::GetTarget()
{
	return effectTarget;
}

void ActionJointTrack::SetSource(struct IEffectTarget * source)
{
	effectSource = source;
}

struct IEffectTarget * ActionJointTrack::GetSource()
{
	return effectSource;
}

void ActionJointTrack::SetAngVelocity(SINGLE newVel)
{
	angVelocity = newVel;
}

SINGLE ActionJointTrack::GetAngVelocity()
{
	return angVelocity;
}

IEffectAction * MakeEffectActionJointTrack()
{
	return new ActionJointTrack();
}


