#ifndef DPLATFORM_H
#define DPLATFORM_H
//--------------------------------------------------------------------------//
//                                                                          //
//                                DPlatform.h                                 //
//                                                                          //
//                  COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*

    $Author: Rmarr $

    $Header: /Conquest/App/DInclude/DPlatform.h 53    10/08/00 9:45p Rmarr $
*/			    
//--------------------------------------------------------------------------//
#ifndef DBASEDATA_H
#include "DBaseData.h"
#endif

#ifndef DTYPES_H
#include "DTypes.h"
#endif

#ifndef DINSTANCE_H
#include "DInstance.h"
#endif

#ifndef DFABRICATOR_H
#include "DFabricator.h"		// for DRONE_RELEASE
#endif

#ifndef DMTECHNODE_H
#include "DMTechNode.h"
#endif

#ifndef DEXTENSION_H
#include "DExtension.h"
#endif

#define NUM_BLINKIES 6

//----------------------------------------------------------------
//
struct BASE_PLATFORM_DATA : BASIC_DATA
{
	PLATFORMCLASS type;
    char fileName[GT_PATH];
	MISSION_DATA missionData;
	EXTENSION_DATA extension[MAX_EXTENSIONS];
	U8 extensionBits;
	S8 extensionLevel;
	char explosionType[GT_PATH];
	char shieldHitType[GT_PATH];
	char ambient_animation[GT_PATH];
	char ambientEffect[GT_PATH];
	SINGLE mass;
	SINGLE_TECHNODE techActive;
	struct SHIELD_DATA shield;
	U32 slotsNeeded;
	struct BLINKER_DATA blinkers;
	U8 commandPoints;
	U32 metalStorage;
	U32 gasStorage;
	U32 crewStorage;
	U8  size;
	bool bMoonPlatform:1;
};
//----------------------------------------------------------------
//
#ifndef _ADB
struct IEffectHandle;

template <class BT_TYPE> 
struct PLATFORM_INIT
{
	const BT_TYPE * pData;
 	PARCHETYPE pArchetype;
 	S32 animArchetype, archIndex, smoke_archID;
	IMeshArchetype * meshArch;

	IEffectHandle * ambientEffect;
		
 	PARCHETYPE pExplosionType;
 	PARCHETYPE pShieldHitType;
	PARCHETYPE pDamageBlast, pSparkBlast;
	PARCHETYPE pBuildEffect;
	PARCHETYPE pExtBuildEffect;

	struct AnimArchetype * damageAnimArch;
	struct AnimArchetype * shieldAnimArch, *shieldFizzAnimArch;
	struct SMesh *smesh;
	struct BaseExtent *m_extent;
	struct EXTENT_DATA extent;

	struct BlinkersArchetype *blink_arch;
	U32 blinkTex,damageTexID, controlTexID;
	//render experiment
	int numChildren;
	struct IMeshInfoTree *mesh_info;
	struct IMeshRender **mr;
	SINGLE fp_radius;  //this sucks

	////////////////////

	void * operator new (size_t size)
	{
		return calloc(size,1);
	}
	void   operator delete (void *ptr)
	{
		::free(ptr);
	}

	PLATFORM_INIT (void)
	{
		meshArch = NULL;
		smoke_archID = 
		animArchetype = archIndex = -1;
		ambientEffect = NULL;
	}

	/* methods defined in TPlatform.h */

	bool loadPlatformArchetype (BT_TYPE * _pData, PARCHETYPE _pArchetype);		// load archetype data (return true on success)

	~PLATFORM_INIT (void);					// free archetype references
};

#endif		// end #ifndef _ADB
//----------------------------------------------------------------
//

#endif
