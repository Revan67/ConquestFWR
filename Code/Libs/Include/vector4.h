#ifndef _VECTOR4_H_
#define _VECTOR4_H_

#include <memory.h>
#include <math.h>

#ifdef SGI
#ifndef bool
typedef unsigned char bool;
#endif
#endif

#ifndef true
#define true ((bool)1)
#endif

#ifndef false
#define false ((bool)0)
#endif

typedef float SINGLE;

class Vector4
{
public:
	SINGLE x, y, z, w;

	Vector4(void) {}

	Vector4(SINGLE v) { x = y = z = w = v; }

	Vector4(SINGLE v1, SINGLE v2, SINGLE v3, SINGLE v4) { x=v1; y=v2; z=v3; w=v4; }

	inline void zero(void)
	{
		memset(this, 0, sizeof(*this));
	}

	inline const Vector4 & operator += (const Vector4 & v)
	{
		x += v.x;
		y += v.y;
		z += v.z;
		w += v.w;

		return *this;
	}

	inline const Vector4 & operator -= (const Vector4 & v)
	{
		x -= v.x;
		y -= v.y;
		z -= v.z;
		w -= v.w;

		return *this;
	}

	inline friend Vector4 operator + (const Vector4 & v1, const Vector4 & v2)
	{
		return Vector4( v1.x + v2.x, v1.y + v2.y, v1.z + v2.z, v1.w + v2.w); 
	}

	inline friend Vector4 operator * (const Vector4 & v, const SINGLE s)
	{
		return Vector4( v.x*s, v.y*s, v.z*s, v.w*s);
	}

	inline friend Vector4 operator * (const SINGLE s, const Vector4 & v)
	{
		return v * s;
	}

	inline const Vector4 & operator *= (const SINGLE s)
	{
		x *= s;
		y *= s;
		z *= s;
		w *= s;

		return *this;
	}

	inline friend SINGLE dot4(const Vector4 & v1, const Vector4 & v2)
	{
		return v1.x*v2.x + v1.y*v2.y + v1.z*v2.z + v1.w*v2.w;
	}
};


#endif
