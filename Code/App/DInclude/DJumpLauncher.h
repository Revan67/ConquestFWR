#ifndef DJUMPLAUNCHER_H
#define DJUMPLAUNCHER_H
//--------------------------------------------------------------------------//
//                                                                          //
//                              DWormholeLauncher.h                           //
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

#ifndef DWEAPON_H
#include "DWeapon.h"
#endif

//--------------------------------------------------------------------------//
// 
//
struct BT_JUMP_LAUNCHER : BASE_LAUNCHER
{
	char jumpParticle[GT_PATH];
};

//--------------------------------------------------------------------------//
//
struct JUMP_LAUNCHER_SAVELOAD
{
	U32 workingID;
	GRIDVECTOR targetPos;
	enum JumpMode
	{
		NO_JUMP,
		JUMPING_OUT,
		JUMPING_IN,
	}jumpMode;
	SINGLE timer;
};

//--------------------------------------------------------------------------//
//
#endif