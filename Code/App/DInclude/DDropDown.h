#ifndef DDROPDOWN_H
#define DDROPDOWN_H
//--------------------------------------------------------------------------//
//                                                                          //
//                               DDropDown.h                                //
//                                                                          //
//                  COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/DInclude/DDropDown.h 2     2/05/99 7:19p Jasony $
*/			    
//--------------------------------------------------------------------------//

#ifndef DBASEDATA_H
#include "DBaseData.h"
#endif

#ifndef DLISTBOX_H
#include "DListBox.h"
#endif

#ifndef DBUTTON_H
#include "DButton.h"
#endif


//--------------------------------------------------------------------------//
//  Definition file for dropdown controls
//--------------------------------------------------------------------------//

struct GT_DROPDOWN : GENBASE_DATA
{
	// nothing to add?
};

//--------------------------------------------------------------------------//
//
struct DROPDOWN_DATA
{
    char dropdownType[GT_PATH];
	RECT screenRect;
	BUTTON_DATA buttonData;
	LISTBOX_DATA listboxData;
};

#endif
