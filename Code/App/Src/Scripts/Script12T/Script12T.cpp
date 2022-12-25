//--------------------------------------------------------------------------//
//                                                                          //
//                                Script11T.cpp                             //
//				/Conquest/App/Src/Scripts/Script12T/Script12T.cpp			//
//								MISSION PROGRAMS							//
//                                                                          //
//               COPYRIGHT (C) 2000 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//



#include <ScriptDef.h>
#include <DLLScriptMain.h>
#include "DataDef.h"
#include "stdlib.h"

#include "..\helper\helper.h"

//--------------------------------------------------------------------------
//  DEFINES
//--------------------------------------------------------------------------

#define PLAYER_ID			1
#define MANTIS_ID			2
#define SOLARIAN_ID			3
#define REBEL_ID			4

#define SOL					1
#define TAUCETI				2
#define ALKAID				3
#define BAAI				4
#define CAPELLA				5
#define PERILON				6
#define LUXOR				7
#define BEKKA				8
#define GHELEN				9
#define RENDAR				10
#define	XIN					11
#define	WOR					12
#define CORLAR				13
#define SERPCE				14
#define	QUASILI				15
#define EPSILON				16

//--------------------------------------------------------------------------//
//  FUNCTION PROTOTYPES
//--------------------------------------------------------------------------//

static void SetupAI
(
    bool bOn
);

static void ToggleBuildUnits
(
    bool bOn
);

static void TweakMantis
(
    void
);

//--------------------------------------------------------------------------//
//  MISSION PROGRAM 														//
//--------------------------------------------------------------------------//

CQSCRIPTDATA( MissionData, data );

//--------------------------------------------------------------------------
//	MISSION BRIEFING
//--------------------------------------------------------------------------

CQSCRIPTPROGRAM(TM12_Briefing, TM12_Briefing_Save,CQPROGFLAG_STARTBRIEFING);

void TM12_Briefing::Initialize (U32 eventFlags, const MPartRef & part)
{
    MScript::PlayMusic( NULL );

	data.mhandle = 0;
	data.shandle = 0;
	data.thandle = 0;
	data.mission_state = Briefing;
	data.failureID = 0;

	state = BRIEFING_STATE_RADIO_BROADCAST;

	MScript::EnableBriefingControls(true);
}

bool TM12_Briefing::Update (void)
{
    CQBRIEFINGITEM slotAnim;

	switch( state )
	{
        case BRIEFING_STATE_RADIO_BROADCAST:

            // Radio broadcast has been replaced

            MScript::FlushTeletype();

            data.thandle = TeletypeBriefing( IDS_TM12_INCOMING );

			state = BRIEFING_STATE_ALARM;

			break;

        case BRIEFING_STATE_ALARM:

            if ( !MScript::IsStreamPlaying( data.mhandle ) )
			{
                // the alarm sound effect has been removed, do an uplink

                MScript::FreeBriefingSlot( -1 );

				ShowBriefingAnimation( -1, "Xmission", ANIMSPD_XMISSION, false, false ); 

                data.mhandle = MScript::PlayAudio( "high_level_data_uplink.wav" );

                state = BRIEFING_STATE_HALSEY_1;
            }

            break;

        case BRIEFING_STATE_HALSEY_1:

            if ( !MScript::IsStreamPlaying( data.mhandle ) )
			{
                // bring up the whole gang

                ShowBriefingHead( 0, CHAR_HALSEY );
                ShowBriefingHead( 1, CHAR_ELAN );
                ShowBriefingHead( 2, CHAR_BLACKWELL );
                ShowBriefingHead( 3, CHAR_BENSON );

                // Halsey starts it off: Disabled.

                //data.mhandle = DoBriefingSpeechMagpieDirect( "m12ha02.wav", 0, CHAR_HALSEY, true );

				state = BRIEFING_STATE_BLACKWELL_1;
            }

            break;

        case BRIEFING_STATE_BLACKWELL_1:

            if ( !MScript::IsStreamPlaying( data.mhandle ) )
			{
                data.mhandle = DoBriefingSpeechMagpieDirect(IDS_TM12_SUB_M12BL03, "m12bl03.wav", 2, CHAR_BLACKWELL, true );

				state = BRIEFING_STATE_ELAN_1;
            }

            break;

        case BRIEFING_STATE_ELAN_1:

            if ( !MScript::IsStreamPlaying( data.mhandle ) )
			{
                data.mhandle = DoBriefingSpeechMagpieDirect(IDS_TM12_SUB_M12EL05, "m12EL05.wav", 1, CHAR_ELAN, true );

				state = BRIEFING_STATE_BLACKWELL_2;
            }

            break;

        case BRIEFING_STATE_BLACKWELL_2:

            if ( !MScript::IsStreamPlaying( data.mhandle ) )
			{
                data.mhandle = DoBriefingSpeechMagpieDirect(IDS_TM12_SUB_M12BL06, "m12bl06.wav", 2, CHAR_BLACKWELL, true );

				state = BRIEFING_STATE_HALSEY_2;
            }

            break;

        case BRIEFING_STATE_HALSEY_2:

            if ( !MScript::IsStreamPlaying( data.mhandle ) )
			{
                data.mhandle = DoBriefingSpeechMagpieDirect(IDS_TM12_SUB_M12HA09, "m12ha09.wav", 0, CHAR_HALSEY, true );

				state = BRIEFING_STATE_BLACKWELL_3;
            }

            break;

        case BRIEFING_STATE_BLACKWELL_3:

            if ( !MScript::IsStreamPlaying( data.mhandle ) )
			{
                data.mhandle = DoBriefingSpeechMagpieDirect(IDS_TM12_SUB_M12BL10, "m12bl10.wav", 2, CHAR_BLACKWELL, true );

				state = BRIEFING_STATE_HALSEY_3;
            }

            break;

        case BRIEFING_STATE_HALSEY_3:

            if ( !MScript::IsStreamPlaying( data.mhandle ) )
			{
                data.mhandle = DoBriefingSpeechMagpieDirect(IDS_TM12_SUB_M12HA11, "m12ha11.wav", 0, CHAR_HALSEY, true );

				state = BRIEFING_STATE_UPLINK_TO_TERRA;
            }

            break;

        case BRIEFING_STATE_UPLINK_TO_TERRA:

            if ( !MScript::IsStreamPlaying( data.mhandle ) )
			{
                data.mhandle = MScript::PlayAudio( "high_level_data_uplink.wav" );

                ShowBriefingAnimation( 1, "TechLoop1", ANIMSPD_TECHLOOP, true, true );

				state = BRIEFING_STATE_COMM;
            }

            break;

        case BRIEFING_STATE_COMM:

            if ( !MScript::IsStreamPlaying( data.mhandle ) )
			{
                data.mhandle = MScript::PlayAudio("m12CM12.wav");
				MScript::BriefingSubtitle(data.mhandle,IDS_TM12_SUB_M12CM12);

				state = BRIEFING_STATE_HALSEY_4;
            }

            break;

        case BRIEFING_STATE_HALSEY_4:

            if ( !MScript::IsStreamPlaying( data.mhandle ) )
			{
                data.mhandle = DoBriefingSpeechMagpieDirect(IDS_TM12_SUB_M12HA13, "m12ha13.wav", 0, CHAR_HALSEY, true );

				state = BRIEFING_STATE_BENSON_1;
            }

            break;

        case BRIEFING_STATE_BENSON_1:

            if ( !MScript::IsStreamPlaying( data.mhandle ) )
			{
                ShowBriefingHead( 1, CHAR_HAWKES );

                data.mhandle = DoBriefingSpeechMagpieDirect(IDS_TM12_SUB_M12BN14, "m12bn14.wav", 3, CHAR_BENSON, true );

				state = BRIEFING_STATE_BLACKWELL_4;
            }

            break;

        case BRIEFING_STATE_BLACKWELL_4:

            if ( !MScript::IsStreamPlaying( data.mhandle ) )
			{
                data.mhandle = DoBriefingSpeechMagpieDirect(IDS_TM12_SUB_M12BL15, "m12bl15.wav", 2, CHAR_BLACKWELL, true );

				state = BRIEFING_STATE_HALSEY_5;
            }

            break;

        case BRIEFING_STATE_HALSEY_5:

            if ( !MScript::IsStreamPlaying( data.mhandle ) )
			{
                data.mhandle = DoBriefingSpeechMagpieDirect(IDS_TM12_SUB_M12HA16, "m12ha16.wav", 0, CHAR_HALSEY, true );

				state = BRIEFING_STATE_BLACKWELL_5;
            }

            break;

        case BRIEFING_STATE_BLACKWELL_5:

            if ( !MScript::IsStreamPlaying( data.mhandle ) )
			{
                data.mhandle = DoBriefingSpeechMagpieDirect(IDS_TM12_SUB_M12BL17, "m12bl17.wav", 2, CHAR_BLACKWELL, true );

				state = BRIEFING_STATE_HAWKES_1;
            }

            break;

        case BRIEFING_STATE_HAWKES_1:

            if ( !MScript::IsStreamPlaying( data.mhandle ) )
			{
                data.mhandle = DoBriefingSpeechMagpieDirect(IDS_TM12_SUB_M12HW18, "m12hw18.wav", 1, CHAR_HAWKES, true );

				state = BRIEFING_STATE_BLACKWELL_6;
            }

            break;

        case BRIEFING_STATE_BLACKWELL_6:

            if ( !MScript::IsStreamPlaying( data.mhandle ) )
			{
                MScript::FreeBriefingSlot( 1 );

                data.mhandle = DoBriefingSpeechMagpieDirect(IDS_TM12_SUB_M12BL19, "m12bl19.wav", 2, CHAR_BLACKWELL, true );

				state = BRIEFING_STATE_BENSON_2;
            }

            break;

        case BRIEFING_STATE_BENSON_2:

            if ( !MScript::IsStreamPlaying( data.mhandle ) )
			{
                data.mhandle = DoBriefingSpeechMagpieDirect(IDS_TM12_SUB_M12BN20, "m12bn20.wav", 3, CHAR_BENSON, true );

				state = BRIEFING_STATE_BLACKWELL_7;
            }

            break;

        case BRIEFING_STATE_BLACKWELL_7:

            if ( !MScript::IsStreamPlaying( data.mhandle ) )
			{
                data.mhandle = DoBriefingSpeechMagpieDirect(IDS_TM12_SUB_M12BL21, "m12bl21.wav", 2, CHAR_BLACKWELL, true );

				state = BRIEFING_STATE_HALSEY_6;
            }

            break;

        case BRIEFING_STATE_HALSEY_6:

            if ( !MScript::IsStreamPlaying( data.mhandle ) )
			{
                MScript::FreeBriefingSlot( 2 );
                MScript::FreeBriefingSlot( 3 );

                data.mhandle = DoBriefingSpeechMagpieDirect(IDS_TM12_SUB_M12HA22, "m12ha22.wav", 0, CHAR_HALSEY, true );

				state = BRIEFING_STATE_OBJECTIVES;
            }

            break;

        case BRIEFING_STATE_OBJECTIVES:

            if ( !MScript::IsStreamPlaying( data.mhandle ) )
			{
                MScript::FreeBriefingSlot( -1 );

                data.thandle = MScript::PlayBriefingTeletype( IDS_TM12_OBJECTIVES,
                    STANDARD_TELETYPE_COLOR, 0, 
                    MScript::GetScriptStringLength( IDS_TM12_OBJECTIVES ) * STANDARD_TELETYPE_MILS_PER_CHAR, 
                    false );

				state = BRIEFING_STATE_FINISHED;
			}

            break;

        case BRIEFING_STATE_FINISHED:

            break;

        default:

            break;	
	}

	return true;
}

//--------------------------------------------------------------------------

CQSCRIPTPROGRAM(TM12_MissionStart, TM12_MissionStart_Save,CQPROGFLAG_STARTMISSION);

void TM12_MissionStart::Initialize (U32 eventFlags, const MPartRef & part)
{

	data.mission_state = MissionStart;
	state = Begin;

	MScript::SetAllies( REBEL_ID, MANTIS_ID, true);
	MScript::SetAllies( MANTIS_ID, REBEL_ID, true);

    MScript::SetMissionID( 11 );
	MScript::SetMissionName( IDS_TM12_MISSION_NAME );
	MScript::SetMissionDescription( IDS_TM12_MISSION_DESC );

    MScript::SetEnemyCharacter( MANTIS_ID, IDS_TM12_MANTIS_PLAYER_NAME );

    MScript::EnableRegenMode( true );

	MScript::AddToObjectiveList( IDS_TM12_OBJECTIVE1 );
	MScript::AddToObjectiveList( IDS_TM12_OBJECTIVE2 );
	MScript::AddToObjectiveList( IDS_TM12_OBJECTIVE3 );
	MScript::AddToObjectiveList( IDS_TM12_OBJECTIVE4 );

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
        TECHTREE::RES_SHIELDS4 ^
        TECHTREE::RES_SHIELDS5 ^
        TECHTREE::RES_ENGINE5 ^
        TECHTREE::RES_WEAPONS3 ^
        TECHTREE::RES_WEAPONS4 ^
        TECHTREE::RES_WEAPONS5 );

	missionTech.race[0].build = missionTech.race[1].build = 
        (TECHTREE::BUILDNODE) (TECHTREE::ALL_BUILDNODE ^ 
        TECHTREE::TDEPEND_ION_CANNON );

	missionTech.race[2].build = 
        (TECHTREE::BUILDNODE) ( ( TECHTREE::ALL_BUILDNODE ^ 
        TECHTREE::SDEPEND_PORTAL ) | 
        TECH_TREE_RACE_BITS_ALL );

	missionTech.race[0].tech = missionTech.race[1].tech = missionTech.race[2].tech = 
        (TECHTREE::TECHUPGRADE) (TECHTREE::ALL_TECHUPGRADE ^ 
        TECHTREE::T_RES_TROOPSHIP3 );    

    MScript::SetAvailiableTech( missionTech );

	MScript::SetMaxGas(MANTIS_ID, 240);
	MScript::SetMaxMetal(MANTIS_ID, 240);
	MScript::SetMaxCrew(MANTIS_ID, 200);

    MScript::SetMaxCommandPoitns( MANTIS_ID, 200 );
    MScript::SetMaxCommandPoitns( PLAYER_ID, 100 );

	MScript::EnableSystem( QUASILI, true, true );
	MScript::EnableSystem( EPSILON, true, true );
	MScript::EnableSystem( CORLAR, true, true );
	MScript::EnableSystem( SOL, true, true );
	MScript::EnableSystem( TAUCETI, true, true );
	MScript::EnableSystem( ALKAID, true, true );
	MScript::EnableSystem( BAAI, true, true );
	MScript::EnableSystem( PERILON, true, true );
	MScript::EnableSystem( BEKKA, true, true );
	MScript::EnableSystem( LUXOR, true, true );
	MScript::EnableSystem( GHELEN, true, true );

	SetupAI( true );
	MassEnableAI( MANTIS_ID, false, 0 );

    MScript::EnableAIForPart( MScript::GetPartByName( "Spitter1" ), false );
    MScript::EnableAIForPart( MScript::GetPartByName( "Spitter2" ), false );

	MassEnableAI( MANTIS_ID, true, CORLAR );


	MassEnableAI( MANTIS_ID, true, SERPCE );
	MassEnableAI( MANTIS_ID, true, QUASILI );
	MassEnableAI( MANTIS_ID, true, EPSILON );

	MassStance( M_HIVECARRIER, MANTIS_ID, US_DEFEND, SERPCE );
	MassStance( M_SCARAB, MANTIS_ID, US_DEFEND, SERPCE );
	MassStance( M_PLASMASPLITTER, MANTIS_ID, US_ATTACK, SERPCE );

    //Set up the disabled plats

	MPartRef dHQ = MScript::GetPartByName("HQ");
	MPartRef dRepair = MScript::GetPartByName("Academy");
	MPartRef dRefinery = MScript::GetPartByName("Thripid");
	MPartRef dLight = MScript::GetPartByName("Plantation");

	MScript::EnableSelection( dHQ, false);
	MScript::EnableSelection( dRepair, false);
	MScript::EnableSelection( dRefinery, false);
	MScript::EnableSelection( dLight, false);

	MScript::SetHullPoints( dHQ, 50);
	MScript::SetHullPoints( dRepair, 50);
	MScript::SetHullPoints( dRefinery, 50);
	MScript::SetHullPoints( dLight, 50);

    //End set up of disabled plats

    data.mainAcropolis = MScript::GetPartByName( "#26071#Acropolis" );

	data.Vivac = MScript::GetPartByName("Vivac");
	MScript::EnablePartnameDisplay(data.Vivac, true);

	data.Steele = MScript::GetPartByName("TNS Steele");
	MScript::EnablePartnameDisplay(data.Steele, true);

	data.Benson = MScript::GetPartByName("TNS Benson");
	MScript::EnablePartnameDisplay( data.Benson, true);

	data.Takei = MScript::GetPartByName("TNS Takei");
	MScript::EnablePartnameDisplay(data.Takei, true);
	
	data.Blackwell = MScript::GetPartByName("TNS Blackwell");
	MScript::EnablePartnameDisplay(data.Blackwell, true);
	
	data.Hawkes = MScript::GetPartByName("TNS Hawkes");
	MScript::EnablePartnameDisplay(data.Hawkes, true);

	data.SpawnPoint = MScript::GetPartByName( "SmirnoffSpawnWP" );

	data.Quasili = MScript::GetPartByName("Quasili Prime");
	MScript::MakeAreaVisible(MANTIS_ID, data.Quasili, 3);

	data.Epsilon = MScript::GetPartByName("Epsilon 2");
	MScript::MakeAreaVisible(MANTIS_ID, data.Epsilon, 3);

    MPartRef EpsGate = MScript::GetPartByName("Gate34");
	MScript::MakeAreaVisible(MANTIS_ID, EpsGate, 2);

    // block off mantis space to the player so he doesn't witness the funkiness going on behind the scenes

	data.serpceGate = MScript::GetPartByName("Gate33");

    MScript::EnableSystem( SERPCE, false );
	MScript::EnableJumpgate( data.serpceGate, false );

    MScript::MoveCamera( data.Blackwell, 0, MOVIE_CAMERA_JUMP_TO);
	
	UnderPlayerControl();

	MScript::PlayMusic( "escort.wav" );

	MScript::RunProgramByName("TM12_InitialMantisAttacks", MPartRef() );
	MScript::RunProgramByName("TM12_ProgressCheck", MPartRef() );
	MScript::RunProgramByName("TM12_QuasiliAttack", MPartRef() );
	MScript::RunProgramByName("TM12_AIProgram", MPartRef() );
}

bool TM12_MissionStart::Update (void)
{
	return false;
}

//--------------------------------------------------------------------------//

CQSCRIPTPROGRAM(TM12_AIProgram, TM12_AIProgram_Save, 0);

void TM12_AIProgram::Initialize (U32 eventFlags, const MPartRef & part)
{

}

bool TM12_AIProgram::Update (void)
{
	if ( data.mission_over )
    {
		return false;
    }

	//Set up cases in which the AI will turn off

	if ( MScript::GetUsedCommandPoints( MANTIS_ID ) > 85 || MScript::GetUsedCommandPoints( PLAYER_ID ) < 15 )
	{
		ToggleBuildUnits( false );

		return true;
	}

    //Set up cases in which the AI will turn back on

	if ( MScript::GetUsedCommandPoints( MANTIS_ID ) < 50 || MScript::GetUsedCommandPoints( PLAYER_ID ) > 70 )
	{
		ToggleBuildUnits( true );
	}

	return true;
}

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//

CQSCRIPTPROGRAM(TM12_InitialMantisAttacks, TM12_InitialMantisAttacks_Save, 0);

void TM12_InitialMantisAttacks::Initialize (U32 eventFlags, const MPartRef & part)
{
	XinSensor = MScript::GetPartByName( "XinSensor" );
	RendarSensor = MScript::GetPartByName( "RendarSensor" );

	GPrime = MScript::GetPartByName( "Ghelen Prime" );
	MScript::MakeAreaVisible( MANTIS_ID, GPrime, 3 );

	GhelenGate1 = MScript::GetPartByName( "Gate27" );
	MScript::MakeAreaVisible( MANTIS_ID, GhelenGate1, 3 );

	GhelenGate2 = MScript::GetPartByName( "Gate25" );
	MScript::MakeAreaVisible( MANTIS_ID, GhelenGate2, 3 );

	XinSpawn = MScript::GetPartByName( "XinSpawn" );
	RenSpawn = MScript::GetPartByName( "RenSpawn" );

    // set up attack on Rendar

	MScript::SetHullPoints(RendarSensor, 20);

	MPartRef Frig1 = MScript::CreatePart("GBOAT!!M_Frigate", MScript::GetPartByName("RendarWP"), MANTIS_ID, 0);
	MPartRef Frig2 = MScript::CreatePart("GBOAT!!M_Frigate", MScript::GetPartByName("RendarWP"), MANTIS_ID, 0);

	MScript::OrderAttack(Frig1, RendarSensor);
	MScript::OrderAttack(Frig2, RendarSensor);

	MassEnableAI(MANTIS_ID, true, RENDAR);
	MassEnableAI(MANTIS_ID, true, XIN);

    // set up attack on Xin

	MScript::SetHullPoints(XinSensor, 20);

	MPartRef Frig3 = MScript::CreatePart("GBOAT!!M_Frigate", MScript::GetPartByName("XinWP"), MANTIS_ID, 0);
	MPartRef Frig4 = MScript::CreatePart("GBOAT!!M_Frigate", MScript::GetPartByName("XinWP"), MANTIS_ID, 0);

	MScript::OrderAttack(Frig3, XinSensor);
	MScript::OrderAttack(Frig4, XinSensor);

	MassEnableAI(MANTIS_ID, true, RENDAR);
	MassEnableAI(MANTIS_ID, true, XIN);

	timer = 30;
}

bool TM12_InitialMantisAttacks::Update (void)
{
	if ( data.mission_over )
    {
		return false;
    }

	timer -= ELAPSED_TIME;

	if( timer < 0 )
	{
		for (int x=0; x<2; x++)
		{
			MScript::CreatePart("GBOAT!!M_Tiamat", XinSpawn, MANTIS_ID);
		}
		for (x=0; x<2; x++)
		{
			MScript::CreatePart("GBOAT!!M_Scout Carrier", XinSpawn, MANTIS_ID);
		}

		MScript::SetEnemyAITarget(MANTIS_ID, GPrime, 3, GHELEN);
		MScript::LaunchOffensive(MANTIS_ID, US_ATTACK);

		for (x=0; x<2; x++)
		{
			MScript::CreatePart("GBOAT!!M_Tiamat", RenSpawn, MANTIS_ID);
		}
		for (x=0; x<2; x++)
		{
			MScript::CreatePart("GBOAT!!M_Scout Carrier", RenSpawn, MANTIS_ID);
		}


		MScript::SetEnemyAITarget(MANTIS_ID, GPrime, 3, GHELEN);
		MScript::LaunchOffensive(MANTIS_ID, US_ATTACK);

		SetupAI(true);

        return false;
	}

	return true;
}



//-----------------------------------------------------------------------
// 

CQSCRIPTPROGRAM(TM12_QuasiliAttack, TM12_QuasiliAttack_Save,0);

void TM12_QuasiliAttack::Initialize (U32 eventFlags, const MPartRef & part)
{
    MScript::MakeAreaVisible( MANTIS_ID, data.SpawnPoint, 50 );
    MScript::MakeAreaVisible( MANTIS_ID, data.Quasili, 50 );
    MScript::MakeAreaVisible( MANTIS_ID, data.Epsilon, 50 );

	timer = 20;
}

bool TM12_QuasiliAttack::Update (void)
{
	timer -= ELAPSED_TIME;

	if ( data.mission_over )
    {
		return false;
    }

	if(timer < 0)
	{
		for (int x=0; x<2; x++)
		{
			MScript::CreatePart("GBOAT!!M_Scarab", data.SpawnPoint, MANTIS_ID);
		}
		for (x=0; x<2; x++)
		{
			MScript::CreatePart("GBOAT!!M_Hive Carrier", data.SpawnPoint, MANTIS_ID);
		}

		MScript::SetEnemyAITarget(MANTIS_ID, data.Epsilon, 50, EPSILON);
		MScript::LaunchOffensive(MANTIS_ID, US_ATTACK);

		for (x=0; x<2; x++)
		{
			MScript::CreatePart("GBOAT!!M_Tiamat", data.SpawnPoint, MANTIS_ID);
		}
		for (x=0; x<2; x++)
		{
			MScript::CreatePart("GBOAT!!M_Scout Carrier", data.SpawnPoint, MANTIS_ID);
		}

		MScript::SetEnemyAITarget(MANTIS_ID, data.Epsilon, 50, EPSILON);
		MScript::LaunchOffensive(MANTIS_ID, US_ATTACK);

		timer = 180;
	}

	return true;
}

//--------------------------------------------------------------------------//

CQSCRIPTPROGRAM(TM12_ProgressCheck, TM12_ProgressCheck_Save, 0);

void TM12_ProgressCheck::Initialize (U32 eventFlags, const MPartRef & part)
{
	blackwellPerilon = hawkesPerilon = worFound = luxorReached = perilonReached = ghelenReached = false;
}

bool TM12_ProgressCheck::Update (void)
{
	MPartRef target = MScript::GetFirstPart();

	if(data.mission_over)
	{
		return false;
	}

    // check if Blackwell and Hawkes beet the Mantis to Perilon

    if ( !perilonReached )
    {
		if((!blackwellPerilon) && data.Blackwell->systemID == PERILON)
		{
			blackwellPerilon = true;
		}
		if((!hawkesPerilon) && data.Hawkes->systemID == PERILON)
		{
			hawkesPerilon = true;
		}
	    if ( PlayerInSystem( MANTIS_ID, PERILON ) )
	    {
            // The Mantis have beet Hawkes and Blackwell to Perilon, Game Over man!

            data.mhandle = MScript::PlayAnimatedMessage( "M12HA56.wav", "Animate!!Halsey2", 
                MAIN_MAGPIE_X_POS, MAIN_MAGPIE_Y_POS, IDS_TM12_SUB_M12HA56 );

		    MScript::RunProgramByName( "TM12_MissionFailure", MPartRef() );

		    return false;
	    }
	    else if ( hawkesPerilon && blackwellPerilon )
	    {
            // Hawkes and Blackwell have arrived in Perilon

            MScript::MarkObjectiveCompleted( IDS_TM12_OBJECTIVE2 );

            // JUSNOTE: This should be triggered in Perilon: temp jury rig

    		MScript::RunProgramByName( "TM12_ComeInBlackwell", MPartRef() );

		    perilonReached = true;
	    }
    }

    // check if the player has reached Ghelen

    if ( !ghelenReached && ( data.Hawkes->systemID == GHELEN || data.Blackwell->systemID == GHELEN ) )
    {
        // time for Smirnoff to strike

		MScript::RunProgramByName( "TM12_SmirnoffAttack", MPartRef() );

        ghelenReached = true;
    }

    // check if the player has reached Luxor

	if ( !luxorReached && ( data.Hawkes->systemID == LUXOR || data.Blackwell->systemID == LUXOR ) )
	{
        // JUSNOTE: This is where this should be done, but jury-rigging for time being

		// MScript::RunProgramByName( "TM12_ComeInBlackwell", MPartRef() );

		luxorReached = true;
	}

    // check if the player has reached Wor

	if ( PlayerInSystem( PLAYER_ID, WOR ) && !worFound )
	{
		MPartRef Fab, Weaver, Sup1, Sup2, Sup3;

		Fab = MScript::GetPartByName("Fab");
		Weaver = MScript::GetPartByName("Weaver");
		Sup1 = MScript::GetPartByName("Sup1");
		Sup2 = MScript::GetPartByName("Sup2");
		Sup3 = MScript::GetPartByName("Sup3");

		MScript::ClearHardFog(Fab, 1);
		MScript::ClearHardFog(Weaver, 1);
		MScript::ClearHardFog(Sup1, 1);
		MScript::ClearHardFog(Sup2, 1);
		MScript::ClearHardFog(Sup3, 1);

		MScript::RunProgramByName( "TM12_CaptureUnits", MPartRef() );
		MScript::RunProgramByName( "TM12_RepairedPlatform", MPartRef() );

		worFound = true;
	}

	return true;
}

//--------------------------------------------------------------------------//

CQSCRIPTPROGRAM(TM12_CaptureUnits, TM12_CaptureUnits_Save, 0);

void TM12_CaptureUnits::Initialize (U32 eventFlags, const MPartRef & part)
{

}

bool TM12_CaptureUnits::Update (void)
{
	MPartRef target = MScript::GetFirstPart();

	if ( data.mission_over )
    {
		return false;
    }

	if ( !MScript::PlayerHasUnits( REBEL_ID ) )
	{
		return false;
	}

	while(target.isValid())
	{	
		if(target->playerID == REBEL_ID && MScript::IsVisibleToPlayer(target, PLAYER_ID))
		{
			MScript::SwitchPlayerID(target, PLAYER_ID);
		}

		target = MScript::GetNextPart(target);
	}

	return true;
}

//-----------------------------------------------------------------------
// 

CQSCRIPTPROGRAM(TM12_RepairedPlatform, TM12_RepairedPlatform_Save,0);

void TM12_RepairedPlatform::Initialize (U32 eventFlags, const MPartRef & part)
{
	data.NumOfPlats = 0;

}

bool TM12_RepairedPlatform::Update (void)
{
	if ( data.mission_over )
    {
		return false;
    }

	if ( data.NumOfPlats < 4 )
	{
		MPartRef target = MScript::GetFirstPart();

		while ( target.isValid() )
		{
			if ( target->bUnselectable && target->hullPoints >= target->hullPointsMax )
			{
				MScript::EnableSelection( target, true );
				data.NumOfPlats++;
			}

			target = MScript::GetNextPart( target );
		}
	}
	else 
    {
		return false;
    }

	return true;
}

//-----------------------------------------------------------------------
// 

CQSCRIPTPROGRAM(TM12_SmirnoffAttack, TM12_SmirnoffAttack_Save,0);

void TM12_SmirnoffAttack::Initialize (U32 eventFlags, const MPartRef & part)
{
    int tempShip;
    MPartRef Attacker;

	RallyPoint = MScript::GetPartByName("AttackWP");

	data.Smirnoff = MScript::CreatePart("CHAR!!Smirnoff_D", data.SpawnPoint, MANTIS_ID, "TNS Smirnoff");

	MScript::EnableAIForPart(data.Smirnoff, false);
	MScript::SetStance(data.Smirnoff, US_ATTACK);

    // don't let Smirnoff get lower than 10% damage as he has to survive the mission

    MScript::MakeInvincible( data.Smirnoff, true );

	MScript::OrderMoveTo(data.Smirnoff, RallyPoint);

    tempShip = 0;

	for (int x=0; x<5; x++)
	{
		Attacker = MScript::CreatePart("GBOAT!!T_Midas Battleship", data.SpawnPoint, MANTIS_ID);
		MScript::OrderMoveTo(Attacker, RallyPoint);

        shipsInFleet[tempShip] = Attacker;

        tempShip++;
	}

	for (x=0; x<4; x++)
	{
		Attacker = MScript::CreatePart("GBOAT!!M_Scarab", data.SpawnPoint, MANTIS_ID);
		MScript::OrderMoveTo(Attacker, RallyPoint);

        shipsInFleet[tempShip] = Attacker;

        tempShip++;
	}

	for (x=0; x<2; x++)
	{
		Attacker = MScript::CreatePart("GBOAT!!M_Hive Carrier", data.SpawnPoint, MANTIS_ID);
		MScript::OrderMoveTo(Attacker, RallyPoint);

        shipsInFleet[tempShip] = Attacker;

        tempShip++;
	}

	for (x=0; x<1; x++)
	{
		Attacker = MScript::CreatePart("GBOAT!!M_Tiamat", data.SpawnPoint, MANTIS_ID);
		MScript::OrderMoveTo(Attacker, RallyPoint);

        shipsInFleet[tempShip] = Attacker;

        tempShip++;
	}

    MScript::AlertMessage( RallyPoint, NULL );

	state = PROG_STATE_SMIRNOFF;
}

bool TM12_SmirnoffAttack::Update (void)
{
    int tempShip;

	timer -= ELAPSED_TIME;

	if ( data.mission_over )
    {
		return false;
    }

    // make sure that Smirnoff's fleet doesn't just hang out in the starting system

    for ( tempShip = 0; tempShip < 12; tempShip++ )
    {
        if ( shipsInFleet[tempShip].isValid() )
        {
            if ( MScript::IsIdle( shipsInFleet[tempShip] ) && 
                shipsInFleet[tempShip]->systemID == data.SpawnPoint->systemID )
            {
    		    MScript::OrderMoveTo( shipsInFleet[tempShip], RallyPoint );
            }
        }
    }

	switch(state)
	{
		case PROG_STATE_SMIRNOFF:

			if ( !MScript::IsStreamPlaying( data.mhandle ) )
			{
                data.mhandle = MScript::PlayAnimatedMessage( "M12SM23.wav", "Animate!!Smirnoff2", 
                    MAIN_MAGPIE_X_POS, MAIN_MAGPIE_Y_POS, data.Smirnoff , IDS_TM12_SUB_M12SM23);

				state = PROG_STATE_STEELE_1;
			}

			break;

		case PROG_STATE_STEELE_1:

			if ( !MScript::IsStreamPlaying( data.mhandle ) )
			{
                data.mhandle = MScript::PlayAnimatedMessage( "M12ST24.wav", "Animate!!Steele2", 
                    MAIN_MAGPIE_X_POS, MAIN_MAGPIE_Y_POS, data.Steele, IDS_TM12_SUB_M12ST24 );
	
				state = PROG_STATE_SMIRNOFF_2;
			}

			break;

		case PROG_STATE_SMIRNOFF_2:

			if ( !MScript::IsStreamPlaying( data.mhandle ) )
			{
                data.mhandle = MScript::PlayAnimatedMessage( "M12SM25.wav", "Animate!!Smirnoff2", 
                    MAIN_MAGPIE_X_POS, MAIN_MAGPIE_Y_POS, data.Smirnoff , IDS_TM12_SUB_M12SM25);

				state = PROG_STATE_STEELE_2;
			}

			break;

		case PROG_STATE_STEELE_2:

			if ( !MScript::IsStreamPlaying( data.mhandle ) )
			{
                data.mhandle = MScript::PlayAnimatedMessage( "M12ST26.wav", "Animate!!Steele2", 
                    MAIN_MAGPIE_X_POS, MAIN_MAGPIE_Y_POS, data.Steele, IDS_TM12_SUB_M12ST26 );
	
				state = PROG_STATE_SMIRNOFF_BUGGERS_OFF;
			}

			break;

		case PROG_STATE_SMIRNOFF_BUGGERS_OFF:

			if ( !MScript::IsStreamPlaying( data.mhandle ) && data.Smirnoff->hullPoints < 300)
			{
                data.mhandle = MScript::PlayAnimatedMessage( "M12SM28.wav", "Animate!!Smirnoff2", 
                    MAIN_MAGPIE_X_POS, MAIN_MAGPIE_Y_POS, IDS_TM12_SUB_M12SM28 );

                // create the wormhole through which smirnoff will escape

                smirnoffWormhole = MScript::CreateWormBlast( data.Smirnoff, 1, MANTIS_ID, false );

                // make sure smirnoff doesn't leave the vicinity of the wormhole

                MScript::OrderCancel( data.Smirnoff );
                MScript::SetStance( data.Smirnoff, US_STOP );
                MScript::EnableMoveCap( data.Smirnoff, false );
	
				state = PROG_STATE_TAKEI_1;
			}

			break;

		case PROG_STATE_TAKEI_1:

			if ( !MScript::IsStreamPlaying( data.mhandle ) )
			{
                // warp smirnoff out

                MScript::FlashWormBlast( smirnoffWormhole );
				MScript::RemovePart( data.Smirnoff );

                data.mhandle = MScript::PlayAnimatedMessage( "M12TK29.wav", "Animate!!Takei2", 
                    MAIN_MAGPIE_X_POS, MAIN_MAGPIE_Y_POS, data.Takei, IDS_TM12_SUB_M12TK29 );
	
				state = PROG_STATE_STEELE_3;
			}

			break;

		case PROG_STATE_STEELE_3:

			if ( !MScript::IsStreamPlaying( data.mhandle ) )
			{
                MScript::CloseWormBlast( smirnoffWormhole );

                data.mhandle = MScript::PlayAnimatedMessage( "M12ST30.wav", "Animate!!Steele2", 
                    MAIN_MAGPIE_X_POS, MAIN_MAGPIE_Y_POS, data.Steele, IDS_TM12_SUB_M12ST30 );	
	
				state = PROG_STATE_TAKEI_2;
			}

			break;

		case PROG_STATE_TAKEI_2:

			if ( !MScript::IsStreamPlaying( data.mhandle ) )
			{
                data.mhandle = MScript::PlayAnimatedMessage( "M12TK31.wav", "Animate!!Takei2", 
                    MAIN_MAGPIE_X_POS, MAIN_MAGPIE_Y_POS, data.Takei, IDS_TM12_SUB_M12TK31 );
	
				state = PROG_STATE_STEELE_4;
			}

			break;

		case PROG_STATE_STEELE_4:

			if ( !MScript::IsStreamPlaying( data.mhandle ) )
			{
                data.mhandle = MScript::PlayAnimatedMessage( "M12ST32.wav", "Animate!!Steele2", 
                    MAIN_MAGPIE_X_POS, MAIN_MAGPIE_Y_POS, data.Steele, IDS_TM12_SUB_M12ST32 );	
	
				state = PROG_STATE_VIVAC_1;
			}

			break;

		case PROG_STATE_VIVAC_1:

			if ( !MScript::IsStreamPlaying( data.mhandle ) )
			{
                data.mhandle = MScript::PlayAnimatedMessage( "M12VV33.wav", "Animate!!Vivac2", 
                    MAIN_MAGPIE_X_POS, MAIN_MAGPIE_Y_POS, data.Vivac, IDS_TM12_SUB_M12VV33 );
	
				state = PROG_STATE_TAKEI_3;
			}

			break;

		case PROG_STATE_TAKEI_3:

			if ( !MScript::IsStreamPlaying( data.mhandle ) )
			{
                data.mhandle = MScript::PlayAnimatedMessage( "M12TK34.wav", "Animate!!Takei2", 
                    MAIN_MAGPIE_X_POS, MAIN_MAGPIE_Y_POS, data.Takei, IDS_TM12_SUB_M12TK34 );
	
				state = PROG_STATE_VIVAC_2;
			}

			break;

		case PROG_STATE_VIVAC_2:

			if ( !MScript::IsStreamPlaying( data.mhandle ) )
			{
                data.mhandle = MScript::PlayAnimatedMessage( "M12VV35.wav", "Animate!!Vivac2", 
                    MAIN_MAGPIE_X_POS, MAIN_MAGPIE_Y_POS, data.Vivac, IDS_TM12_SUB_M12VV35 );
	
				state = PROG_STATE_STEELE_5;
			}

			break;

		case PROG_STATE_STEELE_5:

			if ( !MScript::IsStreamPlaying( data.mhandle ) )
			{
                data.mhandle = MScript::PlayAnimatedMessage( "M12ST36.wav", "Animate!!Steele2", 
                    MAIN_MAGPIE_X_POS, MAIN_MAGPIE_Y_POS, data.Steele, IDS_TM12_SUB_M12ST36 );	
	
				state = PROG_STATE_TAKEI_4;
			}

			break;

		case PROG_STATE_TAKEI_4:

			if ( !MScript::IsStreamPlaying( data.mhandle ) )
			{
                data.mhandle = MScript::PlayAnimatedMessage( "M12TK37.wav", "Animate!!Takei2", 
                    MAIN_MAGPIE_X_POS, MAIN_MAGPIE_Y_POS, data.Takei, IDS_TM12_SUB_M12TK37 );

                return false;	
			}

			break;

        default:

            break;
	}

	return true;
}

//-----------------------------------------------------------------------
// 

CQSCRIPTPROGRAM(TM12_ComeInBlackwell, TM12_ComeInBlackwell_Save,0);

void TM12_ComeInBlackwell::Initialize (U32 eventFlags, const MPartRef & part)
{
	state = PROG_STATE_STEELE_1;
}

bool TM12_ComeInBlackwell::Update (void)
{
	if ( data.mission_over )
    {
		return false;
    }

    switch ( state )
    {
        case PROG_STATE_STEELE_1:

			if ( !MScript::IsStreamPlaying( data.mhandle ) )
			{
                data.mhandle = MScript::PlayAnimatedMessage( "M12ST38.wav", "Animate!!Steele2", 
                    MAIN_MAGPIE_X_POS, MAIN_MAGPIE_Y_POS, data.Steele, IDS_TM12_SUB_M12ST38 );
	
				state = PROG_STATE_BLACKWELL_1;
			}

            break;

        case PROG_STATE_BLACKWELL_1:

			if ( !MScript::IsStreamPlaying( data.mhandle ) )
			{
                data.mhandle = MScript::PlayAnimatedMessage( "M12BL39.wav", "Animate!!Blackwell2", 
                    MAIN_MAGPIE_X_POS, MAIN_MAGPIE_Y_POS, data.Blackwell, IDS_TM12_SUB_M12BL39 );

                state = PROG_STATE_STEELE_2;
			}

            break;

        case PROG_STATE_STEELE_2:

			if ( !MScript::IsStreamPlaying( data.mhandle ) )
			{
                data.mhandle = MScript::PlayAnimatedMessage( "M12ST40.wav", "Animate!!Steele2", 
                    MAIN_MAGPIE_X_POS, MAIN_MAGPIE_Y_POS, data.Steele, IDS_TM12_SUB_M12ST40 );
	
				state = PROG_STATE_BLACKWELL_2;
			}

            break;

        case PROG_STATE_BLACKWELL_2:

			if ( !MScript::IsStreamPlaying( data.mhandle ) )
			{
                data.mhandle = MScript::PlayAnimatedMessage( "M12BL41.wav", "Animate!!Blackwell2", 
                    MAIN_MAGPIE_X_POS, MAIN_MAGPIE_Y_POS, data.Blackwell, IDS_TM12_SUB_M12BL41 );

                state = PROG_STATE_STEELE_3;
			}

            break;

        case PROG_STATE_STEELE_3:

			if ( !MScript::IsStreamPlaying( data.mhandle ) )
			{
                data.mhandle = MScript::PlayAnimatedMessage( "M12ST42.wav", "Animate!!Steele2", 
                    MAIN_MAGPIE_X_POS, MAIN_MAGPIE_Y_POS, data.Steele, IDS_TM12_SUB_M12ST42 );
	
				state = PROG_STATE_BLACKWELL_3;
			}

            break;

        case PROG_STATE_BLACKWELL_3:

			if ( !MScript::IsStreamPlaying( data.mhandle ) )
			{
                data.mhandle = MScript::PlayAnimatedMessage( "M12BL43.wav", "Animate!!Blackwell2", 
                    MAIN_MAGPIE_X_POS, MAIN_MAGPIE_Y_POS, data.Blackwell, IDS_TM12_SUB_M12BL43 );

                state = PROG_STATE_WAIT_FOR_SOL;
			}

            break;

        case PROG_STATE_WAIT_FOR_SOL:

			if( ( data.Blackwell->systemID == SOL && data.Hawkes->systemID == SOL ) )
			{
				MScript::RunProgramByName("TM12_MissionSuccess", MPartRef());

				return false;
			}

            break;

        default:

            break;
    }

    return true;
}

//-----------------------------------------------------------------------
// OBJECT DESTRUCTION TRIGGER

CQSCRIPTPROGRAM( TM12_ObjectDestroyed, TM12_ObjectDestroyed_Save, CQPROGFLAG_OBJECTDESTROYED );

void TM12_ObjectDestroyed::Initialize (U32 eventFlags, const MPartRef & part)
{
	if ( data.mission_over )
    {
		return;
    }
  
    if ( part == data.Hawkes || part == data.Benson || part == data.Takei )
    {
        MScript::EndStream( data.mhandle );

        data.mhandle = MScript::PlayAnimatedMessage( "M12ST51.wav", "Animate!!Steele2", 
            MAIN_MAGPIE_X_POS, MAIN_MAGPIE_Y_POS, data.Steele, IDS_TM12_SUB_M12ST51 );

    	MScript::MoveCamera( part, 0, MOVIE_CAMERA_JUMP_TO);

		if(part == data.Hawkes)
			data.failureID = IDS_TM12_FAIL_HAWKES_LOST;
		else if(part == data.Benson)
			data.failureID = IDS_TM12_FAIL_BENSON_LOST;
		else if(part == data.Takei)
			data.failureID = IDS_TM12_FAIL_TAKEI_LOST;

		MScript::RunProgramByName("TM12_MissionFailure", MPartRef());
    }
	else if ( part == data.Blackwell )
	{
        MScript::EndStream( data.mhandle );

        data.mhandle = MScript::PlayAnimatedMessage( "M12HW55.wav", "Animate!!Hawkes2", 
            MAIN_MAGPIE_X_POS, MAIN_MAGPIE_Y_POS, data.Hawkes, IDS_TM12_SUB_M12HW55 );

    	MScript::MoveCamera( part, 0, MOVIE_CAMERA_JUMP_TO);

		data.failureID = IDS_TM12_FAIL_BLACKWELL_LOST;

		MScript::RunProgramByName("TM12_MissionFailure", MPartRef());
	}
	else if ( part == data.Steele )
	{
        MScript::EndStream( data.mhandle );

        data.mhandle = MScript::PlayAnimatedMessage( "M12TK52.wav", "Animate!!Takei2", 
            MAIN_MAGPIE_X_POS, MAIN_MAGPIE_Y_POS, data.Takei, IDS_TM12_SUB_M12TK52 );
	
    	MScript::MoveCamera( part, 0, MOVIE_CAMERA_JUMP_TO);

		data.failureID = IDS_TM12_FAIL_STEELE_LOST;

		MScript::RunProgramByName("TM12_MissionFailure", MPartRef());
	}
	else if ( part == data.Vivac )
	{
        MScript::EndStream( data.mhandle );

        data.mhandle = MScript::PlayAnimatedMessage( "M12ST53.wav", "Animate!!Steele2", 
            MAIN_MAGPIE_X_POS, MAIN_MAGPIE_Y_POS, data.Steele, IDS_TM12_SUB_M12ST53 );

    	MScript::MoveCamera( part, 0, MOVIE_CAMERA_JUMP_TO);

		data.failureID = IDS_TM12_FAIL_VIVAC_LOST;

		MScript::RunProgramByName("TM12_MissionFailure", MPartRef());
	}
    else if ( part == data.mainAcropolis )
    {
        MScript::EndStream( data.mhandle );

        data.mhandle = MScript::PlayAnimatedMessage( "M12BN58.wav", "Animate!!Benson2", 
            MAIN_MAGPIE_X_POS, MAIN_MAGPIE_Y_POS, data.Benson, IDS_TM12_SUB_M12BN58 );
	
    	MScript::MoveCamera( part, 0, MOVIE_CAMERA_JUMP_TO);

		MScript::RunProgramByName("TM12_MissionFailure", MPartRef());
    }
}

bool TM12_ObjectDestroyed::Update (void)
{
	return false;
}

//---------------------------------------------------------------------------
// MISSION FAILURE PROGRAMS


//---------------------------------------------------------------------------

CQSCRIPTPROGRAM(TM12_MissionFailure, TM12_MissionFailure_Save, 0);

void TM12_MissionFailure::Initialize (U32 eventFlags, const MPartRef & part)
{
	UnderMissionControl();	

	MScript::EnableMovieMode( true );

	data.mission_over = true;
	state = Uplink;
}

bool TM12_MissionFailure::Update (void)
{
	switch (state)
	{
		case Uplink:

			if ( !MScript::IsStreamPlaying( data.mhandle ) )
			{
				data.mhandle = MScript::PlayAudio("high_level_data_uplink.wav");
				state = Halsey;
			}

			break;

		case Halsey:

			if ( !MScript::IsStreamPlaying(data.mhandle) )
			{
                data.mhandle = MScript::PlayAnimatedMessage( "M12HA54.wav", "Animate!!Halsey2", 
                    MAIN_MAGPIE_X_POS, MAIN_MAGPIE_Y_POS, IDS_TM12_SUB_M12HA54 );
	
				state = Fail;
			}

			break;

		case Fail:

			if ( !MScript::IsStreamPlaying(data.mhandle) )
			{
				MScript::FlushTeletype();

                data.thandle = TeletypeMissionOver( IDS_TM12_MISSION_FAILURE, data.failureID );

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
// MISSION SUCCESS

CQSCRIPTPROGRAM(TM12_MissionSuccess, TM12_MissionSuccess_Save, 0);

void TM12_MissionSuccess::Initialize (U32 eventFlags, const MPartRef & part)
{
	UnderMissionControl();	

	MScript::EnableMovieMode( true );

    MScript::MoveCamera( MScript::GetPartByName( "Gate0" ), 0, MOVIE_CAMERA_JUMP_TO);

	data.mission_over = true;

	state = PROG_STATE_NEWS;

}

bool TM12_MissionSuccess::Update (void)
{
	switch (state)
	{
        case PROG_STATE_NEWS:

			if ( !MScript::IsStreamPlaying( data.mhandle ) )
			{
                data.mhandle = MScript::PlayAudio( "M12NP49.wav", IDS_TM12_SUB_M12NP49 );

                state = PROG_STATE_HAWKES_1;
			}

			break;

        case PROG_STATE_HAWKES_1:

			if ( !MScript::IsStreamPlaying( data.mhandle ) )
			{
                data.mhandle = MScript::PlayAnimatedMessage( "M12HW46.wav", "Animate!!Hawkes2", 
                    MAIN_MAGPIE_X_POS, MAIN_MAGPIE_Y_POS, data.Hawkes, IDS_TM12_SUB_M12HW46 );

                state = PROG_STATE_BLACKWELL_1;
			}

			break;

        case PROG_STATE_BLACKWELL_1:

			if ( !MScript::IsStreamPlaying( data.mhandle ) )
			{
                data.mhandle = MScript::PlayAnimatedMessage( "M12BL47.wav", "Animate!!Blackwell2", 
                    MAIN_MAGPIE_X_POS, MAIN_MAGPIE_Y_POS, data.Blackwell, IDS_TM12_SUB_M12BL47 );

                state = PROG_STATE_HAWKES_2;
			}

			break;

        case PROG_STATE_HAWKES_2:

			if ( !MScript::IsStreamPlaying( data.mhandle ) )
			{
                data.mhandle = MScript::PlayAnimatedMessage( "M12HW48.wav", "Animate!!Hawkes2", 
                    MAIN_MAGPIE_X_POS, MAIN_MAGPIE_Y_POS, data.Hawkes, IDS_TM12_SUB_M12HW48 );

                state = PROG_STATE_UPLINK;
			}

			break;

        case PROG_STATE_UPLINK:

			if ( !MScript::IsStreamPlaying( data.mhandle ) )
			{
                data.mhandle = MScript::PlayAudio( "high_level_data_uplink.wav" );

                state = PROG_STATE_HALSEY;
			}

			break;

        case PROG_STATE_HALSEY:

			if ( !MScript::IsStreamPlaying( data.mhandle ) )
			{
                data.mhandle = MScript::PlayAnimatedMessage( "M12HA50.wav", "Animate!!Halsey2", 
                    MagpieLeft, MagpieTop, IDS_TM12_SUB_M12HA50 );

                state = PROG_STATE_TELETYPE;
			}

			break;

        case PROG_STATE_TELETYPE:

			if ( !MScript::IsStreamPlaying( data.mhandle ) )
			{
			    MScript::FlushTeletype();

                data.thandle = TeletypeMissionOver( IDS_TM12_MISSION_SUCCESS );

                state = PROG_STATE_DONE;
			}

			break;

		case PROG_STATE_DONE:

		    if ( !MScript::IsTeletypePlaying( data.thandle ) )
		    {
			    UnderPlayerControl();

			    MScript::EndMissionVictory( 11 );

			    return false;
            }

			break;

        default:

            break;
	}

	return true;
}

//--------------------------------------------------------------------------//
//	OTHER PROGRAMS
//--------------------------------------------------------------------------//

static void SetupAI
(
    bool bOn
)
{

	MScript::EnableEnemyAI(MANTIS_ID, bOn, "MANTIS_FRIGATE_RUSH");

	if (bOn)
	{
		AIPersonality airules;
		airules.difficulty = MEDIUM;
		airules.buildMask.bBuildPlatforms = true;
		airules.buildMask.bBuildLightGunboats = true;
		airules.buildMask.bBuildMediumGunboats = true;
		airules.buildMask.bBuildHeavyGunboats = true;
		airules.buildMask.bHarvest = true;
		airules.buildMask.bScout = true;
		airules.buildMask.bBuildAdmirals = false;
		airules.buildMask.bLaunchOffensives = true;
		airules.nGunboatsPerSupplyShip = 5;
		airules.nNumMinelayers = 2;
		airules.uNumScouts = 3;
		airules.nNumFabricators = 3;

		MScript::SetEnemyAIRules(MANTIS_ID, airules);
	}

}

static void ToggleBuildUnits
(
    bool bOn
)
{

	AIPersonality airules;

	airules.buildMask.bBuildLightGunboats = bOn;
	airules.buildMask.bBuildMediumGunboats = bOn;
	airules.buildMask.bBuildHeavyGunboats = bOn;

	MScript::SetEnemyAIRules(MANTIS_ID, airules);
}

