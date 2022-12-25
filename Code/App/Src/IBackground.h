#ifndef IBACKGROUND_H
#define IBACKGROUND_H
//--------------------------------------------------------------------------//
//                                                                          //
//                              IBackground.h                                //
//                                                                          //
//                  COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/IBackground.h 3     6/22/00 4:13p Jasony $
*/			    
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
#ifndef DACOM_H
#include <DACOM.h>
#endif

//--------------------------------------------------------------------------//
//
struct DACOM_NO_VTABLE IBackground : IDAComponent
{
	virtual void LoadBackground(char * filename,U32 systemID) = 0;

	virtual void __stdcall RenderNeb (void) = 0;
};


#endif