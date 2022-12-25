#ifndef DATADEF_H
#define DATADEF_H
//--------------------------------------------------------------------------//
//                                                                          //
//                                DataDef.h                                 //
//				/Conquest/App/Src/Scripts/Script10T/DataDef.h				//
//					MISSION SPECIFIC DATA DEFINITIONS						//
//                                                                          //
//               COPYRIGHT (C) 2000 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    Created:	6/12/00		JeffP
	Modified:	6/12/00		JeffP

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

#define NATUS				1
#define BARYON				2
#define DALASI				3
#define ACRE				4
#define UNGER				5
#define POLLUX				6
#define PROCYON				7
#define SIRIUS				8
#define KASSE				9
#define VORLAN				10
#define FEMTO				11
#define VELL				12
#define SOLAR				13

#define NUM_SYSTEMS         13

//#define MAX_PLANETS 3*NUM_SYSTEMS

//--------------------------------------------------------------------------//
//  MISSION DATA
//--------------------------------------------------------------------------//

enum MissionState
{
	//Begin,
    Briefing,
	MissionStart,
	FindVivac,
	GoToNatus,
	Done
};
	
struct MissionData
{
	MissionState mission_state;

	bool mission_over, briefing_over, ai_enabled, next_state, vivacFound;
	U32 failureID;
	char failureFile[16];
	U32 mhandle, shandle;

	MPartRef Steele, Benson, Takei, KerTak, Vivac, Terran;
	MPartRef VivacTrigger, vivPlat1, vivPlat2, vivPlat3, vivPlat4, vivPlat5;
	MPartRef HQBase, AcropBase, mantisGate1, mantisGate2;
	bool bSeenMantis, bNoAcropKill;
	bool bAIOnInSystem[NUM_SYSTEMS+1];
	bool bDestroyedPlatInSystem[NUM_SYSTEMS+1];
	bool bBuiltInSystem[NUM_SYSTEMS+1];

    // variables used as parametrs when starting up the DisplayObjectiveHandler

    U32 displayObjectiveHandlerParams_stringID;
    U32 displayObjectiveHandlerParams_dependantHandle;
    U32 displayObjectiveHandlerParams_teletypeHandle;

	U32 textFailureID;
};

//--------------------------------------------------------------------------//
//  PROGRAM DATA
//--------------------------------------------------------------------------//

struct TM10_AIProgram_Data
{
	U32 timer;
};

//--------------------------------------------------------------------------//
struct TM10_Start_Data
{
	
};

//--------------------------------------------------------------------------//
//
struct TM10_Briefing_Data
{
	SINGLE timer;

	enum
	{
		Begin,
		Decoding,
		PreBrief,
		Elan,
		Smirnoff,
		Halsey,
		Smirnoff2,
		Hawkes,
		Smirnoff3,
		Hawkes2,
		Hawkes3,
		Smirnoff4,
		Hawkes4,
		Natus,
		Halsey2,
		Hawkes5,
		Halsey3,
		Smirnoff5,
		Halsey4,
		Smirnoff6,
		Halsey5,
		Benson,
		Blackwell,
		Objectives,
		Finish
	} state;
};

//--------------------------------------------------------------------------//
//
struct TM10_BriefingSkip_Data
{
	
};

//--------------------------------------------------------------------------//
//
struct TM10_MissionStart_Data
{
	SINGLE timer;

	enum
	{
		Begin,
		LeaveSolar,
		Bregon1,
		Solarian1,
		Bregon2,
		Solarian2,
		Bregon3,
		Solarian3,
		Objectives,
		Done
	} state;

};

//--------------------------------------------------------------------------//

struct TM10_VivacDeathClock_Save
{
	SINGLE timer;	
};

//--------------------------------------------------------------------------//

struct TM10_FindVivac_Data
{
	enum
	{
		Begin,
		Steele,
		Takei,
		KerTak,
		Benson,
		Takei2,
		Searching,
		SawPrime,
		SawPrime2,
		WheresVivac,
		EnterVivac,
		Vivac1,
		Steele1,
		Vivac2,
		Steele2,
		Vivac3,
		KerTak1,
		Vivac4,
		Steele3,
		Vivac5,
		PreDone,
		Done,
	} state;

	U32 timer;
	MPartRef prime;
	bool bSawPrime, bVivacExists, bStopped;
};

struct TM10_nearprocyonprime_Data { };

//--------------------------------------------------------------------------//

struct TM10_GoToNatus_Data
{

	enum
	{
		Begin,
		Steele1,
		KerTak1,
		Steele2,
		Vivac1,
		Benson1,
		Steele3,
		Benson2,
		Done
	} state;

	U32 timer;

};

//--------------------------------------------------------------------------//

//Mission Events

struct TM10_ObjectDestroyed_Data { };
struct TM10_ObjectConstructed_Data { };
struct TM10_ForbiddenJump_Data { };
struct TM10_UnderAttack_Data { };

//--------------------------------------------------------------------------//
//Mission Failures

struct TM10_MissionFailure_Data
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

struct TM10_MissionSuccess_Data
{
	enum
	{
		Begin,
		Teletype,
		Done
	} state;
};

struct TM10_DisplayObjectiveHandlerData
{
    U32 stringID;
    U32 dependantHandle;
};

//--------------------------------------------------------------------------//
//--------------------------------End DataDef.h-----------------------------//
//--------------------------------------------------------------------------//
#endif
