#ifndef DMENU1_H
#define DMENU1_H
//--------------------------------------------------------------------------//
//                                                                          //
//                               DMenu1.h       							//
//                                                                          //
//                  COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/DInclude/DMenu1.h 98    9/13/01 10:02a Tmauer $
*/			    
//--------------------------------------------------------------------------//

#ifndef DBUTTON_H
#include "DButton.h"
#endif

#ifndef DSTATIC_H
#include "DStatic.h"
#endif

#ifndef DLISTBOX_H
#include "DListbox.h"
#endif

#ifndef DEDIT_H
#include "DEdit.h"
#endif
 
#ifndef DDROPDOWN_H
#include "DDropDown.h"
#endif

#ifndef DCOMBOBOX_H
#include "DCombobox.h"
#endif

#ifndef DANIMATE_H
#include "DAnimate.h"
#endif

#ifndef DSLIDER_H
#include "DSlider.h"
#endif

#define NUM_MISSIONS 16
#define NUM_MOVIES    6


//--------------------------------------------------------------------------//
//  Definition file for front end menu #1
//--------------------------------------------------------------------------//

struct GT_MENU1
{
	struct OPENING
	{
		RECT screenRect;
		STATIC_DATA background;
		BUTTON_DATA single, multi, intro, options, help, quit;
		STATIC_DATA staticSingle, staticMulti, staticIntro, staticOptions, staticHelp;
		ANIMATE_DATA animMedia, animSingle, animMulti, animOptions, animQuestion;
		STATIC_DATA staticLegal;
	} opening;

	struct SINGLEPLAYER_MENU
	{
		RECT screenRect;
		STATIC_DATA background, staticSingle, staticName;
		BUTTON_DATA buttonCampaign, buttonSkirmish, buttonLoad, buttonQBLoad;
		BUTTON_DATA buttonBack;
	} singlePlayerMenu;

	struct SELECT_CAMPAIGN
	{
		RECT screenRect;
		STATIC_DATA background, title;
		STATIC_DATA staticName;
		BUTTON_DATA buttonTerran, buttonMantis, buttonSolarian;
		BUTTON_DATA buttonBack;
	} selectCampaign;

	struct SELECT_MISSION
	{
		RECT screenRect;
		STATIC_DATA background, title;
		LISTBOX_DATA list;
		BUTTON_DATA start, ok, cancel;

		BUTTON_DATA buttonUnlock;

		STATIC_DATA staticMission, staticHolder;
		BUTTON_DATA buttonMissions[NUM_MISSIONS];
		BUTTON_DATA buttonMovies[NUM_MOVIES];
		BUTTON_DATA buttonBack;
		STATIC_DATA staticMovie, staticMissionTitle;
		ANIMATE_DATA animSystem;
		int nLineFrom[NUM_MISSIONS];
		int nMoiveBeforeMission[NUM_MOVIES];

	} selectMission;

	struct NET_CONNECTIONS
	{
		RECT screenRect;
		STATIC_DATA background, description;
		LISTBOX_DATA list;
		BUTTON_DATA next, back;
		STATIC_DATA staticTitle;
		BUTTON_DATA buttonZone;
		BUTTON_DATA buttonWeb;
	} netConnections;
	
	struct IP_ADDRESS
	{
		RECT screenRect;
		STATIC_DATA background, description;
		STATIC_DATA enterIP, enterName;
		STATIC_DATA staticName;
		STATIC_DATA staticJoin, staticCreate;
		BUTTON_DATA checkJoin, checkCreate;
		BUTTON_DATA next, back;
		COMBOBOX_DATA comboboxIP;
	} ipAddress;

	struct GAME_ZONE
	{
		RECT screenRect;
		STATIC_DATA description;
	} gameZone;

	struct NET_SESSIONS2
	{ 
		RECT screenRect;
		STATIC_DATA background, description, version;
		STATIC_DATA title;
		LISTBOX_DATA list;
		BUTTON_DATA next, back;

		STATIC_DATA staticGame, staticPlayers, staticSpeed, staticMap, staticResources;
	} netSessions2;

	struct MSHELL
	{
		RECT screenRect;
		STATIC_DATA background;
		STATIC_DATA title;
		STATIC_DATA enterChat;
		STATIC_DATA ipaddress;
		LISTBOX_DATA listChat;
		EDIT_DATA editChat;
	} mshell;

#define MAX_PLAYERS 8		// also defined in globals.h

	struct MAP
	{
		RECT screenRect;

		BUTTON_DATA mapType;
		STATIC_DATA staticGameType, staticSpeed, staticMoney, staticUnits;
		STATIC_DATA staticMapType, staticSize, staticTerrain;
		STATIC_DATA staticVisibility, staticBandwidth;
		DROPDOWN_DATA dropGameType, dropSpeed, dropMoney;
		DROPDOWN_DATA dropSize, dropTerrain, dropUnits;
		DROPDOWN_DATA dropVisibility, bandWidthOption;

		SLIDER_DATA sliderSpeed;
		SLIDER_DATA sliderCmdPoints;

		STATIC_DATA staticSpectator, staticDiplomacy;
		BUTTON_DATA pushSpectator, pushDiplomacy;

		STATIC_DATA staticDifficulty, staticEasy, staticAverage, staticHard;
		BUTTON_DATA pushEasy, pushAverage, pushHard;

		STATIC_DATA staticLockSettings;
		BUTTON_DATA pushLockSettings;

		STATIC_DATA staticSystems;
		DROPDOWN_DATA dropSystems;

		STATIC_DATA staticCmdPoints;
		STATIC_DATA staticCmdPointsDisplay;
	} map;

	struct SLOTS
	{
		RECT screenRect;
		DROPDOWN_DATA dropSlots[MAX_PLAYERS];
		DROPDOWN_DATA dropTeams[MAX_PLAYERS];
		DROPDOWN_DATA dropRaces[MAX_PLAYERS];
		DROPDOWN_DATA dropPlayers[MAX_PLAYERS];
		STATIC_DATA staticNames[MAX_PLAYERS];
		STATIC_DATA staticPings[MAX_PLAYERS];
		M_STRINGW terranComputerNames[MAX_PLAYERS];
		M_STRINGW mantisComputerNames[MAX_PLAYERS];
		M_STRINGW solarianComputerNames[MAX_PLAYERS];
	} slots;

	struct FINAL
	{
		RECT screenRect;
		STATIC_DATA staticState, staticName, staticColor, staticRace, staticTeam, staticPing;
		STATIC_DATA description;
		STATIC_DATA staticAccept; 
		BUTTON_DATA accept, start, cancel;
		STATIC_DATA staticCountdown;
	} final;

	struct HELPMENU
	{
		RECT screenRect;
		STATIC_DATA background, title;
		STATIC_DATA staticConquest, staticVersion, staticNumber;
		BUTTON_DATA buttonOk;
		STATIC_DATA staticProductID, staticProductNumber;
		STATIC_DATA staticLegal;
		BUTTON_DATA	buttonCredits;
	} helpMenu;

	struct DEVICEMENU
	{
		RECT screenRect;
		STATIC_DATA staticBackground, staticTitle;
		STATIC_DATA staticPickDevice;
		LISTBOX_DATA listDevices;
		BUTTON_DATA buttonOk;
	} deviceMenu;
};

struct GT_MAPSELECT
{
	RECT screenRect;
	STATIC_DATA staticBackground, staticTitle;
	STATIC_DATA staticRandom, staticSupplied, staticSaved;
	LISTBOX_DATA listRandom, listSupplied, listSaved;
	BUTTON_DATA buttonOk, buttonCancel;
};

#endif
