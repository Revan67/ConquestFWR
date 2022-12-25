//--------------------------------------------------------------------------//
//                                                                          //
//                             Menu_EndGame.cpp                             //
//                                                                          //
//               COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
   $Header: /Conquest/App/Src/Menu_EndGame.cpp 51    7/16/01 9:22a Tmauer $
*/
//--------------------------------------------------------------------------//
 
#include "pch.h"
#include <globals.h>

#include <DEndGame.h>
#include <MGlobals.h>
#include "Frame.h"
#include "IButton2.h"
#include "IStatic.h"
#include "IListbox.h"
#include "ITabControl.h"

#include "IShapeLoader.h"
#include "IImageReader.h"
#include "MusicManager.h"
#include "IGameProgress.h"

#include "OpAgent.h"
#include "NetPacket.h"
#include "CQGame.h"

#include <stdio.h>

#define TAB_VFX		"VFXShape!!TabEndGame"

#define MAX_TICKS  8000

//--------------------------------------------------------------------------//
//
//--------------------------------------------------------------------------//
//
struct MenuEndGame : public DAComponent<Frame>
{
	//
	// data items
	//
	GT_ENDGAME data;
	COMPTR<IButton2> cont;
	COMPTR<IStatic> background, banner;
	COMPTR<IListbox> list;

	COMPTR<IStatic> staticMenu;
	COMPTR<ITabControl> tabControl;
	COMPTR<IStatic> staticPlayer, staticPlayerArray[MAX_PLAYERS];

	// in tab one (Overview)
	COMPTR<IStatic> staticOverviewTitles[TITLES_O];
	COMPTR<IStatic> staticOverviewUnits[MAX_PLAYERS];
	COMPTR<IStatic> staticOverviewBuildings[MAX_PLAYERS];
	COMPTR<IStatic> staticOverviewResources[MAX_PLAYERS];
	COMPTR<IStatic> staticOverviewTotals[MAX_PLAYERS];

	// in tab two (Units)
	COMPTR<IStatic> staticUnitsTitles[TITLES_U];
	COMPTR<IStatic> staticUnitsMade[MAX_PLAYERS];
	COMPTR<IStatic> staticUnitsLost[MAX_PLAYERS];
	COMPTR<IStatic> staticUnitsKills[MAX_PLAYERS];
	COMPTR<IStatic> staticUnitsConverted[MAX_PLAYERS];
	COMPTR<IStatic> staticUnitsAdmirals[MAX_PLAYERS];
	COMPTR<IStatic> staticUnitsTotals[MAX_PLAYERS];

	// in tab three (Buildings)
	COMPTR<IStatic> staticBuildingsTitles[TITLES_B];	// fixit - one of the titles should come out of data
	COMPTR<IStatic> staticBuildingsMade[MAX_PLAYERS];
	COMPTR<IStatic> staticBuildingsLost[MAX_PLAYERS];
	COMPTR<IStatic> staticBuildingsDestroyed[MAX_PLAYERS];
	COMPTR<IStatic> staticBuildingsConverted[MAX_PLAYERS];
//	COMPTR<IStatic> staticBuildingsResearch[MAX_PLAYERS];  // fixit - take out of data
	COMPTR<IStatic> staticBuildingsTotals[MAX_PLAYERS];

	// in tab four (Resources)
	COMPTR<IStatic> staticResourcesTitles[TITLES_R];
	COMPTR<IStatic> staticResourcesCrew[MAX_PLAYERS];
	COMPTR<IStatic> staticResourcesOre[MAX_PLAYERS];
	COMPTR<IStatic> staticResourcesGas[MAX_PLAYERS];
	COMPTR<IStatic> staticResourcesTotals[MAX_PLAYERS];

//	COMPTR<IStatic> staticDescription;
	COMPTR<IStatic> staticTime;

	const bool bWon;
	
	U32 numberPlayers;
	int slotIndexArray[MAX_PLAYERS];

	//
	// instance methods
	//

	MenuEndGame (bool _bWon) : bWon(_bWon)
	{
		initializeFrame(NULL);
		init();
	}

	~MenuEndGame (void);

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

	DEFMETHOD(Notify) (U32 message, void *param);


	/* MenuEndGame methods */

	virtual void setStateInfo (void);

	virtual void onButtonPressed (U32 buttonID)
	{
		switch (buttonID)
		{
		case IDS_CONTINUE:
			endDialog(1);
			break;
		}
	}

	virtual bool onEscPressed (void)	//  return true if message was handled
	{
		endDialog(0);
		return true;
	}

	void init (void);

	void initNames (void);

	void initScores (void);

	void initTime (void);

	void checkForMovie (void);
};
//----------------------------------------------------------------------------------//
//
MenuEndGame::~MenuEndGame (void)
{
}
//----------------------------------------------------------------------------------//
//
GENRESULT MenuEndGame::OnUpdate (struct IDocument *doc, const C8 *message, void *parm)
{
	DWORD dwRead;

	doc->SetFilePointer(0,0);
	doc->ReadFile(0, &data, sizeof(data), &dwRead, 0);
	setStateInfo();

	return GR_OK;
}
//----------------------------------------------------------------------------------//
//
GENRESULT MenuEndGame::Notify (U32 message, void *param)
{
	if (message == CQE_TABCONTROL_CHANGE)
	{
		// change our description static control
//		int index = int(param);

//		staticDescription->SetTextID(IDS_SCORE_DESC_BEGIN + index + 1);

		// eat the message
		return GR_GENERIC;
	}

	return Frame::Notify(message, param);

}
//----------------------------------------------------------------------------------//
//
void MenuEndGame::checkForMovie (void)
{
	// we play movies after certain missions
	const U32 gameMissionID = MGlobals::GetMissionID();
//	const U32 movieWidth = 640;
//	const U32 movieHeight = 272;

	if (bWon)
	{
		switch (gameMissionID)
		{
		case 2:
//			MovieScreen(this, "cq_1.mpg", movieWidth, movieHeight);
			GAMEPROGRESS->SetMovieSeen(1);
			break;

		case 8:
//			MovieScreen(this, "cq_2.mpg", movieWidth, movieHeight);
			GAMEPROGRESS->SetMovieSeen(2);
			break;

		case 10:
//			MovieScreen(this, "cq_3.mpg", movieWidth, movieHeight);
			GAMEPROGRESS->SetMovieSeen(3);
			break;

		case 15:
//			MovieScreen(this, "cq_4b.mpg", movieWidth, movieHeight);
			GAMEPROGRESS->SetMovieSeen(5);
			break;
		}
	}
	else
	{
		if (gameMissionID == 12 || gameMissionID == 15)
		{
//			MovieScreen(this, "cq_4.mpg", movieWidth, movieHeight);
			GAMEPROGRESS->SetMovieSeen(4);

			if (gameMissionID == 15)
			{
				// special, let's us know if we've seen the losing movie by losing the last mission
				GAMEPROGRESS->SetMovieSeen(6);
			}
		}
	}
}
//----------------------------------------------------------------------------------//
//
void MenuEndGame::setStateInfo (void)
{
	// we've just completed a mission, is there a movie we have to play?
	if (MGlobals::GetGameSettings().gameType == CQGAMETYPES::MISSION_DEFINED)
	{
		checkForMovie();
	}

	U32 i;
	//
	// create members if not done already
	//
	screenRect.left		= IDEAL2REALX(data.screenRect.left);
	screenRect.right	= IDEAL2REALX(data.screenRect.right);
	screenRect.top		= IDEAL2REALY(data.screenRect.top);
	screenRect.bottom	= IDEAL2REALY(data.screenRect.bottom);

	cont->InitButton(data.cont, this);
	cont->SetTransparent(true);

	background->InitStatic(data.background, this);
	staticMenu->InitStatic(data.staticMenu, this);
	banner->InitStatic(data.banner, this);
	list->InitListbox(data.list, this); 

	staticPlayer->InitStatic(data.staticPlayer, this);
	for (i = 0; i < MAX_PLAYERS; i++)
	{
		staticPlayerArray[i]->InitStatic(data.staticPlayerArray[i], this);
		staticPlayerArray[i]->SetVisible(false);
	}

//	staticDescription->InitStatic(data.staticDescription, this);
	staticTime->InitStatic(data.staticTime, this);

	// load up the tab control - man I hate doing this
	COMPTR<IDAComponent> pBase;
	COMPTR<IShapeLoader> loader;

	GENDATA->CreateInstance(TAB_VFX, pBase);
	pBase->QueryInterface("IShapeLoader", loader);

	// init the tab control
	tabControl->InitTab(data.tab, this, loader);
	COMPTR<BaseHotRect> base;

	//
	// put everything into the overview tab
	//
	tabControl->GetTabMenu(0, base);

	for (i = 0; i < TITLES_O; i++)
	{
		staticOverviewTitles[i]->InitStatic(data.overviewTab.staticOverviewTitles[i], base);
	}

	for (i = 0; i < MAX_PLAYERS; i++)
	{
		staticOverviewUnits[i]->InitStatic(data.overviewTab.staticOverviewUnits[i], base);
		staticOverviewBuildings[i]->InitStatic(data.overviewTab.staticOverviewBuildings[i], base);
		staticOverviewResources[i]->InitStatic(data.overviewTab.staticOverviewResources[i], base);
		staticOverviewTotals[i]->InitStatic(data.overviewTab.staticOverviewTotals[i], base);

		// everyting is invisible at first
		staticOverviewUnits[i]->SetVisible(false);
		staticOverviewBuildings[i]->SetVisible(false);
		staticOverviewResources[i]->SetVisible(false);
		staticOverviewTotals[i]->SetVisible(false);

	}

	//
	// fill out the units tab
	//
	tabControl->GetTabMenu(1, base);

	for (i = 0; i < TITLES_U; i++)
	{
		staticUnitsTitles[i]->InitStatic(data.unitsTab.staticUnitsTitles[i], base);
	}

	for (i = 0; i < MAX_PLAYERS; i++)
	{
		staticUnitsMade[i]->InitStatic(data.unitsTab.staticUnitsMade[i], base);
		staticUnitsLost[i]->InitStatic(data.unitsTab.staticUnitsLost[i], base);
		staticUnitsKills[i]->InitStatic(data.unitsTab.staticUnitsKills[i], base);
		staticUnitsConverted[i]->InitStatic(data.unitsTab.staticUnitsConverted[i], base);
		staticUnitsAdmirals[i]->InitStatic(data.unitsTab.staticUnitsAdmirals[i], base);
		staticUnitsTotals[i]->InitStatic(data.unitsTab.staticUnitsTotals[i], base);

		staticUnitsMade[i]->SetVisible(false);
		staticUnitsLost[i]->SetVisible(false);
		staticUnitsKills[i]->SetVisible(false);
		staticUnitsConverted[i]->SetVisible(false);
		staticUnitsAdmirals[i]->SetVisible(false);
		staticUnitsTotals[i]->SetVisible(false);
	}

	// 
	// fill out the buildings tab
	//
	tabControl->GetTabMenu(2, base);

	for (i = 0; i < TITLES_B; i++)
	{
		staticBuildingsTitles[i]->InitStatic(data.buildingsTab.staticBuildingsTitles[i], base);
	}
	staticBuildingsTitles[4]->SetVisible(false);

	for (i = 0; i < MAX_PLAYERS; i++)
	{
		staticBuildingsMade[i]->InitStatic(data.buildingsTab.staticBuildingsMade[i], base);
		staticBuildingsLost[i]->InitStatic(data.buildingsTab.staticBuildingsLost[i], base);
		staticBuildingsDestroyed[i]->InitStatic(data.buildingsTab.staticBuildingsDestroyed[i], base);
		staticBuildingsConverted[i]->InitStatic(data.buildingsTab.staticBuildingsConverted[i], base);
		staticBuildingsTotals[i]->InitStatic(data.buildingsTab.staticBuildingsTotals[i], base);

		staticBuildingsMade[i]->SetVisible(false);
		staticBuildingsLost[i]->SetVisible(false);
		staticBuildingsDestroyed[i]->SetVisible(false);
		staticBuildingsConverted[i]->SetVisible(false);
		staticBuildingsTotals[i]->SetVisible(false);
	}

	//
	// fill out the resources tab
	//
	tabControl->GetTabMenu(3, base);

	for (i = 0; i < TITLES_R; i++)
	{
		staticResourcesTitles[i]->InitStatic(data.resourceTab.staticResourcesTitles[i], base);
	}

	for (i = 0; i < MAX_PLAYERS; i++)
	{
		staticResourcesCrew[i]->InitStatic(data.resourceTab.staticResourcesCrew[i], base); 
		staticResourcesOre[i]->InitStatic(data.resourceTab.staticResourcesOre[i], base);
		staticResourcesGas[i]->InitStatic(data.resourceTab.staticResourcesGas[i], base);
		staticResourcesTotals[i]->InitStatic(data.resourceTab.staticResourcesTotals[i], base);

		staticResourcesCrew[i]->SetVisible(false);
		staticResourcesOre[i]->SetVisible(false);
		staticResourcesGas[i]->SetVisible(false);
		staticResourcesTotals[i]->SetVisible(false);
	}

	if (bWon)
	{
		banner->SetText(_localLoadStringW(IDS_YOU_WIN));
		MUSICMANAGER->PlayMusic("victory.wav");
	}
	else
	{
		banner->SetText(_localLoadStringW(IDS_YOU_LOSE));
		MUSICMANAGER->PlayMusic("defeat.wav");
	}

	initNames();
	initScores();
	initTime();

	list->SetVisible(false);

	tabControl->EnableKeyboardFocusing();
	tabControl->SetCurrentTab(0);

	if (childFrame)
		childFrame->setStateInfo();
	else
	{
		setFocus(cont);
	}
}
//--------------------------------------------------------------------------//
//
void MenuEndGame::initTime (void)
{
	// put the right variable into the time text
	const U32 updateCounter = MGlobals::GetUpdateCount();			// number of times we have updated AI
	U32 hours;
	U32 minutes;
	U32 seconds;
	U32 rem;

	hours   = updateCounter/(U32(REALTIME_FRAMERATE*8*3600));
	rem = updateCounter%(U32(REALTIME_FRAMERATE*8*3600));
	minutes = rem/(U32(REALTIME_FRAMERATE*8*60));
	rem = rem%(U32(REALTIME_FRAMERATE*8*60));
	seconds = rem / (U32(REALTIME_FRAMERATE*8));

	wchar_t buffer[128];
	swprintf(buffer, _localLoadStringW(IDS_FORMAT_GAMETIME), hours, minutes, seconds);
	staticTime->SetText(buffer);
} 
//--------------------------------------------------------------------------//
//
void MenuEndGame::initNames (void)
{
	wchar_t name[32];
	const CQGAME & cqgame = MGlobals::GetGameSettings();
	U32 i;
	
	numberPlayers = 0;

	for (i = 0; i < cqgame.activeSlots; i++)
	{
		if (cqgame.slot[i].state == CQGAMETYPES::READY)
		{
			U32 playID = MGlobals::GetPlayerIDFromSlot(i);

			if (playID != 0)
			{
				slotIndexArray[numberPlayers++] = i;
			}
		}
	}

	for (i = 0; i < numberPlayers; i++)
	{
		if (MGlobals::GetPlayerNameBySlot(slotIndexArray[i], name, sizeof(name)) == 0)
		{
			// might be a computer player
			if (cqgame.slot[i].type == CQGAMETYPES::COMPUTER && cqgame.slot[i].state == CQGAMETYPES::READY)
			{
				wcsncpy(name, _localLoadStringW(IDS_COMPUTER_NAME), sizeof(name)/sizeof(wchar_t));
			}
		}

		staticPlayerArray[i]->SetText(name);
		staticPlayerArray[i]->SetVisible(true);
	}
}
//--------------------------------------------------------------------------//
//
void MenuEndGame::initScores (void)
{
	U32 i;
	U32 playID;

	int over_unit[8];
	int over_bldg[8];
	int over_resr[8];
	int over_totl[8];

	int units_made[8];
	int units_lost[8];
	int units_kill[8];
	int units_conv[8];
	int units_admr[8];
	int units_totl[8];

	// get the scores for the units tab
	for (i = 0; i < numberPlayers; i++)
	{
		playID = MGlobals::GetPlayerIDFromSlot(slotIndexArray[i]);

		units_totl[i] = 0;
		units_totl[i] += units_made[i] = MGlobals::GetNumUnitsBuilt(playID) * 10;
		units_totl[i] += units_lost[i] = MGlobals::GetUnitsLost(playID) * -5;
		units_totl[i] += units_kill[i] = MGlobals::GetUnitsDestroyed(playID) * 10;
		units_totl[i] += units_conv[i] = MGlobals::GetUnitsConverted(playID) * 50;
		units_totl[i] += units_admr[i] = MGlobals::GetNumAdmiralsBuilt(playID)  * 100;

		over_totl[i] = over_unit[i] = units_totl[i];

		// take away the magic multipliers
		units_made[i] /= 10;
		units_lost[i] /= -5;
		units_kill[i] /= 10;
		units_conv[i] /= 50;
		units_admr[i] /= 100;

		staticUnitsMade[i]->SetVisible(true);
		staticUnitsMade[i]->EnableRollupBehavior(units_made[i]);

		staticUnitsLost[i]->SetVisible(true);
		staticUnitsLost[i]->EnableRollupBehavior(units_lost[i]);
	
		staticUnitsKills[i]->SetVisible(true);
		staticUnitsKills[i]->EnableRollupBehavior(units_kill[i]);

		staticUnitsConverted[i]->SetVisible(true);
		staticUnitsConverted[i]->EnableRollupBehavior(units_conv[i]);

		staticUnitsAdmirals[i]->SetVisible(true);
		staticUnitsAdmirals[i]->EnableRollupBehavior(units_admr[i]);

		staticUnitsTotals[i]->SetVisible(true);
		staticUnitsTotals[i]->EnableRollupBehavior(units_totl[i]);
	}

	int build_made[8];
	int build_lost[8];
	int build_dest[8];
	int build_conv[8];
	int build_totl[8];

	// get the scores for the buildings tab
	for (i = 0; i < numberPlayers; i++)
	{
		playID = MGlobals::GetPlayerIDFromSlot(slotIndexArray[i]);

		build_totl[i] = 0;
		build_totl[i] += build_made[i] = MGlobals::GetNumPlatformsBuilt(playID) * 25;
		build_totl[i] += build_lost[i] = MGlobals::GetPlatformsLost(playID) * -10;
		build_totl[i] += build_dest[i] = MGlobals::GetPlatformsDestroyed(playID) * 25;
		build_totl[i] += build_conv[i] = MGlobals::GetPlatformsConverted(playID) * 50;

		over_totl[i] += over_bldg[i] = build_totl[i];

		// take away the magic multipliers
		build_made[i] /= 25;
		build_lost[i] /= -10;
		build_dest[i] /= 25;
		build_conv[i] /= 50;

		staticBuildingsMade[i]->SetVisible(true);
		staticBuildingsMade[i]->EnableRollupBehavior(build_made[i]);
	
		staticBuildingsLost[i]->SetVisible(true);
		staticBuildingsLost[i]->EnableRollupBehavior(build_lost[i]);

		staticBuildingsDestroyed[i]->SetVisible(true);
		staticBuildingsDestroyed[i]->EnableRollupBehavior(build_dest[i]);

		staticBuildingsConverted[i]->SetVisible(true);
		staticBuildingsConverted[i]->EnableRollupBehavior(build_conv[i]);

		staticBuildingsTotals[i]->SetVisible(true);
		staticBuildingsTotals[i]->EnableRollupBehavior(build_totl[i]);

	}

	int res_crw[8];
	int res_ore[8];
	int res_gas[8];
	int res_tot[8];

	// get the scores for the resources tab
	for (i = 0; i < numberPlayers; i++)
	{
		playID = MGlobals::GetPlayerIDFromSlot(slotIndexArray[i]);

		res_tot[i] = 0;
		res_tot[i] += res_crw[i] = MGlobals::GetCrewGained(playID) * CREW_MULTIPLIER / 10;
		res_tot[i] += res_ore[i] = MGlobals::GetMetalGained(playID) * METAL_MULTIPLIER / 10;
		res_tot[i] += res_gas[i] = MGlobals::GetGasGained(playID) * GAS_MULTIPLIER / 10;

		over_totl[i] += over_resr[i] = res_tot[i];

		res_crw[i] *= 10;
		res_ore[i] *= 10;
		res_gas[i] *= 10;

		staticResourcesCrew[i]->SetVisible(true);
		staticResourcesCrew[i]->EnableRollupBehavior(res_crw[i]);

		staticResourcesOre[i]->SetVisible(true);
		staticResourcesOre[i]->EnableRollupBehavior(res_ore[i]);

		staticResourcesGas[i]->SetVisible(true);
		staticResourcesGas[i]->EnableRollupBehavior(res_gas[i]);

		staticResourcesTotals[i]->SetVisible(true);
		staticResourcesTotals[i]->EnableRollupBehavior(res_tot[i]);
	}

	// initialize the scores in the overview tab
	for (i = 0; i < numberPlayers; i++)
	{
		staticOverviewUnits[i]->SetVisible(true);
		staticOverviewUnits[i]->EnableRollupBehavior(over_unit[i]);

		staticOverviewBuildings[i]->SetVisible(true);
		staticOverviewBuildings[i]->EnableRollupBehavior(over_bldg[i]);

		staticOverviewResources[i]->SetVisible(true);
		staticOverviewResources[i]->EnableRollupBehavior(over_resr[i]);

		staticOverviewTotals[i]->SetVisible(true);
		staticOverviewTotals[i]->EnableRollupBehavior(over_totl[i]);
	}
}
//--------------------------------------------------------------------------//
//
void MenuEndGame::init (void)
{
	data = 	*((GT_ENDGAME *) GENDATA->GetArchetypeData("MenuEndGame"));

	COMPTR<IDAComponent> pComp;
	GENDATA->CreateInstance(data.cont.buttonType, pComp);
	pComp->QueryInterface("IButton2", cont);

	GENDATA->CreateInstance(data.background.staticType, pComp);
	pComp->QueryInterface("IStatic", background);

	GENDATA->CreateInstance(data.banner.staticType, pComp);
	pComp->QueryInterface("IStatic", banner);

	GENDATA->CreateInstance(data.list.listboxType, pComp);
	pComp->QueryInterface("IListbox", list);

	GENDATA->CreateInstance(data.staticMenu.staticType, pComp);
	pComp->QueryInterface("IStatic", staticMenu);

	GENDATA->CreateInstance(data.staticPlayer.staticType, pComp);
	pComp->QueryInterface("IStatic", staticPlayer);

//	GENDATA->CreateInstance(data.staticDescription.staticType, pComp);
//	pComp->QueryInterface("IStatic", staticDescription);

	U32 i;
	for (i = 0; i < MAX_PLAYERS; i++)
	{
		GENDATA->CreateInstance(data.staticPlayerArray[i].staticType, pComp);
		pComp->QueryInterface("IStatic", staticPlayerArray[i]);
	}

	// tab contol stuff
	GENDATA->CreateInstance(data.tab.tabControlType, pComp);
	pComp->QueryInterface("ITabControl", tabControl);

	for (i = 0; i < 5; i++)
	{
		GENDATA->CreateInstance(data.overviewTab.staticOverviewTitles[i].staticType, pComp);
		pComp->QueryInterface("IStatic", staticOverviewTitles[i]);
	}

	for (i = 0; i < 6; i++)
	{
		GENDATA->CreateInstance(data.unitsTab.staticUnitsTitles[i].staticType, pComp);
		pComp->QueryInterface("IStatic", staticUnitsTitles[i]);
	}

	for (i = 0; i < 6; i++)
	{
		GENDATA->CreateInstance(data.buildingsTab.staticBuildingsTitles[i].staticType, pComp);
		pComp->QueryInterface("IStatic", staticBuildingsTitles[i]);
	}

	for (i = 0; i < 4; i++)
	{
		GENDATA->CreateInstance(data.resourceTab.staticResourcesTitles[i].staticType, pComp);
		pComp->QueryInterface("IStatic", staticResourcesTitles[i]);
	}
	
	for (i = 0; i < MAX_PLAYERS; i++)
	{
		// in the overview tab
		GENDATA->CreateInstance(data.overviewTab.staticOverviewUnits[i].staticType, pComp);
		pComp->QueryInterface("IStatic", staticOverviewUnits[i]);

		GENDATA->CreateInstance(data.overviewTab.staticOverviewBuildings[i].staticType, pComp);
		pComp->QueryInterface("IStatic", staticOverviewBuildings[i]);

		GENDATA->CreateInstance(data.overviewTab.staticOverviewResources[i].staticType, pComp);
		pComp->QueryInterface("IStatic", staticOverviewResources[i]);

		GENDATA->CreateInstance(data.overviewTab.staticOverviewTotals[i].staticType, pComp);
		pComp->QueryInterface("IStatic", staticOverviewTotals[i]);

		// in the units tab
		GENDATA->CreateInstance(data.unitsTab.staticUnitsMade[i].staticType, pComp);
		pComp->QueryInterface("IStatic", staticUnitsMade[i]);

		GENDATA->CreateInstance(data.unitsTab.staticUnitsLost[i].staticType, pComp);
		pComp->QueryInterface("IStatic", staticUnitsLost[i]);

		GENDATA->CreateInstance(data.unitsTab.staticUnitsKills[i].staticType, pComp);
		pComp->QueryInterface("IStatic", staticUnitsKills[i]);

		GENDATA->CreateInstance(data.unitsTab.staticUnitsConverted[i].staticType, pComp);
		pComp->QueryInterface("IStatic", staticUnitsConverted[i]);

		GENDATA->CreateInstance(data.unitsTab.staticUnitsAdmirals[i].staticType, pComp);
		pComp->QueryInterface("IStatic", staticUnitsAdmirals[i]);

		GENDATA->CreateInstance(data.unitsTab.staticUnitsTotals[i].staticType, pComp);
		pComp->QueryInterface("IStatic", staticUnitsTotals[i]);

		// in the buildings tab
		GENDATA->CreateInstance(data.buildingsTab.staticBuildingsMade[i].staticType, pComp);
		pComp->QueryInterface("IStatic", staticBuildingsMade[i]);

		GENDATA->CreateInstance(data.buildingsTab.staticBuildingsLost[i].staticType, pComp);
		pComp->QueryInterface("IStatic", staticBuildingsLost[i]);

		GENDATA->CreateInstance(data.buildingsTab.staticBuildingsDestroyed[i].staticType, pComp);
		pComp->QueryInterface("IStatic", staticBuildingsDestroyed[i]);

		GENDATA->CreateInstance(data.buildingsTab.staticBuildingsConverted[i].staticType, pComp);
		pComp->QueryInterface("IStatic", staticBuildingsConverted[i]);

		GENDATA->CreateInstance(data.buildingsTab.staticBuildingsTotals[i].staticType, pComp);
		pComp->QueryInterface("IStatic", staticBuildingsTotals[i]);

		// in the resources tab
		GENDATA->CreateInstance(data.resourceTab.staticResourcesCrew[i].staticType, pComp);
		pComp->QueryInterface("IStatic", staticResourcesCrew[i]);

		GENDATA->CreateInstance(data.resourceTab.staticResourcesOre[i].staticType, pComp);
		pComp->QueryInterface("IStatic", staticResourcesOre[i]);

		GENDATA->CreateInstance(data.resourceTab.staticResourcesGas[i].staticType, pComp);
		pComp->QueryInterface("IStatic", staticResourcesGas[i]);

		GENDATA->CreateInstance(data.resourceTab.staticResourcesTotals[i].staticType, pComp);
		pComp->QueryInterface("IStatic", staticResourcesTotals[i]);
	}

		GENDATA->CreateInstance(data.staticTime.staticType, pComp);
		pComp->QueryInterface("IStatic", staticTime);
}
//--------------------------------------------------------------------------//
//
U32 __stdcall CreateMenuEndGame (bool bWon)
{
	GENDATA->FlushUnusedArchetypes();
	MenuEndGame * menu = new MenuEndGame(bWon);

	menu->createViewer("\\GT_ENDGAME\\MenuEndGame", "GT_ENDGAME", IDS_VIEWENDGAME);
	menu->beginModalFocus();

	U32 result = CQDoModal(menu);
	delete menu;

	return result;
}
//--------------------------------------------------------------------------//
//-----------------------------End Menu_EndGame.cpp-------------------------//
//--------------------------------------------------------------------------//
