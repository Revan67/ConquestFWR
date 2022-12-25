#ifndef DFABSAVE_H
#define DFABSAVE_H
//--------------------------------------------------------------------------//
//                                                                          //
//                               DFabSave.h                                 //
//                                                                          //
//                  COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/DInclude/DFabSave.h 38    10/05/00 5:16p Tmauer $
*/			    
//--------------------------------------------------------------------------//
#ifndef DBASEDATA_H
#include "DBaseData.h"
#endif

#ifndef DTYPES_H
#include "DTypes.h"
#endif

#ifndef DSHIPSAVE_H
#include "DShipSave.h"
#endif

#define FAB_MAX_QUEUE_SIZE 15		// also defined in Globals.h

//----------------------------------------------------------------
//
struct FAB_SAVELOAD		// template code save structure
{
	U32 buildeeID,repaireeID,selleeID;
	BOOL32 bDoorsOpen:1;
	BOOL32 bBuilding:1;
	BOOL32 bDrones:1;
	BOOL32 bReturning:1;
	U32 doorPause;

	//queue data
	U32 queueStart;
	U32 queueSize;
	U8 lastIndex;
	U32 buildQueue[FAB_MAX_QUEUE_SIZE];
	U8 buildQueueIndex[FAB_MAX_QUEUE_SIZE];
};

struct BUILDQUEUE_SAVELOAD
{
	U32 queueStart;
	U32 queueSize;
	U8 lastIndex;
	U32 buildQueue[FAB_MAX_QUEUE_SIZE];
	U8 buildQueueIndex[FAB_MAX_QUEUE_SIZE];
};
//----------------------------------------------------------------
//
enum FAB_MODES
{
	FAB_IDLE,
	FAB_MOVING_TO_TARGET,
	FAB_MOVING_TO_POSITION,
	FAB_WAITING_INIT_CONS_CLIENT,
	FAB_AT_TARGET_CLIENT,
	FAB_WATING_TO_START_BUILD_CLIENT,
	FAB_MOVING_TO_READY_TARGET_CLIENT,
	FAB_MOVING_TO_READY_TARGET_HOST,

	FAB_BUILDING,
	FAB_UNBUILD,

	FAB_EXPLODING,

	FAB_MOVING_TO_TARGET_REPAIR,
	FAB_WAIT_REPAIR_INFO_CLIENT,
	FAB_MOVING_TO_READY_TARGET_REPAIR_HOST,
	FAB_AT_TARGET_REPAIR_CLIENT,
	FAB_MOVING_TO_READY_TARGET_REPAIR_CLIENT,
	FAB_WATING_TO_START_REPAIR_CLIENT,
	FAB_REPAIRING,

	FAB_MOVING_TO_TARGET_DISM,
	FAB_WAIT_DISM_INFO_CLIENT,
	FAB_MOVING_TO_READY_TARGET_DISM_HOST,
	FAB_AT_TARGET_DISM_CLIENT,
	FAB_MOVING_TO_READY_TARGET_DISM_CLIENT,
	FAB_WATING_TO_START_DISM_CLIENT,
	FAB_DISMING,
};

enum MOVESTAGE
{
	MS_PATHFIND,
	MS_BEELINE,
	MS_DONE
};

struct BASE_FABRICATOR_SAVELOAD	 // leaf class's save structure
{
	Vector dir;

	FAB_MODES mode;
	U32 targetPlanetID;
	U32 targetSlotID;
	U32 buildingID;
	U32 workingID;
	U32 workTargID;

	ResourceCost workingCost;

	U32 oldHullPoints;//used for repairing

	GRIDVECTOR targetPosition;

	//for parking at the platform
	MOVESTAGE moveStage;
	Vector destPos;
	SINGLE destYaw;


	U8 lastTab;
};
//----------------------------------------------------------------
//
struct FABRICATOR_SAVELOAD : SPACESHIP_SAVELOAD		// structure written to disk
{
	BASE_FABRICATOR_SAVELOAD  baseSaveLoad;
	FAB_SAVELOAD fab_SL;
};
//----------------------------------------------------------------
//

#endif
