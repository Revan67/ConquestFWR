#ifndef EULERANGLES_H
#define EULERANGLES_H

//

#include "Matrix.h"

//

void EulerAnglesFromMatrix(float & x, float & y, float & z, const Matrix & R, bool negate = false);
void MatrixFromEulerAngles(Matrix & R, float x, float y, float z);

void XRotationMatrix(Matrix & R, float x);
void YRotationMatrix(Matrix & R, float y);
void ZRotationMatrix(Matrix & R, float z);

//

//
// All the following code is for XEDS (Rx * Ry * Rz * ?)
// 
// We may want XEDR (Rz * Ry * Rx * ?) ???
// 
//

//
// call with normalize = false if vectors are already unit.
//
inline float VectorAngle( const Vector & v1, const Vector & v2, bool normalize = true )
{
	Vector vc = cross_product(v1, v2);
	float sin_theta = vc.magnitude();
	float cos_theta = dot_product(v1, v2);

	if (normalize)
	{
		float mag = (float)sqrt( v1.magnitude_squared() * v2.magnitude_squared() );
		float inv_mag = 1.0 / mag;
		sin_theta *= inv_mag;
		cos_theta *= inv_mag;
	}

	return atan2(sin_theta, cos_theta);
}

//

inline void EulerAnglesFromMatrix(float & x, float & y, float & z, const Matrix & R, bool negate)
{
//	const int i = 0, j = 1, k = 2;
	const Vector & i = R.get_i();
	const Vector & j = R.get_j();
	const Vector & k = R.get_k();

// CY is magnitude of i projected onto x-y plane.
	float cy = sqrt(i.x * i.x + i.y * i.y);
	if (cy > 16 * FLT_EPSILON)
	{
	// i is not parallel to (global) z-axis.

		if (negate)
		{
			float sin0 = j.z;	// use j and k z-components to determine angle about i.
			float cos0 = -k.z;
			x = atan2( sin0,  cos0);

			float sin1 = -i.z;
			float cos1 = -cy;
			y = atan2( sin1,  cos1);

			float sin2 = i.y;
			float cos2 = -i.x;
			z = atan2( sin2,  cos2);
		}
		else
		{
			float sin0 = j.z;	// use j and k z-components to determine angle about i.
			float cos0 = k.z;
			x = atan2( sin0,  cos0);

			float sin1 = -i.z;
			float cos1 = cy;
			y = atan2( sin1,  cos1);

			float sin2 = i.y;
			float cos2 = i.x;
			z = atan2( sin2,  cos2);
		}
	}
	else
	{
	// i is parallel to global z-axis.
		if (negate)
		{
			x = atan2(k.y, -j.y);
			y = atan2(i.z, -cy);
			z = 0;
		}
		else
		{
			x = atan2(-k.y, j.y);
			y = atan2(-i.z, cy);
			z = 0;
		}
	}
}

//

inline void XRotationMatrix(Matrix & R, float x)
{
	double sinx = sin(x);
	double cosx = cos(x);

	R.d[0][0] = 1;
	R.d[0][1] = 0;
	R.d[0][2] = 0;

	R.d[1][0] = 0;
	R.d[1][1] = cosx;
	R.d[1][2] = -sinx;

	R.d[2][0] = 0;
	R.d[2][1] = sinx;
	R.d[2][2] = cosx;
}

//

inline void YRotationMatrix(Matrix & R, float y)
{
	double siny = sin(y);
	double cosy = cos(y);

	R.d[0][0] = cosy;
	R.d[0][1] = 0;
	R.d[0][2] = siny;

	R.d[1][0] = 0;
	R.d[1][1] = 1;
	R.d[1][2] = 0;

	R.d[2][0] = -siny;
	R.d[2][1] = 0;
	R.d[2][2] = cosy;
}

//

inline void ZRotationMatrix(Matrix & R, float z)
{
	double sinz = sin(z);
	double cosz = cos(z);

	R.d[0][0] = cosz;
	R.d[0][1] = -sinz;
	R.d[0][2] = 0;

	R.d[1][0] = sinz;
	R.d[1][1] = cosz;
	R.d[1][2] = 0;

	R.d[2][0] = 0;
	R.d[2][1] = 0;
	R.d[2][2] = 1;
}

//

inline void MatrixFromEulerAngles(Matrix & R, float x, float y, float z)
{
	double sinx = sin(x);
	double cosx = cos(x);

	double siny = sin(y);
	double cosy = cos(y);

	double sinz = sin(z);
	double cosz = cos(z);

	float sysx = siny * sinx;
	float cxsz = cosx * sinz;
	float cxcz = cosx * cosz;

	R.d[0][0] = cosy * cosz;
	R.d[0][1] = sysx * cosz - cxsz;
	R.d[0][2] = siny * cxcz + sinx * sinz;

	R.d[1][0] = cosy * sinz;
	R.d[1][1] = sysx * sinz + cxcz;
	R.d[1][2] = siny * cxsz - sinx * cosz;

	R.d[2][0] = -siny;
	R.d[2][1] = cosy * sinx;
	R.d[2][2] = cosy * cosx;
}

//

#endif