//--------------------------------------------------------------------------//
//                                                                          //
//                                DataDef.h                                 //
//                                                                          //
//               COPYRIGHT (C) 2000 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
//																			//
// Created:		4/24/00		Justin Przedwojewski							//
//																			//
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

#define MANTIS_UNITS_PER_ATTACK_WAVE    4

struct MissionData
{
    enum MissionState
    {
        INVALID_MISSION_STATE = -1,

        MISSION_STATE_BRIEFING,
        MISSION_STATE_INTRO,
        MISSION_STATE_START,
        MISSION_STATE_INITIAL_MANTIS_ATTACK,
        MISSION_STATE_RESCUE_HAWKES,

        // mission over states beyond this point

        MISSION_STATE_BLACKWELL_IS_DEAD,
        MISSION_STATE_PRISON_WAS_DESTROYED,

        MISSION_STATE_PRISON_CAPTURED,

        MISSION_STATE_END_VICTORY,
        MISSION_STATE_END_DEFEAT,

        NUM_MISSION_STATES

    } state;

	TECHNODE missionTech;

    // triggers

	MPartRef blackwellsBogeysTrigger;
    MPartRef attackOfTheMantisTrigger;
	MPartRef mantisWaypoint1;
	MPartRef mantisWaypoint2;
	MPartRef mantisWaypoint3;

    // mission critical units

	MPartRef blackwellsShip;
	MPartRef hawkesPrison;
    MPartRef thripidPrime;
    MPartRef alkaid2Inhibitor;

    // planets

    MPartRef TauCetiPrime;
    MPartRef alkaidPrime;
    MPartRef alkaid2;
    MPartRef alkaid3;
    MPartRef alkaid2Prime;
    MPartRef alkaid22;
    MPartRef alkaid23;
    MPartRef alkaid24;

    // gates

    MPartRef alkaid2Gate;
    MPartRef gateToTheEnd;

    MPartRef mantisBuildBin[MANTIS_UNITS_PER_ATTACK_WAVE];
    U32 mantisBuildBinIndex;
    bool mantisShouldBuild;

    bool outpostAlreadyBuilt;
    bool troopshipAlreadyBuilt;
    bool outpostAlreadyUpgraded;

    bool hawkesPrisonDestroyed;

    U32 mantisKillCount, playerKillCount;

    U32 alkaidDefensiveLasersBuilt;
    U32 alkaidShipyardsBuilt;
    U32 alkaidSupplyDepotBuilt;

	U32 primaryHandle, secondaryHandle;

    // variables used as parametrs when starting up the DisplayObjectiveHandler

    U32 displayObjectiveHandlerParams_stringID;
    U32 displayObjectiveHandlerParams_dependantHandle;
    U32 displayObjectiveHandlerParams_teletypeHandle;

	U32 failureID;
};

struct TM3_StartData
{
	
};

struct TM3_BriefingData
{
	
	enum BriefingState
	{
		INVALID_BRIEFING_STATE = -1,

		BRIEFING_STATE_BEGIN,
		BRIEFING_STATE_TELETYPE,
		BRIEFING_STATE_TV1,
        BRIEFING_STATE_UPLINK,
		BRIEFING_STATE_HALSEY_1,
		BRIEFING_STATE_HALSEY_2,
		BRIEFING_STATE_HALSEY_3,
		BRIEFING_STATE_OBJECTIVES,
		BRIEFING_STATE_FINISHED,

		BRIEFING_STATE_DRAMATIC_PAUSE, // set postPauseState and dramaticPauseLength before using this state


		NUM_BRIEFING_STATES

	} state;

	BriefingState postPauseState;
	S32 dramaticPauseLength;
};

struct TM3_BriefingEndData
{
};

struct TM3_MissionIntroData
{
	S32 durationCounter;

    bool shipyardBlown;

	MPartRef mantisFrigates[6];
	MPartRef mantisCarriers[2];
    MPartRef mantisZorap;

	MPartRef terranCorvettes[2];
	MPartRef terranRepair;
	MPartRef terranRefinery;
};

struct TM3_MissionStartData
{
};

struct TM3_BlackwellsBogeysData
{
};

struct TM3_AttackOfTheMantisData
{
    S32 buildCountdown;
};

struct TM3_MantisWaypoint1TriggerData
{
};

struct TM3_MantisWaypoint2TriggerData
{
};

struct TM3_MantisWaypoint3TriggerData
{
};

struct TM3_ShipDestroyedData
{
};

struct TM3_InitialMantisAttackData
{
    bool soundCueTripped;
};

struct TM3_UnitBuiltData
{
};

struct TM3_CheckForAlkaidSecureData
{
};

struct TM3_AlkaidSecureData
{
};

struct TM3_VisibilityHandlerData
{
    enum VisState
    {
        VIS_STATE_INVALID = -1,

        VIS_STATE_HIDDEN,
        VIS_STATE_VISIBLE,
        VIS_STATE_PROCESSING_1,
        VIS_STATE_PROCESSED,

        NUM_VIS_STATES
    };

    enum VisState alkaidPrison;
    enum VisState alkaid2Inhibitor;
    enum VisState alkaid2Prime;
    enum VisState alkaid23;
    enum VisState alkaid24;
    enum VisState gateToTheEnd;
};

struct TM3_HoldYourFireData
{
};

struct TM3_OutpostUpgradeCompleteData
{
};

struct TM3_BlackwellIsDeadData
{
    S32 countdownTimer;
    bool halseyStartedTalking;
};

struct TM3_PrisonDestroyedData
{
    S32 countdownTimer;
    bool halseyStartedTalking;
};

struct TM3_MissionSuccessData
{
};

struct TM3_MissionFailureData
{
    S32 countdownTimer;
};

struct TM3_CheckForCapturedPrisonData
{
    enum CaptureState
    {
        INVALID_CAPTURE_STATE = -1,

        CAPTURE_STATE_CHECKING,
        CAPTURE_STATE_CAPTURED,

        CAPTURE_STATE_DRAMATIC_PAUSE,

        NUM_CAPTURE_STATES
        
    } state, afterPauseState;

    S32 dramaticPauseCounter;
};

struct TM3_DisplayObjectiveHandlerData
{
    U32 stringID;
    U32 dependantHandle;
};

#endif