#ifndef DWAYPOINT_H
#define DWAYPOINT_H
//--------------------------------------------------------------------------//
//                                                                          //
//                               DWaypoint.h     							//
//                                                                          //
//                  COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/DInclude/DWaypoint.h 5     6/11/99 9:11p Jasony $
*/			    
//--------------------------------------------------------------------------//

#ifndef DMBASEDATA_H
#include "DMBaseData.h"
#endif

//----------------------------------------------------------------
//----------------------------------------------------------------
//
struct BT_WAYPOINT : BASIC_DATA
{
	char fileName[GT_PATH];
	MISSION_DATA missionData;
};

//----------------------------------------------------------------
//
struct WAYPOINT_SAVELOAD
{
	// physics data
	TRANS_SAVELOAD trans_SL;

	// mission data
	MISSION_SAVELOAD mission;
};

//----------------------------------------------------------------
//

//----------------------------------------------------------------
//
struct WAYPOINT_VIEW 
{
	MISSION_SAVELOAD * mission;
	Vector position;
};

//----------------------------------------------------------------
//

#endif
