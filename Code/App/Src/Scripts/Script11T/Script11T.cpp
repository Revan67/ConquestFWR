//--------------------------------------------------------------------------//
//                                                                          //
//                                Script11T.cpp                             //
//				/Conquest/App/Src/Scripts/Script11T/Script11T.cpp			//
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
#define TEXT_HOLD_TIME_INF  0
#define TIME_TO_TELETYPE	10
#define TELETYPE_TIME		3000

//--------------------------------------------------------------------------//
//  MISSION PROGRAM 														//
//--------------------------------------------------------------------------//
CQBRIEFINGITEM slotAnim;

CQSCRIPTDATA(MissionData, data);
//--------------------------------------------------------------------------//
//	OTHER PROGRAMS
//--------------------------------------------------------------------------//
void SetupAI(bool bOn)
{

	M_ EnableEnemyAI(MANTIS_ID, bOn, "MANTIS_FRIGATE_RUSH");
	M_ EnableEnemyAI(MANTIS_ID6, bOn, "MANTIS_SWARM");

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
		airules.nGunboatsPerSupplyShip = 10;
		airules.nNumMinelayers = 2;
		airules.uNumScouts = 2;
		airules.nNumFabricators = 2;
		airules.fHarvestersPerRefinery = 1;
		airules.nMaxHarvesters = 5;
		airules.nPanicThreshold = 2000;

		M_ SetEnemyAIRules(MANTIS_ID, airules);
		M_ SetEnemyAIRules(MANTIS_ID6, airules);
	}

}

void ToggleBuildUnits(bool bOn, U32 TeamID)
{

	AIPersonality airules;

	airules.difficulty = MEDIUM;
	airules.buildMask.bBuildPlatforms = true;
	airules.buildMask.bBuildLightGunboats = bOn;
	airules.buildMask.bBuildMediumGunboats = bOn;
	airules.buildMask.bBuildHeavyGunboats = bOn;
	airules.buildMask.bHarvest = true;
	airules.buildMask.bScout = true;
	airules.buildMask.bBuildAdmirals = false;
	airules.buildMask.bLaunchOffensives = true;
	airules.nGunboatsPerSupplyShip = 10;
	airules.nNumMinelayers = 3;
	airules.uNumScouts = 3;
	airules.nNumFabricators = 2;
	airules.fHarvestersPerRefinery = 2;
	airules.nMaxHarvesters = 5;
	airules.nPanicThreshold = 2000;

	M_ SetEnemyAIRules(TeamID, airules);
}


bool SameObj(const MPartRef & obj1, const MPartRef & obj2)
{
	if (!obj1.isValid() || !obj2.isValid())
		return false;

	if (obj1->dwMissionID == obj2->dwMissionID)
		return true;
	else
		return false;
}

//--------------------------------------------------------------------------
//	MISSION BRIEFING
//--------------------------------------------------------------------------

CQSCRIPTPROGRAM(TM11_Briefing, TM11_Briefing_Save,CQPROGFLAG_STARTBRIEFING);

void TM11_Briefing::Initialize (U32 eventFlags, const MPartRef & part)
{
	data.failureID = 0;
	data.mhandle = 0;
	data.shandle = 0;
	data.mission_state = Briefing;

	state = Begin;

	M_ PlayMusic(NULL);
	M_ EnableBriefingControls(true);

}

bool TM11_Briefing::Update (void)
{
	if (data.briefing_over)
		return false;

	switch(state)
	{
		case Begin:

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

			M_ FlushTeletype();

			data.mhandle = TeletypeBriefing(IDS_TM11_TELETYPE_LOCATION, false);

			state = Radio;

			break;

		case Radio:

			if (!M_ IsStreamPlaying(data.mhandle))
			{
				data.mhandle = M_ PlayAudio("prolog11.wav");
				MScript::BriefingSubtitle(data.mhandle,IDS_TM11_SUB_PROLOG11);
				state = Uplink;
			}

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

				state = Radio2;
			}
			break;

		case Radio2:

			if(M_ IsCustomBriefingAnimationDone(0))
			{
				strcpy(slotAnim.szFileName, "m11ha18.wav");
				strcpy(slotAnim.szTypeName, "Animate!!Halsey");
				slotAnim.slotID = 0;
				slotAnim.bHighlite = false;
				data.mhandle = M_ PlayBriefingTalkingHead(slotAnim);
				MScript::BriefingSubtitle(data.mhandle,IDS_TM11_SUB_M11HA18);

//				M_ FlushTeletype();
//				data.mhandle = M_ PlayBriefingTeletype(IDS_TM11_HALSEY_M11HA02, MOTextColor, 6000, 1000, false);
				state = MO;
			}

			break;

		case MO:
			
			if (!M_ IsStreamPlaying(data.mhandle))
			{
				M_ FlushTeletype();

				data.mhandle = TeletypeBriefing(IDS_TM11_OBJECTIVES, true);
				state = Finish;
			}

			break;
			
		case Finish:
			if (!M_ IsTeletypePlaying(data.mhandle))
				{
					M_ RunProgramByName("TM11_BriefingSkip",MPartRef ());
					return false;
				}
			
			break;

	
	}

	return true;
}

//--------------------------------------------------------------------------//

CQSCRIPTPROGRAM(TM11_InGameScene, TM11_InGameScene_Save, CQPROGFLAG_STARTMISSION);

void TM11_InGameScene::Initialize (U32 eventFlags, const MPartRef & part)
{
	SceneSmirnoff = M_ CreatePart("CHAR!!Smirnoff_D_ND", M_ GetPartByName("SmirnoffSpawnWP"), PLAYER_ID, "Smirnoff");

	M_ EnableAttackCap(SceneSmirnoff, false);
	M_ ClearHardFog(M_ GetPartByName("SmirnoffMovePoint"), 4);

	M_ SetAllies(REBEL_ID, MANTIS_ID, true);
	M_ SetAllies(MANTIS_ID, REBEL_ID, true);
	M_ SetAllies(MANTIS_ID6, MANTIS_ID, true);
	M_ SetAllies(MANTIS_ID, MANTIS_ID6, true);
}

bool TM11_InGameScene::Update (void)
{
	timer -= ELAPSED_TIME;

	switch(state)
	{
		case Begin:
				UnderMissionControl();

				M_ EnableMovieMode(true);
				M_ OrderMoveTo(SceneSmirnoff, M_ GetPartByName("SmirnoffMovePoint"));

				state = Smirnoff;
			break;

		case Smirnoff:
				M_ PlayMusic(ESCORT);
				M_ MoveCamera(M_ GetPartByName("CameraPoint"), 0, 0);
//				data.mhandle = M_ PlayTeletype(IDS_TM11_SMIRNOFF_M11SM02, SuperLeft, SuperTop, SuperRight, SuperBottom, MOTextColor, 4000, 1000, false);
				data.mhandle = M_ PlayAnimatedMessage("M11SM02.wav", "Animate!!Smirnoff2", MagpieLeft, MagpieTop, IDS_TM11_SUB_M11SM02);

				state = Hawkes;
			break;

		case Hawkes:
				if(!M_ IsStreamPlaying(data.mhandle))
				{
//					data.mhandle = M_ PlayTeletype(IDS_TM11_HAWKES_M11HW03, SuperLeft, SuperTop, SuperRight, SuperBottom, MOTextColor, 3000, 1000, false);
					data.mhandle = M_ PlayAnimatedMessage("M11HW03.wav", "Animate!!Hawkes2", MagpieLeft, MagpieTop, IDS_TM11_SUB_M11HW03);

					state = Smirnoff2;
				}
			break;

		case Smirnoff2:
				if(!M_ IsStreamPlaying(data.mhandle))
				{
//					data.mhandle = M_ PlayTeletype(IDS_TM11_SMIRNOFF_M11SM04, SuperLeft, SuperTop, SuperRight, SuperBottom, MOTextColor, 4000, 1000, false);
					data.mhandle = M_ PlayAnimatedMessage("M11SM04.wav", "Animate!!Smirnoff2", MagpieLeft, MagpieTop, IDS_TM11_SUB_M11SM04);

					state = Hawkes2;
				}
			break;

		case Hawkes2:
				if(!M_ IsStreamPlaying(data.mhandle))
				{
//					data.mhandle = M_ PlayTeletype(IDS_TM11_HAWKES_M11HW05, SuperLeft, SuperTop, SuperRight, SuperBottom, MOTextColor, 4000, 1000, false);
					data.mhandle = M_ PlayAnimatedMessage("M11HW05.wav", "Animate!!Hawkes2", MagpieLeft, MagpieTop, IDS_TM11_SUB_M11HW05);

					state = Leech;
				}
			break;

		case Leech:
				if(!M_ IsStreamPlaying(data.mhandle))
				{
					SceneLeech = M_ CreatePart("TSHIP!!M_Leech", M_ GetPartByName("LeechSpawnWP"), MANTIS_ID, 0);
					M_ OrderMoveTo(SceneLeech, M_ GetPartByName("SmirnoffMovePoint"));

					state = Smirnoff3;
				}
			break;

		case Smirnoff3:
				if(!M_ IsStreamPlaying(data.mhandle))
				{
//					data.mhandle = M_ PlayTeletype(IDS_TM11_SMIRNOFF_M11SM06, SuperLeft, SuperTop, SuperRight, SuperBottom, MOTextColor, 7000, 2000, false);
					data.mhandle = M_ PlayAnimatedMessage("M11SM06.wav", "Animate!!Smirnoff2", MagpieLeft, MagpieTop, IDS_TM11_SUB_M11SM06);

					state = Officer;
				}
			break;

		case Officer:
				if(M_ DistanceTo(SceneLeech, SceneSmirnoff) <= 1)
				{
//					data.mhandle = M_ PlayTeletype(IDS_TM11_OFFICER_M11OF08, SuperLeft, SuperTop, SuperRight, SuperBottom, MOTextColor, 5000, 2000, false);
					data.mhandle = M_ PlayAudio("M11of08.wav",IDS_TM11_SUB_M11OF08);

					state = Smirnoff4;
				}
			break;

		case Smirnoff4:
				if(!M_ IsStreamPlaying(data.mhandle))
				{
	//				data.mhandle = M_ PlayTeletype(IDS_TM11_SMIRNOFF_M11SM09, SuperLeft, SuperTop, SuperRight, SuperBottom, MOTextColor, 5000, 2000, false);
					data.mhandle = M_ PlayAnimatedMessage("M11SM09.wav", "Animate!!Smirnoff2", MagpieLeft, MagpieTop, IDS_TM11_SUB_M11SM09);

					state = Malkor;
				}
			break;

		case Malkor:
				if(!M_ IsStreamPlaying(data.mhandle))
				{
//					data.mhandle = M_ PlayTeletype(IDS_TM11_MALKOR_M11MA10, SuperLeft, SuperTop, SuperRight, SuperBottom, MOTextColor, 4000, 1000, false);
					data.mhandle = M_ PlayAnimatedMessage("M11MA10.wav", "Animate!!Malkor2", MagpieLeft, MagpieTop, IDS_TM11_SUB_M11MA10);

					M_ PlayMusic(DANGER);

					state = Smirnoff5;
				}
			break;

		case Smirnoff5:
				if(!M_ IsStreamPlaying(data.mhandle))
				{
	//				data.mhandle = M_ PlayTeletype(IDS_TM11_SMIRNOFF_M11SM11, SuperLeft, SuperTop, SuperRight, SuperBottom, MOTextColor, 7000, 2000, false);
					data.mhandle = M_ PlayAnimatedMessage("M11SM11.wav", "Animate!!Smirnoff2", MagpieLeft, MagpieTop, IDS_TM11_SUB_M11SM11);

					state = Malkor2;
				}
			break;

		case Malkor2:
				if(!M_ IsStreamPlaying(data.mhandle))
				{
//					data.mhandle = M_ PlayTeletype(IDS_TM11_MALKOR_M11MA12, SuperLeft, SuperTop, SuperRight, SuperBottom, MOTextColor, 3000, 1000, false);
					data.mhandle = M_ PlayAnimatedMessage("M11MA12.wav", "Animate!!Malkor2", MagpieLeft, MagpieTop, IDS_TM11_SUB_M11MA12);

					state = Hawkes3;
				}
			break;

		case Hawkes3:
				if(!M_ IsStreamPlaying(data.mhandle))
				{
//					data.mhandle = M_ PlayTeletype(IDS_TM11_HAWKES_M11HW14, SuperLeft, SuperTop, SuperRight, SuperBottom, MOTextColor, 4000, 2000, false);
					data.mhandle = M_ PlayAnimatedMessage("M11HW13.wav", "Animate!!Hawkes2", MagpieLeft, MagpieTop, IDS_TM11_SUB_M11HW13);

					state = Smirnoff6;
				}
			break;

		case Smirnoff6:
				if(!M_ IsStreamPlaying(data.mhandle))
				{
	//				data.mhandle = M_ PlayTeletype(IDS_TM11_SMIRNOFF_M11SM14, SuperLeft, SuperTop, SuperRight, SuperBottom, MOTextColor, 6000, 2000, false);
					data.mhandle = M_ PlayAnimatedMessage("M11SM14.wav", "Animate!!Smirnoff2", MagpieLeft, MagpieTop, IDS_TM11_SUB_M11SM14);

					state = Hawkes4;
				}
			break;

		case Hawkes4:
				if(!M_ IsStreamPlaying(data.mhandle))
				{
//					data.mhandle = M_ PlayTeletype(IDS_TM11_HAWKES_M11HW15, SuperLeft, SuperTop, SuperRight, SuperBottom, MOTextColor, 4000, 2000, false);
					data.mhandle = M_ PlayAnimatedMessage("M11HW15.wav", "Animate!!Hawkes2", MagpieLeft, MagpieTop, IDS_TM11_SUB_M11HW15);

					state = Smirnoff7;
				}
			break;

		case Smirnoff7:
				if(!M_ IsStreamPlaying(data.mhandle))
				{
//					data.mhandle = M_ PlayTeletype(IDS_TM11_SMIRNOFF_M11SM16, SuperLeft, SuperTop, SuperRight, SuperBottom, MOTextColor, 6000, 2000, false);
					data.mhandle = M_ PlayAnimatedMessage("M11SM16.wav", "Animate!!Smirnoff2", MagpieLeft, MagpieTop, IDS_TM11_SUB_M11SM16);

					M_ OrderMoveTo(SceneLeech, M_ GetPartByName("LeechLeave"));

					SceneBlackwell = M_ CreatePart("CHAR!!Blackwell_Chrome", M_ GetPartByName("BlackwellSpawnWP"), PLAYER_ID, "Blackwell");
					M_ EnableAttackCap(SceneBlackwell, false);
					M_ OrderSpecialAbility(SceneBlackwell);

					M_ OrderMoveTo(SceneBlackwell, M_ GetPartByName("BlackwellMovePoint"));
					M_ PlayMusic(DETERMINATION);

					state = Blackwell;
				}
			break;

		case Blackwell:
				if(M_ IsIdle(SceneBlackwell) && !M_ IsStreamPlaying(data.mhandle))
				{
//					data.mhandle = M_ PlayTeletype(IDS_TM11_BLACKWELL_M11BL17, SuperLeft, SuperTop, SuperRight, SuperBottom, MOTextColor, 5000, 2000, false);
					data.mhandle = M_ PlayAnimatedMessage("M11BL17.wav", "Animate!!Blackwell2", MagpieLeft, MagpieTop, data.Blackwell, IDS_TM11_SUB_M11BL17);
					M_ OrderSpecialAbility(SceneBlackwell);

					state = FireDestab;
				}
			break;

		case FireDestab:
				if(!M_ IsStreamPlaying(data.mhandle))
				{
					M_ OrderSpecialAttack(SceneBlackwell, SceneSmirnoff);//Fire Destab

					state = Fire;
				}
			break;

		case Fire:
				if(SceneBlackwell->supplies < SceneBlackwell->supplyPointsMax)
				{
					timer = 8;

					state = HawkesFree;
				}
				break;

		case HawkesFree:
				if(timer < 0)
				{
					SceneHawkes = M_ CreatePart("FSHIP!!T_Hawkes", SceneSmirnoff, PLAYER_ID, 0);

					M_ OrderMoveTo(SceneHawkes, SceneBlackwell);

					timer = 2;
					state = BlowupSmirnoff;
				}
				break;

		case BlowupSmirnoff:
				if(timer < 0)
				{
					M_ DestroyPart(SceneSmirnoff);

					state = FlyAway;
				}
				break;

		case FlyAway:
				if(M_ IsIdle(SceneHawkes))
				{
					M_ RemovePart(SceneHawkes);

					M_ OrderSpecialAbility(SceneBlackwell);
					M_ OrderMoveTo(SceneBlackwell, M_ GetPartByName("BlackwellLeave"));

					timer = 5;
					state = End;
				}
				break;


		case End:
				if(timer < 0)
				{
					M_ RunProgramByName("TM11_MissionStart", MPartRef());

					M_ EnableMovieMode(false);

					M_ RemovePart(SceneBlackwell);
					M_ RemovePart(SceneLeech);
					M_ PlayMusic(TERRAN_WAV);

					UnderPlayerControl();

					return false;
				}
				break;
	}
	return true;
}


//--------------------------------------------------------------------------

CQSCRIPTPROGRAM(TM11_MissionStart, TM11_MissionStart_Save,0);

void TM11_MissionStart::Initialize (U32 eventFlags, const MPartRef & part)
{
	data.mission_state = MissionStart;
	state = Begin;

	TECHNODE missionTech;

	//Set up player Tech lvls

	missionTech.InitLevel( TECHTREE::FULL_TREE );

	missionTech.race[0].build = missionTech.race[1].build = (TECHTREE::BUILDNODE) (
		TECHTREE::RES_REFINERY_GAS1 |
		TECHTREE::RES_REFINERY_GAS2 |
		TECHTREE::RES_REFINERY_METAL1 |
		TECHTREE::RES_REFINERY_METAL2 |
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
		TECHTREE::TDEPEND_SPACE_STATION |
		TECHTREE::TDEPEND_PROPLAB  |
		TECHTREE::TDEPEND_AWSLAB  |
		TECHTREE::TDEPEND_DISPLACEMENT );

    missionTech.race[1].build = (TECHTREE::BUILDNODE) ( missionTech.race[1].build |
		TECHTREE::MDEPEND_PLANTATION |
		TECHTREE::MDEPEND_BIOFORGE 
        );

	missionTech.race[2].build = 
        (TECHTREE::BUILDNODE) ( ( TECHTREE::ALL_BUILDNODE ^ 
        TECHTREE::SDEPEND_PORTAL ) | 
        TECH_TREE_RACE_BITS_ALL );

    // add in necessary upgrades, and troopship

	missionTech.race[0].tech = missionTech.race[1].tech = missionTech.race[2].tech = (TECHTREE::TECHUPGRADE) (
        TECHTREE::T_SHIP__FABRICATOR | 
        TECHTREE::T_SHIP__HARVEST |
		TECHTREE::T_SHIP__SUPPLY |
        TECHTREE::T_SHIP__CORVETTE | 
        TECHTREE::T_SHIP__MISSILECRUISER |
		TECHTREE::T_SHIP__BATTLESHIP |
		TECHTREE::T_SHIP__TROOPSHIP |
  		TECHTREE::T_SHIP__CARRIER  |
		TECHTREE::T_SHIP__INFILTRATOR  |
		TECHTREE::T_SHIP__LANCER   |
		TECHTREE::T_SHIP__DREADNOUGHT   |
		TECHTREE::T_RES_TROOPSHIP1 |
		TECHTREE::T_RES_TROOPSHIP2 |
		TECHTREE::T_RES_MISSLEPACK1 |
		TECHTREE::T_RES_MISSLEPACK2 |
		TECHTREE::T_RES_XCHARGES |
		TECHTREE::T_RES_XPROBE  |
		TECHTREE::T_RES_XCLOAK  |
		TECHTREE::T_RES_XVAMPIRE 
        );

    missionTech.race[1].tech = (TECHTREE::TECHUPGRADE) ( missionTech.race[1].tech |
		TECHTREE::M_SHIP_KHAMIR );

    missionTech.race[2].tech = (TECHTREE::TECHUPGRADE) ( missionTech.race[2].tech |
		TECHTREE::S_RES_LEGION2 |
		TECHTREE::S_SHIP_MONOLITH |
		TECHTREE::S_RES_MASSDISRUPTOR );

	missionTech.race[2].tech = (TECHTREE::TECHUPGRADE) ( (
		missionTech.race[2].tech ^
		TECHTREE::S_RES_TRACTOR ) |
        TECH_TREE_RACE_BITS_ALL
		);

	missionTech.race[0].common_extra = missionTech.race[1].common_extra = missionTech.race[2].common_extra = (TECHTREE::COMMON_EXTRA) (
        TECHTREE::RES_SENSORS1  |
        TECHTREE::RES_SENSORS2  |
		TECHTREE::RES_TANKER1  |
		TECHTREE::RES_TANKER2  |
		TECHTREE::RES_TENDER1 |
		TECHTREE::RES_TENDER2 |
		TECHTREE::RES_FIGHTER1 |
		TECHTREE::RES_FIGHTER2 |
		TECHTREE::RES_FIGHTER3
        );

	missionTech.race[0].common = missionTech.race[1].common = missionTech.race[2].common = (TECHTREE::COMMON) (
		TECHTREE::RES_WEAPONS1  |
		TECHTREE::RES_WEAPONS2  |
		TECHTREE::RES_SUPPLY1 |
		TECHTREE::RES_SUPPLY2  |
		TECHTREE::RES_SUPPLY3  |
		TECHTREE::RES_SUPPLY4  |
		TECHTREE::RES_HULL1  |
		TECHTREE::RES_HULL2  |
		TECHTREE::RES_HULL3  |
		TECHTREE::RES_HULL4  |
		TECHTREE::RES_ENGINE1 |
		TECHTREE::RES_ENGINE2 |
		TECHTREE::RES_ENGINE3 |
		TECHTREE::RES_SHIELDS1 |
		TECHTREE::RES_SHIELDS2
        );

    MScript::SetAvailiableTech( missionTech );

	SetupAI(true);
	MassEnableAI(MANTIS_ID, false, 0);
	MassEnableAI(MANTIS_ID6, false, 0);

	data.WaveSent = data.AttackSent = data.HurtPlatSeen = data.HomeFree = false;

	data.PaseoGate = M_ GetPartByName("1Gate");
	data.TariffGate = M_ GetPartByName("2Gate");
	data.EpsilonGate = M_ GetPartByName("EpsilonGate");
	data.VarnGate = M_ GetPartByName("VarnGate");
	data.KalidonGate = M_ GetPartByName("KalGate");
	data.SendGate = M_ GetPartByName("SendlorGate");
	data.CorlarGate = M_ GetPartByName("CorlarGate");

	data.Kal1 = M_ GetPartByName("Gate9");
	data.Kal2 = M_ GetPartByName("Gate14"); 
	data.Kal3 = M_ GetPartByName("Gate16"); 

    M_ MoveCamera(M_ GetPartByName("Acropolis"), 0, MOVIE_CAMERA_JUMP_TO);

  //Set up all the derelict(disabled) Platforms 
	data.dHQ = M_ GetPartByName("HQ");
	data.dRepair = M_ GetPartByName("Repair");
	data.dRefinery = M_ GetPartByName("Refinery");
	data.dLight = M_ GetPartByName("Light");
	data.dSensor = M_ GetPartByName("Sensor");
	data.dHeavy = M_ GetPartByName("Heavy");
	data.dAcademy = M_ GetPartByName("Academy");

	M_ EnableSelection(data.dHQ, false);
	M_ EnableSelection(data.dRepair, false);
	M_ EnableSelection(data.dRefinery, false);
	M_ EnableSelection(data.dLight, false);
	M_ EnableSelection(data.dSensor, false);
	M_ EnableSelection(data.dHeavy, false);
	M_ EnableSelection(data.dAcademy, false);

	M_ SetHullPoints(data.dHQ, 50);
	M_ SetHullPoints(data.dRepair, 50);
	M_ SetHullPoints(data.dRefinery, 50);
	M_ SetHullPoints(data.dLight, 50);
	M_ SetHullPoints(data.dSensor, 50);
	M_ SetHullPoints(data.dHeavy, 50);
	M_ SetHullPoints(data.dAcademy, 50);
  //All done!
	
	data.Vivac = M_ GetPartByName("Vivac");
	M_ EnablePartnameDisplay(data.Vivac, true);
	data.Steele = M_ GetPartByName("Steele");
	M_ EnablePartnameDisplay(data.Steele, true);
	data.Takei = M_ GetPartByName("Takei");
	M_ EnablePartnameDisplay(data.Takei, true);

	M_ SetMissionName(IDS_TM11_MISSION_NAME);
	M_ SetMissionID(10);

	M_ SetEnemyCharacter(MANTIS_ID, IDS_TM11_MANTIS);
	M_ SetEnemyCharacter(MANTIS_ID6, IDS_TM11_MANTIS6);

	M_ EnableRegenMode(true);

	M_ SetMissionDescription(IDS_TM11_MISSION_DESC);

	M_ AddToObjectiveList(IDS_TM11_OBJECTIVE1);
	M_ AddToObjectiveList(IDS_TM11_OBJECTIVE2);
	M_ AddToObjectiveList(IDS_TM11_OBJECTIVE3);
//	M_ AddToObjectiveList(IDS_TM11_OBJECTIVE4);
	M_ AddToObjectiveList(IDS_TM11_OBJECTIVE5);

	M_ RunProgramByName("TM11_CaptureUnits", MPartRef ());
	
	MassAttackCap(REBEL_ID, false);


}

bool TM11_MissionStart::Update (void)
{
	if(data.mission_over == true)
	{
		return false;
	}

	if(M_ IsVisibleToPlayer(M_ GetJumpgateSibling(data.PaseoGate), PLAYER_ID) || M_ IsVisibleToPlayer(M_ GetJumpgateSibling(data.TariffGate), PLAYER_ID))
	{
		M_ FlushTeletype();
//		data.mhandle = M_ PlayTeletype(IDS_TM11_STEELE_M11ST03, SuperLeft, SuperTop, SuperRight, SuperBottom, MOTextColor, SuperHoldTime, TELETYPE_TIME, false);
		data.mhandle = M_ PlayAnimatedMessage("M11ST19.wav", "Animate!!Steele2", MagpieLeft, MagpieTop, data.Steele, IDS_TM11_SUB_M11ST19);

		M_ RunProgramByName("TM11_BlackwellsEntrance", MPartRef());

		return false;
	}

	return true;
}

//--------------------------------------------------------------------------//

CQSCRIPTPROGRAM(TM11_AIProgram, TM11_AIProgram_Save, 0);

void TM11_AIProgram::Initialize (U32 eventFlags, const MPartRef & part)
{
	
}

bool TM11_AIProgram::Update (void)
{
	if(data.mission_over == true)
	{
		return false;
	}

	//Set up cases in which the AI will turn off for Red Mantis
	if(M_ GetUsedCommandPoints(MANTIS_ID) > 80 || M_ GetUsedCommandPoints(PLAYER_ID) < 15)
	{
		ToggleBuildUnits(false, MANTIS_ID);

		return true;
	}

 //Set up cases in which the AI will turn back on for Red Mantis
	if(M_ GetUsedCommandPoints(MANTIS_ID) < 50 || M_ GetUsedCommandPoints(PLAYER_ID) > 65)
	{
		ToggleBuildUnits(true, MANTIS_ID);
	}

  //---------------------------------------------------

	//Set up cases in which the AI will turn off for Orange Mantis
	if(M_ GetUsedCommandPoints(MANTIS_ID6) > 80 || M_ GetUsedCommandPoints(PLAYER_ID) < 15)
	{
		ToggleBuildUnits(false, MANTIS_ID6);

		return true;
	}

 //Set up cases in which the AI will turn back on for Orange Mantis
	if(M_ GetUsedCommandPoints(MANTIS_ID6) < 50 || M_ GetUsedCommandPoints(PLAYER_ID) > 65)
	{
		ToggleBuildUnits(true, MANTIS_ID6);
	}

	return true;
}

//--------------------------------------------------------------------------//

CQSCRIPTPROGRAM(TM11_CaptureUnits, TM11_CaptureUnits_Save, 0);

void TM11_CaptureUnits::Initialize (U32 eventFlags, const MPartRef & part)
{
	FirstUnitRescued = false;

}

bool TM11_CaptureUnits::Update (void)
{
	if(data.mission_over == true)
	{
		M_ MarkObjectiveCompleted(IDS_TM11_OBJECTIVE7);

		return false;
	}

	MPartRef target = M_ GetFirstPart();


	if(!M_ PlayerHasUnits(REBEL_ID))
	{
	
		return false;
	}

	while(target.isValid())
	{
		if(target->playerID == PLAYER_ID && target->systemID == CORLAR && data.HomeFree == false)
		{
			MassSwitchSystem(REBEL_ID, PLAYER_ID, CORLAR);

			if(data.HomeFree == false)
			{
				M_ RunProgramByName("TM11_MissionPartTwoFinished", MPartRef());

				data.HomeFree = true;
			}

		}
	
		if(target->playerID == REBEL_ID && M_ IsVisibleToPlayer(target, PLAYER_ID))
		{
			if(!M_ IsPlatform(target))
			{
				M_ EnableAttackCap(target, true);
				M_ SwitchPlayerID(target, PLAYER_ID);

				if(!FirstUnitRescued)
				{
					FirstUnitRescued = true;
					M_ PlayAudio("M11RV28A.WAV", IDS_TM11_SUB_M11RV28A);
				}
			}
			else if(data.HurtPlatSeen || target->systemID == CORLAR)
			{
				M_ SwitchPlayerID(target, PLAYER_ID);
			}
		}
		target = M_ GetNextPart(target);
	}


	return true;
}

//--------------------------------------------------------------------------//

CQSCRIPTPROGRAM(TM11_BlackwellsEntrance, TM11_BlackwellsEntrance_Save, 0);

void TM11_BlackwellsEntrance::Initialize (U32 eventFlags, const MPartRef & part)
{
    BlackwellWP = M_ GetPartByName("BlackwellWP");
	HawkesWP = M_ GetPartByName("HawkesWP");

	state = Begin;
}

bool TM11_BlackwellsEntrance::Update (void)
{
	if(data.mission_over == true)
	{
		return false;
	}

	switch(state)
	{
		case Begin:
			if(!M_ IsStreamPlaying(data.mhandle))
			{
				M_ FlushTeletype();
//				data.mhandle = M_ PlayTeletype(IDS_TM11_TAKEI_M11TK04, SuperLeft, SuperTop, SuperRight, SuperBottom, MOTextColor, SuperHoldTime, TELETYPE_TIME, false);
				data.mhandle = M_ PlayAnimatedMessage("M11TK20.wav", "Animate!!Takei2", MagpieLeft, MagpieTop, data.Takei, IDS_TM11_SUB_M11TK20);
				
				state = OnScreen;
			}
			break;

		case OnScreen:
			if(!M_ IsStreamPlaying(data.mhandle))
			{
				M_ FlushTeletype();
//				data.mhandle = M_ PlayTeletype(IDS_TM11_STEELE_M11ST05, SuperLeft, SuperTop, SuperRight, SuperBottom, MOTextColor, 2000, 1000, false);
				data.mhandle = M_ PlayAnimatedMessage("M11ST21.wav", "Animate!!Steele2", MagpieLeft, MagpieTop, data.Steele, IDS_TM11_SUB_M11ST21);
				
				state = Blackwell;
			}
			break;

		case Blackwell:
			if(!M_ IsStreamPlaying(data.mhandle))
			{
				M_ FlushTeletype();
				
				data.Blackwell = M_ CreatePart("CHAR!!Blackwell_Chrome", BlackwellWP, PLAYER_ID, "Blackwell");
				M_ EnablePartnameDisplay(data.Blackwell, true);

//				data.mhandle = M_ PlayTeletype(IDS_TM11_BLACKWELL_M11BK06, SuperLeft, SuperTop, SuperRight, SuperBottom, MOTextColor, SuperHoldTime, TELETYPE_TIME, false);
				data.mhandle = M_ PlayAnimatedMessage("M11BL22.wav", "Animate!!Blackwell2", MagpieLeft, MagpieTop, data.Blackwell, IDS_TM11_SUB_M11BL22);

				data.Hawkes = M_ CreatePart("CHAR!!Hawkes_MC", HawkesWP, PLAYER_ID, "Hawkes");
				M_ EnablePartnameDisplay(data.Hawkes, true);

				M_ RunProgramByName("TM11_FirstTerranPlatSeen", MPartRef());

				state = Steele;
			}
			break;

		case Steele:
			if(!M_ IsStreamPlaying(data.mhandle))
			{
				M_ FlushTeletype();
//				data.mhandle = M_ PlayTeletype(IDS_TM11_STEELE_M11ST07, SuperLeft, SuperTop, SuperRight, SuperBottom, MOTextColor, SuperHoldTime, TELETYPE_TIME, false);
				data.mhandle = M_ PlayAnimatedMessage("M11ST23.wav", "Animate!!Steele2", MagpieLeft, MagpieTop, data.Steele, IDS_TM11_SUB_M11ST23);
				
				state = Blackwell2;
			}
			break;

		case Blackwell2:
			if(!M_ IsStreamPlaying(data.mhandle))
			{
				M_ FlushTeletype();
//				data.mhandle = M_ PlayTeletype(IDS_TM11_BLACKWELL_M11BK08, SuperLeft, SuperTop, SuperRight, SuperBottom, MOTextColor, SuperHoldTime, TELETYPE_TIME, false);
				data.mhandle = M_ PlayAnimatedMessage("M11BL24.wav", "Animate!!Blackwell2", MagpieLeft, MagpieTop, data.Blackwell, IDS_TM11_SUB_M11BL24);
				
				state = Steele2;
			}
			break;

		case Steele2:
			if(!M_ IsStreamPlaying(data.mhandle))
			{
				M_ FlushTeletype();
//				data.mhandle = M_ PlayTeletype(IDS_TM11_STEELE_M11ST09, SuperLeft, SuperTop, SuperRight, SuperBottom, MOTextColor, SuperHoldTime, TELETYPE_TIME, false);
				data.mhandle = M_ PlayAnimatedMessage("M11ST25.wav", "Animate!!Steele2", MagpieLeft, MagpieTop, data.Steele, IDS_TM11_SUB_M11ST25);
				
				state = BlackwellOut;
			}
			break;

		case BlackwellOut:
			if(!M_ IsStreamPlaying(data.mhandle))
			{
				M_ FlushTeletype();
//				data.mhandle = M_ PlayTeletype(IDS_TM11_BLACKWELL_M11BK12, SuperLeft, SuperTop, SuperRight, SuperBottom, MOTextColor, SuperHoldTime, TELETYPE_TIME, false);
				data.mhandle = M_ PlayAnimatedMessage("M11BL26.wav", "Animate!!Blackwell2", MagpieLeft, MagpieTop, data.Blackwell, IDS_TM11_SUB_M11BL26);
				
//				M_ SaveCameraPos();
//				M_ MoveCamera(data.Blackwell, 3, 0);
				M_ EnableSystem(GROG, true, true);
				M_ EnableSystem(CORLAR, true, true);

				M_ RunProgramByName("TM11_ViewBlackwell", MPartRef());

				state = NewMO;
			}
			break;

		case NewMO:
			if(!M_ IsStreamPlaying(data.mhandle))
			{
				M_ FlushTeletype();

				data.mhandle = TeletypeObjective(IDS_TM11_OBJECTIVE6);
				M_ AddToObjectiveList(IDS_TM11_OBJECTIVE6);
				
				state = Ob2;
			}
			break;

		case Ob2:
			if(!M_ IsTeletypePlaying(data.mhandle))
			{
				M_ FlushTeletype();

				data.mhandle = TeletypeObjective(IDS_TM11_OBJECTIVE7);
				M_ AddToObjectiveList(IDS_TM11_OBJECTIVE7);
				state = Ob3;
			}
			break;

		case Ob3:
			if(!M_ IsTeletypePlaying(data.mhandle))
			{
				M_ FlushTeletype();

				data.mhandle = TeletypeObjective(IDS_TM11_OBJECTIVE8);
				M_ AddToObjectiveList(IDS_TM11_OBJECTIVE8);
				state = Hawkes;
			}
			break;

		case Hawkes:
			if(!M_ IsTeletypePlaying(data.mhandle))
			{
				M_ FlushTeletype();
//				data.mhandle = M_ PlayTeletype(IDS_TM11_HAWKES_M11HW13, SuperLeft, SuperTop, SuperRight, SuperBottom, MOTextColor, SuperHoldTime, TELETYPE_TIME, false);
				data.mhandle = M_ PlayAnimatedMessage("M11HW27.wav", "Animate!!Hawkes2", MagpieLeft, MagpieTop, data.Hawkes, IDS_TM11_SUB_M11HW27);
				
				state = Done;
			}
			break;

		case Done:
			if(!M_ IsStreamPlaying(data.mhandle))
			{
				M_ FlushTeletype();
//				data.mhandle = M_ PlayTeletype(IDS_TM11_BLACKWELL_M11BK14, SuperLeft, SuperTop, SuperRight, SuperBottom, MOTextColor, SuperHoldTime, TELETYPE_TIME, false);
				data.mhandle = M_ PlayAnimatedMessage("M11BL28.wav", "Animate!!Blackwell2", MagpieLeft, MagpieTop, data.Blackwell, IDS_TM11_SUB_M11BL28);
				
//				M_ LoadCameraPos(3, 0);
		
				return false ;
			}
			break;
	}

	return true;	
}

//-----------------------------------------------------------------------
// 

CQSCRIPTPROGRAM(TM11_ViewBlackwell, TM11_ViewBlackwell_Save,0);

void TM11_ViewBlackwell::Initialize (U32 eventFlags, const MPartRef & part)
{
	state = Begin;

}

bool TM11_ViewBlackwell::Update (void)
{
	if(data.mission_over == true)
	{
		return false;
	}

	if(M_ GetCurrentSystem() == GROG)
	{
		M_ MoveCamera(data.Blackwell, 0, MOVIE_CAMERA_JUMP_TO);
		return false;
	}

	return true;
}

//-----------------------------------------------------------------------
// 

CQSCRIPTPROGRAM(TM11_SendAttack, TM11_SendAttack_Save,0);

void TM11_SendAttack::Initialize (U32 eventFlags, const MPartRef & part)
{


}

bool TM11_SendAttack::Update (void)
{
	if(data.mission_over == true)
	{
		return false;
	}

	if(data.WaveSent)
	{


		MPartRef target1;

		target1 = FindFirstPlat(PLAYER_ID, NATUS);
		
		MassAttack(M_TIAMAT, MANTIS_ID6, target1, PASEO);
		MassAttack(M_SCOUTCARRIER, MANTIS_ID6, target1, PASEO);

		MassAttack(M_SCARAB, MANTIS_ID6, target1, TARIFF);
		MassAttack(M_FRIGATE, MANTIS_ID6, target1, TARIFF);
		MassAttack(M_HIVECARRIER, MANTIS_ID6, target1, TARIFF);

		MassEnableAI(MANTIS_ID6, true, PASEO);
		MassEnableAI(MANTIS_ID6, true, TARIFF);
		MassEnableAI(MANTIS_ID6, true, ABELL);
		MassEnableAI(MANTIS_ID6, true, EPSILON);
		MassEnableAI(MANTIS_ID6, true, NATUS);

		M_ RunProgramByName("TM11_AIProgram", MPartRef());

		return false;

	}

	return true;
}

//-----------------------------------------------------------------------
// 

CQSCRIPTPROGRAM(TM11_FirstTerranPlatSeen, TM11_FirstTerranPlatSeen_Save,0);

void TM11_FirstTerranPlatSeen::Initialize (U32 eventFlags, const MPartRef & part)
{
	state = Begin;

}

bool TM11_FirstTerranPlatSeen::Update (void)
{
	if(data.mission_over == true)
	{
		return false;
	}

	if(state == Begin  && (M_ IsVisibleToPlayer(data.dRefinery, PLAYER_ID) || M_ IsVisibleToPlayer(data.dHQ, PLAYER_ID) || M_ IsVisibleToPlayer(data.dRepair, PLAYER_ID) || M_ IsVisibleToPlayer(data.dLight, PLAYER_ID) || M_ IsVisibleToPlayer(data.dSensor, PLAYER_ID) || M_ IsVisibleToPlayer(data.dHeavy, PLAYER_ID) || M_ IsVisibleToPlayer(data.dAcademy, PLAYER_ID)))
	{
		M_ FlushTeletype();
//		data.mhandle = M_ PlayTeletype(IDS_TM11_HAWKES_M11HW17, SuperLeft, SuperTop, SuperRight, SuperBottom, MOTextColor, SuperHoldTime, TELETYPE_TIME, false);			
		data.mhandle = M_ PlayAnimatedMessage("M11HW31.wav", "Animate!!Hawkes2", MagpieLeft, MagpieTop, IDS_TM11_SUB_M11HW31);

		M_ RunProgramByName("TM11_RepairedPlatform", MPartRef());

		data.HurtPlatSeen = true;
		state = Done;
	}

	if(!M_ IsStreamPlaying(data.mhandle) && state == Done)
	{
		M_ FlushTeletype();
//		data.mhandle = M_ PlayTeletype(IDS_TM11_BLACKWELL_M11BK18, SuperLeft, SuperTop, SuperRight, SuperBottom, MOTextColor, SuperHoldTime, TELETYPE_TIME, false);			
		data.mhandle = M_ PlayAnimatedMessage("M11BL32.wav", "Animate!!Blackwell2", MagpieLeft, MagpieTop, data.Blackwell, IDS_TM11_SUB_M11BL32);

		return false;
	}

	return true;
}

//-----------------------------------------------------------------------
// 

CQSCRIPTPROGRAM(TM11_RepairedPlatform, TM11_RepairedPlatform_Save,0);

void TM11_RepairedPlatform::Initialize (U32 eventFlags, const MPartRef & part)
{
	data.NumOfPlats = 0;

	M_ RunProgramByName("TM11_AttackPlatform", MPartRef());

}

bool TM11_RepairedPlatform::Update (void)
{
	if(data.mission_over == true)
	{
		return false;
	}

	if(data.NumOfPlats < 7)
	{
		MPartRef target = M_ GetFirstPart();

		while(target.isValid())
		{
			if(target->bUnselectable && target->hullPoints > 50)
			{
				M_ EnableSelection(target, true);
				data.PlatArray[data.NumOfPlats] = target;
				data.NumOfPlats++;
			}
			target = M_ GetNextPart(target);
		}
	}
	else 
		return false;

	return true;
}


//-----------------------------------------------------------------------
// 

CQSCRIPTPROGRAM(TM11_AttackPlatform, TM11_AttackPlatform_Save,0);

void TM11_AttackPlatform::Initialize (U32 eventFlags, const MPartRef & part)
{
	timer = 0;
}

bool TM11_AttackPlatform::Update (void)
{
	if(data.mission_over == true)
	{
		return false;
	}
	
	timer -= ELAPSED_TIME;

	if(timer < 0 && data.NumOfPlats > 0)
	{
		MPartRef Attacker = M_ GetFirstPart();

		while(Attacker.isValid())
		{
			if(Attacker->playerID == MANTIS_ID && Attacker->caps.moveOk && Attacker->caps.attackOk && M_ IsIdle(Attacker) && (Attacker->systemID == VARN || Attacker->systemID == GRAAN || Attacker->systemID == CENTAURUS))
			{
				U32 x = rand()%data.NumOfPlats;

				M_ OrderAttack(Attacker, data.PlatArray[x]);
				timer = 90;
				return true;			
			}

			Attacker = M_ GetNextPart(Attacker);
		}

	}

	return true;
}


//-----------------------------------------------------------------------
// 

CQSCRIPTPROGRAM(TM11_MissionPartOneFinished, TM11_MissionPartOneFinished_Save,0);

void TM11_MissionPartOneFinished::Initialize (U32 eventFlags, const MPartRef & part)
{
	state = Begin;

	GatesDone = UnitsGone = data.HQDone = AdmalSafe = false;

}

bool TM11_MissionPartOneFinished::Update (void)
{
	if(data.mission_over == true)
	{
		return false;
	}
	
	switch(state)
	{
		case Begin:
			if(!M_ IsTeletypePlaying(data.mhandle))
			{
				M_ FlushTeletype();
//				data.mhandle = M_ PlayTeletype(IDS_TM11_VIVAC_M11VV15, SuperLeft, SuperTop, SuperRight, SuperBottom, MOTextColor, SuperHoldTime, TELETYPE_TIME, false);
				data.mhandle = M_ PlayAnimatedMessage("M11vv29.wav", "Animate!!Vivac2", MagpieLeft, MagpieTop, data.Vivac, IDS_TM11_SUB_M11VV29);
				
				state = Steele;
			}
			break;

		case Steele:
			if(!M_ IsStreamPlaying(data.mhandle))
			{
				M_ FlushTeletype();
//				data.mhandle = M_ PlayTeletype(IDS_TM11_STEELE_M11ST16, SuperLeft, SuperTop, SuperRight, SuperBottom, MOTextColor, SuperHoldTime, TELETYPE_TIME, false);
				data.mhandle = M_ PlayAnimatedMessage("M11ST30.wav", "Animate!!Steele2", MagpieLeft, MagpieTop, data.Steele, IDS_TM11_SUB_M11ST30);

				state = Jumpgates;
			}
			break;

		case Jumpgates:
			if(!AdmalSafe && data.Takei->systemID == EPSILON && data.Vivac->systemID == EPSILON && data.Steele->systemID == EPSILON)
			{
				AdmalSafe = true;

				M_ MarkObjectiveCompleted(IDS_TM11_OBJECTIVE5);
			}

		 	if(!GatesDone && CountObjects(M_JUMPPLAT, PLAYER_ID, EPSILON) == 1 && CountObjects(M_JUMPPLAT, PLAYER_ID, NATUS) == 2 && CountObjects(M_JUMPPLAT, PLAYER_ID, TARIFF) == 2 && CountObjects(M_JUMPPLAT, PLAYER_ID, PASEO) == 3 && CountObjects(M_JUMPPLAT, PLAYER_ID, ABELL) == 2)
			{
				GatesDone = true;

				M_ MarkObjectiveCompleted(IDS_TM11_OBJECTIVE2);
			}

			if(!UnitsGone && !M_ PlayerHasUnits(MANTIS_ID6))
			{
				UnitsGone = true;

				M_ MarkObjectiveCompleted(IDS_TM11_OBJECTIVE1);
			}

			if(data.HQDone && UnitsGone && GatesDone && AdmalSafe)
			{
				M_ FlushTeletype();
				data.mhandle = TeletypeObjective(IDS_TM11_MISSION1_COMPLETE);

				MassEnableAI(MANTIS_ID6, false, PASEO);
				MassEnableAI(MANTIS_ID6, false, TARIFF);	
				MassEnableAI(MANTIS_ID6, false, ABELL);
				MassEnableAI(MANTIS_ID6, false, EPSILON);
				MassEnableAI(MANTIS_ID6, false, NATUS);	
				
				data.PartOneDone = true;

				return false;
			}

			break;

	}

	return true;
}

//-----------------------------------------------------------------------
// 

CQSCRIPTPROGRAM(TM11_MissionPartTwoFinished, TM11_MissionPartTwoFinished_Save,0);

void TM11_MissionPartTwoFinished::Initialize (U32 eventFlags, const MPartRef & part)
{
	state = Begin;

}

bool TM11_MissionPartTwoFinished::Update (void)
{
	if(data.mission_over == true)
	{
		return false;
	}

	switch(state)
	{
		case Begin:
			if(!M_ IsTeletypePlaying(data.mhandle))
			{
				M_ FlushTeletype();
//				data.mhandle = M_ PlayTeletype(IDS_TM11_BLACKWELL_M11BK19, SuperLeft, SuperTop, SuperRight, SuperBottom, MOTextColor, SuperHoldTime, TELETYPE_TIME, false);
				data.mhandle = M_ PlayAnimatedMessage("M11BL33.wav", "Animate!!Blackwell2", MagpieLeft, MagpieTop, data.Blackwell, IDS_TM11_SUB_M11BL33);
				
				state = EnterCorlar;
			}
			break;

		case EnterCorlar:
			if(data.Hawkes->systemID == CORLAR && data.Blackwell->systemID == CORLAR && !M_ IsStreamPlaying(data.mhandle))
			{
				M_ FlushTeletype();
				M_ MarkObjectiveCompleted(IDS_TM11_OBJECTIVE6);

				data.mhandle = TeletypeObjective(IDS_TM11_MISSION2_COMPLETE);
									
				data.PartTwoDone = true;

				MassAttackCap(PLAYER_ID, true, CORLAR);

				M_ RunProgramByName("TM11_MissionSuccess", MPartRef());

				return false;
			}
			break;
	}

	return true;
}

//-----------------------------------------------------------------------
// UNDER ATTACK EVENT

CQSCRIPTPROGRAM(TM11_AttackOnTerrans, TM11_AttackOnTerrans_Save, 0);

void TM11_AttackOnTerrans::Initialize (U32 eventFlags, const MPartRef & part)
{
	timer = 0;
}

bool TM11_AttackOnTerrans::Update (void)
{

	timer -= ELAPSED_TIME;

	if(data.mission_over)
		return false;

	if(timer < 0)
	{
		MPartRef target = M_ GetFirstPart();
		MPartRef Attacker;

		while(target.isValid())
		{
			if(target->caps.moveOk && target->caps.attackOk && target->playerID == MANTIS_ID && target->systemID == VARN)
			{
				Attacker = target;
			}

			if((target->systemID == SENDLOR || target->systemID == KALIDON) && !M_ IsPlatform(target) && target->playerID == PLAYER_ID)
			{
				M_ OrderAttack(Attacker, target);

				timer = 120;
				return true;			
			}
			target = M_ GetNextPart(target);
		}

	}

	return true;
}

//-----------------------------------------------------------------------
// 

CQSCRIPTPROGRAM(TM11_FabRunning, TM11_FabRunning_Save,0);

void TM11_FabRunning::Initialize (U32 eventFlags, const MPartRef & part)
{

}

bool TM11_FabRunning::Update (void)
{
	if(data.mission_over == true)
	{
		return false;
	}

	if(M_ IsVisibleToPlayer(data.Kal1, PLAYER_ID) || M_ IsVisibleToPlayer(data.Kal2, PLAYER_ID) || M_ IsVisibleToPlayer(data.Kal3, PLAYER_ID))
	{
		if(M_ IsVisibleToPlayer(data.Kal1, PLAYER_ID))
		{
			MPartRef Frig = M_ CreatePart("GBOAT!!M_Frigate", M_ GetPartByName("Frigate3"), MANTIS_ID, 0);
			MPartRef Fab = 	M_ CreatePart("FAB!!T_Fabricator", M_ GetPartByName("Fab3"), PLAYER_ID, 0);

			M_ OrderMoveTo(Fab, M_ GetPartByName("Run3"), false);
			M_ OrderAttack(Frig, Fab);
			
			return false;
		}
		else if(M_ IsVisibleToPlayer(data.Kal2, PLAYER_ID))
		{
			MPartRef Frig = M_ CreatePart("GBOAT!!M_Frigate", M_ GetPartByName("Frigate2"), MANTIS_ID, 0);
			MPartRef Fab = 	M_ CreatePart("FAB!!T_Fabricator", M_ GetPartByName("Fab2"), PLAYER_ID, 0);

			M_ OrderMoveTo(Fab, M_ GetPartByName("Run2"), false);
			M_ OrderAttack(Frig, Fab);

			return false;
		}
		else if(M_ IsVisibleToPlayer(data.Kal3, PLAYER_ID))
		{
			MPartRef Frig = M_ CreatePart("GBOAT!!M_Frigate", M_ GetPartByName("Frigate1"), MANTIS_ID, 0);
			MPartRef Fab = 	M_ CreatePart("FAB!!T_Fabricator", M_ GetPartByName("Fab1"), PLAYER_ID, 0);

			M_ OrderMoveTo(Fab, M_ GetPartByName("Run1"), false);
			M_ OrderAttack(Frig, Fab);

			return false;
		}

	}
	return true;
}

//-----------------------------------------------------------------------
// OBJECT DESTRUCTION TRIGGER

CQSCRIPTPROGRAM(TM11_ObjectDestroyed, TM11_ObjectDestroyed_Save,CQPROGFLAG_OBJECTDESTROYED);

void TM11_ObjectDestroyed::Initialize (U32 eventFlags, const MPartRef & part)
{
	if(data.mission_over)
	{
		return;
	}

	if((part == data.VarnGate || part == data.KalidonGate) && data.AttackSent == false)
	{
		M_ RunProgramByName("TM11_FabRunning", MPartRef());
		M_ RunProgramByName("TM11_AttackOnTerrans", MPartRef());
		data.AttackSent = true;
	}

	if((part == data.PaseoGate || part == data.TariffGate) && data.WaveSent == false)
	{
		M_ RunProgramByName("TM11_SendAttack", MPartRef());
		data.WaveSent = true;
	}

	if(part == data.EpsilonGate)
	{
		M_ RunProgramByName("TM11_MissionPartOneFinished", MPartRef());
	}


	if(part == data.SendGate)
	{
		MassEnableAI(MANTIS_ID, true, VARN);
		MassEnableAI(MANTIS_ID, true, GRAAN);
		MassEnableAI(MANTIS_ID, true, CENTAURUS);
		MassEnableAI(MANTIS_ID, true, LEMO);
		MassEnableAI(MANTIS_ID, true, AEON);
		MassEnableAI(MANTIS_ID, true, KRACUS);
		MassEnableAI(MANTIS_ID, true, ARCTURUS);
		MassEnableAI(MANTIS_ID, true, KALIDON);
		MassEnableAI(MANTIS_ID, true, SENDLOR);
	}

	switch (part->mObjClass)
	{
			
		case M_CORVETTE:

			if(part == data.Blackwell)
			{
				M_ FlushStreams();
        	    MScript::MoveCamera( part, 0, MOVIE_CAMERA_JUMP_TO);

//				data.mhandle = M_ PlayTeletype(IDS_TM11_HAWKES_M11HW24, SuperLeft, SuperTop, SuperRight, SuperBottom, MOTextColor, SuperHoldTime, TELETYPE_TIME, false);
				data.mhandle = M_ PlayAnimatedMessage("M11HW38.wav", "Animate!!Hawkes2", MagpieLeft, MagpieTop, IDS_TM11_SUB_M11HW38);

				data.failureID = IDS_TM11_FAIL_BLACKWELL_LOST;

				M_ RunProgramByName("TM11_MissionFailure", MPartRef());
			}

			break;

		case M_MISSILECRUISER:

			if(part == data.Hawkes)
			{
				M_ FlushStreams();
        	    MScript::MoveCamera( part, 0, MOVIE_CAMERA_JUMP_TO);

//				data.mhandle = M_ PlayTeletype(IDS_TM11_STEELE_M11ST20, SuperLeft, SuperTop, SuperRight, SuperBottom, MOTextColor, SuperHoldTime, TELETYPE_TIME, false);
				data.mhandle = M_ PlayAnimatedMessage("M11ST34.wav", "Animate!!Steele2", MagpieLeft, MagpieTop, data.Steele, IDS_TM11_SUB_M11ST34);

				data.failureID = IDS_TM11_FAIL_HAWKES_LOST;

				M_ RunProgramByName("TM11_MissionFailure", MPartRef());
			}

			break;

		case M_CARRIER:

			if(part == data.Takei)
			{
				M_ FlushStreams();
        	    MScript::MoveCamera( part, 0, MOVIE_CAMERA_JUMP_TO);

//				data.mhandle = M_ PlayTeletype(IDS_TM11_STEELE_M11ST20, SuperLeft, SuperTop, SuperRight, SuperBottom, MOTextColor, SuperHoldTime, TELETYPE_TIME, false);
				data.mhandle = M_ PlayAnimatedMessage("M11ST34.wav", "Animate!!Steele2", MagpieLeft, MagpieTop, data.Steele, IDS_TM11_SUB_M11ST34);

				data.failureID = IDS_TM11_FAIL_TAKEI_LOST;

				M_ RunProgramByName("TM11_MissionFailure", MPartRef());
			}

			break;

		case M_BATTLESHIP:

			if(part == data.Steele)
			{
				M_ FlushStreams();
        	    MScript::MoveCamera( part, 0, MOVIE_CAMERA_JUMP_TO);

//				data.mhandle = M_ PlayTeletype(IDS_TM11_TAKEI_M11TK21, SuperLeft, SuperTop, SuperRight, SuperBottom, MOTextColor, SuperHoldTime, TELETYPE_TIME, false);
				data.mhandle = M_ PlayAnimatedMessage("M11TK35.wav", "Animate!!Takei2", MagpieLeft, MagpieTop, data.Takei, IDS_TM11_SUB_M11TK35);

				data.failureID = IDS_TM11_FAIL_STEELE_LOST;

				M_ RunProgramByName("TM11_MissionFailure", MPartRef());
			}

			break;

		case M_MONOLITH:

			if(part == data.Vivac)
			{
				M_ FlushStreams();
        	    MScript::MoveCamera( part, 0, MOVIE_CAMERA_JUMP_TO);

//				data.mhandle = M_ PlayTeletype(IDS_TM11_STEELE_M11ST22, SuperLeft, SuperTop, SuperRight, SuperBottom, MOTextColor, SuperHoldTime, TELETYPE_TIME, false);
				data.mhandle = M_ PlayAnimatedMessage("M11ST36.wav", "Animate!!Steele2", MagpieLeft, MagpieTop, data.Steele, IDS_TM11_SUB_M11ST36);

				data.failureID = IDS_TM11_FAIL_VIVAC_LOST;

				M_ RunProgramByName("TM11_MissionFailure", MPartRef());
			}

			break;

	}
}

bool TM11_ObjectDestroyed::Update (void)
{
	return false;
}

//-----------------------------------------------------------------------
// OBJECT DESTRUCTION TRIGGER

CQSCRIPTPROGRAM(TM11_ObjectConstructed, TM11_ObjectConstructed_Save,CQPROGFLAG_OBJECTCONSTRUCTED);

void TM11_ObjectConstructed::Initialize (U32 eventFlags, const MPartRef & part)
{
	if(data.mission_over)
	{
		return;
	}

	if(part->systemID == EPSILON)
	{
		if(part->mObjClass == M_HQ || part->mObjClass == M_ACROPOLIS)
		{
			data.HQDone = true;
	
			M_ MarkObjectiveCompleted(IDS_TM11_OBJECTIVE3);
		}
	}
}

bool TM11_ObjectConstructed::Update (void)
{
	return false;
}

//-----------------------------------------------------------------------
// UNDER ATTACK EVENT

CQSCRIPTPROGRAM(TM11_UnderAttack, TM11_UnderAttack_Save, CQPROGFLAG_UNITHIT);

void TM11_UnderAttack::Initialize (U32 eventFlags, const MPartRef & part)
{
	if(data.mission_over)
	{
		return;
	}

	if((part == data.PaseoGate || part == data.TariffGate) && data.WaveSent == false)
	{

		M_ RunProgramByName("TM11_SendAttack", MPartRef());
		data.WaveSent = true;
	}

	if((part == data.VarnGate || part == data.KalidonGate) && data.AttackSent == false)
	{
		M_ RunProgramByName("TM11_FabRunning", MPartRef());
		M_ RunProgramByName("TM11_AttackOnTerrans", MPartRef());
		data.AttackSent = true;
	}


}

bool TM11_UnderAttack::Update (void)
{
	return false;
}


//---------------------------------------------------------------------------
// MISSION FAILURE PROGRAMS


//---------------------------------------------------------------------------

#define TELETYPE_TIME 3000

CQSCRIPTPROGRAM(TM11_MissionFailure, TM11_MissionFailure_Save, 0);

void TM11_MissionFailure::Initialize (U32 eventFlags, const MPartRef & part)
{
	M_ PlayMusic(DEFEAT);

	M_ EnableMovieMode(true);
	UnderMissionControl();	
	data.mission_over = true;
	state = Uplink;

}

bool TM11_MissionFailure::Update (void)
{
	switch (state)
	{
		case Uplink:
			if (!M_ IsTeletypePlaying(data.mhandle) && !M_ IsStreamPlaying(data.mhandle))
			{
				data.shandle = M_ PlayAudio("high_level_data_uplink.wav");
				state = Halsey;
			}
			break;

		case Halsey:
			if (!M_ IsStreamPlaying(data.shandle))
			{
				M_ FlushTeletype();
//				data.mhandle = M_ PlayTeletype(IDS_TM11_HALSEY_M11HA23, SuperLeft, SuperTop, SuperRight, SuperBottom, MOTextColor, 6000, 1000, false);
				data.mhandle = M_ PlayAnimatedMessage("M11HA37.wav", "Animate!!Halsey2", MagpieLeft, MagpieTop, IDS_TM11_SUB_M11HA37);

				state = Fail;
			}
			break;

		case Fail:
			if (!M_ IsStreamPlaying(data.mhandle))
			{
				M_ FlushTeletype();
				data.mhandle = TeletypeMissionOver(IDS_TM11_MISSION_FAILURE,data.failureID);
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

CQSCRIPTPROGRAM(TM11_MissionSuccess, TM11_MissionSuccess_Save, 0);

void TM11_MissionSuccess::Initialize (U32 eventFlags, const MPartRef & part)
{
	state = Uplink;

}

bool TM11_MissionSuccess::Update (void)
{
	switch (state)
	{
		case Uplink:
			if(data.mission_over)
			{
				return false;
			}
			if (!M_ IsTeletypePlaying(data.mhandle) && data.PartOneDone && data.PartTwoDone)
			{
				M_ PlayMusic(VICTORY);

				M_ EnableMovieMode(true);

				UnderMissionControl();	
				data.mission_over = true;

				data.shandle = M_ PlayAudio("high_level_data_uplink.wav");
				state = Halsey;
			}
			break;


		case Halsey:	
			if (!M_ IsStreamPlaying(data.shandle))
			{
				M_ FlushTeletype();
//				data.mhandle = M_ PlayTeletype(IDS_TM11_HALSEY_M11HA25, MOLeft, MOTop, MORight, MOBottom, MOTextColor, 5000, 2000, false);
				data.mhandle = M_ PlayAnimatedMessage("M11HA39.wav", "Animate!!Halsey2", MagpieLeft, MagpieTop, IDS_TM11_SUB_M11HA39);
				
				state = Success;
			}
			break;

		case Success:
			
			if(!M_ IsStreamPlaying(data.mhandle))
			{
				M_ FlushTeletype();
				data.mhandle = TeletypeMissionOver(IDS_TM11_MISSION_SUCCESS);
				
				state = Done;
			}
			break;

		case Done:

			UnderPlayerControl();
			M_ EndMissionVictory(10);
			return false;

			break;
	}

	return true;
}

//--------------------------------------------------------------------------//
//--------------------------------End Script11T.cpp-------------------------//
//--------------------------------------------------------------------------//
