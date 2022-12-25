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
   $Header: /Conquest/App/Src/Scripts/Script06T/DataDef.h 21    4/12/01 4:25p Tmauer $

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
#define SOL_ID				3
#define REBEL_ID			4

#define LUXOR				1
#define GHELEN				2
#define WOR					3
#define CORLAR				4
#define XIN					5
#define	RENDAR				6
#define CENTAURUS			7
#define LEMO				8
#define KRACUS				9
#define CAPELLA				10

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

	bool mission_over, briefing_over;
	bool SentryBaseDefeated;
	bool XinFreed, RendarFreed, CorlarFreed, AllFreed;
	bool HoleFound, CollectorSeen;


	SINGLE ExecutionTimer;

	U32 mhandle, shandle;
	int Troopships, RebelsToGo;

	MPartRef KerTak, Hawkes, Steele, Siphon;
	MPartRef Group1WP, Group2WP, KerTakWP;
	MPartRef BriefJump;
	MPartRef Thripid1, Thripid2, XinNiad, RendarNiad, CorlarNiad1, CorlarNiad2;
	MPartRef XinWarlord, RendarWarlord, CorlarWarlord;
	MPartRef XinCamp, RendarCamp, CorlarCamp, MalkorBase;
	MPartRef WorCollector,WorJumpGate;
	MPartRef WorP, Wor2;

	U32 failureID;
};


//--------------------------------------------------------------------------//
//  PROGRAM DATA
//--------------------------------------------------------------------------//

struct TM6_Start_Save
{
};

//--------------------------------------------------------------------------//
//
struct TM6_Briefing_Save
{	

	enum
	{
		Begin,
		TeleTypeLocation,
		Conference,
		Uplink,
		Fuzz,
		Radio,
		Radio1,
		Radio2,
		Radio3,
		Radio4,
		Radio5,
		Radio6,
		Radio7,
		Radio8,
		Radio9,
		Radio10,
		Radio11,
		Radio12,
		MO,
		Finish
	}state;

};

//--------------------------------------------------------------------------//
//
struct TM6_AIManager_Save
{
};

//--------------------------------------------------------------------------//
//
struct TM6_BriefingSkip_Save
{
};

//--------------------------------------------------------------------------//
//
struct TM6_MissionStart_Save
{

	enum
	{
		Begin,
		Done
	} state;

};

//--------------------------------------------------------------------------//
//
struct TM6_EnemyThripidControl_Save
{
	SINGLE scarabtimer, frigatetimer, hivetimer, scouttimer;
};

//--------------------------------------------------------------------------//
//
struct TM6_FindMalkorBase_Save
{
};

//--------------------------------------------------------------------------//
//
struct TM6_DisableMalkorBase_Save
{
	enum
	{
		Begin,
		Next,
		Done,
		NewMO,
		Ob2,
		Ob3
	} state;
};

//--------------------------------------------------------------------------//
//
struct TM6_BlackHoleSeen_Save
{
	enum
	{
		Begin,
		Done
	} state;

};

//--------------------------------------------------------------------------//

struct TM6_ObjectDestroyed_Save
{

};

//--------------------------------------------------------------------------//
//
struct TM6_Troopshipped_Save
{
};

//--------------------------------------------------------------------------//

struct TM6_NewMO_Save
{

};

//--------------------------------------------------------------------------//

struct TM6_CorlarCampFreed_Save
{

};

//--------------------------------------------------------------------------//

struct TM6_RendarCampFreed_Save
{

};

//--------------------------------------------------------------------------//

struct TM6_XinCampFreed_Save
{

};

//--------------------------------------------------------------------------//

struct TM6_AllRebelsFreed_Save
{
	MPartRef SiphonSpawn;

	enum
	{
		Begin,
		Next,
		Done,
		Final
	} state;
};

//--------------------------------------------------------------------------//

struct TM6_JumpGateEncounterToWor_Save
{
	MPartRef Collector;
	
	enum
	{
		GateSeen,
		Hawkes,
		CollectorSeen
	} state;
};

//--------------------------------------------------------------------------//

struct TM6_MimicAttack_Save
{
	MPartRef Frig1, Frig2, Frig3, Frig4, Frig5, Frig6, Frig7, Frig8;
	SINGLE timer;

	enum
	{
		Begin,
		Move,
		Reveal,
		Reveal2,
		End
	} state;
};

//--------------------------------------------------------------------------//

struct TM6_RebelsAreHome_Save
{
	bool XinWarlordHome, CorlarWarlordHome, RendarWarlordHome;
};

//--------------------------------------------------------------------------//

struct TM6_ForbiddenJump_Save
{

};

//--------------------------------------------------------------------------//
//Mission Failures
struct TM6_HawkesKilled_Save
{

};

struct TM6_SteeleKilled_Save
{

};

struct TM6_KerTakKilled_Save
{

};

struct TM6_RebelKilled_Save
{

};

struct TM6_MissionFailure_Save
{
	enum
	{
		Begin,
		Done
	} state;
};

struct TM6_MissionSuccess_Save
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