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
   $Header: /Conquest/App/Src/Scripts/Script05T/DataDef.h 43    4/12/01 11:33a Tmauer $

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

#define PERILON_SYSTEM		1
#define LUXOR_SYSTEM		2
#define BEKKA_SYSTEM		3
#define PERILON2_SYSTEM		4
#define GHELEN3_SYSTEM		5
#define ALKAID_SYSTEM		6

#define MANTIS_WAVE_INTERVAL       ( 6 * 60 )  // five minutes between mantis wave attacks

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

	bool mission_over, briefing_over, BlackwellFirst;
	bool ScoutSent, BlackholeDeath, HangerBuilt, LuxCleared, WavesSent;
	bool HoleFound, TurnOff;

	U32 mhandle, shandle;

	MPartRef PerilonJumpGate, BlackholeAlert, Goodhole;

	MPartRef Luxor1, Luxor2;

	MPartRef WaveScout1, WaveScout2, WaveHive1;
	MPartRef WaveFrigate1, WaveFrigate2, WaveScarab1;

	MPartRef PlatformTrigger, JumpgateTrigger, BlackHole;
	MPartRef Blackwell, Hawkes;
	MPartRef Scout1, Scout2;
	MPartRef Hive1;
	MPartRef Frigate1,Frigate2;
	MPartRef Scarab;
	MPartRef CorvWaypoint, MissleWaypoint, BattleWaypoint, FabWaypoint, GoliathWaypoint;
	MPartRef KerTak;
	MPartRef PerilontoBekka, PerilontoLuxor, LuxortoPerilon, BekkatoPerilon, GhelentoLuxA, LuxtoGhelenA, GhelentoLuxB, LuxtoGhelenB;

	U32 failureID;
};


//--------------------------------------------------------------------------//
//  PROGRAM DATA
//--------------------------------------------------------------------------//

struct TM5_Start_Save
{
};

//--------------------------------------------------------------------------//
//
struct TM5_Briefing_Save
{	

	enum
	{
		Begin,
		TeleTypeLocation,
		Prolog,
		Uplink,
		Fuzz,
		HalseyBrief,
		MO,
		Finish
	}state;

};

//--------------------------------------------------------------------------//
//
struct TM5_AIManager_Save
{
	bool AIswitch;
};

//--------------------------------------------------------------------------//
//
struct TM5_BriefingSkip_Save
{
};

//--------------------------------------------------------------------------//
//
struct TM5_FlashBeacon_Save
{
	int count;
};


//--------------------------------------------------------------------------//
//
struct TM5_MissionStart_Save
{

	enum
	{
		Begin,
		Blackwell,
		Fleet,
		Blackwell2,
		Done
	}state;

};

//--------------------------------------------------------------------------//
//
struct TM5_LocateTheMantis_Save
{

};
//--------------------------------------------------------------------------//
//
struct TM5_PerilonCleared_Save
{

	MPartRef P1, P2, P3, P4;

	enum
	{
		Begin,
		Done
	}state;
};
//--------------------------------------------------------------------------//
//
struct TM5_LightShipyardStarted_Save
{

};
//--------------------------------------------------------------------------//
//
struct TM5_LightShipyardBuilt_Save
{

};
//--------------------------------------------------------------------------//
//
struct TM5_SendMantisWave1_Save
{
	SINGLE timer;
	MPartRef HiveGen, ScoutGen;
};
//--------------------------------------------------------------------------//
//
struct TM5_HeavyShipyardStarted_Save
{

};
//--------------------------------------------------------------------------//
//
struct TM5_HeavyShipyardBuilt_Save
{

};
//--------------------------------------------------------------------------//
//
struct TM5_SendMantisWave2_Save
{
	SINGLE timer;
	MPartRef FrigateGen, ScarabGen;
};
//--------------------------------------------------------------------------//
//
struct TM5_LuxorBase1Cleared_Save
{

	enum
	{
		Begin,
		Hawkes,
		NewMO,
		Done
	}state;
};
//--------------------------------------------------------------------------//
//
struct TM5_ShowGhelen3_Save
{


		enum
	{
		Start,
		Flash,
		Done
	}state;
};
//--------------------------------------------------------------------------//

struct TM5_HideWormhole_Save
{

};
//--------------------------------------------------------------------------//

struct TM5_Ghelen3BlackHole_Save
{
	bool BlackwellInsists;

	SINGLE timer;

};

//--------------------------------------------------------------------------//
//
struct TM5_RepeatBlackwellsMission_Save
{

};

//--------------------------------------------------------------------------//

struct TM5_KillOffBlackwell_Save
{
	enum
	{
		Begin,
		Hawkes,
		Blackwell,
		Done
	}state;

};
//--------------------------------------------------------------------------//

struct TM5_KerTakTrigger_Save
{

};
//--------------------------------------------------------------------------//

struct TM5_IntroduceKerTak_Save
{
	MPartRef target, KerTakWaypoint;

		enum
	{
		Begin,
		Hawkes,
		Kertak,
		Finish,
		Trick,
		Done,
		End
	}state;
};

//--------------------------------------------------------------------------//

struct TM5_ClearOutLuxor_Save
{
	MPartRef KerTakWaypoint;
	SINGLE timer;

	enum
	{
		Begin,
		Done
	}state;
};
//--------------------------------------------------------------------------//
/*
struct TM5_ConfirmWithKertak_Save
{

};
//--------------------------------------------------------------------------//

struct TM5_SpeakWithKertak_Save
{

};
*/
//--------------------------------------------------------------------------//

struct TM5_ObjectDestroyed_Save
{

};

//--------------------------------------------------------------------------//

struct TM5_ObjectConstructed_Save
{

};

//--------------------------------------------------------------------------//
//Mission Failures

struct TM5_HQDestroyed_Save
{

};
struct TM5_BlackwellKilled_Save
{

};
struct TM5_HawkesKilled_Save
{

};
struct TM5_KerTakKilled_Save
{

};

struct TM5_MissionFailure_Save
{
	enum
	{
		Begin,
		Halsey,
		Tele,
		Done
	} state;
};

struct TM5_MissionSuccess_Save
{
	enum
	{
		Begin,
		HalseyTalk,
		PrintSuccess,
		Done
	} state;
};


//--------------------------------------------------------------------------//
//--------------------------------End DataDef.h-----------------------------//
//--------------------------------------------------------------------------//
#endif