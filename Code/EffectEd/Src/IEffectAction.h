#ifndef _IEFFECTACTION_H_
#define _IEFFECTACTION_H_
//IEffectEvent.h

struct __declspec(novtable) IParticleFilter
{
	virtual IParticleFilter * GetNextFilter() = 0;

	virtual void SetNextFilter(IParticleFilter * filter) = 0;

	virtual enum ParticleEffectType GetType() = 0;

	virtual HWND InitWindow() = 0;

	virtual HWND GetWindow() = 0;

	virtual void SetWindow(HWND win) = 0;

	virtual void OpenWindow() = 0;

	virtual void SetWindowPos(S32 newX,S32 newY) = 0;

	virtual S32 GetWinX() = 0;

	virtual S32 GetWinY() = 0;

	virtual void DestroyOutput(U32 id) = 0;

	virtual void DestroyInput(U32 id) = 0;

	virtual void ConnectOutput(IParticleFilter * filter, U32 id, U32 fromID) = 0;
	
	virtual void ConnectInput(IParticleFilter * filter, U32 id, U32 fromID) = 0;

	virtual bool GetOutputPoint(POINT & point,U32 id) = 0;

	virtual bool GetInputInfo(U32 inputID, U32 &sourceOutputID, IParticleFilter *& source) = 0;

	virtual SINGLE GetDataValue(U32 id) = 0;

	virtual void SetDataValue(U32 id, SINGLE value) = 0;

	virtual struct IEffectTarget * GetDataTarget(U32 id) = 0;

	virtual void SetDataTarget(U32 id, struct IEffectTarget * targ) = 0;

	virtual struct ITargetHp * GetDataHardPoint(U32 id) = 0;

	virtual void SetDataHardPoint(U32 id, ITargetHp * targ) = 0;

	virtual char * GetDataString(U32 id) = 0;

	virtual void SetDataString(U32 id, const char * string) = 0;

	virtual bool IsRoot() = 0;

	virtual void AddToInstance(struct IParticleInstance * inst, struct IParticleEffectInstance* parent, U32 inID) =0;

	virtual IParticleEffectInstance * GetWorkingEffect() = 0;

	virtual void ResetPlayback() = 0;

	virtual void SaveCore(IFileSystem * outFile) = 0;

	virtual void ReLink(IParticleFilter * instList) = 0;

	virtual U32 LinkID() = 0;

	virtual void SetLinkID(U32 newLinkID) = 0;

	virtual void SetFilterName(const char * name) = 0;

	virtual const char * GetFilterName() = 0;

	virtual struct IParticleProgramer * GetProgramer() = 0;

	virtual IParticleFilter * CreateCopy() = 0;
};

struct __declspec(novtable) IActionParticle
{
	virtual IParticleFilter * CreateNewFilter(enum ParticleEffectType type) = 0;

	virtual void DestroyFilter(IParticleFilter * filter) = 0;

	virtual IParticleFilter * FindFilterFromHWND(HWND hWindow) = 0;

	virtual IParticleFilter * GetFirstFilter() = 0;

	virtual void SetFirstFilter(IParticleFilter *  filter) = 0;

	virtual void ActivateFilterWindows() = 0;

	virtual void InsertFilter(IParticleFilter * filter) = 0;

	virtual void AddFilterEvent(const char * eventName) = 0;

	virtual void RemoveFilterEvent(const char * eventName) = 0;

	virtual void RenameFilterEvent(const char * oldName, const char * eventName) = 0;

	virtual void ImportLoadCore(struct IFileSystem * inFile) = 0;

	virtual bool IsDisabled() = 0;

	virtual void SetDisabled(bool bSetting) = 0;
};

struct __declspec(novtable) IActionAnimation
{
	virtual void SetAnimation(struct ITargetAnim * anim) = 0;

	virtual struct ITargetAnim * GetAnimation() = 0;

	virtual void SetTarget(struct IEffectTarget * target) = 0;

	virtual struct IEffectTarget * GetTarget() = 0;

	virtual void SetLooping(bool bSetting) = 0;

	virtual bool IsLooping() = 0;

	virtual void SetForced(bool bSetting) = 0;

	virtual bool IsForced() = 0;

	virtual bool IsNamedAnim() = 0;

	virtual void SetNamedAnim(const char * newName) = 0;

	virtual const char * GetAnimName() = 0;
};

struct __declspec(novtable) IActionGameEvent
{
	virtual void SetGameEventName(const char * eventName) = 0;

	virtual const char * GetGameEventName() = 0;

	virtual void SetTarget(struct IEffectTarget * target) = 0;

	virtual struct IEffectTarget * GetTarget() = 0;

	virtual void SetResponceFlag(bool bSetting) = 0;

	virtual bool GetResponceFlag() = 0;

	virtual void SetDistanceFlag(bool bSetting) = 0;

	virtual bool GetDistanceFlag() = 0;

	virtual void SetEndEffectFlag(bool bSetting) = 0;

	virtual bool GetEndEffectFlag() = 0;

	virtual void SetSpeed(SINGLE newSpeed) = 0;

	virtual SINGLE GetSpeed() = 0;
};

struct __declspec(novtable) IActionSound
{
	virtual void SetSoundEntry(const char * soundName) = 0;

	virtual const char * GetSoundEntry() = 0;

	virtual void SetTarget(struct IEffectTarget * target) = 0;

	virtual struct IEffectTarget * GetTarget() = 0;

	virtual SINGLE GetMinSound() = 0;

	virtual void SetMinSound(SINGLE setting) = 0;

	virtual SINGLE GetMaxSound() = 0;

	virtual void SetMaxSound(SINGLE setting) = 0;

	virtual bool IsLooping() = 0;

	virtual void SetLooping(bool bSetting) = 0;

	virtual void SetHpName(const char * setting) = 0;

	virtual const char * GetHpName() = 0;
};

struct __declspec(novtable) IActionSwitch
{
	virtual U32 GetSwitchNumber() = 0;

	virtual void SetSwitchNumber(U32 setting) = 0;
};

struct __declspec(novtable) IActionHideTarget
{
	virtual bool GetHide() = 0;

	virtual void SetHide(bool bSetting) = 0;

	virtual void SetTarget(struct IEffectTarget * target) = 0;

	virtual struct IEffectTarget * GetTarget() = 0;
};

struct __declspec(novtable) IActionListen
{
	virtual void SetListenName(const char * newListenName) = 0;

	virtual char * GetListenName() = 0;

	virtual void SetTarget(struct IEffectTarget * target) = 0;

	virtual struct IEffectTarget * GetTarget() = 0;
};

struct __declspec(novtable) IActionJointTrack
{
	virtual void SetJointName(const char * newJointName) = 0;

	virtual char * GetJointName() = 0;

	virtual void SetTarget(struct IEffectTarget * target) = 0;

	virtual struct IEffectTarget * GetTarget() = 0;

	virtual void SetSource(struct IEffectTarget * source) = 0;

	virtual struct IEffectTarget * GetSource() = 0;

	virtual void SetAngVelocity(SINGLE newVel) = 0;

	virtual SINGLE GetAngVelocity() = 0;
};


struct __declspec(novtable) IEffectAction
{
	virtual void Delete() = 0;

	virtual IEffectAction * GetNextAction() = 0;

	virtual void SetNextAction(IEffectAction * target) = 0;

	virtual struct IEffectEvent * GetFirstEvent() = 0;

	virtual void SetFirstEvent(struct IEffectEvent * target) = 0;

	virtual const char * GetName() = 0;

	virtual void SetName(const char * newName) = 0;

	virtual IActionAnimation * GetActionAnimation() = 0;

	virtual IActionParticle * GetActionParticle() = 0;

	virtual IActionGameEvent * GetActionGameEvent() = 0;

	virtual IActionSound * GetActionSound() = 0;

	virtual IActionSwitch * GetActionSwitch() = 0;

	virtual IActionHideTarget * GetActionHideTarget() = 0;

	virtual IActionListen * GetActionListen() = 0;

	virtual IActionJointTrack * GetActionJointTrack() = 0;

	virtual void StartAction() = 0;

	virtual void TriggerEvent(const char * event) = 0;

	virtual void AnimationFinished() = 0;

	virtual S32 GetDrawWidth() = 0;

	virtual bool IsOpen() = 0;

	virtual void SetOpen(bool setting) = 0;

	virtual void SaveCore(IFileSystem * outFile) = 0;

	virtual void LoadCore(struct IFileSystem * inFile, U32 _context, U32 version) = 0;

	virtual void NullTarget(struct IEffectTarget * target) = 0;

	virtual void NullTarget(struct ITargetAnim * target) = 0;

	virtual void NullTarget(struct ITargetHp * target) = 0;

	virtual IEffectAction * CreateCopy() = 0;

	virtual S32 GetIconXPos() = 0;

	virtual S32 GetIconYPos() = 0;

	virtual IEffectEvent * GetParent() = 0;

	virtual void SetParent(IEffectEvent* event) = 0;

	virtual void SetDimentions(S32 newWidth, S32 newHeight) = 0;

	virtual bool HitTest(S32 x, S32 y) = 0; 

	virtual void RealitveIconMove(S32 x, S32 y) = 0;

	virtual void SetIconPos(S32 x, S32 y) = 0;

	virtual void UpdateAction() = 0;
};

IEffectAction * MakeEffectAction();
IEffectAction * MakeEffectActionSound();
IEffectAction * MakeEffectActionAnim();
IEffectAction * MakeEffectActionParticle();
IEffectAction * MakeEffectActionGameEvent();
IEffectAction * MakeEffectActionSwitch();
IEffectAction * MakeEffectActionHideTarget();
IEffectAction * MakeEffectActionListen();
IEffectAction * MakeEffectActionJointTrack();

IEffectAction * LoadActionCore(struct IFileSystem * inFile,U32 _context);


///////////////////////////////////////////////////////////////////////////
//Base Action Class
struct EffectAction: public IEffectAction
{
	IEffectAction * next;
	char name[64];

	IEffectEvent * parent;

	S32 iconXPos;
	S32 iconYPos;
	S32 iconWidth;
	S32 iconHeight;

	IEffectEvent * firstEvent;

	EffectAction();

	~EffectAction();

	//IEffectAction
	virtual void Delete();

	virtual IEffectAction * GetNextAction();

	virtual void SetNextAction(IEffectAction * target);

	virtual struct IEffectEvent * GetFirstEvent();

	virtual void SetFirstEvent(struct IEffectEvent * target);

	virtual const char * GetName();

	virtual void SetName(const char * newName);

	virtual IActionAnimation * GetActionAnimation();

	virtual IActionParticle * GetActionParticle();

	virtual IActionGameEvent * GetActionGameEvent();

	virtual IActionSound * GetActionSound();

	virtual IActionSwitch * GetActionSwitch();

	virtual IActionHideTarget * GetActionHideTarget();

	virtual IActionListen * GetActionListen();

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

	virtual S32 GetIconXPos();

	virtual S32 GetIconYPos();

	virtual IEffectEvent * GetParent();

	virtual void SetParent(IEffectEvent* event);

	virtual void SetDimentions(S32 newWidth, S32 newHeight);

	virtual bool HitTest(S32 x, S32 y); 

	virtual void RealitveIconMove(S32 x, S32 y);

	virtual void SetIconPos(S32 x, S32 y);

	virtual void UpdateAction();
};

#endif