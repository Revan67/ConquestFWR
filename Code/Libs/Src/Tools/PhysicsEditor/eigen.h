#ifndef EIGEN_H
#define EIGEN_H

class EigenSolve
{
public:
    EigenSolve (int _size);
    ~EigenSolve ();

    // set the matrix for eigensolving
    float& Matrix (int row, int col) { return mat[row][col]; }
    EigenSolve& Matrix (float** inmat);

    // get the results of eigensolving
    float Eigenvalue (int d) { return diag[d]; }
    float Eigenvector (int row, int col) { return mat[row][col]; }
    const float* Eigenvalue () { return diag; }
    const float** Eigenvector () { return (const float**) mat; }

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

private:
    int size;
    float** mat;
    float* diag;
    float* subd;

    // Householder reduction to tridiagonal form
    void Tridiagonal2 (float** mat, float* diag, float* subd);
    void Tridiagonal3 (float** mat, float* diag, float* subd);
    void Tridiagonal4 (float** mat, float* diag, float* subd);
    void TridiagonalN (int n, float** mat, float* diag, float* subd);

    // QL algorithm with implicit shifting, applies to tridiagonal matrices
    void QLAlgorithm (int n, float* diag, float* subd, float** mat);

    // sort eigenvalues from largest to smallest
    void DecreasingSort (int n, float* eigval, float** eigvec);

    // sort eigenvalues from smallest to largest
    void IncreasingSort (int n, float* eigval, float** eigvec);
};


#endif