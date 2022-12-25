//--------------------------------------------------------------------------------//
//                                                                                //
//                                 MSwarm.cpp			                          //
//                                                                                //
//                  COPYRIGHT (C) 2000 BY DIGITAL ANVIL, INC.                     //
//                                                                                //
//--------------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/MSwarm.cpp 43    10/24/00 6:37p Jasony $
*/			    
//--------------------------------------------------------------------------------//

#include "pch.h"
#include <globals.h>
#include "MGlobals.h"
#include <CQTrace.h>

#include "SPlayerAI.h"
#include "ObjList.h"
#include "IObject.h"
#include "MPart.h"
#include "Resource.h"
#include "BaseHotrect.h"
#include "OpAgent.h"
#include "Mpart.h"
#include "IPlanet.h"
#include "Commpacket.h"
#include "ObjSet.h"
#include "Sector.h"
#include "GridVector.h"

#include <DPlatform.h>

#include <Eventsys.h>
#include <Heapobj.h>
#include <TSmartPointer.h>

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
/*

  Hardcoded traits:


*/
//--------------------------------------------------------------------------//
//-------------------------------------------
struct MSwarm : public DAComponent<SPlayerAI>
{
	/* mantis AI data items */
	

	
	/* end of data items */
	
	virtual void	init (U32 playerID, M_RACE race);
	virtual void	initBuildDesires(void);
	virtual void	initResearchDesires(bool bFirstTime = false);
	virtual void	initShipDesires(void);

	//virtual void	onIdleUnit (IBaseObject * obj);   //using parent method
	
	/* MSwarm methods */
	//virtual void			doSupplyShip (MPart & part);
	//virtual void			doFabricator (MPart & part);
	//virtual void			doHQ (MPart & part);
	//virtual void			doLight (MPart & part);
	//virtual void			doHeavyInd (MPart & part);
	//virtual void			assignGunboat (MPart & part);
};
//--------------------------------------------------------------------------//
//
void MSwarm::init (U32 _playerID, M_RACE race)
{
	SPlayerAI::init(_playerID, race);

	CQASSERT(race == M_MANTIS);
	strategy = MANTIS_SWARM;

	AIPersonality settings = DNA;
	settings.buildMask.bSendTaunts = true;
	settings.buildMask.bResignationPossible = true;
	settings.buildMask.bVisibilityRules = true;
	settings.nNumFleets = 2;
	settings.nHarvestEscorts = 0;		//number of gunboats to use to escort each harvester
	settings.uNumScouts = 2 + (SECTOR->GetNumSystems() / 4);
	settings.nNumTroopships = rand() & 1;  //could be based on difficulty level  fix  
	settings.nNumMinelayers = rand() & 1;  //could be based on difficulty level  fix  
	settings.nGunboatsPerSupplyShip = 6;
	settings.nBuildPatience = 3;
	settings.nNumFabricators = 2;
	settings.nFabricateEscorts = 1;
	SetPersonality(settings);
	
	m_AttackWaveSize = MAX_SELECTED_UNITS - (2 + (rand() & 15));

	//
	// create a scouting assignment, then attack
	//
	if(DNA.buildMask.bScout)
	{
		pAssignments = new ASSIGNMENT;
		pAssignments->Init();
		pAssignments->type = SCOUT;
	}
}
//--------------------------------------------------------------------------//
// initializing build affinities data, finer tuning in other functions
//--------------------------------------------------------------------------//
void MSwarm::initBuildDesires(void)
{
	for(int c = 0; c < M_ENDOBJCLASS; c++)
	{
		BuildPriority * cur = &BuildDesires[c];

		//defaults
		cur->nPriority = -10000; // +rand() & 7
		cur->nPlanetMultiplier = 0;
		cur->nSystemMultiplier = 0;
		cur->nAdditiveDecrement = 50;  // + rand() & 7
		cur->nExponentialDecrement = 0;
		cur->prerequisite = M_NONE;
//		cur->nNumSlots = 0;
		cur->bMutation = false;

		if(GetRace(c) != m_nRace)
		{
			cur->nPriority = -10000;
			continue;
		}

		if(c == M_NONE)
		{
			cur->nPriority = -10000;
			cur->nPlanetMultiplier = 0;
			cur->nAdditiveDecrement = 0;
			cur->nExponentialDecrement = 0;
			cur->prerequisite = M_NONE;
//			cur->nNumSlots = 0;
			cur->bMutation = false;
		}

		if(c == M_COCOON)  
		{
			cur->nPriority = 1600;
			cur->nPlanetMultiplier = 0;
			cur->nSystemMultiplier = 0;
			cur->nAdditiveDecrement = 150;
			//cur->nExponentialDecrement = 10;
			cur->nExponentialDecrement = 8;
//			cur->nNumSlots = 3;
		}

		if(c == M_COLLECTOR)
		{
			cur->nPriority = 500;
			cur->nPlanetMultiplier = 1;
			cur->nAdditiveDecrement = 175;
			cur->nExponentialDecrement = 0;
			cur->prerequisite = M_COCOON;
//			cur->nNumSlots = 3;
		}

		if(c == M_GREATER_COLLECTOR)
		{
			cur->nPriority = 560;
			cur->nPlanetMultiplier = 0;
			cur->nAdditiveDecrement = 195;
			cur->nExponentialDecrement = 0;
			cur->prerequisite = M_COLLECTOR;
//			cur->nNumSlots = 3;
			cur->bMutation = true;
		}

		if(c == M_GREATER_PLANTATION)
		{
			cur->nPriority = 350;
			cur->nPlanetMultiplier = 0;
			cur->nAdditiveDecrement = 195;
			cur->nExponentialDecrement = 0;
			cur->prerequisite = M_PLANTATION;
		//	cur->nNumSlots = 3;
			cur->bMutation = true;
		}
		
		if(c == M_THRIPID)
		{
			cur->nPriority = 600;
			cur->nPlanetMultiplier = 0;
			cur->nAdditiveDecrement = 150;
			cur->nExponentialDecrement = 0;
			cur->prerequisite = M_COLLECTOR;
//			cur->nNumSlots = 4;
		}

		if(c == M_PLANTATION)
		{
			cur->nPriority = 450;
			cur->nPlanetMultiplier = 0;
			cur->nAdditiveDecrement = 0;
			cur->nExponentialDecrement = 3;
			cur->prerequisite = M_THRIPID;
//			cur->nNumSlots = 2;
		}

		if(c == M_EYESTOCK)
		{
			cur->nPriority = 300;
			cur->nPlanetMultiplier = 0;
			cur->nAdditiveDecrement = 10;
			cur->nExponentialDecrement = 0;
			cur->prerequisite = M_COLLECTOR;
//			cur->nNumSlots = 2;
		}

		if(c == M_WARLORDTRAINING)
		{
			cur->nPriority = 670;
			cur->nPlanetMultiplier = 0;
			cur->nAdditiveDecrement = 175;
			cur->nExponentialDecrement = 0;
			cur->prerequisite = M_COLLECTOR;
//			cur->nNumSlots = 2;
		}

		if(c == M_BLASTFURNACE)
		{
			cur->nPriority = 500;
			cur->nPlanetMultiplier = 0;
			cur->nAdditiveDecrement = 1000;
			cur->nExponentialDecrement = 0;
			cur->prerequisite = M_THRIPID;
//			cur->nNumSlots = 1;
		}

		if(c == M_EXPLOSIVESRANGE)
		{
			cur->nPriority = 500;
			cur->nPlanetMultiplier = 0;
			cur->nAdditiveDecrement = 1000;
			cur->nExponentialDecrement = 0;
			cur->prerequisite = M_BLASTFURNACE;
//			cur->nNumSlots = 1;
			cur->bMutation = true;
		}

		if(c == M_PLASMASPLITTER)
		{
			cur->nPriority = 60;
			cur->nPlanetMultiplier = 0;
			cur->nAdditiveDecrement = 23;
			cur->nExponentialDecrement = 0;
			cur->prerequisite = M_WARLORDTRAINING;
//			cur->nNumSlots = 0;
		}

		if(c == M_CARRIONROOST)
		{
			cur->nPriority = 770;            
			cur->nPlanetMultiplier = 0;
			cur->nAdditiveDecrement = 1200;
			cur->nExponentialDecrement = 0;
			cur->prerequisite = M_WARLORDTRAINING;
//			cur->nNumSlots = 2;
		}
		
		if(c == M_VORAAKCANNON)
		{
			cur->nPriority = 260;            
			cur->nPlanetMultiplier = 0;
			cur->nAdditiveDecrement = 0;
			cur->nExponentialDecrement = 2;
			cur->prerequisite = M_PLASMASPLITTER;
//			cur->nNumSlots = 1;
			cur->bMutation = true;
		}
		
		if(c == M_MUTATIONCOLONY)
		{
			cur->nPriority = 500;            
			cur->nPlanetMultiplier = 0;
			cur->nAdditiveDecrement = 0;
			cur->nExponentialDecrement = 5;
			cur->prerequisite = M_EYESTOCK;
//			cur->nNumSlots = 2;
			cur->bMutation = true;
		}

		if(c == M_NIAD)
		{
			cur->nPriority = 625;            
			cur->nPlanetMultiplier = 0;
			cur->nAdditiveDecrement = 0;
			cur->nExponentialDecrement = 2;
			cur->prerequisite = M_THRIPID;
//			cur->nNumSlots = 4;
			cur->bMutation = true;
		}

		if(c == M_BIOFORGE)
		{
			cur->nPriority = 450;            
			cur->nPlanetMultiplier = 0;
			cur->nAdditiveDecrement = 1000;
			cur->nExponentialDecrement = 0;
			cur->prerequisite = M_NIAD;
//			cur->nNumSlots = 1;
		}

		if(c == M_FUSIONMILL)
		{
			cur->nPriority = 451;            
			cur->nPlanetMultiplier = 0;
			cur->nAdditiveDecrement = 1000;
			cur->nExponentialDecrement = 0;
			cur->prerequisite = M_BIOFORGE;
//			cur->nNumSlots = 1;
			cur->bMutation = true;
		}

		if(c == M_CARPACEPLANT)
		{
			cur->nPriority = 449;            
			cur->nPlanetMultiplier = 0;
			cur->nAdditiveDecrement = 1000;
			cur->nExponentialDecrement = 0;
			cur->prerequisite = M_BIOFORGE;
//			cur->nNumSlots = 1;
			cur->bMutation = true;
		}

		if(c == M_DISSECTIONCHAMBER)
		{
			cur->nPriority = 400;            
			cur->nPlanetMultiplier = 0;
			cur->nAdditiveDecrement = 1200;
			cur->nExponentialDecrement = 0;
			cur->prerequisite = M_PLANTATION;
//			cur->nNumSlots = 2;
		}

		if(c == M_HYBRIDCENTER)
		{
			cur->nPriority = 505;            
			cur->nPlanetMultiplier = 0;
			cur->nAdditiveDecrement = 1000;
			cur->nExponentialDecrement = 0;
			cur->prerequisite = M_CARPACEPLANT;
//			cur->nNumSlots = 1;
			cur->bMutation = true;
		}

		if(c == M_PLASMAHIVE)
		{
			cur->nPriority = 50;
			cur->nPlanetMultiplier = 0;
			cur->nAdditiveDecrement = 100;
			cur->nExponentialDecrement = 0;
			cur->prerequisite = M_PLASMASPLITTER;
//			cur->nNumSlots = 1;
			cur->bMutation = true;
		}

		if(c == M_JUMPPLAT)
		{
			cur->nPriority = 600;
			cur->nPlanetMultiplier = 0;
			cur->nSystemMultiplier = 1;
			cur->nAdditiveDecrement = 250;
			cur->nExponentialDecrement = 0;
			cur->prerequisite = M_COLLECTOR;
			cur->nNumSlots = 1;
		}
	}

}
//--------------------------------------------------------------------------//
// research priorities
//--------------------------------------------------------------------------//
void MSwarm::initResearchDesires(bool bFirstTime)
{
	for(int c = 0; c < AI_TECH_END; c++)
	{
		ResearchPriority * cur = &ResearchDesires[c];

		if(bFirstTime) cur->bAcquired = false;

		if(GetTechRace((AI_TECH_TYPE)c) != m_nRace)
		{
			cur->nPriority = -10000;
			cur->nAdditiveDecrement = 0;
			cur->nExponentialDecrement = 0;
			cur->prerequisite = NOTECH;
			continue;
		}
		
		//defaults
		cur->nPriority = -10000; // +rand() & 7
		cur->nAdditiveDecrement = 50;  // + rand() & 7
		cur->nExponentialDecrement = 0;
		cur->prerequisite = NOTECH;

		switch(c)
		{
		case M_REPULSOR:
			cur->nPriority = 200;//temporarily high for testing
			cur->prerequisite = NOTECH;
			cur->facility = M_MUTATIONCOLONY;
			break;
		case M_CAMOFLAGE:
			cur->nPriority = 250;//temporarily high for testing
			cur->prerequisite = NOTECH;
			cur->facility = M_MUTATIONCOLONY;
			break;
		case M_ENGINE1:
			cur->nPriority = 45;//105 + (rand() & 7);
			cur->prerequisite = NOTECH;
			cur->facility = M_FUSIONMILL;
			break;
		case M_ENGINE2:
			cur->nPriority = 47; // +rand() & 7
			cur->prerequisite = M_ENGINE1;
			cur->facility = M_FUSIONMILL;
			break;
		case M_ENGINE3:
			cur->nPriority = 29; // +rand() & 7
			cur->prerequisite = M_ENGINE2;
			cur->facility = M_FUSIONMILL;
			break;
		case M_FIGHTER1:
			cur->nPriority = 285; // +rand() & 7
			cur->prerequisite = NOTECH;
			cur->facility = M_CARRIONROOST;
			break;
		case M_FIGHTER2:
			cur->nPriority = 187; // +rand() & 7
			cur->prerequisite = M_FIGHTER1;
			cur->facility = M_CARRIONROOST;
			break;
		case M_FIGHTER3:
			cur->nPriority = 189; // +rand() & 7
			cur->prerequisite = M_FIGHTER2;
			cur->facility = M_CARRIONROOST;
			break;
		case M_FIGHTER4:
			cur->nPriority = 193; // +rand() & 7
			cur->prerequisite = M_FIGHTER3;
			cur->facility = M_CARRIONROOST;
			break;
		case M_FIGHTER5:
			cur->nPriority = 395; // +rand() & 7
			cur->prerequisite = M_FIGHTER4;
			cur->facility = M_CARRIONROOST;
			break;
		case M_GRAVWELL:
			cur->nPriority = 220; // should be lower
			cur->prerequisite = NOTECH;
			cur->facility = M_MUTATIONCOLONY;
			break;
		case M_HULL1:
			cur->nPriority = 50; // +rand() & 7
			cur->prerequisite = NOTECH;
			cur->facility = M_CARPACEPLANT;
			break;
		case M_HULL2:
			cur->nPriority = 55; // +rand() & 7
			cur->prerequisite = M_HULL1;
			cur->facility = M_CARPACEPLANT;
			break;
		case M_HULL3:
			cur->nPriority = 58; // +rand() & 7
			cur->prerequisite = M_HULL2;
			cur->facility = M_CARPACEPLANT;
			break;
		case M_LEECH1:
			cur->nPriority = -160 + (DNA.nNumTroopships * 300); // +rand() & 7
			cur->prerequisite = NOTECH;
			cur->facility = M_WARLORDTRAINING;
			break;
		case M_LEECH2:
			cur->nPriority = -60 + (DNA.nNumTroopships * 300);//100 + rand() & 7;
			cur->prerequisite = M_LEECH1;
			cur->facility = M_WARLORDTRAINING;
			break;
		case M_RAM1:
			cur->nPriority = 320; // +rand() & 7
			cur->prerequisite = NOTECH;
			cur->facility = M_EXPLOSIVESRANGE;
			break;
		case M_RAM2:
			cur->nPriority = 360;//100 + rand() & 7;
			cur->prerequisite = M_RAM1;
			cur->facility = M_EXPLOSIVESRANGE;
			break;
		case M_REPELCLOUD:
			cur->nPriority = 280; // +rand() & 7
			cur->prerequisite = NOTECH;
			cur->facility = M_MUTATIONCOLONY;
			break;
		case M_SENSOR1:
			cur->nPriority = 450; // +rand() & 7
			cur->prerequisite = NOTECH;
			cur->facility = M_EYESTOCK;
			break;
		case M_SENSOR2:
			cur->nPriority = 205; // +rand() & 7
			cur->prerequisite = M_SENSOR1;
			cur->facility = M_EYESTOCK;
			break;
		case M_SENSOR3:
			cur->nPriority = 105; // +rand() & 7
			cur->prerequisite = M_SENSOR2;
			cur->facility = M_EYESTOCK;
			break;
		case M_SHIELD1:
			cur->nPriority = 70; // +rand() & 7
			cur->prerequisite = NOTECH;
			cur->facility = M_BIOFORGE;
			break;
		case M_SHIELD2:
			cur->nPriority = 130; // +rand() & 7
			cur->prerequisite = M_SHIELD1;
			cur->facility = M_BIOFORGE;
			break;
		case M_SHIELD3:
			cur->nPriority = 140; // +rand() & 7
			cur->prerequisite = M_SHIELD2;
			cur->facility = M_BIOFORGE;
			break;
		case M_SUPPLY1:
			cur->nPriority = 50; // +rand() & 7
			cur->prerequisite = NOTECH;
			cur->facility = M_COLLECTOR;
			break;
		case M_SUPPLY2:
			cur->nPriority = 40; // +rand() & 7
			cur->prerequisite = M_SUPPLY1;
			cur->facility = M_COLLECTOR;
			break;
		case M_SUPPLY3:
			cur->nPriority = 30; // +rand() & 7
			cur->prerequisite = M_SUPPLY2;
			cur->facility = M_COLLECTOR;
			break;
		case M_TANKER1:
			cur->nPriority = 9 + (DNA.nMaxHarvesters * 18); // +rand() & 7
			cur->prerequisite = NOTECH;
			cur->facility = M_COLLECTOR;
			break;
		case M_TANKER2:
			cur->nPriority = 6 + (DNA.nMaxHarvesters * 17); // +rand() & 7
			cur->prerequisite = M_TANKER1;
			cur->facility = M_COLLECTOR;
			break;
		case M_TANKER3:
			cur->nPriority = 3 + (DNA.nMaxHarvesters * 16); // +rand() & 7
			cur->prerequisite = M_TANKER2;
			cur->facility = M_COLLECTOR;
			break;
		case M_TENDER1:
			cur->nPriority = 14; // +rand() & 7
			cur->prerequisite = NOTECH;
			cur->facility = M_COLLECTOR;//M_PLANTATION;
			break;
		case M_TENDER2:
			cur->nPriority = 11; // +rand() & 7
			cur->prerequisite = M_TENDER1;
			cur->facility = M_COLLECTOR;//M_PLANTATION;
			break;
		case M_TENDER3:
			cur->nPriority = 11; // +rand() & 7
			cur->prerequisite = M_TENDER2;
			cur->facility = M_COLLECTOR;//M_PLANTATION;
			break;
		case M_TROOPSHIP1:
			cur->nPriority = -100 + (DNA.nNumTroopships * 400); // +rand() & 7
			cur->prerequisite = NOTECH;
			cur->facility = M_WARLORDTRAINING;
			break;
		case M_WEAPON1:
			cur->nPriority = 120; // +rand() & 7
			cur->prerequisite = NOTECH;
			cur->facility = M_BLASTFURNACE;
			break;
		case M_WEAPON2:
			cur->nPriority = 122; // +rand() & 7
			cur->prerequisite = M_WEAPON1;
			cur->facility = M_BLASTFURNACE;
			break;
		case M_WEAPON3:
			cur->nPriority = 123; // +rand() & 7
			cur->prerequisite = M_WEAPON2;
			cur->facility = M_BLASTFURNACE;
			break;
		
		default:
			break;
		}
	}

	ResearchDesires[NOTECH].bAcquired = true;
}
//--------------------------------------------------------------------------//
// gunboat priorities
//--------------------------------------------------------------------------//
void MSwarm::initShipDesires(void)
{
	SPlayerAI::initShipDesires();

	for(int c = 0; c < M_ENDOBJCLASS; c++)
	{
		ShipPriority * cur = &ShipDesires[c];
		
		if(c == M_CORVETTE)
		{
			cur->nPriority = 300;
			cur->nAdditiveDecrement = 8;
			cur->nExponentialDecrement = 0;
			cur->facility = M_LIGHTIND;
		}

		if(c == M_MISSILECRUISER)
		{
			cur->nPriority = rand() & 127;
			cur->nAdditiveDecrement = 45;
			cur->nExponentialDecrement = 0;
			cur->facility = M_LIGHTIND;
		}

		if(c == M_BATTLESHIP)
		{
			cur->nPriority = 200 + (rand() & 127);
			cur->nAdditiveDecrement = 70;
			cur->nExponentialDecrement = 0;
			cur->facility = M_HEAVYIND;
		}

		if(c == M_DREADNOUGHT)
		{
			cur->nPriority = 600;
			cur->nAdditiveDecrement = 100 + (rand() & 127);
			cur->nExponentialDecrement = 0;
			cur->facility = M_HEAVYIND;
		}

		if(c == M_CARRIER)
		{
			cur->nPriority = 450;
			cur->nAdditiveDecrement = 60 + (rand() & 127);
			cur->nExponentialDecrement = 0;
			cur->facility = M_HEAVYIND;
		}

		if(c == M_LANCER)
		{
			cur->nPriority = 300;
			cur->nAdditiveDecrement = 45;
			cur->nExponentialDecrement = 0;
			cur->facility = M_HEAVYIND;
		}

		if(c == M_INFILTRATOR)
		{
			cur->nPriority = SIGHTSHIP_PRI + DNA.uNumScouts * 100;
			cur->nAdditiveDecrement = 130;
			cur->nExponentialDecrement = 0;
			cur->facility = M_LIGHTIND;
		}

		if(c == M_TROOPSHIP)
		{
			cur->nPriority = TROOPSHIP_PRI + DNA.nNumTroopships * 100;
			cur->nAdditiveDecrement = 120;
			cur->nExponentialDecrement = 0;
			cur->facility = M_LIGHTIND;
		}

		if(c == M_SPINELAYER)
		{
			cur->nPriority = MINELAYER_PRI + DNA.nNumMinelayers * 100;
			cur->nAdditiveDecrement = 125;
			cur->nExponentialDecrement = 0;
			cur->facility = M_THRIPID;
		}

		if(c == M_LEECH)
		{
			cur->nPriority = TROOPSHIP_PRI + DNA.nNumTroopships * 100;
			cur->nAdditiveDecrement = 150;
			cur->nExponentialDecrement = 0;
			cur->facility = M_NIAD;
		}

		if(c == M_SEEKER)
		{
			cur->nPriority = SIGHTSHIP_PRI + DNA.uNumScouts * 200;
			cur->nAdditiveDecrement = 400;
			cur->nExponentialDecrement = 0;
			cur->facility = M_THRIPID;
		}

		if(c == M_SCOUTCARRIER)
		{
			cur->nPriority = 380;
			cur->nAdditiveDecrement = 35;
			cur->nExponentialDecrement = 0;
			cur->facility = M_THRIPID;
		}

		if(c == M_FRIGATE)
		{
			cur->nPriority = 50;
			cur->nAdditiveDecrement = 15;
			cur->nExponentialDecrement = 0;
			cur->facility = M_THRIPID;
		}

		if(c == M_KHAMIR)
		{
			cur->nPriority = 50;
			cur->nAdditiveDecrement = 40;
			cur->nExponentialDecrement = 0;
			cur->facility = M_THRIPID;
		}

		if(c == M_HIVECARRIER)
		{
			cur->nPriority = 520;
			cur->nAdditiveDecrement = 9 + (DNA.buildMask.bRandomBit1 * 30);
			cur->nExponentialDecrement = 0;
			cur->facility = M_NIAD;
		}

		if(c == M_SCARAB)
		{
			cur->nPriority = 220;
			cur->nAdditiveDecrement = 34;
			cur->nExponentialDecrement = 0;
			cur->facility = M_NIAD;
		}

		if(c == M_TIAMAT)
		{
			cur->nPriority = 900;
			cur->nAdditiveDecrement = 23 + (DNA.buildMask.bRandomBit1 * 100);
			cur->nExponentialDecrement = 0;
			cur->facility = M_NIAD;
		}

		if(c == M_ATLAS)
		{
			cur->nPriority = MINELAYER_PRI + DNA.nNumMinelayers * 100;
			cur->nAdditiveDecrement = 170;
			cur->nExponentialDecrement = 0;
			cur->facility = M_GREATERPAVILION;
		}

		if(c == M_LEGIONAIRE)
		{
			cur->nPriority = TROOPSHIP_PRI + DNA.nNumTroopships * 100;
			cur->nAdditiveDecrement = 100 + (rand() & 127);
			cur->nExponentialDecrement = 0;
			cur->facility = M_PAVILION;
		}

		if(c == M_TAOS)
		{
			cur->nPriority = 200 + (rand() & 255);
			cur->nAdditiveDecrement = 32;
			cur->nExponentialDecrement = 0;
			cur->facility = M_PAVILION;
		}

		if(c == M_POLARIS)
		{
			cur->nPriority = 300;
			cur->nAdditiveDecrement = 10 + (rand() & 63);
			cur->nExponentialDecrement = 0;
			cur->facility = M_PAVILION;
		}

		if(c == M_AURORA)
		{
			cur->nPriority = 250;
			cur->nAdditiveDecrement = 70;
			cur->nExponentialDecrement = 0;
			cur->facility = M_PAVILION;
		}

		if(c == M_ORACLE)
		{
			cur->nPriority = SIGHTSHIP_PRI + DNA.uNumScouts * 100;
			cur->nAdditiveDecrement = 10;
			cur->nExponentialDecrement = 0;
			cur->facility = M_PAVILION;
		}

		if(c == M_TRIREME)
		{
			cur->nPriority = 500;
			cur->nAdditiveDecrement = 10 + (rand() % 255);
			cur->nExponentialDecrement = 0;
			cur->facility = M_GREATERPAVILION;
		}

		if(c == M_MONOLITH)
		{
			cur->nPriority = 2000;
			cur->nAdditiveDecrement = 10;
			cur->nExponentialDecrement = 2;
			cur->facility = M_GREATERPAVILION;
		}

	}

}


//---------------------------------------------------------------------------//
//
/*
void MSwarm::doHQ (MPart & part)
{
	const char * pArchename = 0;
	U32 archID;
	if (((S32)UnitsOwned[M_WEAVER]) < DNA.nNumFabricators)
		pArchename = "FAB!!M_NymphWeever";
	else
	if (DNA.nGunboatsPerSupplyShip &&
		(UnitsOwned[M_ZORAP] < (m_TotalMilitaryUnits / DNA.nGunboatsPerSupplyShip)))
		pArchename = "SUPSHIP!!M_Zorap";
	
	if (pArchename)
		archID = ARCHLIST->GetArchetypeDataID(pArchename);
	else return;

	U32 fail = 0;
	if(!CanIBuild(archID, &fail))
	{
		if(fail > M_COMMANDPTS)
		{
			CQASSERT(M_COMMANDPTS + fail < M_ENDOBJCLASS);
			BuildDesires[M_COMMANDPTS + fail].nPriority += 25;
		}
		return;
	}

	if (pArchename && archID)
	{
		USR_PACKET<USRBUILD> packet;
		packet.cmd = USRBUILD::ADDIFEMPTY;
		packet.dwArchetypeID = archID;
		packet.objectID[0] = part.obj->GetPartID();
		packet.init(1);
		NETPACKET->Send(HOSTID,0,&packet);
	}
}
//---------------------------------------------------------------------------//
//
void MSwarm::doFabricator (MPart & part)
{
	//const char * pArchename = 0;
	U32 pArcheID = 0;
	U32 pRefineryThreeArcID = 0;
	U32 pBlastFurnaceOneArcID = 0;
	U32 pTenderTwoArcID = 0;
	U32 pLightIndFourArcID = 0;
	U32 genArcID = 0;
	PARCHETYPE pArchetype;
	M_OBJCLASS ChosenPlat = M_NONE;
	M_OBJCLASS LastPlat = M_NONE;
	bool bDone = false;
	bool bSentPacket = false;
	NETGRIDVECTOR resultLoc, fail;
	fail.zero();
	resultLoc.zero();
	fail.systemID = 0;
	resultLoc.systemID = 0;

	U32 resultSite = 0;
	U32 resultSlot = 0;

	S32 bestSlot=-1;
	NETGRIDVECTOR loc;
	loc.zero();
	DOUBLE dist;
	U32 dwFabID = part->dwMissionID;
	U32 systemID = part->systemID;
	U32 siteID = 0;
	U32 closestPlanetID = 0;

	BuildSite SlotSites[4];
	int closestSite = 0;
	DOUBLE closestDist = 10e8;

	//somewhat slow, yet quite useful
	for(int c = 0; c < 4; c++)
	{
		SlotSites[c].planet = 0;

		switch(c)
		{
		case 0:
			pBlastFurnaceOneArcID = ARCHLIST->GetArchetypeDataID("PLATBUILD!!M_BlastFurnace");
			genArcID = pBlastFurnaceOneArcID;
		break;
		case 1:
			pTenderTwoArcID = ARCHLIST->GetArchetypeDataID("PLATREFINERY!!M_EyeStalk");
			genArcID = pTenderTwoArcID;
		break;
		case 2:
			pRefineryThreeArcID = ARCHLIST->GetArchetypeDataID("PLATREFINERY!!M_Collector");
			genArcID = pRefineryThreeArcID;
		break;
		case 3:
			pLightIndFourArcID = ARCHLIST->GetArchetypeDataID("PLATBUILD!!M_Thripid");
			genArcID = pLightIndFourArcID;
		break;
		}

		CQASSERT(genArcID);

		if ((pArchetype = ARCHLIST->LoadArchetype(genArcID)) != 0)
		{
			siteID = ChooseBuildSite(M_NONE, pArchetype, dwFabID, systemID, 
									part.obj->GetPosition(), &part.obj->GetTransform().translation, 
									&bestSlot, &dist, &loc);

			SlotSites[c].planet = siteID;
			SlotSites[c].slot = bestSlot;
			SlotSites[c].dist = dist;
			SlotSites[c].pos = loc;
			
			if(siteID && (dist <= closestDist || siteID == closestPlanetID))
			{
				closestSite = c;
				closestDist = dist;
				closestPlanetID = siteID;
			}
		}
	}

	//get a load a this...
	if (closestSite < 2) IncreaseSlotPriorities(closestSite + 1, 125);
	else if (!IsPlanetOwned(SlotSites[2].planet)) BuildDesires[M_COLLECTOR].nPriority += 200;

	U32 newSystem = SlotSites[2].pos.systemID;
	if((newSystem > 0) && (newSystem < MAX_SYSTEMS) && (!m_bSystemSupplied[newSystem-1]))
	{
		BuildDesires[M_COCOON].nPriority += 205;
		BuildDesires[M_JUMPPLAT].nPriority += 155;
	}

	c = 0;
	while(!bDone)
	{
		ChosenPlat = ChooseNextBuild(true);    //true for platforms

		switch(ChosenPlat)
		{
		default:
			return;
		break;
		case M_COCOON:			pArcheID = ARCHLIST->GetArchetypeDataID("PLATBUILDSUP!!M_Cocoon");			break;
		case M_COLLECTOR:		pArcheID = pRefineryThreeArcID;												break;
		case M_THRIPID:			pArcheID = pLightIndFourArcID;												break;
		case M_EYESTOCK:		pArcheID = pTenderTwoArcID;													break;
		case M_PLANTATION:		pArcheID = ARCHLIST->GetArchetypeDataID("PLATREPAIR!!M_Plantation");		break;
		case M_WARLORDTRAINING:	pArcheID = ARCHLIST->GetArchetypeDataID("PLATREFINERY!!M_Warlord");			break;
		case M_BLASTFURNACE:	pArcheID = pBlastFurnaceOneArcID;											break;
		case M_EXPLOSIVESRANGE:	pArcheID = ARCHLIST->GetArchetypeDataID("PLATBUILD!!M_ExplosivesRange");	break;
		case M_PLASMASPLITTER:	pArcheID = ARCHLIST->GetArchetypeDataID("PLATGUN!!M_PlasmaSpitter");		break;
		case M_CARRIONROOST:	pArcheID = ARCHLIST->GetArchetypeDataID("PLATBUILD!!M_CarrionRoost");		break;
		case M_VORAAKCANNON:	pArcheID = ARCHLIST->GetArchetypeDataID("PLATGUN!!M_Voraak");				break;
		case M_MUTATIONCOLONY:	pArcheID = ARCHLIST->GetArchetypeDataID("PLATREFINERY!!M_Mutation");		break;
		case M_NIAD:			pArcheID = ARCHLIST->GetArchetypeDataID("PLATBUILD!!M_Niad");				break;
		case M_BIOFORGE:		pArcheID = ARCHLIST->GetArchetypeDataID("PLATBUILD!!M_BioForge");			break;
		case M_FUSIONMILL:		pArcheID = ARCHLIST->GetArchetypeDataID("PLATBUILD!!M_FusionMill");			break;
		case M_CARPACEPLANT:	pArcheID = ARCHLIST->GetArchetypeDataID("PLATBUILD!!M_CarpacePlant");		break;
		case M_DISSECTIONCHAMBER:	pArcheID = ARCHLIST->GetArchetypeDataID("PLATREPAIR!!M_Disection");		break;
		case M_HYBRIDCENTER:	pArcheID = ARCHLIST->GetArchetypeDataID("PLATBUILD!!M_HybridCenter");		break;
		case M_PLASMAHIVE:		pArcheID = ARCHLIST->GetArchetypeDataID("PLATGUN!!M_PlasmaHive");			break;
		case M_JUMPPLAT:		pArcheID = ARCHLIST->GetArchetypeDataID("PLATJUMP!!M_JumpPlat");			break;
		}

		U32 fail = 0;
		if(CanIBuild(pArcheID, &fail)) 
		{
			bDone = true;

			S32 index = BuildDesires[ChosenPlat].nNumSlots - 1;

			pArchetype = ARCHLIST->LoadArchetype(pArcheID);

			if(index >= 0)  //free floating plats should be okay
			{
				if (pArchetype != 0)
				{
					siteID = ChooseBuildSite(ChosenPlat, pArchetype, dwFabID, systemID, 
									part.obj->GetPosition(), &part.obj->GetTransform().translation, 
									&bestSlot, &dist, &loc, true);

					SlotSites[index].planet = siteID;
					SlotSites[index].slot = bestSlot;
					SlotSites[index].dist = dist;
					SlotSites[index].pos = loc;
				}
			}	
		}

		if(!bDone)
		{
			if(ChosenPlat == LastPlat) c++;

			if(ChosenPlat < M_ENDOBJCLASS) BuildDesires[ChosenPlat].nPriority -= 75;
			LastPlat = ChosenPlat;

			//increase priority of prerequisite (lack of resource) that caused can I build to fail
			//(or harvesting of needed resource)  fix 

			c++;
			if(c > DNA.nBuildPatience)
			{
				bDone = true;

				doRepair(part);
				return;
			}
		}
	}


	if (pArcheID && (pArchetype = ARCHLIST->LoadArchetype(pArcheID)) != 0)
	{
	//	BASE_PLATFORM_DATA * data = (BASE_PLATFORM_DATA *)ARCHLIST->GetArchetypeData(pArchetype);
	//	U32 numSlots = data->slotsNeeded - 1;
		S32 numSlots = BuildDesires[ChosenPlat].nNumSlots - 1;

		if(numSlots >= 0)
		{
			CQASSERT((numSlots >= 0) && (numSlots <= 3));
			U32 siteID = SlotSites[numSlots].planet;
			S32 slotID = SlotSites[numSlots].slot;
			resultLoc = SlotSites[numSlots].pos;
			resultSite = siteID;
			resultSlot = slotID;

			if (siteID)
			{
				if(ChosenPlat == M_JUMPPLAT)
				{
					USR_PACKET<USRFABJUMP> packet;
					packet.jumpgateID = siteID;
					packet.dwArchetypeID = ARCHLIST->GetArchetypeDataID(pArchetype);
					packet.userBits = 0;
					packet.objectID[0] = dwFabID;
					packet.init(1);
					NETPACKET->Send(HOSTID, 0, &packet);
				}
				else
				{
					CQASSERT(slotID >> 12 == 0);
				
					addPlanet(siteID);

					USR_PACKET<USRFAB> packet;

					packet.planetID = siteID;
					packet.dwArchetypeID = ARCHLIST->GetArchetypeDataID(pArchetype);
					packet.slotID = slotID;
					packet.userBits = 0;
					packet.objectID[0] = dwFabID;
					packet.init(1);
					NETPACKET->Send(HOSTID, 0, &packet);
				}

				bSentPacket = true;
			}
			else
			{
				//lower priority of ChosenPlat
			}
		}
		else
		{	//this is a free-floating plat
			resultLoc = ChooseFreeBuildSite(pArchetype, dwFabID, systemID, part.obj->GetPosition());
							//this is gonna one day have to take into account which type of free
							//floating platform we're doing here, for instance jumpgates'd be different

			if(resultLoc != fail)
			{
				USR_PACKET<USRFABPOS> packet;
				//MISSION_DATA::M_CAPS matchCaps;
				//memset(&matchCaps, 0, sizeof(matchCaps));
				//matchCaps.buildOk = true;
				//initializeUserPacket(&packet, selected, matchCaps);
				//packet.position.init(result,result.systemID);
				//packet.position.quarterpos();  //?

				packet.position = resultLoc;
				packet.userBits = 0;
				packet.dwArchetypeID = pArcheID;

				packet.objectID[0] = dwFabID;
				packet.init(1);
				NETPACKET->Send(HOSTID, 0, &packet);

				bSentPacket = true;
			}
		}
	}

	if(bSentPacket)
	{
		CQASSERT(ChosenPlat >=0 && ChosenPlat < M_ENDOBJCLASS);

		//UnitsOwned[ChosenPlat]++;
		AddSpacialPoint(FAB_POINT, resultLoc, part->dwMissionID, ChosenPlat, resultSite, resultSlot);

		int c = ChosenPlat;
		BuildPriority *cur = &(BuildDesires[c]);

		if(cur->nExponentialDecrement > 0)
		{
			cur->nPriority /= cur->nExponentialDecrement;
		}
		else
		{
			cur->nPriority -= cur->nAdditiveDecrement;
			//let's just keep the flollowing in the once-every-ten-updates build update
			//if(cur->nPlanetMultiplier > 0)
			//	cur->nPriority += m_PlanetsUnderMyControl * cur->nPlanetMultiplier * cur->nAdditiveDecrement;
		}
	}
}
//---------------------------------------------------------------------------//
//
void MSwarm::doLight (MPart & part)
{
	const char * pArchename = 0;
	U32 archID;

	U32 numHarvesters = UnitsOwned[M_SIPHON];
	if(!DNA.buildMask.bHarvest) numHarvesters = 5;

	if (UnitsOwned[M_SCOUTCARRIER] < 5)
		pArchename = "GBOAT!!M_Scout Carrier";
	else
	if (UnitsOwned[M_SEEKER] < DNA.uNumScouts)
		pArchename = "GBOAT!!M_Seeker";
	else
	if ((S32)UnitsOwned[M_SPINELAYER] < DNA.nNumMinelayers)
		pArchename = "MLAYER!!M_Spinelayer";
	//if (UnitsOwned[M_KHAMIR] < 4 && (rand() % 2))
	//	pArchename = "GBOAT!!M_Khamir";
	else
	if (UnitsOwned[M_FRIGATE] < 3)
		pArchename = "GBOAT!!M_Frigate";
	else
	if (UnitsOwned[M_SCOUTCARRIER] < 7 || UnitsOwned[M_NIAD] < 1)
		pArchename = "GBOAT!!M_Scout Carrier";
	//else 	// 2 for each escort, plus 1 for scouting
	//if (UnitsOwned[M_SCOUTCARRIER]+UnitsOwned[M_FRIGATE] < (numHarvesters * 2) + 30)  //maybe m_HarvestEscorts figures in here
	//	pArchename = (rand() % 2) ? "GBOAT!!M_Scout Carrier" : "GBOAT!!M_Frigate";

	if(!pArchename) return;

	archID = ARCHLIST->GetArchetypeDataID(pArchename);

	U32 fail = 0;
	if(!CanIBuild(archID, &fail))
	{
		if(fail > M_COMMANDPTS)
		{
			CQASSERT(M_COMMANDPTS + fail < M_ENDOBJCLASS);
			BuildDesires[M_COMMANDPTS + fail].nPriority += 25;
		}
		return;
	}

	if (pArchename && archID)
	{
		USR_PACKET<USRBUILD> packet;
		packet.cmd = USRBUILD::ADDIFEMPTY;
		packet.dwArchetypeID = archID;
		packet.objectID[0] = part.obj->GetPartID();
		packet.init(1);
		NETPACKET->Send(HOSTID,0,&packet);
	}
}
//---------------------------------------------------------------------------//
//
void MSwarm::doHeavyInd (MPart & part)
{
	const char * pArchename = 0;
	U32 archID;

	pArchename = "GBOAT!!M_Tiamat";
	archID = ARCHLIST->GetArchetypeDataID(pArchename);
	U32 fail = 0;
	if(!CanIBuild(archID, &fail))
	{
		if(fail > M_COMMANDPTS)
		{
df			CQASSERT(M_COMMANDPTS + fail < M_ENDOBJCLASS);
dgdf			BuildDesires[M_COMMANDPTS + fail].nPriority += 25;
		}

		pArchename = NULL;

		if ((S32)UnitsOwned[M_LEECH] < DNA.nNumTroopships)
			pArchename = "TSHIP!!M_Leech";
		else
		if (UnitsOwned[M_FRIGATE] < 3)
			pArchename = "GBOAT!!M_Frigate";
		else
		if (UnitsOwned[M_SCOUTCARRIER] < 3)
			pArchename = "GBOAT!!M_Scout Carrier";
		//else
		//if (UnitsOwned[M_SCARAB] < 2)
		//	pArchename = "GBOAT!!M_Scarab";
		else 
			pArchename = "GBOAT!!M_Hive Carrier";
	}
	else
	{
		pArchename = NULL;
		pArchename = (rand() % 2) ? "GBOAT!!M_Hive Carrier" : "GBOAT!!M_Tiamat";
	}

	if(!pArchename) return;

	archID = ARCHLIST->GetArchetypeDataID(pArchename);
	if(!CanIBuild(archID, &fail)) return;

	if (pArchename && archID)
	{
		USR_PACKET<USRBUILD> packet;
		packet.cmd = USRBUILD::ADDIFEMPTY;
		packet.dwArchetypeID = archID;
		packet.objectID[0] = part.obj->GetPartID();
		packet.init(1);
		NETPACKET->Send(HOSTID,0,&packet);

		m_AttackWaveSize--;
	}
}
*/
//---------------------------------------------------------------------------//
//
/*
void MSwarm::doSupplyShip (MPart & part)
{
	// escort biggest ship we have
	//
	// find biggest ship in the system
	//
	IBaseObject * obj = OBJLIST->GetTargetList();     // mission objects
	MPart bestShip;
	const U32 systemID = part->systemID;
	MPart other;

	while (obj)
	{
		if ((other = obj).isValid() && other->systemID==systemID && other->playerID==playerID && (other->mObjClass==M_WARLORD || other->mObjClass==M_SCARAB))
		{
			ASSIGNMENT * assign = findAssignment(other);
			if (assign == 0 || assign->type == DEFEND)  //maybe check here if "other" needs supplies
			{
				if (bestShip.isValid())
				{
					if (other->mObjClass > bestShip->mObjClass)
					{
						bestShip = other;
					}
				}
				else
				{
					bestShip = other;
				}
			}
		} 
		
		obj = obj->nextTarget;
	} // end while()

	if (bestShip.isValid())
	{
		USR_PACKET<USRESCORT> packet;

		packet.objectID[0] = part->dwMissionID;
		packet.userBits = 0;
		packet.targetID = bestShip->dwMissionID;
		packet.init(1);
		NETPACKET->Send(HOSTID, 0, &packet);
	}
	else  //add to defense squadron
	{
		ASSIGNMENT * assign = findAssignment(part);
		if (!assign) assignGunboat(part);
	}
}
*/
//----------------------------------------------------------------------------------------//
//
void __stdcall CreateSwarmPlayerAI (ISPlayerAI ** ppPlayerAI, U32 playerID, M_RACE race)
{
	MSwarm * player = new MSwarm;
	*ppPlayerAI = player;
	player->init(playerID, race);
}
//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//
//-------------------------END MSwarm.cpp-------------------------//
//---------------------------------------------------------------------------//