//--------------------------------------------------------------------------//
//                                                                          //
//                             Menu_Chat.cpp                               //
//                                                                          //
//               COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
   $Header: /Conquest/App/Src/Menu_Chat.cpp 43    5/09/01 11:20a Tmauer $
*/
//--------------------------------------------------------------------------//
 
#include "pch.h"
#include <globals.h>
#include <mglobals.h>
#include <mscript.h>

#include <stdio.h>

#include "Frame.h"
#include "IEdit2.h"
#include "IStatic.h"
#include <DChatMenu.h>
#include "ScrollingText.h"
#include "Hotkeys.h"
#include "NetPacket.h"
#include "CommPacket.h"
#include "OpAgent.h"
#include "IGameProgress.h"
#include "IBanker.h"
#include "FogOfWar.h"

#define EDIT_ID		1
#define MAX_TEXT_LENGTH 100

Frame * __stdcall CreateMenuChat (bool bChatAll);

//--------------------------------------------------------------------------//
//
struct MenuChat : public DAComponent<Frame>
{
	//
	// data items
	//
	GT_CHAT data;
	COMPTR<IEdit2> chatbox;
	COMPTR<IStatic> background, ask;

	EDIT_DATA editData;
	STATIC_DATA backData, statData;

	const bool bChatAll;

	//
	// instance methods
	//

	MenuChat (bool _bChatAll) : bChatAll(_bChatAll)
	{
		eventPriority = EVENT_PRIORITY_CHATMENU;
		parent->SetCallbackPriority(this, eventPriority);
		initializeFrame(NULL);
		init();
	}

	~MenuChat (void);

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


	/* Frame methods */
	virtual void onFocusChanged (void)
	{
		if (bHasFocus)
		{
			EVENTSYS->Send(CQE_DELETE_CHAT_MENU, 0);
		}
	}


	/* IEventCallback methods */

	GENRESULT __stdcall Notify (U32 message, void *param);

	/* MenuChat methods */

	virtual void setStateInfo (void);

	virtual bool onTabPressed (void)
	{
		if (childFrame!=0)
			return false;
		return Frame::onTabPressed();
	}

	virtual void onButtonPressed (U32 buttonID);

	void init (void);
	
	void initLobby (void);

	void onHandleEdit (void);

	bool checkForCheats (wchar_t * buffer);

	bool checkToggleCheating (wchar_t * buffer);

	void setMenuYPos (int yPos)
	{
		if (yPos == screenRect.top)
			return;

		// need to move the screen rect to the given ypos
		int diff = screenRect.top - yPos;
		::OffsetRect(&screenRect, 0, -diff);

		chatbox->InitEdit(editData, this); 
		background->InitStatic(backData, this);
		ask->InitStatic(statData, this);

		chatbox->SetTransparentBehavior(true);

		if (childFrame)
			childFrame->setStateInfo();
		else
		{
			setFocus(chatbox);
		}

		{
			S32 x, y;

			WM->GetCursorPos(x, y);
			if (CheckRect(x, y))
			{
				desiredOwnedFlags = RF_CURSOR;
				grabAllResources();
			}
		}
	}
};
//----------------------------------------------------------------------------------//
//
MenuChat::~MenuChat (void)
{
}
//----------------------------------------------------------------------------------//
//
GENRESULT MenuChat::OnUpdate (struct IDocument *doc, const C8 *message, void *parm)
{
	DWORD dwRead;

	doc->SetFilePointer(0,0);
	doc->ReadFile(0, &data, sizeof(data), &dwRead, 0);
	setStateInfo();

	return GR_OK;
}
//----------------------------------------------------------------------------------//
//
GENRESULT MenuChat::Notify (U32 message, void *param)
{
	MSG *msg = (MSG *) param;
	GENRESULT result=GR_OK;

	switch (message)
	{
	case CQE_CHATDLG_SETYPOS:
		setMenuYPos(int(param));
		break;
	
	case WM_MOUSEMOVE:
		if ((result = Frame::Notify(message, param)) == GR_OK)
		{
			if (bAlert)
			{
				desiredOwnedFlags = RF_CURSOR;
				grabAllResources();
			}
			else
			{
				desiredOwnedFlags = 0;
				releaseResources();
			}
		}
		return result;

	case WM_KEYDOWN:
		if (msg->wParam == VK_RETURN)
		{
			onButtonPressed(EDIT_ID);
		}
		break;

	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
		if ((result = Frame::Notify(message, param)) == GR_OK)
		{
			if (bAlert)
				result = GR_GENERIC;	// eat mouse press events
		}
		return result;
	}

	return Frame::Notify(message, param);
}
//----------------------------------------------------------------------------------//
//
void MenuChat::setStateInfo (void)
{
	//
	// create members if not done already
	//
	screenRect.left		= IDEAL2REALX(data.screenRect.left);  
	screenRect.right	= IDEAL2REALX(data.screenRect.right);
	screenRect.top		= IDEAL2REALY(data.screenRect.top);
	screenRect.bottom	= IDEAL2REALY(data.screenRect.bottom);

	// init the controls first in their proper order...
	background->InitStatic(data.background, this);
	ask->InitStatic(data.ask, this);
	if(bChatAll)
		ask->SetTextID(IDS_REQUEST_CHAT);
	else
		ask->SetTextID(IDS_REQUEST_TEAM_CHAT);
	chatbox->InitEdit(data.chatbox, this);

	// find the length of the string we're using in the 'ask' control
	const U32 string_width = REAL2IDEALX(ask->GetStringWidth() + 4);

	// the width of this control depends on the length of the string we're using
	backData = data.background;
	backData.width = string_width;
	background->InitStatic(backData, this);

	// the posistion of the edit control depends on the length of the text we're printing in the 'ask' control
	editData = data.chatbox;
	editData.xOrigin = string_width;
	chatbox->InitEdit(editData, this); 

	// the width of this control depends on the length of the string we're using in 'ask' plus the length of the editbox control
	statData = data.ask;
	statData.width = string_width + chatbox->GetEditWidth();
	if(bChatAll)
		statData.staticText = (STTXT::STATIC_TEXT)IDS_REQUEST_CHAT;
	else
		statData.staticText = (STTXT::STATIC_TEXT)IDS_REQUEST_TEAM_CHAT;

	// make sure that the width is less than 639
	if (statData.width >= 640)
	{
		statData.width = 639;
	}
	ask->InitStatic(statData, this);

	chatbox->SetTransparentBehavior(true);
	chatbox->EnableLockedTextBehavior();


	if (childFrame)
		childFrame->setStateInfo();
	else
	{
		setFocus(chatbox);
	}

	{
		S32 x, y;

		WM->GetCursorPos(x, y);
		if (CheckRect(x, y))
		{
			desiredOwnedFlags = RF_CURSOR;
			grabAllResources();
		}
	}
}
//--------------------------------------------------------------------------//
//
void MenuChat::init (void)
{
	data = 	*((GT_CHAT *) GENDATA->GetArchetypeData("MenuChat"));

	COMPTR<IDAComponent> pComp;
	GENDATA->CreateInstance(data.chatbox.editType, pComp);
	pComp->QueryInterface("IEdit2", chatbox);

	chatbox->EnableToolbarBehavior();
	chatbox->EnableChatboxBehavior();
	chatbox->SetMaxChars(MAX_TEXT_LENGTH);

	GENDATA->CreateInstance(data.background.staticType, pComp);
	pComp->QueryInterface("IStatic", background);

	GENDATA->CreateInstance(data.ask.staticType, pComp);
	pComp->QueryInterface("IStatic", ask);

	resPriority = RES_PRIORITY_HIGH;
	cursorID = IDC_CURSOR_ARROW;
}
//--------------------------------------------------------------------------//
//
void MenuChat::onButtonPressed (U32 buttonID)
{
	switch (buttonID)
	{
	case EDIT_ID:
		onHandleEdit();
		break;
	}
}
//--------------------------------------------------------------------------//
//
bool MenuChat::checkToggleCheating (wchar_t * buffer)
{
	if (_wcsicmp(buffer, _localLoadStringW(IDS_CHEATS_ENABLE)) == 0)
	{
		MISSION->ToggleCheating();
		return true;
	}

	return false;
}
//--------------------------------------------------------------------------//
//
bool MenuChat::checkForCheats (wchar_t * buffer)
{
	// is this the victory cheat?
	if (_wcsicmp(buffer, _localLoadStringW(IDS_CHEATS_VICTORY)) == 0)
	{
		// we have won this mission!
		SCROLLTEXT->SetText(IDS_CHEATS_WON);
		MScript::EndMissionVictory(MGlobals::GetMissionID());
		return true;
	}
	else if (_wcsicmp(buffer, _localLoadStringW(IDS_CHEATS_DEFEAT)) == 0)
	{
		// we have lost the mission
		SCROLLTEXT->SetText(IDS_CHEATS_LOST);
		MScript::EndMissionDefeat();
		return true;
	}
	else if (wcsstr(buffer, _localLoadStringW(IDS_CHEATS_SKIP)) != 0)
	{
		// we are attempting to unlock some missions
		wchar_t * pbuff = wcschr(buffer, L'#');
		if (pbuff)
		{
			U32 id = wcstol(++pbuff, NULL, 10);
			if (id == 1)
				id = 0;
			else if (id == 2)
				id = 1;
			else 
				id = id-2;

			SCROLLTEXT->SetText(IDS_CHEATS_UNLOCK);
			GAMEPROGRESS->SetTempMissionsCompleted(id);
			MScript::EndMissionDefeat();
			return true;
		}
	}
	else if (_wcsicmp(buffer, _localLoadStringW(IDS_CHEATS_QUICKBUILD)) == 0)
	{
		// everything costs nothing
		BOOL32 bInstant = CQFLAGS.bInstantBuilding = !CQFLAGS.bInstantBuilding;


		if (bInstant)
		{
			SCROLLTEXT->SetText(IDS_CHEATS_BUILDON);
		}
		else
		{
			SCROLLTEXT->SetText(IDS_CHEATS_BUILDOFF);
		}
		return true;
	}
	else if (_wcsicmp(buffer, _localLoadStringW(IDS_CHEATS_FREE)) == 0)
	{
		// everything costs nothing
		BOOL32 bFree = CQFLAGS.bEverythingFree = !CQFLAGS.bEverythingFree;

		if (bFree)
		{
			SCROLLTEXT->SetText(IDS_CHEATS_COSTOFF);
		}
		else
		{
			SCROLLTEXT->SetText(IDS_CHEATS_COSTON);
		}
		return true;
	}
	else if (wcsstr(buffer, _localLoadStringW(IDS_CHEATS_KILLPLAYER)) != 0)
	{
		// we are killing everything that a certain player has
		wchar_t * pbuff = wcschr(buffer, L'#');
		if (pbuff)
		{
			U32 playID = wcstol(++pbuff, NULL, 10);
			
			if (playID < MAX_PLAYERS)
			{
				THEMATRIX->TerminatePlayerUnits(playID);

				wchar_t buff[64];
				swprintf(buff, L"%s #%d", _localLoadStringW(IDS_CHEATS_DESTROY), playID);

				SCROLLTEXT->SetTextString(buff);
				return true;
			}
		}
	}
	else if (_wcsicmp(buffer, _localLoadStringW(IDS_CHEATS_FOGOFF)) == 0)
	{
		DEFAULTS->GetDefaults()->fogMode = FOGOWAR_NONE;

		SCROLLTEXT->SetTextString(L"No fog of war");
		return true;
	}
	else if (_wcsicmp(buffer, _localLoadStringW(IDS_CHEATS_FOGON)) == 0)
	{
		DEFAULTS->GetDefaults()->fogMode = FOGOWAR_NORMAL;

		SCROLLTEXT->SetTextString(L"Normal fog of war turned on");
		return true;
	}
	else if (_wcsicmp(buffer, _localLoadStringW(IDS_CHEATS_DISCOVERY)) == 0)
	{
		FOGOFWAR->ClearAllHardFog();
		SCROLLTEXT->SetTextString(L"Clearing fog of discovery");
		return true;
	}
	else if (_wcsicmp(buffer, _localLoadStringW(IDS_CHEATS_ORE)) == 0)
	{
		// add the amount of ore to the players bank
		U32 amount = 2500/METAL_MULTIPLIER;
		BANKER->AddMetal(MGlobals::GetThisPlayer(), amount);

		SCROLLTEXT->SetTextString(L"Ore awarded to player");
		return true;
	}
	else if (_wcsicmp(buffer, _localLoadStringW(IDS_CHEATS_GAS)) == 0)
	{
		// award some gas to the player
		U32 amount = 2500/GAS_MULTIPLIER;
		BANKER->AddGas(MGlobals::GetThisPlayer(), amount);

		SCROLLTEXT->SetTextString(L"Gas awarded to player");
		return true;
	}
	else if (_wcsicmp(buffer, _localLoadStringW(IDS_CHEATS_CREW)) == 0)
	{
		// award some crew to the player
		U32 amount = 250/CREW_MULTIPLIER;
		BANKER->AddCrew(MGlobals::GetThisPlayer(), amount);

		SCROLLTEXT->SetTextString(L"Crew awarded to player");
		return true;
	}
	else if (_wcsicmp(buffer, _localLoadStringW(IDS_CHEATS_RESOURCES)) == 0)
	{
		// give the player all three goodies
		U32 playerID = MGlobals::GetThisPlayer();
		BANKER->AddMetal(playerID, MGlobals::GetMaxMetal(playerID));
		BANKER->AddGas(playerID, MGlobals::GetMaxGas(playerID));
		BANKER->AddCrew(playerID, MGlobals::GetMaxCrew(playerID));

		SCROLLTEXT->SetTextString(L"Maxing out all of player's resources");
		return true;
	}

	// don't need these anymore, but maybe in the demo version?
/*	else if (wcscmp(buffer, _localLoadStringW(IDS_CHEATS_DEMO1)) == 0)
	{
		if (SPMAPDIR)
		{
			SCROLLTEXT->SetTextString(L"demo1 loaded ...");
			MISSION->Load("demo1.mission", SPMAPDIR);
			return true;
		}
	}
	else if (wcscmp(buffer, _localLoadStringW(IDS_CHEATS_DEMO2)) == 0)
	{
		if (SPMAPDIR)
		{
			SCROLLTEXT->SetTextString(L"demo2 loaded ...");
			MISSION->Load("demo2.mission", SPMAPDIR);
			return true;
		}
	}
*/
	return false;
}
//--------------------------------------------------------------------------//
//
void MenuChat::onHandleEdit (void)
{
	// do something with the data that has been typed in
	wchar_t buffer[MAX_TEXT_LENGTH];
	chatbox->GetText(buffer, MAX_TEXT_LENGTH*sizeof(wchar_t));

	// see if we are asking for a cheat - single player only
	if (buffer[0] && PLAYERID == 0)
	{
		if (checkToggleCheating(buffer) == false)
		{
			if (MISSION->GetCheatsEnabled() && checkForCheats(buffer))
			{
				return;
			}
		}
		else
		{
			// we've toggled cheating, re-write the buffer
			swprintf(buffer, L"cheating has been turned %s", MISSION->GetCheatsEnabled() ? L"on" : L"off");
			SCROLLTEXT->SetTextString(buffer);
			return;
		}
	}

	// make sure the null character is at the end of the string
	U32 sMsg = wcslen(buffer);
	buffer[sMsg++] = '\0';

	// create a packet and send it 
	NETTEXT nt;
	if (sMsg <= MAX_TEXT_LENGTH)
	{
		// get the message we're sending
		wcsncpy(nt.chatText, buffer, sizeof(nt.chatText) / sizeof(wchar_t));
		nt.fromID = PLAYERID;
		nt.playerID = MGlobals::GetThisPlayer();

		if (bChatAll)
		{
			nt.toID = 0xFF;
		}
		else
		{
			nt.toID = MGlobals::GetAllyMask(MGlobals::GetThisPlayer());
		}
		nt.dwSize = sizeof(NETTEXT) - sizeof(nt.chatText) + sMsg*sizeof(wchar_t);
		NETPACKET->Send(0, NETF_ALLREMOTE, &nt);

		// don't forget to take care of it on our end
		SCROLLTEXT->SetTextStringEx(buffer, nt.playerID, nt.fromID);
	}

	// also, need to send the message that destroys this menu
	EVENTSYS->Send(CQE_DELETE_CHAT_MENU, 0);
}
//--------------------------------------------------------------------------//
//
Frame * __stdcall CreateMenuChat (bool bChatAll)
{
	MenuChat * menu = new MenuChat(bChatAll);
	menu->createViewer("\\GT_CHAT\\MenuChat", "GT_CHAT", IDS_VIEWCHAT);

	return menu;
}

//--------------------------------------------------------------------------//
//-----------------------------End Menu_Chat.cpp---------------------------//
//--------------------------------------------------------------------------//