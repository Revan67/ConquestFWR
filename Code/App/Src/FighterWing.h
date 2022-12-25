#ifndef FIGHTERWING_H
#define FIGHTERWING_H
//--------------------------------------------------------------------------//
//                                                                          //
//                              FighterWing.h                               //
//                                                                          //
//                  COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/FighterWing.h 52    9/13/01 10:01a Tmauer $
*/			    
//------------------------------- #INCLUDES --------------------------------//
//--------------------------------------------------------------------------//

#ifndef TOBJECT_H
#include "TObject.h"
#endif

#ifndef DFIGHTERWING_H
#include <DFighterWing.h>
#endif

#ifndef _3DMATH_H
#include <3DMath.h>
#endif

#ifndef _IHARDPOINT_H_
#include <IHardPoint.h>
#endif

#ifndef ILAUNCHER_H
#include "ILauncher.h"
#endif

#ifndef DFIGHTER_H
#include <DFighter.h>
#endif

#ifndef IFIGHTER_H
#include "IFighter.h"
#endif

#ifndef MPART_H
#include "MPart.h"
#endif

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
struct FighterNode
{
	struct FighterNode *pNext, *pPrev;
	S32 index;

	IFighter * fighter;
};
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
struct _NO_VTABLE FighterWing : IBaseObject, ILauncher, ISaveLoad, IFighterOwner, FIGHTERWING_SAVELOAD
{
	BEGIN_MAP_INBOUND(FighterWing)
	_INTERFACE_ENTRY(IBaseObject)
	_INTERFACE_ENTRY(ISaveLoad)
	_INTERFACE_ENTRY(ILauncher)
	_INTERFACE_ENTRY(IFighterOwner)
	END_MAP()

	//----------------------------------
	// animation index
	//----------------------------------
	S32 animIndex;
	//----------------------------------
	// hardpoint data
	//----------------------------------
	HardpointInfo  hardpointinfo;
	INSTANCE_INDEX bayDoorIndex;
	mutable TRANSFORM	   hardpointTransform;
	//----------------------------------
	// Fighter data
	//----------------------------------
	PARCHETYPE pFighterType;
	const BT_FIGHTER_DATA * fighterData;
	//----------------------------------
	// Launcher constants
	//----------------------------------
	U32	   maxFighters;			// maximum number of fighters in group
	U32	   maxCapFighters;		// number of fighters in orbit
	U32	   maxWingFighters;		// number of fighters in a wing
	U32	   createPeriod;		// time to generate a new fighter
	U32	   minLaunchPeriod;		// time between launches of fighter wings
	U32	   bayDoorPeriod;		// time for door to open
	U32	   bayDoorClosePeriod;	// time to wait until closing the door
	U32	   costOfNewFighter;		
	U32	   costOfRefueling;		// fixed cost for refueling a fighter that used its weapon
	U32	   patrolRadius;
	SINGLE baseAirAccuracy;
	SINGLE baseGroundAccuracy;
	FighterNode * fighterPool;	// array of maxFighters


	//----------------------------------
	// dynamics fighter arrays
	//----------------------------------
	FighterNode * fighterQueue;	// fighters waiting below decks
	FighterNode * fighterPatrol;// fighters on patrol/attacking
	FighterNode * fighterDead;	// dead fighters

	//
	// target we are shooting at
	//
	OBJPTR<IBaseObject> target;
	OBJPTR<ILaunchOwner> owner;	// person who created us

	//----------------------------------
	//----------------------------------
	
	FighterWing (void);

	~FighterWing (void);	

	/* IBaseObject methods */

	virtual const TRANSFORM & GetTransform (void) const;

	virtual Vector GetVelocity (void);

	virtual U32 GetPartID (void) const;

	virtual U32 GetSystemID (void) const;

	virtual bool GetMissionData (MDATA & mdata) const;	// return true if message was handled

	virtual U32 GetPlayerID (void) const;

	virtual BOOL32 Update (void);	// returning FALSE causes destruction

	virtual void PhysicalUpdate (SINGLE dt);

	virtual void Render (void);

	virtual void RevealFog (const U32 currentSystem);
	
	virtual void CastVisibleArea (void);			

	virtual S32 GetObjectIndex (void) const;

	virtual U32 GetVisibilityFlags (void) const
	{
		return owner.Ptr()->GetVisibilityFlags();
	}
	/* ILauncher methods */

	virtual void InitLauncher (IBaseObject * owner, S32 ownerIndex, S32 animArcheIndex, SINGLE range);

	virtual void AttackPosition (const struct GRIDVECTOR * position, bool bSpecial);

	virtual void AttackObject (IBaseObject * obj);

	virtual void WormAttack (IBaseObject * obj)
	{};

	virtual void AttackMultiSystem (struct GRIDVECTOR * position, U32 targSystemID)
	{}

	virtual void DoCreateWormhole(U32 systemID)
	{}

	virtual const bool TestFightersRetracted (void) const;

	virtual void SetFighterStance(FighterStance stance);

	virtual void HandlePreTakeover (U32 newMissionID, U32 troopID);

	virtual void TakeoverSwitchID (U32 newMissionID)
	{
	}

	virtual void DoSpecialAbility (U32 specialID)
	{
	}

	virtual void DoSpecialAbility (IBaseObject *obj)
	{
	}

	virtual void DoCloak (void)
	{
	}

	virtual void SpecialAttackObject (IBaseObject * obj);

	virtual void  GetSpecialAbility (UNIT_SPECIAL_ABILITY & ability, bool & bEnabled)
	{
		MPart part(owner.Ptr());
		bEnabled = part->caps.specialAttackOk;
		if (bEnabled)
		{
			ability = USA_BOMBER;
		}
		else
		{
			ability = USA_NONE;
		}
	}

	virtual const U32 GetApproxDamagePerSecond (void) const
	{
		return 0;
	}

	virtual void InformOfCancel() {};

	virtual void LauncherOpCreated(U32 agentID, void * buffer, U32 bufferSize) {};

	virtual void LauncherReceiveOpData(U32 agentID, void * buffer, U32 bufferSize) {};

	virtual void LauncherOpCompleted(U32 agentID) {};

	virtual bool CanCloak(){return false;};

	virtual bool IsToggle() {return false;};

	virtual bool CanToggle(){return false;};

	virtual bool IsOn() {return false;};

	// the following methods are for network synchronization of realtime objects
	virtual U32 GetSyncDataSize (void) const
	{
		return 1;
	}

	virtual U32 GetSyncData (void * _buffer)
	{
		return 0;
	}

	virtual void PutSyncData (void * _buffer, U32 bufferSize)
	{
	}

	virtual void OnAllianceChange (U32 allyMask);

	virtual IBaseObject * FindChildTarget(U32 childID);

	/* ISaveLoad methods */

	virtual BOOL32 Save (struct IFileSystem * file);

	virtual BOOL32 Load (struct IFileSystem * file);
	
	virtual void ResolveAssociations (void);

	/* IFighterOwner methods */
	
	virtual void OnFighterDestruction (S32 fighterIndex);	 // fighter was hit by weapon

	virtual struct IFighter * GetFighter (S32 fighterIndex);

	virtual void GetLandingClearance (void);

	virtual void OnFighterLanding (S32 fighterIndex);

	virtual struct IBaseObject * FindFighter (U32 fighterMissionID);

	/* FighterWing methods */

//	static S32 findChild (const char * pathname, INSTANCE_INDEX parent);
//	static BOOL32 findHardpoint (const char * pathname, INSTANCE_INDEX & index, HardpointInfo & hardpointinfo, INSTANCE_INDEX parent);
	void calculateTransform (class TRANSFORM & result) const;
	virtual void init (PARCHETYPE pArchetype, PARCHETYPE _pFighterType);

	void setBayDoorState (BOOL32 bOpen, BOOL32 bAnimate);

	void createFighters (void);
	void destroyFighters (void);
	void resolveFighters (void);

	void moveToList (FighterNode * & list, U32 index);
	void removeFromLists (U32 index);
static void __fastcall updateFighterList (FighterNode * & list);
static void __fastcall physicalUpdateFighterList (FighterNode * & list, SINGLE dt);
static void __fastcall renderFighterList (FighterNode * list);
static void __fastcall revealFighterList (FighterNode * list, const U32 currentSystem);
static void __fastcall castVisibleFighterList (FighterNode * list);
static void __fastcall setFootprintFighterList (FighterNode *list);

static void __fastcall recallFighterList (FighterNode * list);
	virtual void updateFlightDeck (void);
	static U32 getNumFighters (FighterNode * list);
	virtual void assembleFlight (U32 numFighters);
	void updateTargetObject (void);	// tell all airborne fighters about new target
	void scanForTarget (void);	// parent checks horizon looking for enemy fighters
	bool checkRange (void);		// return TRUE if we are within range of target to launch fighters
};

//---------------------------------------------------------------------------------------------
//-------------------------------End FighterWing.h---------------------------------------------
//---------------------------------------------------------------------------------------------
#endif