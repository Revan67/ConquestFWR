#ifndef DAIRDEFENSE_H
#define DAIRDEFENSE_H
//--------------------------------------------------------------------------//
//                                                                          //
//                              DAirDefense.h                               //
//                                                                          //
//                  COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/DInclude/DAirDefense.h 3     6/19/99 11:29p Tmauer $
*/			    
//--------------------------------------------------------------------------//
//
#ifndef DLAUNCHER_H
#include "DLauncher.h"
#endif

//--------------------------------------------------------------------------//
// can only attack fighters
//--------------------------------------------------------------------------//
//
struct BT_AIR_DEFENSE : BASE_LAUNCHER
{
	char hardpoint[HP_PATH];
	SINGLE baseAccuracy;		// between 0 and 1.0
	char flashTextureName[GT_PATH];
	SINGLE flashWidth;
	SINGLE flashFrequency;
	struct _colorMod
	{
		U8 red,green,blue,alpha;
	} flashColor;
	SFX::ID soundFx;
};

//--------------------------------------------------------------------------//
//
struct AIRDEFENSE_SAVELOAD
{
	S32 refireTime;		// ticks until time to fire
};

#endif
