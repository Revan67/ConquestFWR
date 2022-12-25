//--------------------------------------------------------------------------//
//                                                                          //
//                             Menu_SysKitSaveLoad.cpp                            //
//                                                                          //
//               COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
   $Header: /Conquest/App/Src/Menu_SysKitSaveLoad.cpp 7     7/26/00 7:04p Sbarton $
*/
//--------------------------------------------------------------------------//
 
#include "pch.h"
#include <globals.h>

#include "Frame.h"
#include "IButton2.h"
#include "IStatic.h"
#include "IEdit2.h"
#include "IListbox.h"
#include <DSysKitSaveLoad.h>
#include "Mission.h"
#include "Sector.h"
#include "DSector.h"

//--------------------------------------------------------------------------//
//
struct MenuSystemKitSaveLoad : public DAComponent<Frame>
{
	//
	// data items
	//
	GT_SYSTEM_KIT_SAVELOAD data;
	COMPTR<IStatic> background, staticLoad, staticSave, staticFile;
	COMPTR<IButton2> open, save, cancel;
	COMPTR<IEdit2> editFile;
	COMPTR<IListbox> list;
	const bool bLoad;
	COMPTR<IDocument> saveLoadDir;

	//
	// instance methods
	//

	MenuSystemKitSaveLoad (IDocument * baseDir, bool _bLoad) : bLoad(_bLoad)
	{
		eventPriority = EVENT_PRIORITY_CONFIRM_EXIT;
		parent->SetCallbackPriority(this, eventPriority);
		initializeFrame(NULL);
		init(baseDir);
	}

	~MenuSystemKitSaveLoad (void);

    void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}

	/* IDocumentClient methods */

	DEFMETHOD(OnUpdate) (struct IDocument *doc, const C8 *message, void *parm);

	/* MenuSystemKitSaveLoad methods */

	virtual void setStateInfo (void);

	virtual bool onTabPressed (void)
	{
		if (childFrame!=0)
			return false;
		return Frame::onTabPressed();
	}

	virtual void onButtonPressed (U32 buttonID)
	{
		switch (buttonID)
		{
		case IDS_OPEN:
		case IDS_SAVE:
			onButtonPressed();
			break;
		case IDS_CANCEL:
			endDialog(0);
			break;
		}
	}

	virtual void onEditChanged (U32 editID)
	{
		wchar_t buffer[MAX_PATH];
		bool bEditEnabled;
		
		bEditEnabled = editFile->GetText(buffer, sizeof(buffer)) > 0;

		// disable the save and load buttons if there is no text in the edit box
		if (bLoad)
		{
			open->EnableButton(bEditEnabled);
		}
		else
		{
			save->EnableButton(bEditEnabled);
		}
	}


	virtual bool onEscPressed (void)	//  return true if message was handled
	{
		endDialog(0);
		return true;
	}

	virtual void onListCaretMove (S32 listControlID);		// user has moved the caret

	virtual void onListSelection (S32 listID);		// user has selected a list item

	void init (IDocument * baseDir);
	void initList (void);
	void onSelection (const wchar_t *buffer);
	void onButtonPressed (void);
	void addFile (const WIN32_FIND_DATA & data);
	void addFile (HANDLE handle);
};
//----------------------------------------------------------------------------------//
//
MenuSystemKitSaveLoad::~MenuSystemKitSaveLoad (void)
{
}
//----------------------------------------------------------------------------------//
//
GENRESULT MenuSystemKitSaveLoad::OnUpdate (struct IDocument *doc, const C8 *message, void *parm)
{
	DWORD dwRead;

	doc->SetFilePointer(0,0);
	doc->ReadFile(0, &data, sizeof(data), &dwRead, 0);
	setStateInfo();

	return GR_OK;
}
//----------------------------------------------------------------------------------//
//
void MenuSystemKitSaveLoad::setStateInfo (void)
{
	//
	// create members if not done already
	//
	screenRect.left		= IDEAL2REALX(data.screenRect.left);
	screenRect.right	= IDEAL2REALX(data.screenRect.right);
	screenRect.top		= IDEAL2REALY(data.screenRect.top);
	screenRect.bottom	= IDEAL2REALY(data.screenRect.bottom);

	background->InitStatic(data.background, this);
	staticLoad->InitStatic(data.staticLoad, this);
	staticSave->InitStatic(data.staticSave, this);
	staticFile->InitStatic(data.staticFile, this);

	list->InitListbox(data.list, this);
	editFile->InitEdit(data.editFile, this);
	open->InitButton(data.open, this); 
	save->InitButton(data.save, this); 
	cancel->InitButton(data.cancel, this); 

	editFile->SetIgnoreChars(L"-//\\.\"<>:|*?");

	initList();

	if (bLoad==0)		// set edit text to current file name
	{
		COMPTR<IFileSystem> file;
		if (MISSION->GetFileSystem(file) == GR_OK)
		{
			char cmpbuffer[MAX_PATH];
			wchar_t cmpbufferW[MAX_PATH];

			if (file->GetFileName(cmpbuffer, sizeof(cmpbuffer)))
			{
				_localAnsiToWide(cmpbuffer, cmpbufferW, sizeof(cmpbufferW));
				editFile->SetText(cmpbufferW);
				save->EnableButton(true);
			}
			else
			{
				save->EnableButton(false);
			}
		}
		else
		{
			save->EnableButton(false);
		}
	}


	if (childFrame)
		childFrame->setStateInfo();
}
//--------------------------------------------------------------------------//
//
void MenuSystemKitSaveLoad::init (IDocument * baseDir)
{
	data = 	*((GT_SYSTEM_KIT_SAVELOAD *) GENDATA->GetArchetypeData("MenuSystemKitSaveLoad"));

	COMPTR<IDAComponent> pComp;
	GENDATA->CreateInstance(data.open.buttonType, pComp);
	pComp->QueryInterface("IButton2", open);
	GENDATA->CreateInstance(data.cancel.buttonType, pComp);
	pComp->QueryInterface("IButton2", cancel);
	GENDATA->CreateInstance(data.save.buttonType, pComp);
	pComp->QueryInterface("IButton2", save);
	
	GENDATA->CreateInstance(data.background.staticType, pComp);
	pComp->QueryInterface("IStatic", background);
	GENDATA->CreateInstance(data.staticLoad.staticType, pComp);
	pComp->QueryInterface("IStatic", staticLoad);
	GENDATA->CreateInstance(data.staticSave.staticType, pComp);
	pComp->QueryInterface("IStatic", staticSave);
	GENDATA->CreateInstance(data.staticFile.staticType, pComp);
	pComp->QueryInterface("IStatic", staticFile);

	GENDATA->CreateInstance(data.list.listboxType, pComp);
	pComp->QueryInterface("IListbox", list);
	GENDATA->CreateInstance(data.editFile.editType, pComp);
	pComp->QueryInterface("IEdit2", editFile);

	editFile->SetControlID(IDS_OPEN);

	if (bLoad)
	{
		// initally, the open button is disabled
		open->EnableButton(false);
	}

	baseDir->CreateDirectory("\\GT_SYSTEM_KIT");

	DAFILEDESC fdesc = "\\GT_SYSTEM_KIT";
	fdesc.dwDesiredAccess = GENERIC_READ |GENERIC_WRITE;
	

	baseDir->CreateInstance(&fdesc,saveLoadDir);

	if (bLoad)
	{
		save->SetVisible(false);
		staticSave->SetVisible(false);
		setFocus(list);
	}
	else
	{
		open->SetVisible(false);
		staticLoad->SetVisible(false);
		setFocus(editFile);
	}
}
//--------------------------------------------------------------------------//
//
void MenuSystemKitSaveLoad::addFile (const WIN32_FIND_DATA & data)
{
	// ignore directories
	if ((data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
	{
		wchar_t buffer[256];

		_localAnsiToWide(data.cFileName, buffer, sizeof(buffer));
		list->AddString(buffer);
	}
}
//--------------------------------------------------------------------------//
//
void MenuSystemKitSaveLoad::addFile (HANDLE handle)
{
	WIN32_FIND_DATA data;

	if (saveLoadDir->FindNextFile(handle, &data))
	{
		addFile(handle);		// recursion
		addFile(data);
	}
}
//----------------------------------------------------------------------------------------//
//
void MenuSystemKitSaveLoad::initList (void)
{
	HANDLE handle;
	WIN32_FIND_DATA data;

	list->ResetContent();

	if (saveLoadDir==0 || (handle = saveLoadDir->FindFirstFile("*.*", &data)) == INVALID_HANDLE_VALUE)
		return;

	addFile(handle);		// recursion
	addFile(data);
	
	saveLoadDir->FindClose(handle);
}
//----------------------------------------------------------------------------------------//
//
void MenuSystemKitSaveLoad::onListSelection (S32 listID)
{
	S32 sel = list->GetCurrentSelection();
	wchar_t buffer[MAX_PATH];

	list->GetString(sel, buffer, sizeof(buffer));	// returns length of string (in characters)
	onSelection(buffer);
}
//--------------------------------------------------------------------------//
//
void MenuSystemKitSaveLoad::onListCaretMove (S32 listControlID)
{
	S32 sel = list->GetCurrentSelection();
	wchar_t buffer[MAX_PATH];

	if (sel >= 0)
	{
		list->GetString(sel, buffer, sizeof(buffer));	// returns length of string (in characters)
		editFile->SetText(buffer);
		if (bLoad)
		{
			open->EnableButton(true);
		}
		else
		{
			save->EnableButton(true);
		}
	}
}
//--------------------------------------------------------------------------//
//
void MenuSystemKitSaveLoad::onSelection (const wchar_t *buffer)
{
	char abuffer[MAX_PATH];
	
	_localWideToAnsi(buffer, abuffer, sizeof(abuffer));

	if (bLoad)
	{
		SECTOR->SetLightingKit(SECTOR->GetCurrentSystem(),abuffer);
		//do this to activate all the lights
		EVENTSYS->Send(CQE_CAMERA_MOVED,0);
	}
	else
	{
		COMPTR<IDocument> saveFile;

		DAFILEDESC fdesc = abuffer;
		fdesc.dwCreationDistribution = CREATE_ALWAYS;
		fdesc.dwDesiredAccess = GENERIC_READ |GENERIC_WRITE;
		fdesc.lpImplementation = "DOS";

		if(saveLoadDir->CreateInstance(&fdesc,saveFile) == GR_OK)
		{
			GT_SYSTEM_KIT kit = SECTOR->GetSystemLightKit(SECTOR->GetCurrentSystem());

			U32 written;

			saveFile->WriteFile(0,&kit,sizeof(GT_SYSTEM_KIT),&written,0);

			saveFile.free();
			SECTOR->SetLightingKit(SECTOR->GetCurrentSystem(),abuffer);
		}
		else
		{
			U32 error = saveLoadDir->GetLastError();
			CQERROR2("Could not create file %s, error %d",abuffer,error);
		}

	}
	endDialog(1);
}
//--------------------------------------------------------------------------//
//
void MenuSystemKitSaveLoad::onButtonPressed (void)
{
	wchar_t buffer[MAX_PATH];
//	S32 sel;
	
	if (editFile->GetText(buffer, sizeof(buffer)))
	{
		onSelection(buffer);
	}
/*	else
	if ((sel = list->GetCurrentSelection()) >= 0)
	{
		list->GetString(sel, buffer, sizeof(buffer));	// returns length of string (in characters)
		onSelection(buffer);
	}
	else
	if (bLoad==0)
	{
		if (MISSION->Save() == 0)
			CQERROR0("Save Failed!");
		endDialog(1);
	}
*/
}
//--------------------------------------------------------------------------//
//
U32 __stdcall CreateMenuSystemKitSaveLoad (bool bLoad)
{
	COMPTR<IDocument> baseDir;

	if (GENDATA->GetDataFile(baseDir) == GR_OK)
	{
		MenuSystemKitSaveLoad * menu = new MenuSystemKitSaveLoad(baseDir, bLoad);
		BOOL32 bGamePaused = CQFLAGS.bGamePaused;

		menu->createViewer("\\GT_SYSTEM_KIT_SAVELOAD\\MenuSystemKitSaveLoad", "GT_SYSTEM_KIT_SAVELOAD", IDS_VIEWLOADSAVE);
		menu->beginModalFocus();

		CURSOR->SetCursor(IDC_CURSOR_ARROW);

		if (PLAYERID==0)// not on the net
		{
			CQFLAGS.bGamePaused = 1;
			EVENTSYS->Send(CQE_PAUSE_CHANGE, (void *)(CQFLAGS.bGamePaused));
		}

		U32 result = CQDoModal(menu);

		CURSOR->SetDefaultCursor();

		delete menu;

		if (PLAYERID==0)	// not on the net
		{
			CQFLAGS.bGamePaused = bGamePaused;		//restore original value
			EVENTSYS->Send(CQE_PAUSE_CHANGE, (void *)(CQFLAGS.bGamePaused));
		}
		return result;
	}
	else
	{
		CQERROR0("Function not available");
		return 0;
	}
}
//--------------------------------------------------------------------------//
//-----------------------------End Menu_SysKitSaveLoad.cpp------------------------//
//--------------------------------------------------------------------------//
