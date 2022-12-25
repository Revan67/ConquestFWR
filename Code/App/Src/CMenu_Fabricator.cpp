//--------------------------------------------------------------------------//
//                                                                          //
//                              CMenu_Fabricator.cpp                               //
//                                                                          //
//               COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
   $Header: /Conquest/App/Src/CMenu_Fabricator.cpp 43    10/09/01 2:00p Tmauer $
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
#include "IFabricator.h"
#include "MGlobals.h"
#include "DMTechNode.h"
#include "CommPacket.h"
#include "IHotStatic.h"
#include "ITabControl.h"
#include "IMissionActor.h"
#include "DHotButtonText.h"
#include "Hotkeys.h"
#include "IBanker.h"
#include <DMBaseData.h>
#include <DPlatform.h>

#include <TComponent.h>
#include <IConnection.h>
#include <HeapObj.h>
#include <TSmartPointer.h>
#include <EventSys.h>

#include <stdio.h>

#define TIMEOUT_PERIOD 500
#define NUM_FAB_BUTTONS 64
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//

U32 GetArmorUpgrades(M_RACE race);
U32 GetShieldUpgrades(M_RACE race);
U32 GetEngineUpgrades(M_RACE race);
U32 GetSensorUpgrades(M_RACE race);

struct DACOM_NO_VTABLE CMenu_Fabricator : public IEventCallback, IHotControlEvent
{
	BEGIN_DACOM_MAP_INBOUND(CMenu_Fabricator)
	DACOM_INTERFACE_ENTRY(IEventCallback)
	DACOM_INTERFACE_ENTRY(IHotControlEvent)
	END_DACOM_MAP()

	U32 eventHandle, hotEventHandle;		// connection handle
	bool bPanelOwned;
	S32 timeoutCount;
	TECHNODE lastNode,lastTechAviail;

	COMPTR<IToolbar> menu;
	COMPTR<IEdit2> namearea;
	COMPTR<IStatic> shipclass,hull;
	COMPTR<IActiveButton> buildButton[NUM_FAB_BUTTONS];
	COMPTR<IHotStatic> techarmor,techengine,techsheild,techsensors;
	COMPTR<ITabControl> fabTab;

	U32 lastMissionID;
	U32 lastArchID;
	S32 lastObjType;
	bool bHasFocus;

	CMenu_Fabricator (void);

	~CMenu_Fabricator (void);

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
/*		if(namearea)//get the new name for the ship
		{
			if(namearea->GetControlID() == controlID)
			{
				IBaseObject * obj = OBJLIST->GetSelectedList();
				if (obj && obj->nextSelected==0)
				{
					MPartNC ship = obj;

					if (ship.isValid())
					{
						wchar_t buffer[32];
						namearea->GetText(buffer,sizeof(buffer));
						if(buffer[0] != '\0')
						{						
							int size = sizeof(ship->partName.string);
							char * bufPtr = ship->partName.string;
							if(ship->partName[0] == '#')
							{
								do
								{
									++bufPtr;
									--size;
								}while(*bufPtr != '#');
								++bufPtr;
								--size;
							}
							_localWideToAnsi(buffer,bufPtr,size);
						}
						else
						{
							_localAnsiToWide(ship->partName,buffer,sizeof(buffer));
							wchar_t * namePtr = buffer;
							if(namePtr[0] == '#')
							{
								++namePtr;
								namePtr = wcschr(namePtr,'#');
								++namePtr;
							}
							namearea->SetText(namePtr);
						}

					}
				}
			}
		}
*/		for(int index = 0; index < NUM_FAB_BUTTONS; ++index)
		{
			if((buildButton[index] != 0) && (buildButton[index]->GetControlID() == controlID))
			{
				IBaseObject * obj = OBJLIST->GetSelectedList();
				if(obj && (obj->objClass == OC_SPACESHIP) && (MPart(obj)->mObjClass == lastObjType))
				{
					U32 archID = (buildButton[index]->GetBuildArchetype());
					if(archID)
					{
						BASIC_DATA * data = (BASIC_DATA *)ARCHLIST->GetArchetypeData(archID);
						if(data->objClass == OC_PLATFORM)
						{
/*							BASE_PLATFORM_DATA * platData = (BASE_PLATFORM_DATA *)data;
							M_RESOURCE_TYPE type;
							if(BANKER->HasCost(obj->GetPlayerID(),platData->missionData.resourceCost,&type))
							{
*/								lastArchID = archID;
								BUILDARCHEID = archID;
								buildButton[index]->SetBuildModeOn(true);
/*							}
							else
							{
								OBJPTR<IFabricator> fab;
								obj->QueryInterface(IFabricatorID,fab);
								fab->FailSound(type);
							}
*/						}
					}
				}
			}
		}
	}

	virtual void OnRightButtonEvent (U32 menuID, U32 controlID)
	{
/*		if(namearea)//get the old name of the ship
		{
			if(namearea->GetControlID() == controlID)
			{
				wchar_t buffer[256];
				_localAnsiToWide(MPart(OBJLIST->GetSelectedList())->partName,buffer,sizeof(buffer));
				wchar_t * namePtr = buffer;
				if(namePtr[0] == '#')
				{
					++namePtr;
					if ((namePtr = wcschr(namePtr,'#')) != 0)
						++namePtr;
					else
						namePtr = buffer;
				}
				namearea->SetText(namePtr);
			}
		}
*/		for(int index = 0; index < NUM_FAB_BUTTONS; ++index)
		{
			if((buildButton[index] != 0) && (buildButton[index]->GetControlID() == controlID))
			{
				IBaseObject * obj = OBJLIST->GetSelectedList();
				if(obj && (obj->objClass == OC_SPACESHIP) && (MPart(obj)->mObjClass == lastObjType))
				{
					BUILDARCHEID = 0;
					while(obj)
					{
						USR_PACKET<USRBUILD> packet;
						packet.objectID[0] = obj->GetPartID();
						packet.cmd = USRBUILD::REMOVE;
						packet.dwArchetypeID = buildButton[index]->GetBuildArchetype();
						packet.init(1, false);
						NETPACKET->Send(HOSTID,0,&packet);
						obj = obj->nextSelected;
					}
				}
			}
		}
	}

	virtual void OnLeftDblButtonEvent (U32 menuID, U32 controlID)		// double-click
	{
	}
	
	/* CMenu_LightInd methods */

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
CMenu_Fabricator::CMenu_Fabricator (void)
{
		// nothing to init so far?
}
//--------------------------------------------------------------------------//
//
CMenu_Fabricator::~CMenu_Fabricator (void)
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
GENRESULT CMenu_Fabricator::Notify (U32 message, void *param)
{
//	MSG *msg = (MSG *) param;

	switch (message)
	{
	case CQE_PANEL_OWNED:
		if (((CMenu_Fabricator *)param) != this)
			setPanelOwnership(false);
		break;

	case CQE_KILL_FOCUS:
		bHasFocus = false;
		break;
	
	case CQE_SET_FOCUS:
		bHasFocus = true;
		break;

	case CQE_LOAD_INTERFACE:
		enableMenu(param!=0);
		break;

	case CQE_UPDATE:
		onUpdate(U32(param) >> 10);
		break;

	case CQE_HOTKEY:
		if (bHasFocus || ( (TOOLBAR != 0) && (TOOLBAR->bInvisible) && (TOOLBAR->bHasFocus) ) )
			handleHotkey((S32) param);
		break;
	}

	return GR_OK;
}
//--------------------------------------------------------------------------//
//
void CMenu_Fabricator::handleHotkey (S32 hotkey)
{
	if(bPanelOwned && hotkey >= IDH_CONTEXT_A && hotkey <= IDH_CONTEXT_Z)
	{
		for(int index = 0; index < NUM_FAB_BUTTONS; ++index)
		{
			if((buildButton[index] != 0) && (buildButton[index]->IsActive()) && (buildButton[index]->GetHotkey() == hotkey))
			{
				IBaseObject * obj = OBJLIST->GetSelectedList();
				if(obj && (obj->objClass == OC_SPACESHIP) && (MPart(obj)->mObjClass == lastObjType))
				{
					U32 archID = (buildButton[index]->GetBuildArchetype());
					if(archID)
					{
						BASIC_DATA * data = (BASIC_DATA *)ARCHLIST->GetArchetypeData(archID);
						if(data->objClass == OC_PLATFORM)
						{
/*							BASE_PLATFORM_DATA * platData = (BASE_PLATFORM_DATA *)data;
							M_RESOURCE_TYPE type;
							if(BANKER->HasCost(obj->GetPlayerID(),platData->missionData.resourceCost,&type))
							{
*/								lastArchID = archID;
								BUILDARCHEID = archID;
								buildButton[index]->SetBuildModeOn(true);
/*							}
							else
							{
								OBJPTR<IFabricator> fab;
								obj->QueryInterface(IFabricatorID,fab);
								fab->FailSound(type);
							}
*/						}
					}
				}
			}
		}
	}
}
//--------------------------------------------------------------------------//
//
void CMenu_Fabricator::onUpdate (U32 dt)
{
	if (bPanelOwned==false || S32(timeoutCount-dt) <= 0)
	{
		timeoutCount = TIMEOUT_PERIOD;

		//see if I have multiple objects selected object
		bool bKeepIt = false;

		IBaseObject * obj = OBJLIST->GetSelectedList();
		MPart part(obj);
		if (obj && (!(obj->nextSelected)) && obj->objClass == OC_SPACESHIP && 
			((part->mObjClass == M_FABRICATOR) || 
			(part->mObjClass == M_WEAVER) || (part->mObjClass == M_FORGER) ||
			(part->mObjClass == M_SHAPER)))
		{
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
			if(lastObjType != MPart(obj)->mObjClass)
			{
				setPanelOwnership(false);
				setPanelOwnership(true);
			}
			//setup info here
			MPart fabricator = obj;
			VOLPTR(IMissionActor) actor = obj;
			wchar_t buffer[256];
			if(lastMissionID != fabricator->dwMissionID)
			{
				BUILDARCHEID = 0;
				lastMissionID = fabricator->dwMissionID;
				wchar_t * namePtr;

				if(fabricator->bShowPartName)
				{
					_localAnsiToWide(fabricator->partName,buffer,sizeof(buffer));
					namePtr = buffer;
					if(namePtr[0] == '#')
					{
						++namePtr;
						if ((namePtr = wcschr(namePtr,'#')) != 0)
							++namePtr;
						else
							namePtr = buffer;
					}
					namearea->SetText(namePtr);
				}

				wchar_t name[128];

				wcscpy(name, _localLoadStringW(fabricator.pInit->displayName));
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

				if(!(fabricator->bShowPartName))
				{
					swprintf(buffer, _localLoadStringW(IDS_IND_SHIPCLASS), namePtr);
					namearea->SetText(buffer);
				}

				swprintf(buffer, _localLoadStringW(IDS_IND_SHIPCLASS), namePtr);
				shipclass->SetText(buffer);

				OBJPTR<IBuildQueue> fab;
				obj->QueryInterface(IBuildQueueID,fab);
				if(fab)
				{
					U32 job = fab->GetFabJobID();
					if(job)
					{
						for(int i = 0; i <NUM_FAB_BUTTONS; ++i)
						{
							if(buildButton[i] != 0)
							{
								U32 archID = buildButton[i]->GetBuildArchetype();
								if(archID== job)
								{
									if(i < 16)
										fabTab->SetCurrentTab(0);
									else if(i < 32)
										fabTab->SetCurrentTab(1);
									else 
										fabTab->SetCurrentTab(2);
								}
							}
						}
					}
					else
					{
						OBJPTR<IFabricator> fabPtr;
						obj->QueryInterface(IFabricatorID,fabPtr);
						if(fabPtr)
							fabTab->SetCurrentTab(fabPtr->GetFabTab());
					}
				}

			}
			swprintf(buffer, _localLoadStringW(IDS_IND_HULL), (actor) ? actor->GetDisplayHullPoints() : fabricator->hullPoints, fabricator->hullPointsMax);
			hull->SetText(buffer);

			TECHNODE newNode = MGlobals::GetCurrentTechLevel(MGlobals::GetThisPlayer());
			TECHNODE workingNode = MGlobals::GetWorkingTechLevel(MGlobals::GetThisPlayer());
			TECHNODE newTechAvail = MGlobals::GetTechAvailable();
			if((!(newNode.IsEqual(lastNode))) || (!(newTechAvail.IsEqual(lastTechAviail))) ) 
			{
				lastTechAviail = newTechAvail;
				lastNode = newNode;
				for(int i = 0; i <NUM_FAB_BUTTONS; ++i)
				{
					if(buildButton[i] != 0)
						buildButton[i]->SetTechNode(lastNode,lastTechAviail,workingNode);
				}
			}
			//update build buttons
			OBJPTR<IBuildQueue> fab;
			obj->QueryInterface(IBuildQueueID,fab);
			if(fab)
			{
				U32 stallType = 0;
				SINGLE progress = fab->FabGetDisplayProgress(stallType);
				U32 job = fab->GetFabJobID();
				for(int i = 0; i <NUM_FAB_BUTTONS; ++i)
				{
					if(buildButton[i] != 0)
					{
						U32 archID = buildButton[i]->GetBuildArchetype();
						if(job)
						{
							if(archID== job)
							{
								buildButton[i]->UpdateBuild(progress,stallType);
							}
							else
							{
								buildButton[i]->ResetActiveButton();
							}
						}
						else
						{
							if(BUILDARCHEID && (lastArchID == archID))
							{
								buildButton[i]->SetBuildModeOn(true);
							}
							else
								buildButton[i]->ResetActiveButton();
							buildButton[i]->EnableButton(true);
						}
					}
				}
			}
			techarmor->SetImageLevel(fabricator->techLevel.hull,GetArmorUpgrades(fabricator->race));
			techengine->SetImageLevel(fabricator->techLevel.engine,GetEngineUpgrades(fabricator->race));
			techsheild->SetImageLevel(fabricator->techLevel.shields,GetShieldUpgrades(fabricator->race));
			techsensors->SetImageLevel(fabricator->techLevel.sensors,GetSensorUpgrades(fabricator->race));

			OBJPTR<IFabricator> fabPtr;
			obj->QueryInterface(IFabricatorID,fabPtr);
			if(fabPtr)
				fabPtr->SetFabTab(fabTab->GetCurrentTab());
		}
	}
	else
		timeoutCount -= dt;
}
//--------------------------------------------------------------------------//
//
void CMenu_Fabricator::enableMenu (bool bEnable)
{
	if((!bEnable) && bPanelOwned)
	{
		setPanelOwnership(false);
	}
}

void CMenu_Fabricator::setPanelOwnership (bool bOwn)
{
	if(bOwn)
	{
		lastMissionID = 0;
		lastArchID = 0;
		lastObjType = 0;
		COMPTR<IToolbar> toolbar;

		if (TOOLBAR->QueryInterface("IToolbar", toolbar) == GR_OK)
		{
			IBaseObject * obj = OBJLIST->GetSelectedList();
			MPart part(obj);
			char buffer[64];
			buffer[0] = 0;
			lastObjType = part->mObjClass;
			if(lastObjType == M_FABRICATOR)
				strcpy(buffer,"fabricator");
			else if(lastObjType == M_WEAVER)
				strcpy(buffer,"M_Weaver");
			else if(lastObjType == M_FORGER)
				strcpy(buffer,"S_Forger");
			else if(lastObjType == M_SHAPER)
				strcpy(buffer,"V_Shaper");
			if (toolbar->GetToolbar(buffer, menu,part->race) == GR_OK)
			{
				COMPTR<IDAComponent> pComp;

				if (toolbar->GetControl("shipclass", pComp) == GR_OK)
					pComp->QueryInterface("IStatic", shipclass);
				if(namearea)
					namearea->EnableEdit(true);
				if (menu->GetControl("shipname", pComp) == GR_OK)
					pComp->QueryInterface("IEdit2", namearea);
				if (menu->GetControl("hull", pComp) == GR_OK)
					pComp->QueryInterface("IStatic", hull);

				//get build buttons
				lastNode = MGlobals::GetCurrentTechLevel(MGlobals::GetThisPlayer());
				lastTechAviail = MGlobals::GetTechAvailable();
				TECHNODE workingNode = MGlobals::GetWorkingTechLevel(MGlobals::GetThisPlayer());
				for(U32 index = 0; index < NUM_FAB_BUTTONS; ++index)
				{
					char buffer[32];
					sprintf(buffer,"plat%d",index);
					if (menu->GetControl(buffer, pComp) == GR_OK)
						pComp->QueryInterface("IActiveButton", buildButton[index]);
					if(buildButton[index] != 0)
					{
						buildButton[index]->SetControlID(buildButton[index]->GetBuildArchetype());//hack;
						buildButton[index]->SetTechNode(lastNode,lastTechAviail,workingNode);
					}
				}

				if (menu->GetControl("techarmor", pComp) == GR_OK)
					pComp->QueryInterface("IHotStatic", techarmor);
				if (menu->GetControl("techengine", pComp) == GR_OK)
					pComp->QueryInterface("IHotStatic", techengine);
				if (menu->GetControl("techsheild", pComp) == GR_OK)
					pComp->QueryInterface("IHotStatic", techsheild);
				if (menu->GetControl("techsensors", pComp) == GR_OK)
					pComp->QueryInterface("IHotStatic", techsensors);				
				if (menu->GetControl("fabTab", pComp) == GR_OK)
					pComp->QueryInterface("ITabControl", fabTab);

				COMPTR<IDAConnectionPoint> connection;

				if (menu->QueryOutgoingInterface("IHotControlEvent", connection) == GR_OK)
					connection->Advise(getBase(), &hotEventHandle);
			}
			
			OBJPTR<IBuildQueue> fab;
			obj->QueryInterface(IBuildQueueID,fab);
			if(fab)
			{
				U32 job = fab->GetFabJobID();
				if(job)
				{
					for(int i = 0; i <NUM_FAB_BUTTONS; ++i)
					{
						if(buildButton[i] != 0)
						{
							U32 archID = buildButton[i]->GetBuildArchetype();
							if(archID== job)
							{
								if(i < 16)
									fabTab->SetCurrentTab(0);
								else if(i < 32)
									fabTab->SetCurrentTab(1);
								else 
									fabTab->SetCurrentTab(2);
							}
						}
					}
				}
			}


			if(menu)
				menu->SetVisible(true);
			bPanelOwned = true;
		}
	}else{
		lastObjType = 0;
		BUILDARCHEID = 0;
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
		namearea.free();
		shipclass.free();
		hull.free();
		techarmor.free();
		techengine.free();
		techsheild.free();
		techsensors.free();
		fabTab.free();
		for(U32 index = 0; index < NUM_FAB_BUTTONS; ++index)
		{
			buildButton[index].free();
		}
	}
}

//--------------------------------------------------------------------------//
//
struct _cmenu_fabricator: GlobalComponent
{
	CMenu_Fabricator * menu;

	virtual void Startup (void)
	{
		menu = new DAComponent<CMenu_Fabricator>;
		AddToGlobalCleanupList(&menu);
	}

	virtual void Initialize (void)
	{
		COMPTR<IDAConnectionPoint> connection;

		if (TOOLBAR->QueryOutgoingInterface("IEventCallback", connection) == GR_OK)
			connection->Advise(menu->getBase(), &menu->eventHandle);
	}
};

static _cmenu_fabricator startup;

//--------------------------------------------------------------------------//
//-----------------------------End CMenu_Ind.cpp----------------------------//
//--------------------------------------------------------------------------//
