#ifndef DTABCONTROL_H
#define DTABCONTROL_H
//--------------------------------------------------------------------------//
//                                                                          //
//                               DTabControl.h    							//
//                                                                          //
//                  COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/DInclude/DTabControl.h 6     7/24/00 7:58p Sbarton $
*/			    
//--------------------------------------------------------------------------//

#ifndef DBASEDATA_H
#include "DBaseData.h"
#endif

#ifndef DHOTBUTTON_H
#include "DHotButton.h"
#endif

#ifndef GT_PATH
#define GT_PATH 32
#endif

#define MAX_TABS  6
#define TABCONTROLTYPE "Tab!!Test"

//--------------------------------------------------------------------------//
//
namespace TABTXT
{
enum TAB_TEXT
{
	NO_TAB_TEXT=0,
	PLAYER = IDS_TAB_PLAYER,
	GRAPHICS = IDS_TAB_GRAPHICS,
	SOUNDS = IDS_TAB_SOUNDS,
	TOTALS = IDS_TAB_TOTALS,
	UNITS = IDS_TAB_UNITS,
	BUILDINGS = IDS_TAB_BUILDINGS,
	TERRITORY = IDS_TAB_TERRITORY,
	RESOURCES = IDS_TAB_RESOURCES
};
} // end namespace


//---------------------------------------------------------------------------
//
struct GT_TABCONTROL : GENBASE_DATA
{
	struct COLOR
	{
		unsigned char red, green, blue;
	} normalColor, hiliteColor, selectedColor;	
};
//---------------------------------------------------------------------------
//
struct TABCONTROL_DATA
{
    char tabControlType[GT_PATH];
	char hotButtonType[GT_PATH];
	int  iBaseImage;
	int  numTabs;
	TABTXT::TAB_TEXT textID[MAX_TABS];
	bool bUpperTabs;
	int xpos;
	int ypos;
};

#endif
