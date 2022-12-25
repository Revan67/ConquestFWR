#ifndef DCONFIRM_H
#define DCONFIRM_H
//--------------------------------------------------------------------------//
//                                                                          //
//                               DConfirm.h       							//
//                                                                          //
//                  COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/DInclude/DConfirm.h 3     7/24/00 10:48a Sbarton $
*/			    
//--------------------------------------------------------------------------//

#ifndef DBUTTON_H
#include "DButton.h"
#endif

#ifndef DSTATIC_H
#include "DStatic.h"
#endif

struct GT_MESSAGEBOX
{
	STATIC_DATA background, title;
	STATIC_DATA message;
	BUTTON_DATA ok, cancel;
	BUTTON_DATA okAlone;
};

#endif
