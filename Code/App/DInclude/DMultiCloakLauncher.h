#ifndef DMULTICLOAKLAUNCHER_H
#define DMULTICLOAKLAUNCHER_H
//--------------------------------------------------------------------------//
//                                                                          //
//                              DMultiCloakLauncher.h                           //
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

//--------------------------------------------------------------------------//
// cannot attack fighters!
//
struct BT_MULTICLOAK_LAUNCHER : BASE_LAUNCHER
{
	SINGLE cloakSupplyUse;
	SINGLE cloakShutoff;
	SINGLE targetSupplyCostPerHull;
	SINGLE_TECHNODE techNode;
};

//--------------------------------------------------------------------------//
//

struct MULTICLOAK_LAUNCHER_SAVELOAD
{
	SINGLE cloakSupplyCount;
	U32 cloakTargetID;
	U32 decloakTime;
	bool bCloakingTarget:1;
	bool bCloakEnabled:1;
	bool bPrepareToggle:1;
};

//--------------------------------------------------------------------------//
//
#endif