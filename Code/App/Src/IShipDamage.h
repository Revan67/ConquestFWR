#ifndef ISHIPDAMAGE_H
#define ISHIPDAMAGE_H
//--------------------------------------------------------------------------//
//                                                                          //
//                             IShipDamage.h                                  //
//                                                                          //
//                  COPYRIGHT (C) 1997 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*

    $Author: Tmauer $

    $Header: /Conquest/App/Src/IShipDamage.h 3     8/14/00 5:17p Tmauer $
*/			    
//--------------------------------------------------------------------------//

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//

struct _NO_VTABLE IShipDamage : IObject
{	
	virtual U32 GetNumDamageSpots () = 0;

	virtual void GetNextDamageSpot (Vector & vect, Vector & dir) = 0;

	virtual void FixDamageSpot () = 0;

	virtual struct SMesh *GetShieldMesh () = 0;

	virtual void FancyShieldRender() = 0;
};

//----------------------------------------------------------------------------------
//-------------------------END IShipDamage.h------------------------------------------
//----------------------------------------------------------------------------------
#endif