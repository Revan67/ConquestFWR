#ifndef DBASEDATA_H
#define DBASEDATA_H
//--------------------------------------------------------------------------//
//                                                                          //
//                               DBaseData.h                                //
//                                                                          //
//                  COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Author: Rmarr $

    $Header: /Conquest/App/DInclude/DBaseData.h 102   9/27/00 10:09p Rmarr $
*/			    
//--------------------------------------------------------------------------//
#ifndef US_TYPEDEFS
#include "typedefs.h"
#endif

#ifndef OBJCLASS_H
#include "objclass.h"
#endif

#ifndef SFXID_H
#include "sfxid.h"
#endif

#ifndef _ADB
#ifndef SUPERTRANS_H
#include "SuperTrans.h"
#endif
#endif

#ifndef DTYPES_H
#include "DTypes.h"
#endif

#ifndef GT_PATH
#define GT_PATH 32
#endif

#ifndef HP_PATH				// size of hardpoint path
#define HP_PATH 64
#endif

#include "..\Src\Resource.h"

#ifndef DOBJNAMES_H
#include "DObjNames.h"
#endif

#ifndef DSILHOUETTE_H
#include "DSilhouette.h"
#endif

#ifndef DUNITSOUNDS_H
#include "DUnitSounds.h"
#endif

#ifndef DMISSIONENUM_H
#include "DMissionEnum.h"
#endif

#ifdef _ADB 
typedef unsigned __int64 U64;
#else
#define __hexview
#define __readonly
#endif

struct DYNAMICS_DATA
{
    SINGLE linearAcceleration;
    SINGLE angAcceleration;

    SINGLE maxLinearVelocity;
    SINGLE maxAngVelocity;				// used for yaw
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
//	bool bDrawCloaking:1;  //mimic will not draw cloaking
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

#ifdef _ADB
	union FIELDS			// transmorgrify function doesn't like nameless unions
#else
	union 
#endif
	{
		struct ARMOR_DAMAGE
		{
			SINGLE noArmor;
			SINGLE lightArmor;
			SINGLE mediumArmor;
			SINGLE heavyArmor;
		} damageTable;

		SINGLE _damageTable[4];
#ifdef _ADB
	} fields;

#else
	};
#endif
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

#define DAMAGE_RECORDS 5
struct DAMAGE_SAVELOAD
{
	U32 lastRepairSlot;
	SINGLE lastDamage;
	struct DamageSave damageSave[DAMAGE_RECORDS];
};
//----------------------------------------------------------------
//
struct BASIC_DATA		// every game type must inherit from this
{
    OBJCLASS objClass;
	bool bEditDropable;		// include in dropdown list of objects for edit mode
};
//----------------------------------------------------------------
//
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

struct GENBASE_DATA		// every non-game type must inherit from this
{
	GENBASE_TYPE type;
};

//----------------------------------------------------------------
//
struct GT_VFXSHAPE : GENBASE_DATA
{
	char filename[GT_PATH];
	bool bHiRes;
};
//----------------------------------------------------------------
//



//----------------------------------------------------------------
//---------------------Mission definitions------------------------
//----------------------------------------------------------------
//
// Binary layout of MISSION_DATA as stored in gametypes.db (VS6-compiled, 40 bytes).
// armorData/silhouetteImage/specialAbility/speechPriority were added to the source
// after the binary data files were compiled and are NOT present in the binary.
// Use MISSION_DATA_BIN for archetype binary structs; use MISSION_DATA for runtime objects.
struct MISSION_DATA_BIN
{
	// VS6 packed these three differently-typed enum bitfields into one int
	// (9+4+17 = 30 bits ≤ 32) because it treated all enum bitfields as
	// int-typed.  VS2022 gives each different type its own storage unit →
	// 3×4 = 12 bytes instead of 4.  Use unsigned int to force packing and
	// match the VS6 binary layout.
	unsigned int mObjClass:9;	// was M_OBJCLASS
	unsigned int race:4;		// was M_RACE
	unsigned int displayName:17;	// was OBJNAMES::M_DISPLAY_NAME

	struct M_CAPS
	{
		U8 padding;
		// VS6 bool:1 uses byte-sized storage (1-byte per group), packing 8 flags
		// per byte.  24 flags × bool:1 = 3 bytes + 1 byte padding = 4 bytes total.
		// VS2022 bool:1 behaves identically, so no type change is needed.
		bool moveOk:1;
		bool attackOk:1;
		bool specialAttackOk:1;
		bool specialEOAOk:1;
		bool specialAbilityOk:1;
		bool specialAttackWormOk:1;
		bool defendOk:1;	// an area or a friendly unit
		bool supplyOk:1;
		bool harvestOk:1;
		bool buildOk:1;
		bool repairOk:1;
		bool jumpOk:1;
		bool admiralOk:1;	// ship contains admiral
		bool captureOk:1;	// capture enemy units
		bool salvageOk:1;
		bool probeOk:1;
		bool mimicOk:1;
		bool recoverOk:1;
		bool createWormholeOk:1;
		bool synthesisOk:1;
		bool cloakOk:1;
		bool specialAttackShipOk:1;  //repulsor and tractor
		bool targetPositionOk:1;
		bool specialTargetPlanetOk:1;

#ifndef _ADB
		M_CAPS & operator |= (const M_CAPS & other);
		M_CAPS & operator &= (const M_CAPS & other);
#endif

	} caps;

	U16			hullPointsMax;
	U16			supplyPointsMax;
	U16			scrapValue;
	U16			buildTime;

	ResourceCost	resourceCost;

	SINGLE		sensorRadius;
	SINGLE		cloakedSensorRadius;
	SINGLE		maxVelocity;
	SINGLE		baseWeaponAccuracy;
	SINGLE		baseShieldLevel;
};

#ifndef _ADB
static_assert(sizeof(MISSION_DATA_BIN) == 40, "MISSION_DATA_BIN must be 40 bytes to match VS6 binary");
#endif

// Runtime MISSION_DATA extends the binary portion with fields that were added
// to the source after gametypes.db was compiled.  These are NOT read from the
// binary; they are zero-initialised and then populated via MISSION_DATA_OVERRIDE
// scripts at mission load time.
struct MISSION_DATA : MISSION_DATA_BIN
{
	ARMOR_DATA  armorData;
	SILNAMES::M_SILHOUETTE_NAME silhouetteImage;
	UNIT_SPECIAL_ABILITY    specialAbility;
	UNITSOUNDS::PRIORITY speechPriority;

#ifndef _ADB
	// specialAbility1/2 were added post-VS6 and are NOT in the binary archetype
	// files.  Static const so existing call sites compile without changing layout.
	static const UNIT_SPECIAL_ABILITY specialAbility1 = USA_NONE;
	static const UNIT_SPECIAL_ABILITY specialAbility2 = USA_NONE;
#endif
};

#ifndef _ADB
static_assert(sizeof(MISSION_DATA) == 72, "MISSION_DATA runtime size must be 72 bytes");
#endif

//----------------------------------------------------------------
//---------------------Mission data overrides---------------------
//----------------------------------------------------------------
//
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

#endif
