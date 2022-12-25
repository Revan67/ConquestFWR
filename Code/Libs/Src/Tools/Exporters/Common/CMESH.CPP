#include <stdio.h>

#ifndef SGI
#pragma warning( 3 : 4100 ) // unreferenced formal parameter
#pragma warning( 3 : 4189 ) // local variable is initialized but not referenced
#pragma warning( error : 4701 ) // variable may be used without having been initialized
#pragma warning( error : 4700 )
#pragma warning( 3 : 4706 ) // assignment within conditional expression
#endif

#include "cmesh.h"

void ComputeConvexHull(CollisionMesh & mesh, const Vector * v, int n, unsigned int stride)
{
	mesh.free();
    //mesh.Init();

	double * vx = new double[n];
	double * vy = new double[n];
	double * vz = new double[n];

	double * x = vx;
	double * y = vy;
	double * z = vz;

    int i;

	unsigned char * vp = (unsigned char *) v;
	for (i = 0; i < n; i++, vp += stride, x++, y++, z++)
	{
		*x = ((Vector *) vp)->x;
		*y = ((Vector *) vp)->y;
		*z = ((Vector *) vp)->z;
	}

	int num_triangles;
	CHTriangle * triangles = NULL;

	if (ConvexHull3D(n, vx, vy, vz, num_triangles, triangles) &&
            (num_triangles > 0))
	{
		mesh.num_triangles = num_triangles;
		mesh.triangles = new Triangle[num_triangles];
	//
	// Count and identify vertices used in convex hull.
	//
		bool * vert_used = new bool[n];
		memset(vert_used, 0, n * sizeof(bool));
		Triangle * tri = mesh.triangles;
		for (i = 0; i < num_triangles; i++, tri++)
		{
			int v1 = triangles[i][0];
			int v2 = triangles[i][1];
			int v3 = triangles[i][2];

			vert_used[v1] = 
			vert_used[v2] =
			vert_used[v3] = true;

			tri->v[0] = v1;
			tri->v[1] = v2;
			tri->v[2] = v3;
		}

		mesh.num_vertices = 0;
		bool * vu = vert_used;
		for (i = 0; i < n; i++, vu++)
		{
			if (*vu)
			{
				mesh.num_vertices++;
			}
		}

	//
	// Allocate hull vertices, copy used vertices from initial vertex list.
	//
		int * remap = new int[n];
		memset(remap, 0xff, sizeof(int) * n);

		mesh.vertices = new Vertex[n];
		vu = vert_used;
		Vertex * vec = mesh.vertices;
		vp = (unsigned char *) v;
		for (i = 0; i < n; i++, vu++)
		{
			if (*vu)
			{
				vec->p = *((Vector *) (vp + i * stride));

			// Build remapping table from initial vertex list to new reduced vertex list.
				remap[i] = vec - mesh.vertices;
				vec++;
			}
		}

	// Now remap triangle vertex indices.

		tri = mesh.triangles;
		for (i = 0; i < mesh.num_triangles; i++, tri++)
		{
			tri->v[0] = remap[tri->v[0]];
			tri->v[1] = remap[tri->v[1]];
			tri->v[2] = remap[tri->v[2]];
		}

		delete [] vert_used;
		delete [] remap;

		mesh.compute_edges();
		mesh.compute_normals();
	}
	else
	{
	  // ERROR.
	  fprintf(stderr, "Error: ConvexHull3D failed. Increase i1!");
	  delete [] vx;
	  delete [] vy;
	  delete [] vz;
   	  delete [] triangles;
	  num_triangles = 0;
      return ;
	}

	delete [] vx;
	delete [] vy;
	delete [] vz;
	delete [] triangles;
	
	/* old
        // mirror w/ parallel arrays for cleaner export	
        mesh.vertices_p = new Vector[mesh.num_vertices];
        mesh.vertices_n = new int[mesh.num_vertices];
        for(i=0; i<mesh.num_vertices; i++){
          mesh.vertices_p[i]=mesh.vertices[i].p;
          mesh.vertices_n[i]=mesh.vertices[i].n;
        }

        mesh.edges_v2 = new int[2*mesh.num_edges];
        mesh.edges_t2 = new int[2*mesh.num_edges];
        for(i=0; i<mesh.num_edges; i++){
          mesh.edges_v2[2*i]=mesh.edges[i].v[0];
          mesh.edges_v2[2*i+1]=mesh.edges[i].v[1];
          mesh.edges_t2[2*i]=mesh.edges[i].t[0];
          mesh.edges_t2[2*i+1]=mesh.edges[i].t[1];
        }
 
        mesh.triangles_v3 = new int[3*mesh.num_triangles];
        mesh.triangles_e3 = new int[3*mesh.num_triangles];
        mesh.triangles_n = new int[mesh.num_triangles];
        for(i=0; i<mesh.num_triangles; i++){
          mesh.triangles_v3[3*i]=mesh.triangles[i].v[0];
          mesh.triangles_v3[3*i+1]=mesh.triangles[i].v[1];
          mesh.triangles_v3[3*i+2]=mesh.triangles[i].v[2];
          mesh.triangles_e3[3*i]=mesh.triangles[i].e[0];
          mesh.triangles_e3[3*i+1]=mesh.triangles[i].e[1];
          mesh.triangles_e3[3*i+2]=mesh.triangles[i].e[2];
          mesh.triangles_n[i]=mesh.triangles[i].normal;
        }
	*/

	mesh.tr.identity();
}

// The construction of a convex hull in 3D is a modification of code
// written by Dave Watson.  It uses an algorithm described in
//
//     Watson, D.F., 1981, Computing the n-dimensional Delaunay 
//     tessellation with application to Voronoi polytopes: 
//     The Computer J., 24(2), p. 167-172.

//---------------------------------------------------------------------------
int ConvexHull3D (int N, double* x, double* y, double* z, 
				  int& numTriangles, CHTriangle*& triangle)
{
	int result;

	double** point = new double*[N+4];
	int i;
	for (i = 0; i < N+4; i++)
		point[i] = new double[3];
	for (i = 0; i < N; i++)
	{
		point[i][0] = x[i];
		point[i][1] = y[i];
		point[i][2] = z[i];
	}

	const double EPSILON = 0.00001;
	const int TSIZE = 75;
	const double RANGE = 10.0;

	double xmin = point[0][0], xmax = xmin;
	double ymin = point[0][1], ymax = ymin;
	double zmin = point[0][2], zmax = zmin;

	for (i = 1; i < N; i++)
	{
		double value = point[i][0];
		if ( xmax < value )
			xmax = value;
		if ( xmin > value )
			xmin = value;

		value = point[i][1];
		if ( ymax < value )
			ymax = value;
		if ( ymin > value )
			ymin = value;

		value = point[i][2];
		if ( zmax < value )
			zmax = value;
		if ( zmin > value )
			zmin = value;
	}

	double xrange = xmax-xmin, yrange = ymax-ymin, zrange = zmax-zmin;
	double maxrange = xrange;
	if ( maxrange < yrange )
		maxrange = yrange;
	if ( maxrange < zrange )
		maxrange = zrange;

	// need to scale the data later to do a correct tetrahedron count
	//double maxrange3 = maxrange*maxrange*maxrange;

	// tweak the points by very small random numbers
	double bgs = 2.0*EPSILON*maxrange;
	srand(367);
	for (i = 0; i < N; i++)
	{
		point[i][0] += bgs*(0.5 - rand()/double(RAND_MAX));
		point[i][1] += bgs*(0.5 - rand()/double(RAND_MAX));
		point[i][2] += bgs*(0.5 - rand()/double(RAND_MAX));
	}

	double wrk[3][4] =
	{
		{ 8*RANGE, -RANGE, -RANGE, -RANGE },
		{ -RANGE, 8*RANGE, -RANGE, -RANGE },
		{ -RANGE, -RANGE, 8*RANGE, -RANGE }
	};
	for (i = 0; i < 4; i++) 
	{
		point[N+i][0] = xmin+xrange*wrk[0][i];
		point[N+i][1] = ymin+yrange*wrk[1][i];
		point[N+i][2] = zmin+zrange*wrk[2][i];
	}

	int i0, i1, i2, i3, i4, i5, i6, i7, i8, i9, i11;
	int nts, ii[3];
	double xx;

	int tsz = 6*TSIZE;
	int** tmp = new int*[tsz+1];
	tmp[0] = new int[3*(tsz+1)];
	for (i0 = 1; i0 < tsz+1; i0++)
		tmp[i0] = tmp[0] + 3*i0;

	// Estimate of how many tetrahedrons there can be.  Since theoretically
	// the number is O(N^2), this could be quite large.  You may need to
	// increase i1 if a call to this function fails...
	i1 = 6*(N+6);
	// i1 = 16*N;  // Had to increase i1 in the case of N=3000 randomly generated
	               // points in [0,10]^3

	int* id = new int[i1];
	for (i0 = 0; i0 < i1; i0++)
		id[i0] = i0;

	int** a3s = new int*[i1];
	a3s[0] = new int[4*i1];
	for (i0 = 1; i0 < i1; i0++)
		a3s[i0] = a3s[0] + 4*i0;
	a3s[0][0] = N;
	a3s[0][1] = N+1;
	a3s[0][2] = N+2;
	a3s[0][3] = N+3;

	double** ccr = new double*[i1];  // circumscribed centers and radii
	ccr[0] = new double[4*i1];
	for (i0 = 1; i0 < i1; i0++)
		ccr[i0] = ccr[0] + 4*i0;
	ccr[0][0] = 0.0;
	ccr[0][1] = 0.0;
	ccr[0][2] = 0.0;
	ccr[0][3] = FLT_MAX;

	nts = 1;  // number of tetrahedra
	i4 = 1;

	// compute tetrahedralization
	for (i0 = 0; i0 < N; i0++)
	{  
		i1 = i7 = -1;
		i9 = 0;
		for (i11 = 0; i11 < nts; i11++)
		{  
			i1++;
			while ( a3s[i1][0] < 0 ) 
				i1++;
			xx = ccr[i1][3];
			for (i2 = 0; i2 < 3; i2++)
			{
				double z = point[i0][i2]-ccr[i1][i2];
				xx -= z*z;
				if ( xx < 0 ) 
					goto Corner3;
			}
			i9--;
			i4--;
			id[i4] = i1;
			for (i2 = 0; i2 < 4; i2++)
			{
				ii[0] = 0;
				if ( ii[0] == i2 ) 
					ii[0]++;
				for (i3 = 1; i3 < 3; i3++)
				{
					ii[i3] = ii[i3-1] + 1;
					if ( ii[i3] == i2 ) 
						ii[i3]++;
				}
				if ( i7 > 2 )
				{
					i8 = i7;
					for (i3 = 0; i3 <= i8; i3++)
					{
						for (i5 = 0; i5 < 3; i5++) 
							if ( a3s[i1][ii[i5]] != tmp[i3][i5] ) 
								goto Corner1;
						for (i6 = 0; i6 < 3; i6++) 
							tmp[i3][i6] = tmp[i8][i6];
						i7--;
						goto Corner2;
Corner1:;
					}
				}
	            if ( ++i7 > tsz )
				{
					// temporary storage exceeded, increase TSIZE
					result = 0;
					goto ExitConvexHull;
				}
				for (i3 = 0; i3 < 3; i3++) 
					tmp[i7][i3] = a3s[i1][ii[i3]];
Corner2:;
			}
			a3s[i1][0] = -1;
Corner3:;
		}

		for (i1 = 0; i1 <= i7; i1++)
		{
			if ( tmp[i1][0] >= N )
			{
				for (i2 = 0; i2 < 3; i2++)
				{
					for (wrk[i2][3] = 0, i3 = 0; i3 < 3; i3++)
					{
						wrk[i2][i3] = point[tmp[i1][i2]][i3] - point[i0][i3];
						wrk[i2][3] += 0.5*wrk[i2][i3]*
							(point[tmp[i1][i2]][i3]+point[i0][i3]);
					}
				}
				
				xx = 
					(wrk[0][0]*(wrk[1][1]*wrk[2][2]-wrk[2][1]*wrk[1][2])) -
					(wrk[0][1]*(wrk[1][0]*wrk[2][2]-wrk[2][0]*wrk[1][2])) +
					(wrk[0][2]*(wrk[1][0]*wrk[2][1]-wrk[2][0]*wrk[1][1]));
				ccr[id[i4]][0] = 
					((wrk[0][3]*(wrk[1][1]*wrk[2][2]-wrk[2][1]*wrk[1][2])) -
					(wrk[0][1]*(wrk[1][3]*wrk[2][2]-wrk[2][3]*wrk[1][2])) +
					(wrk[0][2]*(wrk[1][3]*wrk[2][1]-wrk[2][3]* wrk[1][1])))/xx;
				ccr[id[i4]][1] = 
					((wrk[0][0]*(wrk[1][3]*wrk[2][2]-wrk[2][3]*wrk[1][2])) -
					(wrk[0][3]*(wrk[1][0]*wrk[2][2]-wrk[2][0]*wrk[1][2])) +
					(wrk[0][2]*(wrk[1][0]*wrk[2][3]-wrk[2][0]*wrk[1][3])))/xx;
				ccr[id[i4]][2] = 
					((wrk[0][0]*(wrk[1][1]*wrk[2][3]-wrk[2][1]*wrk[1][3])) -
					(wrk[0][1]*(wrk[1][0]*wrk[2][3]-wrk[2][0]*wrk[1][3])) +
					(wrk[0][3]*(wrk[1][0]*wrk[2][1]-wrk[2][0]*wrk[1][1])))/xx;
				
				for (ccr[id[i4]][3] = 0, i2 = 0; i2 < 3; i2++) 
				{
					double z = point[i0][i2] - ccr[id[i4]][i2];
					ccr[id[i4]][3] += z*z;
					a3s[id[i4]][i2] = tmp[i1][i2];
				}
				
				a3s[id[i4]][3] = i0;
				i4++;
				i9++;
			}
		}
		nts += i9;
	}

	// count the number of triangular faces
	numTriangles = 0;
	i0 = -1;
	for (i11 = 0; i11 < nts; i11++)
	{
		i0++;
		while ( a3s[i0][0] < 0 )
			i0++;

		if ( a3s[i0][1] < N )
			numTriangles++;
	}

	// create the triangles
	triangle = new CHTriangle[numTriangles];

	numTriangles = 0;
	i0 = -1;
	for (i11 = 0; i11 < nts; i11++)
	{
		i0++;
		while ( a3s[i0][0] < 0 )
			i0++;

		if ( a3s[i0][1] < N )
		{
			CHTriangle& tri = triangle[numTriangles];
			tri[0] = a3s[i0][1];
			tri[1] = a3s[i0][2];
			tri[2] = a3s[i0][3];
			numTriangles++;
		}
	}

	result = 1;

ExitConvexHull:
	delete[] tmp[0];
	delete[] tmp;
	delete[] id;
	delete[] a3s[0];
	delete[] a3s;
	delete[] ccr[0];
	delete[] ccr;
	for (i = 0; i < N+4; i++)
		delete[] point[i];
	delete[] point;

	return result;
}

//
//
// Use hacked linear search to insert determine unique edges...
//
static int FindEEdge(int v1, int v2, EEdge * edges, int n)
{
	int result = -1;

	EEdge * e = edges;
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

static bool AddEEdge(int n, EEdge * list, int v1, int v2, int face, int & edge_index)
{
	bool result;

// See if we've already encountered this edge in another triangle.
	int e = FindEEdge(v1, v2, list, n);
	if (e != -1)
	{
	// EEdge already in list, add face reference.
		list[e].t[1] = face;
		result = false;
		edge_index = e;
	}
	else
	{
	// First time this edge was encountered.
		EEdge * edge = list + n;
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
//
// Euler's formula relates the number of faces, vertices, and edges for a 
// closed polyhedron.
//
	//num_edges = num_triangles + num_vertices - 2;
	//edges = new Edge[num_edges];

	int max_edges = num_triangles * 3;
	EEdge * temp_edges = new EEdge[max_edges];

	int edges_computed = 0;

	Triangle * t = triangles;
	for (int i = 0; i < num_triangles; i++, t++)
	{
		int e0, e1, e2;
		if (AddEEdge(edges_computed, temp_edges, t->v[0], t->v[1], i, e0))
		{
			edges_computed++;
		}

		if (AddEEdge(edges_computed, temp_edges, t->v[1], t->v[2], i, e1))
		{
			edges_computed++;
		}

		if (AddEEdge(edges_computed, temp_edges, t->v[2], t->v[0], i, e2))
		{
			edges_computed++;
		}

		t->e[0] = e0;
		t->e[1] = e1;
		t->e[2] = e2;
	}

	num_edges = edges_computed;
	edges = new EEdge[num_edges];
	memcpy(edges, temp_edges, sizeof(EEdge) * num_edges);
	delete [] temp_edges;

//	assert(edges_computed == num_edges);
}

/*
void CollisionMesh::compute_edges(void)
{
// Euler's formula relates the number of faces, vertices, and edges.
	num_edges = num_triangles + num_vertices - 2;

	edges = new EEdge[num_edges];

	int edges_computed = 0;

	Triangle * t = triangles;
	for (int i = 0; i < num_triangles; i++, t++)
	{
		int e0, e1, e2;
		if (AddEEdge(edges_computed, edges, t->v[0], t->v[1], i, e0))
		{
			edges_computed++;
		}

		if (AddEEdge(edges_computed, edges, t->v[1], t->v[2], i, e1))
		{
			edges_computed++;
		}

		if (AddEEdge(edges_computed, edges, t->v[2], t->v[0], i, e2))
		{
			edges_computed++;
		}

		t->e[0] = e0;
		t->e[1] = e1;
		t->e[2] = e2;
	}

//	assert(edges_computed == num_edges);
}
*/

//


//
//#pragma optimize( "", off )
void CollisionMesh::compute_normals(void)
{
	int max_normals = num_vertices + num_triangles;

	normals = new Vector[max_normals];
	memset(normals, 0, sizeof(Vector) * max_normals);

//
// Adopt convention that normal list contains list of vertex normals, followed
// by list of face normals. So start with normals[num_vertices] here.
//
	centroid.zero();
	Vertex * v = vertices;
	for (int i = 0; i < num_vertices; i++, v++)
	{
		centroid += v->p;
	}

	centroid /= float(num_vertices);

	Vector * n = normals + num_vertices;
	Triangle * t = triangles;
	for (i = 0; i < num_triangles; i++, n++, t++)
	{
		Vector r1 = vertices[t->v[1]].p - vertices[t->v[0]].p;
		Vector r2 = vertices[t->v[2]].p - vertices[t->v[0]].p;
		*n = cross_product(r1, r2);
		//n->normalize();

		{
		  double magnitude = sqrt(n->x*n->x + n->y*n->y + n->z*n->z);

		  if(magnitude)
		  {
			magnitude = 1.0 / magnitude;
			n->x = (float)(n->x * magnitude);
			n->y = (float)(n->y * magnitude);
			n->z = (float)(n->z * magnitude);
		  }
		  else
		  {
			  // TODO: remove face
			  fprintf(stderr, "Zero length normal in CollisionMesh!\n");
		  }
		}


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

// Now we have the master normal list. Remove duplicates and remap face and
// vertices accordingly.
/*
vertex_normals = new int[num_vertices];
for (i = 0; i < num_vertices; i++)
{
	vertex_normals[i] = i;
}
*/
num_normals = max_normals;

	int * remap = new int[max_normals];
	int num_unique_normals = RemapDuplicateVectors(remap, normals, max_normals);

	if (num_unique_normals == max_normals)
	{
		num_normals = max_normals;

		//vertex_normals = new int[num_vertices];
		for (int i = 0; i < num_vertices; i++)
		{
			//vertex_normals[i] = i;
			vertices[i].n = i;
		}
	}
	else
	{
	// Remap vertex normals.
		//vertex_normals = new int[num_vertices];
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
			//vn = vertex_normals;
			vn = vertices;
			for (i = 0; i < num_vertices; i++, vn++)
			{
				//*vn = remap[*vn];
				vn->n = remap[vn->n];
			}

			t = triangles;
			for (i = 0; i < num_triangles; i++, t++)
			{
				t->normal = remap[t->normal];
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
//#pragma optimize( "", on )


//
// Remove duplicate vectors from a list, fill in remap table.
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
				if (v1->equal(*v2, 1e-4f))
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

void DumpCollisionMesh(const CollisionMesh& mesh)
{
  int i;

  printf("num_vertices=%d\n",mesh.num_vertices);
  for(i=0; i<mesh.num_vertices; i++){
    printf("vertex %d n=%d %f %f %f\n",
      i, mesh.vertices[i].n, mesh.vertices[i].p.x, mesh.vertices[i].p.y,
      mesh.vertices[i].p.z);
  }

  printf("num_edges=%d\n", mesh.num_edges);
  for(i=0; i<mesh.num_edges; i++){
    printf("edge %d n=%d   v=%d,%d t=%d,%d\n",
      i, mesh.edges[i].n,
      mesh.edges[i].v[0], mesh.edges[i].v[1],
      mesh.edges[i].t[0], mesh.edges[i].t[1]);

  }

  printf("num_triangles=%d\n",mesh.num_triangles);
  for(i=0; i<mesh.num_triangles; i++){
    printf("%d v=%d,%d,%d e=%d,%d,%d normal=%d D=%f\n",
    i, mesh.triangles[i].v[0], mesh.triangles[i].v[1], mesh.triangles[i].v[2],
    mesh.triangles[i].e[0], mesh.triangles[i].e[1], mesh.triangles[i].e[2],
    mesh.triangles[i].normal,
    mesh.triangle_d[i]);
  }

  printf("num_normals=%d\n",mesh.num_normals);
  for(i=0; i<mesh.num_normals; i++){
    printf("%d normal=%f,%f,%f\n",
    i, mesh.normals[i].x, mesh.normals[i].y, mesh.normals[i].z);
  }

  printf("centroid %f %f %f\n",mesh.centroid.x, mesh.centroid.y, mesh.centroid.z);

}

#if 0
void CollisionMesh::transform(GeometricPrimitive * dst, 
  const Vector & trans, const Matrix & rot) const
{
	CollisionMesh * mesh = (CollisionMesh *) dst;
	memcpy(mesh, this, sizeof(*this));
	mesh->center = trans;
	mesh->R = rot;
}
#endif

//
//
