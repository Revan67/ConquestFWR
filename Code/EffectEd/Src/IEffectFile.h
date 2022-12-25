#ifndef _IEFFECTFILE_H_
#define _IEFFECTFILE_H_
//IEffectFile.h

#define EFFECT_SAVE_VERSION 1 //should only need to change this if a structure changes size

struct EventSave
{
	enum EventType // always add to the end of this list, NEVER remove a value once it is in a version
	{
		START_TRIGGERED = 0,
		ANIM_CUE_TRIGGERED,
		ANIM_FINISHED,
		PARTICLE_CUE_TRIGGERED,
		SWITCH,
		LISTEN_EVENT,
	};
	EventType type;
	char stingParam[64];//the name of the cue... etc.
	U32 targetID;
	S32 xPos;
	S32 yPos;
};

//version 1->2
	//Animation actions did not have atttachID

//version 2->3
	//Game Action were changed to support responces and distance approximation

#define ACTION_SAVE_VERSION 3 //should only need to change this if a structure changes size

struct ActionSaveHeader
{
	U32 version;
	enum ActionType
	{
		AT_ANIM = 0,
		AT_PARTICLE,
		AT_GAME_EVENT,
		AT_SOUND,
		AT_SWITCH,
		AT_HIDE_TARGET,
		AT_LISTEN_ACTION,
		AT_JOINT_TRACK
	};
	ActionType actionType;
};

struct ActionJointTrackSave
{
	U32 sourceID;
	U32 targetID;
	SINGLE angVelocity;
	char actionName[64];
	char jointName[64];
	S32 xPos;
	S32 yPos;
};

struct ActionListenSave
{
	U32 targetID;
	char actionName[64];
	char listenName[64];
	S32 xPos;
	S32 yPos;
};

struct ActionHideTargetSave
{
	U32 targetID;
	char actionName[64];
	bool bHide:1;
	S32 xPos;
	S32 yPos;
};

struct ActionAnimSave
{
	U32 targetID;
	char animName[64];
	char actionName[64];
	bool bLooping:1;
	bool bForced:1;
	S32 xPos;
	S32 yPos;
};

struct ActionParticleSave
{
	char actionName[64];
	S32 xPos;
	S32 yPos;
};

struct ActionGameEventSave
{
	U32 targetID;
	char actionName[64];
	char eventName[64];
	bool bResponce:1;
	bool bDistanceDependant:1;
	bool bEndEffect:1;
	float speed;
	S32 xPos;
	S32 yPos;
};

struct ActionSoundSave
{
	U32 targetID;
	char hpName[64];
	char soundName[64];
	char actionName[64];
	bool bLooping;
	SINGLE minRange;
	SINGLE maxRange;
	S32 xPos;
	S32 yPos;
};

struct ActionSwitchSave
{
	char actionName[64];
	U32 numEvents;
	S32 xPos;
	S32 yPos;
};

#define FILTER_SAVE_VERSION 1 //should only need to change this if a structure changes size

struct FilterSaveHeader
{
	U32 version;
	enum ParticleEffectType type;
	U32 dataSize;
	U32 numOutput;
	U32 numInput;
	S32 xPos;
	S32 yPos;
	U32 linkID;
	//U32 outputLinkID[]
	//U32 outputTargetID[]
	//U32 inputTargetID[]
};

struct __declspec(novtable) IEffectFile
{
	virtual void New() = 0;

	virtual void Load(const char * fileName) = 0;

	virtual void Save(const char * fileName) = 0;

	virtual const char * GetFileName() = 0;

	virtual struct IEffectTarget * GetFirstTarget() = 0;

	virtual struct IEffectTarget * AddTarget(const char * name) = 0;

	virtual struct IEffectTarget * FindTargetByID(U32 id, U32 context) = 0;

	virtual void RemoveEffectTarget(struct IEffectTarget * target) = 0;

	virtual void MoverTargetDown(struct IEffectTarget * target) = 0;

	virtual void MoverTargetUp(struct IEffectTarget * target) = 0;

	virtual struct IEffectParam * GetFirstParam() = 0;

	virtual struct IEffectParam * AddParam(const char * name) = 0;

	virtual void RemoveParam(struct IEffectParam * param) = 0;

	virtual struct IEffectEvent * GetStartEvent() = 0;

	virtual U32 FindTargetPosition(struct IEffectTarget * target) = 0;

	virtual void DeleteAction(struct IEffectAction * action) = 0;

	virtual void NullTarget(struct IEffectTarget * target) = 0;

	virtual void NullTarget(struct ITargetAnim * target) = 0;

	virtual void NullTarget(struct ITargetHp * target) = 0;

	virtual void CopyAction(struct IEffectAction * action) =0;

	virtual void PasteAction(struct IEffectEvent * event) = 0;

	virtual void PlaySubEffect(const char * subName, IEffectTarget * targ0, IEffectTarget * targ1, IEffectTarget * targ2, IEffectTarget * targ3) = 0;

	virtual void KillSubEffects() = 0;
};

#endif