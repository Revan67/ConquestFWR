#ifndef DMOONRESOURCELAUNCHER_H
#define DMOONRESOURCELAUNCHER_H
//--------------------------------------------------------------------------//
//                                                                          //
//                              DMoonResourceLauncher.h                     //
//                                                                          //
//                  COPYRIGHT (C) 2004 Warthog, INC.                        //
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

//--------------------------------------------------------------------------//
// 
//
struct BT_MOON_RESOURCE_LAUNCHER : BASE_LAUNCHER
{
	SINGLE oreRegenRate;
	SINGLE gasRegenRate;
	SINGLE crewRegenRate;
};

//--------------------------------------------------------------------------//
//
struct MOON_RESOURCE_LAUNCHER_SAVELOAD
{
	bool bHaveBoostedPlanet;
};

//--------------------------------------------------------------------------//
//
#endif