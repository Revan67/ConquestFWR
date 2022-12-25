#ifndef DSUPPLYPLAT_H
#define DSUPPLYPLAT_H
//--------------------------------------------------------------------------//
//                                                                          //
//                                DSupplyPlat.h                                //
//                                                                          //
//                  COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/DInclude/DSupplyPlat.h 8     12/14/99 9:55a Tmauer $
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
struct BT_PLAT_SUPPLY_DATA : BASE_PLATFORM_DATA
{
	SINGLE supplyLoadSize;
	SINGLE supplyRange;
	char supplyDroneType[GT_PATH];
	char supplyDroneHardpoint[GT_PATH];
};
//----------------------------------------------------------------
//
#ifndef _ADB
struct SUPPLYPLAT_INIT : PLATFORM_INIT<BT_PLAT_SUPPLY_DATA>
{
	U32 rangeTexID;

	~SUPPLYPLAT_INIT (void);					// free archetype references
};
#endif
//--------------------------------------------------------------------------//
//
struct BASE_SUPPLYPLAT_SAVELOAD
{
	U32 resupplyTargetID;

	U8 stackStart;
	U8 stackEnd;
	U32 supplyStack[MAX_SUPPLY_STACK_SIZE];

	bool bResupplyReturning:1;
	bool bNeedToSendLossOfTarget:1;
	bool bNeedToSendNewTarget:1;
	bool bSupplierOut:1;
};
//--------------------------------------------------------------------------//
//
struct SUPPLYPLAT_SAVELOAD : BASE_PLATFORM_SAVELOAD
{
	BASE_SUPPLYPLAT_SAVELOAD supplyPlatSaveload;

	TRANSFORM resupplyTransform;
};

#endif
