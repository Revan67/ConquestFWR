#ifndef IBUTTON2_H
#define IBUTTON2_H
//--------------------------------------------------------------------------//
//                                                                          //
//                              IButton2.h                                  //
//                                                                          //
//                  COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/IButton2.h 19    9/06/00 7:21p Sbarton $
*/			    
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
#ifndef DACOM_H
#include <DACOM.h>
#endif




struct BaseHotRect;
struct BUTTON_DATA;

//--------------------------------------------------------------------------//
//
struct DACOM_NO_VTABLE IButton2 : IDAComponent
{
	virtual void InitButton (const BUTTON_DATA & data, BaseHotRect * parent) = 0; 

	virtual void EnableButton (bool bEnable) = 0;

	virtual bool GetEnableState (void) = 0;

	virtual void SetVisible (bool bVisible) = 0;

	virtual bool GetVisible (void) = 0;

	virtual void SetTextID (U32 textID) = 0;	// set resource ID for text

	virtual void SetTextString (const wchar_t * szString) = 0;	// set text

	virtual int  GetText (wchar_t * szBuffer, int bufferSize) = 0;

	virtual void SetPushState (bool bPressed) = 0;

	virtual bool GetPushState (void) = 0;

	virtual void SetControlID (U32 id) = 0;

	virtual U32 GetControlID (void) = 0;

	virtual void GetDimensions (U32 & width, U32 & height) = 0;

	virtual void GetPosition (U32 &xpos, U32 &ypos) = 0;

	virtual const bool IsMouseOver (S16 x, S16 y) const = 0;

	virtual const bool IsButtonDown (void) const = 0;

	virtual void SetDropdownBehavior () = 0;

	virtual void SetTransparent (bool bTransparent) = 0;

	virtual void SetDefaultColor (COLORREF color) = 0;

	virtual void ForceAlertState (bool bAlert) = 0;

	virtual void ForceMouseDownState (bool bMouseDown) = 0;
};

#endif