#ifndef DPINGLAUNCHER_H
#define DPINGLAUNCHER_H
//--------------------------------------------------------------------------//
//                                                                          //
//                              DPingLauncher.h                           //
//                                                                          //
//                  COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.               //
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

//--------------------------------------------------------------------------//
//
struct BT_PING_LAUNCHER : BASE_LAUNCHER
{
	SINGLE_TECHNODE techNode;
};

//--------------------------------------------------------------------------//
//

struct PING_LAUNCHER_SAVELOAD
{
};

//--------------------------------------------------------------------------//
//
#endif