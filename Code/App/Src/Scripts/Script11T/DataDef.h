#ifndef DATADEF_H
#define DATADEF_H
//--------------------------------------------------------------------------//
//                                                                          //
//                                DataDef.h                                 //
//				/Conquest/App/Src/Scripts/Script11T/DataDef.h				//
//					MISSION SPECIFIC DATA DEFINITIONS						//
//                                                                          //
//               COPYRIGHT (C) 2000 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
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
#define MANTIS_ID6			6
#define REBEL_ID			4

#define LEMO				1
#define AEON				2
#define GRAAN				3
#define ARCTURUS			4
#define KRACUS				5
#define KALIDON				6
#define SENDLOR				7
#define VARN				8
#define GROG				9
#define CENTAURUS			10
#define	CORLAR				11
#define	EPSILON				12
#define ABELL				13
#define PASEO				14
#define	TARIFF				15
#define NATUS				16

#define NUM_SYSTEMS         16
#define NUM_DISABLED_PLATS	7

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
	
	bool briefing_over, mission_over;
	bool WaveSent, AttackSent, PartOneDone, PartTwoDone, HomeFree;
	bool HurtPlatSeen, HQDone;

	U32 mhandle, shandle;
	U32 NumOfPlats;

	MPartRef dHQ, dRepair, dRefinery, dLight, dSensor, dHeavy, dAcademy;
	MPartRef Steele, Blackwell, Hawkes, Vivac, Takei;
	MPartRef PaseoGate, TariffGate, EpsilonGate, CorlarGate, VarnGate, KalidonGate, SendGate, Kal1, Kal2, Kal3;
	MPartRef PlatArray[NUM_DISABLED_PLATS];

	U32 failureID;
};

//--------------------------------------------------------------------------//
//  PROGRAM DATA
//--------------------------------------------------------------------------//

struct TM11_AIProgram_Save
{
};

//--------------------------------------------------------------------------//
struct TM11_Start_Save
{
	
};

//--------------------------------------------------------------------------//
//
struct TM11_Briefing_Save
{

	enum
	{
		Begin,
		Radio,
		Uplink,
		Fuzz,
		Radio2,
		MO,
		Finish
	} state;
};

//--------------------------------------------------------------------------//
//
struct TM11_BriefingSkip_Save
{
	
};

//--------------------------------------------------------------------------//
//
struct TM11_InGameScene_Save
{
	MPartRef SceneBlackwell, SceneSmirnoff, SceneLeech, SceneHawkes;
	SINGLE timer;

	enum
	{
		Begin,
		Smirnoff,
		Hawkes,
		Smirnoff2,
		Hawkes2,
		Leech,
		Smirnoff3,
		Officer,
		Smirnoff4,
		Malkor,
		Smirnoff5,
		Malkor2,
		Hawkes3,
		Smirnoff6,
		Hawkes4,
		Smirnoff7,
		Blackwell,
		FireDestab,
		Fire,
		HawkesFree,
		FlyAway,
		BlowupSmirnoff,
		End
	}state;
};

//--------------------------------------------------------------------------//
//
struct TM11_MissionStart_Save
{
	enum
	{
		Begin,
		Done
	} state;

};

//--------------------------------------------------------------------------//
//
struct TM11_CaptureUnits_Save
{
	bool FirstUnitRescued;
};

//--------------------------------------------------------------------------//
struct TM11_SendAttack_Save
{
	
};

//--------------------------------------------------------------------------//
//
struct TM11_AttackOnTerrans_Save
{
	SINGLE timer;
};

//--------------------------------------------------------------------------//
struct TM11_BlackwellsEntrance_Save
{
	MPartRef HawkesWP, BlackwellWP;

	enum
	{
		Begin,
		OnScreen,
		Blackwell,
		Steele,
		Blackwell2,
		Steele2,
		Blackwell3,
		Steele3,
		BlackwellOut,
		NewMO,
		Ob2,
		Ob3,
		Hawkes,
		Done
	}state;

};

//--------------------------------------------------------------------------//
//
struct TM11_ViewBlackwell_Save
{
	enum
	{
		Begin,
		Done
	} state;

};

//--------------------------------------------------------------------------//
struct TM11_MissionPartOneFinished_Save
{
	bool GatesDone, UnitsGone, AdmalSafe;

	enum
	{
		Begin,
		Steele,
		EnterEpsilon,
		Jumpgates
	} state;	
};

//--------------------------------------------------------------------------//
struct TM11_MissionPartTwoFinished_Save
{
	enum
	{
		Begin,
		EnterCorlar
	} state;	
};

//--------------------------------------------------------------------------//
//
struct TM11_FirstTerranPlatSeen_Save
{
	enum
	{
		Begin,
		Done
	} state;

};

//--------------------------------------------------------------------------//
struct TM11_RepairedPlatform_Save
{

};

//--------------------------------------------------------------------------//
struct TM11_AttackPlatform_Save
{
	SINGLE timer;

};

//--------------------------------------------------------------------------//
struct TM11_FabRunning_Save
{

};

//--------------------------------------------------------------------------//
//Mission Events

struct TM11_ObjectDestroyed_Save { };
struct TM11_ObjectConstructed_Save { };
struct TM11_UnderAttack_Save { };

//--------------------------------------------------------------------------//
//Mission Failures

struct TM11_MissionFailure_Save
{
	enum
	{
		Uplink,
		Halsey,
		Fail,
		Done
	} state;
};

//--------------------------------------------------------------------------//
//Mission Success

struct TM11_MissionSuccess_Save
{
	enum
	{
		Uplink,
		Halsey,
		Success,
		Done
	} state;
};

//--------------------------------------------------------------------------//
//--------------------------------End DataDef.h-----------------------------//
//--------------------------------------------------------------------------//
#endif
