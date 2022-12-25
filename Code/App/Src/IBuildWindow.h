#ifndef IBUILDWINDOW_H
#define IBUILDWINDOW_H
//--------------------------------------------------------------------------//
//                                                                          //
//                              IBuildWindow.h                                 //
//                                                                          //
//               COPYRIGHT (C) 1997 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
   $Author: Gboswood $

*/
//--------------------------------------------------------------------------//


#ifndef DACOM_H
#include <DACOM.h>
#endif

#include "DStructure.h"


//--------------------------------------------------------------------------//

struct HButton;

struct DACOM_NO_VTABLE IBuildWindow
{
	DEFMETHOD_(void,AddPlatform)		(M_BUILD_CLASS, const M_STRING& name) = 0;
	DEFMETHOD_(void,RemovePlatform)		(M_BUILD_CLASS, const M_STRING& name) = 0;
	DEFMETHOD_(void,SetPlatform)		(M_BUILD_CLASS, const M_STRING& name, BOOL32 enabled, BOOL32 clickable) = 0;
	
	DEFMETHOD_(BOOL32,IsActive)			(M_BUILD_CLASS, const M_STRING& name) = 0;

	DEFMETHOD_(HButton*,AddButton)		(const M_STRING& archetype) = 0;
	DEFMETHOD_(void,EnableButton)		(HButton*) = 0;
	DEFMETHOD_(void,DisableButton)		(HButton*) = 0;
	DEFMETHOD_(void,RemoveButton)		(HButton*) = 0;
	DEFMETHOD_(void,UpdateButton)		(HButton*, U32 buildCount, SINGLE fractionComplete) = 0;
};

#endif

//--------------------------------------------------------------------------//
//-------------------------End IResource.h----------------------------------//
//--------------------------------------------------------------------------//
