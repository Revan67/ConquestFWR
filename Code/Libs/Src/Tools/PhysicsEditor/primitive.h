//
// <primitive.h> - Primitive rendering functions
//

#ifndef PRIMITIVE_H
#define PRIMITIVE_H

#include "extent.h"

void draw_sphere(SphereExtent * sphere, const Vector & center_of_mass, const Matrix & R);
void draw_tube(TubeExtent * tube, const Vector & center_of_mass, const Matrix & R);
void draw_box(BoxExtent * box, const Vector & center_of_mass, const Matrix & R);
void draw_mesh(ConvexMeshExtent * mesh, const Vector & center_of_mass, const Matrix & R);

void render_extent(BaseExtent * extent, const Vector & com, const Matrix & R);

#endif