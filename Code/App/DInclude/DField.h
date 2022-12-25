#ifndef DFIELD_H
#define DFIELD_H
//--------------------------------------------------------------------------//
//                                                                          //
//                                DField.h                                 //
//                                                                          //
//                  COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Author: Rmarr $

    $Header: /Conquest/App/DInclude/DField.h 47    9/19/00 3:32p Rmarr $
*/			    
//--------------------------------------------------------------------------//

#ifndef DMBASEDATA_H
#include "DMBaseData.h"
#endif

#ifndef DQUICKSAVE_H
#include "DQuickSave.h"
#endif

#define MAX_ASTEROID_TYPES 4

struct XYCoord
{
	S32 x,y;
};

struct FIELD_ATTRIBUTES
{
	SINGLE sensorDamp;
	SINGLE toHitPenalty;
	SINGLE damage;
	SINGLE moveSpeedModifier;
	SINGLE maneuverModifier;
};


namespace FIELDTXT
{
enum INFOHELP
{
	NOTEXT = IDS_HINT_FIELDDEFAULT,
	FIELD_ION = IDS_HINT_FIELD_ION,
	FIELD_PLASMA = IDS_HINT_FIELD_PLASMA,
	FIELD_ASTEROID_SML = IDS_HINT_FIELD_ASTEROID_SML,
	FIELD_ASTEROID_MED = IDS_HINT_FIELD_ASTEROID_MED,
	FIELD_ASTEROID_LG = IDS_HINT_FIELD_ASTEROID_LG,
	FIELD_DEBRIS = IDS_HINT_FIELD_DEBRIS,
	FIELD_ANTIMATTER = IDS_HINT_FIELD_ANTIMATTER,
	FIELD_ANTI_NEBULA = IDS_HINT_FIELD_ANTI_NEBULA,
	FIELD_CELSIUS = IDS_HINT_FIELD_CELSIUS,
	FIELD_HELIOS = IDS_HINT_FIELD_HELIOS,
	FIELD_CYGNUS = IDS_HINT_FIELD_CYGNUS,
	FIELD_HYADES = IDS_HINT_FIELD_HYADES,
	FIELD_LITHIUM = IDS_HINT_FIELD_LITHIUM,
	FIELD_BLACKHOLE = IDS_HINT_FIELD_BLACKHOLE,
	FIELD_ORENUGGET = IDS_HINT_FIELD_ORENUGGET,
	FIELD_GASNUGGET = IDS_HINT_FIELD_GASNUGGET
};
}


struct BASE_FIELD_DATA : BASIC_DATA
{
	FIELDCLASS fieldClass;
	FIELDTXT::INFOHELP   infoHelpID;		// resource id for help text
};

struct U8_RGB
{
	U8 r;
	U8 g;
	U8 b;
};
//----------------------------------------------------------------
//
struct BT_ASTEROIDFIELD_DATA : BASE_FIELD_DATA
{
	char fileName[MAX_ASTEROID_TYPES][GT_PATH];
	char dustTexName[GT_PATH];
	char modTextureName[GT_PATH];
	char mapTexName[GT_PATH];

	char softwareTexClearName[GT_PATH];
	char softwareTexFogName[GT_PATH];

	MISSION_DATA missionData;
	FIELD_ATTRIBUTES attributes;
	S32 asteroidsPerSquare;
	S32 polyroidsPerSquare;
	S32 depth;
	S32 range;
	S32 maxDriftSpeed;
	S32 minDriftSpeed;
	SINGLE stationaryPercentage;
	SFX::ID ambientSFX;
	SINGLE modTexSpeedScale;
	S32 animSizeMin,animSizeMax;
	U32 nuggetsPerSquare;
	SINGLE nuggetZHeight;
	char nuggetType[4][GT_PATH];
	U8_RGB animColor;
};

//----------------------------------------------------------------
//
struct ASTEROIDFIELD_SAVELOAD
{
	// physics data
	TRANS_SAVELOAD trans_SL;

	// mission data
	MISSION_SAVELOAD mission;

	U32 exploredFlags;
};

struct ASTEROIDFIELD_DATA
{
	char name[32];
};

struct ANTIMATTER_DATA
{
	char name[32];
	GRIDVECTOR pts[MAX_SEGS_PLUS_ONE];
	U32 numSegPts;
};

struct BT_ANTIMATTER_DATA : BASE_FIELD_DATA
{
	char textureName[GT_PATH];
	char mapTexName[GT_PATH];
	char softwareTexClearName[GT_PATH];
	char softwareTexFogName[GT_PATH];
	S32 height;
	S32 segment_width;
	S32 spacing;

	MISSION_DATA missionData;
};

struct ANTIMATTER_SAVELOAD
{
	// physics data
	TRANS_SAVELOAD trans_SL;

	// mission data
	MISSION_SAVELOAD mission;
};

#endif
