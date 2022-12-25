#ifndef DFLAGSHIP_H
#define DFLAGSHIP_H
//--------------------------------------------------------------------------//
//                                                                          //
//                               DFlagShip.h                               //
//                                                                          //
//                  COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/DInclude/DFlagship.h 22    7/19/01 1:43p Tmauer $
*/			    
//--------------------------------------------------------------------------//

#ifndef DSPACESHIP_H
#include "DSpaceship.h"
#endif

#ifndef DSHIPSAVE_H		// don't need this
#include "DShipSave.h"
#endif

#ifndef DFABSAVE_H		
#include "DFabSave.h"
#endif

#ifndef DHOTBUTTONTEXT_H
#include "DHotButtonText.h"
#endif

#ifndef DRESEARCH_H
#include "DResearch.h"
#endif

struct ShipFilters
{
#ifndef _ADB //we need to be able to use the FormationFilter as a bit mask
	union
	{
#endif
		FORMATION_FILTER positive_filter;
#ifndef _ADB
		DWORD positiveFilterBits;
	};
#endif
#ifndef _ADB //we need to be able to use the FormationFilter as a bit mask
	union
	{
#endif
		FORMATION_FILTER negative_filter;
#ifndef _ADB
		DWORD negativeFilterBits;
	};
#endif
	U32 max;
	U32 min;
	bool bOverflowOnly;
};

#define MAX_SHIP_FILTERS 6
#define MAX_GROUP_DEFS 8

struct FleetGroupDef
{
	ShipFilters filters[MAX_SHIP_FILTERS];
	bool bActive:1;
	bool bOerflowCreation:1;
	U32 parentGroup;//the id of the parent group
	U32 parentGroupNumber;//the number that need to be made
	U32 priority;
	U32 creationNumber;

	enum RelativeType 
	{
		RT_CENTER, 
		RT_GROUP, 
	} relativeTo;
	U32 relativeGroupID;
	enum Relation 
	{
		RE_EDGE,
		RE_CENTER,
	} relation ;

	S32 relDirX;
	S32 relDirY;

	struct AdvancedPlacement
	{
		enum PlacementType
		{
			PT_NORMAL,
			PT_LINE,
			PT_LINE_END,
			PT_V,
		} placementType;
		S32 placementDirX;
		S32 placementDirY;
	}advancedPlacement;

	enum AIType
	{
		NORMAL,
		ROVER,
		SPOTTER,
		STRIKER
	}aiType;
};

#define NUM_FORMATION_BONUS_SHIPS 5
#define NUM_ADMIRAL_BONUS_SHIPS 5

struct AdmiralBonuses
{
	struct BonusValues
	{
		SINGLE damage;
		SINGLE supplyUsage;
		SINGLE rangeModifier;
		SINGLE speed;
		SINGLE sensors;
		SINGLE defence;
		SINGLE platformDamage;
		struct RaceDamage
		{
			SINGLE terran;
			SINGLE mantis;
			SINGLE celareon;
			SINGLE vyrium;
		}hatedRaceDamage;
		struct ArmorDamage
		{
			SINGLE noArmor;
			SINGLE lightArmor;
			SINGLE mediumArmor;
			SINGLE heavyArmor;
		}hatedArmorDamage;
	}baseBonuses, favoredShipBonus, favoredArmor;
	M_OBJCLASS bonusShips[NUM_ADMIRAL_BONUS_SHIPS];
	bool dNoArmorFavored:1;
	bool dLightArmorFavored:1;
	bool dMediumArmorFavored:1;
	bool dHeavyArmorFavored:1;;
};

struct BT_FORMATION : BASIC_DATA
{
	struct ButtonInfo
	{
		U32 baseButton;		// index into shape table
		HBTNTXT::BUTTON_TEXT tooltip;
		HBTNTXT::HOTBUTTONINFO helpBox;//actualy the status text
		HBTNTXT::MULTIBUTTONINFO hintBox;//this is the hint box
	}buttonInfo;
	FleetGroupDef groups[MAX_GROUP_DEFS];
	bool bFreeForm;//setting this bit makes this formation behave in classic conquest style.
	U32 moveTargetRange;
	U32 spotterRange;
	enum CloakUsage
	{
		CU_NONE,
		CU_ALL,
		CU_RECON_ONLY,
		CU_RECON_AND_SPOTTERS,
	}cloakUsage;
	bool bAdmiralAttackControl;
	AdmiralBonuses formationBonuses;
	struct FormationSpecials
	{
		bool bRepairWholeFleet:1;
	}formationSpecials;
};
//----------------------------------------------------------------
//
#define MAX_COMMAND_FORMATIONS 3

struct BT_COMMAND_KIT: BASE_RESEARCH_DATA
{
	AdmiralBonuses kitBonuses;

	struct ButtonInfo
	{
		U32 baseButton;		// index into shape table
		HBTNTXT::BUTTON_TEXT tooltip;
		HBTNTXT::HOTBUTTONINFO helpBox;//actualy the status text
		HBTNTXT::MULTIBUTTONINFO hintBox;//this is the hint box
	}buttonInfo;
	SINGLE_TECHNODE dependancy;

	char formations[MAX_COMMAND_FORMATIONS][GT_PATH];
};
//----------------------------------------------------------------
//
#define MAX_GUNBOAT_LAUNCHERS 5
#define MAX_FORMATIONS 6
#define MAX_COMMAND_KITS 14
#define MAX_KNOWN_KITS 2

struct BT_FLAGSHIP_DATA : BASE_SPACESHIP_DATA
{	
	SINGLE attackRadius;
	AdmiralBonuses admiralBonueses;

	char commandKits[MAX_COMMAND_KITS][GT_PATH];
	char startingFormations[2][GT_PATH];

	struct _toolbarInfo
	{
		U32 baseImage;
		HBTNTXT::BUTTON_TEXT buttonText;
		HBTNTXT::HOTBUTTONINFO buttonStatus;
		HBTNTXT::MULTIBUTTONINFO buttonHintbox;
	}toolbarInfo;

	U32 maxQueueSize;
};							
//----------------------------------------------------------------
//
#ifndef _ADB
struct FLAGSHIP_INIT : SPACESHIP_INIT<BT_FLAGSHIP_DATA>
{
	// archetype init stuff
};
#endif

//-------------------------------------------------------------------------------
//
struct BASE_FLAGSHIP_SAVELOAD
{
	U32 dockAgentID;
	__hexview U32 dockshipID;
	__hexview U32 targetID;
	S32 idleTimer;
	enum ADMIRAL_MODE
	{
		NOMODE,
		MOVING,
		DEFENDING,
		ATTACKING
	} mode;
	bool bAttached;
	U8   numDeadShips;		// number of ships that have died since that user command
	struct DMESSAGES		// damage messages
	{
		bool    inTrouble:1;
		bool	damage50:1;
		bool	damage75:1;
	} dmessages;

	struct SMESSAGES		// supply messages
	{
		bool	suppliesLow:1;
		bool	suppliesOut:1;
	} smessages;

	S32 forecastTimer;
	enum FORECAST
	{
		NOFORECAST,
		GOOD,
		BAD,
		UGLY
	} lastForecast;
	ADMIRAL_TACTIC admiralTactic;
	U32 formationID;
	U32 knownFormations[MAX_FORMATIONS];
	bool bHaveFormationPost:1;
	bool bRovingAllowed:1;
	bool flipFormationPost:1;//flip the Y direction on all groups
	NETGRIDVECTOR formationPost;
	S8 formationDirX;
	S8 formationDirY;
	NETGRIDVECTOR targetFormationPost;
	S8 targetFormationDirX;
	S8 targetFormationDirY;
	U32 formationTargetID;//weapon target
	U32 moveDoneHint;//used by formation as a hint when "most" of the fleet is in position
	U32 targetFormationGateID;

	U32 commandKitsArchID[MAX_KNOWN_KITS];//archID of the known command kits
	SINGLE buildTime;
	enum AdmiralHotkey {NO_HOTKEY = 0, ADMIRAL_F1 = 1, ADMIRAL_F2, ADMRIAL_F3, ADMIRAL_F4, ADMIRAL_F5, ADMIRAL_F6 } admiralHotkey;

};
//-------------------------------------------------------------------------------
//
struct FLAGSHIP_SAVELOAD : SPACESHIP_SAVELOAD 
{
	BASE_FLAGSHIP_SAVELOAD flagshipSaveLoad;
	BUILDQUEUE_SAVELOAD fab_SL;
};


//-------------------------------------------------------------------------------
//------------------------------END DFlagShip.h----------------------------------
//-------------------------------------------------------------------------------
//
#endif
