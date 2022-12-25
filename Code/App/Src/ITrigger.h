#ifndef ITRIGGER_H
#define ITRIGGER_H
//--------------------------------------------------------------------------//
//                                                                          //
//                                ITrigger.h                                  //
//                                                                          //
//                  COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/ITrigger.h 4     11/24/99 11:41a Jasony $
*/			    
//------------------------------- #INCLUDES --------------------------------//
//--------------------------------------------------------------------------//
#ifndef IOBJECT_H
#include "IObject.h"
#endif

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
struct _NO_VTABLE ITrigger : IObject 
{
	virtual void EnableTrigger (bool bEnable) = 0;

	virtual void SetFilter (U32 number, U32 flags, bool bAddTo = false) = 0;

	virtual void SetTriggerRange (SINGLE range) = 0;

	virtual void SetTriggerProgram (const char * progName) = 0;

	virtual IBaseObject * GetLastTriggerObject (void) = 0;
};

//---------------------------------------------------------------------------
//-----------------------END ITrigger.h---------------------------------------
//---------------------------------------------------------------------------
#endif
