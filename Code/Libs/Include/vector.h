//
//
//

#ifndef VECTOR_H
#define VECTOR_H

#ifndef US_TYPEDEFS
#include "typedefs.h"
#endif

#ifndef PERSISTMATH_H__
#include "persistmath.h"
#endif

#ifndef _INC_MATH
#include <math.h>
#endif

#if defined(_DEBUG_VECTORS)
	#include <assert.h>
	#include <float.h>
	#define FIND_NAN(_x) assert(!_isnan(_x))
	#define NAN_CHECK()  FIND_NAN(x); FIND_NAN(y); FIND_NAN(z);
#else
	#define NAN_CHECK() //
#endif

//
// 3d vector.
//
class Vector
{
public:

	SINGLE	x, y, z;

	Vector(void)
	{
	}

	Vector(const SINGLE xx, const SINGLE yy, const SINGLE zz)
	{
		x = xx;
		y = yy;
		z = zz;
		NAN_CHECK();
	}

	Vector (const PersistVector& src)
	{
		x = src.x;
		y = src.y;
		z = src.z;
		NAN_CHECK();
	}

	inline void set(const SINGLE xx, const SINGLE yy, const SINGLE zz)
	{
		x = xx;
		y = yy;
		z = zz;
		NAN_CHECK();
	}

	inline void zero(void)
	{
		x = y = z = 0.0f;
	}

	inline SINGLE magnitude_squared(void) const
	{
		return x*x + y*y + z*z;
	}

	friend SINGLE	Sqrt(const SINGLE x);
	inline SINGLE fast_magnitude(void) const
	{
		return (SINGLE) ::Sqrt(x*x + y*y + z*z);
	}

	inline SINGLE magnitude(void) const
	{
		return (SINGLE) sqrt(x*x + y*y + z*z);
	}

	friend SINGLE	InvSqrt(const SINGLE x);
	inline const Vector& fast_normalize(void)
	{
		const SINGLE inv_m = ::InvSqrt(x*x + y*y + z*z);
		x *= inv_m;
		y *= inv_m;
		z *= inv_m;
		NAN_CHECK();
		return *this;
	}

	inline const Vector& normalize(void)
	{
		const SINGLE inv_m = 1.0F / magnitude();	// 1*DIV+3*MUL is faster than 3*DIV
		x *= inv_m;
		y *= inv_m;
		z *= inv_m;
		NAN_CHECK();
		return *this;
	}

	inline void resize (const SINGLE s)
	{
		const SINGLE inv = s / magnitude();		// 1*DIV+3*MUL is faster than 3*DIV
		x *= inv;
		y *= inv;
		z *= inv;
		NAN_CHECK();
	}

// return -(*this).
	inline Vector negative(void) const
	{
		return Vector(-x, -y, -z);
	}

// *this += v.
	inline const Vector& add(const Vector & v)
	{
		x += v.x;
		y += v.y;
		z += v.z;
		NAN_CHECK();
		return *this;
	}

// *this -= v.
	inline const Vector& subtract(const Vector & v)
	{
		x -= v.x;
		y -= v.y;
		z -= v.z;
		NAN_CHECK();
		return *this;
	}

// *this = v1 + v2;
	inline const Vector& add(const Vector & v1, const Vector & v2)
	{
		x = v1.x + v2.x;
		y = v1.y + v2.y;
		z = v1.z + v2.z;
		NAN_CHECK();
		return *this;
	}

// *this = v1 - v2;
	inline const Vector& subtract(const Vector & v1, const Vector & v2)
	{
		x = v1.x - v2.x;
		y = v1.y - v2.y;
		z = v1.z - v2.z;
		NAN_CHECK();
		return *this;
	}

// *this = *this * s;
	inline const Vector& scale(const SINGLE s)
	{
		x *= s;
		y *= s;
		z *= s;
		NAN_CHECK();
		return *this;
	}

// *this = v * s;
	inline const Vector& scale(const Vector & v, const SINGLE s)
	{
		x = v.x * s;
		y = v.y * s;
		z = v.z * s;
		NAN_CHECK();
		return *this;
	}

// *this = *this / s;
	inline const Vector& scale_by_reciprocal(const SINGLE s)
	{
		const SINGLE inv_s = 1.0F / s;		// 1*DIV+3*MUL is faster than 3*DIV
		x *= inv_s;
		y *= inv_s;
		z *= inv_s;
		NAN_CHECK();
		return *this;
	}

// *this = v / s;
	inline const Vector& scale_by_reciprocal(const Vector & v, const SINGLE s)
	{
		const SINGLE inv_s = 1.0F / s;		// 1*DIV+3*MUL is faster than 3*DIV
		x = v.x * inv_s;
		y = v.y * inv_s;
		z = v.z * inv_s;
		NAN_CHECK();
		return *this;
	}

	static inline SINGLE pow2 ( const SINGLE f ) { return f * f; } // avoids calling fabs below
	inline bool equal(const Vector & v, const SINGLE tolerance) const
	{
		const SINGLE tolerance_sq = tolerance * tolerance;
		return ((pow2( x - v.x ) <= tolerance_sq) &&
				(pow2( y - v.y ) <= tolerance_sq) &&
				(pow2( z - v.z ) <= tolerance_sq));
	}

//
// Overloaded operators follow. These all call the corresponding functions
// above.
//
	inline Vector operator - (void) const
	{
		return negative();
	}

	inline const Vector& operator += (const Vector & v)
	{
		return add(v);
	}

	inline const Vector& operator -= (const Vector & v)
	{
		return subtract(v);
	}

	inline const Vector& operator *= (const SINGLE s)
	{
		return scale(s);
	}

	inline const Vector& operator /= (const SINGLE s)
	{
		return scale_by_reciprocal(s);
	}

	inline const bool operator == (const Vector & v)
	{
		return (x == v.x && y == v.y && z == v.z);
	}


//
// friend functions follow.
//
	inline friend SINGLE dot_product(const Vector & v1, const Vector & v2)
	{
		return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
	}

	inline friend Vector cross_product(const Vector & v1, const Vector & v2)
	{
		return Vector (
			v1.y * v2.z - v1.z * v2.y,
			v1.z * v2.x - v1.x * v2.z,
			v1.x * v2.y - v1.y * v2.x );
	}

	// This function calc reflection vector
	// e' = 2( e.n )n - e
	inline friend Vector reflection( const Vector &n, const Vector &e )
	{
		Vector ans = n;
		ans *= ( dot_product( n, e ) * 2.0f );
		return ans.subtract( e );
	}

	// Calculate position between a and b with linear interpolate
	inline friend Vector lerp( const Vector &a, const Vector &b, float t )
	{
		Vector ans;
		ans.x = a.x + ( b.x - a.x ) * t;
		ans.y = a.y + ( b.y - a.y ) * t;
		ans.z = a.z + ( b.z - a.z ) * t;
		return ans;
	}

	inline friend Vector add(const Vector & v1, const Vector & v2)
	{
		return Vector ( v1.x + v2.x, v1.y + v2.y, v1.z + v2.z );
	}

	inline friend Vector subtract(const Vector & v1, const Vector & v2)
	{
		return Vector ( v1.x - v2.x, v1.y - v2.y, v1.z - v2.z );
	}

	inline friend Vector scale(const Vector & v, const SINGLE s)
	{
		return Vector ( v.x * s, v.y * s, v.z * s );
	}

	inline friend Vector scale_by_reciprocal(const Vector & v, const SINGLE s)
	{
		const SINGLE inv_s = 1.0F / s;		// 1*DIV+3*MUL is faster than 3*DIV
		return Vector ( v.x * inv_s, v.y * inv_s, v.z * inv_s );
	}

	inline friend bool equal(const Vector & v1, const Vector & v2, const SINGLE tolerance)
	{
		return v1.equal(v2, tolerance);
	}

//
// Friend overloaded operators follow.
//
	inline friend Vector operator + (const Vector & v1, const Vector & v2)
	{
		return ::add(v1, v2);
	}

	inline friend Vector operator - (const Vector & v1, const Vector & v2)
	{
		return ::subtract(v1, v2);
	}

	inline friend Vector operator * (const Vector & v, const SINGLE s)
	{
		return ::scale(v, s);
	}

	inline friend Vector operator * (SINGLE s, const Vector & v)
	{
		return ::scale(v, s);
	}

	inline friend Vector operator / (const Vector & v, const SINGLE s)
	{
		return ::scale_by_reciprocal(v, s);
	}
};

//

#endif
