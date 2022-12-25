#ifndef DCLOAKLAUNCHER_H
#define DCLOAKLAUNCHER_H
//--------------------------------------------------------------------------//
//                                                                          //
//                              DCloakLauncher.h                           //
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
struct BT_CLOAK_LAUNCHER : BASE_LAUNCHER
{
	SINGLE cloakSupplyUse;
	SINGLE cloakShutoff;
	SINGLE_TECHNODE techNode;
};

//--------------------------------------------------------------------------//
//

struct CLOAK_LAUNCHER_SAVELOAD
{
	bool bCloakEnabled:1;
	bool bPrepareToggle:1;
	SINGLE cloakSupplyCount;
};

//--------------------------------------------------------------------------//
//
#endif