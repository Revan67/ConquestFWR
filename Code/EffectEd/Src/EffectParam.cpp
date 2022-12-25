//EffectParam.cpp

#include "stdafx.h"
#include "globals.h"
#include "IEffectParam.h"

struct EffectParam: public IEffectParam
{
	IEffectParam * next;
	char name[64];
	SINGLE value;

	EffectParam();

	~EffectParam();

	//IEffectTarget
	virtual IEffectParam * GetNextParam();

	virtual void SetNextParam(IEffectParam * target);

	virtual const char * GetName();

	virtual void SetName(const char * newName);

	virtual void SetValue(SINGLE value);

	virtual SINGLE GetValue();
};

EffectParam::EffectParam()
{
	strcpy(name,"newName");
	next = NULL;
	value = 0;
}

EffectParam::~EffectParam()
{
}

IEffectParam * EffectParam::GetNextParam()
{
	return next;
}

void EffectParam::SetNextParam(IEffectParam * target)
{
	next = target;
}

const char * EffectParam::GetName()
{
	return name;
}

void EffectParam::SetName(const char * newName)
{
	strncpy(name,newName,63);
	name[63] = 0;
}

void EffectParam::SetValue(SINGLE newValue)
{
	value = newValue;
}

SINGLE EffectParam::GetValue()
{
	return value;
}

IEffectParam * MakeEffectParam()
{
	return new EffectParam();
}

