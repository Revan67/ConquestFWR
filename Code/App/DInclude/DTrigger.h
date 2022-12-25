#ifndef DTRIGGER_H
#define DTRIGGER_H
//--------------------------------------------------------------------------//
//                                                                          //
//                               DTrigger.h     							//
//                                                                          //
//                  COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/DInclude/DTrigger.h 3     11/18/99 4:55p Tmauer $
*/			    
//--------------------------------------------------------------------------//

#ifndef DMBASEDATA_H
#include "DMBaseData.h"
#endif

//----------------------------------------------------------------
//----------------------------------------------------------------
//
struct BT_TRIGGER : BASIC_DATA
{
	char fileName[GT_PATH];
	MISSION_DATA missionData;

	enum Type
	{
		SPHERE,
		REGION,
		LINE
	};
	Type type;
	float size;
};
//----------------------------------------------------------------
//
struct BASE_TRIGGER_SAVELOAD
{
	U32 lastTriggerID;

	U32 triggerShipID;
	U32 triggerObjClassID;
	U32 triggerMObjClassID;
	U32 triggerPlayerID;
	SINGLE triggerRange;
	U32 triggerFlags;
	char progName[GT_PATH];
	bool bEnabled:1;
	bool bDetectOnlyReady:1;
};

//----------------------------------------------------------------
//
struct TRIGGER_SAVELOAD
{
	// physics data
	TRANS_SAVELOAD trans_SL;

	// mission data
	MISSION_SAVELOAD mission;

	BASE_TRIGGER_SAVELOAD baseSave;
};

//----------------------------------------------------------------
//
struct TRIGGER_VIEW 
{
	MISSION_SAVELOAD * mission;
	Vector position;

	__hexview U32 triggerShipID;
	OBJCLASS triggerObjClassID;
	M_OBJCLASS triggerMObjClassID;
	U32 triggerPlayerID;
	SINGLE triggerRange;
	char progName[GT_PATH];
	bool bEnabled:1;
	bool bDetectOnlyReady:1;
};

//----------------------------------------------------------------
//

#endif
