//$Header: /Conquest/Libs/Include/matrix.h 5     4/28/00 11:57p Rmarr $
//copyright (c) 1997 Digital Anvil, Inc.
//

#ifndef MATRIX_H
#define MATRIX_H

//

#ifndef US_TYPEDEFS
#include "Typedefs.h"
#endif

#ifndef VECTOR_H
#include "vector.h"
#endif

#ifndef _INC_MEMORY
#include <memory.h>
#endif

//
// Rotation axis definitions
//

#define X_AXIS 0
#define Y_AXIS 1
#define Z_AXIS 2

// assumes Y is up
#define PITCH  X_AXIS
#define YAW    Y_AXIS
#define ROLL   Z_AXIS

//

class Quaternion;	// Forward reference.

//
// 3x3 Matrix class.
//
class Matrix
{
public:

	SINGLE d[3][3];

	Matrix(void) {}
	
	Matrix(const Vector & i, const Vector & j, const Vector & k)
	{
		d[0][0] = i.x;
		d[1][0] = i.y;
		d[2][0] = i.z;

		d[0][1] = j.x;
		d[1][1] = j.y;
		d[2][1] = j.z;

		d[0][2] = k.x;
		d[1][2] = k.y;
		d[2][2] = k.z;
	}

	Matrix(const Quaternion & q);

	Matrix(	const SINGLE e00, const SINGLE e01, const SINGLE e02,
			const SINGLE e10, const SINGLE e11, const SINGLE e12,
			const SINGLE e20, const SINGLE e21, const SINGLE e22)
	{
		d[0][0] = e00;
		d[0][1] = e01;
		d[0][2] = e02;

		d[1][0] = e10;
		d[1][1] = e11;
		d[1][2] = e12;

		d[2][0] = e20;
		d[2][1] = e21;
		d[2][2] = e22;
	}

	Matrix (const PersistMatrix& src)
	{
		d[0][0] = src.e00;
		d[0][1] = src.e01;
		d[0][2] = src.e02;
		d[1][0] = src.e10;
		d[1][1] = src.e11;
		d[1][2] = src.e12;
		d[2][0] = src.e20;
		d[2][1] = src.e21;
		d[2][2] = src.e22;
	}

// Returns determinant of matrix.
	SINGLE det(void) const;

// Sets matrix = Identity matrix.
	inline const Matrix& set_identity(void)
	{
		d[0][0] = d[1][1] = d[2][2] = 1.0f;

		d[0][1] = d[0][2] =
		d[1][0] = d[1][2] =
		d[2][0] = d[2][1] = 0.0f;

		return *this;
	}

	inline void make_orthogonal()
	{
		Vector i = get_i();
		Vector j = get_j();
		i.fast_normalize();
		j.fast_normalize();
		Vector k = cross_product(i,j);
		k.fast_normalize();
		j = cross_product(k,i);
		j.fast_normalize();
		set_i(i);
		set_j(j);
		set_k(k);
	}

	//-------------------------------------------------------------------
	//
	inline void rotate_around_i (SINGLE angle)	// pitch
	{
		SINGLE temp[3][3];
		SINGLE cosine, sine;

		angle = -angle;
		cosine = (SINGLE)(cos(angle));
		sine = (SINGLE)(sin(angle));

		memcpy(temp, d, sizeof(d));
		d[0][1] = (cosine * temp[0][1]) - (sine * temp[0][2]);
		d[1][1] = (cosine * temp[1][1]) - (sine * temp[1][2]);
		d[2][1] = (cosine * temp[2][1]) - (sine * temp[2][2]);

		d[0][2] = (cosine * temp[0][2]) + (sine * temp[0][1]);
		d[1][2] = (cosine * temp[1][2]) + (sine * temp[1][1]);
		d[2][2] = (cosine * temp[2][2]) + (sine * temp[2][1]);
	}

	//-------------------------------------------------------------------
	//
	inline void rotate_around_j (SINGLE angle)	// yaw
	{
		SINGLE temp[3][3];
		SINGLE cosine, sine;

	//	angle *= PI_RADIANS;		// convert to radians
		cosine = (SINGLE)(cos(angle));
		sine = (SINGLE)(sin(angle));
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
	inline void rotate_around_k (SINGLE angle)	// roll
	{
		SINGLE temp[3][3];
		SINGLE cosine, sine;

	//	angle *= -PI_RADIANS;		// convert to radians
		angle = -angle;
		cosine = (SINGLE)(cos(angle));
		sine = (SINGLE)(sin(angle));

		memcpy(temp, d, sizeof(d));

		d[0][0] = (cosine * temp[0][0]) + (sine * temp[0][1]);
		d[1][0] = (cosine * temp[1][0]) + (sine * temp[1][1]);
		d[2][0] = (cosine * temp[2][0]) + (sine * temp[2][1]);

		d[0][1] = (cosine * temp[0][1]) - (sine * temp[0][0]);
		d[1][1] = (cosine * temp[1][1]) - (sine * temp[1][0]);
		d[2][1] = (cosine * temp[2][1]) - (sine * temp[2][0]);
	}

	inline const Matrix& zero(void)
	{
		memset(d, 0, sizeof(SINGLE) * 9);
		return *this;
	}

// For a rotation matrix, inverse == transpose, but not for general matrices.
	inline Matrix get_transpose(void) const
	{
		return Matrix (
			d[0][0], d[1][0], d[2][0],
			d[0][1], d[1][1], d[2][1],
			d[0][2], d[1][2], d[2][2] );
	}

// Returns inverse of matrix.
	Matrix			get_inverse(void) const;

	const Matrix& scale(const SINGLE s);
	const Matrix& scale_by_reciprocal(const SINGLE s);

	const Matrix& mul(const Matrix & m);

	friend Matrix	add(const Matrix & m1, const Matrix & m2)
	{
		return Matrix (
			m1.d[0][0] + m2.d[0][0],
			m1.d[0][1] + m2.d[0][1],
			m1.d[0][2] + m2.d[0][2],
			m1.d[1][0] + m2.d[1][0],
			m1.d[1][1] + m2.d[1][1],
			m1.d[1][2] + m2.d[1][2],
			m1.d[2][0] + m2.d[2][0],
			m1.d[2][1] + m2.d[2][1],
			m1.d[2][2] + m2.d[2][2] );
	}

	friend Matrix	subtract(const Matrix & m1, const Matrix & m2)
	{
		return Matrix (
			m1.d[0][0] - m2.d[0][0],
			m1.d[0][1] - m2.d[0][1],
			m1.d[0][2] - m2.d[0][2],
			m1.d[1][0] - m2.d[1][0],
			m1.d[1][1] - m2.d[1][1],
			m1.d[1][2] - m2.d[1][2],
			m1.d[2][0] - m2.d[2][0],
			m1.d[2][1] - m2.d[2][1],
			m1.d[2][2] - m2.d[2][2] );
	}

	friend Matrix	mul(const Matrix & m, const SINGLE s);
	friend Vector	mul(const Matrix & m, const Vector & v);
	friend Vector	transpose_mul(const Matrix & m, const Vector & v);

	friend Matrix	mul(const Matrix & m1, const Matrix & m2);

// Overloaded operators follow.

	inline const Matrix & operator += (const Matrix & m)
	{
		d[0][0] += m.d[0][0];
		d[0][1] += m.d[0][1];
		d[0][2] += m.d[0][2];

		d[1][0] += m.d[1][0];
		d[1][1] += m.d[1][1];
		d[1][2] += m.d[1][2];

		d[2][0] += m.d[2][0];
		d[2][1] += m.d[2][1];
		d[2][2] += m.d[2][2];

		return *this;
	}

	inline const Matrix & operator -= (const Matrix & m)
	{
		d[0][0] -= m.d[0][0];
		d[0][1] -= m.d[0][1];
		d[0][2] -= m.d[0][2];

		d[1][0] -= m.d[1][0];
		d[1][1] -= m.d[1][1];
		d[1][2] -= m.d[1][2];

		d[2][0] -= m.d[2][0];
		d[2][1] -= m.d[2][1];
		d[2][2] -= m.d[2][2];

		return *this;
	}

	inline const Matrix & operator *= (const SINGLE s)
	{
		return scale(s);
	}

	inline const Matrix & operator /= (const SINGLE s)
	{
		return scale_by_reciprocal(s);
	}

	inline const Matrix & operator *= (const Matrix & m)
	{
		return mul(m);
	}

	inline friend Matrix operator + (const Matrix &m1, const Matrix & m2)
	{
		return add(m1, m2);
	}

	inline friend Matrix operator - (const Matrix & m1, const Matrix & m2)
	{
		return subtract(m1, m2);
	}

	inline friend Matrix operator * (const Matrix & m, const SINGLE s)
	{
		return ::mul(m, s);
	}

	inline friend Matrix operator * (const SINGLE s, const Matrix & m)
	{
		return ::mul(m, s);
	}

	inline friend Matrix operator / (const Matrix & m, const SINGLE s)
	{
		return ::mul(m, 1.0f / s);
	}

//
// NOTE WELL that the following 2 operators ARE NOT EQUIVALENT, since
// as we all know, matrix multiplication is not commutative.
//
// mat * vec PRE-multiplies a column vector, and 
// vec * mat POST-multiplies a row vector.
//
// The former is equivalent to mul(mat, vec).
// The latter IS equivalent to transpose_mul(mat, vec). The operators
// are provided for completeness.
//
	inline friend Vector operator * (const Matrix & m, const Vector & v)
	{
		return ::mul(m, v);
	}

	inline friend Vector operator * (const Vector & v, const Matrix & m)
	{
		return ::transpose_mul(m, v);
	}

	inline friend Matrix operator * (const Matrix & m1, const Matrix & m2)
	{
		return ::mul(m1, m2);
	}

#ifdef _INC_MEMORY	
	inline bool operator == (const Matrix & m) const
	{
		return ( 0 == memcmp(this, &m, sizeof(*this)) );
	}
	
	inline bool operator != (const Matrix & m) const
	{
		return !(*this == m);
	}
#endif

	static inline SINGLE pow2 ( const SINGLE f ) { return f * f; } // avoids calling fabs below
	inline bool equal(const Matrix & m, const SINGLE tolerance) const
	{
		const SINGLE tolerance_sq = tolerance * tolerance;
		return ((pow2( d[0][0] - m.d[0][0] ) <= tolerance_sq) &&
				(pow2( d[0][1] - m.d[0][1] ) <= tolerance_sq) &&
				(pow2( d[0][2] - m.d[0][2] ) <= tolerance_sq) &&
				(pow2( d[1][0] - m.d[1][0] ) <= tolerance_sq) &&
				(pow2( d[1][1] - m.d[1][1] ) <= tolerance_sq) &&
				(pow2( d[1][2] - m.d[1][2] ) <= tolerance_sq) &&
				(pow2( d[2][0] - m.d[2][0] ) <= tolerance_sq) &&
				(pow2( d[2][1] - m.d[2][1] ) <= tolerance_sq) &&
				(pow2( d[2][2] - m.d[2][2] ) <= tolerance_sq));
	}

//
// Returns 2nd order skew-symmetric dual tensor of vector.
//
	friend Matrix dual(const Vector & v)
	{
		return Matrix (
			0.0f, -v.z,  v.y,
			 v.z, 0.0f, -v.x,
			-v.y,  v.x, 0.0f );
	}

	inline Vector get_i(void) const
	{
		return Vector(d[0][0], d[1][0], d[2][0]);
	}

	inline Vector get_j(void) const
	{
		return Vector(d[0][1], d[1][1], d[2][1]);
	}

	inline Vector get_k(void) const
	{
		return Vector(d[0][2], d[1][2], d[2][2]);
	}

	inline void set_i(const Vector &i)
	{
		d[0][0] = i.x;
		d[1][0] = i.y;
		d[2][0] = i.z;
	}

	inline void set_j(const Vector &j)
	{
		d[0][1] = j.x;
		d[1][1] = j.y;
		d[2][1] = j.z;
	}

	inline void set_k(const Vector &k)
	{
		d[0][2] = k.x;
		d[1][2] = k.y;
		d[2][2] = k.z;
	}

	void compose_rotation(const U32 axis, const SINGLE angle); // expects degrees
	void set_orientation(const SINGLE pitch, const SINGLE roll, const SINGLE yaw); // expects degrees

	inline void set_x_rotation(const SINGLE angle) // pitch
	{
		const SINGLE _cos = (SINGLE)cos(angle);
		const SINGLE _sin = (SINGLE)sin(angle);

		d[0][0] =  1.0f; d[0][1] =  0.0f; d[0][2] =  0.0f;
		d[1][0] =  0.0f; d[1][1] =  _cos; d[1][2] = -_sin;
		d[2][0] =  0.0f; d[2][1] =  _sin; d[2][2] =  _cos;
	}

	inline void set_y_rotation(const SINGLE angle) // yaw
	{
		const SINGLE _cos = (SINGLE)cos(angle);
		const SINGLE _sin = (SINGLE)sin(angle);

		d[0][0] =  _cos; d[0][1] =  0.0f; d[0][2] =  _sin;
		d[1][0] =  0.0f; d[1][1] =  1.0f; d[1][2] =  0.0f;
		d[2][0] = -_sin; d[2][1] =  0.0f; d[2][2] =  _cos;
	}

	inline void set_z_rotation(const SINGLE angle) // roll
	{
		const SINGLE _cos = (SINGLE)cos(angle);
		const SINGLE _sin = (SINGLE)sin(angle);

		d[0][0] =  _cos; d[0][1] = -_sin; d[0][2] =  0.0f;
		d[1][0] =  _sin; d[1][1] =  _cos; d[1][2] =  0.0f;
		d[2][0] =  0.0f; d[2][1] =  0.0f; d[2][2] =  1.0f;
	}
	
	inline void x_rotate_left(const SINGLE angle) // pitch
	{
		const SINGLE _cos = (SINGLE)cos(angle);
		const SINGLE _sin = (SINGLE)sin(angle);

		for(int i = 0; i < 3; i++)
		{
			const SINGLE tmp = d[1][i];
			d[1][i] = tmp * _cos - d[2][i] * _sin;
			d[2][i] = tmp * _sin + d[2][i] * _cos;
		}
	}

	inline void y_rotate_left(const SINGLE angle) // yaw
	{
		const SINGLE _cos = (SINGLE)cos(angle);
		const SINGLE _sin = (SINGLE)sin(angle);

		for(int i = 0; i < 3; i++)
		{
			const SINGLE tmp = d[0][i];
			d[0][i] = tmp     * _cos + d[2][i] * _sin;
			d[2][i] = d[2][i] * _cos - tmp     * _sin;
		}
	}

	inline void z_rotate_left(const SINGLE angle) // roll
	{
		const SINGLE _cos = (SINGLE)cos(angle);
		const SINGLE _sin = (SINGLE)sin(angle);

		for(int i = 0; i < 3; i++)
		{
			const SINGLE tmp = d[0][i];
			d[0][i] = tmp * _cos - d[1][i] * _sin;
			d[1][i] = tmp * _sin + d[1][i] * _cos;
		}
	}

	inline void x_rotate_right(const SINGLE angle) // pitch
	{
		const SINGLE _cos = (SINGLE)cos(angle);
		const SINGLE _sin = (SINGLE)sin(angle);

		for(int i = 0; i < 3; i++)
		{
			const SINGLE tmp = d[i][1];
			d[i][1] = d[i][2] * _sin + tmp * _cos;
			d[i][2] = d[i][2] * _cos - tmp * _sin;
		}
	}

	inline void y_rotate_right(const SINGLE angle) // yaw
	{
		const SINGLE _cos = (SINGLE)cos(angle);
		const SINGLE _sin = (SINGLE)sin(angle);

		for(int i = 0; i < 3; i++)
		{
			const SINGLE tmp = d[i][0];
			d[i][0] = tmp * _cos - d[i][2] * _sin;
			d[i][2] = tmp * _sin + d[i][2] * _cos;
		}
	}

	inline void z_rotate_right(const SINGLE angle) // roll
	{
		const SINGLE _cos = (SINGLE)cos(angle);
		const SINGLE _sin = (SINGLE)sin(angle);

		for(int i = 0; i < 3; i++)
		{
			const SINGLE tmp = d[i][0];
			d[i][0] = d[i][1] * _sin + tmp * _cos;
			d[i][1] = d[i][1] * _cos - tmp * _sin;
		}
	}
};

//

#endif

