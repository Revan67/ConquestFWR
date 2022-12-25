#ifndef DRECONLAUNCH_H
#define DRECONLAUNCH_H
//--------------------------------------------------------------------------//
//                                                                          //
//                              DReconLaunch.h                              //
//                                                                          //
//                  COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/DInclude/DReconLaunch.h 9     9/05/00 5:24p Tmauer $
*/			    
//--------------------------------------------------------------------------//

#ifndef DLAUNCHER_H
#include "DLauncher.h"
#endif

#ifndef DMTECHNODE_H
#include "DMTechNode.h"
#endif

//--------------------------------------------------------------------------//
// cannot attack fighters!
//
struct BT_RECON_LAUNCH : BASE_LAUNCHER
{
	char animation[GT_PATH];
	char hardpoint[HP_PATH];
	SINGLE animTime;
	SINGLE effectDuration;
	SFX::ID warmupSound;
	SINGLE_TECHNODE neededTech;
	UNIT_SPECIAL_ABILITY specialAbility;
	bool bWormWeapon;
	bool selfTarget;
};

//--------------------------------------------------------------------------//
//

struct RECON_LAUNCH_SAVELOAD
{
	Vector targetPos;
	U32 targetSystemID;
	U32 attacking;		// 1 if we are trying to shoot at position, 2 if shooting an object
	SINGLE refireDelay; // if <= 0, fire away!
	U32 probeID;
	U32 targetID;
	bool bKillProbe:1;
};

//--------------------------------------------------------------------------//
//
#endif