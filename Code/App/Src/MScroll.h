#ifndef MSCROLL_H
#define MSCROLL_H
//--------------------------------------------------------------------------//
//                                                                          //
//                                MScroll.h                                 //
//                                                                          //
//               COPYRIGHT (C) 1997 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
   $Author: Gboswood $

   $Header: /Conquest/App/Src/MScroll.h 1     9/20/98 1:02p Gboswood $

   scroll the main screen using the mouse
*/
//--------------------------------------------------------------------------//

#ifndef DACOM_H
#include <DACOM.h>
#endif


//--------------------------------------------------------------------------//
//
struct DACOM_NO_VTABLE IMouseScroll : public IDAComponent
{
	DEFMETHOD_(BOOL32,SetArea) (struct tagRECT *inner, struct tagRECT *outer) = 0;

	DEFMETHOD_(BOOL32,SetActive) (BOOL32 activate) = 0;		// returns previous active state
};


//--------------------------------------------------------------------------//
//-----------------------------End MScroll.h--------------------------------//
//--------------------------------------------------------------------------//
#endif