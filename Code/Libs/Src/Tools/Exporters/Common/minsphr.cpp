//---------------------------------------------------------------------------
// Algorithm to compute the minimal volume bounding sphere of a set of
// points in 3D.  Usage is
//
//    Sphere minimal = MinimalSphere(int n, Point2* p);
//
// Input:   number of points n and array of points p[0] through p[n-1].
// Output:  minimal volume bounding sphere, minimal
//---------------------------------------------------------------------------
//
// This is an implementation of Welzl's algorithm based on the
// pseudocode by Hiroki Honda (honda@konami.co.jp).
//
//  Support subroutines:
//    Sphere1(p1) : compute the smallest sphere with p1 on the boundary
//                  (i.e. center = p1, radius = 0)
//    Sphere2(p1,p2) : compute the smallest sphere with p1,p2 on the boundary
//                     (i.e. center = (p1+p2)/2, radius = |p2-p1|/2)
//    Sphere3(p1,p2,p3) : compute the smallest sphere
//                        with p1,p2,p3 on the boundary
//                        (i.e. PlanarCircumScribe3 in minsphr.cpp)
//    Sphere4(p1,p2,p3,p4) : compute the sphere
//                           with p1,p2,p3,p4 on the boundary
//
//  // find the smallest sphere containing points P[1]...P[n]
//  SmallestSphere(P[1..n]):
//      shuffle P into random order         // IMPORTANT!!!
//      B = Empty                           // initial guess
//      for i = 1 to n
//          if Outside(P[i], B)
//              B = SmallestSphereOne(P[1..i-1], P[i])
//      return B;
//
//  // find the smallest sphere containing the points P[1]..P[n]
//  // with the point p on the boundary
//  // *** works only when called by SmallestSphere ***
//  SmallestSphereOne(P[1..n], p):
//      B = Sphere1(p)                      // initial guess
//      for i = 1 to n
//          if Outside(P[i], B)
//              B = SmallestSphereTwo(P[1..i-1], P[i], p)
//      return B;
//
//  // find the smallest sphere containing the points P[1]..P[n]
//  // with the points p and q on the boundary
//  // *** works only when called by SmallestSphereOne ***
//  SmallestSphereTwo(P[1..n], p, q):
//      B = Sphere2(p, q)                   // initial guess
//      for i = 1 to n
//          if Outside(P[i], B)
//              B = SmallestSphereThree(P[1..i-1], P[i], p, q)
//      return B;
//
//  // find the smallest sphere containing the points P[1]..P[n]
//  // with the points p,q,r on the boundary
//  // *** works only when called by SmallestSphereTwo ***
//  SmallestSphereThree(P[1..n], p, q, r):
//      B = Sphere3(p, q, r)                // initial guess
//      for i = 1 to n
//          if Outside(P[i], B)
//              B = Sphere4(P[i], p, q, r)
//      return B;


#include <float.h>
#include <math.h>
#include <stdlib.h>
#include "minsphr.h"

#ifndef SGI
#pragma warning( disable : 4244 )
#endif

//---------------------------------------------------------------------------
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
static unsigned int callsSphere4 = 0;
//---------------------------------------------------------------------------
static int PointInsideSphere (const Point3& p, const Sphere& s)
{
    double dx = p.x - s.x;
    double dy = p.y - s.y;
    double dz = p.z - s.z;
    return ( dx*dx+dy*dy+dz*dz <= s.r*s.r );
}
//---------------------------------------------------------------------------
static Sphere Sphere1 (const Point3& p)
{
    Sphere minimal = { p.x, p.y, p.z, 0.0 };
    return minimal;
}
//---------------------------------------------------------------------------
static Sphere Sphere2 (const Point3& p0, const Point3& p1)
{
    Sphere minimal;

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
static Sphere Sphere3 (const Point3& p0, const Point3& p1, const Point3& p2)
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

    Sphere minimal;

	Point3 A = { p0.x-p2.x, p0.y-p2.y, p0.z-p2.z };
	Point3 B = { p1.x-p2.x, p1.y-p2.y, p1.z-p2.z };
	double AA = A.x*A.x+A.y*A.y+A.z*A.z;
	double AB = A.x*B.x+A.y*B.y+A.z*B.z;
	double BB = B.x*B.x+B.y*B.y+B.z*B.z;
    double det = AA*BB-AB*AB;

    if ( fabs(det) > 1e-06 )
    {
        double halfInvDet = 0.5f/(AA*BB-AB*AB);
        double u0 = halfInvDet*BB*(AA-AB);
        double u1 = halfInvDet*AA*(BB-AB);
        double u2 = 1.0f-u0-u1;
        Point3 tmp = { u0*A.x+u1*B.x, u0*A.y+u1*B.y, u0*A.z+u1*B.z };
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
static Sphere Sphere4 (const Point3& p0, const Point3& p1, const Point3& p2,
                       const Point3& p3)
{
    callsSphere4++;  // performance measure, can be removed

    Sphere minimal;

	Point3 e10 = { p1.x-p0.x, p1.y-p0.y, p1.z-p0.z };
	Point3 e20 = { p2.x-p0.x, p2.y-p0.y, p2.z-p0.z };
	Point3 e30 = { p3.x-p0.x, p3.y-p0.y, p3.z-p0.z };

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
static Sphere MinFix3 (int n, Point3** perm, Point3* fixed0, Point3* fixed1,
                       Point3* fixed2)
{
    callsMinFix3++;  // performance measure, can be removed

    Sphere minimal = Sphere3(*fixed0,*fixed1,*fixed2);

    for (int i = 0; i < n; i++)
    {
        if ( !PointInsideSphere(*perm[i],minimal) )
            minimal = Sphere4(*perm[i],*fixed0,*fixed1,*fixed2);
    }

    return minimal;
}
//---------------------------------------------------------------------------
static Sphere MinFix2 (int n, Point3** perm, Point3* fixed0, Point3* fixed1)
{
    callsMinFix2++;  // performance measure, can be removed

    Sphere minimal = Sphere2(*fixed0,*fixed1);

    for (int i = 0; i < n; i++)
    {
        if ( !PointInsideSphere(*perm[i],minimal) )
            minimal = MinFix3(i,perm,perm[i],fixed0,fixed1);
    }

    return minimal;
}
//---------------------------------------------------------------------------
static Sphere MinFix1 (int n, Point3** perm, Point3* fixed0)
{
    callsMinFix1++;  // performance measure, can be removed

    Sphere minimal = Sphere1(*fixed0);

    for (int i = 0; i < n; i++)
    {
        if ( !PointInsideSphere(*perm[i],minimal) )
            minimal = MinFix2(i,perm,perm[i],fixed0);
    }

    return minimal;
}
//---------------------------------------------------------------------------
Sphere MinimalSphere (int n, Point3* p)
{
    // assert: n >= 1
    Sphere minimal;

    if ( n >= 2 )
    {
        // create identity permutation (0,1,...,n-1)
        Point3** perm = new Point3*[n];
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
                    Point3* save = perm[i];
                    perm[i] = perm[j];
                    perm[j] = save;
                }
            }
        }
        
        minimal = Sphere1(*perm[0]);
        
        for (i = 1; i < n; i++)
        {
            if ( !PointInsideSphere(*perm[i],minimal) )
                minimal = MinFix1(i,perm,perm[i]);
        }
        
        delete[] perm;
    }
    else // n == 1
    {
        minimal = Sphere1(p[0]);
    }

    return minimal;
}
//---------------------------------------------------------------------------

#ifdef MINSPHERE_TEST

#include <iostream.h>

void main ()
{
    const int n = 16384;
    Point3* p = new Point3[n];
    int i;
    Sphere minimal;

    // Example 1.  Randomly generated points inside cube [-1,1]^3.
    // As n is increased, the minimal sphere is asymptotic to sphere
    // with center (0,0,0) and radius = sqrt(3).  In this example, the
    // randomization is slightly irrelevant because the points are already
    // (in a sense) in random order.
    for (i = 0; i < n; i++)
    {
        p[i].x = -1.0+2.0*rand()/double(RAND_MAX);
        p[i].y = -1.0+2.0*rand()/double(RAND_MAX);
        p[i].z = -1.0+2.0*rand()/double(RAND_MAX);
    }

    useRandomized = 1;
    callsMinFix1 = 0;
    callsMinFix2 = 0;
    callsMinFix3 = 0;
    callsSphere4 = 0;
    minimal = MinimalSphere(n,p);
    cout << callsMinFix1 << ' ';
    cout << callsMinFix2 << ' ';
    cout << callsMinFix3 << ' ';
    cout << callsSphere4 << endl;
    // callsMinFix1 = 22
    // callsMinFix2 = 181
    // callsMinFix3 = 497
    // callsSphere4 = 427

    useRandomized = 0;
    callsMinFix1 = 0;
    callsMinFix2 = 0;
    callsMinFix3 = 0;
    callsSphere4 = 0;
    minimal = MinimalSphere(n,p);
    cout << callsMinFix1 << ' ';
    cout << callsMinFix2 << ' ';
    cout << callsMinFix3 << ' ';
    cout << callsSphere4 << endl;
    // callsMinFix1 = 30
    // callsMinFix2 = 310
    // callsMinFix3 = 1067
    // callsSphere4 = 1194

    // Example 2.  Points are all on the unit sphere and are ordered
    // by the spherical coordinate parameterization.  Without randomization,
    // the structured nature of this point set causes the algorithm to take
    // too much time.  (Intermediate bounding spheres grow gradually
    // because the points are visited in order about the sphere.  The
    // randomization causes the first intermediate bounding sphere to
    // be quite large.)
    const int sqrtn = 128;  // = sqrt(16384)
    double Pi = 4.0*atan(1.0);
    double twoPi = 2.0*Pi;
    int k = 0;
    for (i = 0; i < sqrtn; i++)
    {
        double phi = i*Pi/sqrtn;
        double csp = cos(phi);
        double snp = sin(phi);
        for (int j = 0; j < sqrtn; j++)
        {
            double theta = j*twoPi/sqrtn;
            double cst = cos(theta);
            double snt = sin(theta);

            p[k].x = cst*snp;
            p[k].y = snt*snp;
            p[k].z = csp;
            k++;
        }
    }

    useRandomized = 1;
    callsMinFix1 = 0;
    callsMinFix2 = 0;
    callsMinFix3 = 0;
    callsSphere4 = 0;
    minimal = MinimalSphere(n,p);
    cout << callsMinFix1 << ' ';
    cout << callsMinFix2 << ' ';
    cout << callsMinFix3 << ' ';
    cout << callsSphere4 << endl;
    // callsMinFix1 = 13
    // callsMinFix2 = 58
    // callsMinFix3 = 108
    // callsSphere4 = 118

    useRandomized = 0;
    callsMinFix1 = 0;
    callsMinFix2 = 0;
    callsMinFix3 = 0;
    callsSphere4 = 0;
    minimal = MinimalSphere(n,p);
    cout << callsMinFix1 << ' ';
    cout << callsMinFix2 << ' ';
    cout << callsMinFix3 << ' ';
    cout << callsSphere4 << endl;
    // callsMinFix1 = 100
    // callsMinFix2 = 2439
    // callsMinFix3 = 146922
    // callsSphere4 = 11036793

    delete[] p;
}

#endif
