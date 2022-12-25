#ifndef XPRIM_H
#define XPRIM_H

//

#include "geom.h"

//

struct XForm
{
	Vector	x;
	Matrix	R;

	XForm(void)
	{
	}

	XForm(const Vector & _x, const Matrix & _R)
	{
		x = _x;
		R = _R;
	}

	XForm(const Transform & T)
	{
		R = T;
		x = T.translation;
	}

	inline Vector transform(const Vector & v) const
	{
		return x + R * v;
	}

	inline Vector rotate(const Vector & v) const
	{
		return R * v;
	}
};

//

struct XLineSegment : public LineSegment
{
	XLineSegment(const LineSegment & seg, const XForm & xform)
	{
		p0 = xform.transform(seg.p0);
		p1 = xform.transform(seg.p1);
	}

	XLineSegment(const LineSegment & seg, const Transform & T)
	{
		p0 = T.rotate_translate(seg.p0);
		p1 = T.rotate_translate(seg.p1);
	}
};

struct XPlane : public Plane
{
	Vector center;

	XPlane(const Plane & plane, const XForm & xform)
	{
		center = xform.x;
		N = xform.rotate(plane.N);
		D = -dot_product(N, center);
	}

	XPlane(const Plane & plane, const Transform & T)
	{
		center = T.translation;
		N = T.rotate(plane.N);
		D = -dot_product(N, center);
	}
};

struct XSphere : public Sphere
{
	Vector center;

	XSphere(void) : Sphere()
	{
	}

	XSphere(const Sphere & sphere, const XForm & xform) : Sphere(sphere)
	{
		center = xform.x;
	}

	XSphere(const Sphere & sphere, const Transform & T) : Sphere(sphere)
	{
		center = T.translation;
	}
};

struct XCylinder : public Cylinder
{
	Vector center;
	Vector axis;

	XCylinder(void)
	{
	}

	XCylinder(const Cylinder & cyl, const XForm & xform) : Cylinder(cyl)
	{
		center = xform.x;
		axis = xform.R.get_k();
	}

	XCylinder(const Cylinder & cyl, const Transform & T) : Cylinder(cyl)
	{
		center = T.translation;
		axis = T.get_k();
	}
};

struct XBox : public Box
{
	Vector center;
	Matrix R;

	XBox(const Box & box, const XForm & xform) : Box(box)
	{
		center = xform.x;
		R = xform.R;
	}

	XBox(const Box & box, const Transform & T) : Box(box)
	{
		center = T.translation;
		R = (Matrix) T;
	}
};

//

struct XTube : public Tube
{
	Vector center;
	Vector axis;

	XTube(const Tube & tube, const XForm & xform) : Tube(tube)
	{
		center = xform.x;
		axis = xform.R.get_k();
	}

	XTube(const Tube & tube, const Transform & T) : Tube(tube)
	{
		center = T.translation;
		axis = T.get_k();
	}
};

//

#endif