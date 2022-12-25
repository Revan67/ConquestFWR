//EffectActionParticle.cpp

#include "stdafx.h"
#include "globals.h"
#include "IEffectEvent.h"
#include "IEffectAction.h"
#include "resource.h"
#include "ParticleView.h"
#include "IEffectFile.h"
#include "IEffectTarget.h"
#include "ITargetHp.h"
#include "PreviewWin.h"
#include "MathLayoutWin.h"

#include <DACOM.h>
#include <filesys.h>
#include <TSmartPointer.h>
#include <IParticleManager.h>
#include <IMaterialManager.h>

#include <stdio.h>
#include <stdlib.h>
#include <commctrl.h>
#include <commdlg.h>

BOOL CALLBACK filterProc(HWND hWindow, UINT message, WPARAM wParam, LPARAM lParam);

extern HWND particleWorkArea; //the work area

IParticleFilter * initFilter;//used to initialize the dialogs
extern IEffectAction * selectedAction;

extern S32 partOffX;
extern S32 partOffY;

FloatType * MakeDefaultFloat(SINGLE value)
{
	FloatType * retVal = new FloatType;
	retVal->type = FloatType::CONSTANT;
	retVal->constant = value;
	return retVal;
}

TransformType * MakeDefaultTrans()
{
	TransformType * retVal = new TransformType;
	retVal->type = TransformType::UP;
	return retVal;
}

struct BaseFilter : public IParticleFilter
{
	HWND filterWin;
	char filterName[32];

	S32 xPos;
	S32 yPos;

	IParticleFilter * next;

	IParticleFilter ** input;
	U32 *inputTargetID; //id corresponding to the output number of the target

	IParticleFilter ** output;//this is an array
	U32 *outputTargetID; //id corresponding to the input number of the target

	ParticleEffectType filterType;
	IParticleProgramer * programer;

	IParticleEffectInstance * workingEffect;
	U8 * data;

	U32 context;

	U32 linkID;

	BaseFilter(ParticleEffectType type, U32 linkID);

	BaseFilter(IFileSystem * inFile, IActionParticle * master, U32 _context);

	~BaseFilter();

	//IParticleFilter

	virtual IParticleFilter * GetNextFilter();

	virtual void SetNextFilter(IParticleFilter * filter);

	virtual ParticleEffectType GetType();

	virtual HWND InitWindow();

	virtual HWND GetWindow();

	virtual void OpenWindow();

	virtual void SetWindow(HWND win);

	virtual void SetWindowPos(S32 newX,S32 newY);

	virtual S32 GetWinX();

	virtual S32 GetWinY();

	virtual void DestroyOutput(U32 id);

	virtual void DestroyInput(U32 id);

	virtual void ConnectOutput(IParticleFilter * filter, U32 id, U32 fromID);

	virtual void ConnectInput(IParticleFilter * filter, U32 id, U32 fromID);

	virtual bool GetOutputPoint(POINT & point,U32 id);

	virtual bool GetInputInfo(U32 inputID, U32 &sourceOutputID, IParticleFilter *& source);

	virtual SINGLE GetDataValue(U32 id);

	virtual void SetDataValue(U32 id, SINGLE value);

	virtual IEffectTarget * GetDataTarget(U32 id);

	virtual void SetDataTarget(U32 id, IEffectTarget * targ);

	virtual ITargetHp * GetDataHardPoint(U32 id);

	virtual void SetDataHardPoint(U32 id, ITargetHp * targ);

	virtual char * GetDataString(U32 id);

	virtual void SetDataString(U32 id, const char * string);

	virtual bool IsRoot();

	virtual void AddToInstance(struct IParticleInstance * inst, IParticleEffectInstance* parent, U32 inID);

	virtual IParticleEffectInstance * GetWorkingEffect();

	virtual void ResetPlayback();

	virtual void SaveCore(IFileSystem * outFile);

	virtual void ReLink(IParticleFilter * instList);

	virtual U32 LinkID();

	virtual void SetLinkID(U32 newLinkID);

	virtual void SetFilterName(const char * name);

	virtual const char * GetFilterName();

	virtual struct IParticleProgramer * GetProgramer();

	virtual IParticleFilter * CreateCopy();
};

BaseFilter::BaseFilter(ParticleEffectType type,U32 _linkID)
{
	data = NULL;
	linkID = _linkID;
	filterWin = 0;
	filterType = type;
	programer = PARTMAN->CreateParticleProgramer(type);

	U32 numOutput = programer->GetNumOutput();
	output = new IParticleFilter*[numOutput];
	memset(output,0,sizeof(IParticleFilter*)*numOutput);
	outputTargetID = new U32[numOutput];
	memset(outputTargetID,0,sizeof(U32)*numOutput);

	U32 numInput = programer->GetNumInput();
	input = new IParticleFilter*[numInput];
	memset(input,0,sizeof(IParticleFilter*)*numInput);
	inputTargetID = new U32[numInput];
	memset(inputTargetID,0,sizeof(U32)*numInput);

	xPos = partOffX;//always puts them in the upper corner
	yPos = partOffY;
	workingEffect = 0;
	filterName[0] = 0;
	context = 0;
};

BaseFilter::BaseFilter(IFileSystem * inFile, IActionParticle * master, U32 _context)
{
	data = NULL;
	filterWin = 0;
	context = _context;
	next = NULL;
	U32 dwWritten;
	DAFILEDESC fdesc = "FILTER";
	fdesc.dwDesiredAccess = GENERIC_READ;
	fdesc.dwShareMode = 0;  // no sharing
	
	COMPTR<IFileSystem> actionData;
	if (inFile->CreateInstance(&fdesc, actionData) == GR_OK)
	{
		FilterSaveHeader fSave;
		actionData->ReadFile(0,&(fSave) ,sizeof(FilterSaveHeader),&dwWritten);
		filterType = fSave.type;
		programer = PARTMAN->CreateParticleProgramer(filterType);

		U32 numOutput = programer->GetNumOutput();
		output = new IParticleFilter*[numOutput];
		memset(output,0,sizeof(IParticleFilter*)*numOutput);
		outputTargetID = new U32[numOutput];
		memset(outputTargetID,0,sizeof(U32)*numOutput);

		U32 numInput = programer->GetNumInput();
		input = new IParticleFilter*[numInput];
		memset(input,0,sizeof(IParticleFilter*)*numInput);
		inputTargetID = new U32[numInput];
		memset(inputTargetID,0,sizeof(U32)*numInput);

		xPos = fSave.xPos;
		yPos = fSave.yPos;
		linkID = fSave.linkID;
		workingEffect = 0;
		filterName[0] = 0;

		for(U32 i = 0 ; i < numOutput; ++i)
		{
			actionData->ReadFile(0,(void*)(&(output[i])) ,sizeof(U32),&dwWritten);//very hacky
		}

		for(U32 i = 0 ; i < numOutput; ++i)
		{
			actionData->ReadFile(0,(&(outputTargetID[i])) ,sizeof(U32),&dwWritten);
		}

		for( U32 i = 0 ; i < numInput; ++i)
		{
			actionData->ReadFile(0,(void*)(&(input[i])) ,sizeof(U32),&dwWritten);//very hacky
		}

		for(U32 i = 0 ; i < numInput; ++i)
		{
			actionData->ReadFile(0,(&(inputTargetID[i])) ,sizeof(U32),&dwWritten);
		}

		U8 * data = new U8[fSave.dataSize];
		actionData->ReadFile(0,data ,fSave.dataSize,&dwWritten);
		programer->SetDataChunk(data);
		strcpy(filterName,programer->GetEffectName());
		delete data;
	}
	master->InsertFilter(this);
}

BaseFilter::~BaseFilter()
{
	if(data)
		delete [] data;
	delete [] output;
	PARTMAN->ReleaseInstance(programer);
};

IParticleFilter * BaseFilter::GetNextFilter()
{
	return next;
}

void BaseFilter::SetNextFilter(IParticleFilter * filter)
{
	next = filter;
}

ParticleEffectType BaseFilter::GetType()
{
	return filterType;
}

HWND BaseFilter::InitWindow()
{
	initFilter = this;
	filterWin = CreateDialog(hMainInst,MAKEINTRESOURCE(IDD_GENERAL_FILTER_DIALOG), particleWorkArea, filterProc);
	initFilter = NULL;
	RECT rect;
	GetWindowRect(filterWin,&rect);
	MoveWindow(filterWin,xPos-partOffX,yPos-partOffY,rect.right-rect.left,rect.bottom-rect.top,true);
	ShowWindow(filterWin,true);
	return filterWin;	
}

HWND BaseFilter::GetWindow()
{
	return filterWin;
};

void BaseFilter::OpenWindow()
{
	if(!filterWin)
	{
		initFilter = this;
		filterWin = CreateDialog(hMainInst,MAKEINTRESOURCE(IDD_GENERAL_FILTER_DIALOG), mainWindow, filterProc);
		initFilter = NULL;
		ShowWindow(filterWin,true);
	}
}

void BaseFilter::SetWindow(HWND win)
{
	filterWin = win;
}

void BaseFilter::SetWindowPos(S32 newX,S32 newY)
{
	xPos = newX;
	yPos = newY;
};

S32 BaseFilter::GetWinX()
{
	return xPos;
}

S32 BaseFilter::GetWinY()
{
	return yPos;
}

void BaseFilter::DestroyOutput(U32 id)
{
	if(id == -1)
	{
		for(U32 i = 0; i < programer->GetNumOutput(); ++i)
		{
			if(output[i])
			{
				IParticleFilter * tmp = output[i];
				output[i] = NULL;
				tmp->DestroyInput(outputTargetID[i]);
				outputTargetID[i] = 0;
			}
		}
	}
	else if(output[id])
	{
		IParticleFilter * tmp = output[id];
		output[id] = NULL;
		tmp->DestroyInput(outputTargetID[id]);
	}
};

void BaseFilter::DestroyInput(U32 id)
{
	if(id == -1)
	{
		for(U32 i = 0; i < programer->GetNumInput(); ++i)
		{
			if(input[i])
			{
				IParticleFilter * tmp = input[i];
				input[i] = NULL;
				tmp->DestroyOutput(inputTargetID[i]);
				inputTargetID[i] = 0;
			}
		}
	}
	else if(input[id])
	{
		IParticleFilter * tmp = input[id];
		input[id] = NULL;
		tmp->DestroyOutput(inputTargetID[id]);
		inputTargetID[id] = 0;
	}
};

void BaseFilter::ConnectOutput(IParticleFilter * filter, U32 id, U32 fromID)
{
	output[id] = filter;
	outputTargetID[id] = fromID;
};

void BaseFilter::ConnectInput(IParticleFilter * filter, U32 id, U32 fromID)
{
	if(input[id])
	{
		IParticleFilter * tmp = input[id];
		input[id] = NULL;
		tmp->DestroyOutput(inputTargetID[id]);		
		inputTargetID[id] = 0;
	}
	input[id] = filter;
	inputTargetID[id] = fromID;
};

bool BaseFilter::GetOutputPoint(POINT & point,U32 id)
{
	if(output[id])
	{
		RECT rect;
		GetWindowRect(filterWin,&rect);
		point.x = (rect.right);
		point.y = (rect.top+44+(id*15));
		return true;
	}
	return false;
};

bool BaseFilter::GetInputInfo(U32 inID, U32 &sourceOutputID, IParticleFilter *& source)
{
	if(input[inID])
	{
		source = input[inID];
		sourceOutputID = inputTargetID[inID];
		return true;
	}
	return false;
}

SINGLE BaseFilter::GetDataValue(U32 id)
{
	return 0;
};

void BaseFilter::SetDataValue(U32 id, SINGLE value)
{
};

IEffectTarget * BaseFilter::GetDataTarget(U32 id)
{
	return NULL;
};

void BaseFilter::SetDataTarget(U32 id, IEffectTarget * targ)
{
};

ITargetHp * BaseFilter::GetDataHardPoint(U32 id)
{
	return NULL;
};

void BaseFilter::SetDataHardPoint(U32 id, ITargetHp * targ)
{
};

char * BaseFilter::GetDataString(U32 id)
{
	return NULL;
};

void BaseFilter::SetDataString(U32 id, const char * string)
{
};

bool BaseFilter::IsRoot()
{
	for(U32 i = 0; i < programer->GetNumInput(); ++i)
	{
		if(input[i])
			return false;
	}
	return true;
};

void BaseFilter::AddToInstance(struct IParticleInstance * inst, IParticleEffectInstance* parent, U32 inID)
{
	if(workingEffect)
	{
		if(parent)
		{
			parent->LinkFilter(inputTargetID[inID],inID,workingEffect);
		}
	}
	else
	{
		U32 dataSize = programer->GetDataChunkSize();
		if(data)
			delete [] data;
		data = new U8[dataSize];
		programer->GetDataChunk(data);
		if(parent)
			workingEffect = parent->AddFilter(inputTargetID[inID],inID,filterType,data);
		else
			workingEffect = inst->AddFilter(filterType,data);
		U32 numOutput = programer->GetNumOutput();
		for(U32 i = 0; i < numOutput; ++i)
		{
			if(output[i])
				output[i]->AddToInstance(inst,workingEffect,outputTargetID[i]);
		}
	}
};

IParticleEffectInstance * BaseFilter::GetWorkingEffect()
{
	return workingEffect;
};

void BaseFilter::ResetPlayback()
{
	workingEffect = NULL;
}

void BaseFilter::SaveCore(IFileSystem * outFile)
{
	U32 dwWritten;
	DAFILEDESC fdesc = "FILTER";
	fdesc.lpImplementation = "DOS";
	fdesc.dwDesiredAccess = GENERIC_READ|GENERIC_WRITE;
	fdesc.dwCreationDistribution = CREATE_NEW;
	
	COMPTR<IFileSystem> actionFile;
	if (outFile->CreateInstance(&fdesc, actionFile) == GR_OK)
	{
		FilterSaveHeader fSave;
		fSave.version = FILTER_SAVE_VERSION;
		fSave.type = filterType;
		fSave.dataSize = programer->GetDataChunkSize();
		fSave.numOutput = programer->GetNumOutput();
		fSave.numInput = programer->GetNumInput();
		fSave.xPos = xPos;
		fSave.yPos = yPos;
		fSave.linkID = linkID;
		actionFile->WriteFile(0,&fSave ,sizeof(FilterSaveHeader),&dwWritten);
		for(U32 i = 0; i < fSave.numOutput; ++i)
		{
			U32 outputLinkID = 0xFFFFFFFF;
			if(output[i])
				outputLinkID = output[i]->LinkID();
			actionFile->WriteFile(0,&outputLinkID ,sizeof(U32),&dwWritten);
		}
		for(U32 i = 0; i < fSave.numOutput; ++i)
		{
			actionFile->WriteFile(0,&(outputTargetID[i]),sizeof(U32),&dwWritten);
		}
		for(U32 i = 0; i < fSave.numInput; ++i)
		{
			U32 inputLinkID = 0xFFFFFFFF;
			if(input[i])
				inputLinkID = input[i]->LinkID();
			actionFile->WriteFile(0,&inputLinkID ,sizeof(U32),&dwWritten);
		}
		for(U32 i = 0; i < fSave.numInput; ++i)
		{
			actionFile->WriteFile(0,&(inputTargetID[i]),sizeof(U32),&dwWritten);
		}
		U8 * data = new U8[fSave.dataSize];
		programer->GetDataChunk(data);
		actionFile->WriteFile(0,data ,fSave.dataSize,&dwWritten);
		delete data;
	}
}

void BaseFilter::ReLink(IParticleFilter * instList)
{
	for(U32 i = 0 ;i < programer->GetNumOutput(); ++i)
	{
		IParticleFilter * search = instList;
		while(search)
		{
			if(*((U32*)(&(output[i]))) == search->LinkID())
			{
				break;
			}
			search = search->GetNextFilter();
		}
		output[i] = search;
	}
	for(U32 i = 0 ;i < programer->GetNumInput(); ++i)
	{
		IParticleFilter * search = instList;
		while(search)
		{
			if(*((U32*)(&(input[i]))) == search->LinkID())
			{
				break;
			}
			search = search->GetNextFilter();
		}
		input[i] = search;
	}
}

U32 BaseFilter::LinkID()
{
	return linkID;
}

void BaseFilter::SetLinkID(U32 newLinkID)
{
	linkID = newLinkID;
}

void BaseFilter::SetFilterName(const char * name)
{
	strncpy(filterName,name,31);
	filterName[31] = 0;
	programer->SetEffectName(filterName);
	ParticleView::InvalidateView();
}

const char * BaseFilter::GetFilterName()
{
	return filterName;
}

IParticleProgramer * BaseFilter::GetProgramer()
{
	return programer;
}

IParticleFilter * BaseFilter::CreateCopy()
{
	BaseFilter * copy = new BaseFilter(filterType,linkID);
	strcpy(copy->filterName,filterName);
	copy->xPos = xPos;
	copy->yPos = yPos;
	copy->filterType = filterType;

	//still need to match up input output pointers???

	U8 * data = new U8[programer->GetDataChunkSize()];
	programer->GetDataChunk(data);
	copy->programer->SetDataChunk(data);
	delete data;

	return copy;
}

void baseEffectProc (HWND hWindow, UINT message, WPARAM wParam, LPARAM lParam)
{
/*	switch(message)
	{
	case WM_CHILDACTIVATE:
		{
			ParticleView::InputClicked(hWindow);
		}
		break;
	case WM_MOVE:
		{
			IActionParticle * action = selectedAction->GetActionParticle();
			if(action)
			{
				IParticleFilter * filter = action->FindFilterFromHWND(hWindow);
				if(filter)
				{
					RECT rect;
					GetWindowRect(hWindow,&rect);
					POINT point;
					point.x = rect.left;
					point.y = rect.top;
					if(ScreenToClient(GetParent(hWindow),&point))
						filter->SetWindowPos(point.x,point.y);
				}
			}
			InvalidateRect(particleWorkArea,NULL,false);
		}
		break;
	}
*/}

struct FilterParamInfo
{
	enum Type
	{
		FILTER_NAME,
		TRANSFORM_TYPE,
		FLOAT_TYPE,
		OUTPUT_TYPE,
		STRING_TYPE,
		ENUM_TYPE,
		TARGET_TYPE,
	}type;
	union
	{
		TransformType * transType;
		FloatType * floatType;
	};
	U32 index;
};

IParticleFilter * editFilter;

BOOL CALLBACK editFilterNameProc(HWND hWindow, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch(message)
	{
	case WM_INITDIALOG:
		{
			HWND editWin = GetDlgItem(hWindow,IDC_EDIT_NAME);
			SetWindowText(editWin,editFilter->GetFilterName());
		}
		break;
	case WM_COMMAND:
		{
			if(LOWORD(wParam) == IDOK)
			{
				char buffer[256];
				HWND editWin = GetDlgItem(hWindow,IDC_EDIT_NAME);
				GetWindowText(editWin,buffer,255);
				buffer[255] = 0;
				editFilter->SetFilterName(buffer);
				EndDialog(hWindow,0);
			}
		}
		break;
	}
	return false;
}

void editFilterName(IParticleFilter * filter)
{
	editFilter = filter;
	DialogBox(hMainInst,MAKEINTRESOURCE(IDD_EDIT_FILTER_NAME),mainWindow,editFilterNameProc);
};

void editStringType(HWND hWindow, DWORD listItemID, RECT & itemRect,IParticleProgramer * prog,U32 index);

BOOL CALLBACK editStringProc(HWND hWindow, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch(message)
	{
	case WM_INITDIALOG:
		{
			SetWindowLong(hWindow,GWL_USERDATA,lParam);
			char * buffer = (char *)lParam;
			HWND editWin = GetDlgItem(hWindow,IDC_STRING_EDIT);
			SetWindowText(editWin,buffer);
		}
		break;
	case WM_COMMAND:
		{
			if(LOWORD(wParam) == IDOK)
			{
				char * buffer = (char *)(GetWindowLong(hWindow,GWL_USERDATA));
				HWND editWin = GetDlgItem(hWindow,IDC_STRING_EDIT);
				GetWindowText(editWin,buffer,255);
				buffer[255] = 0;
				EndDialog(hWindow,0);
			}
		}
		break;
	}
	return false;
}

U32 editIndex = 0;
DWORD editItemID = 0;

void editStringType(HWND hWindow, DWORD listItemID, RECT & itemRect,IParticleProgramer * prog,U32 index)
{
	if(strcmp("Material Name",prog->GetStringParamName(index)) == 0)//if it is a material open the material selection dialog
	{
		editIndex = index;
		editItemID = listItemID;
		char buffer[256];
		strcpy(buffer,prog->GetStringParam(index));
		HWND workCombo = GetDlgItem(hWindow,IDC_WORK_COMBO);

		SendMessage(workCombo,CB_RESETCONTENT,0,0);

		SendMessage(workCombo,CB_ADDSTRING,0,(DWORD)(""));
		IMaterial * matPtr = NULL;
		MATMAN->FindFirstMaterial(&matPtr);
		if(matPtr)
		{
			do
			{
				U32 index = SendMessage(workCombo,CB_ADDSTRING,0,(DWORD)(matPtr->GetName()));
				if(strcmp(buffer,matPtr->GetName()) == 0)
				{
					SendMessage(workCombo,CB_SETCURSEL,index,0);
				}
				MATMAN->FindNextMaterial(matPtr,&matPtr);
			}
			while(matPtr);
		}

		ShowWindow(workCombo,true);
		SetWindowPos(workCombo,HWND_TOP,itemRect.left,itemRect.top,itemRect.right-itemRect.left,itemRect.bottom-itemRect.top,0);
		SetFocus(workCombo);
		InvalidateRect(workCombo,NULL,false);
	}
	else//otherwise
	{
		char buffer[256];
		strcpy(buffer,prog->GetStringParam(index));
		DialogBoxParam(hMainInst,MAKEINTRESOURCE(IDD_STRING_EDITOR),mainWindow,editStringProc,(DWORD)buffer);
		prog->SetStringParam(index,buffer);
	}
}

struct EnumEditDef
{
	U32 index;
	IParticleProgramer * prog;
};

BOOL CALLBACK editEnumProc(HWND hWindow, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch(message)
	{
	case WM_INITDIALOG:
		{
			SetWindowLong(hWindow,GWL_USERDATA,lParam);
			EnumEditDef * def = (EnumEditDef *)lParam;
			HWND enumWin = GetDlgItem(hWindow,IDC_ENUM_COMBO);
			char buffer[256];
			U32 numEnum = def->prog->GetNumEnumValues(def->index);
			for(U32 i = 0 ; i < numEnum; ++i)
			{
				strcpy(buffer,def->prog->GetEnumValueName(def->index,i));
				U32 comboIndex = SendMessage(enumWin,CB_ADDSTRING,0,(DWORD)(&buffer));
				SendMessage(enumWin,CB_SETITEMDATA,comboIndex,i);
				if(def->prog->GetEnumParam(def->index) == i)
					SendMessage(enumWin,CB_SETCURSEL,comboIndex,0);
			}
		}
		break;
	case WM_COMMAND:
		{
			if(LOWORD(wParam) == IDOK)
			{
				EnumEditDef * def = (EnumEditDef *)(GetWindowLong(hWindow,GWL_USERDATA));
				HWND enumWin = GetDlgItem(hWindow,IDC_ENUM_COMBO);
				U32 comboIndex = SendMessage(enumWin,CB_GETCURSEL,0,0);
				if(comboIndex != -1)
				{
					def->prog->SetEnumParam(def->index,SendMessage(enumWin,CB_GETITEMDATA,comboIndex,0));
				}
				EndDialog(hWindow,0);
			}
		}
		break;
	}
	return false;
}

void editEnumType(IParticleProgramer * prog,U32 index)
{
	EnumEditDef enumEditDef;
	enumEditDef.index = index;
	enumEditDef.prog = prog;
	DialogBoxParam(hMainInst,MAKEINTRESOURCE(IDD_EDIT_ENUM),mainWindow,editEnumProc,(DWORD)(&enumEditDef));
}

BOOL CALLBACK editTargetProc(HWND hWindow, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch(message)
	{
	case WM_INITDIALOG:
		{
			HWND targetWin = GetDlgItem(hWindow,IDC_TARGET_COMBO);

			U32 comboIndex = SendMessage(targetWin,CB_ADDSTRING,0,(DWORD)"None");
			SendMessage(targetWin,CB_SETITEMDATA,comboIndex,-1);
			SendMessage(targetWin,CB_SETCURSEL,comboIndex,0);
			IEffectTarget * search = EFFECTFILE->GetFirstTarget();
			while(search)
			{
				comboIndex = SendMessage(targetWin,CB_ADDSTRING,0,(DWORD)(search->GetName()));
				SendMessage(targetWin,CB_SETITEMDATA,comboIndex,search->GetTargetID());
				if(((U32)lParam) == search->GetTargetID())
					SendMessage(targetWin,CB_SETCURSEL,comboIndex,0);
				search = search->GetNextTarget();
			}
		}
		break;
	case WM_COMMAND:
		{
			if(LOWORD(wParam) == IDOK)
			{
				HWND targetWin = GetDlgItem(hWindow,IDC_TARGET_COMBO);
				U32 comboIndex = SendMessage(targetWin,CB_GETCURSEL,0,0);
				if(comboIndex != -1)
					EndDialog(hWindow,SendMessage(targetWin,CB_GETITEMDATA,comboIndex,0));
				else
					EndDialog(hWindow,0);
			}
		}
		break;
	}
	return false;
}

void editTargetType(IParticleProgramer * prog,U32 index)
{
	U32 newParam = DialogBoxParam(hMainInst,MAKEINTRESOURCE(IDD_EDIT_TARGET),mainWindow,editTargetProc,prog->GetTargetParam(index));
	prog->SetTargetParam(index,newParam);
}

BOOL CALLBACK filterProc(HWND hWindow, UINT message, WPARAM wParam, LPARAM lParam)
{
	baseEffectProc(hWindow,message,wParam,lParam);
	switch(message)
	{
	case WM_INITDIALOG:
		{
			HWND workCombo = GetDlgItem(hWindow,IDC_WORK_COMBO);
			ShowWindow(workCombo,false);

			DWORD style = GetWindowLong(workCombo,GWL_STYLE);
			SetWindowLong(workCombo,GWL_STYLE,style|WS_CLIPSIBLINGS);

			SetWindowText(hWindow,PARTMAN->GetFilterName(initFilter->GetType()));

			HWND listView = GetDlgItem(hWindow,IDC_PARAM_LIST);
			ListView_SetExtendedListViewStyle(listView,LVS_EX_FULLROWSELECT|LVS_EX_GRIDLINES );
			style = GetWindowLong(listView,GWL_STYLE);
			SetWindowLong(listView,GWL_STYLE,style|WS_CLIPSIBLINGS);

			LVCOLUMN column;
			column.mask = LVCF_TEXT;
			column.pszText = "Name";
			ListView_InsertColumn(listView,0,&column);
			column.pszText = "Value";
			ListView_InsertColumn(listView,1,&column);

			RECT rect;
			GetClientRect(listView,&rect);

			ListView_SetColumnWidth(listView,0,(rect.right-rect.left)>>1);
			ListView_SetColumnWidth(listView,1,(rect.right-rect.left)>>1);
	
			IParticleProgramer * prog = initFilter->GetProgramer();
			LVITEM item;
			char buffer[256];
			//add transform types
			U32 numTransforms = prog->GetNumTransformParams();
			for(S32 i = numTransforms-1; i >= 0; --i)
			{
				FilterParamInfo * info = new FilterParamInfo();
				info->type = FilterParamInfo::TRANSFORM_TYPE;
				info->transType = (prog->GetTransformParam(i))?(prog->GetTransformParam(i)->CreateCopy()):NULL;
				info->index = i;
				item.mask = LVIF_TEXT |LVIF_PARAM;
				item.lParam = (DWORD)info;
				strcpy(buffer,prog->GetTransformParamName(i));
				item.pszText = buffer;
				item.iItem = 0;
				item.iSubItem = 0;
				U32 ipos = ListView_InsertItem(listView,&item);
				item.iItem = ipos;
				item.iSubItem = 1;
				item.mask = LVIF_TEXT;
				strcpy(buffer,(info->transType)?info->transType->GetStringDef():"No Value");
				item.pszText = buffer;
				ListView_SetItem(listView,&item);		
			}
			// add Enumerations
			U32 numEnum = prog->GetNumEnumParams();
			for(S32 i = numEnum-1; i >= 0; --i)
			{
				FilterParamInfo * info = new FilterParamInfo();
				info->type = FilterParamInfo::ENUM_TYPE;
				info->index = i;
				item.mask = LVIF_TEXT|LVIF_PARAM;
				item.lParam = (DWORD)info;
				strcpy(buffer,prog->GetEnumParamName(i));
				item.pszText = buffer;
				item.iItem = 0;
				item.iSubItem = 0;
				U32 ipos = ListView_InsertItem(listView,&item);
				item.iItem = ipos;
				item.iSubItem = 1;
				item.mask = LVIF_TEXT;
				strcpy(buffer,prog->GetEnumValueName(i,prog->GetEnumParam(i)));
				item.pszText = buffer;
				ListView_SetItem(listView,&item);
			}

			//add float types
			U32 numFloat = prog->GetNumFloatParams();
			for(S32 i = numFloat-1; i >= 0; --i)
			{
				FilterParamInfo * info = new FilterParamInfo();
				info->type = FilterParamInfo::FLOAT_TYPE;
				info->floatType = (prog->GetFloatParam(i))?(prog->GetFloatParam(i)->CreateCopy()):NULL;
				info->index = i;
				item.mask = LVIF_TEXT |LVIF_PARAM;
				item.lParam = (DWORD)info;
				strcpy(buffer,prog->GetFloatParamName(i));
				item.pszText = buffer;
				item.iItem = 0;
				item.iSubItem = 0;
				U32 ipos = ListView_InsertItem(listView,&item);
				item.iItem = ipos;
				item.iSubItem = 1;
				item.mask = LVIF_TEXT;
				strcpy(buffer,(info->floatType)?info->floatType->GetStringDef():"No Value");
				item.pszText = buffer;
				ListView_SetItem(listView,&item);		
			}

			//add target types
			U32 numTarget = prog->GetNumTargetParams();
			for(S32 i = numTarget-1; i >= 0; --i)
			{
				FilterParamInfo * info = new FilterParamInfo();
				info->type = FilterParamInfo::TARGET_TYPE;
				info->index = i;
				item.mask = LVIF_TEXT |LVIF_PARAM;
				item.lParam = (DWORD)info;
				strcpy(buffer,prog->GetTargetParamName(i));
				item.pszText = buffer;
				item.iItem = 0;
				item.iSubItem = 0;
				U32 ipos = ListView_InsertItem(listView,&item);
				item.iItem = ipos;
				item.iSubItem = 1;
				item.mask = LVIF_TEXT;
				itoa(prog->GetTargetParam(i),buffer,10);
				item.pszText = buffer;
				ListView_SetItem(listView,&item);		
			}
			// add Strings
			U32 numStrings = prog->GetNumStringParams();
			for(S32 i = numStrings-1; i >= 0; --i)
			{
				FilterParamInfo * info = new FilterParamInfo();
				info->type = FilterParamInfo::STRING_TYPE;
				info->index = i;
				item.mask = LVIF_TEXT|LVIF_PARAM;
				item.lParam = (DWORD)info;
				strcpy(buffer,prog->GetStringParamName(i));
				item.pszText = buffer;
				item.iItem = 0;
				item.iSubItem = 0;
				U32 ipos = ListView_InsertItem(listView,&item);
				item.iItem = ipos;
				item.iSubItem = 1;
				item.mask = LVIF_TEXT;
				strcpy(buffer,prog->GetStringParam(i));
				item.pszText = buffer;
				ListView_SetItem(listView,&item);	
			}

			//add filter name
			FilterParamInfo * info = new FilterParamInfo();
			info->type = FilterParamInfo::FILTER_NAME;
			item.mask = LVIF_TEXT |LVIF_PARAM;
			item.lParam = (DWORD)info;
			item.pszText = "Filter Name";
			item.iItem = 0;
			item.iSubItem = 0;
			U32 ipos = ListView_InsertItem(listView,&item);
			item.iItem = ipos;
			item.iSubItem = 1;
			item.mask = LVIF_TEXT;
			strcpy(buffer,initFilter->GetFilterName());
			item.pszText = buffer;
			ListView_SetItem(listView,&item);

			RECT winRect,deskRect;
			POINT point;
			GetWindowRect(hWindow,&winRect);
			GetCursorPos(&point);

			HWND deskWin = GetDesktopWindow();
			GetWindowRect(deskWin,&deskRect);

			int width = winRect.right-winRect.left;
			int height = winRect.bottom-winRect.top;

			int xPos = point.x-((width)/2);
			int yPos = point.y-((height)/2);

			if(xPos+width > deskRect.right)
				xPos = deskRect.right-width;
			if(xPos < 0)
				xPos = 0;

			if(yPos+height > deskRect.bottom)
				yPos = deskRect.bottom-height;
			if(yPos < 0)
				yPos = 0;

			MoveWindow(hWindow,xPos,yPos,width,height,true);
		}
		break;
	case WM_MOVE:
		{
			InvalidateRect(hWindow,NULL,false);
		}
		break;
	case WM_COMMAND:
		{
			switch(LOWORD(wParam))
			{
			case IDC_WORK_COMBO:
				{
					switch (HIWORD(wParam))
					{
					case CBN_KILLFOCUS:
						char buffer[256];
						HWND workCombo = GetDlgItem(hWindow,IDC_WORK_COMBO);
						GetWindowText(workCombo,buffer,255);
						buffer[255] = 0;
						if(selectedAction)
						{
							IActionParticle * action = selectedAction->GetActionParticle();
							if(action)
							{
								IParticleFilter * filter = action->FindFilterFromHWND(hWindow);
								if(filter)
								{
									filter->GetProgramer()->SetStringParam(editIndex,buffer);
									LVITEM item;
									item.mask = LVIF_TEXT;
									item.pszText = buffer;
									item.iItem = editItemID;
									item.iSubItem = 1;
									ListView_SetItem(GetDlgItem(hWindow, IDC_PARAM_LIST),&item);
								}
							}
						}
						ShowWindow(workCombo,false);
						break;
					}
					break;
				}
				break;
			}
		}break;
	case WM_NOTIFY:
		{
			HWND listView = GetDlgItem(hWindow, IDC_PARAM_LIST);
			NMHDR * header = (NMHDR *)lParam;
			if(header->hwndFrom == listView)
			{
				switch(header->code)
				{
				case NM_DBLCLK:
					{
						NMLISTVIEW * nmListView= (NMLISTVIEW *) lParam;
						if(nmListView->iItem != -1)
						{
							LVITEM item;
							item.iItem = nmListView->iItem;
							item.iSubItem = 0;
							item.mask = LVIF_PARAM;
							ListView_GetItem(listView,&item);
							FilterParamInfo * param = (FilterParamInfo *)(item.lParam);
							if(param && selectedAction)
							{
								switch(param->type)
								{
								case FilterParamInfo::FILTER_NAME:
									{
										IActionParticle * action = selectedAction->GetActionParticle();
										if(action)
										{
											IParticleFilter * filter = action->FindFilterFromHWND(hWindow);
											if(filter)
											{
												editFilterName(filter);
												char buffer[256];
												strcpy(buffer,filter->GetFilterName());
												item.mask = LVIF_TEXT;
												item.pszText = buffer;
												item.iItem = nmListView->iItem;
												item.iSubItem = 1;
												ListView_SetItem(listView,&item);
											}
										}
									}
									break;
								case FilterParamInfo::TRANSFORM_TYPE:
									{
										IActionParticle * action = selectedAction->GetActionParticle();
										if(action)
										{
											IParticleFilter * filter = action->FindFilterFromHWND(hWindow);
											if(filter)
											{
												param->transType = MathLayoutWin::EditTrans(param->transType);
												filter->GetProgramer()->SetTransformParam(param->index,param->transType);
												char buffer[256];
												strcpy(buffer,param->transType->GetStringDef());
												item.mask = LVIF_TEXT;
												item.pszText = buffer;
												item.iItem = nmListView->iItem;
												item.iSubItem = 1;
												ListView_SetItem(listView,&item);
											}
										}
										break;
									}
								case FilterParamInfo::FLOAT_TYPE:
									{
										IActionParticle * action = selectedAction->GetActionParticle();
										if(action)
										{
											IParticleFilter * filter = action->FindFilterFromHWND(hWindow);
											if(filter)
											{
												param->floatType = MathLayoutWin::EditFloat(param->floatType);
												filter->GetProgramer()->SetFloatParam(param->index,param->floatType);
												char buffer[256];
												strcpy(buffer,param->floatType->GetStringDef());
												item.mask = LVIF_TEXT;
												item.pszText = buffer;
												item.iItem = nmListView->iItem;
												item.iSubItem = 1;
												ListView_SetItem(listView,&item);
											}
										}
										break;
									}
								case FilterParamInfo::STRING_TYPE:
									{
										IActionParticle * action = selectedAction->GetActionParticle();
										if(action)
										{
											IParticleFilter * filter = action->FindFilterFromHWND(hWindow);
											if(filter)
											{
												RECT itemRect;
												ListView_GetSubItemRect(listView,nmListView->iItem,1,LVIR_BOUNDS,&itemRect);
/*												char oldEventName[32];
												if(filter->GetType() == PE_EVENT)
												{
													strcpy(oldEventName,filter->GetProgramer()->GetStringParam(0));
												}
*/												editStringType(hWindow,nmListView->iItem,itemRect,filter->GetProgramer(),param->index);
/*												if(filter->GetType() == PE_EVENT)
												{
													action->RenameFilterEvent(oldEventName, filter->GetProgramer()->GetStringParam(0));
												}
*/												char buffer[256];
												strcpy(buffer,filter->GetProgramer()->GetStringParam(param->index));
												item.mask = LVIF_TEXT;
												item.pszText = buffer;
												item.iItem = nmListView->iItem;
												item.iSubItem = 1;
												ListView_SetItem(listView,&item);
											}
										}
										break;
									}
								case FilterParamInfo::ENUM_TYPE:
									{
										IActionParticle * action = selectedAction->GetActionParticle();
										if(action)
										{
											IParticleFilter * filter = action->FindFilterFromHWND(hWindow);
											if(filter)
											{
												editEnumType(filter->GetProgramer(),param->index);
												char buffer[256];
												strcpy(buffer,filter->GetProgramer()->GetEnumValueName(param->index,filter->GetProgramer()->GetEnumParam(param->index)));
												item.mask = LVIF_TEXT;
												item.pszText = buffer;
												item.iItem = nmListView->iItem;
												item.iSubItem = 1;
												ListView_SetItem(listView,&item);
											}
										}
										break;
									}
								case FilterParamInfo::TARGET_TYPE:
									{
										IActionParticle * action = selectedAction->GetActionParticle();
										if(action)
										{
											IParticleFilter * filter = action->FindFilterFromHWND(hWindow);
											if(filter)
											{
												editTargetType(filter->GetProgramer(),param->index);
												char buffer[256];
												itoa(filter->GetProgramer()->GetTargetParam(param->index),buffer,10);
												item.mask = LVIF_TEXT;
												item.pszText = buffer;
												item.iItem = nmListView->iItem;
												item.iSubItem = 1;
												ListView_SetItem(listView,&item);
											}
										}
										break;
									}
								}
							}
						}
					}
					break;
				}
			}
		}
		break;
	case WM_CLOSE:
		{
			//delete this filter;
			if(selectedAction)
			{
				IActionParticle * action = selectedAction->GetActionParticle();
				if(action)
				{
					IParticleFilter * filter = action->FindFilterFromHWND(hWindow);
					if(filter)
					{
						filter->SetWindow(NULL);
						EndDialog(hWindow,0);
/*						IParticleFilter * search = action->GetFirstFilter();
						IParticleFilter * prev = NULL;
						while(search)
						{
							if(search == filter)
							{
								if(prev)
									prev->SetNextFilter(filter->GetNextFilter());
								else
									action->SetFirstFilter(filter->GetNextFilter());

								if(filter->GetType() == PE_EVENT)
								{
									action->RemoveFilterEvent(filter->GetProgramer()->GetStringParam(0));
								}

								filter->DestroyInput();
								filter->DestroyOutput(-1);
								delete (BaseFilter*)filter;
								EndDialog(hWindow,0);
								return false;
							}
							prev = search;
							search = search->GetNextFilter();
						}
*/					}
				}
			}
		}
		break;
	}
	return false;
}


struct EffectActionParticle: public EffectAction, IActionParticle,IParticleListener
{
	bool bOpen;
	bool bDisabled;

	IParticleFilter * firstFilter;

	U32 context;

	U32 linkID;

	EffectActionParticle();

	~EffectActionParticle();

	//IEffectAction
	virtual void Delete();

	virtual IActionParticle * GetActionParticle();

	virtual void StartAction();

	virtual void TriggerEvent(const char * event);

	virtual void AnimationFinished();

	virtual S32 GetDrawWidth();

	virtual bool IsOpen();

	virtual void SetOpen(bool setting);

	virtual void SaveCore(IFileSystem * outFile);

	virtual void LoadCore(IFileSystem * inFile, U32 _context, U32 version);

	virtual void NullTarget(struct IEffectTarget * target);

	virtual void NullTarget(struct ITargetAnim * target);

	virtual void NullTarget(struct ITargetHp * target);

	virtual IEffectAction * CreateCopy();

	//IActionParticle
	virtual IParticleFilter *  CreateNewFilter(enum ParticleEffectType type);

	virtual void DestroyFilter(IParticleFilter * filter);

	virtual IParticleFilter * FindFilterFromHWND(HWND hWindow);

	virtual IParticleFilter * GetFirstFilter();

	virtual void SetFirstFilter(IParticleFilter *  filter);

	virtual void ActivateFilterWindows();

	virtual void InsertFilter(IParticleFilter * filter);

	virtual void AddFilterEvent(const char * eventName);

	virtual void RemoveFilterEvent(const char * eventName);

	virtual void RenameFilterEvent(const char * oldName, const char * eventName);

	virtual void ImportLoadCore(struct IFileSystem * inFile);

	virtual bool IsDisabled();

	virtual void SetDisabled(bool bSetting);

	//IParticleListener
	virtual void ParticalEvent(const char * eventName);

};

EffectActionParticle::EffectActionParticle()
{
	linkID = 0;
	context = 0;
	strcpy(name,"newName");
	next = NULL;
	firstEvent = NULL;
	firstFilter = NULL;
	bOpen = false;
	bDisabled = false;
}

EffectActionParticle::~EffectActionParticle()
{
	while(firstEvent)
	{
		IEffectEvent * tmp = firstEvent;
		firstEvent = firstEvent->GetNextEvent();
		delete tmp;
	}
	while(firstFilter)
	{
		IParticleFilter * tmp = firstFilter;
		firstFilter = firstFilter->GetNextFilter();
		delete tmp;
	}
}

void EffectActionParticle::Delete()
{
	delete this;
}

IActionParticle * EffectActionParticle::GetActionParticle()
{
	return this;
}

void EffectActionParticle::StartAction()
{
	if(bDisabled)
		return;
	IParticleInstance * inst = PARTMAN->CreateBaseParticleInstance();
	IParticleFilter * search = firstFilter;
	while(search)
	{
		search->ResetPlayback();
		search = search->GetNextFilter();
	}
	search = firstFilter;
	while(search)
	{
		if(search->IsRoot())
		{
			search->AddToInstance(inst,NULL,0);
		}
		search = search->GetNextFilter();
	}
	inst->AddParticalListener(this);
	inst->Initialize(0);
	PreviewWin::AddParticleEffect(inst);
}

void EffectActionParticle::TriggerEvent(const char * event)
{
	IEffectEvent * search = firstEvent;
	while(search)
	{
		if(strcmp(search->GetName(),event) == 0)
		{
			search->TriggerEvent();
		}
		search = search->GetNextEvent();
	}
}

void EffectActionParticle::AnimationFinished()
{
}

S32 EffectActionParticle::GetDrawWidth()
{
	return 0;
}

bool EffectActionParticle::IsOpen()
{
	return bOpen;
}

void EffectActionParticle::SetOpen(bool setting)
{
	bOpen = setting;
}

void EffectActionParticle::SaveCore(IFileSystem * outFile)
{
	U32 dwWritten;
	DAFILEDESC fdesc = "ACTIONDATA";
	fdesc.lpImplementation = "DOS";
	fdesc.dwDesiredAccess = GENERIC_READ|GENERIC_WRITE;
//	fdesc.dwShareMode = 0;  // no sharing
	fdesc.dwCreationDistribution = CREATE_NEW;
	
	COMPTR<IFileSystem> actionFile;
	if (outFile->CreateInstance(&fdesc, actionFile) == GR_OK)
	{
		ActionSaveHeader aSave;
		aSave.version = ACTION_SAVE_VERSION;
		aSave.actionType = ActionSaveHeader::AT_PARTICLE;
		actionFile->WriteFile(0,&aSave ,sizeof(ActionSaveHeader),&dwWritten);
		actionFile = NULL;
	}

	DAFILEDESC fdesc2 = "PARTICLEDATA";
	fdesc2.lpImplementation = "DOS";
	fdesc2.dwDesiredAccess = GENERIC_READ|GENERIC_WRITE;
//	fdesc2.dwShareMode = 0;  // no sharing
	fdesc2.dwCreationDistribution = CREATE_NEW;
	COMPTR<IFileSystem> customFile;
	if (outFile->CreateInstance(&fdesc2, customFile) == GR_OK)
	{
		ActionParticleSave particleSave;
		strcpy(particleSave.actionName,name);
		particleSave.xPos = iconXPos;
		particleSave.yPos = iconYPos;
		customFile->WriteFile(0,&particleSave ,sizeof(particleSave),&dwWritten);
		customFile = NULL;
	}

	U32 fileId = 0;
	IParticleFilter * filterSearch = firstFilter;
	while(filterSearch)
	{
		char buffer[255];
		sprintf(buffer,"Filter%d",fileId);
		outFile->CreateDirectory(buffer);
		
		if (outFile->SetCurrentDirectory(buffer) == 0)
			return;

		filterSearch->SaveCore(outFile);
		
		if (outFile->SetCurrentDirectory("..") == 0)
			return;
		++fileId;			
		filterSearch = filterSearch->GetNextFilter();
	}


	fileId = 0;
	IEffectEvent * search = firstEvent;
	while(search)
	{
		char buffer[255];
		sprintf(buffer,"Event%d",fileId);
		outFile->CreateDirectory(buffer);
		
		if (outFile->SetCurrentDirectory(buffer) == 0)
			return;

		search->SaveCore(outFile);
		
		if (outFile->SetCurrentDirectory("..") == 0)
			return;
		++fileId;
		search = search->GetNextEvent();
	}
}

void EffectActionParticle::LoadCore(IFileSystem * inFile, U32 _context, U32 version)
{
	context = _context;
	U32 dwWritten;
	DAFILEDESC fdesc = "PARTICLEDATA";
	fdesc.dwDesiredAccess = GENERIC_READ;
	fdesc.dwShareMode = 0;  // no sharing
	
	IEffectAction * action = NULL;
	COMPTR<IFileSystem> actionData;
	if (inFile->CreateInstance(&fdesc, actionData) == GR_OK)
	{
		ActionParticleSave save;
		actionData->ReadFile(0,&(save) ,sizeof(ActionParticleSave),&dwWritten);
		strcpy(name,save.actionName);
		iconXPos = save.xPos;
		iconYPos = save.yPos;
	}

	WIN32_FIND_DATA data;
	HANDLE handle;
	handle = inFile->FindFirstFile("Filter*",&data);
	if(handle != INVALID_HANDLE_VALUE)
	{
		do
		{
			if(data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				inFile->SetCurrentDirectory(data.cFileName);

				IParticleFilter * filter = new BaseFilter(inFile,this,context);

				inFile->SetCurrentDirectory("..");
			}
		}while(inFile->FindNextFile(handle,&data));
		inFile->FindClose(handle);
	}

	IParticleFilter * search = firstFilter;
	U32 maxLink = 0;
	while(search)
	{
		if(search->LinkID() > maxLink)
			maxLink = search->LinkID();
		search->ReLink(firstFilter);
		search = search->GetNextFilter();
	}
	linkID = maxLink+1;
}

void EffectActionParticle::NullTarget(struct IEffectTarget * target)
{
	IEffectEvent * search = firstEvent;
	while(search)
	{
		search->NullTarget(target);
		search = search->GetNextEvent();
	}
}

void EffectActionParticle::NullTarget(struct ITargetAnim * target)
{
	IEffectEvent * search = firstEvent;
	while(search)
	{
		search->NullTarget(target);
		search = search->GetNextEvent();
	}
}

void EffectActionParticle::NullTarget(struct ITargetHp * target)
{
	IEffectEvent * search = firstEvent;
	while(search)
	{
		search->NullTarget(target);
		search = search->GetNextEvent();
	}
}

IEffectAction * EffectActionParticle::CreateCopy()
{
	EffectActionParticle * action = new EffectActionParticle();
	strcpy(action->name,name);
	if(firstEvent)
		action->firstEvent = firstEvent->CreateCopy(action);
	else
		action->firstEvent = NULL;
	if(next)
		action->next = next->CreateCopy();
	else
		action->next = NULL;
	action->bOpen = bOpen;

	IParticleFilter * search = firstFilter;
	while(search)
	{	
		IParticleFilter * filter = search->CreateCopy();
		filter->SetNextFilter(action->firstFilter);
		action->firstFilter = filter;
		search = search->GetNextFilter();
	}

	return action;
}

IParticleFilter * EffectActionParticle::CreateNewFilter(enum ParticleEffectType type)
{
	IParticleFilter * filter = new BaseFilter(type,linkID);
	++linkID;
	if(filter)
	{
		filter->SetNextFilter(firstFilter);
		firstFilter = filter;

/*		if(filter->GetType() == PE_EVENT)
		{
			AddFilterEvent(filter->GetProgramer()->GetStringParam(0));
		}
*/	}
	return filter;
}

void EffectActionParticle::DestroyFilter(IParticleFilter * filter)
{
	filter->SetWindow(NULL);
	IParticleFilter * search = firstFilter;
	IParticleFilter * prev = NULL;
	while(search)
	{
		if(search == filter)
		{
			if(prev)
				prev->SetNextFilter(filter->GetNextFilter());
			else
				firstFilter = filter->GetNextFilter();

/*			if(filter->GetType() == PE_EVENT)
			{
				RemoveFilterEvent(filter->GetProgramer()->GetStringParam(0));
			}
*/
			filter->DestroyInput(-1);
			filter->DestroyOutput(-1);
			delete (BaseFilter*)filter;
			return;
		}
		prev = search;
		search = search->GetNextFilter();
	}
}

IParticleFilter * EffectActionParticle::FindFilterFromHWND(HWND hWindow)
{
	IParticleFilter * search = firstFilter;
	while(search)
	{
		if(search->GetWindow() == hWindow)
			return search;
		search = search->GetNextFilter();
	}
	return NULL;
}

IParticleFilter * EffectActionParticle::GetFirstFilter()
{
	return firstFilter;
}

void EffectActionParticle::SetFirstFilter(IParticleFilter *  filter)
{
	firstFilter = filter;
}

void EffectActionParticle::ActivateFilterWindows()
{
/*	IParticleFilter * search = firstFilter;
	while(search)
	{
		search->InitWindow();
		search = search->GetNextFilter();
	}
*/
}

void EffectActionParticle::InsertFilter(IParticleFilter * filter)
{
	if(filter)
	{
		filter->SetNextFilter(firstFilter);
		firstFilter = filter;

/*		if(filter->GetType() == PE_EVENT)
		{
			AddFilterEvent(filter->GetProgramer()->GetStringParam(0));
		}
*/	}
}

void EffectActionParticle::AddFilterEvent(const char * eventName)
{
	IEffectEvent * search = firstEvent;
	while(search)
	{
		if((search->GetEventType() == EventSave::PARTICLE_CUE_TRIGGERED) && (strcmp(search->GetName(),eventName) == 0))
		{
			search->AddRef();
			return;
		}
		search = search->GetNextEvent();
	}
	//we did not find an existing event witht the name
	search = MakeEffectEvent();
	search->SetEventType(EventSave::PARTICLE_CUE_TRIGGERED);
	search->SetName(eventName);
	search->SetOffset(0);
	search->SetNextEvent(firstEvent);
	search->SetParent(this);
	search->AddRef();
	firstEvent = search;
}

void EffectActionParticle::RemoveFilterEvent(const char * eventName)
{
	IEffectEvent * search = firstEvent;
	IEffectEvent * prev = NULL;
	while(search)
	{
		if((search->GetEventType() == EventSave::PARTICLE_CUE_TRIGGERED) && (strcmp(search->GetName(),eventName) == 0))
		{
			search->RemoveRef();
			if(search->GetRef() == 0)
			{
				if(prev)
					prev->SetNextEvent(search->GetNextEvent());
				else
					firstEvent = search->GetNextEvent();
				delete search;
			}
			return;
		}
		prev = search;
		search = search->GetNextEvent();
	}
}

void EffectActionParticle::RenameFilterEvent(const char * oldName, const char * eventName)
{
	IEffectEvent * newTarg = firstEvent;
	while(newTarg)
	{
		if((newTarg->GetEventType() == EventSave::PARTICLE_CUE_TRIGGERED) && (strcmp(newTarg->GetName(),eventName) == 0))
		{
			break;
		}
		newTarg = newTarg->GetNextEvent();
	}
	IEffectEvent * search = firstEvent;
	IEffectEvent * prev = NULL;
	while(search)
	{
		if((search->GetEventType() == EventSave::PARTICLE_CUE_TRIGGERED) && (strcmp(search->GetName(),oldName) == 0))
		{
			if(search->GetRef() == 1)
			{
				if(newTarg)
				{
					//I have to delete the tree because an existing event already has my name
					newTarg->AddRef();
					if(prev)
						prev->SetNextEvent(search->GetNextEvent());
					else
						firstEvent = search->GetNextEvent();
					delete search;
				}
				else
				{
					search->SetName(eventName);//just rename it
				}
			}
			else
			{
				search->RemoveRef();
				if(newTarg)
				{
					newTarg->AddRef();//just move the ref
				}
				else
				{
					//create a new one.
					search = MakeEffectEvent();
					search->SetEventType(EventSave::PARTICLE_CUE_TRIGGERED);
					search->SetName(eventName);
					search->SetOffset(0);
					search->SetNextEvent(firstEvent);
					search->SetParent(this);
					search->AddRef();
					firstEvent = search;
				}
			}
			return;
		}
		prev = search;
		search = search->GetNextEvent();
	}
}

void EffectActionParticle::ImportLoadCore(struct IFileSystem * inFile)
{
	U32 dwWritten;
	DAFILEDESC fdesc = "ACTIONDATA";
	fdesc.dwDesiredAccess = GENERIC_READ;
	fdesc.dwShareMode = 0;  // no sharing
	
	IEffectAction * action = NULL;
	COMPTR<IFileSystem> actionData;
	if (inFile->CreateInstance(&fdesc, actionData) == GR_OK)
	{
		//just in case I need the version info later
		ActionSaveHeader save;
		actionData->ReadFile(0,&(save) ,sizeof(ActionSaveHeader),&dwWritten);

		U32 dwWritten;
		DAFILEDESC fdesc = "PARTICLEDATA";
		fdesc.dwDesiredAccess = GENERIC_READ;
		fdesc.dwShareMode = 0;  // no sharing
		
		IEffectAction * action = NULL;
		COMPTR<IFileSystem> actionData;
		if (inFile->CreateInstance(&fdesc, actionData) == GR_OK)
		{
			ActionParticleSave save;
			actionData->ReadFile(0,&(save) ,sizeof(ActionParticleSave),&dwWritten);
			strcpy(name,save.actionName);
		}

		//save off the current filter list;
		IParticleFilter * currentList = firstFilter;
		firstFilter = NULL;

		//now load up the new one
		WIN32_FIND_DATA data;
		HANDLE handle;
		handle = inFile->FindFirstFile("Filter*",&data);
		if(handle != INVALID_HANDLE_VALUE)
		{
			do
			{
				if(data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				{
					inFile->SetCurrentDirectory(data.cFileName);

					IParticleFilter * filter = new BaseFilter(inFile,this,context);

					inFile->SetCurrentDirectory("..");
				}
			}while(inFile->FindNextFile(handle,&data));
			inFile->FindClose(handle);
		}

		IParticleFilter * search = firstFilter;
		while(search)
		{
			search->ReLink(firstFilter);
			search = search->GetNextFilter();
		}

		search = firstFilter;
		IParticleFilter * prev = NULL;
		while(search)
		{
			search->SetLinkID(linkID);
			linkID++;
			prev = search;
			search = search->GetNextFilter();
		}
		if(prev)
			prev->SetNextFilter(currentList);
		else 
			firstFilter = currentList;
	}
}

bool EffectActionParticle::IsDisabled()
{
	return bDisabled;
}

void EffectActionParticle::SetDisabled(bool bSetting)
{
	bDisabled = bSetting;
}

void EffectActionParticle::ParticalEvent(const char * eventName)
{
	IEffectEvent * search = firstEvent;
	while(search)
	{
		if((search->GetEventType() == EventSave::PARTICLE_CUE_TRIGGERED) && (strcmp(search->GetName(),eventName) == 0))
		{
			search->TriggerEvent();
			return;
		}
		search = search->GetNextEvent();
	}
}

IEffectAction * MakeEffectActionParticle()
{
	return new EffectActionParticle();
}

