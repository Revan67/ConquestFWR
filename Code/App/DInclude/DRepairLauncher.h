#ifndef DREPAIRLAUNCHER_H
#define DREPAIRLAUNCHER_H
//--------------------------------------------------------------------------//
//                                                                          //
//                              DRepairLauncher.h                           //
//                                                                          //
//                  COPYRIGHT (C) 2004 Warthog , INC.                       //
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

//--------------------------------------------------------------------------//
// 
//
struct BT_REPAIR_LAUNCHER : BASE_LAUNCHER
{
	SINGLE rangeRadius;
	SINGLE repairRate;
	SINGLE repairCostPerPoint;
};

//--------------------------------------------------------------------------//
//
struct REPAIR_LAUNCHER_SAVELOAD
{
};

//--------------------------------------------------------------------------//
//
#endif