#ifndef DSYSKITSAVELOAD_H
#define DSYSKITSAVELOAD_H
//--------------------------------------------------------------------------//
//                                                                          //
//                               DSysKitSaveLoad.h       							//
//                                                                          //
//                  COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/DInclude/DSysKitSaveLoad.h 1     2/11/00 3:36p Tmauer $
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

#ifndef DLISTBOX_H
#include "DListbox.h"
#endif


//--------------------------------------------------------------------------//
//  Definition system kit save load menu
//--------------------------------------------------------------------------//

struct GT_SYSTEM_KIT_SAVELOAD
{
	RECT screenRect;
	STATIC_DATA background, staticLoad, staticSave, staticFile;
	BUTTON_DATA open, save, cancel;
	EDIT_DATA editFile;
	LISTBOX_DATA list;
};

#endif
