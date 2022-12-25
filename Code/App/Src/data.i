











			    



















typedef signed   long    BOOL32;
typedef unsigned char    U8 ;
typedef unsigned short   U16;
typedef unsigned long    U32;
typedef          char    C8 ;
typedef signed   char    S8 ;
typedef signed   short   S16;
typedef signed   long    S32;






















typedef unsigned __int64 U64;
typedef signed   __int64 S64;










typedef float           SINGLE;
typedef double          DOUBLE;








typedef union
{
   S32    i;
   U32    u;
   SINGLE f;
   void  *p;
}
HYBRID;















































			    



enum OBJCLASS
{
	OC_NONE			=  0x00000000,
	OC_SPACESHIP	=  0x00000001,
	OC_PLANETOID	=  0x00000002,
	OC_MEXPLODE		=  0x00000004,
	OC_SHRAPNEL		=  0x00000008,
	OC_JUMPGATE		=  0x00000010,
	OC_NEBULA		=  0x00000020,
	OC_LAUNCHER		=  0x00000040,
	OC_WEAPON       =  0x00000080,
	OC_BLAST		=  0x00000100,
	OC_WAYPOINT		=  0x00000200,
	OC_PLATFORM		=  0x00000400,
	OC_FIGHTER		=  0x00000800,
	OC_LIGHT		=  0x00001000,
	OC_MINEFIELD    =  0x00002000,
	OC_TRAIL		=  0x00004000,
	OC_EFFECT		=  0x00008000,
	OC_FIELD		=  0x00010000,
	OC_NUGGET		=  0x00020000,
	OC_GROUP		=  0x00040000,
	OC_RESEARCH		=  0x00080000,
	OC_BLACKHOLE    =  0x00100000,
	OC_PLAYERBOMB	=  0x00200000,
	OC_BUILDRING	=  0x00400000,
	OC_BUILDOBJ		=  0x00800000,
	OC_MOVIECAMERA	=  0x01000000,
	OC_OBJECT_GENERATOR = 0x02000000,
	OC_TRIGGER		=  0x04000000,
	OC_SCRIPTOBJECT =  0x08000000,
	OC_UI_ANIM		=  0x10000000
};
























enum WPNCLASS
{
	WPN_MISSILE=1,
	WPN_BOLT,
	WPN_BEAM,
	WPN_ARC,
	WPN_AUTOCANNON,
	WPN_SPECIALBOLT,
	WPN_TRACTOR,
	WPN_OVERDRIVE,
	WPN_REPULSOR,
	WPN_SWAPPER,
	WPN_PLASMABOLT,
	WPN_ANMBOLT,
	WPN_AEBOLT,
	WPN_AEGIS,
	WPN_MIMIC,
	WPN_ZEALOT,
	WPN_STASISBOLT,
	WPN_REPELLENTCLOUD,
	WPN_SYNTHESIS,
	WPN_MASS_DISRUPTOR,
	WPN_DESTABILIZER,
	WPN_GATTLINGBEAM,
	WPN_LASERSPRAY,
	WPN_WORMHOLE,
	WPN_REPULSORWAVE,
	WPN_DUMBRECONPROBE,
	WPN_SPACEWAVE,
	WPN_PKBOLT,
	WPN_ART_BUFF,
	WPN_ART_LISTEN,
	WPN_ART_TERRAFORM,
};





enum LAUNCHCLASS
{
	LC_TURRET=1,
	LC_VERTICLE_LAUNCH,
	LC_MISSILE,
	LC_AIR_DEFENSE,
	LC_MINELAYER,
	LC_FIGHTER_WING,
	LC_KAMIKAZE_WING,
	LC_SHIPLAUNCH,
	LC_FANCY_LAUNCH,
	LC_CLOAK_LAUNCH,
	LC_RECON_LAUNCH,
	LC_WORMHOLE_LAUNCH,
	LC_PING_LAUNCH,
	LC_MULTICLOAK_LAUNCH,
	LC_TALORIAN_LAUNCH,
	LC_JUMP_LAUNCH,
	LC_ARTILERY_LAUNCH,
	LC_BUFF_LAUNCHER,
	LC_TRACTOR_WAVE_LAUNCH,
	LC_BARRAGE_LAUNCH,
	LC_REPAIR_LAUNCH,
	LC_MOON_RESOURCE_LAUNCH,
	LC_SYSTEM_BUFF_LAUNCHER,
	LC_NOVA_LAUNCH,
	LC_TEMPHQ_LAUNCHER,
	LC_ARTIFACT_LAUNCHER,
	LC_EFFECT_LAUNCHER,
};





enum SPACESHIPCLASS
{
	SSC_GUNBOAT=1,
	SSC_BUILDSHIP,
	SSC_HARVESTSHIP,
	SSC_FABRICATOR,
	SSC_MINELAYER,
	SSC_RECONPROBE,
	SSC_TORPEDO,
	SSC_GUNSAT,
	SSC_FLAGSHIP,
	SSC_SUPPLYSHIP,
	SSC_TROOPSHIP,
	SSC_RECOVERYSHIP,
	SSC_SPIDERDRONE,
	SSC_TERRANDRONE
};


enum PLATFORMCLASS
{
	PC_OLDSTYLE,
	PC_GUNPLAT=1,
	PC_SUPPLYPLAT,
	PC_BUILDPLAT,
	PC_REPAIRPLAT,
	PC_REFINERY,
	PC_GENERAL,
	PC_BUILDSUPPLAT,
	PC_JUMPPLAT,
	PC_SELL
};



enum EFFECTCLASS
{
	FX_FIREBALL=1,
	FX_PARTICLE,
	FX_STREAK,
	FX_SPARK,
	FX_ANIMOBJ,
	FX_CLOAKEFFECT,
	FX_ENGINETRAIL,
	FX_WORMHOLE_BLAST,
	FX_TALORIAN_EFFECT,
	FX_TROOPPOD,
	FX_PARTICLE_CIRCLE,
	FX_NOVA_EXPLOSION,
};




enum FIELDCLASS
{
	FC_ASTEROIDFIELD=1,
	FC_MINEFIELD,
	FC_NEBULA,
	FC_ANTIMATTER
};




enum BUILDOBJCLASS
{
	BO_TERRAN,
	BO_MANTIS,
	BO_SOLARIAN,
	BO_VYRIUM
};



typedef U16 M_PART_ID;
























			    




















			    






























namespace SFX
{

enum ID
{
	INVALID=0,
	
	BUTTONPRESS1,
	BUTTONPRESS2,
	inRange, 
	startConstruction, 
	endConstruction, 
	zeroMoney, 
	lightindustryButton, 
	heavyindustryButton, 
	hitechindustryButton, 
	hqButton, 
	researchButton, 
	planetDepleted,
	harvestRedeploy, 
	researchCompleted,

	JG_ENTER1,
	JG_ENTER2,
	JG_ARRIVE1,
	JG_ARRIVE2,
	JG_AMBIENCE,
	
	
	T_ARC_CANNON,
	T_DEFENSEGUN1,
	T_SHIELDHIT,
	T_SHIELDFIZZOUT,
	T_SHIELDFIZZIN,
	T_LASERTURRET_SML,
	T_CORVETTE_IMPACT,
	T_LASERTURRET_MED,
	T_LASERTURRET_LGE,
	T_MISSILE,
	T_FLYBY1,
	T_FIGHTERFLYBY1,
	T_FIGHTERFLYBY2,
	T_FIGHTERFLYBY3,
	T_FIGHTERFLYBY4,
	T_FIGHTERLAUNCH1,
	T_FIGHTERLAUNCH2,
	T_FIGHTERLAUNCH3,
	T_FIGHTERLAUNCH4,
	T_CONSTRUCTION,
	T_POWERUP,
	T_LSATFIRE,
	T_LSATDEPLOY,
	T_FIGHTEREXPLO,
	T_HARVESTERUNLOAD,
	T_IMPACTCORVETTE,
	T_IMPACTLSAT,
	T_TROOPRELEASE,
	T_IMPACTMISSILE,
	T_IMPACTFIGHTER,
	T_FIGHTERFIRE1,
	T_FIGHTERFIRE2,
	T_FIGHTERFIRE3,
	T_FIGHTERFIRE4,
	T_PROBELAUNCH,
	T_FIGHTERKAMIKAZE,
	T_TEMPESTLAUNCH,
	T_IMPACTDREADNOUGHT,
	T_GRAVWELL,
	T_TRACTORDEBRIS,
	T_PROBEFIZZLE,
	T_AEGISSHIELD,
	T_IONCHARGE,
	T_TROOPASSAULT,

	
	M_FIGHTERLAUNCH1,
	M_FIGHTERLAUNCH2,
	M_FIGHTERLAUNCH3,
	M_FIGHTERLAUNCH4,
	M_FIGHTERFLYBY1,
	M_FIGHTERFLYBY2,
	M_FIGHTERFLYBY3,
	M_FIGHTERFLYBY4,
	M_PLASMASMALL,
	M_PLASMAMED,
	M_CONSTRUCTION,
	M_FIGHTERBLAST,
	M_PLASMACHARGE,
	M_SELLINGUNIT,
	M_SHIELDHIT,
	M_REPELLCLOUD,
	S_CONSTRUCTION,
	S_BALLENERGY,
	S_BEAM_LG,
	S_BEAM_MED,
	S_DESTABILIZER,
	S_DISRUPTORDAM,
	S_ECLIPSE,
	S_FORGERDEPLOY,
	S_LAUNCHDESTAB,
	S_LAUNCHDISRUP,
	S_LEGIONAIRE,
	S_LEGIONSUCCESS,
	S_MINELAYER,
	S_REPULSOR,
	S_SELLINGUNIT,
	S_STRATUM,
	S_SYNTHESIS,
	S_UNLOAD,
	S_BEAM_MED2,
	S_SHIELDHIT,
	S_BEAM_SML,

	
	NEBULA_ION,
	NEBULA_IONSPARK,
	NEBULA_PLASMA,
	NEBULA_ARGON,
	NEBULA_HELIOUS,
	NEBULA_HYADES,
	ASTEROID_FIELD,
	NEBULA_CELSIUS,
	NEBULA_CYGNUS,
	NEBULA_LITHIUM,
	
		
	
	EXPT_L_SUB,
	EXPT_L_FINAL,
	EXPT_M_SUB,
	EXPT_M_FINAL,
	EXPT_S_SUB,
	EXPT_S_FINAL,
	EXPM_RIPPLE1,
	EXPM_RIPPLE2,
	EXPM_RIPPLE3,
	EXPM_RIPPLE4,
	EXPM_L_SUB,
	EXPM_L_FINAL,
	EXPM_M_SUB,
	EXPM_M_FINAL,
	EXPM_S_SUB,
	EXPM_S_FINAL,
	EXPS_L_SUB,
	EXPS_L_FINAL,
	EXPS_M_SUB,
	EXPS_M_FINAL,
	EXPS_S_SUB,
	EXPS_S_FINAL,
	CONFIRMATION,
	EXPLOSION,
	EXPLOSION1,
	EXPLOSION2,
	EXPLOSION3,
	EXPLOSION4,
	SECEXPLODE1,
	SECEXPLODE2,
	SECEXPLODE3,
	SECEXPLODE4,
	SMALLEXPLOSION,
	TELETYPE,
	SPECIALDENIED,
	BEACON,

	LAST	
};

}  






























			    










class Vector
{
	SINGLE x,y,z;
};

class Matrix
{
	SINGLE d[3][3];
};

class Transform	: Matrix	
{
	Vector translation;
};

struct GRIDVECTOR
{
	U8 x, y;
};

struct NETGRIDVECTOR
{
	U8 x, y;
	U8 systemID;
};


typedef U32 NETVECTOR;

typedef Transform TRANSFORM;
typedef long LONG;

typedef struct tagRECT
{
    LONG    left;
    LONG    top;
    LONG    right;
    LONG    bottom;
} RECT;

typedef char M_STRING[32];
typedef unsigned short wchar_t;
typedef wchar_t M_STRINGW[32];











































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































			    






namespace OBJNAMES
{
enum M_DISPLAY_NAME
{
	MT_NO_DISPLAY_NAME = 0,
	
	MT_HEADQUARTERS =		303,
	MT_LIGHT_INDUSTRIAL =	454,
	MT_HEAVY_INDUSTRIAL =	455,
	MT_HITECH_INDUSTRIAL =	456,
	MT_RESEARCH_FACILITY =	459,
	MT_REFINERY =			458,
	MT_TENDER =				538,
	MT_OUTPOST =			539,
	MT_TURRET =				540,
	MT_ACADEMY =			541,
	MT_PROPLAB =			390,
	MT_ADVHULL =			543,
	MT_REPAIR =				544,
	MT_SRSENSOR =			545,
	MT_AWSLAB =				546,
	MT_DEFENSEBAT =			547,
	MT_MRSENSOR =			548,
	MT_LOGISTICS =			549,
	MT_NANOLAB =			550,
	MT_FORCELAB =			551,
	MT_LRSENSOR =			552,
	MT_CDETECT =			553,
	MT_BALLISTICS =			353,
	MT_DISPLACEMENT =		354,
	MT_JUMPGATE =			389,
	MT_HANGER =				388,
	MT_HEAVYREFINERY =		393,
	MT_SUPERHEAVYREFINERY =	394,
	MT_SUPPLY =				538,
		
	MT_CORVETTE =			424,
	MT_TANKER =				433,
	MT_LSAT_DEFENSE =		428,
	MT_MISSILE_CRUISER =	430,
	MT_TROOPSHIP =			431,
	MT_FLEET_CARRIER =		425,
	MT_BATTLESHIP =			429,
	MT_SPACE_STATION =		432,
	MT_ION_CANNON =			392,
	MT_DREADNOUGHT =		412,
	MT_INFILTRATOR =		426,
	MT_LANCER_CRUISER =		427,
	MT_FABRICATOR =			460,
	MT_ADVFABRICATOR =		434,
	MT_DEBRIS = 			321,
	MT_SUPPLYSHIP =			559,
	MT_FLEETOFFICER =		560,
	MT_HALSEY =				396,
	MT_HAWKES =				397,
	MT_BENSON =				398,
	MT_SMIRNOFF =			399,
	MT_STEELE =				401,
	MT_TAKAI =				670,
	MT_BLACKWELL =			750,
	MT_ELITEDREADNOUGHT =	751,
	MT_ELITEBATTLESHIP =	752,
		
	MT_KHAMIR =				322,
	MT_SCOUT_CARRIER =		323,
	MT_FRIGATE =			324,
	MT_MINELAYER =			325,
	MT_HEAVY_CRUISER =		326,
	MT_SUPER_CARRIER =		327,
	MT_VORAAK_CANNON =		328,
	MT_SEEKER =				329,
	MT_SIPHON_TANKER =		333,
	MT_WEAVER =				334,
	MT_GREATER_WEAVER =		335,
	MT_HIVE_CARRIER =		331,
	MT_LEECH =				336,
	MT_ZORAP_SUPPLY =		332,
	MT_MORDELLA =			671,
	MT_AZKAR =				672,
	MT_VERLAK =				673,
	MT_KERTAK =				674,
	MT_ADTHRIPID =			675,
	MT_MALKOR =				676,

	MT_BIOFORGE =			337,
	MT_BLAST_FURNACE =		338,
	MT_CARPACE_PLANT =		339,
	MT_CARRION_ROOST =		340,
	MT_COCOON =				341,
	MT_EXPLOSIVES_RANGE =	342,
	MT_EYE_STOCK =			343,
	MT_FUSION_MILL =		344,
	MT_MUTATION_COLONY =	345,
	MT_NIAD =				346,
	MT_THRIPID =			347,
	MT_WARLORD_TRAINING =	348,
	MT_DUAL_SPITTER =		349,
	MT_PLASMA_SPITTER =		350,
	MT_COLLECTOR =			351,
	MT_PLANTATION =			352,
	MT_GREAT_PLANTATION =	810,
	MT_PLASMA_HIVE =		355,
	MT_HYBRIDCENTER =		356,
	MT_GREATERCOLLECTOR =	395,
	MT_DISSECTION =			391,

	MT_ACROPOLIS =			357,
	MT_OXIDATOR =			358,
	MT_PAVILION =			359,
	MT_SENTINELTOWER =		360,
	MT_BUNKER =				361,
	MT_SANCTUM =			362,
	MT_EUTROMILL =			363,
	MT_GREATERPAVILION =	364,
	MT_HELIONVEIL =			365,
	MT_CITADEL =			366,
	MT_XENOCHAMBER =		367,
	MT_ANVIL =				368,
	MT_MUNITIONSANNEX =		369,
	MT_TURBINEDOCK =		370,
	MT_TALOREANMATRIX =		371,
	MT_PORTAL =				693,

	MT_FORGER =				372,
	MT_STRATUM =			373,
	MT_GALIOT =				374,
	MT_TAOS =				375,
	MT_POLARIS =			376,
	MT_AURORA =				377,		
	MT_ORACLE =				378,
	MT_ESPCOIL =			379,
	MT_STARBURST =			380,
	MT_HYDROFOIL =			381,
	MT_LEGIONARE =			383,
	MT_PROTEUS =			382,
	MT_ATLAS =				384,
	MT_TRIREME =			385,
	MT_MONOLITH =			386,
	MT_HIGHCOUNSEL =		387,
	MT_BLANUS =				687,
	MT_ELAN =				688,
	MT_VIVAC =				689,
	MT_JOULE =				690,
	MT_PROCYO =				691,
	MT_NATUS =				692,
	MT_ELITEMONOLITH =		753,
	MT_ELITETRIREME =		754,
	MT_APOCARTHROPOD =		755,

	MT_PLANET_EARTH =		654,
	MT_PLANET_MOON =		655,
	MT_PLANET_GAS =			656,
	MT_PLANET_SWAMP =		657,

	MT_LOCUS =				6001,
	MT_COALESCER =			6002,
	MT_HATCHERY =			6003,
	MT_COCHLEA_DISH =		6004,
	MT_COMPILER =			6005,
	MT_FORMULATOR =			6006,
	MT_GUDGEON =			6007,
	MT_REPOSITORIUM =		6008,
	MT_PERPETUATOR =		6009,
	MT_CLAW_OF_VYRIE =		6010,
	MT_EYE_OF_VYRIE =		6011,
	MT_HAMMER_OF_VYRIE =	6012,
	MT_SINUATOR =			6013,
	MT_CYNOSURE =			6014,
	MT_SHAPER =				6015,
	MT_ANACONDA =			6016,
	MT_AGGREGATOR =			6017,
	MT_MYSTIC =				6018,
	MT_VIPER =				6019,
	MT_ADDER =				6020,
	MT_NECTROP =			6021,
	MT_ERTRAG =				6022,
	MT_MOK =				6023,
	MT_COBRA =				6024,
	MT_CROTAL =				6025,
	MT_BASILISK =			6026,
	MT_LEVIATHIAN =			6027,
	MT_TEMPLE_OF_VYRIE =	6028,

	MT_REPAIRSHIP =			6100,
	MT_GEN_ADMIRAL =		6200,
	MT_TMP_NOVABOMB =		6300,
	MT_TMP_INTELLCTR =		6301,
	MT_MP_RESOURCE =		6302,
	MT_MP_RESEARCHLAB =		6303,
	MT_MP_INDUSTRIAL =		6304,
	MT_MMP_HEALCLOUD =		6305,
	MT_MMP_BEEHIVE =		6306,
	MT_CMP_CLOAKING =		6307,
	MT_CMP_TALMATRIX =		6308,
	MT_VMP_ASTEROID =		6309,
	MT_VMP_SPACEFOLDER =	6310,

	MT_ESPIONAGE =			6311,

	MT_BLOCKADE_RUNNER =	6312,
	MT_LISTENING_DEVICE =	6313,
	MT_GENESIS_DEVICE =		6314,

};
} 




















			    






namespace SILNAMES
{
enum M_SILHOUETTE_NAME
{
   CORVETTE			= 1087,               
   TANKER			= 1111,
   LSAT				= 1102,
   MISSILECRUISER	= 1081,
   ASSAULTSHIP		= 1117,
   SPACESTATION		= 1105,
   BATTLESHIP		= 1099,
   FLEETCARRIER		= 1090,
   DREADNOUGHT		= 1084,
   INFILTRATOR		= 1093,
   LANCER			= 1096,
   SUPPLY			= 1114,
   FABRICATOR		= 1075,
   ADVFABRICATOR	= 1078,
   FLEETOFFICER		= 1108,
   IONCANNON		= 1120,
   M_WEAVER			= 943,
   M_GREATERWEAVER	= 946,
   M_ZORAP			= 949,
   M_SIPHON			= 952,
   M_PLASMASPITTER	= 955,
   M_OCELLUS		= 958,
   M_SCOUTCARRIER	= 964,
   M_FRIGATE		= 967,
   M_KHAMIR			= 970,
   M_HIVECARRIER	= 973,
   M_TIAMATCARRIER	= 976,
   M_SITHKAR		= 979,
   M_CAPTORLEECH	= 982,
   M_VORAAKCANNON	= 985,
   M_SPINELAYER		= 988,
   M_PLASMAHIVE		= 961,
   S_FORGER			= 1743,
   S_STRATUM		= 1746,
   S_GALIOT			= 1749,
   S_TAOS			= 1752,
   S_POLARIS		= 1755,
   S_AURORA			= 1758,
   S_STARBURST		= 1761,
   S_HYDROFOIL		= 1764,
   S_LEGIONARE		= 1767,
   S_ATLAS			= 1773,
   S_TRIREME		= 1776,
   S_MONOLITH		= 1779,
   S_HIGHCOUNCIL	= 1782,
   S_ORACLE			= 1785,
   S_ESPCOIL		= 1788,
   S_PORTAL			= 2444,
   S_PROTEUS		= 2447,
   T_ESPIONAGE		= 2734

};
} 




















			    






namespace UNITSOUNDS
{
enum PRIORITY
{
	LOW,
	MEDIUM,
	HIGH,
	ADMIRAL
};

union SPEECH
{
	struct SPACESHIP
	{
		M_STRING underAttack;
		M_STRING alliedAttack;
		M_STRING enemySighted;
		M_STRING selected;
		M_STRING destructionDenied;	
		M_STRING move;
		M_STRING aggravated;
		M_STRING constructComplete;
		M_STRING death;
	};

	struct RESUSER
	{
		M_STRING notEnoughGas;
		M_STRING notEnoughMetal;
		M_STRING notEnoughCrew;
		M_STRING notEnoughCommandPoints;
	};

	struct GUNBOAT : SPACESHIP
	{
		M_STRING attacking;
		M_STRING suppliesout;
		M_STRING specialAttack;
	} gunboat;
	
	struct HARVESTER : SPACESHIP
	{
		M_STRING harvestResource;
		M_STRING planetDepleted;
		M_STRING planetDepletedRedeploy;
	} harvester;
	
	struct FABRICATOR : SPACESHIP, RESUSER
	{
		M_STRING buildImposible;
		M_STRING constructStarted;
	} fabricator;
	
	struct MINELAYER : SPACESHIP
	{
	} minelayer;

	struct SUPPLYSHIP : SPACESHIP
	{
		M_STRING resupplyShips;
	} supplyship;

	struct TROOPSHIP : SPACESHIP
	{
		M_STRING attacking;
		M_STRING attackSuccess;
		M_STRING attackFailed;

	} troopship;

	struct FLAGSHIP : SPACESHIP
	{
		M_STRING attacking;
		M_STRING fleetdamage50;		
		M_STRING fleetdamage75;		
		M_STRING supplieslow;		
		M_STRING suppliesout;		
		M_STRING battlegood;		
		M_STRING battlemoderate;	
		M_STRING battlebleak;		
		M_STRING shipleaving;		
		M_STRING admiralondeck;		
		M_STRING flagshipintrouble;	
		M_STRING transferfailed;	
		M_STRING scuttle;			
	} flagship;

	struct PLATFORM
	{
		M_STRING underAttack;
		M_STRING alliedAttack;
		M_STRING enemySighted;
		M_STRING selected;
		M_STRING destructionDenied;	
		M_STRING constructComplete;
		M_STRING constructStarted;
		M_STRING buildDelayed;
		M_STRING research;
		M_STRING researchCompleted;
		M_STRING upgradeCompleted;
	};
	
	struct GENPLATFORM : PLATFORM, RESUSER
	{
	} platform;

	struct GUNPLAT : PLATFORM
	{
		M_STRING attacking;
		M_STRING suppliesout;
	} gunPlat;

};	

}  

typedef UNITSOUNDS::SPEECH MT_UNITSPEECH;




















			    


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


enum M_OBJCLASS
{
	M_NONE=0,
	
	
	
	M_FABRICATOR=1,
	M_SUPPLY,
	M_REPAIR_SHIP,

	M_CORVETTE,				
	M_MISSILECRUISER,
	M_BATTLESHIP,			
	M_DREADNOUGHT,
	M_CARRIER,
	M_LANCER,
	M_INFILTRATOR,			

	
	M_ESPIONAGE,

	M_HARVEST,				
	M_RECONPROBE,
	M_TROOPSHIP,
	M_FLAGSHIP,

	M_HQ,
	M_REFINERY,				
	M_LIGHTIND,
	M_TENDER,
	M_REPAIR,
	M_OUTPOST,
	M_ACADEMY,				
	M_BALLISTICS,			
	M_ADVHULL,
	M_HEAVYREFINERY,
	M_SUPERHEAVYREFINERY,
	M_REMOTE_HQ,

	
	M_NOVA_BOMB,
	M_INTEL_CENTER,
	M_T_RESOURCE_FACTORY,
	M_T_RESEARCH_LAB,
	M_T_IND_FACILITY,

	

	M_LISTENING_DEVICE,
	M_BLOCKADE_RUNNER,
	M_EARTH_GENESIS_DEVICE,

	
	M_LSAT,					
	M_SPACESTATION,			
	
	M_LRSENSOR,				
	M_AWSLAB,
	M_IONCANNON,
	M_HEAVYIND,				
	M_HANGER,				
	M_PROPLAB,
	M_DISPLAB,

	M_CLOAKSTATION,

	
	
	
	M_PLANET,				
	M_FIELD,				
	M_DEBRIS,
	M_WAYPOINT,
	M_JUMPGATE,
	M_MINEFIELD,			
	M_NUGGET,				
	M_NEBULA,
	M_ASTEROID_FIELD,
	M_JUMPPLAT,				

	
	
	
	M_COCOON,				
	M_COLLECTOR,			
	M_GREATER_COLLECTOR,
	M_PLANTATION,
	M_GREATER_PLANTATION,
	M_EYESTOCK,				
	M_THRIPID,				
	M_WARLORDTRAINING,
	M_BLASTFURNACE,
	M_EXPLOSIVESRANGE,
	M_PLASMASPLITTER,		
	M_CARRIONROOST,			
	M_VORAAKCANNON,
	M_MUTATIONCOLONY,
	M_NIAD,
	M_BIOFORGE,				
	M_FUSIONMILL,			
	M_CARPACEPLANT,
	M_DISSECTIONCHAMBER,
	M_HYBRIDCENTER,
	M_PLASMAHIVE,			
	M_FRONTIER_HIVE,

	
	M_M_RESOURCE_FACTORY,
	M_M_RESEARCH_LAB,
	M_M_IND_FACILITY,
	M_HEAL_CLOUD,
	M_HIVE,

	M_WEAVER,			
	M_SPINELAYER,		
	M_SIPHON,			
	M_ZORAP,			
	M_LEECH,			
	M_SEEKER,			
	M_SCOUTCARRIER,		
	M_FRIGATE,			
	M_KHAMIR,			
	M_HIVECARRIER,		
	M_SCARAB,			
	M_TIAMAT,			
	M_WARLORD,			

	
	
	

	M_ACROPOLIS,
	M_OXIDATOR,				
	M_PAVILION,				
	M_SENTINELTOWER,
	M_EUTROMILL,
	M_GREATERPAVILION,
	M_HELIONVEIL,			
	M_CITADEL,				
	M_XENOCHAMBER,
	M_ANVIL,
	M_MUNITIONSANNEX,
	M_TURBINEDOCK,			
	M_TALOREANMATRIX,			
	M_BUNKER,			
	M_PARTHENON,

	
	M_S_RESOURCE_FACTORY,
	M_S_RESEARCH_LAB,
	M_S_IND_FACILITY,
	M_CLOAK_PLATFORM,

	
	M_PROTEUS,
	M_HYDROFOIL,
	M_ESPCOIL,				
	M_STARBURST,		
	M_PORTAL,

	M_FORGER,			
	M_STRATUM,			
	M_GALIOT,			
	M_ATLAS,			
	M_LEGIONAIRE,		
	M_TAOS,				
	M_POLARIS,			
	M_AURORA,			
	M_ORACLE,			
	M_TRIREME,			
	M_MONOLITH,			
	M_HIGHCOUNSEL,		

	
	
	

	

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

	
	M_V_RESOURCE_FACTORY,
	M_V_RESEARCH_LAB,
	M_V_IND_FACILITY,
	M_SPACE_FOLDER,
	M_GRAVITY_CENTER,

	
	M_SINUATOR,

	
	M_SHAPER,			
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

	M_ENDOBJCLASS			
};


enum UNIT_STANCE
{
	US_DEFEND = 0,
	US_ATTACK,
	US_STAND,
	US_STOP
};


enum HARVEST_STANCE 
{
	HS_NO_STANCE = 0,
	HS_GAS_HARVEST,
	HS_ORE_HARVEST,
};


enum ADMIRAL_TACTIC
{
	AT_PEACE = 0,
	AT_DEFEND,
	AT_HOLD,
	AT_SEEK
};


enum FighterStance
{
	FS_NORMAL = 0,
	FS_PATROL,
};


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
	

	USA_DOCK,
	USA_LAST	
};









typedef unsigned __int64 U64;





struct DYNAMICS_DATA
{
    SINGLE linearAcceleration;
    SINGLE angAcceleration;

    SINGLE maxLinearVelocity;
    SINGLE maxAngVelocity;				
};

struct FLASH_DATA
{
	SINGLE lifeTime;
	SINGLE range;
	U8 red,green,blue;
};

struct TRANS_SAVELOAD
{
	Vector position;
	SINGLE ang_position;
	Vector velocity, ang_velocity;
};

struct EXTENSION_SAVELOAD
{
	U8 levelsAdded;
	S8 extensionLevel;
	S8 workingExtLevel;
	SINGLE percentExt;
};

struct CLOAK_SAVELOAD_BASE
{
	SINGLE cloakTimer,cloakPercent;

	bool bCloakPending:1;
	bool bCloaking:1;
	U32 cloakCount;
};

struct CLOAK_SAVELOAD
{
	CLOAK_SAVELOAD_BASE baseCloak;
	BOOL32 bCloaked;
};

enum ARMOR_TYPE
{
	NO_ARMOR,
	LIGHT_ARMOR,
	MEDIUM_ARMOR,
	HEAVY_ARMOR
};

struct ARMOR_DATA
{
	ARMOR_TYPE myArmor;


	union FIELDS			



	{
		struct ARMOR_DAMAGE
		{
			SINGLE noArmor;
			SINGLE lightArmor;
			SINGLE mediumArmor;
			SINGLE heavyArmor;
		} damageTable;

		SINGLE _damageTable[4];

	} fields;




};

struct ResourceCost
{
	U32 gas:8;
	U32 metal:8;
	U32 crew:8;
	U32 commandPt:8;
};

struct DamageSave
{
	U32 damage;
	Vector pos;
	bool bActive;
};


struct DAMAGE_SAVELOAD
{
	U32 lastRepairSlot;
	SINGLE lastDamage;
	struct DamageSave damageSave[5];
};


struct BASIC_DATA		
{
    OBJCLASS objClass;
	bool bEditDropable;		
};


enum GENBASE_TYPE 
{
	GBT_FONT = 1,
	GBT_BUTTON,
	GBT_STATIC,
	GBT_EDIT,
	GBT_LISTBOX,
	GBT_DROPDOWN,
	GBT_VFXSHAPE,
	GBT_HOTBUTTON,
	GBT_SCROLLBAR,
	GBT_HOTSTATIC,
	GBT_SHIPSILBUTTON,
	GBT_COMBOBOX,
	GBT_SLIDER,
	GBT_TABCONTROL,
	GBT_ICON,
	GBT_QUEUECONTROL,
	GBT_ANIMATE,
	GBT_PROGRESS_STATIC,
	GBT_DIPLOMACYBUTTON
};

struct GENBASE_DATA		
{
	GENBASE_TYPE type;
};



struct GT_VFXSHAPE : GENBASE_DATA
{
	char filename[32];
	bool bHiRes;
};









struct MISSION_DATA		
{
	M_OBJCLASS	mObjClass:9;
	M_RACE		race:4;
	OBJNAMES::M_DISPLAY_NAME displayName:17;

	struct M_CAPS
	{
		U8 padding;
		bool moveOk:1;
		bool attackOk:1;
		bool specialAttackOk:1;
		bool specialEOAOk:1;
		bool specialAbilityOk:1;
		bool specialAttackWormOk:1;
		bool defendOk:1;	
		bool supplyOk:1;
		bool harvestOk:1;
		bool buildOk:1;
		bool repairOk:1;
		bool jumpOk:1;
		bool admiralOk:1;	
		bool captureOk:1;	
		bool salvageOk:1;
		bool probeOk:1;
		bool mimicOk:1;
		bool recoverOk:1;
		bool createWormholeOk:1;
		bool synthesisOk:1;
		bool cloakOk:1;
		bool specialAttackShipOk:1;  
		bool targetPositionOk:1;
		bool specialTargetPlanetOk:1;







	} caps;


	U16			hullPointsMax;
	U16			supplyPointsMax;
	U16			scrapValue;
	U16			buildTime;
	ResourceCost resourceCost;

	SINGLE		sensorRadius;
	SINGLE		cloakedSensorRadius;
	SINGLE		maxVelocity;
	SINGLE		baseWeaponAccuracy;
	SINGLE		baseShieldLevel;		

	ARMOR_DATA  armorData;
	SILNAMES::M_SILHOUETTE_NAME silhouetteImage;
	UNIT_SPECIAL_ABILITY    specialAbility;
	UNIT_SPECIAL_ABILITY    specialAbility1;
	UNIT_SPECIAL_ABILITY    specialAbility2;
	UNITSOUNDS::PRIORITY speechPriority;
};





struct MISSION_DATA_OVERRIDE
{
	U16			 hullPointsMax;
	U16			 supplyPointsMax;
	U16			 scrapValue;
	U16			 buildTime;
	U8           commandPoints;
	SINGLE		 sensorRadius;
	SINGLE		 cloakedSensorRadius;
	SINGLE		 maxVelocity;
	SINGLE		 baseShieldLevel;
	ARMOR_DATA   armorData;
	char         scriptHandle[64];
};



























			    

























			    






struct BASIC_INSTANCE
{
	Vector position, rotation;
};



















			    










namespace TECHTREE
{









enum TECHUPGRADE
{
	NO_TECHUPGRADE		= 0x00000000,
	ALL_TECHUPGRADE		= 0xFFFFFFFF,

	
	
	T_RES_XCHARGES		= 0x00000001,
	T_RES_XSHIELD		= 0x00000002,
	T_RES_XPROBE		= 0x00000004,
	T_RES_XCLOAK		= 0x00000008,
	T_RES_XVAMPIRE		= 0x00000010,	
	T_RES_MISSLEPACK1	= 0x00000020,
	T_RES_MISSLEPACK2	= 0x00000040,
	T_RES_MISSLEPACK3	= 0x00000080,
	T_RES_TROOPSHIP1	= 0x00000100,
	T_RES_TROOPSHIP2	= 0x00000200,
	T_RES_TROOPSHIP3	= 0x00000400,

	

	

	M_RES_XCAMO			= 0x10000001,
	M_RES_XGRAVWELL		= 0x10000002,
	M_RES_XRCLOUD		= 0x10000004,
	M_RES_REPULSOR		= 0x10000010,
	M_RES_EXPLODYRAM1	= 0x10000020,
	M_RES_EXPLODYRAM2	= 0x10000040,
	M_RES_LEECH1		= 0x10000080,
	M_RES_LEECH2		= 0x10000100,

	

	

	S_RES_SYNTHESIS		= 0x20000001,
	S_RES_MASSDISRUPTOR = 0x20000002,
	S_RES_AURORACLOAK   = 0x20000004,
	S_RES_DESTABILIZER  = 0x20000008,
	S_RES_TRACTOR		= 0x20000010,
	S_RES_LEGION1		= 0x20000020,
	S_RES_LEGION2		= 0x20000040,
	S_RES_LEGION3		= 0x20000080,
	S_RES_LEGION4		= 0x20000100,
	S_RES_PROTEUSPACK1	= 0x20000200,
	S_RES_PROTEUSPACK2	= 0x20000400,

	

	

	

	

	T_SHIP__FABRICATOR		= 0x08000000,
	T_SHIP__SUPPLY			= 0x04000000,
	T_SHIP__CORVETTE		= 0x02000000,
	T_SHIP__MISSILECRUISER	= 0x01000000,
	T_SHIP__BATTLESHIP		= 0x00800000,
	T_SHIP__DREADNOUGHT		= 0x00400000,
	T_SHIP__CARRIER			= 0x00200000,
	T_SHIP__LANCER			= 0x00100000,
	T_SHIP__INFILTRATOR		= 0x00080000,
	T_SHIP__HARVEST			= 0x00040000,				
	T_SHIP__RECONPROBE		= 0x00020000,
	T_SHIP__TROOPSHIP		= 0x00010000,
	T_SHIP__FLAGSHIP		= 0x00008000,


	M_SHIP_WEAVER			= 0x18000000,		
	M_SHIP_SPINELAYER		= 0x14000000,		
	M_SHIP_SIPHON			= 0x12000000,			
	M_SHIP_ZORAP			= 0x11000000,			
	M_SHIP_SEEKER			= 0x10800000,			
	M_SHIP_SCOUTCARRIER		= 0x10400000,	
	M_SHIP_FRIGATE			= 0x10200000,				
	M_SHIP_KHAMIR			= 0x10100000,		
	M_SHIP_HIVECARRIER		= 0x10080000,		
	M_SHIP_LEECH			= 0x10040000,		
	M_SHIP_SCARAB			= 0x10020000,				
	M_SHIP_TIAMAT			= 0x10010000,			
	M_SHIP_WARLORD			= 0x10008000,			


	S_SHIP_FORGER			= 0x28000000,			
	S_SHIP_STRATUM			= 0x24000000,			
	S_SHIP_GALIOT			= 0x22000000,			
	S_SHIP_TAOS				= 0x21000000,				
	S_SHIP_POLARIS			= 0x20800000,			
	S_SHIP_AURORA			= 0x20400000,			
	S_SHIP_LEGIONAIRE		= 0x20200000,		
	S_SHIP_ORACLE			= 0x20100000,			
	S_SHIP_ATLAS			= 0x20080000,			
	S_SHIP_TRIREME			= 0x20040000,			
	S_SHIP_MONOLITH			= 0x20020000,			
	S_SHIP_HIGHCOUNSEL		= 0x20010000		

};



enum BUILDNODE
{
	NO_BUILDNODE		=  0x00000000,
	ALL_BUILDNODE		=  0xFFFFFFFF,

	
	RES_REFINERY_GAS1		= 0x08000000,	
	RES_REFINERY_GAS2		= 0x04000000,
	RES_REFINERY_GAS3		= 0x02000000,
	RES_REFINERY_METAL1		= 0x01000000,
	RES_REFINERY_METAL2		= 0x00800000,
	RES_REFINERY_METAL3		= 0x00400000,
	RES_REFINERY_CREW1		= 0x00200000,

	
	
	RES_REFINERY_CREW2		= 0x40000000,
	RES_REFINERY_CREW3		= 0x80000000,
	


	
	TDEPEND_LIGHT_IND	  =  0x00000001,
	TDEPEND_HEAVY_IND	  =  0x00000002,
	TDEPEND_TECH_IND	  =  0x00000004,
	TDEPEND_REFINERY	  =  0x00000008,
	TDEPEND_OUTPOST		  =  0x00000010,
	TDEPEND_ACADEMY		  =  0x00000020,
	TDEPEND_ADVHULL		  =  0x00000040,
	TDEPEND_AWSLAB		  =  0x00000080,
	TDEPEND_BALLISTICS	  =  0x00000100,
	TDEPEND_TENDER		  =  0x00000200,
	TDEPEND_SENSORTOWER	  =  0x00000400,
	TDEPEND_HANGER		  =  0x00000800,
	TDEPEND_WEAPONS		  =  0x00001000,
	TDEPEND_DISPLACEMENT  =  0x00002000,
	TDEPEND_HEADQUARTERS  =  0x00004000,
	TDEPEND_PROPLAB		  =  0x00008000,
	TDEPEND_LASER_TURRET   = 0x00010000,
	TDEPEND_ION_CANNON	  =  0x00020000,
	TDEPEND_JUMP_INHIBITOR  =0x00040000,
	TDEPEND_SPACE_STATION   =0x00080000,
	TDEPEND_REPAIR		    =0x00100000,

	TCOMBO_ACADEMY_DIS_HEAVY	= 0x00002022,	
	TCOMBO_HANGER_HEAVY			= 0x00000802,	
	TCOMBO_LIGHT_BALLISTICS		= 0x00000101,	
	TCOMBO_WEAPONS_HEAVY		= 0x00001002,	
	TCOMBO_PROP_HEAVY			= 0x00008002,	
	TCOMBO_ACADEMY_DIS_PROP		= 0x0000a020,   
	TCOMBO_PROP_AWS				= 0x00008080,	
	TCOMBO_LIGHT_SENSOR			= 0x00000401,
	TCOMBO_BALLISTICS_HEAVY_SENSOR = 0x00000502,
	TCOMBO_PROP_SENSOR			= 0x00008400,
	TCOMBO_ADVHULL_DIS			= 0x00002040,
	TCOMBO_BALLISTICS_HANGER	= 0x00000900,
	TCOMBO_ACADEMY_LIGHT		= 0x00000021,
	TCOMBO_SENSOR_JUMP			= 0x00040400,
	

	

	MDEPEND_COCOON		= 0x10000001,
	MDEPEND_COLLECTOR	= 0x10000002,
	
	MDEPEND_PLANTATION	= 0x10000004,
	MDEPEND_GR_PLANTATION = 0x1000000C,

	MDEPEND_THRIPID		= 0x10000010,
	MDEPEND_NIAD		= 0x10000030,

	MDEPEND_EYESTOCK	= 0x10000040,
	MDEPEND_WARLORDTRAIN= 0x10000080,
	MDEPEND_MUTATECOLON = 0x10000140,

	MDEPEND_CARRIONROOST= 0x10000200,

	MDEPEND_BLASTFURNACE= 0x10000400,
	MDEPEND_EXPLSVRANGE	= 0x10000C00,

	MDEPEND_BIOFORGE	= 0x10001000,
	MDEPEND_FUSIONMILL	= 0x10003000,
	MDEPEND_CARPACEPLANT= 0x10005000,
	MDEPEND_HYBRIDCENTER= 0x10007000,

	MDEPEND_PLASMA_SPITTER =0x10008000,
	MDEPEND_DUAL_SPITTER = 0x10018000,
	MDEPEND_PLASMA_HIVE = 0x10028000,

	MDEPEMD_JUMP_PLAT =   0x10040000,

	MDEPEND_GR_COLLECTOR= 0x10080002,

	MDEPEND_DISECTION	= 0x10100000,

	MCOMBO_CARAPCE_CARRION	= 0x10005200,	
	MCOMBO_COLLECT_PLANT	= 0x10000006,	
	MCOMBO_CARRION_BIO_DISECT=0x10101200,	
	MCOMBO_COCCON_COL_EYE	= 0x10000043,	
	MCOMBO_NIAD_CAR_BIO_DIS = 0x10101230,	
	

	

	SDEPEND_ACROPOLIS	=	0x20000001,
	SDEPEND_OXIDATOR	=	0x20000002,
	SDEPEND_PAVILION	=	0x20000004,
	SDEPEND_SENTINALTOWER = 0x20000008,
	SDEPEND_BUNKER		=	0x20000010,
	SDEPEND_SANCTUM		=	0x20000020,
	SDEPEND_EUTROMIL	=	0x20000040,
	SDEPEND_GREATERPAVILION=0x20000080,
	SDEPEND_HELIONVIEL	=	0x20000100,
	SDEPEND_CITADEL		=	0x20000200,
	SDEPEND_XENOCHAMBER	=	0x20000400,
	SDEPEND_ANVIL		=	0x20000800,
	SDEPEND_MUNITIONSANEX = 0x20001000,
	SDEPEND_TURBINEDOCK =	0x20002000,
	SDEPEND_TALOREANMATRIX= 0x20004000,
	SDEPEND_JUMP_PLAT   =   0x20008000,

	SDEPEND_ESP_COIL    =   0x20010000,
	SDEPEND_PROTEUS     =   0x20020000,
	SDEPEND_HYDROFOIL   =   0x20040000,
	SDEPEND_STARBURST   =   0x20080000,
	SDEPEND_PORTAL      =   0x20100000,

	SCOMBO_MUNITION_TURB	= 0x20003000,	
	SCOMBO_CITADEL_GPAV		= 0x20000280,	
	SCOMBO_XENO_MUNITION	= 0x20001400,	
	SCOMBO_OXY_EUTRO		= 0x20000042,	
	SCOMBO_MUN_TURB_XENO    = 0x20003400,	
	SCOMBO_HELL_GRPAV		= 0x20000180,	
	SCOMBO_OXI_SENT			= 0x2000000a,	
	SCOMBO_GREATPAV_PROT	= 0x20020080,   
	SCOMBO_GRPAV_MUAX_TDOCK_XEN = 0x20003480, 
	SCOMBO_GRPAV_PAV_HEVIEL = 0x20000184,   
	

	

	
};

enum COMMON
{
	NO_COMMONUPGRADE	= 0x00000000,
	ALL_COMMON			= 0x1FFFFFFF,

	RESERVED_FABRICATOR	=  0x20000000,
	RESERVED_ADMIRAL	=  0x40000000,
	RESERVED_NEVER		=  0x80000000,

	RES_ENGINE1		= 0x00000001,	
	RES_ENGINE2		= 0x00000002,
	RES_ENGINE3		= 0x00000004,
	RES_ENGINE4		= 0x00000008,
	RES_ENGINE5		= 0x00000010,
	
	RES_SHIELDS1	= 0x00000020,
	RES_SHIELDS2	= 0x00000040,
	RES_SHIELDS3	= 0x00000080,
	RES_SHIELDS4	= 0x00000100,
	RES_SHIELDS5	= 0x00000200,
	
	RES_HULL1		= 0x00000400,
	RES_HULL2		= 0x00000800,
	RES_HULL3		= 0x00001000,
	RES_HULL4		= 0x00002000,
	RES_HULL5		= 0x00004000,

	RES_SUPPLY1		= 0x00008000,
	RES_SUPPLY2		= 0x00010000,
	RES_SUPPLY3		= 0x00020000,
	RES_SUPPLY4		= 0x00040000,
	RES_SUPPLY5		= 0x00080000,

	RES_WEAPONS1	= 0x00100000,
	RES_WEAPONS2	= 0x00200000,
	RES_WEAPONS3	= 0x00400000,
	RES_WEAPONS4	= 0x00800000,
	RES_WEAPONS5	= 0x01000000,

	MDEPEND_HYBRID_EXTRA = 0x02000000,

};

enum COMMON_EXTRA
{
	NO_COMMONEXTRAUPGRADE	= 0x00000000,
	ALL_COMMONEXTRAUPGRADES = 0xFFFFFFFF,

	RES_FLEET1		= 0x00000001,
	RES_FLEET2		= 0x00000002,
	RES_FLEET3		= 0x00000004,
	RES_FLEET4		= 0x00000008,
	RES_FLEET5		= 0x00000010,

	RES_TANKER1		= 0x00000020,
	RES_TANKER2		= 0x00000040,
	RES_TANKER3		= 0x00000080,
	RES_TANKER4		= 0x00000100,
	RES_TANKER5		= 0x00000200,

	RES_TENDER1		= 0x00000400,
	RES_TENDER2		= 0x00000800,
	RES_TENDER3		= 0x00001000,
	RES_TENDER4		= 0x00002000,
	RES_TENDER5		= 0x00004000,

	RES_SENSORS1	= 0x00008000,
	RES_SENSORS2	= 0x00010000,
	RES_SENSORS3	= 0x00020000,
	RES_SENSORS4	= 0x00040000,
	RES_SENSORS5	= 0x00080000,

	RES_FIGHTER1	= 0x00100000,
	RES_FIGHTER2	= 0x00200000,
	RES_FIGHTER3	= 0x00400000,
	RES_FIGHTER4	= 0x00800000,
	RES_FIGHTER5	= 0x01000000,

	RES_ADMIRAL1	= 0x02000000,
	RES_ADMIRAL2	= 0x04000000,
	RES_ADMIRAL3	= 0x08000000,
	RES_ADMIRAL4	= 0x10000000,
	RES_ADMIRAL5	= 0x20000000,
	RES_ADMIRAL6	= 0x40000000
};






enum CQ2_VARS_1
{
	NO_CQ2_VARS_1		= 0x00000000,
	
	V_SHIP_SHAPER		= 0x40000001,
	V_SHIP_ANACONDA		= 0x40000002,
	V_SHIP_AGGREGATOR	= 0x40000004,
	V_SHIP_MYSTIC		= 0x40000008,
	V_SHIP_VIPER		= 0x40000010,
	V_SHIP_ADDER		= 0x40000020,
	V_SHIP_NECTROP		= 0x40000040,
	V_SHIP_ERTRAG		= 0x40000080,
	V_SHIP_MOK			= 0x40000100,
	V_SHIP_COBRA		= 0x40000200,
	V_SHIP_CROTAL		= 0x40000400,
	V_SHIP_BASILISK		= 0x40000800,
	V_SHIP_LEVIATHIN	= 0x40001000,


	
	VDEPEND_LOCUS			= 0x40002000,
	VDEPEND_COALESCER		= 0x40004000,
	VDEPEND_HATCHERY		= 0x40008000,
	VDEPEND_COCHLEA_DISH	= 0x40010000,
	VDEPEND_COMPILER		= 0x40020000,
	VDEPEND_FORMULATOR		= 0x40040000,
	VDEPEND_GUDGEON			= 0x40080000,
	VDEPEND_REPOSITORIUM	= 0x40100000,
	VDEPEND_PERPETUATOR		= 0x40200000,
	VDEPEND_CLAW_OF_VYRIE	= 0x40400000,
	VDEPEND_EYE_OF_VYRIE	= 0x40800000,
	VDEPEND_TEMPLE_OF_VYRIE	= 0x41000000,
	VDEPEND_HAMMER_OF_VYRIE = 0x42000000,
	VDEPEND_SINUATOR		= 0x44000000,
	VDEPEND_SHIELD_OF_VYRIE = 0x48000000,

	VCOMBO_REPOSIT_COMPILER =0x40120000,
	VCOMBO_CLAW_COMPILER    =0x40420000,
	VCOMBO_EYE_GUDGEON		=0x40880000,
	VCOMBO_HATCHERY_COMPILER=0x40028000,

	
	T_SHIP_REPAIR			= 0x00000001,
	T_SHIP_ESPIONAGE        = 0x00000002,
	T_SHIP_GENESIS_ART      = 0x00000004,
	T_SHIP_BLOCKADE_RUN_ART = 0x00000008,
	T_SHIP_LISTENING_ART    = 0x00000010,
};

enum CQ2_VARS_2
{
	NO_CQ2_VARS_2		= 0x00000000,

	
	TDEPEND_TEMPHQ				=0x00000001,
	TDEPEND_INTEL_CENTER		=0x00000002,
	TDEPEND_NOVA_BOMB			=0x00000004,
	TDEPEND_RESOURCE_FACTORY	=0x00000008,
	TDEPEND_INDUSTRIAL_FACILITY =0x00000010,
	TDEPEND_RESEARCH_LAB		=0x00000020,

	
	V_RES_INHIBITOR_LIMPETS		=0x40000001,
	V_RES_DEATH_BLOOM			=0x40000002,
	V_RES_PARALISIS_BEAM		=0x40000004,
	V_RES_JUMP_DRIVE			=0x40000008,
	V_RES_ANTIMATTER_CANNON		=0x40000010,
	V_RES_CAMOFLAGE				=0x40000020,
	V_RES_ATTRACTOR_WAVE		=0x40000040,



};

enum LEVEL_INIT
{
	FULL_TREE,
};

}  






struct SINGLE_TECHNODE
{
	enum M_RACE raceID;

	TECHTREE::TECHUPGRADE	tech;
	TECHTREE::BUILDNODE		build;
	TECHTREE::COMMON		common;
	TECHTREE::COMMON_EXTRA	common_extra;
	TECHTREE::CQ2_VARS_1	cq2Vars1;
	TECHTREE::CQ2_VARS_2	cq2Vars2;












};





struct TECHNODE
{
	struct _races
	{
		TECHTREE::TECHUPGRADE	tech;
		TECHTREE::BUILDNODE		build;
		TECHTREE::COMMON		common;
		TECHTREE::COMMON_EXTRA	common_extra;
		TECHTREE::CQ2_VARS_1	cq2Vars1;
		TECHTREE::CQ2_VARS_2	cq2Vars2;
	} race[4];


































































































};














struct ROCKING_DATA : DYNAMICS_DATA
{
	SINGLE rockLinearMax, rockAngMax;			
};


struct ENGINE_GLOW_DATA
{
	S32 size[6];
	U8 r,g,b;
	char engine_texture_name[32];
};


struct BLINKER_DATA
{
	char light_script[32];
	char textureName[32];
};


struct SHIELD_DATA
{
	char meshName[32];
	char animName[32];
	char fizzAnimName[32];
	SFX::ID sfx;
	SFX::ID fizzOut;
	SFX::ID fizzIn;
};


struct DAMAGE_DATA
{
	char damageBlast[32];
};


struct CLOAK_DATA
{
	char cloakTex[32];
	bool bAutoCloak:1;
	char cloakEffectType[32];
};


struct BILLBOARD_DATA
{
	char billboardTexName[32];
	U32 billboardThreshhold;
	bool bTex2;
};




struct EXTENT_DATA
{
	RECT extents[10];
	SINGLE _step;
	SINGLE _min;
	SINGLE min_slice,max_slice;
	BOOL32 bX;
};


struct FORMATION_FILTER
{
	bool shortRange:1;
	bool mediumRang:1;
	bool longRange:1;
	bool recon:1;
	bool loneRecon:1;
	bool fighters:1;
	bool supplyShip:1;
	bool airDefence:1;

	bool missileCruiser:1;
	bool lancer:1;
	bool extra2:1;
	bool extra3:1;
	bool extra4:1;
	bool extra5:1;
	bool extra6:1;
	bool extra7:1;

	bool extra10:1;
	bool extra11:1;
	bool extra12:1;
	bool extra13:1;
	bool extra14:1;
	bool extra15:1;
	bool extra16:1;
	bool extra17:1;

	bool extra20:1;
	bool extra21:1;
	bool extra22:1;
	bool extra23:1;
	bool extra24:1;
	bool extra25:1;
	bool extra26:1;
	bool extra27:1;
};


struct BASE_SPACESHIP_DATA : BASIC_DATA
{
	SPACESHIPCLASS type;
    char fileName[32];
	MISSION_DATA missionData;
    DYNAMICS_DATA dynamicsData;
	ROCKING_DATA rockingData;
	char explosionType[32];
	char trailType[32];
	char ambient_animation[32];
	char ambientEffect[32];
	ENGINE_GLOW_DATA engineGlow;
	BLINKER_DATA blinkers;
	SHIELD_DATA shield;
	DAMAGE_DATA damage;
	CLOAK_DATA cloak;
	BILLBOARD_DATA billboard;
	SINGLE_TECHNODE techActive;




		FORMATION_FILTER formationFilter;




	bool bLargeShip; 
};






































































struct BT_GUNBOAT_DATA : BASE_SPACESHIP_DATA
{
	SINGLE outerWeaponRange;
	SINGLE optimalFacingAngle;	
	bool bNoLineOfSight;		
	char launcherType[5][32];
};							













struct BT_TROOPSHIP_DATA : BASE_SPACESHIP_DATA
{
	U32  damagePotential;
	SINGLE assaultRange;
	SFX::ID sfxPodRelease;
};							


struct BT_TROOPPOD_DATA : BASIC_DATA
{
	EFFECTCLASS fxClass;
	char podType[32];
	char podHardpoints[3][64];	
};











struct BT_RECONPROBE_DATA : BASE_SPACESHIP_DATA
{
	SINGLE lifeTime;
	char blastType[32];
};							










struct BT_TORPEDO_DATA : BASE_SPACESHIP_DATA
{
	SINGLE lifeTime;
	S32 damage;
	char blastType[32];
};							











struct BT_MINELAYER_DATA : BASE_SPACESHIP_DATA
{
	char drop_anim[32];
	char mineFieldType[32];
	char mineReleaseHardpoint[32];
	U8 fieldSupplyCostPerMine;
	U32 fieldCompletionTime;
	U32 mineReleaseVelocity;
	SINGLE dropAnimDelay;
};










struct BT_HARVESTSHIP_DATA : BASE_SPACESHIP_DATA
{
	U32 loadingRate;

	struct _dockTiming
	{
		SINGLE unloadRate;
		SINGLE offDist;
		SFX::ID dockingSound;
	}dockTiming;

	struct _nuggetTiming
	{
		SINGLE nuggetTime;
		SINGLE offDist;
		SINGLE minePerSecond;
		char hardpointName[32];
		SFX::ID nuggetSound;
	}nuggetTiming;

	char gasTankMesh[32];
	char metalTankMesh[32];
};

struct DYNAMICS_DATA_JR
{
	SINGLE maxLinearVelocity;
    SINGLE linearAcceleration;
    SINGLE maxAngVelocity;
};


struct BT_BUILDERSHIP_DATA : BASIC_DATA
{
	SPACESHIPCLASS type;
    char fileName[32];
	DYNAMICS_DATA_JR dynamicsData;
	char workAnimation[32];
	char sparkAnim[32];
	char sparkHardpoint[32];
	char explosionType[32];
	SINGLE sparkWidth;
	SINGLE sparkDelay;
};


































			    


























			    











struct MISSION_SAVELOAD
{
	__readonly M_OBJCLASS mObjClass;
	M_STRING partName;
	__readonly __hexview U32 dwMissionID;
	U32 systemID:8;		
	M_RACE race:4;
	U32  playerID:4;	

	SINGLE maxVelocity;
	SINGLE sensorRadius;
	SINGLE cloakedSensorRadius;
	U16 supplies;
	U16 hullPoints;
	U16	hullPointsMax;
	U16	supplyPointsMax;
	U16	numKills;
	bool bReady:1;
	bool bRecallFighters:1;		
	bool bMoveDisabled:1;		
	bool bPendingOpValid:1;		
	bool bUnselectable:1;		
	bool bDerelict:1;			
	bool bTowing:1;				
	bool bInvincible:1;			
	bool bNoAIControl:1;		
	bool bAllEventsOn:1;		
	bool bShowPartName:1;		
	bool bUnderCommand:1;		
	bool bNoAutoTarget:1;		
	U8	 controlGroupID; 
	U8	 groupIndex;			

	MISSION_DATA::M_CAPS caps;
	__readonly __hexview U32 groupID;				
	__readonly __hexview U32 admiralID;				
	__readonly __hexview U32 fleetID;				

	struct InstanceTechLevel
	{
		enum UPGRADE
		{
			LEVEL0=0,
			LEVEL1,
			LEVEL2,
			LEVEL3,
			LEVEL4,
			LEVEL5
		};

		UPGRADE engine:4;
		UPGRADE hull:4;
		UPGRADE supplies:4;
		UPGRADE targeting:4;
		UPGRADE damage:4;
		UPGRADE shields:4;
		UPGRADE experience:4;

		UPGRADE sensors:4;
		UPGRADE classSpecific:4;
	} techLevel;


	
	__readonly U32 lastOpCompleted;
	__readonly U32 pendingOp;		
};






struct BT_PLANET_DATA : BASIC_DATA
{
    char fileName[32];
	char ambient_animation[32];
	char sysMapIcon[32];
	char ambientEffect[32];
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
	char teraParticle[32];
	struct TeraColor
	{
		U8 red;
		U8 green;
		U8 blue;
	}teraColor;
	char teraExplosions[32];
	struct Halo
	{
		U8 red;
		U8 green;
		U8 blue;
		SINGLE sizeInner;
		SINGLE sizeOuter;
	}halo;

	bool bMoon:1;
	bool bUncommon:1;
};









struct BASE_PLANET_SAVELOAD
{
	U32			slotUser[12];
	U16 slotMarks[8];
	U16 mySlotMarks[8];
	U16 trueMarks[8];
	U8 shadowVisibilityFlags;
	U16 shadowMetal[8];
	U16 shadowCrew[8];
	U16 shadowGas[8];

	SINGLE playerMetalRate[8];
	SINGLE playerGasRate[8];
	SINGLE playerCrewRate[8];

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


struct PLANET_SAVELOAD : BASE_PLANET_SAVELOAD
{
	
	TRANS_SAVELOAD trans_SL;

	
	U8  exploredFlags;
	
	
	MISSION_SAVELOAD mission;
};




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
























			    


























			    




















struct DRONE_RELEASE
{
	char hardpoint[64];	
	char builderType[32];
	U8 numDrones;
};



struct BT_FABRICATOR_DATA : BASE_SPACESHIP_DATA
{
	struct DRONE_RELEASE drone_release[2];
	SINGLE repairRate;
	U32 maxQueueSize;
	SFX::ID buildSound;
	SFX::ID beginBuildSfx;

};









































			    

















struct EXTENSION_DATA
{
	char extensionName[32];
};

struct BT_EXTENSION_INFO
{
	char addChildName[32];
	char removeChildName[32];

	char archetypeName[32];
};









struct BASE_PLATFORM_DATA : BASIC_DATA
{
	PLATFORMCLASS type;
    char fileName[32];
	MISSION_DATA missionData;
	EXTENSION_DATA extension[4];
	U8 extensionBits;
	S8 extensionLevel;
	char explosionType[32];
	char shieldHitType[32];
	char ambient_animation[32];
	char ambientEffect[32];
	SINGLE mass;
	SINGLE_TECHNODE techActive;
	struct SHIELD_DATA shield;
	U32 slotsNeeded;
	struct BLINKER_DATA blinkers;
	U8 commandPoints;
	U32 metalStorage;
	U32 gasStorage;
	U32 crewStorage;
	U8  size;
	bool bMoonPlatform:1;
};


















































































			    



















struct BT_SUPPLYSHIP_DATA : BASE_SPACESHIP_DATA
{
	SINGLE suppliesPerSecond;
	SINGLE supplyRange;
};






























			    













struct BASE_WEAPON_DATA : BASIC_DATA
{
	WPNCLASS wpnClass;
};


struct BT_PROJECTILE_DATA : BASE_WEAPON_DATA
{
	char fileName[32];					
	SFX::ID launchSfx;
	U32 damage;								
	SINGLE maxVelocity;						
	char blastType[32];				

	char engineTrailType[32];			
};

struct BT_PLASMABOLT_DATA : BT_PROJECTILE_DATA
{
	char textureName[32];				
	char animName[32];					
	SINGLE numBolts;
	struct _pBolt
	{
		Vector rollSpeed;						
		SINGLE boltWidth;						
		SINGLE boltSpacing;						
		U32 segmentsX;							
		U32 segmentsY;							
		U32 segmentsZ;							
		Vector offset;
		struct _boltColor
		{
			U8 red,green,blue,alpha;
		}boltColor;
	}bolts[4];
};

struct SPECIAL_DAMAGE
{
	U32 supplyDamage;
	SINGLE shieldFraction;
	SINGLE moveFraction;
	SINGLE sensorFraction;
};

struct BT_SPECIALBOLT_DATA : BASE_WEAPON_DATA
{
	char fileName[32];					
	SFX::ID launchSfx;
	U32 damage;								
	SINGLE maxVelocity;						
	char blastType[32];				
	char sparkType[32];				
	FLASH_DATA flash;
	SINGLE MASS;
	SPECIAL_DAMAGE special;
};


struct BT_BEAM_DATA : BASE_WEAPON_DATA
{
	char fileName[32];					
	SFX::ID launchSfx;
	U32 damage;								
	SINGLE lifetime;						
	SINGLE maxSweepDist;					
	SINGLE beamWidth;
	struct _colorStruct
	{
		U8 red,green,blue;
	} blurColor;
	char contactBlast[32];
};



struct BASE_BEAM_SAVELOAD
{
	U32			targetID, ownerID;
	U32			systemID;
	Vector		startOffset,target1,target2,end;
	SINGLE		time;
	bool bHit;
};
struct BEAM_SAVELOAD : BASE_BEAM_SAVELOAD
{
	TRANS_SAVELOAD trans_SL;
};

struct BT_GATTLINGBEAM_DATA : BASE_WEAPON_DATA
{
	char fileName[32];					
	SFX::ID launchSfx;
	U32 damage;								
	SINGLE lifetime;						
	SINGLE maxSweepDist;					
	SINGLE beamWidth;
	char contactBlast[32];
};



struct BASE_GATTLINGBEAM_SAVELOAD
{
	U32			targetID, ownerID;
	U32			systemID;
	Vector		startOffset,target1,target2;
	SINGLE		time, lastTime;
	bool bHit;
};
struct GATTLINGBEAM_SAVELOAD : BASE_GATTLINGBEAM_SAVELOAD
{
	TRANS_SAVELOAD trans_SL;
};


struct BT_LASERSPRAY_DATA : BASE_WEAPON_DATA
{
	char fileName[32];					
	SFX::ID launchSfx;
	U32 damage;								
	SINGLE lifetime;						
	SINGLE maxSweepDist;					
	SINGLE beamWidth;
	SINGLE velocity;
	char contactBlast[32];
};



struct BASE_LASERSPRAY_SAVELOAD
{
	U32			targetID, ownerID;
	U32			systemID;
	Vector		startOffset,target1,target2,target3,target4,target5;
	SINGLE		time, lastTime;
	bool bHit;
};
struct LASERSPRAY_SAVELOAD : BASE_LASERSPRAY_SAVELOAD
{
	TRANS_SAVELOAD trans_SL;
};


struct BT_ANMBOLT_DATA : BT_PROJECTILE_DATA
{
	char animFile[32];					
	SINGLE boltSize;
};



struct BASE_ANMBOLT_SAVELOAD
{
	U32			targetID, ownerID;
	U32			systemID;
	Vector		initialPos;
	S32			flashTime;
	BOOL32		bDeleteRequested:1;
	U32			launchFlags:8;
	Vector		start, direction;
};
struct ANMBOLT_SAVELOAD : BASE_ANMBOLT_SAVELOAD
{
	TRANS_SAVELOAD trans_SL;
};


struct BASE_BOLT_SAVELOAD
{
	U32			targetID, ownerID;
	U32			systemID;
	Vector		initialPos;
	SINGLE		flashTime;
	BOOL32		bDeleteRequested:1;
	U32			launchFlags:8;
	Vector		start, direction;
};
struct BOLT_SAVELOAD : BASE_BOLT_SAVELOAD
{
	TRANS_SAVELOAD trans_SL;
};


struct BASE_MISSILE_SAVELOAD
{
	U32			targetID, ownerID;
	U32			systemID;
	Vector		initialPos;
	SINGLE		timeToLive;
	SINGLE		wobble;		
	BOOL32		bDeleteRequested:1;
	BOOL32		bTargetingOff:1;
	U32			launchFlags:8;

};
struct MISSILE_SAVELOAD : BASE_MISSILE_SAVELOAD
{
	TRANS_SAVELOAD trans_SL;
};

struct BT_ARC_DATA : BASE_WEAPON_DATA
{
	char fileName[32];					
	SFX::ID launchSfx;
	U32 damage;
	SINGLE jumpRange;							
	SINGLE feedbackRange;						
	U32 maxTendrilStages;						
	SINGLE damageDrop;							
	U32 maxTargets;								
	SINGLE period;								
	bool drainSupplies:1;							
};


struct ARC_SAVELOAD
{
	SINGLE time;
	S32 numTendrils;
	U32 ownerID;
	U32 systemID;
	TRANS_SAVELOAD trans_SL;
};






















			    











enum BLENDS
{
	ONE_ONE=0,
	SRC_INVSRC=1,
	SRC_ONE=2,
	ONE_INVSRC=3
};

struct BILLBOARD_MESH
{
    char mesh_name[32];
	char tex_name[32];
	S32 size_min,size_max;
	Vector offset;
	
	BLENDS blendMode;
};


struct BT_JUMPGATE_DATA : BASIC_DATA
{
	BILLBOARD_MESH billboardMesh[3];


	SFX::ID enter1,enter2;
	SFX::ID arrive1,arrive2;
	SFX::ID ambience;
	MISSION_DATA missionData;
	SINGLE min_hold_time;
	SINGLE min_stagger_time;
};
























			    





struct BT_UI_ANIM : BASIC_DATA
{
	char effectType[32];
	SINGLE totalTime;
	SFX::ID sfx;
};

struct BT_BLAST : BASIC_DATA
{
   


	char effectType[3][32];
	FLASH_DATA flash;
    SINGLE totalTime;
	SFX::ID sfx;
	SINGLE leadTime;
	bool bDrawThroughFog;
};



struct BT_MESH_EXPLOSION : BASIC_DATA
{
    char secondaryBlastType[32];
    char pieceBlastType[32];
    char catastrophicBlastType[32];
	char rippleBlastType[32];
	char fireTrail[32];
};






















			    





struct BASE_EFFECT : BASIC_DATA
{
	EFFECTCLASS fxClass;
};


struct BT_CLOAKEFFECT_DATA : BASE_EFFECT
{
	char animName[32];
	char objectName[32];
	SFX ::ID cloakOn;
	SFX ::ID cloakOff;
};


struct BT_FIREBALL_DATA : BASE_EFFECT
{
	char animName[32];
};


struct BT_PARTICLE_DATA : BASE_EFFECT
{
	char fileName[32];
};


struct BT_STREAK_DATA : BASE_EFFECT
{
	char animName[32];
	U8 numLines;
	SINGLE lineTime;
};


struct BT_WORMHOLE_EFFECT : BASE_EFFECT
{
};


struct BT_PARTICLE_CIRCLE : BASE_EFFECT
{
	struct Color
	{
		U8 red;
		U8 green;
		U8 blue;
	}color;
};


struct BT_NOVA_EXPLOSION : BASE_EFFECT
{
	SINGLE interRingTime;
	SINGLE ringTime;
	SINGLE range;
	SINGLE duration;
};


struct BT_TALORIAN_EFFECT : BASE_EFFECT
{
};


struct BT_ANIMOBJ_DATA : BASE_EFFECT
{
	char animName[32];
	SINGLE animSize;    
	SINGLE sizeVel;		
	SINGLE lifeTime;
	bool fadeOut:1;
	bool faceFront:1;
	bool bSmooth:1;
	bool bLooping:1;
	bool bHasAlphaChannel:1;
};

















			    















			    








struct RangeFinderSaveLoad
{
	SINGLE rangeError;
	SINGLE accuracy;
	U32 targetID;
};


struct BASE_LAUNCHER : BASIC_DATA
{
	LAUNCHCLASS type;

	char weaponType[32];		
	S32 supplyCost;
	SINGLE refirePeriod;

	U32 launcherSpecialID;
};








struct BT_TURRET : BASE_LAUNCHER
{
	union TurretJointInfo
	{
		SINGLE angVelocity;
		SINGLE maxAngDeflection;
	} info;
	
	char animation[32];
	char hardpoint[64];
	char joint[64];
	char animMuzzleFlash[32];
	U32 muzzleFlashWidth;
	SINGLE muzzleFlashTime;
	struct _colorMod
	{
		U8 red;
		U8 green;
		U8 blue;
	}colorMod;
	struct WarmUpBlast
	{
		char blastType[32];
		SINGLE triggerTime;
	} warmUpBlast;
};



struct BASE_TURRET_SAVELOAD
{
	U32 dwTargetID;
	Vector targetPos;
	SINGLE relYaw;				
	SINGLE refireDelay; 
	SINGLE currentRot;
	Vector offsetVector;	
};

struct TURRET_SAVELOAD : BASE_TURRET_SAVELOAD
{
	RangeFinderSaveLoad rangeFinderSaveLoad;
};

struct BT_EFFECT_LAUNCHER : BASE_LAUNCHER
{
	U32 weaponDamage;
	SINGLE weaponFireDelay;
	SINGLE weaponVelocity;
};



struct BASE_EFFECT_LAUNCHER_SAVELOAD
{
	U32 dwTargetID;
	Vector targetPos;
	SINGLE refireDelay; 
};

struct EFFECT_LAUNCHER_SAVELOAD : BASE_EFFECT_LAUNCHER_SAVELOAD
{
};




















			    















struct BT_VERTICAL_LAUNCH : BASE_LAUNCHER
{
	struct _salvo
	{
		SINGLE_TECHNODE techNeed;
		U32 salvo;		
	}upgrade[4];
	SINGLE miniRefire;	
	char hardpoint[10][64];
};




struct VLAUNCH_SAVELOAD
{
	U32 dwTargetID, dwOwnerID;
	Vector targetPos;
	U32 attacking;		
	SINGLE refireDelay; 
	S32 miniDelay;
	S32 salvo;
};





















			    









struct BT_WAYPOINT : BASIC_DATA
{
	char fileName[32];
	MISSION_DATA missionData;
};



struct WAYPOINT_SAVELOAD
{
	
	TRANS_SAVELOAD trans_SL;

	
	MISSION_SAVELOAD mission;
};






struct WAYPOINT_VIEW 
{
	MISSION_SAVELOAD * mission;
	Vector position;
};





















			    













enum MINETYPE
{
	MINETYPE_UNKOWN = 0,
	MINETYPE_SPINE
};


struct BT_MINEFIELD_DATA : BASIC_DATA
{
	MISSION_DATA missionData;
	U32 maxMineNumber;
	U32 damagePerHit;
	U32 supplyDamagePerHit;
	U32 hullLostPerHit;
	MINETYPE mineType;
	char regAnimation[32];
	char blastType[32];
	U32 mineWidth;
	S32 maxVerticalVelocity;
	S32 maxHorizontalVelocity;
	U32 mineAcceleration;
	SINGLE explosionRange;
	ResourceCost cost;
};


struct BASE_MINEFIELD_SAVELOAD
{
	struct GRIDVECTOR gridPos;
	SINGLE storedDelay;
	U8 updateCounter;

	U32 layerRef;

	bool bUpdateFading:1;
	bool bDetonating:1;
};


struct MINEFIELD_SAVELOAD : BASE_MINEFIELD_SAVELOAD
{
	
	TRANS_SAVELOAD trans_SL;

	
	MISSION_SAVELOAD mission;
};



struct MINEFIELD_VIEW 
{
	U32 mineNumber;
};





















			    












struct BT_FIGHTER_DATA : BASIC_DATA
{
    char fileName[32];
    DYNAMICS_DATA dynamicsData;
	ARMOR_DATA  armorData;
	char explosionType[32];
	char weaponType[32];
	char engineTrailType[32];

	U16		 sensorRadius;			
	U16		 cloakedSensorRadius;	
	SINGLE   patrolRadius;
	SINGLE   patrolAltitude;
	SINGLE	 attackRange;
	SINGLE	 dodge;
	U32		 hullPointsMax;			
	U32	     maxSupplies;			
	SINGLE 	 refirePeriod;			
	SINGLE	 patrolPeriod;			
	U32		 kamikazeDamage;		
	SFX::ID	 kamikazeYell;			
	SFX::ID	 fighterwhoosh;			
	SFX::ID	 fighterLaunch;			
};


enum FighterState
{
	ONDECK,
	PATROLLING,
	DEAD
};


enum FormationType
{
	FT_HUMAN_PATROL,
	FT_MANTIS_PATROL,
	FT_HUMAN_ATTACK,
	FT_MANTIS_ATTACK
};


struct BASE_FIGHTER_SAVELOAD
{
	
	U32 ownerID, dwMissionID;
	U32 systemID:8;
	U32 playerID:8;			
	S32 supplies:16;
	U8  formationIndex;		
	S8  parentIndex;		
	S8  childIndex;			
	S8  myIndex;			
	S32 refireTime:16;
	S32 patrolTime:16;
	S32 generalCounter;
	Vector kamikaziTarget;	
	FighterState   state:8;
	FormationType  formationType:8;
	M_RACE race:8;
	U8  patrolState;		
	U8   formationState;		
	U8   kamikazeTimer;		
	U8   bCirclePatrol:1;	
	bool bKamikaziTargetSelected:1;		
	bool bKamikaziYellComplete:1;	
	bool bKamikaziComplete:1;		
};

struct FIGHTER_SAVELOAD : BASE_FIGHTER_SAVELOAD
{
	TRANS_SAVELOAD   trans_SL;
};






























			    













struct BT_FIGHTER_WING : BASE_LAUNCHER
{
	SINGLE baseAirAccuracy;		
	SINGLE baseGroundAccuracy;  
	U32	   maxFighters;			
	U32	   maxCapFighters;		
	U32	   maxWingFighters;		
	SINGLE minLaunchPeriod;		
	U32	   costOfNewFighter;		
	U32	   costOfRefueling;		

	bool bSpecialWeapon;		
	SINGLE_TECHNODE neededTech;

	char animation[32];	
	char hardpoint[64];	
};


struct FIGHTERWING_SAVELOAD
{
	U32  firstFighterID;
	bool bBayDoorOpen:1;
	bool bRecallingFighters:1;
	bool bAutoAttack:1;
	bool bSpecialAttack:1;
	bool bSpecialWeapon:1;
	bool bTakeoverInProgress:1;
	S32	 createTime;			
	S32	 minLaunchTime;		
	S32	 bayDoorCloseTime;	
};


















			    










struct BT_AIR_DEFENSE : BASE_LAUNCHER
{
	char hardpoint[64];
	SINGLE baseAccuracy;		
	char flashTextureName[32];
	SINGLE flashWidth;
	SINGLE flashFrequency;
	struct _colorMod
	{
		U8 red,green,blue,alpha;
	} flashColor;
	SFX::ID soundFx;
};



struct AIRDEFENSE_SAVELOAD
{
	S32 refireTime;		
};


















			    








struct BT_SHIPLAUNCH : BASE_LAUNCHER
{
	char hardpoint[64];
	char animation[32];
};




struct SHIPLAUNCH_SAVELOAD
{
	U32 dwTargetID, dwOwnerID;
	Vector targetPos;
	SINGLE relYaw;				
	SINGLE distanceToTarget;	
	U32 attacking;		
	SINGLE refireDelay; 
	SINGLE currentRot;
	Vector offsetVector;	
};






















			    



















			    









struct MT_QSHIPLOAD
{
	SINGLE yaw;
	__hexview U32 dwMissionID;
	NETGRIDVECTOR pos;
};

struct MT_QJGATELOAD
{
	NETGRIDVECTOR pos;
	U32 gate_id,exit_gate_id;
	bool bJumpAllowed:1;
};



struct MT_QFIELDLOAD
{
	U32 systemID;
	U8 numSquares;
	GRIDVECTOR pos[40];
};




struct MT_QANTIMATTERLOAD
{
	U32 systemID;
	U8 numSegments;
	GRIDVECTOR pts[13];
};

struct MT_QCAMERALOAD
{
	U32 systemID;
	Vector look;
	Vector at;
	SINGLE fovX;
	SINGLE fovY;
};

struct MT_QOBJGENERATOR_LOAD
{
	NETGRIDVECTOR position;
	SINGLE mean;
	SINGLE minDiff;
	SINGLE timer;
	SINGLE nextTime;
	char archTypeName[32];
	__hexview U32 dwMissionID;
	bool bGenEnabled:1;
};

struct MT_PLANET_QLOAD
{
	NETGRIDVECTOR pos;
};

struct MT_TRIGGER_QLOAD
{
	NETGRIDVECTOR position;

	U32 triggerShipID;
	U32 triggerObjClassID;
	U32 triggerMObjClassID;
	U32 triggerPlayerID;
	SINGLE triggerRange;
	U32 triggerFlags;
	char progName[32];
	bool bEnabled:1;
	bool bDetectOnlyReady:1;
};

struct MT_PLAYERBOMB_QLOAD
{
	__hexview U32 dwMissionID;
	NETGRIDVECTOR position;
	bool          bNoExplode;
};

struct MT_LIGHT_QLOAD
{
	U8 red,green,blue;
	S32 range;
	NETGRIDVECTOR position;
	Vector direction;
	SINGLE cutoff;
	bool bInfinite;
	char name[32];
	bool bAmbient;
};

struct MT_SCRIPTOBJECT_QLOAD
{
	__hexview U32 dwMissionID;
	NETGRIDVECTOR position;
};

struct MT_WAYPOINT_QLOAD
{
	__hexview U32 dwMissionID;
	NETGRIDVECTOR position;
};

struct MT_NUGGET_QLOAD
{
	NETGRIDVECTOR position;
};

struct MT_PLATFORM_QLOAD
{
	__hexview U32 dwMissionID;
	NETGRIDVECTOR position;
};

struct MT_BLACKHOLE_QLOAD
{
	NETGRIDVECTOR pos;

	U8 targetSys[16];
	U8 numTargetSys;
};

struct MT_OBJECTFAMILY_QLOAD
{
	U32  numObjects;
	char name[32];
};

struct MT_OBJECTFAMILYENTRY_QLOAD
{
	char scriptHandle[32];
};

struct MT_FIELDLINK
{
	char scriptHandle[32];
	char field[32];
	char groupname[32];
};










struct XYCoord
{
	S32 x,y;
};

struct FIELD_ATTRIBUTES
{
	SINGLE sensorDamp;
	SINGLE toHitPenalty;
	SINGLE damage;
	SINGLE moveSpeedModifier;
	SINGLE maneuverModifier;
};


namespace FIELDTXT
{
enum INFOHELP
{
	NOTEXT = 3003,
	FIELD_ION = 3004,
	FIELD_PLASMA = 3012,
	FIELD_ASTEROID_SML = 3013,
	FIELD_ASTEROID_MED = 3117,
	FIELD_ASTEROID_LG = 3118,
	FIELD_DEBRIS = 3127,
	FIELD_ANTIMATTER = 3040,
	FIELD_ANTI_NEBULA = 3125,
	FIELD_CELSIUS = 3114,
	FIELD_HELIOS = 3121,
	FIELD_CYGNUS = 3122,
	FIELD_HYADES = 3123,
	FIELD_LITHIUM = 3124,
	FIELD_BLACKHOLE = 3126,
	FIELD_ORENUGGET = 3128,
	FIELD_GASNUGGET = 3129
};
}


struct BASE_FIELD_DATA : BASIC_DATA
{
	FIELDCLASS fieldClass;
	FIELDTXT::INFOHELP   infoHelpID;		
};

struct U8_RGB
{
	U8 r;
	U8 g;
	U8 b;
};


struct BT_ASTEROIDFIELD_DATA : BASE_FIELD_DATA
{
	char fileName[4][32];
	char dustTexName[32];
	char modTextureName[32];
	char mapTexName[32];

	char softwareTexClearName[32];
	char softwareTexFogName[32];

	MISSION_DATA missionData;
	FIELD_ATTRIBUTES attributes;
	S32 asteroidsPerSquare;
	S32 polyroidsPerSquare;
	S32 depth;
	S32 range;
	S32 maxDriftSpeed;
	S32 minDriftSpeed;
	SINGLE stationaryPercentage;
	SFX::ID ambientSFX;
	SINGLE modTexSpeedScale;
	S32 animSizeMin,animSizeMax;
	U32 nuggetsPerSquare;
	SINGLE nuggetZHeight;
	char nuggetType[4][32];
	U8_RGB animColor;
};



struct ASTEROIDFIELD_SAVELOAD
{
	
	TRANS_SAVELOAD trans_SL;

	
	MISSION_SAVELOAD mission;

	U32 exploredFlags;
};

struct ASTEROIDFIELD_DATA
{
	char name[32];
};

struct ANTIMATTER_DATA
{
	char name[32];
	GRIDVECTOR pts[13];
	U32 numSegPts;
};

struct BT_ANTIMATTER_DATA : BASE_FIELD_DATA
{
	char textureName[32];
	char mapTexName[32];
	char softwareTexClearName[32];
	char softwareTexFogName[32];
	S32 height;
	S32 segment_width;
	S32 spacing;

	MISSION_DATA missionData;
};

struct ANTIMATTER_SAVELOAD
{
	
	TRANS_SAVELOAD trans_SL;

	
	MISSION_SAVELOAD mission;
};





















			    
















struct BT_TRAIL_DATA : BASIC_DATA
{
    char anim[32];
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











struct NEBULA_DATA
{
	char name[32];

	U8 amb_r,amb_g,amb_b;
	U8 alpha;
	SINGLE top_layer_alpha_scale;
};



struct BASE_NEBULA_SAVELOAD
{
	NEBULA_DATA neb;
	U32 numSquares, numZones, numNuggets;
	XYCoord squares_xy[40];
};


struct NEBULA_SAVELOAD : BASE_NEBULA_SAVELOAD
{	
	
	TRANS_SAVELOAD trans_SL;

	
	MISSION_SAVELOAD mission;

	U32 exploredFlags;
};

struct LIGHTNING_DATA
{
	bool bLightning:1;
	bool bFlat:1;
	char lightningTexName[32];
	SFX::ID lightningSFX;
	SINGLE frequency;
	U16 size;
	struct LIGHTNING_LIGHT
	{
		U8 r,g,b;
		S32 range;
	} lightning_light;
};



struct BT_NEBULA_DATA : BASE_FIELD_DATA
{
	char cloudEffect[32];
	char mapTexName[32];

	MISSION_DATA missionData;
	FIELD_ATTRIBUTES attributes;

	struct AMBIENT_NEBULA_LIGHT
	{
		U8 r,g,b;
		SINGLE pulse_frequency;
	} ambient;

	SFX::ID ambientSFX;
	NEBTYPE nebType;

	U32 nuggetsPerSquare;
	SINGLE nuggetZHeight;
	char nuggetType[4][32];
};



















			    



















struct GT_GLOBAL_SOUNDS
{
	SFX::ID defaultButton;
	SFX::ID systemSelect;
	SFX::ID buildConfirm;
	SFX::ID moveConfirm;
	
	SFX::ID startConstuction; 
	SFX::ID endConstruction; 
	SFX::ID zeroMoney; 
	SFX::ID lightindustryButton; 
	SFX::ID heavyindustryButton; 
	SFX::ID hitechindustryButton; 
	SFX::ID hqButton; 
	SFX::ID researchButton; 
	SFX::ID planetDepleted; 
	SFX::ID harvestRedeploy; 
	SFX::ID researchCompleted; 
};























			    















































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































struct GT_FONT : GENBASE_DATA
{
	enum FONT
	{
		BUTTON_FONT			= 907,
		DESCRIPTION_FONT	= 909,
		STATIC_CONTROL		= 910,
		GAMEROOM_LIST		= 911,
		CHATLIST_FONT		= 912,
		OBJTYPE_FONT		= 913,			
		GAMEROOM_DROPDOWN	= 914,
		MONEY_FONT			= 906,
		CONTEXTWINDOW		= 915,
		ENDGAMEBANNER		= 916,
		DEFLISTBOX			= 902,
		MESSAGEBOX_FONT		= 918,
		OPENING_FONT		= 919,
		FUTURE_PLAYER		= 921,
		FUTURE_LISTBOX		= 920,
		PLANET_TITLE		= 922,
		PLANET_INFO			= 928,
		DROP_SMALL			= 923,
		OCR_16_700			= 924,
		NEURO_14_600		= 925,
		COURIER_12_400		= 926,
		NEURO_12_400		= 927,
		OBJECTIVES			= 929,
		LEGAL_FONT			= 903,
		COUNTDOWN			= 930,
		BUTTON3D			= 932
	} font;

	bool bMultiline:1;
	bool bNotScaling:1;		
	bool bToolbarMoney:1;	
};




















			    
















struct GT_ANIMATE : GENBASE_DATA
{
    char vfxType[32];                        
};


struct ANIMATE_DATA
{
    char animateType[32];
    S32 xOrigin, yOrigin;
	U32 dwTimer;		
	bool bFuzzEffect;
};


















			    














namespace BTNTXT
{
enum BUTTON_TEXT
{
	NOTEXT				= 0,
	TEST				= 1101,
	SINGLE_PLAYER		= 1102,
	MULTI_PLAYER		= 1103,
	OPTIONS				= 1104,
	BACK				= 1105,
	JOIN				= 1108,
	CREATE				= 1109,
	NEXT				= 1111,
	QUIT				= 1112,
	CANCEL				= 1113,
	YES					= 1115,
	NO					= 1116,
	FINAL_MENU			= 1120,
	ACCEPT				= 1121,
	START				= 1122,
	RESUME				= 1117,
	OK					= 1124,
	SETTINGS			= 1118,
	OPEN				= 1119,
	SAVE				= 1123,
	CONTINUE			= 1125,
	RANDOM_MAP			= 1635,
	LOAD				= 1126,
	GAME_OPTIONS		= 1104,
	MISSION_OBJECTIVES	= 1128,
	END_MISSION			= 1129,
	RESTART				= 1130,
	RESIGN				= 1131,
	ABDICATE			= 1162,
	RETURN_TO_GAME		= 1132,
	GAME				= 1133,
	SOUND				= 1134,
	GRAPHICS			= 1135,
	NEW_PLAYER			= 1138,
	DELETE_PLAYER		= 1139,
	LOAD_SAVED			= 1140,
	LOAD_CUSTOM			= 1141,
	TERRAN				= 1142,
	MANTIS				= 1143,
	NEW_CAMPAIGN		= 1144,
	REPLAY				= 1145,
	INTRODUCTION		= 1106,
	SKIRMISH			= 1107,
	REPAIR_FLEET		= 1146,
	RESUPPLY_FLEET		= 1147,
	DISBAND_FLEET		= 1148,
	CREATE_FLEET		= 1149,
	TRANSFER_FLAGSHIP	= 1150,
	RESET				= 1151,
	CLOSE				= 1152,
	ALLIES				= 1154,
	ENEMIES				= 1153,
	EVERYONE			= 1155,
	CHANGE_NAME			= 1156,
	CAMPAIGN			= 1473,
	TERRAN_CAMP			= 1157,
	MANTIS_TRAINING		= 1158,
	SOLARIAN_TRAINING	= 1159,
	ZONEGAME			= 1605,
	WEBPAGE				= 1623,
	HELP				= 1497,
	MAP_TYPE			= 1166,
	APPLY				= 1114,
	LOAD_QUICKBATTLE	= 1160,
	CREDITS				= 1127,
	DELETE_FILE			= 5124
};
} 






















struct GT_BUTTON : GENBASE_DATA
{
	enum BUTTON_TYPE
	{
		DEFAULT,
		PUSHPIN,			
		CHECKBOX,			
		REPEATER,			
	} buttonType:8;

	S32 leftMargin:8;		
	S32 rightMargin:8;

	char fontName[32];
	struct COLOR
	{
		unsigned char red, green, blue;
	} disabledText, normalText, highlightText;

	char shapeFile[32];			
};


struct BUTTON_DATA
{
	char buttonType[32];
	BTNTXT::BUTTON_TEXT buttonText;		
	S32 xOrigin, yOrigin;
	RECT buttonArea;					
};



















			    





























struct GT_DIPLOMACYBUTTON : GENBASE_DATA
{
	S32 leftMargin:8;		
	S32 topMargin:8;

	char shapeFile[32];			
};


struct DIPLOMACYBUTTON_DATA
{
	char buttonType[32];
	S32 xOrigin, yOrigin;
};



















			    












namespace STTXT
{
enum STATIC_TEXT
{

  NO_STATIC_TEXT		= 0,
  TEST					= 1101,
  PLAYER_NAME			= 1401,
  IP_ADDRESS			= 1402,
  SESSION_NAME			= 1404,
  CONFIRM_QUIT			= 1406,
  TITLE_SELECT_GAME		= 1407,
  PLAYER				= 1408,
  SESSION				= 1409,
  ENTER_TEAM			= 1412,
  ENTER_RACE			= 1410,
  ENTER_COLOR			= 1411,
  SELECTED_MAP			= 1414,
  CHATBOX				= 1415,
  GAME_PAUSED			= 1416,
  INGAME_TITLE			= 1417,
  OPTIONS				= 1418,
  GAMETYPE				= 1419,
  GAMESPEED				= 1420,
  CURSOR_SENSITIVITY	= 1463,
  ARTIFACTS				= 1421,
  MONEY					= 1422,
  RESEARCH				= 1423,
  MAPTYPE				= 1424, 
  MAPFILE				= 1451,
  MAPSIZE				= 1425,
  TERRAIN				= 1426,
  STATIC_RESOURCES		= 1427,
  WARNBOOT				= 1329,
  SOLOGAME				= 1332,
  LOADGAME				= 1431,
  SAVEGAME				= 1432,
  FILENAME				= 1433,
  FLEET_SIZE			= 1434,
  SKILL					= 1435,
  BONUS					= 1436,
  DUMMY_MONEY			= 10001,
  DUMMY_NAME			= 10002,
  SHIPCLASS				= 1440,
  HULL					= 1441,
  SUPPLIES				= 1442,
  ARMOR					= 1443,
  ENGINES				= 1444,
  WEAPONS				= 1445,
  SHIELDS				= 1446,
  KILLS					= 1447,
  UNITS					= 1449,
  INGAME_CHAT			= 1450,
  MISSION_OBJECTIVES	= 1128,
  END_MISSION			= 1129,
  SCOREBOARD			= 1452,
  SCORE					= 1453,
  VOLUME				= 1403,
  MUSIC					= 1454,
  EFFECTS				= 1455,
  COMMS					= 1456,
  NETCHAT				= 1457,
  TOOLTIPS				= 1458,
  STATUS_TEXT			= 1459,
  FOGOFWAR				= 1460,
  MAPROTATE				= 1461,
  ROLLOVER				= 1462,
  SLOW					= 1625,
  FAST					= 1627,
  GAMMA					= 1465,
  WEAPON_TRAILS			= 1466,
  EMMISIVE_TEXTURES		= 1467,
  LOD					= 1468,
  TEXTURES				= 1469,
  BACKGROUND			= 1470,
  DEVICES				= 1464,
  ENTER_NAME			= 1472,
  CAMP_SELECT			= 1473,
  SINGLE_PLAYER			= 1102,
  MULTI_PLAYER			= 1103,
  FRONT_OPTIONS			= 1104,
  SKIRMISH				= 1474,
  INTRO					= 1475,
  SELECT_MISSION		= 1058,
  RIGHTCLICK			= 1476,
  SUBTITLES				= 1563,
  JOIN					= 1108,
  CREATE				= 1109,
  ACCEPT				= 1121,
  SCROLLRATE			= 1488,
  QUIT					= 1112,
  RESOLUTION			= 1493,
  ENTER_STATE			= 1494,
  PLAYER_LIST			= 1495,
  PING					= 1496,
  APP_NAME				= 44,
  VERSION				= 1428,
  HELP					= 1497,
  NET_CONNECT			= 1489,
  HARDWARE				= 1492,
  GAME					= 1498,
  SPEED					= 1499,
  MAP					= 1413,
  RESOURCES				= 1471,
  BRIEFING				= 1165,
  VISIBILITY			= 1504,
  DEVICE_CHOICE			= 1505,
  SOFTWARE				= 1506,
  MAP_RANDOM			= 1510,
  MAP_CONQUEST			= 1511,
  MAP_SAVED				= 1512,
  HARDWARE_CURSOR		= 1513,
  PRODUCT_ID			= 1514,
  REGENERATE			= 1517,
  SPECTATORS			= 1515,
  LOCK_ALLIANCES		= 1516,
  LOCK_SETTINGS			= 1518,
  ORE					= 1522,
  GAS					= 1523,
  CREW					= 1524,
  ALLIED				= 1525,
  DIPLOMACY				= 1526,
  NEW_PLAYER			= 1527,
  TITLE_UNITS			= 1528,
  TITLE_BUILDINGS		= 1529,
  TITLE_TERRITORY		= 1530,
  TITLE_RESOURCES		= 1531,
  TITLE_TOTAL			= 1532,
  TITLE_MADE			= 1533,
  TITLE_LOST			= 1534,
  TITLE_KILLS			= 1535,
  TITLE_CONVERTED		= 1536,
  TITLE_ADMIRALS		= 1537,
  TITLE_DESTROYED		= 1538,
  TITLE_RESEARCH		= 1539,
  TITLE_CREW			= 1540,
  TITLE_ORE				= 1541,
  TITLE_GAS				= 1542,
  TITLE_JUMPGATES		= 1543,
  TITLE_PLANETS			= 1544,
  TITLE_EXPLORED		= 1545,
  LOAD_MISSION_DATA		= 1546,
  LEGAL_MAINSCREEN		= 41,
  LEGAL_ABOUT			= 42,
  NUM_SYSTEMS			= 1437,
  BATLEMAP				= 1166,
  DISABLED				= 61729,
  STATIC_TERRAIN		= 1553,
  CHAT					= 1555,
  DIFFICULTY			= 1521,
  EASY					= 1557,
  MEDIUM				= 1558,
  HARD					= 1559,
  SHIPS3D				= 1560,
  BANDWIDTH				= 5123
};

enum STATIC_TOOLTIP
{
  NO_TOOLTIP            = 0,
  ORE_TOOLTIP			= 807,
  GAS_TOOLTIP			= 808,
  CREW_TOOLTIP			= 809,
  COMMAND_POINTS		= 813,
  DISABLED_TOOLTIP		= 816,

};

enum STATIC_HINTBOX
{
	NO_HINTBOX			= 0,
	ORE_HINTBOX			= 3306,
	GAS_HINTBOX			= 3307,
	CREW_HINTBOX		= 3308,
	CP_HINTBOX			= 3309,
	DISABLED_HINTBOX	= 3310,

};

} 




struct GT_STATIC : GENBASE_DATA
{
	char fontName[32];
	struct COLOR
	{
		unsigned char red, green, blue;
	} normalText, background;
    char shapeFile[32];                        

	enum drawtype
	{
		NODRAW,
		FILL,
		HASH
	} backgroundDraw;

	bool bBackdraw;
};


struct STATIC_DATA
{
    char staticType[32];
	STTXT::STATIC_TEXT staticText;         
	STTXT::STATIC_TOOLTIP staticTooltip;
	STTXT::STATIC_HINTBOX staticHintbox;
	enum alignmenttype
	{
		LEFT,
		CENTER,
		RIGHT,
		TOPLEFT,
		TENBYSIX,
		LEFTSIX
	} alignment;
    S32 xOrigin, yOrigin;
	S32 width, height;				
};






















			    



















			    

























namespace HOTKEYS
{
enum HOTKEY
{
	NONE,
	IDH_ADMIRAL_TACTIC_DEFEND,
	IDH_ADMIRAL_TACTIC_HOLD,
	IDH_ADMIRAL_TACTIC_PEACE,
	IDH_ADMIRAL_TACTIC_SEEK,
	IDH_BUILD_PLATS,
	IDH_CANCEL_COMMAND,
	IDH_CENTER_SCREEN,
	IDH_CHATALL,
	IDH_CHATTEAM,
	IDH_CHAT_MENU,
	IDH_CLOAK,
	IDH_CONTEXT_A,
	IDH_CONTEXT_B,
	IDH_CONTEXT_C,
	IDH_CONTEXT_D,
	IDH_CONTEXT_E,
	IDH_CONTEXT_F,
	IDH_CONTEXT_G,
	IDH_CONTEXT_H,
	IDH_CONTEXT_I,
	IDH_CONTEXT_J,
	IDH_CONTEXT_K,
	IDH_CONTEXT_L,
	IDH_CONTEXT_M,
	IDH_CONTEXT_N,
	IDH_CONTEXT_O,
	IDH_CONTEXT_P,
	IDH_CONTEXT_Q,
	IDH_CONTEXT_R,
	IDH_CONTEXT_S,
	IDH_CONTEXT_T,
	IDH_CONTEXT_U,
	IDH_CONTEXT_V,
	IDH_CONTEXT_W,
	IDH_CONTEXT_X,
	IDH_CONTEXT_Y,
	IDH_CONTEXT_Z,
	IDH_CYCLE_ADMIRALS,
	IDH_CYCLE_FLEETS,
	IDH_DEBUG_PRINT,
	IDH_DEFEND,
	IDH_DESTROY,
	IDH_DIPLOMACY,
	IDH_DISBAND_FLEET,
	IDH_EJECT_ARTIFACT,
	IDH_FABRICATOR,
	IDH_FIGHTER_NORMAL,
	IDH_FIGHTER_PATROL,
	IDH_FLEET_GROUP_1,
	IDH_FLEET_GROUP_2,
	IDH_FLEET_GROUP_3,
	IDH_FLEET_GROUP_4,
	IDH_FLEET_GROUP_5,
	IDH_FLEET_GROUP_6,
	IDH_FLEET_SET_GROUP_1,
	IDH_FLEET_SET_GROUP_2,
	IDH_FLEET_SET_GROUP_3,
	IDH_FLEET_SET_GROUP_4,
	IDH_FLEET_SET_GROUP_5,
	IDH_FLEET_SET_GROUP_6,
	IDH_FLEET_SPORDER_0,
	IDH_FLEET_SPORDER_1,
	IDH_FLEET_SPORDER_10,
	IDH_FLEET_SPORDER_11,
	IDH_FLEET_SPORDER_2,
	IDH_FLEET_SPORDER_3,
	IDH_FLEET_SPORDER_4,
	IDH_FLEET_SPORDER_5,
	IDH_FLEET_SPORDER_6,
	IDH_FLEET_SPORDER_7,
	IDH_FLEET_SPORDER_8,
	IDH_FLEET_SPORDER_9,
	IDH_FORMATION_1,
	IDH_FORMATION_2,
	IDH_FORMATION_3,
	IDH_FORMATION_4,
	IDH_FORMATION_5,
	IDH_FORMATION_6,
	IDH_FORM_FLEET,
	IDH_FULLSCREEN_MAP,
	IDH_GAME_OPTIONS,
	IDH_GROUP_0_CREATE,
	IDH_GROUP_0_SELECT,
	IDH_GROUP_1_CREATE,
	IDH_GROUP_1_SELECT,
	IDH_GROUP_2_CREATE,
	IDH_GROUP_2_SELECT,
	IDH_GROUP_3_CREATE,
	IDH_GROUP_3_SELECT,
	IDH_GROUP_4_CREATE,
	IDH_GROUP_4_SELECT,
	IDH_GROUP_5_CREATE,
	IDH_GROUP_5_SELECT,
	IDH_GROUP_6_CREATE,
	IDH_GROUP_6_SELECT,
	IDH_GROUP_7_CREATE,
	IDH_GROUP_7_SELECT,
	IDH_GROUP_8_CREATE,
	IDH_GROUP_8_SELECT,
	IDH_GROUP_9_CREATE,
	IDH_GROUP_9_SELECT,
	IDH_HARVEST_STANCE_GAS,
	IDH_HARVEST_STANCE_NONE,
	IDH_HARVEST_STANCE_ORE,
	IDH_LOAD_MENU,
	IDH_MISSION_OBJ,
	IDH_MULTIPLAYER_SCORE,
	IDH_NEXT_UNIT,
	IDH_PATROL,
	IDH_PAUSE_GAME,
	IDH_PREV_UNIT,
	IDH_QUICK_LOAD,
	IDH_QUICK_SAVE,
	IDH_RALLY_POINT,
	IDH_RECORD_SPEECH,
	IDH_REPAIR,
	IDH_REPAIR_FLEET,
	IDH_REPORT_STATS,
	IDH_RESEARCH_PLAT,
	IDH_RESUPPLY_FLEET,
	IDH_RES_KIT_0,
	IDH_RES_KIT_1,
	IDH_RES_KIT_10,
	IDH_RES_KIT_11,
	IDH_RES_KIT_12,
	IDH_RES_KIT_13,
	IDH_RES_KIT_2,
	IDH_RES_KIT_3,
	IDH_RES_KIT_4,
	IDH_RES_KIT_5,
	IDH_RES_KIT_6,
	IDH_RES_KIT_7,
	IDH_RES_KIT_8,
	IDH_RES_KIT_9,
	IDH_ROTATE_0_WORLD,
	IDH_ROTATE_90_WORLD_LEFT,
	IDH_ROTATE_90_WORLD_RIGHT,
	IDH_ROTATE_WORLD_LEFT,
	IDH_ROTATE_WORLD_RIGHT,
	IDH_SAVE_MENU,
	IDH_SCREEN_SHOT,
	IDH_SCRIPT_KEY_1,
	IDH_SCRIPT_KEY_2,
	IDH_SCRIPT_KEY_3,
	IDH_SCRIPT_KEY_4,
	IDH_SCRIPT_KEY_5,
	IDH_SCROLL_DOWN,
	IDH_SCROLL_DOWNLEFT,
	IDH_SCROLL_DOWNRIGHT,
	IDH_SCROLL_LEFT,
	IDH_SCROLL_RIGHT,
	IDH_SCROLL_UP,
	IDH_SCROLL_UPLEFT,
	IDH_SCROLL_UPRIGHT,
	IDH_SEEK_EVENT,
	IDH_SELECT_ALL_FLEET,
	IDH_SELECT_IDLE_CIVILIAN,
	IDH_SELECT_IDLE_MILITARY,
	IDH_SELL,
	IDH_SPECIAL_ABILITY,
	IDH_SPECIAL_ABILITY1,
	IDH_SPECIAL_ABILITY2,
	IDH_SPECIAL_AEGIS,
	IDH_SPECIAL_ASSAULT,
	IDH_SPECIAL_AUGER,
	IDH_SPECIAL_BOMBER,
	IDH_SPECIAL_CLOAK,
	IDH_SPECIAL_DESTABILIZER,
	IDH_SPECIAL_FURYRAM,
	IDH_SPECIAL_MASSDISRUPT,
	IDH_SPECIAL_MIMIC,
	IDH_SPECIAL_MINELAYER,
	IDH_SPECIAL_PROBE,
	IDH_SPECIAL_PROTEUSMINE,
	IDH_SPECIAL_REPEL,
	IDH_SPECIAL_REPULSOR,
	IDH_SPECIAL_SHROUD,
	IDH_SPECIAL_STASIS,
	IDH_SPECIAL_SYNTHESIS,
	IDH_SPECIAL_TEMPEST,
	IDH_SPECIAL_VAMPIRE,
	IDH_SPECIAL_WORMHOLE,
	IDH_STANCE_ATTACK,
	IDH_STANCE_DEFENSIVE,
	IDH_STANCE_IDLE,
	IDH_STANCE_STANDGROUND,
	IDH_SUPPLY_STANCE_FULLYAUTO,
	IDH_SUPPLY_STANCE_NOAUTO,
	IDH_SUPPLY_STANCE_RESUPPLY,
	IDH_TARGET_POSITION,
	IDH_TEST_SPEECH,
	IDH_TOGGLE_SCREENMODE,
	IDH_TOGGLE_TOOLBAR,
	IDH_TOGGLE_ZOOM,
	IDH_TRANSFER_FLAGSHIP,
	IDH_USE_ARTIFACT,
	IDH_WIN_CLOSE,
	IDH_ZOOM_IN,
	IDH_ZOOM_OUT
}; 
}  






























































































































































































































			    












namespace HOTBUTTONTYPE
{
enum TYPE
{
	HOTBUTTON = 0,
	BUILD,
	RESEARCH,
	MULTI,
	TABBUTTON
};
};

namespace HBTNTXT
{




enum BUTTON_TEXT		
{
	NOTEXT = 0,
	ROTATE_LEFT = 409,
	ROTATE_RIGHT = 410,
	MINIMIZE_TB = 477,
	RESTORE_TB = 478,
	OPTIONS = 479,
	HOMEWORLD = 402,
	FLEET = 403,
	LT_INDUSTRY = 404,
	HV_INDUSTRY = 406,
	HITECH = 407,
	RESEARCH = 411,
	DIPLOMACY = 408,
	NEXT = 480,
	PREVIOUS = 481,
	GOTO = 554,
	FLEET_ATTACK = 475,
	FLEET_DEFEND = 476,
	FLEET_ADD = 473,
	FLEET_REMOVE = 474,
	FLEET_RAID = 482,
	STOP = 483,
	PATROL = 484,
	SALVAGE = 485,
	RALLYPOINT = 486,
	REPAIR = 487,
	SUPPLY = 488,
	DIPLOMACY_TT = 489,
	HEADQUARTERS = 490,
	RESEARCH_TT = 491,
	FLEET_TT = 492,
	INDUSTRY_TT = 493,
	HEAVYINDUSTRY = 494,
	TECHINDUSTRY = 495,
	TRANSFER_FO = 496,
	FLEET_INVENTORY = 497,
	VIEW_FLAGSHIP  = 499,
	BACK = 500,
	RESUPPLY_UNIT = 501,
	RESUPPLY_GROUP = 502,
	STANCE_ATTACK = 579,
	STANCE_DEFEND = 580,
	STANCE_STANDGROUND = 581,
	STANCE_IDLE = 582,
	FIGHTER_STANCE_IDLE = 854,
	FIGHTER_STANCE_PATROL = 855,
	ADMIRAL_TACTIC_PEACE = 856,
	ADMIRAL_TACTIC_HOLD = 857,
	ADMIRAL_TACTIC_DEFEND = 858,
	ADMIRAL_TACTIC_SEEK = 859,
	ESCORT_DEFEND = 583,
	NON_GUNBOAT_SELECT = 584,
	ROTATE_0_DEG = 585,
	STARMAP = 586,
	CHAT = 587,
	FLEET_FORM = 588,
	FLEET_DISBAND = 589,
	FLEET_REPAIR = 591,
	FLEET_RESUPPLY = 590,
	TROOPSHIP_ASSAULT = 592,
	TRANSFER_ADMIRAL = 593,
	MISSION_OBJ = 700,
	AUTOSUPP_ON = 703,
	AUTOSUPP_OFF = 704,
	AUTOSUPP_FULL = 705,
	CLOAK = 835,
	TEMPEST_CHARGE = 836,
	PROBE = 837,
	VAMPIRE_ARC = 838,
	AEGIS_SHIELD = 839,
	PING = 840,
	EXPLO_RAM = 841,
	GRAVITY_WELL = 842,
	REPELLENT_CLOUD = 843,
	REPULSOR_BEAM = 844,
	SYNTHESIS = 845,
	MASS_DISRUPTOR = 847,
	AURORA_CLOAKER = 846,
	DESTAB_TORP = 848,
	TRACTOR_BEAM = 849,
	MIMIC = 850,
	EXPLOMINES = 851,
	PULSEMINES = 853,
	HALSEY_F1 = 817,
	HAWKES_F2 = 818,
	TAKEI_F3 = 819,
	STEELE_F4 = 820,
	SMIRNOFF_F5 = 821,
	BENSON_F6 = 822,
	MORDELLA_F1 = 823,
	AZKAR_F2 = 824,
	VERLAK_F3 = 825,
	KERTAK_F4 = 826,
	THRIPID_F5 = 827,
	MALKOR_F6 = 828,
	BLANUS_F1 = 829,
	ELAN_F2 = 830,
	VIVAC_F3 = 831,
	JOULE_F4 = 832,
	PROCYO_F5 = 833,
	NATUS_F6 = 834,
	DAVINCI_SENSORJAM = 6519,
	DAVINCI_WEAPONJAM = 6518,
	DAVINCI_SHIELDJAM = 6517,
	NOVA_BOMB = 6516,
	HARVEST_GAS = 6515,
	HARVEST_ORE = 6514,
	FORM_ATEASE = 6601,
	FORM_STANDASSAULT = 6602,
	FORM_BLOCKADE = 6603,
	FORM_CONVOY = 6604,
	FORM_HALSEYMAN = 6605,
	FORM_LANCER = 6606,
	FORM_PLANETDEF = 6607,
	FORM_MISSILERUSH = 6608,
	FORM_MOBILEDEF = 6609,
	FORM_PLATOFF = 6610,
	FORM_REPAIR = 6611,
	FORM_SCOUT = 6612,
	FORM_SEEKDESTROY = 6613,
	KITA_ANTICEL = 6701,
	KITA_ANTIMANTIS = 6702,
	KITA_ANTITERRAN = 6703,
	KITA_ANTIVYRIUM	= 6704,
	KITT_HALSEY	= 6705,
	KITT_LANCER	= 6706,
	KITT_LOGISTICS = 6707,
	KITT_MISSILERUSH = 6708,
	KITT_MOBILEDEF = 6709,
	KITT_PLATOFF = 6710,
	KITT_REPAIR	= 6711,
	KITT_SCOUT = 6712,
	KITT_SEEKDESTROY = 6713,
	KITT_TACTICS = 6714,
	KITT_ADMTAB	= 6715,

};

enum HOTBUTTONINFO		
{
	NOHOTBUTTONINFO = 1047,
	HB_RALLYPOINT = 1052,
	HB_STOP = 1053,
	HB_SALVAGE = 1054,
	BUTTON_RESEARCH = 3081,
	BUTTON_SHIPYARD = 3082,
	BUTTON_NONGUNBOAT = 3083,
	BUTTON_FLEETS = 3084,
	BUTTON_DIPLOMACY = 3085,
	BUTTON_OPTION = 3087,
	BUTTON_CHAT = 3086,
	BUTTON_SUPPLY = 3088,
	BUTTON_GAS = 3089,
	BUTTON_ORE = 3090,
	BUTTON_CREW = 3091,
	BUTTON_ATTACK = 3092,
	BUTTON_DEFEND = 3093,
	BUTTON_STAND = 3094,
	BUTTON_IDLE = 3095,
	BUTTON_STOP = 3096,
	BUTTON_PATROL = 3097,
	BUTTON_ESCORT = 3098,
	BUTTON_GOTO = 3099,
	BUTTON_SELL = 3100,
	BUTTON_RALLY = 3101,
	BUTTON_FIGHTER_NORMAL = 3333,
	BUTTON_FIGHTER_PATROL = 3334,
	BUTTON_ADMIRAL_PEACE = 3335,
	BUTTON_ADMIRAL_HOLD = 3337,
	BUTTON_ADMIRAL_DEFEND = 3336,
	BUTTON_ADMIRAL_SEEK = 3338,
	MAP_SYSTEM = 3102,
	MAP_SECTOR = 3103,
	MAP_ROTATERIGHT = 3104,
	MAP_ROTATELEFT = 3105,
	MAP_RESTORE = 3106,
	MAP_STARMAP = 3107,
	TOOLBAR_MIN = 3108,
	TOOLBAR_MAX = 3109,
	SUPPLY_ON = 3268,
	SUPPLY_OFF = 3269,
	SUPPLY_AUTO = 3270,
	SPECIALW_CLOAK = 3160,
	SPECIALW_PING = 3272,
	SPECIALW_TEMPEST = 3161,
	SPECIALW_PROBE = 3162,
	SPECIALW_VAMPIRE = 3163,
	SPECIALW_AEGIS = 3164,
	SPECIALW_MIMIC = 3184,
	SPECIALW_GRAVWELL = 3186,
	SPECIALW_REPELLCLOUD = 3185,
	SPECIALW_EXPLORAM = 3273,
	SPECIALW_REPULSOR = 3187,
	SPECIALW_SYNTHESIS = 3243,
	SPECIALW_DISRUPTOR = 3244,
	SPECIALW_AURORACLOAK = 3245,
	SPECIALW_DESTABILIZE = 3246,
	SPECIALW_TRACTORBEAM = 3247,
	SPECIALW_EXPLOMINES = 3274,
	SPECIALW_CPING = 3286,
	SPECIALW_MPING = 3292,
	SPECIALW_SENSORJAM = 6841,
	SPECIALW_SHIELDJAM = 6842,
	SPECIALW_WEAPJAM = 6843
};

enum HOTBUTTONHINT
{
	NO_HOTBUTTONHINT = 0,
	SPECIALW_AD_CLOAK = 3311,
	SPECIALW_AD_PING = 3272,
	SPECIALW_AD_TEMPEST = 3312,
	SPECIALW_AD_PROBE = 3313,
	SPECIALW_AD_VAMPIRE = 3314,
	SPECIALW_AD_AEGIS = 3315,
	SPECIALW_AD_MIMIC = 3316,
	SPECIALW_AD_GRAVWELL = 3318,
	SPECIALW_AD_REPELLCLOUD = 3317,
	SPECIALW_AD_EXPLORAM = 3320,
	SPECIALW_AD_REPULSOR = 3319,
	SPECIALW_AD_SYNTHESIS = 3321,
	SPECIALW_AD_DISRUPTOR = 3322,
	SPECIALW_AD_SHROUD = 3323,
	SPECIALW_AD_DESTABILIZE = 3324,
	SPECIALW_AD_AUGERRAY = 3325,
	SPECIALW_AD_CLOAKS = 3326,
	BUTTON_FORMFLEET = 3327,
	BUTTON_DISBANDFLEET = 3328,
	BUTTON_REPAIRFLEET = 3329,
	BUTTON_RESUPPLYFLEET = 3330,
	BUTTON_TRANSFER = 3331,
	BUTTON_ASSAULT = 3332,
	
};





enum BUILD_TEXT			
{
	NOBUILDTEXT = 0,
	RESEARCH_REQUIRED = 503,
	REFINERY_REQUIRED = 504,
	BALLISTICSLAB_REQUIRED = 595,
	OUTPOST_REQUIRED = 596,
	TENDER_REQUIRED = 597,
	ADVHULL_REQUIRED = 598,
	HVYSHIPYARD_REQUIRED = 603,
	HANGER_REQUIRED = 599,
	ACADEMY_REQUIRED = 600,
	AWSLAB_REQUIRED = 601,
	HQ_REQUIRED = 602,
	DISPLACEMENTLAB_REQUIRED = 605,
	PROPULSION_REQUIRED = 604,
	LIGHTSHIPYARD_REQUIRED = 626,
	COCOON_REQUIRED = 627,
	COLLECTOR_REQUIRED = 628,
	WARLORDTRAINING_REQUIRED = 629,
	THRIPID_REQUIRED = 630,
	NIAD_REQUIRED = 631,
	CARRIONROOST_REQUIRED = 632,
	MUTATIONCOLONY_REQUIRED = 633,
	ACROPOLIS_REQUIRED = 636,
	OXIDATOR_REQUIRED = 637,
	PAVILION_REQUIRED = 638,
	BUNKER_REQUIRED = 639,
	SENTINELTOWER_REQUIRED = 640,
	GREATERPAVILION_REQUIRED = 641,
	CITADEL_REQUIRED = 642,
	XENOCHAMBER_REQUIRED = 643,
	MUNITIONSANNEX_REQUIRED = 644,
	PROTEUS_REQUIRED = 651,
	TURBINEDOCK_REQUIRED = 652,
	TALOREANMATRIX_REQUIRED = 698,
	PLANTATION_REQUIRED = 699,
	GRCOLLECTOR_REQUIRED = 706,
	BLASTFURNACE_REQUIRED = 707,
	EYESTALK_REQUIRED = 708,
	EXPLOSIVE_REQUIRED = 709,
	DISSECT_REQUIRED = 710,
	BIOFORGE_REQUIRED = 711,
	FUSIONMILL_REQUIRED = 712,
	CARAPACEPLANT_REQUIRED = 713,
	HYBRIDCENTER_REQUIRED = 714,
	AWSPROP_REQUIRED = 716,
	ACADEMYDISPPROP_REQUIRED = 717,
	SUPPLY_REQUIRED = 718,
	SENSORTOWER_REQUIRED = 719,
	COLLECT_PLANT_REQUIRED = 720,
	BIO_CARR_DIS_REQUIRED = 721,
	EUTRO_OXI_REQUIRED = 722,
	GRPAV_HEL_REQUIRED = 723,
	MUN_TURB_REQUIRED = 724,
	MUN_TURB_XENO_REQUIRED = 725,
	GRPAV_PROT_REQUIRED = 726,
	REPAIR_REQUIRED = 756,
	LASER_REQUIRED = 757,
	TERRANJUMP_REQUIRED = 758,
	FAB_REQUIRED = 761,
	SUPSHIP_REQUIRED = 762,
	HARVESTER_REQUIRED = 763,
	CORVETTE_REQUIRED = 764,
	MISSILECRUISER_REQUIRED = 765,
	INFILTRATOR_REQUIRED = 766,
	TROOPSHIP_REQUIRED = 767,
	BATTLESHIP_REQUIRED = 768,
	FLEETCARRIER_REQUIRED = 769,
	LANCERCRUISER_REQUIRED = 770,
	DREADNOUGHT_REQUIRED = 771,
	MANTISJUMP_REQUIRED = 772,
	PLASMASPITTER_REQUIRED = 773,
	PLASMAHIVE_REQUIRED = 774,
	VORAAKCANNON_REQUIRED = 775,
	WEAVER_REQUIRED = 776,
	ZORAP_REQUIRED = 777,
	SIPHON_REQUIRED = 778,
	SCOUTCARRIER_REQUIRED = 779,
	FRIGATE_REQUIRED = 780,
	KHAMIR_REQUIRED = 781,
	HIVECARRIER_REQUIRED = 782,
	SEEKER_REQUIRED = 783,
	SCARAB_REQUIRED = 784,
	LEECH_REQUIRED = 785,
	TIAMAT_REQUIRED = 786,
	SPINELAYER_REQUIRED = 787,
	EUTROMILL_REQUIRED = 788,
	CELJUMPGATE_REQUIRED = 789,
	ANVIL_REQUIRED = 790,
	HELIONVEIL_REQUIRED = 791,
	ESPCOIL_REQUIRED = 792,
	HYDROFOIL_REQUIRED = 793,
	STARBURST_REQUIRED = 794,
	PORTAL_REQUIRED = 795,
	FORGER_REQUIRED = 796,
	STRATUM_REQUIRED = 797,
	GALIOT_REQUIRED = 798,
	TAOS_REQUIRED = 799,
	POLARIS_REQUIRED = 800,
	ORACLE_REQUIRED = 801,
	LEGIONNAIRE_REQUIRED = 802,
	AURORA_REQUIRED = 803,
	ATLAS_REQUIRED = 804,
	TRIREME_REQUIRED = 805,
	MONOLITH_REQUIRED = 806,
	ION_REQUIRED	= 760,
	SPACE_REQUIRED	= 759,
	REPAIRSHIP_REQUIRED = 6501,
	MP_RESOURCECTR_REQUIRED = 6503,
	MP_RESEARCHLAB_REQUIRED = 6504,
	MP_INDUSTFACTORY_REQUIRED = 6505,
	MP_INTELLCTR_REQUIRED = 6506,
	MP_NOVABOMB_REQUIRED = 6507,
	MP_HEALINGCLOUD_REQUIRED = 6508,
	MP_BEEHIVE_REQUIRED = 6509,
	MP_CLOAKING_REQUIRED = 6510,
	MP_TALMATRIX_REQUIRED = 6511,
	MP_ASTEROID_REQUIRED = 6512,
	MP_SPACEFOLDER_REQUIRED = 6513,

};

enum BUILDINFO		
{
	NOBUILDINFO = 3001,
	BUILD_CORVETTE = 3005,
	BUILD_MISSILECRUISER = 3006,
	BUILD_LSAT = 3007,
	BUILD_TANKER = 3008,
	BUILD_TROOPSHIP = 3009,
	BUILD_FLEETCARRIER = 3014,
	BUILD_BATTLESHIP = 3015,
	BUILD_SPACESTATION = 3016,
	BUILD_DREADNOUGHT = 3017,
	BUILD_INFILTRATOR = 3018,
	BUILD_LANCERCRUISER = 3019,
	BUILD_SUPPLYSHIP = 3020,
	BUILD_FABRICATOR = 3021,
	BUILD_ADVFABRICATOR = 3022,
	BUILD_FLEETADMIRAL = 3023,
	BUILD_SUPPLYDEPOT = 3024,
	BUILD_OUTPOST = 3025,
	BUILD_LASERTURRET = 3026,
	BUILD_NAVALACADEMY = 3027,
	BUILD_MRSENSOR = 3028,
	BUILD_ADVHULL = 3029,
	BUILD_PROBLAB = 3030,
	BUILD_REPAIRDEPOT = 3031,
	BUILD_HEAVYIND = 3032,
	BUILD_AWSLAB = 3033,
	BUILD_DEFENSEBATTERY = 3034,
	BUILD_LRSENSOR = 3035,
	BUILD_LOGISTICS = 3036,
	BUILD_NANOMETAL = 3037,
	BUILD_TECHIND = 3038,
	BUILD_FORCEWAVE = 3039,
	BUILD_REFINERY = 3010,
	BUILD_LIGHTIND = 3011,
	BUILD_HQ = 3041,
	BUILD_BALLISTICSLAB = 3042,
	BUILD_TENDER = 3043,
	BUILD_HANGER = 3044,
	BUILD_SENSORTOWER = 3045,
	BUILD_DISPLACEMENTLAB = 3046,
	BUILD_IONCANNON = 3047,
	BUILD_HVYREFINERY = 3048,
	BUILD_COCOON = 3049,
	BUILD_COLLECTOR = 3050,
	BUILD_GRCOLLECTOR = 3051,
	BUILD_JUMPGATE = 3052,
	BUILD_WARLORD = 3053,
	BUILD_THRIPID = 3054,
	BUILD_NIAD = 3055,
	BUILD_PLANTATION = 3056,
	BUILD_DISSECTION = 3057,
	BUILD_BLASTFURNACE = 3058,
	BUILD_EXPLORANGE = 3059,
	BUILD_EYESTALK = 3060,
	BUILD_MUTATION = 3061,
	BUILD_BIOFORGE = 3062,
	BUILD_FUSIONMILL = 3063,
	BUILD_CARAPACE = 3064,
	BUILD_HYBRID = 3065,
	BUILD_CARRIONROOST = 3066,
	BUILD_PLASMASPITTER = 3067,
	BUILD_PLASMAHIVE = 3068,
	BUILD_VORAAK = 3069,
	BUILD_WEEVER = 3070,
	BUILD_ZORAP = 3071,
	BUILD_SIPHON = 3072,
	BUILD_FRIGATE = 3073,
	BUILD_SCOUTCARRIER = 3074,
	BUILD_LEECH = 3077,
	BUILD_KHAMIR = 3075,
	BUILD_SPINELAYER = 3076,
	BUILD_HIVECARRIER = 3078,
	BUILD_SCARAB = 3079,
	BUILD_TIAMAT = 3080,
	BUILD_SEEKER = 3176,
	BUILD_ACROPOLIS = 3213,
	BUILD_OXIDATOR = 3214,
	BUILD_SENTINEL = 3215,
	BUILD_PAVILION = 3216,
	BUILD_BUNKER = 3217,
	BUILD_HELION = 3218,
	BUILD_ESPCOIL = 3219,
	BUILD_CITADEL = 3220,
	BUILD_EUTROMIL = 3221,
	BUILD_GRPAVILION = 3222,
	BUILD_PROTEUS = 3223,
	BUILD_XENOCHAMBER = 3224,
	BUILD_MUNITIONS = 3225,
	BUILD_ANVIL = 3226,
	BUILD_TURBINE = 3227,
	BUILD_HYDROFOIL = 3228,
	BUILD_MATRIX = 3229,
	BUILD_STARBURST = 3230,
	BUILD_PORTAL = 3231,
	BUILD_FORGER = 3233,
	BUILD_STRATUM = 3234,
	BUILD_GALIOT = 3235,
	BUILD_TAOS = 3232,
	BUILD_ORACLE = 3236,
	BUILD_AURORA = 3237,
	BUILD_LEGIONAIRE = 3238,
	BUILD_POLARIS = 3239,
	BUILD_ATLAS = 3240,
	BUILD_TRIREME = 3241,
	BUILD_MONOLITH = 3242,
	BUILD_REPAIRSHIP = 6806,
	BUILD_ESPIONAGE = 6807,
	BUILD_MP_INDUSTRIAL = 6801,
	BUILD_MP_RESEARCHLAB = 6802,
	BUILD_MP_RESOURCE = 6803,
	BUILD_MP_INTELLCTR = 6804,
	BUILD_MP_NOVABOMB = 6805,

};





enum RESEARCH_TEXT		
{
	NORESEARCHTEXT = 0,
	RES_MC				= 505,
	RES_LSAT			= 506,
	RES_MCCLOAK			= 507,
	RES_TROOPSHIP1		= 508,
	RES_TROOPSHIP2		= 727,
	RES_TROOPSHIP3		= 728,
	RES_ENGINE1			= 509,
	RES_ENGINE2			= 510,
	RES_ENGINE3			= 511,
	RES_ENGINE4			= 512,
	RES_ENGINE5			= 677,
	RES_BATTLESHIP		= 513,
	RES_BATTLECHARGES	= 514,
	RES_DREADNOUGHT		= 515,
	RES_DREADSHIELD		= 516,
	RES_FCPROBE			= 517,
	RES_SPACESTATION	= 518,
	RES_SUPPLYSHIP		= 519,
	RES_HULL1			= 520,
	RES_HULL2			= 521,
	RES_HULL3			= 522,
	RES_HULL4			= 523,
	RES_HULL5			= 682,
	RES_WEAPON1			= 524,
	RES_WEAPON2			= 525,
	RES_WEAPON3			= 526,
	RES_WEAPON4			= 527,
	RES_WEAPON5			= 681,
	RES_LANCER			= 528,
	RES_LANCERARC		= 529,
	RES_SUPPLY1			= 530,
	RES_SUPPLY2			= 531,
	RES_SUPPLY3			= 532,
	RES_SUPPLY4			= 533,
	RES_SUPPLY5			= 679,
	RES_SHIELD1			= 534,
	RES_SHIELD2			= 535,
	RES_SHIELD3			= 536,
	RES_SHIELD4			= 537,
	RES_SHIELD5			= 680,
	RES_HARVESTER1		= 561,
	RES_HARVESTER2		= 562,
	RES_REPAIR			= 563,
	RES_MISSILE1		= 564,
	RES_MISSILE2		= 565,
	RES_FLEET1			= 566,
	RES_FLEET2			= 567,
	RES_FLEET3			= 568,
	RES_FLEET4			= 569,
	RES_FLEET5			= 678,
	RES_SENSOR1			= 570,
	RES_SENSOR2			= 571,
	RES_SENSOR3			= 572,
	RES_SENSOR4			= 573,
	RES_SENSOR5			= 578,
	RES_FIGHTER1		= 574,
	RES_FIGHTER2		= 575,
	RES_FIGHTER3		= 576,
	RES_FIGHTER4		= 577,
	RES_FIGHTER5		= 609,
	RES_TENDER1			= 542,
	RES_TENDER2			= 608,
	RES_EXPLORAM1		= 610,
	RES_EXPLORAM2		= 611,
	RES_CAMO			= 612,
	RES_REPELLCLOUD		= 613,
	RES_GRAVWELL		= 614,
	RES_BOMBER			= 615,
	RES_MUTATIONCOLONY	= 616,
	RES_WARLORDTRAINING	= 617,
	RES_EXPLOSIVESRANGE	= 618,
	RES_NIAD			= 619,
	RES_FUSIONMILL		= 620,
	RES_CARAPACE		= 621,
	RES_DISSECTION		= 625,
	RES_HYBRID			= 622,
	RES_VORAAK			= 624,
	RES_PLASMAHIVE		= 623,
	RES_HEAVYREFINERY	= 634,
	RES_SUPERHVYREFINERY= 635,
	RES_SYNTHESIS		= 645,
	RES_COLLECTIVE		= 646,
	RES_MASSDISRUPTOR	= 647,
	RES_ECLIPSETORPEDO	= 648,
	RES_DESTABILIZER	= 649,
	RES_REPULSERBEAM	= 650,
	RES_SIPHON1			= 658,
	RES_SIPHON2			= 659,
	RES_SIPHON3			= 663,
	RES_PLANTATION1		= 660,
	RES_PLANTATION2		= 661,
	RES_PLANTATION3		= 662,
	RES_ORE1			= 664,
	RES_ORE2			= 665,
	RES_ORE3			= 666,
	RES_GAS1			= 667,
	RES_GAS2			= 668,
	RES_GAS3			= 669,
	RES_GALIOT1			= 683,
	RES_GALIOT2			= 684,
	RES_EUTROMIL1		= 685,
	RES_EUTROMIL2		= 686,
	RES_LEGIONAIRE1		= 694,
	RES_LEGIONAIRE2		= 695,
	RES_LEGIONAIRE3		= 696,
	RES_LEGIONAIRE4		= 697,
	RES_AURORACLOAK		= 701,
	RES_TRACTOR			= 702,
	RES_T_HALSEY		= 396,
	RES_T_HAWKES		= 397,
	RES_T_BENSON		= 398,
	RES_T_SMIRNOFF		= 399,
	RES_T_STEELE		= 401,
	RES_T_TAKAI			= 670,
	RES_M_MORDELLA		= 671,
	RES_M_AZKAR			= 672,
	RES_M_VERLAK		= 673,
	RES_M_KERTAK		= 674,
	RES_M_THRIPID		= 675,
	RES_M_MALKOR		= 676,
	RES_S_BLANUS		= 687,
	RES_S_ELAN			= 688,
	RES_S_VIVAC			= 689,
	RES_S_JOULE			= 690,
	RES_S_PROCY			= 691,
	RES_S_NATUS			= 692,
	RES_GRCOLLECTOR		= 715,
	RES_LEECH1			= 729,
	RES_LEECH2			= 730,
	RES_CMISSILE1		= 814,
	RES_CMISSILE2		= 815,
	
};

enum RESEARCHINFO	
{
	NORESEARCHINFO = 3002,
	UPGRADE_TENDER1 = 3110,
	UPGRADE_TENDER2 = 3111,
	UPGRADE_SUPPLY1 = 3112,
	UPGRADE_SUPPLY2 = 3113,
	UPGRADE_SUPPLY3 = 3171,
	UPGRADE_SUPPLY4 = 3172,
	UPGRADE_SUPPLY5 = 3173,
	UPGRADE_SENSOR1 = 3115,
	UPGRADE_SENSOR2 = 3116,
	UPGRADE_SENSOR3 = 3119,
	UPGRADE_SENSOR4 = 3120,
	UPGRADE_SENSOR5 = 3208,
	UPGRADE_WEAPON1 = 3130,
	UPGRADE_WEAPON2 = 3131,
	UPGRADE_WEAPON3 = 3132,
	UPGRADE_WEAPON4 = 3133,
	UPGRADE_WEAPON5 = 3201,
	UPGRADE_HARVESTER1 = 3134,
	UPGRADE_HARVESTER2 = 3135,
	UPGRADE_TROOPSHIP1 = 3136,
	UPGRADE_TROOPSHIP2 = 3137,
	UPGRADE_TROOPSHIP3 = 3138,
	UPGRADE_FLEET1 = 3139,
	UPGRADE_FLEET2 = 3140,
	UPGRADE_FLEET3 = 3141,
	UPGRADE_FLEET4 = 3142,
	UPGRADE_FLEET5 = 3203,
	UPGRADE_MISSILE1 = 3143,
	UPGRADE_MISSILE2 = 3144,
	UPGRADE_HULL1 = 3145,
	UPGRADE_HULL2 = 3146,
	UPGRADE_HULL3 = 3147,
	UPGRADE_HULL4 = 3148,
	UPGRADE_HULL5 = 3200,
	UPGRADE_SHIELD1 = 3149,
	UPGRADE_SHIELD2 = 3150,
	UPGRADE_SHIELD3 = 3151,
	UPGRADE_SHIELD4 = 3152,
	UPGRADE_SHIELD5 = 3202,
	UPGRADE_ENGINE1 = 3153,
	UPGRADE_ENGINE2 = 3154,
	UPGRADE_ENGINE3 = 3155,
	UPGRADE_ENGINE4 = 3156,
	UPGRADE_ENGINE5 = 3199,
	UPGRADE_FIGHTER1 = 3157,
	UPGRADE_FIGHTER2 = 3158,
	UPGRADE_FIGHTER3 = 3159,
	UPGRADE_FIGHTER4 = 3169,
	UPGRADE_FIGHTER5 = 3170,
	SPECIAL_CLOAK =	3160,
	SPECIAL_TEMPEST = 3161,
	SPECIAL_PROBE = 3162,
	SPECIAL_VAMPIRE = 3163,
	SPECIAL_AEGIS = 3164,
	UPGRADE_ORE1 = 3165,
	UPGRADE_ORE2 = 3166,
	UPGRADE_ORE3 = 3174,
	UPGRADE_GAS1 = 3167,
	UPGRADE_GAS2 = 3168,
	UPGRADE_GAS3 = 3175,
	UPGRADE_SIPHON1 = 3177,
	UPGRADE_SIPHON2 = 3178,
	UPGRADE_SIPHON3 = 3188,
	UPGRADE_PLANTATION1 = 3179,
	UPGRADE_PLANTATION2 = 3180,
	UPGRADE_PLANTATION3 = 3181,
	UPGRADE_EXPLORAM1 = 3182,
	UPGRADE_EXPLORAM2 = 3183,
	SPECIAL_MIMIC = 3184,
	SPECIAL_REPELLCLOUD = 3185,
	SPECIAL_GRAVWELL = 3186,
	SPECIAL_REPULSEBEAM = 3187,
	EVOLUTION_GRCOLLECTOR = 3189,
	EVOLUTION_EXPLORANGE = 3191,
	EVOLUTION_MUTACOLONY = 3192,
	EVOLUTION_FUSIONMILL = 3193,
	EVOLUTION_CARAPACE = 3194,
	EVOLUTION_HYBRID = 3195,
	EVOLUTION_PLASMAHIVE = 3196,
	EVOLUTION_VORAAK = 3197,
	EVOLUTION_NIAD = 3190,
	UPGRADE_REPAIR = 3198,
	UPGRADE_GALIOT1 = 3204,
	UPGRADE_GALIOT2 = 3205,
	UPGRADE_EUTROMIL1 = 3206,
	UPGRADE_EUTROMIL2 = 3207,
	UPGRADE_LEGIONAIRE1 = 3209,
	UPGRADE_LEGIONAIRE2 = 3210,
	UPGRADE_LEGIONAIRE3 = 3211,
	UPGRADE_LEGIONAIRE4 = 3212,
	SPECIAL_SYNTHESIS = 3243,
	SPECIAL_DISRUPTOR = 3244,
	SPECIAL_AURORACLOAK = 3245,
	SPECIAL_DESTABILIZE = 3246,
	SPECIAL_TRACTORBEAM = 3247,
	ADMIRAL_HALSEY = 3248,
	ADMIRAL_HAWKES = 3249,
	ADMIRAL_BENSON = 3250,
	ADMIRAL_STEELE = 3252,
	ADMIRAL_SMIRNOFF = 3251,
	ADMIRAL_TAKAI = 3253,
	ADMIRAL_MORDELLA = 3254,
	ADMIRAL_AZKAR = 3255,
	ADMIRAL_VERLAK = 3256,
	ADMIRAL_KERTAK = 3257,
	ADMIRAL_THRIPID = 3258,
	ADMIRAL_MALKOR = 3259,
	ADMIRAL_BLANUS = 3260,
	ADMIRAL_ELAN = 3261,
	ADMIRAL_VIVAC = 3262,
	ADMIRAL_JOULE = 3263,
	ADMIRAL_PROCYO = 3264,
	ADMIRAL_NATUS = 3265,
	UPGRADE_LEECH1 = 3266,
	UPGRADE_LEECH2 = 3267,
	UPGRADE_CWEAPON1 = 3277,
	UPGRADE_CWEAPON2 = 3278,
	UPGRADE_CWEAPON3 = 3279,
	UPGRADE_CWEAPON4 = 3280,
	UPGRADE_CWEAPON5 = 3281,
	UPGRADE_CSUPPLY1 = 3287,
	UPGRADE_CSUPPLY2 = 3288,
	UPGRADE_CSUPPLY3 = 3289,
	UPGRADE_CSUPPLY4 = 3290,
	UPGRADE_CSUPPLY5 = 3291,
	UPGRADE_CORE1 = 3282,
	UPGRADE_CORE2 = 3283,
	UPGRADE_CORE3 = 3304,
	UPGRADE_CGAS1 = 3284,
	UPGRADE_CGAS2 = 3285,
	UPGRADE_CGAS3 = 3305,
	UPGRADE_CMISSILE1 = 3275,
	UPGRADE_CMISSILE2 = 3276,
	UPGRADE_MWEAPON1 = 3293,
	UPGRADE_MWEAPON2 = 3294,
	UPGRADE_MWEAPON3 = 3295,
	UPGRADE_MFIGHTER1 = 3296,
	UPGRADE_MFIGHTER2 = 3297,
	UPGRADE_MFIGHTER3 = 3298,
	UPGRADE_MFIGHTER4 = 3299,
	UPGRADE_MFIGHTER5 = 3300,
	UPGRADE_MSUPPLY1 = 3301,
	UPGRADE_MSUPPLY2 = 3302,
	UPGRADE_MSUPPLY3 = 3303,
	
};

enum MULTIBUTTONINFO  
{
	NO_MULTI_INFO = 0,
	TEST_MULTI_INFO = 3303,
	STATIC_HALSEY = 3248,
	STATIC_HAWKES = 3249,
	STATIC_BENSON = 3250,
	STATIC_SMIRNOFF = 3251,
	STATIC_STEELE = 3252,
	STATIC_TAKAI = 3253,
	STATIC_MORDELLA = 3254,
	STATIC_AZKAR = 3255,
	STATIC_VERLAK = 3256,
	STATIC_KERTAK = 3257,
	STATIC_THRIPID = 3258,
	STATIC_MALKOR = 3259,
	STATIC_BLANUS = 3260,
	STATIC_ELAN = 3261,
	STATIC_VIVAC = 3262,
	STATIC_JOULE = 3263,
	STATIC_PROCYO = 3264,
	STATIC_NATUS = 3265,
	KIT_HALSEY = 6808,
	KIT_LANCER = 6809,
	KIT_LOGISTICS = 6810,
	KIT_MISSILERUSH = 6811,
	KIT_MOBILEDEF = 6812,
	KIT_PLATOFF = 6813,
	KIT_REPAIR = 6814,
	KIT_SCOUT = 6815,
	KIT_SEEKDESTROY = 6816,
	KIT_TACTICS = 6817,
	KIT_ANTICEL = 6818,
	KIT_ANTIMANTIS = 6819,
	KIT_ANTITERRAN = 6820,
	KIT_ANTIVYRIUM = 6821,
	FORM_T_ATEASE = 6822,
	FORM_T_ASSAULT = 6823,
	FORM_T_BLOCKADE = 6824,
	FORM_T_CONVOY = 6825,
	FORM_T_HALSEY = 6826,
	FORM_T_LANCER = 6827,
	FORM_T_PLANETDEF = 6828,
	FORM_T_MISSILERUSH = 6829,
	FORM_T_MOBILEDEF = 6830,
	FORM_T_PLATOFF = 6831,
	FORM_T_REPAIR = 6832,
	FORM_T_SCOUT = 6833,
	FORM_T_SEEKDESTROY = 6834,

};

} 























struct GT_HOTBUTTON : GENBASE_DATA
{
	HOTBUTTONTYPE::TYPE buttonType;
	char fontType[32];
	struct _textColor
	{
		U8 red,green,blue;
	}textColor;
};


struct HOTBUTTON_DATA
{
	U32 baseImage;							
	S32 xOrigin, yOrigin;
	HBTNTXT::BUTTON_TEXT buttonText;		
	HBTNTXT::HOTBUTTONINFO buttonInfo;		
	HBTNTXT::HOTBUTTONHINT buttonHint;		
	HOTKEYS::HOTKEY hotkey;
	bool bDisabled;							
};


struct BUILDBUTTON_DATA
{
	U32 baseImage;							
	U32 noMoneyImage;
	S32 xOrigin, yOrigin;
	char rtArchetype[32];
	SINGLE_TECHNODE techDependency, techGreyed;
	HBTNTXT::BUILD_TEXT greyedTooltip;	
	HBTNTXT::BUILDINFO buildInfo;			
	HOTKEYS::HOTKEY hotkey;					
	bool bDisabled;							
};


struct RESEARCHBUTTON_DATA
{
	U32 baseImage;							
	U32 noMoneyImage;
	S32 xOrigin, yOrigin;
	char rtArchetype[32];
	HBTNTXT::RESEARCH_TEXT tooltip;	
	HBTNTXT::RESEARCHINFO researchInfo;		
	HOTKEYS::HOTKEY hotkey;					
	bool bDisabled;							
};


struct MULTIHOTBUTTON_DATA
{
	S32 xOrigin, yOrigin;
	HOTKEYS::HOTKEY hotkey;
	bool bSingleShape;
	bool bDisabled;							
};














namespace TABTXT
{
enum TAB_TEXT
{
	NO_TAB_TEXT=0,
	PLAYER = 1901,
	GRAPHICS = 1902,
	SOUNDS = 1903,
	TOTALS = 1904,
	UNITS = 1905,
	BUILDINGS = 1906,
	TERRITORY = 1907,
	RESOURCES = 1908
};
} 




struct GT_TABCONTROL : GENBASE_DATA
{
	struct COLOR
	{
		unsigned char red, green, blue;
	} normalColor, hiliteColor, selectedColor;	
};


struct TABCONTROL_DATA
{
    char tabControlType[32];
	char hotButtonType[32];
	int  iBaseImage;
	int  numTabs;
	TABTXT::TAB_TEXT textID[6];
	bool bUpperTabs;
	int xpos;
	int ypos;
};


















			    























			    

























struct GT_LISTBOX : GENBASE_DATA
{
	char fontName[32];
	struct COLOR
	{
		unsigned char red, green, blue;
	} disabledText, normalText, highlightText, selectedText, selectedTextGrayed;

	char shapeFile[32];			
	char scrollBarType[32];
};


struct LISTBOX_DATA
{
    char listboxType[32];
    S32 xOrigin, yOrigin;			
	RECT textArea;					
	U32  leadingHeight;				
	bool bStatic:1;					
	bool bSingleClick:1;			
	bool bScrollbar:1;
	bool bSolidBackground:1;		
	bool bNoBorder:1;
	bool bDisableMouseSelect:1;
};


















			    










namespace EDTXT
{
enum EDIT_TEXT
{
	NONE=0,
    ETXT_TEST = 1101,
	IP_ADDRESS_DEFAULT = 1110
};
} 























struct GT_EDIT : GENBASE_DATA
{
	char fontName[32];
	struct COLOR
	{
		unsigned char red, green, blue;
	} disabledText, normalText, highlightText, selectedText, caret;

	char shapeFile[32];			
	S32 justify;
	S32 width;
	S32 height;
};


struct EDIT_DATA
{
	char editType[32];
	EDTXT::EDIT_TEXT editText;         
	S32 xOrigin, yOrigin;
};





 













			    



















struct GT_DROPDOWN : GENBASE_DATA
{
	
};



struct DROPDOWN_DATA
{
    char dropdownType[32];
	RECT screenRect;
	BUTTON_DATA buttonData;
	LISTBOX_DATA listboxData;
};


















			    
























struct GT_COMBOBOX : GENBASE_DATA
{
	
};



struct COMBOBOX_DATA
{
    char comboboxType[32];
	RECT screenRect;
	EDIT_DATA editData;
	BUTTON_DATA buttonData;
	LISTBOX_DATA listboxData;
};






















			    




















struct GT_SLIDER : GENBASE_DATA
{
	struct COLOR
	{
		unsigned char red, green, blue;
	} disabledColor, normalColor, highlightColor, alertColor;
	bool bVertical;
	U32  indent;
	char shapeFile[32];
};



struct SLIDER_DATA
{
    char sliderType[32];
	RECT screenRect;
	S32 xOrigin, yOrigin;
};













struct GT_MENU1
{
	struct OPENING
	{
		RECT screenRect;
		STATIC_DATA background;
		BUTTON_DATA single, multi, intro, options, help, quit;
		STATIC_DATA staticSingle, staticMulti, staticIntro, staticOptions, staticHelp;
		ANIMATE_DATA animMedia, animSingle, animMulti, animOptions, animQuestion;
		STATIC_DATA staticLegal;
	} opening;

	struct SINGLEPLAYER_MENU
	{
		RECT screenRect;
		STATIC_DATA background, staticSingle, staticName;
		BUTTON_DATA buttonCampaign, buttonSkirmish, buttonLoad, buttonQBLoad;
		BUTTON_DATA buttonBack;
	} singlePlayerMenu;

	struct SELECT_CAMPAIGN
	{
		RECT screenRect;
		STATIC_DATA background, title;
		STATIC_DATA staticName;
		BUTTON_DATA buttonTerran, buttonMantis, buttonSolarian;
		BUTTON_DATA buttonBack;
	} selectCampaign;

	struct SELECT_MISSION
	{
		RECT screenRect;
		STATIC_DATA background, title;
		LISTBOX_DATA list;
		BUTTON_DATA start, ok, cancel;

		BUTTON_DATA buttonUnlock;

		STATIC_DATA staticMission, staticHolder;
		BUTTON_DATA buttonMissions[16];
		BUTTON_DATA buttonMovies[6];
		BUTTON_DATA buttonBack;
		STATIC_DATA staticMovie, staticMissionTitle;
		ANIMATE_DATA animSystem;
		int nLineFrom[16];
		int nMoiveBeforeMission[6];

	} selectMission;

	struct NET_CONNECTIONS
	{
		RECT screenRect;
		STATIC_DATA background, description;
		LISTBOX_DATA list;
		BUTTON_DATA next, back;
		STATIC_DATA staticTitle;
		BUTTON_DATA buttonZone;
		BUTTON_DATA buttonWeb;
	} netConnections;
	
	struct IP_ADDRESS
	{
		RECT screenRect;
		STATIC_DATA background, description;
		STATIC_DATA enterIP, enterName;
		STATIC_DATA staticName;
		STATIC_DATA staticJoin, staticCreate;
		BUTTON_DATA checkJoin, checkCreate;
		BUTTON_DATA next, back;
		COMBOBOX_DATA comboboxIP;
	} ipAddress;

	struct GAME_ZONE
	{
		RECT screenRect;
		STATIC_DATA description;
	} gameZone;

	struct NET_SESSIONS2
	{ 
		RECT screenRect;
		STATIC_DATA background, description, version;
		STATIC_DATA title;
		LISTBOX_DATA list;
		BUTTON_DATA next, back;

		STATIC_DATA staticGame, staticPlayers, staticSpeed, staticMap, staticResources;
	} netSessions2;

	struct MSHELL
	{
		RECT screenRect;
		STATIC_DATA background;
		STATIC_DATA title;
		STATIC_DATA enterChat;
		STATIC_DATA ipaddress;
		LISTBOX_DATA listChat;
		EDIT_DATA editChat;
	} mshell;



	struct MAP
	{
		RECT screenRect;

		BUTTON_DATA mapType;
		STATIC_DATA staticGameType, staticSpeed, staticMoney, staticUnits;
		STATIC_DATA staticMapType, staticSize, staticTerrain;
		STATIC_DATA staticVisibility, staticBandwidth;
		DROPDOWN_DATA dropGameType, dropSpeed, dropMoney;
		DROPDOWN_DATA dropSize, dropTerrain, dropUnits;
		DROPDOWN_DATA dropVisibility, bandWidthOption;

		SLIDER_DATA sliderSpeed;
		SLIDER_DATA sliderCmdPoints;

		STATIC_DATA staticSpectator, staticDiplomacy;
		BUTTON_DATA pushSpectator, pushDiplomacy;

		STATIC_DATA staticDifficulty, staticEasy, staticAverage, staticHard;
		BUTTON_DATA pushEasy, pushAverage, pushHard;

		STATIC_DATA staticLockSettings;
		BUTTON_DATA pushLockSettings;

		STATIC_DATA staticSystems;
		DROPDOWN_DATA dropSystems;

		STATIC_DATA staticCmdPoints;
		STATIC_DATA staticCmdPointsDisplay;
	} map;

	struct SLOTS
	{
		RECT screenRect;
		DROPDOWN_DATA dropSlots[8];
		DROPDOWN_DATA dropTeams[8];
		DROPDOWN_DATA dropRaces[8];
		DROPDOWN_DATA dropPlayers[8];
		STATIC_DATA staticNames[8];
		STATIC_DATA staticPings[8];
		M_STRINGW terranComputerNames[8];
		M_STRINGW mantisComputerNames[8];
		M_STRINGW solarianComputerNames[8];
	} slots;

	struct FINAL
	{
		RECT screenRect;
		STATIC_DATA staticState, staticName, staticColor, staticRace, staticTeam, staticPing;
		STATIC_DATA description;
		STATIC_DATA staticAccept; 
		BUTTON_DATA accept, start, cancel;
		STATIC_DATA staticCountdown;
	} final;

	struct HELPMENU
	{
		RECT screenRect;
		STATIC_DATA background, title;
		STATIC_DATA staticConquest, staticVersion, staticNumber;
		BUTTON_DATA buttonOk;
		STATIC_DATA staticProductID, staticProductNumber;
		STATIC_DATA staticLegal;
		BUTTON_DATA	buttonCredits;
	} helpMenu;

	struct DEVICEMENU
	{
		RECT screenRect;
		STATIC_DATA staticBackground, staticTitle;
		STATIC_DATA staticPickDevice;
		LISTBOX_DATA listDevices;
		BUTTON_DATA buttonOk;
	} deviceMenu;
};

struct GT_MAPSELECT
{
	RECT screenRect;
	STATIC_DATA staticBackground, staticTitle;
	STATIC_DATA staticRandom, staticSupplied, staticSaved;
	LISTBOX_DATA listRandom, listSupplied, listSaved;
	BUTTON_DATA buttonOk, buttonCancel;
};


















			    














struct GT_NEWPLAYER
{
	RECT screenRect;
	STATIC_DATA background, title, staticHeading;
	EDIT_DATA edit;
	BUTTON_DATA ok, cancel;
};




























			    











struct BASE_ANIMNUGGET_SAVELOAD
{
	U16 supplyPointsMax;
	U16 supplies;				
	U8 shadowVisibilityFlags;	
	U8 harvestCount;
	bool bDepleted:1;
	bool deleteOK:1;
	bool bUnregistered:1;
	bool bRealized:1;
	U8 systemID;
	U16 shadowSupplies[8];
	U32 processID;
	U32 deathOp;

	SINGLE lifeTimeRemaining;
	U32 dwMissionID;
	Vector position;
};



struct BASE_NUGGET_SAVELOAD
{
	U32 processID;
	U8 harvestCount;

	U32 lifeTimeTicks;
};


struct NUGGET_SAVELOAD : BASE_NUGGET_SAVELOAD
{
	
	TRANS_SAVELOAD trans_SL;

	
	MISSION_SAVELOAD mission;
};


enum M_NUGGET_TYPE
{
	M_GAS_NUGGETS,
	M_METAL_NUGGET,
	M_SCRAP_NUGGET,
	M_HYADES_NUGGET
};

struct BT_NUGGET_DATA : BASIC_DATA
{
	char nugget_anim2D[32];
	char nugget_mesh[32];
	S32 animSizeSmallMax;
	S32 animSizeMax;
	S32 animSizeMin;
	struct _color
	{
		U8 redHi;
		U8 greenHi;
		U8 blueHi;
		U8 redLo;
		U8 greenLo;
		U8 blueLo;
		U8 alphaIn;
		U8 alphaOut;
	}color;

	bool bOriented;
	bool zRender;

	M_RESOURCE_TYPE resType;

	M_NUGGET_TYPE nuggetType;

	U32 maxSupplies;
};



















			    























			    












namespace PRG_STTXT
{
enum PROGRESS_STATIC_TEXT
{

  NO_STATIC_TEXT=0,
  TEST = 1101,
  PLAYER_NAME = 1401,
  IP_ADDRESS = 1402,
  SESSION_NAME = 1404,
  CONFIRM_QUIT = 1406,
  TITLE_SELECT_GAME = 1407,
  PLAYER = 1408,
  SESSION = 1409,
  ENTER_TEAM = 1412,
  ENTER_RACE = 1410,
  ENTER_COLOR = 1411,
  SELECTED_MAP = 1414,
  CHATBOX = 1415,
  GAME_PAUSED = 1416,
  INGAME_TITLE = 1417,
  OPTIONS = 1418,
  GAMETYPE = 1419,
  GAMESPEED = 1420,
  CURSOR_SENSITIVITY = 1463,
  ARTIFACTS = 1421,
  MONEY = 1422,
  RESEARCH = 1423,
  MAPTYPE = 1424, 
  MAPFILE = 1451,
  MAPSIZE = 1425,
  TERRAIN = 1426,
  STATIC_RESOURCES = 1427,
  WARNBOOT = 1329,
  SOLOGAME = 1332,
  LOADGAME = 1431,
  SAVEGAME = 1432,
  FILENAME = 1433,
  FLEET_SIZE = 1434,
  SKILL = 1435,
  BONUS = 1436,
  DUMMY_MONEY = 10001,
  DUMMY_NAME = 10002,
  SHIPCLASS = 1440,
  HULL = 1441,
  SUPPLIES = 1442,
  ARMOR = 1443,
  ENGINES = 1444,
  WEAPONS = 1445,
  SHIELDS = 1446,
  KILLS = 1447,
  UNITS = 1449,
  INGAME_CHAT = 1450,
  MISSION_OBJECTIVES = 1128,
  END_MISSION = 1129,
  SCOREBOARD = 1452,
  SCORE = 1453,
  VOLUME = 1403,
  MUSIC = 1454,
  EFFECTS = 1455,
  COMMS = 1456,
  NETCHAT = 1457,
  TOOLTIPS = 1458,
  STATUS_TEXT = 1459,
  FOGOFWAR = 1460,
  MAPROTATE = 1461,
  ROLLOVER = 1462,
  SLOW = 1625,
  FAST = 1627,
  GAMMA = 1465,
  WEAPON_TRAILS = 1466,
  EMMISIVE_TEXTURES = 1467,
  LOD = 1468,
  TEXTURES = 1469,
  BACKGROUND = 1470,
  DEVICES = 1464,
  ENTER_NAME = 1472,
  CAMP_SELECT = 1473,
  SINGLE_PLAYER = 1102,
  MULTI_PLAYER = 1103,
  FRONT_OPTIONS = 1104,
  SKIRMISH = 1474,
  INTRO = 1475,
  SELECT_MISSION = 1058,
  RIGHTCLICK = 1476,
  JOIN = 1108,
  CREATE = 1109,
  ACCEPT = 1121,
  SCROLLRATE = 1488,
  QUIT = 1112,
  RESOLUTION = 1493,
  ENTER_STATE = 1494,
  PLAYER_LIST = 1495,
  PING = 1496,
  APP_NAME = 1,
  VERSION = 1428,
  HELP = 1497,
  NET_CONNECT = 1489,
  HARDWARE = 1492,
  GAME = 1498,
  SPEED = 1499,
  MAP = 1413,
  RESOURCES = 1471,
  TITLE_UNITS = 1528,
  TITLE_BUILDINGS = 1529,
  TITLE_RESOURCES = 1531,
  TITLE_TERRITORY = 1530,
  BRIEFING = 1165,
  VISIBILITY = 1504,
  DEVICE_CHOICE = 1505,
  SOFTWARE = 1506
};

} 




struct GT_PROGRESS_STATIC : GENBASE_DATA
{
	char fontName[32];
	struct COLOR
	{
		unsigned char red, green, blue;
	} normalText, overText, background, background2;
    char shapeFile[32];                        

	enum drawtype
	{
		NODRAW,
		FILL,
		HASH
	} backgroundDraw;

	bool bBackdraw;
};


struct PROGRESS_STATIC_DATA
{
    char staticType[32];
	PRG_STTXT::PROGRESS_STATIC_TEXT staticText;         
	enum alignmenttype
	{
		LEFT,
		CENTER,
		RIGHT,
		TOPLEFT,
	} alignment;
    S32 xOrigin, yOrigin;
	S32 width, height;				
};






















			    










namespace HSTTXT
{
enum STATIC_TEXT
{
	NOTEXT = 0,
	ENGINE = 1444,
	ARMOR = 1443,
	WEAPON = 1445,
	SHIELD = 1446,
	SENSOR = 1477,
	SUPPLY = 1478,
	TROOPSHIP = 1479,
	HARVESTER = 1480,
	TENDER = 1481,
	FIGHTER = 1482,
	FLEET = 1483,
	MISSLEPAK = 1484,
	RAM = 1485,
	LEECH = 1549,
	LEGIONAIRE = 1550,
	SIPHON = 1551,
	GALIOT = 1552,
};
} 






struct GT_HOTSTATIC : GENBASE_DATA
{
	char fontType[32];
};


struct HOTSTATIC_DATA
{
	U32 baseImage;
	U32 numTechLevels;			
	S32 xOrigin, yOrigin, width, height;
	S32 barStartX;
	U32 barSpacing;
	HSTTXT::STATIC_TEXT text;
	struct _color
	{
		U8 red,green,blue;
	}textColor;
};



















			    














struct GT_SHIPSILBUTTON : GENBASE_DATA
{
	SINGLE redYellowBreak;
	SINGLE yellowGreenBreak;
};


struct SHIPSILBUTTON_DATA
{
	S32 xOrigin, yOrigin;
};


















			    










namespace ICONTXT
{
	enum ICON_TOOLTIP
	{
	NO_TOOLTIP			= 0,
	SYSTEM_INSUPPLY		= 811,
	SYSTEM_OUTSUPPLY	= 812
	};
};





struct GT_ICON : GENBASE_DATA
{
};


struct ICON_DATA
{
	U32 baseImage;
    S32 xOrigin, yOrigin;
	ICONTXT::ICON_TOOLTIP tooltip;
};




















			    














struct GT_QUEUECONTROL : GENBASE_DATA
{
};


struct QUEUECONTROL_DATA
{
    S32 xOrigin, yOrigin;
	S32 width, height;
};
















struct GT_TOOLBAR
{
	
	
	
	char vfxShapeType[32];		
	char vfxToolBar[4][32];

	RECT contextRect;				
	RECT sysmapRect[4];
	RECT sectorMapRect[4];
	U32 topBarX,topBarY;

	
	
	

	struct COMMON
	{
		HOTBUTTON_DATA rotateLeft, returnDefaultView, rotateRight, minimize, restore, options, chat, go, starMap, exitSysMap,  
			hpDiplomacy, hpResearch,hpFleetOfficer,hpIndustrial,hpIdleCivilian,missionObjectives;
		STATIC_DATA gas,metal,crew;
		STATIC_DATA commandPts,shipclass;
		ICON_DATA inSupply,notInSupply;
	} common[4];




 	struct NONE
	{
		STATIC_DATA nothing;
	} none;

	struct FABRICATOR
	{
		M_OBJCLASS type;		
		HOTBUTTON_DATA sell,stop,repair;
		EDIT_DATA shipname;
		STATIC_DATA hull;
		TABCONTROL_DATA fabTab;
		struct TAB1
		{
			BUILDBUTTON_DATA plat0,plat1,plat2,plat3,plat4,plat5,plat6,plat7,plat8,plat9,plat10,plat11,plat12,plat13,plat14,plat15;
		}basicTab;
		struct TAB2
		{
			BUILDBUTTON_DATA plat16,plat17,plat18,plat19,plat20,plat21,plat22,plat23,plat24,plat25,plat26,plat27,plat28,plat29,plat30,plat31;
		}advancedTab;
		struct TAB4
		{
			BUILDBUTTON_DATA plat32,plat33,plat34,plat35,plat36,plat37,plat38,plat39,plat40,plat41,plat42,plat43,plat44,plat45,plat46,plat47;
		}defenceTab;
		struct TAB5
		{
			BUILDBUTTON_DATA plat48,plat49,plat50,plat51,plat52,plat53,plat54,plat55,plat56,plat57,plat58,plat59,plat60,plat61,plat62,plat63;
		}moonTab;
		struct TAB3
		{
			HOTSTATIC_DATA techarmor,techengine,techsheild,techsensors;
		}statisticsTab;
	} fabricator,M_Weaver,S_Forger,V_Shaper;

	struct LINDUSTRIAL
	{
		M_OBJCLASS type;		
		ICON_DATA inSupply,notInSupply;
		HOTBUTTON_DATA rally,stop;
		STATIC_DATA hull,metalStorage,gasStorage,crewStorage,location,disabledText;
		BUILDBUTTON_DATA build0,build1,build2,build3,build4,build5,build6,build7,build8,build9; 
		QUEUECONTROL_DATA buildQueue;
	} lindustrial,hq,hindustrial,
		M_Cocoon,M_Niad,S_Acropolis,S_Pavilion,
		S_Sanctum,S_GreaterPavilion,
		V_Locus, V_Compiler, V_Formulator;

	struct GENERIC
	{	
		M_OBJCLASS type;		
		ICON_DATA inSupply,notInSupply;
		STATIC_DATA hull,supplies,location,disabledText;
	} turret;

	
	struct RESEARCH
	{
		M_OBJCLASS type;		
		ICON_DATA inSupply,notInSupply;
		HOTBUTTON_DATA stop;
		STATIC_DATA hull,supplies,metalStorage,gasStorage,crewStorage,location,disabledText;
		RESEARCHBUTTON_DATA research0,research1,research2,research3,research4,research5,research6,research7,research8,research9;
		QUEUECONTROL_DATA buildQueue;
	} M_Plantation,proplab,ballistics,advHull,awsLab,lrsensor,
		hanger,weapons,displacement,
		M_BlastFurnace,M_ExplosivesRange,M_CarrionRoost,M_BioForge,
		M_FusionMill,M_CarpacePlant,M_HybridCenter,M_PlasmaSpitter,
		S_HelionVeil,S_XenoChamber,S_Anvil,S_MunitionsAnnex,
		M_EyeStock,M_MutationColony,S_TurbineDock,S_Bunker,
		V_Hatchery, V_CochleaDish, V_ClawOfVyrie, V_HammerOfVyrie, V_EyeOfVyrie;

	struct BUILD_RES
	{
		M_OBJCLASS type;		
		ICON_DATA inSupply,notInSupply;
		HOTBUTTON_DATA stop,rally,noAuto,autoOre,autoGas;
		STATIC_DATA hull,metalStorage,gasStorage,crewStorage,location,disabledText;
		RESEARCHBUTTON_DATA research0,research1,research2,research3,research4,research5,research6,research7,research8,research9,research10,research11,research12,research13,research14,research15;
		BUILDBUTTON_DATA build0,build1,build2,build3,build4,build5; 
		QUEUECONTROL_DATA buildQueue;
	}M_Thripid,academy,refinery,M_Collector,M_GreaterCollector,
		M_WarlordTraining,S_SentinalTower,S_Citidel,T_HeavyRefinery,T_SuperHeavyRefinery,S_Oxidator,
		V_Coalescer, V_Gudgeon, V_TempleOfVyrie, outpost;

	struct FLEET
	{
		M_OBJCLASS type;		
		HOTBUTTON_DATA escort,patrol,stop,attackPosition;
		HOTBUTTON_DATA tacticPeace,tacticStandGround, tacticDefend, tacticSeek;
		MULTIHOTBUTTON_DATA formation1, formation2, formation3, formation4, formation5, formation6;
		TABCONTROL_DATA fabTab;
		struct TAB1
		{
			MULTIHOTBUTTON_DATA admiralHead;
			STATIC_DATA o_namearea,o_kills,o_hull;
			HOTBUTTON_DATA order1,order2,order3,order4,order5,order6;
			MULTIHOTBUTTON_DATA specialOrders0,specialOrders1,specialOrders2,specialOrders3,specialOrders4,specialOrders5,specialOrders6,specialOrders7,
				specialOrders8,specialOrders9,specialOrders10,specialOrders11;
		}orderTab;
		struct TAB2
		{
			SHIPSILBUTTON_DATA ship0,ship1,ship2,ship3,ship4,ship5,ship6,ship7,ship8,ship9,ship10,ship11,ship12,ship13,ship14,ship15,ship16,ship17,ship18,ship19,ship20,ship21;
		}silTab;
		struct TAB3
		{
			STATIC_DATA namearea,hull,kills;
			HOTSTATIC_DATA techarmor,techsupply,techengine,techsheild,techweapon,techsensors,techspecial;
		}statTab;
		struct TAB4
		{
			STATIC_DATA k_namearea,k_hull,k_kills;
			MULTIHOTBUTTON_DATA kit0,kit1,kit2,kit3,kit4,kit5,kit6,kit7,kit8,kit9,kit10,kit11,kit12,kit13;
			MULTIHOTBUTTON_DATA kitDisplay0,kitDisplay1;
			QUEUECONTROL_DATA kitQueue;
		}kitTab;
	} fleet[4];
	
	struct WARTURRET
	{
		M_OBJCLASS type;
		ICON_DATA inSupply,notInSupply;
		STATIC_DATA hull,supplies,location;
		MULTIHOTBUTTON_DATA specialweapon;
		HOTBUTTON_DATA fighterStanceNormal,fighterStancePatrol;
		HOTSTATIC_DATA techarmor,techsupply,techsheild,techweapon,techsensors,techspecial;
	}WarTurret[4];

	struct INDIVIDUAL
	{	
		M_OBJCLASS type;		
		HOTBUTTON_DATA patrol,stop,escort,stanceAttack,stanceDefend,stanceStand,stanceStop,
			supplyStanceAuto,supplyStanceNoAuto,supplyStanceResupplyOnly,cloak,attackPosition;
		HOTBUTTON_DATA fighterStanceNormal,fighterStancePatrol,ejectArtifact;
		MULTIHOTBUTTON_DATA specialweapon,specialweapon1,specialweapon2,artifact;
		EDIT_DATA namearea;
		STATIC_DATA hull,supplies,kills,metal,gas;
		HOTSTATIC_DATA techarmor,techsupply,techengine,techsheild,techweapon,techsensors,techspecial;
	} individual[4];

	struct GROUP
	{
		M_OBJCLASS type;		
		MULTIHOTBUTTON_DATA specialweapon,specialweapon1,specialweapon2;
		HOTBUTTON_DATA escort,patrol,stop,cloak,attackPosition;
		HOTBUTTON_DATA stanceAttack,stanceDefend,stanceStand,stanceStop,
			supplyStanceAuto,supplyStanceNoAuto,supplyStanceResupplyOnly;
		HOTBUTTON_DATA fighterStanceNormal,fighterStancePatrol;
		SHIPSILBUTTON_DATA ship0,ship1,ship2,ship3,ship4,ship5,ship6,ship7,ship8,ship9,ship10,ship11,ship12,ship13,ship14,ship15,ship16,ship17,ship18,ship19,ship20,ship21;
	} group[4];
	
};


















			    




























struct GT_ADMIRALBAR
{
	RECT screenRect;
	STATIC_DATA background;
	STATIC_DATA admiralKey;
	HOTBUTTON_DATA hotCreate, hotRepair, hotResupply, hotDisband, hotTransfer, hotAssault;
	
	char vfxType[32];
	TABCONTROL_DATA tab;
	struct TAB1
	{
		HOTBUTTON_DATA hotTerran[5];
	} terranTab;
	struct TAB2
	{
		HOTBUTTON_DATA hotMantis[6];
	} mantisTab;
	struct TAB3
	{
		HOTBUTTON_DATA hotSolarian[5];
	} solarianTab;




};


















			    


















struct GT_BRIEFING
{
	RECT screenRect;
	STATIC_DATA background, title;
	BUTTON_DATA start, replay, cancel;
	RECT rcTeletype;
	RECT rcComm[4];
	ANIMATE_DATA animFuzz[4];
};


















			    
















struct GT_MENUOBJECTIVES
{
	RECT screenRect;
	STATIC_DATA background, staticObjectives, staticName;
	RECT rcTeletype;
	BUTTON_DATA buttonOk;
	BUTTON_DATA checkObjectives[9];
	STATIC_DATA staticObjectiveArray[9];
};


















			    










struct GT_MESSAGEBOX
{
	STATIC_DATA background, title;
	STATIC_DATA message;
	BUTTON_DATA ok, cancel;
	BUTTON_DATA okAlone;
};


















			    







struct GT_MOVIESCREEN
{
	RECT screenRect;
};


















			    










struct GT_CREDITS
{
	RECT screenRect;
	STATIC_DATA staticBackground;
};






















			    















struct GT_PAUSE
{
	RECT screenRect;
	STATIC_DATA staticTitle, staticDescription[8];
};


















			    













struct BT_FANCY_LAUNCH : BASE_LAUNCHER
{





	
	char animation[32];
	char hardpoint[64];
	SINGLE animTime;
	SINGLE effectDuration;
	SFX::ID warmupSound;
	SINGLE_TECHNODE neededTech;
	bool bSpecialWeapon:1;
	bool bTargetRequired:1;
	bool bWormHole:1;
	UNIT_SPECIAL_ABILITY specialAbility;

};




struct FANCY_LAUNCH_SAVELOAD
{
	U32 dwTargetID;
	Vector targetPos;
	U32 attacking;		
	SINGLE refireDelay; 
	SINGLE currentRot;
	Vector offsetVector;	
};




















			    













struct BT_CLOAK_LAUNCHER : BASE_LAUNCHER
{
	SINGLE cloakSupplyUse;
	SINGLE cloakShutoff;
	SINGLE_TECHNODE techNode;
};




struct CLOAK_LAUNCHER_SAVELOAD
{
	bool bCloakEnabled:1;
	bool bPrepareToggle:1;
	SINGLE cloakSupplyCount;
};




















			    













struct BT_MULTICLOAK_LAUNCHER : BASE_LAUNCHER
{
	SINGLE cloakSupplyUse;
	SINGLE cloakShutoff;
	SINGLE targetSupplyCostPerHull;
	SINGLE_TECHNODE techNode;
};




struct MULTICLOAK_LAUNCHER_SAVELOAD
{
	SINGLE cloakSupplyCount;
	U32 cloakTargetID;
	U32 decloakTime;
	bool bCloakingTarget:1;
	bool bCloakEnabled:1;
	bool bPrepareToggle:1;
};




















			    












struct BT_PING_LAUNCHER : BASE_LAUNCHER
{
	SINGLE_TECHNODE techNode;
};




struct PING_LAUNCHER_SAVELOAD
{
};




















			    



















struct BT_WORMHOLE_LAUNCHER : BASE_LAUNCHER
{
	SINGLE_TECHNODE techNode;
	SINGLE damagePerSec;
};



struct WORMHOLE_LAUNCHER_SAVELOAD
{
	U32 numTargets;
	U32 targetsLeft;
	U32 targetIDs[21];
	U32 workingID;
	SINGLE lastChanceTimer;

	GRIDVECTOR targetPos;
	U32 targetSystemID;

	enum _modes
	{
		WORM_IDLE,
		WORM_WAITING_FOR_SYNC,
		WORM_WORKING,
		WORM_DONE
	}mode;
};




















			    

















struct BT_JUMP_LAUNCHER : BASE_LAUNCHER
{
	char jumpParticle[32];
};



struct JUMP_LAUNCHER_SAVELOAD
{
	U32 workingID;
	GRIDVECTOR targetPos;
	enum JumpMode
	{
		NO_JUMP,
		JUMPING_OUT,
		JUMPING_IN,
	}jumpMode;
	SINGLE timer;
};




















			    

















struct BT_ARTILERY_LAUNCHER : BASE_LAUNCHER
{
	SINGLE areaRadius;
	U32 damagePerSec;
	bool bSpecial;
	char explosionType[32];
	char flashType[32];
};



struct ARTILERY_LAUNCHER_SAVELOAD
{
	GRIDVECTOR targetPos;
	SINGLE timer;
	bool bShooting;
};




















			    



















struct BT_TRACTOR_WAVE_LAUNCHER : BASE_LAUNCHER
{
	SINGLE durration;
	char spaceWaveType[32];
	SINGLE waveFrequency;
	SINGLE waveLifeTime;
};



struct TRACTOR_WAVE_LAUNCHER_SAVELOAD
{
	bool bActive;
	GRIDVECTOR tractorPos;
	SINGLE timer;

	U32 targets[12];
	U32 numTargets;

	
	U32 syncRemoveList[12];
	U32 numSyncRemove;
};




















			    

















struct BT_BARRAGE_LAUNCHER : BASE_LAUNCHER
{
	SINGLE damagePerSec;
	SINGLE rangeRadius;
	char flashType[32];
};



struct BARRAGE_LAUNCHER_SAVELOAD
{
};




















			    



















struct BT_TALORIAN_LAUNCHER : BASE_LAUNCHER
{
};



struct TALORIAN_LAUNCHER_SAVELOAD
{
	U32 numTargets;
	U32 targetIDs[12];

	bool bShildOn;
};




















			    



















struct BT_BUFF_LAUNCHER : BASE_LAUNCHER
{
	SINGLE rangeRadius;
	enum BuffType
	{
		SINUATOR_EFFECT,
		STASIS_EFFECT,
		REPELLENT_EFFECT,
		DESTABILIZER_EFFECT,
		TALLORIAN_EFFECT,
		AGIES_EFFECT,
		RESEARCH_EFFECT,
		CONSTRUCTION_EFFECT,
		SHIELD_JAM_EFFECT,
		WEAPON_JAM_EFFECT,
		SENSOR_JAM_EFFECT,
	}buffType;
	enum TargetType
	{
		ALLIES_ONLY,
		ALLIES_SHIPS_ONLY,
		ALLIES_PLATS_ONLY,
		ALLIES_PLANET_ONLY,
		ENEMIES_ONLY,
		ENEMIES_SHIPS_ONLY,
		ENEMIES_PLATS_ONLY,
		ALL_TARGETS,
		SHIPS_ONLY,
		PLATS_ONLY,
	}targetType;
	enum SupplyUseType
	{
		SU_PLATFORM_STANDARD = 0,
		SU_SHIP_SUPPLIES_TOGGLE,
	}supplyUseType;

	char visualName[32];
};



struct BUFF_LAUNCHER_SAVELOAD
{
	U32 numTargets;
	U32 targetIDs[12];
	bool bToggleOn;
};



struct BT_SYSTEM_BUFF_LAUNCHER : BASE_LAUNCHER
{
	enum BuffType
	{
		INTEL_CENTER_EFFECT,
	}buffType;
	enum TargetType
	{
		ALL_PLAYERS,
		ALLIES_ONLY,
		ENEMIES_ONLY,
	}targetType;
};



struct SYSTEM_BUFF_LAUNCHER_SAVELOAD
{
};




























struct ArtifactButtonInfo
{
	U32 baseButton;		
	HBTNTXT::BUTTON_TEXT tooltip;
	HBTNTXT::HOTBUTTONINFO helpBox;
	HBTNTXT::MULTIBUTTONINFO hintBox;
};


struct BT_BUFF_ARTIFACT : BASE_WEAPON_DATA
{
	char shipName[32];
	enum BuffType
	{
		B_BLOCKADE_INHIBITOR,
	}buffType;
	ArtifactButtonInfo buttonInfo;
};


struct BT_LISTEN_ARTIFACT : BASE_WEAPON_DATA
{
	char shipName[32];
	SINGLE successChance;
	ArtifactButtonInfo buttonInfo;
};


struct BT_TERRAFORM_ARTIFACT : BASE_WEAPON_DATA
{
	char shipName[32];
	ArtifactButtonInfo buttonInfo;
	char targetPlanetType[32];
	SINGLE changeTime;
};




















			    

















struct BT_ARTIFACT_LAUNCHER : BASE_LAUNCHER
{
	char artifactName[32];
};



struct ARTIFACT_LAUNCHER_SAVELOAD
{
	U32 targetID;
};




















			    

















struct BT_TEMPHQ_LAUNCHER : BASE_LAUNCHER
{
	SINGLE chargeRegenRate;
	SINGLE chargeUseRate;
	SINGLE maxCharge;
};



struct TEMPHQ_LAUNCHER_SAVELOAD
{
	SINGLE chargeTimer;
	U32 charge;
};




















			    

















struct BT_NOVA_LAUNCHER : BASE_LAUNCHER
{
	SINGLE chargeTime;
	char novaExplosion[32];
};



struct NOVA_LAUNCHER_SAVELOAD
{
	bool bNovaOn;
	SINGLE timer;
};



















			    

















struct BT_MOON_RESOURCE_LAUNCHER : BASE_LAUNCHER
{
	SINGLE oreRegenRate;
	SINGLE gasRegenRate;
	SINGLE crewRegenRate;
};



struct MOON_RESOURCE_LAUNCHER_SAVELOAD
{
	bool bHaveBoostedPlanet;
};




















			    

















struct BT_REPAIR_LAUNCHER : BASE_LAUNCHER
{
	SINGLE rangeRadius;
	SINGLE repairRate;
	SINGLE repairCostPerPoint;
};



struct REPAIR_LAUNCHER_SAVELOAD
{
};




















			    






























struct GT_IGOPTIONS
{
	RECT screenRect;
	STATIC_DATA background, title;

	BUTTON_DATA buttonSave, buttonLoad;
	BUTTON_DATA buttonOptions;
	BUTTON_DATA buttonRestart;
	BUTTON_DATA buttonResign;
	BUTTON_DATA buttonAbdicate;
	BUTTON_DATA buttonReturn;
};


struct GT_OPTIONS
{
	RECT screenRect;
	STATIC_DATA background, title;

	STATIC_DATA staticName;
	LISTBOX_DATA listNames;
	BUTTON_DATA buttonNew, buttonChange, buttonDelete;

	STATIC_DATA staticSound, staticMusic, staticComm, staticChat, staticSpeed, staticScroll, staticMouse;
	SLIDER_DATA sliderSound, sliderMusic, sliderComm, sliderChat, sliderSpeed, sliderScroll, sliderMouse;
	BUTTON_DATA pushSound, pushMusic, pushComm, pushChat;

	STATIC_DATA staticDInput;
	BUTTON_DATA pushDInput;

	STATIC_DATA staticStatus, staticRollover, staticSectorMap, staticRightClick, staticSubtitles;
	BUTTON_DATA pushStatus, pushRollover, pushSectorMap, pushRightClick, pushSubtitles;

	STATIC_DATA staticGamma, staticResolution;
	DROPDOWN_DATA dropResolution;
	SLIDER_DATA sliderGamma;

	STATIC_DATA staticShips3D, staticTrails, staticEmissive, staticDetail, staticDrawBack;
	BUTTON_DATA pushTrails, pushEmissive, pushDetail;
	SLIDER_DATA slideDrawBack, sliderShips3D;

	STATIC_DATA   staticDevice;
	DROPDOWN_DATA dropDevice;

	STATIC_DATA static3DHardware;
	BUTTON_DATA push3DHardware;

	TABCONTROL_DATA tab;

	BUTTON_DATA buttonOk, buttonCancel;
};



















			    


























struct GT_DIPLOMACYMENU
{
	RECT screenRect;
	STATIC_DATA	background;
	STATIC_DATA staticTitle, staticName, staticRace, staticAllies, staticMetalTitle, staticGasTitle, staticCrewTitle;
	STATIC_DATA staticNames[7], staticRaces[7];
	BUTTON_DATA buttonCrew[7], buttonMetal[7], buttonGas[7];
	BUTTON_DATA buttonAllies[7];

	STATIC_DATA staticCrew, staticMetal, staticGas;
	BUTTON_DATA buttonOk, buttonReset, buttonCancel, buttonApply;

	DIPLOMACYBUTTON_DATA diplomacyButtons[7];
};

struct GT_PLAYERCHATMENU
{
	RECT screenRect;
	STATIC_DATA background;
	STATIC_DATA staticNames[7], staticRaces[7];
	BUTTON_DATA checkNames[7];
	BUTTON_DATA buttonAllies, buttonEnemies, buttonEveryone;
	LISTBOX_DATA listChat;
	EDIT_DATA editChat;
	BUTTON_DATA buttonClose;
	STATIC_DATA staticTitle;
	STATIC_DATA staticChat;
};






















			    























struct GT_LOADSAVE
{
	RECT screenRect;
	RECT screenRect2D;
	STATIC_DATA background, staticLoad, staticSave, staticFile;
	BUTTON_DATA open, save, cancel, deleteFile;
	EDIT_DATA editFile;
	LISTBOX_DATA list;
};


















			    























struct GT_SYSTEM_KIT_SAVELOAD
{
	RECT screenRect;
	STATIC_DATA background, staticLoad, staticSave, staticFile;
	BUTTON_DATA open, save, cancel;
	EDIT_DATA editFile;
	LISTBOX_DATA list;
};










































			    


















typedef U32 DPID;		

namespace CQGAMETYPES
{
	enum DIFFICULTY
	{
		NODIFFICULTY,
		EASY,
		AVERAGE,
		HARD
	};

	enum TYPE
	{
		HUMAN,
		COMPUTER
	};

	enum COMP_CHALANGE
	{
		EASY_CH,
		AVERAGE_CH,
		HARD_CH,
		IMPOSIBLE_CH,
		NIGHTMARE_CH,
	};

	enum STATE
	{
		OPEN,		
		CLOSED,		
		ACTIVE,		
		READY		
	};
	enum RACE
	{
		NORACE,
		TERRAN,
		MANTIS,
		SOLARIAN,
		VYRIUM,
	};
	enum COLOR
	{
		UNDEFINEDCOLOR,	
		YELLOW,
		RED,
		BLUE,
		PINK,
		GREEN,
		ORANGE,
		PURPLE,
		AQUA
	};
	enum TEAM
	{
		NOTEAM,
		_1,
		_2,
		_3,
		_4
	};

	struct SLOT
	{
		TYPE type:3;
		COMP_CHALANGE compChalange:4;
		STATE state:3;
		RACE race:4;
		COLOR color:5;
		TEAM team:4;
		U32  zoneSeat:3;
		DPID dpid;			
	};

	enum GAMETYPE			
	{
		KILL_UNITS=-2,
		KILL_HQ_PLATS,
		MISSION_DEFINED,	
		KILL_PLATS_FABS
	};

	enum MONEY				
	{
		LOW_MONEY=-2,
		MEDIUM_MONEY,
		HIGH_MONEY
	};
	enum MAPTYPE			
	{
		SELECTED_MAP=-2,
		USER_MAP,			
		RANDOM_MAP
	};
	enum MAPSIZE			
	{
		SMALL_MAP=-2,
		MEDIUM_MAP,
		LARGE_MAP
	};
	enum TERRAIN			
	{
		LIGHT_TERRAIN=-2,
		MEDIUM_TERRAIN,
		HEAVY_TERRAIN
	};
	enum STARTING_UNITS		
	{
		UNITS_MINIMAL=-2,
		UNITS_MEDIUM,
		UNITS_LARGE
	};
	enum VISIBILITYMODE		
	{
		VISIBILITY_NORMAL=-1,
		VISIBILITY_EXPLORED,
		VISIBILITY_ALL
	};

	enum RANDOM_TEMPLATE	
	{
		TEMPLATE_NEW_RANDOM = -2,
		TEMPLATE_RANDOM,
		TEMPLATE_RING,
		TEMPLATE_STAR,
	};

	enum COMMANDLIMIT		
	{
		COMMAND_LOW=-2,
		COMMAND_NORMAL,
		COMMAND_MID,
		COMMAND_HIGH
	};

	struct OPTIONS
	{
		U32 version;
		GAMETYPE gameType:3;
		S32 gameSpeed:5;	
		U32 regenOn:1;
		U32 spectatorsOn:1;
		U32 lockDiplomacyOn:1;
		U32 numSystems:5;
		MONEY money:2;
		MAPTYPE mapType:2;
		RANDOM_TEMPLATE templateType:2;
		MAPSIZE mapSize:2;
		TERRAIN terrain:2;
		STARTING_UNITS units:2;
		VISIBILITYMODE visibility:2;
		COMMANDLIMIT  commandLimit:2;












	};

	struct _CQGAME
	{
		U32 activeSlots:8;			
		U32 bHostBusy:1;			
		U32 startCountdown:4;	

		SLOT slot[8];
	};

}  



struct CQGAME : CQGAMETYPES::_CQGAME, CQGAMETYPES::OPTIONS
{
};














struct GT_GLOBAL_VALUES
{
	struct TargetingValues
	{
		SINGLE movePenaltySelf;
		SINGLE movePenaltyTarget;
		SINGLE minAccuracy;
	} targetingValues;


	struct INDKILLCHART
	{
		U32 rookie, novice, veteran, elite, superElite, superdouperElite;
	} individualKillChart;





	struct ADMIRALKILLCHART
	{
		U32 rearAdmiral, viceAdmiral, admiral, starAdmiral, superstarAdmiral, superdouperAdmiral;
	} admiralKillChart;






	struct RACE_TARGETING_BONUSES
	{
		SINGLE noRace;
		SINGLE terran;
		SINGLE mantis;
		SINGLE solarian;
		SINGLE vyrium;
	} raceBonuses;




	struct TechUpgrades
	{
		SINGLE		engine[6];			  
		SINGLE 		hull[6];				  
		SINGLE 		supplies[6];			  
		SINGLE		targeting[6];
		SINGLE		damage[6];			  
		SINGLE		shields[6];			  
		SINGLE		shipTargetingExp[6];	  
		SINGLE		admiralTargetingExp[6]; 
		SINGLE		fleet[6];				  
		SINGLE		sensors[6];			  
		SINGLE		tanker[6];			  
														  
		SINGLE		fighter[6];			  
		SINGLE		tender[6];			  
	};
	
	


	struct _upgrades
	{
		TechUpgrades noRace;
		TechUpgrades terran;
		TechUpgrades mantis;
		TechUpgrades solarian;
		TechUpgrades vyrium;
	} techUpgrades;



};


struct MT_GlobalData
{
	M_STRINGW gameDescription;
	M_STRINGW playerNames[8];		
	U8 playerAssignments[8];		
	M_RACE playerRace[9];		
	U32 lastPartID;			
	U8 currentPlayer:4;  
	U8 maxPlayers:4;
	bool bGlobalLighting:1;		
	bool bScriptUIControl:1;	
	U8  allyMask[8];		
	U8 colorAssignment[9];	
	U8 visibilityMask[8];	
	U32 missionID; 

	struct PlayerTechLevel
	{
		enum TECHLEVEL
		{
			LEVEL0,
			LEVEL1,
			LEVEL2,
			LEVEL3,
			LEVEL4,
			LEVEL5
		};

		TECHLEVEL engine:4;
		TECHLEVEL hull:4;
		TECHLEVEL supplies:4;
		TECHLEVEL targeting:4;
		TECHLEVEL damage:4;
		TECHLEVEL shields:4;
		TECHLEVEL sensors:4;

		
		TECHLEVEL fighter:4;
		TECHLEVEL tanker:4;
		TECHLEVEL tender:4;
		TECHLEVEL fleet:4;

	} playerTechLevel[9][5];

	TECHNODE techNode[9];
	TECHNODE workingTechNode[9];
	TECHNODE availableTechNode;
	U32 gas[9];
	U32 gasMax[9];
	U32 metal[9];
	U32 metalMax[9];
	U32 crew[9];
	U32 crewMax[9];
	U32 totalCommandPts[9];
	U32 usedCommandPts[9];
	U32 maxComPts[9];				

	struct GAMESTATS
	{
		
		U32 metalGained:20;
		U32 numUnitsBuilt:12;

		U32 gasGained:17;
		U32	numUnitsDestroyed:12;
		U32	numAdmiralsBuilt:3;

		U32 crewGained:20;	
		U32 numUnitsLost:12;

		U32 numPlatformsBuilt:12;
		U32 numPlatformsDestroyed:12;
		U32	numJumpgatesControlled:8;

		U32 numPlatformsLost:12;
		U32 numUnitsConverted:10;
		U32	numPlatformsConverted:10;

		U32	numResearchComplete:10;
		U32 percentSystemsExplored:8;	
	};
	GAMESTATS gameStats[9];
	
	U8  gameScores[9];

	U32 updateCount;				
	U32 lastStreamID;
	U32 lastTeletypeID;

	struct OBJECTIVES
	{
		int index;
		U32 mission_name_stringID;
		U32 overview_stringID;
		U32 objective_stringID[32];
		
		enum ObjectiveState
		{
			Pending,
			Complete,
			Failed
		} state[32];
		
		bool bObjectiveSecondary[32];
	} objectives;

	M_STRING scriptLibrary;			
	M_STRING baseTerrainMap;		
	CQGAME gameSettings;
};

























			    







typedef BASIC_DATA BT_GROUP;	



struct GROUPOBJ_SAVELOAD
{
	U32 dwMissionID;
	U32 numObjects;
};





















			    







struct GT_SCROLLBAR : GENBASE_DATA
{
	char upButtonType[32];
	char downButtonType[32];
	struct COLOR
	{
		unsigned char red, green, blue;
	} thumbColor, backgroundColor, disabledColor;
	bool bHorizontal:1;
	char shapeFile[32];
};

struct SCROLLBAR_DATA 
{
	char scrollBarType[32];
};




















			    









struct WORLDLIGHT_DATA
{
	U8 red,green,blue;
	Vector direction;
};

struct AMBIENT_DATA
{
	U8 red,green,blue;
};

struct LIGHT_DATA
{
	WORLDLIGHT_DATA light1;
	WORLDLIGHT_DATA light2;
	AMBIENT_DATA ambient;
};

struct BT_LIGHT : BASIC_DATA
{
	U8 red,green,blue;
	S32 range;
	Vector direction;
	SINGLE cutoff;
	bool bInfinite:1;
	bool bAmbient:1;
};

struct CQLIGHT_SAVELOAD
{
	U8 red,green,blue;
	S32 range;
	Vector position;
	Vector direction;
	SINGLE cutoff;
	bool bInfinite;
	char name[32];
	U32 systemID;
	bool bAmbient;
};


















			    










enum ENGINETRAIL_TAPPER_TYPE
{
	EngineTrailTapperZero = 1, 
	EngineTrailStraight, 
	EngineTrailBleed 
};



struct BT_ENGINETRAIL : BASE_EFFECT
{
	SINGLE width;
	U32 segments;
	SINGLE timePerSegment;
	ENGINETRAIL_TAPPER_TYPE tapperType;
	SINGLE tapperMod;
	char hardpoint[64];
	char texture[32];
	struct _colorMod
	{
		U8 red;
		U8 green;
		U8 blue;
		U8 alpha;
	}colorMod, engineGlowColorMod;
	char engineGlowTexture[32];
	SINGLE engineGlowWidth;
};



struct BASE_ENGINETRAIL_SAVELOAD
{
};

struct ENGINETRAIL : BASE_ENGINETRAIL_SAVELOAD
{
};





























 













struct GlobalEffectsOptions
{
	enum OPTVAL { off, on=-1 };
	__readonly U32 version;
	OPTVAL bWeaponTrails:1;
	OPTVAL bEmissiveTextures:1;
	OPTVAL bExpensiveTerrain:1;
	OPTVAL bTextures:1;
	OPTVAL bBackground:1;
	OPTVAL bHighBackground:1;
	OPTVAL bFastRender:1;
	OPTVAL bNoForceMinLOD:1;
	OPTVAL bNoForceMaxLOD:1;
	U32	nFlatShipScale:3;	
}  CQEFFECTS;


struct GlobalEffectsOptions_V3
{
	enum OPTVAL { off, on=-1 };
	__readonly U32 version;
	OPTVAL bWeaponTrails:1;
	OPTVAL bEmissiveTextures:1;
	OPTVAL bExpensiveTerrain:1;
	OPTVAL bTextures:1;
	OPTVAL bBackground:1;
	OPTVAL bFastRender:1;
	OPTVAL b3DShips:1;
	OPTVAL bNoForceMinLOD:1;
	OPTVAL bNoForceMaxLOD:1;
};


























			    






typedef M_STRING MT_STRINGPACK[];


























			    





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
	SUB_RES_MIMIC = 4530,
	SUB_RES_ENGINE = 4531,
	SUB_RES_FIGHTER = 4532,
	SUB_RES_GRAVWELL = 4533,
	SUB_RES_HULL = 4534,
	SUB_RES_LEECH = 4535,
	SUB_RES_RAM = 4536,
	SUB_RES_REPCLOUD = 4537,
	SUB_RES_REP_WAVE = 4538,
	SUB_RES_SENSOR = 4539,
	SUB_RES_SHIELD = 4540,
	SUB_RES_SUPPLY = 4541,
	SUB_RES_SIPHON = 4542,
	SUB_RES_RESUPPLY = 4543,
	SUB_RES_WEAPON = 4544,
	SUB_RES_SHROUD = 4545,
	SUB_RES_DESTABILIZER = 4546,
	SUB_RES_GAS = 4547,
	SUB_RES_LEGIONAIRE = 4548,
	SUB_RES_MASS_DISRUPTOR = 4549,
	SUB_RES_ORE = 4550,
	SUB_RES_PROTEUS = 4551,
	SUB_RES_SYNTH = 4552,
	SUB_RES_GALIOT = 4553,
	SUB_RES_AUGER = 4554,
	SUB_RES_TEMPEST = 4555,
	SUB_RES_PROBE = 4556,
	SUB_RES_AEGIS = 4557,
	SUB_RES_VAMP = 4558,
	SUB_RES_CLOAK = 4559,
	SUB_RES_MISSILE = 4560,
	SUB_RES_HARVESTER = 4561,
	SUB_RES_TROOPSHIP = 4562,
	SUB_UPGRADE = 4563,
};

struct BASE_RESEARCH_DATA : BASIC_DATA
{
	RESEARCH_TYPE type;
	ResourceCost cost;
	U32 time;
};



struct BT_RESEARCH : BASE_RESEARCH_DATA
{
	SINGLE_TECHNODE researchTech;
	SINGLE_TECHNODE dependancy;
	char resFinishedSound[32];
	RESEARCH_SUBTITLE resFinishSubtitle;
};


struct BT_UPGRADE : BASE_RESEARCH_DATA
{
	SINGLE_TECHNODE dependancy;
	U32 extensionID;
	char resFinishedSound[32];
	RESEARCH_SUBTITLE resFinishSubtitle;
};



struct BT_ADMIRAL_RES : BT_RESEARCH
{
	char flagshipType[32];
};




















			    


























			    
























			    















struct CAMERA_DATA
{
	__readonly U32 version;

	SINGLE worldRotation;
	__readonly Vector lookAt;

	SINGLE FOV_x, FOV_y;
	Vector position;
	SINGLE pitch;
	SINGLE minZ, maxZ;

	U32 zoomRate;		
	U32 rotateRate;		
	SINGLE toggleZoomZ;	
};











struct SYSTEM_VIEW
{
	char systemKitName[32];
	char backgroundName[32];
};

struct GT_SYSTEM_KIT
{
	char fileName[32];
	U32 numLights;
	struct _lightInfo
	{
		U8 red,green,blue;
		S32 range;
		Vector position;
		Vector direction;
		SINGLE cutoff;
		bool bInfinite;
		char name[32];
		bool bAmbient;
	}lightInfo[10];
};

struct SYSTEM_DATA
{
	U32 id;
	char name[16];
	char systemKitName[32];
	char backgroundName[32];
	S32 x,y;
	S32 sizeX,sizeY;
	U32 alertState[9];
	CAMERA_DATA cameraBuffer;
	U32 inSupply;
	U32 inRootSupply;
};

struct MT_SECTOR_SAVELOAD
{
	struct SYSTEM_DATA sysData[16];
	U32 currentSystem;
};









struct BT_BLACKHOLE_DATA : BASIC_DATA
{
	BILLBOARD_MESH billboardMesh[3];
	char ringObjectName[32];
	char sysMapIcon[32];
	MISSION_DATA missionData;
	U16 damage;
};

struct BASE_BLACKHOLE_SAVELOAD
{
	U8 targetSys[16];
	U8 numTargetSys;
	U8 currentTarget;
	SINGLE timer;
	U32 workingID;
	U32 waitingID;
	NETGRIDVECTOR waitingJumpPos;
};



struct BLACKHOLE_SAVELOAD : BASE_BLACKHOLE_SAVELOAD
{
	
	MISSION_SAVELOAD mission;

	
	TRANS_SAVELOAD trans_SL;
};



struct BLACKHOLE_DATA
{
	bool jumpsTo[17];
};




















			    



























struct GT_ENDGAME
{
	RECT screenRect;
	STATIC_DATA background, banner;
	BUTTON_DATA cont;
	LISTBOX_DATA list;

	STATIC_DATA staticMenu;
	TABCONTROL_DATA tab;
	struct TAB1
	{
		STATIC_DATA staticOverviewTitles[4], staticOverviewUnits[8], staticOverviewBuildings[8];
		STATIC_DATA staticOverviewResources[8], staticOverviewTotals[8];
	} overviewTab;
	struct TAB2
	{
		STATIC_DATA staticUnitsTitles[6], staticUnitsMade[8], staticUnitsLost[8], staticUnitsKills[8];
		STATIC_DATA staticUnitsConverted[8], staticUnitsAdmirals[8], staticUnitsTotals[8];
	} unitsTab;
	struct TAB3
	{
		STATIC_DATA staticBuildingsTitles[6], staticBuildingsMade[8], staticBuildingsLost[8], staticBuildingsDestroyed[8];
		STATIC_DATA staticBuildingsConverted[8], staticBuildingsResearch[8], staticBuildingsTotals[8];
	} buildingsTab;
	struct TAB5
	{
		STATIC_DATA staticResourcesTitles[4], staticResourcesCrew[8], staticResourcesOre[8];
		STATIC_DATA staticResourcesGas[8], staticResourcesTotals[8];
	} resourceTab;

	STATIC_DATA staticPlayer, staticPlayerArray[8];
	STATIC_DATA staticTime;
	STATIC_DATA staticDescription;
};


















			    



















			    































			    













struct BUILD_SAVELOAD
{
	bool building:1;
	bool whole:1;
	bool pause:1;
	bool bReverseBuild:1;
	bool bDismantle:1;

	SINGLE timeSpent;
	U16 hullPointsAdded;
	U16 hullPointsFinish;
	U32 buildProcessID;
	S32 builderType;
};



















			    












struct REPAIR_SAVELOAD
{
	U32 repairAtID;
	U32 repairAgentID;
	U8 repairMode;  
};














struct BASE_FABRICATOR_SAVELOAD;
struct BASE_FLAGSHIP_SAVELOAD;
struct BASE_TROOPSHIP_SAVELOAD;
struct BASE_SUPPLYSHIP_SAVELOAD;




struct WARP_SAVELOAD
{
	enum WARP_STAGE
	{
		WS_NONE=0,
		WS_WARP_IN,
		WS_LIMBO,
		WS_WARP_OUT,
		WS_PRE_WARP
	};

	WARP_STAGE warpStage;
	Vector warpInVector;
	float warpSpeed;
	float warpRadius;
	SINGLE warpTimer;  
	U32 targetGateID;
	U32 inTargetGateID;
	U32 warpAgentID;
	SINGLE stop_speed;
	SINGLE stop_heading;
	SINGLE releaseTime;
};



struct SPACESHIP_SAVELOAD
{
	struct TOBJMOVE
	{
		
		
		
		GRIDVECTOR goalPosition, currentPosition, jumpToPosition;
		GRIDVECTOR pathList[4];
		SINGLE cruiseDepth;		
		SINGLE cruiseSpeed;		
		SINGLE groupAcceleration;  
		SINGLE mockRotationAngle;
		U32    moveAgentID, jumpAgentID;

		
		
		
		U32 overrideAttackerID;		
		enum 
		{ 
			OVERRIDE_NONE,
			OVERRIDE_PUSH,
			OVERRIDE_DESTABILIZE,
			OVERRIDE_ORIENT
		} overrideMode;
		union
		{
			SINGLE overrideSpeed;
			SINGLE overrideYaw;
		};
		GRIDVECTOR overrideDest;	

		
		
		
		GRIDVECTOR patrolVectors[2];
		S8		   patrolIndex;

		
		
		
		U8   pathLength;		
		bool bMoveActive:1;
		bool bAutoMovement:1;
		bool bCompletionAllowed:1;	
		bool bRotatingBeforeMove:1;	
		bool bSyncNeeded:1;			
		bool bFinalMove:1;			
		bool bPathOverflow:1;		
		bool bMockRotate:1;
		bool bPatroling:1;

		
		
		
		bool bRollUp:1;
		bool bAltUp:1;

		bool slowMove:1;
	} tobjmove;
	
	
	TRANS_SAVELOAD   trans_SL;

	
	BUILD_SAVELOAD build_SL;

	
	REPAIR_SAVELOAD repair_SL;

	

	WARP_SAVELOAD warp_SL;

	

	CLOAK_SAVELOAD cloak_SL;

	DAMAGE_SAVELOAD damage_SL;

	
	U32 firstNuggetID;

	
	MISSION_SAVELOAD mission;
};

enum MINEMODES
{
	MLAY_IDLE,
	MLAY_MOVING_TO_POS,
	MLAY_WAIT_CLIENT,
	MLAY_ROTATING_TO_POS,
	MLAY_LAYING,
};

struct BASE_MINELAYER_SAVELOAD
{
	struct GRIDVECTOR targetMinePos;	
	SINGLE layingTime;
	U32 workingID;
	U32 minefieldMissionID;
	bool bRotating:1;
	MINEMODES mode;
};

struct MINELAYER_SAVELOAD : SPACESHIP_SAVELOAD 
{
	BASE_MINELAYER_SAVELOAD mineLayerSaveLoad;
};


struct BASE_GUNBOAT_SAVELOAD
{
	__hexview U32 dwTargetID;
	U32 attackAgentID;
	U32 escortAgentID;
	__hexview U32 dwEscortID;			
	UNIT_STANCE unitStance;
	FighterStance fighterStance;
	GRIDVECTOR  defendPivot;
	U32 launcherAgentID;
	U32 launcherID;

	bool bWaitingForAdmiral:1;
	bool bRepairUnderway:1;
	bool bUserGenerated:1;
	bool bSpecialAttack:1;
	bool bArtifactUse:1;
	bool bDefendPivotValid:1;
};





struct GUNBOAT_SAVELOAD : SPACESHIP_SAVELOAD
{
	BASE_GUNBOAT_SAVELOAD	gunSaveLoad;
};


struct BASE_RECONPROBE_SAVELOAD
{
	SINGLE probeTimer,totalTime;
	Vector goal;
	U32 workingID;
	bool bMoving:1;
	bool bJumping:1;
	bool bGone:1;
	bool bLauncherDelete:1;
	bool bNoMoreSync:1;
};


struct RECONPROBE_SAVELOAD : SPACESHIP_SAVELOAD
{
	BASE_RECONPROBE_SAVELOAD	baseSaveLoad;
};


struct BASE_TORPEDO_SAVELOAD
{
	SINGLE tTimer,totalTime;
	bool bClearing;
	Vector clearPos;
	__hexview U32 targetID;
	U32 ownerID;
};


struct TORPEDO_SAVELOAD : SPACESHIP_SAVELOAD
{
	BASE_TORPEDO_SAVELOAD	baseSaveLoad;
};


enum HARVEST_MODE
{
	HAR_NO_MODE_AI_ON,
	HAR_NO_MODE_AI_OFF,
	HAR_NO_MODE_CLIENT_WAIT_CANCEL,

	
	HAR_MOVING_TO_REFINERY,
	HAR_AT_REFINERY_CLIENT,
	HAR_MOVING_TO_READY_REFINERY_CLIENT,
	HAR_MOVING_TO_READY_REFINERY_HOST,

	HAR_WAITING_TO_DOCK,
	HAR_WAIT_DOCKING_CANCEL_CLIENT,
	HAR_DOCKING_REFINERY,
	HAR_DOCKED_TO_REFINERY,

	

	HAR_WAIT_FOR_NUGGET_BEGIN_CLIENT,
	HAR_MOVING_TO_NUGGET,
	HAR_ROTATING_TO_NUGGET,
	HAR_WAIT_NUGGET_START,
	HAR_MOVING_TO_READY_NUGGET_CLIENT,
	HAR_MOVING_TO_READY_NUGGET_HOST,
	HAR_ROTATING_TO_READY_NUGGET_CLIENT,
	HAR_ROTATING_TO_READY_NUGGET_HOST,
	HAR_WAIT_NUGGET_ARRIVAL,
	HAR_NUGGETING,
	HAR_WAIT_NUGGET_CANCEL_CLIENT,

	
	
	HAR_MOVE_TO_RECOVERY,
	HAR_ROTATING_TO_RECOVERY,
	HAR_RECOVERING,
	HAR_MOVE_TO_RECOVERY_DROP

};


struct BASE_HARVEST_SAVELOAD
{	
	U8 gas;
	U8 metal;

	__hexview U32 targetPartID;
	__hexview U32 recoverPartID;
	U32 workingID;
	U32 posibleWorkingID;
	NETGRIDVECTOR harvestVector;

	Vector recoverPos;
	SINGLE recoverYaw;
	SINGLE recoverTime;
	SINGLE harvestRemainder;

	HARVEST_MODE mode;
	M_NUGGET_TYPE nuggetType;

	bool bNuggeting:1;
	bool bLockingPlatform:1;
	bool bHostParking:1;
	bool bDockingWithGas:1;
	bool bRotating:1;
	bool bTowingShip:1;
	bool bSendIdle:1;
	bool bNuggetCancelOp:1;
};


struct HARVEST_SAVELOAD : SPACESHIP_SAVELOAD
{
	BASE_HARVEST_SAVELOAD baseSaveLoad;
};


struct SPACESHIP_VIEW
{
	MISSION_SAVELOAD * mission;
	BASIC_INSTANCE	  *	rtData;
	S16 gamma;
	SINGLE contrast;
	union SHIPDATA
	{
		BASE_GUNBOAT_SAVELOAD		* gunboat;
		BASE_MINELAYER_SAVELOAD		* minelayer;
		BASE_RECONPROBE_SAVELOAD	* reconprobe;
		BASE_TORPEDO_SAVELOAD		* torpedo;
		BASE_HARVEST_SAVELOAD		* harvester;
		BASE_FABRICATOR_SAVELOAD	* fabricator;
		BASE_FLAGSHIP_SAVELOAD		* flagship;
		BASE_TROOPSHIP_SAVELOAD		* troopship;
		BASE_SUPPLYSHIP_SAVELOAD	* supplyship;
		void						* nothing;
	} shipData;
};



















			    

















struct FAB_SAVELOAD		
{
	U32 buildeeID,repaireeID,selleeID;
	BOOL32 bDoorsOpen:1;
	BOOL32 bBuilding:1;
	BOOL32 bDrones:1;
	BOOL32 bReturning:1;
	U32 doorPause;

	
	U32 queueStart;
	U32 queueSize;
	U8 lastIndex;
	U32 buildQueue[15];
	U8 buildQueueIndex[15];
};

struct BUILDQUEUE_SAVELOAD
{
	U32 queueStart;
	U32 queueSize;
	U8 lastIndex;
	U32 buildQueue[15];
	U8 buildQueueIndex[15];
};


enum FAB_MODES
{
	FAB_IDLE,
	FAB_MOVING_TO_TARGET,
	FAB_MOVING_TO_POSITION,
	FAB_WAITING_INIT_CONS_CLIENT,
	FAB_AT_TARGET_CLIENT,
	FAB_WATING_TO_START_BUILD_CLIENT,
	FAB_MOVING_TO_READY_TARGET_CLIENT,
	FAB_MOVING_TO_READY_TARGET_HOST,

	FAB_BUILDING,
	FAB_UNBUILD,

	FAB_EXPLODING,

	FAB_MOVING_TO_TARGET_REPAIR,
	FAB_WAIT_REPAIR_INFO_CLIENT,
	FAB_MOVING_TO_READY_TARGET_REPAIR_HOST,
	FAB_AT_TARGET_REPAIR_CLIENT,
	FAB_MOVING_TO_READY_TARGET_REPAIR_CLIENT,
	FAB_WATING_TO_START_REPAIR_CLIENT,
	FAB_REPAIRING,

	FAB_MOVING_TO_TARGET_DISM,
	FAB_WAIT_DISM_INFO_CLIENT,
	FAB_MOVING_TO_READY_TARGET_DISM_HOST,
	FAB_AT_TARGET_DISM_CLIENT,
	FAB_MOVING_TO_READY_TARGET_DISM_CLIENT,
	FAB_WATING_TO_START_DISM_CLIENT,
	FAB_DISMING,
};

enum MOVESTAGE
{
	MS_PATHFIND,
	MS_BEELINE,
	MS_DONE
};

struct BASE_FABRICATOR_SAVELOAD	 
{
	Vector dir;

	FAB_MODES mode;
	U32 targetPlanetID;
	U32 targetSlotID;
	U32 buildingID;
	U32 workingID;
	U32 workTargID;

	ResourceCost workingCost;

	U32 oldHullPoints;

	GRIDVECTOR targetPosition;

	
	MOVESTAGE moveStage;
	Vector destPos;
	SINGLE destYaw;


	U8 lastTab;
};


struct FABRICATOR_SAVELOAD : SPACESHIP_SAVELOAD		
{
	BASE_FABRICATOR_SAVELOAD  baseSaveLoad;
	FAB_SAVELOAD fab_SL;
};















struct ShipFilters
{




		FORMATION_FILTER positive_filter;








		FORMATION_FILTER negative_filter;




	U32 max;
	U32 min;
	bool bOverflowOnly;
};




struct FleetGroupDef
{
	ShipFilters filters[6];
	bool bActive:1;
	bool bOerflowCreation:1;
	U32 parentGroup;
	U32 parentGroupNumber;
	U32 priority;
	U32 creationNumber;

	enum RelativeType 
	{
		RT_CENTER, 
		RT_GROUP, 
	} relativeTo;
	U32 relativeGroupID;
	enum Relation 
	{
		RE_EDGE,
		RE_CENTER,
	} relation ;

	S32 relDirX;
	S32 relDirY;

	struct AdvancedPlacement
	{
		enum PlacementType
		{
			PT_NORMAL,
			PT_LINE,
			PT_LINE_END,
			PT_V,
		} placementType;
		S32 placementDirX;
		S32 placementDirY;
	}advancedPlacement;

	enum AIType
	{
		NORMAL,
		ROVER,
		SPOTTER,
		STRIKER
	}aiType;
};




struct AdmiralBonuses
{
	struct BonusValues
	{
		SINGLE damage;
		SINGLE supplyUsage;
		SINGLE rangeModifier;
		SINGLE speed;
		SINGLE sensors;
		SINGLE defence;
		SINGLE platformDamage;
		struct RaceDamage
		{
			SINGLE terran;
			SINGLE mantis;
			SINGLE celareon;
			SINGLE vyrium;
		}hatedRaceDamage;
		struct ArmorDamage
		{
			SINGLE noArmor;
			SINGLE lightArmor;
			SINGLE mediumArmor;
			SINGLE heavyArmor;
		}hatedArmorDamage;
	}baseBonuses, favoredShipBonus, favoredArmor;
	M_OBJCLASS bonusShips[5];
	bool dNoArmorFavored:1;
	bool dLightArmorFavored:1;
	bool dMediumArmorFavored:1;
	bool dHeavyArmorFavored:1;;
};

struct BT_FORMATION : BASIC_DATA
{
	struct ButtonInfo
	{
		U32 baseButton;		
		HBTNTXT::BUTTON_TEXT tooltip;
		HBTNTXT::HOTBUTTONINFO helpBox;
		HBTNTXT::MULTIBUTTONINFO hintBox;
	}buttonInfo;
	FleetGroupDef groups[8];
	bool bFreeForm;
	U32 moveTargetRange;
	U32 spotterRange;
	enum CloakUsage
	{
		CU_NONE,
		CU_ALL,
		CU_RECON_ONLY,
		CU_RECON_AND_SPOTTERS,
	}cloakUsage;
	bool bAdmiralAttackControl;
	AdmiralBonuses formationBonuses;
	struct FormationSpecials
	{
		bool bRepairWholeFleet:1;
	}formationSpecials;
};




struct BT_COMMAND_KIT: BASE_RESEARCH_DATA
{
	AdmiralBonuses kitBonuses;

	struct ButtonInfo
	{
		U32 baseButton;		
		HBTNTXT::BUTTON_TEXT tooltip;
		HBTNTXT::HOTBUTTONINFO helpBox;
		HBTNTXT::MULTIBUTTONINFO hintBox;
	}buttonInfo;
	SINGLE_TECHNODE dependancy;

	char formations[3][32];
};







struct BT_FLAGSHIP_DATA : BASE_SPACESHIP_DATA
{	
	SINGLE attackRadius;
	AdmiralBonuses admiralBonueses;

	char commandKits[14][32];
	char startingFormations[2][32];

	struct _toolbarInfo
	{
		U32 baseImage;
		HBTNTXT::BUTTON_TEXT buttonText;
		HBTNTXT::HOTBUTTONINFO buttonStatus;
		HBTNTXT::MULTIBUTTONINFO buttonHintbox;
	}toolbarInfo;

	U32 maxQueueSize;
};							











struct BASE_FLAGSHIP_SAVELOAD
{
	U32 dockAgentID;
	__hexview U32 dockshipID;
	__hexview U32 targetID;
	S32 idleTimer;
	enum ADMIRAL_MODE
	{
		NOMODE,
		MOVING,
		DEFENDING,
		ATTACKING
	} mode;
	bool bAttached;
	U8   numDeadShips;		
	struct DMESSAGES		
	{
		bool    inTrouble:1;
		bool	damage50:1;
		bool	damage75:1;
	} dmessages;

	struct SMESSAGES		
	{
		bool	suppliesLow:1;
		bool	suppliesOut:1;
	} smessages;

	S32 forecastTimer;
	enum FORECAST
	{
		NOFORECAST,
		GOOD,
		BAD,
		UGLY
	} lastForecast;
	ADMIRAL_TACTIC admiralTactic;
	U32 formationID;
	U32 knownFormations[6];
	bool bHaveFormationPost:1;
	bool bRovingAllowed:1;
	bool flipFormationPost:1;
	NETGRIDVECTOR formationPost;
	S8 formationDirX;
	S8 formationDirY;
	NETGRIDVECTOR targetFormationPost;
	S8 targetFormationDirX;
	S8 targetFormationDirY;
	U32 formationTargetID;
	U32 moveDoneHint;
	U32 targetFormationGateID;

	U32 commandKitsArchID[2];
	SINGLE buildTime;
	enum AdmiralHotkey {NO_HOTKEY = 0, ADMIRAL_F1 = 1, ADMIRAL_F2, ADMRIAL_F3, ADMIRAL_F4, ADMIRAL_F5, ADMIRAL_F6 } admiralHotkey;

};


struct FLAGSHIP_SAVELOAD : SPACESHIP_SAVELOAD 
{
	BASE_FLAGSHIP_SAVELOAD flagshipSaveLoad;
	BUILDQUEUE_SAVELOAD fab_SL;
};



























			    















			    
































struct BASE_GUNPLAT_SAVELOAD;
struct BASE_BUILDPLAT_SAVELOAD;
struct BASE_GENERALPLAT_SAVELOAD;
struct BASE_SUPPLYPLAT_SAVELOAD;
struct BASE_REFINEPLAT_SAVELOAD;
struct BASE_REPAIRPLAT_SAVELOAD;



struct BASE_PLATFORM_SAVELOAD
{
	
	TRANS_SAVELOAD trans_SL;

	
	BUILD_SAVELOAD build_SL;

	DAMAGE_SAVELOAD damage_SL;

	EXTENSION_SAVELOAD extend_SL;

	
	MISSION_SAVELOAD mission;

	U8 shadowVisibilityFlags;
	U32 exploredFlags;

	S8 shadowUpgrade[8];
	S8 shadowUpgradeWorking[8];
	U8 shadowUpgradeFlags[8];
	SINGLE shadowPercent[8];
	U16 shadowHullPoints[8];
	U16 shadowMaxHull[8];

	
	__hexview U16  buildSlot;		
	__hexview U32  buildPlanetID;
	__hexview U32 firstNuggetID;		
	bool bSetCommandPoints:1;
	bool bPlatDead:1;
	bool bPlatRealyDead:1;
};


struct BASE_OLDSTYLE_PLATFORM_SAVELOAD
{
	U32 workingID;

	NETVECTOR rallyPoint;

	
	U32 reasearchArchetypeID;
	SINGLE researchTimeSpent;

	
	U32 resupplyTargetID;

	U8 stackStart;
	U8 stackEnd;
	U32 supplyStack[10];

	bool bResupplyReturning;
	bool bResearching:1;
	bool bDockLocked:1;
	bool bNeedToSendLossOfTarget:1;
	bool bNeedToSendNewTarget:1;
	bool bSupplierOut:1;
};


struct OLDSTYLE_PLATFORM_SAVELOAD : BASE_PLATFORM_SAVELOAD
{
	
	FAB_SAVELOAD fab_SL;

	

	TRANSFORM resupplyTransform;

	
	BASE_OLDSTYLE_PLATFORM_SAVELOAD	 baseOldstylePlatformSaveload;
};


struct PLATFORM_VIEW
{
	MISSION_SAVELOAD *	mission;
	BASIC_INSTANCE *	rtData;
	union PLATFORM {
		BASE_OLDSTYLE_PLATFORM_SAVELOAD * oldstyle;
		BASE_GUNPLAT_SAVELOAD * gunplat;
		BASE_BUILDPLAT_SAVELOAD * buildPlat;
		BASE_GENERALPLAT_SAVELOAD * generalPlat;
		BASE_SUPPLYPLAT_SAVELOAD * supplyPlat;
		BASE_REFINEPLAT_SAVELOAD * refinePlat;
		BASE_REPAIRPLAT_SAVELOAD * repairPlat;
		void * nothing;
	} platform;
};















struct BT_PLAT_GUN : BASE_PLATFORM_DATA
{
	SINGLE outerWeaponRange;
	bool bNoLineOfSight;		
	char launcherType[5][32];
	char specialLauncherType[32];
};













struct BASE_GUNPLAT_SAVELOAD
{
	__hexview U32 dwTargetID;
	U32 attackAgentID;
	U32 launcherAgentID;
	UNIT_STANCE unitStance;
	enum FighterStance fighterStance;
	U8 launcherID;
	SINGLE buildTimeSpent;
	U32 upgradeID;
	U32 workingID;
	bool bUserGenerated:1;    
	bool isPreferredTarget:1;		  
	bool bUpgrading:1;
	bool bDelayed:1;
	bool bSpecialAttack:1;
};


struct GUNPLAT_SAVELOAD : BASE_PLATFORM_SAVELOAD
{
	BASE_GUNPLAT_SAVELOAD gunplatSaveload;
};


















			    












struct BT_PLAT_SUPPLY_DATA : BASE_PLATFORM_DATA
{
	SINGLE supplyLoadSize;
	SINGLE supplyRange;
	char supplyDroneType[32];
	char supplyDroneHardpoint[32];
};












struct BASE_SUPPLYPLAT_SAVELOAD
{
	U32 resupplyTargetID;

	U8 stackStart;
	U8 stackEnd;
	U32 supplyStack[10];

	bool bResupplyReturning:1;
	bool bNeedToSendLossOfTarget:1;
	bool bNeedToSendNewTarget:1;
	bool bSupplierOut:1;
};


struct SUPPLYPLAT_SAVELOAD : BASE_PLATFORM_SAVELOAD
{
	BASE_SUPPLYPLAT_SAVELOAD supplyPlatSaveload;

	TRANSFORM resupplyTransform;
};


















			    














struct BT_PLAT_BUILD_DATA : BASE_PLATFORM_DATA
{
	char ship_hardpoint[64];	
	DRONE_RELEASE drone_release[2];
	S32 buildRate;
	U32 maxQueueSize;
};
















struct BASE_BUILDPLAT_SAVELOAD
{
	U32 workingID;

	struct NETGRIDVECTOR rallyPoint;

	
	SINGLE buidTimeSpent;
	
	U32 constructionID;

	bool bResearching:1;
	bool bUpgrading:1;
};


struct BUILDPLAT_SAVELOAD : BASE_PLATFORM_SAVELOAD
{
	
	FAB_SAVELOAD fab_SL;

	BASE_BUILDPLAT_SAVELOAD buildPlatSaveload;
};


















			    













struct BT_PLAT_BUILDSUP_DATA : BASE_PLATFORM_DATA
{
	char ship_hardpoint[64];	
	DRONE_RELEASE drone_release[2];
	S32 buildRate;
	U32 maxQueueSize;

	SINGLE supplyRadius;
	SINGLE supplyPerSecond;
};












struct BASE_BUILDSUPPLAT_SAVELOAD
{
	U32 workingID;

	struct NETGRIDVECTOR rallyPoint;

	SINGLE supplyTimer;

	
	U32 constructionID;
	SINGLE buildTimeSpent;

	bool bResearching:1;
};


struct BUILDSUPPLAT_SAVELOAD : BASE_PLATFORM_SAVELOAD
{
	
	FAB_SAVELOAD fab_SL;

	BASE_BUILDSUPPLAT_SAVELOAD buildPlatSaveload;
};


















			    













struct BT_PLAT_REFINE_DATA : BASE_PLATFORM_DATA
{
	char ship_hardpoint[64];	
	char dock_hardpoint[64];   
	DRONE_RELEASE drone_release[2];
	S32 buildRate;
	U32 maxQueueSize;
	
	char harvesterArchetype[32];

	SINGLE gasRate[4];
	SINGLE metalRate[4];
	SINGLE crewRate[4];
};












struct BASE_REFINEPLAT_SAVELOAD
{
	U32 workingID;

	struct NETGRIDVECTOR rallyPoint;

	
	U32 constructionID;
	SINGLE buidTimeSpent;

	SINGLE gasHarvested;
	SINGLE metalHarvested;
	SINGLE crewHarvested;

	HARVEST_STANCE defaultHarvestStance;

	U32 dockLockerID;
	U16 numDocking;
	bool bFreeShipMade:1;
	bool bResearching:1;
	bool bUpgrading:1;
};


struct REFINEPLAT_SAVELOAD : BASE_PLATFORM_SAVELOAD
{
	
	FAB_SAVELOAD fab_SL;

	BASE_REFINEPLAT_SAVELOAD refinePlatSaveload;
};


















			    












struct BT_PLAT_REPAIR_DATA : BASE_PLATFORM_DATA
{
	SINGLE supplyPerSecond;
	U32 repairRate;
	U32 supplyRate;
	SINGLE supplyRange;
	char repairDroneType[32];
	char repairDroneHardpoint[32];
};












struct BASE_REPAIRPLAT_SAVELOAD
{
	U32 workingID;
	U32 potentialWorkingID;

	U32 repairTargetID;
	enum REPAIR_MODE
	{
		REP_NONE,
		REP_WAIT_FOR_DOCK,
		REP_REPAIRING
	} mode;

	S32 oldHullPoints;

	SINGLE supplyTimer;

	SINGLE upgradeTimeSpent;
	U32 upgradeID;

	bool bDockLocked:1;
	bool bUpgradeDelay:1;
	bool bUpgrading:1;
	bool bTakenCost:1;
};


struct REPAIRPLAT_SAVELOAD : BASE_PLATFORM_SAVELOAD
{
	BASE_REPAIRPLAT_SAVELOAD repairPlatSaveload;

	TRANSFORM repairTransform;
};


















			    












struct BT_PLAT_GENERAL_DATA : BASE_PLATFORM_DATA
{
};












struct BASE_GENERALPLAT_SAVELOAD
{
};


struct GENERALPLAT_SAVELOAD : BASE_PLATFORM_SAVELOAD
{
	BASE_GENERALPLAT_SAVELOAD generalPlatSaveload;
};


















			    
















struct BT_PLAT_SELL_DATA : BASE_PLATFORM_DATA
{
	char ship_hardpoint[64];
	U32 maxQueueSize;
};














struct BASE_SELLPLAT_SAVELOAD
{
	U32 workingID;
	U32 potentialWorkingID;
	U32 sellTargetID;
	enum SELL_MODE
	{
		SEL_NONE,
		SEL_WAIT_FOR_DOCK,
		SEL_SELLING
	} mode;

	bool bDockLocked:1;
};


struct SELLPLAT_SAVELOAD : BASE_PLATFORM_SAVELOAD
{
	BASE_SELLPLAT_SAVELOAD sellPlatSaveload;

	FAB_SAVELOAD fab_SL;
};


















			    












struct BT_PLAT_JUMP_DATA : BASE_PLATFORM_DATA
{
};












struct BASE_JUMPPLAT_SAVELOAD
{
	U32 jumpGateID;
	bool bOwnGate:1;
	bool bLockGate:1;
};


struct JUMPPLAT_SAVELOAD : BASE_PLATFORM_SAVELOAD
{
	BASE_JUMPPLAT_SAVELOAD jumpPlatSaveload;
};




















			    












struct BASE_PLAYERBOMB_SAVELOAD
{
	bool bDeployedPlayer;
};


struct PLAYERBOMB_SAVELOAD : BASE_PLAYERBOMB_SAVELOAD
{
	
	TRANS_SAVELOAD trans_SL;

	
	MISSION_SAVELOAD mission;
};



struct PLAYERBOMB_TYPE
{
	char archetypeName[32];
};


struct BT_PLAYERBOMB_DATA : BASIC_DATA
{
	struct _playerRace
	{
		PLAYERBOMB_TYPE minBombType[4];

		PLAYERBOMB_TYPE bombType[4];

		PLAYERBOMB_TYPE largeBombType[8];
	}race[4];

	char playerBomb_anim2D[32];
	S32 animSize;

	MISSION_DATA missionData;

	char filename[32];
};



















			    














struct GT_CHAT
{
	RECT screenRect;
	STATIC_DATA background, ask;
	EDIT_DATA chatbox;	
};


















			    













struct BT_RECON_LAUNCH : BASE_LAUNCHER
{
	char animation[32];
	char hardpoint[64];
	SINGLE animTime;
	SINGLE effectDuration;
	SFX::ID warmupSound;
	SINGLE_TECHNODE neededTech;
	UNIT_SPECIAL_ABILITY specialAbility;
	bool bWormWeapon;
	bool selfTarget;
};




struct RECON_LAUNCH_SAVELOAD
{
	Vector targetPos;
	U32 targetSystemID;
	U32 attacking;		
	SINGLE refireDelay; 
	U32 probeID;
	U32 targetID;
	bool bKillProbe:1;
};























			    
































struct BT_AEBOLT_DATA : BASE_WEAPON_DATA
{
	char fileName[32];					
	char explosionEffect[32];
	SFX::ID launchSfx;
	SINGLE maxVelocity;	
	U32 damage;								
	SINGLE explosionRange;							
	ARMOR_DATA armorData;
};


struct AEBOLT_SAVELOAD
{
	SINGLE time;
	SINGLE lastTime;
	Vector targetPos;
	U32 ownerID;
	U32 targetID;
	U32 numFound;
	U32 systemID;
	TRANS_SAVELOAD trans_SL;
};

struct BT_PKBOLT_DATA : BASE_WEAPON_DATA
{
	char fileName[32];					
	char explosionEffect[32];
	SFX::ID launchSfx;
	SINGLE maxVelocity;	
	char newPlanetType[32];
	SINGLE changeTime;
	char engineTrailType[32];
};


struct PKBOLT_SAVELOAD
{
	U32 ownerID;
	U32 targetID;
	U32 systemID;
	TRANS_SAVELOAD trans_SL;
};
struct BT_STASISBOLT_DATA : BASE_WEAPON_DATA
{
	char fileName[32];					
	char explosionEffect[32];
	SFX::ID launchSfx;
	SINGLE maxVelocity;	
	SINGLE explosionRange;							
	SINGLE duration;
	MISSION_DATA missionData;
};




struct BASE_STASISBOLT_SAVELOAD
{
	SINGLE time;
	struct GRIDVECTOR targetPos;
	U32 ownerID;

	
	GRIDVECTOR gvec[26];
	U32 numSquares;

	U32 numTargets;
	U32 targets[26];
	U32 lastSent;
	U32 targetID;
	U32 targetsHeld;

	bool bDeleteRequested:1;
	bool bGone:1;
	bool bLauncherDelete:1;
	bool bFreeTargets:1;
	bool bNoMoreSync:1;
};

struct STASISBOLT_SAVELOAD : BASE_STASISBOLT_SAVELOAD
{
	TRANS_SAVELOAD trans_SL;
	
	MISSION_SAVELOAD mission;
};

struct BT_REPELLENTCLOUD_DATA : BASE_WEAPON_DATA
{
	char explosionEffect[32];
	SFX::ID launchSfx;
	SINGLE duration;
	MISSION_DATA missionData;
	U32 damagePerSec;
	SINGLE centerRange;
};




struct BASE_REPELLENTCLOUD_SAVELOAD
{
	SINGLE time;
	SINGLE lastTime;
	struct GRIDVECTOR targetPos;
	U32 ownerID;

	
	GRIDVECTOR gvec[24];
	U32 numSquares;

	U32 numTargets;
	U32 targets[24];
	U32 targetsHeld;
	U32 lastSent;

	Vector sprayDir,sideDir;

	bool bDeleteRequested:1;
	bool bGone:1;
	bool bLauncherDelete:1;
	bool bFreeTargets:1;
	bool bNoMoreSync:1;
};

struct REPELLENTCLOUD_SAVELOAD : BASE_REPELLENTCLOUD_SAVELOAD
{
	TRANS_SAVELOAD trans_SL;
	
	MISSION_SAVELOAD mission;
};

struct BT_REPULSORWAVE_DATA : BASE_WEAPON_DATA
{
	SFX::ID launchSfx;
	SINGLE duration;
	SINGLE range;
	SINGLE ringTime;
	SINGLE interRingTime;
	MISSION_DATA missionData;
};



struct BASE_REPULSORWAVE_SAVELOAD
{
	struct GRIDVECTOR targetPos;
	U32 ownerID;

	SINGLE time;

	U32 numTargets;
	U32 targets[24];
	U32 targetsHeld;
	U32 lastSent;

	bool bDeleteRequested:1;
	bool bGone:1;
	bool bLauncherDelete:1;
	bool bFreeTargets:1;
	bool bNoMoreSync:1;
	bool bHasBeenInit:1;
};

struct REPULSORWAVE_SAVELOAD : BASE_REPULSORWAVE_SAVELOAD
{
	TRANS_SAVELOAD trans_SL;
	
	MISSION_SAVELOAD mission;
};

struct BT_TRACTOR_DATA : BASE_WEAPON_DATA
{
	char fileName[32];					
	char hardpoint[64];
	char contactBlastType[32];
	SFX::ID launchSfx;
	SINGLE duration;
	SINGLE damagePerSecond;
	SINGLE_TECHNODE neededTech;
	S32 supplyCost;
	SINGLE refirePeriod;
};

struct TRACTOR_SAVELOAD 
{
	U32			targetID;
	U32			systemID;
	SINGLE			time;
	SINGLE refireDelay;
	SINGLE mass;
};

struct BT_REPULSOR_DATA : BASE_WEAPON_DATA
{
	char fileName[32];					
	char hardpoint[64];
	char contactBlastType[32];
	SFX::ID launchSfx;
	SINGLE pushTime;
	SINGLE minimumMass;
	SINGLE basePushPower;
	SINGLE pushPerMass;
	SINGLE_TECHNODE neededTech;
};

struct REPULSOR_SAVELOAD
{
	U32			targetID, ownerID;
	U32			systemID;

	S32			time;

	SINGLE mass;
};


struct BT_OVERDRIVE_DATA : BASE_WEAPON_DATA
{
	SINGLE speed;
	SFX::ID launchSfx;
};

struct OVERDRIVE_SAVELOAD
{
	Vector destPos;
	U32 systemID;
	U32 ownerID;
};

struct BT_SWAPPER_DATA : BASE_WEAPON_DATA
{
	SFX::ID launchSfx;
};


struct BT_AEGIS_DATA : BASE_WEAPON_DATA
{
	SINGLE supplyPerSec;
	SINGLE_TECHNODE neededTech;
};

struct BASE_AEGIS_SAVELOAD
{
	bool bNetShieldOn:1;
	bool bShieldOn:1;
};

struct AEGIS_SAVELOAD : BASE_AEGIS_SAVELOAD
{
	TRANS_SAVELOAD trans_SL;
};


struct BT_MIMIC_DATA : BASE_WEAPON_DATA
{
	SINGLE supplyUse;
	SINGLE shutoff;
	SINGLE_TECHNODE techNode;
};

struct BASE_MIMIC_SAVELOAD
{
	U32 aliasArchetypeID;
	U8 aliasPlayerID;
	bool bCloakEnabled:1;
};

struct MIMIC_SAVELOAD : BASE_MIMIC_SAVELOAD
{

};




struct BT_ZEALOT_DATA : BASE_WEAPON_DATA
{
	SINGLE kamikazeSpeed;
	SINGLE damageAmount[3];
	char impactBlastType[32];
	SINGLE_TECHNODE neededTech;
};

enum ZSTAGE
{
	Z_NONE,
	Z_ROTATE,
	Z_THRUST
};

struct BASE_ZEALOT_SAVELOAD
{
	U32 dwMissionID;
	U32 targetID;
	U32 zealotArchetypeID;
	U32 systemID;

	ZSTAGE stage;
};

struct ZEALOT_SAVELOAD : BASE_ZEALOT_SAVELOAD
{
	TRANS_SAVELOAD trans_SL;
	U32	   visibilityFlags:8;
};



struct BT_SYNTHESIS_DATA : BASE_WEAPON_DATA
{
	SINGLE_TECHNODE neededTech;
	char animName[32];
};

enum SYN_STAGE
{
	SYN_NONE,
	SYN_ROTATE,
	SYN_ZAP,
	SYN_APPROACH,
	SYN_ABSORB
};

struct BASE_SYNTHESIS_SAVELOAD
{
	SYN_STAGE stage;
	SINGLE hullPointsPer,suppliesPer;
	U32 targetID;
};

struct SYNTHESIS_SAVELOAD : BASE_SYNTHESIS_SAVELOAD
{
	TRANS_SAVELOAD trans_SL;
};


struct BT_MASS_DISRUPTOR_DATA : BASE_WEAPON_DATA
{
	SFX::ID launchSfx;
	char fileName[32];
	char contactBlastType[32];
	char warpAnim[32];
	U16 animWidth;
	SINGLE boltSpeed;
	SINGLE damagePercent;
};

enum MD_STAGE
{
	MD_SHOOT,
	MD_DISRUPT
};

struct BASE_MASS_DISRUPTOR_SAVELOAD
{
	U32 ownerID,systemID;
	MD_STAGE stage;
	U32 targetID;
	U32 damageDealt;
	Vector targetDir;
	SINGLE dist;
};

struct MASS_DISRUPTOR_SAVELOAD : BASE_MASS_DISRUPTOR_SAVELOAD
{
	TRANS_SAVELOAD trans_SL;
};

struct BT_DESTABILIZER_DATA : BASE_WEAPON_DATA
{
	char fileName[32];					
	char explosionEffect[32];
	SFX::ID launchSfx;
	SINGLE maxVelocity;	
	SINGLE explosionRange;							
	SINGLE duration;
	MISSION_DATA missionData;
};




struct BASE_DESTABILIZER_SAVELOAD
{
	SINGLE time;
	struct GRIDVECTOR targetPos;
	U32 ownerID;

	U32 numTargets;
	U32 targetIDs[3];
	U32 lastSent;
	U32 targetID;
	U32 targetsHeld;

	bool bDeleteRequested:1;
	bool bGone:1;
	bool bLauncherDelete:1;
	bool bFreeTargets:1;
	bool bNoMoreSync:1;
};

struct DESTABILIZER_SAVELOAD : BASE_DESTABILIZER_SAVELOAD
{
	TRANS_SAVELOAD trans_SL;
	
	MISSION_SAVELOAD mission;
};

struct BT_DUMBRECONPROBE_DATA : BASE_WEAPON_DATA
{
	char fileName[32];					
	SFX::ID launchSfx;
	SINGLE maxVelocity;	
	SINGLE duration;
	MISSION_DATA missionData;
};

struct BASE_DUMBRECONPROBE_SAVELOAD
{
	SINGLE time;
	struct GRIDVECTOR targetPos;
	U32 ownerID;

	U32 targetID;

	bool bDeleteRequested:1;
	bool bGone:1;
	bool bLauncherDelete:1;
	bool bNoMoreSync:1;
};

struct DUMBRECONPROBE_SAVELOAD : BASE_DUMBRECONPROBE_SAVELOAD
{
	TRANS_SAVELOAD trans_SL;
	
	MISSION_SAVELOAD mission;
};

struct BT_SPACEWAVE_DATA : BASE_WEAPON_DATA
{
};






















			    





struct BASE_BUILD_OBJ : BASIC_DATA
{
	BUILDOBJCLASS boClass;
};

struct BT_TERRAN_BUILD : BASE_BUILD_OBJ
{
};

struct BT_MANTIS_BUILD : BASE_BUILD_OBJ
{
	char cocoonTextureName[32];
};

struct BT_SOLARIAN_BUILD : BASE_BUILD_OBJ
{
	char cocoonTextureName[32];
};



















			    



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
		LEVEL1,
		LEVEL2
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






struct BT_MAP_GEN
{
	struct _terrainTheme
	{
		char systemKit[6][32];

		char metalPlanets[6][32];
		char gasPlanets[6][32];
		char habitablePlanets[6][32];
		char otherPlanets[6][32];

		char moonTypes[6][32];

		MAP_GEN_ENUM::SECTOR_SIZE sizeOk;
		U32 minSize;
		U32 maxSize;
		MAP_GEN_ENUM::DMAP_FUNC sizeFunc;

		U32 numHabitablePlanets[3];
		U32 numMetalPlanets[3];
		U32 numGasPlanets[3];
		U32 numOtherPlanets[3];

		U32 minMoonsPerPlanet;
		U32 maxMoonsPerPlanet;
		MAP_GEN_ENUM::DMAP_FUNC moonNumberFunc;

		U32 numNuggetPatchesMetal[3];
		U32 numNuggetPatchesGas[3];

		struct _terrainInfo
		{
			char terrainArchType[32];
			SINGLE probability;
			U32 minToPlace;
			U32 maxToPlace;
			MAP_GEN_ENUM::DMAP_FUNC numberFunc;
			U32 size;
			U32 requiredToPlace;
			MAP_GEN_ENUM::OVERLAP overlap;
			MAP_GEN_ENUM::PLACEMENT placement;
		}terrain[20],nuggetMetalTypes[6],nuggetGasTypes[6];
		bool okForPlayerStart:1;
		bool okForRemoteSystem:1;
		SINGLE desitiy[3];

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
		}macros[15];
	}themes[30];
	
};























			    












struct BASE_MOVIE_CAMERA_SAVELOAD : MISSION_SAVELOAD
{
	Vector cam_lookAt;
	Vector cam_position;
	SINGLE cam_FOV_x;
	SINGLE cam_FOV_y;
};


struct MOVIE_CAMERA_SAVELOAD
{
	
	TRANS_SAVELOAD trans_SL;

	BASE_MOVIE_CAMERA_SAVELOAD baseSave;

};


struct BT_MOVIE_CAMERA_DATA : BASIC_DATA
{
	char fileName[32];
	MISSION_DATA missionData;
};

struct MOVIE_CAMERA_VIEW
{
	Vector cam_lookAt;
	Vector cam_position;
	SINGLE cam_FOV_x;
	SINGLE cam_FOV_y;
	M_STRING partName;
};

















			    









struct BT_OBJECT_GENERATOR : BASIC_DATA
{
	char fileName[32];
	MISSION_DATA missionData;
	SINGLE mean;
	SINGLE minDiff;
	char generateType[32];
	bool startEnabled;
};



struct BASE_OBJECT_GENERATOR_SAVELOAD
{
	SINGLE mean;
	SINGLE minDiff;
	SINGLE timer;
	SINGLE nextTime;
	U32 archID;
	bool bGenEnabled:1;
};



struct OBJECT_GENERATOR_SAVELOAD
{
	
	TRANS_SAVELOAD trans_SL;

	
	MISSION_SAVELOAD mission;

	BASE_OBJECT_GENERATOR_SAVELOAD baseSave;
};






struct OBJECT_GENERATOR_VIEW 
{
	MISSION_SAVELOAD * mission;
	Vector position;
	char generatorType[32];
	SINGLE mean;
	SINGLE minDiff;
	SINGLE timer;
	SINGLE nextTime;
	bool bGenEnabled:1;
	char partName[32];
};





















			    









struct BT_TRIGGER : BASIC_DATA
{
	char fileName[32];
	MISSION_DATA missionData;

	enum Type
	{
		SPHERE,
		REGION,
		LINE
	};
	Type type;
	float size;
};


struct BASE_TRIGGER_SAVELOAD
{
	U32 lastTriggerID;

	U32 triggerShipID;
	U32 triggerObjClassID;
	U32 triggerMObjClassID;
	U32 triggerPlayerID;
	SINGLE triggerRange;
	U32 triggerFlags;
	char progName[32];
	bool bEnabled:1;
	bool bDetectOnlyReady:1;
};



struct TRIGGER_SAVELOAD
{
	
	TRANS_SAVELOAD trans_SL;

	
	MISSION_SAVELOAD mission;

	BASE_TRIGGER_SAVELOAD baseSave;
};



struct TRIGGER_VIEW 
{
	MISSION_SAVELOAD * mission;
	Vector position;

	__hexview U32 triggerShipID;
	OBJCLASS triggerObjClassID;
	M_OBJCLASS triggerMObjClassID;
	U32 triggerPlayerID;
	SINGLE triggerRange;
	char progName[32];
	bool bEnabled:1;
	bool bDetectOnlyReady:1;
};





















			    













struct BT_SCRIPTOBJECT : BASIC_DATA
{
	char fileName[32];
	MISSION_DATA missionData;
	SFX::ID ambientSound;
	struct BLINKER_DATA blinkers;
	char ambient_animation[32];

	bool bSysMapActive:1;
};

struct BASE_SCRIPTOBJECT_SAVELOAD
{
	bool bTowed:1;
	bool bSysMapActive:1;

    U32 towerID;
};


struct SCRIPTOBJECT_SAVELOAD:BASE_SCRIPTOBJECT_SAVELOAD
{
	
	TRANS_SAVELOAD trans_SL;

	
	MISSION_SAVELOAD mission;

	U8  exploredFlags;
};






struct SCRIPTOBJECT_VIEW 
{
	MISSION_SAVELOAD * mission;
	Vector position;
};



























			    






typedef struct Streamer *HSTREAM;

struct SONG
{
	bool  playing:1;
	bool  looping:1;
	HSTREAM handle;
	SINGLE volume;
	char filename[32];
};

typedef SONG MT_MUSIC_DATA[4];



























			    












struct SPECIALABILITYINFO
{
	U32 baseSpecialWpnButton;		
	HBTNTXT::BUTTON_TEXT specialWpnTooltip;
	HBTNTXT::HOTBUTTONINFO specialWpnHelpBox;
	HBTNTXT::MULTIBUTTONINFO specialWpnHintBox;
};


struct GT_SPECIALABILITIES
{
	
	SPECIALABILITYINFO none, assault, tempest, probe, cloak, aegis, vampire, stasis, furyram, repel, bomber, mimic, minelayer, s_minelayer,
	repulsor, synthesis, massdisruptor, destabilizer, wormhole,ping,m_ping,s_ping, multiCloak, tractor, jump, artilery, tractorWave, nova, shieldJam,
	weaponJam, sensorJam, transfer;
};










































			    








struct SYSMAP_DATA
{
    bool drawSystemLines:1;
    bool drawThumbnail:1;
	bool drawGrid:1;
};

struct SECTOR_DATA
{
	S32 defaultSizeX,defaultSizeY;
	S32 currentSizeX,currentSizeY;
};
























			    














struct RGBA
{
	U8 r;
	U8 g;
	U8 b;
	U8 a;
};

struct FOG_DATA
{
	__readonly U32 version;


	RGBA hardFog;
	RGBA softFog;
	RGBA mapHardFog;
	RGBA mapSoftFog;
};






































			    












struct JUMPGATE_SAVELOAD
{
	U32 id,exit_gate_id;
	U32 ownerID;
	bool bLocked;
	SINGLE time_until_last_jump;

	
	MISSION_SAVELOAD mission;

	
	TRANS_SAVELOAD trans_SL;


	U8  exploredFlags;
	U8	marks;
	U8  visMarks;
	U8	shadowVisabilityFlags;
	bool bJumpAllowed;
	bool bInvisible;	
};




struct JUMPGATE_VIEW
{
	MISSION_SAVELOAD *	mission;
	struct BASIC_INSTANCE *	rtData;
	bool bJumpAllowed;
};


















			    
























struct SUP_SAVELOAD		
{
};


enum SUPPLYSHIP_MODES
{
	SUP_IDLE,
	SUP_RESUPPLY_ESCORT,
	SUP_MOVING_TARGETED_SUPPLY,
	SUP_CLIENT_IDLE,
	SUP_MOVING_TO_TENDER
};

enum SUPPLY_SHIP_STANCE
{
	SUP_STANCE_NONE,
	SUP_STANCE_RESUPPLY,
	SUP_STANCE_FULLYAUTO
};

struct BASE_SUPPLYSHIP_SAVELOAD	 
{
	SUPPLYSHIP_MODES mode;
	U32 workingID;
	U32 targetedTargetID;
	SINGLE supplyTimer;

	struct NETGRIDVECTOR supplyPoint;
	U32 supplyEscortTargetID;
	GRIDVECTOR seekTarget;

	U32 supplyPlatformTargetID;

	SUPPLY_SHIP_STANCE supplyStance;

	bool bNeedToSendMoveToTarget:1;
	bool bStoreAgentID:1;
};


struct SUPPLYSHIP_SAVELOAD : SPACESHIP_SAVELOAD		
{
	BASE_SUPPLYSHIP_SAVELOAD  baseSaveLoad;
	SUP_SAVELOAD sup_SL;
};





















			    






enum TSHIP_NET_COMMANDS
{
	TSHIP_CANCEL,
	TSHIP_APPROVED,
	TSHIP_SUCCESS
};


struct BASE_TROOPSHIP_SAVELOAD
{
	__hexview U32 dwTargetID;
	U32 attackAgentID;
	bool bTakeoverApproved;
};





struct TROOPSHIP_SAVELOAD : SPACESHIP_SAVELOAD
{
	BASE_TROOPSHIP_SAVELOAD	troopSaveLoad;
};


struct TROOPPOD_SAVELOAD
{
	__hexview U32 dwTargetID;
};














