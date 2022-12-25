#ifndef DBUILDOBJ_H
#define DBUILDOBJ_H
//--------------------------------------------------------------------------//
//                                                                          //
//                              DBuildObj.h                                 //
//                                                                          //
//                  COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*

    $Author: Rmarr $

    $Header: /Conquest/App/DInclude/DBuildObj.h 4     3/15/00 4:56p Rmarr $
*/			    
//--------------------------------------------------------------------------//
#ifndef DBASEDATA_H
#include "DBaseData.h"
#endif

struct BASE_BUILD_OBJ : BASIC_DATA
{
	BUILDOBJCLASS boClass;
};

struct BT_TERRAN_BUILD : BASE_BUILD_OBJ
{
};

struct BT_MANTIS_BUILD : BASE_BUILD_OBJ
{
	char cocoonTextureName[GT_PATH];
};

struct BT_SOLARIAN_BUILD : BASE_BUILD_OBJ
{
	char cocoonTextureName[GT_PATH];
};


#endif
