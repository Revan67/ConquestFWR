#ifndef DARTILERYLAUNCHER_H
#define DARTILERYLAUNCHER_H
//--------------------------------------------------------------------------//
//                                                                          //
//                              DArtileryLauncher.h                           //
//                                                                          //
//                  COPYRIGHT (C) 2004 Warthog, INC.               //
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
struct BT_ARTILERY_LAUNCHER : BASE_LAUNCHER
{
	SINGLE areaRadius;
	U32 damagePerSec;
	bool bSpecial;
	char explosionType[GT_PATH];
	char flashType[GT_PATH];
};

//--------------------------------------------------------------------------//
//
struct ARTILERY_LAUNCHER_SAVELOAD
{
	GRIDVECTOR targetPos;
	SINGLE timer;
	bool bShooting;
};

//--------------------------------------------------------------------------//
//
#endif