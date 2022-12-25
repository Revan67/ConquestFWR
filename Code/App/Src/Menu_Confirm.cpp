//--------------------------------------------------------------------------//
//                                                                          //
//                             Menu_Confirm.cpp                             //
//                                                                          //
//               COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
   $Header: /Conquest/App/Src/Menu_Confirm.cpp 32    9/25/00 1:16p Sbarton $
*/
//--------------------------------------------------------------------------//
 
#include "pch.h"
#include <globals.h>
#include <wchar.h>

#include "Frame.h"
#include "IButton2.h"
#include "IStatic.h"
#include <DConfirm.h>
#include "Mission.h"
#include "MScroll.h"

#define MAX_CHARS 256

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
struct Menu_CQMessageBox : public DAComponent<Frame>
{
	//
	// data items
	//
	GT_MESSAGEBOX data;
	COMPTR<IStatic> background, title, message;
	COMPTR<IButton2> ok, cancel;
	COMPTR<IButton2> okAlone;

	wchar_t szMessage[MAX_CHARS], szTitle[MAX_CHARS];
	U32 type;

	//
	// instance methods
	//

	Menu_CQMessageBox (const GT_MESSAGEBOX * pData, wchar_t * _message, wchar_t * _title, U32 uType = MB_OK)
	{
		data = 	*pData;
		swprintf(szMessage, L"%s", _message);
		swprintf(szTitle, L"%s", _title);
		type = uType;

		eventPriority = EVENT_PRIORITY_CONFIRM_EXIT;
		parent->SetCallbackPriority(this, eventPriority);
		initializeFrame(NULL);
		init();
	}

	~Menu_CQMessageBox (void);

	void * operator new (size_t size)
	{
		return HEAP->ClearAllocateMemory(size, "Menu_CQMessageBox");
	}

	void   operator delete (void *ptr)
	{
		HEAP->FreeMemory(ptr);
	}

	/* IDocumentClient methods */

	DEFMETHOD(OnUpdate) (struct IDocument *doc, const C8 *message, void *parm);

	/* Menu_CQMessageBox methods */

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
		case IDS_YES:
		case IDS_OK:
			endDialog(1);
			break;

		case IDS_NO:
		case IDS_CANCEL:
			endDialog(0);
			break;
		}
	}

	virtual bool onEscPressed (void)	//  return true if message was handled
	{
		endDialog(0);
		return true;
	}

	void init (void);
};
//----------------------------------------------------------------------------------//
//
Menu_CQMessageBox::~Menu_CQMessageBox (void)
{
}
//----------------------------------------------------------------------------------//
//
GENRESULT Menu_CQMessageBox::OnUpdate (struct IDocument *doc, const C8 *message, void *parm)
{
	DWORD dwRead;

	doc->SetFilePointer(0,0);
	doc->ReadFile(0, &data, sizeof(data), &dwRead, 0);
	setStateInfo();

	return GR_OK;
}
//----------------------------------------------------------------------------------//
//
void Menu_CQMessageBox::setStateInfo (void)
{
	//
	// create members if not done already
	//
	
	// the screenRect is always in the middle of the screen
/*	if (CQFLAGS.b3DEnabled)
	{
		screenRect.left		= IDEAL2REALX(12);
		screenRect.right	= screenRect.left + IDEAL2REALX(615);
		screenRect.top		= IDEAL2REALY(30);
		screenRect.bottom	= screenRect.top + IDEAL2REALY(310);
	}
	else
	{
*/		screenRect.left		= IDEAL2REALX((SCREEN_WIDTH-615))/2;
		screenRect.right	= screenRect.left + IDEAL2REALX(615);
		screenRect.top		= IDEAL2REALY((SCREEN_HEIGHT-310))/2;
		screenRect.bottom	= screenRect.top + IDEAL2REALY(310);
//	}

	background->InitStatic(data.background, this);
	title->InitStatic(data.title, this);
	message->InitStatic(data.message, this);

	ok->InitButton(data.ok, this); 
	cancel->InitButton(data.cancel, this);
	okAlone->InitButton(data.okAlone, this);

	if (szMessage[0])
	{
		message->SetText(szMessage);
	}
	if (szTitle[0])
	{
		title->SetText(szTitle);
	}

	switch (type)
	{
	case MB_OK:
		okAlone->SetVisible(true);
		ok->SetVisible(false);
		cancel->SetVisible(false);
		break;

	case MB_OKCANCEL:
		okAlone->SetVisible(false);
		ok->SetVisible(true);
		cancel->SetVisible(true);
		break;

	case MB_YESNO:
		okAlone->SetVisible(false);
		ok->SetVisible(true);
		cancel->SetVisible(true);
		ok->SetTextID(IDS_YES);
		ok->SetControlID(IDS_YES);
		cancel->SetTextID(IDS_NO);
		cancel->SetControlID(IDS_NO);
		break;

	default:
		CQBOMB0("Which type of message box??");
		break;
	}

	if (childFrame)
	{
		childFrame->setStateInfo();
	}
	else
	{
		if (type == MB_OK)
		{
			setFocus(okAlone);
		}
		else
		{
			setFocus(ok);
		}
	}
}
//--------------------------------------------------------------------------//
//
void Menu_CQMessageBox::init (void)
{
	data = 	*((const GT_MESSAGEBOX *) GENDATA->GetArchetypeData("CQMessageBox"));
	COMPTR<IDAComponent> pComp;

	GENDATA->CreateInstance(data.background.staticType, pComp);
	pComp->QueryInterface("IStatic", background);

	GENDATA->CreateInstance(data.title.staticType, pComp);
	pComp->QueryInterface("IStatic", title);

	GENDATA->CreateInstance(data.message.staticType, pComp);
	pComp->QueryInterface("IStatic", message);

	GENDATA->CreateInstance(data.ok.buttonType, pComp);
	pComp->QueryInterface("IButton2", ok);

	GENDATA->CreateInstance(data.cancel.buttonType, pComp);
	pComp->QueryInterface("IButton2", cancel);

	GENDATA->CreateInstance(data.okAlone.buttonType, pComp);
	pComp->QueryInterface("IButton2", okAlone);
}
//--------------------------------------------------------------------------//
//
U32 __stdcall CQMessageBox (wchar_t * message, wchar_t * title, U32 uType, BaseHotRect * _parent)
{
	const GT_MESSAGEBOX * pData;
	
	if (GENDATA==0 || (pData = ((const GT_MESSAGEBOX *) GENDATA->GetArchetypeData("CQMessageBox"))) == 0)
	{
		return MessageBoxW(hMainWindow, message, title, uType);
	}
	else
	{
		GENDATA->FlushUnusedArchetypes();
		Menu_CQMessageBox * mbox = new Menu_CQMessageBox(pData, message, title, uType);

		mbox->createViewer("\\GT_MESSAGEBOX\\CQMessageBox", "GT_MESSAGEBOX", IDS_VIEWMESSAGEBOX);

		bool bOldCursorEnabled = CURSOR->IsCursorEnabled();
		CURSOR->EnableCursor(true);

		mbox->beginModalFocus();
		mbox->resPriority = RES_PRIORITY_HIGH;
		mbox->desiredOwnedFlags = RF_CURSOR;
		mbox->cursorID = IDC_CURSOR_ARROW;
		mbox->grabAllResources();

		BOOL bOldGamePaused = CQFLAGS.bGamePaused;

		if (PLAYERID == 0)
		{
			CQFLAGS.bGamePaused = TRUE;
			EVENTSYS->Send(CQE_PAUSE_CHANGE, (void *)(CQFLAGS.bGamePaused));
		}

		BOOL32 bScrollActive = MSCROLL->SetActive(false);

		U32 result = CQDoModal(mbox);

		// unpause the game, if we must
		if (PLAYERID == 0)
		{
			CQFLAGS.bGamePaused = bOldGamePaused;
			EVENTSYS->Send(CQE_PAUSE_CHANGE, (void *)(CQFLAGS.bGamePaused));
		}
				
		delete mbox;

		MSCROLL->SetActive(bScrollActive);
		CURSOR->EnableCursor(bOldCursorEnabled);

		GENDATA->FlushUnusedArchetypes();
		return result;
	}
}
//--------------------------------------------------------------------------//
//
U32 __stdcall CQMessageBox (U32 messageID, U32 titleID, U32 uType, BaseHotRect * _parent)
{
	wchar_t message[MAX_CHARS];
	wchar_t title[MAX_CHARS];

	wcsncpy(message, _localLoadStringW(messageID), sizeof(message)/sizeof(wchar_t));
	wcsncpy(title, _localLoadStringW(titleID), sizeof(title)/sizeof(wchar_t));

	return CQMessageBox(message, title, uType, _parent);
}
//--------------------------------------------------------------------------//
//
U32 __stdcall CQMessageBox (U32 messageID, wchar_t * title, U32 uType, BaseHotRect * _parent)
{
	wchar_t message[MAX_CHARS];
	wcsncpy(message, _localLoadStringW(messageID), sizeof(message)/sizeof(wchar_t));
	return CQMessageBox(message, title, uType, _parent);
}
//--------------------------------------------------------------------------//
//
U32 __stdcall CQMessageBox (wchar_t * message, U32 titleID, U32 uType, BaseHotRect * _parent)
{
	wchar_t title[MAX_CHARS];
	wcsncpy(title, _localLoadStringW(titleID), sizeof(title)/sizeof(wchar_t));

	return CQMessageBox(message, title, uType, _parent);
}
//--------------------------------------------------------------------------//
//-----------------------------End Menu_Confirm.cpp-------------------------//
//--------------------------------------------------------------------------//
