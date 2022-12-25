#ifndef DMAPGEN_H
#define DMAPGEN_H
//--------------------------------------------------------------------------//
//                                                                          //
//                              DMapGen.h                                    //
//                                                                          //
//                  COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/DInclude/DMapGen.h 11    6/28/01 2:28p Tmauer $
*/			    
//------------------------------- #INCLUDES --------------------------------//
//--------------------------------------------------------------------------//

namespace MAP_GEN_ENUM
{
	enum DMAP_FUNC
	{
		LINEAR = 0,
		LESS_IS_LIKLY,
		MORE_IS_LIKLY
	};

	enum PLACEMENT
	{
		RANDOM = 0,
		CLUSTER,
		PLANET_RING,
		STREEKS,
		SPOTS,
	};

	enum OVERLAP
	{
		NO_OVERLAP = 0,
		LEVEL1,//can overlap another LEVEL1 or LEVEL2
		LEVEL2//may be overlaped by a level1
	};

	enum SECTOR_SIZE
	{
		SMALL_SIZE = 0x01,
		MEDIUM_SIZE = 0x02,
		LARGE_SIZE = 0x04,
		S_M_SIZE = 0x03,
		S_L_SIZE = 0x05,
		M_L_SIZE = 0x06,
		ALL_SIZE = 0x07
	};

	enum SECTOR_FORMATION
	{
		SF_RANDOM,
		SF_RING,
		SF_DOUBLERING,
		SF_STAR,
		SF_INRING,
		SF_MULTI_RANDOM
	};

	enum MACRO_OPERATION
	{
		MC_PLACE_HABITABLE_PLANET,
		MC_PLACE_GAS_PLANET,
		MC_PLACE_METAL_PLANET,
		MC_PLACE_OTHER_PLANET,
		MC_PLACE_TERRAIN,
		MC_PLACE_PLAYER_BOMB,
		MC_MARK_RING
	};
};

#define MAX_TERRAIN 20
#define MAX_THEMES 30
#define MAX_TYPES 6
#define MAX_MACROS 15

struct BT_MAP_GEN
{
	struct _terrainTheme
	{
		char systemKit[MAX_TYPES][GT_PATH];

		char metalPlanets[MAX_TYPES][GT_PATH];
		char gasPlanets[MAX_TYPES][GT_PATH];
		char habitablePlanets[MAX_TYPES][GT_PATH];
		char otherPlanets[MAX_TYPES][GT_PATH];

		char moonTypes[MAX_TYPES][GT_PATH];

		MAP_GEN_ENUM::SECTOR_SIZE sizeOk;//dependant on size setting
		U32 minSize;
		U32 maxSize;
		MAP_GEN_ENUM::DMAP_FUNC sizeFunc;

		U32 numHabitablePlanets[3];//dependant on resource setting
		U32 numMetalPlanets[3];//dependant on resource setting
		U32 numGasPlanets[3];//dependant on resource setting
		U32 numOtherPlanets[3];//dependant on resource setting

		U32 minMoonsPerPlanet;
		U32 maxMoonsPerPlanet;
		MAP_GEN_ENUM::DMAP_FUNC moonNumberFunc;

		U32 numNuggetPatchesMetal[3];//dependant on resource setting
		U32 numNuggetPatchesGas[3];//dependant on resource setting

		struct _terrainInfo
		{
			char terrainArchType[GT_PATH];
			SINGLE probability;
			U32 minToPlace;
			U32 maxToPlace;
			MAP_GEN_ENUM::DMAP_FUNC numberFunc;
			U32 size;
			U32 requiredToPlace;
			MAP_GEN_ENUM::OVERLAP overlap;
			MAP_GEN_ENUM::PLACEMENT placement;
		}terrain[MAX_TERRAIN],nuggetMetalTypes[MAX_TYPES],nuggetGasTypes[MAX_TYPES];
		bool okForPlayerStart:1;
		bool okForRemoteSystem:1;
		SINGLE desitiy[3];//dependant on terrain setting

		struct _macros
		{
			MAP_GEN_ENUM::MACRO_OPERATION operation;
			U32 range;
			bool active;
			union _info
			{
				_terrainInfo terrainInfo;
				MAP_GEN_ENUM::OVERLAP overlap;
			}info;
		}macros[MAX_MACROS];
	}themes[MAX_THEMES];
	
};

//---------------------------------------------------------------------------
//-------------------------END DMapGen.h--------------------------------------
//---------------------------------------------------------------------------
#endif
