#ifndef OBJCLASS_H
#define OBJCLASS_H
//--------------------------------------------------------------------------//
//                                                                          //
//                               OBJCLASS.H                                 //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Author: Tmauer $

    $Header: /Libs/Src/Tools/adb/ObjClass.h 1     11/12/02 4:27p Tmauer $
*/			    
//-------------------------------------------------------------------


enum OBJCLASS
{
	OC_SPACESHIP	=  0x00000001,
	OC_PLANETOID	=  0x00000002,
	OC_MEXPLODE		=  0x00000004,
	OC_DEBRIS		=  0x00000008,
	OC_JUMPGATE		=  0x00000010,
	OC_NEBULAE		=  0x00000020,
	OC_LAUNCHER		=  0x00000040,
	OC_WEAPON       =  0x00000080,
	OC_BLAST		=  0x00000100,
	OC_WAYPOINT		=  0x00000200,
	OC_PLATFORM		=  0x00000400,
	OC_FIGHTER		=  0x00000800,
	OC_MINE			=  0x00001000
};

//------------------------------
// class flags
//------------------------------


#define CF_PROJECTILE	0x00000080			// can be shot (e.g. missile)
#define CF_3DOBJECT		0x00000FBF			// has a 3D object representation (not launcher)
#define CF_TRANSPARENT  0x000001B6
#define CF_NAVHAZARD	0x00000413			// navagation hazard, must avoid collision
#define CF_TARGETABLE   0x00000401

//------------------------------
// weapon type
//------------------------------

enum WPNCLASS
{
	WPN_MISSILE=1,
	WPN_BOLT,
	WPN_BEAM,
	WPN_ARC,
	WPN_AUTOCANNON
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
	LC_FIGHTER_WING
};

//------------------------------
// spaceship type
//------------------------------

enum SPACESHIPCLASS
{
	SSC_DEFAULT=1,
	SSC_GUNBOAT,
	SSC_BUILDSHIP,
	SSC_HARVESTSHIP,
	SSC_FABRICATOR,
	SSC_HQ,
	SSC_MINELAYER
};

#endif
