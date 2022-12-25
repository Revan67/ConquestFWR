#ifndef DGROUP_H
#define DGROUP_H
//--------------------------------------------------------------------------//
//                                                                          //
//                                DGroup.h                                  //
//                                                                          //
//                  COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/DInclude/DGroup.h 3     8/25/99 10:09a Jasony $
*/			    
//--------------------------------------------------------------------------//
#ifndef DBASEDATA_H
#include "DBaseData.h"
#endif

//----------------------------------------------------------------
//
typedef BASIC_DATA BT_GROUP;	// no extra data items yet

//----------------------------------------------------------------
//
struct GROUPOBJ_SAVELOAD
{
	U32 dwMissionID;
	U32 numObjects;
};

//----------------------------------------------------------------
//

#endif
