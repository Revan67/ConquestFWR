#ifndef DLISTBOX_H
#define DLISTBOX_H
//--------------------------------------------------------------------------//
//                                                                          //
//                               DListBox.h                                 //
//                                                                          //
//                  COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/DInclude/DListBox.h 11    8/21/00 12:48p Sbarton $
*/			    
//--------------------------------------------------------------------------//

#ifndef DBASEDATA_H
#include "DBaseData.h"
#endif

#ifndef GT_PATH
#define GT_PATH 32
#endif

//---------------------------------------------------------------------------
// text color states
//---------------------------------------------------------------------------
#define GTLBTXT_DISABLED                 0
#define GTLBTXT_NORMAL                   1
#define GTLBTXT_HIGHLIGHT                2
#define GTLBTXT_SELECTED                 3		// background of text when selected
#define GTLBTXT_SELECTED_GRAYED			 4		// background of text when selected, but not keyboard focus

#define GTLBTXT_MAX_STATES               5

//--------------------------------------------------------------------------//
//  Definition file for list controls
//--------------------------------------------------------------------------//

struct GT_LISTBOX : GENBASE_DATA
{
	char fontName[GT_PATH];
	struct COLOR
	{
		unsigned char red, green, blue;
	} disabledText, normalText, highlightText, selectedText, selectedTextGrayed;

	char shapeFile[GT_PATH];			// vfx shape file
	char scrollBarType[GT_PATH];
};
//---------------------------------------------------------------------------
//
struct LISTBOX_DATA
{
    char listboxType[GT_PATH];
    S32 xOrigin, yOrigin;			// relative to parent
	RECT textArea;					// relative to this control's origin
	U32  leadingHeight;				// extra separation between lines of text
	bool bStatic:1;					// style bit, true if control is for display only, no user selection
	bool bSingleClick:1;			// single click to select an item, caret follows the cursor
	bool bScrollbar:1;
	bool bSolidBackground:1;		// default is hash background
	bool bNoBorder:1;
	bool bDisableMouseSelect:1;
};

#endif
