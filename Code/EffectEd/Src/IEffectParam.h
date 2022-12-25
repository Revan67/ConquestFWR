#ifndef _IEFFECTPARAM_H_
#define _IEFFECTPARAM_H_
//ITargetHp.h

struct __declspec(novtable) IEffectParam
{
	virtual IEffectParam * GetNextParam() = 0;

	virtual void SetNextParam(IEffectParam * target) = 0;

	virtual const char * GetName() = 0;

	virtual void SetName(const char * newName) = 0;

	virtual void SetValue(SINGLE value) = 0;

	virtual SINGLE GetValue() = 0;
};

IEffectParam * MakeEffectParam();

#endif