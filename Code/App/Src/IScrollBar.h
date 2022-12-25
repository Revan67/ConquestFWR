#ifndef ISCROLLBAR_H
#define ISCROLLBAR_H
//--------------------------------------------------------------------------//
//                                                                          //
//                              IScrollBar.h                                   //
//                                                                          //
//                  COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/IScrollBar.h 5     5/26/00 7:13p Sbarton $
*/			    
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
#ifndef DACOM_H
#include <DACOM.h>
#endif

struct BaseHotRect;
struct SCROLLBAR_DATA;

//--------------------------------------------------------------------------//
//
struct DACOM_NO_VTABLE IScrollBarOwner : IDAComponent
{
	virtual void ScrollPageUp (U32 scrollId) = 0;

	virtual void ScrollPageDown (U32 scrollId) = 0;

	virtual void ScrollLineUp (U32 scrollId) = 0;

	virtual void ScrollLineDown (U32 scrollId) = 0;

	virtual void SetScrollPosition (U32 scrollId,S32 scrollPosition) = 0;
};
//--------------------------------------------------------------------------//
//
struct DACOM_NO_VTABLE IScrollBar : IDAComponent
{
	virtual void InitScrollBar (const SCROLLBAR_DATA & data, BaseHotRect * parent) = 0; 

	virtual void EnableScrollBar (bool bEnable) = 0;

	virtual void SetVisible (bool bVisible) = 0;

	virtual const bool GetVisible (void) const = 0;

	virtual S32 GetScrollPosition (void) = 0;		// returns index
		
	virtual S32 SetScrollPosition (S32 newIndex) = 0;		// returns old thumb index

	virtual void SetScrollRange (S32 bottomIndex) = 0;		// sets the scrollable range from 0
	
	virtual void SetViewRange (S32 viewSize) = 0;		// sets the view range

	virtual void SetControlID (U32 id) = 0;

	virtual U32 GetControlID (void) = 0;

	virtual void GetDimensions (U32 & width, U32 & height) = 0;
};

#endif