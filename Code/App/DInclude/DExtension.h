#ifndef DEXTENSION_H
#define DEXTENSION_H
//--------------------------------------------------------------------------//
//                                                                          //
//                               DExtension.h                                //
//                                                                          //
//                  COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Author: Tmauer $

    $Header: /Conquest/App/DInclude/DExtension.h 1     1/12/00 8:09a Tmauer $
*/			    
//--------------------------------------------------------------------------//

#ifndef DBASEDATA_H
#include "DBaseData.h"
#endif

#ifndef DMTECHNODE_H
#include "DMTechNode.h"
#endif

//----------------------------------------------------------------
//---------------------Extension definitions------------------------
//----------------------------------------------------------------
//

#define MAX_EXTENSIONS 4

struct EXTENSION_DATA
{
	char extensionName[GT_PATH];
};

struct BT_EXTENSION_INFO
{
	char addChildName[GT_PATH];
	char removeChildName[GT_PATH];

	char archetypeName[GT_PATH];
};

#endif
