#ifndef DATADEF_H
#define DATADEF_H
//--------------------------------------------------------------------------//
//                                                                          //
//                                DataDef.h                                 //
//				/Conquest/App/Src/Scripts/Script07T/DataDef.h				//
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
	Done
};
	
struct MissionData
{
	MissionState mission_state;

	bool mission_over, briefing_over, ai_enabled;
	U32 mhandle, shandle;

	bool bBuiltPropLab, bBuiltAWSLab;
	bool bBensonDied, bKerTakDied;
	bool bPlatInSys[11], bAITargeting, bHQInLemo;
	MPartRef Benson, KerTak, rebelharv;
	MPartRef SysCenter[11];

	U32 failureID;
};

//--------------------------------------------------------------------------//
//  PROGRAM DATA
//--------------------------------------------------------------------------//

struct TM7_AIProgram_Data
{
	U32 timer;
};

//--------------------------------------------------------------------------//
struct TM7_Start_Data
{
	
};

//--------------------------------------------------------------------------//
//

struct TM7_Briefing_Data
{
	SINGLE timer, timer2;
	U32 handle1, handle2, handle3, handle4;

	enum
	{
		Begin,
		TeleTypeLocation,
		Monitor,
		Ghelen,
		Ghelen2,
		Steele,
		Hawkes,
		Ghelen3,
		Hawkes2,
		Ghelen4,
		PreBriefing,
		Steele2,
		KerTak,
		Halsey,
		KerTak2,
		Halsey2,
		Objectives,
		Finish
	} state;
};

//--------------------------------------------------------------------------//
//
struct TM7_BriefingSkip_Data
{
	
};

//--------------------------------------------------------------------------//
//
struct TM7_MissionStart_Data
{
	SINGLE timer;

	enum
	{
		Begin,
		ShowLemo,
		Benson,
		NewObj,
		BuildPropulsion,
		BuildAWS,
		NewObj2,
		FinishAWS,
		Done
	} state;

};

//--------------------------------------------------------------------------//
//

struct TM7_MarchToLemo_Data
{
	U32 timer;
};

//--------------------------------------------------------------------------//
//Mission Events

struct TM7_ObjectDestroyed_Data { };
struct TM7_ObjectConstructed_Data { };
struct TM7_ForbiddenJump_Data { };
struct TM7_UnderAttack_Data { };

//--------------------------------------------------------------------------//
//Mission Failures

struct TM7_MissionFailure_Data
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

struct TM7_MissionSuccess_Data
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
