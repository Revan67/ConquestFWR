//--------------------------------------------------------------------------//
//                                                                          //
//                                Script07T.cpp                             //
//				/Conquest/App/Src/Scripts/Script02T/Script07T.cpp			//
//								MISSION PROGRAMS							//
//                                                                          //
//               COPYRIGHT (C) 2000 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
	Created:	5/22/00		JeffP
	Modified:	5/22/00		JeffP

*/
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
#define REBEL_ID			4

#define LUXOR				1
#define GHELEN				2
#define WOR					3
#define CORLAR				4
#define XIN					5
#define RENDAR				6
#define CENTAURUS			7
#define LEMO				8
#define UNKNOWN1			9
#define UNKNOWN2			10


#define HalseyLeft			150
#define HalseyRight			500
#define HalseyTop			10
#define HalseyBottom		100

#define BlackwellLeft		100
#define BlackwellRight		450
#define BlackwellTop		10
#define BlackwellBottom		100


#define MOLeft				150
#define MORight				500
#define MOTop				175
#define MOBottom			300
 
#define MOTTL				5000
#define TEXT_PRINT_TIME		5000
#define TEXT_HOLD_TIME		10000


//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//

void SetTechLevel(U32 stage)
{
	TECHNODE mission_tech;
	mission_tech.InitLevel(TECHTREE::FULL_TREE);

	if (stage == 1)
	{
		// 0 is the Terran players race...
		mission_tech.race[0].build = 
			(TECHTREE::BUILDNODE) 
			(TECHTREE::TDEPEND_HEADQUARTERS | TECHTREE::TDEPEND_REFINERY | TECHTREE::TDEPEND_LIGHT_IND | 
			 TECHTREE::TDEPEND_JUMP_INHIBITOR | TECHTREE::TDEPEND_LASER_TURRET | TECHTREE::TDEPEND_TENDER | 
			 TECHTREE::TDEPEND_SENSORTOWER | TECHTREE::TDEPEND_OUTPOST | TECHTREE::TDEPEND_BALLISTICS |
			 TECHTREE::TDEPEND_HEAVY_IND | TECHTREE::TDEPEND_ADVHULL | TECHTREE::TDEPEND_ACADEMY |
			 TECHTREE::TDEPEND_HANGER | TECHTREE::TDEPEND_SPACE_STATION | TECHTREE::TDEPEND_REPAIR | 
			 TECHTREE::TDEPEND_PROPLAB | TECHTREE::TDEPEND_AWSLAB |
			 TECHTREE::RES_REFINERY_GAS1 | TECHTREE::RES_REFINERY_METAL1 | TECHTREE::RES_REFINERY_METAL2);
        
	    mission_tech.race[1].build = 
            (TECHTREE::BUILDNODE) ( ( TECHTREE::ALL_BUILDNODE ^ 
            TECHTREE::MDEPEND_NIAD ) |
            TECH_TREE_RACE_BITS_ALL );

	    mission_tech.race[2].build = 
            (TECHTREE::BUILDNODE) ( ( TECHTREE::ALL_BUILDNODE ^ 
            TECHTREE::SDEPEND_PORTAL ) | 
            TECH_TREE_RACE_BITS_ALL );

		mission_tech.race[0].tech = mission_tech.race[1].tech =
			(TECHTREE::TECHUPGRADE) 
			(TECHTREE::T_SHIP__FABRICATOR | TECHTREE::T_SHIP__CORVETTE | TECHTREE::T_SHIP__HARVEST | 
			 TECHTREE::T_SHIP__MISSILECRUISER | TECHTREE::T_SHIP__TROOPSHIP | TECHTREE::T_RES_TROOPSHIP1 |
			 TECHTREE::T_SHIP__SUPPLY | TECHTREE::T_SHIP__INFILTRATOR | TECHTREE::T_SHIP__BATTLESHIP |
			 TECHTREE::T_SHIP__CARRIER | TECHTREE::T_RES_MISSLEPACK1 | TECHTREE::T_RES_MISSLEPACK2 | 
			 TECHTREE::T_RES_TROOPSHIP2 | TECHTREE::T_RES_XCHARGES | TECHTREE::T_RES_XPROBE);

		mission_tech.race[0].common = mission_tech.race[1].common =
			(TECHTREE::COMMON)
			(TECHTREE::RES_SUPPLY1 | TECHTREE::RES_WEAPONS1 | TECHTREE::RES_SUPPLY2 | TECHTREE::RES_WEAPONS2 |
			 TECHTREE::RES_HULL1 | TECHTREE::RES_HULL2 | TECHTREE::RES_HULL3 | TECHTREE::RES_ENGINE1 | 
			 TECHTREE::RES_ENGINE2); 

		mission_tech.race[0].common_extra = mission_tech.race[1].common_extra =
			(TECHTREE::COMMON_EXTRA)
			(TECHTREE::RES_TENDER1 | TECHTREE::RES_TANKER1 | TECHTREE::RES_TENDER2 | TECHTREE::RES_TANKER2 | 
			TECHTREE::RES_SENSORS1 | TECHTREE::RES_FIGHTER1 | TECHTREE::RES_FIGHTER2); 
	}

	M_ SetAvailiableTech(mission_tech);
}

//--------------------------------------------------------------------------//
//  MISSION PROGRAM 														//
//--------------------------------------------------------------------------//

CQSCRIPTDATA(MissionData, data);

//--------------------------------------------------------------------------
//	MISSION BRIEFING
//--------------------------------------------------------------------------

CQSCRIPTPROGRAM(TM7_Briefing, TM7_Briefing_Data,CQPROGFLAG_STARTBRIEFING);

void TM7_Briefing::Initialize (U32 eventFlags, const MPartRef & part)
{
	data.failureID = 0;
	data.mission_over = data.briefing_over = data.ai_enabled = false;
	data.bBuiltPropLab = data.bBuiltAWSLab = false;
	data.bBensonDied = data.bKerTakDied = data.bHQInLemo = false;
	data.mhandle = 0;
	data.shandle = 0;
	data.mission_state = Briefing;

	state = Begin;
	timer = 12;

	M_ PlayMusic ( NULL );

	M_ EnableBriefingControls(true);

}

#define TIME_TO_TELETYPE	10
#define TEXT_PRINT_TIME		5000
#define TEXT_HOLD_TIME		10000
#define TEXT_HOLD_TIME_INF  0

bool TM7_Briefing::Update (void)
{
	if (data.briefing_over)
		return false;

	switch(state)
	{
		case Begin:
	
			M_ FreeBriefingSlot(-1);
			state = TeleTypeLocation;
			break;
		
		case TeleTypeLocation:
		
			M_ FlushTeletype();

			//STANDARDIZED TO MATCH MISSION 1
			data.mhandle = TeletypeBriefing(IDS_TM7_TELETYPE_LOCATION, false);

            timer = 3;
			timer2 = 4;

			state = Monitor;
			break;
		
		case Monitor:

			if (!M_ IsTeletypePlaying(data.mhandle))
			{
				if (timer2 > 0) 
				{ 
					timer2--; 
					break; 
				}

				timer2 = 4;
				data.mhandle = M_ PlayBriefingTeletype(IDS_TM7_MONITORING, RedColor, 1000, 1, false);
				if (timer > 0)
					timer--;
				else
					state = Ghelen;
			}
			break;

		case Ghelen:
			
			if (!M_ IsTeletypePlaying(data.mhandle))
			{
				//data.mhandle = DoBriefingSpeech(IDS_TM7_GHELEN_M07G101, "M07G101.wav", 8000, 2000, 0, CHAR_GHELEN);
				data.mhandle = DoBriefingSpeech(0, "PROLOG07.wav", 8000, 2000, 0, CHAR_NOCHAR);
				MScript::BriefingSubtitle(data.mhandle,IDS_TM7_SUB_PROLOG07);

				//state = Ghelen2;
				state = PreBriefing;
				//state = Steele2;

				ShowBriefingAnimation( 0, "TechLoop1", 100, true, true );
				ShowBriefingAnimation( 1, "TechLoop2", 100, true, true );
				ShowBriefingAnimation( 2, "TechLoop3", 100, true, true );
				ShowBriefingAnimation( 3, "TechLoop1", 100, true, true );
			}
			break;

		/*
		case Ghelen2:
			
			if (!IsSomethingPlaying(data.mhandle))
			{
				data.mhandle = DoBriefingSpeech(IDS_TM7_GHELEN2_M07G202, "M07G202.wav", 8000, 2000, 0, CHAR_GHELEN);
				state = Steele;
			}
			break;

		case Steele:
			
			if (!IsSomethingPlaying(data.mhandle))
			{
				data.mhandle = DoBriefingSpeech(IDS_TM7_STEELE_M07ST03, "M07ST03.wav", 8000, 2000, 2, CHAR_STEELE);
				state = Hawkes;
			}
			break;

		case Hawkes:
			
			if (!IsSomethingPlaying(data.mhandle))
			{
				data.mhandle = DoBriefingSpeech(IDS_TM7_HAWKES_M07HW04, "M07HW04.wav", 8000, 2000, 1, CHAR_HAWKES);
				state = Ghelen3;
			}
			break;

		case Ghelen3:
			
			if (!IsSomethingPlaying(data.mhandle))
			{
				data.mhandle = DoBriefingSpeech(IDS_TM7_GHELEN_M07G105, "M07G105.wav", 8000, 2000, 0, CHAR_GHELEN);
				state = Hawkes2;
			}
			break;

		case Hawkes2:
			
			if (!IsSomethingPlaying(data.mhandle))
			{
				data.mhandle = DoBriefingSpeech(IDS_TM7_HAWKES_M07HW06, "M07HW06.wav", 8000, 2000, 1, CHAR_HAWKES);
				state = Ghelen4;
			}
			break;

		case Ghelen4:
			
			if (!IsSomethingPlaying(data.mhandle))
			{
				data.mhandle = DoBriefingSpeech(IDS_TM7_GHELEN2_M07G207, "M07G207.wav", 8000, 2000, 0, CHAR_GHELEN);
				state = Steele2;
			}
			break;
		*/

		case PreBriefing:

			if (!M_ IsStreamPlaying(data.mhandle))	//audio
			{
				M_ FreeBriefingSlot(-1);

				M_ PlayAudio("high_level_data_uplink.wav");
				ShowBriefingAnimation(0, "Xmission", 2500/30, false, false); 
				ShowBriefingAnimation(1, "Xmission", 2500/30, false, false); 
				ShowBriefingAnimation(2, "Xmission", 2500/30, false, false); 
				state = Steele2;
			}
			break;

		case Steele2:
			
			if (M_ IsCustomBriefingAnimationDone(0) && M_ IsCustomBriefingAnimationDone(1) && 
				M_ IsCustomBriefingAnimationDone(2))	
			{
				handle1 = ShowBriefingHead(0, CHAR_HALSEY);
				handle2 = ShowBriefingHead(1, CHAR_KERTAK);

				data.mhandle = DoBriefingSpeech(IDS_TM7_SUB_M07ST08, "M07ST08.wav", 8000, 2000, 2, CHAR_STEELE, true);
				state = KerTak;
			}
			break;

		case KerTak:
			
			if (!IsSomethingPlaying(data.mhandle))
			{
				data.mhandle = DoBriefingSpeech(IDS_TM7_SUB_M07KR09, "M07KR09.wav", 8000, 2000, 1, CHAR_KERTAK, true);
				state = Halsey;
			}
			break;

		case Halsey:
			
			if (!IsSomethingPlaying(data.mhandle))
			{
				data.mhandle = DoBriefingSpeechMagpie(IDS_TM7_SUB_M07HA10, "M07HA10.wav", 8000, 2000, 0, CHAR_HALSEY, true);
				state = KerTak2;
			}
			break;

		case KerTak2:
			
			if (!IsSomethingPlaying(data.mhandle))
			{
				data.mhandle = DoBriefingSpeech(IDS_TM7_SUB_M07KR11, "M07KR11.wav", 8000, 2000, 1, CHAR_KERTAK, true);
				state = Halsey2;
			}
			break;

		case Halsey2:
			
			if (!IsSomethingPlaying(data.mhandle))
			{
				handle4 = ShowBriefingHead(3, CHAR_BENSON);

				data.mhandle = DoBriefingSpeechMagpie(IDS_TM7_SUB_M07HA12, "M07HA12.wav", 8000, 2000, 0, CHAR_HALSEY, true);
				state = Objectives;
			}
			break;

		case Objectives:
		
			if (!IsSomethingPlaying(data.mhandle))
			{
				M_ FreeBriefingSlot(-1);

				data.mhandle = TeletypeBriefing(IDS_TM7_OBJECTIVES, true);
				//data.mhandle = M_ PlayBriefingTeletype(IDS_TM7_OBJECTIVES, MOTextColor, TEXT_HOLD_TIME_INF, TEXT_PRINT_TIME, false);
				state = Finish;
			}
			break;
			
		case Finish:

			break;
	}

	return true;
}

//--------------------------------------------------------------------------//
//	OTHER PROGRAMS
//--------------------------------------------------------------------------//

bool SameObj(const MPartRef & obj1, const MPartRef & obj2)
{
	if (!obj1.isValid() || !obj2.isValid())
		return false;

	if (obj1->dwMissionID == obj2->dwMissionID)
		return true;
	else
		return false;
}

//--------------------------------------------------------------------------//

void SetupMantisAI()
{
	M_ EnableEnemyAI(MANTIS_ID, true, "MANTIS_SWARM");
	
	AIPersonality airules;
	airules.difficulty = EASY; //CAKEWALK;
	
	airules.buildMask.bBuildPlatforms = true;
	airules.buildMask.bBuildLightGunboats = true;
	airules.buildMask.bBuildMediumGunboats = true;
	airules.buildMask.bBuildHeavyGunboats = false;
	airules.buildMask.bHarvest = true;
	airules.buildMask.bScout = true;
	airules.buildMask.bBuildAdmirals = false;
	airules.buildMask.bLaunchOffensives = true;  

	airules.nGunboatsPerSupplyShip = 50;
	airules.nNumMinelayers = 0;
	airules.uNumScouts = 3;
	airules.nNumFabricators = 2;
	airules.fHarvestersPerRefinery = 2;
	airules.nMaxHarvesters = 8;

	M_ SetEnemyAIRules(MANTIS_ID, airules);

	data.ai_enabled = true;
}

void HinderMantisAI()
{
	U32 gas, metal, crew;

	gas = M_ GetGas(MANTIS_ID);
	metal = M_ GetMetal(MANTIS_ID);
	crew = M_ GetCrew(MANTIS_ID);

	if (gas > 400) gas = 400;
	if (metal > 400) metal = 400;
	if (crew > 400) crew = 400;

	M_ SetGas(MANTIS_ID, gas);
	M_ SetMetal(MANTIS_ID, metal);
	M_ SetCrew(MANTIS_ID, crew);
}

void SetAITargetSys(U32 systemID, MPartRef center, bool bConstruct)
{
	if (!data.ai_enabled)
		M_ RunProgramByName("TM7_AIProgram", MPartRef ());

	M_ SetEnemyAITarget(MANTIS_ID, center, 16, systemID); 

	M_ MakeAreaVisible(MANTIS_ID, center, 255);

	if (bConstruct && (systemID == GHELEN || systemID == XIN || systemID == RENDAR))
	{
		MassEnableAI(MANTIS_ID, true, CORLAR);
		MassEnableAI(MANTIS_ID, true, CENTAURUS);
		M_ ClearPath(CORLAR, systemID, MANTIS_ID);
	}
	else if (bConstruct && systemID == CORLAR) 
	{
		MassEnableAI(MANTIS_ID, true, CORLAR);
		MassEnableAI(MANTIS_ID, true, CENTAURUS);
		MassEnableAI(MANTIS_ID, true, LEMO);
		M_ ClearPath(CENTAURUS, CORLAR, MANTIS_ID);
		M_ ClearPath(LEMO, CENTAURUS, MANTIS_ID);
	}
	else if (bConstruct && systemID == CENTAURUS)
	{
		MassEnableAI(MANTIS_ID, true, CORLAR);
		MassEnableAI(MANTIS_ID, true, CENTAURUS);
		MassEnableAI(MANTIS_ID, true, LEMO);
		M_ ClearPath(LEMO, CENTAURUS, MANTIS_ID);
	}
	else if (bConstruct && systemID == LEMO)
	{
		MassEnableAI(MANTIS_ID, true, LEMO);
		MassEnableAI(MANTIS_ID, true, CENTAURUS);
		M_ ClearPath(CENTAURUS, LEMO, MANTIS_ID);
	}
}

//--------------------------------------------------------------------------//

CQSCRIPTPROGRAM(TM7_AIProgram, TM7_AIProgram_Data, 0);

void TM7_AIProgram::Initialize (U32 eventFlags, const MPartRef & part)
{
	SetupMantisAI();
	timer = 500;
}

bool TM7_AIProgram::Update (void)
{
	if (data.mission_over)
		return false;

	if (timer == 0)
	{
		HinderMantisAI();
		timer = 500;
	}
	else
		timer--;

	return true;
}

//--------------------------------------------------------------------------//

CQSCRIPTPROGRAM(TM7_MissionStart, TM7_MissionStart_Data,CQPROGFLAG_STARTMISSION);

void TM7_MissionStart::Initialize (U32 eventFlags, const MPartRef & part)
{
	data.mission_state = MissionStart;

	data.Benson = M_ GetPartByName("Benson");
	data.KerTak = M_ GetPartByName("Ker Tak");

	M_ EnablePartnameDisplay(data.Benson, true);
	M_ EnablePartnameDisplay(data.KerTak, true);

	for (U32 x=1; x<=3; x++)
		data.bPlatInSys[x] = true;
	for (x=4; x<=8; x++)
		data.bPlatInSys[x] = false;

	data.SysCenter[2] = M_ GetPartByName("Ghelen Prime");
	data.SysCenter[3] = M_ GetPartByName("locWorCenter");
	data.SysCenter[4] = M_ GetPartByName("Corlar Prime");
	data.SysCenter[5] = M_ GetPartByName("locXinCenter");
	data.SysCenter[6] = M_ GetPartByName("locRendarCenter");
	data.SysCenter[7] = M_ GetPartByName("locCentaurusCenter");
	data.SysCenter[8] = M_ GetPartByName("locLemoCenter");

	M_ EnableSystem(LUXOR, false, false);
	M_ EnableSystem(GHELEN, true, true);
	M_ EnableSystem(WOR, true, true);
	M_ EnableSystem(XIN, true, true);
	M_ EnableSystem(RENDAR, true, true);
	M_ EnableSystem(CORLAR, true, false);
	M_ EnableSystem(CENTAURUS, true, false);
	M_ EnableSystem(UNKNOWN1, false, false);
	M_ EnableSystem(UNKNOWN2, false, false);

	M_ ClearHardFog(M_ GetPartByName("Ghelen Prime"), 255);
	M_ ClearHardFog(M_ GetPartByName("locWorCenter"), 255);

	M_ ClearHardFog(M_ GetPartByName("locXinCenter"), 255);
	M_ ClearHardFog(M_ GetPartByName("locRendarCenter"), 255);

	//gates to LUXOR, AEON and KRACUS
	M_ EnableJumpgate(M_ GetPartByName("Gate1"), false);
	M_ EnableJumpgate(M_ GetPartByName("Gate3"), false);
	M_ EnableJumpgate(M_ GetPartByName("Gate20"), false);
	M_ EnableJumpgate(M_ GetPartByName("Gate22"), false);

/*
	M_ ClearPath(GHELEN, RENDAR, PLAYER_ID);
	M_ ClearPath(GHELEN, XIN, PLAYER_ID);
	M_ ClearPath(GHELEN, WOR, REBEL_ID);
*/
	M_ SetAllies(PLAYER_ID, REBEL_ID, true);
	M_ SetAllies(REBEL_ID, PLAYER_ID, true);

	M_ SetGas(PLAYER_ID, 200);
	M_ SetMetal(PLAYER_ID, 260);
	M_ SetCrew(PLAYER_ID, 120);

	M_ SetGas(MANTIS_ID, 200);
	M_ SetMetal(MANTIS_ID, 200);
	M_ SetCrew(MANTIS_ID, 100);

	SetTechLevel(1);

	M_ SetEnemyCharacter(MANTIS_ID, IDS_TM7_MANTIS);
	M_ SetEnemyCharacter(REBEL_ID, IDS_TM7_REBELS);


	M_ EnableEnemyAI(MANTIS_ID, true, "MANTIS_SWARM");
	M_ EnableEnemyAI(MANTIS_ID, false, "MANTIS_SWARM");
	MassEnableAI(MANTIS_ID, false);

	M_ SetMissionID(6);
	M_ SetMissionName(IDS_TM7_MISSION_NAME);
	M_ SetMissionDescription(IDS_TM7_MISSION_DESC);

	M_ AddToObjectiveList(IDS_TM7_OBJECTIVE2);
	M_ AddToObjectiveList(IDS_TM7_OBJECTIVE3);

	M_ EnableRegenMode(true);

	M_ ChangeCamera(M_ GetPartByName("MissionCamStart"), 0, MOVIE_CAMERA_JUMP_TO);

	M_ RunProgramByName("TM7_MarchToLemo", MPartRef ());


}

bool TM7_MissionStart::Update (void)
{
	MPartRef part;

	if (data.mission_over)
		return false;

	// relocated from MissionStart::initialize to prevent crashing
	M_ ClearPath(GHELEN, RENDAR, PLAYER_ID);
	M_ ClearPath(GHELEN, XIN, PLAYER_ID);
	M_ ClearPath(GHELEN, WOR, REBEL_ID);

	switch (state)
	{

		case Begin:
	
			data.mhandle = DoSpeech(IDS_TM7_SUB_M07BN13, "M07BN13.wav", 8000, 2000, data.Benson, CHAR_BENSON);
			
			state = ShowLemo;
			break;

		case ShowLemo:

			if (!IsSomethingPlaying(data.mhandle))
			{
				M_ EnableSystem(LEMO, true, true);
				M_ AlertSector(LEMO);
				M_ AlertObjectInSysMap(M_ GetPartByName("Lemo Prime"), true);

				timer = 16;
				state = Benson;
			}
			break;

		case Benson:

			if (timer > 0) { timer--; break; }

			if (!IsSomethingPlaying(data.mhandle))
			{
				data.mhandle = DoSpeech(IDS_TM7_SUB_M07BN14, "M07BN14.wav", 8000, 2000, data.Benson, CHAR_BENSON);
				state = NewObj;
			}
			break;

		case NewObj:

			if (!IsSomethingPlaying(data.mhandle))
			{
				M_ AddToObjectiveList(IDS_TM7_OBJECTIVE4);
				if (CountObjects(M_PROPLAB, PLAYER_ID) == 0)
					TeletypeObjective(IDS_TM7_OBJECTIVE4);

				state = BuildPropulsion;
			}
			break;

		case BuildPropulsion:

			if (CountObjects(M_PROPLAB, PLAYER_ID) > 0)
			{
				data.mhandle = DoSpeech(IDS_TM7_SUB_M07BN15, "M07BN15.wav", 8000, 2000, data.Benson, CHAR_BENSON);
				state = BuildAWS;
			}
			break;

		case BuildAWS:

			if (data.bBuiltPropLab)
			{
				data.mhandle = DoSpeech(IDS_TM7_SUB_M07BN17, "M07BN17.wav", 8000, 2000, data.Benson, CHAR_BENSON);
				
				M_ MarkObjectiveCompleted(IDS_TM7_OBJECTIVE4);

				state = NewObj2;
			}
			break;

		case NewObj2:

			if (!IsSomethingPlaying(data.mhandle))
			{
				M_ AddToObjectiveList(IDS_TM7_OBJECTIVE1);

				if (CountObjects(M_AWSLAB, PLAYER_ID) == 0)	
					TeletypeObjective(IDS_TM7_OBJECTIVE1);

				state = FinishAWS;
			}
			break;

		case FinishAWS:

			if (data.bBuiltAWSLab)
			{
				data.mhandle = DoSpeech(IDS_TM7_SUB_M07BN19, "M07BN19.wav", 8000, 2000, data.Benson, CHAR_BENSON);
				state = Done;

				M_ MarkObjectiveCompleted(IDS_TM7_OBJECTIVE1);
			}
			break;

		case Done:

			return false;

	}

	return true;
}

//--------------------------------------------------------------------------//

CQSCRIPTPROGRAM(TM7_MarchToLemo, TM7_MarchToLemo_Data, 0);

void TM7_MarchToLemo::Initialize (U32 eventFlags, const MPartRef & part)
{

	timer = 1200;

}

bool TM7_MarchToLemo::Update (void)
{
	if (data.mission_over)
		return false;

	if (!M_ PlayerHasPlatforms(MANTIS_ID) && data.bHQInLemo)
	{
		MPartRef hq;

		hq = FindFirstObjectOfType(M_HQ, PLAYER_ID, LEMO);
		if (hq.isValid())
			M_ MoveCamera(hq, 0, MOVIE_CAMERA_JUMP_TO);

		M_ RunProgramByName("TM7_MissionSuccess", MPartRef ());
		return false;
	}

	// If player takes too long to scout around and start building, attack !
	if (!data.bAITargeting)
	{
		if (timer <= 0)
		{
			SetAITargetSys(GHELEN, data.SysCenter[GHELEN], true);
			data.bAITargeting = true;
		}
		else
			timer--;
	}

	return true;
}

//-----------------------------------------------------------------------
// OBJECT DESTRUCTION TRIGGER

CQSCRIPTPROGRAM(TM7_ObjectConstructed, TM7_ObjectConstructed_Data,CQPROGFLAG_OBJECTCONSTRUCTED);

void TM7_ObjectConstructed::Initialize (U32 eventFlags, const MPartRef & part)
{
	if (data.mission_over)
		return;

	switch (part->mObjClass)
	{
		case M_PROPLAB:

			data.bBuiltPropLab = true;
			break;

		case M_AWSLAB:

			data.bBuiltAWSLab = true;
			break;

		case M_HQ:

			if (part->playerID == PLAYER_ID && part->systemID == LEMO)
				data.bHQInLemo = true;
			break;
	}
	
	// if first time to build in new previously mantis owned system, attack !
	if (M_ IsPlatform(part) && part->playerID == PLAYER_ID)
	{
		//if (data.bPlatInSys[part->systemID] == false)
		//{
			SetAITargetSys(part->systemID, data.SysCenter[part->systemID], true);
			data.bPlatInSys[part->systemID] = true;
			data.bAITargeting = true;
		//}
	}

}

bool TM7_ObjectConstructed::Update (void)
{
	return false;
}

//-----------------------------------------------------------------------
// OBJECT DESTRUCTION TRIGGER

CQSCRIPTPROGRAM(TM7_ObjectDestroyed, TM7_ObjectDestroyed_Data,CQPROGFLAG_OBJECTDESTROYED);

void TM7_ObjectDestroyed::Initialize (U32 eventFlags, const MPartRef & part)
{
	if (data.mission_over)
		return;

	switch (part->mObjClass)
	{

		case M_CARRIER:
		case M_LANCER:
		case M_BATTLESHIP:

			if (SameObj(part, data.Benson))
			{
				data.mhandle = DoSpeech(IDS_TM7_SUB_M07HA20, "M07HA20.wav", 8000, 2000, MPartRef(), CHAR_HALSEY);
				data.bBensonDied = true;

				M_ MoveCamera(part, 0, MOVIE_CAMERA_JUMP_TO);

				data.failureID = IDS_TM7_FAIL_BENSON_LOST;
				M_ RunProgramByName("TM7_MissionFailure", MPartRef ());
			}
			break;
		
		case M_HIVECARRIER:

			if (SameObj(part, data.KerTak))
			{
				data.mhandle = DoSpeech(IDS_TM7_SUB_M07BN22, "M07BN22.wav", 8000, 2000, data.Benson, CHAR_BENSON);
				data.bKerTakDied = true;

				M_ MoveCamera(part, 0, MOVIE_CAMERA_JUMP_TO);

				data.failureID = IDS_TM7_FAIL_KERTAK_LOST;
				M_ RunProgramByName("TM7_MissionFailure", MPartRef ());
			}
			break;

	}

	if (M_ IsPlatform(part) && part->playerID == MANTIS_ID)
	{
		//if (data.bPlatInSys[part->systemID] == false)
		//{
			SetAITargetSys(part->systemID, data.SysCenter[part->systemID], true);
			data.bPlatInSys[part->systemID] = true;
			data.bAITargeting = true;
		//}
	}

}

bool TM7_ObjectDestroyed::Update (void)
{
	return false;
}

//---------------------------------------------------------------------------
// MISSION FAILURE PROGRAMS


//---------------------------------------------------------------------------

#define TELETYPE_TIME 3000

CQSCRIPTPROGRAM(TM7_MissionFailure, TM7_MissionFailure_Data, 0);

void TM7_MissionFailure::Initialize (U32 eventFlags, const MPartRef & part)
{
	UnderMissionControl();	
	M_ EnableMovieMode(true);

	data.mission_over = true;
	state = Begin;

	M_ MarkObjectiveFailed(IDS_TM7_OBJECTIVE3);
}

bool TM7_MissionFailure::Update (void)
{
	switch (state)
	{
		case Begin:

			if (!IsSomethingPlaying(data.mhandle))
			{
				if (data.bBensonDied)
					data.mhandle = DoSpeech(IDS_TM7_SUB_M07HA21, "M07HA21.wav", 8000, 2000, MPartRef(), CHAR_HALSEY);
				else if (data.bKerTakDied)
					data.mhandle = DoSpeech(IDS_TM7_SUB_M07HA23, "M07HA23.wav", 8000, 2000, MPartRef(), CHAR_HALSEY);
				state = Teletype;
			}
			break;

		case Teletype:
			
			if (!IsSomethingPlaying(data.mhandle))
			{
				data.mhandle = TeletypeMissionOver(IDS_TM7_MISSION_FAILURE,data.failureID);
				//data.mhandle = M_ PlayTeletype(IDS_TM7_MISSION_FAILURE, SuperLeft, SuperTop, SuperRight, SuperBottom, MOTextColor, 6000, 1000, false);
				state = Done;
			}
			break;

		case Done:
			if (!M_ IsTeletypePlaying(data.mhandle))
			{
				UnderPlayerControl();
				M_ EndMissionDefeat();
				return false;
			}
	}

	return true;
}

//---------------------------------------------------------------------------
// MISSION SUCCESS

CQSCRIPTPROGRAM(TM7_MissionSuccess, TM7_MissionSuccess_Data, 0);

void TM7_MissionSuccess::Initialize (U32 eventFlags, const MPartRef & part)
{
	UnderMissionControl();	
	M_ EnableMovieMode(true);

	data.mission_over = true;
	state = Begin;

	M_ MarkObjectiveCompleted(IDS_TM7_OBJECTIVE2);
	M_ MarkObjectiveCompleted(IDS_TM7_OBJECTIVE3);
}

bool TM7_MissionSuccess::Update (void)
{
	switch (state)
	{
		case Begin:

			data.mhandle = DoSpeech(IDS_TM7_SUB_M07HA24, "M07HA24.wav", 8000, 2000, MPartRef(), CHAR_HALSEY);
			state = Teletype;
			break;

		case Teletype:

			if (!IsSomethingPlaying(data.mhandle))
			{
				data.mhandle = TeletypeMissionOver(IDS_TM7_MISSION_SUCCESS);
				//data.mhandle = M_ PlayTeletype(IDS_TM7_MISSION_SUCCESS, SuperLeft, SuperTop, SuperRight, SuperBottom, MOTextColor, 6000, 1000, false);
				state = Done;
			}
			break;

		case Done:

			if (!M_ IsTeletypePlaying(data.mhandle))
			{
				UnderPlayerControl();
				M_ EndMissionVictory(6);
				return false;
			}
	}

	return true;
}

//--------------------------------------------------------------------------//
//--------------------------------End Script07T.cpp-------------------------//
//--------------------------------------------------------------------------//
