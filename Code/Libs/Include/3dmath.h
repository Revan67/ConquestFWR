/*
	3DMATH.H - Copyright (C) 1997 Digital Anvil, Inc.
	Author: B. Baldwin

 This file defines the DA Component interface I3DMathEngine, which is used by a
 number of useful static classes like Matrix, Transform, and Quaternion, which
 are defined in MATRIX.H, XFORM.H, and QUAT.H, respectively. The default 
 implementation of this interface will use standard x86 FPU instructions, but 
 another implementation will exist for each piece of hardware that is capable 
 of accelerating the operations in question.

 Function bodies for the Matrix, Transform, and Quaternion classes are all 
 inline, and make calls to the global I3DMathEngine provider where appropriate.
 The app can get a pointer to the global I3DMathEngine provider at any time by
 calling MATH_ENGINE(), defined below in this file.

 The I3DMathEngine interface is a bit of a "catch-all" for potentially-
 acceleratable operations, and as such has a non-zero probability of future
 changes as hardware acceleration schemes proliferate both on PC and other 
 platforms. For example, currently no vector-vector operations, such as the 
 dot product, are part of I3DMathEngine. Should a 1-cycle vector dot product 
 come into existence on our target hardware, we would have to move the dot 
 product into I3DMathEngine. Etc.

*/

#ifndef _3DMATH_H
#define _3DMATH_H

//
// Allow warning-free conversions from "const double" to "float."
//
#pragma warning(disable: 4244 4305)

//

#ifndef DACOM_H
#include "DACOM.h"
#endif

//
// Include files for definitions of static math object classes.
//
#ifndef VECTOR_H
#include "vector.h"
#endif

#ifndef MATRIX_H
#include "matrix.h"
#endif

#ifndef XFORM_H
#include "xform.h"
#endif

#ifndef QUAT_H
#include "quat.h"
#endif

//

#ifndef PI
  #define PI 3.14159265358979323846f 
#endif

//

// NOTE: Second parameter is the version number. MUST BE A NUMERIC LITERAL, NOT A MACRO!
#define IID_I3DMathEngine MAKE_IID("I3DMathEngine", 1)

struct DA3DMATHDESC : public DACOMDESC
{
	C8 * implementation;

	DA3DMATHDESC(const C8 * _interface_name = IID_I3DMathEngine) : DACOMDESC(_interface_name)
	{
		memset(((C8 *)this)+sizeof(DACOMDESC), 0, sizeof(*this)-sizeof(DACOMDESC));
		size = sizeof(*this);
	}
};

//

struct I3DMathEngine : public IDAComponent
{
//
// Matrix and Transform operations.
//
	DEFMETHOD(inverse)	(Matrix & dst, const Matrix & m) = 0;
	DEFMETHOD(scale)	(Matrix & dst, const Matrix & m, SINGLE s) = 0;

// Determinant of matrix.
	DEFMETHOD_(SINGLE, det)	(const Matrix & m) = 0;

//
// Matrix and Transform concatenation.
//
	DEFMETHOD(mul)	(Matrix & dst, const Matrix & m1, const Matrix & m2) = 0;
	DEFMETHOD(mul)	(Transform & dst, const Transform & m1, const Transform & m2) = 0;

//
// Matrix and Transform operation on Vectors.
//
	DEFMETHOD(transform)	(Vector & dst, const Matrix & m, const Vector & v) = 0;
	DEFMETHOD(transform)	(Vector & dst, const Transform & t, const Vector & v) = 0;

// rotate with no translation.
	DEFMETHOD(rotate)			(Vector & dst, const Transform & t, const Vector & v) = 0;
	DEFMETHOD(inverse_rotate)	(Vector & dst, const Transform & t, const Vector & v) = 0;

//
// Transform Vector with Transpose of Matrix. The transpose might also happen
// to be the inverse in the case of a rotation matrix.
//
	DEFMETHOD(transpose_transform)	(Vector & dst, const Matrix & m, const Vector & v) = 0;
	DEFMETHOD(inverse_transform)	(Vector & dst, const Transform & t, const Vector & v) = 0;

//
// Same operations on lists of Vectors.
//
	DEFMETHOD(transform_list)	(Vector * dst, const Matrix & m, const Vector * src, int n)= 0;
	DEFMETHOD(transform_list)	(Vector * dst, const Transform & t, const Vector * src, int n) = 0;

	DEFMETHOD(transpose_transform_list)	(Vector * dst, const Matrix & m, const Vector * src, int n)= 0;
	DEFMETHOD(inverse_transform_list)	(Vector * dst, const Transform & t, const Vector * src, int n) = 0;

//
// Quaternion conversions.
//
	DEFMETHOD(matrix_to_quaternion)	(Quaternion & dst, const Matrix & m) = 0;
	DEFMETHOD(quaternion_to_matrix)	(Matrix & dst, const Quaternion & q) = 0;

//
// Quaternion operations.
//
	DEFMETHOD(transform)	(Vector & dst, const Quaternion & q, const Vector & v) = 0;
	DEFMETHOD(mul)			(Quaternion & dst, const Quaternion & q1, const Quaternion & q2) = 0;

//
// Quaternion interpolation.
//
	DEFMETHOD(quat_slerp)	(Quaternion & dst, const Quaternion & q1, const Quaternion & q2, SINGLE t) = 0;

// put here at the bottom to keep from having to rebuild
	DEFMETHOD(general_inverse)	(Transform & dst, SINGLE & w, const Transform & m) = 0;

	DEFMETHOD(InvSqrt)	(SINGLE & dst, const SINGLE x) = 0;

	DEFMETHOD(Sqrt)	(SINGLE & dst, const SINGLE x) = 0;
};

//
// the following method is implemented in the static lib MathLib.lib, in the x86math workspace:

I3DMathEngine * __stdcall __MATH_ENGINE (void);

//

static I3DMathEngine *math_engine = (I3DMathEngine*)0;

inline I3DMathEngine * MATH_ENGINE (void)
{
	if (math_engine == (I3DMathEngine*)0)
		math_engine = __MATH_ENGINE();
	return math_engine;
}

//

inline void __stdcall MATH_ENGINE_Uninitialize(void)
{
	math_engine = (I3DMathEngine*)0;
}


//-----------------------------------------------------------------------------
//
// Following are all the function bodies from MATRIX.H, XFORM.H, and QUAT.H
// that need to call the global MATH_ENGINE, plus some that don't.
//
//-----------------------------------------------------------------------------


inline Matrix::Matrix(const Quaternion & q)
{
	MATH_ENGINE()->quaternion_to_matrix(*this, q);
}

//

inline SINGLE Matrix::det(void) const
{
	return MATH_ENGINE()->det(*this);
}

//

inline Matrix Matrix::get_inverse(void) const
{
	Matrix result;
	MATH_ENGINE()->inverse(result, *this);
	return result;
}

//

inline const Matrix& Matrix::scale(const SINGLE s)
{
	MATH_ENGINE()->scale(*this, *this, s);
	return *this;
}

//

inline const Matrix& Matrix::scale_by_reciprocal(const SINGLE s)
{
	MATH_ENGINE()->scale(*this, *this, 1.0 / s);
	return *this;
}

//

inline const Matrix& Matrix::mul(const Matrix & m)
{
	Matrix result;
	MATH_ENGINE()->mul(result, *this, m);
	*this = result;
	return *this;
}

inline void Matrix::compose_rotation(const U32 axis, const SINGLE angle)
{
	switch (axis)
	{
		case PITCH:
			set_x_rotation( angle * MUL_DEG_TO_RAD );
			break;

		case YAW:
			set_y_rotation( angle * MUL_DEG_TO_RAD );
			break;

		case ROLL:
			set_z_rotation( angle * MUL_DEG_TO_RAD );
			break;
	}
}

//

inline Matrix mul(const Matrix & m, const SINGLE s)
{
	Matrix result;
	MATH_ENGINE()->scale(result, m, s);
	return result;
}

//

inline Vector mul(const Matrix & m, const Vector & v)
{
	Vector result;
	MATH_ENGINE()->transform(result, m, v);
	return result;
}

//

inline Vector transpose_mul(const Matrix & m, const Vector & v)
{
	Vector result;
	MATH_ENGINE()->transpose_transform(result, m, v);
	return result;
}

//

inline Matrix mul(const Matrix & m1, const Matrix & m2)
{
	Matrix result;
	MATH_ENGINE()->mul(result, m1, m2);
	return result;
}

//

inline void Transform::set_identity(void)
{
	Matrix::set_identity();
	translation.zero();
}

//

inline const Vector & Transform::get_position(void) const
{
	return translation;
}

//

inline Transform Transform::multiply (const Transform & t) const
{
	Transform result(false);	// bool constructor prevents extra initialization
	MATH_ENGINE()->mul(result, *this, t);
	return result;
}

inline const Transform & Transform::operator *= (const Transform & t)
{
	*this = multiply(t);
	return *this;
}

inline void Transform::compose_rotation(const U32 axis, const SINGLE angle)
{
	translation.zero();
	Matrix::compose_rotation(axis, angle);
}

//

inline Transform Transform::get_transpose(void) const
{
	return Transform(Matrix::get_transpose(), -translation);
}

//

inline Transform Transform::get_inverse(void) const
{
	return Transform( Matrix::get_transpose(), -(translation * *static_cast<const Matrix *>(this)) );
}

//

inline Transform Transform::get_general_inverse(SINGLE & w) const
{
	Transform result(false);
	MATH_ENGINE()->general_inverse(result, w, *this);
	return result;
}

//

inline void Transform::set_position(const SINGLE x, const SINGLE y, const SINGLE z)
{
	translation.x = x;
	translation.y = y;
	translation.z = z;
}

//

inline void Transform::set_position(const Vector & pos)
{
	translation = pos;
}

//

inline void Transform::move_position(const SINGLE dx, const SINGLE dy, const SINGLE dz)
{
	const Vector in( dx, dy, dz);
	Vector out;
	MATH_ENGINE()->rotate(out, *this, in);
 	translation += out;
}

//

inline void Transform::move_position(const Vector & delta)
{
	Vector out;
	MATH_ENGINE()->rotate(out, *this, delta);
 	translation += out;
}

//
inline void Transform::set_orientation(const SINGLE pitch, const SINGLE roll, const SINGLE yaw)
{
//
// sets orientation element of transform, leaving position
// vector untouched
// Apply yaw, pitch, roll in order ( angles in degrees )
// 

	Matrix::set_orientation( pitch, roll, yaw );
}

//
inline void Matrix::set_orientation(const SINGLE pitch, const SINGLE roll, const SINGLE yaw)
{
//
// Apply yaw, pitch, roll in order ( angles in degrees )
// 

	set_y_rotation( yaw * MUL_DEG_TO_RAD );
	x_rotate_left( pitch * MUL_DEG_TO_RAD );
	z_rotate_left( roll * MUL_DEG_TO_RAD );
}

//

inline const Matrix &Transform::get_orientation(void) const
{
	return *this;
}

//

inline void Transform::set_orientation(const Matrix & mat)
{
	*this = mat;
}

//

inline void Transform::set_orientation(const Quaternion & q)
{
	Matrix m ( q );
	set_orientation(m);
}

//

inline Vector Transform::rotate(const Vector & v) const
{
	Vector result;
	MATH_ENGINE()->rotate(result, *this, v);
	return result;
}

//

inline Vector Transform::rotate_translate(const Vector & v) const
{
	Vector result;
	MATH_ENGINE()->transform(result, *this, v);
	return result;
}

//

inline Vector Transform::inverse_rotate(const Vector & v) const
{
	Vector result;
	MATH_ENGINE()->inverse_rotate(result, *this, v);
	return result;
}

//

inline Vector Transform::inverse_rotate_translate(const Vector & v) const
{
	Vector result;
	MATH_ENGINE()->inverse_transform(result, *this, v);
	return result;
}

//

inline Quaternion::Quaternion(const Matrix & m)
{
	MATH_ENGINE()->matrix_to_quaternion(*this, m);
}

//

inline Quaternion::Quaternion(const Vector & axis, const SINGLE angle)
{
	const SINGLE half_angle = angle / 2.0f;

#if !defined(ANONYMOUS_ILLEGAL)
	s = (SINGLE) cos(half_angle);
	v = axis;
	v.normalize();
	v *= (SINGLE) sin(half_angle);
#else
	w = (SINGLE) cos(half_angle);
    Vector V = axis;
	V.normalize();
	V *= (SINGLE) sin(half_angle);
	x = V.x;
	y = V.y;
	z = V.z;
#endif
}

//

inline Quaternion Quaternion::set(const Matrix & m)
{
	MATH_ENGINE()->matrix_to_quaternion(*this, m);
	return *this;
}

//

inline Quaternion slerp(const Quaternion & q1, const Quaternion & q2, const SINGLE t)
{
	Quaternion result;
	MATH_ENGINE()->quat_slerp(result, q1, q2, t);
	return result;
}

//

inline Quaternion squad(const Quaternion & q0, const Quaternion & q1, const Quaternion & q2, const Quaternion & q3, const SINGLE t)
{
	return slerp(slerp(q0, q3, t), slerp(q1, q2, t), 2.0f * t * (1.0f - t));
}

//

inline Quaternion spline_squad(const Quaternion & q0, const Quaternion & q1, const Quaternion & q2, const Quaternion & q3, const SINGLE t)
{
	const Quaternion	q1_inv ( q1.get_inverse() );
	const Quaternion	q2_inv ( q2.get_inverse() );

	const Quaternion	a1 ( q1 * (((q1_inv * q2).get_log() + (q1_inv * q0).get_log()) * (-0.25f)).get_exp() );
	const Quaternion	a2 ( q2 * (((q2_inv * q3).get_log() + (q2_inv * q1).get_log()) * (-0.25f)).get_exp() );

	return squad(q1, a1, a2, q2, t);
}

//

inline Quaternion mul(const Quaternion & q1, const Quaternion & q2)
{
	Quaternion result;
	MATH_ENGINE()->mul(result, q1, q2);
	return result;
}

//

inline Vector Quaternion::transform(const Vector & vec)
{
	Vector result;
	MATH_ENGINE()->transform(result, *this, vec);
	return result;
}

inline SINGLE InvSqrt(const SINGLE x)
{
	SINGLE result;
	MATH_ENGINE()->InvSqrt(result, x);
	return result;
}

inline SINGLE Sqrt(const SINGLE x)
{
	SINGLE result;
	MATH_ENGINE()->Sqrt(result, x);
	return result;
}

//

#endif
