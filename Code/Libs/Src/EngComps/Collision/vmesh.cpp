//
//
//

#include "fdump.h"
#include "VMesh.h"

//

VMesh::VMesh(const CollisionMesh * mesh)
{
	if (mesh)
	{
		num_vertices = mesh->num_vertices;
		vertices = new VVertex[num_vertices];

		num_edges = mesh->num_edges;
		edges = new VEdge[num_edges];

		num_faces = mesh->num_triangles;
		faces = new VFace[num_faces];

		num_normals = mesh->num_normals;
		normals = new Vector[num_normals];
		memcpy(normals, mesh->normals, sizeof(Vector) * num_normals);

		for (int i = 0; i < num_vertices; i++)
		{
			vertices[i].p = mesh->vertices[i].p;
			vertices[i].n = mesh->vertices[i].n;
		}
		for (i = 0; i < num_edges; i++)
		{
			edges[i].v[0]	= mesh->edges[i].v[0];
			edges[i].v[1]	= mesh->edges[i].v[1];
			edges[i].t[0]	= mesh->edges[i].t[0];
			edges[i].t[1]	= mesh->edges[i].t[1];
			edges[i].n		= mesh->edges[i].n;
		}
		for (i = 0; i < num_faces; i++)
		{
			faces[i].init(3);
			for (int j = 0; j < faces[i].num_verts; j++)
			{
				faces[i].v[j] = mesh->triangles[i].v[j];
				faces[i].e[j] = mesh->triangles[i].e[j];
			}
			faces[i].normal = mesh->triangles[i].normal;
		}

		compute_vregions();
	}
}

//

VMesh::~VMesh(void)
{
	if (vertices)
	{
		delete [] vertices;
		vertices = NULL;
	}
	if (edges)
	{
		delete [] edges;
		edges = NULL;
	}
	if (faces)
	{
		delete [] faces;
		faces = NULL;
	}				 
	if (normals)
	{
		delete [] normals;
		normals = NULL;
	}
}

//

void VMesh::compute_vregions(void)
{
// Compute vertex voronoi regions.
	VVertex * vtx = vertices;
	for (int i = 0; i < num_vertices; i++, vtx++)
	{
		vtx->id = i;

	// Find edges containing vertex.
		int num_vtx_edges = 0;
		int vtx_edges[32];

		VEdge * e = edges;
		for (int k = 0; k < num_edges; k++, e++)
		{
			if ((e->v[0] == i) || (e->v[1] == i))
			{
				vtx_edges[num_vtx_edges++] = k;
			}
		}

	// Compute vertex-edge planes.
		vtx->num_planes = num_vtx_edges;
		vtx->planes = new VPlane[vtx->num_planes];

		VPlane * p = vtx->planes;
		int edges_found = 0;
		for (k = 0; k < num_vtx_edges; k++)
		{
			e = edges + vtx_edges[k];

		// Compute plane that contains vertex, normal to edge.
			VVertex * other = vertices + ((e->v[0] == i) ? e->v[1] : e->v[0]);

			Vector N = vtx->p - other->p;
			p->init(vtx->p, N);
			p->neighbor = &edges[e - edges];
			p++;
		}
	}

// Compute edge voronoi regions.

	VEdge * e = edges;
	for (i = 0; i < num_edges; i++, e++)
	{
		e->id = i;
		e->verts = vertices;

	// Each edge has 4 planes: 2 vertex-edge, 2 face-edge.
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

	// face-edge plane normal is parallel to face, perpendicular to edge.
		Vector N0 = cross_product(normals[t0->normal], N);
		Vector N1 = cross_product(normals[t1->normal], -N);

		e->planes[2].init(v0->p, N0);
		e->planes[2].neighbor = faces + e->t[0];
		e->planes[3].init(v0->p, N1);
		e->planes[3].neighbor = faces + e->t[1];

	// TEST
		Vector vtest = 0.5f * (v0->p + v1->p);
		vtest *= 1.1f;

		for (int j = 0; j < e->num_planes; j++)
		{
			float d = e->planes[j].compute_distance(vtest);
			if (d < 0.0f)
			{
				e->planes[j].N *= -1.0f;
			}
		}
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