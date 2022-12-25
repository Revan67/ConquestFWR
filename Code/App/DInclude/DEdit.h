#ifndef DEDIT_H
#define DEDIT_H
//--------------------------------------------------------------------------//
//                                                                          //
//                               DEdit.h                                                              //
//                                                                          //
//                  COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/DInclude/DEdit.h 11    8/09/00 3:59p Sbarton $
*/			    
//--------------------------------------------------------------------------//

#ifndef DBASEDATA_H
#include "DBaseData.h"
#endif

#ifndef GT_PATH
#define GT_PATH 32
#endif

namespace EDTXT
{
enum EDIT_TEXT
{
	NONE=0,
    ETXT_TEST = IDS_TEST_BUTTON_TEXT,
	IP_ADDRESS_DEFAULT = IDS_DEFAULT_IP_ADDRESS
};
} // end namespace EDTXT

//---------------------------------------------------------------------------
// order of the images in the edit shape file
//---------------------------------------------------------------------------
#define GTESHP_DISABLED                 0
#define GTESHP_NORMAL                   1
#define GTESHP_MOUSE_FOCUS              2                
#define GTESHP_KEYB_FOCUS               3                       // overlay

#define GTESHP_MAX_SHAPES               4
//---------------------------------------------------------------------------
// text color states
//---------------------------------------------------------------------------
#define GTETXT_DISABLED                 0
#define GTETXT_NORMAL                   1
#define GTETXT_HIGHLIGHT                2
#define GTETXT_SELECTED                 3		// background of text when selected
#define GTETXT_CARET					4		// caret color

#define GTETXT_MAX_STATES               5

//---------------------------------------------------------------------------
//
struct GT_EDIT : GENBASE_DATA
{
	char fontName[GT_PATH];
	struct COLOR
	{
		unsigned char red, green, blue;
	} disabledText, normalText, highlightText, selectedText, caret;

	char shapeFile[GT_PATH];			// vfx shape file
	S32 justify;
	S32 width;
	S32 height;
};
//---------------------------------------------------------------------------
//
struct EDIT_DATA
{
	char editType[GT_PATH];
	EDTXT::EDIT_TEXT editText;         // initial value, can also be used for ID
	S32 xOrigin, yOrigin;
};


#endif
