#ifndef DFIGHTERWING_H
#define DFIGHTERWING_H
//--------------------------------------------------------------------------//
//                                                                          //
//                              DFighterWing.h                              //
//                                                                          //
//                  COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/DInclude/DFighterWing.h 10    9/19/00 2:56a Jasony $
*/			    
//--------------------------------------------------------------------------//

#ifndef DLAUNCHER_H
#include "DLauncher.h"
#endif

#ifndef DMTECHNODE_H
#include "DMTechNode.h"
#endif

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
struct BT_FIGHTER_WING : BASE_LAUNCHER
{
	SINGLE baseAirAccuracy;		// between 0 and 1.0
	SINGLE baseGroundAccuracy;  // between 0 and 1.0
	U32	   maxFighters;			// maximum number of fighters for launcher
	U32	   maxCapFighters;		// number of fighters in orbit
	U32	   maxWingFighters;		// number of fighters in a wing
	SINGLE minLaunchPeriod;		// time between launches of fighter wings
	U32	   costOfNewFighter;		
	U32	   costOfRefueling;		// fixed cost for refueling a fighter that used its weapon

	bool bSpecialWeapon;		// will only lauce when it recieves a special attack
	SINGLE_TECHNODE neededTech;

	char animation[GT_PATH];	// bay doors opening (optional)
	char hardpoint[HP_PATH];	// place where fighters appear
};
//--------------------------------------------------------------------------//
//
struct FIGHTERWING_SAVELOAD
{
	U32  firstFighterID;
	bool bBayDoorOpen:1;
	bool bRecallingFighters:1;
	bool bAutoAttack:1;
	bool bSpecialAttack:1;
	bool bSpecialWeapon:1;
	bool bTakeoverInProgress:1;
	S32	 createTime;			// time (ticks) left to generate a new fighter
	S32	 minLaunchTime;		// time (ticks) left between launches of fighter wings
	S32	 bayDoorCloseTime;	// time to wait until closing the door
};

#endif
