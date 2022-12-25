//--------------------------------------------------------------------------------//
//                                                                                //
//                                 MDeny.cpp			                          //
//                                                                                //
//                  COPYRIGHT (C) 2000 BY DIGITAL ANVIL, INC.                     //
//                                                                                //
//--------------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/MDeny.cpp 15    10/18/00 4:50p Ahunter $
*/			    
//--------------------------------------------------------------------------------//


#include "pch.h"
#include <globals.h>
#include "MGlobals.h"
#include <CQTrace.h>

#include "SPlayerAI.h"
#include "ObjList.h"
#include "IObject.h"
#include "ObjMap.h"
#include "ObjMapIterator.h"
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
struct MDeny : public DAComponent<SPlayerAI>
{
	/* mantis AI data items */
	

	
	/* end of data items */
	
	virtual void	init (U32 playerID, M_RACE race);
	virtual void	initBuildDesires(void);
	virtual void	initResearchDesires(bool bFirstTime = false);
	virtual void	initShipDesires(void);


	virtual S32				CalcWaveSize(void);
	virtual IBaseObject *	findStrategicTarget (MPart & part, bool bNewTarget = false, bool bIgnoreDistance = false);
	//virtual void	onIdleUnit (IBaseObject * obj);  //using parent method
	
	/* MSolarianAI methods */
	//virtual void			doSupplyShip (MPart & part);
	//virtual void			doFabricator (MPart & part);
	//virtual void			doHQ (MPart & part);
	//virtual NETGRIDVECTOR	ChooseFreeBuildSite(PARCHETYPE pArchetype, U32 dwFabID, U32 systemID, Vector position, bool wormgen = false);
	//virtual void			doLight (MPart & part);
	//virtual void			doHeavyInd (MPart & part);
	//virtual void			assignGunboat (MPart & part);
};
//--------------------------------------------------------------------------//
//
void MDeny::init (U32 _playerID, M_RACE race)
{
	SPlayerAI::init(_playerID, race);
	
	CQASSERT(race == M_SOLARIAN);
	strategy = SOLARIAN_DENY;

	AIPersonality settings = DNA;
	settings.buildMask.bSendTaunts = true;
	settings.buildMask.bResignationPossible = true;
	settings.buildMask.bVisibilityRules = true;
	settings.nNumFleets = 1;
	//settings.nHarvestEscorts = 1;		//number of gunboats to use to escort each harvester
	settings.uNumScouts = 3 + (SECTOR->GetNumSystems() / 4);  //should be based on map size  fix  
	settings.nNumTroopships = 0;  //could be based on difficulty level  fix  
	settings.nGunboatsPerSupplyShip = 6;
	settings.nBuildPatience = 5;
	settings.nNumFabricators = 2;
	settings.nNumMinelayers = rand() % 2;
	settings.nFabricateEscorts = 0;
	SetPersonality(settings);
	
	m_AttackWaveSize = CalcWaveSize();
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
void MDeny::initBuildDesires(void)
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
			cur->nPriority = 680;
			cur->nPlanetMultiplier = 1;
			cur->nAdditiveDecrement = 255;
			cur->nExponentialDecrement = 0;
			cur->prerequisite = M_ACROPOLIS;
//			cur->nNumSlots = 3;
		}
		
		if(c == M_PAVILION)
		{
			cur->nPriority = 790;
			cur->nPlanetMultiplier = 0;
			cur->nAdditiveDecrement = 0;
			cur->nExponentialDecrement = 5;
			cur->prerequisite = M_OXIDATOR;
//			cur->nNumSlots = 3;
		}

		if(c == M_SENTINELTOWER)
		{
			cur->nPriority = 550;
			cur->nPlanetMultiplier = 0;
			cur->nAdditiveDecrement = 75;
			cur->nExponentialDecrement = 0;
			cur->prerequisite = M_ACROPOLIS;
//			cur->nNumSlots = 2;
		}

		if(c == M_EUTROMILL)
		{
			cur->nPriority = 400;
			cur->nPlanetMultiplier = 0;
			cur->nAdditiveDecrement = 90;
			cur->nExponentialDecrement = 0;
			cur->prerequisite = M_PAVILION;
//			cur->nNumSlots = 2;
		}

		if(c == M_BUNKER)
		{
			cur->nPriority = 765;
			cur->nPlanetMultiplier = 0;
			cur->nAdditiveDecrement = 195;
			cur->nExponentialDecrement = 0;
			cur->prerequisite = M_OXIDATOR;
//			cur->nNumSlots = 1;
		}

		if(c == M_CITADEL)
		{
			cur->nPriority = 200;
			cur->nPlanetMultiplier = 0;
			cur->nAdditiveDecrement = 287;
			cur->nExponentialDecrement = 0;
			cur->prerequisite = M_BUNKER;
//			cur->nNumSlots = 2;
		}

		if(c == M_HELIONVEIL)
		{
			cur->nPriority = 600;
			cur->nPlanetMultiplier = 0;
			cur->nAdditiveDecrement = 480;
			cur->nExponentialDecrement = 0;
			cur->prerequisite = M_SENTINELTOWER;
//			cur->nNumSlots = 1;
		}

		if(c == M_XENOCHAMBER)
		{
			cur->nPriority = 960;
			cur->nPlanetMultiplier = 0;
			cur->nAdditiveDecrement = 0;
			cur->nExponentialDecrement = 4;
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
			cur->nPriority = 1125;            
			cur->nPlanetMultiplier = 0;
			cur->nAdditiveDecrement = 1000;
			cur->nExponentialDecrement = 0;
			cur->prerequisite = M_GREATERPAVILION;
//			cur->nNumSlots = 1;
		}
		
		if(c == M_TALOREANMATRIX)
		{
			cur->nPriority = 515;            
			cur->nPlanetMultiplier = 0;
			cur->nAdditiveDecrement = 170 + (DNA.buildMask.bRandomBit1 * 100);
			cur->nExponentialDecrement = 0;
			cur->prerequisite = M_XENOCHAMBER;
//			cur->nNumSlots = 2;
		}

		if(c == M_GREATERPAVILION)
		{
			cur->nPriority = 560;            
			cur->nPlanetMultiplier = 0;
			cur->nAdditiveDecrement = 1500;
			cur->nExponentialDecrement = 0;
			cur->prerequisite = M_PAVILION;
//			cur->nNumSlots = 3;
		}

		if(c == M_PROTEUS)
		{
			cur->nPriority = 525;            
			cur->nPlanetMultiplier = 0;
			cur->nAdditiveDecrement = 25 + (DNA.buildMask.bRandomBit2 * 100);
			cur->nExponentialDecrement = 0;
			cur->prerequisite = M_PAVILION;
//			cur->nNumSlots = 0;
		}

		if(c == M_HYDROFOIL)
		{
			cur->nPriority = 850;            
			cur->nPlanetMultiplier = 0;
			cur->nAdditiveDecrement = 290 + (rand() & 127);
			cur->nExponentialDecrement = 0;
			cur->prerequisite = M_PROTEUS;
//			cur->nNumSlots = 0;
		}

		if(c == M_ESPCOIL)
		{
			cur->nPriority = 551;            
			cur->nPlanetMultiplier = 0;
			cur->nAdditiveDecrement = 200 + (rand() & 63);
			cur->nExponentialDecrement = 0;
			cur->prerequisite = M_SENTINELTOWER;
//			cur->nNumSlots = 0;
		}

		if(c == M_STARBURST)
		{
			cur->nPriority = 749;            
			cur->nPlanetMultiplier = 0;
			cur->nAdditiveDecrement = 150 + (DNA.buildMask.bRandomBit3 * 200);
			cur->nExponentialDecrement = 0;
			cur->prerequisite = M_TURBINEDOCK;
//			cur->nNumSlots = 0;
		}

		if(c == M_PORTAL)
		{
			cur->nPriority = 1049 + (DNA.buildMask.bRandomBit4 * 1000);
			cur->nPlanetMultiplier = 0;
			cur->nAdditiveDecrement = 2000;
			cur->nExponentialDecrement = 0;   //  fix  
			cur->prerequisite = M_XENOCHAMBER;
//			cur->nNumSlots = 0;
		}


		if(c == M_JUMPPLAT)
		{
			cur->nPriority = 500;
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
void MDeny::initResearchDesires(bool bFirstTime)
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
			cur->nPriority = 1050;
			cur->prerequisite = NOTECH;
			cur->facility = M_XENOCHAMBER;
			break;
		case S_CLOAKER:
			cur->nPriority = 500;
			cur->prerequisite = NOTECH;
			cur->facility = M_XENOCHAMBER;
			break;
		case S_ENGINE1:
			cur->nPriority = 450;//105 + (rand() & 7);
			cur->prerequisite = NOTECH;
			cur->facility = M_TURBINEDOCK;
			break;
		case S_ENGINE2:
			cur->nPriority = 405; // +rand() & 7
			cur->prerequisite = S_ENGINE1;
			cur->facility = M_TURBINEDOCK;
			break;
		case S_ENGINE3:
			cur->nPriority = 206;// +rand() & 7
			cur->prerequisite = S_ENGINE2;
			cur->facility = M_TURBINEDOCK;
			break;
		case S_ENGINE4:
			cur->nPriority = 207;// +rand() & 7
			cur->prerequisite = S_ENGINE3;
			cur->facility = M_TURBINEDOCK;
			break;
		case S_ENGINE5:
			cur->nPriority = 208;// +rand() & 7
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
			cur->nPriority = 360; // +rand() & 7
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
			cur->nPriority = 630;//100 + rand() & 7;
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
			cur->nPriority = 500; // +rand() & 7
			cur->prerequisite = NOTECH;
			cur->facility = M_OXIDATOR;
			break;
		case S_SUPPLY2:
			cur->nPriority = 440; // +rand() & 7
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
			cur->nPriority = 395; // +rand() & 7 //what the hell is this? hehe
			cur->prerequisite = NOTECH;
			cur->facility = M_XENOCHAMBER;
			break;
		case S_TANKER1:
			cur->nPriority = 315; // +rand() & 7
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
			cur->nPriority = 260; // +rand() & 7
			cur->prerequisite = NOTECH;
			cur->facility = M_MUNITIONSANNEX;
			break;
		case S_WEAPON2:
			cur->nPriority = 220; // +rand() & 7
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
void MDeny::initShipDesires(void)
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
			cur->nPriority = 1380;
			cur->nAdditiveDecrement = 134;
			cur->nExponentialDecrement = 0;
			cur->facility = M_PAVILION;
		}

		if(c == M_POLARIS)
		{
			cur->nPriority = 600;
			cur->nAdditiveDecrement = 35 + (rand() & 31);
			cur->nExponentialDecrement = 0;
			cur->facility = M_PAVILION;
		}

		if(c == M_AURORA)
		{
			cur->nPriority = 550;
			cur->nAdditiveDecrement = 45;
			cur->nExponentialDecrement = 0;
			cur->facility = M_PAVILION;
		}

		if(c == M_ORACLE)
		{
			cur->nPriority = SIGHTSHIP_PRI - 350 + DNA.uNumScouts * 200;
			cur->nAdditiveDecrement = 400;
			cur->nExponentialDecrement = 0;
			cur->facility = M_PAVILION;
		}

		if(c == M_TRIREME)
		{
			cur->nPriority = 1970;
			cur->nAdditiveDecrement = 100 + (rand() & 127);
			cur->nExponentialDecrement = 0;
			cur->facility = M_GREATERPAVILION;
		}

		if(c == M_MONOLITH)
		{
			cur->nPriority = 1500;
			cur->nAdditiveDecrement = 140;
			cur->nExponentialDecrement = 0;
			cur->facility = M_GREATERPAVILION;
		}

	}

}
//---------------------------------------------------------------------------//
S32 MDeny::CalcWaveSize()
{
	S32 age = 3 + (m_Age / 44);
	if(age > 13) age = 13;
	S32 res = (rand() % 5) + age;
	return res;
}
//----------------------------------------------------------------------------------------//
//
IBaseObject * MDeny::findStrategicTarget (MPart & base, bool bNewTarget, bool bIgnoreDistance)
{
	//To force AI to concentrate attacks on specified things and areas
	if(!bNewTarget && m_StrategicTarget && (m_StrategicTargetRange == 0)) 
		return m_StrategicTarget;

	IBaseObject * obj = OBJLIST->GetTargetList();     // mission objects
	DOUBLE bestPoints=10e8;
	IBaseObject * bestPlatform=0;
	U32 systemID = base->systemID;
	if(systemID & HYPER_SYSTEM_MASK) systemID -= 128;
	GRIDVECTOR position = base.obj->GetGridPosition();
	//bool bBestIsVisible=DNA.buildMask.bVisibilityRules;
	bool bBestIsVisible = (((rand() & 15) != 0) && DNA.buildMask.bVisibilityRules);  //dictated by unit-level attacking code
	//bool savedtarg = false;
	MPart other;
	const U32 allyMask = MGlobals::GetAllyMask(playerID);

	DOUBLE points = 0.0;

	if(m_StrategicTarget)
	{
		systemID = m_StrategicTarget->GetSystemID();
		position = m_StrategicTarget->GetGridPosition();
		//position = base.obj->GetGridPosition();  //what was I thinking?
	}
	
	while (obj)
	{
		if (obj->objMapNode && (obj->objMapNode->flags & OM_TARGETABLE) && ((other=obj).isValid()))
		{
			U32 hisPlayerID = other->playerID;
			if (obj->objMapNode->flags & OM_MIMIC)
			{
				hisPlayerID = OBJMAP->GetApparentPlayerID(obj->objMapNode,allyMask);
			}
			
			if ((allyMask & (1 << (hisPlayerID-1)))==0 &&
				obj->IsTargetableByPlayer(playerID) && 
				(other->systemID && other->systemID <= MAX_SYSTEMS) && 
				other->bReady && 
				(other->dwMissionID != base->dwMissionID) &&
				(obj->fieldFlags.bCelsius == false))
			{
				
				const U32 objSystemID = other->systemID;
				M_OBJCLASS mobj = other->mObjClass;
				bool bIsVisible = obj->IsVisibleToPlayer(playerID);
				
				//find the distance
				if(bIgnoreDistance) points = 10e3;
				else
				{
					if (objSystemID == systemID)
						points = position-obj->GetGridPosition();
					else
						points = ((DISTTONEXTSYS*3) * getNumJumps(systemID, objSystemID, playerID));
				}
				
				if(m_StrategicTargetSystem && objSystemID != m_StrategicTargetSystem)
				{
					//points = MAX(10e8, points * 100.0);
					points *= 100.0;
				}
				
				if(m_StrategicTarget && points > ((DOUBLE)m_StrategicTargetRange))
				{
					points *= 100.0;
					//points = MAX(10e8, points * 100.0);
				}
				
				//if(other.obj->objClass == OC_PLATFORM) points /= 10.0;

				if(MGlobals::IsHQ(mobj))
				{
					if(m_bKillHQ) points *= 0.6;
					else points *= 0.9;
				}
				
				//this is to spread out attacks and keep the player having to manage multiple fronts
				//could make this a function of the higher AI difficulty settings only
				if(TargetAssigned(other->dwMissionID))
				{
					points *= 5.0;
					//points = MAX(10e8, points * 5.0);
					
					//consider remembering this obj and adding bonus points to other potential
					//targets based on their distance from this one (for the front-divider or 
					//pronged-attacker personalities fix 
				}
				if(MGlobals::IsShipyard(mobj)) points *= 0.6;
				if(mobj == M_TALOREANMATRIX) points *= 0.4;

				if(MGlobals::IsHeavyGunboat(mobj)) points *= 0.2;
				if(MGlobals::IsMediumGunboat(mobj)) points *= 0.5;
				if(MGlobals::IsRefinery(mobj)) points *= 0.3;

				if((MGlobals::IsHarvester(mobj)) || (MGlobals::IsFabricator(mobj)) || (mobj == M_JUMPPLAT))
				{
					DOUBLE ageFactor = ((300.0 - ((DOUBLE)m_Age)) / 30.0);
					if(ageFactor < -9.0) ageFactor = -9.0;
					if(mobj == M_JUMPPLAT)
						points *= 0.8;
					else
						points /= (10.0 + ageFactor);
				}

				if(!bIsVisible) points *= 4.0;
				if(m_Terminate == hisPlayerID) points /= 20.0;
				
				if(points < bestPoints && (bBestIsVisible==false || bIsVisible || m_Terminate))
				{
					bestPoints = points;
					bestPlatform = obj;
					bBestIsVisible = bIsVisible;
				}
			} 
			
			/*
			if(savedtarg) obj = NULL;
			else
			{
			obj = obj->nextTarget;
			if(!obj)
			{
			if(m_SavedTarget)
			{
			obj = FindObject(m_SavedTarget);
			savedtarg = true;
			}
			}
			}
			*/
		}
		obj = obj->nextTarget;
	} // end while()


	return bestPlatform;
}
//----------------------------------------------------------------------------------------//
//
/*
void MDeny::doHQ (MPart & part)
{
	U32 archID = 0;
	U32& fabs = UnitsOwned[M_FABRICATOR] + UnitsOwned[M_WEAVER] + UnitsOwned[M_FORGER];

	bool smartfab = (!fabs || ((UnitsOwned[M_HARVEST] + UnitsOwned[M_SIPHON] + UnitsOwned[M_GALIOT]) > (fabs + 3)) ||
					(m_Metal > 1700));

	if ((((S32)fabs) < DNA.nNumFabricators && smartfab) || (m_PlanetsUnderMyControl / 4 > fabs))
	{
		if;(part->mObjClass == M_HQ) archID = m_ArchetypeIDs[M_FABRICATOR];
		if(part->mObjClass == M_COCOON) archID = m_ArchetypeIDs[M_WEAVER];
		if(part->mObjClass == M_ACROPOLIS) archID = m_ArchetypeIDs[M_FORGER];
	}
	else
	if (DNA.nGunboatsPerSupplyShip &&
		(((S32)(UnitsOwned[M_SUPPLY] + UnitsOwned[M_ZORAP] + UnitsOwned[M_STRATUM])) < 
		((m_TotalMilitaryUnits - ((S32)DNA.uNumScouts)) / DNA.nGunboatsPerSupplyShip)))
	{
		if(part->mObjClass == M_HQ) archID = m_ArchetypeIDs[M_SUPPLY];
		if(part->mObjClass == M_COCOON) archID = m_ArchetypeIDs[M_ZORAP];
		if(part->mObjClass == M_ACROPOLIS) archID = m_ArchetypeIDs[M_STRATUM];
	}
	else
		return;
	
	CQASSERT(archID != 0);
	
	U32 fail = 0;
	if(!CanIBuild(archID, &fail))
	{
		if(fail > M_COMMANDPTS)
		{
			CQASSERT(((fail - M_COMMANDPTS) < M_ENDOBJCLASS) && ((fail - M_COMMANDPTS) >= 0));
			BuildDesires[fail - M_COMMANDPTS].nPriority += (55 + (rand() & 31));
		}
		return;
	}

	if (archID)
	{
		USR_PACKET<USRBUILD> packet;
		packet.cmd = USRBUILD::ADDIFEMPTY;
		packet.dwArchetypeID = archID;
		packet.objectID[0] = part.obj->GetPartID();
		packet.init(1);
		NETPACKET->Send(HOSTID,0,&packet);
	}
}
*/
//----------------------------------------------------------------------------------------//
//
void __stdcall CreateDenyPlayerAI (ISPlayerAI ** ppPlayerAI, U32 playerID, M_RACE race)
{
	MDeny * player = new MDeny;
	*ppPlayerAI = player;
	player->init(playerID, race);
}
//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//
//-------------------------END MDeny.cpp----------------------------------//
//---------------------------------------------------------------------------//
