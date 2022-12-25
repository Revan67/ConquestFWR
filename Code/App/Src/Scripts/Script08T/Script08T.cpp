//--------------------------------------------------------------------------//
//                                                                          //
//                                Script08T.cpp                             //
//                                                                          //
//               COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
   $Header: /Conquest/App/Src/Scripts/Script08T/Script08T.cpp 118   7/06/01 11:07a Tmauer $
*/
//--------------------------------------------------------------------------//

#include <ScriptDef.h>
#include <DLLScriptMain.h>
#include "DataDef.h"
#include "stdlib.h"

#include "..\helper\helper.h"



//--------------------------------------------------------------------------
//  TM8 #define's


#define HalseyLeft			150
#define HalseyRight			500
#define HalseyTop			10
#define HalseyBottom		100

#define BlackwellLeft		100
#define BlackwellRight		450
#define BlackwellTop		10
#define BlackwellBottom		100

#define MOLeft				150
#define MORight				600
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

	M_ EnableEnemyAI(MANTIS_ID, bOn, "MANTIS_FORWARD_BUILD");

	if (bOn)
	{
		AIPersonality airules;
		airules.difficulty = MEDIUM;
		airules.buildMask.bBuildPlatforms = true;
		airules.buildMask.bBuildLightGunboats = true;
		airules.buildMask.bBuildMediumGunboats = true;
		airules.buildMask.bBuildHeavyGunboats = false;
		airules.buildMask.bHarvest = true;
		airules.buildMask.bScout = true;
		airules.buildMask.bBuildAdmirals = false;
		airules.buildMask.bLaunchOffensives = true;
		airules.buildMask.bUseSpecialWeapons = false;
		airules.nGunboatsPerSupplyShip = 10;
		airules.nNumMinelayers = 1;
		airules.uNumScouts = 1;
		airules.nNumFabricators = 1;
		airules.fHarvestersPerRefinery = .5;
		airules.nMaxHarvesters = 6;
		airules.nPanicThreshold = 3000;
		airules.nFabricateEscorts = 3;
		airules.nCautiousness = 8;

		M_ SetEnemyAIRules(MANTIS_ID, airules);
	}

}

void ToggleBuildUnits(bool bOn)
{

	AIPersonality airules;
	airules.difficulty = MEDIUM;
	airules.buildMask.bBuildPlatforms = true;
	airules.buildMask.bBuildLightGunboats = bOn;
	airules.buildMask.bBuildMediumGunboats = bOn;
	airules.buildMask.bBuildHeavyGunboats = false;
	airules.buildMask.bHarvest = true;
	airules.buildMask.bScout = true;
	airules.buildMask.bBuildAdmirals = false;
	airules.buildMask.bLaunchOffensives = true;
	airules.buildMask.bUseSpecialWeapons = false;
	airules.nGunboatsPerSupplyShip = 10;
	airules.nNumMinelayers = 1;
	airules.uNumScouts = 3;
	airules.nNumFabricators = 1;
	airules.fHarvestersPerRefinery = .5;
	airules.nMaxHarvesters = 6;
	airules.nPanicThreshold = 3000;
	airules.nFabricateEscorts = 3;
	airules.nCautiousness = 8;


	M_ SetEnemyAIRules(MANTIS_ID, airules);
}


void HinderMantisAI()
{
	U32 gas, metal, crew;

	gas = M_ GetGas(MANTIS_ID);
	metal = M_ GetMetal(MANTIS_ID);
	crew = M_ GetCrew(MANTIS_ID);

	if (gas > 240) gas = 300;
	if (metal > 240) metal = 300;
	if (crew > 200) crew = 200;

	M_ SetGas(MANTIS_ID, gas);
	M_ SetMetal(MANTIS_ID, metal);
	M_ SetCrew(MANTIS_ID, crew);
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
//  Mission functions: Terran Campaign Mission 8
//--------------------------------------------------------------------------

CQSCRIPTDATA(MissionData, data);

//--------------------------------------------------------------------------//
//	An attempt to try and monitor and manage the Enemy AI

CQSCRIPTPROGRAM(TM8_AIManager, TM8_AIManager_Save,0);

void TM8_AIManager::Initialize (U32 eventFlags, const MPartRef & part)
{
	SetupAI(true);
	AItimer = 40;
	Crewtimer = 0;

	L1 = M_ GetPartByName("Lemo Prime");
	L4 = M_ GetPartByName("Lemo 4");

}

bool TM8_AIManager::Update (void)
{

	if(data.mission_over == true)
	{
		return false;
	}

	Crewtimer -= ELAPSED_TIME;

	if(L1.isValid() && L4.isValid() && Crewtimer <= 0)
	{
		L1Crew = M_ GetCrewOnPlanet(L1);
		L4Crew = M_ GetCrewOnPlanet(L4);

		if(L1Crew < 500)
		{
			M_ SetResourcesOnPlanet(L1, -1, -1, L1Crew + 1);

		}

		if(L4Crew < 1000)
		{
			M_ SetResourcesOnPlanet(L4, -1, -1, L4Crew + 1);

		}

		Crewtimer = 3;
	}

 //Set up cases in which the AI will turn off
	if(M_ GetUsedCommandPoints(MANTIS_ID) > 170 || M_ GetUsedCommandPoints(PLAYER_ID) < 10)
	{
		ToggleBuildUnits(false);

		return true;
	}

 //Set up cases in which the AI will turn back on
	if(M_ GetUsedCommandPoints(MANTIS_ID) < 30 || M_ GetUsedCommandPoints(PLAYER_ID) > 120)
	{
		ToggleBuildUnits(true);
	}

/*	if (AItimer == 0)
	{
		HinderMantisAI();
		AItimer = 240;
	}
	else
		AItimer--;
*/
	return true;
}

//--------------------------------------------------------------------------//
//	Mission Briefing
//--------------------------------------------------------------------------//

CQSCRIPTPROGRAM(TM8_Briefing, TM8_Briefing_Save,CQPROGFLAG_STARTBRIEFING);

void TM8_Briefing::Initialize (U32 eventFlags, const MPartRef & part)
{
	data.failureID = 0;
	data.mhandle = 0;
	data.shandle = 0;
	data.mission_over = data.briefing_over = false;
	data.mission_state = Briefing;

	M_ PlayMusic(NULL);
	M_ EnableBriefingControls(true);

	state = Begin;
}

bool TM8_Briefing::Update (void)
{
	if (data.briefing_over)
		return false;

	switch(state)
	{
		case Begin:

//			data.mhandle = M_ PlayAudio("M08HR00A.wav");
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

			state = TeleTypeLocation;

			break;

		case TeleTypeLocation:

			M_ FlushTeletype();
			data.mhandle = TeletypeBriefing(IDS_TM8_TELETYPE_LOCATION, false);
			state = Radio;	
			
			break;

		case Radio:

			if (!M_ IsStreamPlaying(data.mhandle))
			{
				data.mhandle = M_ PlayAudio("prolog08.wav");
				MScript::BriefingSubtitle(data.mhandle,IDS_TM8_SUB_PROLOG08);
				state = Uplink;
			}

			break;

/*		case Radio1:

			if (!M_ IsStreamPlaying(data.mhandle))
			{
				data.mhandle = M_ PlayAudio("M08SY02.wav");
				state = Uplink;
			}

			break;
*/
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

				state = Radio2;
			}
			break;

		case Radio2:

			if (M_ IsCustomBriefingAnimationDone(0))
			{
				strcpy(slotAnim.szFileName, "m08ha03.wav");
				strcpy(slotAnim.szTypeName, "Animate!!Halsey");
				slotAnim.slotID = 0;
				slotAnim.bHighlite = false;
				data.mhandle = M_ PlayBriefingTalkingHead(slotAnim);
				MScript::BriefingSubtitle(data.mhandle,IDS_TM8_SUB_M08HA03);

				M_ FreeBriefingSlot(1);
				M_ FreeBriefingSlot(2);
				M_ FreeBriefingSlot(3);


 //				data.mhandle = M_ PlayAudio("M08HA03.wav");
				state = Radio3;
			}

			break;

		case Radio3:

			if (!M_ IsStreamPlaying(data.mhandle))
			{
//				data.mhandle = M_ PlayAudio("M08HW04.wav");
				state = HalseyBrief;
			}

			break;

		case HalseyBrief:

			if(!M_ IsStreamPlaying(data.mhandle))
			{
//				data.mhandle = M_ PlayAudio("M08HA05.wav");
				state = MO;
			}

			break;


		case MO:
			
			if (!M_ IsStreamPlaying(data.mhandle))
			{
				M_ FlushTeletype();

				data.mhandle = TeletypeBriefing(IDS_TM8_OBJECTIVES, true);
				state = Finish;
			}

			break;
			
		case Finish:
			if (!M_ IsTeletypePlaying(data.mhandle))
				{
//					M_ RunProgramByName("TM8_BriefingSkip",MPartRef ());
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

CQSCRIPTPROGRAM(TM8_MissionStart, TM8_MissionStart_Save,CQPROGFLAG_STARTMISSION);

void TM8_MissionStart::Initialize (U32 eventFlags, const MPartRef & part)
{
	M_ SetMissionName(IDS_TM8_MISSION_NAME);
	M_ SetMissionID(7);

	M_ SetEnemyCharacter(MANTIS_ID, IDS_TM8_MANTIS);

	M_ EnableRegenMode(true);

	M_ SetMissionDescription(IDS_TM8_MISSION_DESC);
	M_ AddToObjectiveList(IDS_TM8_OBJECTIVE1);
	M_ AddToObjectiveList(IDS_TM8_OBJECTIVE2);

	data.mission_state = MissionStart;

	M_ EnableSystem(CENTAURUS, false, false);
	M_ EnableSystem(GROG, false, false);
	M_ EnableSystem(TALOS, false, false);

	M_ EnableJumpgate(M_ GetPartByName("Gate28"), false);
	M_ EnableJumpgate(M_ GetPartByName("Gate14"), false);

	data.HasTalosAttacked = data.HasKracusAttacked = data.HasSendlorAttacked = data.BensonFound = false;

	TECHNODE missionTech;

	//Set up player Tech lvls

	missionTech.InitLevel( TECHTREE::FULL_TREE );

	missionTech.race[0].build = missionTech.race[1].build = (TECHTREE::BUILDNODE) (
		TECHTREE::RES_REFINERY_GAS1 |
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
		TECHTREE::TDEPEND_AWSLAB
        );

	missionTech.race[1].build = (TECHTREE::BUILDNODE) (
		missionTech.race[1].build |
		TECHTREE::MDEPEND_PLANTATION |
		TECHTREE::MDEPEND_BIOFORGE 
		);

	missionTech.race[1].build = (TECHTREE::BUILDNODE) (
		missionTech.race[1].build ^
		TECHTREE::MDEPEND_DUAL_SPITTER ^
		TECHTREE::MDEPEND_PLASMA_HIVE
		);

    // add in necessary upgrades, and troopship

	missionTech.race[0].tech = missionTech.race[1].tech = (TECHTREE::TECHUPGRADE) (
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
		TECHTREE::T_RES_TROOPSHIP2 |
		TECHTREE::T_RES_MISSLEPACK1 |
		TECHTREE::T_RES_MISSLEPACK2 |
		TECHTREE::T_RES_XCHARGES |
		TECHTREE::T_RES_XPROBE
        );

	missionTech.race[1].tech = (TECHTREE::TECHUPGRADE) (
		missionTech.race[1].tech |
		TECHTREE::M_SHIP_KHAMIR
		);

	missionTech.race[1].tech = (TECHTREE::TECHUPGRADE) (
		missionTech.race[1].tech ^
		TECHTREE::M_SHIP_TIAMAT
		);

	missionTech.race[0].common_extra = missionTech.race[1].common_extra = (TECHTREE::COMMON_EXTRA) (
        TECHTREE::RES_SENSORS1  |
		TECHTREE::RES_TANKER1  |
		TECHTREE::RES_TANKER2  |
		TECHTREE::RES_TENDER1 |
		TECHTREE::RES_TENDER2 |
		TECHTREE::RES_FIGHTER1 |
		TECHTREE::RES_FIGHTER2 |
		TECHTREE::RES_FIGHTER3
        );

	missionTech.race[0].common = missionTech.race[1].common = (TECHTREE::COMMON) (
		TECHTREE::RES_WEAPONS1  |
		TECHTREE::RES_WEAPONS2  |
		TECHTREE::RES_SUPPLY1 |
		TECHTREE::RES_SUPPLY2  |
		TECHTREE::RES_SUPPLY3  |
		TECHTREE::RES_HULL1  |
		TECHTREE::RES_HULL2  |
		TECHTREE::RES_HULL3  |
		TECHTREE::RES_HULL4  |
		TECHTREE::RES_ENGINE1 |
		TECHTREE::RES_ENGINE2
        );

    MScript::SetAvailiableTech( missionTech );

    // add in necessary upgrades for the destabalizer
	TECHNODE missionTech1 = M_ GetPlayerTech(PLAYER_ID);

	missionTech1.race[2].tech = (TECHTREE::TECHUPGRADE) (TECHTREE::S_RES_DESTABILIZER);

    MScript::SetPlayerTech(  PLAYER_ID, missionTech1 );

    M_ MoveCamera(FindFirstObjectOfType(M_HQ, PLAYER_ID, LEMO), 0, 0);

	M_ SetMaxCommandPoitns(MANTIS_ID, 200);

	M_ SetMaxGas(MANTIS_ID, 240);
	M_ SetMaxMetal(MANTIS_ID, 240);
	M_ SetMaxCrew(MANTIS_ID, 200);


	M_ SetGas(PLAYER_ID, 250);
	M_ SetMetal(PLAYER_ID, 250);
	M_ SetCrew(PLAYER_ID, 100);
	
	M_ SetGas(MANTIS_ID, 400);
	M_ SetMetal(MANTIS_ID, 400);
	M_ SetCrew(MANTIS_ID, 100);

	// Initialize AI
	SetupAI(true);
	
	MassEnableAI(MANTIS_ID, false, 0);
	MassEnableAI(MANTIS_ID, true, AEON);
	MassEnableAI(MANTIS_ID, true, GRAAN);
	MassEnableAI(MANTIS_ID, true, ARCTURUS);
	MassEnableAI(MANTIS_ID, true, KALIDON);
	MassEnableAI(MANTIS_ID, true, KRACUS);

	M_ EnableJumpgate(M_ GetPartByName("Gate26"), false);
	M_ EnableJumpgate(M_ GetPartByName("Gate22"), false);

	data.KerTak	= M_ GetPartByName("Ker'Tak");
	M_ EnablePartnameDisplay(data.KerTak, true);
	data.BensonWP  = M_ GetPartByName("BensonWP");



	//Create a trigger for Benson's entrance
	data.BensonTrigger = M_ CreatePart("MISSION!!TRIGGER", data.BensonWP, 0);
	M_ SetTriggerFilter(data.BensonTrigger, 0, TRIGGER_FORCEREADY, false);
	M_ SetTriggerFilter(data.BensonTrigger, PLAYER_ID, TRIGGER_PLAYER, true);
	M_ SetTriggerProgram(data.BensonTrigger, "TM8_BensonSeen");
	M_ SetTriggerRange(data.BensonTrigger, 6);
	M_ EnableTrigger(data.BensonTrigger, true);

	M_ RunProgramByName("TM8_AeonVisited", MPartRef());
	M_ RunProgramByName("TM8_AIManager", MPartRef());
	M_ RunProgramByName("TM8_NearKalidon", MPartRef());

	timer = 2;
	flag = false;
}

bool TM8_MissionStart::Update (void)
{
	if(data.mission_over == true)
	{
		return false;
	}
	
	timer -= ELAPSED_TIME;

	if(timer < 0 && !flag)
	{
//		data.mhandle = M_ PlayAudio("M08KR06.wav", data.KerTak);
		data.mhandle = M_ PlayAnimatedMessage("M08KR04.wav", "Animate!!KerTak2", MagpieLeft, MagpieTop, data.KerTak,IDS_TM8_SUB_M08KR04);

		flag = true;
	}

	if(!M_ IsStreamPlaying(data.mhandle) && flag)
	{
		M_ AlertSector(KALIDON);

		return false;
	}

	return true;
}

//--------------------------------------------------------------------------//
//	

CQSCRIPTPROGRAM(TM8_AeonVisited, TM8_AeonVisited_Save,0);

void TM8_AeonVisited::Initialize (U32 eventFlags, const MPartRef & part)
{

}

bool TM8_AeonVisited::Update (void)
{
	if(data.mission_over == true)
	{
		return false;
	}

	if(PlayerInSystem(PLAYER_ID, AEON))
	{
//		data.mhandle = M_ PlayAudio("M08KR07.wav", data.KerTak);
		data.mhandle = M_ PlayAnimatedMessage("M08KR05.wav", "Animate!!KerTak2", MagpieLeft, MagpieTop, data.KerTak, IDS_TM8_SUB_M08KR05);

		return false;
	}

	return true;
}

//--------------------------------------------------------------------------//
//	
CQSCRIPTPROGRAM(TM8_NearKalidon, TM8_NearKalidon_Save,0);

void TM8_NearKalidon::Initialize (U32 eventFlags, const MPartRef & part)
{
	Gate1 = M_ GetPartByName("Gate8");
	Gate2 = M_ GetPartByName("Gate17");

	AskedForHelp = false;
}

bool TM8_NearKalidon::Update (void)
{
	if(data.mission_over == true)
	{
		return false;
	}

	switch(state)
	{
		case Asked:
			if((M_ IsVisibleToPlayer(Gate1 ,PLAYER_ID) || M_ IsVisibleToPlayer(Gate2 , PLAYER_ID)))
			{
//				data.mhandle = M_ PlayAudio("M08BN08.wav");
				data.mhandle = M_ PlayAnimatedMessage("M08BN06.wav", "Animate!!Benson2", MagpieLeft, MagpieTop, IDS_TM8_SUB_M08BN06);

				M_ ClearHardFog (data.BensonWP, 2);
				M_ AlertMessage(data.BensonWP, 0);


				AskedForHelp = true;
				M_ RunProgramByName("TM8_BensonDeathClock", MPartRef());
				M_ RunProgramByName("TM8_FlashBeacon", MPartRef());

				state = Next;
			}
			break;

		case Next:
			if(!M_ IsStreamPlaying(data.mhandle))
			{
				M_ FlushTeletype();
				data.mhandle = TeletypeObjective(IDS_TM8_IMMEDIATEMO);
				M_ AddToObjectiveList(IDS_TM8_OBJECTIVE6);

				state = End;
			}
			break;

		case End:
			if(PlayerInSystem(PLAYER_ID, KALIDON))
			{
				M_ RunProgramByName("TM8_InKalidon", MPartRef());

				return false;
			}
			break;
	}


	return true;
}

//--------------------------------------------------------------------------//
//	
CQSCRIPTPROGRAM(TM8_InKalidon, TM8_InKalidon_Save,0);

void TM8_InKalidon::Initialize (U32 eventFlags, const MPartRef & part)
{
	state = Begin;
}

bool TM8_InKalidon::Update (void)
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
//				data.mhandle = M_ PlayAudio("M08BN09.wav");
				data.mhandle = M_ PlayAnimatedMessage("M08BN09.wav", "Animate!!Benson2", MagpieLeft, MagpieTop, IDS_TM8_SUB_M08BN09);

				M_ AlertMessage(data.BensonWP, 0);

				state = KerTak;
			}
			break;

		case KerTak:
			if(!M_ IsStreamPlaying(data.mhandle))
			{
//				data.mhandle = M_ PlayTeletype(IDS_TM8_KERTAK_M08KR10, SuperLeft, SuperTop, SuperRight, SuperBottom, MOTextColor, 6000, 2000, false);
				data.mhandle = M_ PlayAnimatedMessage("M08KR10.wav", "Animate!!KerTak2", MagpieLeft, MagpieTop, data.KerTak, IDS_TM8_SUB_M08KR10);

				state = Benson;
			}
			break;

		case Benson:
			if(!M_ IsStreamPlaying(data.mhandle))
			{
				data.mhandle = M_ PlayAnimatedMessage("M08BN11.wav", "Animate!!Benson2", MagpieLeft, MagpieTop, IDS_TM8_SUB_M08BN11);

				state = End;
			}
			break;

		case End:
			if(!M_ IsStreamPlaying(data.mhandle))
			{
				M_ FlushTeletype();
//				data.mhandle = M_ PlayTeletype(IDS_TM8_KERTAK_M08KR12, SuperLeft, SuperTop, SuperRight, SuperBottom, MOTextColor, 6000, 2000, false);
				data.mhandle = M_ PlayAnimatedMessage("M08KR12.wav", "Animate!!KerTak2", MagpieLeft, MagpieTop, data.KerTak, IDS_TM8_SUB_M08KR12);

				return false;
			}
			break;
	}

	return true;
}

//-----------------------------------------------------------------------

CQSCRIPTPROGRAM(TM8_FlashBeacon, TM8_FlashBeacon_Save, 0);

void TM8_FlashBeacon::Initialize (U32 eventFlags, const MPartRef & part)
{
	count=0;
}

bool TM8_FlashBeacon::Update (void)
{
	if(data.mission_over == true)
	{
		return false;
	}

	if (!data.BensonFound)
	{
		count++;
		if (count >= 20)
		{
			// Highlight the beacon on the players map...
			M_ AlertObjectInSysMap(data.BensonWP, true);
			count = 0;

			if(M_ GetCurrentSystem() == KALIDON)
			{
				M_ PlayAudio("specialDenied.wav");
			}			
		}
		return true;
	}
	else
		return false;
}

//--------------------------------------------------------------------------//
//	
CQSCRIPTPROGRAM(TM8_BensonDeathClock, TM8_BensonDeathClock_Save,0);

void TM8_BensonDeathClock::Initialize (U32 eventFlags, const MPartRef & part)
{
	timer = 300;
}

bool TM8_BensonDeathClock::Update (void)
{
	if(data.mission_over == true)
	{
		return false;
	}

	timer -= ELAPSED_TIME;

	if(data.BensonFound)
	{
		return false;
	}

	if(timer < 0)
	{
		if(data.Benson.isValid())
		{
			M_ RemovePart(data.Benson);
		}

		M_ RunProgramByName("TM8_BensonKilled", MPartRef());

		return false;
	}

	return true;
}

//--------------------------------------------------------------------------//
//	
CQSCRIPTPROGRAM(TM8_BensonSeen, TM8_BensonSeen_Save,0);

void TM8_BensonSeen::Initialize (U32 eventFlags, const MPartRef & part)
{
	M_ EnableTrigger(data.BensonTrigger, false);
	data.BensonFound = true;

	FrigWP = M_ GetPartByName("FrigateWP");
	Frig2WP = M_ GetPartByName("Frig2WP");

	Frig1 = M_ CreatePart("GBOAT!!M_Frigate", FrigWP, MANTIS_ID);
	Frig2 = M_ CreatePart("GBOAT!!M_Frigate", Frig2WP, MANTIS_ID);

	data.Benson = M_ CreatePart("CHAR!!Benson_BS", data.BensonWP, PLAYER_ID, "Benson");
	M_ EnablePartnameDisplay(data.Benson, true);

	M_ OrderAttack(Frig1, data.Benson);
	M_ OrderAttack(Frig2, data.Benson);

	M_ OrderAttack(data.Benson, Frig1);
	M_ SetHullPoints(data.Benson, 350);
	M_ SetSupplies(data.Benson, 75);


	state = Begin;

	timer = 0;
}

bool TM8_BensonSeen::Update (void)
{
	if(data.mission_over == true)
	{
		return false;
	}

	timer -= ELAPSED_TIME;

	switch(state)
	{
	case Begin:


			state = KillFrigate1;
			timer = 4;

		break;

	case KillFrigate1:
		if(timer < 0)
		{
			M_ DestroyPart(Frig1);
			M_ OrderAttack(data.Benson, Frig2);

			state = KillFrigate2;
			timer = 4;
		}
		break;

	case KillFrigate2:
		if(timer < 0)
		{
			M_ DestroyPart(Frig2);

			state = Resupply;
			timer = 4;
		}
		break;

	case Resupply:
		if(!M_ IsStreamPlaying(data.mhandle))
		{
//			data.mhandle = M_ PlayAudio("M08BN10.wav", data.Benson);
			data.mhandle = M_ PlayAnimatedMessage("M08BN13.wav", "Animate!!Benson2", MagpieLeft, MagpieTop, data.Benson, IDS_TM8_SUB_M08BN13);

			M_ EnableJumpgate(M_ GetPartByName("Gate14"), true);

			MassEnableAI(MANTIS_ID, true, VARN);
			MassEnableAI(MANTIS_ID, true, SENDLOR);
						
			M_ RunProgramByName("TM8_SecondSendlorWaveAttack", MPartRef());
			M_ RunProgramByName("TM8_VarnWaveAttack", MPartRef());


			M_ RemoveFromObjectiveList(IDS_TM8_OBJECTIVE2);
			M_ RemoveFromObjectiveList(IDS_TM8_OBJECTIVE6);
			M_ MarkObjectiveCompleted(IDS_TM8_OBJECTIVE1);

			state = NewMO;
		}
		break;

	case NewMO:
		if(!M_ IsStreamPlaying(data.mhandle))
		{
			M_ FlushTeletype();
			data.mhandle = TeletypeObjective(IDS_TM8_OBJECTIVE3);
			M_ AddToObjectiveList(IDS_TM8_OBJECTIVE3);

			state = Ob2;
		}
		break;

	case Ob2:
		if(!M_ IsTeletypePlaying(data.mhandle))
		{
			M_ FlushTeletype();
			data.mhandle = TeletypeObjective(IDS_TM8_OBJECTIVE4);
			M_ AddToObjectiveList(IDS_TM8_OBJECTIVE4);

			state = Ob3;
		}
		break;

	case Ob3:
		if(!M_ IsTeletypePlaying(data.mhandle))
		{
			M_ FlushTeletype();
			data.mhandle = TeletypeObjective(IDS_TM8_OBJECTIVE5);
			M_ AddToObjectiveList(IDS_TM8_OBJECTIVE5);

			return false;

		}
		break;

	}

	return true;
}

//--------------------------------------------------------------------------//
//	

CQSCRIPTPROGRAM(TM8_KracusWaveAttack, TM8_KracusWaveAttack_Save,0);

void TM8_KracusWaveAttack::Initialize (U32 eventFlags, const MPartRef & part)
{
	timer = 0;

	FrigateGen = M_ GetPartByName("KracusWave");

}

bool TM8_KracusWaveAttack::Update (void)
{
	if(data.mission_over == true)
	{
		return false;
	}

	timer -= ELAPSED_TIME;


	if(CountObjects(M_THRIPID, MANTIS_ID, KRACUS) == 0)
	{
		return false;
	}
	else
	{
		if(timer < 0 && CountObjectsInRange(0, PLAYER_ID, FrigateGen, 5) == 0 && M_ GetUsedCommandPoints(MANTIS_ID) < M_ GetMaxCommandPoints(MANTIS_ID))
		{
			GenAndAttack("GBOAT!!M_Frigate",FrigateGen);
			GenAndAttack("GBOAT!!M_Scarab",FrigateGen);
			GenAndAttack("GBOAT!!M_Scarab",FrigateGen);
		
			timer = MANTIS_WAVE_INTERVAL;

			return true;
		}
	}


	return true;
}

//--------------------------------------------------------------------------//
//	

CQSCRIPTPROGRAM(TM8_SendlorWaveAttack, TM8_SendlorWaveAttack_Save,0);

void TM8_SendlorWaveAttack::Initialize (U32 eventFlags, const MPartRef & part)
{
	timer = 0;

	ScoutGen = M_ GetPartByName("SendlorWaveOne");
}

bool TM8_SendlorWaveAttack::Update (void)
{
	if(data.mission_over == true)
	{
		return false;
	}

	timer -= ELAPSED_TIME;


	if(CountObjects(M_THRIPID, MANTIS_ID, SENDLOR) == 0)
	{
		return false;
	}
	else
	{
		if(timer < 0 && CountObjectsInRange(0, PLAYER_ID, ScoutGen, 5) == 0 && M_ GetUsedCommandPoints(MANTIS_ID) < M_ GetMaxCommandPoints(MANTIS_ID))
		{
			GenAndAttack("GBOAT!!M_Scout Carrier",ScoutGen);
			GenAndAttack("GBOAT!!M_Hive Carrier",ScoutGen);
			GenAndAttack("GBOAT!!M_Hive Carrier",ScoutGen);
		
			timer = MANTIS_WAVE_INTERVAL;

			return true;
		}
	}

	return true;
}

//--------------------------------------------------------------------------//
//	

CQSCRIPTPROGRAM(TM8_VarnWaveAttack, TM8_VarnWaveAttack_Save,0);

void TM8_VarnWaveAttack::Initialize (U32 eventFlags, const MPartRef & part)
{
	timer = 0;

	HiveGen = M_ GetPartByName("VarnWave");
}

bool TM8_VarnWaveAttack::Update (void)
{
	if(data.mission_over == true)
	{
		return false;
	}

	timer -= ELAPSED_TIME;


	if(CountObjects(M_NIAD, MANTIS_ID, VARN) == 0)
	{
		return false;
	}
	else
	{
		if(timer < 0 && CountObjectsInRange(0, PLAYER_ID, HiveGen, 5) == 0 && M_ GetUsedCommandPoints(MANTIS_ID) < M_ GetMaxCommandPoints(MANTIS_ID))
		{
			GenAndAttack("GBOAT!!M_Hive Carrier",HiveGen);
			GenAndAttack("GBOAT!!M_Hive Carrier",HiveGen);
//			GenAndAttack("GBOAT!!M_Hive Carrier",HiveGen);
		
			timer = MANTIS_WAVE_INTERVAL2;

			return true;
		}
	}

	return true;}

//--------------------------------------------------------------------------//
//	

CQSCRIPTPROGRAM(TM8_SecondSendlorWaveAttack, TM8_SecondSendlorWaveAttack_Save,0);

void TM8_SecondSendlorWaveAttack::Initialize (U32 eventFlags, const MPartRef & part)
{
	timer = 0;

	ScarabGen = M_ GetPartByName("SendlorWaveTwo");
}

bool TM8_SecondSendlorWaveAttack::Update (void)
{
	if(data.mission_over == true)
	{
		return false;
	}

	timer -= ELAPSED_TIME;


	if(CountObjects(M_NIAD, MANTIS_ID, SENDLOR) == 0)
	{
		return false;
	}
	else
	{
		if(timer < 0 && CountObjectsInRange(0, PLAYER_ID, ScarabGen, 5) == 0 && M_ GetUsedCommandPoints(MANTIS_ID) < M_ GetMaxCommandPoints(MANTIS_ID))
		{
			GenAndAttack("GBOAT!!M_Scarab",ScarabGen);
			GenAndAttack("GBOAT!!M_Scarab",ScarabGen);
//			GenAndAttack("GBOAT!!M_Scarab",ScarabGen);
		
			timer = MANTIS_WAVE_INTERVAL2;

			return true;
		}
	}

	return true;}


//--------------------------------------------------------------------------//
//	

CQSCRIPTPROGRAM(TM8_TalosWave, TM8_TalosWave_Save,0);

void TM8_TalosWave::Initialize (U32 eventFlags, const MPartRef & part)
{
	TalosWaveWaypoint = M_ GetPartByName("TalosAttackWP");
	ElanWP = M_ GetPartByName("ElanWP");
	BlackwellWP = M_ GetPartByName("BlackwellWP");
	TaosWP = M_ GetPartByName("TaosWP");
	TriremeWP = M_ GetPartByName("TriremeWP");

	data.BlackwellDone = false;

	data.Frigate1 = FindFirstObjectOfType(M_FRIGATE, MANTIS_ID, TALOS);
	data.Frigate2 = FindNextObjectOfType(M_FRIGATE, MANTIS_ID, TALOS, data.Frigate1);
	data.Frigate3 = FindNextObjectOfType(M_FRIGATE, MANTIS_ID, TALOS, data.Frigate2);
	data.Frigate4 = FindNextObjectOfType(M_FRIGATE, MANTIS_ID, TALOS, data.Frigate3);
	data.Frigate5 = FindNextObjectOfType(M_FRIGATE, MANTIS_ID, TALOS, data.Frigate4);
	data.Frigate6 = FindNextObjectOfType(M_FRIGATE, MANTIS_ID, TALOS, data.Frigate5);
	data.Frigate7 = FindNextObjectOfType(M_FRIGATE, MANTIS_ID, TALOS, data.Frigate6);
	data.Frigate8 = FindNextObjectOfType(M_FRIGATE, MANTIS_ID, TALOS, data.Frigate7);

	data.Hive1 = FindFirstObjectOfType(M_HIVECARRIER, MANTIS_ID, TALOS);
	data.Hive2 = FindNextObjectOfType(M_HIVECARRIER, MANTIS_ID, TALOS, data.Hive1);
	data.Hive3 = FindNextObjectOfType(M_HIVECARRIER, MANTIS_ID, TALOS, data.Hive2);
	data.Hive4 = FindNextObjectOfType(M_HIVECARRIER, MANTIS_ID, TALOS, data.Hive3);

	data.Scarab1 = FindFirstObjectOfType(M_SCARAB, MANTIS_ID, TALOS);
	data.Scarab2 = FindNextObjectOfType(M_SCARAB, MANTIS_ID, TALOS, data.Scarab1);
	data.Scarab3 = FindNextObjectOfType(M_SCARAB, MANTIS_ID, TALOS, data.Scarab2);
	data.Scarab4 = FindNextObjectOfType(M_SCARAB, MANTIS_ID, TALOS, data.Scarab3);

	MassMove(M_SCARAB, MANTIS_ID, TalosWaveWaypoint, TALOS);
	MassMove(M_HIVECARRIER, MANTIS_ID, TalosWaveWaypoint, TALOS);

	timer = 6;

	state = Begin;
}

bool TM8_TalosWave::Update (void)
{
	if(data.mission_over == true)
	{
		return false;
	}

	timer -= ELAPSED_TIME;
	
	switch(state)
	{
	case Begin:
		if(timer <= 0)
		{
			MassMove(M_FRIGATE, MANTIS_ID, TalosWaveWaypoint, TALOS);
	
			state = EnemySighted;
		}
		break;

	case EnemySighted:
		if(M_ IsVisibleToPlayer(data.Frigate1, PLAYER_ID) || M_ IsVisibleToPlayer(data.Frigate2, PLAYER_ID) || M_ IsVisibleToPlayer(data.Frigate3, PLAYER_ID) || M_ IsVisibleToPlayer(data.Frigate4, PLAYER_ID) || M_ IsVisibleToPlayer(data.Frigate5, PLAYER_ID) || M_ IsVisibleToPlayer(data.Frigate6, PLAYER_ID) || M_ IsVisibleToPlayer(data.Frigate7, PLAYER_ID) || M_ IsVisibleToPlayer(data.Frigate8, PLAYER_ID))
		{
//			M_ SetAllies(PLAYER_ID, MANTIS_ID, true);
//			M_ SetAllies(MANTIS_ID, PLAYER_ID, true);

//			data.mhandle = M_ PlayAudio("M08KR11.wav", data.KerTak);
			data.mhandle = M_ PlayAnimatedMessage("M08KR14.wav", "Animate!!KerTak2", MagpieLeft, MagpieTop, data.KerTak, IDS_TM8_SUB_M08KR14);
			
			M_ PlayMusic(DANGER);

			state = BensonSpeaks;
		}
		break;

	case BensonSpeaks:
		if(!M_ IsStreamPlaying(data.mhandle))
		{
//			data.mhandle = M_ PlayAudio("M08BN12.wav", data.Benson);
			data.mhandle = M_ PlayAnimatedMessage("M08BN15.wav", "Animate!!Benson2", MagpieLeft, MagpieTop, data.Benson, IDS_TM8_SUB_M08BN15);
			
			state = KerTakPanic;
		}
		break;

	case KerTakPanic:
		if(!M_ IsStreamPlaying(data.mhandle))
		{
//			data.mhandle = M_ PlayAudio("M08KR13.wav", data.KerTak);
			data.mhandle = M_ PlayAnimatedMessage("M08KR16.wav", "Animate!!KerTak2", MagpieLeft, MagpieTop, data.KerTak, IDS_TM8_SUB_M08KR16);
			
			MassCancelAll(PLAYER_ID, VARN);
			state = BringInBlackwell;
			timer = 5;
		}
		break;

	case BringInBlackwell:
		if(!M_ IsStreamPlaying(data.mhandle) && timer <= 0)
		{
			M_ RunProgramByName("TM8_BlowUpSendlor", MPartRef());

			data.Blackwell = M_ CreatePart("CHAR!!Blackwell_Chrome", TriremeWP, PLAYER_ID, "Blackwell");
			M_ EnablePartnameDisplay(data.Blackwell, true);

			data.Elan = M_ CreatePart("GBOAT!!S_Monolith", ElanWP, PLAYER_ID, 0);

			M_ CreatePart("GBOAT!!S_Taos", TaosWP, PLAYER_ID, 0);
			M_ CreatePart("GBOAT!!S_Taos", TaosWP, PLAYER_ID, 0);
			M_ CreatePart("GBOAT!!S_Taos", TaosWP, PLAYER_ID, 0);
			M_ CreatePart("GBOAT!!S_Taos", TaosWP, PLAYER_ID, 0);
			M_ CreatePart("GBOAT!!S_Taos", TaosWP, PLAYER_ID, 0);
			M_ CreatePart("GBOAT!!S_Taos", TaosWP, PLAYER_ID, 0);

			SolAttacker1 = M_ CreatePart("GBOAT!!S_Trireme", BlackwellWP, PLAYER_ID, 0);
			SolAttacker2 = M_ CreatePart("GBOAT!!S_Trireme", BlackwellWP, PLAYER_ID, 0);
			SolAttacker3 = M_ CreatePart("GBOAT!!S_Trireme", BlackwellWP, PLAYER_ID, 0);
			SolAttacker4 = M_ CreatePart("GBOAT!!S_Trireme", BlackwellWP, PLAYER_ID, 0);

//			data.mhandle = M_ PlayAudio("M08BL17.wav", data.Blackwell);
			data.mhandle = M_ PlayAnimatedMessage("M08BL17.wav", "Animate!!Blackwell2", MagpieLeft, MagpieTop, data.Blackwell, IDS_TM8_SUB_M08BL17);

//			M_ SaveCameraPos();
		    M_ MoveCamera(data.Blackwell, 2, 0);
			
			state = Benson;
		}
		break;

	case Benson:
		if(!M_ IsStreamPlaying(data.mhandle))
		{

//			data.mhandle = M_ PlayAudio("M08BN15.wav", data.Benson);
			data.mhandle = M_ PlayAnimatedMessage("M08BN18.wav", "Animate!!Benson2", MagpieLeft, MagpieTop, data.Benson,IDS_TM8_SUB_M08BN18);

			state = Blackwell;
		}
		break;

	case Blackwell:
		if(!M_ IsStreamPlaying(data.mhandle))
		{
//			data.mhandle = M_ PlayAudio("M08BL19.wav", data.Blackwell);
			data.mhandle = M_ PlayAnimatedMessage("M08BL19.wav", "Animate!!Blackwell2", MagpieLeft, MagpieTop, data.Blackwell, IDS_TM8_SUB_M08BL19);

			M_ PlayMusic(BATTLE);

			state = Destabilizer;
		}
		break;

	case Destabilizer:
		if(!M_ IsStreamPlaying(data.mhandle))
		{
			M_ RunProgramByName("TM8_FinishOffTalosWave", MPartRef());

			M_ OrderSpecialAttack(data.Blackwell, data.Scarab1);
			M_ OrderSpecialAttack(SolAttacker1, data.Hive1);
//			M_ OrderSpecialAttack(SolAttacker2, data.Scarab1);
//			M_ OrderSpecialAttack(SolAttacker3, data.Frigate1);
			M_ OrderSpecialAttack(SolAttacker4, TalosWaveWaypoint);


//			M_ LoadCameraPos(1, 0);

			state = Fire;

		}
		break;

	case Fire:
		if(data.Blackwell->supplies < data.Blackwell->supplyPointsMax)
		{
			timer = 8;

			state = BensonFreaks;
		}
		break;

	case BensonFreaks:
		if(!M_ IsStreamPlaying(data.mhandle) && timer < 0)
		{
//			data.mhandle = M_ PlayAudio("M08BN17.wav", data.Benson);
			data.mhandle = M_ PlayAnimatedMessage("M08BN20.wav", "Animate!!Benson2", MagpieLeft, MagpieTop, data.Benson, IDS_TM8_SUB_M08BN20);

			state = BlackwellExplains;
		}
		break;

	case BlackwellExplains:
		if(!M_ IsStreamPlaying(data.mhandle))
		{
//			data.mhandle = M_ PlayAudio("M08BL21.wav", data.Blackwell);
			data.mhandle = M_ PlayAnimatedMessage("M08BL21.wav", "Animate!!Blackwell2", MagpieLeft, MagpieTop, data.Blackwell, IDS_TM8_SUB_M08BL21);

			state = BensonRetorts;
		}
		break;

	case BensonRetorts:
		if(!M_ IsStreamPlaying(data.mhandle))
		{
//			data.mhandle = M_ PlayAudio("M08BN19.wav", data.Benson);
			data.mhandle = M_ PlayAnimatedMessage("M08BN22.wav", "Animate!!Benson2", MagpieLeft, MagpieTop, data.Benson, IDS_TM8_SUB_M08BN22);

			state = BlackwellShutsUp;
		}
		break;

	case BlackwellShutsUp:
		if(!M_ IsStreamPlaying(data.mhandle))
		{
//			data.mhandle = M_ PlayAudio("M08BL23.wav", data.Blackwell);
			data.mhandle = M_ PlayAnimatedMessage("M08BL23.wav", "Animate!!Blackwell2", MagpieLeft, MagpieTop, data.Blackwell, IDS_TM8_SUB_M08BL23);

			data.BlackwellDone = true;
			return false;
		}
		break;

	}

	return true;
}

//--------------------------------------------------------------------------//
//	

CQSCRIPTPROGRAM(TM8_BlowUpSendlor, TM8_BlowUpSendlor_Save,0);

void TM8_BlowUpSendlor::Initialize (U32 eventFlags, const MPartRef & part)
{
}

bool TM8_BlowUpSendlor::Update (void)
{
	if(data.mission_over == true)
	{
		return false;
	}

	MPartRef target = M_ GetFirstPart();

	while (target.isValid())
	{
		if(M_ IsPlatform(target) && target->playerID == MANTIS_ID && target->systemID == SENDLOR)
		{
			M_ DestroyPart(target);
		}
		target = M_ GetNextPart(target);
	}

	return false;
}

//--------------------------------------------------------------------------//
//	

CQSCRIPTPROGRAM(TM8_FinishOffTalosWave, TM8_FinishOffTalosWave_Save,0);

void TM8_FinishOffTalosWave::Initialize (U32 eventFlags, const MPartRef & part)
{
	state = BlackwellSpeaks;
}

bool TM8_FinishOffTalosWave::Update (void)
{
	if(data.mission_over == true)
	{
		return false;
	}

	if(data.Frigate1.isValid() && data.Frigate1->systemID != VARN)
	{
		M_ DestroyPart(data.Frigate1);
	}
	if(data.Frigate2.isValid() && data.Frigate2->systemID != VARN)
	{
		M_ DestroyPart(data.Frigate2);
	}
	if(data.Frigate3.isValid() && data.Frigate3->systemID != VARN)
	{
		M_ DestroyPart(data.Frigate3);
	}
	if(data.Frigate4.isValid() && data.Frigate4->systemID != VARN)
	{
		M_ DestroyPart(data.Frigate4);
	}
	if(data.Frigate5.isValid() && data.Frigate5->systemID != VARN)
	{
		M_ DestroyPart(data.Frigate5);
	}
	if(data.Frigate6.isValid() && data.Frigate6->systemID != VARN)
	{
		M_ DestroyPart(data.Frigate6);
	}
	if(data.Frigate7.isValid() && data.Frigate7->systemID != VARN)
	{
		M_ DestroyPart(data.Frigate7);
	}
	if(data.Frigate8.isValid() && data.Frigate8->systemID != VARN)
	{
		M_ DestroyPart(data.Frigate8);
	}
	if(data.Hive1.isValid() && data.Hive1->systemID != VARN)
	{
		M_ DestroyPart(data.Hive1);
	}
	if(data.Hive2.isValid() && data.Hive2->systemID != VARN)
	{
		M_ DestroyPart(data.Hive2);
	}
	if(data.Hive3.isValid() && data.Hive3->systemID != VARN)
	{
		M_ DestroyPart(data.Hive3);
	}
	if(data.Hive4.isValid() && data.Hive4->systemID != VARN)
	{
		M_ DestroyPart(data.Hive4);
	}
	if(data.Scarab1.isValid() && data.Scarab1->systemID != VARN)
	{
		M_ DestroyPart(data.Scarab1);
	}
	if(data.Scarab2.isValid() && data.Scarab2->systemID != VARN)
	{
		M_ DestroyPart(data.Scarab2);
	}
	if(data.Scarab3.isValid() && data.Scarab3->systemID != VARN)
	{
		M_ DestroyPart(data.Scarab3);
	}
	if(data.Scarab4.isValid() && data.Scarab4->systemID != VARN)
	{
		M_ DestroyPart(data.Scarab4);
	}


	if(data.Frigate1.isValid() || data.Frigate2.isValid() || data.Frigate3.isValid() || data.Frigate4.isValid() || data.Frigate5.isValid() || data.Frigate6.isValid() || data.Frigate7.isValid() || data.Frigate8.isValid() || data.Hive1.isValid() || data.Hive2.isValid() || data.Hive3.isValid() || data.Hive4.isValid() || data.Scarab1.isValid() || data.Scarab2.isValid() || data.Scarab3.isValid() || data.Scarab4.isValid())
	{
		if(data.Blackwell.isValid())
		{
			if(data.Blackwell->hullPoints < 1200)
			{
				M_ SetHullPoints(data.Blackwell, 1200);
			}
		}

		if(data.Elan.isValid())
		{
			if(data.Elan->hullPoints < 700)
			{
				M_ SetHullPoints(data.Elan, 700);
			}
		}
	}
	else
	{
		if(CountObjects(M_COCOON, MANTIS_ID, VARN) != 0)
		{
			if(CountObjects(M_COLLECTOR, MANTIS_ID, AEON) != 0 || CountObjects(M_PLANTATION, MANTIS_ID, AEON) || CountObjects(M_COLLECTOR, MANTIS_ID, ARCTURUS) != 0 || CountObjects(M_PLANTATION, MANTIS_ID, ARCTURUS) || CountObjects(M_COLLECTOR, MANTIS_ID, KALIDON) != 0 || CountObjects(M_PLANTATION, MANTIS_ID, KALIDON) || CountObjects(M_COLLECTOR, MANTIS_ID, GRAAN) != 0 || CountObjects(M_PLANTATION, MANTIS_ID, GRAAN))
			{
				switch(state)
				{
					case BlackwellSpeaks:
						if(data.BlackwellDone)
						{
	//						data.mhandle = M_ PlayAudio("M08BL24.wav", data.Blackwell);
							data.mhandle = M_ PlayAnimatedMessage("M08BL24.wav", "Animate!!Blackwell2", MagpieLeft, MagpieTop, data.Blackwell, IDS_TM8_SUB_M08BL24);

							state = Benson;
						}
						break;

					case Benson:

						if(!M_ IsStreamPlaying(data.mhandle))
						{
//							data.mhandle = M_ PlayAudio("M08BN22.wav", data.Benson);
							data.mhandle = M_ PlayAnimatedMessage("M08BN25.wav", "Animate!!Benson2", MagpieLeft, MagpieTop, data.Benson, IDS_TM8_SUB_M08BN25);

							state = KerTak;
						}
	
						break;

					case KerTak:

						if(!M_ IsStreamPlaying(data.mhandle))
						{
//							data.mhandle = M_ PlayAudio("M08KR23.wav", data.KerTak);
							data.mhandle = M_ PlayAnimatedMessage("M08KR26.wav", "Animate!!KerTak2", MagpieLeft, MagpieTop, data.KerTak, IDS_TM8_SUB_M08KR26);

							state = Done;	
						}

						break;

					case Done:

						if(!M_ IsStreamPlaying(data.mhandle))
						{
//							data.mhandle = M_ PlayAudio("M08BN24.wav", data.Benson);
							data.mhandle = M_ PlayAnimatedMessage("M08BN27.wav", "Animate!!Benson2", MagpieLeft, MagpieTop, data.Benson, IDS_TM8_SUB_M08BN27);

							M_ RunProgramByName("TM8_ClearOutVarn", MPartRef());

							return false;
						}

						break;
				}

			}
			else
			{
//				data.mhandle = M_ PlayAudio("M08BN24.wav", data.Benson);
				data.mhandle = M_ PlayAnimatedMessage("M08BN27.wav", "Animate!!Benson2", MagpieLeft, MagpieTop, data.Benson, IDS_TM8_SUB_M08BN27);

				M_ RunProgramByName("TM8_ClearOutVarn", MPartRef());

				return false;
			}
		}
		else
		{
			M_ RunProgramByName("TM8_MissionSuccess", MPartRef());

			return false;
		}

	}

	return true;
}

//--------------------------------------------------------------------------//
//	

CQSCRIPTPROGRAM(TM8_ClearOutVarn, TM8_ClearOutVarn_Save,0);

void TM8_ClearOutVarn::Initialize (U32 eventFlags, const MPartRef & part)
{

}

bool TM8_ClearOutVarn::Update (void)
{
	if(data.mission_over == true)
	{
		return false;
	}

	if(CountObjects(M_COCOON, MANTIS_ID, VARN) == 0)
	{
		M_ RunProgramByName("TM8_MissionSuccess", MPartRef());

		return false;
	}

	return true;
}

//-----------------------------------------------------------------------
// UNDER ATTACK EVENT

CQSCRIPTPROGRAM(TM8_UnderAttack, TM8_UnderAttack_Save, CQPROGFLAG_UNITHIT);

void TM8_UnderAttack::Initialize (U32 eventFlags, const MPartRef & part)
{
	if(data.mission_over)
	{
		return;
	}

	if(part->playerID == MANTIS_ID && data.HasSendlorAttacked == false && part->systemID == KRACUS && part->mObjClass != M_JUMPPLAT)
	{
		data.HasSendlorAttacked = true;
		M_ RunProgramByName("TM8_SendlorWaveAttack", MPartRef());
	}
}

bool TM8_UnderAttack::Update (void)
{
	return false;
}

//-----------------------------------------------------------------------
// OBJECT DESTRUCTION TRIGGER

CQSCRIPTPROGRAM(TM8_ObjectDestroyed, TM8_ObjectDestroyed_Save,CQPROGFLAG_OBJECTDESTROYED);

void TM8_ObjectDestroyed::Initialize (U32 eventFlags, const MPartRef & part)
{
	if(data.mission_over)
	{
		return;
	}

	if(part->playerID == PLAYER_ID && !M_ PlayerHasPlatforms(PLAYER_ID))
	{
		M_ RunProgramByName("TM8_ForwardBaseLost", MPartRef());
	}

	if(data.HasSendlorAttacked == false && part->systemID == KRACUS && part->mObjClass != M_JUMPPLAT && part->playerID == MANTIS_ID)
	{
		data.HasSendlorAttacked = true;
		M_ RunProgramByName("TM8_SendlorWaveAttack", MPartRef());
	}
	
	switch (part->mObjClass)
	{
			
		case M_JUMPPLAT:

			if(data.HasKracusAttacked == false && (part->systemID == GRAAN || M_ GetJumpgateDestination(part) == GRAAN) && part->playerID == MANTIS_ID)
			{
				data.HasKracusAttacked = true;
				M_ RunProgramByName("TM8_KracusWaveAttack", MPartRef());
			}

			break;

		case M_CORVETTE:
			if(part == data.Blackwell)
			{
				M_ FlushStreams();
        	    MScript::MoveCamera( part, 0, MOVIE_CAMERA_JUMP_TO);

//				data.mhandle = M_ PlayAudio("M08BN33.wav", data.Benson);
				data.mhandle = M_ PlayAnimatedMessage("M08BN33.wav", "Animate!!Benson2", MagpieLeft, MagpieTop, data.Benson, IDS_TM8_SUB_M08BN33);

				data.failureID = IDS_TM8_FAIL_BLACKWELL_LOST;
				M_ RunProgramByName("TM8_MissionFailure", MPartRef());

			}
			break;

		case M_BATTLESHIP:

			if(part == data.Benson)
			{
        	    MScript::MoveCamera( part, 0, MOVIE_CAMERA_JUMP_TO);

				M_ RunProgramByName("TM8_BensonKilled", MPartRef());
			}

			break;

		case M_HIVECARRIER:

			if(part == data.KerTak)
			{
        	    MScript::MoveCamera( part, 0, MOVIE_CAMERA_JUMP_TO);

				M_ RunProgramByName("TM8_KerTakKilled", MPartRef());
			}

			break;

		case M_COCOON:
			
			if(data.HasTalosAttacked == false && part->systemID == VARN)
			{
				M_ RunProgramByName("TM8_TalosWave", MPartRef());
				data.HasTalosAttacked = true;
			}

			break;


	}
}
bool TM8_ObjectDestroyed::Update (void)
{


	return false;
}


//---------------------------------------------------------------------------
// MISSION FAILURE PROGRAMS

CQSCRIPTPROGRAM(TM8_KerTakKilled, TM8_KerTakKilled_Save, 0);

void TM8_KerTakKilled::Initialize (U32 eventFlags, const MPartRef & part)
{
	data.mission_over = true;
	M_ MarkObjectiveFailed(IDS_TM8_OBJECTIVE5);

	state = Begin;

	M_ FlushStreams();
}

bool TM8_KerTakKilled::Update (void)
{
	if (!M_ IsStreamPlaying(data.mhandle) && state == Begin)
	{
//		data.mhandle = M_ PlayAudio("M08BN27.wav", data.Benson);
		data.mhandle = M_ PlayAnimatedMessage("M08BN30.wav", "Animate!!Benson2", MagpieLeft, MagpieTop, data.Benson, IDS_TM8_SUB_M08BN30);

		state = Uplink;
	}

	if (!M_ IsStreamPlaying(data.mhandle) && state == Uplink)
	{
		data.shandle = M_ PlayAudio("high_level_data_uplink.wav");

		state = Done;

	}

	if (!M_ IsStreamPlaying(data.shandle) && state == Done)
	{
//		data.mhandle = M_ PlayAudio("M08HA31.wav");
		data.mhandle = M_ PlayAnimatedMessage("M08HA31.wav", "Animate!!Halsey2", MagpieLeft, MagpieTop, IDS_TM8_SUB_M08HA31);

		data.failureID = IDS_TM8_FAIL_KERTAK_LOST;
		M_ RunProgramByName("TM8_MissionFailure", MPartRef ());


		return false;
	}

	return true;
}


CQSCRIPTPROGRAM(TM8_BensonKilled, TM8_BensonKilled_Save, 0);

void TM8_BensonKilled::Initialize (U32 eventFlags, const MPartRef & part)
{
	data.mission_over = true;
	M_ MarkObjectiveFailed(IDS_TM8_OBJECTIVE5);

	state = Begin;

	M_ FlushStreams();
}

bool TM8_BensonKilled::Update (void)
{
	if (!M_ IsStreamPlaying(data.mhandle) && state == Begin)
	{
//		data.mhandle = M_ PlayAudio("M08HA25.wav");
		data.mhandle = M_ PlayAnimatedMessage("M08KR28.wav", "Animate!!KerTak2", MagpieLeft, MagpieTop, IDS_TM8_SUB_M08KR28);

		state = Uplink;
	}

	if (!M_ IsStreamPlaying(data.mhandle) && state == Uplink)
	{
		data.shandle = M_ PlayAudio("high_level_data_uplink.wav");

		state = Done;

	}

	if (!M_ IsStreamPlaying(data.shandle) && state == Done)
	{
//		data.mhandle = M_ PlayAudio("M08HA29.wav");
		data.mhandle = M_ PlayAnimatedMessage("M08HA29.wav", "Animate!!Halsey2", MagpieLeft, MagpieTop, IDS_TM8_SUB_M08HA29);

		data.failureID = IDS_TM8_FAIL_BENSON_LOST;
		M_ RunProgramByName("TM8_MissionFailure", MPartRef ());

		return false;
	}

	return true;
}

CQSCRIPTPROGRAM(TM8_ForwardBaseLost, TM8_ForwardBaseLost_Save, 0);

void TM8_ForwardBaseLost::Initialize (U32 eventFlags, const MPartRef & part)
{
	data.mission_over = true;

	M_ FlushStreams();
}

bool TM8_ForwardBaseLost::Update (void)
{

	if (!M_ IsStreamPlaying(data.mhandle))
	{
//		data.mhandle = M_ PlayAudio("M08HA29.wav");
		data.mhandle = M_ PlayAnimatedMessage("M08HA29.wav", "Animate!!Halsey2", MagpieLeft, MagpieTop, IDS_TM8_SUB_M08HA29);

		M_ RunProgramByName("TM8_MissionFailure", MPartRef ());

		return false;
	}

	return true;
}


//---------------------------------------------------------------------------
//

#define TELETYPE_TIME 3000

CQSCRIPTPROGRAM(TM8_MissionFailure, TM8_MissionFailure_Save, 0);

void TM8_MissionFailure::Initialize (U32 eventFlags, const MPartRef & part)
{
	M_ PlayMusic(DEFEAT);

	M_ FlushStreams();
	M_ EnableMovieMode(true);
	UnderMissionControl();	

	data.mission_over = true;

	state = Begin;
}

bool TM8_MissionFailure::Update (void)
{
	switch (state)
	{
		case Begin:
			if (!M_ IsTeletypePlaying(data.mhandle))
			{
				M_ FlushTeletype();
				data.mhandle = TeletypeMissionOver(IDS_TM8_MISSION_FAILURE,data.failureID);

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


CQSCRIPTPROGRAM(TM8_MissionSuccess, TM8_MissionSuccess_Save,0);

void TM8_MissionSuccess::Initialize (U32 eventFlags, const MPartRef & part)
{
	M_ PlayMusic(VICTORY);

	M_ FlushStreams();
	M_ EnableMovieMode(true);

	UnderMissionControl();

	data.mission_over = true;

	state = Begin;
}

bool TM8_MissionSuccess::Update (void)
{

	if(state == Begin)
	{
		data.shandle = M_ PlayAudio("high_level_data_uplink.wav");
		state = HalseyTalk;
	}

	if(state == HalseyTalk && !M_ IsStreamPlaying(data.shandle))
	{
//		data.mhandle = M_ PlayAudio("M08HA33.wav");
		data.mhandle = M_ PlayAnimatedMessage("M08HA33.wav", "Animate!!Halsey2", MagpieLeft, MagpieTop, IDS_TM8_SUB_M08HA33);

		state = BensonTalk;
	}

	if(state == BensonTalk && !M_ IsStreamPlaying(data.mhandle))
	{
//		data.mhandle = M_ PlayAudio("M08BL34.wav", data.Blackwell);
		data.mhandle = M_ PlayAnimatedMessage("M08BL34.wav", "Animate!!Blackwell2", MagpieLeft, MagpieTop, data.Blackwell, IDS_TM8_SUB_M08BL34);

		state = Halsey2;
	}

	if(state == Halsey2 && !M_ IsStreamPlaying(data.mhandle))
	{
//		data.mhandle = M_ PlayAudio("M08HA35.wav");
		data.mhandle = M_ PlayAnimatedMessage("M08HA35.wav", "Animate!!Halsey2", MagpieLeft, MagpieTop, IDS_TM8_SUB_M08HA35);

		state = PrintSuccess;
	}

	if(state == PrintSuccess && !M_ IsStreamPlaying(data.mhandle))
	{
		M_ FlushTeletype();
		data.mhandle = TeletypeMissionOver(IDS_TM8_MISSION_SUCCESS);
		state = Done;
	}

	if(state == Done && !M_ IsTeletypePlaying(data.mhandle))
	{
		UnderPlayerControl();
		M_ EndMissionVictory(7);
		return false;
	}

	return true;

}


//--------------------------------------------------------------------------//
//--------------------------------End Script08T.cpp-------------------------//
//--------------------------------------------------------------------------//
