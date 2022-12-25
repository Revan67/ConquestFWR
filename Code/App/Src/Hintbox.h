#ifndef HINTBOX_H
#define HINTBOX_H
//--------------------------------------------------------------------------//
//                                                                          //
//                              Hintbox.h                                   //
//                                                                          //
//               COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
   $Header: /Conquest/App/Src/Hintbox.h 1     9/15/99 11:04a Jasony $

		                 Resource manager for the hint system
*/
//--------------------------------------------------------------------------//

#ifndef IRESOURCE_H
#include "IResource.h"
#endif

//--------------------------------------------------------------------------//
//
struct DACOM_NO_VTABLE IHintResource : public IResource
{
	virtual void __stdcall SetText (U32 dwResourceID) = 0;

	virtual void __stdcall SetTextString (const wchar_t *string) = 0;

	virtual U32 __stdcall GetText (void) = 0;

	virtual U32 __stdcall GetHeight (void) = 0;

	virtual void __stdcall SetToolbarHeight (U32 height) = 0;
};

//--------------------------------------------------------------------------//
//------------------------------End Hintbox.h-------------------------------//
//--------------------------------------------------------------------------//
#endif