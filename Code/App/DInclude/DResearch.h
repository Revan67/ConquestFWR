#ifndef DRESEARCH_H
#define DRESEARCH_H
//--------------------------------------------------------------------------//
//                                                                          //
//                            DResearch.h                                 //
//                                                                          //
//                  COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/DInclude/DResearch.h 9     6/22/01 9:56a Tmauer $
*/			    
//--------------------------------------------------------------------------//
#ifndef DMTECHNODE_H
#include "DMTechNode.h"
#endif

enum RESEARCH_TYPE
{
	RESEARCH_TECH,
	RESEARCH_UPGRADE,
	RESEARCH_ADMIRAL,
	RESEARCH_COMMAND_KIT,
};

enum RESEARCH_SUBTITLE
{
	NO_SUBTITLE = 0,
	SUB_RES_MIMIC = IDS_SUB_RES_MIMIC,
	SUB_RES_ENGINE = IDS_SUB_RES_ENGINE,
	SUB_RES_FIGHTER = IDS_SUB_RES_FIGHTER,
	SUB_RES_GRAVWELL = IDS_SUB_RES_GRAVWELL,
	SUB_RES_HULL = IDS_SUB_RES_HULL,
	SUB_RES_LEECH = IDS_SUB_RES_LEECH,
	SUB_RES_RAM = IDS_SUB_RES_RAM,
	SUB_RES_REPCLOUD = IDS_SUB_RES_REPCLOUD,
	SUB_RES_REP_WAVE = IDS_SUB_RES_REP_WAVE,
	SUB_RES_SENSOR = IDS_SUB_RES_SENSOR,
	SUB_RES_SHIELD = IDS_SUB_RES_SHIELD,
	SUB_RES_SUPPLY = IDS_SUB_RES_SUPPLY,
	SUB_RES_SIPHON = IDS_SUB_RES_SIPHON,
	SUB_RES_RESUPPLY = IDS_SUB_RES_RESUPPLY,
	SUB_RES_WEAPON = IDS_SUB_RES_WEAPON,
	SUB_RES_SHROUD = IDS_SUB_RES_SHROUD,
	SUB_RES_DESTABILIZER = IDS_SUB_RES_DESTABILIZER,
	SUB_RES_GAS = IDS_SUB_RES_GAS,
	SUB_RES_LEGIONAIRE = IDS_SUB_RES_LEGIONAIRE,
	SUB_RES_MASS_DISRUPTOR = IDS_SUB_RES_MASS_DISRUPTOR,
	SUB_RES_ORE = IDS_SUB_RES_ORE,
	SUB_RES_PROTEUS = IDS_SUB_RES_PROTEUS,
	SUB_RES_SYNTH = IDS_SUB_RES_SYNTH,
	SUB_RES_GALIOT = IDS_SUB_RES_GALIOT,
	SUB_RES_AUGER = IDS_SUB_RES_AUGER,
	SUB_RES_TEMPEST = IDS_SUB_RES_TEMPEST,
	SUB_RES_PROBE = IDS_SUB_RES_PROBE,
	SUB_RES_AEGIS = IDS_SUB_RES_AEGIS,
	SUB_RES_VAMP = IDS_SUB_RES_VAMP,
	SUB_RES_CLOAK = IDS_SUB_RES_CLOAK,
	SUB_RES_MISSILE = IDS_SUB_RES_MISSILE,
	SUB_RES_HARVESTER = IDS_SUB_RES_HARVESTER,
	SUB_RES_TROOPSHIP = IDS_SUB_RES_TROOPSHIP,
	SUB_UPGRADE = IDS_SUB_UPGRADE,
};

struct BASE_RESEARCH_DATA : BASIC_DATA
{
	RESEARCH_TYPE type;
	ResourceCost cost;
	U32 time;
};

//----------------------------------------------------------------
//
struct BT_RESEARCH : BASE_RESEARCH_DATA
{
	SINGLE_TECHNODE researchTech;
	SINGLE_TECHNODE dependancy;
	char resFinishedSound[GT_PATH];
	RESEARCH_SUBTITLE resFinishSubtitle;
};
//----------------------------------------------------------------
//
struct BT_UPGRADE : BASE_RESEARCH_DATA
{
	SINGLE_TECHNODE dependancy;
	U32 extensionID;
	char resFinishedSound[GT_PATH];
	RESEARCH_SUBTITLE resFinishSubtitle;
};

//----------------------------------------------------------------
//
struct BT_ADMIRAL_RES : BT_RESEARCH
{
	char flagshipType[GT_PATH];
};
#endif
