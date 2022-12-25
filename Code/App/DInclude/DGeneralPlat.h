#ifndef DGENERALPLAT_H
#define DGENERALPLAT_H
//--------------------------------------------------------------------------//
//                                                                          //
//                                DGeneralPlat.h                                //
//                                                                          //
//                  COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/DInclude/DGeneralPlat.h 2     12/03/99 4:49p Jasony $
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
struct BT_PLAT_GENERAL_DATA : BASE_PLATFORM_DATA
{
};
//----------------------------------------------------------------
//
#ifndef _ADB
struct GENERALPLAT_INIT : PLATFORM_INIT<BT_PLAT_GENERAL_DATA>
{
	// nothing yet?

	~GENERALPLAT_INIT (void);					// free archetype references
};
#endif
//--------------------------------------------------------------------------//
//
struct BASE_GENERALPLAT_SAVELOAD
{
};
//--------------------------------------------------------------------------//
//
struct GENERALPLAT_SAVELOAD : BASE_PLATFORM_SAVELOAD
{
	BASE_GENERALPLAT_SAVELOAD generalPlatSaveload;
};

#endif
