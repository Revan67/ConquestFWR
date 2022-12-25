#ifndef _ITARGETANIM_H_
#define _ITARGETANIM_H_
//ITargetAnim.h

struct __declspec(novtable) ITargetAnim
{
	virtual ITargetAnim * GetNextAnim() = 0;

	virtual void SetNextAnim(ITargetAnim * target) = 0;

	virtual const char * GetName() = 0;

	virtual void SetName(const char * newName) = 0;

	virtual void SetFileName(const char * newFile) = 0;

	virtual const char * GetFileName() = 0;

	virtual bool IsScripted() = 0;

	virtual void SetScripted(bool bSetting) = 0;

	virtual struct ITargetCue * GetFirstCue() = 0;

	virtual void SetFirstCue(ITargetCue * target) = 0;

	virtual SINGLE GetPlayTime() = 0;

	virtual void SetPlayTime(SINGLE playTime) = 0;

	virtual void SetFileDependant(bool bSetting) = 0;

	virtual bool IsFileDependant() = 0;

	virtual void SaveTest(struct IFileSystem * outfile) = 0;

	virtual void LoadTest(struct IFileSystem * inFile) = 0;
};

ITargetAnim * MakeTargetAnim();

void DeleteTargetAnim(ITargetAnim * target);

#endif