//

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <float.h>
#include <stdio.h>
#include "vbox.h"
#include "stddat.h"
#include "Debugprint.h"
#include "XPrim.h"
#include "VMesh.h"
#include "fdump.h"

//

void HackDrawPoint(const Vector & pt, int r, int g, int b, float radius);

//

#define CONTINUE     0
#define DISJOINT     1
#define PENETRATION -1

//

//static CollisionMesh * mesh1 = NULL;
//static CollisionMesh * mesh2 = NULL;

//

int vertex_vertex(Feature *& f1, const Vector & r1, const Matrix & R1, Feature *& f2, const Vector & r2, const Matrix & R2, Vector & p1, Vector & p2)
{
	ASSERT(f1);
	ASSERT(f2);

	VVertex * v1 = (VVertex *) f1;
	VVertex * v2 = (VVertex *) f2;

	Vector xv2 = r2 + R2 * v2->p;

	VPlane * p = f1->planes; 
	for (int i = 0; i < f1->num_planes; i++, p++)
	{
		float d = p->compute_distance(xv2);
		if (d < 0.0f)
		{
			f1 = p->neighbor;
			return CONTINUE;
		}
	}

	Vector xv1 = r1 + R1 * v1->p;
	p = f2->planes;
	for (i = 0; i < f2->num_planes; i++, p++)
	{
		float d = p->compute_distance(xv1);
		if (d < 0.0f)
		{
			f2 = p->neighbor;
			return CONTINUE;
		}
	}

	p1 = v1->p;
	p2 = v2->p;
	Vector dp = p1 - xv2;
	float dist = dp.magnitude();

	if (dist > 0.0f)
	{
		return DISJOINT;
	}
	else 
	{
		return PENETRATION;
	}
}

//

int vertex_edge(Feature *& f1, const Vector & r1, const Matrix & R1, Feature *& f2, const Vector & r2, const Matrix & R2, Vector & p1, Vector & p2)
{
	ASSERT(f1);
	ASSERT(f2);

	VVertex * v = (VVertex *) f1;
	VEdge * e = (VEdge *) f2;

	Vector xv = r1 + R1 * v->p;
	VPlane * p = f2->planes;
	for (int i = 0; i < f2->num_planes; i++, p++)
	{
		float d = p->compute_distance(xv);
		if (d < 0.0f)
		{
			f2 = p->neighbor;
			return CONTINUE;
		}
	}

	Vector t = r2 + R2 * e->get_v0();
	Vector h = r2 + R2 * e->get_v1();

// Clip edge against f1.
	float l0 = 0;
	float l1 = 1;
	Feature * n0 = NULL;
	Feature * n1 = NULL;

	p = f1->planes;
	for (i = 0; i < f1->num_planes; i++, p++)
	{
		Feature * n = p->neighbor;
		float dt = p->compute_distance(t);
		float dh = p->compute_distance(h);

		if (dt >= 0) 
		{
			if (dh >= 0.0f) continue;
			float lambda = dt / (dt - dh);
			if (lambda < l1)
			{
				l1 = lambda; 
				n1 = n; 
				if (l1 < l0) 
				{
					break;
				}
			}
		}
		else 
		{ 
		// dt < 0
			if (dh < 0.0f) 
			{
				n0 = n1 = n;
				break;
			}
			float lambda = dt / (dt - dh);
			if (lambda > l0)
			{
				l0 = lambda; 
				n0 = n; 
				if (l0 > l1) 
				{
					break;
				}
			}
		}
	}

	if (i != f1->num_planes && (n0 == n1))
	{
		f1 = n0;
		return CONTINUE;
	}
	else
	{
		if (n0 != NULL)
		{
			Vector el0 = t + l0 * (h - t);
			Vector d = el0 - v->p;
			float dmag = d.magnitude();
			if (dmag == 0.0f)
			{
				return PENETRATION;
			}
			if (dot_product(d, h - t) > 0.0f)
			{
				f1 = n0;
				return CONTINUE;
			}
		}
		if (n1 != NULL)
		{
			Vector el1 = t + l1 * (h - t);
			Vector d = el1 - v->p;
			float dmag = d.magnitude();
			if (dmag == 0.0f)
			{
				return PENETRATION;
			}
			if (dot_product(d, h - t) < 0.0f)
			{
				f1 = n1;
				return CONTINUE;
			}
		}
	}

	p1 = v->p;				// in R1
	h = xv - e->get_v0();	// in R2.
	Vector dir = e->get_v1() - e->get_v0();	// in R2.
	dir.normalize();

	p2 = e->get_v0() + dot_product(h, dir) * dir;	// in R2.
	Vector dp = p2 - xv;
	float dist = dp.magnitude();
	if (dist > 0.0f)
	{
		return DISJOINT;
	}
	else
	{
		return PENETRATION;
	}
}

//

int vertex_face(Feature *& f1, const Vector & r1, const Matrix & R1, Feature *& f2, const Vector & r2, const Matrix & R2, VMesh & box2, Vector & p1, Vector & p2)
{
	ASSERT(f1);
	ASSERT(f2);

	VVertex * v = (VVertex *) f1;
	VFace * f = (VFace *) f2;

	Vector xv = r1 + R1 * v->p;

	VPlane * max_violated_plane = NULL;

// search only edge-face planes in this first loop, not face's supporting plane.
	VPlane * p = f2->planes+1;
	float dmin = 0;
	for (int i = 1; i < f2->num_planes; i++, p++)
	{
		float d = p->compute_distance(xv);
		if (d < dmin)
		{
			dmin = d;
			max_violated_plane = p;
		}
	}

	if (max_violated_plane)
	{
		f2 = max_violated_plane->neighbor;
		return CONTINUE;
	}


// supporting plane for face.
	p = f2->planes;
	float d = p->compute_distance(xv);
	if (d == 0.0f)
	{
		p1 = v->p;	// in R1
		p2 = xv;	// in R2
		return PENETRATION;
	}

//
// Vertex is completely within face's voronoi region.
// See if any edges containing vertex point toward face.
//
	p = f1->planes;
	for (i = 0; i < f1->num_planes; i++, p++)
	{
		VEdge * e = (VEdge *) p->neighbor;

		Vector other;
		if (e->v[0] == f1->id)
		{
			other = r1 + R1 * e->get_v1();
		}
		else 
		{
			other = r1 + R1 * e->get_v0();
		}

	//
	// "d" is distance from vertex, check distance from the edge's other
	// vertex.
	//
		float d2 = f2->planes[0].compute_distance(other);

	//
	// The conditional is equivalent to fabs(d) > fabs(d2).
	//
		if (d < 0.0f && d2 > d || d > 0.0f && d2 < d)
		{
			f1 = e;
			return CONTINUE;
		}
	}

	if (d > 0.0f)
	{
		p1 = v->p;	// in R1
		p2 = xv - d * f2->planes[0].N;
		return DISJOINT;
	}

// LOCAL MINIMUM.
	Feature * update = NULL;
	float dmax = -FLT_MAX;
	VFace * face = box2.faces;
	for (i = 0; i < 6; i++, face++)
	{
		VPlane * p = face->planes;
		float d = p->compute_distance(xv);
		if (d > dmax)
		{
			dmax = d;
			update = face;
		}
	}
	if (dmax <= 0.0f)
	{
		return PENETRATION;
	}
	f2 = update;
	return CONTINUE;
}

//

int clip_edge(float & min, float & max, const Vector & t, const Vector & h, VRegion * vr)
{
	VRegion * Nmin = NULL;
	VRegion * Nmax = NULL;

	VPlane * p = vr->planes;
	for (int i = 0; i < vr->num_planes; i++, p++)
	{
		VRegion * N = p->neighbor;
		float dt = p->compute_distance(t);
		float dh = p->compute_distance(h);
		if (dt < 0.0f && dh < 0.0f)
		{
			Nmin = Nmax = N;
			return 0;
		}
		else if (dt < 0.0f)
		{
			float lambda = dt / (dt - dh);
			if (lambda > min)
			{
				min = lambda;
				Nmin = N;
				if (min > max)
				{
					return 0;
				}
			}
		}
		else if (dh < 0.0f)
		{
			float lambda = dt / (dt - dh);
			if (lambda < max)
			{
				max = lambda;
				Nmax = N;
				if (min > max)
				{
					return 0;
				}
			}
		}
	}

	return 1;
}

//

int edge_edge_subtest(Feature *& f, const Vector & t, const Vector & h, Vector & pt)
{
	float min = 0;
	float max = 1;
	Feature * Nmin = NULL;
	Feature * Nmax = NULL;

	VPlane * p = f->planes;
	float dt = p->compute_distance(t);
	float dh = p->compute_distance(h);

	if (dt < 0.0f)
	{
		if (dh < 0.0f)
		{
			f = p->neighbor;
			return CONTINUE;
		}
		min = dt / (dt - dh);
		Nmin = p->neighbor;
	}
	else if (dh < 0.0f)
	{
		max = dt / (dt - dh);
		Nmax = p->neighbor;
	}

	p++;

	dt = p->compute_distance(t);
	dh = p->compute_distance(h);
	if (dt < 0.0f)
	{
		if (dh < 0.0f)
		{
			f = p->neighbor;
			return CONTINUE;
		}
		min = dt / (dt - dh);
		Nmin = p->neighbor;
	}
	else if (dh < 0.0f)
	{
		max = dt / (dt - dh);
		Nmax = p->neighbor;
	}

	float vmin, vmax;
	VVertex * vNmin = (VVertex *) Nmin;
	VVertex * vNmax = (VVertex *) Nmax;
	if (Nmin)
	{
		vmin = min;
	}
	if (Nmax)
	{
		vmax = max;
	}

	p++;
	for (int i = 0; i < 2; i++, p++)
	{
		dt = p->compute_distance(t);
		dh = p->compute_distance(h);
		if (dt < 0.0f)
		{
			if (dh < 0.0f)
			{
			// completely clipped by a face plane, check derivatives.
				if (vNmin)
				{
					Vector point = t + vmin * (h - t);
					point -= vNmin->p;
					if (point.x == 0.0f && point.y == 0.0f && point.z == 0.0f)
					{
						pt = vNmin->p;
						return PENETRATION;
					}
					if (dot_product(point, h - t) > 0.0f)
					{
						f = vNmin;
						return CONTINUE;
					}
				}
				if (vNmax)
				{
					Vector point = t + vmax * (h - t);
					point -= vNmax->p;
					if (point.x == 0.0f && point.y == 0.0f && point.z == 0.0f)
					{
						pt = vNmax->p;
						return PENETRATION;
					}
					if (dot_product(point, h - t) < 0.0f)
					{
						f = vNmax;
						return CONTINUE;
					}
				}
				f = p->neighbor;
				return CONTINUE;
			}
			else
			{
				float lambda = dt / (dt - dh);
				if (lambda > min)
				{
					min = lambda; 
					Nmin = p->neighbor; 
					if (min > max) 
						break;
				}
			}
		}
		else if (dh < 0.0f)
		{
			float lambda = dt / (dt - dh);
			if (lambda < max)
			{
				max = lambda;
				Nmax = p->neighbor;
				if (max < min)
					break;
			}
		}
	}

	if (i < 2)
	{
		if (Nmin->type == Feature::VERT)
		{
			Vector point = t + min * (h - t);
			point -= ((VVertex *) Nmin)->p;
			if (point.x == 0.0f && point.y == 0.0f && point.z == 0.0f)
			{
				pt = ((VVertex *) Nmin)->p;
				return PENETRATION;
			}
			if (dot_product(point, (h - t)) >= 0.0f)
			{
				f = Nmin;
			}
			else 
			{
				f = Nmax;
			}
			return CONTINUE;
		}
		if (Nmax->type == Feature::VERT)
		{
			Vector point = t + max * (h - t);
			point -= ((VVertex *)Nmax)->p;
			if (point.x == 0.0f && point.y == 0.0f && point.z == 0.0f)
			{
				pt = ((VVertex *) Nmax)->p;
				return PENETRATION;
			}
			if (dot_product(point, h - t) <= 0.0f)
			{
				f = Nmax;
			}
			else 
			{
				f = Nmin;
			}
			return CONTINUE;
		}

	// Neighbors are faces.
		dt = Nmin->planes[0].compute_distance(t);
		dh = Nmin->planes[0].compute_distance(h);
		float dmin = dt + min * (dh - dt);
		if (dmin == 0.0f)
		{
			pt = t + min * (h - t);
			return PENETRATION;
		}

		if (dmin > 0.0f)
		{
			if (dt < dh)
			{
				f = Nmin;
			}
			else 
			{
				f = Nmax;
			}
		}
		else 
		{
			if (dt > dh)
			{
				f = Nmin;
			}
			else
			{
				f = Nmax;
			}
		}

		return CONTINUE;
	}

// edge intersects V-region; analyze derivs
	if (Nmin)
	{
		if (Nmin->type == Feature::FACE)
		{
			dt = Nmin->planes[0].compute_distance(t);
			dh = Nmin->planes[0].compute_distance(h);
			float dmin = dt + min * (dh - dt);
			float dmax = (Nmax) ? (dt + max * (dh - dt)) : dh;
			if (dmin == 0.0f)
			{
				pt = t + min * (h - t);
				return PENETRATION;
			}
			if ((dmin > 0.0f && dmin < dmax) || (dmin < 0.0f && dmin > dmax))
			{
				f = Nmin;
				return CONTINUE;
			}
		}
		else
		{
			Vector point = t + min * (h - t);
			point -= ((VVertex *) Nmin)->p;
			if (point.x == 0.0f && point.y == 0.0f && point.z == 0.0f)
			{
				pt = ((VVertex *) Nmin)->p;
				return PENETRATION;
			}
			if (dot_product(point, h - t) > 0.0f)
			{
				f = Nmin;
				return CONTINUE;
			}
		}
	}

	if (Nmax)
	{
		if (Nmax->type == Feature::FACE)
		{
			dt = Nmax->planes[0].compute_distance(t);
			dh = Nmax->planes[0].compute_distance(h);
			float dmin = (Nmin) ? (dt + min * (dh - dt)) : dt;
			float dmax = dt + max * (dh - dt);
			if (dmax == 0.0f)
			{
				pt = t + max * (h - t);
				return PENETRATION;
			}
			if ((dmax > 0.0f && dmax < dmin) || (dmax < 0.0f && dmax > dmin))
			{
				f = Nmax;
				return CONTINUE;
			}
		}
		else
		{
			Vector point = t + max * (h - t);
			point -= ((VVertex *) Nmax)->p;
			if (point.x == 0.0f && point.y == 0.0f && point.z == 0.0f)
			{
				pt = ((VVertex *) Nmax)->p;
				return PENETRATION;
			}
			if (dot_product(point, h - t) < 0.0f)
			{
				f = Nmax;
				return CONTINUE;
			}
		}
	}

	return DISJOINT;
}


//

int edge_edge(Feature *& f1, const Vector & r1, const Matrix & R1, Feature *& f2, const Vector & r2, const Matrix & R2, Vector & p1, Vector & p2)
{
	VEdge * e1 = (VEdge *) f1;
	VEdge * e2 = (VEdge *) f2;

	Vector t1 = r1 + R1 * e1->get_v0();
	Vector h1 = r1 + R1 * e1->get_v1();
	int result;

	if ((result = edge_edge_subtest(f2, t1, h1, p2)) == PENETRATION)
	{
		p1 = r2 + R2 * p2;	// transform to R1.
	}
	if (result != DISJOINT)
	{
		return result;
	}

	Vector t2 = r2 + R2 * e2->get_v0();
	Vector h2 = r2 + R2 * e2->get_v1();
	if ((result = edge_edge_subtest(f1, t2, h2, p1)) == PENETRATION)
	{
		p2 = r1 + R1 * p1;
	}
	if (result != DISJOINT)
	{
		return result;
	}

// disjoint - compute closest points & distance

	Vector dir1 = e1->get_v1() - e1->get_v0();
	float len1 = dir1.magnitude();
	dir1 /= len1;
	Vector dir2 = e2->get_v1() - e2->get_v0();
	float len2 = dir2.magnitude();
	dir2 /= len2;

	Vector xdir = R2 * dir2;			// in R1
	float k = dot_product(dir1, xdir);
	Vector h = t2 - e1->get_v0();
	h2 = dir1 - k * xdir;
	float num = dot_product(h, h2);
	float denom = 1 - k*k;
	if (denom == 0.0f)
	{
		if (num > 0.0f)
		{
			p1 = e1->get_v1();
		}
		else
		{
			p1 = e1->get_v0();
		}
	}
	else
	{
		float lambda = num / denom;
		if (lambda < 0.0f)
		{
			lambda = 0;
		}
		else if (lambda > len1)
		{
			lambda = len1;
		}
		p1 = e1->get_v0() + lambda * dir1;
	}

	Vector xc = r1 + R1 * p1;
	h = xc - e2->get_v0();
	float lambda = dot_product(h, dir2);
	p2 = e2->get_v0() + lambda * dir2;

	return DISJOINT;
}

//
/*
int edge_face(VRegion *& f1, const Vector & r1, const Matrix & R1, VRegion *& f2, const Vector & r2, const Matrix & R2, Vector & p1, Vector & p2)
{
	enum Code {INSIDE, OUTSIDE, MIN, MAX};

	Code code[4];
	float lambda[4];

	Vector t = r1 + R1 * f1->get_edge_v0();
	Vector h = r1 + R1 * f1->get_edge_v1();

	float min = 0;
	float max = 1;
	VPlane * pmin = NULL;
	VPlane * pmax = NULL;
	VPlane * pcut = NULL;

	VPlane * p = f2->planes+1;
	Code * c = code;
	float * l = lambda;
	for (int i = 1; i < f2->num_planes; i++, p++, c++, l++)
	{
		float dt = p->compute_distance(t);
		float dh = p->compute_distance(h);
		if (dt >= 0.0f)
		{
			if (dh >= 0.0f) 
			{
				*c = INSIDE;
			}
			else 
			{ // dh < 0
				*c = MAX;
				if ((*l = dt / (dt - dh)) < max) 
				{
					max = *l; 
					pmax = p;
				}
			}
		}
		else  // dt < 0
		{
			if (dh >= 0.0f) 
			{
				*c = MIN;
				if ((*l = dt / (dt - dh)) > min) 
				{
					min = *l; 
					pmin = p;
				}
			}
			else 
			{ // dh < 0
				*c = OUTSIDE;
				pcut = p;
			}
		}
	}

	if (pcut || min > max) 
	{
		if (pcut) 
		{
			p = pcut;
		}
		else
		{
		// heuristic:  choose minCn or maxCn, based on which
		// corresponding region contains more of edge being clipped.
			p = (min + max > 1.0.0f) ? pmin : pmax;
		}

		VPlane * prev = NULL; 
		VPlane * next = p;
		int intersect = 0;
		while (next != prev) 
		{
			prev = p;
			p = next;
			VRegion * s = p->neighbor;
			VRegion * minv = NULL;
			VRegion * maxv = NULL;

		// test edge plane
			i = p - (f2->planes+1);
			if (code[i] == INSIDE) 
			{
				break;
			}
			else if (code[i] == OUTSIDE) 
			{
				min = 0; 
				max = 1;
			}
			else if (code[i] == MIN) 
			{
				min = 0; 
				max = lambda[i];
			}
			else if (code[i] == MAX) 
			{
				min = lambda[i]; 
				max = 1;
			}

		// test tail plane
			float dt = s->planes[0].compute_distance(t);
			float dh = s->planes[0].compute_distance(h);
			if (dt >= 0.0f) 
			{
				if (dh < 0.0f)
				{
					float l = dt / (dt - dh);
					if (l < max) 
					{
						max = l; 
						maxv = s->planes + 0; 
						if (min > max) 
						{
							if (intersect) 
								break; 
							next = (s->left == f) ? cn->cw : cn->ccw; 
							continue;
						}
					}
				}
			}
			else 
			{ // dt < 0
				if (dh < 0.0f) 
				{
					next = (s->left == f) ? cn->cw  : cn->ccw; 
					continue;
				}
				if ((lambda = dt / (dt - dh)) > min) 
				{
					min = lambda; 
					minv = s->tail; 
					if (min > max) 
					{
						if (intersect) 
							break; 
						next = (s->left == f) ? cn->cw : cn->ccw; 
						continue;
					}
				}
			}

		// test head plane
			dt = s->planes[1].compute_distance(t);
			dh = s->planes[1].compute_distance(h);
			if (dt >= 0.0f) 
			{
	if (dh < 0.0f)
	if ((lambda = dt / (dt - dh)) < max) {
	max = lambda; maxv = s->head; 
	if (min > max) {
	if (intersect) break; 
	next = (s->left == f) ? cn->ccw : cn->cw; 
	continue;
	}
	}
	}
	else { // dt < 0
	if (dh < 0.0f) {next = (s->left == f) ? cn->ccw : cn->cw;  continue;}
	if ((lambda = dt / (dt - dh)) > min) {
	min = lambda; minv = s->head;
	if (min > max) {
	if (intersect) break; 
	next = (s->left == f) ? cn->ccw : cn->cw; 
	continue;
	}
	}
	}

	intersect = 1; // we've found an edge Voronoi region that's intersected

	if (minv) {
	point.displace(xe.tail, xe.seg, min);
	point.sub(minv->coords_);
	if (point.dot(xe.seg) > 0.0f) {
	next = (s->left == f) ? ((s->tail == minv) ? cn->cw  : cn->ccw) :
					  ((s->tail == minv) ? cn->ccw : cn->cw);
	continue;
	}
	}

	if (maxv) {
	point.displace(xe.tail, xe.seg, max);
	point.sub(maxv->coords_);
	if (point.dot(xe.seg) < 0.0f) {
	next = (s->left == f) ? ((s->head == maxv) ? cn->ccw : cn->cw) :
					  ((s->head == maxv) ? cn->cw  : cn->ccw);
	continue;
	}
	}

	f = s;
	return CONTINUE;
	}

	f = (cn->ccw == prev) ? ((s->left == f) ? s->head : s->tail) :
					 ((s->left == f) ? s->tail : s->head) ;
	return CONTINUE;
	}

	// edge intersects faces cone - check derivatives

	dt = F(f)->plane.dist(xe.tail);
	dh = F(f)->plane.dist(xe.head);
	dmin = (minCn) ? (dt + min * (dh - dt)) : dt;
	dmax = (maxCn) ? (dt + max * (dh - dt)) : dh;
	if (dmin <= 0.0f) {
	if (dmax >= 0.0f) {
	dist = dmin; 
	cpe.displace(E(e)->tail->coords_, E(e)->dir, min * E(e)->len);
	cpf.displace(xe.tail, xe.seg, min);
	cpf.displace(F(f)->plane.normal(), -dmin);
	return PENETRATION;
	}
	}
	else if (dmax <= 0.0f) {
	dist = dmax; 
	cpe.displace(E(e)->tail->coords_, E(e)->dir, max * E(e)->len);
	cpf.displace(xe.tail, xe.seg, max);
	cpf.displace(F(f)->plane.normal(), -dmax);
	return PENETRATION;
	}

	// at this point, dmin & dmax are both +ve or both -ve
	if (dmin > 0.0f && dt <= dh || dmin < 0.0f && dt >= dh)
	if (minCn) f = minCn->nbr; 
	else {
	xe.coords = xe.tail;
	xe.feat = e = E(e)->tail;
	}
	else
	if (maxCn) f = maxCn->nbr; 
	else {
	xe.coords = xe.head;
	xe.feat = e = E(e)->head;
	}

	return CONTINUE;
}
*/

int edge_face(Feature *& f1, const Vector & r1, const Matrix & R1, Feature *& f2, const Vector & r2, const Matrix & R2, Vector & p1, Vector & p2)
{
	VEdge * e = (VEdge *) f1;
	VFace * f = (VFace *) f2;

	Vector t = r1 + R1 * e->get_v0();
	Vector h = r1 + R1 * e->get_v1();

	typedef enum {INSIDE, OUTSIDE, MIN, MAX} ClipCode;

	ClipCode clip[64];
	float lambda[64];

	float min = 0;
	float max = 1;

//
// Clip edge against all edge-face planes.
//
	VPlane * pmin = NULL;
	VPlane * pmax = NULL;
	VPlane * pexclude = NULL;
	VPlane * p = f2->planes+1;
	ClipCode * c = clip;
	float * l = lambda;
	for (int i = 1; i < f2->num_planes; i++, p++, c++, l++)
	{
		float dt = p->compute_distance(t);
		float dh = p->compute_distance(h);
		if (dt >= 0.0f)
		{
			if (dh >= 0.0f)
			{
				*c = INSIDE;
			}
			else
			{
				*c = MAX;
				*l = dt / (dt - dh);
				if (*l < max)
				{
					max = *l;
					pmax = p;
				}
			}
		}
		else
		{
			if (dh >= 0.0f)
			{
				*c = MIN;
				*l = dt / (dt - dh);
				if (*l > min)
				{
					min = *l;
					pmin = p;
				}
			}
			else
			{
				*c = OUTSIDE;
				pexclude = p;
			}
		}
	}

	if (pexclude || min > max)
	{
		if (pexclude)
		{
			p = pexclude;
		}
		else
		{
			p = (min + max > 1) ? pmin : pmax;
		}


		Feature * s = NULL;
		VPlane * prev = NULL;
		VPlane * next = p;
		bool intersect = false;
		while (next != prev)
		{
			prev = p;
			p = next;
			s = p->neighbor;
			VVertex * minv = NULL;
			VVertex * maxv = NULL;

			i = p - (f2->planes+1);
			if (clip[i] == INSIDE)
			{
				break;
			}
			else if (clip[i] == OUTSIDE)
			{
				min = 0;
				max = 1;
			}
			else if (clip[i] == MIN)
			{
				min = 0;
				max = lambda[i];
			}
			else if (clip[i] == MAX)
			{
				min = lambda[i];
				max = 1;
			}

			float dt = s->planes[0].compute_distance(t);
			float dh = s->planes[0].compute_distance(h);
			if (dt >= 0.0f)
			{
				if (dh < 0.0f)
				{
					float lambda = dt / (dt - dh);
					if (lambda < max)
					{
						max = lambda;
						ASSERT(s->planes[0].neighbor->type == Feature::VERT);
						maxv = (VVertex *) s->planes[0].neighbor;
						if (min > max)
						{
							if (intersect)
							{
								break;
							}
							next = (s->planes[2].neighbor == f2) ? f2->next(p) : f2->prev(p);
							continue;
						}
					}
				}
			}
			else
			{
				if (dh < 0.0f)
				{
					next = (s->planes[2].neighbor == f2) ? f2->next(p) : f2->prev(p);
					continue;
				}
				float l = dt / (dt - dh);
				if (l > min)
				{
					min = l;
					ASSERT(s->planes[0].neighbor->type == Feature::VERT);
					minv = (VVertex *) s->planes[0].neighbor;
					if (min > max)
					{
						if (intersect)
						{
							break;
						}
						next = (s->planes[2].neighbor == f2) ? f2->next(p) : f2->prev(p);
						continue;
					}
				}
			}

			dt = s->planes[1].compute_distance(t);
			dh = s->planes[1].compute_distance(h);

			if (dt >= 0.0f)
			{
				if (dh < 0.0f)
				{
					float l = dt / (dt - dh);
					if (l < max)
					{
						max = l;
						ASSERT(s->planes[1].neighbor->type == Feature::VERT);
						maxv = (VVertex *) s->planes[1].neighbor;
						if (min > max)
						{
							if (intersect)
							{
								break;
							}
							next = (s->planes[2].neighbor == f2) ? f2->prev(p) : f2->next(p);
							continue;
						}
					}
				}
			}
			else
			{
				if (dh < 0.0f)
				{
					next = (s->planes[2].neighbor == f2) ? f2->prev(p) : f2->next(p);
					continue;
				}
				float l = dt / (dt - dh);
				if (l > min)
				{
					min = l;
					ASSERT(s->planes[1].neighbor->type == Feature::VERT);
					minv = (VVertex *) s->planes[1].neighbor;
					if (min > max)
					{
						if (intersect)
						{
							break;
						}
						next = (s->planes[2].neighbor == f2) ? f2->prev(p) : f2->next(p);
						continue;
					}
				}
			}

			intersect = 1;

			if (minv)
			{
				Vector point = t + min * (h - t);
				point -= minv->p;
				if (dot_product(point, h - t) > 0.0f)
				{
					if (s->planes[2].neighbor == f2)
					{
						next = (s->planes[0].neighbor == minv) ? f2->next(p) : f2->prev(p);
					}
					else
					{
						next = (s->planes[0].neighbor == minv) ? f2->prev(p) : f2->next(p);
					}
					continue;
				}
			}

			if (maxv)
			{
				Vector point = t + max * (h - t);
				point -= maxv->p;
				if (dot_product(point, h - t) < 0.0f)
				{
					if (s->planes[2].neighbor == f2)
					{
						next = (s->planes[1].neighbor == maxv) ? f2->prev(p) : f2->next(p);
					}
					else
					{
						next = (s->planes[1].neighbor == maxv) ? f2->next(p) : f2->prev(p);
					}
					continue;
				}
			}

			f2 = s;
			return CONTINUE;
		}

		if (f2->prev(p) == prev)
		{
			f2 = (s->planes[2].neighbor == f2) ? s->planes[1].neighbor : s->planes[0].neighbor;
		}
		else
		{
			f2 = (s->planes[2].neighbor == f2) ? s->planes[0].neighbor : s->planes[1].neighbor;
		}


		return CONTINUE;
	}

// edge intersects faces cone - check derivatives

	float dt = f2->planes[0].compute_distance(t);
	float dh = f2->planes[0].compute_distance(h);
	float dmin = (pmin) ? (dt + min * (dh - dt)) : dt;
	float dmax = (pmax) ? (dt + max * (dh - dt)) : dh;

	Vector dir = e->get_v1() - e->get_v0();

	if (dmin <= 0.0f)
	{
		if (dmax >= 1e-5) // TOLERANCE
		{
			//p1 = e->get_v0() + min * dir;
			p2 = t + min * (h - t);	// t & h are already in  R2
			p2 -= f2->planes[0].N * dmin;

		p1 = r2 + R2 * p2;

			return PENETRATION;
		}
	}
	else if (dmax <= 0.0f)
	{
		//p1 = e->get_v0() + dir * max;
		p2 = t + max * (h - t);	
		p2 -= f2->planes[0].N * dmax;

	p1 = r2 + R2 * p2;

		return PENETRATION;
	}

	if (dmin > 0.0f && dt <= dh || dmin < 0.0f && dt >= dh)
	{
		if (pmin)
		{
			f2 = pmin->neighbor;
		}
		else
		{
			f1 = f1->planes[0].neighbor;
		}
	}
	else
	{
		if (pmax)
		{
			f2 = pmax->neighbor;
		}
		else
		{
			f1 = f1->planes[1].neighbor;
		}
	}

	return CONTINUE;
}

//

//
// NORMAL COMPUTATION FUNCTIONS FOLLOW:
//

Vector DoSphereSphere(const XSphere & s1, const XSphere & s2)
{
	Vector diff = s1.center - s2.center;
	diff.normalize();
	return diff;
}

Vector DoSphereCylinder(const XSphere & s, const Vector & c0, const Vector & c1)
{
	Vector dc = c1 - c0;
	Vector diff = s.center - c0;
	float t = dot_product(dc, diff) / dot_product(dc, dc);

	if (t < 0.0f) t = 0;
	if (t > 1) t = 1;

	diff -= t * dc;
	diff.normalize();

	return diff;
}

Vector DoSpherePlane(const Sphere & s, const Plane & p)
{
	return p.N;
}

//

float MinLineLine (const LineSegment & line0, const LineSegment & line1, float & s, float & t);

//

Vector DoCylinderCylinder(const Vector & c0, const Vector & c1, const Vector & d0, const Vector & d1)
{
	LineSegment c(c0, c1);
	LineSegment d(d0, d1);

	float s, t;
	MinLineLine(c, d, s, t);

	Vector p0 = c0 + s * (c1 - c0);
	Vector p1 = d0 + t * (d1 - d0);

	Vector dp = p0 - p1;
	dp.normalize();
	return dp;
}

Vector DoCylinderPlane(const Vector & c0, const Vector & c1, const Plane & p)
{
	return p.N;
}


//

static inline bool PointInBox(const Box & box, const Vector & p)
{
	return ((fabs(p.x) <= box.half_x) && (fabs(p.y) <= box.half_y) && (fabs(p.z) <= box.half_z));
}

struct FeaturePair
{
	Feature * f1;
	Feature * f2;
};

//
// MAIN VCLIP FUNCTION FOLLOWS.
//

void vclip(VMesh & box1, CollisionMesh * _mesh1, 
		   VMesh & box2, CollisionMesh * _mesh2, 
		   const Vector & _r1, const Matrix & _R1,
		   const Vector & _r2, const Matrix & _R2,
		   Vector &p1, Vector & p2, Vector & n)
{
// Compute relative transforms between 2 boxes.
	Vector r1 = _R2.get_transpose() * (_r1 - _r2);
	Matrix R1 = _R2.get_transpose() * _R1;
	Vector r2 = _R1.get_transpose() * (_r2 - _r1);
	Matrix R2 = _R1.get_transpose() * _R2;

	Feature * f1 = box1.vertices;
	Feature * f2 = box2.vertices;

	int num_pairs = 0;
	DynamicArray<FeaturePair> pair_list;

	int result = CONTINUE;
	while (result == CONTINUE)
	{
		bool cycle = false;
		for (int i = 0; i < num_pairs; i++)
		{
			if (pair_list[i].f1 == f1 && pair_list[i].f2 == f2)
			{
				PHYTRACE10("CYCLE DETECTED IN VCLIP.\n");
				cycle = true;
				break;
			}
		}

		if (cycle)
		{
			result = DISJOINT;
		}
		else
		{
			pair_list[num_pairs].f1 = f1;
			pair_list[num_pairs].f2 = f2;
			num_pairs++;
			switch ((f1->type << 2) + f2->type)
			{
				case ((Feature::VERT << 2) + Feature::VERT):
					result = vertex_vertex(f1, r1, R1, f2, r2, R2, p1, p2);
					break;
				case ((Feature::VERT << 2) + Feature::EDGE):
					result = vertex_edge(f1, r1, R1, f2, r2, R2, p1, p2);
					break;
				case ((Feature::EDGE << 2) + Feature::VERT):
					result = vertex_edge(f2, r2, R2, f1, r1, R1, p2, p1);
					break;
				case ((Feature::VERT << 2) + Feature::FACE):
					result = vertex_face(f1, r1, R1, f2, r2, R2, box2, p1, p2);
					break;
				case ((Feature::FACE << 2) + Feature::VERT):
					result = vertex_face(f2, r2, R2, f1, r1, R1, box1, p2, p1);
					break;
				case ((Feature::EDGE << 2) + Feature::EDGE):
					result = edge_edge(f1, r1, R1, f2, r2, R2, p1, p2);
					break;
				case ((Feature::EDGE << 2) + Feature::FACE):
					result = edge_face(f1, r1, R1, f2, r2, R2, p1, p2);
					break;
				case ((Feature::FACE << 2) + Feature::EDGE):
					result = edge_face(f2, r2, R2, f1, r1, R1, p2, p1);
					break;
			}
		}
	}

	char * features[] = {"vert", "edge", "face"};

	if (result == PENETRATION)
	{
//		DebugPrint("PENETRATION\n");
	}
	else
	{
#if 0
		switch ((f1->type << 2) + f2->type)
		{
			case ((Feature::VERT << 2) + Feature::VERT):
				PHYTRACE10("vertex/vertex.\n");
				break;

			case ((Feature::VERT << 2) + Feature::EDGE):
			case ((Feature::EDGE << 2) + Feature::VERT):
				PHYTRACE10("vertex/edge.\n");
				break;

			case ((Feature::VERT << 2) + Feature::FACE):
			case ((Feature::FACE << 2) + Feature::VERT):
				PHYTRACE10("vertex/face.\n");
				break;

			case ((Feature::EDGE << 2) + Feature::EDGE):
				PHYTRACE10("edge/edge.\n");
				break;

			case ((Feature::EDGE << 2) + Feature::FACE):
			case ((Feature::FACE << 2) + Feature::EDGE):
				PHYTRACE10("face/edge.\n");
				break;
		}
#endif
	}

//
//
//
	const float expansion = 1e-2;
	switch ((f1->type << 2) + f2->type)
	{
	//
	// Convert vertex/vertex to sphere/sphere.
	//
		case ((Feature::VERT << 2) + Feature::VERT):
		{
//DebugPrint("vv\n");
			XSphere s1, s2;
			s1.center = p1;
			s2.center = r2 + R2 * p2;
			s1.radius = s2.radius = expansion;
			n = DoSphereSphere(s1, s2);
			break;
		}
	//
	// Convert vertex/edge to sphere/cylinder.
	//
		case ((Feature::VERT << 2) + Feature::EDGE):
		{
//DebugPrint("ve\n");
			XSphere s;
			s.center = p1;
			s.radius = expansion;

			VEdge * edge = (VEdge *) f2;
			Vector e0 = r2 + R2 * edge->get_v0();
			Vector e1 = r2 + R2 * edge->get_v1();
			n = DoSphereCylinder(s, e0, e1);
			break;
		}
		case ((Feature::EDGE << 2) + Feature::VERT):
		{
//DebugPrint("ev\n");
			XSphere s;
			s.center = r2 + R2 * p2;
			s.radius = expansion;

			VEdge * edge = (VEdge *) f1;
			Vector e0 = edge->get_v0();
			Vector e1 = edge->get_v1();

			n = -DoSphereCylinder(s, e0, e1);
			break;
		}

	//
	// Convert vertex/face to sphere/plane.
	// 
		case ((Feature::VERT << 2) + Feature::FACE):
		{
//DebugPrint("vf\n");
			XSphere s;
			s.center = p1;
			s.radius = expansion;

			VFace * face = (VFace *) f2;
			Vector v0 = r2 + R2 * face->get_vert(0);
			Vector v1 = r2 + R2 * face->get_vert(1);
			Vector v2 = r2 + R2 * face->get_vert(2);
			Plane p(v0, v1, v2);

			n = DoSpherePlane(s, p);
			break;
		}

		case ((Feature::FACE << 2) + Feature::VERT):
		{
//DebugPrint("fv\n");
			XSphere s;
			s.center = r2 + R2 * p2;
			s.radius = expansion;

			VFace * face = (VFace *) f1;
			Vector v0 = face->get_vert(0);
			Vector v1 = face->get_vert(1);
			Vector v2 = face->get_vert(2);
			Plane p(v0, v1, v2);

			n = -DoSpherePlane(s, p);
			break;
		}

	//
	// Convert edge/edge to cylinder/cylinder.
	//
		case ((Feature::EDGE << 2) + Feature::EDGE):
		{
//DebugPrint("ee\n");
			VEdge * e0 = (VEdge *) f1;
			VEdge * e1 = (VEdge *) f2;
			Vector c0 = e0->get_v0();
			Vector c1 = e0->get_v1();
			Vector d0 = r2 + R2 * e1->get_v0();
			Vector d1 = r2 + R2 * e1->get_v1();

			n = DoCylinderCylinder(c0, c1, d0, d1);
			break;
		}

	//
	// Convert edge/face to cylinder/plane.
	//
		case ((Feature::EDGE << 2) + Feature::FACE):
		{
//DebugPrint("ef *\n");
			VEdge * edge = (VEdge *) f1;
			VFace * face = (VFace *) f2;

			Vector c0 = edge->get_v0();
			Vector c1 = edge->get_v1();
			Vector v0 = r2 + R2 * face->get_vert(0);
			Vector v1 = r2 + R2 * face->get_vert(1);
			Vector v2 = r2 + R2 * face->get_vert(2);
			Plane p(v0, v1, v2);

			n = DoCylinderPlane(c0, c1, p);
			break;
		}
		case ((Feature::FACE << 2) + Feature::EDGE):
		{
//DebugPrint("fe *\n");
			VEdge * edge = (VEdge *) f2;
			VFace * face = (VFace *) f1;

			Vector c0 = r2 + R2 * edge->get_v0();
			Vector c1 = r2 + R2 * edge->get_v1();
			Vector v0 = face->get_vert(0);
			Vector v1 = face->get_vert(1);
			Vector v2 = face->get_vert(2);
			Plane p(v0, v1, v2);

			n = -DoCylinderPlane(c0, c1, p);
			break;
		}
	}

//	n = _R1.get_transpose() * n;

#if 0
//	DebugPrint("f1: %s %d f2: %s %d\n", features[f1->type], f1->id, features[f2->type], f2->id);
	if (f1->type == Feature::FACE)
	{
		n = -box1.normals[box1.faces[f1->id].normal];
	}
	else if (f2->type == Feature::FACE)
	{
		n = R2 * box2.normals[box2.faces[f2->id].normal];
	}
	else
	{

// expand edges into cylinders, vertices into spheres.

		const float expansion = 1e-2;

		if (f1->type == Feature::EDGE)
		{
			VEdge * e = (VEdge *) f1;
			Vector other = r2 + R2 * p2;
			Vector edge = e->get_v1() - e->get_v0();
			Vector dir = edge;
			dir.normalize();
			Vector dp = other - e->get_v0();
			float t = dot_product(edge, dp) / dot_product(edge, edge);

			Vector pt = e->get_v0() + t * dir;
			Vector diff = other - pt;
			diff.normalize();
			n = -diff;
		}
		else if (f2->type == Feature::EDGE)
		{
			VEdge * e = (VEdge *) f2;
			Vector other = r1 + R1 * p1;
			Vector edge = e->get_v1() - e->get_v0();
			Vector dir = edge;
			dir.normalize();
			Vector dp = other - e->get_v0();
			float t = dot_product(edge, dp) / dot_product(edge, edge);

			Vector pt = e->get_v0() + t * dir;
			Vector diff = other - pt;
			diff.normalize();
			n = diff;
		}
		else
		{
		// vertex/vertex, ouch.
			PHYTRACE10("VERTEX VERTEX\n");
		}

#if 0
	//
	// Find bigger box.
	//
		if (box1.volume > box2.volume)
		{
			if (f1->type == Feature::EDGE)
			{
				n = -box1.normals[box1.edges[f1->id].n];
			}
			else
			{
				n = -box1.normals[box1.vertices[f1->id].n];
			}
		}
		else
		{
			if (f2->type == Feature::EDGE)
			{
				n = R2 * box2.normals[box2.edges[f2->id].n];
			}
			else
			{
				n = R2 * box2.normals[box2.vertices[f2->id].n];
			}
		}
#endif
	}
#endif


// DEBUG DRAW CRAP.
#if 0
	switch (f1->type)
	{
	case Feature::VERT:
		HackDrawPoint(_r1 + _R1 * ((VVertex *) f1)->p, 255, 255, 255, 0.5f);
		break;
	case Feature::EDGE:
		HackDrawPoint(_r1 + _R1 * ((VEdge *) f1)->get_v0(), 128, 128, 128, 0.4f);
		HackDrawPoint(_r1 + _R1 * ((VEdge *) f1)->get_v1(), 128, 128, 128, 0.4f);
		break;
	case Feature::FACE:
		{
			for (int v = 0; v < box1.faces[f1->id].num_verts; v++)
			{
				HackDrawPoint(_r1 + _R1 * _mesh1->vertices[box1.faces[f1->id].v[v]].p, 32, 32, 32, 0.3);
			}
		//HackDrawPoint(_r1 + _R1 * _mesh1->vertices[box1.faces[f1->id].v[1]].p, 32, 32, 32, 0.3);
		//HackDrawPoint(_r1 + _R1 * _mesh1->vertices[box1.faces[f1->id].v[2]].p, 32, 32, 32, 0.3);
		//HackDrawPoint(_r1 + _R1 * _mesh1->vertices[box1.faces[f1->id].v[3]].p, 32, 32, 32, 0.3);
		}
		break;
	}

	switch (f2->type)
	{
	case Feature::VERT:
		HackDrawPoint(_r2 + _R2 * ((VVertex *) f2)->p, 255, 255, 255, 0.5f);
		break;
	case Feature::EDGE:
		HackDrawPoint(_r2 + _R2 * ((VEdge *) f2)->get_v0(), 128, 128, 128, 0.4f);
		HackDrawPoint(_r2 + _R2 * ((VEdge *) f2)->get_v1(), 128, 128, 128, 0.4f);
		break;
	case Feature::FACE:
		HackDrawPoint(_r2 + _R2 * _mesh2->vertices[box2.faces[f2->id].v[0]].p, 32, 32, 32, 0.3f);
		HackDrawPoint(_r2 + _R2 * _mesh2->vertices[box2.faces[f2->id].v[1]].p, 32, 32, 32, 0.3f);
		HackDrawPoint(_r2 + _R2 * _mesh2->vertices[box2.faces[f2->id].v[2]].p, 32, 32, 32, 0.3f);
		HackDrawPoint(_r2 + _R2 * _mesh2->vertices[box2.faces[f2->id].v[3]].p, 32, 32, 32, 0.3f);
		break;
	}
#endif
}

//

