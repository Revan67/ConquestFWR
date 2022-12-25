//--------------------------------------------------------------------------//
//                                                                          //
//                                Script05T.cpp                             //
//                                                                          //
//               COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
   $Header: /Conquest/App/Src/Scripts/Script05T/Script05T.cpp 163   7/06/01 11:07a Tmauer $
*/
//--------------------------------------------------------------------------//

#include <ScriptDef.h>
#include <DLLScriptMain.h>
#include "DataDef.h"
#include "stdlib.h"

#include "..\helper\helper.h"



//--------------------------------------------------------------------------
//  TM5 #define's


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
#define TEXT_HOLD_TIME		0


// Talking Heads "Time To Live" in seconds

CQBRIEFINGITEM slotAnim;

//---
//--------------------------------------------------------------------------//
//-----------------------------Seperate Functions---------------------------//
//--------------------------------------------------------------------------//


void SetupAI(bool bOn)
{

	M_ EnableEnemyAI(MANTIS_ID, bOn, "MANTIS_FRIGATE_RUSH");

	AIPersonality airules;
	airules.difficulty = EASY;
	airules.buildMask.bBuildPlatforms = true;
	airules.buildMask.bBuildLightGunboats = true;
	airules.buildMask.bBuildMediumGunboats = true;
	airules.buildMask.bBuildHeavyGunboats = false;
	airules.buildMask.bHarvest = true;
	airules.buildMask.bScout = true;
	airules.buildMask.bBuildAdmirals = false;
	airules.buildMask.bLaunchOffensives = false;
	airules.buildMask.bUnitDependencyRules = true;
	airules.nGunboatsPerSupplyShip = 8;
	airules.nNumMinelayers = 1;
	airules.uNumScouts = 2;
	airules.nNumFabricators = 1;
	airules.fHarvestersPerRefinery = 2;
	airules.nMaxHarvesters = 5;
	airules.buildMask.bUseSpecialWeapons = false;
	airules.nPanicThreshold = 5000;

	M_ SetEnemyAIRules(MANTIS_ID, airules);

}

void ToggleBuildUnits(bool bOn)
{

	AIPersonality airules;
	airules.difficulty = EASY;
	airules.buildMask.bBuildPlatforms = true;
	airules.buildMask.bBuildLightGunboats = bOn;
	airules.buildMask.bBuildMediumGunboats = bOn;
	airules.buildMask.bBuildHeavyGunboats = false;
	airules.buildMask.bHarvest = true;
	airules.buildMask.bScout = false;
	airules.buildMask.bBuildAdmirals = false;
	airules.buildMask.bLaunchOffensives = true;
	airules.buildMask.bUnitDependencyRules = true;
	airules.nGunboatsPerSupplyShip = 8;
	airules.nNumMinelayers = 1;
	airules.uNumScouts = 2;
	airules.nNumFabricators = 2;
	airules.fHarvestersPerRefinery = 2;
	airules.nMaxHarvesters = 5;
	airules.buildMask.bUseSpecialWeapons = false;
	airules.nPanicThreshold = 5000;

	M_ SetEnemyAIRules(MANTIS_ID, airules);
}

bool SameObj(const MPartRef & obj1, const MPartRef & obj2)
{
	if (obj1->dwMissionID == obj2->dwMissionID)
		return true;
	else
		return false;
}
//--------------------------------------------------------------------------
//--------------------------End Seperate Functions--------------------------
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
//  Mission functions: Terran Campaign Mission 5
//--------------------------------------------------------------------------

CQSCRIPTDATA(MissionData, data);

//--------------------------------------------------------------------------//
//	An attempt to try and monitor and manage the Enemy AI

CQSCRIPTPROGRAM(TM5_AIManager, TM5_AIManager_Save,0);

void TM5_AIManager::Initialize (U32 eventFlags, const MPartRef & part)
{

}

bool TM5_AIManager::Update (void)
{
	if(data.mission_over == true)
	{
		return false;
	}

 //Set up cases in which the AI will turn off
	if((M_ GetUsedCommandPoints(MANTIS_ID) > 70 || M_ GetUsedCommandPoints(PLAYER_ID) < 20))
	{
		ToggleBuildUnits(false);

		return true;
	}

 //Set up cases in which the AI will turn back on
	if((M_ GetUsedCommandPoints(MANTIS_ID) < 30 || M_ GetUsedCommandPoints(PLAYER_ID) > 69))
	{
		ToggleBuildUnits(true);
	}

	return true;
}

//--------------------------------------------------------------------------//
//	Mission Briefing
//--------------------------------------------------------------------------//

CQSCRIPTPROGRAM(TM5_Briefing, TM5_Briefing_Save,CQPROGFLAG_STARTBRIEFING);

void TM5_Briefing::Initialize (U32 eventFlags, const MPartRef & part)
{
	data.failureID = 0;
	data.mission_over = data.briefing_over = false;
	data.mhandle = 0;
	data.shandle = 0;
	data.mission_state = Briefing;

	state = Begin;

	M_ PlayMusic(NULL);
	M_ EnableBriefingControls(true);

	state = Begin;
}

bool TM5_Briefing::Update (void)
{
	if (data.briefing_over)
		return false;

	switch (state)
	{
		case Begin:

			//Set Camera to Long Range
/*			M_ ChangeCamera(M_ GetPartByName("BriefCam1"), 0, MOVIE_CAMERA_JUMP_TO);

			M_ OrderMoveTo(data.Hawkes, data.Blackwell);

			//Close in on the Terran Base
			M_ ChangeCamera(M_ GetPartByName("BriefCam2"), 15, MOVIE_CAMERA_SLIDE_TO | MOVIE_CAMERA_QUEUE);
			M_ MoveCamera(M_ GetPartByName("BriefCam3"), 12, MOVIE_CAMERA_SLIDE_TO | MOVIE_CAMERA_QUEUE);

			MassMove(M_FABRICATOR, PLAYER_ID, data.PerilonJumpGate, ALKAID_SYSTEM);
*/
			strcpy(slotAnim.szTypeName, "Animate!!TNRLogo");
			slotAnim.slotID = 0;
			slotAnim.dwTimer = ANIMSPD_TNRLOGO;
			slotAnim.bLoopAnimation = true;
			data.mhandle = M_ PlayBriefingAnimation(slotAnim);

			strcpy(slotAnim.szTypeName, "Animate!!Radiowave");
			slotAnim.slotID = 1;
			slotAnim.dwTimer = ANIMSPD_RADIOWAVE;
			data.mhandle = M_ PlayBriefingAnimation(slotAnim);

			strcpy(slotAnim.szTypeName, "Animate!!Radiowave");
			slotAnim.slotID = 2;
			slotAnim.dwTimer = ANIMSPD_RADIOWAVE;
			data.mhandle = M_ PlayBriefingAnimation(slotAnim);

			strcpy(slotAnim.szTypeName, "Animate!!TNRLogo");
			slotAnim.slotID = 3;
			slotAnim.dwTimer = ANIMSPD_TNRLOGO;
			data.mhandle = M_ PlayBriefingAnimation(slotAnim);

			data.mhandle = M_ PlayAudio("prolog05.wav");
			MScript::BriefingSubtitle(data.mhandle,IDS_TM5_SUB_PROLOG05);

			state = TeleTypeLocation;

			break;

		case TeleTypeLocation:

			M_ FlushTeletype();
			data.shandle = TeletypeBriefing(IDS_TM5_TELETYPE_LOCATION, false);
			state = Uplink;

			break;

		case Uplink:

			if (!M_ IsStreamPlaying(data.mhandle))
			{
				data.shandle = M_ PlayAudio("high_level_data_uplink.wav");
				state = Fuzz;
			}
			break;

		case Fuzz:
			if(M_ IsStreamPlaying(data.shandle))
			{
				strcpy(slotAnim.szTypeName, "Animate!!Xmission");
				slotAnim.slotID = 0;
				slotAnim.dwTimer = ANIMSPD_XMISSION;
				slotAnim.bLoopAnimation = false;
				data.mhandle = M_ PlayBriefingAnimation(slotAnim);

				M_ FreeBriefingSlot(1);
				M_ FreeBriefingSlot(2);
				M_ FreeBriefingSlot(3);

				state = HalseyBrief;
			}
			break;

		case HalseyBrief:


			if(M_ IsCustomBriefingAnimationDone(0))
			{
				M_ FlushStreams();

				strcpy(slotAnim.szFileName, "m05ha02.wav");
				strcpy(slotAnim.szTypeName, "Animate!!Halsey");
				slotAnim.slotID = 0;
				slotAnim.bHighlite = false;
				data.mhandle = M_ PlayBriefingTalkingHead(slotAnim);
				MScript::BriefingSubtitle(data.mhandle,IDS_TM5_SUB_M05HA02);

				state = MO;
			}

			break;


		case MO:

			
			if (!M_ IsStreamPlaying(data.mhandle))
			{
//				M_ OrderMoveTo(data.Hawkes, data.PerilonJumpGate);

				M_ FlushTeletype();
				data.mhandle = TeletypeBriefing(IDS_TM5_OBJECTIVES, true);

				state = Finish;
			}
			break;
			
		case Finish:
			if (!M_ IsTeletypePlaying(data.mhandle))
				{
					M_ RunProgramByName("TM5_BriefingSkip",MPartRef ());
					return false;
				}		
			break;
	}

	return true;
}

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//	Start of the mission... After the briefing

CQSCRIPTPROGRAM(TM5_MissionStart, TM5_MissionStart_Save,CQPROGFLAG_STARTMISSION);

void TM5_MissionStart::Initialize (U32 eventFlags, const MPartRef & part)
{
	M_ EnableMovieMode(true);

	M_ EnableSystem(ALKAID_SYSTEM, true, true);
	M_ EnableSystem(PERILON_SYSTEM, true, true);
	M_ EnableSystem(GHELEN3_SYSTEM, false, false);

	M_ PlayMusic(DETERMINATION);

	M_ EnableRegenMode(true);

	TECHNODE missionTech;

	data.BlackholeDeath = data.LuxCleared = data.WavesSent = false;

	//Set up player Tech lvls

	missionTech.InitLevel( TECHTREE::FULL_TREE );

	missionTech.race[0].build = (TECHTREE::BUILDNODE) (
		TECHTREE::RES_REFINERY_GAS1 |
		TECHTREE::RES_REFINERY_METAL1 |
        TECHTREE::TDEPEND_HEADQUARTERS | 
        TECHTREE::TDEPEND_REFINERY | 
        TECHTREE::TDEPEND_LIGHT_IND |
        TECHTREE::TDEPEND_JUMP_INHIBITOR |
        TECHTREE::TDEPEND_LASER_TURRET |
        TECHTREE::TDEPEND_BALLISTICS |
        TECHTREE::TDEPEND_TENDER |
		TECHTREE::TDEPEND_REPAIR |
		TECHTREE::TDEPEND_HEAVY_IND |
		TECHTREE::TDEPEND_ADVHULL |
		TECHTREE::TDEPEND_OUTPOST |
		TECHTREE::TDEPEND_HANGER  |
		TECHTREE::TDEPEND_SENSORTOWER  |
		TECHTREE::TDEPEND_ACADEMY  |
		TECHTREE::TDEPEND_HEAVY_IND |
		TECHTREE::TDEPEND_SPACE_STATION );

    missionTech.race[1].build = (TECHTREE::BUILDNODE) (
		TECHTREE::MDEPEND_COCOON |
		TECHTREE::MDEPEND_COLLECTOR |
		TECHTREE::MDEPEND_PLANTATION |
		TECHTREE::MDEPEND_THRIPID |
		TECHTREE::MDEPEND_EYESTOCK |
		TECHTREE::MDEPEND_WARLORDTRAIN |
		TECHTREE::MDEPEND_CARRIONROOST |
		TECHTREE::MDEPEND_BLASTFURNACE |
		TECHTREE::MDEPEND_EXPLSVRANGE |
		TECHTREE::MDEPEND_BIOFORGE |
		TECHTREE::MDEPEMD_JUMP_PLAT |
		TECHTREE::MDEPEND_PLASMA_SPITTER
        );

    // add in necessary upgrades, and troopship

	missionTech.race[0].tech = (TECHTREE::TECHUPGRADE) (
        TECHTREE::T_SHIP__FABRICATOR | 
        TECHTREE::T_SHIP__HARVEST |
		TECHTREE::T_SHIP__SUPPLY |
        TECHTREE::T_SHIP__CORVETTE | 
        TECHTREE::T_SHIP__MISSILECRUISER |
		TECHTREE::T_SHIP__BATTLESHIP |
		TECHTREE::T_SHIP__TROOPSHIP |
  		TECHTREE::T_SHIP__CARRIER  |
		TECHTREE::T_SHIP__INFILTRATOR  |
		TECHTREE::T_RES_TROOPSHIP1 |
		TECHTREE::T_RES_MISSLEPACK1 |
		TECHTREE::T_RES_MISSLEPACK2
        );

	missionTech.race[1].tech = (TECHTREE::TECHUPGRADE) (
		TECHTREE::M_SHIP_WEAVER |
		TECHTREE::M_SHIP_SPINELAYER |
		TECHTREE::M_SHIP_SIPHON |
		TECHTREE::M_SHIP_SEEKER |
		TECHTREE::M_SHIP_SCOUTCARRIER |
		TECHTREE::M_SHIP_FRIGATE |
		TECHTREE::M_SHIP_HIVECARRIER |
		TECHTREE::M_SHIP_LEECH |
		TECHTREE::M_SHIP_KHAMIR |
		TECHTREE::M_RES_LEECH1 |
		TECHTREE::M_RES_EXPLODYRAM1
		);

	missionTech.race[0].common_extra = missionTech.race[1].common_extra = (TECHTREE::COMMON_EXTRA) (
        TECHTREE::RES_SENSORS1  |
		TECHTREE::RES_TANKER1  |
		TECHTREE::RES_TANKER2  |
		TECHTREE::RES_TENDER1 |
		TECHTREE::RES_FIGHTER1
        );

	missionTech.race[0].common = missionTech.race[1].common = (TECHTREE::COMMON) (
		TECHTREE::RES_WEAPONS1  |
		TECHTREE::RES_WEAPONS2  |
		TECHTREE::RES_SUPPLY1 |
		TECHTREE::RES_SUPPLY2  |
		TECHTREE::RES_HULL1 
        );

    MScript::SetAvailiableTech( missionTech );

	//Set each sides resources...

	M_ SetMaxGas(PLAYER_ID, 150);
	M_ SetMaxMetal(PLAYER_ID, 300);
	M_ SetMaxCrew(PLAYER_ID, 200);

	M_ SetGas(PLAYER_ID, 150);
	M_ SetMetal(PLAYER_ID, 300);
	M_ SetCrew(PLAYER_ID, 200);

	M_ SetMaxGas(MANTIS_ID, 500);
	M_ SetMaxMetal(MANTIS_ID, 500);
	M_ SetMaxCrew(MANTIS_ID, 500);

	M_ SetGas(MANTIS_ID, 500);
	M_ SetMetal(MANTIS_ID, 500);
	M_ SetCrew(MANTIS_ID, 500);
	
	data.Blackwell = M_ GetPartByName("Blackwell");
	M_ EnablePartnameDisplay(data.Blackwell, true);
	data.Hawkes    = M_ GetPartByName("Hawkes");
	M_ EnablePartnameDisplay(data.Hawkes, true);

	data.BlackholeAlert = M_ GetPartByName("BlackholeAlert");
	data.Goodhole = M_ GetPartByName("GoodWormhole");

	data.PerilonJumpGate = M_ GetPartByName("JumpGateWP");

	data.PerilontoBekka = M_ GetPartByName("Gate15");
	data.PerilontoLuxor = M_ GetPartByName("Gate10");
	data.LuxortoPerilon = M_ GetPartByName("Gate11");
	data.BekkatoPerilon = M_ GetPartByName("Gate14");
	data.GhelentoLuxA = M_ GetPartByName("Gate19");
	data.LuxtoGhelenA = M_ GetPartByName("Gate18");
	data.GhelentoLuxB = M_ GetPartByName("Gate17");
	data.LuxtoGhelenB = M_ GetPartByName("Gate16");

	SetupAI(true);

	M_ SetMissionName(IDS_TM5_MISSION_NAME);
	M_ SetMissionID(4);

	M_ SetEnemyCharacter(MANTIS_ID, IDS_TM5_MANTIS);

	M_ SetMissionDescription(IDS_TM5_MISSION_DESC);
	M_ AddToObjectiveList(IDS_TM5_OBJECTIVE1);
	M_ AddToObjectiveList(IDS_TM5_OBJECTIVE3);
	M_ AddToObjectiveList(IDS_TM5_OBJECTIVE4);

	M_ EnableJumpgate(data.PerilontoBekka,false);
	M_ EnableJumpgate(data.PerilontoLuxor,false);
	M_ EnableJumpgate(data.LuxortoPerilon,false);
	M_ EnableJumpgate(data.BekkatoPerilon,false);
	M_ EnableJumpgate(data.LuxtoGhelenA,false);
	M_ EnableJumpgate(data.GhelentoLuxA,false);
	M_ EnableJumpgate(data.LuxtoGhelenB,false);
	M_ EnableJumpgate(data.GhelentoLuxB,false);

	data.CorvWaypoint = M_ GetPartByName("CorvWP");
	data.FabWaypoint = M_ GetPartByName("FabWP");
	data.MissleWaypoint = M_ GetPartByName("MissleWP");
	data.BattleWaypoint = M_ GetPartByName("BattleWP");
	data.GoliathWaypoint = M_ GetPartByName("GoliathWP");


	//Set a visible area for the MissionStartCam
	M_ ClearHardFog (M_ GetPartByName("Wormhole to Capella"), 250);
	MPartRef StartPoint = M_ GetPartByName("Gate20");

	MassMove(M_MISSILECRUISER, PLAYER_ID, data.MissleWaypoint, ALKAID_SYSTEM);
	MassMove(M_CORVETTE, PLAYER_ID, data.CorvWaypoint, ALKAID_SYSTEM);
	MassMove(M_FABRICATOR, PLAYER_ID, data.FabWaypoint, ALKAID_SYSTEM);
	MassMove(M_BATTLESHIP, PLAYER_ID, data.BattleWaypoint, ALKAID_SYSTEM);
	MassMove(M_SUPPLY, PLAYER_ID, data.GoliathWaypoint, ALKAID_SYSTEM);
	MassMove(M_INFILTRATOR, PLAYER_ID, data.CorvWaypoint, ALKAID_SYSTEM);

	data.BlackHole = M_ GetPartByName("BlackHole");

	//Set Camera to Mission Start pos
	M_ MoveCamera(M_ GetPartByName("Wormhole to Capella"), 0, MOVIE_CAMERA_JUMP_TO);

	M_ RunProgramByName("TM5_AIManager", MPartRef ());
	M_ RunProgramByName("TM5_HideWormhole", MPartRef());

	state = Begin;
}

bool TM5_MissionStart::Update (void)
{
	if(data.mission_over == true)
	{
		return false;
	}

	switch(state)
	{
		case Begin:
			if(data.Blackwell->systemID == PERILON_SYSTEM)
			{
				data.mhandle = M_ PlayAnimatedMessage("M05HW05.wav","Animate!!Hawkes2", MagpieLeft , MagpieTop, data.Hawkes,IDS_TM5_SUB_M05HW05);
	
				M_ EnableMovieMode(false);

				state = Blackwell;
			}
			break;

		case Blackwell:
			if(!M_ IsStreamPlaying(data.mhandle))
			{
				data.mhandle = M_ PlayAnimatedMessage("M05BL06.wav", "Animate!!Blackwell2", MagpieLeft, MagpieTop, data.Blackwell,IDS_TM5_SUB_M05BL06);

				M_ RunProgramByName("TM5_LocateTheMantis", MPartRef ());
        
				state = Fleet;
			}
			break;

		case Fleet:
			if(!M_ IsStreamPlaying(data.mhandle))
			{
				data.mhandle = M_ PlayAnimatedMessage("M05HW07.wav", "Animate!!Hawkes2", MagpieLeft, MagpieTop, data.Hawkes,IDS_TM5_SUB_M05HW07);
				state = Blackwell2;
			}
			break;
			
		case Blackwell2:
			if(!M_ IsStreamPlaying(data.mhandle))
			{
				data.mhandle = M_ PlayAnimatedMessage("M05BL08.wav", "Animate!!Blackwell2", MagpieLeft, MagpieTop, data.Blackwell,IDS_TM5_SUB_M05BL08);
				state = Done;
			}
			break;

		case Done:
			if(!M_ IsStreamPlaying(data.mhandle))
			{
				data.mhandle = M_ PlayAnimatedMessage("M05HW08a.wav", "Animate!!Hawkes2", MagpieLeft, MagpieTop, data.Hawkes, IDS_TM5_SUB_M05HW08A);
				return false;
			}
	}
    return true;
}

//--------------------------------------------------------------------------//
//	Mission to search Perilon and find the Mantis

CQSCRIPTPROGRAM(TM5_LocateTheMantis, TM5_LocateTheMantis_Save,0);

void TM5_LocateTheMantis::Initialize (U32 eventFlags, const MPartRef & part)
{

}

bool TM5_LocateTheMantis::Update (void)
{
	if(data.mission_over == true)
	{
		return false;
	}

	MPartRef mPlatform = M_ GetFirstPart();

	while (mPlatform.isValid())
	{
		if(M_ IsVisibleToPlayer(mPlatform, PLAYER_ID) && M_ IsPlatform(mPlatform) == true && mPlatform->systemID == PERILON_SYSTEM && mPlatform->playerID == MANTIS_ID && !M_ IsStreamPlaying(data.mhandle))
		{
			data.mhandle = M_ PlayAnimatedMessage("M05HW08c.wav", "Animate!!Hawkes2", MagpieLeft, MagpieTop, data.Hawkes, IDS_TM5_SUB_M05HW08C);

			M_ RunProgramByName("TM5_PerilonCleared", MPartRef ());
			return false;
		}
		mPlatform = M_ GetNextPart(mPlatform);
	}
	return true;
}





//--------------------------------------------------------------------------//
//	Checking to see if the Perilon system is cleared

CQSCRIPTPROGRAM(TM5_PerilonCleared, TM5_PerilonCleared_Save,0);

void TM5_PerilonCleared::Initialize (U32 eventFlags, const MPartRef & part)
{
	state = Begin;

	P1 = M_ GetPartByName("Perilon Prime");
	P2 = M_ GetPartByName("Perilon Two");
	P3 = M_ GetPartByName("Perilon Three");
	P4 = M_ GetPartByName("Perilon Four");
}

bool TM5_PerilonCleared::Update (void)
{
	if(data.mission_over == true)
	{
		return false;
	}

	if(state == Begin && !M_ IsPlayerOnPlanet(P1, MANTIS_ID) && !M_ IsPlayerOnPlanet(P2, MANTIS_ID) && !M_ IsPlayerOnPlanet(P3, MANTIS_ID) && !M_ IsPlayerOnPlanet(P4, MANTIS_ID))
	{
		M_ EnableJumpgate(data.PerilontoBekka,true);
		M_ EnableJumpgate(data.PerilontoLuxor,true);
		M_ EnableJumpgate(data.LuxortoPerilon,true);
		M_ EnableJumpgate(data.BekkatoPerilon,true);

		data.mhandle = M_ PlayAnimatedMessage("M05HW10.wav", "Animate!!Hawkes2", MagpieLeft, MagpieTop, data.Hawkes, IDS_TM5_SUB_M05HW10);

		M_ MarkObjectiveCompleted(IDS_TM5_OBJECTIVE1);
		
		state = Done;
	}

	if(state == Done && !M_ IsTeletypePlaying(data.mhandle))
	{
		M_ RunProgramByName("TM5_LuxorBase1Cleared", MPartRef());
		M_ RunProgramByName("TM5_Ghelen3BlackHole", MPartRef ());

		return false;
	}
	
	return true;
}

//--------------------------------------------------------------------------//
//	Mantis attacks triggered by the lightshipyard being built

CQSCRIPTPROGRAM(TM5_SendMantisWave1, TM5_SendMantisWave1_Save,0);

void TM5_SendMantisWave1::Initialize(U32 eventFlags,const MPartRef &part)
{
	timer = 0;

	HiveGen = M_ GetPartByName("HiveGen");
	ScoutGen = M_ GetPartByName("ScoutGen");

	M_ SetGenType(HiveGen, "GBOAT!!M_Hive Carrier");
	M_ SetGenType(ScoutGen, "GBOAT!!M_Scout Carrier");

}

bool TM5_SendMantisWave1::Update(void)
{
	timer -= ELAPSED_TIME;

	if(data.mission_over == true)
	{
		return false;
	}

	if(CountObjects(M_THRIPID, MANTIS_ID, BEKKA_SYSTEM) != 0 || CountObjects(M_NIAD, MANTIS_ID, BEKKA_SYSTEM) != 0)
	{
		if(timer < 0 && CountObjectsInRange(0, PLAYER_ID, HiveGen, 5) == 0 && M_ GetUsedCommandPoints(MANTIS_ID) < M_ GetMaxCommandPoints(MANTIS_ID))
		{
		    M_ GenEnable(HiveGen, true);
			M_ GenEnable(ScoutGen, true);

			data.WaveScout1 = M_ GenForceGeneration(ScoutGen);
			data.WaveScout2 = M_ GenForceGeneration(HiveGen);
			data.WaveHive1 =	M_ GenForceGeneration(HiveGen);

			AttackNearestEnemy(data.WaveScout1);
			AttackNearestEnemy(data.WaveScout2);
			AttackNearestEnemy(data.WaveHive1);
	
			M_ GenEnable(ScoutGen,false);
			M_ GenEnable(HiveGen,false);
		
			timer = MANTIS_WAVE_INTERVAL;

			return true;
		}



	}
	else
	{
		return false;
	}

	if(data.WaveScout1.isValid() && M_ IsIdle(data.WaveScout1))
	{
		AttackNearestEnemy(data.WaveScout1);
	}
	if(data.WaveScout2.isValid() && M_ IsIdle(data.WaveScout2))
	{
		AttackNearestEnemy(data.WaveScout2);
	}
	if(data.WaveHive1.isValid() && M_ IsIdle(data.WaveHive1))
	{
		AttackNearestEnemy(data.WaveHive1);
	}
		
	return true;
}

//--------------------------------------------------------------------------//
//	Mantis attacks triggered by the heavyshipyard being built

CQSCRIPTPROGRAM(TM5_SendMantisWave2, TM5_SendMantisWave2_Save,0);


void TM5_SendMantisWave2::Initialize(U32 eventFlags,const MPartRef &part)
{
	timer = 0;

	FrigateGen = M_ GetPartByName("FrigateGen");
	ScarabGen = M_ GetPartByName("ScarabGen");

	M_ SetGenType(ScarabGen, "GBOAT!!M_Scarab");
	M_ SetGenType(FrigateGen, "GBOAT!!M_Frigate");

}

bool TM5_SendMantisWave2::Update(void)
{	
	timer -= ELAPSED_TIME;

	if(data.mission_over == true)
	{
		return false;
	}

	if(CountObjects(M_THRIPID, MANTIS_ID, LUXOR_SYSTEM) != 0 || CountObjects(M_NIAD, MANTIS_ID, LUXOR_SYSTEM) != 0)
	{
		if(timer < 0 && CountObjectsInRange(0, PLAYER_ID, ScarabGen, 5) == 0 && M_ GetUsedCommandPoints(MANTIS_ID) < M_ GetMaxCommandPoints(MANTIS_ID))
		{
			M_ GenEnable(FrigateGen, true);
			M_ GenEnable(ScarabGen, true);

			data.WaveFrigate1 = M_ GenForceGeneration(FrigateGen);
			data.WaveFrigate2 = M_ GenForceGeneration(ScarabGen);
			data.WaveScarab1 = M_ GenForceGeneration(ScarabGen);

			AttackNearestEnemy(data.WaveFrigate1);
			AttackNearestEnemy(data.WaveFrigate2);
			AttackNearestEnemy(data.WaveScarab1);

			M_ GenEnable(ScarabGen,false);
			M_ GenEnable(FrigateGen,false);
		
			timer = MANTIS_WAVE_INTERVAL;

			return true;
		}

	}
	else
	{
		return false;
	}

	if(data.WaveFrigate1.isValid() && M_ IsIdle(data.WaveFrigate1))
	{
		AttackNearestEnemy(data.WaveFrigate1);
	}
	if(data.WaveFrigate2.isValid() && M_ IsIdle(data.WaveFrigate2))
	{
		AttackNearestEnemy(data.WaveFrigate2);
	}
	if(data.WaveScarab1.isValid() && M_ IsIdle(data.WaveScarab1))
	{
		AttackNearestEnemy(data.WaveScarab1);
	}
	
	return true;
}
//--------------------------------------------------------------------------//
//	Checking to see if at least 1 base in Luxor system is cleared

CQSCRIPTPROGRAM(TM5_LuxorBase1Cleared, TM5_LuxorBase1Cleared_Save,0);

void TM5_LuxorBase1Cleared::Initialize (U32 eventFlags, const MPartRef & part)
{
	data.Luxor1 = M_ GetPartByName("Luxor Prime");
	data.Luxor2 = M_ GetPartByName("Luxor Two");

	state = Begin;
}

bool TM5_LuxorBase1Cleared::Update (void)
{
	if(data.mission_over == true)
	{
		return false;
	}

	if(state == Begin && (!M_ IsPlayerOnPlanet(data.Luxor1, MANTIS_ID) || !M_ IsPlayerOnPlanet(data.Luxor2, MANTIS_ID)))
	{


//		M_ SaveCameraPos();
//		M_ ChangeCamera(M_ GetPartByName("Ghelen Cam"), 3, MOVIE_CAMERA_SLIDE_TO | MOVIE_CAMERA_QUEUE);
		M_ ClearHardFog(data.LuxtoGhelenA, 1);

		M_ RunProgramByName("TM5_FlashBeacon", MPartRef ());
		data.mhandle = M_ PlayAnimatedMessage("M05BL11.wav", "Animate!!Blackwell2", MagpieLeft, MagpieTop, data.Blackwell, IDS_TM5_SUB_M05BL11);

		M_ EnableJumpgate(data.LuxtoGhelenA,true);
	
		state = Hawkes;
	}

	if(state == Hawkes && !M_ IsStreamPlaying(data.mhandle))
	{
	
		data.mhandle = M_ PlayAnimatedMessage("M05HW12.wav", "Animate!!Hawkes2", MagpieLeft, MagpieTop, data.Hawkes, IDS_TM5_SUB_M05HW12);

//		M_ AlertObjectInSysMap(data.LuxtoGhelenA);
		state = NewMO;
	}

	if(state == NewMO && !M_ IsStreamPlaying(data.mhandle))
	{
		state = Done;
		
		M_ FlushTeletype();
		data.mhandle = TeletypeObjective(IDS_TM5_OBJECTIVE5);
		M_ AddToObjectiveList(IDS_TM5_OBJECTIVE5);

//		M_ LoadCameraPos(3, MOVIE_CAMERA_SLIDE_TO | MOVIE_CAMERA_QUEUE);

	}

	if(state == Done && !M_ IsTeletypePlaying(data.mhandle))
	{
		return false;
	}
	return true;
}

//-----------------------------------------------------------------------

CQSCRIPTPROGRAM(TM5_FlashBeacon, TM5_FlashBeacon_Save, 0);

void TM5_FlashBeacon::Initialize (U32 eventFlags, const MPartRef & part)
{
	count=0;
	data.HoleFound = false;
}

bool TM5_FlashBeacon::Update (void)
{
	if(data.mission_over == true)
	{
		return false;
	}

	if (!data.HoleFound)
	{
		count++;
		if (count >= 25)
		{
			// Highlight the beacon on the players map...
			M_ AlertObjectInSysMap(data.BlackholeAlert, true);
			count = 0;
		}
		return true;
	}
	else
		return false;
}

//--------------------------------------------------------------------------//
//	Once the shipyard is built, initiate the Mantis Waves to attack

CQSCRIPTPROGRAM(TM5_HideWormhole, TM5_HideWormhole_Save,0);

void TM5_HideWormhole::Initialize (U32 eventFlags, const MPartRef & part)
{

}

bool TM5_HideWormhole::Update (void)
{	
	if(!data.LuxCleared)
	{
		M_ ClearVisibility(data.LuxtoGhelenB);
		M_ MakeJumpgateInvisible(data.LuxtoGhelenB, true);
	}
	else
	{
		M_ MakeJumpgateInvisible(data.LuxtoGhelenB, false);
		return false;
	}

	return true;
}

//--------------------------------------------------------------------------//
//

CQSCRIPTPROGRAM(TM5_Ghelen3BlackHole, TM5_Ghelen3BlackHole_Save,0);

void TM5_Ghelen3BlackHole::Initialize (U32 eventFlags, const MPartRef & part)
{	
	data.ScoutSent = false;

	timer = 0;
	data.TurnOff = false;
	data.BlackwellFirst = false;

}

bool TM5_Ghelen3BlackHole::Update (void)
{
	if(data.mission_over == true)
	{
		return false;
	}

	MPartRef Deader = M_ GetFirstPart();

	timer -= ELAPSED_TIME;

	if(data.TurnOff)
	{
		return false;
	}

	while (Deader.isValid())
	{
	
		if(!data.ScoutSent && !M_ IsStreamPlaying(data.mhandle) && Deader->systemID == GHELEN3_SYSTEM && Deader->playerID == PLAYER_ID && !M_ IsPlatform(Deader))
		{
			if(Deader == data.Blackwell)
			{
				data.BlackwellFirst = true;
				data.HoleFound = true;

				M_ RunProgramByName("TM5_KillOffBlackwell", MPartRef());
				M_ EnableJumpgate(data.LuxtoGhelenA, false);
			}

			if(!data.BlackwellFirst && data.Blackwell->systemID != GHELEN3_SYSTEM)
			{
				data.mhandle = M_ PlayAnimatedMessage("M05BL13.wav", "Animate!!Blackwell2", MagpieLeft, MagpieTop, data.Blackwell, IDS_TM5_SUB_M05BL13);
				M_ PlayMusic(SUSPENSE);

				M_ OrderMoveTo(data.Blackwell, M_ GetPartByName("BlackholeWP"));
				M_ RunProgramByName("TM5_KillOffBlackwell", MPartRef());
				
				data.ScoutSent = true;
				data.HoleFound = true;
			}

		}

		if(Deader->systemID == GHELEN3_SYSTEM && Deader->playerID == PLAYER_ID && !M_ IsPlatform(Deader))
		{
			M_ DestroyPart(Deader);

			if(Deader == data.Hawkes)
			{
				return false;
			}
		}

		if(data.ScoutSent && Deader != data.Blackwell && M_ DistanceTo(Deader, data.LuxtoGhelenA) <= 5 && Deader->playerID == PLAYER_ID && Deader->systemID == LUXOR_SYSTEM && Deader != data.BlackholeAlert && Deader != data.Goodhole)
		{
			M_ EnableJumpCap(Deader, false);

			if(!BlackwellInsists && !M_ IsStreamPlaying(data.mhandle) && !data.BlackwellFirst)
			{
				data.mhandle = M_ PlayAnimatedMessage("M05BL14.wav", "Animate!!Blackwell2", MagpieLeft, MagpieTop, data.Blackwell,IDS_TM5_SUB_M05BL14);

				BlackwellInsists = true;
			}

			timer = 10;

			if(timer < 0 && data.Blackwell.isValid())
			{
				M_ RunProgramByName("TM5_RepeatBlackwellsMission", MPartRef());
				timer = 180;
			}

		}
		else
		{
			M_ EnableJumpCap(Deader, true);
		}



		Deader = M_ GetNextPart(Deader);
	}


	return true;

}

//--------------------------------------------------------------------------//
//

CQSCRIPTPROGRAM(TM5_RepeatBlackwellsMission, TM5_RepeatBlackwellsMission_Save,0);

void TM5_RepeatBlackwellsMission::Initialize (U32 eventFlags, const MPartRef & part)
{	
}

bool TM5_RepeatBlackwellsMission::Update (void)
{
	if(data.mission_over == true)
	{
		return false;
	}

	if(!M_ IsStreamPlaying(data.mhandle))
	{
		M_ FlushStreams();
		data.mhandle = M_ PlayAnimatedMessage("M05BL15.wav", "Animate!!Blackwell2", MagpieLeft, MagpieTop, data.Blackwell, IDS_TM5_SUB_M05BL15);

		return false;
	}

	return true;

}

//--------------------------------------------------------------------------//
//

CQSCRIPTPROGRAM(TM5_KillOffBlackwell, TM5_KillOffBlackwell_Save,0);

void TM5_KillOffBlackwell::Initialize (U32 eventFlags, const MPartRef & part)
{
	M_ EnableSelection(data.Blackwell, false);

	state = Begin;
}

bool TM5_KillOffBlackwell::Update (void)
{
	if(data.mission_over == true)
	{
		return false;
	}

	switch(state)
	{
	case Begin:
		if(data.BlackwellFirst || data.Blackwell->systemID == GHELEN3_SYSTEM)
		{
			state = Blackwell;
		}
		if(M_ IsIdle(data.Blackwell) && !M_ IsStreamPlaying(data.mhandle))
		{
			data.mhandle = M_ PlayAnimatedMessage("M05BL20.wav", "Animate!!Blackwell2", MagpieLeft, MagpieTop, data.Blackwell, IDS_TM5_SUB_M05BL20);
			M_ OrderMoveTo(data.Blackwell, data.BlackHole, true);

			M_ PlayMusic(DANGER);
			state = Blackwell;
		}

		break;

	case Hawkes:
		if(!M_ IsStreamPlaying(data.mhandle))
		{
/*			if(!data.Elite1.isValid() || !data.Elite2.isValid())
			{
				data.mhandle = M_ PlayAudio("M05HW07.wav", data.Hawkes);

				state = Blackwell;
			}
			else
			{
*/				state = Blackwell;
//			}
		}
		break;

	case Blackwell:
		if(!M_ IsStreamPlaying(data.mhandle))
		{
			M_ MarkObjectiveCompleted(IDS_TM5_OBJECTIVE5);

			M_ EnableJumpgate(data.LuxtoGhelenA, false);

			data.mhandle = M_ PlayAnimatedMessage("M05BL21.wav", "Animate!!Blackwell2", MagpieLeft, MagpieTop, IDS_TM5_SUB_M05BL21);

			state = Done;
		}
		break;

	case Done:
		if(!M_ IsStreamPlaying(data.mhandle))
		{
			data.mhandle = M_ PlayAnimatedMessage("M05HW22.wav", "Animate!!Hawkes2", MagpieLeft, MagpieTop, data.Hawkes, IDS_TM5_SUB_M05HW22);
		
			M_ RemovePart(data.Blackwell);

			M_ RunProgramByName("TM5_IntroduceKerTak", MPartRef ());
			data.TurnOff = true;

			return false;
		}
		break;
	}

	return true;
}

//--------------------------------------------------------------------------//
//

CQSCRIPTPROGRAM(TM5_IntroduceKerTak, TM5_IntroduceKerTak_Save,0);

void TM5_IntroduceKerTak::Initialize (U32 eventFlags, const MPartRef & part)
{
	data.KerTak = M_ CreatePart("FSHIP!!M_KerTak", M_ GetPartByName("KerTak Create"), PLAYER_ID, "Ker'Tak"); 
	M_ EnablePartnameDisplay(data.KerTak, true);

	M_ EnableSelection(data.KerTak, false);

	KerTakWaypoint = M_ GetPartByName("KerTak Waypoint");
	M_ OrderMoveTo(data.KerTak, KerTakWaypoint);

	M_ AlertMessage(data.KerTak, 0);

	state = Begin;
}

bool TM5_IntroduceKerTak::Update (void)
{
	if(data.mission_over == true)
	{
		return false;
	}

	switch(state)
	{
		case Begin:
			if(M_ IsIdle(data.KerTak) && !M_ IsStreamPlaying(data.mhandle))
			{
				data.mhandle = M_ PlayAnimatedMessage("M05KR23.wav", "Animate!!KerTak2", MagpieLeft, MagpieTop, data.KerTak, IDS_TM5_SUB_M05KR23);
				state = Hawkes;
			}
			break;

		case Hawkes:
			if(!M_ IsStreamPlaying(data.mhandle))
			{
				data.mhandle = M_ PlayAnimatedMessage("M05HW24.wav", "Animate!!Hawkes2", MagpieLeft, MagpieTop, data.Hawkes, IDS_TM5_SUB_M05HW24);

				state = Kertak;
			}
			break;

		case Kertak:
			if(!M_ IsStreamPlaying(data.mhandle))
			{
				data.mhandle = M_ PlayAnimatedMessage("M05KR25.wav", "Animate!!KerTak2", MagpieLeft, MagpieTop, data.KerTak, IDS_TM5_SUB_M05KR25);

				state = Finish;
			}
			break;

		case Finish:
			if(!M_ IsStreamPlaying(data.mhandle))
			{
				data.mhandle = M_ PlayAnimatedMessage("M05HW26.wav", "Animate!!Hawkes2", MagpieLeft, MagpieTop, data.Hawkes, IDS_TM5_SUB_M05HW26);

				state = Trick;

			}
			break;

		case Trick:
			if(!M_ IsStreamPlaying(data.mhandle))
			{
				data.mhandle = M_ PlayAnimatedMessage("M05KR27.wav", "Animate!!KerTak2", MagpieLeft, MagpieTop, data.KerTak, IDS_TM5_SUB_M05KR27);

				state = Done;
			}

		case Done:
			if(!M_ IsStreamPlaying(data.mhandle))
			{
				data.mhandle = M_ PlayAnimatedMessage("M05HW28.wav", "Animate!!Hawkes2", MagpieLeft, MagpieTop, data.Hawkes, IDS_TM5_SUB_M05HW28);

				M_ RunProgramByName("TM5_ClearOutLuxor", MPartRef ());
				
				M_ MarkObjectiveCompleted(IDS_TM5_OBJECTIVE3);

				state = End;
			}

		case End:
			if(!M_ IsStreamPlaying(data.mhandle))
			{
				data.mhandle = TeletypeObjective(IDS_TM5_OBJECTIVE6);
				M_ AddToObjectiveList(IDS_TM5_OBJECTIVE6);

				return false;
			}
			break;
	}

	return true;

}
//--------------------------------------------------------------------------//
//

CQSCRIPTPROGRAM(TM5_ClearOutLuxor, TM5_ClearOutLuxor_Save,0);

void TM5_ClearOutLuxor::Initialize (U32 eventFlags, const MPartRef & part)
{
	timer = 0;

	KerTakWaypoint = M_ GetPartByName("KerTakMove");

	state = Begin;
}

bool TM5_ClearOutLuxor::Update (void)
{
	timer -= ELAPSED_TIME;

	if(data.mission_over == true)
	{
		return false;
	}

	switch(state)
	{
		case Begin:
			if(!M_ PlayerHasPlatformsInSystem(MANTIS_ID, LUXOR_SYSTEM) && !M_ PlayerHasPlatformsInSystem(MANTIS_ID, BEKKA_SYSTEM))
			{
		//		M_ SaveCameraPos();
		//		M_ ChangeCamera(M_ GetPartByName("New Wormhole Cam"), 3, MOVIE_CAMERA_SLIDE_TO | MOVIE_CAMERA_QUEUE);

				M_ ClearHardFog(data.LuxtoGhelenB, 1);

				state = Done;
				timer = 0;
			}
			break;

		case Done:
			if(timer <= 0 && !M_ IsStreamPlaying(data.mhandle))
			{
				M_ MarkObjectiveCompleted(IDS_TM5_OBJECTIVE6);

				M_ AlertMessage(data.Goodhole, 0);
				M_ OrderMoveTo(data.KerTak, KerTakWaypoint);

				data.mhandle = M_ PlayAnimatedMessage("M05KR29.wav", "Animate!!KerTak2", MagpieLeft, MagpieTop, data.KerTak, IDS_TM5_SUB_M05KR29);

				M_ RunProgramByName("TM5_MissionSuccess", MPartRef ());

				data.LuxCleared = true;

				return false;
			}
			break;
	}

	return true;
}

//--------------------------------------------------------------------------//
//	

CQSCRIPTPROGRAM(TM5_ObjectConstructed, TM5_ObjectConstructed_Save,CQPROGFLAG_OBJECTCONSTRUCTED);

void TM5_ObjectConstructed::Initialize (U32 eventFlags, const MPartRef & part)
{
	if(data.mission_over)
	{
		return;
	}

	if(part->playerID == PLAYER_ID && M_ IsPlatform(part) && !data.WavesSent)
	{
		M_ RunProgramByName("TM5_SendMantisWave1", MPartRef ());
		M_ RunProgramByName("TM5_SendMantisWave2", MPartRef ());

		data.WavesSent = true;
	}

	if(part->mObjClass == M_HANGER && !data.HangerBuilt)
	{
			if(!data.HangerBuilt)
			{
				data.mhandle = M_ PlayAnimatedMessage("M05BL08b.wav", "Animate!!Blackwell2", MagpieLeft, MagpieTop, data.Blackwell,IDS_TM5_SUB_M05BL08B);

				data.HangerBuilt = true;
			}

	}
}

bool TM5_ObjectConstructed::Update (void)
{
	return false;
}

//-----------------------------------------------------------------------
// OBJECT DESTRUCTION TRIGGER

CQSCRIPTPROGRAM(TM5_ObjectDestroyed, TM5_ObjectDestroyed_Save,CQPROGFLAG_OBJECTDESTROYED);

void TM5_ObjectDestroyed::Initialize (U32 eventFlags, const MPartRef & part)
{
    int objectCount;

	if(data.mission_over)
	{
		return;
	}

	switch (part->mObjClass)
	{
			//Check for destruction of mission critical objects...
		case M_CORVETTE:

			if (part == data.Blackwell && data.Blackwell->systemID != GHELEN3_SYSTEM)
			{
        	    MScript::MoveCamera( part, 0, MOVIE_CAMERA_JUMP_TO);

				M_ RunProgramByName("TM5_BlackwellKilled", MPartRef ());
			}

			break;

		case M_MISSILECRUISER:

			if(part == data.Hawkes && data.Hawkes->systemID == GHELEN3_SYSTEM)
			{
				data.mhandle = M_ PlayAnimatedMessage("M05BL12a.wav", "Animate!!Blackwell2", MagpieLeft, MagpieTop, data.Blackwell,IDS_TM5_SUB_M05BL12A);
        	    MScript::MoveCamera( data.LuxtoGhelenA, 0, MOVIE_CAMERA_JUMP_TO);

				M_ RunProgramByName("TM5_HawkesKilled", MPartRef ());
				data.BlackholeDeath = true;
			}
			else if(part == data.Hawkes)
			{
        	    MScript::MoveCamera( part, 0, MOVIE_CAMERA_JUMP_TO);

				M_ RunProgramByName("TM5_HawkesKilled", MPartRef ());
			}

			break;

		case M_FLAGSHIP:
			if(SameObj(part, data.KerTak))
			{
        	    MScript::MoveCamera( part, 0, MOVIE_CAMERA_JUMP_TO);

				M_ RunProgramByName("TM5_KerTakKilled", MPartRef ());
			}
			break;

		case M_HQ:

            objectCount = CountObjects( M_HQ, PLAYER_ID );

            if ( part->bReady ) 
            {
                objectCount--;
            }

			if ( !CountObjects(M_FABRICATOR, PLAYER_ID) && !objectCount )
			{
        	    MScript::MoveCamera( part, 0, MOVIE_CAMERA_JUMP_TO );

				M_ RunProgramByName("TM5_HQDestroyed", MPartRef ());
			}
			break;

		case M_FABRICATOR:

            objectCount = CountObjects( M_FABRICATOR, PLAYER_ID );

            if ( part->bReady ) 
            {
                objectCount--;
            }

			if ( !CountObjects(M_HQ, PLAYER_ID) &&  !objectCount )
			{
        	    MScript::MoveCamera( part, 0, MOVIE_CAMERA_JUMP_TO );

				M_ RunProgramByName("TM5_HQDestroyed", MPartRef ());
			}
			break;
	}
	
}

bool TM5_ObjectDestroyed::Update (void)
{
	return false;
}


//---------------------------------------------------------------------------
// MISSION FAILURE PROGRAMS

CQSCRIPTPROGRAM(TM5_BlackwellKilled, TM5_BlackwellKilled_Save, 0);

void TM5_BlackwellKilled::Initialize (U32 eventFlags, const MPartRef & part)
{
	data.mission_over = true;
	M_ MarkObjectiveFailed(IDS_TM5_OBJECTIVE4);

	M_ FlushStreams();
}

bool TM5_BlackwellKilled::Update (void)
{
	if (!M_ IsStreamPlaying(data.mhandle))
	{
		data.mhandle = M_ PlayAnimatedMessage("M05HW29a.wav", "Animate!!Hawkes2", MagpieLeft, MagpieTop, IDS_TM5_SUB_M05HW29A);

		data.failureID = IDS_TM5_FAIL_BLACKWELL_LOST;
		M_ RunProgramByName("TM5_MissionFailure", MPartRef ());
	
		return false;
	}

	return true;
}

CQSCRIPTPROGRAM(TM5_HawkesKilled, TM5_HawkesKilled_Save, 0);

void TM5_HawkesKilled::Initialize (U32 eventFlags, const MPartRef & part)
{
	data.mission_over = true;
	M_ MarkObjectiveFailed(IDS_TM5_OBJECTIVE4);

	M_ FlushStreams();

	if(data.Blackwell.isValid())
	{
		M_ OrderCancel(data.Blackwell);
	}

}

bool TM5_HawkesKilled::Update (void)
{

	if(data.BlackholeDeath)
	{
		data.mhandle = M_ PlayAnimatedMessage("M05BL12a.wav", "Animate!!Blackwell2", MagpieLeft, MagpieTop, IDS_TM5_SUB_M05BL12A);

		data.failureID = IDS_TM5_FAIL_HAWKS_LOST;
		M_ RunProgramByName("TM5_MissionFailure", MPartRef ());
	
		return false;
	}
	else
	{
		data.mhandle = M_ PlayAnimatedMessage("M05HA30.wav", "Animate!!Halsey2", MagpieLeft, MagpieTop, IDS_TM5_SUB_M05HA30);

		data.failureID = IDS_TM5_FAIL_HAWKS_LOST;
		M_ RunProgramByName("TM5_MissionFailure", MPartRef ());
	
		return false;
	}

	return true;
}

CQSCRIPTPROGRAM(TM5_KerTakKilled, TM5_KerTakKilled_Save, 0);

void TM5_KerTakKilled::Initialize (U32 eventFlags, const MPartRef & part)
{
	M_ FlushStreams();

	data.mission_over = true;
	data.mhandle = M_ PlayAnimatedMessage("M05HW31.wav", "Animate!!Hawkes2", MagpieLeft, MagpieTop, data.Hawkes, IDS_TM5_SUB_M05HW31);


}

bool TM5_KerTakKilled::Update (void)
{
	if (!M_ IsStreamPlaying(data.mhandle))
	{
		data.failureID = IDS_TM5_FAIL_KERTAK_LOST;

		M_ RunProgramByName("TM5_MissionFailure", MPartRef ());
	
		return false;
	}

	return true;
}

CQSCRIPTPROGRAM(TM5_HQDestroyed, TM5_HQDestroyed_Save, 0);

void TM5_HQDestroyed::Initialize (U32 eventFlags, const MPartRef & part)
{
	M_ FlushStreams();

	data.mission_over = true;
	
}

bool TM5_HQDestroyed::Update (void)
{
	if (!M_ IsTeletypePlaying(data.mhandle))
	{

		M_ RunProgramByName("TM5_MissionFailure", MPartRef ());
		return false;
	}

	return true;
}
//---------------------------------------------------------------------------
//

#define TELETYPE_TIME 3000

CQSCRIPTPROGRAM(TM5_MissionFailure, TM5_MissionFailure_Save, 0);

void TM5_MissionFailure::Initialize (U32 eventFlags, const MPartRef & part)
{
	M_ PlayMusic(DEFEAT);

	M_ EnableMovieMode(true);
//	M_ FlushStreams();

	UnderMissionControl();	
	data.mission_over = true;

	state = Begin;
}

bool TM5_MissionFailure::Update (void)
{
	switch (state)
	{
		case Begin:
			if(!M_ IsStreamPlaying(data.mhandle))
			{
				M_ FlushTeletype();
				data.shandle = M_ PlayAudio("high_level_data_uplink.wav");
				state = Halsey;
			}
			break;

		case Halsey:
			if(!M_ IsStreamPlaying(data.shandle))
			{
				if(data.BlackholeDeath)
				{
					data.mhandle = M_ PlayAnimatedMessage("M05HA30.wav", "Animate!!Halsey2", MagpieLeft, MagpieTop,IDS_TM5_SUB_M05HA30);
				}
				else
					data.mhandle = M_ PlayAnimatedMessage("M05HA32.wav", "Animate!!Halsey2", MagpieLeft, MagpieTop, IDS_TM5_SUB_M05HA32);

				state = Tele;
			}
			break;

		case Tele:
			if(!M_ IsStreamPlaying(data.mhandle))
			{
				data.mhandle = TeletypeMissionOver(IDS_TM5_TELETYPE_FAILURE,data.failureID);

				state = Done;
			}
			break;

		case Done:
			if(!M_ IsTeletypePlaying(data.mhandle))
			{

				UnderPlayerControl();
				M_ EndMissionDefeat();
				return false;
			}

	}
	return true;
}
//---------------------------------------------------------------------------


CQSCRIPTPROGRAM(TM5_MissionSuccess, TM5_MissionSuccess_Save,0);

void TM5_MissionSuccess::Initialize (U32 eventFlags, const MPartRef & part)
{
	data.mission_over = true;
	
	state = Begin;
}

bool TM5_MissionSuccess::Update (void)
{

	switch(state)
	{
		case Begin:
			if(!M_ IsStreamPlaying(data.mhandle) && !M_ PlayerHasPlatformsInSystem(MANTIS_ID, LUXOR_SYSTEM) && !M_ PlayerHasPlatformsInSystem(MANTIS_ID, BEKKA_SYSTEM))
			{
				M_ PlayMusic(VICTORY);


				M_ EnableMovieMode(true);

				UnderMissionControl();
				data.shandle = M_ PlayAudio("high_level_data_uplink.wav");
				state = HalseyTalk;
			}
			break;

		case HalseyTalk:
			if(!M_ IsStreamPlaying(data.shandle))
			{
				data.mhandle = M_ PlayAnimatedMessage("M05HA33.wav", "Animate!!Halsey2", MagpieLeft, MagpieTop, IDS_TM5_SUB_M05HA33);

				state = PrintSuccess;
			}
			break;

		case PrintSuccess:
			if(!M_ IsStreamPlaying(data.mhandle))
			{
				M_ FlushTeletype();
				data.mhandle = TeletypeMissionOver(IDS_TM5_TELETYPE_SUCCESS);
				state = Done;
			}
			break;

		case Done:
			if(!M_ IsTeletypePlaying(data.mhandle))
			{
				UnderPlayerControl();
				M_ EndMissionVictory(4);
				return false;
			}
	}

	return true;

}


//--------------------------------------------------------------------------//
//--------------------------------End Script05T.cpp-------------------------//
//--------------------------------------------------------------------------//
