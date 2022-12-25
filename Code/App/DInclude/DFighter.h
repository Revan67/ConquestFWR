#ifndef DFIGHTER_H
#define DFIGHTER_H
//--------------------------------------------------------------------------//
//                                                                          //
//                                DFighter.h                                //
//                                                                          //
//                  COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/DInclude/DFighter.h 17    9/26/00 12:21p Tmauer $
*/			    
//--------------------------------------------------------------------------//

#ifndef DBASEDATA_H
#include "DBaseData.h"
#endif

#ifndef DTYPES_H
#include "DTypes.h"
#endif

//----------------------------------------------------------------
//
struct BT_FIGHTER_DATA : BASIC_DATA
{
    char fileName[GT_PATH];
    DYNAMICS_DATA dynamicsData;
	ARMOR_DATA  armorData;
	char explosionType[GT_PATH];
	char weaponType[GT_PATH];
	char engineTrailType[GT_PATH];

	U16		 sensorRadius;			// for fog of war
	U16		 cloakedSensorRadius;	// unhiding cloaked objects
	SINGLE   patrolRadius;
	SINGLE   patrolAltitude;
	SINGLE	 attackRange;
	SINGLE	 dodge;
	U32		 hullPointsMax;			//number of hits before explosion
	U32	     maxSupplies;			// amount of supplies given to new unit
	SINGLE 	 refirePeriod;			// time between shots
	SINGLE	 patrolPeriod;			// time flying around aimlessly
	U32		 kamikazeDamage;		// Damage when on suicide run
	SFX::ID	 kamikazeYell;			// when diving into enemy ship
	SFX::ID	 fighterwhoosh;			// When fighters fly by Capital ships
	SFX::ID	 fighterLaunch;			// when fighters launch from their carrier
};
//----------------------------------------------------------------
//
enum FighterState
{
	ONDECK,
	PATROLLING,
	DEAD
};
//----------------------------------------------------------------
//
enum FormationType
{
	FT_HUMAN_PATROL,
	FT_MANTIS_PATROL,
	FT_HUMAN_ATTACK,
	FT_MANTIS_ATTACK
};
//----------------------------------------------------------------
//
struct BASE_FIGHTER_SAVELOAD
{
	// fighter data
	U32 ownerID, dwMissionID;
	U32 systemID:8;
	U32 playerID:8;			// which player am I? (keep track of this for air defense)
	S32 supplies:16;
	U8  formationIndex;		// 0 - MAX_FIGHTERS_IN_WING-1
	S8  parentIndex;		// from fighter pool of parent, -1 is invalid
	S8  childIndex;			// from fighter pool of parent, -1 is invalid
	S8  myIndex;			// from fighter pool of parent, -1 is invalid
	S32 refireTime:16;
	S32 patrolTime:16;
	S32 generalCounter;
	Vector kamikaziTarget;	// in target's object space
	FighterState   state:8;
	FormationType  formationType:8;
	M_RACE race:8;
	U8  patrolState;		// LAUNCHING, CIRCLING, RETURNING, ENTERING
	U8   formationState;		// LOCKED, BREAKING, LOOSE, FORMING
	U8   kamikazeTimer;		// time to wait until self-destruct (when target and owner are dead)
	U8   bCirclePatrol:1;	// if true, patrol in a circle
	bool bKamikaziTargetSelected:1;		// if true, we have chose place to crash into target
	bool bKamikaziYellComplete:1;	// true after we have yelled appropriately
	bool bKamikaziComplete:1;		// true when hit the target
};

struct FIGHTER_SAVELOAD : BASE_FIGHTER_SAVELOAD
{
	TRANS_SAVELOAD   trans_SL;
};

#ifndef _ADB
struct BASE_FIGHTER_INIT
{
	const BT_FIGHTER_DATA * pData;
	PARCHETYPE pArchetype;
	S32 archIndex;
	IMeshArchetype * meshArch;
	PARCHETYPE pWeaponType, pExplosionType, pEngineTrailType;
	Vector rigidBodyArm;
};
#endif

#endif
