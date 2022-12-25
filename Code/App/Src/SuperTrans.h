#ifndef SUPERTRANS_H
#define SUPERTRANS_H
//--------------------------------------------------------------------------//
//                                                                          //
//                              SUPERTRANS.H                                //
//                                                                          //
//               COPYRIGHT (C) 1997 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
   $Author: Jasony $

   $Header: /Conquest/App/Src/SuperTrans.h 7     10/30/00 9:12a Jasony $

   Conquest's version of the transform class	
*/
//--------------------------------------------------------------------------//
/*
	SINGLE get_roll (void) const;
		Returns: (these numbers are degrees, but function actually returns in radians)
			0     = level flight
			-90   = rolled to the left
			+90   = rolled to the right
			+-180 = upside down
	
	SINGLE get_pitch (void) const;
		Returns: (these numbers are degrees, but function actually returns in radians)
			0   = level flight
			> 0 = pointed up
			< 0 = pointed down
			90	= pointed straight up (along the +z axis)
			-90 = pointed straight down (along the -z axis)


	SINGLE get_yaw (void) const;
		Returns: (these numbers are degrees, but function actually returns in radians)
			0     = pointed down the +y axis
			+-180 = pointed down the -y axis

*/
//-------------------------------------------------------------------
//-------------------------------------------------------------------

#ifndef _3DMATH_H
#include <3DMath.h>
#endif

#ifndef MUL_RAD_TO_DEG
  #define MUL_RAD_TO_DEG (180.0 / PI)
#endif


class TRANSFORM : public Transform
{
//	Vector i, j, k;


public:

	TRANSFORM (void) 
	{ 
	}

	TRANSFORM (const Transform & transform)	: Transform(transform)
	{
	}

	TRANSFORM::TRANSFORM (const Matrix & matrix, const Vector & position) : Transform(matrix)
	{
		translation = position;
	}

	TRANSFORM (const Vector & i, const Vector & j, const Vector & k, const Vector & pos) : Transform (i, j, k)
	{ 
		translation = pos;
	}

	// enable construction of a Transform without initialization
	explicit TRANSFORM (bool idiot) : Transform (idiot)
	{
	}
	
	void set_identity (void)
	{
		// matrix::set_identity()
		d[0][0] = d[1][1] = d[2][2] = 1.0f;

		d[0][1] = d[0][2] =
		d[1][0] = d[1][2] =
		d[2][0] = d[2][1] = 0.0f;

//		translation.zero();
		translation.x = translation.y = translation.z = 0.0f;
	}

	void rotate_about_i (SINGLE angle);		// rotate around i-axis (pitch) (in radians)

	void rotate_about_j (SINGLE angle);		// rotate around j-axis (yaw) (in radians)

	void rotate_about_k (SINGLE angle);		// rotate around k-axis (roll) (in radians)

	void rotate_about_x (SINGLE angle);		// rotate around x-axis (in radians)

	SINGLE get_roll (void) const;	// returns in RADIANS
	
	SINGLE get_pitch (void) const;	// returns in RADIANS

	SINGLE get_yaw (void) const;	// returns in RADIANS

	//for old style transforms with -z forward
	SINGLE get_roll_zbased (void) const;	// returns in RADIANS
	
	SINGLE get_pitch_zbased (void) const;	// returns in RADIANS

	SINGLE get_yaw_zbased (void) const;	// returns in RADIANS

	void set_orientation(const Matrix & mat)
	{
		Transform::set_orientation(mat);
	}
  
	void set_orientation(SINGLE pitch, SINGLE roll, SINGLE yaw)   // in degrees
	{
//		Transform::set_orientation(pitch, -roll, -yaw);

		//this is not consistent with the rest of the pitch, roll, yaw functions....
		d[0][0] = 1.0;
		d[1][0] = 0.0;
		d[2][0] = 0.0;

		d[0][1] = 0.0;
		d[1][1] = 0.0;
		d[2][1] = 1.0;

		d[0][2] = 0.0;
		d[1][2] = -1.0;
		d[2][2] = 0.0;

		rotate_about_k(yaw * MUL_DEG_TO_RAD);
		rotate_about_i(pitch * MUL_DEG_TO_RAD);
		rotate_about_j(roll * MUL_DEG_TO_RAD);

	}

	TRANSFORM & operator = (const Matrix & matrix)
	{
		*static_cast<Matrix *>(this) = matrix;
		return *this;
	}

	TRANSFORM & operator = (const Transform & transform)
	{
		*static_cast<Transform *>(this) = transform;
		return *this;
	}

	static SINGLE get_yaw (const Vector & look);

	static SINGLE get_pitch (const Vector & look);
	
	static Vector rotate_about_z (const Vector & vec, SINGLE angle);		// yaw 

	static inline Vector rotate_about_z_90 (const Vector & vec, SINGLE dir)		// rotate about z by 90 degrees, to the left if < 0
	{
		if (dir < 0)
			return Vector(-vec.y, vec.x, vec.z);
		else
			return Vector(vec.y, -vec.x, vec.z);
	}

	static inline SINGLE normalize (Vector & vec)
	{
		SINGLE M2 = vec.x*vec.x + vec.y*vec.y + vec.z*vec.z;
		if (M2 > 0)
		{
			M2 = sqrt(M2);
			const SINGLE inv_m = 1.0F / M2;	// 1*DIV+3*MUL is faster than 3*DIV
			vec.x *= inv_m;
			vec.y *= inv_m;
			vec.z *= inv_m;
		}

		return M2;
	}

	static SINGLE __fastcall getDistance2D (const Vector &vec, const Vector &vec2);	

	static const TRANSFORM & WORLD;		// rotate_about_i(90 * MUL_DEG_TO_RAD);
};



#endif
