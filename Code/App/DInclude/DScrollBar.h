#ifndef DSCROLLBAR_H
#define DSCROLLBAR_H
//--------------------------------------------------------------------------//
//                                                                          //
//                              DScrollBar.h                                   //
//                                                                          //
//                  COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/DInclude/DScrollBar.h 5     5/26/00 7:12p Sbarton $
*/			    
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
#ifndef DBASEDATA_H
#include "DBaseData.h"
#endif

struct GT_SCROLLBAR : GENBASE_DATA
{
	char upButtonType[GT_PATH];
	char downButtonType[GT_PATH];
	struct COLOR
	{
		unsigned char red, green, blue;
	} thumbColor, backgroundColor, disabledColor;
	bool bHorizontal:1;
	char shapeFile[GT_PATH];
};

struct SCROLLBAR_DATA 
{
	char scrollBarType[GT_PATH];
};

#endif