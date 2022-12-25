#ifndef DENGINETRAIL_H
#define DENGINETRAIL_H
//--------------------------------------------------------------------------//
//                                                                          //
//                                 DEngineTrail.h                                //
//                                                                          //
//                  COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $ $
*/			    
//--------------------------------------------------------------------------//

#ifndef DBASEDATA_H
#include "DBaseData.h"
#endif

#ifndef DEFFECT_H
#include "DEffect.h"
#endif

enum ENGINETRAIL_TAPPER_TYPE
{
	EngineTrailTapperZero = 1, //tappers to a zero width
	EngineTrailStraight, //does not tapper
	EngineTrailBleed //spreads out based on tapperMod
};

//--------------------------------------------------------------------------//
//
struct BT_ENGINETRAIL : BASE_EFFECT
{
	SINGLE width;
	U32 segments;
	SINGLE timePerSegment;
	ENGINETRAIL_TAPPER_TYPE tapperType;
	SINGLE tapperMod;
	char hardpoint[HP_PATH];
	char texture[GT_PATH];
	struct _colorMod
	{
		U8 red;
		U8 green;
		U8 blue;
		U8 alpha;
	}colorMod, engineGlowColorMod;
	char engineGlowTexture[GT_PATH];
	SINGLE engineGlowWidth;
};

//--------------------------------------------------------------------------//
//
struct BASE_ENGINETRAIL_SAVELOAD
{
};

struct ENGINETRAIL : BASE_ENGINETRAIL_SAVELOAD
{
};

//--------------------------------------------------------------------------//
//
#endif