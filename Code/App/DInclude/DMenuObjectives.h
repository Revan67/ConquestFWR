#ifndef DMENUOBJECTIVES_H
#define DMENUOBJECTIVES_H
//--------------------------------------------------------------------------//
//                                                                          //
//                             DMenuObjectives.h  							//
//                                                                          //
//                  COPYRIGHT (C) 2000 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/DInclude/DMenuObjectives.h 7     9/08/00 7:50p Sbarton $
*/			    
//--------------------------------------------------------------------------//

#ifndef DSTATIC_H
#include "DStatic.h"
#endif

#ifndef DLISTBOX_H
#include "DListbox.h"
#endif

#ifndef DBUTTON_H
#include "DButton.h"
#endif

#define MAX_MISSION_OBJECTIVES_SHOWN 9				// also defined in DGlobalData.h

struct GT_MENUOBJECTIVES
{
	RECT screenRect;
	STATIC_DATA background, staticObjectives, staticName;
	RECT rcTeletype;
	BUTTON_DATA buttonOk;
	BUTTON_DATA checkObjectives[MAX_MISSION_OBJECTIVES_SHOWN];
	STATIC_DATA staticObjectiveArray[MAX_MISSION_OBJECTIVES_SHOWN];
};

#endif