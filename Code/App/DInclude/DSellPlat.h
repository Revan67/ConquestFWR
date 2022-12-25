#ifndef DSELLPLAT_H
#define DSELLPLAT_H
//--------------------------------------------------------------------------//
//                                                                          //
//                                DSellPlat.h                                //
//                                                                          //
//                  COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/DInclude/DSellPlat.h 3     10/17/00 6:15p Tmauer $
*/			    
//--------------------------------------------------------------------------//

#ifndef DPLATSAVE_H
#include "DPlatSave.h"
#endif

#ifndef DPLATFORM_H
#include "DPlatform.h"
#endif

#ifndef DFABSAVE_H
#include "DFabSave.h"
#endif

//----------------------------------------------------------------
//
struct BT_PLAT_SELL_DATA : BASE_PLATFORM_DATA
{
	char ship_hardpoint[HP_PATH];
	U32 maxQueueSize;
};
//----------------------------------------------------------------
//
#ifndef _ADB
struct SELLPLAT_INIT : PLATFORM_INIT<BT_PLAT_SELL_DATA>
{
	// nothing yet?

	~SELLPLAT_INIT (void);					// free archetype references

	BUILDER_INFO builderInfo[NUM_DRONE_RELEASE];
};
#endif
//--------------------------------------------------------------------------//
//
struct BASE_SELLPLAT_SAVELOAD
{
	U32 workingID;
	U32 potentialWorkingID;
	U32 sellTargetID;
	enum SELL_MODE
	{
		SEL_NONE,
		SEL_WAIT_FOR_DOCK,
		SEL_SELLING
	} mode;

	bool bDockLocked:1;
};
//--------------------------------------------------------------------------//
//
struct SELLPLAT_SAVELOAD : BASE_PLATFORM_SAVELOAD
{
	BASE_SELLPLAT_SAVELOAD sellPlatSaveload;

	FAB_SAVELOAD fab_SL;
};

#endif
