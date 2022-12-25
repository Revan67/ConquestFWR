#ifndef IMULTIHOTBUTTON_H
#define IMULTIHOTBUTTON_H
//--------------------------------------------------------------------------//
//                                                                          //
//                              IMultiHotButton.h                                //
//                                                                          //
//                  COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/IMultiHotButton.h 2     9/13/00 11:39a Tmauer $
*/			    
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
#ifndef DACOM_H
#include <DACOM.h>
#endif

#ifndef DHOTBUTTON_H
#include <DHotButton.h>
#endif

//--------------------------------------------------------------------------//
//
struct DACOM_NO_VTABLE IMultiHotButton : IDAComponent
{
	virtual void SetState(U32 _baseImage, HBTNTXT::BUTTON_TEXT _buttonText, HBTNTXT::HOTBUTTONINFO _statusTextID, HBTNTXT::MULTIBUTTONINFO _buttonInfo) = 0;

	virtual void NullState() = 0;

	virtual void GetShape(IDrawAgent ** shape) = 0;

	virtual void SetBuildCost(const ResourceCost & cost) = 0;
};


#endif