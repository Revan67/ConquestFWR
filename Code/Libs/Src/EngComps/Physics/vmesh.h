#ifndef VMESH_H
#define VMESH_H

//


#include "geom.h"
#include "cmesh.h"

//

struct VertexEdgeList
{
	int e[3];
};

//

struct Quad
{
	int		num_verts;
	int *	v;
	int *	e;
//	int v[4];
//	int e[4];
	int normal;

	Quad(void)
	{
		memset(this, 0, sizeof(*this));
	}

	~Quad(void)
	{
		delete [] v;
		delete [] e;
	}

	void init(int n)
	{
		num_verts = n;
		v = new int[n];
		e = new int[n];
	}
};

//

struct VPlane : public Plane
{
	struct Feature * neighbor;
};

//

struct VRegion
{
	int		id;

	int		num_planes;
	VPlane *planes;

	//CollisionMesh * mesh;

	VRegion(void)
	{
		memset(this, 0, sizeof(*this));
	}

	~VRegion(void)
	{
		if (planes)
		{
			delete [] planes;
			planes = NULL;
		}
	}


// PREV and NEXT IGNORE THE 0th plane, which is the face's supporting plane.
	inline VPlane * prev(VPlane * p)
	{
		int idx = p - planes;
		if (idx >= num_planes - 1)
		{
			return planes + 1;
		}
		else
		{
			return p + 1;
		}
	}

	inline VPlane * next(VPlane * p)
	{
		int idx = p - planes;
		if (idx > 1)
		{
			return p - 1;
		}
		else
		{
			return planes + num_planes - 1;
		}
	}
};

//

struct Feature : public VRegion
{
	enum {VERT, EDGE, FACE} type;

	bool operator == (const Feature & f) const
	{
		return (type == f.type && id == f.id);
	}
};

//

struct VVertex : public Feature, public Vertex
{
	VVertex(void)
	{
		type = VERT;
	}
};

struct VEdge : public Feature, public Edge
{
	VVertex * verts;

	VEdge(void)
	{
		type = EDGE;
	}

	Vector get_v0(void) const
	{
		return verts[v[0]].p;
	}
	Vector get_v1(void) const
	{
		return verts[v[1]].p;
	}
};

struct VFace : public Feature, public Quad
{
//	int			num_verts;
	VVertex *	verts;

	VFace(void)
	{
		type = FACE;
	}
	
	Vector get_vert(int i) const
	{
		return verts[v[i]].p;
	}
};

//

struct VMesh
{
	int			num_vertices;
	VVertex *	vertices;

	int			num_edges;
	VEdge *		edges;

	int			num_faces;
	VFace *		faces;

	int			num_normals;
	Vector *	normals;

	VMesh(void)
	{
		memset(this, 0, sizeof(*this));
	}

	VMesh(const CollisionMesh * mesh);
	~VMesh(void);

	void compute_vregions(void);
};

//

#endif