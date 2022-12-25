#ifndef DBARRAGELAUNCHER_H
#define DBARRAGELAUNCHER_H
//--------------------------------------------------------------------------//
//                                                                          //
//                         DBarrageLauncher.h                               //
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
struct BT_BARRAGE_LAUNCHER : BASE_LAUNCHER
{
	SINGLE damagePerSec;
	SINGLE rangeRadius;
	char flashType[GT_PATH];
};

//--------------------------------------------------------------------------//
//
struct BARRAGE_LAUNCHER_SAVELOAD
{
};

//--------------------------------------------------------------------------//
//
#endif