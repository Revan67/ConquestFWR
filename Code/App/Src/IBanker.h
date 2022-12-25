#ifndef IBANKER_H
#define IBANKER_H
//--------------------------------------------------------------------------//
//                                                                          //
//                              IBanker.h                                    //
//                                                                          //
//                  COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/IBanker.h 9     9/06/00 7:21p Sbarton $
*/			    
//------------------------------- #INCLUDES --------------------------------//
//--------------------------------------------------------------------------//
#ifndef IOBJECT_H
#include "IObject.h"
#endif

struct ResourceCost;
enum M_RESOURCE_TYPE;

struct _NO_VTABLE ISpender
{
	virtual struct ResourceCost GetAmountNeeded() = 0;
	
	virtual void UnstallSpender() = 0;
};

//--------------------------------------------------------------------------//
//
struct _NO_VTABLE IBanker : public IDAComponent
{
	virtual U32 GetGasResources (U32 playerID) = 0;

	virtual U32 GetMetalResources (U32 playerID) = 0;

	virtual U32 GetCrewResources (U32 playerID) = 0;

	virtual U32 GetFreeCommandPt (U32 playerID) = 0;

	virtual U32 GetTotalCommandPt (U32 playerID) = 0;
	
	virtual bool SpendMoney (U32 playerID, const ResourceCost & amount,M_RESOURCE_TYPE * failType= NULL) = 0;

	virtual bool SpendCommandPoints (U32 playerID, const ResourceCost & amount)= 0;

	virtual bool HasCost (U32 playerID, const ResourceCost & cost,M_RESOURCE_TYPE * failType = NULL) = 0;

	virtual bool HasResources (U32 playerID, const ResourceCost & cost,M_RESOURCE_TYPE * failType = NULL) = 0;

	virtual bool HasCommandPoints (U32 playerID, const ResourceCost & cost,M_RESOURCE_TYPE * failType = NULL) = 0;

	virtual void AddResource (U32 playerID, const ResourceCost & amount) = 0;

	virtual void AddGas (U32 playerID, U32 amount) = 0;

	virtual void AddMetal (U32 playerID, U32 amount) = 0;

	virtual void AddCrew (U32 playerID, U32 amount) = 0;

	virtual void AddCommandPt (U32 playerID, U32 amount) = 0;

	virtual void UseCommandPt (U32 playerID, U32 amount) = 0;

	virtual void FreeCommandPt (U32 playerID, U32 amount) = 0;

	virtual void LoseCommandPt (U32 playerID, U32 amount) = 0;

	virtual void AddStalledSpender (U32 playerID, IBaseObject * spender) = 0;

	virtual void RemoveStalledSpender (U32 playerID, IBaseObject * spender) = 0;

	virtual void AddMaxResources (S32 metal, S32 gas, S32 crew, U32 playerID) = 0;

	virtual void RemoveMaxResources (S32 metal, S32 gas, S32 crew, U32 playerID) = 0;

	virtual void TransferMaxResources (S32 metal, S32 gas, S32 crew, U32 playerID, U32 otherPlayerID) = 0;

	virtual void LoseResources (S32 metal, S32 gas, S32 crew, U32 player) = 0;
};
//---------------------------------------------------------------------------
//-------------------------END IBanker.h--------------------------------------
//---------------------------------------------------------------------------
#endif
