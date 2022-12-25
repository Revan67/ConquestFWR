//
//
//

#ifndef CMESH_H
#define CMESH_H

//

#include <stdlib.h>
#include "geom.h"

//

struct Vertex
{
	Vector	p;
	int		n;
};

//

struct Edge
{
	int	v[2];	// indices into vertex list.
	int	t[2];	// indices into triangle list.
	int	n;		// index into normal list.
};

//

struct Face
{
	int n;		// num verts.
	int v[4];	// indices into face list.
	int e[4];	// indices into edge list.
	int normal;	// index into normal list.

	Face(void)
	{
		memset(this, 0xff, sizeof(*this));
	}
};

//

struct Triangle
{
	int v[3];	// indices into vertex list.
	int e[3];	// indices into edge list.
	int	normal;	// index into normal list.

	Triangle(void)
	{
		memset(this, 0xff, sizeof(*this));
	}
};

//

struct CollisionMesh : public GeometricPrimitive
{
	int			num_vertices;
	Vertex *	vertices;

	int			num_edges;
	Edge *		edges;

	int			num_triangles;
	Triangle *	triangles;

	float *		triangle_d;

	int			num_normals;
	Vector *	normals;

	Vector		centroid;

	CollisionMesh(void)
	{
		num_vertices = 0;
		vertices = NULL;

		num_edges = 0;
		edges = NULL;

		num_triangles = 0;
		triangles = NULL;

		triangle_d = 0;

		num_normals = 0;
		normals = NULL;
	}

	~CollisionMesh(void)
	{
		free();
	}

	void free (void)
	{
		delete [] vertices;
		vertices = 0;
		delete [] edges;
		edges = 0;
		delete [] triangles;
		triangles = 0;
		delete [] triangle_d;
		triangle_d = 0;
		delete [] normals;
		normals = 0;
	}

// THIS IS RIDICULOUS.
	Triangle * find_face(int v1, int v2, int v3) const
	{
		Triangle * result = NULL;

		Triangle * t = triangles;
		for (int i = 0; i < num_triangles; i++, t++)
		{
			if (((t->v[0] == v1) && (t->v[1] == v2) && (t->v[2] == v3)) ||
				((t->v[0] == v1) && (t->v[1] == v3) && (t->v[2] == v2)) ||
				((t->v[0] == v2) && (t->v[1] == v1) && (t->v[2] == v3)) ||
				((t->v[0] == v2) && (t->v[1] == v3) && (t->v[2] == v1)) ||
				((t->v[0] == v3) && (t->v[1] == v1) && (t->v[2] == v2)) ||
				((t->v[0] == v3) && (t->v[1] == v2) && (t->v[2] == v1)))
			{
				result = t;
				break;
			}
		}

		return result;
	}

	virtual void transform(GeometricPrimitive * dst, const Transform & T) const
	{
		CollisionMesh * mesh = (CollisionMesh *) dst;
		memcpy(mesh, this, sizeof(*this));
		//mesh->center = trans;
		//mesh->R = rot;
	}

	void compute_edges(void);
	void compute_normals(bool convex = true);

	void copy(const CollisionMesh * src)
	{
		num_vertices = src->num_vertices;
		vertices = new Vertex[num_vertices];
		memcpy(vertices, src->vertices, sizeof(Vertex) * num_vertices);

		num_edges = src->num_edges;
		edges = new Edge[num_edges];
		memcpy(edges, src->edges, sizeof(Edge) * num_edges);

		num_triangles = src->num_triangles;
		triangles = new Triangle[num_triangles];
		memcpy(triangles, src->triangles, sizeof(Triangle) * num_triangles);

		triangle_d = new float[num_triangles];
		memcpy(triangle_d, src->triangle_d, sizeof(float) * num_triangles);

		num_normals = src->num_normals;
		normals = new Vector[num_normals];
		memcpy(normals, src->normals, sizeof(Vector) * num_normals);

		centroid = src->centroid;
	}
};

//

#endif