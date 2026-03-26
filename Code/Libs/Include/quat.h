//
//
//

#ifndef QUAT_H
#define QUAT_H

//

#ifndef MATRIX_H
#include "matrix.h"
#endif

#ifndef PERSISTMATH_H__
#include "persistmath.h"
#endif

#pragma warning (disable : 4201)   // non-standard extension (nameless struct/union)

//

//
// Unit quaternion class.
//
namespace QuaternionNS
{
const static SINGLE	epsilon = SINGLE (1e-03f);
}

class Quaternion
{
public:
//
// We need to get at the elements of the quaternion in a variety of ways.
//
	union
	{
	// 4-vector with named elements.
		struct
		{
			SINGLE	w, x, y, z;
		};

	// 4-vector array.
		SINGLE		d[4];

#if !defined(ANONYMOUS_ILLEGAL)
	// Scalar + 3-vector.
		struct
		{
			SINGLE	s;
			Vector	v;
		};
#endif
	};

	Quaternion(void) {}

// Converts rotation matrix to quaternion.
	explicit Quaternion(const Matrix & m);

// Creates quaternion representing a rotation by the given angle about the
// given axis.
	Quaternion(const Vector & axis, const SINGLE angle);

// Creates a quaternion with scalar part ZERO, vector part v. NOTE that this
// is NOT in general equivalent to creating a quaternion with axis == v and
// angle == 0.
	explicit Quaternion(const Vector & vec)
	{
#if !defined(ANONYMOUS_ILLEGAL)
		s = 0.0f;
		v = vec;
#else
        w = 0.0f;
        x = vec.x;
        y = vec.y;
        z = vec.z;
#endif
	}

	Quaternion(const SINGLE _w, const SINGLE _x, const SINGLE _y, const SINGLE _z)
	{
		w = _w;
		x = _x;
		y = _y;
		z = _z;
	}

	Quaternion(const PersistQuaternion& src)
	{
		w = src.w;
		x = src.x;
		y = src.y;
		z = src.z;
	}

	inline Quaternion set(const SINGLE _w, const SINGLE _x, const SINGLE _y, const SINGLE _z)
	{
		w = _w;
		x = _x;
		y = _y;
		z = _z;
		return *this;
	}

	inline Quaternion set(const Matrix & m);

	inline void set_identity(void)
	{
#if !defined(ANONYMOUS_ILLEGAL)
		s = 1.0f;
		v.zero();
#else
		w = 1.0f;
        x = y = z = 0.0f;
#endif
	}

	inline Quaternion get_conjugate(void) const
	{
		Quaternion result;
#if !defined(ANONYMOUS_ILLEGAL)
		result.s = s;
		result.v = -v;
#else
		result.w = w;
		result.x = -x;
		result.y = -y;
		result.z = -z;
#endif
		return result;
	}

	inline Quaternion get_inverse(void) const
	{
		return get_conjugate() * (1.0f / norm());
	}

	// Spherical Linear Interpolation
	friend Quaternion slerp(const Quaternion & q1, const Quaternion & q2, const SINGLE t);
	// Spherical Quadratic Interpolation
	inline friend Quaternion squad(const Quaternion & q0, const Quaternion & q1, const Quaternion & q2,
		const Quaternion & q3, const SINGLE t);
	// Spherical Spline Interpolation
	inline friend Quaternion spline_squad(const Quaternion & q0, const Quaternion & q1, const Quaternion & q2,
		const Quaternion & q3, const SINGLE t);

	friend Quaternion mul(const Quaternion & q1, const Quaternion & q2);

	inline friend Quaternion operator + (const Quaternion & q1, const Quaternion & q2)
	{
		return Quaternion(q1.w + q2.w, q1.x + q2.x, q1.y + q2.y, q1.z + q2.z);
	}

	inline friend Quaternion operator - (const Quaternion & q1, const Quaternion & q2)
	{
		return Quaternion(q1.w - q2.w, q1.x - q2.x, q1.y - q2.y, q1.z - q2.z);
	}

	inline friend Quaternion operator * (const Quaternion & q1, const Quaternion & q2)
	{
		return mul(q1, q2);
	}

// For debugging purposes only. Quaternions representing rotations should
// always have magnitude 1.
	inline SINGLE magnitude(void) const
	{
#if !defined(ANONYMOUS_ILLEGAL)
		return (SINGLE) sqrt(s * s + v.magnitude_squared());
#else
		return (SINGLE) sqrt(w * w + Vector(x, y, z).magnitude_squared());
#endif
	}

	inline SINGLE norm(void) const
	{
#if !defined(ANONYMOUS_ILLEGAL)
		return s * s + v.magnitude_squared();
#else
		return w * w + Vector(x, y, z).magnitude_squared();
#endif
	}

	inline Quaternion get_log(void) const
	{
#if !defined(ANONYMOUS_ILLEGAL)
		if(fabs(s) < 1.0f)
		{
			const SINGLE	angle = SINGLE (acos(s));
			const SINGLE	sine = SINGLE (sin(angle));

			if(sine >= QuaternionNS::epsilon)
			{
				return Quaternion(v * (angle / sine));
			}
		}

		return Quaternion(v);
#else
		if(fabs(w) < 1.0f)
		{
			const SINGLE	angle = acos(w);
			const SINGLE	sine = sin(angle);

			if(sine >= QuaternionNS::epsilon)
			{
				return Quaternion(Vector(x, y, z) * (angle / sine));
			}
		}

		return Quaternion(Vector(x, y, z));
#endif
	}

	// Use on pure quaternions (s == 0)
	inline Quaternion get_exp(void) const
	{
#if !defined(ANONYMOUS_ILLEGAL)
		const SINGLE	angle = v.magnitude();
		const SINGLE	sine = SINGLE (sin(angle));
		
		if(sine >= QuaternionNS::epsilon)
		{
			const Vector	exp_v = v * (sine / angle);

			return Quaternion(SINGLE (cos(angle)), exp_v.x, exp_v.y, exp_v.z);
		}
		else
		{
			return Quaternion(SINGLE (cos(angle)), v.x, v.y, v.z);
		}
#else
		const Vector	_v(x, y, z);
		const SINGLE	angle = _v.magnitude();
		const SINGLE	sine = sin(angle);
		
		if(sine >= QuaternionNS::epsilon)
		{
			const Vector	exp_v = _v * (sine / angle);

			return Quaternion(cos(angle), exp_v.x, exp_v.y, exp_v.z);
		}
		else
		{
			return Quaternion(cos(angle), x, y, z);
		}
#endif
	}

	inline void normalize(void)
	{
		SINGLE m = 1.0f / magnitude();
#if !defined(ANONYMOUS_ILLEGAL)
		s *= m;
		v *= m;
#else
		w *= m;
		x *= m;
		y *= m;
		z *= m;
#endif
	}

	inline Quaternion scale(const SINGLE scalar)
	{
#if !defined(ANONYMOUS_ILLEGAL)
		s *= scalar;
		v *= scalar;
#else
		w *= scalar;
		x *= scalar;
		y *= scalar;
		z *= scalar;
#endif
		return *this;
	}

	inline Quaternion scale_by_reciprocal(SINGLE scalar)
	{
		scalar = 1.0f / scalar;
#if !defined(ANONYMOUS_ILLEGAL)
		s *= scalar;
		v *= scalar;
#else
		w *= scalar;
		x *= scalar;
		y *= scalar;
		z *= scalar;
#endif
		return *this;
	}

	inline Quaternion operator *= (const SINGLE scalar)
	{
		return scale(scalar);
	}

	inline Quaternion operator /= (const SINGLE scalar)
	{
		return scale_by_reciprocal(scalar);
	}

	Vector transform(const Vector & vec);

	// 
	// Compute axis & angle representation of quaternion.
	//
	inline void compute_axis_angle(Vector & axis, float & angle)
	{
		angle	= (float) (2.0f * acos(w));
		axis.x	= (float) (2.0f * asin(x));
		axis.y	= (float) (2.0f * asin(y));
		axis.z	= (float) (2.0f * asin(z));

		const float mag = axis.magnitude();
		if (mag > 1e-5f)
		{
			axis /= mag;
		}
	}


	inline friend Quaternion scale(const Quaternion & q, const SINGLE scalar)
	{
		Quaternion result;
#if !defined(ANONYMOUS_ILLEGAL)
		result.s = q.s * scalar;
		result.v = q.v * scalar;
#else
		result.w = q.w * scalar;
		result.x = q.x * scalar;
		result.y = q.y * scalar;
		result.z = q.z * scalar;
#endif
		return result;
	}

	inline friend Quaternion scale_by_reciprocal(const Quaternion & q, SINGLE scalar)
	{
		Quaternion result;
		scalar = 1.0f / scalar;
#if !defined(ANONYMOUS_ILLEGAL)
		result.s = q.s * scalar;
		result.v = q.v * scalar;
#else
		result.w = q.w * scalar;
		result.x = q.x * scalar;
		result.y = q.y * scalar;
		result.z = q.z * scalar;
#endif
		return result;
	}

	inline friend Quaternion operator * (const Quaternion & q, const SINGLE scalar)
	{
		Quaternion result;
#if !defined(ANONYMOUS_ILLEGAL)
		result.s = q.s * scalar;
		result.v = q.v * scalar;
#else
		result.w = q.w * scalar;
		result.x = q.x * scalar;
		result.y = q.y * scalar;
		result.z = q.z * scalar;
#endif
		return result;
	}

	inline friend Quaternion operator / (const Quaternion & q, SINGLE scalar)
	{
		scalar = 1.0f / scalar;
		Quaternion result;
#if !defined(ANONYMOUS_ILLEGAL)
		result.s = q.s * scalar;
		result.v = q.v * scalar;
#else
		result.w = q.w * scalar;
		result.x = q.x * scalar;
		result.y = q.y * scalar;
		result.z = q.z * scalar;
#endif
		return result;
	}
};

//

#endif
