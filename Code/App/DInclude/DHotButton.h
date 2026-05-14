#ifndef DHOTBUTTON_H
#define DHOTBUTTON_H
//--------------------------------------------------------------------------//
//                                                                          //
//                              DHotButton.h       							//
//                                                                          //
//                  COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/DInclude/DHotButton.h 37    9/25/00 9:55p Tmauer $
*/			    
//--------------------------------------------------------------------------//

#ifndef DBASEDATA_H
#include "DBaseData.h"
#endif

#ifndef DMTECHNODE_H
#include "DMTechNode.h"
#endif

#ifndef HOTKEYS_H
#include "..\Src\Hotkeys.h"
#endif

#ifndef DHOTBUTTONTEXT_H
#include "DHotButtonText.h"
#endif

//---------------------------------------------------------------------------
//
struct GT_HOTBUTTON : GENBASE_DATA
{
	HOTBUTTONTYPE::TYPE buttonType;
	char fontType[GT_PATH];
	struct _textColor
	{
		U8 red,green,blue;
	}textColor;
};
//---------------------------------------------------------------------------
//
struct HOTBUTTON_DATA
{
	U32 baseImage;							// keep this in the same position as buildButton
	HBTNTXT::BUTTON_TEXT buttonText;		// tooltip, can also be used for ID
	HBTNTXT::HOTBUTTONINFO buttonInfo;		// statusbar text ID
	HBTNTXT::HOTBUTTONHINT buttonHint;		// hitbox text
	HOTKEYS::HOTKEY hotkey;
	bool bDisabled;							// button is initially disabled if true
	S32 xOrigin, yOrigin;
};
//---------------------------------------------------------------------------
//
struct BUILDBUTTON_DATA
{
	U32 baseImage;							// keep this the in same position as hotButton
	U32 noMoneyImage;
	S32 xOrigin, yOrigin;
	char rtArchetype[GT_PATH];
	SINGLE_TECHNODE techDependency, techGreyed;
	HBTNTXT::BUILD_TEXT greyedTooltip;	
	HBTNTXT::BUILDINFO buildInfo;			// rollover help info
	HOTKEYS::HOTKEY hotkey;					//for reference only
	bool bDisabled;							// button is initially disabled if true
};
//---------------------------------------------------------------------------
//
struct RESEARCHBUTTON_DATA
{
	U32 baseImage;							// keep this the in same position as hotButton
	U32 noMoneyImage;
	S32 xOrigin, yOrigin;
	char rtArchetype[GT_PATH];
	HBTNTXT::RESEARCH_TEXT tooltip;	
	HBTNTXT::RESEARCHINFO researchInfo;		// rollover help info
	HOTKEYS::HOTKEY hotkey;					//for reference only
	bool bDisabled;							// button is initially disabled if true
};
//---------------------------------------------------------------------------
//
struct MULTIHOTBUTTON_DATA
{
	S32 xOrigin, yOrigin;
	HOTKEYS::HOTKEY hotkey;
	bool bSingleShape;
	bool bDisabled;							// button is initially disabled if true
};

#endif
