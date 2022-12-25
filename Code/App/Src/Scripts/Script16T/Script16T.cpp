//--------------------------------------------------------------------------//
//                                                                          //
//                                Script11T.cpp                             //
//				/Conquest/App/Src/Scripts/Script15T/Script15T.cpp			//
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

	M_ EnableEnemyAI(MANTIS_ID, bOn, "MANTIS_FORTRESS");

	if (bOn)
	{
		AIPersonality airules;
		airules.difficulty = HARD;
		airules.buildMask.bBuildPlatforms = true;
		airules.buildMask.bBuildLightGunboats = true;
		airules.buildMask.bBuildMediumGunboats = true;
		airules.buildMask.bBuildHeavyGunboats = true;
		airules.buildMask.bHarvest = true;
		airules.buildMask.bScout = true;
		airules.buildMask.bBuildAdmirals = false;
		airules.buildMask.bLaunchOffensives = true;
		airules.nGunboatsPerSupplyShip = 3;
		airules.nNumMinelayers = 2;
		airules.uNumScouts = 3;
		airules.nNumFabricators = 2;
		airules.fHarvestersPerRefinery = 2;
		airules.nMaxHarvesters = 5;
		airules.nPanicThreshold = 2000;

		M_ SetEnemyAIRules(MANTIS_ID, airules);
	}

}

void ToggleBuildUnits(bool bOn)
{

	AIPersonality airules;

	airules.difficulty = HARD;
	airules.buildMask.bBuildPlatforms = true;
	airules.buildMask.bBuildLightGunboats = bOn;
	airules.buildMask.bBuildMediumGunboats = bOn;
	airules.buildMask.bBuildHeavyGunboats = bOn;
	airules.buildMask.bHarvest = true;
	airules.buildMask.bScout = true;
	airules.buildMask.bBuildAdmirals = false;
	airules.buildMask.bLaunchOffensives = true;
	airules.nGunboatsPerSupplyShip = 15;
	airules.nNumMinelayers = 2;
	airules.uNumScouts = 3;
	airules.nNumFabricators = 2;
	airules.fHarvestersPerRefinery = 2;
	airules.nMaxHarvesters = 5;
	airules.nPanicThreshold = 2000;

	M_ SetEnemyAIRules(MANTIS_ID, airules);
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

void TweakMantis()
{
	U32 gas, metal, crew;

	gas = M_ GetGas(MANTIS_ID);
	metal = M_ GetMetal(MANTIS_ID);
	crew = M_ GetCrew(MANTIS_ID);

	if (gas < 240) gas = 240;
	if (metal < 240) metal = 240;
	if (crew < 200) crew = 200;

	M_ SetGas(MANTIS_ID, gas);
	M_ SetMetal(MANTIS_ID, metal);
	M_ SetCrew(MANTIS_ID, crew);
}

MPartRef GetGate(MPartRef Wormhole)
{
	MPartRef part;

	part = M_ GetFirstPart();

	while (part.isValid())
	{
		if(part->systemID == VORAAK && part->playerID == PLAYER_ID && M_ DistanceTo(Wormhole, part) < 1)
		{
			return part;
		}
		part = M_ GetNextPart(part);
	}
	return part;
}
//--------------------------------------------------------------------------
//	MISSION BRIEFING
//--------------------------------------------------------------------------

CQSCRIPTPROGRAM(TM16_Briefing, TM16_Briefing_Save,CQPROGFLAG_STARTBRIEFING);

void TM16_Briefing::Initialize (U32 eventFlags, const MPartRef & part)
{
	data.failureID = 0;
	data.mhandle = 0;
	data.shandle = 0;
	data.mission_state = Briefing;
	state = Begin;

	M_ PlayMusic(NULL);
	M_ EnableBriefingControls(true);
}

bool TM16_Briefing::Update (void)
{
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
			data.mhandle = TeletypeBriefing(IDS_TM16_TELETYPE_LOCATION, false);

			state = Radio;

			break;

		case Radio:

			if (!M_ IsStreamPlaying(data.mhandle))
			{
				data.mhandle = M_ PlayAudio("prolog16.wav");
				MScript::BriefingSubtitle(data.mhandle,IDS_TM16_SUB_PROLOG16);
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

			if (M_ IsCustomBriefingAnimationDone(0))
			{
				strcpy(slotAnim.szFileName, "m16ha05.wav");
				strcpy(slotAnim.szTypeName, "Animate!!Halsey");
				slotAnim.slotID = 0;
				slotAnim.bHighlite = false;
				data.mhandle = M_ PlayBriefingTalkingHead(slotAnim);
				MScript::BriefingSubtitle(data.mhandle,IDS_TM16_SUB_M16HA05);



				//				M_ FlushTeletype();
//				data.mhandle = M_ PlayBriefingTeletype(IDS_TM16_HALSEY_M16HA02, MOTextColor, 6000, 1000, false);
				state = MO;
			}

			break;

		case MO:
			
			if (!M_ IsStreamPlaying(data.mhandle))
			{
				M_ FlushTeletype();

				data.mhandle = TeletypeBriefing(IDS_TM16_OBJECTIVES, true);
				state = Finish;
			}

			break;
			
		case Finish:
			if (!M_ IsTeletypePlaying(data.mhandle))
				{
					return false;
				}
			
			break;

	
	}

	return true;
}



//--------------------------------------------------------------------------

CQSCRIPTPROGRAM(TM16_MissionStart, TM16_MissionStart_Save,CQPROGFLAG_STARTMISSION);

void TM16_MissionStart::Initialize (U32 eventFlags, const MPartRef & part)
{

	data.mission_state = MissionStart;
	state = Begin;

	data.HalseyDead = false;

	M_ SetMissionName(IDS_TM16_MISSION_NAME);
	M_ SetMissionID(15);

	M_ SetEnemyCharacter(MANTIS_ID, IDS_TM16_MANTIS);

	M_ SetMissionDescription(IDS_TM16_MISSION_DESC);

	M_ EnableRegenMode(true);

	M_ AddToObjectiveList(IDS_TM16_OBJECTIVE1);
//	M_ AddToObjectiveList(IDS_TM16_OBJECTIVE2);
	M_ AddToObjectiveList(IDS_TM16_OBJECTIVE3);

	//Set up player Tech lvls

	data.missionTech.InitLevel( TECHTREE::FULL_TREE );

	data.missionTech.race[0].build = data.missionTech.race[1].build = (TECHTREE::BUILDNODE) (
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
		TECHTREE::TDEPEND_DISPLACEMENT
        );

    data.missionTech.race[1].build = (TECHTREE::BUILDNODE) (
        data.missionTech.race[1].build |
		TECHTREE::MDEPEND_PLANTATION |
		TECHTREE::MDEPEND_BIOFORGE );


    // add in necessary upgrades, and troopship

	data.missionTech.race[0].tech = data.missionTech.race[1].tech = (TECHTREE::TECHUPGRADE) (
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
		TECHTREE::T_RES_TROOPSHIP3 |
		TECHTREE::T_RES_MISSLEPACK1 |
		TECHTREE::T_RES_MISSLEPACK2 |
		TECHTREE::T_RES_XCHARGES |
		TECHTREE::T_RES_XPROBE  |
		TECHTREE::T_RES_XCLOAK  |
		TECHTREE::T_RES_XVAMPIRE  |
		TECHTREE::T_RES_XSHIELD
        );

    data.missionTech.race[1].tech = (TECHTREE::TECHUPGRADE) (
        data.missionTech.race[1].tech |
		TECHTREE::M_SHIP_KHAMIR );

	data.missionTech.race[0].common_extra = data.missionTech.race[1].common_extra = (TECHTREE::COMMON_EXTRA) (
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

	data.missionTech.race[0].common = data.missionTech.race[1].common = (TECHTREE::COMMON) (
		TECHTREE::RES_WEAPONS1  |
		TECHTREE::RES_WEAPONS2  |
		TECHTREE::RES_WEAPONS3  |
		TECHTREE::RES_WEAPONS4  |
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
		TECHTREE::RES_ENGINE4 |
		TECHTREE::RES_SHIELDS1 |
		TECHTREE::RES_SHIELDS2 |
		TECHTREE::RES_SHIELDS3 |
		TECHTREE::RES_SHIELDS4
        );

    MScript::SetAvailiableTech( data.missionTech );

	//Set each sides resources...

	M_ SetMaxCommandPoitns(MANTIS_ID, 175);
	M_ SetMaxCommandPoitns(PLAYER_ID, 125);

	M_ SetGas(PLAYER_ID, 300);
	M_ SetMetal(PLAYER_ID, 300);
	M_ SetCrew(PLAYER_ID, 200);

	M_ SetMaxGas(PLAYER_ID, 300);
	M_ SetMaxMetal(PLAYER_ID, 300);
	M_ SetMaxCrew(PLAYER_ID, 200);

	data.LagoIsDefended = data.LagoHasAttacked = data.OctariusHasAttacked = data.GigaHasAttacked = data.TareHasAttacked = data.GaarHasAttacked = data.VoraakHasAttacked = data.IodeHasAttacked = false;

	M_ SetMaxGas(MANTIS_ID, 240);
	M_ SetMaxMetal(MANTIS_ID, 240);
	M_ SetMaxCrew(MANTIS_ID, 200);

	SetupAI(true);
	MassEnableAI(MANTIS_ID, false, 0);
	MassEnableAI(MANTIS_ID, true, MOG);
	MassEnableAI(MANTIS_ID, true, LAGO);

	data.Vivac = M_ GetPartByName("Vivac");
	M_ EnablePartnameDisplay(data.Vivac, true);

	data.Steele = M_ GetPartByName("Steele");
	M_ EnablePartnameDisplay(data.Steele, true);

	data.Takei = M_ GetPartByName("Takei");
	M_ EnablePartnameDisplay(data.Takei, true);
	
	data.Halsey = M_ GetPartByName("Halsey");
	M_ EnablePartnameDisplay(data.Halsey, true);
	M_ EnableJumpCap(data.Halsey, true);

	data.Blackwell = M_ GetPartByName("Blackwell");
	M_ EnablePartnameDisplay(data.Blackwell, true);

	data.Hawkes = M_ GetPartByName("Hawkes");
	M_ EnablePartnameDisplay(data.Hawkes, true);

	data.Benson = M_ GetPartByName("Benson");
	M_ EnablePartnameDisplay(data.Benson, true);

	data.SuperShip = M_ GetPartByName("Malkors' SuperShip");
	M_ EnableGenerateAllEvents(data.SuperShip, true);
	M_ MakeInvincible(data.SuperShip, true);
	M_ EnablePartnameDisplay(data.SuperShip, true);
	M_ EnableAIForPart(data.SuperShip, false);
	M_ SetStance(data.SuperShip, US_STOP);
	M_ EnableJumpCap(data.SuperShip, false);

	data.E1 = M_ GetPartByName("Elite Dreadnought ");
	M_ EnablePartnameDisplay(data.E1, true);
	M_ EnableJumpCap(data.E1, true);
	data.E2 = M_ GetPartByName("Elite  Dreadnought ");
	M_ EnablePartnameDisplay(data.E2, true);
	M_ EnableJumpCap(data.E2, true);

	M_ MoveCamera(data.Halsey, 0, MOVIE_CAMERA_JUMP_TO);
	
// Attack target arrays init here...
	data.OctariusTargetArray[0][0] = TARE;
	data.OctariusTargetArray[0][1] = IODE;
	data.OctariusTargetArray[1][0] = false;
	data.OctariusTargetArray[1][1] = false;
	data.OctariusTargetPlanet[0] = M_ GetPartByName("Tare Prime");
	data.OctariusTargetPlanet[1] = M_ GetPartByName("Iode");

	data.GigaTargetArray[0][0] = TARE;
	data.GigaTargetArray[0][1] = GAAR;
	data.GigaTargetArray[1][0] = false;
	data.GigaTargetArray[1][1] = false;
	data.GigaTargetPlanet[0] = M_ GetPartByName("Tare Prime");
	data.GigaTargetPlanet[1] = M_ GetPartByName("Gaar Prime");

	data.GaarTargetArray[0][0] = GIGA;
	data.GaarTargetArray[0][1] = IODE;
	data.GaarTargetArray[1][0] = false;
	data.GaarTargetArray[1][1] = false;
	data.GaarTargetPlanet[0] = M_ GetPartByName("Giga Prime");
	data.GaarTargetPlanet[1] = M_ GetPartByName("Iode");

	data.VoraakTargetArray[0][0] = GAAR;
	data.VoraakTargetArray[0][1] = IODE;
	data.VoraakTargetArray[1][0] = false;
	data.VoraakTargetArray[1][1] = false;
	data.VoraakTargetPlanet[0] = M_ GetPartByName("Gaar Prime");
	data.VoraakTargetPlanet[1] = M_ GetPartByName("Iode");

	data.TareTargetArray[0][0] = GIGA;
	data.TareTargetArray[0][1] = OCTARIUS;
	data.TareTargetArray[1][0] = false;
	data.TareTargetArray[1][1] = false;
	data.TareTargetPlanet[0] = M_ GetPartByName("Giga Prime");
	data.TareTargetPlanet[1] = M_ GetPartByName("Octarius 2");

	data.IodeTargetArray[0][0] = GAAR;
	data.IodeTargetArray[0][1] = OCTARIUS;
	data.IodeTargetArray[1][0] = false;
	data.IodeTargetArray[1][1] = false;
	data.IodeTargetPlanet[0] = M_ GetPartByName("Gaar Prime");
	data.IodeTargetPlanet[1] = M_ GetPartByName("Octarius 2");

	data.ExitPlanetArray[0] = M_ GetPartByName("Iode");
	data.ExitPlanetArray[1] = M_ GetPartByName("Octarius Prime");
	data.ExitPlanetArray[2] = M_ GetPartByName("Tare Prime");
	data.ExitPlanetArray[3] = M_ GetPartByName("Giga Prime");
	data.ExitPlanetArray[4] = M_ GetPartByName("Gaar Prime");
	
	M_ EnableSystem(ALTO, true, true);
	M_ EnableSystem(VORAAK, true, true);

	M_ RunProgramByName("TM16_AIProgram", MPartRef ());
	M_ RunProgramByName("TM16_ProgressCheck", MPartRef ());
	M_ RunProgramByName("TM16_HalseyReturn", MPartRef ());

	state = Begin;
}

bool TM16_MissionStart::Update (void)
{
	if(data.mission_over == true)
	{
		return false;
	}

	switch(state)
	{
		case Begin:

			M_ FlushTeletype();
//			data.mhandle = M_ PlayTeletype(IDS_TM16_HALSEY_M16HA03, SuperLeft, 250, SuperRight, SuperBottom, MOTextColor, 6000, 1000, false);
			data.mhandle = M_ PlayAnimatedMessage("M16HA06.wav", "Animate!!Halsey2", MagpieLeft, MagpieTop, data.Halsey, IDS_TM16_SUB_M16HA06);

			return false;
		break;

	}

	return true;
}

//--------------------------------------------------------------------------//

CQSCRIPTPROGRAM(TM16_AIProgram, TM16_AIProgram_Save, 0);

void TM16_AIProgram::Initialize (U32 eventFlags, const MPartRef & part)
{
	A1 = M_ GetPartByName("Alto Prime");
	A3 = M_ GetPartByName("Alto 3");
}

bool TM16_AIProgram::Update (void)
{
	if(data.mission_over)
		return false;
	
	timer -= ELAPSED_TIME;

	if(A1.isValid() && A3.isValid() && timer <= 0)
	{
		A1Crew = M_ GetCrewOnPlanet(A1);
		A3Crew = M_ GetCrewOnPlanet(A3);

		if(A1Crew < 500)
		{
			M_ SetResourcesOnPlanet(A1, -1, -1, A1Crew + 1);

		}

		if(A3Crew < 1000)
		{
			M_ SetResourcesOnPlanet(A3, -1, -1, A3Crew + 1);

		}

		timer = 3;
	}


	//Set up cases in which the AI will turn off
	if(M_ GetUsedCommandPoints(MANTIS_ID) > 140 || M_ GetUsedCommandPoints(PLAYER_ID) < 30)
	{
		ToggleBuildUnits(false);

		return true;
	}

 //Set up cases in which the AI will turn back on
	if(M_ GetUsedCommandPoints(MANTIS_ID) < 50 || M_ GetUsedCommandPoints(PLAYER_ID) > 110)
	{
		ToggleBuildUnits(true);
	}

	return true;
}

//--------------------------------------------------------------------------//

CQSCRIPTPROGRAM(TM16_ProgressCheck, TM16_ProgressCheck_Save, 0);

void TM16_ProgressCheck::Initialize (U32 eventFlags, const MPartRef & part)
{
	AITrigger1 = AITrigger2 = AITrigger3 = PlayerInVoraak = false;
}

bool TM16_ProgressCheck::Update (void)
{	
	if(data.mission_over)
	{
		return false;
	}
	
	if(PlayerInSystem(PLAYER_ID, VORAAK) && !PlayerInVoraak && M_ IsVisibleToPlayer(data.SuperShip, PLAYER_ID))
	{
		M_ RunProgramByName("TM16_SuperShipSeen", MPartRef());

		MassStance(0, MANTIS_ID, US_ATTACK, 0);

		data.ShipSeen = true;
		PlayerInVoraak = true;
	}

	if(PlayerInSystem(PLAYER_ID, LAGO) && !AITrigger1)
	{
		MassEnableAI(MANTIS_ID, true, TARE);
		MassEnableAI(MANTIS_ID, true, GIGA);

		AITrigger1 = true;
	}

	if((PlayerInSystem(PLAYER_ID, TARE) || PlayerInSystem(PLAYER_ID, GIGA)) && !AITrigger2)
	{
		MassEnableAI(MANTIS_ID, true, IODE);
		MassEnableAI(MANTIS_ID, true, VORAAK);

		M_ EnableGenerateAllEvents(data.SuperShip, true);
		M_ EnableAIForPart(data.SuperShip, false);
		M_ SetStance(data.SuperShip, US_STOP);

		MassEnableAI(MANTIS_ID, true, OCTARIUS);
		MassEnableAI(MANTIS_ID, true, GAAR);

		AITrigger2 = true;
	}

	return true;
}
/*
//--------------------------------------------------------------------------//

CQSCRIPTPROGRAM(TM16_HalseyReturn, TM16_HalseyReturn_Save, 0);

void TM16_HalseyReturn::Initialize (U32 eventFlags, const MPartRef & part)
{
	Return = M_ GetPartByName("HalseyReturn");
}

bool TM16_HalseyReturn::Update (void)
{
	if(data.mission_over == true)
	{
		return false;
	}

	if(data.Halsey->systemID != ALTO && !data.Halsey->bUnselectable)
	{
		if(!M_ IsStreamPlaying(data.mhandle))
		{
			data.mhandle = M_ PlayAnimatedMessage("M16HA07.wav", "Animate!!Halsey2", MagpieLeft, MagpieTop, data.Halsey);
		}
		M_ EnableSelection(data.Halsey, false);

		M_ OrderMoveTo(data.Halsey, Return);
	}
	if(data.Halsey->systemID == ALTO && data.Halsey->bUnselectable)
	{
		M_ EnableSelection(data.Halsey, true);
	}

	if(data.E1.isValid())
	{
		if(data.E1->systemID != ALTO && !data.E1->bUnselectable)
		{
			if(!M_ IsStreamPlaying(data.mhandle))
			{
				data.mhandle = M_ PlayAnimatedMessage("M16HA07.wav", "Animate!!Halsey2", MagpieLeft, MagpieTop, data.Halsey);
			}
			M_ EnableSelection(data.E1, false);

			M_ OrderMoveTo(data.E1, Return);
		}
		if(data.E1->systemID == ALTO && data.E1->bUnselectable)
		{
			M_ EnableSelection(data.E1, true);
		}
	}

	if(data.E2.isValid())
	{
		if(data.E2->systemID != ALTO && !data.E2->bUnselectable)
		{
			if(!M_ IsStreamPlaying(data.mhandle))
			{
				data.mhandle = M_ PlayAnimatedMessage("M16HA07.wav", "Animate!!Halsey2", MagpieLeft, MagpieTop, data.Halsey);
			}
			M_ EnableSelection(data.E2, false);

			M_ OrderMoveTo(data.E2, Return);
		}
		if(data.E2->systemID == ALTO && data.E2->bUnselectable)
		{
			M_ EnableSelection(data.E2, true);
		}
	}


	return true;
}*/
//--------------------------------------------------------------------------//

CQSCRIPTPROGRAM(TM16_SuperShipSeen, TM16_SuperShipSeen_Save, 0);

void TM16_SuperShipSeen::Initialize (U32 eventFlags, const MPartRef & part)
{
	M_ RunProgramByName("TM16_KeepSuperShipAlive", MPartRef());

	MassEnableAI(MANTIS_ID, false, 0);

	M_ SetStance(data.SuperShip, US_ATTACK);
	
	VoraakPrime = M_ GetPartByName("Voraak Prime");

	state = Begin;
}

bool TM16_SuperShipSeen::Update (void)
{
	if(data.mission_over == true)
	{
		return false;
	}

	timer -= ELAPSED_TIME;

	switch(state)
	{
		case Begin:
			if(!M_ IsStreamPlaying(data.mhandle) && !M_ IsStreamPlaying(data.mhandle))
			{
				M_ FlushTeletype();
//				data.mhandle = M_ PlayTeletype(IDS_TM16_HAWKES_M16HW04, SuperLeft, SuperTop, SuperRight, SuperBottom, MOTextColor, 6000, 1000, false);
				data.mhandle = M_ PlayAnimatedMessage("M16HW08.wav", "Animate!!Hawkes2", MagpieLeft, MagpieTop, data.Hawkes, IDS_TM16_SUB_M16HW08);
			
				state = Blackwell;
			}
			break;

		case Blackwell:
			if(!M_ IsStreamPlaying(data.mhandle))
			{
				M_ FlushTeletype();
//				data.mhandle = M_ PlayTeletype(IDS_TM16_BLACKWELL_M16BL05, SuperLeft, SuperTop, SuperRight, SuperBottom, MOTextColor, 6000, 1000, false);
				data.mhandle = M_ PlayAnimatedMessage("M16BL09.wav", "Animate!!Blackwell2", MagpieLeft, MagpieTop, data.Blackwell, IDS_TM16_SUB_M16BL09);

				M_ SetEnemyAITarget(MANTIS_ID, VoraakPrime, 50, VORAAK);
				target = M_ FindNearestEnemy(data.SuperShip, true, false);
				MassAttack(0, MANTIS_ID, target, VORAAK);
			
				state = Hawkes;
			}
			break;

		case Hawkes:
			if(!M_ IsStreamPlaying(data.mhandle))
			{
				data.mhandle = M_ PlayAnimatedMessage("M16HW10.wav", "Animate!!Hawkes2", MagpieLeft, MagpieTop, data.Hawkes, IDS_TM16_SUB_M16HW10);

				state = Malkor;
			}
			break;

		case Malkor:
			if(!M_ IsStreamPlaying(data.mhandle))
			{
				M_ FlushTeletype();
//				data.mhandle = M_ PlayTeletype(IDS_TM16_MALKOR_M16ML07, SuperLeft, SuperTop, SuperRight, SuperBottom, MOTextColor, 6000, 1000, false);
				data.mhandle = M_ PlayAnimatedMessage("M16MA11.wav", "Animate!!Malkor2", MagpieLeft, MagpieTop, IDS_TM16_SUB_M16MA11);
			
				SetupAI(true);
				state = Halsey;
			}
			break;

		case Halsey:
			if(!M_ IsStreamPlaying(data.mhandle))
			{
				M_ FlushTeletype();
//				data.mhandle = M_ PlayTeletype(IDS_TM16_HALSEY_M16HA08, SuperLeft, SuperTop, SuperRight, SuperBottom, MOTextColor, 6000, 1000, false);
				data.mhandle = M_ PlayAnimatedMessage("M16HA12.wav", "Animate!!Halsey2", MagpieLeft, MagpieTop, data.Halsey, IDS_TM16_SUB_M16HA12);
			
				state = Malkor2;
			}
			break;
	
		case Malkor2:
			if(!M_ IsStreamPlaying(data.mhandle) && data.HasBeenAttacked)
			{
				M_ FlushTeletype();
//				data.mhandle = M_ PlayTeletype(IDS_TM16_MALKOR_M16ML09, SuperLeft, SuperTop, SuperRight, SuperBottom, MOTextColor, 6000, 1000, false);
			
				timer = 30;
				state = Seconds;
			}
			break;

		case Seconds:
			if(timer < 0 && !M_ IsTeletypePlaying(data.mhandle))
			{
				M_ FlushTeletype();
//				data.mhandle = M_ PlayTeletype(IDS_TM16_STEELE_M16ST10, SuperLeft, SuperTop, SuperRight, SuperBottom, MOTextColor, 6000, 1000, false);
				data.mhandle = M_ PlayAnimatedMessage("M16st13.wav", "Animate!!Steele2", MagpieLeft, MagpieTop, data.Steele, IDS_TM16_SUB_M16ST13);

				state = Vivac;
			}
			break;
/*
		case Halsey2:
			if(!M_ IsTeletypePlaying(data.mhandle))
			{
				M_ FlushTeletype();
				data.mhandle = M_ PlayTeletype(IDS_TM16_HALSEY_M16HA08, SuperLeft, SuperTop, SuperRight, SuperBottom, MOTextColor, 6000, 1000, false);
			
				state = Vivac;
			}
			break;
*/
		case Vivac:
			if(!M_ IsStreamPlaying(data.mhandle))
			{
				M_ FlushTeletype();
//				data.mhandle = M_ PlayTeletype(IDS_TM16_VIVAC_M16VV11, SuperLeft, SuperTop, SuperRight, SuperBottom, MOTextColor, 6000, 1000, false);
				data.mhandle = M_ PlayAnimatedMessage("M16vv14.wav", "Animate!!Vivac2", MagpieLeft, MagpieTop, data.Vivac, IDS_TM16_SUB_M16VV14);

				state = Benson;
			}
			break;

		case Benson:
			if(!M_ IsStreamPlaying(data.mhandle))
			{
				M_ FlushTeletype();
//				data.mhandle = M_ PlayTeletype(IDS_TM16_BENSON_M16BN12, SuperLeft, SuperTop, SuperRight, SuperBottom, MOTextColor, 6000, 1000, false);
				data.mhandle = M_ PlayAnimatedMessage("M16BN15.wav", "Animate!!Benson2", MagpieLeft, MagpieTop, data.Benson, IDS_TM16_SUB_M16BN15);
			
				state = Vivac2;
			}
			break;

		case Vivac2:
			if(!M_ IsStreamPlaying(data.mhandle))
			{
				M_ FlushTeletype();
//				data.mhandle = M_ PlayTeletype(IDS_TM16_VIVAC_M16VV13, SuperLeft, SuperTop, SuperRight, SuperBottom, MOTextColor, 6000, 1000, false);
				data.mhandle = M_ PlayAnimatedMessage("M16vv16.wav", "Animate!!Vivac2", MagpieLeft, MagpieTop, data.Vivac, IDS_TM16_SUB_M16VV16);
			
				state = Ion;
			}
			break;

		case Ion:
			if(!M_ IsStreamPlaying(data.mhandle))
			{
				M_ FlushTeletype();
//				data.mhandle = M_ PlayTeletype(IDS_TM16_HALSEY_M16HA14, SuperLeft, SuperTop, SuperRight, SuperBottom, MOTextColor, 6000, 1000, false);
				data.mhandle = M_ PlayAnimatedMessage("M16HA17.wav", "Animate!!Halsey2", MagpieLeft, MagpieTop, data.Halsey, IDS_TM16_SUB_M16HA17);
			
				state = Done;
			}
			break;
			
		case Done:
			if(!M_ IsStreamPlaying(data.mhandle))
			{
				M_ FlushTeletype();
				data.mhandle = TeletypeObjective(IDS_TM16_NEWMO);
			
				data.missionTech.race[0].build = TECHTREE::BUILDNODE( data.missionTech.race[0].build | 
				TECHTREE::TDEPEND_ION_CANNON );

				M_ SetAvailiableTech( data.missionTech );


				M_ AddToObjectiveList(IDS_TM16_OBJECTIVE4);

				return false;
			}
	}
	return true;
}

//--------------------------------------------------------------------------//

CQSCRIPTPROGRAM(TM16_PlayerInAlto, TM16_PlayerInAlto_Save, 0);

void TM16_PlayerInAlto::Initialize (U32 eventFlags, const MPartRef & part)
{
	state = Begin;
}

bool TM16_PlayerInAlto::Update (void)
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
//				data.mhandle = M_ PlayTeletype(IDS_TM16_BENSON_M15BN07, SuperLeft, SuperTop, SuperRight, SuperBottom, MOTextColor, 6000, 1000, false);
	
				state = Next;
			}
			break;

		case Next:
			if(!M_ IsTeletypePlaying(data.mhandle))
			{
				M_ FlushTeletype();
//				data.mhandle = M_ PlayTeletype(IDS_TM16_STEELE_M15ST08, SuperLeft, SuperTop, SuperRight, SuperBottom, MOTextColor, 6000, 1000, false);
	
				state = Done;
			}
			break;

		case Done:
			if(!M_ IsTeletypePlaying(data.mhandle))
			{
				M_ FlushTeletype();
//				data.mhandle = M_ PlayTeletype(IDS_TM16_NEWMO, SuperLeft, SuperTop, SuperRight, SuperBottom, MOTextColor, 6000, 1000, false);
			
//				M_ AddToObjectiveList(IDS_TM16_OBJECTIVE4);
				return false;
			}
			break;
	}

	return true;
}

//--------------------------------------------------------------------------//

CQSCRIPTPROGRAM(TM16_KeepSuperShipAlive, TM16_KeepSuperShipAlive_Save, 0);

void TM16_KeepSuperShipAlive::Initialize (U32 eventFlags, const MPartRef & part)
{
	LeavingTown = FabInTown = false;
	data.HasBeenAttacked = false;
}

bool TM16_KeepSuperShipAlive::Update (void)
{
	if(data.mission_over == true)
	{
		return false;
	}

	timer -= ELAPSED_TIME;
	wavetimer -= ELAPSED_TIME;

	if(!data.SuperShip.isValid() || data.mission_over)
	{
		return false;
	}

	if(CountObjectsInRange(0, PLAYER_ID, data.SuperShip, 5) > 3 && wavetimer < 0)
	{
		M_ OrderSpecialAbility(data.SuperShip);
		wavetimer = 60;
	}

	if(data.SuperShip->hullPoints < 10000)
	{
		M_ SetHullPoints(data.SuperShip, 10000);
		data.HasBeenAttacked = true;
	}

	if(data.SuperShip->supplies < 2500)
	{
		M_ SetSupplies(data.SuperShip, 2500);
	}

	if(CountObjects(M_FABRICATOR, PLAYER_ID, VORAAK) >= 1 && data.ShipSeen && !FabInTown)
	{
		data.PlanetTrigger = M_ CreatePart("MISSION!!TRIGGER",M_ GetPartByName("Voraak Prime"), 0);
		M_ EnableTrigger(data.PlanetTrigger, true);
		M_ SetTriggerFilter(data.PlanetTrigger, M_IONCANNON, TRIGGER_MOBJCLASS, false);
		M_ SetTriggerFilter(data.PlanetTrigger, 0, TRIGGER_NOFORCEREADY, true);
		M_ SetTriggerFilter(data.PlanetTrigger, PLAYER_ID, TRIGGER_PLAYER, true);
		M_ SetTriggerRange(data.PlanetTrigger, 100);
		M_ SetTriggerProgram(data.PlanetTrigger, "TM16_AttackTheCannon");

		timer = 300;
		FabInTown = true;
	}

	if(timer < 0 && FabInTown && !LeavingTown)
	{
		M_ RunProgramByName("TM16_LeaveVoraak", MPartRef());

		M_ SetStance(data.SuperShip, US_STOP);
		M_ OrderCancel(data.SuperShip);

		LeavingTown = true;
	}

	if(data.SuperShip.isValid() && data.SuperShip->systemID != VORAAK)
	{
		M_ RunProgramByName("TM16_MissionFailure", MPartRef());
	}


	return true;
}

CQSCRIPTPROGRAM(TM16_LeaveVoraak, TM16_LeaveVoraak_Save, 0);

void TM16_LeaveVoraak::Initialize (U32 eventFlags, const MPartRef & part)
{
	TargetSystem = rand() % 5;
	M_ EnableJumpCap(data.SuperShip, true);

	GaarWorm = M_ GetPartByName("Wormhole to Gaar (Voraak)");
	GigaWorm = M_ GetPartByName("Wormhole to Giga (Voraak)");
	IodeWorm = M_ GetPartByName("Wormhole to Iode (Voraak)");
	TareWorm = M_ GetPartByName("Wormhole to Tare (Voraak)");
	OctariusWorm = M_ GetPartByName("Wormhole to Octarius (Voraak)");

	timer = 10;
}

bool TM16_LeaveVoraak::Update (void)
{
	if(data.mission_over == true)
	{
		return false;
	}

	timer -= ELAPSED_TIME;

	switch (TargetSystem)
	{
		case 0:
			if(CountObjectsInRange(M_JUMPPLAT, PLAYER_ID, IodeWorm, 2) > 0)
			{
				JumpGate = GetGate(IodeWorm);
				M_ OrderAttack(data.SuperShip, JumpGate);

				TargetHolder = TargetSystem;
				TargetSystem = 5;
			}
			else if(timer < 0)
			{
				M_ OrderMoveTo(data.SuperShip, data.ExitPlanetArray[TargetSystem]);
				timer = 15;
			}
			break;

		case 1:
			if(CountObjectsInRange(M_JUMPPLAT, PLAYER_ID, OctariusWorm, 2) > 0)
			{
				JumpGate = GetGate(OctariusWorm);
				M_ OrderAttack(data.SuperShip, JumpGate);

				TargetHolder = TargetSystem;
				TargetSystem = 5;
			}
			else if(timer < 0)
			{
				M_ OrderMoveTo(data.SuperShip, data.ExitPlanetArray[TargetSystem]);
				timer = 15;
			}
			break;

		case 2:
			if(CountObjectsInRange(M_JUMPPLAT, PLAYER_ID, TareWorm, 2) > 0)
			{
				JumpGate = GetGate(TareWorm);
				M_ OrderAttack(data.SuperShip, JumpGate);

				TargetHolder = TargetSystem;
				TargetSystem = 5;
			}
			else if(timer < 0)
			{
				M_ OrderMoveTo(data.SuperShip, data.ExitPlanetArray[TargetSystem]);
				timer = 15;
			}
			break;

		case 3:
			if(CountObjectsInRange(M_JUMPPLAT, PLAYER_ID, GigaWorm, 2) > 0)
			{
				JumpGate = GetGate(GigaWorm);
				M_ OrderAttack(data.SuperShip, JumpGate);

				TargetHolder = TargetSystem;
				TargetSystem = 5;
			}
			else if(timer < 0)
			{
				M_ OrderMoveTo(data.SuperShip, data.ExitPlanetArray[TargetSystem]);
				timer = 15;
			}
			break;

		case 4:
			if(CountObjectsInRange(M_JUMPPLAT, PLAYER_ID, GaarWorm, 2) > 0)
			{
				JumpGate = GetGate(GaarWorm);
				M_ OrderAttack(data.SuperShip, JumpGate);

				TargetHolder = TargetSystem;
				TargetSystem = 5;
			}
			else if(timer < 0)
			{
				M_ OrderMoveTo(data.SuperShip, data.ExitPlanetArray[TargetSystem]);
				timer = 15;
			}
			break;

		case 5:
			if(!JumpGate.isValid())
			{
				TargetSystem = TargetHolder;
				timer = -1;
			}
			else if(timer < 0)
			{
				M_ OrderAttack(data.SuperShip, JumpGate);
				timer = 10;
			}
			break;

	}



	return true;
}


//--------------------------------------------------------------------------//

CQSCRIPTPROGRAM(TM16_AttackTheCannon, TM16_AttackTheCannon_Save, 0);

void TM16_AttackTheCannon::Initialize (U32 eventFlags, const MPartRef & part)
{
	target = M_ GetLastTriggerObject(data.PlanetTrigger);

	MassAttack(0, MANTIS_ID, target);
	M_ OrderCancel(data.SuperShip);
}

bool TM16_AttackTheCannon::Update (void)
{
	if(data.mission_over == true)
	{
		return false;
	}

	if(target.isValid())
	{
		M_ EnableTrigger(data.PlanetTrigger, false);

		return true;
	}
	else
	{
		M_ EnableTrigger(data.PlanetTrigger, true);

		return false;
	}
}

//--------------------------------------------------------------------------//

CQSCRIPTPROGRAM(TM16_LagoAttack, TM16_LagoAttack_Save, 0);

void TM16_LagoAttack::Initialize (U32 eventFlags, const MPartRef & part)
{
	timer = 0;

	Spawn = M_ GetPartByName("LagoSpawn");
	MogPrime = M_ GetPartByName("Mog Prime");
}

bool TM16_LagoAttack::Update (void)
{
	if(data.mission_over == true)
	{
		return false;
	}

	timer -= ELAPSED_TIME;

	if(timer < 0 && CountObjectsInRange(0, PLAYER_ID, Spawn, 5) == 0 && M_ GetUsedCommandPoints(MANTIS_ID) < M_ GetMaxCommandPoints(MANTIS_ID))
	{
		if(CountObjects(M_NIAD, MANTIS_ID, LAGO) > 0)
		{
			if(PlayerInSystem(PLAYER_ID, MOG))
			{
				for (int x=0; x<3; x++)
				{
					M_ CreatePart("GBOAT!!M_Scarab", Spawn, MANTIS_ID);
				}

				M_ SetEnemyAITarget(MANTIS_ID, MogPrime, 50, MOG);
				M_ LaunchOffensive(MANTIS_ID, US_ATTACK);

				timer = 300;
			}
		}
		else
			return false;
	}

	return true;
}

//--------------------------------------------------------------------------//

CQSCRIPTPROGRAM(TM16_OctariusAttack, TM16_OctariusAttack_Save, 0);

void TM16_OctariusAttack::Initialize (U32 eventFlags, const MPartRef & part)
{
	timer = 0;

	Spawn = M_ GetPartByName("OctariusSpawn");
}

bool TM16_OctariusAttack::Update (void)
{
	if(data.mission_over == true)
	{
		return false;
	}

	timer -= ELAPSED_TIME;

	if(timer < 0 && CountObjectsInRange(0, PLAYER_ID, Spawn, 5) == 0 && M_ GetUsedCommandPoints(MANTIS_ID) < M_ GetMaxCommandPoints(MANTIS_ID))
	{
		if(CountObjects(M_COCOON, MANTIS_ID, OCTARIUS) > 0)
		{
			if(!PlayerInSystem(PLAYER_ID, TARE) && !PlayerInSystem(PLAYER_ID, IODE)) //if player isnt in the systems under attack, dont attack
			{
				return true;
			}

			if(data.OctariusTargetArray[1][0] && data.OctariusTargetArray[1][1])
				Target = rand() % 2;
			else
			{
				if(data.OctariusTargetArray[1][0])
					Target = 0;
				else
					Target = 1;
			}

			for (int x=0; x<3; x++)
			{
				M_ CreatePart("GBOAT!!M_Scarab", Spawn, MANTIS_ID);
			}
			
			M_ SetEnemyAITarget(MANTIS_ID, data.OctariusTargetPlanet[Target], 50, data.OctariusTargetArray[0][Target]);
			M_ LaunchOffensive(MANTIS_ID, US_ATTACK);

			timer = 300;
		}
		else
			return false;
	}

	return true;
}

//--------------------------------------------------------------------------//

CQSCRIPTPROGRAM(TM16_GigaAttack, TM16_GigaAttack_Save, 0);

void TM16_GigaAttack::Initialize (U32 eventFlags, const MPartRef & part)
{
	timer = 0;

	Spawn = M_ GetPartByName("GigaSpawn");
}

bool TM16_GigaAttack::Update (void)
{
	if(data.mission_over == true)
	{
		return false;
	}

	timer -= ELAPSED_TIME;

	if(timer < 0 && CountObjectsInRange(0, PLAYER_ID, Spawn, 5) == 0 && M_ GetUsedCommandPoints(MANTIS_ID) < M_ GetMaxCommandPoints(MANTIS_ID))
	{
		if(CountObjects(M_NIAD, MANTIS_ID, GIGA) > 0)
		{
			if(!PlayerInSystem(PLAYER_ID, TARE) && !PlayerInSystem(PLAYER_ID, GAAR)) //if player isnt in the systems under attack, dont attack
			{
				return true;
			}

			if(data.GigaTargetArray[1][0] && data.GigaTargetArray[1][1])
				Target = rand() % 2;
			else
			{
				if(data.GigaTargetArray[1][0])
					Target = 0;
				else
					Target = 1;
			}

			for (int x=0; x<3; x++)
			{
				M_ CreatePart("GBOAT!!M_Scarab", Spawn, MANTIS_ID);
			}
			
			M_ SetEnemyAITarget(MANTIS_ID, data.GigaTargetPlanet[Target], 50, data.GigaTargetArray[0][Target]);
			M_ LaunchOffensive(MANTIS_ID, US_ATTACK);

			timer = 300;
		}
		else
			return false;
	}

	return true;
}

//--------------------------------------------------------------------------//

CQSCRIPTPROGRAM(TM16_TareAttack, TM16_TareAttack_Save, 0);

void TM16_TareAttack::Initialize (U32 eventFlags, const MPartRef & part)
{
	Spawn = M_ GetPartByName("TareSpawn");
	GigaPrime = M_ GetPartByName("Giga Prime");
}

bool TM16_TareAttack::Update (void)
{
	if(data.mission_over == true)
	{
		return false;
	}

	timer -= ELAPSED_TIME;

	if(timer < 0 && CountObjectsInRange(0, PLAYER_ID, Spawn, 5) == 0 && M_ GetUsedCommandPoints(MANTIS_ID) < M_ GetMaxCommandPoints(MANTIS_ID))
	{
		if(CountObjects(M_NIAD, MANTIS_ID, TARE) > 0)
		{
			if(PlayerInSystem(PLAYER_ID, GIGA))
			{
				for (int x=0; x<3; x++)
				{
					M_ CreatePart("GBOAT!!M_Scarab", Spawn, MANTIS_ID);
				}

				M_ SetEnemyAITarget(MANTIS_ID, GigaPrime, 50, GIGA);
				M_ LaunchOffensive(MANTIS_ID, US_ATTACK);

				timer = 300;
			}
		}
		else
			return false;
	}
	
	return true;
}

//--------------------------------------------------------------------------//

CQSCRIPTPROGRAM(TM16_GaarAttack, TM16_GaarAttack_Save, 0);

void TM16_GaarAttack::Initialize (U32 eventFlags, const MPartRef & part)
{
	timer = 0;

	Spawn = M_ GetPartByName("GaarSpawn");
}

bool TM16_GaarAttack::Update (void)
{
	if(data.mission_over == true)
	{
		return false;
	}

	timer -= ELAPSED_TIME;

	if(timer < 0 && CountObjectsInRange(0, PLAYER_ID, Spawn, 5) == 0 && M_ GetUsedCommandPoints(MANTIS_ID) < M_ GetMaxCommandPoints(MANTIS_ID))
	{
		if(CountObjects(M_NIAD, MANTIS_ID, GAAR) > 0)
		{
			if(!PlayerInSystem(PLAYER_ID, GIGA) && !PlayerInSystem(PLAYER_ID, IODE)) //if player isnt in the systems under attack, dont attack
			{
				return true;
			}

			if(data.GaarTargetArray[1][0] && data.GaarTargetArray[1][1])
				Target = rand() % 2;
			else
			{
				if(data.GaarTargetArray[1][0])
					Target = 0;
				else
					Target = 1;
			}

			for (int x=0; x<3; x++)
			{
				M_ CreatePart("GBOAT!!M_Scarab", Spawn, MANTIS_ID);
			}
			
			M_ SetEnemyAITarget(MANTIS_ID, data.GaarTargetPlanet[Target], 50, data.GaarTargetArray[0][Target]);
			M_ LaunchOffensive(MANTIS_ID, US_ATTACK);

			timer = 300;
		}
		else
			return false;
	}

	return true;
}

//--------------------------------------------------------------------------//

CQSCRIPTPROGRAM(TM16_VoraakAttack, TM16_VoraakAttack_Save, 0);

void TM16_VoraakAttack::Initialize (U32 eventFlags, const MPartRef & part)
{
	timer = 0;

	Spawn = M_ GetPartByName("VoraakSpawn");
}

bool TM16_VoraakAttack::Update (void)
{
	if(data.mission_over == true)
	{
		return false;
	}

	timer -= ELAPSED_TIME;

	if(timer < 0 && CountObjectsInRange(0, PLAYER_ID, Spawn, 5) == 0 && M_ GetUsedCommandPoints(MANTIS_ID) < M_ GetMaxCommandPoints(MANTIS_ID))
	{
		if(CountObjects(M_NIAD, MANTIS_ID, VORAAK) > 0)
		{
			if(!PlayerInSystem(PLAYER_ID, GAAR) && !PlayerInSystem(PLAYER_ID, IODE)) //if player isnt in the systems under attack, dont attack
			{
				return true;
			}

			if(data.VoraakTargetArray[1][0] && data.VoraakTargetArray[1][1])
				Target = rand() % 2;
			else
			{
				if(data.VoraakTargetArray[1][0])
					Target = 0;
				else
					Target = 1;
			}

			for (int x=0; x<3; x++)
			{
				M_ CreatePart("GBOAT!!M_Scarab", Spawn, MANTIS_ID);
			}
			
			M_ SetEnemyAITarget(MANTIS_ID, data.VoraakTargetPlanet[Target], 50, data.VoraakTargetArray[0][Target]);
			M_ LaunchOffensive(MANTIS_ID, US_ATTACK);

			timer = 300;
		}
		else
			return false;
	}

	return true;
}

//--------------------------------------------------------------------------//

CQSCRIPTPROGRAM(TM16_IodeAttack, TM16_IodeAttack_Save, 0);

void TM16_IodeAttack::Initialize (U32 eventFlags, const MPartRef & part)
{
	Spawn = M_ GetPartByName("IodeSpawn");
	GaarPrime = M_ GetPartByName("Gaar Prime");
}

bool TM16_IodeAttack::Update (void)
{
	if(data.mission_over == true)
	{
		return false;
	}

	timer -= ELAPSED_TIME;

	if(timer < 0 && CountObjectsInRange(0, PLAYER_ID, Spawn, 5) == 0 && M_ GetUsedCommandPoints(MANTIS_ID) < M_ GetMaxCommandPoints(MANTIS_ID))
	{
		if(CountObjects(M_NIAD, MANTIS_ID, IODE) > 0)
		{
			if(PlayerInSystem(PLAYER_ID, GAAR))
			{
				for (int x=0; x<3; x++)
				{
					M_ CreatePart("GBOAT!!M_Scarab", Spawn, MANTIS_ID);
				}

				M_ SetEnemyAITarget(MANTIS_ID, GaarPrime, 50, GAAR);
				M_ LaunchOffensive(MANTIS_ID, US_ATTACK);

				timer = 300;
			}
		}
		else
			return false;
	}
	
	return true;
}

//--------------------------------------------------------------------------//

CQSCRIPTPROGRAM(TM16_VoraakAttackedByPlayer, TM16_VoraakAttackedByPlayer_Save, 0);

void TM16_VoraakAttackedByPlayer::Initialize (U32 eventFlags, const MPartRef & part)
{
	MassEnableAI(MANTIS_ID, false, 0);

	Attacked = false;
	
	VoraakPrime = M_ GetPartByName("Voraak Prime");
	timer = 10;
}

bool TM16_VoraakAttackedByPlayer::Update (void)
{
	if(data.mission_over == true)
	{
		return false;
	}

	timer -= ELAPSED_TIME;
 
	if(timer < 0 && !Attacked)
	{
		M_ SetEnemyAITarget(MANTIS_ID, VoraakPrime, 50, VORAAK);
		M_ LaunchOffensive(MANTIS_ID, US_ATTACK);
		
		timer = 20;
		Attacked = true;
	}

	if(timer < 0)
	{
		SetupAI(true);	
		return false;
	}
	
	return true;
}

//-----------------------------------------------------------------------
// UNDER ATTACK EVENT

CQSCRIPTPROGRAM(TM16_Hit, TM16_Hit_Save, CQPROGFLAG_UNITHIT);

void TM16_Hit::Initialize (U32 eventFlags, const MPartRef & part)
{
	if(data.mission_over)
	{
		return;
	}

	if(part == data.SuperShip)
	{
		MPartRef shooter = M_ GetExtendedEventPartRef();

		if(shooter.isValid() && shooter->mObjClass == M_IONCANNON)
		{
			M_ DestroyPart(data.SuperShip);
		}
	}

}

bool TM16_Hit::Update (void)
{
	return false;
}
//-----------------------------------------------------------------------
// OBJECT DESTRUCTION TRIGGER

CQSCRIPTPROGRAM(TM16_ObjectDestroyed, TM16_ObjectDestroyed_Save,CQPROGFLAG_OBJECTDESTROYED);

void TM16_ObjectDestroyed::Initialize (U32 eventFlags, const MPartRef & part)
{
	if(data.mission_over)
	{
		return;
	}

	if(part->systemID == MOG && part->playerID == MANTIS_ID && !data.MogFlag)
	{
		M_ RunProgramByName("TM16_LagoAttack", MPartRef());

		data.MogFlag = true;
	}

	if(part->systemID == TARE && part->playerID == MANTIS_ID && !data.TareFlag)
	{
		if(!data.GigaHasAttacked)
		{
			M_ RunProgramByName("TM16_GigaAttack", MPartRef());

			data.GigaHasAttacked = true;
			data.GigaTargetArray[1][0] = true;
		}
		else
			data.GigaTargetArray[1][0] = true;

		if(!data.OctariusHasAttacked)
		{
			M_ RunProgramByName("TM16_OctariusAttack", MPartRef());

			data.OctariusHasAttacked = true;
			data.OctariusTargetArray[1][0] = true;
		}
		else
			data.OctariusTargetArray[1][0] = true;

		data.TareFlag = true;
	}

	if(part->systemID == GIGA && part->playerID == MANTIS_ID && !data.GigaFlag)
	{
		if(!data.TareHasAttacked)
		{
			M_ RunProgramByName("TM16_TareAttack", MPartRef());

			data.TareHasAttacked = true;
			data.TareTargetArray[1][0] = true;
		}
		else
			data.TareTargetArray[1][0] = true;

		if(!data.GaarHasAttacked)
		{
			M_ RunProgramByName("TM16_GaarAttack", MPartRef());

			data.GaarHasAttacked = true;
			data.GaarTargetArray[1][0] = true;
		}
		else
			data.GaarTargetArray[1][0] = true;

		data.GigaFlag = true;
	}

	if(part->systemID == GAAR && part->playerID == MANTIS_ID && !data.GaarFlag)
	{
		if(!data.IodeHasAttacked)
		{
			M_ RunProgramByName("TM16_IodeAttack", MPartRef());

			data.IodeHasAttacked = true;
			data.IodeTargetArray[1][0] = true;
		}
		else
			data.IodeTargetArray[1][0] = true;

		if(!data.GigaHasAttacked)
		{
			M_ RunProgramByName("TM16_GigaAttack", MPartRef());

			data.GigaHasAttacked = true;
			data.GigaTargetArray[1][1] = true;
		}
		else
			data.GigaTargetArray[1][1] = true;

		if(!data.VoraakHasAttacked)
		{
			M_ RunProgramByName("TM16_VoraakAttack", MPartRef());

			data.VoraakHasAttacked = true;
			data.VoraakTargetArray[1][0] = true;
		}
		else
			data.VoraakTargetArray[1][0] = true;

		data.GaarFlag = true;
	}

	if(part->systemID == IODE && part->playerID == MANTIS_ID && !data.IodeFlag)
	{
		if(!data.VoraakHasAttacked)
		{
			M_ RunProgramByName("TM16_VoraakAttack", MPartRef());

			data.VoraakHasAttacked = true;
			data.VoraakTargetArray[1][1] = true;
		}
		else
			data.VoraakTargetArray[1][1] = true;

		if(!data.OctariusHasAttacked)
		{
			M_ RunProgramByName("TM16_OctariusAttack", MPartRef());

			data.OctariusHasAttacked = true;
			data.OctariusTargetArray[1][1] = true;
		}
		else
			data.OctariusTargetArray[1][1] = true;

		if(!data.GaarHasAttacked)
		{
			M_ RunProgramByName("TM16_GaarAttack", MPartRef());

			data.GaarHasAttacked = true;
			data.GaarTargetArray[1][1] = true;
		}
		else
			data.GaarTargetArray[1][1] = true;

		data.IodeFlag = true;
	}

	if(part->systemID == VORAAK && part->playerID == MANTIS_ID && !data.VoraakFlag)
	{
		M_ RunProgramByName("TM16_VoraakAttackedByPlayer", MPartRef());

		data.VoraakFlag = true;
	}


	switch (part->mObjClass)
	{
		case M_CORVETTE:

			if(part == data.Blackwell)
			{
				M_ FlushStreams();
        	    MScript::MoveCamera( part, 0, MOVIE_CAMERA_JUMP_TO);

//				data.mhandle = M_ PlayTeletype(IDS_TM16_HALSEY_M16HA17, SuperLeft, SuperTop, SuperRight, SuperBottom, MOTextColor, SuperHoldTime, TELETYPE_TIME, false);
				data.mhandle = M_ PlayAnimatedMessage("M16HA22.wav", "Animate!!Halsey2", MagpieLeft, MagpieTop, data.Halsey, IDS_TM16_SUB_M16HA22);

				data.failureID = IDS_TM16_FAIL_BLACKWELL_LOST;

				M_ RunProgramByName("TM16_MissionFailure", MPartRef());
			}

			break;

		case M_TIAMAT:

			if(part == data.SuperShip)
			{
				M_ FlushStreams();
        	    MScript::MoveCamera( part, 0, MOVIE_CAMERA_JUMP_TO);

//				data.mhandle = M_ PlayTeletype(IDS_TM16_MALKOR_M16ML15, SuperLeft, SuperTop, SuperRight, SuperBottom, MOTextColor, SuperHoldTime, TELETYPE_TIME, false);
				data.mhandle = M_ PlayAnimatedMessage("M16MA18.wav", "Animate!!Malkor2", MagpieLeft, MagpieTop, IDS_TM16_SUB_M16MA18);

				M_ RunProgramByName("TM16_MissionSuccess", MPartRef());
			}

			break;

		case M_MISSILECRUISER:

			if(part == data.Hawkes)
			{
				M_ FlushStreams();
        	    MScript::MoveCamera( part, 0, MOVIE_CAMERA_JUMP_TO);

//				data.mhandle = M_ PlayTeletype(IDS_TM16_HALSEY_M16HA17, SuperLeft, SuperTop, SuperRight, SuperBottom, MOTextColor, SuperHoldTime, TELETYPE_TIME, false);
				data.mhandle = M_ PlayAnimatedMessage("M16HA22.wav", "Animate!!Halsey2", MagpieLeft, MagpieTop, data.Halsey, IDS_TM16_SUB_M16HA22);

				data.failureID = IDS_TM16_FAIL_HAWKES_LOST;

				M_ RunProgramByName("TM16_MissionFailure", MPartRef());
			}

			break;
		
		case M_DREADNOUGHT:

			if(part == data.Halsey)
			{
				data.HalseyDead = true;

				M_ FlushStreams();
        	    MScript::MoveCamera( part, 0, MOVIE_CAMERA_JUMP_TO);

//				data.mhandle = M_ PlayTeletype(IDS_TM16_STEELE_M16ST19, SuperLeft, SuperTop, SuperRight, SuperBottom, MOTextColor, SuperHoldTime, TELETYPE_TIME, false);
				data.mhandle = M_ PlayAnimatedMessage("M16st23.wav", "Animate!!Steele2", MagpieLeft, MagpieTop, data.Steele, IDS_TM16_SUB_M16ST23);

				data.failureID = IDS_TM16_FAIL_HALSEY_LOST;

				M_ RunProgramByName("TM16_MissionFailure", MPartRef());
			}

			break;		

		case M_CARRIER:

			if(part == data.Takei)
			{
				M_ FlushStreams();
        	    MScript::MoveCamera( part, 0, MOVIE_CAMERA_JUMP_TO);

//				data.mhandle = M_ PlayTeletype(IDS_TM16_HALSEY_M16HA17, SuperLeft, SuperTop, SuperRight, SuperBottom, MOTextColor, SuperHoldTime, TELETYPE_TIME, false);
				data.mhandle = M_ PlayAnimatedMessage("M16HA22.wav", "Animate!!Halsey2", MagpieLeft, MagpieTop, data.Halsey, IDS_TM16_SUB_M16HA22);

				data.failureID = IDS_TM16_FAIL_TAKEI_LOST;

				M_ RunProgramByName("TM16_MissionFailure", MPartRef());
			}

			break;

		case M_LANCER:
			if(part == data.Benson)
			{
				M_ FlushStreams();
        	    MScript::MoveCamera( part, 0, MOVIE_CAMERA_JUMP_TO);

///				data.mhandle = M_ PlayTeletype(IDS_TM16_HALSEY_M16HA17, SuperLeft, SuperTop, SuperRight, SuperBottom, MOTextColor, SuperHoldTime, TELETYPE_TIME, false);
				data.mhandle = M_ PlayAnimatedMessage("M16HA22.wav", "Animate!!Halsey2", MagpieLeft, MagpieTop, data.Halsey, IDS_TM16_SUB_M16HA22);

				data.failureID = IDS_TM16_FAIL_BENSON_LOST;

				M_ RunProgramByName("TM16_MissionFailure", MPartRef());
			}

			break;

		case M_BATTLESHIP:

			if(part == data.Steele)
			{
				M_ FlushStreams();
        	    MScript::MoveCamera( part, 0, MOVIE_CAMERA_JUMP_TO);

//				data.mhandle = M_ PlayTeletype(IDS_TM16_HALSEY_M16HA17, SuperLeft, SuperTop, SuperRight, SuperBottom, MOTextColor, SuperHoldTime, TELETYPE_TIME, false);
				data.mhandle = M_ PlayAnimatedMessage("M16HA22.wav", "Animate!!Halsey2", MagpieLeft, MagpieTop, data.Halsey, IDS_TM16_SUB_M16HA22);

				data.failureID = IDS_TM16_FAIL_STEELE_LOST;

				M_ RunProgramByName("TM16_MissionFailure", MPartRef());
			}



			break;

		case M_MONOLITH:

			if(part == data.Vivac)
			{
				M_ FlushStreams();
        	    MScript::MoveCamera( part, 0, MOVIE_CAMERA_JUMP_TO);

//				data.mhandle = M_ PlayTeletype(IDS_TM16_HALSEY_M16HA18, SuperLeft, SuperTop, SuperRight, SuperBottom, MOTextColor, SuperHoldTime, TELETYPE_TIME, false);
				data.mhandle = M_ PlayAnimatedMessage("M16HA24.wav", "Animate!!Halsey2", MagpieLeft, MagpieTop, data.Halsey, IDS_TM16_SUB_M16HA24);

				data.failureID = IDS_TM16_FAIL_VIVAC_LOST;

				M_ RunProgramByName("TM16_MissionFailure", MPartRef());
			}

			break;

	}
}

bool TM16_ObjectDestroyed::Update (void)
{
	return false;
}

//-----------------------------------------------------------------------
// UNDER ATTACK EVENT

CQSCRIPTPROGRAM(TM16_UnderAttack, TM16_UnderAttack_Save, CQPROGFLAG_UNDERATTACK);

void TM16_UnderAttack::Initialize (U32 eventFlags, const MPartRef & part)
{
	if(data.mission_over)
	{
		return;
	}

	if(part->systemID == MOG && part->playerID == MANTIS_ID && !data.MogFlag)
	{
		M_ RunProgramByName("TM16_LagoAttack", MPartRef());

		data.MogFlag = true;
	}

	if(part->systemID == TARE && part->playerID == MANTIS_ID && !data.TareFlag)
	{
		if(!data.GigaHasAttacked)
		{
			M_ RunProgramByName("TM16_GigaAttack", MPartRef());

			data.GigaHasAttacked = true;
			data.GigaTargetArray[1][0] = true;
		}
		else
			data.GigaTargetArray[1][0] = true;

		if(!data.OctariusHasAttacked)
		{
			M_ RunProgramByName("TM16_OctariusAttack", MPartRef());

			data.OctariusHasAttacked = true;
			data.OctariusTargetArray[1][0] = true;
		}
		else
			data.OctariusTargetArray[1][0] = true;

		data.TareFlag = true;
	}

	if(part->systemID == GIGA && part->playerID == MANTIS_ID && !data.GigaFlag)
	{
		if(!data.TareHasAttacked)
		{
			M_ RunProgramByName("TM16_TareAttack", MPartRef());

			data.TareHasAttacked = true;
			data.TareTargetArray[1][0] = true;
		}
		else
			data.TareTargetArray[1][0] = true;

		if(!data.GaarHasAttacked)
		{
			M_ RunProgramByName("TM16_GaarAttack", MPartRef());

			data.GaarHasAttacked = true;
			data.GaarTargetArray[1][0] = true;
		}
		else
			data.GaarTargetArray[1][0] = true;

		data.GigaFlag = true;
	}

	if(part->systemID == GAAR && part->playerID == MANTIS_ID && !data.GaarFlag)
	{
		if(!data.IodeHasAttacked)
		{
			M_ RunProgramByName("TM16_IodeAttack", MPartRef());

			data.IodeHasAttacked = true;
			data.IodeTargetArray[1][0] = true;
		}
		else
			data.IodeTargetArray[1][0] = true;

		if(!data.GigaHasAttacked)
		{
			M_ RunProgramByName("TM16_GigaAttack", MPartRef());

			data.GigaHasAttacked = true;
			data.GigaTargetArray[1][1] = true;
		}
		else
			data.GigaTargetArray[1][1] = true;

		if(!data.VoraakHasAttacked)
		{
			M_ RunProgramByName("TM16_VoraakAttack", MPartRef());

			data.VoraakHasAttacked = true;
			data.VoraakTargetArray[1][0] = true;
		}
		else
			data.VoraakTargetArray[1][0] = true;

		data.GaarFlag = true;
	}

	if(part->systemID == IODE && part->playerID == MANTIS_ID && !data.IodeFlag)
	{
		if(!data.VoraakHasAttacked)
		{
			M_ RunProgramByName("TM16_VoraakAttack", MPartRef());

			data.VoraakHasAttacked = true;
			data.VoraakTargetArray[1][1] = true;
		}
		else
			data.VoraakTargetArray[1][1] = true;

		if(!data.OctariusHasAttacked)
		{
			M_ RunProgramByName("TM16_OctariusAttack", MPartRef());

			data.OctariusHasAttacked = true;
			data.OctariusTargetArray[1][1] = true;
		}
		else
			data.OctariusTargetArray[1][1] = true;

		if(!data.GaarHasAttacked)
		{
			M_ RunProgramByName("TM16_GaarAttack", MPartRef());

			data.GaarHasAttacked = true;
			data.GaarTargetArray[1][1] = true;
		}
		else
			data.GaarTargetArray[1][1] = true;

		data.IodeFlag = true;
	}

	if(part->systemID == VORAAK && part->playerID == MANTIS_ID && !data.VoraakFlag)
	{
		M_ RunProgramByName("TM16_VoraakAttackedByPlayer", MPartRef());

		data.VoraakFlag = true;
	}

}

bool TM16_UnderAttack::Update (void)
{
	return false;
}

//-----------------------------------------------------------------------
//OBJECT CONSTRUCTED EVENT

CQSCRIPTPROGRAM(TM16_ObjectConstructed, TM16_ObjectConstructed_Save, CQPROGFLAG_OBJECTCONSTRUCTED);

void TM16_ObjectConstructed::Initialize (U32 eventFlags, const MPartRef & part)
{
	if(data.mission_over)
	{
		return;
	}

	if(part->systemID == MOG && part->playerID == PLAYER_ID && !data.LagoIsDefended)
	{
		for (int x=0; x<2; x++)
		{
			MPartRef target = M_ CreatePart("GBOAT!!M_Scarab", M_ GetPartByName("TareSpawn"), MANTIS_ID);
			M_ SetStance(target, US_DEFEND);
			M_ OrderMoveTo(target, M_ GetPartByName("LagoDefend1"));

			target = M_ CreatePart("GBOAT!!M_Tiamat", M_ GetPartByName("GigaSpawn"), MANTIS_ID);
			M_ SetStance(target, US_DEFEND);
			M_ OrderMoveTo(target, M_ GetPartByName("LagoDefend2"));
		}

		data.LagoIsDefended = true;
	}

	if(part->systemID == MOG && part->playerID == PLAYER_ID && !data.OctariusFlag)
	{
		if(!data.TareHasAttacked)
		{
			M_ RunProgramByName("TM16_TareAttack", MPartRef());

			data.TareHasAttacked = true;
			data.TareTargetArray[1][1] = true;
		}
		else
			data.TareTargetArray[1][1] = true;

		if(!data.IodeHasAttacked)
		{
			M_ RunProgramByName("TM16_IodeAttack", MPartRef());

			data.IodeHasAttacked = true;
			data.IodeTargetArray[1][1] = true;
		}
		else
			data.IodeTargetArray[1][1] = true;

		data.OctariusFlag = true;
	}



}

bool TM16_ObjectConstructed::Update (void)
{
	return false;
}


//---------------------------------------------------------------------------
// MISSION FAILURE PROGRAMS


//---------------------------------------------------------------------------

#define TELETYPE_TIME 3000

CQSCRIPTPROGRAM(TM16_MissionFailure, TM16_MissionFailure_Save, 0);

void TM16_MissionFailure::Initialize (U32 eventFlags, const MPartRef & part)
{
	M_ PlayMusic(DEFEAT);

	M_ EnableMovieMode(true);
	UnderMissionControl();	
	data.mission_over = true;

	if(data.HalseyDead)
	{
		state = Fail;
	}
	else
		state = Uplink;
}

bool TM16_MissionFailure::Update (void)
{
	switch (state)
	{
		case Uplink:
			if (!M_ IsStreamPlaying(data.mhandle))
			{
//				data.shandle = M_ PlayAudio("high_level_data_uplink.wav");
				state = Halsey;
			}
			break;

		case Halsey:
			if (!M_ IsStreamPlaying(data.shandle) && !M_ IsStreamPlaying(data.mhandle))
			{
				if(data.SuperShip->systemID != VORAAK)
				{
					M_ FlushTeletype();
//					data.mhandle = M_ PlayTeletype(IDS_TM16_HALSEY_M16HA21, SuperLeft, SuperTop, SuperRight, SuperBottom, MOTextColor, 6000, 1000, false);
					data.mhandle = M_ PlayAnimatedMessage("M16HA25.wav", "Animate!!Halsey2", MagpieLeft, MagpieTop, data.Halsey, IDS_TM16_SUB_M16HA25);
				}
				state = Fail;
			}
			break;

		case Fail:
			if (!M_ IsStreamPlaying(data.mhandle))
			{
				M_ FlushTeletype();
				data.mhandle = TeletypeMissionOver(IDS_TM16_MISSION_FAILURE,data.failureID);
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

CQSCRIPTPROGRAM(TM16_MissionSuccess, TM16_MissionSuccess_Save, 0);

void TM16_MissionSuccess::Initialize (U32 eventFlags, const MPartRef & part)
{
	M_ PlayMusic(VICTORY);

	M_ EnableMovieMode(true);

	UnderMissionControl();	
	data.mission_over = true;
	state = Uplink;

}

bool TM16_MissionSuccess::Update (void)
{
	switch (state)
	{
		case Uplink:
			if (!M_ IsStreamPlaying(data.mhandle))
			{
//				data.shandle = M_ PlayAudio("high_level_data_uplink.wav");
				state = Steele;
			}
			break;

		case Steele:
			if(!M_ IsStreamPlaying(data.shandle))
			{
//				data.mhandle = M_ PlayTeletype(IDS_TM16_STEELE_M16ST20, SuperLeft, SuperTop, SuperRight, SuperBottom, MOTextColor, 5000, 2000, false);
				data.mhandle = M_ PlayAnimatedMessage("M16st19.wav", "Animate!!Steele2", MagpieLeft, MagpieTop, data.Steele, IDS_TM16_SUB_M16ST19);

				state = Benson;
			}
			break;
				
		case Benson:
			if(!M_ IsStreamPlaying(data.mhandle))
			{
				data.mhandle = M_ PlayAnimatedMessage("M16BN20.wav", "Animate!!Benson2", MagpieLeft, MagpieTop, data.Benson, IDS_TM16_SUB_M16BN20);

				state = Success;
			}
			break;

		case Success:
			
			if(!M_ IsStreamPlaying(data.mhandle))
			{
				M_ FlushTeletype();
				data.mhandle = TeletypeMissionOver(IDS_TM16_MISSION_SUCCESS);
				state = Done;
			}
			break;

		case Done:

			UnderPlayerControl();
			M_ EndMissionVictory(15);
			return false;

			break;
	}

	return true;
}

//--------------------------------------------------------------------------//
//--------------------------------End Script15T.cpp-------------------------//
//--------------------------------------------------------------------------//
