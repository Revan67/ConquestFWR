//
//
//

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>

#include "3dmath.h"
#include "filesys.h"


#include "fileutil.h"
#include "cmesh.h"

#include "fdump.h"

//

void GenerateCylinder(CollisionMesh * mesh, float len, float radius)
{
	ASSERT(mesh);

// Make circle.
#define CYL_STEPS	16
	Vector circle[CYL_STEPS];
	float theta = 0.0f;
	float dt = 2.0f * 3.14159f / CYL_STEPS;
	for (int i = 0; i < CYL_STEPS; i++)
	{
		circle[i].x = radius * cos(theta);
		circle[i].y = radius * sin(theta);
		theta += dt;
	}

	mesh->num_vertices = CYL_STEPS * 2 + 2;
	mesh->vertices = new Vertex[mesh->num_vertices];

	float top = len / 2.0f;
	float bottom = -top;

	Vertex * v = mesh->vertices;
	for (i = 0; i < CYL_STEPS; i++, v++)
	{
		v->p.set(circle[i].x, circle[i].y, top);
	}

	for (i = 0; i < CYL_STEPS; i++, v++)
	{
		v->p.set(circle[i].x, circle[i].y, bottom);
	}

	mesh->vertices[mesh->num_vertices - 2].p.set(0, 0, top);
	mesh->vertices[mesh->num_vertices - 1].p.set(0, 0, bottom);

	mesh->num_triangles = CYL_STEPS * 4;
	mesh->triangles = new Triangle[mesh->num_triangles];

// Create radial part.
	Triangle * t = mesh->triangles;
	for (i = 0; i < CYL_STEPS - 1; i++)
	{
		t->v[0] = i;
		t->v[1] = i + CYL_STEPS;
		t->v[2] = i + 1;
		t++;

		t->v[0] = i + 1;
		t->v[1] = i + CYL_STEPS;
		t->v[2] = i + CYL_STEPS + 1;
		t++;
	}

	t->v[0] = i;
	t->v[1] = i + CYL_STEPS;
	t->v[2] = 0;
	t++;

	t->v[0] = 0;
	t->v[1] = i + CYL_STEPS;
	t->v[2] = CYL_STEPS;
	t++;

// Create end caps.
	for (i = 0; i < CYL_STEPS - 1; i++, t++)
	{
		t->v[0] = mesh->num_vertices - 2;
		t->v[1] = i;
		t->v[2] = i + 1;
	}

	t->v[0] = mesh->num_vertices - 2;
	t->v[1] = i;
	t->v[2] = 0;
	t++;

	for (i = 0; i < CYL_STEPS - 1; i++, t++)
	{
		t->v[0] = mesh->num_vertices - 1;
		t->v[1] = i + CYL_STEPS;
		t->v[2] = i + CYL_STEPS + 1;
	}

	t->v[0] = mesh->num_vertices - 1;
	t->v[1] = i + CYL_STEPS;
	t->v[2] = 0 + CYL_STEPS;

	mesh->compute_edges();
	mesh->compute_normals();
}

//
//
// Use hacked linear search to insert determine unique edges...
//
static int FindEdge(int v1, int v2, Edge * edges, int n)
{
	ASSERT(edges);

	int result = -1;

	Edge * e = edges;
	for (int i = 0; i < n; i++, e++)
	{
		if (((e->v[0] == v1) && (e->v[1] == v2)) ||
			((e->v[1] == v1) && (e->v[0] == v2)))
		{
			result = i;
			break;
		}
	}

	return result;
}

//

static bool AddEdge(int n, Edge * list, int v1, int v2, int face, int & edge_index)
{
	ASSERT(list);

	bool result;

// See if we've already encountered this edge in another triangle.
	int e = FindEdge(v1, v2, list, n);
	if (e != -1)
	{
	// Edge already in list, add face reference.
		list[e].t[1] = face;
		result = false;
		edge_index = e;
	}
	else
	{
	// First time this edge was encountered.
		Edge * edge = list + n;
		edge->v[0] = v1;
		edge->v[1] = v2;
		edge->t[0] = face;
		result = true;
		edge_index = n;
	}

	return result;
}

//

void CollisionMesh::compute_edges(void)
{
// Euler's formula relates the number of faces, vertices, and edges FOR A 
// SOLID POLYHEDRON.

	//num_edges = num_triangles + num_vertices - 2;
	//edges = new Edge[num_edges];

	int max_edges = num_triangles * 3;
	Edge * temp_edges = new Edge[max_edges];

	int edges_computed = 0;

	Triangle * t = triangles;
	for (int i = 0; i < num_triangles; i++, t++)
	{
		int e0, e1, e2;
		if (AddEdge(edges_computed, temp_edges, t->v[0], t->v[1], i, e0))
		{
			edges_computed++;
		}

		if (AddEdge(edges_computed, temp_edges, t->v[1], t->v[2], i, e1))
		{
			edges_computed++;
		}

		if (AddEdge(edges_computed, temp_edges, t->v[2], t->v[0], i, e2))
		{
			edges_computed++;
		}

		t->e[0] = e0;
		t->e[1] = e1;
		t->e[2] = e2;
	}

	num_edges = edges_computed;
	edges = new Edge[num_edges];
	memcpy(edges, temp_edges, sizeof(Edge) * num_edges);
	delete [] temp_edges;

//	ASSERT(edges_computed == num_edges);
}

//

int RemapDuplicateVectors(int * remap, Vector * src, int n);

//

void CollisionMesh::compute_normals(bool convex)
{
	int max_normals = num_vertices + num_triangles + num_edges;

	normals = new Vector[max_normals];
	memset(normals, 0, sizeof(Vector) * max_normals);

	centroid.zero();
	Vertex * v = vertices;
	for (int i = 0; i < num_vertices; i++, v++)
	{
		centroid += v->p;
	}

	centroid /= float(num_vertices);

//
// Adopt convention that normal list contains list of vertex normals, followed
// by list of face normals, followed by list of edge normals. 
// So start with normals[num_vertices] here, since we compute face normals first.
//
	Vector * n = normals + num_vertices;
	Triangle * t = triangles;
	for (i = 0; i < num_triangles; i++, n++, t++)
	{
		Vector r1 = vertices[t->v[1]].p - vertices[t->v[0]].p;
		Vector r2 = vertices[t->v[2]].p - vertices[t->v[0]].p;
		*n = cross_product(r1, r2);
		n->normalize();

		if (convex)
		{
		// Check outward facing. Object is CONVEX, so we can easily check this.
			Vector check = vertices[t->v[0]].p - centroid;
			if (dot_product(*n, check) < 0.0f)
			{
			// Negate normal, swap vertices.
				*n = -*n;
				int temp = t->v[1];
				t->v[1] = t->v[2];
				t->v[2] = temp;
			}
		}
		t->normal = i + num_vertices;
	}

//
// Now compute vertex normals from adjacent face normals.
//
	t = triangles;
	for (i = 0; i < num_triangles; i++, t++)
	{
		Vector * face_normal = normals + t->normal;
		for (int j = 0; j < 3; j++)
		{
			normals[t->v[j]] += *face_normal;
		}
	}

	Vector * vn = normals;
	for (i = 0; i < num_vertices; i++, vn++)
	{
		vn->normalize();
	}

	Edge * e = edges;
	n = normals + num_vertices + num_triangles;
	for (i = 0; i < num_edges; i++, e++, n++)
	{
		Vector * n0 = normals + triangles[e->t[0]].normal;
		Vector * n1 = normals + triangles[e->t[1]].normal;
		*n = 0.5f * (*n0 + *n1);
		e->n = n - normals;
	}

// Now we have the master normal list. Remove duplicates and remap face and
// vertices accordingly.
num_normals = max_normals;

	int * remap = new int[max_normals];
	int num_unique_normals = RemapDuplicateVectors(remap, normals, max_normals);

	if (num_unique_normals == max_normals)
	{
		num_normals = max_normals;
		for (int i = 0; i < num_vertices; i++)
		{
			vertices[i].n = i;
		}
	}
	else
	{
	// Remap vertex normals.
		Vertex * v = vertices;
		for (i = 0; i < num_vertices; i++, v++)
		{
			if (remap[i] != -1)
			{
				//vertex_normals[i] = remap[i];
				vertices[i].n = remap[i];
			}
			else
			{
				//vertex_normals[i] = i;
				vertices[i].n = i;
			}
		}

	// Remap face normals.
		t = triangles;
		for (i = 0; i < num_triangles; i++, t++)
		{
			if (remap[t->normal] != -1)
			{
				t->normal = remap[t->normal];
			}
		}

	// Remap edge normals.
		e = edges;
		for (i = 0; i < num_edges; i++, e++)
		{
			if (remap[e->n] != -1)
			{
				e->n = remap[e->n];
			}
		}

	// NOW REMOVE UNUSED NORMALS.
		bool * normal_used = new bool[max_normals];
		memset(normal_used, 0, sizeof(bool) * max_normals);
		//int * vn = vertex_normals;
		Vertex * vn = vertices;
		for (i = 0; i < num_vertices; i++, vn++)
		{
			//normal_used[*vn] = true;
			normal_used[vn->n] = true;
		}

		t = triangles;
		for (i = 0; i < num_triangles; i++, t++)
		{
			normal_used[t->normal] = true;
		}

		e = edges;
		for (i = 0; i < num_edges; i++, e++)
		{
			normal_used[e->n] = true;
		}

		num_normals = 0;
		bool * nu = normal_used;
		for (i = 0; i < max_normals; i++, nu++)
		{
			if (*nu)
			{
				num_normals++;
			}
		}

		if (num_normals != max_normals)
		{
			Vector * new_normals = new Vector[num_normals];
			memset(remap, 0xff, sizeof(int) * max_normals);

			nu = normal_used;
			Vector * n = new_normals;
			for (i = 0; i < max_normals; i++, nu++)
			{
				if (*nu)
				{
					*n = normals[i];
					remap[i] = n - new_normals;
					n++;
				}
			}

			delete [] normals;
			normals = new_normals;

		// Now remap everyone.
			vn = vertices;
			for (i = 0; i < num_vertices; i++, vn++)
			{
				vn->n = remap[vn->n];
			}

			t = triangles;
			for (i = 0; i < num_triangles; i++, t++)
			{
				t->normal = remap[t->normal];
			}

			e = edges;
			for (i = 0; i < num_edges; i++, e++)
			{
				e->n = remap[e->n];
			}
		}

		delete [] normal_used;
	}

	delete [] remap;

// Compute face d-coefficents for plane testing.
	triangle_d = new float[num_triangles];
	float * d = triangle_d;
	t = triangles;
	for (i = 0; i < num_triangles; i++, t++, d++)
	{
		Vector * v = &vertices[t->v[0]].p;
		Vector * N = normals + t->normal;
		*d = -dot_product(*v, *N);
	}
}

//
