//--------------------------------------------------------------------------//
//                                                                          //
//                                Script02T.cpp                             //
//				/Conquest/App/Src/Scripts/Script02T/Script02T.cpp			//
//								MISSION PROGRAMS							//
//                                                                          //
//               COPYRIGHT (C) 2000 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
	Created:	4/17/00		JeffP
	Modified:	4/28/00		JeffP

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

#define TERRA				1
#define TAU_CETI			2
#define MANTIS_SPACE		3

#define HalseyLeft			150
#define HalseyRight			500
#define HalseyTop			10
#define HalseyBottom		100

#define BlackwellLeft		100
#define BlackwellRight		450
#define BlackwellTop		10
#define BlackwellBottom		100


// Mission Objectives text
#define MOLeft				150
#define MORight				500
#define MOTop				175
#define MOBottom			300

#define MOTTL				5000
#define TEXT_PRINT_TIME		5000
#define TEXT_HOLD_TIME		10000
#define TEXT_HOLD_TIME_INF  0

// Talking Heads "Time To Live" in seconds
#define DURATION_7000		53
#define DURATION_7001		65
#define DURATION_7010		23
#define DURATION_7011		27
#define DURATION_7012		6
#define DURATION_7013		19
#define DURATION_7014		4
#define DURATION_7015		22
#define DURATION_7016		20
#define DURATION_7017		13

#define DURATION_7018		20
#define DURATION_7019		4
#define DURATION_7020		12
#define DURATION_7021		6
#define DURATION_7022		5
#define DURATION_7023		11
#define DURATION_7024		8
#define DURATION_7025		6
#define DURATION_7026		3
#define DURATION_7027		17

#define DURATION_7028		6
#define DURATION_7029		8
#define DURATION_7030		5
#define DURATION_7031		12
#define DURATION_7032		6
#define DURATION_7033		5
#define DURATION_7033A		5
#define DURATION_7034		2
#define DURATION_7035		12
#define DURATION_7036		6
#define DURATION_7037		6
#define DURATION_7038		8

#define NUM_STARTING_MANTIS 0

#define PLAYER_MAX_GAS		60
#define PLAYER_MAX_METAL	100
#define PLAYER_MAX_CREW		75

#define PLAYER_STARTING_GAS		60 //30
#define PLAYER_STARTING_METAL	100 //80
#define PLAYER_STARTING_CREW	75 //30

CQBRIEFINGITEM slotAnim;

//--------------------------------------------------------------------------//
//  MISSION PROGRAM 														//
//--------------------------------------------------------------------------//

CQSCRIPTDATA(MissionData, data);

CQSCRIPTPROGRAM(TM2_GlobalProc, TM2_GlobalProc_Data,0);

void TM2_GlobalProc::Initialize (U32 eventFlags, const MPartRef & part)
{
	bGotToMaxCP = false;
}

bool TM2_GlobalProc::Update(void)
{
	if (data.mission_over)
		return false;

	// HAS OUR JUMPGATE PLATFORM BEEN ATTACKED ?
	if (data.Jumpgate_TC2ALK.isValid()) //data.mission_state > BuildGate)
	{
		if (!data.bJumpgateAttacked && !M_ IsDead(data.Jumpgate_TC2ALK) &&
			 data.Jumpgate_TC2ALK->hullPoints < data.Jumpgate_TC2ALK->hullPointsMax)  
			M_ RunProgramByName("TM2_jumpgateattacked", MPartRef ());
	}

	return true;
}

//--------------------------------------------------------------------------//
//	OTHER PROGRAMS

void SetupAI()
{
	MPartRef AlkOne = M_ GetPartByName("Alkaid Prime");
	
	M_ EnableEnemyAI(MANTIS_ID, true, "MANTIS_FRIGATE_RUSH");

	M_ SetVisibleToPlayer(AlkOne, MANTIS_ID);

	AIPersonality airules;
	airules.difficulty = EASY;
	
	airules.buildMask.bBuildPlatforms = false;
	airules.buildMask.bBuildLightGunboats = true;
	airules.buildMask.bBuildMediumGunboats = false;
	airules.buildMask.bBuildHeavyGunboats = false;
	airules.buildMask.bHarvest = false;
	airules.buildMask.bScout = true;
	airules.buildMask.bBuildAdmirals = false;
	airules.buildMask.bLaunchOffensives = true;
		
	M_ SetEnemyAIRules(MANTIS_ID, airules);

	M_ SetEnemyAITarget(MANTIS_ID, AlkOne, 2);

	MPartRef Scout1, Scout2;
	Scout1 = FindFirstObjectOfType(M_SCOUTCARRIER, MANTIS_ID, MANTIS_SPACE);
	if (Scout1.isValid())
		Scout2 = FindNextObjectOfType(M_SCOUTCARRIER, MANTIS_ID, MANTIS_SPACE, Scout1);
	
	if (Scout1.isValid())
		M_ EnableAIForPart(Scout1, false);
	if (Scout2.isValid())
		M_ EnableAIForPart(Scout2, false);
}

bool SameObj(const MPartRef & obj1, const MPartRef & obj2)
{
	if (obj1->dwMissionID == obj2->dwMissionID)
		return true;
	else
		return false;
}

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
			 TECHTREE::TDEPEND_LASER_TURRET | TECHTREE::TDEPEND_SENSORTOWER | TECHTREE::TDEPEND_BALLISTICS |
			 TECHTREE::RES_REFINERY_GAS1 | TECHTREE::RES_REFINERY_METAL1 | TECHTREE::TDEPEND_OUTPOST);
		mission_tech.race[0].tech = mission_tech.race[1].tech =
			(TECHTREE::TECHUPGRADE) 
			(TECHTREE::T_SHIP__FABRICATOR | TECHTREE::T_SHIP__CORVETTE | TECHTREE::T_SHIP__HARVEST | 
			 TECHTREE::T_SHIP__MISSILECRUISER | TECHTREE::T_SHIP__SUPPLY);
		mission_tech.race[0].common = mission_tech.race[1].common =
			(TECHTREE::COMMON)
			(TECHTREE::RES_SUPPLY1 | TECHTREE::RES_SUPPLY2 | TECHTREE::RES_WEAPONS1 | TECHTREE::RES_WEAPONS2); 
		mission_tech.race[0].common_extra = mission_tech.race[1].common_extra =
			(TECHTREE::COMMON_EXTRA)
			(TECHTREE::RES_TENDER1 | TECHTREE::RES_TANKER1 | TECHTREE::RES_SENSORS1); 

	}
	else if (stage == 2)
	{
		// 0 is the Terran players race...
		mission_tech.race[0].build = 
			(TECHTREE::BUILDNODE) 
			(TECHTREE::TDEPEND_HEADQUARTERS | TECHTREE::TDEPEND_REFINERY | TECHTREE::TDEPEND_LIGHT_IND | 
			 TECHTREE::TDEPEND_LASER_TURRET | TECHTREE::TDEPEND_TENDER | 
			 TECHTREE::TDEPEND_SENSORTOWER | TECHTREE::TDEPEND_BALLISTICS |
			 TECHTREE::RES_REFINERY_GAS1 | TECHTREE::RES_REFINERY_METAL1 | TECHTREE::TDEPEND_OUTPOST);
		mission_tech.race[0].tech = 
			(TECHTREE::TECHUPGRADE) 
			(TECHTREE::T_SHIP__FABRICATOR | TECHTREE::T_SHIP__CORVETTE | TECHTREE::T_SHIP__HARVEST | 
			 TECHTREE::T_SHIP__MISSILECRUISER | TECHTREE::T_SHIP__SUPPLY);
		mission_tech.race[0].common =
			(TECHTREE::COMMON)
			(TECHTREE::RES_SUPPLY1 | TECHTREE::RES_WEAPONS1 | TECHTREE::RES_WEAPONS2); 
		mission_tech.race[0].common_extra =
			(TECHTREE::COMMON_EXTRA)
			(TECHTREE::RES_TENDER1 | TECHTREE::RES_TANKER1 | TECHTREE::RES_SENSORS1); 
	}
	else if (stage == 3)
	{
		// 0 is the Terran players race...
		mission_tech.race[0].build = 
			(TECHTREE::BUILDNODE) 
			(TECHTREE::TDEPEND_HEADQUARTERS | TECHTREE::TDEPEND_REFINERY | TECHTREE::TDEPEND_LIGHT_IND | 
			 TECHTREE::TDEPEND_JUMP_INHIBITOR | TECHTREE::TDEPEND_LASER_TURRET | TECHTREE::TDEPEND_TENDER | 
			 TECHTREE::TDEPEND_SENSORTOWER | TECHTREE::TDEPEND_BALLISTICS |
			 TECHTREE::RES_REFINERY_GAS1 | TECHTREE::RES_REFINERY_METAL1 | TECHTREE::TDEPEND_OUTPOST);
		mission_tech.race[0].tech = 
			(TECHTREE::TECHUPGRADE) 
			(TECHTREE::T_SHIP__FABRICATOR | TECHTREE::T_SHIP__CORVETTE | TECHTREE::T_SHIP__HARVEST | 
			 TECHTREE::T_SHIP__MISSILECRUISER | TECHTREE::T_SHIP__SUPPLY);
		mission_tech.race[0].common =
			(TECHTREE::COMMON)
			(TECHTREE::RES_SUPPLY1 | TECHTREE::RES_WEAPONS1 | TECHTREE::RES_WEAPONS2); 
		mission_tech.race[0].common_extra =
			(TECHTREE::COMMON_EXTRA)
			(TECHTREE::RES_TENDER1 | TECHTREE::RES_TANKER1 | TECHTREE::RES_SENSORS1); 
	}
	else if (stage == 4)
	{
		// 0 is the Terran players race...
		mission_tech.race[0].build = 
			(TECHTREE::BUILDNODE) 
			(TECHTREE::TDEPEND_HEADQUARTERS | TECHTREE::TDEPEND_REFINERY | TECHTREE::TDEPEND_LIGHT_IND | 
			 TECHTREE::TDEPEND_JUMP_INHIBITOR | TECHTREE::TDEPEND_LASER_TURRET | TECHTREE::TDEPEND_TENDER | 
			 TECHTREE::TDEPEND_SENSORTOWER | TECHTREE::TDEPEND_OUTPOST | TECHTREE::TDEPEND_BALLISTICS |
			 TECHTREE::RES_REFINERY_GAS1 | TECHTREE::RES_REFINERY_METAL1);
		mission_tech.race[0].tech = 
			(TECHTREE::TECHUPGRADE) 
			(TECHTREE::T_SHIP__FABRICATOR | TECHTREE::T_SHIP__CORVETTE | TECHTREE::T_SHIP__HARVEST | 
			 TECHTREE::T_SHIP__MISSILECRUISER | TECHTREE::T_SHIP__TROOPSHIP | TECHTREE::T_RES_TROOPSHIP1 |
			 TECHTREE::T_SHIP__SUPPLY);
		mission_tech.race[0].common =
			(TECHTREE::COMMON)
			(TECHTREE::RES_SUPPLY1 | TECHTREE::RES_WEAPONS1 | TECHTREE::RES_WEAPONS2); 
		mission_tech.race[0].common_extra =
			(TECHTREE::COMMON_EXTRA)
			(TECHTREE::RES_TENDER1 | TECHTREE::RES_TANKER1 | TECHTREE::RES_SENSORS1); 
	}

	M_ SetAvailiableTech(mission_tech);
}

//--------------------------------------------------------------------------//


// Mission Briefing 

#define NUM_STARTING_MANTIS_EYESTOCKS 2
#define LOCTELE_TIME 2000

CQSCRIPTPROGRAM(TM2_Briefing, TM2_Briefing_Data,CQPROGFLAG_STARTBRIEFING);

void TM2_Briefing::Initialize (U32 eventFlags, const MPartRef & part)
{	
	data.failureID = 0;
	data.mission_over = false;
	data.mhandle = 0;
	data.shandle = 0;
	data.num_mantis_ships = NUM_STARTING_MANTIS;
	data.mission_state = Briefing;
	data.mantisgate_destroyed = false;
	data.bHurtBase = data.bPlayed = false;
	data.bOutpostBegun = data.bOutpostDone = data.bTroopDone = false;
	state = Begin;

	data.bJumpgateAttacked = data.bJumpgateDestroyed = false;
	data.numMantisEyeStocksLeft = NUM_STARTING_MANTIS_EYESTOCKS;

	M_ PlayMusic( NULL );

	M_ EnableBriefingControls(true);
}

bool TM2_Briefing::Update (void)
{
	if (data.mission_state != Briefing)
		return false;

	switch (state)
	{
		case Begin:

			M_ FreeBriefingSlot(-1);
			
			M_ FlushTeletype();
#ifdef _TELETYPE_SPEECH
			data.mhandle = TeletypeBriefing(IDS_TM2_TELETYPE_LOCATION, false);
			//data.mhandle = M_ PlayBriefingTeletype(IDS_TM2_TELETYPE_LOCATION, MOTextColor, 6000, LOCTELE_TIME, false);
#else

			//FIX TO STANDARDIZE WITH MISSION 1
			TeletypeBriefing(IDS_TM2_TELETYPE_LOCATION, false);

			timer = 0;
			//TeletypeBriefing(IDS_TM2_TELETYPE_LOCATION, true);
			//timer = 128;

#endif
			state = Begin2;
			break;
		
		case Begin2:

#ifdef _TELETYPE_SPEECH
			if (!M_ IsTeletypePlaying(data.mhandle))
#else
			if (timer > 0) { timer--; break; }
			else
#endif
			{
				//data.mhandle = DoBriefingSpeech(IDS_TM2_DAVIS_M02JD01, "M02JD01.wav", 8000, 2000, 0, CHAR_NOCHAR);
				data.mhandle = DoBriefingSpeech(IDS_TM2_SUB_PROLOG02, "PROLOG02.wav", 8000, 2000, 0, CHAR_NOCHAR);
				//state = Begin3;
				state = PreHalseyBrief;

				ShowBriefingAnimation(0, "TNRLogo", ANIMSPD_TNRLOGO, true, true);
				ShowBriefingAnimation(3, "TNRLogo", ANIMSPD_TNRLOGO, true, true);
				ShowBriefingAnimation(1, "Radiowave", ANIMSPD_RADIOWAVE, true, true);
				ShowBriefingAnimation(2, "Radiowave", ANIMSPD_RADIOWAVE, true, true);
			}

			break;


		case PreHalseyBrief:

			if (!M_ IsStreamPlaying(data.mhandle))	//audio
			{
				M_ FreeBriefingSlot(-1);

				M_ PlayAudio("high_level_data_uplink.wav");

				ShowBriefingAnimation(0, "Xmission", ANIMSPD_XMISSION, false, false); 
				state = HalseyBrief;
			}
			break;

		case HalseyBrief:
			
	    	if (M_ IsCustomBriefingAnimationDone(0))	
			{
				data.mhandle = DoBriefingSpeechMagpie(IDS_TM2_SUB_M02HA04, "M02HA04.wav", 8000, 2000, 0, CHAR_HALSEY);
				state = Beacon;
			}
			break;

		case Beacon:
		
	    	if (!M_ IsStreamPlaying(data.mhandle))	//audio
			{
				// complete black box recording
				data.mhandle = DoBriefingSpeech(IDS_TM2_SUB_M02HA01B, "M02HA01b.wav", 8000, 2000, 0, CHAR_NOCHAR);

				ShowBriefingAnimation(1, "BlackBox");
				ShowBriefingAnimation(2, "Boxwave", ANIMSPD_BOXWAVE, true, true);
				ShowBriefingAnimation(3, "Hawkesonscreen");
				
				// M02HA01b NOW CONTAINS WHOLE BLACK BOX RECORDING
				state = HalseyBrief2;
			    //state = Beacon2;
			}
			break;


		case HalseyBrief2:

	    	if (!M_ IsStreamPlaying(data.mhandle))	//audio
			{
				M_ FreeBriefingSlot(1);
				M_ FreeBriefingSlot(2);

				M_ FlushStreams();
				data.mhandle = DoBriefingSpeechMagpie(IDS_TM2_SUB_M02HA08, "M02HA08.wav", 8000, 2000, 0, CHAR_HALSEY);
			    state = HalseyBrief3;
			}
			break;

		case HalseyBrief3:

			if (!M_ IsStreamPlaying(data.mhandle))	//audio
			{
				M_ FlushStreams();
				data.mhandle = DoBriefingSpeechMagpie(IDS_TM2_SUB_M02HA08A, "M02HA08a.wav", 8000, 2000, 0, CHAR_HALSEY, false);
			    state = DisplayMO;
			}
			break;
		
		case DisplayMO:
		
		    if (!M_ IsStreamPlaying(data.mhandle))	//audio
			{
				M_ FreeBriefingSlot(0);
				//M_ FreeBriefingSlot(2);
				M_ FreeBriefingSlot(3);

			    M_ FlushTeletype();
				M_ FlushStreams();
				data.mhandle = TeletypeBriefing(IDS_TM2_OBJECTIVES, true);
				//data.mhandle = M_ PlayBriefingTeletype(IDS_TM2_OBJECTIVES, MOTextColor, TEXT_HOLD_TIME_INF, TEXT_PRINT_TIME, false);
			    state = Finish;
			}
		    break;
		
		case Finish:
		
		    if (!M_ IsTeletypePlaying(data.mhandle))
				return false;
			break;

	}

	return true;
}


//--------------------------------------------------------------------------//

CQSCRIPTPROGRAM(TM2_InGameScene, TM2_InGameScene_Save, CQPROGFLAG_STARTMISSION);

void TM2_InGameScene::Initialize (U32 eventFlags, const MPartRef & part)
{
	Duck1 = M_ GetPartByName("SittingDuck1");
	SceneWeaver = M_ GetPartByName("SceneWeaver");

	AlkSpawn = M_ GetPartByName("locAlkWormhole");
	TauWayPoint = M_ GetPartByName("LocTauWormhole");
	GatePoint = M_ GetPartByName("Gate3"); 

	M_ MoveCamera(M_ GetPartByName("Blackwell"), 0, MOVIE_CAMERA_JUMP_TO);


}

bool TM2_InGameScene::Update (void)
{
	timer -= ELAPSED_TIME;

	switch(state)
	{
		case Begin:
				UnderMissionControl();
				M_ EnableMovieMode(true);

				data.mhandle = DoSpeech(IDS_TM2_SUB_M02BL09, "M02BL09.wav", 8000, 2000, M_ GetPartByName("Blackwell"), CHAR_BLACKWELL);

				M_ OrderBuildPlatform(SceneWeaver, GatePoint, "PLATJUMP!!M_JumpPlat");

				state = AttackJumpgate;
			break;

		case AttackJumpgate:
			Target = FindFirstObjectOfType(M_JUMPPLAT, MANTIS_ID, TAU_CETI);

			if(Target.isValid() && M_ IsVisibleToPlayer(Target, PLAYER_ID) && !IsSomethingPlaying(data.mhandle))
			{
				M_ MoveCamera(M_ GetPartByName("Gate2"), 0, MOVIE_CAMERA_JUMP_TO);
				data.mhandle = DoSpeech(IDS_TM2_SUB_M02BL80, "M02BL80.wav", 8000, 2000, M_ GetPartByName("Blackwell"), CHAR_BLACKWELL);

				M_ OrderAttack(Duck1, Target);
				M_ OrderAttack(Duck2, Target);

				state = SendMantis;
			}
			break;

		case SendMantis:
			if(Duck1->supplies < Duck1->supplyPointsMax)
			{
				MGroupRef group;

				for(int x = 0; x < 4; x++)
				{
					MPartRef Attacker = M_ CreatePart("GBOAT!!M_Frigate", AlkSpawn, MANTIS_ID);

					group += Attacker;
				}
					M_ OrderAttack(group, Duck1);

				state = LeaveTau;
			}
			break;

		case LeaveTau:
			if(!Duck1.isValid())
			{
				MassMove(M_FRIGATE, MANTIS_ID, AlkSpawn, TAU_CETI);

				timer = 3;
				state = Done;
			}
			break;

		case Done:
			if(timer < 0)
			{
				M_ RunProgramByName("TM2_MissionStart", MPartRef());

				M_ EnableMovieMode(false);
				
				UnderPlayerControl();

				state = End;
			}
			break;

		case End:
			if(M_ IsIdle(SceneWeaver))
			{
				M_ RemovePart(SceneWeaver);
				return false;
			}
			break;
	}
	return true;
}


//--------------------------------------------------------------------------//
//	Mission Start (after briefing)

CQSCRIPTPROGRAM(TM2_MissionStart, TM2_MissionStart_Data,0);

void TM2_MissionStart::Initialize (U32 eventFlags, const MPartRef & part)
{
	data.mission_over = false;
	data.bNoControl = false;
	data.mhandle = 0;
	data.shandle = 0;
	data.num_mantis_ships = NUM_STARTING_MANTIS;
	data.mission_state = Briefing;
	data.mantisgate_destroyed = false;
	data.bOutpostBegun = data.bOutpostDone = data.bTroopDone = false;
	data.bJumpgateAttacked = data.bJumpgateDestroyed = false;
	data.numMantisEyeStocksLeft = NUM_STARTING_MANTIS_EYESTOCKS;
	data.bBallStart = data.bBallDone = data.bFleetReady = data.bNewObj8 = false;


	data.mission_state = Starting;
	data.Blackwell = M_ GetPartByName("Blackwell");
	M_ EnablePartnameDisplay(data.Blackwell, true);

	MPartRef Eye1, Eye2;

	Eye1 = FindFirstObjectOfType(M_EYESTOCK, MANTIS_ID, MANTIS_SPACE);
	Eye2 = FindNextObjectOfType(M_EYESTOCK, MANTIS_ID, MANTIS_SPACE, Eye1);

	M_ MakeNonAutoTarget(Eye1, true);
	M_ MakeNonAutoTarget(Eye2, true);
	
	SetTechLevel(1);

	//Take control from player
	//UnderMissionControl();

	//Set Scout Carriers to stand their ground
	MassStance(0, MANTIS_ID, US_ATTACK, MANTIS_SPACE);
	MassStance(M_SCOUTCARRIER, MANTIS_ID, US_DEFEND, MANTIS_SPACE);

	M_ EnableJumpgate(M_ GetPartByName("Gate1"), false);
	M_ EnableJumpgate(M_ GetPartByName("Gate4"), false);
	M_ EnableJumpgate(M_ GetPartByName("Gate2"), false);

	M_ EnableSystem(TERRA, false, false);
	M_ EnableSystem(TAU_CETI, true, true);
	M_ EnableSystem(MANTIS_SPACE, false, false);
	M_ EnableSystem(4, false, false);
	M_ EnableSystem(5, false, false);

	M_ SetEnemyCharacter(MANTIS_ID, IDS_TM2_MANTIS);

	//set starting resources
	M_ SetMaxGas(PLAYER_ID, PLAYER_MAX_GAS);
	M_ SetMaxMetal(PLAYER_ID, PLAYER_MAX_METAL);
	M_ SetMaxCrew(PLAYER_ID, PLAYER_MAX_CREW);

	M_ SetGas(PLAYER_ID, PLAYER_STARTING_GAS);
	M_ SetMetal(PLAYER_ID, PLAYER_STARTING_METAL);
	M_ SetCrew(PLAYER_ID, PLAYER_STARTING_CREW);

	MPartRef tmp;

	//remove fog from terran space
	M_ ClearHardFog(data.Blackwell, 255);
	//M_ ClearHardFog(M_ GetPartByName("Earth Planet"), 255);

	tmp = FindFirstObjectOfType(M_JUMPPLAT, MANTIS_ID, TAU_CETI);
	if (tmp.isValid())
		M_ ClearHardFog(tmp, 1);

	//create AI and then deactivate
	M_ EnableEnemyAI(MANTIS_ID, true, "MANTIS_FRIGATE_RUSH");
	M_ EnableEnemyAI(MANTIS_ID, false, "MANTIS_FRIGATE_RUSH");

	tmp = M_ GetFirstPart();
	while (tmp.isValid())
	{
		if (tmp->mObjClass == M_EYESTOCK)
		{
			M_ SetMaxHullPoints(tmp, 4000);
			M_ SetHullPoints(tmp, 4000);
		}
		tmp = M_ GetNextPart(tmp);
	}

	// Create triggers around asteroid belt
	data.AsteroidTrigger = M_ CreatePart("MISSION!!TRIGGER",M_ GetPartByName("LocAlkAsteroidBelt"), 0);
	// Set trigger to detect player nearby
	M_ SetTriggerFilter(data.AsteroidTrigger, PLAYER_ID, TRIGGER_PLAYER, false);
	M_ SetTriggerProgram(data.AsteroidTrigger, "TM2_foundasteroid");
	M_ SetTriggerRange(data.AsteroidTrigger, 3);
	M_ EnableTrigger(data.AsteroidTrigger, true);

	// Create trigger around first Scout Carrier
	data.AsteroidTrigger2 = M_ CreatePart("MISSION!!TRIGGER",M_ GetPartByName("LocAlkAsteroidBelt2"), 0);
	// Set trigger to detect player nearby
	M_ SetTriggerFilter(data.AsteroidTrigger2, PLAYER_ID, TRIGGER_PLAYER, false);
	M_ SetTriggerProgram(data.AsteroidTrigger2, "TM2_foundasteroid");
	M_ SetTriggerRange(data.AsteroidTrigger2, 3);
	M_ EnableTrigger(data.AsteroidTrigger2, true);

	// Create trigger around Alkaid 2
	data.AlkTwoTrigger = M_ CreatePart("MISSION!!TRIGGER",M_ GetPartByName("Alkaid 2"), 0);
	// Set trigger to detect player
	M_ SetTriggerFilter(data.AlkTwoTrigger, PLAYER_ID, TRIGGER_PLAYER, false);
	M_ SetTriggerProgram(data.AlkTwoTrigger, "TM2_MantisBase");
	M_ SetTriggerRange(data.AlkTwoTrigger, 4);
	M_ EnableTrigger(data.AlkTwoTrigger, true);

	// Create trigger around tau ceti prime
	
	data.PlanetTrigger = M_ CreatePart("MISSION!!TRIGGER",M_ GetPartByName("Gate2"), 0);
	// Set trigger to detect a supply depot in process
	M_ SetTriggerFilter(data.PlanetTrigger, M_TENDER, TRIGGER_MOBJCLASS, false);
	M_ SetTriggerFilter(data.PlanetTrigger, 0, TRIGGER_NOFORCEREADY, true);
	M_ SetTriggerFilter(data.PlanetTrigger, PLAYER_ID, TRIGGER_PLAYER, true);
	M_ SetTriggerProgram(data.PlanetTrigger, "TM2_starteddepot");
	M_ SetTriggerRange(data.PlanetTrigger, 200);
	M_ EnableTrigger(data.PlanetTrigger, true);

	// Create trigger around tau ceti prime
	data.PlanetTrigger2 = M_ CreatePart("MISSION!!TRIGGER",M_ GetPartByName("Gate3"), 0);
	// Set trigger to detect a supply depot in process
	M_ SetTriggerFilter(data.PlanetTrigger2, M_TENDER, TRIGGER_MOBJCLASS, false);
	M_ SetTriggerFilter(data.PlanetTrigger2, 0, TRIGGER_NOFORCEREADY, true);
	M_ SetTriggerFilter(data.PlanetTrigger2, PLAYER_ID, TRIGGER_PLAYER, true);
	M_ SetTriggerProgram(data.PlanetTrigger2, "TM2_starteddepot");
	M_ SetTriggerRange(data.PlanetTrigger2, 200);
	M_ EnableTrigger(data.PlanetTrigger2, true);
	

	// Create trigger around tau ceti wormhole
	data.WormholeTrigger = M_ CreatePart("MISSION!!TRIGGER",M_ GetPartByName("Gate2"), 0);
	// Set trigger to detect a jumpgate in process
	M_ SetTriggerFilter(data.WormholeTrigger, M_TROOPSHIP, TRIGGER_MOBJCLASS, false);
	M_ SetTriggerFilter(data.WormholeTrigger, 0, TRIGGER_NOFORCEREADY, true);
	M_ SetTriggerFilter(data.WormholeTrigger, PLAYER_ID, TRIGGER_PLAYER, true);
	M_ SetTriggerProgram(data.WormholeTrigger, "TM2_TroopStarted");
	M_ SetTriggerRange(data.WormholeTrigger, 200);
	M_ EnableTrigger(data.WormholeTrigger, true);

	// Create trigger around alkaid wormhole
	data.WormholeTriggerB = M_ CreatePart("MISSION!!TRIGGER",M_ GetPartByName("Gate3"), 0);
	// Set trigger to detect a jumpgate in process
	M_ SetTriggerFilter(data.WormholeTriggerB, M_TROOPSHIP, TRIGGER_MOBJCLASS, false);
	M_ SetTriggerFilter(data.WormholeTriggerB, 0, TRIGGER_NOFORCEREADY, true);
	M_ SetTriggerFilter(data.WormholeTriggerB, PLAYER_ID, TRIGGER_PLAYER, true);
	M_ SetTriggerProgram(data.WormholeTriggerB, "TM2_TroopStarted");
	M_ SetTriggerRange(data.WormholeTriggerB, 200);
	M_ EnableTrigger(data.WormholeTriggerB, true);

	// Create trigger2 around wormhole
	data.WormholeTrigger2 = M_ CreatePart("MISSION!!TRIGGER",M_ GetPartByName("Gate2"), 0);
	// Set trigger to detect a fabricator nearby
	M_ SetTriggerFilter(data.WormholeTrigger2, PLAYER_ID, TRIGGER_PLAYER, false);
	M_ SetTriggerProgram(data.WormholeTrigger2, "TM2_nearwormhole");
	M_ SetTriggerRange(data.WormholeTrigger2, 3);
	M_ EnableTrigger(data.WormholeTrigger2, true);

	M_ SetMissionID(1);
	M_ SetMissionName(IDS_TM2_MISSION_NAME);
	M_ SetMissionDescription(IDS_TM2_MISSION_DESC);

	M_ AddToObjectiveList(IDS_TM2_OBJECTIVE2);
	M_ AddToObjectiveList(IDS_TM2_OBJECTIVE3);
	M_ AddToObjectiveList(IDS_TM2_OBJECTIVE4);
	M_ AddToObjectiveList(IDS_TM2_OBJECTIVE5);

	M_ EnableRegenMode(true);

	M_ RunProgramByName("TM2_GlobalProc", MPartRef ());
	M_ RunProgramByName("TM2_BuildBallistics", MPartRef ());
	M_ RunProgramByName("TM2_AlkPrimeVisible", MPartRef());

	//Cleanup
	M_ FlushTeletype();
	M_ FlushStreams();
	
	//M_ MoveCamera(M_ GetPartByName("Tau Ceti Prime"), 0, MOVIE_CAMERA_JUMP_TO);
	M_ ChangeCamera(M_ GetPartByName("MissionCamStart"), 0, MOVIE_CAMERA_JUMP_TO);

	M_ PlayMusic( DETERMINATION );

}

bool TM2_MissionStart::Update (void)
{
	return false;
}

//--------------------------------------------------------------------------//
//	Build Jump Gate

void SendAttackToTauCeti()
{
	MPartRef locCreate, Mantis1, Mantis2, Mantis3;
	MGroupRef group;
	locCreate = M_ GetPartByName("locAlkWormhole");

	Mantis1 = M_ CreatePart("GBOAT!!M_Frigate", locCreate, MANTIS_ID);
	Mantis2 = M_ CreatePart("GBOAT!!M_Frigate", locCreate, MANTIS_ID);
	Mantis3 = M_ CreatePart("GBOAT!!M_Frigate", locCreate, MANTIS_ID);
	data.num_mantis_ships += 3;

	group += Mantis1;
	group += Mantis2;
	group += Mantis3;
	M_ OrderAttack(group, M_ FindNearestEnemy(Mantis1, true, false));
}

/********************/
//  Trigger programs

CQSCRIPTPROGRAM(TM2_nearwormhole, TM2_nearwormhole_Data, 0);

void TM2_nearwormhole::Initialize(U32 eventFlags, const MPartRef & part)
{
	M_ EnableTrigger(data.WormholeTrigger2, false);	
	M_ ClearPath(TAU_CETI, MANTIS_SPACE, MANTIS_ID);
	M_ MakeAreaVisible(MANTIS_ID, M_ GetPartByName("Tau Ceti Prime"), 100);

	SendAttackToTauCeti();
}

bool TM2_nearwormhole::Update (void)
{
	return false;
}

CQSCRIPTPROGRAM(TM2_AlkPrimeVisible, TM2_AlkPrimeVisible_Data, 0);

void TM2_AlkPrimeVisible::Initialize(U32 eventFlags, const MPartRef & part)
{

}

bool TM2_AlkPrimeVisible::Update (void)
{
	if(!IsSomethingPlaying(data.mhandle) && M_ IsVisibleToPlayer(M_ GetPartByName("Alkaid Prime"), PLAYER_ID))
	{
		data.mhandle = DoSpeech(IDS_TM2_SUB_M02BL20A, "M02BL20a.wav", 8000, 2000, data.Blackwell, CHAR_BLACKWELL);

		return false;
	}


	return true;
}

CQSCRIPTPROGRAM(TM2_starteddepot, TM2_starteddepot_Data, 0);

void TM2_starteddepot::Initialize(U32 eventFlags, const MPartRef & part)
{
	M_ EnableTrigger(data.PlanetTrigger, false);	
	M_ EnableTrigger(data.PlanetTrigger2, false);	

}

bool TM2_starteddepot::Update (void)
{
	if(!IsSomethingPlaying(data.mhandle))
	{
		data.mhandle = DoSpeech(IDS_TM2_SUB_M02BL20B, "M02BL20b.wav", 8000, 2000, data.Blackwell, CHAR_BLACKWELL);

		return false;
	}


	return true;
}


CQSCRIPTPROGRAM(TM2_TroopStarted, TM2_TroopStarted_Data, 0);

void TM2_TroopStarted::Initialize(U32 eventFlags, const MPartRef & part)
{
	M_ EnableTrigger(data.WormholeTrigger, false);	
	M_ EnableTrigger(data.WormholeTriggerB, false);	

}

bool TM2_TroopStarted::Update (void)
{
	if(!IsSomethingPlaying(data.mhandle))
	{
		data.mhandle = DoSpeech(IDS_TM2_SUB_M02BL26, "M02BL26.wav", 8000, 2000, data.Blackwell, CHAR_BLACKWELL);

		return false;
	}


	return true;
}
//--------------------------------------------------------------------------//
//	Build Ballistics

CQSCRIPTPROGRAM(TM2_BuildBallistics, TM2_BuildBallistics_Data, 0);

void TM2_BuildBallistics::Initialize (U32 eventFlags, const MPartRef & part)
{
	data.mission_state = BuildBallistics;
	state = PreBegin;

	// Create trigger around tau ceti prime
	if (data.PlanetTrigger.isValid())
		M_ DestroyPart(data.PlanetTrigger);
	data.PlanetTrigger = M_ CreatePart("MISSION!!TRIGGER",M_ GetPartByName("Tau Ceti Prime"), 0);
	// Set trigger to detect a ballistics in process
	M_ SetTriggerFilter(data.PlanetTrigger, M_BALLISTICS, TRIGGER_MOBJCLASS, false);
	M_ SetTriggerFilter(data.PlanetTrigger, 0, TRIGGER_NOFORCEREADY, true);
	M_ SetTriggerFilter(data.PlanetTrigger, PLAYER_ID, TRIGGER_PLAYER, true);
	M_ SetTriggerProgram(data.PlanetTrigger, "TM2_startedballistics");
	M_ SetTriggerRange(data.PlanetTrigger, 3);
	M_ EnableTrigger(data.PlanetTrigger, true);

	// Create trigger around tau ceti two
	if (data.PlanetTrigger3.isValid())
		M_ DestroyPart(data.PlanetTrigger3);
	data.PlanetTrigger3 = M_ CreatePart("MISSION!!TRIGGER",M_ GetPartByName("Alkaid Prime"), 0);
	// Set trigger to detect a ballistics in process
	M_ SetTriggerFilter(data.PlanetTrigger3, M_BALLISTICS, TRIGGER_MOBJCLASS, false);
	M_ SetTriggerFilter(data.PlanetTrigger3, 0, TRIGGER_NOFORCEREADY, true);
	M_ SetTriggerFilter(data.PlanetTrigger3, PLAYER_ID, TRIGGER_PLAYER, true);
	M_ SetTriggerProgram(data.PlanetTrigger3, "TM2_startedballistics");
	M_ SetTriggerRange(data.PlanetTrigger3, 3);
	M_ EnableTrigger(data.PlanetTrigger3, true);

}

bool TM2_BuildBallistics::Update (void)
{
	if (data.mission_over)
		return false;

	switch (state)
	{
		case PreBegin:

			M_ AddToObjectiveList(IDS_TM2_OBJECTIVE6);
//			data.mhandle = DoSpeech(IDS_TM2_BLACKWELL_M02BL09, "M02BL09.wav", 8000, 2000, data.Blackwell, CHAR_BLACKWELL);
			state = PreBegin2;
			break;

		case PreBegin2:

			if (!IsSomethingPlaying(data.mhandle))
			{
				if (!data.bBallStart)
					data.shandle = TeletypeObjective(IDS_TM2_OBJECTIVE6);
				//data.shandle = M_ PlayTeletype(IDS_TM2_OBJECTIVE6, SuperLeft, SuperTop, SuperRight, SuperBottom, GreenColor, SuperHoldTime, 1000, false);
				state = Begin;
			}
			break;

		case Begin:

			if (data.bBallStart)	//have we started ballistics?
			{ 
				data.mhandle = DoSpeech(IDS_TM2_SUB_M02BL14, "M02BL14.wav", 8000, 2000, data.Blackwell, CHAR_BLACKWELL);
				state=Building; 
			} 
			break;

		case Building:
			if (data.bBallDone)		//finished?
			{ 
				data.mhandle = DoSpeech(IDS_TM2_SUB_M02BL15, "M02BL15.wav", 8000, 2000, data.Blackwell, CHAR_BLACKWELL);
				state=Done; 
			} 
			break;

		case Done:
			M_ AddToObjectiveList(IDS_TM2_OBJECTIVE7);
			M_ RunProgramByName("TM2_BuildCruisers", MPartRef ());
			return false;
	}

	return true;
}

CQSCRIPTPROGRAM(TM2_startedballistics, TM2_startedballistics_Data, 0);

void TM2_startedballistics::Initialize(U32 eventFlags, const MPartRef & part)
{
	M_ SetTriggerFilter(data.PlanetTrigger, M_BALLISTICS, TRIGGER_MOBJCLASS, false);
	M_ SetTriggerFilter(data.PlanetTrigger, 0, TRIGGER_FORCEREADY, true);
	M_ SetTriggerFilter(data.PlanetTrigger, PLAYER_ID, TRIGGER_PLAYER, true);
	M_ SetTriggerProgram(data.PlanetTrigger, "TM2_finishedballistics");

	M_ SetTriggerFilter(data.PlanetTrigger3, M_BALLISTICS, TRIGGER_MOBJCLASS, false);
	M_ SetTriggerFilter(data.PlanetTrigger3, 0, TRIGGER_FORCEREADY, true);
	M_ SetTriggerFilter(data.PlanetTrigger3, PLAYER_ID, TRIGGER_PLAYER, true);
	M_ SetTriggerProgram(data.PlanetTrigger3, "TM2_finishedballistics");

	data.bBallStart = true;
}
bool TM2_startedballistics::Update (void)
{
	return false;
}

CQSCRIPTPROGRAM(TM2_finishedballistics, TM2_finishedballistics_Data, 0);

void TM2_finishedballistics::Initialize(U32 eventFlags, const MPartRef & part)
{
	M_ EnableTrigger(data.PlanetTrigger, false);	
	M_ EnableTrigger(data.PlanetTrigger3, false);	

	M_ MarkObjectiveCompleted(IDS_TM2_OBJECTIVE6);

	data.bBallDone = true;
}
bool TM2_finishedballistics::Update (void)
{	
	return false;
}

//--------------------------------------------------------------------------//
//	Build Cruisers

#define NUM_CRUISERS 3
CQSCRIPTPROGRAM(TM2_BuildCruisers, TM2_BuildCruisers_Data, 0);

void TM2_BuildCruisers::Initialize (U32 eventFlags, const MPartRef & part)
{
	data.mission_state = BuildCruisers;
	state = PreBegin;
}

bool TM2_BuildCruisers::Update (void)
{
	if (data.mission_over)
		return false;

	switch (state)
	{
		case PreBegin:

			if (!IsSomethingPlaying(data.mhandle))
			{
				if (CountObjects(M_MISSILECRUISER, PLAYER_ID) == 0)
					data.shandle = TeletypeObjective(IDS_TM2_OBJECTIVE7);
				//data.shandle = M_ PlayTeletype(IDS_TM2_OBJECTIVE7, SuperLeft, SuperTop, SuperRight, SuperBottom, GreenColor, SuperHoldTime, 1000, false);
				state = Begin;
			}
			break;

		case Begin:

			if (CountObjects(M_MISSILECRUISER, PLAYER_ID) > 0)
			{
//				data.mhandle = DoSpeech(IDS_TM2_BLACKWELL_M02BL16, "M02BL16.wav", 8000, 2000, data.Blackwell, CHAR_BLACKWELL);
				state = Building; 

				SetTechLevel(2);
			}
			break;
		
		case Building:

			if (data.bFleetReady)	//got 1 cruiser and 6 corvettes?
			{ 
				state = Done; 

				if (!data.mantisgate_destroyed)
				{
					data.mhandle = DoSpeech(IDS_TM2_SUB_M02BL16C, "M02BL16c.wav", 8000, 2000, data.Blackwell, CHAR_BLACKWELL);
				}
			}
			break;

		case Done:

			if (data.mantisgate_destroyed)
			{
				M_ EnableSystem(MANTIS_SPACE, true, true);
				M_ RunProgramByName("TM2_JumpToAlkaid", MPartRef ());
				//M_ RunProgramByName("TM2_BuildGate", MPartRef ());
				return false;
			}
	}

	return true;
}

//--------------------------------------------------------------------------//
//	Jump to Alkaid

CQSCRIPTPROGRAM(TM2_JumpToAlkaid, TM2_JumpToAlkaid_Data, 0);

void TM2_JumpToAlkaid::Initialize (U32 eventFlags, const MPartRef & part)
{
	data.mission_state = JumpToAlkaid;
	state = Begin;

	//Set up 2 Scouts in Asteroid field
	MPartRef locCreate;
	locCreate = M_ GetPartByName("locScout1");
	data.Scout1 = M_ CreatePart("GBOAT!!M_Scout Carrier", locCreate, MANTIS_ID);
	locCreate = M_ GetPartByName("locScout2");
	data.Scout2 = M_ CreatePart("GBOAT!!M_Scout Carrier", locCreate, MANTIS_ID);
	M_ SetStance(data.Scout1, US_DEFEND);
	M_ SetStance(data.Scout2, US_DEFEND);
	data.num_mantis_ships += 2;
}

bool TM2_JumpToAlkaid::Update (void)
{
	MPartRef part;

	if (data.mission_over)
		return false;

	switch (state)
	{
		case Begin:
			
			if (!IsSomethingPlaying(data.mhandle))
			{
				M_ EnableJumpgate(M_ GetPartByName("Gate2"), true);

				data.mhandle = DoSpeech(IDS_TM2_SUB_M02BL16A, "M02BL16a.wav", 8000, 2000, data.Blackwell, CHAR_BLACKWELL);
				state = Jump;
			}
			
			break;

		case Jump:

			part = FindFirstObjectOfType(M_MISSILECRUISER, PLAYER_ID, MANTIS_SPACE);
			if(!part.isValid())
				part = FindFirstObjectOfType(M_CORVETTE, PLAYER_ID, MANTIS_SPACE);
			if (part.isValid())
			{
				state = Blackwell;
			}
			break;

		case Blackwell:

			data.mhandle = DoSpeech(IDS_TM2_SUB_M02BL16B, "M02BL16b.wav", 8000, 2000, data.Blackwell, CHAR_BLACKWELL);
			SetTechLevel(3);
			state = Done;
			break;

		case Done:

			if (!M_ IsStreamPlaying(data.mhandle))
			{
				M_ MarkObjectiveCompleted(IDS_TM2_OBJECTIVE3);
				M_ RunProgramByName("TM2_MantisAttack", MPartRef ());
				return false;
			}
			break;
	}

	return true;
}

//--------------------------------------------------------------------------//
//	Mantis attack !

CQSCRIPTPROGRAM(TM2_MantisAttack, TM2_MantisAttack_Data, 0);

void TM2_MantisAttack::Initialize (U32 eventFlags, const MPartRef & part)
{
	data.mission_state = MantisAttack;
	state = Begin;
	timer = 40;
	
	locAlkWormhole = M_ GetPartByName("locAlkWormhole");

	MPartRef frig, locCreate = M_ GetPartByName("LocMantisAttack");
	frig = M_ CreatePart("GBOAT!!M_Frigate", locCreate, MANTIS_ID);
	M_ OrderMoveTo(frig, M_ GetPartByName("Gate3"));
	M_ SetStance(frig, US_ATTACK);
	frig = M_ CreatePart("GBOAT!!M_Frigate", locCreate, MANTIS_ID);
	M_ OrderMoveTo(frig, M_ GetPartByName("Gate3"));
	M_ SetStance(frig, US_ATTACK);
	frig = M_ CreatePart("GBOAT!!M_Frigate", locCreate, MANTIS_ID);
	M_ OrderMoveTo(frig, M_ GetPartByName("Gate3"));
	M_ SetStance(frig, US_ATTACK);
	
	data.num_mantis_ships += 3;

	MPartRef cruiser;
	cruiser = FindFirstObjectOfType(M_MISSILECRUISER, PLAYER_ID, MANTIS_SPACE);
	if (cruiser.isValid())
		M_ MakeAreaVisible(MANTIS_ID, cruiser, 3);

	MPartRef gunboat;
	gunboat = FindFirstObjectOfType(M_MISSILECRUISER, PLAYER_ID, MANTIS_SPACE);
	if (!gunboat.isValid())
		gunboat = FindFirstObjectOfType(M_CORVETTE, PLAYER_ID, MANTIS_SPACE);
	if (gunboat.isValid())
		M_ MoveCamera(gunboat, 0, MOVIE_CAMERA_JUMP_TO);

	data.mhandle = DoSpeech(IDS_TM2_SUB_M02BL17, "M02BL17.wav", 8000, 2000, data.Blackwell, CHAR_BLACKWELL);
}

bool TM2_MantisAttack::Update (void)
{
	MPartRef locCreate, frig;

	if (data.mission_over)
		return false;

	switch (state)
	{

		case Begin:

			if (timer > 0) { timer--; break; }

			//if (M_ IsVisibleToPlayer(Frig1, PLAYER_ID) || M_ IsVisibleToPlayer(Frig2, PLAYER_ID) ||
			//	M_ IsVisibleToPlayer(Frig3, PLAYER_ID))
			//{
				state = Wave2;
			//}
			break;

		case Wave1:

			locCreate = M_ GetPartByName("LocMantisAttack");
			frig = M_ CreatePart("GBOAT!!M_Frigate", locCreate, MANTIS_ID);
			M_ OrderMoveTo(frig, M_ GetPartByName("Gate3"));
			M_ SetStance(frig, US_ATTACK);
			frig = M_ CreatePart("GBOAT!!M_Frigate", locCreate, MANTIS_ID);
			M_ OrderMoveTo(frig, M_ GetPartByName("Gate3"));
			M_ SetStance(frig, US_ATTACK);
			frig = M_ CreatePart("GBOAT!!M_Frigate", locCreate, MANTIS_ID);				
			M_ OrderMoveTo(frig, M_ GetPartByName("Gate3"));
			M_ SetStance(frig, US_ATTACK);
			data.num_mantis_ships += 3;

			state=Wave2; 
			break;

		case Wave2:
			
			//if (M_ IsDead(Frig1) && M_ IsDead(Frig2) && M_ IsDead(Frig3) && M_ IsDead(Frig4) && 
			//	M_ IsDead(Frig5) && M_ IsDead(Frig6))		
			if (CountObjectsInRange(M_FRIGATE, MANTIS_ID, locAlkWormhole, 6) == 0)
			{
				data.mhandle = DoSpeech(IDS_TM2_SUB_M02BL18, "M02BL18.wav", 8000, 2000, data.Blackwell, CHAR_BLACKWELL);
				state = Done;
			}
			break;

		case Done:

			M_ RunProgramByName("TM2_ScoutAlkaid", MPartRef ());
			return false;
	}

	return true;
}

//--------------------------------------------------------------------------//
//	Scout Alkaid

CQSCRIPTPROGRAM(TM2_ScoutAlkaid, TM2_ScoutAlkaid_Data, 0);

void TM2_ScoutAlkaid::Initialize (U32 eventFlags, const MPartRef & part)
{
	data.mission_state = ScoutAlkaid;
	bFoundScouts = false;

	state = Begin;
}

bool TM2_ScoutAlkaid::Update (void)
{
	if (data.mission_over)
		return false;


	if (!bFoundScouts && (M_ IsVisibleToPlayer(data.Scout1, PLAYER_ID) || M_ IsVisibleToPlayer(data.Scout2, PLAYER_ID)))
	{
		data.mhandle = DoSpeech(IDS_TM2_SUB_M02BL19, "M02BL19.wav", 8000, 2000, data.Blackwell, CHAR_BLACKWELL);
		
		bFoundScouts = true;
	}


	switch (state)
	{
		case Begin:

			break;

		case Done:

			return false;
	}

	return true;
}


CQSCRIPTPROGRAM(TM2_foundasteroid, TM2_foundasteroid_Data, 0);

void TM2_foundasteroid::Initialize(U32 eventFlags, const MPartRef & part)
{
	M_ EnableTrigger(data.AsteroidTrigger, false);
	M_ EnableTrigger(data.AsteroidTrigger2, false);
}
bool TM2_foundasteroid::Update (void)
{
	if (data.mission_over)
		return false;

	if (!IsSomethingPlaying(data.mhandle))
	{
		data.mhandle = DoSpeech(IDS_TM2_SUB_M02BL20, "M02BL20.wav", 8000, 2000, data.Blackwell, CHAR_BLACKWELL);
		return false;
	}
	else
		return true;
}

//--------------------------------------------------------------------------//
//	Found Alkaid 2, more battle ensues

CQSCRIPTPROGRAM(TM2_MantisBase, TM2_MantisBase_Data, 0);

void TM2_MantisBase::Initialize (U32 eventFlags, const MPartRef & part)
{
	//data.mission_state = MantisBase;
	MPartRef A2 = M_ GetPartByName("Alkaid 2");
	M_ AlertMessage(A2, NULL);

	state = Begin;
	timer = 0;

	M_ EnableTrigger(data.AlkTwoTrigger, false);

	MPartRef Thripid;
	Thripid = FindFirstObjectOfType(M_THRIPID, MANTIS_ID, MANTIS_SPACE);
	if (!M_ IsDead(Thripid))
	{
		M_ OrderBuildUnit(Thripid, "GBOAT!!M_Scout Carrier");
	}

	// Let AI control Alkaid 2 Base.  Should build frigs n scouts and attack Alk One
	SetupAI();

	MassStance(M_FRIGATE, MANTIS_ID, US_ATTACK, MANTIS_SPACE);
	MassStance(M_SCOUTCARRIER, MANTIS_ID, US_ATTACK, MANTIS_SPACE);

	MPartRef tmp=part;
	MassAttack(M_FRIGATE, MANTIS_ID, tmp, MANTIS_SPACE);

	//M_ AddToObjectiveList(IDS_TM2_OBJECTIVE8);

	M_ PlayMusic( DANGER );
}

bool TM2_MantisBase::Update (void)
{
	if (data.mission_over)
		return false;

	switch (state)
	{
		case Begin:

			if (!IsSomethingPlaying(data.mhandle))
			{
				data.mhandle = DoSpeech(IDS_TM2_SUB_M02BL21, "M02BL21.wav", 8000, 2000, data.Blackwell, CHAR_BLACKWELL);
				MassStance(0, MANTIS_ID, US_ATTACK, MANTIS_SPACE);

				state = Gawking;
			}
			break;

		case Gawking:

			if (!IsSomethingPlaying(data.mhandle))
			{
				M_ AddToObjectiveList(IDS_TM2_OBJECTIVE9);
				data.mhandle = DoSpeech(IDS_TM2_SUB_M02BL22, "M02BL22.wav", 8000, 2000, data.Blackwell, CHAR_BLACKWELL);
				
				MPartRef Frig1, Frig2, Frig3, Scout1, locCreate, locAlkTwo, Zorap;
				MGroupRef group;
				locAlkTwo = M_ GetPartByName("LocNearAlkTwo");

				if (!M_ IsDead(data.Jumpgate_ALK2TC))
				{
					// Create 3 frigs to go attack jumpgate
					locCreate = M_ GetPartByName("LocAlkCreation");
					Frig1 = M_ CreatePart("GBOAT!!M_Frigate", locCreate, MANTIS_ID);
					Frig2 = M_ CreatePart("GBOAT!!M_Frigate", locCreate, MANTIS_ID);
					Frig3 = M_ CreatePart("GBOAT!!M_Frigate", locCreate, MANTIS_ID);
					Zorap = M_ CreatePart("SUPSHIP!!M_Zorap", locCreate, MANTIS_ID);

					M_ EnableAIForPart(Frig1, false);
					M_ EnableAIForPart(Frig2, false);
					M_ EnableAIForPart(Frig3, false);
					M_ EnableAIForPart(Zorap, false);

					group += Frig1;
					group += Frig2;
					group += Frig3;

					M_ OrderAttack(group, data.Jumpgate_ALK2TC);
					M_ OrderMoveTo(Zorap, data.Jumpgate_ALK2TC);
					data.num_mantis_ships += 4;
				}

				//Just throw more at them for fun
				locCreate = M_ GetPartByName("LocAlkTwoCreation");
				Frig1 = M_ CreatePart("GBOAT!!M_Frigate", locCreate, MANTIS_ID);
				Frig2 = M_ CreatePart("GBOAT!!M_Frigate", locCreate, MANTIS_ID);
				Scout1 = M_ CreatePart("GBOAT!!M_Scout Carrier", locCreate, MANTIS_ID);
				M_ OrderMoveTo(Frig1, locAlkTwo);
				M_ OrderMoveTo(Frig2, locAlkTwo);
				M_ OrderMoveTo(Scout1, locAlkTwo);
				data.num_mantis_ships += 3;

				SetTechLevel(4);

				state = NewObj;
			}
			break;

		case NewObj:

			if (!IsSomethingPlaying(data.mhandle))
			{
				if (!M_ IsObjectiveCompleted(IDS_TM2_OBJECTIVE9))
					data.shandle = TeletypeObjective(IDS_TM2_OBJECTIVE9);
				//data.shandle = M_ PlayTeletype(IDS_TM2_OBJECTIVE9, SuperLeft, SuperTop, SuperRight, SuperBottom, GreenColor, SuperHoldTime, 1000, false);
				state = AIControlled;
			}
			break;

		case AIControlled:

			if (!data.bHurtBase)
			{
				MPartRef part;
				bool bGo=false;
				
				part = M_ GetFirstPart();
				while (!bGo && part.isValid())
				{
					//plat1 = FindFirstObjectOfType(M_COCOON, MANTIS_ID, MANTIS_SPACE);
					//plat2 = FindFirstObjectOfType(M_THRIPID, MANTIS_ID, MANTIS_SPACE);
					//plat3 = FindFirstObjectOfType(M_EYESTOCK, MANTIS_ID, MANTIS_SPACE);
					if (part->playerID == MANTIS_ID && M_ IsPlatform(part) && part->mObjClass == M_EYESTOCK)
					{
										
						if (part->hullPoints < ((double)part->hullPointsMax*((double)9/(double)10))) 
							bGo=true;
					}

					part = M_ GetNextPart(part);
				}
				
				if (bGo)
				{
					data.mhandle = DoSpeech(IDS_TM2_SUB_M02BL22A, "M02BL22a.wav", 8000, 2000, data.Blackwell, CHAR_BLACKWELL);
					data.bHurtBase = true;
				}
			}

			if (!data.bNewObj8 && data.bTroopDone && !IsSomethingPlaying(data.mhandle))
			{
				data.shandle = TeletypeObjective(IDS_TM2_OBJECTIVE8);
				data.bNewObj8 = true;
			}

			if (CountObjects(M_THRIPID, PLAYER_ID) > 0 || CountObjects(M_COCOON, PLAYER_ID) > 0 || CountObjects(M_BLASTFURNACE, PLAYER_ID) > 0 || CountObjects(M_EYESTOCK, PLAYER_ID) > 0)
			{
				M_ RunProgramByName("TM2_MissionSuccess", MPartRef ());
				state = Done;
			}
			break;

		case Done:

			return false;
	}

	return true;
}

//-----------------------------------------------------------------------
// OBJECT CREATION TRIGGER

CQSCRIPTPROGRAM(TM2_ObjectBuilt, TM2_ObjectBuilt_Data, CQPROGFLAG_OBJECTCONSTRUCTED);

void TM2_ObjectBuilt::Initialize (U32 eventFlags, const MPartRef & part)
{
	if (data.mission_over) 
		return;

	switch (part->mObjClass)
	{
		case M_MISSILECRUISER:
		case M_CORVETTE:
			if (data.mission_state == BuildCruisers &&
				CountObjects(M_CORVETTE, PLAYER_ID) >= 6 &&
				CountObjects(M_MISSILECRUISER, PLAYER_ID) >= 3)
			{
				data.bFleetReady = true;
				M_ MarkObjectiveCompleted(IDS_TM2_OBJECTIVE2);
				M_ RemoveFromObjectiveList(IDS_TM2_OBJECTIVE6);
				M_ RemoveFromObjectiveList(IDS_TM2_OBJECTIVE7);
			}
			break;

		case M_TROOPSHIP:

			if (!data.bTroopDone)
			{
				data.mhandle = DoSpeech(IDS_TM2_SUB_M02BL27, "M02BL27.wav", 8000, 2000, data.Blackwell, CHAR_BLACKWELL);
				data.bTroopDone = true;
				M_ MarkObjectiveCompleted(IDS_TM2_OBJECTIVE9);
				M_ AddToObjectiveList(IDS_TM2_OBJECTIVE8);
			}
			break;

		case M_FRIGATE:
		case M_SCOUTCARRIER:
			// When the computer builds a Mantis attack ship...
			data.num_mantis_ships++;
			break;
		case M_JUMPPLAT:
			if ((part->systemID == TAU_CETI && M_ GetJumpgateDestination(part) == MANTIS_SPACE) ||
				(part->systemID == MANTIS_SPACE && M_ GetJumpgateDestination(part) == TAU_CETI))
			{
				if(part->playerID == PLAYER_ID)
				{
					if (part->systemID == TAU_CETI)
					{
						data.Jumpgate_TC2ALK = part;
						data.Jumpgate_ALK2TC = M_ GetJumpgateSibling(part);
					}
					else
					{
						data.Jumpgate_TC2ALK = M_ GetJumpgateSibling(part);
						data.Jumpgate_ALK2TC = part;
					}
				}
			}
			break;
	}
}

bool TM2_ObjectBuilt::Update (void)
{
	return false;
}

//-----------------------------------------------------------------------
// OBJECT DESTRUCTION TRIGGER

CQSCRIPTPROGRAM(TM2_ObjectDestroyed, TM2_ObjectDestroyed_Data,CQPROGFLAG_OBJECTDESTROYED);

void TM2_ObjectDestroyed::Initialize (U32 eventFlags, const MPartRef & part)
{
	if (data.mission_over) 
		return;

	switch (part->mObjClass)
	{

		case M_FRIGATE:
		case M_SCOUTCARRIER:
			
			data.num_mantis_ships--;

			/*
			if (data.mission_state >= FrigateRuns && SameObj(part, data.FrigateRunner) )
				M_ RunProgramByName("TM2_FrigateRunDestroyed", MPartRef ());
			*/
			
			//if (data.mission_state == MantisBase)
			//	CheckMantisBaseStatus();
			
			break;
	
		//case M_COCOON:
		//case M_THRIPID:
		case M_EYESTOCK:
		
			if (part->playerID == MANTIS_ID)
			{
				data.numMantisEyeStocksLeft--;
				if (data.numMantisEyeStocksLeft <= 0)
				{
					data.mhandle = DoSpeech(IDS_TM2_SUB_M02BL32, "M02BL32.wav", 8000, 2000, data.Blackwell, CHAR_BLACKWELL);
					M_ RunProgramByName("TM2_MissionFailure", MPartRef ());

					M_ MoveCamera(part, 0, MOVIE_CAMERA_JUMP_TO);
				}
			}
			break;

		case M_CORVETTE:
			
			if (part == data.Blackwell)
			{
				M_ RunProgramByName("TM2_BlackwellKilled", MPartRef ());
			}
			break;
		
		case M_JUMPPLAT:

			if (part->playerID == PLAYER_ID && !data.bJumpgateDestroyed)
				M_ RunProgramByName("TM2_jumpgatedestroyed", MPartRef ());

			if (part->playerID == MANTIS_ID)
				M_ RunProgramByName("TM2_mantisgatedestroyed", MPartRef ());
			
			break;			
	}
	
}

bool TM2_ObjectDestroyed::Update (void)
{
	return false;
}

//-----------------------------------------------------------------------
// Jumpgate attacked

CQSCRIPTPROGRAM(TM2_jumpgateattacked, TM2_jumpgateattacked_Data, 0);

void TM2_jumpgateattacked::Initialize (U32 eventFlags, const MPartRef & part)
{
	data.bJumpgateAttacked = true;
}

bool TM2_jumpgateattacked::Update (void)
{
	if (data.mission_over)
		return false;

	if (!IsSomethingPlaying(data.mhandle))
	{
		data.mhandle = DoSpeech(IDS_TM2_SUB_M02BL23, "M02BL23.wav", 8000, 2000, data.Blackwell, CHAR_BLACKWELL);
		return false;
	}

	return true;
}

//-----------------------------------------------------------------------
// Jumpgate destroyed

CQSCRIPTPROGRAM(TM2_jumpgatedestroyed, TM2_jumpgatedestroyed_Data, 0);

void TM2_jumpgatedestroyed::Initialize (U32 eventFlags, const MPartRef & part)
{
	data.bJumpgateDestroyed = true;
}

bool TM2_jumpgatedestroyed::Update (void)
{
	if (data.mission_over)
		return false;

	if (!IsSomethingPlaying(data.mhandle))
	{
		data.mhandle = DoSpeech(IDS_TM2_SUB_M02BL24, "M02BL24.wav", 8000, 2000, data.Blackwell, CHAR_BLACKWELL);
		return false;
	}


	return true;
}

//-----------------------------------------------------------------------
// Mantis Jumpgate destroyed

CQSCRIPTPROGRAM(TM2_mantisgatedestroyed, TM2_mantisgatedestroyed_Data, 0);

void TM2_mantisgatedestroyed::Initialize (U32 eventFlags, const MPartRef & part)
{
	if (data.mission_over)
		return;

	data.mantisgate_destroyed = true;
	
	data.mhandle = DoSpeech(IDS_TM2_SUB_M02BL10, "M02BL10.wav", 8000, 2000, data.Blackwell, CHAR_BLACKWELL);
	timer = 30;
}

bool TM2_mantisgatedestroyed::Update (void)
{
	if (data.mission_over)
		return false;

	if (timer > 0) { timer--; return true; }

	SendAttackToTauCeti();

	return false;
}

//-----------------------------------------------------------------------
// OBJECT CREATION TRIGGER

CQSCRIPTPROGRAM(TM2_ForbiddenJump, TM2_ForbiddenJump_Data, CQPROGFLAG_FORBIDEN_JUMP);

void TM2_ForbiddenJump::Initialize (U32 eventFlags, const MPartRef & part)
{
	if (data.mission_over)
		return;
	
	gate2 = M_ GetPartByName("Gate2");
	gate4 = M_ GetPartByName("Gate4");


	if (!IsSomethingPlaying(data.mhandle) && part == gate2)
	{
		data.mhandle = DoSpeech(IDS_TM2_SUB_M02BL13, "M02BL13.wav", 8000, 2000, data.Blackwell, CHAR_BLACKWELL);
	}

	if (!IsSomethingPlaying(data.mhandle) && part == gate4)
	{
		data.mhandle = DoSpeech(IDS_TM2_SUB_M01BL16, "M01BL16.wav", 8000, 2000, data.Blackwell, CHAR_BLACKWELL);
	}
}

bool TM2_ForbiddenJump::Update (void)
{
	return false;
}

//---------------------------------------------------------------------------
// MISSION FAILURE PROGRAMS


CQSCRIPTPROGRAM(TM2_BlackwellKilled, TM2_BlackwellKilled_Data, 0);

void TM2_BlackwellKilled::Initialize (U32 eventFlags, const MPartRef & part)
{
	data.mission_over = true;
	state = Begin;

	data.mhandle = DoSpeech(IDS_TM2_SUB_M02BL34, "M02BL34.wav", 8000, 2000, data.Blackwell, CHAR_BLACKWELL);

	if (!data.bNoControl) {	UnderMissionControl(); M_ EnableMovieMode(true); data.bNoControl=true; }

	M_ MoveCamera(data.Blackwell, 0, MOVIE_CAMERA_JUMP_TO);

	M_ MarkObjectiveFailed(IDS_TM2_OBJECTIVE5);
}

bool TM2_BlackwellKilled::Update (void)
{
	switch (state)
	{
		case Begin:

			if (!IsSomethingPlaying(data.mhandle))
			{
				data.mhandle = M_ PlayAudio("high_level_data_uplink.wav");
				state = Done;
			}
			break;

		case Done:

			if (!M_ IsStreamPlaying(data.mhandle))
			{
				data.mhandle = DoSpeech(IDS_TM2_SUB_M02HA35, "M02HA35.wav", 8000, 2000, MPartRef(), CHAR_HALSEY);
				data.failureID = IDS_TM2_FAIL_BLACKWELL_LOST;
				M_ RunProgramByName("TM2_MissionFailure", MPartRef ());
				return false;
			}
			break;
	}

	return true;
}

//---------------------------------------------------------------------------

#define TELETYPE_TIME 3000

CQSCRIPTPROGRAM(TM2_MissionFailure, TM2_MissionFailure_Data, 0);

void TM2_MissionFailure::Initialize (U32 eventFlags, const MPartRef & part)
{
	data.mission_over = true;

	if (!data.bNoControl) {	UnderMissionControl(); M_ EnableMovieMode(true); data.bNoControl=true; }

	state = Begin;
}

bool TM2_MissionFailure::Update (void)
{
	switch (state)
	{
		case Begin:
			if (!M_ IsStreamPlaying(data.mhandle))
			{
				M_ FlushTeletype();
				data.mhandle = TeletypeMissionOver(IDS_TM2_TELETYPE_FAILURE,data.failureID);
				//data.mhandle = M_ PlayTeletype(IDS_TM2_TELETYPE_FAILURE, SuperLeft, SuperTop, SuperRight, SuperBottom, GreenColor, SuperHoldTime, TELETYPE_TIME, false);
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
// MISSION SUCCESS PROGRAMS

CQSCRIPTPROGRAM(TM2_MissionSuccess, TM2_MissionSuccess_Data, 0);

void TM2_MissionSuccess::Initialize (U32 eventFlags, const MPartRef & part)
{
	M_ MarkObjectiveCompleted(IDS_TM2_OBJECTIVE4);
	M_ MarkObjectiveCompleted(IDS_TM2_OBJECTIVE5);
	M_ MarkObjectiveCompleted(IDS_TM2_OBJECTIVE8);

	//to stop the battles
	M_ SetAllies(PLAYER_ID, MANTIS_ID, true);
	M_ SetAllies(MANTIS_ID, PLAYER_ID, true);

	data.mission_over = true;

	if (!data.bNoControl) {	UnderMissionControl(); M_ EnableMovieMode(true); data.bNoControl=true; }

	MPartRef newplat;
	newplat = FindFirstObjectOfType(M_THRIPID, PLAYER_ID, MANTIS_SPACE);

	if (!newplat.isValid())
		newplat = FindFirstObjectOfType(M_COCOON, PLAYER_ID, MANTIS_SPACE);

	if (!newplat.isValid())
		newplat = FindFirstObjectOfType(M_EYESTOCK, PLAYER_ID, MANTIS_SPACE);

	if (newplat.isValid())
	M_ MoveCamera(newplat, 0, MOVIE_CAMERA_JUMP_TO);

	state = Marine;
}

bool TM2_MissionSuccess::Update (void)
{
	switch (state)
	{

		case Marine:

			if (!IsSomethingPlaying(data.mhandle))
			{
				data.mhandle = DoSpeech(IDS_TM2_SUB_M02MC28, "M02MC28.wav", 8000, 2000, MPartRef(), CHAR_MARINE);
				state = Blackwell;
			}
			break;

		case Blackwell:

			if (!IsSomethingPlaying(data.mhandle))
			{
				data.mhandle = DoSpeech(IDS_TM2_SUB_M02BL29, "M02BL29.wav", 8000, 2000, data.Blackwell, CHAR_BLACKWELL);
				state = Marine2;
			}
			break;

		case Marine2:

			if (!IsSomethingPlaying(data.mhandle))
			{
				data.mhandle = DoSpeech(IDS_TM2_SUB_M02MC30, "M02MC30.wav", 8000, 2000, MPartRef(), CHAR_MARINE);
				state = Intel;
			}
			break;

		case Intel:

			if (!IsSomethingPlaying(data.mhandle))
			{
				data.mhandle = DoSpeech(IDS_TM2_SUB_M02IO31, "M02IO31.wav", 8000, 2000, MPartRef(), CHAR_INTEL);
				state = PreHalsey;
			}
			break;

		case PreHalsey:

			if (!IsSomethingPlaying(data.mhandle))
			{
				data.mhandle = M_ PlayAudio("high_level_data_uplink.wav");
				state = Halsey;
			}
			break;

		case Halsey:

			if (!M_ IsStreamPlaying(data.mhandle))
			{
				data.mhandle = DoSpeech(IDS_TM2_SUB_M02HA31A, "M02HA31a.wav", 8000, 2000, MPartRef(), CHAR_HALSEY);
				state = TeleType;
			}
			break;

		case TeleType:

			if (!IsSomethingPlaying(data.mhandle))
			{
				M_ FlushTeletype();

				U32 displayTime;
				displayTime = MScript::GetScriptStringLength( IDS_TM2_TELETYPE_SUCCESS ) * STANDARD_TELETYPE_MILS_PER_CHAR;
				data.mhandle = M_ PlayTeletype( IDS_TM2_TELETYPE_SUCCESS, 
						MISSION_OVER_TELETYPE_MIN_X_POS, MISSION_OVER_TELETYPE_MIN_Y_POS-30, 
						MISSION_OVER_TELETYPE_MAX_X_POS, MISSION_OVER_TELETYPE_MAX_Y_POS, 
						STANDARD_TELETYPE_COLOR, STANDARD_TELETYPE_HOLD_TIME, displayTime,
						false, true );
				//data.mhandle = TeletypeMissionOver(IDS_TM2_TELETYPE_SUCCESS);
				
				state = Done;
			}
			break;

		case Done:
			if (!M_ IsTeletypePlaying(data.mhandle))
			{
				UnderPlayerControl();
				M_ EndMissionVictory(1);
				return false;
			}
	}
	return true;
}

//--------------------------------End Script02T.cpp-------------------------//
//--------------------------------------------------------------------------//
