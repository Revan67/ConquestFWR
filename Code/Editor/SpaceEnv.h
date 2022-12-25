#ifndef IBACKGROUND_H
#define IBACKGROUND_H

//--------------------------------------------------------------------------//
//                                                                          //
//                              SpaceEnv.h                                  //
//                                                                          //
//                  COPYRIGHT (C) 2004 BY DIGITAL ANVIL, INC.               //
//                                                                          //
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