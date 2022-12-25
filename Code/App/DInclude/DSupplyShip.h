#ifndef DSUPPLYSHIP_H
#define DSUPPLYSHIP_H
//--------------------------------------------------------------------------//
//                                                                          //
//                            DSupplyShip.h                                 //
//                                                                          //
//                  COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/DInclude/DSupplyShip.h 7     9/15/00 10:47p Tmauer $
*/			    
//--------------------------------------------------------------------------//
#ifndef DBASEDATA_H
#include "DBaseData.h"
#endif

#ifndef DSPACESHIP_H
#include "DSpaceship.h"
#endif

#ifndef DTYPES_H
#include "DTypes.h"
#endif

#ifndef DINSTANCE_H
#include "DInstance.h"
#endif

//----------------------------------------------------------------
//
struct BT_SUPPLYSHIP_DATA : BASE_SPACESHIP_DATA
{
	SINGLE suppliesPerSecond;
	SINGLE supplyRange;
};
//----------------------------------------------------------------
//
#ifndef _ADB
struct SUPPLYSHIP_INIT : SPACESHIP_INIT<BT_SUPPLYSHIP_DATA>
{
};
#endif
//----------------------------------------------------------------
//

#endif
