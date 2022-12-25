#ifndef IADMIRAL_H
#define IADMIRAL_H
//--------------------------------------------------------------------------//
//                                                                          //
//                                IAdmiral.h                                //
//                                                                          //
//                  COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/IAdmiral.h 33    7/19/01 1:43p Tmauer $
*/			    
//------------------------------- #INCLUDES --------------------------------//
//--------------------------------------------------------------------------//
#ifndef IOBJECT_H
#include "IObject.h"
#endif

#ifndef DHOTBUTTONTEXT_H
#include "DHotButtonText.h"
#endif

// this includes the assault attack
#define NUM_SPECIAL_ATTACKS 17

#define TERRAN_BEGIN   1
#define TERRAN_END     6
#define MANTIS_BEGIN   6
#define MANTIS_END     11
#define SOLARIAN_BEGIN 11
#define SOLARIAN_END   17

#define NUM_TERRAN		TERRAN_END - TERRAN_BEGIN
#define NUM_MANTIS		MANTIS_END - MANTIS_BEGIN
#define NUM_SOLARIAN	SOLARIAN_END - SOLARIAN_BEGIN

enum SpecialAttack
{
	SA_ASSAULT = 0,		// 0
	SA_AEGIS,			// 1
	SA_PROBE,			// 2
	SA_CLOAK,			// 3
	SA_VAMPIRE,			// 4
	SA_TEMPEST,			// 5
// mantis
	SA_STASIS,			// 6
	SA_FURYRAM,			// 7
	SA_REPEL,			// 8
	SA_REPULSOR,		// 9
	SA_MIMIC,			// 10
// solarian
	SA_SYNTHESIS,		// 11
	SA_MASSDISRUPT,		// 12
	SA_DESTABILIZER,	// 13	
	SA_SOLARIANCLOAK,	// 14
	SA_SHROUD,			// 15
	SA_AUGER			// 16
};

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
struct _NO_VTABLE IAdmiral : IObject 
{
	virtual void DockFlagship (IBaseObject * target, U32 agentID) = 0;

	virtual void UndockFlagship (U32 agentID, U32 dockshipID) = 0;

	virtual void OnFleetShipDamaged (IBaseObject * victim, U32 attackerID) = 0;

	virtual void OnFleetShipDestroyed (IBaseObject * victim) = 0;

	virtual void OnFleetShipTakeover (IBaseObject * victim) = 0;

	virtual IBaseObject * FleetShipTargetDestroyed (IBaseObject * ship) = 0;		// admiral should return a new target

	virtual U32  GetFleetMembers (U32 objectIDs[MAX_SELECTED_UNITS]) = 0;

	virtual void SetFleetMembers (U32 objectIDs[MAX_SELECTED_UNITS], U32 numUnits) = 0;

	virtual void RemoveShipFromFleet (U32 shipID) = 0;

	virtual U32 GetAdmiralHotkey (void) = 0;

	virtual void SetAdmiralHotkey(U32 hotkey) = 0;

	virtual BOOL32 IsDocked (void) = 0;

	virtual IBaseObject * GetDockship (void) = 0; // return the admiral's dockship - NULL if no dockship present

	virtual SINGLE GetDamageBonus(M_OBJCLASS sourceMObjClass,ARMOR_TYPE sourceArmor,OBJCLASS targetObjClass,M_RACE targetRace, ARMOR_TYPE targetArmor) = 0;

	virtual SINGLE GetDefenceBonus(M_OBJCLASS targetMObjClass, ARMOR_TYPE targetArmor) = 0;

	virtual SINGLE GetFighterDamageBonus() =0;

	virtual SINGLE GetSpeedBonus(M_OBJCLASS shipMObjClass, ARMOR_TYPE shipArmor) = 0;

	virtual U32 ConvertSupplyBonus(M_OBJCLASS shipMObjClass, ARMOR_TYPE shipArmor, U32 suppliesUsed) = 0;

	virtual SINGLE GetSensorBonus(M_OBJCLASS shipMObjClass, ARMOR_TYPE shipArmor) = 0;

	virtual SINGLE GetFighterTargetingBonus() = 0;

	virtual SINGLE GetFighterDodgeBonus() = 0;

	virtual SINGLE GetRangeBonus(M_OBJCLASS shipMObjClass, ARMOR_TYPE shipArmor) = 0;

	virtual bool IsInFleetRepairMode() =0;

	virtual U32 GetToolbarImage() = 0;

	virtual HBTNTXT::BUTTON_TEXT GetToolbarText() = 0;

	virtual HBTNTXT::HOTBUTTONINFO GetToolbarStatusText() = 0;

	virtual HBTNTXT::MULTIBUTTONINFO GetToolbarHintbox () = 0;

	virtual void SetAdmiralTactic (ADMIRAL_TACTIC tactic) = 0;

	virtual ADMIRAL_TACTIC GetAdmiralTactic () = 0;

	virtual void SetFormation (U32 formationArchID) = 0;

	virtual U32 GetFormation () = 0;

	virtual U32 GetKnownFormation(U32 index) = 0;

	virtual void LearnCommandKit(const char * kitName) = 0;

	virtual void RenderFleetInfo() = 0;

	virtual void HandleMoveCommand(const NETGRIDVECTOR &vec) = 0;

	virtual void HandleAttackCommand(U32 targetID, U32 destSystemID) = 0;

	virtual bool IsInLockedFormation() = 0;

	virtual Vector GetFormationDir() = 0;

	virtual void MoveDoneHint(IBaseObject * ship) = 0;

	virtual struct BT_COMMAND_KIT * GetAvailibleCommandKit(U32 index) = 0;

	virtual U32 GetAvailibleCommandKitID(U32 index) = 0;
	
	virtual bool IsKnownKit(struct BT_COMMAND_KIT * comKit) = 0;

	virtual struct BT_COMMAND_KIT * GetKnownCommandKit(U32 index) = 0;

	virtual bool CanResearchKits() = 0;
};
//--------------------------------------------------------------------------//
//
struct _NO_VTABLE IFleetShip : IObject
{
	virtual void WaitForAdmiral (bool bWait) = 0;			// admiral is enroute to board you

	virtual bool IsAvailableForAdmiral (void) = 0;

	virtual void SetFleetshipTarget (IBaseObject * target) = 0;

	virtual IBaseObject * GetFleetshipTarget() = 0;

	virtual bool IsInMove (void) = 0;
};
//--------------------------------------------------------------------------//
//
//this is and interface to the menu management code in mission.
struct _NO_VTABLE IFleetMenu 
{
	virtual SpecialAttack GetSpecialAttackType(U32 buttinIndex) = 0;
};
//----------------------------------------------------------------------------
//-----------------------END IAdmiral.h---------------------------------------
//----------------------------------------------------------------------------
#endif
