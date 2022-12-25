#ifndef DATADEF_H
#define DATADEF_H
//--------------------------------------------------------------------------//
//                                                                          //
//                                DataDef.h                                 //
//				/Conquest/App/Src/Scripts/Script04T/DataDef.h				//
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

//--------------------------------------------------------------------------//
//  MISSION DATA
//--------------------------------------------------------------------------//

enum MissionState
{
	//Begin,
    Briefing,
	MissionStart,
	ScoutPerilon,
	KillRedMantis,
	TalkWithRebels,
	BuildPlatform,
	ProtectRebels,
	RebelsGoHome,
	InvestigateAlto,
	RetreatToCapella,
	Done
};
	
struct MissionData
{
	MissionState mission_state;

	bool mission_over, briefing_over;
	bool bBuiltPlatform;
	bool bLuxorAttack, bBekkaAttack, bAcademy, bHeavyInd, bWave;

	U32 mhandle, shandle, AlertID;
	U32 numRebelsDead, numWaves;

	MPartRef Blackwell, Hawkes;
	MPartRef Rebel1, Rebel2, Rebel3, Rebel4;
	MPartRef RebelGate, RebelGateAlto;

    // variables used as parametrs when starting up the DisplayObjectiveHandler

    U32 displayObjectiveHandlerParams_stringID;
    U32 displayObjectiveHandlerParams_dependantHandle;
    U32 displayObjectiveHandlerParams_teletypeHandle;

    U32 numExits;
    U32 exitID[64];

	U32 failureID;
};

//--------------------------------------------------------------------------//
//  PROGRAM DATA
//--------------------------------------------------------------------------//

struct TM4_Start_Data
{
	
};

//--------------------------------------------------------------------------//
//

struct TM4_Briefing_Data
{
	SINGLE timer;
	U32 handle1, handle2, handle3, handle4;

	enum
	{
		Begin,
		TeleTypeLocation,
		RadioBroadcast,
		RadioBroadcast2,
		PreHalseyBrief,
		HalseyBrief,
		HalseyBrief2,
		HalseyBrief3,
		DisplayMO,
		Finish
	}state;
};

//--------------------------------------------------------------------------//
//
struct TM4_BriefingSkip_Data
{
	
};

//--------------------------------------------------------------------------//

struct TM4_ControlProgram_Data 
{
	bool bMsg; 
};

//--------------------------------------------------------------------------//
//
struct TM4_MissionStart_Data
{
	SINGLE timer;

	enum
	{
		Begin,
		Talk1,
		Talk2,
		LookAtGate,
		LookAtGate2,
		Done
	}state;

};

//--------------------------------------------------------------------------//
//
struct TM4_ScoutPerilon_Data
{
	enum
	{
		Begin,
		Begin2,
		Talk1,
		Talk2,
		Done
	}state;
	
	U32 timer;
};

//--------------------------------------------------------------------------//
//
struct TM4_KillRedMantis_Data
{
	enum
	{
		Begin,
		NewObj,
		KillMantis,
		Done
	}state;

	U32 timer;
};

//--------------------------------------------------------------------------//
//

struct TM4_TalkWithRebels_Data
{
	enum
	{
		Begin,
		State1,
		State2,
		State3,
		State4,
		State5,
		State6,
		State7,
		State8,
		State9,
		State10,
		State11,
		State12,
		State13,
		//State14,
		Done
	} state;

};

//--------------------------------------------------------------------------//
//

struct TM4_BuildPlatform_Data
{
	enum
	{
		Begin,
		Done
	} state;

	U32 timer;
};

//--------------------------------------------------------------------------//
//
/*
struct TM4_RebelsHarvest_Data
{
	bool bReb1Out, bReb2Out, bReb3Out;
	U32 fakegas;
};
*/

//--------------------------------------------------------------------------//
//
struct TM4_SendBekkaWave_Data
{
	U32 timer;
};

//--------------------------------------------------------------------------//
//
struct TM4_SendLuxorWave_Data
{
	U32 timer;
	bool bFullFleet;
};

//--------------------------------------------------------------------------//
//
struct TM4_ProtectRebels_Data
{
	
};

//--------------------------------------------------------------------------//
//
struct TM4_RebelsGoHome_Data
{
	enum
	{
		Begin,
		Done
	} state;
};

//--------------------------------------------------------------------------//
//
struct TM4_InvestigateAlto_Data
{
	enum
	{
		Begin,
		NewObj,
		Investigate,
		Investigate2,
		Attack,
		Done
	} state;
	
	SINGLE timer;
};

//--------------------------------------------------------------------------//
//
struct TM4_RetreatToCapella_Data
{
	enum
	{
		Begin,
		NewObj,
		Retreating,
		Done
	} state;

	U32 timer;
};

//--------------------------------------------------------------------------//
//Mission Events

struct TM4_ObjectDestroyed_Data { };
struct TM4_ObjectConstructed_Data { };
struct TM4_ForbiddenJump_Data { };
struct TM4_UnderAttack_Data { };

//--------------------------------------------------------------------------//
//Mission Failures

struct TM4_BlackwellKilled_Data
{

};

struct TM4_HawkesKilled_Data
{

};

struct TM4_RebelsKilled_Data
{

};

struct TM4_RebelJGateDestroyed_Data
{

};


struct TM4_MissionFailure_Data
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

struct TM4_MissionSuccess_Data
{
	enum
	{
		Begin,
		Teletype,
		Done
	} state;
};

struct TM4_DisplayObjectiveHandlerData
{
    U32 stringID;
    U32 dependantHandle;
};

//--------------------------------------------------------------------------//
//--------------------------------End DataDef.h-----------------------------//
//--------------------------------------------------------------------------//
#endif
