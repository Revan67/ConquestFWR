#ifndef ISTRUCTENUMERATOR_H
#define ISTRUCTENUMERATOR_H
//--------------------------------------------------------------------------//
//                                                                          //
//                             IStructEnumerator.h                          //
//                                                                          //
//                  COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Libs/Src/Tools/adb/IStructEnumerator.h 1     11/25/02 2:01p Tmauer $
*/			    
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
#ifndef DACOM_H
#include "DACOM.H"
#endif

struct IViewer;
#define IID_IStructEnumerator MAKE_IID("IStructEnumerator",1)
//--------------------------------------------------------------------------//
//
struct DACOM_NO_VTABLE IStructEnumerator : public IDAComponent
{
	DEFMETHOD_(BOOL32,BeginEnumStruct) (IViewer *viewer, const char * structName, const char *instanceName) = 0;

	DEFMETHOD_(BOOL32,EndEnumStruct) (IViewer *viewer) = 0;

	DEFMETHOD_(BOOL32,BeginEnumArray) (IViewer *viewer, const char * structName, const char *instanceName, int size) = 0;

	DEFMETHOD_(BOOL32,EndEnumArray) (IViewer *viewer) = 0;

	DEFMETHOD_(BOOL32,EnumMember) (IViewer *viewer, const char * typeName, const char * instanceName, const char * value) = 0;
};

//------------------------------------------------------------------------------//
//----------------------------End IStructViewer.h-------------------------------//
//------------------------------------------------------------------------------//

#endif