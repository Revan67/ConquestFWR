#ifndef IREPAIRPLATFORM_H
#define IREPAIRPLATFORM_H
//--------------------------------------------------------------------------//
//                                                                          //
//                             IRepairPlatform.h                                //
//                                                                          //
//                  COPYRIGHT (C) 1997 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*

    $Author: Tmauer $

    $Header: /Conquest/App/Src/IRepairPlatform.h 7     9/12/00 3:04p Tmauer $
*/			    
//--------------------------------------------------------------------------//

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//

struct _NO_VTABLE IRepairPlatform : IObject
{
	virtual TRANSFORM GetShipTransform () = 0;

	virtual bool IsDockLocked() = 0;

	virtual void LockDock(IBaseObject * locker) = 0;

	virtual void UnlockDock(IBaseObject * locker) = 0;

	virtual void BeginRepairOperation(U32 agentID, IBaseObject * repairee) = 0;

	virtual void RepaireeDocked () = 0;

	virtual void RepaireeDestroyed () = 0;

	virtual void RepaireeTakeover () = 0;

	virtual void UpgradeToRepair() = 0;
};


//----------------------------------------------------------------------------------
//-----------------------END IFabricator.h------------------------------------------
//----------------------------------------------------------------------------------
#endif