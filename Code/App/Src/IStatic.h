#ifndef ISTATIC_H
#define ISTATIC_H
//--------------------------------------------------------------------------//
//                                                                          //
//                              IStatic.h                                   //
//                                                                          //
//                  COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/IStatic.h 12    10/22/00 10:26p Jasony $
*/			    
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
#ifndef DACOM_H
#include <DACOM.h>
#endif

struct BaseHotRect;
struct STATIC_DATA;

//--------------------------------------------------------------------------//
//
struct DACOM_NO_VTABLE IStatic : IDAComponent
{
	virtual void InitStatic (const STATIC_DATA & data, BaseHotRect * parent, bool bHighPriority = false) = 0; 

	virtual void SetText (const wchar_t *szText) = 0; 

	virtual void SetTextID (const U32 textID) = 0;

	virtual int  GetText (wchar_t * szBuffer, int bufferSize) = 0;

	virtual void SetVisible (bool bVisible) = 0;

	virtual void SetTextColor (U8 red, U8 green, U8 blue) = 0;

	virtual void SetTextColor (COLORREF color) = 0;

	virtual void EnableRollupBehavior (int value) = 0;	// the control will display text rolling up to the value given

	virtual void SetBuddyControl (struct IButton2 * buddy) = 0;

	virtual const U32  GetStringWidth (void) const = 0; // get the width of the string that is being used by this control
};

#endif