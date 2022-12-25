#ifndef DWORMHOLELAUNCHER_H
#define DWORMHOLELAUNCHER_H
//--------------------------------------------------------------------------//
//                                                                          //
//                              DWormholeLauncher.h                           //
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

#define MAX_WORM_VICTIMS 21

//--------------------------------------------------------------------------//
// 
//
struct BT_WORMHOLE_LAUNCHER : BASE_LAUNCHER
{
	SINGLE_TECHNODE techNode;
	SINGLE damagePerSec;
};

//--------------------------------------------------------------------------//
//
struct WORMHOLE_LAUNCHER_SAVELOAD
{
	U32 numTargets;
	U32 targetsLeft;
	U32 targetIDs[MAX_WORM_VICTIMS];
	U32 workingID;
	SINGLE lastChanceTimer;

	GRIDVECTOR targetPos;
	U32 targetSystemID;

	enum _modes
	{
		WORM_IDLE,
		WORM_WAITING_FOR_SYNC,
		WORM_WORKING,
		WORM_DONE
	}mode;
};

//--------------------------------------------------------------------------//
//
#endif