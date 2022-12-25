#ifndef _ITARGETCUE_H_
#define _ITARGETCUE_H_
//ITargetCue.h

struct __declspec(novtable) ITargetCue
{
	virtual ITargetCue * GetNextCue() = 0;

	virtual void SetNextCue(ITargetCue * target) = 0;

	virtual const char * GetName() = 0;

	virtual void SetName(const char * newName) = 0;

	virtual void SetTime(SINGLE setting) = 0;

	virtual SINGLE GetTime() = 0;
};

ITargetCue * MakeTargetCue();

#endif