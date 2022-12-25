//EffectFile.cpp

#include "stdafx.h"
#include "globals.h"

#include "IEffectFile.h"
#include "IEffectTarget.h"
#include "IEffectParam.h"
#include "IEffectEvent.h"
#include "IEffectAction.h"

#include <DACOM.h>
#include <filesys.h>
#include <TSmartPointer.h>
#include <stdio.h>

struct IEffectFile * EFFECTFILE;
extern IEffectTarget * selectedTarg;

struct SubEffect
{
	char subname[256];

	SubEffect * nextSubEffect;
	IEffectTarget * targ0;
	IEffectTarget * targ1;
	IEffectTarget * targ2;
	IEffectTarget * targ3;

	U32 contextID;

	IEffectEvent * startEvent;

	SubEffect(U32 context);

	~SubEffect();

	////////////////////////////////////
	void SetTargets(IEffectTarget *t0,IEffectTarget *t1,IEffectTarget *t2,IEffectTarget *t3);
	void Load(const char * name);
	void Start();
};

SubEffect::SubEffect(U32 context)
{
	targ0 = NULL;
	targ1 = NULL;
	targ2 = NULL;
	targ3 = NULL;
	nextSubEffect = NULL;
	contextID = context;
	startEvent = NULL;
}

SubEffect::~SubEffect()
{
	if(startEvent)
		delete startEvent;
}

void SubEffect::SetTargets(IEffectTarget *t0,IEffectTarget *t1,IEffectTarget *t2,IEffectTarget *t3)
{
	targ0 = t0;
	targ1 = t1;
	targ2 = t2;
	targ3 = t3;
}

void SubEffect::Load(const char * name)
{
	U32 dwWritten;
	strncpy(subname,name,255);
	subname[255] = 0;
	DAFILEDESC fdesc = subname;
	COMPTR<IFileSystem> inFile;

	fdesc.lpImplementation = "UTF";
	if (DACOM->CreateInstance(&fdesc, inFile) == GR_OK)
	{
		DAFILEDESC fdesc = "BASEDATA";
		fdesc.dwDesiredAccess = GENERIC_READ;
		fdesc.dwShareMode = 0;  // no sharing
		
		COMPTR<IFileSystem> baseData;
		if (inFile->CreateInstance(&fdesc, baseData) == GR_OK)
		{
			U32 version;
			baseData->ReadFile(0,&version ,sizeof(U32),&dwWritten);
		}

		if (inFile->SetCurrentDirectory("\\CORE_EFFECT") == 0)
			return;

		startEvent = MakeEffectEvent();
		startEvent->LoadCore(inFile,contextID);
	}
}

void SubEffect::Start()
{
	startEvent->TriggerEvent();
}

struct EffectFile : public IEffectFile
{
	char filename[256];

	IEffectTarget * firstTarget;
	IEffectParam * firstParam;

	IEffectEvent * startEvent;

	IEffectAction * copyAction;

	U32 lastContext;

	SubEffect * subEffects;

	EffectFile();

	~EffectFile();

	//IEffectFile

	virtual void New();

	virtual void Load(const char * fileName);

	virtual void Save(const char * fileName);

	virtual const char * GetFileName();

	virtual struct IEffectTarget * GetFirstTarget();

	virtual struct IEffectTarget * AddTarget(const char * name);

	virtual struct IEffectTarget * FindTargetByID(U32 id, U32 context);

	virtual void RemoveEffectTarget(struct IEffectTarget * target);

	virtual void MoverTargetDown(struct IEffectTarget * target);

	virtual void MoverTargetUp(struct IEffectTarget * target);

	virtual struct IEffectParam * GetFirstParam();

	virtual struct IEffectParam * AddParam(const char * name);

	virtual void RemoveParam(struct IEffectParam * param);

	virtual struct IEffectEvent * GetStartEvent();

	virtual U32 FindTargetPosition(struct IEffectTarget * target);

	virtual void DeleteAction(struct IEffectAction * action);

	virtual void NullTarget(struct IEffectTarget * target);

	virtual void NullTarget(struct ITargetAnim * target);

	virtual void NullTarget(struct ITargetHp * target);

	virtual void CopyAction(struct IEffectAction * action);

	virtual void PasteAction(struct IEffectEvent * event);

	virtual void PlaySubEffect(const char * subName, IEffectTarget * targ0, IEffectTarget * targ1, IEffectTarget * targ2, IEffectTarget * targ3);

	virtual void KillSubEffects();

	//EffectFile

	void close();

	IEffectEvent * findActionParent(IEffectEvent * current, struct IEffectAction * action);
};

EffectFile::EffectFile()
{
	firstTarget = NULL;
	firstParam = NULL;
	startEvent = NULL;
	copyAction = NULL;
	subEffects = NULL;
	lastContext = 1;
	New();
	EFFECTFILE = this;
}

EffectFile::~EffectFile()
{
	close();
	EFFECTFILE = NULL;
}

void EffectFile::New()
{
	close();
	startEvent = MakeEffectEvent();
	startEvent->SetEventType(EventSave::START_TRIGGERED);
	startEvent->SetName("StartEvent");
	copyAction = NULL;
	strcpy(filename,"Untitled");
}

void EffectFile::Load(const char * _fileName)
{
	close();
	//load code here
	U32 dwWritten;
	strncpy(filename,_fileName,255);
	filename[255] = 0;
	DAFILEDESC fdesc = filename;
	COMPTR<IFileSystem> inFile;

	fdesc.lpImplementation = "UTF";
	if (DACOM->CreateInstance(&fdesc, inFile) == GR_OK)
	{
		DAFILEDESC fdesc = "BASEDATA";
		fdesc.dwDesiredAccess = GENERIC_READ;
		fdesc.dwShareMode = 0;  // no sharing
		
		COMPTR<IFileSystem> baseData;
		if (inFile->CreateInstance(&fdesc, baseData) == GR_OK)
		{
			U32 version;
			baseData->ReadFile(0,&version ,sizeof(U32),&dwWritten);
		}

		if (inFile->SetCurrentDirectory("\\TEST_EFFECT") == 0)
			return;

		if (inFile->SetCurrentDirectory("TARGET_LIST") == 0)
			return;

		WIN32_FIND_DATA data;
		HANDLE handle;
		handle = inFile->FindFirstFile("Targ*",&data);
		if(handle != INVALID_HANDLE_VALUE)
		{
			IEffectTarget * lastTarget = NULL;
			do
			{
				if(data.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)
				{
					if (inFile->SetCurrentDirectory(data.cFileName) == 0)
						return;
					IEffectTarget * target = MakeEffectTarget();
					target->LoadTest(inFile);
					if(lastTarget)
						lastTarget->SetNextTarget(target);
					else
						firstTarget = target;
					lastTarget = target;
					if (inFile->SetCurrentDirectory("..") == 0)
						return;
				}
			}while(inFile->FindNextFile(handle,&data));
			inFile->CloseHandle(handle);
		}	

		if (inFile->SetCurrentDirectory("\\CORE_EFFECT") == 0)
			return;

		startEvent = MakeEffectEvent();
		startEvent->LoadCore(inFile,0);
		InvalidateRect(mainWindow,NULL,false);
	}
}

void EffectFile::Save(const char * _fileName)
{
	U32 dwWritten;
	strncpy(filename,_fileName,255);
	filename[255] = 0;

	char testBuff[256];
	strcpy(testBuff,filename);
	U32 filelen = strlen(testBuff);
	_strupr(testBuff);
	bool bAppend = false;
	if(filelen < 4)
		bAppend = true;
	else if(strcmp(&(testBuff[filelen-4]),".EFF") != 0)
		bAppend = true;
	if(bAppend)
	{
		if(filelen+4 > 255)
		{
			strcpy(&(filename[251]),".EFF");
		}
		else
		{
			strcat(filename,".EFF");
		}
	}
	//save code here

	COMPTR<IFileSystem> outFile;
	DAFILEDESC fdesc = filename;
	fdesc.lpImplementation = "UTF";
	fdesc.dwDesiredAccess = GENERIC_READ|GENERIC_WRITE;
//	fdesc.dwShareMode = 0;		// no sharing, faster reads/ writes
	fdesc.dwCreationDistribution = CREATE_ALWAYS;
	if (DACOM->CreateInstance(&fdesc, outFile) != GR_OK)
	{
//		CQERROR0("Failed to create save file.");
		MessageBox(NULL,"Failed to create save file.","File Error",MB_OK);
		return ;
	}

	DAFILEDESC fdesc2 = "BASEDATA";
	fdesc2.lpImplementation = "DOS";
	fdesc2.dwDesiredAccess = GENERIC_READ|GENERIC_WRITE;
//	fdesc2.dwShareMode = 0;  // no sharing
	fdesc2.dwCreationDistribution = CREATE_NEW;
	
	COMPTR<IFileSystem> baseData;
	if (outFile->CreateInstance(&fdesc2, baseData) == GR_OK)
	{
		U32 version = EFFECT_SAVE_VERSION;
		baseData->WriteFile(0,&version ,sizeof(U32),&dwWritten);
	}

	//save off the compiled effect
	outFile->SetCurrentDirectory("\\");
	outFile->CreateDirectory("\\CORE_EFFECT");
	
	if (outFile->SetCurrentDirectory("\\CORE_EFFECT") == 0)
		return;

	if(startEvent)
		startEvent->SaveCore(outFile);

	//save out all of the test information
	outFile->SetCurrentDirectory("\\");
	outFile->CreateDirectory("\\TEST_EFFECT");
	
	if (outFile->SetCurrentDirectory("\\TEST_EFFECT") == 0)
		return;

	//save out test target list
	outFile->CreateDirectory("TARGET_LIST");
	if (outFile->SetCurrentDirectory("TARGET_LIST") == 0)
		return;
	IEffectTarget * targ = GetFirstTarget();
	U32 i = 0;
	while(targ)
	{
		char dirName[256];
		sprintf(dirName,"Targ%d",i);
		outFile->CreateDirectory(dirName);
		if (outFile->SetCurrentDirectory(dirName) == 0)
			return;
		targ->SaveTest(outFile);
		if (outFile->SetCurrentDirectory("..") == 0)
			return;
		targ = targ->GetNextTarget();
		++i;
	}
}

const char * EffectFile::GetFileName()
{
	return filename;
}

IEffectTarget * EffectFile::GetFirstTarget()
{
	return firstTarget;
}

IEffectTarget * EffectFile::AddTarget(const char * name)
{
	IEffectTarget * target = MakeEffectTarget();
	target->SetName(name);
	target->SetNextTarget(NULL);
	
	IEffectTarget * search = firstTarget;
	while(search && search->GetNextTarget())
	{
		search = search->GetNextTarget();
	}
	if(search)
	{
		search->SetNextTarget(target);
	}
	else
	{
		firstTarget = target;
	}
	return target;
}

struct IEffectTarget * EffectFile::FindTargetByID(U32 id, U32 context)
{
	if(context == 0)
	{
		IEffectTarget * search = firstTarget;
		while(search)
		{
			if(search->GetTargetID() == id)
				return search;
			search = search->GetNextTarget();
		}
	}
	else
	{
		SubEffect * search = subEffects;
		while(search)
		{
			if(search->contextID == context)
			{
				switch(id)
				{
				case 0:
					return search->targ0;
				case 1:
					return search->targ1;
				case 2:
					return search->targ2;
				case 3:
					return search->targ3;
				}
				return NULL;
			}
			search = search->nextSubEffect;
		}
	}
	return NULL;
}

void EffectFile::RemoveEffectTarget(struct IEffectTarget * target)
{
	IEffectTarget * search = firstTarget;
	IEffectTarget * prev = NULL;
	while(search)
	{
		if(search == target)
		{
			if(prev)
				prev->SetNextTarget(search->GetNextTarget());
			else
				firstTarget = search->GetNextTarget();
			DeleteEffectTarget(target);
			return;
		}
		prev = search;
		search = search->GetNextTarget();
	}
}

void EffectFile::MoverTargetDown(struct IEffectTarget * target)
{
	IEffectTarget * search = firstTarget;
	IEffectTarget * prev = NULL;
	while(search)
	{
		if(search == target)
		{
			IEffectTarget * next = search->GetNextTarget();
			if(next)//otherwise we are at the bottom
			{
				if(prev)
					prev->SetNextTarget(next);
				else
					firstTarget = next;
				search->SetNextTarget(next->GetNextTarget());
				next->SetNextTarget(search);
			}
			return;
		}
		prev = search;
		search = search->GetNextTarget();
	}
}

void EffectFile::MoverTargetUp(struct IEffectTarget * target)
{
	IEffectTarget * search = firstTarget;
	IEffectTarget * prev = NULL;
	IEffectTarget * prevPrev = NULL;
	while(search)
	{
		if(search == target)
		{
			if(prev)//otherwise we are at the top
			{
				prev->SetNextTarget(search->GetNextTarget());
				search->SetNextTarget(prev);
				if(prevPrev)
					prevPrev->SetNextTarget(search);
				else
					firstTarget = search;
			}
			return;
		}
		prevPrev = prev;
		prev = search;
		search = search->GetNextTarget();
	}
}

IEffectParam * EffectFile::GetFirstParam()
{
	return firstParam;
}

IEffectParam * EffectFile::AddParam(const char * name)
{
	IEffectParam * param = MakeEffectParam();
	param->SetName(name);
	param->SetNextParam(NULL);
	
	IEffectParam * search = firstParam;
	while(search && search->GetNextParam())
	{
		search = search->GetNextParam();
	}
	if(search)
	{
		search->SetNextParam(param);
	}
	else
	{
		firstParam = param;
	}
	return param;
}

void EffectFile::RemoveParam(struct IEffectParam * param)
{
	IEffectParam * search = firstParam;
	IEffectParam * prev = NULL;
	while(search)
	{
		if(search == param)
		{
			if(prev)
				prev->SetNextParam(search->GetNextParam());
			else
				firstParam = search->GetNextParam();
			delete param;
			return;
		}
		prev = search;
		search = search->GetNextParam();
	}
}

IEffectEvent * EffectFile::GetStartEvent()
{
	return startEvent;
}

U32 EffectFile::FindTargetPosition(struct IEffectTarget * target)
{
	U32 pos = 0;
	IEffectTarget * search = firstTarget;
	while(search)
	{
		if(search == target)
			return pos;
		++pos;
		search = search->GetNextTarget();
	}
	return 0;
}

void EffectFile::DeleteAction(struct IEffectAction * action)
{
	//to delete an action we need to find its' parent to null the pointer
	IEffectEvent * event = findActionParent(startEvent,action);
	if(event)
		event->RemoveAction(action);

	//now delete the action.
	action->Delete();//self destruct function
}

void EffectFile::NullTarget(struct IEffectTarget * target)
{
	if(startEvent)
		startEvent->NullTarget(target);
	if(copyAction)
		copyAction->NullTarget(target);
}

void EffectFile::NullTarget(struct ITargetAnim * target)
{
	if(startEvent)
		startEvent->NullTarget(target);
	if(copyAction)
		copyAction->NullTarget(target);
}

void EffectFile::NullTarget(struct ITargetHp * target)
{
	if(startEvent)
		startEvent->NullTarget(target);
	if(copyAction)
		copyAction->NullTarget(target);
}

void EffectFile::CopyAction(struct IEffectAction * action)
{
	if(copyAction)
		delete copyAction;
	copyAction = action->CreateCopy();
}

void EffectFile::PasteAction(struct IEffectEvent * event)
{
	if(copyAction)
	{
		IEffectAction * newAction = copyAction->CreateCopy();
		newAction->SetNextAction(event->GetFirstAction());
		event->SetFirstAction(newAction);
	}
}

void EffectFile::PlaySubEffect(const char * subName, IEffectTarget * targ0, IEffectTarget * targ1, IEffectTarget * targ2, IEffectTarget * targ3)
{
	SubEffect * sub = new SubEffect(lastContext);
	++lastContext;
	sub->SetTargets(targ0,targ1,targ2,targ3);
	sub->Load(subName);
	sub->nextSubEffect = subEffects;
	subEffects = sub;
	sub->Start();
}

void EffectFile::KillSubEffects()
{
	while(subEffects)
	{
		SubEffect * tmp = subEffects;
		subEffects = subEffects->nextSubEffect;
		delete tmp;
	}
}

void EffectFile::close()
{
	selectedTarg = 0;
	delete startEvent;//should delete the whole tree because start event has no siblings;
	startEvent = NULL;
	delete copyAction;
	copyAction = NULL;
	while(firstTarget)
	{
		IEffectTarget * tmp = firstTarget;
		firstTarget = firstTarget->GetNextTarget();
		delete tmp;
	}
	while(firstParam)
	{
		IEffectParam * tmp = firstParam;
		firstParam = firstParam->GetNextParam();
		delete tmp;
	}
	while(subEffects)
	{
		SubEffect * tmp = subEffects;
		subEffects = subEffects->nextSubEffect;
		delete tmp;
	}
}

IEffectEvent * EffectFile::findActionParent(IEffectEvent * current, struct IEffectAction * action)
{
	if(current)
	{
		IEffectAction * aSearch = current->GetFirstAction();
		while(aSearch)
		{
			if(aSearch == action)
				return current;
			IEffectEvent * nextEvent = aSearch->GetFirstEvent();
			while(nextEvent)
			{
				IEffectEvent * retVal = findActionParent(nextEvent,action);
				if(retVal)
					return retVal;
				nextEvent = nextEvent->GetNextEvent();
			}
			aSearch = aSearch->GetNextAction();
		}
	}
	return NULL;
}

EffectFile effectFile;