//--------------------------------------------------------------------------//
//                                                                          //
//                              CMenu_MulMix.cpp                               //
//                                                                          //
//               COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
   $Header: /Conquest/App/Src/CMenu_MulMix.cpp 29    6/09/01 11:01 Tmauer $
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
#include "IMultiHotButton.h"
#include "GenData.h"
#include <DMBaseData.h>
#include <DSupplyShipSave.h>
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

inline MISSION_DATA::M_CAPS & MISSION_DATA::M_CAPS::operator |= (const MISSION_DATA::M_CAPS & other)
{
	CQASSERT(sizeof(*this) == sizeof(U32));
	U32 * ptr = (U32 *) this;
	*ptr |= *((U32 *) &other);

	return *this;
};
//--------------------------------------------------------------------------//
//
inline MISSION_DATA::M_CAPS & MISSION_DATA::M_CAPS::operator &= (const MISSION_DATA::M_CAPS & other)
{
	CQASSERT(sizeof(*this) == sizeof(U32));
	U32 * ptr = (U32 *) this;
	*ptr &= *((U32 *) &other);

	return *this;
}
struct DACOM_NO_VTABLE CMenu_MulMix : public IEventCallback, IHotControlEvent
{
	BEGIN_DACOM_MAP_INBOUND(CMenu_MulMix)
	DACOM_INTERFACE_ENTRY(IEventCallback)
	DACOM_INTERFACE_ENTRY(IHotControlEvent)
	END_DACOM_MAP()

	U32 eventHandle, hotEventHandle;		// connection handle
	bool bPanelOwned;
	S32 timeoutCount;

	COMPTR<IToolbar> menu;
	COMPTR<IShipSilButton> shipStatus[MAX_NUM_SEL_SHIPS];
	COMPTR<IStatic> shipclass;
	COMPTR<IHotButton> patrol,escort;
	COMPTR<IHotButton> stanceAttack,stanceDefend,stanceStand,stanceStop,supplyStanceAuto,supplyStanceNoAuto,supplyStanceResupplyOnly, cloak,attackPosition,specialWpnCmd,specialWpnCmd1,specialWpnCmd2;
	COMPTR<IHotButton> fighterStanceNormal,fighterStancePatrol;
	COMPTR<IMultiHotButton> specialWpnMulti,specialWpnMulti1,specialWpnMulti2;

	CMenu_MulMix (void);

	~CMenu_MulMix (void);

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
	
	/* CMenu_MulMix methods */

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
CMenu_MulMix::CMenu_MulMix (void)
{
		// nothing to init so far?
}
//--------------------------------------------------------------------------//
//
CMenu_MulMix::~CMenu_MulMix (void)
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
GENRESULT CMenu_MulMix::Notify (U32 message, void *param)
{
//	MSG *msg = (MSG *) param;

	switch (message)
	{
	case CQE_PANEL_OWNED:
		if (((CMenu_MulMix *)param) != this)
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
//---------------------------------------------------------------------------
//
MISSION_DATA::M_CAPS getCaps (IBaseObject * selected)
{
	MISSION_DATA::M_CAPS caps;
	MPart hSource;

	memset(&caps, 0, sizeof(caps));

	while (selected)
	{
		hSource = selected;
		if (hSource.isValid())
			caps |= hSource->caps;

		selected = selected->nextSelected;
	}

	return caps;
}
//--------------------------------------------------------------------------//
//
MISSION_DATA::M_CAPS getCapsExclusive (IBaseObject * selected)
{
	MISSION_DATA::M_CAPS caps;
	MPart hSource;

	if (selected==0)
		memset(&caps, 0, sizeof(caps));
	else
	{
		memset(&caps, 0xFF, sizeof(caps));

		while (selected)
		{
			hSource = selected;
			if (hSource.isValid())
				caps &= hSource->caps;

			selected = selected->nextSelected;
		}
	}

	return caps;
}
//--------------------------------------------------------------------------//
//
void CMenu_MulMix::onUpdate (U32 dt)
{
	if (bPanelOwned==false || S32(timeoutCount-dt) <= 0)
	{
		timeoutCount = TIMEOUT_PERIOD;

		//see if I have multiple objects selected object that are not all fabricators
		bool bKeepIt = false;

		IBaseObject * obj = OBJLIST->GetSelectedList();
		if (obj && obj->nextSelected!=0)
		{
			IBaseObject * searchObj = obj;
			while(searchObj)
			{
				bKeepIt = true;
				MPart part(searchObj);
				if(TESTADMIRAL(part->dwMissionID) || part->admiralID)
				{
					bKeepIt = false;
					break;
				}
				searchObj = searchObj->nextSelected;
			}
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

			obj = OBJLIST->GetSelectedList();
			bool bHasShip = false;
			while(obj)
			{
				if(obj->objClass == OC_SPACESHIP)
				{
					bHasShip = true;
					break;
				}
				obj = obj->nextSelected;
			}
			if(bHasShip)
			{
				if(patrol)
					patrol->SetVisible(true);
				if(escort)
					escort->SetVisible(true);
			}
			else
			{
				if(patrol)
					patrol->SetVisible(false);
				if(escort)
					escort->SetVisible(false);
			}
				
			obj = OBJLIST->GetSelectedList();
			bool bHasStance = false;
			bool bHasMultiStance = false;
			bool bAllCarrier = true;
			bool bHasCarrierStance = false;
			bool bHasMultiCarrierStance = false;
			FighterStance fStance = FS_NORMAL;
			UNIT_STANCE stance = US_ATTACK;
			while(obj)
			{
				OBJPTR<IAttack> attack;
				obj->QueryInterface(IAttackID,attack);
				MPart ship(obj);
				if((!MGlobals::IsCarrier(ship->mObjClass)) && ship->mObjClass != M_SPACESTATION && ship->mObjClass != M_PLASMAHIVE)
				{
					bAllCarrier = false;
				}
				if(attack && (ship->fleetID == 0))
				{
					if(MGlobals::IsCarrier(ship->mObjClass)|| ship->mObjClass == M_SPACESTATION || ship->mObjClass == M_PLASMAHIVE)
					{
						if(bHasCarrierStance)
						{
							if(fStance != attack->GetFighterStance())
							{
								bHasMultiCarrierStance = true;
							}
						}
						else
						{
							bHasCarrierStance = true;
							fStance = attack->GetFighterStance();
						}
					}
					if(obj->objClass == OC_SPACESHIP)
					{
						if(bHasStance)
						{
							if(stance != attack->GetUnitStance())
							{
								bHasMultiStance = true;
							}
						}
						else
						{
							bHasStance = true;
							stance = attack->GetUnitStance();
						}
					}
				}
				obj = obj->nextSelected;
			}
			if(bHasStance)
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

				stanceAttack->SetVisible(true);
				stanceStand->SetVisible(true);
				stanceStop->SetVisible(true);
				stanceDefend->SetVisible(true);
				stanceAttack->EnableButton(true);
				stanceDefend->EnableButton(true);
				stanceStop->EnableButton(true);
				stanceStand->EnableButton(true);
				if(bHasMultiStance)
				{
					stanceAttack->SetHighlightState(false);
					stanceDefend->SetHighlightState(false);
					stanceStop->SetHighlightState(false);
					stanceStand->SetHighlightState(false);
				}
				else
				{
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
				}
			}
			else
			{
				bHasStance = false;
				bHasMultiStance = false;
				enum SUPPLY_SHIP_STANCE supplyStance = SUP_STANCE_NONE;
				obj = OBJLIST->GetSelectedList();
				while(obj)
				{
					OBJPTR<ISupplier> supplier;
					obj->QueryInterface(ISupplierID,supplier);
					if(supplier)
					{
						if(bHasStance)
						{
							if(supplyStance != supplier->GetSupplyStance())
							{
								bHasMultiStance = true;
								break;
							}
						}
						else
						{
							bHasStance = true;
							supplyStance = supplier->GetSupplyStance();
						}
					}
					obj = obj->nextSelected;
				}
				if(bHasStance)
				{				
					supplyStanceAuto->SetVisible(true);
					supplyStanceNoAuto->SetVisible(true);
					supplyStanceResupplyOnly->SetVisible(true);
					supplyStanceAuto->EnableButton(true);
					supplyStanceNoAuto->EnableButton(true);
					supplyStanceResupplyOnly->EnableButton(true);
					if(bHasMultiStance)
					{
						supplyStanceAuto->SetHighlightState(false);
						supplyStanceNoAuto->SetHighlightState(false);
						supplyStanceResupplyOnly->SetHighlightState(false);
					}
					else
					{
						if(supplyStance == SUP_STANCE_NONE)
						{
							supplyStanceAuto->SetHighlightState(false);
							supplyStanceNoAuto->SetHighlightState(true);
							supplyStanceResupplyOnly->SetHighlightState(false);
						}
						else if(supplyStance ==SUP_STANCE_RESUPPLY)
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
			}
			if(bAllCarrier && bHasCarrierStance)
			{
				fighterStanceNormal->SetVisible(true);
				fighterStancePatrol->SetVisible(true);
				fighterStanceNormal->EnableButton(true);
				fighterStancePatrol->EnableButton(true);
				if(bHasMultiCarrierStance)
				{
					fighterStanceNormal->SetHighlightState(false);
					fighterStancePatrol->SetHighlightState(false);
				}
				else if(fStance == FS_NORMAL)
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

			MISSION_DATA::M_CAPS caps = getCaps(OBJLIST->GetSelectedList());
			cloak->SetVisible(caps.cloakOk);


			obj = OBJLIST->GetSelectedList();
			UNIT_SPECIAL_ABILITY spAbility = USA_NONE;
			UNIT_SPECIAL_ABILITY spAbility1 = USA_NONE;
			UNIT_SPECIAL_ABILITY spAbility2 = USA_NONE;
			while(obj)
			{
				MPart part(obj);
				if(part.isValid())
				{
					if(spAbility == USA_NONE)
					{
						spAbility = part.pInit->specialAbility;
						spAbility1 = part.pInit->specialAbility1;
						spAbility2 = part.pInit->specialAbility2;
					}
					else if(spAbility != part.pInit->specialAbility || spAbility1 != part.pInit->specialAbility1 || spAbility2 != part.pInit->specialAbility2)
					{
						spAbility = USA_NONE;
						spAbility1 = USA_NONE;
						spAbility2 = USA_NONE;
						break;
					}
				}
				else
				{
					spAbility = USA_NONE;
					spAbility1 = USA_NONE;
					spAbility2 = USA_NONE;
					break;
				}
				obj = obj->nextSelected;
			}
			const _GT_SPECIALABILITIES * specialAbilities = (const _GT_SPECIALABILITIES *) GENDATA->GetArchetypeData("SpecialAbilities");
			CQASSERT(specialAbilities);
			CQASSERT(sizeof(_GT_SPECIALABILITIES) == sizeof(GT_SPECIALABILITIES) && "Data definition error");
			const SPECIALABILITYINFO * specialInfo = specialAbilities->info + spAbility;
					
			caps = getCapsExclusive(OBJLIST->GetSelectedList());
			attackPosition->SetVisible(caps.targetPositionOk);

			if(spAbility != USA_NONE && (caps.specialEOAOk || caps.specialAttackOk || caps.specialAttackShipOk || caps.specialAbilityOk ||
				caps.probeOk || caps.mimicOk || caps.synthesisOk|| caps.specialTargetPlanetOk))
			{
				specialWpnCmd->SetVisible(true);
				specialWpnMulti->SetState(specialInfo->baseSpecialWpnButton,specialInfo->specialWpnTooltip,specialInfo->specialWpnHelpBox,specialInfo->specialWpnHintBox);
			}
			else
			{
				specialWpnCmd->SetVisible(false);
				specialWpnMulti->NullState();
			}

			specialInfo = specialAbilities->info + spAbility1;
			if(spAbility1 != USA_NONE && (caps.specialEOAOk || caps.specialAttackOk || caps.specialAttackShipOk || caps.specialAbilityOk ||
				caps.probeOk || caps.mimicOk || caps.synthesisOk|| caps.specialTargetPlanetOk))
			{
				specialWpnCmd1->SetVisible(true);
				specialWpnMulti1->SetState(specialInfo->baseSpecialWpnButton,specialInfo->specialWpnTooltip,specialInfo->specialWpnHelpBox,specialInfo->specialWpnHintBox);
			}
			else
			{
				specialWpnCmd1->SetVisible(false);
				specialWpnMulti1->NullState();
			}

			specialInfo = specialAbilities->info + spAbility2;
			if(spAbility2 != USA_NONE && (caps.specialEOAOk || caps.specialAttackOk || caps.specialAttackShipOk || caps.specialAbilityOk ||
				caps.probeOk || caps.mimicOk || caps.synthesisOk|| caps.specialTargetPlanetOk))
			{
				specialWpnCmd2->SetVisible(true);
				specialWpnMulti2->SetState(specialInfo->baseSpecialWpnButton,specialInfo->specialWpnTooltip,specialInfo->specialWpnHelpBox,specialInfo->specialWpnHintBox);
			}
			else
			{
				specialWpnCmd2->SetVisible(false);
				specialWpnMulti2->NullState();
			}
		}
	}
	else
		timeoutCount -= dt;
}
//--------------------------------------------------------------------------//
//
void CMenu_MulMix::enableMenu (bool bEnable)
{
	if((!bEnable) && bPanelOwned)
	{
		setPanelOwnership(false);
	}
}

void CMenu_MulMix::setPanelOwnership (bool bOwn)
{
	if(bOwn)
	{
		COMPTR<IToolbar> toolbar;

		if (TOOLBAR->QueryInterface("IToolbar", toolbar) == GR_OK)
		{
			IBaseObject * obj = OBJLIST->GetSelectedList();
			MPart part(obj);
			if (toolbar->GetToolbar("group", menu, part->race) == GR_OK)
			{
				COMPTR<IDAComponent> pComp;

				for(U32 index = 0; index < MAX_NUM_SEL_SHIPS; ++index)
				{
					char buffer[256];
					sprintf(buffer,"ship%d",index);
					if (menu->GetControl(buffer, pComp) == GR_OK)
						pComp->QueryInterface("IShipSilButton", shipStatus[index]);
					
				}
				if (toolbar->GetControl("shipclass", pComp) == GR_OK)
					pComp->QueryInterface("IStatic", shipclass);
				if(shipclass)
				{
/*					wchar_t * buffer[256];
					const char * str = _localLoadString(IDS_TOOLBAR_GROUP);
					_localAnsiToWide(str,(wchar_t *)buffer,sizeof(buffer));
					shipclass->SetText((wchar_t *)buffer);
*/
					shipclass->SetText(_localLoadStringW(IDS_TOOLBAR_GROUP));
				}

				if (menu->GetControl("patrol", pComp) == GR_OK)
					pComp->QueryInterface("IHotButton", patrol);
				if (menu->GetControl("escort", pComp) == GR_OK)
					pComp->QueryInterface("IHotButton", escort);

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
				if (menu->GetControl("cloak", pComp) == GR_OK)
					pComp->QueryInterface("IHotButton", cloak);			
				if (menu->GetControl("attackPosition", pComp) == GR_OK)
					pComp->QueryInterface("IHotButton", attackPosition);			
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

		for(U32 index = 0; index < MAX_NUM_SEL_SHIPS; ++index)
		{
			shipStatus[index].free();
		}
		menu.free();
		shipclass.free();
		patrol.free();
		escort.free();
		stanceAttack.free();		
		stanceDefend.free();		
		stanceStand.free();		
		stanceStop.free();	
		fighterStanceNormal.free();
		fighterStancePatrol.free();
		supplyStanceAuto.free();		
		supplyStanceNoAuto.free();		
		supplyStanceResupplyOnly.free();
		cloak.free();
		attackPosition.free();
		specialWpnMulti.free();
		specialWpnCmd.free();
		specialWpnMulti1.free();
		specialWpnCmd1.free();
		specialWpnMulti2.free();
		specialWpnCmd2.free();
	}
}

//--------------------------------------------------------------------------//
//
struct _cmenu_mulmix : GlobalComponent
{
	CMenu_MulMix * menu;

	virtual void Startup (void)
	{
		menu = new DAComponent<CMenu_MulMix>;
		AddToGlobalCleanupList(&menu);
	}

	virtual void Initialize (void)
	{
		COMPTR<IDAConnectionPoint> connection;

		if (TOOLBAR->QueryOutgoingInterface("IEventCallback", connection) == GR_OK)
			connection->Advise(menu->getBase(), &menu->eventHandle);
	}
};

static _cmenu_mulmix startup;

//--------------------------------------------------------------------------//
//-----------------------------End CMenu_Ind.cpp----------------------------//
//--------------------------------------------------------------------------//
