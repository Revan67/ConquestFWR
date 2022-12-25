//--------------------------------------------------------------------------//
//                                                                          //
//                                Script09T.cpp                             //
//				/Conquest/App/Src/Scripts/Script02T/Script09T.cpp			//
//								MISSION PROGRAMS							//
//                                                                          //
//               COPYRIGHT (C) 2000 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
	Created:	5/26/00		JeffP
	Modified:	5/26/00		JeffP

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

//This is a little dangerous.  You cannot store anything in here over time.  It can only be used durning the same update it is set!!!
struct PatrolInit
{
	MPartRef patrolPoints[10];
	U32 numPoints;
	char * archetype;
}patrolInit;

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//

void SetTechLevel(U32 stage)
{

	TECHNODE mission_tech;
	mission_tech.InitLevel(TECHTREE::FULL_TREE);

	if (stage == 1)
	{
		// 0 is the Terran players race...
		mission_tech.race[0].build = mission_tech.race[1].build =
			(TECHTREE::BUILDNODE) 
			(TECHTREE::TDEPEND_HEADQUARTERS | TECHTREE::TDEPEND_REFINERY | TECHTREE::TDEPEND_LIGHT_IND | 
			 TECHTREE::TDEPEND_JUMP_INHIBITOR | TECHTREE::TDEPEND_LASER_TURRET | TECHTREE::TDEPEND_TENDER | 
			 TECHTREE::TDEPEND_SENSORTOWER | TECHTREE::TDEPEND_OUTPOST | TECHTREE::TDEPEND_BALLISTICS |
			 TECHTREE::TDEPEND_HEAVY_IND | TECHTREE::TDEPEND_ADVHULL | TECHTREE::TDEPEND_ACADEMY |
			 TECHTREE::TDEPEND_HANGER | TECHTREE::TDEPEND_SPACE_STATION | TECHTREE::TDEPEND_REPAIR | 
			 TECHTREE::TDEPEND_PROPLAB | TECHTREE::TDEPEND_AWSLAB | 
			 TECHTREE::RES_REFINERY_GAS1 | TECHTREE::RES_REFINERY_METAL1 | TECHTREE::RES_REFINERY_METAL2);

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
			 TECHTREE::T_RES_TROOPSHIP2 | TECHTREE::T_RES_XCHARGES | TECHTREE::T_RES_XPROBE | TECHTREE::M_SHIP_KHAMIR ); 
		
		mission_tech.race[0].common = mission_tech.race[1].common =
			(TECHTREE::COMMON)
			(TECHTREE::RES_SUPPLY1 | TECHTREE::RES_WEAPONS1 | TECHTREE::RES_SUPPLY2 | TECHTREE::RES_WEAPONS2 |
			 TECHTREE::RES_HULL1 | TECHTREE::RES_HULL2 | TECHTREE::RES_HULL3 | TECHTREE::RES_ENGINE1 | 
			 TECHTREE::RES_ENGINE2 | TECHTREE::RES_HULL4 | TECHTREE::RES_SUPPLY3); 
		mission_tech.race[0].common_extra = mission_tech.race[1].common_extra =
			(TECHTREE::COMMON_EXTRA)
			(TECHTREE::RES_TENDER1 | TECHTREE::RES_TANKER1 | TECHTREE::RES_TENDER2 | TECHTREE::RES_TANKER2 | 
			 TECHTREE::RES_SENSORS1 | TECHTREE::RES_FIGHTER1 | TECHTREE::RES_FIGHTER2 | TECHTREE::RES_FIGHTER3);
	}

	M_ SetAvailiableTech(mission_tech);

	TECHNODE playerTech = MScript::GetPlayerTech(PLAYER_ID);
	playerTech.race[2].tech = (TECHTREE::TECHUPGRADE) ( playerTech.race[2].tech | TECHTREE::S_RES_SYNTHESIS ); 

	MScript::SetPlayerTech( PLAYER_ID, playerTech );

	// mike's edits: give explosive ram for khamirs
	TECHNODE mantisTech = MScript::GetPlayerTech(MANTIS_ID);

	mantisTech.race[1].tech = (TECHTREE::TECHUPGRADE) ( TECHTREE::M_RES_EXPLODYRAM1 );

	MScript::SetPlayerTech( MANTIS_ID, mantisTech );

	// end

}

//--------------------------------------------------------------------------//
//  MISSION PROGRAM 														//
//--------------------------------------------------------------------------//

CQSCRIPTDATA(MissionData, data);

//--------------------------------------------------------------------------
//	MISSION BRIEFING
//--------------------------------------------------------------------------

CQSCRIPTPROGRAM(TM9_Briefing, TM9_Briefing_Data,CQPROGFLAG_STARTBRIEFING);

void TM9_Briefing::Initialize (U32 eventFlags, const MPartRef & part)
{
	data.failureID = 0;
	data.mission_over = data.briefing_over = data.ai_enabled = false;
	data.mhandle = 0;
	data.shandle = 0;
	data.bKerTakDied = data.bElanDied = data.bInEpsilon = false;
	data.numInAbell = 0;

	// mike's edits: bools for kamikaze
	data.bInSerpce = data.bInGrog = false;
	//end


	data.mission_state = Briefing;
	state = Begin;

	M_ PlayMusic ( NULL );

	M_ EnableBriefingControls(true);

}

#define TIME_TO_TELETYPE	10
#define TEXT_PRINT_TIME		5000
#define TEXT_HOLD_TIME		10000
#define TEXT_HOLD_TIME_INF  0

bool TM9_Briefing::Update (void)
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

#ifdef _TELETYPE_SPEECH
			data.mhandle = TeletypeBriefing(IDS_TM9_TELETYPE_LOCATION, false);
			//data.mhandle = M_ PlayBriefingTeletype(IDS_TM9_TELETYPE_LOCATION, MOTextColor, 8000, TEXT_PRINT_TIME, false);
#else
			//STANDARDIZED TO MATCH MISSION 1
			TeletypeBriefing(IDS_TM9_TELETYPE_LOCATION, false);
			timer = 0;
			//M_ PlayBriefingTeletype(IDS_TM9_TELETYPE_LOCATION, MOTextColor, TEXT_HOLD_TIME_INF, 2000, false);
			//timer = 128;
#endif

			state = Davis;
			break;
		
		case Davis:

#ifdef _TELETYPE_SPEECH
			if (!M_ IsTeletypePlaying(data.mhandle))
#else
			if (timer > 0) { timer--; break; }
			else
#endif
			{
				//data.mhandle = DoBriefingSpeech(IDS_TM9_DAVIS_M09JD01, "M09JD01.wav", 8000, 2000, 0, CHAR_NOCHAR);
				data.mhandle = DoBriefingSpeech(IDS_TM9_SUB_PROLOG09, "PROLOG09.wav", 8000, 2000, 0, CHAR_NOCHAR);
				//state = Blackwell;
				state = PreHalsey;

				ShowBriefingAnimation(0, "TNRLogo", ANIMSPD_TNRLOGO, true, true);
				ShowBriefingAnimation(3, "TNRLogo", ANIMSPD_TNRLOGO, true, true);
				ShowBriefingAnimation(1, "Radiowave", ANIMSPD_RADIOWAVE, true, true);
				ShowBriefingAnimation(2, "Radiowave", ANIMSPD_RADIOWAVE, true, true);
			}
			break;

		/*
		case Blackwell:
			
			if (!IsSomethingPlaying(data.mhandle))
			{
				data.mhandle = DoBriefingSpeech(IDS_TM9_BLACKWELL_M09BL02, "M09BL02.wav", 8000, 2000, 0, CHAR_NOCHAR);
				state = Davis2;
			}
			break;

		case Davis2:
			
			if (!IsSomethingPlaying(data.mhandle))
			{
				data.mhandle = DoBriefingSpeech(IDS_TM9_DAVIS_M09JD03, "M09JD03.wav", 8000, 2000, 0, CHAR_NOCHAR);
				state = Singers;
			}
			break;

		case Singers:
			
			if (!IsSomethingPlaying(data.mhandle))
			{
				data.mhandle = DoBriefingSpeech(IDS_TM9_SINGERS_M09S03a, "M09S03a.wav", 8000, 2000, 0, CHAR_NOCHAR);
				state = Davis3;
			}
			break;

		case Davis3:
			
			if (!IsSomethingPlaying(data.mhandle))
			{
				data.mhandle = DoBriefingSpeech(IDS_TM9_DAVIS_M09JD03b, "M09JD03b.wav", 8000, 2000, 0, CHAR_NOCHAR);
				state = Halsey;
			}
			break;
		*/

		case PreHalsey:

			if (!M_ IsStreamPlaying(data.mhandle))	//audio
			{
				M_ FreeBriefingSlot(-1);

				M_ PlayAudio("high_level_data_uplink.wav");

				ShowBriefingAnimation(0, "Xmission", 2500/30, false, false); 
				ShowBriefingAnimation(1, "Xmission", 2500/30, false, false); 
				state = Halsey;
			}
			break;

		case Halsey:
			
			if (M_ IsCustomBriefingAnimationDone(0) && M_ IsCustomBriefingAnimationDone(1))	
			{
				data.mhandle = DoBriefingSpeechMagpie(IDS_TM9_SUB_M09HA04, "M09HA04.wav", 8000, 2000, 0, CHAR_HALSEY, true);
				state = Blackwell2;

				handle2 = ShowBriefingHead(1, CHAR_BLACKWELL);
			}
			break;

		case Blackwell2:
			
			if (!M_ IsStreamPlaying(data.mhandle))	//audio
			{
				data.mhandle = DoBriefingSpeech(IDS_TM9_SUB_M09BL05, "M09BL05.wav", 8000, 2000, 1, CHAR_BLACKWELL, true);
				state = PreElan;
			}
			break;

		case PreElan:

			if (!M_ IsStreamPlaying(data.mhandle))	//audio
			{
				M_ PlayAudio("high_level_data_uplink.wav");

				ShowBriefingAnimation(2, "Xmission", 2500/30, false, false); 
				state = Elan;
			}
			break;

		case Elan:
			
			if (M_ IsCustomBriefingAnimationDone(2))
			{
				data.mhandle = DoBriefingSpeech(IDS_TM9_SUB_M09EL06, "M09EL06.wav", 8000, 2000, 2, CHAR_ELAN, true);
				state = Halsey2;
			}
			break;

		case Halsey2:
			
			if (!M_ IsStreamPlaying(data.mhandle))	//audio
			{
				data.mhandle = DoBriefingSpeechMagpie(IDS_TM9_SUB_M09HA07, "M09HA07.wav", 8000, 2000, 0, CHAR_HALSEY, true);
				state = Elan2;
			}
			break;

		
		case Elan2:
			
			if (!M_ IsStreamPlaying(data.mhandle))	//audio
			{
				data.mhandle = DoBriefingSpeech(IDS_TM9_SUB_M09EL08, "M09EL08.wav", 8000, 2000, 2, CHAR_ELAN, true);
				state = Halsey3;
			}
			break;

		case Halsey3:
			
			if (!M_ IsStreamPlaying(data.mhandle))	//audio
			{
				data.mhandle = DoBriefingSpeechMagpie(IDS_TM9_SUB_M09HA09, "M09HA09.wav", 8000, 2000, 0, CHAR_HALSEY, true);
				state = Blackwell3;
			}
			break;

		case Blackwell3:
			
			if (!M_ IsStreamPlaying(data.mhandle))	//audio
			{
				data.mhandle = DoBriefingSpeech(IDS_TM9_SUB_M09BL10, "M09BL10.wav", 8000, 2000, 1, CHAR_BLACKWELL, true);
				state = Elan3;
			}
			break;

		case Elan3:
			
			if (!M_ IsStreamPlaying(data.mhandle))	//audio
			{
				data.mhandle = DoBriefingSpeech(IDS_TM9_SUB_M09EL11, "M09EL11.wav", 8000, 2000, 2, CHAR_ELAN, true);
				state = Halsey4;
			}
			break;

		case Halsey4:
			
			if (!M_ IsStreamPlaying(data.mhandle))	//audio
			{
				//data.mhandle = DoBriefingSpeechMagpie(IDS_TM9_HALSEY_M09HA12, "M09HA12.wav", 8000, 2000, 0, CHAR_HALSEY, true);
				state = Elan4;
			}
			break;

		case Elan4:
			
			if (!M_ IsStreamPlaying(data.mhandle))	//audio
			{
				//data.mhandle = DoBriefingSpeech(IDS_TM9_ELAN_M09EL13, "M09EL13.wav", 8000, 2000, 2, CHAR_ELAN, true);
				state = Halsey5;
			}
			break;

		case Halsey5:
			
			if (!M_ IsStreamPlaying(data.mhandle))	//audio
			{
				data.mhandle = DoBriefingSpeechMagpie(IDS_TM9_SUB_M09HA14, "M09HA14.wav", 8000, 2000, 0, CHAR_HALSEY, true);
				state = Objectives;

				M_ FreeBriefingSlot(2);
			}
			break;

		case Objectives:
		
			if (!M_ IsStreamPlaying(data.mhandle))	//audio
			{
				M_ FreeBriefingSlot(0);
				M_ FreeBriefingSlot(1);

				M_ FlushTeletype();

				data.mhandle = TeletypeBriefing(IDS_TM9_OBJECTIVES, true);
				//data.mhandle = M_ PlayBriefingTeletype(IDS_TM9_OBJECTIVES, MOTextColor, TEXT_HOLD_TIME_INF, TEXT_PRINT_TIME, false);
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

CQSCRIPTPROGRAM(TM9_MissionStart, TM9_MissionStart_Data,CQPROGFLAG_STARTMISSION);

void TM9_MissionStart::Initialize (U32 eventFlags, const MPartRef & part)
{
	data.mission_state = MissionStart;
	state = Begin;
	data.bTaosHelp = false;

	for (U32 c=0; c<NUM_SYSTEMS+1; c++)
		data.bSeenInSystem[c] = false;

	data.Halsey = M_ GetPartByName("Halsey");
	data.Blackwell = M_ GetPartByName("Blackwell");
	data.Elan = M_ GetPartByName("Elan");
	data.KerTak = M_ GetPartByName("Ker Tak");

	// mike's edits:
	// initialize khamirs for kamikaze

	data.Khamir1 = FindFirstObjectOfType(M_KHAMIR, MANTIS_ID, GROG);
	data.Khamir2 = FindNextObjectOfType(M_KHAMIR, MANTIS_ID, GROG, data.Khamir1);
	data.Khamir3 = FindNextObjectOfType(M_KHAMIR, MANTIS_ID, GROG, data.Khamir2);
	data.Khamir4 = FindNextObjectOfType(M_KHAMIR, MANTIS_ID, GROG, data.Khamir3);
	data.Khamir5 = FindNextObjectOfType(M_KHAMIR, MANTIS_ID, GROG, data.Khamir4);
	data.Khamir6 = FindNextObjectOfType(M_KHAMIR, MANTIS_ID, GROG, data.Khamir5);

	data.Khamir7 = FindFirstObjectOfType(M_KHAMIR, MANTIS_ID, SERPCE);
	data.Khamir8 = FindNextObjectOfType(M_KHAMIR, MANTIS_ID, SERPCE, data.Khamir7);
	data.Khamir9 = FindNextObjectOfType(M_KHAMIR, MANTIS_ID, SERPCE, data.Khamir8);
	data.Khamir10 = FindNextObjectOfType(M_KHAMIR, MANTIS_ID, SERPCE, data.Khamir9);
	data.Khamir11 = FindNextObjectOfType(M_KHAMIR, MANTIS_ID, SERPCE, data.Khamir10);
	data.Khamir12 = FindNextObjectOfType(M_KHAMIR, MANTIS_ID, SERPCE, data.Khamir11);

	//end edits

	M_ EnablePartnameDisplay(data.Halsey, true);
	M_ EnablePartnameDisplay(data.Blackwell, true);
	M_ EnablePartnameDisplay(data.Elan, true);
	M_ EnablePartnameDisplay(data.KerTak, true);

	M_ SetAllies(PLAYER_ID, REBEL_ID, true);
	M_ SetAllies(REBEL_ID, PLAYER_ID, true);

	M_ SetAllies(PLAYER_ID, SOLARIAN_ID, true);
	M_ SetAllies(SOLARIAN_ID, PLAYER_ID, true);

	M_ SetAllies(REBEL_ID, SOLARIAN_ID, true);
	M_ SetAllies(SOLARIAN_ID, REBEL_ID, true);

	M_ SetGas(PLAYER_ID, 160);
	M_ SetMetal(PLAYER_ID, 160);
	M_ SetCrew(PLAYER_ID, 40);

	M_ SetGas(MANTIS_ID, 80);
	M_ SetMetal(MANTIS_ID, 80);
	M_ SetCrew(MANTIS_ID, 20);

	SetTechLevel(1);

	M_ SetEnemyCharacter(MANTIS_ID, IDS_TM9_MANTIS);
	//M_ SetEnemyCharacter(REBEL_ID, IDS_TM9_REBELS);
	//M_ SetEnemyCharacter(SOLARIAN_ID, IDS_TM9_SOLARIANS);

	//AI SETUP
	M_ EnableEnemyAI(MANTIS_ID, true, "MANTIS_FORTRESS");
	MassEnableAI(MANTIS_ID, false);
	AIPersonality airules;
	airules.difficulty = EASY; 	
	airules.buildMask.bBuildPlatforms = false;
	airules.buildMask.bBuildLightGunboats = true;
	airules.buildMask.bBuildMediumGunboats = true;
	airules.buildMask.bBuildHeavyGunboats = false;
	airules.buildMask.bHarvest = false;
	airules.buildMask.bScout = false;
	airules.buildMask.bBuildAdmirals = false;
	airules.buildMask.bLaunchOffensives = false;  
	airules.buildMask.bVisibilityRules = true;  
	airules.nNumFabricators = 0;
	airules.nBuildPatience = 0;		
	airules.nNumMinelayers = 3;		
	airules.nNumTroopships = 0;		
	M_ SetEnemyAIRules(MANTIS_ID, airules);
	data.ai_enabled = true;

	M_ EnableSystem(VARN, false, false);
	M_ EnableSystem(EPSILON, true, false);  // mike's edits: disabled system viewed in sysmap
	M_ EnableSystem(ABELL, false, false);
	M_ EnableJumpgate(M_ GetPartByName("Gate19"), false);
	//M_ EnableJumpgate(M_ GetPartByName("Gate16"), false);

	M_ ClearHardFog(M_ GetPartByName("Gate16"), 2);

	for (U32 i=1; i<=10; i++)
		M_ ClearResources(i);

	M_ SetMissionID(8);
	M_ SetMissionName(IDS_TM9_MISSION_NAME);
	M_ SetMissionDescription(IDS_TM9_MISSION_DESC);

	M_ AddToObjectiveList(IDS_TM9_OBJECTIVE1);
	M_ AddToObjectiveList(IDS_TM9_OBJECTIVE2);

	M_ EnableRegenMode(true);

	M_ ChangeCamera(M_ GetPartByName("MissionCamStart"), 0, MOVIE_CAMERA_JUMP_TO);
	M_ ClearHardFog(M_ GetPartByName("Gate19"), 2);

	MPartRef loc;
	loc = M_ GetPartByName("locTalosTop1");
	MassMove(M_CORVETTE, PLAYER_ID, loc, VARN);
	loc = M_ GetPartByName("locTalosTop2");
	MassMove(M_INFILTRATOR, PLAYER_ID, loc, VARN);
	loc = M_ GetPartByName("locTalosTop2");
	MassMove(M_BATTLESHIP, PLAYER_ID, loc, VARN);
	loc = M_ GetPartByName("locTalosTop2");
	MassMove(M_SUPPLY, PLAYER_ID, loc, VARN);
	loc = M_ GetPartByName("locTalosTop1");
	MassMove(M_CARRIER, PLAYER_ID, loc, VARN);

	loc = M_ GetPartByName("locTalosTop3");
	M_ OrderMoveTo(data.Blackwell, loc);
	M_ OrderMoveTo(data.Halsey, loc);
	M_ OrderMoveTo(data.Elan, loc);
	M_ OrderMoveTo(data.KerTak, loc);


	patrolInit.numPoints = 2;
	patrolInit.archetype = "GBOAT!!M_Scout Carrier";
	patrolInit.patrolPoints[0] = MScript::GetPartByName("Talos_PP_0");
	patrolInit.patrolPoints[1] = MScript::GetPartByName("Talos_PP_1");
	M_ RunProgramByName("TM9_Patrol", MPartRef ());
	M_ RunProgramByName("TM9_Patrol", MPartRef ());
	M_ RunProgramByName("TM9_Patrol", MPartRef ());

	patrolInit.numPoints = 2;
	patrolInit.archetype = "GBOAT!!M_Scout Carrier";
	patrolInit.patrolPoints[0] = MScript::GetPartByName("Serpce_PP_0");
	patrolInit.patrolPoints[1] = MScript::GetPartByName("Serpce_PP_1");
	M_ RunProgramByName("TM9_Patrol", MPartRef ());
	M_ RunProgramByName("TM9_Patrol", MPartRef ());
	patrolInit.archetype = "GBOAT!!M_Frigate";
	M_ RunProgramByName("TM9_Patrol", MPartRef ());
	M_ RunProgramByName("TM9_Patrol", MPartRef ());

	patrolInit.numPoints = 4;
	patrolInit.archetype = "GBOAT!!M_Frigate";
	patrolInit.patrolPoints[0] = MScript::GetPartByName("Quasili_PP_0");
	patrolInit.patrolPoints[1] = MScript::GetPartByName("Quasili_PP_1");
	patrolInit.patrolPoints[2] = MScript::GetPartByName("Quasili_PP_2");
	patrolInit.patrolPoints[3] = MScript::GetPartByName("Quasili_PP_3");
	M_ RunProgramByName("TM9_Patrol", MPartRef ());
	M_ RunProgramByName("TM9_Patrol", MPartRef ());
	M_ RunProgramByName("TM9_Patrol", MPartRef ());
	M_ RunProgramByName("TM9_Patrol", MPartRef ());

	patrolInit.numPoints = 3;
	patrolInit.archetype = "GBOAT!!M_Frigate";
	patrolInit.patrolPoints[0] = MScript::GetPartByName("Grog_PP_0");
	patrolInit.patrolPoints[1] = MScript::GetPartByName("Grog_PP_1");
	patrolInit.patrolPoints[2] = MScript::GetPartByName("Grog_PP_2");
	M_ RunProgramByName("TM9_Patrol", MPartRef ());
	M_ RunProgramByName("TM9_Patrol", MPartRef ());
	M_ RunProgramByName("TM9_Patrol", MPartRef ());
	
	patrolInit.numPoints = 4;
	patrolInit.archetype = "GBOAT!!M_Frigate";
	patrolInit.patrolPoints[0] = MScript::GetPartByName("Gera_Kapp_PP_0");
	patrolInit.patrolPoints[1] = MScript::GetPartByName("Gera_Kapp_PP_1");
	patrolInit.patrolPoints[2] = MScript::GetPartByName("Gera_Kapp_PP_2");
	patrolInit.patrolPoints[3] = MScript::GetPartByName("Gera_Kapp_PP_3");
	patrolInit.patrolPoints[4] = MScript::GetPartByName("Gera_Kapp_PP_4");
	M_ RunProgramByName("TM9_Patrol", MPartRef ());
	M_ RunProgramByName("TM9_Patrol", MPartRef ());
	M_ RunProgramByName("TM9_Patrol", MPartRef ());
	patrolInit.archetype = "GBOAT!!M_Scout Carrier";
	M_ RunProgramByName("TM9_Patrol", MPartRef ());
	M_ RunProgramByName("TM9_Patrol", MPartRef ());
	M_ RunProgramByName("TM9_Patrol", MPartRef ());

	patrolInit.numPoints = 2;
	patrolInit.archetype = "GBOAT!!M_Scout Carrier";
	patrolInit.patrolPoints[0] = MScript::GetPartByName("Nano_PP_0");
	patrolInit.patrolPoints[1] = MScript::GetPartByName("Nano_PP_1");
	M_ RunProgramByName("TM9_Patrol", MPartRef ());
	M_ RunProgramByName("TM9_Patrol", MPartRef ());
	M_ RunProgramByName("TM9_Patrol", MPartRef ());
	
	patrolInit.numPoints = 3;
	patrolInit.archetype = "GBOAT!!M_Frigate";
	patrolInit.patrolPoints[0] = MScript::GetPartByName("Epsilon_PP_0");
	patrolInit.patrolPoints[1] = MScript::GetPartByName("Epsilon_PP_1");
	patrolInit.patrolPoints[2] = MScript::GetPartByName("Epsilon_PP_2");
	M_ RunProgramByName("TM9_Patrol", MPartRef ());
	M_ RunProgramByName("TM9_Patrol", MPartRef ());
	M_ RunProgramByName("TM9_Patrol", MPartRef ());
	patrolInit.archetype = "GBOAT!!M_Scarab";
	M_ RunProgramByName("TM9_Patrol", MPartRef ());
	M_ RunProgramByName("TM9_Patrol", MPartRef ());
	M_ RunProgramByName("TM9_Patrol", MPartRef ());	
}

bool TM9_MissionStart::Update (void)
{
	MPartRef part;

	if (data.mission_over) 
        return false;

	switch (state)
	{

		case Begin:
	
			if (CountObjects(M_SUPPLY, PLAYER_ID, VARN) == 0)
			{
				state = Blackwell1;
			}
			break;

		case Blackwell1:

			if (!IsSomethingPlaying(data.mhandle))
			{
				data.mhandle = DoSpeech(IDS_TM9_SUB_M09BL15, "M09BL15.wav", 8000, 2000, data.Blackwell, CHAR_BLACKWELL);
				state = Halsey1;
			}
			break;

		case Halsey1:

			if (!IsSomethingPlaying(data.mhandle))
			{
				data.mhandle = DoSpeech(IDS_TM9_SUB_M09HA16, "M09HA16.wav", 8000, 2000, data.Halsey, CHAR_HALSEY);
				state = Elan1;
			}
			break;

		case Elan1:

			if (!IsSomethingPlaying(data.mhandle))
			{
				data.mhandle = DoSpeech(IDS_TM9_SUB_M09EL17, "M09EL17.wav", 8000, 2000, data.Elan, CHAR_ELAN);
				state = Blackwell2;
			}
			break;

		case Blackwell2:

			if (!IsSomethingPlaying(data.mhandle))
			{
				data.mhandle = DoSpeech(IDS_TM9_SUB_M09BL18, "M09BL18.wav", 8000, 2000, data.Blackwell, CHAR_BLACKWELL);
				state = Halsey2;
			}
			break;

		case Halsey2:

			if (!IsSomethingPlaying(data.mhandle))
			{
				data.mhandle = DoSpeech(IDS_TM9_SUB_M09HA19, "M09HA19.wav", 8000, 2000, data.Halsey, CHAR_HALSEY);
				state = KerTak1;
				timer = 40;

				M_ ClearHardFog(M_ GetPartByName("Gate15"), 2);
				M_ ClearHardFog(M_ GetPartByName("Gate12"), 2);
			}
			break;

		case KerTak1:

			if (timer > 0) { timer--; break; }

			if (!IsSomethingPlaying(data.mhandle))
			{
				data.mhandle = DoSpeech(IDS_TM9_SUB_M09KR20, "M09KR20.wav", 8000, 2000, data.KerTak, CHAR_KERTAK);
				state = Done;
			}
			break;

		case Done:

			M_ RunProgramByName("TM9_EscortToEpsilon", MPartRef ());
		
			MScript::RunProgramByName("TM9_KamikazeElan", MPartRef ()); // mike's edits: run program
			
			return false;

	}

	return true;
}

//--------------------------------------------------------------------------//

CQSCRIPTPROGRAM(TM9_EscortToEpsilon, TM9_EscortToEpsilon_Data, 0);

void TM9_EscortToEpsilon::Initialize (U32 eventFlags, const MPartRef & part)
{
	data.mission_state = EscortToEpsilon;

	// Set up planet triggers
	MPartRef ppart;
	
	data.numTriggers = 0;
	ppart = FindFirstObjectOfType(M_PLANET, 0, 0);
	while (ppart.isValid())
	{
		if (ppart->systemID != VARN)
		{
			data.PlanetTrigger[data.numTriggers] = M_ CreatePart("MISSION!!TRIGGER", ppart, 0);
			M_ SetTriggerFilter(data.PlanetTrigger[data.numTriggers], PLAYER_ID, TRIGGER_PLAYER, false);
			M_ SetTriggerRange(data.PlanetTrigger[data.numTriggers], 2.5);
			M_ SetTriggerProgram(data.PlanetTrigger[data.numTriggers], "TM9_playernearbase");
			M_ EnableTrigger(data.PlanetTrigger[data.numTriggers], true);
			data.numTriggers++;
		}
		ppart = FindNextObjectOfType(M_PLANET, 0, 0, ppart);
	}
}

bool TM9_EscortToEpsilon::Update (void)
{
	if (data.mission_over) 
        return false;

	MPartRef obj;
	obj = FindFirstObjectOfType(0, PLAYER_ID, ABELL);
	if (obj.isValid())
	{
		if (SameObj(obj, data.Elan) || SameObj(obj, data.Blackwell) || 
			SameObj(obj, data.Halsey) || SameObj(obj, data.KerTak))
			data.numInAbell++;

		M_ RemovePart(obj);

		if (data.numInAbell >= 4)
		{
			M_ RunProgramByName("TM9_MissionSuccess", MPartRef());
			return false;
		}
	}

	// ONCE ELAN REACHES EPSILON, ALL GO AFTER IT
	if (!data.bInEpsilon && data.Elan->systemID == EPSILON)
	{
		M_ ClearPath(NANO, EPSILON, MANTIS_ID);
		M_ ClearPath(QUASILI, EPSILON, MANTIS_ID);
		MassAttack(0, MANTIS_ID, data.Elan, NANO);
		MassAttack(0, MANTIS_ID, data.Elan, QUASILI);
		MassAttack(0, MANTIS_ID, data.Elan, EPSILON);
		data.bInEpsilon=true;
	}

	return true;
}
//--------------------------------------------------------------------------//

CQSCRIPTPROGRAM(TM9_Patrol, TM9_Patrol_Data, 0);

void TM9_Patrol::Initialize (U32 eventFlags, const MPartRef & part)
{
	numPoints = patrolInit.numPoints;
	assert(numPoints>1);
	assert(numPoints <= 10);
	U32 index = 0;
	while(index < numPoints)
	{
		patrolPoints[index] = patrolInit.patrolPoints[index];
		++index;
	}
	currentIndex = 0;
	idleTimer = 0.0;
	ship = MScript::CreatePart(patrolInit.archetype,patrolPoints[0],MANTIS_ID);
	MScript::EnableAIForPart(ship,false);
}

bool TM9_Patrol::Update (void)
{
	if(!ship.isValid())
		return false;
	if(MScript::IsIdle(ship))
	{
		idleTimer += ELAPSED_TIME;
		if(idleTimer > 5.0)
		{
			MScript::OrderMoveTo(ship,patrolPoints[currentIndex]);
			currentIndex = (currentIndex+1)%numPoints;
			idleTimer = 0;
		}
	}
	return true;
}
//--------------------------------------------------------------------------//

CQSCRIPTPROGRAM(TM9_TaosHelp, TM9_TaosHelp_Data, 0);

#define TM9_NUM_TAOS 10

void TM9_TaosHelp::Initialize (U32 eventFlags, const MPartRef & part)
{
	if(!data.bTaosHelp)
	{
		data.bTaosHelp = true;
		targetPart = part;
		wormBlast = MScript::CreateWormBlast(part,1,PLAYER_ID,true);
		MScript::AlertMessage( part, NULL );
		timer = 2.0;
		state = TM9_NUM_TAOS;
	}
	else
	{
		state = -1;
	}
	MScript::EnableTrigger(part,false);
}

bool TM9_TaosHelp::Update (void)
{
	if(state == -1)
		return false;
	if(state == 0)
	{
		MScript::CloseWormBlast(wormBlast);
		return false;
	}
	timer -= ELAPSED_TIME;
	if(timer <= 0)
	{
		MPartRef taos = MScript::CreatePart("GBOAT!!S_Taos",targetPart,PLAYER_ID);
		MScript::FlashWormBlast(wormBlast);
		timer = 1.0;
		if(state == TM9_NUM_TAOS)
		{
			MScript::PlayCommAudio("STA3550.wav",taos);
		}
		--state;
	}
	return true;
}

//-----------------------------------------------------------------------
// mike's edits: kamikaze program
CQSCRIPTPROGRAM(TM9_KamikazeElan, TM9_KamikazeElan_Data, 0);

void TM9_KamikazeElan::Initialize (U32 eventFlags, const MPartRef & part)
{

}

bool TM9_KamikazeElan::Update (void)
{
	if (data.mission_over || data.bInEpsilon) 
        return false;

	// Once elan reaches grog, kamikaze him
	if (!data.bInGrog && data.Elan.isValid() && data.Elan->systemID == GROG && data.Elan.isValid() && MScript::IsVisibleToPlayer(data.Elan,MANTIS_ID))
	{

		if (data.Khamir1.isValid() && data.Elan.isValid())
		{
			MScript::OrderSpecialAttack(data.Khamir1, data.Elan);
		}

		if (data.Khamir2.isValid() && data.Elan.isValid())
		{
			MScript::OrderSpecialAttack(data.Khamir2, data.Elan);
		}

		if (data.Khamir3.isValid() && data.Elan.isValid())
		{
			MScript::OrderSpecialAttack(data.Khamir3, data.Elan);
		}

		if (data.Khamir4.isValid() && data.Elan.isValid())
		{
			MScript::OrderSpecialAttack(data.Khamir4, data.Elan);
		}

		if (data.Khamir5.isValid() && data.Elan.isValid())
		{
			MScript::OrderSpecialAttack(data.Khamir5, data.Elan);
		}

		if (data.Khamir6.isValid() && data.Elan.isValid())
		{
			MScript::OrderSpecialAttack(data.Khamir6, data.Elan);
		}

		data.bInGrog = true;
	}

	// Once elan reaches serpce, kamikaze him
	if (!data.bInSerpce && data.Elan.isValid() && data.Elan->systemID == SERPCE && data.Elan.isValid() && MScript::IsVisibleToPlayer(data.Elan,MANTIS_ID))
	{

		if (data.Khamir7.isValid() && data.Elan.isValid())
		{
			MScript::OrderSpecialAttack(data.Khamir7, data.Elan);
		}

		if (data.Khamir8.isValid() && data.Elan.isValid())
		{
			MScript::OrderSpecialAttack(data.Khamir8, data.Elan);
		}

		if (data.Khamir9.isValid() && data.Elan.isValid())
		{
			MScript::OrderSpecialAttack(data.Khamir9, data.Elan);
		}

		if (data.Khamir10.isValid() && data.Elan.isValid())
		{
			MScript::OrderSpecialAttack(data.Khamir10, data.Elan);
		}

		if (data.Khamir11.isValid() && data.Elan.isValid())
		{
			MScript::OrderSpecialAttack(data.Khamir11, data.Elan);
		}

		if (data.Khamir12.isValid() && data.Elan.isValid())
		{
			MScript::OrderSpecialAttack(data.Khamir12, data.Elan);
		}

		data.bInSerpce = true;
	}

	return true;
}

//end edits

//-----------------------------------------------------------------------

void StartAIForSystem(U32 SystemID)
{
	MassEnablePlatformAI(MANTIS_ID, true, SystemID);
}

CQSCRIPTPROGRAM(TM9_playernearbase, TM9_playernearbase_Data, 0);

void TM9_playernearbase::Initialize (U32 eventFlags, const MPartRef & part)
{
	MPartRef triggerobj;
	U32 systemID;

	triggerobj = M_ GetLastTriggerObject(part);

	if (!triggerobj.isValid()) return;

	systemID = triggerobj->systemID;

	if (!data.bSeenInSystem[systemID])
	{
		if (systemID == TALOS)
		{
			if (!data.bAIOnForSystem[TALOS]) { StartAIForSystem(TALOS); data.bAIOnForSystem[TALOS]=true; }
			if (!data.bAIOnForSystem[GROG]) { StartAIForSystem(GROG); data.bAIOnForSystem[GROG]=true; }
		}
		else if (systemID == SERPCE)
		{
			if (!data.bAIOnForSystem[QUASILI]) { StartAIForSystem(QUASILI); data.bAIOnForSystem[QUASILI]=true; }
		}
		else if (systemID == QUASILI)
		{
			if (!data.bAIOnForSystem[SERPCE]) { StartAIForSystem(SERPCE); data.bAIOnForSystem[SERPCE]=true; }
			if (!data.bAIOnForSystem[EPSILON]) { StartAIForSystem(EPSILON); data.bAIOnForSystem[EPSILON]=true; }
		}
		else if (systemID == KAAP)
		{
			if (!data.bAIOnForSystem[EPSILON]) { StartAIForSystem(EPSILON); data.bAIOnForSystem[EPSILON]=true; }
			if (!data.bAIOnForSystem[NANO]) { StartAIForSystem(NANO); data.bAIOnForSystem[NANO]=true; }
		}

		data.bSeenInSystem[systemID] = true;
	}
}

bool TM9_playernearbase::Update (void)
{
	return false;
}

//-----------------------------------------------------------------------
// OBJECT DESTRUCTION TRIGGER

CQSCRIPTPROGRAM(TM9_ObjectDestroyed, TM9_ObjectDestroyed_Data,CQPROGFLAG_OBJECTDESTROYED);

void TM9_ObjectDestroyed::Initialize (U32 eventFlags, const MPartRef & part)
{
	if (data.mission_over) 
        return;

	if (SameObj(part, data.Halsey))
	{
		data.mhandle = DoSpeech(IDS_TM9_SUB_M09BL21, "M09BL21.wav", 8000, 2000, data.Blackwell, CHAR_BLACKWELL);

		M_ MoveCamera(part, 0, MOVIE_CAMERA_JUMP_TO);

		data.failureID = IDS_TM9_FAIL_HALSEY_LOST;
		M_ RunProgramByName("TM9_MissionFailure", MPartRef ());
	}
	else if	(SameObj(part, data.KerTak))
	{
		data.mhandle = DoSpeech(IDS_TM9_SUB_M09BL22, "M09BL22.wav", 8000, 2000, data.Blackwell, CHAR_BLACKWELL);
		data.bKerTakDied = true;

		M_ MoveCamera(part, 0, MOVIE_CAMERA_JUMP_TO);

		data.failureID = IDS_TM9_FAIL_KERTAK_LOST;
		M_ RunProgramByName("TM9_MissionFailure", MPartRef ());
	}
	else if	(SameObj(part, data.Elan))
	{
		data.mhandle = DoSpeech(IDS_TM9_SUB_M09BL24, "M09BL24.wav", 8000, 2000, data.Blackwell, CHAR_BLACKWELL);
		data.bElanDied = true;

		M_ MoveCamera(part, 0, MOVIE_CAMERA_JUMP_TO);

		data.failureID = IDS_TM9_FAIL_ELAN_LOST;
		M_ RunProgramByName("TM9_MissionFailure", MPartRef ());
	}
	else if	(SameObj(part, data.Blackwell))
	{
		data.mhandle = DoSpeech(IDS_TM9_SUB_M09HA26, "M09HA26.wav", 8000, 2000, data.Halsey, CHAR_HALSEY);

		M_ MoveCamera(part, 0, MOVIE_CAMERA_JUMP_TO);

		data.failureID = IDS_TM9_FAIL_BLACKWELL_LOST;
		M_ RunProgramByName("TM9_MissionFailure", MPartRef ());
	}
	
}

bool TM9_ObjectDestroyed::Update (void)
{
	return false;
}

//---------------------------------------------------------------------------
// MISSION FAILURE PROGRAMS


//---------------------------------------------------------------------------

#define TELETYPE_TIME 3000

CQSCRIPTPROGRAM(TM9_MissionFailure, TM9_MissionFailure_Data, 0);

void TM9_MissionFailure::Initialize (U32 eventFlags, const MPartRef & part)
{
	UnderMissionControl();	
	M_ EnableMovieMode(true);

	data.mission_over = true;
	state = Begin;

	M_ MarkObjectiveFailed(IDS_TM9_OBJECTIVE2);
}

bool TM9_MissionFailure::Update (void)
{
	switch (state)
	{
		case Begin:

			if (!IsSomethingPlaying(data.mhandle))
			{
				if (data.bKerTakDied)
					data.mhandle = DoSpeech(IDS_TM9_SUB_M09HA23, "M09HA23.wav", 8000, 2000, data.Halsey, CHAR_HALSEY);
				else if (data.bElanDied)
					data.mhandle = DoSpeech(IDS_TM9_SUB_M09HA25, "M09HA25.wav", 8000, 2000, data.Halsey, CHAR_HALSEY);
				state = Teletype;
			}
			break;

		case Teletype:
			
			if (!IsSomethingPlaying(data.mhandle))
			{
				data.mhandle = TeletypeMissionOver(IDS_TM9_MISSION_FAILURE,data.failureID);
				//data.mhandle = M_ PlayTeletype(IDS_TM9_MISSION_FAILURE, SuperLeft, SuperTop, SuperRight, SuperBottom, MOTextColor, 6000, 1000, false);
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

CQSCRIPTPROGRAM(TM9_MissionSuccess, TM9_MissionSuccess_Data, 0);

void TM9_MissionSuccess::Initialize (U32 eventFlags, const MPartRef & part)
{
	UnderMissionControl();	
	M_ EnableMovieMode(true);

	data.mission_over = true;
	state = Begin;

	M_ MarkObjectiveCompleted(IDS_TM9_OBJECTIVE1);
	M_ MarkObjectiveCompleted(IDS_TM9_OBJECTIVE2);
}

bool TM9_MissionSuccess::Update (void)
{
	switch (state)
	{
		case Begin:

			data.mhandle = DoSpeech(IDS_TM9_SUB_M09HA27, "M09HA27.wav", 8000, 2000, data.Halsey, CHAR_HALSEY);
			state = Teletype;
			break;

		case Teletype:

			if (!IsSomethingPlaying(data.mhandle))
			{
				data.mhandle = TeletypeMissionOver(IDS_TM9_MISSION_SUCCESS);
				//data.mhandle = M_ PlayTeletype(IDS_TM9_MISSION_SUCCESS, SuperLeft, SuperTop, SuperRight, SuperBottom, MOTextColor, 6000, 1000, false);
				state = Done;
			}
			break;

		case Done:

			if (!M_ IsTeletypePlaying(data.mhandle))
			{
				UnderPlayerControl();
				M_ EndMissionVictory(8);
				return false;
			}
	}

	return true;
}

//--------------------------------------------------------------------------//
//--------------------------------End Script09T.cpp-------------------------//
//--------------------------------------------------------------------------//
