//TargetCue.cpp

#include "stdafx.h"
#include "globals.h"
#include "ITargetCue.h"

struct TargetCue: public ITargetCue
{
	ITargetCue * next;
	char name[64];
	SINGLE time;

	TargetCue();

	~TargetCue();

	//IEffectTarget
	virtual ITargetCue * GetNextCue();

	virtual void SetNextCue(ITargetCue * target);

	virtual const char * GetName();

	virtual void SetName(const char * newName);

	virtual void SetTime(SINGLE setting);

	virtual SINGLE GetTime();
};

TargetCue::TargetCue()
{
	strcpy(name,"newName");
	next = NULL;
	time = 0;
}

TargetCue::~TargetCue()
{
}

ITargetCue * TargetCue::GetNextCue()
{
	return next;
}

void TargetCue::SetNextCue(ITargetCue * target)
{
	next = target;
}

const char * TargetCue::GetName()
{
	return name;
}

void TargetCue::SetName(const char * newName)
{
	strncpy(name,newName,63);
	name[63] = 0;
}

void TargetCue::SetTime(SINGLE setting)
{
	time = setting;
}

SINGLE TargetCue::GetTime()
{
	return time;
}

ITargetCue * MakeTargetCue()
{
	return new TargetCue();
}

