#ifndef DOBJECTGENERATOR_H
#define DOBJECTGENERATOR_H
//--------------------------------------------------------------------------//
//                                                                          //
//                               DObjectGenerator.h     							//
//                                                                          //
//                  COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/DInclude/DObjectGenerator.h 4     11/18/99 4:55p Tmauer $
*/			    
//--------------------------------------------------------------------------//

#ifndef DMBASEDATA_H
#include "DMBaseData.h"
#endif

//----------------------------------------------------------------
//----------------------------------------------------------------
//
struct BT_OBJECT_GENERATOR : BASIC_DATA
{
	char fileName[GT_PATH];
	MISSION_DATA missionData;
	SINGLE mean;
	SINGLE minDiff;
	char generateType[GT_PATH];
	bool startEnabled;
};
//----------------------------------------------------------------
//

struct BASE_OBJECT_GENERATOR_SAVELOAD
{
	SINGLE mean;
	SINGLE minDiff;
	SINGLE timer;
	SINGLE nextTime;
	U32 archID;
	bool bGenEnabled:1;
};

//----------------------------------------------------------------
//
struct OBJECT_GENERATOR_SAVELOAD
{
	// physics data
	TRANS_SAVELOAD trans_SL;

	// mission data
	MISSION_SAVELOAD mission;

	BASE_OBJECT_GENERATOR_SAVELOAD baseSave;
};

//----------------------------------------------------------------
//

//----------------------------------------------------------------
//
struct OBJECT_GENERATOR_VIEW 
{
	MISSION_SAVELOAD * mission;
	Vector position;
	char generatorType[GT_PATH];
	SINGLE mean;
	SINGLE minDiff;
	SINGLE timer;
	SINGLE nextTime;
	bool bGenEnabled:1;
	char partName[GT_PATH];
};

//----------------------------------------------------------------
//

#endif
