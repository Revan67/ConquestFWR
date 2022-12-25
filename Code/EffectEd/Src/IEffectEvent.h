#ifndef _IEFFECTEVENT_H_
#define _IEFFECTEVENT_H_
//IEffectEvent.h

#include "IEffectFile.h"

#define END_OFFSET -1

struct __declspec(novtable) IEffectEvent
{
	virtual IEffectEvent * GetNextEvent() = 0;

	virtual void SetNextEvent(IEffectEvent * target) = 0;

	virtual struct IEffectAction * GetFirstAction() = 0;

	virtual void SetFirstAction(struct IEffectAction * target) = 0;

	virtual void RemoveAction(struct IEffectAction * target) = 0;

	virtual struct IEffectAction * GetParent() = 0;

	virtual void SetParent(struct IEffectAction * target) = 0;

	virtual const char * GetName() = 0;

	virtual void SetName(const char * newName) = 0;

	virtual void SetEventType(enum EventSave::EventType type) = 0;

	virtual enum EventSave::EventType GetEventType() = 0;

	virtual SINGLE GetOffset() = 0;

	virtual void SetOffset(SINGLE setting) = 0;

	virtual void TriggerEvent() = 0;

	virtual void SaveCore(struct IFileSystem * outFile) = 0;

	virtual void LoadCore(struct IFileSystem * inFile, U32 context) = 0;

	virtual void LoadCoreLate(struct IFileSystem * inFile, U32 context) = 0;

	virtual void SetFileDependant(bool bSetting) = 0;

	virtual bool IsFileDependant() = 0;

	virtual void NullTarget(struct IEffectTarget * target) = 0;

	virtual void NullTarget(struct ITargetAnim * target) = 0;

	virtual void NullTarget(struct ITargetHp * target) = 0;

	virtual IEffectEvent * CreateCopy(IEffectAction * _newParent) = 0;

	virtual void AddRef() = 0;

	virtual void RemoveRef() = 0;

	virtual U32 GetRef() = 0;

	virtual S32 GetIconXPos() = 0;

	virtual S32 GetIconYPos() = 0;

	virtual void SetDimentions(S32 newWidth, S32 newHeight) = 0;

	virtual bool HitTest(S32 x, S32 y) = 0;

	virtual void RealitveIconMove(S32 x, S32 y) = 0;

	virtual void SetIconPos(S32 x, S32 y) = 0;
};

IEffectEvent * MakeEffectEvent();

#endif