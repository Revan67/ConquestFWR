#ifndef IEDIT2_H
#define IEDIT2_H
//--------------------------------------------------------------------------//
//                                                                          //
//                                IEdit2.h                                  //
//                                                                          //
//                  COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/IEdit2.h 19    19/10/01 11:01 Tmauer $
*/			    
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
#ifndef DACOM_H
#include <DACOM.h>
#endif


struct BaseHotRect;
struct EDIT_DATA;
struct COMBOBOX_DATA;

//--------------------------------------------------------------------------//
//
struct DACOM_NO_VTABLE IEdit2 : IDAComponent
{
	virtual void InitEdit (const EDIT_DATA & data, BaseHotRect * parent) = 0; 

	virtual void EnableEdit (bool bEnable) = 0;

	virtual void SetVisible (bool bVisible) = 0;

	virtual bool GetVisible (void) = 0;

	virtual void SetText (const wchar_t * szText, S32 firstHighlightChar=-1) = 0;

	virtual int  GetText (wchar_t * szBuffer, int bufferSize) = 0;

	virtual void SetControlID (U32 id) = 0;

	virtual U32 GetControlID (void) = 0;

	virtual void SetMaxChars (U32 maxChars) = 0;		// allow user to enter maxChars-1

	virtual void EnableToolbarBehavior (void) = 0;			// edit control in the toolbar

	virtual void EnableChatboxBehavior (void) = 0;

	virtual void EnableLockedTextBehavior (void) = 0;

	virtual void DisableInput (bool bDisableInput) = 0;

	virtual void SetTransparentBehavior (bool _bTransparent) = 0;

	virtual void SetIgnoreChars (wchar_t * ignoreChars) = 0;	// don't allow ignore characters to show up in edit control

	virtual const U32 GetEditWidth (void) const = 0;

	virtual bool IsTextAllVisible (void) = 0;
};

struct DACOM_NO_VTABLE ICombobox : IEdit2
{
	virtual void InitCombobox (const COMBOBOX_DATA & data, BaseHotRect * parent) = 0; 

	virtual void EnableCombobox (bool bEnable) = 0;
};

#endif