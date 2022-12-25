#ifndef IOBJECT_H
#define IOBJECT_H
//--------------------------------------------------------------------------//
//                                                                          //
//                                 IObject.h                                //
//                                                                          //
//                  COPYRIGHT (C) 1997 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*

    $Author: Tmauer $

    $Header: /Conquest/App/Src/IObject.h 73    10/20/00 3:27p Tmauer $
*/			    
//--------------------------------------------------------------------------//

#ifndef US_TYPEDEFS
#include <Typedefs.h>
#endif

#ifndef OBJCLASS_H
#include "ObjClass.h"
#endif

#ifndef SUPERTRANS_H
#include "SuperTrans.h"
#endif

#ifndef MGLOBALS_H
#include "MGlobals.h"
#endif

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//

#ifndef _NO_VTABLE	
  #if (_MSC_VER < 1100)      // 1100 = VC++ 5.0
    #define _NO_VTABLE
  #else
    #define _NO_VTABLE __declspec(novtable)
  #endif
#endif

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
class Vector;
class TRANSFORM;
struct USER_DEFAULTS;


typedef float OBJBOX[6];	// maxx,minx,maxy,miny,maxz,minz  in object coordinates

struct _NO_VTABLE IObject
{
	virtual void * __fastcall QueryInterface (OBJID objid, OBJPTR<IBaseObject> & pInterface, U32 playerID=TOTALLYVOLATILEPTR) = 0;
};

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
struct FIELDFLAGS
{
	bool bAsteroidsLight:1;
	bool bAsteroids:1;	
	bool bAsteroidsHeavy:1;
	bool bIon:1;
	bool bAntiMatter:1;
	bool bHelious:1;
	bool bLithium:1;
	bool bHades:1;
	bool bCelsius:1;
	bool bCygnus:1;
	U8 damagePerTwentySeconds;

	bool hasBlast (void) const
	{
		return bHelious;
	}

	bool inhibitsSpeed (void) const
	{
		return bAsteroidsLight|bAsteroids|bAsteroidsHeavy|bLithium|bCygnus;
	}
	SINGLE getSpeedModifier (void) const
	{
		if(bCygnus)
			return 2.0;
		else if(bAsteroidsLight)
			return 0.75;
		else if(bAsteroids|bLithium)
			return 0.5;
		else if(bAsteroidsHeavy)
			return 0.25;
		return 1.0;
	}

	bool sensorDamping (void) const
	{
		return (bIon);
	}
	
	SINGLE getSensorDampingMod (void) const
	{
		if(bIon)
			return 0.5;
		else 
			return 1.0;
	}

	bool fieldCloak (void) const
	{
		return false;
	}

	bool shieldsInoperable (void) const
	{
		return bIon;
	}

	SINGLE getDamageModifier (void) const
	{
		if(bHelious)
			return 2.0;
		return 1.0;
	}

	bool shieldEnhance (void) const
	{
		return false;
	}

	bool suppliesLocked (void) const
	{
		return bCelsius;
	}

	SINGLE targetingBonus (void) const
	{
		return 0.0;
	}

	void zero (void)
	{
		memset(this, 0, sizeof(*this));
	}
};

struct EFFECT_FLAGS
{
	bool bStasis:1;
	bool bRepellent:1;
	bool bDestabilizer:1;
	bool bTalorianShield:1;
	bool bAgeisShield:1;
	bool bRepulsorWave:1;
	bool bSinuator:1;
	bool bTractorWave:1;
	bool bIndusrialBoost1:1;
	bool bIndusrialBoost2:1;
	bool bIndusrialBoost3:1;
	bool bIndusrialBoost4:1;
	bool bResearchBoost1:1;
	bool bResearchBoost2:1;
	bool bResearchBoost3:1;
	bool bResearchBoost4:1;
	bool bShieldJam:1;
	bool bWeaponJam:1;
	bool bSensorJam:1;
	bool bInihibitorBreak:1;

	bool canMove()
	{
		return (!bStasis && !bDestabilizer);
	}

	bool canShoot()
	{
		return (!bStasis && !bDestabilizer);
	}

	bool canBeHurt()
	{
		return ((!bAgeisShield) && (!bStasis));
	}

	SINGLE getSensorDampingMod (void) const
	{
		if(bRepellent || bSensorJam)
			return 0.1;
		else 
			return 1.0;
	}

	SINGLE getSpeedModifier (void) const
	{
		if(bRepellent)
			return 0.25;
		else if(bSinuator)
			return 2.0;
		else 
			return 1.0;
	}

	SINGLE getDamageModifier (void) const//damage taken
	{
		if(bTalorianShield)
			return 0.25;
		else
			return 1.0;
	}

	SINGLE getWeaponDamageModifier (void) const//dmaage dealt
	{
		if(bWeaponJam)
			return 0.5;
		else
			return 1.0;
	}

	bool getShieldsInoperable (void) const
	{
		return bShieldJam;
	}

	SINGLE getIndustrialBoost (void) const
	{
		SINGLE boost = 1.0;
		if(bIndusrialBoost1)
			boost += 0.50;
		if(bIndusrialBoost2)
			boost += 0.50;
		if(bIndusrialBoost3)
			boost += 0.50;
		if(bIndusrialBoost4)
			boost += 0.50;
		return boost;
	}

	SINGLE getResearchBoost (void) const
	{
		SINGLE boost = 1.0;
		if(bResearchBoost1)
			boost += 0.50;
		if(bResearchBoost2)
			boost += 0.50;
		if(bResearchBoost3)
			boost += 0.50;
		if(bResearchBoost4)
			boost += 0.50;
		return boost;
	}

	bool canIgnoreInhibitors (void) const
	{
		return bInihibitorBreak;
	}

	void zero (void)
	{
		memset(this, 0, sizeof(*this));
	}
};

struct _NO_VTABLE IBaseObject : public IObject
{
	struct MDATA
	{
		const struct MISSION_SAVELOAD * pSaveData;
		const struct MISSION_DATA * pInitData;
	};
	
	//assuming these next 3 should be on the same cache line whenever possible
	//cache aligned would be best
	OBJCLASS objClass;			// See ObjClass.h
	struct IBaseObject *next;
	struct IBaseObject *hashNext;

	struct IBaseObject *prev,*hashPrev;
	struct ObjMapNode *objMapNode;

	//template<> class OBJPTR<IBaseObject> * objwatch;//this could be made faster by pre sorting by playerID into buckets, more memory though
	OBJPTR<IBaseObject> * objwatch;//this could be made faster by pre sorting by playerID into buckets, more memory though

	BOOL32 bHighlight:1;
	BOOL32 bVisible:1;
	BOOL32 bSelected:1;
	BOOL32 bCloaked:1;
	BOOL32 bSpecialRender:1;
	BOOL32 bMimic:1;
	BOOL32 bSpecialHighlight:1;
	BOOL32 bExploding:1;
	U32	   updateBin:5;				// AI update, visibility checking

private:
	U32	   visibilityFlags:8;		// bitfield
	U32	   pendingVisibilityFlags:8;		// bitfield
public:

	struct IBaseObject *nextTarget, *prevTarget;
	struct IBaseObject *nextSelected, *prevSelected;
	struct IBaseObject *nextHighlighted, *prevHighlighted;

	PARCHETYPE pArchetype;
	FIELDFLAGS fieldFlags;		// Set by Field manager
	EFFECT_FLAGS effectFlags;	// Set and unset by the cause of the effect

	/* IBaseObject methods */

	IBaseObject (void)
	{
		objwatch = NULL;
	}

	virtual ~IBaseObject (void);	// See ObjList.cpp

	//
	// Engine abstraction
	//

	virtual const TRANSFORM & GetTransform (void) const;

	inline Vector GetPosition (void) const
	{
		return GetTransform().get_position();
	}

	virtual Vector GetVelocity (void);

	//
	// game specific
	//
	
	virtual struct GRIDVECTOR GetGridPosition (void) const;

	virtual U32 GetSystemID (void) const;			// current system
	
	virtual void TestVisible (const USER_DEFAULTS & defaults, const U32 currentSystem, const U32 currentPlayer);	// set bVisible if possible for any part of object to appear

	virtual SINGLE TestHighlight (const RECT & rect);	// set bHighlight if possible for any part of object to appear within rect,
														// return the distance to center of the object
	virtual void RevealFog (const U32 currentSystem);	// call the FogOfWar manager, if appropriate

	virtual void SetTerrainFootprint (struct ITerrainMap * terrainMap);	

	virtual void CastVisibleArea (void);				// set visible flags of objects nearby

	// realtime AI updating at a constant rate per seconds. Returning FALSE causes destruction
	virtual BOOL32 Update (void);	

	// updates position, rotation of root object. "dt" is in seconds
	virtual void PhysicalUpdate (SINGLE dt);

	virtual void DrawSelected (void);

	virtual void DrawHighlighted (void);		// called during 2D render

	virtual void DrawFleetMoniker (bool bAllShips);	// called during 2D render, if bAllShips is false, only draw if allied with player

	virtual void Render (void);

	virtual void MapRender (bool bPing);

	virtual void MapTerrainRender (void);

	virtual void View (void);

	virtual void DEBUG_print (void) const;	// screen has been locked, OK to do VFX stuff (for debugging)

	virtual bool GetObjectBox (OBJBOX & box) const;  // return false if not supported

	virtual U32 GetPartID (void) const;
	
	virtual bool GetMissionData (MDATA & mdata) const;	// return true if message was handled

	virtual bool MatchesSomeFilter(DWORD filter);

	virtual S32 GetObjectIndex (void) const;		// return engine index, if any

	virtual U32 GetPlayerID (void) const;

	virtual void SetReady(bool _bReady);

	virtual bool IsTargetableByPlayer(U32 _playerID) const
	{
		return true;
	}

	// game visibility
	bool IsVisibleToPlayer (U32 playerID) const
	{
		return (playerID==0 || (visibilityFlags & (1 << (playerID-1))) != 0);
	}

#ifdef MGLOBALS_H
	void SetVisibleToPlayer (U32 playerID)
	{
		SetVisibleToAllies(MGlobals::GetAllyMask(playerID));
	}
#endif

	void SetVisibleToAllies (U32 teamFlags)
	{
		pendingVisibilityFlags |= teamFlags;
	}

	virtual void UpdateVisibilityFlags (void)
	{
		visibilityFlags = pendingVisibilityFlags;
		pendingVisibilityFlags = 0;
	}

	virtual void UpdateVisibilityFlags2 (void)
	{
		visibilityFlags |= pendingVisibilityFlags;
		pendingVisibilityFlags = 0;
	}

	bool IsPendingVisibilityToPlayer (U32 playerID) const		// visibility has been set this update frame
	{
		return (playerID==0 || (pendingVisibilityFlags & (1 << (playerID-1))) != 0);
	}

	U32 GetVisibilityFlags (void) const
	{
		return visibilityFlags;
	}

	U32 GetPendingVisibilityFlags (void) const
	{
		return pendingVisibilityFlags;
	}

	virtual U32 GetTrueVisibilityFlags(void) const
	{
		return visibilityFlags;
	}

	void ClearVisibilityFlags (void)
	{
		visibilityFlags = pendingVisibilityFlags = 0;		// no one can see me!  (docked flagships)
	}

	void SetJustVisibilityFlags (U32 flags)
	{
		visibilityFlags = flags;		
	}

	void SetVisibilityFlags (U32 flags)
	{
		visibilityFlags = pendingVisibilityFlags = flags;		
	}


	void drawRangeCircle(SINGLE range,COLORREF color);
	void drawPartialRangeCircle(SINGLE range,SINGLE spin, COLORREF color);
	void drawCircle(Vector center, SINGLE range,COLORREF color);
	void drawLine(Vector end1, Vector end2,COLORREF color);

protected:

#ifndef FINAL_RELEASE
private:
	const char *debugName;
public:
	void SetDebugName (const char *name)
	{	
		debugName = name;
	}
	const char * GetDebugName (void) const
	{
		return debugName;
	}

#endif	// end !FINAL_RELEASE
};

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
struct _NO_VTABLE IPhysicalObject : IObject
{
	virtual void SetSystemID (U32 newSystemID) = 0;

	virtual void SetPosition (const Vector & position, U32 newSystemID) = 0;

	virtual void SetTransform (const TRANSFORM & transform, U32 newSystemID) = 0;

	virtual void SetVelocity (const Vector & velocity) = 0;

	virtual void SetAngVelocity (const Vector & angVelocity) = 0;
};

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//

struct _NO_VTABLE ISaveLoad : IObject 
{
	virtual BOOL32 Save (struct IFileSystem * file) = 0;

	virtual BOOL32 Load (struct IFileSystem * file) = 0;

	virtual void ResolveAssociations (void) = 0;
};

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//

struct _NO_VTABLE IQuickSaveLoad : IObject
{
	virtual void QuickSave (struct IFileSystem * file) = 0;

	virtual void QuickLoad (const char *szSaveLoadType, const char *szInstanceName, void * buffer, U32 bufferSize) = 0;

	virtual void QuickResolveAssociations (void) = 0;
};

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//

struct _NO_VTABLE IPlayerBomb : IObject
{
	virtual void ExplodePlayerBomb (void) = 0;
};

//----------------------------------------------------------------------------------
//---------------------------END IObject.h------------------------------------------
//----------------------------------------------------------------------------------
#endif