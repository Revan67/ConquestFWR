
#include <fstream.h>
#include <math.h>
#include "eigen.h"


//---------------------------------------------------------------------------
EigenSolve::EigenSolve (int _size)
{
    // assert:  _size > 1
    size = _size;

    mat = new float*[size];
    for (int d = 0; d < size; d++)
        mat[d] = new float[size];

    diag = new float[size];
    subd = new float[size];
}
//---------------------------------------------------------------------------
EigenSolve::~EigenSolve ()
{
    delete[] subd;
    delete[] diag;
    for (int d = 0; d < size; d++)
        delete[] mat[d];
    delete[] mat;
}
//---------------------------------------------------------------------------
void EigenSolve::Tridiagonal2 (float** mat, float* diag, float* subd)
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
void EigenSolve::Tridiagonal3 (float** mat, float* diag, float* subd)
{
    float a = mat[0][0], b = mat[0][1], c = mat[0][2],
                         d = mat[1][1], e = mat[1][2],
                                        f = mat[2][2];

    diag[0] = a;
    subd[2] = 0;
    if ( c != 0 )
    {
        float ell = float(sqrt(b*b+c*c));
        float invell = 1.0f/ell;
        b *= invell;
        c *= invell;
        float q = 2*b*e+c*(f-d);
        diag[1] = d+c*q;
        diag[2] = f-c*q;
        subd[0] = ell;
        subd[1] = e-b*q;
        mat[0][0] = 1; mat[0][1] = 0; mat[0][2] = 0;
        mat[1][0] = 0; mat[1][1] = b; mat[1][2] = c;
        mat[2][0] = 0; mat[2][1] = c; mat[2][2] = -b;
    }
    else
    {
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
void EigenSolve::Tridiagonal4 (float** mat, float* diag, float* subd)
{
    // save matrix M
    float a = mat[0][0], b = mat[0][1], c = mat[0][2], d = mat[0][3],
                         e = mat[1][1], f = mat[1][2], g = mat[1][3],
                                        h = mat[2][2], i = mat[2][3],
                                                       j = mat[3][3];

    diag[0] = a;
    subd[3] = 0;

    mat[0][0] = 1; mat[0][1] = 0; mat[0][2] = 0; mat[0][3] = 0;
    mat[1][0] = 0;
    mat[2][0] = 0;
    mat[3][0] = 0;

    if ( c != 0 || d != 0 )
    {
        float q11, q12, q13;
        float q21, q22, q23;
        float q31, q32, q33;

        // build column Q1
        float len = float(sqrt(b*b+c*c+d*d));
        float invlen = 1.0f/len;
        q11 = b*invlen;
        q21 = c*invlen;
        q31 = d*invlen;

        subd[0] = len;

        // compute S*Q1
        float v0 = e*q11+f*q21+g*q31;
        float v1 = f*q11+h*q21+i*q31;
        float v2 = g*q11+i*q21+j*q31;

        diag[1] = q11*v0+q21*v1+q31*v2;

        // build column Q3 = Q1x(S*Q1)
        q13 = q21*v2-q31*v1;
        q23 = q31*v0-q11*v2;
        q33 = q11*v1-q21*v0;
        len = float(sqrt(q13*q13+q23*q23+q33*q33));
        if ( len > 0 )
        {
            invlen = 1.0f/len;
            q13 *= invlen;
            q23 *= invlen;
            q33 *= invlen;

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
        else
        {
            // S*Q1 parallel to Q1, choose any valid Q2 and Q3
            subd[1] = 0;

            len = q21*q21+q31*q31;
            if ( len > 0 )
            {
                invlen = 1.0f/len;
                float tmp = q11-1;
                q12 = -q21;
                q22 = 1+tmp*q21*q21*invlen;
                q32 = tmp*q21*q31*invlen;

                q13 = -q31;
                q23 = q32;
                q33 = 1+tmp*q31*q31*invlen;

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
            else
            {
                // Q1 = (+-1,0,0)
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
    else
    {
        diag[1] = e;
        subd[0] = b;
        mat[1][1] = 1;
        mat[2][1] = 0;
        mat[3][1] = 0; 

        if ( g != 0 )
        {
            float ell = float(sqrt(f*f+g*g));
            float invell = 1.0f/ell;
            f *= invell;
            g *= invell;
            float Q = 2*f*i+g*(j-h);

            diag[2] = h+g*Q;
            diag[3] = j-g*Q;
            subd[1] = ell;
            subd[2] = i-f*Q;
            mat[1][2] = 0;  mat[1][3] = 0;
            mat[2][2] = f;  mat[2][3] = g;
            mat[3][2] = g;  mat[3][3] = -f;
        }
        else
        {
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
void EigenSolve::TridiagonalN (int n, float** mat, float* diag, float* subd)
{
    int i, j, k, ell;

    for (i = n-1, ell = n-2; i >= 1; i--, ell--)
    {
        float h = 0, scale = 0;

        if ( ell > 0 )
        {
            for (k = 0; k <= ell; k++)
                scale += float(fabs(mat[i][k]));
            if ( scale == 0 )
            {
                subd[i] = mat[i][ell];
            }
            else
            {
                float invscale = 1.0f/scale;
                for (k = 0; k <= ell; k++)
                {
                    mat[i][k] *= invscale;
                    h += mat[i][k]*mat[i][k];
                }
                float f = mat[i][ell];
                float g = ( f > 0 ? -float(sqrt(h)) : float(sqrt(h)) );
                subd[i] = scale*g;
                h -= f*g;
                mat[i][ell] = f-g;
                f = 0;
                float invh = 1.0f/h;
                for (j = 0; j <= ell; j++)
                {
                    mat[j][i] = mat[i][j]*invh;
                    g = 0;
                    for (k = 0; k <= j; k++)
                        g += mat[j][k]*mat[i][k];
                    for (k = j+1; k <= ell; k++)
                        g += mat[k][j]*mat[i][k];
                    subd[j] = g*invh;
                    f += subd[j]*mat[i][j];
                }
                float hh = 0.5f*f*invh;
                for (j = 0; j <= ell; j++)
                {
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
    for (i = 0, ell = -1; i <= n-1; i++, ell++)
    {
        if ( diag[i] )
        {
            for (j = 0; j <= ell; j++)
            {
                float sum = 0;
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

    // re-ordering if EigenSolve::QLAlgorithm is used subsequently
    for (i = 1, ell = 0; i < n; i++, ell++)
        subd[ell] = subd[i];
    subd[n-1] = 0;
}
//---------------------------------------------------------------------------
void EigenSolve::QLAlgorithm (int n, float* diag, float* subd, float** mat)
{
    const int eigen_maxiter = 30;

    for (int ell = 0; ell < n; ell++)
    {
        int iter;
        for (iter = 0; iter < eigen_maxiter; iter++)
        {
            int m;
            for (m = ell; m <= n-2; m++)
            {
                float dd = float(fabs(diag[m])+fabs(diag[m+1]));
                if ( (float)(fabs(subd[m])+dd) == dd )
                    break;
            }
            if ( m == ell )
                break;

            float g = (diag[ell+1]-diag[ell])/(2*subd[ell]);
            float r = float(sqrt(g*g+1));
            if ( g < 0 )
                g = diag[m]-diag[ell]+subd[ell]/(g-r);
            else
                g = diag[m]-diag[ell]+subd[ell]/(g+r);
            float s = 1, c = 1, p = 0;
            for (int i = m-1; i >= ell; i--)
            {
                float f = s*subd[i], b = c*subd[i];
                if ( fabs(f) >= fabs(g) ) {
                    c = g/f;
                    r = float(sqrt(c*c+1));
                    subd[i+1] = f*r;
                    c *= (s = 1/r);
                }
                else {
                    s = f/g;
                    r = float(sqrt(s*s+1));
                    subd[i+1] = g*r;
                    s *= (c = 1/r);
                }
                g = diag[i+1]-p;
                r = (diag[i]-g)*s+2*b*c;
                p = s*r;
                diag[i+1] = g+p;
                g = c*r-b;

                for (int k = 0; k < n; k++)
                {
                    f = mat[k][i+1];
                    mat[k][i+1] = s*mat[k][i]+c*f;
                    mat[k][i] = c*mat[k][i]-s*f;
                }
            }
            diag[ell] -= p;
            subd[ell] = g;
            subd[m] = 0;
        }
        if ( iter == eigen_maxiter )
            return;
    }
}
//---------------------------------------------------------------------------
void EigenSolve::DecreasingSort (int n, float* eigval, float** eigvec)
{
    // sort eigenvalues in decreasing order, e[0] >= ... >= e[n-1]
    for (int i = 0, k; i <= n-2; i++)
    {
        // locate maximum eigenvalue
        float max = eigval[k=i];
        int j;
        for (j = i+1; j < n; j++)
            if ( eigval[j] > max )
                max = eigval[k=j];

        if ( k != i )
        {
            // swap eigenvalues
            eigval[k] = eigval[i];
            eigval[i] = max;

            // swap eigenvectors
            for (j = 0; j < n; j++)
            {
                float tmp = eigvec[j][i];
                eigvec[j][i] = eigvec[j][k];
                eigvec[j][k] = tmp;
            }
        }
    }
}
//---------------------------------------------------------------------------
void EigenSolve::IncreasingSort (int n, float* eigval, float** eigvec)
{
    // sort eigenvalues in increasing order, e[0] <= ... <= e[n-1]
    for (int i = 0, k; i <= n-2; i++)
    {
        // locate minimum eigenvalue
        float min = eigval[k=i];
        int j;
        for (j = i+1; j < n; j++)
            if ( eigval[j] < min )
                min = eigval[k=j];

        if ( k != i )
        {
            // swap eigenvalues
            eigval[k] = eigval[i];
            eigval[i] = min;

            // swap eigenvectors
            for (j = 0; j < n; j++)
            {
                float tmp = eigvec[j][i];
                eigvec[j][i] = eigvec[j][k];
                eigvec[j][k] = tmp;
            }
        }
    }
}
//---------------------------------------------------------------------------
EigenSolve& EigenSolve::Matrix (float** inmat)
{
    for (int row = 0; row < size; row++)
        for (int col = 0; col < size; col++)
            mat[row][col] = inmat[row][col];
    return *this;
}
//---------------------------------------------------------------------------
void EigenSolve::EigenStuff2 ()
{
    Tridiagonal2(mat,diag,subd);
    QLAlgorithm(size,diag,subd,mat);
}
//---------------------------------------------------------------------------
void EigenSolve::EigenStuff3 ()
{
    Tridiagonal3(mat,diag,subd);
    QLAlgorithm(size,diag,subd,mat);
}
//---------------------------------------------------------------------------
void EigenSolve::EigenStuff4 ()
{
    Tridiagonal4(mat,diag,subd);
    QLAlgorithm(size,diag,subd,mat);
}
//---------------------------------------------------------------------------
void EigenSolve::EigenStuffN ()
{
    TridiagonalN(size,mat,diag,subd);
    QLAlgorithm(size,diag,subd,mat);
}
//---------------------------------------------------------------------------
void EigenSolve::EigenStuff ()
{
    switch ( size )
    {
        case 2 : Tridiagonal2(mat,diag,subd);       break;
        case 3 : Tridiagonal3(mat,diag,subd);       break;
        case 4 : Tridiagonal4(mat,diag,subd);       break;
        default: TridiagonalN(size,mat,diag,subd);  break;
    }
    QLAlgorithm(size,diag,subd,mat);
}
//---------------------------------------------------------------------------
void EigenSolve::DecrSortEigenStuff2 ()
{
    Tridiagonal2(mat,diag,subd);
    QLAlgorithm(size,diag,subd,mat);
    DecreasingSort(size,diag,mat);
}
//---------------------------------------------------------------------------
void EigenSolve::DecrSortEigenStuff3 ()
{
    Tridiagonal3(mat,diag,subd);
    QLAlgorithm(size,diag,subd,mat);
    DecreasingSort(size,diag,mat);
}
//---------------------------------------------------------------------------
void EigenSolve::DecrSortEigenStuff4 ()
{
    Tridiagonal4(mat,diag,subd);
    QLAlgorithm(size,diag,subd,mat);
    DecreasingSort(size,diag,mat);
}
//---------------------------------------------------------------------------
void EigenSolve::DecrSortEigenStuffN ()
{
    TridiagonalN(size,mat,diag,subd);
    QLAlgorithm(size,diag,subd,mat);
    DecreasingSort(size,diag,mat);
}
//---------------------------------------------------------------------------
void EigenSolve::DecrSortEigenStuff ()
{
    switch ( size )
    {
        case 2 : Tridiagonal2(mat,diag,subd);       break;
        case 3 : Tridiagonal3(mat,diag,subd);       break;
        case 4 : Tridiagonal4(mat,diag,subd);       break;
        default: TridiagonalN(size,mat,diag,subd);  break;
    }
    QLAlgorithm(size,diag,subd,mat);
    DecreasingSort(size,diag,mat);
}
//---------------------------------------------------------------------------
void EigenSolve::IncrSortEigenStuff2 ()
{
    Tridiagonal2(mat,diag,subd);
    QLAlgorithm(size,diag,subd,mat);
    IncreasingSort(size,diag,mat);
}
//---------------------------------------------------------------------------
void EigenSolve::IncrSortEigenStuff3 ()
{
    Tridiagonal3(mat,diag,subd);
    QLAlgorithm(size,diag,subd,mat);
    IncreasingSort(size,diag,mat);
}
//---------------------------------------------------------------------------
void EigenSolve::IncrSortEigenStuff4 ()
{
    Tridiagonal4(mat,diag,subd);
    QLAlgorithm(size,diag,subd,mat);
    IncreasingSort(size,diag,mat);
}
//---------------------------------------------------------------------------
void EigenSolve::IncrSortEigenStuffN ()
{
    TridiagonalN(size,mat,diag,subd);
    QLAlgorithm(size,diag,subd,mat);
    IncreasingSort(size,diag,mat);
}
//---------------------------------------------------------------------------
void EigenSolve::IncrSortEigenStuff ()
{
    switch ( size )
    {
        case 2 : Tridiagonal2(mat,diag,subd);       break;
        case 3 : Tridiagonal3(mat,diag,subd);       break;
        case 4 : Tridiagonal4(mat,diag,subd);       break;
        default: TridiagonalN(size,mat,diag,subd);  break;
    }
    QLAlgorithm(size,diag,subd,mat);
    IncreasingSort(size,diag,mat);
}
