#include "pch.h"

#include "SimpleMesh.h"
#include "Globals.h"

#include <FileSys.h>
#include <TSmartPointer.h>

#include <CMesh.h>

void SMesh::MakeEdges()
{
	int this_edge;
	SEdge *temp_e_list = new SEdge[3*f_cnt];
	f_edge_list = new U16[3*f_cnt];
	for (int f=0;f<f_cnt;f++)
	{
		for (int ee=0;ee<3;ee++)
		{
			this_edge = e_cnt;
			//find edge
			for (int e=0;e<e_cnt;e++)
			{
				if (temp_e_list[e].v[0] == f_list[f].v[ee] && temp_e_list[e].v[1] == f_list[f].v[(ee+1)%3])
				{
					this_edge = e;

				}
				else if (temp_e_list[e].v[1] == f_list[f].v[ee] && temp_e_list[e].v[0] == f_list[f].v[(ee+1)%3])
				{
					this_edge = e;
				}
			}
			
			//making a new edge
			if (this_edge == e_cnt)
			{
				temp_e_list[this_edge].f[0] = f;
				temp_e_list[this_edge].v[0] = f_list[f].v[ee];
				temp_e_list[this_edge].v[1] = f_list[f].v[(ee+1)%3];
				e_cnt++;
			}
			else
			{
				temp_e_list[this_edge].f[1] = f;
			}

			f_edge_list[f*3+ee] = this_edge;

		}
	}

	e_list = new SEdge[e_cnt];
	memcpy(e_list,temp_e_list,e_cnt*sizeof(SEdge));
	delete [] temp_e_list;
}


int SMesh::GetNeighborOnEdge(int f,int e)
{
	if (e_list[f_edge_list[f*3+e]].f[0] == f)
		return e_list[f_edge_list[f*3+e]].f[1];
	else
		return e_list[f_edge_list[f*3+e]].f[0];
}

BOOL32 SMesh::save(const char *fileName)
{
	BOOL32 result=0;

	COMPTR<IFileSystem> file,file2;
	DAFILEDESC fdesc = fileName;
	fdesc.lpImplementation = "UTF";
	fdesc.dwDesiredAccess = GENERIC_READ|GENERIC_WRITE;
	fdesc.dwCreationDistribution = CREATE_ALWAYS;
	fdesc.dwShareMode = 0;
	DWORD dwWritten;

	if (DACOM->CreateInstance(&fdesc,file) != GR_OK)
		goto Done;

	fdesc.lpFileName = "Faces";
	fdesc.lpImplementation = "DOS";
	if (file->CreateInstance(&fdesc,file2) != GR_OK)
		goto Done;

	file2->WriteFile(0,&f_cnt,sizeof(f_cnt),&dwWritten);
	file2->WriteFile(0,f_list,sizeof(SFace)*f_cnt,&dwWritten);

	fdesc.lpFileName = "Vertices";
	if (file->CreateInstance(&fdesc,file2) != GR_OK)
		goto Done;

	file2->WriteFile(0,&v_cnt,sizeof(v_cnt),&dwWritten);
	file2->WriteFile(0,v_list,sizeof(SVertex)*v_cnt,&dwWritten);

	result = 1;

Done:
	return result;
}

BOOL32 SMesh::load(const char *fileName)
{
	BOOL32 result=0;

	COMPTR<IFileSystem> file,file2;
	DAFILEDESC fdesc = fileName;
	fdesc.lpImplementation = "UTF";
	fdesc.dwDesiredAccess = GENERIC_READ;
	fdesc.dwCreationDistribution = OPEN_EXISTING;
	fdesc.dwShareMode = 0;
	DWORD dwRead;

	if (DACOM->CreateInstance(&fdesc,file) != GR_OK)
		goto Done;

	fdesc.lpFileName = "Faces";
	fdesc.lpImplementation = "DOS";
	if (file->CreateInstance(&fdesc,file2) != GR_OK)
		goto Done;

	file2->ReadFile(0,&f_cnt,sizeof(f_cnt),&dwRead);
	f_list = new SFace[f_cnt];
	file2->ReadFile(0,f_list,sizeof(SFace)*f_cnt,&dwRead);

	fdesc.lpFileName = "Vertices";
	if (file->CreateInstance(&fdesc,file2) != GR_OK)
		goto Done;

	file2->ReadFile(0,&v_cnt,sizeof(v_cnt),&dwRead);
	v_list = new SVertex[v_cnt];
	file2->ReadFile(0,v_list,sizeof(SVertex)*v_cnt,&dwRead);

	result = 1;

Done:
	return result;
}

BOOL32 SMesh::load(IFileSystem *file)
{
	COMPTR<IFileSystem> file2;
	DAFILEDESC fdesc;
	DWORD dwRead;
	BOOL32 result=0;

	fdesc.lpFileName = "Faces";
	fdesc.lpImplementation = "DOS";
	if (file->CreateInstance(&fdesc,file2) != GR_OK)
		goto Done;

	file2->ReadFile(0,&f_cnt,sizeof(f_cnt),&dwRead);
	f_list = new SFace[f_cnt];
	file2->ReadFile(0,f_list,sizeof(SFace)*f_cnt,&dwRead);

	fdesc.lpFileName = "Vertices";
	if (file->CreateInstance(&fdesc,file2) != GR_OK)
		goto Done;

	file2->ReadFile(0,&v_cnt,sizeof(v_cnt),&dwRead);
	v_list = new SVertex[v_cnt];
	file2->ReadFile(0,v_list,sizeof(SVertex)*v_cnt,&dwRead);

	result = 1;

Done:
	return result;
}

void SMesh::MakeCollisionMesh(CollisionMesh *mesh)
{
	mesh->num_vertices = v_cnt;
	mesh->vertices = new Vertex[v_cnt];

	int vc;
	for (vc=0;vc<v_cnt;vc++)
	{
		mesh->vertices[vc].p = v_list[vc].pt;
	}

	mesh->num_triangles = f_cnt;
	mesh->triangles = new Triangle[f_cnt];

	for (int fc=0;fc<f_cnt;fc++)
	{
		for (vc=0;vc<3;vc++)
			mesh->triangles[fc].v[vc] = f_list[fc].v[vc];
	}

	mesh->compute_edges();
	mesh->compute_normals();
}




//
//
// Use hacked linear search to insert determine unique edges...
//
static int FindEdge(int v1, int v2, Edge * edges, int n)
{
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


static bool AddEdge(int n, Edge * list, int v1, int v2, int face, int & edge_index)
{
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

//
// Returns number of unique vectors in list.
//
int RemapDuplicateVectors(int * remap, Vector * src, int n)
{
// Locate duplicate vertices.
	memset(remap, 0xff, sizeof(int) * n);

// Assume they're all unique.
	int num_unique = n;

// Loop through all vertices, 
	Vector * v1 = src;
	for (int i = 0; i < n; i++, v1++)
	{
	// comparing each to all the remaining vertices.

		if (remap[i] == -1)	// don't count already remapped vectors multiple times.
		{
			Vector * v2 = v1 + 1;
			for (int j = i + 1; j < n; j++, v2++)
			{
				if (v1->equal(*v2, 1e-4))
				{
				// We've got a duplicate.
					int back = i;

				// Traverse duplicates back to first in case of multiple duplicates.
					while (remap[back] != -1)
					{
						back = remap[back];
					}

				// Record index that this vertex duplicates.
					remap[j] = back;

				// One less unique vertex...
					num_unique--;
				}
			}
		}
	}

	return num_unique;
}
//

void CollisionMesh::compute_normals(bool convex)
{
	int max_normals = num_vertices + num_triangles + num_edges;

	normals = new Vector[max_normals];
	memset(normals, 0, sizeof(Vector) * max_normals);

	centroid.zero();
	Vertex * v = vertices;
	int i;
	for (i = 0; i < num_vertices; i++, v++)
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
			if (dot_product(*n, check) < 0)
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
		*n = 0.5 * (*n0 + *n1);
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

void SMesh::Sort(const Vector &n)
{
	U16 re_index_list[600];
	SINGLE values[600];
	//memset(re_index_list,sizeof(U16)*600);
	int f;
	for (f=0;f<f_cnt;f++)
	{
		re_index_list[f] = f;
		values[f] = dot_product(v_list[f_list[f].v[0]].pt,n);
	}

	for (int i=0;i<f_cnt-1;i++)
	{
		for (int j=i;j<f_cnt;j++)
		{
			U16 iref = re_index_list[i];
			U16 jref = re_index_list[j];

			if (values[i] > values[j])
			{
				SINGLE temp_val = values[i];
				values[i] = values[j];
				values[j] = temp_val;

				re_index_list[i] = jref;
				re_index_list[j] = iref;
			}
		}
	}

	SFace *new_f_list = new SFace[f_cnt];
	for (f=0;f<f_cnt;f++)
	{
		new_f_list[f] = f_list[re_index_list[f]];
	}
	delete [] f_list;
	f_list = new_f_list;
}