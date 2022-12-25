//--------------------------------------------------------------------------//
//                                                                          //
//                              MapGen.cpp                                  //
//                                                                          //
//               COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
   $Header: /Conquest/App/Src/MapGen.cpp 69    6/28/01 2:28p Tmauer $
*/
//--------------------------------------------------------------------------//

 
#include "pch.h"
#include <globals.h>
#include <stdio.h>

#include "MapGen.h"
#include "CQGame.h"
#include "ObjList.h"
#include "sector.h"
#include "RuseMap.h"
#include "MGlobals.h"
#include "startup.h"
#include "IObject.h"
#include "DMapGen.h"
#include "MPart.h"
#include "DField.h"
#include "Field.h"
#include "GridVector.h"
#include "ObjMap.h"
#include "DrawAgent.h"
#include "IPlanet.h"

#include <IConnection.h>
#include <TSmartPointer.h>

using namespace CQGAMETYPES;

#define PLANETSIZE (GRIDSIZE*2.0)
#define MAX_MAP_GRID 64
#define MAX_MAP_SIZE (MAX_MAP_GRID*GRIDSIZE)

//random charts

#define RND_MAX_PLAYER_SYSTEMS 8

U32 rndPlayerX[RND_MAX_PLAYER_SYSTEMS] = {0,2,5,8,9,7,4,1};
U32 rndPlayerY[RND_MAX_PLAYER_SYSTEMS] = {4,1,0,2,5,8,9,7};

#define RND_MAX_REMOTE_SYSTEMS 20

U32 rndRemoteX[RND_MAX_REMOTE_SYSTEMS] = {4,6,1,3,5,7,2,4,6,8,1,3,5,7,2,4,6,8,3,5};
U32 rndRemoteY[RND_MAX_REMOTE_SYSTEMS] = {2,2,3,3,3,3,4,4,4,4,5,5,5,5,6,6,6,6,7,7};

#define RING_MAX_SYSTEMS 16

U32 ringSystemX[RING_MAX_SYSTEMS] = {5,6,8,7,9,7,7,5,4,3,1,2,0,2,2,4};
U32 ringSystemY[RING_MAX_SYSTEMS] = {0,2,2,4,5,6,8,7,9,7,7,5,4,3,1,2};

#define STAR_MAX_TREE 8
U32 starCenterX = 4;
U32 starCenterY = 4;
U32 starTreeX[STAR_MAX_TREE][3] = {{4,3,5},{6,7,8},{6,8,8},{6,8,7},{4,5,3},{2,1,0},{2,0,0},{2,0,1}};
U32 starTreeY[STAR_MAX_TREE][3] = {{2,0,0},{2,0,1},{4,3,5},{6,7,8},{6,8,8},{6,8,7},{4,5,3},{2,1,0}};


//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
#ifdef FINAL_RELEASE
#define OPPRINT1(exp,p1) ((void)0)
#else
#define OPPRINT1(exp,p1) FDUMP(ErrorCode(ERR_GENERAL, SEV_TRACE_1), exp, p1)
#endif // !FINAL_RELEASE

//the random numbers caqn be understood to be in fixed 15 floating point.
#define FIX15 15 

static U32 linearFunc()
{
	return rand();
}
//--------------------------------------------------------------------------//
//
static U32 lessIsLikelyFunc()
{
	U32 v = linearFunc();

	v = v*v >> FIX15;
	v = v*v >> FIX15;
	return v;
}
//--------------------------------------------------------------------------//
//
static U32 moreIsLikelyFunc()
{
	U32 v = linearFunc();

	v = v*v >> FIX15;
	v = v*v >> FIX15;

	return 0x00007FFF-v;
}
//--------------------------------------------------------------------------//
//
U32 (*randFunc[])() =
{
	linearFunc,
	lessIsLikelyFunc,
	moreIsLikelyFunc
};

#define GENMAP_TAKEN 1
#define GENMAP_LEVEL1 2
#define GENMAP_LEVEL2 3
#define GENMAP_PATH 4

#define FLAG_PLANET 0x01
#define FLAG_PATHON 0x02

#define MAX_FLAGS 20

struct FlagPost
{
	U8 type;
	U8 xPos;
	U8 yPos;
};

struct GenSystem
{
	FlagPost flags[MAX_FLAGS];
	U32 numFlags;

	U32 sectorGridX,
		sectorGridY,
		index;

	U32 planetNameCount;

	U32 x,
		y, 
		size,
		jumpgateCount;
	U32 distToSystems[MAX_SYSTEMS];
	struct GenJumpgate * jumpgates[MAX_SYSTEMS];
	U32 systemID;
	U32 playerID;
	U32 connectionOrder;
	U32 playerDistToSystems[MAX_PLAYERS][MAX_SYSTEMS];
	BT_MAP_GEN::_terrainTheme * theme;

	U32 omStartEmpty;
	U32 omUsed;
	U8 objectMap[MAX_MAP_GRID][MAX_MAP_GRID];

	void initObjectMap()
	{	
		omUsed = 0;
		omStartEmpty = 0;
		S32 centerDist = size/2;
		S32 centerBoarder = centerDist-1;
		S32 centerBoarder2 = centerBoarder*centerBoarder;
		S32 centerDist2 = centerDist*centerDist;
		for(S32 i = 0; i < (S32)size; ++i)
		{
			for(S32 j = 0; j < (S32)size; ++j)
			{
				S32 dist = (i-centerDist)*(i-centerDist)+(j-centerDist)*(j-centerDist);
				if(dist >= centerDist2)
				{
					objectMap[i][j] = GENMAP_TAKEN;
				}
				else if(dist >= centerBoarder2)
				{
					objectMap[i][j] = GENMAP_LEVEL1;
				}
				else
				{
					++omStartEmpty;
					objectMap[i][j] = 0;
				}
			}
		}
	}
};

struct GenJumpgate
{
	GenSystem * system1;
	GenSystem * system2;
	U32 x1,
		y1,
		x2,
		y2;
	U32 dist;
	bool created:1;
};

struct GenStruct
{
	BT_MAP_GEN * data;
	
	U32 numPlayers;

	U32 sectorSize;
	U32 sectorGrid[17];//17 hight, use a shift to get the width.

	U32 gameSize;
	MAP_GEN_ENUM::SECTOR_FORMATION sectorLayout;

	U32 systemsToMake;

	U8 terrainSize;

	S32 objectBoarder;

	GenSystem systems[MAX_SYSTEMS];
	U32 systemCount;

	GenJumpgate jumpgate[MAX_SYSTEMS*MAX_SYSTEMS];
	U32 numJumpGates;
};

struct DACOM_NO_VTABLE MapGen : public IMapGen
{
	BEGIN_DACOM_MAP_INBOUND(MapGen)
	DACOM_INTERFACE_ENTRY(IMapGen)
	END_DACOM_MAP()

	virtual ~MapGen();

	/* IMapGen methods */
	virtual void GenerateMap (const FULLCQGAME & game, U32 seed, IPANIM * ipAnim);

	virtual U32 GetBestSystemNumber(const FULLCQGAME & game, U32 approxNumber);

	virtual U32 GetPosibleSystemNumbers(const FULLCQGAME & game, U32 * list);

	//map gen stuff

	void initMap (GenStruct & map, const FULLCQGAME & game);

	void insertObject (char * object,Vector position,U32 playerID, U32 systemID, GenSystem * system);

	//Util funcs

	U32 GetRand(U32 min, U32 max, MAP_GEN_ENUM::DMAP_FUNC mapFunc);

	void GenerateSystems(GenStruct & map);
		
		void generateSystemsRandom(GenStruct & map);

		void generateSystemsRing(GenStruct & map);

		void generateSystemsStar(GenStruct & map);

	bool SystemsOverlap(GenStruct & map, GenSystem * system);

	void GetJumpgatePositions(GenStruct & map, GenSystem * sys1, GenSystem * sys2, U32 & jx1, U32 & jy1, U32 & jx2, U32 & jy2);

	bool CrossesAnotherSystem(GenStruct & map, GenSystem * sys1, GenSystem * sys2, U32 jx1, U32 jy1, U32 jx2, U32 jy2);

	bool CrossesAnotherLink(GenStruct & map, GenJumpgate * gate);

	bool LinesCross(S32 line1x1, S32 line1y1, S32 line1x2, S32 line1y2, S32 line2x1, S32 line2y1, S32 line2x2, S32 line2y2);

	void SelectThemes(GenStruct & map);

	void CreateSystems(GenStruct & map);

	void RunHomeMacros(GenStruct & map);

		bool findMacroPosition(GenSystem * system,S32 centerX,S32 centerY,U32 range,U32 size,MAP_GEN_ENUM::OVERLAP overlap,U32 & posX,U32 &posY);

		void getMacroCenterPos(GenSystem * system,U32 & x, U32 & y);

		void createPlanetFromList(S32 xPos, S32 yPos, GenSystem * system, U32 range ,char planetList[][GT_PATH]);

		void placeMacroTerrain(S32 centerX,S32 centerY,GenSystem * system,S32 range,BT_MAP_GEN::_terrainTheme::_terrainInfo * terrainInfo);

	void CreateJumpgates(GenStruct & map);

		void createRandomGates3(GenStruct & map);

			void createGateLevel2(GenStruct & map, U32 totalLevel, U32 levelSystems,U32 targetSystems,U32 gateNum, U32 currentGates[64], U32 score, 
							 U32 * bestGates, U32 &bestScore, U32 & bestGateNum, bool moreAllowed);

		void createRandomGates2(GenStruct & map);

			void createGateLevel(GenStruct & map, U32 totalLevel, U32 levelSystems,U32 targetSystems,U32 gateNum, U32 currentGates[64], U32 score, 
							 U32 * bestGates, U32 &bestScore, U32 & bestGateNum, bool moreAllowed);

			U32 scoreGate(GenStruct & map,U32 gateIndex);

			void markSystems(U32 & systemUnconnected,GenSystem * system, U32 & systemsVisited);

		void createRandomGates(GenStruct & map);

		void createRingGates(GenStruct & map);

		void createStarGates(GenStruct & map);

		void createJumpgatesForIndexs(GenStruct & map,U32 index1, U32 index2);

	void PopulateSystems(GenStruct & map, IPANIM * ipAnim);

	void PopulateSystem(GenStruct & map,GenSystem * system);

		void placePlanetsMoons(GenStruct & map, GenSystem * system,U32 planetPosX,U32 planetPosY);

	bool SpaceEmpty(GenSystem * system,U32 xPos,U32 yPos,MAP_GEN_ENUM::OVERLAP overlap,U32 size);

	void FillPosition(GenSystem * system,U32 xPos, U32 yPos,U32 size,MAP_GEN_ENUM::OVERLAP overlap);

	bool FindPosition(GenSystem * system,U32 width,MAP_GEN_ENUM::OVERLAP overlap, U32 & xPos, U32 & yPos);

//	bool ColisionWithObject(GenObj * obj,Vector vect,U32 rad,MAP_GEN_ENUM::OVERLAP overlap);

	void GenerateTerain(GenStruct & map, GenSystem * system);

	void PlaceTerrain(GenStruct & map,BT_MAP_GEN::_terrainTheme::_terrainInfo terrain,GenSystem *system);

	void PlaceRandomField(BT_MAP_GEN::_terrainTheme::_terrainInfo * terrain,U32 numToPlace,S32 startX, S32 startY,GenSystem * system);

	void PlaceSpottyField(BT_MAP_GEN::_terrainTheme::_terrainInfo * terrain,U32 numToPlace,S32 startX, S32 startY,GenSystem * system);

	void PlaceRingField(BT_MAP_GEN::_terrainTheme::_terrainInfo * terrain,GenSystem *system);

	void PlaceRandomRibbon(BT_MAP_GEN::_terrainTheme::_terrainInfo * terrain,U32 length,S32 startX, S32 startY,GenSystem * system);

	void BuildPaths(GenSystem * system);

	//other
	void init (void);

	void removeFromArray(U32 nx,U32 ny,U32 * tempX,U32 * tempY,U32 & tempIndex);

	bool isInArray(U32 * arrX, U32 * arrY, U32 index, U32 nx, U32 ny);

	bool isOverlapping(U32 * arrX, U32 * arrY, U32 index, U32 nx, U32 ny);

	void checkNewXY(U32 * tempX,U32 * tempY,U32 & tempIndex, U32 * finalX, U32 * finalY, U32 finalIndex, BT_MAP_GEN::_terrainTheme::_terrainInfo terrain,
				GenSystem * system,U32 newX,U32 newY);

	void connectPosts(FlagPost * post1, FlagPost * post2,GenSystem * system);

};
//--------------------------------------------------------------------------//
//
MapGen::~MapGen()
{
}
//--------------------------------------------------------------------------//
//
void MapGen::GenerateMap (const FULLCQGAME & game, U32 seed, IPANIM * ipAnim)
{
	srand(seed);
	OPPRINT1("MAP GENERATION SEED = %d\n",seed);

	//init the map struct to set up the generation
	GenStruct map;

	if (ipAnim)
	{
		ipAnim->UpdateString(IDS_PROG_INITMAP);
		initMap(map,game);
		ipAnim->SetProgress(0.02f);

		ipAnim->UpdateString(IDS_PROG_GENSYSTEMS);
		GenerateSystems(map);
		ipAnim->SetProgress(0.04f);

		ipAnim->UpdateString(IDS_PROG_SELECTTHEME);
		SelectThemes(map);
		ipAnim->SetProgress(0.06f);

		ipAnim->UpdateString(IDS_PROG_CREATESUB);
		CreateSystems(map);

		ipAnim->SetProgress(0.10f);
		RunHomeMacros(map);

		ipAnim->SetProgress(0.15f);

		ipAnim->UpdateString(IDS_PROG_RATESYSTEMS);
		CreateJumpgates(map);

		ipAnim->SetProgress(0.20f);

		ipAnim->UpdateString(IDS_PROG_POPULATE);
		PopulateSystems(map,ipAnim);
	}
	else
	{
		initMap(map,game);
		GenerateSystems(map);
		SelectThemes(map);
		CreateSystems(map);
		RunHomeMacros(map);
		CreateJumpgates(map);

		PopulateSystems(map,ipAnim);
	}
}
//--------------------------------------------------------------------------//
//
U32 MapGen::GetBestSystemNumber(const FULLCQGAME & game, U32 approxNumber)
{
	U32 numPlayers = 0;

	U32 assignments[MAX_PLAYERS+1];
	memset(assignments, 0, sizeof(assignments));
	S32 i;
	for(i = 0; i < S32(game.activeSlots); ++i)
	{
		if ((game.slot[i].state == READY) || (game.slot[i].state == ACTIVE))
			assignments[game.slot[i].color] = 1;
	}

	for (i = 1; i <= MAX_PLAYERS; i++)
		numPlayers += assignments[i];

	if(game.templateType == TEMPLATE_RANDOM || game.templateType == TEMPLATE_NEW_RANDOM)
	{
		return __max(numPlayers,approxNumber);
	}
	else if(game.templateType == TEMPLATE_RING)
	{
		return __max(numPlayers * (approxNumber/numPlayers),numPlayers);
	}
	else if(game.templateType == TEMPLATE_STAR)
	{
		if(numPlayers < 6)
		{
			for(U32 i = 3; i > 0;--i)
			{
				U32 number =1+(i*numPlayers);
				if(number <= approxNumber)
					return number;
			}
			return 1+numPlayers;
		}
		else if(numPlayers < 8)
		{
			for(U32 i = 2; i > 0;--i)
			{
				U32 number =1+(i*numPlayers);
				if(number <= approxNumber)
					return number;
			}
			return 1+numPlayers;
		}
		else
		{
			return 9;
		}
	}

	return approxNumber;
}
//--------------------------------------------------------------------------//
//
U32 MapGen::GetPosibleSystemNumbers(const FULLCQGAME & game, U32 * list)
{
	U32 numPlayers = 0;

	U32 assignments[MAX_PLAYERS+1];
	memset(assignments, 0, sizeof(assignments));
	S32 i;
	for(i = 0; i < S32(game.activeSlots); ++i)
	{
		if ((game.slot[i].state == READY) || (game.slot[i].state == ACTIVE))
			assignments[game.slot[i].color] = 1;
	}

	for (i = 1; i <= MAX_PLAYERS; i++)
		numPlayers += assignments[i];

	if(game.templateType == TEMPLATE_RANDOM || game.templateType == TEMPLATE_NEW_RANDOM)
	{
#ifdef _DEMO_
		U32 count = numPlayers;
		for(i = 0; i < 6;++i)
		{
			list[i] = count;
			++count;
			if(count > 6)
			{
				return i+1;
			}
		}
		return MAX_SYSTEMS;		
#else
		U32 count = numPlayers;
		for(i = 0; i < MAX_SYSTEMS;++i)
		{
			list[i] = count;
			++count;
			if(count > MAX_SYSTEMS)
			{
				return i+1;
			}
		}
		return MAX_SYSTEMS;
#endif
	}
	else if(game.templateType == TEMPLATE_RING)
	{
		for(i = 0; i < MAX_SYSTEMS; ++i)
		{
			list[i] = (i+1)*numPlayers;
			if(list[i] > MAX_SYSTEMS)
			{
				return i;
			}
		}
		return MAX_SYSTEMS;
	}
	else if(game.templateType == TEMPLATE_STAR)
	{
		for(i = 0; i < 3; ++i)
		{
			list[i] = 1+((i+1)*numPlayers);
			if(list[i] > MAX_SYSTEMS)
			{
				return i;
			}
		}
		return 3;		
	}
	return 1;
}
//--------------------------------------------------------------------------//
//
void MapGen::generateSystemsRandom(GenStruct & map)
{
	U32 s1;

	for (s1 = 0; s1 < map.numPlayers; s1++)
	{
		GenSystem * system1 = &(map.systems[s1]);			

		memset(system1, 0, sizeof(GenSystem));

		do
		{
			U32 val =GetRand(0,RND_MAX_PLAYER_SYSTEMS-1 , MAP_GEN_ENUM::LINEAR);
			system1->sectorGridX = rndPlayerX[val];
			system1->sectorGridY = rndPlayerY[val];
			system1->connectionOrder = val;
		} while (SystemsOverlap(map,system1));
		
		map.sectorGrid[system1->sectorGridX] |= (0x00000001 <<system1->sectorGridY); 

		system1->index = s1;
		system1->playerID = s1+1;

		map.systemCount++;
	}
	for (s1 = map.numPlayers; s1 < map.systemsToMake; s1++)
	{
		GenSystem * system1 = &(map.systems[s1]);			

		memset(system1, 0, sizeof(GenSystem));

		do
		{
			U32 val =GetRand(0,RND_MAX_REMOTE_SYSTEMS-1 , MAP_GEN_ENUM::LINEAR);
			system1->sectorGridX = rndRemoteX[val];
			system1->sectorGridY = rndRemoteY[val];
		} while (SystemsOverlap(map,system1));
		
		map.sectorGrid[system1->sectorGridX] |= (0x00000001 <<system1->sectorGridY); 

		system1->index = s1;

		map.systemCount++;
	}
}
//--------------------------------------------------------------------------//
//
void MapGen::generateSystemsStar(GenStruct & map)
{
	//create center system

	GenSystem * system1 = &(map.systems[0]);			

	memset(system1, 0, sizeof(GenSystem));

	system1->sectorGridX = starCenterX;
	system1->sectorGridY = starCenterY;
	
	map.sectorGrid[system1->sectorGridX] |= (0x00000001 <<system1->sectorGridY); 

	system1->index = 0;

	map.systemCount++;

	//create trees
	U32 systemsPerPlayer = (map.systemsToMake-1)/map.numPlayers;
	U32 treeUsed = 0;
	for(U32 i = 0; i < map.numPlayers; ++i)
	{
		U32 tree;
		do
		{
			tree = GetRand(0,STAR_MAX_TREE-1,MAP_GEN_ENUM::LINEAR);
		}while((0x01 << tree)&treeUsed);
		treeUsed |= (0x01 << tree);

		//create home system
		system1 = &(map.systems[map.systemCount]);			

		memset(system1, 0, sizeof(GenSystem));

		system1->sectorGridX = starTreeX[tree][0];
		system1->sectorGridY = starTreeY[tree][0];
		
		map.sectorGrid[system1->sectorGridX] |= (0x00000001 <<system1->sectorGridY); 

		system1->index = map.systemCount;
		system1->playerID = i+1;

		map.systemCount++;

		//create leaf systems
		bool createSys1 = false;
		bool createSys2 = false;
		if(systemsPerPlayer == 2)
		{
			if(GetRand(1,10,MAP_GEN_ENUM::LINEAR) > 5)
				createSys1 = true;
			else
				createSys2 = true;
		}else if(systemsPerPlayer == 3)
		{
			createSys1 = true;
			createSys2 = true;
		}
		if(createSys1)
		{
			system1 = &(map.systems[map.systemCount]);			

			memset(system1, 0, sizeof(GenSystem));

			system1->sectorGridX = starTreeX[tree][1];
			system1->sectorGridY = starTreeY[tree][1];
			
			map.sectorGrid[system1->sectorGridX] |= (0x00000001 <<system1->sectorGridY); 

			system1->index = map.systemCount;

			map.systemCount++;
		}
		if(createSys2)
		{
			system1 = &(map.systems[map.systemCount]);			

			memset(system1, 0, sizeof(GenSystem));

			system1->sectorGridX = starTreeX[tree][2];
			system1->sectorGridY = starTreeY[tree][2];
			
			map.sectorGrid[system1->sectorGridX] |= (0x00000001 <<system1->sectorGridY); 

			system1->index = map.systemCount;

			map.systemCount++;
		}
	}
}
//--------------------------------------------------------------------------//
//
void MapGen::generateSystemsRing(GenStruct & map)
{
	U32 playerSpace = RING_MAX_SYSTEMS/map.numPlayers;
	U32 remotePicks = (map.systemsToMake/map.numPlayers)-1;
	U32 s1;
	U32 playersPlaced = 0;
	for(s1 = 0; s1 < map.numPlayers; ++s1)
	{
		GenSystem * system1 = &(map.systems[s1]);			

		memset(system1, 0, sizeof(GenSystem));

		U32 val = s1*playerSpace;
		system1->sectorGridX = ringSystemX[val];
		system1->sectorGridY = ringSystemY[val];
		system1->connectionOrder = val;
		
		map.sectorGrid[system1->sectorGridX] |= (0x00000001 <<system1->sectorGridY); 

		system1->index = s1;
		do
		{
			system1->playerID = GetRand(1,map.numPlayers,MAP_GEN_ENUM::LINEAR);
		}while((0x01 << system1->playerID) & playersPlaced);
		playersPlaced |= (0x01 << system1->playerID);

		map.systemCount++;		
	}
	for (U32 i = 0; i < map.numPlayers; ++i)
	{
		for(U32 j = 0; j < remotePicks;++j)
		{
			s1 = map.systemCount;
			GenSystem * system1 = &(map.systems[s1]);			

			memset(system1, 0, sizeof(GenSystem));

			do
			{
				U32 maxVal = ((i+1)*playerSpace)-1;
				if(maxVal > 15)
					maxVal = 15;
				U32 val =GetRand(i*playerSpace+1,maxVal , MAP_GEN_ENUM::LINEAR);
				system1->sectorGridX = ringSystemX[val];
				system1->sectorGridY = ringSystemY[val];
				system1->connectionOrder = val;
			} while (SystemsOverlap(map,system1));
			
			map.sectorGrid[system1->sectorGridX] |= (0x00000001 <<system1->sectorGridY); 

			system1->index = s1;

			map.systemCount++;
		}
	}
}
//--------------------------------------------------------------------------//
//
void MapGen::GenerateSystems(GenStruct & map)
{
	if(map.sectorLayout == MAP_GEN_ENUM::SF_RANDOM || map.sectorLayout == MAP_GEN_ENUM::SF_MULTI_RANDOM)
	{	
		generateSystemsRandom(map);
	}
	else if(map.sectorLayout == MAP_GEN_ENUM::SF_RING)
	{
		generateSystemsRing(map);
	}
	else if(map.sectorLayout == MAP_GEN_ENUM::SF_STAR)
	{
		generateSystemsStar(map);
	}
}
//--------------------------------------------------------------------------//
//
bool validData(BT_MAP_GEN * data )
{
	U32 numThemes = 0;
	while((data->themes[numThemes].okForRemoteSystem || data->themes[numThemes].okForPlayerStart) && numThemes < MAX_THEMES)
	{
		++numThemes;
	}
	CQASSERT(numThemes && "You don't have any themes with okForRemoteSystem or okForPlayerStart");

	if(numThemes)
	{
		bool bHasHomeSystem = false;
		bool bHasRemoteSystem = false;
		for(U32 i = 0; i < numThemes; ++i)
		{
			if(data->themes[i].okForRemoteSystem)
				bHasRemoteSystem = true;
			if(data->themes[i].okForPlayerStart)
				bHasHomeSystem = true;

			U32 numHabitable = 0;
			U32 numMetalPL = 0;
			U32 numGasPL = 0;
			U32 numCrew = 0;

			U32 count;
			for(count = 0; count < MAX_TYPES; ++count)
			{
				if(data->themes[i].habitablePlanets[count][0])
				{
					++numHabitable;
					if(!(ARCHLIST->GetArchetypeData(data->themes[i].habitablePlanets[count])))
					{
						CQBOMB3("Bad MapGenData - Theme:%d habitablePlanets:%d String:%s",i,count,data->themes[i].habitablePlanets[count]);
						return true;
					}
				}
				else
					break;
			}

			for(count = 0; count < MAX_TYPES; ++count)
			{
				if(data->themes[i].metalPlanets[count][0])
				{
					++numMetalPL;
					if(!(ARCHLIST->GetArchetypeData(data->themes[i].metalPlanets[count])))
					{
						CQBOMB3("Bad MapGenData - Theme:%d metalPlanets:%d String:%s",i,count,data->themes[i].metalPlanets[count]);
						return true;
					}
				}
				else
					break;
			}

			for(count = 0; count < MAX_TYPES; ++count)
			{
				if(data->themes[i].gasPlanets[count][0])
				{
					++numGasPL;
					if(!(ARCHLIST->GetArchetypeData(data->themes[i].gasPlanets[count])))
					{
						CQBOMB3("Bad MapGenData - Theme:%d gasPlanets:%d String:%s",i,count,data->themes[i].gasPlanets[count]);
						return true;
					}
				}
				else
					break;
			}

			for(count = 0; count < MAX_TYPES; ++count)
			{
				if(data->themes[i].otherPlanets[count][0])
				{
					++numCrew;
					if(!(ARCHLIST->GetArchetypeData(data->themes[i].otherPlanets[count])))
					{
						CQBOMB3("Bad MapGenData - Theme:%d otherPlanets:%d String:%s",i,count,data->themes[i].otherPlanets[count]);
						return true;
					}
				}
				else
					break;
			}
			
			if((!numHabitable) && (data->themes[i].numHabitablePlanets[0] || data->themes[i].numHabitablePlanets[1]|| data->themes[i].numHabitablePlanets[2]))
			{
				CQBOMB1("Bad MapGenData - Theme:%d  Has no habitable planets but tries to place them",i);
			}
			if((!numMetalPL) && (data->themes[i].numMetalPlanets[0] || data->themes[i].numMetalPlanets[1]|| data->themes[i].numMetalPlanets[2]))
			{
				CQBOMB1("Bad MapGenData - Theme:%d  Has no metal planets but tries to place them",i);
			}
			if((!numGasPL) && (data->themes[i].numGasPlanets[0] || data->themes[i].numGasPlanets[1]|| data->themes[i].numGasPlanets[2]))
			{
				CQBOMB1("Bad MapGenData - Theme:%d  Has no gas planets but tries to place them",i);
			}
			if((!numCrew) && (data->themes[i].numOtherPlanets[0] || data->themes[i].numOtherPlanets[1]|| data->themes[i].numOtherPlanets[2]))
			{
				CQBOMB1("Bad MapGenData - Theme:%d  Has no other planets but tries to place them",i);
			}
		}
		CQASSERT(bHasHomeSystem && "You need at least one theme marked as okForPlayerStart");
		CQASSERT(bHasRemoteSystem && "You need at least one theme marked as okForRemoteSystem");
	}
	return true;
}
//--------------------------------------------------------------------------//
//
void MapGen::initMap (GenStruct & map, const FULLCQGAME & game)
{
	BT_MAP_GEN * data = (BT_MAP_GEN *)ARCHLIST->GetArchetypeData("MAPGEN!!Map");

	CQASSERT(validData(data));

	S32 i;
	for(i = 0; i <17;++i)
	{
		map.sectorGrid[i] = 0;
	}

	if(game.templateType == TEMPLATE_RANDOM)
	{
		map.sectorLayout = MAP_GEN_ENUM::SF_RANDOM;
	}
	else if(game.templateType == TEMPLATE_NEW_RANDOM)
	{
		map.sectorLayout = MAP_GEN_ENUM::SF_MULTI_RANDOM;
	}
	else if(game.templateType == TEMPLATE_RING)
	{
		map.sectorLayout = MAP_GEN_ENUM::SF_RING;
	}
	else if(game.templateType == TEMPLATE_STAR)
	{
		map.sectorLayout = MAP_GEN_ENUM::SF_STAR;
	}

	map.data = data;

	map.systemCount = 0;
	map.numJumpGates = 0;

	map.numPlayers = 0;

	U32 assignments[MAX_PLAYERS+1];
	memset(assignments, 0, sizeof(assignments));
	for(i = 0; i < S32(game.activeSlots); ++i)
	{
		if (game.slot[i].state == READY)
			assignments[game.slot[i].color] = 1;
	}

	for (i = 1; i <= MAX_PLAYERS; i++)
		map.numPlayers += assignments[i];

	if(game.mapSize == SMALL_MAP)
		map.gameSize = 0;
	else if(game.mapSize == MEDIUM_MAP)
		map.gameSize = 1;
	else
		map.gameSize = 2;

	if(game.terrain == LIGHT_TERRAIN)
		map.terrainSize = 0;
	else if(game.terrain == MEDIUM_TERRAIN)
		map.terrainSize = 1;
	else
		map.terrainSize = 2;

	map.systemsToMake = GetBestSystemNumber(game,game.numSystems);

	map.sectorSize = 8;
	MGlobals::SetFileMaxPlayers(map.numPlayers);
}
//--------------------------------------------------------------------------//
//
void MapGen::insertObject (char * name,Vector position,U32 playerID,U32 systemID, GenSystem * system)
{
	IBaseObject * rtObject = MGlobals::CreateInstance(ARCHLIST->LoadArchetype(name),MGlobals::CreateNewPartID(playerID));
	OBJLIST->AddObject(rtObject);

	MPartNC part(rtObject);
	if(part.isValid())
	{
		CQASSERT(part->partName[0]);
		part->hullPoints = part->hullPointsMax;
		if ((part->mObjClass != M_HARVEST) && (part->mObjClass != M_GALIOT) &&(part->mObjClass != M_SIPHON))
			part->supplies   = part->supplyPointsMax;
		rtObject->SetReady(true);

		if (part->playerID != 0)
		{
			rtObject->SetVisibleToPlayer(part->playerID);
			rtObject->UpdateVisibilityFlags();
		}
	}

	OBJPTR<IPhysicalObject> physObj;
	rtObject->QueryInterface(IPhysicalObjectID, physObj);
	if (physObj)
	{
		physObj->SetPosition(position, systemID);
		ENGINE->update_instance(physObj.Ptr()->GetObjectIndex(),0,0);
	}
	if(rtObject->GetPlayerID())
		SECTOR->RevealSystem(systemID, rtObject->GetPlayerID());

	if(rtObject->objClass == OC_PLANETOID && part.isValid())
	{
		VOLPTR(IPlanet) planet = rtObject;
		if(planet && (!(planet->IsMoon())))
		{
			system->planetNameCount++;
			char buffer[32];
			SECTOR->GetSystemNameChar(systemID, buffer, sizeof(buffer));
			sprintf(part->partName,"%-.20s %-.10s",buffer,_localLoadString(IDS_BEGIN_PLANET_NAMES+system->planetNameCount));
		}
	}
}
//--------------------------------------------------------------------------//
//
U32 MapGen::GetRand(U32 min, U32 max,MAP_GEN_ENUM::DMAP_FUNC mapFunc)
{
	max++;
	U64 val = (U64)(randFunc[mapFunc]());
	val = (val*((U64)(max-min))) >> FIX15;
	val = val+((U64)min);
	CQASSERT(((U32)val) >= min && ((U32)val) <= (max-1));
	return (U32)val;
}
//--------------------------------------------------------------------------//
//
bool MapGen::SystemsOverlap(GenStruct & map, GenSystem * system)
{
	for(U32 count = 0; count < map.systemCount;++count)
	{
		if(((map.sectorGrid[system->sectorGridX]) >> system->sectorGridY) & 0x01)
			return true;
	}
	return false;
}
//--------------------------------------------------------------------------//
//
bool MapGen::SpaceEmpty(GenSystem * system,U32 xPos,U32 yPos,MAP_GEN_ENUM::OVERLAP overlap,U32 size)
{
	if(xPos+size-1 >= system->size)
		return false;
	if(yPos+size-1 >= system->size)
		return false;
	for(U32 ix = 0; ix<size;++ix)
	{
		for(U32 iy = 0; iy< size; ++iy)
		{
			U8 value = system->objectMap[xPos+ix][yPos+iy];
			if(value == GENMAP_TAKEN)
				return false;
			else if(value == GENMAP_LEVEL1 && overlap == MAP_GEN_ENUM::NO_OVERLAP)
				return false;
			else if(value == GENMAP_LEVEL2 && overlap != MAP_GEN_ENUM::LEVEL1)
				return false;
			else if(value == GENMAP_PATH && overlap == MAP_GEN_ENUM::NO_OVERLAP)
				return false;
		}
	}
	return true;
}
//--------------------------------------------------------------------------//
//
void MapGen::FillPosition(GenSystem * system,U32 xPos, U32 yPos,U32 size,MAP_GEN_ENUM::OVERLAP overlap)
{
	for(U32 ix = 0; ix<size;++ix)
	{
		if(xPos+ix >= 0 && xPos+ix < system->size)
		{
			for(U32 iy = 0; iy< size; ++iy)
			{
				if(yPos+iy >= 0 && yPos+iy < system->size)
				{
					system->omUsed++;
					if(overlap == MAP_GEN_ENUM::NO_OVERLAP)
						system->objectMap[xPos+ix][yPos+iy] = GENMAP_TAKEN;
					else if(overlap == MAP_GEN_ENUM::LEVEL1)
					{
						U8 value = system->objectMap[xPos+ix][yPos+iy];
						if(value == 0 || value == GENMAP_PATH)
							system->objectMap[xPos+ix][yPos+iy] = GENMAP_LEVEL1;
					}
					else if(overlap == MAP_GEN_ENUM::LEVEL2)
					{
						U8 value = system->objectMap[xPos+ix][yPos+iy];
						if(value == 0 || value == GENMAP_LEVEL1|| value == GENMAP_PATH)
							system->objectMap[xPos+ix][yPos+iy] = GENMAP_LEVEL2;
					}
				}
			}
		}
	}	
}
//--------------------------------------------------------------------------//
//
void MapGen::GetJumpgatePositions(GenStruct & map, GenSystem * sys1, GenSystem * sys2, U32 & jx1, U32 & jy1, U32 & jx2, U32 & jy2)
{
	S32 xDif = sys1->sectorGridX-sys2->sectorGridX;
	S32 yDif = sys1->sectorGridY-sys2->sectorGridY;
	SINGLE dist = sqrt((FLOAT)(xDif*xDif+yDif*yDif));

	S32 cSize = sys1->size/2;
	
	S32 edge = (((cSize-1)*xDif)/dist)+cSize;
	S32 t = GetRand(0,0x00004FFF,MAP_GEN_ENUM::MORE_IS_LIKLY);
	jx1 = (((cSize-edge)*t) >> FIX15) + cSize;

	edge = (((cSize-1)*yDif)/dist)+cSize;
	t = GetRand(0,0x00004FFF,MAP_GEN_ENUM::MORE_IS_LIKLY);
	jy1 = (((cSize-edge)*t) >> FIX15) + cSize;

	while(!SpaceEmpty(sys1,jx1,jy1,MAP_GEN_ENUM::NO_OVERLAP,3))
	{
		if(jx1== (U32)cSize && jy1 == (U32)cSize)
		{
			bool findSuccess = FindPosition(sys1,3,MAP_GEN_ENUM::NO_OVERLAP,jx1,jy1);
			CQASSERT(findSuccess && "Full System could not place jumpgate");
			break;
		}else
		{
			if(jx1 < (U32)cSize)
				++jx1;
			else if(jx1 > (U32)cSize)
				--jx1;
		}
		if(!SpaceEmpty(sys1,jx1,jy1,MAP_GEN_ENUM::NO_OVERLAP,3))
		{
			if(jy1 < (U32)cSize)
				++jy1;
			else if(jy1 > (U32)cSize)
				--jy1;
		}
	}

	cSize = sys2->size/2;

	edge = (((cSize-1)*(-xDif))/dist)+cSize;
	t = GetRand(0,0x00004FFF,MAP_GEN_ENUM::MORE_IS_LIKLY);
	jx2 = (((cSize-edge)*t) >> FIX15) + cSize;

	edge = (((cSize-1)*(-yDif))/dist)+cSize;
	t = GetRand(0,0x00004FFF,MAP_GEN_ENUM::MORE_IS_LIKLY);
	jy2 = (((cSize-edge)*t) >> FIX15) + cSize;

	while(!SpaceEmpty(sys2,jx2,jy2,MAP_GEN_ENUM::NO_OVERLAP,3))
	{
		if(jx2== (U32)cSize && jy2 == (U32)cSize)
		{
			bool findSuccess = FindPosition(sys2,3,MAP_GEN_ENUM::NO_OVERLAP,jx2,jy2);
			CQASSERT(findSuccess && "Full System could not place jumpgate");
			break;
		}else
		{
			if(jx2 < (U32)cSize)
				++jx2;
			else if(jx2 > (U32)cSize)
				--jx2;
		}
		if(!SpaceEmpty(sys2,jx2,jy2,MAP_GEN_ENUM::NO_OVERLAP,3))
		{
			if(jy2 < (U32)cSize)
				++jy2;
			else if(jy2 > (U32)cSize)
				--jy2;
		}
	}

}
//--------------------------------------------------------------------------//
//
bool MapGen::CrossesAnotherSystem(GenStruct & map, GenSystem * sys1, GenSystem * sys2, U32 jx1, U32 jy1, U32 jx2, U32 jy2)
{
	U32 s;

	GenSystem* sys;

	U32 halfSystemSize = MAX_MAP_SIZE/2;

	jx1 = jx1*2*MAX_MAP_SIZE+halfSystemSize;
	jy1 = jy1*2*MAX_MAP_SIZE+halfSystemSize;
	jx2 = jx2*2*MAX_MAP_SIZE+halfSystemSize;
	jy2 = jy2*2*MAX_MAP_SIZE+halfSystemSize;
	for (s = 0; s < map.systemCount; s++)
	{
		if (s != sys1->index && s != sys2->index)
		{
			sys = &(map.systems[s]);

			if (LinesCross(jx1, jy1, jx2, jy2, 
				sys->sectorGridX*2*MAX_MAP_SIZE, sys->sectorGridY*2*MAX_MAP_SIZE, 
				sys->sectorGridX*2*MAX_MAP_SIZE+MAX_MAP_SIZE, sys->sectorGridY*2*MAX_MAP_SIZE))
				return true;
			if (LinesCross(jx1, jy1, jx2, jy2, 
				sys->sectorGridX*2*MAX_MAP_SIZE, sys->sectorGridY*2*MAX_MAP_SIZE+MAX_MAP_SIZE, 
				sys->sectorGridX*2*MAX_MAP_SIZE+MAX_MAP_SIZE, sys->sectorGridY*2*MAX_MAP_SIZE+MAX_MAP_SIZE))
				return true;
			if (LinesCross(jx1, jy1, jx2, jy2, 
				sys->sectorGridX*2*MAX_MAP_SIZE, sys->sectorGridY*2*MAX_MAP_SIZE, 
				sys->sectorGridX*2*MAX_MAP_SIZE, sys->sectorGridY*2*MAX_MAP_SIZE+MAX_MAP_SIZE))
				return true;
			if (LinesCross(jx1, jy1, jx2, jy2, 
				sys->sectorGridX*2*MAX_MAP_SIZE+MAX_MAP_SIZE, sys->sectorGridY*2*MAX_MAP_SIZE, 
				sys->sectorGridX*2*MAX_MAP_SIZE+MAX_MAP_SIZE, sys->sectorGridY*2*MAX_MAP_SIZE+MAX_MAP_SIZE))
				return true;
		}
	}
	return false;
}
//--------------------------------------------------------------------------//
//
bool MapGen::CrossesAnotherLink(GenStruct & map, GenJumpgate * gate)
{
	GenJumpgate* other;
	U32 j;

	for (j = 0; j < map.numJumpGates; j++)
	{
		other = &(map.jumpgate[j]);
		if (other != gate && other->created && 
			((other->system1 != gate->system1) && (other->system1 != gate->system2) &&
			(other->system2 != gate->system1) && (other->system2 != gate->system2)))
		{
			if (LinesCross(gate->x1, gate->y1, gate->x2, gate->y2,
						   other->x1, other->y1, other->x2, other->y2))
			{
				return true;
			}
		}
	}
	return false;
}
//--------------------------------------------------------------------------//
//
bool MapGen::LinesCross(S32 minX1, S32 minY1, S32 maxX1, S32 maxY1, S32 minX2, S32 minY2, S32 maxX2, S32 maxY2)
{
	SINGLE deltaX1 = maxX1 - minX1;
	SINGLE deltaY1 = maxY1 - minY1;
	SINGLE deltaX2 = maxX2 - minX2;
	SINGLE deltaY2 = maxY2 - minY2;
 
	SINGLE delta = deltaX1 * deltaY2 - deltaY1 * deltaX2;

	if (fabsf(delta) < 0.00001f) return false;

	SINGLE mu1 = ((minX2 - minX1) * deltaY2 - (minY2 - minY1) * deltaX2) / delta;
	SINGLE mu2 = ((minX1 - minX2) * deltaY1 - (minY1 - minY2) * deltaX1) / -delta;

	return (mu1 >= 0.0f && mu1 <= 1.0f && mu2 >= 0.0f && mu2 <= 1.0f);
}
//--------------------------------------------------------------------------//
//
void MapGen::SelectThemes(GenStruct & map)
{
	U32 playerThemeCount = 0;
	U32 i;
	for(i = 0; i < MAX_THEMES;++i)
	{
		if(map.data->themes[i].okForPlayerStart && (map.data->themes[i].sizeOk & (0x01 << map.gameSize)))
		{
			++playerThemeCount;
		}
	}
	U32 themeCount = 0;
	for(i = 0; i < MAX_THEMES;++i)
	{
		if(map.data->themes[i].okForRemoteSystem && (map.data->themes[i].sizeOk & (0x01 << map.gameSize)))
		{
			++themeCount;
		}
	}
	for (U32 s1 = 0; s1 < map.systemCount; s1++)
	{
		GenSystem * system = &(map.systems[s1]);

		if (system->playerID != 0)
		{
			U32 themeNumber = GetRand(1,playerThemeCount,MAP_GEN_ENUM::LINEAR);
			U32 theme =0;
			while(themeNumber)
			{
				if(map.data->themes[theme].okForPlayerStart && (map.data->themes[theme].sizeOk & (0x01 << map.gameSize)))
				{
					--themeNumber;
				}
				++theme;
			}
			--theme;
			system->theme = &(map.data->themes[theme]);
		}	
		else
		{
			U32 themeNumber = GetRand(1,themeCount,MAP_GEN_ENUM::LINEAR);
			U32 theme =0;
			while(themeNumber)
			{
				if(map.data->themes[theme].okForRemoteSystem && (map.data->themes[theme].sizeOk & (0x01 << map.gameSize)))
				{
					--themeNumber;
				}
				++theme;
			}
			--theme;
			system->theme = &(map.data->themes[theme]);
		}

		system->size = GetRand(system->theme->minSize,system->theme->maxSize,system->theme->sizeFunc);
		system->x = GetRand(0,MAX_MAP_GRID-system->size,MAP_GEN_ENUM::LINEAR);
		system->y = GetRand(0,MAX_MAP_GRID-system->size,MAP_GEN_ENUM::LINEAR);

		system->initObjectMap();
	}
}
//--------------------------------------------------------------------------//
//
void MapGen::CreateSystems(GenStruct & map)
{
	for(U32 s1 = 0; s1 < map.systemCount; ++s1)
	{
		GenSystem * system = &(map.systems[s1]);
		system->systemID = SECTOR->CreateSystem(system->x*GRIDSIZE+system->sectorGridX*2*MAX_MAP_SIZE,system->y*GRIDSIZE+system->sectorGridY*2*MAX_MAP_SIZE,system->size*GRIDSIZE+(GRIDSIZE/2),system->size*GRIDSIZE+(GRIDSIZE/2));
	}

	U32 namesUsed = 0;
	for(U32 i = 0; i < map.systemCount; ++i)
	{
		U32 name;
		do
		{
			name = GetRand(1,24,MAP_GEN_ENUM::LINEAR);
		}while(namesUsed & (0x01<< name));
		namesUsed |= (0x01 << name);
		SECTOR->SetSystemName(map.systems[i].systemID,IDS_BEGIN_SYSTEM_NAMES+name);
	}

	OBJMAP->Init();

}
//--------------------------------------------------------------------------//
//
void MapGen::RunHomeMacros(GenStruct & map)
{
	U32 xPos,yPos;
	for(U32 s1 = 0; s1 < map.systemCount; ++s1)
	{
		GenSystem * system = &(map.systems[s1]);
		if(system->playerID)
		{
			getMacroCenterPos(system,xPos,yPos);
			for(U32 i = 0; i < MAX_MACROS;++i)
			{
				BT_MAP_GEN::_terrainTheme::_macros * macro = &(system->theme->macros[i]);
				if(macro->active)
				{
					switch(macro->operation)
					{
						case MAP_GEN_ENUM::MC_PLACE_HABITABLE_PLANET:
							createPlanetFromList(xPos,yPos,system,macro->range,system->theme->habitablePlanets);
							break;
						case MAP_GEN_ENUM::MC_PLACE_GAS_PLANET:
							createPlanetFromList(xPos,yPos,system,macro->range,system->theme->gasPlanets);
							break;
						case MAP_GEN_ENUM::MC_PLACE_METAL_PLANET:
							createPlanetFromList(xPos,yPos,system,macro->range,system->theme->metalPlanets);
							break;
						case MAP_GEN_ENUM::MC_PLACE_OTHER_PLANET:
							createPlanetFromList(xPos,yPos,system,macro->range,system->theme->otherPlanets);
							break;
						case MAP_GEN_ENUM::MC_PLACE_TERRAIN:
							placeMacroTerrain(xPos,yPos,system,macro->range,&(macro->info.terrainInfo));
							break;
						case MAP_GEN_ENUM::MC_PLACE_PLAYER_BOMB:
							{
								U32 halfWidth = (GRIDSIZE)/2;
								insertObject("MISSION!!PLAYERBOMB",Vector(xPos*GRIDSIZE+halfWidth,yPos*GRIDSIZE+halfWidth,0),system->playerID,system->systemID,system);
							}
							break;
						case MAP_GEN_ENUM::MC_MARK_RING:
							{
								FillPosition(system,xPos-(macro->range-1),yPos-(macro->range-1),(2*macro->range),macro->info.overlap);
							}
							break;
						default:
							CQBOMB2("Unsupported macro:%d in theme:%d",macro->operation,((system->theme)-(map.data->themes))/sizeof(BT_MAP_GEN::_terrainTheme));
					}
				}
			}
		}
	}	
}
//--------------------------------------------------------------------------//
//
void MapGen::placeMacroTerrain(S32 centerX,S32 centerY,GenSystem * system,S32 range,BT_MAP_GEN::_terrainTheme::_terrainInfo * terrainInfo)
{
	BASIC_DATA * data = (BASIC_DATA *)( ARCHLIST->GetArchetypeData(terrainInfo->terrainArchType));
	if(!data)
	{
		CQBOMB1("Bad archtype name in random map generator.  Fix the data not a code bug, Name:%s",terrainInfo->terrainArchType);
	}
	if((data->objClass == OC_NEBULA) || (data->objClass == OC_FIELD))
	{
		U32 numToPlace = GetRand(terrainInfo->minToPlace,terrainInfo->maxToPlace,terrainInfo->numberFunc);
		U32 startX,startY;
		if(findMacroPosition(system,centerX,centerY,range,1,terrainInfo->overlap,startX,startY))
		{
			BASE_FIELD_DATA * fData = (BASE_FIELD_DATA *) data;
			if(fData->fieldClass == FC_ANTIMATTER)
			{
				switch(terrainInfo->placement)
				{
				case MAP_GEN_ENUM::RANDOM:
					PlaceRandomRibbon(terrainInfo,numToPlace,startX,startY,system);
					break;
				case MAP_GEN_ENUM::SPOTS:
					CQASSERT(0 && "NOT SUPPORTED");
					break;
				case MAP_GEN_ENUM::CLUSTER:
					CQASSERT(0 && "NOT SUPPORTED");
					break;
				case MAP_GEN_ENUM::PLANET_RING:
					CQASSERT(0 && "NOT SUPPORTED");
					break;
				case MAP_GEN_ENUM::STREEKS:
					CQASSERT(0 && "NOT SUPPORTED");
					break;
				}
			}
			else  //a regular nebula
			{
				switch(terrainInfo->placement)
				{
				case MAP_GEN_ENUM::RANDOM:
					PlaceRandomField(terrainInfo,numToPlace,startX,startY,system);
					break;
				case MAP_GEN_ENUM::SPOTS:
					PlaceSpottyField(terrainInfo,numToPlace,startX,startY,system);
					break;
				case MAP_GEN_ENUM::CLUSTER:
					CQASSERT(0 && "NOT SUPPORTED");
					break;
				case MAP_GEN_ENUM::PLANET_RING:
					PlaceRingField(terrainInfo,system);					
					break;
				case MAP_GEN_ENUM::STREEKS:
					CQASSERT(0 && "NOT SUPPORTED");
					break;
				}
			}
		}
	}
	else // it is a regular object
	{
		switch(terrainInfo->placement)
		{
		case MAP_GEN_ENUM::RANDOM:
			{
				U32 numToPlace = GetRand(terrainInfo->minToPlace,terrainInfo->maxToPlace,terrainInfo->numberFunc);
				for(U32 i = 0; i < numToPlace; ++i)
				{				
					U32 xPos,yPos;
					bool bSuccess = findMacroPosition(system,centerX,centerY,range,terrainInfo->size,terrainInfo->overlap,xPos,yPos);
					if(bSuccess)
					{
						FillPosition(system,xPos,yPos,terrainInfo->size,terrainInfo->overlap);
						U32 halfWidth = (GRIDSIZE*terrainInfo->size)/2;
						U32 playerID = 0;
						if(data->objClass&CF_PLAYERALIGNED)
							playerID = system->playerID;
						insertObject(terrainInfo->terrainArchType,Vector(xPos*GRIDSIZE+halfWidth,yPos*GRIDSIZE+halfWidth,0),playerID,system->systemID,system);
					}
					else
					{
						system->omUsed += terrainInfo->size*terrainInfo->size;
					}
				}
			}
			break;
		case MAP_GEN_ENUM::SPOTS:
			CQASSERT(0 && "NOT SUPPORTED");
			break;
		case MAP_GEN_ENUM::CLUSTER:
			CQASSERT(0 && "NOT SUPPORTED");
			break;
		case MAP_GEN_ENUM::PLANET_RING:
			CQASSERT(0 && "NOT SUPPORTED");
			break;
		case MAP_GEN_ENUM::STREEKS:
			CQASSERT(0 && "NOT SUPPORTED");
			break;
		}
	}}
//--------------------------------------------------------------------------//
//
void MapGen::getMacroCenterPos(GenSystem * system,U32 & x, U32 & y)
{
	FindPosition(system,6,MAP_GEN_ENUM::NO_OVERLAP,x,y);
}
//--------------------------------------------------------------------------//
//
S32 mapGenMacroX[16] = {-2,0,2,2,2,0,-2,-2,-1,1,2,2,1,-1,-2,-2};
S32 mapGenMacroY[16] = {-2,-2,-2,0,2,2,2,0,-2,-2,-1,1,2,2,1,-1};

bool MapGen::findMacroPosition(GenSystem * system,S32 centerX,S32 centerY,U32 range,U32 size,MAP_GEN_ENUM::OVERLAP overlap,U32 & posX,U32 &posY)
{
	S32 currentRange = range;
	U32 numPos = 0;
	while(!numPos)
	{
		for(U32 i = 0; i < 16; ++i)
		{
			if(SpaceEmpty(system,centerX+((mapGenMacroX[i]*currentRange)/2),centerY+((mapGenMacroY[i]*currentRange)/2),overlap,size))
				++numPos;
		}
		if(!numPos)
		{
			++currentRange;
			if(currentRange > ((S32)(system->size)))
				return false;
		}
	}
	U32 t = GetRand(0,numPos-1,MAP_GEN_ENUM::LINEAR);
	for(U32 i = 0; i < 16; ++i)
	{
		if(SpaceEmpty(system,centerX+((mapGenMacroX[i]*currentRange)/2),centerY+((mapGenMacroY[i]*currentRange)/2),overlap,size))
		{
			if(!t)
			{
				posX = centerX+((mapGenMacroX[i]*currentRange)/2);
				posY = centerY+((mapGenMacroY[i]*currentRange)/2);
				CQASSERT(posX+size-1 < system->size);
				CQASSERT(posY+size-1 < system->size);
				return true;
			}
			--t;
		}
	}
	return false;
}
//--------------------------------------------------------------------------//
//
void MapGen::createPlanetFromList(S32 xPos, S32 yPos, GenSystem * system, U32 range ,char planetList[][GT_PATH])
{
	U32 posX,posY;
	bool bSuccess = findMacroPosition(system,xPos,yPos,range,4,MAP_GEN_ENUM::NO_OVERLAP,posX,posY);
	if(bSuccess)
	{
		FillPosition(system,posX,posY,4,MAP_GEN_ENUM::NO_OVERLAP);
		if(system->numFlags < MAX_FLAGS)
		{
			system->flags[system->numFlags].xPos = posX+2;
			system->flags[system->numFlags].yPos = posY+2;
			system->flags[system->numFlags].type = FLAG_PLANET | FLAG_PATHON;
			system->numFlags++;
		}
		U32 maxPlanetIndex = 0;
		for(U32 i = 0; i < MAX_TYPES;++i)
		{
			if(planetList[i][0])
				++maxPlanetIndex;
			else
				break;
		}

		U32 planetID = GetRand(0,maxPlanetIndex-1,MAP_GEN_ENUM::LINEAR);
		insertObject(planetList[planetID],Vector((posX+2)*GRIDSIZE,(posY+2)*GRIDSIZE,0),0,system->systemID,system);
	}
}
//--------------------------------------------------------------------------//
//
void MapGen::CreateJumpgates(GenStruct & map)
{
	for(U32 i = 0; i < map.systemCount; ++i)
	{
		GenSystem * system1 = &(map.systems[i]);
		U32 cx1 = system1->sectorGridX;
		U32 cy1 = system1->sectorGridY;
		for(U32 j = i+1; j < map.systemCount; ++j)
		{
			GenSystem * system2 = &(map.systems[j]);
			U32 cx2 = system2->sectorGridX;
			U32 cy2 = system2->sectorGridY;
			if(!CrossesAnotherSystem(map,system1,system2,cx1,cy1,cx2,cy2))
			{
				GenJumpgate * jumpgate = &(map.jumpgate[map.numJumpGates]);
				++map.numJumpGates;

				jumpgate->system1 = system1;
				jumpgate->system2 = system2;
				jumpgate->dist = (cx1-cx2)*(cx1-cx2)+(cy1-cy2)*(cy1-cy2);
				jumpgate->x1 = cx1;
				jumpgate->y1 = cy1;
				jumpgate->x2 = cx2;
				jumpgate->y2 = cy2;
				jumpgate->created = false;
			}
		}
	}

	if(map.sectorLayout == MAP_GEN_ENUM::SF_RANDOM)
	{
		createRandomGates2(map);
	}
	else if(map.sectorLayout == MAP_GEN_ENUM::SF_MULTI_RANDOM)
	{
		createRandomGates3(map);
	}
	else if(map.sectorLayout == MAP_GEN_ENUM::SF_RING)
	{
		createRingGates(map);
	}
	else if(map.sectorLayout == MAP_GEN_ENUM::SF_STAR)
	{
		createStarGates(map);
	}
	
	for(U32 j1 = 0; j1 < map.numJumpGates; ++j1)
	{
		GenJumpgate * gate = &(map.jumpgate[j1]);
		if(gate->created)
		{
			U32 posX1,posY1,posX2,posY2;
			GetJumpgatePositions(map,gate->system1,gate->system2,posX1,posY1,posX2,posY2);
			if(gate->system1->numFlags < MAX_FLAGS)
			{
				gate->system1->flags[gate->system1->numFlags].xPos = posX1;
				gate->system1->flags[gate->system1->numFlags].yPos = posY1;
				gate->system1->flags[gate->system1->numFlags].type = FLAG_PATHON;
				gate->system1->numFlags++;
			}
			if(gate->system2->numFlags < MAX_FLAGS)
			{
				gate->system2->flags[gate->system2->numFlags].xPos = posX2;
				gate->system2->flags[gate->system2->numFlags].yPos = posY2;
				gate->system2->flags[gate->system2->numFlags].type = FLAG_PATHON;
				gate->system2->numFlags++;
			}
			FillPosition(gate->system1,posX1,posY1,3,MAP_GEN_ENUM::NO_OVERLAP);
			FillPosition(gate->system2,posX2,posY2,3,MAP_GEN_ENUM::NO_OVERLAP);
			U32 id1,id2;
			SECTOR->CreateJumpGate(gate->system1->systemID,posX1*GRIDSIZE+GRIDSIZE*1.5,posY1*GRIDSIZE+GRIDSIZE*1.5,id1,
				gate->system2->systemID,posX2*GRIDSIZE+GRIDSIZE*1.5,posY2*GRIDSIZE+GRIDSIZE*1.5,id2,"JGATE!!Jumpgate");
		}
	}
}
//--------------------------------------------------------------------------//
//
U32 MapGen::scoreGate(GenStruct & map,U32 gateIndex)
{
	GenJumpgate * gate = &(map.jumpgate[gateIndex]);
	U32 score = gate->dist;
	score += gate->system1->jumpgateCount*100;
	score += gate->system2->jumpgateCount*100;
	return score;
}
//--------------------------------------------------------------------------//
//
void MapGen::createGateLevel(GenStruct & map,U32 totalLevel, U32 levelSystems,U32 targetSystems,U32 gateNum, U32 currentGates[64], U32 score, 
							 U32 * bestGates, U32 &bestScore, U32 & bestGateNum, bool moreAllowed)
{
	if(!levelSystems)
	{
		if(bestScore)
		{
			if(bestScore > score)
			{
				bestScore = score;
				bestGateNum = gateNum;
				memcpy(bestGates,currentGates,sizeof(U32)*gateNum);
			}
		}
		else
		{
			bestScore = score;
			bestGateNum = gateNum;
			memcpy(bestGates,currentGates,sizeof(U32)*gateNum);
		}
	}
	else
	{
		U32 currentSystem = 0;
		U32 i;
		for(i = 0; i < map.systemCount; ++i)
		{
			if(levelSystems & (0x01 << i))
			{
				currentSystem = i;
				break;
			}
		}
		U32 newLevel = levelSystems & (~(0x01<< currentSystem));
		bool gateMade = !moreAllowed;//only do back up if more are alowed as well.
		for(i = 0; i < map.numJumpGates; ++i)
		{
			if(!(map.jumpgate[i].created))
			{
				if( (map.jumpgate[i].system1->index == currentSystem && (targetSystems & (0x01 << (map.jumpgate[i].system2->index)))) ||
					(map.jumpgate[i].system2->index == currentSystem && (targetSystems & (0x01 << (map.jumpgate[i].system1->index)))) )
				{
					if((!CrossesAnotherLink(map,&(map.jumpgate[i]))))
					{
						gateMade = true;
						U32 gateScore = scoreGate(map, i);
						map.jumpgate[i].system1->jumpgates[map.jumpgate[i].system1->jumpgateCount++] = &(map.jumpgate[i]);
						map.jumpgate[i].system2->jumpgates[map.jumpgate[i].system2->jumpgateCount++] = &(map.jumpgate[i]);
						map.jumpgate[i].created = true;
						currentGates[gateNum] = i;
						if(map.systems[currentSystem].playerID)
						{
							createGateLevel(map,totalLevel,newLevel,targetSystems,gateNum+1,currentGates,score+gateScore,bestGates,bestScore,bestGateNum,true);
						}
						else if(moreAllowed)
						{
							createGateLevel(map,totalLevel,levelSystems,targetSystems,gateNum+1,currentGates,score+gateScore,bestGates,bestScore,bestGateNum,false);
						}
						else
							createGateLevel(map,totalLevel,newLevel,targetSystems,gateNum+1,currentGates,score+gateScore,bestGates,bestScore,bestGateNum,true);
						map.jumpgate[i].system1->jumpgates[map.jumpgate[i].system1->jumpgateCount--] = 0;
						map.jumpgate[i].system2->jumpgates[map.jumpgate[i].system2->jumpgateCount--] = 0;
						map.jumpgate[i].created = false;
					}
				}
			}
		}
		if(!gateMade)
		{
			for(i = 0; i < map.numJumpGates; ++i)
			{
				if(!(map.jumpgate[i].created))
				{
					if( (map.jumpgate[i].system1->index == currentSystem && (totalLevel & (0x01 << (map.jumpgate[i].system2->index)))) ||
						(map.jumpgate[i].system2->index == currentSystem && (totalLevel & (0x01 << (map.jumpgate[i].system1->index)))) )
					{
						if((!CrossesAnotherLink(map,&(map.jumpgate[i]))))
						{
							U32 gateScore = scoreGate(map, i);
							map.jumpgate[i].system1->jumpgates[map.jumpgate[i].system1->jumpgateCount++] = &(map.jumpgate[i]);
							map.jumpgate[i].system2->jumpgates[map.jumpgate[i].system2->jumpgateCount++] = &(map.jumpgate[i]);
							map.jumpgate[i].created = true;
							currentGates[gateNum] = i;
							createGateLevel(map,totalLevel,newLevel,targetSystems,gateNum+1,currentGates,score+gateScore,bestGates,bestScore,bestGateNum,true);
							map.jumpgate[i].system1->jumpgates[map.jumpgate[i].system1->jumpgateCount--] = 0;
							map.jumpgate[i].system2->jumpgates[map.jumpgate[i].system2->jumpgateCount--] = 0;
							map.jumpgate[i].created = false;
						}
					}
				}
			}
		}
	}
}
//--------------------------------------------------------------------------//
//
void MapGen::markSystems(U32 & systemUnconnected,GenSystem * system, U32 & systemsVisited)
{
	systemUnconnected &= ~(0x01 << system->index);
	systemsVisited |= (0x01 << system->index);
	for(U32 i = 0; i < system->jumpgateCount; ++i)
	{
		if(!((0x01 <<system->jumpgates[i]->system1->index) & systemsVisited))
		{
			markSystems(systemUnconnected,system->jumpgates[i]->system1,systemsVisited);
		}
		if(!((0x01 <<system->jumpgates[i]->system2->index) & systemsVisited))
		{
			markSystems(systemUnconnected,system->jumpgates[i]->system2,systemsVisited);
		}
	}
}
//--------------------------------------------------------------------------//
//
void MapGen::createRandomGates2(GenStruct & map)
{
	if(map.systemCount == map.numPlayers)
	{
		createRingGates(map);
	}
	else
	{
		U32 levelSystems = 0;

		U32 targetSystems = 0;

		U32 i;
		for(i = 0 ; i < map.systemCount; ++i)
		{
			if(map.systems[i].playerID)
			{
				levelSystems |= (0x01 << i);
			}
			else
			{
				targetSystems |= (0x01 << i);
			}
		}

		//create web of systems;
		bool freeSystems = false;
		while(targetSystems) 
		{
			U32 currentGates[64];
			U32 bestGates[64];
			U32 bestScore = 0;
			U32 bestGateNum = 0;
			createGateLevel(map, levelSystems,levelSystems,targetSystems,0,currentGates,0,bestGates,bestScore,bestGateNum,false);
			if(bestGateNum == 0)
			{
				freeSystems = true;
				break;
			}
			else
			{
				U32 newLevel = 0;
				for(U32 i = 0; i < bestGateNum; ++i)
				{
					if((0x01 << (map.jumpgate[bestGates[i]].system1->index)) &  levelSystems)
						newLevel |= (0x01 << map.jumpgate[bestGates[i]].system2->index);
					else
						newLevel |= (0x01 << map.jumpgate[bestGates[i]].system1->index);
					map.jumpgate[bestGates[i]].system1->jumpgates[map.jumpgate[bestGates[i]].system1->jumpgateCount++] = &(map.jumpgate[bestGates[i]]);
					map.jumpgate[bestGates[i]].system2->jumpgates[map.jumpgate[bestGates[i]].system2->jumpgateCount++] = &(map.jumpgate[bestGates[i]]);
					map.jumpgate[bestGates[i]].created = true;
				}
				targetSystems &= (~newLevel);
				levelSystems = newLevel;
			}
		}
		//attempt to connect all of the systems on the last level to one another
		if(levelSystems)
		{
			for(i = 0; i < map.numJumpGates; ++i)
			{
				if(!(map.jumpgate[i].created))
				{
					if(((0x01 << map.jumpgate[i].system1->index) & levelSystems) && ((0x01 << map.jumpgate[i].system2->index) & levelSystems))
					{
						if(!CrossesAnotherLink(map,& (map.jumpgate[i])))
						{
							map.jumpgate[i].system1->jumpgates[map.jumpgate[i].system1->jumpgateCount++] = &(map.jumpgate[i]);
							map.jumpgate[i].system2->jumpgates[map.jumpgate[i].system2->jumpgateCount++] = &(map.jumpgate[i]);
							map.jumpgate[i].created = true;
						}
					}
				}
			}
		}
		//make sure all players are connected
		U32 systemUnconnected = ~(0xFFFFFFFF << map.systemCount);
		U32 systemsVisited = 0;
		markSystems(systemUnconnected,map.systems,systemsVisited);
		while(systemUnconnected)
		{
			for(U32 i = 0; i < map.systemCount; ++i)
			{
				if((0x01 << i) & systemUnconnected)
				{
					bool breakOk = false;
					for(U32 j = 0; j < map.numJumpGates; ++j)
					{
						if(!map.jumpgate[j].created)
						{
							if( ( (map.jumpgate[j].system1->index == i) && (!((0x01 << map.jumpgate[j].system2->index) & systemUnconnected)) && (!(map.jumpgate[j].system2->playerID)) ) || 
								( (map.jumpgate[j].system2->index == i) && (!((0x01 << map.jumpgate[j].system1->index) & systemUnconnected)) && (!(map.jumpgate[j].system1->playerID)) ) )
							{
								if(!CrossesAnotherLink(map,& (map.jumpgate[j])))
								{
									if((!breakOk) || GetRand(1,10,MAP_GEN_ENUM::LINEAR) > 5)
									{
										map.jumpgate[j].system1->jumpgates[map.jumpgate[j].system1->jumpgateCount++] = &(map.jumpgate[j]);
										map.jumpgate[j].system2->jumpgates[map.jumpgate[j].system2->jumpgateCount++] = &(map.jumpgate[j]);
										map.jumpgate[j].created = true;
										breakOk = true;
									}
								}
							}
						}
					}
					if(breakOk)
						break;
				}
			}
			systemUnconnected = ~(0xFFFFFFFF << map.systemCount);
			systemsVisited = 0;
			markSystems(systemUnconnected,map.systems,systemsVisited);
		}
	}
}
//--------------------------------------------------------------------------//
//
void MapGen::createGateLevel2(GenStruct & map,U32 totalLevel, U32 levelSystems,U32 targetSystems,U32 gateNum, U32 currentGates[64], U32 score, 
							 U32 * bestGates, U32 &bestScore, U32 & bestGateNum, bool moreAllowed)
{
	if(!levelSystems)
	{
		if(bestScore)
		{
			if(bestScore > score)
			{
				bestScore = score;
				bestGateNum = gateNum;
				memcpy(bestGates,currentGates,sizeof(U32)*gateNum);
			}
		}
		else
		{
			bestScore = score;
			bestGateNum = gateNum;
			memcpy(bestGates,currentGates,sizeof(U32)*gateNum);
		}
	}
	else
	{
		U32 currentSystem = 0;
		U32 i;
		for(i = 0; i < map.systemCount; ++i)
		{
			if(levelSystems & (0x01 << i))
			{
				currentSystem = i;
				break;
			}
		}
		U32 newLevel = levelSystems & (~(0x01<< currentSystem));
		bool gateMade = !moreAllowed;//only do back up if more are alowed as well.
		for(i = 0; i < map.numJumpGates; ++i)
		{
			if(!(map.jumpgate[i].created))
			{
				if( (map.jumpgate[i].system1->index == currentSystem && (targetSystems & (0x01 << (map.jumpgate[i].system2->index)))) ||
					(map.jumpgate[i].system2->index == currentSystem && (targetSystems & (0x01 << (map.jumpgate[i].system1->index)))) )
				{
					if((!CrossesAnotherLink(map,&(map.jumpgate[i]))))
					{
						gateMade = true;
						U32 gateScore = scoreGate(map, i);
						map.jumpgate[i].system1->jumpgates[map.jumpgate[i].system1->jumpgateCount++] = &(map.jumpgate[i]);
						map.jumpgate[i].system2->jumpgates[map.jumpgate[i].system2->jumpgateCount++] = &(map.jumpgate[i]);
						map.jumpgate[i].created = true;
						currentGates[gateNum] = i;
						if(moreAllowed)
						{
							createGateLevel(map,totalLevel,levelSystems,targetSystems,gateNum+1,currentGates,score+gateScore,bestGates,bestScore,bestGateNum,false);
						}
						else
							createGateLevel(map,totalLevel,newLevel,targetSystems,gateNum+1,currentGates,score+gateScore,bestGates,bestScore,bestGateNum,true);
						map.jumpgate[i].system1->jumpgates[map.jumpgate[i].system1->jumpgateCount--] = 0;
						map.jumpgate[i].system2->jumpgates[map.jumpgate[i].system2->jumpgateCount--] = 0;
						map.jumpgate[i].created = false;
					}
				}
			}
		}
		if(!gateMade)
		{
			for(i = 0; i < map.numJumpGates; ++i)
			{
				if(!(map.jumpgate[i].created))
				{
					if( (map.jumpgate[i].system1->index == currentSystem && (totalLevel & (0x01 << (map.jumpgate[i].system2->index)))) ||
						(map.jumpgate[i].system2->index == currentSystem && (totalLevel & (0x01 << (map.jumpgate[i].system1->index)))) )
					{
						if((!CrossesAnotherLink(map,&(map.jumpgate[i]))))
						{
							U32 gateScore = scoreGate(map, i);
							map.jumpgate[i].system1->jumpgates[map.jumpgate[i].system1->jumpgateCount++] = &(map.jumpgate[i]);
							map.jumpgate[i].system2->jumpgates[map.jumpgate[i].system2->jumpgateCount++] = &(map.jumpgate[i]);
							map.jumpgate[i].created = true;
							currentGates[gateNum] = i;
							createGateLevel(map,totalLevel,newLevel,targetSystems,gateNum+1,currentGates,score+gateScore,bestGates,bestScore,bestGateNum,true);
							map.jumpgate[i].system1->jumpgates[map.jumpgate[i].system1->jumpgateCount--] = 0;
							map.jumpgate[i].system2->jumpgates[map.jumpgate[i].system2->jumpgateCount--] = 0;
							map.jumpgate[i].created = false;
						}
					}
				}
			}
		}
	}
}
//--------------------------------------------------------------------------//
//
void MapGen::createRandomGates3(GenStruct & map)
{
	if(map.systemCount < map.numPlayers*2)
	{
		createRandomGates2(map);
	}
	else
	{
		U32 levelSystems = 0;

		U32 targetSystems = 0;

		U32 i;
		for(i = 0 ; i < map.systemCount; ++i)
		{
			if(map.systems[i].playerID)
			{
				levelSystems |= (0x01 << i);
			}
			else
			{
				targetSystems |= (0x01 << i);
			}
		}

		//create web of systems;
		bool freeSystems = false;
		while(targetSystems) 
		{
			U32 currentGates[64];
			U32 bestGates[64];
			U32 bestScore = 0;
			U32 bestGateNum = 0;
			createGateLevel2(map, levelSystems,levelSystems,targetSystems,0,currentGates,0,bestGates,bestScore,bestGateNum,true);
			if(bestGateNum == 0)
			{
				freeSystems = true;
				break;
			}
			else
			{
				U32 newLevel = 0;
				for(U32 i = 0; i < bestGateNum; ++i)
				{
					if((0x01 << (map.jumpgate[bestGates[i]].system1->index)) &  levelSystems)
						newLevel |= (0x01 << map.jumpgate[bestGates[i]].system2->index);
					else
						newLevel |= (0x01 << map.jumpgate[bestGates[i]].system1->index);
					map.jumpgate[bestGates[i]].system1->jumpgates[map.jumpgate[bestGates[i]].system1->jumpgateCount++] = &(map.jumpgate[bestGates[i]]);
					map.jumpgate[bestGates[i]].system2->jumpgates[map.jumpgate[bestGates[i]].system2->jumpgateCount++] = &(map.jumpgate[bestGates[i]]);
					map.jumpgate[bestGates[i]].created = true;
				}
				targetSystems &= (~newLevel);
				levelSystems = newLevel;
			}
		}
		//attempt to connect all of the systems on the last level to one another
		if(levelSystems)
		{
			for(i = 0; i < map.numJumpGates; ++i)
			{
				if(!(map.jumpgate[i].created))
				{
					if(((0x01 << map.jumpgate[i].system1->index) & levelSystems) && ((0x01 << map.jumpgate[i].system2->index) & levelSystems))
					{
						if(!CrossesAnotherLink(map,& (map.jumpgate[i])))
						{
							map.jumpgate[i].system1->jumpgates[map.jumpgate[i].system1->jumpgateCount++] = &(map.jumpgate[i]);
							map.jumpgate[i].system2->jumpgates[map.jumpgate[i].system2->jumpgateCount++] = &(map.jumpgate[i]);
							map.jumpgate[i].created = true;
						}
					}
				}
			}
		}
		//make sure all players are connected
		U32 systemUnconnected = ~(0xFFFFFFFF << map.systemCount);
		U32 systemsVisited = 0;
		markSystems(systemUnconnected,map.systems,systemsVisited);
		while(systemUnconnected)
		{
			for(U32 i = 0; i < map.systemCount; ++i)
			{
				if((0x01 << i) & systemUnconnected)
				{
					bool breakOk = false;
					for(U32 j = 0; j < map.numJumpGates; ++j)
					{
						if(!map.jumpgate[j].created)
						{
							if( ( (map.jumpgate[j].system1->index == i) && (!((0x01 << map.jumpgate[j].system2->index) & systemUnconnected)) && (!(map.jumpgate[j].system2->playerID)) ) || 
								( (map.jumpgate[j].system2->index == i) && (!((0x01 << map.jumpgate[j].system1->index) & systemUnconnected)) && (!(map.jumpgate[j].system1->playerID)) ) )
							{
								if(!CrossesAnotherLink(map,& (map.jumpgate[j])))
								{
									if((!breakOk) || GetRand(1,10,MAP_GEN_ENUM::LINEAR) > 5)
									{
										map.jumpgate[j].system1->jumpgates[map.jumpgate[j].system1->jumpgateCount++] = &(map.jumpgate[j]);
										map.jumpgate[j].system2->jumpgates[map.jumpgate[j].system2->jumpgateCount++] = &(map.jumpgate[j]);
										map.jumpgate[j].created = true;
										breakOk = true;
									}
								}
							}
						}
					}
					if(breakOk)
						break;
				}
			}
			systemUnconnected = ~(0xFFFFFFFF << map.systemCount);
			systemsVisited = 0;
			markSystems(systemUnconnected,map.systems,systemsVisited);
		}
	}
}
//--------------------------------------------------------------------------//
//
void MapGen::createRandomGates(GenStruct & map)
{
	if(map.systemCount == map.numPlayers)
	{
		createRingGates(map);
	}
	else
	{
		//connect the players to unconnected systems if posible
		U32 i;
		for(i = 0; i <map.numPlayers; ++i)
		{
			GenJumpgate * bestJumpgate = NULL;
			for(U32 j = 0; j < map.numJumpGates; ++j)
			{
				GenJumpgate * jumpgate = &(map.jumpgate[j]);
				if((!jumpgate->created) && ((jumpgate->system1->playerID && (!jumpgate->system1->jumpgateCount) && (!jumpgate->system2->playerID)) 
						|| (jumpgate->system2->playerID && (!jumpgate->system2->jumpgateCount) && (!jumpgate->system1->playerID))) &&
						(!CrossesAnotherLink(map,jumpgate)))
				{
					if(bestJumpgate)
					{
						if(bestJumpgate->dist > jumpgate->dist)
						{
							bestJumpgate = jumpgate;
						}
					}
					else
					{
						bestJumpgate = jumpgate;
					}
				}
			}
			if(bestJumpgate)
			{
				bestJumpgate->system1->jumpgates[bestJumpgate->system1->jumpgateCount++] = bestJumpgate;
				bestJumpgate->system2->jumpgates[bestJumpgate->system2->jumpgateCount++] = bestJumpgate;
				bestJumpgate->created = true;
			}
		}
		//force player connections if nessesary;
		for(i = 0; i <map.numPlayers; ++i)
		{
			GenJumpgate * bestJumpgate = NULL;
			for(U32 j = 0; j < map.numJumpGates; ++j)
			{
				GenJumpgate * jumpgate = &(map.jumpgate[j]);
				if((!jumpgate->created) && ((jumpgate->system1->playerID && (!jumpgate->system1->jumpgateCount)) 
						|| (jumpgate->system2->playerID && (!jumpgate->system2->jumpgateCount))) &&
						(!CrossesAnotherLink(map,jumpgate)))
				{
					if(bestJumpgate)
					{
						if(bestJumpgate->dist > jumpgate->dist)
						{
							bestJumpgate = jumpgate;
						}
					}
					else
					{
						bestJumpgate = jumpgate;
					}
				}
			}
			if(bestJumpgate)
			{
				bestJumpgate->system1->jumpgates[bestJumpgate->system1->jumpgateCount++] = bestJumpgate;
				bestJumpgate->system2->jumpgates[bestJumpgate->system2->jumpgateCount++] = bestJumpgate;
				bestJumpgate->created = true;
			}
		}	

		//close graph
		U32 connectedSys = 0;
		U32 jumpgatesCreated = 0;
		while(jumpgatesCreated < (map.systemCount-map.numPlayers -1))
		{
			GenJumpgate * bestJumpgate = NULL;
			for(U32 s1 = 0; s1 < map.numJumpGates; ++s1)
			{
				GenJumpgate * jumpgate = &(map.jumpgate[s1]);
				if((!jumpgate->created) && (!jumpgate->system1->playerID) && (!jumpgate->system2->playerID) && (!CrossesAnotherLink(map,jumpgate)))
				{
					if(connectedSys)
					{
						bool check1 = (((0x01<<jumpgate->system1->index) & connectedSys) != 0);
						bool check2 = (((0x01<<jumpgate->system2->index) & connectedSys) != 0);
						if(check1 != check2)
						{
							if(bestJumpgate)
							{
								if(jumpgate->dist < bestJumpgate->dist)
									bestJumpgate = jumpgate;
							}
							else
							{
								bestJumpgate = jumpgate;
							}
						}
					}
					else
					{
						if(bestJumpgate)
						{
							if(jumpgate->dist < bestJumpgate->dist)
								bestJumpgate = jumpgate;
						}
						else
						{
							bestJumpgate = jumpgate;
						}
					}
				}
			}
			CQASSERT(bestJumpgate && "Could not close system map graph");
			connectedSys |= (0x01<<bestJumpgate->system1->index);
			connectedSys |= (0x01<<bestJumpgate->system2->index);
			bestJumpgate->system1->jumpgates[bestJumpgate->system1->jumpgateCount++] = bestJumpgate;
			bestJumpgate->system2->jumpgates[bestJumpgate->system2->jumpgateCount++] = bestJumpgate;
			bestJumpgate->created = true;
			jumpgatesCreated++;
		}

		//add some extra jumpgates
		bool bCreateFail = false;
		do
		{
			GenJumpgate * bestJumpgate = NULL;
			for(U32 s1 = 0; s1 < map.numJumpGates; ++s1)
			{
				GenJumpgate * jumpgate = &(map.jumpgate[s1]);
				if((!jumpgate->created) && (!jumpgate->system1->playerID) && (!jumpgate->system2->playerID) && (!CrossesAnotherLink(map,jumpgate)))
				{
					if(bestJumpgate)
					{
						if(GetRand(1,10,MAP_GEN_ENUM::LINEAR) >5)
							bestJumpgate = jumpgate;
					}
					else
					{
						bestJumpgate = jumpgate;
					}
				}
			}
			if(bestJumpgate)
			{
				bestJumpgate->system1->jumpgates[bestJumpgate->system1->jumpgateCount++] = bestJumpgate;
				bestJumpgate->system2->jumpgates[bestJumpgate->system2->jumpgateCount++] = bestJumpgate;
				bestJumpgate->created = true;
			}
			else
			{
				bCreateFail = true;
			}
		}while((!bCreateFail) && GetRand(1,10,MAP_GEN_ENUM::LINEAR) >5);
	}
}
//--------------------------------------------------------------------------//
//
void MapGen::createRingGates(GenStruct & map)
{
	for(U32 i = 0; i < map.systemCount; ++i)
	{
		U32 connectVal = map.systems[i].connectionOrder;
		U32 bestConnectVal = 0;
		GenJumpgate * bestJumpgate = NULL;
		for(U32 j = 0; j < map.numJumpGates; ++j)
		{
			GenJumpgate * jumpgate = &(map.jumpgate[j]);
			if(!jumpgate->created)
			{
				if(jumpgate->system1->index == i)
				{
					if(bestJumpgate)
					{
						if(connectVal < jumpgate->system2->connectionOrder)
						{
							if(bestConnectVal > connectVal)
							{
								if(bestConnectVal > jumpgate->system2->connectionOrder)
								{
									bestJumpgate = jumpgate;
									bestConnectVal = jumpgate->system2->connectionOrder;
								}
							}
							else
							{
								bestJumpgate = jumpgate;
								bestConnectVal = jumpgate->system2->connectionOrder;
							}
						}
						else if(bestConnectVal < connectVal)
						{
							if(jumpgate->system2->connectionOrder < bestConnectVal)
							{
								bestJumpgate = jumpgate;
								bestConnectVal = jumpgate->system2->connectionOrder;
							}
						}
					}
					else
					{
						bestJumpgate = jumpgate;
						bestConnectVal = jumpgate->system2->connectionOrder;
					}
				}
				else if(jumpgate->system2->index == i)
				{
					if(bestJumpgate)
					{
						if(connectVal < jumpgate->system1->connectionOrder)
						{
							if(bestConnectVal > connectVal)
							{
								if(bestConnectVal > jumpgate->system1->connectionOrder)
								{
									bestJumpgate = jumpgate;
									bestConnectVal = jumpgate->system1->connectionOrder;
								}
							}
							else
							{
								bestJumpgate = jumpgate;
								bestConnectVal = jumpgate->system1->connectionOrder;
							}
						}
						else if(bestConnectVal < connectVal)
						{
							if(jumpgate->system1->connectionOrder < bestConnectVal)
							{
								bestJumpgate = jumpgate;
								bestConnectVal = jumpgate->system1->connectionOrder;
							}
						}
					}
					else
					{
						bestJumpgate = jumpgate;
						bestConnectVal = jumpgate->system1->connectionOrder;
					}
				}
			}
		}
		if(bestJumpgate)
		{
			bestJumpgate->system1->jumpgates[bestJumpgate->system1->jumpgateCount++] = bestJumpgate;
			bestJumpgate->system2->jumpgates[bestJumpgate->system2->jumpgateCount++] = bestJumpgate;
			bestJumpgate->created = true;
		}
	}
}
//--------------------------------------------------------------------------//
//
void MapGen::createJumpgatesForIndexs(GenStruct & map,U32 index1, U32 index2)
{
	for(U32 i = 0; i < map.numJumpGates; ++i)
	{
		GenJumpgate * jumpgate = &(map.jumpgate[i]);
		if((index1 == jumpgate->system1->index && index2 == jumpgate->system2->index) ||
			(index2 == jumpgate->system1->index && index2 == jumpgate->system1->index))
		{
			jumpgate->system1->jumpgates[jumpgate->system1->jumpgateCount++] = jumpgate;
			jumpgate->system2->jumpgates[jumpgate->system2->jumpgateCount++] = jumpgate;
			jumpgate->created = true;
			return;
		}
	}
}
//--------------------------------------------------------------------------//
//
void MapGen::createStarGates(GenStruct & map)
{
	for(U32 i = 0; i < map.numPlayers; ++i)
	{
		U32 systemsPerPlayer = (map.systemsToMake-1)/map.numPlayers;
		createJumpgatesForIndexs(map,0,1+(i*systemsPerPlayer));
		if(systemsPerPlayer > 1)
		{
			createJumpgatesForIndexs(map,1+(i*systemsPerPlayer),2+(i*systemsPerPlayer));
		}
		if(systemsPerPlayer > 2)
		{
			createJumpgatesForIndexs(map,1+(i*systemsPerPlayer),3+(i*systemsPerPlayer));
		}
	}
}
//--------------------------------------------------------------------------//
//
void MapGen::PopulateSystems(GenStruct & map, IPANIM * ipAnim)
{
	U32 s1;
	GenSystem* system;

	// have to cut up 75% of map generation between the number of systems
	CQASSERT(map.systemCount > 0);
	SINGLE rate = 0.50f/map.systemCount;
	SINGLE progress = 0.25;

	for (s1 = 0; s1 < map.systemCount; s1++)
	{
		system = &(map.systems[s1]);
		
		PopulateSystem(map,system);

		if (ipAnim)
		{
			progress += rate;
			if (progress < 1.0f)
			{
				ipAnim->SetProgress(progress);	
			}
		}
	}
}
//--------------------------------------------------------------------------//
//
void MapGen::PopulateSystem(GenStruct & map,GenSystem * system)
{
	//habitable Planets
	U32 maxPlanetIndex = 0;
	U32 i;
	for(i = 0; i < MAX_TYPES;++i)
	{
		if(system->theme->habitablePlanets[i][0])
			++maxPlanetIndex;
		else
			break;
	}
	U32 numPlanets = system->theme->numHabitablePlanets[map.terrainSize];
	for(i = 0; i < numPlanets; ++i)
	{
		U32 posX,posY;
		bool bSuccess = FindPosition(system,4,MAP_GEN_ENUM::NO_OVERLAP,posX,posY);
		if(bSuccess)
		{
			if(system->numFlags < MAX_FLAGS)
			{
				system->flags[system->numFlags].xPos = posX+2;
				system->flags[system->numFlags].yPos = posY+2;
				system->flags[system->numFlags].type = FLAG_PLANET | FLAG_PATHON;
				system->numFlags++;
			}
			U32 planetID = GetRand(0,maxPlanetIndex-1,MAP_GEN_ENUM::LINEAR);
			insertObject(system->theme->habitablePlanets[planetID],Vector((posX+2)*GRIDSIZE,(posY+2)*GRIDSIZE,0),0,system->systemID,system);
			placePlanetsMoons(map,system,posX+2,posY+2);
			FillPosition(system,posX,posY,4,MAP_GEN_ENUM::NO_OVERLAP);
		}
	}

	//Ore Planets
	maxPlanetIndex = 0;
	for(i = 0; i < MAX_TYPES;++i)
	{
		if(system->theme->metalPlanets[i][0])
			++maxPlanetIndex;
		else
			break;
	}
	numPlanets = system->theme->numMetalPlanets[map.terrainSize];
	for(i = 0; i < numPlanets; ++i)
	{
		U32 posX,posY;
		bool bSuccess = FindPosition(system,4,MAP_GEN_ENUM::NO_OVERLAP,posX,posY);
		if(bSuccess)
		{
			if(system->numFlags < MAX_FLAGS)
			{
				system->flags[system->numFlags].xPos = posX+2;
				system->flags[system->numFlags].yPos = posY+2;
				system->flags[system->numFlags].type = FLAG_PLANET | FLAG_PATHON;
				system->numFlags++;
			}
			U32 planetID = GetRand(0,maxPlanetIndex-1,MAP_GEN_ENUM::LINEAR);
			insertObject(system->theme->metalPlanets[planetID],Vector((posX+2)*GRIDSIZE,(posY+2)*GRIDSIZE,0),0,system->systemID,system);
			placePlanetsMoons(map,system,posX+2,posY+2);
			FillPosition(system,posX,posY,4,MAP_GEN_ENUM::NO_OVERLAP);
		}
	}

	//gas Planets
	maxPlanetIndex = 0;
	for(i = 0; i < MAX_TYPES;++i)
	{
		if(system->theme->gasPlanets[i][0])
			++maxPlanetIndex;
		else
			break;
	}
	numPlanets = system->theme->numGasPlanets[map.terrainSize];
	for(i = 0; i < numPlanets; ++i)
	{
		U32 posX,posY;
		bool bSuccess = FindPosition(system,4,MAP_GEN_ENUM::NO_OVERLAP,posX,posY);
		if(bSuccess)
		{
			if(system->numFlags < MAX_FLAGS)
			{
				system->flags[system->numFlags].xPos = posX+2;
				system->flags[system->numFlags].yPos = posY+2;
				system->flags[system->numFlags].type = FLAG_PLANET | FLAG_PATHON;
				system->numFlags++;
			}
			U32 planetID = GetRand(0,maxPlanetIndex-1,MAP_GEN_ENUM::LINEAR);
			insertObject(system->theme->gasPlanets[planetID],Vector((posX+2)*GRIDSIZE,(posY+2)*GRIDSIZE,0),0,system->systemID,system);
			placePlanetsMoons(map,system,posX+2,posY+2);
			FillPosition(system,posX,posY,4,MAP_GEN_ENUM::NO_OVERLAP);
		}
	}
	
	//crew Planets
	maxPlanetIndex = 0;
	for(i = 0; i < MAX_TYPES;++i)
	{
		if(system->theme->otherPlanets[i][0])
			++maxPlanetIndex;
		else
			break;
	}
	numPlanets = system->theme->numOtherPlanets[map.terrainSize];
	for(i = 0; i < numPlanets; ++i)
	{
		U32 posX,posY;
		bool bSuccess = FindPosition(system,4,MAP_GEN_ENUM::NO_OVERLAP,posX,posY);
		if(bSuccess)
		{
			if(system->numFlags < MAX_FLAGS)
			{
				system->flags[system->numFlags].xPos = posX+2;
				system->flags[system->numFlags].yPos = posY+2;
				system->flags[system->numFlags].type = FLAG_PLANET | FLAG_PATHON;
				system->numFlags++;
			}
			U32 planetID = GetRand(0,maxPlanetIndex-1,MAP_GEN_ENUM::LINEAR);
			insertObject(system->theme->otherPlanets[planetID],Vector((posX+2)*GRIDSIZE,(posY+2)*GRIDSIZE,0),0,system->systemID,system);
			placePlanetsMoons(map,system,posX+2,posY+2);
			FillPosition(system,posX,posY,4,MAP_GEN_ENUM::NO_OVERLAP);
		}
	}

	BuildPaths(system);
	GenerateTerain(map,system);
}
//--------------------------------------------------------------------------//
//
#define NUM_MOON_POINTS 16

POINT moonPlaces[] = {{-3,-2},{-3,-1},{-3,0},{-3,1},
						{2,-2},{2,-1},{2,0},{2,1},
						{-2,-3},{-1,-3},{0,-3},{1,-3},
						{-2,2},{-1,2},{0,2},{1,2}};

void MapGen::placePlanetsMoons(GenStruct & map, GenSystem * system,U32 planetPosX,U32 planetPosY)
{
	U32 maxMoonIndex = 0;
	for(U32 i = 0; i < MAX_TYPES;++i)
	{
		if(system->theme->moonTypes[i][0])
			++maxMoonIndex;
		else
			break;
	}
	U32 numMoons = GetRand(system->theme->minMoonsPerPlanet,system->theme->maxMoonsPerPlanet,system->theme->moonNumberFunc);
	while(numMoons)
	{
		//find a good place near our planet
		U32 numPos = 0;
		U32 index;
		for(index = 0; index< NUM_MOON_POINTS;++index)
		{
			if(SpaceEmpty(system,planetPosX+moonPlaces[index].x-1,planetPosY+moonPlaces[index].y-1,MAP_GEN_ENUM::NO_OVERLAP,3))
				++numPos;
		}
		if(!numPos)
			return;
		U32 t = GetRand(0,numPos-1,MAP_GEN_ENUM::LINEAR);
		for(index = 0; index< NUM_MOON_POINTS;++index)
		{
			if(SpaceEmpty(system,planetPosX+moonPlaces[index].x-1,planetPosY+moonPlaces[index].y-1,MAP_GEN_ENUM::NO_OVERLAP,3))
			{
				if(!t)
				{
					//place the moon
					FillPosition(system,planetPosX+moonPlaces[index].x-1,planetPosY+moonPlaces[index].y-1,3,MAP_GEN_ENUM::NO_OVERLAP);
					U32 xPos = planetPosX+moonPlaces[index].x;
					U32 yPos = planetPosY+moonPlaces[index].y;
					U32 moonID = GetRand(0,maxMoonIndex-1,MAP_GEN_ENUM::LINEAR);
					insertObject(system->theme->moonTypes[moonID],Vector(((xPos)*GRIDSIZE)+(0.5*GRIDSIZE),((yPos)*GRIDSIZE)+(0.5*GRIDSIZE),0),0,system->systemID,system);

					break;
				}
				--t;
			}
		}
		--numMoons;
	}

}
//--------------------------------------------------------------------------//
//
bool MapGen::FindPosition(GenSystem * system,U32 width,MAP_GEN_ENUM::OVERLAP overlap, U32 & xPos, U32 & yPos)
{
	U32 numPos = 0;
	U32 ix;
	for(ix = 0; ix< system->size;++ix)
	{
		for(U32 iy = 0; iy<system->size;++iy)
		{
			if(SpaceEmpty(system,ix,iy,overlap,width))
				++numPos;
		}
	}
	if(!numPos)
		return false;
	U32 t = GetRand(0,numPos-1,MAP_GEN_ENUM::LINEAR);
	for(ix = 0; ix< system->size-width+1;++ix)
	{
		for(U32 iy = 0; iy<system->size-width+1;++iy)
		{
			if(SpaceEmpty(system,ix,iy,overlap,width))
			{
				if(!t)
				{
					xPos = ix;
					yPos = iy;
					CQASSERT(xPos+width-1 < system->size);
					CQASSERT(yPos+width-1 < system->size);
					return true;
				}
				--t;
			}
		}
	}
	return false;
}
//--------------------------------------------------------------------------//
//
/*bool MapGen::ColisionWithObject(GenObj * obj,Vector vect,U32 rad,MAP_GEN_ENUM::OVERLAP overlap)
{
	if(overlap == MAP_GEN_ENUM::LEVEL1 && (obj->overlap == MAP_GEN_ENUM::LEVEL1 || obj->overlap == MAP_GEN_ENUM::LEVEL2))
		return false;
	else if(overlap == MAP_GEN_ENUM::LEVEL2 && obj->overlap == MAP_GEN_ENUM::LEVEL1)
		return false;
	else
	{
		Vector objPos(obj->x,obj->y,0);
		if((objPos-vect).magnitude() > rad+obj->rad)
			return false;
	}
	return true;
}
*/
//--------------------------------------------------------------------------//
//
void MapGen::GenerateTerain(GenStruct & map, GenSystem * system)
{
	//select system Kit
	U32 numTypes = 0;
#ifndef _DEMO_
	while(system->theme->systemKit[numTypes][0] && numTypes <MAX_TYPES)
	{
		++numTypes;
	}
	if(numTypes)
	{
		SECTOR->SetLightingKit(system->systemID,system->theme->systemKit[GetRand(0,numTypes-1,MAP_GEN_ENUM::LINEAR)]);
	}
#else  //only use one background for the demo
	SECTOR->SetLightingKit(system->systemID,"terran1");
#endif
	
	//gas Nebulas
	S32 resourceAmount = system->theme->numNuggetPatchesGas[map.terrainSize];
	numTypes = 0;
	while(system->theme->nuggetGasTypes[numTypes].terrainArchType[0] && numTypes <MAX_TYPES)
	{
		++numTypes;
	}
	if(numTypes)
	{
		while(resourceAmount > 0)
		{
			U32 pos = GetRand(0,numTypes-1,MAP_GEN_ENUM::LINEAR);
			PlaceTerrain(map,system->theme->nuggetGasTypes[pos],system);
			--resourceAmount;
		}
	}

	//ore nebulas
	resourceAmount = system->theme->numNuggetPatchesMetal[map.terrainSize];
	numTypes = 0;
	while(system->theme->nuggetMetalTypes[numTypes].terrainArchType[0] && numTypes <MAX_TYPES)
	{
		++numTypes;
	}
	if(numTypes)
	{
		while(resourceAmount > 0)
		{
			U32 pos = GetRand(0,numTypes-1,MAP_GEN_ENUM::LINEAR);
			PlaceTerrain(map,system->theme->nuggetMetalTypes[pos],system);
			--resourceAmount;
		}
	}

	U32 typeProb[MAX_TERRAIN];
	//find number of terrain types in theme and place required terrain
	numTypes = 0;
	U32 totalProb = 0;
	U32 totalPlaced = 0;
	while(system->theme->terrain[numTypes].terrainArchType[0] && numTypes <MAX_TERRAIN)
	{
		U32 numPlaced = 0;
		while(numPlaced < system->theme->terrain[numTypes].requiredToPlace)
		{
			++numPlaced;
			++totalPlaced;
			PlaceTerrain(map,system->theme->terrain[numTypes],system);
		}
		typeProb[numTypes] = (U32)((system->theme->terrain[numTypes].probability) * 10000);
		totalProb += typeProb[numTypes];
		++numTypes;
	}

	if((!numTypes) || (!totalProb))
		return;
	//now generate the rest of the terrain
	while(system->theme->desitiy[map.terrainSize] > (system->omUsed/((SINGLE)(system->omStartEmpty))))
	{
		U32 prob = GetRand(0,totalProb,MAP_GEN_ENUM::LINEAR);
		U32 index = 0;
		while(index < numTypes)
		{
			if(typeProb[index] >= prob)
				break;
			else
				prob -= typeProb[index];
			++index;
		}
		CQASSERT(index < numTypes);

		PlaceTerrain(map,system->theme->terrain[index],system);
	}
}
//--------------------------------------------------------------------------//
//
void MapGen::PlaceTerrain(GenStruct & map,BT_MAP_GEN::_terrainTheme::_terrainInfo terrain,GenSystem *system)
{
	//special types to place
		//field
		//antimatter ribbons

	BASIC_DATA * data = (BASIC_DATA *)( ARCHLIST->GetArchetypeData(terrain.terrainArchType));
	if(!data)
	{
		CQBOMB1("Bad archtype name in random map generator.  Fix the data not a code bug, Name:%s",terrain.terrainArchType);
	}
	if((data->objClass == OC_NEBULA) || (data->objClass == OC_FIELD))
	{
		BASE_FIELD_DATA * fData = (BASE_FIELD_DATA *) data;
		if(fData->fieldClass == FC_ANTIMATTER)
		{
			switch(terrain.placement)
			{
			case MAP_GEN_ENUM::RANDOM:
				{
					U32 length = GetRand(terrain.minToPlace,terrain.maxToPlace,terrain.numberFunc);
					PlaceRandomRibbon(&terrain,length,0,0,system);
				}
				break;
			case MAP_GEN_ENUM::SPOTS:
				CQASSERT(0 && "NOT SUPPORTED");
				break;
			case MAP_GEN_ENUM::CLUSTER:
				CQASSERT(0 && "NOT SUPPORTED");
				break;
			case MAP_GEN_ENUM::PLANET_RING:
				CQASSERT(0 && "NOT SUPPORTED");
				break;
			case MAP_GEN_ENUM::STREEKS:
				CQASSERT(0 && "NOT SUPPORTED");
				break;
			}
		}
		else  //a regular nebula
		{
			switch(terrain.placement)
			{
			case MAP_GEN_ENUM::RANDOM:
				{
					U32 numToPlace = GetRand(terrain.minToPlace,terrain.maxToPlace,terrain.numberFunc);
					PlaceRandomField(&terrain,numToPlace,0,0,system);
				}
				break;
			case MAP_GEN_ENUM::SPOTS:
				{
					U32 numToPlace = GetRand(terrain.minToPlace,terrain.maxToPlace,terrain.numberFunc);
					PlaceSpottyField(&terrain,numToPlace,0,0,system);
				}
				break;
			case MAP_GEN_ENUM::CLUSTER:
				CQASSERT(0 && "NOT SUPPORTED");
				break;
			case MAP_GEN_ENUM::PLANET_RING:
				{
					PlaceRingField(&terrain,system);
				}
				break;
			case MAP_GEN_ENUM::STREEKS:
				CQASSERT(0 && "NOT SUPPORTED");
				break;
			}
		}
	}
	else // it is a regular object
	{
		switch(terrain.placement)
		{
		case MAP_GEN_ENUM::RANDOM:
			{
				U32 numToPlace = GetRand(terrain.minToPlace,terrain.maxToPlace,terrain.numberFunc);
				for(U32 i = 0; i < numToPlace; ++i)
				{				
					U32 xPos,yPos;
					bool bSuccess = FindPosition(system,terrain.size,terrain.overlap,xPos,yPos);
					if(bSuccess)
					{
						FillPosition(system,xPos,yPos,terrain.size,terrain.overlap);
						U32 halfWidth = (GRIDSIZE*terrain.size)/2;
						insertObject(terrain.terrainArchType,Vector(xPos*GRIDSIZE+halfWidth,yPos*GRIDSIZE+halfWidth,0),0,system->systemID,system);
					}
					else
					{
						system->omUsed += terrain.size*terrain.size;
					}
				}
			}
			break;
		case MAP_GEN_ENUM::SPOTS:
			CQASSERT(0 && "NOT SUPPORTED");
			break;
		case MAP_GEN_ENUM::CLUSTER:
			{
				U32 numToPlace = GetRand(terrain.minToPlace,terrain.maxToPlace,terrain.numberFunc);
				U32 xPos,yPos;
				bool bSuccess = FindPosition(system,terrain.size,terrain.overlap,xPos,yPos);
				if(bSuccess)
				{
					for(U32 i = 0; i < numToPlace; ++i)
					{	
						U32 angle = GetRand(0,360,MAP_GEN_ENUM::LINEAR);
						U32 dist = GetRand(0,terrain.size*(GRIDSIZE/2),MAP_GEN_ENUM::LINEAR);
						Vector position = Vector(xPos*GRIDSIZE+(GRIDSIZE/2),yPos*GRIDSIZE+(GRIDSIZE/2),0) +Vector(cos(angle*MUL_DEG_TO_RAD)*dist,sin(angle*MUL_DEG_TO_RAD)*dist,0);
						insertObject(terrain.terrainArchType,position,0,system->systemID,system);
					}
					FillPosition(system,xPos,yPos,terrain.size,terrain.overlap);
				}
				else
				{
					system->omUsed = terrain.size*terrain.size;
				}
			}
			break;
		case MAP_GEN_ENUM::PLANET_RING:
			CQASSERT(0 && "NOT SUPPORTED");
			break;
		case MAP_GEN_ENUM::STREEKS:
			CQASSERT(0 && "NOT SUPPORTED");
			break;
		}
	}
	
}
//--------------------------------------------------------------------------//
//
void MapGen::BuildPaths(GenSystem * system)
{
	FlagPost * post1 = NULL;
	for(U32 i = 0; i < system->numFlags;++i)
	{
		if(system->flags[i].type & FLAG_PATHON)
		{
			if(post1)
			{
				connectPosts(post1,&(system->flags[i]),system);
				post1 = &(system->flags[i]);
			}
			else
			{
				post1 = &(system->flags[i]);
			}
		}
	}
}
//--------------------------------------------------------------------------//
//
void MapGen::connectPosts(FlagPost * post1, FlagPost * post2,GenSystem * system)
{
	U32 xPos = post1->xPos;
	U32 yPos = post1->yPos;
	while(1)
	{
		if(system->objectMap[xPos][yPos] != GENMAP_TAKEN)
			system->objectMap[xPos][yPos] = GENMAP_PATH;
		if(xPos == post2->xPos)
		{
			if(yPos == post2->yPos)
			{
				return;
			}
			else
			{
				if(yPos > post2->yPos)
				{
					U32 rVal = rand()%4;
					if(rVal == 0)
						++xPos;
					else if(rVal == 1)
						--xPos;
					else
						--yPos;
				}
				else
				{
					U32 rVal = rand()%4;
					if(rVal == 0)
						++xPos;
					else if(rVal == 1)
						--xPos;
					else
						++yPos;
				}
			}
		}
		else if(yPos == post2->yPos)
		{
			if(xPos > post2->xPos)
			{
				U32 rVal = rand()%4;
				if(rVal == 0)
					++yPos;
				else if(rVal == 1)
					--yPos;
				else
					--xPos;
			}
			else
			{
				U32 rVal = rand()%4;
				if(rVal == 0)
					++yPos;
				else if(rVal == 1)
					--yPos;
				else
					++xPos;
			}
		}
		else
		{
			if(xPos > post2->xPos)
			{
				if(yPos > post2->xPos)
				{
					U32 rVal = rand()%2;
					if(rVal == 0)
						--yPos;
					else
						--xPos;
				}
				else
				{
					U32 rVal = rand()%2;
					if(rVal == 0)
						++yPos;
					else
						--xPos;
				}
			}
			else
			{
				if(yPos > post2->xPos)
				{
					U32 rVal = rand()%2;
					if(rVal == 0)
						--yPos;
					else
						++xPos;
				}
				else
				{
					U32 rVal = rand()%2;
					if(rVal == 0)
						++yPos;
					else
						++xPos;
				}
			}
		}
	}
}
//--------------------------------------------------------------------------//
//
void MapGen::removeFromArray(U32 nx,U32 ny,U32 * tempX,U32 * tempY,U32 & tempIndex)
{
	U32 skip = 0;
	for(U32 index = 0; index < tempIndex; ++index)
	{
		if(tempX[index] == nx && tempY[index] == ny)
			++skip;
		else if(skip)
		{
			tempX[index-skip] = tempX[index];
			tempY[index-skip] = tempY[index];
		}
	}
	tempIndex -= skip;
}
//--------------------------------------------------------------------------//
//
bool MapGen::isInArray(U32 * arrX, U32 * arrY, U32 index, U32 nx, U32 ny)
{
	while(index)
	{
		--index;
		if((arrX[index] == nx) && (arrY[index] == ny))
			return true;
	}
	return false;
}
//--------------------------------------------------------------------------//
//
void MapGen::checkNewXY(U32 * tempX,U32 * tempY,U32 & tempIndex, U32 * finalX, U32 * finalY, U32 finalIndex, BT_MAP_GEN::_terrainTheme::_terrainInfo terrain,
				GenSystem * system,U32 newX,U32 newY)
{
	if(newX < system->size && newY < system->size)
	{
		if(tempIndex < 256)
		{
			if(!(isInArray(finalX,finalY,finalIndex,newX,newY)))
			{
				if(SpaceEmpty(system,newX,newY,terrain.overlap,1))
				{
					tempX[tempIndex] = newX;
					tempY[tempIndex] = newY;
					++tempIndex;
				}
			}
		}
	}
}
//--------------------------------------------------------------------------//
//
void MapGen::PlaceRandomField(BT_MAP_GEN::_terrainTheme::_terrainInfo * terrain,U32 numToPlace,S32 startX, S32 startY,GenSystem * system)
{
	CQASSERT(numToPlace < 256 && "If you are getting this I can make the number bigger");
	U32 tempX[256];
	U32 tempY[256];
	U32 tempIndex = 0;
	U32 finalX[256];
	U32 finalY[256];
	U32 finalIndex = 1;
	if(startX == 0 && startY == 0)
	{
		bool bSucess = FindPosition(system,1,terrain->overlap,finalX[0],finalY[0]);
		if(!bSucess)
		{
			system->omUsed += numToPlace;
			return;
		}
	}
	else
	{
		finalX[0] = startX;
		finalY[0] = startY;
	}
	U32 lastFinal = 0;
	U32 numMade = 1;
	while(numMade < numToPlace)
	{
		lastFinal = finalIndex-1;
		checkNewXY(tempX,tempY,tempIndex,finalX,finalY,finalIndex,*terrain,system,finalX[lastFinal]+1,finalY[lastFinal]);
		checkNewXY(tempX,tempY,tempIndex,finalX,finalY,finalIndex,*terrain,system,finalX[lastFinal],finalY[lastFinal]+1);
		if(finalX[lastFinal])
			checkNewXY(tempX,tempY,tempIndex,finalX,finalY,finalIndex,*terrain,system,finalX[lastFinal]-1,finalY[lastFinal]);
		if(finalY[lastFinal])
			checkNewXY(tempX,tempY,tempIndex,finalX,finalY,finalIndex,*terrain,system,finalX[lastFinal],finalY[lastFinal]-1);

		if(tempIndex == 0)
			return;
		U32 newIndex = GetRand(0,tempIndex-1,MAP_GEN_ENUM::LINEAR);
		finalX[finalIndex] = tempX[newIndex];
		finalY[finalIndex] = tempY[newIndex];

		removeFromArray(finalX[finalIndex],finalY[finalIndex],tempX,tempY,tempIndex);

		++finalIndex;
		++numMade;
	}

	U32 i;
	for(i = 0; i < numMade; ++i)
	{
		FillPosition(system,finalX[i],finalY[i],1,terrain->overlap);
	}

	for(i = 0; i < finalIndex;++i)
	{
		finalX[i] = finalX[i]*GRIDSIZE+(GRIDSIZE/2);
		finalY[i] = finalY[i]*GRIDSIZE+(GRIDSIZE/2);
	}

	FIELDMGR->CreateField(terrain->terrainArchType,(S32 *)finalX,(S32 *)finalY,finalIndex,system->systemID);
}
//--------------------------------------------------------------------------//
//
void MapGen::PlaceSpottyField(BT_MAP_GEN::_terrainTheme::_terrainInfo * terrain,U32 numToPlace,S32 startX, S32 startY,GenSystem * system)
{
	numToPlace = numToPlace*2;
	CQASSERT(numToPlace < 256 && "If you are getting this I can make the number bigger");
	U32 tempX[256];
	U32 tempY[256];
	U32 tempIndex = 0;
	U32 finalX[256];
	U32 finalY[256];
	U32 finalIndex = 1;
	if(startX == 0 && startY == 0)
	{
		bool bSucess = FindPosition(system,1,terrain->overlap,finalX[0],finalY[0]);
		if(!bSucess)
		{
			system->omUsed += numToPlace;
			return;
		}
	}
	else
	{
		finalX[0] = startX;
		finalY[0] = startY;
	}
	U32 lastFinal = 0;
	U32 numMade = 1;
	while(numMade < numToPlace)
	{
		lastFinal = finalIndex-1;
		checkNewXY(tempX,tempY,tempIndex,finalX,finalY,finalIndex,*terrain,system,finalX[lastFinal]+1,finalY[lastFinal]);
		checkNewXY(tempX,tempY,tempIndex,finalX,finalY,finalIndex,*terrain,system,finalX[lastFinal],finalY[lastFinal]+1);
		if(finalX[lastFinal])
			checkNewXY(tempX,tempY,tempIndex,finalX,finalY,finalIndex,*terrain,system,finalX[lastFinal]-1,finalY[lastFinal]);
		if(finalY[lastFinal])
			checkNewXY(tempX,tempY,tempIndex,finalX,finalY,finalIndex,*terrain,system,finalX[lastFinal],finalY[lastFinal]-1);

		if(tempIndex == 0)
			return;
		U32 newIndex = GetRand(0,tempIndex-1,MAP_GEN_ENUM::LINEAR);
		finalX[finalIndex] = tempX[newIndex];
		finalY[finalIndex] = tempY[newIndex];

		removeFromArray(finalX[finalIndex],finalY[finalIndex],tempX,tempY,tempIndex);

		++finalIndex;
		++numMade;
	}

	for(U32 count = 0; count < (finalIndex/2); ++count)
	{
		finalX[count] = finalX[(count*2)+1];
		finalY[count] = finalY[(count*2)+1];
	}
	finalIndex = finalIndex/2;
	U32 i;
	for(i = 0; i < numMade; ++i)
	{
		FillPosition(system,finalX[i],finalY[i],1,terrain->overlap);
	}

	for(i = 0; i < finalIndex;++i)
	{
		finalX[i] = finalX[i]*GRIDSIZE+(GRIDSIZE/2);
		finalY[i] = finalY[i]*GRIDSIZE+(GRIDSIZE/2);
	}

	FIELDMGR->CreateField(terrain->terrainArchType,(S32 *)finalX,(S32 *)finalY,finalIndex,system->systemID);
}
//--------------------------------------------------------------------------//
//
void MapGen::PlaceRingField(BT_MAP_GEN::_terrainTheme::_terrainInfo * terrain,GenSystem *system)
{
	bool hasCenter = false;
	U32 centerX = 0;
	U32 centerY = 0;

	U32 i;
	for(i = 0; i < system->numFlags;++i)
	{
		if(system->flags[i].type & FLAG_PLANET)
		{
			if(hasCenter)
			{
				if(GetRand(1,10,MAP_GEN_ENUM::LINEAR) > 5)
				{
					centerX = system->flags[i].xPos;
					centerY = system->flags[i].yPos;
				}
			}
			else
			{
				hasCenter = true;
				centerX = system->flags[i].xPos;
				centerY = system->flags[i].yPos;
			}
		}
	}
	if(!hasCenter)
	{
		system->omUsed += terrain->minToPlace;
		return;
	}

	U32 numToPlace = GetRand(terrain->minToPlace,terrain->maxToPlace,terrain->numberFunc);
	CQASSERT(numToPlace < 256 && "If you are getting this I can make the number bigger");
	U32 finalX[256];
	U32 finalY[256];
	U32 finalIndex = 0;
	U32 width = (numToPlace/(6*terrain->size));
	for(i = 0; i < numToPlace*3; ++i)
	{
		S32 x,y;
		if(GetRand(1,10,MAP_GEN_ENUM::LINEAR) > 5)
		{
			x = GetRand(0,terrain->size,MAP_GEN_ENUM::LINEAR);
			y = F2LONG(sqrt((float)((terrain->size*terrain->size) - (x*x)))) + GetRand(0,width,MAP_GEN_ENUM::LINEAR);
		}
		else
		{
			y = GetRand(0,terrain->size,MAP_GEN_ENUM::LINEAR);
			x = F2LONG(sqrt((float)((terrain->size*terrain->size) - (y*y)))) + GetRand(0,width,MAP_GEN_ENUM::LINEAR);
		}
		if(GetRand(1,10,MAP_GEN_ENUM::LINEAR) > 5)
			x = centerX+x;
		else
			x = centerX-x;
		if(GetRand(1,10,MAP_GEN_ENUM::LINEAR) > 5)
			y = centerY+y;
		else
			y = centerY-y;
		if((!isInArray(finalX,finalY,finalIndex,x,y)) && SpaceEmpty(system,x,y,terrain->overlap,1))
		{
			finalX[finalIndex] = x;
			finalY[finalIndex] = y;
			++finalIndex;
			if(finalIndex >= numToPlace)
			{
				break;
			}
		}
	}
	if(!finalIndex)
	{
		system->omUsed += terrain->minToPlace;
		return;
	}
	for(i = 0; i < finalIndex; ++i)
	{
		FillPosition(system,finalX[i],finalY[i],1,terrain->overlap);
	}

	for(i = 0; i < finalIndex;++i)
	{
		finalX[i] = finalX[i]*GRIDSIZE+(GRIDSIZE/2);
		finalY[i] = finalY[i]*GRIDSIZE+(GRIDSIZE/2);
	}

	FIELDMGR->CreateField(terrain->terrainArchType,(S32 *)finalX,(S32 *)finalY,finalIndex,system->systemID);

}
//--------------------------------------------------------------------------//
//
void MapGen::PlaceRandomRibbon(BT_MAP_GEN::_terrainTheme::_terrainInfo * terrain,U32 length,S32 startX, S32 startY,GenSystem * system)
{
	CQASSERT(length < 256 && "If you are getting this I can make the number bigger");
	U32 tempX[256];
	U32 tempY[256];
	U32 tempIndex = 0;
	U32 finalX[256];
	U32 finalY[256];
	U32 finalIndex = 1;
	if(startX == 0 && startY == 0)
	{
		bool bSucess = FindPosition(system,1,terrain->overlap,finalX[0],finalY[0]);
		if(!bSucess)
		{
			system->omUsed += length;
			return;
		}
	}
	else
	{
		finalX[0] = startX;
		finalY[0] = startY;
	}
	U32 lastFinal = 0;
	U32 numMade = 1;
	while(numMade < length)
	{
		lastFinal = finalIndex-1;
		tempIndex = 0;
		checkNewXY(tempX,tempY,tempIndex,finalX,finalY,finalIndex,*terrain,system,finalX[lastFinal]+1,finalY[lastFinal]);
		checkNewXY(tempX,tempY,tempIndex,finalX,finalY,finalIndex,*terrain,system,finalX[lastFinal],finalY[lastFinal]+1);
		if(finalX[lastFinal])
			checkNewXY(tempX,tempY,tempIndex,finalX,finalY,finalIndex,*terrain,system,finalX[lastFinal]-1,finalY[lastFinal]);
		if(finalY[lastFinal])
			checkNewXY(tempX,tempY,tempIndex,finalX,finalY,finalIndex,*terrain,system,finalX[lastFinal],finalY[lastFinal]-1);

		if(tempIndex == 0)
			return;
		U32 newIndex = GetRand(0,tempIndex-1,MAP_GEN_ENUM::LINEAR);
		finalX[finalIndex] = tempX[newIndex];
		finalY[finalIndex] = tempY[newIndex];

		++finalIndex;
		++numMade;
	}

	if(numMade < 2)
	{
		system->omUsed += length;
		return;
	}
	U32 i ;
	for(i = 0; i < numMade; ++i)
	{
		FillPosition(system,finalX[i],finalY[i],1,terrain->overlap);
	}

	for(i = 0; i < finalIndex;++i)
	{
		finalX[i] = finalX[i]*GRIDSIZE+(GRIDSIZE/2);
		finalY[i] = finalY[i]*GRIDSIZE+(GRIDSIZE/2);
	}

	FIELDMGR->CreateField(terrain->terrainArchType,(S32 *)finalX,(S32 *)finalY,finalIndex,system->systemID);
}
//--------------------------------------------------------------------------//
//
void MapGen::init (void)
{
}

//--------------------------------------------------------------------------//
//
struct _mapGen : GlobalComponent
{
	MapGen * mapGen;

	virtual void Startup (void)
	{
		MAPGEN = mapGen = new DAComponent<MapGen>;
		AddToGlobalCleanupList(&mapGen);
	}

	virtual void Initialize (void)
	{
		COMPTR<IDAConnectionPoint> connection;
		
		mapGen->init();
	}
};

static _mapGen globalGenerator;

//--------------------------------------------------------------------------//
//-----------------------------End MapGen.cpp-------------------------------//
//--------------------------------------------------------------------------//
