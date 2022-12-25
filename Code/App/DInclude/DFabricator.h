#ifndef DFABRICATOR_H
#define DFABRICATOR_H
//--------------------------------------------------------------------------//
//                                                                          //
//                            DFabricator.h                                 //
//                                                                          //
//                  COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/DInclude/DFabricator.h 17    2/28/00 2:40p Rmarr $
*/			    
//--------------------------------------------------------------------------//
#ifndef DBASEDATA_H
#include "DBaseData.h"
#endif

#ifndef DSPACESHIP_H
#include "DSpaceship.h"
#endif

#ifndef DTYPES_H
#include "DTypes.h"
#endif

#ifndef DINSTANCE_H
#include "DInstance.h"
#endif

#define NUM_DRONE_RELEASE 2
//----------------------------------------------------------------
//
struct DRONE_RELEASE
{
	char hardpoint[HP_PATH];	// place where drones appear
	char builderType[GT_PATH];
	U8 numDrones;
};
//----------------------------------------------------------------
//

struct BT_FABRICATOR_DATA : BASE_SPACESHIP_DATA
{
	struct DRONE_RELEASE drone_release[NUM_DRONE_RELEASE];
	SINGLE repairRate;
	U32 maxQueueSize;
	SFX::ID buildSound;
	SFX::ID beginBuildSfx;

};
//----------------------------------------------------------------
//
#ifndef _ADB
struct BUILDER_INFO
{
	PARCHETYPE pBuilderType;
	U8 numDrones;
};

struct FABRICATOR_INIT : SPACESHIP_INIT<BT_FABRICATOR_DATA>
{
	PARCHETYPE pBuildEffect;
	BUILDER_INFO builderInfo[NUM_DRONE_RELEASE];
};
#endif
//----------------------------------------------------------------
//

#endif
