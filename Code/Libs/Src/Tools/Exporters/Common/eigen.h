#ifndef EIGEN_H
#define EIGEN_H

//#include <iostream.h>
#include "vector.h"

//void CovarianceEigenOrient (int n, const Vector * pt, Vector axes[3]);

class mgcEigenD
{
public:
        mgcEigenD (int _size);
        ~mgcEigenD ();

    // set the matrix for eigensolving
        double& Matrix (int row, int col) { return mat[row][col]; }
        mgcEigenD& Matrix (double** inmat);

        // get the results of eigensolving
        double Eigenvalue (int d) { return diag[d]; }
        double Eigenvector (int row, int col) { return mat[row][col]; }
        const double* Eigenvalue () { return diag; }
        const double** Eigenvector () { return (const double**) mat; }

        // solve eigensystem
        void EigenStuff2 ();  // uses TriDiagonal2
        void EigenStuff3 ();  // uses TriDiagonal3
        void EigenStuff4 ();  // uses TriDiagonal4
        void EigenStuffN ();  // uses TriDiagonalN
        void EigenStuff  ();  // uses switch statement

        // solve eigensystem, use decreasing sort on eigenvalues
        void DecrSortEigenStuff2 ();
        void DecrSortEigenStuff3 ();
        void DecrSortEigenStuff4 ();
        void DecrSortEigenStuffN ();
        void DecrSortEigenStuff  ();

        // solve eigensystem, use increasing sort on eigenvalues
        void IncrSortEigenStuff2 ();
        void IncrSortEigenStuff3 ();
        void IncrSortEigenStuff4 ();
        void IncrSortEigenStuffN ();
        void IncrSortEigenStuff  ();

        // debugging output?
        double& Tdiag (int i) { return diag[i]; }
        double& Tsubdiag (int i) { return subd[i]; }
        void Reduce () { TridiagonalN(size,mat,diag,subd); }

private:
        int size;
        double** mat;
        double* diag;
        double* subd;

        // Householder reduction to tridiagonal form
        void Tridiagonal2 (double** mat, double* diag, double* subd);
        void Tridiagonal3 (double** mat, double* diag, double* subd);
        void Tridiagonal4 (double** mat, double* diag, double* subd);
        void TridiagonalN (int n, double** mat, double* diag, double* subd);

        // QL algorithm with implicit shifting, applies to tridiagonal matrices
        void QLAlgorithm (int n, double* diag, double* subd, double** mat);

        // sort eigenvalues from largest to smallest
        void DecreasingSort (int n, double* eigval, double** eigvec);

        // sort eigenvalues from smallest to largest
        void IncreasingSort (int n, double* eigval, double** eigvec);

// error handling
public:
        static int verbose;
        static unsigned error;
        //static void Report (void/*ostream& ostr*/);
private:
        static const unsigned invalid_size;
        static const unsigned allocation_failed;
        static const unsigned ql_exceeded;
        static const char* message[3];
        static int Number (unsigned single_error);
        //static void Report (unsigned single_error);
};

#endif
