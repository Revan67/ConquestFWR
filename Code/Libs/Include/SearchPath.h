#ifndef SEARCHPATH_H
#define SEARCHPATH_H
//--------------------------------------------------------------------------//
//                                                                          //
//                           SearchPath.cpp                                 //
//                                                                          //
//               COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
   $Header: /Conquest/Libs/Include/SearchPath.h 5     4/28/00 11:57p Rmarr $
	
   A component that implements a data file search path .
*/
//--------------------------------------------------------------------------//

#ifndef DACOM_H
#include "DACOM.h"
#endif

//--------------------------------------------------------------------------//
//
struct SEARCHPATHDESC : DACOMDESC
{
	SEARCHPATHDESC (const C8 * _interface_name = "ISearchPath") : DACOMDESC(_interface_name)
	{
	}
};
//--------------------------------------------------------------------------//
//
#define IID_ISearchPath MAKE_IID("ISearchPath",1)

struct DACOM_NO_VTABLE ISearchPath : public IComponentFactory
{
   // *** IDAComponent methods ***

   DEFMETHOD(QueryInterface) (const C8 *interface_name, void **instance) = 0;
   DEFMETHOD_(U32,AddRef)           (void) = 0;
   DEFMETHOD_(U32,Release)          (void) = 0;

   // *** IComponentFactory methods ***

   DEFMETHOD(CreateInstance) (DACOMDESC *descriptor, void **instance) = 0;

   // *** ISearchPath methods ***

   DEFMETHOD(SetPath) (const C8 *path) = 0;

   DEFMETHOD_(U32,GetPath) (C8 *path, U32 bufferSize) const = 0;
};

//----------------------------------------------------------------------------//
//-------------------------------End SearchPath.h-----------------------------//
//----------------------------------------------------------------------------//
#endif