#ifndef DSYSMAP_H
#define DSYSMAP_H
//--------------------------------------------------------------------------//
//                                                                          //
//                                DSysMap.h                                 //
//                                                                          //
//                  COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*

    $Author: Jasony $

    $Header: /Conquest/App/DInclude/DSysmap.h 5     8/14/00 8:01a Jasony $
*/			    
//--------------------------------------------------------------------------//

#ifndef DTYPES_H
#include "DTypes.h"
#endif

//--------------------------------------------------------------------------//
//
struct SYSMAP_DATA
{
    bool drawSystemLines:1;
    bool drawThumbnail:1;
	bool drawGrid:1;
};

struct SECTOR_DATA
{
	S32 defaultSizeX,defaultSizeY;
	S32 currentSizeX,currentSizeY;
};

#endif
