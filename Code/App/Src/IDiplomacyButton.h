#ifndef IDIPLOMACYBUTTON_H
#define IDIPLOMACYBUTTON_H
//--------------------------------------------------------------------------//
//                                                                          //
//                              IDiplomacyButton.h                          //
//                                                                          //
//                  COPYRIGHT (C) 2000 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/IDiplomacyButton.h 2     9/08/00 7:15p Sbarton $
*/			    
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
#ifndef DACOM_H
#include <DACOM.h>
#endif

struct BaseHotRect;
struct DIPLOMACYBUTTON_DATA;

//--------------------------------------------------------------------------//
//
struct DACOM_NO_VTABLE IDiplomacyButton : IDAComponent
{
	virtual void InitDiplomacyButton (const DIPLOMACYBUTTON_DATA & data, BaseHotRect * parent) = 0; 

	virtual void EnableDiplomacyButton (bool bEnable) = 0;

	virtual const bool GetEnableState (void) const = 0;

	virtual void SetVisible (bool bVisible) = 0;

	virtual const bool GetVisible (void) const = 0;

	virtual void SetControlID (U32 id) = 0;

	virtual void SetFirstState (const bool bState) = 0;

	virtual void SetSecondState (const bool bState) = 0;

	virtual const bool GetFirstState (void) const = 0;

	virtual const bool GetSecondState (void) const = 0;
};

#endif