#ifndef DEFFECT_H
#define DEFFECT_H
//--------------------------------------------------------------------------//
//                                                                          //
//                             DExplosion.h                                 //
//                                                                          //
//                  COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*

    $Author: Tmauer $

    $Header: /Conquest/App/DInclude/DEffect.h 9     7/14/00 3:50p Tmauer $
*/			    
//--------------------------------------------------------------------------//
#ifndef DBASEDATA_H
#include "DBaseData.h"
#endif

struct BASE_EFFECT : BASIC_DATA
{
	EFFECTCLASS fxClass;
};
//---------------------------------------------
//
struct BT_CLOAKEFFECT_DATA : BASE_EFFECT
{
	char animName[GT_PATH];
	char objectName[GT_PATH];
	SFX ::ID cloakOn;
	SFX ::ID cloakOff;
};
//---------------------------------------------
//
struct BT_FIREBALL_DATA : BASE_EFFECT
{
	char animName[GT_PATH];
};
//---------------------------------------------
//
struct BT_PARTICLE_DATA : BASE_EFFECT
{
	char fileName[GT_PATH];
};
//---------------------------------------------
//
struct BT_STREAK_DATA : BASE_EFFECT
{
	char animName[GT_PATH];
	U8 numLines;
	SINGLE lineTime;
};
//---------------------------------------------
//
struct BT_WORMHOLE_EFFECT : BASE_EFFECT
{
};
//---------------------------------------------
//
struct BT_PARTICLE_CIRCLE : BASE_EFFECT
{
	struct Color
	{
		U8 red;
		U8 green;
		U8 blue;
	}color;
};
//---------------------------------------------
//
struct BT_NOVA_EXPLOSION : BASE_EFFECT
{
	SINGLE interRingTime;
	SINGLE ringTime;
	SINGLE range;
	SINGLE duration;
};
//---------------------------------------------
//
struct BT_TALORIAN_EFFECT : BASE_EFFECT
{
};
//---------------------------------------------
//
struct BT_ANIMOBJ_DATA : BASE_EFFECT
{
	char animName[GT_PATH];
	SINGLE animSize;    //actual world units at start
	SINGLE sizeVel;		//change in size per/sec
	SINGLE lifeTime;
	bool fadeOut:1;
	bool faceFront:1;
	bool bSmooth:1;
	bool bLooping:1;
	bool bHasAlphaChannel:1;
};
#endif
