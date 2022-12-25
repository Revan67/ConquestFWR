#ifndef STATUSBAR_H
#define STATUSBAR_H
//--------------------------------------------------------------------------//
//                                                                          //
//                              StatusBar.h                                 //
//                                                                          //
//               COPYRIGHT (C) 1997 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
   $Author: Jasony $

		                 Resource manager for the status bar

*/
//--------------------------------------------------------------------------//

#ifndef IRESOURCE_H
#include "IResource.h"
#endif


enum STATUSTEXTMODE
{
	STM_DEFAULT,
	STM_BUILD,
	STM_BUILD_DENIED,
	STM_TOOLTIP,
	STM_NAME,
	STM_DEFAULT_NAME		// has name and regular text
};


struct DACOM_NO_VTABLE IStatusBarResource : public IResource
{
	DEFMETHOD(SetText) (U32 dwResourceID, STATUSTEXTMODE mode=STM_DEFAULT) = 0;

	DEFMETHOD(SetTextString) (const wchar_t *string, STATUSTEXTMODE mode=STM_DEFAULT) = 0;

	// used for sector map, which has both a "name", and a regular text string
	DEFMETHOD(SetTextString2) (const wchar_t *name, const wchar_t *text) = 0;

	DEFMETHOD_(U32,GetText) (void) = 0;

	DEFMETHOD_(U32,GetHeight) (void) = 0;

	DEFMETHOD_(void,SetToolbarHeight) (U32 height) = 0;

	void SetRect (const struct tagRECT & rect) { }
};












//--------------------------------------------------------------------------//
//------------------------------End StatusBar.h-----------------------------//
//--------------------------------------------------------------------------//
#endif