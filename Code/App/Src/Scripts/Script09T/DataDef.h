#ifndef DATADEF_H
#define DATADEF_H
//--------------------------------------------------------------------------//
//                                                                          //
//                                DataDef.h                                 //
//				/Conquest/App/Src/Scripts/Script09T/DataDef.h				//
//					MISSION SPECIFIC DATA DEFINITIONS						//
//                                                                          //
//               COPYRIGHT (C) 2000 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    Created:	4/28/00		JeffP
	Modified:	5/01/00		JeffP

*/
//--------------------------------------------------------------------------//

#ifndef US_TYPEDEFS
#include <typedefs.h>
#endif

#ifndef MPARTREF_H
#include <MPartRef.h>
#endif

//--------------------------------------------------------------------------
//  DEFINES
//--------------------------------------------------------------------------

#define PLAYER_ID			1
#define MANTIS_ID			2
#define SOLARIAN_ID			3
#define REBEL_ID			4

#define QUASILI				1
#define EPSILON				2
#define NANO				3
#define KAAP				4
#define GERA				5
#define SERPCE				6
#define TALOS				7
#define GROG				8
#define ABELL				9
#define VARN				10


#define NUM_SYSTEMS         12

#define MAX_PLANETS 3*NUM_SYSTEMS

//--------------------------------------------------------------------------//
//  MISSION DATA
//--------------------------------------------------------------------------//

enum MissionState
{
	//Begin,
    Briefing,
	MissionStart,
	EscortToEpsilon,
	Done
};
	
struct MissionData
{
	MissionState mission_state;

	bool mission_over, briefing_over, ai_enabled;
	U32 mhandle, shandle;

	bool bKerTakDied, bElanDied, bInEpsilon;

	//mike's edits
	bool bInGrog, bInSerpce, bTaosHelp;	
	MPartRef Khamir1, Khamir2, Khamir3, Khamir4, Khamir5, Khamir6;
	MPartRef Khamir7, Khamir8, Khamir9, Khamir10, Khamir11, Khamir12;
	// end

	MPartRef Halsey, KerTak, Elan, Blackwell;
	MPartRef PlanetTrigger[MAX_PLANETS];
	U32 numTriggers, numInAbell;
	bool bSeenInSystem[NUM_SYSTEMS+1], bAIOnForSystem[NUM_SYSTEMS+1];

	U32 failureID;
};

//--------------------------------------------------------------------------//
//  PROGRAM DATA
//--------------------------------------------------------------------------//

struct TM9_playernearbase_Data { };

//--------------------------------------------------------------------------//

struct TM9_AIProgram_Data
{
	U32 timer;
};

//--------------------------------------------------------------------------//
struct TM9_Start_Data
{
	
};

//--------------------------------------------------------------------------//
//

struct TM9_Briefing_Data
{
	SINGLE timer;
	U32 handle1, handle2, handle3, handle4;

	enum
	{
		Begin,
		TeleTypeLocation,
		Davis,
		Blackwell,
		Davis2,
		Singers,
		Davis3,
		PreHalsey,
		Halsey,
		Blackwell2,
		PreElan,
		Elan,
		Halsey2,
		Elan2,
		Halsey3,
		Blackwell3,
		Elan3,
		Halsey4,
		Elan4,
		Halsey5,
		Objectives,
		Finish
	} state;
};

//--------------------------------------------------------------------------//
//
struct TM9_BriefingSkip_Data
{
	
};

//--------------------------------------------------------------------------//
//
struct TM9_MissionStart_Data
{
	SINGLE timer;

	enum
	{
		Begin,
		Blackwell1,
		Halsey1,
		Elan1,
		Blackwell2,
		Halsey2,
		KerTak1,
		Done
	} state;

};

//--------------------------------------------------------------------------//
//
struct TM9_EscortToEpsilon_Data
{
	SINGLE timer;	
};
//--------------------------------------------------------------------------//
//

struct TM9_Patrol_Data
{
	U32 numPoints;
	MPartRef ship;
	SINGLE idleTimer;
	U32 currentIndex;
	MPartRef patrolPoints[10];
};

//--------------------------------------------------------------------------//
//

struct TM9_TaosHelp_Data
{
	S32 state;
	U32 wormBlast;
	SINGLE timer;
	MPartRef targetPart;
};

// mike's edits
struct TM9_KamikazeElan_Data
{
	
};
// end

//--------------------------------------------------------------------------//
//Mission Events

struct TM9_ObjectDestroyed_Data { };
struct TM9_ObjectConstructed_Data { };
struct TM9_ForbiddenJump_Data { };
struct TM9_UnderAttack_Data { };

//--------------------------------------------------------------------------//
//Mission Failures

struct TM9_MissionFailure_Data
{
	enum
	{
		Begin,
		Teletype,
		Done
	} state;
};

//--------------------------------------------------------------------------//
//Mission Success

struct TM9_MissionSuccess_Data
{
	enum
	{
		Begin,
		Teletype,
		Done
	} state;
};

//--------------------------------------------------------------------------//
//--------------------------------End DataDef.h-----------------------------//
//--------------------------------------------------------------------------//
#endif
