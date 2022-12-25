//--------------------------------------------------------------------------//
//                                                                          //
//                                Script00T.cpp                             //
//                                                                          //
//               COPYRIGHT (C) 2000 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
   MANTIS TRAINING MISSION

   Created:  Michael Dula

*/
//--------------------------------------------------------------------------//

#include <ScriptDef.h>
#include <DLLScriptMain.h>
#include "DataDef.h"
#include "Local.h"
#include "stdlib.h"

#include "..\helper\helper.h"

//--------------------------------------------------------------------------//
//  DEFINES																	//
//--------------------------------------------------------------------------//
#define PLAYER_ID			1
#define TERRAN_ID			2

#define VORAAK				1
#define IODE				2


//--------------------------------------------------------------------------//
//  FUNCTION PROTOTYPES
//--------------------------------------------------------------------------//

static void AddAndDisplayObjective
(
    U32 stringID,
    U32 dependantHandle,
    bool isSecondary = false
);

//--------------------------------------------------------------------------


CQSCRIPTDATA(MissionData, data);
//--------------------------------------------------------------------------
//	MISSION BRIEFING
//--------------------------------------------------------------------------

CQSCRIPTPROGRAM(MantisTrain_Briefing, MantisTrain_Briefing_Save,CQPROGFLAG_STARTBRIEFING);

void MantisTrain_Briefing::Initialize (U32 eventFlags, const MPartRef & part)
{
    MScript::PlayMusic( NULL );

	data.mission_over = false;

	data.displayObjectiveHandlerParams_teletypeHandle = 0;

	data.mhandle = 0;
	data.shandle = 0;
	data.mission_state = Briefing;
	state = Begin;
	
	MScript::EnableBriefingControls(true);
}

bool MantisTrain_Briefing::Update (void)
{
	CQBRIEFINGITEM slotAnim;

	switch(state)
	{
		case Begin:

			data.mhandle = MScript::PlayAudio( "prolog17.wav" );
			MScript::BriefingSubtitle(data.mhandle,IDS_MT_SUB_PROLOG17);

			ShowBriefingAnimation( 0, "MantisRebelLeader", 100, true, true );
			ShowBriefingAnimation( 3, "MantisPrisoner", 100, true, true );
			ShowBriefingAnimation( 1, "MantisPrisoner", 100, true, true );
			ShowBriefingAnimation( 2, "MantisPrisoner", 100, true, true );

			data.thandle = MScript::PlayBriefingTeletype(IDS_MT_TELETYPE_LOCATION, 
				STANDARD_TELETYPE_COLOR, STANDARD_TELETYPE_HOLD_TIME, 
                MScript::GetScriptStringLength( IDS_MT_TELETYPE_LOCATION ) * STANDARD_TELETYPE_MILS_PER_CHAR, 
                false );

			state = Radio;

			break;

		case Radio:

			if (!MScript::IsStreamPlaying(data.mhandle))
			{
				strcpy( slotAnim.szFileName, "mtmd03.wav" );
				strcpy( slotAnim.szTypeName, "Animate!!Mordella" );
				
				slotAnim.slotID = 0;
				slotAnim.bHighlite = false;
				data.mhandle = MScript::PlayBriefingTalkingHead(slotAnim);
				MScript::BriefingSubtitle(data.mhandle,IDS_MT_SUB_MTMD03);
				state = Radio2;
			}

			break;

		case Radio2:

			if (!MScript::IsStreamPlaying(data.mhandle))
			{
				MScript::FreeBriefingSlot(0);
				ShowBriefingAnimation( 0, "MantisPrisoner", 100, true, true );

				data.thandle = MScript::PlayBriefingTeletype( IDS_MT_BEGIN,
                    STANDARD_TELETYPE_COLOR, 0, 
                    MScript::GetScriptStringLength( IDS_MT_BEGIN ) * STANDARD_TELETYPE_MILS_PER_CHAR, 
                    false );

				state = Finish;
			}

			break;

		case Finish:

		/*	if (!M_ IsTeletypePlaying(data.mhandle))
				{
					return false;
				}
		*/	
			break;	
	}

	return true;
}


//--------------------------------------------------------------------------//
//  MISSION PROGRAM 														//
//--------------------------------------------------------------------------//

CQSCRIPTPROGRAM(MantisTrain_Start, MantisTrain_Start_Save, CQPROGFLAG_STARTMISSION);

void MantisTrain_Start::Initialize (U32 eventFlags, const MPartRef & part)
{
    MScript::SetMissionID( 0 );

	MScript::SetEnemyCharacter( TERRAN_ID, IDS_MT_ENEMY_NAME );

	// enable regenerating resources

	MScript::EnableRegenMode(true);

	// set available tech

	data.mission_tech.InitLevel(TECHTREE::FULL_TREE);


	// 1 is the Mantis players race

	data.mission_tech.race[1].build = (TECHTREE::BUILDNODE) (TECHTREE::MDEPEND_COCOON);

	// remove warlords from tech tree
/*	data.mission_tech.race[0].common_extra  = (TECHTREE::COMMON_EXTRA) (
		TECHTREE::RES_FIGHTER1 | TECHTREE::RES_FIGHTER2 | TECHTREE::RES_FIGHTER3 |
		TECHTREE::RES_FIGHTER4 | TECHTREE::RES_FIGHTER5 | TECHTREE::RES_SENSORS1 |
		TECHTREE::RES_SENSORS2 | TECHTREE::RES_SENSORS3 | TECHTREE::RES_TANKER1  |
		TECHTREE::RES_TANKER2  | TECHTREE::RES_TANKER3  | TECHTREE::RES_TENDER1  |
		TECHTREE::RES_TENDER2  | TECHTREE::RES_TENDER3 );
*/
	MScript::SetAvailiableTech(data.mission_tech);
	
	data.mission_state = Begin;
	data.MillCreated = data.PlayerInIode = data.PlasmaCreated = false;

	MScript::EnableJumpgate( MScript::GetPartByName( "Wormhole to Iode (Voraak)" ), false );

	// Init triggers for building a Warlord training around earth or swamp planets
	data.WarlordTrig1 = MScript::CreatePart("MISSION!!TRIGGER",MScript::GetPartByName("WarlordTrig1"), 0);
	MScript::EnableTrigger(data.WarlordTrig1, false);
	MScript::SetTriggerRange(data.WarlordTrig1, 4);

	data.WarlordTrig2 = MScript::CreatePart("MISSION!!TRIGGER",MScript::GetPartByName("WarlordTrig2"), 0);
	MScript::EnableTrigger(data.WarlordTrig2, false);
	MScript::SetTriggerRange(data.WarlordTrig2, 4);

	//Init the Sol trigger
	data.TrainTrigger1 = MScript::CreatePart("MISSION!!TRIGGER",MScript::GetPartByName("TriggerPoint"), 0);
	MScript::EnableTrigger(data.TrainTrigger1, false);
	MScript::SetTriggerRange(data.TrainTrigger1, 100);
	//Init the Iota trigger
	data.TrainTrigger2 = MScript::CreatePart("MISSION!!TRIGGER",MScript::GetPartByName("TriggerPoint_I"), 0);
	MScript::EnableTrigger(data.TrainTrigger2, false);
	MScript::SetTriggerRange(data.TrainTrigger2, 100);

			//set resources for player...
	MScript::SetGas(PLAYER_ID, 200);
	MScript::SetMetal(PLAYER_ID, 200);
	MScript::SetCrew(PLAYER_ID, 100);
	MScript::SetMaxCrew(PLAYER_ID, 300);
	MScript::SetMaxGas(PLAYER_ID, 500);
	MScript::SetMaxMetal(PLAYER_ID, 500);

	// set up pointers
	data.Weaver = MScript::GetPartByName("Weaver");
	data.Frig1	= MScript::GetPartByName("Frig1");
	data.Frig2	= MScript::GetPartByName("Frig2");
	data.Cocoon = MScript::GetPartByName("Cocoon");

	data.Corv1 = MScript::GetPartByName("TNS - Caloocan City");
	MScript::EnablePartnameDisplay(data.Corv1, true);

	data.Corv2 = MScript::GetPartByName("TNS - Manila");
	MScript::EnablePartnameDisplay(data.Corv2, true);

	data.Corv3 = MScript::GetPartByName("TNS - Tarlac");
	MScript::EnablePartnameDisplay(data.Corv3, true);

	//disable selection of 2 frigates and Cocoon until necessary
	MScript::EnableSelection( data.Frig1, false );
	MScript::EnableSelection( data.Frig2, false );
	MScript::EnableSelection( data.Cocoon, false );

	MScript::PlayMusic( "mantisgame14.wav" );

	MScript::EndStream(data.mhandle);

	data.mhandle = MScript::PlayAnimatedMessage( "MTMD04.wav", "Animate!!Mordella2",
		MagpieLeft, MagpieTop, IDS_MT_SUB_MTMD04);

	AddAndDisplayObjective( IDS_MT_OBJECTIVE1, data.mhandle );

	MScript::RunProgramByName("MantisTrain_SelectAndMoveWeaver", MPartRef());
	MScript::RunProgramByName("MantisTrain_CheckForMissionFail", MPartRef());

}

bool MantisTrain_Start::Update (void)
{
	return false;
}

//----------------------------------------------------------------------------------------------------
//

CQSCRIPTPROGRAM(MantisTrain_SelectAndMoveWeaver, MantisTrain_SelectAndMoveWeaver_Save, 0);

void MantisTrain_SelectAndMoveWeaver::Initialize (U32 eventFlags, const MPartRef & part)
{
	state = Begin;
}

bool MantisTrain_SelectAndMoveWeaver::Update (void)
{

	switch(state)
	{
		case Begin:

			if (MScript::IsSelectedUnique(data.Weaver) && !MScript::IsSelected(data.Frig1) && !MScript::IsSelected(data.Frig2))
			{
				MScript::MarkObjectiveCompleted(IDS_MT_OBJECTIVE1);
	
				MScript::EndStream(data.mhandle);
				data.mhandle = MScript::PlayAnimatedMessage( "MTMD05.wav", "Animate!!Mordella2",
					MagpieLeft, MagpieTop, IDS_MT_SUB_MTMD05);

				AddAndDisplayObjective( IDS_MT_OBJECTIVE2, data.mhandle );

				state = MoveWeaver;
			}

			break;

		case MoveWeaver:

			// check if player blows up mission critical object and end mission in failure
			if (/*!MScript::IsStreamPlaying(data.mhandle) &&*/ !data.Weaver.isValid() && !data.mission_over)
			{
				data.mission_over = true;

				MScript::EnableMovieMode(true);

				MScript::FlushStreams();

				UnderMissionControl();
				MScript::EndStream( data.mhandle );
				data.mhandle = MScript::PlayAnimatedMessage( "MTMD37.wav", "Animate!!Mordella2", 
					MagpieLeft, MagpieTop, IDS_MT_SUB_MTMD37);

				state = MissionFail;
			}

			if (!MScript::IsIdle(data.Weaver))
			{
				MScript::MarkObjectiveCompleted( IDS_MT_OBJECTIVE2 );
				MScript::EndStream(data.mhandle);

				data.mhandle = MScript::PlayAnimatedMessage( "MTMD06.wav", "Animate!!Mordella2", 
                    MagpieLeft, MagpieTop, IDS_MT_SUB_MTMD06);

				AddAndDisplayObjective( IDS_MT_OBJECTIVE3, data.mhandle );
				
					//enable selection of 2 frigates 
				MScript::EnableSelection( data.Frig1, true );
				MScript::EnableSelection( data.Frig2, true );

				state = SelectAll;
			}

			break;

		case SelectAll:

			// check if player blows up mission critical object and end mission in failure
			if (/*!MScript::IsStreamPlaying(data.mhandle) &&*/ (!data.Weaver.isValid() || !data.Frig1.isValid() || !data.Frig2.isValid() )
				&& !data.mission_over)
			{
				data.mission_over = true;

				MScript::EnableMovieMode(true);

				MScript::FlushStreams();

				UnderMissionControl();
				MScript::EndStream( data.mhandle );
				data.mhandle = MScript::PlayAnimatedMessage( "MTMD37.wav", "Animate!!Mordella2", 
					MagpieLeft, MagpieTop, IDS_MT_SUB_MTMD37);

				state = MissionFail;
			}

			if (!MScript::IsStreamPlaying(data.mhandle) && MScript::IsSelected(data.Weaver) && MScript::IsSelected(data.Frig1) && M_ IsSelected(data.Frig2))
			{
				MScript::EndStream (data.mhandle);

				MScript::MarkObjectiveCompleted( IDS_MT_OBJECTIVE3 );

				data.mhandle = MScript::PlayAnimatedMessage( "MTMD07.wav", "Animate!!Mordella2", 
                    MagpieLeft, MagpieTop, IDS_MT_SUB_MTMD07);

				AddAndDisplayObjective( IDS_MT_OBJECTIVE4, data.mhandle );
				
				MScript::EnableSelection( data.Cocoon, true );

				state = FOWuncovered;
			}

			break;

		case FOWuncovered:

			if (!MScript::IsStreamPlaying( data.mhandle ) && (!MScript::IsIdle(data.Frig1) || !MScript::IsIdle(data.Frig2) || 
				MScript::IsVisibleToPlayer(MScript::GetPartByName("Voraak 2"), PLAYER_ID) ||
				MScript::IsVisibleToPlayer(MScript::GetPartByName("Voraak 3"), PLAYER_ID) ||
				MScript::IsVisibleToPlayer(MScript::GetPartByName("Voraak 4"), PLAYER_ID) ||
				MScript::IsVisibleToPlayer(MScript::GetPartByName("Voraak 5"), PLAYER_ID)) )
			{
				MScript::EndStream(data.mhandle);
				//M_ FlushTeletype();
				MScript::MarkObjectiveCompleted( IDS_MT_OBJECTIVE4 );

				data.mhandle = MScript::PlayAnimatedMessage( "MTMD08.wav", "Animate!!Mordella2", 
                    MagpieLeft, MagpieTop, IDS_MT_SUB_MTMD08);

				AddAndDisplayObjective( IDS_MT_OBJECTIVE5, data.mhandle );

				data.mission_tech.race[1].build = TECHTREE::BUILDNODE( data.mission_tech.race[1].build |
					TECHTREE::MDEPEND_COLLECTOR );

				MScript::SetAvailiableTech( data.mission_tech );

				state = StartBuild;
			}

			break;

		case StartBuild:

			if (!MScript::IsTeletypePlaying(data.mhandle))
			{

				MScript::EnableTrigger(data.TrainTrigger1, true);

				//setup trigger to detect when collector is started
				MScript::SetTriggerProgram(data.TrainTrigger1, "MantisTrain_CollectorInProgress");

				// Set trigger to detect a collector in process or otherwise from player 1.
				MScript::SetTriggerFilter(data.TrainTrigger1, M_COLLECTOR, TRIGGER_MOBJCLASS, false);
				MScript::SetTriggerFilter(data.TrainTrigger1, 0, TRIGGER_NOFORCEREADY, true);
				MScript::SetTriggerFilter(data.TrainTrigger1, PLAYER_ID, TRIGGER_PLAYER, true);

				
				return false;
			}

			break;

		case MissionFail:

			if (!MScript::IsStreamPlaying( data.mhandle ) )
			{
				MScript::FlushTeletype();
				data.thandle = TeletypeMissionOver(IDS_MT_MISSION_FAILURE);

				UnderPlayerControl();
				MScript::EndMissionDefeat();

				return false;
			}

			break;
	
	}

	return true;
}

//--------------------------------------------------------------------------//
//	Called by the trigger when the user starts building a Collector 

CQSCRIPTPROGRAM(MantisTrain_CollectorInProgress, MantisTrain_CollectorInProgress_Save,0);

void MantisTrain_CollectorInProgress::Initialize (U32 eventFlags, const MPartRef & part)
{

	//setup trigger to detect when Collector is finished
	MScript::SetTriggerProgram(data.TrainTrigger1, "MantisTrain_CollectorFinished");

	// detect a Collector finished player 1.
	MScript::SetTriggerFilter(data.TrainTrigger1, M_COLLECTOR, TRIGGER_MOBJCLASS, false);
	MScript::SetTriggerFilter(data.TrainTrigger1, 0, TRIGGER_FORCEREADY, true);
	MScript::SetTriggerFilter(data.TrainTrigger1, PLAYER_ID, TRIGGER_PLAYER, true);

	data.Collector = MScript::GetLastTriggerObject(part);
 
	MScript::EndStream(data.mhandle);
	data.mhandle = MScript::PlayAnimatedMessage( "MTMD09.wav", "Animate!!Mordella2", 
		MagpieLeft, MagpieTop, IDS_MT_SUB_MTMD09);

}

bool MantisTrain_CollectorInProgress::Update (void)
{
	if (data.mission_over)
	{
		 return false;
	}

	if (!data.Collector.isValid())
	{
		return false;	// construction canceled, abort this loop
	}

	return false;
}

//--------------------------------------------------------------------------//
//	

CQSCRIPTPROGRAM(MantisTrain_CollectorFinished, MantisTrain_CollectorFinished_Save,0);

void MantisTrain_CollectorFinished::Initialize (U32 eventFlags, const MPartRef & part)
{
	MScript::MarkObjectiveCompleted( IDS_MT_OBJECTIVE5 );

	//setup trigger to detect when Siphon is in progress
	MScript::SetTriggerProgram(data.TrainTrigger1, "MantisTrain_SiphonInProgress");

	// detect a Siphon in progress player 1.
	MScript::SetTriggerFilter(data.TrainTrigger1, M_SIPHON, TRIGGER_MOBJCLASS, false);
	MScript::SetTriggerFilter(data.TrainTrigger1, 0, TRIGGER_NOFORCEREADY, true);
	MScript::SetTriggerFilter(data.TrainTrigger1, PLAYER_ID, TRIGGER_PLAYER, true);
 
	MScript::EndStream(data.mhandle);
	data.mhandle = MScript::PlayAnimatedMessage( "MTMD10.wav", "Animate!!Mordella2", 
		MagpieLeft, MagpieTop, IDS_MT_SUB_MTMD10);

	AddAndDisplayObjective( IDS_MT_OBJECTIVE6, data.mhandle );
}

bool MantisTrain_CollectorFinished::Update (void)
{
	return false;
}

//--------------------------------------------------------------------------//
//	 

CQSCRIPTPROGRAM(MantisTrain_SiphonInProgress, MantisTrain_SiphonInProgress_Save,0);

void MantisTrain_SiphonInProgress::Initialize (U32 eventFlags, const MPartRef & part)
{

	//setup trigger to detect when Siphon is finished
	MScript::SetTriggerProgram(data.TrainTrigger1, "MantisTrain_SiphonFinished");

	// detect a Siphon finished player 1.
	MScript::SetTriggerFilter(data.TrainTrigger1, M_SIPHON, TRIGGER_MOBJCLASS, false);
	MScript::SetTriggerFilter(data.TrainTrigger1, 0, TRIGGER_FORCEREADY, true);
	MScript::SetTriggerFilter(data.TrainTrigger1, PLAYER_ID, TRIGGER_PLAYER, true);
 
	MScript::EndStream(data.mhandle);
	data.mhandle = MScript::PlayAnimatedMessage( "MTMD11.wav", "Animate!!Mordella2", 
		MagpieLeft, MagpieTop, IDS_MT_SUB_MTMD11);

	MScript::EnableTrigger(data.WarlordTrig1, true);
	MScript::EnableTrigger(data.WarlordTrig2, true);

}

bool MantisTrain_SiphonInProgress::Update (void)
{
	return false;
}

//--------------------------------------------------------------------------//
//	 
CQSCRIPTPROGRAM(MantisTrain_SiphonFinished, MantisTrain_SiphonFinished_Save,0);

void MantisTrain_SiphonFinished::Initialize (U32 eventFlags, const MPartRef & part)
{
	MScript::MarkObjectiveCompleted( IDS_MT_OBJECTIVE6 );
	MScript::EnableTrigger(data.TrainTrigger1, false);
/*	
	//setup trigger to detect when Warlord TG is in progress
	MScript::SetTriggerProgram(data.TrainTrigger1, "MantisTrain_WarlordInProgress");

	// detect a Warlord TG in progress player 1.
	MScript::SetTriggerFilter(data.TrainTrigger1, M_WARLORDTRAINING, TRIGGER_MOBJCLASS, false);
	MScript::SetTriggerFilter(data.TrainTrigger1, 0, TRIGGER_NOFORCEREADY, true);
	MScript::SetTriggerFilter(data.TrainTrigger1, PLAYER_ID, TRIGGER_PLAYER, true);
*/
	//setup trigger to detect when Warlord TG is in progress
	MScript::SetTriggerProgram(data.WarlordTrig1, "MantisTrain_WarlordInProgress");

	// detect a Warlord TG in progress player 1.
	MScript::SetTriggerFilter(data.WarlordTrig1, M_WARLORDTRAINING, TRIGGER_MOBJCLASS, false);
	MScript::SetTriggerFilter(data.WarlordTrig1, 0, TRIGGER_NOFORCEREADY, true);
	MScript::SetTriggerFilter(data.WarlordTrig1, PLAYER_ID, TRIGGER_PLAYER, true);

	//setup trigger to detect when Warlord TG is in progress
	MScript::SetTriggerProgram(data.WarlordTrig2, "MantisTrain_WarlordInProgress");

	// detect a Warlord TG in progress player 1.
	MScript::SetTriggerFilter(data.WarlordTrig2, M_WARLORDTRAINING, TRIGGER_MOBJCLASS, false);
	MScript::SetTriggerFilter(data.WarlordTrig2, 0, TRIGGER_NOFORCEREADY, true);
	MScript::SetTriggerFilter(data.WarlordTrig2, PLAYER_ID, TRIGGER_PLAYER, true);

	data.mission_tech.race[1].build = TECHTREE::BUILDNODE( data.mission_tech.race[1].build |
		TECHTREE::MDEPEND_WARLORDTRAIN );

	MScript::SetAvailiableTech( data.mission_tech );

	MScript::EndStream(data.mhandle);
	data.mhandle = MScript::PlayAnimatedMessage( "MTMD12A.wav", "Animate!!Mordella2", 
		MagpieLeft, MagpieTop, IDS_MT_SUB_MTMD12A);

	AddAndDisplayObjective( IDS_MT_OBJECTIVE7, data.mhandle );
}

bool MantisTrain_SiphonFinished::Update (void)
{
	return false;
}

//--------------------------------------------------------------------------//
//	 

CQSCRIPTPROGRAM(MantisTrain_WarlordInProgress, MantisTrain_WarlordInProgress_Save,0);

void MantisTrain_WarlordInProgress::Initialize (U32 eventFlags, const MPartRef & part)
{
/* 
	//setup trigger to detect when Warlord TG is finished
	MScript::SetTriggerProgram(data.TrainTrigger1, "MantisTrain_WarlordFinished");

	// detect a Warlord TG finished player 1.
	MScript::SetTriggerFilter(data.TrainTrigger1, M_WARLORDTRAINING, TRIGGER_MOBJCLASS, false);
	MScript::SetTriggerFilter(data.TrainTrigger1, 0, TRIGGER_FORCEREADY, true);
	MScript::SetTriggerFilter(data.TrainTrigger1, PLAYER_ID, TRIGGER_PLAYER, true);
*/

	//setup trigger to detect when Warlord TG is finished
	MScript::SetTriggerProgram(data.WarlordTrig1, "MantisTrain_WarlordFinished");

	// detect a Warlord TG finished player 1.
	MScript::SetTriggerFilter(data.WarlordTrig1, M_WARLORDTRAINING, TRIGGER_MOBJCLASS, false);
	MScript::SetTriggerFilter(data.WarlordTrig1, 0, TRIGGER_FORCEREADY, true);
	MScript::SetTriggerFilter(data.WarlordTrig1, PLAYER_ID, TRIGGER_PLAYER, true);

	//setup trigger to detect when Warlord TG is finished
	MScript::SetTriggerProgram(data.WarlordTrig2, "MantisTrain_WarlordFinished");

	// detect a Warlord TG finished player 1.
	MScript::SetTriggerFilter(data.WarlordTrig2, M_WARLORDTRAINING, TRIGGER_MOBJCLASS, false);
	MScript::SetTriggerFilter(data.WarlordTrig2, 0, TRIGGER_FORCEREADY, true);
	MScript::SetTriggerFilter(data.WarlordTrig2, PLAYER_ID, TRIGGER_PLAYER, true);

	MScript::EndStream(data.mhandle);
	data.mhandle = MScript::PlayAnimatedMessage( "MTMD13A.wav", "Animate!!Mordella2", 
		MagpieLeft, MagpieTop, IDS_MT_SUB_MTMD13A);

}

bool MantisTrain_WarlordInProgress::Update (void)
{
	if (!MScript::IsStreamPlaying( data.mhandle ) )
	{
		data.mhandle = MScript::PlayAnimatedMessage( "MTMD32.wav", "Animate!!Mordella2", 
			MagpieLeft, MagpieTop, IDS_MT_SUB_MTMD32);
		return false;
	}
	return true;
}


//--------------------------------------------------------------------------//
//	 

CQSCRIPTPROGRAM(MantisTrain_WarlordFinished, MantisTrain_WarlordFinished_Save,0);

void MantisTrain_WarlordFinished::Initialize (U32 eventFlags, const MPartRef & part)
{
	MScript::EnableTrigger(data.TrainTrigger1, true);
	MScript::EnableTrigger(data.WarlordTrig1, false);
	MScript::EnableTrigger(data.WarlordTrig2, false);

	MScript::MarkObjectiveCompleted( IDS_MT_OBJECTIVE7 );

	//setup trigger to detect when Eye Stalk is finished
	MScript::SetTriggerProgram(data.TrainTrigger1, "MantisTrain_EyestalkInProgress");

	// detect a Eye Stalk finished player 1.
	MScript::SetTriggerFilter(data.TrainTrigger1, M_EYESTOCK, TRIGGER_MOBJCLASS, false);
	MScript::SetTriggerFilter(data.TrainTrigger1, 0, TRIGGER_NOFORCEREADY, true);
	MScript::SetTriggerFilter(data.TrainTrigger1, PLAYER_ID, TRIGGER_PLAYER, true);

	data.mission_tech.race[1].build = TECHTREE::BUILDNODE( data.mission_tech.race[1].build | 
        TECHTREE::MDEPEND_EYESTOCK );
	
	MScript::SetAvailiableTech( data.mission_tech );

	MScript::EndStream(data.mhandle);
	data.mhandle = MScript::PlayAnimatedMessage( "MTMD14.wav", "Animate!!Mordella2", 
		MagpieLeft, MagpieTop, IDS_MT_SUB_MTMD14);

	AddAndDisplayObjective( IDS_MT_OBJECTIVE8, data.mhandle );

}

bool MantisTrain_WarlordFinished::Update (void)
{
	return false;
}

//--------------------------------------------------------------------------//
//	 

CQSCRIPTPROGRAM(MantisTrain_EyestalkInProgress, MantisTrain_EyestalkInProgress_Save,0);

void MantisTrain_EyestalkInProgress::Initialize (U32 eventFlags, const MPartRef & part)
{

	MScript::MarkObjectiveCompleted( IDS_MT_OBJECTIVE7 );

	//setup trigger to detect when HQ is finished
	MScript::SetTriggerProgram(data.TrainTrigger1, "MantisTrain_EyestalkFinished");

	// detect a HQ finished player 1.
	MScript::SetTriggerFilter(data.TrainTrigger1, M_EYESTOCK, TRIGGER_MOBJCLASS, false);
	MScript::SetTriggerFilter(data.TrainTrigger1, 0, TRIGGER_FORCEREADY, true);
	MScript::SetTriggerFilter(data.TrainTrigger1, PLAYER_ID, TRIGGER_PLAYER, true);

	MScript::EndStream(data.mhandle);
	data.mhandle = MScript::PlayAnimatedMessage( "MTMD15.wav", "Animate!!Mordella2", 
		MagpieLeft, MagpieTop, IDS_MT_SUB_MTMD15);
}

bool MantisTrain_EyestalkInProgress::Update (void)
{
	return false;
}


//--------------------------------------------------------------------------//
//	 

CQSCRIPTPROGRAM(MantisTrain_EyestalkFinished, MantisTrain_EyestalkFinished_Save,0);

void MantisTrain_EyestalkFinished::Initialize (U32 eventFlags, const MPartRef & part)
{

	MScript::MarkObjectiveCompleted( IDS_MT_OBJECTIVE8 );

	// clean up objectives queue
	MScript::RemoveFromObjectiveList( IDS_MT_OBJECTIVE1 );
    MScript::RemoveFromObjectiveList( IDS_MT_OBJECTIVE2 );
    MScript::RemoveFromObjectiveList( IDS_MT_OBJECTIVE3 );
	MScript::RemoveFromObjectiveList( IDS_MT_OBJECTIVE4 );
    MScript::RemoveFromObjectiveList( IDS_MT_OBJECTIVE5 );
    
	//setup trigger to detect when Thripid is in progress
	MScript::SetTriggerProgram(data.TrainTrigger1, "MantisTrain_ThripidInProgress");

	// detect a Thripid in progress player 1.
	MScript::SetTriggerFilter(data.TrainTrigger1, M_THRIPID, TRIGGER_MOBJCLASS, false);
	MScript::SetTriggerFilter(data.TrainTrigger1, 0, TRIGGER_NOFORCEREADY, true);
	MScript::SetTriggerFilter(data.TrainTrigger1, PLAYER_ID, TRIGGER_PLAYER, true);

	data.mission_tech.race[1].build = TECHTREE::BUILDNODE( data.mission_tech.race[1].build | 
        TECHTREE::MDEPEND_THRIPID );

	MScript::SetAvailiableTech( data.mission_tech );
	
	MScript::EndStream(data.mhandle);
	data.mhandle = MScript::PlayAnimatedMessage( "MTMD16.wav", "Animate!!Mordella2", 
		MagpieLeft, MagpieTop, IDS_MT_SUB_MTMD16);

	AddAndDisplayObjective( IDS_MT_OBJECTIVE9, data.mhandle );

}

bool MantisTrain_EyestalkFinished::Update (void)
{
	return false;
}

//--------------------------------------------------------------------------//
//	 

CQSCRIPTPROGRAM(MantisTrain_ThripidInProgress, MantisTrain_ThripidInProgress_Save,0);

void MantisTrain_ThripidInProgress::Initialize (U32 eventFlags, const MPartRef & part)
{

	//setup trigger to detect when HQ is finished
	MScript::SetTriggerProgram(data.TrainTrigger1, "MantisTrain_ThripidFinished");

	// detect a HQ finished player 1.
	MScript::SetTriggerFilter(data.TrainTrigger1, M_THRIPID, TRIGGER_MOBJCLASS, false);
	MScript::SetTriggerFilter(data.TrainTrigger1, 0, TRIGGER_FORCEREADY, true);
	MScript::SetTriggerFilter(data.TrainTrigger1, PLAYER_ID, TRIGGER_PLAYER, true);
 
	MScript::EndStream(data.mhandle);
	data.mhandle = MScript::PlayAnimatedMessage( "MTMD17.wav", "Animate!!Mordella2", 
		MagpieLeft, MagpieTop, IDS_MT_SUB_MTMD17);
}

bool MantisTrain_ThripidInProgress::Update (void)
{
	return false;
}


//--------------------------------------------------------------------------//
//	 

CQSCRIPTPROGRAM(MantisTrain_ThripidFinished, MantisTrain_ThripidFinished_Save,0);

void MantisTrain_ThripidFinished::Initialize (U32 eventFlags, const MPartRef & part)
{
	data.mission_state = BlastFirst;

	MScript::MarkObjectiveCompleted( IDS_MT_OBJECTIVE9 );

	MScript::EndStream(data.mhandle);
	data.mhandle = MScript::PlayAnimatedMessage( "MTMD18A.wav", "Animate!!Mordella2", 
		MagpieLeft, MagpieTop, IDS_MT_SUB_MTMD18A);

	AddAndDisplayObjective( IDS_MT_OBJECTIVE10, data.mhandle );

	//setup trigger to detect when Blast Furnace is in progress
	MScript::SetTriggerProgram(data.TrainTrigger1, "MantisTrain_BlastInProgress");

	// detect a Blast Furnace in progress player 1.
	MScript::SetTriggerFilter(data.TrainTrigger1, M_BLASTFURNACE, TRIGGER_MOBJCLASS, false);
	MScript::SetTriggerFilter(data.TrainTrigger1, 0, TRIGGER_NOFORCEREADY, true);
	MScript::SetTriggerFilter(data.TrainTrigger1, PLAYER_ID, TRIGGER_PLAYER, true);
	
	data.mission_tech.race[1].build = TECHTREE::BUILDNODE( data.mission_tech.race[1].build | 
        TECHTREE::MDEPEND_BLASTFURNACE );

	MScript::SetAvailiableTech( data.mission_tech );

}

bool MantisTrain_ThripidFinished::Update (void)
{
	return false;
}

CQSCRIPTPROGRAM(MantisTrain_ObjectBuilt, MantisTrain_ObjectBuilt_Save, CQPROGFLAG_OBJECTCONSTRUCTED);

void MantisTrain_ObjectBuilt::Initialize (U32 eventFlags, const MPartRef & part)
{	

	switch (part->mObjClass)
	{
		case M_HIVECARRIER:

			if(CountObjects(M_HIVECARRIER, PLAYER_ID) >= 6 && data.mission_state == Six_Hives)
			{

				MScript::EndStream( data.mhandle );
				data.mhandle = MScript::PlayAnimatedMessage( "MTMD23.wav", "Animate!!Mordella2", 
					MagpieLeft, MagpieTop, IDS_MT_SUB_MTMD23);

				MScript::MarkObjectiveCompleted( IDS_MT_OBJECTIVE12 );

				AddAndDisplayObjective( IDS_MT_OBJECTIVE13, data.mhandle );

				//setup trigger to detect when Plantation is in progress
				MScript::EnableTrigger(data.TrainTrigger1, true);
				MScript::SetTriggerProgram(data.TrainTrigger1, "MantisTrain_PlantInProgress");

				// detect a Plantation in progress player 1.
				MScript::SetTriggerFilter(data.TrainTrigger1, M_PLANTATION, TRIGGER_MOBJCLASS, false);
				MScript::SetTriggerFilter(data.TrainTrigger1, 0, TRIGGER_NOFORCEREADY, true);
				MScript::SetTriggerFilter(data.TrainTrigger1, PLAYER_ID, TRIGGER_PLAYER, true);

				data.mission_tech.race[1].build = TECHTREE::BUILDNODE( data.mission_tech.race[1].build | 
					TECHTREE::MDEPEND_PLANTATION );

				MScript::SetAvailiableTech( data.mission_tech );
		
				data.mission_state = Six_Built;

			}

			break;
	}

}

bool MantisTrain_ObjectBuilt::Update (void)
{
	return false;
}

//--------------------------------------------------------------------------//
//	 

CQSCRIPTPROGRAM(MantisTrain_BlastInProgress, MantisTrain_BlastInProgress_Save,0);

void MantisTrain_BlastInProgress::Initialize (U32 eventFlags, const MPartRef & part)
{

	//setup trigger to detect when HQ is finished
	MScript::SetTriggerProgram(data.TrainTrigger1, "MantisTrain_BlastFinished");

	// detect a HQ finished player 1.
	MScript::SetTriggerFilter(data.TrainTrigger1, M_BLASTFURNACE, TRIGGER_MOBJCLASS, false);
	MScript::SetTriggerFilter(data.TrainTrigger1, 0, TRIGGER_FORCEREADY, true);
	MScript::SetTriggerFilter(data.TrainTrigger1, PLAYER_ID, TRIGGER_PLAYER, true);

}

bool MantisTrain_BlastInProgress::Update (void)
{
	return false;
}

//--------------------------------------------------------------------------//
//	 

CQSCRIPTPROGRAM(MantisTrain_BlastFinished, MantisTrain_BlastFinished_Save,0);

void MantisTrain_BlastFinished::Initialize (U32 eventFlags, const MPartRef & part)
{
	data.mission_state = BlastDone;

	MScript::MarkObjectiveCompleted( IDS_MT_OBJECTIVE10 );

	MScript::EnableTrigger(data.TrainTrigger1, false);

	Spawn = MScript::GetPartByName("TerranSpawn");

	state = Begin;

	data.mission_tech.race[1].build = TECHTREE::BUILDNODE( data.mission_tech.race[1].build |  
		TECHTREE::MDEPEND_PLASMA_SPITTER );

	MScript::SetAvailiableTech( data.mission_tech );
}

bool MantisTrain_BlastFinished::Update (void)
{

	if (data.mission_over)
	{
		 return false;
	}

	switch(state)
	{
		case Begin:

			if(CountObjects(M_FRIGATE, PLAYER_ID) >= 1 || MScript::PlayerHasUnits(PLAYER_ID))
			{/*
				Attacker = MScript::CreatePart("GBOAT!!T_Corvette", Spawn, TERRAN_ID);
				MScript::SetStance (Attacker, US_ATTACK);
				MScript::OrderMoveTo(Attacker, MScript::GetPartByName("Move"));

				Attacker2 = MScript::CreatePart("GBOAT!!T_Corvette", Spawn, TERRAN_ID);
				MScript::SetStance (Attacker2, US_ATTACK);
				MScript::OrderMoveTo(Attacker2, MScript::GetPartByName("Move"));

				Attacker3 = MScript::CreatePart("GBOAT!!T_Corvette", Spawn, TERRAN_ID);
				MScript::SetStance (Attacker3, US_ATTACK);
				MScript::OrderMoveTo(Attacker3, MScript::GetPartByName("Move"));
				*/
				MScript::SetStance (data.Corv1, US_ATTACK);
				MScript::SetStance (data.Corv2, US_ATTACK);
				MScript::SetStance (data.Corv3, US_ATTACK);

				MScript::OrderMoveTo(data.Corv1, MScript::GetPartByName("Move"));
				MScript::OrderMoveTo(data.Corv2, MScript::GetPartByName("Move"));
				MScript::OrderMoveTo(data.Corv3, MScript::GetPartByName("Move"));
			
				if (data.Cocoon.isValid())
				{
					MScript::OrderAttack(data.Corv1, data.Cocoon, true);
					MScript::OrderAttack(data.Corv2, data.Cocoon, true);
					MScript::OrderAttack(data.Corv3, data.Cocoon, true);
				}

				state = Attack;
			}

			break;

		case Attack:

			if(MScript::IsVisibleToPlayer(data.Corv1, PLAYER_ID))
			{
				//setup trigger to detect when Spitter is in progress
				MScript::EnableTrigger(data.TrainTrigger1, true);
				MScript::SetTriggerRange(data.TrainTrigger1, 100 );
				MScript::SetTriggerProgram(data.TrainTrigger1, "MantisTrain_SpitterInProgress");

				// detect a Spitter in progress player 1.
				MScript::SetTriggerFilter(data.TrainTrigger1, M_PLASMASPLITTER, TRIGGER_MOBJCLASS, false);
				MScript::SetTriggerFilter(data.TrainTrigger1, 0, TRIGGER_NOFORCEREADY, true);
				MScript::SetTriggerFilter(data.TrainTrigger1, PLAYER_ID, TRIGGER_PLAYER, true);

				MScript::EndStream(data.mhandle);
				data.mhandle = MScript::PlayAnimatedMessage( "MTMD20.wav", "Animate!!Mordella2", 
					MagpieLeft, MagpieTop, IDS_MT_SUB_MTMD20);

				AddAndDisplayObjective( IDS_MT_OBJECTIVE11, data.mhandle );

				return false;
			}

			break;
	}

	return true;
}

//--------------------------------------------------------------------------//
//	 

CQSCRIPTPROGRAM(MantisTrain_SpitterInProgress, MantisTrain_SpitterInProgress_Save,0);

void MantisTrain_SpitterInProgress::Initialize (U32 eventFlags, const MPartRef & part)
{

	//setup trigger to detect when Spitter is finished
	MScript::SetTriggerProgram(data.TrainTrigger1, "MantisTrain_SpitterFinished");

	// detect a Spitter finished player 1.
	MScript::SetTriggerFilter(data.TrainTrigger1, M_PLASMASPLITTER, TRIGGER_MOBJCLASS, false);
	MScript::SetTriggerFilter(data.TrainTrigger1, 0, TRIGGER_FORCEREADY, true);
	MScript::SetTriggerFilter(data.TrainTrigger1, PLAYER_ID, TRIGGER_PLAYER, true);
 
	MScript::EndStream( data.mhandle );
    data.mhandle = MScript::PlayAnimatedMessage( "MTMD21.wav", "Animate!!Mordella2", 
		MagpieLeft, MagpieTop, IDS_MT_SUB_MTMD21);
}

bool MantisTrain_SpitterInProgress::Update (void)
{
	return false;
}

//--------------------------------------------------------------------------//
//	 

CQSCRIPTPROGRAM(MantisTrain_SpitterFinished, MantisTrain_SpitterFinished_Save,0);

void MantisTrain_SpitterFinished::Initialize (U32 eventFlags, const MPartRef & part)
{

	MScript::MarkObjectiveCompleted( IDS_MT_OBJECTIVE11 );

	MScript::EnableTrigger(data.TrainTrigger1, false);
	MScript::SetTriggerRange(data.TrainTrigger1, 100);

	MScript::EndStream( data.mhandle );
    data.mhandle = MScript::PlayAnimatedMessage( "MTMD21B.wav", "Animate!!Mordella2", 
		MagpieLeft, MagpieTop, IDS_MT_SUB_MTMD21B);

//	AddAndDisplayObjective( IDS_MT_OBJECTIVE12, data.mhandle );
	AddAndDisplayObjective( IDS_MT_OBJECTIVE12a, data.mhandle, true );


	data.mission_tech.race[1].build = TECHTREE::BUILDNODE( data.mission_tech.race[1].build |  
		TECHTREE::MDEPEND_CARRIONROOST );

	MScript::SetAvailiableTech( data.mission_tech );

	//setup trigger to detect when Carrion is in progress
	MScript::EnableTrigger(data.TrainTrigger1, true);
	MScript::SetTriggerRange(data.TrainTrigger1, 100 );
	MScript::SetTriggerProgram(data.TrainTrigger1, "MantisTrain_CarrionInProgress");

	// detect a Spitter in progress player 1.
	MScript::SetTriggerFilter(data.TrainTrigger1, M_CARRIONROOST, TRIGGER_MOBJCLASS, false);
	MScript::SetTriggerFilter(data.TrainTrigger1, 0, TRIGGER_NOFORCEREADY, true);
	MScript::SetTriggerFilter(data.TrainTrigger1, PLAYER_ID, TRIGGER_PLAYER, true);

}

bool MantisTrain_SpitterFinished::Update (void)
{
	return false;
}
//--------------------------------------------------------------------------//
//	 

CQSCRIPTPROGRAM(MantisTrain_CarrionInProgress, MantisTrain_CarrionInProgress_Save,0);

void MantisTrain_CarrionInProgress::Initialize (U32 eventFlags, const MPartRef & part)
{

	//setup trigger to detect when Spitter is finished
	MScript::SetTriggerProgram(data.TrainTrigger1, "MantisTrain_CarrionFinished");

	// detect a Spitter finished player 1.
	MScript::SetTriggerFilter(data.TrainTrigger1, M_CARRIONROOST, TRIGGER_MOBJCLASS, false);
	MScript::SetTriggerFilter(data.TrainTrigger1, 0, TRIGGER_FORCEREADY, true);
	MScript::SetTriggerFilter(data.TrainTrigger1, PLAYER_ID, TRIGGER_PLAYER, true);
}

bool MantisTrain_CarrionInProgress::Update (void)
{
	return false;
}

//--------------------------------------------------------------------------//
//	 

CQSCRIPTPROGRAM(MantisTrain_CarrionFinished, MantisTrain_CarrionFinished_Save,0);

void MantisTrain_CarrionFinished::Initialize (U32 eventFlags, const MPartRef & part)
{

	MScript::MarkObjectiveCompleted( IDS_MT_OBJECTIVE12a );

	MScript::EnableTrigger(data.TrainTrigger1, false);
	MScript::SetTriggerRange(data.TrainTrigger1, 100);

	MScript::EndStream( data.mhandle );
    data.mhandle = MScript::PlayAnimatedMessage( "MTMD22.wav", "Animate!!Mordella2", 
		MagpieLeft, MagpieTop, IDS_MT_SUB_MTMD22);

	AddAndDisplayObjective( IDS_MT_OBJECTIVE12, data.mhandle );

	MScript::RunProgramByName("MantisTrain_SixScouts", MPartRef());
}

bool MantisTrain_CarrionFinished::Update (void)
{
	return false;
}

//--------------------------------------------------------------------------//
//	 

CQSCRIPTPROGRAM(MantisTrain_SixScouts, MantisTrain_SixScouts_Save,0);

void MantisTrain_SixScouts::Initialize (U32 eventFlags, const MPartRef & part)
{

	data.mission_state = Six_Hives;

		// clean up objectives queue
	MScript::RemoveFromObjectiveList( IDS_MT_OBJECTIVE6 );
    MScript::RemoveFromObjectiveList( IDS_MT_OBJECTIVE7 );
    MScript::RemoveFromObjectiveList( IDS_MT_OBJECTIVE8 );
	MScript::RemoveFromObjectiveList( IDS_MT_OBJECTIVE9 );
    MScript::RemoveFromObjectiveList( IDS_MT_OBJECTIVE10 );

	// check if already have 6 built
	if ((CountObjects(M_HIVECARRIER, PLAYER_ID) >= 6))
	{
				MScript::EndStream( data.mhandle );
				data.mhandle = MScript::PlayAnimatedMessage( "MTMD23.wav", "Animate!!Mordella2", 
					MagpieLeft, MagpieTop, IDS_MT_SUB_MTMD23);

				MScript::MarkObjectiveCompleted( IDS_MT_OBJECTIVE12 );

				AddAndDisplayObjective( IDS_MT_OBJECTIVE13, data.mhandle );

				//setup trigger to detect when Plantation is in progress
				MScript::EnableTrigger(data.TrainTrigger1, true);
				MScript::SetTriggerProgram(data.TrainTrigger1, "MantisTrain_PlantInProgress");

				// detect a Plantation in progress player 1.
				MScript::SetTriggerFilter(data.TrainTrigger1, M_PLANTATION, TRIGGER_MOBJCLASS, false);
				MScript::SetTriggerFilter(data.TrainTrigger1, 0, TRIGGER_NOFORCEREADY, true);
				MScript::SetTriggerFilter(data.TrainTrigger1, PLAYER_ID, TRIGGER_PLAYER, true);

				data.mission_tech.race[1].build = TECHTREE::BUILDNODE( data.mission_tech.race[1].build | 
					TECHTREE::MDEPEND_PLANTATION );

				MScript::SetAvailiableTech( data.mission_tech );
		
				data.mission_state = Six_Built;

	}

}

bool MantisTrain_SixScouts::Update (void)
{
	return false;
}

//--------------------------------------------------------------------------//
//	 

CQSCRIPTPROGRAM(MantisTrain_PlantInProgress, MantisTrain_PlantInProgress_Save,0);

void MantisTrain_PlantInProgress::Initialize (U32 eventFlags, const MPartRef & part)
{

	//setup trigger to detect when Plantation is finished
	MScript::SetTriggerProgram(data.TrainTrigger1, "MantisTrain_PlantationFinished");

	// detect a Plantation finished player 1.
	MScript::SetTriggerFilter(data.TrainTrigger1, M_PLANTATION, TRIGGER_MOBJCLASS, false);
	MScript::SetTriggerFilter(data.TrainTrigger1, 0, TRIGGER_FORCEREADY, true);
	MScript::SetTriggerFilter(data.TrainTrigger1, PLAYER_ID, TRIGGER_PLAYER, true);
 
	MScript::EndStream( data.mhandle );
	data.mhandle = MScript::PlayAnimatedMessage( "MTMD24.wav", "Animate!!Mordella2", 
		MagpieLeft, MagpieTop, IDS_MT_SUB_MTMD24);
	
}

bool MantisTrain_PlantInProgress::Update (void)
{
	return false;
}

//--------------------------------------------------------------------------//
//	 

CQSCRIPTPROGRAM(MantisTrain_PlantationFinished, MantisTrain_PlantationFinished_Save,0);

void MantisTrain_PlantationFinished::Initialize (U32 eventFlags, const MPartRef & part)
{

	MScript::MarkObjectiveCompleted( IDS_MT_OBJECTIVE13 );

	//setup trigger to detect when Jump Gate is in progress
	MScript::SetTriggerProgram(data.TrainTrigger1, "MantisTrain_JumpgateInProgress");

	// detect a Jump Gate in progress player 1.
	MScript::SetTriggerFilter(data.TrainTrigger1, M_JUMPPLAT, TRIGGER_MOBJCLASS, false);
	MScript::SetTriggerFilter(data.TrainTrigger1, 0, TRIGGER_NOFORCEREADY, true);
	MScript::SetTriggerFilter(data.TrainTrigger1, PLAYER_ID, TRIGGER_PLAYER, true);
 
	data.mission_tech.race[1].build = TECHTREE::BUILDNODE( data.mission_tech.race[1].build | 
		TECHTREE::MDEPEMD_JUMP_PLAT );

	MScript::SetAvailiableTech( data.mission_tech );

	MScript::EndStream( data.mhandle );
	data.mhandle = MScript::PlayAnimatedMessage( "MTMD25.wav", "Animate!!Mordella2", 
		MagpieLeft, MagpieTop, IDS_MT_SUB_MTMD25);

	AddAndDisplayObjective( IDS_MT_OBJECTIVE14, data.mhandle );

	data.WormholeToIode = MScript::GetPartByName( "Wormhole to Iode (Voraak)" );
	MScript::AlertMessage(data.WormholeToIode, 0);

}

bool MantisTrain_PlantationFinished::Update (void)
{
	return false;
}

//--------------------------------------------------------------------------//
//	 

CQSCRIPTPROGRAM(MantisTrain_JumpgateInProgress, MantisTrain_JumpgateInProgress_Save,0);

void MantisTrain_JumpgateInProgress::Initialize (U32 eventFlags, const MPartRef & part)
{

	//setup trigger to detect when Jump Gate is finished
	MScript::SetTriggerProgram(data.TrainTrigger1, "MantisTrain_JumpgateFinished");
	// detect a Jump Gate finished player 1.
	MScript::SetTriggerFilter(data.TrainTrigger1, M_JUMPPLAT, TRIGGER_MOBJCLASS, false);
	MScript::SetTriggerFilter(data.TrainTrigger1, 0, TRIGGER_FORCEREADY, true);
	MScript::SetTriggerFilter(data.TrainTrigger1, PLAYER_ID, TRIGGER_PLAYER, true);

	MScript::EndStream( data.mhandle );
	data.mhandle = MScript::PlayAnimatedMessage( "MTMD26.wav", "Animate!!Mordella2", 
		MagpieLeft, MagpieTop, IDS_MT_SUB_MTMD26);
}

bool MantisTrain_JumpgateInProgress::Update (void)
{
	return false;
}

//--------------------------------------------------------------------------//
//	 

CQSCRIPTPROGRAM(MantisTrain_JumpgateFinished, MantisTrain_JumpgateFinished_Save,0);

void MantisTrain_JumpgateFinished::Initialize (U32 eventFlags, const MPartRef & part)
{
	MScript::MarkObjectiveCompleted( IDS_MT_OBJECTIVE14 );

	MScript::EnableJumpgate( MScript::GetPartByName( "Wormhole to Iode (Voraak)" ), true );

	MScript::EnableTrigger(data.TrainTrigger1, false);

	MScript::EndStream( data.mhandle );
	data.mhandle = MScript::PlayAnimatedMessage( "MTMD27.wav", "Animate!!Mordella2", 
		MagpieLeft, MagpieTop, IDS_MT_SUB_MTMD27);

	AddAndDisplayObjective( IDS_MT_OBJECTIVE15, data.mhandle );
}

bool MantisTrain_JumpgateFinished::Update (void)
{

	if (data.mission_over)
	{
		 return false;
	}

	if(!MScript::IsStreamPlaying(data.mhandle))
	{
		MScript::RunProgramByName("MantisTrain_JumpToIode", MPartRef());

		return false;
	}
	return true;
}

//--------------------------------------------------------------------------//
//	 

CQSCRIPTPROGRAM(MantisTrain_JumpToIode, MantisTrain_JumpToIode_Save,0);

void MantisTrain_JumpToIode::Initialize (U32 eventFlags, const MPartRef & part)
{
	Spawn = MScript::GetPartByName("HQ");
}

bool MantisTrain_JumpToIode::Update (void)
{

	if (data.mission_over)
	{
		 return false;
	}

	if(PlayerInSystem(PLAYER_ID, IODE) && !data.PlayerInIode)
	{
		Attacker4 = MScript::CreatePart("GBOAT!!T_Dreadnought", Spawn, TERRAN_ID, "Admiral M.V. Dula");
		MScript::EnablePartnameDisplay(Attacker4, true);
		MScript::SetStance (Attacker4, US_ATTACK);
		MScript::OrderMoveTo(Attacker4, MScript::GetPartByName("Iode 3"));

		Attacker5 = MScript::CreatePart("GBOAT!!T_Dreadnought", Spawn, TERRAN_ID, "Admiral R.C. Viray");
		MScript::EnablePartnameDisplay(Attacker5, true);
		MScript::SetStance (Attacker5, US_ATTACK);
		MScript::OrderMoveTo(Attacker5, MScript::GetPartByName("Iode 3"));
	
	
		for(x = 0; x < 4; x++)
		{
			Attacker2 = MScript::CreatePart("GBOAT!!T_Missile Cruiser", Spawn, TERRAN_ID);
			MScript::SetStance (Attacker2, US_ATTACK);
			MScript::OrderMoveTo(Attacker2, MScript::GetPartByName("Iode 3"));
		}
	
		for(x = 0; x < 3; x++)
		{
			Attacker3 = MScript::CreatePart("GBOAT!!T_Midas Battleship", Spawn, TERRAN_ID);
			MScript::SetStance (Attacker3, US_ATTACK);
			MScript::OrderMoveTo(Attacker3, MScript::GetPartByName("Iode 3"));
		}
		
		for(x = 0; x < 3; x++)
		{
			Attacker = MScript::CreatePart("GBOAT!!T_Corvette", Spawn, TERRAN_ID);
			MScript::SetStance (Attacker, US_ATTACK);
			MScript::OrderMoveTo(Attacker, MScript::GetPartByName("Iode 3"));
		}
		MScript::EndStream( data.mhandle );
		data.mhandle = MScript::PlayAnimatedMessage( "MTMD28.wav", "Animate!!Mordella2", 
			MagpieLeft, MagpieTop, IDS_MT_SUB_MTMD28);

		MScript::MarkObjectiveCompleted( IDS_MT_OBJECTIVE15 );
		AddAndDisplayObjective( IDS_MT_OBJECTIVE16, data.mhandle );
		AddAndDisplayObjective( IDS_MT_OBJECTIVE17, data.mhandle );

		data.mission_state = DestroyBase;

		MScript::RunProgramByName("MantisTrain_KillTheTerrans", MPartRef());

		return false;
	}

	return true;
}

//--------------------------------------------------------------------------//
//	 

CQSCRIPTPROGRAM(MantisTrain_NiadFinished, MantisTrain_NiadFinished_Save,0);

void MantisTrain_NiadFinished::Initialize (U32 eventFlags, const MPartRef & part)
{
	MScript::EnableTrigger(data.TrainTrigger1, false);
	MScript::EnableTrigger(data.TrainTrigger2, false);

	MScript::MarkObjectiveCompleted( IDS_MT_OBJECTIVE21 );

	MScript::EndStream( data.mhandle );
	data.mhandle = MScript::PlayAnimatedMessage( "MTMD30A.wav", "Animate!!Mordella2", 
		MagpieLeft, MagpieTop, IDS_MT_SUB_MTMD30A);

}

bool MantisTrain_NiadFinished::Update (void)
{

	if (data.mission_over)
	{
		 return false;
	}

	if(!MScript::IsStreamPlaying(data.mhandle))
	{
		MScript::EndStream( data.mhandle );
		data.mhandle = MScript::PlayAnimatedMessage( "MTMD34A.wav", "Animate!!Mordella2", 
			MagpieLeft, MagpieTop, IDS_MT_SUB_MTMD34A);

		if(CountObjects(M_BIOFORGE, PLAYER_ID) == 0)
		{
			//setup trigger to detect when HQ is finished
			MScript::EnableTrigger(data.TrainTrigger1, true);
			MScript::SetTriggerProgram(data.TrainTrigger1, "MantisTrain_BioforgeInProgress");
			// detect a HQ finished player 1.
			MScript::SetTriggerFilter(data.TrainTrigger1, M_BIOFORGE, TRIGGER_MOBJCLASS, false);
			MScript::SetTriggerFilter(data.TrainTrigger1, 0, TRIGGER_NOFORCEREADY, true);
			MScript::SetTriggerFilter(data.TrainTrigger1, PLAYER_ID, TRIGGER_PLAYER, true);
			//setup trigger to detect when HQ is finished
			MScript::EnableTrigger(data.TrainTrigger2, true);
			MScript::SetTriggerProgram(data.TrainTrigger2, "MantisTrain_BioforgeInProgress");
			// detect a HQ finished player 1.
			MScript::SetTriggerFilter(data.TrainTrigger2, M_BIOFORGE, TRIGGER_MOBJCLASS, false);
			MScript::SetTriggerFilter(data.TrainTrigger2, 0, TRIGGER_NOFORCEREADY, true);
			MScript::SetTriggerFilter(data.TrainTrigger2, PLAYER_ID, TRIGGER_PLAYER, true);
		}
		else
		{
			MScript::RunProgramByName("MantisTrain_BioforgeFinished", MPartRef());
		}
		return false;
	}

	return true;
}

//--------------------------------------------------------------------------//
//	 

CQSCRIPTPROGRAM(MantisTrain_BioforgeInProgress, MantisTrain_BioforgeInProgress_Save,0);

void MantisTrain_BioforgeInProgress::Initialize (U32 eventFlags, const MPartRef & part)
{
	//setup trigger to detect when HQ is finished
	MScript::SetTriggerProgram(data.TrainTrigger1, "MantisTrain_BioforgeFinished");
	// detect a HQ finished player 1.
	MScript::SetTriggerFilter(data.TrainTrigger1, M_BIOFORGE, TRIGGER_MOBJCLASS, false);
	MScript::SetTriggerFilter(data.TrainTrigger1, 0, TRIGGER_FORCEREADY, true);
	MScript::SetTriggerFilter(data.TrainTrigger1, PLAYER_ID, TRIGGER_PLAYER, true);
	//setup trigger to detect when HQ is finished
	MScript::SetTriggerProgram(data.TrainTrigger2, "MantisTrain_BioforgeFinished");
	// detect a HQ finished player 1.
	MScript::SetTriggerFilter(data.TrainTrigger2, M_BIOFORGE, TRIGGER_MOBJCLASS, false);
	MScript::SetTriggerFilter(data.TrainTrigger2, 0, TRIGGER_FORCEREADY, true);
	MScript::SetTriggerFilter(data.TrainTrigger2, PLAYER_ID, TRIGGER_PLAYER, true);
 
}

bool MantisTrain_BioforgeInProgress::Update (void)
{
	return false;
}

//--------------------------------------------------------------------------//
//	 

CQSCRIPTPROGRAM(MantisTrain_BioforgeFinished, MantisTrain_BioforgeFinished_Save,0);

void MantisTrain_BioforgeFinished::Initialize (U32 eventFlags, const MPartRef & part)
{
	MScript::EnableTrigger(data.TrainTrigger1, false);
	MScript::EnableTrigger(data.TrainTrigger2, false);
}

bool MantisTrain_BioforgeFinished::Update (void)
{
	if(data.mission_over)
		return false;
	if(!MScript::IsStreamPlaying(data.mhandle))
	{
		data.mhandle = MScript::PlayAnimatedMessage( "MTMD34.wav", "Animate!!Mordella2", 
			MagpieLeft, MagpieTop, IDS_MT_SUB_MTMD34);
		return false;
	}
	return true;
}

//--------------------------------------------------------------------------//
//	 

CQSCRIPTPROGRAM(MantisTrain_KillTheTerrans, MantisTrain_KillTheTerrans_Save,0);

void MantisTrain_KillTheTerrans::Initialize (U32 eventFlags, const MPartRef & part)
{
	Spawn = MScript::GetPartByName("TerranSpawn");

	state = Ready;
}

bool MantisTrain_KillTheTerrans::Update (void)
{
	switch(state)
	{
		case Ready:
			if(data.MillCreated)
			{
				if(CountObjects(M_NIAD, PLAYER_ID) == 0)
				{
					if(!MScript::IsStreamPlaying(data.mhandle))
					{

						MScript::EndStream( data.mhandle );
						data.mhandle = MScript::PlayAnimatedMessage( "MTMD33.wav", "Animate!!Mordella2", 
							MagpieLeft, MagpieTop, IDS_MT_SUB_MTMD33);

						AddAndDisplayObjective( IDS_MT_OBJECTIVE21, data.mhandle, true );
		
						//setup trigger to detect when Niad is finished
						MScript::EnableTrigger(data.TrainTrigger1, true);
						MScript::SetTriggerProgram(data.TrainTrigger1, "MantisTrain_NiadFinished");

						// detect a Niad finished player 1.
						MScript::SetTriggerFilter(data.TrainTrigger1, M_NIAD, TRIGGER_MOBJCLASS, false);
						MScript::SetTriggerFilter(data.TrainTrigger1, 0, TRIGGER_NOFORCEREADY, true);
						MScript::SetTriggerFilter(data.TrainTrigger1, PLAYER_ID, TRIGGER_PLAYER, true);

						//setup trigger to detect when Niad is finished
						MScript::EnableTrigger(data.TrainTrigger2, true);
						MScript::SetTriggerProgram(data.TrainTrigger2, "MantisTrain_NiadFinished");

						// detect a Niad finished player 1.
						MScript::SetTriggerFilter(data.TrainTrigger2, M_NIAD, TRIGGER_MOBJCLASS, false);
						MScript::SetTriggerFilter(data.TrainTrigger2, 0, TRIGGER_NOFORCEREADY, true);
						MScript::SetTriggerFilter(data.TrainTrigger2, PLAYER_ID, TRIGGER_PLAYER, true);

					}
				}

				if(!CountObjects(M_NIAD, PLAYER_ID) == 0)
				{
					MScript::RunProgramByName("MantisTrain_NiadFinished", MPartRef());

				}

				MPartRef ship;
				MPartRef way = MScript::GetPartByName("Att_Waypoint");

				for(x = 0; x < 4; x++)
				{
					ship = MScript::CreatePart("GBOAT!!T_Missile Cruiser", Spawn, TERRAN_ID);
					MScript::SetStance (ship, US_ATTACK);
					MScript::OrderMoveTo(ship,way );
				}

				for(x = 0; x < 3; x++)
				{
					ship = MScript::CreatePart("GBOAT!!T_Midas Battleship", Spawn, TERRAN_ID);
					MScript::SetStance (ship, US_ATTACK);
					MScript::OrderMoveTo(ship, way);
				}
				
				for(x = 0; x < 6; x++)
				{
					ship = MScript::CreatePart("GBOAT!!T_Corvette", Spawn, TERRAN_ID);
					MScript::SetStance (ship, US_ATTACK);
					MScript::OrderMoveTo(ship, way);
				}

				data.mission_tech.race[1].build = TECHTREE::BUILDNODE( data.mission_tech.race[1].build |
				TECHTREE::MDEPEND_BIOFORGE | TECHTREE::MDEPEND_DISECTION );

				MScript::SetAvailiableTech( data.mission_tech );

				state = DetectFinal;
			}
			break;

		case DetectFinal:

			if (data.mission_state == DestroyBase && MScript::IsVisibleToPlayer(MScript::GetPartByName("Iode 3"), PLAYER_ID) && 
				!MScript::IsStreamPlaying( data.mhandle ) )
			{
				//clean up queue
				MScript::RemoveFromObjectiveList( IDS_MT_OBJECTIVE11 );
				MScript::RemoveFromObjectiveList( IDS_MT_OBJECTIVE12 );
				MScript::RemoveFromObjectiveList( IDS_MT_OBJECTIVE12a );
				MScript::RemoveFromObjectiveList( IDS_MT_OBJECTIVE13 );
				MScript::RemoveFromObjectiveList( IDS_MT_OBJECTIVE14 );
				MScript::RemoveFromObjectiveList( IDS_MT_OBJECTIVE15 );				
				MScript::RemoveFromObjectiveList( IDS_MT_OBJECTIVE16 );
				MScript::RemoveFromObjectiveList( IDS_MT_OBJECTIVE17 );

				MScript::MakeAreaVisible(TERRAN_ID, MScript::GetPartByName("Iode 3"), 7);
				MScript::EndStream( data.mhandle );
				data.mhandle = MScript::PlayAnimatedMessage( "MTMD35.wav", "Animate!!Mordella2", 
					MagpieLeft, MagpieTop, IDS_MT_SUB_MTMD35);

				AddAndDisplayObjective( IDS_MT_FINAL, data.mhandle );

				state = EliminateTerrans;
			}

			break;


		case EliminateTerrans:

			if(!MScript::PlayerHasUnits(TERRAN_ID))
			{
				data.mission_over = true;

				MScript::EnableMovieMode(true);

				MScript::EndStream( data.mhandle );
				data.mhandle = MScript::PlayAnimatedMessage( "MTMD36.wav", "Animate!!Mordella2", 
					MagpieLeft, MagpieTop, IDS_MT_SUB_MTMD36);

				MScript::MarkObjectiveCompleted( IDS_MT_FINAL );

				state = Success;
			}

			break;

		case Success:

			if(!MScript::IsStreamPlaying(data.mhandle))
			{
				UnderMissionControl();

				MScript::FlushTeletype();
				data.thandle = TeletypeMissionOver(IDS_MT_MISSION_SUCCESS);

				state = Done;
			}

			break;

		case Done:

			if(!MScript::IsTeletypePlaying(data.mhandle))
			{
				UnderPlayerControl();
				MScript::EndMissionVictory(0);
	
				return false;
			}

			break;
	}

	return true;
}

//-----------------------------------------------------------------------
//OBJECT CONSTRUCTED EVENT

CQSCRIPTPROGRAM(MantisTrain_ObjectConstructed, MantisTrain_ObjectConstructed_Save, CQPROGFLAG_OBJECTCONSTRUCTED);

void MantisTrain_ObjectConstructed::Initialize (U32 eventFlags, const MPartRef & part)
{

	if(part->systemID == IODE && part->mObjClass == M_PLANTATION && !data.MillCreated)
	{
		data.MillCreated = true;

		MScript::MarkObjectiveCompleted( IDS_MT_OBJECTIVE17 );
	}

	if(part->systemID == IODE && part->mObjClass == M_PLASMASPLITTER && !data.PlasmaCreated)
	{
		data.PlasmaCreated = true;

		MScript::MarkObjectiveCompleted( IDS_MT_OBJECTIVE16 );
	}
	
}

bool MantisTrain_ObjectConstructed::Update (void)
{
	return false;
}

//--------------------------------------------------------------------------//
//	 

CQSCRIPTPROGRAM(MantisTrain_CheckForMissionFail, MantisTrain_CheckForMissionFail_Save,0);

void MantisTrain_CheckForMissionFail::Initialize (U32 eventFlags, const MPartRef & part)
{
	state = Ready;
}

bool MantisTrain_CheckForMissionFail::Update (void)
{

	switch (state) 
	{

		case Ready:

			if ( CountObjects( M_COCOON, PLAYER_ID ) == 0 && /*CountObjects( M_COLLECTOR, PLAYER_ID ) == 0 &&*/ CountObjects( M_WEAVER, PLAYER_ID ) == 0) 
			{
//				if ( CountObjects( M_COLLECTOR, PLAYER_ID ) == 0 && CountObjects( M_WEAVER, PLAYER_ID ) == 0)
//				{
					data.mission_over = true;

					MScript::EnableMovieMode(true);
        			UnderMissionControl();

					MScript::EndStream( data.mhandle );
					data.mhandle = MScript::PlayAnimatedMessage( "MTMD37.wav", "Animate!!Mordella2", 
						MagpieLeft, MagpieTop, IDS_MT_SUB_MTMD37);

					state = Done;
//				}

			}

			break;

		case Done:

			if (!MScript::IsStreamPlaying( data.mhandle ))
			{

				MScript::FlushTeletype();
				data.thandle = TeletypeMissionOver(IDS_MT_MISSION_FAILURE);

				UnderPlayerControl();
				MScript::EndMissionDefeat();
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

    MScript::RunProgramByName( "MT_DisplayObjectiveHandler", MPartRef ());
}

CQSCRIPTPROGRAM( MT_DisplayObjectiveHandler, MT_DisplayObjectiveHandlerData, 0 );

void MT_DisplayObjectiveHandler::Initialize
(
	U32 eventFlags, 
	const MPartRef &part 
)
{
    stringID = data.displayObjectiveHandlerParams_stringID;
    dependantHandle = data.displayObjectiveHandlerParams_dependantHandle;
}

bool MT_DisplayObjectiveHandler::Update
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

