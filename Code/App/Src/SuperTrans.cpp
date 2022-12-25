//--------------------------------------------------------------------------//
//                                                                          //
//                              SUPERTRANS.CPP                              //
//                                                                          //
//               COPYRIGHT (C) 1997 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
   $Author: Jasony $

   $Header: /Conquest/App/Src/SuperTrans.cpp 4     7/22/99 11:09a Jasony $

   Conquest's version of the transform class	
*/
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//

#include "pch.h"
#include <globals.h>

#include "SuperTrans.h"


//SINGLE PI_RADIANS = 3.141592654 / 180.0;

typedef SINGLE OASIS[3][3];

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
// switched the rotate_about_i() to accomodate the -k look vector thingy 

//-------------------------------------------------------------------
//
#if 0
void TRANSFORM::rotate_about_i (SINGLE angle)	// pitch
{
	OASIS temp;
	SINGLE cosine, sine;

//	angle *= PI_RADIANS;		// convert to radians
	cosine = cos(angle);
	sine = sin(angle);

	memcpy(temp, d, sizeof(d));
	d[0][1] = (cosine * temp[0][1]) - (sine * temp[0][2]);
	d[1][1] = (cosine * temp[1][1]) - (sine * temp[1][2]);
	d[2][1] = (cosine * temp[2][1]) - (sine * temp[2][2]);

	d[0][2] = (cosine * temp[0][2]) + (sine * temp[0][1]);
	d[1][2] = (cosine * temp[1][2]) + (sine * temp[1][1]);
	d[2][2] = (cosine * temp[2][2]) + (sine * temp[2][1]);
}
#else
//-------------------------------------------------------------------
//
void TRANSFORM::rotate_about_i (SINGLE angle)	// pitch
{
	OASIS temp;
	SINGLE cosine, sine;

	angle = -angle;
	cosine = cos(angle);
	sine = sin(angle);

	memcpy(temp, d, sizeof(d));
	d[0][1] = (cosine * temp[0][1]) - (sine * temp[0][2]);
	d[1][1] = (cosine * temp[1][1]) - (sine * temp[1][2]);
	d[2][1] = (cosine * temp[2][1]) - (sine * temp[2][2]);

	d[0][2] = (cosine * temp[0][2]) + (sine * temp[0][1]);
	d[1][2] = (cosine * temp[1][2]) + (sine * temp[1][1]);
	d[2][2] = (cosine * temp[2][2]) + (sine * temp[2][1]);
}
#endif
//-------------------------------------------------------------------
//
void TRANSFORM::rotate_about_j (SINGLE angle)	// yaw
{
	OASIS temp;
	SINGLE cosine, sine;

//	angle *= PI_RADIANS;		// convert to radians
	cosine = cos(angle);
	sine = sin(angle);
	memcpy(temp, d, sizeof(d));

	d[0][0] = (cosine * temp[0][0]) + (sine * temp[0][2]);
	d[1][0] = (cosine * temp[1][0]) + (sine * temp[1][2]);
	d[2][0] = (cosine * temp[2][0]) + (sine * temp[2][2]);

	d[0][2] = (cosine * temp[0][2]) - (sine * temp[0][0]);
	d[1][2] = (cosine * temp[1][2]) - (sine * temp[1][0]);
	d[2][2] = (cosine * temp[2][2]) - (sine * temp[2][0]);
}
//-------------------------------------------------------------------
//
// switched the rotate_about_k() to accomodate the -k look vector thingy 
#if 0
void TRANSFORM::rotate_about_k (SINGLE angle)	// roll
{
	OASIS temp;
	SINGLE cosine, sine;

//	angle *= PI_RADIANS;		// convert to radians
	cosine = cos(angle);
	sine = sin(angle);

	memcpy(temp, d, sizeof(d));

	d[0][0] = (cosine * temp[0][0]) + (sine * temp[0][1]);
	d[1][0] = (cosine * temp[1][0]) + (sine * temp[1][1]);
	d[2][0] = (cosine * temp[2][0]) + (sine * temp[2][1]);

	d[0][1] = (cosine * temp[0][1]) - (sine * temp[0][0]);
	d[1][1] = (cosine * temp[1][1]) - (sine * temp[1][0]);
	d[2][1] = (cosine * temp[2][1]) - (sine * temp[2][0]);
}
#else
void TRANSFORM::rotate_about_k (SINGLE angle)	// roll
{
	OASIS temp;
	SINGLE cosine, sine;

//	angle *= -PI_RADIANS;		// convert to radians
	angle = -angle;
	cosine = cos(angle);
	sine = sin(angle);

	memcpy(temp, d, sizeof(d));

	d[0][0] = (cosine * temp[0][0]) + (sine * temp[0][1]);
	d[1][0] = (cosine * temp[1][0]) + (sine * temp[1][1]);
	d[2][0] = (cosine * temp[2][0]) + (sine * temp[2][1]);

	d[0][1] = (cosine * temp[0][1]) - (sine * temp[0][0]);
	d[1][1] = (cosine * temp[1][1]) - (sine * temp[1][0]);
	d[2][1] = (cosine * temp[2][1]) - (sine * temp[2][0]);
}
#endif
//-------------------------------------------------------------------
//
void TRANSFORM::rotate_about_x (SINGLE angle)
{
	TRANSFORM temp, result;

//	angle *= PI_RADIANS;		// convert to radians

	temp.d[1][1]  = temp.d[2][2] = cos(angle);
	temp.d[2][1]  = sin(angle);
	temp.d[1][2]  = -temp.d[2][1];
	
	result = multiply(temp);
	*this = result;
}

//-------------------------------------------------------------------
//
SINGLE TRANSFORM::get_roll (void) const
{
	Vector i = -get_i();
	SINGLE x = sqrt(i.x * i.x  + i.y * i.y);
	
	x = get_angle(-i.z,x);

	if (get_k().z < 0)
	{
		if (x < 0)
			x = (-180*MUL_DEG_TO_RAD) - x;
		else
			x = (180*MUL_DEG_TO_RAD) - x;
	}

	return x;
}
//-------------------------------------------------------------------
//
SINGLE TRANSFORM::get_pitch (void) const
{
	Vector j = get_j();
	SINGLE x = sqrt(j.x * j.x  + j.y * j.y);

	x = get_angle(-j.z, x);

// uncomment this to get a range from 0 to +- 180 degrees instead of 0 to +- 90
/*
	if (get_j().z < 0)
	{
		if (x < 0)
			x = (-180*MUL_DEG_TO_RAD) - x;
		else
			x = (180*MUL_DEG_TO_RAD) - x;
	}
*/
	return -x;
}
//-------------------------------------------------------------------
//
SINGLE TRANSFORM::get_yaw (void) const
{
	Vector j = get_j();

// not useful? (jy)
//	k.z = 0;
//	k.normalize();

	return get_angle(-j.x,-j.y);
}
//-------------------------------------------------------------------
//
SINGLE TRANSFORM::get_roll_zbased (void) const
{
	Vector i = get_i();
	SINGLE x = sqrt(i.x * i.x  + i.y * i.y);
	
	x = get_angle(-i.z,x);

	if (get_j().z < 0)
	{
		if (x < 0)
			x = (-180*MUL_DEG_TO_RAD) - x;
		else
			x = (180*MUL_DEG_TO_RAD) - x;
	}

	return x;
}
//-------------------------------------------------------------------
//
SINGLE TRANSFORM::get_pitch_zbased (void) const
{
	Vector k = get_k();
	SINGLE x = sqrt(k.x * k.x  + k.y * k.y);

	x = get_angle(-k.z, x);

// uncomment this to get a range from 0 to +- 180 degrees instead of 0 to +- 90
/*
	if (get_j().z < 0)
	{
		if (x < 0)
			x = (-180*MUL_DEG_TO_RAD) - x;
		else
			x = (180*MUL_DEG_TO_RAD) - x;
	}
*/
	return x;
}
//-------------------------------------------------------------------
//
SINGLE TRANSFORM::get_yaw_zbased (void) const
{
	Vector k = get_k();

// not useful? (jy)
//	k.z = 0;
//	k.normalize();

	return get_angle(-k.x,-k.y);
}
//-------------------------------------------------------------------
//
SINGLE TRANSFORM::get_yaw (const Vector & vec)
{
	return get_angle(vec.x,vec.y);
}
//-------------------------------------------------------------------
//
SINGLE TRANSFORM::get_pitch (const Vector & vec)
{
	SINGLE x = sqrt(vec.x * vec.x  + vec.y * vec.y);

	x = get_angle(vec.z, x);

	return x;
}
//-------------------------------------------------------------------
//
Vector TRANSFORM::rotate_about_z (const Vector & vec, SINGLE angle)
{
	TRANSFORM t;
	const SINGLE cosine = cos(-angle);
	const SINGLE sine = sin(-angle);
	
//	t.rotate_about_k(angle);

	t.d[0][0] = cosine; //(cosine * 1) + (sine * 0);
	t.d[1][0] = sine; // (cosine * 0) + (sine * 1);
	t.d[2][0] = 0; //(cosine * 0) + (sine * 0);

	t.d[0][1] = -sine; // (cosine * 0) - (sine * 1);
	t.d[1][1] = cosine; //(cosine * 1) - (sine * 0);
	t.d[2][1] = 0; // (cosine * 0) - (sine * 0);

	return t.rotate(vec);
}
//-------------------------------------------------------------------
//
SINGLE TRANSFORM::getDistance2D (const Vector &vec, const Vector &vec2)
{
	SINGLE result1 = vec.x - vec2.x;
	SINGLE result2 = vec.y - vec2.y;

	return sqrt((result1*result1) + (result2*result2));
}
//-------------------------------------------------------------------
//
struct DummyInitializer 
{
	TRANSFORM trans;

	DummyInitializer (void)
	{
		trans.rotate_about_i(90 * MUL_DEG_TO_RAD);
	}
};
static DummyInitializer __realdumbSuperTransInit;

const TRANSFORM & TRANSFORM::WORLD = __realdumbSuperTransInit.trans;		// rotate_about_i(90 * MUL_DEG_TO_RAD);


//--------------------------------------------------------------------------//
//-------------------------End SuperTrans.cpp-------------------------------//
//--------------------------------------------------------------------------//
