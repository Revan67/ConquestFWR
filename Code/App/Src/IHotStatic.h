#ifndef IHOTSTATIC_H
#define IHOTSTATIC_H
//--------------------------------------------------------------------------//
//                                                                          //
//                              IHotStatic.h                               //
//                                                                          //
//                  COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/IHotStatic.h 4     1/18/00 10:02p Tmauer $
*/			    
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
#ifndef DACOM_H
#include <DACOM.h>
#endif

#ifndef DHOTSTATIC_H
#include <DHotStatic.h>
#endif

struct BaseHotRect;
struct HOTSTATIC_DATA;
struct IShapeLoader;


//--------------------------------------------------------------------------//
//
struct DACOM_NO_VTABLE IHotStatic : IDAComponent
{
	virtual void InitHotStatic (const HOTSTATIC_DATA & data, BaseHotRect * parent, IShapeLoader * loader) = 0; 

	virtual U32 GetImageLevel() = 0;

	virtual void SetImageLevel(U32 newImageLevel,U32 newNumLevels) = 0;

	virtual void SetTextString(HSTTXT::STATIC_TEXT text) = 0;

	virtual void SetVisible(bool bVisible) = 0;
};


#endif