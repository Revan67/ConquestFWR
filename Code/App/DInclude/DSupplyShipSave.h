#ifndef DSUPPLYSHIPSAVE_H
#define DSUPPLYSHIPSAVE_H
//--------------------------------------------------------------------------//
//                                                                          //
//                               DSupplyShip.h                                 //
//                                                                          //
//                  COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/DInclude/DSupplyShipSave.h 15    9/15/00 10:47p Tmauer $
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

#ifndef _ADB
#ifndef NETVECTOR_H
#include "NetVector.h"
#endif

#endif

#define MAX_SUPPLY_STACK_SIZE 10

//----------------------------------------------------------------
//
struct SUP_SAVELOAD		// template code save structure
{
};
//----------------------------------------------------------------
//
enum SUPPLYSHIP_MODES
{
	SUP_IDLE,
	SUP_RESUPPLY_ESCORT,
	SUP_MOVING_TARGETED_SUPPLY,
	SUP_CLIENT_IDLE,
	SUP_MOVING_TO_TENDER
};

enum SUPPLY_SHIP_STANCE
{
	SUP_STANCE_NONE,
	SUP_STANCE_RESUPPLY,
	SUP_STANCE_FULLYAUTO
};

struct BASE_SUPPLYSHIP_SAVELOAD	 // leaf class's save structure
{
	SUPPLYSHIP_MODES mode;
	U32 workingID;
	U32 targetedTargetID;
	SINGLE supplyTimer;

	struct NETGRIDVECTOR supplyPoint;
	U32 supplyEscortTargetID;
	GRIDVECTOR seekTarget;

	U32 supplyPlatformTargetID;

	SUPPLY_SHIP_STANCE supplyStance;

	bool bNeedToSendMoveToTarget:1;
	bool bStoreAgentID:1;
};
//----------------------------------------------------------------
//
struct SUPPLYSHIP_SAVELOAD : SPACESHIP_SAVELOAD		// structure written to disk
{
	BASE_SUPPLYSHIP_SAVELOAD  baseSaveLoad;
	SUP_SAVELOAD sup_SL;
};

//----------------------------------------------------------------
//

#endif
