#ifndef EXTENT_H
#define EXTENT_H

//

#include "3dmath.h"
#include "geom.h"
#include "cmesh.h"

//
/*
	Horizontal connection means group of separate extents.
	Vertical connection means parent extent fully contains child extent.
	(Or at least parent extent fully contains child's geometry).

	Top level extent should probably always be an all-encompassing sphere that
	contains whatever complicated group of extents underneath.

	A "cylinder" has flat ends, i.e. cut by planes whose normals are parallel
	to the cylinder's axis.

	A "tube" is a cylinder with rounded (hemisphere) ends. Dealing with tubes
	can be SIGNIFICANTLY CHEAPER than cylinders. THE USER IS STRONGLY ENCOURAGED 
	TO USE TUBES INSTEAD OF CYLINDERS WHEREVER PRACTICAL.
*/
//

//
// WARNING: LINE_SEGMENT EXTENTS ARE NO LONGER SUPPORTED. USE VERY THIN TUBES OR RAYS INSTEAD.
//

typedef enum
{
	ET_LINE_SEGMENT,		
	ET_INFINITE_PLANE,
	ET_SPHERE,				// Fastest volume primitive.
	ET_CYLINDER,			// Cylinder with flat ends.
	ET_BOX,					// Often best balance of speed & accuracy.
	ET_CONVEX_MESH,			// Still reasonably cheap.
	ET_GENERAL_MESH,		// Slowest, since no assumptions can be made.
	ET_TUBE,				// A TUBE is a cylinder with spherical ends (rather than flat).
	ET_NONE
} ExtentType;

//

struct BaseExtent
{
	char *		name;
	ExtentType	type;
	Transform	xform;		// Each extent's transform is relative to the OBJECT'S CENTER OF MASS FRAME,
							// regardless of its depth in the hierarchy.
	BaseExtent *next;
	BaseExtent *child;

	BaseExtent(void)
	{
		name = NULL;
		type = ET_NONE;
		next = child = NULL;
	}

	inline bool is_leaf(void) const
	{
		return (child == NULL);
	}

	virtual ~BaseExtent(void) 
	{
		if( name ) {
			free(name);
			name = NULL;
		}
	}
	virtual const GeometricPrimitive * get_primitive(void) const = 0;
};

//

struct LineSegmentExtent : public BaseExtent
{
	LineSegment	segment;

	LineSegmentExtent(void)
	{
		type = ET_LINE_SEGMENT;
	}

	LineSegmentExtent(const LineSegment & _segment)
	{
		type = ET_LINE_SEGMENT;
		segment = _segment;
	}

	virtual const GeometricPrimitive * get_primitive(void) const
	{
		return &segment;
	}
};

//

struct PlaneExtent : public BaseExtent
{
	Plane plane;

	PlaneExtent(void)
	{
		type = ET_INFINITE_PLANE;
	}

	PlaneExtent(const Plane & _plane)
	{
		type = ET_INFINITE_PLANE;
		plane = _plane;
	}

	virtual const GeometricPrimitive * get_primitive(void) const
	{
		return &plane;
	}
};

//

struct SphereExtent : public BaseExtent
{
	Sphere sphere;

	SphereExtent(void)
	{
		type = ET_SPHERE;
	}

	SphereExtent(const Sphere & _sphere)
	{
		type = ET_SPHERE;
		sphere = _sphere;
	}

	virtual const GeometricPrimitive * get_primitive(void) const
	{
		return &sphere;
	}
};

//

struct CylinderExtent : public BaseExtent
{
	Cylinder cylinder;

	CylinderExtent(void)
	{
		type = ET_CYLINDER;
	}

	CylinderExtent(const Cylinder & _cylinder)
	{
		type = ET_CYLINDER;
		cylinder = _cylinder;
	}

	virtual const GeometricPrimitive * get_primitive(void) const
	{
		return &cylinder;
	}
};

//

struct BoxExtent : public BaseExtent
{
	Box box;

	BoxExtent(void)
	{
		type = ET_BOX;
	}

	BoxExtent(const Box & _box)
	{
		type = ET_BOX;
		box = _box;
	}

	virtual const GeometricPrimitive * get_primitive(void) const
	{
		return &box;
	}
};

//

struct ConvexMeshExtent : public BaseExtent
{
	CollisionMesh * mesh;

	ConvexMeshExtent(void)
	{
		type = ET_CONVEX_MESH;
	}

	ConvexMeshExtent(CollisionMesh * _mesh)
	{
		type = ET_CONVEX_MESH;
		mesh = _mesh;
	}

	virtual ~ConvexMeshExtent(void)
	{
		delete mesh;
	}

	virtual const GeometricPrimitive * get_primitive(void) const
	{
		return mesh;
	}
};

//

struct MeshExtent : public BaseExtent
{
	CollisionMesh * mesh;

	MeshExtent(void)
	{
		type = ET_GENERAL_MESH;
	}

	MeshExtent(CollisionMesh * _mesh)
	{
		type = ET_GENERAL_MESH;
		mesh = _mesh;
	}

	virtual ~MeshExtent(void)
	{
		delete mesh;
	}

	virtual const GeometricPrimitive * get_primitive(void) const
	{
		return mesh;
	}
};

//

struct TubeExtent : public BaseExtent
{
	Tube tube;

	TubeExtent(void)
	{
		type = ET_TUBE;
	}

	TubeExtent(const Tube & _tube)
	{
		type = ET_TUBE;
		tube = _tube;
	}

	virtual const GeometricPrimitive * get_primitive(void) const
	{
		return &tube;
	}
};

//
/* ADD LATER.
struct EllipsoidExtent : public BaseExtent
{
	Ellipsoid ell;

	EllipsoidExtent(void)
	{
		type = ET_ELLIPSOID;
	}

	EllipsoidExtent(const Ellipsoid & _ell)
	{
		type = ET_ELLIPSOID;
		ell = _ell;
	}

	virtual const GeometricPrimitive * get_primitive(void) const
	{
		return &ell;
	}
};
*/
//

#endif