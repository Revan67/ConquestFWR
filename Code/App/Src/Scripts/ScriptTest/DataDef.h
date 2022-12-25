#ifndef DATADEF_H
#define DATADEF_H
//--------------------------------------------------------------------------//
//                                                                          //
//                                DataDef.h                                 //
//                                                                          //
//               COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//


//--------------------------------------------------------------------------//

#ifndef US_TYPEDEFS
#include <typedefs.h>
#endif

#ifndef MPARTREF_H
#include <MPartRef.h>
#endif



//--------------------------------------------------------------------------//
//  MISSION DATA
//--------------------------------------------------------------------------//

enum MissionState
{
	Begin,
    Briefing,
	Done
};
	
struct MissionData
{
	MissionState mission_state;

	bool mission_over, briefing_over;

	U32 mhandle, shandle;

	MPartRef Missle1, Missle2, Missle3, Missle4;
    MPartRef Corvette1, Corvette2, Corvette3, Corvette4, Corvette5, Corvette6;
	MPartRef Spy1, Spy2;
	MPartRef Battleship1, Battleship2;
	MPartRef Supplyship;



};

//--------------------------------------------------------------------------//
//  PROGRAM DATA
//--------------------------------------------------------------------------//

struct TM4_Start_Save
{
	
};

//--------------------------------------------------------------------------//
//

struct TM4_Briefing_Save
{
	SINGLE timer;


	enum
	{
		Begin,
		TeleTypeLocation,
		Officer_Converse,
		Radio_Broadcast,
		HalseyBrief,
		DisplayMO,
		Finish
	}state;
};

//--------------------------------------------------------------------------//
//
struct TM4_BriefingSkip_Save
{
	
};

//--------------------------------------------------------------------------//
//
struct TM4_MissionStart_Save
{

};


//--------------------------------------------------------------------------//
//--------------------------------End DataDef.h-----------------------------//
//--------------------------------------------------------------------------//
#endif