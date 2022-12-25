#ifndef DATADEF_H
#define DATADEF_H
//--------------------------------------------------------------------------//
//                                                                          //
//                                DataDef.h                                 //
//				/Conquest/App/Src/Scripts/Script13T/DataDef.h				//
//					MISSION SPECIFIC DATA DEFINITIONS						//
//                                                                          //
//               COPYRIGHT (C) 2000 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    Created:	6/19/00		JeffP
	Modified:	6/19/00		JeffP

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

/*
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
*/

#define NUM_SYSTEMS         1

//#define MAX_PLANETS 3*NUM_SYSTEMS


#define MAX_WAVE 64

#define ATTACK_SHIP 0
#define FAB_SHIP	1
#define TROOP_SHIP	2
#define RAM_SHIP	3


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

	bool mission_over, briefing_over, ai_enabled, next_state;
	U32 failureID;
	U32 mhandle, shandle;

	MPartRef Blackwell, Hawkes, Smirnoff, Malkor, locFleetEntry;
	MPartRef Earth, Mars, Jupiter, Saturn, Pluto, Neptune, Mercury, Venus, locInner;
	
	MPartRef locWaveEntry, locWaveEntry2, locWaveEntry3, locWaveEntry4;
	U32 numEnemy;
	MPartRef enemyShips[MAX_WAVE];
	U32 enemyShipType[MAX_WAVE], enemyShipTimer[MAX_WAVE];

	bool bFinalAttack, HQattack;
	bool bAIOnInSystem[NUM_SYSTEMS+1]; //, bGateOn[7];
	U32 attackTimer, numTotalCrew;

	U32 textFailureID;
};

//--------------------------------------------------------------------------//
//  PROGRAM DATA
//--------------------------------------------------------------------------//

struct TM13_AIProgram_Data
{
	U32 timer;
};

struct TM13_PrintTimeTillAttack_Data { };

//--------------------------------------------------------------------------//
struct TM13_Start_Data
{
	
};

//--------------------------------------------------------------------------//
//
struct TM13_Briefing_Data
{
	SINGLE timer;

	enum
	{
		Begin,
		TeleTypeLocation,
		RadioProlog,
		PreHalsey,
		Halsey,
		Hawkes,
		Objectives,
		Finish
	} state;
};

//--------------------------------------------------------------------------//
//
struct TM13_BriefingSkip_Data
{
	
};

//--------------------------------------------------------------------------//
//
struct TM13_MissionStart_Data
{
	SINGLE timer;

	enum
	{
		Begin,
		Hawkes1,
		Blackwell1,
		Done
	} state;

};

//--------------------------------------------------------------------------//
//
struct TM13_DefendEarth_Data
{
	SINGLE timer;

	enum
	{
		Begin,
		Done
	} state;

    int attackWave;
};

struct TM13_ControlWormholes_Data { };

//--------------------------------------------------------------------------//

struct TM13_SendWave_A_Data { U32 gateNum, gtimer; };
struct TM13_SendWave_B_Data { U32 gateNum, gtimer; };
struct TM13_SendWave_C_Data { U32 gateNum, gtimer; };
struct TM13_SendWave_D_Data { U32 gateNum, gtimer; };
struct TM13_SendWave_E_Data 
{ 
	bool bGateOn;
	U32 gateNum, timer, gtimer; 
	U32 numFabsSent;
};

struct TM13_SendUpgradedShips_Data 
{ 
	bool bGateOn;
	SINGLE timer, timer2, gtimer; 
	U32 gateNum;
};

struct TM13_SendWave_F_Data 
{ 
	bool bGateOn, bGateOn2;
	U32 gateNum, gateNum2, gtimer, gtimer2; 

	enum
	{
		Begin,
		Smirnoff1,
		Hawkes1,
		Smirnoff2,
		Blackwell1,
		Smirnoff3,
		Hawkes2,
		Malkor1,
		Hawkes3,
		Malkor2,
		Blackwell2,
		Done
	} state;
};

//--------------------------------------------------------------------------//

//Mission Events

struct TM13_ObjectDestroyed_Data { };
struct TM13_ObjectConstructed_Data { };
struct TM13_ForbiddenJump_Data { };
struct TM13_UnderAttack_Data { };

//--------------------------------------------------------------------------//
//Mission Failures

struct TM13_MissionFailure_Data
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

struct TM13_MissionSuccess_Data
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
//--------------------------------End DataDef.h-----------------------------//
//--------------------------------------------------------------------------//
#endif
