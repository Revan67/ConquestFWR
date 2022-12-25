#ifndef DJUMPPLAT_H
#define DJUMPPLAT_H
//--------------------------------------------------------------------------//
//                                                                          //
//                                DJumpPlat.h                                //
//                                                                          //
//                  COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/DInclude/DJumpPlat.h 4     6/07/00 11:11p Tmauer $
*/			    
//--------------------------------------------------------------------------//

#ifndef DPLATSAVE_H
#include "DPlatSave.h"
#endif

#ifndef DPLATFORM_H
#include "DPlatform.h"
#endif

//----------------------------------------------------------------
//
struct BT_PLAT_JUMP_DATA : BASE_PLATFORM_DATA
{
};
//----------------------------------------------------------------
//
#ifndef _ADB
struct JUMPPLAT_INIT : PLATFORM_INIT<BT_PLAT_JUMP_DATA>
{
	// nothing yet?

	~JUMPPLAT_INIT (void);					// free archetype references
};
#endif
//--------------------------------------------------------------------------//
//
struct BASE_JUMPPLAT_SAVELOAD
{
	U32 jumpGateID;
	bool bOwnGate:1;
	bool bLockGate:1;
};
//--------------------------------------------------------------------------//
//
struct JUMPPLAT_SAVELOAD : BASE_PLATFORM_SAVELOAD
{
	BASE_JUMPPLAT_SAVELOAD jumpPlatSaveload;
};

#endif
