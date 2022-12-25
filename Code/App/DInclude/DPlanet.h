#ifndef DPLANET_H
#define DPLANET_H
//--------------------------------------------------------------------------//
//                                                                          //
//                                DPlanet.h                                 //
//                                                                          //
//                  COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/DInclude/DPlanet.h 27    6/24/00 12:22p Jasony $
*/			    
//--------------------------------------------------------------------------//
#ifndef DBASEDATA_H
#include "DBaseData.h"
#endif

#ifndef DTYPES_H
#include "DTypes.h"
#endif

#ifndef DINSTANCE_H
#include "DInstance.h"
#endif

#ifndef DMBASEDATA_H
#include "DMBaseData.h"
#endif

struct BT_PLANET_DATA : BASIC_DATA
{
    char fileName[GT_PATH];
	char ambient_animation[GT_PATH];
	char sysMapIcon[GT_PATH];
	char ambientEffect[GT_PATH];
	MISSION_DATA missionData;
	U16 maxMetal;
	U16 maxGas;
	U16 maxCrew;
	SINGLE metalRegen;
	SINGLE gasRegen;
	SINGLE crewRegen;
	enum _planetType
	{
		M_CLASS,
		METAL_PLANET,
		GAS_PLANET,
		OTHER_PLANET
	}planetType;
	char teraParticle[GT_PATH];
	struct TeraColor
	{
		U8 red;
		U8 green;
		U8 blue;
	}teraColor;
	char teraExplosions[GT_PATH];
	struct Halo
	{
		U8 red;
		U8 green;
		U8 blue;
		SINGLE sizeInner;
		SINGLE sizeOuter;
	}halo;

	bool bMoon:1;//if this is a moon planet then most of this data is useless, should this be made a separate class??
	bool bUncommon:1;//if this planet type is an uncommon thing like a Pirate Planet, Dead Planet, or a special planet like Celaron home planet
};

//----------------------------------------------------------------
//
#define M_MAX_SLOTS				12

#define MAX_PLAYERS 8

//----------------------------------------------------------------
//
struct BASE_PLANET_SAVELOAD
{
	U32			slotUser[M_MAX_SLOTS];
	U16 slotMarks[MAX_PLAYERS];
	U16 mySlotMarks[MAX_PLAYERS];
	U16 trueMarks[MAX_PLAYERS];
	U8 shadowVisibilityFlags;
	U16 shadowMetal[MAX_PLAYERS];
	U16 shadowCrew[MAX_PLAYERS];
	U16 shadowGas[MAX_PLAYERS];

	SINGLE playerMetalRate[MAX_PLAYERS];
	SINGLE playerGasRate[MAX_PLAYERS];
	SINGLE playerCrewRate[MAX_PLAYERS];

	S32 maxMetal,
		maxGas,
		maxCrew;
	S32 metal,
		gas,
		crew;
	SINGLE metalRegen,
		gasRegen,
		crewRegen;
	SINGLE  genCrew,
			genGas,
			genMetal;
	SINGLE oreBoost;
	SINGLE gasBoost;
	SINGLE crewBoost;
};
//----------------------------------------------------------------
//
struct PLANET_SAVELOAD : BASE_PLANET_SAVELOAD
{
	// physics data
	TRANS_SAVELOAD trans_SL;

	// hard fog visibility
	U8  exploredFlags;
	
	// mission data
	MISSION_SAVELOAD mission;
};

//----------------------------------------------------------------
//

struct PLANET_VIEW
{
	MISSION_SAVELOAD * mission;
	BASIC_INSTANCE *	rtData;
	U16 maxMetal;
	U16 maxGas;
	U16 maxCrew;
	U16 metal,
		gas,
		crew;
	SINGLE metalRegen;
	SINGLE gasRegen;
	SINGLE crewRegen;

};

//----------------------------------------------------------------
//

#endif
