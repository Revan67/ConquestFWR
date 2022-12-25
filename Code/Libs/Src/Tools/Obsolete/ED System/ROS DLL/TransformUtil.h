// Author: Shaival Varma
// --------------------------------------------------------------------------
#ifndef TransformUtil_h
#define TransformUtil_h
// --------------------------------------------------------------------------
#include "Location.h"
#include "Orientation.h"
#include "xform.h"
// --------------------------------------------------------------------------
typedef double GLdouble;
// --------------------------------------------------------------------------
inline void GetGLMatrix(const Transform& transform, GLdouble glMatrix[4][4])
{
    for(int i = 0; i < 3; ++i)
    {	for(int j = 0; j < 3; ++j)
    	{	glMatrix[i][j] = transform.d[j][i];
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
// --------------------------------------------------------------------------
inline void GetTransform(const GLdouble glMatrix[4][4], Transform& transform)
{
    for(int i = 0; i < 3; ++i)
    {	for(int j = 0; j < 3; ++j)
    	{	transform.d[i][j] = glMatrix[j][i];
        }
    }

    transform.d[0][3] =
    transform.d[1][3] =
    transform.d[2][3] = 0.0;
}
// --------------------------------------------------------------------------
#endif