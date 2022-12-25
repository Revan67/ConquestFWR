#ifndef ISHIPMOVE_H
#define ISHIPMOVE_H
//--------------------------------------------------------------------------//
//                                                                          //
//                               IShipMove.h                                //
//                                                                          //
//                  COPYRIGHT (C) 2000 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/IShipMove.h 13    9/25/00 12:45p Rmarr $
*/			    
//------------------------------- #INCLUDES --------------------------------//
//--------------------------------------------------------------------------//
#ifndef IOBJECT_H
#include "IObject.h"
#endif

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
struct IShipMove : IObject
{
	virtual void PushShip (U32 attackerID, const Vector & direction, SINGLE velMag) = 0;

	virtual void PushShipTo (U32 attackerID, const Vector & position, SINGLE velMag) = 0;

	virtual void DestabilizeShip (U32 attackerID) = 0;

	virtual void ForceShipOrientation (U32 attackerID, SINGLE yaw) = 0;

	virtual void ReleaseShipControl (U32 attackerID) = 0;

	virtual SINGLE GetCurrentCruiseVelocity (void) = 0;

	virtual void RemoveFromMap (void) = 0;

	virtual bool IsMoving (void) =0;
};

//---------------------------------------------------------------------------
//--------------------------END IShipMove.h----------------------------------
//---------------------------------------------------------------------------
#endif
