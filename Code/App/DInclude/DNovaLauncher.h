#ifndef DNOVALAUNCHER_H
#define DNOVALAUNCHER_H
//--------------------------------------------------------------------------//
//                                                                          //
//                              DNovaLauncher.h                             //
//                                                                          //
//                  COPYRIGHT (C) 2004 WARTHOG, INC.                        //
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
struct BT_NOVA_LAUNCHER : BASE_LAUNCHER
{
	SINGLE chargeTime;
	char novaExplosion[GT_PATH];
};

//--------------------------------------------------------------------------//
//
struct NOVA_LAUNCHER_SAVELOAD
{
	bool bNovaOn;
	SINGLE timer;
};
//--------------------------------------------------------------------------//
//
#endif