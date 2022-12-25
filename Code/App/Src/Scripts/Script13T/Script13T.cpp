//--------------------------------------------------------------------------//
//                                                                          //
//                                Script13T.cpp                             //
//				/Conquest/App/Src/Scripts/Script02T/Script13T.cpp			//
//								MISSION PROGRAMS							//
//                                                                          //
//               COPYRIGHT (C) 2000 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
	Created:	6/19/00		JeffP
	Modified:	6/19/00		JeffP

*/
//--------------------------------------------------------------------------//


#include <ScriptDef.h>
#include <DLLScriptMain.h>
#include "DataDef.h"
#include "stdlib.h"
#include <stdio.h>
#include <time.h>

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


#define TIMERS_PER_MIN 60	//240

#define STARTING_EARTH_CREW  50000


//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//

void SetTechLevel(U32 stage)
{
	TECHNODE mission_tech, mantis_tech;
	mission_tech.InitLevel(TECHTREE::FULL_TREE);

	mantis_tech = M_ GetPlayerTech(MANTIS_ID);

	if (stage == 1)
	{
		// 0 is the Terran players race...
		mission_tech.race[0].build = mission_tech.race[1].build =
			(TECHTREE::BUILDNODE) 
			(TECHTREE::TDEPEND_HEADQUARTERS | TECHTREE::TDEPEND_REFINERY | TECHTREE::TDEPEND_LIGHT_IND | 
			 /*TECHTREE::TDEPEND_JUMP_INHIBITOR |*/ TECHTREE::TDEPEND_LASER_TURRET | TECHTREE::TDEPEND_TENDER | 
			 TECHTREE::TDEPEND_SENSORTOWER | TECHTREE::TDEPEND_OUTPOST | TECHTREE::TDEPEND_BALLISTICS |
			 TECHTREE::TDEPEND_HEAVY_IND | TECHTREE::TDEPEND_ADVHULL | TECHTREE::TDEPEND_ACADEMY |
			 TECHTREE::TDEPEND_HANGER | TECHTREE::TDEPEND_SPACE_STATION | TECHTREE::TDEPEND_REPAIR | 
			 TECHTREE::TDEPEND_PROPLAB | TECHTREE::TDEPEND_AWSLAB | TECHTREE::TDEPEND_DISPLACEMENT |
			 TECHTREE::RES_REFINERY_GAS1 | TECHTREE::RES_REFINERY_METAL1 | TECHTREE::RES_REFINERY_METAL2 |
			 TECHTREE::RES_REFINERY_GAS2 );
        
        mission_tech.race[1].build = (TECHTREE::BUILDNODE) ( mission_tech.race[1].build
            | TECHTREE::MDEPEND_PLANTATION | TECHTREE::MDEPEND_BIOFORGE );

	    mission_tech.race[2].build = 
            (TECHTREE::BUILDNODE) ( ( TECHTREE::ALL_BUILDNODE ^ 
            TECHTREE::SDEPEND_PORTAL ) | 
            TECH_TREE_RACE_BITS_ALL );

		mission_tech.race[0].tech = mission_tech.race[1].tech = 
			(TECHTREE::TECHUPGRADE) 
			(TECHTREE::T_SHIP__FABRICATOR | TECHTREE::T_SHIP__CORVETTE | TECHTREE::T_SHIP__HARVEST | 
			 TECHTREE::T_SHIP__MISSILECRUISER | TECHTREE::T_SHIP__TROOPSHIP | TECHTREE::T_RES_TROOPSHIP1 |
			 TECHTREE::T_SHIP__SUPPLY | TECHTREE::T_SHIP__INFILTRATOR | TECHTREE::T_SHIP__BATTLESHIP |
			 TECHTREE::T_SHIP__CARRIER | TECHTREE::T_SHIP__LANCER | TECHTREE::T_SHIP__DREADNOUGHT |
			 TECHTREE::T_RES_MISSLEPACK1 | TECHTREE::T_RES_MISSLEPACK2 | TECHTREE::T_RES_TROOPSHIP2 | 
			 TECHTREE::T_RES_XCHARGES | TECHTREE::T_RES_XPROBE | TECHTREE::T_RES_XCLOAK | TECHTREE::T_RES_XVAMPIRE | 
			 TECHTREE::T_RES_XSHIELD);

		mantis_tech.race[1].tech =
			(TECHTREE::TECHUPGRADE)
			(TECHTREE::M_RES_LEECH1 | TECHTREE::M_RES_EXPLODYRAM1 | TECHTREE::M_RES_EXPLODYRAM2);

		mission_tech.race[0].common = mission_tech.race[1].common =
			(TECHTREE::COMMON)
			(TECHTREE::RES_SUPPLY1 | TECHTREE::RES_WEAPONS1 | TECHTREE::RES_SUPPLY2 | TECHTREE::RES_WEAPONS2 |
			 TECHTREE::RES_HULL1 | TECHTREE::RES_HULL2 | TECHTREE::RES_HULL3 | TECHTREE::RES_ENGINE1 | 
			 TECHTREE::RES_ENGINE2 | TECHTREE::RES_HULL4 | TECHTREE::RES_ENGINE3 | TECHTREE::RES_SHIELDS1 |
			 TECHTREE::RES_SHIELDS2 | TECHTREE::RES_SHIELDS3 | TECHTREE::RES_SHIELDS4 | TECHTREE::RES_ENGINE4 |
			 TECHTREE::RES_WEAPONS3 | TECHTREE::RES_WEAPONS4 | TECHTREE::RES_SUPPLY3 | TECHTREE::RES_SUPPLY4);
		mission_tech.race[0].common_extra = mission_tech.race[1].common_extra =
			(TECHTREE::COMMON_EXTRA)
			(TECHTREE::RES_TENDER1 | TECHTREE::RES_TANKER1 | TECHTREE::RES_TENDER2 | TECHTREE::RES_TANKER2 | 
			 TECHTREE::RES_SENSORS1 | TECHTREE::RES_FIGHTER1 | TECHTREE::RES_FIGHTER2 | TECHTREE::RES_FIGHTER3 |
			 TECHTREE::RES_SENSORS2); 
	}

	M_ SetAvailiableTech(mission_tech);

	M_ SetPlayerTech(MANTIS_ID, mantis_tech);
}

//--------------------------------------------------------------------------//
//  MISSION PROGRAM 														//
//--------------------------------------------------------------------------//

CQSCRIPTPROGRAM(TM13_Start, TM13_Start_Data,CQPROGFLAG_STARTBRIEFING);
CQSCRIPTDATA(MissionData, data);

void TM13_Start::Initialize (U32 eventFlags, const MPartRef & part)
{
	data.textFailureID = 0;
	data.mission_over = data.briefing_over = data.ai_enabled = data.next_state = false;
	data.bFinalAttack = false;
	data.mhandle = 0;
	data.shandle = 0;
	data.failureID = 0;

	U32 x;
	for (x=0; x<NUM_SYSTEMS; x++)
	{
		data.bAIOnInSystem[x] = false;
	}
	
	data.mission_state = Begin;

	M_ PlayMusic ( NULL );

	M_ EnableBriefingControls(true);

	M_ RunProgramByName("TM13_Briefing", MPartRef ());
}

bool TM13_Start::Update (void)
{
	return false;
}

//--------------------------------------------------------------------------
//	MISSION BRIEFING
//--------------------------------------------------------------------------

CQSCRIPTPROGRAM(TM13_Briefing, TM13_Briefing_Data,0);

void TM13_Briefing::Initialize (U32 eventFlags, const MPartRef & part)
{
	data.mission_state = Briefing;
	state = Begin;

}

bool TM13_Briefing::Update (void)
{
	if (data.briefing_over)
		return false;

	switch(state)
	{
		case Begin:
			
			state = TeleTypeLocation;
			//state = NewsPerson;
			break;

		case TeleTypeLocation:
		
			M_ FlushTeletype();
			data.mhandle = TeletypeBriefing(IDS_TM13_TELETYPE_LOCATION, false);
		    //data.mhandle = M_ PlayBriefingTeletype(IDS_TM13_TELETYPE_LOCATION, MOTextColor, 8000, TEXT_PRINT_TIME, false);
			
			state = RadioProlog;
			break;

		case RadioProlog:

			//if (!M_ IsTeletypePlaying(data.mhandle))
			//{
				data.mhandle = M_ PlayAudio("PROLOG13.wav");
				MScript::BriefingSubtitle(data.mhandle,IDS_TM13_SUB_PROLOG13);
				//data.mhandle = DoBriefingSpeech(IDS_TM13_NEWS_M13NP01, "M13NP01.wav", 8000, 2000, 0, CHAR_NOCHAR);

				ShowBriefingAnimation(0, "TNRLogo", ANIMSPD_TNRLOGO, true, true);
				ShowBriefingAnimation(3, "TNRLogo", ANIMSPD_TNRLOGO, true, true);
				ShowBriefingAnimation(1, "Radiowave", ANIMSPD_RADIOWAVE, true, true);
				ShowBriefingAnimation(2, "Radiowave", ANIMSPD_RADIOWAVE, true, true);

				state = PreHalsey;
			//}
			break;
		
		case PreHalsey:

			if (!M_ IsStreamPlaying(data.mhandle))
			{
				M_ FreeBriefingSlot(-1);

				M_ PlayAudio("high_level_data_uplink.wav");

				ShowBriefingAnimation(0, "Xmission", ANIMSPD_XMISSION, false, false); 
			//	ShowBriefingAnimation(3, "Xmission", ANIMSPD_XMISSION, false, false); 
				state = Halsey;
			}
			break;

		case Halsey:

			if (M_ IsCustomBriefingAnimationDone(0))	
			{
				data.mhandle = DoBriefingSpeechMagpie(IDS_TM13_SUB_M13HA02, "M13HA02.wav", 8000, 2000, 0, CHAR_HALSEY);

//				ShowBriefingHead(3, CHAR_HAWKES);

				state = Hawkes;
			}
			break;

		case Hawkes:

			if (!IsSomethingPlaying(data.mhandle))
			{
//				data.mhandle = DoBriefingSpeech(IDS_TM13_HAWKES_M13HW03, "M13HW03.wav", 8000, 2000, 3, CHAR_HAWKES);
				state = Objectives;
			}
			break;

		case Objectives:
		
			if (!IsSomethingPlaying(data.mhandle))
			{
				M_ FreeBriefingSlot(0);
				M_ FreeBriefingSlot(3);

				M_ FlushTeletype();
				data.mhandle = TeletypeBriefing(IDS_TM13_OBJECTIVES, true);
				//data.mhandle = M_ PlayBriefingTeletype(IDS_TM13_OBJECTIVES, MOTextColor, TEXT_HOLD_TIME_INF, 3000, false);
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

void TurnOnAIForSystem(U32 SystemID)
{
	if (!data.bAIOnInSystem[SystemID])
	{
		MassEnableAI(MANTIS_ID, true, SystemID);
		data.bAIOnInSystem[SystemID] = true;
	}
}

void TweakMantisAI()
{
	U32 gas, metal, crew;

	gas = M_ GetGas(MANTIS_ID);
	metal = M_ GetMetal(MANTIS_ID);
	crew = M_ GetCrew(MANTIS_ID);

	if (gas < 240) gas = 1000;
	if (metal < 240) metal = 1000;
	if (crew < 200) crew = 1000;

	M_ SetGas(MANTIS_ID, gas);
	M_ SetMetal(MANTIS_ID, metal);
	M_ SetCrew(MANTIS_ID, crew);
}

//--------------------------------------------------------------------------//
//  WILL ALSO NOW TAKE CARE OF OTHER GLOBAL THINGS

void AddEnemy(MPartRef enemy, U32 type)
{
    if ( data.numEnemy < MAX_WAVE )
    {
	    data.enemyShips[data.numEnemy] = enemy;
	    data.enemyShipType[data.numEnemy] = type;
	    data.enemyShipTimer[data.numEnemy] = 0;
	    data.numEnemy++;
    }
}

CQSCRIPTPROGRAM(TM13_ControlWormholes, TM13_ControlWormholes_Data, 0);

void TM13_ControlWormholes::Initialize (U32 eventFlags, const MPartRef & part)
{
	
}


MPartRef FindClosestPlatform(MPartRef loc);
MPartRef FindClosestPlatformHQ(MPartRef loc);
MPartRef FindClosestOpenPlanet(MPartRef loc);

bool TM13_ControlWormholes::Update (void)
{
	U32 x;
	S32 crew;
	MPartRef tmpobj, blast;
	bool bRemoveIt=false;

	if (data.mission_over)
		return false;

	// HIDE WORMHOLES
	/*
	for (x=0; x<7; x++)
		if (!data.bGateOn[x])
			M_ ClearVisibility(data.gateWaveEntry[x]);
	*/

	for (x=0; x<data.numEnemy; x++)
	{
		tmpobj = data.enemyShips[x];

		if (!tmpobj.isValid() || M_ IsDead(tmpobj)) bRemoveIt = true;

		//TODO  temp-ish
		/*
		if (tmpobj->systemID == 1 && data.enemyShipTimer[x] == 0)
		{
			blast = M_ CreateWormBlast(loc, (SINGLE)radius, MANTIS_ID, true);  //bSuckOut
			M_ FlashWormBlast(blast);
			//M_ CloseWormBlast(blast);
		}
		*/

		if (!bRemoveIt && tmpobj->systemID == 1) 
			data.enemyShipTimer[x]++;

		if (!bRemoveIt && (tmpobj->systemID == 1 && data.enemyShipTimer[x] >= 16))
		{
			if (data.enemyShipType[x] == ATTACK_SHIP)
			{
				if(data.HQattack)
				{
					M_ SetStance(tmpobj, US_ATTACK);
					M_ OrderAttack(tmpobj, FindClosestPlatformHQ(tmpobj));
				}
				else
				{
					M_ SetStance(tmpobj, US_ATTACK);
					M_ OrderAttack(tmpobj, FindClosestPlatform(tmpobj));
				}
			}
			else if (data.enemyShipType[x] == FAB_SHIP)
				M_ OrderMoveTo(tmpobj, FindClosestOpenPlanet(tmpobj));
			else if (data.enemyShipType[x] == TROOP_SHIP)
				M_ OrderSpecialAttack(tmpobj, FindClosestPlatform(tmpobj));
			else if (data.enemyShipType[x] == RAM_SHIP)
				M_ OrderSpecialAttack(tmpobj, FindClosestPlatformHQ(tmpobj));

			bRemoveIt = true;
		}

		if (bRemoveIt)
		{
			if (x < data.numEnemy-1)
			{
				data.enemyShips[x] = data.enemyShips[data.numEnemy-1];	
				data.enemyShipType[x] = data.enemyShipType[data.numEnemy-1];	
				data.enemyShipTimer[x] = data.enemyShipTimer[data.numEnemy-1];	
			}
			data.numEnemy--;
			x--;
			bRemoveIt = false;
		}
	}

	// FIX CREW ON EARTH
	if (data.numTotalCrew > 0)
	{
		crew = M_ GetCrewOnPlanet(data.Earth);
		if (crew <= (4000/5))
		{
			S32 numAdd;
			if (data.numTotalCrew >= 1000)
				numAdd = 1000;
			else
				numAdd = data.numTotalCrew;
			M_ SetResourcesOnPlanet(data.Earth, -1, -1, crew+(numAdd/5) );
			data.numTotalCrew -= numAdd;
		}
	}

	return true;
}

//--------------------------------------------------------------------------//

CQSCRIPTPROGRAM(TM13_PrintTimeTillAttack, TM13_PrintTimeTillAttack_Data, 0);

void TM13_PrintTimeTillAttack::Initialize (U32 eventFlags, const MPartRef & part)
{
	char buf[128];
	
	sprintf(buf, "Attack Timer value = %d \n", data.attackTimer);
	TextOutput(buf);
	sprintf(buf, "  wave attacks at %d, %d, %d, %d, %d, %d, %d, %d \n", 
		3 * TIMERS_PER_MIN, 8 * TIMERS_PER_MIN, 15 * TIMERS_PER_MIN, 20 * TIMERS_PER_MIN,
		25 * TIMERS_PER_MIN, 28 * TIMERS_PER_MIN, 33 * TIMERS_PER_MIN, 40 * TIMERS_PER_MIN);
	TextOutput(buf);
}

bool TM13_PrintTimeTillAttack::Update (void)
{
	return false;
}

//--------------------------------------------------------------------------

CQSCRIPTPROGRAM(TM13_MissionStart, TM13_MissionStart_Data,CQPROGFLAG_STARTMISSION);

void TM13_MissionStart::Initialize (U32 eventFlags, const MPartRef & part)
{
	data.mission_state = MissionStart;
	state = Begin;
	data.briefing_over = true;

	data.numEnemy = 0;

	M_ SetMissionID(12);
	M_ SetMissionName(IDS_TM13_MISSION_NAME);
	M_ SetMissionDescription(IDS_TM13_MISSION_DESC);

	M_ AddToObjectiveList(IDS_TM13_OBJECTIVE1);
	M_ AddToObjectiveList(IDS_TM13_OBJECTIVE2);
	M_ AddToObjectiveList(IDS_TM13_OBJECTIVE3);

	data.Blackwell = M_ GetPartByName("Blackwell");
	data.Hawkes = M_ GetPartByName("Hawkes");

	M_ EnablePartnameDisplay(data.Blackwell, true);
	M_ EnablePartnameDisplay(data.Hawkes, true);

	data.locFleetEntry = M_ GetPartByName("locTauCetiWormhole");
	data.Earth = M_ GetPartByName("Earth");
	data.Mars = M_ GetPartByName("Mars");
	data.Mercury = M_ GetPartByName("Mercury");
	data.Venus = M_ GetPartByName("Venus");
	data.Jupiter = M_ GetPartByName("Jupiter");
	data.Saturn = M_ GetPartByName("Saturn");
	data.Neptune = M_ GetPartByName("Neptune");
	data.Pluto = M_ GetPartByName("Pluto");
	data.locInner = M_ GetPartByName("locInnerPlanets");

	data.numTotalCrew = STARTING_EARTH_CREW;

	/*
	char tmpstr[20];
	U32 x;
	for (x=0; x<7; x++)
	{
		sprintf(tmpstr, "locWaveEntry%d", x+1);
		data.locWaveEntry[x] = M_ GetPartByName(tmpstr);
		data.bGateOn[x] = false;
	}
	data.gateWaveEntry[0] = M_ GetPartByName("Gate13");
	M_ EnableJumpgate(data.gateWaveEntry[0], false);
	M_ EnableSelection(data.gateWaveEntry[0], false);
	data.gateWaveEntry[1] = M_ GetPartByName("Gate9");
	M_ EnableJumpgate(data.gateWaveEntry[1], false);
	M_ EnableSelection(data.gateWaveEntry[1], false);
	data.gateWaveEntry[2] = M_ GetPartByName("Gate15");
	M_ EnableJumpgate(data.gateWaveEntry[2], false);
	M_ EnableSelection(data.gateWaveEntry[2], false);
	data.gateWaveEntry[3] = M_ GetPartByName("Gate7");
	M_ EnableJumpgate(data.gateWaveEntry[3], false);
	M_ EnableSelection(data.gateWaveEntry[3], false);
	data.gateWaveEntry[4] = M_ GetPartByName("Gate11");
	M_ EnableJumpgate(data.gateWaveEntry[4], false);
	M_ EnableSelection(data.gateWaveEntry[4], false);
	data.gateWaveEntry[5] = M_ GetPartByName("Gate5");
	M_ EnableJumpgate(data.gateWaveEntry[5], false);
	M_ EnableSelection(data.gateWaveEntry[5], false);
	data.gateWaveEntry[6] = M_ GetPartByName("Gate3");
	M_ EnableJumpgate(data.gateWaveEntry[6], false);
	M_ EnableSelection(data.gateWaveEntry[6], false);
	*/
	
	data.locWaveEntry = M_ GetPartByName("locWaveEntry2");
	data.locWaveEntry2 = M_ GetPartByName("locWaveEntry4");
	data.locWaveEntry3 = M_ GetPartByName("locWaveEntry6");
	data.locWaveEntry4 = M_ GetPartByName("locWaveEntry8");

	/*
	OK here is how you do it.  You have a dummy system off somewhere with a waypoint.  
	Create your wave of guys at the waypoint and one "PLATGUN!!S_TesPortal".  Then call 

	void MScript::OrderDoWormhole(const MPartRef & part, U32 systemID)

	giving it the portal and the target systemID.  The portal activates and bang your fleet ends up in the system.  
	You will have no control over where in the system they appear.
	*/

	M_ SetAllies(PLAYER_ID, REBEL_ID, true);
	M_ SetAllies(REBEL_ID, PLAYER_ID, true);

	M_ SetGas(PLAYER_ID, 180);
	M_ SetMetal(PLAYER_ID, 220);
	M_ SetCrew(PLAYER_ID, 200);

	M_ SetMaxGas(MANTIS_ID, 1000);
	M_ SetMaxMetal(MANTIS_ID, 1000);
	M_ SetMaxCrew(MANTIS_ID, 2000);
	M_ SetGas(MANTIS_ID, 1000);
	M_ SetMetal(MANTIS_ID, 1000);
	M_ SetCrew(MANTIS_ID, 2000);

	SetTechLevel(1);

	M_ SetEnemyCharacter(MANTIS_ID, IDS_TM13_MANTIS);

	M_ EnableSystem(1, true, true);
	M_ EnableSystem(2, false, false);
	M_ EnableJumpgate(M_ GetPartByName("Gate0"), false);

	MPartRef planet = M_ GetPartByName("Earth");
	M_ ClearHardFog(planet, 100);
	M_ MakeAreaVisible(MANTIS_ID, planet, 100);

	/*
	M_ SetResourcesOnPlanet(planet, 0, 0, -1);
	planet = M_ GetPartByName("Moon");
	M_ SetResourcesOnPlanet(planet, 0, 0, 0);
	*/

	srand(time(NULL));

	M_ EnableEnemyAI(MANTIS_ID, true, "MANTIS_FRIGATE_RUSH");
	AIPersonality airules;
	airules.difficulty = EASY; 	
	airules.buildMask.bBuildPlatforms = true;
	airules.buildMask.bBuildLightGunboats = true;
	airules.buildMask.bBuildMediumGunboats = true;
	airules.buildMask.bBuildHeavyGunboats = true;
	airules.buildMask.bHarvest = true;
	airules.buildMask.bScout = false;
	airules.buildMask.bBuildAdmirals = false;
	airules.buildMask.bLaunchOffensives = true;  
	airules.buildMask.bVisibilityRules = true;  
	airules.nNumFabricators = 3;
	airules.nBuildPatience = 0;		
	airules.nNumMinelayers = 0;		
	airules.nNumTroopships = 3;		
	M_ SetEnemyAIRules(MANTIS_ID, airules);
	data.ai_enabled = true;
    data.HQattack = false;

	//M_ SetEnemyAITarget(MANTIS_ID, MPartRef(), 0, 1);
	//MassEnableAI(MANTIS_ID, false);

	M_ EnableRegenMode(true);

	M_ ChangeCamera(M_ GetPartByName("MissionCamStart"), 0, MOVIE_CAMERA_JUMP_TO);

	M_ RunProgramByName("TM13_ControlWormholes", MPartRef ());
}

bool TM13_MissionStart::Update (void)
{
	MPartRef part;

	if (data.mission_over)
		return false;

	switch (state)
	{

		case Begin:
			if(!IsSomethingPlaying(data.mhandle))
			{
				data.mhandle = DoSpeech(IDS_TM13_SUB_M13HW03, "M13HW03.wav", 8000, 2000, data.Hawkes, CHAR_HAWKES);

				M_ ClearPath(2, 1, MANTIS_ID);
				state = Hawkes1;
			}
			break;

		case Hawkes1:

			if (!IsSomethingPlaying(data.mhandle))
			{
				data.mhandle = DoSpeech(IDS_TM13_SUB_M13BL05, "M13BL05.wav", 8000, 2000, data.Blackwell, CHAR_BLACKWELL);
				state = Blackwell1;
			}
			break;

		case Blackwell1:

			if (!IsSomethingPlaying(data.mhandle))
			{
				data.mhandle = DoSpeech(IDS_TM13_SUB_M13HW04, "M13HW04.wav", 8000, 2000, data.Hawkes, CHAR_HAWKES);
				state = Done;
			}
			break;

		case Done:

			M_ RunProgramByName("TM13_DefendEarth", MPartRef ());
			return false;

	}

	return true;
}

//--------------------------------------------------------------------------//

CQSCRIPTPROGRAM(TM13_DefendEarth, TM13_DefendEarth_Data, 0);

void TM13_DefendEarth::Initialize (U32 eventFlags, const MPartRef & part)
{
	timer = 0;
	data.attackTimer = timer;

    attackWave = 0;
}

bool TM13_DefendEarth::Update (void)
{
	if (data.mission_over)
		return false;

	timer += ELAPSED_TIME;

	data.attackTimer = timer;

    switch ( attackWave )
    {
        case 0: 

	        if ( timer >= 2 * 60 )
	        {
		        M_ RunProgramByName("TM13_SendWave_A", MPartRef());
		        TweakMantisAI();

                attackWave++;
	        }

            break;

		case 1: 

	        if ( timer >= 4 * 60 )
	        {
		        M_ RunProgramByName("TM13_SendWave_A", MPartRef());
		        TweakMantisAI();

                attackWave++;
	        }

            break;

        case 2: 

	        if ( timer >= 6 * 60 )
	        {
		        M_ RunProgramByName("TM13_SendWave_B", MPartRef());
		        TweakMantisAI();

                attackWave++;
	        }

            break;

		case 3: 

	        if ( timer >= 8 * 60 )
	        {
		        M_ RunProgramByName("TM13_SendWave_A", MPartRef());
		        TweakMantisAI();

                attackWave++;
	        }

            break;

        case 4: 

	        if ( timer >= 11 * 60 )
	        {
		        M_ RunProgramByName("TM13_SendWave_C", MPartRef());
		        TweakMantisAI();

                attackWave++;
	        }

            break;


		case 5: 

	        if ( timer >= 13 * 60 )
	        {
		        M_ RunProgramByName("TM13_SendWave_B", MPartRef());
		        TweakMantisAI();

                attackWave++;
	        }

            break;

        case 6: 

	        if ( timer >= 15 * 60 )
	        {
		        M_ RunProgramByName("TM13_SendWave_D", MPartRef());
		        TweakMantisAI();

                attackWave++;
	        }

            break;

		case 7: 

	        if ( timer >= 17 * 60 )
	        {
		        M_ RunProgramByName("TM13_SendWave_A", MPartRef());
		        TweakMantisAI();

                attackWave++;
	        }

            break;

		case 8: 

	        if ( timer >= 18 * 60 )
	        {
		        M_ RunProgramByName("TM13_SendWave_A", MPartRef());
		        TweakMantisAI();

                attackWave++;
	        }

            break;

        case 9: 

	        if ( timer >= 19 * 60 )
	        {
		        M_ RunProgramByName("TM13_SendWave_E", MPartRef());
		        M_ RunProgramByName("TM13_SendUpgradedShips", MPartRef());
		        TweakMantisAI();

                attackWave++;
	        }

            break;

		case 10: 

	        if ( timer >= 20 * 60 )
	        {
		        M_ RunProgramByName("TM13_SendWave_B", MPartRef());
		        TweakMantisAI();

                attackWave++;
	        }

            break;

        case 11: 

	        if ( timer >= 21 * 60 )
	        {
		        M_ RunProgramByName("TM13_SendWave_C", MPartRef());
		        TweakMantisAI();

                attackWave++;
	        }

            break;

		case 12: 

	        if ( timer >= 23 * 60 )
	        {
		        M_ RunProgramByName("TM13_SendWave_A", MPartRef());
		        TweakMantisAI();

                attackWave++;
	        }

            break;

		case 13: 

	        if ( timer >= 24 * 60 )
	        {
		        M_ RunProgramByName("TM13_SendWave_A", MPartRef());
		        TweakMantisAI();

                attackWave++;
	        }

            break;

        case 14: 

	        if ( timer >= 25 * 60 )
	        {
		        M_ RunProgramByName("TM13_SendWave_B", MPartRef());
		        TweakMantisAI();

                attackWave++;
	        }

            break;

		case 15: 

	        if ( timer >= 26 * 60 )
	        {
		        M_ RunProgramByName("TM13_SendWave_A", MPartRef());
		        TweakMantisAI();

                attackWave++;
	        }

            break;

		case 16: 

	        if ( timer >= 27 * 60 )
	        {
		        M_ RunProgramByName("TM13_SendWave_A", MPartRef());
		        TweakMantisAI();

                attackWave++;
	        }

            break;

		case 17: 

	        if ( timer >= 28 * 60 )
	        {
		        M_ RunProgramByName("TM13_SendWave_A", MPartRef());
		        TweakMantisAI();

                attackWave++;
	        }

            break;

		case 18: 

	        if ( timer >= 29 * 60 )
	        {
		        M_ RunProgramByName("TM13_SendWave_A", MPartRef());
		        TweakMantisAI();

                attackWave++;
	        }

            break;

        case 19: 

	        if ( timer >= 30 * 60 )
	        {
		        M_ RunProgramByName("TM13_SendWave_C", MPartRef());
		        TweakMantisAI();

                attackWave++;
	        }

            break;

		case 20: 

	        if ( timer >= 31 * 60 )
	        {
		        M_ RunProgramByName("TM13_SendWave_A", MPartRef());
		        TweakMantisAI();

                attackWave++;
	        }

            break;

		
        case 21: 

	        if ( timer >= 32 * 60 )
	        {
		        M_ RunProgramByName("TM13_SendWave_B", MPartRef());
		        TweakMantisAI();

                attackWave++;
	        }

            break;

		case 22: 

	        if ( timer >= 34 * 60 )
	        {
		        M_ RunProgramByName("TM13_SendWave_A", MPartRef());
		        TweakMantisAI();

                attackWave++;
	        }

            break;

		case 23: 

	        if ( timer >= 35 * 60 )
	        {
		        M_ RunProgramByName("TM13_SendWave_A", MPartRef());
		        TweakMantisAI();

                attackWave++;
	        }

            break;

        case 24: 

	        if ( timer >= 36 * 60 )
	        {
		        M_ RunProgramByName("TM13_SendWave_C", MPartRef());
		        TweakMantisAI();

                attackWave++;
	        }

            break;

		case 25: 

	        if ( timer >= 37 * 60 )
	        {
		        M_ RunProgramByName("TM13_SendWave_A", MPartRef());
		        TweakMantisAI();

                attackWave++;
	        }

            break;

		case 26: 

	        if ( timer >= 38 * 60 )
	        {
		        M_ RunProgramByName("TM13_SendWave_A", MPartRef());
		        TweakMantisAI();

                attackWave++;
	        }

            break;

		case 27: 

	        if ( timer >= 40 * 60 )
	        {
		        M_ RunProgramByName("TM13_SendWave_A", MPartRef());
		        TweakMantisAI();

                attackWave++;
	        }

            break;

        case 28: 

	        if ( timer >= 41 * 60 )
	        {
		        M_ RunProgramByName("TM13_SendWave_C", MPartRef());
		        TweakMantisAI();

                attackWave++;
	        }

            break;

		
        case 29: 

	        if ( timer >= 45 * 60 )
	        {
		        M_ RunProgramByName("TM13_SendWave_F", MPartRef());
		        TweakMantisAI();

                return false;
	        }

            break;
    }


	return true;
}

//--------------------------------------------------------------------------//

bool PlatOnPlanet(MPartRef loc)
{
	MPartRef part;

	if (CountObjectsInRange(0, PLAYER_ID, loc, 2))	//yes, could fail if ship real close
		return true;
	else
		return false;
}

MPartRef GetRandomPlanet(bool bOuter)
{
	U32 i;

	i = rand() % 4;

	if (!bOuter)  i += 4;
		
	switch (i)
	{
		case 0:
			return data.Pluto;
		case 1:
			return data.Neptune;
		case 2:
			return data.Saturn;
		case 3: 
			return data.Jupiter;
		case 4:
			return data.Mars;
		case 5:
			return data.Mercury;
		case 6:
			return data.Venus;
		case 7: 
			return data.Earth;
	}

	return data.Pluto;

}

MPartRef FindClosestPlatformHQ(MPartRef loc)
{
	MPartRef part, closest;
	SINGLE leastDist=255;

	part = M_ GetFirstPart();
	while (part.isValid())
	{
		if (M_ IsPlatform(part) && part->playerID == PLAYER_ID)
		{
            if ( part->bReady )
            {
			    if (part->systemID == loc->systemID && M_ DistanceTo(part, loc) < leastDist && part->mObjClass == M_HQ)
			    {
				    closest = part;
				    leastDist = M_ DistanceTo(part, loc);
			    }
            }
		}
		part = M_ GetNextPart(part);
	}

	return closest;
}

MPartRef FindClosestPlatform(MPartRef loc)
{
	MPartRef part, closest;
	SINGLE leastDist=255;

	part = M_ GetFirstPart();
	while (part.isValid())
	{
		if (M_ IsPlatform(part) && part->playerID == PLAYER_ID)
		{
            if ( part->bReady )
            {
			    if (part->systemID == loc->systemID && M_ DistanceTo(part, loc) < leastDist)
			    {
				    closest = part;
				    leastDist = M_ DistanceTo(part, loc);
			    }
            }
		}
		part = M_ GetNextPart(part);
	}

	return closest;
}

U32 FindClosestEntryGate(MPartRef loc)
{
	/*
	U32 x, bestX=0;
	SINGLE leastDist=255;
	for (x=0; x<7; x++)
	{
		if (M_ DistanceTo(data.gateWaveEntry[x], loc) < leastDist)
		{
			bestX = x;
			leastDist = M_ DistanceTo(data.gateWaveEntry[x], loc);
		}
	}

	*/
	return 0; //bestX;
}

/*
MPartRef FindClosestPlanetWithPlayer(MPartRef loc)
{
	MPartRef part, closest;
	SINGLE leastDist=255;

	part = M_ GetFirstPart();
	while (part.isValid())
	{
		if (part->mObjClass == M_PLANET)
		{
			if (M_ GetAvailableSlots(part) > 0 && M_ DistanceTo(part, loc) < leastDist)
			{
				closest = part;
				leastDist = M_ DistanceTo(part, loc);
			}
		}
		part = M_ GetNextPart(part);
	}

	return closest;
}
*/

// returns the number for the jumpgate to use for this target
U32 SetAttackWaveTarget(MPartRef primary)
{
	MPartRef target;

	if (PlatOnPlanet(primary)) 
	{
		M_ SetEnemyAITarget(MANTIS_ID, primary, 2);	
		target = primary;
	}
	else
	{
		if (PlatOnPlanet(data.Pluto)) 
		{
			M_ SetEnemyAITarget(MANTIS_ID, data.Pluto, 2);	
			target = data.Pluto;
		}
		else if (PlatOnPlanet(data.Neptune))
		{
			M_ SetEnemyAITarget(MANTIS_ID, data.Neptune, 2);	
			target = data.Neptune;
		}
		else if (PlatOnPlanet(data.Saturn)) {
			target = data.Saturn;
			M_ SetEnemyAITarget(MANTIS_ID, data.Saturn, 2);	
		}
		else if (PlatOnPlanet(data.Jupiter))
		{
			M_ SetEnemyAITarget(MANTIS_ID, data.Jupiter, 2);	
			target = data.Jupiter;
		}
		else {
			M_ SetEnemyAITarget(MANTIS_ID, data.locInner, 9);	
			target = data.locInner;
		}
	}


	U32 bestX = FindClosestEntryGate(target);

	return bestX;
}

//--------------------------------------------------------------------------//

CQSCRIPTPROGRAM(TM13_SendWave_A, TM13_SendWave_A_Data, 0);

void TM13_SendWave_A::Initialize (U32 eventFlags, const MPartRef & part)
{
	//U32 i;
	//MPartRef planet;
	MPartRef tmpobj, portal;
	
	data.HQattack = true;

	//planet = GetRandomPlanet(true);
	//i = SetAttackWaveTarget(planet);

	for (int x=0; x<4; x++)
	{
		tmpobj = M_ CreatePart("GBOAT!!M_Frigate", data.locWaveEntry, MANTIS_ID);
		AddEnemy(tmpobj, ATTACK_SHIP);
	}
	for (x=0; x<3; x++)
	{
		tmpobj = M_ CreatePart("GBOAT!!M_Scout Carrier", data.locWaveEntry, MANTIS_ID);
		AddEnemy(tmpobj, ATTACK_SHIP);
	}
	for (x=0; x<2; x++)
	{
		tmpobj = M_ CreatePart("GBOAT!!M_Hive Carrier", data.locWaveEntry, MANTIS_ID);
		AddEnemy(tmpobj, ATTACK_SHIP);
	}
	/*
	tmpobj = M_ CreatePart("GBOAT!!M_Scarab", data.locWaveEntry, MANTIS_ID);
	AddEnemy(tmpobj, ATTACK_SHIP);
	tmpobj = M_ CreatePart("GBOAT!!M_Tiamat", data.locWaveEntry, MANTIS_ID);
	AddEnemy(tmpobj, ATTACK_SHIP);*/

	tmpobj = M_ CreatePart("SUPSHIP!!M_Zorap", data.locWaveEntry, MANTIS_ID);
	AddEnemy(tmpobj, TROOP_SHIP);

	portal = M_ CreatePart("PLATGUN!!S_TesPortal", data.locWaveEntry, MANTIS_ID);

	//gateNum = i;
	//gtimer = 0;
	//data.bGateOn[i] = true;
		
	//M_ LaunchOffensive(MANTIS_ID);

	M_ OrderDoWormhole(portal, 1);
	
}

bool TM13_SendWave_A::Update (void)
{
	/*
	if (data.mission_over)
		return false;

	if (CountObjectsInRange(0, MANTIS_ID, data.locWaveEntry[gateNum], 3) > 0)
		return true;

	gtimer++;
	if (gtimer < 24) return true;

	data.bGateOn[gateNum] = false;
	*/

	return false;
}

//--------------------------------------------------------------------------//

CQSCRIPTPROGRAM(TM13_SendWave_B, TM13_SendWave_B_Data, 0);

void TM13_SendWave_B::Initialize (U32 eventFlags, const MPartRef & part)
{
	//U32 i;
	//MPartRef planet;
	MPartRef tmpobj, portal;
	
	data.HQattack = true;

	//planet = GetRandomPlanet(true);
	//i = SetAttackWaveTarget(planet);

	for (int x=0; x<3; x++)
	{
		tmpobj = M_ CreatePart("GBOAT!!M_Frigate", data.locWaveEntry, MANTIS_ID);
		AddEnemy(tmpobj, ATTACK_SHIP);
	}
	for (int y=0; y<2; y++)
	{
		tmpobj = M_ CreatePart("GBOAT!!M_Hive Carrier", data.locWaveEntry, MANTIS_ID);
		AddEnemy(tmpobj, ATTACK_SHIP);
	}

	tmpobj = M_ CreatePart("GBOAT!!M_Scarab", data.locWaveEntry, MANTIS_ID);
	AddEnemy(tmpobj, ATTACK_SHIP);
	tmpobj = M_ CreatePart("GBOAT!!M_Scarab", data.locWaveEntry, MANTIS_ID);
	AddEnemy(tmpobj, ATTACK_SHIP);
	tmpobj = M_ CreatePart("SUPSHIP!!M_Zorap", data.locWaveEntry, MANTIS_ID);
	AddEnemy(tmpobj, TROOP_SHIP);
	
	portal = M_ CreatePart("PLATGUN!!S_TesPortal", data.locWaveEntry, MANTIS_ID);

	//gateNum = i;
	//gtimer = 0;
	//data.bGateOn[i] = true;

	M_ OrderDoWormhole(portal, 1);

	//M_ LaunchOffensive(MANTIS_ID);

}

bool TM13_SendWave_B::Update (void)
{
	/*
	if (data.mission_over)
		return false;

	if (CountObjectsInRange(0, MANTIS_ID, data.locWaveEntry[gateNum], 3) > 0)
		return true;

	gtimer++;
	if (gtimer < 24) return true;

	data.bGateOn[gateNum] = false;
	*/
	return false;
}

//--------------------------------------------------------------------------//

CQSCRIPTPROGRAM(TM13_SendWave_C, TM13_SendWave_C_Data, 0);

void TM13_SendWave_C::Initialize (U32 eventFlags, const MPartRef & part)
{
	//U32 i;
	MPartRef tmpobj, portal;	//planet
	
	data.HQattack = false;

	//planet = GetRandomPlanet(true);
	//i = SetAttackWaveTarget(planet);

	for (int x=0; x<2; x++)
	{
		tmpobj = M_ CreatePart("GBOAT!!M_Frigate", data.locWaveEntry, MANTIS_ID);
		AddEnemy(tmpobj, ATTACK_SHIP);
	}
	for (x=0; x<2; x++)
	{
		tmpobj = M_ CreatePart("GBOAT!!M_Scarab", data.locWaveEntry, MANTIS_ID);
		AddEnemy(tmpobj, ATTACK_SHIP);
	}
	for (x=0; x<2; x++)
	{
		tmpobj = M_ CreatePart("GBOAT!!M_Tiamat", data.locWaveEntry, MANTIS_ID);
		AddEnemy(tmpobj, ATTACK_SHIP);
	}
	tmpobj = M_ CreatePart("GBOAT!!M_Hive Carrier", data.locWaveEntry, MANTIS_ID);
	AddEnemy(tmpobj, ATTACK_SHIP);
	tmpobj = M_ CreatePart("SUPSHIP!!M_Zorap", data.locWaveEntry, MANTIS_ID);
	AddEnemy(tmpobj, TROOP_SHIP);


	portal = M_ CreatePart("PLATGUN!!S_TesPortal", data.locWaveEntry, MANTIS_ID);

	//gateNum = i;
	//gtimer = 0;
	//data.bGateOn[i] = true;

	M_ OrderDoWormhole(portal, 1);

	//M_ LaunchOffensive(MANTIS_ID);

}

bool TM13_SendWave_C::Update (void)
{
	/*
	if (data.mission_over)
		return false;

	if (CountObjectsInRange(0, MANTIS_ID, data.locWaveEntry[gateNum], 3) > 0)
		return true;

	gtimer++;
	if (gtimer < 24) return true;

	data.bGateOn[gateNum] = false;
	*/
	return false;
}

//--------------------------------------------------------------------------//

CQSCRIPTPROGRAM(TM13_SendWave_D, TM13_SendWave_D_Data, 0);

void TM13_SendWave_D::Initialize (U32 eventFlags, const MPartRef & part)
{
	//U32 i;
	MPartRef tmpobj, portal; //planet;
	
	data.HQattack = true;

	//planet = GetRandomPlanet(false);
	//SetAttackWaveTarget(planet);
	//i = rand() % 7;

	for (int x=0; x<2; x++)
	{
		tmpobj = M_ CreatePart("GBOAT!!M_Frigate", data.locWaveEntry, MANTIS_ID);
		AddEnemy(tmpobj, ATTACK_SHIP);
	}
	for (x=0; x<2; x++)
	{
		tmpobj = M_ CreatePart("GBOAT!!M_Hive Carrier", data.locWaveEntry, MANTIS_ID);
		AddEnemy(tmpobj, ATTACK_SHIP);
	}
	for (x=0; x<3; x++)
	{
		tmpobj = M_ CreatePart("GBOAT!!M_Scarab", data.locWaveEntry, MANTIS_ID);
		AddEnemy(tmpobj, ATTACK_SHIP);
	}

	tmpobj = M_ CreatePart("SUPSHIP!!M_Zorap", data.locWaveEntry, MANTIS_ID);
	AddEnemy(tmpobj, TROOP_SHIP);
	tmpobj = M_ CreatePart("SUPSHIP!!M_Zorap", data.locWaveEntry, MANTIS_ID);
	AddEnemy(tmpobj, TROOP_SHIP);

	portal = M_ CreatePart("PLATGUN!!S_TesPortal", data.locWaveEntry, MANTIS_ID);

	//(2 spinelayers?)

	//gateNum = i;
	//gtimer = 0;
	//data.bGateOn[i] = true;

	M_ OrderDoWormhole(portal, 1);

	//M_ LaunchOffensive(MANTIS_ID);

}

bool TM13_SendWave_D::Update (void)
{
	/*
	if (data.mission_over)
		return false;

	if (CountObjectsInRange(0, MANTIS_ID, data.locWaveEntry[gateNum], 3) > 0)
		return true;

	gtimer++;
	if (gtimer < 24) return true;

	data.bGateOn[gateNum] = false;
	*/

	return false;
}

//--------------------------------------------------------------------------//

MPartRef FindClosestOpenPlanet(MPartRef loc)
{
	MPartRef part, closest;
	SINGLE leastDist=255;

	part = M_ GetFirstPart();
	while (part.isValid())
	{
		if (part->mObjClass == M_PLANET)
		{
			if (M_ GetAvailableSlots(part) > 0 && M_ DistanceTo(part, loc) < leastDist)
			{
				closest = part;
				leastDist = M_ DistanceTo(part, loc);
			}
		}
		part = M_ GetNextPart(part);
	}

	return closest;
}

CQSCRIPTPROGRAM(TM13_SendWave_E, TM13_SendWave_E_Data, 0);

void TM13_SendWave_E::Initialize (U32 eventFlags, const MPartRef & part)
{
	numFabsSent = 0;
	timer = 0;
}

bool TM13_SendWave_E::Update (void)
{
	//U32 i;

	if (data.mission_over)
		return false;

	timer+= ELAPSED_TIME;

	data.HQattack = false;

	if (timer >= (TIMERS_PER_MIN / 2))
	{
		MPartRef fab, portal; //, dest;

		//i = rand() % 7;
		//dest = FindClosestOpenPlanet(data.locWaveEntry[i]);

		//bGateOn = true;
		//gateNum = i;
		//gtimer = 0;
		//data.bGateOn[i] = true;

		fab = M_ CreatePart("FAB!!M_NymphWeever", data.locWaveEntry, MANTIS_ID);
		portal = M_ CreatePart("PLATGUN!!S_TesPortal", data.locWaveEntry, MANTIS_ID);

		M_ OrderDoWormhole(portal, 1);

		//M_ OrderMoveTo(fab, dest);

		AddEnemy(fab, FAB_SHIP);

		timer = 0;
		numFabsSent++;
	}

	/*
	if (bGateOn)
	{
		if (CountObjectsInRange(0, MANTIS_ID, data.locWaveEntry[gateNum], 3) > 0)
			return true;

		gtimer++;
		if (gtimer < 24) return true;

		data.bGateOn[gateNum] = false;
		bGateOn = false;

		if (numFabsSent >= 10)
			return false;
	}

	*/
	return true;
}

//--------------------------------------------------------------------------//

/*
MPartRef FindClosestHQ(MPartRef loc)
{
	MPartRef part, closest;
	SINGLE leastDist=255;

	part = M_ GetFirstPart();
	while (part.isValid())
	{
		if (part->mObjClass == M_HQ && part->playerID == PLAYER_ID)
		{
			if (M_ DistanceTo(part, loc) < leastDist)
			{
				closest = part;
				leastDist = M_ DistanceTo(part, loc);
			}
		}
		part = M_ GetNextPart(part);
	}

	return closest;
}
*/

CQSCRIPTPROGRAM(TM13_SendUpgradedShips, TM13_SendUpgradedShips_Data, 0);

void TM13_SendUpgradedShips::Initialize (U32 eventFlags, const MPartRef & part)
{
	timer = 300; 
	timer2 = 180;  
}

bool TM13_SendUpgradedShips::Update (void)
{
	//U32 i;

	if (data.mission_over)
		return false;

	timer+= ELAPSED_TIME;
	timer2+= ELAPSED_TIME;

	if (timer >= 150) //5 mins
	{
		MPartRef tship, portal; //dest;

		tship = M_ CreatePart("TSHIP!!M_Leech", data.locWaveEntry, MANTIS_ID);
		portal = M_ CreatePart("PLATGUN!!S_TesPortal", data.locWaveEntry, MANTIS_ID);
		
		M_ OrderDoWormhole(portal, 1);

		AddEnemy(tship, TROOP_SHIP);
			
		timer = 0;
	}

	if (timer2 >= 150) //3 mins
	{
		//U32 i;
		MPartRef khamir, khamir2, khamir3, portal; //dest;

		khamir = M_ CreatePart("GBOAT!!M_Khamir", data.locWaveEntry, MANTIS_ID);
		khamir2 = M_ CreatePart("GBOAT!!M_Khamir", data.locWaveEntry, MANTIS_ID);
		khamir3 = M_ CreatePart("GBOAT!!M_Khamir", data.locWaveEntry, MANTIS_ID);
		portal = M_ CreatePart("PLATGUN!!S_TesPortal", data.locWaveEntry, MANTIS_ID);
		
		M_ OrderDoWormhole(portal, 1);
		
		AddEnemy(khamir, RAM_SHIP);
		AddEnemy(khamir2, RAM_SHIP);
		AddEnemy(khamir3, RAM_SHIP);

		timer2 = 0;
	}


/*
	if (bGateOn)
	{
		if (CountObjectsInRange(0, MANTIS_ID, data.locWaveEntry[gateNum], 3) > 0)
			return true;

		gtimer++;
		if (gtimer < 24) return true;

		data.bGateOn[gateNum] = false;
		bGateOn = false;
	}

 */

	return true;
}

//--------------------------------------------------------------------------//

CQSCRIPTPROGRAM(TM13_SendWave_F, TM13_SendWave_F_Data, 0);

void TM13_SendWave_F::Initialize (U32 eventFlags, const MPartRef & part)
{
	MPartRef tmpobj, portal;
	int x;
	//U32 i = rand() % 7;
	
	//M_ SetEnemyAITarget(MANTIS_ID, data.locInner, 9);	


	// MALKOR FLEET

	for (x=0; x<3; x++)
	{
		tmpobj = M_ CreatePart("GBOAT!!M_Scarab", data.locWaveEntry, MANTIS_ID);
		AddEnemy(tmpobj, ATTACK_SHIP);
	}
	for (x=0; x<3; x++)
	{
		tmpobj = M_ CreatePart("GBOAT!!M_Hive Carrier", data.locWaveEntry, MANTIS_ID);
		AddEnemy(tmpobj, ATTACK_SHIP);
	}
	for (x=0; x<2; x++)
	{
		tmpobj = M_ CreatePart("GBOAT!!M_Tiamat", data.locWaveEntry, MANTIS_ID);
		AddEnemy(tmpobj, ATTACK_SHIP);
	}
	for (x=0; x<2; x++)
	{
		tmpobj = M_ CreatePart("GBOAT!!M_Khamir", data.locWaveEntry, MANTIS_ID);
		AddEnemy(tmpobj, RAM_SHIP);
	}
	for (x=0; x<2; x++)
	{
		tmpobj = M_ CreatePart("SUPSHIP!!M_Zorap", data.locWaveEntry, MANTIS_ID);
		AddEnemy(tmpobj, TROOP_SHIP);
	}

	//bGateOn = true;
	//gateNum = i;
	//gtimer = 0;
	//data.bGateOn[i] = true;

	portal = M_ CreatePart("PLATGUN!!S_TesPortal", data.locWaveEntry, MANTIS_ID);

	M_ OrderDoWormhole(portal, 1);


	for (x=0; x<6; x++)
	{
		tmpobj = M_ CreatePart("GBOAT!!M_Frigate", data.locWaveEntry2, MANTIS_ID);
		AddEnemy(tmpobj, ATTACK_SHIP);
	}
	for (x=0; x<5; x++)
	{
		tmpobj = M_ CreatePart("GBOAT!!M_Scout Carrier", data.locWaveEntry2, MANTIS_ID);
		AddEnemy(tmpobj, ATTACK_SHIP);
	}

	//TODO  do I really need to create him?

	//data.Malkor = M_ CreatePart("GBOAT!!M_Scarab", data.locWaveEntry, MANTIS_ID, "Malkor");
	//AddEnemy(data.Malkor, ATTACK_SHIP);
	//M_ EnablePartnameDisplay(data.Malkor, true);

	portal = M_ CreatePart("PLATGUN!!S_TesPortal", data.locWaveEntry2, MANTIS_ID);

	M_ OrderDoWormhole(portal, 1);

	//M_ LaunchOffensive(MANTIS_ID);

	
	// SMIRNOFF FLEET
	
	//i = (i+3) % 6;	//get opposite side

	for (x=0; x<3; x++)
	{
		tmpobj = M_ CreatePart("GBOAT!!T_Midas Battleship", data.locWaveEntry3, MANTIS_ID);
		AddEnemy(tmpobj, ATTACK_SHIP);
	}
	for (x=0; x<2; x++)
	{
		tmpobj = M_ CreatePart("GBOAT!!T_Fleet Carrier", data.locWaveEntry3, MANTIS_ID);
		AddEnemy(tmpobj, ATTACK_SHIP);
	}
	for (x=0; x<2; x++)
	{
		tmpobj = M_ CreatePart("GBOAT!!T_Lancer Cruiser", data.locWaveEntry3, MANTIS_ID);
		AddEnemy(tmpobj, ATTACK_SHIP);
	}
	for (x=0; x<4; x++)
	{
		tmpobj = M_ CreatePart("GBOAT!!T_Missile Cruiser", data.locWaveEntry3, MANTIS_ID);
		AddEnemy(tmpobj, ATTACK_SHIP);
	}

	portal = M_ CreatePart("PLATGUN!!S_TesPortal", data.locWaveEntry3, MANTIS_ID);

	M_ OrderDoWormhole(portal, 1);
}

bool TM13_SendWave_F::Update (void)
{

	if (data.mission_over)
		return false;

	/*
	if (bGateOn)
	{
		if (CountObjectsInRange(0, MANTIS_ID, data.locWaveEntry[gateNum], 3) > 0)
			return true;

		gtimer++;
		if (gtimer < 24) return true;

		data.bGateOn[gateNum] = false;
		bGateOn = false;
	}
	if (bGateOn2)
	{
		if (CountObjectsInRange(0, MANTIS_ID, data.locWaveEntry[gateNum2], 3) > 0)
			return true;

		gtimer2++;
		if (gtimer2 < 24) return true;

		data.bGateOn[gateNum2] = false;
		bGateOn2 = false;
	}
	*/


	switch (state)
	{

		case Begin:
	
			state = Smirnoff1;
			break;

		case Smirnoff1:

//			if (!M_ IsVisibleToPlayer(data.Smirnoff, PLAYER_ID)) break;

			if (!IsSomethingPlaying(data.mhandle))
			{
				data.mhandle = DoSpeech(IDS_TM13_SUB_M13SM06, "M13SM06.wav", 8000, 2000, MPartRef(), CHAR_SMIRNOFF);
				state = Hawkes1;
			}
			break;

		case Hawkes1:

			if (!IsSomethingPlaying(data.mhandle))
			{
				data.mhandle = DoSpeech(IDS_TM13_SUB_M13HW07, "M13HW07.wav", 8000, 2000, data.Hawkes, CHAR_HAWKES);
				state = Smirnoff2;
			}
			break;

		case Smirnoff2:

			if (!IsSomethingPlaying(data.mhandle))
			{
				data.mhandle = DoSpeech(IDS_TM13_SUB_M13SM08, "M13SM08.wav", 8000, 2000, MPartRef(), CHAR_SMIRNOFF);
				state = Blackwell1;
			}
			break;

		case Blackwell1:

			if (!IsSomethingPlaying(data.mhandle))
			{
				data.mhandle = DoSpeech(IDS_TM13_SUB_M13BL09A, "M13BL09a.wav", 8000, 2000, data.Blackwell, CHAR_BLACKWELL);
				state = Smirnoff3;
			}
			break;

		case Smirnoff3:

			if (!IsSomethingPlaying(data.mhandle))
			{
				data.mhandle = DoSpeech(IDS_TM13_SUB_M13SM10, "M13SM10.wav", 8000, 2000, MPartRef(), CHAR_SMIRNOFF);
				state = Hawkes2;
			}
			break;

		case Hawkes2:

			if (!IsSomethingPlaying(data.mhandle))
			{
				data.mhandle = DoSpeech(IDS_TM13_SUB_M13HW11, "M13HW11.wav", 8000, 2000, data.Hawkes, CHAR_HAWKES);
				state = Malkor1;
			}
			break;

		case Malkor1:

			//if (!M_ IsVisibleToPlayer(data.Malkor, PLAYER_ID)) break;

			if (!IsSomethingPlaying(data.mhandle))
			{
				data.mhandle = DoSpeech(IDS_TM13_SUB_M13MA12, "M13MA12.wav", 8000, 2000, MPartRef()/*data.Malkor*/, CHAR_MALKOR);
				state = Hawkes3;
			}
			break;

		case Hawkes3:

			if (!IsSomethingPlaying(data.mhandle))
			{
				data.mhandle = DoSpeech(IDS_TM13_SUB_M13HW13, "M13HW13.wav", 8000, 2000, data.Hawkes, CHAR_HAWKES);
				state = Malkor2;
			}
			break;

		case Malkor2:

			if (!IsSomethingPlaying(data.mhandle))
			{
				data.mhandle = DoSpeech(IDS_TM13_SUB_M13MA14, "M13MA14.wav", 8000, 2000, MPartRef()/*data.Malkor*/, CHAR_MALKOR);
				state = Blackwell2;

				//TODO  make Malkor go ahead and exit through wormhole now, if I do need to create him
			}
			break;

		case Blackwell2:

			if (!IsSomethingPlaying(data.mhandle))
			{
				data.mhandle = DoSpeech(IDS_TM13_SUB_M13BL15, "M13BL15.wav", 8000, 2000, data.Blackwell, CHAR_BLACKWELL);
				state = Done;
			}
			break;

		case Done:
			if (!IsSomethingPlaying(data.mhandle))
			{
				MPartRef tmpobj;
				for (U32 x=0; x<4; x++)
				{
					tmpobj = M_ CreatePart("GBOAT!!T_Corvette", data.locWaveEntry4, MANTIS_ID);
					AddEnemy(tmpobj, ATTACK_SHIP);
				}
				for (x=0; x<2; x++)
				{
					tmpobj = M_ CreatePart("GBOAT!!T_Dreadnought", data.locWaveEntry4, MANTIS_ID);
					AddEnemy(tmpobj, ATTACK_SHIP);
				}
				for (x=0; x<2; x++)
				{
					tmpobj = M_ CreatePart("SUPSHIP!!T_Supply", data.locWaveEntry4, MANTIS_ID);
					AddEnemy(tmpobj, ATTACK_SHIP);
				}
				
				data.Smirnoff = M_ CreatePart("CHAR!!Smirnoff_D", data.locWaveEntry4, MANTIS_ID, "Smirnoff");
				AddEnemy(data.Smirnoff, ATTACK_SHIP);
				M_ EnablePartnameDisplay(data.Smirnoff, true);

				MPartRef portal = M_ CreatePart("PLATGUN!!S_TesPortal", data.locWaveEntry4, MANTIS_ID);

				M_ OrderDoWormhole(portal, 1);

				for (x=0; x<3; x++)
				{
					tmpobj = M_ CreatePart("GBOAT!!T_Midas Battleship", data.locWaveEntry3, MANTIS_ID);
					AddEnemy(tmpobj, ATTACK_SHIP);
				}
				for (x=0; x<2; x++)
				{
					tmpobj = M_ CreatePart("GBOAT!!T_Fleet Carrier", data.locWaveEntry3, MANTIS_ID);
					AddEnemy(tmpobj, ATTACK_SHIP);
				}
				for (x=0; x<2; x++)
				{
					tmpobj = M_ CreatePart("GBOAT!!T_Lancer Cruiser", data.locWaveEntry3, MANTIS_ID);
					AddEnemy(tmpobj, ATTACK_SHIP);
				}
				for (x=0; x<4; x++)
				{
					tmpobj = M_ CreatePart("GBOAT!!T_Missile Cruiser", data.locWaveEntry3, MANTIS_ID);
					AddEnemy(tmpobj, ATTACK_SHIP);
				}

				portal = M_ CreatePart("PLATGUN!!S_TesPortal", data.locWaveEntry3, MANTIS_ID);

				M_ OrderDoWormhole(portal, 1);

				data.bFinalAttack = true;

				return false;
			}
	}

	return true;
}

//-----------------------------------------------------------------------
// OBJECT DESTRUCTION TRIGGER

CQSCRIPTPROGRAM(TM13_ObjectDestroyed, TM13_ObjectDestroyed_Data,CQPROGFLAG_OBJECTDESTROYED);

void TM13_ObjectDestroyed::Initialize (U32 eventFlags, const MPartRef & part)
{
	
	if (data.mission_over)
		return;

	// IF MANTIS SHIP DIES AND IS IN ENEMYSHIP ARRAY, REMOVE IT
	if (part->playerID == MANTIS_ID)
	{
		for (U32 x=0; x<data.numEnemy; x++)
		{
			if (SameObj(data.enemyShips[x], part))
			{
				if (x < data.numEnemy-1)
				{
					data.enemyShips[x] = data.enemyShips[data.numEnemy-1];	
					data.enemyShipType[x] = data.enemyShipType[data.numEnemy-1];	
					data.enemyShipTimer[x] = data.enemyShipTimer[data.numEnemy-1];	
				}
				data.numEnemy--;
				x--;
			}
		}
	}


	switch (part->mObjClass)
	{
		case M_HQ:

			if (part->playerID == PLAYER_ID)
			{
				U32 numHQ;

				numHQ = CountObjects(M_HQ, PLAYER_ID);

                if ( part->bReady ) 
                {
                    numHQ--;
                }

				if (numHQ < 3)
				{
					data.mhandle = DoSpeech(IDS_TM13_SUB_M13HA19, "M13HA19.wav", 8000, 2000, MPartRef(), CHAR_HALSEY);
					M_ MarkObjectiveFailed(IDS_TM13_OBJECTIVE2);

					M_ MoveCamera(part, 0, MOVIE_CAMERA_JUMP_TO);

					M_ RunProgramByName("TM13_MissionFailure", MPartRef());
				}
			}
			break;

		case M_DREADNOUGHT:

			if (data.bFinalAttack)
			{
				if (SameObj(part, data.Smirnoff))
				{
					M_ MoveCamera(part, 0, MOVIE_CAMERA_JUMP_TO);

					M_ RunProgramByName("TM13_MissionSuccess", MPartRef ());
				}
			}

			break;

        default: 

			if (SameObj(part, data.Blackwell))
			{
				data.mhandle = DoSpeech(IDS_TM13_SUB_M13HW18, "M13HW18.wav", 8000, 2000, data.Hawkes, CHAR_HAWKES);
				M_ MarkObjectiveFailed(IDS_TM13_OBJECTIVE3);

				M_ MoveCamera(part, 0, MOVIE_CAMERA_JUMP_TO);

				data.textFailureID = IDS_TM13_FAIL_BLACKWELL_LOST;

				M_ RunProgramByName("TM13_MissionFailure", MPartRef());
			}
			else if (SameObj(part, data.Hawkes))
			{
				data.mhandle = DoSpeech(IDS_TM13_SUB_M13HA17, "M13HA17.wav", 8000, 2000, MPartRef(), CHAR_HALSEY);
				M_ MarkObjectiveFailed(IDS_TM13_OBJECTIVE3);

				M_ MoveCamera(part, 0, MOVIE_CAMERA_JUMP_TO);

				data.textFailureID = IDS_TM13_FAIL_HAWKES_LOST;

				M_ RunProgramByName("TM13_MissionFailure", MPartRef());
			}

			break;
	}

}

bool TM13_ObjectDestroyed::Update (void)
{
	return false;
}

//---------------------------------------------------------------------------
// MISSION FAILURE PROGRAMS


//---------------------------------------------------------------------------

#define TELETYPE_TIME 3000

CQSCRIPTPROGRAM(TM13_MissionFailure, TM13_MissionFailure_Data, 0);

void TM13_MissionFailure::Initialize (U32 eventFlags, const MPartRef & part)
{
	UnderMissionControl();	
	M_ EnableMovieMode(true);

	data.mission_over = true;
	state = Begin;

}

bool TM13_MissionFailure::Update (void)
{
	switch (state)
	{
		case Begin:

			state = Teletype;
			break;

		case Teletype:
			
			if (!IsSomethingPlaying(data.mhandle))
			{
				data.mhandle = TeletypeMissionOver(IDS_TM13_MISSION_FAILURE,data.textFailureID);
				//data.mhandle = M_ PlayTeletype(IDS_TM13_MISSION_FAILURE, SuperLeft, SuperTop, SuperRight, SuperBottom, MOTextColor, 6000, 1000, false);
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

CQSCRIPTPROGRAM(TM13_MissionSuccess, TM13_MissionSuccess_Data, 0);

void TM13_MissionSuccess::Initialize (U32 eventFlags, const MPartRef & part)
{
	UnderMissionControl();	
	M_ EnableMovieMode(true);

	data.mission_over = true;
	state = Begin;

	M_ MarkObjectiveCompleted(IDS_TM13_OBJECTIVE1);
	M_ MarkObjectiveCompleted(IDS_TM13_OBJECTIVE2);
	M_ MarkObjectiveCompleted(IDS_TM13_OBJECTIVE3);

}

bool TM13_MissionSuccess::Update (void)
{
	switch (state)
	{
		case Begin:

			state = Halsey;
			break;

		case Halsey:

			data.mhandle = DoSpeech(IDS_TM13_SUB_M13HA16, "M13HA16.wav", 8000, 2000, MPartRef(), CHAR_HALSEY);
			state = Teletype;
			break;

		case Teletype:

			if (!M_ IsTeletypePlaying(data.mhandle) && !M_ IsStreamPlaying(data.mhandle))
			{
				data.mhandle = TeletypeMissionOver(IDS_TM13_MISSION_SUCCESS);
				//data.mhandle = M_ PlayTeletype(IDS_TM13_MISSION_SUCCESS, SuperLeft, SuperTop, SuperRight, SuperBottom, MOTextColor, 6000, 1000, false);
				state = Done;
			}
			break;

		case Done:

			if (!M_ IsTeletypePlaying(data.mhandle))
			{
				UnderPlayerControl();
				M_ EndMissionVictory(12);
				return false;
			}
	}

	return true;
}

//--------------------------------------------------------------------------//
//--------------------------------End Script13T.cpp-------------------------//
//--------------------------------------------------------------------------//