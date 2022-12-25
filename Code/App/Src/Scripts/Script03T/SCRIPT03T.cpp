//--------------------------------------------------------------------------//
//                                                                          //
//                                Script03T.cpp                             //
//                                                                          //
//               COPYRIGHT (C) 2000 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
//																			//
// Created:		4/24/00		Justin Przedwojewski							//
//																			//
//--------------------------------------------------------------------------//

#include <stdlib.h>

#include <ScriptDef.h>
#include <DLLScriptMain.h>

#include "DataDef.h"
#include "local.h"

#include "..\helper\helper.h"

//  TM3 #define's

#define PLAYER_ID			1
#define MANTIS_ID			2

#define SYSTEM_ID_TERRA     1
#define SYSTEM_ID_TAU_CETI  2
#define SYSTEM_ID_ALKAID    3
#define SYSTEM_ID_ALKAID_2  5
#define SYSTEM_ID_ALKAID_3  4

#define MANTIS_BUILD_INTERVAL       ( 5 * 60 * 4 )  // five minutes between mantis wave attacks

#define BRIEFING_CAMERA_ZOOM_IN_TIME	55		// in seconds

#define MISSION_INTRO_DURATION			60	// in game updates

static void TM3_TurnOnTheAI
(
    void
);

static void AddAndDisplayObjective
(
    U32 stringID,
    U32 dependantHandle = 0,
    bool isSecondary = false
);

//--------------------------------------------------------------------------

CQSCRIPTDATA( MissionData, missionData );

//--------------------------------------------------------------------------//
//	Mission Briefing
//--------------------------------------------------------------------------//                                         

CQSCRIPTPROGRAM( TM3_Briefing, TM3_BriefingData, CQPROGFLAG_STARTBRIEFING);

void TM3_Briefing::Initialize 
(	
	U32 eventFlags, 
	const MPartRef &part
)
{
	missionData.failureID = 0;
    MScript::PlayMusic( NULL );

    // pause the game so that mantis in Alkaid sector don't launch attack during briefing

    MScript::PauseGame( true );

    missionData.primaryHandle = 0;
    missionData.secondaryHandle = 0;
    missionData.displayObjectiveHandlerParams_teletypeHandle = 0;

    missionData.state = missionData.MISSION_STATE_BRIEFING;

	state = BRIEFING_STATE_BEGIN;
}

bool TM3_Briefing::Update 
(
	void
)
{
    CQBRIEFINGITEM slotAnim;

	if ( missionData.state != missionData.MISSION_STATE_BRIEFING )
	{
		// the user triggers the end of the briefing, and the above enum is set
		// by TM3_BriefingEnd when this occurs.  Returning false here causes TM3_Briefing
		// to be shut down, effectively ending the briefing.

		return false;
	}

	switch(state)
	{
		case BRIEFING_STATE_BEGIN:

			state = BRIEFING_STATE_TELETYPE;

			break;

		case BRIEFING_STATE_TELETYPE:

            MScript::FlushTeletype();

            MScript::PlayBriefingTeletype( IDS_TM3_TELETYPE_LOC, 
                STANDARD_TELETYPE_COLOR, STANDARD_TELETYPE_HOLD_TIME, 
                MScript::GetScriptStringLength( IDS_TM3_TELETYPE_LOC ) * STANDARD_TELETYPE_MILS_PER_CHAR, 
                false );
			
			state = BRIEFING_STATE_TV1;

			// fall through

		case BRIEFING_STATE_TV1:

			missionData.primaryHandle = MScript::PlayAudio( "prolog03.wav" );
			MScript::BriefingSubtitle(missionData.primaryHandle,IDS_TM3_SUB_PROLOG03);

            ShowBriefingAnimation( 0, "TNRLogo", 100, true, true );
            ShowBriefingAnimation( 3, "TNRLogo", 100, true, true );
            ShowBriefingAnimation( 1, "Radiowave", ANIMSPD_RADIOWAVE, true, true );
            ShowBriefingAnimation( 2, "Radiowave", ANIMSPD_RADIOWAVE, true, true );

			state = BRIEFING_STATE_UPLINK;

			break;

		case BRIEFING_STATE_UPLINK:

            if ( !MScript::IsStreamPlaying( missionData.primaryHandle ) )
			{
                MScript::FreeBriefingSlot( -1 );

				ShowBriefingAnimation( 0, "Xmission", ANIMSPD_XMISSION, false, false ); 

                missionData.primaryHandle = MScript::PlayAudio( "high_level_data_uplink.wav" );

				state = BRIEFING_STATE_HALSEY_1;
			}

			break;

		case BRIEFING_STATE_HALSEY_1:

			if ( !MScript::IsStreamPlaying( missionData.primaryHandle ) )
			{
				strcpy( slotAnim.szFileName, "m03ha01.wav" );
				strcpy( slotAnim.szTypeName, "Animate!!Halsey" );

				slotAnim.slotID = 0;
				slotAnim.bHighlite = false;
                missionData.primaryHandle = MScript::PlayBriefingTalkingHead( slotAnim );
				MScript::BriefingSubtitle(missionData.primaryHandle,IDS_TM3_SUB_M03HA01);

				state = BRIEFING_STATE_HALSEY_2;
			}

			break;

		case BRIEFING_STATE_HALSEY_2:

			if ( !MScript::IsStreamPlaying( missionData.primaryHandle ) )
			{
                ShowBriefingAnimation( 1, "MantisPrisoner" );

				strcpy( slotAnim.szFileName, "m03ha02.wav" );
				strcpy( slotAnim.szTypeName, "Animate!!Halsey" );

				slotAnim.slotID = 0;
				slotAnim.bHighlite = false;
                missionData.primaryHandle = MScript::PlayBriefingTalkingHead( slotAnim );
				MScript::BriefingSubtitle(missionData.primaryHandle,IDS_TM3_SUB_M03HA02);

				state = BRIEFING_STATE_HALSEY_3;
			}

			break;

		case BRIEFING_STATE_HALSEY_3:

			if ( !MScript::IsStreamPlaying( missionData.primaryHandle ) )
			{
                ShowBriefingAnimation( 2, "EnergyRibbon" );

				strcpy( slotAnim.szFileName, "m03ha02a.wav" );
				strcpy( slotAnim.szTypeName, "Animate!!Halsey" );

				slotAnim.slotID = 0;
				slotAnim.bHighlite = false;
                missionData.primaryHandle = MScript::PlayBriefingTalkingHead( slotAnim );
				MScript::BriefingSubtitle(missionData.primaryHandle,IDS_TM3_SUB_M03HA02A);

				state = BRIEFING_STATE_OBJECTIVES;
			}

			break;

		case BRIEFING_STATE_OBJECTIVES:

			if ( !MScript::IsStreamPlaying( missionData.primaryHandle ) )
			{
                MScript::FreeBriefingSlot( -1 );

				MScript::PlayBriefingTeletype( IDS_TM3_OBJECTIVES, 
                    STANDARD_TELETYPE_COLOR, 0, 
                    MScript::GetScriptStringLength( IDS_TM3_OBJECTIVES ) * STANDARD_TELETYPE_MILS_PER_CHAR, 
                    false );

				state = BRIEFING_STATE_FINISHED;
			}

			break;

		case BRIEFING_STATE_FINISHED:

			// just hang out until the player decides to start the mission

			break;

		case BRIEFING_STATE_DRAMATIC_PAUSE:

			dramaticPauseLength--;

			if ( dramaticPauseLength <= 0 )
			{
				state = postPauseState;
			}

			break;
	}

	return true;
}



//--------------------------------------------------------------------------//
//	Mission Intro involving Mantis attack on base

CQSCRIPTPROGRAM( TM3_MissionIntro, TM3_MissionIntroData, CQPROGFLAG_STARTMISSION );

void TM3_MissionIntro::Initialize
( 
	U32 eventFlags, 
	const MPartRef &part 
)
{
	U32 i;
    MPartRef tempRef;

    MScript::SetMissionID( 2 );
	MScript::SetMissionName( IDS_TM3_NAME );
	MScript::SetMissionDescription( IDS_TM3_DESCRIPTION );

    MScript::SetEnemyCharacter( MANTIS_ID, IDS_TM3_MANTIS_PLAYER_NAME );

    MScript::PlayMusic( "danger.wav" );

    MScript::EnableRegenMode( true );

	durationCounter = MISSION_INTRO_DURATION;
    shipyardBlown = false;

	missionData.primaryHandle = missionData.secondaryHandle = 0;

    // set up initial tech tree for the mission

	missionData.missionTech.InitLevel( TECHTREE::FULL_TREE );

	missionData.missionTech.race[0].build = missionData.missionTech.race[1].build = (TECHTREE::BUILDNODE) (
        TECHTREE::TDEPEND_HEADQUARTERS | 
        TECHTREE::TDEPEND_REFINERY | 
        TECHTREE::TDEPEND_LIGHT_IND |
        TECHTREE::TDEPEND_SENSORTOWER | 
        TECHTREE::TDEPEND_LASER_TURRET |
        TECHTREE::TDEPEND_BALLISTICS |
        TECHTREE::TDEPEND_TENDER |
        TECHTREE::RES_REFINERY_GAS1 | 
        TECHTREE::TDEPEND_OUTPOST |
        TECHTREE::RES_REFINERY_METAL1);
	missionData.missionTech.race[1].build = 	(TECHTREE::BUILDNODE) (
		missionData.missionTech.race[1].build | 
		TECHTREE::MDEPEND_COLLECTOR |
		TECHTREE::MDEPEND_PLANTATION|
		TECHTREE::MDEPEND_BIOFORGE |
		TECHTREE::MDEPEND_WARLORDTRAIN);

	missionData.missionTech.race[2].build = 
        (TECHTREE::BUILDNODE) ( ( TECHTREE::ALL_BUILDNODE ^ 
        TECHTREE::SDEPEND_PORTAL ) | 
        TECH_TREE_RACE_BITS_ALL );
        
	missionData.missionTech.race[0].tech = missionData.missionTech.race[1].tech = (TECHTREE::TECHUPGRADE) (
        TECHTREE::T_SHIP__FABRICATOR | 
        TECHTREE::T_SHIP__HARVEST |
        TECHTREE::T_SHIP__CORVETTE | 
        TECHTREE::T_SHIP__MISSILECRUISER |
        TECHTREE::T_SHIP__SUPPLY |
        TECHTREE::T_SHIP__INFILTRATOR |
        TECHTREE::T_SHIP__TROOPSHIP |
        TECHTREE::T_RES_TROOPSHIP1 );
	missionData.missionTech.race[1].tech = 	(TECHTREE::TECHUPGRADE) (
		missionData.missionTech.race[1].tech | 
		TECHTREE::M_SHIP_KHAMIR | 
		TECHTREE::M_SHIP_FRIGATE |
		TECHTREE::M_SHIP_SEEKER);

	missionData.missionTech.race[0].common_extra = missionData.missionTech.race[1].common_extra = (TECHTREE::COMMON_EXTRA) (
        TECHTREE::RES_TANKER1 | 
        TECHTREE::RES_TANKER2 | 
        TECHTREE::RES_SENSORS1);

	missionData.missionTech.race[0].common = missionData.missionTech.race[1].common = (TECHTREE::COMMON) (
        TECHTREE::RES_WEAPONS1 |
        TECHTREE::RES_WEAPONS2 |
        TECHTREE::RES_SUPPLY1 |
        TECHTREE::RES_SUPPLY2 );

    MScript::SetAvailiableTech( missionData.missionTech );

    // set up resources for each side

    MScript::SetMetal( PLAYER_ID, 100 );
	MScript::SetGas( PLAYER_ID, 20 );
	MScript::SetCrew( PLAYER_ID, 400 );

    // set the mantis up with a ton of resources since they don't harvest
    // them in this mission

    MScript::SetMetal( MANTIS_ID, 100 );
	MScript::SetGas( MANTIS_ID, 100 );
	MScript::SetCrew( MANTIS_ID, 500 );

    MScript::SetMaxCommandPoitns( MANTIS_ID, 150 );

    // get handles to various key elements within the mission so they don't have to be searched for repeatedly

    missionData.blackwellsBogeysTrigger = MScript::GetPartByName( "BlackwellsBogeys" );
    missionData.attackOfTheMantisTrigger = MScript::GetPartByName( "MantisAttack" );
    missionData.mantisWaypoint1 = MScript::GetPartByName( "Mantis Waypoint 1" );
    missionData.mantisWaypoint2 = MScript::GetPartByName( "Mantis Waypoint 2" );
    missionData.mantisWaypoint3 = MScript::GetPartByName( "Mantis Waypoint 3" );

    missionData.blackwellsShip = MScript::GetPartByName( "Blackwell" );
    missionData.hawkesPrison = MScript::GetPartByName( "Warlord Training - Hawkes" );
    missionData.thripidPrime = MScript::GetPartByName( "Thripid Prime" );

    missionData.TauCetiPrime = MScript::GetPartByName( "Tau Ceti Prime" );
    missionData.alkaidPrime = MScript::GetPartByName( "Alkaid Prime" );
    missionData.alkaid2 = MScript::GetPartByName( "Alkaid 2" );
    missionData.alkaid3 = MScript::GetPartByName( "Alkaid 3" );
    missionData.alkaid2Prime = MScript::GetPartByName( "Baai Prime" );
    missionData.alkaid22 = MScript::GetPartByName( "Baai 2" );
    missionData.alkaid23 = MScript::GetPartByName( "Baai 3" );
    missionData.alkaid24 = MScript::GetPartByName( "Baai 4" );
    missionData.gateToTheEnd = MScript::GetPartByName( "Gate8" );

    missionData.alkaid2Gate = MScript::GetPartByName( "Gate4" );
    missionData.alkaid2Inhibitor = MScript::GetPartByName( "Mantis Jump Gate" );


    missionData.mantisKillCount = missionData.playerKillCount = 0;

    missionData.mantisBuildBinIndex = 0;
    missionData.mantisShouldBuild = false;

    missionData.alkaidDefensiveLasersBuilt = 0;
    missionData.alkaidShipyardsBuilt = 0;
    missionData.alkaidSupplyDepotBuilt = 0;

    missionData.outpostAlreadyBuilt = false;
    missionData.troopshipAlreadyBuilt = false;
    missionData.outpostAlreadyUpgraded = false;

    missionData.hawkesPrisonDestroyed = false;

    MScript::SetHullPoints( missionData.hawkesPrison, 7200 );
    MScript::SetMaxHullPoints( missionData.hawkesPrison, 7200 );

    MScript::MakeNonAutoTarget( missionData.hawkesPrison, true );

    MScript::SetHullPoints( MScript::GetPartByName( "ShipyardOmega" ), 10 );

    // disable jump-gate to terra

    MScript::EnableJumpgate( MScript::GetPartByName( "Gate1" ), false );

    // disable the jump gate from alkaid to alkaid 3 until the player secures alkaid

    MScript::EnableJumpgate( MScript::GetPartByName( "Gate4" ), false );

    // Don't let the player look at Terra

    MScript::EnableSystem( SYSTEM_ID_TERRA, false );
    MScript::EnableSystem( SYSTEM_ID_ALKAID_2, false );
    MScript::EnableSystem( SYSTEM_ID_ALKAID_3, false );

	// clear the fog of war in the starting sector by clearing fog around a dummy waypoint
	// set up in the middle of it.

	tempRef = MScript::GetPartByName( "DummyFogWay1" );

	MScript::ClearHardFog( tempRef, 200);

    // clear hard fog around objects players discovered in previous mission

    MScript::ClearHardFog( missionData.alkaidPrime, 5 );
    MScript::ClearHardFog( missionData.alkaid2, 5 );
    MScript::ClearHardFog( missionData.alkaid2Gate, 5 );
    MScript::ClearHardFog( MScript::GetPartByName( "Gate3" ), 5 );

    // start up the mission intro

	UnderMissionControl();

	MScript::ChangeCamera( MScript::GetPartByName( "MissionIntroCam" ), 0, MOVIE_CAMERA_ZERO );

	MScript::EnableMovieMode( true );

    MScript::PauseGame( false );

	mantisFrigates[0] = MScript::GetPartByName( "Mantis1" );
	mantisFrigates[1] = MScript::GetPartByName( "Mantis2" );
	mantisFrigates[2] = MScript::GetPartByName( "Mantis3" );
	mantisFrigates[3] = MScript::GetPartByName( "Mantis4" );
	mantisFrigates[4] = MScript::GetPartByName( "Mantis5" );
	mantisFrigates[5] = MScript::GetPartByName( "Mantis6" );

	mantisCarriers[0] = MScript::GetPartByName( "Mantis7" );
	mantisCarriers[1] = MScript::GetPartByName( "Mantis8" );

    mantisZorap = MScript::GetPartByName( "Zorap1" );

	terranCorvettes[0] = MScript::GetPartByName( "TNS Randal" );
	terranCorvettes[1] = MScript::GetPartByName( "TNS Jacobius" );

	terranRepair = MScript::GetPartByName( "Alkaid Repair" );
	terranRefinery = MScript::GetPartByName( "Alkaid Refinery" );

	MScript::SetVisibleToPlayer( terranCorvettes[0], MANTIS_ID );
	MScript::SetVisibleToPlayer( terranCorvettes[1], MANTIS_ID );

	// set half the fighters to attack one of the terran corvettes

	for ( i = 0; i < 3; i++ )
	{
		MScript::OrderAttack( mantisFrigates[i], terranCorvettes[0] );
	}

	// and the other half to attack the other

	for ( ; i < 6; i++ )
	{
		MScript::OrderAttack( mantisFrigates[i], terranCorvettes[1] );  
	}

	MScript::OrderAttack( mantisCarriers[0], terranCorvettes[0] );  
	MScript::OrderAttack( mantisCarriers[1], terranCorvettes[1] );  

    // order the zorap to escort one of the fighters so they stay in supply

    MScript::OrderEscort( mantisZorap, mantisFrigates[2] );

    // set all carriers on the map to defense stance

	MassStance( M_SCOUTCARRIER, MANTIS_ID, US_DEFEND );
	MassStance( M_HIVECARRIER, MANTIS_ID, US_DEFEND );

    // set all frigates on the map to attack stance

	MassStance( M_FRIGATE, MANTIS_ID, US_ATTACK );

    // make sure the refinery around alkaid is visible to the mantis so that they
    // press the attack after blowing up the corvettes

	MScript::SetVisibleToPlayer( terranRepair, MANTIS_ID );
	MScript::SetVisibleToPlayer( terranRefinery, MANTIS_ID );

    // set up starting mission objectives

	MScript::AddToObjectiveList( IDS_TM3_OBJECTIVE1 );
	MScript::AddToObjectiveList( IDS_TM3_OBJECTIVE2 );
	MScript::AddToObjectiveList( IDS_TM3_OBJECTIVE3 );

    missionData.state = missionData.MISSION_STATE_INTRO;
}

bool TM3_MissionIntro::Update 
(
	void
)
{	
	durationCounter--;

    if ( missionData.state != missionData.MISSION_STATE_INTRO )
    {
		MScript::EnableMovieMode( false );

        // if the mission state no longer suits this program, halt execution of it

        return false;
    }

	if ( durationCounter <= 0 )
	{
		MScript::EnableMovieMode( false );

        missionData.state = missionData.MISSION_STATE_START;

		MScript::RunProgramByName( "TM3_MissionStart", MPartRef ());

		return false;
	}
	else
	{
        if ( durationCounter <= MISSION_INTRO_DURATION / 2 && !shipyardBlown )
        {
            MScript::SetHullPoints( MScript::GetPartByName( "ShipyardOmega" ), 0 );

            shipyardBlown = true;
        }


		return true;
	}
}

//--------------------------------------------------------------------------//
//	Start of the mission... After the briefing and intro.

CQSCRIPTPROGRAM( TM3_MissionStart, TM3_MissionStartData, 0 );

void TM3_MissionStart::Initialize
( 
	U32 eventFlags, 
	const MPartRef &part 
)
{
	UnderPlayerControl();

	MScript::ChangeCamera( MScript::GetPartByName( "MissionStartCam" ), 0, MOVIE_CAMERA_ZERO );

    // set up trigger for blackwell's 'bogeys at 12 o'clock' message

	MScript::SetTriggerProgram( missionData.blackwellsBogeysTrigger, "TM3_BlackwellsBogeys" );
	MScript::SetTriggerRange( missionData.blackwellsBogeysTrigger, 8 );

    MScript::SetTriggerFilter( missionData.blackwellsBogeysTrigger, PLAYER_ID, 
        TRIGGER_PLAYER, true);

    MScript::SetTriggerFilter( missionData.blackwellsBogeysTrigger, M_CORVETTE, 
        TRIGGER_MOBJCLASS, true);

    MScript::EnableTrigger( missionData.blackwellsBogeysTrigger, true );

    // set up triggers for Mantis waypoints

	MScript::SetTriggerProgram( missionData.mantisWaypoint1, "TM3_MantisWaypoint1Trigger" );
	MScript::SetTriggerProgram( missionData.mantisWaypoint2, "TM3_MantisWaypoint2Trigger" );
	MScript::SetTriggerProgram( missionData.mantisWaypoint3, "TM3_MantisWaypoint3Trigger" );

	MScript::SetTriggerRange( missionData.mantisWaypoint1, 2 );
	MScript::SetTriggerRange( missionData.mantisWaypoint2, 2 );
	MScript::SetTriggerRange( missionData.mantisWaypoint3, 2 );

    MScript::SetTriggerFilter( missionData.mantisWaypoint1, MANTIS_ID, 
        TRIGGER_PLAYER, true);

    MScript::SetTriggerFilter( missionData.mantisWaypoint2, MANTIS_ID, 
        TRIGGER_PLAYER, true);

    MScript::SetTriggerFilter( missionData.mantisWaypoint3, MANTIS_ID, 
        TRIGGER_PLAYER, true);

    MScript::SetTriggerFilter( missionData.mantisWaypoint1, OC_SPACESHIP, 
        TRIGGER_OBJCLASS, true);

    MScript::SetTriggerFilter( missionData.mantisWaypoint2, OC_SPACESHIP, 
        TRIGGER_OBJCLASS, true);

    MScript::SetTriggerFilter( missionData.mantisWaypoint3, OC_SPACESHIP, 
        TRIGGER_OBJCLASS, true);

    MScript::EnableTrigger( missionData.mantisWaypoint1, true );
    MScript::EnableTrigger( missionData.mantisWaypoint2, true );
    MScript::EnableTrigger( missionData.mantisWaypoint3, true );

    // run programs in the background to check for specific events

    MScript::RunProgramByName( "TM3_VisibilityHandler", MPartRef ());
    MScript::RunProgramByName( "TM3_OutpostUpgradeComplete", MPartRef () );
    MScript::RunProgramByName( "TM3_HoldYourFire", MPartRef ());
}

bool TM3_MissionStart::Update 
(
	void
)
{	
    if ( missionData.state != missionData.MISSION_STATE_START )
    {
        // if the mission state no longer suits this program, halt execution of it

        return false;
    }

	if ( !MScript::IsStreamPlaying( missionData.primaryHandle ) )
	{
        missionData.primaryHandle = MScript::PlayAnimatedMessage( "M03BL01.wav", "Animate!!Blackwell2", 
            MagpieLeft, MagpieTop, missionData.blackwellsShip, IDS_TM3_SUB_M03BL01 );

        AddAndDisplayObjective( IDS_TM3_OBJECTIVE4, missionData.primaryHandle );

        missionData.state = missionData.MISSION_STATE_INITIAL_MANTIS_ATTACK;

		MScript::RunProgramByName( "TM3_InitialMantisAttack", MPartRef ());

    	return false;
	}
    else
    {
	    return true;
    }
}

//--------------------------------------------------------------------------//
//	Initial Mantis Attack on Alkaid 1.  Mainly tracks how many mantis ships
// have been shot down to determine if the attack is over or not.

CQSCRIPTPROGRAM( TM3_InitialMantisAttack, TM3_InitialMantisAttackData, 0 );

void TM3_InitialMantisAttack::Initialize
( 
	U32 eventFlags, 
	const MPartRef &part 
)
{
    soundCueTripped = false;
}

bool TM3_InitialMantisAttack::Update 
(
	void
)
{
    if ( missionData.state != missionData.MISSION_STATE_INITIAL_MANTIS_ATTACK )
    {
        // if the mission state no longer suits this program, halt execution of it

        return false;
    }

    if ( missionData.mantisKillCount >= 8 )
    {
        // player has destroyed the initial wave

	    if ( !MScript::IsStreamPlaying( missionData.primaryHandle ) )
	    {
            if ( soundCueTripped )
            {
                // blackwell has finished giving his speech, move on in the mission

                return false;
            }
            else
            {
        		MScript::MarkObjectiveCompleted( IDS_TM3_OBJECTIVE4 );

                missionData.primaryHandle = MScript::PlayAnimatedMessage( "M03BL03.wav", "Animate!!Blackwell2", 
                    MagpieLeft, MagpieTop, missionData.blackwellsShip,IDS_TM3_SUB_M03BL03 );

                AddAndDisplayObjective( IDS_TM3_OBJECTIVE5, missionData.primaryHandle );
                AddAndDisplayObjective( IDS_TM3_OBJECTIVE6, missionData.primaryHandle, true );
                AddAndDisplayObjective( IDS_TM3_OBJECTIVE7, missionData.primaryHandle, true );
                AddAndDisplayObjective( IDS_TM3_OBJECTIVE8, missionData.primaryHandle, true );

        		MScript::RunProgramByName( "TM3_CheckForAlkaidSecure", MPartRef() );

                // start the mantis wave attacks coming in

                MScript::RunProgramByName( "TM3_AttackOfTheMantis", MPartRef() );

                soundCueTripped = true;
            }
        }
    }

    return true;
}

//--------------------------------------------------------------------------//
//	Blackwell's Bogey message when triggered.

CQSCRIPTPROGRAM( TM3_BlackwellsBogeys, TM3_BlackwellsBogeysData, 0 );

void TM3_BlackwellsBogeys::Initialize
( 
	U32 eventFlags, 
	const MPartRef &part 
)
{
    // prevent this trigger from being activated again

    MScript::EnableTrigger( missionData.blackwellsBogeysTrigger, false );
}

bool TM3_BlackwellsBogeys::Update 
(
	void
)
{	
    if ( missionData.state != missionData.MISSION_STATE_INITIAL_MANTIS_ATTACK )
    {
        // if the mission state no longer suits this program, halt execution of it

        return false;
    }

	if ( !MScript::IsStreamPlaying( missionData.primaryHandle ) )
	{
        missionData.primaryHandle = MScript::PlayAnimatedMessage( "M03BL02.wav", "Animate!!Blackwell2", 
            MagpieLeft, MagpieTop, missionData.blackwellsShip ,IDS_TM3_SUB_M03BL02);

    	return false;
	}
    else
    {
        return true;
    }
}

//--------------------------------------------------------------------------//
//	Launches and maintains the Mantis wave attacks when triggered.

CQSCRIPTPROGRAM( TM3_AttackOfTheMantis, TM3_AttackOfTheMantisData, 0 );

void TM3_AttackOfTheMantis::Initialize
( 
	U32 eventFlags, 
	const MPartRef &part 
)
{
    int i;

    // prevent this trigger from being activated again

    MScript::EnableTrigger( missionData.attackOfTheMantisTrigger, false );

    missionData.mantisBuildBinIndex = 0;
    missionData.mantisShouldBuild = true;

    for ( i = 0; i < MANTIS_UNITS_PER_ATTACK_WAVE; i++ )
    {
        MScript::OrderBuildUnit( missionData.thripidPrime, "GBOAT!!M_Frigate", true );
    }

    // make sure the Mantis know how to get from system to system

    MScript::ClearPath( SYSTEM_ID_ALKAID_2, SYSTEM_ID_ALKAID, MANTIS_ID );
    MScript::ClearPath( SYSTEM_ID_ALKAID_2, SYSTEM_ID_ALKAID_3, MANTIS_ID );
    MScript::ClearPath( SYSTEM_ID_ALKAID_3, SYSTEM_ID_ALKAID, MANTIS_ID );

    buildCountdown = MANTIS_BUILD_INTERVAL;
}

bool TM3_AttackOfTheMantis::Update 
(
	void
)
{	
    int i;

    if ( !missionData.mantisShouldBuild )
    {
        return false;
    }

    if ( missionData.state >= missionData.MISSION_STATE_BLACKWELL_IS_DEAD )
    {
        // if the mission state no longer suits this program, halt execution of it

        return false;
    }

    buildCountdown--;

    if ( buildCountdown <= 0 )
    {
        buildCountdown = MANTIS_BUILD_INTERVAL;

        for ( i = 0; i < MANTIS_UNITS_PER_ATTACK_WAVE; i++ )
        {
            MScript::OrderBuildUnit( missionData.thripidPrime, "GBOAT!!M_Frigate", true );
        }
    }

    if ( missionData.mantisBuildBinIndex >= MANTIS_UNITS_PER_ATTACK_WAVE )
    {
        missionData.mantisBuildBinIndex = 0;

        for ( i = 0; i < MANTIS_UNITS_PER_ATTACK_WAVE; i++ )
        {
            MScript::OrderMoveTo( missionData.mantisBuildBin[i], missionData.mantisWaypoint1 );
        }
    }

    return true;
}

//--------------------------------------------------------------------------//
// Orders Mantis fighters that reach this waypoint to proceed to the next.
// This avoids problems with the AI getting confused by the two entry points
// into Alkaid.

CQSCRIPTPROGRAM( TM3_MantisWaypoint1Trigger, TM3_MantisWaypoint1TriggerData, 0 );

void TM3_MantisWaypoint1Trigger::Initialize
( 
	U32 eventFlags, 
	const MPartRef &part 
)
{
    MPartRef tempRef, tempEnemyRef;

    tempRef = MScript::GetLastTriggerObject( part );

    if ( tempRef.isValid() )
    {
        if ( MScript::IsIdle( tempRef ) )
        {
            tempEnemyRef = MScript::FindNearestEnemy( tempRef, false, true );

            if ( tempEnemyRef.isValid() )
            {
                // make sure the area is cleared of enemies before proceeding

                if ( tempRef->caps.attackOk && MScript::DistanceTo( tempRef, tempEnemyRef ) <= 3 )
                {
                    MScript::OrderAttack( tempRef, tempEnemyRef );
                }
                else
                {
                    MScript::OrderMoveTo( tempRef, missionData.mantisWaypoint2 );
                }
            }
            else
            {
                MScript::OrderMoveTo( tempRef, missionData.mantisWaypoint2 );
            }
        }
    }
}

bool TM3_MantisWaypoint1Trigger::Update 
(
	void
)
{	
    return false;
}

//--------------------------------------------------------------------------//
// Orders Mantis fighters that reach this waypoint to proceed to the next.
// This avoids problems with the AI getting confused by the two entry points
// into Alkaid.

CQSCRIPTPROGRAM( TM3_MantisWaypoint2Trigger, TM3_MantisWaypoint2TriggerData, 0 );

void TM3_MantisWaypoint2Trigger::Initialize
( 
	U32 eventFlags, 
	const MPartRef &part 
)
{
    MPartRef tempRef, tempEnemyRef;

    tempRef = MScript::GetLastTriggerObject( part );

    if ( tempRef.isValid() )
    {
        if ( MScript::IsIdle( tempRef ) )
        {
            tempEnemyRef = MScript::FindNearestEnemy( tempRef, false, true );

            if ( tempEnemyRef.isValid() )
            {
                // make sure the area is cleared of enemies before proceeding

                if ( tempRef->caps.attackOk && MScript::DistanceTo( tempRef, tempEnemyRef ) <= 3 )
                {
                    MScript::OrderAttack( tempRef, tempEnemyRef );
                }
                else
                {
                    MScript::OrderMoveTo( tempRef, missionData.mantisWaypoint3 );
                }
            }
            else
            {
                MScript::OrderMoveTo( tempRef, missionData.mantisWaypoint3 );
            }
        }
    }
}

bool TM3_MantisWaypoint2Trigger::Update 
(
	void
)
{	
    return false;
}

//--------------------------------------------------------------------------//
// Orders Mantis fighters that reach this waypoint to attack the nearest
// enemy to prevent them from just hanging out in Terran space.

CQSCRIPTPROGRAM( TM3_MantisWaypoint3Trigger, TM3_MantisWaypoint3TriggerData, 0 );

void TM3_MantisWaypoint3Trigger::Initialize
( 
	U32 eventFlags, 
	const MPartRef &part 
)
{
    MPartRef tempRef;
    MPartRef tempEnemy;

    tempRef = MScript::GetLastTriggerObject( part );

    if ( tempRef.isValid() )
    {
        if ( tempRef->caps.attackOk && MScript::IsIdle( tempRef ) )
        {
            tempEnemy = MScript::FindNearestEnemy( tempRef, false, false );

            MScript::OrderAttack( tempRef, tempEnemy );
        }
    }
}

bool TM3_MantisWaypoint3Trigger::Update 
(
	void
)
{	
    return false;
}

//--------------------------------------------------------------------------//
// Generic program to handle ships being destroyed within the mission

CQSCRIPTPROGRAM( TM3_ShipDestroyed, TM3_ShipDestroyedData, CQPROGFLAG_OBJECTDESTROYED );

void TM3_ShipDestroyed::Initialize
( 
	U32 eventFlags, 
	const MPartRef &part 
)
{
	MPartRef tempRef;

    if ( missionData.state >= missionData.MISSION_STATE_BLACKWELL_IS_DEAD )
    {
        return;
    }

    if ( part->playerID == MANTIS_ID )
    {
        missionData.mantisKillCount++;

        if ( part == missionData.alkaid2Inhibitor )
        {
            MScript::EnableSystem( SYSTEM_ID_ALKAID_2, true );

            MScript::RemoveFromObjectiveList( IDS_TM3_OBJECTIVE9 );
        }

        if ( missionData.state == missionData.MISSION_STATE_INITIAL_MANTIS_ATTACK )
        {
            // turn off blackwells initial 'Bogeys at twelve o'clock' message once one of them 
            // has been destroyed, otherwise he might sound like a dufus telling his guys
            // to attack when they've already been doing so.

            MScript::EnableTrigger( missionData.blackwellsBogeysTrigger, false );
        }

        // if this is Hawkes Prison, then it be all over da house

        if ( missionData.hawkesPrison.isValid() )
        {
            if ( part == missionData.hawkesPrison )
            {
                missionData.state = missionData.MISSION_STATE_PRISON_WAS_DESTROYED;

                missionData.hawkesPrisonDestroyed = true;

                MScript::MoveCamera( missionData.hawkesPrison, 0, MOVIE_CAMERA_JUMP_TO );

		        MScript::RunProgramByName( "TM3_PrisonDestroyed", MPartRef ());
            }
        }

        if ( missionData.thripidPrime.isValid() )
        {
            if ( part == missionData.thripidPrime )
            {
                // if the thripid thingy in Alkaid 2 Prime has been blown up, 
                // stop trying to produce ships there

                missionData.mantisShouldBuild = false;
            }
        }
    }
    else if ( part->playerID == PLAYER_ID )
    {
        missionData.playerKillCount++;

        // if this is Blackwell's ship, it's game over man

        if ( part == missionData.blackwellsShip )
        {
            missionData.state = missionData.MISSION_STATE_BLACKWELL_IS_DEAD;

            MScript::MoveCamera( missionData.blackwellsShip, 0, MOVIE_CAMERA_JUMP_TO );

		    MScript::RunProgramByName( "TM3_BlackwellIsDead", MPartRef ());
        }
    }
}

bool TM3_ShipDestroyed::Update 
(
	void
)
{
    return FALSE;
}

//--------------------------------------------------------------------------//
// Generic program to handle units being built within the mission

CQSCRIPTPROGRAM( TM3_UnitBuilt, TM3_UnitBuiltData, CQPROGFLAG_OBJECTCONSTRUCTED );

void TM3_UnitBuilt::Initialize
( 
	U32 eventFlags, 
	const MPartRef &part 
)
{
    if ( missionData.state >= missionData.MISSION_STATE_BLACKWELL_IS_DEAD )
    {
        return;
    }

    if ( part->playerID == MANTIS_ID )
    {
        if ( missionData.mantisBuildBinIndex < MANTIS_UNITS_PER_ATTACK_WAVE )
        {
            // make sure the little buggers do us proud

            MScript::SetStance( part, US_ATTACK );

            missionData.mantisBuildBin[missionData.mantisBuildBinIndex] = part;

            missionData.mantisBuildBinIndex++;
        }
    }

    if ( part->playerID == PLAYER_ID )
    {
        switch ( part->mObjClass )
        {
            case M_LSAT:

                if ( MScript::DistanceTo( part, missionData.alkaid2Gate ) < 5 )
                {
                    missionData.alkaidDefensiveLasersBuilt++;
                }

                break;

            case M_LIGHTIND:

                if ( part->systemID == SYSTEM_ID_ALKAID )
                {
                    missionData.alkaidShipyardsBuilt++;
                }

                break;

            case M_TENDER:

                if ( part->systemID == SYSTEM_ID_ALKAID )
                {
                    missionData.alkaidSupplyDepotBuilt++;
                }

                break;

            case M_OUTPOST:

                if ( !missionData.outpostAlreadyBuilt )
                {
	                if ( !MScript::IsStreamPlaying( missionData.primaryHandle ) )
	                {
                        if ( missionData.state == missionData.MISSION_STATE_RESCUE_HAWKES )
                        {
                            missionData.primaryHandle = MScript::PlayAnimatedMessage( "M03BL16.wav", "Animate!!Blackwell2", 
                                MagpieLeft, MagpieTop, missionData.blackwellsShip, IDS_TM3_SUB_M03BL16 );

                    		MScript::MarkObjectiveCompleted( IDS_TM3_OBJECTIVE11 );

                            if ( missionData.outpostAlreadyUpgraded && missionData.troopshipAlreadyBuilt )
                            {               
                                MScript::RemoveFromObjectiveList( IDS_TM3_OBJECTIVE11 );
                                MScript::RemoveFromObjectiveList( IDS_TM3_OBJECTIVE12 );
                                MScript::RemoveFromObjectiveList( IDS_TM3_OBJECTIVE13 );
                            }
                        }

                        missionData.outpostAlreadyBuilt = true;

                    }
                }

                break;

            case M_TROOPSHIP:

                if ( !missionData.troopshipAlreadyBuilt )
                {
	                if ( !MScript::IsStreamPlaying( missionData.primaryHandle ) )
	                {
                        if ( missionData.state == missionData.MISSION_STATE_RESCUE_HAWKES )
                        {
        	                MScript::MarkObjectiveCompleted( IDS_TM3_OBJECTIVE12 );

                            if ( missionData.outpostAlreadyBuilt && missionData.outpostAlreadyUpgraded )
                            {               
                                MScript::RemoveFromObjectiveList( IDS_TM3_OBJECTIVE11 );
                                MScript::RemoveFromObjectiveList( IDS_TM3_OBJECTIVE12 );
                                MScript::RemoveFromObjectiveList( IDS_TM3_OBJECTIVE13 );
                            }
                        }

                        missionData.troopshipAlreadyBuilt = true;                        
                    }
                }

                break;
        }

    }
}

bool TM3_UnitBuilt::Update 
(
	void
)
{
    return false;
}

//--------------------------------------------------------------------------//

CQSCRIPTPROGRAM( TM3_CheckForAlkaidSecure, TM3_CheckForAlkaidSecureData, 0 );

void TM3_CheckForAlkaidSecure::Initialize
( 
	U32 eventFlags, 
	const MPartRef &part 
)
{
}

bool TM3_CheckForAlkaidSecure::Update 
(
	void
)
{
    if ( missionData.state >= missionData.MISSION_STATE_BLACKWELL_IS_DEAD )
    {
        // if the mission state no longer suits this program, halt execution of it

        return false;
    }

    if ( missionData.alkaidDefensiveLasersBuilt >= 2 && !MScript::IsObjectiveCompleted( IDS_TM3_OBJECTIVE6 ) )
    {
        MScript::MarkObjectiveCompleted( IDS_TM3_OBJECTIVE6 );
    }

    if ( missionData.alkaidShipyardsBuilt >= 1 && !MScript::IsObjectiveCompleted( IDS_TM3_OBJECTIVE7 ) )
    {
        MScript::MarkObjectiveCompleted( IDS_TM3_OBJECTIVE7 );
    }

    if ( missionData.alkaidSupplyDepotBuilt >= 1 && !MScript::IsObjectiveCompleted( IDS_TM3_OBJECTIVE8 ) )
    {
        MScript::MarkObjectiveCompleted( IDS_TM3_OBJECTIVE8 );
    }

    if ( missionData.alkaidDefensiveLasersBuilt >= 2 &&
        missionData.alkaidShipyardsBuilt >= 1 &&
        missionData.alkaidSupplyDepotBuilt >= 1 )
    {
        // completed overall objective of securing alkaid

	    MScript::MarkObjectiveCompleted( IDS_TM3_OBJECTIVE5 );

        // remove secondary objectives that lead to the above.

        MScript::RemoveFromObjectiveList( IDS_TM3_OBJECTIVE6 );
        MScript::RemoveFromObjectiveList( IDS_TM3_OBJECTIVE7 );
        MScript::RemoveFromObjectiveList( IDS_TM3_OBJECTIVE8 );

        MScript::RunProgramByName( "TM3_AlkaidSecure", MPartRef () );

        return false;
    }

    return true;
}

//---------------------------------------------------------------------------
// Program executed when player has built enough stuff in Alkaid for it to be
// considered "secure"

CQSCRIPTPROGRAM( TM3_AlkaidSecure, TM3_AlkaidSecureData, 0 );

void TM3_AlkaidSecure::Initialize 
(
	U32 eventFlags, 
	const MPartRef &part
)
{
    // Open up the jumpgate to Alkaid 3

    MScript::EnableJumpgate( MScript::GetPartByName( "Gate4" ), true );

    // allow jump inhibitors to be built (not allowed before now to prevent gate from being
    // built on blocked wormhole, which can lead to problems)

	missionData.missionTech.race[0].build = TECHTREE::BUILDNODE( missionData.missionTech.race[0].build | 
        TECHTREE::TDEPEND_JUMP_INHIBITOR );

    MScript::SetAvailiableTech( missionData.missionTech );

    MScript::EnableSystem( SYSTEM_ID_ALKAID_3, true );
}

bool TM3_AlkaidSecure::Update 
(
	void
)
{
    if ( missionData.state >= missionData.MISSION_STATE_BLACKWELL_IS_DEAD )
    {
        // if the mission state no longer suits this program, halt execution of it

        return false;
    }

	if ( !MScript::IsStreamPlaying( missionData.primaryHandle ) )
	{
        missionData.primaryHandle = MScript::PlayAnimatedMessage( "M03BL04.wav", "Animate!!Blackwell2", 
            MagpieLeft, MagpieTop, missionData.blackwellsShip, IDS_TM3_SUB_M03BL04 );

    	return false;
	}

    return true;
}

//---------------------------------------------------------------------------
// Program that runs in the background and checks whether certain
// key mission elements are visible to the player, so that appropriate
// responses may be generated when they are.

CQSCRIPTPROGRAM( TM3_VisibilityHandler, TM3_VisibilityHandlerData, 0 );

void TM3_VisibilityHandler::Initialize 
(
	U32 eventFlags, 
	const MPartRef &part
)
{
    alkaidPrison = VIS_STATE_HIDDEN;
    alkaid2Inhibitor = VIS_STATE_HIDDEN;
}

bool TM3_VisibilityHandler::Update 
(
	void
)
{
    if ( missionData.state >= missionData.MISSION_STATE_BLACKWELL_IS_DEAD )
    {
        // if the mission state no longer suits this program, halt execution of it

        return false;
    }

    switch ( alkaidPrison )
    {
        case VIS_STATE_HIDDEN:

            if ( MScript::IsVisibleToPlayer( missionData.hawkesPrison, PLAYER_ID ) )
            {
                alkaidPrison = VIS_STATE_VISIBLE;
            }    
            else
            {
                break;
            }

            // fall through

        case VIS_STATE_VISIBLE:

	        if ( !MScript::IsStreamPlaying( missionData.primaryHandle ) )
	        {
                // clear fog for a short distance around the prison to avoid wierdness in rendering

            	MScript::ClearHardFog( missionData.hawkesPrison, 2 );

                MScript::AlertMessage( missionData.hawkesPrison, NULL );

                missionData.primaryHandle = MScript::PlayAnimatedMessage( "M03BL13.wav", "Animate!!Blackwell2", 
                    MagpieLeft, MagpieTop, missionData.blackwellsShip, IDS_TM3_SUB_M03BL13 );

                AddAndDisplayObjective( IDS_TM3_OBJECTIVE10, missionData.primaryHandle );

                if ( !( missionData.outpostAlreadyBuilt && missionData.outpostAlreadyUpgraded &&
                    missionData.troopshipAlreadyBuilt ) )
                {
                    // only display these objectives if they are pertinent (i.e. the player hasn't
                    // already accomplished them earlier in the mission

                    AddAndDisplayObjective( IDS_TM3_OBJECTIVE11, missionData.primaryHandle, true );
                    AddAndDisplayObjective( IDS_TM3_OBJECTIVE12, missionData.primaryHandle, true );
                    AddAndDisplayObjective( IDS_TM3_OBJECTIVE13, missionData.primaryHandle, true );

                    // mark any already completed objectives

                    if ( missionData.outpostAlreadyBuilt )
                    {
                        MScript::MarkObjectiveCompleted( IDS_TM3_OBJECTIVE11 );
                    }

                    if ( missionData.troopshipAlreadyBuilt )
                    {
                        MScript::MarkObjectiveCompleted( IDS_TM3_OBJECTIVE12 );
                    }

                    if ( missionData.outpostAlreadyUpgraded )
                    {
                        MScript::MarkObjectiveCompleted( IDS_TM3_OBJECTIVE13 );
                    }
                }

            	MScript::RunProgramByName( "TM3_CheckForOutpost", MPartRef() );                

                missionData.state = missionData.MISSION_STATE_RESCUE_HAWKES;

                MScript::RunProgramByName( "TM3_CheckForCapturedPrison", MPartRef() );

                alkaidPrison = VIS_STATE_PROCESSED;
            }

            break;

        default:

            break;
    }

    switch ( alkaid2Inhibitor )
    {
        case VIS_STATE_HIDDEN:

            if ( MScript::IsVisibleToPlayer( missionData.alkaid2Inhibitor, PLAYER_ID ) )
            {
                alkaid2Inhibitor = VIS_STATE_VISIBLE;
            }    
            else
            {
                break;
            }

            // fall through

        case VIS_STATE_VISIBLE:

	        if ( !MScript::IsStreamPlaying( missionData.primaryHandle ) )
	        {
                missionData.primaryHandle = MScript::PlayAnimatedMessage( "M03BL06.wav", "Animate!!Blackwell2", 
                    MagpieLeft, MagpieTop, missionData.blackwellsShip,IDS_TM3_SUB_M03BL06 );

                // insert objective to destroy jump inhibitor into the objectives list

                MScript::RemoveFromObjectiveList( IDS_TM3_OBJECTIVE2 );
                MScript::RemoveFromObjectiveList( IDS_TM3_OBJECTIVE3 );

                AddAndDisplayObjective( IDS_TM3_OBJECTIVE9, missionData.primaryHandle, true );

	            MScript::AddToObjectiveList( IDS_TM3_OBJECTIVE2 );
	            MScript::AddToObjectiveList( IDS_TM3_OBJECTIVE3 );

                // turn on the AI in the next system

                TM3_TurnOnTheAI();

                alkaid2Inhibitor = VIS_STATE_PROCESSED;
            }

            break;
    }

    switch ( alkaid2Prime )
    {
        case VIS_STATE_HIDDEN:

            if ( MScript::IsVisibleToPlayer( missionData.alkaid2Prime, PLAYER_ID ) )
            {
                alkaid2Prime = VIS_STATE_VISIBLE;
            }    
            else
            {
                break;
            }

            // fall through

        case VIS_STATE_VISIBLE:

	        if ( !MScript::IsStreamPlaying( missionData.primaryHandle ) )
	        {
                missionData.primaryHandle = MScript::PlayAnimatedMessage( "M03BL08.wav", "Animate!!Blackwell2", 
                    MagpieLeft, MagpieTop, missionData.blackwellsShip,IDS_TM3_SUB_M03BL08 );

                alkaid2Prime = VIS_STATE_PROCESSED;
            }

            break;
    }

    switch ( alkaid23 )
    {
        case VIS_STATE_HIDDEN:

            if ( MScript::IsVisibleToPlayer( missionData.alkaid23, PLAYER_ID ) )
            {
                alkaid23 = VIS_STATE_VISIBLE;
            }    
            else
            {
                break;
            }

            // fall through

        case VIS_STATE_VISIBLE:

	        if ( !MScript::IsStreamPlaying( missionData.primaryHandle ) )
	        {
                missionData.primaryHandle = MScript::PlayAnimatedMessage( "M03BL09.wav", "Animate!!Blackwell2", 
                    MagpieLeft, MagpieTop, missionData.blackwellsShip ,IDS_TM3_SUB_M03BL09);

                alkaid23 = VIS_STATE_PROCESSED;
            }

            break;
    }

    switch ( alkaid24 )
    {
        case VIS_STATE_HIDDEN:

            if ( MScript::IsVisibleToPlayer( missionData.alkaid24, PLAYER_ID ) )
            {
                alkaid24 = VIS_STATE_VISIBLE;
            }    
            else
            {
                break;
            }

            // fall through

        case VIS_STATE_VISIBLE:

	        if ( !MScript::IsStreamPlaying( missionData.primaryHandle ) )
	        {
                missionData.primaryHandle = MScript::PlayAnimatedMessage( "M03BL10.wav", "Animate!!Blackwell2", 
                    MagpieLeft, MagpieTop, missionData.blackwellsShip ,IDS_TM3_SUB_M03BL10);

                alkaid24 = VIS_STATE_PROCESSED;
            }

            break;
    }

    switch ( gateToTheEnd )
    {
        case VIS_STATE_HIDDEN:

            if ( MScript::IsVisibleToPlayer( missionData.gateToTheEnd, PLAYER_ID ) )
            {
                gateToTheEnd = VIS_STATE_VISIBLE;
            }    
            else
            {
                break;
            }

            // fall through

        case VIS_STATE_VISIBLE:

	        if ( !MScript::IsStreamPlaying( missionData.primaryHandle ) )
	        {
                // complete the objective about the player finding a path to the other side of the ribbon.

            	MScript::MarkObjectiveCompleted( IDS_TM3_OBJECTIVE1 );

                missionData.primaryHandle = MScript::PlayAnimatedMessage( "M03BL12.wav", "Animate!!Blackwell2", 
                    MagpieLeft, MagpieTop, missionData.blackwellsShip, IDS_TM3_SUB_M03BL12 );

                gateToTheEnd = VIS_STATE_PROCESSED;
            }

            break;
    }

    return true;
}

//--------------------------------------------------------------------------//
// Program that checks if the prison that Hawkes is being held in has been
// damaged and warns the player if it has.

CQSCRIPTPROGRAM( TM3_HoldYourFire, TM3_HoldYourFireData, 0 );

void TM3_HoldYourFire::Initialize
( 
	U32 eventFlags, 
	const MPartRef &part 
)
{
}

bool TM3_HoldYourFire::Update 
(
	void
)
{
    if ( missionData.state >= missionData.MISSION_STATE_BLACKWELL_IS_DEAD )
    {
        // if the mission state no longer suits this program, halt execution of it

        return false;
    }

    if ( missionData.hawkesPrison.isValid() )
    {
        if ( missionData.hawkesPrison->hullPoints < missionData.hawkesPrison->hullPointsMax )
        {
	        if ( !MScript::IsStreamPlaying( missionData.primaryHandle ) )
            {
                missionData.primaryHandle = MScript::PlayAnimatedMessage( "M03BL14.wav", "Animate!!Blackwell2", 
                    MagpieLeft, MagpieTop, missionData.blackwellsShip , IDS_TM3_SUB_M03BL14);

                return false;
            }
        }
    }
    else
    {
        return false;
    }
        

    return true;
}

//---------------------------------------------------------------------------
// program that check if the user has upgraded his outpost

CQSCRIPTPROGRAM( TM3_OutpostUpgradeComplete, TM3_OutpostUpgradeCompleteData, 0 );

void TM3_OutpostUpgradeComplete::Initialize
( 
	U32 eventFlags, 
	const MPartRef &part 
)
{
}

bool TM3_OutpostUpgradeComplete::Update 
(
	void
)
{
    struct TECHNODE tempNode;

    if ( missionData.state >= missionData.MISSION_STATE_BLACKWELL_IS_DEAD )
    {
        // if the mission state no longer suits this program, halt execution of it

        return false;
    }

    tempNode = MScript::GetPlayerTech( PLAYER_ID );

    if ( tempNode.race[0].tech & TECHTREE::T_RES_TROOPSHIP1 )
    {        
        if ( missionData.state == missionData.MISSION_STATE_RESCUE_HAWKES )
        {
            MScript::MarkObjectiveCompleted( IDS_TM3_OBJECTIVE13 );

            if ( missionData.outpostAlreadyBuilt && missionData.troopshipAlreadyBuilt )
            {               
                MScript::RemoveFromObjectiveList( IDS_TM3_OBJECTIVE11 );
                MScript::RemoveFromObjectiveList( IDS_TM3_OBJECTIVE12 );
                MScript::RemoveFromObjectiveList( IDS_TM3_OBJECTIVE13 );
            }
        }

        missionData.outpostAlreadyUpgraded = true;

        return false;
    }

    return true;
}

//---------------------------------------------------------------------------
// Program executed when Blackwell dies, which does a little dance, then ends the mission

CQSCRIPTPROGRAM( TM3_BlackwellIsDead, TM3_BlackwellIsDeadData, 0 );

void TM3_BlackwellIsDead::Initialize 
(
	U32 eventFlags, 
	const MPartRef &part
)
{
    UnderMissionControl();

	MScript::EnableMovieMode( true );

    missionData.secondaryHandle = MScript::PlayAnimatedMessage( "M03BL20.wav", "Animate!!Blackwell2", 
        MagpieLeft, MagpieTop, missionData.blackwellsShip,IDS_TM3_SUB_M03BL20 );

    halseyStartedTalking = false;
}

bool TM3_BlackwellIsDead::Update 
(
	void
)
{
    if ( missionData.state != missionData.MISSION_STATE_BLACKWELL_IS_DEAD )
    {
        // if the mission state no longer suits this program, halt execution of it

        return false;
    }

	if ( !MScript::IsStreamPlaying( missionData.secondaryHandle ) &&
        !MScript::IsStreamPlaying( missionData.primaryHandle ) )
	{
        if ( halseyStartedTalking )
        {
            missionData.state = missionData.MISSION_STATE_END_DEFEAT;

			missionData.failureID = IDS_TM3_FAIL_BLACKWELL_LOST;
	        MScript::RunProgramByName( "TM3_MissionFailure", MPartRef ());

	        return false;
        }
        else
        {
            missionData.primaryHandle = MScript::PlayAnimatedMessage( "M03HA04.wav", "Animate!!Halsey2", 
                MagpieLeft, MagpieTop, IDS_TM3_SUB_M03HA04 );

            halseyStartedTalking = true;
        }
    }

    return true;
}

//---------------------------------------------------------------------------
// Program executed when Hawke's prison is destroyed, which does a little dance, 
// then ends the mission

CQSCRIPTPROGRAM( TM3_PrisonDestroyed, TM3_PrisonDestroyedData, 0 );

void TM3_PrisonDestroyed::Initialize 
(
	U32 eventFlags, 
	const MPartRef &part
)
{
    UnderMissionControl();

	MScript::EnableMovieMode( true );

    missionData.primaryHandle = MScript::PlayAnimatedMessage( "M03BL19.wav", "Animate!!Blackwell2", 
        MagpieLeft, MagpieTop, missionData.blackwellsShip, IDS_TM3_SUB_M03BL19 );

    halseyStartedTalking = false;
}

bool TM3_PrisonDestroyed::Update 
(
	void
)
{
    if ( missionData.state != missionData.MISSION_STATE_PRISON_WAS_DESTROYED )
    {
        // if the mission state no longer suits this program, halt execution of it

        return false;
    }

	if ( !MScript::IsStreamPlaying( missionData.secondaryHandle ) &&
        !MScript::IsStreamPlaying( missionData.primaryHandle ) )
	{
        if ( halseyStartedTalking )
        {
            missionData.state = missionData.MISSION_STATE_END_DEFEAT;

	        MScript::RunProgramByName( "TM3_MissionFailure", MPartRef ());

	        return false;
        }
        else
        {
            missionData.primaryHandle = MScript::PlayAnimatedMessage( "M03HA03.wav", "Animate!!Halsey2", 
                MagpieLeft, MagpieTop ,IDS_TM3_SUB_M03HA03);

            halseyStartedTalking = true;
        }
    }

    return true;
}

//---------------------------------------------------------------------------
// Program executed when player succeeds in the mission

CQSCRIPTPROGRAM( TM3_MissionFailure, TM3_MissionFailureData, 0 );

void TM3_MissionFailure::Initialize 
(
	U32 eventFlags, 
	const MPartRef &part
)
{
    MScript::FlushTeletype();

    missionData.secondaryHandle = TeletypeMissionOver( IDS_TM3_MISSION_FAILURE,missionData.failureID );

    missionData.state = missionData.MISSION_STATE_END_DEFEAT;
}

bool TM3_MissionFailure::Update 
(
	void
)
{
    if ( !MScript::IsTeletypePlaying( missionData.secondaryHandle ) )
	{
        UnderPlayerControl();	
	    MScript::EndMissionDefeat();

	    return false;
    }
    else
    {
        return true;
    }
}

//---------------------------------------------------------------------------
// This program checks to see if Hawkes prison has been captured, and end the 
// mission in a victory if it has been.

CQSCRIPTPROGRAM( TM3_CheckForCapturedPrison, TM3_CheckForCapturedPrisonData, 0 );

void TM3_CheckForCapturedPrison::Initialize
( 
	U32 eventFlags, 
	const MPartRef &part 
)
{
    state = CAPTURE_STATE_CHECKING;
}

bool TM3_CheckForCapturedPrison::Update 
(
	void
)
{
    if ( missionData.state >= missionData.MISSION_STATE_BLACKWELL_IS_DEAD &&
        missionData.state != missionData.MISSION_STATE_PRISON_CAPTURED )
    {
        // if the mission state no longer suits this program, halt execution of it

        return false;
    }

    switch ( state )
    {
        case CAPTURE_STATE_CHECKING:

            if ( missionData.hawkesPrisonDestroyed  )
            {
                return false;
            }

            if ( !missionData.hawkesPrison.isValid() )
            {
                // prison has been captured (since we already know it hasn't been destroyed
                // due to above test).  Yeehaa, we just won.

                missionData.state = missionData.MISSION_STATE_PRISON_CAPTURED;

            	UnderMissionControl();

            	MScript::EnableMovieMode( true );

                MScript::ChangeCamera( MScript::GetPartByName( "AfterWarlordCam" ), 0, MOVIE_CAMERA_JUMP_TO );

                // pause so the player actually sees the facility being taken over before
                // cut-scene is triggerd.

                state = CAPTURE_STATE_DRAMATIC_PAUSE;

                afterPauseState = CAPTURE_STATE_CAPTURED;

                dramaticPauseCounter = 1;
            }

            break;

        case CAPTURE_STATE_CAPTURED:

            UnderPlayerControl();	
        	MScript::EndMissionVictory(2);

            return false;

            break;

        case CAPTURE_STATE_DRAMATIC_PAUSE:

            dramaticPauseCounter--;

            if ( dramaticPauseCounter <= 0 )
            {
                state = afterPauseState;
            }

            break;

        default:

            break;
    }

    return true;
}

static void TM3_TurnOnTheAI
(
    void
)
{
	AIPersonality airules;

	MScript::EnableEnemyAI(MANTIS_ID, true, "MANTIS_FRIGATE_RUSH");

	airules.difficulty = MEDIUM;
	airules.buildMask.bBuildPlatforms = true;
	airules.buildMask.bBuildLightGunboats = true;
	airules.buildMask.bBuildMediumGunboats = false;
	airules.buildMask.bBuildHeavyGunboats = false;
	airules.buildMask.bHarvest = true;
	airules.buildMask.bScout = true;
	airules.buildMask.bBuildAdmirals = false;
	airules.buildMask.bLaunchOffensives = true;
	airules.nGunboatsPerSupplyShip = 5;
    airules.nNumFleets = 1;  
	airules.nNumMinelayers = 2;
	airules.uNumScouts = 3;
	airules.nNumFabricators = 2;

	MScript::SetEnemyAIRules(MANTIS_ID, airules);

    MassEnableAI( MANTIS_ID, true, SYSTEM_ID_TAU_CETI );
    MassEnableAI( MANTIS_ID, true, SYSTEM_ID_ALKAID );
    MassEnableAI( MANTIS_ID, true, SYSTEM_ID_ALKAID_2 );
    MassEnableAI( MANTIS_ID, true, SYSTEM_ID_ALKAID_3 );
}

//**************************************************************************
// Objective Handler Routines
//**************************************************************************

static void AddAndDisplayObjective
(
    U32 stringID,
    U32 dependantHandle,
    bool isSecondary
)
{
    MScript::AddToObjectiveList( stringID, isSecondary );

    missionData.displayObjectiveHandlerParams_stringID = stringID;
    missionData.displayObjectiveHandlerParams_dependantHandle = dependantHandle;

    MScript::RunProgramByName( "TM3_DisplayObjectiveHandler", MPartRef ());
}

CQSCRIPTPROGRAM( TM3_DisplayObjectiveHandler, TM3_DisplayObjectiveHandlerData, 0 );

void TM3_DisplayObjectiveHandler::Initialize
(
	U32 eventFlags, 
	const MPartRef &part 
)
{
    stringID = missionData.displayObjectiveHandlerParams_stringID;
    dependantHandle = missionData.displayObjectiveHandlerParams_dependantHandle;
}

bool TM3_DisplayObjectiveHandler::Update
(
    void
)
{
    if ( missionData.state >= missionData.MISSION_STATE_BLACKWELL_IS_DEAD )
    {
        // if the mission state no longer suits this program, halt execution of it

        return false;
    }

    if ( MScript::IsObjectiveCompleted( stringID ) || !MScript::IsObjectiveInList( stringID ) )
    {
        // if the objective has been completed, do not display it at all

        return false;
    }
    else if ( missionData.displayObjectiveHandlerParams_dependantHandle == 0 || 
        !MScript::IsStreamPlaying( dependantHandle ) )
    {
        // make sure that any previous objectives are finished playing

        if ( missionData.displayObjectiveHandlerParams_teletypeHandle == 0 ||
            !MScript::IsTeletypePlaying( missionData.displayObjectiveHandlerParams_teletypeHandle ) )
        {
		    missionData.displayObjectiveHandlerParams_teletypeHandle = TeletypeObjective( stringID );

            return false;
        }
    }

    return true;
}