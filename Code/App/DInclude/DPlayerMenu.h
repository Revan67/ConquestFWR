#ifndef DPLAYERMENU_H
#define DPLAYERMENU_H
//--------------------------------------------------------------------------//
//                                                                          //
//                             DPlayerMenu.h       							//
//                                                                          //
//                  COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/DInclude/DPlayerMenu.h 11    9/27/00 10:13p Sbarton $
*/			    
//--------------------------------------------------------------------------//

#ifndef DSTATIC_H
#include "DStatic.h"
#endif

#ifndef DLISTBOX_H
#include "DListbox.h"
#endif

#ifndef DBUTTON_H
#include "DButton.h"
#endif

#ifndef DEDIT_H
#include "DEdit.h"
#endif

#ifndef DDIPLOMACYBUTTON_H
#include "DDiplomacyButton.h"
#endif

//--------------------------------------------------------------------------//
//  Definition file for in-game options
//--------------------------------------------------------------------------//

struct GT_DIPLOMACYMENU
{
	RECT screenRect;
	STATIC_DATA	background;
	STATIC_DATA staticTitle, staticName, staticRace, staticAllies, staticMetalTitle, staticGasTitle, staticCrewTitle;
	STATIC_DATA staticNames[7], staticRaces[7];
	BUTTON_DATA buttonCrew[7], buttonMetal[7], buttonGas[7];
	BUTTON_DATA buttonAllies[7];

	STATIC_DATA staticCrew, staticMetal, staticGas;
	BUTTON_DATA buttonOk, buttonReset, buttonCancel, buttonApply;

	DIPLOMACYBUTTON_DATA diplomacyButtons[7];
};

struct GT_PLAYERCHATMENU
{
	RECT screenRect;
	STATIC_DATA background;
	STATIC_DATA staticNames[7], staticRaces[7];
	BUTTON_DATA checkNames[7];
	BUTTON_DATA buttonAllies, buttonEnemies, buttonEveryone;
	LISTBOX_DATA listChat;
	EDIT_DATA editChat;
	BUTTON_DATA buttonClose;
	STATIC_DATA staticTitle;
	STATIC_DATA staticChat;
};

#endif