#ifndef DTEMPHQLAUNCHER_H
#define DTEMPHQLAUNCHER_H
//--------------------------------------------------------------------------//
//                                                                          //
//                              DTempHQLauncher.h                           //
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
struct BT_TEMPHQ_LAUNCHER : BASE_LAUNCHER
{
	SINGLE chargeRegenRate;
	SINGLE chargeUseRate;
	SINGLE maxCharge;
};

//--------------------------------------------------------------------------//
//
struct TEMPHQ_LAUNCHER_SAVELOAD
{
	SINGLE chargeTimer;
	U32 charge;
};

//--------------------------------------------------------------------------//
//
#endif