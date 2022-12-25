#include "PCH.h"
#include "RayMeshCollision.h"

// collide_ray_with_mesh:
//
//  ray_start		- start of ray in world space
//  ray_direction		- direction of ray in world space
//  position		- position of object in world space
//  orientation	 	- orientation of object in world space
//  mesh			- instance's mesh
//  plane			- optional parameters that will pass you back the plane of collision,
//  mesh			- ... the index of the polygon in the mesh that intersected
//  point			- ... and the point of collision

#define EPSILON 0.000001

// intersect_triangle stolen and ported from: http://www.acm.org/jgt/papers/MollerTrumbore97/
//
// t = distance from plane of triangle to ray origin
// u, v = position inside triangle
//

BOOL32 intersect_triangle(Vector & orig, Vector & dir, Vector & vert0, Vector & vert1, Vector & vert2, SINGLE * t, SINGLE * u, SINGLE * v)
{
	Vector edge1, edge2, tvec, pvec, qvec;
	SINGLE det, inv_det;

	/* find vectors for two edges sharing vert0 */

	edge1 = vert1 - vert0;
	edge2 = vert2 - vert0;

	pvec = cross_product(dir, edge2);	// begin calculating determinant - also used to calculate U parameter
	det = dot_product(edge1, pvec);		// if determinant is near zero, ray lies in plane of triangle

	if (det > -EPSILON && det < EPSILON)
	 return FALSE;

	inv_det = 1.0 / det;
	tvec = orig - vert0;				 // calculate distance from vert0 to ray origin

	*u = dot_product(tvec, pvec) * inv_det;

	if (*u < 0.0 || *u > 1.0)
	 return FALSE;

	qvec = cross_product(tvec, edge1);		// prepare to test V parameter

	*v = dot_product(dir, qvec) * inv_det;	// calculate V parameter and test bounds

	if (*v < 0.0 || *u + *v > 1.0)
	 return FALSE;

	*t = dot_product(edge2, qvec) * inv_det; // calculate t, ray intersects triangle

	return TRUE;
}

BOOL32 collide_ray_with_mesh(const Vector& ray_start, const Vector& ray_direction, const Vector& position, const Matrix& orientation, const Mesh* mesh, Plane* plane, S32* mesh_poly_index, SINGLE* distance)
{
	if (mesh == NULL)
		__asm int 0x03;

	static Vector vp[3];
	static SINGLE t, u, v;

	BOOL32 intersect = FALSE;

	SINGLE dist = 100000.0;

	// transform ray to object space
	Matrix RT		= orientation.get_transpose();
	Vector r_origin = RT * (ray_start - position);
	Vector r_dir	= RT * ray_direction;

	for (int i = 0; i < mesh->face_cnt; i++)
	{
		vp[0] = *mesh->get_face_vertex(i, 0);
		vp[1] = *mesh->get_face_vertex(i, 1);
		vp[2] = *mesh->get_face_vertex(i, 2);

		if (intersect_triangle(r_origin, r_dir, vp[0], vp[1], vp[2], &t, &u, &v))
		{
			if (t < dist)
			{
				dist = t;

				if (distance)
					*distance = t;

				if (mesh_poly_index)
					*mesh_poly_index = i;

				if (plane)
				{
					plane->init(position + (orientation * vp[2]), position + (orientation * vp[1]), position + (orientation * vp[0]));
				}

			}

			intersect = TRUE;

		}

	}


	return intersect;
}
