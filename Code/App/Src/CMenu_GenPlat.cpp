//--------------------------------------------------------------------------//
//                                                                          //
//                              CMenu_GenPlat.cpp                               //
//                                                                          //
//               COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
   $Header: /Conquest/App/Src/CMenu_GenPlat.cpp 21    5/07/01 9:22a Tmauer $
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
#include "MGlobals.h"
#include "Sector.h"
#include "IIcon.h"
#include "IMissionActor.h"

#include <DMBaseData.h>

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

struct DACOM_NO_VTABLE CMenu_GenPlat : public IEventCallback, IHotControlEvent
{
	BEGIN_DACOM_MAP_INBOUND(CMenu_GenPlat)
	DACOM_INTERFACE_ENTRY(IEventCallback)
	DACOM_INTERFACE_ENTRY(IHotControlEvent)
	END_DACOM_MAP()

	U32 eventHandle, hotEventHandle;		// connection handle
	bool bPanelOwned;
	S32 timeoutCount;

	COMPTR<IToolbar> menu;
	COMPTR<IStatic> shipclass,hull,supplies,location,disabledText;
	COMPTR<IIcon> inSupply,notInSupply;

	U32 lastMissionID;
	S32 lastObjType;

	CMenu_GenPlat (void);

	~CMenu_GenPlat (void);

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
	
	/* CMenu_GenPlat methods */

	void onUpdate (U32 dt);	// in mseconds

	void enableMenu (bool bEnable);

	void setPanelOwnership (bool bOwn);

	IDAComponent * getBase (void)
	{
		return static_cast<IEventCallback *>(this);
	}

};
//--------------------------------------------------------------------------//
//
CMenu_GenPlat::CMenu_GenPlat (void)
{
		// nothing to init so far?
}
//--------------------------------------------------------------------------//
//
CMenu_GenPlat::~CMenu_GenPlat (void)
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
GENRESULT CMenu_GenPlat::Notify (U32 message, void *param)
{
//	MSG *msg = (MSG *) param;

	switch (message)
	{
	case CQE_PANEL_OWNED:
		if (((CMenu_GenPlat *)param) != this)
			setPanelOwnership(false);
		break;

	case CQE_LOAD_INTERFACE:
		enableMenu(param!=0);
		break;

	case CQE_UPDATE:
		onUpdate(U32(param) >> 10);
		break;

	}

	return GR_OK;
}
//--------------------------------------------------------------------------//
//
void CMenu_GenPlat::onUpdate (U32 dt)
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
			if(	(part->mObjClass == M_DISSECTIONCHAMBER) ||
				(part->mObjClass == M_JUMPPLAT) || (part->mObjClass == M_REPAIR) ||
				(part->mObjClass == M_TALOREANMATRIX) || 
				(part->mObjClass == M_EUTROMILL) ||
				(part->mObjClass == M_GREATER_PLANTATION) || (part->mObjClass ==M_TENDER) ||
				(part->mObjClass == M_REPOSITORIUM) || (part->mObjClass == M_PERPETUATOR) ||
				(part->mObjClass == M_SINUATOR) || (part->mObjClass == M_INTEL_CENTER) ||
				(part->mObjClass == M_T_RESOURCE_FACTORY) || (part->mObjClass == M_T_RESEARCH_LAB) ||
				(part->mObjClass == M_T_IND_FACILITY))
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
				if(disabledText)
					disabledText->SetVisible(false);
				if(inSupply)
					inSupply->SetVisible(true);
				if(notInSupply)
					notInSupply->SetVisible(false);
			}
			else
			{
				if(disabledText != 0)
					disabledText->SetVisible(true);
				else 
					disabledText->SetVisible(false);
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
			swprintf(buffer, _localLoadStringW(IDS_IND_HULL), (actor)?actor->GetDisplayHullPoints() : platform->hullPoints, platform->hullPointsMax);
			hull->SetText(buffer);

			if(platform->supplyPointsMax)
			{
				swprintf(buffer, _localLoadStringW(IDS_IND_SUPPLIES), (actor) ? actor->GetDisplaySupplies() : platform->supplies, platform->supplyPointsMax);
				supplies->SetText(buffer);
			}
			else
			{
				supplies->SetText(L"");
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
		}
	}
	else
		timeoutCount -= dt;
}
//--------------------------------------------------------------------------//
//
void CMenu_GenPlat::enableMenu (bool bEnable)
{
	if((!bEnable) && bPanelOwned)
	{
		setPanelOwnership(false);
	}
}

void CMenu_GenPlat::setPanelOwnership (bool bOwn)
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
			if (lastObjType == M_DISSECTIONCHAMBER || 
				(lastObjType == M_JUMPPLAT) || (part->mObjClass == M_REPAIR) ||
				(lastObjType == M_TALOREANMATRIX) || 
				(lastObjType == M_EUTROMILL)||
				(lastObjType == M_GREATER_PLANTATION)|| (part->mObjClass ==M_TENDER) ||
				(part->mObjClass == M_REPOSITORIUM) || (part->mObjClass == M_PERPETUATOR) ||
				(part->mObjClass == M_SINUATOR)|| (part->mObjClass == M_INTEL_CENTER) ||
				(part->mObjClass == M_T_RESOURCE_FACTORY) || (part->mObjClass == M_T_RESEARCH_LAB) ||
				(part->mObjClass == M_T_IND_FACILITY))
				strcpy(buffer,"turret");
			if (toolbar->GetToolbar(buffer, menu, part->race) == GR_OK)
			{
				COMPTR<IDAComponent> pComp;

				if (toolbar->GetControl("shipclass", pComp) == GR_OK)
					pComp->QueryInterface("IStatic", shipclass);
				if (menu->GetControl("hull", pComp) == GR_OK)
					pComp->QueryInterface("IStatic", hull);
				if (menu->GetControl("supplies", pComp) == GR_OK)
					pComp->QueryInterface("IStatic", supplies);
				if (menu->GetControl("location", pComp) == GR_OK)
					pComp->QueryInterface("IStatic", location);
				if (menu->GetControl("disabledText", pComp) == GR_OK)
					pComp->QueryInterface("IStatic", disabledText);
				if (menu->GetControl("inSupply", pComp) == GR_OK)
					pComp->QueryInterface("IIcon", inSupply);
				if (menu->GetControl("notInSupply", pComp) == GR_OK)
					pComp->QueryInterface("IIcon", notInSupply);


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
		lastObjType = 0;
		bPanelOwned = false;

		menu.free();
		shipclass.free();
		hull.free();
		supplies.free();
		location.free();
		inSupply.free();
		notInSupply.free();
		disabledText.free();
	}
}

//--------------------------------------------------------------------------//
//
struct _cmenu_genplat: GlobalComponent
{
	CMenu_GenPlat * menu;

	virtual void Startup (void)
	{
		menu = new DAComponent<CMenu_GenPlat>;
		AddToGlobalCleanupList(&menu);
	}

	virtual void Initialize (void)
	{
		COMPTR<IDAConnectionPoint> connection;

		if (TOOLBAR->QueryOutgoingInterface("IEventCallback", connection) == GR_OK)
			connection->Advise(menu->getBase(), &menu->eventHandle);
	}
};

static _cmenu_genplat startup;

//--------------------------------------------------------------------------//
//-----------------------------End CMenu_GenPlat.cpp----------------------------//
//--------------------------------------------------------------------------//
