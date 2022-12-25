#ifndef MENU_H
#define MENU_H
//--------------------------------------------------------------------------//
//                                                                          //
//                                 Menu.h                                   //
//                                                                          //
//               COPYRIGHT (C) 1997 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
   $Author: Jasony $

		                 Resource manager for the menu

   $Header: /Conquest/App/Src/Menu.h 10    5/25/00 12:46p Jasony $
*/
//--------------------------------------------------------------------------//

#ifndef IRESOURCE_H
#include "IResource.h"
#endif


//
// position values for menu headings
//
#define MENUPOS_FILE	0x00000000
#define MENUPOS_PREF	0x00000001
#define MENUPOS_VIEW	0x00000002
#define MENUPOS_EDITOR	0x00000003

//
// position values for file sub-menus (btw, separators DO count in menu position totals)
//
#define MENUPOS_MRU_MISSION    10



struct DACOM_NO_VTABLE IMenuResource : public IDAComponent
{
	virtual void __cdecl Redraw (void) = 0;

	virtual HMENU __stdcall GetSubMenu (U32 nPos) = 0;

	virtual void __stdcall ForceDraw (void) = 0;

	virtual BOOL32 __stdcall EnableMenuItem (U32 nPos, U32 flags) = 0;

	virtual void __stdcall InitPreferences (void) = 0;		// reset the menu state

	virtual HMENU __stdcall GetMainMenu (void) const = 0;

	virtual void __stdcall SetMainMenu (HMENU hMenu) = 0;

	virtual void __stdcall AddToPartMenu (const char *szPartName, U32 identifier) = 0;

	virtual void __stdcall RemoveFromPartMenu (U32 identifier) = 0;

	virtual void __stdcall RemoveAllFromPartMenu (void) = 0;
};
//--------------------------------------------------------------------------//
//--------------------------------End Menu.h--------------------------------//
//--------------------------------------------------------------------------//
#endif