#ifndef DMENUBRIEFING_H
#define DMENUBRIEFING_H
//--------------------------------------------------------------------------//
//                                                                          //
//                             DMenuBriefing.h    							//
//                                                                          //
//                  COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/DInclude/DMenuBriefing.h 3     6/07/00 10:09p Sbarton $
*/			    
//--------------------------------------------------------------------------//

#ifndef DSTATIC_H
#include "DStatic.h"
#endif

#ifndef DBUTTON_H
#include "DButton.h"
#endif

#ifndef DANIMATE_H
#include "DAnimate.h"
#endif

//--------------------------------------------------------------------------//
//  Definition file for in-game options
//--------------------------------------------------------------------------//

struct GT_BRIEFING
{
	RECT screenRect;
	STATIC_DATA background, title;
	BUTTON_DATA start, replay, cancel;
	RECT rcTeletype;
	RECT rcComm[4];
	ANIMATE_DATA animFuzz[4];
};

#endif