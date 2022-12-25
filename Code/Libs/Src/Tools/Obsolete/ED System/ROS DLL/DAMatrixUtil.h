#ifndef MatrixUtil_h
#define MatrixUtil_h

#include <Math.h>
#include <windows.h>

typedef float	GLfloat;

inline Matrix MatrixMultiply(const Matrix& m1, const Matrix& m2)
{
    Matrix	result;

    for(int i = 0; i < 3; ++i)
    {	for(int j = 0; j < 3; ++j)
        {	result.d[i][j] =	m1.d[i][0] * m2.d[0][j] +
                                m1.d[i][1] * m2.d[1][j] +
                                m1.d[i][2] * m2.d[2][j];
        }
    }

    return result;
}

inline float DegreeToRadian(float angle)
{
	const float degreeToRadian = 3.1416 / 180.0;

	return angle * degreeToRadian;
}

inline Matrix GetRotationMatrix(const Vector& vec, float angle)
{
	float x = vec.x;
    float y = vec.y;
    float z = vec.z;

    float s = sin(DegreeToRadian(angle));
    float c = cos(DegreeToRadian(angle));
    float t = 1.0 - c;

    Matrix	result;

    // | tx^2 + c   txy + sz  txz - sy |
    // | txy - sz   ty^2 + c  tyz + sx |
    // | txz + sy   tyz - sx  tz^2 + c |
    result.d[0][0] = t * x * x + c;		result.d[0][1] = t * x * y + s * z;	result.d[0][2] = t * x * z - s * y;
    result.d[1][0] = t * x * y - s * z;	result.d[1][1] = t * y * y + c;		result.d[1][2] = t * y * z + s * x;
    result.d[2][0] = t * x * z + s * y;	result.d[2][1] = t * y * z - s * x;	result.d[2][2] = t * z * z + c;

	return result;
}

inline void GetGLMatrix(const Matrix& matrix, GLfloat glMatrix[4][4])
{
    Matrix tMatrix = matrix.get_transpose();

    for(int i = 0; i < 3; ++i)
    {	for(int j = 0; j < 3; ++j)
    	{	glMatrix[i][j] = tMatrix.d[i][j];
        }
    }

    glMatrix[3][0] =
    glMatrix[3][1] =
    glMatrix[3][2] = 0.0;

    glMatrix[0][3] =
    glMatrix[1][3] =
    glMatrix[2][3] = 0.0;

    glMatrix[3][3] = 1.0;
}
#endif
