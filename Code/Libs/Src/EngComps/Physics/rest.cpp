//
// Baraff-style analytical contact solver.
//

#include <float.h>
#include "rigid.h"
#include "Debugprint.h"
#include "fdump.h"

//

//

#define MAX_CONTACTS	16

//

float compute_aij(const Contact & ci, const Contact & cj)
{
	float result;

	if ((ci.a != cj.a) && (ci.b != cj.b) && (ci.a != cj.b) &&	(ci.b != cj.a))
	{
		result = 0.0f;
	}
	else
	{
		Instance * A = ci.a;
		Instance * B = ci.b;
		Vector Ni = ci.N;
		Vector Nj = cj.N;
		Vector pi = ci.p;
		Vector pj = cj.p;

		Vector a_linear, a_angular;
		if (A)
		{
			Vector force_a(0.0f, 0.0f, 0.0f);
			Vector torque_a(0.0f, 0.0f, 0.0f);

			if (cj.a == ci.a)
			{
				force_a = Nj;
				torque_a = cross_product(pj - A->x, Nj);
			}
			else if (cj.b == ci.a)
			{
				force_a = -Nj;
				torque_a = cross_product(pj - A->x, -Nj);
			}

			Vector ra = pi - A->x;
			Vector a_linear = force_a / A->mass;
			Vector a_angular = cross_product(A->Iinv * torque_a, ra);
		}
		else
		{
			a_linear.zero();
			a_angular.zero();
		}

		Vector b_linear, b_angular;
		if (B)
		{
			Vector force_b(0.0f, 0.0f, 0.0f);
			Vector torque_b(0.0f, 0.0f, 0.0f);

			if (cj.a == ci.b)
			{
				force_b = Nj;
				torque_b = cross_product(pj - B->x, Nj);
			}
			else if (cj.b == ci.b)
			{
				force_b = -Nj;
				torque_b = cross_product(pj - B->x, -Nj);
			}

			Vector rb = pi - B->x;
			b_linear = force_b / B->mass;
			b_angular = cross_product(B->Iinv * torque_b, rb);
		}
		else
		{
			b_linear.zero();
			b_angular.zero();
		}

		Vector sum_a = a_linear + a_angular;
		Vector sum_b = b_linear + b_angular;
		Vector diff = sum_a - sum_b;
		result = dot_product(Ni, diff);

if (result < 0.0f)
{
	PHYTRACE10("aij < 0\n");
}
	}

	return result;
}

//

void compute_A(const Contact * contacts, int n, float A[MAX_CONTACTS][MAX_CONTACTS])
{
	ASSERT(!(n && (!contacts)));	//n is nonzero and contacts ! null
	ASSERT(!(n && (!A)));			//n is nonzero and A is ! null

	for (int i = 0; i < n; i++)
	{
		for (int j = 0; j < n; j++)
		{
			A[i][j] = compute_aij(contacts[i], contacts[j]);
		}
	}
}

//

void compute_b(const Contact * contacts, int n, float b[MAX_CONTACTS])
{
	ASSERT(!(n && (!contacts)));	//n is nonzero and contacts ! null
	ASSERT(!(n && (!b)));			//n is nonzero and b is ! null

	const Contact * c = contacts;
	for (int i = 0; i < n; i++, c++)
	{
		Instance * A = c->a;
		Instance * B = c->b;
		Vector N = c->N;

		Vector a_ext_part, a_vel_part, va;
		if (A)
		{
			Vector ra = c->p - A->x;
			Vector f_ext_a = A->F_applied;
			Vector t_ext_a = A->T_applied;

			a_ext_part = f_ext_a / A->mass + cross_product(A->Iinv * t_ext_a, ra);
			a_vel_part = cross_product(A->w, cross_product(A->w, ra)) + cross_product(A->Iinv * cross_product(A->L, A->w), ra);

			va = A->v + cross_product(A->w, ra);

		}
		else
		{
			a_ext_part.zero();
			a_vel_part.zero();
			va.zero();
		}

		Vector b_ext_part, b_vel_part, vb;
		if (B)
		{
			Vector rb = c->p - B->x;
			Vector f_ext_b = B->F_applied;
			Vector t_ext_b = B->T_applied;

			b_ext_part = f_ext_b / B->mass + cross_product(B->Iinv * t_ext_b, rb);
			b_vel_part = cross_product(B->w, cross_product(B->w, rb)) + cross_product(B->Iinv * cross_product(B->L, B->w), rb);

			vb = B->v + cross_product(B->w, rb);
		}
		else
		{
			b_ext_part.zero();
			b_vel_part.zero();
			vb.zero();
		}

		float k1 = dot_product(N, (a_ext_part + a_vel_part) - (b_ext_part + b_vel_part));
		Vector Ndot = c->compute_Ndot();

		float k2 = 2 * dot_product(Ndot, va - vb);

		b[i] = k1 + k2;
	}
}

//

struct ContactInfo
{
	int		status;

	ContactInfo(void)
	{
		status = 0;
	}			   
	
	bool member_C(void)
	{
		return (status == 1);
	}

	bool member_NC(void)
	{
		return (status == -1);
	}

	void add_to_C(void)
	{
		status = 1;
	}

	void add_to_NC(void)
	{ 
		status = -1;
	}

};

//

ContactInfo info[MAX_CONTACTS];
float A[MAX_CONTACTS][MAX_CONTACTS];
float b[MAX_CONTACTS];
float f[MAX_CONTACTS];
float a[MAX_CONTACTS];

float delta_f[MAX_CONTACTS];
float delta_a[MAX_CONTACTS];

float A11[MAX_CONTACTS][MAX_CONTACTS];
float v1[MAX_CONTACTS][1];

const float tol = 1e-5;

//

int gaussj(float a[MAX_CONTACTS][MAX_CONTACTS], int n, float b[MAX_CONTACTS][1], int m);

//

void fdirection(float delta_f[], int n, int d)
{
	ASSERT(delta_f);

	memset(delta_f, 0, sizeof(float) * n);
	delta_f[d] = 1;

// set A11 equal to the submatrix of A obtained by deleting the j-th row and
// column of A for all j NOT IN C.

	int row = 0, col = 0;
	for (int i = 0; i < n; i++)
	{
	// is i in C?
		if (info[i].member_C())
		{
			for (int j = 0; j < n; j++)
			{
				if (info[j].member_C())
				{
					A11[row][col++] = A[i][j];
				}
			}
			row++;
			col = 0;
		}
	}

// set v1 equal to the d-th column of A with element j deleted for all j NOT IN C.

	row = 0;
	for (i = 0; i < n; i++)
	{
		if (info[i].member_C())
		{
			v1[row++][1] = A[i][d];
		}
	}

// solve A11 * x = -v1
	gaussj(A11, row, v1, 1);

	row = 0;
	for (i = 0; i < n; i++)
	{
		if (i != d)
		{
			delta_f[i] = v1[row++][1];
		}
	}
}

//

void maxstep(float & s, int & j, int n, int d)
{
	s = FLT_MAX;
	j = -1;

	if (delta_a[d] > tol)
	{
		j = d;
		s = -a[d] / delta_a[d];
	}
	else
	{
		PHYTRACE10("ouch\n");
	}

	for (int i = 0; i < n; i++)
	{
		if (info[i].member_C())
		{
			if (delta_f[i] < -tol)
			{
				float sprime = -f[i] / delta_f[i];
				if (sprime < s)
				{
					s = sprime;
					j = i;
				}
			}
		}
	}
	for (i = 0; i < n; i++)
	{
		if (info[i].member_NC())
		{
			if (delta_a[i] < -tol)
			{
				float sprime = -a[i] / delta_a[i];
				if (sprime < s)
				{
					s = sprime;
					j = i;
				}
			}
		}
	}
}

//

void drive_to_zero(int n, int d)
{
__L1:

	fdirection(delta_f, n, d);

	for (int i = 0; i < n; i++)
	{
		delta_a[i] = 0;

		for (int j = 0; j < n; j++)
		{
			delta_a[i] += A[i][j] * delta_f[j];
		}
	}

	float s;
	int j;
	maxstep(s, j, n, d);

	for (i = 0; i < n; i++)
	{
		f[i] += s * delta_f[i];
		a[i] += s * delta_a[i];
	}

	if (info[j].member_C())
	{
		info[j].add_to_NC();
		goto __L1;
	}
	else if (info[j].member_NC())
	{
		info[j].add_to_C();
		goto __L1;
	}
	else
	{
		info[j].add_to_C();
	}
}

//

extern bool HackFlag;
void RigidBodyPhysics::compute_contact_forces(const Contact * contacts, int n)
{
	ASSERT(!(n && (!contacts)));	//n is nonzero and contacts ! null

// TEMPORARY
	compute_internal_forces();

	compute_A(contacts, n, A);
	compute_b(contacts, n, b);

// solve.
	for (int i = 0; i < n; i++)
	{
		info[i].status = 0;
	}

	memset(f, 0, sizeof(float) * n);
	memcpy(a, b, sizeof(float) * n);

	bool done = false;
	while (!done)
	{
		for (i = 0; i < n; i++)
		{
			if (a[i] < -tol)
			{
				drive_to_zero(n, i);
				break;	// start loop over?
			}
			else if (a[i] < tol)
			{
				a[i] = 0;	// clamp to 0 if between -tol and tol.
			}
			else
			{
				PHYTRACE10("a > 0\n");
			}
		}

	// If we made it all the way through with no a's negative, we're done.
		if (i == n)
		{
			done = true;
		}
	}

//
	const Contact * c = contacts;
	for (i = 0; i < n; i++, c++)
	{
		float fi = f[i];
		Vector N = c->N;
		Instance * A = c->a;
		Instance * B = c->b;

		if (A)
		{
			Vector fN = fi * N;
			A->F_internal += fN;
			A->T_internal += cross_product(c->p - A->x, fN);
		}
		if (B)
		{
			Vector fN = fi * N;
			B->F_internal -= fN;
			B->T_internal -= cross_product(c->p - B->x, fN);
		}
	}
}

//

void nrerror(const char *) 
{
}

#define SWAP(a,b) { float temp=(a);(a)=(b);(b)=temp; }

// Linear equation solution by Gauss-Jordan elimination, equation (2.1.1) above. a[1..n][1..n]
// is the input matrix. b[1..n][1..m] is input containing the m right-hand side vectors. On
// output, a is replaced by its matrix inverse, and b is replaced by the corresponding set of 
// solution vectors. 

static int temp1[MAX_CONTACTS];
static int temp2[MAX_CONTACTS];
static int temp3[MAX_CONTACTS];

//

int gaussj(float a[MAX_CONTACTS][MAX_CONTACTS], int n, float b[MAX_CONTACTS][1], int m)
{
	ASSERT(!(n && (!b)));	//n is nonzero and b ! null
	ASSERT(!(n && (!a)));	//n is nonzero and a ! null

  int *indxc,*indxr,*ipiv;
  int i,icol,irow,j,k,l,ll;
  float big,dum,pivinv;

	indxc = temp1; 
	indxr = temp2; 
	ipiv = temp3; 



  for (j=0;j<n;j++) ipiv[j]=0;
  for (i=0;i<n;i++) {
    big=(float)0.0f;
    for (j=0;j<n;j++)
      if (ipiv[j] != 1)
        for (k=0;k<n;k++) {
          if (ipiv[k] == 0) {
            if (fabs(a[j][k]) >= big) {
              big=fabs(a[j][k]);
              irow=j;
              icol=k;
            }
          } 
		  else if (ipiv[k] > 1) 
		  {
			  nrerror("GAUSSJ: Singular Matrix-1");
		  }
        }
    ++(ipiv[icol]);
    if (irow != icol) {
      for (l=0;l<n;l++) SWAP(a[irow][l],a[icol][l]);
      for (l=0;l<m;l++) SWAP(b[irow][l],b[icol][l]);
    }
    indxr[i]=irow;
    indxc[i]=icol;
    if (a[icol][icol] == 0.0f)
	{
		nrerror("GAUSSJ: Singular Matrix-2");
	}
    pivinv=(float)1.0f/a[icol][icol];
    a[icol][icol]=(float)1.0f;
    for (l=0;l<n;l++) a[icol][l] *= pivinv;
    for (l=0;l<m;l++) b[icol][l] *= pivinv;
    for (ll=0;ll<n;ll++)
      if (ll != icol) {
        dum=a[ll][icol];
        a[ll][icol]=(float)0.0f;
        for (l=0;l<n;l++) a[ll][l] -= a[icol][l]*dum;
        for (l=0;l<m;l++) b[ll][l] -= b[icol][l]*dum;
      }
  }
  for (l=n-1;l>=0;l--) {
    if (indxr[l] != indxc[l])
      for (k=0;k<n;k++)
        SWAP(a[k][indxr[l]],a[k][indxc[l]]);
  }

  return 1;
}

//