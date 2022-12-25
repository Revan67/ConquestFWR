#ifndef DRECOVERYSHIP_H
#define DRECOVERYSHIP_H
//--------------------------------------------------------------------------//
//                                                                          //
//                            DRecoveryShip.h                                 //
//                                                                          //
//                  COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/DInclude/DRecoveryShip.h 3     12/14/99 9:55a Tmauer $
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
struct BT_RECOVERYSHIP_DATA : BASE_SPACESHIP_DATA
{
	SINGLE recoveryRadius;
	SINGLE recoverTime;
	char beamPointName1[GT_PATH];
	char beamPointName2[GT_PATH];
};
//----------------------------------------------------------------
//
#ifndef _ADB
struct RECOVERYSHIP_INIT : SPACESHIP_INIT<BT_RECOVERYSHIP_DATA>
{
	U32 mineTex;
};
#endif
//----------------------------------------------------------------
//

#endif
