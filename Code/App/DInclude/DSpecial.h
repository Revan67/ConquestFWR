#ifndef DSPECIAL_H
#define DSPECIAL_H
//--------------------------------------------------------------------------//
//                                                                          //
//                               DSpecial.h                                  //
//                                                                          //
//                  COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*

    $Author: Tmauer $

    $Header: /Conquest/App/DInclude/DSpecial.h 48    4/25/01 10:10a Tmauer $
*/			    
//--------------------------------------------------------------------------//
#ifndef DBASEDATA_H
#include "DBaseData.h"
#endif

#ifndef DTYPES_H
#include "DTypes.h"
#endif

#ifndef DINSTANCE_H
#include "DInstance.h"
#endif

#ifndef DWEAPON_H
#include "DWeapon.h"
#endif

#ifndef DMTECHNODE_H
#include "DMTechNode.h"
#endif

#ifndef DMBASEDATA_H
#include "DMBaseData.h"
#endif

#ifndef _ADB
#ifndef GRIDVECTOR_H
#include "..\src\GridVector.h"
#endif
#endif

//--------------------------------------------------------------------------//
struct BT_AEBOLT_DATA : BASE_WEAPON_DATA
{
	char fileName[GT_PATH];					// realspace object
	char explosionEffect[GT_PATH];
	SFX::ID launchSfx;
	SINGLE maxVelocity;	
	U32 damage;								// amount of points to deduct from opponent
	SINGLE explosionRange;							// max distance a child tendril can travel	
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
//--------------------------------------------------------------------------//
struct BT_PKBOLT_DATA : BASE_WEAPON_DATA
{
	char fileName[GT_PATH];					// realspace object
	char explosionEffect[GT_PATH];
	SFX::ID launchSfx;
	SINGLE maxVelocity;	
	char newPlanetType[GT_PATH];
	SINGLE changeTime;
	char engineTrailType[GT_PATH];
};


struct PKBOLT_SAVELOAD
{
	U32 ownerID;
	U32 targetID;
	U32 systemID;
	TRANS_SAVELOAD trans_SL;
};//--------------------------------------------------------------------------//
struct BT_STASISBOLT_DATA : BASE_WEAPON_DATA
{
	char fileName[GT_PATH];					// realspace object
	char explosionEffect[GT_PATH];
	SFX::ID launchSfx;
	SINGLE maxVelocity;	
	SINGLE explosionRange;							// max distance a child tendril can travel	
	SINGLE duration;
	MISSION_DATA missionData;
};

#define MAX_STASIS_TARGETS 26
#define MAX_STASIS_SQUARES 26

struct BASE_STASISBOLT_SAVELOAD
{
	SINGLE time;
	struct GRIDVECTOR targetPos;
	U32 ownerID;

	//terrain map stuff
	GRIDVECTOR gvec[MAX_STASIS_SQUARES];
	U32 numSquares;

	U32 numTargets;
	U32 targets[MAX_STASIS_TARGETS];
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
	// mission data
	MISSION_SAVELOAD mission;
};
//--------------------------------------------------------------------------//
struct BT_REPELLENTCLOUD_DATA : BASE_WEAPON_DATA
{
	char explosionEffect[GT_PATH];
	SFX::ID launchSfx;
	SINGLE duration;
	MISSION_DATA missionData;
	U32 damagePerSec;
	SINGLE centerRange;
};

#define MAX_REPEL_TARGETS 24
#define MAX_REPEL_SQUARES 24

struct BASE_REPELLENTCLOUD_SAVELOAD
{
	SINGLE time;
	SINGLE lastTime;
	struct GRIDVECTOR targetPos;
	U32 ownerID;

	//terrain map stuff
	GRIDVECTOR gvec[MAX_REPEL_SQUARES];
	U32 numSquares;

	U32 numTargets;
	U32 targets[MAX_REPEL_TARGETS];
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
	// mission data
	MISSION_SAVELOAD mission;
};
//--------------------------------------------------------------------------//
struct BT_REPULSORWAVE_DATA : BASE_WEAPON_DATA
{
	SFX::ID launchSfx;
	SINGLE duration;
	SINGLE range;
	SINGLE ringTime;
	SINGLE interRingTime;
	MISSION_DATA missionData;
};

#define MAX_REPULSORWAVE_TARGETS 24

struct BASE_REPULSORWAVE_SAVELOAD
{
	struct GRIDVECTOR targetPos;
	U32 ownerID;

	SINGLE time;

	U32 numTargets;
	U32 targets[MAX_REPULSORWAVE_TARGETS];
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
	// mission data
	MISSION_SAVELOAD mission;
};
//--------------------------------------------------------------------------//
struct BT_TRACTOR_DATA : BASE_WEAPON_DATA
{
	char fileName[GT_PATH];					// texture filename
	char hardpoint[HP_PATH];
	char contactBlastType[GT_PATH];
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
//--------------------------------------------------------------------------//
struct BT_REPULSOR_DATA : BASE_WEAPON_DATA
{
	char fileName[GT_PATH];					// texture filename
	char hardpoint[HP_PATH];
	char contactBlastType[GT_PATH];
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
//	Vector		start, direction;
	S32			time;
//	Vector pushPos;
	SINGLE mass;
};

//--------------------------------------------------------------------------//
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
//--------------------------------------------------------------------------//
struct BT_SWAPPER_DATA : BASE_WEAPON_DATA
{
	SFX::ID launchSfx;
};
//---------------------------------------------
//
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
//-------------------------------------------
//
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
//	TRANS_SAVELOAD trans_SL;
};


//-------------------------------------------
//
struct BT_ZEALOT_DATA : BASE_WEAPON_DATA
{
	SINGLE kamikazeSpeed;
	SINGLE damageAmount[3];
	char impactBlastType[GT_PATH];
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

//-------------------------------------------
//
struct BT_SYNTHESIS_DATA : BASE_WEAPON_DATA
{
	SINGLE_TECHNODE neededTech;
	char animName[GT_PATH];
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
//-------------------------------------------
//
struct BT_MASS_DISRUPTOR_DATA : BASE_WEAPON_DATA
{
	SFX::ID launchSfx;
	char fileName[GT_PATH];
	char contactBlastType[GT_PATH];
	char warpAnim[GT_PATH];
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
//--------------------------------------------------------------------------//
struct BT_DESTABILIZER_DATA : BASE_WEAPON_DATA
{
	char fileName[GT_PATH];					// realspace object
	char explosionEffect[GT_PATH];
	SFX::ID launchSfx;
	SINGLE maxVelocity;	
	SINGLE explosionRange;							// max distance a child tendril can travel	
	SINGLE duration;
	MISSION_DATA missionData;
};

#define MAX_DESTABILIZER_TARGETS 3
//#define MAX_STASIS_SQUARES 26

struct BASE_DESTABILIZER_SAVELOAD
{
	SINGLE time;
	struct GRIDVECTOR targetPos;
	U32 ownerID;

	U32 numTargets;
	U32 targetIDs[MAX_DESTABILIZER_TARGETS];
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
	// mission data
	MISSION_SAVELOAD mission;
};
//--------------------------------------------------------------------------//
struct BT_DUMBRECONPROBE_DATA : BASE_WEAPON_DATA
{
	char fileName[GT_PATH];					// realspace object
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
	// mission data
	MISSION_SAVELOAD mission;
};
//--------------------------------------------------------------------------//
struct BT_SPACEWAVE_DATA : BASE_WEAPON_DATA
{
};


#endif
