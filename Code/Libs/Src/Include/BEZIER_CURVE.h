#ifndef __BEZIER_CURVE__H
#define __BEZIER_CURVE__H

#include "vector.h"

// inplace
inline void SplitBezCurve2(const Vector & l0, Vector & l1,
								 Vector & r0, Vector & r1 )
{
	r1 = l1;

	l1 =
	r0 = .5f * (l0 + l1);
}

inline void SplitBezCurve2( const Vector & v0, const Vector & v1,
						   Vector & l0, Vector & l1,
						   Vector & r0, Vector & r1 )
{
	l0 = v0; 
	r1 = v1;

	l1 =
	r0 = .5f * (v0 + v1);
}

// inplace
inline void SplitBezCurve3( const Vector & l0, Vector & l1, Vector & l2,
								  Vector & r0, Vector & r1, Vector & r2 )
{
	r2 = l2;

	r1 = .5f * (l1 + l2);
	l1 = .5f * (l0 + l1);
	
	l2 =
	r0 = .5f * (l1 + r1);
}

inline void SplitBezCurve3( const Vector & v0, const Vector & v1, const Vector & v2,
						   Vector & l0, Vector & l1, Vector & l2,
						   Vector & r0, Vector & r1, Vector & r2 )
{
	l0 = v0; 
	r2 = v2;

	l1 = .5f * (v0 + v1);
	r1 = .5f * (v1 + v2);

	l2 =
	r0 = .5f * (l1 + r1);
}

// this is done in place.  l0-l3 is the initial source
inline void SplitBezCurve4( const Vector & l0, Vector & l1, Vector & l2, Vector & l3,
						          Vector & r0, Vector & r1, Vector & r2, Vector & r3 )
{
	r3 = l3;

	l1 = .5f * (l0 + l1);
	r2 = .5f * (l2 + l3);

	r1 = .5f * (l1 + l2); // temporary
	l2 = .5f * (l1 + r1);
	r1 = .5f * (r2 + r1);

	l3 = 
	r0 = .5f * (l2 + r1);
}

inline void SplitBezCurve4( const Vector & v0, const Vector & v1, const Vector & v2, const Vector & v3,
						   Vector & l0, Vector & l1, Vector & l2, Vector & l3,
						   Vector & r0, Vector & r1, Vector & r2, Vector & r3 )
{
	l0 = v0; 
	r3 = v3;

	l1 = .5f * (v0 + v1);
	r2 = .5f * (v2 + v3);

	l2 =
	r1 = .25f * (v1 + v2);
	l2 += .5f * l1;
	r1 += .5f * r2;

	l3 =
	r0 = .5f * (l2 + r1);
}

// inplace
inline void SplitBezCurve5( const Vector & l0, Vector & l1, Vector & l2, Vector & l3, Vector & l4,
								  Vector & r0, Vector & r1, Vector & r2, Vector & r3, Vector & r4 )
{
	r4 = l4;

	l1 = .5f * (l0 + l1);
	r3 = .5f * (l3 + l4);

	r1 = .5f * (l1 + l2); // tmp
	r2 = .5f * (l2 + l3); // tmp

	l2 = .5f * (l1 + r1);
	r1 = .5f * (r1 + r2); // tmp
	r2 = .5f * (r3 + r2);

	l3 = .5f * (l2 + r1);
	r1 = .5f * (r2 + r1);

	l4 =
	r0 = .5f * (l3 + r1);
}

inline void SplitBezCurve5( const Vector & v0, const Vector & v1, const Vector & v2, const Vector & v3, const Vector & v4,
						   Vector & l0, Vector & l1, Vector & l2, Vector & l3, Vector & l4,
						   Vector & r0, Vector & r1, Vector & r2, Vector & r3, Vector & r4 )
{
#if 1 // optimized
	l0 = v0; 
	r4 = v4;

	l1 = .5f * (v0 + v1);
	r3 = .5f * (v3 + v4);

	// tmp
	l2 = .5f * (v1 + v2);
	r2 = .5f * (v2 + v3);
	
	// tmp
	r1 = .5f * (l2 + r2);

	l2 = .5f * (l1 + l2); 
	r2 = .5f * (r3 + r2);

	l3 = .5f * (l2 + r1);
	r1 = .5f * (r2 + r1);

	l4 =
	r0 = .5f * (l3 + r1);
#else // straight forward
	l0 = v0; 
	r4 = v4;

	l1 = .5f * (v0 + v1);
	r3 = .5f * (v3 + v4);

	const Vector v12 ( .5f * (v1 + v2) );
	const Vector v23 ( .5f * (v2 + v3) );
	l2 = .5f * (l1 + v12); 
	r2 = .5f * (r3 + v23);
	
	const Vector v1223( .5f * (v12 + v23) );
	l3 = .5f * (l2 + v1223);
	r1 = .5f * (r2 + v1223);

	l4 =
	r0 = .5f * (l3 + r1);
#endif
}

#endif
