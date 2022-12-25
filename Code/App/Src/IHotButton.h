#ifndef IHOTBUTTON_H
#define IHOTBUTTON_H
//--------------------------------------------------------------------------//
//                                                                          //
//                              IHotButton.h                                //
//                                                                          //
//                  COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/IHotButton.h 11    7/24/00 7:58p Sbarton $
*/			    
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
#ifndef DACOM_H
#include <DACOM.h>
#endif


struct BaseHotRect;
struct HOTBUTTON_DATA;
struct BUILDBUTTON_DATA;
struct RESEARCHBUTTON_DATA;
struct MULTIHOTBUTTON_DATA;
struct IShapeLoader;


//--------------------------------------------------------------------------//
//
struct DACOM_NO_VTABLE IHotButton : IDAComponent
{
	virtual void InitHotButton (const HOTBUTTON_DATA & data, BaseHotRect * parent, IShapeLoader * loader) = 0; 

	virtual void InitBuildButton (const BUILDBUTTON_DATA & data, BaseHotRect * parent, IShapeLoader * loader) = 0; 

	virtual void InitResearchButton (const RESEARCHBUTTON_DATA & data, BaseHotRect * parent, IShapeLoader * loader) = 0; 

	virtual void InitMultiHotButton (const MULTIHOTBUTTON_DATA & data, BaseHotRect * parent, IShapeLoader * loader) = 0; 

	virtual void EnableButton (bool bEnable) = 0;

	virtual bool GetEnableState (void) = 0;

	virtual void SetVisible (bool bVisible) = 0;

	virtual bool GetVisible (void) = 0;

	virtual void SetTextID (U32 textID) = 0;	// set resource ID for text  (tooltip)

	virtual void SetTextString (const wchar_t * szString) = 0;	// set text (tooltip)

	virtual void SetPushState (bool bPressed) = 0;		// selected state

	virtual bool GetPushState (void) = 0;

	virtual void SetControlID (U32 id) = 0;

	virtual U32 GetControlID (void) = 0;

	virtual void SetNumericValue (S32 value) = 0;

	virtual S32 GetNumericValue (void) = 0;

	virtual void EnableContextMenuBehavior (void) = 0;		// control within the context menus

	virtual void SetHighlightState (bool bHighlight) = 0;
};

//--------------------------------------------------------------------------//
//
struct DACOM_NO_VTABLE ITabButton : IDAComponent
{
	virtual void InitTabButton (const HOTBUTTON_DATA & data, BaseHotRect * parent, IShapeLoader * loader) = 0; 

	virtual void EnableButton (bool bEnable) = 0;

	virtual bool GetEnableState (void) = 0;

	virtual void SetVisible (bool bVisible) = 0;

	virtual bool GetVisible (void) = 0;

	virtual void SetTextID (U32 textID) = 0;	

	virtual void SetTextString (const wchar_t * szString) = 0;
	
	virtual void SetTextColorNormal (COLORREF color) = 0;

	virtual void SetTextColorHilite (COLORREF color) = 0;

	virtual void SetTextColorSelected (COLORREF color) = 0;

	virtual void SetPushState (bool bPressed) = 0;		// selected state

	virtual bool GetPushState (void) = 0;

	virtual void SetControlID (U32 id) = 0;

	virtual U32 GetControlID (void) = 0;

	virtual void EnableContextMenuBehavior (void) = 0;		// control within the context menus

	virtual void SetHighlightState (bool bHighlight) = 0;

	virtual void SetTabSelected (bool bSelected) = 0;

	virtual void SetDefaultFocusControl (struct IDAComponent * component) = 0;

	virtual void EnableKeyboardFocusing (void) = 0;
};

#endif