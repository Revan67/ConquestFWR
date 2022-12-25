//--------------------------------------------------------------------------//
//                                                                          //
//                               Menu_final.cpp                             //
//                                                                          //
//               COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
   $Header: /Conquest/App/Src/Menu_final.cpp 109   9/13/01 10:01a Tmauer $
*/
//--------------------------------------------------------------------------//
// Multiplayer final setup screen
//--------------------------------------------------------------------------//
 
#include "pch.h"
#include <globals.h>

#include "Frame.h"
#include "IStatic.h"
#include "IListbox.h"
#include "IButton2.h"
#include <DMenu1.h>
#include "CQGame.h"
#include "NetBuffer.h"
#include "NetPacket.h"

#include <dplay.h>
#include <stdio.h>

#define RESYNC_PERIOD   15000     // milliseconds

void  __stdcall DoMenu_map (Frame * parent, const GT_MENU1 & data, ICQGame & cqgame, bool _bInternet);

using namespace CQGAMETYPES;
//--------------------------------------------------------------------------//
//
struct Menu_final : public DAComponent<Frame>
{
	//
	// data items
	//
	const GT_MENU1::FINAL & data;
	const GT_MENU1 & menu1;
	ICQGame & cqgame;

	COMPTR<IStatic> staticState, staticName, staticColor, staticRace, staticTeam, staticPing;
	COMPTR<IStatic> description;
	COMPTR<IStatic> staticAccept;
	COMPTR<IButton2> accept, start;
	COMPTR<IButton2> cancel;
	COMPTR<IStatic>  staticCountdown;

	bool bValidData;		// true if we are the host or we have received game info from host
	S32  syncWaitTime;		// milliseconds until we try to resync again

	Frame * realParent;
	Frame * slotsFrame;
	Frame * mshellFrame;
	Frame * mapFrame;

	bool bInternet;

	//
	// instance methods
	//

	Menu_final (Frame * _parent, const GT_MENU1 & _data, ICQGame & _cqgame, bool _bInternet) 
		: data(_data.final), menu1(_data), cqgame(_cqgame)
	{
		bInternet = _bInternet;
		realParent = _parent;
		initializeFrame(_parent);
		init();
	}


	~Menu_final (void);
    void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}


	virtual GENRESULT __stdcall Notify (U32 message, void *param)
	{
		switch (message)
		{
		case CQE_KILL_FOCUS:
			if (childFrame)
			{
				childFrame->Notify(CQE_KILL_FOCUS, 0);
			}
			break;

		case CQE_SET_FOCUS:
			if (childFrame)
			{
				childFrame->Notify(CQE_SET_FOCUS, 0);
			}
			break;

		default:
			break;
		} 

		return Frame::Notify(message, param);
	}

	/* Menu_final methods */

	virtual void setStateInfo (void);

	virtual void onButtonPressed (U32 buttonID)
	{
		switch (buttonID)
		{
		case IDS_START:
			if (cqgame.slot[cqgame.localSlot].state == READY && PLAYERID && checkReadyState())
				break;		// don't do anything during countdown
			// fall through intentional
		case IDS_ACCEPT:
			toggleAcceptState();
			break;

		case IDS_CANCEL:
			// if in countdown mode, don't allow player to leave session
			if (cqgame.slot[cqgame.localSlot].state == READY && PLAYERID && checkReadyState())
				toggleAcceptState();
			else
			{
				NETPACKET->StallPacketDelivery(true);
				if (CQMessageBox(IDS_HELP_WARNDROP, IDS_APP_NAMETM, MB_YESNO, realParent))
				{
					NETPACKET->StallPacketDelivery(false);
					endDialog(0);
				}
				else
				{
					NETPACKET->StallPacketDelivery(false);
				}
			}
			break;

		case IDS_MPLAYER_SETTINGS:
			parentFrame->PostMessage(CQE_BUTTON, (void*)buttonID);
			break;
		}
	}

	virtual bool onEscPressed (void)
	{
		onButtonPressed(IDS_CANCEL);
		return true;
	}

	virtual bool onTabPressed (void)
	{
		if (focusControl == NULL)
		{
			return false;
		}

		if (onGroupTabPressed())
		{
			// let the frame do its work
			return Frame::onTabPressed();
		}
		else
		{
			// switch focus to the next control group
			if (slotsFrame)
			{
				slotsFrame->onSetDefaultFocus(true);
				return true;
			}
		}

		return Frame::onTabPressed();
	}

	virtual bool onShiftTabPressed (void)
	{
		if (focusControl == NULL)
		{
			return false;
		}

		if (onGroupShiftTabPressed())
		{
			// let the frame do its work
			return Frame::onShiftTabPressed();
		}
		else
		{
			if (PLAYERID)
			{
				if (mshellFrame)
				{
					mshellFrame->setLastFocus();
					return true;
				}
			}
			else
			{
				if (mapFrame)
				{
					mapFrame->setLastFocus();
					return true;
				}
			}
		}
		return Frame::onShiftTabPressed();
	}

	virtual void onSetDefaultFocus (bool bFromPrevious)
	{
		if (focusControl == NULL)
		{
			if (HOSTID == PLAYERID)
			{
				if (start->GetEnableState())
				{
					setFocus(start);
				}
				else
				{
					setFocus(cancel);
				}
			}
			else
			{
				if (cqgame.bHostBusy == 0)
				{
					setFocus(accept);
				}
				else
				{
					setFocus(cancel);
				}
			}
		}
		else
		{
			Frame::onSetDefaultFocus(bFromPrevious);
		}
	}

	virtual void onUpdate (U32 dt)   // dt in milliseconds
	{
		// are we in the ready state?
		if (cqgame.slot[cqgame.localSlot].state == READY && PLAYERID && checkReadyState())
		{
			// start the carebear countdown
			S32 num = 10 - cqgame.startCountdown;
			if (num > 10)
				num = 10;
			if (num < 0)
				num = 0;
			U32 stringID = IDS_NUMBERS_0 + num;
			staticCountdown->SetTextID(stringID);
			staticCountdown->SetVisible(true);
		}
		else
		{
			// only do resync thing when countdown is not running (can throw off results)
			U32 latency;
			if (NETBUFFER->GetLatency(0, &latency) == SR_SUCCESS)
			{
				if (S32(syncWaitTime -= dt) <= 0)
				{
					syncWaitTime = RESYNC_PERIOD;
					NETBUFFER->SyncPlayer(0);		// get latency info for all players
				}
			}
			
			staticCountdown->SetVisible(false);
		}
	}

	DEFMETHOD_(BOOL32,OnAppClose) (void)
	{
		// if in count down mode, just act like a cancel
		if (PLAYERID != 0)
		{
			if (cqgame.slot[cqgame.localSlot].state == READY)
				toggleAcceptState();
			NETPACKET->StallPacketDelivery(true);

			if (CQFLAGS.bNoExitConfirm || CQMessageBox(IDS_CONFIRM_QUIT, IDS_CONFIRM, MB_OKCANCEL))
				PostQuitMessage(0);

			NETPACKET->StallPacketDelivery(false);

			return TRUE;	// deny request
		}

		return FALSE;		// allow app close by default
	}
	
	void init (void);

	void toggleAcceptState (void);
	
	void setAcceptState (void);
	
	bool checkReadyState (void);
	
	int countNumberTeams (void);	// how many different teams are there?

	int countRepeatComputerPlayers (void);

	int countPlayers (void);		// return number of players in the game

	int countHumanPlayers (void);	// return number of Human players in the game

	bool checkComputerAndHumanCooperative (void);

	bool checkCooperativePlayers (bool bMessageBox);

	bool checkForAllCooperative (void);
	
	static bool checkDataValid (const ICQGame & cqgame);

	bool testForCheating (void);	// return true if player doesn't like cheaters

	bool isAvailable (COLOR color)
	{
		U32 i=0;

		while (i < cqgame.activeSlots)
		{
			if (i != cqgame.localSlot && cqgame.slot[i].color == color && (cqgame.slot[i].state==ACTIVE||cqgame.slot[i].state==READY))
			{
				return false;
			}
			i++;
		}

		return true;
	}
};
//----------------------------------------------------------------------------------//
//
Menu_final::~Menu_final (void)
{
}
//----------------------------------------------------------------------------------//
//
bool Menu_final::checkDataValid (const ICQGame & cqgame)
{
	U32 i;
	bool result = (PLAYERID==0);

	if (HOSTID!=0 && PLAYERID!=0)
	{
		for (i = 0; i < cqgame.activeSlots; i++)
		{
			switch (cqgame.slot[i].state)
			{
			case READY:
			case ACTIVE:
				if (cqgame.slot[i].dpid == HOSTID)
					result = true;
				break;
			}
		}
	}
	
	return result;
}
//----------------------------------------------------------------------------------//
//
bool Menu_final::testForCheating (void)
{
	bool result = false;
	const U32 checkSum = GetMultiplayerVersion();
	U32 i;

	if (PLAYERID!=0)
	for (i = 0; i < cqgame.activeSlots; i++)
	{
		if (cqgame.slot[i].state == READY || cqgame.slot[i].state == ACTIVE)
		{
			if (cqgame.slot[i].type == HUMAN)
			{
				if (cqgame.slot[i].dpid != PLAYERID)
				{
					U32 otherSum = NETBUFFER->GetChecksumForPlayer(cqgame.slot[i].dpid);
					if (checkSum != otherSum)
					{
						wchar_t szhelp[MAX_PATH+4];
						if (otherSum==0)
						{
							swprintf(szhelp, _localLoadStringW(IDS_WARN_NETWORK_DIFFICULTY), cqgame.szPlayerNames[i]);
							CQMessageBox(szhelp, IDS_NETWORK_ERROR, MB_OK);
							result = true;
						}
						else
						{
							swprintf(szhelp, _localLoadStringW(IDS_WARN_CHEATING), cqgame.szPlayerNames[i]);

							if (CQMessageBox(szhelp, IDS_CONFIRM, MB_YESNO) == 0)
							{
								result = true;
								break;
							}
						}
					}
				}
			}
		}
	}

	return result;
}
//----------------------------------------------------------------------------------//
//
void Menu_final::setStateInfo (void)
{
	//screenRect = data.screenRect;
	screenRect.left = IDEAL2REALX(data.screenRect.left);
	screenRect.right = IDEAL2REALX(data.screenRect.right);
	screenRect.top = IDEAL2REALY(data.screenRect.top);
	screenRect.bottom = IDEAL2REALY(data.screenRect.bottom);

	//
	// initialize in draw-order
	//
	staticState->InitStatic(data.staticState, this);
	staticName->InitStatic(data.staticName, this);
	staticColor->InitStatic(data.staticColor, this);
	staticRace->InitStatic(data.staticRace, this);
	staticTeam->InitStatic(data.staticTeam, this);
	staticPing->InitStatic(data.staticPing, this);
	staticCountdown->InitStatic(data.staticCountdown, this, true);

	description->InitStatic(data.description, this);
	staticAccept->InitStatic(data.staticAccept, this);

	accept->InitButton(data.accept, this);
	start->InitButton(data.start, this);
	cancel->InitButton(data.cancel, this);

	if (childFrame == 0)
	{
		DoMenu_map (this, menu1, cqgame, bInternet);
	}
	else if (childFrame)
	{
		childFrame->setStateInfo();
	}

	// get a pointer to menu_slots, this is your childFrame's (menu_map) child
	// get a pointer to menu_mshell, this is your parent frame
	// get a pointer to menu_map, this is your child frame
	slotsFrame = childFrame->childFrame;
	mshellFrame = parentFrame;
	mapFrame = childFrame;

	staticCountdown->SetVisible(false);

	bValidData = (bValidData || (HOSTID==PLAYERID) || checkDataValid(cqgame));

	setAcceptState();
	accept->SetControlID(IDS_ACCEPT);
	staticAccept->SetBuddyControl(accept);

	if (HOSTID == PLAYERID)
	{
		start->SetVisible(true);
		accept->SetVisible(false);
		staticAccept->SetVisible(false);
	}
	else
	{
		start->SetVisible(false);
		accept->SetVisible(true);
		staticAccept->SetVisible(true);
	}

	if (bValidData)
	{
		const bool bReadyState = checkReadyState();
		const bool bMultiplayer = PLAYERID!=0;
		bool bEnoughPlayers = false;

		if (bMultiplayer)
		{
			// a multiplayer game, we need at least two humans
			bEnoughPlayers = (countHumanPlayers() >= 2);
		}
		else
		{
			// need two players
			bEnoughPlayers = (countPlayers() >= 2);
		}
		S32 maxPlayers;
		bool bNotTooManyPlayers = true;
		if(mapFrame)
		{
			mapFrame->Notify(CQE_GET_MAX_PLAYERS,&maxPlayers);
			bNotTooManyPlayers = (countPlayers() <= maxPlayers);
		}
		if (DEFAULTS->GetDefaults()->bNoWinningConditions || (bReadyState && bEnoughPlayers && bNotTooManyPlayers))
		{
			start->EnableButton(true);		// enable button if everyone else is ready
			if (HOSTID == PLAYERID)
			{
				description->SetText(_localLoadStringW(IDS_HELP_PRESSSTART));
			}
			else
			{
				description->SetText(_localLoadStringW(IDS_HELP_WAITSTART));
			}
		}
		else
		{
			start->EnableButton(false);		// enable button if everyone else is ready
			
			if (HOSTID == PLAYERID)
			{
				if(!bNotTooManyPlayers)
				{
					description->SetText(_localLoadStringW(IDS_HELP_TOOMANYPLAYERS));
				}
				else
				{
					description->SetText(_localLoadStringW(IDS_HELP_HOSTWAITSTART));
				}
			}
			else if (cqgame.bHostBusy)
			{
				description->SetText(_localLoadStringW(IDS_HELP_HOSTBUSY));
			}
			else if (cqgame.slot[cqgame.localSlot].state == READY)
			{
				description->SetText(_localLoadStringW(IDS_HELP_WAITSTART));
			}
			else if(!bNotTooManyPlayers)
			{
				description->SetText(_localLoadStringW(IDS_HELP_TOOMANYPLAYERS));
			}
			else
			{
				description->SetText(_localLoadStringW(IDS_HELP_SIGNALREADY));
			}
		}
	}
	else  // no valid data yet
	{
		description->SetText(_localLoadStringW(IDS_HELP_CLIENTWAITDATA));
	}


	// set the group behavior so we do the right tab thing
	setGroupBehavior();
}
//--------------------------------------------------------------------------//
//
void Menu_final::init (void)
{
	//
	// create members
	//
	COMPTR<IDAComponent> pComp;
	
	GENDATA->CreateInstance(data.staticState.staticType, pComp);
	pComp->QueryInterface("IStatic", staticState);

	GENDATA->CreateInstance(data.staticName.staticType, pComp);
	pComp->QueryInterface("IStatic", staticName);

	GENDATA->CreateInstance(data.staticColor.staticType, pComp);
	pComp->QueryInterface("IStatic", staticColor);

	GENDATA->CreateInstance(data.staticRace.staticType, pComp);
	pComp->QueryInterface("IStatic", staticRace);

	GENDATA->CreateInstance(data.staticTeam.staticType, pComp);
	pComp->QueryInterface("IStatic", staticTeam);

	GENDATA->CreateInstance(data.staticPing.staticType, pComp);
	pComp->QueryInterface("IStatic", staticPing);

	GENDATA->CreateInstance(data.description.staticType, pComp);
	pComp->QueryInterface("IStatic", description);
	
	GENDATA->CreateInstance(data.staticAccept.staticType, pComp);
	pComp->QueryInterface("IStatic", staticAccept);

	GENDATA->CreateInstance(data.accept.buttonType, pComp);
	pComp->QueryInterface("IButton2", accept);
	
	GENDATA->CreateInstance(data.start.buttonType, pComp);
	pComp->QueryInterface("IButton2", start);
	
	GENDATA->CreateInstance(data.cancel.buttonType, pComp);
	pComp->QueryInterface("IButton2", cancel);

	GENDATA->CreateInstance(data.staticCountdown.staticType, pComp);
	pComp->QueryInterface("IStatic", staticCountdown);

	hookSystemEvent();

	setStateInfo();
}
//--------------------------------------------------------------------------//
//
void Menu_final::toggleAcceptState (void)
{
	const SLOT & slot = cqgame.slot[cqgame.localSlot];
	const bool bHost = (HOSTID == PLAYERID);
	const bool bNoWinningConditions = DEFAULTS->GetDefaults()->bNoWinningConditions;
	const bool bCheatsEnabled = DEFAULTS->GetDefaults()->bCheatsEnabled;
	const bool bMultiplayer = PLAYERID!=0;

	if (slot.state == ACTIVE)
	{
		int playerCount = 0;
		wchar_t szhelp[256];

		// if cheats are not enabled, see if any humans are co-oping with computers
		if (bCheatsEnabled == false)
		{
			if (checkComputerAndHumanCooperative())
			{
				CQMessageBox(IDS_HELP_COOPCOMPUTER, IDS_ERROR, MB_OK);
				return;
			}
		}

		int teamCount = countNumberTeams();

		if (bMultiplayer)
		{
			playerCount = countHumanPlayers();
			swprintf(szhelp, L"You are the only HUMAN player, continue?");
		}
		else
		{
			playerCount = countPlayers();
			swprintf(szhelp, L"You are the only player, continue?");
		}

		// if there are repeat computer AI's the signal the player
		if (bNoWinningConditions == false && bHost && countRepeatComputerPlayers() > 0)
		{
			CQMessageBox(IDS_HELP_REPEATCOMPUTER, IDS_ERROR, MB_OK);
			return;
		}
		
		// if all the players are of the same color (playerID) then inform the player
		if (bNoWinningConditions == false && bHost && checkForAllCooperative() == true)
		{
			CQMessageBox(IDS_HELP_ALLCOOP, IDS_ERROR, MB_OK);
			return;
		}

		if (testForCheating())
		{
			return;
		}

		if (bNoWinningConditions)
		{
			if (playerCount != 1 || CQMessageBox(szhelp, IDS_CONFIRM, MB_OKCANCEL))
			{
				accept->SetPushState(true);
				cqgame.SetState(cqgame.localSlot, READY, true);
			}
		}
		else
		{
			// make sure we have at least 2 teams
			if (teamCount < 2)
			{
				CQMessageBox(IDS_HELP_ONETEAM, IDS_ERROR, MB_OK);
				return;
			}

			if (PLAYERID!=0)
			{
				if (playerCount == 1)
				{
					CQMessageBox(IDS_HELP_SOLOHUMAN, IDS_ERROR, MB_OK);
					return;
				}
				else if (bHost && checkCooperativePlayers(true) == false)
				{
					return;
				}
				else 
				{
					accept->SetPushState(true);
					cqgame.SetState(cqgame.localSlot, READY, true);
				}
			}
			else
			{
				if (playerCount == 1)
				{
					CQMessageBox(IDS_HELP_SOLOGAME, IDS_ERROR, MB_OK);
				}
				else 
				{
					accept->SetPushState(true);
					cqgame.SetState(cqgame.localSlot, READY, true);
				}
			}
		}
	}
	else
	if (slot.state == READY)
	{
		accept->SetPushState(false);
		cqgame.SetState(cqgame.localSlot, ACTIVE, true);
	}
}
//--------------------------------------------------------------------------//
//
void Menu_final::setAcceptState (void)
{
	SLOT & slot = cqgame.slot[cqgame.localSlot];

	if (slot.state == ACTIVE)
		accept->SetPushState(false);
	else
	if (slot.state == READY)
		accept->SetPushState(true);

	S32 maxPlayers;
	bool bNotTooManyPlayers = true;
	if(mapFrame)
	{
		mapFrame->Notify(CQE_GET_MAX_PLAYERS,&maxPlayers);
		bNotTooManyPlayers = (countPlayers() <= maxPlayers);
	}
	accept->EnableButton(!cqgame.bHostBusy && bNotTooManyPlayers);
}
//--------------------------------------------------------------------------//
// return true if everyone else is ready
//
bool Menu_final::checkReadyState (void)
{
	U32 i;
	bool result = true;

	for (i = 0; i < cqgame.activeSlots; i++)
		if (i != cqgame.localSlot && cqgame.slot[i].state == ACTIVE)
		{
			result = false;
			break;
		}

	return result;
}
//--------------------------------------------------------------------------//
//
int Menu_final::countNumberTeams (void)
{
	U32 i;
	U32 teamsMask = 0;
	U32 nTeams = 0;

	for (i = 0; i < cqgame.activeSlots; i++)
	{
		if (cqgame.slot[i].state == ACTIVE || cqgame.slot[i].state == READY)
		{
			if (cqgame.slot[i].team != 0)
			{
				teamsMask |= 1 << cqgame.slot[i].team;
			}
			else
			{
				nTeams++;
			}
		}
	}

	// how many bits have been set?
	for (i = 1; i < 5; i++)
	{
		if (teamsMask & (1 << i))
		{
			nTeams++;
		}
	}

	return nTeams;
}
//--------------------------------------------------------------------------//
//
// return true if all coop players have the same race
bool Menu_final::checkCooperativePlayers (bool bMessageBox)
{
	U32 i;
	U32 j;
	
	for (i = 0; i < cqgame.activeSlots; i++)
	{
		if (cqgame.slot[i].state == ACTIVE || cqgame.slot[i].state == READY)
		{
			for (j = 0; j < cqgame.activeSlots; j++)
			{
				if (cqgame.slot[j].state == ACTIVE || cqgame.slot[j].state == READY)
				{
					if (i != j)
					{
						if (cqgame.slot[i].color == cqgame.slot[j].color)
						{
							// two different players have the same color (ie. cooperative), return false if
							// their races are different
							if (cqgame.slot[i].race != cqgame.slot[j].race)
							{
								if (bMessageBox)
								{
									CQMessageBox(IDS_HELP_COOPCHECK, IDS_ERROR, MB_OK);
								}
								return false;
							}

							// also return false if their teams are different
							if (cqgame.slot[i].team != cqgame.slot[j].team)
							{
								if (bMessageBox)
								{
									CQMessageBox(IDS_HELP_COOPTEAM, IDS_ERROR, MB_OK);
								}
								return false;
							}
						}
					}
				}
			}
		}
	}

	return true;
}
//--------------------------------------------------------------------------//
//
bool Menu_final::checkForAllCooperative (void)
{
	// if all the players are sharing the same color - then return true
	U32 i;
	CQGAMETYPES::COLOR color = CQGAMETYPES::UNDEFINEDCOLOR; 

	// find the first color
	for (i = 0; i < cqgame.activeSlots; i++)
	{
		if (cqgame.slot[i].state == ACTIVE || cqgame.slot[i].state == READY)
		{
			color = cqgame.slot[i].color;
			break;
		}
	}

	// now go through all the players and if you find one that doesn't match the given color return false
	for (i = 0; i < cqgame.activeSlots; i++)
	{
		if (cqgame.slot[i].state == ACTIVE || cqgame.slot[i].state == READY)
		{
			if (cqgame.slot[i].color != color)
			{
				return false;
			}
		}
	}

	return true;
}
//--------------------------------------------------------------------------//
//
bool Menu_final::checkComputerAndHumanCooperative (void)
{
	// find a comptuer and human cooperative match
	U32 i;
	U32 j;
	
	for (i = 0; i < cqgame.activeSlots; i++)
	{
		if (cqgame.slot[i].state == ACTIVE || cqgame.slot[i].state == READY)
		{
			for (j = 0; j < cqgame.activeSlots; j++)
			{
				if (cqgame.slot[j].state == ACTIVE || cqgame.slot[j].state == READY)
				{
					if (i != j)
					{
						if (cqgame.slot[i].type == CQGAMETYPES::HUMAN && cqgame.slot[j].type == CQGAMETYPES::COMPUTER)
						{
							if (cqgame.slot[i].color == cqgame.slot[j].color)
							{
								// we've found a human matched with computer
								return true;
							}
						}
					}
				}
			}
		}
	}

	return false;
}
//--------------------------------------------------------------------------//
//
int Menu_final::countRepeatComputerPlayers (void)
{
	// how many computer players have the same playerID?
	U32 i;
	U32 j;
	int result = 0;
	
	for (i = 0; i < cqgame.activeSlots; i++)
	{
		if (cqgame.slot[i].state == ACTIVE || cqgame.slot[i].state == READY)
		{
			for (j = 0; j < cqgame.activeSlots; j++)
			{
				if (cqgame.slot[j].state == ACTIVE || cqgame.slot[j].state == READY)
				{
					if (i != j)
					{
						if (cqgame.slot[i].type == CQGAMETYPES::COMPUTER && cqgame.slot[j].type == CQGAMETYPES::COMPUTER)
						{
							if (cqgame.slot[i].color == cqgame.slot[j].color)
							{
								result++;
							}
						}
					}
				}
			}
		}
	}

	return result;
}
//--------------------------------------------------------------------------//
//
int Menu_final::countPlayers (void)
{
	int result = 0;
	U32 i;

	for (i = 0; i < cqgame.activeSlots; i++)
	{
		if (cqgame.slot[i].state == ACTIVE || cqgame.slot[i].state == READY)
		{
			result++;
		}
	}

	return result;
}
//--------------------------------------------------------------------------//
//
int Menu_final::countHumanPlayers (void)
{
	int result = 0;
	U32 i;

	for (i = 0; i < cqgame.activeSlots; i++)
	{
		if (cqgame.slot[i].state == ACTIVE || cqgame.slot[i].state == READY)
		{
			if (cqgame.slot[i].type == HUMAN)
			{
				result++;
			}
		}
	}

	return result;
}
//--------------------------------------------------------------------------//
//
void  __stdcall DoMenu_final (Frame * parent, const GT_MENU1 & data, ICQGame & cqgame, bool _bInternet)
{
	new Menu_final(parent, data, cqgame, _bInternet);
}

//--------------------------------------------------------------------------//
//----------------------------End Menu_final.cpp------------------------------//
//--------------------------------------------------------------------------//
