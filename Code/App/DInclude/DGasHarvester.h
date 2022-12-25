#ifndef DGASHARVESTER_H
#define DGASHARVESTER_H
//--------------------------------------------------------------------------//
//                                                                          //
//                                DGasHarvester.h                                //
//                                                                          //
//                  COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/DInclude/DGasHarvester.h 3     12/14/99 9:55a Tmauer $
*/			    
//--------------------------------------------------------------------------//

#ifndef DPLATSAVE_H
#include "DPlatSave.h"
#endif

#ifndef DPLATFORM_H
#include "DPlatform.h"
#endif

#define NUM_MINERS 6

//----------------------------------------------------------------
//
struct MINER_DRONE_RELEASE
{
	char hardpoint[HP_PATH];	// place where drones appear
	char builderType[GT_PATH];
};
//----------------------------------------------------------------
//
struct BT_GASHARVESTER_DATA : BASE_PLATFORM_DATA
{
	MINER_DRONE_RELEASE drone_release[NUM_MINERS];
	U32 maxMinerLoad;
	SINGLE mineNebTime;
	SINGLE mineRadius;
	char ship_hardpoint[GT_PATH];
};
//----------------------------------------------------------------
//
#ifndef _ADB
struct GASHARVESTER_INIT : PLATFORM_INIT<BT_GASHARVESTER_DATA>
{
	U32 rangeTexID;

	~GASHARVESTER_INIT (void);					// free archetype references
};
#endif
//--------------------------------------------------------------------------//
//
enum MINER_STATE
{
	MINER_PARKED,
	MINER_MOVING_TO_WORK,
	MINER_MOVING_TO_PARK,
	MINER_WORKING
};
struct BASE_GASHARVESTER_SAVELOAD
{
	struct _minerInfo
	{
		U32 nebulaID;
		U32 locationIndex;
		U32 load;
		SINGLE mineTime;
		S32 state;		
	}minerInfo[NUM_MINERS];

	U32 dockLockerID;
	U16 numDocking;
};
//--------------------------------------------------------------------------//
//
struct GASHARVESTER_SAVELOAD : BASE_PLATFORM_SAVELOAD
{
	BASE_GASHARVESTER_SAVELOAD gasHarvesterSaveload;

	TRANSFORM minerTransform[NUM_MINERS];
};

#endif
