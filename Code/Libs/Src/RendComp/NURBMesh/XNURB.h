#ifndef __XNURB_H
#define __XNURB_H

#include <memory.h>
#include "iigloo\iigloo.h"
#include "NURB.h"

#define NUM_ORDERS 2 // number of polynomial orders supported
#define LOWEST_ORDER 3 // lowest polynomial order supported
#define HIGHEST_ORDER (LOWEST_ORDER + NUM_ORDERS - 1)

struct XNURB : public NURB
{
	XNURB(void)
	{
		memset(this, 0, sizeof(*this));
	}

	~XNURB()
	{
		for(int i=0; i < patch_cnt; i++)
		{
			NURBPatch & patch = patch_list[i];

			for(int j=0; j < patch.s_basis_cnt; j++)
			{
				patch.s_basis_list[j]->Release();
			}
			delete [] patch.s_basis_list;

			for(j=0; j < patch.t_basis_cnt; j++)
			{
				patch.t_basis_list[j]->Release();
			}
			delete [] patch.t_basis_list;

			for(j=0; j < patch.s_basis_cnt; j++)
			{
				for(int i=0; i < patch.t_basis_cnt; i++)
				{
					patch.x_polynom[j * patch.t_basis_cnt + i]->Release();
					patch.y_polynom[j * patch.t_basis_cnt + i]->Release();
					patch.z_polynom[j * patch.t_basis_cnt + i]->Release();
					if(patch.w_polynom)
						patch.w_polynom[j * patch.t_basis_cnt + i]->Release();

					if(patch.u_polynom)
						patch.u_polynom[j * patch.t_basis_cnt + i]->Release();
					if(patch.v_polynom)
						patch.v_polynom[j * patch.t_basis_cnt + i]->Release();
				}
			}
			delete [] patch.x_polynom;
			delete [] patch.y_polynom;
			delete [] patch.z_polynom;
			delete [] patch.w_polynom;
			delete [] patch.u_polynom;
			delete [] patch.v_polynom;

			delete [] patch.s_knot_list;
			delete [] patch.t_knot_list;
			delete [] patch.point_list;
			delete [] patch.weight_list;
			delete [] patch.uv_list;
			delete [] patch.normals;
			delete [] patch.vertices;
			delete [] patch.D_coefficient;
		}
		delete [] patch_list;
		delete [] material_list; // material destructor releases textures
	}

	bool read (struct IFileSystem * parent, struct ITextureLibrary * txm_lib);
	bool InitBasis (void);
	bool InitPolys (const struct PolynomialEnumData poly_data_user[],
					const struct PolynomialEnumData & poly_data_linear);
	bool AssignBasis (void);
	bool AssignData (void);
	bool AssignUVData (void);
	
	void compute_bounds (void);
	void compute_centroid (void);
	void get_bounding_box (SINGLE box[6]) const;
	// Fills in array of 8 vertices of bounding box (in object space).
	//void get_bounding_box(Vector * verts) const;
	void copy_nurb(const XNURB & src);
};

#endif