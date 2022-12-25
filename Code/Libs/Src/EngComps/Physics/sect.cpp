//
// SECT.CPP - 3D triangle intersection code. Given the 3 vertex locations
// for each of 2 triangles, expressed in the same coordinate system of course,
// the function TrianglesIntersect() returns true if the triangles intersect,
// false if not.
//

#include <stdlib.h>
#include "3dmath.h"
#include "Tfuncs.h"

//

#define ZERO_REL

//

static bool overlap(const Vector & axis, 
					const Vector & p1, const Vector & p2, const Vector & p3, 
					const Vector & q1, const Vector & q2, const Vector & q3)
{
	bool result;

//
// project all points onto axis.
//
#ifdef ZERO_REL
	float ap1 = 0.0f;	// The first vertex of the first triangle is zero by convention.
#else
	float ap1 = dot_product(axis, p1);
#endif
	float ap2 = dot_product(axis, p2);
	float ap3 = dot_product(axis, p3);

	float aq1 = dot_product(axis, q1);
	float aq2 = dot_product(axis, q2);
	float aq3 = dot_product(axis, q3);

//
// get intervals.
//
	float maxp = Tmax(ap1, Tmax(ap2, ap3));
	float minp = Tmin(ap1, Tmin(ap2, ap3));
	float maxq = Tmax(aq1, Tmax(aq2, aq3));
	float minq = Tmin(aq1, Tmin(aq2, aq3));

//
// Do intervals overlap?
//
	if ((minp > maxq) || (minq > maxp))
	{
		result = false;
	}
	else
	{
		result = true;
	}

	return result;
}

//

bool TrianglesIntersect(const Vector & a1, const Vector & a2, const Vector & a3, 
						const Vector & b1, const Vector & b2, const Vector & b3)
{
#ifdef ZERO_REL
//
// Move vertices into new coordinate system such that a1 is at (0, 0, 0).
//
	Vector p1 = Vector(0.0f, 0.0f, 0.0f);
	Vector p2 = a2 - a1;
	Vector p3 = a3 - a1;

	Vector q1 = b1 - a1;
	Vector q2 = b2 - a1;
	Vector q3 = b3 - a1;

//
// Form edge vectors.
//
	Vector e1 = p2;			// p1 is zero by convention.
	Vector e2 = p3 - p2;
	Vector e3 = p1 - p3;

#else
	Vector p1 = a1;
	Vector p2 = a2;
	Vector p3 = a3;
	Vector q1 = b1;
	Vector q2 = b2;
	Vector q3 = b3;

	Vector e1 = p2 - p1;
	Vector e2 = p3 - p2;
	Vector e3 = p1 - p3;

#endif

	Vector f1 = q2 - q1;
	Vector f2 = q3 - q2;
	Vector f3 = q1 - q3;

//
// Compute normals.
//
	Vector np = cross_product(e1, e2);
	Vector nq = cross_product(f1, f2);

//
// Compute outward vector for each edge.
//
	Vector g1 = cross_product(e1, np);
	Vector g2 = cross_product(e2, np);
	Vector g3 = cross_product(e3, np);

	Vector h1 = cross_product(f1, nq);
	Vector h2 = cross_product(f2, nq);
	Vector h3 = cross_product(f3, nq);

//
// Compute edge-edge cross terms.
//
	Vector ef11 = cross_product(e1, f1);
	Vector ef12 = cross_product(e1, f2);
	Vector ef13 = cross_product(e1, f3);

	Vector ef21 = cross_product(e2, f1);
	Vector ef22 = cross_product(e2, f2);
	Vector ef23 = cross_product(e2, f3);

	Vector ef31 = cross_product(e3, f1);
	Vector ef32 = cross_product(e3, f2);
	Vector ef33 = cross_product(e3, f3);

//
// Do projection interval tests. Triangles intersect only if intervals overlap
// on ALL axes.
//
	bool result;

	if (!overlap(np, p1, p2, p3, q1, q2, q3))
	{
		result = false;
		goto done;
	}

	if (!overlap(nq, p1, p2, p3, q1, q2, q3))
	{
		result = false;
		goto done;
	}

	if (!overlap(ef11, p1, p2, p3, q1, q2, q3))
	{
		result = false;
		goto done;
	}

	if (!overlap(ef12, p1, p2, p3, q1, q2, q3))
	{
		result = false;
		goto done;
	}

	if (!overlap(ef13, p1, p2, p3, q1, q2, q3))
	{
		result = false;
		goto done;
	}

	if (!overlap(ef21, p1, p2, p3, q1, q2, q3))
	{
		result = false;
		goto done;
	}

	if (!overlap(ef22, p1, p2, p3, q1, q2, q3))
	{
		result = false;
		goto done;
	}

	if (!overlap(ef23, p1, p2, p3, q1, q2, q3))
	{
		result = false;
		goto done;
	}

	if (!overlap(ef31, p1, p2, p3, q1, q2, q3))
	{
		result = false;
		goto done;
	}

	if (!overlap(ef32, p1, p2, p3, q1, q2, q3))
	{
		result = false;
		goto done;
	}

	if (!overlap(ef33, p1, p2, p3, q1, q2, q3))
	{
		result = false;
		goto done;
	}

	if (!overlap(g1, p1, p2, p3, q1, q2, q3))
	{
		result = false;
		goto done;
	}

	if (!overlap(g2, p1, p2, p3, q1, q2, q3))
	{
		result = false;
		goto done;
	}

	if (!overlap(g3, p1, p2, p3, q1, q2, q3))
	{
		result = false;
		goto done;
	}

	if (!overlap(h1, p1, p2, p3, q1, q2, q3))
	{
		result = false;
		goto done;
	}

	if (!overlap(h2, p1, p2, p3, q1, q2, q3))
	{
		result = false;
		goto done;
	}

	if (!overlap(h3, p1, p2, p3, q1, q2, q3))
	{
		result = false;
		goto done;
	}

//
// Okay, you win.
//
	result = true;

done:

	return result;
}