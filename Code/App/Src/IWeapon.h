#ifndef IWEAPON_H
#define IWEAPON_H
//--------------------------------------------------------------------------//
//                                                                          //
//                                IWeapon.h                                 //
//                                                                          //
//                  COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/IWeapon.h 17    8/14/00 11:41a Tmauer $
*/			    
//------------------------------- #INCLUDES --------------------------------//
//--------------------------------------------------------------------------//
#ifndef IOBJECT_H
#include "IObject.h"
#endif

//--------------------------------------------------------------------------//
// flags for InitWeapon()
#define IWF_ALWAYS_HIT		0x00000001
#define IWF_ALWAYS_MISS		0x00000002		// illegal to use both flags at once

//--------------------------------------------------------------------------//
// max number of units that can be affected by an area of effect weapon
#define MAX_AOE_VICTIMS		16
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
struct _NO_VTABLE IWeapon : IObject 
{
	virtual void InitWeapon (IBaseObject * owner, const class TRANSFORM & orientation, IBaseObject * target, U32 flags=0, const class Vector * pos=0) = 0;
};
//--------------------------------------------------------------------------//
//
struct _NO_VTABLE IAOEWeapon : IObject 
{
	// "target" can be NULL on the client side
	// owner is the spaceship, not the launcher!!!
	virtual void InitWeapon (IBaseObject * owner, const class TRANSFORM & orientation, IBaseObject * target, U32 flags=0, const class Vector * pos=0) = 0;

	// weapon should determine who it will damage, and how much, then return the result to the caller
	virtual U32 GetAffectedUnits (U32 partIDs[MAX_AOE_VICTIMS], U32 damage[MAX_AOE_VICTIMS]) = 0;

	// caller has determined who it will damage, and how much.
	virtual void SetAffectedUnits (const U32 partIDs[MAX_AOE_VICTIMS], const U32 damage[MAX_AOE_VICTIMS]) = 0;
};
/*
//--------------------------------------------------------------------------//
//
struct _NO_VTABLE IArcWeapon : IAOEWeapon
{
	// will tell us how effective the target is for use with the arc cannon
	// returns a number between 0 and 1
	// 0 - no enemies will be hurt
	// 1 - arc cannon will hurt only enemies - no friendlies will be hit with the beam
	virtual SINGLE GetTargetEffectiveness (IBaseObject * target) = 0;  
};*/
//--------------------------------------------------------------------------//
//
struct _NO_VTABLE IWeaponTarget : IObject		// can be the target of a weapon
{
	// apply damage from the area
	virtual void ApplyAOEDamage (U32 ownerID, U32 damageAmount) = 0;
	
	// ownerID is the partID of the unit responsible for the damage. e.g. The carrier is the owner, even though
	// the actual shooter may be the fighter.
	// returns true if shield was up
	virtual BOOL32 ApplyDamage (IBaseObject * collider, U32 ownerID, const Vector & pos, const Vector &dir,U32 amount,BOOL32 shieldHit=1) = 0;

	virtual BOOL32 ApplyVisualDamage (IBaseObject * collider, U32 ownerID, const Vector & pos, const Vector &dir,U32 amount,BOOL32 shieldHit=1) = 0;

	//collide_point and dir are in world space just cause
	virtual BOOL32 GetCollisionPosition (Vector &collide_point,Vector &dir,const Vector &start,const Vector &direction) = 0;

	//see above
	virtual BOOL32 GetModelCollisionPosition (Vector &collide_point,Vector &dir,const Vector &start,const Vector &direction) = 0;

	virtual void AttachBlast(PARCHETYPE pBlast,const Vector &pos,const Vector &dir) = 0;

	virtual void AttachBlast(PARCHETYPE pBlast,const Transform & baseTrans) = 0;
};

//---------------------------------------------------------------------------
//-----------------------END IWeapon.h---------------------------------------
//---------------------------------------------------------------------------
#endif
