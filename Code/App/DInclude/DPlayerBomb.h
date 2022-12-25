#ifndef DPLAYERBOMB_H
#define DPLAYERBOMB_H
//--------------------------------------------------------------------------//
//                                                                          //
//                                DPlayerBomb.h                                 //
//                                                                          //
//                  COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Author: Tmauer $

    $Header: /Conquest/App/DInclude/DPlayerBomb.h 4     11/16/99 2:40p Tmauer $
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
struct BASE_PLAYERBOMB_SAVELOAD
{
	bool bDeployedPlayer;
};
//--------------------------------------------------------------------------//
//
struct PLAYERBOMB_SAVELOAD : BASE_PLAYERBOMB_SAVELOAD
{
	// physics data
	TRANS_SAVELOAD trans_SL;

	// mission data
	MISSION_SAVELOAD mission;
};

//--------------------------------------------------------------------------
//
struct PLAYERBOMB_TYPE
{
	char archetypeName[GT_PATH];
};
//--------------------------------------------------------------------------//
//
struct BT_PLAYERBOMB_DATA : BASIC_DATA
{
	struct _playerRace
	{
		PLAYERBOMB_TYPE minBombType[4];

		PLAYERBOMB_TYPE bombType[4];

		PLAYERBOMB_TYPE largeBombType[8];
	}race[4];

	char playerBomb_anim2D[GT_PATH];
	S32 animSize;

	MISSION_DATA missionData;

	char filename[GT_PATH];
};


#endif
