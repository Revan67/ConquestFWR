//--------------------------------------------------------------------------//
//                                                                          //
//                                Script10T.cpp                             //
//				/Conquest/App/Src/Scripts/Script02T/Script10T.cpp			//
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
#define TEXT_HOLD_TIME_INF  0
#define TIME_TO_TELETYPE	10


//--------------------------------------------------------------------------//
//  FUNCTION PROTOTYPES
//--------------------------------------------------------------------------//

static void AddAndDisplayObjective
(
    U32 stringID,
    U32 dependantHandle = 0,
    bool isSecondary = false
);

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//

#define ALL_ADMIRAL (TECHTREE::RES_ADMIRAL1 | TECHTREE::RES_ADMIRAL2 | TECHTREE::RES_ADMIRAL3 | TECHTREE::RES_ADMIRAL4 | TECHTREE::RES_ADMIRAL5 | TECHTREE::RES_ADMIRAL6)

void SetTechLevel(U32 stage)
{
	TECHNODE mission_tech;
	mission_tech.InitLevel(TECHTREE::FULL_TREE);

	if (stage == 1)
	{
		// 0 is the Terran players race...
		mission_tech.race[0].build = mission_tech.race[1].build = mission_tech.race[2].build =
			(TECHTREE::BUILDNODE) 
			(TECHTREE::TDEPEND_HEADQUARTERS | TECHTREE::TDEPEND_REFINERY | TECHTREE::TDEPEND_LIGHT_IND | 
			 TECHTREE::TDEPEND_JUMP_INHIBITOR | TECHTREE::TDEPEND_LASER_TURRET | TECHTREE::TDEPEND_TENDER | 
			 TECHTREE::TDEPEND_SENSORTOWER | TECHTREE::TDEPEND_OUTPOST | TECHTREE::TDEPEND_BALLISTICS |
			 TECHTREE::TDEPEND_HEAVY_IND | TECHTREE::TDEPEND_ADVHULL | TECHTREE::TDEPEND_ACADEMY |
			 TECHTREE::TDEPEND_HANGER | TECHTREE::TDEPEND_SPACE_STATION | TECHTREE::TDEPEND_REPAIR | 
			 TECHTREE::TDEPEND_PROPLAB | TECHTREE::TDEPEND_AWSLAB | TECHTREE::TDEPEND_DISPLACEMENT |
			 TECHTREE::RES_REFINERY_GAS1 | TECHTREE::RES_REFINERY_METAL1 | TECHTREE::RES_REFINERY_METAL2 |
			 TECHTREE::RES_REFINERY_GAS2 );
        
        mission_tech.race[1].build = (TECHTREE::BUILDNODE)( mission_tech.race[1].build |
            TECHTREE::MDEPEND_PLANTATION | TECHTREE::MDEPEND_BIOFORGE );

		mission_tech.race[0].tech = mission_tech.race[1].tech = mission_tech.race[2].tech =
			(TECHTREE::TECHUPGRADE) 
			(TECHTREE::T_SHIP__FABRICATOR | TECHTREE::T_SHIP__CORVETTE | TECHTREE::T_SHIP__HARVEST | 
			 TECHTREE::T_SHIP__MISSILECRUISER | TECHTREE::T_SHIP__TROOPSHIP | TECHTREE::T_RES_TROOPSHIP1 |
			 TECHTREE::T_SHIP__SUPPLY | TECHTREE::T_SHIP__INFILTRATOR | TECHTREE::T_SHIP__BATTLESHIP |
			 TECHTREE::T_SHIP__CARRIER | TECHTREE::T_SHIP__LANCER | TECHTREE::T_RES_MISSLEPACK1 | 
			 TECHTREE::T_RES_MISSLEPACK2 | TECHTREE::T_RES_TROOPSHIP2 | TECHTREE::T_RES_XCHARGES | 
			 TECHTREE::T_RES_XPROBE | TECHTREE::T_RES_XCLOAK );
        
        mission_tech.race[2].tech = (TECHTREE::TECHUPGRADE)( mission_tech.race[2].tech |
            TECHTREE::S_SHIP_AURORA | TECHTREE::S_RES_LEGION2 );

		mission_tech.race[0].common = mission_tech.race[1].common = mission_tech.race[2].common =
			(TECHTREE::COMMON)
			(TECHTREE::RES_SUPPLY1 | TECHTREE::RES_WEAPONS1 | TECHTREE::RES_SUPPLY2 | TECHTREE::RES_WEAPONS2 |
			 TECHTREE::RES_HULL1 | TECHTREE::RES_HULL2 | TECHTREE::RES_HULL3 | TECHTREE::RES_ENGINE1 | 
			 TECHTREE::RES_ENGINE2 | TECHTREE::RES_HULL4 | TECHTREE::RES_ENGINE3 | TECHTREE::RES_SHIELDS1 |
			 TECHTREE::RES_SUPPLY3 | TECHTREE::RES_SUPPLY4); 

		mission_tech.race[0].common_extra = mission_tech.race[1].common_extra = mission_tech.race[2].common_extra =
			(TECHTREE::COMMON_EXTRA)
			(TECHTREE::RES_TENDER1 | TECHTREE::RES_TANKER1 | TECHTREE::RES_TENDER2 | TECHTREE::RES_TANKER2 | 
			 TECHTREE::RES_SENSORS1 | TECHTREE::RES_FIGHTER1 | TECHTREE::RES_FIGHTER2 | TECHTREE::RES_FIGHTER3 |
			 TECHTREE::RES_SENSORS2); 

		// CELAREON

	    mission_tech.race[2].build = 
            (TECHTREE::BUILDNODE) ( ( TECHTREE::ALL_BUILDNODE ^ 
            TECHTREE::SDEPEND_PORTAL ) | 
            TECH_TREE_RACE_BITS_ALL );
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

CQSCRIPTPROGRAM(TM10_Briefing, TM10_Briefing_Data,CQPROGFLAG_STARTBRIEFING);

void TM10_Briefing::Initialize (U32 eventFlags, const MPartRef & part)
{
	data.mission_over = data.briefing_over = data.ai_enabled = data.next_state = data.vivacFound = false;
	data.bNoAcropKill = false;
	data.mhandle = 0;
	data.shandle = 0;
	data.failureID = 0;
	data.textFailureID = 0;

    data.displayObjectiveHandlerParams_teletypeHandle = 0;

	M_ PlayMusic( NULL );

	data.mission_state = Briefing;
	state = Begin;

	M_ EnableBriefingControls(true);

	//M_ ChangeCamera(M_ GetPartByName("BriefCamStart"), 0, MOVIE_CAMERA_JUMP_TO);  

}

bool TM10_Briefing::Update (void)
{
	if (data.briefing_over)
		return false;

	switch(state)
	{
		case Begin:
			
			state = Decoding;

            // fall through
		
		case Decoding:

            MScript::FlushTeletype();

			data.mhandle = TeletypeBriefing( IDS_TM10_DECODING );

            state = PreBrief;

			break;

		case PreBrief:

			M_ PlayAudio("high_level_data_uplink.wav");

			ShowBriefingAnimation(-1, "Xmission", ANIMSPD_XMISSION, false, false); 

			state = Elan;

			break;

		case Elan:
			
			if ( M_ IsCustomBriefingAnimationDone(0) )
			{
				data.mhandle = DoBriefingSpeech(IDS_TM10_SUB_M10EL01, "M10EL01.wav", 8000, 2000, 1, CHAR_ELAN);
				
				ShowBriefingHead(0, CHAR_HALSEY);
				ShowBriefingHead(2, CHAR_SMIRNOFF);
				ShowBriefingHead(3, CHAR_HAWKES);
				
				state = Smirnoff;
			}

			break;

		case Smirnoff:
			
			if (!IsSomethingPlaying(data.mhandle))
			{
				M_ FreeBriefingSlot(1);

				data.mhandle = DoBriefingSpeech(IDS_TM10_SUB_M10SM02, "M10SM02.wav", 8000, 2000, 2, CHAR_SMIRNOFF);
				state = Halsey;
			}
			break;
		
		case Halsey:
			
			if (!IsSomethingPlaying(data.mhandle))
			{
				data.mhandle = DoBriefingSpeechMagpie(IDS_TM10_SUB_M10HA03, "M10HA03.wav", 8000, 2000, 0, CHAR_HALSEY);
				state = Smirnoff2;
			}
			break;

		case Smirnoff2:
			
			if (!IsSomethingPlaying(data.mhandle))
			{
				data.mhandle = DoBriefingSpeech(IDS_TM10_SUB_M10SM04, "M10SM04.wav", 8000, 2000, 2, CHAR_SMIRNOFF);
				state = Hawkes;
			}
			break;

		case Hawkes:
			
			if (!IsSomethingPlaying(data.mhandle))
			{
				data.mhandle = DoBriefingSpeech(IDS_TM10_SUB_M10HW05, "M10HW05.wav", 8000, 2000, 3, CHAR_HAWKES);
				state = Smirnoff3;
			}
			break;

		case Smirnoff3:
			
			if (!IsSomethingPlaying(data.mhandle))
			{
				data.mhandle = DoBriefingSpeech(IDS_TM10_SUB_M10SM06, "M10SM06.wav", 8000, 2000, 2, CHAR_SMIRNOFF);
				state = Hawkes2;
			}
			break;

		case Hawkes2:
			
			if (!IsSomethingPlaying(data.mhandle))
			{
				data.mhandle = DoBriefingSpeech(IDS_TM10_SUB_M10HW07, "M10HW07.wav", 8000, 2000, 1, CHAR_HAWKES);
				state = Hawkes3;
			}
			break;

		case Hawkes3:
			
			if (!IsSomethingPlaying(data.mhandle))
			{
				M_ FreeBriefingSlot(1);

				data.mhandle = DoBriefingSpeech(IDS_TM10_SUB_M10HW08, "M10HW08.wav", 8000, 2000, 3, CHAR_HAWKES);
				state = Smirnoff4;
			}
			break;

		case Smirnoff4:
			
			if (!IsSomethingPlaying(data.mhandle))
			{
				data.mhandle = DoBriefingSpeech(IDS_TM10_SUB_M10SM09, "M10SM09.wav", 8000, 2000, 2, CHAR_SMIRNOFF);
				state = Hawkes4;
			}
			break;

		case Hawkes4:
			
			if (!IsSomethingPlaying(data.mhandle))
			{
				data.mhandle = DoBriefingSpeech(IDS_TM10_SUB_M10HW10, "M10HW10.wav", 8000, 2000, 3, CHAR_HAWKES);
				state = Natus;
			}
			break;

		case Natus:
			
			if (!IsSomethingPlaying(data.mhandle))
			{
				data.mhandle = DoBriefingSpeech(IDS_TM10_SUB_M10NT11, "M10NT11.wav", 8000, 2000, 1, CHAR_NATUS);
				state = Halsey2;
			}
			break;

		case Halsey2:
			
			if (!IsSomethingPlaying(data.mhandle))
			{
				M_ FreeBriefingSlot(1);

				data.mhandle = DoBriefingSpeechMagpie(IDS_TM10_SUB_M10HA12, "M10HA12.wav", 8000, 2000, 0, CHAR_HALSEY);
				state = Hawkes5;
			}
			break;

		case Hawkes5:
			
			if (!IsSomethingPlaying(data.mhandle))
			{
				data.mhandle = DoBriefingSpeech(IDS_TM10_SUB_M10HW13, "M10HW13.wav", 8000, 2000, 3, CHAR_HAWKES);
				state = Halsey3;
			}
			break;

		case Halsey3:
			
			if (!IsSomethingPlaying(data.mhandle))
			{
				data.mhandle = DoBriefingSpeechMagpie(IDS_TM10_SUB_M10HA14, "M10HA14.wav", 8000, 2000, 0, CHAR_HALSEY);
				timer = 160;
				state = Smirnoff5;
					
				//ShowBriefingHead(1, CHAR_BENSON);	
			}
			break;

		case Smirnoff5:
			
			// THIS SHOULD REMOVE HAWKES AT BETTER TIME
			if (timer > 0) timer--; 
			if (timer == 1) M_ FreeBriefingSlot(3);

			if (!IsSomethingPlaying(data.mhandle))
			{
				data.mhandle = DoBriefingSpeech(IDS_TM10_SUB_M10SM15, "M10SM15.wav", 8000, 2000, 2, CHAR_SMIRNOFF);
				state = Halsey4;
			}
			break;

		case Halsey4:
			
			if (!IsSomethingPlaying(data.mhandle))
			{
				data.mhandle = DoBriefingSpeechMagpie(IDS_TM10_SUB_M10HA16, "M10HA16.wav", 8000, 2000, 0, CHAR_HALSEY);
				state = Smirnoff6;
			}
			break;

		case Smirnoff6:
			
			if (!IsSomethingPlaying(data.mhandle))
			{
				data.mhandle = DoBriefingSpeech(IDS_TM10_SUB_M10SM17, "M10SM17.wav", 8000, 2000, 2, CHAR_SMIRNOFF);
				state = Halsey5;
			}
			break;

		case Halsey5:
			
			if (!IsSomethingPlaying(data.mhandle))
			{
				data.mhandle = DoBriefingSpeechMagpie(IDS_TM10_SUB_M10HA18, "M10HA18.wav", 8000, 2000, 0, CHAR_HALSEY);
				state = Benson;
			}
			break;

		case Benson:
			
			if (!IsSomethingPlaying(data.mhandle))
			{
				M_ FreeBriefingSlot(0);
				M_ FreeBriefingSlot(2);

				//data.mhandle = DoBriefingSpeech(IDS_TM10_BENSON_M10BN18a, "M10BN18a.wav", 8000, 2000, 1, CHAR_BENSON);
				
				timer = 16;
				state = Blackwell;
			}
			break;

		case Blackwell:
			
			if (!IsSomethingPlaying(data.mhandle))
			{
				//if (timer >= 16)
				//	M_ FreeBriefingSlot(1);

				if (timer > 0) { timer--; break; }

				data.mhandle = DoBriefingSpeech(IDS_TM10_SUB_M10BL19, "M10BL19.wav", 8000, 2000, 0, CHAR_BLACKWELL);
				state = Objectives;
			}
			break;

		case Objectives:
		
			if (!IsSomethingPlaying(data.mhandle))
			{
				M_ FreeBriefingSlot(0);

				M_ FlushTeletype();
				data.mhandle = TeletypeBriefing(IDS_TM10_OBJECTIVES_START, true);
				//data.mhandle = M_ PlayBriefingTeletype(IDS_TM10_OBJECTIVES_START, MOTextColor, TEXT_HOLD_TIME_INF, 3000, false);
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

//--------------------------------------------------------------------------

CQSCRIPTPROGRAM(TM10_MissionStart, TM10_MissionStart_Data,CQPROGFLAG_STARTMISSION);

void TM10_MissionStart::Initialize (U32 eventFlags, const MPartRef & part)
{

	U32 x;
	data.bSeenMantis = false;
	for (x=0; x<NUM_SYSTEMS; x++)
	{
		data.bAIOnInSystem[x] = false;
		data.bBuiltInSystem[x] = false;
		data.bDestroyedPlatInSystem[x] = false;
	}
	
	data.Steele = M_ GetPartByName("Steele");
	data.Benson = M_ GetPartByName("Benson");
	data.Takei = M_ GetPartByName("Takei");
	data.KerTak = M_ GetPartByName("Ker'Tak");

	M_ EnablePartnameDisplay(data.Steele, true);
	M_ EnablePartnameDisplay(data.Benson, true);
	M_ EnablePartnameDisplay(data.Takei, true);
	M_ EnablePartnameDisplay(data.KerTak, true);

	data.vivPlat1 = M_ GetPartByName("Acropolis (Procyon Prime)");
	data.vivPlat2 = M_ GetPartByName("Citadel (Procyon Prime)");
	data.vivPlat3 = M_ GetPartByName("Pavilion (Procyon Prime)");
	data.vivPlat4 = M_ GetPartByName("Helion Veil (Procyon Prime)");
	data.vivPlat5 = M_ GetPartByName("Oxidator (Procyon Prime)");

	M_ EnablePartnameDisplay(data.vivPlat1, true);
	M_ EnablePartnameDisplay(data.vivPlat2, true);
	M_ EnablePartnameDisplay(data.vivPlat3, true);
	M_ EnablePartnameDisplay(data.vivPlat4, true);
	M_ EnablePartnameDisplay(data.vivPlat5, true);

	data.HQBase = M_ GetPartByName("Headquarters (Halsey)");
	data.AcropBase = M_ GetPartByName("Acropolis (Elan)");

	M_ EnablePartnameDisplay(data.HQBase, true);
	M_ EnablePartnameDisplay(data.AcropBase, true);

	M_ SetEnemyCharacter( MANTIS_ID, IDS_TM10_MANTIS );

	M_ SetAllies(PLAYER_ID, REBEL_ID, true);
	M_ SetAllies(REBEL_ID, PLAYER_ID, true);

	//M_ SetAllies(MANTIS_ID, REBEL_ID, true);
	//M_ SetAllies(REBEL_ID, MANTIS_ID, true);

	M_ EnableSystem(SOLAR, true, true);
	M_ EnableSystem(PROCYON, true, true);

	MPartRef planet = M_ GetPartByName("Procyon Prime");

	M_ ClearHardFog(planet, 1);

	M_ SetMaxGas(PLAYER_ID, 500);
	M_ SetMaxMetal(PLAYER_ID, 500);
	M_ SetMaxCrew(PLAYER_ID, 100);

	M_ SetGas(PLAYER_ID, 20);
	M_ SetMetal(PLAYER_ID, 100);
	M_ SetCrew(PLAYER_ID, 40);

	M_ SetMaxGas(MANTIS_ID, 500);
	M_ SetMaxMetal(MANTIS_ID, 500);
	M_ SetMaxCrew(MANTIS_ID, 100);

	M_ SetGas(MANTIS_ID, 40);
	M_ SetMetal(MANTIS_ID, 10);
	M_ SetCrew(MANTIS_ID, 40);

	SetTechLevel(1);

	M_ EnableEnemyAI(MANTIS_ID, true, "SOLARIAN_FORWARD_BUILD");
	AIPersonality airules;
	airules.difficulty = EASY; 	
	airules.buildMask.bBuildPlatforms = false;
	airules.buildMask.bBuildLightGunboats = true;
	airules.buildMask.bBuildMediumGunboats = true;
	airules.buildMask.bBuildHeavyGunboats = false;
	airules.buildMask.bHarvest = true;
	airules.buildMask.bScout = false;
	airules.buildMask.bBuildAdmirals = false;
	airules.buildMask.bLaunchOffensives = true;  
	airules.buildMask.bVisibilityRules = true;  
	airules.nNumFabricators = 0;
	airules.nBuildPatience = 0;		
	airules.nNumMinelayers = 3;		
	airules.nNumTroopships = 0;		
	M_ SetEnemyAIRules(MANTIS_ID, airules);
	data.ai_enabled = true;
	
	MassEnableAI(MANTIS_ID, false);

	data.mission_state = MissionStart;
	state = Begin;

	M_ SetMissionID(9);
	M_ SetMissionName(IDS_TM10_MISSION_NAME);
	M_ SetMissionDescription(IDS_TM10_MISSION_DESC);

	M_ AddToObjectiveList(IDS_TM10_OBJECTIVE5);
	M_ AddToObjectiveList(IDS_TM10_OBJECTIVE2);

	M_ EnableRegenMode(true);

	M_ EnableJumpgate(M_ GetPartByName("Gate36"), false);
	M_ EnableJumpgate(M_ GetPartByName("Gate38"), false);

	// mike's edits: disable jumpgates to Natus until Vivac is found
	MScript::EnableJumpgate(MScript::GetPartByName("Gate9"), false);
	MScript::EnableJumpgate(MScript::GetPartByName("Gate11"), false);
	// end edits

	MScript::EnableSystem(VELL, false);


	MPartRef apart;

	// MAKE VIVAC'S JUMPGATES INVINCIBLE

	apart = M_ GetFirstPart();
	while (apart.isValid())
	{
		if (apart->playerID == REBEL_ID && apart->mObjClass == M_JUMPPLAT)
			M_ MakeInvincible(apart, true);

		apart = M_ GetNextPart(apart);
	}

	M_ ChangeCamera(M_ GetPartByName("MissionStartCam"), 0, MOVIE_CAMERA_JUMP_TO);

}

bool TM10_MissionStart::Update (void)
{
	MPartRef part, gate;

    if ( data.mission_over )
    {
        return false;
    }

	switch (state)
	{

		case Begin:

			// MOVED THIS CODE HERE BECAUSE IT DOESNT WORK ON FIRST UPDATE
			//data.mantisGate1 = M_ GetPartByName("Enemy Jumpgate to Kasse");
			gate = M_ GetPartByName("Enemy Jumpgate to Solar");
			data.mantisGate1 = M_ GetJumpgateSibling(gate);
			data.mantisGate2 = M_ GetPartByName("Enemy Jumpgate to Pollux");
			M_ SetPartName(data.mantisGate1, "Enemy Jumpgate to Kasse");
			M_ EnablePartnameDisplay(data.mantisGate1, true);
			M_ EnablePartnameDisplay(data.mantisGate2, true);

			data.mhandle = DoSpeech(IDS_TM10_SUB_M10BN18A, "M10BN18a.wav", 8000, 2000, data.Benson, CHAR_BENSON);

			state = LeaveSolar;
			break;

		case LeaveSolar:

			//if (data.next_state) 
			if (M_ IsVisibleToPlayer(data.mantisGate1, PLAYER_ID) || M_ IsVisibleToPlayer(data.mantisGate2, PLAYER_ID))
			{ 
				data.mission_state = FindVivac; 
				state = Bregon1; 
				//UnderMissionControl();
				data.next_state = false; 

				MassStance(0, PLAYER_ID, US_STOP, SOLAR);
			}
			break;

		case Bregon1:

			if (!IsSomethingPlaying(data.mhandle))
			{
				data.mhandle = DoSpeech(IDS_TM10_SUB_M10NT26, "M10NT26.wav", 8000, 2000, MPartRef(), CHAR_NATUS);
				state = Solarian1;
			}
			break;

		case Solarian1:

			if (!IsSomethingPlaying(data.mhandle))
			{
				data.mhandle = DoSpeech(IDS_TM10_SUB_M10EL27, "M10EL27.wav", 8000, 2000, MPartRef(), CHAR_ELAN);
				state = Bregon2;
			}
			break;

		case Bregon2:

			if (!IsSomethingPlaying(data.mhandle))
			{
				data.mhandle = DoSpeech(IDS_TM10_SUB_M10NT28, "M10NT28.wav", 8000, 2000, MPartRef(), CHAR_NATUS);
				state = Solarian2;
			}
			break;

		case Solarian2:

			if (!IsSomethingPlaying(data.mhandle))
			{
				data.mhandle = DoSpeech(IDS_TM10_SUB_M10EL29, "M10EL29.wav", 8000, 2000, MPartRef(), CHAR_ELAN);
				state = Bregon3;
			}
			break;

		case Bregon3:

			if (!IsSomethingPlaying(data.mhandle))
			{
				data.mhandle = DoSpeech(IDS_TM10_SUB_M10NT30, "M10NT30.wav", 8000, 2000, MPartRef(), CHAR_NATUS);
				state = Solarian3;
			}
			break;

		case Solarian3:

			if (!IsSomethingPlaying(data.mhandle))
			{
				data.mhandle = DoSpeech(IDS_TM10_SUB_M10EL31, "M10EL31.wav", 8000, 2000, MPartRef(), CHAR_ELAN);

				M_ AlertSector(PROCYON);

				state = Objectives;
			}
			break;

		case Objectives:

			if (!IsSomethingPlaying(data.mhandle))
			{
				//UnderPlayerControl();

				M_ RemoveFromObjectiveList(IDS_TM10_OBJECTIVE5);

				M_ AddToObjectiveList(IDS_TM10_OBJECTIVE1);
				TeletypeObjective(IDS_TM10_OBJECTIVE1);

				MassStance(0, PLAYER_ID, US_DEFEND, SOLAR);

				state = Done;
			}
			break;

		case Done:

			M_ RunProgramByName("TM10_FindVivac", MPartRef ());
			M_ RunProgramByName("TM10_VivacDeathClock", MPartRef ());

			return false;

	}

	return true;
}

//--------------------------------------------------------------------------//
//	
CQSCRIPTPROGRAM(TM10_VivacDeathClock, TM10_VivacDeathClock_Save,0);

void TM10_VivacDeathClock::Initialize (U32 eventFlags, const MPartRef & part)
{
	timer = 0;
}

bool TM10_VivacDeathClock::Update (void)
{
	if ( data.mission_over || data.vivacFound )
	{
		return false;
	}

	timer += ELAPSED_TIME;

	if(timer > 1200) //20 mins
	{

		data.mhandle = DoSpeech(IDS_TM10_SUB_M10ST58, "M10ST58.wav", 8000, 2000, data.Steele, CHAR_STEELE);
		data.failureID = IDS_TM10_SUB_M10HA59;
		strcpy(data.failureFile, "M10HA59.wav");
		M_ MarkObjectiveFailed(IDS_TM10_OBJECTIVE2);

		M_ MoveCamera(data.vivPlat1, 0, MOVIE_CAMERA_JUMP_TO); 

		M_ RunProgramByName("TM10_MissionFailure", MPartRef ());

		return false;
	}

	return true;
}

//--------------------------------------------------------------------------//

CQSCRIPTPROGRAM(TM10_FindVivac, TM10_FindVivac_Data, 0);

void TM10_FindVivac::Initialize (U32 eventFlags, const MPartRef & part)
{
	data.mission_state = FindVivac;
	state = Begin;
	bVivacExists = bStopped = false;

	M_ EnableJumpgate(M_ GetPartByName("Gate36"), true);
	M_ EnableJumpgate(M_ GetPartByName("Gate38"), true);

	MScript::EnableSystem( VELL, true );

	prime = M_ GetPartByName("Procyon Prime");
	bSawPrime = false;

}

bool TM10_FindVivac::Update (void)
{
	MPartRef locTL, locBL, apart;

    if ( data.mission_over )
    {
        return false;
    }

	if (bVivacExists && !bStopped)
	{
		if (M_ DistanceTo(data.Vivac, data.Terran) <= 2)
		{
			M_ OrderCancel(data.Vivac);
			M_ SetStance(data.Vivac, US_STOP);
			bStopped = true;
		}
	}

	if (!bSawPrime && M_ IsVisibleToPlayer(prime, PLAYER_ID) && PlayerInSystem(PLAYER_ID, PROCYON))
	{
		// set up VivacTrigger near Procyon Prime
		data.VivacTrigger = M_ CreatePart("MISSION!!TRIGGER", prime, 0);
		M_ SetTriggerFilter(data.VivacTrigger, PLAYER_ID, TRIGGER_PLAYER, false);
		M_ SetTriggerProgram(data.VivacTrigger, "TM10_nearprocyonprime");
		M_ SetTriggerRange(data.VivacTrigger, 3);
		M_ EnableTrigger(data.VivacTrigger, true);

		data.next_state = true;
		bSawPrime = true;
	}

	switch (state)
	{

		case Begin:
	
			state = Steele;
			break;

		case Steele:

			if (!IsSomethingPlaying(data.mhandle))
			{
				data.mhandle = DoSpeech(IDS_TM10_SUB_M10ST32, "M10ST32.wav", 8000, 2000, data.Steele, CHAR_STEELE);
				state = Takei;
			}
			break;

		case Takei:

			if (!IsSomethingPlaying(data.mhandle))
			{
				data.mhandle = DoSpeech(IDS_TM10_SUB_M10TK33, "M10TK33.wav", 8000, 2000, data.Takei, CHAR_TAKEI);
				state = KerTak;
			}
			break;

		case KerTak:

			if (!IsSomethingPlaying(data.mhandle))
			{
				data.mhandle = DoSpeech(IDS_TM10_SUB_M10KR34, "M10KR34.wav", 8000, 2000, data.KerTak, CHAR_KERTAK);
				state = Benson;
			}
			break;

		case Benson:

			if (!IsSomethingPlaying(data.mhandle))
			{
				data.mhandle = DoSpeech(IDS_TM10_SUB_M10BN35, "M10BN35.wav", 8000, 2000, data.Benson, CHAR_BENSON);
				state = Takei2;
			}
			break;

		case Takei2:

			if (!IsSomethingPlaying(data.mhandle))
			{
				data.mhandle = DoSpeech(IDS_TM10_SUB_M10TK36, "M10TK36.wav", 8000, 2000, data.Takei, CHAR_TAKEI);
				state = Searching;
			}
			break;

		case Searching:

			if (data.next_state) { state = SawPrime; data.next_state = false; }
			break;

		case SawPrime:

			if (!IsSomethingPlaying(data.mhandle))
			{
				data.mhandle = DoSpeech(IDS_TM10_SUB_M10TK37, "M10TK37.wav", 8000, 2000, data.Takei, CHAR_TAKEI);
				state = SawPrime2;
			}
			break;

		case SawPrime2:

			if (!IsSomethingPlaying(data.mhandle))
			{
				data.mhandle = DoSpeech(IDS_TM10_SUB_M10ST38, "M10ST38.wav", 8000, 2000, data.Steele, CHAR_STEELE);
				state = WheresVivac;
			}
			break;

		case WheresVivac:

			if (data.next_state) { state = EnterVivac; data.next_state = false; }
			break;

		case EnterVivac:

			//UnderMissionControl();
			
			locTL = M_ GetPartByName("locProcyonPrimeTL");
			locBL = M_ GetPartByName("locProcyonPrimeBL");
			if (M_ DistanceTo(locTL, data.Terran) > M_ DistanceTo(locBL, data.Terran))
				data.Vivac = M_ CreatePart("CHAR!!Vivac_SMono", locTL, REBEL_ID, "Vivac");
			else
				data.Vivac = M_ CreatePart("CHAR!!Vivac_SMono", locBL, REBEL_ID, "Vivac");
			M_ EnablePartnameDisplay(data.Vivac, true);
			M_ EnableAIForPart(data.Vivac, false);
			M_ SetVisibleToPlayer(data.Terran, MANTIS_ID);
			M_ OrderMoveTo(data.Vivac, data.Terran);
			M_ SetStance(data.Vivac, US_STOP);
			
			MassStance(0, PLAYER_ID, US_STOP, PROCYON);

			M_ MoveCamera(data.Terran, 0, MOVIE_CAMERA_JUMP_TO);

            data.vivacFound = true;

            // turn off invincibility on jumpgates

	        apart = M_ GetFirstPart();

	        while (apart.isValid())
	        {
		        if (apart->playerID == REBEL_ID && apart->mObjClass == M_JUMPPLAT)
			        M_ MakeInvincible(apart, false);

		        apart = M_ GetNextPart(apart);
	        }

			bVivacExists = true;
			timer = 16;
			state = Vivac1;
			break;

		case Vivac1:

			if (timer > 0) timer--;
			if (timer <= 0) 
			{
				M_ OrderMoveTo(data.Vivac, data.Terran);
				timer = 16;
			}

			if (!M_ IsVisibleToPlayer(data.Vivac, PLAYER_ID)) break;

			M_ MarkObjectiveCompleted(IDS_TM10_OBJECTIVE1);

			data.mhandle = DoSpeech(IDS_TM10_SUB_M10VV39, "M10VV39.wav", 8000, 2000, data.Vivac, CHAR_VIVAC);
			state = Steele1;
			break;

		case Steele1:

			if (!IsSomethingPlaying(data.mhandle))
			{
				data.mhandle = DoSpeech(IDS_TM10_SUB_M10ST40, "M10ST40.wav", 8000, 2000, data.Steele, CHAR_STEELE);
				state = Vivac2;
			}
			break;

		case Vivac2:

			if (!IsSomethingPlaying(data.mhandle))
			{
				data.mhandle = DoSpeech(IDS_TM10_SUB_M10VV41, "M10VV41.wav", 8000, 2000, data.Vivac, CHAR_VIVAC);
				state = Steele2;
			}
			break;

		case Steele2:

			if (!IsSomethingPlaying(data.mhandle))
			{
				data.mhandle = DoSpeech(IDS_TM10_SUB_M10ST42, "M10ST42.wav", 8000, 2000, data.Steele, CHAR_STEELE);
				state = Vivac3;
			}
			break;

		case Vivac3:

			if (!IsSomethingPlaying(data.mhandle))
			{
				data.mhandle = DoSpeech(IDS_TM10_SUB_M10VV43, "M10VV43.wav", 8000, 2000, data.Vivac, CHAR_VIVAC);
				state = KerTak1;
			}
			break;

		case KerTak1:

			if (!IsSomethingPlaying(data.mhandle))
			{
				data.mhandle = DoSpeech(IDS_TM10_SUB_M10KR44, "M10KR44.wav", 8000, 2000, data.KerTak, CHAR_KERTAK);
				state = Vivac4;
			}
			break;

		case Vivac4:

			if (!IsSomethingPlaying(data.mhandle))
			{
				data.mhandle = DoSpeech(IDS_TM10_SUB_M10VV45, "M10VV45.wav", 8000, 2000, data.Vivac, CHAR_VIVAC);
				state = Steele3;
			}
			break;

		case Steele3:

			if (!IsSomethingPlaying(data.mhandle))
			{
				data.mhandle = DoSpeech(IDS_TM10_SUB_M10ST46, "M10ST46.wav", 8000, 2000, data.Steele, CHAR_STEELE);
				state = Vivac5;
			}
			break;

		case Vivac5:

			if (!IsSomethingPlaying(data.mhandle))
			{
				data.bNoAcropKill = true;	//Ok, now they know not to kill Acrops/Oxids

				data.mhandle = DoSpeech(IDS_TM10_SUB_M10VV47, "M10VV47.wav", 8000, 2000, data.Vivac, CHAR_VIVAC);
				state = PreDone;
			}
			break;

		case PreDone:

			if (!IsSomethingPlaying(data.mhandle))
			{
				M_ EnableSystem(NATUS, true, true);
				M_ AlertSector(NATUS);
				timer = 10;
				state = Done;
			}
			break;

		case Done:

			if (timer > 0) timer--;

			if (timer <= 0)
			{
				//UnderPlayerControl();
				
                AddAndDisplayObjective( IDS_TM10_OBJECTIVE3 );
                //AddAndDisplayObjective( IDS_TM10_OBJECTIVE4 );

				M_ SwitchPlayerID(data.Vivac, PLAYER_ID);
				data.Vivac = M_ GetPartByName("Vivac");	
				M_ SwitchPlayerID(data.vivPlat1, PLAYER_ID);
				M_ SwitchPlayerID(data.vivPlat2, PLAYER_ID);
				M_ SwitchPlayerID(data.vivPlat3, PLAYER_ID);
				M_ SwitchPlayerID(data.vivPlat4, PLAYER_ID);
				M_ SwitchPlayerID(data.vivPlat5, PLAYER_ID);

				TurnOnAIForSystem(POLLUX);

				MassEnableAI(MANTIS_ID, true, PROCYON);
				MassStance(0, MANTIS_ID, US_ATTACK, PROCYON);
				MassStance(0, PLAYER_ID, US_DEFEND, PROCYON);

				M_ RunProgramByName("TM10_GoToNatus", MPartRef ());
				return false;
			}
			break;

	}

	return true;
}

//-----------------------------------------------------------------------

CQSCRIPTPROGRAM(TM10_nearprocyonprime, TM10_nearprocyonprime_Data, 0);

void TM10_nearprocyonprime::Initialize (U32 eventFlags, const MPartRef & part)
{
	data.Terran = M_ GetLastTriggerObject(part);
	M_ OrderMoveTo(data.Terran, data.Terran);

	M_ EnableTrigger(data.VivacTrigger, false);		
	data.next_state = true;
}

bool TM10_nearprocyonprime::Update (void)
{
	return false;
}

//-----------------------------------------------------------------------

CQSCRIPTPROGRAM(TM10_GoToNatus, TM10_GoToNatus_Data, 0);

void TM10_GoToNatus::Initialize (U32 eventFlags, const MPartRef & part)
{
	data.mission_state = GoToNatus;
	state = Begin;
	timer = 20;

	// mike's edits: now that Vivac is found enable jumgates into Natus
	MScript::EnableJumpgate(MScript::GetPartByName("Gate9"), true);
	MScript::EnableJumpgate(MScript::GetPartByName("Gate11"), true);
	// end edits	
}

bool TM10_GoToNatus::Update (void)
{
    if ( data.mission_over )
    {
        return false;
    }

	if (!data.bSeenMantis)
	{
		timer--;
		if (timer == 0)
		{
			MPartRef part = M_ GetFirstPart();
			while (part.isValid())
			{
				if (part->playerID == MANTIS_ID && part->mObjClass >= M_COCOON && part->mObjClass <= M_WARLORD &&
					M_ IsVisibleToPlayer(part, PLAYER_ID))
				{
					state = Steele1;
					data.bSeenMantis = true;
				}
				part = M_ GetNextPart(part);
			}
			timer = 20;
		}
	}


	switch (state)
	{

		case Begin:
	
			break;

		case Steele1:

			if (!IsSomethingPlaying(data.mhandle))
			{
				data.mhandle = DoSpeech(IDS_TM10_SUB_M10ST48, "M10ST48.wav", 8000, 2000, data.Steele, CHAR_STEELE);
				state = KerTak1;
			}

		case KerTak1:

			if (!IsSomethingPlaying(data.mhandle))
			{
				data.mhandle = DoSpeech(IDS_TM10_SUB_M10KR49, "M10KR49.wav", 8000, 2000, data.KerTak, CHAR_KERTAK);
				state = Steele2;
			}

		case Steele2:

			if (!IsSomethingPlaying(data.mhandle))
			{
				data.mhandle = DoSpeech(IDS_TM10_SUB_M10ST50, "M10ST50.wav", 8000, 2000, data.Steele, CHAR_STEELE);
				state = Vivac1;
			}

		case Vivac1:

			if (!IsSomethingPlaying(data.mhandle))
			{
				data.mhandle = DoSpeech(IDS_TM10_SUB_M10VV51, "M10VV51.wav", 8000, 2000, data.Vivac, CHAR_VIVAC);
				state = Benson1;
			}

		case Benson1:

			if (!IsSomethingPlaying(data.mhandle))
			{
				data.mhandle = DoSpeech(IDS_TM10_SUB_M10BN52, "M10BN52.wav", 8000, 2000, data.Benson, CHAR_BENSON);
				state = Steele3;
			}

		case Steele3:

			if (!IsSomethingPlaying(data.mhandle))
			{
				data.mhandle = DoSpeech(IDS_TM10_SUB_M10ST53, "M10ST53.wav", 8000, 2000, data.Steele, CHAR_STEELE);
				state = Benson2;
			}

		case Benson2:

			if (!IsSomethingPlaying(data.mhandle))
			{
				data.mhandle = DoSpeech(IDS_TM10_SUB_M10BN52A, "M10BN52a.wav", 8000, 2000, data.Benson, CHAR_BENSON);
				state = Done;
			}

		case Done:

			break;

	}


	return true;
}

//-----------------------------------------------------------------------
// OBJECT CONSTRUCTION TRIGGER

CQSCRIPTPROGRAM(TM10_ObjectConstructed, TM10_ObjectConstructed_Data,CQPROGFLAG_OBJECTCONSTRUCTED);

void TM10_ObjectConstructed::Initialize (U32 eventFlags, const MPartRef & part)
{
	U32 sys = part->systemID;

    if ( data.mission_over )
    {
        return;
    }

	if (M_ IsPlatform(part) && part->playerID == PLAYER_ID && !data.bBuiltInSystem[sys])
	{
		if (sys == SIRIUS && !data.bDestroyedPlatInSystem[KASSE] && !data.bDestroyedPlatInSystem[VELL])
		{
			TurnOnAIForSystem(PROCYON);
			TurnOnAIForSystem(FEMTO);
		}
		if (sys == UNGER && !data.bDestroyedPlatInSystem[POLLUX] && !data.bDestroyedPlatInSystem[VORLAN])
		{
			TurnOnAIForSystem(DALASI);
			TurnOnAIForSystem(ACRE);
		}
		

		data.bBuiltInSystem[sys] = true;
	}

	switch (part->mObjClass)
	{
		case M_ACROPOLIS:	
		case M_HQ:	

			if (part->playerID == PLAYER_ID && part->systemID == NATUS)
			{
				data.mhandle = DoSpeech(IDS_TM10_SUB_M10HA61, "M10HA61.wav", 8000, 2000, MPartRef(), CHAR_HALSEY);
				M_ MarkObjectiveCompleted(IDS_TM10_OBJECTIVE3);

				M_ MoveCamera(part, 0, MOVIE_CAMERA_JUMP_TO);

				M_ RunProgramByName("TM10_MissionSuccess", MPartRef ());
			}
			break;

	}
	
}

bool TM10_ObjectConstructed::Update (void)
{
	return false;
}

//-----------------------------------------------------------------------
// OBJECT DESTRUCTION TRIGGER

CQSCRIPTPROGRAM(TM10_ObjectDestroyed, TM10_ObjectDestroyed_Data,CQPROGFLAG_OBJECTDESTROYED);

void TM10_ObjectDestroyed::Initialize (U32 eventFlags, const MPartRef & part)
{
	U32 sys = part->systemID;

	if (data.mission_over)
		return;

	if (part->playerID == MANTIS_ID && M_ IsPlatform(part) && !data.bDestroyedPlatInSystem[sys])
	{
		if (sys == KASSE || sys == VELL)
		{
			TurnOnAIForSystem(SIRIUS);
			TurnOnAIForSystem(PROCYON);
			TurnOnAIForSystem(FEMTO);
		}
		if (sys == PROCYON)
			TurnOnAIForSystem(POLLUX);
		if (sys == FEMTO)
			TurnOnAIForSystem(VORLAN);
		if (sys == VORLAN)
		{
			TurnOnAIForSystem(POLLUX);
			TurnOnAIForSystem(UNGER);
			TurnOnAIForSystem(DALASI);
		}
		if (sys == POLLUX)
		{
			TurnOnAIForSystem(VORLAN);
			TurnOnAIForSystem(UNGER);
		}
		if (sys == UNGER)
			TurnOnAIForSystem(ACRE);	
		if (sys == ACRE)
			TurnOnAIForSystem(DALASI);
		if (sys == DALASI)
		{
			TurnOnAIForSystem(NATUS);
			TurnOnAIForSystem(BARYON);
			TurnOnAIForSystem(ACRE);
		}
		data.bDestroyedPlatInSystem[sys] = true;
	}
	
	if (SameObj(part, data.HQBase) || SameObj(part, data.AcropBase))
	{
		// need speech line

		data.failureID = 0;

		M_ MoveCamera(part, 0, MOVIE_CAMERA_JUMP_TO);

		M_ RunProgramByName("TM10_MissionFailure", MPartRef ());
	}
	else if (SameObj(part, data.Benson) || SameObj(part, data.Takei))
	{
		data.mhandle = DoSpeech(IDS_TM10_SUB_M10ST54, "M10ST54.wav", 8000, 2000, data.Steele, CHAR_STEELE);
		data.failureID = IDS_TM10_SUB_M10HA57;
		strcpy(data.failureFile, "M10HA57.wav");
		M_ MarkObjectiveFailed(IDS_TM10_OBJECTIVE2);

		M_ MoveCamera(part, 0, MOVIE_CAMERA_JUMP_TO);

		if(SameObj(part, data.Benson))
			data.textFailureID = IDS_TM10_FAIL_BENSON_LOST;
		else
			data.textFailureID = IDS_TM10_FAIL_TAKEI_LOST;

		M_ RunProgramByName("TM10_MissionFailure", MPartRef ());
	}
	else if (SameObj(part, data.Steele))
	{
		data.mhandle = DoSpeech(IDS_TM10_SUB_M10BN55, "M10BN55.wav", 8000, 2000, data.Benson, CHAR_BENSON);
		data.failureID = IDS_TM10_SUB_M10HA57;
		strcpy(data.failureFile, "M10HA57.wav");
		M_ MarkObjectiveFailed(IDS_TM10_OBJECTIVE2);

		M_ MoveCamera(part, 0, MOVIE_CAMERA_JUMP_TO);

		data.textFailureID = IDS_TM10_FAIL_STEELE_LOST;

		M_ RunProgramByName("TM10_MissionFailure", MPartRef ());
	}
	else if (SameObj(part, data.KerTak))
	{
		data.mhandle = DoSpeech(IDS_TM10_SUB_M10ST56, "M10ST56.wav", 8000, 2000, data.Steele, CHAR_STEELE);
		data.failureID = IDS_TM10_SUB_M10HA57;
		strcpy(data.failureFile, "M10HA57.wav");
		M_ MarkObjectiveFailed(IDS_TM10_OBJECTIVE2);

		M_ MoveCamera(part, 0, MOVIE_CAMERA_JUMP_TO);

		data.textFailureID = IDS_TM10_FAIL_KERTAK_LOST;

		M_ RunProgramByName("TM10_MissionFailure", MPartRef ());
	}
	else if (SameObj(part, data.Vivac))
	{
		data.mhandle = DoSpeech(IDS_TM10_SUB_M10ST58, "M10ST58.wav", 8000, 2000, data.Steele, CHAR_STEELE);
		data.failureID = IDS_TM10_SUB_M10HA59;
		strcpy(data.failureFile, "M10HA59.wav");
		M_ MarkObjectiveFailed(IDS_TM10_OBJECTIVE2);

		M_ MoveCamera(part, 0, MOVIE_CAMERA_JUMP_TO);

		data.textFailureID = IDS_TM10_FAIL_VIVAC_LOST;

		M_ RunProgramByName("TM10_MissionFailure", MPartRef ());
	}

}

bool TM10_ObjectDestroyed::Update (void)
{
	return false;
}

//---------------------------------------------------------------------------
// MISSION FAILURE PROGRAMS


//---------------------------------------------------------------------------

#define TELETYPE_TIME 3000

CQSCRIPTPROGRAM(TM10_MissionFailure, TM10_MissionFailure_Data, 0);

void TM10_MissionFailure::Initialize (U32 eventFlags, const MPartRef & part)
{
	UnderMissionControl();	
	M_ EnableMovieMode(true);

	data.mission_over = true;
	state = Begin;

}

bool TM10_MissionFailure::Update (void)
{
	switch (state)
	{
		case Begin:

			if (!IsSomethingPlaying(data.mhandle))
			{
				if (data.failureID)
					data.mhandle = DoSpeech(data.failureID, data.failureFile, 8000, 2000, MPartRef(), CHAR_HALSEY);

				state = Teletype;
			}
			break;

		case Teletype:
			
			if (!IsSomethingPlaying(data.mhandle))
			{
				data.mhandle = TeletypeMissionOver(IDS_TM10_MISSION_FAILURE,data.textFailureID);

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

CQSCRIPTPROGRAM(TM10_MissionSuccess, TM10_MissionSuccess_Data, 0);

void TM10_MissionSuccess::Initialize (U32 eventFlags, const MPartRef & part)
{
	UnderMissionControl();	
	M_ EnableMovieMode(true);

	data.mission_over = true;
	state = Begin;

	M_ MarkObjectiveCompleted(IDS_TM10_OBJECTIVE2);
	//M_ MarkObjectiveCompleted(IDS_TM10_OBJECTIVE4);
}

bool TM10_MissionSuccess::Update (void)
{
	switch (state)
	{
		case Begin:

			state = Teletype;
			break;

		case Teletype:

			if (!IsSomethingPlaying(data.mhandle))
			{
				data.mhandle = TeletypeMissionOver(IDS_TM10_MISSION_SUCCESS);
				//data.mhandle = M_ PlayTeletype(IDS_TM10_MISSION_SUCCESS, SuperLeft, SuperTop, SuperRight, SuperBottom, MOTextColor, 6000, 1000, false);
				state = Done;
			}
			break;

		case Done:

			if (!M_ IsTeletypePlaying(data.mhandle))
			{
				UnderPlayerControl();
				M_ EndMissionVictory(9);
				return false;
			}
	}

	return true;
}

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

    MScript::RunProgramByName( "TM10_DisplayObjectiveHandler", MPartRef ());
}

CQSCRIPTPROGRAM( TM10_DisplayObjectiveHandler, TM10_DisplayObjectiveHandlerData, 0 );

void TM10_DisplayObjectiveHandler::Initialize
(
	U32 eventFlags, 
	const MPartRef &part 
)
{
    stringID = data.displayObjectiveHandlerParams_stringID;
    dependantHandle = data.displayObjectiveHandlerParams_dependantHandle;
}

bool TM10_DisplayObjectiveHandler::Update
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
//--------------------------------End Script10T.cpp-------------------------//
//--------------------------------------------------------------------------//
