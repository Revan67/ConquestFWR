#ifndef DMINEFIELD_H
#define DMINEFIELD_H
//--------------------------------------------------------------------------//
//                                                                          //
//                               DMinefield.h     							//
//                                                                          //
//                  COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/DInclude/DMinefield.h 18    7/28/00 1:37p Tmauer $
*/			    
//--------------------------------------------------------------------------//

#ifndef DMBASEDATA_H
#include "DMBaseData.h"
#endif

#ifndef _ADB
#ifndef GRIDVECTOR_H
#include "..\Src\GridVector.h"
#endif
#endif
//----------------------------------------------------------------
//
enum MINETYPE
{
	MINETYPE_UNKOWN = 0,
	MINETYPE_SPINE
};
//----------------------------------------------------------------
//
struct BT_MINEFIELD_DATA : BASIC_DATA
{
	MISSION_DATA missionData;
	U32 maxMineNumber;
	U32 damagePerHit;
	U32 supplyDamagePerHit;
	U32 hullLostPerHit;
	MINETYPE mineType;
	char regAnimation[GT_PATH];
	char blastType[GT_PATH];
	U32 mineWidth;
	S32 maxVerticalVelocity;
	S32 maxHorizontalVelocity;
	U32 mineAcceleration;
	SINGLE explosionRange;
	ResourceCost cost;
};
//----------------------------------------------------------------
//
struct BASE_MINEFIELD_SAVELOAD
{
	struct GRIDVECTOR gridPos;
	SINGLE storedDelay;
	U8 updateCounter;

	U32 layerRef;

	bool bUpdateFading:1;
	bool bDetonating:1;
};
//----------------------------------------------------------------
//
struct MINEFIELD_SAVELOAD : BASE_MINEFIELD_SAVELOAD
{
	// physics data
	TRANS_SAVELOAD trans_SL;

	// mission data
	MISSION_SAVELOAD mission;
};

//----------------------------------------------------------------
//
struct MINEFIELD_VIEW 
{
	U32 mineNumber;
};

//----------------------------------------------------------------
//

#endif
