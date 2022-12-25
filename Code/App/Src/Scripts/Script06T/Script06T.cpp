//--------------------------------------------------------------------------//
//                                                                          //
//                                Script06T.cpp                             //
//                                                                          //
//               COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
   $Header: /Conquest/App/Src/Scripts/Script06T/Script06T.cpp 105   9.10.01 11:42 Tmauer $
*/
//--------------------------------------------------------------------------//

#include <ScriptDef.h>
#include <DLLScriptMain.h>
#include "DataDef.h"
#include "stdlib.h"

#include "..\helper\helper.h"



//--------------------------------------------------------------------------
//  TM6 #define's

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


//--------------------------------------------------------------------------//
//-----------------------------Seperate Functions---------------------------//
//--------------------------------------------------------------------------//


void SetupAI(bool bOn)
{

	M_ EnableEnemyAI(MANTIS_ID, bOn, "MANTIS_FORTRESS");

	AIPersonality airules;
	airules.difficulty = MEDIUM;
	airules.buildMask.bBuildPlatforms = false;
	airules.buildMask.bBuildLightGunboats = true;
	airules.buildMask.bBuildMediumGunboats = true;
	airules.buildMask.bBuildHeavyGunboats = false;
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
//  Mission functions: Terran Campaign Mission 6
//--------------------------------------------------------------------------

CQSCRIPTDATA(MissionData, data);

//--------------------------------------------------------------------------//
//	Mission Briefing
//--------------------------------------------------------------------------//

CQSCRIPTPROGRAM(TM6_Briefing, TM6_Briefing_Save,CQPROGFLAG_STARTBRIEFING);

void TM6_Briefing::Initialize (U32 eventFlags, const MPartRef & part)
{
	data.failureID = 0;
	data.mission_over = data.briefing_over = false;
	data.mhandle = 0;
	data.shandle = 0;
	data.mission_state = Briefing;

	M_ PlayMusic(NULL);
	M_ EnableBriefingControls(true);

	state = Begin;
	
}

bool TM6_Briefing::Update (void)
{
	if (data.briefing_over)
		return false;

	switch(state)
	{
		case Begin:

//		    M_ ChangeCamera(M_ GetPartByName("Brief Cam"), 0, MOVIE_CAMERA_JUMP_TO);

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

			data.mhandle = M_ PlayAudio("prolog06.wav");
			MScript::BriefingSubtitle(data.mhandle,IDS_TM6_SUB_PROLOG06);

			state = TeleTypeLocation;

			break;

		case TeleTypeLocation:

			M_ FlushTeletype();
			data.shandle = TeletypeBriefing(IDS_TM6_TELETYPE_LOCATION, false);
			state = Uplink;	
			
			break;

		case Uplink:

			if (!M_ IsStreamPlaying(data.mhandle))
			{
				TeletypeBriefing(IDS_TM6_FORYOUREYES, false);

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

				state = Radio;
			}
			break;

		case Radio:
			if(M_ IsCustomBriefingAnimationDone(0))
			{

				strcpy(slotAnim.szFileName, "m06hw01.wav");
				strcpy(slotAnim.szTypeName, "Animate!!Hawkes");
				slotAnim.slotID = 2;
				slotAnim.bContinueAnimating = true;
				slotAnim.bHighlite = false;
				data.mhandle = M_ PlayBriefingTalkingHead(slotAnim);
				MScript::BriefingSubtitle(data.mhandle,IDS_TM6_SUB_M06HW01);

				ShowBriefingHead(1, CHAR_KERTAK);
				ShowBriefingHead(0, CHAR_HALSEY);

			
//				data.mhandle = M_ PlayAudio("M06HW01.wav");

				state = Radio1;
			}
			break;

		case Radio1:
			if (!M_ IsStreamPlaying(data.mhandle))
			{
				strcpy(slotAnim.szFileName, "m06ha02.wav");
				strcpy(slotAnim.szTypeName, "Animate!!Halsey");
				slotAnim.slotID = 0;
				slotAnim.bHighlite = false;
				data.mhandle = M_ PlayBriefingTalkingHead(slotAnim);
				MScript::BriefingSubtitle(data.mhandle,IDS_TM6_SUB_M06HA02);

//				data.mhandle = M_ PlayAudio("M06HA02.wav");
				state = Radio2;
			}
			break;

		case Radio2:
			if (!M_ IsStreamPlaying(data.mhandle))
			{
				strcpy(slotAnim.szFileName, "m06kr03.wav");
				strcpy(slotAnim.szTypeName, "Animate!!KerTak");
				slotAnim.slotID = 1;
				slotAnim.bHighlite = false;
				data.mhandle = M_ PlayBriefingTalkingHead(slotAnim);
				MScript::BriefingSubtitle(data.mhandle,IDS_TM6_SUB_M06KR03);

//				data.mhandle = M_ PlayAudio("M06KR03.wav");
				state = Radio3;
			}
			break;

		case Radio3:
			if (!M_ IsStreamPlaying(data.mhandle))
			{
				strcpy(slotAnim.szFileName, "m06ha04.wav");
				strcpy(slotAnim.szTypeName, "Animate!!Halsey");
				slotAnim.slotID = 0;
				slotAnim.bHighlite = false;
				data.mhandle = M_ PlayBriefingTalkingHead(slotAnim);
				MScript::BriefingSubtitle(data.mhandle,IDS_TM6_SUB_M06HA04);

//				data.mhandle = M_ PlayAudio("M06HA04.wav");
				state = Radio4;
			}
			break;

		case Radio4:
			if (!M_ IsStreamPlaying(data.mhandle))
			{
				strcpy(slotAnim.szFileName, "m06kr05.wav");
				strcpy(slotAnim.szTypeName, "Animate!!KerTak");
				slotAnim.slotID = 1;
				slotAnim.bHighlite = false;
				data.mhandle = M_ PlayBriefingTalkingHead(slotAnim);
				MScript::BriefingSubtitle(data.mhandle,IDS_TM6_SUB_M06KR05);

//				data.mhandle = M_ PlayAudio("M06KR05.wav");
				state = Radio5;
			}
			break;

		case Radio5:
			if (!M_ IsStreamPlaying(data.mhandle))
			{
				strcpy(slotAnim.szFileName, "m06ha06.wav");
				strcpy(slotAnim.szTypeName, "Animate!!Halsey");
				slotAnim.slotID = 0;
				slotAnim.bHighlite = false;
				data.mhandle = M_ PlayBriefingTalkingHead(slotAnim);
				MScript::BriefingSubtitle(data.mhandle,IDS_TM6_SUB_M06HA06);

//				data.mhandle = M_ PlayAudio("M06HW06.wav");
				state = Radio6;
			}
			break;

		case Radio6:
			if (!M_ IsStreamPlaying(data.mhandle))
			{
				strcpy(slotAnim.szFileName, "m06kr07.wav");
				strcpy(slotAnim.szTypeName, "Animate!!KerTak");
				slotAnim.slotID = 1;
				slotAnim.bHighlite = false;
				data.mhandle = M_ PlayBriefingTalkingHead(slotAnim);
				MScript::BriefingSubtitle(data.mhandle,IDS_TM6_SUB_M06KR07);

//				data.mhandle = M_ PlayAudio("M06KR07.wav");
				state = Radio7;
			}
			break;

		case Radio7:
			if (!M_ IsStreamPlaying(data.mhandle))
			{
				strcpy(slotAnim.szFileName, "m06ha08.wav");
				strcpy(slotAnim.szTypeName, "Animate!!Halsey");
				slotAnim.slotID = 0;
				slotAnim.bHighlite = false;
				data.mhandle = M_ PlayBriefingTalkingHead(slotAnim);
				MScript::BriefingSubtitle(data.mhandle,IDS_TM6_SUB_M06HA08);

//				data.mhandle = M_ PlayAudio("M06HA08.wav");
				state = Radio8;
			}
			break;

		case Radio8:
			if (!M_ IsStreamPlaying(data.mhandle))
			{
				strcpy(slotAnim.szFileName, "m06kr09.wav");
				strcpy(slotAnim.szTypeName, "Animate!!KerTak");
				slotAnim.slotID = 1;
				slotAnim.bHighlite = false;
				data.mhandle = M_ PlayBriefingTalkingHead(slotAnim);
				MScript::BriefingSubtitle(data.mhandle,IDS_TM6_SUB_M06KR09);

//				data.mhandle = M_ PlayAudio("M06KR09.wav");
				state = Radio9;
			}
			break;

		case Radio9:
			if (!M_ IsStreamPlaying(data.mhandle))
			{
				strcpy(slotAnim.szFileName, "m06ha10.wav");
				strcpy(slotAnim.szTypeName, "Animate!!Halsey");
				slotAnim.slotID = 0;
				slotAnim.bHighlite = false;
				data.mhandle = M_ PlayBriefingTalkingHead(slotAnim);
				MScript::BriefingSubtitle(data.mhandle,IDS_TM6_SUB_M06HA10);

//				data.mhandle = M_ PlayAudio("M06HA10.wav");
				state = Radio10;
			}
			break;

		case Radio10:
			if (!M_ IsStreamPlaying(data.mhandle))
			{
				strcpy(slotAnim.szFileName, "m06kr11.wav");
				strcpy(slotAnim.szTypeName, "Animate!!KerTak");
				slotAnim.slotID = 1;
				slotAnim.bHighlite = false;
				data.mhandle = M_ PlayBriefingTalkingHead(slotAnim);
				MScript::BriefingSubtitle(data.mhandle,IDS_TM6_SUB_M06KR11);

//				data.mhandle = M_ PlayAudio("M06KR11.wav");
				state = Radio11;
			}
			break;

		case Radio11:
			if (!M_ IsStreamPlaying(data.mhandle))
			{

				strcpy(slotAnim.szFileName, "m06ha12.wav");
				strcpy(slotAnim.szTypeName, "Animate!!Halsey");
				slotAnim.slotID = 0;
				slotAnim.bHighlite = false;
				data.mhandle = M_ PlayBriefingTalkingHead(slotAnim);
				MScript::BriefingSubtitle(data.mhandle,IDS_TM6_SUB_M06HA12);


//				data.mhandle = M_ PlayAudio("M06HA12.wav");
				state = Radio12;
			}
			break;

		case Radio12:
			if (!M_ IsStreamPlaying(data.mhandle))
			{
				ShowBriefingHead(3, CHAR_STEELE);

				strcpy(slotAnim.szFileName, "m06ha12a.wav");
				strcpy(slotAnim.szTypeName, "Animate!!Halsey");
				slotAnim.slotID = 0;
				slotAnim.bHighlite = false;
				data.mhandle = M_ PlayBriefingTalkingHead(slotAnim);
				MScript::BriefingSubtitle(data.mhandle,IDS_TM6_SUB_M06HA12A);


//				data.mhandle = M_ PlayAudio("M06HA12a.wav");
				state = MO;
			}
			break;

		case MO:
			
			if (!M_ IsStreamPlaying(data.mhandle))
			{
				M_ FreeBriefingSlot(-1);

				M_ FlushTeletype();
				data.mhandle = TeletypeBriefing(IDS_TM6_OBJECTIVES, true);
				state = Finish;
			}

			break;
			
		case Finish:
			if (!M_ IsTeletypePlaying(data.mhandle))
				{
					M_ RunProgramByName("TM6_BriefingSkip",MPartRef ());
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

CQSCRIPTPROGRAM(TM6_MissionStart, TM6_MissionStart_Save,CQPROGFLAG_STARTMISSION);

void TM6_MissionStart::Initialize (U32 eventFlags, const MPartRef & part)
{
	data.mission_state = MissionStart;

	M_ EnableMovieMode(true);

	SetupAI(true);
	MassEnableAI(MANTIS_ID, false, 0);
	M_ EnableAIForPart(M_ GetPartByName("Seeker1"), true);
	M_ EnableAIForPart(M_ GetPartByName("Seeker2"), true);
	M_ EnableAIForPart(M_ GetPartByName("Seeker3"), true);

	M_ SetMissionName(IDS_TM6_MISSION_NAME);
	M_ SetMissionID(5);

	M_ SetEnemyCharacter(MANTIS_ID, IDS_TM6_MANTIS);

	M_ EnableRegenMode(true);

	M_ SetMissionDescription(IDS_TM6_MISSION_DESC);
	M_ AddToObjectiveList(IDS_TM6_OBJECTIVE1);
	M_ AddToObjectiveList(IDS_TM6_OBJECTIVE3);
	M_ AddToObjectiveList(IDS_TM6_OBJECTIVE4);

	M_ EnableJumpgate(M_ GetPartByName("Gate1"),false);
	M_ EnableJumpgate(M_ GetPartByName("Gate2"),false);
	M_ EnableJumpgate(M_ GetPartByName("Gate3"),false);
	M_ EnableJumpgate(M_ GetPartByName("Gate16"),false);

	M_ EnableJumpgate(M_ GetPartByName("Gate4"),false);
	M_ EnableJumpgate(M_ GetPartByName("Gate6"),false);
	M_ EnableJumpgate(M_ GetPartByName("Gate8"),false);

	M_ SetAllies(PLAYER_ID, REBEL_ID, true);
	M_ SetAllies(REBEL_ID, PLAYER_ID, true);

	data.XinFreed = false;
	data.RendarFreed = false;
	data.CorlarFreed = false;
	data.AllFreed = false;

	data.Troopships = 8;
	data.RebelsToGo = 3;

    // add in necessary upgrades, and troopship
	TECHNODE missionTech = M_ GetPlayerTech(PLAYER_ID);
	TECHNODE missionTech2 = M_ GetPlayerTech(MANTIS_ID);

	missionTech.race[0].tech = missionTech.race[1].tech = (TECHTREE::TECHUPGRADE) (
		TECHTREE::T_RES_MISSLEPACK1 |
		TECHTREE::T_RES_MISSLEPACK2 |
		TECHTREE::T_RES_TROOPSHIP1 | 
		TECHTREE::T_RES_TROOPSHIP2 
		);

	missionTech2.race[1].tech = (TECHTREE::TECHUPGRADE) (
		TECHTREE::M_RES_XCAMO
		);

	missionTech.race[0].common_extra = missionTech.race[1].common_extra = (TECHTREE::COMMON_EXTRA) (
		TECHTREE::RES_TANKER1  |
		TECHTREE::RES_TANKER2  |
		TECHTREE::RES_FIGHTER1 |
		TECHTREE::RES_FIGHTER2 |
		TECHTREE::RES_TENDER1 |
		TECHTREE::RES_TENDER2 |
		TECHTREE::RES_SENSORS1 
		);

	missionTech.race[0].common = missionTech.race[1].common = (TECHTREE::COMMON) (
		TECHTREE::RES_WEAPONS1  |
		TECHTREE::RES_WEAPONS2  |
		TECHTREE::RES_SUPPLY1 |
		TECHTREE::RES_SUPPLY2  |
		TECHTREE::RES_HULL1 |
		TECHTREE::RES_HULL2 |
		TECHTREE::RES_ENGINE1 
        );

    MScript::SetPlayerTech(  PLAYER_ID, missionTech );
    MScript::SetPlayerTech(  MANTIS_ID, missionTech2 );

	MassStance(0, MANTIS_ID, US_ATTACK);

	M_ SetMaxGas(MANTIS_ID, 440);
	M_ SetMaxMetal(MANTIS_ID, 440);
	M_ SetMaxCrew(MANTIS_ID, 400);

	data.KerTak	   = M_ GetPartByName("Ker'Tak");
	M_ EnablePartnameDisplay(data.KerTak, true);
	data.Hawkes    = M_ GetPartByName("Hawkes");
	M_ EnablePartnameDisplay(data.Hawkes, true);
	data.Steele    = M_ GetPartByName("Steele");
	M_ EnablePartnameDisplay(data.Steele, true);

	data.Group1WP  = M_ GetPartByName("Battle Group 1");
	data.Group2WP  = M_ GetPartByName("Battle Group 2");
	data.KerTakWP  = M_ GetPartByName("KerTak Waypoint");

	data.XinCamp = M_ GetPartByName("Xin Prison Camp");
	M_ EnablePartnameDisplay(data.XinCamp, true);
	M_ SetMaxHullPoints(data.XinCamp, 6000);
	M_ SetHullPoints(data.XinCamp, 6000);
	M_ MakeNonAutoTarget(data.XinCamp, true);

	data.RendarCamp = M_ GetPartByName("Rendar Prison Camp");
	M_ EnablePartnameDisplay(data.RendarCamp, true);
	M_ SetMaxHullPoints(data.RendarCamp, 6000);
	M_ SetHullPoints(data.RendarCamp, 6000);
	M_ MakeNonAutoTarget(data.RendarCamp, true);

	data.CorlarCamp = M_ GetPartByName("Corlar Prison Camp");
	M_ EnablePartnameDisplay(data.CorlarCamp, true);
	M_ SetMaxHullPoints(data.CorlarCamp, 6000);
	M_ SetHullPoints(data.CorlarCamp, 6000);
	M_ MakeNonAutoTarget(data.CorlarCamp, true);

	data.MalkorBase = M_ GetPartByName("Malkor's Sentry Base");
	M_ EnablePartnameDisplay(data.MalkorBase, true);
	M_ SetMaxHullPoints(data.MalkorBase, 8000);
	M_ SetHullPoints(data.MalkorBase, 8000);

	data.BriefJump = M_ GetPartByName("Gate0");

	M_ SetGas(PLAYER_ID, 0);
	M_ SetMetal(PLAYER_ID, 0);
	M_ SetCrew(PLAYER_ID, 0);

	//fix up supply loss due to supply tech upgrades.
	MPartRef tmp = M_ GetFirstPart();
	while(tmp.isValid())
	{
		if(tmp->playerID == PLAYER_ID)
		{
			M_ SetSupplies(tmp,tmp->supplyPointsMax);
		}
		tmp = M_ GetNextPart(tmp);
	}


	tmp = M_ GetFirstPart();
	MGroupRef group, group2;

	while(tmp.isValid())
	{
		if(tmp->playerID == PLAYER_ID && (tmp->mObjClass == M_CORVETTE || tmp->mObjClass == M_INFILTRATOR || tmp->mObjClass == M_TROOPSHIP || tmp == data.Steele))
		{
			group += tmp;
		}
		else if(tmp->playerID == PLAYER_ID && (tmp->mObjClass != M_NONE))
		{
			group2 += tmp;
		}

		tmp = M_ GetNextPart(tmp);
	}

	M_ OrderUseJumpgate(group, data.BriefJump, true);
	M_ OrderUseJumpgate(group2, data.BriefJump, true);

/*	MassTeleport(M_MISSILECRUISER, PLAYER_ID, data.BriefJump, LUXOR);
	MassTeleport(M_BATTLESHIP, PLAYER_ID, data.BriefJump, LUXOR);`
	MassTeleport(M_CORVETTE, PLAYER_ID, data.BriefJump, LUXOR);
	MassTeleport(M_CARRIER, PLAYER_ID, data.BriefJump, LUXOR);
	MassTeleport(M_DREADNOUGHT, PLAYER_ID, data.BriefJump, LUXOR);
	MassTeleport(M_SCARAB, PLAYER_ID, data.BriefJump, LUXOR);
	MassTeleport(M_HIVECARRIER, PLAYER_ID, data.BriefJump, LUXOR);
	MassTeleport(M_INFILTRATOR, PLAYER_ID, data.BriefJump, LUXOR);
	MassTeleport(M_SUPPLY, PLAYER_ID, data.BriefJump, LUXOR);
	MassTeleport(M_TROOPSHIP, PLAYER_ID, data.BriefJump, LUXOR);

	MassMove(M_CARRIER, PLAYER_ID, data.Group2WP, LUXOR);
	MassMove(M_BATTLESHIP, PLAYER_ID, data.Group1WP, LUXOR);
	MassMove(M_MISSILECRUISER, PLAYER_ID, data.Group1WP, LUXOR);
	MassMove(M_CORVETTE, PLAYER_ID, data.Group2WP, LUXOR);
	M_ OrderMoveTo(data.KerTak, data.KerTakWP);
	M_ OrderMoveTo(data.Steele, data.Group2WP);
	MassMove(M_SCARAB, PLAYER_ID, data.KerTakWP, LUXOR);



	//Split units up that are in both Battle Groups
	//Spy group
	MPartRef Spy = FindFirstObjectOfType(M_INFILTRATOR, PLAYER_ID, LUXOR);
	M_ OrderMoveTo(Spy, data.Group1WP);
	Spy = FindNextObjectOfType(M_INFILTRATOR, PLAYER_ID, LUXOR, Spy);
	M_ OrderMoveTo(Spy, data.Group2WP);

	//Supply group
	MPartRef Supply = FindFirstObjectOfType(M_SUPPLY, PLAYER_ID, LUXOR);
	M_ OrderMoveTo(Supply, data.Group1WP);
	Supply = FindNextObjectOfType(M_SUPPLY, PLAYER_ID, LUXOR, Supply);
	M_ OrderMoveTo(Supply, data.Group2WP);

	//Troopship group
	MPartRef Troop = FindFirstObjectOfType(M_TROOPSHIP, PLAYER_ID, LUXOR);
	M_ OrderMoveTo(Troop, data.Group1WP);
	Troop = FindNextObjectOfType(M_TROOPSHIP, PLAYER_ID, LUXOR, Troop);
	M_ OrderMoveTo(Troop, data.Group1WP);
	Troop = FindNextObjectOfType(M_TROOPSHIP, PLAYER_ID, LUXOR, Troop);
	M_ OrderMoveTo(Troop, data.Group1WP);
	Troop = FindNextObjectOfType(M_TROOPSHIP, PLAYER_ID, LUXOR, Troop);
	M_ OrderMoveTo(Troop, data.Group1WP);
	Troop = FindNextObjectOfType(M_TROOPSHIP, PLAYER_ID, LUXOR, Troop);
	M_ OrderMoveTo(Troop, data.Group2WP);
	Troop = FindNextObjectOfType(M_TROOPSHIP, PLAYER_ID, LUXOR, Troop);
	M_ OrderMoveTo(Troop, data.Group2WP);
	Troop = FindNextObjectOfType(M_TROOPSHIP, PLAYER_ID, LUXOR, Troop);
	M_ OrderMoveTo(Troop, data.Group2WP);
	Troop = FindNextObjectOfType(M_TROOPSHIP, PLAYER_ID, LUXOR, Troop);
	M_ OrderMoveTo(Troop, data.Group2WP);
*/

	//Enable systems for the blackhole
	M_ EnableSystem(LUXOR, false, false);
	M_ EnableSystem(WOR, false, true);
	M_ EnableSystem(CORLAR, false, true);
	M_ EnableSystem(XIN, false, true);
	M_ EnableSystem(RENDAR, false, true);
	M_ EnableSystem(CENTAURUS, false, false);
	M_ EnableSystem(LEMO, false, false);
	M_ EnableSystem(KRACUS, false, false);
	M_ EnableSystem(CAPELLA, false, true);

	//Set a visible area for the MissionStartCam
	M_ ClearHardFog (M_ GetPartByName("Gate1"), 4);

	//Set Camera to Mission Start pos
	M_ MoveCamera(M_ GetPartByName("Gate1"), 0, MOVIE_CAMERA_JUMP_TO);

	M_ RunProgramByName("TM6_EnemyThripidControl", MPartRef ());

	state = Begin;
}

bool TM6_MissionStart::Update (void)
{
	if(data.mission_over == true)
	{
		return false;
	}

	if(PlayerInSystem(PLAYER_ID, GHELEN) && state == Begin)
	{
//		data.mhandle = M_ PlayAudio("M06KR13.wav", data.KerTak);
		M_ EnableMovieMode(false);

		data.mhandle = M_ PlayAnimatedMessage("M06KR13.wav", "Animate!!KerTak2", MagpieLeft, MagpieTop, data.KerTak,IDS_TM6_SUB_M06KR13);
		state = Done;
	}

	if(state == Done && !M_ IsStreamPlaying(data.mhandle))
	{
//		data.mhandle = M_ PlayAudio("M06HA14.wav", data.Hawkes);
		data.mhandle = M_ PlayAnimatedMessage("M06HW14.wav", "Animate!!Hawkes2", MagpieLeft, MagpieTop, data.Hawkes,IDS_TM6_SUB_M06HW14);

		M_ RunProgramByName("TM6_FindMalkorBase",MPartRef ());
		M_ RunProgramByName("TM6_BlackHoleSeen",MPartRef ());
		
		return false;
	}
	return true;

}

//-----------------------------------------------------------------------------------------
//

CQSCRIPTPROGRAM(TM6_EnemyThripidControl, TM6_EnemyThripidControl_Save, 0);

void TM6_EnemyThripidControl::Initialize (U32 eventFlags, const MPartRef & part)
{
	data.SentryBaseDefeated = false;

	data.Thripid1 = FindFirstObjectOfType(M_THRIPID, MANTIS_ID, GHELEN);
	data.Thripid2 = FindNextObjectOfType(M_THRIPID, MANTIS_ID, GHELEN, data.Thripid1);

	data.XinNiad = FindFirstObjectOfType(M_NIAD, MANTIS_ID, XIN);

	data.RendarNiad = FindFirstObjectOfType(M_NIAD, MANTIS_ID, RENDAR);

	data.CorlarNiad1 = FindFirstObjectOfType(M_NIAD, MANTIS_ID, CORLAR);
	data.CorlarNiad2 = FindNextObjectOfType(M_NIAD, MANTIS_ID, CORLAR, data.CorlarNiad1);

	frigatetimer = scarabtimer = hivetimer = 0;
}

bool TM6_EnemyThripidControl::Update (void)
{
	if(data.mission_over == true)
	{
		return false;
	}

	frigatetimer -= ELAPSED_TIME;
	scarabtimer -= ELAPSED_TIME;
	hivetimer -= ELAPSED_TIME;

	if (M_ GetGas(MANTIS_ID) > 240) M_ SetGas(MANTIS_ID, 240);
	if (M_ GetMetal(MANTIS_ID) > 240) M_ SetMetal(MANTIS_ID, 240);
	if (M_ GetCrew(MANTIS_ID) > 200) M_ SetCrew(MANTIS_ID, 200);

	if(data.SentryBaseDefeated == false && frigatetimer <= 0)
	{
		if(data.Thripid1.isValid() && CountObjects(M_FRIGATE, MANTIS_ID, GHELEN) < 4)
		{
			M_ OrderBuildUnit(data.Thripid1, "GBOAT!!M_Frigate", false);
			frigatetimer = 20;
		}

		if(data.Thripid2.isValid() && CountObjects(M_SCOUTCARRIER, MANTIS_ID, GHELEN) < 3)
		{
			M_ OrderBuildUnit(data.Thripid2, "GBOAT!!M_Scout Carrier", false);
			frigatetimer = 20;
		}
	}

	if(scarabtimer <= 0)
	{
		if(data.XinNiad.isValid() && CountObjects(M_SCARAB, MANTIS_ID, XIN) < 5 && !data.XinFreed)
		{
			M_ OrderBuildUnit(data.XinNiad, "GBOAT!!M_Scarab", false);
			scarabtimer = 30;
		}

		if(data.CorlarNiad1.isValid() && CountObjects(M_SCARAB, MANTIS_ID, CORLAR) < 3)
		{
			M_ OrderBuildUnit(data.CorlarNiad1, "GBOAT!!M_Scarab", false);
			scarabtimer = 30;
		}
	}

	if(hivetimer <= 0)
	{
		if(data.RendarNiad.isValid() && CountObjects(M_HIVECARRIER, MANTIS_ID, RENDAR) < 5 && !data.RendarFreed)
		{
			M_ OrderBuildUnit(data.RendarNiad, "GBOAT!!M_Hive Carrier", false);
			hivetimer = 50;
		}

		if(data.CorlarNiad2.isValid() && CountObjects(M_HIVECARRIER, MANTIS_ID, CORLAR) < 3)
		{
			M_ OrderBuildUnit(data.CorlarNiad2, "GBOAT!!M_Hive Carrier", false);
			hivetimer = 50;
		}
	}


	return true;
}

//-----------------------------------------------------------------------------------------
//

CQSCRIPTPROGRAM(TM6_FindMalkorBase, TM6_FindMalkorBase_Save, 0);

void TM6_FindMalkorBase::Initialize (U32 eventFlags, const MPartRef & part)
{

}

bool TM6_FindMalkorBase::Update (void)
{
	if(data.mission_over == true)
	{
		return false;
	}

	MPartRef mPlatform = M_ GetFirstPart();

	while (mPlatform.isValid())
	{
		if(M_ IsVisibleToPlayer(mPlatform, PLAYER_ID) && M_ IsPlatform(mPlatform) == true && mPlatform->systemID == GHELEN && mPlatform->playerID == MANTIS_ID)
		{
//			M_ PlayAudio("M06KR18.wav", data.KerTak);
			data.mhandle = M_ PlayAnimatedMessage("M06KR18.wav", "Animate!!KerTak2", MagpieLeft, MagpieTop, data.KerTak, IDS_TM6_SUB_M06KR18);

			M_ RunProgramByName("TM6_DisableMalkorBase", MPartRef ());
			return false;
		}
		mPlatform = M_ GetNextPart(mPlatform);
	}
	return true;
}

//-----------------------------------------------------------------------------------------
//

CQSCRIPTPROGRAM(TM6_DisableMalkorBase, TM6_DisableMalkorBase_Save, 0);

void TM6_DisableMalkorBase::Initialize (U32 eventFlags, const MPartRef & part)
{
	state = Begin;
}

bool TM6_DisableMalkorBase::Update (void)
{
	if(data.mission_over == true)
	{
		return false;
	}

	switch(state)
	{
		case Begin:
			if((M_ DistanceTo(data.KerTak, data.MalkorBase) < 3 || M_ DistanceTo(data.KerTak, data.Thripid1) < 3 || M_ DistanceTo(data.KerTak, data.Thripid2) < 3))
			{
		//		data.mhandle = M_ PlayAudio("M06KR19.wav", data.KerTak);
				data.mhandle = M_ PlayAnimatedMessage("M06KR19.wav", "Animate!!KerTak2", MagpieLeft, MagpieTop, data.KerTak, IDS_TM6_SUB_M06KR19);

				MPartRef mPlatform = M_ GetFirstPart();

				M_ EnableSystem(CORLAR, true, true);
				M_ EnableSystem(XIN, true, true);
				M_ EnableSystem(RENDAR, true, true);

				while (mPlatform.isValid())
				{
					if(M_ IsPlatform(mPlatform) == true && mPlatform->systemID == GHELEN && mPlatform->playerID == MANTIS_ID)
					{
						M_ MakeDerelict(mPlatform);
					}
					mPlatform = M_ GetNextPart(mPlatform);
				}

				state = Next;
				data.SentryBaseDefeated = true;

				M_ ClearHardFog (data.XinCamp, 1);
				M_ ClearHardFog (data.CorlarCamp, 1);
				M_ ClearHardFog (data.RendarCamp, 1);

				M_ EnableJumpgate(M_ GetPartByName("Gate4"),true);
				M_ EnableJumpgate(M_ GetPartByName("Gate6"),true);

			}
			break;

		case Next:
			if(!M_ IsStreamPlaying(data.mhandle))
			{
		//		data.mhandle = M_ PlayAudio("M06HA20.wav", data.Hawkes);
				data.mhandle = M_ PlayAnimatedMessage("M06HW20.wav", "Animate!!Hawkes2", MagpieLeft, MagpieTop, data.Hawkes,IDS_TM6_SUB_M06HW20);

				state = Done;
			}
			break;

		case Done:
			if(!M_ IsStreamPlaying(data.mhandle))
			{
		//		data.mhandle = M_ PlayAudio("M06ST21.wav", data.Steele);
				data.mhandle = M_ PlayAnimatedMessage("M06ST21.wav", "Animate!!Steele2", MagpieLeft, MagpieTop, data.Steele, IDS_TM6_SUB_M06ST21);

				state = NewMO;
			}
			break;
			
		case NewMO:
			if(!M_ IsStreamPlaying(data.mhandle))
			{
				M_ FlushTeletype();

				data.mhandle = TeletypeObjective(IDS_TM6_OBJECTIVE2);
				M_ AddToObjectiveList(IDS_TM6_OBJECTIVE2);


				state = Ob2;
			}
			break;

		case Ob2:
			if(!M_ IsTeletypePlaying(data.mhandle))
			{
				M_ FlushTeletype();

				data.mhandle = TeletypeObjective(IDS_TM6_OBJECTIVE7);
				M_ AddToObjectiveList(IDS_TM6_OBJECTIVE5a);
				M_ AddToObjectiveList(IDS_TM6_OBJECTIVE5b);
				M_ AddToObjectiveList(IDS_TM6_OBJECTIVE5c);

				state = Ob3;
			}
			break;

		case Ob3:
			if(!M_ IsTeletypePlaying(data.mhandle))
			{
				M_ FlushTeletype();

				data.mhandle = TeletypeObjective(IDS_TM6_OBJECTIVE6);
				M_ RemoveFromObjectiveList(IDS_TM6_OBJECTIVE4);
				M_ AddToObjectiveList(IDS_TM6_OBJECTIVE6);

				M_ RunProgramByName("TM6_NewMO", MPartRef ());
				return false;
			}
			break;
	}
	return true;
}

//-----------------------------------------------------------------------------------------
//

CQSCRIPTPROGRAM(TM6_BlackHoleSeen, TM6_BlackHoleSeen_Save, 0);

void TM6_BlackHoleSeen::Initialize (U32 eventFlags, const MPartRef & part)
{
	state = Begin;
}

bool TM6_BlackHoleSeen::Update (void)
{
	if(data.mission_over == true)
	{
		return false;
	}

	if(state == Begin && M_ IsVisibleToPlayer(M_ GetPartByName("BlackHole"), PLAYER_ID) && !M_ IsStreamPlaying(data.mhandle))
	{
//		data.mhandle = M_ PlayAudio("M06ST15.wav", data.Steele);
		data.mhandle = M_ PlayAnimatedMessage("M06ST15.wav", "Animate!!Steele2", MagpieLeft, MagpieTop, data.Steele,IDS_TM6_SUB_M06ST15);

		state = Done;
	}

	if(state == Done && !M_ IsStreamPlaying(data.mhandle))
	{
//		data.mhandle = M_ PlayAudio("M06HW16.wav", data.Hawkes);
		data.mhandle = M_ PlayAnimatedMessage("M06HW16.wav", "Animate!!Hawkes2", MagpieLeft, MagpieTop, data.Hawkes,IDS_TM6_SUB_M06HW16);

		return false;
	}
	return true;
}

//-----------------------------------------------------------------------------------------
//

CQSCRIPTPROGRAM(TM6_NewMO, TM6_NewMO_Save, 0);

void TM6_NewMO::Initialize (U32 eventFlags, const MPartRef & part)
{
	M_ PlayMusic(DETERMINATION);

//	M_ RunProgramByName("TM6_XinCampFreed", MPartRef ());
//	M_ RunProgramByName("TM6_RendarCampFreed", MPartRef ());
//	M_ RunProgramByName("TM6_CorlarCampFreed", MPartRef ());
	M_ RunProgramByName("TM6_AllRebelsFreed", MPartRef());

	data.ExecutionTimer = 900;
}

bool TM6_NewMO::Update (void)
{
	if(data.mission_over == true)
	{
		return false;
	}

	data.ExecutionTimer -= ELAPSED_TIME;

	if(data.ExecutionTimer <= 0)
	{
		M_ RunProgramByName("TM6_RebelKilled", MPartRef ());
		return false;
	}

	if(data.ExecutionTimer > 840 && data.ExecutionTimer < 840.26)
	{
		M_ FlushTeletype();
		data.mhandle = M_ PlayTeletype(IDS_TM6_14MINS, 100, 320, 500, 350, MOTextColor, 4000, 1, false);
	}

	if(data.ExecutionTimer > 780 && data.ExecutionTimer < 780.26)
	{
		M_ FlushTeletype();
		data.mhandle = M_ PlayTeletype(IDS_TM6_13MINS, 100, 320, 500, 350, MOTextColor, 4000, 1, false);
	}

	if(data.ExecutionTimer > 720 && data.ExecutionTimer < 720.26)
	{
		M_ FlushTeletype();
		data.mhandle = M_ PlayTeletype(IDS_TM6_12MINS, 100, 320, 500, 350, MOTextColor, 4000, 1, false);
	}

	if(data.ExecutionTimer > 660 && data.ExecutionTimer < 660.26)
	{
		M_ FlushTeletype();
		data.mhandle = M_ PlayTeletype(IDS_TM6_11MINS, 100, 320, 500, 350, MOTextColor, 4000, 1, false);
	}

	if(data.ExecutionTimer > 600 && data.ExecutionTimer < 600.26)
	{
		M_ FlushTeletype();
		data.mhandle = M_ PlayTeletype(IDS_TM6_10MINS, 100, 320, 500, 350, MOTextColor, 4000, 1, false);
	}

	if(data.ExecutionTimer > 540 && data.ExecutionTimer < 540.26)
	{
		M_ FlushTeletype();
		data.mhandle = M_ PlayTeletype(IDS_TM6_9MINS, 100, 320, 500, 350, MOTextColor, 4000, 1, false);
	}

	if(data.ExecutionTimer > 480 && data.ExecutionTimer < 480.26)
	{
		M_ FlushTeletype();
		data.mhandle = M_ PlayTeletype(IDS_TM6_8MINS, 100, 320, 500, 350, MOTextColor, 4000, 1, false);
	}

	if(data.ExecutionTimer > 420 && data.ExecutionTimer < 420.26)
	{
		M_ FlushTeletype();
		data.mhandle = M_ PlayTeletype(IDS_TM6_7MINS, 100, 320, 500, 350, MOTextColor, 4000, 1, false);
	}

	if(data.ExecutionTimer > 360 && data.ExecutionTimer < 360.26)
	{
		M_ FlushTeletype();
		data.mhandle = M_ PlayTeletype(IDS_TM6_6MINS, 100, 320, 500, 350, MOTextColor, 4000, 1, false);
	}

	if(data.ExecutionTimer > 300 && data.ExecutionTimer < 300.26)
	{
		M_ FlushTeletype();
		data.mhandle = M_ PlayTeletype(IDS_TM6_5MINS, 100, 320, 500, 350, MOTextColor, 4000, 1, false);
	}

	if(data.ExecutionTimer > 240 && data.ExecutionTimer < 240.26)
	{
		M_ FlushTeletype();
		data.mhandle = M_ PlayTeletype(IDS_TM6_4MINS, 100, 320, 500, 350, MOTextColor, 4000, 1, false);
	}

	if(data.ExecutionTimer > 180 && data.ExecutionTimer < 180.26)
	{
		M_ FlushTeletype();
		data.mhandle = M_ PlayTeletype(IDS_TM6_3MINS, 100, 320, 500, 350, MOTextColor, 4000, 1, false);
	}

	if(data.ExecutionTimer > 120 && data.ExecutionTimer < 120.26)
	{
		M_ FlushTeletype();
		data.mhandle = M_ PlayTeletype(IDS_TM6_2MINS, 100, 320, 500, 350, MOTextColor, 4000, 1, false);
	}

	if(data.ExecutionTimer > 60 && data.ExecutionTimer < 60.26)
	{
		M_ FlushTeletype();
		data.mhandle = M_ PlayTeletype(IDS_TM6_1MIN, 100, 320, 500, 350, MOTextColor, 4000, 1, false);
	}

	if(data.ExecutionTimer > 30 && data.ExecutionTimer < 30.26)
	{
		M_ FlushTeletype();
		data.mhandle = M_ PlayTeletype(IDS_TM6_30SECS, 100, 320, 500, 350, MOTextColor, 4000, 1, false);
	}

		if(data.ExecutionTimer > 10 && data.ExecutionTimer < 10.26)
	{
		M_ FlushTeletype();
		data.mhandle = M_ PlayTeletype(IDS_TM6_10SECS, 100, 320, 500, 350, MOTextColor, 4000, 1, false);
	}

	if(data.ExecutionTimer > 9 && data.ExecutionTimer < 9.26)
	{
		M_ FlushTeletype();
		data.mhandle = M_ PlayTeletype(IDS_TM6_9SECS, 100, 320, 500, 350, MOTextColor, 1000, 1, false);
	}

	if(data.ExecutionTimer > 8 && data.ExecutionTimer < 8.26)
	{
		M_ FlushTeletype();
		data.mhandle = M_ PlayTeletype(IDS_TM6_8SECS, 100, 320, 500, 350, MOTextColor, 1000, 1, false);
	}

	if(data.ExecutionTimer > 7 && data.ExecutionTimer < 7.26)
	{
		M_ FlushTeletype();
		data.mhandle = M_ PlayTeletype(IDS_TM6_7SECS, 100, 320, 500, 350, MOTextColor, 1000, 1, false);
	}

	if(data.ExecutionTimer > 6 && data.ExecutionTimer < 6.26)
	{
		M_ FlushTeletype();
		data.mhandle = M_ PlayTeletype(IDS_TM6_6SECS, 100, 320, 500, 350, MOTextColor, 1000, 1, false);
	}

	if(data.ExecutionTimer > 5 && data.ExecutionTimer < 5.26)
	{
		M_ FlushTeletype();
		data.mhandle = M_ PlayTeletype(IDS_TM6_5SECS, 100, 320, 500, 350, MOTextColor, 1000, 1, false);
	}

	if(data.ExecutionTimer > 4 && data.ExecutionTimer < 4.26)
	{
		M_ FlushTeletype();
		data.mhandle = M_ PlayTeletype(IDS_TM6_4SECS, 100, 320, 500, 350, MOTextColor, 1000, 1, false);
	}

	if(data.ExecutionTimer > 3 && data.ExecutionTimer < 3.26)
	{
		M_ FlushTeletype();
		data.mhandle = M_ PlayTeletype(IDS_TM6_3SECS, 100, 320, 500, 350, MOTextColor, 1000, 1, false);
	}

	if(data.ExecutionTimer > 2 && data.ExecutionTimer < 2.26)
	{
		M_ FlushTeletype();
		data.mhandle = M_ PlayTeletype(IDS_TM6_2SECS, 100, 320, 500, 350, MOTextColor, 1000, 1, false);
	}

	if(data.ExecutionTimer > 1 && data.ExecutionTimer < 1.26)
	{
		M_ FlushTeletype();
		data.mhandle = M_ PlayTeletype(IDS_TM6_1SEC, 100, 320, 500, 350, MOTextColor, 1000, 1, false);
	}

	if(data.CorlarFreed == true && data.RendarFreed == true && data.XinFreed == true)
	{
		return false;
	}

	return true;
}

//-----------------------------------------------------------------------------------------
//

CQSCRIPTPROGRAM(TM6_AllRebelsFreed, TM6_AllRebelsFreed_Save, 0);

void TM6_AllRebelsFreed::Initialize (U32 eventFlags, const MPartRef & part)
{
	SiphonSpawn = M_ GetPartByName("Siphon Spawn");
	state = Begin;
}

bool TM6_AllRebelsFreed::Update (void)
{
	if(data.mission_over == true)
	{
		return false;
	}

	if(state == Begin && data.CorlarFreed == true && data.RendarFreed == true && data.XinFreed == true && !M_ IsStreamPlaying(data.mhandle))
	{
		data.AllFreed = true;
//		data.mhandle = M_ PlayAudio("M06KR25.wav", data.KerTak);
		data.mhandle = M_ PlayAnimatedMessage("M06KR25.wav", "Animate!!KerTak2", MagpieLeft, MagpieTop, data.KerTak, IDS_TM6_SUB_M06KR25);

		M_ EnableJumpgate(M_ GetPartByName("Gate8"),true);
		M_ EnableSystem(WOR, true, true);

		data.WorJumpGate = M_ CreatePart("PLATJUMP!!M_JumpPlat", M_ GetPartByName("Gate8"), MANTIS_ID); 
		MPartRef Siphon = M_ CreatePart("HARVEST!!M_Siphon", SiphonSpawn, MANTIS_ID);
		M_ OrderHarvest(Siphon, SiphonSpawn);

		M_ MarkObjectiveCompleted(IDS_TM6_OBJECTIVE1);
		M_ MarkObjectiveCompleted(IDS_TM6_OBJECTIVE2);
		M_ MarkObjectiveCompleted(IDS_TM6_OBJECTIVE5);
		
		state = Next;
	}

	if (state == Next && !M_ IsStreamPlaying(data.mhandle))
	{
//		data.mhandle = M_ PlayAudio("M06RL26.wav", data.CorlarWarlord);
		data.mhandle = M_ PlayAnimatedMessage("M06RL26.wav", "Animate!!MantisRebelLeader2", MagpieLeft, MagpieTop, data.CorlarWarlord, IDS_TM6_SUB_M06RL26);

		state = Done;
	}

	if (state == Done && !M_ IsStreamPlaying(data.mhandle))
	{
//		data.mhandle = M_ PlayAudio("M06KR27.wav", data.KerTak);
		data.mhandle = M_ PlayAnimatedMessage("M06KR27.wav", "Animate!!KerTak2", MagpieLeft, MagpieTop, data.KerTak, IDS_TM6_SUB_M06KR27);

		M_ RunProgramByName("TM6_JumpGateEncounterToWor", MPartRef());
		M_ PlayMusic(ESCORT);
		state = Final;
	}

	if(state == Final && !M_ IsStreamPlaying(data.mhandle))
	{
		M_ AlertSector(WOR);

		TeletypeObjective(IDS_TM6_WOR_REMINDER);
		return false;
	}

	return true;
}

//-----------------------------------------------------------------------------------------
//

CQSCRIPTPROGRAM(TM6_JumpGateEncounterToWor, TM6_JumpGateEncounterToWor_Save, 0);

void TM6_JumpGateEncounterToWor::Initialize (U32 eventFlags, const MPartRef & part)
{
	Collector = M_ GetPartByName("Collector");
	data.WorP = M_ GetPartByName("Wor Prime"); 
	data.Wor2 = M_ GetPartByName("Wor 2");

	data.CollectorSeen = false;

	state = GateSeen;
}

bool TM6_JumpGateEncounterToWor::Update (void)
{
	if(data.mission_over == true)
	{
		return false;
	}

	if(state == GateSeen && M_ IsVisibleToPlayer(data.WorJumpGate, PLAYER_ID) == true)
	{
//		data.mhandle = M_ PlayAudio("M06KR28.wav", data.KerTak);
		data.mhandle = M_ PlayAnimatedMessage("M06KR28.wav", "Animate!!KerTak2", MagpieLeft, MagpieTop, data.KerTak, IDS_TM6_SUB_M06KR28);

		M_ ClearHardFog(data.WorP, 3);
		M_ ClearHardFog(data.Wor2, 3);

		M_ RunProgramByName("TM6_RebelsAreHome", MPartRef());

		state = Hawkes;

	}

	if(state == Hawkes && !M_ IsStreamPlaying(data.mhandle))
	{
//		data.mhandle = M_ PlayAudio("M06HW29.wav", data.Hawkes);
		data.mhandle = M_ PlayAnimatedMessage("M06HW29.wav", "Animate!!Hawkes2", MagpieLeft, MagpieTop, data.Hawkes, IDS_TM6_SUB_M06HW29);

		M_ RunProgramByName("TM6_MimicAttack", MPartRef());

		state = CollectorSeen;
	}

	if(state == CollectorSeen && !M_ IsStreamPlaying(data.mhandle) && M_ IsVisibleToPlayer(Collector, PLAYER_ID))
	{

//		data.mhandle = M_ PlayAudio("M06KR30.wav", data.KerTak);
		data.mhandle = M_ PlayAnimatedMessage("M06KR30.wav", "Animate!!KerTak2", MagpieLeft, MagpieTop, data.KerTak, IDS_TM6_SUB_M06KR30);

		data.CollectorSeen = true;

		return false;
	}
	return true;
}


//-----------------------------------------------------------------------------------------
//

CQSCRIPTPROGRAM(TM6_MimicAttack, TM6_MimicAttack_Save, 0);

void TM6_MimicAttack::Initialize (U32 eventFlags, const MPartRef & part)
{
	Frig1 = M_ GetPartByName("F1");
	Frig2 = M_ GetPartByName("F2");
	Frig3 = M_ GetPartByName("F3");
	Frig4 = M_ GetPartByName("F4");
	Frig5 = M_ GetPartByName("F5");
	Frig6 = M_ GetPartByName("F6");
	Frig7 = M_ GetPartByName("F7");
	Frig8 = M_ GetPartByName("F8");

	data.Siphon = FindFirstObjectOfType(M_SIPHON, MANTIS_ID, WOR);

	state = Begin;

}

bool TM6_MimicAttack::Update (void)
{
	timer -= ELAPSED_TIME;

	if(data.mission_over == true)
	{
		return false;
	}

	switch(state)
	{
		case Begin:
			if(PlayerInSystem(PLAYER_ID, WOR))
			{
				M_ OrderSpecialAttack(Frig1, data.Siphon);
				M_ OrderSpecialAttack(Frig2, data.Siphon);
				M_ OrderSpecialAttack(Frig3, data.Siphon);
				M_ OrderSpecialAttack(Frig4, data.Siphon);
				M_ OrderSpecialAttack(Frig5, data.Siphon);
				M_ OrderSpecialAttack(Frig6, data.Siphon);
				M_ OrderSpecialAttack(Frig7, data.Siphon);
				M_ OrderSpecialAttack(Frig8, data.Siphon);

				state = Move;
			}
			break;

		case Move:
			if(data.CollectorSeen)
			{
				M_ OrderAttack(Frig1, M_ FindNearestEnemy(Frig1, false, true));
				M_ OrderAttack(Frig2, M_ FindNearestEnemy(Frig2, false, true));
				M_ OrderAttack(Frig3, M_ FindNearestEnemy(Frig3, false, true));
				M_ OrderAttack(Frig4, M_ FindNearestEnemy(Frig4, false, true));

				M_ OrderMoveTo(Frig5, M_ FindNearestEnemy(Frig5, false, true));
				M_ OrderMoveTo(Frig6, M_ FindNearestEnemy(Frig5, false, true));
				M_ OrderMoveTo(Frig7, M_ FindNearestEnemy(Frig5, false, true));
				M_ OrderMoveTo(Frig8, M_ FindNearestEnemy(Frig5, false, true));

				timer = 16;

				state = Reveal;
			}
			break;

		case Reveal:
			if(timer < 10 && !M_ IsStreamPlaying(data.mhandle))
			{
				if(Frig1.isValid())
				{
					M_ OrderCancel(Frig1);
					M_ OrderAttack(Frig1, M_ FindNearestEnemy(Frig1, false, true));
				}

				if(Frig2.isValid())
				{
					M_ OrderCancel(Frig2);
					M_ OrderAttack(Frig2, M_ FindNearestEnemy(Frig2, false, true));
				}

				if(Frig3.isValid())
				{
					M_ OrderCancel(Frig3);
					M_ OrderAttack(Frig3, M_ FindNearestEnemy(Frig3, false, true));
				}

				if(Frig4.isValid())
				{
					M_ OrderCancel(Frig4);
					M_ OrderAttack(Frig4, M_ FindNearestEnemy(Frig4, false, true));
				}

				state = Reveal2;
			}
			break;

		case Reveal2:
			if(timer < 0)
			{
				if(Frig5.isValid())
				{
					M_ OrderCancel(Frig5);
					M_ OrderAttack(Frig5, M_ FindNearestEnemy(Frig5, false, true));
				}

				if(Frig6.isValid())
				{
					M_ OrderCancel(Frig6);
					M_ OrderAttack(Frig6, M_ FindNearestEnemy(Frig6, false, true));
				}

				if(Frig7.isValid())
				{
					M_ OrderCancel(Frig7);
					M_ OrderAttack(Frig7, M_ FindNearestEnemy(Frig7, false, true));
				}

				if(Frig8.isValid())
				{
					M_ OrderCancel(Frig8);
					M_ OrderAttack(Frig8, M_ FindNearestEnemy(Frig8, false, true));
				}


				return false;
			}
			break;
	}

	return true;
}

//-----------------------------------------------------------------------------------------
//

CQSCRIPTPROGRAM(TM6_RebelsAreHome, TM6_RebelsAreHome_Save, 0);

void TM6_RebelsAreHome::Initialize (U32 eventFlags, const MPartRef & part)
{
	XinWarlordHome = RendarWarlordHome =CorlarWarlordHome = false;
}

bool TM6_RebelsAreHome::Update (void)
{
	if(data.mission_over == true)
	{
		return false;
	}

	if(data.XinWarlord->systemID == WOR && data.CorlarWarlord->systemID == WOR && data.RendarWarlord->systemID == WOR)
	{
		if(M_ DistanceTo(data.XinWarlord, data.WorP) <= 3 || M_ DistanceTo(data.XinWarlord, data.Wor2) <= 3)
		{
			XinWarlordHome = true;
		}

		if(M_ DistanceTo(data.CorlarWarlord, data.WorP) <= 3 || M_ DistanceTo(data.CorlarWarlord, data.Wor2) <= 3)
		{
			CorlarWarlordHome = true;
		}

		if(M_ DistanceTo(data.RendarWarlord, data.WorP) <= 3 || M_ DistanceTo(data.RendarWarlord, data.Wor2) <= 3)
		{
			RendarWarlordHome = true;
		}

		if(RendarWarlordHome == true && CorlarWarlordHome == true && XinWarlordHome == true)
		{
			M_ RunProgramByName("TM6_MissionSuccess", MPartRef());

			return false;
		}
	}

	return true;
}

//-----------------------------------------------------------------------
// OBJECT DESTRUCTION TRIGGER

CQSCRIPTPROGRAM(TM6_ObjectDestroyed, TM6_ObjectDestroyed_Save,CQPROGFLAG_OBJECTDESTROYED);

void TM6_ObjectDestroyed::Initialize (U32 eventFlags, const MPartRef & part)
{
	if(data.mission_over)
	{
		return;
	}

	switch (part->mObjClass)
	{
			//Check for destruction of mission critical objects...

		case M_NIAD:

			if(part == data.XinCamp || part == data.RendarCamp || part == data.CorlarCamp)
			{
        	    MScript::MoveCamera( part, 0, MOVIE_CAMERA_JUMP_TO);

				M_ RunProgramByName("TM6_RebelKilled", MPartRef());
			}
			break;

		case M_MISSILECRUISER:

			if(part == data.Hawkes)
			{
        	    MScript::MoveCamera( part, 0, MOVIE_CAMERA_JUMP_TO);

				M_ RunProgramByName("TM6_HawkesKilled", MPartRef ());
			}

			break;

		case M_BATTLESHIP:

			if(part == data.Steele)
			{
        	    MScript::MoveCamera( part, 0, MOVIE_CAMERA_JUMP_TO);

				M_ RunProgramByName("TM6_SteeleKilled", MPartRef());
			}

			break;

		case M_HIVECARRIER:

			if(part == data.KerTak)
			{
        	    MScript::MoveCamera( part, 0, MOVIE_CAMERA_JUMP_TO);

				M_ RunProgramByName("TM6_KerTakKilled", MPartRef());
			}

			break;

		case M_CORVETTE:
			
			if(part == data.XinWarlord || part == data.CorlarWarlord || part == data.RendarWarlord)
			{
        	    MScript::MoveCamera( part, 0, MOVIE_CAMERA_JUMP_TO);

				M_ RunProgramByName("TM6_RebelKilled", MPartRef());
			}

			break;

		case M_TROOPSHIP:
			
			data.Troopships--;
			if(data.RebelsToGo > data.Troopships && !data.AllFreed)
			{
				M_ FlushStreams();
        	    MScript::MoveCamera( part, 0, MOVIE_CAMERA_JUMP_TO);

				data.mhandle = M_ PlayAnimatedMessage("M06HA33a.wav", "Animate!!Halsey2", MagpieLeft, MagpieTop, IDS_TM6_SUB_M06HA33A);
				M_ RunProgramByName("TM6_MissionFailure", MPartRef());
			}

			break;

	}
}

bool TM6_ObjectDestroyed::Update (void)
{
	return false;
}

//-----------------------------------------------------------------------
// FORBIDDEN JUMP EVENT

CQSCRIPTPROGRAM(TM6_ForbiddenJump, TM6_ForbiddenJump_Save, CQPROGFLAG_FORBIDEN_JUMP);

void TM6_ForbiddenJump::Initialize (U32 eventFlags, const MPartRef & part)
{
	if(data.mission_over)
	{
		return;
	}

	MPartRef CentJump = M_ GetPartByName("Gate16");
	MPartRef BlackHole = M_ GetPartByName("Gate2");

	if (SameObj(part, CentJump))
	{
//		data.mhandle = M_ PlayAudio("M06ST17.wav", data.Steele);
		data.mhandle = M_ PlayAnimatedMessage("M06ST17.wav", "Animate!!Steele2", MagpieLeft, MagpieTop, data.Steele,IDS_TM6_SUB_M06ST17);

	}

	if (SameObj(part, BlackHole))
	{
	}
}

bool TM6_ForbiddenJump::Update (void)
{
	return false;
}

//-----------------------------------------------------------------------
// Troopship event


CQSCRIPTPROGRAM(TM6_Troopshipped, TM6_Troopshipped_Save, CQPROGFLAG_TROOPSHIPED);

void TM6_Troopshipped::Initialize (U32 eventFlags, const MPartRef & part)
{
	if(data.mission_over)
	{
		return;
	}

	data.Troopships--;

	if(M_ WasPart(data.XinCamp, part->dwMissionID))
	{
		data.XinWarlord = M_ CreatePart("CHAR!!Warlord", M_ GetPartByName("Xin Spawn"), PLAYER_ID, "Rebel Mantis Warlord"); 
		M_ EnablePartnameDisplay(data.XinWarlord, true);

//		data.mhandle = M_ PlayAudio("M06RL22.wav", data.XinWarlord);
		data.mhandle = M_ PlayAnimatedMessage("M06RL22.wav", "Animate!!MantisRebelLeader2", MagpieLeft, MagpieTop, data.XinWarlord,IDS_TM6_SUB_M06RL22);

//		data.ExecutionTimer = 360;
		data.XinFreed = true;
		data.RebelsToGo--;

		M_ MarkObjectiveCompleted(IDS_TM6_OBJECTIVE5a);

	}

	if(M_ WasPart(data.CorlarCamp, part->dwMissionID))
	{
		data.CorlarWarlord = M_ CreatePart("CHAR!!Warlord", M_ GetPartByName("Corlar Spawn"), PLAYER_ID, "Rebel Mantis Warlord"); 
		M_ EnablePartnameDisplay(data.CorlarWarlord, true);

//		data.mhandle = M_ PlayAudio("M06RL24.wav", data.CorlarWarlord);
		data.mhandle = M_ PlayAnimatedMessage("M06RL24.wav", "Animate!!MantisRebelLeader2", MagpieLeft, MagpieTop, data.CorlarWarlord, IDS_TM6_SUB_M06RL24);
		
//		data.ExecutionTimer = 360;
		data.CorlarFreed = true;
		data.RebelsToGo--;

		M_ MarkObjectiveCompleted(IDS_TM6_OBJECTIVE5c);

	}

	if(M_ WasPart(data.RendarCamp, part->dwMissionID))
	{
		data.RendarWarlord = M_ CreatePart("CHAR!!Warlord", M_ GetPartByName("Rendar Spawn"), PLAYER_ID, "Rebel Mantis Warlord"); 
		M_ EnablePartnameDisplay(data.RendarWarlord, true);

//		data.mhandle = M_ PlayAudio("M06RL23.wav", data.RendarWarlord);
		data.mhandle = M_ PlayAnimatedMessage("M06RL23.wav", "Animate!!MantisRebelLeader2", MagpieLeft, MagpieTop, data.RendarWarlord, IDS_TM6_SUB_M06RL23);

//		data.ExecutionTimer = 360;
		data.RendarFreed = true;
		data.RebelsToGo--;

		M_ MarkObjectiveCompleted(IDS_TM6_OBJECTIVE5b);
	}
}

bool TM6_Troopshipped::Update (void)
{
	return false;
}
//---------------------------------------------------------------------------
// MISSION FAILURE PROGRAMS
CQSCRIPTPROGRAM(TM6_HawkesKilled, TM6_HawkesKilled_Save, 0);

void TM6_HawkesKilled::Initialize (U32 eventFlags, const MPartRef & part)
{
	data.mission_over = true;
	M_ MarkObjectiveFailed(IDS_TM6_OBJECTIVE4);

	M_ FlushStreams();
}

bool TM6_HawkesKilled::Update (void)
{
	if (!M_ IsStreamPlaying(data.mhandle))
	{
//		data.mhandle = M_ PlayAudio("M06HA31.wav");
		data.mhandle = M_ PlayAnimatedMessage("M06HA31.wav", "Animate!!Halsey2", MagpieLeft, MagpieTop,IDS_TM6_SUB_M06HA31);

		data.failureID = IDS_TM6_FAIL_HAWKS_LOST;
		M_ RunProgramByName("TM6_MissionFailure", MPartRef ());
	
		return false;
	}

	return true;
}

CQSCRIPTPROGRAM(TM6_SteeleKilled, TM6_SteeleKilled_Save, 0);

void TM6_SteeleKilled::Initialize (U32 eventFlags, const MPartRef & part)
{
	data.mission_over = true;
	M_ MarkObjectiveFailed(IDS_TM6_OBJECTIVE4);

	M_ FlushStreams();
}

bool TM6_SteeleKilled::Update (void)
{
	if (!M_ IsStreamPlaying(data.mhandle))
	{
//		data.mhandle = M_ PlayAudio("M06HA32.wav");
		data.mhandle = M_ PlayAnimatedMessage("M06HA32.wav", "Animate!!Halsey2", MagpieLeft, MagpieTop, IDS_TM6_SUB_M06HA32);

		data.failureID = IDS_TM6_FAIL_STEEL_LOST;
		M_ RunProgramByName("TM6_MissionFailure", MPartRef ());
	
		return false;
	}

	return true;
}

CQSCRIPTPROGRAM(TM6_KerTakKilled, TM6_KerTakKilled_Save, 0);

void TM6_KerTakKilled::Initialize (U32 eventFlags, const MPartRef & part)
{
	data.mission_over = true;
	M_ MarkObjectiveFailed(IDS_TM6_OBJECTIVE4);

	M_ FlushStreams();
}

bool TM6_KerTakKilled::Update (void)
{
	if (!M_ IsStreamPlaying(data.mhandle))
	{
//		data.mhandle = M_ PlayAudio("M06HA33.wav");
		data.mhandle = M_ PlayAnimatedMessage("M06HA33.wav", "Animate!!Halsey2", MagpieLeft, MagpieTop,IDS_TM6_SUB_M06HA33);

		data.failureID = IDS_TM6_FAIL_KERTAK_LOST;
		M_ RunProgramByName("TM6_MissionFailure", MPartRef ());
	
		return false;
	}

	return true;
}

CQSCRIPTPROGRAM(TM6_RebelKilled, TM6_RebelKilled_Save, 0);

void TM6_RebelKilled::Initialize (U32 eventFlags, const MPartRef & part)
{
	data.mission_over = true;
	M_ MarkObjectiveFailed(IDS_TM6_OBJECTIVE4);

	M_ FlushStreams();
}

bool TM6_RebelKilled::Update (void)
{
	if (!M_ IsStreamPlaying(data.mhandle))
	{
//		data.mhandle = M_ PlayAudio("M06HA34.wav");
		data.mhandle = M_ PlayAnimatedMessage("M06HA34.wav", "Animate!!Halsey2", MagpieLeft, MagpieTop, IDS_TM6_SUB_M06HA34);

		data.failureID = IDS_TM6_FAIL_WARLORD_LOST;
		M_ RunProgramByName("TM6_MissionFailure", MPartRef ());
	
		return false;
	}

	return true;
}



#define TELETYPE_TIME 3000

CQSCRIPTPROGRAM(TM6_MissionFailure, TM6_MissionFailure_Save, 0);

void TM6_MissionFailure::Initialize (U32 eventFlags, const MPartRef & part)
{
	M_ PlayMusic(DEFEAT);

	M_ EnableMovieMode(true);
	UnderMissionControl();	
	data.mission_over = true;

	state = Begin;
}

bool TM6_MissionFailure::Update (void)
{
	switch (state)
	{
		case Begin:
			if (!M_ IsStreamPlaying(data.mhandle) && !M_ IsTeletypePlaying(data.mhandle))
			{
				M_ FlushTeletype();
				data.mhandle = TeletypeMissionOver(IDS_TM6_TELETYPE_FAILURE,data.failureID);
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


CQSCRIPTPROGRAM(TM6_MissionSuccess, TM6_MissionSuccess_Save,0);

void TM6_MissionSuccess::Initialize (U32 eventFlags, const MPartRef & part)
{
	M_ PlayMusic(VICTORY);

	M_ EnableMovieMode(true);
    MScript::MoveCamera( data.XinWarlord, 0, MOVIE_CAMERA_JUMP_TO);

	UnderMissionControl();
	data.mission_over = true;

	state = Begin;

}

bool TM6_MissionSuccess::Update (void)
{
	if(state == Begin)
	{
		data.shandle = M_ PlayAudio("high_level_data_uplink.wav");
		state = HalseyTalk;
	}

	if(state == HalseyTalk && !M_ IsStreamPlaying(data.shandle))
	{
//		data.mhandle = M_ PlayAudio("M06HA35.wav");
		data.mhandle = M_ PlayAnimatedMessage("M06HA35.wav", "Animate!!Halsey2", MagpieLeft, MagpieTop, IDS_TM6_SUB_M06HA35);

		state = PrintSuccess;
	}

	if(state == PrintSuccess && !M_ IsStreamPlaying(data.mhandle))
	{
		M_ FlushTeletype();
		data.mhandle = TeletypeMissionOver(IDS_TM6_TELETYPE_SUCCESS);
		state = Done;
	}

	if(state == Done && !M_ IsTeletypePlaying(data.mhandle))
	{
		UnderPlayerControl();
		M_ EndMissionVictory(5);
		return false;
	}

	return true;
}


//--------------------------------------------------------------------------//
//--------------------------------End Script06T.cpp-------------------------//
//--------------------------------------------------------------------------//
