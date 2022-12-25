#ifndef DFONTS_H
#define DFONTS_H
//--------------------------------------------------------------------------//
//                                                                          //
//                               DFonts.h       							//
//                                                                          //
//                  COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/DInclude/DFonts.h 30    10/21/00 9:54p Sbarton $
*/			    
//--------------------------------------------------------------------------//

#ifndef DBASEDATA_H
#include "DBaseData.h"
#endif

#include "..\Src\Resource.h"

//---------------------------------------------------------------------------
//
struct GT_FONT : GENBASE_DATA
{
	enum FONT
	{
		BUTTON_FONT			= IDS_BUTTON_FONT,
		DESCRIPTION_FONT	= IDS_DESCRIPTION_FONT,
		STATIC_CONTROL		= IDS_STATIC_CONTROL_FONT,
		GAMEROOM_LIST		= IDS_GAMEROOMLIST_FONT,
		CHATLIST_FONT		= IDS_CHATLIST_FONT,
		OBJTYPE_FONT		= IDS_LOCAL_DISPLAYNAME_FONT,			// font for showing name next to ship in 3D
		GAMEROOM_DROPDOWN	= IDS_SMALL_DROPDOWN_FONT,
		MONEY_FONT			= IDS_TOOLBAR_MONEY_FONT,
		CONTEXTWINDOW		= IDS_CONTEXTWINDOW,
		ENDGAMEBANNER		= IDS_ENDGAMEBANNER_FONT,
		DEFLISTBOX			= IDS_DEFAULTLISTBOX_FONT,
		MESSAGEBOX_FONT		= IDS_MESSAGEBOX_FONT,
		OPENING_FONT		= IDS_OPENING_FONT,
		FUTURE_PLAYER		= IDS_FONT_PLAYERS,
		FUTURE_LISTBOX		= IDS_FUTURELB_FONT,
		PLANET_TITLE		= IDS_PLANET_TITLE_FONT,
		PLANET_INFO			= IDS_PLANET_INFO_FONT,
		DROP_SMALL			= IDS_FONT_DROPSMALL,
		OCR_16_700			= IDS_FONT_OCR_16_700,
		NEURO_14_600		= IDS_NEURO_14_600,
		COURIER_12_400		= IDS_COURIER_12_400,
		NEURO_12_400		= IDS_NEURO_12_400,
		OBJECTIVES			= IDS_FONT_OBJECTIVES,
		LEGAL_FONT			= IDS_HINTBOX_FONT,
		COUNTDOWN			= IDS_FONT_COUNTDOWN,
		BUTTON3D			= IDS_BUTTON3D_FONT
	} font;

	bool bMultiline:1;
	bool bNotScaling:1;		// true = font size not proportional to screen res
	bool bToolbarMoney:1;	// optimized for displaying money
};
//---------------------------------------------------------------------------
//

#endif
