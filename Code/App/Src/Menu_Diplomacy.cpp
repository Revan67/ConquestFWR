//--------------------------------------------------------------------------//
//                                                                          //
//                           Menu_Diplomacy.cpp                             //
//                                                                          //
//               COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
   $Header: /Conquest/App/Src/Menu_Diplomacy.cpp 32    5/03/01 5:19p Tmauer $
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
#include "IDiplomacyButton.h"
#include "OpAgent.h"

#include "Mission.h"
#include "Hotkeys.h"
#include "CQGame.h"
#include "CommPacket.h"
#include "Sector.h"

#include <stdio.h>

#define CREW_BUTTONS_BEGIN		60000
#define METAL_BUTTONS_BEGIN		70000
#define GAS_BUTTONS_BEGIN		80000
#define ALLY_BUTTONS_BEGIN      90000

#define NUM_OTHERS 7

using namespace CQGAMETYPES;

//--------------------------------------------------------------------------//
//
struct dummy_menudiplomacy : public Frame
{
	BEGIN_DACOM_MAP_INBOUND(dummy_menudiplomacy)
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
struct Menu_Diplomacy : public DAComponent<dummy_menudiplomacy>
{
	//
	// data items
	//
	GT_DIPLOMACYMENU data;

	const CQGAME & cqgame;

	COMPTR<IStatic> background;
	COMPTR<IStatic> staticTitle, staticName, staticRace, staticAllies, staticMetalTitle, staticGasTitle, staticCrewTitle;
	COMPTR<IButton2> buttonOk, buttonReset, buttonCancel, buttonApply;
	COMPTR<IStatic> staticNames[NUM_OTHERS], staticRaces[NUM_OTHERS];
	COMPTR<IButton2> buttonCrew[NUM_OTHERS], buttonMetal[NUM_OTHERS], buttonGas[NUM_OTHERS];
	COMPTR<IStatic> staticCrew, staticMetal, staticGas;
	COMPTR<IDiplomacyButton> diplomacyButtons[NUM_OTHERS];

	int slotIndexArray[NUM_OTHERS];
	
	U32 numPlayers;

	U32 timer;
	bool bReady;

	//
	// instance methods
	//

	Menu_Diplomacy (const CQGAME & _cqgame) : cqgame(_cqgame)
	{
		bReady = false;
		eventPriority = EVENT_PRIORITY_DIPLOMACYMENU;
		parent->SetCallbackPriority(this, eventPriority);
		initializeFrame(NULL);
		init();
	}

	~Menu_Diplomacy (void);

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

	/* IEventCallback methods */

	GENRESULT __stdcall Notify (U32 message, void *param);

	/* Menu_Diplomacy methods */

	virtual void setStateInfo (void);

	virtual bool onEscPressed (void)	//  return true if message was handled
	{
		EVENTSYS->Send(CQE_DELETE_DIPLOMACY_MENU, 0);
		return true;
	}

	virtual void onButtonPressed (U32 buttonID);

	virtual void onUpdate (U32 dt);

	void init (void);

	void setPlayersNames (void);

	void setPlayersColors (void);

	void setResources (void);

	void pushResourceButton (IButton2 * button, U32 addition = 1);

	void clearButtons (void);

	int getVirtualCrew (void);
	
	int getVirtualMetal (void);

	int getVirtualGas (void);

	void pushAllyButton (int index);
	
	void updateAllyButton (int index);

	void updateAllAllyButtons (void)
	{
		for (int i = 0; i < NUM_OTHERS; i++)
		{
			if (buttonCrew[i]->GetVisible() == false)
			{
				break;
			}
			else
			{
				updateAllyButton(i);
			}
		}
	}

	void transferGoods (void);

	void properPushStates (void);
};
//----------------------------------------------------------------------------------//
//
Menu_Diplomacy::~Menu_Diplomacy (void)
{
}
//----------------------------------------------------------------------------------//
//
GENRESULT Menu_Diplomacy::OnUpdate (struct IDocument *doc, const C8 *message, void *parm)
{
	DWORD dwRead;

	doc->SetFilePointer(0,0);
	doc->ReadFile(0, &data, sizeof(data), &dwRead, 0);
	setStateInfo();

	return GR_OK;
}
//----------------------------------------------------------------------------------//
//
GENRESULT Menu_Diplomacy::Notify (U32 message, void *param)
{
	switch (message)
	{
	case CQE_DIPLOMACYBUTTON:
		pushAllyButton(U32(param) - ALLY_BUTTONS_BEGIN);
		break;

	default:
		break;
	}

	return Frame::Notify(message, param);
}
//----------------------------------------------------------------------------------//
//
void Menu_Diplomacy::onUpdate (U32 dt)
{
	if (!bReady)
		return;

	// update every 100 ms (10 times a second)
	timer += dt;

	if (timer > 100)
	{
		// update our resources
		setResources();

		// are all the players still in the game
		setPlayersColors();

		// are there any new alliences?
		updateAllAllyButtons();

		timer = 0;
	}
}
//----------------------------------------------------------------------------------//
//
void Menu_Diplomacy::clearButtons (void)
{
	U32 i;
	for (i = 0; i < NUM_OTHERS; i++)
	{
		buttonCrew[i]->SetTextString(L"0");
		buttonMetal[i]->SetTextString(L"0");
		buttonGas[i]->SetTextString(L"0");
	}

	// clear the ally buttons - set their push state to what they were originally
	properPushStates();
}
//----------------------------------------------------------------------------------//
//
int Menu_Diplomacy::getVirtualCrew ()
{
	// update the resources, just in case we're a bit behind
	setResources();

	wchar_t szWide[32];
	char	szValue[32];

	staticCrew->GetText(szWide, sizeof(szWide));
	_localWideToAnsi(szWide, szValue, sizeof(szValue));
	return atoi(szValue);
}
//----------------------------------------------------------------------------------//
//
int Menu_Diplomacy::getVirtualMetal ()
{
	// update the resources, just in case we're a bit behind
	setResources();

	wchar_t szWide[32];
	char	szValue[32];

	staticMetal->GetText(szWide, sizeof(szWide));
	_localWideToAnsi(szWide, szValue, sizeof(szValue));
	return atoi(szValue);
}
//----------------------------------------------------------------------------------//
//
int Menu_Diplomacy::getVirtualGas ()
{
	// update the resources, just in case we're a bit behind
	setResources();

	wchar_t szWide[32];
	char	szValue[32];

	staticGas->GetText(szWide, sizeof(szWide));
	_localWideToAnsi(szWide, szValue, sizeof(szValue));
	return atoi(szValue);
}
//----------------------------------------------------------------------------------//
//
void Menu_Diplomacy::onButtonPressed (U32 buttonID)
{
	// did we push one of the crew buttons?
	U32 testID = buttonID - CREW_BUTTONS_BEGIN;
	int addition;

	if (testID >= 0 && testID < NUM_OTHERS)
	{
		addition = 50;
		if (getVirtualCrew() - addition >= 0)
		{
			pushResourceButton(buttonCrew[testID], addition);
		}
		return;
	}

	// did we push one of the metal buttons?
	testID = buttonID - METAL_BUTTONS_BEGIN;
	if (testID >= 0 && testID < NUM_OTHERS)
	{
		addition = 250;
		if (getVirtualMetal() - addition >= 0)
		{
			pushResourceButton(buttonMetal[testID], addition);
		}
		return;
	}

	// did we push one of the gas buttons?
	testID = buttonID - GAS_BUTTONS_BEGIN;
	if (testID >= 0 && testID < NUM_OTHERS)
	{
		addition = 250;
		if (getVirtualGas() - addition >= 0)
		{
			pushResourceButton(buttonGas[testID], addition);
		}
	}

	// we pushed one of the lamer buttons
	switch (buttonID)
	{
	case IDS_CANCEL:
		// close the menu and don't transfer any goodies
		EVENTSYS->Send(CQE_DELETE_DIPLOMACY_MENU, 0);
		break;

	case IDS_OK:
		// transfer the goodies and close the menu
		transferGoods();
		EVENTSYS->Send(CQE_DELETE_DIPLOMACY_MENU, 0);
		break;

	case IDS_APPLY:
		// transfer the goodies but don't close the menu
		transferGoods();
		clearButtons();
		updateAllAllyButtons();
		break;

	case IDS_RESET:
		// clear the crew, metal, and gas buttons
		clearButtons();
		break;
	}
}
//----------------------------------------------------------------------------------//
//
void Menu_Diplomacy::pushAllyButton (int index)
{
	// toggle between want-to-be-allies and don't-want-to-be-allies mode
	bool bPushed = !diplomacyButtons[index]->GetFirstState();
	diplomacyButtons[index]->SetFirstState(bPushed);
	updateAllyButton(index);

	// also want to push the buttons of any cooperative players
	const U32 hisPlayerID = MGlobals::GetPlayerIDFromSlot(slotIndexArray[index]);

	// do we have any matching playerID's for this guy
	for (U32 i = 0; i < numPlayers; i++)
	{
		if (i == (U32)index)
		{
			continue;
		}
		else
		{
			if (MGlobals::GetPlayerIDFromSlot(slotIndexArray[i]) == hisPlayerID)
			{
				diplomacyButtons[i]->SetFirstState(bPushed);
				updateAllyButton(index);
			}
		}
	}
}
//----------------------------------------------------------------------------------//
//
void Menu_Diplomacy::updateAllyButton (int index)
{
	const U32 myPlayerID	= MGlobals::GetThisPlayer();
	const U32 hisPlayerID	= MGlobals::GetPlayerIDFromSlot(slotIndexArray[index]);
	const U32 hisOneWayMask = MGlobals::GetOneWayAllyMask(hisPlayerID);

	const bool bPushed = diplomacyButtons[index]->GetFirstState();

	// the info may not be up to date, it is what we want to happen when okay is pressed
	if (bPushed)
	{
		// either he sees us as a hostile or not
		if ((1 << (myPlayerID-1) & hisOneWayMask))
		{
			// he wants to ally me
			diplomacyButtons[index]->SetFirstState(true);
			diplomacyButtons[index]->SetSecondState(true);
		}
		else
		{
			// I'm friendly, he isn't
			diplomacyButtons[index]->SetFirstState(true);
			diplomacyButtons[index]->SetSecondState(false);
		}
	}
	else
	{
		// I don't like him, but maybe he likes us
		if ((1 << (myPlayerID-1) & hisOneWayMask))
		{
			// he wants to ally me
			diplomacyButtons[index]->SetFirstState(false);
			diplomacyButtons[index]->SetSecondState(true);
		}
		else
		{
			// we both hate each other
			diplomacyButtons[index]->SetFirstState(false);
			diplomacyButtons[index]->SetSecondState(false);
		}

	}
}
//----------------------------------------------------------------------------------//
//
void Menu_Diplomacy::pushResourceButton (IButton2 * pButton, U32 addition)
{
	// add to whatever value is currently displayed in the button
	wchar_t szBuffer[32];
	char szValue[32];
	int value;

	pButton->GetText(szBuffer, sizeof(szBuffer));
	_localWideToAnsi(szBuffer, szValue, sizeof(szValue));

	value = atoi(szValue);

	value += addition;

	// now rewrite the string in the button
	swprintf(szBuffer, L"%u", value);
	pButton->SetTextString(szBuffer);
}
//----------------------------------------------------------------------------------//
//
void Menu_Diplomacy::setResources (void)
{
	// take our current resouce and subtract what we're planning to give away
	wchar_t szResource[32];

	U32 playID = MGlobals::GetThisPlayer();
	int crew = MGlobals::GetCurrentCrew(playID) * CREW_MULTIPLIER;
	int metal = MGlobals::GetCurrentMetal(playID) * METAL_MULTIPLIER;
	int gas = MGlobals::GetCurrentGas(playID) * GAS_MULTIPLIER;

	// go through all the crew buttons and subtract
	char szValue[32];
	U32 i;
	for (i = 0; i < NUM_OTHERS; i++)
	{
		if (buttonCrew[i]->GetVisible() == false)
		{
			break;
		}
		else
		{
			buttonCrew[i]->GetText(szResource, sizeof(szResource));
			_localWideToAnsi(szResource, szValue, sizeof(szValue));
			crew -= atol(szValue);
		}
	}

	// go through all the metal buttons and subtract
	for (i = 0; i < NUM_OTHERS; i++)
	{
		if (buttonMetal[i]->GetVisible() == false)
		{
			break;
		}
		else
		{
			buttonMetal[i]->GetText(szResource, sizeof(szResource));
			_localWideToAnsi(szResource, szValue, sizeof(szValue));
			metal -= atol(szValue);
		}
	} 

	// go through all the gas buttons and subtract
	for (i = 0; i < NUM_OTHERS; i++)
	{
		if (buttonGas[i]->GetVisible() == false)
		{
			break;
		}
		else
		{
			buttonGas[i]->GetText(szResource, sizeof(szResource));
			_localWideToAnsi(szResource, szValue, sizeof(szValue));
			gas -= atol(szValue);
		}
	}

	swprintf(szResource, L"%d", crew);
	staticCrew->SetText(szResource);

	swprintf(szResource, L"%d", metal);
	staticMetal->SetText(szResource);

	swprintf(szResource, L"%d", gas);
	staticGas->SetText(szResource);
}
//----------------------------------------------------------------------------------//
//
void Menu_Diplomacy::transferGoods (void)
{
	// complete the transactions
	wchar_t szWide[32];
	char szAnsi[32];
	U32 slotID;
	U32 playID;
	int amount;
	U32 i;

	// first, transfer Crew
	for (i = 0; i < numPlayers; i++)
	{
		slotID = slotIndexArray[i];
		playID = MGlobals::GetPlayerIDFromSlot(slotID);

		buttonCrew[i]->GetText(szWide, sizeof(szWide));
		_localWideToAnsi(szWide, szAnsi, sizeof(szAnsi));
		amount = atoi(szAnsi)/CREW_MULTIPLIER;

		if (amount)
		{
			GIFTCREW_PACKET packet;
			packet.dwSize = sizeof(GIFTCREW_PACKET);
			packet.giverID = MGlobals::GetThisPlayer();
			packet.recieveID = playID;
			packet.amount = amount;
			NETPACKET->Send(HOSTID, 0, &packet);
		}
	}

	// now, transfer the metal
	for (i = 0; i < numPlayers; i++)
	{
		slotID = slotIndexArray[i];
		playID = MGlobals::GetPlayerIDFromSlot(slotID);
		
		buttonMetal[i]->GetText(szWide, sizeof(szWide));
		_localWideToAnsi(szWide, szAnsi, sizeof(szAnsi));
		amount = atoi(szAnsi)/METAL_MULTIPLIER;

		if (amount)
		{
			GIFTORE_PACKET packet;
			packet.dwSize = sizeof(GIFTORE_PACKET);
			packet.giverID = MGlobals::GetThisPlayer();
			packet.recieveID = playID;
			packet.amount = amount;
			NETPACKET->Send(HOSTID, 0, &packet);
		}
	}

	// now, transfer the gas
	for (i = 0; i < numPlayers; i++)
	{
		slotID = slotIndexArray[i];
		playID = MGlobals::GetPlayerIDFromSlot(slotID);
		
		buttonGas[i]->GetText(szWide, sizeof(szWide));
		_localWideToAnsi(szWide, szAnsi, sizeof(szAnsi));
		amount = atoi(szAnsi)/GAS_MULTIPLIER;
 
		if (amount)
		{
			GIFTGAS_PACKET packet;
			packet.dwSize = sizeof(GIFTGAS_PACKET);
			packet.giverID = MGlobals::GetThisPlayer();
			packet.recieveID = playID;
			packet.amount = amount;
			NETPACKET->Send(HOSTID, 0, &packet);
		}
	}

	// now set all the appropiate ally flags
	for (i = 0; i < numPlayers; i++)
	{
		MGlobals::SetAlly(MGlobals::GetThisPlayer(), MGlobals::GetPlayerIDFromSlot(slotIndexArray[i]), diplomacyButtons[i]->GetFirstState());
		SECTOR->ComputeSupplyForAllPlayers();
	}

	ALLIANCE_PACKET packet;
	packet.dwSize = sizeof(ALLIANCE_PACKET);
	packet.playerID = MGlobals::GetThisPlayer();
	packet.allianceFlags = 0;

	for (i = 0; i < numPlayers; i++)
	{
		if (diplomacyButtons[i]->GetFirstState())
		{
			packet.allianceFlags |= 1 << (MGlobals::GetPlayerIDFromSlot(slotIndexArray[i]) - 1);
		}
	}

	// and of course you have to stay allied to yourself, dumbass
	packet.allianceFlags |= 1 << (packet.playerID - 1);
	NETPACKET->Send(HOSTID, 0, &packet);
}
//----------------------------------------------------------------------------------//
//
void Menu_Diplomacy::setPlayersNames (void)
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
			wcsncpy(playerName, _localLoadStringW(IDS_COMPUTER_NAME), sizeof(playerName) / sizeof(wchar_t));			
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
		buttonCrew[i]->SetVisible(true);
		buttonMetal[i]->SetVisible(true);
		buttonGas[i]->SetVisible(true);
		diplomacyButtons[i]->SetVisible(true);
	}

	setPlayersColors();

	bReady = true;
}
//----------------------------------------------------------------------------------//
//
void Menu_Diplomacy::setPlayersColors (void)
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
		if (MGlobals::HasPlayerResigned(id))
		{
			colorID = UNDEFINEDCOLOR;

			// disable all the associated controls
			buttonCrew[i]->SetVisible(false);
			buttonMetal[i]->SetVisible(false);
			buttonGas[i]->SetVisible(false);
			diplomacyButtons[i]->SetVisible(false);
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
void Menu_Diplomacy::properPushStates (void)
{
	const U32 myPlayerID	= MGlobals::GetThisPlayer();
	const U32 myOneWayMask	= MGlobals::GetOneWayAllyMask(myPlayerID);
	U32 hisPlayerID;
	U32 i;

	for (i = 0; i < numPlayers; i++)
	{
		hisPlayerID = MGlobals::GetPlayerIDFromSlot(slotIndexArray[i]);

		if ((1 << (hisPlayerID-1) & myOneWayMask))
		{
			diplomacyButtons[i]->SetFirstState(true);
		}
		else
		{
			diplomacyButtons[i]->SetFirstState(false);
		}
	}
}
//----------------------------------------------------------------------------------//
//
void Menu_Diplomacy::setStateInfo (void)
{
	//
	// create members if not done already
	//
	screenRect.left		= IDEAL2REALX(data.screenRect.left);
	screenRect.right	= IDEAL2REALX(data.screenRect.right);
	screenRect.top		= IDEAL2REALY(data.screenRect.top);
	screenRect.bottom	= IDEAL2REALY(data.screenRect.bottom);
	
	background->InitStatic(data.background, this);
	
	staticTitle->InitStatic(data.staticTitle, this);
	staticName->InitStatic(data.staticName, this);
	staticRace->InitStatic(data.staticRace, this);
	staticAllies->InitStatic(data.staticAllies, this);
	staticMetalTitle->InitStatic(data.staticMetalTitle, this);
	staticGasTitle->InitStatic(data.staticGasTitle, this);
	staticCrewTitle->InitStatic(data.staticCrewTitle, this);

	// fixit - take these out eventually
	staticMetalTitle->SetVisible(false);
	staticGasTitle->SetVisible(false);
	staticCrewTitle->SetVisible(false);
	
	buttonOk->InitButton(data.buttonOk, this);
	buttonApply->InitButton(data.buttonApply, this);
	buttonReset->InitButton(data.buttonReset, this);
	buttonCancel->InitButton(data.buttonCancel, this);

	buttonOk->SetTransparent(true);
	buttonReset->SetTransparent(true);
	buttonCancel->SetTransparent(true);
	buttonApply->SetTransparent(true);

	staticCrew->InitStatic(data.staticCrew, this);
	staticMetal->InitStatic(data.staticMetal, this);
	staticGas->InitStatic(data.staticGas, this);

	setResources();
 
	// initialize the static controls for players names
	U32 i;
	for (i = 0; i < NUM_OTHERS; i++)
	{
		staticNames[i]->InitStatic(data.staticNames[i], this);
		staticNames[i]->SetVisible(false);
	
		staticRaces[i]->InitStatic(data.staticRaces[i], this);
		staticRaces[i]->SetVisible(false);

		diplomacyButtons[i]->InitDiplomacyButton(data.diplomacyButtons[i], this);
		diplomacyButtons[i]->SetControlID(ALLY_BUTTONS_BEGIN + i);
		diplomacyButtons[i]->SetVisible(false);

		buttonMetal[i]->InitButton(data.buttonMetal[i], this);
		buttonMetal[i]->SetControlID(METAL_BUTTONS_BEGIN + i);
		buttonMetal[i]->SetTextString(L"0");
		buttonMetal[i]->SetVisible(false);
		buttonMetal[i]->SetTransparent(true);

		buttonGas[i]->InitButton(data.buttonGas[i], this);
		buttonGas[i]->SetControlID(GAS_BUTTONS_BEGIN + i);
		buttonGas[i]->SetTextString(L"0");
		buttonGas[i]->SetVisible(false);
		buttonGas[i]->SetTransparent(true);

		buttonCrew[i]->InitButton(data.buttonCrew[i], this);
		buttonCrew[i]->SetControlID(CREW_BUTTONS_BEGIN + i);
		buttonCrew[i]->SetTextString(L"0");
		buttonCrew[i]->SetVisible(false);
		buttonCrew[i]->SetTransparent(true);

		// if the diplomacy settings are locked, then don't allow any ally changes
		if (DEFAULTS->GetDefaults()->bLockDiplomacy)
		{
			diplomacyButtons[i]->EnableDiplomacyButton(false);
		}
	}
	
	// fill the players names properly
	setPlayersNames();
	properPushStates();
	updateAllAllyButtons();

	if (childFrame)
	{
		childFrame->setStateInfo();
	}
	else
	{
		setFocus(buttonOk);
	}
}
//--------------------------------------------------------------------------//
//
void Menu_Diplomacy::init (void)
{
	data = 	*((GT_DIPLOMACYMENU*) GENDATA->GetArchetypeData("Menu_Diplomacy"));

	COMPTR<IDAComponent> pComp;

	GENDATA->CreateInstance(data.background.staticType, pComp);
	pComp->QueryInterface("IStatic", background);

	GENDATA->CreateInstance(data.staticTitle.staticType, pComp);
	pComp->QueryInterface("IStatic", staticTitle);

	GENDATA->CreateInstance(data.staticName.staticType, pComp);
	pComp->QueryInterface("IStatic", staticName);

	GENDATA->CreateInstance(data.staticRace.staticType, pComp);
	pComp->QueryInterface("IStatic", staticRace);

	GENDATA->CreateInstance(data.staticAllies.staticType, pComp);
	pComp->QueryInterface("IStatic", staticAllies);

	GENDATA->CreateInstance(data.staticMetalTitle.staticType, pComp);
	pComp->QueryInterface("IStatic", staticMetalTitle);

	GENDATA->CreateInstance(data.staticGasTitle.staticType, pComp);
	pComp->QueryInterface("IStatic", staticGasTitle);

	GENDATA->CreateInstance(data.staticCrewTitle.staticType, pComp);
	pComp->QueryInterface("IStatic", staticCrewTitle);

	GENDATA->CreateInstance(data.buttonOk.buttonType, pComp);
	pComp->QueryInterface("IButton2", buttonOk);

	GENDATA->CreateInstance(data.buttonReset.buttonType, pComp);
	pComp->QueryInterface("IButton2", buttonReset);

	GENDATA->CreateInstance(data.buttonCancel.buttonType, pComp);
	pComp->QueryInterface("IButton2", buttonCancel);

	GENDATA->CreateInstance(data.buttonApply.buttonType, pComp);
	pComp->QueryInterface("IButton2", buttonApply);
 
	GENDATA->CreateInstance(data.staticCrew.staticType, pComp);
	pComp->QueryInterface("IStatic", staticCrew);

	GENDATA->CreateInstance(data.staticMetal.staticType, pComp);
	pComp->QueryInterface("IStatic", staticMetal);

	GENDATA->CreateInstance(data.staticGas.staticType, pComp);
	pComp->QueryInterface("IStatic", staticGas);

	U32 i;
	for (i = 0; i < NUM_OTHERS; i++)
	{
		GENDATA->CreateInstance(data.staticNames[i].staticType, pComp);
		pComp->QueryInterface("IStatic", staticNames[i]);

		GENDATA->CreateInstance(data.staticRaces[i].staticType, pComp);
		pComp->QueryInterface("IStatic", staticRaces[i]);

		GENDATA->CreateInstance(data.buttonCrew[i].buttonType, pComp);
		pComp->QueryInterface("IButton2", buttonCrew[i]);

		GENDATA->CreateInstance(data.buttonMetal[i].buttonType, pComp);
		pComp->QueryInterface("IButton2", buttonMetal[i]);

		GENDATA->CreateInstance(data.buttonGas[i].buttonType, pComp);
		pComp->QueryInterface("IButton2", buttonGas[i]);

		GENDATA->CreateInstance(data.diplomacyButtons[i].buttonType, pComp);
		pComp->QueryInterface("IDiplomacyButton", diplomacyButtons[i]);
	}

	resPriority = RES_PRIORITY_HIGH;
	cursorID = IDC_CURSOR_ARROW;
	desiredOwnedFlags = RF_CURSOR;
	grabAllResources();
}
//--------------------------------------------------------------------------//
//
Frame * __stdcall CreateMenuDiplomacy (void)
{
	Menu_Diplomacy * menu = new Menu_Diplomacy(MGlobals::GetGameSettings());
	menu->createViewer("\\GT_DIPLOMACYMENU\\Menu_Diplomacy", "GT_DIPLOMACYMENU", IDS_VIEWDIPLOMACYMENU);
	menu->beginModalFocus();
	return menu;
}

//--------------------------------------------------------------------------//
//-----------------------------End Menu_Diplomacy.cpp-----------------------//
//--------------------------------------------------------------------------//
