#ifndef OPAGENT_H
#define OPAGENT_H
//--------------------------------------------------------------------------//
//                                                                          //
//                               OpAgent.h                                  //
//                                                                          //
//                  COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/OpAgent.h 47    9/19/00 9:02p Jasony $
*/			    
//-------------------------------------------------------------------
// We're tracing the call!
//-------------------------------------------------------------------
//
#ifndef DACOM_H
#include <DACOM.h>
#endif
//-------------------------------------------------------------------
//
struct IAgentEnumerator
{
	struct NODE
	{
		U32 opID;
		enum HOSTCMD hostCmd;
		const struct ObjSet *pSet;
		Vector targetPosition;
		U32 targetSystemID;
		U32 dwFabArchetype;
		U32 fabTargetID;
		U32 fabSlotID;
	};

	virtual bool EnumerateAgent (const NODE & node) = 0;
};
//-------------------------------------------------------------------
//
struct DACOM_NO_VTABLE IOpAgent : public IDAComponent
{
	// can be called on client and host machines
	// WARNING: Calling this method can trigger another agent before this method returns!!
	//          You might consider using the helper function Operation_Completed2() below.
	virtual void OperationCompleted (U32 agentID, U32 dwMissionID) = 0;

	inline void OperationCompleted2 (U32 & agentID, U32 dwMissionID)
	{
		U32 tmp = agentID;
		agentID = 0;
		OperationCompleted(tmp, dwMissionID);
	}

	// can be called on client and host machines
	virtual void SetCancelState (U32 agentID, bool bEnable) = 0;

	// can be called on client and host machines
	virtual void SetLongTermState (U32 agentID, bool bEnable) = 0;		// for blackholes

	// called when a mission object has been fatally wounded by an attacker
	// the object will be destroyed at a later time, and the attacker notified.
	// Host side only!
	// if attackerID==0x80000000, do not explode.
	// if attackerID==0, there is no attacker
	virtual void ObjectTerminated (U32 victimID, U32 attackerID = 0x80000000) = 0;

	// 
	// the next set of methods are for support of a "process" concept
	//
	// creates an agent, calls IMissionActor::OnOperationCreation()
	// call OperationCompleted() to destroy the op
	// valid on host machine only!
	// returns agent created
	virtual U32 CreateOperation (const struct ObjSet & set, void *lpBuffer=0, U32 bufferSize=0) = 0;

	// returns agent created
	virtual U32 CreateOperation (U32 dwMissionID, void *lpBuffer=0, U32 bufferSize=0) = 0;

	// copies data, sends it to IMissionActor::ReceiveOperationData()
	// valid on host machine only!
	virtual void SendOperationData (U32 agentID, U32 dwMissionID, void *lpBuffer, U32 bufferSize) = 0;

	// adds a mission object to a PROCESS. 
	// can be called only on host machines!
	virtual void AddObjectToOperation (U32 agentID, U32 dwMissionID) = 0;

	virtual U32 GetNumAgents (void) const = 0;	// for debugging

	// valid on host machine only!
	virtual void FlushSyncData (void) = 0;	// called by Host before shutdown of net session

	// valid on host and client machines
	virtual bool IsMaster (void) = 0;	// return true if on host machine AND there no pending ops from old host

	// called by MGlobals::CreateInstance(), sets the b_deathPending flag accordingly.
	// this is needed because a unit might not exist when the packet is received, and needs to be tagged later.
	virtual bool HasPendingDeath (U32 dwMissionID) = 0;

	// used by troopship to make sure ENABLEDEATH packets arrive for the new ID
	// called only on the client side
	virtual void SwitchPendingDeath (U32 oldMissionID, U32 newMissionID) = 0;

	// group hasn't been used in a while, remove it from the networked session. (valid only on the master machine)
	virtual void SignalStaleGroupObject (U32 dwGroupID) = 0;

	virtual bool HasPendingOp (U32 dwMissionID) = 0;

	virtual bool HasPendingOp (IBaseObject * obj) = 0;

	virtual U32  GetWorkingOp (IBaseObject * obj) = 0;

	virtual void FlushOpQueueForUnit (IBaseObject * obj) = 0;

	virtual U32  GetDataThroughput (U32 * syncMask = 0, U32 * syncData = 0, U32 * opData = 0, U32 * harvest = 0) = 0;	// returns total bytes sent (before multiply by players)

	// host side only
	virtual void TerminatePlayerUnits (U32 playerID) = 0;

	virtual bool HasActiveJump (const struct ObjSet & set) = 0;

	virtual bool EnumerateQueuedMoveAgents (const struct ObjSet & set, IAgentEnumerator * callback) = 0;

	virtual bool EnumerateFabricateAgents (U32 playerID, IAgentEnumerator * callback) = 0;
	
	virtual bool IsFabricating (IBaseObject * obj) = 0;

	// is currently working on a non-cancellable op
	virtual bool IsNotInterruptable (IBaseObject * obj) = 0;

	virtual bool GetOperationSet (U32 agentID, const struct ObjSet ** ppSet) = 0;


	// nugget management methods (used only by nugget manager)
	virtual void SendNuggetData (void *lpBuffer, U32 bufferSize) = 0;

	virtual void SendNuggetDeath (U32 nuggetID, void *lpBuffer, U32 bufferSize) = 0;

	// method to send the find death packet for platforms
	virtual void SendPlatformDeath (U32 platformID) = 0;

	// method to force the object to be syncronized accross the network
	virtual void ForceSyncData(IBaseObject * obj) = 0;

private:

	virtual BOOL32 __stdcall New (void) = 0;

	virtual void __stdcall Close (void) = 0;

	virtual BOOL32 __stdcall Load (struct IFileSystem * inFile) = 0;
	
	virtual BOOL32 __stdcall Save (struct IFileSystem * outFile) = 0;

	virtual void Update (void) = 0;	// to be called at AI frame rate

	// start of network game, initialize all sync items
	virtual void ResetSyncItems (void) = 0;

	friend struct Mission;
};




#endif
