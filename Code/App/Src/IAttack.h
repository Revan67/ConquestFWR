#ifndef IATTACK_H
#define IATTACK_H
//--------------------------------------------------------------------------//
//                                                                          //
//                                IAttack.h                                 //
//                                                                          //
//                  COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/IAttack.h 36    9/13/01 10:01a Tmauer $
*/			    
//------------------------------- #INCLUDES --------------------------------//
//--------------------------------------------------------------------------//
#ifndef IOBJECT_H
#include "IObject.h"
#endif

#ifndef DMISSIONENUM_H
#include <DMissionEnum.h>
#endif

#ifndef ILAUNCHER_H
#include "ILauncher.h"
#endif


struct _NO_VTABLE IToggle : IObject
{
	virtual bool IsToggle() = 0;

	virtual bool CanToggle() = 0;

	virtual bool IsOn() = 0;
};

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//

struct _NO_VTABLE IAttack : IObject
{
	/* regular attack methods */
	virtual void Attack (IBaseObject * victim, U32 agentID, bool bUserGenerated) = 0;	// moves to attack

	virtual void AttackPosition(const struct GRIDVECTOR & position, U32 agentID) = 0; 

	virtual void CancelAttack (void) = 0;

	/* special attack methods */
	
	virtual void SpecialAttack (IBaseObject * victim, U32 agentID) = 0;

	virtual void SpecialAOEAttack (const struct GRIDVECTOR & position, U32 agentID) = 0;

	virtual void WormAttack (IBaseObject * victim, U32 agentID) = 0;

	/* both attack methods */

	// called when you fatally wound your target, your operation is complete.
	virtual void ReportKill (U32 partID) = 0;

	// if target==0, vec is valid. else vec should be ignored
	virtual void Escort (IBaseObject * target, U32 agentID) = 0;

	virtual void MultiSystemAttack (struct GRIDVECTOR & position, U32 targSystemID, U32 agentID) = 0;

	virtual void DoCreateWormhole(U32 systemID, U32 agentID) = 0;

	virtual const UNIT_STANCE GetUnitStance (void) const  = 0;

	virtual void GetTarget(IBaseObject* & targObj, U32 targID) = 0;

	/* what special attack (if any) does this unit have, and can we use it?  Variables get overwritten */

	virtual void  GetSpecialAbility (UNIT_SPECIAL_ABILITY & ability, bool & bEnabled) = 0; 

	// stop attacking allies!
	virtual void  OnAllianceChange (U32 allyMask) = 0;

	// stance-like functions, only called on the host

	virtual void SetUnitStance (const UNIT_STANCE stance) = 0;

	virtual void SetFighterStance (const FighterStance stance) = 0;

	virtual const FighterStance GetFighterStance (void) = 0;

	virtual void DoSpecialAbility (U32 specialID) = 0;

	virtual void DoSpecialAbility (IBaseObject * target) = 0;

	virtual void DoCloak (void) = 0;
};
/*
struct _NO_VTABLE IMinelayerControl : IObject
{
	virtual void SetDroppingMines (const bool bDrop) = 0;

	virtual const bool GetDroppingMines
}
*/
//---------------------------------------------------------------------------
//-----------------------END IAttack.h---------------------------------------
//---------------------------------------------------------------------------
#endif
