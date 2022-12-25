//TargetHp.cpp

#include "stdafx.h"
#include "globals.h"
#include "ITargetHp.h"
#include "IEffectFile.h"

struct TargetHp: public ITargetHp
{
	ITargetHp * next;
	char name[64];

	U32 hpIndex;

	TargetHp();

	~TargetHp();

	//IEffectTarget
	virtual ITargetHp * GetNextHP();

	virtual void SetNextHP(ITargetHp * target);

	virtual const char * GetName();

	virtual void SetName(const char * newName);

	virtual void SetIndex(U32 index);

	virtual U32 GetIndex();
};

TargetHp::TargetHp()
{
	strcpy(name,"newName");
	next = NULL;
	hpIndex = INVALID_HARD_POINT;
}

TargetHp::~TargetHp()
{
	EFFECTFILE->NullTarget(this);
}

ITargetHp * TargetHp::GetNextHP()
{
	return next;
}

void TargetHp::SetNextHP(ITargetHp * target)
{
	next = target;
}

const char * TargetHp::GetName()
{
	return name;
}

void TargetHp::SetName(const char * newName)
{
	strncpy(name,newName,63);
	name[63] = 0;
}

void TargetHp::SetIndex(U32 index)
{
	hpIndex = index;
}

U32 TargetHp::GetIndex()
{
	return hpIndex;
}

ITargetHp * MakeTargetHp()
{
	return new TargetHp();
}

void DeleteTargetHp(ITargetHp * target)
{
	delete ((TargetHp *)target);
}


