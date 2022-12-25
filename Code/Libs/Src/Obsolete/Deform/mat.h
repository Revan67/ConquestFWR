#ifndef MAT_H
#define MAT_H

//

#include "3dmath.h"

//

template <unsigned int DIM> class Vec
{
protected:
public:

	float			d[DIM];
	unsigned int	len;

	Vec(unsigned int _len) : len(_len)
	{
	}

	Vec(const Vector & _v)
	{
		len = 3;
		d[0] = _v.x;
		d[1] = _v.y;
		d[2] = _v.z;
	}

	Vec(const Vec<DIM> & _v)
	{
		len = _v.len;
		memcpy(d, _v.d, sizeof(float) * len);
	}

	void zero(void);

	inline const float operator [] (unsigned int idx) const
	{
		return d[idx];
	}

	float magnitude(void) const
	{
		float sqsum = 0;
		for (int i = 0; i < len; i++)
		{
			sqsum += d[i] * d[i];
		}
		return sqrt(sqsum);
	}

	Vec<DIM> & operator *= (float s);
	Vec<DIM> & operator /= (float s);

	Vec<DIM> & operator += (const Vec<DIM> & v);
	Vec<DIM> & operator -= (const Vec<DIM> & v);

	Vec<DIM> add(const Vec<DIM> & v) const;
	//static Vec<DIM> operator + (const Vec<DIM> & v1, const Vec<DIM> & v2);
	//static Vec<DIM> operator - (const Vec<DIM> & v1, const Vec<DIM> & v2);
};



//
// DIM specifies max dimensions. 
//
template <unsigned int DIM> class Mat
{
protected:
public:

	float	d[DIM][DIM];

// m, n specify actual dimensions of this instance.
	unsigned int		m, n;

	float pythag(float a, float b) const;
	void svdcmp(Mat<DIM> & U, Vec<DIM> & W, Mat<DIM> & V) const;
	int gaussj(Mat<DIM> & A, Mat<DIM> & B) const;

	Mat<DIM>(unsigned int _m, unsigned int _n) : m(_m), n(_n)
	{
	}

	Mat<DIM>(const Matrix & _m)
	{
		m = n = 3;
		for (int i = 0; i < 3; i++)
		{
			for (int j = 0; j < 3; j++)
			{
				d[i][j] = _m.d[i][j];
			}
		}
	}

	void set_identity(void);

	Mat<DIM> get_transpose(void) const;

//
// get_inverse() will call get_pseudo_inverse() in the following cases:
// 1. The matrix is non-square.
// 2. Gauss-Jordan elimination determines that the matrix is singular.
//
	Mat get_inverse(void) const;

//
// Use get_pseudo_inverse() for non-square matrices or matrices expected to
// be singular or nearly so (e.g., the Jacobian of a highly-redundant serial chain).
//
// Uses singular value decomposition to get numerically stable pseudo-inverse.
// Not necessarily cheap.
//
	Mat<DIM> get_pseudo_inverse(int & rank) const;

	const Mat<DIM> & operator *= (float s);

// multiply by diagonal matrix whose elements are contained in v.
	static Mat<DIM> mul_by_diag(const Mat<DIM> & m, const Vec<DIM> & v);

// result = m1 * m2T
	static Mat<DIM> mul_by_transpose(const Mat<DIM> & m1, const Mat<DIM> & m2);

	static Mat<DIM> operator + (const Mat<DIM> & m1, const Mat<DIM> & m2);
	static Mat<DIM> operator - (const Mat<DIM> & m1, const Mat<DIM> & m2);
	static Mat<DIM> operator * (const Mat<DIM> & m1, const Mat<DIM> & m2);

	static Mat<DIM> mul(const Mat<DIM> & m1, const Mat<DIM> & m2);

	//static Vec<DIM> operator * (const Mat<DIM> & m, const Vec<DIM> & v);
	Vec<DIM> mul(const Vec<DIM> & v) const;
};

//

//

template <unsigned int DIM> void Vec<DIM>::zero(void)
{
	memset(d, 0, sizeof(float) * len);
}

//

template <unsigned int DIM> Vec<DIM> & Vec<DIM>::operator *= (float s)
{
	for (int i = 0; i < len; i++)
	{
		d[i] *= s;
	}
	return *this;
}

template <unsigned int DIM> Vec<DIM> & Vec<DIM>::operator /= (float s)
{
	float s_inv = 1.0 / s;
	for (int i = 0; i < len; i++)
	{
		d[i] *= s_inv;
	}
	return *this;
}

template <unsigned int DIM> Vec<DIM> & Vec<DIM>::operator += (const Vec<DIM> & v)
{
	ASSERT(len == v.len);
	for (U32 i = 0; i < len; i++)
	{
		d[i] += v.d[i];
	}
	return *this;
}

//

template <unsigned int DIM> Vec<DIM> & Vec<DIM>::operator -= (const Vec<DIM> & v)
{
	ASSERT(len == v.len);
	for (int i = 0; i < len; i++)
	{
		d[i] -= v.d[i];
	}
	return *this;
}

//

template <unsigned int DIM> Vec<DIM> Vec<DIM>::add(const Vec<DIM> & v) const
{
	ASSERT(len == v.len);

	Vec<DIM> result(len);
	for (int i = 0; i < len; i++)
	{
		result.d[i] = d[i] + v.d[i];
	}
	return result;
}

//

template <unsigned int DIM> Vec<DIM> operator - (const Vec<DIM> & v1, const Vec<DIM> & v2)
{
	ASSERT(v1.len == v2.len);
	Vec<DIM> result;
	for (int i = 0; i < len; i++)
	{
		result.d[i] = v1.d[i] - v2.d[i];
	}
	return result;
}

//

template <unsigned int DIM> void Mat<DIM>::set_identity(void)
{
	for (int i = 0; i < m; i++)
	{
		for (int j = 0; j < n; j++)
		{
			d[i][j] = (i == j) ? 1.0 : 0.0;
		}
	}
}

//

//#pragma warning(disable : 4018 4244 4305)
#pragma warning( disable : 4018 )

//

template <unsigned int DIM> Mat<DIM> Mat<DIM>::get_transpose(void) const
{
	Mat<DIM> result(n, m);
	for (unsigned int i = 0; i < m; i++)
	{
		for (unsigned int j = 0; j < n; j++)
		{
			result.d[j][i] = d[i][j];
		}
	}
	return result;
}

//

#define SWAP(a,b) { float temp=(a);(a)=(b);(b)=temp; }

//

template <unsigned int DIM> int Mat<DIM>::gaussj(Mat<DIM> & A, Mat<DIM> & B) const
{
	int *indxc,*indxr,*ipiv;
	int i,icol,irow,j,k,l,ll;
	float big,dum,pivinv;

	int temp1[DIM];
	int temp2[DIM];
	int temp3[DIM];

	indxc = temp1; 
	indxr = temp2; 
	ipiv = temp3; 

  for (j=0;j<A.n;j++) ipiv[j]=0;
  for (i=0;i<A.n;i++) {
    big=(float)0.0;
    for (j=0;j<A.n;j++)
      if (ipiv[j] != 1)
        for (k=0;k<A.n;k++) {
          if (ipiv[k] == 0) {
            if (fabs(A.d[j][k]) >= big) {
              big=fabs(A.d[j][k]);
              irow=j;
              icol=k;
            }
          } 
		  else if (ipiv[k] > 1) 
		  {
			  return 0;
		  }
        }
    ++(ipiv[icol]);
    if (irow != icol) {
      for (l=0;l<n;l++) SWAP(A.d[irow][l],A.d[icol][l]);
      for (l=0;l<m;l++) SWAP(B.d[irow][l],B.d[icol][l]);
    }
    indxr[i]=irow;
    indxc[i]=icol;
    if (A.d[icol][icol] == 0.0)
	{
		return 0;
	}
    pivinv=(float)1.0/A.d[icol][icol];
    A.d[icol][icol]=(float)1.0;
    for (l=0;l<A.n;l++) A.d[icol][l] *= pivinv;
    for (l=0;l<A.m;l++) B.d[icol][l] *= pivinv;
    for (ll=0;ll<A.n;ll++)
      if (ll != icol) {
        dum=A.d[ll][icol];
        A.d[ll][icol]=(float)0.0;
        for (l=0;l<A.n;l++) A.d[ll][l] -= A.d[icol][l]*dum;
        for (l=0;l<A.m;l++) B.d[ll][l] -= B.d[icol][l]*dum;
      }
  }
  for (l=n-1;l>=0;l--) {
    if (indxr[l] != indxc[l])
      for (k=0;k<A.n;k++)
        SWAP(A.d[k][indxr[l]],A.d[k][indxc[l]]);
  }

  return 1;
}

//

template <unsigned int DIM> Mat<DIM> Mat<DIM>::get_inverse(void) const
{
	Mat<DIM> result(n, m);

	if (m == n)
	{
		Mat<DIM> result = *this;
		Mat<DIM> id(m, n);
		id.set_identity();

	// Use Gauss-Jordan elimination from Numerical Recipes.
		if (!gaussj(result, id))
		{
			int rank;
			result = get_pseudo_inverse(rank);
		}
		else
		{
		// TEST:
			Mat<DIM> test = mul(result, *this);
		}

	}
	else
	{
		int rank;
		result = get_pseudo_inverse(rank);
	}

	return result;
}

//
// BULLSHIT NEEDED FOR SVD from Numerical Recipes.
//

//

static float sqrarg;
#define SQR(a) ((sqrarg=(a)) == 0.0 ? 0.0 : sqrarg*sqrarg)

#define SIGN(a,b) ((b) >= 0.0 ? fabs(a) : -fabs(a))

static float maxarg1,maxarg2;
#define FMAX(a,b) (maxarg1=(a),maxarg2=(b),(maxarg1) > (maxarg2) ?\
        (maxarg1) : (maxarg2))

static float minarg1,minarg2;
#define FMIN(a,b) (minarg1=(a),minarg2=(b),(minarg1) < (minarg2) ?\
        (minarg1) : (minarg2))

static int iminarg1,iminarg2;
#define IMIN(a,b) (iminarg1=(a),iminarg2=(b),(iminarg1) < (iminarg2) ?\
        (iminarg1) : (iminarg2))


//

template <unsigned int DIM> float Mat<DIM>::pythag(float a, float b) const
{
	float absa = fabs(a);
	float absb = fabs(b);
	if (absa > absb) return absa*sqrt(1.0+SQR(absb/absa));
	else return (absb == 0.0 ? 0.0 : absb*sqrt(1.0+SQR(absa/absb)));
}

//
#if 0
template <unsigned int DIM> void Mat<DIM>::svdcmp(Mat<DIM> & U, Vec<DIM> & W, Mat<DIM> & V) const
{
	int flag,its,j,jj,k,l,nm;
	float anorm,c,f,g,h,s,scale,x,y,z;

// ELIMINATE THIS RIDICULOUS COPY. The goddamn authors of Numerical Recipes were
// too lazy to convert properly their Fortran code to C, so they left everything
// using 1..N arrays rather than 0..(N-1). Some routines are fairly easy to convert,
// but this one is very ugly and complicated. So we copy to N+1-arrays before running 
// the routine, then back to N-arrays at the end. Ridiculous.

	float a[DIM+1][DIM+1];
	for (U32 i = 0; i < m; i++)
	{
		for (j = 0; j < n; j++)
		{
			a[i+1][j+1] = U.d[i][j];
		}
	}

	float w[DIM+1];
	float v[DIM+1][DIM+1];

	float rv1[DIM+1];

	g=scale=anorm=0.0;
	for (i=1;i<=n;i++) 
	{
		l=i+1;
		rv1[i]=scale*g;
		g=s=scale=0.0;
		if (i <= m) 
		{
			for (k=i;k<=m;k++) scale += fabs(a[k][i]);
			if (scale) 
			{
				for (k=i;k<=m;k++) 
				{
					a[k][i] /= scale;
					s += a[k][i]*a[k][i];
				}
				f=a[i][i];
				g = -SIGN(sqrt(s),f);
				h=f*g-s;
				a[i][i]=f-g;
				for (j=l;j<=n;j++) 
				{
					for (s=0.0,k=i;k<=m;k++) s += a[k][i]*a[k][j];
					f=s/h;
					for (k=i;k<=m;k++) a[k][j] += f*a[k][i];
				}
				for (k=i;k<=m;k++) a[k][i] *= scale;
			}
		}
		w[i]=scale *g;
		g=s=scale=0.0;
		if (i <= m && i != n) 
		{
			for (k=l;k<=n;k++) scale += fabs(a[i][k]);
			if (scale) 
			{
				for (k=l;k<=n;k++) 
				{
					a[i][k] /= scale;
					s += a[i][k]*a[i][k];
				}
				f=a[i][l];
				g = -SIGN(sqrt(s),f);
				h=f*g-s;
				a[i][l]=f-g;
				for (k=l;k<=n;k++) rv1[k]=a[i][k]/h;
				for (j=l;j<=m;j++) 
				{
					for (s=0.0,k=l;k<=n;k++) s += a[j][k]*a[i][k];
					for (k=l;k<=n;k++) a[j][k] += s*rv1[k];
				}
				for (k=l;k<=n;k++) a[i][k] *= scale;
			}
		}
		anorm=FMAX(anorm,(fabs(w[i])+fabs(rv1[i])));
	}
	for (i=n;i>=1;i--) 
	{
		if (i < n) 
		{
			if (g) 
			{
				for (j=l;j<=n;j++)
					v[j][i]=(a[i][j]/a[i][l])/g;
				for (j=l;j<=n;j++) 
				{
					for (s=0.0,k=l;k<=n;k++) s += a[i][k]*v[k][j];
					for (k=l;k<=n;k++) v[k][j] += s*v[k][i];
				}
			}
			for (j=l;j<=n;j++) v[i][j]=v[j][i]=0.0;
		}
		v[i][i]=1.0;
		g=rv1[i];
		l=i;
	}
	for (i=IMIN(m,n);i>=1;i--) 
	{
		l=i+1;
		g=w[i];
		for (j=l;j<=n;j++) a[i][j]=0.0;
		if (g) 
		{
			g=1.0/g;
			for (j=l;j<=n;j++) 
			{
				for (s=0.0,k=l;k<=m;k++) s += a[k][i]*a[k][j];
				f=(s/a[i][i])*g;
				for (k=i;k<=m;k++) a[k][j] += f*a[k][i];
			}
			for (j=i;j<=m;j++) a[j][i] *= g;
		} else for (j=i;j<=m;j++) a[j][i]=0.0;
		++a[i][i];
	}
	for (k=n;k>=1;k--) 
	{
		for (its=1;its<=30;its++) 
		{
			flag=1;
			for (l=k;l>=1;l--) 
			{
				nm=l-1;
				if ((float)(fabs(rv1[l])+anorm) == anorm) 
				{
					flag=0;
					break;
				}
				if ((float)(fabs(w[nm])+anorm) == anorm) break;
			}
			if (flag) 
			{
				c=0.0;
				s=1.0;
				for (i=l;i<=k;i++) 
				{
					f=s*rv1[i];
					rv1[i]=c*rv1[i];
					if ((float)(fabs(f)+anorm) == anorm) break;
					g=w[i];
					h=pythag(f,g);
					w[i]=h;
					h=1.0/h;
					c=g*h;
					s = -f*h;
					for (j=1;j<=m;j++) 
					{
						y=a[j][nm];
						z=a[j][i];
						a[j][nm]=y*c+z*s;
						a[j][i]=z*c-y*s;
					}
				}
			}
			z=w[k];
			if (l == k) 
			{
				if (z < 0.0) 
				{
					w[k] = -z;
					for (j=1;j<=n;j++) v[j][k] = -v[j][k];
				}
				break;
			}
			if (its == 30) break;//nrerror("no convergence in 30 svdcmp iterations");
			x=w[l];
			nm=k-1;
			y=w[nm];
			g=rv1[nm];
			h=rv1[k];
			f=((y-z)*(y+z)+(g-h)*(g+h))/(2.0*h*y);
			g=pythag(f,1.0);
			f=((x-z)*(x+z)+h*((y/(f+SIGN(g,f)))-h))/x;
			c=s=1.0;
			for (j=l;j<=nm;j++) 
			{
				i=j+1;
				g=rv1[i];
				y=w[i];
				h=s*g;
				g=c*g;
				z=pythag(f,h);
				rv1[j]=z;
				c=f/z;
				s=h/z;
				f=x*c+g*s;
				g = g*c-x*s;
				h=y*s;
				y *= c;
				for (jj=1;jj<=n;jj++) 
				{
					x=v[jj][j];
					z=v[jj][i];
					v[jj][j]=x*c+z*s;
					v[jj][i]=z*c-x*s;
				}
				z=pythag(f,h);
				w[j]=z;
				if (z) 
				{
					z=1.0/z;
					c=f*z;
					s=h*z;
				}
				f=c*g+s*y;
				x=c*y-s*g;
				for (jj=1;jj<=m;jj++) 
				{
					y=a[jj][j];
					z=a[jj][i];
					a[jj][j]=y*c+z*s;
					a[jj][i]=z*c-y*s;
				}
			}
			rv1[l]=0.0;
			rv1[k]=f;
			w[k]=x;
		}
	}

	for (i = 0; i < m; i++)
	{
		for (j = 0; j < n; j++)
		{
			U.d[i][j] = a[i+1][j+1];
		}
	}

	for (i = 0; i < n; i++)
	{
		W.d[i] = w[i+1];
		for (int j = 0; j < n; j++)
		{
			V.d[i][j] = v[i+1][j+1];
		}
	}
}
#else
//
// EXPERIMENTAL CONVERSION TO 0..N-1 arrays.
//

template <unsigned int DIM> void Mat<DIM>::svdcmp(Mat<DIM> & U, Vec<DIM> & W, Mat<DIM> & V) const
{
	int i, flag,its,j,jj,k,l,nm;
	float anorm,c,f,g,h,s,scale,x,y,z;

	float rv1[DIM];

	g=scale=anorm=0.0;
	for (i=0;i<n;i++) 
	{
		l=i+1;
		rv1[i]=scale*g;
		g=s=scale=0.0;
		if (i < m) 
		{
			for (k=i;k<m;k++) scale += fabs(U.d[k][i]);
			if (scale) 
			{
				for (k=i;k<m;k++) 
				{
					U.d[k][i] /= scale;
					s += U.d[k][i]*U.d[k][i];
				}
				f=U.d[i][i];
				g = -SIGN(sqrt(s),f);
				h=f*g-s;
				U.d[i][i]=f-g;
				for (j=l;j<n;j++) 
				{
					for (s=0.0,k=i;k<m;k++) s += U.d[k][i]*U.d[k][j];
					f=s/h;
					for (k=i;k<m;k++) U.d[k][j] += f*U.d[k][i];
				}
				for (k=i;k<m;k++) U.d[k][i] *= scale;
			}
		}
		W.d[i]=scale *g;
		g=s=scale=0.0;
		if (i < m && i != n) 
		{
			for (k=l;k<n;k++) scale += fabs(U.d[i][k]);
			if (scale) 
			{
				for (k=l;k<n;k++) 
				{
					U.d[i][k] /= scale;
					s += U.d[i][k]*U.d[i][k];
				}
				f=U.d[i][l];
				g = -SIGN(sqrt(s),f);
				h=f*g-s;
				U.d[i][l]=f-g;
				for (k=l;k<n;k++) rv1[k]=U.d[i][k]/h;
				for (j=l;j<m;j++) 
				{
					for (s=0.0,k=l;k<n;k++) s += U.d[j][k]*U.d[i][k];
					for (k=l;k<n;k++) U.d[j][k] += s*rv1[k];
				}
				for (k=l;k<n;k++) U.d[i][k] *= scale;
			}
		}
		anorm=FMAX(anorm,(fabs(W.d[i])+fabs(rv1[i])));
	}
	for (i=n-1;i>=0;i--) 
	{
		if (i < n) 
		{
			if (g) 
			{
				for (j=l;j<n;j++)
					V.d[j][i]=(U.d[i][j]/U.d[i][l])/g;
				for (j=l;j<n;j++) 
				{
					for (s=0.0,k=l;k<n;k++) s += U.d[i][k]*V.d[k][j];
					for (k=l;k<n;k++) V.d[k][j] += s*V.d[k][i];
				}
			}
			for (j=l;j<n;j++) V.d[i][j]=V.d[j][i]=0.0;
		}
		V.d[i][i]=1.0;
		g=rv1[i];
		l=i;
	}
	for (i=IMIN(m,n)-1;i>=0;i--) 
	{
		l=i+1;
		g=W.d[i];
		for (j=l;j<n;j++) U.d[i][j]=0.0;
		if (g) 
		{
			g=1.0/g;
			for (j=l;j<n;j++) 
			{
				for (s=0.0,k=l;k<m;k++) s += U.d[k][i]*U.d[k][j];
				f=(s/U.d[i][i])*g;
				for (k=i;k<m;k++) U.d[k][j] += f*U.d[k][i];
			}
			for (j=i;j<m;j++) U.d[j][i] *= g;
		} else for (j=i;j<m;j++) U.d[j][i]=0.0;
		++U.d[i][i];
	}
	for (k=n-1;k>=0;k--) 
	{
		for (its=1;its<=30;its++) 
		{
			flag=1;
			for (l=k;l>=0;l--) 
			{
				nm=l-1;
				if ((float)(fabs(rv1[l])+anorm) == anorm) 
				{
					flag=0;
					break;
				}
				if ((float)(fabs(W.d[nm])+anorm) == anorm) break;
			}
			if (flag) 
			{
				c=0.0;
				s=1.0;
				for (i=l;i<=k;i++) 
				{
					f=s*rv1[i];
					rv1[i]=c*rv1[i];
					if ((float)(fabs(f)+anorm) == anorm) break;
					g=W.d[i];
					h=pythag(f,g);
					W.d[i]=h;
					h=1.0/h;
					c=g*h;
					s = -f*h;
					for (j=0;j<m;j++) 
					{
						y=U.d[j][nm];
						z=U.d[j][i];
						U.d[j][nm]=y*c+z*s;
						U.d[j][i]=z*c-y*s;
					}
				}
			}
			z=W.d[k];
			if (l == k) 
			{
				if (z < 0.0) 
				{
					W.d[k] = -z;
					for (j=0;j<n;j++) V.d[j][k] = -V.d[j][k];
				}
				break;
			}
			if (its == 30) break;//nrerror("no convergence in 30 svdcmp iterations");
			x=W.d[l];
			nm=k-1;
			y=W.d[nm];
			g=rv1[nm];
			h=rv1[k];
			f=((y-z)*(y+z)+(g-h)*(g+h))/(2.0*h*y);
			g=pythag(f,1.0);
			f=((x-z)*(x+z)+h*((y/(f+SIGN(g,f)))-h))/x;
			c=s=1.0;
			for (j=l;j<=nm;j++) 
			{
				i=j+1;
				g=rv1[i];
				y=W.d[i];
				h=s*g;
				g=c*g;
				z=pythag(f,h);
				rv1[j]=z;
				c=f/z;
				s=h/z;
				f=x*c+g*s;
				g = g*c-x*s;
				h=y*s;
				y *= c;
				for (jj=0;jj<n;jj++) 
				{
					x=V.d[jj][j];
					z=V.d[jj][i];
					V.d[jj][j]=x*c+z*s;
					V.d[jj][i]=z*c-x*s;
				}
				z=pythag(f,h);
				W.d[j]=z;
				if (z) 
				{
					z=1.0/z;
					c=f*z;
					s=h*z;
				}
				f=c*g+s*y;
				x=c*y-s*g;
				for (jj=0;jj<m;jj++) 
				{
					y=U.d[jj][j];
					z=U.d[jj][i];
					U.d[jj][j]=y*c+z*s;
					U.d[jj][i]=z*c-y*s;
				}
			}
			rv1[l]=0.0;
			rv1[k]=f;
			W.d[k]=x;
		}
	}
}

#pragma warning( default : 4018 )

#endif


//

template <unsigned int DIM> const Mat<DIM> & Mat<DIM>::operator *= (float s)
{
	for (int i = 0; i < m; i++)
	{
		for (int j = 0; j < n; j++)
		{
			d[i][j] *= s;
		}
	}
	return *this;
}

//

template <unsigned int DIM> Mat<DIM> Mat<DIM>::mul_by_diag(const Mat<DIM> & m, const Vec<DIM> & v)
{
	ASSERT(m.n == v.len);
	Mat<DIM> result(m.m, m.n);
	for (unsigned int i = 0; i < m.m; i++)
	{
		for (unsigned int j = 0; j < m.n; j++)
		{
			result.d[i][j] = m.d[i][j] * v.d[j];
		}
	}
	return result;
}

//

template <unsigned int DIM> Mat<DIM> Mat<DIM>::mul_by_transpose(const Mat<DIM> & m1, const Mat<DIM> & m2)
{
	ASSERT(m1.n == m2.n);
	Mat<DIM> result(m1.m, m2.m);
	for (unsigned int i = 0; i < m1.m; i++)
	{
		for (unsigned int j = 0; j < m2.n; j++)
		{
			result.d[i][j] = 0;
			for (unsigned int k = 0; k < m1.n; k++)
			{
				result.d[i][j] += m1.d[i][k] * m2.d[j][k];
			}
		}
	}
	return result;
}

//

template <unsigned int DIM> Mat<DIM> Mat<DIM>::get_pseudo_inverse(int & rank) const
{
	Mat<DIM> result(n, m);

	Vec<DIM> w(n);
	w.zero();

	Mat<DIM> v(n, n);
	Mat<DIM> u(*this);

// decompose.
	svdcmp(u, w, v);

// Deal with singular values.
	rank = 0;
	for (int i = 0; i < DIM; i++)
	{
		if (w.d[i] < 1e-6)
		{
			w.d[i] = 0;
		}
		else
		{
			w.d[i] = 1.0 / w.d[i];
			rank++;
		}
	}

// Scale columns of V by W values.
	Mat<DIM> vw = mul_by_diag(v, w);

// pseudo-inverse = VWUt.
	result = mul_by_transpose(vw, u);

	return result;
}

//

template <unsigned int DIM> Mat<DIM> operator + (const Mat<DIM> & m1, const Mat<DIM> & m2)
{
}

//

template <unsigned int DIM> Mat<DIM> operator - (const Mat<DIM> & m1, const Mat<DIM> & m2)
{
}

//

template <unsigned int DIM> Mat<DIM> Mat<DIM>::mul(const Mat<DIM> & m1, const Mat<DIM> & m2)
{
	ASSERT(m1.n == m2.m);
	Mat<DIM> result(m1.m, m2.n);
	for (unsigned int i = 0; i < m1.m; i++)
	{
		for (unsigned int j = 0; j < m2.n; j++)
		{
			result.d[i][j] = 0;
			for (unsigned int k = 0; k < m1.n; k++)
			{
				result.d[i][j] += m1.d[i][k] * m2.d[k][j];
			}
		}
	}
	return result;
}

//

template <unsigned int DIM> Mat<DIM> operator * (const Mat<DIM> & m1, const Mat<DIM> & m2)
{
	ASSERT(m1.n == m2.m);
	for (int i = 0; i < m1.m; i++)
	{
		for (int j = 0; j < m2.n; j++)
		{
			result[i][j] = 0;
			for (int k = 0; k < m1.n; k++)
			{
				result[i][j] += m1.d[i][k] * m2.d[k][j];
			}
		}
	}
}

//

template <unsigned int DIM>	Vec<DIM> Mat<DIM>::mul(const Vec<DIM> & v) const
{
	Vec<DIM> result(m);
	ASSERT(n == v.len);
	for (unsigned int i = 0; i < m; i++)
	{
		result.d[i] = 0;
		for (unsigned int j = 0; j < n; j++)
		{
			result.d[i] += d[i][j] * v[j];
		}
	}
	return result;
}

//
/*
template <unsigned int DIM> Vec<DIM> Mat<DIM>::operator * (const Mat<DIM> & m, const Vec<DIM> & v)
{
	Vec<DIM> result(m.m);
	ASSERT(m.n == vec.len);
	for (int i = 0; i < m.m; i++)
	{
		result[i] = 0;
		for (int j = 0; j < m.n; j++)
		{
			result[i] += m.d[i][j] * v[j];
		}
	}
}
*/
//


//

#endif