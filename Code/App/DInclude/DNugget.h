#ifndef DNUGGET_H
#define DNUGGET_H
//--------------------------------------------------------------------------//
//                                                                          //
//                                DNugget.h                                 //
//                                                                          //
//                  COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Author: Jasony $

    $Header: /Conquest/App/DInclude/DNugget.h 28    10/23/00 11:27a Jasony $
*/			    
//--------------------------------------------------------------------------//
#ifndef DTYPES_H
#include "DTypes.h"
#endif

#ifndef DMBASEDATA_H
#include "DMBaseData.h"
#endif

//--------------------------------------------------------------------------//
//
struct BASE_ANIMNUGGET_SAVELOAD
{
	U16 supplyPointsMax;
	U16 supplies;				//in the quick dropout case these 2 are needed
	U8 shadowVisibilityFlags;	//therefore these 2 should be on the same cacheline
	U8 harvestCount;
	bool bDepleted:1;
	bool deleteOK:1;
	bool bUnregistered:1;
	bool bRealized:1;
	U8 systemID;
	U16 shadowSupplies[MAX_PLAYERS];
	U32 processID;
	U32 deathOp;

	SINGLE lifeTimeRemaining;
	U32 dwMissionID;
	Vector position;
};

//--------------------------------------------------------------------------//
//
struct BASE_NUGGET_SAVELOAD
{
	U32 processID;
	U8 harvestCount;

	U32 lifeTimeTicks;
};
//--------------------------------------------------------------------------//
//
struct NUGGET_SAVELOAD : BASE_NUGGET_SAVELOAD
{
	// physics data
	TRANS_SAVELOAD trans_SL;

	// mission data
	MISSION_SAVELOAD mission;
};
//--------------------------------------------------------------------------//
//
enum M_NUGGET_TYPE
{
	M_GAS_NUGGETS,
	M_METAL_NUGGET,
	M_SCRAP_NUGGET,
	M_HYADES_NUGGET
};

struct BT_NUGGET_DATA : BASIC_DATA
{
	char nugget_anim2D[GT_PATH];
	char nugget_mesh[GT_PATH];
	S32 animSizeSmallMax;
	S32 animSizeMax;
	S32 animSizeMin;
	struct _color
	{
		U8 redHi;
		U8 greenHi;
		U8 blueHi;
		U8 redLo;
		U8 greenLo;
		U8 blueLo;
		U8 alphaIn;
		U8 alphaOut;
	}color;

	bool bOriented;
	bool zRender;

	M_RESOURCE_TYPE resType;

	M_NUGGET_TYPE nuggetType;

	U32 maxSupplies;
};


#endif
