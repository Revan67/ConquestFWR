//--------------------------------------------------------------------------//
//                                                                          //
//                              CMenu_BuildInd.cpp                               //
//                                                                          //
//               COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
   $Header: /Conquest/App/Src/CMenu_BuildInd.cpp 33    9/19/01 9:54a Tmauer $
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
#include "IQueueControl.h"
#include "MGlobals.h"
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
#define NUM_LIND_BUTTONS 10
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//

struct DACOM_NO_VTABLE CMenu_BuildInd : public IEventCallback, IHotControlEvent
{
	BEGIN_DACOM_MAP_INBOUND(CMenu_BuildInd)
	DACOM_INTERFACE_ENTRY(IEventCallback)
	DACOM_INTERFACE_ENTRY(IHotControlEvent)
	END_DACOM_MAP()

	U32 eventHandle, hotEventHandle;		// connection handle
	bool bPanelOwned;
	S32 timeoutCount;
	TECHNODE lastNode;

	COMPTR<IToolbar> menu;
	COMPTR<IStatic> shipClass;
	COMPTR<IStatic> hull,metalStorage,gasStorage,crewStorage,location,disabledText;
	COMPTR<IActiveButton> buildButton[NUM_LIND_BUTTONS];
	COMPTR<IQueueControl> buildQueue;
	COMPTR<IIcon> inSupply,notInSupply;

	U32 lastMissionID;
	S32 lastObjType;
	bool bHasFocus;

	CMenu_BuildInd (void);

	~CMenu_BuildInd (void);

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
		for(int index = 0; index < NUM_LIND_BUTTONS; ++index)
		{
			if((buildButton[index] != 0) && (buildButton[index]->GetControlID() == controlID))
			{
				IBaseObject * obj = OBJLIST->GetSelectedList();
				MPart part(obj);
				if (part.isValid())
				{
					if((lastObjType == part->mObjClass) && (obj->objClass == OC_PLATFORM) && ((part->mObjClass == M_LIGHTIND) ||
						(part->mObjClass == M_HEAVYIND) || (part->mObjClass == M_HQ) ||
						(part->mObjClass == M_COCOON) || (part->mObjClass == M_NIAD) || (part->mObjClass == M_PAVILION) ||
						(part->mObjClass == M_GREATERPAVILION) || (part->mObjClass == M_ACROPOLIS) ||
						(part->mObjClass == M_LOCUS) || (part->mObjClass == M_COMPILER) ||
						(part->mObjClass == M_FORMULATOR)))
					{
						USR_PACKET<USRBUILD> packet;
						packet.cmd = USRBUILD::ADD;
						packet.dwArchetypeID = buildButton[index]->GetBuildArchetype();
						packet.objectID[0] = obj->GetPartID();
						packet.init(1, false);
						NETPACKET->Send(HOSTID,0,&packet);
	//					buildButton[index]->UpdateBuild(0.0);
					}				
				}
			}
		}
	}

	virtual void OnRightButtonEvent (U32 menuID, U32 controlID)
	{
		if(!controlID)
			return;
/*		for(int index = 0; index < NUM_LIND_BUTTONS; ++index)
		{
			if((buildButton[index] != 0) && (buildButton[index]->GetControlID() == controlID))
			{
				IBaseObject * obj = OBJLIST->GetSelectedList();
				MPart part(obj);
				if((lastObjType == part->mObjClass) &&(obj->objClass == OC_PLATFORM) && ((part->mObjClass == M_LIGHTIND) ||
					(part->mObjClass == M_HEAVYIND) || (part->mObjClass == M_HQ)||
					(part->mObjClass == M_COCOON) || (part->mObjClass == M_NIAD)|| (part->mObjClass == M_PAVILION) ||
					(part->mObjClass == M_GREATERPAVILION)|| (part->mObjClass == M_ACROPOLIS)))
				{
					USR_PACKET<USRBUILD> packet;
					packet.init(1);
					packet.cmd = USRBUILD::REMOVE;
					packet.dwArchetypeID = buildButton[index]->GetBuildArchetype();
					packet.objectID[0] = obj->GetPartID();
					NETPACKET->Send(HOSTID,0,&packet);
				}				
			}
		}
*/
	}

	virtual void OnLeftDblButtonEvent (U32 menuID, U32 controlID)		// double-click
	{
	}
	
	/* CMenu_BuildInd methods */

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
CMenu_BuildInd::CMenu_BuildInd (void)
{
		// nothing to init so far?
}
//--------------------------------------------------------------------------//
//
CMenu_BuildInd::~CMenu_BuildInd (void)
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
GENRESULT CMenu_BuildInd::Notify (U32 message, void *param)
{
//	MSG *msg = (MSG *) param;

	switch (message)
	{
	case CQE_PANEL_OWNED:
		if (((CMenu_BuildInd *)param) != this)
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
					if(((part->mObjClass == M_LIGHTIND) || (part->mObjClass == M_HEAVYIND) || (part->mObjClass == M_HQ)||
								(part->mObjClass == M_COCOON)|| (part->mObjClass == M_NIAD)|| (part->mObjClass == M_PAVILION) ||
								(part->mObjClass == M_GREATERPAVILION)|| (part->mObjClass == M_ACROPOLIS) || 
								(part->mObjClass == M_LOCUS) || (part->mObjClass == M_COMPILER) ||
								(part->mObjClass == M_FORMULATOR)))
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
void CMenu_BuildInd::handleHotkey (S32 hotkey)
{
	if(bPanelOwned && hotkey >= IDH_CONTEXT_A && hotkey <= IDH_CONTEXT_Z)
	{
		for(int index = 0; index < NUM_LIND_BUTTONS; ++index)
		{
			if((buildButton[index] != 0) && (buildButton[index]->IsActive()) && (buildButton[index]->GetHotkey() == hotkey))
			{
				IBaseObject * obj = OBJLIST->GetSelectedList();
				MPart part(obj);
				if (part.isValid())
				{
					if((lastObjType == part->mObjClass) && (obj->objClass == OC_PLATFORM) && ((part->mObjClass == M_LIGHTIND) ||
						(part->mObjClass == M_HEAVYIND) || (part->mObjClass == M_HQ) ||
						(part->mObjClass == M_COCOON) || (part->mObjClass == M_NIAD) || (part->mObjClass == M_PAVILION) ||
						(part->mObjClass == M_GREATERPAVILION) || (part->mObjClass == M_ACROPOLIS) ||
						(part->mObjClass == M_LOCUS) || (part->mObjClass == M_COMPILER) ||
						(part->mObjClass == M_FORMULATOR)))
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
		}
	}
}
//--------------------------------------------------------------------------//
//
void CMenu_BuildInd::updateQueue ()
{
	buildQueue->ResetQueue();
	IBaseObject * obj = OBJLIST->GetSelectedList();
	if (obj && obj->nextSelected==0 && obj->objClass == OC_PLATFORM)
	{
		MPart part = obj;
		if(((part->mObjClass == M_LIGHTIND) || (part->mObjClass == M_HEAVYIND) || (part->mObjClass == M_HQ)||
					(part->mObjClass == M_COCOON)|| (part->mObjClass == M_NIAD)|| (part->mObjClass == M_PAVILION) ||
					(part->mObjClass == M_GREATERPAVILION)|| (part->mObjClass == M_ACROPOLIS) ||
					(part->mObjClass == M_LOCUS) || (part->mObjClass == M_COMPILER) ||
					(part->mObjClass == M_FORMULATOR)))
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
					U32 stallType;
					SINGLE progress = fab->FabGetDisplayProgress(stallType);
					buildQueue->SetPercentage(progress,stallType);
					for(U32 i = 0; i < numInQueue; ++i)
					{
						IDrawAgent * queueShape[GTHBSHP_MAX_SHAPES];
						queueShape[0] = 0;
						for(U32 j = 0; j <NUM_LIND_BUTTONS;++j)
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
void CMenu_BuildInd::onUpdate (U32 dt)
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
			if(((part->mObjClass == M_LIGHTIND) || (part->mObjClass == M_HEAVYIND) || (part->mObjClass == M_HQ)||
					(part->mObjClass == M_COCOON) || (part->mObjClass == M_NIAD)|| (part->mObjClass == M_PAVILION) ||
					(part->mObjClass == M_GREATERPAVILION)|| (part->mObjClass == M_ACROPOLIS) ||
					(part->mObjClass == M_LOCUS) || (part->mObjClass == M_COMPILER) ||
					(part->mObjClass == M_FORMULATOR)))
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
				for(int i = 0; i <NUM_LIND_BUTTONS; ++i)
				{
					if(buildButton[i] != 0)
						buildButton[i]->EnableButton(true);
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
				for(int i = 0; i <NUM_LIND_BUTTONS; ++i)
				{
					if(buildButton[i] != 0)
						buildButton[i]->EnableButton(false);
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
				wchar_t name[128];
				wchar_t * namePtr;

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
				shipClass->SetText(buffer);
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
			if(!(newNode.IsEqual(lastNode)))
			{
				lastNode = newNode;
				for(int i = 0; i <NUM_LIND_BUTTONS; ++i)
				{
					if(buildButton[i] != 0)
						buildButton[i]->SetTechNode(lastNode,MGlobals::GetTechAvailable(),workingNode);
				}
			}
			//update queue and prograss bar
/*			OBJPTR<IBuildQueue> fab;
			obj->QueryInterface(IBuildQueueID,fab);
			if(fab)
			{
				U32 stallType;
				SINGLE progress = fab->FabGetDisplayProgress(stallType);
				U32 job = fab->GetFabJobID();
				for(int i = 0; i <NUM_LIND_BUTTONS; ++i)
				{
					if(buildButton[i] != 0)
					{
						buildButton[i]->SetQueueNumber(fab->GetNumInQueue(buildButton[i]->GetBuildArchetype()));
						if(job && (buildButton[i]->GetBuildArchetype() == job))
						{
							buildButton[i]->UpdateBuild(progress,stallType);
						}
						else
							buildButton[i]->ResetActiveButton();
					}
				}
			}
*/			updateQueue();
		}
	}
	else
		timeoutCount -= dt;
}
//--------------------------------------------------------------------------//
//
void CMenu_BuildInd::enableMenu (bool bEnable)
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

void CMenu_BuildInd::setPanelOwnership (bool bOwn)
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
			if(part->mObjClass == M_LIGHTIND) 
				strcpy(menuName,"lindustrial");
			else if(part->mObjClass == M_HEAVYIND)
				strcpy(menuName,"hindustrial");
			else if(part->mObjClass == M_HQ)
				strcpy(menuName,"hq");
			else if(part->mObjClass == M_COCOON)
				strcpy(menuName,"M_Cocoon");
			else if(part->mObjClass == M_NIAD)
				strcpy(menuName,"M_Niad");
			else if(part->mObjClass == M_PAVILION)
				strcpy(menuName,"S_Pavilion");
			else if(part->mObjClass == M_GREATERPAVILION)
				strcpy(menuName,"S_GreaterPavilion");
			else if(part->mObjClass == M_ACROPOLIS)
				strcpy(menuName,"S_Acropolis");
			else if(part->mObjClass == M_LOCUS)
				strcpy(menuName,"V_Locus");
			else if(part->mObjClass == M_COMPILER)
				strcpy(menuName,"V_Compiler");
			else if(part->mObjClass == M_FORMULATOR)
				strcpy(menuName,"V_Formulator");
			if (toolbar->GetToolbar(menuName, menu, part->race) == GR_OK)
			{
				COMPTR<IDAComponent> pComp;

				if (toolbar->GetControl("shipclass", pComp) == GR_OK)
					pComp->QueryInterface("IStatic", shipClass);
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
				if (menu->GetControl("disabledText", pComp) == GR_OK)
					pComp->QueryInterface("IStatic", disabledText);
				if (menu->GetControl("inSupply", pComp) == GR_OK)
					pComp->QueryInterface("IIcon", inSupply);
				if (menu->GetControl("notInSupply", pComp) == GR_OK)
					pComp->QueryInterface("IIcon", notInSupply);
				if (menu->GetControl("buildQueue", pComp) == GR_OK)
					pComp->QueryInterface("IQueueControl", buildQueue);

				lastNode = MGlobals::GetCurrentTechLevel(MGlobals::GetThisPlayer());
				TECHNODE workingNode = MGlobals::GetWorkingTechLevel(MGlobals::GetThisPlayer());
				//get the build buttons
				for(U32 index = 0; index < NUM_LIND_BUTTONS; ++index)
				{
					char buffer[32];
					sprintf(buffer,"build%d",index);
					if (menu->GetControl(buffer, pComp) == GR_OK)
						pComp->QueryInterface("IActiveButton", buildButton[index]);
					if(buildButton[index] != 0)
					{
						buildButton[index]->SetControlID(buildButton[index]->GetBuildArchetype());//hack;
						buildButton[index]->SetTechNode(lastNode,MGlobals::GetTechAvailable(),workingNode);
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
		shipClass.free();
		hull.free();
		crewStorage.free();
		metalStorage.free();
		gasStorage.free();
		location.free();
		buildQueue.free();
		inSupply.free();
		notInSupply.free();
		disabledText.free();
		for(U32 index = 0; index < NUM_LIND_BUTTONS; ++index)
		{
			buildButton[index].free();
		}
	}
}

//--------------------------------------------------------------------------//
//
struct _cmenu_buildind: GlobalComponent
{
	CMenu_BuildInd * menu;

	virtual void Startup (void)
	{
		menu = new DAComponent<CMenu_BuildInd>;
		AddToGlobalCleanupList(&menu);
	}

	virtual void Initialize (void)
	{
		COMPTR<IDAConnectionPoint> connection;

		if (TOOLBAR->QueryOutgoingInterface("IEventCallback", connection) == GR_OK)
			connection->Advise(menu->getBase(), &menu->eventHandle);
	}
};

static _cmenu_buildind startup;

//--------------------------------------------------------------------------//
//-----------------------------End CMenu_Ind.cpp----------------------------//
//--------------------------------------------------------------------------//
