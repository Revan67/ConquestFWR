#ifndef DATADEF_H
#define DATADEF_H
//--------------------------------------------------------------------------//
//                                                                          //
//                                DataDef.h                                 //
//				/Conquest/App/Src/Scripts/Script12T/DataDef.h				//
//					MISSION SPECIFIC DATA DEFINITIONS						//
//                                                                          //
//               COPYRIGHT (C) 2000 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
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

//--------------------------------------------------------------------------//
//  MISSION DATA
//--------------------------------------------------------------------------//

enum MissionState
{
	Begin,
    Briefing,
	MissionStart,
	Done
};
	
struct MissionData
{
	MissionState mission_state;
	
	bool mission_over;

	U32 mhandle, shandle, thandle;
	U32 NumOfPlats;

	MPartRef Steele, Blackwell, Hawkes, Benson, Vivac, Takei, Smirnoff;
	MPartRef SpawnPoint, Quasili, Epsilon, serpceGate;
    MPartRef mainAcropolis;

	U32 failureID;
};

//--------------------------------------------------------------------------//
//  PROGRAM DATA
//--------------------------------------------------------------------------//

struct TM12_AIProgram_Save
{
	SINGLE timer;
};

//--------------------------------------------------------------------------//
//
struct TM12_Briefing_Save
{
	enum
	{
        BRIEFING_STATE_INVALID = -1,

        BRIEFING_STATE_RADIO_BROADCAST,
        BRIEFING_STATE_ALARM,
        BRIEFING_STATE_HALSEY_1,
        BRIEFING_STATE_BLACKWELL_1,
        BRIEFING_STATE_ELAN_1,
        BRIEFING_STATE_BLACKWELL_2,
        BRIEFING_STATE_HALSEY_2,
        BRIEFING_STATE_BLACKWELL_3,
        BRIEFING_STATE_HALSEY_3,
        BRIEFING_STATE_UPLINK_TO_TERRA,
        BRIEFING_STATE_COMM,
        BRIEFING_STATE_HALSEY_4,
        BRIEFING_STATE_BENSON_1,
        BRIEFING_STATE_BLACKWELL_4,
        BRIEFING_STATE_HALSEY_5,
        BRIEFING_STATE_BLACKWELL_5,
        BRIEFING_STATE_HAWKES_1,
        BRIEFING_STATE_BLACKWELL_6,
        BRIEFING_STATE_BENSON_2,
        BRIEFING_STATE_BLACKWELL_7,
        BRIEFING_STATE_HALSEY_6,
        BRIEFING_STATE_OBJECTIVES,
        BRIEFING_STATE_FINISHED,

        NUM_BRIEFING_STATES

    } state;
};

//--------------------------------------------------------------------------//
struct TM12_InitialMantisAttacks_Save
{
	SINGLE timer;

	MPartRef XinSensor, RendarSensor, GPrime, GhelenGate1, GhelenGate2, XinSpawn, RenSpawn;
};

//--------------------------------------------------------------------------//
//
struct TM12_MissionStart_Save
{
	enum
	{
		Begin,
		Done
	} state;

};

//--------------------------------------------------------------------------//
//
struct TM12_QuasiliAttack_Save
{
	SINGLE timer;
};

//--------------------------------------------------------------------------//
//
struct TM12_CaptureUnits_Save
{

};

//--------------------------------------------------------------------------//
//
struct TM12_ProgressCheck_Save
{
	bool perilonReached, luxorReached, worFound, ghelenReached;

	bool blackwellPerilon,hawkesPerilon;
	
};

//--------------------------------------------------------------------------//
//
struct TM12_RepairedPlatform_Save
{
	
};

//--------------------------------------------------------------------------//
struct TM12_SmirnoffAttack_Save
{
	SINGLE timer;
	MPartRef RallyPoint;

	enum
	{
        PROG_STATE_INVALID = -1,

		PROG_STATE_SMIRNOFF,
		PROG_STATE_STEELE_1,
		PROG_STATE_SMIRNOFF_2,
		PROG_STATE_STEELE_2,
		PROG_STATE_SMIRNOFF_BUGGERS_OFF,
		PROG_STATE_TAKEI_1,
		PROG_STATE_STEELE_3,
		PROG_STATE_TAKEI_2,
		PROG_STATE_STEELE_4,
		PROG_STATE_VIVAC_1,
		PROG_STATE_TAKEI_3,
		PROG_STATE_VIVAC_2,
		PROG_STATE_STEELE_5,
		PROG_STATE_TAKEI_4,

        NUM_PROG_STATES

	} state;

    MPartRef shipsInFleet[12];

    U32 smirnoffWormhole;
};

//--------------------------------------------------------------------------//
struct TM12_ComeInBlackwell_Save
{
	enum
	{
        PROG_STATE_INVALID = -1,

        PROG_STATE_STEELE_1,
        PROG_STATE_BLACKWELL_1,
        PROG_STATE_STEELE_2,
        PROG_STATE_BLACKWELL_2,
        PROG_STATE_STEELE_3,
        PROG_STATE_BLACKWELL_3,

        PROG_STATE_WAIT_FOR_SOL,

        NUM_PROG_STATES

	} state;
};

//--------------------------------------------------------------------------//
//Mission Events

struct TM12_ObjectDestroyed_Save { };

//--------------------------------------------------------------------------//
//Mission Failures

struct TM12_MissionFailure_Save
{
	enum
	{
		Uplink,
		Halsey,
		Fail,
		Done
	} state;
};

//--------------------------------------------------------------------------//
//Mission Success

struct TM12_MissionSuccess_Save
{
	enum
	{
        PROG_STATE_INVALID = -1,

        PROG_STATE_HAWKES_1,
        PROG_STATE_BLACKWELL_1,
        PROG_STATE_HAWKES_2,
        PROG_STATE_NEWS,
        PROG_STATE_UPLINK,
        PROG_STATE_HALSEY,
        PROG_STATE_TELETYPE,
        PROG_STATE_DONE,

        NUM_PROG_STATES

	} state;
};

struct TM1_DisplayObjectiveHandlerData
{
    U32 stringID;
    U32 dependantHandle;
};

#endif
