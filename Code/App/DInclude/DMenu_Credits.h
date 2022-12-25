#ifndef DMENUCREDITS_H
#define DMENUCREDITS_H
//--------------------------------------------------------------------------//
//                                                                          //
//                               DMenu_Credits.h   							//
//                                                                          //
//                  COPYRIGHT (C) 2000 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/DInclude/DMenu_Credits.h 2     9/04/00 3:41p Sbarton $
*/			    
//--------------------------------------------------------------------------//

#ifndef DSTATIC_H
#include "DStatic.h"
#endif

//--------------------------------------------------------------------------//
//  Definition file for a full-screen credits menu
//--------------------------------------------------------------------------//

struct GT_CREDITS
{
	RECT screenRect;
	STATIC_DATA staticBackground;
};

#endif