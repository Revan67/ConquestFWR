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

#define MAX_EXTENSIONS 4	// retail schema: EXTENSION_DATA extension[4].
							// Previously 5, "confirmed" by sizeof(BASE_PLATFORM_DATA)==560 --
							// but that only balanced because MISSION_DATA_BIN was 32 bytes
							// short in the same struct. Both are fixed together.

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
