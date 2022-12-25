#ifndef DATADEF_H
#define DATADEF_H
//--------------------------------------------------------------------------//
//                                                                          //
//                                DataDef.h                                 //
//				/Conquest/App/Src/Scripts/Script14T/DataDef.h				//
//					MISSION SPECIFIC DATA DEFINITIONS						//
//                                                                          //
//               COPYRIGHT (C) 2000 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    Created:	6/29/00		JeffP

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

#define NUM_SYSTEMS         13

//--------------------------------------------------------------------------//
//  MISSION DATA
//--------------------------------------------------------------------------//

enum MissionState
{
	Begin,
    Briefing,
	MissionStart,
	FindMalkor,
	Done
};
	
struct MissionData
{
	MissionState mission_state;

	bool mission_over, briefing_over, ai_enabled, next_state;
	U32 failureID;
	U32 mhandle, shandle, thandle;

	MPartRef Benson, Takei, Steele, Vivac, Halsey, Malkor, MalkorCocoon, elite1, elite2, halseyStartPoint;
    MPartRef gateToJahr, gateToQuasili;

	bool bGotCocoon, bClearedBases, bMalkorRun;
	bool bAIOnInSystem[NUM_SYSTEMS+1];

    // variables used as parametrs when starting up the DisplayObjectiveHandler

    U32 displayObjectiveHandlerParams_stringID;
    U32 displayObjectiveHandlerParams_dependantHandle;
    U32 displayObjectiveHandlerParams_teletypeHandle;

	U32 textFailureID;
};

//--------------------------------------------------------------------------//
//  PROGRAM DATA
//--------------------------------------------------------------------------//

struct TM14_AIProgram_Data
{
	U32 timer;
};

struct TM14_KillMantisPlats_Data { };

//--------------------------------------------------------------------------//
//
struct TM14_Briefing_Data
{
	SINGLE timer;

	enum
	{
		INVALID_BRIEFING_STATE = -1,

		BRIEFING_STATE_BEGIN,
		BRIEFING_STATE_TELETYPE,
		BRIEFING_STATE_TV1,
        BRIEFING_STATE_UPLINK,
		BRIEFING_STATE_HALSEY_1,
        BRIEFING_STATE_OBJECTIVES,
		BRIEFING_STATE_FINISHED,

		BRIEFING_STATE_DRAMATIC_PAUSE, // set postPauseState and dramaticPauseLength before using this state


		NUM_BRIEFING_STATES

	} state;
};

//--------------------------------------------------------------------------//
//
struct TM14_MissionStart_Data
{
	SINGLE timer;

	enum
	{
        PROGRAM_STATE_INVALID = -1,

        PROGRAM_STATE_BEGIN,
        PROGRAM_STATE_STEELE_1,
        PROGRAM_STATE_BENSON_1,
        PROGRAM_STATE_HALSEY_1,
        PROGRAM_STATE_VIVAC_1,
        PROGRAM_STATE_BENSON_2,
        PROGRAM_STATE_TAKEI_1,
        PROGRAM_STATE_HALSEY_2,
        PROGRAM_STATE_TAKEI_2,
        PROGRAM_STATE_HALSEY_3,
        PROGRAM_STATE_GROUP_HUG,
        PROGRAM_STATE_DONE,

        NUM_PROGRAM_STATES

	} state;

};

//--------------------------------------------------------------------------//
//
struct TM14_FindMalkor_Data
{
	enum
	{
        PROG_STATE_INVALID = -1,

        PROG_STATE_BEGIN,
        PROG_STATE_MALKOR_1,
        PROG_STATE_BENSON_1,
        PROG_STATE_MALKOR_2,
        PROG_STATE_TAKEI_1,
        PROG_STATE_STEELE_1,

        PROG_STATE_DONE,

        NUM_PROG_STATES

	} state;

};

//--------------------------------------------------------------------------//

struct TM14_HerdHalsey_Data
{
};

//--------------------------------------------------------------------------//

struct TM14_RunCowardRun_Data
{
	SINGLE timer;
    U32 malkorWormhole;

    enum
    {
        STATE_BEGIN,
        STATE_WARPING_OUT,
        STATE_DONE,

        NUM_STATES

    } state;
};

//--------------------------------------------------------------------------//

//Mission Events

struct TM14_ObjectDestroyed_Data { };
struct TM14_ObjectConstructed_Data { };
struct TM14_ForbiddenJump_Data { };
struct TM14_UnderAttack_Data { };

//--------------------------------------------------------------------------//
//Mission Failures

struct TM14_MissionFailure_Data
{
	enum
	{
		Begin,
		Halsey,
		Teletype,
		Done
	} state;
};

//--------------------------------------------------------------------------//
//Mission Success

struct TM14_MissionSuccess_Data
{
	enum
	{
		Begin,
		Takei,
		Halsey,
		Teletype,
		Done
	} state;
};

struct TM14_DisplayObjectiveHandlerData
{
    U32 stringID;
    U32 dependantHandle;
};

//--------------------------------------------------------------------------//
//--------------------------------End DataDef.h-----------------------------//
//--------------------------------------------------------------------------//
#endif
