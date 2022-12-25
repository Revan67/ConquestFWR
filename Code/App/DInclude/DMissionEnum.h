#ifndef DMISSIONENUM_H
#define DMISSIONENUM_H
//--------------------------------------------------------------------------//
//                                                                          //
//                              DMissionEnum.h                              //
//                                                                          //
//                  COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/DInclude/DMissionEnum.h 41    7/19/01 1:43p Tmauer $
*/			    
//--------------------------------------------------------------------------//

enum M_RACE
{
	M_NO_RACE,
	M_TERRAN,
	M_MANTIS,
	M_SOLARIAN,
	M_VYRIUM
};

enum M_ARTIFACT
{
	M_NO_ARTIFACT
};

enum M_RESOURCE_TYPE
{
	M_GAS,
	M_METAL,
	M_CREW,
	M_COMMANDPTS
};

//changing the order of these M_OBJCLASSes will break many things
enum M_OBJCLASS
{
	M_NONE=0,
	//
	// terran ships
	//
	M_FABRICATOR=1,
	M_SUPPLY,
	M_REPAIR_SHIP,

	M_CORVETTE,				//Beginning of Terran gunboat classification
	M_MISSILECRUISER,
	M_BATTLESHIP,			//
	M_DREADNOUGHT,
	M_CARRIER,
	M_LANCER,
	M_INFILTRATOR,			//End of Terran gunboat classification

	//hybrid ships
	M_ESPIONAGE,

	M_HARVEST,				//
	M_RECONPROBE,
	M_TROOPSHIP,
	M_FLAGSHIP,

	M_HQ,
	M_REFINERY,				//
	M_LIGHTIND,
	M_TENDER,
	M_REPAIR,
	M_OUTPOST,
	M_ACADEMY,				//
	M_BALLISTICS,			
	M_ADVHULL,
	M_HEAVYREFINERY,
	M_SUPERHEAVYREFINERY,
	M_REMOTE_HQ,

	//moon platforms
	M_NOVA_BOMB,
	M_INTEL_CENTER,
	M_T_RESOURCE_FACTORY,
	M_T_RESEARCH_LAB,
	M_T_IND_FACILITY,

	//custom tech

	M_LISTENING_DEVICE,
	M_BLOCKADE_RUNNER,
	M_EARTH_GENESIS_DEVICE,

	// floating platforms
	M_LSAT,					//
	M_SPACESTATION,			
	
	M_LRSENSOR,				
	M_AWSLAB,
	M_IONCANNON,
	M_HEAVYIND,				//
	M_HANGER,				
	M_PROPLAB,
	M_DISPLAB,

	M_CLOAKSTATION,

	//
	// non-race related items...
	//
	M_PLANET,				//
	M_FIELD,				
	M_DEBRIS,
	M_WAYPOINT,
	M_JUMPGATE,
	M_MINEFIELD,			//
	M_NUGGET,				
	M_NEBULA,
	M_ASTEROID_FIELD,
	M_JUMPPLAT,				// gate inhibitor, used by all races

	//
	//Mantis Plats
	//
	M_COCOON,				//
	M_COLLECTOR,			
	M_GREATER_COLLECTOR,
	M_PLANTATION,
	M_GREATER_PLANTATION,
	M_EYESTOCK,				//
	M_THRIPID,				
	M_WARLORDTRAINING,
	M_BLASTFURNACE,
	M_EXPLOSIVESRANGE,
	M_PLASMASPLITTER,		//
	M_CARRIONROOST,			
	M_VORAAKCANNON,
	M_MUTATIONCOLONY,
	M_NIAD,
	M_BIOFORGE,				//
	M_FUSIONMILL,			
	M_CARPACEPLANT,
	M_DISSECTIONCHAMBER,
	M_HYBRIDCENTER,
	M_PLASMAHIVE,			//
	M_FRONTIER_HIVE,

	//moon platforms
	M_M_RESOURCE_FACTORY,
	M_M_RESEARCH_LAB,
	M_M_IND_FACILITY,
	M_HEAL_CLOUD,
	M_HIVE,

	M_WEAVER,			// m fabricator				
	M_SPINELAYER,		// m minelayer						
	M_SIPHON,			// m harvester
	M_ZORAP,			// m supply ship
	M_LEECH,			// m troop ship				//
	M_SEEKER,			// m sight ship					//Beginning of Mantis Gunboat classification
	M_SCOUTCARRIER,		// m corvette/carrier				
	M_FRIGATE,			// m more powerful corvette
	M_KHAMIR,			// m ram ship (suicide)
	M_HIVECARRIER,		// m medium carrier			//
	M_SCARAB,			// m battleship				
	M_TIAMAT,			// m heavy carrier (best ship)		//End of Mantis Gunboat classification
	M_WARLORD,			// m admiral

	//
	// Solarian stuff
	//

	M_ACROPOLIS,
	M_OXIDATOR,				//
	M_PAVILION,				
	M_SENTINELTOWER,
	M_EUTROMILL,
	M_GREATERPAVILION,
	M_HELIONVEIL,			//
	M_CITADEL,				
	M_XENOCHAMBER,
	M_ANVIL,
	M_MUNITIONSANNEX,
	M_TURBINEDOCK,			//
	M_TALOREANMATRIX,			
	M_BUNKER,			
	M_PARTHENON,

	//moon platforms
	M_S_RESOURCE_FACTORY,
	M_S_RESEARCH_LAB,
	M_S_IND_FACILITY,
	M_CLOAK_PLATFORM,

	// floating platforms
	M_PROTEUS,
	M_HYDROFOIL,
	M_ESPCOIL,				//
	M_STARBURST,		
	M_PORTAL,

	M_FORGER,			// s fabricator	
	M_STRATUM,			// s supply shi
	M_GALIOT,			// s harvester		//
	M_ATLAS,			// s mine layer		
	M_LEGIONAIRE,		// s troopship
	M_TAOS,				// s corvette						//Beginning of Solarian gunboat classification
	M_POLARIS,			// s amazing
	M_AURORA,			// s missle cruiser		//
	M_ORACLE,			// s sight ship			
	M_TRIREME,			// s more amazing, perhaps battleship
	M_MONOLITH,			// s big kick-ass thing				//End of Solarian gunboat classification
	M_HIGHCOUNSEL,		// s admiral

	//
	// Vyrium Stuff
	//

	//platforms

	M_LOCUS,
	M_COALESCER,
	M_HATCHERY,
	M_COCHLEA_DISH,
	M_COMPILER,
	M_FORMULATOR,
	M_GUDGEON,
	M_REPOSITORIUM,
	M_PERPETUATOR,
	M_CLAW_OF_VYRIE,
	M_EYE_OF_VYRIE,
	M_TEMPLE_OF_VYRIE,
	M_HAMMER_OF_VYRIE,
	M_SHIELD_OF_VYRIE,
	M_CYNOSURE,

	//moon platforms
	M_V_RESOURCE_FACTORY,
	M_V_RESEARCH_LAB,
	M_V_IND_FACILITY,
	M_SPACE_FOLDER,
	M_GRAVITY_CENTER,

	//floating
	M_SINUATOR,

	//ships
	M_SHAPER,			// v fabricator
	M_ANACONDA,
	M_AGGREGATOR,
	M_MYSTIC,
	M_VIPER,
	M_ADDER,
	M_NECTROP,
	M_ERTRAG,
	M_MOK,
	M_COBRA,
	M_CROTAL,
	M_BASILISK,
	M_LEVIATHIN,

	M_ENDOBJCLASS			// this must be at the end!
};
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
enum UNIT_STANCE
{
	US_DEFEND = 0,
	US_ATTACK,
	US_STAND,
	US_STOP
};
//--------------------------------------------------------------------------//
//
enum HARVEST_STANCE 
{
	HS_NO_STANCE = 0,
	HS_GAS_HARVEST,
	HS_ORE_HARVEST,
};
//--------------------------------------------------------------------------//
//
enum ADMIRAL_TACTIC
{
	AT_PEACE = 0,
	AT_DEFEND,
	AT_HOLD,
	AT_SEEK
};
//--------------------------------------------------------------------------//
//
enum FighterStance
{
	FS_NORMAL = 0,
	FS_PATROL,
};
//--------------------------------------------------------------------------//
//
enum UNIT_SPECIAL_ABILITY
{
	USA_NONE,
	USA_ASSAULT,
	USA_TEMPEST,
	USA_PROBE,
	USA_CLOAK,
	USA_AEGIS,
	USA_VAMPIRE,
	USA_STASIS,
	USA_FURYRAM,
	USA_REPEL,
	USA_BOMBER,
	USA_MIMIC,
	USA_MINELAYER,
	USA_S_MINELAYER,
	USA_REPULSOR,
	USA_SYNTHESIS,
	USA_MASS_DISRUPTOR,
	USA_DESTABILIZER,
	USA_WORMHOLE,
	USA_PING,
	USA_M_PING,
	USA_S_PING,
	USA_MULTICLOAK,
	USA_TRACTOR,
	USA_JUMP,
	USA_ARTILERY,
	USA_TRACTOR_WAVE,
	USA_NOVA,
	USA_SHIELD_JAM,
	USA_WEAPON_JAM,
	USA_SENSOR_JAM,
	//...

	USA_DOCK,
	USA_LAST	// this must be at the
};
//--------------------------------------------------------------------------//
//------------------------End DMissionEnum.h--------------------------------//
//--------------------------------------------------------------------------//

#endif
