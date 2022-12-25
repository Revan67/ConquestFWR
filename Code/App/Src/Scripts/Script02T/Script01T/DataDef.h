//--------------------------------------------------------------------------//
//                                                                          //
//                                DataDef.h                                 //
//                                                                          //
//               COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
   $Header: /Conquest/App/Src/Scripts/Script01T/DataDef.h 47    4/12/01 11:00a Tmauer $

	Mission specific data definitions.
*/
//--------------------------------------------------------------------------//

#ifndef DATADEF_H

#define DATADEF_H

#ifndef US_TYPEDEFS
#include <typedefs.h>
#endif

#ifndef MPARTREF_H
#include <MPartRef.h>
#endif

#ifndef DMTECHNODE_H
#include <..\..\dinclude\DMTechNode.h>
#endif

enum MissionState
{
	StartMission,
	AttackOnTNSAustin,
	RetrieveBlackbox,
	MantisInvasion,
	TauCetiAttack,
	HarvesterAttack,
	DefendHarvester,
	IntoMantisSpace,
	GetBeacon,
	BeaconRecovered,
	MissionOver
};
	
struct MissionData
{

	TECHNODE mission_tech;

	MissionState mission_state;
	
	bool mission_over, bFlashBeacon, gameAIEnabled;

	MPartRef WayPoint1, WayPoint2, WayPoint3;	//starting locations of your ships (including Blackwell's)

	MPartRef Corvette1, Corvette2;
	MPartRef Blackwell;

	MPartRef MantisGenWormhole;

	MPartRef PlanetTrigger;
	MPartRef WormholeTrigger;

    MPartRef alkaidPrimeTrigger;

	MPartRef TNSAustin;
	MPartRef Blackbox, Beacon;
	MPartRef BlackboxTrigger;

	MPartRef HQ;
	MPartRef Fabricator, Tanker;

	U16 corvettes, mantis_ships;

	U32 mhandle, shandle, thandle;

    // variables used as parametrs when starting up the DisplayObjectiveHandler

    U32 displayObjectiveHandlerParams_stringID;
    U32 displayObjectiveHandlerParams_dependantHandle;
    U32 displayObjectiveHandlerParams_teletypeHandle;

	// failure string ID
	U32 failureID;
};

//--------------------------------------------------------------------------//
//

#define NONE	0
#define ADMIRAL 1

struct TM1_Briefing_Save
{
	U32 uScreenMode;
	U32 handle1, handle2, handle3, handle4;

	bool Andro, Hawkes, Worm, Austin, Blackwell, Logo;

	enum
	{
		BRIEFING_STATE_RADIO_BROADCAST,
        BRIEFING_STATE_UPLINK,
		BRIEFING_STATE_HALSEY_BRIEF,
		BRIEFING_STATE_HALSEY_BRIEF_2,
		BRIEFING_STATE_HALSEY_BRIEF_3,
		BRIEFING_STATE_HALSEY_BRIEF_4,
        BRIEFING_STATE_BLACKWELL,
        BRIEFING_STATE_OBJECTIVES,
        BRIEFING_STATE_FINISHED

    } state;
};

//--------------------------------------------------------------------------//
//
struct TM1_BriefingSkip_Save
{
};

//--------------------------------------------------------------------------//
//
struct TM1_MissionStart_Save
{
	enum
	{
		Begin,
		Done
	}state;
};
//--------------------------------------------------------------------------//
//
struct TM1_SelectAndMoveBlackwell_Save
{
	SINGLE dist;

    MPartRef lastSelectedShip;
	MPartRef StageForJump;

	enum
	{
		Begin,
		BlackwellSelected,
		MoveBlackwell,
		SelectGroup,
		LookForWormHole,
		Done
	}state;
};
//--------------------------------------------------------------------------//
//
struct TM1_EnterTheMantis_Save
{
	SINGLE timer;
	
	MPartRef Mantis1, Mantis2, Mantis3;

	MPartRef TNSAustinDest;

	MPartRef TerranSpawnPoint;

	bool corvettesIdle;

	enum
	{
        PrePreBegin,
		PreBegin,
		Begin,
		BackToBoys,
		BlackwellOrdersAttack,
		AttackMantis,
		BlackwellContacts,
		Uplink,
		NewObjectives,
		LookAtJumpPoint,
		Fabricator,
		BlackwellBrief,
		Done
	}state;

};


struct TM1_BuildBase_Save
{
};

struct TM1_HQInProgress_Save
{
    bool attackLaunched;
};

struct TM1_DetectMantisDestroyed_Save
{
};

struct TM1_HQFinished_Save
{
};

struct TM1_RefineryInProgress_Save
{
};

struct TM1_RefineryFinished_Save
{
};

struct TM1_MarineTrainingInProgress_Save
{
    MPartRef marineTrainingPart;
};

struct TM1_TankerStarted_Save
{
};

struct TM1_TankerFinished_Save
{
};


struct TM1_LightShipYardInProgress_Save
{
    MPartRef lightShipyardPart;
};

struct TM1_LightShipYardFinished_Save
{
};

struct TM1_MoveFocusToRecovery_Save
{
};

struct TM1_ItemRecovered_Save
{
	enum
	{
		BOGUS,
		Blackbox,
		Beacon,
		Done
	}state;
};

struct TM1_ItemDroppedOff_Save
{
	enum
	{
		BOGUS,
		Blackbox,
		Beacon,
		Done
	}state;
};

struct TM1_ObjectBuilt_Save
{
};

struct TM1_EnemySighted_Save
{
};

struct TM1_UnderAttack_Save
{
};

#define SHIPS_IN_MANTIS_FLEET 6

struct TM1_MantisAttack_Save
{
	SINGLE timer;

	enum
	{
		Begin,
        HalseyWait,
		HalseysFinalMission,
		HalseyBrief,
		BlackwellSupply,
        BlackwellHQExplanation,
		ThroughWormhole,
		Done
	}state;

};

struct TM1_NewPlanetFound_Save
{
	MPartRef UnchartedPlanet;
};

struct TM1_SupplyPlatformBuilding_Save
{

};

struct TM1_ShipDestroyed_Save
{
};


#define SHIPS_IN_SECOND_MANTIS_FLEET 5

struct TM1_BreakOnThrough_Save
{
	SINGLE timer;

	enum
	{
		Begin,
		Scout,
		RoadBlock,
		Done
	}state;
};

struct TM1_FlashBeacon_Save
{
    U32 animID;
};

struct TM1_SightedBeacon_Save
{
};

struct TM1_MantisAI_Save
{
};

struct TM1_TheSecondWave_Save
{
};

struct TM1_ForbiddenJump_Save
{
};

struct TM1_BlackwellKilled_Save
{
};

struct TM1_HQDestroyed_Save
{
};

struct TM1_BlackboxDestroyed_Save
{
};


struct TM1_ClearSystemOfMantis_Save
{
	enum
	{
		STATE_BEGIN,
		STATE_CHECK,
        STATE_DONE

	}state;
};

struct TM1_MissionFailure_Save
{
	enum
	{
		Begin,
		Done
	}state;
	U32 failureID;
};

struct TM1_MissionSuccess_Save
{

	enum
	{
		Begin,
		ClearSystem,
		HalseyBrief,
		PrintSuccess,
		Done
	}state;
};

struct TM1_VisibilityHandlerData
{
    MPartRef mantisWormhole, alkaid2;
};


struct TM1_DisplayObjectiveHandlerData
{
    U32 stringID;
    U32 dependantHandle;
};

#endif