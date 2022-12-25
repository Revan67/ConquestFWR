#ifndef DTURRET_H
#define DTURRET_H
//--------------------------------------------------------------------------//
//                                                                          //
//                                 DTurret.h                                //
//                                                                          //
//                  COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/DInclude/DTurret.h 11    9/05/00 6:56p Jasony $
*/			    
//--------------------------------------------------------------------------//

#ifndef DLAUNCHER_H
#include "DLauncher.h"
#endif

//--------------------------------------------------------------------------//
// cannot attack fighters!
//
struct BT_TURRET : BASE_LAUNCHER
{
	union TurretJointInfo
	{
		SINGLE angVelocity;
		SINGLE maxAngDeflection;
	} info;
	
	char animation[GT_PATH];
	char hardpoint[HP_PATH];
	char joint[HP_PATH];
	char animMuzzleFlash[GT_PATH];
	U32 muzzleFlashWidth;
	SINGLE muzzleFlashTime;
	struct _colorMod
	{
		U8 red;
		U8 green;
		U8 blue;
	}colorMod;
	struct WarmUpBlast
	{
		char blastType[GT_PATH];
		SINGLE triggerTime;
	} warmUpBlast;
};

//--------------------------------------------------------------------------//
//
struct BASE_TURRET_SAVELOAD
{
	U32 dwTargetID;
	Vector targetPos;
	SINGLE relYaw;				// valid after calling swivelToTarget()
	SINGLE refireDelay; // if <= 0, fire away!
	SINGLE currentRot;
	Vector offsetVector;	// offset from target position
};

struct TURRET_SAVELOAD : BASE_TURRET_SAVELOAD
{
	RangeFinderSaveLoad rangeFinderSaveLoad;
};

struct BT_EFFECT_LAUNCHER : BASE_LAUNCHER
{
	U32 weaponDamage;
	SINGLE weaponFireDelay;
	SINGLE weaponVelocity;
};

//--------------------------------------------------------------------------//
//
struct BASE_EFFECT_LAUNCHER_SAVELOAD
{
	U32 dwTargetID;
	Vector targetPos;
	SINGLE refireDelay; // if <= 0, fire away!
};

struct EFFECT_LAUNCHER_SAVELOAD : BASE_EFFECT_LAUNCHER_SAVELOAD
{
};

//--------------------------------------------------------------------------//
//
#endif