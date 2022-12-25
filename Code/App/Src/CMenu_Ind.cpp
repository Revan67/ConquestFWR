//--------------------------------------------------------------------------//
//                                                                          //
//                              CMenu_Ind.cpp                               //
//                                                                          //
//               COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
   $Header: /Conquest/App/Src/CMenu_Ind.cpp 66    19/10/01 11:01 Tmauer $
*/
//--------------------------------------------------------------------------//

 
#include "pch.h"
#include <globals.h>

#include "Startup.h"
#include "BaseHotRect.h"
#include "IToolbar.h"
#include "IStatic.h"
#include "ObjList.h"
#include "IObject.h"
#include "MPart.h"
#include "IHotStatic.h"
#include "IEdit2.h"
#include "IHotButton.h"
#include "IMultiHotButton.h"
#include <DMBaseData.h>
#include "CommPacket.h"
#include "NetPacket.h"
#include "IAttack.h"
#include <DSpecialData.h>
#include "GenData.h"
#include "ISupplier.h"
#include "IHarvest.h"
#include "IMissionActor.h"
#include <DSupplyShipSave.h>
#include "IArtifact.h"
#include <DArtifacts.h>

#include <TComponent.h>
#include <IConnection.h>
#include <HeapObj.h>
#include <TSmartPointer.h>
#include <EventSys.h>

#include <stdio.h>

U32 GetArmorUpgrades(M_RACE race)
{
	switch(race)
	{
	case M_TERRAN:
		return 5;
	case M_MANTIS:
		return 4;
	case M_SOLARIAN:
		return 6;
	case M_VYRIUM:
		return 1;
	}
	return 1;
}

U32 GetShieldUpgrades(M_RACE race)
{
	switch(race)
	{
	case M_TERRAN:
		return 5;
	case M_MANTIS:
		return 4;
	case M_SOLARIAN:
		return 6;
	case M_VYRIUM:
		return 1;
	}
	return 1;
}

U32 GetEngineUpgrades(M_RACE race)
{
	switch(race)
	{
	case M_TERRAN:
		return 5;
	case M_MANTIS:
		return 4;
	case M_SOLARIAN:
		return 6;
	case M_VYRIUM:
		return 1;
	}
	return 1;
}

U32 GetSensorUpgrades(M_RACE race)
{
	switch(race)
	{
	case M_TERRAN:
		return 3;
	case M_MANTIS:
		return 4;
	case M_SOLARIAN:
		return 6;
	case M_VYRIUM:
		return 1;
	}
	return 1;
}

U32 GetSuppliesUpgrades(M_RACE race)
{
	switch(race)
	{
	case M_TERRAN:
		return 5;
	case M_MANTIS:
		return 4;
	case M_SOLARIAN:
		return 6;
	case M_VYRIUM:
		return 1;
	}
	return 1;
}

U32 GetWeaponsUpgrades(M_RACE race)
{
	switch(race)
	{
	case M_TERRAN:
		return 5;
	case M_MANTIS:
		return 4;
	case M_SOLARIAN:
		return 6;
	case M_VYRIUM:
		return 1;
	}
	return 1;
}

U32 GetTroopShipUpgrades(M_RACE race)
{
	switch(race)
	{
	case M_TERRAN:
		return 4;
	case M_MANTIS:
		return 3;
	case M_SOLARIAN:
		return 5;
	case M_VYRIUM:
		return 1;
	}
	return 1;
}

U32 GetRamUpgrades(M_RACE race)
{
	switch(race)
	{
	case M_TERRAN:
		return 1;
	case M_MANTIS:
		return 3;
	case M_SOLARIAN:
		return 1;
	case M_VYRIUM:
		return 1;
	}
	return 1;
}

U32 GetTenderUpgrades(M_RACE race)
{
	switch(race)
	{
	case M_TERRAN:
		return 3;
	case M_MANTIS:
		return 4;
	case M_SOLARIAN:
		return 3;
	case M_VYRIUM:
		return 1;
	}
	return 1;
}

U32 GetHarvesterUpgrades(M_RACE race)
{
	switch(race)
	{
	case M_TERRAN:
		return 3;
	case M_MANTIS:
		return 4;
	case M_SOLARIAN:
		return 3;
	case M_VYRIUM:
		return 1;
	}
	return 1;
}

U32 GetFighterUpgrades(M_RACE race)
{
	switch(race)
	{
	case M_TERRAN:
		return 4;
	case M_MANTIS:
		return 6;
	case M_SOLARIAN:
		return 1;
	case M_VYRIUM:
		return 1;
	}
	return 1;
}

U32 GetFleetUpgrades(M_RACE race)
{
	switch(race)
	{
	case M_TERRAN:
		return 5;
	case M_MANTIS:
		return 4;
	case M_SOLARIAN:
		return 6;
	case M_VYRIUM:
		return 1;
	}
	return 1;
}

#define TIMEOUT_PERIOD 500
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//

struct DACOM_NO_VTABLE CMenu_Ind : public IEventCallback, IHotControlEvent
{
	BEGIN_DACOM_MAP_INBOUND(CMenu_Ind)
	DACOM_INTERFACE_ENTRY(IEventCallback)
	DACOM_INTERFACE_ENTRY(IHotControlEvent)
	END_DACOM_MAP()

	U32 eventHandle, hotEventHandle;		// connection handle
//	bool bEnabled;
	bool bPanelOwned;
	S32 timeoutCount;

	DWORD lastMissionID;	//id of the last selected ship

	COMPTR<IToolbar> menu;
	COMPTR<IStatic> shipclass,hull,supplies,kills,metal,gas;
	COMPTR<IHotStatic> techarmor,techsupply,techengine,techsheild,techweapon,techsensors,
		techspecial;
	COMPTR<IEdit2> namearea;
	COMPTR<IHotButton> stopCmd, sellCmd, specialWpnCmd, patrolCmd, escortCmd, cloak, attackPosition,specialWpnCmd1,specialWpnCmd2;
	COMPTR<IHotButton> stanceAttack,stanceDefend,stanceStand,stanceStop,
			supplyStanceAuto,supplyStanceNoAuto,supplyStanceResupplyOnly,ejectArtifact, useArtifactHB;
	COMPTR<IHotButton> fighterStanceNormal,fighterStancePatrol;
	COMPTR<IMultiHotButton> specialWpnMulti,specialWpnMulti1,specialWpnMulti2, useArtifactMB;


	CMenu_Ind (void);

	~CMenu_Ind (void);

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
*/	}

	virtual void OnLeftDblButtonEvent (U32 menuID, U32 controlID)		// double-click
	{
	}
	
	/* CMenu_Ind methods */

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
CMenu_Ind::CMenu_Ind (void)
{
		// nothing to init so far?
}
//--------------------------------------------------------------------------//
//
CMenu_Ind::~CMenu_Ind (void)
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
GENRESULT CMenu_Ind::Notify (U32 message, void *param)
{
//	MSG *msg = (MSG *) param;

	switch (message)
	{
	case CQE_PANEL_OWNED:
		if (((CMenu_Ind *)param) != this)
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
void CMenu_Ind::onUpdate (U32 dt)
{
	if (bPanelOwned==false || S32(timeoutCount-dt) <= 0)
	{
		timeoutCount = TIMEOUT_PERIOD;

		//see if I have a single selected object
		bool bKeepIt = false;

		IBaseObject * obj = OBJLIST->GetSelectedList();
		MPart part(obj);
		if (obj && obj->nextSelected==0 && obj->objClass == OC_SPACESHIP &&
			 part->mObjClass != M_FABRICATOR && part->mObjClass != M_WEAVER && part->mObjClass != M_FORGER && part->mObjClass != M_SHAPER) //I do
		{
			if(!(TESTADMIRAL(part->dwMissionID) || part->admiralID))
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
			MPart ship = obj;
			VOLPTR(IMissionActor) actor = ship.obj;

			if (ship.isValid())
			{
				//
				// fill in the data
				//
				wchar_t buffer[256];
				wchar_t name[128];
				wchar_t * ptr;

				wcscpy(name, _localLoadStringW(ship.pInit->displayName));
				if ((ptr = wcschr(name, '#')) != 0)
					*ptr = 0;
				if ((ptr = wcschr(name, '(')) != 0)
					*ptr = 0;
				if ((ptr = wcsrchr(name, '\'')) != 0)
				{
					ptr++;
					if (ptr[0] == ' ')
						ptr++;
				}
				else
					ptr = name;

				if(lastMissionID != ship->dwMissionID)
				{
					lastMissionID = ship->dwMissionID;
					if(ship->bShowPartName)
					{
						_localAnsiToWide(ship->partName,buffer,sizeof(buffer));
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
					else
					{
						swprintf(buffer, _localLoadStringW(IDS_IND_SHIPCLASS), ptr);
						namearea->SetText(buffer);
					}
					if(namearea->IsTextAllVisible())
						namearea->SetVisible(true);
					else
						namearea->SetVisible(false);

					if(ship->caps.moveOk)
						patrolCmd->SetVisible(true);
					else
						patrolCmd->SetVisible(false);
					if (MGlobals::IsGunboat(ship->mObjClass) || MGlobals::IsSupplyShip(ship->mObjClass))
					{
						escortCmd->SetVisible(true);
					}
					else
					{
						escortCmd->SetVisible(false);
					}

					cloak->SetVisible(ship->caps.cloakOk);

					if (attackPosition)	// Was crashing here with attackPosition NULL	-ryan
					{
						attackPosition->SetVisible(ship->caps.targetPositionOk);
					}

					VOLPTR(IArtifactHolder) artHolder = obj;
					if(artHolder)
					{
						VOLPTR(IArtifact) art = artHolder->GetArtifact();
						if(art)
						{
							ArtifactButtonInfo * info = art->GetButtonInfo();
							useArtifactMB->SetState(info->baseButton,info->tooltip,info->helpBox,info->hintBox);
							ejectArtifact->SetVisible(true);
							useArtifactHB->SetVisible(true);
							if(art->IsUsable())
							{
								useArtifactHB->EnableButton(true);
							}
							else
							{
								useArtifactHB->EnableButton(false);
							}
						}
						else
						{
							ejectArtifact->SetVisible(false);
							useArtifactHB->SetVisible(false);
						}
					}
					else
					{
						ejectArtifact->SetVisible(false);
						useArtifactHB->SetVisible(false);
					}

					if((ship->mObjClass == M_SUPPLY) || (ship->mObjClass == M_ZORAP) || (ship->mObjClass == M_STRATUM))
					{
						kills->SetVisible(false);
						supplies->SetVisible(true);
						metal->SetVisible(false);
						gas->SetVisible(false);
					}
					else if((ship->mObjClass == M_HARVEST) || (ship->mObjClass == M_GALIOT) || (ship->mObjClass == M_SIPHON))
					{
						kills->SetVisible(false);
						supplies->SetVisible(false);
						metal->SetVisible(true);
						gas->SetVisible(true);
					}
					else if(MGlobals::IsTroopship(ship->mObjClass))
					{
						kills->SetVisible(false);
						supplies->SetVisible(false);
						metal->SetVisible(false);
						gas->SetVisible(false);
					}
					else
					{
						kills->SetVisible(true);
						supplies->SetVisible(true);
						metal->SetVisible(false);
						gas->SetVisible(false);
					}
				}

				const _GT_SPECIALABILITIES * specialAbilities = (const _GT_SPECIALABILITIES *) GENDATA->GetArchetypeData("SpecialAbilities");
				CQASSERT(specialAbilities);
				CQASSERT(sizeof(_GT_SPECIALABILITIES) == sizeof(GT_SPECIALABILITIES) && "Data definition error");
				const SPECIALABILITYINFO * specialInfo = specialAbilities->info + ship.pInit->specialAbility;

				if(ship.pInit->specialAbility != USA_NONE && (ship->caps.specialEOAOk || ship->caps.specialAttackOk || ship->caps.specialAttackShipOk || ship->caps.specialAbilityOk ||
					ship->caps.probeOk || ship->caps.mimicOk || ship->caps.synthesisOk) ||ship->caps.specialTargetPlanetOk)
				{
					specialWpnCmd->SetVisible(true);
					specialWpnMulti->SetState(specialInfo->baseSpecialWpnButton,specialInfo->specialWpnTooltip,specialInfo->specialWpnHelpBox,specialInfo->specialWpnHintBox);
				}
				else
				{
					specialWpnCmd->SetVisible(false);
					specialWpnMulti->NullState();
				}

				specialInfo = specialAbilities->info + ship.pInit->specialAbility1;
				if(ship.pInit->specialAbility1 != USA_NONE && (ship->caps.specialEOAOk || ship->caps.specialAttackOk || ship->caps.specialAttackShipOk || ship->caps.specialAbilityOk ||
					ship->caps.probeOk || ship->caps.mimicOk || ship->caps.synthesisOk) ||ship->caps.specialTargetPlanetOk)
				{
					specialWpnCmd1->SetVisible(true);
					specialWpnMulti1->SetState(specialInfo->baseSpecialWpnButton,specialInfo->specialWpnTooltip,specialInfo->specialWpnHelpBox,specialInfo->specialWpnHintBox);
				}
				else
				{
					specialWpnCmd1->SetVisible(false);
					specialWpnMulti1->NullState();
				}

				specialInfo = specialAbilities->info + ship.pInit->specialAbility2;
				if(ship.pInit->specialAbility2 != USA_NONE && (ship->caps.specialEOAOk || ship->caps.specialAttackOk || ship->caps.specialAttackShipOk || ship->caps.specialAbilityOk ||
					ship->caps.probeOk || ship->caps.mimicOk || ship->caps.synthesisOk) ||ship->caps.specialTargetPlanetOk)
				{
					specialWpnCmd2->SetVisible(true);
					specialWpnMulti2->SetState(specialInfo->baseSpecialWpnButton,specialInfo->specialWpnTooltip,specialInfo->specialWpnHelpBox,specialInfo->specialWpnHintBox);
				}
				else
				{
					specialWpnCmd2->SetVisible(false);
					specialWpnMulti2->NullState();
				}

				swprintf(buffer, _localLoadStringW(IDS_IND_SHIPCLASS), ptr);
				shipclass->SetText(buffer);

				swprintf(buffer, _localLoadStringW(IDS_IND_HULL), (actor)?actor->GetDisplayHullPoints() : ship->hullPoints, ship->hullPointsMax);
				hull->SetText(buffer);

				if((ship->mObjClass == M_HARVEST) || (ship->mObjClass == M_GALIOT) || (ship->mObjClass == M_SIPHON))
				{
					U32 numGas = 0;
					U32 numMetal = 0;
					OBJPTR<IHarvest> harvest;
					obj->QueryInterface(IHarvestID,harvest);
					if(harvest)
					{
						numGas = harvest->GetGas();
						numMetal = harvest->GetMetal();
					}
					swprintf(buffer, _localLoadStringW(IDS_IND_METAL), numMetal*METAL_MULTIPLIER, (ship->supplyPointsMax-numGas)*METAL_MULTIPLIER);
					metal->SetText(buffer);

					swprintf(buffer, _localLoadStringW(IDS_IND_GAS),numGas*GAS_MULTIPLIER, (ship->supplyPointsMax-numMetal)*GAS_MULTIPLIER);
					gas->SetText(buffer);
				}
				else
				{
					swprintf(buffer, _localLoadStringW(IDS_IND_SUPPLIES), (actor) ? actor->GetDisplaySupplies() : ship->supplies, ship->supplyPointsMax);
					supplies->SetText(buffer);
					if(obj->fieldFlags.suppliesLocked())
						supplies->SetTextColor(RGB(240,128,0));
					else
						supplies->SetTextColor(RGB(0,255,255));

					swprintf(buffer, _localLoadStringW(IDS_IND_KILLS),(U32)(ship->numKills));
					kills->SetText(buffer);
				}

				if((ship->mObjClass == M_HARVEST) || (ship->mObjClass == M_GALIOT) || (ship->mObjClass == M_SIPHON) || MGlobals::IsTroopship(ship->mObjClass))
					techsupply->SetVisible(false);
				else
					techsupply->SetVisible(true);

				if(MGlobals::IsSupplyShip(ship->mObjClass) || (ship->mObjClass == M_HARVEST) || (ship->mObjClass == M_GALIOT) || (ship->mObjClass == M_SIPHON) || MGlobals::IsTroopship(ship->mObjClass) ||
					(ship->mObjClass == M_INFILTRATOR) || (ship->mObjClass == M_ORACLE) || (ship->mObjClass == M_SEEKER))
					techweapon->SetVisible(false);
				else
					techweapon->SetVisible(true);

				if(ship.pInit->baseShieldLevel)
					techsheild->SetVisible(true);
				else
					techsheild->SetVisible(false);

				techsupply->SetImageLevel(ship->techLevel.supplies,GetSuppliesUpgrades(ship->race));
				techarmor->SetImageLevel(ship->techLevel.hull,GetArmorUpgrades(ship->race));
				techengine->SetImageLevel(ship->techLevel.engine,GetEngineUpgrades(ship->race));
				techsheild->SetImageLevel(ship->techLevel.shields,GetShieldUpgrades(ship->race));
				techweapon->SetImageLevel(ship->techLevel.damage,GetWeaponsUpgrades(ship->race));
				techsensors->SetImageLevel(ship->techLevel.sensors,GetSensorUpgrades(ship->race));
				if(ship->mObjClass == M_TROOPSHIP)
				{
					techspecial->SetImageLevel(ship->techLevel.classSpecific,GetTroopShipUpgrades(ship->race));
					techspecial->SetTextString(HSTTXT::TROOPSHIP);
				}
				else if(ship->mObjClass == M_LEECH)
				{
					techspecial->SetImageLevel(ship->techLevel.classSpecific,GetTroopShipUpgrades(ship->race));
					techspecial->SetTextString(HSTTXT::LEECH);
				}
				else if(ship->mObjClass == M_LEGIONAIRE)
				{
					techspecial->SetImageLevel(ship->techLevel.classSpecific,GetTroopShipUpgrades(ship->race));
					techspecial->SetTextString(HSTTXT::LEGIONAIRE);
				}				
				else if(ship->mObjClass == M_HARVEST)
				{
					techspecial->SetImageLevel(ship->techLevel.classSpecific,GetHarvesterUpgrades(ship->race));
					techspecial->SetTextString(HSTTXT::HARVESTER);
				}
				else if(ship->mObjClass == M_GALIOT)
				{
					techspecial->SetImageLevel(ship->techLevel.classSpecific,GetHarvesterUpgrades(ship->race));
					techspecial->SetTextString(HSTTXT::GALIOT);
				}
				else if(ship->mObjClass == M_SIPHON)
				{
					techspecial->SetImageLevel(ship->techLevel.classSpecific,GetHarvesterUpgrades(ship->race));
					techspecial->SetTextString(HSTTXT::SIPHON);
				}
				else if((ship->mObjClass == M_SUPPLY) || (ship->mObjClass == M_ZORAP) || (ship->mObjClass == M_STRATUM))
				{
					techspecial->SetImageLevel(ship->techLevel.classSpecific,GetTenderUpgrades(ship->race));
					techspecial->SetTextString(HSTTXT::TENDER);
				}
				else if(MGlobals::IsFlagship(ship->mObjClass))
				{
					techspecial->SetImageLevel(ship->techLevel.classSpecific,GetFleetUpgrades(ship->race));
					techspecial->SetTextString(HSTTXT::FLEET);
				}
				else if(MGlobals::IsCarrier(ship->mObjClass) ||
					ship.pInit->displayName == OBJNAMES::MT_SPACE_STATION)
				{
					techspecial->SetImageLevel(ship->techLevel.classSpecific,GetFighterUpgrades(ship->race));
					techspecial->SetTextString(HSTTXT::FIGHTER);
				}
				else if(ship.pInit->displayName == OBJNAMES::MT_MISSILE_CRUISER)
				{
					techspecial->SetImageLevel(ship->techLevel.classSpecific,3);
					techspecial->SetTextString(HSTTXT::MISSLEPAK);
				}
				else if(ship.pInit->displayName == OBJNAMES::MT_KHAMIR)
				{
					techspecial->SetImageLevel(ship->techLevel.classSpecific,3);
					techspecial->SetTextString(HSTTXT::RAM);
				}
				else
				{
					techspecial->SetImageLevel(0,1);
					techspecial->SetTextString(HSTTXT::NOTEXT);
				}

				OBJPTR<IAttack> attack;
				obj->QueryInterface(IAttackID,attack);
				if(attack && obj->objClass == OC_SPACESHIP && (ship->fleetID == 0))
				{
					supplyStanceNoAuto->SetVisible(false);
					supplyStanceAuto->SetVisible(false);
					supplyStanceResupplyOnly->SetVisible(false);
					supplyStanceNoAuto->EnableButton(false);
					supplyStanceAuto->EnableButton(false);
					supplyStanceResupplyOnly->EnableButton(false);
					supplyStanceNoAuto->SetHighlightState(false);
					supplyStanceAuto->SetHighlightState(false);
					supplyStanceResupplyOnly->SetHighlightState(false);

					stanceAttack->SetVisible(true);
					stanceStand->SetVisible(true);
					stanceStop->SetVisible(true);
					stanceDefend->SetVisible(true);
					stanceAttack->EnableButton(true);
					stanceDefend->EnableButton(true);
					stanceStop->EnableButton(true);
					stanceStand->EnableButton(true);
					UNIT_STANCE stance = attack->GetUnitStance();
					if(stance == US_STOP)
					{
						stanceAttack->SetHighlightState(false);
						stanceDefend->SetHighlightState(false);
						stanceStop->SetHighlightState(true);
						stanceStand->SetHighlightState(false);
					}
					else if(stance == US_ATTACK)
					{
						stanceAttack->SetHighlightState(true);
						stanceDefend->SetHighlightState(false);
						stanceStop->SetHighlightState(false);
						stanceStand->SetHighlightState(false);
					}
					else if(stance == US_DEFEND)
					{
						stanceAttack->SetHighlightState(false);
						stanceDefend->SetHighlightState(true);
						stanceStop->SetHighlightState(false);
						stanceStand->SetHighlightState(false);
					}
					else //US_STAND
					{
						stanceAttack->SetHighlightState(false);
						stanceDefend->SetHighlightState(false);
						stanceStop->SetHighlightState(false);
						stanceStand->SetHighlightState(true);
					}

					if(MGlobals::IsCarrier(ship->mObjClass))
					{
						fighterStanceNormal->SetVisible(true);
						fighterStancePatrol->SetVisible(true);
						fighterStanceNormal->EnableButton(true);
						fighterStancePatrol->EnableButton(true);

						FighterStance fStance = attack->GetFighterStance();
						if(fStance == FS_PATROL)
						{
							fighterStanceNormal->SetHighlightState(false);
							fighterStancePatrol->SetHighlightState(true);
						}
						else //FS_NORMAL
						{
							fighterStanceNormal->SetHighlightState(true);
							fighterStancePatrol->SetHighlightState(false);
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
					stanceAttack->SetHighlightState(false);
					stanceDefend->SetHighlightState(false);
					stanceStop->SetHighlightState(false);
					stanceStand->SetHighlightState(false);
					stanceAttack->EnableButton(false);
					stanceDefend->EnableButton(false);
					stanceStop->EnableButton(false);
					stanceStand->EnableButton(false);
					stanceAttack->SetVisible(false);
					stanceStand->SetVisible(false);
					stanceStop->SetVisible(false);
					stanceDefend->SetVisible(false);

					fighterStanceNormal->SetVisible(false);
					fighterStancePatrol->SetVisible(false);
					fighterStanceNormal->EnableButton(false);
					fighterStancePatrol->EnableButton(false);

					OBJPTR<ISupplier> supplier;
					obj->QueryInterface(ISupplierID,supplier);
					if(supplier)
					{
						supplyStanceNoAuto->SetVisible(true);
						supplyStanceAuto->SetVisible(true);
						supplyStanceResupplyOnly->SetVisible(true);
						supplyStanceNoAuto->EnableButton(true);
						supplyStanceAuto->EnableButton(true);
						supplyStanceResupplyOnly->EnableButton(true);

						if(supplier->GetSupplyStance() == SUP_STANCE_NONE)
						{
							supplyStanceAuto->SetHighlightState(false);
							supplyStanceNoAuto->SetHighlightState(true);
							supplyStanceResupplyOnly->SetHighlightState(false);	
						}
						else if(supplier->GetSupplyStance() == SUP_STANCE_RESUPPLY)
						{
							supplyStanceAuto->SetHighlightState(false);
							supplyStanceNoAuto->SetHighlightState(false);
							supplyStanceResupplyOnly->SetHighlightState(true);
						}
						else
						{
							supplyStanceAuto->SetHighlightState(true);
							supplyStanceNoAuto->SetHighlightState(false);
							supplyStanceResupplyOnly->SetHighlightState(false);
						}
					}
					else
					{
						supplyStanceAuto->SetHighlightState(false);
						supplyStanceNoAuto->SetHighlightState(false);
						supplyStanceResupplyOnly->SetHighlightState(false);
						supplyStanceAuto->EnableButton(false);
						supplyStanceNoAuto->EnableButton(false);
						supplyStanceResupplyOnly->EnableButton(false);
						supplyStanceAuto->SetVisible(false);
						supplyStanceNoAuto->SetVisible(false);
						supplyStanceResupplyOnly->SetVisible(false);
					}
				}
				
			}
		}
	}
	else
		timeoutCount -= dt;
}
//--------------------------------------------------------------------------//
//
void CMenu_Ind::enableMenu (bool bEnable)
{
	if((!bEnable) && bPanelOwned)
	{
		setPanelOwnership(false);
	}
}

void CMenu_Ind::setPanelOwnership (bool bOwn)
{
	if(bOwn && (!bPanelOwned))
	{
		COMPTR<IToolbar> toolbar;

		if (TOOLBAR->QueryInterface("IToolbar", toolbar) == GR_OK)
		{
			IBaseObject * obj = OBJLIST->GetSelectedList();
			MPart part(obj);
			if (toolbar->GetToolbar("individual", menu, part->race) == GR_OK)
			{
				COMPTR<IDAComponent> pComp;

				if (menu->GetControl("namearea", pComp) == GR_OK)
					pComp->QueryInterface("IEdit2", namearea);
				if (menu->GetControl("hull", pComp) == GR_OK)
					pComp->QueryInterface("IStatic", hull);
				if (menu->GetControl("supplies", pComp) == GR_OK)
					pComp->QueryInterface("IStatic", supplies);
				if (menu->GetControl("kills", pComp) == GR_OK)
					pComp->QueryInterface("IStatic", kills);
				if (menu->GetControl("metal", pComp) == GR_OK)
					pComp->QueryInterface("IStatic", metal);
				if (menu->GetControl("gas", pComp) == GR_OK)
					pComp->QueryInterface("IStatic", gas);
				if (menu->GetControl("techarmor", pComp) == GR_OK)
					pComp->QueryInterface("IHotStatic", techarmor);
				if (menu->GetControl("techsupply", pComp) == GR_OK)
					pComp->QueryInterface("IHotStatic", techsupply);
				if (menu->GetControl("techengine", pComp) == GR_OK)
					pComp->QueryInterface("IHotStatic", techengine);
				if (menu->GetControl("techsheild", pComp) == GR_OK)
					pComp->QueryInterface("IHotStatic", techsheild);
				if (menu->GetControl("techweapon", pComp) == GR_OK)
					pComp->QueryInterface("IHotStatic", techweapon);
				if (menu->GetControl("techsensors", pComp) == GR_OK)
					pComp->QueryInterface("IHotStatic", techsensors);
				if (menu->GetControl("techspecial", pComp) == GR_OK)
					pComp->QueryInterface("IHotStatic", techspecial);
				if (menu->GetControl("stop", pComp) == GR_OK)
					pComp->QueryInterface("IHotButton", stopCmd);
				if (menu->GetControl("sell", pComp) == GR_OK)
					pComp->QueryInterface("IHotButton", sellCmd);
				if (menu->GetControl("specialweapon", pComp) == GR_OK)
				{
					pComp->QueryInterface("IHotButton", specialWpnCmd);
					pComp->QueryInterface("IMultiHotButton",specialWpnMulti);
				}
				if (menu->GetControl("specialweapon1", pComp) == GR_OK)
				{
					pComp->QueryInterface("IHotButton", specialWpnCmd1);
					pComp->QueryInterface("IMultiHotButton",specialWpnMulti1);
				}
				if (menu->GetControl("specialweapon2", pComp) == GR_OK)
				{
					pComp->QueryInterface("IHotButton", specialWpnCmd2);
					pComp->QueryInterface("IMultiHotButton",specialWpnMulti2);
				}
				if (menu->GetControl("patrol", pComp) == GR_OK)
					pComp->QueryInterface("IHotButton", patrolCmd);
				if (menu->GetControl("escort", pComp) == GR_OK)
					pComp->QueryInterface("IHotButton", escortCmd);
				if (menu->GetControl("cloak", pComp) == GR_OK)
					pComp->QueryInterface("IHotButton", cloak);
				if (menu->GetControl("attackPosition", pComp) == GR_OK)
					pComp->QueryInterface("IHotButton", attackPosition);
			
				if (toolbar->GetControl("shipclass", pComp) == GR_OK)
					pComp->QueryInterface("IStatic", shipclass);
				if(namearea)
					namearea->EnableEdit(true);

				if (menu->GetControl("stanceAttack", pComp) == GR_OK)
					pComp->QueryInterface("IHotButton", stanceAttack);
				if (menu->GetControl("stanceDefend", pComp) == GR_OK)
					pComp->QueryInterface("IHotButton", stanceDefend);
				if (menu->GetControl("stanceStand", pComp) == GR_OK)
					pComp->QueryInterface("IHotButton", stanceStand);
				if (menu->GetControl("stanceStop", pComp) == GR_OK)
					pComp->QueryInterface("IHotButton", stanceStop);
				if (menu->GetControl("supplyStanceAuto", pComp) == GR_OK)
					pComp->QueryInterface("IHotButton", supplyStanceAuto);
				if (menu->GetControl("supplyStanceNoAuto", pComp) == GR_OK)
					pComp->QueryInterface("IHotButton", supplyStanceNoAuto);
				if (menu->GetControl("supplyStanceResupplyOnly", pComp) == GR_OK)
					pComp->QueryInterface("IHotButton", supplyStanceResupplyOnly);
				if (menu->GetControl("fighterStanceNormal", pComp) == GR_OK)
					pComp->QueryInterface("IHotButton", fighterStanceNormal);
				if (menu->GetControl("fighterStancePatrol", pComp) == GR_OK)
					pComp->QueryInterface("IHotButton", fighterStancePatrol);

				if (menu->GetControl("ejectArtifact", pComp) == GR_OK)
					pComp->QueryInterface("IHotButton", ejectArtifact);
				if (menu->GetControl("artifact", pComp) == GR_OK)
				{
					pComp->QueryInterface("IHotButton", useArtifactHB);
					pComp->QueryInterface("IMultiHotButton", useArtifactMB);
				}
			}

		
			COMPTR<IDAConnectionPoint> connection;

			if (TOOLBAR->QueryOutgoingInterface("IHotControlEvent", connection) == GR_OK)
				connection->Advise(getBase(), &hotEventHandle);

			if(menu)
				menu->SetVisible(true);
			bPanelOwned = true;
		}
	}else if((!bOwn) && bPanelOwned)
	{
		if(menu)
			menu->SetVisible(false);
		bPanelOwned = false;
		if(TOOLBAR)
		{
			COMPTR<IDAConnectionPoint> connection;
		
			if (TOOLBAR && TOOLBAR->QueryOutgoingInterface("IHotControlEvent", connection) == GR_OK)
				connection->Unadvise(hotEventHandle);
			hotEventHandle = 0;
		}
	
		menu.free();
		shipclass.free();
		hull.free();
		supplies.free();
		kills.free();
		metal.free();
		gas.free();
		techarmor.free();
		techsupply.free();
		techengine.free();
		techsheild.free();
		techweapon.free();
		techsensors.free();
		techspecial.free();
		namearea.free();
		sellCmd.free();
		stopCmd.free();
		specialWpnCmd.free();
		specialWpnMulti.free();
		specialWpnCmd1.free();
		specialWpnMulti1.free();
		specialWpnCmd2.free();
		specialWpnMulti2.free();
		patrolCmd.free();
		escortCmd.free();
		cloak.free();
		attackPosition.free();
		stanceAttack.free();		
		stanceDefend.free();		
		stanceStand.free();		
		stanceStop.free();		
		fighterStanceNormal.free();
		fighterStancePatrol.free();
		supplyStanceAuto.free();		
		supplyStanceNoAuto.free();		
		supplyStanceResupplyOnly.free();
		useArtifactHB.free();
		useArtifactMB.free();
		ejectArtifact.free();
		lastMissionID = 0;
	}
}

//--------------------------------------------------------------------------//
//
struct _cmenu_ind : GlobalComponent
{
	CMenu_Ind * menu;

	virtual void Startup (void)
	{
		menu = new DAComponent<CMenu_Ind>;
		AddToGlobalCleanupList(&menu);
	}

	virtual void Initialize (void)
	{
		COMPTR<IDAConnectionPoint> connection;

		if (TOOLBAR->QueryOutgoingInterface("IEventCallback", connection) == GR_OK)
			connection->Advise(menu->getBase(), &menu->eventHandle);
	}
};

static _cmenu_ind startup;

//--------------------------------------------------------------------------//
//-----------------------------End CMenu_Ind.cpp----------------------------//
//--------------------------------------------------------------------------//
