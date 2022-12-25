#ifndef ISUPPLIER_H
#define ISUPPLIER_H
//--------------------------------------------------------------------------//
//                                                                          //
//                             ISupplier.h                                  //
//                                                                          //
//                  COPYRIGHT (C) 1997 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*

    $Author: Jasony $

    $Header: /Conquest/App/Src/ISupplier.h 8     7/15/00 6:07p Jasony $
*/			    
//--------------------------------------------------------------------------//

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//

struct _NO_VTABLE ISupplier : IObject
{	
	virtual void SupplyTarget (U32 agentID, IBaseObject * obj) = 0;

	virtual void SetSupplyEscort (U32 agentID, IBaseObject * target) = 0;

	virtual void SetSupplyStance (enum SUPPLY_SHIP_STANCE _bAutoSupply) = 0;

	virtual enum SUPPLY_SHIP_STANCE GetSupplyStance (void) = 0;
};

//----------------------------------------------------------------------------------
//-------------------------END ISupplier.h------------------------------------------
//----------------------------------------------------------------------------------
#endif