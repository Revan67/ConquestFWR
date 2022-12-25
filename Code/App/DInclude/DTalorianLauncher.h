#ifndef DTALORIANLAUNCHER_H
#define DTALORIANLAUNCHER_H
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

#define MAX_SHIELD_VICTIMS 12

//--------------------------------------------------------------------------//
// 
//
struct BT_TALORIAN_LAUNCHER : BASE_LAUNCHER
{
};

//--------------------------------------------------------------------------//
//
struct TALORIAN_LAUNCHER_SAVELOAD
{
	U32 numTargets;
	U32 targetIDs[MAX_SHIELD_VICTIMS];

	bool bShildOn;
};

//--------------------------------------------------------------------------//
//
#endif