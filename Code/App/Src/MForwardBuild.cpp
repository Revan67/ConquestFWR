//--------------------------------------------------------------------------------//
//                                                                                //
//                               MForwardBuild.cpp                                //
//                                                                                //
//                  COPYRIGHT (C) 2000 BY DIGITAL ANVIL, INC.                     //
//                                                                                //
//--------------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/MForwardBuild.cpp 58    10/11/00 3:19p Ahunter $
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
//-------------------------------------------
struct MForwardBuild : public DAComponent<SPlayerAI>
{
	/* corvette rush data items */
	

	
	/* end of data items */
	
	virtual void		init (U32 playerID, M_RACE race);
	virtual void		initBuildDesires(void);
	virtual void		initResearchDesires(bool bFirstTime = false);
	virtual void		initShipDesires(void);

	virtual bool		ReadyForMilitary(void);

//	virtual void						onIdleUnit (IBaseObject * obj);
//	virtual void						evaluate (void);
	
	/* MForwardBuild methods */

	//using parent methods
	//void		doHarvest (MPart & part);
	//void		doSupplyShip (MPart & part);
	//void		doFabricator (MPart & part);
	//void		doHQ (MPart & part);
	//virtual void		doLight (MPart & part);
	//virtual void		doHeavyInd (MPart & part);
	//void		doResupply (MPart & part);
	//bool		doScouting (MPart & part);
	//void		doFlagShip (MPart & part);
	//void		doAttack (ASSIGNMENT * assign);
	//void		assignGunboat (MPart & part);
	//void		ensureAssignmentFor (MPart & part);


};
//--------------------------------------------------------------------------//
//
void MForwardBuild::init (U32 _playerID, M_RACE race)
{
	SPlayerAI::init(_playerID, race);

	CQASSERT(race == M_TERRAN);
	strategy = TERRAN_FORWARD_BUILD;

	AIPersonality settings = DNA;
	settings.nNumFleets = 3;
	settings.nHarvestEscorts = 0;		//number of gunboats to use to escort each harvester
	settings.uNumScouts = 1 + (SECTOR->GetNumSystems() / 4);
	settings.nNumTroopships = rand() & 1;  //could be based on difficulty level  fix  
	settings.nGunboatsPerSupplyShip = 6;
	settings.nNumFabricators = 4;
	settings.nFabricateEscorts = 2;
	settings.nBuildPatience = 7;
	SetPersonality(settings);

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
void MForwardBuild::initBuildDesires(void)
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

		if(c == M_HQ)
		{
			cur->nPriority = 3550;
			cur->nPlanetMultiplier = 0;
			cur->nSystemMultiplier = 0;
			cur->nAdditiveDecrement = 0;
			cur->nExponentialDecrement = 4;
			cur->prerequisite = M_NONE;
//			cur->nNumSlots = 3;  
		}

		if(c == M_REFINERY)
		{
			cur->nPriority = 750;
			cur->nPlanetMultiplier = 1;
			cur->nAdditiveDecrement = 235;
			cur->nExponentialDecrement = 0;
			cur->prerequisite = M_HQ;
//			cur->nNumSlots = 3;
		}

		if(c == M_HEAVYREFINERY)
		{
			cur->nPriority = -5000;
		}

		if(c == M_SUPERHEAVYREFINERY)
		{
			cur->nPriority = -5000;
		}
		
		if(c == M_LIGHTIND)
		{
			cur->nPriority = 550;
			cur->nPlanetMultiplier = 0;
			cur->nAdditiveDecrement = 140;
			cur->nExponentialDecrement = 0;
			cur->prerequisite = M_REFINERY;
//			cur->nNumSlots = 3;
		}

		if(c == M_TENDER)
		{
			cur->nPriority = 450;
			cur->nPlanetMultiplier = 0;
			cur->nAdditiveDecrement = 0;
			cur->nExponentialDecrement = 3;
			cur->prerequisite = M_REFINERY;
//			cur->nNumSlots = 2;
		}

		if(c == M_REPAIR)
		{
			cur->nPriority = 90;
			cur->nPlanetMultiplier = 0;
			cur->nAdditiveDecrement = 400;
			cur->nExponentialDecrement = 0;
			cur->prerequisite = M_TENDER;
			//cur->nNumSlots = 2;
		}

		if(c == M_OUTPOST)
		{
			cur->nPriority = 575;
			cur->nPlanetMultiplier = 0;
			cur->nAdditiveDecrement = 280;
			cur->nExponentialDecrement = 0;
			cur->prerequisite = M_HQ;
//			cur->nNumSlots = 2;
		}

		if(c == M_ACADEMY)
		{
			cur->nPriority = 610;
			cur->nPlanetMultiplier = 0;
			cur->nAdditiveDecrement = 280;
			cur->nExponentialDecrement = 0;
			cur->prerequisite = M_OUTPOST;
//			cur->nNumSlots = 2;
		}

		if(c == M_BALLISTICS)
		{
			cur->nPriority = 500;
			cur->nPlanetMultiplier = 0;
			cur->nAdditiveDecrement = 0;
			cur->nExponentialDecrement = 20;
			cur->prerequisite = M_LIGHTIND;
//			cur->nNumSlots = 1;
		}

		if(c == M_ADVHULL)
		{
			cur->nPriority = 500;
			cur->nPlanetMultiplier = 0;
			cur->nAdditiveDecrement = 0;
			cur->nExponentialDecrement = 2;
			cur->prerequisite = M_BALLISTICS;
//			cur->nNumSlots = 1;
		}

		if(c == M_HEAVYIND)
		{
			cur->nPriority = 2770;
			cur->nPlanetMultiplier = 0;
			cur->nAdditiveDecrement = 0;
			cur->nExponentialDecrement = 12;
			cur->prerequisite = M_ADVHULL;
//			cur->nNumSlots = 4;
		}

		if(c == M_PROPLAB)
		{
			cur->nPriority = 500;
			cur->nPlanetMultiplier = 0;
			cur->nAdditiveDecrement = 1000;
			cur->nExponentialDecrement = 0;
			cur->prerequisite = M_ADVHULL;
//			cur->nNumSlots = 1;
		}

		if(c == M_DISPLAB)
		{
			cur->nPriority = 500;
			cur->nPlanetMultiplier = 0;
			cur->nAdditiveDecrement = 1000;
			cur->nExponentialDecrement = 0;
			cur->prerequisite = M_ADVHULL;
//			cur->nNumSlots = 1;
		}

		if(c == M_AWSLAB)
		{
			cur->nPriority = 750;
			cur->nPlanetMultiplier = 0;
			cur->nAdditiveDecrement = 0;
			cur->nExponentialDecrement = 4;
			cur->prerequisite = M_HEAVYIND;
//			cur->nNumSlots = 2;
		}

		if(c == M_LRSENSOR)
		{
			cur->nPriority = 410;
			cur->nPlanetMultiplier = 0;
			cur->nAdditiveDecrement = 65;
			cur->nExponentialDecrement = 0;
			cur->prerequisite = M_HQ;
//			cur->nNumSlots = 1;
		}

		if(c == M_HANGER)
		{
			cur->nPriority = 420;
			cur->nPlanetMultiplier = 0;
			cur->nAdditiveDecrement = 0;
			cur->nExponentialDecrement = 3;
			cur->prerequisite = M_ACADEMY;
//			cur->nNumSlots = 1;
		}

		if(c == M_IONCANNON)
		{
			cur->nPriority = 170;
			cur->nPlanetMultiplier = 0;
			cur->nAdditiveDecrement = 30;
			cur->nExponentialDecrement = 0;
			cur->prerequisite = M_AWSLAB;
//			cur->nNumSlots = 2;
		}

		if(c == M_LSAT)
		{
			cur->nPriority = 280;  //50
			cur->nPlanetMultiplier = 0;
			cur->nAdditiveDecrement = 10;
			cur->nExponentialDecrement = 0;
			cur->prerequisite = M_HQ;
//			cur->nNumSlots = 0;
		}
		
		if(c == M_SPACESTATION)
		{
			cur->nPriority = 200;
			cur->nPlanetMultiplier = 0;
			cur->nAdditiveDecrement = 75;
			cur->nExponentialDecrement = 0;
			cur->prerequisite = M_HANGER;
//			cur->nNumSlots = 0;
		}

		if(c == M_JUMPPLAT)
		{
			cur->nPriority = 600;            //haven't seen this working yet
			cur->nPlanetMultiplier = 0;
			cur->nSystemMultiplier = 1;
			cur->nAdditiveDecrement = 250;
			cur->nExponentialDecrement = 0;
			cur->prerequisite = M_REFINERY;
			cur->nNumSlots = 1;
		}
	}

}
//--------------------------------------------------------------------------//
// research priorities
//--------------------------------------------------------------------------//
void MForwardBuild::initResearchDesires(bool bFirstTime)
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
		case T_BATTLESHIPCHARGE:
			cur->nPriority = 1240;//temporarily high for testing
			cur->prerequisite = NOTECH;
			cur->facility = M_AWSLAB;
			break;
		case T_CARRIERPROBE:
			cur->nPriority = 250;//temporarily high for testing
			cur->prerequisite = NOTECH;
			cur->facility = M_AWSLAB;
			break;
		case T_DREADNOUGHTSHIELD:
			cur->nPriority = 1250;//temporarily high for testing
			cur->prerequisite = NOTECH;
			cur->facility = M_AWSLAB;
			break;
		case T_ENGINEUPGRADE1:
			cur->nPriority = 50; // +rand() & 7
			cur->prerequisite = NOTECH;
			cur->facility = M_PROPLAB;
			break;
		case T_ENGINEUPGRADE2:
			cur->nPriority = 50; // +rand() & 7
			cur->prerequisite = T_ENGINEUPGRADE1;
			cur->facility = M_PROPLAB;
			break;
		case T_ENGINEUPGRADE3:
			cur->nPriority = 55; // +rand() & 7
			cur->prerequisite = T_ENGINEUPGRADE2;
			cur->facility = M_PROPLAB;
			break;
		case T_ENGINEUPGRADE4:
			cur->nPriority = 60; // +rand() & 7
			cur->prerequisite = T_ENGINEUPGRADE3;
			cur->facility = M_PROPLAB;
			break;
		case T_FIGHTER1:
			cur->nPriority = 20; // +rand() & 7
			cur->prerequisite = NOTECH;
			cur->facility = M_HANGER;
			break;
		case T_FIGHTER2:
			cur->nPriority = 25; // +rand() & 7
			cur->prerequisite = T_FIGHTER1;
			cur->facility = M_HANGER;
			break;
		case T_FIGHTER3:
			cur->nPriority = 30; // +rand() & 7
			cur->prerequisite = T_FIGHTER2;
			cur->facility = M_HANGER;
			break;
		case T_HULLUPGRADE1:
			cur->nPriority = 50; // +rand() & 7
			cur->prerequisite = NOTECH;
			cur->facility = M_ADVHULL;
			break;
		case T_HULLUPGRADE2:
			cur->nPriority = 55; // +rand() & 7
			cur->prerequisite = T_HULLUPGRADE1;
			cur->facility = M_ADVHULL;
			break;
		case T_HULLUPGRADE3:
			cur->nPriority = 58; // +rand() & 7
			cur->prerequisite = T_HULLUPGRADE2;
			cur->facility = M_ADVHULL;
			break;
		case T_HULLUPGRADE4:
			cur->nPriority = 60; // +rand() & 7
			cur->prerequisite = T_HULLUPGRADE3;
			cur->facility = M_ADVHULL;
			break;
		case T_LANCERVAMPIRE:
			cur->nPriority = 200;//temporarily high for testing
			cur->prerequisite = NOTECH;
			cur->facility = M_AWSLAB;
			break;
		case T_MISSILECLOAKING:
			cur->nPriority = 233; //temporarily high for testing
			cur->prerequisite = NOTECH;
			cur->facility = M_BALLISTICS;
			break;
		case T_MISSILEPAK1:
			cur->nPriority = 60; // +rand() & 7
			cur->prerequisite = NOTECH;
			cur->facility = M_BALLISTICS;
			break;
		case T_MISSILEPAK2:
			cur->nPriority = 62; // +rand() & 7
			cur->prerequisite = T_MISSILEPAK1;
			cur->facility = M_BALLISTICS;
			break;
			/*
		case T_MISSILEPAK3:
			cur->nPriority = 64; // +rand() & 7
			cur->prerequisite = T_MISSILEPAK2;
			cur->facility = M_BALLISTICS;
			break;
			*/

		case T_GAS1:
			cur->nPriority = 1; // +rand() & 7
			cur->prerequisite = NOTECH;
			cur->facility = M_REFINERY;
			break;
		case T_GAS2:
			cur->nPriority = 10; // +rand() & 7
			cur->prerequisite = T_GAS1;
			cur->facility = M_REFINERY;
			break;
		case T_GAS3:
			cur->nPriority = 15; // +rand() & 7
			cur->prerequisite = T_GAS2;
			cur->facility = M_REFINERY;
			break;
		case T_ORE1:
			cur->nPriority = 5; // +rand() & 7
			cur->prerequisite = NOTECH;
			cur->facility = M_REFINERY;
			break;
		case T_ORE2:
			cur->nPriority = 6; // +rand() & 7
			cur->prerequisite = T_ORE1;
			cur->facility = M_REFINERY;
			break;
		case T_ORE3:
			cur->nPriority = 7; // +rand() & 7
			cur->prerequisite = T_ORE2;
			cur->facility = M_REFINERY;
			break;

		case T_SENSOR1:
			cur->nPriority = 400; // +rand() & 7
			cur->prerequisite = NOTECH;
			cur->facility = M_LRSENSOR;
			break;
		case T_SENSOR2:
			cur->nPriority = 210; // +rand() & 7
			cur->prerequisite = T_SENSOR1;
			cur->facility = M_LRSENSOR;
			break;
		case T_SHIELDUPGRADE1:
			cur->nPriority = 100; // +rand() & 7
			cur->prerequisite = NOTECH;
			cur->facility = M_DISPLAB;
			break;
		case T_SHIELDUPGRADE2:
			cur->nPriority = 99; // +rand() & 7
			cur->prerequisite = T_SHIELDUPGRADE1;
			cur->facility = M_DISPLAB;
			break;
		case T_SHIELDUPGRADE3:
			cur->nPriority = 98; // +rand() & 7
			cur->prerequisite = T_SHIELDUPGRADE2;
			cur->facility = M_DISPLAB;
			break;
		case T_SHIELDUPGRADE4:
			cur->nPriority = 97; // +rand() & 7
			cur->prerequisite = T_SHIELDUPGRADE3;
			cur->facility = M_DISPLAB;
			break;
		case T_SUPPLYUPGRADE1:
			cur->nPriority = 100; // +rand() & 7
			cur->prerequisite = NOTECH;
			cur->facility = M_REFINERY;
			break;
		case T_SUPPLYUPGRADE2:
			cur->nPriority = 110; // +rand() & 7
			cur->prerequisite = T_SUPPLYUPGRADE1;
			cur->facility = M_REFINERY;
			break;
		case T_SUPPLYUPGRADE3:
			cur->nPriority = 111; // +rand() & 7
			cur->prerequisite = T_SUPPLYUPGRADE2;
			cur->facility = M_REFINERY;
			break;
		case T_SUPPLYUPGRADE4:
			cur->nPriority = 120; // +rand() & 7
			cur->prerequisite = T_SUPPLYUPGRADE3;
			cur->facility = M_REFINERY;
			break;
		case T_TANKER1:
			cur->nPriority = 10 + (DNA.nMaxHarvesters * 15); // +rand() & 7
			cur->prerequisite = NOTECH;
			cur->facility = M_REFINERY;
			break;
		case T_TANKER2:
			cur->nPriority = 9 + (DNA.nMaxHarvesters * 15); // +rand() & 7
			cur->prerequisite = T_TANKER1;
			cur->facility = M_REFINERY;
			break;
		case T_TENDER1:
			cur->nPriority = 15; // +rand() & 7
			cur->prerequisite = NOTECH;
			cur->facility = M_REFINERY;//M_TENDER;
			break;
		case T_TENDER2:
			cur->nPriority = 14; // +rand() & 7
			cur->prerequisite = T_TENDER1;
			cur->facility = M_REFINERY;//M_TENDER;
			break;
		case T_TROOPSHIP1:
			cur->nPriority = -100 + (DNA.nNumTroopships * 500); // +rand() & 7
			cur->prerequisite = NOTECH;
			cur->facility = M_OUTPOST;
			break;
		case T_TROOPSHIP2:
			cur->nPriority = -101 + (DNA.nNumTroopships * 400); // +rand() & 7
			cur->prerequisite = T_TROOPSHIP1;
			cur->facility = M_OUTPOST;
			break;
		case T_TROOPSHIP3:
			cur->nPriority = -50 + (DNA.nNumTroopships * 300); // +rand() & 7
			cur->prerequisite = T_TROOPSHIP2;
			cur->facility = M_OUTPOST;
			break;
		case T_WEAPONUPGRADE1:
			cur->nPriority = 200; // +rand() & 7
			cur->prerequisite = NOTECH;
			cur->facility = M_BALLISTICS;
			break;
		case T_WEAPONUPGRADE2:
			cur->nPriority = 205; // +rand() & 7
			cur->prerequisite = T_WEAPONUPGRADE1;
			cur->facility = M_BALLISTICS;
			break;
		case T_WEAPONUPGRADE3:
			cur->nPriority = 210; // +rand() & 7
			cur->prerequisite = T_WEAPONUPGRADE2;
			cur->facility = M_BALLISTICS;
			break;
		case T_WEAPONUPGRADE4:
			cur->nPriority = 160; // +rand() & 7
			cur->prerequisite = T_WEAPONUPGRADE3;
			cur->facility = M_BALLISTICS;
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
void MForwardBuild::initShipDesires(void)
{
	SPlayerAI::initShipDesires();

	for(int c = 0; c < M_ENDOBJCLASS; c++)
	{
		ShipPriority * cur = &ShipDesires[c];
		
		if(c == M_CORVETTE)
		{
			cur->nPriority = 400;
			cur->nAdditiveDecrement = 48;
			cur->nExponentialDecrement = 0;
			cur->facility = M_LIGHTIND;
		}

		if(c == M_MISSILECRUISER)
		{
			cur->nPriority = 300 + (DNA.buildMask.bRandomBit1 * 288);
			cur->nAdditiveDecrement = 75;
			cur->nExponentialDecrement = 0;
			cur->facility = M_LIGHTIND;
		}

		if(c == M_BATTLESHIP)
		{
			cur->nPriority = 880;
			cur->nAdditiveDecrement = 27 + (DNA.buildMask.bRandomBit2 * 100);
			cur->nExponentialDecrement = 0;
			cur->facility = M_HEAVYIND;
		}

		if(c == M_DREADNOUGHT)
		{
			cur->nPriority = 750;
			cur->nAdditiveDecrement = (rand() & 127);
			cur->nExponentialDecrement = 0;
			cur->facility = M_HEAVYIND;
		}

		if(c == M_CARRIER)
		{
			cur->nPriority = 650;
			cur->nAdditiveDecrement = 100 + (rand() & 127);
			cur->nExponentialDecrement = 0;
			cur->facility = M_HEAVYIND;
		}

		if(c == M_LANCER)
		{
			cur->nPriority = 680;
			cur->nAdditiveDecrement = 25 + (DNA.buildMask.bRandomBit3 * 100);
			cur->nExponentialDecrement = 0;
			cur->facility = M_HEAVYIND;
		}

		if(c == M_INFILTRATOR)
		{
			cur->nPriority = SIGHTSHIP_PRI + DNA.uNumScouts * 200;
			cur->nAdditiveDecrement = 400;
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
//  should i build gunboats or save resource for a platform i need?
//---------------------------------------------------------------------------//
bool MForwardBuild::ReadyForMilitary(void)
{
	if((UnitsOwned[M_INFILTRATOR] + UnitsOwned[M_SEEKER] + UnitsOwned[M_ORACLE]) == 0)
		return true;

	U32 refs = 0;

	if(!UnitsOwned[M_OUTPOST] && !UnitsOwned[M_ACADEMY]) return false;
	if(!UnitsOwned[M_HQ]) return false;
	refs = UnitsOwned[M_REFINERY] + UnitsOwned[M_HARVEST];
	if((refs < 3) && (m_Age < 100)) return false;

	if((UnitsOwned[M_CORVETTE] > ((DNA.nFabricateEscorts * UnitsOwned[M_FABRICATOR] +
		DNA.nHarvestEscorts * UnitsOwned[M_HARVEST]) + 4)) &&
		(UnitsOwned[M_HEAVYIND] == 0))
		return false;

	int resfail = 0, buildfail = 0;
	ChooseNextTech(&resfail);
	ChooseNextBuild(&buildfail, m_nRace, true);
	if((buildfail + resfail > (1500 - ((m_TotalMilitaryUnits * m_TotalMilitaryUnits) / 2))) && ((m_Metal < 2100) || (m_Gas < 1700)))
		return false;

	if((rand() % ((m_TotalMilitaryUnits + 1) * 2)) > 15)
		return false;

	if(m_ComPtsUsedUp >= 99)
		return false;

	return true;
}
/*
//---------------------------------------------------------------------------//
//
void MForwardBuild::doLight (MPart & part)
{
	const char * pArchename = 0;
	U32 archID;

	U32 numHarvesters = UnitsOwned[M_HARVEST];
	if(!DNA.buildMask.bHarvest) numHarvesters = 5;

	U32 corvs_wanted = ((UnitsOwned[M_HARVEST] * DNA.nHarvestEscorts) + 
		(UnitsOwned[M_FABRICATOR] * DNA.nFabricateEscorts));

	if (UnitsOwned[M_CORVETTE] < 2)
		pArchename = "GBOAT!!T_Corvette";
	else
	if (UnitsOwned[M_INFILTRATOR] < DNA.uNumScouts)
		pArchename = "GBOAT!!T_Infiltrator";
	else
	if ((S32)UnitsOwned[M_TROOPSHIP] < DNA.nNumTroopships)
		pArchename = "TSHIP!!T_Troopship";
	else
	if (UnitsOwned[M_CORVETTE] < 2 + corvs_wanted)
		pArchename = "GBOAT!!T_Corvette";
	else
	if (UnitsOwned[M_HEAVYIND] < 1)
	{
		if (UnitsOwned[M_CORVETTE]+UnitsOwned[M_MISSILECRUISER] < (numHarvesters * 2) + 30)  //maybe m_HarvestEscorts figures in here
			pArchename = (rand() % 2) ? "GBOAT!!T_Corvette" : "GBOAT!!T_Missile Cruiser";
	}
	
	if(!pArchename) return;

	archID = ARCHLIST->GetArchetypeDataID(pArchename);

	U32 fail = 0;
	if(!CanIBuild(archID, &fail))
	{
		if(fail > M_COMMANDPTS)
		{
dfggf			CQASSERT(M_COMMANDPTS + fail < M_ENDOBJCLASS);
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
void MForwardBuild::doHeavyInd (MPart & part)
{
	const char * pArchename = 0;
	U32 archID;

	pArchename = "GBOAT!!T_Dreadnought";
	archID = ARCHLIST->GetArchetypeDataID(pArchename);

	U32 fail = 0;
	if(!CanIBuild(archID, &fail))
	{
		if(fail > M_COMMANDPTS)
		{
dfggd			CQASSERT(M_COMMANDPTS + fail < M_ENDOBJCLASS);
dfggf			BuildDesires[M_COMMANDPTS + fail].nPriority += 25;
		}
	
		S32 total = UnitsOwned[M_HARVEST];
		if(DNA.buildMask.bHarvest) total = 5;

		pArchename = NULL;

		if(DNA.buildMask.bBuildMediumGunboats)
		{
			if (((S32)UnitsOwned[M_BATTLESHIP]) < total / 3)
				pArchename = "GBOAT!!T_Midas Battleship";
			else if (((S32)UnitsOwned[M_LANCER]) < total / 2)
				pArchename = "GBOAT!!T_Lancer Cruiser";
		}
		
		if(pArchename == NULL && DNA.buildMask.bBuildHeavyGunboats)
		{
			if (((S32)UnitsOwned[M_CARRIER]) < total / 2)
				pArchename = "GBOAT!!T_Fleet Carrier";
		}

		if(!pArchename) return;
		archID = ARCHLIST->GetArchetypeDataID(pArchename);
		U32 fail;
		if(!CanIBuild(archID, &fail)) return;
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
//----------------------------------------------------------------------------------------//
//
void __stdcall CreateForwardBuildPlayerAI (ISPlayerAI ** ppPlayerAI, U32 playerID, M_RACE race)
{
	MForwardBuild * player = new MForwardBuild;
	*ppPlayerAI = player;
	player->init(playerID, race);
}
//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//
//-------------------------END MForwardBuild.cpp-----------------------------//
//---------------------------------------------------------------------------//



/*
void MForwardBuild::doGunsat (MPart & part)
{
	if (part->caps.deployOk)
	{
		U32 systemID = part->systemID;
		int rcnt = rand() & 7;
		IBaseObject *gates[8];
		int numGates=0;

		// scout out this system first
		IBaseObject * obj = OBJLIST->GetTargetList();     // mission objects
		while (obj)
		{
			if (obj->objClass == OC_JUMPGATE)
			{
				const U32 objSystemID = obj->GetSystemID();

				if (objSystemID == systemID && numGates < 8)
				{
					gates[numGates++] = obj;
					if (rcnt-- == 0)
					{
						rcnt=numGates-1;
						break;
					}
				}
			}
			
			obj = obj->nextTarget;
		} // end while()

		if (numGates)
		{
			rcnt = rcnt % numGates;
			
			// calculate actual scout position
			RECT rect;
			SECTOR->GetSystemRect(systemID, &rect);
			Vector vec;
			OBJBOX box;
			USR_PACKET<USRDEPLOY> packet;

			// watch your step!
			if (numGates > 0)
			{
				const TRANSFORM & trans = gates[rcnt]->GetTransform();
				vec.x = (rect.right/2) - trans.translation.x;
				vec.y = (rect.top / 2) - trans.translation.y;
				vec.z = 0;
				vec.fast_normalize();
				gates[rcnt]->GetObjectBox(box);
				vec *= (box[0]+500);	//maxx
				vec += trans.translation;
			}
			else	// choose center of system
			{
				vec.x = (rect.right/2);
				vec.y = (rect.top / 2);
				vec.z = 0;
			}

			packet.userBits = 0;
			packet.position.init(vec, systemID);
			packet.objectID[0] = part->dwMissionID;
			packet.init(1);
			NETPACKET->Send(HOSTID,0,&packet);
		}
	}
}
*/  // is this shit useful?
