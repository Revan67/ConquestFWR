#ifndef DPLATSAVE_H
#define DPLATSAVE_H
//--------------------------------------------------------------------------//
//                                                                          //
//                                DPlatSave.h                               //
//                                                                          //
//                  COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/DInclude/DPlatSave.h 31    6/24/00 12:22p Jasony $
*/			    
//--------------------------------------------------------------------------//
#ifndef DBASEDATA_H
#include "DBaseData.h"
#endif

#ifndef DTYPES_H
#include "DTypes.h"
#endif

#ifndef DINSTANCE_H
#include "DInstance.h"
#endif

#ifndef DFABSAVE_H
#include "DFabSave.h"
#endif

#ifndef DBUILDSAVE_H
#include "DBuildSave.h"
#endif

#ifndef _ADB
#ifndef NETVECTOR_H
#include "Netvector.h"
#endif
#endif

#define FAB_QUEUE_SIZE 10

#define MAX_SUPPLY_STACK_SIZE 10


struct BASE_GUNPLAT_SAVELOAD;
struct BASE_BUILDPLAT_SAVELOAD;
struct BASE_GENERALPLAT_SAVELOAD;
struct BASE_SUPPLYPLAT_SAVELOAD;
struct BASE_REFINEPLAT_SAVELOAD;
struct BASE_REPAIRPLAT_SAVELOAD;

//----------------------------------------------------------------
//
struct BASE_PLATFORM_SAVELOAD
{
	// physics data
	TRANS_SAVELOAD trans_SL;

	// build data
	BUILD_SAVELOAD build_SL;

	DAMAGE_SAVELOAD damage_SL;

	EXTENSION_SAVELOAD extend_SL;

	// mission data
	MISSION_SAVELOAD mission;

	U8 shadowVisibilityFlags;
	U32 exploredFlags;

	S8 shadowUpgrade[MAX_PLAYERS];
	S8 shadowUpgradeWorking[MAX_PLAYERS];
	U8 shadowUpgradeFlags[MAX_PLAYERS];
	SINGLE shadowPercent[MAX_PLAYERS];
	U16 shadowHullPoints[MAX_PLAYERS];
	U16 shadowMaxHull[MAX_PLAYERS];

	// slot info
	__hexview U16  buildSlot;		// 10 bit bitfield (0 means no build slot)
	__hexview U32  buildPlanetID;
	__hexview U32 firstNuggetID;		// mission part ID
	bool bSetCommandPoints:1;
	bool bPlatDead:1;
	bool bPlatRealyDead:1;
};
//----------------------------------------------------------------
//
struct BASE_OLDSTYLE_PLATFORM_SAVELOAD
{
	U32 workingID;

	NETVECTOR rallyPoint;

	//researchInfo
	U32 reasearchArchetypeID;
	SINGLE researchTimeSpent;

	//resupplyInfo
	U32 resupplyTargetID;

	U8 stackStart;
	U8 stackEnd;
	U32 supplyStack[MAX_SUPPLY_STACK_SIZE];

	bool bResupplyReturning;
	bool bResearching:1;
	bool bDockLocked:1;
	bool bNeedToSendLossOfTarget:1;
	bool bNeedToSendNewTarget:1;
	bool bSupplierOut:1;
};
//----------------------------------------------------------------
//
struct OLDSTYLE_PLATFORM_SAVELOAD : BASE_PLATFORM_SAVELOAD
{
	// fabricator data
	FAB_SAVELOAD fab_SL;

	// platform data
//	U32 firstNuggetID;
	TRANSFORM resupplyTransform;

	// platform specific stuff
	BASE_OLDSTYLE_PLATFORM_SAVELOAD	 baseOldstylePlatformSaveload;
};
//----------------------------------------------------------------
//
struct PLATFORM_VIEW
{
	MISSION_SAVELOAD *	mission;
	BASIC_INSTANCE *	rtData;
	union PLATFORM {
		BASE_OLDSTYLE_PLATFORM_SAVELOAD * oldstyle;
		BASE_GUNPLAT_SAVELOAD * gunplat;
		BASE_BUILDPLAT_SAVELOAD * buildPlat;
		BASE_GENERALPLAT_SAVELOAD * generalPlat;
		BASE_SUPPLYPLAT_SAVELOAD * supplyPlat;
		BASE_REFINEPLAT_SAVELOAD * refinePlat;
		BASE_REPAIRPLAT_SAVELOAD * repairPlat;
		void * nothing;
	} platform;
};
//----------------------------------------------------------------
//

#endif
