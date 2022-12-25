#ifndef DCHATMENU_H
#define DCHATMENU_H
//--------------------------------------------------------------------------//
//                                                                          //
//                               DChatMenu.h       							//
//                                                                          //
//                  COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/DInclude/DChatMenu.h 2     10/11/99 8:58p Sbarton $
*/			    
//--------------------------------------------------------------------------//

#ifndef DEDIT_H
#include "DEdit.h"
#endif

#ifndef DSTATIC_H
#include "DStatic.h"
#endif

//--------------------------------------------------------------------------//
//  Definition file for chat box control menu
//--------------------------------------------------------------------------//

struct GT_CHAT
{
	RECT screenRect;
	STATIC_DATA background, ask;
	EDIT_DATA chatbox;	
};

#endif
