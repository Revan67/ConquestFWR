#ifndef ITOOLBAR_H
#define ITOOLBAR_H
//--------------------------------------------------------------------------//
//                                                                          //
//                              IToolbar.h                                  //
//                                                                          //
//                  COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/IToolbar.h 7     8/10/00 10:16p Rmarr $
*/			    
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
#ifndef DACOM_H
#include <DACOM.h>
#endif

//--------------------------------------------------------------------------//
//
struct DACOM_NO_VTABLE IToolbar : IDAComponent
{
	virtual GENRESULT __stdcall GetControl (const char *buttonName, void ** ppControl) = 0;

	virtual GENRESULT __stdcall GetToolbar (const char *menuName, struct IToolbar ** ppMenu, enum M_RACE race=static_cast<M_RACE>(0)) = 0;

	virtual void SetVisible (bool bVisible) = 0;

	virtual bool GetVisible (void) = 0;

	virtual void SetToolbarID (U32 id) = 0;

	virtual U32 GetToolbarID (void) = 0;

	virtual void GetSystemMapRect (S32 & left, S32 & top, S32 & right, S32 & bottom) = 0;
	
	virtual void GetSectorMapRect (S32 & left, S32 & top, S32 & right, S32 & bottom) = 0;
};

#endif