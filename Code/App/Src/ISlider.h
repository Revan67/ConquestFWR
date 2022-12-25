#ifndef ISLIDER_H
#define ISLIDER_H
//--------------------------------------------------------------------------//
//                                                                          //
//                              ISlider.h                                   //
//                                                                          //
//                  COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/ISlider.h 3     8/08/00 3:02p Sbarton $
*/			    
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
#ifndef DACOM_H
#include <DACOM.h>
#endif


struct BaseHotRect;
struct SLIDER_DATA;

//--------------------------------------------------------------------------//
//
struct DACOM_NO_VTABLE ISlider : IDAComponent
{
	virtual void InitSlider (const SLIDER_DATA & data, BaseHotRect * parent) = 0; 

	virtual void EnableSlider (bool bEnable) = 0;

	virtual bool GetEnableState (void) = 0;

	virtual void SetVisible (bool bVisible) = 0;

	virtual bool GetVisible (void) = 0;

	virtual void SetControlID (U32 id) = 0;

	virtual U32 GetControlID (void) = 0;

	virtual void GetDimensions (U32 & width, U32 & height) = 0;

	virtual void SetRangeMax (const S32& max) = 0;

	virtual void SetRangeMin (const S32& min) = 0;

	virtual const S32& GetRangeMax (void) = 0;

	virtual const S32& GetRangeMin (void) = 0;

	virtual void SetPosition (const S32& pos) = 0;
	
	virtual const S32& GetPosition (void) = 0;

	virtual void EnableClickBehavior (bool bEnableClick) = 0;	// does the slider act upon a mouse move or an l-button up?
};

#endif