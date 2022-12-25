#ifndef SYSTEM_H
#define SYSTEM_H
//--------------------------------------------------------------------------//
//                                                                          //
//                              System.h                                    //
//                                                                          //
//               COPYRIGHT (C) 1997 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
$Author: Rmarr $

  $Header: /Conquest/Libs/Include/SYSTEM.H 5     4/28/00 11:57p Rmarr $
*/			    
//---------------------------------------------------------------------------
//
// General type definitions for portability
// 

#ifndef DACOM_H
#include "dacom.h"   // DA component object manager
#endif

//--------------------------------------------------------------------
//--------------------------------------------------------------------
//

#define IID_ISystemComponent MAKE_IID("ISystemComponent", 1)

struct DACOM_NO_VTABLE ISystemComponent : public IAggregateComponent
{
	DEFMETHOD_(void,Update) (void) = 0;

	DEFMETHOD(ShutdownAggregate) (void) {return GR_OK;};
};

//--------------------------------------------------------------------
//--------------------------------------------------------------------

//
// To create an ISystemContainer instance, 
// use AGGDESC with interface_name = "ISystemContainer"
//

#define IID_ISystemContainer MAKE_IID("ISystemContainer", 1)

struct DACOM_NO_VTABLE ISystemContainer : public ISystemComponent
{
	DEFMETHOD(LoadSystemComponents) (void) = 0;
	
	DEFMETHOD(Shutdown) (void) = 0;
	
	DEFMETHOD(AddComponent) (const AGGDESC * descriptor) = 0;
};

#endif
