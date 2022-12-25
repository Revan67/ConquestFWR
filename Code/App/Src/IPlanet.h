#ifndef IPLANET_H
#define IPLANET_H
//--------------------------------------------------------------------------//
//                                                                          //
//                                IPlanet.h                                 //
//                                                                          //
//                  COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/IPlanet.h 21    7/17/00 7:49p Tmauer $
*/			    
//------------------------------- #INCLUDES --------------------------------//
//--------------------------------------------------------------------------//
#ifndef IOBJECT_H
#include "IObject.h"
#endif

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
struct _NO_VTABLE IPlanet : IObject 
{
	virtual U32 GetHighlightedSlot() = 0;

	virtual TRANSFORM GetSlotTransform (U32 slotNumber) = 0;

	virtual void ClickSlot(U32 slotNumber) = 0;

	virtual U8 GetMaxSlots (void) = 0;

	virtual U16 AllocateBuildSlot (U32 dwMissionID, U16 dwSlotHandle,bool bFail = false) = 0;		

	virtual void DeallocateBuildSlot (U32 dwMissionID, U16 dwSlotHandle) = 0;

	virtual bool HasOpenSlots (void) const = 0;

	virtual U32 GetSlotUser (U32 slotNumber) = 0;

	virtual void GetPlanetSlotMissionIDs (U32 * buffer, U32 slotID) = 0;

	virtual void CompleteSlotOperations (U32 workingID,U32 targetSlotID) = 0;

	virtual bool IsBuildableBy (U32 playerID) = 0;

	virtual U32 FindBestSlot (PARCHETYPE buildType, const Vector * preferedLoc = NULL) = 0;

	virtual bool MarkSlot (U32 playerID,U32 dwSlotHandle,U32 achtypeID) = 0;

	virtual void UnmarkSlot (U32 playerID,U32 dwSlotHandle) = 0;

	virtual U32 GetGas () = 0;

	virtual void SetGas (U32 newGas) = 0;

	virtual U32 GetMetal () = 0;

	virtual void SetMetal (U32 newMetal) = 0;

	virtual U32 GetCrew () =0;

	virtual void SetCrew (U32 newCrew) = 0;

	virtual bool IsMouseOver () = 0;
	
	virtual bool IsRenderingFaded () = 0;

	virtual bool IsHighlightingBuild() = 0;

	virtual void ChangePlayerRates(U32 playerID, SINGLE metalRate, SINGLE gasRate, SINGLE crewRate) = 0;

	virtual U32 GetEmptySlots() = 0;

	virtual void TeraformPlanet(const char * newArch, SINGLE changeTime, const Vector & hitDir) = 0;

	virtual bool IsMoon() = 0;

	virtual void BoostRegen(SINGLE boostOre, SINGLE boostGas, SINGLE boostCrew) = 0;
};

//---------------------------------------------------------------------------
//-----------------------END IPlanet.h---------------------------------------
//---------------------------------------------------------------------------
#endif
