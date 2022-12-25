#ifndef IFABRICATOR_H
#define IFABRICATOR_H
//--------------------------------------------------------------------------//
//                                                                          //
//                             IFabricator.h                                //
//                                                                          //
//                  COPYRIGHT (C) 1997 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*

    $Author: Jasony $

    $Header: /Conquest/App/Src/ifabricator.h 65    10/19/00 6:15p Jasony $
*/			    
//--------------------------------------------------------------------------//

#ifndef GRIDVECTOR_H
#include "GridVector.h"
#endif

#ifndef DMISSIONENUM_H
#include "DMissionEnum.h"
#endif

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
struct IBuild;

struct _NO_VTABLE IFabricator : IObject
{
	// slotID ( 1..MAX_SLOTS, or 0=no preference) is the suggested slot
	virtual void ParkYourself (const TRANSFORM & _trans, U32 planetID, U32 slotID) = 0;

	// slotID ( 1..MAX_SLOTS, or 0=no preference) is the suggested slot
	// offset is used when multiple fabricators are sent to the same slot. Null==no offset
	virtual void BeginConstruction (U32 planetID, U32 slotID, U32 dwArchetypeDataID, U32 agentID) = 0;

	virtual void BeginConstruction (GRIDVECTOR position, U32 dwArchetypeDataID, U32 agentID) = 0;

	virtual void BeginConstruction (IBaseObject * jumpgate, U32 dwArchetypeDataID, U32 agentID) = 0;

	virtual void BeginRepair (U32 agentID, IBaseObject * repairTarget) = 0;

	virtual void BeginDismantle(U32 agentID, IBaseObject * dismantleTarget) = 0;

	virtual void LocalStartBuild () = 0;

	virtual void FabStartBuild (IBuild * newguy) = 0;

	virtual void FabStartAssistBuild (IBuild * newguy) = 0;

	virtual void FabAssistBuildDone () = 0;

	virtual void FabStopAssistBuild () = 0;

	virtual void FabStartDismantle (IBuild * oldguy) = 0;			

	virtual void LocalEndBuild (bool killAgent = false) = 0;

	virtual void FabEndBuild (bool bAborting) = 0;

	virtual void FabHaltBuild () = 0;

	virtual void FabSetBuildPause (bool bPause,SINGLE percent) = 0;

	virtual void FabEnableCompletion () = 0;

	virtual IBaseObject * GetBuildee() = 0;

	virtual void CloseBayDoors (SINGLE delay = 0) = 0;

	virtual void OpenBayDoors (SINGLE duration) = 0;

	virtual U32 GetNumDrones () =0;

	virtual IBaseObject * GetBuildEffectObj () = 0;

	virtual void GetDrone (struct IBuildShip **obj,U8 which) = 0;

	virtual TRANSFORM GetDroneTransform () = 0;

	virtual bool IsBuildingAgent (U32 agentID) = 0;

	virtual U8 GetFabTab() = 0;

	virtual void SetFabTab(U8 tab) = 0;

	virtual void FailSound(M_RESOURCE_TYPE resType) = 0;

	virtual bool IsFabAtObj () = 0;
};
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//

struct IBuildQueue: IObject
{
	enum COMMANDS
	{
		ADD,
		PAUSE,
		REMOVE,
		ADDIFEMPTY		// add only if queue is empty
	};

	virtual void BuildQueue (U32 dwArchetypeDataID, COMMANDS command) = 0;
	
	virtual U32 GetNumInQueue (U32 value) = 0;

	virtual bool IsInQueue (U32 value) = 0;

	virtual U32 GetQueue(U32 * queueCopy,U32 * slotIDs = NULL) = 0;

	virtual SINGLE FabGetProgress(U32 & stallType) = 0;

	virtual SINGLE FabGetDisplayProgress(U32 & stallType) = 0;

	virtual U32 GetFabJobID () = 0;

	virtual bool IsUpgradeInQueue () = 0;
};

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//

struct _NO_VTABLE IPlatform : IObject
{
	virtual TRANSFORM GetShipTransform () = 0;

	virtual TRANSFORM GetDockTransform () = 0;

	// slotID ( 1..MAX_SLOTS, or 0=no preference) is the suggested slot
	virtual void ParkYourself (const TRANSFORM & _trans, U32 planetID, U32 slotID) = 0;

	virtual void ParkYourself (IBaseObject * jumpgate) = 0;

	virtual void SetRallyPoint (const struct NETGRIDVECTOR & point) = 0;

	virtual bool GetRallyPoint (struct NETGRIDVECTOR & point) = 0;	// override point, return false if there is no rally point

	virtual bool IsDockLocked() = 0;

	virtual void LockDock(IBaseObject * locker) = 0;

	virtual void UnlockDock(IBaseObject * locker) = 0;

	virtual void FreePlanetSlot () = 0;

	virtual U32 GetPlanetID () = 0;

	virtual U32 GetSlotID () = 0;

	virtual U32 GetNumDocking () = 0;

	virtual void IncNumDocking () = 0;

	virtual void DecNumDocking () = 0;

	virtual bool IsDeepSpacePlatform() = 0;

	virtual bool IsMoonPlatform() = 0;

	virtual bool IsJumpPlatform() = 0;

	virtual bool IsRootSupply() = 0;

	virtual bool IsTempSupply() = 0;

	virtual bool IsReallyDead (void) = 0;

	virtual bool IsHalfSquare (void) = 0;

	virtual void ReceiveDeathPacket() = 0;

	virtual void StopActions() = 0;

	virtual U32 GetMetalStored() = 0;

	virtual U32 GetGasStored() = 0;

	virtual U32 GetCrewStored() = 0;

	virtual U32 GetMaxMetalStored() = 0;

	virtual U32 GetMaxGasStored() = 0;

	virtual U32 GetMaxCrewStored() = 0;

	virtual struct MeshInfo * GetRoots() = 0;

	virtual void AddHarvestRates(SINGLE & gas, SINGLE & metal, SINGLE & crew) = 0;

	virtual void ClientSideTakeover(U32 newID)= 0;
};


//----------------------------------------------------------------------------------
//-----------------------END IFabricator.h------------------------------------------
//----------------------------------------------------------------------------------
#endif