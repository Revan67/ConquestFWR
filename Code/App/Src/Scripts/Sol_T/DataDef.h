
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
	Done
};
	
struct MissionData
{	
	TECHNODE mission_tech;

	MissionState mission_state;

	bool mission_over, briefing_over;
	bool MillCreated, EutroCreated, PlayerInKasse, GreaterPavilionCreated, BunkerCreated, BunkCreated;

	U32 mhandle, shandle, thandle;

	MPartRef TrainTrigger1, WarnTrigger, TrainTrigger2, TrainTrigger3, TrainTrigger4;
	MPartRef Forger, Taos1, Taos2, Acropolis;
	MPartRef Oxidator, Galiot;
	MPartRef Alert;
	
	U32 displayObjectiveHandlerParams_stringID;
    U32 displayObjectiveHandlerParams_dependantHandle;
    U32 displayObjectiveHandlerParams_teletypeHandle;
};

//--------------------------------------------------------------------------//
//  PROGRAM DATA
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
struct SolTrain_Briefing_Save
{

	enum
	{
		BRIEFING_STATE_RADIO_BROADCAST,
		BRIEFING_STATE_ELAN_BRIEF,
		BRIEFING_STATE_OBJECTIVES,
		BRIEFING_STATE_FINISHED
	} state;
};

struct SolTrain_Start_Save
{
	
};

struct SolTrain_SelectAndMoveForger_Save
{
	enum
	{
		Begin,
		MoveForger,
		SelectAll,
		FOWuncovered,
		StartBuild,
		MissionFail
	}state;
};

struct SolTrain_OxiInProgress_Save
{
	
};

struct SolTrain_OxiFinished_Save
{
	
};

struct SolTrain_GaliotInProgress_Save
{
	
};

struct SolTrain_GaliotFinished_Save
{
	
};

struct SolTrain_BunkerInProgress_Save
{
	
};

struct SolTrain_BunkerFinished_Save
{
	
};

struct SolTrain_STowerInProgress_Save
{
	
};

struct SolTrain_STowerFinished_Save
{
	
};

struct SolTrain_PavilionInProgress_Save
{
	
};

struct SolTrain_PavilionFinished_Save
{
	
};

struct SolTrain_HelionFirst_Save
{
	
};

struct SolTrain_HelionInProgress_Save
{
	
};

struct SolTrain_HelionFinished_Save
{
	MPartRef Spawn, Attacker, Attacker2, Attacker3;
	
	enum
	{
		Begin,
		Attack
	}state;
};

struct SolTrain_TeslaInProgress_Save
{
	
};

struct SolTrain_TeslaFinished_Save
{
	
};

struct SolTrain_CitadelInProgress_Save
{

};

struct SolTrain_CitadelFinished_Save
{
	
};

struct SolTrain_GPavilionInProgress_Save
{
	
};

struct SolTrain_GPavilionFinished_Save
{
	
};

struct SolTrain_AnnexInProgress_Save
{
	
};

struct SolTrain_AnnexFinished_Save
{
	
};

struct SolTrain_AnvilInProgress_Save
{
	
};

struct SolTrain_AnvilFinished_Save
{
	
};

struct SolTrain_DockInProgress_Save
{
	
};

struct SolTrain_DockFinished_Save
{
	
};
struct SolTrain_XenoInProgress_Save
{
	
};

struct SolTrain_XenoFinished_Save
{
	
};

struct SolTrain_SixPolaris_Save
{
	
};

struct SolTrain_EutroInProgress_Save
{
	
};

struct SolTrain_EutroFinished_Save
{
	
};

struct SolTrain_JumpgateInProgress_Save
{
	
};


struct SolTrain_JumpgateFinished_Save
{
	
};

struct SolTrain_JumpToKasse_Save
{
	MPartRef Spawn, Spawn1, Spawn2, Attacker, Attacker2, Attacker3, Attacker4, Attacker5, Attacker6, 
		Attacker7, Attacker8, Attacker9, Attacker10, Attacker11, Attacker12;
	int x;
	enum
	{
		Begin,
		Wave,
		Wave2,
		Attack
	}state;
};

struct SolTrain_4and2_Save
{
	enum
	{
		Built,
		Matrix
	}state;
};

struct SolTrain_TMatrixInProgress_Save
{
	
};

struct SolTrain_TMatrixFinished_Save
{
	
};

struct SolTrain_KillTheMantis_Save
{	
	enum
	{
		Begin,
		FinishOffMantis,
		Success,
		Done
	}state;	
};

struct SolTrain_CheckForMissionFail_Save
{
	enum
	{
		Ready,
		Done
	}state;

};


struct SolTrain_ObjectConstructed_Save
{
	
};
struct TM1_DisplayObjectiveHandlerData
{
    U32 stringID;
    U32 dependantHandle;
};
//--------------------------------------------------------------------------//
//--------------------------------End DataDef.h-----------------------------//
//--------------------------------------------------------------------------//
#endif