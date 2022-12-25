#ifndef INPROGRESSANIM_H
#define INPROGRESSANIM_H
//--------------------------------------------------------------------------//
//                                                                          //
//                             InProgressAnim.h                             //
//                                                                          //
//                  COPYRIGHT (C) 2000 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/InProgressAnim.h 6     9/21/00 3:18p Jasony $
*/			    
//------------------------------- #INCLUDES --------------------------------//
//--------------------------------------------------------------------------//

#ifndef DACOM_H
#include <DACOM.h>
#endif

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
struct IPANIM : IDAComponent
{
	virtual void Start (void) = 0;

	virtual void Stop (void) = 0;

	virtual void SetProgress (SINGLE percent) = 0;		// 0 to 1

	virtual SINGLE GetProgress (void) = 0;

	virtual void UpdateString (U32 dwResID) = 0;
};
//-------------------------------------------------------------------------------
//-----------------------END InProgressAnim.h------------------------------------
//-------------------------------------------------------------------------------
#endif
