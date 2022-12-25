//--------------------------------------------------------------------------//
//                                                                          //
//                                Menu_newplayer.cpp                        //
//                                                                          //
//               COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
   $Header: /Conquest/App/Src/Menu_newplayer.cpp 26    10/24/00 5:04p Jasony $
*/
//--------------------------------------------------------------------------//
// New player dialog
//--------------------------------------------------------------------------//
 
#include "pch.h"
#include <globals.h>
#include <wchar.h>

#include "Frame.h"
#include "IButton2.h"
#include "IStatic.h"
#include "IEdit2.h"
#include "IListbox.h"
#include "Mission.h"
#include <DNewPlayer.h>

#define MAX_CHARS_NAME 13
#define MAX_EDIT_CHARS 256
#define EDIT_NAME 6000

#define GAME_DIR L"SavedGame\\"

//--------------------------------------------------------------------------//
//
struct dummy_newplayer : public Frame
{
	BEGIN_DACOM_MAP_INBOUND(dummy_newplayer)
	DACOM_INTERFACE_ENTRY(IDocumentClient)
	DACOM_INTERFACE_ENTRY_REF("IViewer", viewer)
	DACOM_INTERFACE_ENTRY(IEventCallback)
	
	// the following are for BaseHotRect
	DACOM_INTERFACE_ENTRY(IResourceClient)
	DACOM_INTERFACE_ENTRY(IDAConnectionPointContainer)
	DACOM_INTERFACE_ENTRY2(IID_IDAConnectionPointContainer, IDAConnectionPointContainer)
	END_DACOM_MAP()
};

//--------------------------------------------------------------------------//
//
struct Menu_newplayer : public DAComponent<dummy_newplayer>
{
	//
	// data items
	//
	GT_NEWPLAYER data;

	COMPTR<IStatic>  background, title, staticHeading;
	COMPTR<IEdit2>   edit;
	COMPTR<IButton2> ok, cancel;

	const wchar_t * szOldName;
	const bool bChangeName;

	wchar_t * bufferNew;
	int numChars;
	U32 timer;

	//
	// instance methods
	//

	Menu_newplayer (Frame * _parent, wchar_t * oldName, wchar_t * bufferNewName, int _numChars) 
		: bChangeName(oldName != NULL), szOldName(oldName)
	{
		bufferNew = bufferNewName;
		numChars = _numChars;
		initializeFrame(_parent);
		init();
	}

	~Menu_newplayer (void);

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


	virtual void onUpdate (U32 dt)
	{
		// check for text in the edit box 10 times a second
		timer += dt;

		if (timer > 100)
		{
			wchar_t buffer[10];
			ok->EnableButton(edit->GetText(buffer, 10) != 0);
			timer = 0;
		}
	}

	/* Menu_newplayer methods */

	virtual void setStateInfo (void);

	virtual void onButtonPressed (U32 buttonID);

	virtual bool onEscPressed (void)	//  return true if message was handled
	{
		endDialog(0);
		return true;
	}

	void init (void);

	U32 prepareNewName (void);
};
//----------------------------------------------------------------------------------//
//
Menu_newplayer::~Menu_newplayer (void)
{
}
//----------------------------------------------------------------------------------//
//
GENRESULT Menu_newplayer::OnUpdate (struct IDocument *doc, const C8 *message, void *parm)
{
	DWORD dwRead;

	doc->SetFilePointer(0,0);
	doc->ReadFile(0, &data, sizeof(data), &dwRead, 0);
	setStateInfo();

	return GR_OK;
}
//----------------------------------------------------------------------------------//
//
void Menu_newplayer::setStateInfo (void)
{
	//screenRect = data.screenRect;
	screenRect.left = IDEAL2REALX(data.screenRect.left);
	screenRect.right = IDEAL2REALX(data.screenRect.right);
	screenRect.top = IDEAL2REALY(data.screenRect.top);
	screenRect.bottom = IDEAL2REALY(data.screenRect.bottom);

	//
	// initialize in draw-order
	//
	background->InitStatic(data.background, this);
	title->InitStatic(data.title, this);
	edit->InitEdit(data.edit, this);
	ok->InitButton(data.ok, this);
	cancel->InitButton(data.cancel, this);
	staticHeading->InitStatic(data.staticHeading, this);

	edit->SetControlID(EDIT_NAME);
	ok->EnableButton(false);

	ok->SetTransparent(true);
	cancel->SetTransparent(true);


	//	< > : " / | * ?
	// don't want to have certain chars in the file name
	edit->SetMaxChars(MAX_CHARS_NAME+1);
	edit->SetIgnoreChars(L"-//\\.\"<>:|*?");

	if (szOldName)
	{
		edit->SetText(szOldName);
	}

	setFocus(edit);
}
//--------------------------------------------------------------------------//
//
void Menu_newplayer::init (void)
{
	data = 	*((GT_NEWPLAYER *) GENDATA->GetArchetypeData("MenuNewPlayer"));

	//
	// create members
	//
	COMPTR<IDAComponent> pComp;
	GENDATA->CreateInstance(data.background.staticType, pComp);
	pComp->QueryInterface("IStatic", background);

	GENDATA->CreateInstance(data.title.staticType, pComp);
	pComp->QueryInterface("IStatic", title);

	GENDATA->CreateInstance(data.staticHeading.staticType, pComp);
	pComp->QueryInterface("IStatic", staticHeading);

	GENDATA->CreateInstance(data.edit.editType, pComp);
	pComp->QueryInterface("IEdit2", edit);

	GENDATA->CreateInstance(data.ok.buttonType, pComp);
	pComp->QueryInterface("IButton2", ok);

	GENDATA->CreateInstance(data.cancel.buttonType, pComp);
	pComp->QueryInterface("IButton2", cancel);
}
//--------------------------------------------------------------------------//
//
U32 Menu_newplayer::prepareNewName (void)
{
	wchar_t buffer[MAX_CHARS_NAME+1];
	edit->GetText(buffer, sizeof(buffer));
	buffer[MAX_CHARS_NAME] = 0;

	wchar_t lowercase[MAX_CHARS_NAME+1];
	memset(lowercase, 0, sizeof(lowercase));
	wcscpy(lowercase, buffer);
	_wcslwr(lowercase);
	bool bBadName = false;

	// first make sure the name entered isn't some kind of system command thing
	if (wcscmp(lowercase, L"aux") == 0)
	{
		bBadName = true;
	}
	else if (wcscmp(lowercase, L"con") == 0)
	{
		bBadName = true;
	}
	else if (wcscmp(lowercase, L"lpt1") == 0)
	{
		bBadName = true;
	}
	else if (wcscmp(lowercase, L"prn") == 0)
	{
		bBadName = true;
	}
	else if (wcscmp(lowercase, L"nul") == 0)
	{
		bBadName = true;
	}

	if (bBadName)
	{
		// clear out the edit control and don't do anything
		edit->SetText(L"");
		ok->EnableButton(false);
		return 0;
	}


	if (bufferNew)
	{
		// take out all the leading spaces (if any) that are in buffer
		wchar_t * bufferNoSpace = buffer;
		while (*bufferNoSpace == L' ')
		{
			bufferNoSpace++;
		}

		// are we asking to switch to the exact same name?
		if (bChangeName)
		{
			if (wcscmp(bufferNoSpace, szOldName) == 0)
			{
				SetVisible(false);
				CQMessageBox(IDS_PLAYER_EXISTS, IDS_ERROR, MB_OK);
				SetVisible(true);
				return 0;
			}
		}
		wcscpy(bufferNew, bufferNoSpace);
	}

	return 1;
}
//--------------------------------------------------------------------------//
//
void Menu_newplayer::onButtonPressed (U32 buttonID)
{
	switch (buttonID)
	{
	case EDIT_NAME:
	case IDS_OK:
		if (ok->GetEnableState())
		{
			U32 result = prepareNewName();

			if (result)
			{
				endDialog(result);
			}
		}
		break;

	case IDS_CANCEL:
		endDialog(0);
		break;
	}
}
//--------------------------------------------------------------------------//
//
U32 __stdcall DoMenu_newplayer (Frame * parent, wchar_t * szOldName, wchar_t * bufferNewName, int numChars)
{
	Menu_newplayer * dlg = new Menu_newplayer(parent, szOldName, bufferNewName, numChars);
	dlg->createViewer("\\GT_NEWPLAYER\\MenuNewPlayer", "GT_NEWPLAYER", IDS_VIEWNEWPLAYER);
	dlg->beginModalFocus();

	U32 result = CQDoModal(dlg);
	delete dlg;

	return result;
}

//--------------------------------------------------------------------------//
//-----------------------------End Menu_newplayer.cpp-----------------------//
//--------------------------------------------------------------------------//