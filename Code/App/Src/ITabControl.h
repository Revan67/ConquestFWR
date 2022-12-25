#ifndef ITABCONTROL_H
#define ITABCONTROL_H
//--------------------------------------------------------------------------//
//                                                                          //
//                              ITabControl.h                               //
//                                                                          //
//                  COPYRIGHT (C) 2000 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/ITabControl.h 6     7/03/00 7:06p Sbarton $
*/			    
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
#ifndef DACOM_H
#include <DACOM.h>
#endif

struct BaseHotRect;
struct TABCONTROL_DATA;

//--------------------------------------------------------------------------//
//
struct DACOM_NO_VTABLE ITabControl : IDAComponent
{
	virtual void InitTab (const TABCONTROL_DATA & data, BaseHotRect * parent, struct IShapeLoader * loader) = 0;

	virtual GENRESULT __stdcall GetTabMenu (U32 index, struct BaseHotRect ** ppMenu) = 0;

	virtual void SetControlID (U32 id) = 0;

	virtual U32 GetControlID (void) = 0;

	virtual int GetTabCount (void) = 0;

	virtual int GetCurrentTab (void) = 0;

	virtual int SetCurrentTab (int tabID) = 0;

	virtual void SetDefaultControlForTab (int tabID, IDAComponent * component) = 0;

	virtual void EnableKeyboardFocusing (void) = 0;
};

#endif