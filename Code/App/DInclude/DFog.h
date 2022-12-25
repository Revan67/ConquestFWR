#ifndef DFOG_H
#define DFOG_H
//--------------------------------------------------------------------------//
//                                                                          //
//                                  DFog.h                                  //
//                                                                          //
//                  COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Author: Rmarr $

    $Header: /Conquest/App/DInclude/DFog.h 4     9/10/99 7:29p Rmarr $
*/			    
//--------------------------------------------------------------------------//

#ifndef DTYPES_H
#include "DTypes.h"
#endif

#define FOGDATA_VERSION 2

#ifndef _ADB 
#define __hexview
#define __readonly
#endif

#define RGBA_DEFINED 1
struct RGBA
{
	U8 r;
	U8 g;
	U8 b;
	U8 a;
};

struct FOG_DATA
{
	__readonly U32 version;
//	S32 resolution;

	RGBA hardFog;
	RGBA softFog;
	RGBA mapHardFog;
	RGBA mapSoftFog;
};

#endif
