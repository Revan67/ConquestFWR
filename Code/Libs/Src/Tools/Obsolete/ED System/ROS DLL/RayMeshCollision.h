#ifndef RayMeshCollision_h
#define RayMeshCollision_h

#include "Typedefs.h"
#include "vector.h"
#include "matrix.h"
#include "Mesh.h"
#include "Geom.h"

BOOL32 collide_ray_with_mesh(const Vector& ray_start, const Vector& ray_direction, const Vector& position, const Matrix& orientation, const Mesh* mesh, Plane* plane, S32* mesh_poly_index, SINGLE* distance);

#endif