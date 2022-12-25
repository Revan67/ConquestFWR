#ifndef DSPACESHIP_H
#define DSPACESHIP_H
//--------------------------------------------------------------------------//
//                                                                          //
//                               DSpaceShip.h                               //
//                                                                          //
//                  COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*

    $Author: Rmarr $

    $Header: /Conquest/App/DInclude/DSpaceship.h 150   10/08/00 9:45p Rmarr $
*/			    
//--------------------------------------------------------------------------//

#ifndef DBASEDATA_H
#include "DBaseData.h"
#endif

#ifndef DTYPES_H
#include "DTypes.h"
#endif

#ifndef DINSTANCE_H
#include "DInstance.h"		// for SAVELOAD, animation
#endif

#ifndef DMTECHNODE_H
#include "DMTechNode.h"
#endif

#ifdef _ADB
#define mutable
#endif

#define MAX_ENGINE_GLOWS 6

//----------------------------------------------------------------
//
struct ROCKING_DATA : DYNAMICS_DATA
{
	SINGLE rockLinearMax, rockAngMax;			// max distance to rock (meters, radians)
};
//----------------------------------------------------------------
//
struct ENGINE_GLOW_DATA
{
	S32 size[MAX_ENGINE_GLOWS];
	U8 r,g,b;
	char engine_texture_name[GT_PATH];
};
//----------------------------------------------------------------
//
struct BLINKER_DATA
{
	char light_script[GT_PATH];
	char textureName[GT_PATH];
};
//----------------------------------------------------------------
//
struct SHIELD_DATA
{
	char meshName[GT_PATH];
	char animName[GT_PATH];
	char fizzAnimName[GT_PATH];
	SFX::ID sfx;
	SFX::ID fizzOut;
	SFX::ID fizzIn;
};
//----------------------------------------------------------------
//
struct DAMAGE_DATA
{
	char damageBlast[GT_PATH];
};
//----------------------------------------------------------------
//
struct CLOAK_DATA
{
	char cloakTex[GT_PATH];
	bool bAutoCloak:1;
	char cloakEffectType[GT_PATH];
};
//----------------------------------------------------------------
//
struct BILLBOARD_DATA
{
	char billboardTexName[GT_PATH];
	U32 billboardThreshhold;
	bool bTex2;
};

#define SLICES 10
//----------------------------------------------------------------
//
struct EXTENT_DATA
{
	RECT extents[SLICES];
	SINGLE _step;
	SINGLE _min;
	SINGLE min_slice,max_slice;
	BOOL32 bX;
};
//----------------------------------------------------------------
//
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
//----------------------------------------------------------------
//
struct BASE_SPACESHIP_DATA : BASIC_DATA
{
	SPACESHIPCLASS type;
    char fileName[GT_PATH];
	MISSION_DATA missionData;
    DYNAMICS_DATA dynamicsData;
	ROCKING_DATA rockingData;
	char explosionType[GT_PATH];
	char trailType[GT_PATH];
	char ambient_animation[GT_PATH];
	char ambientEffect[GT_PATH];
	ENGINE_GLOW_DATA engineGlow;
	BLINKER_DATA blinkers;
	SHIELD_DATA shield;
	DAMAGE_DATA damage;
	CLOAK_DATA cloak;
	BILLBOARD_DATA billboard;
	SINGLE_TECHNODE techActive;
#ifndef _ADB //we need to be able to use the FormationFilter as a bit mask
	union
	{
#endif
		FORMATION_FILTER formationFilter;
#ifndef _ADB
		DWORD formationFilterBits;
	};
#endif
	bool bLargeShip; // does this take up more than one grid square?
};

//----------------------------------------------------------------
//
#ifndef _ADB
struct IEffectHandle;

template <class BT_TYPE> 
struct SPACESHIP_INIT
{
	const BT_TYPE * pData;

	PARCHETYPE pArchetype;
	PARCHETYPE pExplosionType;
	PARCHETYPE pShieldHitType;
	PARCHETYPE pTrailType;
	struct AnimArchetype *damageAnimArch,*shieldAnimArch,*shieldFizzAnimArch;
	U32 engineTex,blinkTex,damageTexID,controlTexID;
	struct BlinkersArchetype *blink_arch;
	U32 cloakTex,cloakTex2;
	struct SMesh *smesh;
	struct BaseExtent *m_extent;
	PARCHETYPE pDamageBlast, pSparkBlast;
	PARCHETYPE pCloakEffect;
	Vector rigidBodyArm;
	EXTENT_DATA extent;

	S32 smoke_archID;
	S32 animArchetype;
	S32 archIndex;
	IMeshArchetype * meshArch;
	IEffectHandle * ambientEffect;
	SINGLE fp_radius;  //this sucks

	U32 hiliteTex;

	U32 billboardTex;

	//render experiment
	int numChildren;
	struct IMeshInfoTree *mesh_info;
	struct IMeshRender **mr;
	////////////////////

	void * operator new (size_t size)
	{
		return calloc(size,1);
	}
	void operator delete (void * ptr)
	{
		::free(ptr);
	}

	SPACESHIP_INIT (void)
	{
		meshArch = NULL;
		smoke_archID = 
		animArchetype = archIndex = -1;
		ambientEffect = NULL;
	}

	/* methods defined in TSpaceShip.h */

	bool loadSpaceshipArchetype (BT_TYPE * _pData, PARCHETYPE _pArchetype);		// load archetype data (return true on success)

	~SPACESHIP_INIT (void);					// free archetype references
};
#endif
//----------------------------------------------------------------
//
#define MAX_GUNBOAT_LAUNCHERS 5
struct BT_GUNBOAT_DATA : BASE_SPACESHIP_DATA
{
	SINGLE outerWeaponRange;
	SINGLE optimalFacingAngle;	// in radians
	bool bNoLineOfSight;		// true means we don't line of sight
	char launcherType[MAX_GUNBOAT_LAUNCHERS][GT_PATH];
};							
//----------------------------------------------------------------
//
#ifndef _ADB
#define GUNBOAT_INIT _GI
struct GUNBOAT_INIT : SPACESHIP_INIT<BT_GUNBOAT_DATA>
{
	PARCHETYPE launchers[MAX_GUNBOAT_LAUNCHERS];
};
#endif

#define MAX_TROOPSHIP_PODS 3
//----------------------------------------------------------------
//
struct BT_TROOPSHIP_DATA : BASE_SPACESHIP_DATA
{
	U32  damagePotential;
	SINGLE assaultRange;
	SFX::ID sfxPodRelease;
};							
//----------------------------------------------------------------
//
struct BT_TROOPPOD_DATA : BASIC_DATA
{
	EFFECTCLASS fxClass;
	char podType[GT_PATH];
	char podHardpoints[MAX_TROOPSHIP_PODS][HP_PATH];	// place where pods appear
};
//----------------------------------------------------------------
//
#ifndef _ADB
#define TROOPSHIP_INIT _GTI
struct TROOPSHIP_INIT : SPACESHIP_INIT<BT_TROOPSHIP_DATA>
{
	PARCHETYPE podType;
};
#endif
//----------------------------------------------------------------
//
struct BT_RECONPROBE_DATA : BASE_SPACESHIP_DATA
{
	SINGLE lifeTime;
	char blastType[GT_PATH];
};							
//----------------------------------------------------------------
//
#ifndef _ADB
struct RECONPROBE_INIT : SPACESHIP_INIT<BT_RECONPROBE_DATA>
{
	PARCHETYPE pBlastType;
};
#endif
//----------------------------------------------------------------
//
struct BT_TORPEDO_DATA : BASE_SPACESHIP_DATA
{
	SINGLE lifeTime;
	S32 damage;
	char blastType[GT_PATH];
};							
//----------------------------------------------------------------
//
#ifndef _ADB
struct TORPEDO_INIT : SPACESHIP_INIT<BT_TORPEDO_DATA>
{
	PARCHETYPE pBlastType;
	S32 textureID;
};
#endif
//----------------------------------------------------------------
//
struct BT_MINELAYER_DATA : BASE_SPACESHIP_DATA
{
	char drop_anim[GT_PATH];
	char mineFieldType[GT_PATH];
	char mineReleaseHardpoint[GT_PATH];
	U8 fieldSupplyCostPerMine;
	U32 fieldCompletionTime;
	U32 mineReleaseVelocity;
	SINGLE dropAnimDelay;
};
//----------------------------------------------------------------
//
#ifndef _ADB
struct MINELAYER_INIT : SPACESHIP_INIT<BT_MINELAYER_DATA>
{
	PARCHETYPE pMineFieldType;
};
#endif
//----------------------------------------------------------------
//
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
		char hardpointName[GT_PATH];
		SFX::ID nuggetSound;
	}nuggetTiming;

	char gasTankMesh[GT_PATH];
	char metalTankMesh[GT_PATH];
};

struct DYNAMICS_DATA_JR
{
	SINGLE maxLinearVelocity;
    SINGLE linearAcceleration;
    SINGLE maxAngVelocity;
};
//----------------------------------------------------------------
//
struct BT_BUILDERSHIP_DATA : BASIC_DATA
{
	SPACESHIPCLASS type;
    char fileName[GT_PATH];
	DYNAMICS_DATA_JR dynamicsData;
	char workAnimation[GT_PATH];
	char sparkAnim[GT_PATH];
	char sparkHardpoint[GT_PATH];
	char explosionType[GT_PATH];
	SINGLE sparkWidth;
	SINGLE sparkDelay;
};
//----------------------------------------------------------------
//
#ifndef _ADB
struct HARVEST_INIT : SPACESHIP_INIT<BT_HARVESTSHIP_DATA>
{
	S32 mineTex;
	U32 metalTankArch;
	U32 gasTankArch;
	struct RenderArch *gasTankRenderArch,*metalTankRenderArch;
	struct IMeshObj *gasTankMeshObj,*metalTankMeshObj;
	struct AnimArchetype *harvestAnimArch;
};
typedef SPACESHIP_INIT<BT_BUILDERSHIP_DATA> BUILDER_INIT;
#endif
//----------------------------------------------------------------
//----------------------------------------------------------------
//
#endif
