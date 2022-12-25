#if _MFC_VER >= 0x0700
	#include <fstream.h>
#else
	#include <fstream>
#endif
#include <math.h>

#ifndef SGI
#pragma warning( 3 : 4100 ) // unreferenced formal parameter
#pragma warning( 3 : 4189 ) // local variable is initialized but not referenced
#pragma warning( error : 4701 ) // variable may be used without having been initialized
#pragma warning( error : 4700 )
#pragma warning( 3 : 4706 ) // assignment within conditional expression
#endif

#include "eigen.h"

//===========================================================================
// error handling
int mgcEigenD::verbose = 0;
unsigned mgcEigenD::error = 0;
const unsigned mgcEigenD::invalid_size      = 0x00000001;
const unsigned mgcEigenD::allocation_failed = 0x00000002;
const unsigned mgcEigenD::ql_exceeded       = 0x00000004;
const char* mgcEigenD::message[3] = {
        "invalid matrix size",
        "allocation failed",
        "QL algorithm - exceeded maximum iterations"
};
//---------------------------------------------------------------------------
mgcEigenD::
mgcEigenD (int _size)
{
        if ( (size = _size) <= 1 ) {
                //Report(invalid_size);
                return;
        }
        if ( (mat = new double*[size]) == 0 ) {
                //Report(allocation_failed);
                return;
        }
        for (int d = 0; d < size; d++)
                if ( (mat[d] = new double[size]) == 0 ) {
                        //Report(allocation_failed);
                        return;
                }
        if ( (diag = new double[size]) == 0 ) {
                //Report(allocation_failed);
                return;
        }
        if ( (subd = new double[size]) == 0 ) {
                //Report(allocation_failed);
                return;
        }
}
//---------------------------------------------------------------------------
mgcEigenD::
~mgcEigenD ()
{
        delete[] subd;
        delete[] diag;
        for (int d = 0; d < size; d++)
                delete[] mat[d];
        delete[] mat;
}
//---------------------------------------------------------------------------
void mgcEigenD::
Tridiagonal2 (double** mat, double* diag, double* subd)
{
        // matrix is already tridiagonal

        diag[0] = mat[0][0];
        diag[1] = mat[1][1];
        subd[0] = mat[0][1];
        subd[1] = 0;
        mat[0][0] = 1;  mat[0][1] = 0;
        mat[1][0] = 0;  mat[1][1] = 1;
}
//---------------------------------------------------------------------------
void mgcEigenD::
Tridiagonal3 (double** mat, double* diag, double* subd)
{
        double a = mat[0][0], b = mat[0][1], c = mat[0][2],
                                                 d = mat[1][1], e = mat[1][2],
                                                                                f = mat[2][2];

        diag[0] = a;
        subd[2] = 0;
        if ( c != 0 ) {
                double ell = sqrt(b*b+c*c);
                b /= ell;
                c /= ell;
                double q = 2*b*e+c*(f-d);
                diag[1] = d+c*q;
                diag[2] = f-c*q;
                subd[0] = ell;
                subd[1] = e-b*q;
                mat[0][0] = 1; mat[0][1] = 0; mat[0][2] = 0;
                mat[1][0] = 0; mat[1][1] = b; mat[1][2] = c;
                mat[2][0] = 0; mat[2][1] = c; mat[2][2] = -b;
        }
        else {
                diag[1] = d;
                diag[2] = f;
                subd[0] = b;
                subd[1] = e;
                mat[0][0] = 1; mat[0][1] = 0; mat[0][2] = 0;
                mat[1][0] = 0; mat[1][1] = 1; mat[1][2] = 0;
                mat[2][0] = 0; mat[2][1] = 0; mat[2][2] = 1;
        }
}
//---------------------------------------------------------------------------
void mgcEigenD::
Tridiagonal4 (double** mat, double* diag, double* subd)
{
        // save matrix M
        double
        a = mat[0][0], b = mat[0][1], c = mat[0][2], d = mat[0][3],
                                   e = mat[1][1], f = mat[1][2], g = mat[1][3],
                                                                  h = mat[2][2], i = mat[2][3],
                                                                                                 j = mat[3][3];

        diag[0] = a;
        subd[3] = 0;

        mat[0][0] = 1; mat[0][1] = 0; mat[0][2] = 0; mat[0][3] = 0;
        mat[1][0] = 0;
        mat[2][0] = 0;
        mat[3][0] = 0;

        if ( c != 0 || d != 0 ) {
                double q11, q12, q13;
                double q21, q22, q23;
                double q31, q32, q33;

                // build column Q1
                double len = sqrt(b*b+c*c+d*d);
                q11 = b/len;
                q21 = c/len;
                q31 = d/len;

                subd[0] = len;

                // compute S*Q1
                double v0 = e*q11+f*q21+g*q31;
                double v1 = f*q11+h*q21+i*q31;
                double v2 = g*q11+i*q21+j*q31;

                diag[1] = q11*v0+q21*v1+q31*v2;

        // build column Q3 = Q1x(S*Q1)
                q13 = q21*v2-q31*v1;
                q23 = q31*v0-q11*v2;
                q33 = q11*v1-q21*v0;
                len = sqrt(q13*q13+q23*q23+q33*q33);
                if ( len > 0 ) {
                        q13 /= len;
                        q23 /= len;
                        q33 /= len;

                        // build column Q2 = Q3xQ1
                        q12 = q23*q31-q33*q21;
                        q22 = q33*q11-q13*q31;
                        q32 = q13*q21-q23*q11;

                        v0 = q12*e+q22*f+q32*g;
                        v1 = q12*f+q22*h+q32*i;
                        v2 = q12*g+q22*i+q32*j;
                        subd[1] = q11*v0+q21*v1+q31*v2;
                        diag[2] = q12*v0+q22*v1+q32*v2;
                        subd[2] = q13*v0+q23*v1+q33*v2;

                        v0 = q13*e+q23*f+q33*g;
                        v1 = q13*f+q23*h+q33*i;
                        v2 = q13*g+q23*i+q33*j;
                        diag[3] = q13*v0+q23*v1+q33*v2;
                }
                else {  // S*Q1 parallel to Q1, choose any valid Q2 and Q3
                        subd[1] = 0;

                        len = q21*q21+q31*q31;
                        if ( len > 0 ) {
                                double tmp = q11-1;
                                q12 = -q21;
                                q22 = 1+tmp*q21*q21/len;
                q32 = tmp*q21*q31/len;

                                q13 = -q31;
                                q23 = q32;
                                q33 = 1+tmp*q31*q31/len;

                                v0 = q12*e+q22*f+q32*g;
                                v1 = q12*f+q22*h+q32*i;
                                v2 = q12*g+q22*i+q32*j;
                                diag[2] = q12*v0+q22*v1+q32*v2;
                                subd[2] = q13*v0+q23*v1+q33*v2;

                                v0 = q13*e+q23*f+q33*g;
                                v1 = q13*f+q23*h+q33*i;
                                v2 = q13*g+q23*i+q33*j;
                                diag[3] = q13*v0+q23*v1+q33*v2;
                        }
                        else {  // Q1 = (+-1,0,0)
                                q12 = 0; q22 = 1; q32 = 0;
                                q13 = 0; q23 = 0; q33 = 1;

                                diag[2] = h;
                                diag[3] = j;
                                subd[2] = i;
                        }
                }

                mat[1][1] = q11; mat[1][2] = q12; mat[1][3] = q13;
                mat[2][1] = q21; mat[2][2] = q22; mat[2][3] = q23;
                mat[3][1] = q31; mat[3][2] = q32; mat[3][3] = q33;
        }
        else {
                diag[1] = e;
                subd[0] = b;
                mat[1][1] = 1;
                mat[2][1] = 0;
                mat[3][1] = 0; 

                if ( g != 0 ) {
                        double ell = sqrt(f*f+g*g);
                        f /= ell;
                        g /= ell;
                        double Q = 2*f*i+g*(j-h);

                        diag[2] = h+g*Q;
                        diag[3] = j-g*Q;
                        subd[1] = ell;
                        subd[2] = i-f*Q;
                        mat[1][2] = 0;  mat[1][3] = 0;
                        mat[2][2] = f;  mat[2][3] = g;
                        mat[3][2] = g;  mat[3][3] = -f;
                }
                else {
                        diag[2] = h;
                        diag[3] = j;
                        subd[1] = f;
                        subd[2] = i;
                        mat[1][2] = 0;  mat[1][3] = 0;
                        mat[2][2] = 1;  mat[2][3] = 0;
                        mat[3][2] = 0;  mat[3][3] = 1;
                }
        }
}
//---------------------------------------------------------------------------
void mgcEigenD::
TridiagonalN (int n, double** mat, double* diag, double* subd)
{
        int i, j, k, ell;

        for (i = n-1, ell = n-2; i >= 1; i--, ell--) {
                double h = 0, scale = 0;

                if ( ell > 0 ) {
                        for (k = 0; k <= ell; k++)
                                scale += fabs(mat[i][k]);
                        if ( scale == 0 )
                                subd[i] = mat[i][ell];
                        else {
                                for (k = 0; k <= ell; k++) {
                                        mat[i][k] /= scale;
                                        h += mat[i][k]*mat[i][k];
                                }
                                double f = mat[i][ell];
                                double g = ( f > 0 ? -sqrt(h) : sqrt(h) );
                                subd[i] = scale*g;
                                h -= f*g;
                                mat[i][ell] = f-g;
                                f = 0;
                                for (j = 0; j <= ell; j++) {
                                        mat[j][i] = mat[i][j]/h;
                                        g = 0;
                                        for (k = 0; k <= j; k++)
                                                g += mat[j][k]*mat[i][k];
                                        for (k = j+1; k <= ell; k++)
                                                g += mat[k][j]*mat[i][k];
                                        subd[j] = g/h;
                                        f += subd[j]*mat[i][j];
                                }
                                double hh = f/(h+h);
                                for (j = 0; j <= ell; j++) {
                                        f = mat[i][j];
                                        subd[j] = g = subd[j] - hh*f;
                                        for (k = 0; k <= j; k++)
                                                mat[j][k] -= f*subd[k]+g*mat[i][k];
                                }
            }
                }
                else
                        subd[i] = mat[i][ell];

                diag[i] = h;
        }

        diag[0] = subd[0] = 0;
        for (i = 0, ell = -1; i <= n-1; i++, ell++) {
                if ( diag[i] ) {
                        for (j = 0; j <= ell; j++) {
                                double sum = 0;
                                for (k = 0; k <= ell; k++)
                                        sum += mat[i][k]*mat[k][j];
                                for (k = 0; k <= ell; k++)
                                        mat[k][j] -= sum*mat[k][i];
                        }
                }
                diag[i] = mat[i][i];
                mat[i][i] = 1;
                for (j = 0; j <= ell; j++)
                        mat[j][i] = mat[i][j] = 0;
        }

        // re-ordering if mgcEigenD::QLAlgorithm is used subsequently
        for (i = 1, ell = 0; i < n; i++, ell++)
                subd[ell] = subd[i];
        subd[n-1] = 0;
}
//---------------------------------------------------------------------------
void mgcEigenD::
QLAlgorithm (int n, double* diag, double* subd, double** mat)
{
        const int eigen_maxiter = 30;

        for (int ell = 0; ell < n; ell++) {
                int iter;
                for (iter = 0; iter < eigen_maxiter; iter++) {
                        int m;
                        for (m = ell; m <= n-2; m++) {
                                double dd = fabs(diag[m])+fabs(diag[m+1]);
                                if ( (double)(fabs(subd[m])+dd) == dd )
                                        break;
                        }
                        if ( m == ell )
                                break;

                        double g = (diag[ell+1]-diag[ell])/(2*subd[ell]);
                        double r = sqrt(g*g+1);
                        if ( g < 0 )
                                g = diag[m]-diag[ell]+subd[ell]/(g-r);
                        else
                                g = diag[m]-diag[ell]+subd[ell]/(g+r);
                        double s = 1, c = 1, p = 0;
                        for (int i = m-1; i >= ell; i--) {
                                double f = s*subd[i], b = c*subd[i];
                                if ( fabs(f) >= fabs(g) ) {
                                        c = g/f;
                                        r = sqrt(c*c+1);
                                        subd[i+1] = f*r;
                                        c *= (s = 1/r);
                                }
                                else {
                                        s = f/g;
                                        r = sqrt(s*s+1);
                                        subd[i+1] = g*r;
                                        s *= (c = 1/r);
                                }
                                g = diag[i+1]-p;
                                r = (diag[i]-g)*s+2*b*c;
                                p = s*r;
                                diag[i+1] = g+p;
                                g = c*r-b;

                                for (int k = 0; k < n; k++) {
                                        f = mat[k][i+1];
                                        mat[k][i+1] = s*mat[k][i]+c*f;
                                        mat[k][i] = c*mat[k][i]-s*f;
                                }
                        }
                        diag[ell] -= p;
                        subd[ell] = g;
                        subd[m] = 0;
                }
                if ( iter == eigen_maxiter ) {
                        //Report(ql_exceeded);
                        return;
                }
        }
}
//---------------------------------------------------------------------------
void mgcEigenD::
DecreasingSort (int n, double* eigval, double** eigvec)
{
        // sort eigenvalues in decreasing order, e[0] >= ... >= e[n-1]
        for (int i = 0, k; i <= n-2; i++) {
                // locate maximum eigenvalue
                double max = eigval[k=i];
                int j;
                for (j = i+1; j < n; j++)
                        if ( eigval[j] > max )
                                max = eigval[k=j];

                if ( k != i ) {
                        // swap eigenvalues
                        eigval[k] = eigval[i];
                        eigval[i] = max;

                        // swap eigenvectors
                        for (j = 0; j < n; j++) {
                                double tmp = eigvec[j][i];
                                eigvec[j][i] = eigvec[j][k];
                                eigvec[j][k] = tmp;
                        }
                }
        }
}
//---------------------------------------------------------------------------
void mgcEigenD::
IncreasingSort (int n, double* eigval, double** eigvec)
{
        // sort eigenvalues in increasing order, e[0] <= ... <= e[n-1]
        for (int i = 0, k; i <= n-2; i++) {
                // locate minimum eigenvalue
                double min = eigval[k=i];
        int j;
                for (j = i+1; j < n; j++)
                        if ( eigval[j] < min )
                                min = eigval[k=j];

                if ( k != i ) {
                        // swap eigenvalues
                        eigval[k] = eigval[i];
                        eigval[i] = min;

                        // swap eigenvectors
                        for (j = 0; j < n; j++) {
                                double tmp = eigvec[j][i];
                                eigvec[j][i] = eigvec[j][k];
                                eigvec[j][k] = tmp;
                        }
                }
        }
}
//---------------------------------------------------------------------------
mgcEigenD& mgcEigenD::
Matrix (double** inmat)
{
        for (int row = 0; row < size; row++)
                for (int col = 0; col < size; col++)
                        mat[row][col] = inmat[row][col];
        return *this;
}
//---------------------------------------------------------------------------
void mgcEigenD::
EigenStuff2 ()
{
        Tridiagonal2(mat,diag,subd);
        QLAlgorithm(size,diag,subd,mat);
}
//---------------------------------------------------------------------------
void mgcEigenD::
EigenStuff3 ()
{
        Tridiagonal3(mat,diag,subd);
        QLAlgorithm(size,diag,subd,mat);
}
//---------------------------------------------------------------------------
void mgcEigenD::
EigenStuff4 ()
{
        Tridiagonal4(mat,diag,subd);
        QLAlgorithm(size,diag,subd,mat);
}
//---------------------------------------------------------------------------
void mgcEigenD::
EigenStuffN ()
{
        TridiagonalN(size,mat,diag,subd);
        QLAlgorithm(size,diag,subd,mat);
}
//---------------------------------------------------------------------------
void mgcEigenD::
EigenStuff ()
{
        switch ( size ) {
                case 2 : Tridiagonal2(mat,diag,subd);       break;
                case 3 : Tridiagonal3(mat,diag,subd);       break;
                case 4 : Tridiagonal4(mat,diag,subd);       break;
                default: TridiagonalN(size,mat,diag,subd);  break;
        }
        QLAlgorithm(size,diag,subd,mat);
}
//---------------------------------------------------------------------------
void mgcEigenD::
DecrSortEigenStuff2 ()
{
        Tridiagonal2(mat,diag,subd);
        QLAlgorithm(size,diag,subd,mat);
        DecreasingSort(size,diag,mat);
}
//---------------------------------------------------------------------------
void mgcEigenD::
DecrSortEigenStuff3 ()
{
        Tridiagonal3(mat,diag,subd);
        QLAlgorithm(size,diag,subd,mat);
        DecreasingSort(size,diag,mat);
}
//---------------------------------------------------------------------------
void mgcEigenD::
DecrSortEigenStuff4 ()
{
        Tridiagonal4(mat,diag,subd);
        QLAlgorithm(size,diag,subd,mat);
        DecreasingSort(size,diag,mat);
}
//---------------------------------------------------------------------------
void mgcEigenD::
DecrSortEigenStuffN ()
{
        TridiagonalN(size,mat,diag,subd);
        QLAlgorithm(size,diag,subd,mat);
        DecreasingSort(size,diag,mat);
}
//---------------------------------------------------------------------------
void mgcEigenD::
DecrSortEigenStuff ()
{
        switch ( size ) {
                case 2 : Tridiagonal2(mat,diag,subd);       break;
                case 3 : Tridiagonal3(mat,diag,subd);       break;
                case 4 : Tridiagonal4(mat,diag,subd);       break;
                default: TridiagonalN(size,mat,diag,subd);  break;
        }
        QLAlgorithm(size,diag,subd,mat);
        DecreasingSort(size,diag,mat);
}
//---------------------------------------------------------------------------
void mgcEigenD::
IncrSortEigenStuff2 ()
{
        Tridiagonal2(mat,diag,subd);
        QLAlgorithm(size,diag,subd,mat);
        IncreasingSort(size,diag,mat);
}
//---------------------------------------------------------------------------
void mgcEigenD::
IncrSortEigenStuff3 ()
{
        Tridiagonal3(mat,diag,subd);
        QLAlgorithm(size,diag,subd,mat);
        IncreasingSort(size,diag,mat);
}
//---------------------------------------------------------------------------
void mgcEigenD::
IncrSortEigenStuff4 ()
{
        Tridiagonal4(mat,diag,subd);
        QLAlgorithm(size,diag,subd,mat);
        IncreasingSort(size,diag,mat);
}
//---------------------------------------------------------------------------
void mgcEigenD::
IncrSortEigenStuffN ()
{
        TridiagonalN(size,mat,diag,subd);
        QLAlgorithm(size,diag,subd,mat);
        IncreasingSort(size,diag,mat);
}
//---------------------------------------------------------------------------
void mgcEigenD::
IncrSortEigenStuff ()
{
        switch ( size ) {
                case 2 : Tridiagonal2(mat,diag,subd);       break;
                case 3 : Tridiagonal3(mat,diag,subd);       break;
                case 4 : Tridiagonal4(mat,diag,subd);       break;
                default: TridiagonalN(size,mat,diag,subd);  break;
        }
        QLAlgorithm(size,diag,subd,mat);
        IncreasingSort(size,diag,mat);
}
//---------------------------------------------------------------------------
int mgcEigenD::
Number (unsigned single_error)
{
        int result;
        for (result = -1; single_error; single_error >>= 1)
                result++;
        return result;
}
//---------------------------------------------------------------------------
/*
void mgcEigenD::
Report (unsigned single_error)
{
        if ( mgcEigenD::verbose )
                cout << "mgcEigenD: " << message[Number(single_error)] << endl;
        else
                ofstream("eigen.err",ios::out|ios::app)
                         << "mgcEigenD: " << message[Number(single_error)] << endl;

        error |= single_error;
}
*/
//---------------------------------------------------------------------------
/*
void mgcEigenD::
Report (ostream& ostr)
{
        for (unsigned single_error = 1; single_error; single_error <<= 1)
                if ( error & single_error )
                        ostr << "mgcEigenD: " << message[Number(single_error)] << endl;

        error = 0;
}
*/

/*
void CovarianceEigenOrient (int n, const Vector * pt, Vector axes[3])
{
    int i;

    // compute mean of points
    double xsum = 0.0, ysum = 0.0, zsum = 0.0;
    for (i = 0; i < n; i++)
    {
        xsum += pt[i].x;
        ysum += pt[i].y;
        zsum += pt[i].z;
    }
    double xmean = xsum/n;
    double ymean = ysum/n;
    double zmean = zsum/n;

    // compute covariances of points
    double xxsum = 0.0, xysum = 0.0, xzsum = 0.0;
    double yysum = 0.0, yzsum = 0.0, zzsum = 0.0;
    for (i = 0; i < n; i++)
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
    double xxcov = xxsum/n;
    double xycov = xysum/n;
    double xzcov = xzsum/n;
    double yycov = yysum/n;
    double yzcov = yzsum/n;
    double zzcov = zzsum/n;

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
    Vector U((float)eig.Eigenvector(0,0), (float)eig.Eigenvector(1,0), (float)eig.Eigenvector(2,0));
    Vector V((float)eig.Eigenvector(0,1), (float)eig.Eigenvector(1,1), (float)eig.Eigenvector(2,1));
    Vector W((float)eig.Eigenvector(0,2), (float)eig.Eigenvector(1,2), (float)eig.Eigenvector(2,2));
    
    // Box axes are the eigenvectors of the covariance matrix with
    // adjusted lengths to enclose the points.  If U, V, and W are the
    // eigenvectors, C is the center of the box, and X is a point in
    // the input list, then X = C + a*U + b*V + c*W.  The box extent is
    // determined by max|a|, max|b|, and max|c|.  The box axes are then
    // defined to be (max|a|)*U and (max|b|)*V.  Note that since U and V
    // are unit length and orthogonal, a = Dot(U,X-C), b = Dot(V,X-C),
    // and c = Dot(W,X-C).
    double amax = 0.0, bmax = 0.0, cmax = 0.0;
    for (i = 0; i < n; i++)
    {
        double dx = pt[i].x - xmean;
        double dy = pt[i].y - ymean;
        double dz = pt[i].z - zmean;
        double absdot = fabs(U.x*dx+U.y*dy+U.z*dz);
        if ( absdot > amax )
            amax = absdot;
        absdot = fabs(V.x*dx+V.y*dy+V.z*dz);
        if ( absdot > bmax )
            bmax = absdot;
        absdot = fabs(W.x*dx+W.y*dy+W.z*dz);
        if ( absdot > cmax )
            cmax = absdot;
    }

    axes[0].x = (float)amax*U.x;
    axes[0].y = (float)amax*U.y;
    axes[0].z = (float)amax*U.z;
    axes[1].x = (float)bmax*V.x;
    axes[1].y = (float)bmax*V.y;
    axes[1].z = (float)bmax*V.z;
    axes[2].x = (float)cmax*W.x;
    axes[2].y = (float)cmax*W.y;
    axes[2].z = (float)cmax*W.z;
}
*/
//===========================================================================

#ifdef EIGEN_TEST

int main ()
{
        mgcEigenD eig(3);

        eig.Matrix(0,0) = 2;  eig.Matrix(0,1) = 1;  eig.Matrix(0,2) = 1;
        eig.Matrix(1,0) = 1;  eig.Matrix(1,1) = 2;  eig.Matrix(1,2) = 1;
        eig.Matrix(2,0) = 1;  eig.Matrix(2,1) = 1;  eig.Matrix(2,2) = 2;

        eig.IncrSortEigenStuff3();

        cout.setf(ios::fixed);

        cout << "eigenvalues = " << endl;
        for (int row = 0; row < 3; row++)
                cout << eig.Eigenvalue(row) << ' ';
        cout << endl;

        cout << "eigenvectors = " << endl;
        for (row = 0; row < 3; row++) {
                for (int col = 0; col < 3; col++)
                        cout << eig.Eigenvector(row,col) << ' ';
                cout << endl;
        }

        // eigenvalues =
        //    1.000000 1.000000 4.000000
        // eigenvectors =
        //    0.411953  0.704955 0.577350
        //    0.404533 -0.709239 0.577350
        //   -0.816485  0.004284 0.577350

        return 0;
}

#endif
