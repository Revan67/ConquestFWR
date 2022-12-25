#ifndef DNEWPLAYER_H
#define DNEWPLAYER_H
//--------------------------------------------------------------------------//
//                                                                          //
//                               DNewPlayer.h      							//
//                                                                          //
//                  COPYRIGHT (C) 2000 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/DInclude/DNewPlayer.h 3     8/09/00 3:59p Sbarton $
*/			    
//--------------------------------------------------------------------------//

#ifndef DBUTTON_H
#include "DButton.h"
#endif

#ifndef DSTATIC_H
#include "DStatic.h"
#endif

#ifndef DEDIT_H
#include "DEdit.h"
#endif

struct GT_NEWPLAYER
{
	RECT screenRect;
	STATIC_DATA background, title, staticHeading;
	EDIT_DATA edit;
	BUTTON_DATA ok, cancel;
};

#endif