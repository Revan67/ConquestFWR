//
// <chull3d.cpp>
//
//
// The construction of a convex hull in 3D is a modification of code
// written by Dave Watson.  It uses an algorithm described in
//
//     Watson, D.F., 1981, Computing the n-dimensional Delaunay 
//     tessellation with application to Voronoi polytopes: 
//     The Computer J., 24(2), p. 167-172.

#include <stdlib.h>
#include <float.h>
#include <math.h>
#include "chull3d.h"

//

int ConvexHull3D (int N, SINGLE* x, SINGLE* y, SINGLE* z, 
                                  int& numTriangles, CTriangle * triangles)
{
        int result;

        SINGLE** point = new SINGLE*[N+4];
        int i;
        for (i = 0; i < N+4; i++)
                point[i] = new SINGLE[3];
        for (i = 0; i < N; i++)
        {
                point[i][0] = x[i];
                point[i][1] = y[i];
                point[i][2] = z[i];
        }

        const SINGLE EPSILON = 0.00001;
        const int TSIZE = 75;
        const SINGLE RANGE = 10.0;

        SINGLE xmin = point[0][0], xmax = xmin;
        SINGLE ymin = point[0][1], ymax = ymin;
        SINGLE zmin = point[0][2], zmax = zmin;

        for (i = 1; i < N; i++)
        {
                SINGLE value = point[i][0];
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

        SINGLE xrange = xmax-xmin, yrange = ymax-ymin, zrange = zmax-zmin;
        SINGLE maxrange = xrange;
        if ( maxrange < yrange )
                maxrange = yrange;
        if ( maxrange < zrange )
                maxrange = zrange;

        // need to scale the data later to do a correct tetrahedron count
        SINGLE maxrange3 = maxrange*maxrange*maxrange;

        // tweak the points by very small random numbers
        SINGLE bgs = EPSILON*maxrange;
        srand(367);
        for (i = 0; i < N; i++)
        {
                point[i][0] += bgs*(0.5-rand()/SINGLE(RAND_MAX));
                point[i][1] += bgs*(0.5-rand()/SINGLE(RAND_MAX));
                point[i][2] += bgs*(0.5-rand()/SINGLE(RAND_MAX));
        }

        SINGLE wrk[3][4] =
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
        SINGLE xx;

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

        SINGLE** ccr = new SINGLE*[i1];  // circumscribed centers and radii
        ccr[0] = new SINGLE[4*i1];
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
                                SINGLE z = point[i0][i2]-ccr[i1][i2];
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
                                        // tetrahedron estimation is fucked
                                        __asm int 0x03;
                                        
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
                                        SINGLE z = point[i0][i2] - ccr[id[i4]][i2];
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

        numTriangles = 0;
        i0 = -1;
        for (i11 = 0; i11 < nts; i11++)
        {
                i0++;
                while ( a3s[i0][0] < 0 )
                        i0++;

                if ( a3s[i0][1] < N )
                {
                        CTriangle & tri = triangles[numTriangles];
                        tri[0] = a3s[i0][1];
                        tri[1] = a3s[i0][2];
                        tri[2] = a3s[i0][3];
                        numTriangles++;
                }
        }

        result = 1;

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
