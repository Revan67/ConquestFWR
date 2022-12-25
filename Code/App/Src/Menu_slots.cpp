//--------------------------------------------------------------------------//
//                                                                          //
//                             Menu_slots.cpp                               //
//                                                                          //
//               COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
   $Header: /Conquest/App/Src/Menu_slots.cpp 59    10/18/02 2:36p Tmauer $
*/
//--------------------------------------------------------------------------//
// Multiplayer load map
//--------------------------------------------------------------------------//
 
#include "pch.h"
#include <globals.h>
#include <wchar.h>

#include "Frame.h"
#include "IStatic.h"
#include "IListbox.h"
#include "IButton2.h"
#include <DMenu1.h>
#include "CQGame.h"
#include "Mission.h"
#include "NetBuffer.h"

#include <dplay.h>

#define DROP_SLOT_BEGIN		11200
#define DROP_RACE_BEGIN		11300
#define DROP_PLAYER_BEGIN	11400
#define DROP_TEAM_BEGIN     11500

#define M_MAX_STRING	32

using namespace CQGAMETYPES;
//--------------------------------------------------------------------------//
//
struct Menu_slots : public DAComponent<Frame>
{
	//
	// data items
	//
	const GT_MENU1::SLOTS & data;
	const GT_MENU1 & menu1;
	ICQGame & cqgame;

	COMPTR<IDropdown> dropSlots[MAX_PLAYERS];
	COMPTR<IDropdown> dropRaces[MAX_PLAYERS];
	COMPTR<IDropdown> dropPlayers[MAX_PLAYERS];
	COMPTR<IDropdown> dropTeams[MAX_PLAYERS];
	COMPTR<IStatic> staticNames[MAX_PLAYERS];
	COMPTR<IStatic> staticPings[MAX_PLAYERS];

	M_STRINGW terranComputerNames[MAX_PLAYERS];
	M_STRINGW mantisComputerNames[MAX_PLAYERS];
	M_STRINGW solarianComputerNames[MAX_PLAYERS];

	bool bAlreadySetFocus;

	Frame * mapFrame;
	Frame * finalFrame;
	int thisSlotID;

	//
	// instance methods
	//

	Menu_slots (Frame * _parent, const GT_MENU1 & _data, ICQGame & _cqgame) 
		: data(_data.slots), menu1(_data), cqgame(_cqgame)
	{
		thisSlotID = -1;
		initializeFrame(_parent);
		init();
	}

	~Menu_slots (void);

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

	/* Menu_slots methods */

	virtual void setStateInfo (void);

	virtual void onListSelection (S32 listID);		// user has selected a list item

	virtual void onListSelectionRace (S32 listID);

	virtual void onListSelectionTeam (S32 listID);		

	virtual void onListSelectionPlayer (S32 listID);		

	virtual void onListSelectionState (S32 listID);		

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
			// switch focus to the next control group (menu_final)
			if (mapFrame)
			{
				mapFrame->onSetDefaultFocus(true);
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
			if (finalFrame)
			{
				finalFrame->setLastFocus();
				return true;
			}
		}
		return Frame::onShiftTabPressed();
	}

	virtual void onSetDefaultFocus (bool bFromPrevious)
	{
		// if we are a client and we have accepted (and therefore everything in this menu is disabled)
		// then set the default focus on the next menu
		const SLOT & slot = cqgame.slot[cqgame.localSlot];
		
		if (HOSTID != PLAYERID && slot.state == CQGAMETYPES::READY)
		{
			if (bFromPrevious)
			{
				mapFrame->onSetDefaultFocus(true);
			}
			else
			{
				finalFrame->onSetDefaultFocus(false);
			}
		}
		else if (focusControl == NULL)
		{
				setFocus(dropPlayers[thisSlotID]);
		}
		else
		{
			Frame::onSetDefaultFocus(bFromPrevious);
		}
	}

	void flushChanges()
	{
		COMPTR<IDocument> pDatabase, doc;

		GENDATA->GetDataFile(pDatabase);

		if (pDatabase->GetChildDocument("\\GT_MENU1\\Menu1", doc) != GR_OK)
			CQBOMB0("Could not create document");

		DWORD dwWritten;
		doc->SetFilePointer(0,0);
		doc->WriteFile(0, &menu1, sizeof(GT_MENU1), &dwWritten, 0);

		doc->UpdateAllClients();
	}

	void init (void);
	
	void addComputerPlayer (S32 row, CQGAMETYPES::COMP_CHALANGE compChalange);
	
	bool bootPlayer (S32 row);		// return true if successful

	bool isAvailable (COLOR color)
	{
		U32 i=0;

		while (i < cqgame.activeSlots)
		{
			if (cqgame.slot[i].color == color && (cqgame.slot[i].state==ACTIVE||cqgame.slot[i].state==READY))
			{
				return false;
			}
			i++;
		}

		return true;
	}

	void enableDropColor (IDropdown* drop, COLOR color, U32 idString, COLOR caretColor)
	{
		S32 index = drop->AddString(_localLoadStringW(idString));
		drop->SetDataValue(index, color);
		drop->SetColorValue(index, COLORTABLE[color]);
		if (cqgame.slot[cqgame.localSlot].color == color)
		{
			drop->SetCurrentSelection(index);
		}
		if (caretColor == color)
		{
			drop->SetCaretPosition(index);
		}
	}

	void setAcceptStatus (BOOL32 bAccept)
	{
		// cannot change the race, team, player drop down boxes while accepting
		int index = cqgame.localSlot;;

		dropRaces[index]->EnableDropdown(bAccept != TRUE);
		dropPlayers[index]->EnableDropdown(bAccept != TRUE);
		dropTeams[index]->EnableDropdown(bAccept != TRUE);
	}

	static bool checkDataValid (const ICQGame & cqgame);
};
//----------------------------------------------------------------------------------//
//
Menu_slots::~Menu_slots (void)
{
}
//----------------------------------------------------------------------------------//
//
bool Menu_slots::checkDataValid (const ICQGame & cqgame)
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
void Menu_slots::setStateInfo (void)
{
	U32 i;

	//screenRect = data.screenRect;
	screenRect.left = IDEAL2REALX(data.screenRect.left);
	screenRect.right = IDEAL2REALX(data.screenRect.right);
	screenRect.top = IDEAL2REALY(data.screenRect.top);
	screenRect.bottom = IDEAL2REALY(data.screenRect.bottom);

	const bool bDataValid = checkDataValid(cqgame);

	//
	// initialize in draw-order
	//
	for (i=0; i < MAX_PLAYERS; i++)
	{
		dropSlots[i]->InitDropdown(data.dropSlots[i], this);
		dropSlots[i]->SetControlID(i + DROP_SLOT_BEGIN);

		dropPlayers[i]->InitDropdown(data.dropPlayers[i], this);
		dropPlayers[i]->SetControlID(i + DROP_PLAYER_BEGIN);

		dropRaces[i]->InitDropdown(data.dropRaces[i], this);
		dropRaces[i]->SetControlID(i + DROP_RACE_BEGIN);

		dropTeams[i]->InitDropdown(data.dropTeams[i], this);
		dropTeams[i]->SetControlID(i + DROP_TEAM_BEGIN);

		staticNames[i]->InitStatic(data.staticNames[i], this);
		staticPings[i]->InitStatic(data.staticPings[i], this);

		terranComputerNames[i] = data.terranComputerNames[i];
		mantisComputerNames[i] = data.mantisComputerNames[i];
		solarianComputerNames[i] = data.solarianComputerNames[i];
	}

	// initialize the names and colors of the players
	wchar_t szName[PLAYERNAMESIZE];
	wchar_t szPing[PLAYERNAMESIZE];
	for (i = 0; U32(i) < cqgame.activeSlots; i++)
	{
		szPing[0] = 0;
		if (bDataValid==0 || cqgame.slot[i].state == CLOSED || cqgame.slot[i].state == OPEN)
		{
			szName[0] = 0;
		}
		else if (cqgame.slot[i].type == COMPUTER)
		{
			if (cqgame.slot[i].race == TERRAN)
			{
				wcscpy(szName, terranComputerNames[i]);
			}
			else if (cqgame.slot[i].race == MANTIS)
			{
				wcscpy(szName, mantisComputerNames[i]);
			}
			else
			{
				wcscpy(szName, solarianComputerNames[i]);
			}

			wcscpy(cqgame.szPlayerNames[i], szName);
		}
		else
		{
			memcpy(szName, cqgame.szPlayerNames[i], PLAYERNAMESIZE * sizeof(wchar_t));

			// what's the latency?
			U32 latency = 0;
			if (cqgame.localSlot != i)
			{
				NETBUFFER->GetLatency(cqgame.slot[i].dpid, &latency);
				if (latency)
					swprintf(szPing, L"%3d", latency);
				else
					wcscpy(szPing, L" ?");
			}
		}

		staticNames[i]->SetTextColor(COLORTABLE[cqgame.slot[i].color]);
		staticNames[i]->SetText(szName);
		
		staticPings[i]->SetTextColor(COLORTABLE[cqgame.slot[i].color]);
		staticPings[i]->SetText(szPing);
	}

	//
	// initialize the slot dropdowns
	//
	for (i=0; U32(i) < cqgame.activeSlots; i++)
	{
		S32 index = dropSlots[i]->GetCaretPosition();
		dropSlots[i]->ResetContent();
		dropSlots[i]->AddString(_localLoadStringW(IDS_SLOT_OPEN));
		dropSlots[i]->AddString(_localLoadStringW(IDS_SLOT_CLOSED));
		dropSlots[i]->AddString(_localLoadStringW(IDS_SLOT_AI_EASY));
		dropSlots[i]->AddString(_localLoadStringW(IDS_SLOT_AI_AVERAGE));
		dropSlots[i]->AddString(_localLoadStringW(IDS_SLOT_AI_HARD));
		dropSlots[i]->AddString(_localLoadStringW(IDS_SLOT_AI_IMPOSIBLE));
		dropSlots[i]->AddString(_localLoadStringW(IDS_SLOT_AI_NIGHTMARE));
	
		STATE state = (bDataValid) ? cqgame.slot[i].state : CLOSED;
		switch (state)
		{
		case CLOSED:
			dropSlots[i]->SetCurrentSelection(1);
			break;
		case OPEN:
			dropSlots[i]->SetCurrentSelection(0);
			break;

		case ACTIVE:
		case READY:
			if (cqgame.slot[i].type == HUMAN)
			{
				dropSlots[i]->AddString(_localLoadStringW((cqgame.slot[i].state==READY)?IDS_SLOT_READY:IDS_SLOT_ACTIVE));
				dropSlots[i]->SetCurrentSelection(7);
				if (U32(i) == cqgame.localSlot)
					dropSlots[i]->EnableDropdown(false);
			}
			else
			{
				if(cqgame.slot[i].compChalange == EASY_CH)
					dropSlots[i]->SetCurrentSelection(2);
				else if(cqgame.slot[i].compChalange == AVERAGE_CH)
					dropSlots[i]->SetCurrentSelection(3);
				else if(cqgame.slot[i].compChalange == HARD_CH)
					dropSlots[i]->SetCurrentSelection(4);
				else if(cqgame.slot[i].compChalange == IMPOSIBLE_CH)
					dropSlots[i]->SetCurrentSelection(5);
				else
					dropSlots[i]->SetCurrentSelection(6);
			}
			break;
		}
		if (index >= 0)
			dropSlots[i]->SetCaretPosition(index);
		dropSlots[i]->EnsureVisible(0);
	}


	// initialize the race select dropdowns
	for (i = 0; i < MAX_PLAYERS; i++)
	{
		S32 index;
		S32 sel;

		dropRaces[i]->ResetContent();
		index = dropRaces[i]->AddString(_localLoadStringW(IDS_RACESELECT_HUMAN));
		dropRaces[i]->SetDataValue(index, TERRAN);

		// the demo build does not include the mantis and solarian races
#ifndef _DEMO_
		index = dropRaces[i]->AddString(_localLoadStringW(IDS_RACESELECT_MANTIS));
		dropRaces[i]->SetDataValue(index, MANTIS);
		index = dropRaces[i]->AddString(_localLoadStringW(IDS_RACESELECT_SOLARIAN));
		dropRaces[i]->SetDataValue(index, SOLARIAN);
		index = dropRaces[i]->AddString(_localLoadStringW(IDS_RACESELECT_VYRIUM));
		dropRaces[i]->SetDataValue(index, VYRIUM);
#endif	

		// set the proper selection
		switch (cqgame.slot[i].race)
		{
		case MANTIS:
			sel = 1;
			break;

		case SOLARIAN:
			sel = 2;
			break;

		case VYRIUM:
			sel = 3;
			break;

		default:
			// includes terran class
			sel = 0;
			break;
		}

		// make extra sure that everyone is a terran
#ifdef _DEMO_
		sel = 0;
#endif

		dropRaces[i]->SetCurrentSelection(sel);
	}


	// initialize the plyer select dropdowns
	for (i = 0; i < MAX_PLAYERS; i++)
	{
		//
		// reset the color dropdown with all available colors
		//
		COLOR caretColor;
		S32 index = dropPlayers[i]->GetCaretPosition();		// returns index
		if (index >= 0)
		{
			caretColor = static_cast<COLOR>(dropPlayers[i]->GetDataValue(index));
		}
		else
		{
			caretColor = cqgame.slot[cqgame.localSlot].color;
		}

		dropPlayers[i]->ResetContent();


		enableDropColor(dropPlayers[i], YELLOW, IDS_PLAYERSELECT_1, caretColor);
		enableDropColor(dropPlayers[i], RED,	IDS_PLAYERSELECT_2, caretColor);
		enableDropColor(dropPlayers[i], BLUE,	IDS_PLAYERSELECT_3, caretColor);
		enableDropColor(dropPlayers[i], PINK,	IDS_PLAYERSELECT_4, caretColor);
		enableDropColor(dropPlayers[i], GREEN,	IDS_PLAYERSELECT_5, caretColor);
		enableDropColor(dropPlayers[i], ORANGE,	IDS_PLAYERSELECT_6, caretColor);
		enableDropColor(dropPlayers[i], PURPLE, IDS_PLAYERSELECT_7, caretColor);
		enableDropColor(dropPlayers[i], AQUA,	IDS_PLAYERSELECT_8, caretColor);

		if (cqgame.slot[i].color == UNDEFINEDCOLOR)
		{
			dropPlayers[i]->SetCurrentSelection(0);
			dropPlayers[i]->SetSelectionColor(COLORTABLE[UNDEFINEDCOLOR]);
		}
		else
		{
			dropPlayers[i]->SetCurrentSelection(cqgame.slot[i].color - YELLOW);
			dropPlayers[i]->SetSelectionColor(COLORTABLE[cqgame.slot[i].color]);
		}
	}

	// fill the team dropdowns with the right strings
	for (i = 0; i < MAX_PLAYERS; i++)
	{
		dropTeams[i]->ResetContent();
		dropTeams[i]->AddString(_localLoadStringW(IDS_TEAMSELECT_NONE));
		dropTeams[i]->AddString(_localLoadStringW(IDS_TEAMSELECT_1));
		dropTeams[i]->AddString(_localLoadStringW(IDS_TEAMSELECT_2));
		dropTeams[i]->AddString(_localLoadStringW(IDS_TEAMSELECT_3));
		dropTeams[i]->AddString(_localLoadStringW(IDS_TEAMSELECT_4));

		// make the proper selection
		dropTeams[i]->SetCurrentSelection(cqgame.slot[i].team);
	}


	// disable the proper dropdown boxes
	const U32 row = cqgame.localSlot;

	for (i = 0; i < MAX_PLAYERS; i++)
	{
		bool bEnable = bDataValid;
		bool bInvisible = (bDataValid == 0 || cqgame.slot[i].state == OPEN || cqgame.slot[i].state == CLOSED);

		if (i >= cqgame.activeSlots)
		{
			bEnable = bInvisible = true;
			dropSlots[i]->EnableDropdown(false);
		}

		if (HOSTID != PLAYERID || cqgame.bHostBusy==0)
		{
			// the drop slots are always disabled if we are not the host
			dropSlots[i]->EnableDropdown(false);

			// only keep dropboxes enabled if it corresponds to the current player and we are not in the ready state
			if (i != row || cqgame.slot[i].state == READY)
			{
				bEnable = false;
			}
		}
		else
		{
			// we are the host and keep all the dropboxes associated to a computer player enabled
			if (i != row)
			{
				if (i < cqgame.activeSlots)
					dropSlots[i]->EnableDropdown(true);
//				dropSlots[i]->SetVisible(true);


				if ((cqgame.slot[i].state == ACTIVE || cqgame.slot[i].state == READY) && cqgame.slot[i].type == COMPUTER)
				{
					bEnable = true;
				}
				else
				{
					bEnable = false;
				}
			}
			else
			{
				dropSlots[i]->EnableDropdown(false);
//				dropSlots[i]->SetVisible(false);
			}
		}

		dropTeams[i]->EnableDropdown(bEnable);
		dropTeams[i]->SetVisible(!bInvisible);
	
		dropPlayers[i]->EnableDropdown(bEnable);
		dropPlayers[i]->SetVisible(!bInvisible);

		dropRaces[i]->EnableDropdown(bEnable);
		dropRaces[i]->SetVisible(!bInvisible);

		if (bEnable && thisSlotID == -1)
		{
			thisSlotID = i;
		}
	}

	// after a slotID is properly assisgned to us the set focus to the appropriate control
	if (thisSlotID != -1 && bAlreadySetFocus == false)
	{
		bAlreadySetFocus = true;
		setFocus(dropPlayers[thisSlotID]);
	}

	if (childFrame)
	{
		childFrame->setStateInfo();
	}

	// need a pointer to the map menu, this is the parent frame
	// get a pointer to menu_final, this is your parent's parent frame
	mapFrame = parentFrame;
	finalFrame = parentFrame->parentFrame;

	// set the group behavior so we do the right tab thing
	setGroupBehavior();
}
//--------------------------------------------------------------------------//
//
void Menu_slots::init (void)
{
	//
	// create members
	//
	COMPTR<IDAComponent> pComp;
	int i;
	for (i=0; i < MAX_PLAYERS; i++)
	{
		GENDATA->CreateInstance(data.dropSlots[i].dropdownType, pComp);
		pComp->QueryInterface("IDropdown", dropSlots[i]);

		GENDATA->CreateInstance(data.dropRaces[i].dropdownType, pComp);
		pComp->QueryInterface("IDropdown", dropRaces[i]);

		GENDATA->CreateInstance(data.dropPlayers[i].dropdownType, pComp);
		pComp->QueryInterface("IDropdown", dropPlayers[i]);

		GENDATA->CreateInstance(data.dropTeams[i].dropdownType, pComp);
		pComp->QueryInterface("IDropdown", dropTeams[i]);

		GENDATA->CreateInstance(data.staticNames[i].staticType, pComp);
		pComp->QueryInterface("IStatic", staticNames[i]);

		GENDATA->CreateInstance(data.staticPings[i].staticType, pComp);
		pComp->QueryInterface("IStatic", staticPings[i]);
	}

	setStateInfo();
}
//--------------------------------------------------------------------------//
//
void Menu_slots::onListSelection (S32 listID)		// user has selected a list item
{
	U32 testID = listID - DROP_SLOT_BEGIN;

	if (testID < MAX_PLAYERS)
	{
		// changing player state
		onListSelectionState(testID);
		return;
	}

	testID = listID - DROP_RACE_BEGIN;
	if (testID < MAX_PLAYERS)
	{
		// change player race
		onListSelectionRace(testID);
		return;
	}

	testID = listID - DROP_PLAYER_BEGIN;
	if (testID < MAX_PLAYERS)
	{
		// changing player ID / player Color
		onListSelectionPlayer(testID);
		return;
	}

	testID = listID - DROP_TEAM_BEGIN;
	if (testID < MAX_PLAYERS)
	{
		// changing team number
		onListSelectionTeam(testID);
		return;
	}
}
//--------------------------------------------------------------------------//
//
void Menu_slots::onListSelectionState (S32 listID)		
{
	CQASSERT(U32(listID) < cqgame.activeSlots);

	S32 sel = -1;

	sel = dropSlots[listID]->GetCurrentSelection();		// if == 3, selected user name
	if (U32(sel) < 7)	// if not user name selected
	{
		// was this slot available for use?
		if (cqgame.slot[listID].state == OPEN)
		{
			switch (sel)
			{
			case 0:	//open
				break;
			case 1:	// closed
				cqgame.SetState(listID, CLOSED, true);
				break;
			case 2:	// computer
				addComputerPlayer(listID,EASY_CH);
				break;
			case 3:
				addComputerPlayer(listID,AVERAGE_CH);
				break;
			case 4:
				addComputerPlayer(listID,HARD_CH);
				break;
			case 5:
				addComputerPlayer(listID,IMPOSIBLE_CH);
				break;
			case 6:
				addComputerPlayer(listID,NIGHTMARE_CH);
				break;
			}
		}
		else if (cqgame.slot[listID].state == CLOSED)
		{
			switch (sel)
			{
			case 0:	//open
				cqgame.SetState(listID, OPEN, true);
				break;
			case 1:	// closed
				break;
			case 2:	// computer
				addComputerPlayer(listID,EASY_CH);
				break;
			case 3:
				addComputerPlayer(listID,AVERAGE_CH);
				break;
			case 4:
				addComputerPlayer(listID,HARD_CH);
				break;
			case 5:
				addComputerPlayer(listID,IMPOSIBLE_CH);
				break;
			case 6:
				addComputerPlayer(listID,NIGHTMARE_CH);
				break;
			}
		}
		else  // active or ready
		{
			switch (sel)
			{
			case 0:	//open
				if (cqgame.slot[listID].type != HUMAN || bootPlayer(listID))
					cqgame.SetState(listID, OPEN, true);
				break;
			case 1:	// closed
				if (cqgame.slot[listID].type != HUMAN || bootPlayer(listID))
					cqgame.SetState(listID, CLOSED, true);
				break;
			case 2:	// computer
				if (cqgame.slot[listID].type == HUMAN)
				{
					if (bootPlayer(listID))
						addComputerPlayer(listID,EASY_CH);
				}
				else
				{
					cqgame.SetCompChalange(listID, EASY_CH);
				}
				break;
			case 3:	// computer
				if (cqgame.slot[listID].type == HUMAN)
				{
					if (bootPlayer(listID))
						addComputerPlayer(listID,AVERAGE_CH);
				}
				else
				{
					cqgame.SetCompChalange(listID, AVERAGE_CH);
				}
				break;
			case 4:	// computer
				if (cqgame.slot[listID].type == HUMAN)
				{
					if (bootPlayer(listID))
						addComputerPlayer(listID,HARD_CH);
				}
				else
				{
					cqgame.SetCompChalange(listID, HARD_CH);
				}
				break;
			case 5:	// computer
				if (cqgame.slot[listID].type == HUMAN)
				{
					if (bootPlayer(listID))
						addComputerPlayer(listID,IMPOSIBLE_CH);
				}
				else
				{
					cqgame.SetCompChalange(listID, IMPOSIBLE_CH);
				}
				break;
			case 6:	// computer
				if (cqgame.slot[listID].type == HUMAN)
				{
					if (bootPlayer(listID))
						addComputerPlayer(listID,NIGHTMARE_CH);
				}
				else
				{
					cqgame.SetCompChalange(listID, NIGHTMARE_CH);
				}
				break;
			}
		}
	}
}
//--------------------------------------------------------------------------//
//
void Menu_slots::onListSelectionRace (S32 listID)		
{
	int sel;
	int index = listID%MAX_PLAYERS;

	sel = dropRaces[index]->GetCurrentSelection();
	sel = dropRaces[index]->GetDataValue(sel);

	cqgame.SetRace(index, static_cast<RACE>(sel));

	// are there any other player slots who share this player/color ID
	// if so, make sure that their races/teams match!
	U32 i;
	for (i = 0; i < cqgame.activeSlots; i++)
	{
		if (i == (U32)index)
		{
			// if this is a computer player, then change his name accordingly
			if (cqgame.slot[i].type == COMPUTER)
			{
				if (cqgame.slot[i].race == TERRAN)
				{
					wcsncpy(cqgame.szPlayerNames[i], terranComputerNames[i], M_MAX_STRING);
				}
				else if (cqgame.slot[i].race == MANTIS)
				{
					wcsncpy(cqgame.szPlayerNames[i], mantisComputerNames[i], M_MAX_STRING);
				}
				else
				{
					wcsncpy(cqgame.szPlayerNames[i], solarianComputerNames[i], M_MAX_STRING);
				}
				staticNames[i]->SetText(cqgame.szPlayerNames[i]);
			}
		}
		else if (cqgame.slot[i].color == cqgame.slot[index].color) 
		{
			if (cqgame.slot[i].race != cqgame.slot[index].race)
			{
				cqgame.SetRace(i, cqgame.slot[index].race, true);
			}
			if (cqgame.slot[i].team != cqgame.slot[index].team)
			{
				cqgame.SetTeam(i, cqgame.slot[index].team, true);
			}
		}
	}
}
//--------------------------------------------------------------------------//
//
void Menu_slots::onListSelectionPlayer (S32 listID)		
{
	int sel;
	int index = listID%MAX_PLAYERS;

	sel = dropPlayers[index]->GetCurrentSelection();
	cqgame.SetColor(index, static_cast<COLOR>(dropPlayers[index]->GetDataValue(sel)), true);

	// are there any other player slots who share this player/color ID
	// if so, make sure that their races/teams match!
	U32 i;
	for (i = 0; i < cqgame.activeSlots; i++)
	{
		if (i == (U32)index)
		{
			continue;
		}
		else if (cqgame.slot[i].color == cqgame.slot[index].color) 
		{
			if (cqgame.slot[i].race != cqgame.slot[index].race)
			{
				cqgame.SetRace(i, cqgame.slot[index].race, true);
			}
			if (cqgame.slot[i].team != cqgame.slot[index].team)
			{
				cqgame.SetTeam(i, cqgame.slot[index].team, true);
			}
		}
	}
}
//--------------------------------------------------------------------------//
//
void Menu_slots::onListSelectionTeam (S32 listID)		
{
	int sel;
	int index = listID%MAX_PLAYERS;

	sel = dropTeams[index]->GetCurrentSelection();
	cqgame.SetTeam(index, static_cast<TEAM>(sel), true);

	// are there any other player slots who share this player/color ID
	// if so, make sure that their races/teams match!
	U32 i;
	for (i = 0; i < cqgame.activeSlots; i++)
	{
		if (i == (U32)index)
		{
			continue;
		}
		else if (cqgame.slot[i].color == cqgame.slot[index].color) 
		{
			if (cqgame.slot[i].race != cqgame.slot[index].race)
			{
				cqgame.SetRace(i, cqgame.slot[index].race, true);
			}
			if (cqgame.slot[i].team != cqgame.slot[index].team)
			{
				cqgame.SetTeam(i, cqgame.slot[index].team, true);
			}
		}
	}
}
//--------------------------------------------------------------------------//
//
bool Menu_slots::bootPlayer (S32 row)
{
	bool result = false;

	if (CQMessageBox(IDS_HELP_WARNBOOT, IDS_CONFIRM, MB_OKCANCEL))
	{
		NETBUFFER->DestroyPlayer(cqgame.slot[row].dpid);
		result = true;
	}
	else
	{
		dropSlots[row]->SetCurrentSelection(7);
		dropSlots[row]->EnsureVisible(0);
	}

	return result;
}
//--------------------------------------------------------------------------//
//
void Menu_slots::addComputerPlayer (S32 row, CQGAMETYPES::COMP_CHALANGE compChalange)
{
	COLOR temp, color;

	// get an available color
	for (temp = YELLOW, color = UNDEFINEDCOLOR; temp < YELLOW + MAX_PLAYERS; temp = COLOR((int)temp + 1))
	{
		if (isAvailable(temp))
		{
			color = temp;
			break;
		}
	}

	cqgame.SetState(row, READY);
	cqgame.SetType(row, COMPUTER);
	cqgame.SetCompChalange(row, compChalange);
	cqgame.SetRace(row, TERRAN);
	cqgame.SetColor(row, color);
	cqgame.SetTeam(row, NOTEAM);
	cqgame.SetState(row, READY, true);		// gets set to ACTIVE by previous calls

	// get the computer name
	wcsncpy(cqgame.szPlayerNames[row], terranComputerNames[row], M_MAX_STRING);
}
//--------------------------------------------------------------------------//
//
void  __stdcall DoMenu_slots (Frame * parent, const GT_MENU1 & data, ICQGame & cqgame)
{
	new Menu_slots(parent, data, cqgame);
}

//----------------------------------------------------------------------------//
//----------------------------End Menu_slots.cpp------------------------------//
//----------------------------------------------------------------------------//
