#ifndef DARTIFACTLAUNCHER_H
#define DARTIFACTLAUNCHER_H
//--------------------------------------------------------------------------//
//                                                                          //
//                          DArtifactLauncher.h                             //
//                                                                          //
//                  COPYRIGHT (C) 2004 by Warthog, INC.                     //
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

//--------------------------------------------------------------------------//
// 
//
struct BT_ARTIFACT_LAUNCHER : BASE_LAUNCHER
{
	char artifactName[GT_PATH];
};

//--------------------------------------------------------------------------//
//
struct ARTIFACT_LAUNCHER_SAVELOAD
{
	U32 targetID;
};

//--------------------------------------------------------------------------//
//
#endif