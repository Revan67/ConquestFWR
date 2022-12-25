#ifndef DBUILDSUPPLAT_H
#define DBUILDSUPPLAT_H
//--------------------------------------------------------------------------//
//                                                                          //
//                                DBuildSupPlat.h                                //
//                                                                          //
//                  COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/DInclude/DBuildSupPlat.h 10    9/15/00 10:47p Tmauer $
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

struct BT_PLAT_BUILDSUP_DATA : BASE_PLATFORM_DATA
{
	char ship_hardpoint[HP_PATH];	// place where ship appears
	DRONE_RELEASE drone_release[NUM_DRONE_RELEASE];
	S32 buildRate;
	U32 maxQueueSize;

	SINGLE supplyRadius;
	SINGLE supplyPerSecond;
};
//----------------------------------------------------------------
//
#ifndef _ADB
struct BUILDSUPPLAT_INIT : PLATFORM_INIT<BT_PLAT_BUILDSUP_DATA>
{
	BUILDER_INFO builderInfo[NUM_DRONE_RELEASE];

	~BUILDSUPPLAT_INIT (void);					// free archetype references
};
#endif
//--------------------------------------------------------------------------//
//
struct BASE_BUILDSUPPLAT_SAVELOAD
{
	U32 workingID;

	struct NETGRIDVECTOR rallyPoint;

	SINGLE supplyTimer;

	//researchInfo
	U32 constructionID;
	SINGLE buildTimeSpent;

	bool bResearching:1;
};
//--------------------------------------------------------------------------//
//
struct BUILDSUPPLAT_SAVELOAD : BASE_PLATFORM_SAVELOAD
{
	// fabricator data
	FAB_SAVELOAD fab_SL;

	BASE_BUILDSUPPLAT_SAVELOAD buildPlatSaveload;
};

#endif
