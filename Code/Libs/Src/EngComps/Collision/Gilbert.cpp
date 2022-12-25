//
//

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <float.h>
#include <stdio.h>
#include <string.h>

#include "3dmath.h"
#include "cmesh.h"
//#include "rigid.h"
#include "Debugprint.h"

//

bool GlobalHack = false;

//
//#define VERBOSE
#undef VERBOSE

//

struct Simplex
{
	int		n;
	Vector	v[4];
	float	lambda[4];

	Vertex *p[4];
	Vertex *q[4];

	Vector compute_point(void) const
	{
		Vector result(0.0f, 0.0f, 0.0f);
		const Vector * p = v;
		const float * l = lambda;
		for (int i = 0; i < n; i++, p++, l++)
		{
			result += (*p) * (*l);
		}
		return result;
	}
};

//

int NumPermutations[] = {0, 1, 3, 7, 15};

//
// List of indices contained in each subset. First number is size of subset.
//
int Subsets[15][5]; 

//
// List of indices NOT contained in each subset.
//
int Complements[15][5];

// Largest element of each subset.
int MaxElement[15];

//

//Vector SolveSimplex(Vector v[4], int n, float lambda[4]);
Vector SolveSimplex(Simplex & s);

//

static bool Started = FALSE;

//

void ComputeSubsets(void)
{
	for (int i = 0; i < 15; i++)
	{
		int num_elements = 0;
		int subset = i + 1;

		for (int n = 0; n < 4; n++)
		{
			if (subset & 1)
			{
				Subsets[i][num_elements + 1] = n;
				num_elements++;
			}
			else
			{
				Complements[i][n - num_elements + 1] = n;
			}
			
			subset >>= 1;
		}

		Subsets[i][0] = num_elements;
		Complements[i][0] = 4 - num_elements;

		MaxElement[i] = Subsets[i][Subsets[i][0]];
	}
}

//

float	Delta[15][4];
//float	DeltaSum[15];

//

float ComputeDelta(Vector v[4], int subset, int index)
{
	float result;

	int * set = Subsets[subset];
	int size = set[0];

	if (size == 1)
	{
		result = float(1);
	}
	else
	{
	// Find subset one smaller which does not contain index.

		int smaller_set = ((subset + 1) ^ ( 1 << index)) - 1;
		int * smset = Subsets[smaller_set];

		float sum = 0.0f;
		const Vector & zy = v[smset[1]];
		const Vector & x = v[index];

		for (int i = 1; i <= smset[0]; i++)
		{
			int idx = smset[i];

		// No need to recursively compute subset deltas, because we're guaranteed that deltas of all relevant subsets
		// are already computed.

			const Vector & y = v[idx];
			float yz = dot_product(y, zy);
			float yx = dot_product(y, x);

			sum += Delta[smaller_set][idx] * (yz - yx);
		}

		result = sum;
	}

	return result;
}

//

static int gjk_backup(const Simplex & simplex)
{
	int num_subsets = NumPermutations[simplex.n];

	float distsq_num[32], distsq_den[32];
	int bests = 0;
	for (int s = 1; s < num_subsets; s++)
	{
		int * set = Subsets[s];

		int size = set[0];
		float delta_set = 0.0f;
		for (int j = 1; j <= size; j++)
		{
			int idx = set[j];
			if (Delta[s][idx] <= 0)
			{
				break;
			}
			delta_set += Delta[s][idx];
		}

		if (j <= size || delta_set <= 0)
		{
			continue;
		}

		distsq_num[s] = 0;
		for (j = 1; j <= size; j++)
		{
			for (int k = 1; k <= size; k++)
			{											   						   							
				distsq_num[s] += Delta[s][set[j]] * Delta[s][set[k]] * dot_product(simplex.v[set[j]], simplex.v[set[k]]);
			}
		}

		distsq_den[s] = delta_set * delta_set;

		if ((bests < 1) || (distsq_num[s] * distsq_den[bests]) < ( distsq_num[bests] * distsq_den[s]))
			bests = s;
	}	

	return bests;
}


//

Vector SolveSimplex(Simplex & s)
{
	int num_subsets = NumPermutations[s.n];

	memset(s.lambda, 0, sizeof(float) * 4);	
	memset(Delta, 0, sizeof(float) * num_subsets * 4);

	for (int i = 0; i < num_subsets; i++)
	{
		int * set = Subsets[i];
		for (int j = 1; j <= set[0]; j++)
		{
			int idx = set[j];
			Delta[i][idx] = ComputeDelta(s.v, i, idx);
		}
	}


// Now examine table of deltas to find set which meets conditions.
	const float tolerance = 1e-5;

	int winner = -1;
	for (i = 0; i < num_subsets; i++)
	{
		bool candidate = TRUE;

		int * set = Subsets[i];

		//DeltaSum[i] = 0;

		for (int j = 1; j <= set[0]; j++)
		{
			int idx = set[j];

			//DeltaSum[i] += Delta[i][idx];
			if (Delta[i][idx] <= tolerance)
			{
				candidate = FALSE;
				break;
			}
		}

		if (candidate)
		{
			int * css = Complements[i];

			int cnt = s.n - set[0];
			for (int j = 1; j <= cnt; j++)
			{
				int idx = css[j];

			// Find superset of S which is the union of S and idx.

				int superset = ((i + 1) | (1 << idx)) - 1;

				if (Delta[superset][idx] > tolerance)
				{
					candidate = FALSE;
					break;
				}
			}
		}

		if (candidate/* && DeltaSum[i] >= 1e-20*/)
		{
			winner = i;
			break;
		}
	}

	Vector result;
	result.zero();

	bool exhaustive = (winner == -1);

	if (winner == -1)
	{
	// Exhaustive search necessary.
	PHYTRACE10("\nPerforming exhaustive search in FindClosestPoints().\n");

//DebugPrint("backup\n");
		winner = gjk_backup(s);
	/*
		float min_dist = FLT_MAX;

		for (int i = 0; i < num_subsets; i++)
		{
			int * set = Subsets[i];

			int size = set[0];
			int k = set[1];

			float delta_set = 0.0f;
			for (int j = 1; j <= size; j++)
			{
				int idx = set[j];
				delta_set += Delta[i][idx];
			}

			float dot_sum = 0.0f;
			for (j = 1; j <= size; j++)
			{
				int idx = set[j];
				dot_sum += Delta[i][idx] * dot_product(s.v[idx], s.v[k]);
			}

	if (fabs(delta_set) < 1e-5)
	{
		DebugPrint("ouch\n");
	}

			float dist = dot_sum / delta_set;
			if (dist > 0 && dist < min_dist)
			{
				min_dist = dist;
				winner = i;
			}
		}
	*/
	}

	int * set = Subsets[winner];

	float sum = 0;
	for (i = 1; i <= set[0]; i++)
	{
		int idx = set[i];
		sum += Delta[winner][idx];
	}

// Compute lambdas.
	for (i = 1; i <= set[0]; i++)
	{
		int idx = set[i];

		int slot = i - 1;

		if (idx != slot)
		{
			s.v[slot] = s.v[idx];
			s.p[slot] = s.p[idx];
			s.q[slot] = s.q[idx];
		}

		s.lambda[slot] = Delta[winner][idx] / sum;
/*
if (s.lambda[slot] < 1e-5)
{
	DebugPrint("ouch ");
	if (exhaustive)
	{
		DebugPrint("exhaustive");
	}
	DebugPrint("\n");
}
*/

		result += s.v[slot] * s.lambda[slot];

	}

	s.n = set[0];

	return result;
}

//

#define EPSILON				(1e-6)
#define EPSILON_SQUARED		(EPSILON * EPSILON)

static Vertex * saved[32][2];
static int num_saved;

//
// DEBUG HACK DRAW SHIT.
//

void HackDrawPoint(const Vector & pt, int r, int g, int b, float radius)
{
}

//
#ifdef CAMERON
#define DIM				3
#define DIM_PLUS_ONE	DIM+1
REAL gjk_distance(int npts1, REAL (* pts1)[DIM], INDEX * rings1, REAL (* tr1)[DIM_PLUS_ONE],
				  int npts2, REAL (* pts2)[DIM], INDEX * rings2, REAL (* tr2)[DIM_PLUS_ONE],
				  REAL wpt1[DIM], REAL wpt2[DIM], struct simplex_point * simplex, int use_seed);
#endif

//
// Returns true if results are valid, false otherwise (probably because objects are interpenetrating).
//
bool FindClosestPoints(const CollisionMesh & p, const CollisionMesh & q, const Matrix & Rp, const Vector & rp, const Matrix & Rq, const Vector & rq, Vector & v1, Vector & v2, Vector & normal)
{
#ifdef CAMERON
float ptemp[64][3];
float qtemp[64][3];
for (int i = 0; i < p.num_vertices; i++)
{
	ptemp[i][0] = p.vertices[i].p.x;
	ptemp[i][1] = p.vertices[i].p.y;
	ptemp[i][2] = p.vertices[i].p.z;
}
for (i = 0; i < q.num_vertices; i++)
{
	qtemp[i][0] = q.vertices[i].p.x;
	qtemp[i][1] = q.vertices[i].p.y;
	qtemp[i][2] = q.vertices[i].p.z;
}
float Tp[3][4], Tq[3][4];
for (i = 0; i < 3; i++)
{
	for (int j = 0; j < 3; j++)
	{
		Tp[i][j] = Rp.d[i][j];
		Tq[i][j] = Rq.d[i][j];
	}
}
Tp[0][3] = rp.x; Tp[1][3] = rp.y; Tp[2][3] = rp.z;
Tq[0][3] = rq.x; Tq[1][3] = rq.y; Tq[2][3] = rq.z;
float dist = gjk_distance(p.num_vertices, ptemp, NULL, NULL,
						  q.num_vertices, qtemp, NULL, NULL,
						  &v1.x, &v2.x, NULL, 0);

v1 = Rp.get_transpose() * (v1 - rp);
v2 = Rq.get_transpose() * (v2 - rq);

return true;
#else

	bool penetrating = false;

	if (!Started)
	{
		ComputeSubsets();
		Started = TRUE;
	}

	num_saved = 0;

	Simplex s;
	memset(&s, 0, sizeof(s));

// Start with single point chosen randomly from TCSO.
	s.n = 1;

	saved[num_saved][0] = s.p[0] = p.vertices;
	saved[num_saved][1] = s.q[0] = q.vertices;
	num_saved++;

// Initial simplex point is trivial.
	Vector x = s.v[0] = (rq + Rq * s.q[0]->p) - (rp + Rp * s.p[0]->p);
	s.lambda[0] = 1.0f;

	int num_iterations = 0;
/*
if (GlobalHack)
{
	DrawTCSO(p, q, Rp, rp, Rq, rq);
}
*/
	bool done = false;
	while (!done)
	{
		num_iterations++;

		float dist_squared = dot_product(x, x);

		if ((dist_squared < EPSILON_SQUARED) || (s.n == 4))
		{
			done = true;
		}
		else
		{
		// See if x is really the closest point. Find vertex from p that maximizes x dot vp,
		// and vertex from q that minimizes x dot vq.

			Vertex * pv = p.vertices;
			Vertex * a;
			float max_a = -FLT_MAX;

		// Express x in p's frame for dot products with p's vertices.
			Vector xp = x * Rp;	// NOTE ORDER. Using transpose of Rp.
			for (int i = 0; i < p.num_vertices; i++, pv++)
			{
				float xa = dot_product(xp, pv->p);
				if (xa > max_a)
				{
					max_a = xa;
					a = pv;
				}
			}

			Vertex * qv = q.vertices;
			Vertex * b;
			float min_b = FLT_MAX;

		// Express x in q's frame for dot products with q's vertices.
			Vector xq = x * Rq; // NOTE ORDER. Using transpose of Rq.
			for (i = 0; i < q.num_vertices; i++, qv++)
			{
				float xb = dot_product(xq, qv->p);
				if (xb < min_b)
				{
					min_b = xb;
					b = qv;
				}
			}
#if 1
		// "has this pair appeared before?"
			for (i = 0; i < num_saved; i++)
			{
				if ((saved[i][0] == a) && (saved[i][1] == b))
				{
//					PHYTRACE13("Exiting FindClosestPoints() because of recurring point. last gv = %g, max_a = %g, min_b = %g\n", gv, max_a, min_b);
					done = true;
					break;
				}
			}
#endif

			if (!done)
			{
			// Do G-test.
			// We've already compute most of the dot products needed for gv...
				float gv = dist_squared - min_b + max_a;
				gv -= dot_product(x, rq);
				gv += dot_product(x, rp);

				if (gv <= EPSILON)
				{
					PHYTRACE13("Exiting FindClosestPoints() normally. gv = %g, max_a = %g, min_b = %g\n", gv, max_a, min_b);
					done = true;
				}
				else
				{
				// Save for future reference.
					saved[num_saved][0] = s.p[s.n] = a;
					saved[num_saved][1] = s.q[s.n] = b;
					num_saved++;

					s.v[s.n] = (rq + Rq * b->p) - (rp + Rp * a->p);
					s.lambda[s.n] = 0;
					s.n++;
					
				// Find closest point as convex sum of s.v[] points.
					x = SolveSimplex(s);
				}
			}

		}

	}

//DebugPrint("GJK iterations: %d\n", num_iterations);

#define COMPUTE_NORMAL	1
/*
	if (s.n == 4)
	{
		penetrating = true;

#if COMPUTE_NORMAL
		normal = x - rq;
		normal.normalize();

#endif

		v1.zero();
		v2.zero();
		for (i = 0; i < s.n; i++)
		{
			v1 += s.p[i]->p * s.lambda[i];
			v2 += s.q[i]->p * s.lambda[i];
		}

#ifdef VERBOSE
		DebugPrint("penetrating\n");
#endif
	}
	else
*/
	{
		v1.zero();
		v2.zero();
		for (int i = 0; i < s.n; i++)
		{
			v1 += s.p[i]->p * s.lambda[i];
			v2 += s.q[i]->p * s.lambda[i];
		}
/*
Vector p1 = rp + Rp * v1;
Vector p2 = rq + Rq * v2;
Vector dv = p1 - p2;
float ddv = dot_product(dv, dv);
if (ddv > 1.0f)
{
//	DebugPrint("ouch.\n");
}

HackDrawPoint(rp + Rp * v1, 255, 128, 128, 0.25);
HackDrawPoint(rq + Rq * v2, 128, 128, 255, 0.25);
// DEBUG
for (i = 0; i < s.n; i++)
{
	Vector v = rp + Rp * s.p[i]->p;
	HackDrawPoint(v, 255, 0, 0, 0.25);
	v = rq + Rq * s.q[i]->p;
	HackDrawPoint(v, 0, 0, 255, 0.25);
}
*/
#if COMPUTE_NORMAL
	// Now to compute the normal. Need to count unique p and q vertices
	// in simplex.
		Vertex * up[4];
		Vertex * uq[4];

		int num_up = 0;
		int num_uq = 0;

		for (i = 0; i < s.n; i++)
		{
			bool found = false;
			for (int j = 0; j < num_up; j++)
			{
				if (s.p[i] == up[j])
				{
					found = true;
					break;
				}
			}

			if (!found)
			{
				up[num_up] = s.p[i];
				num_up++;
			}

			found = false;
			for (j = 0; j < num_uq; j++)
			{
				if (s.q[i] == uq[j])
				{
					found = true;
					break;
				}
			}

			if (!found)
			{
				uq[num_uq] = s.q[i];
				num_uq++;
			}
		}

		Vertex ** v;
		int n;
		const CollisionMesh * m;
		float f;
		const Matrix * R;

		bool using_p;

		if (num_uq >= num_up)
		{
			v = uq;
			n = num_uq;
			m = &q;
			f = 1.0f;
			R = &Rq;
			using_p = false;
		}
		else
		{
			v = up;
			n = num_up;
			m = &p;
			f = -1.0f;
			R = &Rp;
			using_p = true;
		}

		if (n == 4)
		{
		// Average normals of 4 vertices. 
			normal = m->normals[v[0]->n] + m->normals[v[1]->n] + m->normals[v[2]->n] + m->normals[v[3]->n];
			float nmag = normal.magnitude();
			if (nmag > 1e-5)
			{
				normal /= nmag;
			}
		}
		else if (n == 3)
		{
		// Compute outward normal of triangle containing 3 vertices. May not
		// be a face of the actual mesh.
			Vector nv1 = v[1]->p - v[0]->p;
			Vector nv2 = v[2]->p - v[0]->p;
			normal = cross_product(nv1, nv2);
			normal.normalize();
			Vector check = v[0]->p - m->centroid;
			if (dot_product(check, normal) < 0.0f)
			{
				normal = -normal;
			}
		}
		else if (n == 2)
		{
		// Average normals. Not perfect.
			normal = 0.5f * (m->normals[v[0]->n] + m->normals[v[1]->n]);

			float nms = dot_product(normal, normal);
			if (nms < 1e-5)
			{
			// hack workaround:
				normal = m->normals[v[0]->n];
			/*
			// THE FOLLOWING CODE MAY BE A GOOD WORKAROUND, BUT I HAVE NO GOOD TEST CASE
			// AT THIS TIME.
			// average of normals is zero. no good.
				if (using_p)
				{
					switch (num_uq)
					{
					case 2:
						normal = 0.5f * (q.normals[uq[0]->n] + q.normals[uq[1]->n]);
						break;
					case 1:
						normal = q.normals[uq[0]->n];
						break;
					}
				}
				else
				{
					switch (num_up)
					{
					case 2:
						normal = 0.5f * (p.normals[up[0]->n] + p.normals[up[1]->n]);
						break;
					case 1:
						normal = p.normals[up[0]->n];
						break;
					}
				}

				nms = dot_product(normal, normal);
				if (nms < 1e-5)
				{
				// STILL NO GOOD. PUNT.
					normal = m->normals[v[0]->n];
				}
				else
				{
					normal /= sqrt(nms);
					f = -f;
				}
			*/
			}
			else
			{
				normal /= sqrt(nms);
			}
		}
		else
		{
		// Use vertex normal.
			normal = m->normals[v[0]->n];
		}

	// Transform to world frame.
		normal = *R * normal;

	// Negate if necessary.
		normal *= f;
#endif
		//sprintf(temp, "Forming normal from %d vertices.\n", n);
		//DebugPrint(temp);
	}


	//sprintf(temp, "\nSimplex dimension: %d\n", s.n);
	//DebugPrint(temp);
#if 0
	char temp2[40];
	strcpy(temp, "p indices: ");
	for (int i = 0; i < s.n; i++)
	{
		sprintf(temp2, "%d ", s.p[i] - p.vertices);
		strcat(temp, temp2);
	}
	PHYTRACE10(temp);

	strcpy(temp, "\nq indices: ");
	for (i = 0; i < s.n; i++)
	{
		sprintf(temp2, "%d ", s.q[i] - q.vertices);
		strcat(temp, temp2);
	}
	PHYTRACE10(temp);

	PHYTRACE10("\n");
#endif

	return (!penetrating);
#endif // CAMERON
}

//

