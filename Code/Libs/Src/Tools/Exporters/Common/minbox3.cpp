#include <float.h>
#include <math.h>
#include <stdlib.h>
#include <assert.h>
//#include <stdio.h>

#ifndef SGI
#pragma warning( 3 : 4100 ) // unreferenced formal parameter
#pragma warning( 3 : 4189 ) // local variable is initialized but not referenced
#pragma warning( error : 4701 ) // variable may be used without having been initialized
#pragma warning( error : 4700 )
#pragma warning( 3 : 4706 ) // assignment within conditional expression
#endif

#include "eigen.h"
#include "minbox3.h"

//extern int Winprint(const char *format, ...);
//static const double pi = 4.0*atan(1.0);
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

//---------------------------------------------------------------------------
static void MatrixToAngleAxis (double** R, double& angle, double axis[3])
{
    // Let (x,y,z) be the unit-length axis and let A be an angle of rotation.
    // The rotation matrix is R = I + sin(A)*P + (1-cos(A))*P^2 where
    // I is the identity and
    //
    //       +-        -+
    //   P = |  0 +z -y |
    //       | -z  0 +x |
    //       | +y -x  0 |
    //       +-        -+
    //
    // Some algebra will show that
    //
    //   cos(A) = (trace(R)-1)/2  and  R - R^t = 2*sin(A)*P
    //
    // In the event that A = pi, R-R^t = 0 which prevents us from extracting
    // the axis through P.  Instead note that R = I+2*P^2 when A = pi, so
    // P^2 = (R-I)/2.  The diagonal entries of P^2 are x^2-1, y^2-1, and z^2-1.
    // We can solve these for axis (x,y,z).  Because the angle is pi, it does
    // not matter which sign you choose on the square roots.

    double trace = R[0][0]+R[1][1]+R[2][2];
    double cs = 0.5*(trace-1.0);
    if ( -1 < cs )
    {
        if ( cs < 1 )
            angle = acos(cs);
        else
            angle = 0;
    }
    else
    {
        angle = M_PI;
    }

    axis[0] = R[1][2]-R[2][1];
    axis[1] = R[2][0]-R[0][2];
    axis[2] = R[0][1]-R[1][0];
    double length = sqrt(axis[0]*axis[0]+axis[1]*axis[1]+axis[2]*axis[2]);
    const double epsilon = 1e-06;
    if ( length > epsilon )
    {
        axis[0] /= length;
        axis[1] /= length;
        axis[2] /= length;
    }
    else  // angle is 0 or pi
    {
        if ( angle > 1.0 )  // any number strictly between 0 and pi works
        {
            // angle must be pi
            axis[0] = sqrt(0.5*fabs((1.0+R[0][0])));
            axis[1] = sqrt(0.5*fabs((1.0+R[1][1])));
            axis[2] = sqrt(0.5*fabs((1.0+R[2][2])));

            // determine signs of axis components
            double test[3];
            test[0] = R[0][0]*axis[0]+R[0][1]*axis[1]+R[0][2]*axis[2]-axis[0];
            test[1] = R[1][0]*axis[0]+R[1][1]*axis[1]+R[1][2]*axis[2]-axis[1];
            test[2] = R[2][0]*axis[0]+R[2][1]*axis[1]+R[2][2]*axis[2]-axis[2];
            length = test[0]*test[0]+test[1]*test[1]+test[2]*test[2];
            if ( length < epsilon )
                return;

            axis[1] = -axis[1];
            test[0] = R[0][0]*axis[0]+R[0][1]*axis[1]+R[0][2]*axis[2]-axis[0];
            test[1] = R[1][0]*axis[0]+R[1][1]*axis[1]+R[1][2]*axis[2]-axis[1];
            test[2] = R[2][0]*axis[0]+R[2][1]*axis[1]+R[2][2]*axis[2]-axis[2];
            length = test[0]*test[0]+test[1]*test[1]+test[2]*test[2];
            if ( length < epsilon )
                return;

            axis[2] = -axis[2];
            test[0] = R[0][0]*axis[0]+R[0][1]*axis[1]+R[0][2]*axis[2]-axis[0];
            test[1] = R[1][0]*axis[0]+R[1][1]*axis[1]+R[1][2]*axis[2]-axis[1];
            test[2] = R[2][0]*axis[0]+R[2][1]*axis[1]+R[2][2]*axis[2]-axis[2];
            length = test[0]*test[0]+test[1]*test[1]+test[2]*test[2];
            if ( length < epsilon )
                return;

            axis[1] = -axis[1];
            test[0] = R[0][0]*axis[0]+R[0][1]*axis[1]+R[0][2]*axis[2]-axis[0];
            test[1] = R[1][0]*axis[0]+R[1][1]*axis[1]+R[1][2]*axis[2]-axis[1];
            test[2] = R[2][0]*axis[0]+R[2][1]*axis[1]+R[2][2]*axis[2]-axis[2];
            length = test[0]*test[0]+test[1]*test[1]+test[2]*test[2];
            if ( length < epsilon )
                return;
        }
        else
        {
            // Angle is zero, matrix is the identity, no unique axis, so
            // return (1,0,0) for as good a guess as any.
            axis[0] = 1.0;
            axis[1] = 0.0;
            axis[2] = 0.0;
        }
    }
}
//---------------------------------------------------------------------------
static void AngleAxisToMatrix (double angle, double axis[3], double R[3][3])
{
    double cs = cos(angle), sn = sin(angle);
    double length = sqrt(axis[0]*axis[0]+axis[1]*axis[1]+axis[2]*axis[2]);
    double x = axis[0]/length;
    double y = axis[1]/length;
    double z = axis[2]/length;
    double omc = 1.0-cs;
    double x2 = x*x, y2 = y*y, z2 = z*z;
    double xy = x*y, xz = x*z, yz = y*z;
    double snx = sn*x, sny = sn*y, snz = sn*z;
    
    R[0][0] = 1.0-omc*(y2+z2);
    R[0][1] = +snz+omc*xy;
    R[0][2] = -sny+omc*xz;
    R[1][0] = -snz+omc*xy;
    R[1][1] = 1.0-omc*(x2+z2);
    R[1][2] = +snx+omc*yz;
    R[2][0] = +sny+omc*xz;
    R[2][1] = -snx+omc*yz;
    R[2][2] = 1.0-omc*(x2+y2);
}
//---------------------------------------------------------------------------
static double Volume (int N, ExtPoint3* pt, double angle[3])
{
    double cs0 = cos(angle[0]);
    double sn0 = sin(angle[0]);
    double cs1 = cos(angle[1]);
    double sn1 = sin(angle[1]);
    double axis[3] = { cs0*sn1, sn0*sn1, cs1 };
    double rot[3][3];
    AngleAxisToMatrix(angle[2], axis, rot);

    double min[3] =
    {
        rot[0][0]*pt[0].x+rot[1][0]*pt[0].y+rot[2][0]*pt[0].z,
        rot[0][1]*pt[0].x+rot[1][1]*pt[0].y+rot[2][1]*pt[0].z,
        rot[0][2]*pt[0].x+rot[1][2]*pt[0].y+rot[2][2]*pt[0].z
    };

    double max[3] = { min[0], min[1], min[2] };

    for (int i = 1; i < N; i++)
    {
        double test[3] =
        {
            rot[0][0]*pt[i].x + rot[1][0]*pt[i].y + rot[2][0]*pt[i].z,

            rot[0][1]*pt[i].x + rot[1][1]*pt[i].y + rot[2][1]*pt[i].z,

            rot[0][2]*pt[i].x + rot[1][2]*pt[i].y + rot[2][2]*pt[i].z
        };

        if ( test[0] < min[0] )
            min[0] = test[0];
        else if ( test[0] > max[0] )
            max[0] = test[0];

        if ( test[1] < min[1] )
            min[1] = test[1];
        else if ( test[1] > max[1] )
            max[1] = test[1];

        if ( test[2] < min[2] )
            min[2] = test[2];
        else if ( test[2] > max[2] )
            max[2] = test[2];
    }

    double volume = (max[0]-min[0])*(max[1]-min[1])*(max[2]-min[2]);
    return volume;
}
//---------------------------------------------------------------------------
static void MinimalBoxForAngles (int N, ExtPoint3* pt, double angle[3],
                                 OBBox3& box)
{
    double cs0 = cos(angle[0]), sn0 = sin(angle[0]);
    double cs1 = cos(angle[1]), sn1 = sin(angle[1]);
    double axis[3] = { cs0*sn1, sn0*sn1, cs1 };
    double rot[3][3];
    AngleAxisToMatrix(angle[2],axis,rot);

    double min[3] =
    {
        rot[0][0]*pt[0].x+rot[1][0]*pt[0].y+rot[2][0]*pt[0].z,
        rot[0][1]*pt[0].x+rot[1][1]*pt[0].y+rot[2][1]*pt[0].z,
        rot[0][2]*pt[0].x+rot[1][2]*pt[0].y+rot[2][2]*pt[0].z
    };

    double max[3] = { min[0], min[1], min[2] };

    for (int i = 1; i < N; i++)
    {
        double test[3] =
        {
            rot[0][0]*pt[i].x+rot[1][0]*pt[i].y+rot[2][0]*pt[i].z,
            rot[0][1]*pt[i].x+rot[1][1]*pt[i].y+rot[2][1]*pt[i].z,
            rot[0][2]*pt[i].x+rot[1][2]*pt[i].y+rot[2][2]*pt[i].z
        };

        if ( test[0] < min[0] )
            min[0] = test[0];
        else if ( test[0] > max[0] )
            max[0] = test[0];

        if ( test[1] < min[1] )
            min[1] = test[1];
        else if ( test[1] > max[1] )
            max[1] = test[1];

        if ( test[2] < min[2] )
            min[2] = test[2];
        else if ( test[2] > max[2] )
            max[2] = test[2];
    }

    double mid[3] =
    {
        0.5*(max[0]+min[0]), 0.5*(max[1]+min[1]), 0.5*(max[2]+min[2])
    };

    box.center.x = mid[0]*rot[0][0]+mid[1]*rot[0][1]+mid[2]*rot[0][2];
    box.center.y = mid[0]*rot[1][0]+mid[1]*rot[1][1]+mid[2]*rot[1][2];
    box.center.z = mid[0]*rot[2][0]+mid[1]*rot[2][1]+mid[2]*rot[2][2];
    box.axis[0].x = rot[0][0];
    box.axis[0].y = rot[1][0];
    box.axis[0].z = rot[2][0];
    box.axis[1].x = rot[0][1];
    box.axis[1].y = rot[1][1];
    box.axis[1].z = rot[2][1];
    box.axis[2].x = rot[0][2];
    box.axis[2].y = rot[1][2];
    box.axis[2].z = rot[2][2];
    box.extent[0] = 0.5*(max[0]-min[0]);
    box.extent[1] = 0.5*(max[1]-min[1]);
    box.extent[2] = 0.5*(max[2]-min[2]);
}
//---------------------------------------------------------------------------
static void GetInterval (double A[3], double D[3], double& tmin, double& tmax)
{
    static const double angleMin[3] = { -M_PI, 0.0, 0.0 };
    static const double angleMax[3] = {  M_PI,  M_PI,  M_PI };

    tmin = -DBL_MAX;
    tmax = +DBL_MAX;

	const double epsilon = 1e-09;

    for (int i = 0; i < 3; i++)
    {    
        double b0 = angleMin[i]-A[i];
        double b1 = angleMax[i]-A[i];

        double inv, tmp;
        if ( D[i] > epsilon )
        {
            inv = 1.0/D[i];
            tmp = inv*b0;
            if ( tmp > tmin )
                tmin = tmp;
            tmp = inv*b1;
            if ( tmp < tmax )
                tmax = tmp;
        }
        else if ( D[i] < -epsilon )
        {
            inv = 1.0/D[i];
            tmp = inv*b0;
            if ( tmp < tmax )
                tmax = tmp;
            tmp = inv*b1;
            if ( tmp > tmin )
                tmin = tmp;
        }
    }

	if(tmax == tmin)
	{
		tmax += epsilon;
		tmin -= epsilon;
	}

    assert(tmin != -DBL_MAX);
    assert(tmax != +DBL_MAX);
}
//---------------------------------------------------------------------------
static void Combine (double result[3], double A[3], double t, double D[3])
{
    for (int i = 0; i < 3; i++)
        result[i] = A[i]+t*D[i];
}

//---------------------------------------------------------------------------
static double MinimizeOnInterval (int N, ExtPoint3* pt, double A[3], double D[3])
{
    // compute intersection of line A+t*D with domain of function
    double tmin, tmax;
    GetInterval(A,D,tmin,tmax);
    double tran = tmax-tmin;
    double angle[3];

    // bracket a minimum for angles in [A+tmin*D,A+tmax*D]
    double t = 0.0;
    double volumeMin = Volume(N,pt,A);
    double volume;

    const int maxPartition = 64;
    int i, imin;
    for (i = 0, imin = -1; i <= maxPartition; i++)
    {
        t = tmin+i*tran/maxPartition;
        Combine(angle,A,t,D);

        volume = Volume(N,pt,angle);
        if ( volume < volumeMin )
        {
            imin = i;
            volumeMin = volume;
        }
    }

    if ( imin != -1 )
    {
        t = tmin+imin*tran/maxPartition;
    }
    else
    {
        t = 0.0;

        // interval in which t=0 lies
        imin = int(-tmin*maxPartition/tran+0.5);
    }
    volume = volumeMin;

    double t0 = tmin+(imin-1)*tran/maxPartition;
    Combine(angle,A,t0,D);
    double volume0 = Volume(N,pt,angle);

    double t1 = tmin+(imin+1)*tran/maxPartition;
    Combine(angle,A,t1,D);
    double volume1 = Volume(N,pt,angle);
    
    // use inverse parabolic interpolation to find the minimum
    const int invInterp = 64;
    for (i = 0; i <= invInterp; i++)
    {
        double tMid, volumeMid;

        // test for convergence (do not change these parameters)
        const double epsilon = 1e-08, tol = 1e-04; // omtol = 1.0-tol;
        if ( fabs(t1-t0) <= 2*tol*fabs(t)+epsilon )
            break;

        // compute vertex of interpolating parabola
        double dt0 = t0-t, dt1 = t1-t;
        double dvolume0 = volume0-volume, dvolume1 = volume1-volume;
        double temp0 = dt0*dvolume1, temp1 = dt1*dvolume0;
        double delta = temp1-temp0;
        if ( fabs(delta) < epsilon )
           break;

        tMid = t+0.5*(dt1*temp1-dt0*temp0)/(temp1-temp0);

        // update bracket
        if ( tMid < t )
        {
            Combine(angle,A,tMid,D);
            volumeMid = Volume(N,pt,angle);
            if ( volumeMid <= volume )
            {
                t1 = t;
                volume1 = volume;
                t = tMid;
                volume = volumeMid;
            }
            else
            {
                t0 = tMid;
                volume0 = volumeMid;
            }
        }
        else if ( tMid > t )
        {
            Combine(angle,A,tMid,D);
            volumeMid = Volume(N,pt,angle);
            if ( volumeMid <= volume )
            {
                t0 = t;
                volume0 = volume;
                t = tMid;
                volume = volumeMid;
            }
            else
            {
                t1 = tMid;
                volume1 = volumeMid;
            }
        }
        else
        {
            // bracket middle already vertex of parabola
            break;
        }
    }

    Combine(A,A,t,D);
    return volume;
}
//---------------------------------------------------------------------------
static double MinimizeOnLattice (int N, ExtPoint3* pt, double A[3], int layers,
                                 double thickness)
{
    int xmin = 0, ymin = 0, zmin = 0;
    double volume = Volume(N,pt,A);

    double angle[3];
    for (int z = -layers; z <= layers; z++)
    {
        angle[2] = A[2]+thickness*z/layers;
        for (int y = -layers; y <= layers; y++)
        {
            angle[1] = A[1]+thickness*y/layers;
            for (int x = -layers; x <= layers; x++)
            {
                angle[0] = A[0]+thickness*x/layers;

                double v = Volume(N,pt,angle);
                if ( v < volume )
                {
                    xmin = x;
                    ymin = y;
                    zmin = z;
                    volume = v;
                }
            }
        }
    }

    A[0] += thickness*xmin/layers;
    A[1] += thickness*ymin/layers;
    A[2] += thickness*zmin/layers;

    return volume;
}
//---------------------------------------------------------------------------
static void InitialGuess (int N, ExtPoint3* pt, double angle[3])
{
    int i;

    assert(N != 0);
    // compute mean of poitns
    double xsum = 0.0, ysum = 0.0, zsum = 0.0;
    for (i = 0; i < N; i++)
    {
        xsum += pt[i].x;
        ysum += pt[i].y;
        zsum += pt[i].z;
    }
    double xmean = xsum/N;
    double ymean = ysum/N;
    double zmean = zsum/N;

    // compute covariances of points
    double xxsum = 0.0f, xysum = 0.0f, xzsum = 0.0f;
    double yysum = 0.0f, yzsum = 0.0f, zzsum = 0.0f;
    for (i = 0; i < N; i++)
    {
        double dx = pt[i].x - xmean;
        double dy = pt[i].y - ymean;
        double dz = pt[i].z - zmean;
        xxsum += dx*dx;
        xysum += dx*dy;
        xzsum += dx*dz;
        yysum += dy*dy;
        yzsum += dy*dz;
        zzsum += dz*dz;
    }
    double xxcov = xxsum/N;
    double xycov = xysum/N;
    double xzcov = xzsum/N;
    double yycov = yysum/N;
    double yzcov = yzsum/N;
    double zzcov = zzsum/N;

    // compute eigenvectors for covariance matrix
    mgcEigenD eig(3);
    eig.Matrix(0,0) = xxcov;
    eig.Matrix(0,1) = xycov;
    eig.Matrix(0,2) = xzcov;
    eig.Matrix(1,0) = xycov;
    eig.Matrix(1,1) = yycov;
    eig.Matrix(1,2) = yzcov;
    eig.Matrix(2,0) = xzcov;
    eig.Matrix(2,1) = yzcov;
    eig.Matrix(2,2) = zzcov;
    eig.EigenStuff3();

    // Use eigenvectors as the box axes.  Eigenmatrix must not have a
    // reflection component, thus the check for negative determinant.
    const double epsilon = 1e-06;
    double** R = (double**)eig.Eigenvector();
    double det =
        +R[0][0]*R[1][1]*R[2][2]
        +R[0][1]*R[1][2]*R[2][0]
        +R[0][2]*R[1][0]*R[2][1]
        -R[0][2]*R[1][1]*R[2][0]
        -R[0][1]*R[1][0]*R[2][2]
        -R[0][0]*R[1][2]*R[2][1];
    if ( det < 0.0 )
    {
        R[0][0] = -R[0][0];
        R[1][0] = -R[1][0];
        R[2][0] = -R[2][0];
    }

    // extract angles from rotation axis = (cos(u)sin(v),sin(u)sin(v),cos(v))
    double axis[3];
    MatrixToAngleAxis(R,angle[2],axis);
    if ( -1+epsilon < axis[2] )
    {
        if ( axis[2] < 1-epsilon )
        {
            angle[0] = atan2(axis[1],axis[0]);
            angle[1] = acos(axis[2]);
        }
        else
        {
            angle[0] = 0;
            angle[1] = 0;
        }
    }
    else
    {
        angle[0] = 0;
        angle[1] = M_PI;
    }
}
//---------------------------------------------------------------------------
OBBox3 MinimalBox3 (int N, ExtPoint3* pt)
{
    // compute a good initial guess for an oriented bounding box
    double angle[3];
    InitialGuess(N,pt,angle);
    double oldVolume = Volume(N,pt,angle);
    double saveAngle[3] = { angle[0], angle[1], angle[2] };

    // Powell's direction set method
    double U[3][3], volume;
    const int maxiters = 3*24;//3*16;
    for (int iter = 0; iter < maxiters; iter++)
    {
        // reset directions to avoid linear dependence degeneration
        if ( iter % 3 == 0 )
        {
            U[0][0] = 1.0;  U[0][1] = 0.0;  U[0][2] = 0.0;
            U[1][0] = 0.0;  U[1][1] = 1.0;  U[1][2] = 0.0;
            U[2][0] = 0.0;  U[2][1] = 0.0;  U[2][2] = 1.0;
        }

        // find minima in specified directions
        for (int d = 0; d < 3; d++)
            volume = MinimizeOnInterval(N,pt,angle,U[d]);

        // estimate a conjugate direction
        double conj[3] =
        {
            angle[0]-saveAngle[0],
            angle[1]-saveAngle[1],
            angle[2]-saveAngle[2]
        };

        // if all elements are too small things blow up
        if ( fabs(conj[0]) <= 1e-09 && fabs(conj[1]) <= 1e-09 && fabs(conj[2]) <= 1e-09)
        {
            double tmpx, tmpy, tmpz;
            if(fabs(angle[0]) + fabs(angle[1]) + fabs(angle[2]) > 0.0)
            {
              tmpx = .001 * fabs(angle[0]) * (.5 - (double)rand()/(double)RAND_MAX);
              tmpy = .001 * fabs(angle[1]) * (.5 - (double)rand()/(double)RAND_MAX);
              tmpz = .001 * fabs(angle[2]) * (.5 - (double)rand()/(double)RAND_MAX);
            }
            else
            {
              tmpx = M_PI/3600.0 * (.5 - (double)rand()/(double)RAND_MAX);
              tmpy = M_PI/3600.0 * (.5 - (double)rand()/(double)RAND_MAX);
              tmpz = M_PI/3600.0 * (.5 - (double)rand()/(double)RAND_MAX);
            }

			bool flag=false;
            do
            {
                angle[0] += tmpx;
                angle[1] += tmpy;
                angle[2] += tmpz;

				if(flag && conj[0] == 0.0 && angle[0] - saveAngle[0] == 0.0)
				{
					conj[0] = 1e-08 * (double)rand()/(double)RAND_MAX;
				}
				else
				{
					conj[0] = angle[0] - saveAngle[0];
				}

				if(flag && conj[1] == 0.0 && angle[1] - saveAngle[1] == 0.0)
				{
					conj[1] = 1e-08 * (double)rand()/(double)RAND_MAX;
				}
				else
				{
					conj[1] = angle[1] - saveAngle[1];
				}

				if(flag && conj[2] == 0.0 && angle[2] - saveAngle[2] == 0.0)
				{
					conj[2] = 1e-08 * (double)rand()/(double)RAND_MAX;
				}
				else
				{
					conj[2] = angle[2] - saveAngle[2];
				}
                
				flag = true;
            } while( (fabs(conj[0]) <= 1e-09) && 
                     (fabs(conj[1]) <= 1e-09) && 
                     (fabs(conj[2]) <= 1e-09));
        }
        
        double length = sqrt(conj[0]*conj[0]+conj[1]*conj[1]+conj[2]*conj[2]);

        if ( length >= 1e-06 )
        {
            double invLen = 1.0/length;
            conj[0] *= invLen;
            conj[1] *= invLen;
            conj[2] *= invLen;
            
            // minimize in conjugate direction
            volume = MinimizeOnInterval(N,pt,angle,conj);
        }
        else
        {
            // Possible local, but not global, minimum.  Search nearby for
            // a smaller volume.
            volume = MinimizeOnLattice(N,pt,angle,2,0.0001);
            volume = MinimizeOnLattice(N,pt,angle,2,0.0010);
            volume = MinimizeOnLattice(N,pt,angle,2,0.0100);
            volume = MinimizeOnLattice(N,pt,angle,2,0.1000);
        }

        // test for convergence
        const double epsilon = 1e-04;
        double diff = fabs(volume-oldVolume);
        if ( diff <= epsilon )
        {
            // Possible local, but not global, minimum.  Search nearby for
            // a smaller volume.
            volume = MinimizeOnLattice(N,pt,angle,2,0.0001);
            volume = MinimizeOnLattice(N,pt,angle,2,0.0010);
            volume = MinimizeOnLattice(N,pt,angle,2,0.0100);
            volume = MinimizeOnLattice(N,pt,angle,2,0.1000);
            diff = fabs(volume-oldVolume);
            if ( diff <= epsilon )
                break;
        }

        // cycle the directions and add conjugate direction to list
        U[0][0] = U[1][0];  U[0][1] = U[1][1];  U[0][2] = U[1][2];
        U[1][0] = U[2][0];  U[1][1] = U[2][1];  U[1][2] = U[2][2];
        U[2][0] = conj[0];  U[2][1] = conj[1];  U[2][2] = conj[2];

        // set parameters for next pass
        oldVolume = volume;
        saveAngle[0] = angle[0];
        saveAngle[1] = angle[1];
        saveAngle[2] = angle[2];
    }

    OBBox3 box;
    MinimalBoxForAngles(N,pt,angle,box);

	// avoid 2D box
	#define T_TOLERANCE .00001
    if(box.extent[0]<T_TOLERANCE)
    {
        box.extent[0] = (box.extent[1]<box.extent[2]) ? box.extent[1]/100.0 : box.extent[2]/100.0;
    }
    if(box.extent[1]<T_TOLERANCE)
    {
        box.extent[1] = (box.extent[0]<box.extent[2]) ? box.extent[0]/100.0 : box.extent[2]/100.0;
    }
    if(box.extent[2]<T_TOLERANCE)
    {
        box.extent[2] = (box.extent[0]<box.extent[1]) ? box.extent[0]/100.0 : box.extent[1]/100.0;
    }
    #undef T_TOLERANCE

    // pad to avoid numerical accuracy problems
    box.extent[0] *= 1.00001f;
    box.extent[1] *= 1.00001f;
    box.extent[2] *= 1.00001f;

    box.volume = 8.0f * box.extent[0] * box.extent[1] * box.extent[2];

#ifdef WIN32
	//_control87(_CW_DEFAULT, 0xfffff);
#endif
	
    return box;
}
//---------------------------------------------------------------------------

/*
#ifdef MINBOX3_TEST

#define RAND (rand()/double(RAND_MAX))

void main ()
{
    // build box with axes parallel to coordinate axes
    const int N = 16;
    const double ex = 1.0;
    const double ey = 2.0;
    const double ez = 3.0;
    ExtPoint3 pt[N];
    pt[0].x = -ex;  pt[0].y = -ey;  pt[0].z = -ez;
    pt[1].x = -ex;  pt[1].y = +ey;  pt[1].z = -ez;
    pt[2].x = +ex;  pt[2].y = +ey;  pt[2].z = -ez;
    pt[3].x = +ex;  pt[3].y = -ey;  pt[3].z = -ez;
    pt[4].x = -ex;  pt[4].y = -ey;  pt[4].z = +ez;
    pt[5].x = -ex;  pt[5].y = +ey;  pt[5].z = +ez;
    pt[6].x = +ex;  pt[6].y = +ey;  pt[6].z = +ez;
    pt[7].x = +ex;  pt[7].y = -ey;  pt[7].z = +ez;

    for (int k = 8; k < N; k++)
    {
        // generate random points inside box to confound initial Gaussian fit
        pt[k].x = -ex+2.0*ex*RAND;
        pt[k].y = -ey+2.0*ey*RAND;
        pt[k].z = -ez+2.0*ez*RAND;
    }

    double maxNorm = 0.0;
    int iMaxNorm = -1;
    for (int iter = 0; iter < 1024; iter++)
    {
        // build arbitrary rotation matrix
        double angle = RAND;
        double line[3] = { RAND, RAND, RAND };
        double rot[3][3];
        double length = sqrt(line[0]*line[0]+line[1]*line[1]+line[2]*line[2]);
        line[0] /= length;
        line[1] /= length;
        line[2] /= length;
        AngleAxisToMatrix(angle,line,rot);
        
        // rotate box
        ExtPoint3 rpt[N];
        for (int i = 0; i < N; i++)
        {
            rpt[i].x = rot[0][0]*pt[i].x+rot[0][1]*pt[i].y+rot[0][2]*pt[i].z;
            rpt[i].y = rot[1][0]*pt[i].x+rot[1][1]*pt[i].y+rot[1][2]*pt[i].z;
            rpt[i].z = rot[2][0]*pt[i].x+rot[2][1]*pt[i].y+rot[2][2]*pt[i].z;
        }
        
        OBBox3 minimal = MinimalBox3(N,rpt);

        int index[3];
        if ( minimal.extent[0] <= minimal.extent[1] )
        {
            if ( minimal.extent[1] <= minimal.extent[2] )
            {
                index[0] = 0;
                index[1] = 1;
                index[2] = 2;
            }
            else
            {
                if ( minimal.extent[0] <= minimal.extent[2] )
                {
                    index[0] = 0;
                    index[1] = 2;
                    index[2] = 1;
                }
                else
                {
                    index[0] = 2;
                    index[1] = 0;
                    index[2] = 1;
                }
            }
        }
        else
        {
            if ( minimal.extent[0] <= minimal.extent[2] )
            {
                index[0] = 1;
                index[1] = 0;
                index[2] = 2;
            }
            else
            {
                if ( minimal.extent[1] <= minimal.extent[2] )
                {
                    index[0] = 1;
                    index[1] = 2;
                    index[2] = 0;
                }
                else
                {
                    index[0] = 2;
                    index[1] = 1;
                    index[2] = 0;
                }
            }
        }

        double dx = ex-minimal.extent[index[0]];
        double dy = ey-minimal.extent[index[1]];
        double dz = ez-minimal.extent[index[2]];
        double norm = sqrt(dx*dx+dy*dy+dz*dz);
        if ( norm > maxNorm )
        {
            maxNorm = norm;
            iMaxNorm = iter;
        }
    }
}

#endif
*/
