#ifndef DSECTOR_H
#define DSECTOR_H
//--------------------------------------------------------------------------//
//                                                                          //
//                               DSector.h                                  //
//                                                                          //
//                  COPYRIGHT (C) 2000 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/DInclude/DSector.h 9     6/22/00 4:13p Jasony $
*/			    
//--------------------------------------------------------------------------//

#ifndef DBASEDATA_H
#include "DBaseData.h"
#endif

//----------------------------------------------------------------------------
//
#ifndef DCAMERA_H
#include "DCamera.h"
#endif

#define MAX_PLAYERS_PLUS_1 9
#define MAX_SYSTEMS 16
#define MAX_SYS_SIZE GRIDSIZE*64//0x1FFFF

#define MAX_SYSTEM_LIGHTS 10

struct SYSTEM_VIEW
{
	char systemKitName[GT_PATH];
	char backgroundName[GT_PATH];
};

struct GT_SYSTEM_KIT
{
	char fileName[GT_PATH];
	U32 numLights;
	struct _lightInfo
	{
		U8 red,green,blue;
		S32 range;
		Vector position;// 0.0 is bottom left, 1.0 top right.  most should be outside
		Vector direction;
		SINGLE cutoff;
		bool bInfinite;
		char name[GT_PATH];
		bool bAmbient;
	}lightInfo[MAX_SYSTEM_LIGHTS];
};

struct SYSTEM_DATA
{
	U32 id;
	char name[16];
	char systemKitName[GT_PATH];
	char backgroundName[GT_PATH];
	S32 x,y;
	S32 sizeX,sizeY;
	U32 alertState[MAX_PLAYERS_PLUS_1];
	CAMERA_DATA cameraBuffer;
	U32 inSupply;
	U32 inRootSupply;
};

struct MT_SECTOR_SAVELOAD
{
	struct SYSTEM_DATA sysData[MAX_SYSTEMS];
	U32 currentSystem;
};
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//
#endif
