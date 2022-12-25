#include "bezier.h"
#include "ITXMLib.h"

#pragma warning( push, 4 )

void BezierMesh::compute_bounds (void)
{
	bounds[0] = bounds[1] =
	bounds[2] = bounds[3] =
	bounds[4] = bounds[5] = 0.0f;

#pragma message("TODO: implement compute_bounds() " __FILE__  )
}

//

void BezierMesh::get_bounding_box(Vector * verts) const
{
	verts[0].set(bounds[0], bounds[2], bounds[4]);
	verts[1].set(bounds[1], bounds[2], bounds[4]);
	verts[2].set(bounds[0], bounds[3], bounds[4]);
	verts[3].set(bounds[1], bounds[3], bounds[4]);
	verts[4].set(bounds[0], bounds[2], bounds[5]);
	verts[5].set(bounds[1], bounds[2], bounds[5]);
	verts[6].set(bounds[0], bounds[3], bounds[5]);
	verts[7].set(bounds[1], bounds[3], bounds[5]);
}

//

bool BezierMesh::expand_bounding_box (float* box) const
{
	bool result = false;

	if (patch_cnt > 0)
	{
		if (bounds[1] > box[0])
			box[0] = bounds[1];

		if (bounds[0] < box[1])
			box[1] = bounds[0];

		if (bounds[3] > box[2])
			box[2] = bounds[3];

		if (bounds[2] < box[3])
			box[3] = bounds[2];

		if (bounds[5] > box[4])
			box[4] = bounds[5];

		if (bounds[4] < box[5])
			box[5] = bounds[4];

		result = true;
	}

	return result;
}

//

void BezierMesh::compute_centroid (void)
{
	centroid = Vector(0, 0, 0);
	
#pragma message("TODO: implement compute_centroid() " __FILE__  )
}

//

void BezierMesh::copy_bezier(const BezierMesh & src)
{
#pragma message("TODO: implement copy_nurb() " __FILE__  )

	sphere_center = src.sphere_center;
	radius = src.radius;

	bounds[0] = src.bounds[0];
	bounds[1] = src.bounds[1];
	bounds[2] = src.bounds[2];
	bounds[3] = src.bounds[3];
	bounds[4] = src.bounds[4];
	bounds[5] = src.bounds[5];

	centroid = src.centroid;

	last_face_cnt = 0;
	last_vertex_cnt = 0;
}

#pragma warning( pop )