#ifndef DCAMERA_H
#define DCAMERA_H
//--------------------------------------------------------------------------//
//                                                                          //
//                                DCamera.h                                 //
//                                                                          //
//                  COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*

    $Author: Jasony $

    $Header: /Conquest/App/DInclude/DCamera.h 7     12/06/99 1:56p Jasony $
*/			    
//--------------------------------------------------------------------------//

#ifndef DTYPES_H
#include "DTypes.h"
#endif

#define CAMERA_DATA_VERSION 5

#ifndef _ADB
#ifndef __readonly
#define __readonly
#endif
#endif
//--------------------------------------------------------------------------//
//
struct CAMERA_DATA
{
	__readonly U32 version;

	SINGLE worldRotation;
	__readonly Vector lookAt;

	SINGLE FOV_x, FOV_y;
	Vector position;
	SINGLE pitch;
	SINGLE minZ, maxZ;

	U32 zoomRate;		// world units per second
	U32 rotateRate;		// degrees per second
	SINGLE toggleZoomZ;	// camera Z when toggle took place
};

#endif
