#ifndef IICON_H
#define IICON_H
//--------------------------------------------------------------------------//
//                                                                          //
//                              IIcon.h                                   //
//                                                                          //
//                  COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/IIcon.h 2     3/24/00 3:17p Tmauer $
*/			    
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
#ifndef DACOM_H
#include <DACOM.h>
#endif

struct BaseHotRect;
struct ICON_DATA;
struct IShapeLoader;

//--------------------------------------------------------------------------//
//
struct DACOM_NO_VTABLE IIcon : IDAComponent
{
	virtual void InitIcon (const ICON_DATA & data, BaseHotRect * parent, IShapeLoader * loader) = 0; 

	virtual void SetVisible (bool bVisible) = 0;
};

#endif