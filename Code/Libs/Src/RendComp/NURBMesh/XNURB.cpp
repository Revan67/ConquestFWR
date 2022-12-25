#include "xnurb.h"
#include "ITextureLibrary.h"

#pragma warning( push, 4 )

void XNURB::compute_bounds (void)
{
	bounds[0] = bounds[1] =
	bounds[2] = bounds[3] =
	bounds[4] = bounds[5] = 0.0f;

	if(patch_cnt > 0)
	{
		bounds[0] = bounds[1] = patch_list[0].point_list[0].x;
		bounds[2] = bounds[3] = patch_list[0].point_list[0].y;
		bounds[4] = bounds[5] = patch_list[0].point_list[0].z;

		for(NURBPatch *patch = patch_list; patch < patch_list + patch_cnt; patch++)
		{
			for(Vector *v = patch->point_list; 
				v < patch->point_list + patch->s_point_cnt * patch->t_point_cnt;
				v++)
			{
				if (v->x > bounds[0])
					bounds[0] = v->x;
				else
				if (v->x < bounds[1])
					bounds[1] = v->x;

				if (v->y > bounds[2])
					bounds[2] = v->y;
				else
				if (v->y < bounds[3])
					bounds[3] = v->y;

				if (v->z > bounds[4])
					bounds[4] = v->z;
				else
				if (v->z < bounds[5])
					bounds[5] = v->z;
			}
		}
	}	
}

//

void XNURB::get_bounding_box (SINGLE box[6]) const
{
	box[0] = bounds[0];
	box[1] = bounds[1];
	box[2] = bounds[2];
	box[3] = bounds[3];
	box[4] = bounds[4];
	box[5] = bounds[5];
}

/*
bool XNURB::expand_bounding_box (float box[6]) const
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

*/
//

void XNURB::compute_centroid (void)
{
	centroid = Vector(0, 0, 0);
	
	int sum = 0;
	for(NURBPatch *patch = patch_list; patch < patch_list + patch_cnt; patch++)
	{
		for(Vector *v = patch->point_list; 
			v < patch->point_list + patch->s_point_cnt * patch->t_point_cnt;
			v++)
		{
				centroid += *v;
		}

		sum += patch->s_point_cnt * patch->t_point_cnt;
	}

	centroid /= (float)sum;
}

//

void XNURB::copy_nurb(const XNURB & src)
{
	patch_cnt = src.patch_cnt;
	patch_list = new NURBPatch[patch_cnt];

	material_cnt = src.material_cnt;
	material_list = new Material[material_cnt];
	for (int i = 0; i < material_cnt; i++)
	{
		material_list[i].copy_from( src.material_list + i );
	}

	for(i = 0; i < patch_cnt; i++)
	{
		const NURBPatch & sp = src.patch_list[i];
		NURBPatch & dp = patch_list[i];

		dp.s_order = sp.s_order;
		dp.t_order = sp.t_order;

		if(sp.s_knot_cnt > 0 && sp.t_knot_cnt > 0)
		{
			dp.s_knot_cnt = sp.s_knot_cnt;
			dp.s_knot_list = new float[dp.s_knot_cnt];
			memcpy(dp.s_knot_list, sp.s_knot_list, dp.s_knot_cnt * sizeof(float));

			dp.t_knot_cnt = sp.t_knot_cnt;
			dp.t_knot_list = new float[dp.t_knot_cnt];
			memcpy(dp.t_knot_list, sp.t_knot_list, dp.t_knot_cnt * sizeof(float));
		}

		dp.s_point_cnt = sp.s_point_cnt;
		dp.t_point_cnt = sp.t_point_cnt;
		dp.point_list = new Vector[dp.s_point_cnt * dp.t_point_cnt];
		memcpy(dp.point_list, sp.point_list, dp.s_point_cnt * dp.t_point_cnt * sizeof(Vector));

		if(sp.weight_list)
		{
			dp.weight_list = new float[dp.s_point_cnt * dp.t_point_cnt];
			memcpy(dp.weight_list, sp.weight_list, dp.s_point_cnt * dp.t_point_cnt * sizeof(float));
		}

		/* done by GenerateNormalsUVs()
		dp.s_vertex_cnt = sp.s_vertex_cnt;
		dp.t_vertex_cnt = sp.t_vertex_cnt;
		dp.normals = new Vector[dp.s_vertex_cnt * dp.t_vertex_cnt];
		memcpy(dp.normals, sp.normals, dp.s_vertex_cnt * dp.t_vertex_cnt * sizeof(Vector));
		dp.vertices = new Vector[dp.s_vertex_cnt * dp.t_vertex_cnt];
		memcpy(dp.vertices, sp.vertices, dp.s_vertex_cnt * dp.t_vertex_cnt * sizeof(Vector));
		dp.D_coefficient = new float[dp.s_vertex_cnt * dp.t_vertex_cnt];
		memcpy(dp.D_coefficient, sp.D_coefficient, dp.s_vertex_cnt * dp.t_vertex_cnt * sizeof(float));
		*/

		dp.mtl_id = sp.mtl_id;

		dp.u_cnt = sp.u_cnt;
		dp.v_cnt = sp.v_cnt;
		dp.uv_list = new Vector2[dp.u_cnt * dp.v_cnt];
		memcpy(dp.uv_list, sp.uv_list, dp.u_cnt * dp.v_cnt * sizeof(Vector2));

		// not copied so that archetype is unique even as far as IIGLOO is concerned
#if 0
		dp.s_basis_cnt = sp.s_basis_cnt;
		dp.s_basis_list = new IIBasis * [dp.s_basis_cnt];
		memcpy(dp.s_basis_list, sp.s_basis_list, dp.s_basis_cnt * sizeof(IIBasis*));
		for(i = 0; i < dp.s_basis_cnt; i++)
		{
			dp.s_basis_list[i]->AddRef();
		}
		dp.t_basis_cnt = sp.t_basis_cnt;
		dp.t_basis_list = new IIBasis * [dp.t_basis_cnt];
		memcpy(dp.t_basis_list, sp.t_basis_list, dp.t_basis_cnt * sizeof(IIBasis*));
		for(i = 0; i < dp.t_basis_cnt; i++)
		{
			dp.t_basis_list[i]->AddRef();
		}
#endif

		// done by AssignCacheHandles()
		//iicache_hangle_list
	}

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

	non_rational_count = src.non_rational_count;
	rational_count = src.rational_count;
}

#pragma warning( pop )