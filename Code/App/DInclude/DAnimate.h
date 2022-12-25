#ifndef DANIMATE_H
#define DANIMATE_H
//--------------------------------------------------------------------------//
//                                                                          //
//                               DAnimate.h                                 //
//                                                                          //
//                  COPYRIGHT (C) 2000 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/DInclude/DAnimate.h 4     5/10/00 5:11p Sbarton $
*/			    
//--------------------------------------------------------------------------//

#ifndef DBASEDATA_H
#include "DBaseData.h"
#endif

#ifndef GT_PATH
#define GT_PATH 32
#endif

#define GTASHP_MAX_SHAPES 64

//--------------------------------------------------------------------------//
//  Definition file for static controls
//--------------------------------------------------------------------------//

struct GT_ANIMATE : GENBASE_DATA
{
    char vfxType[GT_PATH];                        // vfx shape file
};
//---------------------------------------------------------------------------
//
struct ANIMATE_DATA
{
    char animateType[GT_PATH];
    S32 xOrigin, yOrigin;
	U32 dwTimer;		// number of milliseconds used to complete animation
	bool bFuzzEffect;
};

#endif