#ifndef DSLIDER_H
#define DSLIDER_H
//--------------------------------------------------------------------------//
//                                                                          //
//                               DSlider.h                                  //
//                                                                          //
//                  COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/DInclude/DSlider.h 5     6/21/00 1:05p Sbarton $
*/			    
//--------------------------------------------------------------------------//

#ifndef DBASEDATA_H
#include "DBaseData.h"
#endif

//---------------------------------------------------------------------------
// slider color states
//---------------------------------------------------------------------------
#define GTS_DISABLED		0
#define GTS_NORMAL			1
#define GTS_HIGHLIGHT		2
#define GTS_ALERT			3

#define GTS_MAX_STATES		4

//--------------------------------------------------------------------------//
//  Definition file for slider controls
//--------------------------------------------------------------------------//

struct GT_SLIDER : GENBASE_DATA
{
	struct COLOR
	{
		unsigned char red, green, blue;
	} disabledColor, normalColor, highlightColor, alertColor;
	bool bVertical;
	U32  indent;
	char shapeFile[GT_PATH];
};

//--------------------------------------------------------------------------//
//
struct SLIDER_DATA
{
    char sliderType[GT_PATH];
	RECT screenRect;
	S32 xOrigin, yOrigin;
};

#endif
