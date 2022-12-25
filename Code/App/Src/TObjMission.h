#ifndef TOBJMISSION_H
#define TOBJMISSION_H
//--------------------------------------------------------------------------//
//                                                                          //
//                              TObjMission.h                               //
//                                                                          //
//                  COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/TObjMission.h 48    10/20/00 4:16p Tmauer $
*/			    
//------------------------------- #INCLUDES --------------------------------//
//--------------------------------------------------------------------------//

#ifndef IMISSIONACTOR_H
#include "IMissionActor.h"
#endif

#ifndef OBJLIST_H
#include "ObjList.h"
#endif

#ifndef TOBJFRAME_H
#include "TObjFrame.h"
#endif

#ifndef DMBASEDATA_H
#include <DMBaseData.h>
#endif

#ifndef SECTOR_H
#include "Sector.h"
#endif

#ifndef IADMIRAL_H
#include "IAdmiral.h"
#endif

#ifndef USERDEFAULTS_H
#include "UserDefaults.h"
#endif

#ifndef MPART_H
#include "MPart.h"
#endif

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
#define ObjectMission _Com

template <class Base=IBaseObject> 
struct _NO_VTABLE ObjectMission : public Base, IMissionActor, MISSION_SAVELOAD
{
	typename typedef Base::SAVEINFO MISSIONSAVEINFO;
	typename typedef Base::INITINFO MISSIONINITINFO;

	struct SaveNode  saveNode;
	struct LoadNode  loadNode;
	struct InitNode  initNode;
	struct RenderNode renderNode;
	
	//
	// mission data
	//
	const MISSION_DATA * pInitData;

	ObjectMission (void) :
					saveNode(this, SaveLoadProc(&ObjectMission::saveMissionData)),
					loadNode(this, SaveLoadProc(&ObjectMission::loadMissionData)),
					initNode(this, InitProc(&ObjectMission::initMissionState)),
					renderNode(this, RenderProc(&ObjectMission::renderMission))
	{
#ifndef FINAL_RELEASE
		SetDebugName(partName);
#endif  // end !FINAL_RELEASE
	}
	
	virtual ~ObjectMission (void)
	{
		OBJLIST->RemovePartID(this, dwMissionID);
	}

	/* IBaseObject methods */

	virtual U32 GetSystemID (void) const
	{
		return systemID;
	}

	virtual U32 GetPartID (void) const
	{
		return dwMissionID;
	}

	virtual U32 GetPlayerID (void) const
	{
		return dwMissionID & PLAYERID_MASK;
	}

	virtual bool GetMissionData (IBaseObject::MDATA & mdata) const	// return true if message was handled
	{
		mdata.pSaveData = this;
		mdata.pInitData = pInitData;
		return true;
	}

	/* IPhysicalObject methods */

	virtual void SetSystemID (U32 newSystemID)
	{
		systemID = newSystemID;
		/*
		if(!(systemID & HYPER_SYSTEM_MASK))
		{
			U32 alertState = SECTOR->GetAlertState(systemID,GetPlayerID());
			if(alertState & S_LOCKED)
			{
				bUntouchable = true;
				if (objMapNode)
					objMapNode->flags |= OM_UNTOUCHABLE;
				if(bSelected)
					OBJLIST->UnselectObject(this);
			}
			else
			{
				bUntouchable = false;
				if (objMapNode)
					objMapNode->flags &= ~OM_UNTOUCHABLE;
			}
		}
		*/
	}

	/* IMissionActor methods */

	virtual void InitActor (void)
	{
	}

	// methods for accessing the "displayed" hullPoints/supplies
	virtual U32 GetDisplayHullPoints (void) const
	{
		return hullPoints;
	}
	virtual U32 GetDisplaySupplies (void) const
	{
		return supplies;
	}

	void SetMissionData (const MISSION_SAVELOAD * pMissionData)
	{
		OBJLIST->RemovePartID(this, dwMissionID);

		CQASSERT(pInitData->mObjClass == pMissionData->mObjClass);

		memcpy(static_cast<MISSION_SAVELOAD *>(this), pMissionData, sizeof(MISSION_SAVELOAD));

		OBJLIST->AddPartID(this, dwMissionID);
	}

	virtual void PreSelfDestruct (void)
	{
		FRAME_preDestruct();
	}

	virtual void SelfDestruct (bool bExplode)
	{
		UnregisterWatchersForObject(this);
		FRAME_explode(bExplode);
		//
		// if for some reason we are still in the object list, delete self
		//
		if (OBJLIST->IsInList(this))
			delete this;
	}

	// the following methods are for network synchronization of realtime objects

	virtual U32 GetPrioritySyncData (void * buffer)
	{
		return FRAME_getPrioritySyncData(buffer);
	}

	virtual U32 GetGeneralSyncData (void * buffer)
	{
		return FRAME_getGeneralSyncData(buffer);
	}

	virtual void PutPrioritySyncData (void * buffer, U32 bufferSize, bool bLateDelivery)
	{
		FRAME_putPrioritySyncData(buffer, bufferSize, bLateDelivery);
	}

	virtual void PutGeneralSyncData (void * buffer, U32 bufferSize, bool bLateDelivery)
	{
		FRAME_putGeneralSyncData(buffer, bufferSize, bLateDelivery);
	}

	virtual void OnOperationCreation (U32 agentID, void *buffer, U32 bufferSize)
	{
	}

	virtual void ReceiveOperationData (U32 agentID, void *buffer, U32 bufferSize)
	{
		FRAME_receiveOpData(agentID,buffer,bufferSize);
	}

	virtual void OnOperationCancel (U32 agentID)
	{
		FRAME_onOpCancel(agentID);
	}

	virtual void OnStopRequest (U32 agentID)
	{
	}

	virtual void PrepareTakeover (U32 newMissionID, U32 troopID)
	{
		FRAME_preTakeover(newMissionID, troopID);
	}

	virtual void TakeoverSwitchID (U32 newMissionID)
	{
	}

	virtual void OnMasterChange (bool bIsMaster)
	{
	}

	virtual void OnAddToOperation (U32 agentID)
	{
	}

	/* ObjectMission methods */

	void saveMissionData (MISSIONSAVEINFO & save)
	{
		save.mission = *static_cast<MISSION_SAVELOAD *> (this);
	}

	void loadMissionData (MISSIONSAVEINFO & load)
	{
		*static_cast<MISSION_SAVELOAD *> (this) = load.mission;
		if (load.mission.dwMissionID)		// use this load variable for optimizer (jy)
			OBJLIST->AddPartID(this, load.mission.dwMissionID);
	}

	void initMissionState (const MISSIONINITINFO & data)
	{
		pInitData = &data.pData->missionData;
	}

	void renderMission()
	{
		if(DEFAULTS->GetDefaults()->bInfoHighlights && bHighlight)
		{
			if(sensorRadius)
			{
				SINGLE admiralSensorMod = 1.0;
				if(fleetID && supplies)
				{
					VOLPTR(IAdmiral) flagship;
					OBJLIST->FindObject(fleetID,TOTALLYVOLATILEPTR,flagship,IAdmiralID);
					if(flagship.Ptr())
					{
						MPart part(this);
						admiralSensorMod = 1+flagship->GetSensorBonus(mObjClass,part.pInit->armorData.myArmor);
					}				
				}
				SINGLE bonus = fieldFlags.getSensorDampingMod()*effectFlags.getSensorDampingMod()*admiralSensorMod*SECTOR->GetSectorEffects(playerID,systemID)->getSensorMod();
				drawRangeCircle(__max(0.75,pInitData->sensorRadius*bonus ),RGB(0,128,0));
				drawRangeCircle(__max(0.75,sensorRadius*bonus),RGB(0,255,0));
			}
			if(cloakedSensorRadius)
			{
				SINGLE admiralSensorMod = 1.0;
				if(fleetID && supplies)
				{
					VOLPTR(IAdmiral) flagship;
					OBJLIST->FindObject(fleetID,TOTALLYVOLATILEPTR,flagship,IAdmiralID);
					if(flagship.Ptr())
					{
						MPart part(this);
						admiralSensorMod = 1+flagship->GetSensorBonus(mObjClass,part.pInit->armorData.myArmor);
					}				
				}
				SINGLE bonus = fieldFlags.getSensorDampingMod()*effectFlags.getSensorDampingMod()*admiralSensorMod*SECTOR->GetSectorEffects(playerID,systemID)->getSensorMod();
				drawRangeCircle(pInitData->cloakedSensorRadius*bonus,RGB(128,128,128));
				drawRangeCircle(cloakedSensorRadius*bonus,RGB(255,255,255));
			}

		}
	}
};

//---------------------------------------------------------------------------
//---------------------------End TObjMission.h-------------------------------
//---------------------------------------------------------------------------
#endif