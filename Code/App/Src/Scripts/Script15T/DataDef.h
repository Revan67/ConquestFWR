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

//--------------------------------------------------------------------------
//  DEFINES
//--------------------------------------------------------------------------

#define PLAYER_ID			1
#define MANTIS_ID			2
#define SOLARIAN_ID			3
#define REBEL_ID			4

#define NACH				1
#define BEATUS				2
#define AMPERE				3
#define JOVIAN				4
#define VOX					5
#define DECI				6
#define ALTO				7
#define USTED				8
#define MALOTI				9
#define CENTEL				10
#define	FARAD				11
#define	GELD				12

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
	MissionState mission_state;
	
	bool mission_over, AltoFlag, CentelFlag;
	bool MantisLastStand, BeatusHasAttacked, JovianHasAttacked, AmpereHasAttacked, VoxHasAttacked, UstedHasAttacked, MalotiHasAttacked, CentelHasAttacked, AltoHasAttacked, FaradHasAttacked, DeciHasAttacked;
	bool HalseyDead;

	U32 mhandle, shandle;

	U32 JovianTargetArray[2][2], AmpereTargetArray[2][2], VoxTargetArray[4][2], MalotiTargetArray[2][2], CentelTargetArray[3][2], AltoTargetArray[2][2], FaradTargetArray[2][2], DeciTargetArray[2][2];
	MPartRef JovianTargetPlanet[2], AmpereTargetPlanet[2], VoxTargetPlanet[4], MalotiTargetPlanet[2], CentelTargetPlanet[3], AltoTargetPlanet[2], FaradTargetPlanet[2], DeciTargetPlanet[2];

	MPartRef Steele, Halsey, Hawkes, Benson, Vivac, Takei, Smirnoff, E1, E2;

	U32 failureID;
};

//--------------------------------------------------------------------------//
//  PROGRAM DATA
//--------------------------------------------------------------------------//

struct TM15_AIProgram_Save
{
	SINGLE timer;

	MPartRef G1, G2;
	float G1Crew, G2Crew;
};

//--------------------------------------------------------------------------//
//
struct TM15_Briefing_Save
{

	enum
	{
		Begin,
		Jill,
		Uplink,
		Fuzz,
		Halsey,
		KerTak,
		Steele,
		KerTak2,
		Halsey2,
		MO,
		Finish
	} state;
};

//--------------------------------------------------------------------------//
//
struct TM15_MissionStart_Save
{
	enum
	{
		Begin,
		Takei,
		End
	} state;

};

//--------------------------------------------------------------------------//
struct TM15_HalseyReturn_Save 
{ 
	MPartRef Return;
};

//--------------------------------------------------------------------------//
//
struct TM15_ProgressCheck_Save 
{ 
	bool AITrigger1, AITrigger2, AITrigger3, PlayerInAlto;
};

//--------------------------------------------------------------------------//
//
struct TM15_PlayerInAlto_Save 
{ 
	enum
	{
		Begin,
		Next,
		Benson,
		Steele,
		Done
	}state;
};

//--------------------------------------------------------------------------//
// Attack Waves
struct TM15_BeatusAttack_Save 
{ 
	SINGLE timer;
	
	MPartRef Spawn, NachPrime;
	int Target;
};

struct TM15_JovianAttack_Save 
{ 
	SINGLE timer;
	
	MPartRef Spawn;
	int Target;
};

struct TM15_AmpereAttack_Save 
{ 
	SINGLE timer;

	MPartRef Spawn;
	int Target;
};

struct TM15_VoxAttack_Save 
{ 
	SINGLE timer; 

	MPartRef Spawn;
	int Target;
};

struct TM15_UstedAttack_Save 
{ 
	SINGLE timer; 

	MPartRef Spawn, JovianPrime;
	int Target;
};

struct TM15_MalotiAttack_Save 
{ 
	SINGLE timer;

	MPartRef Spawn;
	int Target;
};

struct TM15_CentelAttack_Save 
{ 
	SINGLE timer; 

	MPartRef Spawn;
	int Target;
};

struct TM15_AltoAttack_Save 
{ 
	SINGLE timer;

	MPartRef Spawn;
	int Target;
};

struct TM15_FaradAttack_Save 
{ 
	SINGLE timer; 

	MPartRef Spawn;
	int Target;
};

struct TM15_DeciAttack_Save 
{ 
	SINGLE timer; 

	MPartRef Spawn;
	int Target;
};

//--------------------------------------------------------------------------//
//Mission Events

struct TM15_ObjectDestroyed_Save { };
struct TM15_UnderAttack_Save { };
struct TM15_ObjectConstructed_Save { };

//--------------------------------------------------------------------------//
//Mission Failures

struct TM15_MissionFailure_Save
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

struct TM15_MissionSuccess_Save
{
	enum
	{
		Uplink,
		Halsey,
		Success,
		Done
	} state;
};

//--------------------------------------------------------------------------//
//--------------------------------End DataDef.h-----------------------------//
//--------------------------------------------------------------------------//
#endif
