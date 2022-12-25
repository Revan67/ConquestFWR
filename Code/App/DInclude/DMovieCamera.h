#ifndef DMOVIECAMERA_H
#define DMOVIECAMERA_H
//--------------------------------------------------------------------------//
//                                                                          //
//                                DMovieCamera.h                                 //
//                                                                          //
//                  COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Author: Tmauer $

    $Header: /Conquest/App/DInclude/DMovieCamera.h 2     11/15/99 1:02p Tmauer $
*/			    
//--------------------------------------------------------------------------//
#ifndef DTYPES_H
#include "DTypes.h"
#endif

#ifndef DMBASEDATA_H
#include "DMBaseData.h"
#endif


//--------------------------------------------------------------------------//
//
struct BASE_MOVIE_CAMERA_SAVELOAD : MISSION_SAVELOAD
{
	Vector cam_lookAt;
	Vector cam_position;
	SINGLE cam_FOV_x;
	SINGLE cam_FOV_y;
};
//--------------------------------------------------------------------------//
//
struct MOVIE_CAMERA_SAVELOAD
{
	// physics data
	TRANS_SAVELOAD trans_SL;

	BASE_MOVIE_CAMERA_SAVELOAD baseSave;

};
//--------------------------------------------------------------------------//
//
struct BT_MOVIE_CAMERA_DATA : BASIC_DATA
{
	char fileName[GT_PATH];
	MISSION_DATA missionData;
};

struct MOVIE_CAMERA_VIEW
{
	Vector cam_lookAt;
	Vector cam_position;
	SINGLE cam_FOV_x;
	SINGLE cam_FOV_y;
	M_STRING partName;
};
#endif
