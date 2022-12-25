#ifndef DTRACTORWAVELAUNCHER_H
#define DTRACTORWAVELAUNCHER_H
//--------------------------------------------------------------------------//
//                                                                          //
//                              DTractorWaveLauncher.h                      //
//                                                                          //
//                  COPYRIGHT (C) 2004 Warthog, INC.                        //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header:
*/			    
//--------------------------------------------------------------------------//

#ifndef DLAUNCHER_H
#include "DLauncher.h"
#endif

#ifndef DMTECHNODE_H
#include <DMTechNode.h>
#endif

#ifndef DWEAPON_H
#include "DWeapon.h"
#endif

#define MAX_TRACTOR_WAVE_TARGETS 12

//--------------------------------------------------------------------------//
// 
//
struct BT_TRACTOR_WAVE_LAUNCHER : BASE_LAUNCHER
{
	SINGLE durration;
	char spaceWaveType[GT_PATH];
	SINGLE waveFrequency;
	SINGLE waveLifeTime;
};

//--------------------------------------------------------------------------//
//
struct TRACTOR_WAVE_LAUNCHER_SAVELOAD
{
	bool bActive;
	GRIDVECTOR tractorPos;
	SINGLE timer;

	U32 targets[MAX_TRACTOR_WAVE_TARGETS];
	U32 numTargets;

	//this needs to be saved because data in the ships is set uppon removeal frem these lists
	U32 syncRemoveList[MAX_TRACTOR_WAVE_TARGETS];
	U32 numSyncRemove;
};

//--------------------------------------------------------------------------//
//
#endif