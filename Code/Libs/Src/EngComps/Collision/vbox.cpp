//
// VBox.cpp - Axis-aligned box with voronoi regions.
//

#include "fdump.h"
#include "vbox.h"
#include "vcyl.h"

//

VBox::VBox(float x, float y, float z)
{
	volume = 4.0f * (x*x + y*y + z*z);

	num_vertices = 8;
	vertices = new VVertex[num_vertices];

	num_edges = 12;
	edges = new VEdge[num_edges];

	num_faces = 6;
	faces = new VFace[num_faces];

	num_normals = num_vertices + num_edges + num_faces;
	normals = new Vector[num_normals];

	vertices[0].p.set( x,  y,  z);
	vertices[1].p.set( x, -y,  z);
	vertices[2].p.set(-x, -y,  z);
	vertices[3].p.set(-x,  y,  z);
	vertices[4].p.set( x,  y, -z);
	vertices[5].p.set( x, -y, -z);
	vertices[6].p.set(-x, -y, -z);
	vertices[7].p.set(-x,  y, -z);

	faces[0].init(4);
	faces[0].v[0] = 0;
	faces[0].v[1] = 1;
	faces[0].v[2] = 2;
	faces[0].v[3] = 3;

	faces[1].init(4);
	faces[1].v[0] = 0;
	faces[1].v[1] = 4;
	faces[1].v[2] = 5;
	faces[1].v[3] = 1;

	faces[2].init(4);
	faces[2].v[0] = 4;
	faces[2].v[1] = 7;
	faces[2].v[2] = 6;
	faces[2].v[3] = 5;

	faces[3].init(4);
	faces[3].v[0] = 2;
	faces[3].v[1] = 6;
	faces[3].v[2] = 7;
	faces[3].v[3] = 3;

	faces[4].init(4);
	faces[4].v[0] = 0;
	faces[4].v[1] = 3;
	faces[4].v[2] = 7;
	faces[4].v[3] = 4;

	faces[5].init(4);
	faces[5].v[0] = 1;
	faces[5].v[1] = 5;
	faces[5].v[2] = 6;
	faces[5].v[3] = 2;

// edges
	edges[0].v[0] = 0;
	edges[0].v[1] = 1;
	edges[0].t[0] = 0;
	edges[0].t[1] = 1;

	edges[1].v[0] = 1;
	edges[1].v[1] = 2;
	edges[1].t[0] = 0;
	edges[1].t[1] = 5;

	edges[2].v[0] = 2;
	edges[2].v[1] = 3;
	edges[2].t[0] = 0;
	edges[2].t[1] = 3;

	edges[3].v[0] = 3;
	edges[3].v[1] = 0;
	edges[3].t[0] = 0;
	edges[3].t[1] = 4;

	edges[4].v[0] = 0;
	edges[4].v[1] = 4;
	edges[4].t[0] = 1;
	edges[4].t[1] = 4;

	edges[5].v[0] = 4;
	edges[5].v[1] = 5;
	edges[5].t[0] = 1;
	edges[5].t[1] = 2;

	edges[6].v[0] = 5;
	edges[6].v[1] = 1;
	edges[6].t[0] = 1;
	edges[6].t[1] = 5;

	edges[7].v[0] = 4;
	edges[7].v[1] = 7;
	edges[7].t[0] = 2;
	edges[7].t[1] = 4;

	edges[8].v[0] = 7;
	edges[8].v[1] = 6;
	edges[8].t[0] = 2;
	edges[8].t[1] = 3;

	edges[9].v[0] = 6;
	edges[9].v[1] = 5;
	edges[9].t[0] = 2;
	edges[9].t[1] = 5;

	edges[10].v[0] = 2;
	edges[10].v[1] = 6;
	edges[10].t[0] = 3;
	edges[10].t[1] = 5;

	edges[11].v[0] = 7;
	edges[11].v[1] = 3;
	edges[11].t[0] = 3;
	edges[11].t[1] = 4;

	int num_normals = 0;

	VFace * f = faces;
	for (int i = 0; i < num_faces; i++, f++)
	{
		Vector v0 = vertices[f->v[0]].p;
		Vector v1 = vertices[f->v[1]].p;
		Vector v2 = vertices[f->v[2]].p;

		Vector e0 = v0 - v2;
		Vector e1 = v0 - v1;
		Vector n = cross_product(e0, e1);
		n.normalize();

		f->normal = num_normals;
		normals[num_normals++] = n;

	// Find edges.
		for (int v = 0; v < 3; v++)
		{
			int vert0 = f->v[v];
			int vert1 = f->v[v+1];
			VEdge * e = edges;
			for (int j = 0; j < 12; j++, e++)
			{
				if ((e->v[0] == vert0 && e->v[1] == vert1) || (e->v[0] == vert1 && e->v[1] == vert0))
				{
					f->e[v] = j;
					break;
				}
			}
		}

	// close the loop.
		int vert0 = f->v[3];
		int vert1 = f->v[0];
		VEdge * e = edges;
		for (int j = 0; j < num_edges; j++, e++)
		{
			if ((e->v[0] == vert0 && e->v[1] == vert1) || (e->v[0] == vert1 && e->v[1] == vert0))
			{
				f->e[v] = j;
				break;
			}
		}

	// DEBUG CHECK.
		for (j = 0; j < f->num_verts; j++)
		{
			e = edges + f->e[j];
			ASSERT((e->t[0] == i) || (e->t[1] == i));
		}
	}

	VVertex * v = vertices;
	for (i = 0; i < num_vertices; i++, v++)
	{
		Vector n(0, 0, 0);

		f = faces;
		for (int j = 0; j < num_faces; j++, f++)
		{
			if (f->v[0] == i || f->v[1] == i || f->v[2] == i || f->v[3] == i)
			{
				n += normals[f->normal];
			}
		}

		n.normalize();
		v->n = num_normals;
		normals[num_normals++] = n;
	}

	VEdge * e = edges;
	for (i = 0; i < num_edges; i++, e++)
	{
		Vector n0 = normals[e->t[0]];
		Vector n1 = normals[e->t[1]];
		e->n = num_normals;
		normals[num_normals++] = 0.5f * (n0 + n1);
	}

	compute_vregions();
}

//

void VBox::compute_vregions(void)
{
// Compute vertex voronoi regions.
	VVertex * vtx = vertices;
	for (int i = 0; i < num_vertices; i++, vtx++)
	{
		vtx->id = i;

		vtx->num_planes = 3;
		vtx->planes = new VPlane[3];

		VPlane * p = vtx->planes;
	// find edges containing vertex.
		int edges_found = 0;
		VEdge * e = edges;
		for (int k = 0; k < num_edges; k++, e++)
		{
			if ((e->v[0] == i) || (e->v[1] == i))
			{
			// Compute plane that contains vertex, normal to edge.
				VVertex * other;
				if (e->v[0] == i)
				{
					other = vertices + e->v[1];
				}
				else 
				{
					other = vertices + e->v[0];
				}

				Vector N = vtx->p - other->p;
				p->init(vtx->p, N);
				p->neighbor = &edges[e - edges];
				p++;

				if (++edges_found == 3)
				{
					break;
				}
			}
		}

		ASSERT(edges_found == 3);
	}

//
// Compute edge voronoi regions.
//
// NOTE THAT SOME OF THESE EDGES ARE UNNECESSARY. CLEAN IT UP LATER.
//
	VEdge * e = edges;
	for (i = 0; i < num_edges; i++, e++)
	{
		e->id = i;
		e->verts = vertices;

		e->num_planes = 4;
		e->planes = new VPlane[4];

	// Compute vertex-edge planes.
		Vertex * v0 = vertices + e->v[0];
		Vertex * v1 = vertices + e->v[1];
		
		Vector N = v1->p - v0->p;
		N.normalize();

		e->planes[0].init(v0->p, N);
		e->planes[0].neighbor = vertices + e->v[0];
		e->planes[1].init(v1->p, -N);
		e->planes[1].neighbor = vertices + e->v[1];

	// Compute face-edge planes.
		VFace * t0 = faces + e->t[0];
		VFace * t1 = faces + e->t[1];

	// Opposite:
		Vector * n0 = normals + t1->normal;
		Vector * n1 = normals + t0->normal;
		//Vector * n0 = normals + t0->normal;
		//Vector * n1 = normals + t1->normal;

		e->planes[2].init(v0->p, *n0);
		e->planes[2].neighbor = faces + e->t[0];
		e->planes[3].init(v0->p, *n1);
		e->planes[3].neighbor = faces + e->t[1];
	}

// Compute face voronoi regions.
	VFace * f = faces;
	for (i = 0; i < num_faces; i++, f++)
	{
		f->id = i;
		f->verts = vertices;

		f->num_planes = f->num_verts + 1;
		f->planes = new VPlane[f->num_planes];

		Vertex * v0 = vertices + f->v[0];
		Vector * N = normals + f->normal;

	// Supporting plane for face.
		f->planes[0].init(v0->p, *N);
		f->planes[0].neighbor = NULL;

	// 1 face-edge plane for each edge.
		for (int j = 0; j < f->num_verts; j++)
		{
			VEdge * e = edges + f->e[j];
			v0 = vertices + e->v[0];
			VFace * t0 = faces + e->t[0];
			VFace * t1 = faces + e->t[1];

			int nidx = (t0->normal == f->normal) ? t1->normal : t0->normal;
			N = normals + nidx;

			f->planes[j+1].init(v0->p, -(*N));
			f->planes[j+1].neighbor = edges + f->e[j];
		}
	}
}

//
/*
void VBox::compute_faces(CollisionMesh * mesh)
{
// Compute triangle --> face mapping.
	int * map = tri_face_map;
	Triangle * t = mesh->triangles;
	Quad * f = faces;
	for (int i = 0; i < 6; i++, f++)
	{
		*map++ = i;
		*map++ = i;

		Triangle * t0 = t++;
		Triangle * t1 = t++;

		f->normal = t0->normal;

		int num_face_verts = 0;
		bool vert_other_tri[3] = {false, false, false};
		for (int j = 0; j < 3; j++)
		{
			bool dup = false;
			for (int k = 0; k < num_face_verts; k++)
			{
				if (t0->v[j] == f->v[k])
				{
					dup = true;
					break;
				}
			}

			if (!dup)
			{
				for (int k = 0; k < 3; k++)
				{
					if (t0->v[j] == t1->v[k])
					{
						if (vert_other_tri[k])
						{
							dup = true;
						}
						else
						{
							vert_other_tri[k] = true;
						}
						break;
					}
				}
			}

			if (!dup)
			{
				f->v[num_face_verts++] = t0->v[j];
			}
		}

		for (j = 0; j < 3; j++)
		{
			if (!vert_other_tri[j])
			{
				f->v[num_face_verts++] = t1->v[j];
				break;
			}
		}

	// Find non-shared edges between 2 triangles.
		int num_face_edges = 0;
		bool edge_other_tri[3] = {false, false, false};

		for (j = 0; j < 3; j++)
		{
		// see if edge is already in face.
			bool dup = false;

			for (int k = 0; k < num_face_edges; k++)
			{
				if (t0->e[j] == f->e[k])
				{
					dup = true;
					break;
				}
			}

			if (!dup)
			{
			// see if edge is in other triangle.
				for (int k = 0; k < 3; k++)
				{
					if (t0->e[j] == t1->e[k])
					{
						dup = true;
						edge_other_tri[k] = true;
						break;
					}
				}
			}

			if (!dup)
			{
			// Unique, add to face.
				f->e[num_face_edges++] = t0->e[j];
			}
		}

		for (j = 0; j < 3; j++)
		{
			if (!edge_other_tri[j])
			{
				f->e[num_face_edges++] = t1->e[j];
			}
		}
	}
}
*/

//

VBox::~VBox(void)
{
#if 0
	delete [] tri_face_map;
	tri_face_map = NULL;
	delete [] faces;
	delete [] vert_edges;
	delete [] vert;
	delete [] edge;
	delete [] face;
#endif
}

//

void VBox::adjust(float x, float y, float z)
{
	vertices[0].p.set( x,  y,  z);
	vertices[1].p.set( x, -y,  z);
	vertices[2].p.set(-x, -y,  z);
	vertices[3].p.set(-x,  y,  z);
	vertices[4].p.set( x,  y, -z);
	vertices[5].p.set( x, -y, -z);
	vertices[6].p.set(-x, -y, -z);
	vertices[7].p.set(-x,  y, -z);

// Topology and normals stay the same, but need to adjust VRegion plane constants.

	VVertex * v = vertices;
	for (int i = 0; i < 8; i++, v++)
	{
		VPlane * p = v->planes;
		for (int j = 0; j < v->num_planes; j++, p++)
		{
			p->D = -dot_product(p->N, v->p);
		}
	}

	VEdge * e = edges;
	for (i = 0; i < 12; i++, e++)
	{
	// Compute vertex-edge planes.
		Vertex * v0 = vertices + e->v[0];
		Vertex * v1 = vertices + e->v[1];
		
		e->planes[0].D = -dot_product(v0->p, e->planes[0].N);
		e->planes[1].D = -dot_product(v1->p, e->planes[1].N);

	// Compute face-edge planes.
		VFace * t0 = faces + e->t[0];
		VFace * t1 = faces + e->t[1];

		e->planes[2].D = -dot_product(v0->p, e->planes[2].N);
		e->planes[3].D = -dot_product(v0->p, e->planes[3].N);
	}

	VFace * f = faces;
	for (i = 0; i < 6; i++, f++)
	{
		Vertex * v0 = vertices + f->v[0];

	// Supporting plane for face.
		f->planes[0].D = -dot_product(v0->p, f->planes[0].N);

	// 1 face-edge plane for each edge.
		for (int j = 0; j < 4; j++)
		{
			VEdge * e = edges + f->e[j];
			v0 = vertices + e->v[0];

			f->planes[j+1].D = -dot_product(v0->p, f->planes[j+1].N);
		}
	}
}

//

VCyl::VCyl(float len, float rad)
{
	length = len;
	radius = rad;

// Make circle.
#define CYL_STEPS	16
	Vector circle[CYL_STEPS];
	float theta = 0;
	float dt = 2.0f * 3.14159f / CYL_STEPS;
	for (int i = 0; i < CYL_STEPS; i++)
	{
		circle[i].x = radius * cos(theta);
		circle[i].y = radius * sin(theta);
		theta += dt;
	}

	num_vertices = CYL_STEPS * 2;// + 2;
	vertices = new VVertex[num_vertices];

// NO END RADIALS
	num_edges = CYL_STEPS * 3;//CYL_STEPS * 5;
	edges = new VEdge[num_edges];

// Generate edges.
	VEdge * e = edges;
	for (i = 0; i < CYL_STEPS; i++, e++)
	{
	// verticals.
		e->v[0] = i;
		e->v[1] = i + CYL_STEPS;
	}
#if 0
	for (i = 0; i < CYL_STEPS; i++, e++)
	{
	// TOP radials.
		e->v[0] = num_vertices - 2;
		e->v[1] = i;
	}
	for (i = 0; i < CYL_STEPS; i++, e++)
	{
	// BOTTOM radials.
		e->v[0] = num_vertices - 1;
		e->v[1] = i + CYL_STEPS;
	}
#endif
	for (i = 0; i < CYL_STEPS - 1; i++, e++)
	{
	// TOP corners.
		e->v[0] = i;
		e->v[1] = i + 1;
	}
	e->v[0] = i;
	e->v[1] = 0;
	e++;

	for (i = 0; i < CYL_STEPS - 1; i++, e++)
	{
	// BOTTOM corners.
		e->v[0] = i + CYL_STEPS;
		e->v[1] = i + CYL_STEPS + 1;
	}
	e->v[0] = i + CYL_STEPS;
	e->v[1] = CYL_STEPS;

	float top = 0.5f * length;
	float bottom = -top;

	VVertex * v = vertices;
	for (i = 0; i < CYL_STEPS; i++, v++)
	{
		v->p.set(circle[i].x, circle[i].y, top);
	}

	for (i = 0; i < CYL_STEPS; i++, v++)
	{
		v->p.set(circle[i].x, circle[i].y, bottom);
	}
/*
	vertices[num_vertices - 2].p.set(0, 0, top);
	vertices[num_vertices - 1].p.set(0, 0, bottom);
*/
	num_faces = 2 + CYL_STEPS;//CYL_STEPS * 3;
	faces = new VFace[num_faces];

// Create radial part.
	VFace * f = faces;
	for (i = 0; i < CYL_STEPS - 1; i++, f++)
	{
		f->init(4);
		f->v[0] = i;
		f->v[1] = i + CYL_STEPS;
		f->v[2] = i + CYL_STEPS + 1;
		f->v[3] = i + 1;
	}

	f->init(4);
	f->v[0] = i;
	f->v[1] = i + CYL_STEPS;
	f->v[2] = CYL_STEPS;
	f->v[3] = 0;
	f++;

#if 1
	f->init(CYL_STEPS);
	for (i = 0; i < CYL_STEPS; i++)
	{
		f->v[i] = i;
	}
	f++;

	f->init(CYL_STEPS);
	for (i = 0; i < CYL_STEPS; i++)
	{
		f->v[i] = CYL_STEPS + i;
	}

#else
// Create end caps.
	for (i = 0; i < CYL_STEPS - 1; i++, f++)
	{
		f->init(3);
		f->v[0] = num_vertices - 2;
		f->v[1] = i;
		f->v[2] = i + 1;
	}

	f->init(3);
	f->v[0] = num_vertices - 2;
	f->v[1] = i;
	f->v[2] = 0;
	f++;

	for (i = 0; i < CYL_STEPS - 1; i++, f++)
	{
		f->init(3);
		f->v[0] = num_vertices - 1;
		f->v[1] = i + CYL_STEPS;
		f->v[2] = i + CYL_STEPS + 1;
	}

	f->init(3);
	f->v[0] = num_vertices - 1;
	f->v[1] = i + CYL_STEPS;
	f->v[2] = 0 + CYL_STEPS;
#endif

//
// search for face edges; too painful to hard-code it.
//
	f = faces;
	for (i = 0; i < num_faces; i++, f++)
	{
		int face_edges_found = 0;

	// Find edges.
		for (int v = 0; v < f->num_verts-1; v++)
		{
			int vert0 = f->v[v];
			int vert1 = f->v[v+1];
			VEdge * e = edges;
			for (int j = 0; j < num_edges; j++, e++)
			{
				if ((e->v[0] == vert0 && e->v[1] == vert1) || (e->v[0] == vert1 && e->v[1] == vert0))
				{
					f->e[v] = j;
					face_edges_found++;
					break;
				}
			}
		}

	// close the loop.
		int vert0 = f->v[v];
		int vert1 = f->v[0];
		VEdge * e = edges;
		for (int j = 0; j < num_edges; j++, e++)
		{
			if ((e->v[0] == vert0 && e->v[1] == vert1) || (e->v[0] == vert1 && e->v[1] == vert0))
			{
				f->e[v] = j;
				face_edges_found++;
				break;
			}
		}

		ASSERT(face_edges_found == f->num_verts);
	}

// Set up edge face indices.
	e = edges;
	for (i = 0; i < num_edges; i++, e++)
	{
		int cnt = 0;

		f = faces;
		for (int j = 0; j < num_faces; j++, f++)
		{
			for (int v = 0; v < f->num_verts; v++)
			{
				if (f->e[v] == i)
				{
					ASSERT(cnt < 2);

					e->t[cnt++] = j;
					break;
				}
			}
		}
	}

// 2 endcaps + CS outer faces + CS outer edges + 2 * CS corner edges + 2 * CS corner verts
	num_normals = 2 + CYL_STEPS * 6;
	normals = new Vector[num_normals];

	Vector * N = normals;
	N->set(0, 0, +1);
	N++;
	N->set(0, 0, -1);
	N++;

	f = faces;
	for (i = 0; i < CYL_STEPS; i++, f++, N++)
	{
		f->normal = N - normals;

		const Vector & v0 = vertices[f->v[0]].p;
		const Vector & v1 = vertices[f->v[1]].p;
		const Vector & v2 = vertices[f->v[2]].p;
		const Vector & v3 = vertices[f->v[3]].p;

		*N = 0.25 * (v0 + v1 + v2 + v3);
		N->normalize();
	}
#if 1
	f->normal = 0;
	f++;
	f->normal = 1;
#else
	for (i = 0; i < CYL_STEPS; i++, f++)
	{
		f->normal = 0;
	}
	for (i = 0; i < CYL_STEPS; i++, f++)
	{
		f->normal = 1;
	}
#endif

	ASSERT(N - normals < num_normals);

	e = edges;
	for (i = 0; i < CYL_STEPS; i++, e++, N++)
	{
	// side edges.
		e->n = N - normals;
		const Vector & N0 = normals[faces[e->t[0]].normal];
		const Vector & N1 = normals[faces[e->t[1]].normal];
		*N = 0.5f * (N0 + N1);
		N->normalize();
	}
/* NO RADIALS
	for (i = 0; i < CYL_STEPS; i++, e++)
	{
	// top radial edges.
		e->n = 0;
	}
	for (i = 0; i < CYL_STEPS; i++, e++)
	{
	// bottom radial edges.
		e->n = 1;
	}
*/
	for (i = 0; i < CYL_STEPS*2; i++, e++, N++)
	{
	// top & bottom corner edges.
		e->n = N - normals;
		const Vector & N0 = normals[faces[e->t[0]].normal];
		const Vector & N1 = normals[faces[e->t[1]].normal];
		*N = 0.5f * (N0 + N1);
		N->normalize();
	}

	float inv_r = 1.0f / radius;
	v = vertices;
	for (i = 0; i < CYL_STEPS; i++, v++, N++)
	{
		v->n = N - normals;
		*N = v->p;
		N->z = 0;
		*N *= inv_r;
		N->z = 1;
		N->normalize();
	}
	for (i = 0; i < CYL_STEPS; i++, v++, N++)
	{
		v->n = N - normals;
		*N = v->p;
		N->z = 0;
		*N *= inv_r;
		N->z = -1;
		N->normalize();
	}
/*
	vertices[num_vertices - 2].n = 0;
	vertices[num_vertices - 1].n = 1;
*/
	ASSERT(N - normals == num_normals);

	compute_vregions();

// DEBUG check vregions.
// for each feature, verify that the center of that feature is inside its voronoi region.
	const float epsilon = -1e-5;
	v = vertices;
	for (i = 0; i < num_vertices; i++, v++)
	{
		for (int j = 0; j < v->num_planes; j++)
		{
			float d = v->planes[j].compute_distance(v->p);
			ASSERT(d >= epsilon);
		}
	}
	e = edges;
	for (i = 0; i < num_edges; i++, e++)
	{
		Vector c = 0.5f * (vertices[e->v[0]].p + vertices[e->v[1]].p);
		for (int j = 0; j < e->num_planes; j++)
		{
			float d = e->planes[j].compute_distance(c);
			ASSERT(d >= epsilon);
		}
	}
	f = faces;
	for (i = 0; i < num_faces; i++, f++)
	{
		Vector c(0, 0, 0);
		for (int j = 0; j < f->num_verts; j++)
		{
			c += vertices[f->v[j]].p;
		}
		c /= f->num_verts;
		for (j = 0; j < f->num_planes; j++)
		{
			float d = f->planes[j].compute_distance(c);
			ASSERT(d >= epsilon);
		}
	}
}

//

void VCyl::adjust(float len, float rad)
{
	if (length != len || radius != rad)
	{
		float scalar = rad / radius;

		length = len;
		radius = rad;

		float top = 0.5f * length;
		float bottom = -top;

		VVertex * v = vertices;
		for (int i = 0; i < CYL_STEPS; i++, v++)
		{
			v->p.z = 0;
			v->p *= scalar;	// scale radius.
			v->p.z = top;	// scale length;
		}
		for (i = 0; i < CYL_STEPS; i++, v++)
		{
			v->p.z = 0;
			v->p *= scalar;		// scale radius.
			v->p.z = bottom;	// scale length;
		}
/*
		vertices[num_vertices - 2].p.set(0, 0, top);
		vertices[num_vertices - 1].p.set(0, 0, bottom);
*/
	// Now we need to adjust all voronoi planes.

		v = vertices;
		for (i = 0; i < num_vertices; i++, v++)
		{
			VPlane * p = v->planes;
			for (int j = 0; j < v->num_planes; j++, p++)
			{
				p->D = -dot_product(p->N, v->p);
			}
		}

		VEdge * e = edges;
		for (i = 0; i < num_edges; i++, e++)
		{
		// Compute vertex-edge planes.
			Vertex * v0 = vertices + e->v[0];
			Vertex * v1 = vertices + e->v[1];
			
			e->planes[0].D = -dot_product(v0->p, e->planes[0].N);
			e->planes[1].D = -dot_product(v1->p, e->planes[1].N);

			e->planes[2].D = -dot_product(v0->p, e->planes[2].N);
			e->planes[3].D = -dot_product(v0->p, e->planes[3].N);
		}

		VFace * f = faces;
		for (i = 0; i < num_faces; i++, f++)
		{
			Vertex * v0 = vertices + f->v[0];

		// Supporting plane for face.
			f->planes[0].D = -dot_product(v0->p, f->planes[0].N);

		// 1 face-edge plane for each edge.
			for (int j = 0; j < f->num_verts; j++)
			{
				VEdge * e = edges + f->e[j];
				v0 = vertices + e->v[0];

				f->planes[j+1].D = -dot_product(v0->p, f->planes[j+1].N);
			}
		}

	}
}

//