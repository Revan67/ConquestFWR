#ifndef DLIGHT_H
#define DLIGHT_H
//--------------------------------------------------------------------------//
//                                                                          //
//                                DLight.h                                  //
//                                                                          //
//                  COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Author: Rmarr $

    $Header: /Conquest/App/DInclude/DLight.h 5     12/08/99 2:49p Rmarr $
*/			    
//--------------------------------------------------------------------------//
#ifndef DTYPES_H
#include "DTypes.h"
#endif

#ifndef DBASEDATA_H
#include "DBaseData.h"
#endif

struct WORLDLIGHT_DATA
{
	U8 red,green,blue;
	Vector direction;
};

struct AMBIENT_DATA
{
	U8 red,green,blue;
};

struct LIGHT_DATA
{
	WORLDLIGHT_DATA light1;
	WORLDLIGHT_DATA light2;
	AMBIENT_DATA ambient;
};

struct BT_LIGHT : BASIC_DATA
{
	U8 red,green,blue;
	S32 range;
	Vector direction;
	SINGLE cutoff;
	bool bInfinite:1;
	bool bAmbient:1;
};

struct CQLIGHT_SAVELOAD
{
	U8 red,green,blue;
	S32 range;
	Vector position;
	Vector direction;
	SINGLE cutoff;
	bool bInfinite;
	char name[32];
	U32 systemID;
	bool bAmbient;
};

#endif
