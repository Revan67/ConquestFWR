#ifndef DNEBULA_H
#define DNEBULA_H
//--------------------------------------------------------------------------//
//                                                                          //
//                                DNebula.h                                 //
//                                                                          //
//                  COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Author: Rmarr $

    $Header: /Conquest/App/DInclude/DNebula.h 26    8/30/00 8:21p Rmarr $
*/			    
//--------------------------------------------------------------------------//
#ifndef DTYPES_H
#include "DTypes.h"
#endif

#ifndef DFIELD_H
#include "DField.h"
#endif

enum NEBTYPE
{
	NEB_ION,
	NEB_ANTIMATTER,
	NEB_HELIOUS,
	NEB_LITHIUM,
	NEB_HYADES,
	NEB_CELSIUS,
	NEB_CYGNUS
};

/*struct NEBLIGHT_DATA
{
	S8 x,y;
	U32 size;
	U32 speed;
	SINGLE pulse;
	SINGLE life;
	U8 red,green,blue;
};*/

struct NEBULA_DATA
{
	char name[32];
//	NEBLIGHT_DATA light[2];
	U8 amb_r,amb_g,amb_b;
	U8 alpha;
	SINGLE top_layer_alpha_scale;
};

//----------------------------------------------------------------
//
struct BASE_NEBULA_SAVELOAD
{
	NEBULA_DATA neb;
	U32 numSquares, numZones, numNuggets;
	XYCoord squares_xy[MAX_SQUARES];
};
//----------------------------------------------------------------
//
struct NEBULA_SAVELOAD : BASE_NEBULA_SAVELOAD
{	
	// physics data
	TRANS_SAVELOAD trans_SL;

	// mission data
	MISSION_SAVELOAD mission;

	U32 exploredFlags;
};

struct LIGHTNING_DATA
{
	bool bLightning:1;
	bool bFlat:1;
	char lightningTexName[GT_PATH];
	SFX::ID lightningSFX;
	SINGLE frequency;
	U16 size;
	struct LIGHTNING_LIGHT
	{
		U8 r,g,b;
		S32 range;
	} lightning_light;
};

//----------------------------------------------------------------
//
// Restored to retail's schema 2026-08-28. Nebula rendering was rewritten for CQ2 to use a
// particle effect (cloudEffect) in place of retail's texture-based path, dropping ~130 bytes
// of fields. That put every field after them -- including nuggetType -- at the wrong offset,
// so FieldManager::CreateArchetype read garbage archetype names and raised a recoverable
// error at mission load.
struct BT_NEBULA_DATA : BASE_FIELD_DATA
{
	char textureName[GT_PATH];
	char modTextureName[GT_PATH];
	char mapTexName[GT_PATH];
	char softwareTexClearName[GT_PATH];
	char softwareTexFogName[GT_PATH];

	MISSION_DATA missionData;
	FIELD_ATTRIBUTES attributes;

	U8 amb_r,amb_g,amb_b;
	U8 alpha;

	struct AMBIENT_NEBULA_LIGHT
	{
		U8 r,g,b;
		SINGLE pulse_frequency;
	} ambient;

	LIGHTNING_DATA lightning;

	SINGLE bottomSpeedScaleMin,bottomSpeedScaleMax;
	SINGLE topSpeedScaleMin,topSpeedScaleMax;
	SINGLE top_layer_alpha_scale;

	SFX::ID ambientSFX;
	NEBTYPE nebType;

	U32 nuggetsPerSquare;
	SINGLE nuggetZHeight;
	char nuggetType[4][GT_PATH];
};


#endif
