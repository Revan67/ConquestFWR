//
//
//

#include <float.h>

#include "collision.h"
#include "rigid.h"
#include "XPrim.h"
#include "Tfuncs.h"

//

static float tolerance = 1e-5f;

//

static float square(float n)
{
	return n * n;
}

//

bool RayLineSegment(float & t, Vector & p, Vector & normal, const Vector & base, const Vector & dir, const LineSegment & line)
{
	return false;
}

//

bool RayPlane(float & t, Vector & p, Vector & normal, const Vector & base, const Vector & dir, const Plane & plane)
{
	bool result;

	float denom = dot_product(dir, plane.N);
	if (fabs(denom) >= tolerance)
	{
		float num = -plane.compute_distance(base);
		t = num / denom;
		if (t >= 0.0f)
		{
			result = true;
			p = base + t * dir;
			normal = plane.N;
		}
		else
		{
		// Ray intersects plane behind origin.
			result = false;
		}
	}
	else
	{
	// Ray is parallel to plane.
		result = false;
	}

	return result;
}

//

bool RaySphere(float & t, Vector & p, Vector & N, const Vector & base, const Vector & dir, const XSphere & sphere)
{
	bool result;

	Vector dx = base - sphere.center;

	float r_squared = square(sphere.radius);

	float a = dot_product(dir, dir);
	float b = 2.0f * dot_product(dir, dx);
	float c = dot_product(dx, dx) - r_squared;
	float disc = square(b) - 4.0f * a * c;
	if (disc >= 0)
	{
		float sqrt_disc = sqrt(disc);
		float one_over_2a = 0.5f * 1.0f / a;
		float t0 = (-b - sqrt_disc) * one_over_2a;
		float t1 = (-b + sqrt_disc) * one_over_2a;

	// Need smaller non-negative root.
		if (t0 >= 0.0f)
		{
			if (t1 >= 0.0f)
			{
				t = Tmin(t0, t1);
			}
			else
			{
				t = t0;
			}
		}
		else if (t1 >= 0.0f)
		{
			t = t1;
		}
		else 
		{
			t = -1;
		}

		if (t >= 0.0f)
		{
			result = true;
			p = base + t * dir;
			N = p - sphere.center;
			N.normalize();
		}
		else
		{
			result = false;
		}
	}
	else
	{
	// No real roots, no intersection with sphere surface.
		result = false;
	}

	return result;
}

//

#define		SIDE	0		/* Object surface		*/
#define		BOT	1		/* Bottom end-cap surface	*/
#define		TOP	2		/* Top	  end-cap surface	*/

//

static bool ClipObj(const Vector & base, const Vector & dir, const Plane & bottom, const Plane & top, float & obj_in, float & obj_out, int & surf_in, int & surf_out)
{
	surf_in = surf_out = SIDE;
	float in  = obj_in;
	float out = obj_out;

// Intersect the ray with the bottom end-cap plane.

	float dc = dot_product(bottom.N, dir);
	float dw = bottom.compute_distance(base);

	if (fabs(dc) < tolerance) 
	{		
	// If parallel to bottom plane.
	    if (dw >= 0) 
		{
			return false;
		}		
	} 
	else 
	{
		float t  = - dw / dc;
		if (dc >= 0.0f) 
		{			    /* If far plane	*/
			if (t > in && t < out) 
			{ 
				out = t; 
				surf_out = BOT; 
			}
			if (t < in) 
			{
				return false;
			}
		} 
		else 
		{				    /* If near plane	*/
			if  (t > in && t < out)
			{
				in = t; 
				surf_in = BOT; 
			}
			if (t > out) 
			{
				return false;
			}
		}
	}

/*	Intersect the ray with the top end-cap plane.			*/

	dc = dot_product(top.N, dir);
	dw = top.compute_distance(base);

	if  (fabs(dc) < tolerance) 
	{		/* If parallel to top plane	*/
		if (dw >= 0.0f) 
		{
			return false;
		}
	} 
	else 
	{
	    float t = - dw / dc;
		if (dc >= 0.0f) 
		{			    /* If far plane	*/
			if  (t > in && t < out) 
			{
				out = t; 
				surf_out = TOP; 
			}
			if (t < in) 
			{
				return false;
			}
		} 
		else 
		{				    /* If near plane	*/
			if (t > in && t < out) 
			{ 
				in = t; 
				surf_in  = TOP; 
			}
			if (t > out) 
			{
				return false;
			}
		}
	}

	obj_in	= in;
	obj_out = out;

	return (in < out);
}


bool RayCylinder(float & t, Vector & p, Vector & N, const Vector & base, const Vector & dir, const XCylinder & cyl)
{
	bool result;

	float llen = dir.magnitude();
	Vector ldir = dir / llen;

	float half_len = 0.5f * cyl.length;

	Vector cbase = cyl.center - half_len * cyl.axis;
	Vector cdir = cyl.axis;

	Vector rel = base - cbase;

// Get vector perpendicular to both line and cylinder axis.
	Vector perp = cross_product(ldir, cdir);
	float pmag = perp.magnitude();

	float t_in, t_out;

// If magnitude == 0, line is parallel to cylinder axis.
	if (pmag < tolerance)
	{
	// line is parallel to cylinder, check distance.
		float d = dot_product(rel, cyl.axis);
		Vector D = rel - d * cyl.axis;
		d = D.magnitude();
		if (d <= cyl.radius)
		{
			result = true;
			t_in = -FLT_MAX;
			t_out = FLT_MAX;
		}
		else
		{
			result = false;
		}
	}
	else
	{
	// Not parallel. Normalize perp.
		perp /= pmag;

	// Compute distance of closest approach.
		float d = fabs(dot_product(rel, perp));
		if (d <= cyl.radius)
		{
		// Closest approach is within cylinder. Now compute vector
		// perpendicular to cylinder axis and perp.

			Vector o = cross_product(rel, cyl.axis);
			t = -dot_product(o, perp) / pmag;

			o = cross_product(perp, cyl.axis);
			o.normalize();
			float s = fabs(sqrt(square(cyl.radius) - square(d)) / dot_product(ldir, o));
			t_in = t - s; 
			t_out = t + s;

			result = true;
		}
		else
		{
			result = false;
		}
	}

	if (result)
	{
	// Compute endcap planes.
		Plane p0, p1;
		Vector half_axis = cyl.axis * half_len;
		Vector e0 = cyl.center - half_axis;
		p0.N = -cyl.axis;
		p0.D = -dot_product(p0.N, e0);

		Vector e1 = cyl.center + half_axis;
		p1.N = cyl.axis;
		p1.D = -dot_product(p1.N, e1);

		int in, out;
		if (ClipObj(base, ldir, p0, p1, t_in, t_out, in, out))
		{
			if (t_in >= 0.0f)
			{
				if (t_out >= 0.0f)
				{
					t = Tmin(t_in, t_out);
				}
				else
				{
					t = t_in;
				}
			}
			else if (t_out >= 0.0f)
			{
				t = t_out;
			}
			else 
			{
				t = -1;
			}

			t /= llen;
			if (t >= 0.0f)
			{
				result = true;
				p = base + t * dir;

			// compute normal:
				Vector dp = p - cbase;
				float d = dot_product(dp, cyl.axis);
				Vector paxis = cbase + d * cyl.axis;
				dp = p - paxis;
				float drad = dp.magnitude();
				if (fabs(drad - cyl.radius) < 1e-3)
				{
					N = dp;
					N.normalize();
				}
				else if (fabs(d) < 1e-3)
				{
					N = -cyl.axis;
				}
				else if (fabs(d - cyl.length) < 1e-3)
				{
					N = cyl.axis;
				}
			}
			else
			{
				result = false;
			}
		}
		else
		{
			result = false;
		}
	}

	return result;
}

//

#define RIGHT	0
#define LEFT	1
#define MIDDLE	2

//

bool RayBox(float & t, Vector & p, const Vector & base, const Vector & dir, const Box & box, Vector & normal)
{
	bool result;

	bool inside = true;
	int quadrant[3];
	int whichPlane;

	float maxT[3];
	float candidatePlane[3];

	float origin[3] = {base.x, base.y, base.z};
	float direction[3] = {dir.x, dir.y, dir.z};
	float minB[3] = {-box.half_x, -box.half_y, -box.half_z};
	float maxB[3] = { box.half_x,  box.half_y,  box.half_z};

	for (int i = 0; i < 3; i++)
	{
		if (origin[i] < minB[i]) 
		{  
			quadrant[i] = LEFT;
			candidatePlane[i] = minB[i];
			inside = FALSE;
		}
		else if (origin[i] > maxB[i]) 
		{
			quadrant[i] = RIGHT;
			candidatePlane[i] = maxB[i];
			inside = FALSE;
		}
		else	
		{
			quadrant[i] = MIDDLE;
		}
	}

// Ray origin inside bounding box
	if (inside)	
	{
		p = base;
		normal.zero();	// normal is meaningless.
		result = true;
	}
	else
	{
	// Calculate T distances to candidate planes
		for (i = 0; i < 3; i++)
		{
			if (quadrant[i] != MIDDLE && fabs(direction[i]) > tolerance)
			{
				maxT[i] = (candidatePlane[i] - origin[i]) / direction[i];
			}
			else
			{
				maxT[i] = -1.0f;
			}
		}

	// Get largest of the maxT's for final choice of intersection
		whichPlane = 0;
		for (i = 1; i < 3; i++)
		{
			if (maxT[whichPlane] < maxT[i])
			{
				whichPlane = i;
			}
		}

	// Check final candidate actually inside box
		if (maxT[whichPlane] < 0)
		{
			result = false;
		}
		else
		{
			t = maxT[whichPlane];

			float coord[3];

			for (i = 0; i < 3; i++)
			{
				if (whichPlane != i) 
				{
					coord[i] = origin[i] + maxT[whichPlane] * direction[i];
					if (coord[i] < minB[i] || coord[i] > maxB[i])
					{
						result = false;
						break;
					}
				}
				else 
				{
					coord[i] = candidatePlane[i];
					result = true;
				}
			}

			if (result)
			{
				p.set(coord[0], coord[1], coord[2]);
				normal.zero();
				switch (whichPlane)
				{
				case 0:
					normal.x = (coord[0] > 0) ? 1.0f : -1.0f;
					break;
				case 1:	 
					normal.y = (coord[1] > 0) ? 1.0f : -1.0f;
					break;
				case 2:
					normal.z = (coord[2] > 0) ? 1.0f : -1.0f;
					break;
				}
			}
		}
	}

	return result;
}

//

bool RayConvexMesh(float & t, Vector & p, Vector & N, const Vector & base, const Vector & dir, const CollisionMesh & mesh)
{
	bool result;

	if (RigidBodyPhysics::buffer_length < mesh.num_normals)
	{
		if (RigidBodyPhysics::buffer1)
		{
			delete [] RigidBodyPhysics::buffer1;
		}
		if (RigidBodyPhysics::buffer2)
		{
			delete [] RigidBodyPhysics::buffer2;
		}

		RigidBodyPhysics::buffer_length = mesh.num_normals;
		RigidBodyPhysics::buffer1 = new float[mesh.num_normals];
		RigidBodyPhysics::buffer2 = new float[mesh.num_normals];
	}

	float llen = dir.magnitude();
	Vector d = dir / llen;

	float tnear = -FLT_MAX;
	float tfar = FLT_MAX;
	int front_normal, back_normal;

	result = true;

	Vector * n = mesh.normals;
	for (int i = 0; i < mesh.num_normals; i++, n++)
	{
		RigidBodyPhysics::buffer1[i] = dot_product(d, *n);
		RigidBodyPhysics::buffer2[i] = dot_product(base, *n);
	}

	Triangle * tri = mesh.triangles;
	float * D = mesh.triangle_d;
	for (i = 0; i < mesh.num_triangles; i++, tri++, D++)
	{
	// vd = ray_direction dot plane_normal.
		float vd = RigidBodyPhysics::buffer1[tri->normal];

	// vn = distance of ray_origin from plane.
		float vn = RigidBodyPhysics::buffer2[tri->normal] + *D;

		if (fabs(vd) < tolerance)
		{
		// Ray is parallel to plane.
			if (vn > 0.0f)
			{
			// Ray is outside plane & parallel to plane, can't intersect object.
				result = false;
				break;
			}
		}
		else
		{
		// Ray isn't parallel to plane.

		// Find distance along ray to intersection with plane.
			t = -vn / vd;

			if (vd < 0.0f)
			{
			// Plane faces away from ray.
				if (t > tfar)
				{
					result = false;
					break;
				}

				if (t > tnear)
				{
					front_normal = tri->normal;
					tnear = t;
				}
			}
			else
			{
			// Plane faces ray.

				if (t < tnear)
				{
					result = false;
					break;
				}

				if (t < tfar)
				{
					back_normal = tri->normal;
					tfar = t;
				}
			}
		}
	}


	if (result)
	{
		if (tnear > 0.0f)
		{
			t = tnear;
			p = base + d * t;
			N = mesh.normals[front_normal];
		}
		else 
		{
			if (tfar > 0.0f && tfar < FLT_MAX)
			{
				t = tfar;
				p = base + d * t;
				N = mesh.normals[back_normal];
			}
			else
			{
				result = false;
			}
		}
	}

	return result;
}

//

bool RayTrilist(float & t, Vector & p, Vector & N, const Vector & base, const Vector & dir, const CollisionMesh & mesh)
{
	bool result=false;

	float tmin = FLT_MAX;

	Triangle * tri = mesh.triangles;
	for (int i = 0; i < mesh.num_triangles; i++, tri++)
	{
		Vector * v0 = &mesh.vertices[tri->v[0]].p;
		Vector * v1 = &mesh.vertices[tri->v[1]].p;
		Vector * v2 = &mesh.vertices[tri->v[2]].p;

		Vector edge1 = *v1 - *v0;
		Vector edge2 = *v2 - *v0;

		Vector pvec = cross_product(dir, edge2);
		float det = dot_product(edge1, pvec);
		if (det < tolerance)
		{
		//	result = false;
		}
		else
		{
			Vector tvec = base - *v0;
			float u = dot_product(tvec, pvec);
			if (u < 0.0f || u > det)
			{
			//	result = false;
			}
			else
			{
				Vector qvec = cross_product(tvec, edge1);
				float v = dot_product(dir, qvec);
				if (v < 0.0f || (u+v) > det)
				{
				//	result = false;
				}
				else
				{
					result = true;
					t = dot_product(edge2, qvec) / det;
					if (t < tmin)
					{
						tmin = t;
						p = base + t * dir;					
						N = mesh.normals[tri->normal];
					}
				}
			}
		}
	}

	t = tmin;

	return result;
}

//

// QUICK HACK, CLEAN UP LATER.
bool RayTube(float & t, Vector & p, Vector & N, const Vector & base, const Vector & dir, const XTube & tube)
{
	bool result = false;

	float	t0 = FLT_MAX, t1 = FLT_MAX, t2 = FLT_MAX;
	Vector	p0, p1, p2;
	Vector	N0, N1, N2;

	XCylinder cyl;
	memcpy(&cyl, &tube, sizeof(XTube));
	bool r0 = RayCylinder(t0, p0, N0, base, dir, cyl);

	Vector half_axis = tube.axis * 0.5f * tube.length;
	Vector te = tube.center - half_axis;
	Sphere s;
	s.radius = tube.radius;
	Transform T; T.set_identity(); T.set_position(te);
	XSphere sphere(s, T);
	bool r1 = RaySphere(t1, p1, N1, base, dir, sphere);

	te = tube.center + half_axis;
	sphere.center = te;
	bool r2 = RaySphere(t2, p2, N2, base, dir, sphere);

	if (r0 || r1 || r2)
	{
		result = true;

		float tmin = Tmin(Tmin(t0, t1), t2);
		if (tmin == t0)
		{
			t = t0;
			p = p0;
			N = N0;
		}
		else if (tmin == t1)
		{
			t = t1;
			p = p1;
			N = N1;
		}
		else if (tmin == t2)
		{
			t = t2;
			p = p2;
			N = N2;
		}
	}

	return result;
}

//

void TransformRayToObjectSpace(Vector & xorg, Vector & xdir, const Vector & org, const Vector & dir, const Transform & T)
{
	const Matrix & R = (Matrix) T;

	//Matrix RT = R.get_transpose();
	xorg = (org - T.translation) * R;		// multiply V * M is the same as M.T * V. 
	xdir = dir * R;
}

//

static bool internal_check_extent(float & t, Vector & point, Vector & normal, 
						   const Vector & origin, const Vector & direction,
						   const BaseExtent * x, const Transform & T)
{
	ASSERT(x);

	bool result = false;

	if (x->type >= ET_LINE_SEGMENT && x->type <= ET_TUBE)
	{
		const GeometricPrimitive * gp = x->get_primitive();

	// Take extent's transform into account:
		Transform X = T.multiply(x->xform);

		switch (x->type)
		{
			case ET_LINE_SEGMENT:
			{
				XLineSegment line(*((LineSegment *) gp), X);
				result = RayLineSegment(t, point, normal, origin, direction, line);
				break;
			}
			case ET_INFINITE_PLANE:
			{
				XPlane plane(*((Plane *) gp), X);
				result = RayPlane(t, point, normal, origin, direction, plane);
				break;
			}
			case ET_SPHERE:
			{
				XSphere sphere(*((Sphere *) gp), X);
				result = RaySphere(t, point, normal, origin, direction, sphere);
				break;
			}
			case ET_CYLINDER:
			{
				XCylinder cyl(*((Cylinder *) gp), X);
				result = RayCylinder(t, point, normal, origin, direction, cyl);
				break;
			}
			case ET_BOX:
			{
				Vector org, dir;
				TransformRayToObjectSpace(org, dir, origin, direction, X);

				result = RayBox(t, point, org, dir, *((Box *) gp), normal);
				if (result)
				{
					point = X.rotate_translate(point);
					normal = X.rotate(normal);
				}
				break;
			}
			case ET_CONVEX_MESH:
			{
				Vector org, dir;
				TransformRayToObjectSpace(org, dir, origin, direction, X);

				result = RayConvexMesh(t, point, normal, org, dir, (CollisionMesh &) *gp);
				if (result)
				{
					point = X.rotate_translate(point);
					normal = X.rotate(normal);
				}
				break;
			}
			case ET_GENERAL_MESH:
			{
				Vector org, dir;
				TransformRayToObjectSpace(org, dir, origin, direction, X);

				result = RayTrilist(t, point, normal, org, dir, (CollisionMesh &) *gp);
				if (result)
				{
					point = X.rotate_translate(point);
					normal = X.rotate(normal);
				}
				break;
			}
			case ET_TUBE:
			{
				XTube tube(*((Tube *) gp), X);
				result = RayTube(t, point, normal, origin, direction, tube);
				break;
			}
		}
	}
	else	
	{
		result = false;
	}

	return result;
}

//

bool COMAPI RigidBodyPhysics::intersect_ray_with_extent(Vector & point, Vector & normal, const Vector & origin, const Vector & direction, const BaseExtent & extent, const Transform & T)
{
	float t;
	bool result = internal_check_extent(t, point, normal, origin, direction, &extent, T);
	return result;
}

//

static bool internal_check_hierarchy(float & t, Vector & point, Vector & normal, 
							  const Vector & org, const Vector & dir, 
							  const BaseExtent * root, const Transform & T)
{
	ASSERT(root);
	bool result = false;

	float tmin = FLT_MAX;
	Vector pmin, Nmin;

	const BaseExtent * x = root;
	while (x)
	{
		if (internal_check_extent(t, point, normal, org, dir, x, T))
		{
			bool hit = false;

			if (x->child)
			{
				hit = internal_check_hierarchy(t, point, normal, org, dir, x->child, T);
			}
			else
			{
			// this is a leaf.
				hit = true;
			}

			if (hit && (t < tmin))
			{
				tmin = t;
				pmin = point;
				Nmin = normal;

				result = true;
			}
		}
		
		x = x->next;
	}

	if (result)
	{
		t = tmin;
		point = pmin;
		normal = Nmin;
	}

	return result;
}

//

bool COMAPI RigidBodyPhysics::intersect_ray_with_extent_hierarchy(	Vector & point, Vector & normal,
																	const Vector & origin, const Vector & direction, 
																	const BaseExtent & extent, 
																	const Transform & T, bool find_closest)
{
	bool result = false;

	float t;

	if (find_closest)
	{
		result = internal_check_hierarchy(t, point, normal, origin, direction, &extent, T);
	}
	else
	{
		const BaseExtent * x = &extent;

		while (!result && x)
		{
			result = internal_check_extent(t, point, normal, origin, direction, x, T);

			if (result && x->child)
			{
				result = intersect_ray_with_extent_hierarchy(point, normal, origin, direction, *x->child, T, false);
			}

			x = x->next;
		}
	}

	return result;
}

//
