#ifndef DATADEF_H
#define DATADEF_H
//--------------------------------------------------------------------------//
//                                                                          //
//                                DataDef.h                                 //
//                                                                          //
//               COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//


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


//--------------------------------------------------------------------------//
//  MISSION DATA
//--------------------------------------------------------------------------//

enum MissionState
{
	Begin,
    Briefing,
	BlastFirst,
	BlastDone,
	Six_Hives,
	Six_Built,
	DestroyBase,
	BuildForce,
	Done
};
	
struct MissionData
{
	TECHNODE mission_tech;

	MissionState mission_state;

	bool mission_over, briefing_over;
	bool MillCreated, PlasmaCreated, PlayerInIode, NiadCreated;

	U32 mhandle, shandle, thandle;

	MPartRef TrainTrigger1, TrainTrigger2;
	MPartRef WarlordTrig1, WarlordTrig2;
	MPartRef Collector, Siphon, Cocoon, Weaver, Frig1, Frig2;
	MPartRef Corv1, Corv2, Corv3;
	MPartRef WormholeToIode;

    // variables used as parametrs when starting up the DisplayObjectiveHandler

    U32 displayObjectiveHandlerParams_stringID;
    U32 displayObjectiveHandlerParams_dependantHandle;
    U32 displayObjectiveHandlerParams_teletypeHandle;

};

//--------------------------------------------------------------------------//
//  PROGRAM DATA
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
struct MantisTrain_Briefing_Save
{

	enum
	{
		Begin,
		Radio,
		Radio2,
		Finish
	} state;
};

struct MantisTrain_Start_Save
{

};

struct MantisTrain_SelectAndMoveWeaver_Save
{
	enum
	{
		Begin,
		MoveWeaver,
		SelectAll,
		FOWuncovered,
		StartBuild,
		MissionFail
	}state;
};

struct MantisTrain_CollectorInProgress_Save
{
	
};

struct MantisTrain_CollectorFinished_Save
{
	
};

struct MantisTrain_SiphonInProgress_Save
{
	
};

struct MantisTrain_SiphonFinished_Save
{
	
};

struct MantisTrain_WarlordInProgress_Save
{
	
};

struct MantisTrain_WarlordFinished_Save
{
	
};

struct MantisTrain_EyestalkInProgress_Save
{
	
};

struct MantisTrain_EyestalkFinished_Save
{
	
};

struct MantisTrain_ThripidInProgress_Save
{
	
};

struct MantisTrain_ThripidFinished_Save
{
	
};

struct MantisTrain_ObjectBuilt_Save
{
	enum
	{
		Begin,
		Collector,
		Thripid,
		EyeStalk,
		Cocoon,
		Warlord,
		BlastFurnace,
		Done
	}state;
	
};

struct MantisTrain_HelionFirst_Save
{
	
};

struct MantisTrain_BlastInProgress_Save
{
	
};

struct MantisTrain_BlastFinished_Save
{
	MPartRef Spawn, Attacker, Attacker2, Attacker3;
	
	enum
	{
		Begin,
		Attack
	}state;
};

struct MantisTrain_BlastFirst_Save
{
	enum
	{
		Check,
		Done
	}state;
};

struct MantisTrain_SpitterInProgress_Save
{
	
};

struct MantisTrain_SpitterFinished_Save
{
	
};

struct MantisTrain_CarrionInProgress_Save
{
	
};

struct MantisTrain_CarrionFinished_Save
{
	
};

struct MantisTrain_SixScouts_Save
{
	
};

struct MantisTrain_PlantInProgress_Save
{
	
};

struct MantisTrain_PlantationFinished_Save
{
	
};

struct MantisTrain_JumpgateInProgress_Save
{
	
};


struct MantisTrain_JumpgateFinished_Save
{
	
};

struct MantisTrain_JumpToIode_Save
{
	MPartRef Spawn, Attacker, Attacker2, Attacker3, Attacker4, Attacker5;
	int x;
	
};

struct MantisTrain_NiadFinished_Save
{
	
};

struct MantisTrain_KillTheTerrans_Save
{
	MPartRef Spawn, Attacker, Attacker2, Attacker3, Attacker4;
	int x;

	enum
	{
		Ready,
		FinishOffMantis,
		DetectFinal,
		Begin,
		BuildCheck,
		EliminateTerrans,
		Success,
		Done
	}state;	
};

struct MantisTrain_BioforgeInProgress_Save
{
	
};

struct MantisTrain_BioforgeFinished_Save
{
	
};

struct MantisTrain_CheckForMissionFail_Save
{

	enum
	{
		Ready,
		Done
	}state;

};




struct MantisTrain_ObjectConstructed_Save
{
	
};

struct MT_DisplayObjectiveHandlerData
{
    U32 stringID;
    U32 dependantHandle;
};
//--------------------------------------------------------------------------//
//--------------------------------End DataDef.h-----------------------------//
//--------------------------------------------------------------------------//
#endif