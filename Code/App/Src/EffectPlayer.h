#ifndef EFFECTPLAYER_H
#define EFFECTPLAYER_H
//--------------------------------------------------------------------------//
//                                                                          //
//                               EffectPlayer.H                             //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Author: Tmauer $

    $Header: /App/Src/EffectPlayer.h 8     11/07/03 9:19a Tmauer $
*/			    
//-------------------------------------------------------------------
/*
	Management of the archetype list
*/
//-------------------------------------------------------------------
//-------------------------------------------------------------------
#ifndef DACOM_H
#include <DACOM.h>
#endif

#ifndef IOBJECT_H
#include "IObject.h"
#endif

enum GlobalEffectParam
{
	GP_PARAM_NONE = 0,
};

enum GameEventFlags
{
	GEF_NONE         = 0x00000000,
	GEF_EFFECT_END   = 0x00000001,
	GEF_CUE_RESPONSE = 0x00000002,
	GEF_DISTANCE     = 0x00000004,
};

#define INVALID_EFFECT_INDEX 0xFFFFFFFF

struct IMeshInstance;

//any object that can be the target of an effect needs to implement this interface.
struct _NO_VTABLE IEffectTarget : IObject
{
	virtual void PlayAnimation(const char * animName, bool bLooping) = 0;

	virtual SINGLE QueryEffectParam(const char* targetEffectParam) = 0;

	virtual void AddToEffectListeners(struct IEffectInstance * inst) = 0;

	virtual void RemoveFromEffectListeners(struct IEffectInstance * inst) = 0;

	virtual U32 GetHardPointIndex(const char * hpName) = 0;

	virtual void EffectSystemEvent(U32 context, const char * eventString, DWORD gameEventFlags) = 0;

	virtual void PositionCallback(const char * string, Transform & trans) = 0;

	virtual void EffectSystemHide(U32 context, bool bSetting) = 0;

	virtual SINGLE GetAnimationLength(const char * animName) = 0;

	virtual SINGLE GetAnmationCueTime(const char * animName, const char * cue) = 0;

	virtual IMeshInstance * GetMesh() = 0;

	virtual void TriggerGameEvent(const char * eventName) = 0;
};
//----------------------------------------------------------------------------
//
struct _NO_VTABLE IEffectInstance
{
	virtual void SetTarget(IBaseObject * obj, U32 id, DWORD context) = 0;

	virtual void SetTargetPositon(const Vector & pos, U32 id, DWORD context) = 0;

	virtual void SetSystemID(U32 systemID) = 0;

	virtual void SetSystemID(IBaseObject * sysObj) = 0;

	virtual U32 GetSystemID() = 0;

	virtual void TriggerStartEvent() = 0;

	virtual void AnimationEvent(IBaseObject * obj, const char * eventName) = 0;

	virtual void AnimationCompleted(IBaseObject * obj) = 0;

	virtual void GameEvent(IBaseObject * obj, const char * eventName) = 0;

	virtual void Enable() = 0;

	virtual void Disable() = 0;

	virtual void Stop() = 0;

	virtual U32 GetEffectId() = 0;

	virtual BOOL32 ValidateTargets() = 0;

	virtual IEffectTarget* GetTarget(U32 id, DWORD& context) = 0;
};
//----------------------------------------------------------------------------
//
struct _NO_VTABLE  IEffectCallback
{
	virtual void EffectInstanceEnded(DWORD effectID) = 0;

	virtual void CueResponse(IBaseObject * target, const char * responseName, DWORD gameEventFlags, U32 context) = 0;

	virtual void CuePositionResponse(const Vector & position, const char * responseName, DWORD gameEventFlags, U32 context) = 0;
};
//----------------------------------------------------------------------------
//
//handle to the basic effect definition.  There is only realy one handle for all instances of the same type.
//used by the instance to handle the event chains.
struct _NO_VTABLE IEffectHandle
{
	//once created you should not keep the pointer to the IEffectInstance as it will go away when its' lifetime is up.
	//note(aaj): this should not be true anymore. All effects have handles, and should use IEffectPlayer::IsValidInstance() before operating of them
	virtual IEffectInstance * CreateInstance(U32 effectId = 0xFFFFFFFF, IEffectCallback * callback = NULL) = 0;

	virtual IEffectInstance * CreateMovieInstance() = 0;

	virtual SINGLE GetTotalPlayTime(IBaseObject ** targets, U32 numTargets) = 0;

	virtual SINGLE GetTimeToReactionCue(U32 targetID, IBaseObject ** targets, U32 numTargets) = 0;

	virtual const char * GetReactionType(U32 targetID) = 0;
};
//----------------------------------------------------------------------------
//
//The core of the effect system.  Handles loading, caching and instance management.
struct DACOM_NO_VTABLE IEffectPlayer : public IDAComponent
{
	virtual IEffectHandle * LoadEffect(const char * effectFile) = 0;

	virtual void ReleaseEffect(IEffectHandle * handle) =0;

	virtual void Update() = 0;

	virtual void Render() = 0;

	virtual void Close() = 0;

	virtual SINGLE QueryEffectParam(GlobalEffectParam param) = 0;

	virtual void SetEffectParam(GlobalEffectParam param, SINGLE value) = 0;

	virtual void ForcePlayCanceled(IBaseObject * obj) = 0;

	virtual bool IsValidInstance(IEffectInstance * test) = 0;

	virtual IEffectInstance* GetEffectInstanceById(U32 effectId) = 0;

	virtual void ClearCallbacksTo(IEffectCallback * callback) = 0;
};

/*#ifdef _DEBUG
	#define EFFECT_TRACE(exp) (FileLog::LogLine("EffectPlayer",exp),FileLog::Flush())
	#define EFFECT_TRACE1(exp,p1) (FileLog::LogLine("EffectPlayer",exp,p1),FileLog::Flush())
	#define EFFECT_TRACE2(exp,p1,p2) (FileLog::LogLine("EffectPlayer",exp,p1,p2),FileLog::Flush())
	#define EFFECT_TRACE3(exp,p1,p2,p3) (FileLog::LogLine("EffectPlayer",exp,p1,p2,p3),FileLog::Flush())
	#define EFFECT_TRACE4(exp,p1,p2,p3,p4) (FileLog::LogLine("EffectPlayer",exp,p1,p2,p3,p4),FileLog::Flush())

#else
*/
	#define EFFECT_TRACE(exp) ((void)0)
	#define EFFECT_TRACE1(exp,p1) ((void)0)
	#define EFFECT_TRACE2(exp,p1,p2) ((void)0)
	#define EFFECT_TRACE3(exp,p1,p2,p3) ((void)0)
	#define EFFECT_TRACE4(exp,p1,p2,p3,p4) ((void)0)

//#endif //_DEBUG

#endif
