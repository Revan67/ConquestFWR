//--------------------------------------------------------------------------//
//                                                                          //
//                              CMenu_FindButtons.cpp                               //
//                                                                          //
//               COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
   $Header: /Conquest/App/Src/CMenu_FindButtons.cpp 41    9/19/01 9:54a Tmauer $
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
#include "Hotkeys.h"
#include "Sector.h"
#include "Camera.h"
#include "OpAgent.h"
#include "IIcon.h"
#include "IHarvest.h"
#include "IAdmiral.h"
#include <DMBaseData.h>
#include <DMTechNode.h>

#include <TComponent.h>
#include <IConnection.h>
#include <HeapObj.h>
#include <TSmartPointer.h>
#include <EventSys.h>

#include <stdio.h>

#define TIMEOUT_PERIOD 500
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//

struct DACOM_NO_VTABLE CMenu_FindButtons : public IEventCallback, IHotControlEvent
{
	BEGIN_DACOM_MAP_INBOUND(CMenu_FindButtons)
	DACOM_INTERFACE_ENTRY(IEventCallback)
	DACOM_INTERFACE_ENTRY(IHotControlEvent)
	END_DACOM_MAP()

	bool bHasFocus;
	U32 eventHandle, hotEventHandle;		// connection handle
	S32 timeoutCount;
	TECHNODE lastNode;
	U32 lastSelectedAdmiral;

	M_OBJCLASS lastRefineryFound;
	M_OBJCLASS lastIndustryFound;

	COMPTR<IHotButton> indButton,civButton,researchButton,fleetButton, 
		gotoButton,missionButton, diplomacyButton, chatButton;
	COMPTR<IIcon> inSupply, notInSupply;

	U32 lastMissionID;

	CMenu_FindButtons (void);

	~CMenu_FindButtons (void);

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
	}

	virtual void OnRightButtonEvent (U32 menuID, U32 controlID)
	{
		if(!controlID)
			return;
	}

	virtual void OnLeftDblButtonEvent (U32 menuID, U32 controlID)		// double-click
	{
	}
	
	/* CMenu_HeavyInd methods */

	void onUpdate (U32 dt);	// in mseconds

	void onHotkey (U32 key);

	void enableMenu (bool bEnable);

	IDAComponent * getBase (void)
	{
		return static_cast<IEventCallback *>(this);
	}

	M_OBJCLASS findNextRefineryType(M_OBJCLASS lastType);

	M_OBJCLASS findNextIndustryType(M_OBJCLASS lastType);

	IBaseObject * findFirstObject(M_OBJCLASS type);
};
//--------------------------------------------------------------------------//
//
CMenu_FindButtons::CMenu_FindButtons (void)
{
	lastSelectedAdmiral = 0;
	bHasFocus = false;
}
//--------------------------------------------------------------------------//
//
CMenu_FindButtons::~CMenu_FindButtons (void)
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
GENRESULT CMenu_FindButtons::Notify (U32 message, void *param)
{
//	MSG *msg = (MSG *) param;

	switch (message)
	{
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
		if ((bHasFocus || ( (TOOLBAR != 0) && (TOOLBAR->bInvisible) && (TOOLBAR->bHasFocus) ) )&& CQFLAGS.bGameActive   )
			onHotkey((U32)param);
		break;
	}

	return GR_OK;
}
//--------------------------------------------------------------------------//
//
M_OBJCLASS CMenu_FindButtons::findNextRefineryType(M_OBJCLASS lastType)
{
	switch(lastType)
	{
	case M_NONE:
	case M_REFINERY:
		return M_OUTPOST;
		break;
	case M_OUTPOST:
		return M_ACADEMY;
		break;
	case M_ACADEMY:
		return M_BALLISTICS;
		break;
	case M_BALLISTICS:
		return M_ADVHULL;
		break;
	case M_ADVHULL:
		return M_HEAVYREFINERY;
		break;
	case M_HEAVYREFINERY:
		return M_SUPERHEAVYREFINERY;
		break;
	case M_SUPERHEAVYREFINERY:
		return M_LRSENSOR;
		break;
	case M_LRSENSOR:
		return M_AWSLAB;
		break;
	case M_AWSLAB:
		return M_HANGER;
		break;
	case M_HANGER:
		return M_PROPLAB;
		break;
	case M_PROPLAB:
		return M_DISPLAB;
		break;
	case M_DISPLAB:
		return M_COLLECTOR;
		break;
	case M_COLLECTOR:
		return M_GREATER_COLLECTOR;
		break;
	case M_GREATER_COLLECTOR:
		return M_EYESTOCK;
		break;
	case M_EYESTOCK:
		return M_WARLORDTRAINING;
		break;
	case M_WARLORDTRAINING:
		return M_BLASTFURNACE;
		break;
	case M_BLASTFURNACE:
		return M_EXPLOSIVESRANGE;
		break;
	case M_EXPLOSIVESRANGE:
		return M_CARRIONROOST;
		break;
	case M_CARRIONROOST:
		return M_MUTATIONCOLONY;
		break;
	case M_MUTATIONCOLONY:
		return M_BIOFORGE;
		break;
	case M_BIOFORGE:
		return M_FUSIONMILL;
		break;
	case M_FUSIONMILL:
		return M_CARPACEPLANT;
		break;
	case M_CARPACEPLANT:
		return M_HYBRIDCENTER;
		break;
	case M_HYBRIDCENTER:
		return M_OXIDATOR;
		break;
	case M_OXIDATOR:
		return M_BUNKER;
		break;
	case M_BUNKER:
		return M_SENTINELTOWER;
		break;
	case M_SENTINELTOWER:
		return M_HELIONVEIL;
		break;
	case M_HELIONVEIL:
		return M_CITADEL;
		break;
	case M_CITADEL:
		return M_XENOCHAMBER;
		break;
	case M_XENOCHAMBER:
		return M_ANVIL;
		break;
	case M_ANVIL:
		return M_MUNITIONSANNEX;
		break;
	case M_MUNITIONSANNEX:
		return M_TURBINEDOCK;
		break;
	case M_TURBINEDOCK:
		return M_GUDGEON;
		break;
	case M_GUDGEON:
		return M_CLAW_OF_VYRIE;
		break;
	case M_CLAW_OF_VYRIE:
		return M_EYE_OF_VYRIE;
		break;
	case M_EYE_OF_VYRIE:
		return M_TEMPLE_OF_VYRIE;
		break;
	case M_TEMPLE_OF_VYRIE:
		return M_HAMMER_OF_VYRIE;
		break;
	case M_HAMMER_OF_VYRIE:	
		return M_REFINERY;
		break;
	default:
		return M_NONE;
	}
}
//--------------------------------------------------------------------------//
//
M_OBJCLASS CMenu_FindButtons::findNextIndustryType(M_OBJCLASS lastType)
{
	switch(lastType)
	{
	case M_NONE:
	case M_HQ:
		return M_LIGHTIND;
		break;
	case M_LIGHTIND:
		return M_HEAVYIND;
		break;
	case M_HEAVYIND:
		return M_COCOON;
		break;
	case M_COCOON:
		return M_THRIPID;
		break;
	case M_THRIPID:
		return M_NIAD;
		break;
	case M_NIAD:
		return M_ACROPOLIS;
		break;
	case M_ACROPOLIS:
		return M_PAVILION;
		break;
	case M_PAVILION:
		return M_GREATERPAVILION;
		break;
	case M_GREATERPAVILION:
		return M_LOCUS;
		break;
	case M_LOCUS:
		return M_COMPILER;
		break;
	case M_COMPILER:
		return M_FORMULATOR;
		break;
	case M_FORMULATOR:		
		return M_HQ;
		break;
	default:
		return M_NONE;
	}
}
//--------------------------------------------------------------------------//
//
IBaseObject * CMenu_FindButtons::findFirstObject(M_OBJCLASS type)
{
	IBaseObject * obj = OBJLIST->GetTargetList();
	IBaseObject * nonIdleObject = NULL;
	while(obj)
	{
		if(((obj->objClass == OC_PLATFORM) || (obj->objClass == OC_PLATFORM) ) && (obj->GetPlayerID() == MGlobals::GetThisPlayer()))
		{
			MPart nextPart(obj);
			if((nextPart->mObjClass == type) &&	(nextPart->bReady) && 
				(SECTOR->GetAlertState(obj->GetSystemID(),MGlobals::GetThisPlayer())&S_VISIBLE) )
			{
				if((!THEMATRIX->HasPendingOp(obj->GetPartID())) && SECTOR->SystemInSupply(obj->GetSystemID(),MGlobals::GetThisPlayer()))
				{
					return obj;
				}
				else if(!nonIdleObject)
				{
					nonIdleObject = obj;
				}
			}
		}
		obj = obj->nextTarget;
	}
	return nonIdleObject;
}
//--------------------------------------------------------------------------//
//
void CMenu_FindButtons::onHotkey(U32 key)
{
	switch(key)
	{
#if 0
	case IDH_NEXT_IDLE:
		{
			IBaseObject * first = OBJLIST->GetSelectedList();
			const U32 playerID = MGlobals::GetThisPlayer();
			if (first)
				first = first->nextTarget;
			if (first==0)
				first = OBJLIST->GetTargetList();
			if (first)
			{
				IBaseObject * obj = first;
				do
				{
					if (obj->GetPlayerID() == playerID)
					{
						if (THEMATRIX->HasPendingOp(obj->GetPartID()) == 0)
						{
							OBJLIST->FlushHighlightedList();
							OBJLIST->FlushSelectedList();
							OBJLIST->HighlightObject(obj);
							OBJLIST->SelectHighlightedObjects();
							OBJLIST->FlushHighlightedList();
							return;
						}
					}

					if ((obj = obj->nextTarget) == 0)
						obj = OBJLIST->GetTargetList();
				} while (obj != first);
			}
		}
		break;
	case IDH_PREV_IDLE:
		{
			IBaseObject * first = OBJLIST->GetSelectedList();
			const U32 playerID = MGlobals::GetThisPlayer();
			if (first)
				first = first->prevTarget;
			if (first==0)
			{
				if ((first = OBJLIST->GetTargetList()) != 0)
				{
					while(first->nextTarget)
						first = first->nextTarget;
				}
			}
			if (first)
			{
				IBaseObject * obj = first;
				do
				{
					if (obj->GetPlayerID() == playerID)
					{
						if (THEMATRIX->HasPendingOp(obj->GetPartID()) == 0)
						{
							OBJLIST->FlushHighlightedList();
							OBJLIST->FlushSelectedList();
							OBJLIST->HighlightObject(obj);
							OBJLIST->SelectHighlightedObjects();
							OBJLIST->FlushHighlightedList();
							return;
						}
					}

					if ((obj = obj->prevTarget) == 0)
					{
						if ((obj = OBJLIST->GetTargetList()) != 0)
						{
							while (obj->nextTarget)
								obj = obj->nextTarget;
						}
					}
				} while (obj != first);
			}
		}
		break;
#endif
/*	case IDH_QUIT_MISSION:
		{
			::PostMessage(hMainWindow,WM_CLOSE,0,0);
			break;
		}
	case IDH_CYCLE_SYSTEMS:
		{
			U32 currentSystem = SECTOR->GetCurrentSystem();
			U32 maxSys = SECTOR->GetNumSystems();
			U32 nextSys = (currentSystem%maxSys)+1;
			while(nextSys != currentSystem && (!(SECTOR->GetAlertState(nextSys,MGlobals::GetThisPlayer())&S_VISIBLE)))
			{
				nextSys = (nextSys%maxSys)+1;
			}
			if(nextSys != currentSystem)
				SECTOR->SetCurrentSystem(nextSys);
			break;
		}
*/	case IDH_NEXT_UNIT:
		{
			IBaseObject * obj = OBJLIST->GetSelectedList();
			if(obj && (!obj->nextSelected))
			{
				U32 archetypeID = ARCHLIST->GetArchetypeDataID(obj->pArchetype);
				while(1)//will always be true because it will always find the current selection
				{
					if(!(obj->nextTarget))
					{
						obj = OBJLIST->GetTargetList();
					}
					else
						obj = obj->nextTarget;
					MPart part(obj);
					if((obj->GetPlayerID() == MGlobals::GetThisPlayer()) && (ARCHLIST->GetArchetypeDataID(obj->pArchetype) == archetypeID) &&
						(part->bReady) && 
						(SECTOR->GetAlertState(obj->GetSystemID(),MGlobals::GetThisPlayer())&S_VISIBLE))
					{
						OBJLIST->FlushHighlightedList();
						OBJLIST->FlushSelectedList();
						OBJLIST->HighlightObject(obj);
						OBJLIST->SelectHighlightedObjects();
						OBJLIST->FlushHighlightedList();
						return;
					}
				}
			}
		}
		break;
	case IDH_PREV_UNIT:
		{
			IBaseObject * obj = OBJLIST->GetSelectedList();

			if(obj && (!obj->nextSelected))
			{
				U32 archetypeID = ARCHLIST->GetArchetypeDataID(obj->pArchetype);
				while(1)//will always be true because it will always find the current selection
				{
					if(!(obj->prevTarget))
					{
						while(obj->nextTarget)
							obj = obj->nextTarget;
					}
					else
						obj = obj->prevTarget;
					MPart part(obj);
					if((obj->GetPlayerID() == MGlobals::GetThisPlayer()) &&(ARCHLIST->GetArchetypeDataID(obj->pArchetype) == archetypeID)&&
						(part->bReady) && 
						(SECTOR->GetAlertState(obj->GetSystemID(),MGlobals::GetThisPlayer())&S_VISIBLE))
					{
						OBJLIST->FlushHighlightedList();
						OBJLIST->FlushSelectedList();
						OBJLIST->HighlightObject(obj);
						OBJLIST->SelectHighlightedObjects();
						OBJLIST->FlushHighlightedList();
						return;
					}
				}
			}
		}
		break;
	case IDH_CYCLE_FLEETS:
		{
			U32 nextFound = 0;
			IBaseObject * bestAdmiral = NULL;

			IBaseObject * obj = OBJLIST->GetTargetList();
			while(obj)
			{
				MPart part(obj);
				if(obj && obj->GetPlayerID() == MGlobals::GetThisPlayer()&&
						(part->bReady) && 
						(SECTOR->GetAlertState(obj->GetSystemID(),MGlobals::GetThisPlayer())&S_VISIBLE))
				{
					if(TESTADMIRAL(obj->GetPartID()) && (obj->objMapNode!=0))
					{
						OBJPTR<IAdmiral> admiral;
						obj->QueryInterface(IAdmiralID,admiral);
						U32 hotkey = admiral->GetAdmiralHotkey();
						if(hotkey <= lastSelectedAdmiral)
							hotkey += 6;
						if(nextFound)
						{
							if(hotkey < nextFound)
							{
								nextFound = hotkey;
								bestAdmiral = obj;
							}
						}
						else
						{
							nextFound = hotkey;
							bestAdmiral = obj;
						}	
					}
					else if(part.isValid())
					{
						if(part->admiralID)
						{
							OBJPTR<IAdmiral> admiral;
							OBJLIST->FindObject(part->admiralID,NONSYSVOLATILEPTR,admiral,IAdmiralID);
							if(admiral)
							{
								U32 hotkey = admiral->GetAdmiralHotkey();
								if(hotkey <= lastSelectedAdmiral)
									hotkey += 6;
								if(nextFound)
								{
									if(hotkey < nextFound)
									{
										nextFound = hotkey;
										bestAdmiral = obj;
									}
								}
								else
								{
									nextFound = hotkey;
									bestAdmiral = obj;
								}	
							}
						}
					}
				}
				obj = obj->nextTarget;
			}

			if(bestAdmiral)
			{
				lastSelectedAdmiral = ((nextFound-1)%6)+1;
				OBJLIST->FlushHighlightedList();
				OBJLIST->FlushSelectedList();
				OBJLIST->HighlightObject(bestAdmiral);
				OBJLIST->SelectHighlightedObjects();
				OBJLIST->FlushHighlightedList();
				const U32 systemID = bestAdmiral->GetSystemID();
				if (systemID <= MAX_SYSTEMS)
				{
					SECTOR->SetCurrentSystem(systemID);
					CAMERA->SetLookAtPosition(bestAdmiral->GetPosition());
				}
				return;
			}
		}
		break;
	case IDH_SELECT_IDLE_CIVILIAN:
		{
			IBaseObject * obj = OBJLIST->GetSelectedList();
			if(obj && (obj->objClass == OC_SPACESHIP))
			{
				MPart part(obj);
				if(((part->mObjClass == M_FABRICATOR) || (part->mObjClass == M_FORGER) ||
					(part->mObjClass == M_WEAVER) || (part->mObjClass == M_SIPHON) ||
					(part->mObjClass == M_HARVEST)|| (part->mObjClass == M_GALIOT) ||
					(part->mObjClass == M_SHAPER)|| (part->mObjClass == M_AGGREGATOR))
					&& !(THEMATRIX->HasPendingOp(obj->GetPartID())))
				{
					bool bHarvestIdle = true;
					OBJPTR<IHarvest> harvest;
					obj->QueryInterface(IHarvestID,harvest);
					if(harvest)
						bHarvestIdle = harvest->IsIdle();
					if(bHarvestIdle)
					{
						U32 playerID = obj->GetPlayerID();
						while(1)//will always be true because it will always find the current selection
						{
							if(!(obj->nextTarget))
							{
								obj = OBJLIST->GetTargetList();
							}
							else
								obj = obj->nextTarget;
							if((obj->objClass == OC_SPACESHIP) && (obj->GetPlayerID() == playerID)&& 
								(SECTOR->GetAlertState(obj->GetSystemID(),MGlobals::GetThisPlayer())&S_VISIBLE))
							{
								MPart nextPart(obj);
								if(((nextPart->mObjClass == M_FABRICATOR) || (nextPart->mObjClass == M_FORGER) ||
									(nextPart->mObjClass == M_WEAVER) || (nextPart->mObjClass == M_SIPHON) ||
									(nextPart->mObjClass == M_HARVEST)|| (nextPart->mObjClass == M_GALIOT)||
									(nextPart->mObjClass == M_SHAPER)|| (nextPart->mObjClass == M_AGGREGATOR)) 
									&& (!THEMATRIX->HasPendingOp(obj->GetPartID())) &&
									(nextPart->bReady))
								{
									bHarvestIdle = true;
									obj->QueryInterface(IHarvestID,harvest);
									if(harvest)
										bHarvestIdle = harvest->IsIdle();
									if(bHarvestIdle)
									{
										OBJLIST->FlushHighlightedList();
										OBJLIST->FlushSelectedList();
										OBJLIST->HighlightObject(obj);
										OBJLIST->SelectHighlightedObjects();
										OBJLIST->FlushHighlightedList();
										const U32 systemID = obj->GetSystemID();
										if (systemID <= MAX_SYSTEMS)
										{
											SECTOR->SetCurrentSystem(systemID);
											CAMERA->SetLookAtPosition(obj->GetPosition());
										}
										return;
									}
								}
							}
						}
					}
				} 
			}
			obj = OBJLIST->GetTargetList();
			while(obj)
			{
				if(obj && obj->objClass == OC_SPACESHIP && obj->GetPlayerID() == MGlobals::GetThisPlayer() && 
					(SECTOR->GetAlertState(obj->GetSystemID(),MGlobals::GetThisPlayer())&S_VISIBLE))
				{
					MPart nextPart(obj);
					if(((nextPart->mObjClass == M_FABRICATOR) || (nextPart->mObjClass == M_FORGER) ||
						(nextPart->mObjClass == M_WEAVER) || (nextPart->mObjClass == M_SIPHON) ||
						(nextPart->mObjClass == M_HARVEST)|| (nextPart->mObjClass == M_GALIOT)||
						(nextPart->mObjClass == M_SHAPER)|| (nextPart->mObjClass == M_AGGREGATOR)) &&
						(!THEMATRIX->HasPendingOp(obj->GetPartID())) &&
						(nextPart->bReady))
					{
						bool bHarvestIdle = true;
						OBJPTR<IHarvest> harvest;
						obj->QueryInterface(IHarvestID,harvest);
						if(harvest)
							bHarvestIdle = harvest->IsIdle();
						if(bHarvestIdle)
						{
							OBJLIST->FlushHighlightedList();
							OBJLIST->FlushSelectedList();
							OBJLIST->HighlightObject(obj);
							OBJLIST->SelectHighlightedObjects();
							OBJLIST->FlushHighlightedList();
							const U32 systemID = obj->GetSystemID();
							if (systemID <= MAX_SYSTEMS)
							{
								SECTOR->SetCurrentSystem(systemID);
								CAMERA->SetLookAtPosition(obj->GetPosition());
							}
							return;
						}
					}
				}
				obj = obj->nextTarget;
			}
		}
		break;
	case IDH_FABRICATOR:
		{
			IBaseObject * obj = OBJLIST->GetSelectedList();
			if(obj && (obj->objClass == OC_SPACESHIP) && 
				(SECTOR->GetAlertState(obj->GetSystemID(),MGlobals::GetThisPlayer())&S_VISIBLE))
			{
				MPart part(obj);
				if((part->mObjClass == M_FABRICATOR) || (part->mObjClass == M_FORGER) ||
					(part->mObjClass == M_WEAVER) || (part->mObjClass == M_SHAPER))
				{
					U32 playerID = obj->GetPlayerID();
					while(1)//will always be true because it will always find the current selection
					{
						if(!(obj->nextTarget))
						{
							obj = OBJLIST->GetTargetList();
						}
						else
							obj = obj->nextTarget;
						if((obj->objClass == OC_SPACESHIP) && (obj->GetPlayerID() == playerID)&& 
							(SECTOR->GetAlertState(obj->GetSystemID(),MGlobals::GetThisPlayer())&S_VISIBLE))
						{
							MPart nextPart(obj);
							if(((nextPart->mObjClass == M_FABRICATOR) || (nextPart->mObjClass == M_FORGER) ||
								(nextPart->mObjClass == M_WEAVER)|| (nextPart->mObjClass == M_SHAPER)  ) &&
								(nextPart->bReady))
							{
								OBJLIST->FlushHighlightedList();
								OBJLIST->FlushSelectedList();
								OBJLIST->HighlightObject(obj);
								OBJLIST->SelectHighlightedObjects();
								OBJLIST->FlushHighlightedList();
								const U32 systemID = obj->GetSystemID();
								if (systemID <= MAX_SYSTEMS)
								{
									SECTOR->SetCurrentSystem(systemID);
									CAMERA->SetLookAtPosition(obj->GetPosition());
								}
								return;
							}
						}
					}
				} 
			}
			obj = OBJLIST->GetTargetList();
			while(obj)
			{
				if(obj && obj->objClass == OC_SPACESHIP && obj->GetPlayerID() == MGlobals::GetThisPlayer() && 
					(SECTOR->GetAlertState(obj->GetSystemID(),MGlobals::GetThisPlayer())&S_VISIBLE))
				{
					MPart nextPart(obj);
					if(((nextPart->mObjClass == M_FABRICATOR) || (nextPart->mObjClass == M_FORGER) ||
						(nextPart->mObjClass == M_WEAVER) || (nextPart->mObjClass == M_SHAPER)) &&
						(nextPart->bReady))
					{
						OBJLIST->FlushHighlightedList();
						OBJLIST->FlushSelectedList();
						OBJLIST->HighlightObject(obj);
						OBJLIST->SelectHighlightedObjects();
						OBJLIST->FlushHighlightedList();
						const U32 systemID = obj->GetSystemID();
						if (systemID <= MAX_SYSTEMS)
						{
							SECTOR->SetCurrentSystem(systemID);
							CAMERA->SetLookAtPosition(obj->GetPosition());
						}
						return;
					}
				}
				obj = obj->nextTarget;
			}
		}
		break;
	case IDH_RESEARCH_PLAT:
		{
			IBaseObject * obj = NULL;
			M_OBJCLASS last = lastRefineryFound;
			while(lastRefineryFound != (last=findNextRefineryType(last)))
			{
				if(last == M_NONE)
					return;
				if(lastRefineryFound == M_NONE)
					lastRefineryFound = last;
				obj = findFirstObject(last);
				if(obj)
				{
					OBJLIST->FlushHighlightedList();
					OBJLIST->FlushSelectedList();
					OBJLIST->HighlightObject(obj);
					OBJLIST->SelectHighlightedObjects();
					OBJLIST->FlushHighlightedList();
					lastRefineryFound = last;
					return;
				}
			}
			lastRefineryFound = M_NONE;
		}
		break;
	case IDH_SELECT_IDLE_MILITARY:
		{
			IBaseObject * first = OBJLIST->GetSelectedList();
			const U32 playerID = MGlobals::GetThisPlayer();
			if (first)
				first = first->nextTarget;
			if (first==0)
				first = OBJLIST->GetTargetList();
			if (first)
			{
				IBaseObject * obj = first;
				do
				{
					if (obj->GetPlayerID() == playerID && 
						(SECTOR->GetAlertState(obj->GetSystemID(),MGlobals::GetThisPlayer())&S_VISIBLE))
					{
						MPart part(obj);
						if (part.isValid() && (MGlobals::IsMilitaryShip(part->mObjClass) ||
								(part->mObjClass == M_SUPPLY)|| (part->mObjClass == M_ZORAP) ||
								(part->mObjClass == M_STRATUM)))
						{
							if (THEMATRIX->HasPendingOp(obj->GetPartID()) == 0)
							{
								OBJLIST->FlushHighlightedList();
								OBJLIST->FlushSelectedList();
								OBJLIST->HighlightObject(obj);
								OBJLIST->SelectHighlightedObjects();
								OBJLIST->FlushHighlightedList();
								return;
							}
						}
					}

					if ((obj = obj->nextTarget) == 0)
						obj = OBJLIST->GetTargetList();
				} while (obj != first);
			}
		}
		break;
	case IDH_BUILD_PLATS:
		{
			IBaseObject * obj = NULL;
			M_OBJCLASS last = lastIndustryFound;
			while(lastIndustryFound != (last=findNextIndustryType(last)))
			{
				if(last == M_NONE)
					return;
				if(lastIndustryFound == M_NONE)
					lastIndustryFound = last;
				obj = findFirstObject(last);
				if(obj)
				{
					OBJLIST->FlushHighlightedList();
					OBJLIST->FlushSelectedList();
					OBJLIST->HighlightObject(obj);
					OBJLIST->SelectHighlightedObjects();
					OBJLIST->FlushHighlightedList();
					lastIndustryFound = last;
					return;
				}
			}
			lastIndustryFound = M_NONE;
		}
		break;
	}
}

//--------------------------------------------------------------------------//
//
void CMenu_FindButtons::onUpdate (U32 dt)
{
	if(SECTOR->SystemInSupply(SECTOR->GetCurrentSystem(),MGlobals::GetThisPlayer()))
	{
		if(inSupply)
			inSupply->SetVisible(true);
		if(notInSupply)
			notInSupply->SetVisible(false);
	}
	else
	{
		if(notInSupply)
			notInSupply->SetVisible(true);
		if(inSupply)
			inSupply->SetVisible(false);
	}
/*	if (S32(timeoutCount-dt) <= 0)
	{
		TECHNODE curTech = MGlobals::GetCurrentTechLevel(MGlobals::GetThisPlayer());
		if(curTech.HasSomeTech(M_TERRAN,0,(TECHTREE::TDEPEND_LIGHT_IND | TECHTREE::TDEPEND_HEAVY_IND |
			TECHTREE::TDEPEND_TECH_IND | TECHTREE::TDEPEND_HEADQUARTERS),0,0))
		{
			if(indButton)
				indButton->EnableButton(true);
		}
		else
		{
			if(indButton)
				indButton->EnableButton(false);
		}
		if(curTech.HasSomeTech(M_TERRAN,0,TECHTREE::RESERVED_FABRICATOR,0,0))
		{
			if(fabButton)
				fabButton->EnableButton(true);
		}
		else
		{
			if(fabButton)
				fabButton->EnableButton(false);
		}
		if(curTech.HasSomeTech(M_TERRAN,0,(TECHTREE::TDEPEND_PROPLAB |
			TECHTREE::TDEPEND_AWSLAB),0,0))
		{
			if(researchButton)
				researchButton->EnableButton(true);
		}
		else
		{
			if(researchButton)
				researchButton->EnableButton(false);
		}
		if(curTech.HasSomeTech(M_TERRAN,0, TECHTREE::RESERVED_ADMIRAL,0,0))
		{
			if(fleetButton)
				fleetButton->EnableButton(true);
		}
		else
		{
			if(fleetButton)
				fleetButton->EnableButton(false);
		}
		IBaseObject * tester = OBJLIST->GetSelectedList();
		if(tester)
		{
			if(gotoButton)
				gotoButton->EnableButton(true);
			if(!(tester->nextSelected))
			{
				if(next)
					next->EnableButton(true);
				if(prev)
					prev->EnableButton(true);
			}
			else
			{
				if(next)
					next->EnableButton(false);
				if(prev)
					prev->EnableButton(false);
			}
		}
		else
		{
			if(gotoButton)
				gotoButton->EnableButton(false);
			if(next)
				next->EnableButton(false);
			if(prev)
				prev->EnableButton(false);
		}
		timeoutCount = TIMEOUT_PERIOD;

	}
	else
		timeoutCount -= dt;*/
}
//--------------------------------------------------------------------------//
//
void CMenu_FindButtons::enableMenu (bool bEnable)
{
	if(bEnable)
	{
		COMPTR<IToolbar> toolbar;

		if (TOOLBAR->QueryInterface("IToolbar", toolbar) == GR_OK)
		{
			COMPTR<IDAComponent> pComp;
			if (toolbar->GetControl("hpIndustrial", pComp) == GR_OK)
				pComp->QueryInterface("IHotButton", indButton);
			if (toolbar->GetControl("hpIdleCivilian", pComp) == GR_OK)
				pComp->QueryInterface("IHotButton", civButton);
			if (toolbar->GetControl("hpResearch", pComp) == GR_OK)
				pComp->QueryInterface("IHotButton", researchButton);
			if (toolbar->GetControl("hpFleetOfficer", pComp) == GR_OK)
				pComp->QueryInterface("IHotButton", fleetButton);
			if (toolbar->GetControl("go", pComp) == GR_OK)
				pComp->QueryInterface("IHotButton", gotoButton);
			if (toolbar->GetControl("inSupply", pComp) == GR_OK)
				pComp->QueryInterface("IIcon", inSupply);
			if (toolbar->GetControl("notInSupply", pComp) == GR_OK)
				pComp->QueryInterface("IIcon", notInSupply);
			if (toolbar->GetControl("missionObjectives", pComp) == GR_OK)
				pComp->QueryInterface("IHotButton", missionButton);
			if (toolbar->GetControl("hpDiplomacy", pComp) == GR_OK)
				pComp->QueryInterface("IHotButton", diplomacyButton);
			if (toolbar->GetControl("chat", pComp) == GR_OK)
				pComp->QueryInterface("IHotButton", chatButton);
			
			COMPTR<IDAConnectionPoint> connection;

			if (TOOLBAR->QueryOutgoingInterface("IHotControlEvent", connection) == GR_OK)
				connection->Advise(getBase(), &hotEventHandle);
		}
		lastRefineryFound = M_NONE;
		lastIndustryFound = M_NONE;

		if(missionButton)
		{
			missionButton->EnableButton(MISSION->IsSinglePlayerGame());
		}
		if(diplomacyButton)
		{
			diplomacyButton->EnableButton(MISSION->IsSinglePlayerGame() == false);
		}
		if(chatButton)
		{
			chatButton->EnableButton(MISSION->IsSinglePlayerGame() == false);
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
		indButton.free();
		civButton.free();
		researchButton.free();
		fleetButton.free();
		gotoButton.free();
		inSupply.free();
		notInSupply.free();
		missionButton.free();
		diplomacyButton.free();
		chatButton.free();
	}
}

//--------------------------------------------------------------------------//
//
struct _cmenu_findbuttons: GlobalComponent
{
	CMenu_FindButtons * menu;

	virtual void Startup (void)
	{
		menu = new DAComponent<CMenu_FindButtons>;
		AddToGlobalCleanupList(&menu);
	}

	virtual void Initialize (void)
	{
		COMPTR<IDAConnectionPoint> connection;

		if (TOOLBAR->QueryOutgoingInterface("IEventCallback", connection) == GR_OK)
			connection->Advise(menu->getBase(), &menu->eventHandle);
	}
};

static _cmenu_findbuttons startup;

//--------------------------------------------------------------------------//
//-----------------------------End CMenu_Ind.cpp----------------------------//
//--------------------------------------------------------------------------//
