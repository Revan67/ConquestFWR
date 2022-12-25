//--------------------------------------------------------------------------------//
//                                                                                //
//                                      SPlayerAI.cpp                             //
//                                                                                //
//                  COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.                     //
//                                                                                //
//--------------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/SPlayerAI.cpp 345   7/06/01 2:08p Tmauer $
*/			    
//--------------------------------------------------------------------------------//

#include "pch.h"
#include "stdio.h"
#include <globals.h>
#include "MGlobals.h"
#include <CQTrace.h>

#include "SPlayerAI.h"
#include "ObjList.h"
#include "IObject.h"
#include "ObjMap.h"
#include "ObjMapIterator.h"
#include "IBanker.h"
#include "IAttack.h"
#include "TerrainMap.h"
#include "MPart.h"
#include "Resource.h"
#include "BaseHotrect.h"
#include "OpAgent.h"
#include "IPlanet.h"
#include "Field.h"
#include "IJumpGate.h"
#include "IHarvest.h"
#include "INugget.h"
#include "Commpacket.h"
#include "ObjSet.h"
#include "Sector.h"
#include "GridVector.h"
#include "CQGAME.h"
#include "DCQGame.h"
#include "DResearch.h"
#include "ScrollingText.h"
#include "IAdmiral.h"
//#include "ScrollingText.cpp"

#include <DPlatform.h>
#include <DRefinePlat.h>
#include <DPlanet.h>

#include <TSmartPointer.h>
#include <FileSys.h>

using namespace CQGAMETYPES;

#define UPDATE_PERIOD   (30*U32(REALTIME_FRAMERATE))		// 30 seconds
//#define MAX_TEXT_LENGTH		100

void __stdcall CreateStandardMantisPlayerAI (ISPlayerAI ** ppPlayerAI, U32 playerID, M_RACE race);
void __stdcall CreateStandardSolarianPlayerAI (ISPlayerAI ** ppPlayerAI, U32 playerID, M_RACE race);
void __stdcall CreateForwardBuildPlayerAI (ISPlayerAI ** ppPlayerAI, U32 playerID, M_RACE race);
void __stdcall CreateDreadnoughtsPlayerAI (ISPlayerAI ** ppPlayerAI, U32 playerID, M_RACE race);
void __stdcall CreateCorvetteRushPlayerAI (ISPlayerAI ** ppPlayerAI, U32 playerID, M_RACE race);
void __stdcall CreateFortressPlayerAI (ISPlayerAI ** ppPlayerAI, U32 playerID, M_RACE race);
void __stdcall CreateSwarmPlayerAI (ISPlayerAI ** ppPlayerAI, U32 playerID, M_RACE race);
void __stdcall CreateFrigateRushPlayerAI (ISPlayerAI ** ppPlayerAI, U32 playerID, M_RACE race);
void __stdcall CreateForgersPlayerAI (ISPlayerAI ** ppPlayerAI, U32 playerID, M_RACE race);
void __stdcall CreateDenyPlayerAI (ISPlayerAI ** ppPlayerAI, U32 playerID, M_RACE race);

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
SPlayerAI::SPlayerAI (void)
{
	// nothing else to init so far?
}
//--------------------------------------------------------------------------//
//
SPlayerAI::~SPlayerAI (void)
{
	COMPTR<IDAConnectionPoint> connection;

	resetAssignments();

	if (FULLSCREEN && FULLSCREEN->QueryOutgoingInterface("IEventCallback", connection) == GR_OK)
		connection->Unadvise(eventHandle);
}
//--------------------------------------------------------------------------//
//
void SPlayerAI::init (U32 _playerID, M_RACE race)
{
	COMPTR<IDAConnectionPoint> connection;

	if (eventHandle==0 && FULLSCREEN->QueryOutgoingInterface("IEventCallback", connection) == GR_OK)
	{
		connection->Advise(getBase(), &eventHandle);
		FULLSCREEN->SetCallbackPriority(this, EVENT_PRIORITY_MISSION);
	}

	const struct CQGAME & cqgame = MGlobals::GetGameSettings();

	playerID = _playerID;

	if(race == M_NO_RACE) 
		m_nRace = MGlobals::GetPlayerRace(playerID);
	else m_nRace = race;

	int c;
	for(c = 0; c < MAX_PLAYERS; c++)
	{
		NETGRIDVECTOR ngv;
		ngv.zero();
		ngv.systemID = 0;
		Enemies[c].HQLocation.init(ngv, 0);
		Enemies[c].numMilitaryUnits = 0;
		Enemies[c].TotalMilitaryPower = 0;
		Enemies[c].numUnits = 0;
	}

	for(c = 0; c < 10; c++)
	{
		m_OffLimits[c].loc.zero();
		m_OffLimits[c].loc.systemID = 0;
		m_OffLimits[c].hits = 0;
	}

	for(c = 0; c < 100; c++)
	{
		m_ActingUnits[c].mID = 0;
		m_ActingUnits[c].timer = 0;
	}
	
	m_Age = ((U32)playerID);
	m_bOnOff = true;

	m_Gas = 0;
	m_GasPerTurn = 0;
	m_Metal = 0;
	m_MetalPerTurn = 0;
	m_Crew = 0;
	m_CrewPerTurn = 0;
	m_ComPts = 0;
	m_ComPtsUsedUp = 0;

	AIPersonality settings;
	settings.uNumScouts = 2;
	settings.buildMask.bVisibilityRules = true;
	settings.nOffenseVsDefense = (rand() % 99) + 1;
	settings.buildMask.bRandomBit1 = rand() & 1;
	settings.buildMask.bRandomBit2 = rand() & 1;
	settings.buildMask.bRandomBit3 = rand() & 1;
	settings.buildMask.bRandomBit4 = rand() & 1;
	settings.buildMask.bRandomBit5 = rand() & 1;
	settings.buildMask.bRandomBit6 = rand() & 1;
	settings.buildMask.bRandomBit7 = rand() & 1;
	SetPersonality(settings);

	pAssignments = NULL;

	for(c = 0; c < MAX_SYSTEMS; c++) 
	{
		m_bSystemSupplied[c] = false;
	}
	memset(m_Threats, 0, (16 * 16 * MAX_SYSTEMS * (sizeof(GridSquare))));

	initArchetypeIDs();
	initBuildDesires();
	initShipDesires();

	m_AdmiralsOwned = 0;

	m_AttackWaveSize = CalcWaveSize();
	m_StrategicTarget = NULL;
	m_StrategicTargetRange = 0;
	m_StrategicTargetSystem = 0;
	m_ThreatResponse = 0;

	m_RepairSite = 0;

	m_LastSpacialPointActor = 0;
	m_NumDangerPoints = 0;
	m_NumIonCloudPoints = 0;
	m_NumFabPoints = 0;
	m_bAssignsInvalid = false;
	m_bTechInvalid = false;
	m_WorkingTech = S_HULL5;

	//governs which player i should be finishing off, 0 means none
	m_Terminate = 0;
	m_ResignComing = 0;

	//game settings, victory conditions, etc
	if (cqgame.gameType == KILL_HQ_PLATS) m_bKillHQ = true;
	else m_bKillHQ = false;
	m_bRegenResources = MGlobals::GetGameSettings().regenOn;

	m_bShipRepair = false;
	m_bWorldSeen = false;
	memset(m_ScoutRoutes, 0, (sizeof(ScoutRoute) * MAX_SCOUT_ROUTES));
	m_nNumScoutRoutes = 0;
	memset(m_UnitTotals, 0, (sizeof(U32) * MAX_PLAYERS));

	initResearchDesires(true);
}
//--------------------------------------------------------------------------//
// optimization basically
//--------------------------------------------------------------------------//
void SPlayerAI::initArchetypeIDs(void)
{
	U32 id = 0;
	for(U32 c = 0; c < M_ENDOBJCLASS; c++)
	{
		id = 0;
		
		if(c == M_FABRICATOR) id = ARCHLIST->GetArchetypeDataID("FAB!!T_Fabricator");
		if(c == M_SUPPLY) id = ARCHLIST->GetArchetypeDataID("SUPSHIP!!T_Supply");

		if(c == M_CORVETTE) id = ARCHLIST->GetArchetypeDataID("GBOAT!!T_Corvette");
		if(c == M_MISSILECRUISER) id = ARCHLIST->GetArchetypeDataID("GBOAT!!T_Missile Cruiser");
		if(c == M_BATTLESHIP) id = ARCHLIST->GetArchetypeDataID("GBOAT!!T_Midas Battleship");
		if(c == M_DREADNOUGHT) id = ARCHLIST->GetArchetypeDataID("GBOAT!!T_Dreadnought");
		if(c == M_CARRIER) id = ARCHLIST->GetArchetypeDataID("GBOAT!!T_Fleet Carrier");
		if(c == M_LANCER) id = ARCHLIST->GetArchetypeDataID("GBOAT!!T_Lancer Cruiser");
		if(c == M_INFILTRATOR) id = ARCHLIST->GetArchetypeDataID("GBOAT!!T_Infiltrator");

		if(c == M_HARVEST) id = ARCHLIST->GetArchetypeDataID("HARVEST!!T_Harvester");
		if(c == M_RECONPROBE) id = 0; //id = ARCHLIST->GetArchetypeDataID("");
		if(c == M_TROOPSHIP) id = ARCHLIST->GetArchetypeDataID("TSHIP!!T_Troopship");
		if(c == M_FLAGSHIP) id = 0; //id = ARCHLIST->GetArchetypeDataID("");

		if(c == M_HQ) id = ARCHLIST->GetArchetypeDataID("PLATBUILDSUP!!T_HQ");
		if(c == M_REFINERY) id = ARCHLIST->GetArchetypeDataID("PLATREFINERY!!T_Refinery");
		if(c == M_LIGHTIND) id = ARCHLIST->GetArchetypeDataID("PLATBUILD!!T_LightIndustrial");
		if(c == M_TENDER) id = ARCHLIST->GetArchetypeDataID("PLATREPAIR!!T_Tender");
		if(c == M_REPAIR) id = ARCHLIST->GetArchetypeDataID("PLATREPAIR!!T_Repair");
		if(c == M_OUTPOST) id = ARCHLIST->GetArchetypeDataID("PLATREFINERY!!T_Outpost");
		if(c == M_ACADEMY) id = ARCHLIST->GetArchetypeDataID("PLATREFINERY!!T_Academy");
		if(c == M_BALLISTICS) id = ARCHLIST->GetArchetypeDataID("PLATBUILD!!T_BallisticsLab");
		if(c == M_ADVHULL) id = ARCHLIST->GetArchetypeDataID("PLATBUILD!!T_AdvHull");
		if(c == M_HEAVYREFINERY) id = ARCHLIST->GetArchetypeDataID("UPGRADE!!T_HeavyRefinery");
		if(c == M_SUPERHEAVYREFINERY) id = ARCHLIST->GetArchetypeDataID("UPGRADE!!T_SuperHvyRefinery");

		if(c == M_LSAT) id = ARCHLIST->GetArchetypeDataID("PLATGUN!!T_Turret");
		if(c == M_SPACESTATION) id = ARCHLIST->GetArchetypeDataID("PLATGUN!!T_SpaceStation");
		
		if(c == M_LRSENSOR) id = ARCHLIST->GetArchetypeDataID("PLATBUILD!!T_SensorTower");
		if(c == M_AWSLAB) id = ARCHLIST->GetArchetypeDataID("PLATBUILD!!T_AWSLab");
		if(c == M_IONCANNON) id = ARCHLIST->GetArchetypeDataID("PLATGUN!!T_IonCannon");
		if(c == M_HEAVYIND) id = ARCHLIST->GetArchetypeDataID("PLATBUILD!!T_HeavyIndustrial");
		if(c == M_HANGER) id = ARCHLIST->GetArchetypeDataID("PLATBUILD!!T_Hanger");
		if(c == M_PROPLAB) id = ARCHLIST->GetArchetypeDataID("PLATBUILD!!T_PropLab");
		if(c == M_DISPLAB) id = ARCHLIST->GetArchetypeDataID("PLATBUILD!!T_DispLab");

		if(c == M_CLOAKSTATION) id = 0; //ARCHLIST->GetArchetypeDataID("");

		if(c == M_JUMPPLAT) id = ARCHLIST->GetArchetypeDataID("PLATJUMP!!T_JumpPlat");
		//case M_JUMPPLAT:		pArcheID = ARCHLIST->GetArchetypeDataID("PLATJUMP!!M_JumpPlat");
		//case M_JUMPPLAT:		pArcheID = ARCHLIST->GetArchetypeDataID("PLATJUMP!!S_JumpPlat");

		if(c == M_COCOON) id = ARCHLIST->GetArchetypeDataID("PLATBUILDSUP!!M_Cocoon");
		if(c == M_COLLECTOR) id = ARCHLIST->GetArchetypeDataID("PLATREFINERY!!M_Collector");
		if(c == M_GREATER_COLLECTOR) id = ARCHLIST->GetArchetypeDataID("UPGRADE!!M_GreaterCollector");
		if(c == M_GREATER_PLANTATION) id = ARCHLIST->GetArchetypeDataID("UPGRADE!!M_GreaterPlantation");
		if(c == M_PLANTATION) id = ARCHLIST->GetArchetypeDataID("PLATREPAIR!!M_Plantation");
		if(c == M_EYESTOCK) id = ARCHLIST->GetArchetypeDataID("PLATREFINERY!!M_EyeStalk");
		if(c == M_THRIPID) id = ARCHLIST->GetArchetypeDataID("PLATBUILD!!M_Thripid");
		if(c == M_WARLORDTRAINING) id = ARCHLIST->GetArchetypeDataID("PLATREFINERY!!M_Warlord");
		if(c == M_BLASTFURNACE) id = ARCHLIST->GetArchetypeDataID("PLATBUILD!!M_BlastFurnace");
		if(c == M_EXPLOSIVESRANGE) id = ARCHLIST->GetArchetypeDataID("UPGRADE!!M_Explosive");
		if(c == M_PLASMASPLITTER) id = ARCHLIST->GetArchetypeDataID("PLATGUN!!M_PlasmaSpitter");
		if(c == M_CARRIONROOST) id = ARCHLIST->GetArchetypeDataID("PLATBUILD!!M_CarrionRoost");
		if(c == M_VORAAKCANNON) id = ARCHLIST->GetArchetypeDataID("UPGRADE!!M_Voraak");
		if(c == M_MUTATIONCOLONY) id = ARCHLIST->GetArchetypeDataID("UPGRADE!!M_Mutation");
		if(c == M_NIAD) id = ARCHLIST->GetArchetypeDataID("UPGRADE!!M_Niad");
		if(c == M_BIOFORGE) id = ARCHLIST->GetArchetypeDataID("PLATBUILD!!M_BioForge");
		if(c == M_FUSIONMILL) id = ARCHLIST->GetArchetypeDataID("UPGRADE!!M_Fusion");
		if(c == M_CARPACEPLANT) id = ARCHLIST->GetArchetypeDataID("UPGRADE!!M_Carpace");
		if(c == M_DISSECTIONCHAMBER) id = ARCHLIST->GetArchetypeDataID("PLATSELL!!M_Dissection");
		if(c == M_HYBRIDCENTER) id = ARCHLIST->GetArchetypeDataID("UPGRADE!!M_Hybrid_Carpace");
		if(c == M_PLASMAHIVE) id = ARCHLIST->GetArchetypeDataID("UPGRADE!!M_PlasmaHive");

		if(c == M_WEAVER) id = ARCHLIST->GetArchetypeDataID("FAB!!M_NymphWeever");
		if(c == M_SPINELAYER) id = ARCHLIST->GetArchetypeDataID("MLAYER!!M_Spinelayer");
		if(c == M_SIPHON) id = ARCHLIST->GetArchetypeDataID("HARVEST!!M_Siphon");
		if(c == M_ZORAP) id = ARCHLIST->GetArchetypeDataID("SUPSHIP!!M_Zorap");
		if(c == M_LEECH) id = ARCHLIST->GetArchetypeDataID("TSHIP!!M_Leech");
		if(c == M_SEEKER) id = ARCHLIST->GetArchetypeDataID("GBOAT!!M_Seeker");
		if(c == M_SCOUTCARRIER) id = ARCHLIST->GetArchetypeDataID("GBOAT!!M_Scout Carrier");
		if(c == M_FRIGATE) id = ARCHLIST->GetArchetypeDataID("GBOAT!!M_Frigate");
		if(c == M_KHAMIR) id = ARCHLIST->GetArchetypeDataID("GBOAT!!M_Khamir");
		if(c == M_HIVECARRIER) id = ARCHLIST->GetArchetypeDataID("GBOAT!!M_Hive Carrier");
		if(c == M_SCARAB) id = ARCHLIST->GetArchetypeDataID("GBOAT!!M_Scarab");
		if(c == M_TIAMAT) id = ARCHLIST->GetArchetypeDataID("GBOAT!!M_Tiamat");
		if(c == M_WARLORD) id = 0;//id = ARCHLIST->GetArchetypeDataID("");

		if(c == M_ACROPOLIS) id = ARCHLIST->GetArchetypeDataID("PLATBUILDSUP!!S_Acropolis");
		if(c == M_OXIDATOR) id = ARCHLIST->GetArchetypeDataID("PLATREFINERY!!S_Oxidator");
		if(c == M_PAVILION) id = ARCHLIST->GetArchetypeDataID("PLATBUILD!!S_Pavilion");
		if(c == M_SENTINELTOWER) id = ARCHLIST->GetArchetypeDataID("PLATBUILD!!S_SentinelTower");
		if(c == M_EUTROMILL) id = ARCHLIST->GetArchetypeDataID("PLATREPAIR!!S_Eutromill");
		if(c == M_GREATERPAVILION) id = ARCHLIST->GetArchetypeDataID("PLATBUILD!!S_GreaterPavilion");
		if(c == M_HELIONVEIL) id = ARCHLIST->GetArchetypeDataID("PLATBUILD!!S_HelionVeil");
		if(c == M_CITADEL) id = ARCHLIST->GetArchetypeDataID("PLATREFINERY!!S_Citadel");
		if(c == M_XENOCHAMBER) id = ARCHLIST->GetArchetypeDataID("PLATBUILD!!S_XenoChamber");
		if(c == M_ANVIL) id = ARCHLIST->GetArchetypeDataID("PLATBUILD!!S_Anvil");
		if(c == M_MUNITIONSANNEX) id = ARCHLIST->GetArchetypeDataID("PLATBUILD!!S_MunitionsAnnex");
		if(c == M_TURBINEDOCK) id = ARCHLIST->GetArchetypeDataID("PLATBUILD!!S_TurbineDock");
		if(c == M_TALOREANMATRIX) id = ARCHLIST->GetArchetypeDataID("PLATGUN!!S_TaloreanMatrix");
		if(c == M_BUNKER) id = ARCHLIST->GetArchetypeDataID("PLATREFINERY!!S_Bunker");

		if(c == M_PROTEUS) id = ARCHLIST->GetArchetypeDataID("PLATGUN!!S_Proteus");
		if(c == M_HYDROFOIL) id = ARCHLIST->GetArchetypeDataID("PLATGUN!!S_Hydrofoil");
		if(c == M_ESPCOIL) id = ARCHLIST->GetArchetypeDataID("PLATGUN!!S_ESPCoil");
		if(c == M_STARBURST) id = ARCHLIST->GetArchetypeDataID("PLATGUN!!S_Starburst");
		if(c == M_PORTAL) id = ARCHLIST->GetArchetypeDataID("PLATGUN!!S_TesPortal");

		if(c == M_FORGER) id = ARCHLIST->GetArchetypeDataID("FAB!!S_Forger");
		if(c == M_STRATUM) id = ARCHLIST->GetArchetypeDataID("SUPSHIP!!S_Stratum");
		if(c == M_GALIOT) id = ARCHLIST->GetArchetypeDataID("HARVEST!!S_Galiot");
		if(c == M_ATLAS) id = ARCHLIST->GetArchetypeDataID("GBOAT!!M_Atlas");
		if(c == M_LEGIONAIRE) id = ARCHLIST->GetArchetypeDataID("TSHIP!!S_Legionare");
		if(c == M_TAOS) id = ARCHLIST->GetArchetypeDataID("GBOAT!!S_Taos");
		if(c == M_POLARIS) id = ARCHLIST->GetArchetypeDataID("GBOAT!!S_Polaris");
		if(c == M_AURORA) id = ARCHLIST->GetArchetypeDataID("GBOAT!!S_Aurora");
		if(c == M_ORACLE) id = ARCHLIST->GetArchetypeDataID("GBOAT!!S_Oracle");
		if(c == M_TRIREME) id = ARCHLIST->GetArchetypeDataID("GBOAT!!S_Trireme");
		if(c == M_MONOLITH) id = ARCHLIST->GetArchetypeDataID("GBOAT!!S_Monolith");
		if(c == M_HIGHCOUNSEL) id = 0; //id = ARCHLIST->GetArchetypeDataID("");	

		m_ArchetypeIDs[c] = id;

		BuildDesires[c].nNumSlots = 0; 
		if(id && MGlobals::IsPlatform((M_OBJCLASS)c))
		{
			BASIC_DATA* data = (BASIC_DATA*)ARCHLIST->GetArchetypeData(m_ArchetypeIDs[c]);
			if(data)
			{
				BASE_PLATFORM_DATA* sdata = (BASE_PLATFORM_DATA*)data;
				BuildDesires[c].nNumSlots = sdata->slotsNeeded;
			}
		}
	}
}
//--------------------------------------------------------------------------//
// initializing build affinities data, finer tuning in other functions
//--------------------------------------------------------------------------//
void SPlayerAI::initBuildDesires(void)
{
	for(int c = 0; c < M_ENDOBJCLASS; c++)
	{
		BuildPriority * cur = &BuildDesires[c];

		//defaults
		cur->nPriority = -250;
		cur->nPlanetMultiplier = 0;
		cur->nSystemMultiplier = 0;
		cur->nAdditiveDecrement = 50;
		cur->nExponentialDecrement = 0;
		cur->prerequisite = M_NONE;
		cur->bMutation = false;
		
		//dead entries
		if(c == M_NONE || c == M_CLOAKSTATION || c == M_RECONPROBE || c == M_FLAGSHIP ||
			c == M_WARLORD || c == M_HIGHCOUNSEL)
		{
			cur->nPriority = -10000;
			cur->nPlanetMultiplier = 0;
			cur->nAdditiveDecrement = 0;
			cur->nExponentialDecrement = 0;
			cur->prerequisite = M_NONE;
		}

		if((c == M_HQ) || (c == M_COCOON) || (c == M_ACROPOLIS))
		{
			cur->nPriority = 500;
			cur->nPlanetMultiplier = 0;
			cur->nAdditiveDecrement = 0;
			cur->nExponentialDecrement = 10;
		}

		if((c == M_REFINERY) || (c == M_COLLECTOR) || (c == M_OXIDATOR))
		{
			cur->nPriority = 501;
			cur->nPlanetMultiplier = 1;
			cur->nAdditiveDecrement = 100;
			cur->nExponentialDecrement = 0;
		}

		if((c == M_HEAVYREFINERY) || (c == M_GREATER_COLLECTOR) || (c == M_SUPERHEAVYREFINERY))
		{
			cur->nPriority = 301;
			cur->nPlanetMultiplier = 1;
			cur->nAdditiveDecrement = 130;
			cur->nExponentialDecrement = 0;
		}
		
		if((c == M_LIGHTIND) || (c == M_THRIPID) || (c == M_PAVILION))
		{
			cur->nPriority = 500;
			cur->nPlanetMultiplier = 0;
			cur->nAdditiveDecrement = 0;
			cur->nExponentialDecrement = 2;
		}

		if((c == M_TENDER) || (c == M_PLANTATION) || (c == M_EUTROMILL))
		{
			cur->nPriority = 450;
			cur->nPlanetMultiplier = 0;
			cur->nAdditiveDecrement = 0;
			cur->nExponentialDecrement = 3;
		}

		if(c == M_OUTPOST || c == M_BUNKER)
		{
			cur->nPriority = 500;
			cur->nPlanetMultiplier = 0;
			cur->nAdditiveDecrement = 0;
			cur->nExponentialDecrement = 2;
		}

		if(c == M_ACADEMY || c == M_WARLORDTRAINING || c == M_CITADEL)
		{
			cur->nPriority = 600;
			cur->nPlanetMultiplier = 0;
			cur->nAdditiveDecrement = 290;
			cur->nExponentialDecrement = 0;
		}

		if(c == M_BALLISTICS || c == M_BLASTFURNACE || c == M_MUNITIONSANNEX)
		{
			cur->nPriority = 650;
			cur->nPlanetMultiplier = 0;
			cur->nAdditiveDecrement = 0;
			cur->nExponentialDecrement = 3;
		}

		if(c == M_HEAVYIND || c == M_NIAD || c == M_GREATERPAVILION)
		{
			cur->nPriority = 750;
			cur->nPlanetMultiplier = 0;
			cur->nAdditiveDecrement = 265;
			cur->nExponentialDecrement = 0;
		}
	}

}
//--------------------------------------------------------------------------//
// research priorities
//--------------------------------------------------------------------------//
void SPlayerAI::initResearchDesires(bool bFirstTime)
{
	/*
	struct ResearchPriority : public BasePriority
{
	U32					indentifier;
	M_OBJCLASS			facility;
	TECHTREE::COMMON	prerequisite;
	//second prereq
	//third prereq
};

	*/
	/*
	for(int c = 0; c < AI_TECH_END; c++)
	{
		BuildPriority * cur = &BuildDesires[c];

		if(GetTechRace(c) != m_nRace)
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
			case 
		}	
	}
	*/ //always overridden

}
//--------------------------------------------------------------------------//
// gunboat priorities
//--------------------------------------------------------------------------//
void SPlayerAI::initShipDesires(void)
{
	for(int c = 0; c < M_ENDOBJCLASS; c++)
	{
		ShipPriority * cur = &ShipDesires[c];

		//defaults
		cur->nPriority = 200;
		cur->nAdditiveDecrement = 45;
		cur->nExponentialDecrement = 0;
		cur->facility = M_LIGHTIND;

		//non-gboat defaults
		if(!MGlobals::IsGunboat((M_OBJCLASS)c))
		{
			cur->nPriority = -100000;
			cur->nAdditiveDecrement = 50;
			cur->nExponentialDecrement = 0;
			cur->facility = M_NONE;
		}

		if(c == M_CORVETTE)
		{
			cur->nPriority = 200;
			cur->nAdditiveDecrement = 10;
			cur->nExponentialDecrement = 0;
			cur->facility = M_LIGHTIND;
		}

		if(c == M_MISSILECRUISER)
		{
			cur->nPriority = 100 + (rand() & 127);
			cur->nAdditiveDecrement = 15;
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
			cur->nPriority = 1600 + DNA.uNumScouts * 100;
			cur->nAdditiveDecrement = 130;
			cur->nExponentialDecrement = 0;
			cur->facility = M_LIGHTIND;
		}

		if(c == M_TROOPSHIP)
		{
			cur->nPriority = 1300 + DNA.nNumTroopships * 100;
			cur->nAdditiveDecrement = 120;
			cur->nExponentialDecrement = 0;
			cur->facility = M_LIGHTIND;
		}

		if(c == M_SPINELAYER)
		{
			cur->nPriority = 700 + DNA.nNumMinelayers * 100;
			cur->nAdditiveDecrement = 125;
			cur->nExponentialDecrement = 0;
			cur->facility = M_THRIPID;
		}

		if(c == M_LEECH)
		{
			cur->nPriority = 1000 + DNA.nNumTroopships * 100;
			cur->nAdditiveDecrement = 150;
			cur->nExponentialDecrement = 0;
			cur->facility = M_NIAD;
		}

		if(c == M_SEEKER)
		{
			cur->nPriority = 1600 + DNA.uNumScouts * 100;
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
			cur->nPriority = 710 + DNA.nNumMinelayers * 100;
			cur->nAdditiveDecrement = 170;
			cur->nExponentialDecrement = 0;
			cur->facility = M_GREATERPAVILION;
		}

		if(c == M_LEGIONAIRE)
		{
			cur->nPriority = 1000 + DNA.nNumTroopships * 100;
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
			cur->nPriority = 1700 + DNA.uNumScouts * 100;
			cur->nAdditiveDecrement = 10;
			cur->nExponentialDecrement = 0;
			cur->facility = M_PAVILION;
		}

		if(c == M_TRIREME)
		{
			cur->nPriority = 900;
			cur->nAdditiveDecrement = 10 + (rand() % 255);
			cur->nExponentialDecrement = 0;
			cur->facility = M_GREATERPAVILION;
		}

		if(c == M_MONOLITH)
		{
			cur->nPriority = 2000;
			cur->nAdditiveDecrement = 180;
			cur->nExponentialDecrement = 0;
			cur->facility = M_GREATERPAVILION;
		}

	}

}
//--------------------------------------------------------------------------//
// the AI's update loop
//--------------------------------------------------------------------------//
void SPlayerAI::evaluate (void)
{
	SinglePassDataUpdate();

	IBaseObject * obj = OBJLIST->GetTargetList();
	MPart part;

	while (obj)
	{
		if ((part = obj).isValid())
		{
			if (part->bReady && part->playerID == playerID && (part->bNoAIControl == false))
			{
				if ((part->systemID & HYPER_SYSTEM_MASK)==0)
				{
					if(m_bShipRepair && MGlobals::IsHeavyGunboat(part->mObjClass) && 
						((((SINGLE)part->hullPoints) / ((SINGLE)part->hullPointsMax)) < 0.2))
					{
						ASSIGNMENT *rep_node = findAssignment(REPAIR_ASS);

						bool bWait = false;
						if(rep_node && (rep_node->set.contains(part->dwMissionID) == true))
							bWait = ((rand() & 3) != 0);

						if((!bWait) && doShipRepair(part))
						{
							ASSIGNMENT *node = findAssignment(RESUPPLY);
							if(node && node->set.contains(part->dwMissionID))
								RemoveAssMember(node, part);

							if(rep_node && (rep_node->set.numObjects < MAX_SELECTED_UNITS) && 
								(rep_node->set.contains(part->dwMissionID) == false))
							{
								//remove from current assignment
								removeFromAssignments(part);
								//add to resupply assignment
								rep_node->set.objectIDs[rep_node->set.numObjects++] = part->dwMissionID;
							}
						}
					}
					else if(((part->supplies == 0) && (MGlobals::IsGunboat(part->mObjClass) || MGlobals::IsSupplyShip(part->mObjClass))) || 
						(MGlobals::IsMinelayer(part->mObjClass) && (part->supplies < 10)))//  fix  supply cost of mines hardcoded, will cause thrashing if cost ever greater than 10
					{
						ASSIGNMENT *node = findAssignment(RESUPPLY);

						bool bWait = false;
						if(node && (node->set.contains(part->dwMissionID) == true))
							bWait = ((rand() & 3) != 0);

						if((!bWait) && doResupply(part))
						{
							ASSIGNMENT *rep_node = findAssignment(REPAIR_ASS);
							if(rep_node && rep_node->set.contains(part->dwMissionID))
								RemoveAssMember(rep_node, part);

							if(node && (node->set.numObjects < MAX_SELECTED_UNITS) && 
								(node->set.contains(part->dwMissionID) == false))
							{
								//remove from current assignment
								removeFromAssignments(part);
								//add to resupply assignment
								node->set.objectIDs[node->set.numObjects++] = part->dwMissionID;
							}
						}
					}
					else
					{
						//remove from resupply assignment
						ASSIGNMENT *node = findAssignment(RESUPPLY);
						if(node && node->set.contains(part->dwMissionID))
							RemoveAssMember(node, part);

						ASSIGNMENT *rep_node = findAssignment(REPAIR_ASS);
						if(rep_node && rep_node->set.contains(part->dwMissionID))
							RemoveAssMember(rep_node, part);

						if (THEMATRIX->HasPendingOp(obj) == 0)
						{
							onIdleUnit(obj);
						}
						else if ((MGlobals::IsGunboat(part->mObjClass)) && (findAssignment(part) == 0) && 
							((rand() & 3) == 0) && ((part->systemID & HYPER_SYSTEM_MASK)==0))
						{
							assignGunboat(part);
						}

						ensureAssignmentFor(part);  //have HQ's, heavy shipyards, and harvesters init defend assgns
					}
				}
			}
		}

		obj = obj->nextTarget;
	}
}
//--------------------------------------------------------------------------//
//
void SPlayerAI::onIdleUnit (IBaseObject *obj)
{
	if(!m_bOnOff) return;  //I believe all packet sending must come through here. (except for monitorassignments)

	MPart part = obj;
	NETGRIDVECTOR loc;
	loc.zero();

	if (part.isValid())
	{
		/*	//for self-monitoring of tech research  (which probably will never work. haha)
		U32 howmany = HowManyResearchFabPoints(part->dwMissionID);
		CQASSERT(howmany < 2);
		if(howmany > 0)
		{
			if(!MGlobals::IsFabricator(part->mObjClass))
			{  //if it's not a fabricator it must be a platform that was researching
				BuildSite * point = GetFabPoint(part->dwMissionID);
				S32 tech = point->slot;
				if(tech > NOTECH && tech < AI_TECH_END)
					ResearchDesires[point->slot].bAcquired = true;
			}
		}
		*/

		//this unit is idle, so it should not represent fab points
		//RemoveSpacialPoint(FAB_POINT, loc, part->dwMissionID);
		U32 sys = part->systemID;
		if(sys > 0 && sys <= MAX_SYSTEMS && ((part.obj->objClass != OC_PLATFORM) || 
			SECTOR->SystemInSupply(sys, playerID))) 
		{
			switch (part->mObjClass)
			{
			case M_HARVEST:
			case M_SIPHON:
			case M_GALIOT:
				if(DNA.buildMask.bHarvest)
				{
					OBJPTR<IHarvest> harvest;
					if(obj->QueryInterface(IHarvestID, harvest)!=0)
					{
						S32 numHarvesters = UnitsOwned[M_HARVEST] + UnitsOwned[M_SIPHON] + UnitsOwned[M_GALIOT];
						if(numHarvesters >= 0)
						{
							if(harvest->IsIdle() && (!harvest->GetGas()) && (!harvest->GetMetal()) && 
								(((rand()) % ((numHarvesters / 3) + 1)) < 2))
								doHarvest(part);
						}
					}
				}
			break;
			case M_FABRICATOR:
			case M_WEAVER:
			case M_FORGER:
				if(DNA.buildMask.bBuildPlatforms)  //could instead, decrease prior of all plats with > 0 slots
				{
					//this unit is idle, so it should not represent fab points
					RemoveSpacialPoint(FAB_POINT, loc, part->dwMissionID);

					//for building L-Sats when in danger  fix  
					//if(GetFabDanger())
					//{
					//
					//}
					if(ReadyForBuilding())
						doFabricator(part);
					else if((rand() & 3) == 0)
						doRepair(part);
				}
			break;
			case M_HQ:
			case M_COCOON:
			case M_ACROPOLIS:
				if(part->race == m_nRace)  //troopshipping problems
				{
					doHQ(part);
				}
			break;
			case M_THRIPID:
				doUpgrade(part);
			case M_LIGHTIND:
			case M_PAVILION:
				if(DNA.buildMask.bBuildLightGunboats && ReadyForMilitary())
					doShipyard(part);
			break;
			case M_NIAD:
			case M_HEAVYIND:
			case M_GREATERPAVILION:
				if((DNA.buildMask.bBuildMediumGunboats || DNA.buildMask.bBuildHeavyGunboats || 
					(DNA.buildMask.bBuildLightGunboats && part->mObjClass == M_NIAD)) &&
					ReadyForMilitary())
					doShipyard(part);
			break;
			case M_SUPPLY:
			case M_ZORAP:
			case M_STRATUM:
				if (part->supplies && ((rand() & 3) == 0))
				{
					doSupplyShip(part);
				}
			break;
			case M_PORTAL:
				if((rand() & 3) == 0)
					doPortal(part);
			break;
			case M_TROOPSHIP:
			case M_LEECH:
			case M_LEGIONAIRE:
				if((rand() & 3) == 0)
					doTroopship(part);
			break;
			case M_SPINELAYER:
			case M_ATLAS:
				if((rand() & 3) == 0)
					doMinelayer(part);
			break;
			case M_ACADEMY:
			case M_WARLORDTRAINING:
			case M_CITADEL:
				if(DNA.buildMask.bBuildAdmirals && 
					(m_AdmiralsOwned < getNumAssignments(DEFEND)) &&
					((rand() & 31) == 0))
				{
					doNavalAcademy(part);
				}
			break;
			case M_FLAGSHIP:
			case M_WARLORD:
			case M_HIGHCOUNSEL:
				if((rand() & 3) == 0)
					doFlagShip(part);
			break;
			case M_REFINERY:
			case M_HEAVYREFINERY:
			case M_SUPERHEAVYREFINERY:
				if(part->race == m_nRace && ReadyForHarvest())
				{
					doRefinery(part);
				}
			break; //I have stopped terran refineries from mutating  fix  (deal with)
			//case M_TENDER:  fix  tender changes to repair plat, but there's no mission enum for repairplat
			//no break on purpose
			case M_PLANTATION:
			case M_BLASTFURNACE:
			case M_EYESTOCK:
			case M_PLASMASPLITTER:  //does this line ever get called?
			case M_BIOFORGE:
			case M_FUSIONMILL:
			case M_CARPACEPLANT:
				doUpgrade(part);
			break;
			case M_COLLECTOR:
				doUpgrade(part);
			case M_GREATER_COLLECTOR:
			case M_OXIDATOR:
				if(ReadyForHarvest())
				{
					doRefinery(part);
				}
			default:
			break;
			}

			if (MGlobals::IsGunboat(part->mObjClass))
			{
				assignGunboat(part);
			}
			else if ((part.obj->objClass == OC_PLATFORM) && DNA.buildMask.bResearchTechs && ReadyForResearch() && 
				(m_bTechInvalid == false))
			{
				doResearch(part);
			}
		}
	}
}
//--------------------------------------------------------------------------//
//
void SPlayerAI::resetUpdateCount (void)
{
	updateCount = (((rand() % 128)+(rand() % 128)) * UPDATE_PERIOD) / 256;
}
//--------------------------------------------------------------------------//
//
GENRESULT SPlayerAI::Notify (U32 message, void *param)
{
	//if(!m_bOnOff) return GR_OK;
	
	if (THEMATRIX->IsMaster())		// don't respond if on the client side
	switch (message)
	{
	case CQE_IDLE_UNIT:
		if (MGlobals::IsHost())
		{
			U32 dwMissionID = (U32)param;
			if ((dwMissionID & PLAYERID_MASK) == playerID)
			{
				IBaseObject *obj = OBJLIST->FindObject(dwMissionID);

				if(obj)
				{
					MPart part = obj;

					if(part->bReady)
					{
						// verify one last time
						if ((THEMATRIX->HasPendingOp(dwMissionID) == 0) &&
							(part->bNoAIControl == false))
						{
							ASSIGNMENT *node = findAssignment(RESUPPLY);
							ASSIGNMENT *rep = findAssignment(REPAIR_ASS);
							if(((!node) || (node->set.contains(part->dwMissionID) == 0)) && ((!rep) || 
								(rep->set.contains(part->dwMissionID) == 0)))
								onIdleUnit(obj);
						}
					}
				}
			}
		}
		break;
	case CQE_OBJECT_PRECAPTURE:
	case CQE_OBJECT_DESTROYED:
		if (MGlobals::IsHost())
		{
			U32 dwMissionID = (U32)param;
			U32 whichplayer = (dwMissionID & PLAYERID_MASK);
			if (whichplayer == playerID)
			{
				// one of my units has just been blown up
				IBaseObject *obj = OBJLIST->FindObject(dwMissionID);

				if(obj)
				{
					MPart part = obj;
					NETGRIDVECTOR loc;
					loc.init(obj->GetPosition(),obj->GetSystemID());
					AddSpacialPoint(DANGER_POINT, loc, CQMAP_RANGE_LARGE, M_NONE, 0, 0);

					//if this is a fabricator, remove any fab points that might be set for it
					if(MGlobals::IsFabricator(part->mObjClass))
						RemoveSpacialPoint(FAB_POINT, loc, dwMissionID);

					ASSIGNMENT * test = findAssignmentFor(part);
					if(test)
					{
						test->targetID = 0;
						cullAssignments();
					}
				
					if(TESTADMIRAL(dwMissionID))
					{
						if(m_AdmiralsOwned) m_AdmiralsOwned--;
						ASSIGNMENT * test = findAssignment(part);
						if(test) test->bHasAdmiral = false;
					}

					removeFromAssignments(part);

					if(MGlobals::IsHQ(part->mObjClass) && DNA.buildMask.bSendTaunts)
						SendDisparagingRemark();
				}
			}
			else
			{
				//this is the death of a different player's unit
				IBaseObject *obj = OBJLIST->FindObject(dwMissionID);

				if(obj)
				{
					MPart part = obj;

					if(part->playerID == m_Terminate)
					{
						SinglePassDataUpdate();
						UpdateFinalSolution();
					}

					ASSIGNMENT * test = findAssignmentFor(part);
					if(test)
					{
						if(test->type == ATTACK)
						{
							if(DNA.buildMask.bSendTaunts)
								SendCelebratoryRemark(part->playerID);

							IBaseObject * TargetObj = findStrategicTarget(part);
							MPart TargetPart;
							if (TargetObj && ((TargetPart = TargetObj).isValid()))
							{
								test->targetID = TargetObj->GetPartID();
								test->systemID = TargetObj->GetSystemID();
							}
						}
					}
				}
			}
		}
		break;
	case CQE_FABRICATOR_FINISHED:
		/*
		if (MGlobals::IsHost())
		{
			U32 dwMissionID = (U32)param;
			if ((dwMissionID & PLAYERID_MASK) == playerID)
			{
				// i finished a construction site -- remove "to-be-built" info
				
				NETGRIDVECTOR loc;
				IBaseObject *obj = OBJLIST->FindObject(dwMissionID);
				loc.init(obj->GetPosition(),obj->GetSystemID());
				RemoveSpacialPoint(FAB_POINT, loc, dwMissionID);
			}
		}
		*/
//		SinglePassDataUpdate();
//		UpdateBuildDesires();
		break;
		
	case CQE_UNIT_UNDER_ATTACK:
		if (MGlobals::IsHost())
		{
			U32 dwMissionID = (U32)param;
			if ((dwMissionID & PLAYERID_MASK) == playerID)
			{
				// my unit has been attacked!

				m_bAssignsInvalid = true;
				//IBaseObject *obj = OBJLIST->FindObject(dwMissionID);
				//NETGRIDVECTOR loc;
				//loc.init(obj->GetPosition(),obj->GetSystemID());
				//AddSpacialPoint(DANGER_POINT, loc, CQMAP_RANGE_MEDIUM,0,0);
			}
		}
		break;
	}

	return GR_OK;
}
//--------------------------------------------------------------------------//
//
void SPlayerAI::Update (void)
{
	if (MGlobals::IsHost())
	{
		if (--updateCount < 0)
		{
			evaluate();
			resetUpdateCount();
		}
	}
}
//--------------------------------------------------------------------------//
//
void SPlayerAI::SinglePassDataUpdate(void)
{
	IBaseObject * obj = OBJLIST->GetTargetList();
	//ASSIGNMENT * shadow = createShadowList();
	MPart part;
	OBJPTR<IPlanet> planet;
//	OBJPTR<IField> field;

	m_Age++;
	bool bTenthUpdate,bNinthUpdate,bFifthUpdate,bThirdUpdate,bSecondUpdate;

	if(m_Age < 20)
	{
		bTenthUpdate = true;
		bNinthUpdate = true;
		bFifthUpdate = true;
		bThirdUpdate = true;
		bSecondUpdate = true;
	}
	else
	{
		bTenthUpdate = ((m_Age % 10) == false);
		bNinthUpdate = ((m_Age % 9) == false);
		bFifthUpdate = ((m_Age % 5) == false);
		bThirdUpdate = ((m_Age % 3) == false);
		bSecondUpdate = ((m_Age % 2) == false);

		S32 mcp = MGlobals::GetMaxControlPoints(playerID) - 1;
		//impatience, aggression
		if((m_Age > 1000 && (m_Age % 200 == 0)) || (m_Terminate && m_Age % 25 == 0) ||
			(m_ComPtsUsedUp > 70 && (m_Age % 10 == 0)) || (m_ComPtsUsedUp > mcp))
		{
			if(m_AttackWaveSize > 1)
				m_AttackWaveSize--;  //should be AttackAggregatePower  fix  
			else m_AttackWaveSize = 1;
		}
	}

	bool bBuildThreat = ((m_Age % 2) == 1);
	
	//update resource totals
	m_Gas    = MGlobals::GetCurrentGas(playerID) * METALGAS_MULT;
	m_Metal  = MGlobals::GetCurrentMetal(playerID) * METALGAS_MULT;
	m_Crew   = MGlobals::GetCurrentCrew(playerID) * CREW_MULT;
	m_ComPts = MGlobals::GetCurrentTotalComPts(playerID);
	m_ComPtsUsedUp = MGlobals::GetCurrentUsedComPts(playerID);
	
	//if(m_Crew > MGlobals::GetMaxCrew(playerID) - CREW_MULT)
	//if(m_Gas > MGlobals::GetMaxGas(playerID) - METALGAS_MULT)
	//if(m_Metal > MGlobals::GetMaxMetal(playerID) - METALGAS_MULT)

	S32 c;
	for(c = 0; c < SECTOR->GetNumSystems(); c++)
	{
		if(c < MAX_SYSTEMS)
		{
			m_bSystemSupplied[c] = SECTOR->SystemInSupply(c+1, playerID);
		}
	}
	
	if(bBuildThreat) memset(m_Threats, 0, (16 * 16 * MAX_SYSTEMS * (sizeof(GridSquare))));

	S32 bestPanic = 0;
	S32 bestPanicThreat = 0;
	IBaseObject * panicUnit = NULL;

	const U32 allyMask = MGlobals::GetAllyMask(playerID);
	
	resetUnitsOwned();

	if(bSecondUpdate) clearPlanets();
	if(bThirdUpdate) m_RepairSite = 0;
	if(bFifthUpdate)
	{
		for(c = 0; c < 10; c++)
		{	
			m_OffLimits[c].loc.zero();
			m_OffLimits[c].loc.systemID = 0;
			m_OffLimits[c].hits = 0;
		}
	}

	for(c = 0; c < 100; c++)
	{
		if(m_ActingUnits[c].mID)
		{
			m_ActingUnits[c].timer++;

			if(m_ActingUnits[c].timer >= ACTING_WAIT)
			{
				m_ActingUnits[c].mID = 0;
				m_ActingUnits[c].timer = 0;
			}
		}
	}

	for(c = 0; c < MAX_PLAYERS; c++) m_UnitTotals[c] = 0;
	m_TotalMilitaryUnits = 0;
	m_TotalMilitaryPower = 0;
	m_TotalImportance = 0;
	for(c = 0; c < MAX_PLAYERS; c++)
	{
		NETGRIDVECTOR ngv;
		ngv.zero();
		ngv.systemID = 0;
		Enemies[c].HQLocation.init(ngv, 0);
		Enemies[c].numMilitaryUnits = 0;
		Enemies[c].TotalMilitaryPower = 0;
		Enemies[c].numUnits = 0;
	}

	while (obj)
	{
		if ((part = obj).isValid())
		{
			S32 pn = part->playerID - 1;
			if(pn >= 0 && pn < MAX_PLAYERS) 
			{
				if(obj->IsTargetableByPlayer(playerID))
					m_UnitTotals[pn]++;
			}

			M_OBJCLASS mobj = part->mObjClass;

			GRIDVECTOR temploc = obj->GetGridPosition();
			
			S32 gridIndexX = temploc.getIntX() / GRIDSQUARE_CONV_M;
			S32 gridIndexY = temploc.getIntY() / GRIDSQUARE_CONV_M;
			CQASSERT((gridIndexX >= 0) && (gridIndexX < 16));
			CQASSERT((gridIndexY >= 0) && (gridIndexY < 16));
			U32 enemySys = part->systemID;

			if(part->playerID == playerID)
			{
				CQASSERT(mobj < M_ENDOBJCLASS);

			//	if(part->bNoAIControl == false)
			//	{
					if(part->bReady) addPartToUnitsOwned(part);

					if(IsMilitary(mobj) || (MGlobals::IsGunPlat(mobj)))
					{
						if(MGlobals::IsGunPlat(mobj) == false)
							m_TotalMilitaryUnits++;

						m_TotalMilitaryPower += GetPowerRating(part);
					}

					m_TotalImportance += GetImportance(part);
			//	}

				//add something to record the IDs of ships in need of repair,
				//possibly ones in need of supplies as well  fix  

				/*
				if((IsMilitary(part->mObjClass) || IsFabricator(part->mObjClass)) && (part->hullPoints < 10))
				{
					if(m_LastSpacialPointActor != part->dwMissionID)
					{
						m_LastSpacialPointActor = part->dwMissionID;
						NETGRIDVECTOR loc2;
						loc2.init(obj->GetPosition(),obj->GetSystemID());
						AddSpacialPoint(DANGER_POINT, loc2, CQMAP_RANGE_MEDIUM);
					}
				}
				*/

				if(bThirdUpdate)
				{
					if(MGlobals::IsHQ(mobj))
					{
						if(rand() % 3 == 0)
							m_HQLocation.init(obj->GetPosition(), obj->GetSystemID());

						if((m_bKillHQ && (part->hullPoints < part->hullPointsMax)) || 
							((((SINGLE)part->hullPoints) / ((SINGLE)part->hullPointsMax)) < 0.5))
						{
							m_RepairSite = part->dwMissionID;	
						}

					//	m_StrategicTarget = findStrategicTarget(part);  //will only be necessary if I decide to use m_StrategicTarget for calculations besides targetID on Attack Packets
					}
					if(MGlobals::IsShipyard(mobj))
					{
						if((((SINGLE)part->hullPoints) / ((SINGLE)part->hullPointsMax)) < 0.5)
						{
							m_RepairSite = part->dwMissionID;	
						}
					}
				}

				if(bBuildThreat && (enemySys < MAX_SYSTEMS))
					SpreadThreat(gridIndexX, gridIndexY, enemySys, part, false);
			}
			else if(allyMask & (1 << (part->playerID-1)))
			{
			//an allied unit
				if(bBuildThreat && (enemySys < MAX_SYSTEMS))
					SpreadThreat(gridIndexX, gridIndexY, enemySys, part, false);
			}
			else if((part->playerID > 0) && (part->playerID <= MAX_PLAYERS))
			{
				S32 enemyID = part->playerID - 1;
				CQASSERT(enemyID >= 0 && enemyID < 8);
				if(/*1*/obj->IsVisibleToPlayer(playerID) || (DNA.buildMask.bVisibilityRules == false))
				{
					if(MGlobals::IsHQ(mobj))
					{
						Enemies[enemyID].HQLocation.init(obj->GetPosition(), obj->GetSystemID());
					}
					if(IsMilitary(mobj) || (MGlobals::IsGunPlat(mobj)))
					{
						Enemies[enemyID].numMilitaryUnits++;
						Enemies[enemyID].TotalMilitaryPower += GetPowerRating(part);
					}

					Enemies[enemyID].numUnits++;

					if(enemySys < MAX_SYSTEMS)
					{
						if(bBuildThreat) 
						{
							//if(MGlobals::IsGunboat(mobj) || MGlobals::IsGunPlat(mobj))  //platforms should be threatening too
							SpreadThreat(gridIndexX, gridIndexY, enemySys, part, true);
						}
						else
						{
							S32 panic = m_Threats[gridIndexY][gridIndexX][enemySys].threat * m_Threats[gridIndexY][gridIndexX][enemySys].importance;
							panic += (part->hullPointsMax / 8);
							panic += ((part->hullPointsMax - part->hullPoints) / 16);
							if((panic > bestPanic) && (obj->objMapNode && (obj->objMapNode->flags & OM_TARGETABLE)))
							{
								bestPanic = panic;
								bestPanicThreat = m_Threats[gridIndexY][gridIndexX][enemySys].threat;
								panicUnit = obj;
							}
						}
					}
				}
			}
			else //nebula, planet, wormhole, etc
			{
				if(bSecondUpdate)
				{
					if (obj->QueryInterface(IPlanetID, planet)!=0)  //object is a planet
					{
						U32 mID = 0;
						bool bMine = false;
						for(int c = 0; c < M_MAX_SLOTS; c++)
						{
							U32 bitField = 1 << c;
							mID = planet->GetSlotUser(bitField);
							if(mID && ((mID & PLAYERID_MASK) == playerID))
							{
								addPlanet(planet.Ptr()->GetPartID(),planet.Ptr()->GetSystemID());
								bMine = true;
								//if slot user is a refinery, set planetholdings[getplanetindex(mID)].bHasRefinery=true fix 
								//what may be best for this is a callback notification to the AI whenver a refinry is bilt
							}
						}

						//should be unnecessary
						/*
						if(!bMine)
						{
							U32 pID = planet.ptr->GetPartID();
							if(IsPlanetOwned(pID)) removePlanet(pID);
						}*/

						//what if planet is out of resources?  fix 

					}
				}
/*
				if(obj->IsVisibleToPlayer(playerID) || (DNA.buildMask.bVisibilityRules == false))
				{
					if (obj->QueryInterface(IFieldID, field)!=0)
					{
						FIELDCLASS fieldType = field->fieldType;
						filedPos = field->GetCenterPos();
					}
				}

					pos
					range
					NEBTYPE nebula = obj->
//////////////////////
enum FIELDCLASS
{
	FC_ASTEROIDFIELD=1,
	FC_MINEFIELD,
	FC_NEBULA,
	FC_ANTIMATTER
};


enum NEBTYPE
{
	NEB_ION,
	NEB_ANTIMATTER,
	NEB_HELIOUS,
	NEB_LITHIUM,
	NEB_HYADES,
	NEB_CELSIUS,
	NEB_CYGNUS
};
//////////////////////

				}
*/
			}
		}

		//addToShadow(part->dwMissionID, shadow);
		obj = obj->nextTarget;
	}
	
	//copyShadowData(&shadow);		// destroys the shadow list	
	cullAssignments();

	//fix up unit totals
	UnitsOwned[M_JUMPPLAT] /= 2;
	////////////////////

	m_bShipRepair = ((UnitsOwned[M_REPAIR] + UnitsOwned[M_GREATER_PLANTATION] + UnitsOwned[M_EUTROMILL]) > 0);

	if(bTenthUpdate || m_bTechInvalid)
	{
		//this would get updated VERY rarely.  like maybe once a minute.
		UpdateTechStatus();
	}
	if(bFifthUpdate || m_bAssignsInvalid)
	{
		if(m_bOnOff)
			MonitorAssignments();	
		if(bFifthUpdate)
		{
			if (findAssignment(RESUPPLY) == 0)
			{	
				ASSIGNMENT * res_ass = new ASSIGNMENT;
				res_ass->Init();
				addAssignment(res_ass);
				res_ass->targetID = 0;
				res_ass->type = RESUPPLY;
				res_ass->systemID = 0;
				res_ass->militaryPower = 0;
				res_ass->uStallTime = 0;
			}

			if (m_bShipRepair && (findAssignment(REPAIR_ASS) == 0))
			{	
				ASSIGNMENT * rep_ass = new ASSIGNMENT;
				rep_ass->Init();
				addAssignment(rep_ass);
				rep_ass->targetID = 0;
				rep_ass->type = REPAIR_ASS;
				rep_ass->systemID = 0;
				rep_ass->militaryPower = 0;
				rep_ass->uStallTime = 0;
			}

		}
	}
	if(bThirdUpdate)
	{
		UpdateResearchDesires();
	}
	if(bSecondUpdate)
	{
		UpdateBuildDesires();
		UpdateShipDesires();
	}

	UpdateFinalSolution();

	//emergency defense
	S32 ageMult = ((m_Age * m_Age) / 2);
	if(ageMult > 10000) ageMult = 10000;
	if(m_ThreatResponse == 0)
	{
		if((!bBuildThreat) && (bestPanic > (DNA.nPanicThreshold + ageMult)) && panicUnit)
		{
			U32 fc = 0;
			bool bFound = false;
			bool bCloaker = panicUnit->bCloaked || panicUnit->bMimic;
			S32 power = 0;
			while(!bFound && fc < 3)
			{
				ASSIGNMENT * threat_node = pAssignments;
				while (threat_node)
				{
					ASSIGNMENTTYPE t = threat_node->type;
					if((t == ATTACK) && (threat_node->targetID == panicUnit->GetPartID()))
					{
						power += threat_node->militaryPower;
					}
					else
					{
						if((t == DEFEND) || (((t == ATTACK) || (t == ESCORT)) && fc) || 
							(((t != SCOUT) || (bCloaker)) && (fc > 1) && (t != RESUPPLY) && 
							(t != REPAIR_ASS)))
						{
							U32 bAll = AllInSystem(threat_node);
							U32 eSys = panicUnit->GetSystemID();
							
							if(bAll && ((bAll == eSys) || (fc && getNumJumps(bAll, eSys, playerID) < 2) ||
								(fc > 1)))
							{
								if( (((S32)threat_node->set.numObjects) > 2) && 
									(((power + threat_node->militaryPower) > (((bestPanicThreat + (ageMult / 8)) * 2) / 3)) || 
									((bestPanic > OH_SHIT) || (bAll == eSys))))
								{
									if(ReissueAttack(threat_node, panicUnit))
									{
										doAttack(threat_node, US_ATTACK, panicUnit);
										m_ThreatResponse = 10;
									}

									power += threat_node->militaryPower;
								}
							}
						}
					}

					if(power > ((bestPanicThreat * 3) / 2))
					{
						bFound = true;
						break;
					}

					threat_node = threat_node->pNext;
				}
				fc++;
			}

		}
	}
	else m_ThreatResponse--;
	
	if(DNA.buildMask.bResignationPossible && 
		(m_UnitTotals[playerID - 1] < 25) && 
		(m_UnitTotals[playerID - 1] > 0))
		ShouldIResign();

	//various debug stuff  fix  
	CQASSERT(m_NumFabPoints <= MAX_DANGER_POINTS);
}
//---------------------------------------------------------------------------//
//  changes AI's perception of what he should be building according to game circumstances
//---------------------------------------------------------------------------//
void SPlayerAI::UpdateResearchDesires(void)
{
	initResearchDesires();

	//U32 amount = 0;
	//U32 req_amount = 0;

	for(U32 c = (AI_TECH_END - 1); c > NOTECH; c--)
	{
		if(GetTechRace((AI_TECH_TYPE)c) != m_nRace) continue;

		ResearchPriority *cur = &(ResearchDesires[c]);

		if(cur->bAcquired)
		{
			cur->nPriority = -100000;
			continue;
		}

		if(cur->prerequisite && cur->prerequisite < AI_TECH_END && 
			ResearchDesires[cur->prerequisite].bAcquired == false)
		{
			S32 newpri = cur->nPriority;
			if(newpri < 0) newpri = 0;
			ResearchDesires[cur->prerequisite].nPriority += cur->nPriority >> 1;
		}

		if(cur->facility && cur->facility < M_ENDOBJCLASS && (UnitsOwned[cur->facility] == 0))
		{
			S32 newpri = cur->nPriority;
			if(newpri < 0) newpri = 0;
			BuildDesires[cur->facility].nPriority += newpri >> 2;
		}

		//hmmmmmmmmmmmmmm
		S32 agebonus = m_Age / 2;
		if(agebonus > 400) agebonus = 400;
		cur->nPriority += agebonus;
	}
	
	//Various Modifiers
	if ((UnitsOwned[M_TROOPSHIP] + UnitsOwned[M_LEECH] + UnitsOwned[M_LEGIONAIRE]) > 0)
	{
		ResearchDesires[T_TROOPSHIP1].nPriority += DNA.nNumTroopships * 450;
		ResearchDesires[T_TROOPSHIP2].nPriority += DNA.nNumTroopships * 250;
		ResearchDesires[T_TROOPSHIP3].nPriority += DNA.nNumTroopships * 250;
		ResearchDesires[M_LEECH1].nPriority += DNA.nNumTroopships * 450;
		ResearchDesires[M_LEECH2].nPriority += DNA.nNumTroopships * 250;
		ResearchDesires[S_LEGIONAIRE1].nPriority += DNA.nNumTroopships * 450;
		ResearchDesires[S_LEGIONAIRE2].nPriority += DNA.nNumTroopships * 250;
		ResearchDesires[S_LEGIONAIRE3].nPriority += DNA.nNumTroopships * 250;
		ResearchDesires[S_LEGIONAIRE4].nPriority += DNA.nNumTroopships * 250;
	}

	if (DNA.uNumScouts)
	{
		//more sensors?
	}

	if (m_bRegenResources)
	{
		ResearchDesires[T_ORE1].nPriority += 70;
		ResearchDesires[T_ORE2].nPriority += 70;
		ResearchDesires[T_ORE3].nPriority += 70;

		ResearchDesires[T_GAS1].nPriority += 70;
		ResearchDesires[T_GAS2].nPriority += 70;
		ResearchDesires[T_GAS3].nPriority += 70;

		ResearchDesires[S_ORE1].nPriority += 70;
		ResearchDesires[S_ORE2].nPriority += 70;
		ResearchDesires[S_ORE3].nPriority += 70;

		ResearchDesires[S_GAS1].nPriority += 70;
		ResearchDesires[S_GAS2].nPriority += 70;
		ResearchDesires[S_GAS3].nPriority += 70;
	}

	ResearchDesires[T_LANCERVAMPIRE].nPriority += UnitsOwned[M_LANCER] * UnitsOwned[M_LANCER] * 16;
	ResearchDesires[T_MISSILECLOAKING].nPriority += UnitsOwned[M_MISSILECRUISER] * UnitsOwned[M_MISSILECRUISER] * 2;
	ResearchDesires[T_BATTLESHIPCHARGE].nPriority += UnitsOwned[M_BATTLESHIP] * UnitsOwned[M_BATTLESHIP] * 16;
	ResearchDesires[T_CARRIERPROBE].nPriority += UnitsOwned[M_CARRIER] * UnitsOwned[M_CARRIER] * 4;
	ResearchDesires[T_DREADNOUGHTSHIELD].nPriority += UnitsOwned[M_DREADNOUGHT] * UnitsOwned[M_DREADNOUGHT] * 16;
	
	ResearchDesires[M_REPULSOR].nPriority += UnitsOwned[M_TIAMAT] * UnitsOwned[M_TIAMAT] * 16;
	ResearchDesires[M_CAMOFLAGE].nPriority += UnitsOwned[M_FRIGATE] * UnitsOwned[M_FRIGATE] * 2;
	ResearchDesires[M_GRAVWELL].nPriority += UnitsOwned[M_SCARAB] * UnitsOwned[M_SCARAB] * 16;
	ResearchDesires[M_REPELCLOUD].nPriority += UnitsOwned[M_HIVECARRIER] * UnitsOwned[M_HIVECARRIER] * 16;
	ResearchDesires[M_RAM1].nPriority += UnitsOwned[M_KHAMIR] * UnitsOwned[M_KHAMIR] * 4;
	ResearchDesires[M_RAM2].nPriority += UnitsOwned[M_KHAMIR] * UnitsOwned[M_KHAMIR] * 4;

	ResearchDesires[S_DESTABILIZER].nPriority += UnitsOwned[M_TRIREME] * UnitsOwned[M_TRIREME] * 16;
	ResearchDesires[S_CLOAKER].nPriority += UnitsOwned[M_AURORA] * UnitsOwned[M_AURORA] * 8;
	ResearchDesires[S_MASSDISRUPTOR].nPriority += UnitsOwned[M_POLARIS] * UnitsOwned[M_POLARIS] * 8;
	ResearchDesires[S_TRACTOR].nPriority += UnitsOwned[M_MONOLITH] * UnitsOwned[M_MONOLITH] * 16;
	ResearchDesires[S_SYNTHESIS].nPriority += UnitsOwned[M_TAOS] * UnitsOwned[M_TAOS] * 2;

	//based on resource situations forecasts on the map... increase gas or ore research
	//priorities of which one is likely to be more scarce  fix  

	//special case stuff
	/*
	S32 comptmultiplier = 0;
	if(m_ComPts <= m_ComPtsUsedUp) comptmultiplier = 1;
	else comptmultiplier = m_ComPts - m_ComPtsUsedUp;

	if(comptmultiplier > 20) comptmultiplier = 20;
	comptmultiplier = 20 - comptmultiplier;
	comptmultiplier = comptmultiplier * comptmultiplier;

	BuildDesires[M_HQ].nPriority += comptmultiplier;
	BuildDesires[M_ACADEMY].nPriority += comptmultiplier / 2;
	BuildDesires[M_OUTPOST].nPriority += comptmultiplier / 3;
	BuildDesires[M_COCOON].nPriority += comptmultiplier;
	BuildDesires[M_WARLORDTRAINING].nPriority += comptmultiplier / 3;
	BuildDesires[M_THRIPID].nPriority += comptmultiplier / 2;
	BuildDesires[M_ACROPOLIS].nPriority += comptmultiplier;
	BuildDesires[M_CITADEL].nPriority += comptmultiplier / 2;
	BuildDesires[M_BUNKER].nPriority += comptmultiplier / 3;
	*/
	///////////////////////////////////////////////////////
}
//---------------------------------------------------------------------------//
//  changes AI's perception of what he should be building according to game circumstances
//---------------------------------------------------------------------------//
void SPlayerAI::UpdateBuildDesires(void)
{
	initBuildDesires();

	U32 syssessupplied = 0;
	U32 c;
	for(c = 0; c < MAX_SYSTEMS; c++)
		if(m_bSystemSupplied[c]) syssessupplied++;

	if(syssessupplied == 1) syssessupplied = 0;
	else if(syssessupplied >= 1) syssessupplied--;

	U32 amount = 0;
	U32 req_amount = 0;

	bool bTreeDone = ((ReadyForBuilding() == false) && (m_RepairSite == false));

	for(c = M_FABRICATOR /* 1 */; c < M_ENDOBJCLASS; c++)
	{
		BuildPriority *cur = &(BuildDesires[c]);

		if(!MGlobals::IsPlatform((M_OBJCLASS)c))
		{
			cur->nPriority = -100000;
			continue;
		}

		amount = UnitsOwned[c];

		if((c == M_HEAVYIND || c == M_NIAD || c == M_GREATERPAVILION) &&
			DNA.buildMask.bBuildHeavyGunboats == 0 && DNA.buildMask.bBuildMediumGunboats == 0)
		{
			cur->nPriority = -1000;
			continue;
		}

		U32 req = cur->prerequisite;
		CQASSERT((req >= M_NONE) && (req < M_ENDOBJCLASS));
		
		req_amount = UnitsOwned[req];

		U32 add = HowManyFabPoints((M_OBJCLASS)c);
		U32 req_add = HowManyFabPoints((M_OBJCLASS)req);
		//CQASSERT(add < 4 && req_add < 4);
		amount += add;
		req_amount += req_add;

		if(cur->nExponentialDecrement > 0)
		{
			int x, iterations = 0;
			if(cur->nPlanetMultiplier > 0) iterations -= m_PlanetsUnderMyControl;
			if(cur->nSystemMultiplier > 0) iterations -= syssessupplied;
			iterations += amount;
			for(x = 0; x < iterations; x++)
				cur->nPriority /= cur->nExponentialDecrement;
		}
		else
		{
			cur->nPriority -= (amount * cur->nAdditiveDecrement);
			if(cur->nPlanetMultiplier > 0)
				cur->nPriority += m_PlanetsUnderMyControl * cur->nPlanetMultiplier * ((cur->nAdditiveDecrement * 3) / 4);
			if(cur->nSystemMultiplier > 0)
				cur->nPriority += syssessupplied * cur->nSystemMultiplier * ((cur->nAdditiveDecrement) / 2);
		}

		if((req_amount == 0) && (req > M_NONE) && (req < M_ENDOBJCLASS))
		{
			if(cur->nPriority > 0) BuildDesires[req].nPriority += (cur->nPriority / 4);
			cur->nPriority -= 2500;
		}

		if(bTreeDone && (MGlobals::IsGunPlat((M_OBJCLASS)c) == false))
			cur->nPriority -= 300;

	}

	//m_Gas;
	//m_Metal;
	
	////////////crew stores factoring
	S32 crew_deficiency = ((300 - ((S32)m_Crew)) / 8) + ((m_ComPts - 15) / 2);
	S32 crew_surplus = ((S32)m_Crew) / 5;
	if(crew_deficiency < 0) crew_deficiency = 0;

	if(((MGlobals::GetMaxCrew(playerID) * CREW_MULT) < 1260) && (SECTOR->GetNumSystems() > 3))
		crew_deficiency += 15;
	
	BuildDesires[M_CITADEL].nPriority += (crew_deficiency * crew_deficiency / 10) - crew_surplus;
	BuildDesires[M_BUNKER].nPriority += ((crew_deficiency * crew_deficiency) / 20)  - crew_surplus;
	BuildDesires[M_WARLORDTRAINING].nPriority += (crew_deficiency * crew_deficiency / 15) - crew_surplus;
	BuildDesires[M_ACADEMY].nPriority += ((crew_deficiency * crew_deficiency) / 12) - crew_surplus;
	BuildDesires[M_OUTPOST].nPriority += ((crew_deficiency * crew_deficiency) / 20) - crew_surplus;
	
	//////////////////////Resource Maximums Stuff
	S32 mg = 10 - (UnitsOwned[M_REFINERY] + UnitsOwned[M_COLLECTOR] + UnitsOwned[M_OXIDATOR] +
					(UnitsOwned[M_HEAVYREFINERY]/2) + 
					(UnitsOwned[M_SUPERHEAVYREFINERY]/2));  //perhaps instead of just 10, this constant should be based on num systems/num players in the game  fix  
	S32 crewf = 10 - (UnitsOwned[M_ACADEMY] + UnitsOwned[M_COLLECTOR] + UnitsOwned[M_CITADEL] + 
						UnitsOwned[M_OUTPOST] + UnitsOwned[M_BUNKER]);  //or maybe even based on how much of this resource i expect to need  fix  
	if(mg < 0) mg = 0;
	if((m_Gas > (S32)MGlobals::GetMaxGas(playerID) - METALGAS_MULT) ||
		(m_Metal > (S32)MGlobals::GetMaxMetal(playerID) - METALGAS_MULT))
	{
		BuildDesires[M_REFINERY].nPriority += (mg * mg * 2);
		BuildDesires[M_COLLECTOR].nPriority += (mg * mg * 2);
		BuildDesires[M_OXIDATOR].nPriority += (mg * mg * 2);
	}
	if(m_Crew > (S32)MGlobals::GetMaxCrew(playerID) - CREW_MULT)
	{
		BuildDesires[M_ACADEMY].nPriority += (crewf * crewf * 2);
		BuildDesires[M_OUTPOST].nPriority += (crewf * crewf * 2);
		BuildDesires[M_WARLORDTRAINING].nPriority += (crewf * crewf * 2);
		BuildDesires[M_GREATER_COLLECTOR].nPriority += (crewf * crewf * 2);
		BuildDesires[M_BUNKER].nPriority += (crewf * crewf * 2);
		BuildDesires[M_CITADEL].nPriority += (crewf * crewf * 2);
	}
	/////////////////////////////

	//////////command point stuff
	S32 comptmultiplier = 0;
	S32 approachingmax = 0;
	S32 mcp = MGlobals::GetMaxControlPoints(playerID);
	if(m_ComPts <= m_ComPtsUsedUp) comptmultiplier = 1;
	else comptmultiplier = m_ComPts - m_ComPtsUsedUp;
	approachingmax = mcp - m_ComPts;
	if(approachingmax > 20) approachingmax = 20;
	if(approachingmax < 0) approachingmax = 0;
	approachingmax -= 10;

	if(comptmultiplier > 10) comptmultiplier = 10;
	comptmultiplier = 10 - comptmultiplier + approachingmax;
	if(comptmultiplier < 0) comptmultiplier = 0;  //shouldn't be necessary, logically
	comptmultiplier = ((comptmultiplier * comptmultiplier * comptmultiplier) / 16);

	BuildDesires[M_HQ].nPriority += (comptmultiplier / 2);
	BuildDesires[M_LRSENSOR].nPriority += comptmultiplier;
	
	BuildDesires[M_COCOON].nPriority += (comptmultiplier / 2);
	BuildDesires[M_EYESTOCK].nPriority += comptmultiplier;

	BuildDesires[M_ACROPOLIS].nPriority += (comptmultiplier / 2);	
	BuildDesires[M_SENTINELTOWER].nPriority += comptmultiplier;

	if(m_ComPts > mcp-10)
	{
		if(m_ComPts > mcp-3)
		{
			BuildDesires[M_LRSENSOR].nPriority -= 500;
			BuildDesires[M_EYESTOCK].nPriority -= 500;
			BuildDesires[M_SENTINELTOWER].nPriority -= 500;

			BuildDesires[M_PORTAL].nPriority -= 1000;
			BuildDesires[M_IONCANNON].nPriority -= 1000;
		}


		BuildDesires[M_LRSENSOR].nPriority -= 200;
		BuildDesires[M_EYESTOCK].nPriority -= 200;
		BuildDesires[M_SENTINELTOWER].nPriority -= 200;

		BuildDesires[M_LSAT].nPriority -= 500;
		BuildDesires[M_SPACESTATION].nPriority -= 500;
		BuildDesires[M_IONCANNON].nPriority -= 200;
		BuildDesires[M_PLASMASPLITTER].nPriority -= 500;
		BuildDesires[M_VORAAKCANNON].nPriority -= 250;
		BuildDesires[M_PLASMAHIVE].nPriority -= 500;
		BuildDesires[M_PROTEUS].nPriority -= 500;
		BuildDesires[M_HYDROFOIL].nPriority -= 500;
		BuildDesires[M_ESPCOIL].nPriority -= 500;
		BuildDesires[M_STARBURST].nPriority -= 500;
	}

	////// special case stuff for opening-game
	if(m_nRace == M_TERRAN)
	{
		U32 refs = UnitsOwned[M_REFINERY] + (UnitsOwned[M_HEAVYREFINERY]/2) + (UnitsOwned[M_SUPERHEAVYREFINERY]/2);
		if((refs < 3) && (m_Metal < 2500))
			BuildDesires[M_LIGHTIND].nPriority = 0;
	}

	////// special case for HQ games
	if(m_bKillHQ)
	{
		BuildDesires[M_HQ].nPriority += 170;// maybe a bit more complex?  like based on age or something else?  fix  
		BuildDesires[M_COCOON].nPriority += 170;
		BuildDesires[M_ACROPOLIS].nPriority += 170;
	}

	if (m_bRegenResources)
	{
		ResearchDesires[M_REFINERY].nPriority += 120;
		ResearchDesires[M_COLLECTOR].nPriority += 120;
		ResearchDesires[M_GREATER_COLLECTOR].nPriority += 220;

		ResearchDesires[M_OUTPOST].nPriority += 100;
		ResearchDesires[M_OXIDATOR].nPriority += 100;
		ResearchDesires[M_BUNKER].nPriority += 100;
		ResearchDesires[M_WARLORDTRAINING].nPriority += 60;
		ResearchDesires[M_ACADEMY].nPriority += 60;
	}

	if ((UnitsOwned[M_HQ] + UnitsOwned[M_COCOON] + UnitsOwned[M_ACROPOLIS]) == 0)
	{
		BuildDesires[M_HQ].nPriority += 500;
		BuildDesires[M_COCOON].nPriority += 500;
		BuildDesires[M_ACROPOLIS].nPriority += 500;
	}

	////////////////////////////////////////////////
}
//---------------------------------------------------------------------------//
// 
void SPlayerAI::UpdateShipDesires(void)
{
	initShipDesires();

	S32 comptmultiplier = m_ComPts;
	//if(m_ComPts <= m_ComPtsUsedUp) comptmultiplier = 1;
	if(comptmultiplier < 1) comptmultiplier = 1;
	if(comptmultiplier > 99) comptmultiplier = 99;
	comptmultiplier /= 5;
	comptmultiplier = comptmultiplier * comptmultiplier;

	U32 amount = 0;

	for(U32 c = M_FABRICATOR /* 1 */; c < M_ENDOBJCLASS; c++)
	{
		amount = UnitsOwned[c];
		ShipPriority *cur = &(ShipDesires[c]);

		if(MGlobals::IsPlatform((M_OBJCLASS)c))
		{
			cur->nPriority = -100000;
			continue;
		}

		if((!DNA.buildMask.bBuildLightGunboats && MGlobals::IsLightGunboat((M_OBJCLASS)c)) || 
			(!DNA.buildMask.bBuildMediumGunboats && MGlobals::IsMediumGunboat((M_OBJCLASS)c)) ||
			(!DNA.buildMask.bBuildHeavyGunboats && MGlobals::IsHeavyGunboat((M_OBJCLASS)c)))
		{
			cur->nPriority = -100000;
			continue;
		}

		/////////////build bigger ships later in game
		if(MGlobals::IsMediumGunboat((M_OBJCLASS)c)) cur->nPriority += (comptmultiplier/2);
		else if(MGlobals::IsHeavyGunboat((M_OBJCLASS)c)) cur->nPriority += comptmultiplier;

		M_OBJCLASS failclass = M_NONE;
		if(HasDependencies((M_OBJCLASS)c, &failclass)) cur->nPriority += 100;
		/////////////////////////////////////////////

		if(cur->nExponentialDecrement > 0)
		{
			int x, iterations = 0;
			iterations += amount;
			for(x = 0; x < iterations; x++)
				cur->nPriority /= cur->nExponentialDecrement;
		}
		else
		{
			cur->nPriority -= (amount * cur->nAdditiveDecrement);
		}

		U32 fac = cur->facility;
		CQASSERT(fac < M_ENDOBJCLASS);
		if(!UnitsOwned[fac])
		{
			if(cur->nPriority > 0) BuildDesires[fac].nPriority += (cur->nPriority / 4);  //divide by four
			cur->nPriority -= 1000;
		}
	}

	////////////////////////////////////special case stuff	
	if((UnitsOwned[M_INFILTRATOR] + UnitsOwned[M_SEEKER] + UnitsOwned[M_ORACLE]) >= DNA.uNumScouts)
	{
		ShipDesires[M_INFILTRATOR].nPriority -= 3000;
		ShipDesires[M_SEEKER].nPriority -= 3000;
		ShipDesires[M_ORACLE].nPriority -= 3000;
	}

	//hmmmmmmmmmmmmmm
	S32 agebonus = m_Age / 2;
	if(agebonus > 1500) agebonus = 1500;
	ShipDesires[M_INFILTRATOR].nPriority -= agebonus;
	ShipDesires[M_SEEKER].nPriority -= agebonus;
	ShipDesires[M_ORACLE].nPriority -= agebonus;

	if(((S32)(UnitsOwned[M_TROOPSHIP] + UnitsOwned[M_LEECH] + UnitsOwned[M_LEGIONAIRE]))
		>= DNA.nNumTroopships)
	{
		ShipDesires[M_TROOPSHIP].nPriority -= 3000;
		ShipDesires[M_LEECH].nPriority -= 3000;
		ShipDesires[M_LEGIONAIRE].nPriority -= 3000;
	}

	if(((S32)(UnitsOwned[M_SPINELAYER] + UnitsOwned[M_ATLAS])) >=  DNA.nNumMinelayers)
	{
		ShipDesires[M_SPINELAYER].nPriority -= 3000;
		ShipDesires[M_ATLAS].nPriority -= 3000;
	}
	///////////////////////////////////////////////////////
}
//---------------------------------------------------------------------------//
// 
void SPlayerAI::SpreadThreat(S32 x, S32 y, U32 sys, MPart & part, bool bThreat)
{
	S32 fx = x + 2;
	S32 fy = y + 2;
	S32 pr = 1;
	if(bThreat) pr = GetPowerRating(part);
	else pr = GetImportance(part);

	for(S32 cy = y - 2; cy < fy; cy++)
	{
		for(S32 cx = x - 2; cx < fx; cx++)
		{
			if(((cx >= 0) && (cx < 16)) && ((cy >= 0) && (cy < 16)))
			{
				S32 div = 1;
				div = abs(x - cx) + abs(y - cy) + div;
				S32 val = 0;
				if(bThreat)
				{
					val = pr / div;
					m_Threats[cy][cx][sys].threat += val;
				}
				else
				{
					val = pr / div;
					m_Threats[cy][cx][sys].importance += val;
				}
			}
		}
	}
}
//---------------------------------------------------------------------------//
// 
void SPlayerAI::ShouldIResign(void)
{
	bool bHopeless = false;
	U32 nFabs = UnitsOwned[M_FABRICATOR] + UnitsOwned[M_WEAVER] + UnitsOwned[M_FORGER];
	U32 nHQs  = UnitsOwned[M_HQ] + UnitsOwned[M_COCOON] + UnitsOwned[M_ACROPOLIS];
	U32 nTroops  = UnitsOwned[M_TROOPSHIP] + UnitsOwned[M_LEECH] + UnitsOwned[M_LEGIONAIRE];
	
	bHopeless = (bHopeless || ((nFabs == 0) && (nHQs == 0) && (nTroops == 0)));

	const U32 allyMask = MGlobals::GetAllyMask(playerID);
	U32 myCount = 0;
	U32 allyCount = 0;
	U32 enemyCount = 0;
	S32 allyPlayerID = -1;

	for(U32 c = 0; c < MAX_PLAYERS; c++)
	{
		if(c + 1 == playerID)
		{
			myCount += m_UnitTotals[c];
		}
		else if(allyMask & (1 << c))
		{
			allyPlayerID = c + 1;
			allyCount += m_UnitTotals[c];
		}
		else enemyCount += m_UnitTotals[c];
	}

	U32 strength = (((myCount * 2) + (allyCount / 2)) * 12);
	if(strength < enemyCount || 
		(bHopeless))
	{
		if(m_ResignComing == 0)
		{
			//get ready to resign	
			m_ResignComing = 3;

			wchar_t text[MAX_CHAT_LENGTH];

			if(m_nRace == M_TERRAN)
				wcsncpy(text, _localLoadStringW(IDS_AI_RESIGN), MAX_CHAT_LENGTH);
			else if(m_nRace == M_MANTIS)
				wcsncpy(text, _localLoadStringW(IDS_AI_MANTIS_RESIGN), MAX_CHAT_LENGTH);
			else
				wcsncpy(text, _localLoadStringW(IDS_AI_CELERY_RESIGN), MAX_CHAT_LENGTH);

			U8 mask = 255;
			SendChatMessage(text, mask);
		}
	}
	else
	{
		if(m_ResignComing && (strength - 50 > enemyCount))
		{
			//just kidding
			wchar_t text[MAX_CHAT_LENGTH];
			wcsncpy(text, _localLoadStringW(IDS_AI_NEVERMIND), MAX_CHAT_LENGTH);
			U8 mask = 255;
			SendChatMessage(text, mask);

			m_ResignComing = 0;
		}
	}
	
	if(m_ResignComing > 0)
	{
		m_ResignComing--;

		if(m_ResignComing < 1)
		{
			/*
			BASE_PACKET packet;
			packet.dwSize = sizeof(packet);
			packet.type = PT_RESIGN;
			NETPACKET->Send(playerID, 0, &packet);
			*/
			MISSION->SetAIResign(playerID);
			m_bOnOff = false;
			DNA.buildMask.bSendTaunts = false;
		}
		else if(allyPlayerID >= 0)
		{
			U8 mask = 0;
			switch(m_ResignComing)
			{
			case 3:
				if(allyPlayerID > 0)
				{
					wchar_t text[MAX_CHAT_LENGTH];
					wcsncpy(text, _localLoadStringW(IDS_AI_GIFT), MAX_CHAT_LENGTH);
					mask = 1 << (((U32)allyPlayerID) - 1);
					SendChatMessage(text, mask);
				}
			break;
			case 2:
				{
				GIFTORE_PACKET packet;
				packet.dwSize = sizeof(GIFTORE_PACKET);
				packet.giverID = playerID;
				packet.recieveID = allyPlayerID;
				packet.amount = MGlobals::GetCurrentMetal(playerID);
				NETPACKET->Send(HOSTID, 0, &packet);
				}
				{
				GIFTGAS_PACKET packet;
				packet.dwSize = sizeof(GIFTORE_PACKET);
				packet.giverID = playerID;
				packet.recieveID = allyPlayerID;
				packet.amount = MGlobals::GetCurrentGas(playerID);
				NETPACKET->Send(HOSTID, 0, &packet);
				}
				{
				GIFTCREW_PACKET packet;
				packet.dwSize = sizeof(GIFTORE_PACKET);
				packet.giverID = playerID;
				packet.recieveID = allyPlayerID;
				packet.amount = MGlobals::GetCurrentCrew(playerID);
				NETPACKET->Send(HOSTID, 0, &packet);
				}
			break;
			default:
			break;
			}
		}
	}
}

//---------------------------------------------------------------------------//
// 
void SPlayerAI::UpdateFinalSolution(void)
{
	CQASSERT(playerID > 0);
	U32 mycount = m_UnitTotals[playerID - 1];
	U32 hiscount= 0;

	if(m_Terminate && (m_Terminate <= MAX_PLAYERS))
	{
		hiscount = m_UnitTotals[m_Terminate - 1];
		if(hiscount)
		{
			//consider renig on terminate
			if((mycount < 10) || (mycount - hiscount < mycount / 2))
			{
				m_Terminate = 0;
			}
		}
		else
		{
			m_Terminate = 0;
		}
	}
	else
	{
		const U32 allyMask = MGlobals::GetAllyMask(playerID);
		for(U32 c = 0; c < MAX_PLAYERS; c++)
		{
			if((c + 1 == playerID) || (allyMask & (1 << c))) continue;

			hiscount = m_UnitTotals[c];

			if(hiscount && (mycount > 10) && (mycount - hiscount > (mycount / 2 + 1)))
			{
				m_Terminate = c + 1;
				break;
			}
		}
	}
}
//---------------------------------------------------------------------------//
// 
S32 SPlayerAI::UpdateTechStatus(bool bFull)
{
	S32 count = 0;
	TECHNODE tn1, tn2;
	tn1 = MGlobals::GetCurrentTechLevel(playerID);
	tn2 = MGlobals::GetWorkingTechLevel(playerID);

	for(U32 c = (AI_TECH_END - 1); c > NOTECH; c--)
	{
		if(bFull)
		{
			ResearchDesires[c].bAcquired = false;

			U32 pArcheID = GetTechArcheID((AI_TECH_TYPE)c);
			BASE_RESEARCH_DATA * resData = (BASE_RESEARCH_DATA *) (ARCHLIST->GetArchetypeData(pArcheID));
			BT_RESEARCH * research = (BT_RESEARCH *) resData;

			if(research)
			{
				if(tn1.HasTech(research->researchTech))
					ResearchDesires[c].bAcquired = true;

				if(tn2.HasTech(research->researchTech))
					ResearchDesires[c].bAcquired = true;
			}
		}

		if(ResearchDesires[c].bAcquired) count++;
	}

	m_bTechInvalid = false;
	return count;
}
//---------------------------------------------------------------------------//
//
bool SPlayerAI::SiteTaken(U32 planet, S32 slot)
{
	for(U32 c = 0; c < m_NumFabPoints; c++)
	{
		//S32 slots = BuildDesires[m_FabPoints[c].plattype].nNumSlots;  fix  
		if((m_FabPoints[c].planet == planet) && (m_FabPoints[c].slot == slot))
		{
			return true;
		}
	}

	return false;
}
//---------------------------------------------------------------------------//
//
DOUBLE SPlayerAI::ShouldIBuild(OBJPTR<IPlanet> planet, PARCHETYPE pArchetype, BuildSite & site)
{
	DOUBLE result = 0.0;

	if(!site.planet) return 10e8;

	//include fab points in this check   fix  

	BASE_PLATFORM_DATA* platdata = (BASE_PLATFORM_DATA*)ARCHLIST->GetArchetypeData(pArchetype);
	bool bRefinery = (platdata->type == PC_REFINERY);
	//if(BANKER->HasCost(playerID, sdata->missionData.resourceCost))
	//		return true;

	BT_PLAT_REFINE_DATA* refinedata = (BT_PLAT_REFINE_DATA*)platdata;

	IBaseObject * obj = OBJLIST->FindObject(site.planet);
	if(obj == 0) return false;

	PARCHETYPE pPlanetArchetype = obj->pArchetype;
	BT_PLANET_DATA * planetdata = (BT_PLANET_DATA *)ARCHLIST->GetArchetypeData(pPlanetArchetype);

	DOUBLE maxOre = 4000, maxCrew = 2500, maxGas = 3000;
	if(site.plattype == M_COLLECTOR)
	{
		maxOre = 3000;
		maxCrew= 3500;
		maxGas = 3000;
	}
	else if (site.plattype == M_OXIDATOR)
	{
		maxOre = 3000;
		maxGas = 4000;
		maxCrew= 1500;
	}
	DOUBLE GasMult = (maxGas - m_Gas) / 256;
	DOUBLE OreMult = (maxOre - m_Metal) / 256;
	DOUBLE CrewMult= (maxCrew - m_Crew) / 256;
	GasMult  *= GasMult;
	OreMult  *= OreMult;
	CrewMult *= CrewMult;
	if(GasMult < 0.0) GasMult = 0.0;
	if(OreMult < 0.0) OreMult = 0.0;
	if(CrewMult < 0.0) CrewMult = 0.0;

	if(bRefinery)
	{
		bool bHarvGas = (refinedata->gasRate[0] > 0.0);
		bool bHarvOre = (refinedata->metalRate[0] > 0.0);
		bool bHarvCrew= (refinedata->crewRate[0] > 0.0);
		bool bHasCrew = (planetdata->maxCrew > 0.0);
		bool bHasOre  = (planetdata->maxMetal > 0.0);
		bool bHasGas  = (planetdata->maxGas > 0.0);

		if(m_bRegenResources == 0)
		{
			if(planet)
			{
				bHasGas = (planet->GetGas() > 25);
				bHasOre = (planet->GetMetal() > 25);
				bHasCrew= (planet->GetCrew() > 25);
			}
		}

		if(bHarvGas && !bHasGas)
		{
			if(!bHarvOre && !bHarvCrew)
				result += 10e4;
			else
				result += GasMult;
		}
		if(bHarvOre && !bHasOre)
		{
			if(!bHarvGas && !bHarvCrew)
				result += 10e4;
			else
				result += OreMult;
		}
		if(bHarvCrew && !bHasCrew)
		{
			if(!bHarvGas && !bHarvOre)
				result += 10e4;
			else
				result += CrewMult;
		}

		if(!bHarvCrew && bHasCrew)
			result += (CrewMult / 2);

		if(!bHarvCrew && bHarvGas && bHarvOre && bHasCrew && !bHasOre && !bHasGas)
			result += 20;


/*
	}
		if(m_Gas > m_Metal)
		{
			if(!bHasOre)
				result += 10e2;
		}

		if(bHarvGas && !bHasGas) bNoGas = true;
		if(((refinedata->metalRate[0]) > 0.0) && (!bHasOre)) bNoMetal = true;
		if(((refinedata->crewRate[0]) > 0.0) && !bHasCrew) bNoCrew = true;

		if((bNoGas && bNoMetal) || bNoCrew)
		{
			result += 10e6;
		}
		
		if((refinedata->crewRate[0] <= 0.0) && bNoGas && (m_Metal > m_Gas))
		{
			result += 10e3;
		}
		
		if((refinedata->crewRate <= 0.0) && bNoMetal && (m_Gas > m_Metal))
		{
			result += 10e3;
		}

		if((refinedata->crewRate[0] <= 0.0) && (planetdata->maxCrew > 0.0))
			result += 20;
			*/
	}
	else if(planetdata->maxCrew > 0.0) //earthplanet
	{
		result += 4 + CrewMult / 2;
	}

	U32 sysID = site.pos.systemID;
	if((sysID > 0) && (sysID <= MAX_SYSTEMS) && (m_bSystemSupplied[sysID - 1] == false) &&
		(MGlobals::IsHQ(site.plattype) == false) && (site.plattype != M_JUMPPLAT) && (!MGlobals::IsRefinery(site.plattype)))
	{
		result += 10e4;
	}
	

	//struct BT_PLAT_REFINE_DATA : BASE_PLATFORM_DATA
	//{
	//SINGLE gasRate;
	//SINGLE metalRate;
	//SINGLE crewRate;
	//};

	//planetdata->maxGas;
	//planetdata->maxMetal;
	//planetdata->maxCrew;
	///////////////////////////////////////

	
	//disable the following in corvette rush for speed-harvesting?
	
	OBJPTR<IPlanet> planetinterface;
	if (obj->QueryInterface(IPlanetID, planetinterface)!=0)  //if object is a planet
	{
		//if(this is a gunplat, ignore per-planet dupe restrictions
		int c;
		for(c = 1; c <= M_MAX_SLOTS; c++)
		{
			U32 bitField = 1 << (c - 1);
			U32 mID = planetinterface->GetSlotUser(bitField);
			if(mID)
			{
				MPart part = OBJLIST->FindObject(mID);
				M_OBJCLASS moc = part->mObjClass;
				if(mID && ((mID & PLAYERID_MASK) == playerID))
				{
					if(MGlobals::IsRefinery(moc) && MGlobals::IsRefinery(site.plattype) && 
						(moc == site.plattype))
					{
						if(m_bRegenResources) result += 55;
						else result += 85;
					}
				}
			}
		}

		if(MGlobals::IsRefinery(site.plattype))
		{
			for(c = 0; c < ((S32)m_NumFabPoints); c++)
			{
				if(m_FabPoints[c].planet == planet.Ptr()->GetPartID() && 
					MGlobals::IsRefinery(m_FabPoints[c].plattype) && 
					(m_FabPoints[c].plattype == site.plattype))
				{
					if(m_bRegenResources) result += 56;
					else result += 70;			
				}
			}
		}
	}

	return result;
}
//---------------------------------------------------------------------------//
//  blah
//---------------------------------------------------------------------------//
void SPlayerAI::IncreaseSlotPriorities(S32 slotnum, S32 amount)
{
	for(int c = M_NONE; c < M_ENDOBJCLASS; c++)
	{
		if(GetRace(c) != m_nRace) continue;
		if(c == M_JUMPPLAT) continue;

		if(BuildDesires[c].nNumSlots == slotnum)
			BuildDesires[c].nPriority += amount;
	}
}

//---------------------------------------------------------------------------//
//  For fabricators.... tell me what to go build
//---------------------------------------------------------------------------//
M_OBJCLASS SPlayerAI::ChooseNextBuild(int *fail, M_RACE race, bool bPlat, bool bFab)
{
	int c;
	int bestpri = -1000;
	int bestobj = M_NONE;

	for(c = M_NONE; c < M_ENDOBJCLASS; c++)
	{
		if(bPlat && IsMilitary(c)) continue;
		if(bFab && BuildDesires[c].bMutation) continue;
		if(race != GetRace(c)) continue;
		if(c == M_PORTAL)
		{
			NETGRIDVECTOR ngv = ChooseAssignmentSite(DEFEND);
			if(ngv.systemID == 0)
				continue;
		}

		if(HowManyFabPoints(c)) continue;

		if(BuildDesires[c].nPriority > bestpri)
		{
			bestpri = BuildDesires[c].nPriority;
			*fail = bestpri;
			bestobj = c;
		}
	}

	return (M_OBJCLASS)bestobj;
}
//---------------------------------------------------------------------------//
//  For Shipyards.... tell me what to build
//---------------------------------------------------------------------------//
M_OBJCLASS SPlayerAI::ChooseNextShip(int *fail, M_RACE race, M_OBJCLASS facil)
{
	int c;
	int bestpri = -1000;
	int bestobj = M_NONE;

	for(c = M_FABRICATOR; c < M_ENDOBJCLASS; c++)
	{
		if(MGlobals::IsPlatform((M_OBJCLASS)c)) continue;
		if(GetRace(c) != race) continue;
		if((ShipDesires[c].facility != facil) && 
			(facil != M_NIAD) && (ShipDesires[c].facility != M_THRIPID))
			continue;

		if(ShipDesires[c].nPriority > bestpri)
		{
			bestpri = ShipDesires[c].nPriority;
			bestobj = c;
			*fail = bestpri;
		}
	}

	return (M_OBJCLASS)bestobj;
}
//---------------------------------------------------------------------------//
//  For fabricators.... tell me what to go build
//---------------------------------------------------------------------------//
AI_TECH_TYPE SPlayerAI::ChooseNextTech(int *fail)
{
	int c;
	int bestpri = -1000;
	AI_TECH_TYPE bestobj = NOTECH;

	for(c = 0; c < AI_TECH_END; c++)
	{
		AI_TECH_TYPE indx = (AI_TECH_TYPE)c;
		if(ResearchDesires[c].bAcquired) continue;
		if(ResearchDesires[c].prerequisite > NOTECH && ResearchDesires[c].prerequisite < AI_TECH_END 
			&& ResearchDesires[ResearchDesires[c].prerequisite].bAcquired == false) continue;
		if(UnitsOwned[ResearchDesires[c].facility] == 0) continue;

		if(ResearchDesires[c].nPriority > bestpri)
		{
			bestpri = ResearchDesires[c].nPriority;
			*fail = bestpri;
			bestobj = indx;
		}
	}

	return bestobj;
}

//---------------------------------------------------------------------------//
//
void SPlayerAI::addPlanet(U32 planet, U32 system)
{
	if(m_PlanetsUnderMyControl >= M_MAX_PLANETS - 1) return;

	for(U32 c = 0; c < m_PlanetsUnderMyControl; c++)
	{
		if(m_OwnedPlanets[c].planet == planet) return;
	}

	m_OwnedPlanets[m_PlanetsUnderMyControl].planet = planet;
	m_OwnedPlanets[m_PlanetsUnderMyControl].system = system;
	m_OwnedPlanets[m_PlanetsUnderMyControl].bHasRefinery = false;
	m_PlanetsUnderMyControl++;
}
//---------------------------------------------------------------------------//
//
void SPlayerAI::clearPlanets(void)
{
	memset(m_OwnedPlanets, 0, M_MAX_PLANETS * sizeof(PlanetHolding));
	m_PlanetsUnderMyControl = 0;
}
//---------------------------------------------------------------------------//
//
bool SPlayerAI::IsPlanetOwned(U32 planet)
{
	for(U32 c = 0; c < m_PlanetsUnderMyControl; c++)
	{
		if(m_OwnedPlanets[c].planet == planet) return true;
	}
	return false;
}
//---------------------------------------------------------------------------//
//
int SPlayerAI::GetPlanetIndex(U32 planet)
{
	for(U32 c = 0; c < m_PlanetsUnderMyControl; c++)
	{
		if(m_OwnedPlanets[c].planet == planet) return c;
	}
	return -1;
}
//---------------------------------------------------------------------------//
//
void SPlayerAI::removePlanet(U32 planet)
{
	U32 total = m_PlanetsUnderMyControl;
	for(U32 c = 0; c < total; c++)
	{
		if(m_OwnedPlanets[c].planet == planet)
		{
			memcpy(&m_OwnedPlanets[c], &m_OwnedPlanets[c+1], ((total - c - 1) * sizeof(PlanetHolding)));
			break;
		}
	}

	m_PlanetsUnderMyControl--;
}
//---------------------------------------------------------------------------//
//
void SPlayerAI::doResearch (MPart & part)
{
	AI_TECH_TYPE possibleTech = CouldResearch(part);

	if(possibleTech != NOTECH)
	{
		int bestpri = 0;
		AI_TECH_TYPE bestTech = ChooseNextTech(&bestpri);
		S32 diff = ResearchDesires[bestTech].nPriority - ResearchDesires[possibleTech].nPriority;
		//conversion of diff to resource levels... ie research less desired techs only when 
		//resources are plentiful
		if((m_Gas < 300 || m_Metal < 300) && (diff > 200))
			return;
		
		//m_ComPts = MGlobals::GetCurrentTotalComPts(playerID);
		//m_ComPtsUsedUp = MGlobals::GetCurrentUsedComPts(playerID);

		U32 archeID = GetTechArcheID(possibleTech);
		CQASSERT(archeID);
		U32 fail = 0;
		if(CanIBuild(archeID, &fail))
		{
			if (archeID)
			{
				USR_PACKET<USRBUILD> packet;
				packet.cmd = USRBUILD::ADD;
				packet.dwArchetypeID = archeID;
				packet.objectID[0] = part->dwMissionID;
				packet.init(1);

				NETPACKET->Send(HOSTID,0,&packet);

				/* //for self-monitoring of research
				NETGRIDVECTOR temp;
				temp.zero();
				temp.systemID = 0;
				AddSpacialPoint(FAB_POINT, temp, part->dwMissionID, 0, 0, (int)possibleTech);
				*/
				m_bTechInvalid = true;
				m_WorkingTech = possibleTech;
			}
		}
		else
		{
			CQASSERT((possibleTech >= 0) && (possibleTech < AI_TECH_END));
			ResearchDesires[possibleTech].nPriority -= 15;
		}
	}
}
//---------------------------------------------------------------------------//
//
void SPlayerAI::doUpgrade (MPart & part)
{
	U32 pArcheID = 0;
	int fail = 0;
	M_OBJCLASS desiredplat = ChooseNextBuild(&fail, part->race, true, false);
	CQASSERT(desiredplat >= 0 && desiredplat < M_ENDOBJCLASS);

	/*  //   fix   don't upgrade collectors on mined out planets in a non-regen game
	if(desiredplat == M_GREATER_COLLECTOR)
	{
		if(m_bRegenResources == false)
		{
			
		}
	}
	*/

	if(BuildDesires[desiredplat].prerequisite == part->mObjClass && 
		BuildDesires[desiredplat].bMutation)
	{
		pArcheID = m_ArchetypeIDs[desiredplat];

		U32 fail = 0;
		if(pArcheID && CanIBuild(pArcheID, &fail))
		{
			//upgrade
			USR_PACKET<USRBUILD> packet;
			packet.cmd = USRBUILD::ADD;
			packet.dwArchetypeID = pArcheID;
			packet.objectID[0] = part->dwMissionID;
			packet.init(1);

			NETPACKET->Send(HOSTID,0,&packet);
		}
		else
			BuildDesires[desiredplat].nPriority -= 50;
	}

}
//---------------------------------------------------------------------------//
//
void SPlayerAI::doHQ (MPart & part)
{
	U32 archID = 0;
	U32 fabs = UnitsOwned[M_FABRICATOR] + UnitsOwned[M_WEAVER] + UnitsOwned[M_FORGER];

	bool smartfab = (!fabs || ((UnitsOwned[M_HARVEST] + UnitsOwned[M_SIPHON] + UnitsOwned[M_GALIOT]) > (fabs + 3)) ||
					(m_Metal > (1700 + ((-3 + ((S32)fabs)) * 200))));

	if (((!fabs) || (m_Age > 10)) && (((((S32)fabs) < DNA.nNumFabricators && smartfab) || (m_PlanetsUnderMyControl / 4 > fabs)) && ReadyForBuilding()))
	{
		if(part->mObjClass == M_HQ) archID = m_ArchetypeIDs[M_FABRICATOR];
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
//---------------------------------------------------------------------------//
//
void SPlayerAI::doRefinery (MPart & part)
{
	U32 archID = 0;
	U32 NumRefineries = 2 + UnitsOwned[M_REFINERY] + (UnitsOwned[M_HEAVYREFINERY] / 2) + 
						(UnitsOwned[M_SUPERHEAVYREFINERY] / 2) +
						UnitsOwned[M_COLLECTOR] + /*+ UnitsOwned[M_GREATER_COLLECTOR]*/
						UnitsOwned[M_OXIDATOR];

	if(NumRefineries > 10) NumRefineries = 10;
	
	SINGLE harvsOwned = (SINGLE)(UnitsOwned[M_HARVEST] + UnitsOwned[M_SIPHON] + UnitsOwned[M_GALIOT]);

	if (DNA.buildMask.bHarvest && /*(harvsOwned < (((SINGLE)NumRefineries) * DNA.fHarvestersPerRefinery)) &&*/
		(harvsOwned < DNA.nMaxHarvesters) && (harvsOwned < (m_PlanetsUnderMyControl * 4)))
	{
		switch(part->race)
		{
		case M_TERRAN:
			archID = m_ArchetypeIDs[M_HARVEST];
		break;
		case M_MANTIS:
			archID = m_ArchetypeIDs[M_SIPHON];
		break;
		case M_SOLARIAN:
			archID = m_ArchetypeIDs[M_GALIOT];
		break;
		}
	}
	//else if(there are lots of nuggets and i need ore perhaps build extra harvesters)  // fix 

	if (archID == 0) return;
	
	U32 fail = 0;
	if(!CanIBuild(archID, &fail))
	{
		if(fail > M_COMMANDPTS)
		{
			CQASSERT(((fail - M_COMMANDPTS) < M_ENDOBJCLASS) && ((fail - M_COMMANDPTS) >= 0));
			BuildDesires[M_COMMANDPTS + fail].nPriority += 125;
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
//--------------------------------------------------------------------------------------------------//
// fun
//--------------------------------------------------------------------------------------------------//
void SPlayerAI::doFabricator (MPart & part)
{
	//const char * pArchename = 0;
	U32 pArcheID = 0;
	U32 pHQThreeArcID = 0;
	U32 pBallisticsOneArcID = 0;
	U32 pAcademyTwoArcID = 0;
	U32 pHeavyIndFourArcID = 0;
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

	//may wish to repair a building rather than build a new one  fix  

	//the following makes this function 4 times slower, but hey, it determines some nice things -aeh
	int c;
	for(c = 0; c < 4; c++)
	{
		SlotSites[c].planet = 0;
		switch(c)
		{
		case 0:
			pBallisticsOneArcID = m_ArchetypeIDs[M_BALLISTICS];
			genArcID = pBallisticsOneArcID;
		break;
		case 1:
			pAcademyTwoArcID = m_ArchetypeIDs[M_ACADEMY];
			genArcID = pAcademyTwoArcID;
		break;
		case 2:
			pHQThreeArcID = m_ArchetypeIDs[M_HQ];
			genArcID = pHQThreeArcID;
		break;
		case 3:
			pHeavyIndFourArcID = m_ArchetypeIDs[M_HEAVYIND];
			genArcID = pHeavyIndFourArcID;
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
	else if (!IsPlanetOwned(SlotSites[2].planet)) 
	{
		U32 oc = M_REFINERY;
		if(part->race == M_MANTIS) oc = M_COLLECTOR;
		if(part->race == M_SOLARIAN) oc = M_OXIDATOR;
		BuildDesires[oc].nPriority += 200;
	}

	U32 newSystem = SlotSites[2].pos.systemID;
	if((newSystem > 0) && (newSystem < MAX_SYSTEMS) && (!m_bSystemSupplied[newSystem-1]))
	{
		BuildDesires[M_JUMPPLAT].nPriority += 155;

		U32 oc = M_HQ;
		if(part->race == M_MANTIS) oc = M_COCOON;
		if(part->race == M_SOLARIAN) oc = M_ACROPOLIS;
		BuildDesires[oc].nPriority += 205;
	}
	
	c = 0;
	while(!bDone)
	{
		int bestpri = 0;
		ChosenPlat = ChooseNextBuild(&bestpri, part->race, true);    //true for platforms
		CQASSERT(ChosenPlat >= 0 && ChosenPlat < M_ENDOBJCLASS);
		pArcheID = m_ArchetypeIDs[ChosenPlat];

		if(ChosenPlat == M_JUMPPLAT)
		{
			if(part->race == M_TERRAN) pArcheID = ARCHLIST->GetArchetypeDataID("PLATJUMP!!T_JumpPlat");
			if(part->race == M_MANTIS) pArcheID = ARCHLIST->GetArchetypeDataID("PLATJUMP!!M_JumpPlat");
			if(part->race == M_SOLARIAN) pArcheID = ARCHLIST->GetArchetypeDataID("PLATJUMP!!S_JumpPlat");
		}

		U32 fail = 0;
		if(CanIBuild(pArcheID, &fail)) 
		{
			bDone = true;

			S32 index = BuildDesires[ChosenPlat].nNumSlots - 1;

			pArchetype = ARCHLIST->LoadArchetype(pArcheID);

			CQASSERT(index < 4);
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
			c++;
			if(ChosenPlat == LastPlat) c++;

			if(ChosenPlat >= 0 && ChosenPlat < M_ENDOBJCLASS) 
			{
				if(BuildDesires[ChosenPlat].nPriority > 100)
					BuildDesires[ChosenPlat].nPriority -= 45;
				else
					BuildDesires[ChosenPlat].nPriority -= 4;

				S32 failplat = ((S32)fail) - M_COMMANDPTS;
				if((failplat > 0) && (failplat < M_ENDOBJCLASS))
				{
					BuildDesires[failplat].nPriority += (BuildDesires[ChosenPlat].nPriority / 2);
				}
			}

			LastPlat = ChosenPlat;

			//increase priority of prerequisite (lack of resource) that caused can I build to fail
			//(or harvesting of needed resource)  fix 

			if(c > DNA.nBuildPatience)
			{
				bDone = true;

				//more things need to be true for the following
				//like ReadyForBuild() maybe should return false  fix  
				//doRepair(part);
				return;
			}
		}
	}

	if (pArcheID && (pArchetype = ARCHLIST->LoadArchetype(pArcheID)) != 0)
	{
		//BASE_PLATFORM_DATA * data = (BASE_PLATFORM_DATA *)ARCHLIST->GetArchetypeData(pArchetype);
		//S32 numSlots = data->slotsNeeded - 1;
		S32 numSlots = BuildDesires[ChosenPlat].nNumSlots - 1;

		CQASSERT(numSlots <= 3);
	
		if(numSlots >= 0)
		{
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
				
					addPlanet(siteID,1); //1 is wrong  fix

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
			if((ChosenPlat == M_PORTAL) && (rand() & 7))
				resultLoc = ChooseAssignmentSite(DEFEND);
			else
				resultLoc = ChooseFreeBuildSite(pArchetype, dwFabID, systemID, part.obj->GetPosition(), ((/*  fix  need real portal using code*/ChosenPlat == M_PORTAL) || (strategy == SOLARIAN_FORGERS && MGlobals::IsGunPlat(ChosenPlat) && (m_Age > 100))));
			

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
U32 SPlayerAI::ChooseBuildSite(M_OBJCLASS plattype, PARCHETYPE pArchetype, U32 dwFabID, U32 systemID, Vector position, const Vector* translation, S32* slot, DOUBLE* dist, NETGRIDVECTOR* loc, bool bResourcesMatter)
{
	//
	// find closest planet with an opening
	//
	IBaseObject * obj = OBJLIST->GetTargetList();     // mission objects
	S32 bestSlot=-1;
	DOUBLE bestPoints = 10e8;
	//DOUBLE bestDistance = 10e8;
	DOUBLE points = 10e8;
	NETGRIDVECTOR pos;
	NETGRIDVECTOR bestLoc;

	U32 bestPlanet = 0;
	OBJPTR<IPlanet> planet;
	OBJPTR<IJumpGate> jump;
	bool bBestIsVisible = DNA.buildMask.bVisibilityRules && ((rand() & 15) != 0);

	bestLoc.zero(); bestLoc.systemID = 0;
	pos.zero(); bestLoc.systemID = 0;

	DOUBLE div = 1024;
	DOUBLE dtonext = DISTTONEXTSYS;
	if((strategy == TERRAN_FORWARD_BUILD) || (strategy == MANTIS_FORWARD_BUILD) || 
		(strategy == SOLARIAN_FORWARD_BUILD)) 
	{
		div = 2048;
		dtonext = DISTTONEXTSYS - 5;
	}
					

	while (obj)
	{
		bool bIsVisible = false;
		U32 tempSlot = 0;
		points = 10e8;
		if(plattype == M_JUMPPLAT)
		{
			//for(int c = 0; c < numplanetsowned; c++) m_OwnedPlanet[c].system  //  fix  make sure of connection
			//supposedly there's a SECTOR->  function for which sector a jumpgate goes to
			if(obj->objClass == OC_JUMPGATE)
			{
				bool canbuild = false;
				if (obj->QueryInterface(IJumpGateID, jump)!=0)
				{
					canbuild = jump->CanIBuild(playerID);
				}
				
				bIsVisible = obj->IsVisibleToPlayer(playerID);
				DOUBLE distance;
				if(!canbuild) distance = 10e8;
				else
				{
					if (obj->GetSystemID() == systemID)
						distance = 5;  //quick hack
					else
						distance = (10e5 / 512) * getNumJumps(systemID, obj->GetSystemID(), playerID);
				}
				
				points = distance;
				pos.init(obj->GetGridPosition(),obj->GetSystemID());
			}
		}
		else if (obj->QueryInterface(IPlanetID, planet)!=0)
		{
			//
			// object is a planet, find the closest open slot
			//
			bIsVisible = obj->IsVisibleToPlayer(playerID);
			const U32 objSystemID = obj->GetSystemID();
			if (((tempSlot = planet->FindBestSlot(pArchetype, objSystemID==systemID ? translation : NULL)) != 0) && 
				(SiteTaken(obj->GetPartID(),tempSlot) == 0))
			{
				DOUBLE distance;				
				if (objSystemID == systemID)
					distance = ((position-planet->GetSlotTransform(tempSlot).translation).magnitude()) / div;
				else
					distance = (dtonext * getNumJumps(systemID, objSystemID, playerID));

				//put in something to build HQs in defensive position away from wormhole?  fix  
				//if(m_bKillHQ

				pos.init(obj->GetGridPosition(),obj->GetSystemID());
				U32 danger = GetDanger(pos);
				points = distance * ((float)(danger * DNA.nCautiousness + 1.0));

				if((SECTOR->SystemInSupply(objSystemID, playerID) == false) && 
					(MGlobals::IsRefinery(plattype) == false))
				{
					points *= 100.0;
				}

				if(bResourcesMatter)
				{
					BuildSite tempSite;
					tempSite.planet = planet.Ptr()->GetPartID();
					tempSite.slot = tempSlot;
					tempSite.dist = points;
					tempSite.pos = pos;
					tempSite.plattype = plattype;

					points += ShouldIBuild(planet, pArchetype, tempSite);
				}
			}
		} // end obj->GetSystemID() ...

		//no slot figuring here, that's done in the calling function

		//at a certain point, when the only build sites left are very dangerous,
		//fabricator production should be shrinked.  fix  
		//or fabricators should be set to defend the HQ, that is hang around it
		//and if it gets damaged repair it.

		if (points < bestPoints && (bBestIsVisible==false || bIsVisible))
		{
			bestPoints = points;
			bestLoc = pos;
			//bBestIsVisible = bIsVisible;

			if(plattype == M_JUMPPLAT)
			{
				bestSlot = 0;
				bestPlanet = obj->GetPartID();
			}
			else
			{
				bestSlot = tempSlot;
				bestPlanet = planet.Ptr()->GetPartID();
			}
		}
		
		obj = obj->nextTarget;
	} // end while()

	if(bestPlanet)
	{
		//U32 mID = bestPlanet.ptr->GetPartID();
		*slot = bestSlot;
		//*dist = bestDistance;
		*dist = bestPoints;
		*loc = bestLoc;

		//return mID;
	}
	//else return 0;

	return bestPlanet;
}

//---------------------------------------------------------------------------//
//
NETGRIDVECTOR SPlayerAI::ChooseFreeBuildSite(PARCHETYPE pArchetype, U32 dwFabID, U32 systemID, Vector position, bool wormgen)
{	
	DOUBLE bestDistance = 10e8;
	Vector bestLoc;
	U32 bestSys = 0;
	U32 bestGate = 0;
	const U32 allyMask = MGlobals::GetAllyMask(playerID);
	NETGRIDVECTOR hqloc = m_HQLocation;
	U32 enemysys = 0, enemy = 0;
	S32 g = 0, c = 0;
	while((enemysys==0) || (allyMask & (1 << g)))  
	{
		g = rand() % MAX_PLAYERS;
		enemysys = Enemies[g].HQLocation.systemID;
		enemy = g;
		c++;
		if(c > 20)
		{
			enemysys = 0;
			break;
		}
	}

	if(wormgen && enemysys)
		hqloc = Enemies[enemy].HQLocation;

	IBaseObject * obj = OBJLIST->GetTargetList();     // mission objects
	MPart part;
	U32 objSystemID = 0;

	while (obj)
	{
		if ((part = obj).isValid())
		{
			if(part->mObjClass == M_JUMPGATE)
			{
				//weigh this so that it sometimes chooses a farther jumpgate fix 
				//in the case that it chooses a different system, replace HQ loc with jumpgate in loc
				objSystemID = part->systemID;

				if (objSystemID == systemID)
				{
					DOUBLE distance;
					Vector diff = position.subtract(obj->GetPosition());
					distance = diff.fast_magnitude();
					if (distance < bestDistance)
					{
						bestDistance = distance;
						bestGate = part->dwMissionID;
						bestLoc = obj->GetPosition();
						bestSys = obj->GetSystemID();
					}
				}
				else
				{
					DOUBLE distance = 10e5 * getNumJumps(systemID, objSystemID, playerID);
					if (distance < bestDistance)
					{
						bestDistance = bestDistance;
						bestGate = part->dwMissionID;
						bestLoc = obj->GetPosition();
						bestSys = obj->GetSystemID();
					}
				}
			}
		}

		obj = obj->nextTarget;
	} // end while()

	NETGRIDVECTOR final;
	final.zero();
	final.systemID = 0;
	GRIDVECTOR temp;

	if(bestGate)
	{	
		DOUBLE diffx, diffy;
		U32 c = 0;
		bool bLegal = false;

		while(!bLegal)
		{
			if(m_bKillHQ)
				final = m_HQLocation;
			else
				final.init(bestLoc, bestSys);

			diffx = m_HQLocation.getX();
			diffy = m_HQLocation.getY();
			diffx -= final.getX();
			diffy -= final.getY();
			diffx *= 1.0 / ((DOUBLE)((rand() & 3) + 2));
			diffy *= 1.0 / ((DOUBLE)((rand() & 3) + 2));

			temp.init((DOUBLE)(final.getIntX() + (rand() & 7) - 4 + ((int)diffx)), (DOUBLE)(final.getIntY() + (rand() & 7) - 4 + ((int)diffy)));
			final.init(temp, bestSys);

			COMPTR<ITerrainMap> map;
			SECTOR->GetTerrainMap(systemID, map);
			bLegal = map->IsGridValid(temp);  //check if the grid is unoccupied, and if it's the SAME as the jumpgate fix 
			if(c++ > 25)
			{
				final.zero();
				bLegal = true;
			}

		}
	}
	
	return final;
}
//---------------------------------------------------------------------------//
//
NETGRIDVECTOR SPlayerAI::ChooseAssignmentSite(ASSIGNMENTTYPE aType)
{
	ASSIGNMENT * node = pAssignments;
	//ASSIGNMENT * newNode;
	//ASSIGNMENT * prev = 0;
	U32 bestID = 0;
	S32 bestmilPower = 700;

	while (node)
	{
		if(node->type == aType)
		{
			S32 milPower = node->militaryPower;
			if(milPower > bestmilPower)
			{
				bestID = node->set.objectIDs[0];
				bestmilPower = milPower;
			}
		}

		node = node->pNext;
	}

	NETGRIDVECTOR loc;
	loc.zero();
	loc.systemID = 0;
	
	if(bestID)
	{
		IBaseObject * obj = OBJLIST->FindObject(bestID);
		if(obj)
		{
			loc.init(obj->GetPosition(),obj->GetSystemID());		
		}
	}

	return loc;
}
//---------------------------------------------------------------------------//
//
void SPlayerAI::doMinelayer (MPart & part)
{
	if(rand() % 2)
	{
		PARCHETYPE pArchetype = 0;
		NETGRIDVECTOR resultLoc, fail;
		fail.zero();
		resultLoc.zero();
		fail.systemID = 0;
		resultLoc.systemID = 0;

		resultLoc = ChooseFreeBuildSite(pArchetype, part->dwMissionID, part->systemID, part.obj->GetPosition());

		if(resultLoc != fail)
		{
			USR_PACKET<USRAOEATTACK> packet;
					
			packet.position = resultLoc;
			packet.userBits = 0;
			packet.objectID[0] = part->dwMissionID;
			packet.init(1);

			NETPACKET->Send(HOSTID, 0, &packet);
		}
	}
	else
	{
		//go on a mining run  fix  
	}
	

	//send lay-mine packet

	//doScouting code?
}
//---------------------------------------------------------------------------//
//
void SPlayerAI::doTroopship (MPart & part)
{
	IBaseObject * obj = OBJLIST->GetTargetList();     // mission objects
	DOUBLE bestPoints = 50.0;
	IBaseObject * bestPlatform=0;
	U32 systemID = part->systemID;
	GRIDVECTOR position = part.obj->GetGridPosition();
	bool bBestIsVisible=DNA.buildMask.bVisibilityRules && ((rand() & 15) != 0);
	MPart other;
	const U32 allyMask = MGlobals::GetAllyMask(playerID);
	DOUBLE points = 0.0;
	
	while (obj)
	{
		if (obj->objMapNode && (obj->objMapNode->flags & OM_TARGETABLE) && (other=obj).isValid())
		{
			U32 hisPlayerID = other->playerID;
			M_OBJCLASS otherMobj = other->mObjClass;
			if (obj->objMapNode->flags & OM_MIMIC)
			{
				hisPlayerID = OBJMAP->GetApparentPlayerID(obj->objMapNode,allyMask);
			}

			if ((allyMask & (1 << (hisPlayerID-1)))==0 &&
				obj->IsTargetableByPlayer(playerID) && 
				((other->systemID & HYPER_SYSTEM_MASK)==0) && 
				(other->systemID) && 
				other->bReady && 
				((m_Terminate == 0) || (m_Terminate == hisPlayerID)) &&
				MGlobals::CanTroopship(playerID, part->dwMissionID, other->dwMissionID) && 
				MGlobals::IsPlatform(otherMobj))
			{
				
				const U32 objSystemID = other->systemID;
				bool bIsVisible = obj->IsVisibleToPlayer(playerID);

				points = 300.0;
				
				//find the distance		
				if (objSystemID == systemID)
					points = (position-obj->GetGridPosition()) * 5;
				else
					points = (DISTTONEXTSYS * (getNumJumps(systemID, objSystemID, playerID)) * 5);
				
				points -= GetPowerRating(other);
				//some randomness
				points -= ((DOUBLE)(rand() % 100));
				
				/*
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
				*/	
				
				//if(other.obj->objClass == OC_PLATFORM) points /= 1.2;
				
				//(MGlobals::IsShipyard(otherMobj)) points *= 1.2;
				//(MGlobals::IsRefinery(otherMobj)) points /= 1.2;
				
				//  fix  danger/cautiousness figuring?
				
				/*
				if(MGlobals::IsHQ(other->mObjClass))
				{
				if(m_bKillHQ) points *= 0.4;
				else points *= 0.8;
				}
				*/
				
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
				
				if(MGlobals::IsShipyard(otherMobj)) points *= 0.7;
				
				if(points < bestPoints && (bBestIsVisible==false || bIsVisible))
				{
					bestPoints = points;
					bestPlatform = obj;
					bBestIsVisible = bIsVisible;
				}
			} 
		}
		obj = obj->nextTarget;
	} // end while()

	if(bestPlatform)
	{
		//USR_PACKET<USRATTACK> packet;
		USR_PACKET<USRCAPTURE> packet;
		
		packet.targetID = bestPlatform->GetPartID();
//		packet.bUserGenerated = 1;
		packet.objectID[0] = part->dwMissionID;
		packet.userBits = 0;  //0 means cancel current op when this one arrives.
		packet.init(1);
			
		NETPACKET->Send(HOSTID, 0, &packet);
	}
}
//---------------------------------------------------------------------------//
//
void SPlayerAI::doShipyard (MPart & part)
{
	U32 archID = 0;
	bool bDone = false;

	U32 c = 0;
	U32 ChosenShip = 0;
	U32 LastShip = 0;
	S32 numShipyards = 1 + UnitsOwned[M_LIGHTIND] + UnitsOwned[M_HEAVYIND] + UnitsOwned[M_THRIPID] + UnitsOwned[M_PAVILION] + UnitsOwned[M_GREATERPAVILION];

	while(!bDone)
	{
		int shipfail = 0;
		ChosenShip = ChooseNextShip(&shipfail, part->race, part->mObjClass);
		if(!ChosenShip) break;

		archID = m_ArchetypeIDs[ChosenShip];

		//race-specific modifiers
		/*
		if(ChosenPlat == M_JUMPPLAT)
		{
			if(part->race == M_TERRAN) pArcheID = ARCHLIST->GetArchetypeDataID("PLATJUMP!!T_JumpPlat");
			if(part->race == M_MANTIS) pArcheID = ARCHLIST->GetArchetypeDataID("PLATJUMP!!M_JumpPlat");
			if(part->race == M_SOLARIAN) pArcheID = ARCHLIST->GetArchetypeDataID("PLATJUMP!!S_JumpPlat");
		}
		*/

		U32 fail = 0;
		if(!CanIBuild(archID, &fail))
		{
			c++;
			if(ChosenShip == LastShip) c++;

			if(ChosenShip < M_ENDOBJCLASS) 
			{
				ShipDesires[ChosenShip].nPriority -= ((75 / numShipyards) + (rand() & 31));
			}
			LastShip = ChosenShip;

			//increase priority of prerequisite (lack of resource) that caused can I build to fail
			//(or harvesting of needed resource)  needs resource deplete handling too  fix  
			if(fail > M_COMMANDPTS)
			{
				CQASSERT((fail - M_COMMANDPTS >= 0) && (fail - M_COMMANDPTS < M_ENDOBJCLASS));
				BuildDesires[fail - M_COMMANDPTS].nPriority += 25;
			}
			else if(fail == M_CREW)
			{
				BuildDesires[M_CITADEL].nPriority += 20;
				BuildDesires[M_BUNKER].nPriority += 20;
				BuildDesires[M_WARLORDTRAINING].nPriority += 20;
				BuildDesires[M_ACADEMY].nPriority += 20;
				BuildDesires[M_OUTPOST].nPriority += 20;
			}

			if((S32)c > DNA.nShipyardPatience)
			{
				bDone = true;
				return;
			}
		}
		else bDone = true;
	}	

	if (archID)
	{
		USR_PACKET<USRBUILD> packet;
		packet.cmd = USRBUILD::ADDIFEMPTY;
		packet.dwArchetypeID = archID;
		packet.objectID[0] = part.obj->GetPartID();
		packet.init(1);
		NETPACKET->Send(HOSTID,0,&packet);

		//  fix  attack wave size should be attack wave power, size should only matter for keeping it under 20 ships
		//yes, this comment is completely out of place
	}
}
//---------------------------------------------------------------------------//
// unit is out of supplies
//
bool SPlayerAI::doResupply (MPart & part)
{
	bool result = false;
	U32 fail = 0;
	IBaseObject * supship;
	if(MGlobals::IsSupplyShip(part->mObjClass))
	{
		supship = findClosestPlatform(part, &MGlobals::IsHQ);
		if(supship)
		{
			//maybe set the bUserGenerated bit?
			USR_PACKET<USRSHIPREPAIR> packet;
			packet.targetID = supship->GetPartID();
			packet.userBits = 0;
			packet.objectID[0] = part->dwMissionID;
			packet.init(1);
			NETPACKET->Send(HOSTID,0,&packet);

			fail = 0xffffffff;
		}
	}
	else
		supship = findNearbySupplyShip(part, &fail);
	
	if(fail != 0xffffffff)
	{
		if(supship)
		{
			USR_PACKET<USRMOVE> packet;

			NETGRIDVECTOR targloc;
			targloc.init(supship->GetPosition(), supship->GetSystemID());
			packet.position = targloc;
			packet.userBits = 0;
			packet.objectID[0] = part->dwMissionID;
			packet.init(1);
			NETPACKET->Send(HOSTID,0,&packet);

			result = true;
		}
		else
		{
			/*
			ASSIGNMENT *node = findAssignment(RESUPPLY);
			if(node)
			{
				//MISSION_DATA::M_CAPS caps;
				//memset(&caps, 0, sizeof(caps));
				//caps.supplyOk = true;
			*/
				IBaseObject * plat = findClosestPlatform(part, &MGlobals::IsHQ);

				if (plat)
				{
					//maybe set the bUserGenerated bit?
					USR_PACKET<USRSHIPREPAIR> packet;
					packet.targetID = plat->GetPartID();
					packet.userBits = 0;
					packet.objectID[0] = part->dwMissionID;
					packet.init(1);
					NETPACKET->Send(HOSTID,0,&packet);

					result = true;
					

					/*
					USR_PACKET<USRMOVE> packet;

					NETGRIDVECTOR targloc;
					targloc.init(plat->GetPosition(), plat->GetSystemID());
					packet.position = targloc;
					packet.userBits = 0;
					packet.objectID[0] = part->dwMissionID;
					packet.init(1);
					NETPACKET->Send(HOSTID,0,&packet);

					result = true;
					*/
				}
				else
				{
					//no hq plats!  freak out!  fix  
				}

			//}
		}
	}
	else
		result = true;

	return result;
}
//---------------------------------------------------------------------------//
// unit is hurt
//
bool SPlayerAI::doShipRepair (MPart & part)
{
	bool result = false;
	
	IBaseObject * plat = findClosestPlatform(part, &MGlobals::IsRepairPlat);

	if (plat)
	{
		//maybe set the bUserGenerated bit?
		USR_PACKET<USRSHIPREPAIR> packet;
		packet.targetID = plat->GetPartID();
		packet.userBits = 0;
		packet.objectID[0] = part->dwMissionID;
		packet.init(1);
		NETPACKET->Send(HOSTID,0,&packet);

		result = true;
	}
	else
	{
					//no repair plats!  freak out! //increase repair plat pri?
	}

	return result;
}
//---------------------------------------------------------------------------//
// idle fabricator, you go repair something
//
void SPlayerAI::doRepair (MPart & part)
{
	IBaseObject * plat = findClosestPlatform(part, &MGlobals::IsHQ, true);

	if (plat)
	{
		removeFromAssignments(part);

		//maybe set the bUserGenerated bit?
		USR_PACKET<USRFABREPAIR> packet;
		packet.targetID = plat->GetPartID();
		packet.userBits = 0;
		packet.objectID[0] = part->dwMissionID;
		packet.init(1);
		NETPACKET->Send(HOSTID,0,&packet);
	}
	else
	{
		//check for repair sites other than HQs in a killHQ game   fix   
		IBaseObject * plat = findClosestPlatform(part, &MGlobals::IsPlatform, true);

		if(plat)
		{
			USR_PACKET<USRFABREPAIR> packet;
			packet.targetID = plat->GetPartID();
			packet.userBits = 0;
			packet.objectID[0] = part->dwMissionID;
			packet.init(1);
			NETPACKET->Send(HOSTID,0,&packet);
		}
	}
	
	if(!plat)
	{
		m_RepairSite = 0;
	}
}
//---------------------------------------------------------------------------//
//
bool SPlayerAI::doScouting (MPart & part)
{
	U32 result;
	NETGRIDVECTOR positions[SCOUT_HOPS];

	if(m_bWorldSeen && m_nNumScoutRoutes && (m_nNumScoutRoutes > 2))
	{
		result = repeatScoutingRoute(part, positions);
	}
	else
	{
		bool flip = (((part->dwMissionID / 16) % 2) == 0);
		result = findScoutingRoute(part, positions, flip);
		if(result == 1)
		{
			AddOfflimits(positions[0]);
		}
		else if(result && m_nNumScoutRoutes < MAX_SCOUT_ROUTES && (!m_bWorldSeen))
		{
			bool bKnownRoute = false;
			for(int c = m_nNumScoutRoutes - 1; c >= 0; c--)
			{
				if((m_ScoutRoutes[c].numhops == result) && 
					(m_ScoutRoutes[c].positions[0] == positions[0]) &&
					(m_ScoutRoutes[c].positions[1] == positions[1]) && 
					(m_ScoutRoutes[c].positions[2] == positions[2]))
					bKnownRoute = true;
			}
			if(!bKnownRoute)
			{
				m_ScoutRoutes[m_nNumScoutRoutes].numhops = result;
				memcpy(m_ScoutRoutes[m_nNumScoutRoutes].positions, positions, (sizeof(NETGRIDVECTOR) * SCOUT_HOPS));
				m_nNumScoutRoutes++;
			}
		}
	}

	for (U32 i = 0; i < result; i++)
	{
		// schedule a MOVE command for each position
		USR_PACKET<USRMOVE> packet;

		packet.position = positions[i];
		if(i)
			packet.userBits = 1;	// queue this command
		else
			packet.userBits = 0;
		packet.objectID[0] = part->dwMissionID;
		packet.init(1);
		NETPACKET->Send(HOSTID,0,&packet);
	}

	/*
	if(result == 0)
	{
		DNA.uNumScouts = 1;

		if(m_nRace == M_MANTIS)
		{
			//go to dissection chamber instead of the following  fix
			USR_PACKET<USRKILLUNIT> packet;
			packet.objectID[0] = part->dwMissionID;
			packet.init(1);
			CQASSERT(packet.objectID[0]);
			NETPACKET->Send(HOSTID,0,&packet);
		}
		else
		{
			USR_PACKET<USRKILLUNIT> packet;
			packet.objectID[0] = part->dwMissionID;
			packet.init(1);
			CQASSERT(packet.objectID[0]);
			NETPACKET->Send(HOSTID,0,&packet);
		}
	}
	*/

	return (result!=0);
}
//---------------------------------------------------------------------------//
//
void SPlayerAI::doSupplyShip (MPart & part)
{
	bool bSentPacket = false;
	U32 numGunplats = getNumGunplats();

	// escort biggest ship we have
	//
	// find biggest ship in the system
	//
	// go to a battle..?
	//

	U32 systemID = part->systemID;
	IBaseObject * bestShip;

	if((rand() & 15) > ((S32)numGunplats))
	{
		bestShip = findSupplyEscortee(systemID,part.obj->GetGridPosition());
		if(bestShip)
		{
			MPart ship = bestShip;
			if (ship.isValid())
			{
				USR_PACKET<USRESCORT> packet;

				packet.objectID[0] = part->dwMissionID;
				packet.userBits = 0;
				packet.targetID = ship->dwMissionID;
				packet.init(1);
				NETPACKET->Send(HOSTID, 0, &packet);

				bSentPacket = true;
			}
		}
	}
	
	if(!bSentPacket)
	{
		bestShip = findGunplat(systemID,part.obj->GetGridPosition());
		if(!bestShip)
		{
			bestShip = findSupplyEscortee(systemID,part.obj->GetGridPosition());
			if(bestShip)
			{
				MPart ship = bestShip;
				if (ship.isValid())
				{
					USR_PACKET<USRESCORT> packet;

					packet.objectID[0] = part->dwMissionID;
					packet.userBits = 0;
					packet.targetID = ship->dwMissionID;
					packet.init(1);
					NETPACKET->Send(HOSTID, 0, &packet);

					bSentPacket = true;
				}
			}
		}
		else
		{
			MPart ship = bestShip;
			if (ship.isValid())
			{
				USR_PACKET<USRMOVE> packet;

				NETGRIDVECTOR targloc;
				targloc.init(bestShip->GetPosition(), ship->systemID);
				packet.position = targloc;
				packet.userBits = 0;
				packet.objectID[0] = part->dwMissionID;
				packet.init(1);
				NETPACKET->Send(HOSTID,0,&packet);
			
				bSentPacket = true;
			}
		}
	}
}
//---------------------------------------------------------------------------//
//
IBaseObject * SPlayerAI::findSupplyEscortee(U32 systemID, GRIDVECTOR position)
{
	IBaseObject * obj = OBJLIST->GetTargetList();     // mission objects
	IBaseObject * bestShip = 0;
	MPart other;

	SINGLE points = 0;
	SINGLE bestPoints = 10e8;

	while (obj)
	{
		if ((other = obj).isValid() && (other->systemID <= MAX_SYSTEMS) && 
			/*systemID == other->systemID&&*/other->playerID==playerID && 
			MGlobals::IsGunboat(other->mObjClass) && 
			other->bReady && 
			(other->supplies < other->supplyPointsMax))
		{
			ASSIGNMENT * assign = findAssignment(other);
			if (assign == 0 || assign->type == DEFEND || assign->type == ATTACK)  //maybe check here if "other" needs supplies
			{
				if (other->systemID == systemID)
					points = position - obj->GetGridPosition();
				else
					points = DISTTONEXTSYS * getNumJumps(systemID, other->systemID, playerID);
			
				if(points > 2.4) //  fix  needs to be supply range
				{
					if(other->supplies)
					{
						SINGLE divisor = ((((SINGLE)other->supplies) / ((SINGLE)other->supplyPointsMax)) * 10.0);
						if(divisor < 0.1) divisor = 0.1;
						points *= divisor;
					}

					if(assign)
					{
						points /= 2.0;
						if(assign->type == ATTACK) points /= 2.0;
						S32 gps  = DNA.nGunboatsPerSupplyShip;
						if(!gps) gps = 30;
						S32 calc = (assign->supships + 1 - (assign->set.numObjects / gps));
						if(calc < 1) calc = 1;
						points *= (DOUBLE)calc;
					}

					if (points < bestPoints)
					{
						bestPoints = points;
						bestShip = obj;
					}
				}
			}
		} 
		
		obj = obj->nextTarget;
	} // end while()

	return bestShip;
}
//---------------------------------------------------------------------------//
//
IBaseObject * SPlayerAI::findGunplat(U32 systemID, GRIDVECTOR position)
{
	IBaseObject * obj = OBJLIST->GetTargetList();     // mission objects
	IBaseObject * bestShip = 0;
	MPart other;

	SINGLE points = 0;
	SINGLE bestPoints = 10e8;

	while (obj)
	{
		if ((other = obj).isValid() && /*other->systemID==systemID &&*/ 
			(MGlobals::IsGunPlat(other->mObjClass)) && 
			(other->playerID==playerID) && 
			other->bReady && 
			other->supplyPointsMax && 
			(other->supplies < other->supplyPointsMax))
		{
			if (other->systemID == systemID)
				points = position - obj->GetGridPosition();
			else
				points = DISTTONEXTSYS * getNumJumps(systemID, other->systemID, playerID);

			if(points > 2.4) //  fix  needs to be supply range  no biggie
			{				
				SINGLE divisor = ((((SINGLE)other->supplies) / ((SINGLE)other->supplyPointsMax)) * 10.0);
				if(divisor < 0.1) divisor = 0.1;
				points *= divisor;
					
				if (points < bestPoints)
				{
					bestPoints = points;
					bestShip = obj;
				}
			}
		} 
		
		obj = obj->nextTarget;
	} // end while()

	return bestShip;
}
//---------------------------------------------------------------------------//
//
void SPlayerAI::doNavalAcademy (MPart & part)
{
	const char * pArchename = 0;
	U32 order[6];
	bool bUsed = false;

	U32 c;
	for(c = 0; c < 6; c++) order[c] = 99;
	for(c = 0; c < 6; c++)
	{
		U32 r = 99;
		do
		{
			bUsed = false;
			r = rand() % 6;

			for(U32 x = 0; x < 6; x++) if(order[x] == r) bUsed = true;
		} while(bUsed);
		
		order[c] = r;
	}

	bUsed = true;
	c = 0;
	U32 aID = 0;
	while(bUsed)
	{
		if(m_nRace == M_TERRAN)
		{
			switch(order[c])
			{
			case 0:	pArchename = "ADMIRALRES!!T_Benson";					break;
			case 1:	pArchename = "ADMIRALRES!!T_Halsey";					break;
			case 2:	pArchename = "ADMIRALRES!!T_Hawkes";					break;
			case 3:	pArchename = "ADMIRALRES!!T_Smirnoff";					break;
			case 4:	pArchename = "ADMIRALRES!!T_Steele";					break;
			case 5:	pArchename = "ADMIRALRES!!T_Takei";						break;
			}
		}
		else if(m_nRace == M_MANTIS)
		{
			switch(order[c])
			{
			case 0:	pArchename = "ADMIRALRES!!M_Azkar";						break;
			case 1:	pArchename = "ADMIRALRES!!M_KerTak";					break;
			case 2:	pArchename = "ADMIRALRES!!M_Malkor";					break;
			case 3:	pArchename = "ADMIRALRES!!M_Mordella";					break;
			case 4:	pArchename = "ADMIRALRES!!M_Thripid";					break;
			case 5:	pArchename = "ADMIRALRES!!M_VerLak";					break;
			}
		}
		else if(m_nRace == M_SOLARIAN)
		{
			switch(order[c])
			{
			case 0:	pArchename = "ADMIRALRES!!S_Blanus";					break;
			case 1:	pArchename = "ADMIRALRES!!S_Elan";						break;
			case 2:	pArchename = "ADMIRALRES!!S_Joule";						break;
			case 3:	pArchename = "ADMIRALRES!!S_Natus";						break;
			case 4:	pArchename = "ADMIRALRES!!S_Procyo";					break;
			case 5:	pArchename = "ADMIRALRES!!S_Vivac";						break;
			}
		}

		aID = ARCHLIST->GetArchetypeDataID(pArchename);
		CQASSERT(aID != 0);

		//have I made this admiral already?
		U32 fail = 0;
		if(CanIBuild(aID, &fail)) bUsed = false;
		else bUsed = true;
	
		c++;
		if(c >= 6)
		{
			return;
		}
	}

	if (aID)
	{
		USR_PACKET<USRBUILD> packet;
		packet.cmd = USRBUILD::ADD;
		packet.dwArchetypeID = aID;
		packet.objectID[0] = part->dwMissionID;
		packet.init(1);

		NETPACKET->Send(HOSTID,0,&packet);

		m_AdmiralsOwned++;
	}
}
//---------------------------------------------------------------------------//
//
void SPlayerAI::doFlagShip (MPart & part)
{
	// attach to biggest ship we have
	if (part.obj->objMapNode != 0)		// if not already attached  bUntouchable
	{
		//
		// find biggest ship in the system
		//
		IBaseObject * obj = OBJLIST->GetTargetList();     // mission objects
		MPart bestShip;
		ASSIGNMENT *bestAss = NULL;
		MPart other;

		U32 fID = part->fleetID;
		/*
		if((fID != 0) && (fID != part->dwMissionID))
		{
			
		}
		*/
		bool bNone = true;

		while (obj)
		{
			if ((other = obj).isValid() && /*other->systemID==part->systemID &&*/ other->playerID==playerID && 
				MGlobals::IsGunboat(other->mObjClass) && 
				(other->admiralID == 0) &&
				(((other->fleetID == fID) && (fID == part->dwMissionID)) || (other->fleetID == 0)))
			{
				ASSIGNMENT * assign = findAssignment(other);
				if (assign && (assign->type == DEFEND || (assign->type == ATTACK && bNone)) && 
					(assign->bHasAdmiral == false))
				{
					bNone = false;
					if (bestShip.isValid())
					{
						if (other->mObjClass > bestShip->mObjClass)
						{
							bestShip = other;
							bestAss = assign;
						}
					}
					else
					{
						bestShip = other;
						bestAss = assign;
					}

				}

			} 
			
			obj = obj->nextTarget;
		} // end while()

		if (bestShip.isValid())
		{
			if(bestAss)
			{
				bestAss->bHasAdmiral = true;

				if(bestAss->set.contains(part->dwMissionID) == 0 && bestAss->set.numObjects < MAX_SELECTED_UNITS)
					bestAss->set.objectIDs[bestAss->set.numObjects++] = part->dwMissionID;

				// we are about to send a dock flagship packet, we must send an undock packet first if the flagship is currently
				// docked
				VOLPTR(IAdmiral) admiral = part.obj;
				if (admiral)
				{
					IBaseObject * dockship = admiral->GetDockship();
					if (dockship)
					{
						USR_PACKET<USRUNDOCKFLAGSHIP> packet;
						packet.objectID[0] = part->dwMissionID;
						packet.objectID[1] = dockship->GetPartID();
						packet.userBits = 0;		// clear out the queue - important
						packet.position.init(admiral.Ptr()->GetGridPosition(), admiral.Ptr()->GetSystemID());
						packet.init(2, false);		// don't track this command
						NETPACKET->Send(HOSTID, 0, &packet);
					}	

					USR_PACKET<USRDOCKFLAGSHIP> packet;

					packet.objectID[0] = part->dwMissionID;
					packet.objectID[1] = bestShip->dwMissionID;
					packet.userBits = (dockship!=0);			// very important - we don't want to flush the queue IF WE SENT AN UNDOCK COMMAND!
					packet.init(2);
					NETPACKET->Send(HOSTID, 0, &packet);
				}
			}
		}
	}
}
//-------------------------------------------------------------------------------------//
//
void SPlayerAI::doPortal (MPart & part)
{
	U32 targsys = 0;

	//fire the damn thing
	if(GetNumEnemiesAround(part) > 4)
	{
		targsys = part->systemID;
		if(SECTOR->GetNumSystems() > 1)
			while(targsys == part->systemID) targsys = (rand() % SECTOR->GetNumSystems()) + 1;
	}
	else
	{
		IBaseObject * targObj = findStrategicTarget(part,false,true);
		if(targObj)
		{
			targsys = targObj->GetSystemID();			
		}
	}

	if(targsys)
	{
		USR_PACKET<USRCREATEWORMHOLE> packet;
				
		packet.systemID = targsys;
		packet.objectID[0] = part->dwMissionID;
		packet.init(1);
		NETPACKET->Send(HOSTID, 0, &packet);

		CQTRACE12("SPWEAPON: Player %d firing Forger Portal for unit %X", playerID, part->dwMissionID);
	}
}
//-------------------------------------------------------------------------------------//
//
void SPlayerAI::MonitorAssignments(void)
{
	ASSIGNMENT * node = pAssignments;
	ASSIGNMENT * newNode;
	ASSIGNMENT * prev = 0;

	U32 scouttasks = 0;
	while (node)
	{
		node->militaryPower = 0;
		node->supships = 0;

		S32 numUnits = node->set.numObjects;
		if(numUnits < 0) numUnits = 0;

		MPart base = NULL;
		
		U32 Allin = AllInSystem(node);
		U32 AllinFleet = AllInFleet(node);
		while(numUnits)
		{
			U32 mID = node->set.objectIDs[numUnits - 1];
			IBaseObject * obj = OBJLIST->FindObject(mID);
			if(!obj)
			{
				node->set.removeObject(mID);
				numUnits--;
				continue;
			}
			
			MPart part = obj;
			if(!base) base = part;
	
			node->militaryPower += GetPowerRating(part);

			IBaseObject *targ = GetAttackInfo(obj);
			SINGLE targdist = 100000.0;
			if(targ && (targ->GetSystemID() == obj->GetSystemID()))
				targdist = targ->GetGridPosition() - obj->GetGridPosition();

			if((part->bNoAIControl) || ((obj->fieldFlags.bCelsius && targ && (targdist < 6)) && (rand() & 1)))
			{
				RemoveAssMember(node, part);
				numUnits--;
				continue;
			}

			if(MGlobals::IsSupplyShip(part->mObjClass))
				node->supships++;

			if((node->type != RESUPPLY) && (node->type != ATTACK) && (node->type != REPAIR_ASS))
			{
				if(Allin && TESTADMIRAL(mID) && ((node->type == DEFEND) || (node->type == ATTACK)) &&
					(AllinFleet == false))
				{
					//ensure admiral's ID is first in set
					ObjSet tempSet;
					//tempSet.numObjects = 0;
					CQASSERT(tempSet.numObjects == 0);
					tempSet.objectIDs[tempSet.numObjects++] = mID;

					U32 g = node->set.numObjects; 
					while(g > 0)
					{
						if(node->set.objectIDs[g - 1] != mID)
							tempSet.objectIDs[tempSet.numObjects++] = node->set.objectIDs[g - 1];
						g--;
					}
					
					node->set = tempSet;

					//send createfleet packet
					USR_PACKET<FLEETDEF_PACKET> packet;
					
					memcpy(packet.objectID, node->set.objectIDs, sizeof(packet.objectID));
					packet.init(node->set.numObjects);
					NETPACKET->Send(HOSTID, 0, &packet);
				}

				if(targ && DNA.buildMask.bUseSpecialWeapons && 
					(part->systemID > 0 && part->systemID <= MAX_SYSTEMS) &&
					(obj->fieldFlags.bCelsius == false) && 
					(!IsActing(mID)))
					FieldCorporal(mID, node);
			}

			numUnits--;
		}

		if(node->type == DEFEND)
		{
			if(DNA.buildMask.bLaunchOffensives && node->set.numObjects > 0)
			{
				if  ((m_AttackWaveSize && ((S32)node->set.numObjects) >= m_AttackWaveSize) || 
					(!DNA.buildMask.bBuildPlatforms && (m_ComPts - m_ComPtsUsedUp <= 2)) )
				{
					if(AllInSystem(node)/* && ((rand() & 3) == 1)*/)
					{
						doAttack(node);

						//chance to bring in DEFENDers to go all out...
						//chance to bring in SCOUTs to go all out...
						//chance to bring in ESCORTs to go all out...   fix   
					
						//set turnAttackLaunched
					}
					else if (node->uStallTime > ASSIGNMENT_STALL)
					{
						if(TESTADMIRAL(node->set.objectIDs[0]) && AllinFleet && (rand() & 1))
						{
							//send disband fleet packet
							USR_PACKET<FLEETDEF_PACKET> packet;
	
							packet.objectID[0] = node->set.objectIDs[0];
							packet.userBits = 0;
							packet.init(1, false);
							NETPACKET->Send(HOSTID, 0, &packet);
						}

						//disband assignment
						if (prev)
							prev->pNext = node->pNext;
						else
							pAssignments = node->pNext;
						delete node;
						if(prev)
							node = prev->pNext;
						else if(pAssignments)
							node = pAssignments->pNext;
						else
							node = 0;

						continue;
					}
					else
					{
						node->uStallTime++;
						//   fix   cull lost member and launch anyway.
					}
				}
				else if(m_bAssignsInvalid)
				{
					if(DNA.buildMask.bUseSpecialWeapons)
						FieldGeneral(node);
				}
			}
		}

		if(node->type == ATTACK)
		{
			if(node->targetID == 0)
			{
				U32 systemcheck = AllInSystem(node);
				if(systemcheck && (systemcheck < MAX_SYSTEMS))
				{
					UpdateFinalSolution();

					if(base)
					{
						IBaseObject *strategictarg = findStrategicTarget(base);
						if(strategictarg && (ReissueAttack(node, strategictarg)))
						{
							node->targetID = base->dwMissionID;
							doAttack(node, US_ATTACK, strategictarg);
						}
					}
				}
				else if (node->uStallTime > 30)
				{
					if(TESTADMIRAL(node->set.objectIDs[0]) && AllinFleet && (rand() & 1))
					{
						USR_PACKET<FLEETDEF_PACKET> packet;
	
						packet.objectID[0] = node->set.objectIDs[0];
						packet.userBits = 0;
						packet.init(1, false);
						NETPACKET->Send(HOSTID, 0, &packet);
					}

					//disband assignment
					if (prev)
						prev->pNext = node->pNext;
					else
						pAssignments = node->pNext;
					delete node;
					if(prev)
						node = prev->pNext;
					else if(pAssignments)
						node = pAssignments->pNext;
					else
						node = 0;

					continue;
				}
				else
					node->uStallTime++;
			}
			else if(node->uStallTime >= 50)
			{
				S32 ageFactor = ((m_Age * m_Age) / 32);
				if(ageFactor > 5000) ageFactor = 5000;
				U32 mID = node->set.objectIDs[0];
				IBaseObject * targ_obj = OBJLIST->FindObject(node->targetID);
				IBaseObject * obj = OBJLIST->FindObject(mID);
				if(m_Terminate && targ_obj && (m_Terminate == targ_obj->GetPlayerID()))
					ageFactor = 0;
				SINGLE dist = 100.0;
				if(targ_obj && obj && (targ_obj->GetSystemID() == obj->GetSystemID()))
				{
					GRIDVECTOR position = obj->GetGridPosition();
					dist = position-targ_obj->GetGridPosition();
					CQASSERT(dist < 100.0);
				}
				dist = ((dist * dist) / 8.0) - (((SINGLE)DNA.nOffenseVsDefense) * 16.0);

				if((node->militaryPower * ((S32)node->set.numObjects)) > (200 + ageFactor + ((S32)dist)))
				{
					if(AllInSystem(node))
					{	
						//Relaunch Mass Attack
						/*
						USR_PACKET<USRATTACK> packet;
						packet.targetID = node->targetID;

						//concentrate on that HQ!  Otherwise, let unit AI pick targets...
						packet.bUserGenerated = m_bKillHQ;
						//packet.bUserGenerated = 1;

						memcpy(packet.objectID, node->set.objectIDs, sizeof(packet.objectID));
						packet.userBits = 0;  //0 means cancel current op when this one arrives.
						packet.init(node->set.numObjects);
						NETPACKET->Send(HOSTID, 0, &packet);

						node->uStallTime = 0;
						*/

						if(node->set.numObjects && ((rand()) % node->set.numObjects))
						{
							UpdateFinalSolution();

							if(base)
							{
								IBaseObject *strategictarg = findStrategicTarget(base);
								if(strategictarg && (ReissueAttack(node, strategictarg)))
								{
									node->targetID = base->dwMissionID;
									doAttack(node, US_ATTACK, strategictarg);
								}
							}
						}
						else
						{
							//disband assignment
							if(TESTADMIRAL(node->set.objectIDs[0]) && AllinFleet && (rand() & 1))
							{
								USR_PACKET<FLEETDEF_PACKET> packet;
	
								packet.objectID[0] = node->set.objectIDs[0];
								packet.userBits = 0;
								packet.init(1, false);
								NETPACKET->Send(HOSTID, 0, &packet);
							}

							if (prev)
								prev->pNext = node->pNext;
							else
								pAssignments = node->pNext;
							delete node;
							if(prev)
								node = prev->pNext;
							else if(pAssignments)
								node = pAssignments->pNext;
							else
								node = 0;

							continue;
						}
					}
					else
					{
						//the offending, out-of-system member of this ass will be dealt with in assigngunboat
					}
				}
				else
				{
					//disband assignment

					if(TESTADMIRAL(node->set.objectIDs[0]) && AllinFleet && (rand() & 1))
					{
						USR_PACKET<FLEETDEF_PACKET> packet;
	
						packet.objectID[0] = node->set.objectIDs[0];
						packet.userBits = 0;
						packet.init(1, false);
						NETPACKET->Send(HOSTID, 0, &packet);
					}

					if (prev)
						prev->pNext = node->pNext;
					else
						pAssignments = node->pNext;
					delete node;
					if(prev)
						node = prev->pNext;
					else if(pAssignments)
						node = pAssignments->pNext;
					else
						node = 0;
					
					continue;
				}
			}
			else
			{
				if(DNA.buildMask.bUseSpecialWeapons)
					FieldGeneral(node);

				if(strategy == SOLARIAN_DENY)
				{
					IBaseObject *newTargObj = findStrategicTarget(base);
					MPart newTarg = newTargObj;
					if (newTarg && (MGlobals::IsHarvester(newTarg->mObjClass) || MGlobals::IsFabricator(newTarg->mObjClass)))
					{
						U32 newTargID = newTarg->dwMissionID;
						if(node->targetID != newTargID)
						{
							node->targetID = base->dwMissionID;
							doAttack(node, US_ATTACK, newTargObj, true);
						}
					}
				}
			}
		}

		if(node->type == SCOUT)
		{
			scouttasks++;
			if(DNA.buildMask.bScout == false)
			{
				//put this in removeAssignment function, goddammit, fix  
				if (prev)
					prev->pNext = node->pNext;
				else
					pAssignments = node->pNext;
				delete node;
				if(prev)
					node = prev->pNext;
				else if(pAssignments)
					node = pAssignments->pNext;
				else
					node = 0;
				scouttasks--;
				continue;
			}
		}

		if(node->type == ESCORT)
		{
			numUnits = node->set.numObjects;
			U32 Allin = AllInSystem(node);
			while(numUnits)
			{
				U32 mID = node->set.objectIDs[numUnits - 1];
				IBaseObject * obj = OBJLIST->FindObject(mID);
				MPart part = obj;
				
				if(part)
				{
					if(part->mObjClass == M_AURORA && node->bFabricator && Allin && (!obj->bCloaked))
					{
						if(DoIHave(S_CLOAKER))  //can also cloak itself normally
						{
						//use friendly target
							USR_PACKET<USRSPATTACK> packet;
										
							packet.targetID = node->targetID;
							packet.objectID[0] = mID;
							packet.init(1);
							NETPACKET->Send(HOSTID, 0, &packet);
						
							CQTRACE12("SPWEAPON: Player %d firing Aurora Cloak Provider for unit %X", playerID, mID);
							//bSentPacket = true;
						}
						else
						{
							ResearchDesires[S_CLOAKER].nPriority += 30;
						}
						/* cloak normally
						{
							USR_PACKET<USRCLOAK> packet;
								
							//packet.targetID = target;
							packet.objectID[0] = unitID;
							packet.init(1);
							NETPACKET->Send(HOSTID, 0, &packet);
							bSentPacket = true;
						}
						*/
					}
				}

				numUnits--;
			}
		}

		prev = node;
		node = node->pNext;
	}

	if(DNA.buildMask.bScout && scouttasks==0)
	{
		//create scouting assignment
		newNode = new ASSIGNMENT;
		newNode->Init();
		newNode->type = SCOUT;
		newNode->pNext = pAssignments;
		pAssignments = newNode;
		newNode->uStallTime = 0;

		//cullAssignments will not remove a scouting assignment
	}

	if(m_bAssignsInvalid)
	{
		if((rand() % 5) != 0) m_bAssignsInvalid = false;
	}
}
//---------------------------------------------------------------------------------------------//
//  FieldGeneral.  Uses special weapons.
//---------------------------------------------------------------------------------------------//
void SPlayerAI::FieldGeneral (ASSIGNMENT * node)
{
	bool bSentPacket = false;
	U32 numUnits = node->set.numObjects;

	bSentPacket = false;

	if(numUnits < 0) numUnits = 0;
	while(numUnits)
	{
		U32 mID = node->set.objectIDs[numUnits - 1];

		IBaseObject * obj = OBJLIST->FindObject(mID); //wish i didn't have to do this
		if(obj && (obj->GetSystemID() > 0) && (obj->GetSystemID() <= MAX_SYSTEMS) && (obj->fieldFlags.bCelsius == false) && (!IsActing(mID)))
			bSentPacket = bSentPacket || FieldCorporal(mID, node);
	
		numUnits--;
	}

	/*

	if(bSentPacket)
	{
		//revalidate monitorAttack update flag (to prevent doubling up/flooding)
		m_bAssignsInvalid = false;
	}
	*/
}
//-------------------------------------------------------------------------------------//
//
bool SPlayerAI::FieldCorporal(U32 unitID, ASSIGNMENT * my_ass)
{
	bool bSentPacket = false;
	IBaseObject * obj = OBJLIST->FindObject(unitID);
	if(!obj) return false;

	MPart part = obj;
	//if(part->supplies < (part->supplyPointsMax / 2)) //in some situations you might want to fire a special weapon even if you have less than half your supplies  fix  
	//	return false;
		
/////////////////SPECIAL WEAPON USAGE CHECKS//////////////////////
	if(part->mObjClass == M_DREADNOUGHT)
	{
		//gonna work like cloaking
		if(DoIHave(T_DREADNOUGHTSHIELD))
		{
			//if((GetNumEnemiesAround(part) + (rand() & 7)) > DREADTHRESH)
			U32 enemies = GetNumEnemiesAround(part, 6);
			if((((((DOUBLE)part->hullPoints) / ((DOUBLE)part->hullPointsMax)) < 0.5) && 
				(obj->effectFlags.bAgeisShield == false) && (enemies > 0)) || 
				((obj->effectFlags.bAgeisShield == true) && (enemies == 0)))
			{
				USR_PACKET<USRSPABILITY> packet;
				
				packet.objectID[0] = unitID;
				packet.init(1);
				NETPACKET->Send(HOSTID, 0, &packet);
				bSentPacket = true;

				CQTRACE12("SPWEAPON: Player %d toggling Dreadnaught Aegis Shield for unit %X", playerID, unitID);
			}
		}
		else
		{
			ResearchDesires[T_DREADNOUGHTSHIELD].nPriority += 50;
		}
	}
	if(part->mObjClass == M_CARRIER)
	{
		if(DoIHave(T_CARRIERPROBE))
		{
			//fire towards enemy base, i suppose
			bool bIgnoreDistance = true;
			IBaseObject * targObj = findStrategicTarget(part,false,bIgnoreDistance);
			if(targObj)
			{
				NETGRIDVECTOR targloc;
				targloc.init(targObj->GetPosition(), targObj->GetSystemID());
				
				USR_PACKET<USRAOEATTACK> packet;
				
				packet.position = targloc;
				packet.objectID[0] = unitID;
				packet.init(1);
				NETPACKET->Send(HOSTID, 0, &packet);
				bSentPacket = true;

				//CQASSERT("Firing Carrier Probe.  100% Igonrable" && 0);
				CQTRACE12("SPWEAPON: Player %d firing Carrier Probe for unit %X", playerID, unitID);
			}
			
		}
		else
		{
			ResearchDesires[T_CARRIERPROBE].nPriority += 50;
		}
	}
	
	if(part->mObjClass == M_LANCER)
	{
		if(DoIHave(T_LANCERVAMPIRE))
		{
			IBaseObject * targObj = GetAttackInfo(obj);
			if(targObj)
			{
				U32 target = targObj->GetPartID();
					
				//USR_PACKET<USRSPABILITY> packet;
				USR_PACKET<USRSPATTACK> packet;
					
				packet.targetID = target;
				packet.objectID[0] = unitID;
				packet.init(1);
				NETPACKET->Send(HOSTID, 0, &packet);
				bSentPacket = true;

				//CQASSERT("Firing Lancer Vampire Arc.  100% Igonrable" && 0);
				CQTRACE12("SPWEAPON: Player %d firing Vampire Arc for unit %X", playerID, unitID);
			}
		}
		else
		{
			ResearchDesires[T_LANCERVAMPIRE].nPriority += 50;
		}
	}
	if(part->mObjClass == M_BATTLESHIP)
	{
		if(DoIHave(T_BATTLESHIPCHARGE))
		{
			IBaseObject * objTarg = GetJuicyAreaEffectTarget (part, 6, 7);  //these parameters correct?  fix  
		
			//////////////////////////for firing through wormholes  fix  
			/*
			struct USRWORMATTACK : BASE_PACKET
{
	U32 targetID;

	USRWORMATTACK (void)
	{
		type = PT_USRWORMATTACK;
	}
};

  set target id to wormhole id
			*/
			///////////////////////////////////////////////////////////

			if(objTarg)
			{
				//U32 target = objTarg->GetPartID();
				NETGRIDVECTOR targloc;
				targloc.init(objTarg->GetGridPosition(), objTarg->GetSystemID());
					//struct USRSPATTACK : BASE_PACKET
				//USR_PACKET<USRSPABILITY> packet;
				USR_PACKET<USRAOEATTACK> packet;
				
				packet.position = targloc;
				packet.objectID[0] = unitID;
				packet.init(1);
				NETPACKET->Send(HOSTID, 0, &packet);
				bSentPacket = true;

				//CQASSERT("Firing Battleship Tempest Charge.  100% Igonrable" && 0);
				CQTRACE12("SPWEAPON: Player %d firing Tempest Charge for unit %X", playerID, unitID);
			}
		}
		else
		{
			ResearchDesires[T_BATTLESHIPCHARGE].nPriority += 30;
		}
	}
	if(part->mObjClass == M_MISSILECRUISER)
	{
		if(DoIHave(T_MISSILECLOAKING))
		{
			UNIT_STANCE stance = US_DEFEND;
			IBaseObject * targObj = GetAttackInfo(obj, &stance);
			if(targObj && (!obj->bCloaked) && (targObj->GetPlayerID() != playerID) && (stance == US_ATTACK))
			{
				//SINGLE dist = (part.obj->GetGridPosition() - targObj->GetGridPosition()).magnitude();
				SINGLE dist = part.obj->GetGridPosition() - targObj->GetGridPosition();
				if((targObj->GetSystemID() != part->systemID) || 
					 dist > CLOAK_DISTANCE)
				{				
					USR_PACKET<USRCLOAK> packet;
					
					//packet.targetID = target;
					packet.objectID[0] = unitID;
					packet.init(1);
					NETPACKET->Send(HOSTID, 0, &packet);
					bSentPacket = true;

					//  fix  reissue attack packet on the heels of this?  or let monitor ass take care of it?
					// note: should probobly set the usergenerated flag here
					// grouped command would be nice  FIX  
					USR_PACKET<USRATTACK> packet2;
					packet2.targetID = targObj->GetPartID();
					packet2.userBits = 1;
					packet2.objectID[0] = unitID;
					packet2.init(1);
					NETPACKET->Send(HOSTID, 0, &packet2);
					bSentPacket = true;

					//CQASSERT("Firing Missile Cruiser Cloaking.  100% Igonrable" && 0);
					CQTRACE12("SPWEAPON: Player %d firing MCruiser Cloaking for unit %X", playerID, unitID);
				}
			}

			if(obj->bCloaked && (stance == US_DEFEND))
			{
				//turn off cloaking
				USR_PACKET<USRCLOAK> packet;
				packet.objectID[0] = unitID;
				packet.init(1);
				NETPACKET->Send(HOSTID, 0, &packet);
				bSentPacket = true;

				CQTRACE12("SPWEAPON: Player %d turned off MCruiser Cloaking for unit %X", playerID, unitID);
			}
		}
		else
		{
			ResearchDesires[T_MISSILECLOAKING].nPriority += 10;
		}
	}
	if(part->mObjClass == M_FRIGATE && !obj->bCloaked)
	{
		if(DoIHave(M_CAMOFLAGE))
		{
			UNIT_STANCE stance = US_DEFEND;
			IBaseObject * targObj = GetAttackInfo(obj, &stance);
			if(targObj && (stance == US_ATTACK) && (obj->bMimic == false))
			{
				//SINGLE dist = (part.obj->GetGridPosition() - targObj->GetGridPosition()).magnitude();
				SINGLE dist = part.obj->GetGridPosition() - targObj->GetGridPosition();
				if((targObj->GetSystemID() != part->systemID) || 
					 dist > CLOAK_DISTANCE)
				{
					IBaseObject * mimicObj = GetMimic(obj);
					if(mimicObj)
					{
						U32 target = mimicObj->GetPartID();
						USR_PACKET<USRMIMIC> packet;
						
						packet.targetID = target;
						packet.objectID[0] = unitID;
						packet.init(1);
						NETPACKET->Send(HOSTID, 0, &packet);
						bSentPacket = true;

						//CQASSERT("Firing Frigate Mimic.  100% Igonrable" && 0);
						CQTRACE12("SPWEAPON: Player %d firing Frigate Mimic for unit %X", playerID, unitID);
					}
				}
			}
		}
		else
		{
			ResearchDesires[M_CAMOFLAGE].nPriority += 10;
		}
	}
	
	if(part->mObjClass == M_KHAMIR)
	{
		if(DoIHave(M_RAM1))
		{
			IBaseObject * targObj = GetAttackInfo(obj);
			if(targObj && (targObj->GetPartID() == my_ass->targetID) && (!IsAlly(targObj->GetPlayerID())))
			{
				U32 target = targObj->GetPartID();
					
				//USR_PACKET<USRSPABILITY> packet;
				USR_PACKET<USRSPATTACK> packet;
					
				packet.targetID = target;
				packet.objectID[0] = unitID;
				packet.init(1);
				NETPACKET->Send(HOSTID, 0, &packet);
				bSentPacket = true;

				//CQASSERT("Firing Khamir Kamikaze Attack.  100% Igonrable" && 0);
				CQTRACE12("SPWEAPON: Player %d firing Khamir Ram for unit %X", playerID, unitID);
			}
		}
		else
		{
			ResearchDesires[M_RAM2].nPriority += 10;
		}
	}

	if(part->mObjClass == M_HIVECARRIER)
	{
		if(DoIHave(M_REPELCLOUD))
		{
			IBaseObject * objTarg = GetJuicyAreaEffectTarget (part, 6, 5); //parameters ok?  fix  
			if(objTarg && (objTarg->GetSystemID() > 0) && (objTarg->GetSystemID() <= MAX_SYSTEMS))
			{
				//U32 target = objTarg->GetPartID();
				NETGRIDVECTOR targloc;
				targloc.init(objTarg->GetGridPosition(), objTarg->GetSystemID());
				//struct USRSPATTACK : BASE_PACKET
				//USR_PACKET<USRSPABILITY> packet;
				USR_PACKET<USRAOEATTACK> packet;
				
				packet.position = targloc;
				packet.objectID[0] = unitID;
				packet.init(1);
				NETPACKET->Send(HOSTID, 0, &packet);
				bSentPacket = true;

				//CQASSERT("Firing Hive Carrier Repellent Cloud.  100% Igonrable" && 0);
				CQTRACE12("SPWEAPON: Player %d firing Repellent Cloud for unit %X", playerID, unitID);
			}
		}
		else
		{
			ResearchDesires[M_REPELCLOUD].nPriority += 50;
		}
	}
	if(part->mObjClass == M_SCARAB)
	{
		if(DoIHave(M_GRAVWELL))
		{
			//IBaseObject * objTarg = GetJuicyAreaEffectTarget (part, 6, 10);  //what i originally intended
			IBaseObject * objTarg = GetJuicyAreaEffectTarget (part, 6, 7);  //parameters ok?  fix  

			if(objTarg)
			{
				//U32 target = objTarg->GetPartID();
				NETGRIDVECTOR targloc;
				targloc.init(objTarg->GetGridPosition(), objTarg->GetSystemID());
				//USR_PACKET<USRSPABILITY> packet;
				USR_PACKET<USRAOEATTACK> packet;
				
				packet.position = targloc;
				packet.objectID[0] = unitID;
				packet.init(1);
				NETPACKET->Send(HOSTID, 0, &packet);
				bSentPacket = true;

				//CQASSERT("Firing Scarab Gravity Well.  100% Igonrable" && 0);
				CQTRACE12("SPWEAPON: Player %d firing Gravity Well for unit %X", playerID, unitID);
			}
		}
		else
		{
			ResearchDesires[M_GRAVWELL].nPriority += 30;
		}
	}
	
	if(part->mObjClass == M_TIAMAT)
	{
		if(DoIHave(M_REPULSOR))
		{
			if((((S32)GetNumEnemiesAround(part, 5)) > (7 + (rand() & 15))) && (part->supplies > (part->supplyPointsMax / 3)))
			{
				USR_PACKET<USRSPABILITY> packet;
			
				//packet.targetID = target;
				packet.objectID[0] = unitID;
				packet.init(1);
				NETPACKET->Send(HOSTID, 0, &packet);
				bSentPacket = true;

				//CQASSERT("Firing Tiamat Repulsor Beam.  100% Igonrable" && 0);
				CQTRACE12("SPWEAPON: Player %d firing Repulsor Wave for unit %X", playerID, unitID);
			}
		}
		else
		{
			ResearchDesires[M_REPULSOR].nPriority += 50;
		}
	}
	if(part->mObjClass == M_TAOS)
	{
		if(DoIHave(S_SYNTHESIS))
		{
			U32 c = my_ass->set.numObjects;
			while(c)
			{
				U32 tID = my_ass->set.objectIDs[c - 1];
				if(tID != unitID)
				{
					MPart targ = OBJLIST->FindObject(tID);
					if(targ)
					{
						S32 pow = GetPowerRating(targ);
						if((pow > 75) && (((targ->hullPointsMax - targ->hullPoints) / 16) > SYNTH_THRESH))
						{
							USR_PACKET<USRSPATTACK> packet;
							
							packet.targetID = targ->dwMissionID;
							packet.objectID[0] = unitID;
							packet.init(1);
							NETPACKET->Send(HOSTID, 0, &packet);
							bSentPacket = true;

							CQTRACE12("SPWEAPON: Player %X firing Taos Synthesis for unit %X", playerID, unitID);
							break;
						}
					}
				}
				c--;
			}
			//use friendly target
			/*
			USR_PACKET<USRSPATTACK> packet;
							
			packet.targetID = targObj->GetPartID();
			packet.objectID[0] = unitID;
			packet.init(1);
			NETPACKET->Send(HOSTID, 0, &packet);
			bSentPacket = true;

			CQASSERT("Firing Taos Synthesis.  100% Igonrable" && 0);
			CQTRACE12("SPWEAPON: Player %d firing Taos Synthesis for unit %d", playerID, unitID);
			*/
		}
		else
		{
			ResearchDesires[S_SYNTHESIS].nPriority += 5;
		}
	}
	if(part->mObjClass == M_POLARIS)
	{
		if(DoIHave(S_MASSDISRUPTOR))
		{
			IBaseObject * targObj = GetAttackInfo(obj);
			if(targObj)
			{
				MPart targpart = targObj;
				U32 rating = GetOffensivePowerRating(targpart);
				if((rand() % 3 == 0) || (rating > GetOffensivePowerRating(part))
					|| (m_bKillHQ && MGlobals::IsHQ(targpart->mObjClass))) 
				{
					USR_PACKET<USRSPATTACK> packet;
				
					packet.targetID = targObj->GetPartID();
					packet.objectID[0] = unitID;
					packet.init(1);
					NETPACKET->Send(HOSTID, 0, &packet);
					bSentPacket = true;

					//CQASSERT("Firing Polaris Mass Disruptor.  100% Igonrable" && 0);
					CQTRACE12("SPWEAPON: Player %d firing Mass Disruptor for unit %X", playerID, unitID);
				}
			}
		}
		else
		{
			ResearchDesires[S_MASSDISRUPTOR].nPriority += 30;
		}
	}	
	if(part->mObjClass == M_AURORA)
	{
		if(DoIHave(S_CLOAKER))  //changing to cloak provider, can also cloak itself normally
		{
			//use friendly target, handled in monitorAssignments
			/*
			{
				USR_PACKET<USRSPATTACK> packet;
							
				packet.targetID = targObj->GetPartID();
				packet.objectID[0] = unitID;
				packet.init(1);
				NETPACKET->Send(HOSTID, 0, &packet);
				bSentPacket = true;

				CQASSERT("Firing Aurora Cloak Provider.  100% Igonrable" && 0);	
				CQTRACE12("SPWEAPON: Player %d Firing Aurora Cloaker for unit %d", playerID, unitID);
			}
			{
				USR_PACKET<USRCLOAK> packet;
					
				//packet.targetID = target;
				packet.objectID[0] = unitID;
				packet.init(1);
				NETPACKET->Send(HOSTID, 0, &packet);
				bSentPacket = true;
			}
			*/
		}
		else
		{
			ResearchDesires[S_CLOAKER].nPriority += 30;
		}
	}
	if(part->mObjClass == M_TRIREME)
	{
		if(DoIHave(S_DESTABILIZER))
		{
			IBaseObject * objTarg = GetJuicyAreaEffectTarget (part, 6, 7);  //parameters ok?  fix  
		
			//  fix  through wormholes
			if(objTarg)
			{
				//U32 target = objTarg->GetPartID();
				NETGRIDVECTOR targloc;
				targloc.init(objTarg->GetGridPosition(), objTarg->GetSystemID());
				//struct USRSPATTACK : BASE_PACKET
				//USR_PACKET<USRSPABILITY> packet;
				USR_PACKET<USRAOEATTACK> packet;
				
				packet.position = targloc;
				packet.objectID[0] = unitID;
				packet.init(1);
				NETPACKET->Send(HOSTID, 0, &packet);
				bSentPacket = true;

				//CQASSERT("Firing Trireme Destabilizer.  100% Igonrable" && 0);
				CQTRACE12("SPWEAPON: Player %d firing Destabilizer for unit %X", playerID, unitID);
			}
		}
		else
		{
			ResearchDesires[S_DESTABILIZER].nPriority += 40;
		}
	}
	
	if(part->mObjClass == M_MONOLITH)
	{
		if(DoIHave(S_TRACTOR))
		{
			IBaseObject * targObj = GetAttackInfo(obj);
			if(targObj)
			{
				USR_PACKET<USRSPABILITY> packet;
				
				packet.objectID[0] = unitID;
				packet.init(1);
				NETPACKET->Send(HOSTID, 0, &packet);
				bSentPacket = true;

				//CQASSERT("Firing Monolith Tractor Beam.  100% Igonrable" && 0);
				CQTRACE12("SPWEAPON: Player %d firing Tractor Beam for unit %X", playerID, unitID);
			}
		}
		else
		{
			ResearchDesires[S_TRACTOR].nPriority += 50;
		}
	}
	//////////////////////////////////////////////////////////////////////////

	if(bSentPacket)
	{
		for(U32 c = 0; c < 100; c++)
		{
			if(m_ActingUnits[c].mID == 0)
			{
				m_ActingUnits[c].mID = unitID;

				U32 timer = 10;   ///  fix   might be smart to base timer on dist to target
				switch(part->mObjClass)
				{
				case M_CARRIER:
					timer = 0;
				break;
				case M_MONOLITH:
				case M_POLARIS:
					timer = 15;
				break;
				case M_TIAMAT:
					timer = 8;
				break;
				case M_BATTLESHIP:
					timer = 12;
				break;
				case M_LANCER:
					timer = 15;
				break;
				default:
				break;
				}

				m_ActingUnits[c].timer = timer;
				break;
			}
		}
	}

	return bSentPacket;
}
//-------------------------------------------------------------------------------------//
//
bool SPlayerAI::IsActing (U32 uID)
{
	for(U32 c = 0; c < 100; c++)
	{
		if(m_ActingUnits[c].mID == uID)
			return true;
	}

	return false;
}
//-------------------------------------------------------------------------------------//
//
U32 SPlayerAI::AllInSystem (ASSIGNMENT * node)
{
	U32 numUnits = node->set.numObjects;
	U32 sys = 0;

	if(numUnits < 0) numUnits = 0;
	while(numUnits)
	{
		U32 mID = node->set.objectIDs[numUnits - 1];
		IBaseObject * obj = OBJLIST->FindObject(mID);
		if(obj)
		{
			if(sys == 0)
			{
				sys = obj->GetSystemID();
				if((sys == 0) || (sys > MAX_SYSTEMS)) return 0;
			}
			else
			{
				if(sys != obj->GetSystemID()) return 0;
			}
		}

		numUnits--;
	}

	return sys;
}
//-------------------------------------------------------------------------------------//
//
U32 SPlayerAI::AllInFleet (ASSIGNMENT * node)
{
	U32 numUnits = node->set.numObjects;
	U32 fleetID = 0;

	if(numUnits < 0) numUnits = 0;
	while(numUnits)
	{
		U32 mID = node->set.objectIDs[numUnits - 1];
		MPart part = OBJLIST->FindObject(mID);
		if(part)
		{
			if(fleetID == 0)
			{
				fleetID = part->fleetID;
				if(fleetID == 0) return 0;
			}
			else
			{
				if(fleetID != part->fleetID) return 0;
			}
		}

		numUnits--;
	}

	return fleetID;
}
//-------------------------------------------------------------------------------------//
//
bool SPlayerAI::AssignmentMemberLost(MPart & part, ASSIGNMENT * node)
{
	U32 numUnits = node->set.numObjects;
	U32 sys = 0;
	U32 SavedSys = 0;
	IBaseObject * obj = NULL;
	U32 unitcount[MAX_SYSTEMS+1];

	memset(unitcount, 0, (sizeof(U32) * (MAX_SYSTEMS+1)));

	if(numUnits < 0) numUnits = 0;
	while(numUnits)
	{
		U32 mID = node->set.objectIDs[numUnits - 1];
		if(part->dwMissionID == mID) SavedSys = part->systemID;
	
		obj = OBJLIST->FindObject(mID);
		if(obj)
		{
			sys = obj->GetSystemID();
			if(sys >= 0 && sys <= MAX_SYSTEMS)
			{
			//CQASSERT(sys >= 0 && sys <= MAX_SYSTEMS);
			unitcount[sys]++;
			}
			else return false;  //people are in hyperspace, cats living with dogs, let it slide
		}

		numUnits--;
	}

	U32 best = 0;
	U32 highest = 0;
	for(U32 c = 0; c < MAX_SYSTEMS+1; c++)
	{
		if(unitcount[c] > highest)
		{
			highest = unitcount[c];
			best = c;
		}
	}
	
	if(best != SavedSys)
		return true;
	else
		return false;
}
//-------------------------------------------------------------------------------------//
//
IBaseObject * SPlayerAI::GetAttackInfo (IBaseObject * base, UNIT_STANCE *stance)
{
	OBJPTR<IAttack> attack;
	IBaseObject *ret = NULL;
	U32 retID = 0;
	
	if (base->QueryInterface(IAttackID, attack))
	{
		if(attack)
		{
			attack->GetTarget(ret, retID);
			if(stance != NULL) *stance = attack->GetUnitStance();
		}
	}

	return ret;
}
//-------------------------------------------------------------------------------------//
//
IBaseObject * SPlayerAI::GetMimic(IBaseObject *base)
{
	IBaseObject * ret = NULL;
	IBaseObject * obj = OBJLIST->GetTargetList();
	U32 points = 0;
	U32 bestPoints = 0;
	MPart part;
	while (obj)
	{
		points = 0;
		if ((part = obj).isValid())
		{
			if(MGlobals::IsHarvester(part->mObjClass)) points += (100 + (rand() & 63));
			if(MGlobals::IsSupplyShip(part->mObjClass)) points += (75 + (rand() & 63));
			if(MGlobals::IsFabricator(part->mObjClass)) points += (50 + (rand() & 63));
			if(MGlobals::IsHeavyGunboat(part->mObjClass)) points += (50 + (rand() & 127));
		}

		if(points > bestPoints)
		{
			bestPoints = points;
			ret = obj;
		}

		obj = obj->nextTarget;
	}

	return ret;
}

//-------------------------------------------------------------------------------------//
//
IBaseObject * SPlayerAI::GetJuicyAreaEffectTarget (MPart & part, U32 range, U32 radius)
{
	UNIT_STANCE stance = US_DEFEND;
	IBaseObject * betterTarget = GetAttackInfo(part.obj, &stance);
	if(betterTarget && ((betterTarget->GetSystemID() < 1) || (betterTarget->GetSystemID() > MAX_SYSTEMS)))
		betterTarget = 0;

	/*    fix    TOTAL BULLSHIT


	IBaseObject * betterTarget = NULL;
	SINGLE testDistance = 0.0;
	SINGLE bestDistance = 0.0;
	radius *= GRIDSIZE;
	const U32 allyMask = MGlobals::GetAllyMask(part->dwMissionID & PLAYERID_MASK);
	ObjMapIterator it(part->systemID, part.obj->GetTransform().translation, radius, playerID);

	while (it)
	{
		if ((it->flags & OM_UNTOUCHABLE) == 0)
		{
			const U32 hisPlayerID = it.GetApparentPlayerID(allyMask);//it->dwMissionID & PLAYERID_MASK;
			// if we are enemies
			if (hisPlayerID && (((1 << (hisPlayerID-1)) & allyMask) == 0))
			{
				// as long as the enemy is visible
				if ((it->flags & OM_TARGETABLE) && it->obj->IsVisibleToPlayer(part->dwMissionID & PLAYERID_MASK))
				{
//////////////////////FUN WITH NESTED LOOPS
					GRIDVECTOR base = it->obj->GetGridPosition();
					testDistance = 0.0;
					
					ObjMapIterator more_of_it(part->systemID, it->obj->GetTransform().translation, radius/2 + 1, playerID);
					while (more_of_it)
					{
						if (((more_of_it->flags & OM_UNTOUCHABLE) == 0) && (more_of_it->dwMissionID != it->dwMissionID))
						{
							const U32 more_of_hisPlayerID = more_of_it.GetApparentPlayerID(allyMask);//((more_of_it->dwMissionID) & PLAYERID_MASK);

							if (more_of_hisPlayerID && (((1 << (more_of_hisPlayerID-1)) & allyMask) == 0))
							{
								//SINGLE temp_mag = (base - more_of_it->obj->GetGridPosition()).magnitude();
								SINGLE temp_mag = base - more_of_it->obj->GetGridPosition();
								if((U32)temp_mag < range)
								{
									testDistance += (range - ((U32)temp_mag));
								}
							}
							else if (more_of_hisPlayerID || (more_of_hisPlayerID == playerID))
							{
								SINGLE temp_mag = base - more_of_it->obj->GetGridPosition();
								if((U32)temp_mag < range)
								{
									testDistance -= ((range - ((U32)temp_mag)) / 2);
								}
							}
						}

						++more_of_it;
					}

					if (testDistance > bestDistance)
					{
						bestDistance = testDistance;
						betterTarget = it->obj;
					}
///////////////////////////////////////////
				}
			}
		}
		++it;
	}
*/
	return betterTarget;
}
//---------------------------------------------------------------------------//
//
U32 SPlayerAI::getNumGunplats (void)
{
	return	UnitsOwned[M_LSAT] + UnitsOwned[M_SPACESTATION] + UnitsOwned[M_IONCANNON] + 
			UnitsOwned[M_PLASMASPLITTER] + UnitsOwned[M_PLASMAHIVE] + UnitsOwned[M_VORAAKCANNON] + 
			UnitsOwned[M_PROTEUS] + UnitsOwned[M_HYDROFOIL] + 
			UnitsOwned[M_ESPCOIL] + UnitsOwned[M_STARBURST];
}
//-------------------------------------------------------------------------------------//
//
U32 SPlayerAI::GetNumEnemiesAround(MPart & part, int range)
{
	U32 ret = 0;

	const U32 allyMask = MGlobals::GetAllyMask(part->dwMissionID & PLAYERID_MASK);
	ObjMapIterator it(part->systemID, part.obj->GetTransform().translation, range * GRIDSIZE, playerID);

	while (it)
	{
		const U32 hisPlayerID = it.GetApparentPlayerID(allyMask);//it->dwMissionID & PLAYERID_MASK;

		// if we are enemies
		if (hisPlayerID && (((1 << (hisPlayerID-1)) & allyMask) == 0))
			ret++;

		++it;
	}

	return ret;
}
//-------------------------------------------------------------------------------------//
//  for thrashing, mainly
bool SPlayerAI::ReissueAttack(ASSIGNMENT *assign, IBaseObject * targ)
{
	S32 numUnits = assign->set.numObjects;
		if(numUnits < 0) numUnits = 0;

	S32 numBusy = 0;
		
	while(numUnits)
	{
		U32 mID = assign->set.objectIDs[numUnits - 1];
		IBaseObject * obj = OBJLIST->FindObject(mID);
		if(!obj)
		{
			numUnits--;
			continue;
		}

		UNIT_STANCE stance = US_DEFEND;
		IBaseObject * cur_targ = GetAttackInfo(obj, &stance);

		if(cur_targ && (cur_targ->GetSystemID() == targ->GetSystemID()) && (stance == US_ATTACK))
			numBusy++;

		numUnits--;
	}

	if(numBusy > (((S32)(assign->set.numObjects)) / 2))
		return false;
	else return true;
}
//-------------------------------------------------------------------------------------//
//  
void SPlayerAI::doAttack (ASSIGNMENT * assign, UNIT_STANCE stance, IBaseObject * targ, bool bForce)
{
	//concentrate on that HQ!  Otherwise, let unit AI pick targets.
	bool bUserGen = false;
			
	MPart base = OBJLIST->FindObject(assign->targetID);
	if ((base && base.isValid()) || (targ != NULL))
	{
		IBaseObject * obj = NULL;
		if(base && targ == NULL)
			obj = findStrategicTarget(base);
		else
		{
			obj = targ;
			bUserGen = true;
		}

		//build tempSet for units that NEED stance change

		//build tempSet for units that NEED to be told to attack
		//IBaseObject * SPlayerAI::GetTarget (IBaseObject * base)
		//(if they already have a target in the vicinity of the new target, don't iclude them in the attack packet set

		MPart part;
		if (obj && ((part = obj).isValid()))
		{
			if(bForce == false)
				bUserGen = (bUserGen || ((m_bKillHQ && MGlobals::IsHQ(part->mObjClass))));
			else
				bUserGen = true;

			//prep stance packet
			UNIT_STANCE cur_stance = US_DEFEND;
			ObjSet TEMPset;
			TEMPset.objectIDs[0] = 0;			// quiet a compiler warning
			for(U32 c = 0; c < assign->set.numObjects; c++)
			{
				IBaseObject *unit_obj = OBJLIST->FindObject(assign->set.objectIDs[c]);
				if(unit_obj)
				{
					GetAttackInfo(unit_obj, &cur_stance);
					if(cur_stance != stance)
						TEMPset.objectIDs[TEMPset.numObjects++] = assign->set.objectIDs[c];
				}
			}

			if(TEMPset.numObjects > 0)
			{
				USR_PACKET<STANCE_PACKET> packet;
				packet.stance = stance;
				memcpy(packet.objectID, TEMPset.objectIDs, sizeof(packet.objectID));
				packet.userBits = 0;
				packet.init(TEMPset.numObjects);
				NETPACKET->Send(HOSTID, 0, &packet);
			}

			/*
			if (otherSystemID != base->systemID)
			{
				// move to other system
				USR_PACKET<AIMOVE> packet;

				memcpy(packet.objectID, assign->set.objectIDs, sizeof(packet.objectID));
				packet.userBits = 0;	// concat if needed, 0 for cancel current action
				packet.systemID = otherSystemID;
				packet.bUserCommand = 1;
				packet.init(assign->set.numObjects);
				NETPACKET->Send(HOSTID, 0, &packet);
			}
			*/

			USR_PACKET<USRATTACK> packet;
			packet.targetID = obj->GetPartID();

			//packet.bUserGenerated = rand() & 1;
			//packet.bUserGenerated = 0;
			packet.bUserGenerated = bUserGen;

			memcpy(packet.objectID, assign->set.objectIDs, sizeof(packet.objectID));
			packet.userBits = 0;  //0 means cancel current op when this one arrives. Hopefully stance packet not cancelled.
			packet.init(assign->set.numObjects);
			NETPACKET->Send(HOSTID, 0, &packet);

			assign->targetID = obj->GetPartID();
			assign->systemID = obj->GetSystemID();
			assign->type = ATTACK;
			assign->uStallTime = 0;

			//check for special weapons
			m_bAssignsInvalid = true;

			if(DNA.buildMask.bSendTaunts && (rand() & 1))
				SendThreateningRemark(part->playerID);
			
			//recalc next fleet size
			m_AttackWaveSize = CalcWaveSize();
		}
		else
		{
			//CQASSERT(0 && "Can't find strategic target.  Ignorable."); fix 
			if(m_Terminate && (m_Terminate <= MAX_PLAYERS) && (m_UnitTotals[m_Terminate - 1] > 0))
			{
				//seek n destroy
			}
			else
			{
				assign->targetID = 0;
			}
		}
	}
}
//------------------------------------------------------------------------------------------------//
S32 SPlayerAI::CalcWaveSize(void)
{
	S32 res = ((MAX_SELECTED_UNITS / 2) + (rand() & 10));
	return res;
}
//------------------------------------------------------------------------------------------------//
// look for an assignment that needs more help
//
void SPlayerAI::assignGunboat (MPart & part)
{
	//CQASSERT(MGlobals::IsGunboat(part->mObjClass));
	bool bGunboatStillAvailable = false;
	ASSIGNMENT *cur, *node, *prev=0;
	ASSIGNMENTTYPE task = NOASSIGNMENT;

	S32 sightshipclass = 0;
	if(m_nRace == M_TERRAN) sightshipclass = M_INFILTRATOR;
	else if(m_nRace == M_MANTIS) sightshipclass = M_SEEKER;
	else sightshipclass = M_ORACLE;

	U32 delay = 1;//may cause thrashing //UnitsOwned[sightshipclass] + 1 + (m_Age / 128);

	cur = findAssignment(part);
	if(cur) task = cur->type;
	switch(task)
	{
	case SCOUT:
		//check if we have too many scouts and remove from this assignment
		CQASSERT(delay < 10000);
		if((rand() % delay) == 0)
		{
			if ((UnitsOwned[sightshipclass] >= DNA.uNumScouts) &&
				(part->mObjClass != sightshipclass))
			{
				RemoveAssMember(cur, part);
				bGunboatStillAvailable = true;
				break;
			}

			if (doScouting(part) == false)
			{
				//for constant recon patterns
				m_bWorldSeen = true;

				if(part->mObjClass != sightshipclass)
				{
					RemoveAssMember(cur, part);
					bGunboatStillAvailable = true;
				}
				else doScouting(part);
			}
		}
	break;
	case ESCORT:
	{
		//check if this gboat should be moved off escort assignment fix 

		USR_PACKET<USRESCORT> packet;
		packet.targetID = cur->targetID;
		packet.userBits = 0;
		packet.objectID[0] = part->dwMissionID;
		packet.init(1);
		NETPACKET->Send(HOSTID,0,&packet);
	}
	break;
	case DEFEND:
	{
		USR_PACKET<USRESCORT> packet;
		packet.targetID = cur->targetID;
		packet.userBits = 0;
		packet.objectID[0] = part->dwMissionID;
		packet.init(1);
		NETPACKET->Send(HOSTID,0,&packet);
	}
	break;
	case ATTACK:
		//check to see if this gunboat should flee out of the attack assignment  fix  

		//could just return out of here and let an monitorassignments control attack fleet

		//might need stance packet, just in case
		//if(current_stance != US_ATTACK) send stance packet

		if(part->systemID && (part->systemID <= MAX_SYSTEMS) && AssignmentMemberLost(part, cur))
		{
			RemoveAssMember(cur,part);
			cullAssignments();
			bGunboatStillAvailable = true;
		}
		else cur->uStallTime += 2;
			/*  For individual attack packets
			USR_PACKET<USRATTACK> packet;
			packet.targetID = cur->targetID;

			//concentrate on that HQ!  Otherwise, let unit AI pick targets...
			packet.bUserGenerated = (m_bKillHQ && MGlobals::IsHQ(part->mObjClass));  //fix 
			//packet.bUserGenerated = 1;
			
			packet.userBits = 0;  //0 means cancel current op when this one arrives.  Stance packets shouldn't be overridden.
			packet.objectID[0] = part->dwMissionID;
			packet.init(1);
			
			NETPACKET->Send(HOSTID, 0, &packet);
			*/
		break;
	default:
		bGunboatStillAvailable = true;
	break;
	}

	if(!bGunboatStillAvailable) return;

	U32 Dassignmentsleft = getNumAssignments(DEFEND) + getNumAssignments(ATTACK) + getNumAssignments(ESCORT);
	U32 max = MAX_SELECTED_UNITS;
	if(part->admiralID) max--;

	if ((node = pAssignments) != 0)
	{
		do
		{
			//if((node->set.numObjects < max) && 
			//	((!part->admiralID) || (!node->bHasAdmiral)))
			if(node->set.numObjects < max)
			{
				if (node->type == SCOUT && (((node->set.numObjects < 2) &&
					MGlobals::IsLightGunboat(part->mObjClass) && UnitsOwned[sightshipclass] == 0)
					|| part->mObjClass == sightshipclass))
				{
					if (doScouting(part) == 0)
					{
						m_bWorldSeen = true;
						doScouting(part);

						// everything has been explored, remove the scouting assignment CONSTANT SCOUTING NOW
						//if (prev)
						//	prev->pNext = node->pNext;
						//else
						//	pAssignments = node->pNext;
						//delete node;
						//prev=0;
						//goto Top;
					}
					
					// assign the unit
					node->set.objectIDs[node->set.numObjects++] = part->dwMissionID;
					break;
				}
				else
				if (node->type == ESCORT)
				{
					S32 numEscorts = 0;
					if(node->bFabricator) numEscorts = DNA.nFabricateEscorts;
					else numEscorts = DNA.nHarvestEscorts;

					if((((S32)node->set.numObjects) < numEscorts) && 
						MGlobals::IsLightGunboat(part->mObjClass))
					{
						CQASSERT(Dassignmentsleft > 0);
						if(/*Dassignmentsleft && */((rand() % Dassignmentsleft) == false) || 
							(part->mObjClass == M_AURORA))
						{
							node->set.objectIDs[node->set.numObjects++] = part->dwMissionID;
							USR_PACKET<USRESCORT> packet;

							packet.targetID = node->targetID;
							packet.userBits = 0;
							packet.objectID[0] = part->dwMissionID;
							packet.init(1);
							NETPACKET->Send(HOSTID,0,&packet);
							break;
						}
					}
				}
				else
				if (node->type == ATTACK)
				{
					//a target was destroyed....
					//keep attacking, or pull out?  fix  
					//eventually remove the attack assignment?
					//pick new strategic target
					//do nothing and trust tactical?
					//have this particular gunboat join in this attack?

					//attack still running, so perhaps we should join it, if we have supplies
					//and we're in the same system, join in!...? haha
					/*
					if(node->systemID == part->systemID && 
						(((SINGLE)part->supplies) / ((SINGLE)part->supplyPointsMax)) > 0.3))
					{
						//find next target in this system and attack it?
						break;
					}
					*/
				}
				else
				if (node->type == DEFEND)
				{
					CQASSERT(Dassignmentsleft > 0);
					if(/*Dassignmentsleft && */((rand() % Dassignmentsleft) == false))
					{
						CQASSERT(node->set.contains(part->dwMissionID) == 0);
						node->set.objectIDs[node->set.numObjects++] = part->dwMissionID;
						if(part->admiralID && 
							(node->set.contains(part->admiralID) == 0)) 
							node->set.objectIDs[node->set.numObjects++] = part->admiralID;

						if ((DNA.buildMask.bLaunchOffensives==0) || 
							(((S32)node->set.numObjects) < m_AttackWaveSize) ||
							(part->systemID != node->systemID)) 
						{
							///STANCE PACKET  //YEs, this is necessary because stance will override escort code
//							{
//				USR_PACKET<STANCE_PACKET> packet;
//				packet.stance = US_ATTACK;
//				memcpy(packet.objectID, assign->set.objectIDs, sizeof(packet.objectID));
//				packet.userBits = 0;
//				packet.init(assign->set.numObjects);
//				NETPACKET->Send(HOSTID, 0, &packet);
//			}
							/////////////////
							USR_PACKET<USRESCORT> packet;

							packet.targetID = node->targetID;
							packet.userBits = 0;
							packet.objectID[0] = part->dwMissionID;
							packet.init(1);
							NETPACKET->Send(HOSTID,0,&packet);
						}
						else
						{
							if(AllInSystem(node))
								doAttack(node);
						}
						break;
					}
				}
			}
			
			if(node->type == DEFEND || node->type == ATTACK || node->type == ESCORT)
				Dassignmentsleft--;
			
			prev = node;
		}
		while ((node = node->pNext) != 0);
	}
}
//---------------------------------------------------------------------------//
//
void SPlayerAI::ensureAssignmentFor (MPart & part)
{
	ASSIGNMENT * node = 0;

	switch (part->mObjClass)
	{
	case M_HARVEST:
	case M_SIPHON:
	case M_GALIOT:
		if (DNA.nHarvestEscorts && (node = findAssignmentFor(part)) == 0)
		{
			node = new ASSIGNMENT;
			node->Init();
			addAssignment(node);
			node->targetID = part->dwMissionID;
			node->type = ESCORT;
			node->systemID = part->systemID;
			node->militaryPower = 0;
			node->uStallTime = 0;
		}
		break;
	case M_FABRICATOR:
	case M_WEAVER:
	case M_FORGER:
		if (DNA.nFabricateEscorts && (node = findAssignmentFor(part)) == 0)
		{
			node = new ASSIGNMENT;
			node->Init();
			addAssignment(node);
			node->targetID = part->dwMissionID;
			node->type = ESCORT;
			node->systemID = part->systemID;
			node->militaryPower = 0;
			node->uStallTime = 0;

			node->bFabricator = true;
		}
		break;
	case M_HQ:
	case M_COCOON:
	case M_ACROPOLIS:
	case M_HEAVYIND:
	case M_NIAD:
	case M_GREATERPAVILION:
		if (((node = findAssignmentFor(part)) == 0) && (((S32)getNumAssignments(DEFEND)) < DNA.nNumFleets))
		{
			node = new ASSIGNMENT;
			node->Init();
			addAssignment(node);
			node->targetID = part->dwMissionID;
			node->type = DEFEND;
			node->systemID = part->systemID;
			node->militaryPower = 0;
			node->uStallTime = 0;
		}
		break;
	default:
		break;
	}
}
//----------------------------------------------------------------------------------------//
//
void SPlayerAI::doHarvest (MPart & part)
{
	if (part->supplies == 0)
	{
		S32 uo = (S32)(UnitsOwned[M_HARVEST] + UnitsOwned[M_SIPHON] + UnitsOwned[M_GALIOT]);
		if(uo > 20) uo = 20;
		if(uo < 0) uo = 0;
		uo = 5000 - (uo * uo * 23);

		if((m_Gas > uo) && (m_Metal > uo) && (m_ComPtsUsedUp > 75) && ((rand() & 3) == 0))
		{
			if(m_nRace == M_MANTIS)
			{
				//go to dissection chamber instead of the following  fix
				USR_PACKET<USRKILLUNIT> packet;

				packet.objectID[0] = part->dwMissionID;
				packet.init(1);
				CQASSERT(packet.objectID[0]);
				NETPACKET->Send(HOSTID,0,&packet);
			}
			else
			{
				USR_PACKET<USRKILLUNIT> packet;

				packet.objectID[0] = part->dwMissionID;
				packet.init(1);
				CQASSERT(packet.objectID[0]);
				NETPACKET->Send(HOSTID,0,&packet);
			}
		}
		else
		{
			IBaseObject * obj = findHarvestTarget(part);

			if (obj)
			{
				USR_PACKET<USRHARVEST> packet;
				packet.objectID[0] = part->dwMissionID;
				packet.targetID = obj->GetPartID();
				packet.bAutoSelected = false;
				packet.userBits = 0;
				packet.init(1);
				NETPACKET->Send(HOSTID,0,&packet);
			}
		}
	}
}
//---------------------------------------------------------------------------//
//
void SPlayerAI::resetAssignments (void)
{
	ASSIGNMENT * node = pAssignments;    

	while (node)
	{
		pAssignments = node->pNext;
		delete node;
		node = pAssignments;
	}
}
//---------------------------------------------------------------------------//
//
ASSIGNMENT * SPlayerAI::createShadowList (void)
{
	ASSIGNMENT * node, *node2, *result=0;

	if ((node = pAssignments) != 0)
	{
		node2 = result = new ASSIGNMENT;
		node2->Init();
		result->Init();
		node2->type = node->type;

		while ((node = node->pNext) != 0)
		{
			node2->pNext = new ASSIGNMENT;
			node2->pNext->Init(); 
			node2 = node2->pNext;
			node2->type = node->type;
		}
	}

	return result;
}
//---------------------------------------------------------------------------//
//
void SPlayerAI::addPartToUnitsOwned (MPart & part)
{
	M_OBJCLASS c = part->mObjClass;
	if (c < M_ENDOBJCLASS && c >= M_NONE)
	{
		UnitsOwned[c]++;

		if(BuildDesires[c].bMutation)
		{
			int indx = BuildDesires[c].prerequisite;
			if (indx < M_ENDOBJCLASS && indx >= M_NONE) UnitsOwned[indx]++;
		}
	}
}
//---------------------------------------------------------------------------//
//
void SPlayerAI::resetUnitsOwned (void)
{
	memset(UnitsOwned, 0, sizeof(UnitsOwned));
}
//---------------------------------------------------------------------------//
//
void SPlayerAI::addToShadow (U32 dwMissionID, ASSIGNMENT * pShadow)
{
	ASSIGNMENT * node;

	if ((node = pAssignments) != 0)
	{
		do
		{
			if (node->targetID == dwMissionID)
			{
				pShadow->targetID = dwMissionID;
				break;
			}
			else
			if (node->set.contains(dwMissionID) /*&& pShadow->set.numObjects < MAX_SELECTED_UNITS*/)
			{
				pShadow->set.objectIDs[pShadow->set.numObjects++] = dwMissionID;
			}

			pShadow = pShadow->pNext;
		}
		while ((node = node->pNext) != 0);
	}
}
//---------------------------------------------------------------------------//
//
void SPlayerAI::copyShadowData (ASSIGNMENT ** ppShadow)
{
	ASSIGNMENT * pShadow = *ppShadow, *next=0, *node=pAssignments;
	*ppShadow = 0;

	while (pShadow)
	{
		next = pShadow->pNext;
		node->targetID = pShadow->targetID;
		node->set = pShadow->set;
		delete pShadow;

		pShadow = next;
		node = node->pNext;
	}
}
//---------------------------------------------------------------------------//
//
void SPlayerAI::cullAssignments (void)
{
	ASSIGNMENT *node=pAssignments, *prev=0;

	while (node)
	{
		if ( ((node->type == ESCORT || node->type == DEFEND) && node->targetID==0) || 
			 ((node->type == ATTACK) && node->set.numObjects <= 1) )
		{
			if (prev)
				prev->pNext = node->pNext;
			else
				pAssignments = node->pNext;
			delete node;
			node = (prev) ? prev->pNext : pAssignments;
		}
		else
		{
			prev = node;
			node = node->pNext;
		}
	}
}
//------------------------------------------------------------------------------------------------------//
//
void SPlayerAI::RemoveAssMember(ASSIGNMENT * cur, MPart & part)
{
	U32 adID = part->admiralID;
	cur->set.removeObject(part->dwMissionID);
	if(adID)
	{
		cur->set.removeObject(adID);
		cur->bHasAdmiral = false;
	}
}			
//------------------------------------------------------------------------------------------------------//
//
IBaseObject * SPlayerAI::findClosestPlatform (MPart & part, bool (__cdecl *compare )(enum M_OBJCLASS), bool bForRepair)
{
	//
	// find closest planet with an opening
	//
	IBaseObject * obj = OBJLIST->GetTargetList();     // mission objects
	DOUBLE bestDistance=10e8;
	IBaseObject * bestPlatform=0;
	const U32 systemID = part->systemID;
	const GRIDVECTOR position = part.obj->GetGridPosition();
	bool bBestIsVisible=DNA.buildMask.bVisibilityRules && ((rand() & 15) != 0);
	MPart other;
	//U32 * caps = (U32 *) & platCaps;

	while (obj)
	{
		if ((other = obj).isValid() && other.obj->objClass == OC_PLATFORM && 
			/*(((U32 *)&other->caps)[0] & caps[0]) != 0 &&*/
			compare(other->mObjClass) &&
			other->bReady && 
			playerID==other->playerID && 
			((bForRepair == false) || ((other->hullPoints < other->hullPointsMax) && CanIRepair(other))) && 
			1/*check here for things that could cause command to fail upon reception*/)
		{
			const U32 objSystemID = other->systemID;
			bool bIsVisible = obj->IsVisibleToPlayer(playerID);
			DOUBLE distance;

			if (objSystemID == systemID)
				distance = position-obj->GetGridPosition();
			else 
				distance = DISTTONEXTSYS * getNumJumps(systemID, objSystemID, playerID);
				
			if (distance < bestDistance && (bBestIsVisible==false || bIsVisible))
			{
				bestDistance = bestDistance;
				bestPlatform = obj;
				bBestIsVisible = bIsVisible;
			}
		} 
		
		obj = obj->nextTarget;
	} // end while()

	return bestPlatform;
}
//---------------------------------------------------------------------------//
//
IBaseObject * SPlayerAI::findNearbySupplyShip(MPart & part, U32 *fail)
{
	IBaseObject * obj = OBJLIST->GetTargetList();     // mission objects
	DOUBLE bestDistance = 8.0;
	IBaseObject * bestShip=0;
	const U32 systemID = part->systemID;
	const GRIDVECTOR position = part.obj->GetGridPosition();
	
	MPart other;

	while (obj)
	{
		if ((other = obj).isValid() && (playerID==other->playerID) && 
			(systemID == other->systemID) &&
			(MGlobals::IsSupplyShip(other->mObjClass)) &&
			(other->supplies > 20))
		{
			DOUBLE distance = position-obj->GetGridPosition();
			
			if(distance < 2.4) //  fix  needs to be supply range
			{
				*fail = 0xffffffff;
				return obj;
			}
			else if(distance < bestDistance) //  fix  low distance should be okay, it should just keep the ship idle, instead of falling through to findSupplyPlat
			{
				bestDistance = bestDistance;
				bestShip = obj;
			}
		} 
		
		obj = obj->nextTarget;
	} // end while()

	return bestShip;
}
//---------------------------------------------------------------------------//
//
void SPlayerAI::removeFromAssignments (MPart & part)
{
	ASSIGNMENT * node;

	if ((node = pAssignments) != 0)
	{
		do
		{
			RemoveAssMember(node, part);
		}
		while ((node = node->pNext) != 0);
	}
}
//---------------------------------------------------------------------------//
//
ASSIGNMENT * SPlayerAI::findAssignment (MPart & part)
{
	ASSIGNMENT * node;

	if ((node = pAssignments) != 0)
	{
		do
		{
			if (node->set.contains(part->dwMissionID))
				break;
		}
		while ((node = node->pNext) != 0);
	}

	return node;
}
//---------------------------------------------------------------------------//
//
ASSIGNMENT * SPlayerAI::findAssignmentFor (MPart & part)
{
	ASSIGNMENT * node;

	if ((node = pAssignments) != 0)
	{
		do
		{
			if (node->targetID == part->dwMissionID)
				break;
		}
		while ((node = node->pNext) != 0);
	}

	return node;
}
//---------------------------------------------------------------------------//
//
U32 SPlayerAI::getNumJumps (U32 startSystem, U32 endSystem, U32 playerID)
{
	U32 list[MAX_SYSTEMS];
	U32 pathLength = (startSystem==endSystem) ? 0 : (SECTOR->GetShortestPath(startSystem, endSystem, list, playerID) - 1);

	if (pathLength > MAX_SYSTEMS)
		pathLength = MAX_SYSTEMS;

	return pathLength;
}
//---------------------------------------------------------------------------//
//
static U32 insertPosition (NETGRIDVECTOR position[SCOUT_HOPS], DOUBLE distance[SCOUT_HOPS], NETGRIDVECTOR newPos, DOUBLE newDistance, bool bFlip)
{
	int i;
	U32 result=SCOUT_HOPS;

	for (i = 0; i < SCOUT_HOPS; i++)
	{
		if (position[i].systemID == 0)
		{
			position[i] = newPos;
			distance[i] = newDistance;
			result = i+1;
			break;
		}

		bool bD = (newDistance < distance[i]);
		if(bFlip) bD = (newDistance > distance[i]);
		if (bD)
		{
			// push everyone else down
			int j;
			for (j = SCOUT_HOPS-1; j > i ; j--)
			{
				position[j] = position[j-1];
				distance[j] = distance[j-1];
				if (position[j].systemID == 0)
					result = j;
			}

			position[i] = newPos;
			distance[i] = newDistance;
			break;
		}
	}

	return result;
}
//---------------------------------------------------------------------------//
//
U32 SPlayerAI::repeatScoutingRoute (MPart & part, NETGRIDVECTOR positions[SCOUT_HOPS])
{
	U32 result = 0;	
	DOUBLE distances[SCOUT_HOPS];
	//bool bOneJump = false;	// true when one remote jump has been added to list
	memset(positions, 0, sizeof(NETGRIDVECTOR) * SCOUT_HOPS);
	memset(distances, 0, sizeof(distances));

	//U32 systemID = part->systemID;
	Vector position = part.obj->GetPosition();
	bool bResultFound = m_nNumScoutRoutes == 0;

	if(!bResultFound)
	{
		U32 route = rand() % m_nNumScoutRoutes;  //this divisor has already been assured to not be 0

		result = m_ScoutRoutes[route].numhops;
		memcpy(positions, m_ScoutRoutes[route].positions, (sizeof(NETGRIDVECTOR) * SCOUT_HOPS));
		bResultFound = true;
	}

	/*
	while(!bResultFound)
	{
		for(S32 c = 0; c < m_nNumScoutRoutes; c++)
		{
			//best distance 
		}
	}*/

	return result;
}
//---------------------------------------------------------------------------//
//
U32 SPlayerAI::findScoutingRoute (MPart & part, NETGRIDVECTOR positions[SCOUT_HOPS], bool bFlip)
{
	U32 result = 0;	
	DOUBLE distances[SCOUT_HOPS];
	bool bOneJump = false;	// true when one remote jump has been added to list
	memset(positions, 0, sizeof(NETGRIDVECTOR) * SCOUT_HOPS);
	memset(distances, 0, sizeof(distances));

	IBaseObject * obj = 0;// = OBJLIST->GetTargetList();     // mission objects
	U32 systemID = part->systemID;
	Vector position = part.obj->GetPosition();
	GRIDVECTOR partloc = part.obj->GetGridPosition();

	bool bInsertStandardSpot = true;
	bool done = false;
	S32 curSys = systemID;

	//seems only way to get last in list
	IBaseObject *last = 0;
	if(bFlip)
	{
		obj = OBJLIST->GetTargetList();
		while (obj) 
		{
			last = obj;
			obj = obj->nextTarget;  //prevTarget
		}
	}

	while((result == 0) && (!done))
	{
		// scout out this system first
		bool bFirstPass = true;
		U32 fpc = 0;
		while (fpc < 2)
		{
			if(bFlip) obj = last;
			else obj = OBJLIST->GetTargetList();     // mission objects
			while (obj)
			{
				U32 visFlags = obj->GetTrueVisibilityFlags();
				bool reallyVisible = ((visFlags >> (playerID - 1)) & 1);
				bool seen = (m_bWorldSeen && (!reallyVisible));
				bool unknown = ((obj->IsVisibleToPlayer(playerID)) == 0);
				if(seen) bInsertStandardSpot = false;

				if ((((!bFirstPass) && (obj->objClass == OC_JUMPGATE || obj->objClass == OC_PLANETOID)) || (obj->objClass == OC_NEBULA || obj->objClass == OC_FIELD)) && 
					(unknown || seen))
				{
					const U32 objSystemID = obj->GetSystemID();

					bool bCheck;
					if(bFirstPass) bCheck = (((S32)objSystemID) == curSys);
					else bCheck = ((((S32)objSystemID) == curSys) && (obj->objClass == OC_JUMPGATE || obj->objClass == OC_PLANETOID)) || (bOneJump==0 && (getNumJumps(curSys, objSystemID, playerID) == 1) && (bFirstPass == false) && (obj->objClass == OC_NEBULA || obj->objClass == OC_FIELD));

					if (bCheck)
					{
						//if((((S32)objSystemID) == curSys) && (obj->objClass == OC_NEBULA || obj->objClass == OC_FIELD) && !bFirstPass)
						//	break;
						if ((objSystemID != part->systemID) || ((partloc-obj->GetGridPosition()) > 5))
						{						
							NETGRIDVECTOR netvector;
							DOUBLE distance;

							if(!bFirstPass)
							{
								// calculate actual scout position
								RECT rect;
								SECTOR->GetSystemRect(objSystemID, &rect);
								Vector vec;
								OBJBOX box;
								// watch your step!
								const TRANSFORM & trans = obj->GetTransform();
								vec.x = trans.translation.x - (rect.right/2);
								vec.y = trans.translation.y - (rect.top / 2);
								vec.z = 0;
								vec.normalize();
								obj->GetObjectBox(box);
								vec *= (box[0]+500);	//maxx
								vec += trans.translation;

								distance = (position-vec).magnitude();
								netvector.init(vec, objSystemID);
							}
							else
							{
								OBJPTR<IField> field;
								if (obj->QueryInterface(IFieldID, field)!=0)
								{	
									//FIELDCLASS fieldType = field->fieldType;
									Vector vec = field->GetCenterPos();
									distance = (position-vec).magnitude();
									netvector.init(vec, objSystemID);
								}
								else continue;
							}

							if(IsOfflimits(netvector) == false)
							{
								if (objSystemID != systemID)
								{
									bOneJump = true;
									distance += 10e5;
								}

								result = insertPosition(positions, distances, netvector, distance, bFlip);
								if(result == SCOUT_HOPS) break;
							}
						}
					}
				}
				
				if(bFlip) obj = obj->prevTarget;  //hopefully this doesn't cause infinite loops. er.
				else obj = obj->nextTarget;
			} // end while()

			bFirstPass = false;
			fpc++;
		}

		if(result == 0)
		{
			curSys++;
			if((curSys == ((S32)systemID)) || ((systemID == 1) && (curSys > SECTOR->GetNumSystems()))) done = true;
			if(curSys > SECTOR->GetNumSystems()) curSys = 1;
		}
		else
			done = true;
	}

	return result;
}
//----------------------------------------------------------------------------------------//
//
IBaseObject * SPlayerAI::findStrategicTarget (MPart & base, bool bNewTarget, bool bIgnoreDistance)
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
						points = ((DISTTONEXTSYS*4) * getNumJumps(systemID, objSystemID, playerID));
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
					if(m_bKillHQ) points *= 0.4;
					else points *= 0.8;
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
				if(MGlobals::IsShipyard(mobj)) points *= 0.5;
				if(mobj == M_TALOREANMATRIX) points *= 0.4;

				if(MGlobals::IsHeavyGunboat(mobj)) points *= 0.1;
				if(MGlobals::IsMediumGunboat(mobj)) points *= 0.3;

				if(mobj == M_JUMPPLAT)
				{
					points *= 0.5;
					if(systemID == objSystemID)
						points *= 0.4;
				}

				if(mobj == M_IONCANNON || mobj == M_VORAAKCANNON || mobj == M_STARBURST) points *= 0.15;
				/*now solarian deny overrides this entire function
				if((strategy == SOLARIAN_DENY) && ((MGlobals::IsHarvester(mobj)) || (MGlobals::IsFabricator(mobj)) || (mobj == M_JUMPPLAT)))
				{
					DOUBLE ageFactor = ((300.0 - ((DOUBLE)m_Age)) / 30.0);
					if(ageFactor < -29.0) ageFactor = -29.0;
					if(mobj == M_JUMPPLAT)
						points *= 0.8;
					else
						points /= (10.0 + ageFactor);
				}
				*/

				if(!bIsVisible) points *= 4.0;
				if(m_Terminate == hisPlayerID) points /= 30.0;
				
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
void SPlayerAI::AddAllUnits(ASSIGNMENT * assign)
{
	IBaseObject * obj = OBJLIST->GetTargetList();
	MPart part;

	while (obj)
	{
		if ((part = obj).isValid() && (part->playerID == playerID) && 
			MGlobals::IsGunboat(part->mObjClass) &&
			(findAssignment(part) == 0) &&
			(part->bNoAIControl == false))
		{
			assign->set.objectIDs[assign->set.numObjects++] = part->dwMissionID;
			if(assign->set.numObjects == MAX_SELECTED_UNITS) break;
		}

		obj = obj->nextTarget;
	}
}
//----------------------------------------------------------------------------------------//
//
void SPlayerAI::addAssignment (ASSIGNMENT * assign)
{
	ASSIGNMENT * node;
	// add to the end of the list
	if ((node = pAssignments) == 0)
	{
		pAssignments = assign;
	}
	else
	{
		while (node->pNext)
			node = node->pNext;

		node->pNext = assign;
	}

	assign->pNext = 0;
}
//---------------------------------------------------------------------------//
//
bool SPlayerAI::TargetAssigned (U32 target)
{
	ASSIGNMENT * node=pAssignments;
	
	while (node)
	{
		if(/*(node->type == ATTACK) &&*/ (node->targetID == target)) return true;
		node = node->pNext;
	}

	return false;
}
//---------------------------------------------------------------------------//
//
U32 SPlayerAI::getNumAssignments (ASSIGNMENTTYPE atype)
{
	ASSIGNMENT * node=pAssignments;
	U32 result = 0;

	while (node)
	{
		if((atype == NOASSIGNMENT) || (atype == node->type)) result++;
		node = node->pNext;
	}

	return result;
}
//---------------------------------------------------------------------------//
//
ASSIGNMENT * SPlayerAI::findAssignment (ASSIGNMENTTYPE atype)
{
	ASSIGNMENT * node=pAssignments;

	while (node)
	{
		if(atype == node->type) return node;
		node = node->pNext;
	}

	return 0;
}

//---------------------------------------------------------------------------//
//
//---------------------------------------------------------------------------//
IBaseObject * SPlayerAI::findHarvestTarget (MPart & part)
{
	IBaseObject * closest = NULL;
	IBaseObject * bestClosest = NULL;
	GRIDVECTOR position = part.obj->GetGridPosition();
	
	closest = GetNugget(part->systemID, part->systemID, position);
	if(closest) return closest;

	S32 bestNumJumps = 20;
	S32 numsystems = SECTOR->GetNumSystems();
	if(numsystems < 0 || numsystems > 50) numsystems = 0;

	for(U32 c = 0; c < ((U32)numsystems); c++)
	{
		closest = 0;
		U32 g = c + 1;
		if(g != part->systemID) closest = GetNugget(g, part->systemID, position);
		if(closest)
		{
			U32 nugSysID = closest->GetSystemID();
			S32 numJumps = getNumJumps(part->systemID, nugSysID, playerID);
			if(numJumps < bestNumJumps)
			{
				bestNumJumps = numJumps;
				bestClosest = closest;
			}
		}
	}

	if(!bestClosest)
	{
		//the universe has been mined, cease harvesting!  fix  
	}

	return bestClosest;
}
//---------------------------------------------------------------------------//
//
//---------------------------------------------------------------------------//
IBaseObject * SPlayerAI::GetNugget (U32 sysID, U32 harvsysID, GRIDVECTOR position)
{
	IBaseObject * closest = NULL;
	//bool bBestIsVisible = DNA.buildMask.bVisibilityRules;   //   fix   if scouting ever works good, re-enable this
	bool bBestIsVisible = false;

	bool bGas, bMetalMax, bGasMax;
	if(m_Gas > m_Metal) bGas = false;
    else bGas = true;

	if(m_Gas >= ((S32)MGlobals::GetMaxGas(playerID) * (METALGAS_MULT-1))) bGasMax = true;
	else bGasMax = false;
	if(m_Metal >= ((S32)MGlobals::GetMaxMetal(playerID) * (METALGAS_MULT-1))) bMetalMax = true;
	else bMetalMax = false;

	S32 metDivisor, gasDivisor;
	if(m_nRace == M_TERRAN)
	{
		metDivisor = 100;
		gasDivisor = 300;
	}
	else if(m_nRace == M_MANTIS)
	{
		metDivisor = 100;
		gasDivisor = 200;
	}
	else
	{
		metDivisor = 200;
		gasDivisor = 150;
	}
	SINGLE metalMult = (SINGLE)((1000 - m_Metal) / metDivisor);
	SINGLE gasMult = (SINGLE)((1000 - m_Gas) / gasDivisor);
	if(metalMult < 1.0) metalMult = 1.0;
	if(gasMult < 1.0) gasMult = 1.0;

	if(sysID != 0)
	{
		IBaseObject * obj = NUGGETMANAGER->GetFirstNugget(sysID);
		//GRIDVECTOR vect = harvestVector;
		//ObjMapIterator iter(harvestVector.systemID,vect,sensorRadius*GRIDSIZE);
		SINGLE dist = 0;
		while(obj)
		{
			if(obj->objClass == OC_NUGGET)
			{
				if(obj->IsVisibleToPlayer(playerID) || (bBestIsVisible == false))
				{
					OBJPTR<INugget> nugget;
					obj->QueryInterface(INuggetID,nugget);
					if(nugget)
					{
						M_RESOURCE_TYPE rtype = nugget->GetResourceType();
						bool bDontBother = (((rtype == M_GAS) && (m_Gas > 6000)) || ((rtype == M_METAL) && (m_Metal > 6000)) || ((rtype == M_CREW) && (m_Crew > 4000)));
						if((nugget->GetSupplies() > 0) && (bDontBother == false))
						{
							if(closest)
							{
								SINGLE newDist = position - obj->GetGridPosition();
								if(sysID != harvsysID) newDist = 100;

								M_RESOURCE_TYPE rtype = nugget->GetResourceType();
								if(rtype == M_GAS)
								{
									if(bGas && !bGasMax)
										newDist /= 10.0;

									newDist /= gasMult;

									if(bGasMax) newDist = 0;
								}
								if(rtype == M_METAL)
								{
									if(!bGas && !bMetalMax) 
										newDist /= 10.0;

									newDist /= metalMult;

									if(bMetalMax) newDist = 0;
								}

								NETGRIDVECTOR pos;
								pos.init(obj->GetGridPosition(),obj->GetSystemID());
								U32 danger = GetDanger(pos);
								newDist *= ((float)(danger * DNA.nCautiousness + 1.0));

								if(newDist < dist)
								{
									dist = newDist;
									closest = obj;
								}
							}
							else
							{
								closest = obj;
								dist = position - obj->GetGridPosition();
							}
						}
					}
				}
			}
			obj = NUGGETMANAGER->GetNextNugget(obj);
		}
	}

	return closest;
}
//---------------------------------------------------------------------------//
//  important map locations
//---------------------------------------------------------------------------//
void SPlayerAI::AddSpacialPoint(E_POINT_TYPE type, NETGRIDVECTOR pt, U32 range, int objclass, U32 planet, S32 slot, NEBTYPE nebula)
{
	U32 c = 0;

	switch(type)
	{
	case DANGER_POINT:
		if(m_NumDangerPoints < (MAX_DANGER_POINTS - 1)) 
		{
			m_DangerPoints[m_NumDangerPoints].pos = pt;
			m_DangerPoints[m_NumDangerPoints].range = range;
			//m_DangerPoints[m_NumDangerPoints].plattype = range;
			m_NumDangerPoints++;
		}
		else
		{
			//possibly flush danger points here and start over instead of this rand crap  
			int indx = ((rand() % (m_NumDangerPoints - 1)));
			CQASSERT(indx >=0 && indx < MAX_DANGER_POINTS);
			m_DangerPoints[indx].pos = pt;
			m_DangerPoints[indx].range = range;
		}
	break;
	case ION_CLOUD_POINT:
		m_IonCloudPoints[m_NumIonCloudPoints].pos = pt;
		m_IonCloudPoints[m_NumIonCloudPoints].range = range;
		m_IonCloudPoints[m_NumIonCloudPoints].nebulaType = nebula;
		if(m_NumIonCloudPoints < (MAX_IONCLOUD_POINTS - 1)) m_NumIonCloudPoints++;
	break;
	case FAB_POINT:
		for(c = 0; c < m_NumFabPoints; c++)
		{
			if(m_FabPoints[c].fabID == 0)
			{
				m_FabPoints[c].fabID = range;
				m_FabPoints[c].pos = pt;
				m_FabPoints[c].plattype = (M_OBJCLASS)objclass;
				m_FabPoints[c].planet = planet;
				m_FabPoints[c].slot = slot;
				//need dist?
				return;
			}
		}

		m_FabPoints[m_NumFabPoints].pos = pt;
		m_FabPoints[m_NumFabPoints].fabID = range;
		m_FabPoints[m_NumFabPoints].plattype = (M_OBJCLASS)objclass;
		if(m_NumFabPoints < (MAX_DANGER_POINTS - 1)) m_NumFabPoints++;
	break;

	default:
	break;
	}
}
//---------------------------------------------------------------------------//
//  if yer calling this for dangerpoints etc not fabpoints, it won't work unless you
//  add the forloop information above to AddSpacialPoint
//---------------------------------------------------------------------------//
void SPlayerAI::RemoveSpacialPoint(E_POINT_TYPE type, NETGRIDVECTOR pt, U32 range)
{
	U32 c;
	switch(type)
	{
	case DANGER_POINT:
		for(c = 0; c < m_NumDangerPoints; c++)
		{
			if(m_DangerPoints[c].pos == pt)
			{
				m_DangerPoints[c].pos.zero();
				m_DangerPoints[c].range = 0;
			}
		}
	break;
	case ION_CLOUD_POINT:
		for(c = 0; c < m_NumIonCloudPoints; c++)
		{
			if(m_IonCloudPoints[c].pos == pt)
			{
				m_IonCloudPoints[c].pos.zero();
				m_IonCloudPoints[c].range = 0;
				m_IonCloudPoints[c].nebulaType = NEB_ANTIMATTER;
			}
		}
	break;
	case FAB_POINT:
		for(c = 0; c < m_NumFabPoints; c++)
		{
			if(m_FabPoints[c].fabID == range)
			{
				m_FabPoints[c].pos.zero();
				m_FabPoints[c].fabID = 0;
				m_FabPoints[c].plattype = M_NONE;
			}
		}
	break;
	default:
	break;
	}
}

//---------------------------------------------------------------------------//
//  See what our map locations have recorded
//---------------------------------------------------------------------------//
U32 SPlayerAI::GetDanger(NETGRIDVECTOR pt)
{
	U32 danger = 0;
	for(U32 c = 0; c < m_NumDangerPoints; c++)
	{
		NETGRIDVECTOR loc = m_DangerPoints[c].pos;
		U32 size = m_DangerPoints[c].range;

		if(pt.systemID == loc.systemID)
		{
			int x1 = pt.getIntX();
			int x2 = loc.getIntX();
			int y1 = pt.getIntY();
			int y2 = loc.getIntY();

			//optimization: remove sqrt and just do rectangular distance
			U32 dist = (int)sqrt((float)(((x1 - x2) * (x1 - x2)) + ((y1 - y2) * (y1 - y2))));
			if(dist < size) danger++;
		}
	}

	return danger;
}
//---------------------------------------------------------------------------//
//  should i build gunboats or save resource for a platform i need?
//---------------------------------------------------------------------------//
bool SPlayerAI::ReadyForMilitary(void)
{
	//if((UnitsOwned[M_INFILTRATOR] + UnitsOwned[M_SEEKER] + UnitsOwned[M_ORACLE]) == 0)
	//	return true;

	U32 refs = 0;

	switch(m_nRace)
	{
	case M_TERRAN:
		if(!UnitsOwned[M_OUTPOST]) return false;
		if(!UnitsOwned[M_HQ]) return false;
		refs = UnitsOwned[M_REFINERY] + UnitsOwned[M_HARVEST];
		if((refs < 3) && (m_Age < 100)) return false;
		if((UnitsOwned[M_HEAVYIND] < 1) && (UnitsOwned[M_CORVETTE] > 10)) return false;
		break;
	case M_MANTIS:
		//if(!UnitsOwned[M_OUTPOST] && !UnitsOwned[M_ACADEMY]) return false;
		if(!UnitsOwned[M_COCOON]) return false;
		refs = UnitsOwned[M_COLLECTOR] + (UnitsOwned[M_GREATER_COLLECTOR] / 2) + UnitsOwned[M_SIPHON];
		if((refs < 3) && (m_Age < 100)) return false;
		break;
	case M_SOLARIAN:
		if(!UnitsOwned[M_CITADEL] && !UnitsOwned[M_BUNKER]) return false;
		if(!UnitsOwned[M_ACROPOLIS]) return false;
		if(!UnitsOwned[M_OXIDATOR]) return false;
		break;
	default:
		break;
	}

	int resfail = 0, buildfail = 0;
	ChooseNextTech(&resfail);
	ChooseNextBuild(&buildfail, m_nRace, true);

	if((m_Metal < 2250) || (m_Gas < 2250) || (m_Crew < 250))
	{
		if(((buildfail + resfail) > (1750 - (m_TotalMilitaryUnits * m_TotalMilitaryUnits))))
			return false;

		if((rand() % ((m_TotalMilitaryUnits + 1) * 2)) > 15)
			return false;
	}

	S32 mcp = MGlobals::GetMaxControlPoints(playerID);
	CQASSERT((mcp > 75) && (mcp < 2000));
	if(m_ComPtsUsedUp >= (mcp - 1))
		return false;

	return true;
}
//---------------------------------------------------------------------------//
//  should i build gunboats or save resource for a platform i need?
//---------------------------------------------------------------------------//
bool SPlayerAI::ReadyForResearch(void)
{
	U32 refs = 0;

	if((UnitsOwned[M_LIGHTIND] + UnitsOwned[M_HEAVYIND] + UnitsOwned[M_THRIPID] + 
		UnitsOwned[M_PAVILION] + UnitsOwned[M_GREATERPAVILION]) == 0)
		return false;

	switch(m_nRace)
	{
	case M_TERRAN:
		if(!UnitsOwned[M_OUTPOST] && !UnitsOwned[M_ACADEMY]) return false;
		if(!UnitsOwned[M_HQ]) return false;
		//if(!UnitsOwned[M_LRSENSOR]) return false;
		//if(!UnitsOwned[M_LIGHTIND]) return false;
		//if(!UnitsOwned[M_BALLISTICS]) return false;
		//if(!UnitsOwned[M_AWSLAB]) return false;  //completely temporary  fix  (for getting to speciweaps)
		refs = UnitsOwned[M_REFINERY] + (UnitsOwned[M_HEAVYREFINERY]/2) + (UnitsOwned[M_SUPERHEAVYREFINERY]/2);
		if(refs < 2) return false;
		break;
	case M_MANTIS:
		//if(!UnitsOwned[M_OUTPOST] && !UnitsOwned[M_ACADEMY]) return false;
		if(!UnitsOwned[M_COCOON]) return false;
		if(!UnitsOwned[M_EYESTOCK]) return false;
		//if(!UnitsOwned[M_MUTATIONCOLONY]) return false;  //completely temporary  fix  (for getting to speciweaps)
		if(!UnitsOwned[M_THRIPID] && !UnitsOwned[M_NIAD]) return false;
		refs = UnitsOwned[M_COLLECTOR] + UnitsOwned[M_GREATER_COLLECTOR];
		if(refs < 2) return false;
		break;
	case M_SOLARIAN:
		if(!UnitsOwned[M_ACROPOLIS]) return false;
		if(!UnitsOwned[M_OXIDATOR]) return false;
		if(!UnitsOwned[M_BUNKER]) return false;
		if(!UnitsOwned[M_PAVILION]) return false;
		//if(!UnitsOwned[M_XENOCHAMBER]) return false;  //completely temporary  fix  (for getting to speciweaps)
		break;
	default:
		break;
	}

	int resfail = 0, buildfail = 0;
	ChooseNextTech(&resfail);
	ChooseNextBuild(&buildfail, m_nRace, true);

	if((m_Metal < 2250) || (m_Gas < 2250) || (m_Crew < 250))
	{
		if(buildfail > (resfail + (rand() & 127)))
			return false;

		if((rand() % ((m_TotalMilitaryUnits + 1) * 2)) < 13)
			return false;

		S32 numTechs = 1 + UpdateTechStatus(false);
		if((rand() % (numTechs * numTechs)) > 40)
			return false;
	}

	return true;
}
//---------------------------------------------------------------------------//
//  should i build platforms or save resources for gunboats/upgrades/research?
//---------------------------------------------------------------------------//
bool SPlayerAI::ReadyForBuilding(void)
{
	bool res = false;
	M_OBJCLASS start = M_NONE;
	M_OBJCLASS end = M_NONE;
	
	switch(m_nRace)
	{
	case M_TERRAN:
		start = M_HQ;
		end = M_DISPLAB;
		break;
	case M_MANTIS:
		start = M_COCOON;
		end = M_PLASMAHIVE;
		break;
	case M_SOLARIAN:
		start = M_ACROPOLIS;
		end = M_PORTAL;
		break;
	}

	int missing = 0;
	int c = start;
	while(c <= end)
	{
		if((c != M_HEAVYREFINERY) && (c != M_SUPERHEAVYREFINERY))
		{
			if((UnitsOwned[c] == 0) && (BuildDesires[c].bMutation == false))
			{
				res = true;
				missing++;
			}
		}
		c++;
	}

	//force construction to continue when a more dificult AI is chosen.
	if(MGlobals::AdvancedAI(playerID,true))
		res = true;

	if(m_RepairSite) //for HQ games only, currently
		res = false;

	int buildfail = 0;
	int shipfail = 0;
	//ChooseNextTech(&resfail);
	int g = 2;
	int score = 0;
	M_OBJCLASS facil = M_NONE;
	while(g)
	{
		if(m_nRace == M_TERRAN)
		{
			if(g == 2)
				facil = M_LIGHTIND;
			else
				facil = M_HEAVYIND;
		}
		else if (m_nRace == M_MANTIS)
		{
			if(g == 2)
				facil = M_THRIPID;
			else
				facil = M_NIAD;
		}
		else
		{
			if(g == 2)
				facil = M_PAVILION;
			else
				facil = M_GREATERPAVILION;
		}

		ChooseNextShip(&score, m_nRace, facil);
		if(score > shipfail) shipfail = score;

		g--;
	}

	ChooseNextBuild(&buildfail, m_nRace, true);

	if((m_Metal < 2250) || (m_Gas < 2250) || (m_Crew < 250))
	{
		if((buildfail < 110) && ((rand() & 31) != 0))
			return false;

		if((shipfail + m_Age) > (buildfail + ((rand() & 255) * missing)))
			return false;
	}
	/*
	U32 refs = UnitsOwned[M_OUTPOST] + UnitsOwned[M_BUNKER] + UnitsOwned[M_COLLECTOR] + UnitsOwned[M_REFINERY] + UnitsOwned[M_OXIDATOR];
	if(m_Age < 25 && (refs >= 2))
		return false;
	*/

	return res;
}
//---------------------------------------------------------------------------//
//  do i even NEED any more resources?
//---------------------------------------------------------------------------//
bool SPlayerAI::ReadyForHarvest(void)
{
	S32 uo = UnitsOwned[M_HARVEST] + UnitsOwned[M_SIPHON] + UnitsOwned[M_GALIOT];

	if((m_Gas > 2000 - (uo * uo * 20)) && 
		(m_Metal > 2000 - (uo * uo * 20)))
		return false;

	return true;
}
//---------------------------------------------------------------------------//
//  ridiculous hacks
//---------------------------------------------------------------------------//
U32 SPlayerAI::HowManyFabPoints(int oc)
{
	U32 result = 0;
	for(U32 c = 0; c < m_NumFabPoints; c++)
		if(m_FabPoints[c].plattype == ((M_OBJCLASS)oc)) result++;
		
	return result;
}
U32 SPlayerAI::HowManyResearchFabPoints(U32 mID)
{
	U32 result = 0;
	for(U32 c = 0; c < m_NumFabPoints; c++)
		if(m_FabPoints[c].fabID == mID) result++;
		
	return result;
}
BuildSite * SPlayerAI::GetFabPoint(U32 mID)
{
	BuildSite *result = NULL;
	for(U32 c = 0; c < m_NumFabPoints; c++)
	{
		if(m_FabPoints[c].fabID == mID)
		{
			result = &m_FabPoints[c];
			break;
		}
	}
	
	return result;
}
//---------------------------------------------------------------------------//
//  Gimme Race of object class
//---------------------------------------------------------------------------//
M_RACE SPlayerAI::GetRace(int objclass)
{
	int res = 0;

	if(objclass == M_JUMPPLAT) res = m_nRace;
	else if (objclass >= M_ACROPOLIS) res = M_SOLARIAN;
	else if (objclass >= M_COCOON) res = M_MANTIS;
	else if (objclass <= M_CLOAKSTATION) res = M_TERRAN;
	else res = M_NO_RACE;

	return (M_RACE)res;
}
//---------------------------------------------------------------------------//
//  Gimme Race of tech
//---------------------------------------------------------------------------//
M_RACE SPlayerAI::GetTechRace(AI_TECH_TYPE tech)
{
	int res = 0;

	if(tech == 0) res = M_NO_RACE;
	else if (tech >= S_DESTABILIZER) res = M_SOLARIAN;
	else if (tech >= M_REPULSOR) res = M_MANTIS;
	else if (tech <= T_WEAPONUPGRADE4) res = M_TERRAN;
	else tech = NOTECH;

	return (M_RACE)res;
}

//---------------------------------------------------------------------------//
//  Tell me if this object class is a fighting ship
//---------------------------------------------------------------------------//
bool SPlayerAI::IsMilitary(int objclass)
{
	bool res = false;
	int oc = objclass;

	if (((oc >= M_CORVETTE) && (oc <= M_INFILTRATOR)) ||
		((oc >= M_SCOUTCARRIER) && (oc <= M_TIAMAT)) ||
		((oc >= M_TAOS) && (oc <= M_MONOLITH)))
		res = true;

	return res;
}
//---------------------------------------------------------------------------//
//  Tell me if this object class is a fabricator
//---------------------------------------------------------------------------//
bool SPlayerAI::IsFabricator(int objclass)
{
	bool res = false;
	int oc = objclass;
	if ((oc == M_FABRICATOR) || (oc == M_WEAVER) || (oc == M_FORGER)) res = true;
	return res;
}
//---------------------------------------------------------------------------//
bool SPlayerAI::IsOfflimits(NETGRIDVECTOR pos)
{
	for(int c = 0; c < 10; c++)
	{
		if((m_OffLimits[c].loc == pos) && (m_OffLimits[c].hits > 5))
			return true;
	}

	/*
	COMPTR<ITerrainMap> map;
	SECTOR->GetTerrainMap(pos.systemID, map);
	bool bLegal = map->IsGridValid(pos);
	if(!bLegal) return true;
	*/

	return false;
}
//---------------------------------------------------------------------------//
void SPlayerAI::AddOfflimits(NETGRIDVECTOR pos)
{
	bool bAlready = false;
	int c;
	for(c = 0; c < 10; c++)
	{
		if((m_OffLimits[c].loc == pos) && (m_OffLimits[c].loc.systemID == pos.systemID))
		{
			m_OffLimits[c].hits++;
			bAlready = true;
			break;
		}
	}

	if(!bAlready)
	{
		for(c = 0; c < 10; c++)
		{
			if(m_OffLimits[c].hits == 0)
			{
				m_OffLimits[c].hits++;
				m_OffLimits[c].loc = pos;
				bAlready = true;
				break;
			}
		}
	}

	CQASSERT(bAlready);  //ran out of 10 spaces in offlimits-locations array
}
//---------------------------------------------------------------------------//
//  Give me a general, relative power rating for this unit
//---------------------------------------------------------------------------//
S32 SPlayerAI::GetImportance(MPart & part)
{
	S32 res = 0;
	M_OBJCLASS oc = part->mObjClass;

	if(MGlobals::IsPlatform(oc) == false)
	{
		res = 0;
	}
	else if(MGlobals::IsShipyard(oc)) res = 8;
	else if(MGlobals::IsHQ(oc))
	{
		if(m_bKillHQ && UnitsOwned[oc] < 2)
			res = 25;
		else res = 15;
	}
	else if(MGlobals::IsRefinery(oc)) res = GetRefineryImportance(part);
	else res = 6;

	
	return res;
}
//---------------------------------------------------------------------------//
//  Give me a general, relative power rating for this unit
//---------------------------------------------------------------------------//
S32 SPlayerAI::GetRefineryImportance(MPart & part)
{
	//  fix  shouldibuild has code
	S32 res = 5;

	return res;
}

//---------------------------------------------------------------------------//
//  Give me a general, relative power rating for this unit
//---------------------------------------------------------------------------//
S32 SPlayerAI::GetPowerRating(MPart & part)
{
	S32 res = 256;
	M_OBJCLASS mobj = part->mObjClass;

	// fix (add in what type of ship this is to calculations here)
	res += part->hullPoints * 2;
	res += part->hullPointsMax;
	//res += part->numKills * 10;

	if(mobj == M_IONCANNON || mobj == M_VORAAKCANNON) res += 4096;
	else if(MGlobals::IsGunPlat(mobj)) res += 1024;

	if(MGlobals::IsMilitaryShip(mobj)) res += 512;
	if(MGlobals::IsMediumGunboat(mobj)) res += 2048;
	if(MGlobals::IsHeavyGunboat(mobj)) res += 8192;
	
	//res *= part->supplies;

	return res / 16;
}
//---------------------------------------------------------------------------//
//  Give me an offensive power rating for this unit
//---------------------------------------------------------------------------//
U32 SPlayerAI::GetOffensivePowerRating(MPart & part)
{
	U32 res = 0;

	// fix (add in what type of ship this is to calculations here)
	if(IsMilitary(part->mObjClass)) res = 10;

	res += part->hullPointsMax;
	//res += part->numKills * 10;
	
	res *= part->supplies;

	return res;
}
//---------------------------------------------------------------------------//
//  blah
//---------------------------------------------------------------------------//
U32 SPlayerAI::GetTechArcheID(AI_TECH_TYPE i)
{
	U32 aID = 0;

	switch(i)
	{
	case T_BATTLESHIPCHARGE:
		aID = ARCHLIST->GetArchetypeDataID("RESE!!T_BattleshipCharge");
		break;
	case T_CARRIERPROBE:
		aID = ARCHLIST->GetArchetypeDataID("RESE!!T_CarrierProbe");
		break;
	case T_DREADNOUGHTSHIELD:
		aID = ARCHLIST->GetArchetypeDataID("RESE!!T_DreadnoughtShield");
		break;
	case T_ENGINEUPGRADE1:
		aID = ARCHLIST->GetArchetypeDataID("RESE!!T_EngineUpgrade1");
		break;
	case T_ENGINEUPGRADE2:
		aID = ARCHLIST->GetArchetypeDataID("RESE!!T_EngineUpgrade2");
		break;
	case T_ENGINEUPGRADE3:
		aID = ARCHLIST->GetArchetypeDataID("RESE!!T_EngineUpgrade3");
		break;
	case T_ENGINEUPGRADE4:
		aID = ARCHLIST->GetArchetypeDataID("RESE!!T_EngineUpgrade4");
		break;
	case T_FIGHTER1:
		aID = ARCHLIST->GetArchetypeDataID("RESE!!T_Fighter1");
		break;
	case T_FIGHTER2:
		aID = ARCHLIST->GetArchetypeDataID("RESE!!T_Fighter2");
		break;
	case T_FIGHTER3:
		aID = ARCHLIST->GetArchetypeDataID("RESE!!T_Fighter3");
		break;
	case T_GAS1:
		aID = ARCHLIST->GetArchetypeDataID("RESE!!T_Gas1");
		break;
	case T_GAS2:
		aID = ARCHLIST->GetArchetypeDataID("RESE!!T_Gas2");
		break;
	case T_GAS3:
		aID = ARCHLIST->GetArchetypeDataID("RESE!!T_Gas3");
		break;
	case T_HULLUPGRADE1:
		aID = ARCHLIST->GetArchetypeDataID("RESE!!T_HullUpgrade1");
		break;
	case T_HULLUPGRADE2:
		aID = ARCHLIST->GetArchetypeDataID("RESE!!T_HullUpgrade2");
		break;
	case T_HULLUPGRADE3:
		aID = ARCHLIST->GetArchetypeDataID("RESE!!T_HullUpgrade3");
		break;
	case T_HULLUPGRADE4:
		aID = ARCHLIST->GetArchetypeDataID("RESE!!T_HullUpgrade4");
		break;
	case T_LANCERVAMPIRE:
		aID = ARCHLIST->GetArchetypeDataID("RESE!!T_LancerVampire");
		break;
	case T_MISSILECLOAKING:
		aID = ARCHLIST->GetArchetypeDataID("RESE!!T_MissileCloaking");
		break;
	case T_MISSILEPAK1:
		aID = ARCHLIST->GetArchetypeDataID("RESE!!T_MisslePak1");
		break;
	case T_MISSILEPAK2:
		aID = ARCHLIST->GetArchetypeDataID("RESE!!T_MisslePak2");
		break;
	/*
	case T_MISSILEPAK3:
		aID = ARCHLIST->GetArchetypeDataID("RESE!!T_MisslePak3");
		break;
	*/
	case T_ORE1:
		aID = ARCHLIST->GetArchetypeDataID("RESE!!T_Ore1");
		break;
	case T_ORE2:
		aID = ARCHLIST->GetArchetypeDataID("RESE!!T_Ore2");
		break;
	case T_ORE3:
		aID = ARCHLIST->GetArchetypeDataID("RESE!!T_Ore3");
		break;
	case T_SENSOR1:
		aID = ARCHLIST->GetArchetypeDataID("RESE!!T_Sensor1");
		break;
	case T_SENSOR2:
		aID = ARCHLIST->GetArchetypeDataID("RESE!!T_Sensor2");
		break;
	case T_SHIELDUPGRADE1:
		aID = ARCHLIST->GetArchetypeDataID("RESE!!T_ShieldUpgrade1");
		break;
	case T_SHIELDUPGRADE2:
		aID = ARCHLIST->GetArchetypeDataID("RESE!!T_ShieldUpgrade2");
		break;
	case T_SHIELDUPGRADE3:
		aID = ARCHLIST->GetArchetypeDataID("RESE!!T_ShieldUpgrade3");
		break;
	case T_SHIELDUPGRADE4:
		aID = ARCHLIST->GetArchetypeDataID("RESE!!T_ShieldUpgrade4");
		break;
	case T_SUPPLYUPGRADE1:
		aID = ARCHLIST->GetArchetypeDataID("RESE!!T_SupplyUpgrade1");
		break;
	case T_SUPPLYUPGRADE2:
		aID = ARCHLIST->GetArchetypeDataID("RESE!!T_SupplyUpgrade2");
		break;
	case T_SUPPLYUPGRADE3:
		aID = ARCHLIST->GetArchetypeDataID("RESE!!T_SupplyUpgrade3");
		break;
	case T_SUPPLYUPGRADE4:
		aID = ARCHLIST->GetArchetypeDataID("RESE!!T_SupplyUpgrade4");
		break;
	case T_TANKER1:
		aID = ARCHLIST->GetArchetypeDataID("RESE!!T_Tanker1");
		break;
	case T_TANKER2:
		aID = ARCHLIST->GetArchetypeDataID("RESE!!T_Tanker2");
		break;
	case T_TENDER1:
		aID = ARCHLIST->GetArchetypeDataID("RESE!!T_Tender1");
		break;
	case T_TENDER2:
		aID = ARCHLIST->GetArchetypeDataID("RESE!!T_Tender2");
		break;
	case T_TROOPSHIP1:
		aID = ARCHLIST->GetArchetypeDataID("RESE!!T_Troopship1");
		break;
	case T_TROOPSHIP2:
		aID = ARCHLIST->GetArchetypeDataID("RESE!!T_Troopship2");
		break;
	case T_TROOPSHIP3:
		aID = ARCHLIST->GetArchetypeDataID("RESE!!T_Troopship3");
		break;
	case T_WEAPONUPGRADE1:
		aID = ARCHLIST->GetArchetypeDataID("RESE!!T_WeaponUpgrade1");
		break;
	case T_WEAPONUPGRADE2:
		aID = ARCHLIST->GetArchetypeDataID("RESE!!T_WeaponUpgrade2");
		break;
	case T_WEAPONUPGRADE3:
		aID = ARCHLIST->GetArchetypeDataID("RESE!!T_WeaponUpgrade3");
		break;
	case T_WEAPONUPGRADE4:
		aID = ARCHLIST->GetArchetypeDataID("RESE!!T_WeaponUpgrade4");
		break;
	default:
		break;
	}

	switch(i)
	{
	case M_REPULSOR:
		aID = ARCHLIST->GetArchetypeDataID("RESE!!M_Repulser");
		break;
	case M_CAMOFLAGE:
		aID = ARCHLIST->GetArchetypeDataID("RESE!!M_Camoflage");
		break;
	case M_ENGINE1:
		aID = ARCHLIST->GetArchetypeDataID("RESE!!M_Engine1");
		break;
	case M_ENGINE2:
		aID = ARCHLIST->GetArchetypeDataID("RESE!!M_Engine2");
		break;
	case M_ENGINE3:
		aID = ARCHLIST->GetArchetypeDataID("RESE!!M_Engine3");
		break;
	case M_FIGHTER1:
		aID = ARCHLIST->GetArchetypeDataID("RESE!!M_Fighter1");
		break;
	case M_FIGHTER2:
		aID = ARCHLIST->GetArchetypeDataID("RESE!!M_Fighter2");
		break;
	case M_FIGHTER3:
		aID = ARCHLIST->GetArchetypeDataID("RESE!!M_Fighter3");
		break;
	case M_FIGHTER4:
		aID = ARCHLIST->GetArchetypeDataID("RESE!!M_Fighter4");
		break;
	case M_FIGHTER5:
		aID = ARCHLIST->GetArchetypeDataID("RESE!!M_Fighter5");
		break;
	case M_GRAVWELL:
		aID = ARCHLIST->GetArchetypeDataID("RESE!!M_GravWell");
		break;
	case M_HULL1:
		aID = ARCHLIST->GetArchetypeDataID("RESE!!M_Hull1");
		break;
	case M_HULL2:
		aID = ARCHLIST->GetArchetypeDataID("RESE!!M_Hull2");
		break;
	case M_HULL3:
		aID = ARCHLIST->GetArchetypeDataID("RESE!!M_Hull3");
		break;
	case M_LEECH1:
		aID = ARCHLIST->GetArchetypeDataID("RESE!!M_Leech1");
		break;
	case M_LEECH2:
		aID = ARCHLIST->GetArchetypeDataID("RESE!!M_Leech2");
		break;
	case M_RAM1:
		aID = ARCHLIST->GetArchetypeDataID("RESE!!M_Ram1");
		break;
	case M_RAM2:
		aID = ARCHLIST->GetArchetypeDataID("RESE!!M_Ram2");
		break;
	case M_REPELCLOUD:
		aID = ARCHLIST->GetArchetypeDataID("RESE!!M_RepelCloud");
		break;
	case M_SENSOR1:
		aID = ARCHLIST->GetArchetypeDataID("RESE!!M_Sensor1");
		break;
	case M_SENSOR2:
		aID = ARCHLIST->GetArchetypeDataID("RESE!!M_Sensor2");
		break;
	case M_SENSOR3:
		aID = ARCHLIST->GetArchetypeDataID("RESE!!M_Sensor3");
		break;
	case M_SHIELD1:
		aID = ARCHLIST->GetArchetypeDataID("RESE!!M_Shield1");
		break;
	case M_SHIELD2:
		aID = ARCHLIST->GetArchetypeDataID("RESE!!M_Shield2");
		break;
	case M_SHIELD3:
		aID = ARCHLIST->GetArchetypeDataID("RESE!!M_Shield3");
		break;
	case M_SUPPLY1:
		aID = ARCHLIST->GetArchetypeDataID("RESE!!M_Supply1");
		break;
	case M_SUPPLY2:
		aID = ARCHLIST->GetArchetypeDataID("RESE!!M_Supply2");
		break;
	case M_SUPPLY3:
		aID = ARCHLIST->GetArchetypeDataID("RESE!!M_Supply3");
		break;
	case M_TANKER1:
		aID = ARCHLIST->GetArchetypeDataID("RESE!!M_Tanker1");
		break;
	case M_TANKER2:
		aID = ARCHLIST->GetArchetypeDataID("RESE!!M_Tanker2");
		break;
	case M_TANKER3:
		aID = ARCHLIST->GetArchetypeDataID("RESE!!M_Tanker3");
		break;
	case M_TENDER1:
		aID = ARCHLIST->GetArchetypeDataID("RESE!!M_Tender1");
		break;
	case M_TENDER2:
		aID = ARCHLIST->GetArchetypeDataID("RESE!!M_Tender2");
		break;
	case M_TENDER3:
		aID = ARCHLIST->GetArchetypeDataID("RESE!!M_Tender3");
		break;
	case M_TROOPSHIP1:
		aID = ARCHLIST->GetArchetypeDataID("RESE!!M_Leech1");
		break;
	case M_WEAPON1:
		aID = ARCHLIST->GetArchetypeDataID("RESE!!M_Weapon1");
		break;
	case M_WEAPON2:
		aID = ARCHLIST->GetArchetypeDataID("RESE!!M_Weapon2");
		break;
	case M_WEAPON3:
		aID = ARCHLIST->GetArchetypeDataID("RESE!!M_Weapon3");
		break;
	default:
		break;
	}

	switch(i)
	{
	case S_DESTABILIZER:
		aID = ARCHLIST->GetArchetypeDataID("RESE!!S_Destabilizer");
		break;
	case S_CLOAKER:
		aID = ARCHLIST->GetArchetypeDataID("RESE!!S_AuroraCloak"); //  fix  only available string in ADB
		break;
	case S_ENGINE1:
		aID = ARCHLIST->GetArchetypeDataID("RESE!!S_Engine1");
		break;
	case S_ENGINE2:
		aID = ARCHLIST->GetArchetypeDataID("RESE!!S_Engine2");
		break;
	case S_ENGINE3:
		aID = ARCHLIST->GetArchetypeDataID("RESE!!S_Engine3");
		break;
	case S_ENGINE4:
		aID = ARCHLIST->GetArchetypeDataID("RESE!!S_Engine4");
		break;
	case S_ENGINE5:
		aID = ARCHLIST->GetArchetypeDataID("RESE!!S_Engine5");
		break;
	case S_HULL1:
		aID = ARCHLIST->GetArchetypeDataID("RESE!!S_Hull1");
		break;
	case S_HULL2:
		aID = ARCHLIST->GetArchetypeDataID("RESE!!S_Hull2");
		break;
	case S_HULL3:
		aID = ARCHLIST->GetArchetypeDataID("RESE!!S_Hull3");
		break;
	case S_HULL4:
		aID = ARCHLIST->GetArchetypeDataID("RESE!!S_Hull4");
		break;
	case S_HULL5:
		aID = ARCHLIST->GetArchetypeDataID("RESE!!S_Hull5");
		break;
	case S_LEGIONAIRE1:
		aID = ARCHLIST->GetArchetypeDataID("RESE!!S_Legionaire1");
		break;
	case S_LEGIONAIRE2:
		aID = ARCHLIST->GetArchetypeDataID("RESE!!S_Legionaire2");
		break;
	case S_LEGIONAIRE3:
		aID = ARCHLIST->GetArchetypeDataID("RESE!!S_Legionaire3");
		break;
	case S_LEGIONAIRE4:
		aID = ARCHLIST->GetArchetypeDataID("RESE!!S_Legionaire4");
		break;
	case S_MASSDISRUPTOR:
		aID = ARCHLIST->GetArchetypeDataID("RESE!!S_MassDisruptor");
		break;
	case S_ORE1:
		aID = ARCHLIST->GetArchetypeDataID("RESE!!S_Ore1");
		break;
	case S_ORE2:
		aID = ARCHLIST->GetArchetypeDataID("RESE!!S_Ore2");
		break;
	case S_ORE3:
		aID = ARCHLIST->GetArchetypeDataID("RESE!!S_Ore3");
		break;
	case S_GAS1:
		aID = ARCHLIST->GetArchetypeDataID("RESE!!S_Gas1");
		break;
	case S_GAS2:
		aID = ARCHLIST->GetArchetypeDataID("RESE!!S_Gas2");
		break;
	case S_GAS3:
		aID = ARCHLIST->GetArchetypeDataID("RESE!!S_Gas3");
		break;
	case S_TRACTOR:
		aID = ARCHLIST->GetArchetypeDataID("RESE!!S_Tractor");
		break;
	case S_SENSOR1:
		aID = ARCHLIST->GetArchetypeDataID("RESE!!S_Sensor1");
		break;
	case S_SENSOR2:
		aID = ARCHLIST->GetArchetypeDataID("RESE!!S_Sensor2");
		break;
	case S_SENSOR3:
		aID = ARCHLIST->GetArchetypeDataID("RESE!!S_Sensor3");
		break;
	case S_SENSOR4:
		aID = ARCHLIST->GetArchetypeDataID("RESE!!S_Sensor4");
		break;
	case S_SENSOR5:
		aID = ARCHLIST->GetArchetypeDataID("RESE!!S_Sensor5");
		break;
	case S_SHIELD1:
		aID = ARCHLIST->GetArchetypeDataID("RESE!!S_Shield1");
		break;
	case S_SHIELD2:
		aID = ARCHLIST->GetArchetypeDataID("RESE!!S_Shield2");
		break;
	case S_SHIELD3:
		aID = ARCHLIST->GetArchetypeDataID("RESE!!S_Shield3");
		break;
	case S_SHIELD4:
		aID = ARCHLIST->GetArchetypeDataID("RESE!!S_Shield4");
		break;
	case S_SHIELD5:
		aID = ARCHLIST->GetArchetypeDataID("RESE!!S_Shield5");
		break;
	case S_SUPPLY1:
		aID = ARCHLIST->GetArchetypeDataID("RESE!!S_Supply1");
		break;
	case S_SUPPLY2:
		aID = ARCHLIST->GetArchetypeDataID("RESE!!S_Supply2");
		break;
	case S_SUPPLY3:
		aID = ARCHLIST->GetArchetypeDataID("RESE!!S_Supply3");
		break;
	case S_SUPPLY4:
		aID = ARCHLIST->GetArchetypeDataID("RESE!!S_Supply4");
		break;
	case S_SUPPLY5:
		aID = ARCHLIST->GetArchetypeDataID("RESE!!S_Supply5");
		break;
	case S_SYNTHESIS:
		aID = ARCHLIST->GetArchetypeDataID("RESE!!S_Synthesis");
		break;
	case S_TANKER1:
		aID = ARCHLIST->GetArchetypeDataID("RESE!!S_Tanker1");
		break;
	case S_TANKER2:
		aID = ARCHLIST->GetArchetypeDataID("RESE!!S_Tanker2");
		break;
		/*
	case S_TANKER3:
		aID = ARCHLIST->GetArchetypeDataID("RESE!!S_Tanker3");
		break;
	case S_TANKER4:
		aID = ARCHLIST->GetArchetypeDataID("RESE!!S_Tanker4");
		break;
	case S_TANKER5:
		aID = ARCHLIST->GetArchetypeDataID("RESE!!S_Tanker5");
		break;
		*/
	case S_TENDER1:
		aID = ARCHLIST->GetArchetypeDataID("RESE!!S_Tender1");
		break;
	case S_TENDER2:
		aID = ARCHLIST->GetArchetypeDataID("RESE!!S_Tender2");
		break;
	case S_WEAPON1:
		aID = ARCHLIST->GetArchetypeDataID("RESE!!S_Weapon1");
		break;
	case S_WEAPON2:
		aID = ARCHLIST->GetArchetypeDataID("RESE!!S_Weapon2");
		break;
	case S_WEAPON3:
		aID = ARCHLIST->GetArchetypeDataID("RESE!!S_Weapon3");
		break;
	case S_WEAPON4:
		aID = ARCHLIST->GetArchetypeDataID("RESE!!S_Weapon4");
		break;
	case S_WEAPON5:
		aID = ARCHLIST->GetArchetypeDataID("RESE!!S_Weapon5");
		break;

	default:
		break;
	}

	return aID;
}
//---------------------------------------------------------------------------//
//  
bool SPlayerAI::IsAlly(U32 mID)
{
	const U32 allyMask = MGlobals::GetAllyMask(playerID);
	U32 player = (mID & PLAYERID_MASK);
	if(allyMask & (1 << (player-1))) return true;
	else return false;
}
//---------------------------------------------------------------------------//
AI_TECH_TYPE SPlayerAI::CouldResearch(MPart & part)
{
	AI_TECH_TYPE res = NOTECH;
	int bestPri = -10000;

	for(int c = 0; c < AI_TECH_END; c++)
	{
		ResearchPriority *cur = &(ResearchDesires[c]);
		if((cur->facility == part->mObjClass) && cur->bAcquired == false &&
			ResearchDesires[cur->prerequisite].bAcquired &&
			cur->prerequisite != m_WorkingTech)
		{
			if(res != NOTECH)
			{
				if(cur->nPriority > bestPri)
				{
					bestPri = cur->nPriority;
					res = (AI_TECH_TYPE)c;
				}
			}
			else
			{
				bestPri = cur->nPriority;
				res = (AI_TECH_TYPE)c;
			}
		}
	}
	
	return res;
}
//---------------------------------------------------------------------------//
//  does this assignment contain an admiral
//---------------------------------------------------------------------------//
bool SPlayerAI::HasAdmiral(ASSIGNMENT * assign)
{
	assign->bHasAdmiral = false;

	U32 numUnits = assign->set.numObjects;
	if(numUnits < 0) numUnits = 0;
	while(numUnits)
	{
		U32 mID = assign->set.objectIDs[numUnits - 1];
		
		if(TESTADMIRAL(mID))
		{
			assign->bHasAdmiral = true;
			return true;
		}

		numUnits--;
	}

	return false;
}
//---------------------------------------------------------------------------//
//  do i have the tech level
//---------------------------------------------------------------------------//
bool SPlayerAI::DoIHave(AI_TECH_TYPE t)
{
	return ResearchDesires[t].bAcquired;
}
//---------------------------------------------------------------------------//
//  Do I have the resources to build this?
//---------------------------------------------------------------------------//
bool SPlayerAI::CanIBuild(U32 pArcheID, void *fail)
{
	if(pArcheID == 0) return false;

	BASIC_DATA* data = (BASIC_DATA*)ARCHLIST->GetArchetypeData(pArcheID);
	if(!data) return false;

	M_OBJCLASS prereq = M_NONE;

	if(data->objClass == OC_SPACESHIP) 
	{	
		BASE_SPACESHIP_DATA* sdata = (BASE_SPACESHIP_DATA*)data;
		MISSION_DATA* mdata = &sdata->missionData;
		if(!mdata) return false;
		
		if(DNA.buildMask.bUnitDependencyRules && 
			(HasDependencies(mdata->mObjClass, &prereq) == false))
		{		
			*((U32 *)fail) = M_COMMANDPTS + prereq;
			return false;
		}

		//M_RESOURCE_TYPE * failType = NULL
		if(BANKER->HasCost(playerID, sdata->missionData.resourceCost, (M_RESOURCE_TYPE *)fail))
			return true;
	}
	else if(data->objClass == OC_RESEARCH)
	{
		BASE_RESEARCH_DATA * resData = (BASE_RESEARCH_DATA *) (ARCHLIST->GetArchetypeData(pArcheID));
		if(resData)
		{
			if (resData->type == RESEARCH_ADMIRAL || resData->type == RESEARCH_TECH)
			{
				BT_RESEARCH * research = (BT_RESEARCH *) resData;
				if(research)
				{
					TECHNODE tn1, tn2;
					tn1 = MGlobals::GetCurrentTechLevel(playerID);
					tn2 = MGlobals::GetWorkingTechLevel(playerID);

					if(tn1.HasTech(research->researchTech) || tn2.HasTech(research->researchTech))
					{
						//fail type setting unnecessary here
						return false;
					}

					//upgradeID = -2;
					//nodeDepend = research->dependancy;
					//nodeAchieved = research->researchTech;
					//buildCost = research->cost;
					//researchID = ARCHLIST->GetArchetypeDataID(data.rtArchetype);

					if(BANKER->HasCost(playerID, research->cost, (M_RESOURCE_TYPE *)fail))
						return true;
				}
			}
			else  //it's an upgrade/mutation
			{
				BT_UPGRADE * upgrade = (BT_UPGRADE *) resData;
				if(upgrade && BANKER->HasCost(playerID, upgrade->cost, (M_RESOURCE_TYPE *)fail))
					return true;
			}
		}
	}
	else //probably a platform
	{
		BASE_PLATFORM_DATA* sdata = (BASE_PLATFORM_DATA*)data;
		MISSION_DATA* mdata = &sdata->missionData;
		
		if(DNA.buildMask.bUnitDependencyRules && 
			(HasDependencies(mdata->mObjClass, &prereq) == false))
		{	
			*((U32 *)fail) = M_COMMANDPTS + prereq;
			return false;
		}

		if(BANKER->HasCost(playerID, sdata->missionData.resourceCost, (M_RESOURCE_TYPE *)fail))
			return true;
	}

	return false;
}
//---------------------------------------------------------------------------//
//  Do I have the resources to repair this? For PLATFORMS only
//---------------------------------------------------------------------------//
bool SPlayerAI::CanIRepair(MPart & part)
{
	M_OBJCLASS oc = part->mObjClass;
	CQASSERT(oc < M_ENDOBJCLASS);
	U32 pArcheID = m_ArchetypeIDs[oc];

	if(pArcheID == 0) return false;

	BASIC_DATA* data = (BASIC_DATA*)ARCHLIST->GetArchetypeData(pArcheID);
	if(!data) return false;

	if(data->objClass != OC_PLATFORM) return false;
	
	BASE_PLATFORM_DATA* sdata = (BASE_PLATFORM_DATA*)data;
	//MISSION_DATA* mdata = &sdata->missionData;

	U32 curGas = MGlobals::GetCurrentGas(playerID);
	U32 curOre = MGlobals::GetCurrentMetal(playerID);
	U32 curCrew = MGlobals::GetCurrentCrew(playerID);
	U32 gasCost = sdata->missionData.resourceCost.gas;
	U32 oreCost = sdata->missionData.resourceCost.metal;
	U32 crewCost = sdata->missionData.resourceCost.crew;
	
	SINGLE repairAmount = ((((SINGLE)part->hullPointsMax) - ((SINGLE)part->hullPoints)) / ((SINGLE)part->hullPointsMax));
	bool hasGas = (curGas > (gasCost * 0.33 * repairAmount));
	bool hasOre = (curOre > (oreCost * 0.33 * repairAmount));
	bool hasCrew = (curCrew > (crewCost * 0.33 * repairAmount));
	
	if(hasGas && hasOre && hasCrew) return true;

	return false;
}
//---------------------------------------------------------------------------//
//
bool SPlayerAI::HasDependencies(M_OBJCLASS oc, M_OBJCLASS *failtype)
{
	*failtype = M_NONE;

	M_OBJCLASS dep1 = M_NONE;
	M_OBJCLASS dep2 = M_NONE;
	M_OBJCLASS dep3 = M_NONE;

	switch(oc)
	{

	//  fix  add platform dependencies

	//case M_FABRICATOR:
	//case M_HARVESTER:
	case M_SUPPLY:			dep1 = M_TENDER;	dep2 = M_HQ;						break;
	//case M_CORVETTE:
	case M_MISSILECRUISER:	dep1 = M_BALLISTICS;dep2 = M_LIGHTIND;					break;
	case M_TROOPSHIP:		dep1 = M_OUTPOST;	dep2 = M_LIGHTIND;					break;
	case M_INFILTRATOR:		dep1 = M_LRSENSOR;	dep2 = M_LIGHTIND;					break;
	//case M_FLAGSHIP:
	//case M_BATTLESHIP:
	case M_CARRIER:			dep1 = M_HANGER;	dep2 = M_HEAVYIND;					break;
	case M_LANCER:			dep1 = M_PROPLAB;	dep2 = M_DISPLAB; dep3 = M_ACADEMY;	break;
	case M_DREADNOUGHT:		dep1 = M_AWSLAB;	dep2 = M_DISPLAB; dep3 = M_PROPLAB;	break;
	
	case M_SIPHON:			dep1 = M_COLLECTOR;	dep2 = M_COCOON;					break;
	case M_ZORAP:			dep1 = M_COLLECTOR; dep2 = M_PLANTATION;dep3 = M_COCOON;break;
	case M_FRIGATE:			dep1 = M_BLASTFURNACE; dep2 = M_THRIPID;				break;
	case M_KHAMIR:			dep1 = M_BLASTFURNACE; dep2 = M_THRIPID;				break;
	case M_HIVECARRIER:		dep1 = M_CARRIONROOST; dep2 = M_THRIPID;				break;
	case M_TIAMAT: dep1=M_CARRIONROOST; dep2=M_BIOFORGE; dep3=M_DISSECTIONCHAMBER;	break;
	case M_LEECH:			dep1 = M_HYBRIDCENTER; dep2 = M_NIAD;					break;
	case M_SPINELAYER:		dep1 = M_EXPLOSIVESRANGE; dep2 = M_NIAD;				break;
	case M_SCARAB:			dep1 = M_CARPACEPLANT; dep2 = M_NIAD;					break;
	case M_SEEKER:			dep1 = M_EYESTOCK;										break;
	case M_PLASMASPLITTER:	dep1 = M_WARLORDTRAINING;								break;
	case M_VORAAKCANNON:	dep1 = M_MUTATIONCOLONY;								break;
	case M_PLASMAHIVE:		dep1 = M_CARRIONROOST;									break;


	case M_NIAD:			dep1 = M_WARLORDTRAINING; 								break;
	
	case M_GALIOT:			dep1 = M_OXIDATOR;										break;
	case M_STRATUM:			dep1 = M_OXIDATOR;	dep2 = M_EUTROMILL; dep3 = M_ACROPOLIS;break;
	case M_ORACLE:			dep1 = M_SENTINELTOWER;									break;
	//case M_POLARIS:			dep1 = M_GREATERPAVILION;								break;
	case M_POLARIS:			dep1 = M_CITADEL; dep2 = M_PAVILION;					break;
	case M_AURORA:			dep1 = M_HELIONVEIL; dep2 = M_PAVILION; dep3 = M_GREATERPAVILION;	break;
	case M_LEGIONAIRE:		dep1 = M_BUNKER;  dep2 = M_PAVILION;					break;
	case M_TRIREME:			dep1 = M_MUNITIONSANNEX; dep2 = M_TURBINEDOCK; dep3 = M_GREATERPAVILION;break;
	case M_MONOLITH:		dep1 = M_MUNITIONSANNEX; dep2 = M_TURBINEDOCK; dep3 = M_XENOCHAMBER; break;

	case M_STARBURST:		dep1 = M_MUNITIONSANNEX; dep2 = M_TURBINEDOCK;			break;
	case M_HYDROFOIL:		dep1 = M_GREATERPAVILION; dep2 = M_PROTEUS;				break;
	case M_PROTEUS:			dep1 = M_PAVILION;										break;
	case M_ATLAS:			dep1 = M_GREATERPAVILION;								break;

	case M_PAVILION:		dep1 = M_ACROPOLIS; dep2 = M_SENTINELTOWER;				break;
	
	}

	if(dep1 == M_NONE) return true;

	bool result = false;
	if(UnitsOwned[dep1]==0)
		*failtype = dep1;
	else if(dep2 != M_NONE && UnitsOwned[dep2]==0)
		*failtype = dep2;
	else if(dep3 != M_NONE && UnitsOwned[dep3]==0)
		*failtype = dep3;
	else result = true;

	return result;
}
//---------------------------------------------------------------------------//
//
static void AppendName(wchar_t *ptr, U32 tlen, U32 pID)
{
	// get the slotID for this player
	U32 slotID;
	MGlobals::GetSlotIDForPlayerID(pID, &slotID); 

	wchar_t text[MAX_NAME_LENGTH];

	// get the player's name
	MGlobals::GetPlayerNameBySlot(slotID, text, (MAX_NAME_LENGTH-3)*sizeof(wchar_t));

	U32 sName = wcslen(text);
	text[sName++] = '.';
	text[sName++] = ' ';
	text[sName++] = '\0';

	size_t count = CHARCOUNT(text);
	if(count < (MAX_CHAT_LENGTH / 2))
		wcsncpy(&ptr[tlen], text, count);
}
//---------------------------------------------------------------------------//
//  for sending taunts and other messages, max string length is MAX_CHAT_LENGTH
//---------------------------------------------------------------------------//
void SPlayerAI::SendChatMessage(wchar_t *buffer, U8 sendToMask)
{			
	// make sure there is a null character at the end of the string
	U32 sMsg = wcslen(buffer);
	buffer[sMsg++] = '\0';

	// create a packet and send it 
	NETTEXT nt;
	if (sMsg <= MAX_CHAT_LENGTH)
	{
		// get the message we're sending
		wcsncpy(nt.chatText, buffer, sMsg);
		
		// what's the playerID for the computer sending the message
		nt.playerID = playerID;

		// if you want to send the message to everyone the sendToMask will be 0xFF
		nt.toID = sendToMask;

		nt.dwSize = sizeof(NETTEXT) - sizeof(nt.chatText) + sMsg*sizeof(wchar_t);
		NETPACKET->Send(0, NETF_ALLREMOTE, &nt);

		// don't forget to take care of it on our end
		SCROLLTEXT->SetTextStringEx(buffer, playerID);
	}
}
//---------------------------------------------------------------------------//
//
void SPlayerAI::SendThreateningRemark(U32 pID)
{
	if(ShouldIComment())
	{
		wchar_t text[MAX_CHAT_LENGTH];
		for(int c = 0; c < MAX_CHAT_LENGTH; c++) text[c] = 0;

		U32 r = rand() % 3;
		if(m_nRace == M_TERRAN)
		{
			if(r == 0)
				wcsncpy(text, _localLoadStringW(IDS_AI_TERRAN_ATTACK_1), MAX_CHAT_LENGTH);
			else if (r == 1)
			{
				wcsncpy(text, _localLoadStringW(IDS_AI_TERRAN_ATTACK_2), MAX_CHAT_LENGTH);

				AppendName(text, wcslen(text), pID);
			}
			else
				wcsncpy(text, _localLoadStringW(IDS_AI_TERRAN_ATTACK_3), MAX_CHAT_LENGTH);
		}
		else if(m_nRace == M_MANTIS)
		{
			if(r == 2)
				wcsncpy(text, _localLoadStringW(IDS_AI_MANTIS_ATTACK_1), MAX_CHAT_LENGTH);
			else
				wcsncpy(text, _localLoadStringW(IDS_AI_MANTIS_ATTACK_2), MAX_CHAT_LENGTH);
		}
		else
		{
			if(r == 2)
				wcsncpy(text, _localLoadStringW(IDS_AI_CELERY_ATTACK_1), MAX_CHAT_LENGTH);
			else
				wcsncpy(text, _localLoadStringW(IDS_AI_CELERY_ATTACK_2), MAX_CHAT_LENGTH);
		}

		U8 mask = 1 << (pID - 1);
		SendChatMessage(text, mask);
	}
}
//---------------------------------------------------------------------------//
//
void SPlayerAI::SendCelebratoryRemark(U32 pID)
{
	if(ShouldIComment())
	{
		wchar_t text[MAX_CHAT_LENGTH];
		for(int c = 0; c < MAX_CHAT_LENGTH; c++) text[c] = 0;

		U32 r = rand() % 3;
		if(m_nRace == M_TERRAN)
		{
			if(r == 0)
			{
				wcsncpy(text, _localLoadStringW(IDS_AI_TERRAN_CELEBRATE_1), MAX_CHAT_LENGTH/2);
				AppendName(text, wcslen(text), pID);
			}
			else if (r == 1)
			{
				wcsncpy(text, _localLoadStringW(IDS_AI_TERRAN_CELEBRATE_2), MAX_CHAT_LENGTH/2);
				AppendName(text, wcslen(text), pID);
			}
			else
				wcsncpy(text, _localLoadStringW(IDS_AI_TERRAN_CELEBRATE_3), MAX_CHAT_LENGTH);
		}
		else if(m_nRace == M_MANTIS)
		{
			if(r == 2)
				wcsncpy(text, _localLoadStringW(IDS_AI_MANTIS_CELEBRATE_1), MAX_CHAT_LENGTH);
			else
				wcsncpy(text, _localLoadStringW(IDS_AI_MANTIS_CELEBRATE_2), MAX_CHAT_LENGTH);
		}
		else
		{
			if(r == 2)
				wcsncpy(text, _localLoadStringW(IDS_AI_CELERY_CELEBRATE_1), MAX_CHAT_LENGTH);
			else
				wcsncpy(text, _localLoadStringW(IDS_AI_CELERY_CELEBRATE_2), MAX_CHAT_LENGTH);
		}

		U8 mask = 1 << (pID - 1);
		SendChatMessage(text, mask);
	}
}
//---------------------------------------------------------------------------//
//
void SPlayerAI::SendDisparagingRemark(void)
{
	if((rand() & 31) == 10)
	{
		wchar_t text[MAX_CHAT_LENGTH];

		U32 r = rand() % 3;
		if(m_nRace == M_TERRAN)
		{
			if(r == 0)
				wcsncpy(text, _localLoadStringW(IDS_AI_TERRAN_SAD_1), MAX_CHAT_LENGTH);
			else if (r == 1)
				wcsncpy(text, _localLoadStringW(IDS_AI_TERRAN_SAD_2), MAX_CHAT_LENGTH);
			else
				wcsncpy(text, _localLoadStringW(IDS_AI_TERRAN_SAD_3), MAX_CHAT_LENGTH);
		}
		else if(m_nRace == M_MANTIS)
		{
			if(r == 2)
				wcsncpy(text, _localLoadStringW(IDS_AI_MANTIS_SAD_1), MAX_CHAT_LENGTH);
			else
				wcsncpy(text, _localLoadStringW(IDS_AI_MANTIS_SAD_2), MAX_CHAT_LENGTH);
		}
		else
		{
			if(r == 2)
				wcsncpy(text, _localLoadStringW(IDS_AI_CELERY_SAD_1), MAX_CHAT_LENGTH);
			else
				wcsncpy(text, _localLoadStringW(IDS_AI_CELERY_SAD_2), MAX_CHAT_LENGTH);
		}

		U8 mask = 0xff;
		SendChatMessage(text, mask);
	}
}
//---------------------------------------------------------------------------//
//
bool SPlayerAI::ShouldIComment(void)
{
	U32 numPlayers = 0;
	for(U32 c = 0; c < MAX_PLAYERS; c++) if(m_UnitTotals[c] > 0) numPlayers++;

	U32 r = 63;
	if(numPlayers > 3) r = 127;
	if(numPlayers > 6) r = 255;

	if((rand() & r) == 10) return true;
	else return false;
}
//---------------------------------------------------------------------------//
//
const char * SPlayerAI::getSaveLoadName (void) const
{
	const char * result = 0;

	switch (strategy)
	{
	case TERRAN_CORVETTE_RUSH:
		result = "TERRAN_CORVETTE_RUSH";
		break;
	case TERRAN_FORWARD_BUILD:
		result = "TERRAN_FORWARD_BUILD";
		break;
	case TERRAN_DREADNOUGHTS:
		result = "TERRAN_DREADNOUGHTS";
		break;
	case MANTIS_FRIGATE_RUSH:
		result = "MANTIS_FRIGATE_RUSH";
		break;
	case MANTIS_FORTRESS:
		result = "MANTIS_FORTRESS";
		break;
	case MANTIS_FORWARD_BUILD:
		result = "MANTIS_FORWARD_BUILD";
		break;
	case MANTIS_SWARM:
		result = "MANTIS_SWARM";
		break;
	case SOLARIAN_FORWARD_BUILD:
		result = "SOLARIAN_FORWARD_BUILD";
		break;
	case SOLARIAN_FORGERS:
		result = "SOLARIAN_FORGERS";
		break;
	case SOLARIAN_DENY:
		result = "SOLARIAN_DENY";
		break;
	default:
		CQASSERT(0 && "No name for AI type");
		result = "BOGUS";
		break;
	}

	return result;
}
//---------------------------------------------------------------------------//
//  Save / Load functions
//---------------------------------------------------------------------------//
BOOL32 SPlayerAI::Load (struct IFileSystem * inFile) 
{
	DAFILEDESC fdesc;
	COMPTR<IFileSystem> file;
	U32 dwRead, numAssignments;
	BOOL32 result = 0;
	ASSIGNMENT* curass = NULL;

	fdesc.lpFileName = "StrategicAI";
	if (inFile->CreateInstance(&fdesc, file) != GR_OK)
		goto Done;

	file->ReadFile(0,static_cast<PLAYERAI_SAVELOAD *>(this),sizeof(PLAYERAI_SAVELOAD),&dwRead);

	if (m_StrategicTargetID)
		OBJLIST->FindObject(m_StrategicTargetID, playerID, m_StrategicTarget, IBaseObjectID);

	resetAssignments();
	CQASSERT(pAssignments == NULL);

	fdesc.lpFileName = "Assignments";
	if (inFile->CreateInstance(&fdesc, file) != GR_OK)
		goto Done;

	numAssignments = file->GetFileSize() / sizeof(ASSIGNMENT);
	if ((file->GetFileSize() % sizeof(ASSIGNMENT)) != 0)
		goto Done;		// structure size changed!

	while(numAssignments)
	{
		curass = new ASSIGNMENT;
		curass->Init();

		file->ReadFile(0,curass,sizeof(ASSIGNMENT),&dwRead);
		
		curass->pNext = pAssignments;
		pAssignments = curass;
		curass = NULL;
		numAssignments--;
	}

	result = 1;
Done:	
	return result;
}
//---------------------------------------------------------------------------//
//  Save / Load functions
//---------------------------------------------------------------------------//
BOOL32 SPlayerAI::Save (struct IFileSystem * outFile) 
{
	U32 dwWritten;
	COMPTR<IFileSystem> file;
	BOOL32 result = 0;
	DAFILEDESC fdesc;
	ASSIGNMENT* node = pAssignments;
	
	
	fdesc.lpImplementation = "DOS";
	fdesc.dwDesiredAccess = GENERIC_READ|GENERIC_WRITE;
	fdesc.dwShareMode = 0;  // no sharing
	fdesc.dwCreationDistribution = CREATE_ALWAYS;
	
	fdesc.lpFileName = "StrategicAI";
	if (outFile->CreateInstance(&fdesc, file) != GR_OK)
		goto Done;

	file->WriteFile(0,static_cast<PLAYERAI_SAVELOAD *>(this),sizeof(PLAYERAI_SAVELOAD),&dwWritten);

	fdesc.lpFileName = "Assignments";
	if (outFile->CreateInstance(&fdesc, file) != GR_OK)
		goto Done;

	while(node)
	{
		file->WriteFile(0,node,sizeof(ASSIGNMENT),&dwWritten);
		node = node->pNext;
	}

	result = 1;
Done:
	return result;
}
//---------------------------------------------------------------------------//
// resolve pointers
void SPlayerAI::ResolveResolveAssociations (void)
{
	//not saving any pointers yet
}
//---------------------------------------------------------------------------//
//
GENRESULT ISPlayerAI::CreateInstance (const char * type, U32 playerID, ISPlayerAI ** pplayerAI)
{
	*pplayerAI = 0;
	GENRESULT result = GR_OK;

	if (strcmp(type, "TERRAN_CORVETTE_RUSH") == 0)
		CreateCorvetteRushPlayerAI(pplayerAI, playerID, M_TERRAN);
	else
	if (strcmp(type, "TERRAN_FORWARD_BUILD") == 0)
		CreateForwardBuildPlayerAI(pplayerAI, playerID, M_TERRAN);
	else
	if (strcmp(type, "TERRAN_DREADNOUGHTS") == 0)
		CreateDreadnoughtsPlayerAI(pplayerAI, playerID, M_TERRAN);
	else
	if (strcmp(type, "MANTIS_FRIGATE_RUSH") == 0)
		CreateFrigateRushPlayerAI(pplayerAI, playerID, M_MANTIS);
	else
	if (strcmp(type, "SOLARIAN_FORGERS") == 0)
		CreateForgersPlayerAI(pplayerAI, playerID, M_SOLARIAN);
	else
	if (strcmp(type, "SOLARIAN_DENY") == 0)
		CreateDenyPlayerAI(pplayerAI, playerID, M_SOLARIAN);
	else
	if (strcmp(type, "MANTIS_FORTRESS") == 0)
		CreateFortressPlayerAI(pplayerAI, playerID, M_MANTIS);
	else
	if (strcmp(type, "MANTIS_FORWARD_BUILD") == 0)
		CreateStandardMantisPlayerAI(pplayerAI, playerID, M_MANTIS);
	else
	if (strcmp(type, "MANTIS_SWARM") == 0)
		CreateSwarmPlayerAI(pplayerAI, playerID, M_MANTIS);
	else
	if (strcmp(type, "SOLARIAN_FORWARD_BUILD") == 0)
		CreateStandardSolarianPlayerAI(pplayerAI, playerID, M_SOLARIAN);
	else
		result = GR_GENERIC;

	return result;
}
//---------------------------------------------------------------------------//
// TODO: choose a random ai type here!
//
GENRESULT ISPlayerAI::CreateInstance (enum M_RACE race, U32 playerID, ISPlayerAI ** ppPlayerAI)
{
	if(race == M_TERRAN)
	{
		U32 r = rand() % 2;//3!!!!!
		//r = 2;
		switch(r)
		{
		case 0:
			CreateCorvetteRushPlayerAI(ppPlayerAI, playerID, race);
		break;
		case 1:
			CreateDreadnoughtsPlayerAI(ppPlayerAI, playerID, race);
		break;
		case 2:
			CreateForwardBuildPlayerAI(ppPlayerAI, playerID, race);
		break;
		}
	}
	else if(race == M_MANTIS)
	{
		U32 r = rand() % 4;
		//r = 3; //TEMP!!!!!!!!!!!!!!!!
		switch(r)
		{
		case 0:
			CreateStandardMantisPlayerAI(ppPlayerAI, playerID, race);  //Mantis Forward Build
		break;
		case 1:
			CreateFrigateRushPlayerAI(ppPlayerAI, playerID, race);
		break;
		case 2:
			CreateFortressPlayerAI(ppPlayerAI, playerID, race);
		break;
		case 3:
			CreateSwarmPlayerAI(ppPlayerAI, playerID, race);
		break;
		}
	}
	else if(race == M_SOLARIAN)
	{
		U32 r = rand() % 3;
		//r = 1;  //    TEMP!!!!!!!!!!!!!
		switch(r)
		{
		case 1:
			CreateDenyPlayerAI(ppPlayerAI, playerID, race);
		break;
		case 0:
			CreateStandardSolarianPlayerAI(ppPlayerAI, playerID, race);  //Solarian Forward Build
		break;
		case 2:
			CreateForgersPlayerAI(ppPlayerAI, playerID, race);
		break;
		}
	}
	
	if (*ppPlayerAI)
		return GR_OK;
	return GR_GENERIC;
}
//////////////////////////////////////////////////////////////////////////////////////////////
//			           			P U B L I C   H O O K S										//
//////////////////////////////////////////////////////////////////////////////////////////////

//----------------------------------------------------------------------------------------------------//
//  Set traits for this AI, you will have to get an AIPersonality struct, fill it out, and pass it in
//----------------------------------------------------------------------------------------------------//
void SPlayerAI::SetPersonality (const struct AIPersonality & settings)
{
	CQASSERT(DNA.size == settings.size && "AIPersonality is out of sync");
	DNA = settings;

	if(!DNA.buildMask.bBuildPlatforms) DNA.nNumFabricators = 0;
}
//------------------------------------------------------------------------------------------//
//  Force AI to attack whatever object you send in as the parameter, ie a platform or ship
//------------------------------------------------------------------------------------------//
void SPlayerAI::SetStrategicTarget (IBaseObject *obj, U32 range, U32 systemID)
{
	if (obj)
	{
		obj->QueryInterface(IBaseObjectID, m_StrategicTarget, playerID);
		m_StrategicTargetID = obj->GetPartID();
	}
	else
	{
		m_StrategicTarget = 0;
		m_StrategicTargetID = 0;
	}
	m_StrategicTargetRange = range;
	m_StrategicTargetSystem = systemID;
}
//------------------------------------------------------------------------------------------//
//  Send all gunboats owned by this AI to attack enemy
//------------------------------------------------------------------------------------------//
void SPlayerAI::LaunchOffensive (UNIT_STANCE stance)
{
	ASSIGNMENT * node;
	node = new ASSIGNMENT;
	node->Init();
	addAssignment(node);
	
	node->targetID = 0;
	node->systemID = 0;
	node->type = DEFEND;
	node->militaryPower = 0;
	node->uStallTime = 0;

	AddAllUnits(node);
	//CQASSERT1(node->set.numObjects > 0, "Mission script is telling AI to attack with nothing", 0);
	if(node->set.numObjects > 0)
	{
		node->targetID = node->set.objectIDs[0];
		doAttack(node, stance);
	}
}
//------------------------------------------------------------------------------------------//
//  Say whether this AI should do stuff or not.  pass in true for ON, false for OFF
//------------------------------------------------------------------------------------------//
void SPlayerAI::Activate (bool bOnOff)
{
	m_bOnOff = bOnOff;
}
//---------------------------------------------------------------------------//
//---------------------------END SPlayerAI.cpp-------------------------------//
//---------------------------------------------------------------------------//
