#ifndef DCOMBOBOX_H
#define DCOMBOBOX_H
//--------------------------------------------------------------------------//
//                                                                          //
//                               DCombobox.h                                //
//                                                                          //
//                  COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/DInclude/DCombobox.h 2     8/31/99 10:12a Sbarton $
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


#ifndef DEDIT_H
#include "DEdit.h"
#endif


//--------------------------------------------------------------------------//
//  Definition file for dropdown controls
//--------------------------------------------------------------------------//

struct GT_COMBOBOX : GENBASE_DATA
{
	// nothing to add?
};

//--------------------------------------------------------------------------//
//
struct COMBOBOX_DATA
{
    char comboboxType[GT_PATH];
	RECT screenRect;
	EDIT_DATA editData;
	BUTTON_DATA buttonData;
	LISTBOX_DATA listboxData;
};

#endif
