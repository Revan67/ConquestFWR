#ifndef GEOM_H
#define GEOM_H

//
// NOTE: Most of the geometric primitive types are axis-aligned, centered at origin, etc. for simplicity.
// The extent types that contain these primitives have arbitrary transforms, so you're not really limited.
//

#include "3dmath.h"

//

struct GeometricPrimitive
{
};

//

struct LineSegment : public GeometricPrimitive
{
	Vector p0, p1;	// endpoints.

	LineSegment(void)
	{
		p0.zero();
		p1.zero();
	}

	LineSegment(const Vector & start, const Vector & end)
	{
		init(start, end);
	}

	LineSegment(const Vector & start, const Vector & direction, float length)
	{
		init(start, direction, length);
	}

	void init(const Vector & start, const Vector & end)
	{
		p0 = start;
		p1 = end;
	}

	void init(const Vector & start, const Vector & direction, float length)
	{
		p0 = start;
		Vector dir = direction;
		dir.normalize();
		p1 = start + dir * length;
	}
};

//

struct Ray : public GeometricPrimitive
{
	Vector p0, dir;

	Ray(void)
	{
		p0.zero();
		dir.zero();
	}

	Ray(const Vector &point, const Vector & direction)
	{
		init(point, direction);
	}

	void init(const Vector &point, const Vector &direction)
	{
		p0 = point;
		dir = direction;
		dir.normalize();
	}

	Vector evaluate (SINGLE t) const
	{
		// NOTE: evaluate(0) == p0.
		return p0 + t * dir;
	}
};

//

struct Plane : public GeometricPrimitive
{
//
// A plane is defined by the equation Ax + By + Cz + D = 0.
//
	Vector	N;	// Normal vector. A, B, C in the plane equation.
	float	D;	// D in the plane equation.

	Plane(void)
	{
		N.zero();
		D = 0;
	}

// Construct a plane give a point on the plane and the plane normal.
	Plane(const Vector & point, const Vector & normal)
	{
		init(point, normal);
	}

	Plane(const Vector & v1, const Vector & v2, const Vector & v3)
	{
		init(v1, v2, v3);
	}

	void init(const Vector & point, const Vector & normal)
	{
		N = normal;
		N.normalize();
		D = -dot_product(N, point);
	}

// Submit vertices in CLOCKWISE order.
	void init(const Vector & v1, const Vector & v2, const Vector & v3)
	{
		Vector e1 = v3 - v2;
		Vector e2 = v2 - v1;
		N = cross_product(e1, e2);
		N.normalize();
		D = -dot_product(N, v1);
	}

	float compute_distance(const Vector & p) const
	{
		return dot_product(N, p) + D;
	}

	bool intersect_ray (const Ray &ray, Vector &intersectPoint, SINGLE &t, bool &inPlane) const
	{
		// Find the intersection between the given ray and this plane. If they intersect, this function returns true,
		// otherwise it returns false. If the ray lays entirely in the plane, the intersection point returned is the
		// p0 point of the ray. The parameter at the point of interesection is also returned.

		// The intersection occurs where (N dot (ray.p0 + t * ray.dir)) + D == 0.
		// <derivation steps skipped>
		// Therefore t = (-D - (N dot ray.p0)) / (N dot ray.dir).
		// If N dot ray.dir == 0 (i.e. the direction of the ray and the normal of the surface are perpendicular)
		// the ray intersects only if (N dot ray.p0) == D, indicating that the entire ray lays in the plane.

		// Find the parameter of intersection.

		SINGLE num = -D - dot_product (N, ray.p0);
		SINGLE denom = dot_product (N, ray.dir);

		inPlane = false;
		if (denom == 0)
		{
			if (num == 0)
			{
				// Lays in plane. Choose intersect at t=0
				t = 0;
				inPlane = true;
			}
			else
			{
				// No intersection. Parallel.
				return false;
			}
		}
		else
		{
			t = num/denom;
		}

		intersectPoint = ray.evaluate (t);
		return true;
	}
};

//

struct Sphere : public GeometricPrimitive
{
	SINGLE radius;
};

//
// Cylinder centered at origin, along z-axis, symmetrical about x-y plane.
//
struct Cylinder : public GeometricPrimitive
{
	float	length;
	float	radius;

	Cylinder(void)
	{
		length = radius = 1;
	}

	Cylinder(float len, float rad)
	{
		length = len;
		radius = rad;
	}
};

//
// Axis-aligned box symmetrical about origin in each dimension. 
//
struct Box : public GeometricPrimitive
{
	SINGLE half_x;
	SINGLE half_y;
	SINGLE half_z;

//	Matrix R;

	Box(void)
	{
		half_x = half_y = half_z = 1.0;
	}

	float volume(void) const
	{
		return 4.0 * (half_x*half_x + half_y*half_y + half_z*half_z);
	}
};

//
// Tube - cylinder with rounded ends. Same description, treated differently.
//
struct Tube : public Cylinder
{
	Tube(void) : Cylinder()
	{
	}

	Tube(float len, float rad) : Cylinder(len, rad)
	{
	}
};

//
// Axis-aligned ellipsoid, with semi-axis lengths rx, ry, rz.
//
struct Ellipsoid : public GeometricPrimitive
{
	float rx, ry, rz;

	Ellipsoid(void)
	{
		rx = ry = rz = 1.0;
	}

	Ellipsoid(float x, float y, float z)
	{
		rx = x;
		ry = y;
		rz = z;
	}
};

//

#endif