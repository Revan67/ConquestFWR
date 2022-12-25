#ifndef DSHIPLAUNCH_H
#define DSHIPLAUNCH_H
//--------------------------------------------------------------------------//
//                                                                          //
//                                 DShipLaunch.h                                //
//                                                                          //
//                  COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/DInclude/DShipLaunch.h 3     4/20/99 6:34p Jasony $
*/			    
//--------------------------------------------------------------------------//

#ifndef DLAUNCHER_H
#include "DLauncher.h"
#endif

//--------------------------------------------------------------------------//
//
struct BT_SHIPLAUNCH : BASE_LAUNCHER
{
	char hardpoint[HP_PATH];
	char animation[GT_PATH];
};

//--------------------------------------------------------------------------//
//

struct SHIPLAUNCH_SAVELOAD
{
	U32 dwTargetID, dwOwnerID;
	Vector targetPos;
	SINGLE relYaw;				// valid after calling swivelToTarget()
	SINGLE distanceToTarget;	// valid after calling swivelToTarget()
	U32 attacking;		// 1 if we are trying to shoot at position, 2 if shooting an object
	SINGLE refireDelay; // if <= 0, fire away!
	SINGLE currentRot;
	Vector offsetVector;	// offset from target position
};

//--------------------------------------------------------------------------//
//
#endif