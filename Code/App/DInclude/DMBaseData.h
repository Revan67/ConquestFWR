#ifndef DMBASEDATA_H
#define DMBASEDATA_H
//--------------------------------------------------------------------------//
//                                                                          //
//                               DMBaseData.h                               //
//                                                                          //
//                  COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/DInclude/DMBaseData.h 37    10/31/00 9:03a Jasony $
*/			    
//--------------------------------------------------------------------------//

#ifndef DBASEDATA_H
#include "DBaseData.h"
#endif

//--------------------------------------------------------------------------//
//-- Base save / load structure for all mission objects
//--------------------------------------------------------------------------//
//

struct MISSION_SAVELOAD
{
	__readonly M_OBJCLASS mObjClass;
	M_STRING partName;
	__readonly __hexview U32 dwMissionID;
	U32 systemID:8;		// need at least 8 bits because of hyperspace (sector.h)
	M_RACE race:4;
	U32  playerID:4;	// which slot

	SINGLE maxVelocity;
	SINGLE sensorRadius;
	SINGLE cloakedSensorRadius;
	U16 supplies;
	U16 hullPoints;
	U16	hullPointsMax;
	U16	supplyPointsMax;
	U16	numKills;
	bool bReady:1;
	bool bRecallFighters:1;		// used for carriers
	bool bMoveDisabled:1;		// disable bobbing
	bool bPendingOpValid:1;		// if true, the pendingOp variable, below, is valid (set by the matrix)
	bool bUnselectable:1;		//for script code only, turns off user input for the object
	bool bDerelict:1;			//for script code only, lets harvester know it is ok to tug.
	bool bTowing:1;				//for script code only, lets the harvester know that it is being tuged
	bool bInvincible:1;			//for script code only, makes it so this unit can't drop below 10% hull points
	bool bNoAIControl:1;		//when true the player AI should not issue any orders to this unit.
	bool bAllEventsOn:1;		//for script only, makes this object generate ALL acript events that happen to it.
	bool bShowPartName:1;		// for script only. show part name istead of display name
	bool bUnderCommand:1;		//if true command points have been spent for this object
	bool bNoAutoTarget:1;		//enemy units will not target this unit (player may still target it)
	U8	 controlGroupID; 
	U8	 groupIndex;			// which member (0 to num_ships-1) of the group am I?

	MISSION_DATA::M_CAPS caps;
	__readonly __hexview U32 groupID;				// just for spaceships, might contain a fleet officer
	__readonly __hexview U32 admiralID;				// admiral is onboard!
	__readonly __hexview U32 fleetID;				// controlling adimiral ID

	struct InstanceTechLevel
	{
		enum UPGRADE
		{
			LEVEL0=0,
			LEVEL1,
			LEVEL2,
			LEVEL3,
			LEVEL4,
			LEVEL5
		};

		UPGRADE engine:4;
		UPGRADE hull:4;
		UPGRADE supplies:4;
		UPGRADE targeting:4;
		UPGRADE damage:4;
		UPGRADE shields:4;
		UPGRADE experience:4;
//		UPGRADE tanker:4;
		UPGRADE sensors:4;
		UPGRADE classSpecific:4;//reserved for tanker, fleet tender, and fighter upgrades
	} techLevel;


	// opAgent data
	__readonly U32 lastOpCompleted;
	__readonly U32 pendingOp;		// operation now active, or queued for this unit
};


#endif
