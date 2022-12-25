//--------------------------------------------------------------------------------//
//                                                                                //
//                                 MStandardSolarianAI.cpp                          //
//                                                                                //
//                  COPYRIGHT (C) 2000 BY DIGITAL ANVIL, INC.                     //
//                                                                                //
//--------------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/MStandardSolarianAI.cpp 78    10/18/00 4:50p Ahunter $
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
struct MSolarianAI : public DAComponent<SPlayerAI>
{
	/* mantis AI data items */
	

	
	/* end of data items */
	
	virtual void	init (U32 playerID, M_RACE race);
	virtual void	initBuildDesires(void);
	virtual void	initResearchDesires(bool bFirstTime = false);
	virtual void	initShipDesires(void);


	//virtual void	onIdleUnit (IBaseObject * obj);  //using parent method
	
	/* MSolarianAI methods */
	//virtual void			doSupplyShip (MPart & part);
	//virtual void			doFabricator (MPart & part);
	//virtual void			doHQ (MPart & part);
	//virtual void			doLight (MPart & part);
	//virtual void			doHeavyInd (MPart & part);
	//virtual void			assignGunboat (MPart & part);
};
//--------------------------------------------------------------------------//
//
void MSolarianAI::init (U32 _playerID, M_RACE race)
{
	SPlayerAI::init(_playerID, race);
	
	CQASSERT(race == M_SOLARIAN);
	strategy = SOLARIAN_FORWARD_BUILD;

	AIPersonality settings = DNA;
	settings.buildMask.bSendTaunts = true;
	settings.buildMask.bResignationPossible = true;
	settings.buildMask.bVisibilityRules = true;
	settings.nNumFleets = 2;
	//settings.nHarvestEscorts = 1;		//number of gunboats to use to escort each harvester
	settings.uNumScouts = 2 + (SECTOR->GetNumSystems() / 4);  //should be based on map size  fix  
	settings.nNumTroopships = rand() & 1;  //could be based on difficulty level  fix  
	settings.nGunboatsPerSupplyShip = 6;
	settings.nBuildPatience = 6;
	settings.nNumFabricators = 2;
	settings.nNumMinelayers = rand() % 3;
	settings.nFabricateEscorts = 1; //does this work?  fix 
	SetPersonality(settings);
	
	m_AttackWaveSize = MAX_SELECTED_UNITS / 2 + (rand() & 7);
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
void MSolarianAI::initBuildDesires(void)
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

		if(c == M_ACROPOLIS)
		{
			cur->nPriority = 1500;
			cur->nPlanetMultiplier = 0;
			cur->nSystemMultiplier = 0;
			cur->nAdditiveDecrement = 0;
			cur->nExponentialDecrement = 6;
//			cur->nNumSlots = 3;
		}

		if(c == M_OXIDATOR)
		{
			cur->nPriority = 695;
			cur->nPlanetMultiplier = 1;
			cur->nAdditiveDecrement = 375;
			cur->nExponentialDecrement = 0;
			cur->prerequisite = M_ACROPOLIS;
//			cur->nNumSlots = 3;
		}
		
		if(c == M_PAVILION)
		{
			cur->nPriority = 750;
			cur->nPlanetMultiplier = 0;
			cur->nAdditiveDecrement = 0;
			cur->nExponentialDecrement = 5;
			cur->prerequisite = M_OXIDATOR;
//			cur->nNumSlots = 3;
		}

		if(c == M_SENTINELTOWER)
		{
			cur->nPriority = 450;
			cur->nPlanetMultiplier = 0;
			cur->nAdditiveDecrement = 55;
			cur->nExponentialDecrement = 0;
			cur->prerequisite = M_ACROPOLIS;
//			cur->nNumSlots = 2;
		}

		if(c == M_EUTROMILL)
		{
			cur->nPriority = 680;
			cur->nPlanetMultiplier = 0;
			cur->nAdditiveDecrement = 0;
			cur->nExponentialDecrement = 3;
			cur->prerequisite = M_PAVILION;
//			cur->nNumSlots = 2;
		}

		if(c == M_BUNKER)
		{
			cur->nPriority = 815;
			cur->nPlanetMultiplier = 0;
			cur->nAdditiveDecrement = 245;
			cur->nExponentialDecrement = 0;
			cur->prerequisite = M_OXIDATOR;
//			cur->nNumSlots = 1;
		}

		if(c == M_CITADEL)
		{
			cur->nPriority = 530;
			cur->nPlanetMultiplier = 0;
			cur->nAdditiveDecrement = 225;
			cur->nExponentialDecrement = 0;
			cur->prerequisite = M_BUNKER;
//			cur->nNumSlots = 2;
		}

		if(c == M_HELIONVEIL)
		{
			cur->nPriority = 500;
			cur->nPlanetMultiplier = 0;
			cur->nAdditiveDecrement = 1000;
			cur->nExponentialDecrement = 0;
			cur->prerequisite = M_SENTINELTOWER;
//			cur->nNumSlots = 1;
		}

		if(c == M_XENOCHAMBER)
		{
			cur->nPriority = 1100;
			cur->nPlanetMultiplier = 0;
			cur->nAdditiveDecrement = 0;
			cur->nExponentialDecrement = 3;
			cur->prerequisite = M_CITADEL;
//			cur->nNumSlots = 3;
		}

		if(c == M_ANVIL)
		{
			cur->nPriority = 570;
			cur->nPlanetMultiplier = 0;
			cur->nAdditiveDecrement = 650;
			cur->nExponentialDecrement = 0;
			cur->prerequisite = M_GREATERPAVILION;
//			cur->nNumSlots = 1;
		}

		if(c == M_MUNITIONSANNEX)
		{
			cur->nPriority = 500;            
			cur->nPlanetMultiplier = 0;
			cur->nAdditiveDecrement = 1000;
			cur->nExponentialDecrement = 0;
			cur->prerequisite = M_GREATERPAVILION;
//			cur->nNumSlots = 1;
		}
		
		if(c == M_TURBINEDOCK)
		{
			cur->nPriority = 525;            
			cur->nPlanetMultiplier = 0;
			cur->nAdditiveDecrement = 1000;
			cur->nExponentialDecrement = 0;
			cur->prerequisite = M_GREATERPAVILION;
//			cur->nNumSlots = 1;
		}
		
		if(c == M_TALOREANMATRIX)
		{
			cur->nPriority = 615;            
			cur->nPlanetMultiplier = 0;
			cur->nAdditiveDecrement = 35 + ((rand() & 511) * DNA.buildMask.bRandomBit1);
			cur->nExponentialDecrement = 0;
			cur->prerequisite = M_XENOCHAMBER;
//			cur->nNumSlots = 2;
		}

		if(c == M_GREATERPAVILION)
		{
			cur->nPriority = 825;            
			cur->nPlanetMultiplier = 0;
			cur->nAdditiveDecrement = 0;
			cur->nExponentialDecrement = 3;
			cur->prerequisite = M_PAVILION;
//			cur->nNumSlots = 3;
		}

		if(c == M_PROTEUS)
		{
			cur->nPriority = 425;            
			cur->nPlanetMultiplier = 0;
			cur->nAdditiveDecrement = 49;
			cur->nExponentialDecrement = 0;
			cur->prerequisite = M_PAVILION;
//			cur->nNumSlots = 0;
		}

		if(c == M_HYDROFOIL)
		{
			cur->nPriority = 450;            
			cur->nPlanetMultiplier = 0;
			cur->nAdditiveDecrement = 150;
			cur->nExponentialDecrement = 0;
			cur->prerequisite = M_PROTEUS;
//			cur->nNumSlots = 0;
		}

		if(c == M_ESPCOIL)
		{
			cur->nPriority = 451;            
			cur->nPlanetMultiplier = 0;
			cur->nAdditiveDecrement = 120;
			cur->nExponentialDecrement = 0;
			cur->prerequisite = M_SENTINELTOWER;
//			cur->nNumSlots = 0;
		}

		if(c == M_STARBURST)
		{
			cur->nPriority = 549;            
			cur->nPlanetMultiplier = 0;
			cur->nAdditiveDecrement = 110;
			cur->nExponentialDecrement = 0;
			cur->prerequisite = M_TURBINEDOCK;
//			cur->nNumSlots = 0;
		}

		if(c == M_PORTAL)
		{
			cur->nPriority = 449 + (DNA.buildMask.bRandomBit2 * 256) + (rand() & 255);            
			cur->nPlanetMultiplier = 0;
			cur->nAdditiveDecrement = 0;
			cur->nExponentialDecrement = 20;
			cur->prerequisite = M_XENOCHAMBER;
//			cur->nNumSlots = 0;
		}


		if(c == M_JUMPPLAT)
		{
			cur->nPriority = 500;            //haven't seen this working yet
			cur->nPlanetMultiplier = 0;
			cur->nSystemMultiplier = 1;
			cur->nAdditiveDecrement = 350;
			cur->nExponentialDecrement = 0;
			cur->prerequisite = M_OXIDATOR;
			cur->nNumSlots = 1;
		}

	}

}
//--------------------------------------------------------------------------//
// research priorities
//--------------------------------------------------------------------------//
void MSolarianAI::initResearchDesires(bool bFirstTime)
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
		case S_DESTABILIZER:
			cur->nPriority = 850;
			cur->prerequisite = NOTECH;
			cur->facility = M_XENOCHAMBER;
			break;
		case S_CLOAKER:
			cur->nPriority = 200;
			cur->prerequisite = NOTECH;
			cur->facility = M_XENOCHAMBER;
			break;
		case S_ENGINE1:
			cur->nPriority = 0;//105 + (rand() & 7);
			cur->prerequisite = NOTECH;
			cur->facility = M_TURBINEDOCK;
			break;
		case S_ENGINE2:
			cur->nPriority = 5; // +rand() & 7
			cur->prerequisite = S_ENGINE1;
			cur->facility = M_TURBINEDOCK;
			break;
		case S_ENGINE3:
			cur->nPriority = 6;// +rand() & 7
			cur->prerequisite = S_ENGINE2;
			cur->facility = M_TURBINEDOCK;
			break;
		case S_ENGINE4:
			cur->nPriority = 7;// +rand() & 7
			cur->prerequisite = S_ENGINE3;
			cur->facility = M_TURBINEDOCK;
			break;
		case S_ENGINE5:
			cur->nPriority = 8;// +rand() & 7
			cur->prerequisite = S_ENGINE4;
			cur->facility = M_TURBINEDOCK;
			break;
		case S_HULL1:
			cur->nPriority = 65;// +rand() & 7
			cur->prerequisite = NOTECH;
			cur->facility = M_ANVIL;
			break;
		case S_HULL2:
			cur->nPriority = 67;//+rand() & 7
			cur->prerequisite = S_HULL1;
			cur->facility = M_ANVIL;
			break;
		case S_HULL3:
			cur->nPriority = 70; // +rand() & 7
			cur->prerequisite = S_HULL2;
			cur->facility = M_ANVIL;
			break;
		case S_HULL4:
			cur->nPriority = 75; // +rand() & 7
			cur->prerequisite = S_HULL3;
			cur->facility = M_ANVIL;
			break;
		case S_HULL5:
			cur->nPriority = 85; // +rand() & 7
			cur->prerequisite = S_HULL4;
			cur->facility = M_ANVIL;
			break;
		case S_LEGIONAIRE1:
			cur->nPriority = -160 + (DNA.nNumTroopships * 300); // +rand() & 7
			cur->prerequisite = NOTECH;
			cur->facility = M_BUNKER;
			break;
		case S_LEGIONAIRE2:
			cur->nPriority = -100 + (DNA.nNumTroopships * 300);//100 + rand() & 7;
			cur->prerequisite = S_LEGIONAIRE1;
			cur->facility = M_BUNKER;
			break;
		case S_LEGIONAIRE3:
			cur->nPriority = -160 + (DNA.nNumTroopships * 300); // +rand() & 7
			cur->prerequisite = S_LEGIONAIRE2;
			cur->facility = M_BUNKER;
			break;
		case S_LEGIONAIRE4:
			cur->nPriority = -120 + (DNA.nNumTroopships * 300);//100 + rand() & 7;
			cur->prerequisite = S_LEGIONAIRE3;
			cur->facility = M_BUNKER;
			break;
		case S_MASSDISRUPTOR:
			cur->nPriority = 460; // +rand() & 7
			cur->prerequisite = NOTECH;
			cur->facility = M_XENOCHAMBER;
			break;
		case S_ORE1:
			cur->nPriority = 112;
			cur->prerequisite = NOTECH;
			cur->facility = M_OXIDATOR;
			break;
		case S_ORE2:	
			cur->nPriority = 60;
			cur->prerequisite = S_ORE1;
			cur->facility = M_OXIDATOR;
			break;
		case S_ORE3:
			cur->nPriority = 0;
			cur->prerequisite = S_ORE2;
			cur->facility = M_OXIDATOR;
			break;
		case S_GAS1:
			cur->nPriority = 100;
			cur->prerequisite = NOTECH;
			cur->facility = M_OXIDATOR;
			break;
		case S_GAS2:
			cur->nPriority = 102;
			cur->prerequisite = S_GAS1;
			cur->facility = M_OXIDATOR;
			break;
		case S_GAS3:
			cur->nPriority = 101;
			cur->prerequisite = S_GAS2;
			cur->facility = M_OXIDATOR;
			break;
		case S_TRACTOR:
			cur->nPriority = 1030;//100 + rand() & 7;
			cur->prerequisite = NOTECH;
			cur->facility = M_XENOCHAMBER;
			break;
		case S_SENSOR1:
			cur->nPriority = 510; // +rand() & 7
			cur->prerequisite = NOTECH;
			cur->facility = M_SENTINELTOWER;
			break;
		case S_SENSOR2:
			cur->nPriority = 206; // +rand() & 7
			cur->prerequisite = S_SENSOR1;
			cur->facility = M_SENTINELTOWER;
			break;
		case S_SENSOR3:
			cur->nPriority = 106; // +rand() & 7
			cur->prerequisite = S_SENSOR2;
			cur->facility = M_SENTINELTOWER;
			break;
		case S_SENSOR4:
			cur->nPriority = 104; // +rand() & 7
			cur->prerequisite = S_SENSOR3;
			cur->facility = M_SENTINELTOWER;
			break;
		case S_SENSOR5:
			cur->nPriority = 102; // +rand() & 7
			cur->prerequisite = S_SENSOR4;
			cur->facility = M_SENTINELTOWER;
			break;
		case S_SHIELD1:
			cur->nPriority = 110; // +rand() & 7
			cur->prerequisite = NOTECH;
			cur->facility = M_HELIONVEIL;
			break;
		case S_SHIELD2:
			cur->nPriority = 120; // +rand() & 7
			cur->prerequisite = S_SHIELD1;
			cur->facility = M_HELIONVEIL;
			break;
		case S_SHIELD3:
			cur->nPriority = 130; // +rand() & 7
			cur->prerequisite = S_SHIELD2;
			cur->facility = M_HELIONVEIL;
			break;
		case S_SHIELD4:
			cur->nPriority = 200; // +rand() & 7
			cur->prerequisite = S_SHIELD3;
			cur->facility = M_HELIONVEIL;
			break;
		case S_SHIELD5:
			cur->nPriority = 202; // +rand() & 7
			cur->prerequisite = S_SHIELD4;
			cur->facility = M_HELIONVEIL;
			break;
		case S_SUPPLY1:
			cur->nPriority = 100; // +rand() & 7
			cur->prerequisite = NOTECH;
			cur->facility = M_OXIDATOR;
			break;
		case S_SUPPLY2:
			cur->nPriority = 110; // +rand() & 7
			cur->prerequisite = S_SUPPLY1;
			cur->facility = M_OXIDATOR;
			break;
		case S_SUPPLY3:
			cur->nPriority = 111; // +rand() & 7
			cur->prerequisite = S_SUPPLY2;
			cur->facility = M_OXIDATOR;
			break;
		case S_SUPPLY4:
			cur->nPriority = 120; // +rand() & 7
			cur->prerequisite = S_SUPPLY3;
			cur->facility = M_OXIDATOR;
			break;
		case S_SUPPLY5:
			cur->nPriority = 50; // +rand() & 7
			cur->prerequisite = S_SUPPLY4;
			cur->facility = M_OXIDATOR;
			break;
		case S_SYNTHESIS:
			cur->nPriority = 595; // +rand() & 7 //what the hell is this? hehe
			cur->prerequisite = NOTECH;
			cur->facility = M_XENOCHAMBER;
			break;
		case S_TANKER1:
			cur->nPriority = 15; // +rand() & 7
			cur->prerequisite = NOTECH;
			cur->facility = M_OXIDATOR;
			break;
		case S_TANKER2:
			cur->nPriority = 14; // +rand() & 7
			cur->prerequisite = S_TANKER1;
			cur->facility = M_OXIDATOR;
			break;
		/*
		case S_TANKER3:
			cur->nPriority = 10; // +rand() & 7
			cur->prerequisite = S_TANKER2;
			cur->facility = M_OXIDATOR;
			break;
		case S_TANKER4:
			cur->nPriority = 11; // +rand() & 7
			cur->prerequisite = S_TANKER3;
			cur->facility = M_OXIDATOR;
			break;
		case S_TANKER5:
			cur->nPriority = 200; // +rand() & 7
			cur->prerequisite = S_TANKER4;
			cur->facility = M_OXIDATOR;
			break;
			*/
		case S_TENDER1:
			cur->nPriority = 5; // +rand() & 7
			cur->prerequisite = NOTECH;
			cur->facility = M_OXIDATOR;//M_EUTROMILL;
			break;
		case S_TENDER2:
			cur->nPriority = 10; // +rand() & 7
			cur->prerequisite = S_TENDER1;
			cur->facility = M_OXIDATOR;//M_EUTROMILL;
			break;
		/*
		case S_TROOPSHIP1:
			cur->nPriority = 10; // +rand() & 7
			cur->prerequisite = NOTECH;
			cur->facility = M_BUNKER;
			break;
		case S_TROOPSHIP2:
			cur->nPriority = 10; // +rand() & 7
			cur->prerequisite = S_TROOPSHIP1;
			cur->facility = M_BUNKER;
			break;
		case S_TROOPSHIP3:
			cur->nPriority = 10; // +rand() & 7
			cur->prerequisite = S_TROOPSHIP2;
			cur->facility = M_BUNKER;
			break;
		*/
		case S_WEAPON1:
			cur->nPriority = 160; // +rand() & 7
			cur->prerequisite = NOTECH;
			cur->facility = M_MUNITIONSANNEX;
			break;
		case S_WEAPON2:
			cur->nPriority = 180; // +rand() & 7
			cur->prerequisite = S_WEAPON1;
			cur->facility = M_MUNITIONSANNEX;
			break;
		case S_WEAPON3:
			cur->nPriority = 200; // +rand() & 7
			cur->prerequisite = S_WEAPON2;
			cur->facility = M_MUNITIONSANNEX;
			break;
		case S_WEAPON4:
			cur->nPriority = 250; // +rand() & 7
			cur->prerequisite = S_WEAPON3;
			cur->facility = M_MUNITIONSANNEX;
			break;
		case S_WEAPON5:
			cur->nPriority = 280; // +rand() & 7
			cur->prerequisite = S_WEAPON4;
			cur->facility = M_MUNITIONSANNEX;
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
void MSolarianAI::initShipDesires(void)
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
			cur->nPriority = SIGHTSHIP_PRI + DNA.uNumScouts * 100;
			cur->nAdditiveDecrement = 130;
			cur->nExponentialDecrement = 0;
			cur->facility = M_THRIPID;
		}

		if(c == M_SCOUTCARRIER)
		{
			cur->nPriority = 200;
			cur->nAdditiveDecrement = 35;
			cur->nExponentialDecrement = 0;
			cur->facility = M_THRIPID;
		}

		if(c == M_FRIGATE)
		{
			cur->nPriority = 250;
			cur->nAdditiveDecrement = 15;
			cur->nExponentialDecrement = 0;
			cur->facility = M_THRIPID;
		}

		if(c == M_KHAMIR)
		{
			cur->nPriority = 200;
			cur->nAdditiveDecrement = 40;
			cur->nExponentialDecrement = 0;
			cur->facility = M_THRIPID;
		}

		if(c == M_HIVECARRIER)
		{
			cur->nPriority = 400;
			cur->nAdditiveDecrement = 80 + (rand() & 127);
			cur->nExponentialDecrement = 0;
			cur->facility = M_NIAD;
		}

		if(c == M_SCARAB)
		{
			cur->nPriority = 400;
			cur->nAdditiveDecrement = 155 + (rand() & 63);
			cur->nExponentialDecrement = 0;
			cur->facility = M_NIAD;
		}

		if(c == M_TIAMAT)
		{
			cur->nPriority = 500;
			cur->nAdditiveDecrement = 30 + (rand() & 127);
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
			cur->nPriority = 200 + (((rand() & 255) + 20) * DNA.buildMask.bRandomBit3);
			cur->nAdditiveDecrement = 32;
			cur->nExponentialDecrement = 0;
			cur->facility = M_PAVILION;
		}

		if(c == M_POLARIS)
		{
			cur->nPriority = 400;
			cur->nAdditiveDecrement = 10 + (rand() & 15);
			cur->nExponentialDecrement = 0;
			cur->facility = M_PAVILION;
		}

		if(c == M_AURORA)
		{
			cur->nPriority = 750;
			cur->nAdditiveDecrement = 42;
			cur->nExponentialDecrement = 0;
			cur->facility = M_PAVILION;
		}

		if(c == M_ORACLE)
		{
			cur->nPriority = SIGHTSHIP_PRI + DNA.uNumScouts * 200;
			cur->nAdditiveDecrement = 400;
			cur->nExponentialDecrement = 0;
			cur->facility = M_PAVILION;
		}

		if(c == M_TRIREME)
		{
			cur->nPriority = 970;
			cur->nAdditiveDecrement = 50 + (((rand() & 255) + 20) * DNA.buildMask.bRandomBit4);
			cur->nExponentialDecrement = 0;
			cur->facility = M_GREATERPAVILION;
		}

		if(c == M_MONOLITH)
		{
			cur->nPriority = 1400;
			cur->nAdditiveDecrement = 70 + (((rand() & 255) + 20) * DNA.buildMask.bRandomBit5);
			cur->nExponentialDecrement = 0;
			cur->facility = M_GREATERPAVILION;
		}

	}

}


//---------------------------------------------------------------------------//
//
/*
void MSolarianAI::doHQ (MPart & part)
{
	const char * pArchename = 0;
	U32 archID;
	if (((S32)UnitsOwned[M_FORGER]) < DNA.nNumFabricators)
		pArchename = "FAB!!S_Forger";
	else
	if (DNA.nGunboatsPerSupplyShip &&
		(UnitsOwned[M_STRATUM] < (m_TotalMilitaryUnits / DNA.nGunboatsPerSupplyShip)))
		pArchename = "SUPSHIP!!S_Stratum";
	
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
void MSolarianAI::doFabricator (MPart & part)
{
	//const char * pArchename = 0;
	U32 pArcheID = 0;
	U32 pOxidatorThreeArcID = 0;
	U32 pAnvilOneArcID = 0;
	U32 pSentTowerTwoArcID = 0;
	//U32 pFourArcID = 0;
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

	for(int c = 0; c < 3; c++)
	{
		SlotSites[c].planet = 0;

		switch(c)
		{
		case 0:
			pAnvilOneArcID = ARCHLIST->GetArchetypeDataID("PLATBUILD!!S_Anvil");
			genArcID = pAnvilOneArcID;
		break;
		case 1:
			pSentTowerTwoArcID = ARCHLIST->GetArchetypeDataID("PLATBUILD!!S_SentinelTower");
			genArcID = pSentTowerTwoArcID;
		break;
		case 2:
			pOxidatorThreeArcID = ARCHLIST->GetArchetypeDataID("PLATREFINERY!!S_Oxidator");
			genArcID = pOxidatorThreeArcID;
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

			if(siteID && (dist <= closestDist || siteID == closestPlanetID)) //so larger bldgs get precedence
			{
				closestSite = c;
				closestDist = dist;
				closestPlanetID = siteID;
			}
		}
	}

	//get a load a this...
	if (closestSite < 2) IncreaseSlotPriorities(closestSite + 1, 125);
	else if (!IsPlanetOwned(SlotSites[2].planet)) BuildDesires[M_OXIDATOR].nPriority += 200;

	U32 newSystem = SlotSites[2].pos.systemID;
	if((newSystem > 0) && (newSystem < MAX_SYSTEMS) && (!m_bSystemSupplied[newSystem-1]))
	{
		BuildDesires[M_ACROPOLIS].nPriority += 205;
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
		break; //bunker, anvil
		case M_ACROPOLIS:		pArcheID = ARCHLIST->GetArchetypeDataID("PLATBUILDSUP!!S_Acropolis");		break;
		case M_OXIDATOR:		pArcheID = pOxidatorThreeArcID;												break;
		case M_PAVILION:		pArcheID = ARCHLIST->GetArchetypeDataID("PLATBUILD!!S_Pavilion");			break;
		case M_GREATERPAVILION:	pArcheID = ARCHLIST->GetArchetypeDataID("PLATBUILD!!S_GreaterPavilion");	break;
		case M_SENTINELTOWER:	pArcheID = pSentTowerTwoArcID;												break;
		case M_CITADEL:			pArcheID = ARCHLIST->GetArchetypeDataID("PLATREFINERY!!S_Citadel");			break;
		case M_EUTROMILL:		pArcheID = ARCHLIST->GetArchetypeDataID("PLATREPAIR!!S_Eutromill");			break;
		case M_BUNKER:			pArcheID = ARCHLIST->GetArchetypeDataID("PLATREFINERY!!S_Bunker");			break;
		case M_HELIONVEIL:		pArcheID = ARCHLIST->GetArchetypeDataID("PLATBUILD!!S_HelionVeil");			break;
		case M_XENOCHAMBER:		pArcheID = ARCHLIST->GetArchetypeDataID("PLATBUILD!!S_XenoChamber");		break;
		case M_ANVIL:			pArcheID = pAnvilOneArcID;													break;
		case M_MUNITIONSANNEX:	pArcheID = ARCHLIST->GetArchetypeDataID("PLATBUILD!!S_MunitionsAnnex");		break;
		case M_TURBINEDOCK:		pArcheID = ARCHLIST->GetArchetypeDataID("PLATBUILD!!S_TurbineDock");		break;
		case M_TALOREANMATRIX:	pArcheID = ARCHLIST->GetArchetypeDataID("PLATGUN!!S_TaloreanMatrix");		break;
		case M_PROTEUS:			pArcheID = ARCHLIST->GetArchetypeDataID("PLATGUN!!S_Proteus");				break;
		case M_HYDROFOIL:		pArcheID = ARCHLIST->GetArchetypeDataID("PLATGUN!!S_Hydrofoil");			break;
		case M_ESPCOIL:			pArcheID = ARCHLIST->GetArchetypeDataID("PLATGUN!!S_ESPCoil");				break;
		case M_STARBURST:		pArcheID = ARCHLIST->GetArchetypeDataID("PLATGUN!!S_Starburst");			break;
		case M_JUMPPLAT:		pArcheID = ARCHLIST->GetArchetypeDataID("PLATJUMP!!S_JumpPlat");			break;
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
		//BASE_PLATFORM_DATA * data = (BASE_PLATFORM_DATA *)ARCHLIST->GetArchetypeData(pArchetype);
		//U32 numSlots = data->slotsNeeded - 1;
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

			if(resultLoc != fail)
			{
				USR_PACKET<USRFABPOS> packet;
				//MISSION_DATA::M_CAPS matchCaps;
				//memset(&matchCaps, 0, sizeof(matchCaps));
				//matchCaps.buildOk = true;
				//initializeUserPacket(&packet, selected, matchCaps);
				//packet.position.init(resultLoc,resultLoc.systemID);
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
void MSolarianAI::doLight (MPart & part)
{
	const char * pArchename = 0;
	U32 archID;

	U32 numHarvesters = UnitsOwned[M_GALIOT];
	if(!DNA.buildMask.bHarvest) numHarvesters = 5;

	if (UnitsOwned[M_TAOS] < 2)
		pArchename = "GBOAT!!S_Taos";
	else
	if (UnitsOwned[M_AURORA] < 2 && (rand() % 2))
		pArchename = "GBOAT!!S_Aurora";
	else
	if (UnitsOwned[M_ORACLE] < DNA.uNumScouts)
		pArchename = "GBOAT!!S_Oracle";
	else
	if ((S32)UnitsOwned[M_ATLAS] < DNA.nNumMinelayers)
		pArchename = "MLAYER!!S_Atlas";
	else
	if ((S32)UnitsOwned[M_LEGIONAIRE] < DNA.nNumTroopships)
		pArchename = "TSHIP!!S_Legionare";
	else 	// 2 for each escort, plus 1 for scouting
	if (UnitsOwned[M_TAOS]+UnitsOwned[M_AURORA] < (numHarvesters * 2) + 30)  //maybe m_HarvestEscorts figures in here
		pArchename = (rand() % 2) ? "GBOAT!!S_Taos" : "GBOAT!!S_Aurora";
	
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
void MSolarianAI::doHeavyInd (MPart & part)
{
	const char * pArchename = 0;
	U32 archID;

	if(DNA.buildMask.bBuildMediumGunboats && (!DNA.buildMask.bBuildHeavyGunboats || (rand() & 3)))
	{
		if (UnitsOwned[M_POLARIS] < 2)
			pArchename = "GBOAT!!S_Polaris";
		else if (UnitsOwned[M_TRIREME] < 2)
			pArchename = "GBOAT!!S_Trireme";
		else 
		if (UnitsOwned[M_MONOLITH] < 3)
			pArchename = "GBOAT!!S_Monolith";
		else
			pArchename = (rand() % 2) ? "GBOAT!!S_Trireme" : "GBOAT!!S_Monolith";
	}
	else if(DNA.buildMask.bBuildHeavyGunboats)
	{
		if (UnitsOwned[M_MONOLITH] < 3)
			pArchename = "GBOAT!!S_Monolith";
	}
	//if (UnitsOwned[M_ATLAS] < 3)
	//	pArchename = "GBOAT!!M_Atlas";   //this is what the archname should be, but its not in ADB
	//else

	if(!pArchename) return;

	archID = ARCHLIST->GetArchetypeDataID(pArchename);
	U32 fail = 0;
	if(!CanIBuild(archID, &fail))
	{
		if(fail > M_COMMANDPTS)
		{
			CQASSERT(M_COMMANDPTS + fail < M_ENDOBJCLASS);
			BuildDesires[M_COMMANDPTS + fail].nPriority += 25;  //now go put a bp on updatebuilddesires
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

		m_AttackWaveSize--;
	}
}
*/
//---------------------------------------------------------------------------//
//   relies on parent
//---------------------------------------------------------------------------//
/*
void MSolarianAI::doSupplyShip (MPart & part)
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
		if ((other = obj).isValid() && other->systemID==systemID && other->playerID==playerID && (other->mObjClass==M_HIGHCOUNSEL || other->mObjClass==M_TRIREME))
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
void __stdcall CreateStandardSolarianPlayerAI (ISPlayerAI ** ppPlayerAI, U32 playerID, M_RACE race)
{
	MSolarianAI * player = new MSolarianAI;
	*ppPlayerAI = player;
	player->init(playerID, race);
}
//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//
//-------------------------END MStandardSolarianAI.cpp-------------------------//
//---------------------------------------------------------------------------//
