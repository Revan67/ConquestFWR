#ifndef DJUMPGATE_H
#define DJUMPGATE_H
//--------------------------------------------------------------------------//
//                                                                          //
//                               DJumpGate.h                                //
//                                                                          //
//                  COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*

    $Author: Rmarr $

    $Header: /Conquest/App/DInclude/DJumpGate.h 10    7/07/99 6:20p Rmarr $
*/			    
//--------------------------------------------------------------------------//
#ifndef DBASEDATA_H
#include "DBaseData.h"
#endif

#ifndef DINSTANCE_H
#include "DInstance.h"
#endif

#define MAX_BB_MESHES 3

enum BLENDS
{
	ONE_ONE=0,
	SRC_INVSRC=1,
	SRC_ONE=2,
	ONE_INVSRC=3
};

struct BILLBOARD_MESH
{
    char mesh_name[GT_PATH];
	char tex_name[GT_PATH];
	S32 size_min,size_max;
	Vector offset;
	//bool bAdditive;
	BLENDS blendMode;
};
//----------------------------------------------------------------
//
struct BT_JUMPGATE_DATA : BASIC_DATA
{
	BILLBOARD_MESH billboardMesh[MAX_BB_MESHES];
//	char file_anm[GT_PATH];
//	char file_particle[GT_PATH];
	SFX::ID enter1,enter2;
	SFX::ID arrive1,arrive2;
	SFX::ID ambience;
	MISSION_DATA missionData;
	SINGLE min_hold_time;
	SINGLE min_stagger_time;
};

//----------------------------------------------------------------
//

#endif
