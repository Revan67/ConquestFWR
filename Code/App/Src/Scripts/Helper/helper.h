#ifndef HELPER_H
#define HELPER_H
//--------------------------------------------------------------------------//
//                                                                          //
//                                 Helper.h                                 //
//                                                                          //
//               COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
   $Header: /Conquest/App/Src/Scripts/Helper/helper.h 88    7/06/01 11:07a Tmauer $

*/
//--------------------------------------------------------------------------//


#define	M_ MScript::

#ifndef _MAGPIE_SPEECH
#ifndef _TELETYPE_SPEECH
#ifndef _AUDIO_SPEECH

// Defining the following to make sure that we don't end up with no speech whatsoever

#define _MAGPIE_SPEECH

#endif
#endif
#endif

// Text types
enum CommType {MO, Audio, TopLeftVid, TopRightVid, BottomLeftVid, Halsey, Blackwell};

// Magpie Position

#define MAIN_MAGPIE_X_POS       550  
#define MAIN_MAGPIE_Y_POS       25

#define MagpieLeft				MAIN_MAGPIE_X_POS
#define MagpieTop				MAIN_MAGPIE_Y_POS

#define MISSION_OVER_TELETYPE_MIN_X_POS 80
#define MISSION_OVER_TELETYPE_MAX_X_POS 560
#define MISSION_OVER_TELETYPE_MIN_Y_POS 300
#define MISSION_OVER_TELETYPE_MAX_Y_POS 330

// Mission objectives
#define MOTextLeft				160
#define MOTextRight				480
#define	MOTextTop				175
#define MOTextBottom			300

// Audio text lines
#define AudioTextLeft			160
#define AudioTextRight			480
#define AudioTextTop			10
#define AudioTextBottom			100

// Top Left video position
#define TopLeftVidLeft			8
#define TopLeftVidTop			8

#define TopLeftTextLeft			66
#define TopLeftTextRight		448
#define TopLeftTextTop			8
#define TopLeftTextBottom		86

// Top Right video position
#define TopRightVidLeft			582
#define TopRightVidTop			8

#define TopRightTextLeft		234
#define TopRightTextRight		574
#define TopRightTextTop			8
#define TopRightTextBottom		86

// Bottom Left video position
#define BottomLeftVidLeft		8
#define BottomLeftVidTop		402

#define BottomLeftTextLeft		66
#define BottomLeftTextRight		448
#define BottomLeftTextTop		402
#define BottomLeftTextBottom	472

#define HalseyVidLeft		0
#define HalseyVidTop		0

#define HalseyTextLeft			150
#define HalseyTextRight			500
#define HalseyTextTop			10
#define HalseyTextBottom		100

#define BlackwellVidLeft	540
#define BlackwellVidTop		16

#define BlackwellTextLeft		100
#define BlackwellTextRight		450
#define BlackwellTextTop		10
#define BlackwellTextBottom		100

#define SuperLeft			80
#define SuperRight			560
#define SuperTop			300
#define SuperBottom			330

#define SuperPrintTime		10000
#define SuperHoldTime		12000

#define STANDARD_TELETYPE_MILS_PER_CHAR     75
#define STANDARD_TELETYPE_COLOR				RGB(63, 199, 208)

#define STANDARD_TELETYPE_HOLD_TIME     10000

#define STANDARD_BRIEFING_TELETYPE_HOLD_TIME    5000

#define NEW_OBJECTIVES_LEFT     25
#define NEW_OBJECTIVES_RIGHT    325
#define NEW_OBJECTIVES_TOP      275
#define NEW_OBJECTIVES_BOTTOM   400

#define NEW_OBJECTIVES_COLOR		RGB( 208, 199, 63 )
#define NEW_OBJECTIVES_MILS_PER_CHAR   75

#define NEW_OBJECTIVES_HOLD_TIME        3000  // intended to be added to the time required to display the string

#define MissionFailureSuperPrintTime    5000
#define MissionFailureSuperHoldTime     12000

//Animate Defines

#define ANIMSPD_RADIOWAVE	2000/30
#define ANIMSPD_BOXWAVE		2000/30
#define ANIMSPD_TECHLOOP	2000/30
#define ANIMSPD_TNRLOGO		2750/30
#define ANIMSPD_FUZZ		2000/30
#define ANIMSPD_XMISSION	2000/30

//Music
#define BATTLE			"battle.wav"
#define BATTLE_LOADING	"battle_loading.wav"
#define DANGER			"danger.wav"
#define DEFEAT			"defeat.wav"
#define DETERMINATION	"determination.wav"
#define ESCORT			"escort.wav"
#define MENU			"main_menu_screen.wav"
#define MANTIS_WAV		"mantisgame14.wav"
#define MULTI_MENU		"multiplayer_menu.wav"
#define MYSTERY			"mystery.wav"
#define NEGOTIATION		"negotiation.wav"
#define CELAREAN_WAV	"solarian_game14.wav"
#define SUSPENSE		"suspense.wav"
#define TERRAN_WAV		"terrangame14.wav"
#define VICTORY			"victory.wav"

//Colors
#define GreenColor				RGB(64, 200, 64)
#define OrangeColor				RGB(200, 64, 0)
#define RedColor				RGB(200, 0, 0)

#define TalkyTextColor			RGB(64, 64, 200)
#define MOTextColor				RGB(63, 199, 208)
#define PointyLineColor			RGB(200, 64, 64)
#define BriefingMOColor			RGB(63, 127, 175)

// tech tree race bits

#define TECH_TREE_RACE_BITS_CELEREAN  0x20000000
#define TECH_TREE_RACE_BITS_MANTIS    0x10000000

#define TECH_TREE_RACE_BITS_ALL ( TECH_TREE_RACE_BITS_CELEREAN | TECH_TREE_RACE_BITS_MANTIS )


//--------------------------------------------------------------------------//
//MPartRef nopart;
//--------------------------------------------------------------------------//

enum CHAR_TYPE
{
	CHAR_NOCHAR,		//1
	CHAR_BLACKWELL,
	CHAR_HALSEY,
	CHAR_HAWKES,
	CHAR_BENSON,
	CHAR_KERTAK,
	CHAR_TAKEI,
	CHAR_STEELE,
	CHAR_SMIRNOFF,
	CHAR_MALKOR,		//10
	CHAR_GHELEN,
	CHAR_VIVAC,
	CHAR_ELAN,
	CHAR_NATUS,
	CHAR_MARINE,
	CHAR_INTEL			//16
};

#define NUM_CHARS		32

//--------------------------------------------------------------------------//

#define TTLCALC(ttl) (ttl + 3)

U32 CountObjects(S32 type, U32 PlayerID, U32 SystemID = 0);

U32 CountObjectsInSupply(S32 type, U32 PlayerID, U32 SystemID = 0);

U32 CountObjectsInRange(S32 type, U32 PlayerID, MPartRef & loc, S32 range);

U32 CountObjectsInRangeInSupply(S32 type, U32 PlayerID, MPartRef & loc, S32 range);

void MassStance(S32 type, U32 PlayerID, UNIT_STANCE stance, U32 SystemID = 0);

void MassStanceInRange(S32 type, U32 PlayerID, UNIT_STANCE stance, MPartRef & loc, S32 range);

void MassAttack(S32 type, U32 PlayerID, MPartRef & target, U32 SystemID = 0);

void MassAttackInRange(S32 type, U32 PlayerID, MPartRef & target, MPartRef & loc, S32 range);

void MassMove(S32 type, U32 PlayerID, MPartRef & target, U32 SystemID = 0);

void MassMoveInRange(S32 type, U32 PlayerID, MPartRef &target, MPartRef &loc, S32 range);

void MassTeleport(S32 type, U32 PlayerID, MPartRef & target, U32 SystemID = 0);

void MassCancel(S32 type, U32 PlayerID, U32 SystemID = 0);

void MassCancelAll(U32 PlayerID, U32 SystemID = 0);

void MassAttackCap(U32 PlayerID, bool bAttackOk, U32 SystemID = 0);

void MassSwitchSystem(U32 PlayerID, U32 NewID, U32 SystemID);

void MassEnableAI(U32 PlayerID, bool bOn, U32 SystemID = 0);

void MassEnableGunboatAI(U32 PlayerID, bool bOn, U32 SystemID = 0);

void MassEnablePlatformAI(U32 PlayerID, bool bOn, U32 SystemID = 0);

void MassSwitchPlayerIDInRange(S32 type, U32 PlayerID, U32 NewPlayerID, MPartRef & loc, S32 range);

MPartRef FindFirstObjectOfType (S32 type, U32 PlayerID, U32 SystemID);

MPartRef FindFirstPlat (U32 PlayerID, U32 SystemID);

MPartRef FindNextObjectOfType (S32 type, U32 PlayerID, U32 SystemID, MPartRef & prevPart);

MPartRef PlayerInSystem (U32 PlayerID, U32 SystemID);

MPartRef GenAndMove(const char * name, MPartRef & Generator, MPartRef & WayPoint);

MPartRef GenAndAttack(const char * name, MPartRef & Generator);

void AttackNearestEnemy( MPartRef & part);

bool VisibleOrDead( MPartRef & part, U32 PlayerID);

void UnderMissionControl(void);

void UnderPlayerControl(void);

U32 PlayBlackwellMagpie (const char * file, U32 text, MPartRef & speaker, U32 mhandle = 0);

MPartRef GetFirstJumpgate(U32 fromSystemID, U32 toSystemID, U32 playerID = 0); 

MPartRef GetNextJumpgate(U32 fromSystemID, U32 toSystemID, MPartRef & prevgate, U32 playerID = 0);  

void TextOutput(char * buffer);

bool IsSomethingPlaying(U32 handle);

U32 DoSpeech(U32 stringID, char *file = NULL, U32 holdTime = 8000, U32 printTime = 2000, const MPartRef &part = MPartRef(), CHAR_TYPE charNum = CHAR_BLACKWELL);

U32 DoBriefingSpeech(U32 stringID, char *file = NULL, U32 holdTime = 8000, U32 printTime = 2000, U32 slotNum = 0, CHAR_TYPE charNum = CHAR_BLACKWELL, bool bStatic = true);

U32 DoBriefingSpeechMagpie(U32 stringID, char *file = NULL, U32 holdTime = 8000, U32 printTime = 2000, U32 slotNum = 0, CHAR_TYPE charNum = CHAR_BLACKWELL, bool bStatic = true);

U32 DoBriefingSpeechMagpieDirect
( 
	U32 stringID,
    char *file, 
    U32 slotNum,
    CHAR_TYPE charNum,
    bool bStatic = true
);

U32 ShowBriefingAnimation(S32 slotNum, char *archetype, U32 timer = 200, bool bLoop = true, bool bRepeat = true);

U32 ShowBriefingHead(U32 slotNum, CHAR_TYPE charNum);

U32 TeletypeObjective( U32 stringID );

U32 TeletypeMissionOver( U32 stringID , U32 failureID = 0);

U32 TeletypeBriefing( U32 stringID, bool bPerm=false );

#endif //HELPER_H