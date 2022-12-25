#ifndef DATADEF_H
#define DATADEF_H
//--------------------------------------------------------------------------//
//                                                                          //
//                                DataDef.h                                 //
//                                                                          //
//               COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
   $Header: /Conquest/App/Src/Scripts/Script08T/DataDef.h 24    4/13/01 10:36a Tmauer $

	Mission specific data definitions.
*/
//--------------------------------------------------------------------------//

#ifndef US_TYPEDEFS
#include <typedefs.h>
#endif

#ifndef MPARTREF_H
#include <MPartRef.h>
#endif


#ifndef DMTECHNODE_H
#include <..\..\dinclude\DMTechNode.h>
#endif


#define PLAYER_ID			1
#define MANTIS_ID			2
#define REBEL_ID			4

#define LEMO				1
#define AEON				2
#define GRAAN				3
#define ARCTURUS			4
#define KRACUS				5
#define KALIDON				6
#define SENDLOR				7
#define VARN				8
#define TALOS				9
#define GROG				10
#define	ZHAL				11
#define	SERPCE				12
#define GERA				13
#define CENTAURUS			14

#define MANTIS_WAVE_INTERVAL      300  // ( 4 * 60 )  four minutes between mantis wave attacks
#define MANTIS_WAVE_INTERVAL2	  180  // ( 3 * 60 )

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

	U8 HIGHMANTIS, LOWMANTIS, HIGHPLAYER, LOWPLAYER;

	U32 mhandle, shandle;

	bool mission_over, briefing_over;
	bool HasKracusAttacked, HasSendlorAttacked, HasTalosAttacked;
	bool BensonFound, BlackwellDone;

	MPartRef KerTak, Benson, Blackwell, Elan, BensonWP;
	MPartRef BensonTrigger;
	MPartRef Frigate1, Frigate2, Frigate3, Frigate4, Frigate5, Frigate6, Frigate7, Frigate8;
	MPartRef Hive1, Hive2, Hive3, Hive4;
	MPartRef Scarab1, Scarab2, Scarab3, Scarab4;

	U32 failureID;
};


//--------------------------------------------------------------------------//
//  PROGRAM DATA
//--------------------------------------------------------------------------//

struct TM8_Start_Save
{

};

//--------------------------------------------------------------------------//
//
struct TM8_Briefing_Save
{	

	enum
	{
		Begin,
		TeleTypeLocation,
		Radio,
		Radio1,
		Uplink,
		Fuzz,
		Radio2,
		Radio3,
		HalseyBrief,
		MO,
		Finish
	}state;

};

//--------------------------------------------------------------------------//
//
struct TM8_AIManager_Save
{
	SINGLE AItimer, Crewtimer;

	MPartRef L1, L4;
	float L1Crew, L4Crew; //Lemo Prime and Lemo 4
};

//--------------------------------------------------------------------------//
//
struct TM8_BriefingSkip_Save
{
};

//--------------------------------------------------------------------------//
//
struct TM8_MissionStart_Save
{
	SINGLE timer;
	bool flag;

	enum
	{
		Begin,
		Done
	}state;

};

//--------------------------------------------------------------------------//

struct TM8_AeonVisited_Save
{
	
};

//--------------------------------------------------------------------------//

struct TM8_NearKalidon_Save
{
	MPartRef Gate1, Gate2;

	bool AskedForHelp;

	enum
	{
		Asked,
		Next,
		End
	}state;

};

//--------------------------------------------------------------------------//

struct TM8_InKalidon_Save
{

	enum
	{
		Begin,
		KerTak,
		Benson,
		End
	}state;
};

//--------------------------------------------------------------------------//
//
struct TM8_FlashBeacon_Save
{
	int count;
};

//--------------------------------------------------------------------------//

struct TM8_BensonDeathClock_Save
{
	SINGLE timer;	
};

//--------------------------------------------------------------------------//

struct TM8_BensonSeen_Save
{
	MPartRef Frig1, Frig2;
	MPartRef FrigWP, Frig2WP;

	SINGLE timer;

	enum
	{
		Begin,
		KillFrigate1,
		KillFrigate2,
		Resupply,
		NewMO,
		Ob2,
		Ob3
	}state;
};

//--------------------------------------------------------------------------//

struct TM8_SendlorWaveAttack_Save
{
	SINGLE timer;
	MPartRef ScoutGen;

};

//--------------------------------------------------------------------------//

struct TM8_KracusWaveAttack_Save
{
	SINGLE timer;
	MPartRef FrigateGen;
};

//--------------------------------------------------------------------------//

struct TM8_SecondSendlorWaveAttack_Save
{
	SINGLE timer;
	MPartRef ScarabGen;

};

//--------------------------------------------------------------------------//

struct TM8_VarnWaveAttack_Save
{
	SINGLE timer;
	MPartRef HiveGen;

};

//--------------------------------------------------------------------------//

struct TM8_TalosWave_Save
{
	SINGLE timer;
	MPartRef target, SolAttacker1, SolAttacker2, SolAttacker3, SolAttacker4;
	MPartRef BlackwellWP, ElanWP, TaosWP, TriremeWP, TalosWaveWaypoint;

	enum
	{
		Begin,
		EnemySighted,
		BensonSpeaks,
		KerTakPanic,
		BringInBlackwell,
		Benson,
		Blackwell,
		Destabilizer,
		Fire,
		BensonFreaks,
		BlackwellExplains,
		BensonRetorts,
		BlackwellShutsUp,
		Done,

	}state;
};

//--------------------------------------------------------------------------//

struct TM8_BlowUpSendlor_Save
{

};

//--------------------------------------------------------------------------//

struct TM8_FinishOffTalosWave_Save
{
	enum
	{
		BlackwellSpeaks,
		Benson,
		KerTak,
		Done
	}state;
};

//--------------------------------------------------------------------------//

struct TM8_ClearOutVarn_Save
{

};

//--------------------------------------------------------------------------//

struct TM8_UnderAttack_Save
{

};

//--------------------------------------------------------------------------//

struct TM8_ObjectDestroyed_Save
{

};

//--------------------------------------------------------------------------//
//Mission Failures


struct TM8_BensonKilled_Save
{
	enum
	{
		Begin,
		Uplink,
		Done
	}state;
};


struct TM8_KerTakKilled_Save
{
	enum
	{
		Begin,
		Uplink,
		Done
	}state;
};

struct TM8_ForwardBaseLost_Save
{

};


struct TM8_MissionFailure_Save
{
	enum
	{
		Begin,
		Done
	} state;
};

struct TM8_MissionSuccess_Save
{
	enum
	{
		Begin,
		HalseyTalk,
		BensonTalk,
		Halsey2,
		PrintSuccess,
		Done
	} state;
};


//--------------------------------------------------------------------------//
//--------------------------------End DataDef.h-----------------------------//
//--------------------------------------------------------------------------//
#endif