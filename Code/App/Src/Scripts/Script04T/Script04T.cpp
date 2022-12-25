//--------------------------------------------------------------------------//
//                                                                          //
//                                Script04T.cpp                             //
//				/Conquest/App/Src/Scripts/Script02T/Script04T.cpp			//
//								MISSION PROGRAMS							//
//                                                                          //
//               COPYRIGHT (C) 2000 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
	Created:	4/28/00		JeffP
	Modified:	5/1/00		JeffP

*/
//--------------------------------------------------------------------------//
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
#define REBEL_ID			4

#define PERILON				1
#define LUXOR				2
#define BEKKA				3
#define ALTO				4
#define GHELEN				5
#define CAPELLA				6

#define NUM_REBEL_HARVESTERS 4


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

CQBRIEFINGITEM slotAnim;

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
			 TECHTREE::RES_REFINERY_GAS1 | TECHTREE::RES_REFINERY_METAL1 );
             
        mission_tech.race[1].build = (TECHTREE::BUILDNODE) (
            mission_tech.race[1].build |
             TECHTREE::MDEPEND_BIOFORGE |	
             TECHTREE::MDEPEND_WARLORDTRAIN);

		mission_tech.race[0].tech = mission_tech.race[1].tech =
			(TECHTREE::TECHUPGRADE) 
			(TECHTREE::T_SHIP__FABRICATOR | TECHTREE::T_SHIP__CORVETTE | TECHTREE::T_SHIP__HARVEST | 
			 TECHTREE::T_SHIP__MISSILECRUISER | TECHTREE::T_SHIP__TROOPSHIP | TECHTREE::T_RES_TROOPSHIP1 |
			 TECHTREE::T_SHIP__SUPPLY | TECHTREE::T_SHIP__INFILTRATOR | TECHTREE::T_SHIP__BATTLESHIP |
			 TECHTREE::T_RES_MISSLEPACK1 | TECHTREE::T_RES_MISSLEPACK2);
		mission_tech.race[1].tech =
			(TECHTREE::TECHUPGRADE)
			(mission_tech.race[1].tech | TECHTREE::M_SHIP_KHAMIR);
		mission_tech.race[0].common = mission_tech.race[1].common =
			(TECHTREE::COMMON)
			(TECHTREE::RES_SUPPLY1 | TECHTREE::RES_WEAPONS1 | TECHTREE::RES_SUPPLY2 | TECHTREE::RES_WEAPONS2 |
			 TECHTREE::RES_HULL1); 
		mission_tech.race[0].common_extra = mission_tech.race[1].common_extra =
			(TECHTREE::COMMON_EXTRA)
			(TECHTREE::RES_TENDER1 | TECHTREE::RES_TANKER1 | TECHTREE::RES_TENDER2 | TECHTREE::RES_TANKER2 | 
			 TECHTREE::RES_SENSORS1); 
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

CQSCRIPTPROGRAM(TM4_Briefing, TM4_Briefing_Data,CQPROGFLAG_STARTBRIEFING);

void TM4_Briefing::Initialize (U32 eventFlags, const MPartRef & part)
{
	data.failureID = 0;
	data.mission_over = data.briefing_over = data.bBuiltPlatform = false;
	data.bLuxorAttack = data.bBekkaAttack = data.bAcademy = data.bHeavyInd = data.bWave = false;
	data.mhandle = 0;
	data.shandle = 0;
	data.numRebelsDead = data.numWaves = 0;
	data.mission_state = Briefing;
    data.numExits = 0;

    data.displayObjectiveHandlerParams_teletypeHandle = 0;

	state = Begin;

	M_ PlayMusic( NULL );

	M_ EnableBriefingControls(true);

}

#define TIME_TO_TELETYPE	10
#define TEXT_PRINT_TIME		5000
#define TEXT_HOLD_TIME		10000
#define TEXT_HOLD_TIME_INF  0

bool TM4_Briefing::Update (void)
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
			data.mhandle = TeletypeBriefing(IDS_TM4_TELETYPE_LOCATION, false);
			//data.mhandle = M_ PlayBriefingTeletype(IDS_TM4_TELETYPE_LOCATION, MOTextColor, 8000, 2000, false);
#else
			//FIX TO STANDARDIZE WITH MISSION 1
			TeletypeBriefing(IDS_TM4_TELETYPE_LOCATION, false);
			timer = 0;
		    //TeletypeBriefing(IDS_TM4_TELETYPE_LOCATION, true);
			//timer = 128;
#endif
			
			state = RadioBroadcast;
			break;
		
		case RadioBroadcast:

#ifdef _TELETYPE_SPEECH
			if (!M_ IsTeletypePlaying(data.mhandle))		
#else
			if (timer > 0) 
			{ 
				timer--; 
				break; 
			}
			else
#endif
			{
				//data.mhandle = DoBriefingSpeech(IDS_TM4_DAVIS_M04JD01, "M04JD01.wav");
				data.mhandle = DoBriefingSpeech(IDS_TM4_SUB_PROLOG04, "PROLOG04.wav", 8000, 2000, 0, CHAR_NOCHAR);
				//state = RadioBroadcast2;
				state = PreHalseyBrief;

				ShowBriefingAnimation(0, "TNRLogo", ANIMSPD_TNRLOGO, true, true);
				ShowBriefingAnimation(3, "TNRLogo", ANIMSPD_TNRLOGO, true, true);
				ShowBriefingAnimation(1, "Radiowave", ANIMSPD_RADIOWAVE, true, true);
				ShowBriefingAnimation(2, "Radiowave", ANIMSPD_RADIOWAVE, true, true);
			}
			break;

		case RadioBroadcast2:

			/*
			if (!IsSomethingPlaying(data.mhandle))
			{
				data.mhandle = DoBriefingSpeech(IDS_TM4_ANNOUNCER_M04AN02, "M04AN02.wav");
				state = HalseyBrief;				 
			}
			*/
			break;

		case PreHalseyBrief:

			if (!M_ IsStreamPlaying(data.mhandle))	//audio
			{
				M_ FreeBriefingSlot(-1);

				M_ PlayAudio("high_level_data_uplink.wav");

				ShowBriefingAnimation(0, "Xmission", 2500/30, false, false); 
				state = HalseyBrief;
			}
			break;

		case HalseyBrief:

			if (M_ IsCustomBriefingAnimationDone(0))	
			{
				M_ FreeBriefingSlot(-1);

				data.mhandle = DoBriefingSpeechMagpie(IDS_TM4_SUB_M04HA03, "M04HA03.wav", 8000, 2000, 0, CHAR_HALSEY, true);
				state = HalseyBrief2;
			}
			break;

		case HalseyBrief2:

			if (!M_ IsStreamPlaying(data.mhandle))	//audio
			{
				handle2 = ShowBriefingAnimation(1, "AndromedaWreckage");
				
				data.mhandle = DoBriefingSpeechMagpie(IDS_TM4_SUB_M04HA03A, "M04HA03a.wav", 8000, 2000, 0, CHAR_HALSEY, true);
				state = HalseyBrief3;
			}
			break;

		case HalseyBrief3:

			if (!M_ IsStreamPlaying(data.mhandle))	//audio
			{
				M_ FreeBriefingSlot(1);

				data.mhandle = DoBriefingSpeechMagpie(IDS_TM4_SUB_M04HA03B, "M04HA03b.wav", 8000, 2000, 0, CHAR_HALSEY, false);
				state = DisplayMO;
			}
			break;

		case DisplayMO:
		
			if (!M_ IsStreamPlaying(data.mhandle))	//audio
			{
				M_ FreeBriefingSlot(0);
				M_ FreeBriefingSlot(1);

			    M_ FlushTeletype();
				data.mhandle = TeletypeBriefing(IDS_TM4_OBJECTIVES, true);
		 	    //data.mhandle = M_ PlayBriefingTeletype(IDS_TM4_OBJECTIVES, MOTextColor, TEXT_HOLD_TIME_INF, 3000, false);
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
	if (obj1->dwMissionID == obj2->dwMissionID)
		return true;
	else
		return false;
}

//--------------------------------------------------------------------------//
/*
CQSCRIPTPROGRAM(TM4_BriefingSkip, TM4_BriefingSkip_Data,CQPROGFLAG_SKIP);

void TM4_BriefingSkip::Initialize (U32 eventFlags, const MPartRef & part)
{
	data.briefing_over = true;

	//UnderPlayerControl();
	M_ FlushTeletype();
	M_ FlushStreams();
		
	M_ EnableBriefingControls(false);

	M_ RunProgramByName("TM4_MissionStart", MPartRef ());
}

bool TM4_BriefingSkip::Update (void)
{
	return false;
}
*/

//--------------------------------------------------------------------------//

bool IsExiting(U32 missionID)
{
	for (U32 x=0; x < data.numExits; x++)
	{
		if ( missionID == data.exitID[x] )
			return true;
	}

	return false;
}

void AddExiter(U32 missionID)
{
    if ( data.numExits < 64 )
    {
	    data.exitID[data.numExits] = missionID;
	    data.numExits++;
    }
}

void ExitCleanup(void)
{
	for (U32 x = 0; x < data.numExits; x++)
	{
		MPartRef obj;

		obj = M_ GetPartByID( data.exitID[x] );

		if ( obj.isValid() && obj->systemID != ALTO )
		{
			if ( x < data.numExits-1 )
				data.exitID[x] = data.exitID[data.numExits-1];

			data.numExits--;
			x--;
		}
	}
}

//--------------------------------------------------------------------------

CQSCRIPTPROGRAM(TM4_ControlProgram, TM4_ControlProgram_Data, 0);

void TM4_ControlProgram::Initialize (U32 eventFlags, const MPartRef & part)
{
	bMsg = false;
}

bool TM4_ControlProgram::Update(void)
{
	if (data.mission_over)
		return false;

	if (PlayerInSystem(PLAYER_ID, ALTO) && data.mission_state < InvestigateAlto)
	{
		MPartRef obj;
		
		obj = M_ GetFirstPart();

		while (obj.isValid())
		{
			if (obj->playerID == PLAYER_ID && obj->systemID == ALTO && !M_ IsPlatform(obj))
			{
				if (!IsExiting(obj->dwMissionID))
				{
					M_ OrderCancel(obj);
					M_ OrderMoveTo(obj, M_ GetPartByName("LocFabPerilon"));
					AddExiter(obj->dwMissionID);
					if (!bMsg)
					{
						data.mhandle = DoSpeech(IDS_TM4_SUB_M04HW28, "M04HW28.wav", 8000, 2000, data.Hawkes, CHAR_HAWKES);
						bMsg = true;
					}
				}
			}

			obj = M_ GetNextPart(obj);
		}
	}

	ExitCleanup();

	return true;
}


//--------------------------------------------------------------------------

CQSCRIPTPROGRAM(TM4_MissionStart, TM4_MissionStart_Data,CQPROGFLAG_STARTMISSION);

void TM4_MissionStart::Initialize (U32 eventFlags, const MPartRef & part)
{
	MPartRef loc, tmp;

	data.mission_state = MissionStart;


	M_ EnableSystem(ALTO, false, false);
	M_ EnableSystem(GHELEN, false, false);
	M_ EnableSystem(BEKKA, false, false);
	M_ EnableSystem(LUXOR, false, false);

	M_ EnableSystem(CAPELLA, false, false);
	tmp = M_ GetFirstPart();
	while (tmp.isValid())
	{
		if (M_ IsPlatform(tmp) && tmp->playerID == PLAYER_ID && tmp->systemID == CAPELLA)
			M_ EnableSelection(tmp, false);
		tmp = M_ GetNextPart(tmp);
	}

/*
	M_ ClearPath(CAPELLA, PERILON, PLAYER_ID);
	M_ ClearPath(LUXOR, PERILON, MANTIS_ID);
	M_ ClearPath(BEKKA, PERILON, MANTIS_ID);
	M_ ClearPath(ALTO, PERILON, REBEL_ID);
	M_ ClearPath(PERILON, ALTO, REBEL_ID);
*/
	// Initialize Terran ships
	data.Blackwell = M_ GetPartByName("Blackwell");
	data.Hawkes	= M_ GetPartByName("Hawkes");
	M_ EnablePartnameDisplay(data.Blackwell, true);
	M_ EnablePartnameDisplay(data.Hawkes, true);
		
	// Initialize Rebel ships

	data.Rebel1 = FindFirstObjectOfType(M_SIPHON, REBEL_ID, PERILON);
	data.Rebel2 = FindNextObjectOfType(M_SIPHON, REBEL_ID, PERILON, data.Rebel1);
	data.Rebel3 = FindNextObjectOfType(M_SIPHON, REBEL_ID, PERILON, data.Rebel2);
	data.Rebel4 = FindNextObjectOfType(M_SIPHON, REBEL_ID, PERILON, data.Rebel3);
	
	data.RebelGate = FindFirstObjectOfType(M_JUMPPLAT, REBEL_ID, PERILON);
	
	// Initialize player resources
	M_ SetMaxGas(PLAYER_ID, 200);
	M_ SetMaxMetal(PLAYER_ID, 200);
	M_ SetMaxCrew(PLAYER_ID, 300);

	M_ SetGas(PLAYER_ID, 200);
	M_ SetMetal(PLAYER_ID, 200);
	M_ SetCrew(PLAYER_ID, 300);

	// Initialize Rebel gas
	M_ SetMaxGas(REBEL_ID, 400);
	M_ SetGas(REBEL_ID, 0);

	SetTechLevel(1);

	M_ SetEnemyCharacter(MANTIS_ID, IDS_TM4_MANTIS);
	M_ SetEnemyCharacter(REBEL_ID, IDS_TM4_REBELS);

	// Set stance for Frigates near Rebels
	MassStance(M_FRIGATE, MANTIS_ID, US_STOP, PERILON);

	// Set stance for Bekka Mantis
	MassStance(M_SCOUTCARRIER, MANTIS_ID, US_DEFEND, BEKKA);

	// Set stance for Luxor Mantis
	MassStance(M_SCARAB, MANTIS_ID, US_DEFEND, LUXOR);
	MassStance(M_FRIGATE, MANTIS_ID, US_DEFEND, LUXOR);

	M_ EnableJumpgate(M_ GetPartByName("Wormhole to Capella"), false);
	M_ EnableJumpgate(M_ GetPartByName("Wormhole to Alto"), false);
	M_ EnableJumpgate(M_ GetPartByName("Gate10"), false);
	M_ EnableJumpgate(M_ GetPartByName("Gate15"), false);
	M_ EnableJumpgate(M_ GetPartByName("Gate16"), false);
	M_ EnableJumpgate(M_ GetPartByName("Gate18"), false);

	M_ EnablePartnameDisplay(M_ GetPartByName("Wormhole to Alto"), true);
	M_ EnablePartnameDisplay(M_ GetPartByName("Wormhole to Capella"), true);

	M_ SetMissionID(3);
	M_ SetMissionName(IDS_TM4_MISSION_NAME);
	M_ SetMissionDescription(IDS_TM4_MISSION_DESC);

	M_ AddToObjectiveList(IDS_TM4_OBJECTIVE1);
	M_ AddToObjectiveList(IDS_TM4_OBJECTIVE6);

	// Set visible area for Jump Gate to Alkaid Prime in Perilon Prime System
	M_ ClearHardFog (M_ GetPartByName("Wormhole to Capella"), 1);

	//loc	= M_ GetPartByName("BattleWayPoint");
	//MassStance(M_BATTLESHIP, PLAYER_ID, US_STOP, CAPELLA);
	//MassMove(M_BATTLESHIP, PLAYER_ID, loc, CAPELLA);

	loc	= M_ GetPartByName("MissileWayPoint");
	MassStance(M_MISSILECRUISER, PLAYER_ID, US_STOP, CAPELLA);
	MassMove(M_MISSILECRUISER, PLAYER_ID, loc, CAPELLA);

	//loc	= M_ GetPartByName("SupplyWayPoint");
	//MassStance(M_INFILTRATOR, PLAYER_ID, US_STOP, CAPELLA);
	//MassMove(M_INFILTRATOR, PLAYER_ID, loc, CAPELLA);

	loc	= M_ GetPartByName("CorvetteWayPoint");
	MassStance(M_CORVETTE, PLAYER_ID, US_STOP, CAPELLA);
	MassMove(M_CORVETTE, PLAYER_ID, loc, CAPELLA);

	//loc	= M_ GetPartByName("SpyWayPoint");
	//MassStance(M_SUPPLY, PLAYER_ID, US_STOP, CAPELLA);
	//MassMove(M_SUPPLY, PLAYER_ID, loc, CAPELLA);

	// KEEPS BLACKWELL AND HAWKES FROM BEING PUSSIES
	M_ EnableJumpCap(data.Blackwell, false);
	M_ EnableJumpCap(data.Hawkes, false);

	M_ EnableRegenMode(true);

	// Set camera to Mission start position
	//M_ ChangeCamera(M_ GetPartByName("MissionCamStart"), 0, MOVIE_CAMERA_JUMP_TO);
	M_ MoveCamera(M_ GetPartByName("Wormhole to Capella"), 0, MOVIE_CAMERA_JUMP_TO);

	M_ RunProgramByName("TM4_ControlProgram", MPartRef ());

	M_ FlushTeletype();
	data.mhandle = M_ PlayTeletype(IDS_TM4_PERILON, SuperLeft, SuperTop, SuperRight, SuperBottom, MOTextColor, 4000, 1000, false);

	M_ EnableMovieMode(true);

	M_ PlayMusic( NEGOTIATION );

	state = Begin;
}

bool TM4_MissionStart::Update (void)
{
	if (data.mission_over)
		return false;

	switch (state)
	{

		case Begin:

#ifdef _TELETYPE_SPEECH
			if (!M_ IsTeletypePlaying(data.mhandle))
#endif
			{
				state = Talk1;
			}

		case Talk1:

			// Check to see if hawkes is in Perilon Prime 
			if(data.Hawkes->systemID == PERILON)	
			{
				data.mhandle = DoSpeech(IDS_TM4_SUB_M04HW05, "M04HW05.wav", 8000, 2000, data.Hawkes, CHAR_HAWKES);

				data.RebelGateAlto = M_ GetJumpgateSibling(data.RebelGate);	//dont ask me
				M_ EnableMovieMode(false);

 				state = Talk2;
			}
			break;

		case Talk2:

			if(data.Blackwell->systemID == PERILON && !IsSomethingPlaying(data.mhandle))
			{
				data.mhandle = DoSpeech(IDS_TM4_SUB_M04BL06, "M04BL06.wav", 8000, 2000, data.Blackwell, CHAR_BLACKWELL);
				state = Done;
				//state = LookAtGate;
			}
			break;

		/*
		case LookAtGate:

			if(!M_ IsStreamPlaying(data.mhandle))
			{
				// Show jumpgate to Perilon 2 (ALTO)
				M_ ChangeCamera(M_ GetPartByName("MissionCamTurn"), 2, MOVIE_CAMERA_SLIDE_TO);
				//M_ ChangeCamera(M_ GetPartByName("Wormhole to Alto"), 4, MOVIE_CAMERA_SLIDE_TO);
				//M_ ChangeCamera(M_ GetPartByName("MissionCamStart"), 4, MOVIE_CAMERA_SLIDE_TO | MOVIE_CAMERA_QUEUE);
				timer = 4;
				state = LookAtGate2;
			}			
			break;

		case LookAtGate2:

			if (timer > 0) { timer--; break; }
			state = Done;
			break;
		*/

		case Done:

			if (CountObjects(M_MISSILECRUISER, PLAYER_ID, CAPELLA) == 0 &&
				CountObjects(M_CORVETTE, PLAYER_ID, CAPELLA) == 0)
			{
				M_ RunProgramByName("TM4_ScoutPerilon", MPartRef ());
			}
			return false;

	}

	return true;
}



//--------------------------------------------------------------------------//
// Mission to Scout Perilon and find Perilon2 Jump Gate 

void FixRebels()
{
	U32 hullmax = data.Rebel1->hullPointsMax;

	M_ SetHullPoints(data.Rebel1, hullmax);
	M_ SetHullPoints(data.Rebel2, hullmax);
}

CQSCRIPTPROGRAM(TM4_ScoutPerilon, TM4_ScoutPerilon_Data, 0);

void TM4_ScoutPerilon::Initialize (U32 eventFlags, const MPartRef & part)
{
	data.mission_state = ScoutPerilon;

	// relocated from TM4_MissionStart to prevent crashing
	M_ ClearPath(CAPELLA, PERILON, PLAYER_ID);
	M_ ClearPath(LUXOR, PERILON, MANTIS_ID);
	M_ ClearPath(BEKKA, PERILON, MANTIS_ID);
	M_ ClearPath(ALTO, PERILON, REBEL_ID);
	M_ ClearPath(PERILON, ALTO, REBEL_ID);

	//data.RebelGateAlto = M_ GetJumpgateSibling(data.RebelGate);

	state = Begin;
}


bool TM4_ScoutPerilon::Update (void)
{
	if (data.mission_over)
		return false;

	FixRebels();

	switch (state)
	{

		case Begin:

			M_ SetAllies(PLAYER_ID, REBEL_ID, true);
			M_ SetAllies(REBEL_ID, PLAYER_ID, true);

			//nomissioncontrol
			//M_ MoveCamera(data.Rebel2, 12, MOVIE_CAMERA_SLIDE_TO);
			data.AlertID = M_ StartAlertAnim(M_ GetPartByName("New_BattleWP"));
			
   			// Start attacking rebels
			MassStance(M_FRIGATE, MANTIS_ID, US_ATTACK, PERILON);
	
			state = Begin2;
			break;

		case Begin2:

			if (!IsSomethingPlaying(data.mhandle))
			{
				//TODO:  should actually wait until battle viewable on screen
				data.mhandle = DoSpeech(IDS_TM4_SUB_M04HW08, "M04HW08.wav", 8000, 2000, data.Hawkes, CHAR_HAWKES);
				M_ MarkObjectiveCompleted(IDS_TM4_OBJECTIVE1);
   				state = Talk1;
			}
			break;

		case Talk1:

			if (!IsSomethingPlaying(data.mhandle))
			{
				data.mhandle = DoSpeech(IDS_TM4_SUB_M04BL09, "M04BL09.wav", 8000, 2000, data.Blackwell, CHAR_BLACKWELL);
				state = Talk2;
			}
			break;

		case Talk2:

			if (!IsSomethingPlaying(data.mhandle))
			{
				data.mhandle = DoSpeech(IDS_TM4_SUB_M04HW10, "M04HW10.wav", 8000, 2000, data.Hawkes, CHAR_HAWKES);
				state = Done;
			}
			break;


		case Done:

			if (!IsSomethingPlaying(data.mhandle))
			{
				M_ StopAlertAnim(data.AlertID);

				M_ RunProgramByName("TM4_KillRedMantis", MPartRef ());
				return false;
			}
			break;

	}
	
	return true;
}


//--------------------------------------------------------------------------//
// Mission to stop attacks on rebels by killing Red Mantis Frigates

CQSCRIPTPROGRAM(TM4_KillRedMantis, TM4_KillRedMantis_Data, 0);

void TM4_KillRedMantis::Initialize (U32 eventFlags, const MPartRef & part)
{
	data.mission_state = KillRedMantis;
	state = Begin;

	// !! using new objective method
	//M_ AddToObjectiveList(IDS_TM4_OBJECTIVE2);
}


bool TM4_KillRedMantis :: Update (void)
{
	if (data.mission_over)
		return false;

	switch (state)
	{

		case Begin:

			/*
			if (!M_ IsTeletypePlaying(data.mhandle))
			{
				M_ FlushTeletype();
				data.mhandle = M_ PlayTeletype(IDS_TM4_NEW_OBJ1, MOLeft, MOTop, MORight, MOBottom, MOTextColor, 10000, 2000, false);
				state = KillMantis;
			}*/

			//UnderPlayerControl();		//nomissioncontrol, lets see how long it lasts

			MassStance(0, PLAYER_ID, US_DEFEND, PERILON);

			M_ AddToObjectiveList(IDS_TM4_OBJECTIVE2);

			state = NewObj;
			break;

		case NewObj:

			FixRebels();

			if (!IsSomethingPlaying(data.mhandle))
			{
				if (CountObjects(M_FRIGATE, MANTIS_ID, PERILON) > 0)
					data.shandle = TeletypeObjective(IDS_TM4_OBJECTIVE2);
				state = KillMantis;
			}
			break;

		case KillMantis:

			if (M_ IsTeletypePlaying(data.shandle))  FixRebels();

			if (CountObjects(M_FRIGATE, MANTIS_ID, PERILON) == 0)
			{
				state = Done;
			}
			break;

		case Done:
		
			M_ RunProgramByName("TM4_TalkWithRebels", MPartRef ());
			return false;

	}

	return true;
}

//--------------------------------------------------------------------------//
// Discussion with Terrans and Mantis rebels



CQSCRIPTPROGRAM(TM4_TalkWithRebels, TM4_TalkWithRebels_Data, 0);

void TM4_TalkWithRebels::Initialize (U32 eventFlags, const MPartRef &part)
{
	MPartRef loc;

	data.mission_state = TalkWithRebels;
	M_ MarkObjectiveCompleted(IDS_TM4_OBJECTIVE2);

	//loc = M_ GetPartByName("New_BattleWP");
	//MassMove(M_BATTLESHIP, PLAYER_ID, loc, PERILON);

	loc = M_ GetPartByName("New_MissileWP");
	MassMove(M_MISSILECRUISER, PLAYER_ID, loc, PERILON);

	//loc = M_ GetPartByName("New_SpyWP");
	//MassMove(M_INFILTRATOR, PLAYER_ID, loc, PERILON);

	loc = M_ GetPartByName("New_CorvetteWP");
	MassMove(M_CORVETTE, PLAYER_ID, loc, PERILON);

	//loc = M_ GetPartByName("New_SupplyWP");
	//MassMove(M_SUPPLY, PLAYER_ID, loc, PERILON);
	
	//loc = M_ GetPartByName("New_BattleWP");
	//MassMove(M_BATTLESHIP, PLAYER_ID, loc, PERILON);

	loc = M_ GetPartByName("New_SupplyWP");
	M_ OrderMoveTo(data.Blackwell, loc);

	loc = M_ GetPartByName("New_BattleWP");
	M_ OrderMoveTo(data.Hawkes, loc);

	//UnderMissionControl();		//nomissioncontrol, lets see how long it lasts

	// Make nebulas visible to rebels
	M_ MakeAreaVisible(REBEL_ID, M_ GetPartByName("Perilon Prime"), 100);


	M_ FlushStreams();
	MPartRef aRebel;
	if (data.Rebel1.isValid())
		aRebel = data.Rebel1;
	else if (data.Rebel2.isValid())
		aRebel = data.Rebel2;
	CQASSERT(aRebel.isValid());
	
	//nomissioncontrol:  lets see how long it lasts
	//M_ ChangeCamera(M_ GetPartByName("MissionCamRebelTalk"), 4, MOVIE_CAMERA_SLIDE_TO);

	data.mhandle = DoSpeech(IDS_TM4_SUB_M04MR11, "M04MR11.wav", 8000, 2000, aRebel, CHAR_KERTAK);
	
	state = Begin;
}


bool TM4_TalkWithRebels::Update (void)
{
	MPartRef aRebel, tmp;

	if (data.mission_over)
		return false;

	if (data.Rebel1.isValid())
		aRebel = data.Rebel1;
	else if (data.Rebel2.isValid())
		aRebel = data.Rebel2;
	
	switch (state)
	{

		case Begin:

			if (!IsSomethingPlaying(data.mhandle))
			{
				data.mhandle = DoSpeech(IDS_TM4_SUB_M04HW12, "M04HW12.wav", 8000, 2000, data.Hawkes, CHAR_HAWKES);
				state = State3;
			}
			break;

		/*
		case State1:

			if (!IsSomethingPlaying(data.mhandle))
			{
				data.mhandle = DoSpeech(IDS_TM4_ENGINEER_M04EN13, "M04EN13.wav", 8000, 2000, MPartRef(), CHAR_NOCHAR);
				state = State2;
			}
			break;

		case State2:

			if (!IsSomethingPlaying(data.mhandle))
			{
				data.mhandle = DoSpeech(IDS_TM4_HAWKES_M04HW14, "M04HW14.wav", 8000, 2000, data.Hawkes, CHAR_HAWKES);
				state = State3;
			}
			break;
		*/

		case State3:

			if (!IsSomethingPlaying(data.mhandle))
			{
				data.mhandle = DoSpeech(IDS_TM4_SUB_M04MR15, "M04MR15.wav", 8000, 2000, aRebel, CHAR_KERTAK);
				state = State4;
			}
			break;

		case State4:

			if (!IsSomethingPlaying(data.mhandle))
			{
				data.mhandle = DoSpeech(IDS_TM4_SUB_M04BL16, "M04BL16.wav", 8000, 2000, data.Blackwell, CHAR_BLACKWELL);
				state = State5;
			}
			break;

		case State5:

			if (!IsSomethingPlaying(data.mhandle))
			{
				data.mhandle = DoSpeech(IDS_TM4_SUB_M04HW17, "M04HW17.wav", 8000, 2000, data.Hawkes, CHAR_HAWKES);
				state = State6;
			}
			break;

		case State6:

			if (!IsSomethingPlaying(data.mhandle))
			{
				data.mhandle = DoSpeech(IDS_TM4_SUB_M04MR03, "M04MR03.wav", 8000, 2000, aRebel, CHAR_KERTAK);
				state = State7;
			}
			break;

		case State7:

			if (!IsSomethingPlaying(data.mhandle))
			{
				data.mhandle = DoSpeech(IDS_TM4_SUB_M04HW18, "M04HW18.wav", 8000, 2000, data.Hawkes, CHAR_HAWKES);
				state = State8;
			}
			break;

		case State8:

			if (!IsSomethingPlaying(data.mhandle))
			{
				data.mhandle = DoSpeech(IDS_TM4_SUB_M04MR19, "M04MR19.wav", 8000, 2000, aRebel, CHAR_KERTAK);
				state = State9;
			}
			break;

		case State9:

			if (!IsSomethingPlaying(data.mhandle))
			{
				data.mhandle = DoSpeech(IDS_TM4_SUB_M04BL20, "M04BL20.wav", 8000, 2000, data.Blackwell, CHAR_BLACKWELL);
				state = State10;
			}
			break;

		case State10:

			if (!IsSomethingPlaying(data.mhandle))
			{
				data.mhandle = DoSpeech(IDS_TM4_SUB_M04HW21, "M04HW21.wav", 8000, 2000, data.Hawkes, CHAR_HAWKES);
				state = State11;
			}
			break;

		case State11:

			if (!IsSomethingPlaying(data.mhandle))
			{
				// Send Fabricator through to Perilon System
				MPartRef fab = FindFirstObjectOfType(M_FABRICATOR, PLAYER_ID, CAPELLA);
				if (fab.isValid())
					M_ OrderMoveTo(fab, M_ GetPartByName("LocFabPerilon"));

				data.mhandle = DoSpeech(IDS_TM4_SUB_M04BL22, "M04BL22.wav", 8000, 2000, data.Blackwell, CHAR_BLACKWELL);
				state = State12;
			}
			break;

		case State12:

			if (!IsSomethingPlaying(data.mhandle))
			{
				data.mhandle = DoSpeech(IDS_TM4_SUB_M04HW23, "M04HW23.wav", 8000, 2000, data.Hawkes, CHAR_HAWKES);
				state = State13;
			}
			break;

		case State13:

			if (!IsSomethingPlaying(data.mhandle))
			{
				data.mhandle = DoSpeech(IDS_TM4_SUB_M04HW24, "M04HW24.wav", 8000, 2000, data.Hawkes, CHAR_HAWKES);
				state = Done;
				//state = State14;
			}
			break;

		/*
		case State14:

			if (!IsSomethingPlaying(data.mhandle))
			{
				data.mhandle = DoSpeech(IDS_TM4_BLACKWELL_M04BL25, "M04BL25.wav", 8000, 2000, data.Blackwell, CHAR_BLACKWELL);

				state = Done;

				// Send Supply ship through to Perilon System
				//MPartRef supply = FindFirstObjectOfType(M_SUPPLY, PLAYER_ID, CAPELLA);
				//M_ OrderMoveTo(supply, M_ GetPartByName("LocFabPerilon"));
			}
			break;
		*/

		case Done:

			if (!M_ IsStreamPlaying(data.mhandle))
			{
				// !! using new objective thing

                AddAndDisplayObjective( IDS_TM4_OBJECTIVE3 );
                AddAndDisplayObjective( IDS_TM4_OBJECTIVE4 );
                AddAndDisplayObjective( IDS_TM4_OBJECTIVE5 );

				//UnderPlayerControl();		//nomissioncontrol:  lets see how long it lasts

				M_ EnableSystem(CAPELLA, true, true);
				M_ EnableJumpgate(M_ GetPartByName("Wormhole to Capella"), true);

				tmp = M_ GetFirstPart();
				while (tmp.isValid())
				{
					if (M_ IsPlatform(tmp) && tmp->playerID == PLAYER_ID && tmp->systemID == CAPELLA)
						M_ EnableSelection(tmp, true);
					tmp = M_ GetNextPart(tmp);
				}

				M_ RunProgramByName("TM4_BuildPlatform", MPartRef ());
				return false;
			}
			break;

	}

	return true;
}

//--------------------------------------------------------------------------//
// Build first platform in Perilon

CQSCRIPTPROGRAM(TM4_BuildPlatform, TM4_BuildPlatform_Data, 0);

void TM4_BuildPlatform::Initialize (U32 eventFlags, const MPartRef & part)
{
	data.mission_state = BuildPlatform;
	timer = 60;  //200

	MPartRef loc;

	loc = M_ GetPartByName("LocPerilonGas1");
	if (!M_ IsDead(data.Rebel1))
		M_ OrderHarvest(data.Rebel1, loc);
	loc = M_ GetPartByName("LocPerilonGas2");
	if (!M_ IsDead(data.Rebel2))
		M_ OrderHarvest(data.Rebel2, loc);
	
	loc = M_ GetPartByName("LocPerilonOre");
	if (!M_ IsDead(data.Rebel3))
		M_ OrderHarvest(data.Rebel3, loc);
	loc = M_ GetPartByName("LocPerilonOre2");
	if (!M_ IsDead(data.Rebel4))
		M_ OrderHarvest(data.Rebel4, loc);

	//M_ RunProgramByName("TM4_RebelsHarvest", MPartRef ());

}

bool TM4_BuildPlatform::Update (void)
{
	if (data.mission_over)
		return false;

	if (timer <= 0 || data.bBuiltPlatform)
	{
		M_ RunProgramByName("TM4_SendBekkaWave", MPartRef ());
		M_ RunProgramByName("TM4_SendLuxorWave", MPartRef ());
		M_ RunProgramByName("TM4_ProtectRebels", MPartRef ());
		return false;
	}

	timer--;
	return true;
}

//--------------------------------------------------------------------------//
// Fakes the rebel harvesting ... TEMPORARY

/*
CQSCRIPTPROGRAM(TM4_RebelsHarvest, TM4_RebelsHarvest_Data, 0);

void TM4_RebelsHarvest::Initialize (U32 eventFlags, const MPartRef & part)
{
	bReb1Out = bReb2Out = bReb3Out = true;
	fakegas = 0;

	if (!M_ IsDead(data.Rebel1))
		M_ OrderMoveTo(data.Rebel1, M_ GetPartByName("LocPerilonGas1"));
	if (!M_ IsDead(data.Rebel2))
		M_ OrderMoveTo(data.Rebel2, M_ GetPartByName("LocPerilonGas1"));
	if (!M_ IsDead(data.Rebel3))
		M_ OrderMoveTo(data.Rebel3, M_ GetPartByName("LocPerilonGas2"));
}

bool TM4_RebelsHarvest::Update (void)
{
	if (data.mission_over)
		return false;

	if (!M_ IsDead(data.Rebel1) && M_ IsIdle(data.Rebel1))
	{
		bReb1Out = !bReb2Out;
		if (bReb1Out)
		{
			M_ OrderMoveTo(data.Rebel1, M_ GetPartByName("LocPerilonGas1"));
			fakegas += 10;
		}
		else
			M_ OrderMoveTo(data.Rebel1, M_ GetPartByName("LocNearAltoPrime"));
	}
	if (!M_ IsDead(data.Rebel2) && M_ IsIdle(data.Rebel2))
	{
		bReb2Out = !bReb2Out;
		if (bReb2Out)
		{
			M_ OrderMoveTo(data.Rebel2, M_ GetPartByName("LocPerilonGas1"));
			fakegas += 10;
		}
		else
			M_ OrderMoveTo(data.Rebel2, M_ GetPartByName("LocNearAltoPrime"));
	}
	if (!M_ IsDead(data.Rebel3) && M_ IsIdle(data.Rebel3))
	{
		bReb3Out = !bReb3Out;
		if (bReb3Out)
		{
			M_ OrderMoveTo(data.Rebel3, M_ GetPartByName("LocPerilonGas2"));
			fakegas += 10;
		}
		else
			M_ OrderMoveTo(data.Rebel3, M_ GetPartByName("LocNearAltoPrime"));
	}
	
	if (fakegas >= 3000) {
		M_ SetGas(REBEL_ID, 500);
		return false;
	}

	return true;
}
*/

//--------------------------------------------------------------------------//

void SendAttackWave(MPartRef * fleet, U32 numShips)
{
	U32 x;
	MPartRef rebel, rebelgate, *target, targ2;

	rebel = FindFirstObjectOfType(M_SIPHON, REBEL_ID, PERILON);

	if (rebel.isValid())
		target = &rebel;
	else if (data.RebelGate.isValid())
		target = &data.RebelGate;
	else
	{
		targ2 = M_ GetPartByName("Wormhole to Alto");
		for (x=0; x<numShips; x++)
		{
			M_ SetStance(fleet[x], US_ATTACK);
			M_ OrderMoveTo(fleet[x], targ2);
		}
		return;
	}

	M_ MakeAreaVisible(MANTIS_ID, *target, 2);

	for (x=0; x<numShips; x++)
	{
		M_ OrderAttack(fleet[x], *target);
	}
}

//--------------------------------------------------------------------------//
// Triggered when a Terran platform in Perilon is complete... sends 2 scouts every 4 mins

U32 BuildBekkaFleet(MPartRef * fleet)
{
	U32 count=0;

	fleet[0] = FindFirstObjectOfType(M_SCOUTCARRIER, MANTIS_ID, BEKKA);
	if (!fleet[0].isValid()) return count;
	count++;
	fleet[1] = FindNextObjectOfType(M_SCOUTCARRIER, MANTIS_ID, BEKKA, fleet[0]);
	if (!fleet[1].isValid()) return count;
	count++;
	fleet[2] = FindNextObjectOfType(M_SCOUTCARRIER, MANTIS_ID, BEKKA, fleet[1]);
	if (!fleet[2].isValid()) return count;
	count++;

	return count;
}

CQSCRIPTPROGRAM(TM4_SendBekkaWave, TM4_SendBekkaWave_Data, 0);

U32 UPDATES_PER_4MIN=(60*4*4);   

void TM4_SendBekkaWave::Initialize (U32 eventFlags, const MPartRef & part)
{
	timer = UPDATES_PER_4MIN / 4;
}

bool TM4_SendBekkaWave::Update (void)
{
	if (data.mission_over || data.mission_state >= RebelsGoHome)
		return false;

	timer--;

	if (timer <= 0)
	{
		MPartRef fleet[3];
		U32 numInFleet;

		numInFleet = BuildBekkaFleet(fleet);

		if (numInFleet)
			SendAttackWave(fleet, numInFleet);

		timer = UPDATES_PER_4MIN / 4;
	}
	
	return true;
}

//--------------------------------------------------------------------------//
// Triggered when a Terran platform in Perilon is complete... sends 2 frigate every 3 mins
//										                      sends 1 scarab every 6 min

U32 BuildLuxorFleet(MPartRef * fleet, bool bFull)
{
	U32 count=0;

	fleet[count] = FindFirstObjectOfType(M_FRIGATE, MANTIS_ID, LUXOR);
	if (fleet[count].isValid())
	{
		count++;
		fleet[count] = FindNextObjectOfType(M_FRIGATE, MANTIS_ID, LUXOR, fleet[count-1]);
		if (fleet[count].isValid())
		{
			count++;
		}
	}

	if (bFull)
	{
		fleet[count] = FindFirstObjectOfType(M_SCARAB, MANTIS_ID, LUXOR);
		if (fleet[count].isValid())
			count++;
	}

	return count;
}
										
CQSCRIPTPROGRAM(TM4_SendLuxorWave, TM4_SendLuxorWave_Data, 0);

U32 UPDATES_PER_3MIN=(40*4*3);		//sped up artificially  (60->40)

void TM4_SendLuxorWave::Initialize (U32 eventFlags, const MPartRef & part)
{
	timer = UPDATES_PER_3MIN / 4;
	bFullFleet = false;
}

bool TM4_SendLuxorWave::Update (void)
{
	if (data.mission_over || data.mission_state >= RebelsGoHome)
		return false;

	timer--;

	if (timer <= 0)
	{
		MPartRef fleet[3];
		U32 numInFleet;

		numInFleet = BuildLuxorFleet(fleet, bFullFleet);  

		if (numInFleet)
		{
			SendAttackWave(fleet, numInFleet);
			data.numWaves++;
		}

		bFullFleet = !bFullFleet;
		timer = UPDATES_PER_3MIN / 4;
	}

	return true;
}

//--------------------------------------------------------------------------//
// Build up and protect rebels

CQSCRIPTPROGRAM(TM4_ProtectRebels, TM4_ProtectRebels_Data, 0);

void TM4_ProtectRebels::Initialize (U32 eventFlags, const MPartRef & part)
{
	data.mission_state = ProtectRebels;
}

bool TM4_ProtectRebels::Update (void)
{
	if (data.mission_over)
		return false;

	if (!data.bWave && data.numWaves >= 2 && CountObjects(M_SCARAB, MANTIS_ID, PERILON) > 0 &&
		CountObjects(M_BALLISTICS, PLAYER_ID) > 0)
	{
		MPartRef scarab;
		scarab = FindFirstObjectOfType(M_SCARAB, MANTIS_ID, PERILON);
		if (M_ IsVisibleToPlayer(scarab, PLAYER_ID))
		{
			data.mhandle = DoSpeech(IDS_TM4_SUB_M04BL25, "M04BL25.wav", 8000, 2000, data.Blackwell, CHAR_BLACKWELL);
			data.bWave = true;

			// Send Fabricator through to Perilon System
			//MPartRef fab = FindFirstObjectOfType(M_FABRICATOR, PLAYER_ID, CAPELLA);
			//if (fab.isValid())
			//	M_ OrderMoveTo(fab, M_ GetPartByName("LocFabPerilon"));
		}
	}

	U32 rebelgas = M_ GetGas(REBEL_ID);
	if (rebelgas >= 200)	//TODO should raise this when I can regenerate nuggets
	{
		M_ RunProgramByName("TM4_RebelsGoHome", MPartRef ());
		return false;
	}

	return true;
}

//--------------------------------------------------------------------------//
// Send Rebel Mantis all back into Alto

CQSCRIPTPROGRAM(TM4_RebelsGoHome, TM4_RebelsGoHome_Data, 0);

void TM4_RebelsGoHome::Initialize (U32 eventFlags, const MPartRef & part)
{
	data.mission_state = RebelsGoHome;
	state = Begin;

	MPartRef loc;
	loc = M_ GetPartByName("LocNearAltoPrime");
	MassMove(M_SIPHON, REBEL_ID, loc);
}

bool TM4_RebelsGoHome::Update (void)
{
	if (data.mission_over)
		return false;

	switch (state)
	{

		case Begin:

			U32 numRebsHome;
			numRebsHome = CountObjects(M_SIPHON, REBEL_ID, ALTO);
			if (numRebsHome >= (NUM_REBEL_HARVESTERS-data.numRebelsDead))
				state = Done;
			break;

		case Done:

			M_ RunProgramByName("TM4_InvestigateAlto", MPartRef ());
			return false;
			break;
	}
	
	return true;
}

//--------------------------------------------------------------------------//
// Player sends Blackwell or someone to go check out Rebels in Alto


//Create fleet of 4 hives and 5 scouts from bekka, 4 scarabs and 7 frigs from luxor
void MakeFinalFleet(MPartRef * fleet)
{
	MPartRef loc;

	loc = M_ GetPartByName("LocLuxor");
	fleet[0] = M_ CreatePart("GBOAT!!M_Frigate", loc, MANTIS_ID);
	fleet[1] = M_ CreatePart("GBOAT!!M_Frigate", loc, MANTIS_ID);
	fleet[2] = M_ CreatePart("GBOAT!!M_Frigate", loc, MANTIS_ID);
	fleet[3] = M_ CreatePart("GBOAT!!M_Frigate", loc, MANTIS_ID);
	fleet[4] = M_ CreatePart("GBOAT!!M_Scarab", loc, MANTIS_ID);
	fleet[5] = M_ CreatePart("GBOAT!!M_Scarab", loc, MANTIS_ID);
	fleet[6] = M_ CreatePart("GBOAT!!M_Scarab", loc, MANTIS_ID);
	fleet[7] = M_ CreatePart("GBOAT!!M_Scarab", loc, MANTIS_ID);
	fleet[8] = M_ CreatePart("GBOAT!!M_Frigate", loc, MANTIS_ID);
	fleet[9] = M_ CreatePart("GBOAT!!M_Frigate", loc, MANTIS_ID);
	fleet[10] = M_ CreatePart("GBOAT!!M_Frigate", loc, MANTIS_ID);

	loc = M_ GetPartByName("LocBekka");
	fleet[11] = M_ CreatePart("GBOAT!!M_Scout Carrier", loc, MANTIS_ID);
	fleet[12] = M_ CreatePart("GBOAT!!M_Hive Carrier", loc, MANTIS_ID);
	fleet[13] = M_ CreatePart("GBOAT!!M_Hive Carrier", loc, MANTIS_ID);
	fleet[14] = M_ CreatePart("GBOAT!!M_Hive Carrier", loc, MANTIS_ID);
	fleet[15] = M_ CreatePart("GBOAT!!M_Scout Carrier", loc, MANTIS_ID);
	fleet[16] = M_ CreatePart("GBOAT!!M_Scout Carrier", loc, MANTIS_ID);
	fleet[17] = M_ CreatePart("GBOAT!!M_Scout Carrier", loc, MANTIS_ID);
	fleet[18] = M_ CreatePart("GBOAT!!M_Hive Carrier", loc, MANTIS_ID);
	fleet[19] = M_ CreatePart("GBOAT!!M_Scout Carrier", loc, MANTIS_ID);
}

CQSCRIPTPROGRAM(TM4_InvestigateAlto, TM4_InvestigateAlto_Data, 0);

void TM4_InvestigateAlto::Initialize (U32 eventFlags, const MPartRef & part)
{
	data.mission_state = InvestigateAlto;
	state = Begin;
	timer = 60;

	M_ MarkObjectiveCompleted(IDS_TM4_OBJECTIVE3);
	M_ MarkObjectiveCompleted(IDS_TM4_OBJECTIVE4);
	M_ MarkObjectiveCompleted(IDS_TM4_OBJECTIVE5);
	
	M_ DestroyPart(data.RebelGate);
	M_ DestroyPart(M_ GetPartByName("Rebel Collector"));
	
	if (!M_ IsDead(data.Rebel1))
		M_ DestroyPart(data.Rebel1);
	if (!M_ IsDead(data.Rebel2))
		M_ DestroyPart(data.Rebel2);
	if (!M_ IsDead(data.Rebel3))
		M_ DestroyPart(data.Rebel3);
	if (!M_ IsDead(data.Rebel4))
		M_ DestroyPart(data.Rebel4);
	
	M_ EnableJumpgate(M_ GetPartByName("Wormhole to Alto"), true);
	M_ EnableSystem(ALTO, true, true);

	// OKAY, YOU'RE FREE TO GO ABOUT YOUR BUSINESS
	M_ EnableJumpCap(data.Blackwell, true);
	M_ EnableJumpCap(data.Hawkes, true);
}

bool TM4_InvestigateAlto::Update (void)
{
	U32 x;
	MPartRef fleet[20];
    MGroupRef tempGroup;

	if (data.mission_over)
		return false;

	timer--;

	switch (state)
	{

		case Begin:

			if (timer < 0)
			{	
				data.mhandle = DoSpeech(IDS_TM4_SUB_M04HW29, "M04HW29.wav", 8000, 2000, data.Hawkes, CHAR_HAWKES);
				M_ AddToObjectiveList(IDS_TM4_OBJECTIVE7);
				state = NewObj;
			}
			break;

		case NewObj:

			if (!IsSomethingPlaying(data.mhandle))
			{
				TeletypeObjective(IDS_TM4_OBJECTIVE7);
				state = Investigate;
			}
			break;

		case Investigate:

			if (PlayerInSystem(PLAYER_ID, ALTO))
			{
				data.mhandle = DoSpeech(IDS_TM4_SUB_M04BL30, "M04BL30.wav", 8000, 2000, data.Blackwell, CHAR_BLACKWELL);
				M_ MarkObjectiveCompleted(IDS_TM4_OBJECTIVE7);
				//M_ EnableJumpCap(data.Hawkes, true);
				state = Investigate2;
			}
			break;

		case Investigate2:

			if (!IsSomethingPlaying(data.mhandle))
			{
				data.mhandle = DoSpeech(IDS_TM4_SUB_M04HW31, "M04HW31.wav", 8000, 2000, data.Hawkes, CHAR_HAWKES);
				state = Attack;
			}
			break;

		case Attack:

			M_ RunProgramByName("TM4_RetreatToCapella", MPartRef ());

			timer = 0;

			state = Done;

			// fall through

		case Done:

            // send in another attack wave if the player dallies

			if ( timer <= 0 && CountObjects( 0, MANTIS_ID, PERILON ) < 20 )  
            {
			    MakeFinalFleet( fleet );

			    for (x=0; x<20; x++)
			    {
				    M_ SetStance(fleet[x], US_ATTACK);

                    tempGroup += fleet[x];

			    }

                MScript::OrderMoveTo( tempGroup, MScript::GetPartByName( "LocFabPerilon" ) ); 

                timer = 60;
            }

			break;
	}

	return true;
}

//--------------------------------------------------------------------------//
// Player brings Blackwell and Hawkes safely back to Capella

CQSCRIPTPROGRAM(TM4_RetreatToCapella, TM4_RetreatToCapella_Data, 0);

void TM4_RetreatToCapella::Initialize (U32 eventFlags, const MPartRef & part)
{
	data.mission_state = RetreatToCapella;
	state = Begin;
	timer = 20;

	//M_ EnableJumpgate(M_ GetPartByName("Wormhole to Capella"), true);
}

bool TM4_RetreatToCapella::Update (void)
{
	U32 x;
	MPartRef fleet[20];

	if (data.mission_over)
		return false;

	switch (state)
	{

		case Begin:

			//if (timer > 0) { timer--; break; }
			
			/*
			if (CountObjects(M_FRIGATE, MANTIS_ID, PERILON) >= 3 && 
				CountObjects(M_HIVECARRIER, MANTIS_ID, PERILON) >= 1 &&
				CountObjects(M_SCARAB, MANTIS_ID, PERILON) >= 1)
			*/

			if (CountObjects(0, MANTIS_ID, PERILON) >= 10)
			{
				data.mhandle = DoSpeech(IDS_TM4_SUB_M04HW32, "M04HW32.wav", 8000, 2000, data.Hawkes, CHAR_HAWKES);
				state = NewObj;

				M_ AddToObjectiveList(IDS_TM4_OBJECTIVE8);

				M_ PlayMusic( DANGER );
			}
			break;

		case NewObj:

			if (!IsSomethingPlaying(data.mhandle))
			{
				TeletypeObjective(IDS_TM4_OBJECTIVE8);
				state = Retreating;
			}
			break;

		case Retreating:

			if (CountObjects(0, MANTIS_ID, 0) < 8)
			{
				MakeFinalFleet(fleet);
				for (x=0; x<20; x++)
				{
					M_ SetStance(fleet[x], US_ATTACK);
					M_ OrderMoveTo(fleet[x], M_ GetPartByName("LocFabPerilon"));
				}
			}

			if (data.Blackwell->systemID == CAPELLA && data.Hawkes->systemID == CAPELLA)
			{
				//M_ FlushTeletype();
				//data.mhandle = M_ PlayTeletype(IDS_TM4_HALSEY_M04HA03, MOLeft, MOTop, MORight, MOBottom, MOTextColor, 8000, 2000, false);
				state = Done;
			}
			break;

		case Done:
			
			M_ RunProgramByName("TM4_MissionSuccess", MPartRef ());
			return false;
			break;
	}

	return true;
}

//-----------------------------------------------------------------------
// OBJECT DESTRUCTION TRIGGER

CQSCRIPTPROGRAM(TM4_ObjectConstructed, TM4_ObjectConstructed_Data,CQPROGFLAG_OBJECTCONSTRUCTED);

void TM4_ObjectConstructed::Initialize (U32 eventFlags, const MPartRef & part)
{
	if (data.mission_over)
		return;

	if (!data.bBuiltPlatform && part->systemID == PERILON && M_ IsPlatform(part))
		data.bBuiltPlatform = true;

	switch (part->mObjClass)
	{

		case M_ACADEMY:

			if (!data.bAcademy && part->playerID == PLAYER_ID)
				data.bAcademy = true;
			break;

		case M_HEAVYIND:

			if (!data.Blackwell.isValid()) break;

			if (!data.bHeavyInd)
			{
				if (data.bAcademy)
				{
					data.mhandle = DoSpeech(IDS_TM4_SUB_M04BL25B, "M04BL25b.wav", 8000, 2000, data.Blackwell, CHAR_BLACKWELL);
				}
				else
				{
					data.mhandle = DoSpeech(IDS_TM4_SUB_M04BL25A, "M04BL25a.wav", 8000, 2000, data.Blackwell, CHAR_BLACKWELL);
				}
				data.bHeavyInd = true;
			}
			break;

	}
	
}

bool TM4_ObjectConstructed::Update (void)
{
	return false;
}

//-----------------------------------------------------------------------
// OBJECT DESTRUCTION TRIGGER

CQSCRIPTPROGRAM(TM4_ObjectDestroyed, TM4_ObjectDestroyed_Data,CQPROGFLAG_OBJECTDESTROYED);

void TM4_ObjectDestroyed::Initialize (U32 eventFlags, const MPartRef & part)
{
	if (data.mission_over)
		return;

    switch (part->mObjClass)
	{

		case M_SIPHON:

			if (data.mission_state < InvestigateAlto && part->playerID == REBEL_ID)
			{
				data.numRebelsDead++;
				if (data.numRebelsDead >= NUM_REBEL_HARVESTERS)
				{
					M_ RunProgramByName("TM4_RebelsKilled", part);
				}
			}
			break;
		
		case M_CORVETTE:

			if (data.Blackwell.isValid() && SameObj(part, data.Blackwell))
				M_ RunProgramByName("TM4_BlackwellKilled", part);
			break;

		case M_MISSILECRUISER:
			
			if (data.Hawkes.isValid() && SameObj(part, data.Hawkes))
				M_ RunProgramByName("TM4_HawkesKilled", part);
			break;

		case M_JUMPPLAT:

			if (data.mission_state < InvestigateAlto && data.RebelGate.isValid() && SameObj(part, data.RebelGate))
				M_ RunProgramByName("TM4_RebelJGateDestroyed", part);
			break;

	}
	
}

bool TM4_ObjectDestroyed::Update (void)
{
	return false;
}

//-----------------------------------------------------------------------
// UNDER ATTACK EVENT

CQSCRIPTPROGRAM(TM4_UnderAttack, TM4_UnderAttack_Data, CQPROGFLAG_UNDERATTACK);

void TM4_UnderAttack::Initialize (U32 eventFlags, const MPartRef & part)
{
	if (data.mission_over)
		return;

	if (!data.bLuxorAttack && PlayerInSystem(PLAYER_ID, LUXOR).isValid())
	{
		data.bLuxorAttack = true;
		if (data.Blackwell.isValid())
			data.mhandle = DoSpeech(IDS_TM4_SUB_M04BL26, "M04BL26.wav", 8000, 2000, data.Blackwell, CHAR_BLACKWELL);
	}
	if (!data.bBekkaAttack && PlayerInSystem(PLAYER_ID, BEKKA).isValid())
	{
		data.bBekkaAttack = true;
		if (data.Blackwell.isValid())
			data.mhandle = DoSpeech(IDS_TM4_SUB_M04BL27, "M04BL27.wav", 8000, 2000, data.Blackwell, CHAR_BLACKWELL);
	}

}

bool TM4_UnderAttack::Update (void)
{
	return false;
}

//-----------------------------------------------------------------------
// FORBIDDEN JUMP EVENT

CQSCRIPTPROGRAM(TM4_ForbiddenJump, TM4_ForbiddenJump_Data, CQPROGFLAG_FORBIDEN_JUMP);

void TM4_ForbiddenJump::Initialize (U32 eventFlags, const MPartRef & part)
{
	if (data.mission_over)
		return;
	
	MPartRef AltoJump = M_ GetPartByName("Wormhole to Alto");
	MPartRef LuxJump = M_ GetPartByName("Gate10");
	MPartRef BekJump = M_ GetPartByName("Gate15");

	if(part == LuxJump || part == BekJump)
	{
		if ( !MScript::IsStreamPlaying(data.mhandle) && !MScript::IsStreamPlaying(data.shandle) )	//audio
        {
		  data.mhandle = DoSpeech(IDS_TM4_SUB_M01BL16, "M01BL16.wav", 8000, 2000, data.Blackwell, CHAR_BLACKWELL);
        }
	}

	if (SameObj(part, AltoJump))
	{
		if (data.Hawkes.isValid())
        {
            if ( !MScript::IsStreamPlaying(data.mhandle) && !MScript::IsStreamPlaying(data.shandle) )	//audio
            {
			    data.mhandle = DoSpeech(IDS_TM4_SUB_M04HW28, "M04HW28.wav", 8000, 2000, data.Hawkes, CHAR_HAWKES);
            }
        }
	}
}

bool TM4_ForbiddenJump::Update (void)
{
	return false;
}


//---------------------------------------------------------------------------
// MISSION FAILURE PROGRAMS

CQSCRIPTPROGRAM(TM4_BlackwellKilled, TM4_BlackwellKilled_Data, 0);

void TM4_BlackwellKilled::Initialize (U32 eventFlags, const MPartRef & part)
{
	data.mission_over = true;

	UnderMissionControl();	
	M_ EnableMovieMode(true);

	M_ MoveCamera(part, 0, MOVIE_CAMERA_JUMP_TO);

	M_ MarkObjectiveFailed(IDS_TM4_OBJECTIVE6);
	M_ FlushStreams();
}

bool TM4_BlackwellKilled::Update (void)
{
	if (!IsSomethingPlaying(data.mhandle))
	{
		if (data.Hawkes.isValid())
			data.mhandle = DoSpeech(IDS_TM4_SUB_M04HW35, "M04HW35.wav", 8000, 2000, data.Hawkes, CHAR_HAWKES);
		data.failureID = IDS_TM4_FAIL_BLACKWELL_LOST;
		M_ RunProgramByName("TM4_MissionFailure", MPartRef ());
	
		return false;
	}

	return true;
}

//---------------------------------------------------------------------------

CQSCRIPTPROGRAM(TM4_HawkesKilled, TM4_HawkesKilled_Data, 0);

void TM4_HawkesKilled::Initialize (U32 eventFlags, const MPartRef & part)
{
	data.mission_over = true;

	M_ MoveCamera(part, 0, MOVIE_CAMERA_JUMP_TO);

	UnderMissionControl();	
	M_ EnableMovieMode(true);

	M_ MarkObjectiveFailed(IDS_TM4_OBJECTIVE6);
	M_ FlushStreams();
}

bool TM4_HawkesKilled::Update (void)
{
	if (!IsSomethingPlaying(data.mhandle))
	{
		if (data.Blackwell.isValid())
			data.mhandle = DoSpeech(IDS_TM4_SUB_M04BL34, "M04BL34.wav", 8000, 2000, data.Blackwell, CHAR_BLACKWELL);
		data.failureID = IDS_TM4_FAIL_HAWKS_LOST;
		M_ RunProgramByName("TM4_MissionFailure", MPartRef ());
	
		return false;
	}

	return true;
}

//---------------------------------------------------------------------------

CQSCRIPTPROGRAM(TM4_RebelsKilled, TM4_RebelsKilled_Data, 0);

void TM4_RebelsKilled::Initialize (U32 eventFlags, const MPartRef & part)
{
	data.mission_over = true;

	M_ MoveCamera(part, 0, MOVIE_CAMERA_JUMP_TO);

	UnderMissionControl();	
	M_ EnableMovieMode(true);

	if (data.mission_state < BuildPlatform)
		M_ MarkObjectiveFailed(IDS_TM4_OBJECTIVE2);
	else
		M_ MarkObjectiveFailed(IDS_TM4_OBJECTIVE4);

	M_ FlushStreams();
}

bool TM4_RebelsKilled::Update (void)
{
	if (!IsSomethingPlaying(data.mhandle))
	{
		if (data.Hawkes.isValid())
			data.mhandle = DoSpeech(IDS_TM4_SUB_M04HW33, "M04HW33.wav", 8000, 2000, data.Hawkes, CHAR_HAWKES);
		M_ RunProgramByName("TM4_MissionFailure", MPartRef ());
	
		return false;
	}

	return true;
}

//---------------------------------------------------------------------------

CQSCRIPTPROGRAM(TM4_RebelJGateDestroyed, TM4_RebelJGateDestroyed_Data, 0);

void TM4_RebelJGateDestroyed::Initialize (U32 eventFlags, const MPartRef & part)
{
	data.mission_over = true;

	M_ MoveCamera(part, 0, MOVIE_CAMERA_JUMP_TO);

	UnderMissionControl();	
	M_ EnableMovieMode(true);

	M_ MarkObjectiveFailed(IDS_TM4_OBJECTIVE5);
	M_ FlushStreams();
}

bool TM4_RebelJGateDestroyed::Update (void)
{
	if (!IsSomethingPlaying(data.mhandle))
	{
		if (data.Hawkes.isValid())
			data.mhandle = DoSpeech(IDS_TM4_SUB_M04HW36, "M04HW36.wav", 8000, 2000, data.Hawkes, CHAR_HAWKES);
		M_ RunProgramByName("TM4_MissionFailure", MPartRef ());
		return false;
	}

	return true;
}

//---------------------------------------------------------------------------

#define TELETYPE_TIME 3000

CQSCRIPTPROGRAM(TM4_MissionFailure, TM4_MissionFailure_Data, 0);

void TM4_MissionFailure::Initialize (U32 eventFlags, const MPartRef & part)
{
	data.mission_over = true;
	state = Begin;
}

bool TM4_MissionFailure::Update (void)
{
	switch (state)
	{
		case Begin:
			if (!IsSomethingPlaying(data.mhandle))
			{
				data.mhandle = DoSpeech(IDS_TM4_SUB_M04HA37, "M04HA37.wav", 8000, 2000, MPartRef(), CHAR_HALSEY);			
				state = Teletype;
			}
			break;

		case Teletype:
			if (!IsSomethingPlaying(data.mhandle))
			{
				M_ FlushTeletype();
				data.mhandle = TeletypeMissionOver(IDS_TM4_MISSION_FAILURE,data.failureID);
				//data.mhandle = M_ PlayTeletype(IDS_TM4_MISSION_FAILURE, SuperLeft, SuperTop, SuperRight, SuperBottom, MOTextColor, SuperHoldTime, TELETYPE_TIME, false);
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

CQSCRIPTPROGRAM(TM4_MissionSuccess, TM4_MissionSuccess_Data, 0);

void TM4_MissionSuccess::Initialize (U32 eventFlags, const MPartRef & part)
{
    if ( data.Blackwell.isValid() )
    {
        M_ MoveCamera( data.Blackwell, 0, MOVIE_CAMERA_JUMP_TO );
    }

	UnderMissionControl();	
	M_ EnableMovieMode(true);

	data.mission_over = true;
	state = Begin;

	M_ MarkObjectiveCompleted(IDS_TM4_OBJECTIVE8);
}

bool TM4_MissionSuccess::Update (void)
{
	switch (state)
	{
		case Begin:
			if (!IsSomethingPlaying(data.mhandle))
			{
				data.mhandle = DoSpeech(IDS_TM4_SUB_M04HA38, "M04HA38.wav", 8000, 2000, MPartRef(), CHAR_HALSEY);
				state = Teletype;
			}
			break;

		case Teletype:
			if (!IsSomethingPlaying(data.mhandle))
			{
				M_ FlushTeletype();
				data.mhandle = TeletypeMissionOver(IDS_TM4_MISSION_SUCCESS);
				//data.mhandle = M_ PlayTeletype(IDS_TM4_MISSION_SUCCESS, SuperLeft, SuperTop, SuperRight, SuperBottom, MOTextColor, SuperHoldTime, 1200, false);
				state = Done;
			}
			break;

		case Done:
			if (!M_ IsTeletypePlaying(data.mhandle))
			{
				UnderPlayerControl();
				M_ EndMissionVictory(3);
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

    MScript::RunProgramByName( "TM4_DisplayObjectiveHandler", MPartRef ());
}

CQSCRIPTPROGRAM( TM4_DisplayObjectiveHandler, TM4_DisplayObjectiveHandlerData, 0 );

void TM4_DisplayObjectiveHandler::Initialize
(
	U32 eventFlags, 
	const MPartRef &part 
)
{
    stringID = data.displayObjectiveHandlerParams_stringID;
    dependantHandle = data.displayObjectiveHandlerParams_dependantHandle;
}

bool TM4_DisplayObjectiveHandler::Update
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
//--------------------------------End Script04T.cpp-------------------------//
//--------------------------------------------------------------------------//
