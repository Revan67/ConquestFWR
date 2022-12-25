#ifndef DIGOPTIONS_H
#define DIGOPTIONS_H
//--------------------------------------------------------------------------//
//                                                                          //
//                             Digoptions.h       							//
//                                                                          //
//                  COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/DInclude/Digoptions.h 29    8/23/01 9:12a Tmauer $
*/			    
//--------------------------------------------------------------------------//

#ifndef DBUTTON_H
#include "DButton.h"
#endif

#ifndef DSTATIC_H
#include "DStatic.h"
#endif

#ifndef DLISTBOX_H
#include "DListbox.h"
#endif

#ifndef DSLIDER_H
#include "DSlider.h"
#endif

#ifndef DDROPDOWN_H
#include "DDropDown.h"
#endif

#ifndef DTABCONTROL_H
#include "DTabControl.h"
#endif

//--------------------------------------------------------------------------//
//  Definition file for in-game options
//--------------------------------------------------------------------------//

struct GT_IGOPTIONS
{
	RECT screenRect;
	STATIC_DATA background, title;

	BUTTON_DATA buttonSave, buttonLoad;
	BUTTON_DATA buttonOptions;
	BUTTON_DATA buttonRestart;
	BUTTON_DATA buttonResign;
	BUTTON_DATA buttonAbdicate;
	BUTTON_DATA buttonReturn;
};


struct GT_OPTIONS
{
	RECT screenRect;
	STATIC_DATA background, title;

	STATIC_DATA staticName;
	LISTBOX_DATA listNames;
	BUTTON_DATA buttonNew, buttonChange, buttonDelete;

	STATIC_DATA staticSound, staticMusic, staticComm, staticChat, staticSpeed, staticScroll, staticMouse;
	SLIDER_DATA sliderSound, sliderMusic, sliderComm, sliderChat, sliderSpeed, sliderScroll, sliderMouse;
	BUTTON_DATA pushSound, pushMusic, pushComm, pushChat;

	STATIC_DATA staticDInput;
	BUTTON_DATA pushDInput;

	STATIC_DATA staticStatus, staticRollover, staticSectorMap, staticRightClick, staticSubtitles;
	BUTTON_DATA pushStatus, pushRollover, pushSectorMap, pushRightClick, pushSubtitles;

	STATIC_DATA staticGamma, staticResolution;
	DROPDOWN_DATA dropResolution;
	SLIDER_DATA sliderGamma;

	STATIC_DATA staticShips3D, staticTrails, staticEmissive, staticDetail, staticDrawBack;
	BUTTON_DATA pushTrails, pushEmissive, pushDetail;
	SLIDER_DATA slideDrawBack, sliderShips3D;

	STATIC_DATA   staticDevice;
	DROPDOWN_DATA dropDevice;

	STATIC_DATA static3DHardware;
	BUTTON_DATA push3DHardware;

	TABCONTROL_DATA tab;

	BUTTON_DATA buttonOk, buttonCancel;
};


#endif
