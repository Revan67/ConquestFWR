//--------------------------------------------------------------------------//
//                                                                          //
//                              CMenu_Fleet.cpp                               //
//                                                                          //
//               COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
   $Header: /Conquest/App/Src/CMenu_Fleet.cpp 35    5/07/01 9:21a Tmauer $
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
#include "IShipSilButton.h"
#include "IEdit2.h"
#include "IHotButton.h"
#include "IStatic.h"
#include "IAttack.h"
#include "ISupplier.h"
#include "IHotStatic.h"
#include "IAdmiral.h"
#include "IMultiHotButton.h"
#include "ITabControl.h"
#include "IMissionActor.h"
#include "IQueueControl.h"
#include "IFabricator.h"
#include "GenData.h"
#include "CommPacket.h"

#include <DMBaseData.h>
#include <DSupplyShipSave.h>
#include <DFlagship.h>
#include <DToolbar.h>
#include <DSpecialData.h>

#include <TComponent.h>
#include <IConnection.h>
#include <HeapObj.h>
#include <TSmartPointer.h>
#include <EventSys.h>

#include <stdio.h>

#define TIMEOUT_PERIOD 500
#define MAX_NUM_SEL_SHIPS 22
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//

/*  old settings...
// this includes the assault attack
#define NUM_SPECIAL_ATTACKS 19

#define TERRAN_BEGIN   1
#define TERRAN_END     6
#define MANTIS_BEGIN   6
#define MANTIS_END     12
#define SOLARIAN_BEGIN 12
#define SOLARIAN_END   19

#define NUM_TERRAN		TERRAN_END - TERRAN_BEGIN
#define NUM_MANTIS		MANTIS_END - MANTIS_BEGIN
#define NUM_SOLARIAN	SOLARIAN_END - SOLARIAN_BEGIN
*/

U32 GetArmorUpgrades(M_RACE race);
U32 GetShieldUpgrades(M_RACE race);
U32 GetEngineUpgrades(M_RACE race);
U32 GetSensorUpgrades(M_RACE race);
U32 GetSuppliesUpgrades(M_RACE race);
U32 GetWeaponsUpgrades(M_RACE race);
U32 GetFleetUpgrades(M_RACE race);

MISSION_DATA::M_CAPS getCaps (IBaseObject * selected);

struct DACOM_NO_VTABLE CMenu_Fleet : public IEventCallback, IHotControlEvent, IFleetMenu
{
	BEGIN_DACOM_MAP_INBOUND(CMenu_Fleet)
	DACOM_INTERFACE_ENTRY(IEventCallback)
	DACOM_INTERFACE_ENTRY(IHotControlEvent)
	END_DACOM_MAP()

	U32 eventHandle, hotEventHandle;		// connection handle
	bool bPanelOwned;
	S32 timeoutCount;

	COMPTR<IToolbar> menu;
	COMPTR<IStatic> shipclass;
	
	// order tab
	COMPTR<IMultiHotButton> admiralHead;
	COMPTR<IStatic> o_hull,o_kills,o_namearea;
	COMPTR<IHotButton> hotCreate, hotRepair, hotResupply, hotDisband, hotTransfer, hotAssault;
	U32 nSpecialEnabled[NUM_SPECIAL_ATTACKS]; // which special weapons are enabled? 0 == assault
	U32 nSpecialAssigned[NUM_SPECIAL_ORDERS];//map the order buttons to the special attacks.
	COMPTR<IMultiHotButton> specialOrders[NUM_SPECIAL_ORDERS];
	COMPTR<IHotButton> specialOrdersHB[NUM_SPECIAL_ORDERS];

	// sil tab
	COMPTR<IShipSilButton> shipStatus[MAX_NUM_SEL_SHIPS];
	// stat tab
	COMPTR<IStatic> hull,kills,namearea;
	COMPTR<IHotStatic> techarmor,techsupply,techengine,techsheild,techweapon,techsensors,techspecial;
	COMPTR<IHotButton> tacticPeace,tacticStandGround, tacticDefend, tacticSeek, attackPosition;
	COMPTR<IMultiHotButton> formation1, formation2, formation3, formation4, formation5, formation6;
	COMPTR<IHotButton> formation1BT, formation2BT, formation3BT, formation4BT, formation5BT, formation6BT;

	// kit tab
	COMPTR<IStatic> k_hull,k_kills,k_namearea;
	COMPTR<IMultiHotButton> kit[MAX_COMMAND_KITS];
	COMPTR<IHotButton> kitHB[MAX_COMMAND_KITS];
	COMPTR<IMultiHotButton> kitDisplay[MAX_KNOWN_KITS];
	COMPTR<IHotButton> kitDisplayHB[MAX_KNOWN_KITS];
	COMPTR<IQueueControl> kitQueue;

	U32 admiralKey;
	U32 dwAdmiralID;
	U32 oldAdmiralID;

	CMenu_Fleet (void);

	~CMenu_Fleet (void);

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
	}

	virtual void OnLeftDblButtonEvent (U32 menuID, U32 controlID)		// double-click
	{
	}

	// IFleetMenu
	virtual SpecialAttack GetSpecialAttackType(U32 buttinIndex);
	
	/* CMenu_Fleet methods */

	void onUpdate (U32 dt);	// in mseconds

	void enableMenu (bool bEnable);

	void setPanelOwnership (bool bOwn);

	IDAComponent * getBase (void)
	{
		return static_cast<IEventCallback *>(this);
	}

	void findSpecialsEnabled (void);

	UNIT_SPECIAL_ABILITY findSpecialForOrder(SpecialAttack order);

	bool findLowestAdmiral (IBaseObject * & admiral, U32 & admiralID);

	void updateQueue (IBaseObject * obj);

	void findSpecialPlats (void);
};
//--------------------------------------------------------------------------//
//
CMenu_Fleet::CMenu_Fleet (void)
{
	// nothing to init so far?
}
//--------------------------------------------------------------------------//
//
CMenu_Fleet::~CMenu_Fleet (void)
{
	if (TOOLBAR)
	{
		COMPTR<IDAConnectionPoint> connection;
		
		if (TOOLBAR->QueryOutgoingInterface("IEventCallback", connection) == GR_OK)
			connection->Unadvise(eventHandle);
	}

	FLEET_MENU = NULL;
}
//--------------------------------------------------------------------------//
// receive notifications from event system
//
GENRESULT CMenu_Fleet::Notify (U32 message, void *param)
{
//	MSG *msg = (MSG *) param;

	switch (message)
	{
	case CQE_PANEL_OWNED:
		if (((CMenu_Fleet *)param) != this)
			setPanelOwnership(false);
		break;

	case CQE_LOAD_INTERFACE:
		enableMenu(param!=0);
		break;

	case CQE_UPDATE:
		onUpdate(U32(param) >> 10);
		break;
	case CQE_BUILDQUEUE_REMOVE:
		{
			if(bPanelOwned)
			{
				IBaseObject * obj = NULL;
				U32 admiralID = 0;
				if(findLowestAdmiral(obj,admiralID))
				{
					USR_PACKET<USRBUILD> packet;
					packet.cmd = USRBUILD::REMOVE;
					packet.dwArchetypeID = ((U32)param);
					packet.objectID[0] = obj->GetPartID();
					packet.init(1, false);
					NETPACKET->Send(HOSTID,0,&packet);
				}
			}
			break;
		}
	}

	return GR_OK;
}
//--------------------------------------------------------------------------//
//
SpecialAttack CMenu_Fleet::GetSpecialAttackType(U32 buttonIndex)
{
	return (SpecialAttack)(nSpecialAssigned[buttonIndex]);
}
//--------------------------------------------------------------------------//
//
void CMenu_Fleet::findSpecialsEnabled (void)
{
	// look for repair and supply platforms every 4 updates
	static U32 lookForPlats = 0;
	if (lookForPlats++ % 4)
	{
		findSpecialPlats();
	}

	IBaseObject * obj = OBJLIST->GetSelectedList();
	int numUnits = 0;
	int numDockableUnits = 0;
		
	// zero out all the buttons
	memset(nSpecialEnabled, 0, sizeof(U32) * NUM_SPECIAL_ATTACKS);
    
	while (obj)
	{
		MPart part = obj;
		if (part.isValid())
		{
			// which buttons are enabled?
			if (MGlobals::IsTroopship(part->mObjClass))
			{
				nSpecialEnabled[SA_ASSAULT]++;
			}
			else if (MGlobals::IsGunboat(part->mObjClass))
			{
				// which special abilities does this gunboat have?
				OBJPTR<IAttack> attack;
				if (obj->QueryInterface(IAttackID, attack))
				{
					UNIT_SPECIAL_ABILITY usa;
					bool bSpecialEnabled;
					int index = -1;

					// do we have any dockable units defined in this fleet?
					if (MGlobals::IsGunboat(part->mObjClass) && part->fleetID == dwAdmiralID && part->admiralID == 0)
					{
						numDockableUnits++;
					}

					attack->GetSpecialAbility(usa, bSpecialEnabled);
 
					switch (usa)
					{
					case USA_NONE:
						break;

					case USA_AEGIS:
						index = SA_AEGIS;
						break;

					case USA_TEMPEST:
						index = SA_TEMPEST;
						break;

					case USA_CLOAK:
						index = SA_CLOAK;
						break;

					case USA_PROBE:
						index = SA_PROBE;
						break;

					case USA_VAMPIRE:
						index = SA_VAMPIRE;
						break;

					case USA_STASIS:
						index = SA_STASIS;
						break;

					case USA_FURYRAM:
						index = SA_FURYRAM;
						break;

					case USA_REPEL:
						index = SA_REPEL;
						break;

					case USA_REPULSOR:
						index = SA_REPULSOR;
						break;

					case USA_MIMIC:
						index = SA_MIMIC;
						break;

/*					case USA_MINELAYER:
						// we have two mine layers the mantis one and solarian
						if (part->race == M_MANTIS)
						{
							index = SA_MINELAYER;
						}
						else
						{
							index = SA_PROTEUSMINE;
						}
						// temporarily fucked up
						break;
*/

					case USA_SYNTHESIS:
						index = SA_SYNTHESIS;
						break;

					case USA_MASS_DISRUPTOR:
						index = SA_MASSDISRUPT;
						break;

					case USA_DESTABILIZER:
						index = SA_DESTABILIZER;
						break;

					case USA_MULTICLOAK:
						index = SA_SHROUD;
						break;
	 
					case USA_TRACTOR:
						index = SA_AUGER;
						break;
					}

					if (bSpecialEnabled && index > -1)
					{
						nSpecialEnabled[index]++;
					}

					// make sure both cloak buttons are enabled
					if (usa == USA_CLOAK && bSpecialEnabled)
					{
						nSpecialEnabled[SA_SOLARIANCLOAK]++;
					}
					else if (usa == USA_MULTICLOAK && bSpecialEnabled)
					{
						nSpecialEnabled[SA_CLOAK]++;
						nSpecialEnabled[SA_SOLARIANCLOAK]++;
					}
				}
			}
		}
		obj = obj->nextSelected;
		numUnits++;
	}
	
	// set the buttons to be enabled or not
	
	// is the assault button enabled?
	hotAssault->EnableButton(nSpecialEnabled[SA_ASSAULT] > 0);

	// do we have dockable units *defined* in our fleet
	hotTransfer->EnableButton(numDockableUnits > 0);

	U32 lastOrderIndex = 0;
	for(U32 count = 0; count < NUM_SPECIAL_ATTACKS && lastOrderIndex<NUM_SPECIAL_ORDERS; ++count)
	{
		if(nSpecialEnabled[count])
		{
			nSpecialAssigned[lastOrderIndex] = count;
			UNIT_SPECIAL_ABILITY usa = findSpecialForOrder((SpecialAttack)count);
			const _GT_SPECIALABILITIES * specialAbilities = (const _GT_SPECIALABILITIES *) GENDATA->GetArchetypeData("SpecialAbilities");
			CQASSERT(specialAbilities);
			CQASSERT(sizeof(_GT_SPECIALABILITIES) == sizeof(GT_SPECIALABILITIES) && "Data definition error");
			const SPECIALABILITYINFO * specialInfo = specialAbilities->info + usa;

			specialOrders[lastOrderIndex]->SetState(specialInfo->baseSpecialWpnButton,specialInfo->specialWpnTooltip,specialInfo->specialWpnHelpBox,specialInfo->specialWpnHintBox);
			specialOrdersHB[lastOrderIndex]->SetVisible(true);
			++lastOrderIndex;
		}
	}
	for(; lastOrderIndex < NUM_SPECIAL_ORDERS; ++lastOrderIndex)
	{
		nSpecialAssigned[lastOrderIndex] = -1;
		specialOrdersHB[lastOrderIndex]->SetVisible(false);
	}
}
//--------------------------------------------------------------------------//
//
UNIT_SPECIAL_ABILITY CMenu_Fleet::findSpecialForOrder(SpecialAttack order)
{
	switch(order)
	{
	case SA_ASSAULT:
		return USA_ASSAULT;
		break;
	case SA_AEGIS:
		return USA_AEGIS;
		break;
	case SA_PROBE:
		return USA_PROBE;
		break;
	case SA_CLOAK:
		return USA_CLOAK;
		break;
	case SA_VAMPIRE:
		return USA_VAMPIRE;
		break;
	case SA_TEMPEST:
		return USA_TEMPEST;
		break;
	case SA_STASIS:
		return USA_STASIS;
		break;
	case SA_FURYRAM:
		return USA_FURYRAM;
		break;
	case SA_REPEL:
		return USA_REPEL;
		break;
	case SA_REPULSOR:
		return USA_REPULSOR;
		break;
	case SA_MIMIC:
		return USA_MIMIC;
		break;
	case SA_SYNTHESIS:
		return USA_SYNTHESIS;
		break;
	case SA_MASSDISRUPT:
		return USA_MASS_DISRUPTOR;
		break;
	case SA_DESTABILIZER:
		return USA_DESTABILIZER;
		break;
	case SA_SOLARIANCLOAK:
		return USA_CLOAK;
		break;
	case SA_SHROUD:
		return USA_MULTICLOAK;
		break;
	case SA_AUGER:
		return USA_TRACTOR;
		break;
	}
	return USA_NONE;
}
//--------------------------------------------------------------------------//
//
void CMenu_Fleet::onUpdate (U32 dt)
{
	if (bPanelOwned == false || S32(timeoutCount-dt) <= 0)
	{
		timeoutCount = TIMEOUT_PERIOD;

		//see if I have multiple objects selected object that are not all fabricators
		IBaseObject * admiral=0;
		U32 admiralID = 0;
		bool bKeepIt = findLowestAdmiral(admiral, admiralID);

		// tell the other admiral bar if it is visible or not (more like active or not)
		if (ADMIRALBAR)
		{
			ADMIRALBAR->bInvisible = !bKeepIt;
		}

		if (bKeepIt == false && bPanelOwned) //If I do not and I am owning the panel then deactive myself
		{
			setPanelOwnership(false);
		}
		else
		if (bKeepIt && bPanelOwned==false) //else if I do have a selection and am not owning the panel activate myself
		{
			TOOLBAR->Notify(CQE_PANEL_OWNED, this);
			setPanelOwnership(true);
		}
		 		
		if (bKeepIt)
		{
			MPart admiralPart = admiral;
			VOLPTR(IMissionActor) actor = admiral;
			VOLPTR(IAdmiral) admiralPtr = admiral;

			CQASSERT(admiralPart && admiralPart.isValid());
			
			oldAdmiralID = dwAdmiralID;

			// we have an admiral that is selected find out which buttons should be enabled, fool
			findSpecialsEnabled();

			wchar_t buffer[256];
			wchar_t name[128];
			wchar_t * ptr;

			wcscpy(name, _localLoadStringW(admiralPart.pInit->displayName));
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
			swprintf(buffer, _localLoadStringW(IDS_IND_SHIPCLASS), ptr);
			shipclass->SetText(buffer);
			
			IBaseObject * obj = OBJLIST->GetSelectedList();
			bool bHasStance = false;
			bool bHasMultiStance = false;
			UNIT_STANCE stance = US_ATTACK;
			while(obj)
			{
				OBJPTR<IAttack> attack;
				obj->QueryInterface(IAttackID,attack);
				MPart ship(obj);
				if(attack && obj->objClass == OC_SPACESHIP)
				{
					if(bHasStance)
					{
						if(stance != attack->GetUnitStance())
						{
							bHasMultiStance = true;
							break;
						}
					}
					else
					{
						bHasStance = true;
						stance = attack->GetUnitStance();
					}
				}
				obj = obj->nextSelected;
			}

			//order tab
			_localAnsiToWide(admiralPart->partName,buffer,sizeof(buffer));
			wchar_t * namePtr = buffer;
			if(namePtr[0] == '#')
			{
				++namePtr;
				if ((namePtr = wcschr(namePtr,'#')) != 0)
					++namePtr;
				else
					namePtr = buffer;
			}

			admiralHead->SetState(admiralPtr->GetToolbarImage(),admiralPtr->GetToolbarText(),admiralPtr->GetToolbarStatusText(),admiralPtr->GetToolbarHintbox());

			namearea->SetText(namePtr);
			o_namearea->SetText(namePtr);
			k_namearea->SetText(namePtr);

			swprintf(buffer, _localLoadStringW(IDS_IND_HULL), (actor) ? actor->GetDisplayHullPoints() : admiralPart->hullPoints, admiralPart->hullPointsMax);
			hull->SetText(buffer);
			IBaseObject * dockShip = admiralPtr->GetDockship();
			if(dockShip)
			{
				MPart dockPart(dockShip);
				VOLPTR(IMissionActor) actor = dockShip;
				swprintf(buffer, _localLoadStringW(IDS_IND_HULL), (actor) ? actor->GetDisplayHullPoints() : dockPart->hullPoints, dockPart->hullPointsMax);
			}
			o_hull->SetText(buffer);
			k_hull->SetText(buffer);

			swprintf(buffer, _localLoadStringW(IDS_IND_KILLS),(U32)(admiralPart->numKills));
			o_kills->SetText(buffer);
			k_kills->SetText(buffer);
			kills->SetText(buffer);

			//sil tab
			obj = OBJLIST->GetSelectedList();
			while(obj->nextSelected)
				obj = obj->nextSelected;
			for(U32 index = 0; index < MAX_NUM_SEL_SHIPS; ++index)
			{
				if(shipStatus[index] != 0)
				{
					if(obj)
					{
						if(shipStatus[index]->GetShip() != obj)
							shipStatus[index]->SetShip(obj);
						obj = obj->prevSelected;
					}
					else
						shipStatus[index]->SetShip(NULL);
				}
			}

			//stat tab

			techarmor->SetImageLevel(admiralPart->techLevel.hull,GetArmorUpgrades(admiralPart->race));
			techsupply->SetImageLevel(admiralPart->techLevel.supplies,GetSuppliesUpgrades(admiralPart->race));
			techengine->SetImageLevel(admiralPart->techLevel.engine,GetEngineUpgrades(admiralPart->race));
			techsheild->SetImageLevel(admiralPart->techLevel.shields,GetShieldUpgrades(admiralPart->race));
			techweapon->SetImageLevel(admiralPart->techLevel.damage,GetWeaponsUpgrades(admiralPart->race));
			techsensors->SetImageLevel(admiralPart->techLevel.sensors,GetSensorUpgrades(admiralPart->race));
//			techspecial->SetImageLevel(admiralPart->techLevel.classSpecific,GetFleetUpgrades(admiralPart->race));
//			techspecial->SetTextString(HSTTXT::FLEET);
			techspecial->SetVisible(false);


			ADMIRAL_TACTIC tactic = admiralPtr->GetAdmiralTactic();
			switch(tactic)
			{
			case AT_PEACE:
				tacticPeace->SetHighlightState(true);
				tacticStandGround->SetHighlightState(false);
				tacticDefend->SetHighlightState(false);
				tacticSeek->SetHighlightState(false);
				break;
			case AT_HOLD:
				tacticPeace->SetHighlightState(false);
				tacticStandGround->SetHighlightState(true);
				tacticDefend->SetHighlightState(false);
				tacticSeek->SetHighlightState(false);
				break;
			case AT_DEFEND:
				tacticPeace->SetHighlightState(false);
				tacticStandGround->SetHighlightState(false);
				tacticDefend->SetHighlightState(true);
				tacticSeek->SetHighlightState(false);
				break;
			case AT_SEEK:
				tacticPeace->SetHighlightState(false);
				tacticStandGround->SetHighlightState(false);
				tacticDefend->SetHighlightState(false);
				tacticSeek->SetHighlightState(true);
				break;
			}

			MISSION_DATA::M_CAPS caps = getCaps(OBJLIST->GetSelectedList());
			attackPosition->SetVisible(caps.targetPositionOk);

			//kit tab
			bool bCanResearch = admiralPtr->CanResearchKits();
			TECHNODE currentTech = MGlobals::GetCurrentTechLevel(admiral->GetPlayerID());
			U32 count;
			for(count = 0; count < MAX_COMMAND_KITS; ++count)
			{
				bool bAvailible = false;
				BT_COMMAND_KIT * comKit = admiralPtr->GetAvailibleCommandKit(count);
				if(comKit)
				{
					kit[count]->SetBuildCost(comKit->cost);
					kit[count]->SetState(comKit->buttonInfo.baseButton,comKit->buttonInfo.tooltip,comKit->buttonInfo.helpBox,comKit->buttonInfo.hintBox);
					if(currentTech.HasTech(comKit->dependancy))
					{
						if(!(admiralPtr->IsKnownKit(comKit)))
						{
							VOLPTR(IBuildQueue) queue = admiral;
							if(queue && (!(queue->IsInQueue(admiralPtr->GetAvailibleCommandKitID(count)))))
							{
								bAvailible = true;
							}
						}
					}
				}

				kitHB[count]->EnableButton(bCanResearch);

				kitHB[count]->SetVisible(bAvailible);
			}
			for(count = 0; count < MAX_KNOWN_KITS; ++count)
			{
				BT_COMMAND_KIT * comKit = admiralPtr->GetKnownCommandKit(count);
				if(comKit)
				{
					kitDisplay[count]->SetState(comKit->buttonInfo.baseButton,comKit->buttonInfo.tooltip,comKit->buttonInfo.helpBox,comKit->buttonInfo.hintBox);
					kitDisplayHB[count]->SetVisible(true);
				}
				else
				{
					kitDisplayHB[count]->SetVisible(false);
				}
				kitDisplayHB[count]->EnableButton(false);
			}

			updateQueue(admiral);

			//update formations
			//0
			U32 formation = admiralPtr->GetKnownFormation(0);
			if(formation)
			{
				formation1BT->SetVisible(true);
				BT_FORMATION * pFormation = (BT_FORMATION *) (ARCHLIST->GetArchetypeData(formation));
				formation1->SetState(pFormation->buttonInfo.baseButton,pFormation->buttonInfo.tooltip,pFormation->buttonInfo.helpBox,pFormation->buttonInfo.hintBox);
				if(formation == admiralPtr->GetFormation())
					formation1BT->SetHighlightState(true);
				else
					formation1BT->SetHighlightState(false);
			}
			else
			{
				formation1BT->SetVisible(false);
			}
			//1
			formation = admiralPtr->GetKnownFormation(1);
			if(formation)
			{
				formation2BT->SetVisible(true);
				BT_FORMATION * pFormation = (BT_FORMATION *) (ARCHLIST->GetArchetypeData(formation));
				formation2->SetState(pFormation->buttonInfo.baseButton,pFormation->buttonInfo.tooltip,pFormation->buttonInfo.helpBox,pFormation->buttonInfo.hintBox);
				if(formation == admiralPtr->GetFormation())
					formation2BT->SetHighlightState(true);
				else
					formation2BT->SetHighlightState(false);
			}
			else
			{
				formation2BT->SetVisible(false);
			}
			//2
			formation = admiralPtr->GetKnownFormation(2);
			if(formation)
			{
				formation3BT->SetVisible(true);
				BT_FORMATION * pFormation = (BT_FORMATION *) (ARCHLIST->GetArchetypeData(formation));
				formation3->SetState(pFormation->buttonInfo.baseButton,pFormation->buttonInfo.tooltip,pFormation->buttonInfo.helpBox,pFormation->buttonInfo.hintBox);
				if(formation == admiralPtr->GetFormation())
					formation3BT->SetHighlightState(true);
				else
					formation3BT->SetHighlightState(false);
			}
			else
			{
				formation3BT->SetVisible(false);
			}
			//3
			formation = admiralPtr->GetKnownFormation(3);
			if(formation)
			{
				formation4BT->SetVisible(true);
				BT_FORMATION * pFormation = (BT_FORMATION *) (ARCHLIST->GetArchetypeData(formation));
				formation4->SetState(pFormation->buttonInfo.baseButton,pFormation->buttonInfo.tooltip,pFormation->buttonInfo.helpBox,pFormation->buttonInfo.hintBox);
				if(formation == admiralPtr->GetFormation())
					formation4BT->SetHighlightState(true);
				else
					formation4BT->SetHighlightState(false);
			}
			else
			{
				formation4BT->SetVisible(false);
			}
			//4
			formation = admiralPtr->GetKnownFormation(4);
			if(formation)
			{
				formation5BT->SetVisible(true);
				BT_FORMATION * pFormation = (BT_FORMATION *) (ARCHLIST->GetArchetypeData(formation));
				formation5->SetState(pFormation->buttonInfo.baseButton,pFormation->buttonInfo.tooltip,pFormation->buttonInfo.helpBox,pFormation->buttonInfo.hintBox);
				if(formation == admiralPtr->GetFormation())
					formation5BT->SetHighlightState(true);
				else
					formation5BT->SetHighlightState(false);
			}
			else
			{
				formation5BT->SetVisible(false);
			}
			//5
			formation = admiralPtr->GetKnownFormation(5);
			if(formation)
			{
				formation6BT->SetVisible(true);
				BT_FORMATION * pFormation = (BT_FORMATION *) (ARCHLIST->GetArchetypeData(formation));
				formation6->SetState(pFormation->buttonInfo.baseButton,pFormation->buttonInfo.tooltip,pFormation->buttonInfo.helpBox,pFormation->buttonInfo.hintBox);
				if(formation == admiralPtr->GetFormation())
					formation6BT->SetHighlightState(true);
				else
					formation6BT->SetHighlightState(false);
			}
			else
			{
				formation6BT->SetVisible(false);
			}
		}
		else
		{
			oldAdmiralID = 0;
		}
	}
	else
		timeoutCount -= dt;
}
//----------------------------------------------------------------------------------//
//
void CMenu_Fleet::findSpecialPlats (void)
{
	// look for plats that can repair and/or resupply
	// don't concern yourself witht the closest plat, just find what you need

	//
	// find closest planet with an opening
	//
	IBaseObject * obj = OBJLIST->GetTargetList();     // mission objects

	const U32 playerID = MGlobals::GetThisPlayer();
	const U32 allyMask = MGlobals::GetAllyMask(playerID);

	bool bRepairOk = false;
	bool bSupplyOk = false;
	MPart other;
	

	// keep looking until we are done with the list or we've found the plats we're looking for
	while (obj != NULL && (bSupplyOk == false || bRepairOk == false))
	{
		// is the object a friendly platform?
		if ((other = obj).isValid() && other.obj->objClass == OC_PLATFORM)
		{
			// is the platform friendly?
			if (other->playerID && (((1 << (other->playerID-1)) & allyMask) != 0))
			{
				// is the object visible to us? (may be a teammate we haven't seen yet)
				if (obj->IsVisibleToPlayer(playerID))
				{
					// do we have the caps we need?
					if (other->caps.repairOk)
					{
						bRepairOk = true;
					}
					if (MGlobals::IsTenderPlat(other->mObjClass))
					{
						bSupplyOk = true;
					}
				}
			}
		} 
		
		obj = obj->nextTarget;
	}
	
	// now enable or disable to proper hot buttons
	hotResupply->EnableButton(bSupplyOk);
	hotRepair->EnableButton(bRepairOk);
}
//--------------------------------------------------------------------------//
//
void CMenu_Fleet::updateQueue (IBaseObject * obj)
{
	kitQueue->ResetQueue();
	if (obj)
	{
		MPart part = obj;
		OBJPTR<IBuildQueue> fab;
		obj->QueryInterface(IBuildQueueID,fab);
		VOLPTR(IAdmiral) admiral = obj;
		if(fab && admiral)
		{
			U32 queue[FAB_MAX_QUEUE_SIZE];
			U32 slotIDs[FAB_MAX_QUEUE_SIZE];
			U32 numInQueue = fab->GetQueue(queue,slotIDs);
			if(numInQueue)
			{
				U32 stallType;
				SINGLE progress = fab->FabGetDisplayProgress(stallType);
				kitQueue->SetPercentage(progress,stallType);
				for(U32 i = 0; i < numInQueue; ++i)
				{
					IDrawAgent * queueShape[GTHBSHP_MAX_SHAPES];
					queueShape[0] = 0;
					U32 j;
					for(j = 0; j <MAX_COMMAND_KITS;++j)
					{
						if(kit[j])
						{
							if(queue[i] == admiral->GetAvailibleCommandKitID(j))
							{
								kit[j]->GetShape(queueShape);
								break;
							}
						}
					}

					if(queueShape[0])
						kitQueue->AddToQueue(queueShape,slotIDs[i]);
				}
			}
		}
	}
}
//----------------------------------------------------------------------------------//
//
bool CMenu_Fleet::findLowestAdmiral (IBaseObject * & admiral, U32 & admiralID)
{
	IBaseObject * obj = OBJLIST->GetSelectedList();
	MPart flagship = NULL;
	VOLPTR(IAdmiral) admiralObj;
	MPart part;

	admiralKey = 10000000; // much greater than highest value
	admiralID = 0;
	admiral = NULL;
	dwAdmiralID = 0;

	bool bFoundAdmiral = false;

	while (obj)
	{
		flagship = NULL;
		part = obj;

		if (part.isValid())
		{
			if (MGlobals::IsFlagship(part->mObjClass))
			{
				// this object is the admiral
				flagship = part;
			}
			else if (part->admiralID)
			{
				// this object contains the admiral
				 flagship = OBJLIST->FindObject(part->admiralID);
			}

			if (flagship && flagship.isValid())
			{
				admiralObj = flagship.obj;
				CQASSERT(admiralObj);

				U32 tmpKey = admiralObj->GetAdmiralHotkey();
				if (tmpKey < admiralKey)
				{
					admiralKey = tmpKey;
					admiral = admiralObj.Ptr();
					dwAdmiralID = admiralID = flagship->dwMissionID;
					bFoundAdmiral = true;
				}
			}
		}
		obj = obj->nextSelected;
	}

	return bFoundAdmiral;
}
//--------------------------------------------------------------------------//
//
void CMenu_Fleet::enableMenu (bool bEnable)
{
	if((!bEnable) && bPanelOwned)
	{
		setPanelOwnership(false);
	}
}

void CMenu_Fleet::setPanelOwnership (bool bOwn)
{
	if(bOwn)
	{
		COMPTR<IToolbar> toolbar;

		if (TOOLBAR->QueryInterface("IToolbar", toolbar) == GR_OK)
		{
			if (toolbar->GetToolbar("fleet", menu, M_TERRAN) == GR_OK)
			{
				COMPTR<IDAComponent> pComp;

				if (toolbar->GetControl("shipclass", pComp) == GR_OK)
					pComp->QueryInterface("IStatic", shipclass);

				//orderTab
				if (menu->GetControl("admiralHead", pComp) == GR_OK)
					pComp->QueryInterface("IMultiHotButton", admiralHead);
				if (menu->GetControl("o_namearea", pComp) == GR_OK)
					pComp->QueryInterface("IStatic", o_namearea);
				if (menu->GetControl("o_hull", pComp) == GR_OK)
					pComp->QueryInterface("IStatic", o_hull);
				if (menu->GetControl("o_kills", pComp) == GR_OK)
					pComp->QueryInterface("IStatic", o_kills);

				// get the pointers to the hotbuttons we need
				if (menu->GetControl("order1", pComp) == GR_OK)
				{
					pComp->QueryInterface("IHotButton", hotCreate);
				}
				if (menu->GetControl("order2", pComp) == GR_OK)
				{
					pComp->QueryInterface("IHotButton",hotDisband);
				}

				if (menu->GetControl("order3", pComp) == GR_OK)
				{
					pComp->QueryInterface("IHotButton", hotRepair);
				}
				if (menu->GetControl("order4", pComp) == GR_OK)
				{
					pComp->QueryInterface("IHotButton", hotResupply);
				}
			
				if (menu->GetControl("order5", pComp) == GR_OK)
				{
					pComp->QueryInterface("IHotButton", hotTransfer);
				}
				if (menu->GetControl("order6", pComp) == GR_OK)
				{
					pComp->QueryInterface("IHotButton", hotAssault);
				}

				U32 index;
				for(index = 0; index < NUM_SPECIAL_ORDERS; ++index)
				{
					char buffer[256];
					sprintf(buffer,"specialOrders%d",index);
					if (menu->GetControl(buffer, pComp) == GR_OK)
					{
						pComp->QueryInterface("IMultiHotButton", specialOrders[index]);					
						pComp->QueryInterface("IHotButton", specialOrdersHB[index]);					
					}
				}

				//sil tab
				for(index = 0; index < MAX_NUM_SEL_SHIPS; ++index)
				{
					char buffer[256];
					sprintf(buffer,"ship%d",index);
					if (menu->GetControl(buffer, pComp) == GR_OK)
						pComp->QueryInterface("IShipSilButton", shipStatus[index]);					
				}
		
				//stat tab
				if (menu->GetControl("namearea", pComp) == GR_OK)
					pComp->QueryInterface("IStatic", namearea);
				if (menu->GetControl("hull", pComp) == GR_OK)
					pComp->QueryInterface("IStatic", hull);
				if (menu->GetControl("kills", pComp) == GR_OK)
					pComp->QueryInterface("IStatic", kills);
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
				if (menu->GetControl("tacticPeace", pComp) == GR_OK)
					pComp->QueryInterface("IHotButton", tacticPeace);
				if (menu->GetControl("tacticStandGround", pComp) == GR_OK)
					pComp->QueryInterface("IHotButton", tacticStandGround);
				if (menu->GetControl("tacticDefend", pComp) == GR_OK)
					pComp->QueryInterface("IHotButton", tacticDefend);
				if (menu->GetControl("tacticSeek", pComp) == GR_OK)
					pComp->QueryInterface("IHotButton", tacticSeek);
				if (menu->GetControl("attackPosition", pComp) == GR_OK)
					pComp->QueryInterface("IHotButton", attackPosition);

				//kit tab
				if (menu->GetControl("k_namearea", pComp) == GR_OK)
					pComp->QueryInterface("IStatic", k_namearea);
				if (menu->GetControl("k_hull", pComp) == GR_OK)
					pComp->QueryInterface("IStatic", k_hull);
				if (menu->GetControl("k_kills", pComp) == GR_OK)
					pComp->QueryInterface("IStatic", k_kills);

				for(U32 count = 0; count < MAX_COMMAND_KITS; ++count)
				{
					char kitName[32];
					sprintf(kitName,"kit%d",count);
					if (menu->GetControl(kitName, pComp) == GR_OK)
					{
						pComp->QueryInterface("IMultiHotButton", kit[count]);
						pComp->QueryInterface("IHotButton", kitHB[count]);
					}
				}
				for(U32 count = 0; count < MAX_KNOWN_KITS; ++count)
				{
					char kitName[32];
					sprintf(kitName,"kitDisplay%d",count);
					if (menu->GetControl(kitName, pComp) == GR_OK)
					{
						pComp->QueryInterface("IMultiHotButton", kitDisplay[count]);
						pComp->QueryInterface("IHotButton", kitDisplayHB[count]);
					}
				}
				
				if (menu->GetControl("kitQueue", pComp) == GR_OK)
				{
					pComp->QueryInterface("IQueueControl", kitQueue);
				}		

				//fomrations
				if (menu->GetControl("formation1", pComp) == GR_OK)
				{
					pComp->QueryInterface("IMultiHotButton", formation1);
					pComp->QueryInterface("IHotButton", formation1BT);
				}
				if (menu->GetControl("formation2", pComp) == GR_OK)
				{
					pComp->QueryInterface("IMultiHotButton", formation2);
					pComp->QueryInterface("IHotButton", formation2BT);
				}
				if (menu->GetControl("formation3", pComp) == GR_OK)
				{
					pComp->QueryInterface("IMultiHotButton", formation3);
					pComp->QueryInterface("IHotButton", formation3BT);
				}
				if (menu->GetControl("formation4", pComp) == GR_OK)
				{
					pComp->QueryInterface("IMultiHotButton", formation4);
					pComp->QueryInterface("IHotButton", formation4BT);
				}
				if (menu->GetControl("formation5", pComp) == GR_OK)
				{
					pComp->QueryInterface("IMultiHotButton", formation5);
					pComp->QueryInterface("IHotButton", formation5BT);
				}
				if (menu->GetControl("formation6", pComp) == GR_OK)
				{
					pComp->QueryInterface("IMultiHotButton", formation6);
					pComp->QueryInterface("IHotButton", formation6BT);
				}
			}

		
			COMPTR<IDAConnectionPoint> connection;

			if (TOOLBAR->QueryOutgoingInterface("IHotControlEvent", connection) == GR_OK)
				connection->Advise(getBase(), &hotEventHandle);

			if(menu)
				menu->SetVisible(true);
			bPanelOwned = true;
		}
	}else{
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

		//orders
		o_hull.free();
		o_kills.free();
		o_namearea.free();
		admiralHead.free();

		// free all the stuff man
		tacticPeace.free();
		tacticStandGround.free();
		tacticDefend.free();
		tacticSeek.free();
		attackPosition.free();
		hotCreate.free();
		hotRepair.free();
		hotResupply.free();
		hotDisband.free();
		hotTransfer.free();
		hotAssault.free();

		U32 index;
		for(index = 0; index < NUM_SPECIAL_ORDERS; ++index)
		{
			specialOrders[index].free();
			specialOrdersHB[index].free();
		}

		//sil tab
		for (index = 0; index < MAX_NUM_SEL_SHIPS; ++index)
		{
			shipStatus[index].free();
		}

		// stat tab
		techarmor.free();
		techsupply.free();
		techengine.free();
		techsheild.free();
		techweapon.free();
		techsensors.free();
		techspecial.free();
		hull.free();
		kills.free();
		namearea.free();

		//kit tab
		k_hull.free();
		k_kills.free();
		k_namearea.free();
		U32 count;
		for(count = 0; count < MAX_COMMAND_KITS; ++count)
		{
			kit[count].free();
			kitHB[count].free();
		}
		for( count = 0; count < MAX_KNOWN_KITS; ++count)
		{
			kitDisplay[count].free();
			kitDisplayHB[count].free();
		}
		kitQueue.free();

		//formations
		formation1.free();
		formation2.free();
		formation3.free();
		formation4.free();
		formation5.free();
		formation6.free();
		formation1BT.free();
		formation2BT.free();
		formation3BT.free();
		formation4BT.free();
		formation5BT.free();
		formation6BT.free();
	}
}

//--------------------------------------------------------------------------//
//
struct _cmenu_fleet : GlobalComponent
{
	CMenu_Fleet * menu;

	virtual void Startup (void)
	{
		FLEET_MENU = menu = new DAComponent<CMenu_Fleet>;
		AddToGlobalCleanupList(&menu);
	}

	virtual void Initialize (void)
	{
		COMPTR<IDAConnectionPoint> connection;

		if (TOOLBAR->QueryOutgoingInterface("IEventCallback", connection) == GR_OK)
			connection->Advise(menu->getBase(), &menu->eventHandle);
	}
};

static _cmenu_fleet startup;

//--------------------------------------------------------------------------//
//-----------------------------End CMenu_Fleet.cpp----------------------------//
//--------------------------------------------------------------------------//
