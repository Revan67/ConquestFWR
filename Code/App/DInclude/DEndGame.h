#ifndef DENDGAME_H
#define DENDGAME_H
//--------------------------------------------------------------------------//
//                                                                          //
//                               DEndGame.h       							//
//                                                                          //
//                  COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/DInclude/DEndGame.h 8     8/22/00 12:44p Sbarton $
*/			    
//--------------------------------------------------------------------------//

#ifndef DBUTTON_H
#include "DButton.h"
#endif

#ifndef DSTATIC_H
#include "DStatic.h"
#endif

#ifndef DLISTBOX_H
#include "DListBox.h"
#endif

#ifndef DTABCONTROL_H
#include "DTabControl.h"
#endif

//--------------------------------------------------------------------------//
//  Definition file for front end menu #1
//--------------------------------------------------------------------------//

#define TITLES_O 4
#define TITLES_U 6
#define TITLES_B 6
#define TITLES_R 4

struct GT_ENDGAME
{
	RECT screenRect;
	STATIC_DATA background, banner;
	BUTTON_DATA cont;
	LISTBOX_DATA list;

	STATIC_DATA staticMenu;
	TABCONTROL_DATA tab;
	struct TAB1
	{
		STATIC_DATA staticOverviewTitles[TITLES_O], staticOverviewUnits[MAX_PLAYERS], staticOverviewBuildings[MAX_PLAYERS];
		STATIC_DATA staticOverviewResources[MAX_PLAYERS], staticOverviewTotals[MAX_PLAYERS];
	} overviewTab;
	struct TAB2
	{
		STATIC_DATA staticUnitsTitles[TITLES_U], staticUnitsMade[MAX_PLAYERS], staticUnitsLost[MAX_PLAYERS], staticUnitsKills[MAX_PLAYERS];
		STATIC_DATA staticUnitsConverted[MAX_PLAYERS], staticUnitsAdmirals[MAX_PLAYERS], staticUnitsTotals[MAX_PLAYERS];
	} unitsTab;
	struct TAB3
	{
		STATIC_DATA staticBuildingsTitles[TITLES_B], staticBuildingsMade[MAX_PLAYERS], staticBuildingsLost[MAX_PLAYERS], staticBuildingsDestroyed[MAX_PLAYERS];
		STATIC_DATA staticBuildingsConverted[MAX_PLAYERS], staticBuildingsResearch[MAX_PLAYERS], staticBuildingsTotals[MAX_PLAYERS];
	} buildingsTab;
	struct TAB5
	{
		STATIC_DATA staticResourcesTitles[TITLES_R], staticResourcesCrew[MAX_PLAYERS], staticResourcesOre[MAX_PLAYERS];
		STATIC_DATA staticResourcesGas[MAX_PLAYERS], staticResourcesTotals[MAX_PLAYERS];
	} resourceTab;

	STATIC_DATA staticPlayer, staticPlayerArray[MAX_PLAYERS];
	STATIC_DATA staticTime;
	STATIC_DATA staticDescription;
};

#endif
