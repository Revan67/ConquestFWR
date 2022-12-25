//--------------------------------------------------------------------------//
//                                                                          //
//                           Menu_PlayerChat.cpp                            //
//                                                                          //
//               COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
   $Header: /Conquest/App/Src/Menu_PlayerChat.cpp 17    10/05/00 10:49p Sbarton $
*/
//--------------------------------------------------------------------------//
 
#include "pch.h"
#include <globals.h>
#include <MGlobals.h>
#include <DPlayerMenu.h>

#include "Frame.h"
#include "IButton2.h"
#include "IStatic.h"
#include "IListbox.h"
#include "IEdit2.h"

#include "Mission.h"
#include "Hotkeys.h"
#include "CQGame.h"
#include "CommPacket.h"
#include "ScrollingText.h"

#include <stdio.h>

#define CHECK_BUTTONS_BEGIN		60000
#define CHAT_EDIT_CONTROL		70000

#define MAX_TEXT_LENGTH 100
#define NUM_OTHERS		7

using namespace CQGAMETYPES;

static bool g_bPushStates [] = { true, true, true, true, true, true, true };

//--------------------------------------------------------------------------//
//
struct dummy_menuplayerchat : public Frame, IPlayerChat
{
	BEGIN_DACOM_MAP_INBOUND(dummy_menuplayerchat)
	DACOM_INTERFACE_ENTRY(IDocumentClient)
	DACOM_INTERFACE_ENTRY(IPlayerChat)
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
struct Menu_PlayerChat : public DAComponent<dummy_menuplayerchat>
{
	//
	// data items
	//
	GT_PLAYERCHATMENU data;

	const CQGAME & cqgame;

	COMPTR<IStatic> background;
	COMPTR<IStatic> staticNames[NUM_OTHERS], staticRaces[NUM_OTHERS];
	COMPTR<IButton2> checkNames[NUM_OTHERS];
	COMPTR<IButton2> buttonAllies, buttonEnemies, buttonEveryone;
	COMPTR<IListbox> listChat;
	COMPTR<IEdit2> editChat;
	COMPTR<IButton2> buttonClose;
	COMPTR<IStatic> staticChat, staticTitle;

	int slotIndexArray[NUM_OTHERS];
	U32 numPlayers;

	U32 timer;
	bool bReady;

	//
	// instance methods
	//

	Menu_PlayerChat (const CQGAME & _cqgame) : cqgame(_cqgame)
	{
		bReady = false;
		eventPriority = EVENT_PRIORITY_DIPLOMACYMENU;
		parent->SetCallbackPriority(this, eventPriority);
		initializeFrame(NULL);
		init();
	}

	~Menu_PlayerChat (void);

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


	/* IPlayerChat methods */

	DEFMETHOD(AddTextString) (const wchar_t *string);

	DEFMETHOD(AddTextString) (const wchar_t *string, const U32 playerID, const U32 dplayID);


	/* Menu_PlayerChat methods */

	virtual void setStateInfo (void);

	virtual bool onEscPressed (void)	//  return true if message was handled
	{
		EVENTSYS->Send(CQE_DELETE_CHATFILTER_MENU, 0);
		return true;
	}

	virtual void onButtonPressed (U32 buttonID);

	virtual void onUpdate(U32 dt);

	void init (void);

	void setPlayersNames (void);

	void setPlayersColors (void);

	void updateAllyButton (int index);

	void sendChatMessage (void);

	void pushCheckBox(U32 index);

	void pushAllies (void);

	void pushEnemies (void);

	void pushEveryone (void);

	bool isEveryoneSelected (void);

	void addToChatList (const wchar_t * string, U32 colorID);
};
//----------------------------------------------------------------------------------//
//
Menu_PlayerChat::~Menu_PlayerChat (void)
{
}
//----------------------------------------------------------------------------------//
//
GENRESULT Menu_PlayerChat::OnUpdate (struct IDocument *doc, const C8 *message, void *parm)
{
	DWORD dwRead;

	doc->SetFilePointer(0,0);
	doc->ReadFile(0, &data, sizeof(data), &dwRead, 0);
	setStateInfo();

	return GR_OK;
}
//----------------------------------------------------------------------------------//
//
GENRESULT Menu_PlayerChat::AddTextString (const wchar_t *string)
{
	addToChatList(string, 0);
	return GR_OK;
}
//----------------------------------------------------------------------------------//
//
GENRESULT Menu_PlayerChat::AddTextString (const wchar_t *string, const U32 playerID, const U32 dplayID)
{
	// add a name to the beginning of the text and set a custom color for the added text
	wchar_t buffer[256];
	wchar_t name[64];

	// get the slotID for this player
	U32 slotIDs[8];
	U32 nSlots;
	
	nSlots = MGlobals::GetSlotIDForPlayerID(playerID, slotIDs); 

	// get the player's name
	if (nSlots > 1 && dplayID != 0)
	{
		MGlobals::GetPlayerNameFromDPID(dplayID, name, sizeof(name));
	}
	else
	{
		MGlobals::GetPlayerNameBySlot(slotIDs[0], name, sizeof(name));
	}
	swprintf(buffer, L"[%s] %s", name, string);

	addToChatList(buffer, playerID);

	return GR_OK;
}
//----------------------------------------------------------------------------------//
//
void Menu_PlayerChat::addToChatList (const wchar_t * string, U32 colorID)
{
	// may have to split the text up into 2 pieces
	const S32 bindex = listChat->GetBreakIndex(string);  // used for multi-line strings

	if (bindex <= 0)
	{
		const S32 index = listChat->AddString(string);
		listChat->EnsureVisible(index ? index-1 : 0);
		listChat->SetColorValue(index, COLORTABLE[colorID]);
	}
	else
	{
		// text has to be split up into 2 parts...
		wchar_t buffer[256];
		memcpy(buffer, string, bindex * sizeof(wchar_t));
		buffer[bindex] = 0;

		S32 index = listChat->AddString(buffer);
		listChat->SetColorValue(index, COLORTABLE[colorID]);
		listChat->EnsureVisible(index ? index-1 : 0);

		index = listChat->AddString(string+bindex);
		listChat->EnsureVisible(index ? index-1 : 0);
		listChat->SetColorValue(index, COLORTABLE[colorID]);
	}
}
//----------------------------------------------------------------------------------//
//
void Menu_PlayerChat::onUpdate (U32 dt)
{
	if (!bReady)
		return;

	// update every 100 ms (10 times a second)
	timer += dt;

	if (timer > 100)
	{
		// are all the players still in the game
		setPlayersColors();
		timer = 0;
	}
}
//----------------------------------------------------------------------------------//
//
void Menu_PlayerChat::sendChatMessage (void)
{
	// send the chat text to all the people we want to
	wchar_t buffer[MAX_TEXT_LENGTH];
	editChat->GetText(buffer, MAX_TEXT_LENGTH*sizeof(wchar_t));

	U32 sMsg = wcslen(buffer);
	buffer[sMsg++] = '\0';

	// create a packet and send it 
	NETTEXT nt;
	if (sMsg <= MAX_TEXT_LENGTH)
	{
		// get the message we're sending
		wcsncpy(nt.chatText, buffer, sMsg);
		nt.fromID = PLAYERID;
		nt.playerID = MGlobals::GetThisPlayer();
		nt.toID = 0;

		if (isEveryoneSelected())
		{
			nt.toID = 0xFF;
		}
		else
		{
			for (U32 i = 0; i < numPlayers; i++)
			{
				if (checkNames[i]->GetPushState())
				{
					// set the playerID'th bit
					nt.toID |= 1 << (MGlobals::GetPlayerIDFromSlot(slotIndexArray[i]) - 1);
				}
			}
		}
		nt.dwSize = sizeof(NETTEXT) - sizeof(nt.chatText) + sMsg*sizeof(wchar_t);
		NETPACKET->Send(0, NETF_ALLREMOTE, &nt);

		// don't forget to take care of it on our end
		SCROLLTEXT->SetTextStringEx(buffer, nt.playerID, nt.fromID);
	}

	// empty the chat edit box
	editChat->SetText(L"");
}
//----------------------------------------------------------------------------------//
//
void Menu_PlayerChat::onButtonPressed (U32 buttonID)
{
	// did we push one of check butons?
	U32 testID = buttonID - CHECK_BUTTONS_BEGIN;
	if (testID >= 0 && testID < NUM_OTHERS)
	{
		pushCheckBox(testID);
		return;
	}

	if (buttonID == CHAT_EDIT_CONTROL)
	{
		sendChatMessage();
	}

	// we pushed one of the lamer buttons
	switch (buttonID)
	{
	case IDS_CLOSE:
		// close the menu
		EVENTSYS->Send(CQE_DELETE_CHATFILTER_MENU, 0);
		break;

	case IDS_ALLIES:
		pushAllies();
		break;

	case IDS_ENEMIES:
		pushEnemies();
		break;

	case IDS_EVERYONE:
		pushEveryone();
		break;

	default:
		break;
	}
}
//----------------------------------------------------------------------------------//
//
void Menu_PlayerChat::pushAllies (void)
{
	// go through all the check boxes and set their pushstate to 'in' if they are allies
	const U32 myPlayerID	= MGlobals::GetThisPlayer();
	const U32 allyMask		= MGlobals::GetAllyMask(myPlayerID);
	U32 i;

	U32 hisPlayerID;

	for (i = 0; i < numPlayers; i++)
	{
		hisPlayerID = MGlobals::GetPlayerIDFromSlot(slotIndexArray[i]);
		if ((1 << (hisPlayerID-1) & allyMask))
		{
			// set the checkbox
			checkNames[i]->SetPushState(true);
		}
		else
		{
			checkNames[i]->SetPushState(false);
		}

		g_bPushStates[i] =  checkNames[i]->GetPushState();
	}
}
//----------------------------------------------------------------------------------//
//
void Menu_PlayerChat::pushEnemies (void)
{
	// go through all the check boxes and set their pushstate to 'in' if they are our enemy
	const U32 myPlayerID	= MGlobals::GetThisPlayer();
	const U32 allyMask		= MGlobals::GetAllyMask(myPlayerID);
	U32 i;

	U32 hisPlayerID;

	for (i = 0; i < numPlayers; i++)
	{
		hisPlayerID = MGlobals::GetPlayerIDFromSlot(slotIndexArray[i]);
		if ((1 << (hisPlayerID-1) & allyMask))
		{
			// set the checkbox to out - he's our enemy
			checkNames[i]->SetPushState(false);
		}
		else
		{
			checkNames[i]->SetPushState(true);
		}

		g_bPushStates[i] =  checkNames[i]->GetPushState();
	}
}
//----------------------------------------------------------------------------------//
//
void Menu_PlayerChat::pushEveryone (void)
{
	// select everyone for a talking to
	U32 i;

	for (i = 0; i < numPlayers; i++)
	{
		// set the checkbox
		checkNames[i]->SetPushState(true);
		g_bPushStates[i] =  true;
	}
}
//----------------------------------------------------------------------------------//
//
bool Menu_PlayerChat::isEveryoneSelected (void)
{
	U32 i;
	for (i = 0; i < numPlayers; i++)
	{
		if (checkNames[i]->GetPushState() == false && checkNames[i]->GetEnableState())
		{
			return false;
		}
	}

	return true;
}
//----------------------------------------------------------------------------------//
//
void Menu_PlayerChat::pushCheckBox (U32 index)
{
	CQASSERT(index >= 0 && index < NUM_OTHERS);

	bool bPushed = checkNames[index]->GetPushState();
	checkNames[index]->SetPushState(!bPushed);
	g_bPushStates[index] =  checkNames[index]->GetPushState();
}
//----------------------------------------------------------------------------------//
//
void Menu_PlayerChat::setPlayersNames (void)
{
	// set up the playerIndexArray
	U32 i;
	U32 playID;

	memset(slotIndexArray, -1, sizeof(int)*NUM_OTHERS);
	numPlayers = 0;

	for (i = 0; i < cqgame.activeSlots; i++)
	{
		if (cqgame.slot[i].state == READY)
		{
			playID = MGlobals::GetPlayerIDFromSlot(i);

			if (playID == MGlobals::GetThisPlayer())
			{
				continue;
			}
			else if (playID != 0)
			{
				slotIndexArray[numPlayers++] = i;
			}
		}
	}

	// now go through all the activated slots and set the names and races (and colors) of all the players
	wchar_t playerName[32];
	U32 id;
	for (i = 0; i < numPlayers; i++)
	{
		id = slotIndexArray[i];

		MGlobals::GetPlayerNameBySlot(id, playerName, sizeof(playerName));

		if (playerName[0] == 0 && cqgame.slot[id].type == CQGAMETYPES::COMPUTER)
		{
			wcsncpy(playerName, _localLoadStringW(IDS_COMPUTER_NAME), 32);			
		}

		if (cqgame.slot[id].race == TERRAN)
		{
			staticRaces[i]->SetText(_localLoadStringW(IDS_RACESELECT_HUMAN));
		}
		else if (cqgame.slot[id].race == MANTIS)
		{
			staticRaces[i]->SetText(_localLoadStringW(IDS_RACESELECT_MANTIS));
		}
		else if (cqgame.slot[id].race == SOLARIAN)
		{
			staticRaces[i]->SetText(_localLoadStringW(IDS_RACESELECT_SOLARIAN));
		}

		staticNames[i]->SetText(playerName);
	
		// set all the controls to visible
		staticNames[i]->SetVisible(true);
		staticRaces[i]->SetVisible(true);

		// enable check boxes for the players
		checkNames[i]->SetVisible(true);
	}

	setPlayersColors();

	bReady = true;
}
//----------------------------------------------------------------------------------//
//
void Menu_PlayerChat::setPlayersColors (void)
{
	// go through all the players and set their colors
	// have to call this often, in case a player resigns
	U32 i;
	U32 id;
	U32 colorID;
	COLORREF color;
	for (i = 0; i < numPlayers; i++)
	{
		id = slotIndexArray[i];
		// has the player resigned?  Then set their color to gray and disable all controls associated with them
		// can't send messages to them if they are computer players
		if (MGlobals::HasPlayerResigned(id))
		{
			colorID = UNDEFINEDCOLOR;

			// disable all the associated controls
		}
		else
		{
			colorID = COLOR(MGlobals::GetColorID(MGlobals::GetPlayerIDFromSlot(id)));
		}

		color = COLORTABLE[colorID];
		staticNames[i]->SetTextColor(color);
		staticRaces[i]->SetTextColor(color);
	}
}
//----------------------------------------------------------------------------------//
//
void Menu_PlayerChat::setStateInfo (void)
{
	//
	// create members if not done already
	//
	screenRect.left		= IDEAL2REALX(data.screenRect.left);
	screenRect.right	= IDEAL2REALX(data.screenRect.right);
	screenRect.top		= IDEAL2REALY(data.screenRect.top);
	screenRect.bottom	= IDEAL2REALY(data.screenRect.bottom);
	
	background->InitStatic(data.background, this);
	buttonAllies->InitButton(data.buttonAllies, this);
	buttonEnemies->InitButton(data.buttonEnemies, this);
	buttonEveryone->InitButton(data.buttonEveryone, this);

	staticTitle->InitStatic(data.staticTitle, this);
	staticChat->InitStatic(data.staticChat, this);

	// initialize the static controls for players names
	U32 i;
	for (i = 0; i < NUM_OTHERS; i++)
	{
		staticNames[i]->InitStatic(data.staticNames[i], this);
		staticNames[i]->SetVisible(false);
	
		staticRaces[i]->InitStatic(data.staticRaces[i], this);
		staticRaces[i]->SetText(L"Solarian");
		staticRaces[i]->SetVisible(false);

		checkNames[i]->InitButton(data.checkNames[i], this);
		checkNames[i]->SetControlID(CHECK_BUTTONS_BEGIN + i);
		checkNames[i]->SetPushState(g_bPushStates[i]);
		checkNames[i]->SetVisible(false);
	}

	listChat->InitListbox(data.listChat, this);
	
	editChat->InitEdit(data.editChat, this);

	editChat->SetControlID(CHAT_EDIT_CONTROL);
//	editChat->EnableToolbarBehavior();
	editChat->EnableChatboxBehavior();
	editChat->SetMaxChars(MAX_TEXT_LENGTH);
	
	buttonClose->InitButton(data.buttonClose, this);
	
	// fill the players names properly
	setPlayersNames();

	if (childFrame)
	{
		childFrame->setStateInfo();
	}
	else
	{
		setFocus(editChat);
//		setFocus(buttonClose);
	}
}
//--------------------------------------------------------------------------//
//
void Menu_PlayerChat::init (void)
{
	data = 	*((GT_PLAYERCHATMENU*) GENDATA->GetArchetypeData("Menu_PlayerChat"));

	COMPTR<IDAComponent> pComp;

	GENDATA->CreateInstance(data.background.staticType, pComp);
	pComp->QueryInterface("IStatic", background);

	GENDATA->CreateInstance(data.staticTitle.staticType, pComp);
	pComp->QueryInterface("IStatic", staticTitle);

	GENDATA->CreateInstance(data.staticChat.staticType, pComp);
	pComp->QueryInterface("IStatic", staticChat);

	GENDATA->CreateInstance(data.buttonClose.buttonType, pComp);
	pComp->QueryInterface("IButton2", buttonClose);

	GENDATA->CreateInstance(data.buttonAllies.buttonType, pComp);
	pComp->QueryInterface("IButton2", buttonAllies);

	GENDATA->CreateInstance(data.buttonEnemies.buttonType, pComp);
	pComp->QueryInterface("IButton2", buttonEnemies);

	GENDATA->CreateInstance(data.buttonEveryone.buttonType, pComp);
	pComp->QueryInterface("IButton2", buttonEveryone);
 
	U32 i;
	for (i = 0; i < NUM_OTHERS; i++)
	{
		GENDATA->CreateInstance(data.staticNames[i].staticType, pComp);
		pComp->QueryInterface("IStatic", staticNames[i]);

		GENDATA->CreateInstance(data.staticRaces[i].staticType, pComp);
		pComp->QueryInterface("IStatic", staticRaces[i]);

		GENDATA->CreateInstance(data.checkNames[i].buttonType, pComp);
		pComp->QueryInterface("IButton2", checkNames[i]);
	}

	GENDATA->CreateInstance(data.listChat.listboxType, pComp);
	pComp->QueryInterface("IListbox", listChat);

	GENDATA->CreateInstance(data.editChat.editType, pComp);
	pComp->QueryInterface("IEdit2", editChat);

	resPriority = RES_PRIORITY_HIGH;
	cursorID = IDC_CURSOR_ARROW;
	desiredOwnedFlags = RF_CURSOR;
	grabAllResources();
}
//--------------------------------------------------------------------------//
//
Frame * __stdcall CreateMenuPlayerChat (void)
{
	Menu_PlayerChat * menu = new Menu_PlayerChat(MGlobals::GetGameSettings());
	menu->createViewer("\\GT_PLAYERCHATMENU\\Menu_PlayerChat", "GT_PLAYERCHATMENU", IDS_VIEWPLAYERCHATMENU);
	menu->beginModalFocus();
	return menu;
}

//--------------------------------------------------------------------------//
//-----------------------------End Menu_PlayerChat.cpp----------------------//
//--------------------------------------------------------------------------//
