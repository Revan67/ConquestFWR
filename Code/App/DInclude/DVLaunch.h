#ifndef DVLAUNCH_H
#define DVLAUNCH_H
//--------------------------------------------------------------------------//
//                                                                          //
//                               DVLaunch.h                                //
//                                                                          //
//                  COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/DInclude/DVLaunch.h 3     12/02/99 5:54p Tmauer $
*/			    
//--------------------------------------------------------------------------//

#ifndef DLAUNCHER_H
#include "DLauncher.h"
#endif

#ifndef DMTECHNODE_H
#include "DMTechNode.h"
#endif

#define MAX_VERTICAL_TUBES 10

//--------------------------------------------------------------------------//
//

struct BT_VERTICAL_LAUNCH : BASE_LAUNCHER
{
	struct _salvo
	{
		SINGLE_TECHNODE techNeed;
		U32 salvo;		// number of missiles to launch in one salvo (ie. less than max num of tubes)
	}upgrade[4];
	SINGLE miniRefire;	// time between missile launches
	char hardpoint[MAX_VERTICAL_TUBES][HP_PATH];
};

//--------------------------------------------------------------------------//
//

struct VLAUNCH_SAVELOAD
{
	U32 dwTargetID, dwOwnerID;
	Vector targetPos;
	U32 attacking;		// 1 if we are trying to shoot at position, 2 if shooting an object
	SINGLE refireDelay; // if <= 0, fire away!
	S32 miniDelay;
	S32 salvo;
};

//--------------------------------------------------------------------------//
//

#endif
