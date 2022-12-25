//--------------------------------------------------------------------------//
//                                                                          //
//                             Menu_LoadSave.cpp                            //
//                                                                          //
//               COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
   $Header: /Conquest/App/Src/Menu_LoadSave.cpp 46    9/26/01 9:14a Tmauer $
*/
//--------------------------------------------------------------------------//
 
#include "pch.h"
#include <globals.h>
#include <MGlobals.h>

#include "Frame.h"
#include "IButton2.h"
#include "IStatic.h"
#include "IEdit2.h"
#include "IListbox.h"
#include <DLoadSave.h>
#include "Mission.h"

#define MAX_GAMEDESC_CHARS 31

//--------------------------------------------------------------------------//
//
struct MenuLoadSave : public DAComponent<Frame>
{
	//
	// data items
	//
	GT_LOADSAVE data;
	COMPTR<IStatic> background, staticLoad, staticSave, staticFile;
	COMPTR<IButton2> open, save, cancel, deleteFile;
	COMPTR<IEdit2> editFile;
	COMPTR<IListbox> list;
	COMPTR<IFileSystem> saveLoadDir;

	const bool bLoad;
	const bool bSinglePlayerMaps;
	U32 dwSaveKey;

	//
	// instance methods
	//

	MenuLoadSave (bool _bLoad, bool _bSinglePlayer) : bLoad(_bLoad), bSinglePlayerMaps(_bSinglePlayer)
	{
		dwSaveKey = 1;
		eventPriority = EVENT_PRIORITY_CONFIRM_EXIT;
		parent->SetCallbackPriority(this, eventPriority);
		initializeFrame(NULL);
		init();
	}

	~MenuLoadSave (void);

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

	/* MenuLoadSave methods */

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
		case IDS_DELETE:
			onDeletePressed();
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

	void init (void);

	void initList (void);

	void onSelection (const wchar_t *buffer);

	void onButtonPressed (void);

	void onDeletePressed (void);

	void addFile (const WIN32_FIND_DATA & data);

	void addFile (HANDLE handle);

	void indexToFileName (int index, char * szFile); 
};
//----------------------------------------------------------------------------------//
//
MenuLoadSave::~MenuLoadSave (void)
{
}
//----------------------------------------------------------------------------------//
//
GENRESULT MenuLoadSave::OnUpdate (struct IDocument *doc, const C8 *message, void *parm)
{
	DWORD dwRead;

	doc->SetFilePointer(0,0);
	doc->ReadFile(0, &data, sizeof(data), &dwRead, 0);
	setStateInfo();

	return GR_OK;
}
//----------------------------------------------------------------------------------//
//
void MenuLoadSave::setStateInfo (void)
{
	//
	// create members if not done already
	//
	if (CQFLAGS.bGameActive)
	{
		screenRect.left		= IDEAL2REALX(data.screenRect.left);
		screenRect.right	= IDEAL2REALX(data.screenRect.right);
		screenRect.top		= IDEAL2REALY(data.screenRect.top);
		screenRect.bottom	= IDEAL2REALY(data.screenRect.bottom);
	}
	else
	{
		screenRect.left		= data.screenRect2D.left;
		screenRect.right	= data.screenRect2D.right;
		screenRect.top		= data.screenRect2D.top;
		screenRect.bottom	= data.screenRect2D.bottom;
	}

	background->InitStatic(data.background, this);
	staticLoad->InitStatic(data.staticLoad, this);
	staticSave->InitStatic(data.staticSave, this);
	staticFile->InitStatic(data.staticFile, this);

	editFile->InitEdit(data.editFile, this);
	list->InitListbox(data.list, this);
	open->InitButton(data.open, this); 

	save->InitButton(data.save, this); 
	cancel->InitButton(data.cancel, this); 
	deleteFile->InitButton(data.deleteFile, this);

	open->SetTransparent(true);
	save->SetTransparent(true);
	cancel->SetTransparent(true);
	deleteFile->SetTransparent(true);
	

	editFile->SetMaxChars(MAX_GAMEDESC_CHARS);
	editFile->EnableLockedTextBehavior();

	initList();

	editFile->DisableInput(bLoad == true);
	editFile->SetTransparentBehavior(true);

	if (bLoad)
	{
		staticFile->SetTextID(IDS_LOAD_DESCRIPTION);

		// if there are no games to load then inform the user and close the menu
		if (list->GetNumberOfItems() == 0)
		{
			SetVisible(false);
			CQMessageBox(IDS_HELP_NOGAMES, IDS_ERROR, MB_OK);
			endDialog(0);
		}
	}
	else
	{
		staticFile->SetTextID(IDS_SAVE_DESCRIPTION);
	}

	if (childFrame)
		childFrame->setStateInfo();
}
//--------------------------------------------------------------------------//
//
void MenuLoadSave::init (void)
{
	data = 	*((GT_LOADSAVE *) GENDATA->GetArchetypeData("MenuLoadSave"));

	COMPTR<IDAComponent> pComp;
	GENDATA->CreateInstance(data.open.buttonType, pComp);
	pComp->QueryInterface("IButton2", open);
	GENDATA->CreateInstance(data.cancel.buttonType, pComp);
	pComp->QueryInterface("IButton2", cancel);
	GENDATA->CreateInstance(data.deleteFile.buttonType, pComp);
	pComp->QueryInterface("IButton2", deleteFile);
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

	if (SAVEDIR == 0)
	{
		CQERROR0("Save Directory not found, tell Sean");
	}

	saveLoadDir = SAVEDIR; 

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

	resPriority = RES_PRIORITY_HIGH;
	cursorID = IDC_CURSOR_ARROW;
	desiredOwnedFlags = RF_CURSOR;
	grabAllResources();
}
//--------------------------------------------------------------------------//
//
void MenuLoadSave::addFile (const WIN32_FIND_DATA & data)
{
	// ignore directories
	if ((data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
	{
		wchar_t buffer[256];

		_localAnsiToWide(data.cFileName, buffer, sizeof(buffer));

		// get the game description string out of the file
		wchar_t string[MAX_GAMEDESC_CHARS];

		if (bSinglePlayerMaps)
		{
			if (data.cFileName[0] != 'f')
			{
				// all files in single player are of the form f0000000d.mission where d is a whole number
				return;
			}
		}
		else 
		{
			if (data.cFileName[0] != 'm')
			{
				// all files in multiplayer are of the form m0000000d.mission where d is a whole number
				return;
			}
		}

		// keep the save key the highest
		// get the save key number out of the file name
		// need a string that contains the number only
		char szKey[8];
		memset(szKey, 0, sizeof(szKey));
		strncpy(szKey, &data.cFileName[1], 7);
		U32 key = atol(szKey);
		if (key >= dwSaveKey)
		{
			dwSaveKey = key+1;
		}

		// do not add the quicksave game (key == 0) to the list if we are opening this dialog for saving purposes
		if (bLoad == false && key == 0)
		{
			return;
		}

		if (MISSION->GetFileDescription(data.cFileName, string, sizeof(string)/sizeof(wchar_t)))
		{
			U32 number, index;
			char szNumber[8];
			memset(szNumber, 0, sizeof(szNumber)); 

			index = list->AddString(string);
			
			// get the number associated with the file
			memcpy(szNumber, &data.cFileName[1], sizeof(char)*7);
			number = atol(szNumber);
			list->SetDataValue(index, number);
		}
	}
}
//--------------------------------------------------------------------------//
//
void MenuLoadSave::addFile (HANDLE handle)
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
void MenuLoadSave::indexToFileName (int index, char * szFile)
{
	U32 number = list->GetDataValue(index);
	
	if (bSinglePlayerMaps)
	{
		wsprintf(szFile, "f%07d.mission", number);
	}
	else
	{
		wsprintf(szFile, "m%07d.mission", number);
	}
}
//----------------------------------------------------------------------------------------//
//
void MenuLoadSave::initList (void)
{
	HANDLE handle;
	WIN32_FIND_DATA data;

	list->ResetContent();

	if (saveLoadDir==0 || (handle = saveLoadDir->FindFirstFile("*.mission", &data)) == INVALID_HANDLE_VALUE)
		return;

	addFile(handle);		// recursion
	addFile(data);
	
	saveLoadDir->FindClose(handle);

	// select the last game that was saved/loaded
	if (list->GetNumberOfItems())
	{
		list->SetCurrentSelection(0);
	}
}
//----------------------------------------------------------------------------------------//
//
void MenuLoadSave::onListSelection (S32 listID)
{
	S32 sel = list->GetCurrentSelection();
	wchar_t buffer[MAX_PATH];

	list->GetString(sel, buffer, sizeof(buffer));	// returns length of string (in characters)
	onSelection(buffer);
}
//--------------------------------------------------------------------------//
//
void MenuLoadSave::onListCaretMove (S32 listControlID)
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
void MenuLoadSave::onSelection (const wchar_t *buffer)
{
	char abuffer[MAX_PATH];
	_localWideToAnsi(buffer, abuffer, sizeof(abuffer));

	if (bLoad)
	{
		BOOL32 bWasEnabled;
		if ((bWasEnabled = CQFLAGS.b3DEnabled) == 0)
		{
			SetVisible(false);
			ChangeInterfaceRes(IR_IN_GAME_RESOLUTION);
		}

		// find the file the matches the description selected...
		indexToFileName(list->GetCurrentSelection(), abuffer);

		if (MISSION->Load(abuffer, saveLoadDir))
			endDialog(1);
		else
		{
			if (bWasEnabled)
				MISSION->New();
			else
			{
				ChangeInterfaceRes(IR_FRONT_END_RESOLUTION);
				SetVisible(true);
			}
		}
	}
	else
	{
		// if we are attempting to save over an existing file than query the user of the action
		bool bRewrite = false;
		int index = list->FindStringExact(buffer);
		if (index != -1)
		{
			wchar_t fileName[MAX_PATH];
			wcsncpy(fileName, buffer, sizeof(fileName)/sizeof(wchar_t));

			if (CQMessageBox(IDS_OVERWRITE_QUERY, fileName, MB_OKCANCEL, this) == 0)
			{
				return;
			}
			bRewrite = true;
		}

		// this is to force a file close
		MISSION->SetFileSystem(NULL);
 
		// are we re-writing over a file?
		if (bRewrite)
		{
			CQASSERT(index != -1);

			// find the file the matches the description selected...
			indexToFileName(index, abuffer);

			MISSION->SaveByDescription(buffer, saveLoadDir, abuffer);

		}
		else
		{
			MISSION->SaveByDescription(buffer, saveLoadDir, NULL, dwSaveKey);
		}

		endDialog(1);
	}
}
//--------------------------------------------------------------------------//
//
void MenuLoadSave::onButtonPressed (void)
{
	wchar_t buffer[MAX_PATH];
	
	if (editFile->GetText(buffer, sizeof(buffer)))
	{
		onSelection(buffer);
	}
}
//--------------------------------------------------------------------------//
//
void MenuLoadSave::onDeletePressed (void)
{
	wchar_t buffer[MAX_PATH];
	
	if (editFile->GetText(buffer, sizeof(buffer)))
	{
		int index = list->FindStringExact(buffer);
		if (index != -1)
		{
			wchar_t fileName[MAX_PATH];
			wcsncpy(fileName, buffer, sizeof(fileName)/sizeof(wchar_t));

			if (CQMessageBox(IDS_DELETE_QUERY, fileName, MB_OKCANCEL, this) == 0)
			{
				return;
			}
		}
		else
		{
			return;
		}

		// this is to force a file close
		MISSION->SetFileSystem(NULL);
 
		CQASSERT(index != -1);

		char abuffer[MAX_PATH];
		// find the file the matches the description selected...
		indexToFileName(index, abuffer);

		saveLoadDir->DeleteFile(abuffer);
		setStateInfo();
	}
}
//--------------------------------------------------------------------------//
//
U32 __stdcall CreateMenuLoadSave (bool bLoad, bool bSinglePlayerMaps)
{
	MenuLoadSave * menu = new MenuLoadSave(bLoad, bSinglePlayerMaps);
	BOOL32 bGamePaused = CQFLAGS.bGamePaused;

	menu->createViewer("\\GT_LOADSAVE\\MenuLoadSave", "GT_LOADSAVE", IDS_VIEWLOADSAVE);
	menu->beginModalFocus();

	if (PLAYERID==0)// not on the net
	{
		CQFLAGS.bGamePaused = 1;
		EVENTSYS->Send(CQE_PAUSE_CHANGE, (void *)(CQFLAGS.bGamePaused));
	}

	U32 result = CQDoModal(menu);
	delete menu;

	if (PLAYERID==0)	// not on the net
	{
		CQFLAGS.bGamePaused = bGamePaused;		//restore original value
		EVENTSYS->Send(CQE_PAUSE_CHANGE, (void *)(CQFLAGS.bGamePaused));
	}
	return result;
}
//--------------------------------------------------------------------------//
//
U32 __stdcall CreateMenuLoadSaveSpecial (bool bLoad, bool bSinglePlayerMaps)
{
	MenuLoadSave * menu = new MenuLoadSave(bLoad, bSinglePlayerMaps);

	BOOL32 bGamePaused = CQFLAGS.bGamePaused;

	menu->createViewer("\\GT_LOADSAVE\\MenuLoadSave", "GT_LOADSAVE", IDS_VIEWLOADSAVE);
	menu->beginModalFocus();

	if (PLAYERID==0)// not on the net
	{
		CQFLAGS.bGamePaused = 1;
		EVENTSYS->Send(CQE_PAUSE_CHANGE, (void *)(CQFLAGS.bGamePaused));
	}

	U32 result = CQDoModal(menu);
	delete menu;

	if (PLAYERID==0)	// not on the net
	{
		CQFLAGS.bGamePaused = bGamePaused;		//restore original value
		EVENTSYS->Send(CQE_PAUSE_CHANGE, (void *)(CQFLAGS.bGamePaused));
	}
	return result;
}
//--------------------------------------------------------------------------//
//-----------------------------End Menu_LoadSave.cpp------------------------//
//--------------------------------------------------------------------------//
