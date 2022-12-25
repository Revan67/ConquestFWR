//TargetAnim.cpp

#include "stdafx.h"
#include "globals.h"
#include "ITargetAnim.h"
#include "ITargetCue.h"
#include "IEffectFile.h"

#include <DACOM.h>
#include <filesys.h>
#include <TSmartPointer.h>

#define TARGET_ANIM_SAVE_VERSION 1
struct TargetAnimSave
{
	U32 version;
	char name[64];
	char fileName[256];
};

struct TargetAnim: public ITargetAnim
{
	ITargetAnim * next;
	char name[64];
	char fileName[256];
	bool bScripted;
	bool bFileDependant;
	ITargetCue * firstCue;

	SINGLE playTime;

	TargetAnim();

	~TargetAnim();

	//ITargetAnim
	virtual ITargetAnim * GetNextAnim();

	virtual void SetNextAnim(ITargetAnim * target);

	virtual const char * GetName();

	virtual void SetName(const char * newName);

	virtual void SetFileName(const char * newFile);

	virtual const char * GetFileName();

	virtual bool IsScripted();

	virtual void SetScripted(bool bSetting);

	virtual struct ITargetCue * GetFirstCue();

	virtual void SetFirstCue(ITargetCue * target);

	virtual SINGLE GetPlayTime();

	virtual void SetPlayTime(SINGLE time);

	virtual void SetFileDependant(bool bSetting);

	virtual bool IsFileDependant();

	virtual void SaveTest(struct IFileSystem * outfile);

	virtual void LoadTest(struct IFileSystem * inFile);
};

TargetAnim::TargetAnim()
{
	strcpy(name,"newName");
	next = NULL;
	bScripted = false;
	fileName[0] = 0;
	firstCue = NULL;
	playTime = 0;
	bFileDependant = false;
}

TargetAnim::~TargetAnim()
{
	EFFECTFILE->NullTarget(this);
	while(firstCue)
	{
		ITargetCue * tmp = firstCue;
		firstCue = firstCue->GetNextCue();
		delete tmp;
	}
}

ITargetAnim * TargetAnim::GetNextAnim()
{
	return next;
}

void TargetAnim::SetNextAnim(ITargetAnim * target)
{
	next = target;
}

const char * TargetAnim::GetName()
{
	return name;
}

void TargetAnim::SetName(const char * newName)
{
	strncpy(name,newName,63);
	name[63] = 0;
}

void TargetAnim::SetFileName(const char * newFile)
{
	strncpy(fileName,newFile,255);
	fileName[255] = 0;
}

const char * TargetAnim::GetFileName()
{
	return fileName;
}

bool TargetAnim::IsScripted()
{
	return bScripted;
}

void TargetAnim::SetScripted(bool bSetting)
{
	bScripted = bSetting;
	if(!bScripted)//if I am not scripted get rid of all of my cue events
	{
		while(firstCue)
		{
			ITargetCue * tmp = firstCue;
			firstCue = firstCue->GetNextCue();
			delete tmp;
		}
	}
}

ITargetCue * TargetAnim::GetFirstCue()
{
	return firstCue;
}

void TargetAnim::SetFirstCue(ITargetCue * target)
{
	firstCue = target;
}

SINGLE TargetAnim::GetPlayTime()
{
	return playTime;
}

void TargetAnim::SetPlayTime(SINGLE time)
{
	playTime = time;
}

void TargetAnim::SetFileDependant(bool bSetting)
{
	bFileDependant = bSetting;
}

bool TargetAnim::IsFileDependant()
{
	return bFileDependant;
}

void TargetAnim::SaveTest(struct IFileSystem * outFile)
{
	DAFILEDESC fdesc = "TARGETANIM";
	fdesc.lpImplementation = "DOS";
	fdesc.dwDesiredAccess = GENERIC_READ|GENERIC_WRITE;
	fdesc.dwCreationDistribution = CREATE_NEW;

	COMPTR<IFileSystem> baseData;
	if (outFile->CreateInstance(&fdesc, baseData) == GR_OK)
	{
		TargetAnimSave save;
		save.version = TARGET_ANIM_SAVE_VERSION;
		strcpy(save.name,name);
		strcpy(save.fileName,fileName);
		U32 dwWritten;
		baseData->WriteFile(0,&save ,sizeof(TargetAnimSave),&dwWritten);
	}
}

void TargetAnim::LoadTest(struct IFileSystem * inFile)
{
	DAFILEDESC fdesc = "TARGETANIM";
	fdesc.dwDesiredAccess = GENERIC_READ;
	fdesc.dwShareMode = 0;  // no sharing
		
	COMPTR<IFileSystem> baseData;
	if (inFile->CreateInstance(&fdesc, baseData) == GR_OK)
	{
		TargetAnimSave save;
		U32 dwWritten;
		baseData->ReadFile(0,&save ,sizeof(TargetAnimSave),&dwWritten);
		strcpy(name,save.name);
		SetFileName(save.fileName);
	}

}

ITargetAnim * MakeTargetAnim()
{
	return new TargetAnim();
}

void DeleteTargetAnim(ITargetAnim * target)
{
	delete ((TargetAnim*)target);
}

