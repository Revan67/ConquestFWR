#ifndef CMESH_H
#define CMESH_H

#include <stdlib.h>
#include <float.h>
//#include "geom.h"
#include "matrix.h"
#include "vector.h"
#include "persistmath.h"

#ifdef SGI
#ifndef bool
typedef unsigned char bool;
#endif
#endif

#ifndef true
#define true ((bool)1)
#endif

#ifndef false
#define false ((bool)0)
#endif

typedef int CHTriangle[3];

//

struct Vertex
{
	Vector	p;
	int	n;
};

//

struct EEdge
{
	int	v[2];	// indices into vertex list.
	int	t[2];	// indices into triangle list.
	int	n;		// index into normal list.

	EEdge(void)
	{
		memset(this, 0xff, sizeof(*this));
	}
};

//

struct FFace
{
	int n;		// num verts.
	int v[4];	// indices into face list.
	int e[4];	// indices into edge list.
	int normal;	// index into normal list.

	FFace(void)
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

struct CollisionMesh //: public GeometricPrimitive
{
	char name[64];
	int		num_vertices;
	Vertex *	vertices;
        //Vector      *vertices_p; // mirrors Vertex
        //int         *vertices_n; // mirrors Vertex

	int		num_edges;
	EEdge *		edges;
        //int         *edges_v2; // v[2]
        //int         *edges_t2; // t[2]


	int		num_triangles;
	Triangle *	triangles;
	float *		triangle_d;
        //int         *triangles_v3; // v[3]
		//int         *triangles_e3; // e[3]
        //int         *triangles_n; // normal


	int		num_normals;
	Vector *	normals;

	Vector		centroid;

	Vector		center;
	Matrix		R;
	PersistTransform tr;
	float volume;

	void Init(void)
	{
		memset(this, 0, sizeof(*this));
        R.set_identity();
	}

	CollisionMesh(void)
	{
		Init();
	}

	~CollisionMesh(void)
	{
		//free(); // freed later explicitly by FreeObject
	}

	void free (void)
	{
		delete [] vertices;
		delete [] edges;
		delete [] triangles;
		delete [] triangle_d;
		delete [] normals;
		Init();
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

	//virtual void transform(GeometricPrimitive * dst, const Vector & trans,
                               //const Matrix & rot) const;

	void compute_edges(void);
	void compute_normals(void);
};

void ComputeConvexHull(CollisionMesh & mesh, const Vector * v, int n, 
                       unsigned int stride = sizeof(Vector));

int ConvexHull3D(int N, double* x, double* y, double* z, 
				 int & numTriangles, 
				 CHTriangle *& triangle);

int RemapDuplicateVectors(int * remap, Vector * src, int n);

void DumpCollisionMesh(const CollisionMesh & mesh);

#endif
