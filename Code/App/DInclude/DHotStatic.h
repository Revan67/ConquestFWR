#ifndef DHOTSTATIC_H
#define DHOTSTATIC_H
//--------------------------------------------------------------------------//
//                                                                          //
//                              DHotStatic.h     							//
//                                                                          //
//                  COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/DInclude/DHotStatic.h 9     9/18/00 11:09a Tmauer $
*/			    
//--------------------------------------------------------------------------//

#ifndef DBASEDATA_H
#include "DBaseData.h"
#endif

#ifndef GT_PATH
#define GT_PATH 32
#endif

namespace HSTTXT
{
enum STATIC_TEXT
{
	NOTEXT = 0,
	ENGINE = IDS_IND_ENGINES_UP,
	ARMOR = IDS_IND_ARMOR_UP,
	WEAPON = IDS_IND_WEAPONS_UP,
	SHIELD = IDS_IND_SHIELDS_UP,
	SENSOR = IDS_IND_SENSOR_UP,
	SUPPLY = IDS_IND_SUPPLY_UP,
	TROOPSHIP = IDS_IND_TROOPSHIP_UP,
	HARVESTER = IDS_IND_HARVESTER_UP,
	TENDER = IDS_IND_TENDER_UP,
	FIGHTER = IDS_IND_FIGHTER_UP,
	FLEET = IDS_IND_FLEET_UP,
	MISSLEPAK = IDS_IND_MISSILEPAK_UP,
	RAM = IDS_IND_RAM_UP,
	LEECH = IDS_IND_LEECH_UP,
	LEGIONAIRE = IDS_IND_LEGIONAIRE_UP,
	SIPHON = IDS_IND_SIPHON_UP,
	GALIOT = IDS_IND_GALIOT_UP,
};
} // end namespace

#define HOTSTATIC_TYPE   "HotStatic!!Default"
#define DEFAULT_TECH_LEVELS 4
#define MAX_TECH_LEVELS 6		
//---------------------------------------------------------------------------
//
struct GT_HOTSTATIC : GENBASE_DATA
{
	char fontType[GT_PATH];
};
//---------------------------------------------------------------------------
//
struct HOTSTATIC_DATA
{
	U32 baseImage;
	U32 numTechLevels;			// defaults to DEFAULT_TECH_LEVELS if this is 0
	S32 xOrigin, yOrigin, width, height;
	S32 barStartX;
	U32 barSpacing;
	HSTTXT::STATIC_TEXT text;
	struct _color
	{
		U8 red,green,blue;
	}textColor;
};


#endif
