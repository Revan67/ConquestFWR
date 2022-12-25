#ifndef DEXPLOSION_H
#define DEXPLOSION_H
//--------------------------------------------------------------------------//
//                                                                          //
//                             DExplosion.h                                 //
//                                                                          //
//                  COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*

    $Author: Rmarr $

    $Header: /Conquest/App/DInclude/DExplosion.h 7     12/14/99 3:14p Rmarr $
*/			    
//--------------------------------------------------------------------------//
#ifndef DBASEDATA_H
#include "DBaseData.h"
#endif

struct BT_UI_ANIM : BASIC_DATA
{
	char effectType[GT_PATH];
	SINGLE totalTime;
	SFX::ID sfx;
};

struct BT_BLAST : BASIC_DATA
{
   // char fileName[GT_PATH];
//	char animName[GT_PATH];
//	SINGLE animScale;
	char effectType[3][GT_PATH];
	FLASH_DATA flash;
    SINGLE totalTime;
	SFX::ID sfx;
	SINGLE leadTime;
	bool bDrawThroughFog;
};

//---------------------------------------------
//
struct BT_MESH_EXPLOSION : BASIC_DATA
{
    char secondaryBlastType[GT_PATH];
    char pieceBlastType[GT_PATH];
    char catastrophicBlastType[GT_PATH];
	char rippleBlastType[GT_PATH];
	char fireTrail[GT_PATH];
};


#endif
