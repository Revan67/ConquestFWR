#ifndef IREPAIREE_H
#define IREPAIREE_H
//--------------------------------------------------------------------------//
//                                                                          //
//                             IRepairee.h                                //
//                                                                          //
//                  COPYRIGHT (C) 1997 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*

    $Author: Tmauer $

    $Header: /Conquest/App/Src/IRepairee.h 5     10/09/00 11:11a Tmauer $
*/			    
//--------------------------------------------------------------------------//

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//

struct _NO_VTABLE IRepairee : IObject
{
	// repair
	virtual void RepairYourselfAt (IBaseObject * platform, U32 agentID) = 0;

	virtual void RepairStartReceived (IBaseObject * platform) = 0;

	virtual void RepairCompleted (void) = 0;

	virtual TRANSFORM RepairIdlePos (void) = 0;
};


//----------------------------------------------------------------------------------
//-----------------------END IRepairee.h------------------------------------------
//----------------------------------------------------------------------------------
#endif