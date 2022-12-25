#ifndef DREPAIRPLAT_H
#define DREPAIRPLAT_H
//--------------------------------------------------------------------------//
//                                                                          //
//                                DRepairPlat.h                                //
//                                                                          //
//                  COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/DInclude/DRepairPlat.h 13    10/17/00 6:15p Tmauer $
*/			    
//--------------------------------------------------------------------------//

#ifndef DPLATSAVE_H
#include "DPlatSave.h"
#endif

#ifndef DPLATFORM_H
#include "DPlatform.h"
#endif

//----------------------------------------------------------------
//
struct BT_PLAT_REPAIR_DATA : BASE_PLATFORM_DATA
{
	SINGLE supplyPerSecond;
	U32 repairRate;
	U32 supplyRate;
	SINGLE supplyRange;
	char repairDroneType[GT_PATH];
	char repairDroneHardpoint[GT_PATH];
};
//----------------------------------------------------------------
//
#ifndef _ADB
struct REPAIRPLAT_INIT : PLATFORM_INIT<BT_PLAT_REPAIR_DATA>
{
	PARCHETYPE repairDroneType;

	~REPAIRPLAT_INIT (void);					// free archetype references
};
#endif
//--------------------------------------------------------------------------//
//
struct BASE_REPAIRPLAT_SAVELOAD
{
	U32 workingID;
	U32 potentialWorkingID;

	U32 repairTargetID;
	enum REPAIR_MODE
	{
		REP_NONE,
		REP_WAIT_FOR_DOCK,
		REP_REPAIRING
	} mode;

	S32 oldHullPoints;

	SINGLE supplyTimer;

	SINGLE upgradeTimeSpent;
	U32 upgradeID;

	bool bDockLocked:1;
	bool bUpgradeDelay:1;
	bool bUpgrading:1;
	bool bTakenCost:1;
};
//--------------------------------------------------------------------------//
//
struct REPAIRPLAT_SAVELOAD : BASE_PLATFORM_SAVELOAD
{
	BASE_REPAIRPLAT_SAVELOAD repairPlatSaveload;

	TRANSFORM repairTransform;
};

#endif
