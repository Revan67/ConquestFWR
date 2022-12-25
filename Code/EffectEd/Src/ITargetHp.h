#ifndef _ITARGETHP_H_
#define _ITARGETHP_H_
//ITargetHp.h

struct __declspec(novtable) ITargetHp
{
	virtual ITargetHp * GetNextHP() = 0;

	virtual void SetNextHP(ITargetHp * target) = 0;

	virtual const char * GetName() = 0;

	virtual void SetName(const char * newName) = 0;

	virtual void SetIndex(U32 index) = 0;

	virtual U32 GetIndex() = 0;
};

ITargetHp * MakeTargetHp();

void DeleteTargetHp(ITargetHp * target);

#endif