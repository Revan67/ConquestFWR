//
// <chull3d.h> - 3D convex hull generation
//

#ifndef CONHULL3_H
#define CONHULL3_H

#include "main.h"

typedef int CTriangle[3];

int ConvexHull3D (int N, SINGLE* x, SINGLE* y, SINGLE* z, 
                                  int& numTriangles, CTriangle* triangle);

#endif

