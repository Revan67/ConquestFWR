//--------------------------------------------------------------------------//
//                                                                          //
//                                Script14T.cpp                             //
//				/Conquest/App/Src/Scripts/Script02T/Script14T.cpp			//
//								MISSION PROGRAMS							//
//                                                                          //
//               COPYRIGHT (C) 2000 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
	Created:	6/19/00		JeffP
	Modified:	6/19/00		JeffP

*/
//--------------------------------------------------------------------------//


#include <ScriptDef.h>
#include <DLLScriptMain.h>
#include "DataDef.h"
#include "stdlib.h"
#include <stdio.h>

#include "..\helper\helper.h"

//--------------------------------------------------------------------------
//  DEFINES
//--------------------------------------------------------------------------

#define PLAYER_ID			1
#define MANTIS_ID			2
#define SOLARIAN_ID			3
#define REBEL_ID			4


#define ZAHL				1
#define BAAI				2
#define CALLE				3
#define SERPCE				4
#define QUASILI				5
#define EPSILON				6
#define REGULUS				7
#define DRACONIS			8
#define KABEL				9
#define OBRA				10
#define GELD				11
#define JAHR				12
#define ORDEN				13

// JUSNOTE: Get rid of these once old teletype is wiped out

//--------------------------------------------------------------------------//
// Local Functions
//--------------------------------------------------------------------------//

static void KeepShipInEpsilon
(
    const MPartRef &part
);

static bool SameObj
(
    const MPartRef & obj1, 
    const MPartRef & obj2
);

static void TurnOnAIForSystem
(
    U32 SystemID
);

static void MantisAttackSystem
(
    U32 SystemID
);

static void AddAndDisplayObjective
(
    U32 stringID,
    U32 dependantHandle,
    bool isSecondary = false
);

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//

//--------------------------------------------------------------------------

CQSCRIPTDATA(MissionData, data);

//--------------------------------------------------------------------------
//	MISSION BRIEFING
//--------------------------------------------------------------------------

CQSCRIPTPROGRAM(TM14_Briefing, TM14_Briefing_Data, CQPROGFLAG_STARTBRIEFING);

void TM14_Briefing::Initialize (U32 eventFlags, const MPartRef & part)
{
	U32 x;

    MScript::PlayMusic( NULL );

	data.mission_over = data.briefing_over = data.ai_enabled = data.next_state = false;
	data.bGotCocoon = data.bClearedBases = data.bMalkorRun = false;
	data.mhandle = 0;
	data.shandle = 0;
    data.thandle = 0;
	data.failureID = 0;
	data.textFailureID = 0;

	data.mission_state = Briefing;

	state = BRIEFING_STATE_BEGIN;

    data.displayObjectiveHandlerParams_teletypeHandle = 0;

	for ( x=0; x<NUM_SYSTEMS; x++ )
	{
		data.bAIOnInSystem[x] = false;
	}

	MScript::EnableBriefingControls( true );

}

bool TM14_Briefing::Update (void)
{
	if (data.briefing_over)
    {
		return false;
    }

	switch(state)
	{
		case BRIEFING_STATE_BEGIN:

			state = BRIEFING_STATE_TELETYPE;

			// fall through

		case BRIEFING_STATE_TELETYPE:

            MScript::FlushTeletype();

            data.thandle = TeletypeBriefing( IDS_TM14_TELETYPE_LOC );
			
			state = BRIEFING_STATE_TV1;

			// fall through

		case BRIEFING_STATE_TV1:

			data.mhandle = MScript::PlayAudio( "prolog14.wav" );
			MScript::BriefingSubtitle(data.mhandle,IDS_TM14_SUB_PROLOG14);

            ShowBriefingAnimation( 0, "TNRLogo", 100, true, true );
            ShowBriefingAnimation( 3, "TNRLogo", 100, true, true );
            ShowBriefingAnimation( 1, "Radiowave", ANIMSPD_RADIOWAVE, true, true );
            ShowBriefingAnimation( 2, "Radiowave", ANIMSPD_RADIOWAVE, true, true );

			state = BRIEFING_STATE_UPLINK;

			break;

		case BRIEFING_STATE_UPLINK:

            if ( !MScript::IsStreamPlaying( data.mhandle ) )
			{
                MScript::FreeBriefingSlot( -1 );

				ShowBriefingAnimation( 0, "Xmission", ANIMSPD_XMISSION, false, false ); 

                data.mhandle = MScript::PlayAudio( "high_level_data_uplink.wav" );

				state = BRIEFING_STATE_HALSEY_1;
			}

			break;

		case BRIEFING_STATE_HALSEY_1:

            if ( !MScript::IsStreamPlaying( data.mhandle ) )
			{
                // Halsey starts it off

                data.mhandle = DoBriefingSpeechMagpieDirect(IDS_TM14_SUB_M14HA02, "m14ha02.wav", 0, CHAR_HALSEY, true );

                state = BRIEFING_STATE_OBJECTIVES;
            }

            break;

        case BRIEFING_STATE_OBJECTIVES:

            if ( !MScript::IsStreamPlaying( data.mhandle ) )
			{
                MScript::FreeBriefingSlot( -1 );

                data.thandle = TeletypeBriefing( IDS_TM14_OBJECTIVES, true );
			
				state = BRIEFING_STATE_FINISHED;
			}

            break;

        default:

            break;	
	}

	return true;
}

//--------------------------------------------------------------------------

CQSCRIPTPROGRAM(TM14_MissionStart, TM14_MissionStart_Data,CQPROGFLAG_STARTMISSION);

void TM14_MissionStart::Initialize (U32 eventFlags, const MPartRef & part)
{
	data.mission_state = MissionStart;

    // dereference mission critical objects for later use

	data.Benson = MScript::GetPartByName( "TNS Benson" );
	data.Steele = MScript::GetPartByName( "TNS Steele" );
	data.Takei = MScript::GetPartByName( "TNS Takei" );
	data.Halsey = MScript::GetPartByName( "TNS Halsey" );
	data.Vivac = MScript::GetPartByName( "TNS Vivac" );
    data.elite1 = MScript::GetPartByName( "Elite Battleship" );
    data.elite2 = MScript::GetPartByName( "Elite Battleship 2" );
	data.Malkor = MScript::GetPartByName( "Malkor" );
	data.MalkorCocoon = MScript::GetPartByName( "Malkor's Cocoon" );
	data.gateToQuasili = MScript::GetPartByName( "Gate11" );
	data.gateToJahr = MScript::GetPartByName( "Gate14" );

    MScript::EnablePartnameDisplay( data.Malkor, true);

	MScript::EnableAIForPart( data.Malkor, false );
    MScript::MakeInvincible( data.Malkor, true ); 

    MScript::MakeNonAutoTarget( data.MalkorCocoon, true );

    data.halseyStartPoint = MScript::CreatePart( "MISSION!!WAYPOINT", data.Halsey, 0 );

	MScript::EnableJumpCap( data.Halsey, false );
	MScript::EnableJumpCap( data.elite1, false );
	MScript::EnableJumpCap( data.elite2, false );	

	TECHNODE missionTech = MScript::GetPlayerTech( PLAYER_ID );

    // XOR out the admirals as they should never appear in a single player mission

	missionTech.race[0].common_extra = missionTech.race[1].common_extra = missionTech.race[2].common_extra = 
        (TECHTREE::COMMON_EXTRA)(
        TECHTREE::ALL_COMMONEXTRAUPGRADES ^
        TECHTREE::RES_ADMIRAL1 ^
        TECHTREE::RES_ADMIRAL2 ^
        TECHTREE::RES_ADMIRAL3 ^
        TECHTREE::RES_ADMIRAL4 ^
        TECHTREE::RES_ADMIRAL5 ^
        TECHTREE::RES_ADMIRAL6 );

	missionTech.race[0].common = missionTech.race[1].common = missionTech.race[2].common = 
        (TECHTREE::COMMON) (TECHTREE::ALL_COMMON ^
        TECHTREE::RES_SHIELDS5 ^
        TECHTREE::RES_ENGINE5 ^
        TECHTREE::RES_WEAPONS5 );

	missionTech.race[0].build = 
        (TECHTREE::BUILDNODE) (TECHTREE::ALL_BUILDNODE ^ 
        TECHTREE::TDEPEND_ION_CANNON );

    missionTech.race[1].build = 
        (TECHTREE::BUILDNODE) (TECHTREE::ALL_BUILDNODE );

	missionTech.race[2].build = 
        (TECHTREE::BUILDNODE) ( ( TECHTREE::ALL_BUILDNODE ^ 
        TECHTREE::SDEPEND_PORTAL ) | 
        TECH_TREE_RACE_BITS_ALL );

	missionTech.race[0].tech = missionTech.race[1].tech = missionTech.race[2].tech = 
        (TECHTREE::TECHUPGRADE) (TECHTREE::ALL_TECHUPGRADE );

    MScript::SetAvailiableTech( missionTech );

    MScript::SetMaxCommandPoitns( PLAYER_ID, 100 );
    MScript::SetMaxCommandPoitns( MANTIS_ID, 100 );

	MScript::SetAllies(PLAYER_ID, REBEL_ID, true);
	MScript::SetAllies(REBEL_ID, PLAYER_ID, true);

    MScript::SetMissionID( 13 );
	MScript::SetMissionName(IDS_TM14_MISSION_NAME);
	MScript::SetMissionDescription(IDS_TM14_MISSION_DESC);

    MScript::SetEnemyCharacter( MANTIS_ID, IDS_TM14_MANTIS_PLAYER_NAME );

    MScript::EnableRegenMode( true );

	MScript::AddToObjectiveList(IDS_TM14_OBJECTIVE2);
	MScript::AddToObjectiveList(IDS_TM14_OBJECTIVE3);

	for (int x=1; x<= NUM_SYSTEMS; x++)
		MScript::EnableSystem(x, true, false);

	MScript::EnableSystem( QUASILI, true, true );
	MScript::EnableSystem( JAHR, true, true );
	MScript::EnableSystem( EPSILON, true, true );

	MScript::ClearPath( QUASILI, JAHR, PLAYER_ID );
	MScript::ClearPath( JAHR, EPSILON, PLAYER_ID );
	MScript::ClearPath( QUASILI, EPSILON, PLAYER_ID );

	MScript::PlayMusic( "battle.wav" );
	
	MScript::ChangeCamera(MScript::GetPartByName("MissionCamStart"), 0, MOVIE_CAMERA_JUMP_TO);

	MScript::RunProgramByName("TM14_AIProgram", MPartRef() );
	MScript::RunProgramByName( "TM14_FindMalkor", MPartRef() );
    MScript::RunProgramByName( "TM14_HerdHalsey", MPartRef() );

	state = PROGRAM_STATE_BEGIN;
}

bool TM14_MissionStart::Update (void)
{
	if ( data.mission_over )
    {
		return false;
    }

	switch ( state )
	{
        case PROGRAM_STATE_BEGIN:

            state = PROGRAM_STATE_STEELE_1;

            // fall through

        case PROGRAM_STATE_STEELE_1:

            if ( !MScript::IsStreamPlaying( data.mhandle ) )
			{
                data.mhandle = MScript::PlayAnimatedMessage( "M14ST03.wav", "Animate!!Steele2", 
                    MAIN_MAGPIE_X_POS, MAIN_MAGPIE_Y_POS, data.Steele, IDS_TM14_SUB_M14ST03 );

                state = PROGRAM_STATE_BENSON_1;
            }

            break;

        case PROGRAM_STATE_BENSON_1:

            if ( !MScript::IsStreamPlaying( data.mhandle ) )
			{
                data.mhandle = MScript::PlayAnimatedMessage( "M14BN04.wav", "Animate!!Benson2", 
                    MAIN_MAGPIE_X_POS, MAIN_MAGPIE_Y_POS, data.Benson, IDS_TM14_SUB_M14BN04);

                state = PROGRAM_STATE_HALSEY_1;
            }

            break;

        case PROGRAM_STATE_HALSEY_1:

            if ( !MScript::IsStreamPlaying( data.mhandle ) )
			{
                data.mhandle = MScript::PlayAnimatedMessage( "M14HA05.wav", "Animate!!Halsey2", 
                    MAIN_MAGPIE_X_POS, MAIN_MAGPIE_Y_POS, data.Halsey, IDS_TM14_SUB_M14HA05 );

                state = PROGRAM_STATE_VIVAC_1;
            }

            break;

        case PROGRAM_STATE_VIVAC_1:

            if ( !MScript::IsStreamPlaying( data.mhandle ) )
			{
                data.mhandle = MScript::PlayAnimatedMessage( "M14VV05A.wav", "Animate!!Vivac2", 
                    MAIN_MAGPIE_X_POS, MAIN_MAGPIE_Y_POS, data.Vivac, IDS_TM14_SUB_M14VV05A );

                state = PROGRAM_STATE_BENSON_2;
            }

            break;

        case PROGRAM_STATE_BENSON_2:

            if ( !MScript::IsStreamPlaying( data.mhandle ) )
			{
                data.mhandle = MScript::PlayAnimatedMessage( "M14BN06.wav", "Animate!!Benson2", 
                    MAIN_MAGPIE_X_POS, MAIN_MAGPIE_Y_POS, data.Benson, IDS_TM14_SUB_M14BN06 );

                state = PROGRAM_STATE_TAKEI_1;
            }

            break;

        case PROGRAM_STATE_TAKEI_1:

            if ( !MScript::IsStreamPlaying( data.mhandle ) )
			{
                data.mhandle = MScript::PlayAnimatedMessage( "M14TK06A.wav", "Animate!!Takei2", 
                    MAIN_MAGPIE_X_POS, MAIN_MAGPIE_Y_POS, data.Takei, IDS_TM14_SUB_M14TK06A );

                state = PROGRAM_STATE_HALSEY_2;
            }

            break;

        case PROGRAM_STATE_HALSEY_2:

            if ( !MScript::IsStreamPlaying( data.mhandle ) )
			{
                data.mhandle = MScript::PlayAnimatedMessage( "M14HA07.wav", "Animate!!Halsey2", 
                    MAIN_MAGPIE_X_POS, MAIN_MAGPIE_Y_POS, data.Halsey, IDS_TM14_SUB_M14HA07 );

                state = PROGRAM_STATE_TAKEI_2;
            }

            break;

        case PROGRAM_STATE_TAKEI_2:

            if ( !MScript::IsStreamPlaying( data.mhandle ) )
			{
                data.mhandle = MScript::PlayAnimatedMessage( "M14TK08.wav", "Animate!!Takei2", 
                    MAIN_MAGPIE_X_POS, MAIN_MAGPIE_Y_POS, data.Takei, IDS_TM14_SUB_M14TK08 );

                state = PROGRAM_STATE_HALSEY_3;
            }

            break;

        case PROGRAM_STATE_HALSEY_3:

            if ( !MScript::IsStreamPlaying( data.mhandle ) )
			{
                data.mhandle = MScript::PlayAnimatedMessage( "M14HA09.wav", "Animate!!Halsey2", 
                    MAIN_MAGPIE_X_POS, MAIN_MAGPIE_Y_POS, data.Halsey, IDS_TM14_SUB_M14HA09 );

                AddAndDisplayObjective( IDS_TM14_OBJECTIVE1, data.mhandle );
				state = PROGRAM_STATE_DONE;
//                state = PROGRAM_STATE_GROUP_HUG;
            }

            break;

        case PROGRAM_STATE_GROUP_HUG:

//            if ( !MScript::IsStreamPlaying( data.mhandle ) )
//			{
//			    data.mhandle = MScript::PlayAudio( "m14svb10.wav", IDS_TM14_SUB_M14TSVB10 );
//               AddAndDisplayObjective( IDS_TM14_OBJECTIVE1, data.mhandle );
//
//                state = PROGRAM_STATE_DONE;
//
//                // fall through
//            }
//            else
//            {
                break;
//            }

        case PROGRAM_STATE_DONE:

            return false;

            break;

        default:

            break;
	}

    // JUSNOTE Add and Display Objective 1 before this is over.

	return true;
}

//--------------------------------------------------------------------------

CQSCRIPTPROGRAM(TM14_FindMalkor, TM14_FindMalkor_Data, 0);

void TM14_FindMalkor::Initialize (U32 eventFlags, const MPartRef & part)
{
	data.mission_state = FindMalkor;
	state = PROG_STATE_BEGIN;
}

bool TM14_FindMalkor::Update (void)
{
	MPartRef part;

	if ( data.mission_over )
    {
		return false;
    }

	if ( !data.bClearedBases )
	{
		if ( !MScript::PlayerHasPlatforms( MANTIS_ID ) )
		{
			data.bClearedBases = true;

			MScript::MarkObjectiveCompleted(IDS_TM14_OBJECTIVE2);

			if (data.bGotCocoon)
			{
				MScript::RunProgramByName( "TM14_MissionSuccess", MPartRef () );

				return false;
			}
		}
	}

	if (!data.bGotCocoon)
	{
		if ( CountObjects( M_COCOON, PLAYER_ID, DRACONIS ) > 0 )
		{
            if ( !MScript::IsStreamPlaying( data.mhandle ) && state == PROG_STATE_DONE )
			{
                data.mhandle = MScript::PlayAnimatedMessage( "M14TK17.wav", "Animate!!Takei2", 
                    MAIN_MAGPIE_X_POS, MAIN_MAGPIE_Y_POS, data.Takei, IDS_TM14_SUB_M14TK17 );

			    data.bGotCocoon = true;

			    MScript::MarkObjectiveCompleted( IDS_TM14_OBJECTIVE1 );

			    if ( data.bClearedBases )
			    {
            	    MScript::MoveCamera( part, 0, MOVIE_CAMERA_JUMP_TO);

				    MScript::RunProgramByName( "TM14_MissionSuccess", MPartRef() );

				    return false;
			    }
            }
		}
	}


	switch (state)
	{
        case PROG_STATE_BEGIN:

			if ( MScript::IsVisibleToPlayer( data.MalkorCocoon, PLAYER_ID ) )
            {
                MScript::AlertMessage( data.MalkorCocoon, NULL );

				state = PROG_STATE_MALKOR_1;
            }

            break;

        case PROG_STATE_MALKOR_1:

            if ( !MScript::IsStreamPlaying( data.mhandle ) )
			{
                data.mhandle = MScript::PlayAnimatedMessage( "M14MA12.wav", "Animate!!Malkor2", 
                    MAIN_MAGPIE_X_POS, MAIN_MAGPIE_Y_POS, data.Malkor, IDS_TM14_SUB_M14MA12 );

                state = PROG_STATE_BENSON_1;
            }

            break;

        case PROG_STATE_BENSON_1:

            if ( !MScript::IsStreamPlaying( data.mhandle ) )
			{
                data.mhandle = MScript::PlayAnimatedMessage( "M14BN13.wav", "Animate!!Benson2", 
                    MAIN_MAGPIE_X_POS, MAIN_MAGPIE_Y_POS, data.Benson, IDS_TM14_SUB_M14BN13 );

                state = PROG_STATE_MALKOR_2;
            }

            break;

        case PROG_STATE_MALKOR_2:

            if ( !MScript::IsStreamPlaying( data.mhandle ) )
			{
                data.mhandle = MScript::PlayAnimatedMessage( "M14MA14.wav", "Animate!!Malkor2", 
                    MAIN_MAGPIE_X_POS, MAIN_MAGPIE_Y_POS, data.Malkor, IDS_TM14_SUB_M14MA14 );

                state = PROG_STATE_TAKEI_1;
            }

            break;

        case PROG_STATE_TAKEI_1:

            if ( !MScript::IsStreamPlaying( data.mhandle ) )
			{
                data.mhandle = MScript::PlayAnimatedMessage( "M14TK16.wav", "Animate!!Takei2", 
                    MAIN_MAGPIE_X_POS, MAIN_MAGPIE_Y_POS, data.Takei, IDS_TM14_SUB_M14TK16 );

                state = PROG_STATE_STEELE_1;
            }

            break;

        case PROG_STATE_STEELE_1:

            if ( !MScript::IsStreamPlaying( data.mhandle ) )
			{
                data.mhandle = MScript::PlayAnimatedMessage( "M14ST16a.wav", "Animate!!Steele2", 
                    MAIN_MAGPIE_X_POS, MAIN_MAGPIE_Y_POS, data.Steele, IDS_TM14_SUB_M14ST16A );

                state = PROG_STATE_DONE;
            }

            break;

        default:

            break;
	}


	return true;
}

//-----------------------------------------------------------------------
// Program to track if Halsey and his battle ships have left Epsilon and to return them if they have

CQSCRIPTPROGRAM( TM14_HerdHalsey, TM14_HerdHalsey_Data, 0);

void TM14_HerdHalsey::Initialize
(
    U32 eventFlags, 
    const MPartRef & part
)
{
}

bool TM14_HerdHalsey::Update 
(
    void
)
{
	if (data.mission_over)
    {
		return false;
    }

    KeepShipInEpsilon( data.Halsey );
    KeepShipInEpsilon( data.elite1 );
    KeepShipInEpsilon( data.elite2 );

    return true;
}

static void KeepShipInEpsilon
(
    const MPartRef &part
)
{
    if ( !part.isValid() )
    {
        return;
    }

    if ( part->systemID == EPSILON )
    {
        if ( part->bUnselectable )
        {
            // ship is back in Epsilon, but is still under computer control
            // return control and tell it to stay put.

			MScript::EnableSelection( part, true );

            MScript::OrderCancel( part );
        }
    }
    else
    {
        if ( !part->bUnselectable )
        {
            // Naughty naughty, Halsey and friends aren't supposed to be here

            MScript::OrderMoveTo( part, data.halseyStartPoint );

			MScript::EnableSelection( part, false );

            // bitch at the player

            if ( !MScript::IsStreamPlaying( data.mhandle ) )
            {
                data.mhandle = MScript::PlayAnimatedMessage( "M14HA11.wav", "Animate!!Halsey2", 
                    MAIN_MAGPIE_X_POS, MAIN_MAGPIE_Y_POS, IDS_TM14_SUB_M14HA11 );
            }
        }
    }
}

//--------------------------------------------------------------------------//

CQSCRIPTPROGRAM(TM14_KillMantisPlats, TM14_KillMantisPlats_Data, 0);

void TM14_KillMantisPlats::Initialize (U32 eventFlags, const MPartRef & part)
{
	MPartRef plat, next;

	plat = MScript::GetFirstPart();
	while (plat.isValid())
	{
		next = MScript::GetNextPart(plat);
		if (MScript::IsPlatform(plat) && plat->playerID == MANTIS_ID)
			MScript::DestroyPart(plat);
		plat = next;
	}
}

bool TM14_KillMantisPlats::Update (void)
{
	return false;
}

//--------------------------------------------------------------------------//

CQSCRIPTPROGRAM(TM14_AIProgram, TM14_AIProgram_Data, 0);

void TM14_AIProgram::Initialize (U32 eventFlags, const MPartRef & part)
{
	
	M_ EnableEnemyAI( MANTIS_ID, true, "MANTIS_FORTRESS" );

	AIPersonality airules;
	airules.difficulty = CAKEWALK;
	airules.buildMask.bBuildPlatforms = true;
	airules.buildMask.bBuildLightGunboats = true;
	airules.buildMask.bBuildMediumGunboats = true;
	airules.buildMask.bBuildHeavyGunboats = true;
	airules.buildMask.bHarvest = true;
	airules.buildMask.bScout = true;
	airules.buildMask.bBuildAdmirals = false;
	airules.buildMask.bLaunchOffensives = true;
	airules.nGunboatsPerSupplyShip = 10;
	airules.nNumMinelayers = 2;
	airules.uNumScouts = 3;
	airules.nNumFabricators = 5;
	airules.fHarvestersPerRefinery = 2;
	airules.nMaxHarvesters = 8;
	airules.nPanicThreshold = 1000;

	M_ SetEnemyAIRules(MANTIS_ID, airules);

	MassEnableAI( MANTIS_ID, false, 0);

    TurnOnAIForSystem( ORDEN );
    TurnOnAIForSystem( SERPCE );
    TurnOnAIForSystem( OBRA );
}

bool TM14_AIProgram::Update (void)
{
	if ( data.mission_over )
    {
		return false;
    }

	// KEEP MALKOR ALIVE, AND MAKE HIM FLEE AND DISAPPEAR WHEN 1/4 HULL

	if ( data.Malkor.isValid() && !data.bMalkorRun )
	{
		U32 hull, hullmax;

		hull = data.Malkor->hullPoints;
		hullmax = data.Malkor->hullPointsMax;

		if ( hull < hullmax / 4 ) 
		{
    		MScript::RunProgramByName( "TM14_RunCowardRun", MPartRef() );

			data.bMalkorRun = true;
		}
	}

	if( PlayerInSystem( PLAYER_ID, SERPCE ) )
    {
        TurnOnAIForSystem( SERPCE );
		TurnOnAIForSystem( CALLE );
    }

	if( PlayerInSystem( PLAYER_ID, JAHR ) )
    {
		TurnOnAIForSystem(JAHR);
		TurnOnAIForSystem(ORDEN);
    }

 
	if( PlayerInSystem( PLAYER_ID, ORDEN ) )
    {
		TurnOnAIForSystem( ORDEN );
		TurnOnAIForSystem( OBRA );
		TurnOnAIForSystem( GELD );
    }

	if( PlayerInSystem( PLAYER_ID, OBRA ) )
    {
		TurnOnAIForSystem(OBRA);
		TurnOnAIForSystem(ORDEN);
		TurnOnAIForSystem(GELD);
    }

	if( PlayerInSystem( PLAYER_ID, GELD ) )
    {
		TurnOnAIForSystem(GELD);
		TurnOnAIForSystem(ORDEN);
		TurnOnAIForSystem(OBRA);
		TurnOnAIForSystem(KABEL);
    }

	if( PlayerInSystem( PLAYER_ID, CALLE ) )
    {
		TurnOnAIForSystem(CALLE);
		TurnOnAIForSystem(ZAHL);
		TurnOnAIForSystem(BAAI);
    }

	if( PlayerInSystem( PLAYER_ID, ZAHL ) )
    {
		TurnOnAIForSystem(ZAHL);
		TurnOnAIForSystem(CALLE);
		TurnOnAIForSystem(BAAI);
    }

	if( PlayerInSystem( PLAYER_ID, BAAI ) )
    {
		TurnOnAIForSystem(BAAI);
		TurnOnAIForSystem(CALLE);
		TurnOnAIForSystem(REGULUS);
		TurnOnAIForSystem(ZAHL);
    }

	if( PlayerInSystem( PLAYER_ID, REGULUS ) )
    {
		TurnOnAIForSystem(REGULUS);
		TurnOnAIForSystem(BAAI);
		TurnOnAIForSystem(KABEL);
		TurnOnAIForSystem(DRACONIS);
    }

	if( PlayerInSystem( PLAYER_ID, KABEL ) )
    {
		TurnOnAIForSystem(KABEL);
		TurnOnAIForSystem(GELD);
		TurnOnAIForSystem(DRACONIS);
    }

	return true;
}

//-----------------------------------------------------------------------
// Program to handle Malkor chickening out and buggering off

CQSCRIPTPROGRAM( TM14_RunCowardRun, TM14_RunCowardRun_Data, 0 );

void TM14_RunCowardRun::Initialize 
(
    U32 eventFlags, 
    const MPartRef & part
)
{
    // create the wormhole through which Malkor will escape

    malkorWormhole = MScript::CreateWormBlast( data.Malkor, 1, MANTIS_ID, false );

    // make sure Malkor doesn't leave the vicinity of the wormhole

    MScript::OrderCancel( data.Malkor );
    MScript::SetStance( data.Malkor, US_STOP );
    MScript::EnableMoveCap( data.Malkor, false );

    state = STATE_BEGIN;
	
	timer = 2.5;
}

bool TM14_RunCowardRun::Update (void)
{
    if ( data.mission_over )
    {
        return false;
    }

	timer -= ELAPSED_TIME;

    switch ( state )
    {
        case STATE_BEGIN:

            if ( timer <= 0 )
            {
                // warp malkor out

                MScript::FlashWormBlast( malkorWormhole );
		        MScript::RemovePart( data.Malkor );

                timer = 2.5;

                state = STATE_WARPING_OUT;
            }

            break;

        case STATE_WARPING_OUT:

            if ( timer <= 0 )
            {
                MScript::CloseWormBlast( malkorWormhole );

                state = STATE_DONE;
            }

            break;

        case STATE_DONE:

            return false;

            break;

        default:

            break;
    }

    return true;
}

//-----------------------------------------------------------------------
// OBJECT CONSTRUCTION TRIGGER

CQSCRIPTPROGRAM(TM14_ObjectConstructed, TM14_ObjectConstructed_Data,CQPROGFLAG_OBJECTCONSTRUCTED);

void TM14_ObjectConstructed::Initialize (U32 eventFlags, const MPartRef & part)
{
    if ( data.mission_over )
    {
        return;
    }

	// Turn on AI and attack system
	if (MScript::IsPlatform(part) && part->playerID == PLAYER_ID)
	{
		TurnOnAIForSystem(part->systemID);
		MantisAttackSystem(part->systemID);
	}


	switch (part->mObjClass)
	{
		case M_ACROPOLIS:	

			break;
	}
	
}

bool TM14_ObjectConstructed::Update (void)
{
	return false;
}

//-----------------------------------------------------------------------
// OBJECT DESTRUCTION TRIGGER

CQSCRIPTPROGRAM(TM14_ObjectDestroyed, TM14_ObjectDestroyed_Data,CQPROGFLAG_OBJECTDESTROYED);

void TM14_ObjectDestroyed::Initialize (U32 eventFlags, const MPartRef & part)
{
    if ( data.mission_over )
    {
        return;
    }

	if ( SameObj( part, data.Benson ) || SameObj( part, data.Takei ) )
	{
		MScript::FlushStreams();

        data.mhandle = MScript::PlayAnimatedMessage( "M14ST20.wav", "Animate!!Steele2", 
            MAIN_MAGPIE_X_POS, MAIN_MAGPIE_Y_POS, data.Steele, IDS_TM14_SUB_M14ST20 );

		MScript::MarkObjectiveFailed(IDS_TM14_OBJECTIVE3);

        MScript::MoveCamera( part, 0, MOVIE_CAMERA_JUMP_TO);

		if(SameObj( part, data.Benson ))
			data.textFailureID = IDS_TM14_FAIL_BENSON_LOST;
		else
			data.textFailureID = IDS_TM14_FAIL_TAKEI_LOST;

		MScript::RunProgramByName("TM14_MissionFailure", MPartRef ());
	}
	else if (SameObj(part, data.Steele))
	{
		MScript::MarkObjectiveFailed(IDS_TM14_OBJECTIVE3);

        MScript::MoveCamera( part, 0, MOVIE_CAMERA_JUMP_TO);

		data.textFailureID = IDS_TM14_FAIL_STEELE_LOST;

		MScript::RunProgramByName("TM14_MissionFailure", MPartRef ());
	}
	else if (SameObj(part, data.Vivac))
	{
		MScript::FlushStreams();

        data.mhandle = MScript::PlayAnimatedMessage( "M14ST22.wav", "Animate!!Steele2", 
            MAIN_MAGPIE_X_POS, MAIN_MAGPIE_Y_POS, data.Steele, IDS_TM14_SUB_M14ST22 );

		MScript::MarkObjectiveFailed(IDS_TM14_OBJECTIVE3);

        MScript::MoveCamera( part, 0, MOVIE_CAMERA_JUMP_TO);

		data.textFailureID = IDS_TM14_FAIL_VIVAC_LOST;

		MScript::RunProgramByName("TM14_MissionFailure", MPartRef ());
	}
	else if (SameObj(part, data.Halsey))
	{
		MScript::FlushStreams();

        data.mhandle = MScript::PlayAnimatedMessage( "M14ST20.wav", "Animate!!Steele2", 
            MAIN_MAGPIE_X_POS, MAIN_MAGPIE_Y_POS, data.Steele, IDS_TM14_SUB_M14ST20 );

		MScript::MarkObjectiveFailed(IDS_TM14_OBJECTIVE3);

        MScript::MoveCamera( part, 0, MOVIE_CAMERA_JUMP_TO);

		data.textFailureID = IDS_TM14_FAIL_HALSEY_LOST;

		MScript::RunProgramByName("TM14_MissionFailure", MPartRef ());
	}
	else if (SameObj(part, data.MalkorCocoon) && !data.bGotCocoon)
	{
		MScript::FlushStreams();

        data.mhandle = MScript::PlayAnimatedMessage( "M14ST19.wav", "Animate!!Steele2", 
            MAIN_MAGPIE_X_POS, MAIN_MAGPIE_Y_POS, data.Steele, IDS_TM14_SUB_M14ST19 );

		MScript::MarkObjectiveFailed(IDS_TM14_OBJECTIVE1);

        MScript::MoveCamera( part, 0, MOVIE_CAMERA_JUMP_TO);

		MScript::RunProgramByName("TM14_MissionFailure", MPartRef ());
	}
}

bool TM14_ObjectDestroyed::Update (void)
{
	return false;
}

//-----------------------------------------------------------------------
// FORBIDDEN JUMP EVENT

CQSCRIPTPROGRAM(TM14_ForbiddenJump, TM14_ForbiddenJump_Data, CQPROGFLAG_FORBIDEN_JUMP);

void TM14_ForbiddenJump::Initialize( U32 eventFlags, const MPartRef & part )
{
    if ( data.mission_over )
    {
        return;
    }

    if ( part == data.gateToJahr || part == data.gateToQuasili )
    {    
        if ( !MScript::IsStreamPlaying( data.mhandle ) )
        {
            data.mhandle = MScript::PlayAnimatedMessage( "M14HA11.wav", "Animate!!Halsey2", 
                MAIN_MAGPIE_X_POS, MAIN_MAGPIE_Y_POS, IDS_TM14_SUB_M14HA11 );
        }
    }
}

bool TM14_ForbiddenJump::Update (void)
{
	return false;
}


//---------------------------------------------------------------------------
// MISSION FAILURE PROGRAMS


//---------------------------------------------------------------------------

CQSCRIPTPROGRAM(TM14_MissionFailure, TM14_MissionFailure_Data, 0);

void TM14_MissionFailure::Initialize (U32 eventFlags, const MPartRef & part)
{
	UnderMissionControl();	

    MScript::EnableMovieMode( true );

	data.mission_over = true;
	state = Begin;

}

bool TM14_MissionFailure::Update (void)
{
	switch (state)
	{
		case Begin:

            if ( data.Halsey.isValid() )
            {
			    state = Halsey;
            }
            else
            {
                // if Halsey is no longer alive, he shouldn't be telling the player to go back for reasignment

                state = Teletype;
            }

			break;

		case Halsey:

			if ( !MScript::IsStreamPlaying(data.mhandle))
			{
				//TEMP
                data.mhandle = MScript::PlayAnimatedMessage( "M14HA21.wav", "Animate!!Halsey2", 
                    MAIN_MAGPIE_X_POS, MAIN_MAGPIE_Y_POS, data.Halsey, IDS_TM14_SUB_M14HA21 );

				state = Teletype;
			}
			break;

		case Teletype:
			
			if ( !MScript::IsStreamPlaying( data.mhandle ) )
			{
				MScript::FlushTeletype();

                data.thandle = TeletypeMissionOver( IDS_TM14_MISSION_FAILURE , data.textFailureID);

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
	}

	return true;
}

//---------------------------------------------------------------------------
// MISSION SUCCESS

CQSCRIPTPROGRAM(TM14_MissionSuccess, TM14_MissionSuccess_Data, 0);

void TM14_MissionSuccess::Initialize (U32 eventFlags, const MPartRef & part)
{
	UnderMissionControl();	

    MScript::EnableMovieMode( true );

	data.mission_over = true;
	state = Begin;

	MScript::MarkObjectiveCompleted(IDS_TM14_OBJECTIVE3);

}

bool TM14_MissionSuccess::Update (void)
{
	switch (state)
	{
		case Begin:

			state = Halsey;
			break;

		case Halsey:

            if ( !MScript::IsStreamPlaying( data.mhandle ) )
            {
                data.mhandle = MScript::PlayAnimatedMessage( "M14HA18.wav", "Animate!!Halsey2", 
                    MAIN_MAGPIE_X_POS, MAIN_MAGPIE_Y_POS, data.Halsey, IDS_TM14_SUB_M14HA18 );

    			state = Teletype;
            }

			break;

		case Teletype:

			if ( !MScript::IsStreamPlaying(data.mhandle))
			{
				MScript::FlushTeletype();

                data.thandle = TeletypeMissionOver( IDS_TM14_MISSION_SUCCESS );

				state = Done;
			}
			break;

		case Done:

			if ( !MScript::IsTeletypePlaying( data.thandle ) )
			{
				UnderPlayerControl();
				MScript::EndMissionVictory(13);

				return false;
			}
	}

	return true;
}

//--------------------------------------------------------------------------//
//	OTHER PROGRAMS
//--------------------------------------------------------------------------//

static bool SameObj(const MPartRef & obj1, const MPartRef & obj2)
{
	if (!obj1.isValid() || !obj2.isValid())
		return false;

	if (obj1->dwMissionID == obj2->dwMissionID)
		return true;
	else
		return false;
}

static void TurnOnAIForSystem(U32 SystemID)
{
	if (!data.bAIOnInSystem[SystemID])
	{
		MassEnableAI(MANTIS_ID, true, SystemID);

		data.bAIOnInSystem[SystemID] = true;

        // don't let Malkor get turned on by accident

    	MScript::EnableAIForPart( data.Malkor, false );
	}
}

static void MantisAttackSystem(U32 SystemID)
{
	MPartRef planet;
	planet = FindFirstObjectOfType(M_PLANET, 0, SystemID);

	MScript::MakeAreaVisible(MANTIS_ID, planet, 255);
	
	MScript::SetEnemyAITarget(MANTIS_ID, MPartRef(), 0, 1);
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

    data.displayObjectiveHandlerParams_stringID = stringID;
    data.displayObjectiveHandlerParams_dependantHandle = dependantHandle;

    MScript::RunProgramByName( "TM14_DisplayObjectiveHandler", MPartRef ());
}

CQSCRIPTPROGRAM( TM14_DisplayObjectiveHandler, TM14_DisplayObjectiveHandlerData, 0 );

void TM14_DisplayObjectiveHandler::Initialize
(
	U32 eventFlags, 
	const MPartRef &part 
)
{
    stringID = data.displayObjectiveHandlerParams_stringID;
    dependantHandle = data.displayObjectiveHandlerParams_dependantHandle;
}

bool TM14_DisplayObjectiveHandler::Update
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