//
// <mingeom.h> - minimum bounding spheres, boxes, etc
//
// most algorithms are from dave eberly's graphics toolkit
// (http://www.cs.unc.edu/~eberly/gr_cont.htm)
//

#ifndef MINGEOM_H
#define MINGEOM_H

#include "main.h"
#include "object.h"

// return geometry needs to be shifted by -COM on return
void compute_optimal_sphere(SphereExtent * sphere, S32 vcount, Vector * vlist);         
void compute_optimal_box(BoxExtent * sphere, S32 vcount, Vector * vlist);            
void compute_optimal_tube(TubeExtent * tube, S32 vcount, Vector * vlist);

#endif