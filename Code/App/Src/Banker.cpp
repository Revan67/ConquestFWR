//--------------------------------------------------------------------------//
//                                                                          //
//                              Banker.cpp                                  //
//                                                                          //
//               COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
   $Header: /Conquest/App/Src/Banker.cpp 46    7/19/01 3:19p Tmauer $
*/
//--------------------------------------------------------------------------//

 
#include "pch.h"
#include <globals.h>

#include "startup.h"
#include "IBanker.h"
#include "EventSys.h"
#include "IStatic.h"
#include "IProgressStatic.h"
#include "MGlobals.h"
#include "BaseHotRect.h"
#include "IToolbar.h"
#include "OpAgent.h"
#include "SoundManager.h"
#include "ObjWatch.h"
#include "ObjList.h"
#include <DBaseData.h>


#include "resource.h"

#include <TSmartPointer.h>

#include <stdio.h>

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//

struct stalledNode
{
	OBJPTR<ISpender> spender;
	stalledNode * next;

	void * operator new (size_t size)
	{
		void * result = calloc(size, 1);
		{
			DWORD dwAddr;
			__asm
			{
				mov eax, DWORD PTR [EBP+4]
				mov DWORD PTR dwAddr, eax
			}
			HEAP->SetBlockOwner(result, dwAddr);
		}
		return result;
	}

	void operator delete (void * ptr)
	{
		free(ptr);
	}
};

struct DACOM_NO_VTABLE Banker : public IBanker, IEventCallback, IHotControlEvent
{
	BEGIN_DACOM_MAP_INBOUND(Banker)
	DACOM_INTERFACE_ENTRY(IBanker)
	DACOM_INTERFACE_ENTRY(IEventCallback)
	DACOM_INTERFACE_ENTRY(IHotControlEvent)
	END_DACOM_MAP()

    void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}

	U32 displayGasValue;//the currentValue to display
	U32 displayMetalValue;
	U32 displayCrewValue;
	S32 displayTotalComPtValue;
	S32 displayUsedComPtValue;

	U32 displayMaxGas;
	U32 displayMaxMetal;
	U32 displayMaxCrew;

	U32 eventHandle;		// connection handle

	stalledNode * stallLists[9];
	stalledNode * stallListEnd[9];

	COMPTR<IStatic> gasArea,metalArea,crewArea;
	COMPTR<IStatic> commandPtArea;

	S32 trueDisplayTimer;

	virtual ~Banker();

	//
	// IBanker methods
	//

	virtual U32 GetGasResources(U32 playerID);

	virtual U32 GetMetalResources(U32 playerID);

	virtual U32 GetCrewResources(U32 playerID);

	virtual U32 GetFreeCommandPt(U32 playerID);

	virtual U32 GetTotalCommandPt(U32 playerID);
	
	virtual bool SpendMoney(U32 playerID, const ResourceCost & amount,M_RESOURCE_TYPE * failType=NULL);

	virtual bool SpendCommandPoints(U32 playerID, const ResourceCost & amount);

	virtual bool HasCost(U32 playerID, const ResourceCost & amount,M_RESOURCE_TYPE * failType = NULL);

	virtual bool HasResources(U32 playerID, const ResourceCost & cost,M_RESOURCE_TYPE * failType = NULL);

	virtual bool HasCommandPoints(U32 playerID, const ResourceCost & cost,M_RESOURCE_TYPE * failType = NULL);

	virtual void AddResource(U32 playerID, const ResourceCost & amount);

	virtual void AddGas(U32 playerID, U32 amount);

	virtual void AddMetal(U32 playerID, U32 amount);

	virtual void AddCrew(U32 playerID, U32 amount);

	virtual void AddCommandPt(U32 playerID, U32 amount);

	virtual void UseCommandPt(U32 playerID, U32 amount);

	virtual void FreeCommandPt(U32 playerID, U32 amount);

	virtual void LoseCommandPt(U32 playerID, U32 amount);

	virtual void AddStalledSpender(U32 playerID, IBaseObject * spender);

	virtual void RemoveStalledSpender(U32 playerID, IBaseObject * spender);

	virtual void AddMaxResources(S32 metal, S32 gas, S32 crew, U32 playerID);

	virtual void RemoveMaxResources(S32 metal, S32 gas, S32 crew, U32 playerID);

	virtual void TransferMaxResources(S32 metal, S32 gas, S32 crew, U32 playerID, U32 otherPlayerID);

	virtual void LoseResources(S32 metal, S32 gas, S32 crew, U32 player);

	//
	// IEventCallback methods 
	//

	DEFMETHOD(Notify) (U32 message, void *param = 0);

	//
	//IHotControlEvent
	//
	virtual void OnLeftButtonEvent (U32 menuID, U32 controlID)
	{
	}
	virtual void OnRightButtonEvent (U32 menuID, U32 controlID)
	{
	}
	virtual void OnLeftDblButtonEvent (U32 menuID, U32 controlID)
	{
	}
	// Banker methods

	void onUpdate (void);

	void onClose (void);

	void enableMenu (bool bEnable);

	void init (void);

	void reset (void);

	IDAComponent * getBase (void)
	{
		return static_cast<IEventCallback *>(this);
	}
};
//--------------------------------------------------------------------------//
//
Banker::~Banker()
{
	reset();

	if (TOOLBAR)
	{
		COMPTR<IDAConnectionPoint> connection;
		
		if (TOOLBAR->QueryOutgoingInterface("IEventCallback", connection) == GR_OK)
			connection->Unadvise(eventHandle);
	}
}
//--------------------------------------------------------------------------//
//
U32 Banker::GetGasResources(U32 playerID)
{
	return MGlobals::GetCurrentGas(playerID);
}
//--------------------------------------------------------------------------//
//
U32 Banker::GetMetalResources(U32 playerID)
{
	return MGlobals::GetCurrentMetal(playerID);
}
//--------------------------------------------------------------------------//
//
U32 Banker::GetCrewResources(U32 playerID)
{
	return MGlobals::GetCurrentCrew(playerID);
}
//--------------------------------------------------------------------------//
//
U32 Banker::GetFreeCommandPt(U32 playerID)
{
	return MGlobals::GetCurrentTotalComPts(playerID)-MGlobals::GetCurrentUsedComPts(playerID);
}
//--------------------------------------------------------------------------//
//
U32 Banker::GetTotalCommandPt(U32 playerID)
{
	return MGlobals::GetCurrentTotalComPts(playerID);
}	
//--------------------------------------------------------------------------//
//
bool Banker::SpendMoney(U32 playerID, const ResourceCost & amount,M_RESOURCE_TYPE * failType)
{
	// if the cheat for free building is on then return true
	if (CQFLAGS.bEverythingFree)
	{
		return true;
	}

	if(HasResources(playerID,amount,failType))
	{
		if(MGlobals::AdvancedAI(playerID))
		{
			ResourceCost realCost;
			realCost.commandPt = amount.commandPt;
			realCost.crew = (amount.crew*(1.0-MGlobals::GetAIBonus(playerID))*(1.0-MGlobals::GetAIBonus(playerID)));
			realCost.gas = (amount.gas*(1.0-MGlobals::GetAIBonus(playerID)));
			realCost.metal = (amount.metal*(1.0-MGlobals::GetAIBonus(playerID)));
			if(HasResources(playerID,realCost,failType))
			{
				MGlobals::SetCurrentCrew(playerID,MGlobals::GetCurrentCrew(playerID)-realCost.crew);
				MGlobals::SetCurrentGas(playerID,MGlobals::GetCurrentGas(playerID)-realCost.gas);
				MGlobals::SetCurrentMetal(playerID,MGlobals::GetCurrentMetal(playerID)-realCost.metal);
			}
			else
			{
				MGlobals::SetCurrentCrew(playerID,MGlobals::GetCurrentCrew(playerID)-amount.crew);
				MGlobals::SetCurrentGas(playerID,MGlobals::GetCurrentGas(playerID)-amount.gas);
				MGlobals::SetCurrentMetal(playerID,MGlobals::GetCurrentMetal(playerID)-amount.metal);
			}
		}
		else
		{
			MGlobals::SetCurrentCrew(playerID,MGlobals::GetCurrentCrew(playerID)-amount.crew);
			MGlobals::SetCurrentGas(playerID,MGlobals::GetCurrentGas(playerID)-amount.gas);
			MGlobals::SetCurrentMetal(playerID,MGlobals::GetCurrentMetal(playerID)-amount.metal);
		}
		return true;
	}
	return false;
}
//--------------------------------------------------------------------------//
//
bool Banker::SpendCommandPoints (U32 playerID, const ResourceCost & amount)
{
	if(HasCommandPoints(playerID,amount))
	{
		MGlobals::SetCurrentUsedComPts(playerID,MGlobals::GetCurrentUsedComPts(playerID)+amount.commandPt);
		return true;
	}
	return false;
}
//--------------------------------------------------------------------------//
//
bool Banker::HasCost (U32 playerID, const ResourceCost & amount,M_RESOURCE_TYPE * failType)
{
	return HasResources(playerID,amount,failType) && HasCommandPoints(playerID,amount,failType);
}
//--------------------------------------------------------------------------//
//
bool Banker::HasResources (U32 playerID, const ResourceCost & cost,M_RESOURCE_TYPE * failType)
{
	// if the cheat for free building is on then return true
	if (CQFLAGS.bEverythingFree)
	{
		return true;
	}

	U32 metal,gas,crew;
	metal = MGlobals::GetCurrentMetal(playerID);
	if(metal < cost.metal)
	{
		if(failType)
			(*failType) = M_METAL;
		return false;
	}
	gas = MGlobals::GetCurrentGas(playerID);
	if(gas < cost.gas)
	{
		if(failType)
			(*failType) = M_GAS;
		return false;
	}
	crew = MGlobals::GetCurrentCrew(playerID);
	if(crew < cost.crew)
	{
		if(failType)
			(*failType) = M_CREW;
		return false;
	}
	return true;
}
//--------------------------------------------------------------------------//
//
bool Banker::HasCommandPoints(U32 playerID, const ResourceCost & cost,M_RESOURCE_TYPE * failType)
{
	S32 comPts = __min(MGlobals::GetCurrentTotalComPts(playerID),MGlobals::GetMaxControlPoints(playerID));
	S32 usedComPts = MGlobals::GetCurrentUsedComPts(playerID);
	if(cost.commandPt && comPts <= usedComPts)
	{
		if(failType)
			(*failType) = M_COMMANDPTS;
		return false;
	}

	return true;
}
//--------------------------------------------------------------------------//
//
void Banker::AddResource(U32 playerID, const ResourceCost & amount)
{
	CQASSERT(THEMATRIX->IsMaster());
	U32 resAmount = MGlobals::GetCurrentCrew(playerID)+amount.crew;
	if(resAmount < MGlobals::GetMaxCrew(playerID))
		MGlobals::SetCurrentCrew(playerID,resAmount);
	else
		MGlobals::SetCurrentCrew(playerID,MGlobals::GetMaxCrew(playerID));

	resAmount = MGlobals::GetCurrentGas(playerID)+amount.gas;
	if(resAmount < MGlobals::GetMaxGas(playerID))
		MGlobals::SetCurrentGas(playerID,resAmount);
	else
		MGlobals::SetCurrentGas(playerID,MGlobals::GetMaxGas(playerID));

	resAmount = MGlobals::GetCurrentMetal(playerID)+amount.metal;
	if(resAmount < MGlobals::GetMaxMetal(playerID))
		MGlobals::SetCurrentMetal(playerID,resAmount);
	else
		MGlobals::SetCurrentMetal(playerID,MGlobals::GetMaxMetal(playerID));
}
//--------------------------------------------------------------------------//
//
void Banker::AddGas (U32 playerID, U32 amount)
{
	CQASSERT(THEMATRIX->IsMaster());
	amount = MGlobals::GetCurrentGas(playerID)+amount;
	if(amount < MGlobals::GetMaxGas(playerID))
		MGlobals::SetCurrentGas(playerID,amount);
	else
		MGlobals::SetCurrentGas(playerID,MGlobals::GetMaxGas(playerID));
}
//--------------------------------------------------------------------------//
//
void Banker::AddMetal (U32 playerID, U32 amount)
{
	CQASSERT(THEMATRIX->IsMaster());
	amount = MGlobals::GetCurrentMetal(playerID)+amount;
	if(amount < MGlobals::GetMaxMetal(playerID))
		MGlobals::SetCurrentMetal(playerID,amount);
	else
		MGlobals::SetCurrentMetal(playerID,MGlobals::GetMaxMetal(playerID));
}
//--------------------------------------------------------------------------//
//
void Banker::AddCrew (U32 playerID, U32 amount)
{
	CQASSERT(THEMATRIX->IsMaster());
	amount = MGlobals::GetCurrentCrew(playerID)+amount;
	if(amount < MGlobals::GetMaxCrew(playerID))
		MGlobals::SetCurrentCrew(playerID,amount);
	else
		MGlobals::SetCurrentCrew(playerID,MGlobals::GetMaxCrew(playerID));
}
//--------------------------------------------------------------------------//
//
void Banker::AddCommandPt (U32 playerID, U32 amount)
{
	MGlobals::SetCurrentTotalComPts(playerID,MGlobals::GetCurrentTotalComPts(playerID)+amount);
}
//--------------------------------------------------------------------------//
//
void Banker::UseCommandPt (U32 playerID, U32 amount)
{
	MGlobals::SetCurrentUsedComPts(playerID,MGlobals::GetCurrentUsedComPts(playerID)+amount);
}
//--------------------------------------------------------------------------//
//
void Banker::FreeCommandPt (U32 playerID, U32 amount)
{
	MGlobals::SetCurrentUsedComPts(playerID,MGlobals::GetCurrentUsedComPts(playerID)-amount);
}
//--------------------------------------------------------------------------//
//
void Banker::LoseCommandPt (U32 playerID, U32 amount)
{
	MGlobals::SetCurrentTotalComPts(playerID,MGlobals::GetCurrentTotalComPts(playerID)-amount);
}
//--------------------------------------------------------------------------//
//
void Banker::AddStalledSpender (U32 playerID, IBaseObject * spender)
{
	stalledNode * node = new stalledNode;
	spender->QueryInterface(ISpenderID,node->spender, NONSYSVOLATILEPTR);
	CQASSERT(node->spender != 0);
	node->next = NULL;
	if(stallListEnd[playerID])
	{
		stallListEnd[playerID]->next = node;
	}
	else
	{
		stallLists[playerID] = node;
	}
	stallListEnd[playerID] = node;
}
//--------------------------------------------------------------------------//
//
void Banker::RemoveStalledSpender (U32 playerID, IBaseObject * spender)
{
	stalledNode * node = stallLists[playerID];
	stalledNode * prev = NULL;
	OBJPTR<ISpender> searchSpend;
	spender->QueryInterface(ISpenderID,searchSpend,NONSYSVOLATILEPTR);
	while(node)
	{
		if(node->spender == searchSpend)
		{
			if(prev)
				prev->next = node->next;
			else
				stallLists[playerID] = node->next;
			if(stallListEnd[playerID] == node)
				stallListEnd[playerID] = prev;
			delete node;
			return;
		}
		prev = node;
		node = node->next;
	}
}
//--------------------------------------------------------------------------//
//
void Banker::AddMaxResources (S32 metal, S32 gas, S32 crew, U32 playerID)
{
	MGlobals::SetMaxMetal(playerID,metal+MGlobals::GetMaxMetal(playerID));
	MGlobals::SetMaxGas(playerID,gas+MGlobals::GetMaxGas(playerID));
	MGlobals::SetMaxCrew(playerID,crew+MGlobals::GetMaxCrew(playerID));
}
//--------------------------------------------------------------------------//
//
void Banker::RemoveMaxResources (S32 metal, S32 gas, S32 crew, U32 playerID)
{
	MGlobals::SetMaxMetal(playerID,MGlobals::GetMaxMetal(playerID)-metal);
	MGlobals::SetMaxGas(playerID,MGlobals::GetMaxGas(playerID)-gas);
	MGlobals::SetMaxCrew(playerID,MGlobals::GetMaxCrew(playerID)-crew);
	if(THEMATRIX->IsMaster())
	{
		if(MGlobals::GetCurrentMetal(playerID) > MGlobals::GetMaxMetal(playerID))
			MGlobals::SetCurrentMetal(playerID,MGlobals::GetMaxMetal(playerID));
		if(MGlobals::GetCurrentGas(playerID) > MGlobals::GetMaxGas(playerID))
			MGlobals::SetCurrentGas(playerID,MGlobals::GetMaxGas(playerID));
		if(MGlobals::GetCurrentCrew(playerID) > MGlobals::GetMaxCrew(playerID))
			MGlobals::SetCurrentCrew(playerID,MGlobals::GetMaxCrew(playerID));
	}
}
//--------------------------------------------------------------------------//
//
void Banker::TransferMaxResources (S32 metal, S32 gas, S32 crew, U32 playerID, U32 otherPlayerID)
{
	AddMaxResources(metal,gas,crew,otherPlayerID);
	if(THEMATRIX->IsMaster())
	{
		U32 max = MGlobals::GetMaxMetal(playerID)-BASE_MAX_METAL;
		if(max)
		{
			U32 current = MGlobals::GetCurrentMetal(playerID);
			if(current > BASE_MAX_METAL)
				current -= BASE_MAX_METAL;
			else
				current = 0;
			U32 metalShare = (metal*current)/max;
			MGlobals::SetCurrentMetal(playerID,MGlobals::GetCurrentMetal(playerID)-metalShare);
			MGlobals::SetCurrentMetal(otherPlayerID,MGlobals::GetCurrentMetal(otherPlayerID)+metalShare);
		}
		
		max = MGlobals::GetMaxGas(playerID)-BASE_MAX_GAS;
		if(max)
		{
			U32 current = MGlobals::GetCurrentGas(playerID);
			if(current > BASE_MAX_GAS)
				current -= BASE_MAX_GAS;
			else
				current = 0;
			U32 gasShare = (gas*current)/max;
			MGlobals::SetCurrentGas(playerID,MGlobals::GetCurrentGas(playerID)-gasShare);
			MGlobals::SetCurrentGas(otherPlayerID,MGlobals::GetCurrentGas(otherPlayerID)+gasShare);
		}

		max = MGlobals::GetMaxCrew(playerID)-BASE_MAX_CREW;
		if(max)
		{
			U32 current = MGlobals::GetCurrentCrew(playerID);
			if(current > BASE_MAX_CREW)
				current -= BASE_MAX_CREW;
			else
				current = 0;
			U32 crewShare = (crew*current)/max;
			MGlobals::SetCurrentCrew(playerID,MGlobals::GetCurrentCrew(playerID)-crewShare);
			MGlobals::SetCurrentCrew(otherPlayerID,MGlobals::GetCurrentCrew(otherPlayerID)+crewShare);
		}
	}
	RemoveMaxResources(metal,gas,crew,playerID);
}
//--------------------------------------------------------------------------//
//
void Banker::LoseResources (S32 metal, S32 gas, S32 crew, U32 playerID)
{
	MGlobals::SetCurrentMetal(playerID,MGlobals::GetCurrentMetal(playerID)-metal);
	MGlobals::SetCurrentGas(playerID,MGlobals::GetCurrentGas(playerID)-gas);
	MGlobals::SetCurrentCrew(playerID,MGlobals::GetCurrentCrew(playerID)-crew);
}

//--------------------------------------------------------------------------//
// receive notifications from event system
//
GENRESULT Banker::Notify (U32 message, void *param)
{
//	MSG *msg = (MSG *) param;

	switch (message)
	{
	case CQE_UPDATE:
		onUpdate();
		break;
	case CQE_LOAD_INTERFACE:
		enableMenu(param!=0);
		break;
	case CQE_MISSION_CLOSE:
		onClose();
		break;
	}

	return GR_OK;
}
//--------------------------------------------------------------------------//
//
S32 getDiff(U32 trueVal,U32 displayVal)
{
	S32 diff = trueVal - displayVal;
	S32 newDiff;

	if (diff < 0)
	{
		newDiff = floor(diff*4 / REALTIME_FRAMERATE);
		if (newDiff < diff)
			newDiff = diff;
	}
	else
	{
		newDiff = ceil(diff*4 / REALTIME_FRAMERATE);
		if (newDiff > diff)
			newDiff = diff;
	}
	return newDiff;
}
//--------------------------------------------------------------------------//
//
void Banker::onUpdate()
{
	if (--trueDisplayTimer <= 0)
	{
		trueDisplayTimer = 4;

		U32 trueGas = MGlobals::GetCurrentGas(MGlobals::GetThisPlayer()) *GAS_MULTIPLIER;

		if (trueGas != displayGasValue || displayMaxGas != MGlobals::GetMaxGas(MGlobals::GetThisPlayer()))
		{
			displayGasValue += getDiff(trueGas,displayGasValue);
			displayMaxGas = MGlobals::GetMaxGas(MGlobals::GetThisPlayer());

			if(gasArea)
			{
				wchar_t buffer[32];
				swprintf(buffer, L"%d/%d", displayGasValue,displayMaxGas*GAS_MULTIPLIER);
				gasArea->SetText(buffer);
//				gasArea->SetProgress(displayGasValue,MGlobals::GetMaxGas(MGlobals::GetThisPlayer())*GAS_MULTIPLIER);
			}
			
		}
		U32 trueMetal = MGlobals::GetCurrentMetal(MGlobals::GetThisPlayer())*METAL_MULTIPLIER;
		if (trueMetal != displayMetalValue || displayMaxMetal != MGlobals::GetMaxMetal(MGlobals::GetThisPlayer()))
		{
			displayMetalValue += getDiff(trueMetal,displayMetalValue);
			displayMaxMetal = MGlobals::GetMaxMetal(MGlobals::GetThisPlayer());

			if(metalArea)
			{
				wchar_t buffer[32];
				swprintf(buffer, L"%d/%d", displayMetalValue,displayMaxMetal*METAL_MULTIPLIER);
				metalArea->SetText(buffer);
//				metalArea->SetProgress(displayMetalValue,MGlobals::GetMaxMetal(MGlobals::GetThisPlayer())*METAL_MULTIPLIER);
			}			
		}
		U32 trueCrew = MGlobals::GetCurrentCrew(MGlobals::GetThisPlayer())*CREW_MULTIPLIER;
		if (trueCrew != displayCrewValue || displayMaxCrew != MGlobals::GetMaxCrew(MGlobals::GetThisPlayer()))
		{
			displayCrewValue += getDiff(trueCrew,displayCrewValue);
			displayMaxCrew = MGlobals::GetMaxCrew(MGlobals::GetThisPlayer());

			if(crewArea)
			{
				wchar_t buffer[32];
				swprintf(buffer, L"%d/%d", displayCrewValue,displayMaxCrew*CREW_MULTIPLIER);
				crewArea->SetText(buffer);
//				crewArea->SetProgress(displayCrewValue,MGlobals::GetMaxCrew(MGlobals::GetThisPlayer())*CREW_MULTIPLIER);
			}			
		}
		S32 trueTotalCom = MGlobals::GetCurrentTotalComPts(MGlobals::GetThisPlayer());
		if(trueTotalCom > (S32)MGlobals::GetMaxControlPoints(MGlobals::GetThisPlayer()))
			trueTotalCom = MGlobals::GetMaxControlPoints(MGlobals::GetThisPlayer());
		S32 trueUsedCom = MGlobals::GetCurrentUsedComPts(MGlobals::GetThisPlayer());
		if (trueTotalCom != displayTotalComPtValue || trueUsedCom != displayUsedComPtValue)
		{
			displayTotalComPtValue += getDiff(trueTotalCom,displayTotalComPtValue);
			displayUsedComPtValue += getDiff(trueUsedCom,displayUsedComPtValue);

			if(commandPtArea)
			{
				wchar_t buffer[32];
				swprintf(buffer, L"%d/%d",displayUsedComPtValue, displayTotalComPtValue);
				commandPtArea->SetText(buffer);
			}
		}
		if(displayTotalComPtValue <= displayUsedComPtValue)
		{
			if(commandPtArea)
			{
				if((GetTickCount()/1000)%2)
					commandPtArea->SetTextColor(255,0,0);
				else
					commandPtArea->SetTextColor(255,255,255);
			}
		}
		else
		{
			if(commandPtArea)
				commandPtArea->SetTextColor(0,255,255);
		}
	}

	//see if I can unstall any stalled processes
	if(THEMATRIX->IsMaster())
	{
		for(U32 index = 1; index <= MAX_PLAYERS; ++index)
		{
			stalledNode * node = stallLists[index];
			stalledNode * prev = NULL;
			while(node)
			{
				if(node->spender)
				{
					ResourceCost cost = node->spender->GetAmountNeeded();
					if(HasCommandPoints(index,cost))
					{
						if(prev)
							prev->next = node->next;
						else
							stallLists[index] = node->next;
						if(!(node->next))
							stallListEnd[index] = prev;
						node->spender->UnstallSpender();
						delete node;
						break;
					}
					prev = node;
					node = node->next;
				}
				else
				{
					//the speder was deleted from the system
					if(prev)
						prev->next = node->next;
					else
						stallLists[index] = node->next;
					if(stallListEnd[index] == node)
						stallListEnd[index] = prev;
					stalledNode * nextNode = node->next;
					delete node;
					node = nextNode;
				}
			}
		}
	}
}
//--------------------------------------------------------------------------//
//
void Banker::enableMenu (bool bEnable)
{
	if(bEnable)
	{
		COMPTR<IDAComponent> pComp;
		COMPTR<IToolbar> toolbar;

		if (TOOLBAR->QueryInterface("IToolbar", toolbar) == GR_OK)
		{
			if (toolbar->GetControl("gas", pComp) == GR_OK)
				pComp->QueryInterface("IStatic", gasArea);
			if (toolbar->GetControl("metal", pComp) == GR_OK)
				pComp->QueryInterface("IStatic", metalArea);
			if (toolbar->GetControl("crew", pComp) == GR_OK)
				pComp->QueryInterface("IStatic", crewArea);
			if (toolbar->GetControl("commandPts", pComp) == GR_OK)
				pComp->QueryInterface("IStatic", commandPtArea);
			if(gasArea)
			{
				wchar_t buffer[32];
				swprintf(buffer, L"%d/%d", displayGasValue,displayMaxGas*GAS_MULTIPLIER);
				gasArea->SetText(buffer);
//				gasArea->SetProgress(displayGasValue,MGlobals::GetMaxGas(MGlobals::GetThisPlayer())*GAS_MULTIPLIER);
			}
			if(metalArea)
			{
				wchar_t buffer[32];
				swprintf(buffer, L"%d/%d", displayMetalValue,displayMaxMetal*METAL_MULTIPLIER);
				metalArea->SetText(buffer);
//				metalArea->SetProgress(displayMetalValue,MGlobals::GetMaxMetal(MGlobals::GetThisPlayer())*METAL_MULTIPLIER);
			}
			if(crewArea)
			{
				wchar_t buffer[32];
				swprintf(buffer, L"%d/%d", displayCrewValue,displayMaxCrew*CREW_MULTIPLIER);
				crewArea->SetText(buffer);
//				crewArea->SetProgress(displayCrewValue,MGlobals::GetMaxCrew(MGlobals::GetThisPlayer())*CREW_MULTIPLIER);
			}
			if(commandPtArea)
			{
				wchar_t buffer[32];
				swprintf(buffer, L"%d/%d",displayUsedComPtValue, displayTotalComPtValue);
				commandPtArea->SetText(buffer);
			}
		}
	}
	else
	{
		gasArea.free();
		metalArea.free();
		crewArea.free();
		commandPtArea.free();
	}
	
}
//--------------------------------------------------------------------------//
//
void Banker::init()
{
	COMPTR<IDAConnectionPoint> connection;
	if (TOOLBAR->QueryOutgoingInterface("IEventCallback", connection) == GR_OK)
		connection->Advise(getBase(), &eventHandle);
	for(U32 playerID = 1; playerID <= MAX_PLAYERS; ++playerID)
	{
		stallLists[playerID] = NULL;
		stallListEnd[playerID] = NULL;
	}
	trueDisplayTimer = 0;
	displayGasValue = 0;
	displayMetalValue = 0;
	displayCrewValue = 0;
	displayUsedComPtValue = 0;
	displayTotalComPtValue = 0;
	displayMaxGas = 0;
	displayMaxMetal = 0;
	displayMaxCrew = 0;
}
//--------------------------------------------------------------------------//
//
void Banker::reset (void)
{
	for(U32 playerID = 1; playerID <= MAX_PLAYERS;++playerID)
	{
		while(stallLists[playerID])
		{
			stalledNode * next = stallLists[playerID]->next;
			delete stallLists[playerID];
			stallLists[playerID] = next;
		}
		stallListEnd[playerID] = NULL;
	}

	trueDisplayTimer = 0;
	displayGasValue = 0;
	displayMetalValue = 0;
	displayCrewValue = 0;
	displayUsedComPtValue = 0;
	displayTotalComPtValue = 0;
	displayMaxGas = 0;
	displayMaxMetal = 0;
	displayMaxCrew = 0;
}
//--------------------------------------------------------------------------//
//
void Banker::onClose (void)
{
	reset();
}
//--------------------------------------------------------------------------//
//
struct _banker : GlobalComponent
{
	Banker * banker;

	virtual void Startup (void)
	{
		BANKER = banker = new DAComponent<Banker>;
		AddToGlobalCleanupList(&banker);
	}

	virtual void Initialize (void)
	{
		COMPTR<IDAConnectionPoint> connection;
		
		banker->init();
	}
};

static _banker globalBanker;

//--------------------------------------------------------------------------//
//-----------------------------End Banker.cpp-------------------------------//
//--------------------------------------------------------------------------//
