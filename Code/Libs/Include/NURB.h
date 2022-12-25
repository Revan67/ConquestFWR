// NURB.h by -ms
#ifndef NURB_H
#define NURB_H

#include "Material.h"
#include "vector.h"

// NOTES: 
// patch_cnt = (point_cnt - order + 1)
// point_cnt = knot_cnt - order

struct Vector2
{
	float u, v;
};

struct NURBPatch
{
	int			s_order;		// ex, order 4 = degree 3 = cubic
	int			t_order;
	
	int			s_knot_cnt;
	int			t_knot_cnt;
	float *		s_knot_list;
	float *		t_knot_list;

	int			s_point_cnt;
	int			t_point_cnt;
	Vector *	point_list;		// s_point_cnt * t_point_cnt
	float *		weight_list;

	// vertices used for lighting/culling (computed once at load time)
	int			s_vertex_cnt;	// (s_basis_cnt * (s_order - 1) + 1)
	int			t_vertex_cnt;	// (t_basis_cnt * (t_order - 1) + 1)
	Vector *	normals;		
	Vector *	vertices;
	float *		D_coefficient;

	int			mtl_id;

	int			u_cnt;
	int			v_cnt;
	Vector2		*uv_list;		// texture coordinates

	int s_basis_cnt;				// # of sub patches in s
	int t_basis_cnt;				// # of sub patches in t

	struct IIBasis ** s_basis_list; // computed from knots
	struct IIBasis ** t_basis_list;

	struct IIPolynomial **x_polynom;// s_basis_cnt * t_basis_cnt	
	struct IIPolynomial **y_polynom;
	struct IIPolynomial **z_polynom;
	struct IIPolynomial **w_polynom;

	struct IIPolynomial **u_polynom;
	struct IIPolynomial **v_polynom;

	NURBPatch(void)
	{
		memset(this, 0, sizeof(*this));
	}
};

// technically each NURBPatch is a NURB on it's own, so our definition of a NURB is
// really a NURB Set 
struct NURB 
{
	int				patch_cnt;
	NURBPatch *		patch_list;

	int				material_cnt; 
	Material *		material_list;

	Vector			sphere_center;	// bounding sphere center
	SINGLE			radius;

	SINGLE			bounds[6];		// max_x, min_x, max_y, min_y, max_z, min_z in object space
	Vector			centroid;		// average of all controll points

	int				last_face_cnt;
	int				last_vertex_cnt;

	int				rational_count;
	int				non_rational_count;
};

#endif