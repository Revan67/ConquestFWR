#ifndef DREFINEPLAT_H
#define DREFINEPLAT_H
//--------------------------------------------------------------------------//
//                                                                          //
//                                DRefinePlat.h                                //
//                                                                          //
//                  COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/DInclude/DRefinePlat.h 19    10/05/00 5:46p Tmauer $
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

struct BT_PLAT_REFINE_DATA : BASE_PLATFORM_DATA
{
	char ship_hardpoint[HP_PATH];	// place where ship docks
	char dock_hardpoint[HP_PATH];   //place where harvester docks
	DRONE_RELEASE drone_release[NUM_DRONE_RELEASE];
	S32 buildRate;
	U32 maxQueueSize;
	
	char harvesterArchetype[GT_PATH];//free harvester type

	SINGLE gasRate[4];
	SINGLE metalRate[4];
	SINGLE crewRate[4];
};
//----------------------------------------------------------------
//
#ifndef _ADB
struct REFINEPLAT_INIT : PLATFORM_INIT<BT_PLAT_REFINE_DATA>
{
	BUILDER_INFO builderInfo[NUM_DRONE_RELEASE];

	~REFINEPLAT_INIT (void);					// free archetype references
};
#endif
//--------------------------------------------------------------------------//
//
struct BASE_REFINEPLAT_SAVELOAD
{
	U32 workingID;

	struct NETGRIDVECTOR rallyPoint;

	//researchInfo
	U32 constructionID;
	SINGLE buidTimeSpent;

	SINGLE gasHarvested;
	SINGLE metalHarvested;
	SINGLE crewHarvested;

	HARVEST_STANCE defaultHarvestStance;

	U32 dockLockerID;
	U16 numDocking;
	bool bFreeShipMade:1;
	bool bResearching:1;
	bool bUpgrading:1;
};
//--------------------------------------------------------------------------//
//
struct REFINEPLAT_SAVELOAD : BASE_PLATFORM_SAVELOAD
{
	// fabricator data
	FAB_SAVELOAD fab_SL;

	BASE_REFINEPLAT_SAVELOAD refinePlatSaveload;
};

#endif
