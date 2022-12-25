#ifndef DATADEF_H
#define DATADEF_H
//--------------------------------------------------------------------------//
//                                                                          //
//                                DataDef.h                                 //
//				/Conquest/App/Src/Scripts/Script15T/DataDef.h				//
//					MISSION SPECIFIC DATA DEFINITIONS						//
//                                                                          //
//               COPYRIGHT (C) 2000 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//


#ifndef US_TYPEDEFS
#include <typedefs.h>
#endif

#ifndef MPARTREF_H
#include <MPartRef.h>
#endif

#ifndef DMTECHNODE_H
#include <..\..\dinclude\DMTechNode.h>
#endif


//--------------------------------------------------------------------------
//  DEFINES
//--------------------------------------------------------------------------

#define PLAYER_ID			1
#define MANTIS_ID			2
#define SOLARIAN_ID			3
#define REBEL_ID			4

#define ALTO				1
#define LAGO				2
#define MOG					3
#define TARE				4
#define OCTARIUS			5
#define VORAAK				6
#define IODE				7
#define GAAR				8
#define GIGA				9

//--------------------------------------------------------------------------//
//  MISSION DATA
//--------------------------------------------------------------------------//

enum MissionState
{
	Begin,
    Briefing,
	MissionStart,
	Done
};
	
struct MissionData
{
	TECHNODE missionTech;

	MissionState mission_state;
	
	bool mission_over;
	bool LagoIsDefended, MantisLastStand, LagoHasAttacked, OctariusHasAttacked, GigaHasAttacked, TareHasAttacked, GaarHasAttacked, VoraakHasAttacked, IodeHasAttacked;
	bool MogFlag, TareFlag, GigaFlag, GaarFlag, IodeFlag, OctariusFlag, VoraakFlag, ShipSeen;
	bool HalseyDead, HasBeenAttacked;

	U32 mhandle, shandle;

	U32 OctariusTargetArray[2][2], GigaTargetArray[2][2], GaarTargetArray[2][2], VoraakTargetArray[2][2], TareTargetArray[2][2], IodeTargetArray[2][2];
	MPartRef OctariusTargetPlanet[2], GigaTargetPlanet[2], GaarTargetPlanet[2], VoraakTargetPlanet[2], TareTargetPlanet[2], IodeTargetPlanet[2];
	MPartRef ExitPlanetArray[5];

	MPartRef Steele, Halsey, Hawkes, Benson, Vivac, Takei, Blackwell, SuperShip, PlanetTrigger, E1, E2;

	U32 failureID;

};

//--------------------------------------------------------------------------//
//  PROGRAM DATA
//--------------------------------------------------------------------------//

struct TM16_AIProgram_Save
{
	SINGLE timer;

	MPartRef A1, A3;
	float A1Crew, A3Crew;
};

//--------------------------------------------------------------------------//
//
struct TM16_Briefing_Save
{

	enum
	{
		Begin,
		Radio,
		Uplink,
		Fuzz,
		Radio2,
		MO,
		Finish
	} state;
};

//--------------------------------------------------------------------------//
//
struct TM16_MissionStart_Save
{
	SINGLE timer;

	enum
	{
		Begin,
		Done
	} state;

};

//--------------------------------------------------------------------------//
//
struct TM16_ProgressCheck_Save 
{ 
	bool AITrigger1, AITrigger2, AITrigger3, PlayerInVoraak;
};

//--------------------------------------------------------------------------//
struct TM16_HalseyReturn_Save 
{ 
	MPartRef Return;
};

//--------------------------------------------------------------------------//
//
struct TM16_SuperShipSeen_Save 
{ 
	SINGLE timer;
	MPartRef VoraakPrime, target;

	enum
	{
		Begin,
		Blackwell,
		Hawkes,
		Malkor,
		Halsey,
		Malkor2,
		Seconds,
		Halsey2,
		Vivac,
		Benson,
		Vivac2,
		Ion,
		Done
	}state;
};

struct TM16_KeepSuperShipAlive_Save 
{ 
	SINGLE timer, wavetimer;
	bool FabInTown, LeavingTown;
};

struct TM16_LeaveVoraak_Save
{
	int TargetSystem, TargetHolder;
	MPartRef GaarWorm, GigaWorm, IodeWorm, TareWorm, OctariusWorm;
	MPartRef JumpGate;

	SINGLE timer;
};

//--------------------------------------------------------------------------//
//
struct TM16_AttackTheCannon_Save 
{ 
	MPartRef target;

};

//--------------------------------------------------------------------------//
//
struct TM16_PlayerInAlto_Save 
{ 
	enum
	{
		Begin,
		Next,
		Done
	}state;
};

//--------------------------------------------------------------------------//
// Attack Waves
struct TM16_LagoAttack_Save 
{ 
	SINGLE timer;
	
	MPartRef Spawn, MogPrime;
	int Target;
};

struct TM16_OctariusAttack_Save 
{ 
	SINGLE timer;
	
	MPartRef Spawn;
	int Target;
};

struct TM16_GigaAttack_Save 
{ 
	SINGLE timer;

	MPartRef Spawn;
	int Target;
};

struct TM16_TareAttack_Save 
{ 
	SINGLE timer; 

	MPartRef Spawn, GigaPrime;
	int Target;
};

struct TM16_GaarAttack_Save 
{ 
	SINGLE timer;

	MPartRef Spawn;
	int Target;
};

struct TM16_VoraakAttack_Save 
{ 
	SINGLE timer;

	MPartRef Spawn;
	int Target;
};

struct TM16_IodeAttack_Save 
{ 
	SINGLE timer; 

	MPartRef Spawn, GaarPrime;
	int Target;
};

struct TM16_VoraakAttackedByPlayer_Save 
{ 
	SINGLE timer; 
	bool Attacked;

	MPartRef VoraakPrime;
	int Target;
};

//--------------------------------------------------------------------------//
//Mission Events

struct TM16_ObjectDestroyed_Save { };
struct TM16_UnderAttack_Save { };
struct TM16_ObjectConstructed_Save { };
struct TM16_Hit_Save { };
//--------------------------------------------------------------------------//
//Mission Failures

struct TM16_MissionFailure_Save
{
	enum
	{
		Uplink,
		Halsey,
		Fail,
		Done
	} state;
};

//--------------------------------------------------------------------------//
//Mission Success

struct TM16_MissionSuccess_Save
{
	enum
	{
		Uplink,
		Steele,
		Benson,
		Halsey,
		Success,
		Done
	} state;
};

//--------------------------------------------------------------------------//
//--------------------------------End DataDef.h-----------------------------//
//--------------------------------------------------------------------------//
#endif
