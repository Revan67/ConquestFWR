#ifndef DPAUSE_H
#define DPAUSE_H
//--------------------------------------------------------------------------//
//                                                                          //
//                               DPause.h       							//
//                                                                          //
//                  COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/DInclude/DPause.h 4     9/30/00 12:39p Jasony $
*/			    
//--------------------------------------------------------------------------//

#ifndef DBUTTON_H
#include "DButton.h"
#endif

#ifndef DSTATIC_H
#include "DStatic.h"
#endif

//--------------------------------------------------------------------------//
//  Definition file for pause menu
//--------------------------------------------------------------------------//
#define MAX_PLAYERS 8

struct GT_PAUSE
{
	RECT screenRect;
	STATIC_DATA staticTitle, staticDescription[MAX_PLAYERS];
};

#endif
