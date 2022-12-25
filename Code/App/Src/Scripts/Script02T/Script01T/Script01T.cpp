//--------------------------------------------------------------------------//
//                                                                          //
//                                Script01T.cpp                             //
//                                                                          //
//               COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
   $Header: /Conquest/App/Src/Scripts/Script01T/Script01T.cpp 166   7/25/01 4:56p Tmauer $
*/
//--------------------------------------------------------------------------//

#include <ScriptDef.h>
#include <DLLScriptMain.h>
#include "DataDef.h"
#include "stdlib.h"

#include "..\helper\helper.h"

//--------------------------------------------------------------------------
//  TM1 #define's

#define PLAYER_ID			1
#define MANTIS_ID			2

#define TERRA				1
#define TAU_CETI			2
#define MANTIS_SPACE		3
#define SYSTEM_ID_ALKAID_2  5
#define SYSTEM_ID_ALKAID_3  4

//--------------------------------------------------------------------------//
//  FUNCTION PROTOTYPES
//--------------------------------------------------------------------------//

static void AddAndDisplayObjective
(
    U32 stringID,
    U32 dependantHandle = 0,
    bool isSecondary = false
);

static void TM1_TurnOnTheAI
(
    void
);

//--------------------------------------------------------------------------

CQSCRIPTDATA(MissionData, data);

//--------------------------------------------------------------------------
//  Mission functions: Terran Campaign Mission 1
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------//
//	Mission Briefing
//--------------------------------------------------------------------------//

CQSCRIPTPROGRAM(TM1_Briefing, TM1_Briefing_Save,CQPROGFLAG_STARTBRIEFING);

void TM1_Briefing::Initialize 
(
    U32 eventFlags, 
    const MPartRef & part
)
{
	data.failureID = 0;
	data.mission_over = false;
	data.mhandle = 0;
	data.shandle = 0;
	data.mission_state = StartMission;

    data.gameAIEnabled = false;

	MScript::EnableBriefingControls( true );

    MScript::PlayMusic( NULL );

    data.displayObjectiveHandlerParams_teletypeHandle = 0;

	state = BRIEFING_STATE_RADIO_BROADCAST;

	// Set dynamic objectives up...

	Andro = Hawkes = Worm = Austin = Blackwell = false;
}

bool TM1_Briefing::Update
( 
    void 
) 
{
    CQBRIEFINGITEM slotAnim;

	switch(state)
	{
		case BRIEFING_STATE_RADIO_BROADCAST:

			data.mhandle = MScript::PlayAudio( "prolog01.wav" );
			MScript::BriefingSubtitle(data.mhandle,IDS_TM1_SUB_PROLOG01);

            ShowBriefingAnimation( 0, "TNRLogo", 100, true, true );
            ShowBriefingAnimation( 3, "TNRLogo", 100, true, true );
            ShowBriefingAnimation( 1, "Radiowave", ANIMSPD_RADIOWAVE, true, true );
            ShowBriefingAnimation( 2, "Radiowave", ANIMSPD_RADIOWAVE, true, true );

            MScript::FlushTeletype();

            data.thandle = MScript::PlayBriefingTeletype( IDS_TM1_TELETYPE_LOCATION, 
                STANDARD_TELETYPE_COLOR, STANDARD_TELETYPE_HOLD_TIME, 
                MScript::GetScriptStringLength( IDS_TM1_TELETYPE_LOCATION ) * STANDARD_TELETYPE_MILS_PER_CHAR, 
                false );
			
			state = BRIEFING_STATE_UPLINK;

			break;

		case BRIEFING_STATE_UPLINK:

            if ( !MScript::IsStreamPlaying( data.mhandle ) )
			{
                MScript::FreeBriefingSlot( -1 );

				ShowBriefingAnimation( 0, "Xmission", ANIMSPD_XMISSION, false, false ); 

                data.thandle = MScript::PlayBriefingTeletype( IDS_TM1_INCOMING,
                    STANDARD_TELETYPE_COLOR, STANDARD_TELETYPE_HOLD_TIME, 
                    MScript::GetScriptStringLength( IDS_TM1_INCOMING ) * STANDARD_TELETYPE_MILS_PER_CHAR, 
                    false );

                data.mhandle = MScript::PlayAudio( "high_level_data_uplink.wav" );

				state = BRIEFING_STATE_HALSEY_BRIEF;
			}

			break;

		case BRIEFING_STATE_HALSEY_BRIEF:

			if ( !MScript::IsStreamPlaying( data.mhandle ) )
			{
				strcpy( slotAnim.szFileName, "m01ha01.wav" );
				strcpy( slotAnim.szTypeName, "Animate!!Halsey" );

				slotAnim.slotID = 0;
				slotAnim.bHighlite = false;
                data.mhandle = MScript::PlayBriefingTalkingHead( slotAnim );
				MScript::BriefingSubtitle(data.mhandle,IDS_TM1_SUB_M01HA01);

			    state = BRIEFING_STATE_HALSEY_BRIEF_2;
			}

			break;
			
		case BRIEFING_STATE_HALSEY_BRIEF_2:

			if ( !MScript::IsStreamPlaying( data.mhandle ) )
			{
                // Show Andomeda coming through wormhole
				
                ShowBriefingAnimation( 1, "AndromWorm" );

				strcpy( slotAnim.szFileName, "m01ha01a.wav" );
				strcpy( slotAnim.szTypeName, "Animate!!Halsey" );

				slotAnim.slotID = 0;
				slotAnim.bHighlite = false;
                data.mhandle = MScript::PlayBriefingTalkingHead( slotAnim );
				MScript::BriefingSubtitle(data.mhandle,IDS_TM1_SUB_M01HA01A);

			    state = BRIEFING_STATE_HALSEY_BRIEF_3;
			}

			break;
			
		case BRIEFING_STATE_HALSEY_BRIEF_3:
			
			if ( !MScript::IsStreamPlaying( data.mhandle ) )
			{
                // Show TNS Austin
				
                ShowBriefingAnimation( 2, "Austin" );

				strcpy( slotAnim.szFileName, "m01ha01b.wav" );
				strcpy( slotAnim.szTypeName, "Animate!!Halsey" );

				slotAnim.slotID = 0;
				slotAnim.bHighlite = false;
                data.mhandle = MScript::PlayBriefingTalkingHead( slotAnim );
				MScript::BriefingSubtitle(data.mhandle,IDS_TM1_SUB_M01HA01B);

			    state = BRIEFING_STATE_HALSEY_BRIEF_4;
			}

			break;

		case BRIEFING_STATE_HALSEY_BRIEF_4:
			
			if ( !MScript::IsStreamPlaying( data.mhandle ) )
			{
				strcpy( slotAnim.szFileName, "m01ha01c.wav" );
				strcpy( slotAnim.szTypeName, "Animate!!Halsey" );

				slotAnim.slotID = 0;
				slotAnim.bHighlite = false;
                data.mhandle = MScript::PlayBriefingTalkingHead( slotAnim );
				MScript::BriefingSubtitle(data.mhandle,IDS_TM1_SUB_M01HA01C);

			    state = BRIEFING_STATE_BLACKWELL;
			}

			break;

		case BRIEFING_STATE_BLACKWELL:
			
			if ( !MScript::IsStreamPlaying( data.mhandle ) )
			{
                MScript::FreeBriefingSlot( -1 );

				strcpy( slotAnim.szFileName, "m01bl02.wav" );
				strcpy( slotAnim.szTypeName, "Animate!!Blackwell" );

				slotAnim.slotID = 3;
				slotAnim.bHighlite = false;
                data.mhandle = MScript::PlayBriefingTalkingHead( slotAnim );
				MScript::BriefingSubtitle(data.mhandle,IDS_TM1_SUB_M01BL02);

			    state = BRIEFING_STATE_OBJECTIVES;
			}

			break;

		case BRIEFING_STATE_OBJECTIVES:

            if ( !MScript::IsStreamPlaying( data.mhandle ) )
			{
                MScript::FreeBriefingSlot( -1 );

                data.thandle = MScript::PlayBriefingTeletype( IDS_TM1_OBJECTIVES,
                    STANDARD_TELETYPE_COLOR, 0, 
                    MScript::GetScriptStringLength( IDS_TM1_OBJECTIVES ) * STANDARD_TELETYPE_MILS_PER_CHAR, 
                    false );

				state = BRIEFING_STATE_FINISHED;
			}

			break;

		case BRIEFING_STATE_FINISHED:			

			break;			
	}

	return true;
}
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//	Start of the mission... After the briefing

CQSCRIPTPROGRAM(TM1_MissionStart, TM1_MissionStart_Save,CQPROGFLAG_STARTMISSION);

void TM1_MissionStart::Initialize (U32 eventFlags, const MPartRef & part)
{
    MScript::SetMissionID( 0 );
	MScript::SetMissionName( IDS_TM1_NAME );
	MScript::SetMissionDescription( IDS_TM1_DESCRIPTION );

    MScript::SetEnemyCharacter( MANTIS_ID, IDS_TM1_MANTIS_PLAYER_NAME );

	// Set dynamic objectives up...

	MScript::AddToObjectiveList( IDS_TM1_OBJECTIVE2 );
	MScript::AddToObjectiveList( IDS_TM1_OBJECTIVE3 );
	MScript::AddToObjectiveList( IDS_TM1_OBJECTIVE5 );

	data.mission_tech.InitLevel(TECHTREE::FULL_TREE);

	// 0 is the Terran players race...

	data.mission_tech.race[0].build = (TECHTREE::BUILDNODE) (
        TECHTREE::TDEPEND_HEADQUARTERS | TECHTREE::TDEPEND_SENSORTOWER | 
        TECHTREE::TDEPEND_LASER_TURRET | TECHTREE::RES_REFINERY_GAS1 | 
        TECHTREE::RES_REFINERY_METAL1);

	data.mission_tech.race[0].tech = (TECHTREE::TECHUPGRADE) (
        TECHTREE::T_SHIP__FABRICATOR | TECHTREE::T_SHIP__CORVETTE | 
        TECHTREE::T_SHIP__HARVEST);

	data.mission_tech.race[0].common_extra = (TECHTREE::COMMON_EXTRA) (
        TECHTREE::RES_TANKER1 | TECHTREE::RES_SENSORS1);

	data.mission_tech.race[0].common = (TECHTREE::COMMON) (
        TECHTREE::NO_COMMONUPGRADE);

	MScript::SetAvailiableTech(data.mission_tech);

	//set starting resources

	MScript::SetMetal( PLAYER_ID, 292 );
	MScript::SetGas( PLAYER_ID, 108 );
	MScript::SetCrew( PLAYER_ID, 200 );

	MScript::SetMaxMetal( PLAYER_ID, 292 );
	MScript::SetMaxGas( PLAYER_ID, 108 );
	MScript::SetMaxCrew( PLAYER_ID, 200 );

	data.corvettes = 4;

	data.WayPoint1 = MScript::GetPartByName( "WayPoint1" );
	data.WayPoint2 = MScript::GetPartByName( "WayPoint2" );
	data.WayPoint3 = MScript::GetPartByName( "WayPoint3" );

	// Create triggers around Tau Ceti Prme and it's wormhole...

	data.PlanetTrigger = MScript::CreatePart( "MISSION!!TRIGGER", MScript::GetPartByName( "Tau Ceti Prime" ), 0);
	MScript::EnableTrigger( data.PlanetTrigger, false );

	data.WormholeTrigger = MScript::CreatePart( "MISSION!!TRIGGER", MScript::GetPartByName( "Gate2" ), 0);
	MScript::EnableTrigger( data.WormholeTrigger, false );

	data.alkaidPrimeTrigger = MScript::CreatePart( "MISSION!!TRIGGER", MScript::GetPartByName( "Alkaid Prime" ), 0);
	MScript::EnableTrigger( data.alkaidPrimeTrigger, false );

	data.MantisGenWormhole = MScript::GetPartByName( "MantisGenWormhole" );

	data.Beacon	= MScript::GetPartByName( "Andromeda Beacon" );

	MScript::EnableJumpgate( MScript::GetPartByName( "Gate1" ), false );
	MScript::EnableJumpgate( MScript::GetPartByName( "Gate2" ), false );
	MScript::EnableJumpgate( MScript::GetPartByName( "Gate3" ), false );
	MScript::EnableJumpgate( MScript::GetPartByName( "Gate4" ), false );
	MScript::EnableJumpgate( MScript::GetPartByName( "Gate5" ), false );
	MScript::EnableJumpgate( MScript::GetPartByName( "Gate6" ), false );
	MScript::EnableJumpgate( MScript::GetPartByName( "Gate7" ), false );
	MScript::EnableJumpgate( MScript::GetPartByName( "Gate8" ), false );
	MScript::EnableJumpgate( MScript::GetPartByName( "Gate9" ), false );

	MScript::MoveCamera( MScript::GetPartByName("Gate1"), 0, MOVIE_CAMERA_JUMP_TO);

    // clear the fog around the wormhole to a sufficient distance to be able to see Tau Ceti Prime

	MScript::ClearHardFog( MScript::GetPartByName("Gate1"), 
        MScript::DistanceTo( MScript::GetPartByName("Gate1" ),
        MScript::GetPartByName("Tau Ceti Prime") )
        );

	data.Corvette1= MScript::GetPartByName("TNS Seattle");
	MScript::SetStance(data.Corvette1, US_DEFEND);

	data.Corvette2 = MScript::GetPartByName("TNS Tacoma");
	MScript::SetStance(data.Corvette2, US_DEFEND);

	data.Blackwell = MScript::GetPartByName("Blackwell");
	MScript::EnablePartnameDisplay(data.Blackwell, true);
	MScript::SetStance(data.Blackwell, US_DEFEND);

	data.TNSAustin	= MScript::GetPartByName("TNS Austin");
	MScript::SetStance(data.TNSAustin, US_DEFEND);

	// Warp Blackwell in...

	MScript::OrderUseJumpgate( data.Blackwell, MScript::GetPartByName("Gate0") );

	MScript::EnableSystem(TERRA, false);
	MScript::EnableSystem(MANTIS_SPACE, false);

    MScript::EnableRegenMode( true );

    MScript::PlayMusic( "mystery.wav" );

    MScript::RunProgramByName( "TM1_VisibilityHandler", MPartRef() );

    state = Begin;
}

bool TM1_MissionStart::Update (void)
{
	
	if (data.mission_over)
	{
		// Check to see if mission has ended. If so don't bother anymore...

		return false;
	}

	if ( state == Begin )
	{
        if ( data.Blackwell->systemID == TAU_CETI )
        {
            MScript::EndStream( data.mhandle );
	    
            data.mhandle = MScript::PlayAnimatedMessage( "M01BL03.wav", "Animate!!Blackwell2", 
                MagpieLeft, MagpieTop, data.Blackwell ,IDS_TM1_SUB_M01BL03);

            MScript::RemoveFromObjectiveList( IDS_TM1_OBJECTIVE3 );
            MScript::RemoveFromObjectiveList( IDS_TM1_OBJECTIVE5 );

            AddAndDisplayObjective( IDS_TM1_OBJECTIVE2_1, data.mhandle, true );

            MScript::AddToObjectiveList( IDS_TM1_OBJECTIVE3 );
            MScript::AddToObjectiveList( IDS_TM1_OBJECTIVE5 );

		    state = Done;
        }
	}
    else if ( state == Done && !MScript::IsStreamPlaying( data.mhandle ) )
	{
		MScript::RunProgramByName( "TM1_SelectAndMoveBlackwell", MPartRef () );

		return false;
	}
    else if ( MScript::IsSelectedUnique( data.Blackwell ) &&
        !MScript::IsObjectiveCompleted( IDS_TM1_OBJECTIVE2_1 ) )
    {
        MScript::MarkObjectiveCompleted( IDS_TM1_OBJECTIVE2_1 );
    }    
	
	return true;
}

//--------------------------------------------------------------------------//
//	Teach the player the basics of selction, moving and group selection

// Player selects blackwell as UI test
CQSCRIPTPROGRAM(TM1_SelectAndMoveBlackwell, TM1_SelectAndMoveBlackwell_Save,0);

void TM1_SelectAndMoveBlackwell::Initialize (U32 eventFlags, const MPartRef & part)
{
	StageForJump			= MScript::GetPartByName("Ready to jump");

	state = Begin;
	dist = MScript::DistanceTo(data.Blackwell, data.WayPoint3);

    lastSelectedShip = data.Blackwell;
}

bool TM1_SelectAndMoveBlackwell::Update (void)
{
    MGroupRef tempGroup;

	if (data.mission_over)
	{
		// Check to see if mission has ended. If so don't bother anymore...

		return false;
	}

	switch (state)
	{
	case Begin:

		if (MScript::IsSelectedUnique(data.Blackwell))
		{
            if ( !MScript::IsObjectiveCompleted( IDS_TM1_OBJECTIVE2_1 ) )
            {
                MScript::MarkObjectiveCompleted( IDS_TM1_OBJECTIVE2_1 );
            }

            MScript::EndStream( data.mhandle );

            data.mhandle = MScript::PlayAnimatedMessage( "M01BL05.wav", "Animate!!Blackwell2", 
                MagpieLeft, MagpieTop, data.Blackwell,IDS_TM1_SUB_M01BL05 );

            MScript::RemoveFromObjectiveList( IDS_TM1_OBJECTIVE3 );
            MScript::RemoveFromObjectiveList( IDS_TM1_OBJECTIVE5 );

            AddAndDisplayObjective( IDS_TM1_OBJECTIVE2_2, data.mhandle, true );

            MScript::AddToObjectiveList( IDS_TM1_OBJECTIVE3 );
            MScript::AddToObjectiveList( IDS_TM1_OBJECTIVE5 );

			state = MoveBlackwell;
		}

		break;

	case MoveBlackwell:

			if ( !MScript::IsIdle( data.Blackwell ) )
			{
                MScript::MarkObjectiveCompleted( IDS_TM1_OBJECTIVE2_2 );

                MScript::EndStream( data.mhandle );

                data.mhandle = MScript::PlayAnimatedMessage( "M01BL06.wav", "Animate!!Blackwell2", 
                    MagpieLeft, MagpieTop, data.Blackwell,IDS_TM1_SUB_M01BL06 );

                MScript::RemoveFromObjectiveList( IDS_TM1_OBJECTIVE3 );
                MScript::RemoveFromObjectiveList( IDS_TM1_OBJECTIVE5 );

                AddAndDisplayObjective( IDS_TM1_OBJECTIVE2_3, data.mhandle, true );

                MScript::AddToObjectiveList( IDS_TM1_OBJECTIVE3 );
                MScript::AddToObjectiveList( IDS_TM1_OBJECTIVE5 );

				// Teleport to ready stage...

				MScript::OrderTeleportTo( data.Corvette1, StageForJump );
				MScript::OrderTeleportTo( data.Corvette2, StageForJump );

				// Warp in your squadron
				
                tempGroup += data.Corvette1;
                tempGroup += data.Corvette2;

				MScript::OrderUseJumpgate( tempGroup, MScript::GetPartByName("Gate0") );

				state = SelectGroup;
			}
		
		break;

	case SelectGroup:

		if ( MScript::IsSelected( data.Corvette1 ) && MScript::IsSelected( data.Corvette2 ) &&
            MScript::IsSelected( data.Blackwell ) )
		{
		    MScript::MarkObjectiveCompleted( IDS_TM1_OBJECTIVE2_3 );

            MScript::EndStream( data.mhandle );

            data.mhandle = MScript::PlayAnimatedMessage( "M01BL07.wav", "Animate!!Blackwell2", 
                MagpieLeft, MagpieTop, data.Blackwell,IDS_TM1_SUB_M01BL07 );

            // done linking up with Blackwell

		    MScript::MarkObjectiveCompleted( IDS_TM1_OBJECTIVE2 );

            // remove secondary objectives involving linking up with Blackwell

            MScript::RemoveFromObjectiveList( IDS_TM1_OBJECTIVE2_1 );
            MScript::RemoveFromObjectiveList( IDS_TM1_OBJECTIVE2_2 );
            MScript::RemoveFromObjectiveList( IDS_TM1_OBJECTIVE2_3 );
            
			state = LookForWormHole;
		}

		break;

	case LookForWormHole:
		
		//setup trigger "WormholeTrigger" to detect player scouting wormhole.
		
		MScript::SetTriggerProgram(data.WormholeTrigger, "TM1_EnterTheMantis");
		MScript::SetTriggerRange(data.WormholeTrigger, 4);
		MScript::SetTriggerFilter(data.WormholeTrigger, PLAYER_ID, TRIGGER_PLAYER, true);
		MScript::EnableTrigger(data.WormholeTrigger, true);

		return false;
		
	}

return true;
}


//--------------------------------------------------------------------------//
//	When the player gets close to the Tau Ceti wormhole
//  Trigger is set by SelectAndMoveBlackwell

CQSCRIPTPROGRAM(TM1_EnterTheMantis, TM1_EnterTheMantis_Save,0);

void TM1_EnterTheMantis::Initialize (U32 eventFlags, const MPartRef & part)
{
	// Disable this trigger...

	MScript::EnableTrigger(part, false);

	MScript::EnableSelection(data.TNSAustin, false);

	TNSAustinDest = MScript::GetPartByName("TNSAustinDest");
	
	// Warp into Tau Ceti

	MScript::SetHullPoints(data.TNSAustin, 100);
	MScript::OrderTeleportTo(data.TNSAustin, MScript::GetPartByName("MantisGenWormhole"));
	
	MScript::ClearPath(TAU_CETI, MANTIS_SPACE, MANTIS_ID);
	MScript::ClearPath(TAU_CETI, MANTIS_SPACE, PLAYER_ID);	
	
	MScript::FlushStreams();

	// Move focus to the wormhole so we can see the TNS Austin jump in...

    MScript::AlertMessage( data.WormholeTrigger, NULL );

	// We've now scouted the wormhole...

	MScript::MarkObjectiveCompleted(IDS_TM1_OBJECTIVE3);

	// Start up the general Mantis AI

	MScript::RunProgramByName("TM1_MantisAI", MPartRef ());

	data.mission_state = AttackOnTNSAustin;

	state = PrePreBegin;
	corvettesIdle = false;
	timer = 1.5;
}

bool TM1_EnterTheMantis::Update (void)
{
    MGroupRef tempGroup;

	timer -= ELAPSED_TIME;

	if (data.mission_over)
	{
		// Check to see if mission has ended. If so don't bother anymore...

		return false;
	}

	switch (state)
	{
        case PrePreBegin:

            // wait a second after the alert goes off before Blackwell starts blabbing..

            if ( timer <= 0 )
            {
                // tell the player how alerts work

                data.mhandle = MScript::PlayAnimatedMessage( "M01BL99.wav", "Animate!!Blackwell2", 
                    MagpieLeft, MagpieTop, data.Blackwell,IDS_TM1_SUB_M01BL99 );

                state = PreBegin;
            }

            break;

	    case PreBegin:

            // wait for blackwell to finish describing alerts.

    		if ( !MScript::IsStreamPlaying( data.shandle ) && !MScript::IsStreamPlaying( data.mhandle ) )
            {
			    MScript::OrderMoveTo( data.TNSAustin, TNSAustinDest );

			    // Fuck up the TNS Austin, but make it invincible until the sequence plays out

			    MScript::SetHullPoints( data.TNSAustin, 25 );

                MScript::MakeInvincible( data.TNSAustin, true );

			    state = Begin;
            }

		    break;

	    case Begin:

		    if(data.TNSAustin->systemID == TAU_CETI)
		    {
			    Mantis1 = MScript::CreatePart("GBOAT!!M_Frigate", data.MantisGenWormhole, MANTIS_ID);
			    Mantis2 = MScript::CreatePart("GBOAT!!M_Frigate", data.MantisGenWormhole, MANTIS_ID);
			    Mantis3 = MScript::CreatePart("GBOAT!!M_Frigate", data.MantisGenWormhole, MANTIS_ID);
			    data.mantis_ships = 3;

			    MScript::SetVisibleToPlayer(data.TNSAustin, MANTIS_ID);

                tempGroup += Mantis1;
                tempGroup += Mantis2;
                tempGroup += Mantis3;

			    MScript::OrderAttack( tempGroup,data.TNSAustin );  

			    MScript::FlushStreams();
			    data.shandle = MScript::PlayAudio( "M01AU09.wav", data.TNSAustin ,IDS_TM1_SUB_M01AU09);
			    state = BackToBoys;
		    }

		    break;

	    case BackToBoys:

            // This state used to do some camera movement, but that is no longer in the game -JUS

			state = BlackwellOrdersAttack;

		    break;

	    case BlackwellOrdersAttack:

		    if (!MScript::IsStreamPlaying(data.shandle) && !MScript::IsStreamPlaying(data.mhandle))
		    {
			    if (Mantis1.isValid() || Mantis2.isValid() || Mantis3.isValid())
                {
                    data.mhandle = MScript::PlayAnimatedMessage( "M01BL10.wav", "Animate!!Blackwell2", 
                        MagpieLeft, MagpieTop, data.Blackwell,IDS_TM1_SUB_M01BL10 );

                    AddAndDisplayObjective( IDS_TM1_OBJECTIVE4, data.mhandle );
                }
                else
                {
                	MScript::AddToObjectiveList( IDS_TM1_OBJECTIVE4 );
                }

			    state = AttackMantis;
		    }
		    
		    break;

	    case  AttackMantis:

		    if ( !MScript::IsStreamPlaying( data.mhandle ) )
            {
                // turn off the Austin's invincibility after blackwell is done talking

                MScript::MakeInvincible( data.TNSAustin, false );
            }

		    if ( !data.mantis_ships )
		    {
			    if ( data.TNSAustin.isValid() )
			    {
				    // if the Mantis are dead and the TNS Austin is still alive, blow it up..

				    MScript::DestroyPart( data.TNSAustin );
			    }
                else if ( !MScript::IsStreamPlaying( data.mhandle ) && !MScript::IsStreamPlaying( data.shandle ) )
			    {
        		    MScript::MarkObjectiveCompleted( IDS_TM1_OBJECTIVE4 );

                    data.mhandle = MScript::PlayAnimatedMessage( "M01BL13.wav", "Animate!!Blackwell2", 
                        MagpieLeft, MagpieTop, data.Blackwell,IDS_TM1_SUB_M01BL13 );

				    // When all the Mantis are destroyed it's
				    // 3 seconds until Halsey new orders...

				    timer = 3;

				    state = BlackwellContacts;
			    }
		    }
		    else if ( MScript::IsIdle( data.Corvette1 ) && MScript::IsIdle( data.Corvette2 ) )
		    {
                if ( !corvettesIdle )
                {
                    // the player's corvettes are idle, give him 30 seconds before bitching at him

                    corvettesIdle = true;

                    timer = 30;
                }
                else if ( timer <= 0 )
                {
			        if (!MScript::IsStreamPlaying(data.mhandle))
			        {
                        timer = 30;

				        // What are you wainting for? Help the Austin?

                        data.mhandle = MScript::PlayAnimatedMessage( "M01BL11.wav", "Animate!!Blackwell2", 
                            MagpieLeft, MagpieTop, data.Blackwell,IDS_TM1_SUB_M01BL11 );
                    }
                }
		    }
		    else
		    {
			    corvettesIdle = false;
		    }

		    break;

	    case BlackwellContacts:
            
		    if (!MScript::IsStreamPlaying(data.mhandle) && timer < 0)
		    {
			    // Halsey uplink.
			    state = Uplink;
			    timer = 2.5;
		    }

		    break;

	    case Uplink:

		    if (timer <0)
		    {
			    // Halsey uplink.

			    data.mhandle = MScript::PlayAudio( "high_level_data_uplink.wav" );
			    state = NewObjectives;
		    }

		    break;


	    case NewObjectives:

		    if ( !MScript::IsStreamPlaying( data.mhandle ) )
		    {
			    // Halsey beams in with new objectives..

			    //Get your Fab ready...

			    data.Fabricator = MScript::GetPartByName( "Fabricator" );

                data.mhandle = MScript::PlayAnimatedMessage( "M01HA14.wav", "Animate!!Halsey2", 
                    MagpieLeft, MagpieTop,IDS_TM1_SUB_M01HA14 );

                AddAndDisplayObjective( IDS_TM1_OBJECTIVE6, data.mhandle );

			    state = LookAtJumpPoint;
		    }

		    break;

	    case LookAtJumpPoint:

		    if ( !MScript::IsStreamPlaying( data.mhandle ) )
		    {
			    // Look at Fabricator jumping in...

                MScript::AlertMessage( MScript::GetPartByName( "Gate1" ), NULL );

			    // And jump it into Tau Ceti

                tempGroup += data.Fabricator;
                tempGroup += MScript::GetPartByName( "Alpha");
                tempGroup += MScript::GetPartByName( "Beta" );

	            MScript::OrderUseJumpgate( tempGroup, MScript::GetPartByName( "Gate0" ) );

			    state = Fabricator;
		    }

		    break;

	    case Fabricator:

		    if(data.Fabricator->systemID == TAU_CETI)
		    {
			    state = BlackwellBrief;
		    }

		    break;
		    
	    case BlackwellBrief:

		    // Ok, since this is your first time, I'll help you build a base...

		    if (!MScript::IsStreamPlaying(data.mhandle))
		    {
                data.mhandle = MScript::PlayAnimatedMessage( "M01BL15.wav", "Animate!!Blackwell2", 
                    MagpieLeft, MagpieTop, data.Blackwell,IDS_TM1_SUB_M01BL15 );

                AddAndDisplayObjective( IDS_TM1_OBJECTIVE2_4, data.mhandle, true );

			    state = Done;
		    }

		    break;

	    case Done:

		    //setup trigger "PlanetTrigger" to detect fabriator from player near planet.

		    MScript::EnableTrigger(data.PlanetTrigger, true);
		    MScript::SetTriggerFilter(data.PlanetTrigger, PLAYER_ID, TRIGGER_PLAYER, true);
		    MScript::SetTriggerFilter(data.PlanetTrigger, M_FABRICATOR, TRIGGER_MOBJCLASS, false);
		    MScript::SetTriggerRange(data.PlanetTrigger, 3);
		    MScript::SetTriggerProgram(data.PlanetTrigger, "TM1_BuildBase");

		    return false;
	}
	return true;
}



//----------------------------------------------------------------------------
// Mission 1 Part 2 -- Build a base
//----------------------------------------------------------------------------

//--------------------------------------------------------------------------//
//	Called by the trigger when the fab gets close to the planet

CQSCRIPTPROGRAM(TM1_BuildBase, TM1_BuildBase_Save,0);

void TM1_BuildBase::Initialize (U32 eventFlags, const MPartRef & part)
{
    MScript::MarkObjectiveCompleted( IDS_TM1_OBJECTIVE2_4 );

	//setup trigger to detect when HQ is started
	MScript::SetTriggerProgram(data.PlanetTrigger, "TM1_HQInProgress");

	// Set trigger to detect a HQ in process or otherwise from player 1.

	MScript::SetTriggerFilter(data.PlanetTrigger, M_HQ, TRIGGER_MOBJCLASS, false);
	MScript::SetTriggerFilter(data.PlanetTrigger, 0, TRIGGER_NOFORCEREADY, true);
	MScript::SetTriggerFilter(data.PlanetTrigger, PLAYER_ID, TRIGGER_PLAYER, true);

    MScript::EndStream( data.mhandle );

    data.mhandle = MScript::PlayAnimatedMessage( "M01BL17.wav", "Animate!!Blackwell2", 
        MagpieLeft, MagpieTop, data.Blackwell,IDS_TM1_SUB_M01BL17 );

    AddAndDisplayObjective( IDS_TM1_OBJECTIVE2_5, data.mhandle, true );
}

bool TM1_BuildBase::Update (void)
{
	if (data.mission_over)
	{
		// Check to see if mission has ended. If so don't bother anymore...

		return false;
	}

	return false;
}


//--------------------------------------------------------------------------//
//	Called by the trigger when the user starts building a HQ 

CQSCRIPTPROGRAM(TM1_HQInProgress, TM1_HQInProgress_Save,0);

void TM1_HQInProgress::Initialize (U32 eventFlags, const MPartRef & part)
{
	//setup trigger to detect when HQ is finished

	MScript::SetTriggerProgram(data.PlanetTrigger, "TM1_HQFinished");

	// detect a HQ finished player 1.

	MScript::SetTriggerFilter(data.PlanetTrigger, M_HQ, TRIGGER_MOBJCLASS, false);
	MScript::SetTriggerFilter(data.PlanetTrigger, 0, TRIGGER_FORCEREADY, true);
	MScript::SetTriggerFilter(data.PlanetTrigger, PLAYER_ID, TRIGGER_PLAYER, true);

	data.HQ = MScript::GetLastTriggerObject(part);

    MScript::EndStream( data.mhandle );

    data.mhandle = MScript::PlayAnimatedMessage( "M01BL17a.wav", "Animate!!Blackwell2", 
        MagpieLeft, MagpieTop, data.Blackwell,IDS_TM1_SUB_M01BL17A );

    attackLaunched = false;
}

bool TM1_HQInProgress::Update (void)
{
    MPartRef tempMantis[2];
    MGroupRef tempGroup;
    int tempCount;

	if (data.mission_over)
	{
		// Check to see if mission has ended. If so don't bother anymore...

		return false;
	}

    if ( !data.HQ.isValid() )
    {
        // construction has been canceled, abort this loop

        return false;
    }

    if ( !attackLaunched )
    {
        if ( data.HQ->hullPoints >= 1 * data.HQ->hullPointsMax / 3 )
        {
			tempMantis[0] = MScript::CreatePart("GBOAT!!M_Frigate", data.MantisGenWormhole, MANTIS_ID);
			tempMantis[1] = MScript::CreatePart("GBOAT!!M_Frigate", data.MantisGenWormhole, MANTIS_ID);
			data.mantis_ships += 2;

            for ( tempCount = 0; tempCount < 2; tempCount++ )
            {
                tempGroup += tempMantis[tempCount];
            }

			MScript::OrderAttack( tempGroup, data.HQ );  

            MScript::RunProgramByName( "TM1_DetectMantisDestroyed", MPartRef() );

            attackLaunched = true;
        }
    }

    if ( data.HQ->hullPoints >= 2 * data.HQ->hullPointsMax / 3 )
    {
	    if ( !MScript::IsStreamPlaying( data.mhandle ) )
	    {
            data.mhandle = MScript::PlayAnimatedMessage( "M01BL18.wav", "Animate!!Blackwell2", 
                MagpieLeft, MagpieTop, data.Blackwell,IDS_TM1_SUB_M01BL18 );

	        return false;
        }
    }

    return true;
}

//--------------------------------------------------------------------------//

CQSCRIPTPROGRAM( TM1_DetectMantisDestroyed, TM1_DetectMantisDestroyed_Save, 0 );

void TM1_DetectMantisDestroyed::Initialize 
(
    U32 eventFlags, 
    const MPartRef & part
)
{
}

bool TM1_DetectMantisDestroyed::Update (void)
{
	if (data.mission_over)
	{
		// Check to see if mission has ended. If so don't bother anymore...

		return false;
	}

	if ( !data.mantis_ships )
    {
	    if ( !MScript::IsStreamPlaying( data.mhandle ) )
	    {
            data.mhandle = MScript::PlayAnimatedMessage( "M01BL29B.wav", "Animate!!Blackwell2", 
                MagpieLeft, MagpieTop, data.Blackwell,IDS_TM1_SUB_M01BL29B );

            return false;
        }
    }

    return true;
}

//--------------------------------------------------------------------------//
//	Called by the trigger when the user finishes building a HQ 

CQSCRIPTPROGRAM(TM1_HQFinished, TM1_HQFinished_Save,0);

void TM1_HQFinished::Initialize (U32 eventFlags, const MPartRef & part)
{
    MScript::MarkObjectiveCompleted( IDS_TM1_OBJECTIVE2_5 );
	
	//Save off the HQ for later...

	data.HQ = MScript::GetLastTriggerObject(part);

	//setup trigger to detect when Refinery is started

	MScript::SetTriggerProgram(data.PlanetTrigger, "TM1_RefineryInProgress");

	MScript::SetTriggerFilter(data.PlanetTrigger, M_REFINERY, TRIGGER_MOBJCLASS, false);
	MScript::SetTriggerFilter(data.PlanetTrigger, 0, TRIGGER_NOFORCEREADY, true);
	MScript::SetTriggerFilter(data.PlanetTrigger, PLAYER_ID, TRIGGER_PLAYER, true);

	data.mission_tech.race[0].build = TECHTREE::BUILDNODE( data.mission_tech.race[0].build | 
        TECHTREE::TDEPEND_REFINERY );

    MScript::SetAvailiableTech( data.mission_tech );

}

bool TM1_HQFinished::Update (void)
{
	if (data.mission_over)
	{
		// Check to see if mission has ended. If so don't bother anymore...

		return false;
	}

	if ( !MScript::IsStreamPlaying( data.mhandle ) )
	{
        data.mhandle = MScript::PlayAnimatedMessage( "M01BL19.wav", "Animate!!Blackwell2", 
            MagpieLeft, MagpieTop, data.Blackwell,IDS_TM1_SUB_M01BL19 );

        AddAndDisplayObjective( IDS_TM1_OBJECTIVE2_6, data.mhandle, true );

        return false;
    }

	return true;
}

//--------------------------------------------------------------------------//
//	Called by the trigger when the user begins building a refinery

CQSCRIPTPROGRAM(TM1_RefineryInProgress, TM1_RefineryInProgress_Save,0);

void TM1_RefineryInProgress::Initialize (U32 eventFlags, const MPartRef & part)
{	
	//setup trigger to detect when Refinery is finished

	MScript::SetTriggerProgram(data.PlanetTrigger, "TM1_RefineryFinished");

	// detect a Refinery Finished from player 1.

	MScript::SetTriggerFilter(data.PlanetTrigger, M_REFINERY, TRIGGER_MOBJCLASS, false);
	MScript::SetTriggerFilter(data.PlanetTrigger, 0, TRIGGER_FORCEREADY, true);
	MScript::SetTriggerFilter(data.PlanetTrigger, PLAYER_ID, TRIGGER_PLAYER, true);

    MScript::EndStream( data.mhandle );

    data.mhandle = MScript::PlayAnimatedMessage( "M01BL20.wav", "Animate!!Blackwell2", 
        MagpieLeft, MagpieTop, data.Blackwell,IDS_TM1_SUB_M01BL20 );
}

bool TM1_RefineryInProgress::Update (void)
{
	if (data.mission_over)
	{
		// Check to see if mission has ended. If so don't bother anymore...

		return false;
	}

	return false;
}
//--------------------------------------------------------------------------//
//	Called by the trigger when the user finishes building a Refinery

CQSCRIPTPROGRAM(TM1_RefineryFinished, TM1_RefineryFinished_Save,0);

void TM1_RefineryFinished::Initialize (U32 eventFlags, const MPartRef & part)
{
    MScript::MarkObjectiveCompleted( IDS_TM1_OBJECTIVE2_6 );
    MScript::RemoveFromObjectiveList( IDS_TM1_OBJECTIVE2_4 );

    // setup trigger to detect when a marine training facility is started
	
	MScript::SetTriggerProgram(data.PlanetTrigger, "TM1_MarineTrainingInProgress");

	MScript::SetTriggerFilter(data.PlanetTrigger, M_OUTPOST, TRIGGER_MOBJCLASS, false);
	MScript::SetTriggerFilter(data.PlanetTrigger, 0, TRIGGER_NOFORCEREADY, true);
	MScript::SetTriggerFilter(data.PlanetTrigger, PLAYER_ID, TRIGGER_PLAYER, true);

	data.mission_tech.race[0].build = TECHTREE::BUILDNODE( data.mission_tech.race[0].build | 
        TECHTREE::TDEPEND_OUTPOST );

    MScript::SetAvailiableTech( data.mission_tech );

    MScript::EndStream( data.mhandle );

    data.mhandle = MScript::PlayAnimatedMessage( "M01BL20a.wav", "Animate!!Blackwell2", 
        MagpieLeft, MagpieTop, data.Blackwell,IDS_TM1_SUB_M01BL20A );

    AddAndDisplayObjective( IDS_TM1_OBJECTIVE2_7, data.mhandle, true );
}

bool TM1_RefineryFinished::Update (void)
{
	if (data.mission_over)
	{
		// Check to see if mission has ended. If so don't bother anymore...

		return false;
	}

	return false;
}

//--------------------------------------------------------------------------//
//	Called by the trigger when the user starts building a Marine Training Facility

CQSCRIPTPROGRAM( TM1_MarineTrainingInProgress, TM1_MarineTrainingInProgress_Save, 0 );

void TM1_MarineTrainingInProgress::Initialize 
(
    U32 eventFlags, 
    const MPartRef & part
)
{
	marineTrainingPart = MScript::GetLastTriggerObject(part);

    MScript::RemoveFromObjectiveList( IDS_TM1_OBJECTIVE2_5 );

	//setup trigger to detect when tanker is created

	MScript::SetTriggerProgram(data.PlanetTrigger, "TM1_TankerStarted");

	MScript::SetTriggerFilter(data.PlanetTrigger, M_HARVEST, TRIGGER_MOBJCLASS, false);
	MScript::SetTriggerFilter(data.PlanetTrigger, 0, TRIGGER_NOFORCEREADY, true);
	MScript::SetTriggerFilter(data.PlanetTrigger, PLAYER_ID, TRIGGER_PLAYER, true);

    MScript::EndStream( data.mhandle );

    data.mhandle = MScript::PlayAnimatedMessage( "M01BL21.wav", "Animate!!Blackwell2", 
        MagpieLeft, MagpieTop, data.Blackwell,IDS_TM1_SUB_M01BL21 );

    AddAndDisplayObjective( IDS_TM1_OBJECTIVE2_8, data.mhandle, true );
}

bool TM1_MarineTrainingInProgress::Update
( 
    void
)
{
    MPartRef tempMantis[2];
    MGroupRef tempGroup;
    int tempCount;

	if (data.mission_over)
	{
		// Check to see if mission has ended. If so don't bother anymore...

		return false;
	}

    if ( !marineTrainingPart.isValid() )
    {
        return false;
    }

    if ( marineTrainingPart->hullPoints >= 1 * marineTrainingPart->hullPointsMax / 2 )
    {
		tempMantis[0] = MScript::CreatePart("GBOAT!!M_Frigate", data.MantisGenWormhole, MANTIS_ID);
		tempMantis[1] = MScript::CreatePart("GBOAT!!M_Frigate", data.MantisGenWormhole, MANTIS_ID);
		data.mantis_ships += 2;

        for ( tempCount = 0; tempCount < 2; tempCount++ )
        {
            tempGroup += tempMantis[tempCount];
        }

		MScript::OrderAttack( tempGroup, marineTrainingPart );  

        return false;

    }

	return true;
}


//--------------------------------------------------------------------------//
//	Called by the trigger when the user starts building a tanker

CQSCRIPTPROGRAM(TM1_TankerStarted, TM1_TankerStarted_Save,0);

void TM1_TankerStarted::Initialize (U32 eventFlags, const MPartRef & part)
{
	//setup trigger to detect when tanker is finished

	MScript::SetTriggerProgram(data.PlanetTrigger, "TM1_TankerFinished");

	// detect a tanker finished from player 1.

	MScript::SetTriggerFilter(data.PlanetTrigger, M_HARVEST, TRIGGER_MOBJCLASS, false);
	MScript::SetTriggerFilter(data.PlanetTrigger, 0, TRIGGER_FORCEREADY, true);
	MScript::SetTriggerFilter(data.PlanetTrigger, PLAYER_ID, TRIGGER_PLAYER, true);

    MScript::EndStream( data.mhandle );

    data.mhandle = MScript::PlayAnimatedMessage( "M01BL22.wav", "Animate!!Blackwell2", 
        MagpieLeft, MagpieTop, data.Blackwell,IDS_TM1_SUB_M01BL22 );
}

bool TM1_TankerStarted::Update (void)
{
	if (data.mission_over)
	{
		// Check to see if mission has ended. If so don't bother anymore...

		return false;
	}

	return false;
}

CQSCRIPTPROGRAM(TM1_TankerFinished, TM1_TankerFinished_Save,0);

void TM1_TankerFinished::Initialize (U32 eventFlags, const MPartRef & part)
{
    MScript::MarkObjectiveCompleted( IDS_TM1_OBJECTIVE2_8 );
    MScript::RemoveFromObjectiveList( IDS_TM1_OBJECTIVE2_6 );

	MScript::EnableTrigger(part, false);

	data.Tanker = MScript::GetLastTriggerObject(part);

    MScript::EndStream( data.mhandle );

    data.mhandle = MScript::PlayAnimatedMessage( "M01BL23.wav", "Animate!!Blackwell2", 
        MagpieLeft, MagpieTop, data.Blackwell ,IDS_TM1_SUB_M01BL23);

    AddAndDisplayObjective( IDS_TM1_OBJECTIVE7, data.mhandle );
}

bool TM1_TankerFinished::Update (void)
{
	if (data.mission_over)
	{
		// Check to see if mission has ended. If so don't bother anymore...

		return false;
	}

	if (!MScript::IsIdle(data.Tanker) && !MScript::IsStreamPlaying(data.mhandle))
	{
        data.mhandle = MScript::PlayAnimatedMessage( "M01BL24.wav", "Animate!!Blackwell2", 
            MagpieLeft, MagpieTop, data.Blackwell,IDS_TM1_SUB_M01BL24 );

        MScript::RemoveFromObjectiveList( IDS_TM1_OBJECTIVE7 );

        AddAndDisplayObjective( IDS_TM1_OBJECTIVE2_9, data.mhandle, true );

    	MScript::AddToObjectiveList( IDS_TM1_OBJECTIVE7 );

		//setup trigger to detect when the lightshipyard is finished

		MScript::EnableTrigger( data.PlanetTrigger, true );
		MScript::SetTriggerProgram( data.PlanetTrigger, "TM1_LightShipYardInProgress" );

		// detect a Light Shipyard finished from player 1.

		MScript::SetTriggerFilter( data.PlanetTrigger, M_LIGHTIND, TRIGGER_MOBJCLASS, false );
		MScript::SetTriggerFilter( data.PlanetTrigger, 0, TRIGGER_NOFORCEREADY, true );
		MScript::SetTriggerFilter( data.PlanetTrigger, PLAYER_ID, TRIGGER_PLAYER, true );

	    data.mission_tech.race[0].build = TECHTREE::BUILDNODE( data.mission_tech.race[0].build | 
            TECHTREE::TDEPEND_LIGHT_IND );

        MScript::SetAvailiableTech( data.mission_tech );

		return false;
	}

	return true;
}

//--------------------------------------------------------------------------//
//	Called by the trigger when the user finishes building a Light Ship Yard

CQSCRIPTPROGRAM(TM1_LightShipYardInProgress, TM1_LightShipYardInProgress_Save,0);

void TM1_LightShipYardInProgress::Initialize (U32 eventFlags, const MPartRef & part)
{
	lightShipyardPart = MScript::GetLastTriggerObject(part);

	//setup trigger to detect when the lightshipyard is started

	MScript::SetTriggerProgram(data.PlanetTrigger, "TM1_LightShipYardFinished");
	MScript::SetTriggerFilter(data.PlanetTrigger, M_LIGHTIND, TRIGGER_MOBJCLASS, false);
	MScript::SetTriggerFilter(data.PlanetTrigger, 0, TRIGGER_FORCEREADY, true);
	MScript::SetTriggerFilter(data.PlanetTrigger, PLAYER_ID, TRIGGER_PLAYER, true);
	
    MScript::EndStream( data.mhandle );

    data.mhandle = MScript::PlayAnimatedMessage( "M01BL25.wav", "Animate!!Blackwell2", 
        MagpieLeft, MagpieTop, data.Blackwell,IDS_TM1_SUB_M01BL25 );
}

bool TM1_LightShipYardInProgress::Update (void)
{
    MPartRef tempMantis[2];
    MGroupRef tempGroup;
    int tempCount;

	if (data.mission_over)
	{
		// Check to see if mission has ended. If so don't bother anymore...

		return false;
	}

    if ( !lightShipyardPart.isValid() )
    {
        return false;
    }

    if ( lightShipyardPart->hullPoints >= 1 * lightShipyardPart->hullPointsMax / 2 )
    {
		tempMantis[0] = MScript::CreatePart("GBOAT!!M_Frigate", data.MantisGenWormhole, MANTIS_ID);
		tempMantis[1] = MScript::CreatePart("GBOAT!!M_Frigate", data.MantisGenWormhole, MANTIS_ID);
		data.mantis_ships += 2;

        for ( tempCount = 0; tempCount < 2; tempCount++ )
        {
            tempGroup += tempMantis[tempCount];
        }

		MScript::OrderAttack( tempGroup, lightShipyardPart );  

        return false;

    }

	return true;
}

//--------------------------------------------------------------------------//
//	Called by the trigger when the user finishes building a Light Ship Yard

CQSCRIPTPROGRAM(TM1_LightShipYardFinished, TM1_LightShipYardFinished_Save,0);

void TM1_LightShipYardFinished::Initialize (U32 eventFlags, const MPartRef & part)
{
    MScript::MarkObjectiveCompleted( IDS_TM1_OBJECTIVE2_9 );
    MScript::RemoveFromObjectiveList( IDS_TM1_OBJECTIVE2_7 );

	MScript::EnableTrigger( data.PlanetTrigger, false );
			
    MScript::EndStream( data.mhandle );

    data.mhandle = MScript::PlayAnimatedMessage( "M01BL26.wav", "Animate!!Blackwell2", 
        MagpieLeft, MagpieTop, data.Blackwell,IDS_TM1_SUB_M01BL26 );

    MScript::RemoveFromObjectiveList( IDS_TM1_OBJECTIVE7 );

    AddAndDisplayObjective( IDS_TM1_OBJECTIVE2_10, data.mhandle, true );

    MScript::AddToObjectiveList( IDS_TM1_OBJECTIVE7 );
}

bool TM1_LightShipYardFinished::Update (void)
{
	if (data.mission_over)
	{
		// Check to see if mission has ended. If so don't bother anymore...

		return false;
	}

	if(MScript::GetFreeCommandPoints(PLAYER_ID) <= 0)
	{
		if (!MScript::IsStreamPlaying(data.shandle) && !MScript::IsStreamPlaying(data.mhandle))
        {
            data.mhandle = MScript::PlayAnimatedMessage( "M01BL27.wav", "Animate!!Blackwell2", 
                MagpieLeft, MagpieTop, data.Blackwell,IDS_TM1_SUB_M01BL27 );

		    return false;
        }
	}
	return true;
}

//--------------------------------------------------------------------------//
//	Called by the event system when the user recovers something...

CQSCRIPTPROGRAM(TM1_ItemRecovered, TM1_ItemRecovered_Save, CQPROGFLAG_RECOVERY_PICKUP);

void TM1_ItemRecovered::Initialize (U32 eventFlags, const MPartRef & part)
{
    MPartRef tempTower;

    if ( data.mission_over )
    {
        return;
    }

	if ( data.Beacon.isValid() )
	{
		if ( part == data.Beacon && data.mission_state != BeaconRecovered )
		{
			// Beacon Recovered...

            tempTower = MScript::GetTowerID( data.Beacon );

            data.shandle = MScript::PlayAudio( "M01HV38.wav", tempTower , IDS_TM1_SUB_M01HV38);

            if ( data.mission_state != BeaconRecovered )
            {
                // mantis attack on harvester

                MassAttackInRange( M_SCOUTCARRIER, MANTIS_ID, tempTower, data.alkaidPrimeTrigger, 4 );
	            MassStanceInRange( M_SCOUTCARRIER, MANTIS_ID, US_DEFEND, data.alkaidPrimeTrigger, 4 );

                MassAttackInRange( M_FRIGATE, MANTIS_ID, tempTower, data.alkaidPrimeTrigger, 4 );
	            MassStanceInRange( M_FRIGATE, MANTIS_ID, US_DEFEND, data.alkaidPrimeTrigger, 4 );

			    data.mission_state = BeaconRecovered;
			    state = Beacon;
            }
            else
            {
                state = BOGUS;
            }

			return;
		}
	}
	state = BOGUS;
}


bool TM1_ItemRecovered::Update (void)
{
	if ( data.mission_over )
	{
		// Check to see if mission has ended. If so don't bother anymore...
		return false;
	}

	switch ( state )
	{
	    case Beacon:

		    if ( !MScript::IsStreamPlaying( data.shandle ) )
		    {
                MScript::MarkObjectiveCompleted( IDS_TM1_OBJECTIVE2_13 );

                MScript::EndStream( data.mhandle );

                data.mhandle = MScript::PlayAnimatedMessage( "M01BL36.wav", "Animate!!Blackwell2", 
                    MagpieLeft, MagpieTop, data.Blackwell,IDS_TM1_SUB_M01BL36 );

                AddAndDisplayObjective( IDS_TM1_OBJECTIVE2_14, data.mhandle, true );

			    return false;
		    }

		    break;
					    
	    case BOGUS:
		    return false;
	}

	return true;
}

//-----------------------------------------------------------------------
// Triggered once the player has built three corvettes...

CQSCRIPTPROGRAM(TM1_MantisAttack, TM1_MantisAttack_Save,0);

void TM1_MantisAttack::Initialize (U32 eventFlags, const MPartRef & part)
{

	int i;
    MGroupRef tempGroup;

	MPartRef Mantis, dest;

	dest = MScript::GetPartByName("TNSAustinDest");


	for (i = 0; i< SHIPS_IN_MANTIS_FLEET; i++)
	{
    	// Create five Mantis Frigates....

		Mantis = MScript::CreatePart("GBOAT!!M_Frigate", data.MantisGenWormhole, MANTIS_ID);

        tempGroup += Mantis;
	}

	// And send them into Tau Ceti...

	MScript::OrderMoveTo( tempGroup, dest); 

	data.mantis_ships = i;
	
	state = Begin;
	data.mission_state = TauCetiAttack;
}

bool TM1_MantisAttack::Update (void)
{
	if (data.mission_over)
	{
		// Check to see if mission has ended. If so don't bother anymore... Used for mission failed
		return false;
	}

	timer -= ELAPSED_TIME;

	switch (state)
	{
	case Begin:

		if (data.mantis_ships == 0)
		{
            MScript::MarkObjectiveCompleted( IDS_TM1_OBJECTIVE2_11 );

            // Tau Ceti is now secured

            MScript::MarkObjectiveCompleted( IDS_TM1_OBJECTIVE6 );

            MScript::RemoveFromObjectiveList( IDS_TM1_OBJECTIVE4 );

            MScript::RemoveFromObjectiveList( IDS_TM1_OBJECTIVE2_8 );
            MScript::RemoveFromObjectiveList( IDS_TM1_OBJECTIVE2_9 );
            MScript::RemoveFromObjectiveList( IDS_TM1_OBJECTIVE2_10 );
            MScript::RemoveFromObjectiveList( IDS_TM1_OBJECTIVE2_11 );

			state = HalseyWait;
		}

		break;

    case HalseyWait:

		if ( !MScript::IsStreamPlaying( data.mhandle ) )
        {
            timer = 3;		// 3 Seconds before Halsey beams in with new objectives

            state = HalseysFinalMission;
        }

        break;


	case HalseysFinalMission:

		if (timer <0)
		{
			// When all  Mantis are destroyed... Halsey beams in with new objectives... Again...

        	MScript::MarkObjectiveCompleted( IDS_TM1_OBJECTIVE6 );

            // delete secondary objectives regarding securing Tau Ceti

            MScript::RemoveFromObjectiveList( IDS_TM1_OBJECTIVE2_4 );
            MScript::RemoveFromObjectiveList( IDS_TM1_OBJECTIVE2_5 );

			data.shandle = MScript::PlayAudio( "high_level_data_uplink.wav" );
			state = HalseyBrief;
		}

		break;

	case HalseyBrief:

		if (!MScript::IsStreamPlaying(data.shandle))
		{
            MScript::EndStream( data.mhandle );

            data.mhandle = MScript::PlayAnimatedMessage( "M01HA30.wav", "Animate!!Halsey2", 
                MagpieLeft, MagpieTop,IDS_TM1_SUB_M01HA30 );

            AddAndDisplayObjective( IDS_TM1_OBJECTIVE9, data.mhandle );

			state = BlackwellSupply;
		}

		break;

	case BlackwellSupply:

		// Turn on the wormhole to Mantis space...

		if (!MScript::IsStreamPlaying(data.mhandle))
		{
            MScript::EndStream( data.mhandle );

            data.mhandle = MScript::PlayAnimatedMessage( "M01BL31.wav", "Animate!!Blackwell2", 
                MagpieLeft, MagpieTop, data.Blackwell,IDS_TM1_SUB_M01BL31 );

            AddAndDisplayObjective( IDS_TM1_OBJECTIVE2_12, data.mhandle, true );

			state = BlackwellHQExplanation;
		}

		break;
    case BlackwellHQExplanation:

		if ( !MScript::IsStreamPlaying( data.mhandle ) )
		{
            // missing sound file

#if 0
            data.mhandle = MScript::PlayAnimatedMessage( "M01BL80.wav", "Animate!!Blackwell2", 
                MagpieLeft, MagpieTop, data.Blackwell );
#endif

            state = ThroughWormhole;
        }

        break;

	case ThroughWormhole:

		if (!MScript::IsStreamPlaying(data.mhandle))
		{
			//Enbale jump points to and from Mantis space for player...

			MScript::EnableJumpgate(MScript::GetPartByName("Gate2"), true);
			MScript::EnableJumpgate(MScript::GetPartByName("Gate3"), true);
			MScript::EnableSystem(MANTIS_SPACE, true, true);

			// Now we have an strong recon group, let's jump into Mantis space...

            MScript::EndStream( data.mhandle );

            data.mhandle = MScript::PlayAnimatedMessage( "M01BL33.wav", "Animate!!Blackwell2", 
                MagpieLeft, MagpieTop, data.Blackwell,IDS_TM1_SUB_M01BL33 );

			MScript::RunProgramByName("TM1_BreakOnThrough", MPartRef ());

			return false;
		}

		break;
	}

	return true;
}

//-----------------------------------------------------------------------
// Triggered everytime an Enemy sighted alert is created...

CQSCRIPTPROGRAM(TM1_EnemySighted, TM1_EnemySighted_Save, CQPROGFLAG_ENEMYSIGHTED);

void TM1_EnemySighted::Initialize (U32 eventFlags, const MPartRef & part)
{
    if ( data.mission_over )
    {
        return;
    }

	switch (data.mission_state)
	{
	    case TauCetiAttack:

		    // When the Mantis launch an attcak against Tau Ceti - The first time we get an enemy sighted ALERT

		    // Blackwell -- It looks like the the aliens are launching a full scale attack..

            MScript::EndStream( data.mhandle );

            data.mhandle = MScript::PlayAnimatedMessage( "M01BL28.wav", "Animate!!Blackwell2", 
                MagpieLeft, MagpieTop, data.Blackwell, IDS_TM1_SUB_M01BL28 );

            AddAndDisplayObjective( IDS_TM1_OBJECTIVE2_11, data.mhandle, true );

		    data.mission_state = HarvesterAttack;

		    break;

	    case IntoMantisSpace:

		    data.mission_state = GetBeacon;

		    break;
	}
}

bool TM1_EnemySighted::Update (void)
{
	if (data.mission_over)
	{
		// Check to see if mission has ended. If so don't bother anymore...

		return false;
	}

	return false;
}


//-----------------------------------------------------------------------
// Triggered everytime an Enemy sighted alert is created...

CQSCRIPTPROGRAM(TM1_UnderAttack, TM1_UnderAttack_Save, CQPROGFLAG_UNDERATTACK);

void TM1_UnderAttack::Initialize (U32 eventFlags, const MPartRef & part)
{
    if ( data.mission_over )
    {
        return;
    }

	switch (data.mission_state)
	{
	    case TauCetiAttack:
		    // When the Mantis launch an attcak against Tau Ceti - The first time we get an enemy sighted ALERT

		    // Blackwell -- It looks like the the aliens are launching a full scale attack..

            MScript::EndStream( data.mhandle );

            data.mhandle = MScript::PlayAnimatedMessage( "M01BL28.wav", "Animate!!Blackwell2", 
                MagpieLeft, MagpieTop, data.Blackwell,IDS_TM1_SUB_M01BL28 );

		    data.mission_state = HarvesterAttack;

		    break;

	    case HarvesterAttack:

		    if (part->mObjClass == M_HARVEST)
		    {
				    // These aliens are smarter than they seem, they're going after our resources.

				    MScript::EndStream( data.mhandle );

                    data.mhandle = MScript::PlayAnimatedMessage( "M01BL29.wav", "Animate!!Blackwell2", 
                        MagpieLeft, MagpieTop, data.Blackwell,IDS_TM1_SUB_M01BL29 );

				    data.mission_state = DefendHarvester;

		    }

		    break;
	}
	
}

bool TM1_UnderAttack::Update (void)
{
	if (data.mission_over)
	{
		// Check to see if mission has ended. If so don't bother anymore...

		return false;
	}

	return false;
}

//-----------------------------------------------------------------------
// Triggered everytime an object is created, Used for mission unit tracking...

CQSCRIPTPROGRAM(TM1_ObjectBuilt, TM1_ObjectBuilt_Save, CQPROGFLAG_OBJECTCONSTRUCTED);

void TM1_ObjectBuilt::Initialize (U32 eventFlags, const MPartRef & part)
{
    if ( data.mission_over )
    {
        return;
    }

	switch (part->mObjClass)
	{
	    case M_CORVETTE:

		    // When a player builds a Terran Corvette

		    data.corvettes++;

		    if (data.corvettes >= 6 && data.mission_state == MantisInvasion)
		    {
			    MScript::RunProgramByName("TM1_MantisAttack", MPartRef ());
		    }

		    if (data.corvettes == 6)
		    {
			    // Six corvettes have been built

                MScript::MarkObjectiveCompleted( IDS_TM1_OBJECTIVE2_10 );
                MScript::RemoveFromObjectiveList( IDS_TM1_OBJECTIVE2_8 );
		    }

		    break;

	    case M_SCOUTCARRIER:
	    case M_FRIGATE:

		    // When the computer builds a Mantis attack ship...

		    data.mantis_ships++;

		    break;

        case M_OUTPOST:

            if ( MScript::IsObjectiveInList( IDS_TM1_OBJECTIVE2_7 ) )
            {
                MScript::MarkObjectiveCompleted( IDS_TM1_OBJECTIVE2_7 );
            }

            break;
	}
}

bool TM1_ObjectBuilt::Update (void)
{
	if (data.mission_over)
	{
		// Check to see if mission has ended. If so don't bother anymore...

		return false;
	}

	return false;
}

//-----------------------------------------------------------------------

CQSCRIPTPROGRAM(TM1_FlashBeacon, TM1_FlashBeacon_Save, 0);

void TM1_FlashBeacon::Initialize (U32 eventFlags, const MPartRef & part)
{
	data.bFlashBeacon = true;

    animID = MScript::StartAlertAnim( data.Beacon );
}

bool TM1_FlashBeacon::Update (void)
{
	if (data.mission_over)
	{
		// Check to see if mission has ended. If so don't bother anymore...

		return false;
	}

	if (data.bFlashBeacon)
	{
		return true;
	}
	else
    {
        MScript::StopAlertAnim( animID );

		return false;
    }
}

//-----------------------------------------------------------------------
// Triggered everytime an object is destroyed. Checks for mission critals objects and also
// keeps track of enemy ships...

CQSCRIPTPROGRAM(TM1_ShipDestroyed, TM1_ShipDestroyed_Save, CQPROGFLAG_OBJECTDESTROYED );

void TM1_ShipDestroyed::Initialize (U32 eventFlags, const MPartRef & part)
{
    int objectCount;

    if ( data.mission_over )
    {
        return;
    }

	//Envent handler for when ships / objects are destroyed for mission tracking purposes...

	switch (part->mObjClass)
	{
	    case M_SCOUTCARRIER:
	    case M_FRIGATE:

		    //If we kill a Mantis frigate decrement the counter...

		    data.mantis_ships--;

		    break;

	    case M_CORVETTE:

		    //Check for destruction of mission critical objects...

		    if (part == data.TNSAustin)
		    {
            	MScript::FlushStreams();

			    data.shandle = MScript::PlayAudio( "M01AU12.wav", data.TNSAustin , IDS_TM1_SUB_M01AU12);

				data.mission_state = MantisInvasion;
		    }
		    else if (part == data.Blackwell)
		    {
			    // If Blackwell is killed you lose...

        	    MScript::MoveCamera( part, 0, MOVIE_CAMERA_JUMP_TO);

			    MScript::RunProgramByName("TM1_BlackwellKilled", MPartRef ());
		    }
            else
            {
                data.corvettes--;
            }

		    break;

	    case M_HQ:

            objectCount = CountObjects( M_HQ, PLAYER_ID );

            if ( part->bReady ) 
            {
                objectCount--;
            }

		    if ( !CountObjects( M_FABRICATOR, PLAYER_ID ) && !objectCount )
		    {
			    // If HQ is destroyed and you don't have a fab you LOSE

        	    MScript::MoveCamera( part, 0, MOVIE_CAMERA_JUMP_TO);

			    MScript::RunProgramByName( "TM1_HQDestroyed", MPartRef () );
		    }

		    break;
	    
	    case M_FABRICATOR:

            objectCount = CountObjects( M_FABRICATOR, PLAYER_ID );

            if ( part->bReady ) 
            {
                objectCount--;
            }

		    if ( !CountObjects( M_HQ, PLAYER_ID ) && !objectCount )
		    {
			    // If fab is destroyed and you don't have a HQ you LOSE

        	    MScript::MoveCamera( part, 0, MOVIE_CAMERA_JUMP_TO);

			    MScript::RunProgramByName( "TM1_HQDestroyed", MPartRef () );
		    }

		    break;
	}
	
}

bool TM1_ShipDestroyed::Update (void)
{
    if (data.mission_over)
	{
		// Check to see if mission has ended. If so don't bother anymore...

		return false;
	}

	return false;
}

//---------------------------------------------------------------------------
//  The third and final part of the mission...
//  The player heads into Mantis Space...
//  Triggered once player has at least six corvettes in full supply...

CQSCRIPTPROGRAM(TM1_BreakOnThrough, TM1_BreakOnThrough_Save,0);

void TM1_BreakOnThrough::Initialize (U32 eventFlags, const MPartRef & part)
{
	//setup the trigger when player gets in sight of the beacon

	MPartRef trigger = MScript::CreatePart("MISSION!!TRIGGER",data.Beacon, 0);
	MScript::EnableTrigger(trigger, true);
	MScript::SetTriggerProgram(trigger, "TM1_SightedBeacon");
	MScript::SetTriggerRange(trigger, 2);
	MScript::SetTriggerFilter(trigger, PLAYER_ID, TRIGGER_PLAYER, true);

	state = Begin;
	data.mission_state = IntoMantisSpace;

	timer = 0;
}

bool TM1_BreakOnThrough::Update (void)
{
	MPartRef part;

	if (data.mission_over)
	{
		// Check to see if mission has ended. If so don't bother anymore...

		return false;
	}

	timer += ELAPSED_TIME;

	switch (state)
	{
	    case Begin:

		    part = PlayerInSystem(PLAYER_ID, MANTIS_SPACE);

		    if( part.isValid() )
		    {
                MScript::MarkObjectiveCompleted( IDS_TM1_OBJECTIVE2_12 );

                MScript::AlertMessage( part, NULL );

                MScript::EndStream( data.mhandle );

                data.mhandle = MScript::PlayAnimatedMessage( "M01BL34.wav", "Animate!!Blackwell2", 
                    MagpieLeft, MagpieTop, data.Blackwell,IDS_TM1_SUB_M01BL34 );

			    MScript::RunProgramByName("TM1_FlashBeacon", MPartRef ());

			    state = Scout;
		    }

		    break;

	    case Scout:
	    
		    // Trigger when player starts moving a couple of his ships to scout

		    if (!MScript::IsStreamPlaying(data.mhandle))
		    {
			    // Check to see if the player has any non idle ships in Mantis space...
		    
			    part = MScript::GetFirstPart();

			    while (part.isValid())
			    {
				    if(part->playerID == PLAYER_ID && part->systemID == MANTIS_SPACE)
				    {
					    if (!MScript::IsIdle(part))
					    {
						    // If so assume the player is scouting with them...

                            MScript::EndStream( data.mhandle );

                            data.mhandle = MScript::PlayAnimatedMessage( "M01BL35.wav", "Animate!!Blackwell2", 
                                MagpieLeft, MagpieTop, data.Blackwell,IDS_TM1_SUB_M01BL35 );

						    state  = RoadBlock;
						    return false;
					    }
				    }
				    part = MScript::GetNextPart(part);
			    }
		    }

		    break;

        default:

            break;
	}
	return true;

}

//--------------------------------------------------------------------------//
//	Called by the trigger when the player gets close to the beacon

CQSCRIPTPROGRAM(TM1_SightedBeacon, TM1_SightedBeacon_Save,0);

void TM1_SightedBeacon::Initialize (U32 eventFlags, const MPartRef & part)
{
	MScript::EnableTrigger(part, false);

	//turn off flashing
	data.bFlashBeacon = false;

	// Need That's the beacon! Let's grab it line for Blackwell....

    MScript::EndStream( data.mhandle );

    data.mhandle = MScript::PlayAnimatedMessage( "M01BL35a.wav", "Animate!!Blackwell2", 
        MagpieLeft, MagpieTop, data.Blackwell,IDS_TM1_SUB_M01BL35A );

    AddAndDisplayObjective( IDS_TM1_OBJECTIVE2_13, data.mhandle, true );
}

bool TM1_SightedBeacon::Update (void)
{
	if (data.mission_over)
	{
		// Check to see if mission has ended. If so don't bother anymore...

		return false;
	}

	return false;
}
//--------------------------------------------------------------------
// Ai for Mantis ships. Only triggered when player goes into Mantis space...

CQSCRIPTPROGRAM(TM1_MantisAI, TM1_MantisAI_Save,0);

void TM1_MantisAI::Initialize (U32 eventFlags, const MPartRef & part)
{
	MScript::ClearPath(MANTIS_SPACE, TAU_CETI, MANTIS_ID);

}

bool TM1_MantisAI::Update (void)
{
    MPartRef tempPart;

	if (data.mission_over)
	{
		// Check to see if mission has ended. If so don't bother anymore...
		return false;
	}

    if ( data.Beacon.isValid() )
    {
	    if ( data.Beacon->systemID == TAU_CETI )
	    {
		    if ( !MScript::IsStreamPlaying( data.shandle ) && 
                MScript::DistanceTo( MScript::GetPartByName("Tau Ceti Prime"), data.Beacon) <= 4 )
		    {
                MScript::MarkObjectiveCompleted( IDS_TM1_OBJECTIVE9 );

                MScript::RemoveFromObjectiveList( IDS_TM1_OBJECTIVE2_12 );
                MScript::RemoveFromObjectiveList( IDS_TM1_OBJECTIVE2_13 );
                MScript::RemoveFromObjectiveList( IDS_TM1_OBJECTIVE2_14 );

			    data.shandle = MScript::PlayAudio( "M01HV40.wav", MScript::GetTowerID( data.Beacon ),IDS_TM1_SUB_M01HV40 );

			    MScript::RemovePart(data.Beacon);

                // check if there are any mantis in Tau Ceti

		        tempPart = PlayerInSystem( MANTIS_ID, TAU_CETI );

                if ( !tempPart.isValid() )
                {
			        if (MScript::GetCurrentSystem() == data.Beacon->systemID)
			        {
				        // If in same system slide to to

				        MScript::MoveCamera( data.Beacon, 4, MOVIE_CAMERA_SLIDE_TO );
			        }
			        else
			        {
				        // Otherwise jump to the view

				        MScript::MoveCamera( data.Beacon, 0, MOVIE_CAMERA_JUMP_TO );
			        }

			        MScript::RunProgramByName("TM1_MissionSuccess", MPartRef ());

    			    return false;
                }
                else
                {
			        MScript::RunProgramByName("TM1_ClearSystemOfMantis", MPartRef ());
                }
		    }
        }
	}

    // no further AI processing if the game AI is handling it

    if ( data.gameAIEnabled )
    {
        return true;
    }

	//---------------------------
	//Combat AI...
	int attack_count = 0;


	MPartRef part;
	part = MScript::GetFirstPart();

	while ( part.isValid() )
	{
        if ( part->playerID == MANTIS_ID && MScript::IsGunboat( part ) && MScript::IsIdle( part ) )
		{
			// We've found an idle Mantis ship
			switch ( data.mission_state )
			{
			    case AttackOnTNSAustin:

				    if (part->systemID == TAU_CETI)
				    {
					    // Only for the ships that jumped into Tau Ceti...
					    if (data.TNSAustin.isValid())
					    {
						    MScript::OrderAttack(part, data.TNSAustin);
					    }
					    else
					    {
						    AttackNearestEnemy(part);
					    }
				    }

				    break;

			    case MantisInvasion:
			    case TauCetiAttack:
			    case HarvesterAttack:
			    case DefendHarvester:

				    if (part->systemID == TAU_CETI)
				    {
					    if (attack_count <3)
					    {
						    MScript::OrderAttack(part, FindFirstObjectOfType(M_HARVEST, PLAYER_ID, 0));
					    }
					    else if (attack_count <6)
					    {
						    AttackNearestEnemy(part);
					    }
					    else
					    {
						    MScript::OrderAttack(part, FindFirstObjectOfType(M_HQ, PLAYER_ID, 0));
					    }

					    attack_count++;
				    }
				    break;

                default:

                    break;
			}
				    
		}

		part = MScript::GetNextPart(part);
	}

	return true;
}


CQSCRIPTPROGRAM( TM1_ClearSystemOfMantis, TM1_ClearSystemOfMantis_Save, 0 );

void TM1_ClearSystemOfMantis::Initialize (U32 eventFlags, const MPartRef & part)
{
    state = STATE_BEGIN;
}

bool TM1_ClearSystemOfMantis::Update (void)
{
    MPartRef tempPart;

    switch ( state )
    {
        case STATE_BEGIN:

		    if ( !MScript::IsStreamPlaying( data.mhandle ) && 
                !MScript::IsStreamPlaying( data.mhandle ) )
            {
                data.mhandle = MScript::PlayAnimatedMessage( "M01BL41.wav", "Animate!!Blackwell2", 
                    MagpieLeft, MagpieTop, data.Blackwell,IDS_TM1_SUB_M01BL41 );

                AddAndDisplayObjective( IDS_TM1_OBJECTIVE10, data.mhandle );

                state = STATE_CHECK;
            }

            break;

        case STATE_CHECK:

	        tempPart = PlayerInSystem( MANTIS_ID, TAU_CETI );

            if ( !tempPart.isValid() )
            {
                MScript::MarkObjectiveCompleted( IDS_TM1_OBJECTIVE10 );

		        MScript::RunProgramByName("TM1_MissionSuccess", MPartRef ());

    	        return false;
            }

            break;

        default:

            break;
    }

    return true;
}
//---------------------------------------------------------------------------


CQSCRIPTPROGRAM(TM1_MissionSuccess, TM1_MissionSuccess_Save,0);

void TM1_MissionSuccess::Initialize (U32 eventFlags, const MPartRef & part)
{
	state = Begin;
	data.mission_over = true;

    // make sure the AI stops attacking

    MassEnableAI( MANTIS_ID, false );

    MassStance( 0, MANTIS_ID, US_STOP );

	MScript::EnableMovieMode( true );

	UnderMissionControl();
}

bool TM1_MissionSuccess::Update (void)
{
    MPartRef tempPart;

    tempPart = PlayerInSystem( MANTIS_ID, TAU_CETI );

    if ( tempPart.isValid() )
    {
        MScript::DestroyPart( tempPart );
    }

	switch (state)
	{
	    case Begin:

		    state = ClearSystem;

            // fall through

	    case ClearSystem:

		    if ( !MScript::IsStreamPlaying( data.mhandle ) &&
                !MScript::IsStreamPlaying( data.shandle ) )
		    {
			    data.shandle = MScript::PlayAudio( "high_level_data_uplink.wav" );
			    state = HalseyBrief;
		    }

		    break;

	    case HalseyBrief:

		    if ( !MScript::IsStreamPlaying( data.mhandle ) && 
                !MScript::IsStreamPlaying( data.shandle ) )
		    {
                data.mhandle = MScript::PlayAnimatedMessage( "M01HA42.wav", "Animate!!Halsey2", 
                    MagpieLeft, MagpieTop,IDS_TM1_SUB_M01HA42 );

			    state = PrintSuccess;
		    }

		    break;
	    
	    case PrintSuccess:

		    if ( !MScript::IsStreamPlaying( data.mhandle ) )
		    {
			    MScript::FlushTeletype();

                data.thandle = TeletypeMissionOver( IDS_TM1_TELETYPE_SUCCESS );

			    state = Done;
		    }

		    break;

	    case Done:

		    if ( !MScript::IsTeletypePlaying( data.thandle ) )
		    {
			    UnderPlayerControl();
			    MScript::EndMissionVictory( 0 );

			    return false;

		    }

            break;
	}

	return true;
}

//---------------------------------------------------------------------------
// Mission failure programs....

CQSCRIPTPROGRAM(TM1_BlackwellKilled, TM1_BlackwellKilled_Save,0);

void TM1_BlackwellKilled::Initialize (U32 eventFlags, const MPartRef & part)
{
	data.mission_over = true;

    UnderMissionControl();

	MScript::EnableMovieMode( true );

    MScript::EndStream( data.mhandle );

    data.mhandle = MScript::PlayAnimatedMessage( "M01BL43.wav", "Animate!!Blackwell2", 
        MagpieLeft, MagpieTop, data.Blackwell,IDS_TM1_SUB_M01BL43 );
}

bool TM1_BlackwellKilled::Update (void)
{
	if (!MScript::IsStreamPlaying(data.mhandle))
	{
        data.mhandle = MScript::PlayAnimatedMessage( "M01HA44.wav", "Animate!!Halsey2", 
            MagpieLeft, MagpieTop,IDS_TM1_SUB_M01HA44 );

		data.failureID = IDS_TM1_FAIL_BLACKWELL_LOST;
		MScript::RunProgramByName("TM1_MissionFailure", MPartRef ());
	
		return false;
	}

	return true;
}

//---------------------------------------------------------------------------
CQSCRIPTPROGRAM( TM1_HQDestroyed, TM1_HQDestroyed_Save,0 );

void TM1_HQDestroyed::Initialize (U32 eventFlags, const MPartRef & part)
{
	data.mission_over = true;

    UnderMissionControl();

	MScript::EnableMovieMode( true );

    MScript::EndStream( data.mhandle );

    data.mhandle = MScript::PlayAnimatedMessage( "M01HA45.wav", "Animate!!Halsey2", 
        MagpieLeft, MagpieTop,IDS_TM1_SUB_M01HA45 );

	MScript::RunProgramByName( "TM1_MissionFailure", MPartRef () );
}

bool TM1_HQDestroyed::Update (void)
{
	return false;
}

//---------------------------------------------------------------------------

CQSCRIPTPROGRAM(TM1_MissionFailure, TM1_MissionFailure_Save,0);

void TM1_MissionFailure::Initialize (U32 eventFlags, const MPartRef & part)
{
	data.mission_over = true;
	failureID = data.failureID;
}

bool TM1_MissionFailure::Update (void)
{
	switch ( state )
	{
	    case Begin:

		    if (!MScript::IsStreamPlaying(data.mhandle))
		    {
			    MScript::FlushTeletype();

                data.thandle = TeletypeMissionOver( IDS_TM1_TELETYPE_FAILURE , failureID);

			    state = Done;
		    }

		    break;

	    case Done:

		    if ( !MScript::IsTeletypePlaying( data.thandle ) )
		    {
			    UnderPlayerControl();
			    MScript::EndMissionDefeat();

			    return false;
		    }

            break;
	}

	return true;
}

//---------------------------------------------------------------------------
CQSCRIPTPROGRAM(TM1_ForbiddenJump, TM1_ForbiddenJump_Save, CQPROGFLAG_FORBIDEN_JUMP);

void TM1_ForbiddenJump::Initialize (U32 eventFlags, const MPartRef & part)
{
	MPartRef TerraJump = MScript::GetPartByName("Gate1");
	MPartRef TauCetiJump = MScript::GetPartByName("Gate2");

    if ( data.mission_over )
    {
        return;
    }

	if (part == TerraJump)
    {
		if (!MScript::IsStreamPlaying(data.mhandle))
		{
            data.mhandle = MScript::PlayAnimatedMessage( "M01BL08.wav", "Animate!!Blackwell2", 
                MagpieLeft, MagpieTop, data.Blackwell,IDS_TM1_SUB_M01BL08 );
        }
    }
	else if (part == TauCetiJump)
    {
		if (!MScript::IsStreamPlaying(data.mhandle))
		{
            data.mhandle = MScript::PlayAnimatedMessage( "M01BL16.wav", "Animate!!Blackwell2", 
                MagpieLeft, MagpieTop, data.Blackwell,IDS_TM1_SUB_M01BL16 );
        }
    }
}

bool TM1_ForbiddenJump::Update (void)
{
	if ( data.mission_over )
	{
		// Check to see if mission has ended. If so don't bother anymore...

		return false;
	}

	return false;
}

//---------------------------------------------------------------------------
// Program that runs in the background and checks whether certain
// key mission elements are visible to the player, so that appropriate
// responses may be generated when they are.

CQSCRIPTPROGRAM( TM1_VisibilityHandler, TM1_VisibilityHandlerData, 0 );

void TM1_VisibilityHandler::Initialize 
(
	U32 eventFlags, 
	const MPartRef &part
)
{
    mantisWormhole = MScript::GetPartByName( "Gate4" );
    alkaid2 = MScript::GetPartByName( "Alkaid 2" );
}

bool TM1_VisibilityHandler::Update 
(
	void
)
{
    if ( MScript::IsVisibleToPlayer( mantisWormhole, PLAYER_ID ) || MScript::IsVisibleToPlayer( alkaid2, PLAYER_ID ) )
    {
        // the player is going too far into Mantis space, start retaliating.

        TM1_TurnOnTheAI();

        return false;
    }

    return true;
}

//---------------------------------------------------------------------------

static void TM1_TurnOnTheAI
(
    void
)
{
	AIPersonality airules;

    data.gameAIEnabled = true;

	MScript::EnableEnemyAI(MANTIS_ID, true, "MANTIS_FRIGATE_RUSH");

	airules.difficulty = MEDIUM;
	airules.buildMask.bBuildPlatforms = true;
	airules.buildMask.bBuildLightGunboats = true;
	airules.buildMask.bBuildMediumGunboats = true;
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

    MassEnableAI( MANTIS_ID, true, TAU_CETI );
    MassEnableAI( MANTIS_ID, true, MANTIS_SPACE );
    MassEnableAI( MANTIS_ID, true, SYSTEM_ID_ALKAID_2 );
    MassEnableAI( MANTIS_ID, true, SYSTEM_ID_ALKAID_3 );
}

//---------------------------------------------------------------------------

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

    data.displayObjectiveHandlerParams_stringID = stringID;
    data.displayObjectiveHandlerParams_dependantHandle = dependantHandle;

    MScript::RunProgramByName( "TM1_DisplayObjectiveHandler", MPartRef ());
}

CQSCRIPTPROGRAM( TM1_DisplayObjectiveHandler, TM1_DisplayObjectiveHandlerData, 0 );

void TM1_DisplayObjectiveHandler::Initialize
(
	U32 eventFlags, 
	const MPartRef &part 
)
{
    stringID = data.displayObjectiveHandlerParams_stringID;
    dependantHandle = data.displayObjectiveHandlerParams_dependantHandle;
}

bool TM1_DisplayObjectiveHandler::Update
(
    void
)
{
    if ( data.mission_over )
    {
        // if the mission state no longer suits this program, halt execution of it

        return false;
    }

    if ( MScript::IsObjectiveCompleted( stringID ) || !MScript::IsObjectiveInList( stringID ) )
    {
        // if the objective has been completed, do not display it at all

        return false;
    }
    else if ( data.displayObjectiveHandlerParams_dependantHandle == 0 || 
        !MScript::IsStreamPlaying( dependantHandle ) )
    {
        // make sure that any previous objectives are finished playing

        if ( data.displayObjectiveHandlerParams_teletypeHandle == 0 ||
            !MScript::IsTeletypePlaying( data.displayObjectiveHandlerParams_teletypeHandle ) )
        {
		    data.displayObjectiveHandlerParams_teletypeHandle = TeletypeObjective( stringID );

            return false;
        }
    }

    return true;
}

