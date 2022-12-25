//--------------------------------------------------------------------------//
//                                                                          //
//                                Script00T.cpp                             //
//                                                                          //
//               COPYRIGHT (C) 2000 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
   Celareon Training Mission 

   $Header: /Conquest/App/Src/Scripts/Sol_T/Sol_T.cpp 92    7/06/01 11:07a Tmauer $

*/
//--------------------------------------------------------------------------//

#include <ScriptDef.h>
#include <DLLScriptMain.h>
#include "DataDef.h"
#include "Local.h"
#include "stdlib.h"

#include "..\helper\helper.h"

//--------------------------------------------------------------------------//
//  Solarian Training #define's																	//
//--------------------------------------------------------------------------//
#define PLAYER_ID			1
#define MANTIS_ID			2

#define SOLAR				2
#define KASSE				1

//--------------------------------------------------------------------------//
//  FUNCTION PROTOTYPES
//--------------------------------------------------------------------------//

static void AddAndDisplayObjective
(
    U32 stringID,
    U32 dependantHandle,
    bool isSecondary = false
);
//---------------------------------------------------------------------------//
CQSCRIPTDATA(MissionData, data);
//--------------------------------------------------------------------------//
// Mission functions: Solarian Training Campaign 
//--------------------------------------------------------------------------//

//--------------------------------------------------------------------------
//	MISSION BRIEFING
//--------------------------------------------------------------------------

CQSCRIPTPROGRAM(SolTrain_Briefing, SolTrain_Briefing_Save,CQPROGFLAG_STARTBRIEFING);

void SolTrain_Briefing::Initialize (U32 eventFlags, const MPartRef & part)
{	   
	data.mission_over = false;
	data.mhandle = 0;
	data.shandle = 0;
	data.mission_state = Briefing;
	data.displayObjectiveHandlerParams_teletypeHandle = 0;
	state = BRIEFING_STATE_RADIO_BROADCAST;
	MScript:: EnableBriefingControls(true);
	MScript::PlayMusic( NULL );
}

bool SolTrain_Briefing::Update (void)
{
	CQBRIEFINGITEM slotAnim;

		switch(state)
		{
			case BRIEFING_STATE_RADIO_BROADCAST:

				data.mhandle = MScript::PlayAudio( "prolog18.wav" );
				MScript::BriefingSubtitle(data.mhandle,IDS_SOL_SUB_PROLOGSOL);
				
				ShowBriefingAnimation( 0, "TechLoop1", 100, true, true );
				ShowBriefingAnimation( 1, "TechLoop2", 100, true, true );
				ShowBriefingAnimation( 2, "TechLoop3", 100, true, true );
				ShowBriefingAnimation( 3, "TechLoop1", 100, true, true );

				MScript::FlushTeletype();
				data.thandle = MScript::PlayBriefingTeletype( IDS_SOLTRAIN_TELETYPE_LOCATION, 
					STANDARD_TELETYPE_COLOR, STANDARD_TELETYPE_HOLD_TIME, 
				MScript::GetScriptStringLength( IDS_SOLTRAIN_TELETYPE_LOCATION ) * STANDARD_TELETYPE_MILS_PER_CHAR, 
					false );
				
				state = BRIEFING_STATE_ELAN_BRIEF;

			break;


			case BRIEFING_STATE_ELAN_BRIEF:

					if (!MScript::IsStreamPlaying( data.mhandle ))
					{
						strcpy( slotAnim.szFileName, "stmg02.wav" );
						strcpy( slotAnim.szTypeName, "Animate!!Elan" );
						slotAnim.slotID = 0;
						slotAnim.bHighlite = false;
						data.mhandle = MScript::PlayBriefingTalkingHead( slotAnim );
						MScript::BriefingSubtitle(data.mhandle,IDS_SOL_SUB_STMG02);
						
				
						state = BRIEFING_STATE_OBJECTIVES;
					}

                    break;

			case BRIEFING_STATE_OBJECTIVES:

				if ( !MScript::IsStreamPlaying( data.mhandle ) )
					{
            
						data.thandle = MScript::PlayBriefingTeletype( IDS_SOLTRAIN_OBJECTIVES,
							STANDARD_TELETYPE_COLOR, 0, 
						MScript::GetScriptStringLength( IDS_SOLTRAIN_OBJECTIVES ) * STANDARD_TELETYPE_MILS_PER_CHAR, 
							false );
						ShowBriefingAnimation( 0, "TechLoop1", 100, true, true );
						
						state = BRIEFING_STATE_FINISHED;
					}

			break;

			case BRIEFING_STATE_FINISHED:			

			break;			
			}

return true;
}
//--------------------------------------------------------------------------//
//  MISSION PROGRAM 														//
//--------------------------------------------------------------------------//

CQSCRIPTPROGRAM(SolTrain_Start, SolTrain_Start_Save, CQPROGFLAG_STARTMISSION);

void SolTrain_Start::Initialize (U32 eventFlags, const MPartRef & part)
{
    MScript::SetMissionID( 0 );
	data.mission_tech.InitLevel(TECHTREE::FULL_TREE);
	data.mission_tech.race[2].build = (TECHTREE::BUILDNODE)	(TECHTREE::SDEPEND_ACROPOLIS | TECHTREE::RES_REFINERY_GAS1 |
		TECHTREE::RES_REFINERY_GAS2 | TECHTREE::RES_REFINERY_GAS3 |	TECHTREE::RES_REFINERY_METAL1 |
		TECHTREE::RES_REFINERY_METAL2 | TECHTREE::RES_REFINERY_METAL3 );

	data.mission_state = Begin;
	
/*	data.mission_tech.race[0].common_extra  = (TECHTREE::COMMON_EXTRA) ( TECHTREE::RES_SENSORS1 |
		TECHTREE::RES_SENSORS2 | TECHTREE::RES_SENSORS3 |  TECHTREE::RES_SENSORS4 |  TECHTREE::RES_SENSORS5 | TECHTREE::RES_TANKER1  |
		TECHTREE::RES_TANKER2  |  TECHTREE::RES_TENDER1 |  TECHTREE::RES_TENDER2 );
*/
	MScript::SetAvailiableTech( data.mission_tech );
	
	MScript::EnableRegenMode (true);
	MScript::EnableJumpgate( MScript::GetPartByName( "Wormhole to Kasse (Solar)" ), false );

	//Init the Sol trigger
	data.TrainTrigger1 = MScript:: CreatePart("MISSION!!TRIGGER",MScript:: GetPartByName("TriggerPoint"), 0);
	data.TrainTrigger2 = MScript:: CreatePart("MISSION!!TRIGGER",MScript:: GetPartByName("TriggerPt"), 0);
	data.TrainTrigger3 = MScript:: CreatePart("MISSION!!TRIGGER",MScript:: GetPartByName("Bunker1"), 0);
	data.TrainTrigger4 = MScript:: CreatePart("MISSION!!TRIGGER",MScript:: GetPartByName("Bunker2"), 0);
	
	MScript::EnableTrigger(data.TrainTrigger1, false);
	MScript::EnableTrigger(data.TrainTrigger2, false);
	MScript::EnableTrigger(data.TrainTrigger3, false);
	MScript::EnableTrigger(data.TrainTrigger4, false);
	
	MScript::SetTriggerRange(data.TrainTrigger1, 100);
	MScript::SetTriggerRange(data.TrainTrigger2, 100);
	MScript::SetTriggerRange(data.TrainTrigger3, 4);
	MScript::SetTriggerRange(data.TrainTrigger4, 4);
	
	data.WarnTrigger = MScript:: CreatePart("MISSION!!TRIGGER",MScript:: GetPartByName("TriggerPoint"), 0);
	
	MScript::EnableTrigger(data.WarnTrigger, false);
	MScript::SetTriggerRange(data.WarnTrigger, 100);
	//set resources for player...
	MScript::SetGas(PLAYER_ID, 200);
	MScript::SetMetal(PLAYER_ID, 200);
	MScript::SetCrew(PLAYER_ID, 20);
	MScript::SetMaxCrew(PLAYER_ID, 300);
	MScript::SetMaxGas(PLAYER_ID, 500);
	MScript::SetMaxMetal(PLAYER_ID, 500);

	data.Forger = MScript:: GetPartByName("Forger");
	data.Taos1 = MScript:: GetPartByName("Taos1");
	data.Taos2 = MScript:: GetPartByName("Taos2");
	data.Acropolis = MScript:: GetPartByName("Acropolis");

	MScript::EnableSelection( data.Taos1, false );
	MScript::EnableSelection( data.Taos2, false );
	MScript::EnableSelection( data.Acropolis, false );

    MScript::PlayMusic( "solarian_game14.wav" );

	//MScript::EnableJumpgate( MScript::GetPartByName( "Wormhole to Kasse (Solar)" ), false );

	MScript::EndStream( data.mhandle );
	data.mhandle = MScript::PlayAnimatedMessage( "STMG03.wav", "Animate!!Elan2", 
		MagpieLeft, MagpieTop, IDS_SOL_SUB_STMG03 );

	AddAndDisplayObjective( IDS_SOLTRAIN_OBJECTIVE1, data.mhandle, true );
	MScript:: RunProgramByName("SolTrain_SelectAndMoveForger", MPartRef());

	MScript:: RunProgramByName("SolTrain_CheckForMissionFail", MPartRef());
}

bool SolTrain_Start::Update (void)
{
		if (data.mission_over)
	{
		// Check to see if mission has ended. If so don't bother anymore...

		return false;
	}

	return true;
}

//----------------------------------------------------------------------------------------------------
//

CQSCRIPTPROGRAM(SolTrain_SelectAndMoveForger, SolTrain_SelectAndMoveForger_Save, 0);

void SolTrain_SelectAndMoveForger::Initialize (U32 eventFlags, const MPartRef & part)
{	
	state = Begin;
}

bool SolTrain_SelectAndMoveForger::Update (void)
{	
	if (data.mission_over && state != MissionFail )
	{
		// Check to see if mission has ended. If so don't bother anymore...

		return false;
	}

	switch(state)
	{
			case Begin:
			
				if(MScript:: IsSelectedUnique(data.Forger))
				{	
					MScript::MarkObjectiveCompleted( IDS_SOLTRAIN_OBJECTIVE1);
					MScript::EndStream( data.mhandle );
	    			data.mhandle = MScript::PlayAnimatedMessage( "STMG04.wav", "Animate!!Elan2", 
						MagpieLeft, MagpieTop, IDS_SOL_SUB_STMG04 );

					AddAndDisplayObjective (IDS_SOLTRAIN_OBJECTIVE2 , data.mhandle, true );
					
					state = MoveForger;
				}
			
			break;


			case MoveForger:
			
				if( !data.Forger.isValid() )
				{	
					data.mission_over = true;

					MScript::EnableMovieMode(true);
					UnderMissionControl();

                	MScript::FlushStreams();

					data.mhandle = MScript::PlayAnimatedMessage( "STMG39.wav", "Animate!!Elan2", 
						MagpieLeft, MagpieTop, IDS_SOL_SUB_STMG39);
					
					state = MissionFail;
				}	

				if(!MScript:: IsIdle(data.Forger))
				{	
					MScript::MarkObjectiveCompleted( IDS_SOLTRAIN_OBJECTIVE2);
					MScript::EndStream( data.mhandle );
					data.mhandle = MScript::PlayAnimatedMessage( "STMG05.wav", "Animate!!Elan2", 
						MagpieLeft, MagpieTop, IDS_SOL_SUB_STMG05 );
					AddAndDisplayObjective (IDS_SOLTRAIN_OBJECTIVE3 , data.mhandle, true );
					MScript::EnableSelection( data.Taos1, true );
					MScript::EnableSelection( data.Taos2, true );
					
					state = SelectAll;
				}
			
			break;

			case SelectAll:
				
				if( !data.Forger.isValid() || !data.Taos1.isValid() || !data.Taos2.isValid() )
				{	
					data.mission_over = true;

					MScript::EnableMovieMode(true);
					UnderMissionControl();

                	MScript::FlushStreams();

					data.mhandle = MScript::PlayAnimatedMessage( "STMG39.wav", "Animate!!Elan2", 
						MagpieLeft, MagpieTop, IDS_SOL_SUB_STMG39);
					
					state = MissionFail;
				}	

				if(!MScript:: IsStreamPlaying(data.mhandle) && MScript:: IsSelected(data.Forger) && MScript:: IsSelected(data.Taos1) && MScript:: IsSelected(data.Taos2))
				{	
					MScript::MarkObjectiveCompleted( IDS_SOLTRAIN_OBJECTIVE3);
					MScript::EndStream(data.mhandle);
					data.mhandle = MScript::PlayAnimatedMessage( "STMG06.wav", "Animate!!Elan2", 
						MagpieLeft, MagpieTop, IDS_SOL_SUB_STMG06 );
					AddAndDisplayObjective (IDS_SOLTRAIN_OBJECTIVE4 , data.mhandle, true);

					MScript::EnableSelection( data.Acropolis, true );
					
					state = FOWuncovered;
				}
			
			break;

			case FOWuncovered:
				if(!MScript::IsStreamPlaying (data.mhandle) &&
						( MScript::IsVisibleToPlayer(MScript::GetPartByName("Solar 2"), PLAYER_ID) ||
						MScript::IsVisibleToPlayer(MScript::GetPartByName("Solar 3"), PLAYER_ID) ||
						MScript::IsVisibleToPlayer(MScript::GetPartByName("Solar 4"), PLAYER_ID) ) )
				{	
					MScript::MarkObjectiveCompleted( IDS_SOLTRAIN_OBJECTIVE4);
					MScript::EndStream(data.mhandle);
					data.mhandle = MScript::PlayAnimatedMessage( "STMG07.wav", "Animate!!Elan2", 
						MagpieLeft, MagpieTop, IDS_SOL_SUB_STMG07 );
					AddAndDisplayObjective (IDS_SOLTRAIN_OBJECTIVE5, data.mhandle, true);
					data.mission_tech.race[2].build = TECHTREE::BUILDNODE( data.mission_tech.race[2].build | 
						TECHTREE::SDEPEND_OXIDATOR );
					MScript::SetAvailiableTech( data.mission_tech );
					
					state = StartBuild;
			}
			
			break;

			case StartBuild:
		//	if(!MScript:: IsStreamPlaying(data.mhandle))
								
				{	MScript:: EnableTrigger(data.TrainTrigger1, true);
					//setup trigger to detect when OXIDATOR is started
					MScript:: SetTriggerProgram(data.TrainTrigger1, "SolTrain_OxiInProgress");	
					// Set trigger to detect a OXIDATOR in process or otherwise from player 1.
					MScript:: SetTriggerFilter(data.TrainTrigger1, M_OXIDATOR, TRIGGER_MOBJCLASS, false);
					MScript:: SetTriggerFilter(data.TrainTrigger1, 0, TRIGGER_NOFORCEREADY, true);
					MScript:: SetTriggerFilter(data.TrainTrigger1, PLAYER_ID, TRIGGER_PLAYER, true);
					return false;
				}
			
			case MissionFail:

				if (!MScript::IsStreamPlaying( data.mhandle ) )
				{	
					MScript::EnableMovieMode(true);
					UnderPlayerControl();
					MScript::FlushTeletype();
					data.thandle = TeletypeMissionOver(IDS_SOLTRAIN_MISSION_FAILURE);

					MScript::EndMissionDefeat();

					return false;
				}
			
			break;

	}
	return true;
}

//--------------------------------------------------------------------------//
//	Called by the trigger when the user starts building a Oxidator 

CQSCRIPTPROGRAM(SolTrain_OxiInProgress, SolTrain_OxiInProgress_Save,0);

void SolTrain_OxiInProgress::Initialize (U32 eventFlags, const MPartRef & part)
{
	//setup trigger to detect when OXIDATOR is finished
	MScript:: SetTriggerProgram(data.TrainTrigger1, "SolTrain_OxiFinished");
	// detect a OXIDATOR finished player 1.
	MScript:: SetTriggerFilter(data.TrainTrigger1, M_OXIDATOR, TRIGGER_MOBJCLASS, false);
	MScript:: SetTriggerFilter(data.TrainTrigger1, 0, TRIGGER_FORCEREADY, true);
	MScript:: SetTriggerFilter(data.TrainTrigger1, PLAYER_ID, TRIGGER_PLAYER, true);
	data.Oxidator = MScript::GetLastTriggerObject(part);

	MScript::EndStream(data.mhandle);
	data.mhandle = MScript::PlayAnimatedMessage( "STMG08.wav", "Animate!!Elan2", 
		MagpieLeft, MagpieTop, IDS_SOL_SUB_STMG08 );
}

bool SolTrain_OxiInProgress::Update (void)
{	
		if (data.mission_over)
	{
		// Check to see if mission has ended. If so don't bother anymore...

		return false;
	}

	return true;
}

//--------------------------------------------------------------------------//
//	

CQSCRIPTPROGRAM(SolTrain_OxiFinished, SolTrain_OxiFinished_Save,0);

void SolTrain_OxiFinished::Initialize (U32 eventFlags, const MPartRef & part)
{
	//setup trigger to detect when Oxidator is finished
	MScript:: SetTriggerProgram(data.TrainTrigger1, "SolTrain_GaliotInProgress");
	// detect a HQ finished player 1.
	MScript:: SetTriggerFilter(data.TrainTrigger1, M_GALIOT, TRIGGER_MOBJCLASS, false);
	MScript:: SetTriggerFilter(data.TrainTrigger1, 0, TRIGGER_NOFORCEREADY, true);
	MScript:: SetTriggerFilter(data.TrainTrigger1, PLAYER_ID, TRIGGER_PLAYER, true);

	MScript::EndStream(data.mhandle);
	data.mhandle = MScript::PlayAnimatedMessage( "STMG09.wav", "Animate!!Elan2", 
		MagpieLeft, MagpieTop, IDS_SOL_SUB_STMG09 );
	
	AddAndDisplayObjective( IDS_SOLTRAIN_OBJECTIVE6, data.mhandle, true );
	MScript::MarkObjectiveCompleted( IDS_SOLTRAIN_OBJECTIVE5);

}

bool SolTrain_OxiFinished::Update (void)
{	
	if (data.mission_over)
	{
		// Check to see if mission has ended. If so don't bother anymore...

		return false;
	}
	
	return true;
}

//--------------------------------------------------------------------------//
//	 

CQSCRIPTPROGRAM(SolTrain_GaliotInProgress, SolTrain_GaliotInProgress_Save,0);

void SolTrain_GaliotInProgress::Initialize (U32 eventFlags, const MPartRef & part)
{
	//setup trigger to detect when HQ is finished
	MScript:: SetTriggerProgram(data.TrainTrigger1, "SolTrain_GaliotFinished");
	// detect a HQ finished player 1.
	MScript:: SetTriggerFilter(data.TrainTrigger1, M_GALIOT, TRIGGER_MOBJCLASS, false);
	MScript:: SetTriggerFilter(data.TrainTrigger1, 0, TRIGGER_FORCEREADY, true);
	MScript:: SetTriggerFilter(data.TrainTrigger1, PLAYER_ID, TRIGGER_PLAYER, true);
 
	MScript::EndStream(data.mhandle);
	data.mhandle = MScript::PlayAnimatedMessage( "STMG10.wav", "Animate!!Elan2", 
	    MagpieLeft, MagpieTop, IDS_SOL_SUB_STMG10 );

	MScript::EnableTrigger(data.TrainTrigger3, true);
	MScript::EnableTrigger(data.TrainTrigger4, true);
	
}

bool SolTrain_GaliotInProgress::Update (void)
{
	if (data.mission_over)
	{
		// Check to see if mission has ended. If so don't bother anymore...

		return false;
	}
	return true;
}

//--------------------------------------------------------------------------//
//	 

CQSCRIPTPROGRAM(SolTrain_GaliotFinished, SolTrain_GaliotFinished_Save,0);

void SolTrain_GaliotFinished::Initialize (U32 eventFlags, const MPartRef & part)
{	
	data.mission_tech.race[2].build = TECHTREE::BUILDNODE( data.mission_tech.race[2].build | 
		TECHTREE::SDEPEND_BUNKER );
	MScript::EnableTrigger(data.TrainTrigger1, false);
	MScript::SetAvailiableTech( data.mission_tech );

	//setup trigger to detect when HQ is finished
	MScript:: SetTriggerProgram(data.TrainTrigger3, "SolTrain_BunkerInProgress");
	// detect a HQ finished player 1.
	MScript:: SetTriggerFilter(data.TrainTrigger3, M_BUNKER, TRIGGER_MOBJCLASS, false);
	MScript:: SetTriggerFilter(data.TrainTrigger3, 0, TRIGGER_NOFORCEREADY, true);
	MScript:: SetTriggerFilter(data.TrainTrigger3, PLAYER_ID, TRIGGER_PLAYER, true);

	MScript:: SetTriggerProgram(data.TrainTrigger4, "SolTrain_BunkerInProgress");
	// detect a HQ finished player 1.
	MScript:: SetTriggerFilter(data.TrainTrigger4, M_BUNKER, TRIGGER_MOBJCLASS, false);
	MScript:: SetTriggerFilter(data.TrainTrigger4, 0, TRIGGER_NOFORCEREADY, true);
	MScript:: SetTriggerFilter(data.TrainTrigger4, PLAYER_ID, TRIGGER_PLAYER, true);

	MScript::MarkObjectiveCompleted( IDS_SOLTRAIN_OBJECTIVE6 );
	MScript::EndStream(data.mhandle);
	data.mhandle = MScript::PlayAnimatedMessage( "STMG11.wav", "Animate!!Elan2", 
	    MagpieLeft, MagpieTop, IDS_SOL_SUB_STMG11 );
	
	MScript::RemoveFromObjectiveList( IDS_SOLTRAIN_OBJECTIVE1 );
    MScript::RemoveFromObjectiveList( IDS_SOLTRAIN_OBJECTIVE2 );
	MScript::RemoveFromObjectiveList( IDS_SOLTRAIN_OBJECTIVE3 );
    MScript::RemoveFromObjectiveList( IDS_SOLTRAIN_OBJECTIVE4 );
	
	AddAndDisplayObjective( IDS_SOLTRAIN_OBJECTIVE7, data.mhandle, true );

}

bool SolTrain_GaliotFinished::Update (void)
{
	if (data.mission_over)
	{
		// Check to see if mission has ended. If so don't bother anymore...

		return false;
	}

	return true;
}

//--------------------------------------------------------------------------//
//	 

CQSCRIPTPROGRAM(SolTrain_BunkerInProgress, SolTrain_BunkerInProgress_Save,0);

void SolTrain_BunkerInProgress::Initialize (U32 eventFlags, const MPartRef & part)
{	
	//setup trigger to detect when HQ is finished
	MScript:: SetTriggerProgram(data.TrainTrigger3, "SolTrain_BunkerFinished");
	// detect a HQ finished player 1.
	MScript:: SetTriggerFilter(data.TrainTrigger3, M_BUNKER, TRIGGER_MOBJCLASS, false);
	MScript:: SetTriggerFilter(data.TrainTrigger3, 0, TRIGGER_FORCEREADY, true);
	MScript:: SetTriggerFilter(data.TrainTrigger3, PLAYER_ID, TRIGGER_PLAYER, true);

	MScript:: SetTriggerProgram(data.TrainTrigger4, "SolTrain_BunkerFinished");
	// detect a HQ finished player 1.
	MScript:: SetTriggerFilter(data.TrainTrigger4, M_BUNKER, TRIGGER_MOBJCLASS, false);
	MScript:: SetTriggerFilter(data.TrainTrigger4, 0, TRIGGER_FORCEREADY, true);
	MScript:: SetTriggerFilter(data.TrainTrigger4, PLAYER_ID, TRIGGER_PLAYER, true);

	MScript::EndStream(data.mhandle);
	data.mhandle = MScript::PlayAnimatedMessage( "STMG12.wav", "Animate!!Elan2", 
		MagpieLeft, MagpieTop, IDS_SOL_SUB_STMG12 );
	
}

bool SolTrain_BunkerInProgress::Update (void)
{
	if (data.mission_over)
	{
		// Check to see if mission has ended. If so don't bother anymore...

		return false;
	}
	
	return true;
}


//--------------------------------------------------------------------------//
//	 

CQSCRIPTPROGRAM(SolTrain_BunkerFinished, SolTrain_BunkerFinished_Save,0);

void SolTrain_BunkerFinished::Initialize (U32 eventFlags, const MPartRef & part)
{	
	MScript::EnableTrigger(data.TrainTrigger1, true);
	MScript::EnableTrigger(data.TrainTrigger3, false);
	MScript::EnableTrigger(data.TrainTrigger4, false);
	
	data.mission_tech.race[2].build = TECHTREE::BUILDNODE( data.mission_tech.race[2].build | 
		TECHTREE::SDEPEND_SENTINALTOWER );
	MScript::SetAvailiableTech( data.mission_tech );

	//setup trigger to detect when HQ is finished
	MScript:: SetTriggerProgram(data.TrainTrigger1, "SolTrain_STowerInProgress");
	// detect a HQ finished player 1.
	MScript:: SetTriggerFilter(data.TrainTrigger1, M_SENTINELTOWER, TRIGGER_MOBJCLASS, false);
	MScript:: SetTriggerFilter(data.TrainTrigger1, 0, TRIGGER_NOFORCEREADY, true);
	MScript:: SetTriggerFilter(data.TrainTrigger1, PLAYER_ID, TRIGGER_PLAYER, true);

	MScript::MarkObjectiveCompleted( IDS_SOLTRAIN_OBJECTIVE7 );
 	MScript::EndStream(data.mhandle);
	data.mhandle = MScript::PlayAnimatedMessage( "STMG13.wav", "Animate!!Elan2", 
	    MagpieLeft, MagpieTop, IDS_SOL_SUB_STMG13 );
	
	AddAndDisplayObjective( IDS_SOLTRAIN_OBJECTIVE8, data.mhandle, true );
	
}

bool SolTrain_BunkerFinished::Update (void)
{
	if (data.mission_over)
	{
		// Check to see if mission has ended. If so don't bother anymore...

		return false;
	}
	
	return true;
}

//--------------------------------------------------------------------------//
//	 

CQSCRIPTPROGRAM(SolTrain_STowerInProgress, SolTrain_STowerInProgress_Save,0);

void SolTrain_STowerInProgress::Initialize (U32 eventFlags, const MPartRef & part)
{
	//setup trigger to detect when HQ is finished
	MScript:: SetTriggerProgram(data.TrainTrigger1, "SolTrain_STowerFinished");
	// detect a HQ finished player 1.
	MScript:: SetTriggerFilter(data.TrainTrigger1, M_SENTINELTOWER, TRIGGER_MOBJCLASS, false);
	MScript:: SetTriggerFilter(data.TrainTrigger1, 0, TRIGGER_FORCEREADY, true);
	MScript:: SetTriggerFilter(data.TrainTrigger1, PLAYER_ID, TRIGGER_PLAYER, true);

	MScript::EndStream(data.mhandle);
	data.mhandle = MScript::PlayAnimatedMessage( "STMG14.wav", "Animate!!Elan2", 
	    MagpieLeft, MagpieTop, IDS_SOL_SUB_STMG14 );
	
}

bool SolTrain_STowerInProgress::Update (void)
{
	if (data.mission_over)
	{
		// Check to see if mission has ended. If so don't bother anymore...

		return false;
	}
	
	return true;
}


//--------------------------------------------------------------------------//
//	 

CQSCRIPTPROGRAM(SolTrain_STowerFinished, SolTrain_STowerFinished_Save,0);

void SolTrain_STowerFinished::Initialize (U32 eventFlags, const MPartRef & part)
{	
	data.mission_tech.race[2].build = TECHTREE::BUILDNODE( data.mission_tech.race[2].build | 
		TECHTREE::SDEPEND_PAVILION );
	MScript::SetAvailiableTech( data.mission_tech );
	
	//setup trigger to detect when HQ is finished
	MScript:: SetTriggerProgram(data.TrainTrigger1, "SolTrain_PavilionInProgress");
	// detect a HQ finished player 1.
	MScript:: SetTriggerFilter(data.TrainTrigger1, M_PAVILION, TRIGGER_MOBJCLASS, false);
	MScript:: SetTriggerFilter(data.TrainTrigger1, 0, TRIGGER_NOFORCEREADY, true);
	MScript:: SetTriggerFilter(data.TrainTrigger1, PLAYER_ID, TRIGGER_PLAYER, true);
 
	MScript::MarkObjectiveCompleted( IDS_SOLTRAIN_OBJECTIVE8 );
 
	MScript::EndStream(data.mhandle);
	data.mhandle = MScript::PlayAnimatedMessage( "STMG15.wav", "Animate!!Elan2", 
	    MagpieLeft, MagpieTop, IDS_SOL_SUB_STMG15 );

	AddAndDisplayObjective( IDS_SOLTRAIN_OBJECTIVE9, data.mhandle, true );
}

bool SolTrain_STowerFinished::Update (void)
{	
	if (data.mission_over)
	{
		// Check to see if mission has ended. If so don't bother anymore...

		return false;
	}
	
	return true;
}

//--------------------------------------------------------------------------//
//	 

CQSCRIPTPROGRAM(SolTrain_PavilionInProgress, SolTrain_PavilionInProgress_Save,0);

void SolTrain_PavilionInProgress::Initialize (U32 eventFlags, const MPartRef & part)
{
	//setup trigger to detect when HQ is finished
	MScript:: SetTriggerProgram(data.TrainTrigger1, "SolTrain_PavilionFinished");
	// detect a HQ finished player 1.
	MScript:: SetTriggerFilter(data.TrainTrigger1, M_PAVILION, TRIGGER_MOBJCLASS, false);
	MScript:: SetTriggerFilter(data.TrainTrigger1, 0, TRIGGER_FORCEREADY, true);
	MScript:: SetTriggerFilter(data.TrainTrigger1, PLAYER_ID, TRIGGER_PLAYER, true);
 
	MScript::EndStream(data.mhandle);
	data.mhandle = MScript::PlayAnimatedMessage( "STMG16.wav", "Animate!!Elan2", 
	    MagpieLeft, MagpieTop, IDS_SOL_SUB_STMG16 );
}

bool SolTrain_PavilionInProgress::Update (void)
{	
	if (data.mission_over)
	{
		// Check to see if mission has ended. If so don't bother anymore...

		return false;
	}
	
	return true;
}


//--------------------------------------------------------------------------//
//	 

CQSCRIPTPROGRAM(SolTrain_PavilionFinished, SolTrain_PavilionFinished_Save,0);

void SolTrain_PavilionFinished::Initialize (U32 eventFlags, const MPartRef & part)
{
	data.mission_tech.race[2].build = TECHTREE::BUILDNODE( data.mission_tech.race[2].build | 
		TECHTREE::SDEPEND_HELIONVIEL );
	MScript::SetAvailiableTech( data.mission_tech );

	MScript:: SetTriggerProgram(data.TrainTrigger1, "SolTrain_HelionInProgress");
	// detect a HQ finished player 1.
	MScript:: SetTriggerFilter(data.TrainTrigger1, M_HELIONVEIL, TRIGGER_MOBJCLASS, false);
	MScript:: SetTriggerFilter(data.TrainTrigger1, 0, TRIGGER_NOFORCEREADY, true);
	MScript:: SetTriggerFilter(data.TrainTrigger1, PLAYER_ID, TRIGGER_PLAYER, true);
	
	MScript::RemoveFromObjectiveList( IDS_SOLTRAIN_OBJECTIVE5 );
    MScript::RemoveFromObjectiveList( IDS_SOLTRAIN_OBJECTIVE6 );

	MScript::MarkObjectiveCompleted( IDS_SOLTRAIN_OBJECTIVE9 );
 
	MScript::EndStream(data.mhandle);
	data.mhandle = MScript::PlayAnimatedMessage( "STMG17.wav", "Animate!!Elan2", 
	    MagpieLeft, MagpieTop, IDS_SOL_SUB_STMG17 );
	
	AddAndDisplayObjective( IDS_SOLTRAIN_OBJECTIVE10, data.mhandle, true );
	MScript::RemoveFromObjectiveList( IDS_SOLTRAIN_OBJECTIVE7 );

}

bool SolTrain_PavilionFinished::Update (void)
{	
	if (data.mission_over)
	{
		// Check to see if mission has ended. If so don't bother anymore...

		return false;
	}
	
	return true;
}


//--------------------------------------------------------------------------//
//	 

CQSCRIPTPROGRAM(SolTrain_HelionInProgress, SolTrain_HelionInProgress_Save,0);

void SolTrain_HelionInProgress::Initialize (U32 eventFlags, const MPartRef & part)
{
	//setup trigger to detect when HQ is finished
	MScript:: SetTriggerProgram(data.TrainTrigger1, "SolTrain_HelionFinished");
	// detect a HQ finished player 1.
	MScript:: SetTriggerFilter(data.TrainTrigger1, M_HELIONVEIL, TRIGGER_MOBJCLASS, false);
	MScript:: SetTriggerFilter(data.TrainTrigger1, 0, TRIGGER_FORCEREADY, true);
	MScript:: SetTriggerFilter(data.TrainTrigger1, PLAYER_ID, TRIGGER_PLAYER, true);

	
}

bool SolTrain_HelionInProgress::Update (void)
{	
	if (data.mission_over)
	{
		// Check to see if mission has ended. If so don't bother anymore...

		return false;
	}
	
	return true;
}


//--------------------------------------------------------------------------//
//	 

CQSCRIPTPROGRAM(SolTrain_HelionFinished, SolTrain_HelionFinished_Save,0);

void SolTrain_HelionFinished::Initialize (U32 eventFlags, const MPartRef & part)
{	
		MScript::EndStream(data.mhandle);
		data.mhandle = MScript::PlayAnimatedMessage( "STMG20.wav", "Animate!!Elan2", 
	        MagpieLeft, MagpieTop, IDS_SOL_SUB_STMG20 );
			
		data.mission_tech.race[2].build = TECHTREE::BUILDNODE( data.mission_tech.race[2].build | 
			TECHTREE::SDEPEND_ESP_COIL );
		MScript::SetAvailiableTech( data.mission_tech );

		MScript:: EnableTrigger(data.TrainTrigger1, false);
		Spawn = MScript:: GetPartByName("MantisSpawn");
	
		MScript::MarkObjectiveCompleted( IDS_SOLTRAIN_OBJECTIVE10 );
		MScript::RemoveFromObjectiveList( IDS_SOLTRAIN_OBJECTIVE8 );
		
		state = Begin;
	
}

bool SolTrain_HelionFinished::Update (void)
{	
	if (data.mission_over)
	{
		// Check to see if mission has ended. If so don't bother anymore...

		return true;
	}

	switch(state)
	{
			case Begin:

				if(CountObjectsInSupply(M_TAOS, PLAYER_ID) >= 1)
				{
					Attacker = MScript:: CreatePart("GBOAT!!M_Frigate", Spawn, MANTIS_ID );
					MScript:: SetStance (Attacker, US_ATTACK);
					MScript:: OrderMoveTo(Attacker, MScript:: GetPartByName("Move"));

					Attacker2 = MScript:: CreatePart("GBOAT!!M_Frigate", Spawn, MANTIS_ID);
					MScript:: SetStance (Attacker2, US_ATTACK);
					MScript:: OrderMoveTo(Attacker2, MScript:: GetPartByName("Move"));

					Attacker3 = MScript:: CreatePart("GBOAT!!M_Frigate", Spawn, MANTIS_ID);
					MScript:: SetStance (Attacker3, US_ATTACK);
					MScript:: OrderMoveTo(Attacker3, M_ GetPartByName("Move"));

					state = Attack;
				}
			break;

			case Attack:
				if(MScript:: IsVisibleToPlayer(Attacker, PLAYER_ID))
				{	
					MScript::EndStream(data.mhandle);
					data.mhandle = MScript::PlayAnimatedMessage( "STMG19.wav", "Animate!!Elan2", 
						MagpieLeft, MagpieTop, IDS_SOL_SUB_STMG19 );
					AddAndDisplayObjective( IDS_SOLTRAIN_OBJECTIVE11, data.mhandle, true );
					//setup trigger to detect when HQ is finished
					MScript::EnableTrigger(data.TrainTrigger1, true);
					MScript:: SetTriggerProgram(data.TrainTrigger1, "SolTrain_TeslaInProgress");
					// detect a HQ finished player 1.
					MScript:: SetTriggerFilter(data.TrainTrigger1, M_ESPCOIL, TRIGGER_MOBJCLASS, false);
					MScript:: SetTriggerFilter(data.TrainTrigger1, 0, TRIGGER_NOFORCEREADY, true);
					MScript:: SetTriggerFilter(data.TrainTrigger1, PLAYER_ID, TRIGGER_PLAYER, true);

				return false;
				}
			break;
	
	}

	return true;
}

//--------------------------------------------------------------------------//
//	 

CQSCRIPTPROGRAM(SolTrain_TeslaInProgress, SolTrain_TeslaInProgress_Save,0);

void SolTrain_TeslaInProgress::Initialize (U32 eventFlags, const MPartRef & part)
{	
		MScript:: SetTriggerProgram(data.TrainTrigger1, "SolTrain_TeslaFinished");
		MScript:: SetTriggerFilter(data.TrainTrigger1, M_ESPCOIL, TRIGGER_MOBJCLASS, false);
		MScript:: SetTriggerFilter(data.TrainTrigger1, 0, TRIGGER_FORCEREADY, true);
		MScript:: SetTriggerFilter(data.TrainTrigger1, PLAYER_ID, TRIGGER_PLAYER, true);
	
}

bool SolTrain_TeslaInProgress::Update (void)
{	
	if (data.mission_over)
	{
		// Check to see if mission has ended. If so don't bother anymore...

		return false;
	}
	
	return true;
}

//--------------------------------------------------------------------------//
//	 

CQSCRIPTPROGRAM(SolTrain_TeslaFinished, SolTrain_TeslaFinished_Save,0);

void SolTrain_TeslaFinished::Initialize (U32 eventFlags, const MPartRef & part)
{
	MScript::MarkObjectiveCompleted( IDS_SOLTRAIN_OBJECTIVE11 );
	MScript::RemoveFromObjectiveList( IDS_SOLTRAIN_OBJECTIVE9 );
	
		if(!MScript:: IsStreamPlaying(data.mhandle))
		{	
			

			MScript::EndStream(data.mhandle);
	        data.mhandle = MScript::PlayAnimatedMessage( "STMG30.wav", "Animate!!Elan2", 
				MagpieLeft, MagpieTop, IDS_SOL_SUB_STMG30 );
		
			AddAndDisplayObjective( IDS_SOLTRAIN_OBJECTIVE16, data.mhandle, true );
				
			//setup trigger to detect when HQ is finished
			data.mission_tech.race[2].build = TECHTREE::BUILDNODE( data.mission_tech.race[2].build | 
				TECHTREE::SDEPEND_CITADEL | TECHTREE::SDEPEND_PROTEUS | TECHTREE::SDEPEND_HYDROFOIL |TECHTREE::SDEPEND_STARBURST );
			MScript::SetAvailiableTech( data.mission_tech );	

			MScript::EnableTrigger(data.TrainTrigger3, true);
			MScript::EnableTrigger(data.TrainTrigger4, true);
			MScript::EnableTrigger(data.TrainTrigger1, false);
			
			MScript:: SetTriggerProgram(data.TrainTrigger3, "SolTrain_CitadelInProgress");
			// detect a HQ finished player 1.
			MScript:: SetTriggerFilter(data.TrainTrigger3, M_CITADEL, TRIGGER_MOBJCLASS, false);
			MScript:: SetTriggerFilter(data.TrainTrigger3, 0, TRIGGER_NOFORCEREADY, true);
			MScript:: SetTriggerFilter(data.TrainTrigger3, PLAYER_ID, TRIGGER_PLAYER, true);

			MScript:: SetTriggerProgram(data.TrainTrigger4, "SolTrain_CitadelInProgress");
			// detect a HQ finished player 1.
			MScript:: SetTriggerFilter(data.TrainTrigger4, M_CITADEL, TRIGGER_MOBJCLASS, false);
			MScript:: SetTriggerFilter(data.TrainTrigger4, 0, TRIGGER_NOFORCEREADY, true);
			MScript:: SetTriggerFilter(data.TrainTrigger4, PLAYER_ID, TRIGGER_PLAYER, true);
				
		}
}

bool SolTrain_TeslaFinished::Update (void)
{	
	if (data.mission_over)
	{
		// Check to see if mission has ended. If so don't bother anymore...

		return false;
	}
	
	return true;
}

//--------------------------------------------------------------------------//
//	 

CQSCRIPTPROGRAM(SolTrain_CitadelInProgress, SolTrain_CitadelInProgress_Save,0);

void SolTrain_CitadelInProgress::Initialize (U32 eventFlags, const MPartRef & part)
{	
	MScript:: SetTriggerProgram(data.TrainTrigger3, "SolTrain_CitadelFinished");
	MScript:: SetTriggerFilter(data.TrainTrigger3, M_CITADEL, TRIGGER_MOBJCLASS, false);
	MScript:: SetTriggerFilter(data.TrainTrigger3, 0, TRIGGER_FORCEREADY, true);
	MScript:: SetTriggerFilter(data.TrainTrigger3, PLAYER_ID, TRIGGER_PLAYER, true);

	MScript:: SetTriggerProgram(data.TrainTrigger4, "SolTrain_CitadelFinished");
	MScript:: SetTriggerFilter(data.TrainTrigger4, M_CITADEL, TRIGGER_MOBJCLASS, false);
	MScript:: SetTriggerFilter(data.TrainTrigger4, 0, TRIGGER_FORCEREADY, true);
	MScript:: SetTriggerFilter(data.TrainTrigger4, PLAYER_ID, TRIGGER_PLAYER, true);
	
	MScript::EndStream(data.mhandle);
	data.mhandle = MScript::PlayAnimatedMessage( "STMG31.wav", "Animate!!Elan2", 
		MagpieLeft, MagpieTop, IDS_SOL_SUB_STMG31 );}


bool SolTrain_CitadelInProgress::Update (void)
{	
	if (data.mission_over)
	{
		// Check to see if mission has ended. If so don't bother anymore...

		return false;
	}
	
	return true;
}

	
//--------------------------------------------------------------------------//
//	 

CQSCRIPTPROGRAM(SolTrain_CitadelFinished, SolTrain_CitadelFinished_Save,0);

void SolTrain_CitadelFinished::Initialize (U32 eventFlags, const MPartRef & part)
{	
	MScript::EnableTrigger(data.TrainTrigger1, true);
	MScript::EnableTrigger(data.TrainTrigger3, false);
	MScript::EnableTrigger(data.TrainTrigger4, false);
	
	MScript::RemoveFromObjectiveList( IDS_SOLTRAIN_OBJECTIVE11);
	MScript::MarkObjectiveCompleted( IDS_SOLTRAIN_OBJECTIVE16 );
	
	data.mission_tech.race[2].build = TECHTREE::BUILDNODE( data.mission_tech.race[2].build | 
		TECHTREE::SDEPEND_GREATERPAVILION);
	MScript::SetAvailiableTech( data.mission_tech );

	MScript::SetTriggerProgram(data.TrainTrigger1, "SolTrain_GPavilionInProgress");
	MScript::SetTriggerFilter(data.TrainTrigger1, M_GREATERPAVILION, TRIGGER_MOBJCLASS, false);
	MScript::SetTriggerFilter(data.TrainTrigger1, 0, TRIGGER_NOFORCEREADY, true);
	MScript::SetTriggerFilter(data.TrainTrigger1, PLAYER_ID, TRIGGER_PLAYER, true);
	
	AddAndDisplayObjective( IDS_SOLTRAIN_OBJECTIVE18, data.mhandle, true );

}

bool SolTrain_CitadelFinished::Update (void)
{
	if (data.mission_over)
	{
		// Check to see if mission has ended. If so don't bother anymore...

		return false;
	}
	
	return true;
}
		

//--------------------------------------------------------------------------//
//	 

CQSCRIPTPROGRAM(SolTrain_GPavilionInProgress, SolTrain_GPavilionInProgress_Save,0);

void SolTrain_GPavilionInProgress::Initialize (U32 eventFlags, const MPartRef & part)
{
	if(!MScript:: IsStreamPlaying(data.mhandle))
	
		{	
			MScript:: SetTriggerProgram(data.TrainTrigger1, "SolTrain_GPavilionFinished");
			MScript:: SetTriggerFilter(data.TrainTrigger1, M_GREATERPAVILION, TRIGGER_MOBJCLASS, false);
			MScript:: SetTriggerFilter(data.TrainTrigger1, 0, TRIGGER_FORCEREADY, true);
			MScript:: SetTriggerFilter(data.TrainTrigger1, PLAYER_ID, TRIGGER_PLAYER, true);
		}
 
}

bool SolTrain_GPavilionInProgress::Update (void)
{
	
	if (data.mission_over)
	{
		// Check to see if mission has ended. If so don't bother anymore...

		return false;
	}
	
	return true;
}


//--------------------------------------------------------------------------//
//	 


CQSCRIPTPROGRAM(SolTrain_GPavilionFinished, SolTrain_GPavilionFinished_Save,0);

void SolTrain_GPavilionFinished::Initialize (U32 eventFlags, const MPartRef & part)
{	

	data.mission_tech.race[2].build = TECHTREE::BUILDNODE( data.mission_tech.race[2].build | 
		TECHTREE::SDEPEND_MUNITIONSANEX);
	MScript::SetAvailiableTech( data.mission_tech );
	
	MScript::MarkObjectiveCompleted( IDS_SOLTRAIN_OBJECTIVE18 );
	
	MScript:: SetTriggerProgram(data.TrainTrigger1, "SolTrain_AnnexInProgress");
	MScript:: SetTriggerFilter(data.TrainTrigger1, M_MUNITIONSANNEX, TRIGGER_MOBJCLASS, false);
	MScript:: SetTriggerFilter(data.TrainTrigger1, 0, TRIGGER_NOFORCEREADY, true);
	MScript:: SetTriggerFilter(data.TrainTrigger1, PLAYER_ID, TRIGGER_PLAYER, true);
	
	AddAndDisplayObjective( IDS_SOLTRAIN_OBJECTIVE17, data.mhandle, true );
}

bool SolTrain_GPavilionFinished::Update (void)
{
	if (data.mission_over)
	{
		// Check to see if mission has ended. If so don't bother anymore...

		return false;
	}
	
	return true;
}

//--------------------------------------------------------------------------//
//	 

CQSCRIPTPROGRAM(SolTrain_AnnexInProgress, SolTrain_AnnexInProgress_Save,0);

void SolTrain_AnnexInProgress::Initialize (U32 eventFlags, const MPartRef & part)
{
	MScript:: SetTriggerProgram(data.TrainTrigger1, "SolTrain_AnnexFinished");
	MScript:: SetTriggerFilter(data.TrainTrigger1, M_MUNITIONSANNEX, TRIGGER_MOBJCLASS, false);
	MScript:: SetTriggerFilter(data.TrainTrigger1, 0, TRIGGER_FORCEREADY, true);
	MScript:: SetTriggerFilter(data.TrainTrigger1, PLAYER_ID, TRIGGER_PLAYER, true);
}

bool SolTrain_AnnexInProgress::Update (void)
{
	if (data.mission_over)
	{
		// Check to see if mission has ended. If so don't bother anymore...

		return false;
	}
	
	return true;
}

//--------------------------------------------------------------------------//
//	 

CQSCRIPTPROGRAM(SolTrain_AnnexFinished, SolTrain_AnnexFinished_Save,0);

void SolTrain_AnnexFinished::Initialize (U32 eventFlags, const MPartRef & part)
{
	data.mission_tech.race[2].build = TECHTREE::BUILDNODE( data.mission_tech.race[2].build | 
		TECHTREE::SDEPEND_ANVIL);
	MScript::SetAvailiableTech( data.mission_tech );
	
	MScript::MarkObjectiveCompleted( IDS_SOLTRAIN_OBJECTIVE17 );
	MScript::RemoveFromObjectiveList( IDS_SOLTRAIN_OBJECTIVE16 );
	
	MScript:: SetTriggerProgram(data.TrainTrigger1, "SolTrain_AnvilInProgress");
	MScript:: SetTriggerFilter(data.TrainTrigger1, M_ANVIL, TRIGGER_MOBJCLASS, false);
	MScript:: SetTriggerFilter(data.TrainTrigger1, 0, TRIGGER_NOFORCEREADY, true);
	MScript:: SetTriggerFilter(data.TrainTrigger1, PLAYER_ID, TRIGGER_PLAYER, true);
	
	AddAndDisplayObjective( IDS_SOLTRAIN_OBJECTIVE22, data.mhandle, true );
 
}

bool SolTrain_AnnexFinished::Update (void)
{
	if (data.mission_over)
	{
		// Check to see if mission has ended. If so don't bother anymore...

		return false;
	}
	
	return true;
}

//--------------------------------------------------------------------------//
//	 

CQSCRIPTPROGRAM(SolTrain_AnvilInProgress, SolTrain_AnvilInProgress_Save,0);

void SolTrain_AnvilInProgress::Initialize (U32 eventFlags, const MPartRef & part)
{
	MScript:: SetTriggerProgram(data.TrainTrigger1, "SolTrain_AnvilFinished");
	MScript:: SetTriggerFilter(data.TrainTrigger1, M_ANVIL, TRIGGER_MOBJCLASS, false);
	MScript:: SetTriggerFilter(data.TrainTrigger1, 0, TRIGGER_FORCEREADY, true);
	MScript:: SetTriggerFilter(data.TrainTrigger1, PLAYER_ID, TRIGGER_PLAYER, true);
}

bool SolTrain_AnvilInProgress::Update (void)
{
	if (data.mission_over)
	{
		// Check to see if mission has ended. If so don't bother anymore...

		return false;
	}
	
	return true;
}

//--------------------------------------------------------------------------//
//	 

CQSCRIPTPROGRAM(SolTrain_AnvilFinished, SolTrain_AnvilFinished_Save,0);

void SolTrain_AnvilFinished::Initialize (U32 eventFlags, const MPartRef & part)
{
	data.mission_tech.race[2].build = TECHTREE::BUILDNODE( data.mission_tech.race[2].build | 
		TECHTREE::SDEPEND_TURBINEDOCK);
	MScript::SetAvailiableTech( data.mission_tech );

	MScript::MarkObjectiveCompleted( IDS_SOLTRAIN_OBJECTIVE22 );
	
	MScript:: SetTriggerProgram(data.TrainTrigger1, "SolTrain_DockInProgress");
	MScript:: SetTriggerFilter(data.TrainTrigger1, M_TURBINEDOCK, TRIGGER_MOBJCLASS, false);
	MScript:: SetTriggerFilter(data.TrainTrigger1, 0, TRIGGER_NOFORCEREADY, true);
	MScript:: SetTriggerFilter(data.TrainTrigger1, PLAYER_ID, TRIGGER_PLAYER, true);
	
	AddAndDisplayObjective( IDS_SOLTRAIN_OBJECTIVE23, data.mhandle, true );
 
}

bool SolTrain_AnvilFinished::Update (void)
{
	if (data.mission_over)
	{
		// Check to see if mission has ended. If so don't bother anymore...

		return false;
	}

	return true;
}

//--------------------------------------------------------------------------//
//	 

CQSCRIPTPROGRAM(SolTrain_DockInProgress, SolTrain_DockInProgress_Save,0);

void SolTrain_DockInProgress::Initialize (U32 eventFlags, const MPartRef & part)
{		
	MScript:: SetTriggerProgram(data.TrainTrigger1, "SolTrain_DockFinished");
	MScript:: SetTriggerFilter(data.TrainTrigger1, M_TURBINEDOCK, TRIGGER_MOBJCLASS, false);
	MScript:: SetTriggerFilter(data.TrainTrigger1, 0, TRIGGER_FORCEREADY, true);
	MScript:: SetTriggerFilter(data.TrainTrigger1, PLAYER_ID, TRIGGER_PLAYER, true);

}

bool SolTrain_DockInProgress::Update (void)
{
	if (data.mission_over)
	{
		// Check to see if mission has ended. If so don't bother anymore...

		return false;
	}
	
	return true;
}

//--------------------------------------------------------------------------//
//	 

CQSCRIPTPROGRAM(SolTrain_DockFinished, SolTrain_DockFinished_Save,0);

void SolTrain_DockFinished::Initialize (U32 eventFlags, const MPartRef & part)
{
	data.mission_tech.race[2].build = TECHTREE::BUILDNODE( data.mission_tech.race[2].build | 
		TECHTREE::SDEPEND_XENOCHAMBER);
	MScript::SetAvailiableTech( data.mission_tech );
	
	MScript::MarkObjectiveCompleted( IDS_SOLTRAIN_OBJECTIVE23 );
	
	MScript:: SetTriggerProgram(data.TrainTrigger1, "SolTrain_XenoInProgress");
	MScript:: SetTriggerFilter(data.TrainTrigger1, M_XENOCHAMBER, TRIGGER_MOBJCLASS, false);
	MScript:: SetTriggerFilter(data.TrainTrigger1, 0, TRIGGER_NOFORCEREADY, true);
	MScript:: SetTriggerFilter(data.TrainTrigger1, PLAYER_ID, TRIGGER_PLAYER, true);
	
	AddAndDisplayObjective( IDS_SOLTRAIN_OBJECTIVE24, data.mhandle, true );
	
	MScript::EndStream(data.mhandle);
	data.mhandle = MScript::PlayAnimatedMessage( "STMG33.wav", "Animate!!Elan2", 
		MagpieLeft, MagpieTop, IDS_SOL_SUB_STMG33 );
 
}

bool SolTrain_DockFinished::Update (void)
{
	if (data.mission_over)
	{
		// Check to see if mission has ended. If so don't bother anymore...

		return false;
	}
	
	return true;
}

//--------------------------------------------------------------------------//
//	 

CQSCRIPTPROGRAM(SolTrain_XenoInProgress, SolTrain_XenoInProgress_Save,0);

void SolTrain_XenoInProgress::Initialize (U32 eventFlags, const MPartRef & part)
{	
	MScript:: SetTriggerProgram(data.TrainTrigger1, "SolTrain_XenoFinished");
	MScript:: SetTriggerFilter(data.TrainTrigger1, M_XENOCHAMBER, TRIGGER_MOBJCLASS, false);
	MScript:: SetTriggerFilter(data.TrainTrigger1, 0, TRIGGER_FORCEREADY, true);
	MScript:: SetTriggerFilter(data.TrainTrigger1, PLAYER_ID, TRIGGER_PLAYER, true);

}

bool SolTrain_XenoInProgress::Update (void)
{
	if (data.mission_over)
	{
		// Check to see if mission has ended. If so don't bother anymore...

		return false;
	}
	
	return true;
}

//--------------------------------------------------------------------------//
//	 

CQSCRIPTPROGRAM(SolTrain_XenoFinished, SolTrain_XenoFinished_Save,0);

void SolTrain_XenoFinished::Initialize (U32 eventFlags, const MPartRef & part)
{
	AddAndDisplayObjective( IDS_SOLTRAIN_OBJECTIVE12, data.mhandle, true );
	MScript::MarkObjectiveCompleted( IDS_SOLTRAIN_OBJECTIVE24 );
	
	MScript:: RunProgramByName("SolTrain_SixPolaris", MPartRef());
}

bool SolTrain_XenoFinished::Update (void)
{
	if (data.mission_over)
	{
		// Check to see if mission has ended. If so don't bother anymore...

		return false;
	}
	
	return true;
}

//------------------------------------------------------------------------//
//	 

CQSCRIPTPROGRAM(SolTrain_SixPolaris, SolTrain_SixPolaris_Save,0);

void SolTrain_SixPolaris::Initialize (U32 eventFlags, const MPartRef & part)

{		
	MScript:: EnableTrigger(data.TrainTrigger1, false);
	
	MScript::EndStream( data.mhandle );
	data.mhandle = MScript::PlayAnimatedMessage( "STMG21.wav", "Animate!!Elan2", 
		MagpieLeft, MagpieTop, IDS_SOL_SUB_STMG21 );
}

bool SolTrain_SixPolaris::Update (void)
{	
	if (data.mission_over)
	{
		// Check to see if mission has ended. If so don't bother anymore...

		return false;
	}


	if(CountObjectsInSupply(M_POLARIS, PLAYER_ID) >= 6)
		{	
			MScript::MarkObjectiveCompleted( IDS_SOLTRAIN_OBJECTIVE16 );
			MScript::MarkObjectiveCompleted( IDS_SOLTRAIN_OBJECTIVE12 );
	
			MScript::EndStream(data.mhandle);
			data.mhandle = MScript::PlayAnimatedMessage( "STMG22.wav", "Animate!!Elan2", 
		        MagpieLeft, MagpieTop, IDS_SOL_SUB_STMG22 );
	
			AddAndDisplayObjective( IDS_SOLTRAIN_OBJECTIVE13, data.mhandle, true );
			MScript::RemoveFromObjectiveList( IDS_SOLTRAIN_OBJECTIVE10 );

			data.mission_tech.race[2].build = TECHTREE::BUILDNODE( data.mission_tech.race[2].build | 
				TECHTREE::SDEPEND_EUTROMIL );
			MScript::SetAvailiableTech( data.mission_tech );

			MScript:: EnableTrigger(data.TrainTrigger1, true);
			MScript:: SetTriggerProgram(data.TrainTrigger1, "SolTrain_EutroInProgress");
			MScript:: SetTriggerFilter(data.TrainTrigger1, M_EUTROMILL, TRIGGER_MOBJCLASS, false);
			MScript:: SetTriggerFilter(data.TrainTrigger1, 0, TRIGGER_NOFORCEREADY, true);
			MScript:: SetTriggerFilter(data.TrainTrigger1, PLAYER_ID, TRIGGER_PLAYER, true);

			return false;
		}

	return true;
}


//--------------------------------------------------------------------------//
//	 

CQSCRIPTPROGRAM(SolTrain_EutroInProgress, SolTrain_EutroInProgress_Save,0);

void SolTrain_EutroInProgress::Initialize (U32 eventFlags, const MPartRef & part)
{
	data.mission_tech.race[2].build = TECHTREE::BUILDNODE( data.mission_tech.race[2].build | 
		TECHTREE::SDEPEND_JUMP_PLAT );
	MScript::SetAvailiableTech( data.mission_tech );

	MScript::RemoveFromObjectiveList( IDS_SOLTRAIN_OBJECTIVE18 );
	MScript::RemoveFromObjectiveList( IDS_SOLTRAIN_OBJECTIVE17 );
	MScript::RemoveFromObjectiveList( IDS_SOLTRAIN_OBJECTIVE22 );
	MScript::RemoveFromObjectiveList( IDS_SOLTRAIN_OBJECTIVE23 );
	MScript::RemoveFromObjectiveList( IDS_SOLTRAIN_OBJECTIVE24 );

	MScript:: SetTriggerProgram(data.TrainTrigger1, "SolTrain_EutroFinished");
	MScript:: SetTriggerFilter(data.TrainTrigger1, M_EUTROMILL, TRIGGER_MOBJCLASS, false);
	MScript:: SetTriggerFilter(data.TrainTrigger1, 0, TRIGGER_FORCEREADY, true);
	MScript:: SetTriggerFilter(data.TrainTrigger1, PLAYER_ID, TRIGGER_PLAYER, true);
 
	MScript::EndStream(data.mhandle);
	data.mhandle = MScript::PlayAnimatedMessage( "STMG23.wav", "Animate!!Elan2", 
	    MagpieLeft, MagpieTop, IDS_SOL_SUB_STMG23 );

}

bool SolTrain_EutroInProgress::Update (void)
{	
	if (data.mission_over)
	{
		// Check to see if mission has ended. If so don't bother anymore...

		return false;
	}

	return true;
}

//--------------------------------------------------------------------------//
//	 

CQSCRIPTPROGRAM(SolTrain_EutroFinished, SolTrain_EutroFinished_Save,0);

void SolTrain_EutroFinished::Initialize (U32 eventFlags, const MPartRef & part)
{
	MScript:: SetTriggerProgram(data.TrainTrigger1, "SolTrain_JumpgateInProgress");
	MScript:: SetTriggerFilter(data.TrainTrigger1, M_JUMPPLAT, TRIGGER_MOBJCLASS, false);
	MScript:: SetTriggerFilter(data.TrainTrigger1, 0, TRIGGER_NOFORCEREADY, true);
	MScript:: SetTriggerFilter(data.TrainTrigger1, PLAYER_ID, TRIGGER_PLAYER, true);
 
	MScript::EndStream(data.mhandle);
	data.mhandle = MScript::PlayAnimatedMessage( "STMG24.wav", "Animate!!Elan2", 
	    MagpieLeft, MagpieTop, IDS_SOL_SUB_STMG24 );
	
	MScript::MarkObjectiveCompleted( IDS_SOLTRAIN_OBJECTIVE13 );
	AddAndDisplayObjective( IDS_SOLTRAIN_OBJECTIVE14, data.mhandle, true );
	MScript::RemoveFromObjectiveList( IDS_SOLTRAIN_OBJECTIVE11 );

	data.Alert = MScript::GetPartByName( "Alert" );
	MScript::AlertMessage(data.Alert, 0);
	
}

bool SolTrain_EutroFinished::Update (void)
{
	if (data.mission_over)
	{
		// Check to see if mission has ended. If so don't bother anymore...

		return false;
	}

	return true;
}

//--------------------------------------------------------------------------//
//	 

CQSCRIPTPROGRAM(SolTrain_JumpgateInProgress, SolTrain_JumpgateInProgress_Save,0);

void SolTrain_JumpgateInProgress::Initialize (U32 eventFlags, const MPartRef & part)
{	
	MScript:: SetTriggerProgram(data.TrainTrigger1, "SolTrain_JumpgateFinished");
	MScript:: SetTriggerFilter(data.TrainTrigger1, M_JUMPPLAT, TRIGGER_MOBJCLASS, false);
	MScript:: SetTriggerFilter(data.TrainTrigger1, 0, TRIGGER_FORCEREADY, true);
	MScript:: SetTriggerFilter(data.TrainTrigger1, PLAYER_ID, TRIGGER_PLAYER, true);
	
	MScript::MarkObjectiveCompleted( IDS_SOLTRAIN_OBJECTIVE13 );
	
	MScript::EndStream(data.mhandle);
	data.mhandle = MScript::PlayAnimatedMessage( "STMG25.wav", "Animate!!Elan2", 
	    MagpieLeft, MagpieTop, IDS_SOL_SUB_STMG25 );
}

bool SolTrain_JumpgateInProgress::Update (void)
{
	if (data.mission_over)
	{
		// Check to see if mission has ended. If so don't bother anymore...

		return false;
	}

	return true;
}

//--------------------------------------------------------------------------//
//	 

CQSCRIPTPROGRAM(SolTrain_JumpgateFinished, SolTrain_JumpgateFinished_Save,0);

void SolTrain_JumpgateFinished::Initialize (U32 eventFlags, const MPartRef & part)
{
	MScript::MarkObjectiveCompleted( IDS_SOLTRAIN_OBJECTIVE14 );
	
	MScript::EnableJumpgate( MScript::GetPartByName( "Wormhole to Kasse (Solar)" ), true );

	MScript::EnableTrigger(data.TrainTrigger1, false);

	MScript::EndStream(data.mhandle);
	data.mhandle = MScript::PlayAnimatedMessage( "STMG26.wav", "Animate!!Elan2", 
		MagpieLeft, MagpieTop, IDS_SOL_SUB_STMG26 );
	
	AddAndDisplayObjective( IDS_SOLTRAIN_OBJECTIVE15, data.mhandle, true );
	MScript::RemoveFromObjectiveList( IDS_SOLTRAIN_OBJECTIVE12 );
	MScript:: RunProgramByName("SolTrain_JumpToKasse", MPartRef());

}

bool SolTrain_JumpgateFinished::Update (void)
{
	if (data.mission_over)
	{
		// Check to see if mission has ended. If so don't bother anymore...

		return false;
	}

	return true;

}

//--------------------------------------------------------------------------//
//	 

CQSCRIPTPROGRAM(SolTrain_JumpToKasse, SolTrain_JumpToKasse_Save,0);

void SolTrain_JumpToKasse::Initialize (U32 eventFlags, const MPartRef & part)
{	
	Spawn = MScript:: GetPartByName("Attack2");
	Spawn1 = MScript:: GetPartByName("Niad");
	Spawn2 = MScript:: GetPartByName("Tiamat");
	
	state = Begin;
}
bool SolTrain_JumpToKasse::Update (void)
{	
	if (data.mission_over)
	{
		// Check to see if mission has ended. If so don't bother anymore...

		return false;
	}


	switch(state)
		{
			case Begin:
		
			
				if(PlayerInSystem(PLAYER_ID, KASSE) && !data.PlayerInKasse) 
				{	

					Attacker = MScript:: CreatePart("GBOAT!!M_Scout Carrier", Spawn, MANTIS_ID , "Warlord Agar");
					MScript::EnablePartnameDisplay(Attacker, true);
					MScript::SetStance (Attacker, US_ATTACK);
					MScript:: OrderMoveTo(Attacker, MScript:: GetPartByName("Move2"));

					Attacker2 = MScript:: CreatePart("GBOAT!!M_Scout Carrier", Spawn, MANTIS_ID, "Warlord Hir");
					MScript::EnablePartnameDisplay(Attacker2, true);
					MScript::SetStance (Attacker2, US_ATTACK);
					MScript:: OrderMoveTo(Attacker2, MScript:: GetPartByName("Kasse Prime"));

					Attacker3 = MScript:: CreatePart("GBOAT!!M_Scout Carrier", Spawn, MANTIS_ID, "Warlord Kash");
					MScript::EnablePartnameDisplay(Attacker3, true);
					MScript::SetStance (Attacker3, US_ATTACK);
					MScript:: OrderMoveTo(Attacker3, MScript:: GetPartByName("Move2"));

					Attacker4 = MScript:: CreatePart("GBOAT!!M_Scout Carrier", Spawn, MANTIS_ID, "Warlord Hotes");
					MScript::EnablePartnameDisplay(Attacker4, true);
					MScript::SetStance (Attacker4, US_ATTACK);
					MScript:: OrderMoveTo(Attacker4, MScript:: GetPartByName("Kasse Prime"));

					state = Wave;
				}
			
			break;		
				
			case Wave:
				
					if(PlayerInSystem(PLAYER_ID, KASSE) && !data.PlayerInKasse) 
					{	
						for(x = 0; x < 4; x++)
						{
							Attacker5 = MScript:: CreatePart("GBOAT!!M_Scarab", Spawn1, MANTIS_ID);
							MScript:: SetStance (Attacker5, US_ATTACK);
							MScript:: OrderMoveTo(Attacker5, MScript:: GetPartByName("Kasse 2"));
						}
													
						for(x = 0; x < 5; x++)
						{
							Attacker7 = MScript:: CreatePart("GBOAT!!M_Hive Carrier", Spawn1, MANTIS_ID);
							MScript:: SetStance (Attacker7, US_ATTACK);
							MScript:: OrderMoveTo(Attacker7, MScript:: GetPartByName("Kasse 2"));
						}
						for(x = 0; x < 8; x++)
						{	
							Attacker8 = MScript:: CreatePart("GBOAT!!M_Frigate", Spawn1, MANTIS_ID);
							MScript:: SetStance (Attacker8, US_ATTACK);
							MScript:: OrderMoveTo(Attacker8, MScript:: GetPartByName("Kasse 2"));
							state = Wave2;
						}
					}
				
			break;

			case Wave2:

				if(PlayerInSystem(PLAYER_ID, KASSE) && !data.PlayerInKasse) 
					{	
						Attacker6 = MScript:: CreatePart("GBOAT!!M_Tiamat", Spawn2, MANTIS_ID, "Warlord Indra");
						MScript::EnablePartnameDisplay(Attacker6, true);
						MScript:: SetStance (Attacker6, US_ATTACK);
						MScript:: OrderMoveTo(Attacker6, MScript:: GetPartByName("Blindside"));

						Attacker9 = MScript:: CreatePart("GBOAT!!M_Tiamat", Spawn2, MANTIS_ID, "Warlord Jaydev");
						MScript::EnablePartnameDisplay(Attacker9, true);
						MScript:: SetStance (Attacker9, US_ATTACK);
						MScript:: OrderMoveTo(Attacker9, MScript:: GetPartByName("Blindside"));

						Attacker10 = MScript:: CreatePart("GBOAT!!M_Tiamat", Spawn2, MANTIS_ID, "Warlord Vasan");
						MScript::EnablePartnameDisplay(Attacker10, true);
						MScript:: SetStance (Attacker10, US_ATTACK);
						MScript:: OrderMoveTo(Attacker10, MScript:: GetPartByName("Blindside"));

						Attacker11 = MScript:: CreatePart("GBOAT!!M_Tiamat", Spawn2, MANTIS_ID, "Warlord Digvi");
						MScript::EnablePartnameDisplay(Attacker11, true);
						MScript:: SetStance (Attacker11, US_ATTACK);
						MScript:: OrderMoveTo(Attacker11, MScript:: GetPartByName("Blindside"));

						Attacker12 = MScript:: CreatePart("GBOAT!!M_Scarab", Spawn2, MANTIS_ID,"Warlord Prit");
						MScript::EnablePartnameDisplay(Attacker12, true);
						MScript:: SetStance (Attacker12, US_ATTACK);
						MScript:: OrderMoveTo(Attacker12, MScript:: GetPartByName("Blindside"));

						state = Attack;
						
					}
				break;
					
			
			case Attack:	
				
			
				if(PlayerInSystem(PLAYER_ID, KASSE) && !data.PlayerInKasse)
				{	
					MScript::MarkObjectiveCompleted( IDS_SOLTRAIN_OBJECTIVE15 );
 					
					MScript::EndStream(data.mhandle);
					data.mhandle = MScript::PlayAnimatedMessage( "STMG27.wav", "Animate!!Elan2", 
						MagpieLeft, MagpieTop, IDS_SOL_SUB_STMG27 );
				
					MScript:: RunProgramByName("SolTrain_KillTheMantis", MPartRef());
					MScript:: RunProgramByName("SolTrain_4and2", MPartRef());
				
				return false;
				}

			break;

		
		}
	
	return true;
}

//------------------------------------------------------------------------//
//	 

CQSCRIPTPROGRAM(SolTrain_4and2, SolTrain_4and2_Save,0);

void SolTrain_4and2::Initialize (U32 eventFlags, const MPartRef & part)

{	
	MScript::MarkObjectiveCompleted( IDS_SOLTRAIN_OBJECTIVE18 );
	
	MScript::EndStream(data.mhandle);
	data.mhandle = MScript::PlayAnimatedMessage( "STMG29.wav", "Animate!!Elan2", 
		MagpieLeft, MagpieTop, IDS_SOL_SUB_STMG29 );
	
	state = Built;

}


bool SolTrain_4and2::Update (void)
{	
	if (data.mission_over)
	{
		// Check to see if mission has ended. If so don't bother anymore...

		return false;
	}

	
	switch (state)
	{		
			case Built:

				if(!MScript:: IsStreamPlaying(data.mhandle))
				{	
					MScript::EndStream(data.mhandle);
					data.mhandle = MScript::PlayAnimatedMessage( "STMG34.wav", "Animate!!Elan2", 
						MagpieLeft, MagpieTop, IDS_SOL_SUB_STMG34 );
				
					data.mission_tech.race[2].build = TECHTREE::BUILDNODE( data.mission_tech.race[2].build | 
						TECHTREE::SDEPEND_TALOREANMATRIX);
					MScript::SetAvailiableTech( data.mission_tech );
		
					AddAndDisplayObjective( IDS_SOLTRAIN_OBJECTIVE20, data.mhandle, true );
					state = Matrix;
				}

			break;

			case Matrix:

				

				if(!MScript::IsStreamPlaying(data.mhandle))
				
				{	
					MScript::EnableTrigger(data.TrainTrigger1, true);
					MScript::EnableTrigger(data.TrainTrigger2, true);
					MScript::SetTriggerProgram(data.TrainTrigger1, "SolTrain_TMatrixInProgress");
					MScript::SetTriggerFilter(data.TrainTrigger1, M_TALOREANMATRIX, TRIGGER_MOBJCLASS, false);
					MScript::SetTriggerFilter(data.TrainTrigger1, 0, TRIGGER_NOFORCEREADY, true);
					MScript::SetTriggerFilter(data.TrainTrigger1, PLAYER_ID, TRIGGER_PLAYER, true);
					
					MScript::SetTriggerProgram(data.TrainTrigger2, "SolTrain_TMatrixInProgress");
					MScript::SetTriggerFilter(data.TrainTrigger2, M_TALOREANMATRIX, TRIGGER_MOBJCLASS, false);
					MScript::SetTriggerFilter(data.TrainTrigger2, 0, TRIGGER_NOFORCEREADY, true);
					MScript::SetTriggerFilter(data.TrainTrigger2, PLAYER_ID, TRIGGER_PLAYER, true);

				return false;
				
				}
			
			break;
	}
	

	return true;
}



//--------------------------------------------------------------------------//
//	 

CQSCRIPTPROGRAM(SolTrain_TMatrixInProgress, SolTrain_TMatrixInProgress_Save,0);

void SolTrain_TMatrixInProgress::Initialize (U32 eventFlags, const MPartRef & part)
{	 
	MScript::RemoveFromObjectiveList( IDS_SOLTRAIN_OBJECTIVE12 );
	MScript::RemoveFromObjectiveList( IDS_SOLTRAIN_OBJECTIVE13 );
	MScript::RemoveFromObjectiveList( IDS_SOLTRAIN_OBJECTIVE14 );
	MScript::RemoveFromObjectiveList( IDS_SOLTRAIN_OBJECTIVE15 );

	MScript:: SetTriggerProgram(data.TrainTrigger2, "SolTrain_TMatrixFinished");
	MScript:: SetTriggerFilter(data.TrainTrigger2, M_TALOREANMATRIX, TRIGGER_MOBJCLASS, false);
	MScript:: SetTriggerFilter(data.TrainTrigger2, 0, TRIGGER_FORCEREADY, true);
	MScript:: SetTriggerFilter(data.TrainTrigger2, PLAYER_ID, TRIGGER_PLAYER, true);

	MScript:: SetTriggerProgram(data.TrainTrigger1, "SolTrain_TMatrixFinished");
	MScript:: SetTriggerFilter(data.TrainTrigger1, M_TALOREANMATRIX, TRIGGER_MOBJCLASS, false);
	MScript:: SetTriggerFilter(data.TrainTrigger1, 0, TRIGGER_FORCEREADY, true);
	MScript:: SetTriggerFilter(data.TrainTrigger1, PLAYER_ID, TRIGGER_PLAYER, true);
	
	MScript::EndStream(data.mhandle);
	data.mhandle = MScript::PlayAnimatedMessage( "STMG35.wav", "Animate!!Elan2", 
		MagpieLeft, MagpieTop, IDS_SOL_SUB_STMG35 );
 
}

bool SolTrain_TMatrixInProgress::Update (void)
{	
	if (data.mission_over)
	{
		// Check to see if mission has ended. If so don't bother anymore...

		return false;
	}

	return true;
	
}

//--------------------------------------------------------------------------//
//	 

CQSCRIPTPROGRAM(SolTrain_TMatrixFinished, SolTrain_TMatrixFinished_Save,0);

void SolTrain_TMatrixFinished::Initialize (U32 eventFlags, const MPartRef & part)
{
	AddAndDisplayObjective( IDS_SOLTRAIN_OBJECTIVE21, data.mhandle, true );
					
	MScript::EndStream(data.mhandle);
	data.mhandle = MScript::PlayAnimatedMessage( "STMG36.wav", "Animate!!Elan2", 
		MagpieLeft, MagpieTop, IDS_SOL_SUB_STMG36 );

	MScript::MarkObjectiveCompleted( IDS_SOLTRAIN_OBJECTIVE20 );

	MScript::EnableTrigger(data.TrainTrigger1, false);
	MScript::EnableTrigger(data.TrainTrigger2, false);
}

bool SolTrain_TMatrixFinished::Update (void)
{	
	if (data.mission_over)
	{
		// Check to see if mission has ended. If so don't bother anymore...

		return false;
	}

	return true;
}

//--------------------------------------------------------------------------//
//	 

CQSCRIPTPROGRAM(SolTrain_KillTheMantis, SolTrain_KillTheMantis_Save,0);

void SolTrain_KillTheMantis::Initialize (U32 eventFlags, const MPartRef & part)
{	
	MScript::RemoveFromObjectiveList( IDS_SOLTRAIN_OBJECTIVE17);

	MScript::RemoveFromObjectiveList( IDS_SOLTRAIN_OBJECTIVE20);

	state = Begin;

}

bool SolTrain_KillTheMantis::Update (void)
{
	if (data.mission_over)
	{
		// Check to see if mission has ended. If so don't bother anymore...

		return false;
	}

	switch(state)
	{

		case Begin:
				if(MScript:: IsVisibleToPlayer(MScript:: GetPartByName("Kasse 2"), PLAYER_ID))
				{
					MScript::EndStream(data.mhandle);

					data.mhandle = MScript::PlayAnimatedMessage( "STMG37.wav", "Animate!!Elan2", 
						MagpieLeft, MagpieTop, IDS_SOL_SUB_STMG37 );

					state = FinishOffMantis;
				}
		break;
			
		

		case FinishOffMantis:
		
			if(!MScript:: PlayerHasUnits(MANTIS_ID))
				{	
					
					
					MScript::EnableMovieMode(true);
					UnderMissionControl();

					MScript::EndStream(data.mhandle);
					data.mhandle = MScript::PlayAnimatedMessage( "STMG38.wav", "Animate!!Elan2", 
						MagpieLeft, MagpieTop, IDS_SOL_SUB_STMG38 );
	
					state = Success;
				}
		break;

		case Success:
				
			if(!MScript:: IsStreamPlaying(data.mhandle))
				{
					MScript::MarkObjectiveCompleted( IDS_SOLTRAIN_OBJECTIVE21 );
					MScript:: FlushTeletype();
					
					data.thandle = TeletypeMissionOver( IDS_SOLTRAIN_MISSION_SUCCESS );
					
					state = Done;
				}
		break;

		case Done:
				
			if(!MScript:: IsTeletypePlaying(data.mhandle))
				{
					UnderPlayerControl();	
					MScript:: EndMissionVictory(0);
		
				return false;
				}
			
		break;
	}

	return true;
}

//--------------------------------------------------------------------------//
//	 

CQSCRIPTPROGRAM(SolTrain_CheckForMissionFail, SolTrain_CheckForMissionFail_Save,0);

void SolTrain_CheckForMissionFail::Initialize (U32 eventFlags, const MPartRef & part)
{
	state = Ready;
}

bool SolTrain_CheckForMissionFail::Update (void)
{
    switch (state)
	{
		
	case Ready:
	
        if ( data.mission_over )
        {
            return false;
        }

        
		if(CountObjects(M_ACROPOLIS, PLAYER_ID) == 0 && CountObjects(M_FORGER, PLAYER_ID) == 0)
		{	
			data.mission_over = true;	

			MScript::EnableMovieMode(true);
			UnderMissionControl();
				
           	MScript::FlushStreams();

			data.mhandle = MScript::PlayAnimatedMessage( "STMG39.wav", "Animate!!Elan2", 
				MagpieLeft, MagpieTop, IDS_SOL_SUB_STMG39 );
								
			state = Done;
				
		}
		
		break;
				
	case Done:
		
		if(!MScript::IsStreamPlaying (data.mhandle ))
		{
				
				MScript:: FlushTeletype();
				data.thandle = TeletypeMissionOver( IDS_SOLTRAIN_MISSION_FAILURE );
				
				UnderPlayerControl();
				MScript:: EndMissionDefeat();
			
				return false;
		}
		
		break;
	}	

	return true;
}

//--------------------------------------------------------------------------//
//  OTHER PROGRAMS
//--------------------------------------------------------------------------//
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



//--------------------------------------------------------------------------//
//--End ScriptTest.cpp-------------------------------------------------------//

