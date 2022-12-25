#ifndef DTRAIL_H
#define DTRAIL_H
//--------------------------------------------------------------------------//
//                                                                          //
//                                DPlanet.h                                 //
//                                                                          //
//                  COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*

    $Author: Jasony $

    $Header: /Conquest/App/DInclude/DTrail.h 4     4/20/99 6:34p Jasony $
*/			    
//--------------------------------------------------------------------------//
#ifndef DBASEDATA_H
#include "DBaseData.h"
#endif

#ifndef DTYPES_H
#include "DTypes.h"
#endif

#ifndef DINSTANCE_H
#include "DInstance.h"
#endif

//----------------------------------------------------------------
//

struct BT_TRAIL_DATA : BASIC_DATA
{
    char anim[GT_PATH];
};


#endif
