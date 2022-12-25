//
// <minsphere.cpp>
//

#include <float.h>
#include "mingeom.h"
#include "mesh.h"

struct TSphere
{
    SINGLE x, y, z, r;
};


// --------------------------------------------------------------------------
// SPHERE
// --------------------------------------------------------------------------
//
// initialize random number generator
static int InitializeSeed () { srand(367); return 1; }
static int _initializeSeed = InitializeSeed();

// Should always be 1.  If set to 0, see catastrophic failure of algorithm
// on structured point sets in main driver below.
static int useRandomized = 1;

// for performance measurements (can be removed)
static unsigned int callsMinFix1 = 0;
static unsigned int callsMinFix2 = 0;
static unsigned int callsMinFix3 = 0;
static unsigned int callsTSphere4 = 0;

//---------------------------------------------------------------------------
static int PointInsideTSphere (const Vector& p, const TSphere& s)
{

    double dx = p.x - s.x;
    double dy = p.y - s.y;
    double dz = p.z - s.z;
    return ( dx*dx+dy*dy+dz*dz <= s.r*s.r );
}
//---------------------------------------------------------------------------
static TSphere TSphere1 (const Vector& p)
{
    TSphere minimal = { p.x, p.y, p.z, 0.0 };
    return minimal;
}
//---------------------------------------------------------------------------
static TSphere TSphere2 (const Vector& p0, const Vector& p1)
{
    TSphere minimal;

    minimal.x = 0.5*(p0.x+p1.x);
    minimal.y = 0.5*(p0.y+p1.y);
    minimal.z = 0.5*(p0.z+p1.z);
    double dx = p1.x-p0.x;
    double dy = p1.y-p0.y;
    double dz = p1.z-p0.z;
    minimal.r = 0.5*sqrt(dx*dx+dy*dy+dz*dz);

    return minimal;
}
//---------------------------------------------------------------------------
static TSphere TSphere3 (const Vector& p0, const Vector& p1, const Vector& p2)
{
    // Compute the circle (in 3D) containing p0, p1, and p2.  The center in
        // barycentric coordinates is C = u0*P0+u1*P1+u2*P2 where u0+u1+u2=1,
    // 0 < u0 < 1, 0 < u1 < 1, and 0 < u2 < 1.  The center is equidistant
    // from the three points, so |C-p0| = |C-p1| = |C-p2| = R where R is the
    // radius of the circle.
        //
        // From these conditions,
        //   C-p0 = u0*A + u1*B - A
        //   C-p1 = u0*A + u1*B - B
        //   C-p2 = u0*A + u1*B
        // where A = P0-P2 and B = P1-P2, which leads to
        //   r^2 = |u0*A+u1*B|^2 - 2*Dot(A,u0*A+u1*B) + |A|^2
        //   r^2 = |u0*A+u1*B|^2 - 2*Dot(B,u0*A+u1*B) + |B|^2
        //   r^2 = |u0*A+u1*B|^2
        // Subtracting the last equation from the first two and writing
        // the equations as a linear system,
        //
        // +-                 -++   -+       +-        -+
        // | Dot(A,A) Dot(A,B) || u0 | = 0.5 | Dot(A,A) |
        // | Dot(A,B) Dot(B,B) || u1 |       | Dot(B,B) |
        // +-                 -++   -+       +-        -+
        //
        // The following code solves this system for u0 and u1, then
        // evaluates the third equation in r^2 to obtain r.

    TSphere minimal;

        Vector A = p0 - p2;
        Vector B = p1 - p2;;
        double AA = dot_product(A, A);
        double AB = dot_product(A, B);
        double BB = dot_product(B, B);
    
        double det = AA*BB-AB*AB;

    if ( fabs(det) > 1e-06 )
    {
        double halfInvDet = 0.5f/(AA*BB-AB*AB);
        double u0 = halfInvDet*BB*(AA-AB);
        double u1 = halfInvDet*AA*(BB-AB);
        double u2 = 1.0f-u0-u1;
        Vector tmp(u0*A.x+u1*B.x, u0*A.y+u1*B.y, u0*A.z+u1*B.z);
        minimal.x = u0*p0.x+u1*p1.x+u2*p2.x;
        minimal.y = u0*p0.y+u1*p1.y+u2*p2.y;
        minimal.z = u0*p0.z+u1*p1.z+u2*p2.z;
        minimal.r = sqrt(tmp.x*tmp.x+tmp.y*tmp.y+tmp.z*tmp.z);
    }
    else
    {
        minimal.x = FLT_MAX;
        minimal.y = FLT_MAX;
        minimal.z = FLT_MAX;
        minimal.r = FLT_MAX;
    }

    return minimal;
}
//---------------------------------------------------------------------------
static TSphere TSphere4 (const Vector& p0, const Vector& p1, const Vector& p2,
                       const Vector& p3)
{
    callsTSphere4++;  // performance measure, can be removed

    TSphere minimal;

        Vector e10 = p1 - p0;
        Vector e20 = p2 - p0;
        Vector e30 = p3 - p0;

    double A[3][3];
        A[0][0] = e10.x;  A[0][1] = e10.y;  A[0][2] = e10.z;
        A[1][0] = e20.x;  A[1][1] = e20.y;  A[1][2] = e20.z;
        A[2][0] = e30.x;  A[2][1] = e30.y;  A[2][2] = e30.z;

    double b[3];
        b[0] = 0.5*(e10.x*e10.x+e10.y*e10.y+e10.z*e10.z);
        b[1] = 0.5*(e20.x*e20.x+e20.y*e20.y+e20.z*e20.z);
        b[2] = 0.5*(e30.x*e30.x+e30.y*e30.y+e30.z*e30.z);

    double Ainv[3][3];
        Ainv[0][0] = A[1][1]*A[2][2]-A[1][2]*A[2][1];
        Ainv[0][1] = A[0][2]*A[2][1]-A[0][1]*A[2][2];
        Ainv[0][2] = A[0][1]*A[1][2]-A[0][2]*A[1][1];
        Ainv[1][0] = A[1][2]*A[2][0]-A[1][0]*A[2][2];
        Ainv[1][1] = A[0][0]*A[2][2]-A[0][2]*A[2][0];
        Ainv[1][2] = A[0][2]*A[1][0]-A[0][0]*A[1][2];
        Ainv[2][0] = A[1][0]*A[2][1]-A[1][1]*A[2][0];
        Ainv[2][1] = A[0][1]*A[2][0]-A[0][0]*A[2][1];
        Ainv[2][2] = A[0][0]*A[1][1]-A[0][1]*A[1][0];
        double det = A[0][0]*Ainv[0][0]+A[0][1]*Ainv[1][0]+A[0][2]*Ainv[2][0];
        if ( fabs(det) > 1e-06 )
    {
        double invdet = 1.0/det;
        int row, col;
        for (row = 0; row < 3; row++)
        {
            for (col = 0; col < 3; col++)
                Ainv[row][col] *= invdet;
        }
        
        double x[3];
        for (row = 0; row < 3; row++)
        {
            x[row] = 0.0;
            for (col = 0; col < 3; col++)
                x[row] += Ainv[row][col]*b[col];
        }

        minimal.x = p0.x + x[0];
            minimal.y = p0.y + x[1];
            minimal.z = p0.z + x[2];
            minimal.r = sqrt(x[0]*x[0]+x[1]*x[1]+x[2]*x[2]);
    }
    else
    {
        minimal.x = FLT_MAX;
        minimal.y = FLT_MAX;
        minimal.z = FLT_MAX;
        minimal.r = FLT_MAX;
    }

    return minimal;
}
//---------------------------------------------------------------------------
static TSphere MinFix3 (int n, Vector** perm, Vector* fixed0, Vector* fixed1,
                       Vector* fixed2)
{
    callsMinFix3++;  // performance measure, can be removed

    TSphere minimal = TSphere3(*fixed0,*fixed1,*fixed2);

    for (int i = 0; i < n; i++)
    {
        if ( !PointInsideTSphere(*perm[i],minimal) )
            minimal = TSphere4(*perm[i],*fixed0,*fixed1,*fixed2);
    }

    return minimal;
}
//---------------------------------------------------------------------------
static TSphere MinFix2 (int n, Vector** perm, Vector* fixed0, Vector* fixed1)
{
    callsMinFix2++;  // performance measure, can be removed

    TSphere minimal = TSphere2(*fixed0,*fixed1);

    for (int i = 0; i < n; i++)
    {
        if ( !PointInsideTSphere(*perm[i],minimal) )
            minimal = MinFix3(i,perm,perm[i],fixed0,fixed1);
    }

    return minimal;
}
//---------------------------------------------------------------------------
static TSphere MinFix1 (int n, Vector** perm, Vector* fixed0)
{
    callsMinFix1++;  // performance measure, can be removed

    TSphere minimal = TSphere1(*fixed0);

    for (int i = 0; i < n; i++)
    {
        if ( !PointInsideTSphere(*perm[i],minimal) )
            minimal = MinFix2(i,perm,perm[i],fixed0);
    }

    return minimal;
}

TSphere compute_optimal_TSphere (int n, Vector * p)
{
    // assert: n >= 1
    TSphere minimal;

    if ( n >= 2 )
    {
        // create identity permutation (0,1,...,n-1)
        Vector** perm = new Vector*[n];
        int i;
        for (i = 0; i < n; i++)
            perm[i] = &p[i];

        if ( useRandomized )
        {
            // generate random permutation (code by Jeff Erickson)
            for (i = n-1; i > 0; i--)
            {
                int j = rand() % (i+1);
                if ( j != i )
                {
                    Vector* save = perm[i];
                    perm[i] = perm[j];
                    perm[j] = save;
                }
            }
        }
        
        minimal = TSphere1(*perm[0]);
        
        for (i = 1; i < n; i++)
        {
            if ( !PointInsideTSphere(*perm[i],minimal) )
                minimal = MinFix1(i,perm,perm[i]);
        }
        
        delete[] perm;
    }
    else // n == 1
    {
        minimal = TSphere1(p[0]);
    }

    return minimal;

}

void compute_optimal_sphere(SphereExtent * sphere, S32 vcount, Vector * vlist)
{
    TSphere ts;
    
    ts = compute_optimal_TSphere(vcount, vlist);

    Vector sc(ts.x, ts.y, ts.z);
    
    sphere->xform.set_position(sc);
    sphere->sphere.radius = ts.r;
    

}