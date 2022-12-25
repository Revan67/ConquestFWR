//
//
//

#include "extent.h"

//

void ComputeBox(Box & box, const Vector * v, int n, int stride = sizeof(Vector))
{
}

//

void ComputeBox(Box & box, const CollisionMesh & mesh)
{
	ComputeBox(box, (const Vector *) mesh.vertices, mesh.num_vertices, sizeof(Vertex));
}

//

