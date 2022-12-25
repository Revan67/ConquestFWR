#ifndef DBUFFLAUNCHER_H
#define DBUFFLAUNCHER_H
//--------------------------------------------------------------------------//
//                                                                          //
//                              DTalorianLauncher.h                           //
//                                                                          //
//                  COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header:
*/			    
//--------------------------------------------------------------------------//

#ifndef DLAUNCHER_H
#include "DLauncher.h"
#endif

#ifndef DMTECHNODE_H
#include <DMTechNode.h>
#endif

#ifndef DWEAPON_H
#include "DWeapon.h"
#endif

#define MAX_BUFF_VICTIMS 12

//--------------------------------------------------------------------------//
// 
//
struct BT_BUFF_LAUNCHER : BASE_LAUNCHER
{
	SINGLE rangeRadius;
	enum BuffType
	{
		SINUATOR_EFFECT,
		STASIS_EFFECT,
		REPELLENT_EFFECT,
		DESTABILIZER_EFFECT,
		TALLORIAN_EFFECT,
		AGIES_EFFECT,
		RESEARCH_EFFECT,
		CONSTRUCTION_EFFECT,
		SHIELD_JAM_EFFECT,
		WEAPON_JAM_EFFECT,
		SENSOR_JAM_EFFECT,
	}buffType;
	enum TargetType
	{
		ALLIES_ONLY,
		ALLIES_SHIPS_ONLY,
		ALLIES_PLATS_ONLY,
		ALLIES_PLANET_ONLY,
		ENEMIES_ONLY,
		ENEMIES_SHIPS_ONLY,
		ENEMIES_PLATS_ONLY,
		ALL_TARGETS,
		SHIPS_ONLY,
		PLATS_ONLY,
	}targetType;
	enum SupplyUseType
	{
		SU_PLATFORM_STANDARD = 0,
		SU_SHIP_SUPPLIES_TOGGLE,
	}supplyUseType;

	char visualName[GT_PATH];
};

//--------------------------------------------------------------------------//
//
struct BUFF_LAUNCHER_SAVELOAD
{
	U32 numTargets;
	U32 targetIDs[MAX_BUFF_VICTIMS];
	bool bToggleOn;
};
//--------------------------------------------------------------------------//
// 
//
struct BT_SYSTEM_BUFF_LAUNCHER : BASE_LAUNCHER
{
	enum BuffType
	{
		INTEL_CENTER_EFFECT,
	}buffType;
	enum TargetType
	{
		ALL_PLAYERS,
		ALLIES_ONLY,
		ENEMIES_ONLY,
	}targetType;
};

//--------------------------------------------------------------------------//
//
struct SYSTEM_BUFF_LAUNCHER_SAVELOAD
{
};

//--------------------------------------------------------------------------//
//
#endif