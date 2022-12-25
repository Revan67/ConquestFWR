#ifndef _IEFFECTTARGET_H_
#define _IEFFECTTARGET_H_
//IEffectTarget.h

struct IMeshInstance;

struct __declspec(novtable) IEffectTarget
{
	virtual IEffectTarget * GetNextTarget() = 0;

	virtual void SetNextTarget(IEffectTarget * target) = 0;

	virtual const char * GetName() = 0;

	virtual void SetName(const char * newName) = 0;

	virtual struct ITargetHp * GetFirstHardpoint() = 0;

	virtual struct ITargetHp * AddHardpoint(const char * newName) = 0;

	virtual void RemoveTargetHp(struct ITargetHp * target) = 0;

	virtual struct ITargetAnim * GetFirstAnim() = 0;

	virtual struct ITargetAnim * AddAnim(const char * newName) = 0;

	virtual void RemoveAnim(struct ITargetAnim * target) = 0;

	virtual void Update() = 0;

	virtual void Render() = 0;

	virtual void LoadMesh(const char * meshName) = 0;

	virtual char * GetMeshName() = 0;

	virtual IMeshInstance * GetMesh() = 0;

	virtual bool LoadAnimFile(const char * filename,const char * animName) = 0;

	virtual SINGLE GetAnimPlaytime(const char * animName) = 0;

	virtual void UnloadAnimation(const char * animName) = 0;

	virtual void Deselect() = 0;
	
	virtual void Select() = 0;

	virtual bool HitTest(class Vector origin, class Vector dir, SINGLE & dist) = 0;

	virtual class TRANSFORM GetTransform() = 0;

	virtual void SetTransform(const class TRANSFORM & trans) = 0;

	virtual class Vector GetPosition() = 0;

	virtual void SetPosition(class Vector pos) = 0;

	virtual void PlayAnimation(ITargetAnim * anim, bool bLooping) = 0;

	virtual void PlayNamedAnimation(const char * animName, bool bLooping) = 0;

	virtual void StopAnimation() = 0;

	virtual void SetCurrentAction(struct IEffectAction * action) =0;

	virtual U32 GetEffectID() = 0;

	virtual U32 GetHardPointIndex(struct ITargetHp * hp) = 0;

	virtual U32 GetHardPointIndex(const char * hpName) = 0;

	virtual void GetHardPointTransform(U32 index, class Transform & hpTrans) = 0;

	virtual U32 GetTargetID() = 0;

	virtual void SaveTest(struct IFileSystem * outfile) = 0;

	virtual void LoadTest(struct IFileSystem * inFile) = 0;

//	virtual struct IGrannyInstance * GetMesh() = 0;

	virtual void InitMovieLights() = 0;

	virtual void DeleteMovieLights() = 0;

	virtual void Hide(bool bSetting) = 0;

	virtual void AddToUpdateList(struct IEffectAction * action) = 0;

	virtual void ClearUpdateList() = 0;
};

IEffectTarget * MakeEffectTarget();

void DeleteEffectTarget(IEffectTarget *  target);

#endif