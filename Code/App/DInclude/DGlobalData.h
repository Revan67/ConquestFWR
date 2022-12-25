#ifndef DGLOBALDATA_H
#define DGLOBALDATA_H
//--------------------------------------------------------------------------//
//                                                                          //
//                              DGlobalData.h								//
//                                                                          //
//                  COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.               //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/DInclude/DGlobalData.h 56    5/02/01 4:28p Tmauer $
*/
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

#ifndef DBASEDATA_H
#include <DBaseData.h>
#endif

#ifndef DMTECHNODE_H
#include "DMTechNode.h"
#endif

#ifndef DCQGAME_H
#include "DCQGame.h"
#endif

#define MAX_PLAYERS        8		// also defined in Globals.h
#define MAX_PLAYERS_PLUS_1 9		// parser cannot evaluate explessions
#define NUM_TECH_LEVELS	   6
#define NUM_RACES_PLUS_1   5
#define MAX_MISSION_OBJECTIVES 32	

//---------------------------------------------------------------------------
//
struct GT_GLOBAL_VALUES
{
	struct TargetingValues
	{
		SINGLE movePenaltySelf;
		SINGLE movePenaltyTarget;
		SINGLE minAccuracy;
	} targetingValues;

#ifdef _ADB
	struct INDKILLCHART
	{
		U32 rookie, novice, veteran, elite, superElite, superdouperElite;
	} individualKillChart;
#else
	U32 individualKillChart[NUM_TECH_LEVELS];
#endif

#ifdef _ADB
	struct ADMIRALKILLCHART
	{
		U32 rearAdmiral, viceAdmiral, admiral, starAdmiral, superstarAdmiral, superdouperAdmiral;
	} admiralKillChart;
#else
	U32 admiralKillChart[NUM_TECH_LEVELS];
#endif


#ifdef _ADB
	struct RACE_TARGETING_BONUSES
	{
		SINGLE noRace;
		SINGLE terran;
		SINGLE mantis;
		SINGLE solarian;
		SINGLE vyrium;
	} raceBonuses;
#else
	SINGLE raceTargetingBonus[NUM_RACES_PLUS_1];	// one for each race
#endif

	struct TechUpgrades
	{
		SINGLE		engine[NUM_TECH_LEVELS];			  //  ship_max_velocity * engine[upgrade] = velocity
		SINGLE 		hull[NUM_TECH_LEVELS];				  //  ship_hull * hull[upgrade] = hull
		SINGLE 		supplies[NUM_TECH_LEVELS];			  //  ship_supplies * supplies[upgrade] = supplies
		SINGLE		targeting[NUM_TECH_LEVELS];
		SINGLE		damage[NUM_TECH_LEVELS];			  //  weapon_damage * damage[upgrade] = damage
		SINGLE		shields[NUM_TECH_LEVELS];			  //  ship_shields + shields[upgrade] = shields
		SINGLE		shipTargetingExp[NUM_TECH_LEVELS];	  //  added to targeting[upgrade]
		SINGLE		admiralTargetingExp[NUM_TECH_LEVELS]; //  added to targeting[upgrade]	
		SINGLE		fleet[NUM_TECH_LEVELS];				  //  added to targeting[upgrade]	
		SINGLE		sensors[NUM_TECH_LEVELS];			  //  ship_sensorRadius * sensors[upgrade] = sensorRadius , and cloaked too	
		SINGLE		tanker[NUM_TECH_LEVELS];			  //  ship_unloadRate * tanker[upgrade] = unloadRate
														  //  ship_loadRate*tanker[upgrade] = loadRate , both nuggets and planets
		SINGLE		fighter[NUM_TECH_LEVELS];			  //  fighter_baseAccuracey * fighter[upgrade] = baseAccuracy
		SINGLE		tender[NUM_TECH_LEVELS];			  //  tender_supplyLoadSize * tender[upgrade] = supplyLoadSize
	};
	
	// use a union so we can access it as an array or a structure (for viewing)

#ifdef _ADB
	struct _upgrades
	{
		TechUpgrades noRace;
		TechUpgrades terran;
		TechUpgrades mantis;
		TechUpgrades solarian;
		TechUpgrades vyrium;
	} techUpgrades;
#else
	TechUpgrades techUpgrades[NUM_RACES_PLUS_1];
#endif
};
//---------------------------------------------------------------------------
//
struct MT_GlobalData
{
	M_STRINGW gameDescription;
	M_STRINGW playerNames[MAX_PLAYERS];		// in order that they appear in lobby
	U8 playerAssignments[MAX_PLAYERS];		// convert lobby slot into playerID
	M_RACE playerRace[MAX_PLAYERS_PLUS_1];		// used to start multiplayer session
	U32 lastPartID;			// last part id that was given out, increment before using
	U8 currentPlayer:4;  // valid from 1 to 8
	U8 maxPlayers:4;
	bool bGlobalLighting:1;		// turn on the standard lights
	bool bScriptUIControl:1;	// true if the script has disabled the mouse/keyboard UI
	U8  allyMask[MAX_PLAYERS];		// two dimensional array
	U8 colorAssignment[MAX_PLAYERS_PLUS_1];	// valid from 1 to 8
	U8 visibilityMask[MAX_PLAYERS];	// two dimensional array
	U32 missionID; 

	struct PlayerTechLevel
	{
		enum TECHLEVEL
		{
			LEVEL0,
			LEVEL1,
			LEVEL2,
			LEVEL3,
			LEVEL4,
			LEVEL5
		};

		TECHLEVEL engine:4;
		TECHLEVEL hull:4;
		TECHLEVEL supplies:4;
		TECHLEVEL targeting:4;
		TECHLEVEL damage:4;
		TECHLEVEL shields:4;
		TECHLEVEL sensors:4;

		//class specific
		TECHLEVEL fighter:4;
		TECHLEVEL tanker:4;
		TECHLEVEL tender:4;
		TECHLEVEL fleet:4;

	} playerTechLevel[MAX_PLAYERS_PLUS_1][NUM_RACES_PLUS_1];

	TECHNODE techNode[MAX_PLAYERS_PLUS_1];
	TECHNODE workingTechNode[MAX_PLAYERS_PLUS_1];
	TECHNODE availableTechNode;
	U32 gas[MAX_PLAYERS_PLUS_1];
	U32 gasMax[MAX_PLAYERS_PLUS_1];
	U32 metal[MAX_PLAYERS_PLUS_1];
	U32 metalMax[MAX_PLAYERS_PLUS_1];
	U32 crew[MAX_PLAYERS_PLUS_1];
	U32 crewMax[MAX_PLAYERS_PLUS_1];
	U32 totalCommandPts[MAX_PLAYERS_PLUS_1];
	U32 usedCommandPts[MAX_PLAYERS_PLUS_1];
	U32 maxComPts[MAX_PLAYERS_PLUS_1];				// max command points

	struct GAMESTATS
	{
		// allignment!!! do not screw with this
		U32 metalGained:20;
		U32 numUnitsBuilt:12;

		U32 gasGained:17;
		U32	numUnitsDestroyed:12;
		U32	numAdmiralsBuilt:3;

		U32 crewGained:20;	
		U32 numUnitsLost:12;

		U32 numPlatformsBuilt:12;
		U32 numPlatformsDestroyed:12;
		U32	numJumpgatesControlled:8;

		U32 numPlatformsLost:12;
		U32 numUnitsConverted:10;
		U32	numPlatformsConverted:10;

		U32	numResearchComplete:10;
		U32 percentSystemsExplored:8;	// 0 to 255
	};
	GAMESTATS gameStats[MAX_PLAYERS_PLUS_1];
	
	U8  gameScores[MAX_PLAYERS_PLUS_1];

	U32 updateCount;				// runs at 30 times / second
	U32 lastStreamID;
	U32 lastTeletypeID;

	struct OBJECTIVES
	{
		int index;
		U32 mission_name_stringID;
		U32 overview_stringID;
		U32 objective_stringID[MAX_MISSION_OBJECTIVES];
		
		enum ObjectiveState
		{
			Pending,
			Complete,
			Failed
		} state[MAX_MISSION_OBJECTIVES];
		
		bool bObjectiveSecondary[MAX_MISSION_OBJECTIVES];
	} objectives;

	M_STRING scriptLibrary;			// name of the DLL
	M_STRING baseTerrainMap;		//name of qmission to load all terrain from
	CQGAME gameSettings;
};

//-------------------------------------------------------------------

#endif

