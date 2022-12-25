//
//
//

#ifndef CHULL_H
#define CHULL_H

#include "view2d.h"

//
// Returns number of points on convex hull. Can be zero of dst_len isn't long enough to
// hold the points.
//
int ComputeConvexHull(ViewPoint * dst, int dst_len, const ViewPoint * src, int src_len);

void FreeConvexHullScratchBuffer (void);

//

#endif