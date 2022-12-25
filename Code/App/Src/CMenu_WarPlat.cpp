//--------------------------------------------------------------------------//
//                                                                          //
//                              CMenu_WarPlat.cpp                               //
//                                                                          //
//               COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
   $Header: /Conquest/App/Src/CMenu_WarPlat.cpp 2     5/07/01 9:22a Tmauer $
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
#include "IAttack.h"
#include "ILauncher.h"
#include "IHotStatic.h"
#include "IMultiHotButton.h"
#include "Gendata.h"

#include <DSpecialData.h>
#include <DMBaseData.h>

#include <TComponent.h>
#include <IConnection.h>
#include <HeapObj.h>
#include <TSmartPointer.h>
#include <EventSys.h>

#include <stdio.h>

#define TIMEOUT_PERIOD 500

U32 GetArmorUpgrades(M_RACE race);
U32 GetShieldUpgrades(M_RACE race);
U32 GetEngineUpgrades(M_RACE race);
U32 GetSensorUpgrades(M_RACE race);
U32 GetSuppliesUpgrades(M_RACE race);
U32 GetWeaponsUpgrades(M_RACE race);
U32 GetFleetUpgrades(M_RACE race);
U32 GetFighterUpgrades(M_RACE race);

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//

struct DACOM_NO_VTABLE CMenu_WarPlat : public IEventCallback, IHotControlEvent
{
	BEGIN_DACOM_MAP_INBOUND(CMenu_WarPlat)
	DACOM_INTERFACE_ENTRY(IEventCallback)
	DACOM_INTERFACE_ENTRY(IHotControlEvent)
	END_DACOM_MAP()

	U32 eventHandle, hotEventHandle;		// connection handle
	bool bPanelOwned;
	S32 timeoutCount;

	COMPTR<IToolbar> menu;
	COMPTR<IStatic> shipclass,hull,supplies,location;
	COMPTR<IHotButton> fighterStanceNormal,fighterStancePatrol,specialWpnCmd;
	COMPTR<IIcon> inSupply,notInSupply;
	COMPTR<IHotStatic> techarmor,techsupply,techsheild,techweapon,techsensors,
		techspecial;
	COMPTR<IMultiHotButton> specialWpnMulti;

	U32 lastMissionID;
	S32 lastObjType;

	CMenu_WarPlat (void);

	~CMenu_WarPlat (void);

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
	
	/* CMenu_WarPlat methods */

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
CMenu_WarPlat::CMenu_WarPlat (void)
{
		// nothing to init so far?
}
//--------------------------------------------------------------------------//
//
CMenu_WarPlat::~CMenu_WarPlat (void)
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
GENRESULT CMenu_WarPlat::Notify (U32 message, void *param)
{
//	MSG *msg = (MSG *) param;

	switch (message)
	{
	case CQE_PANEL_OWNED:
		if (((CMenu_WarPlat *)param) != this)
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
void CMenu_WarPlat::onUpdate (U32 dt)
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
			if((part->mObjClass == M_LSAT) || (part->mObjClass == M_SPACESTATION) || (part->mObjClass == M_IONCANNON) ||
				(part->mObjClass == M_VORAAKCANNON) || (part->mObjClass == M_PLASMAHIVE) ||
				(part->mObjClass == M_PROTEUS) ||
				(part->mObjClass == M_HYDROFOIL) || (part->mObjClass == M_ESPCOIL) ||
				(part->mObjClass == M_STARBURST) || (part->mObjClass == M_NOVA_BOMB))
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
				if(inSupply)
					inSupply->SetVisible(true);
				if(notInSupply)
					notInSupply->SetVisible(false);
			}
			else
			{
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

			techsupply->SetImageLevel(platform->techLevel.supplies,GetSuppliesUpgrades(platform->race));
			techarmor->SetImageLevel(platform->techLevel.hull,GetArmorUpgrades(platform->race));
			techsheild->SetImageLevel(platform->techLevel.shields,GetShieldUpgrades(platform->race));
			techweapon->SetImageLevel(platform->techLevel.damage,GetWeaponsUpgrades(platform->race));
			techsensors->SetImageLevel(platform->techLevel.sensors,GetSensorUpgrades(platform->race));
			if(platform->mObjClass == M_SPACESTATION || platform->mObjClass == M_PLASMAHIVE)
			{
				techspecial->SetImageLevel(platform->techLevel.classSpecific,GetFighterUpgrades(platform->race));
				techspecial->SetTextString(HSTTXT::FIGHTER);
			}
			else
			{
				techspecial->SetImageLevel(0,1);
				techspecial->SetTextString(HSTTXT::NOTEXT);
			}

			OBJPTR<IAttack> attack;
			obj->QueryInterface(IAttackID,attack);
			if(attack && (platform->fleetID == 0))
			{
				if(platform->mObjClass == M_SPACESTATION || platform->mObjClass == M_PLASMAHIVE)
				{
					FighterStance fStance = attack->GetFighterStance();
					fighterStanceNormal->SetVisible(true);
					fighterStancePatrol->SetVisible(true);
					fighterStanceNormal->EnableButton(true);
					fighterStancePatrol->EnableButton(true);
					if(fStance == FS_NORMAL)
					{
						fighterStanceNormal->SetHighlightState(true);
						fighterStancePatrol->SetHighlightState(false);
					}
					else if(fStance == FS_PATROL)
					{
						fighterStanceNormal->SetHighlightState(false);
						fighterStancePatrol->SetHighlightState(true);
					}
				}
				else
				{
					fighterStanceNormal->SetVisible(false);
					fighterStancePatrol->SetVisible(false);
					fighterStanceNormal->EnableButton(false);
					fighterStancePatrol->EnableButton(false);
				}
			}
			else
			{
				fighterStanceNormal->SetVisible(false);
				fighterStancePatrol->SetVisible(false);
				fighterStanceNormal->EnableButton(false);
				fighterStancePatrol->EnableButton(false);
			}

			const _GT_SPECIALABILITIES * specialAbilities = (const _GT_SPECIALABILITIES *) GENDATA->GetArchetypeData("SpecialAbilities");
			CQASSERT(specialAbilities);
			CQASSERT(sizeof(_GT_SPECIALABILITIES) == sizeof(GT_SPECIALABILITIES) && "Data definition error");
			const SPECIALABILITYINFO * specialInfo = specialAbilities->info + platform.pInit->specialAbility;

			if(platform.pInit->specialAbility != USA_NONE && (platform->caps.specialEOAOk || platform->caps.specialAttackOk || platform->caps.specialAttackShipOk || platform->caps.specialAbilityOk ||
				platform->caps.probeOk || platform->caps.mimicOk || platform->caps.synthesisOk) ||platform->caps.specialTargetPlanetOk)
			{
				specialWpnCmd->SetVisible(true);
				specialWpnMulti->SetState(specialInfo->baseSpecialWpnButton,specialInfo->specialWpnTooltip,specialInfo->specialWpnHelpBox,specialInfo->specialWpnHintBox);
			}
			else
			{
				specialWpnCmd->SetVisible(false);
				specialWpnMulti->NullState();
			}
		}
	}
	else
		timeoutCount -= dt;
}
//--------------------------------------------------------------------------//
//
void CMenu_WarPlat::enableMenu (bool bEnable)
{
	if((!bEnable) && bPanelOwned)
	{
		setPanelOwnership(false);
	}
}

void CMenu_WarPlat::setPanelOwnership (bool bOwn)
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
			if (lastObjType == M_LSAT || lastObjType == M_SPACESTATION || lastObjType == M_IONCANNON ||
				lastObjType == M_PLASMAHIVE || (lastObjType == M_VORAAKCANNON)||
				(lastObjType == M_PROTEUS) ||
				(lastObjType == M_HYDROFOIL) || (lastObjType == M_ESPCOIL) ||
				(lastObjType == M_STARBURST)|| (lastObjType == M_NOVA_BOMB))
				strcpy(buffer,"WarTurret");
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
				if (menu->GetControl("inSupply", pComp) == GR_OK)
					pComp->QueryInterface("IIcon", inSupply);
				if (menu->GetControl("notInSupply", pComp) == GR_OK)
					pComp->QueryInterface("IIcon", notInSupply);
				if (menu->GetControl("fighterStanceNormal", pComp) == GR_OK)
					pComp->QueryInterface("IHotButton", fighterStanceNormal);
				if (menu->GetControl("fighterStancePatrol", pComp) == GR_OK)
					pComp->QueryInterface("IHotButton", fighterStancePatrol);
				if (menu->GetControl("techarmor", pComp) == GR_OK)
					pComp->QueryInterface("IHotStatic", techarmor);
				if (menu->GetControl("techsupply", pComp) == GR_OK)
					pComp->QueryInterface("IHotStatic", techsupply);
				if (menu->GetControl("techsheild", pComp) == GR_OK)
					pComp->QueryInterface("IHotStatic", techsheild);
				if (menu->GetControl("techweapon", pComp) == GR_OK)
					pComp->QueryInterface("IHotStatic", techweapon);
				if (menu->GetControl("techsensors", pComp) == GR_OK)
					pComp->QueryInterface("IHotStatic", techsensors);
				if (menu->GetControl("techspecial", pComp) == GR_OK)
					pComp->QueryInterface("IHotStatic", techspecial);
				if (menu->GetControl("specialweapon", pComp) == GR_OK)
				{
					pComp->QueryInterface("IHotButton", specialWpnCmd);
					pComp->QueryInterface("IMultiHotButton",specialWpnMulti);
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
		lastObjType = 0;
		bPanelOwned = false;

		menu.free();
		shipclass.free();
		hull.free();
		supplies.free();
		location.free();
		inSupply.free();
		notInSupply.free();
		fighterStanceNormal.free();
		fighterStancePatrol.free();
		techarmor.free();
		techsupply.free();
		techsheild.free();
		techweapon.free();
		techsensors.free();
		techspecial.free();
		specialWpnCmd.free();
		specialWpnMulti.free();
	}
}

//--------------------------------------------------------------------------//
//
struct _cmenu_warplat: GlobalComponent
{
	CMenu_WarPlat * menu;

	virtual void Startup (void)
	{
		menu = new DAComponent<CMenu_WarPlat>;
		AddToGlobalCleanupList(&menu);
	}

	virtual void Initialize (void)
	{
		COMPTR<IDAConnectionPoint> connection;

		if (TOOLBAR->QueryOutgoingInterface("IEventCallback", connection) == GR_OK)
			connection->Advise(menu->getBase(), &menu->eventHandle);
	}
};

static _cmenu_warplat startup;

//--------------------------------------------------------------------------//
//-----------------------------End CMenu_WarPlat.cpp----------------------------//
//--------------------------------------------------------------------------//
