#ifndef OBJCLASS_H
#define OBJCLASS_H
//--------------------------------------------------------------------------//
//                                                                          //
//                               OBJCLASS.H                                 //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Author: Tmauer $

    $Header: /Conquest/App/DInclude/ObjClass.h 93    9/27/00 9:49p Tmauer $
*/			    
//-------------------------------------------------------------------


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

//------------------------------
// class flags
//------------------------------


#define CF_PROJECTILE	 0x00000080			// can be shot (e.g. missile)
#define CF_3DOBJECT		 0x0F101FBF			// has a 3D object representation (not launcher)
#define CF_TRANSPARENT   0x001001B6
#define CF_NAVHAZARD	 0x00410033			// navagation hazard, must avoid collision, things that show up on the terain map
#define CF_TARGETABLE    0x00002401
#define CF_SELECT1PASS   0x0d031601			// for selection, test on first pass
#define CF_SELECT2PASS	 0x00000032			// for selection, test other objects first
#define CF_SELECTABLE	 0x0d001401			// things that can have a selected state
#define CF_MISSION		 0x0d322613			// things that are mission objects
#define CF_BLOCKS_LOS	 0x00110022			// antimatter cloud + planets + black hole block line-of-sight
#define CF_PLAYERALIGNED 0x02200401			// things to drop in the editor that have a player alignment
#define CF_RENDER2ND	 0x00111030
#define CF_2FOOTPRINT	 0x00000002			// things that take up two terrain slots
#define CF_RENDEROVERFOG 0x10000200			// things that show up on top of everything
//------------------------------
// weapon type
//------------------------------

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

//------------------------------
// launcher type
//------------------------------

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

//------------------------------
// spaceship type
//------------------------------

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
//------------------------------
//
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
//------------------------------
// effect type
//------------------------------
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

//------------------------------
// field type
//------------------------------
enum FIELDCLASS
{
	FC_ASTEROIDFIELD=1,
	FC_MINEFIELD,
	FC_NEBULA,
	FC_ANTIMATTER
};

//------------------------------
// build object type
//------------------------------
enum BUILDOBJCLASS
{
	BO_TERRAN,
	BO_MANTIS,
	BO_SOLARIAN,
	BO_VYRIUM
};

//////////////////////////////////////////////////////////////////

typedef U16 M_PART_ID;

//////////////////////////////////////////////////////////////////


#endif
