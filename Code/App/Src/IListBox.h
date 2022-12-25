#ifndef ILISTBOX_H
#define ILISTBOX_H
//--------------------------------------------------------------------------//
//                                                                          //
//                              IListbox.h                                   //
//                                                                          //
//                  COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/IListBox.h 15    9/05/00 4:18p Sbarton $
*/			    
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
#ifndef DACOM_H
#include <DACOM.h>
#endif

struct BaseHotRect;
struct LISTBOX_DATA;
struct DROPDOWN_DATA;

//--------------------------------------------------------------------------//
//
struct DACOM_NO_VTABLE IListbox : IDAComponent
{
	virtual void InitListbox (const LISTBOX_DATA & data, BaseHotRect * parent) = 0;
	
	virtual void EnableListbox (bool bEnable) = 0;

	virtual void SetVisible (bool bVisible) = 0;

	virtual S32 AddStringToHead (const wchar_t * szString) = 0;

	virtual S32 AddString (const wchar_t * szString) = 0;	// returns index

	virtual S32 FindString (const wchar_t * szString) = 0;	// returns index of matching string prefix

	virtual S32 FindStringExact (const wchar_t * szString) = 0;	// returns index of matching string

	virtual void RemoveString (S32 index) = 0;	

	virtual U32 GetString (S32 index, wchar_t * buffer, U32 bufferSize) = 0;	// returns length of string (in characters)

	virtual S32 SetString (S32 index, const wchar_t * szString) = 0;	// changes string value, returns index

	virtual void SetDataValue (S32 index, U32 data) = 0;	// set user defined data item

	virtual U32 GetDataValue (S32 index) = 0;	// return user defined value, 0 on error

	virtual void SetColorValue (S32 index, COLORREF color) = 0;		// set color of text for item

	virtual COLORREF GetColorValue (S32 index) = 0;
	
	virtual S32 GetCurrentSelection (void) = 0;		// returns index
		
	virtual S32 SetCurrentSelection (S32 newIndex) = 0;		// returns old selected index

	virtual S32 GetCaretPosition (void) = 0;		// returns index
		
	virtual S32 SetCaretPosition (S32 newIndex) = 0;		// returns old caret index

	virtual void ResetContent (void) = 0;		// remove all items from list

	virtual U32 GetNumberOfItems (void) = 0;

	virtual S32 GetTopVisibleString (void) = 0;
	
	virtual S32 GetBottomVisibleString (void) = 0;

	virtual void EnsureVisible (S32 index) = 0;		// make sure a string is visible

	virtual void ScrollPageUp (void) = 0;

	virtual void ScrollPageDown (void) = 0;

	virtual void ScrollLineUp (void) = 0;

	virtual void ScrollLineDown (void) = 0;

	virtual void ScrollHome (void) = 0;

	virtual void ScrollEnd (void) = 0;

	virtual void CaretPageUp (void) = 0;

	virtual void CaretPageDown (void) = 0;

	virtual void CaretLineUp (void) = 0;

	virtual void CaretLineDown (void) = 0;

	virtual void CaretHome (void) = 0;

	virtual void CaretEnd (void) = 0;

	virtual void SetControlID (U32 id) = 0;

	virtual U32 GetControlID (void) = 0;

	virtual S32 GetBreakIndex(const wchar_t * szString) = 0;  // used for multi-line strings

	virtual const bool IsMouseOver (S16 x, S16 y) const = 0;
};
//--------------------------------------------------------------------------//
//
struct DACOM_NO_VTABLE IDropdown : IListbox
{
	virtual void InitDropdown (const DROPDOWN_DATA & data, BaseHotRect * parent) = 0; 

	virtual void EnableDropdown (bool bEnable) = 0;

	virtual void SetSelectionColor (COLORREF color) = 0;
};

#endif