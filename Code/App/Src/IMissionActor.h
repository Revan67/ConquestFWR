#ifndef IMISSIONACTOR_H
#define IMISSIONACTOR_H
//--------------------------------------------------------------------------//
//                                                                          //
//                             IMissionActor.h                              //
//                                                                          //
//                  COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/IMissionActor.h 26    9/19/00 9:02p Jasony $
*/			    
//------------------------------- #INCLUDES --------------------------------//
//--------------------------------------------------------------------------//
#ifndef IOBJECT_H
#include "IObject.h"
#endif

struct BASE_MISSION_SAVELOAD;		// "..\dinclude\DMBaseData.h"
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
struct _NO_VTABLE IMissionActor : IObject
{
	// called after mission data is set up, position is set, etc.
	virtual void InitActor (void) = 0;

	// methods for accessing the "visible" hullPoints/supplies
	virtual U32 GetDisplayHullPoints (void) const = 0;
	virtual U32 GetDisplaySupplies (void) const = 0;

	// called just before SelfDestruct()
	virtual void PreSelfDestruct (void) = 0;

	virtual void SelfDestruct (bool bExplode) = 0;

	virtual void SetMissionData (const MISSION_SAVELOAD * pMissionData) = 0;

	// the following methods are for network synchronization of realtime objects

	virtual U32 GetPrioritySyncData (void * buffer) = 0;

	virtual U32 GetGeneralSyncData (void * buffer) = 0;

	// some sync packets may be delivered early if the client completes ops early
	virtual void PutPrioritySyncData (void * buffer, U32 bufferSize, bool bLateDelivery) = 0;

	// some sync packets may be delivered early if the client completes ops early
	virtual void PutGeneralSyncData (void * buffer, U32 bufferSize, bool bLateDelivery) = 0;


	// the following are process related
	// called in response to OpAgent::CreateOperation()
	virtual void OnOperationCreation (U32 agentID, void *buffer, U32 bufferSize) = 0;

	// called in response to OpAgent::SendOperationData()
	virtual void ReceiveOperationData (U32 agentID, void *buffer, U32 bufferSize) = 0;

	// user has requested a different action, op is about to be cancelled
	virtual void OnOperationCancel (U32 agentID) = 0;

	// user has requested a different action (op is not cancellable)
	virtual void OnStopRequest (U32 agentID) = 0;

	// An event has occured that requires the object to enter a completly neutral state.
	virtual void PrepareTakeover (U32 newMissionID, U32 troopID) = 0;

	virtual void TakeoverSwitchID (U32 newMissionID) = 0;
	
	// notify that Master status is changing!
	virtual void OnMasterChange (bool bIsMaster) = 0;

	// called in response to OpAgent::AddToOperation()
	virtual void OnAddToOperation (U32 agentID) = 0;
};

//---------------------------------------------------------------------------
//-----------------------END IMissionActor.h---------------------------------
//---------------------------------------------------------------------------
#endif
