#ifndef DWEAPON_H
#define DWEAPON_H
//--------------------------------------------------------------------------//
//                                                                          //
//                               DWeapon.h                                  //
//                                                                          //
//                  COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*

    $Author: Rmarr $

    $Header: /Conquest/App/DInclude/DWeapon.h 40    7/26/00 10:25a Rmarr $
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

struct BASE_WEAPON_DATA : BASIC_DATA
{
	WPNCLASS wpnClass;
};
//--------------------------------------------------------------------------//

struct BT_PROJECTILE_DATA : BASE_WEAPON_DATA
{
	char fileName[GT_PATH];					// realspace object
	SFX::ID launchSfx;
	U32 damage;								// amount of points to deduct from opponent
	SINGLE maxVelocity;						// traveling speed of the projectile
	char blastType[GT_PATH];				// e.g. the shield hit effect
//	char sparkType[GT_PATH];				// effect will appear at the barrel of gun
	char engineTrailType[GT_PATH];			// type of engine trail that the projectile will have
};

struct BT_PLASMABOLT_DATA : BT_PROJECTILE_DATA
{
	char textureName[GT_PATH];				//name of the texture for the ball
	char animName[GT_PATH];					//how will I find a dress to wear to the ball?
	SINGLE numBolts;
	struct _pBolt
	{
		Vector rollSpeed;						//speed the ball rolls at on the Z axis
		SINGLE boltWidth;						//width of the ball, determines size of billboard
		SINGLE boltSpacing;						//distance between, billboards
		U32 segmentsX;							//number of segments in length X
		U32 segmentsY;							//number of segments in length Y
		U32 segmentsZ;							//number of segments in length Z
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
	char fileName[GT_PATH];					// realspace object
	SFX::ID launchSfx;
	U32 damage;								// amount of points to deduct from opponent
	SINGLE maxVelocity;						// traveling speed of the projectile
	char blastType[GT_PATH];				// e.g. the shield hit effect
	char sparkType[GT_PATH];				// effect will appear at the barrel of gun
	FLASH_DATA flash;
	SINGLE MASS;
	SPECIAL_DAMAGE special;
};

//--------------------------------------------------------------------------//
struct BT_BEAM_DATA : BASE_WEAPON_DATA
{
	char fileName[GT_PATH];					// texture filename
	SFX::ID launchSfx;
	U32 damage;								// amount of points to deduct from opponent
	SINGLE lifetime;						// how long the beam will stay arround
	SINGLE maxSweepDist;					//how far the beam will sweep arround
	SINGLE beamWidth;
	struct _colorStruct
	{
		U8 red,green,blue;
	} blurColor;
	char contactBlast[GT_PATH];
};

//--------------------------------------------------------------------------//
//
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
//--------------------------------------------------------------------------//
struct BT_GATTLINGBEAM_DATA : BASE_WEAPON_DATA
{
	char fileName[GT_PATH];					// texture filename
	SFX::ID launchSfx;
	U32 damage;								// amount of points to deduct from opponent
	SINGLE lifetime;						// how long the beam will stay arround
	SINGLE maxSweepDist;					//how far the beam will sweep arround
	SINGLE beamWidth;
	char contactBlast[GT_PATH];
};

//--------------------------------------------------------------------------//
//
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

//--------------------------------------------------------------------------//
struct BT_LASERSPRAY_DATA : BASE_WEAPON_DATA
{
	char fileName[GT_PATH];					// texture filename
	SFX::ID launchSfx;
	U32 damage;								// amount of points to deduct from opponent
	SINGLE lifetime;						// how long the beam will stay arround
	SINGLE maxSweepDist;					//how far the beam will sweep arround
	SINGLE beamWidth;
	SINGLE velocity;
	char contactBlast[GT_PATH];
};

//--------------------------------------------------------------------------//
//
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

//--------------------------------------------------------------------------//
struct BT_ANMBOLT_DATA : BT_PROJECTILE_DATA
{
	char animFile[GT_PATH];					// anim filename
	SINGLE boltSize;
};

//--------------------------------------------------------------------------//
//
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

//--------------------------------------------------------------------------//
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
//--------------------------------------------------------------------------//

struct BASE_MISSILE_SAVELOAD
{
	U32			targetID, ownerID;
	U32			systemID;
	Vector		initialPos;
	SINGLE		timeToLive;
	SINGLE		wobble;		// in radians
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
	char fileName[GT_PATH];					// texture filename
	SFX::ID launchSfx;
	U32 damage;
	SINGLE jumpRange;							// max distance a child tendril can travel
	SINGLE feedbackRange;						// distance at which feedback can occur (0 == not at all)
	U32 maxTendrilStages;						// how many times can the spark jump
	SINGLE damageDrop;							// percentage of decrease in damage after each jump
	U32 maxTargets;								// total number of ships who can be hit
	SINGLE period;								// length of time arc stays on
	bool drainSupplies:1;							// drain supplies instead of hull points
};


struct ARC_SAVELOAD
{
	SINGLE time;
	S32 numTendrils;
	U32 ownerID;
	U32 systemID;
	TRANS_SAVELOAD trans_SL;
};


#endif
