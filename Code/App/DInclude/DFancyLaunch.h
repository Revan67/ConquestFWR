#ifndef DFANCYLAUNCH_H
#define DFANCYLAUNCH_H
//--------------------------------------------------------------------------//
//                                                                          //
//                              DFancyLaunch.h                              //
//                                                                          //
//                  COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/DInclude/DFancyLaunch.h 13    7/06/00 8:07p Tmauer $
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
struct BT_FANCY_LAUNCH : BASE_LAUNCHER
{
/*	union JointInfo
	{
		SINGLE angVelocity;
		SINGLE maxAngDeflection;
	} info;*/
	
	char animation[GT_PATH];
	char hardpoint[HP_PATH];
	SINGLE animTime;
	SINGLE effectDuration;
	SFX::ID warmupSound;
	SINGLE_TECHNODE neededTech;
	bool bSpecialWeapon:1;
	bool bTargetRequired:1;
	bool bWormHole:1;
	UNIT_SPECIAL_ABILITY specialAbility;
//	char joint[HP_PATH];
};

//--------------------------------------------------------------------------//
//

struct FANCY_LAUNCH_SAVELOAD
{
	U32 dwTargetID;
	Vector targetPos;
	U32 attacking;		// 1 if we are trying to shoot at position, 2 if shooting an object
	SINGLE refireDelay; // if <= 0, fire away!
	SINGLE currentRot;
	Vector offsetVector;	// offset from target position
};

//--------------------------------------------------------------------------//
//
#endif