#ifndef DGUNPLAT_H
#define DGUNPLAT_H
//--------------------------------------------------------------------------//
//                                                                          //
//                                DGunPlat.h                                //
//                                                                          //
//                  COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/DInclude/DGunPlat.h 13    5/07/01 9:21a Tmauer $
*/			    
//--------------------------------------------------------------------------//

#ifndef DPLATSAVE_H
#include "DPlatSave.h"
#endif

#ifndef DPLATFORM_H
#include "DPlatform.h"
#endif

#define MAX_GUNPLAT_LAUNCHERS 4

//----------------------------------------------------------------
//
struct BT_PLAT_GUN : BASE_PLATFORM_DATA
{
	SINGLE outerWeaponRange;
	bool bNoLineOfSight;		// true means we don't line of sight
	char launcherType[MAX_GUNBOAT_LAUNCHERS][GT_PATH];
	char specialLauncherType[GT_PATH];
};
//----------------------------------------------------------------
//
#ifndef _ADB
struct GUNPLAT_INIT : PLATFORM_INIT<BT_PLAT_GUN>
{
	PARCHETYPE launchers[MAX_GUNBOAT_LAUNCHERS];
	PARCHETYPE specialLauncher;

	~GUNPLAT_INIT (void);					// free archetype references
};
#endif
//--------------------------------------------------------------------------//
//
struct BASE_GUNPLAT_SAVELOAD
{
	__hexview U32 dwTargetID;
	U32 attackAgentID;
	U32 launcherAgentID;
	UNIT_STANCE unitStance;
	enum FighterStance fighterStance;
	U8 launcherID;
	SINGLE buildTimeSpent;
	U32 upgradeID;
	U32 workingID;
	bool bUserGenerated:1;    // is the current target selected by the user or the game AI?
	bool isPreferredTarget:1;		  // we prefer the targets to be gunboats or troopships	
	bool bUpgrading:1;
	bool bDelayed:1;
	bool bSpecialAttack:1;
};
//--------------------------------------------------------------------------//
//
struct GUNPLAT_SAVELOAD : BASE_PLATFORM_SAVELOAD
{
	BASE_GUNPLAT_SAVELOAD gunplatSaveload;
};

#endif
