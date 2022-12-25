//--------------------------------------------------------------------------//
//                                                                          //
//                              CMenu_ResBuild.cpp                               //
//                                                                          //
//               COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
   $Header: /Conquest/App/Src/CMenu_ResBuild.cpp 40    9/19/01 9:54a Tmauer $
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
#include "CommPacket.h"
#include "IFabricator.h"
#include "MGlobals.h"
#include "IQueueControl.h"
#include "DHotButtonText.h"
#include "Hotkeys.h"
#include "Sector.h"
#include "IUpgrade.h"
#include "IIcon.h"
#include "IMissionActor.h"
#include "IBuild.h"

#include <DMBaseData.h>
#include <DMTechNode.h>

#include <TComponent.h>
#include <IConnection.h>
#include <HeapObj.h>
#include <TSmartPointer.h>
#include <EventSys.h>

#include <stdio.h>

#define TIMEOUT_PERIOD 500
#define NUM_IND_BUTTONS 6
#define NUM_RESEARCH_BUTTONS 16

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//

struct DACOM_NO_VTABLE CMenu_ResBuild : public IEventCallback, IHotControlEvent
{
	BEGIN_DACOM_MAP_INBOUND(CMenu_ResBuild)
	DACOM_INTERFACE_ENTRY(IEventCallback)
	DACOM_INTERFACE_ENTRY(IHotControlEvent)
	END_DACOM_MAP()

	U32 eventHandle, hotEventHandle;		// connection handle
	bool bPanelOwned;
	S32 timeoutCount;
	TECHNODE lastNode;
	TECHNODE lastWorkingNode;

	COMPTR<IToolbar> menu;
	COMPTR<IStatic> shipclass,hull,metalStorage,gasStorage,crewStorage,location,disabledText;
	COMPTR<IActiveButton> buildButton[NUM_IND_BUTTONS];
	COMPTR<IActiveButton> research[NUM_RESEARCH_BUTTONS];
	COMPTR<IQueueControl> buildQueue;
	COMPTR<IIcon> inSupply,notInSupply;
	COMPTR<IHotButton> noAuto,autoOre,autoGas;

	U32 lastMissionID;
	S32 lastObjType;
	bool bHasFocus;

	CMenu_ResBuild (void);

	~CMenu_ResBuild (void);

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
		int index;
		for(index = 0; index < NUM_IND_BUTTONS; ++index)
		{
			if((buildButton[index] != 0) && (buildButton[index]->GetControlID() == controlID))
			{
				IBaseObject * obj = OBJLIST->GetSelectedList();
				MPart part(obj);
				if(obj && part.isValid() && (lastObjType == part->mObjClass) && (obj->objClass == OC_PLATFORM) && 
					((part->mObjClass == M_ACADEMY) || (part->mObjClass == M_EYESTOCK) ||(part->mObjClass == M_WARLORDTRAINING)||(part->mObjClass == M_THRIPID)
				|| (part->mObjClass == M_MUTATIONCOLONY)|| (part->mObjClass == M_SENTINELTOWER)|| 
				(part->mObjClass == M_CITADEL)|| (part->mObjClass == M_HEAVYREFINERY) || 
				(part->mObjClass == M_REFINERY) || (part->mObjClass == M_COLLECTOR) ||
				(part->mObjClass == M_SUPERHEAVYREFINERY) || (part->mObjClass == M_GREATER_COLLECTOR) ||
				(part->mObjClass == M_OXIDATOR) || (part->mObjClass == M_COALESCER) ||
				(part->mObjClass == M_GUDGEON) || (part->mObjClass == M_TEMPLE_OF_VYRIE) ||
				(part->mObjClass == M_OUTPOST)))
				{
					USR_PACKET<USRBUILD> packet;
					packet.cmd = USRBUILD::ADD;
					packet.dwArchetypeID = buildButton[index]->GetBuildArchetype();
					packet.objectID[0] = obj->GetPartID();
					packet.init(1, false);
					NETPACKET->Send(HOSTID,0,&packet);
				}				
			}
		}
		for(index = 0; index < NUM_RESEARCH_BUTTONS; ++index)
		{
			if((research[index] != 0) && (research[index]->GetControlID() == controlID))
			{
				IBaseObject * obj = OBJLIST->GetSelectedList();
				MPart part(obj);
				if(obj && part.isValid() && (lastObjType == part->mObjClass) && (obj->objClass == OC_PLATFORM) && 
					((part->mObjClass == M_ACADEMY)|| (part->mObjClass == M_EYESTOCK) ||(part->mObjClass == M_WARLORDTRAINING)||(part->mObjClass == M_THRIPID)
				|| (part->mObjClass == M_MUTATIONCOLONY) || (part->mObjClass == M_SENTINELTOWER)|| 
				(part->mObjClass == M_CITADEL)|| (part->mObjClass == M_HEAVYREFINERY)|| 
				(part->mObjClass == M_REFINERY)|| (part->mObjClass == M_COLLECTOR)||
				(part->mObjClass == M_SUPERHEAVYREFINERY)|| (part->mObjClass == M_GREATER_COLLECTOR)||
				(part->mObjClass == M_OXIDATOR) || (part->mObjClass == M_COALESCER) ||
				(part->mObjClass == M_GUDGEON) || (part->mObjClass == M_TEMPLE_OF_VYRIE)) ||
				(part->mObjClass == M_OUTPOST))
				{
					OBJPTR<IBuildQueue> fab;
					obj->QueryInterface(IBuildQueueID,fab);
					if(fab)
					{
						if(!fab->IsInQueue(research[index]->GetBuildArchetype()))
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
/*		for(int index = 0; index < NUM_IND_BUTTONS; ++index)
		{
			if((buildButton[index] != 0) && (buildButton[index]->GetControlID() == controlID))
			{
				IBaseObject * obj = OBJLIST->GetSelectedList();
				MPart part(obj);
				if((lastObjType == part->mObjClass) && (obj->objClass == OC_PLATFORM) && 
					((part->mObjClass == M_ACADEMY)|| (part->mObjClass == M_EYESTOCK) ||(part->mObjClass == M_WARLORDTRAINING) ||(part->mObjClass == M_THRIPID)
				|| (part->mObjClass == M_MUTATIONCOLONY)|| (part->mObjClass == M_SENTIALTOWER)|| 
				(part->mObjClass == M_CITADEL)|| (part->mObjClass == M_HEAVYREFINERY)|| 
				(part->mObjClass == M_REFINERY)|| (part->mObjClass == M_COLLECTOR)||
				(part->mObjClass == M_SUPERHEAVYREFINERY)|| (part->mObjClass == M_GREATER_COLLECTOR)||
				(part->mObjClass == M_OXIDATOR)))
				{
					USR_PACKET<USRBUILD> packet;
					packet.cmd = USRBUILD::REMOVE;
					packet.dwArchetypeID = buildButton[index]->GetBuildArchetype();
					packet.objectID[0] = obj->GetPartID();
					packet.init(1);
					NETPACKET->Send(HOSTID,0,&packet);
				}				
			}
		}
		for(index = 0; index < NUM_RESEARCH_BUTTONS; ++index)
		{
			if((research[index] != 0) && (research[index]->GetControlID() == controlID))
			{
				IBaseObject * obj = OBJLIST->GetSelectedList();
				MPart part(obj);
				if((lastObjType == part->mObjClass) && (obj->objClass == OC_PLATFORM) && 
					((part->mObjClass == M_ACADEMY)|| (part->mObjClass == M_EYESTOCK) ||(part->mObjClass == M_WARLORDTRAINING)||(part->mObjClass == M_THRIPID)
				|| (part->mObjClass == M_MUTATIONCOLONY)|| (part->mObjClass == M_SENTIALTOWER)|| 
				(part->mObjClass == M_CITADEL)|| (part->mObjClass == M_HEAVYREFINERY)|| 
				(part->mObjClass == M_REFINERY)|| (part->mObjClass == M_COLLECTOR)||
				(part->mObjClass == M_SUPERHEAVYREFINERY)|| (part->mObjClass == M_GREATER_COLLECTOR)||
				(part->mObjClass == M_OXIDATOR)))
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
*/
	}

	virtual void OnLeftDblButtonEvent (U32 menuID, U32 controlID)		// double-click
	{
	}
	
	/* CMenu_LightInd methods */

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
CMenu_ResBuild::CMenu_ResBuild (void)
{
		// nothing to init so far?
}
//--------------------------------------------------------------------------//
//
CMenu_ResBuild::~CMenu_ResBuild (void)
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
GENRESULT CMenu_ResBuild::Notify (U32 message, void *param)
{
//	MSG *msg = (MSG *) param;

	switch (message)
	{
	case CQE_PANEL_OWNED:
		if (((CMenu_ResBuild *)param) != this)
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
					if((lastObjType == part->mObjClass) && (obj->objClass == OC_PLATFORM) && 
						((part->mObjClass == M_ACADEMY)|| (part->mObjClass == M_EYESTOCK) ||(part->mObjClass == M_WARLORDTRAINING) ||(part->mObjClass == M_THRIPID)
						|| (part->mObjClass == M_MUTATIONCOLONY)|| (part->mObjClass == M_SENTINELTOWER)|| 
						(part->mObjClass == M_CITADEL)|| (part->mObjClass == M_HEAVYREFINERY)|| 
						(part->mObjClass == M_REFINERY)|| (part->mObjClass == M_COLLECTOR)||
						(part->mObjClass == M_SUPERHEAVYREFINERY)|| (part->mObjClass == M_GREATER_COLLECTOR)||
						(part->mObjClass == M_OXIDATOR) || (part->mObjClass == M_COALESCER) ||
						(part->mObjClass == M_GUDGEON) || (part->mObjClass == M_TEMPLE_OF_VYRIE) ||
						(part->mObjClass == M_OUTPOST)))
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
void CMenu_ResBuild::handleHotkey (S32 hotkey)
{
	if(bPanelOwned && hotkey >= IDH_CONTEXT_A && hotkey <= IDH_CONTEXT_Z)
	{
		int index;
		for(index = 0; index < NUM_IND_BUTTONS; ++index)
		{
			if((buildButton[index] != 0) && (buildButton[index]->IsActive()) && (buildButton[index]->GetHotkey() == hotkey))
			{
				IBaseObject * obj = OBJLIST->GetSelectedList();
				MPart part(obj);
				if((lastObjType == part->mObjClass) && (obj->objClass == OC_PLATFORM) && 
					((part->mObjClass == M_ACADEMY) || (part->mObjClass == M_EYESTOCK) ||(part->mObjClass == M_WARLORDTRAINING)||(part->mObjClass == M_THRIPID)
				|| (part->mObjClass == M_MUTATIONCOLONY)|| (part->mObjClass == M_SENTINELTOWER)|| 
				(part->mObjClass == M_CITADEL)|| (part->mObjClass == M_HEAVYREFINERY) || 
				(part->mObjClass == M_REFINERY) || (part->mObjClass == M_COLLECTOR) ||
				(part->mObjClass == M_SUPERHEAVYREFINERY) || (part->mObjClass == M_GREATER_COLLECTOR) ||
				(part->mObjClass == M_OXIDATOR) || (part->mObjClass == M_COALESCER) ||
				(part->mObjClass == M_GUDGEON) || (part->mObjClass == M_TEMPLE_OF_VYRIE) ||
				(part->mObjClass == M_OUTPOST)))
				{
					USR_PACKET<USRBUILD> packet;
					packet.cmd = USRBUILD::ADD;
					packet.dwArchetypeID = buildButton[index]->GetBuildArchetype();
					packet.objectID[0] = obj->GetPartID();
					packet.init(1, false);
					NETPACKET->Send(HOSTID,0,&packet);
				}				
			}
		}
		for(index = 0; index < NUM_RESEARCH_BUTTONS; ++index)
		{
			if((research[index] != 0) && (research[index]->IsActive()) && (research[index]->GetHotkey() == hotkey))
			{
				IBaseObject * obj = OBJLIST->GetSelectedList();
				MPart part(obj);
				if((lastObjType == part->mObjClass) && (obj->objClass == OC_PLATFORM) && 
					((part->mObjClass == M_ACADEMY)|| (part->mObjClass == M_EYESTOCK) ||(part->mObjClass == M_WARLORDTRAINING)||(part->mObjClass == M_THRIPID)
				|| (part->mObjClass == M_MUTATIONCOLONY) || (part->mObjClass == M_SENTINELTOWER)|| 
				(part->mObjClass == M_CITADEL)|| (part->mObjClass == M_HEAVYREFINERY)|| 
				(part->mObjClass == M_REFINERY)|| (part->mObjClass == M_COLLECTOR)||
				(part->mObjClass == M_SUPERHEAVYREFINERY)|| (part->mObjClass == M_GREATER_COLLECTOR)||
				(part->mObjClass == M_OXIDATOR) || (part->mObjClass == M_COALESCER) ||
				(part->mObjClass == M_GUDGEON) || (part->mObjClass == M_TEMPLE_OF_VYRIE) ||
				(part->mObjClass == M_OUTPOST)))
				{
					OBJPTR<IBuildQueue> fab;
					obj->QueryInterface(IBuildQueueID,fab);
					if(fab)
					{
						if(!fab->IsInQueue(research[index]->GetBuildArchetype()))
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
void CMenu_ResBuild::updateQueue ()
{
	buildQueue->ResetQueue();
	IBaseObject * obj = OBJLIST->GetSelectedList();
	if (obj && obj->nextSelected==0 && obj->objClass == OC_PLATFORM)
	{
		MPart part = obj;
		if(((part->mObjClass == M_ACADEMY)|| (part->mObjClass == M_EYESTOCK) ||(part->mObjClass == M_WARLORDTRAINING) ||(part->mObjClass == M_THRIPID)
			|| (part->mObjClass == M_MUTATIONCOLONY)|| (part->mObjClass == M_SENTINELTOWER)|| 
			(part->mObjClass == M_CITADEL)|| (part->mObjClass == M_HEAVYREFINERY)|| 
			(part->mObjClass == M_REFINERY)|| (part->mObjClass == M_COLLECTOR)||
			(part->mObjClass == M_SUPERHEAVYREFINERY)|| (part->mObjClass == M_GREATER_COLLECTOR)||
			(part->mObjClass == M_OXIDATOR) || (part->mObjClass == M_COALESCER) ||
			(part->mObjClass == M_GUDGEON) || (part->mObjClass == M_TEMPLE_OF_VYRIE) ||
				(part->mObjClass == M_OUTPOST)))
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
						U32 j;
						for(j = 0; j <NUM_IND_BUTTONS;++j)
						{
							if(buildButton[j])
							{
								if(queue[i] == buildButton[j]->GetControlID())
								{
									buildButton[j]->GetShape(queueShape);
									break;
								}
							}
						}
						if(j == NUM_IND_BUTTONS)
						{
							for(j = 0; j <NUM_RESEARCH_BUTTONS;++j)
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
void CMenu_ResBuild::onUpdate (U32 dt)
{
	if (bPanelOwned==false || S32(timeoutCount-dt) <= 0)
	{
//		TECHNODE curTech = MGlobals::GetCurrentTechLevel(MGlobals::GetThisPlayer());

		timeoutCount = TIMEOUT_PERIOD;

		//see if I have multiple objects selected object
		bool bKeepIt = false;

		IBaseObject * obj = OBJLIST->GetSelectedList();
		if (obj && obj->nextSelected==0 && obj->objClass == OC_PLATFORM)
		{
			MPart part = obj;
			if(((part->mObjClass == M_ACADEMY)|| (part->mObjClass == M_EYESTOCK) ||(part->mObjClass == M_WARLORDTRAINING) ||(part->mObjClass == M_THRIPID)
				|| (part->mObjClass == M_MUTATIONCOLONY)|| (part->mObjClass == M_SENTINELTOWER)|| 
				(part->mObjClass == M_CITADEL)|| (part->mObjClass == M_HEAVYREFINERY)|| 
				(part->mObjClass == M_REFINERY)|| (part->mObjClass == M_COLLECTOR)||
				(part->mObjClass == M_SUPERHEAVYREFINERY)|| (part->mObjClass == M_GREATER_COLLECTOR)||
				(part->mObjClass == M_OXIDATOR) || (part->mObjClass == M_COALESCER) ||
				(part->mObjClass == M_GUDGEON) || (part->mObjClass == M_TEMPLE_OF_VYRIE) ||
				(part->mObjClass == M_OUTPOST)))
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

			wchar_t buffer[256];
			if(lastObjType != platform->mObjClass)
			{
				setPanelOwnership(false);
				setPanelOwnership(true);
			}

			//see if the system is in supply
			if(SECTOR->SystemInSupply(obj->GetSystemID(),obj->GetPlayerID()))
			{
				int i;
				for(i = 0; i <NUM_IND_BUTTONS; ++i)
				{
					if(buildButton[i] != 0)
						buildButton[i]->EnableButton(true);
				}
				for(i = 0; i <NUM_RESEARCH_BUTTONS; ++i)
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
				int i;
				for(i = 0; i <NUM_IND_BUTTONS; ++i)
				{
					if(buildButton[i] != 0)
						buildButton[i]->EnableButton(false);
				}
				for(i = 0; i <NUM_RESEARCH_BUTTONS; ++i)
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
				shipclass->SetText(buffer);
			}
			swprintf(buffer, _localLoadStringW(IDS_IND_HULL), (actor) ? actor->GetDisplayHullPoints() : platform->hullPoints, platform->hullPointsMax);
			hull->SetText(buffer);


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

			//update technode infomation
			TECHNODE newNode = MGlobals::GetCurrentTechLevel(MGlobals::GetThisPlayer());
			TECHNODE workingNode = MGlobals::GetWorkingTechLevel(MGlobals::GetThisPlayer());
			if(!(newNode.IsEqual(lastNode)) || !(workingNode.IsEqual(lastWorkingNode)) )
			{
				lastNode = newNode;
				lastWorkingNode = workingNode;
				int i;
				for(i = 0; i <NUM_IND_BUTTONS; ++i)
				{
					if(buildButton[i] != 0)
						buildButton[i]->SetTechNode(lastNode,MGlobals::GetTechAvailable(),workingNode);
				}
				for(i = 0; i <NUM_RESEARCH_BUTTONS; ++i)
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

			//if this is a refinery then show the harvest buttons

			if(noAuto != 0 && autoOre != 0 && autoGas != 0)
			{
				if(platform->mObjClass == M_REFINERY || platform->mObjClass == M_GREATER_COLLECTOR ||
					platform->mObjClass == M_COLLECTOR || platform->mObjClass == M_OXIDATOR ||
					platform->mObjClass == M_COALESCER)
				{
					noAuto->SetVisible(true);
					autoOre->SetVisible(true);
					autoGas->SetVisible(true);
					VOLPTR(IHarvestBuilder) harvest = obj;
					if(harvest)
					{
						switch(harvest->GetHarvestStance())
						{
						case HS_NO_STANCE:
							noAuto->SetHighlightState(true);
							autoOre->SetHighlightState(false);
							autoGas->SetHighlightState(false);					
							break;
						case HS_GAS_HARVEST:
							noAuto->SetHighlightState(false);
							autoOre->SetHighlightState(false);
							autoGas->SetHighlightState(true);					
							break;
						case HS_ORE_HARVEST:
							noAuto->SetHighlightState(false);
							autoOre->SetHighlightState(true);
							autoGas->SetHighlightState(false);					
							break;
						}
					}
				}
				else
				{
					noAuto->SetVisible(false);
					autoOre->SetVisible(false);
					autoGas->SetVisible(false);
				}
			}

			updateQueue();
		}
	}
	else
		timeoutCount -= dt;
}
//--------------------------------------------------------------------------//
//
void CMenu_ResBuild::enableMenu (bool bEnable)
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

void CMenu_ResBuild::setPanelOwnership (bool bOwn)
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
			lastObjType = part->mObjClass;
			char menuName[256];
			menuName[0] = 0;
			if(part->mObjClass == M_ACADEMY) 
				strcpy(menuName,"academy");
			else if(part->mObjClass == M_EYESTOCK) 
				strcpy(menuName,"M_EyeStock");
			else if(part->mObjClass == M_WARLORDTRAINING) 
				strcpy(menuName,"M_WarlordTraining");
			else if(part->mObjClass == M_THRIPID)
				strcpy(menuName,"M_Thripid");
			else if(part->mObjClass == M_MUTATIONCOLONY)
				strcpy(menuName,"M_MutationColony");
			else if(part->mObjClass == M_SENTINELTOWER)
				strcpy(menuName,"S_SentinalTower");
			else if(part->mObjClass == M_CITADEL)
				strcpy(menuName,"S_Citidel");
			else if(part->mObjClass == M_HEAVYREFINERY)
				strcpy(menuName,"T_HeavyRefinery");
			else if(part->mObjClass == M_SUPERHEAVYREFINERY)
				strcpy(menuName,"T_SuperHeavyRefinery");
			else if(part->mObjClass == M_REFINERY)
				strcpy(menuName,"refinery");
			else if(part->mObjClass == M_COLLECTOR)
				strcpy(menuName,"M_Collector");
			else if(part->mObjClass == M_GREATER_COLLECTOR)
				strcpy(menuName,"M_GreaterCollector");
			else if(part->mObjClass == M_OXIDATOR)
				strcpy(menuName,"S_Oxidator");
			else if(part->mObjClass == M_COALESCER)
				strcpy(menuName,"V_Coalescer");
			else if(part->mObjClass == M_GUDGEON)
				strcpy(menuName,"V_Gudgeon");
			else if(part->mObjClass == M_TEMPLE_OF_VYRIE)
				strcpy(menuName,"V_TempleOfVyrie");
			else if(part->mObjClass == M_OUTPOST)
				strcpy(menuName,"outpost");

			if (toolbar->GetToolbar(menuName, menu, part->race) == GR_OK)
			{
				COMPTR<IDAComponent> pComp;

				if (toolbar->GetControl("shipclass", pComp) == GR_OK)
					pComp->QueryInterface("IStatic", shipclass);
				if (menu->GetControl("hull", pComp) == GR_OK)
					pComp->QueryInterface("IStatic", hull);
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
				if (menu->GetControl("noAuto", pComp) == GR_OK)
					pComp->QueryInterface("IHotButton", noAuto);
				if (menu->GetControl("autoOre", pComp) == GR_OK)
					pComp->QueryInterface("IHotButton", autoOre);
				if (menu->GetControl("autoGas", pComp) == GR_OK)
					pComp->QueryInterface("IHotButton", autoGas);

				lastNode = MGlobals::GetCurrentTechLevel(MGlobals::GetThisPlayer());
				lastWorkingNode = MGlobals::GetWorkingTechLevel(MGlobals::GetThisPlayer());
				//get the build buttons
				for(U32 index = 0; index < NUM_IND_BUTTONS; ++index)
				{
					char buffer[32];
					sprintf(buffer,"build%d",index);
					if (menu->GetControl(buffer, pComp) == GR_OK)
						pComp->QueryInterface("IActiveButton", buildButton[index]);
					if(buildButton[index] != 0)
					{
						buildButton[index]->SetControlID(buildButton[index]->GetBuildArchetype());//hack;
						buildButton[index]->SetTechNode(lastNode,MGlobals::GetTechAvailable(),lastWorkingNode);
					}
				}
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

		menu.free();
		shipclass.free();
		hull.free();
		crewStorage.free();
		metalStorage.free();
		gasStorage.free();
		location.free();
		buildQueue.free();
		inSupply.free();
		notInSupply.free();
		disabledText.free();
		autoGas.free();
		autoOre.free();
		noAuto.free();
		for(U32 index = 0; index < NUM_IND_BUTTONS; ++index)
		{
			buildButton[index].free();
		}
		for(int i = 0; i <NUM_RESEARCH_BUTTONS; ++i)
		{
			research[i].free();
		}
	}
}

//--------------------------------------------------------------------------//
//
struct _cmenu_resbuild: GlobalComponent
{
	CMenu_ResBuild * menu;

	virtual void Startup (void)
	{
		menu = new DAComponent<CMenu_ResBuild>;
		AddToGlobalCleanupList(&menu);
	}

	virtual void Initialize (void)
	{
		COMPTR<IDAConnectionPoint> connection;

		if (TOOLBAR->QueryOutgoingInterface("IEventCallback", connection) == GR_OK)
			connection->Advise(menu->getBase(), &menu->eventHandle);
	}
};

static _cmenu_resbuild startup;

//--------------------------------------------------------------------------//
//-----------------------------End CMenu_ResBuild.cpp----------------------------//
//--------------------------------------------------------------------------//
