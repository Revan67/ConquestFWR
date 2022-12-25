#ifndef DATADEF_H
#define DATADEF_H
//--------------------------------------------------------------------------//
//                                                                          //
//                                DataDef.h                                 //
//				/Conquest/App/Src/Scripts/Script02T/DataDef.h				//
//					MISSION SPECIFIC DATA DEFINITIONS						//
//                                                                          //
//               COPYRIGHT (C) 2000 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    Created:	4/17/00		JeffP
	Modified:	4/28/00		JeffP

*/
//--------------------------------------------------------------------------//

#ifndef US_TYPEDEFS
#include <typedefs.h>
#endif

#ifndef MPARTREF_H
#include <MPartRef.h>
#endif

//#ifndef DMTECHNODE_H
//#include <..\..\dinclude\DMTechNode.h>
//#endif

//--------------------------------------------------------------------------//
//  MISSION DATA															//
//--------------------------------------------------------------------------//

enum MissionState
{
	Begin,
	Briefing,
	Starting,
	//BuildDepot,
	BuildBallistics,
	BuildCruisers,
	//BuildGate,
	JumpToAlkaid,
	MantisAttack,
	ScoutAlkaid,
	MantisBase,
	/*
	FrigateRuns,
	SalvageFrigate,
	TakeFrigateToHQ,
	*/
	Done
};
	
struct MissionData
{
	MissionState mission_state;
	bool mission_over, next_state, bNoControl;
	bool bJumpgateAttacked, bJumpgateDestroyed, bOutpostBegun, bOutpostDone, bTroopDone;
	bool mantisgate_destroyed, bBallStart, bBallDone, bFleetReady, bHurtBase, bNewObj8, bPlayed;
	U32 mhandle, shandle, num_mantis_ships;
	S32 numMantisEyeStocksLeft;
	MPartRef corvette1, corvette2, corvette3, corvette4, corvette5, corvette6;
	MPartRef Blackwell; //, FrigateRunner;
	MPartRef Scout1, Scout2;
	MPartRef PlanetTrigger, PlanetTrigger2, PlanetTrigger3, WormholeTrigger, WormholeTriggerB, WormholeTrigger2;
	MPartRef OutpostTrigger, OutpostTrigger2, OutpostTrigger3;
	MPartRef AsteroidTrigger, AsteroidTrigger2;
	MPartRef AlkTwoTrigger; //AlkGateTrigger
	//MPartRef NebulaTrigger, NebulaTrigger2, HQTrigger;
	MPartRef Jumpgate_TC2ALK, Jumpgate_ALK2TC;

	U32 failureID;
};

//--------------------------------------------------------------------------//
//  PROGRAM DATA															//
//--------------------------------------------------------------------------//

struct TM2_Start_Data { };
struct TM2_GlobalProc_Data { bool bGotToMaxCP; };

struct TM2_startedoutpost_Data { };
struct TM2_finishedoutpost_Data { };
struct TM2_builttroopship_Data { };

//--------------------------------------------------------------------------//

struct TM2_Briefing_Data
{
	SINGLE timer;
	U32 handle1, handle2, handle3, handle4;

	enum 
	{
		Begin,
		Begin2,
		Begin3,
		Begin4,
		Begin5,
		PreHalseyBrief,
		HalseyBrief,
		Beacon,
		Beacon2,
		Beacon3,
		HalseyBrief2,
		HalseyBrief3,
		DisplayMO,
		Finish
	} state;


};

//--------------------------------------------------------------------------//
//
struct TM2_InGameScene_Save
{
	MPartRef Duck1, Duck2, SceneWeaver, Target;
	MPartRef AlkSpawn, TauWayPoint, GatePoint;
	SINGLE timer;

	enum
	{
		Begin,
		AttackJumpgate,
		SendMantis,
		LeaveTau,
		Done,
		End
	}state;
};

//--------------------------------------------------------------------------//

struct TM2_BriefingSkip_Data
{
	
};

//--------------------------------------------------------------------------//

struct TM2_MissionStart_Data
{

};

//--------------------------------------------------------------------------//

struct TM2_AlkPrimeVisible_Data
{

};


//--------------------------------------------------------------------------//


struct TM2_nearwormhole_Data { 
	U32 timer; 
	//MPartRef Mantis1, Mantis2, Mantis3;
};

struct TM2_TroopStarted_Data { };
struct TM2_starteddepot_Data { };


//--------------------------------------------------------------------------//

struct TM2_BuildBallistics_Data
{
	enum
	{
		PreBegin,
		PreBegin2,
		Begin,
		Building,
		Done
	} state;
};

struct TM2_startedballistics_Data { };
struct TM2_finishedballistics_Data { };

//--------------------------------------------------------------------------//

struct TM2_BuildCruisers_Data
{
	enum
	{
		PreBegin,
		Begin,
		Building,
		Done
	} state;
};

//--------------------------------------------------------------------------//

struct TM2_JumpToAlkaid_Data
{
	enum
	{
		Begin,
		Jump,
		Blackwell,
		Done
	} state;
};

//--------------------------------------------------------------------------//

struct TM2_MantisAttack_Data
{
	enum
	{
		Begin,
		Wave1,
		Wave2,
		Done
	} state;

	U32 timer;
	MPartRef locAlkWormhole;
	//MPartRef Frig1, Frig2, Frig3, Frig4, Frig5, Frig6;
};

//--------------------------------------------------------------------------//

struct TM2_ScoutAlkaid_Data
{
	bool bFoundScouts;

	enum
	{
		Begin,
		Done
	} state;

};

struct TM2_foundasteroid_Data { };

/*
struct TM2_foundalkone_Data { };
struct TM2_fabjumpedin_Data { };
struct TM2_fabnearalkone_Data { MPartRef Frig1, Frig2, Frig3, Frig4; };
*/

//--------------------------------------------------------------------------//

struct TM2_MantisBase_Data
{
	enum
	{
		Begin,
		Gawking,
		NewObj,
		AIControlled,
		Done
	} state;

	U32 timer;
};

//--------------------------------------------------------------------------//

struct TM2_ObjectBuilt_Data
{

};

//--------------------------------------------------------------------------//

struct TM2_ObjectDestroyed_Data
{

};

struct TM2_jumpgateattacked_Data { };
struct TM2_jumpgatedestroyed_Data { };
struct TM2_mantisgatedestroyed_Data { U32 timer; };

//--------------------------------------------------------------------------//

struct TM2_ForbiddenJump_Data
{
	MPartRef gate4, gate2;
};

//--------------------------------------------------------------------------//

struct TM2_BlackwellKilled_Data 
{ 

	enum
	{
		Begin,
		Done
	} state;
};

struct TM2_HQDestroyed_Data { };
struct TM2_FrigateRunDestroyed_Data 
{ 
	enum
	{
		Begin,
		Done
	} state;
};

struct TM2_MissionFailure_Data
{
	enum
	{
		Begin,
		Done
	} state;
};

struct TM2_MissionSuccess_Data
{
	enum
	{
		Marine,
		Blackwell,
		Marine2,
		Intel,
		PreHalsey,
		Halsey,
		TeleType,
		Done
	} state;
};

//--------------------------------End DataDef.h-----------------------------//

#endif
