//--------------------------------------------------------------------------//
//                                                                          //
//                              CMenu_Research.cpp                               //
//                                                                          //
//               COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
   $Header: /Conquest/App/Src/CMenu_Research.cpp 60    9/19/01 9:54a Tmauer $
*/
//--------------------------------------------------------------------------//

 
#include "pch.h"
#include <globals.h>

#include "Startup.h"
#include "BaseHotRect.h"
#include "IToolbar.h"
#include "ObjList.h"
#include "IObject.h"
#include "MPart.h"
#include "IEdit2.h"
#include "IStatic.h"
#include "IHotButton.h"
#include "IActiveButton.h"
#include "MGlobals.h"
#include "CommPacket.h"
#include "IFabricator.h"
#include "IUpgrade.h"
#include "IQueueControl.h"
#include "DHotButtonText.h"
#include "Hotkeys.h"
#include "Sector.h"
#include "IIcon.h"
#include "IMissionActor.h"

#include <DMBaseData.h>
#include <DMTechNode.h>

#include <TComponent.h>
#include <IConnection.h>
#include <HeapObj.h>
#include <TSmartPointer.h>
#include <EventSys.h>

#include <stdio.h>

#define TIMEOUT_PERIOD 500
#define NUM_RESEARCH_BUTTONS 10
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//

struct DACOM_NO_VTABLE CMenu_Research : public IEventCallback, IHotControlEvent
{
	BEGIN_DACOM_MAP_INBOUND(CMenu_Research)
	DACOM_INTERFACE_ENTRY(IEventCallback)
	DACOM_INTERFACE_ENTRY(IHotControlEvent)
	END_DACOM_MAP()

	U32 eventHandle, hotEventHandle;		// connection handle
	bool bPanelOwned;
	S32 timeoutCount;

	COMPTR<IToolbar> menu;
	COMPTR<IStatic> shipclass,hull,supplies,metalStorage,gasStorage,crewStorage,location,disabledText;
	COMPTR<IActiveButton> research[NUM_RESEARCH_BUTTONS];
	COMPTR<IQueueControl> buildQueue;
	COMPTR<IIcon> inSupply,notInSupply;

	TECHNODE lastNode;
	TECHNODE lastWorkingNode;
	U32 lastMissionID;
	S32 lastObjType;
	bool bHasFocus;

	CMenu_Research (void);

	~CMenu_Research (void);

    void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}

	/* IEventCallback methods */

	DEFMETHOD(Notify) (U32 message, void *param = 0);

	/* IHotControlEvent methods */

	virtual void OnLeftButtonEvent (U32 menuID, U32 controlID)
	{
		if(!controlID)
			return;
		for(int index = 0; index < NUM_RESEARCH_BUTTONS; ++index)
		{
			if((research[index] != 0) && (research[index]->GetControlID() == controlID))
			{
				IBaseObject * obj = OBJLIST->GetSelectedList();
				MPart part(obj);
				if(obj && part.isValid() && (lastObjType == part->mObjClass) && (obj->objClass == OC_PLATFORM) && ((part->mObjClass == M_PROPLAB) || 
					(part->mObjClass == M_AWSLAB) || 
					(part->mObjClass == M_BALLISTICS) || 
					(part->mObjClass == M_ADVHULL) || (part->mObjClass == M_HANGER)|| 
					(part->mObjClass == M_DISPLAB) ||
					(part->mObjClass == M_LRSENSOR)  || (part->mObjClass == M_BUNKER)  || 
					(part->mObjClass == M_BLASTFURNACE) ||(part->mObjClass == M_EXPLOSIVESRANGE)||
					(part->mObjClass == M_CARRIONROOST) ||(part->mObjClass == M_BIOFORGE)
					||(part->mObjClass == M_FUSIONMILL)|| (part->mObjClass == M_PLANTATION)||
					(part->mObjClass == M_CARPACEPLANT) ||
					(part->mObjClass == M_HYBRIDCENTER) || (part->mObjClass == M_PLASMASPLITTER)||
					(part->mObjClass == M_HELIONVEIL)||
					(part->mObjClass == M_ANVIL) || (part->mObjClass == M_XENOCHAMBER)||
					(part->mObjClass == M_MUNITIONSANNEX) || (part->mObjClass == M_TURBINEDOCK) || 
					(part->mObjClass == M_HATCHERY) || (part->mObjClass == M_COCHLEA_DISH) ||
					(part->mObjClass == M_CLAW_OF_VYRIE) || (part->mObjClass == M_EYE_OF_VYRIE) || 
					(part->mObjClass == M_HAMMER_OF_VYRIE)))
				{
					OBJPTR<IBuildQueue> fab;
					obj->QueryInterface(IBuildQueueID,fab);
					if(fab)
					{
						if(!(fab->IsInQueue(research[index]->GetBuildArchetype())))
						{
							USR_PACKET<USRBUILD> packet;
							packet.cmd = USRBUILD::ADD;
							packet.dwArchetypeID = research[index]->GetBuildArchetype();
							packet.objectID[0] = obj->GetPartID();
							packet.init(1, false);
							NETPACKET->Send(HOSTID,0,&packet);
						}
					}
				}				
			}
		}
	}

	virtual void OnRightButtonEvent (U32 menuID, U32 controlID)
	{
		if(!controlID)
			return;
/*		for(int index = 0; index < NUM_RESEARCH_BUTTONS; ++index)
		{
			if((research[index] != 0) && (research[index]->GetControlID() == controlID))
			{
				IBaseObject * obj = OBJLIST->GetSelectedList();
				MPart part(obj);
				if((obj->objClass == OC_PLATFORM) && ((part->mObjClass == M_PROPLAB) || 
					(part->mObjClass == M_AWSLAB) || 
					(part->mObjClass == M_BALLISTICS) || 
					(part->mObjClass == M_ADVHULL) || (part->mObjClass == M_HANGER)|| 
					(part->mObjClass == M_WEAPONSLAB) || (part->mObjClass == M_DISPLAB) ||
					(part->mObjClass == M_LRSENSOR) ||(part->mObjClass == M_BUNKER)  || 
					(part->mObjClass == M_BLASTFURNACE) ||(part->mObjClass == M_EXPLOSIVESRANGE)||
					(part->mObjClass == M_CARRIONROOST) ||(part->mObjClass == M_BIOFORGE)
					||(part->mObjClass == M_FUSIONMILL)||(part->mObjClass == M_PLANTATION)||
					(part->mObjClass == M_CARPACEPLANT)|| 
					(part->mObjClass == M_HYBRIDCENTER) || (part->mObjClass == M_PLASMASPLITTER)||
					(part->mObjClass == M_HELIONVIEL)||
					(part->mObjClass == M_ANVIL) || (part->mObjClass == M_XENOCHAMBER)||
					(part->mObjClass == M_MUNITIONSANNEX) || (part->mObjClass == M_TURBINEDOCK)))
				{
					USR_PACKET<USRBUILD> packet;
					packet.cmd = USRBUILD::REMOVE;
					packet.dwArchetypeID = research[index]->GetBuildArchetype();
					packet.objectID[0] = obj->GetPartID();
					packet.init(1);
					NETPACKET->Send(HOSTID,0,&packet);
				}				
			}
		}
*/	}

	virtual void OnLeftDblButtonEvent (U32 menuID, U32 controlID)		// double-click
	{
	}
	
	/* CMenu_Research methods */

	void updateQueue ();

	void onUpdate (U32 dt);	// in mseconds

	void enableMenu (bool bEnable);

	void setPanelOwnership (bool bOwn);

	void handleHotkey (S32 hotkey);

	IDAComponent * getBase (void)
	{
		return static_cast<IEventCallback *>(this);
	}

};
//--------------------------------------------------------------------------//
//
CMenu_Research::CMenu_Research (void)
{
		// nothing to init so far?
}
//--------------------------------------------------------------------------//
//
CMenu_Research::~CMenu_Research (void)
{
	if (TOOLBAR)
	{
		COMPTR<IDAConnectionPoint> connection;
		
		if (TOOLBAR->QueryOutgoingInterface("IEventCallback", connection) == GR_OK)
			connection->Unadvise(eventHandle);
	}

}
//--------------------------------------------------------------------------//
// receive notifications from event system
//
GENRESULT CMenu_Research::Notify (U32 message, void *param)
{
//	MSG *msg = (MSG *) param;

	switch (message)
	{
	case CQE_PANEL_OWNED:
		if (((CMenu_Research *)param) != this)
			setPanelOwnership(false);
		break;

	case CQE_LOAD_INTERFACE:
		enableMenu(param!=0);
		break;

	case CQE_UPDATE:
		onUpdate(U32(param) >> 10);
		break;

	case CQE_KILL_FOCUS:
		bHasFocus = false;
		break;
	
	case CQE_SET_FOCUS:
		bHasFocus = true;
		break;

	case CQE_HOTKEY:
		if (bHasFocus || ( (TOOLBAR != 0) && (TOOLBAR->bInvisible) && (TOOLBAR->bHasFocus) ) )
			handleHotkey((S32) param);
		break;

	case CQE_BUILDQUEUE_REMOVE:
		{
			if(bPanelOwned)
			{
				IBaseObject * obj = OBJLIST->GetSelectedList();
				if (obj && obj->nextSelected==0 && obj->objClass == OC_PLATFORM)
				{
					MPart part = obj;
					if((obj->objClass == OC_PLATFORM) && ((part->mObjClass == M_PROPLAB) || 
						(part->mObjClass == M_AWSLAB) || 
						(part->mObjClass == M_BALLISTICS) || 
						(part->mObjClass == M_ADVHULL) || (part->mObjClass == M_HANGER)|| 
						(part->mObjClass == M_DISPLAB) ||
						(part->mObjClass == M_LRSENSOR) || (part->mObjClass == M_BUNKER)  || 
						(part->mObjClass == M_BLASTFURNACE) ||(part->mObjClass == M_EXPLOSIVESRANGE)||
						(part->mObjClass == M_CARRIONROOST) ||(part->mObjClass == M_BIOFORGE)
						||(part->mObjClass == M_FUSIONMILL)||(part->mObjClass == M_PLANTATION)||
						(part->mObjClass == M_CARPACEPLANT)||
						(part->mObjClass == M_HYBRIDCENTER) || (part->mObjClass == M_PLASMASPLITTER)||
						(part->mObjClass == M_HELIONVEIL)||
						(part->mObjClass == M_ANVIL) || (part->mObjClass == M_XENOCHAMBER)||
						(part->mObjClass == M_MUNITIONSANNEX) || (part->mObjClass == M_TURBINEDOCK) || 
						(part->mObjClass == M_HATCHERY) || (part->mObjClass == M_COCHLEA_DISH) ||
						(part->mObjClass == M_CLAW_OF_VYRIE) || (part->mObjClass == M_EYE_OF_VYRIE) || 
						(part->mObjClass == M_HAMMER_OF_VYRIE)))
					{
						USR_PACKET<USRBUILD> packet;
						packet.cmd = USRBUILD::REMOVE;
						packet.dwArchetypeID = ((U32)param);
						packet.objectID[0] = obj->GetPartID();
						packet.init(1, false);
						NETPACKET->Send(HOSTID,0,&packet);
					}
				}
			}
			break;
		}
	}

	return GR_OK;
}
//--------------------------------------------------------------------------//
//
void CMenu_Research::handleHotkey (S32 hotkey)
{
	if(bPanelOwned && hotkey >= IDH_CONTEXT_A && hotkey <= IDH_CONTEXT_Z)
	{
		for(int index = 0; index < NUM_RESEARCH_BUTTONS; ++index)
		{
			if((research[index] != 0) && (research[index]->IsActive()) && (research[index]->GetHotkey() == hotkey))
			{
				IBaseObject * obj = OBJLIST->GetSelectedList();
				MPart part(obj);
				if((lastObjType == part->mObjClass) && (obj->objClass == OC_PLATFORM) && ((part->mObjClass == M_PROPLAB) || 
					(part->mObjClass == M_AWSLAB) || 
					(part->mObjClass == M_BALLISTICS)|| 
					(part->mObjClass == M_ADVHULL) || (part->mObjClass == M_HANGER)|| 
					(part->mObjClass == M_DISPLAB) ||
					(part->mObjClass == M_LRSENSOR)  || (part->mObjClass == M_BUNKER)  || 
					(part->mObjClass == M_BLASTFURNACE) ||(part->mObjClass == M_EXPLOSIVESRANGE)||
					(part->mObjClass == M_CARRIONROOST) ||(part->mObjClass == M_BIOFORGE)
					||(part->mObjClass == M_FUSIONMILL)|| (part->mObjClass == M_PLANTATION)||
					(part->mObjClass == M_CARPACEPLANT) ||
					(part->mObjClass == M_HYBRIDCENTER) || (part->mObjClass == M_PLASMASPLITTER)||
					(part->mObjClass == M_HELIONVEIL)||
					(part->mObjClass == M_ANVIL) || (part->mObjClass == M_XENOCHAMBER)||
					(part->mObjClass == M_MUNITIONSANNEX) || (part->mObjClass == M_TURBINEDOCK) || 
					(part->mObjClass == M_HATCHERY) || (part->mObjClass == M_COCHLEA_DISH) ||
					(part->mObjClass == M_CLAW_OF_VYRIE) || (part->mObjClass == M_EYE_OF_VYRIE) || 
					(part->mObjClass == M_HAMMER_OF_VYRIE)))
				{
					OBJPTR<IBuildQueue> fab;
					obj->QueryInterface(IBuildQueueID,fab);
					if(fab)
					{
						if(!(fab->IsInQueue(research[index]->GetBuildArchetype())))
						{
							USR_PACKET<USRBUILD> packet;
							packet.cmd = USRBUILD::ADD;
							packet.dwArchetypeID = research[index]->GetBuildArchetype();
							packet.objectID[0] = obj->GetPartID();
							packet.init(1, false);
							NETPACKET->Send(HOSTID,0,&packet);
						}
					}
				}				
			}
		}
	}
}
//--------------------------------------------------------------------------//
//
void CMenu_Research::updateQueue ()
{
	buildQueue->ResetQueue();
	IBaseObject * obj = OBJLIST->GetSelectedList();
	if (obj && obj->nextSelected==0 && obj->objClass == OC_PLATFORM)
	{
		MPart part = obj;
		if(((part->mObjClass == M_PROPLAB) || 
					(part->mObjClass == M_AWSLAB) || 
					(part->mObjClass == M_BALLISTICS)|| 
					(part->mObjClass == M_ADVHULL) || (part->mObjClass == M_HANGER)|| 
					(part->mObjClass == M_DISPLAB) ||
					(part->mObjClass == M_LRSENSOR)|| (part->mObjClass == M_BUNKER)  || 
					(part->mObjClass == M_BLASTFURNACE) ||(part->mObjClass == M_EXPLOSIVESRANGE)||
					(part->mObjClass == M_CARRIONROOST) ||(part->mObjClass == M_BIOFORGE)
					||(part->mObjClass == M_FUSIONMILL)||(part->mObjClass == M_PLANTATION)||
					(part->mObjClass == M_CARPACEPLANT)|| 
					(part->mObjClass == M_HYBRIDCENTER) || (part->mObjClass == M_PLASMASPLITTER)||
					(part->mObjClass == M_HELIONVEIL)||
					(part->mObjClass == M_ANVIL) || (part->mObjClass == M_XENOCHAMBER)||
					(part->mObjClass == M_MUNITIONSANNEX) || (part->mObjClass == M_TURBINEDOCK) || 
					(part->mObjClass == M_HATCHERY) || (part->mObjClass == M_COCHLEA_DISH) ||
					(part->mObjClass == M_CLAW_OF_VYRIE) || (part->mObjClass == M_EYE_OF_VYRIE) || 
					(part->mObjClass == M_HAMMER_OF_VYRIE)))
		{
			OBJPTR<IBuildQueue> fab;
			obj->QueryInterface(IBuildQueueID,fab);
			if(fab)
			{
				U32 queue[FAB_MAX_QUEUE_SIZE];
				U32 slotIDs[FAB_MAX_QUEUE_SIZE];
				U32 numInQueue = fab->GetQueue(queue,slotIDs);
				if(numInQueue)
				{
					if(fab->IsUpgradeInQueue())
					{
						for(U32 rb = 0; rb < NUM_RESEARCH_BUTTONS; ++rb)
						{
							if(research[rb])
								research[rb]->SetUpgradeLevel(10);//show no upgradeButtons
						}
					}
					U32 stallType;
					SINGLE progress = fab->FabGetDisplayProgress(stallType);
					buildQueue->SetPercentage(progress,stallType);
					for(U32 i = 0; i < numInQueue; ++i)
					{
						IDrawAgent * queueShape[GTHBSHP_MAX_SHAPES];
						queueShape[0] = 0;
						for(U32 j = 0; j <NUM_RESEARCH_BUTTONS;++j)
						{
							if(research[j]) 
							{
								if(queue[i] == research[j]->GetControlID())
								{
									research[j]->GetShape(queueShape);
									break;
								}
							}
						}
						if(queueShape[0])
							buildQueue->AddToQueue(queueShape,slotIDs[i]);
					}
				}
			}
		}
	}
}
//--------------------------------------------------------------------------//
//
void CMenu_Research::onUpdate (U32 dt)
{
	if (bPanelOwned==false || S32(timeoutCount-dt) <= 0)
	{
		timeoutCount = TIMEOUT_PERIOD;

		//see if I have multiple objects selected object
		bool bKeepIt = false;

		IBaseObject * obj = OBJLIST->GetSelectedList();
		if (obj && obj->nextSelected==0 && obj->objClass == OC_PLATFORM)
		{
			MPart part = obj;
			if(((part->mObjClass == M_PROPLAB) || 
					(part->mObjClass == M_AWSLAB) || 
					(part->mObjClass == M_BALLISTICS)|| 
					(part->mObjClass == M_ADVHULL) || (part->mObjClass == M_HANGER)|| 
					(part->mObjClass == M_DISPLAB) ||
					(part->mObjClass == M_LRSENSOR)|| (part->mObjClass == M_BUNKER)  || 
					(part->mObjClass == M_BLASTFURNACE) ||(part->mObjClass == M_EXPLOSIVESRANGE)||
					(part->mObjClass == M_CARRIONROOST) ||(part->mObjClass == M_BIOFORGE)||
					(part->mObjClass == M_FUSIONMILL)||(part->mObjClass == M_PLANTATION)||
					(part->mObjClass == M_CARPACEPLANT)||
					(part->mObjClass == M_HYBRIDCENTER)|| (part->mObjClass == M_PLASMASPLITTER)||
					(part->mObjClass == M_HELIONVEIL)||
					(part->mObjClass == M_ANVIL) || (part->mObjClass == M_XENOCHAMBER)||
					(part->mObjClass == M_MUNITIONSANNEX) || (part->mObjClass == M_TURBINEDOCK) || 
					(part->mObjClass == M_HATCHERY) || (part->mObjClass == M_COCHLEA_DISH) ||
					(part->mObjClass == M_CLAW_OF_VYRIE) || (part->mObjClass == M_EYE_OF_VYRIE) || 
					(part->mObjClass == M_HAMMER_OF_VYRIE) ))
				bKeepIt = true;
		}
		if (bKeepIt==false && bPanelOwned) //If I do not and I am owning the panel then deactive myself
		{
			setPanelOwnership(false);
		}
		else
		if (bKeepIt && bPanelOwned==false) //else if I do have a selection and am not owning the panel activate myself
		{
			TOOLBAR->Notify(CQE_PANEL_OWNED, this);
			setPanelOwnership(true);
		}
		if(bKeepIt)
		{
			//setup info here
			MPart platform = obj;
			VOLPTR(IMissionActor) actor = obj;

			if(lastObjType != platform->mObjClass)
			{
				setPanelOwnership(false);
				setPanelOwnership(true);
			}

			//see if the system is in supply
			if(SECTOR->SystemInSupply(obj->GetSystemID(),obj->GetPlayerID()))
			{
				for(U32 i = 0; i <NUM_RESEARCH_BUTTONS; ++i)
				{
					if(research[i] != 0)
						research[i]->EnableButton(true);
				}
				if(disabledText)
					disabledText->SetVisible(false);
				if(inSupply)
					inSupply->SetVisible(true);
				if(notInSupply)
					notInSupply->SetVisible(false);
			}
			else
			{
				for(U32 i = 0; i <NUM_RESEARCH_BUTTONS; ++i)
				{
					if(research[i] != 0)
						research[i]->EnableButton(false);
				}
				if(disabledText)
					disabledText->SetVisible(true);
				if(inSupply)
					inSupply->SetVisible(false);
				if(notInSupply)
					notInSupply->SetVisible(true);
			}

			wchar_t buffer[256];
			if(lastMissionID != platform->dwMissionID)
			{
				lastMissionID = platform->dwMissionID;
				wchar_t * namePtr;
				wchar_t name[128];

				wcscpy(name, _localLoadStringW(platform.pInit->displayName));
				if ((namePtr = wcschr(name, '#')) != 0)
					*namePtr = 0;
				if ((namePtr = wcschr(name, '(')) != 0)
					*namePtr = 0;
				if ((namePtr = wcsrchr(name, '\'')) != 0)
				{
					namePtr++;
					if (namePtr[0] == ' ')
						namePtr++;
				}
				else
					namePtr = name;

				swprintf(buffer, _localLoadStringW(IDS_IND_SHIPCLASS), namePtr);
				if(shipclass)
					shipclass->SetText(buffer);			
			}
			
			//update technode infomation
			TECHNODE newNode = MGlobals::GetCurrentTechLevel(MGlobals::GetThisPlayer());
			TECHNODE workingNode = MGlobals::GetWorkingTechLevel(MGlobals::GetThisPlayer());
			if(!(newNode.IsEqual(lastNode)) || !(workingNode.IsEqual(lastWorkingNode)))
			{
				lastNode = newNode;
				lastWorkingNode = workingNode;
				for(int i = 0; i <NUM_RESEARCH_BUTTONS; ++i)
				{
					if(research[i] != 0)
						research[i]->SetTechNode(lastNode,MGlobals::GetTechAvailable(),workingNode);
				}
			}
			//update queue and prograss bar
			OBJPTR<IUpgrade> upgrade;
			obj->QueryInterface(IUpgradeID,upgrade);
			if(upgrade)
			{
				S8 upgradeVal = upgrade->GetWorkingUpgrade();
				if(upgradeVal == -1)
					upgradeVal = upgrade->GetUpgrade();
				for(U32 i = 0; i <NUM_RESEARCH_BUTTONS; ++i)
				{
					if(research[i] != 0)
					{
						research[i]->SetUpgradeLevel(upgradeVal);
					}
				}
			}


			swprintf(buffer, _localLoadStringW(IDS_IND_HULL), (actor)?actor->GetDisplayHullPoints() : platform->hullPoints, platform->hullPointsMax);
			hull->SetText(buffer);

			if(platform->supplyPointsMax)
			{
				supplies->SetVisible(true);
				swprintf(buffer, _localLoadStringW(IDS_IND_SUPPLIES), (actor) ? actor->GetDisplaySupplies() : platform->supplies, platform->supplyPointsMax);
				supplies->SetText(buffer);
			}
			else
			{
				supplies->SetVisible(false);
			}

			VOLPTR(IPlatform) plat = obj;
			if(plat)
			{
				U32 maxStore = plat->GetMaxMetalStored();
				if(maxStore)
				{
					U32 store = plat->GetMetalStored();
					swprintf(buffer, _localLoadStringW(IDS_METAL_STORE), store*METAL_MULTIPLIER,maxStore*METAL_MULTIPLIER);
					metalStorage->SetText(buffer);
					metalStorage->SetVisible(true);
				}
				else
				{
					metalStorage->SetVisible(false);
				}
				maxStore = plat->GetMaxGasStored();
				if(maxStore)
				{
					U32 store = plat->GetGasStored();
					swprintf(buffer, _localLoadStringW(IDS_GAS_STORE), store*GAS_MULTIPLIER,maxStore*GAS_MULTIPLIER);
					gasStorage->SetText(buffer);
					gasStorage->SetVisible(true);
				}
				else
				{
					gasStorage->SetVisible(false);
				}
				maxStore = plat->GetMaxCrewStored();
				if(maxStore)
				{
					U32 store = plat->GetCrewStored();
					swprintf(buffer, _localLoadStringW(IDS_CREW_STORE), store*CREW_MULTIPLIER,maxStore*CREW_MULTIPLIER);
					crewStorage->SetText(buffer);
					crewStorage->SetVisible(true);
				}
				else
				{
					crewStorage->SetVisible(false);
				}		
			}
			else
			{
				metalStorage->SetVisible(false);
				gasStorage->SetVisible(false);
				crewStorage->SetVisible(false);
			}

			U32 systemID = obj->GetSystemID();
			wchar_t nameBuff[255];
			if(systemID & HYPER_SYSTEM_MASK)
			{
				swprintf(nameBuff,_localLoadStringW(IDS_HYPERSPACE_LOC));
			}
			else
			{
				SECTOR->GetSystemName(nameBuff,sizeof(nameBuff),systemID);
			}
			swprintf(buffer,_localLoadStringW(IDS_CONTEXT_LOCATION),nameBuff);
			location->SetText(buffer);

			updateQueue();
		}
	}
	else
		timeoutCount -= dt;
}
//--------------------------------------------------------------------------//
//
void CMenu_Research::enableMenu (bool bEnable)
{
	if((!bEnable) && bPanelOwned)
	{
		setPanelOwnership(false);
	}
	if(bEnable)
	{
		COMPTR<IToolbar> toolbar;

		if (TOOLBAR->QueryInterface("IToolbar", toolbar) == GR_OK)
		{
			COMPTR<IDAConnectionPoint> connection;

			if (TOOLBAR->QueryOutgoingInterface("IHotControlEvent", connection) == GR_OK)
				connection->Advise(getBase(), &hotEventHandle);
		}
	}
	else
	{
		if(TOOLBAR)
		{
			COMPTR<IDAConnectionPoint> connection;
		
			if (TOOLBAR && TOOLBAR->QueryOutgoingInterface("IHotControlEvent", connection) == GR_OK)
				connection->Unadvise(hotEventHandle);
			hotEventHandle = 0;
		}
	}
}

void CMenu_Research::setPanelOwnership (bool bOwn)
{
	if(bOwn)
	{
		lastObjType = 0;
		lastMissionID = 0;
		COMPTR<IToolbar> toolbar;

		if (TOOLBAR->QueryInterface("IToolbar", toolbar) == GR_OK)
		{
			IBaseObject * obj = OBJLIST->GetSelectedList();
			MPart part(obj);
			char buffer[64];
			buffer[0] = 0;
			lastObjType = part->mObjClass;
			if(lastObjType == M_PROPLAB)
				strcpy(buffer,"proplab");
			else if(lastObjType == M_AWSLAB)
				strcpy(buffer,"awsLab");
			else if(part->mObjClass == M_BALLISTICS) 
				strcpy(buffer,"ballistics");
			else if(part->mObjClass == M_ADVHULL) 
				strcpy(buffer,"advHull");
			else if(part->mObjClass == M_HANGER)
				strcpy(buffer,"hanger");
			else if(part->mObjClass == M_DISPLAB)
				strcpy(buffer,"displacement");
			else if(part->mObjClass == M_LRSENSOR)
				strcpy(buffer,"lrsensor");
			else if(part->mObjClass == M_BLASTFURNACE)
				strcpy(buffer,"M_BlastFurnace");
			else if(part->mObjClass == M_EXPLOSIVESRANGE)
				strcpy(buffer,"M_ExplosivesRange");
			else if(part->mObjClass == M_CARRIONROOST)
				strcpy(buffer,"M_CarrionRoost");
			else if(part->mObjClass == M_BIOFORGE)
				strcpy(buffer,"M_BioForge");
			else if(part->mObjClass == M_FUSIONMILL)
				strcpy(buffer,"M_FusionMill");
			else if(part->mObjClass == M_CARPACEPLANT)
				strcpy(buffer,"M_CarpacePlant");
			else if(part->mObjClass == M_PLANTATION)
				strcpy(buffer,"M_Plantation");
			else if(part->mObjClass == M_HYBRIDCENTER)
				strcpy(buffer,"M_HybridCenter");
			else if(part->mObjClass == M_PLASMASPLITTER)
				strcpy(buffer,"M_PlasmaSpitter");
			else if(part->mObjClass == M_HELIONVEIL)
				strcpy(buffer,"S_HelionVeil");
			else if(part->mObjClass == M_ANVIL)
				strcpy(buffer,"S_Anvil");
			else if(part->mObjClass == M_XENOCHAMBER)
				strcpy(buffer,"S_XenoChamber");
			else if(part->mObjClass == M_MUNITIONSANNEX)
				strcpy(buffer,"S_MunitionsAnnex");
			else if(part->mObjClass == M_TURBINEDOCK)
				strcpy(buffer,"S_TurbineDock");
			else if(part->mObjClass == M_BUNKER)
				strcpy(buffer,"S_Bunker");
			else if(part->mObjClass == M_HATCHERY)
				strcpy(buffer,"V_Hatchery");
			else if(part->mObjClass == M_COCHLEA_DISH)
				strcpy(buffer,"V_CochleaDish");
			else if(part->mObjClass == M_CLAW_OF_VYRIE)
				strcpy(buffer,"V_ClawOfVyrie");
			else if(part->mObjClass == M_EYE_OF_VYRIE)
				strcpy(buffer,"V_EyeOfVyrie");
			else if(part->mObjClass == M_HAMMER_OF_VYRIE)
				strcpy(buffer,"V_HammerOfVyrie");

		if (toolbar->GetToolbar(buffer, menu, part->race) == GR_OK)
			{
				COMPTR<IDAComponent> pComp;

				if (toolbar->GetControl("shipclass", pComp) == GR_OK)
					pComp->QueryInterface("IStatic", shipclass);
				if (menu->GetControl("hull", pComp) == GR_OK)
					pComp->QueryInterface("IStatic", hull);
				if (menu->GetControl("supplies", pComp) == GR_OK)
					pComp->QueryInterface("IStatic", supplies);
				if (menu->GetControl("metalStorage", pComp) == GR_OK)
					pComp->QueryInterface("IStatic", metalStorage);
				if (menu->GetControl("gasStorage", pComp) == GR_OK)
					pComp->QueryInterface("IStatic", gasStorage);
				if (menu->GetControl("crewStorage", pComp) == GR_OK)
					pComp->QueryInterface("IStatic", crewStorage);
				if (menu->GetControl("location", pComp) == GR_OK)
					pComp->QueryInterface("IStatic", location);
				if (menu->GetControl("buildQueue", pComp) == GR_OK)
					pComp->QueryInterface("IQueueControl", buildQueue);
				if (menu->GetControl("disabledText", pComp) == GR_OK)
					pComp->QueryInterface("IStatic", disabledText);
				if (menu->GetControl("inSupply", pComp) == GR_OK)
					pComp->QueryInterface("IIcon", inSupply);
				if (menu->GetControl("notInSupply", pComp) == GR_OK)
					pComp->QueryInterface("IIcon", notInSupply);

				lastNode = MGlobals::GetCurrentTechLevel(MGlobals::GetThisPlayer());
				lastWorkingNode = MGlobals::GetWorkingTechLevel(MGlobals::GetThisPlayer());
				char resName[] = "research";
				char final[32];
				for(int i = 0; i <NUM_RESEARCH_BUTTONS; ++i)
				{
					sprintf(final,"%s%d",resName,i);
					if (menu->GetControl(final, pComp) == GR_OK)
						pComp->QueryInterface("IActiveButton", research[i]);
					if(research[i] != 0)
					{
						research[i]->SetControlID(research[i]->GetBuildArchetype());//hack;
						research[i]->SetTechNode(lastNode,MGlobals::GetTechAvailable(),lastWorkingNode);
					}
				}

				COMPTR<IDAConnectionPoint> connection;
	
				if (menu->QueryOutgoingInterface("IHotControlEvent", connection) == GR_OK)
					connection->Advise(getBase(), &hotEventHandle);
			}

			if(menu)
				menu->SetVisible(true);
			bPanelOwned = true;
		}
	}else{
		if(menu)
		{
			menu->SetVisible(false);
			COMPTR<IDAConnectionPoint> connection;
		
			if (menu->QueryOutgoingInterface("IHotControlEvent", connection) == GR_OK)
				connection->Unadvise(hotEventHandle);
			hotEventHandle = 0;
		}
		bPanelOwned = false;
		lastObjType = 0;

		menu.free();
		shipclass.free();
		hull.free();
		supplies.free();
		crewStorage.free();
		metalStorage.free();
		gasStorage.free();
		location.free();
		buildQueue.free();
		inSupply.free();
		notInSupply.free();
		disabledText.free();
		for(int i = 0; i <NUM_RESEARCH_BUTTONS; ++i)
		{
			research[i].free();
		}

	}
}

//--------------------------------------------------------------------------//
//
struct _cmenu_research: GlobalComponent
{
	CMenu_Research * menu;

	virtual void Startup (void)
	{
		menu = new DAComponent<CMenu_Research>;
		AddToGlobalCleanupList(&menu);
	}

	virtual void Initialize (void)
	{
		COMPTR<IDAConnectionPoint> connection;

		if (TOOLBAR->QueryOutgoingInterface("IEventCallback", connection) == GR_OK)
			connection->Advise(menu->getBase(), &menu->eventHandle);
	}
};

static _cmenu_research startup;

//--------------------------------------------------------------------------//
//-----------------------------End CMenu_Ind.cpp----------------------------//
//--------------------------------------------------------------------------//
