#ifndef DLOADSAVE_H
#define DLOADSAVE_H
//--------------------------------------------------------------------------//
//                                                                          //
//                               DLoadSave.h       							//
//                                                                          //
//                  COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/DInclude/DLoadSave.h 4     9/13/01 10:02a Tmauer $
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
//  Definition file for load/save menu #1
//--------------------------------------------------------------------------//

struct GT_LOADSAVE
{
	RECT screenRect;
	RECT screenRect2D;
	STATIC_DATA background, staticLoad, staticSave, staticFile;
	BUTTON_DATA open, save, cancel, deleteFile;
	EDIT_DATA editFile;
	LISTBOX_DATA list;
};

#endif
