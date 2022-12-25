#ifndef DARTIFACTS_H
#define DARTIFACTS_H
//--------------------------------------------------------------------------//
//                                                                          //
//                              DArtifacts.h                                //
//                                                                          //
//                  COPYRIGHT (C) 2004 by WARTHOG, INC.                     //
//                                                                          //
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//

#ifndef DWEAPON_H
#include "DWeapon.h"
#endif

#ifndef DHOTBUTTONTEXT_H
#include "DHotButtonText.h"
#endif

struct ArtifactButtonInfo
{
	U32 baseButton;		// index into shape table
	HBTNTXT::BUTTON_TEXT tooltip;
	HBTNTXT::HOTBUTTONINFO helpBox;//actualy the status text
	HBTNTXT::MULTIBUTTONINFO hintBox;//this is the hint box
};
//--------------------------------------------------------------------------//
//
struct BT_BUFF_ARTIFACT : BASE_WEAPON_DATA
{
	char shipName[GT_PATH];
	enum BuffType
	{
		B_BLOCKADE_INHIBITOR,
	}buffType;
	ArtifactButtonInfo buttonInfo;
};
//--------------------------------------------------------------------------//
//
struct BT_LISTEN_ARTIFACT : BASE_WEAPON_DATA
{
	char shipName[GT_PATH];
	SINGLE successChance;
	ArtifactButtonInfo buttonInfo;
};
//--------------------------------------------------------------------------//
//
struct BT_TERRAFORM_ARTIFACT : BASE_WEAPON_DATA
{
	char shipName[GT_PATH];
	ArtifactButtonInfo buttonInfo;
	char targetPlanetType[GT_PATH];
	SINGLE changeTime;
};

//--------------------------------------------------------------------------//
//
#endif