#ifndef DSCRIPTOBJECT_H
#define DSCRIPTOBJECT_H
//--------------------------------------------------------------------------//
//                                                                          //
//                               DScriptObject.h     							//
//                                                                          //
//                  COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/DInclude/DScriptObject.h 6     8/30/00 4:25p Justinp $
*/			    
//--------------------------------------------------------------------------//

#ifndef DMBASEDATA_H
#include "DMBaseData.h"
#endif

#ifndef DSPACESHIP_H
#include "DSpaceShip.h"
#endif

//----------------------------------------------------------------
//----------------------------------------------------------------
//
struct BT_SCRIPTOBJECT : BASIC_DATA
{
	char fileName[GT_PATH];
	MISSION_DATA missionData;
	SFX::ID ambientSound;
	struct BLINKER_DATA blinkers;
	char ambient_animation[GT_PATH];

	bool bSysMapActive:1;
};

struct BASE_SCRIPTOBJECT_SAVELOAD
{
	bool bTowed:1;
	bool bSysMapActive:1;

    U32 towerID;
};
//----------------------------------------------------------------
//
struct SCRIPTOBJECT_SAVELOAD:BASE_SCRIPTOBJECT_SAVELOAD
{
	// physics data
	TRANS_SAVELOAD trans_SL;

	// mission data
	MISSION_SAVELOAD mission;

	U8  exploredFlags;
};

//----------------------------------------------------------------
//

//----------------------------------------------------------------
//
struct SCRIPTOBJECT_VIEW 
{
	MISSION_SAVELOAD * mission;
	Vector position;
};

//----------------------------------------------------------------
//

#endif
