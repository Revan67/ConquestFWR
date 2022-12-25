#ifndef DBLACKHOLE_H
#define DBLACKHOLE_H
//--------------------------------------------------------------------------//
//                                                                          //
//                           DBlackHole.h                                   //
//                                                                          //
//                  COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*

    $Author: Tmauer $

    $Header: /Conquest/App/DInclude/DBlackHole.h 9     9/01/00 3:50p Tmauer $
*/			    
//--------------------------------------------------------------------------//
#ifndef DBASEDATA_H
#include "DBaseData.h"
#endif

#ifndef DINSTANCE_H
#include "DInstance.h"
#endif

#ifndef DBLACKHOLE_H
#include "DBlackHole.h"
#endif

#ifndef DSECTOR_H
#include "DSector.h"
#endif

//----------------------------------------------------------------
//
struct BT_BLACKHOLE_DATA : BASIC_DATA
{
	BILLBOARD_MESH billboardMesh[MAX_BB_MESHES];
	char ringObjectName[GT_PATH];
	char sysMapIcon[GT_PATH];
	MISSION_DATA missionData;
	U16 damage;
};

struct BASE_BLACKHOLE_SAVELOAD
{
	U8 targetSys[MAX_SYSTEMS];
	U8 numTargetSys;
	U8 currentTarget;
	SINGLE timer;
	U32 workingID;
	U32 waitingID;
	NETGRIDVECTOR waitingJumpPos;
};

//----------------------------------------------------------------
//
struct BLACKHOLE_SAVELOAD : BASE_BLACKHOLE_SAVELOAD
{
	/* mission data */
	MISSION_SAVELOAD mission;

	/* physics data */
	TRANS_SAVELOAD trans_SL;
};

#define MAX_SYSTEMS_PLUS_ONE 17

struct BLACKHOLE_DATA
{
	bool jumpsTo[MAX_SYSTEMS_PLUS_ONE];
};
//----------------------------------------------------------------
//

#endif
