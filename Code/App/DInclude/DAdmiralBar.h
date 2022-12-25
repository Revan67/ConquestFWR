#ifndef DADMIRALBAR_H
#define DADMIRALBAR_H
//--------------------------------------------------------------------------//
//                                                                          //
//                               DAdmiralBar.h     							//
//                                                                          //
//                  COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/DInclude/DAdmiralBar.h 12    4/24/00 4:35p Sbarton $
*/			    
//--------------------------------------------------------------------------//

#ifndef DHOTBUTTON_H
#include "DHotButton.h"
#endif

#ifndef DBUTTON_H
#include "DButton.h"
#endif

#ifndef DSTATIC_H
#include "DStatic.h"
#endif

#ifndef DHOTSTATIC_H
#include "DHotStatic.h"
#endif

#ifndef DHOTBUTTON_H
#include "DHotButton.h"
#endif

#ifndef DTABCONTROL_H
#include "DTabControl.h"
#endif

//---------------------------------------------------------------------------
//
struct GT_ADMIRALBAR
{
	RECT screenRect;
	STATIC_DATA background;
	STATIC_DATA admiralKey;
	HOTBUTTON_DATA hotCreate, hotRepair, hotResupply, hotDisband, hotTransfer, hotAssault;
	
	char vfxType[GT_PATH];
	TABCONTROL_DATA tab;
	struct TAB1
	{
		HOTBUTTON_DATA hotTerran[5];
	} terranTab;
	struct TAB2
	{
		HOTBUTTON_DATA hotMantis[6];
	} mantisTab;
	struct TAB3
	{
		HOTBUTTON_DATA hotSolarian[5];
	} solarianTab;
//	struct TAB4
//	{
//		BUTTON_DATA vyrium[5];
//	} vyriumTab;
};

#endif
