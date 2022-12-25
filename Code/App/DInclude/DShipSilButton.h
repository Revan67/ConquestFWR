#ifndef DSHIPSILBUTTON_H
#define DSHIPSILBUTTON_H
//--------------------------------------------------------------------------//
//                                                                          //
//                              DShipSilButton.h       							//
//                                                                          //
//                  COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/DInclude/DShipSilButton.h 1     6/21/99 6:56p Tmauer $
*/			    
//--------------------------------------------------------------------------//

#ifndef DBASEDATA_H
#include "DBaseData.h"
#endif

#ifndef GT_PATH
#define GT_PATH 32
#endif

#define SHIPSILBUTTON_TYPE   "ShipSilButton!!Default"

//---------------------------------------------------------------------------
//
struct GT_SHIPSILBUTTON : GENBASE_DATA
{
	SINGLE redYellowBreak;
	SINGLE yellowGreenBreak;
};
//---------------------------------------------------------------------------
//
struct SHIPSILBUTTON_DATA
{
	S32 xOrigin, yOrigin;
};

#endif
