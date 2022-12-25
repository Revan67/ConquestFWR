#ifndef DBUILDPLAT_H
#define DBUILDPLAT_H
//--------------------------------------------------------------------------//
//                                                                          //
//                                DBuildPlat.h                                //
//                                                                          //
//                  COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/DInclude/DBuildPlat.h 11    2/28/00 2:40p Rmarr $
*/			    
//--------------------------------------------------------------------------//

#ifndef DPLATSAVE_H
#include "DPlatSave.h"
#endif

#ifndef DPLATFORM_H
#include "DPlatform.h"
#endif

#define NUM_DRONE_RELEASE 2
//----------------------------------------------------------------
//

struct BT_PLAT_BUILD_DATA : BASE_PLATFORM_DATA
{
	char ship_hardpoint[HP_PATH];	// place where ship appears
	DRONE_RELEASE drone_release[NUM_DRONE_RELEASE];
	S32 buildRate;
	U32 maxQueueSize;
};
//----------------------------------------------------------------
//
#ifndef _ADB

struct BUILDPLAT_INIT : PLATFORM_INIT<BT_PLAT_BUILD_DATA>
{
	// nothing yet?

	~BUILDPLAT_INIT (void);					// free archetype references

	BUILDER_INFO builderInfo[NUM_DRONE_RELEASE];

};
#endif
//--------------------------------------------------------------------------//
//
struct BASE_BUILDPLAT_SAVELOAD
{
	U32 workingID;

	struct NETGRIDVECTOR rallyPoint;

	//researchInfo
	SINGLE buidTimeSpent;
	
	U32 constructionID;

	bool bResearching:1;
	bool bUpgrading:1;
};
//--------------------------------------------------------------------------//
//
struct BUILDPLAT_SAVELOAD : BASE_PLATFORM_SAVELOAD
{
	// fabricator data
	FAB_SAVELOAD fab_SL;

	BASE_BUILDPLAT_SAVELOAD buildPlatSaveload;
};

#endif
