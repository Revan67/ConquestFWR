#ifndef SCROLLINGTEXT_H
#define SCROLLINGTEXT_H
//--------------------------------------------------------------------------//
//                                                                          //
//                        ScrollingText.h                                   //
//                                                                          //
//               COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
   $Author: Sbarton $

		Scrolling Text control 

*/
//--------------------------------------------------------------------------//

#define CHARCOUNT(x) (sizeof(x)/sizeof(x[0]))
#define MAX_NAME_LENGTH 32
#define MAX_LINES_TEXT 4
#define TEXT_LIFETIME 6000
#define XPOS_TEXT 10

struct DACOM_NO_VTABLE IScrollingText : IDAComponent
{
	DEFMETHOD_(void,Redraw) (void) = 0;

	DEFMETHOD(SetText) (U32 dwResourceID) = 0;

	DEFMETHOD(SetTextString) (const wchar_t *string) = 0;

	DEFMETHOD(SetTextStringEx) (const wchar_t *strmsg, U32 playerID, U32 dplayID = 0) = 0;

	DEFMETHOD_(U32,GetText) (void) = 0;

	DEFMETHOD_(U32,GetHeight) (void) = 0;

	DEFMETHOD_(void,SetToolbarHeight) (U32 height) = 0;
};

struct DACOM_NO_VTABLE IPlayerChat : IDAComponent
{
	DEFMETHOD(AddTextString) (const wchar_t *string) = 0;

	DEFMETHOD(AddTextString) (const wchar_t *string, const U32 playerID, const U32 dplayID) = 0;
};






//--------------------------------------------------------------------------//
//------------------------------End StatusBar.h-----------------------------//
//--------------------------------------------------------------------------//
#endif