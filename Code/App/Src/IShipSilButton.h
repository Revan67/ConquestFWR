#ifndef ISHIPSILBUTTON_H
#define ISHIPSILBUTTON_H
//--------------------------------------------------------------------------//
//                                                                          //
//                              IShipSilButton.h                                //
//                                                                          //
//                  COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/IShipSilButton.h 2     6/22/99 11:16a Tmauer $
*/			    
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
#ifndef DACOM_H
#include <DACOM.h>
#endif


struct BaseHotRect;
struct SHIPSILBUTTON_DATA;
struct IShapeLoader;
struct IBaseObject;


//--------------------------------------------------------------------------//
//
struct DACOM_NO_VTABLE IShipSilButton : IDAComponent
{
	virtual void InitShipSilButton (SHIPSILBUTTON_DATA & data, BaseHotRect * parent, IShapeLoader * loader) = 0; 

	virtual void SetShip(IBaseObject * ship) = 0;

	virtual IBaseObject * GetShip() = 0;

	virtual void SetVisible (bool bVisible) = 0;

	virtual bool GetVisible (void) = 0;

	virtual void SetControlID (U32 id) = 0;

	virtual U32 GetControlID (void) = 0;

};


#endif