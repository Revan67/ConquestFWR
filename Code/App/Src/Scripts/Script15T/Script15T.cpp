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
		airules.difficulty = MEDIUM;
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
		airules.nPanicThreshold = 2500;

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
	airules.buildMask.bBuildHeavyGunboats = bOn;
	airules.buildMask.bHarvest = true;
	airules.buildMask.bScout = true;
	airules.buildMask.bBuildAdmirals = false;
	airules.buildMask.bLaunchOffensives = true;
	airules.nGunboatsPerSupplyShip = 12;
	airules.nNumMinelayers = 2;
	airules.uNumScouts = 3;
	airules.nNumFabricators = 2;
	airules.fHarvestersPerRefinery = 2;
	airules.nMaxHarvesters = 5;
	airules.nPanicThreshold = 2500;

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

//--------------------------------------------------------------------------
//	MISSION BRIEFING
//--------------------------------------------------------------------------

CQSCRIPTPROGRAM(TM15_Briefing, TM15_Briefing_Save,CQPROGFLAG_STARTBRIEFING);

void TM15_Briefing::Initialize (U32 eventFlags, const MPartRef & part)
{
	data.failureID = 0;
	data.mhandle = 0;
	data.shandle = 0;
	data.mission_state = Briefing;
	state = Begin;

	M_ PlayMusic(NULL);
	M_ EnableBriefingControls(true);
	
}

bool TM15_Briefing::Update (void)
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
			data.mhandle = TeletypeBriefing(IDS_TM15_TELETYPE_LOCATION, false);

			state = Jill;

			break;

		case Jill:

			if (!M_ IsStreamPlaying(data.mhandle))
			{
				data.mhandle = M_ PlayAudio("prolog15.wav");
				MScript::BriefingSubtitle(data.mhandle,IDS_TM15_SUB_PROLOG15);
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
				data.mhandle = M_ PlayBriefingAnimation(slotAnim);

				strcpy(slotAnim.szTypeName, "Animate!!Xmission");
				slotAnim.slotID = 1;
				slotAnim.dwTimer = ANIMSPD_XMISSION;
				data.mhandle = M_ PlayBriefingAnimation(slotAnim);

				strcpy(slotAnim.szTypeName, "Animate!!Xmission");
				slotAnim.slotID = 2;
				slotAnim.dwTimer = ANIMSPD_XMISSION;
				data.mhandle = M_ PlayBriefingAnimation(slotAnim);

				M_ FreeBriefingSlot(3);

				state = Halsey;
			}
			break;

		case Halsey:

			if (M_ IsCustomBriefingAnimationDone(0))
			{
				M_ FlushTeletype();

				strcpy(slotAnim.szFileName, "m15ha03.wav");
				strcpy(slotAnim.szTypeName, "Animate!!Halsey");
				slotAnim.slotID = 0;
				slotAnim.bHighlite = false;
				slotAnim.bContinueAnimating = true;
				data.mhandle = M_ PlayBriefingTalkingHead(slotAnim);
				MScript::BriefingSubtitle(data.mhandle,IDS_TM15_SUB_M15HA03);

				ShowBriefingHead(1, CHAR_KERTAK);
				ShowBriefingHead(2, CHAR_STEELE);

				state = KerTak;
			}

			break;

		case KerTak:
			if(!M_ IsStreamPlaying(data.mhandle))
			{
				M_ FlushTeletype();
//				data.mhandle = M_ PlayBriefingTeletype(IDS_TM15_KERTAK_M15KR04, MOTextColor, 6000, 1000, false);

				strcpy(slotAnim.szFileName, "m15kr04.wav");
				strcpy(slotAnim.szTypeName, "Animate!!KerTak");
				slotAnim.slotID = 1;
				slotAnim.bHighlite = false;
				slotAnim.bContinueAnimating = true;
				data.mhandle = M_ PlayBriefingTalkingHead(slotAnim);
				MScript::BriefingSubtitle(data.mhandle,IDS_TM15_SUB_M15KR04);
				
				state = Steele;
			}
			break;

		case Steele:
			if(!M_ IsStreamPlaying(data.mhandle))
			{
				M_ FlushTeletype();
//				data.mhandle = M_ PlayBriefingTeletype(IDS_TM15_STEELE_M15ST05, MOTextColor, 6000, 1000, false);

				strcpy(slotAnim.szFileName, "m15st05.wav");
				strcpy(slotAnim.szTypeName, "Animate!!Steele");
				slotAnim.slotID = 2;
				slotAnim.bHighlite = false;
				data.mhandle = M_ PlayBriefingTalkingHead(slotAnim);
				MScript::BriefingSubtitle(data.mhandle,IDS_TM15_SUB_M15ST05);
				
				state = KerTak2;
			}
			break;

		case KerTak2:
			if(!M_ IsStreamPlaying(data.mhandle))
			{
				M_ FlushTeletype();
//				data.mhandle = M_ PlayBriefingTeletype(IDS_TM15_KERTAK_M15KR06, MOTextColor, 6000, 1000, false);
				
				strcpy(slotAnim.szFileName, "m15kr06.wav");
				strcpy(slotAnim.szTypeName, "Animate!!KerTak");
				slotAnim.slotID = 1;
				slotAnim.bHighlite = false;
				slotAnim.bContinueAnimating = true;
				data.mhandle = M_ PlayBriefingTalkingHead(slotAnim);
				MScript::BriefingSubtitle(data.mhandle,IDS_TM15_SUB_M15KR06);

				state = Halsey2;
			}
			break;

		case Halsey2:
			if(!M_ IsStreamPlaying(data.mhandle))
			{
				M_ FlushTeletype();

				M_ FreeBriefingSlot(1);

				strcpy(slotAnim.szFileName, "m15ha07.wav");
				strcpy(slotAnim.szTypeName, "Animate!!Halsey");
				slotAnim.slotID = 0;
				slotAnim.bHighlite = false;
				data.mhandle = M_ PlayBriefingTalkingHead(slotAnim);
				MScript::BriefingSubtitle(data.mhandle,IDS_TM15_SUB_M15HA07);


				state = MO;

			}
			break;

		case MO:
			
			if (!M_ IsStreamPlaying(data.mhandle))
			{
				M_ FlushTeletype();
				data.mhandle = TeletypeBriefing(IDS_TM15_OBJECTIVES, true);
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

CQSCRIPTPROGRAM(TM15_MissionStart, TM15_MissionStart_Save,CQPROGFLAG_STARTMISSION);

void TM15_MissionStart::Initialize (U32 eventFlags, const MPartRef & part)
{

	data.mission_state = MissionStart;
	state = Begin;
	data.HalseyDead = false;

	M_ SetMissionName(IDS_TM15_MISSION_NAME);
	M_ SetMissionID(14);

	M_ SetEnemyCharacter(MANTIS_ID, IDS_TM15_MANTIS);

	M_ EnableRegenMode(true);

	M_ SetMissionDescription(IDS_TM15_MISSION_DESC);

	M_ AddToObjectiveList(IDS_TM15_OBJECTIVE1);
	M_ AddToObjectiveList(IDS_TM15_OBJECTIVE2);
	M_ AddToObjectiveList(IDS_TM15_OBJECTIVE3);

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
		TECHTREE::TDEPEND_DISPLACEMENT
        );

    missionTech.race[1].build = (TECHTREE::BUILDNODE) (
        missionTech.race[1].build |
		TECHTREE::MDEPEND_PLANTATION |
		TECHTREE::MDEPEND_BIOFORGE );


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

    missionTech.race[1].tech = (TECHTREE::TECHUPGRADE) (
        missionTech.race[1].tech |
		TECHTREE::M_SHIP_KHAMIR );

	missionTech.race[0].common_extra = missionTech.race[1].common_extra = (TECHTREE::COMMON_EXTRA) (
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

	missionTech.race[0].common = missionTech.race[1].common = (TECHTREE::COMMON) (
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

    MScript::SetAvailiableTech( missionTech );

	data.MantisLastStand = data.BeatusHasAttacked = data.JovianHasAttacked = data.AmpereHasAttacked = data.VoxHasAttacked = data.UstedHasAttacked = data.MalotiHasAttacked = data.CentelHasAttacked = data.AltoHasAttacked = data.FaradHasAttacked = data.DeciHasAttacked = false;


	//Set each sides resources...

	M_ SetMaxCommandPoitns(MANTIS_ID, 100);
	M_ SetMaxCommandPoitns(PLAYER_ID, 125);

	M_ SetGas(PLAYER_ID, 300);
	M_ SetMetal(PLAYER_ID, 300);
	M_ SetCrew(PLAYER_ID, 200);

	M_ SetMaxGas(PLAYER_ID, 300);
	M_ SetMaxMetal(PLAYER_ID, 300);
	M_ SetMaxCrew(PLAYER_ID, 200);

	SetupAI(true);
	MassEnableAI(MANTIS_ID, false, 0);
	MassEnableAI(MANTIS_ID, true, NACH);

	data.Vivac = M_ GetPartByName("Vivac");
	M_ EnablePartnameDisplay(data.Vivac, true);

	data.Steele = M_ GetPartByName("Steele");
	M_ EnablePartnameDisplay(data.Steele, true);

	data.Takei = M_ GetPartByName("Takei");
	M_ EnablePartnameDisplay(data.Takei, true);
	
	data.Halsey = M_ GetPartByName("Halsey");
	M_ EnablePartnameDisplay(data.Halsey, true);
	M_ EnableJumpCap(data.Halsey, true);
	
	data.Benson = M_ GetPartByName("Benson");
	M_ EnablePartnameDisplay(data.Benson, true);

	data.E1 = M_ GetPartByName("Elite Battleship ");
	M_ EnablePartnameDisplay(data.E1, true);
	M_ EnableJumpCap(data.E1, true);

	data.E2 = M_ GetPartByName("Elite Battleship");
	M_ EnablePartnameDisplay(data.E2, true);
	M_ EnableJumpCap(data.E2, true);

	M_ MoveCamera(data.Halsey, 0, MOVIE_CAMERA_JUMP_TO);

// Attack target arrays init here...
	data.JovianTargetArray[0][0] = BEATUS;
	data.JovianTargetArray[0][1] = AMPERE;
	data.JovianTargetArray[1][0] = false;
	data.JovianTargetArray[1][1] = false;
	data.JovianTargetPlanet[0] = M_ GetPartByName("Beatus Prime");
	data.JovianTargetPlanet[1] = M_ GetPartByName("Ampere Prime");

	data.AmpereTargetArray[0][0] = BEATUS;
	data.AmpereTargetArray[0][1] = JOVIAN;
	data.AmpereTargetArray[1][0] = false;
	data.AmpereTargetArray[1][1] = false;
	data.AmpereTargetPlanet[0] = M_ GetPartByName("Beatus Prime");
	data.AmpereTargetPlanet[1] = M_ GetPartByName("Jovian Prime");

	data.VoxTargetArray[0][0] = AMPERE;
	data.VoxTargetArray[0][1] = JOVIAN;
	data.VoxTargetArray[0][2] = DECI;
	data.VoxTargetArray[0][3] = CENTEL;
	data.VoxTargetArray[1][0] = false;
	data.VoxTargetArray[1][1] = false;
	data.VoxTargetArray[1][2] = false;
	data.VoxTargetArray[1][3] = false;
	data.VoxTargetPlanet[0] = M_ GetPartByName("Ampere Prime");
	data.VoxTargetPlanet[1] = M_ GetPartByName("Jovian Prime");
	data.VoxTargetPlanet[2] = M_ GetPartByName("Deci Prime");
	data.VoxTargetPlanet[3] = M_ GetPartByName("Centel Prime");

	data.MalotiTargetArray[0][0] = USTED;
	data.MalotiTargetArray[0][1] = CENTEL;
	data.MalotiTargetArray[1][0] = false;
	data.MalotiTargetArray[1][1] = false;
	data.MalotiTargetPlanet[0] = M_ GetPartByName("Usted Prime");
	data.MalotiTargetPlanet[1] = M_ GetPartByName("Centel Prime");


	data.CentelTargetArray[0][0] = DECI;
	data.CentelTargetArray[0][1] = ALTO;
	data.CentelTargetArray[0][2] = FARAD;
	data.CentelTargetArray[1][0] = false;
	data.CentelTargetArray[1][1] = false;
	data.CentelTargetArray[1][2] = false;
	data.CentelTargetPlanet[0] = M_ GetPartByName("Deci Prime");
	data.CentelTargetPlanet[1] = M_ GetPartByName("Alto Prime");
	data.CentelTargetPlanet[2] = M_ GetPartByName("Farad Prime");

	data.AltoTargetArray[0][0] = DECI;
	data.AltoTargetArray[0][1] = CENTEL;
	data.AltoTargetArray[1][0] = false;
	data.AltoTargetArray[1][1] = false;
	data.AltoTargetPlanet[0] = M_ GetPartByName("Deci Prime");
	data.AltoTargetPlanet[1] = M_ GetPartByName("Centel Prime");

	data.FaradTargetArray[0][0] = ALTO;
	data.FaradTargetArray[0][1] = CENTEL;
	data.FaradTargetArray[1][0] = false;
	data.FaradTargetArray[1][1] = false;
	data.FaradTargetPlanet[0] = M_ GetPartByName("Alto Prime");
	data.FaradTargetPlanet[1] = M_ GetPartByName("Centel Prime");

	data.DeciTargetArray[0][0] = ALTO;
	data.DeciTargetArray[0][1] = CENTEL;
	data.DeciTargetArray[1][0] = false;
	data.DeciTargetArray[1][1] = false;
	data.DeciTargetPlanet[0] = M_ GetPartByName("Alto Prime");
	data.DeciTargetPlanet[1] = M_ GetPartByName("Centel Prime");

	M_ EnableSystem(ALTO, true, true);
	M_ EnableSystem(GELD, true, true);

	M_ RunProgramByName("TM15_AIProgram", MPartRef ());
	M_ RunProgramByName("TM15_ProgressCheck", MPartRef ());
	M_ RunProgramByName("TM15_HalseyReturn", MPartRef ());

	state = Begin;
}

bool TM15_MissionStart::Update (void)
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
				data.mhandle = M_ PlayAnimatedMessage("M15HA08.wav", "Animate!!Halsey2", MagpieLeft, MagpieTop, data.Halsey, IDS_TM15_SUB_M15HA08);

				state = Takei;
			}
			break;

		case Takei:
			if(!M_ IsStreamPlaying(data.mhandle))
			{
				M_ FlushTeletype();
//				data.mhandle = M_ PlayTeletype(IDS_TM15_TAKEI_M15TK04, SuperLeft, SuperTop, SuperRight, SuperBottom, MOTextColor, 6000, 1000, false);
				data.mhandle = M_ PlayAnimatedMessage("M15TK09.wav", "Animate!!Takei2", MagpieLeft, MagpieTop, data.Takei, IDS_TM15_SUB_M15TK09);
				
				state = End;
			}
			break;

		case End:
			if(!M_ IsStreamPlaying(data.mhandle))
			{
				data.mhandle = M_ PlayAnimatedMessage("M15HA10.wav", "Animate!!Halsey2", MagpieLeft, MagpieTop, data.Halsey, IDS_TM15_SUB_M15HA10);

				return false;
			}
			break;

	}
	return true;
}

//--------------------------------------------------------------------------//

CQSCRIPTPROGRAM(TM15_AIProgram, TM15_AIProgram_Save, 0);

void TM15_AIProgram::Initialize (U32 eventFlags, const MPartRef & part)
{
	G1 = M_ GetPartByName("Geld Prime");
	G2 = M_ GetPartByName("Geld 2");
}

bool TM15_AIProgram::Update (void)
{
	if(data.mission_over)
		return false;
	
	timer -= ELAPSED_TIME;

	if(G1.isValid() && G2.isValid() && timer <= 0)
	{
		G1Crew = M_ GetCrewOnPlanet(G1);
		G2Crew = M_ GetCrewOnPlanet(G2);

		if(G1Crew < 1000)
		{
			M_ SetResourcesOnPlanet(G1, -1, -1, G1Crew + 1);

		}

		if(G2Crew < 1000)
		{
			M_ SetResourcesOnPlanet(G2, -1, -1, G2Crew + 1);

		}

		timer = 3;
	}

	//Set up cases in which the AI will turn off
	if(M_ GetUsedCommandPoints(MANTIS_ID) > 75 || M_ GetUsedCommandPoints(PLAYER_ID) < 25)
	{
		ToggleBuildUnits(false);

		return true;
	}

 //Set up cases in which the AI will turn back on
	if(M_ GetUsedCommandPoints(MANTIS_ID) < 25 || M_ GetUsedCommandPoints(PLAYER_ID) > 80)
	{
		ToggleBuildUnits(true);
	}

	return true;
}

//--------------------------------------------------------------------------//

CQSCRIPTPROGRAM(TM15_ProgressCheck, TM15_ProgressCheck_Save, 0);

void TM15_ProgressCheck::Initialize (U32 eventFlags, const MPartRef & part)
{
	AITrigger1 = AITrigger2 = AITrigger3 = PlayerInAlto = false;
}

bool TM15_ProgressCheck::Update (void)
{	
	if(data.mission_over)
	{
		return false;
	}
	
	if(PlayerInSystem(PLAYER_ID, ALTO) && !PlayerInAlto)
	{
		M_ RunProgramByName("TM15_PlayerInAlto", MPartRef());
		M_ RunProgramByName("TM15_MissionSuccess", MPartRef());

		PlayerInAlto = true;
		return false;
	}

	if(PlayerInSystem(PLAYER_ID, NACH) && !AITrigger1)
	{
		MassEnableAI(MANTIS_ID, true, BEATUS);
		MassEnableAI(MANTIS_ID, true, JOVIAN);
		MassEnableAI(MANTIS_ID, true, AMPERE);

		AITrigger1 = true;
	}

	if((PlayerInSystem(PLAYER_ID, AMPERE) || PlayerInSystem(PLAYER_ID, JOVIAN)) && !AITrigger2)
	{
		MassEnableAI(MANTIS_ID, true, MALOTI);
		MassEnableAI(MANTIS_ID, true, USTED);
		MassEnableAI(MANTIS_ID, true, VOX);
		MassEnableAI(MANTIS_ID, true, DECI);

		AITrigger2 = true;
	}

	if((PlayerInSystem(PLAYER_ID, MALOTI) || PlayerInSystem(PLAYER_ID, USTED) || PlayerInSystem(PLAYER_ID, VOX) || PlayerInSystem(PLAYER_ID, DECI)) && !AITrigger3)
	{
		MassEnableAI(MANTIS_ID, true, FARAD);
		MassEnableAI(MANTIS_ID, true, CENTEL);
		MassEnableAI(MANTIS_ID, true, ALTO);

		AITrigger3 = true;
	}

	return true;
}
/*
//--------------------------------------------------------------------------//

CQSCRIPTPROGRAM(TM15_HalseyReturn, TM15_HalseyReturn_Save, 0);

void TM15_HalseyReturn::Initialize (U32 eventFlags, const MPartRef & part)
{
	Return = M_ GetPartByName("HalseyReturn");
}

bool TM15_HalseyReturn::Update (void)
{
	if(data.mission_over == true)
	{
		return false;
	}

	if(data.Halsey->systemID != GELD && !data.Halsey->bUnselectable)
	{
		if(!M_ IsStreamPlaying(data.mhandle))
		{
			data.mhandle = M_ PlayAnimatedMessage("M15HA11.wav", "Animate!!Halsey2", MagpieLeft, MagpieTop, data.Halsey);
		}
		M_ EnableSelection(data.Halsey, false);

		M_ OrderMoveTo(data.Halsey, Return);
	}
	if(data.Halsey->systemID == GELD && data.Halsey->bUnselectable)
	{
		M_ EnableSelection(data.Halsey, true);
	}

	if(data.E1.isValid())
	{
		if(!data.E1->bUnselectable && data.E1->systemID != GELD)
		{
			if(!M_ IsStreamPlaying(data.mhandle))
			{
				data.mhandle = M_ PlayAnimatedMessage("M15HA11.wav", "Animate!!Halsey2", MagpieLeft, MagpieTop, data.Halsey);
			}
			M_ EnableSelection(data.E1, false);

			M_ OrderMoveTo(data.E1, Return);
		}
		if(data.E1->bUnselectable && data.E1->systemID == GELD)
		{
			M_ EnableSelection(data.E1, true);
		}
	}

	if(data.E2.isValid())
	{
		if(!data.E2->bUnselectable && data.E2->systemID != GELD)
		{
			if(!M_ IsStreamPlaying(data.mhandle))
			{
				data.mhandle = M_ PlayAnimatedMessage("M15HA11.wav", "Animate!!Halsey2", MagpieLeft, MagpieTop, data.Halsey);
			}
			M_ EnableSelection(data.E2, false);

			M_ OrderMoveTo(data.E2, Return);
		}
		if(data.E2->bUnselectable && data.E2->systemID == GELD)
		{
			M_ EnableSelection(data.E2, true);
		}
	}

	return true;
}
*/
//--------------------------------------------------------------------------//

CQSCRIPTPROGRAM(TM15_PlayerInAlto, TM15_PlayerInAlto_Save, 0);

void TM15_PlayerInAlto::Initialize (U32 eventFlags, const MPartRef & part)
{
	state = Begin;
}

bool TM15_PlayerInAlto::Update (void)
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
//				data.mhandle = M_ PlayTeletype(IDS_TM15_BENSON_M15BN07, SuperLeft, SuperTop, SuperRight, SuperBottom, MOTextColor, 6000, 1000, false);
				data.mhandle = M_ PlayAnimatedMessage("M15BN12.wav", "Animate!!Benson2", MagpieLeft, MagpieTop, data.Benson, IDS_TM15_SUB_M15BN12);
	
				state = Next;
			}
			break;

		case Next:
			if(!M_ IsStreamPlaying(data.mhandle))
			{
				M_ FlushTeletype();
//				data.mhandle = M_ PlayTeletype(IDS_TM15_STEELE_M15ST13, SuperLeft, SuperTop, SuperRight, SuperBottom, MOTextColor, 6000, 1000, false);
				data.mhandle = M_ PlayAnimatedMessage("M15st13.wav", "Animate!!Steele2", MagpieLeft, MagpieTop, data.Steele, IDS_TM15_SUB_M15ST13);
	
				state = Benson;
			}
			break;

		case Benson:
			if(!M_ IsStreamPlaying(data.mhandle))
			{
				M_ FlushTeletype();
				data.mhandle = M_ PlayAnimatedMessage("M15BN14.wav", "Animate!!Benson2", MagpieLeft, MagpieTop, data.Benson, IDS_TM15_SUB_M15BN14);

				state = Steele;
			}
			break;

		case Steele:
			if(!M_ IsStreamPlaying(data.mhandle))
			{
				M_ FlushTeletype();
//				data.mhandle = M_ PlayTeletype(IDS_TM15_STEELE_M15ST08, SuperLeft, SuperTop, SuperRight, SuperBottom, MOTextColor, 6000, 1000, false);
				data.mhandle = M_ PlayAnimatedMessage("M15st15.wav", "Animate!!Steele2", MagpieLeft, MagpieTop, data.Steele, IDS_TM15_SUB_M15ST15);

				state = Done;
			}
			break;

		case Done:
			if(!M_ IsStreamPlaying(data.mhandle))
			{
				M_ FlushTeletype();
				data.mhandle = TeletypeObjective(IDS_TM15_NEWMO);
				
				M_ RemoveFromObjectiveList(IDS_TM15_OBJECTIVE2);
				M_ AddToObjectiveList(IDS_TM15_OBJECTIVE4);
				return false;
			}
			break;
	}

	return true;
}

//--------------------------------------------------------------------------//

CQSCRIPTPROGRAM(TM15_BeatusAttack, TM15_BeatusAttack_Save, 0);

void TM15_BeatusAttack::Initialize (U32 eventFlags, const MPartRef & part)
{
	timer = 0;

	Spawn = M_ GetPartByName("BeatusSpawn");
	NachPrime = M_ GetPartByName("Nach Prime");
}

bool TM15_BeatusAttack::Update (void)
{
	if(data.mission_over == true)
	{
		return false;
	}

	timer -= ELAPSED_TIME;

	if(timer < 0 && CountObjectsInRange(0, PLAYER_ID, Spawn, 5) == 0 && M_ GetUsedCommandPoints(MANTIS_ID) < M_ GetMaxCommandPoints(MANTIS_ID))
	{
		if(CountObjects(M_NIAD, MANTIS_ID, BEATUS) > 0)
		{
			if(PlayerInSystem(PLAYER_ID, NACH))
			{
				for (int x=0; x<3; x++)
				{
					M_ CreatePart("GBOAT!!M_Scarab", Spawn, MANTIS_ID);
				}

				M_ SetEnemyAITarget(MANTIS_ID, NachPrime, 50, NACH);
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

CQSCRIPTPROGRAM(TM15_JovianAttack, TM15_JovianAttack_Save, 0);

void TM15_JovianAttack::Initialize (U32 eventFlags, const MPartRef & part)
{
	timer = 0;

	Spawn = M_ GetPartByName("JovianSpawn");
}

bool TM15_JovianAttack::Update (void)
{
	if(data.mission_over == true)
	{
		return false;
	}

	timer -= ELAPSED_TIME;

	if(timer < 0 && CountObjectsInRange(0, PLAYER_ID, Spawn, 5) == 0 && M_ GetUsedCommandPoints(MANTIS_ID) < M_ GetMaxCommandPoints(MANTIS_ID))
	{
		if(CountObjects(M_NIAD, MANTIS_ID, JOVIAN) > 0)
		{
			if(!PlayerInSystem(PLAYER_ID, BEATUS) && !PlayerInSystem(PLAYER_ID, AMPERE)) //if player isnt in the systems under attack, dont attack
			{
				return true;
			}

			if(data.JovianTargetArray[1][0] && data.JovianTargetArray[1][1])
				Target = rand() % 2;
			else
			{
				if(data.JovianTargetArray[1][0])
					Target = 0;
				else
					Target = 1;
			}

			for (int x=0; x<3; x++)
			{
				M_ CreatePart("GBOAT!!M_Scarab", Spawn, MANTIS_ID);
			}
			
			M_ SetEnemyAITarget(MANTIS_ID, data.JovianTargetPlanet[Target], 50, data.JovianTargetArray[0][Target]);
			M_ LaunchOffensive(MANTIS_ID, US_ATTACK);

			timer = 300;
		}
		else
			return false;
	}

	return true;
}

//--------------------------------------------------------------------------//

CQSCRIPTPROGRAM(TM15_AmpereAttack, TM15_AmpereAttack_Save, 0);

void TM15_AmpereAttack::Initialize (U32 eventFlags, const MPartRef & part)
{
	timer = 0;

	Spawn = M_ GetPartByName("AmpereSpawn");
}

bool TM15_AmpereAttack::Update (void)
{
	if(data.mission_over == true)
	{
		return false;
	}

	timer -= ELAPSED_TIME;

	if(timer < 0 && CountObjectsInRange(0, PLAYER_ID, Spawn, 5) == 0 && M_ GetUsedCommandPoints(MANTIS_ID) < M_ GetMaxCommandPoints(MANTIS_ID))
	{
		if(CountObjects(M_NIAD, MANTIS_ID, AMPERE) > 0)
		{
			if(!PlayerInSystem(PLAYER_ID, BEATUS) && !PlayerInSystem(PLAYER_ID, JOVIAN)) //if player isnt in the systems under attack, dont attack
			{
				return true;
			}

			if(data.AmpereTargetArray[1][0] && data.AmpereTargetArray[1][1])
				Target = rand() % 2;
			else
			{
				if(data.AmpereTargetArray[1][0])
					Target = 0;
				else
					Target = 1;
			}

			for (int x=0; x<3; x++)
			{
				M_ CreatePart("GBOAT!!M_Scarab", Spawn, MANTIS_ID);
			}
			
			M_ SetEnemyAITarget(MANTIS_ID, data.AmpereTargetPlanet[Target], 50, data.AmpereTargetArray[0][Target]);
			M_ LaunchOffensive(MANTIS_ID, US_ATTACK);

			timer = 300;
		}
		else
			return false;
	}

	return true;
}

//--------------------------------------------------------------------------//

CQSCRIPTPROGRAM(TM15_VoxAttack, TM15_VoxAttack_Save, 0);

void TM15_VoxAttack::Initialize (U32 eventFlags, const MPartRef & part)
{
	timer = 0;

	Spawn = M_ GetPartByName("VoxSpawn");
}

bool TM15_VoxAttack::Update (void)
{
	if(data.mission_over == true)
	{
		return false;
	}

	timer -= ELAPSED_TIME;

	if(timer < 0 && CountObjectsInRange(0, PLAYER_ID, Spawn, 5) == 0 && M_ GetUsedCommandPoints(MANTIS_ID) < M_ GetMaxCommandPoints(MANTIS_ID))
	{
		if(CountObjects(M_NIAD, MANTIS_ID, VOX) > 0)
		{
			if(data.VoxTargetArray[1][0] && data.VoxTargetArray[1][1] && data.VoxTargetArray[1][2] && data.VoxTargetArray[1][3])
				Target = rand() % 4;
			else if(data.VoxTargetArray[1][0] && data.VoxTargetArray[1][1] && data.VoxTargetArray[1][2])
				Target = rand() % 3;
			else if(data.VoxTargetArray[1][0] && data.VoxTargetArray[1][1])
				Target = rand() % 2;
			else
			{
				if(data.VoxTargetArray[1][0])
					Target = 0;
				else
					Target = 1;
			}

			for (int x=0; x<3; x++)
			{
				M_ CreatePart("GBOAT!!M_Scarab", Spawn, MANTIS_ID);
			}
			
			M_ SetEnemyAITarget(MANTIS_ID, data.VoxTargetPlanet[Target], 50, data.VoxTargetArray[0][Target]);
			M_ LaunchOffensive(MANTIS_ID, US_ATTACK);

			timer = 300;
		}
		else
			return false;
	}

	return true;
}

//--------------------------------------------------------------------------//

CQSCRIPTPROGRAM(TM15_UstedAttack, TM15_UstedAttack_Save, 0);

void TM15_UstedAttack::Initialize (U32 eventFlags, const MPartRef & part)
{
	Spawn = M_ GetPartByName("UstedSpawn");
	JovianPrime = M_ GetPartByName("Jovian Prime");
}

bool TM15_UstedAttack::Update (void)
{
	if(data.mission_over == true)
	{
		return false;
	}

	timer -= ELAPSED_TIME;

	if(timer < 0 && CountObjectsInRange(0, PLAYER_ID, Spawn, 5) == 0 && M_ GetUsedCommandPoints(MANTIS_ID) < M_ GetMaxCommandPoints(MANTIS_ID))
	{
		if(CountObjects(M_NIAD, MANTIS_ID, USTED) > 0)
		{
			if(PlayerInSystem(PLAYER_ID, NACH))
			{
				for (int x=0; x<3; x++)
				{
					M_ CreatePart("GBOAT!!M_Scarab", Spawn, MANTIS_ID);
				}

				M_ SetEnemyAITarget(MANTIS_ID, JovianPrime, 50, JOVIAN);
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

CQSCRIPTPROGRAM(TM15_MalotiAttack, TM15_MalotiAttack_Save, 0);

void TM15_MalotiAttack::Initialize (U32 eventFlags, const MPartRef & part)
{
	timer = 0;

	Spawn = M_ GetPartByName("MalotiSpawn");
}

bool TM15_MalotiAttack::Update (void)
{
	if(data.mission_over == true)
	{
		return false;
	}

	timer -= ELAPSED_TIME;

	if(timer < 0 && CountObjectsInRange(0, PLAYER_ID, Spawn, 5) == 0 && M_ GetUsedCommandPoints(MANTIS_ID) < M_ GetMaxCommandPoints(MANTIS_ID))
	{
		if(CountObjects(M_NIAD, MANTIS_ID, MALOTI) > 0)
		{
			if(!PlayerInSystem(PLAYER_ID, USTED) && !PlayerInSystem(PLAYER_ID, CENTEL)) //if player isnt in the systems under attack, dont attack
			{
				return true;
			}

			if(data.MalotiTargetArray[1][0] && data.MalotiTargetArray[1][1])
				Target = rand() % 2;
			else
			{
				if(data.MalotiTargetArray[1][0])
					Target = 0;
				else
					Target = 1;
			}

			for (int x=0; x<3; x++)
			{
				M_ CreatePart("GBOAT!!M_Scarab", Spawn, MANTIS_ID);
			}
			
			M_ SetEnemyAITarget(MANTIS_ID, data.MalotiTargetPlanet[Target], 50, data.MalotiTargetArray[0][Target]);
			M_ LaunchOffensive(MANTIS_ID, US_ATTACK);

			timer = 300;
		}
		else
			return false;
	}

	return true;
}

//--------------------------------------------------------------------------//

CQSCRIPTPROGRAM(TM15_CentelAttack, TM15_CentelAttack_Save, 0);

void TM15_CentelAttack::Initialize (U32 eventFlags, const MPartRef & part)
{
	timer = 0;

	Spawn = M_ GetPartByName("CentelSpawn");
}

bool TM15_CentelAttack::Update (void)
{
	if(data.mission_over == true)
	{
		return false;
	}

	timer -= ELAPSED_TIME;

	if(timer < 0 && CountObjectsInRange(0, PLAYER_ID, Spawn, 5) == 0 && M_ GetUsedCommandPoints(MANTIS_ID) < M_ GetMaxCommandPoints(MANTIS_ID))
	{
		if(CountObjects(M_NIAD, MANTIS_ID, CENTEL) > 0)
		{
			if(data.CentelTargetArray[1][0] && data.CentelTargetArray[1][1] && data.CentelTargetArray[1][2])
				Target = rand() % 3;
			else if(data.CentelTargetArray[1][0] && data.CentelTargetArray[1][1])
				Target = rand() % 2;
			else
			{
				if(data.CentelTargetArray[1][0])
					Target = 0;
				else
					Target = 1;
			}

			for (int x=0; x<3; x++)
			{
				M_ CreatePart("GBOAT!!M_Scarab", Spawn, MANTIS_ID);
			}
			
			M_ SetEnemyAITarget(MANTIS_ID, data.CentelTargetPlanet[Target], 50, data.CentelTargetArray[0][Target]);
			M_ LaunchOffensive(MANTIS_ID, US_ATTACK);

			timer = 300;
		}
		else
			return false;
	}

	return true;
}

//--------------------------------------------------------------------------//

CQSCRIPTPROGRAM(TM15_AltoAttack, TM15_AltoAttack_Save, 0);

void TM15_AltoAttack::Initialize (U32 eventFlags, const MPartRef & part)
{
	timer = 0;

	Spawn = M_ GetPartByName("AltoSpawn");
}

bool TM15_AltoAttack::Update (void)
{
	if(data.mission_over == true)
	{
		return false;
	}

	timer -= ELAPSED_TIME;

	if(timer < 0 && CountObjectsInRange(0, PLAYER_ID, Spawn, 5) == 0 && M_ GetUsedCommandPoints(MANTIS_ID) < M_ GetMaxCommandPoints(MANTIS_ID))
	{
		if(CountObjects(M_NIAD, MANTIS_ID, ALTO) > 0)
		{
			if(!PlayerInSystem(PLAYER_ID, DECI) && !PlayerInSystem(PLAYER_ID, CENTEL)) //if player isnt in the systems under attack, dont attack
			{
				return true;
			}

			if(data.AltoTargetArray[1][0] && data.AltoTargetArray[1][1])
				Target = rand() % 2;
			else
			{
				if(data.AltoTargetArray[1][0])
					Target = 0;
				else
					Target = 1;
			}

			for (int x=0; x<3; x++)
			{
				M_ CreatePart("GBOAT!!M_Scarab", Spawn, MANTIS_ID);
			}
			
			M_ SetEnemyAITarget(MANTIS_ID, data.AltoTargetPlanet[Target], 50, data.AltoTargetArray[0][Target]);
			M_ LaunchOffensive(MANTIS_ID, US_ATTACK);

			timer = 300;
		}
		else
			return false;
	}

	return true;
}

//--------------------------------------------------------------------------//

CQSCRIPTPROGRAM(TM15_FaradAttack, TM15_FaradAttack_Save, 0);

void TM15_FaradAttack::Initialize (U32 eventFlags, const MPartRef & part)
{
	timer = 0;

	Spawn = M_ GetPartByName("FaradSpawn");
}

bool TM15_FaradAttack::Update (void)
{
	if(data.mission_over == true)
	{
		return false;
	}

	timer -= ELAPSED_TIME;

	if(timer < 0 && CountObjectsInRange(0, PLAYER_ID, Spawn, 5) == 0 && M_ GetUsedCommandPoints(MANTIS_ID) < M_ GetMaxCommandPoints(MANTIS_ID))
	{
		if(CountObjects(M_NIAD, MANTIS_ID, FARAD) > 0)
		{
			if(!PlayerInSystem(PLAYER_ID, ALTO) && !PlayerInSystem(PLAYER_ID, CENTEL)) //if player isnt in the systems under attack, dont attack
			{
				return true;
			}

			if(data.FaradTargetArray[1][0] && data.FaradTargetArray[1][1])
				Target = rand() % 2;
			else
			{
				if(data.FaradTargetArray[1][0])
					Target = 0;
				else
					Target = 1;
			}

			for (int x=0; x<3; x++)
			{
				M_ CreatePart("GBOAT!!M_Scarab", Spawn, MANTIS_ID);
			}
			
			M_ SetEnemyAITarget(MANTIS_ID, data.FaradTargetPlanet[Target], 50, data.FaradTargetArray[0][Target]);
			M_ LaunchOffensive(MANTIS_ID, US_ATTACK);

			timer = 300;
		}
		else
			return false;
	}

	return true;
}

//--------------------------------------------------------------------------//

CQSCRIPTPROGRAM(TM15_DeciAttack, TM15_DeciAttack_Save, 0);

void TM15_DeciAttack::Initialize (U32 eventFlags, const MPartRef & part)
{
	timer = 0;

	Spawn = M_ GetPartByName("DeciSpawn");
}

bool TM15_DeciAttack::Update (void)
{
	if(data.mission_over == true)
	{
		return false;
	}

	timer -= ELAPSED_TIME;

	if(timer < 0 && CountObjectsInRange(0, PLAYER_ID, Spawn, 5) == 0 && M_ GetUsedCommandPoints(MANTIS_ID) < M_ GetMaxCommandPoints(MANTIS_ID))
	{
		if(CountObjects(M_NIAD, MANTIS_ID, DECI) > 0)
		{
			if(!PlayerInSystem(PLAYER_ID, ALTO) && !PlayerInSystem(PLAYER_ID, CENTEL)) //if player isnt in the systems under attack, dont attack
			{
				return true;
			}

			if(data.DeciTargetArray[1][0] && data.DeciTargetArray[1][1])
				Target = rand() % 2;
			else
			{
				if(data.DeciTargetArray[1][0])
					Target = 0;
				else
					Target = 1;
			}

			for (int x=0; x<3; x++)
			{
				M_ CreatePart("GBOAT!!M_Scarab", Spawn, MANTIS_ID);
			}
			
			M_ SetEnemyAITarget(MANTIS_ID, data.DeciTargetPlanet[Target], 50, data.DeciTargetArray[0][Target]);
			M_ LaunchOffensive(MANTIS_ID, US_ATTACK);

			timer = 300;
		}
		else
			return false;
	}

	return true;
}

//-----------------------------------------------------------------------
// OBJECT DESTRUCTION TRIGGER

CQSCRIPTPROGRAM(TM15_ObjectDestroyed, TM15_ObjectDestroyed_Save,CQPROGFLAG_OBJECTDESTROYED);

void TM15_ObjectDestroyed::Initialize (U32 eventFlags, const MPartRef & part)
{
	if(data.mission_over)
	{
		return;
	}

	switch (part->mObjClass)
	{
		
		case M_DREADNOUGHT:

			if(part == data.Halsey)
			{
				data.HalseyDead = true;

				M_ FlushStreams();
        	    MScript::MoveCamera( part, 0, MOVIE_CAMERA_JUMP_TO);

//				data.mhandle = M_ PlayTeletype(IDS_TM15_STEELE_M15ST10, SuperLeft, SuperTop, SuperRight, SuperBottom, MOTextColor, SuperHoldTime, TELETYPE_TIME, false);
				data.mhandle = M_ PlayAnimatedMessage("M15st17.wav", "Animate!!Steele2", MagpieLeft, MagpieTop, data.Steele, IDS_TM15_SUB_M15ST17);

				data.failureID = IDS_TM15_FAIL_HALSEY_LOST;

				M_ RunProgramByName("TM15_MissionFailure", MPartRef());
			}

			break;		

		case M_CARRIER:

			if(part == data.Takei)
			{
				M_ FlushStreams();
        	    MScript::MoveCamera( part, 0, MOVIE_CAMERA_JUMP_TO);

//				data.mhandle = M_ PlayTeletype(IDS_TM15_STEELE_M15ST10, SuperLeft, SuperTop, SuperRight, SuperBottom, MOTextColor, SuperHoldTime, TELETYPE_TIME, false);
				data.mhandle = M_ PlayAnimatedMessage("M15st17.wav", "Animate!!Steele2", MagpieLeft, MagpieTop, data.Steele, IDS_TM15_SUB_M15ST17);

				data.failureID = IDS_TM15_FAIL_TAKEI_LOST;

				M_ RunProgramByName("TM15_MissionFailure", MPartRef());
			}

			break;

		case M_LANCER:
			if(part == data.Benson)
			{
				M_ FlushStreams();
        	    MScript::MoveCamera( part, 0, MOVIE_CAMERA_JUMP_TO);

//				data.mhandle = M_ PlayTeletype(IDS_TM15_STEELE_M15ST10, SuperLeft, SuperTop, SuperRight, SuperBottom, MOTextColor, SuperHoldTime, TELETYPE_TIME, false);
				data.mhandle = M_ PlayAnimatedMessage("M15st17.wav", "Animate!!Steele2", MagpieLeft, MagpieTop, data.Steele, IDS_TM15_SUB_M15ST17);

				data.failureID = IDS_TM15_FAIL_BENSON_LOST;

				M_ RunProgramByName("TM15_MissionFailure", MPartRef());
			}

			break;

		case M_BATTLESHIP:

			if(part == data.Steele)
			{
				M_ FlushStreams();
        	    MScript::MoveCamera( part, 0, MOVIE_CAMERA_JUMP_TO);

//				data.mhandle = M_ PlayTeletype(IDS_TM15_HALSEY_M15HA11, SuperLeft, SuperTop, SuperRight, SuperBottom, MOTextColor, SuperHoldTime, TELETYPE_TIME, false);
				data.mhandle = M_ PlayAnimatedMessage("M15HA18.wav", "Animate!!Halsey2", MagpieLeft, MagpieTop, IDS_TM15_SUB_M15HA18);

				data.failureID = IDS_TM15_FAIL_STEELE_LOST;

				M_ RunProgramByName("TM15_MissionFailure", MPartRef());
			}



			break;

		case M_MONOLITH:

			if(part == data.Vivac)
			{
				M_ FlushStreams();
        	    MScript::MoveCamera( part, 0, MOVIE_CAMERA_JUMP_TO);

//				data.mhandle = M_ PlayTeletype(IDS_TM15_STEELE_M15ST12, SuperLeft, SuperTop, SuperRight, SuperBottom, MOTextColor, SuperHoldTime, TELETYPE_TIME, false);
				data.mhandle = M_ PlayAnimatedMessage("M15st19.wav", "Animate!!Steele2", MagpieLeft, MagpieTop, data.Steele, IDS_TM15_SUB_M15ST19);

				data.failureID = IDS_TM15_FAIL_VIVAC_LOST;

				M_ RunProgramByName("TM15_MissionFailure", MPartRef());
			}

			break;

	}
}

bool TM15_ObjectDestroyed::Update (void)
{
	return false;
}

//-----------------------------------------------------------------------
// UNDER ATTACK EVENT

CQSCRIPTPROGRAM(TM15_UnderAttack, TM15_UnderAttack_Save, CQPROGFLAG_UNDERATTACK);

void TM15_UnderAttack::Initialize (U32 eventFlags, const MPartRef & part)
{
	if(data.mission_over)
	{
		return;
	}

	if(part->systemID == ALTO && part->playerID == MANTIS_ID && !data.AltoFlag)
	{
		if(!data.FaradHasAttacked)
		{
			M_ RunProgramByName("TM15_FaradAttack", MPartRef());

			data.FaradHasAttacked = true;
			data.FaradTargetArray[1][0] = true;
		}
			data.FaradTargetArray[1][0] = true;

		if(!data.CentelHasAttacked)
		{
			M_ RunProgramByName("TM15_CentelAttack", MPartRef());

			data.CentelHasAttacked = true;
			data.CentelTargetArray[1][1] = true;
		}
		else
			data.CentelTargetArray[1][1] = true;


		if(!data.DeciHasAttacked)
		{
			M_ RunProgramByName("TM15_DeciAttack", MPartRef());

			data.DeciHasAttacked = true;
			data.DeciTargetArray[1][0] = true;
		}
		else
			data.DeciTargetArray[1][0] = true;

		data.AltoFlag = true;
	}

	if(part->systemID == CENTEL && part->playerID == MANTIS_ID && !data.CentelFlag)
	{
		if(!data.FaradHasAttacked)
		{
			M_ RunProgramByName("TM15_FaradAttack", MPartRef());

			data.FaradHasAttacked = true;
			data.FaradTargetArray[1][1] = true;
		}
		else
			data.FaradTargetArray[1][1] = true;

		if(!data.MalotiHasAttacked)
		{
			M_ RunProgramByName("TM15_MalotiAttack", MPartRef());

			data.MalotiHasAttacked = true;
			data.MalotiTargetArray[1][1] = true;
		}
		else
			data.MalotiTargetArray[1][1] = true;


		if(!data.DeciHasAttacked)
		{
			M_ RunProgramByName("TM15_DeciAttack", MPartRef());

			data.DeciHasAttacked = true;
			data.DeciTargetArray[1][1] = true;
		}
		else
			data.DeciTargetArray[1][1] = true;

		if(!data.AltoHasAttacked)
		{
			M_ RunProgramByName("TM15_AltoAttack", MPartRef());

			data.AltoHasAttacked = true;
			data.AltoTargetArray[1][1] = true;
		}
		else
			data.AltoTargetArray[1][1] = true;

		if(!data.VoxHasAttacked)
		{
			M_ RunProgramByName("TM15_VoxAttack", MPartRef());

			data.VoxHasAttacked = true;
			data.VoxTargetArray[1][3] = true;
		}
		else
			data.VoxTargetArray[1][3] = true;

		data.CentelFlag = true;
	}


}

bool TM15_UnderAttack::Update (void)
{
	return false;
}

//-----------------------------------------------------------------------
//OBJECT CONSTRUCTED EVENT

CQSCRIPTPROGRAM(TM15_ObjectConstructed, TM15_ObjectConstructed_Save, CQPROGFLAG_OBJECTCONSTRUCTED);

void TM15_ObjectConstructed::Initialize (U32 eventFlags, const MPartRef & part)
{
	if(data.mission_over)
	{
		return;
	}

	if(part->systemID == ALTO && part->playerID == PLAYER_ID && !data.MantisLastStand)
	{
		MassEnableAI(MANTIS_ID, false, 0);

		M_ SetEnemyAITarget(MANTIS_ID, M_ GetPartByName("Alto Prime"), 50, ALTO);
		M_ LaunchOffensive(MANTIS_ID, US_ATTACK);

		MassEnableAI(MANTIS_ID, true, 0);
		data.MantisLastStand = true;
	}

	if(part->systemID == NACH && part->playerID == PLAYER_ID && !data.BeatusHasAttacked)
	{
		M_ RunProgramByName("TM15_BeatusAttack", MPartRef());

		data.BeatusHasAttacked = true;
	}

	if(part->systemID == BEATUS && part->playerID == PLAYER_ID)
	{
		if(!data.JovianHasAttacked)
		{
			M_ RunProgramByName("TM15_JovianAttack", MPartRef());

			data.JovianHasAttacked = true;
			data.JovianTargetArray[1][0] = true;
		}
		else
			data.JovianTargetArray[1][0] = true;


		if(!data.AmpereHasAttacked)
		{
			M_ RunProgramByName("TM15_AmpereAttack", MPartRef());

			data.AmpereHasAttacked = true;
			data.AmpereTargetArray[1][0] = true;
		}
		else
			data.AmpereTargetArray[1][0] = true;

	}

	if(part->systemID == AMPERE && part->playerID == PLAYER_ID)
	{
		if(!data.JovianHasAttacked)
		{
			M_ RunProgramByName("TM15_JovianAttack", MPartRef());

			data.JovianHasAttacked = true;
			data.JovianTargetArray[1][1] = true;
		}
		else
			data.JovianTargetArray[1][1] = true;


		if(!data.VoxHasAttacked)
		{
			M_ RunProgramByName("TM15_VoxAttack", MPartRef());

			data.VoxHasAttacked = true;
			data.VoxTargetArray[1][0] = true;
		}
		else
			data.VoxTargetArray[1][0] = true;
	}

	if(part->systemID == JOVIAN && part->playerID == PLAYER_ID)
	{
		if(!data.UstedHasAttacked)
		{
			M_ RunProgramByName("TM15_UstedAttack", MPartRef());

			data.UstedHasAttacked = true;
		}

		if(!data.AmpereHasAttacked)
		{
			M_ RunProgramByName("TM15_AmpereAttack", MPartRef());

			data.AmpereHasAttacked = true;
			data.AmpereTargetArray[1][1] = true;
		}
		else
			data.AmpereTargetArray[1][1] = true;


		if(!data.VoxHasAttacked)
		{
			M_ RunProgramByName("TM15_VoxAttack", MPartRef());

			data.VoxHasAttacked = true;
			data.VoxTargetArray[1][1] = true;
		}
		else
			data.VoxTargetArray[1][1] = true;
	}

	if(part->systemID == USTED && part->playerID == PLAYER_ID)
	{
		if(!data.MalotiHasAttacked)
		{
			M_ RunProgramByName("TM15_MalotiAttack", MPartRef());

			data.MalotiHasAttacked = true;
			data.MalotiTargetArray[1][0] = true;
		}
		else
			data.MalotiTargetArray[1][0] = true;
	}

	if(part->systemID == DECI && part->playerID == PLAYER_ID)
	{
		if(!data.VoxHasAttacked)
		{
			M_ RunProgramByName("TM15_VoxAttack", MPartRef());

			data.VoxHasAttacked = true;
			data.VoxTargetArray[1][2] = true;
		}
		else
			data.VoxTargetArray[1][2] = true;

		if(!data.CentelHasAttacked)
		{
			M_ RunProgramByName("TM15_CentelAttack", MPartRef());

			data.CentelHasAttacked = true;
			data.CentelTargetArray[1][0] = true;
		}
		else
			data.CentelTargetArray[1][0] = true;


		if(!data.AltoHasAttacked)
		{
			M_ RunProgramByName("TM15_AltoAttack", MPartRef());

			data.AltoHasAttacked = true;
			data.AltoTargetArray[1][0] = true;
		}
		else
			data.AltoTargetArray[1][0] = true;
	}

}

bool TM15_ObjectConstructed::Update (void)
{
	return false;
}


//---------------------------------------------------------------------------
// MISSION FAILURE PROGRAMS


//---------------------------------------------------------------------------

#define TELETYPE_TIME 3000

CQSCRIPTPROGRAM(TM15_MissionFailure, TM15_MissionFailure_Save, 0);

void TM15_MissionFailure::Initialize (U32 eventFlags, const MPartRef & part)
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

bool TM15_MissionFailure::Update (void)
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
			if (!M_ IsStreamPlaying(data.shandle))
			{
				M_ FlushTeletype();
//				data.mhandle = M_ PlayTeletype(IDS_TM15_HALSEY_M15HA13, SuperLeft, SuperTop, SuperRight, SuperBottom, MOTextColor, 6000, 1000, false);
				data.mhandle = M_ PlayAnimatedMessage("M15HA20.wav", "Animate!!Halsey2", MagpieLeft, MagpieTop, IDS_TM15_SUB_M15HA20);

				state = Fail;
			}
			break;

		case Fail:
			if (!M_ IsStreamPlaying(data.mhandle))
			{
				M_ FlushTeletype();
				data.mhandle = TeletypeMissionOver(IDS_TM15_MISSION_FAILURE, data.failureID);
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

CQSCRIPTPROGRAM(TM15_MissionSuccess, TM15_MissionSuccess_Save, 0);

void TM15_MissionSuccess::Initialize (U32 eventFlags, const MPartRef & part)
{
	state = Uplink;

}

bool TM15_MissionSuccess::Update (void)
{
	switch (state)
	{
		case Uplink:
			if(data.mission_over)
			{
				return false;
			}
			if(!M_ PlayerHasPlatforms(MANTIS_ID))
			{
				M_ MarkObjectiveCompleted(IDS_TM15_OBJECTIVE1);
			}
			if(CountObjects(M_TENDER, PLAYER_ID, ALTO) >= 2 && CountObjects(M_REFINERY, PLAYER_ID, ALTO) >= 2 && CountObjects(M_HQ, PLAYER_ID, ALTO) >= 1)
			{
				M_ MarkObjectiveCompleted(IDS_TM15_OBJECTIVE4);
			}
		
			if (!M_ IsStreamPlaying(data.mhandle) && M_ IsObjectiveCompleted(IDS_TM15_OBJECTIVE1) && M_ IsObjectiveCompleted(IDS_TM15_OBJECTIVE4))
			{
				M_ PlayMusic(VICTORY);

				M_ EnableMovieMode(true);

				UnderMissionControl();	
				data.mission_over = true;

//				data.shandle = M_ PlayAudio("high_level_data_uplink.wav");
				state = Halsey;
			}
			break;


		case Halsey:	
			if (!M_ IsStreamPlaying(data.shandle))
			{
				M_ FlushTeletype();
//				data.mhandle = M_ PlayTeletype(IDS_TM15_HALSEY_M15HA09, SuperLeft, SuperTop, SuperRight, SuperBottom, MOTextColor, 5000, 2000, false);
				data.mhandle = M_ PlayAnimatedMessage("M15HA16.wav", "Animate!!Halsey2", MagpieLeft, MagpieTop, IDS_TM15_SUB_M15HA16);
				
				state = Success;
			}
			break;

		case Success:
			
			if(!M_ IsStreamPlaying(data.mhandle))
			{
				M_ FlushTeletype();
				data.mhandle = TeletypeMissionOver(IDS_TM15_MISSION_SUCCESS);

				state = Done;
			}
			break;

		case Done:

			UnderPlayerControl();
			M_ EndMissionVictory(14);
			return false;

			break;
	}

	return true;
}

//--------------------------------------------------------------------------//
//--------------------------------End Script15T.cpp-------------------------//
//--------------------------------------------------------------------------//
