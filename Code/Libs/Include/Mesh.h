//
// ¡Viva la Revolución! Mesh access for all.
//

#ifndef MESH_H
#define MESH_H

//
#include <string.h>

#include "3DMath.h"
#include "FaceProp.h"
#include "EdgeProp.h"
#include "Material.h"
#include "UVChannel.h"
#include "cont_lod.h"
#include "FaceGroup.h"
#include "fdump.h"
#include "TextureCoord.h"
#include "IMaterialManager.h"

//
// See member functions for examples of how these lists index into
// 
//  Batch lists support TRIANGLES only

struct Mesh
{
	int				face_cnt;					// # of faces
	int				face_group_cnt;
	FaceGroup *		face_groups;
	U32				sorted_faces;				// flag indicating faces are sorted by area

	U32				have_smooth_shaded_faces;
	U32				have_double_sided_faces;
										
//
// List of unique 3D object-space vertices in mesh
//
	int				object_vertex_cnt;			// # of unique object-space vertices
	Vector *		object_vertex_list;			// object-space vertices.
	unsigned char * vertex_color_list;			// optional vertex colors
//
// List of unique 2D texture-space vertices in mesh
//
	int				texture_vertex_cnt;
	TexCoord *		texture_vertex_list;

// parallel unique lists of vertex indices
	int				vertex_batch_cnt;			// numer of unique vertices based on x,y,z,u,v,m
	int	*			vertex_batch_list;			// indices into object_vertex_list
	int *			texture_batch_list;			// indices into texture_vertex_list
	int *			texture_batch_list2;			// (optional) second uv indices indices into texture_vertex_list
	int				normal_cnt;					// # of unique surface normals 
	Vector *		normal_ABC;					// Normal A, B, and C coefficients (Ax+By+Cz)

//
// Indices into normal list
//
	int *			vertex_normal;				// In same order as object_vertex_list.
	float *			vertex_D_coefficient;		
//
// List of materials used in mesh
// 
	int				material_cnt;
	Material *		material_list;

	int				imaterial_cnt;
	IMaterial **	imaterial_list;

	int				edge_cnt;					// number of edges
	int	*			edge_vertex_chain;			// indices into object_vertex_list (2 per edge)
	float *			edge_angle_list;			// sorted list of angles between the normals of the two faces
												// of this edge

	float			radius;						// centered sphere radius
	Vector			sphere_center;				// sphere center

	float			bounds[6];					// max_x, min_x, max_y, min_y, max_z, min_z in object space
	Vector			centroid;					// average of all vertices

//
// UV animation data
//
	int				uv_channel_cnt;				// number of UV texture animation channels
	UVChannel *		uv_channel_list;			// channels w/ uv animations

//
// Lod data to deform mesh
//
	bool			allow_lod_change;	
	float			lod_fraction;				// amount of current detail 0 to 1
	float			lod_fraction_previous;		// used to detect thrashing
	float			lod_threshold;              // minimal change before any work is done .000001f
	int				lod_step_cnt;				// number of edge collapses/vertex splits
	lod_step *		lod_step_list;				// data for each split
	lod_uv_step *	lod_uv_step_list;			// UV data for each split

	int				lod_removed_face_cnt;		// number of removed faces
	int *			lod_removed_face_list;		// group is'd of removed faces

	int				lod_morphed_face_cnt;		// number of morphed faces
	int *			lod_morphed_face_list;		// morphed face indices
	int *			lod_group_face_list;		// group id's of morphed faces
	int *			lod_high_face_normal_list;	// normals for high detail morphed faces
	float *			lod_high_face_D_coefficient;// D's for morphed faces
	int *			lod_low_face_normal_list;	// normals for low detail morphed faces
	float *			lod_low_face_D_coefficient;	// D's for morphed faces

	int				lod_vertex_cnt;				// number of reindexed vertices
	int *			lod_vertex_list;			// indices into object_vertex_chain

	int				lod_edge_cnt;				// number of reindexed edges
	int *			lod_edge_list;				// indices into edge_vertex_chain

	int				uv_chain_count;				// length of lod_uv_batch_chain
	int *			lod_uv_batch_chain;			// chain of affected uv batch indices

	int				lod_uv_count;				// length of uv lists
	int *			lod_uv_high_id1;			// high detail indices into texture_vertex_list for above
	int *			lod_uv_batch_cnt1;			// number of affected batch vertices for uv1
	int *			lod_uv_batch_first1;		// position of first one in the chain
	int *			lod_uv_high_id2;			
	int *			lod_uv_batch_cnt2;
	int *			lod_uv_batch_first2;
	int *			lod_uv_low_id;				// low detail indices

	TC_UVGENERATOR		 tc_user_generator;
	void				*tc_user_generator_context;
//
// Here are some functions that show how to extract information from
// the data above:
//
	int get_face_num_vertices(int face) const
	{
		return 3;
	}

	Vector * get_face_vertex(int face, int vertex) const
	{
		int group;
		int new_face;
		get_group_face(face, &group, &new_face);

		return object_vertex_list +
			vertex_batch_list[ face_groups[group].face_vertex_chain[3 * new_face + vertex] ];
	}

	int get_face_vertex_index(int face, int vertex) const
	{
		int group;
		int new_face;
		get_group_face(face, &group, &new_face);
		return vertex_batch_list[face_groups[group].face_vertex_chain[3 * new_face + vertex]];
	}

	TexCoord * get_face_texcoord(int face, int vertex) const
	{
		int group;
		int new_face;
		get_group_face(face, &group, &new_face);

		return texture_vertex_list +
			texture_batch_list[ face_groups[group].face_vertex_chain[3 * new_face + vertex] ];
	}

	Material * get_face_material(int face) const
	{
		return material_list + face_groups[ get_group(face) ].material;
	}

	Vector * get_face_normal(int face)
	{
		int group;
		int new_face;
		get_group_face(face, &group, &new_face);

		return normal_ABC + face_groups[group].face_normal[new_face];
	}

	Vector * get_vertex_normal(int face, int vertex)
	{
		int group;
		int new_face;
		get_group_face(face, &group, &new_face);

		return normal_ABC +
			vertex_normal[ vertex_batch_list[ face_groups[group].face_vertex_chain[3 * new_face + vertex] ] ];
	}

	Material * find_material(const char * name)
	{
		Material * result = NULL;
		Material * mat = material_list;
		for (int i = 0; i < material_cnt; i++, mat++)
		{
			if (strcmp(mat->name, name) == 0)
			{
				result = mat;
				break;
			}
		}
		return result;
	}

	int get_group(int face) const
	{
		int subtotal = 0;
		for(int result = 0; result < face_group_cnt; result++)
		{
			subtotal += face_groups[result].face_cnt;
			if( face < subtotal )
			{
				return result;
			}
		}

		// not found
		return -1;
	}

	void get_group_face(int face, int *group, int *new_face) const
	{
		*group = 0;
		*new_face = face;

		for(int g_id = 0; g_id < face_group_cnt; g_id++)
		{
			if(*new_face < face_groups[g_id].face_cnt)
			{
				break;
			}

			(*group)++;
			(*new_face) -= face_groups[g_id].face_cnt;
		}
	}

	GENRESULT add_lod_step(const int step_id)
	{
		GENRESULT result = GR_GENERIC;
		ASSERT(lod_step_list);
		ASSERT(step_id < lod_step_cnt);

		const lod_step & current_los = lod_step_list[step_id];

		// add FACES
		ASSERT(current_los.first_removed_face + current_los.removed_face_count <= lod_removed_face_cnt);
		for(int rf_id = current_los.first_removed_face;
				rf_id < current_los.first_removed_face + current_los.removed_face_count;
				rf_id++)
		{
			face_groups[ lod_removed_face_list[rf_id] ].face_cnt++;
			face_cnt++;
		}

		// morph faces
		ASSERT(current_los.first_morphed_face + current_los.morphed_face_count <= lod_morphed_face_cnt);
		for(int mf_id = current_los.first_morphed_face;
				mf_id < current_los.first_morphed_face + current_los.morphed_face_count;
				mf_id++)
		{	
			FaceGroup *fg = face_groups + lod_group_face_list[mf_id];
			ASSERT(fg < face_groups + face_group_cnt);

			int f_id = lod_morphed_face_list[mf_id];
			ASSERT(f_id < fg->face_cnt);

			// change face normal
			fg->face_normal[f_id] = lod_high_face_normal_list[mf_id];
			// change face D
			fg->face_D_coefficient[f_id] = lod_high_face_D_coefficient[mf_id];
		}

		// add VERTICES
		vertex_batch_cnt += current_los.removed_batch_vertex_count;

		object_vertex_list[current_los.vid1] = current_los.v1;
		vertex_normal[current_los.vid1] = current_los.v1_nid;
		vertex_D_coefficient[current_los.vid1] = current_los.v1_D;

		object_vertex_list[current_los.vid2] = current_los.v2;
		vertex_normal[current_los.vid2] = current_los.v2_nid;
		vertex_D_coefficient[current_los.vid2] = current_los.v2_D;

		ASSERT(current_los.vid2 == object_vertex_cnt);
		object_vertex_cnt++;

		// morph VERTICES
		ASSERT(current_los.first_vertex + current_los.morphed_vertex_chain_count <= lod_vertex_cnt);
		for(int mv_id = current_los.first_vertex;
				mv_id < current_los.first_vertex + current_los.morphed_vertex_chain_count;
				mv_id++)
		{
			int v_id = lod_vertex_list[mv_id];

			ASSERT(v_id < vertex_batch_cnt);

			ASSERT(vertex_batch_list[v_id] == current_los.vid1);
			vertex_batch_list[v_id] = current_los.vid2;
		}

		// UV's
		if(lod_uv_step_list)
		{
			const lod_uv_step & current_uv_los = lod_uv_step_list[step_id];
			for(int i = current_uv_los.first_morphed_uv; 
					i < current_uv_los.first_morphed_uv + current_uv_los.morphed_uv_count;
					i++)
			{
				int j;
				for(j = lod_uv_batch_first1[i];
						j < lod_uv_batch_first1[i] + lod_uv_batch_cnt1[i];
						j++)
				{
					texture_batch_list[ lod_uv_batch_chain[j] ] = lod_uv_high_id1[i];
				}

				for(	j = lod_uv_batch_first2[i];
						j < lod_uv_batch_first2[i] + lod_uv_batch_cnt2[i];
						j++)
				{
					texture_batch_list[ lod_uv_batch_chain[j] ] = lod_uv_high_id2[i];
				}
			}
		}

	/*
		// remove EDGES
		edge_cnt += current_los.removed_edge_count;

		// morph EDGES
		ASSERT(current_los.first_edge + current_los.morphed_edge_count <= lod_edge_cnt);
		for(int me_id=0; me_id < current_los.morphed_edge_count; me_id++)
		{
			int e_id = lod_edge_list[current_los.first_edge + me_id];

			ASSERT(e_id < edge_cnt);

			// TODO: store which of the two vertices is changed
			if(edge_vertex_chain[2*e_id] == current_los.vid1)
			{
				edge_vertex_chain[2*e_id] = current_los.vid2;
			}
			else
			{
				ASSERT(edge_vertex_chain[2*e_id+1] == current_los.vid1);
				edge_vertex_chain[2*e_id+1] = current_los.vid2;
			}
		}
	*/

		result = GR_OK;
		return result;
	}

	GENRESULT remove_lod_step(const int step_id)
	{
		GENRESULT result = GR_GENERIC;
		ASSERT(lod_step_list);
		ASSERT(step_id < lod_step_cnt);

		const lod_step & current_los = lod_step_list[step_id];

		// remove FACES
		ASSERT(current_los.first_removed_face + current_los.removed_face_count <= lod_removed_face_cnt);
		for(int rf_id = current_los.first_removed_face;
				rf_id < current_los.first_removed_face + current_los.removed_face_count;
				rf_id++)
		{
			face_groups[ lod_removed_face_list[rf_id] ].face_cnt--;
			face_cnt--;
		}

		// morph faces
		ASSERT(current_los.first_morphed_face + current_los.morphed_face_count <= lod_morphed_face_cnt);
		for(int mf_id = current_los.first_morphed_face;
				mf_id < current_los.first_morphed_face + current_los.morphed_face_count;
				mf_id++)
		{	
			FaceGroup *fg = face_groups + lod_group_face_list[mf_id];
			ASSERT(fg < face_groups + face_group_cnt);

			int f_id = lod_morphed_face_list[mf_id];
			ASSERT(f_id < fg->face_cnt);

			// change face normal
			fg->face_normal[f_id] = lod_low_face_normal_list[mf_id];
			// change face D
			fg->face_D_coefficient[f_id] = lod_low_face_D_coefficient[mf_id];
		}

		// UV's
		if(lod_uv_step_list)
		{
			const lod_uv_step & current_uv_los = lod_uv_step_list[step_id];
			for(int i = current_uv_los.first_morphed_uv; 
					i < current_uv_los.first_morphed_uv + current_uv_los.morphed_uv_count;
					i++)
			{
				int j;
				for(j = lod_uv_batch_first1[i];
						j < lod_uv_batch_first1[i] + lod_uv_batch_cnt1[i]; j++)
				{
					texture_batch_list[ lod_uv_batch_chain[j] ] = lod_uv_low_id[i];
				}

				for(	j = lod_uv_batch_first2[i];
						j < lod_uv_batch_first2[i] + lod_uv_batch_cnt2[i]; j++)
				{
					texture_batch_list[ lod_uv_batch_chain[j] ] = lod_uv_low_id[i];
				}
			}
		}

		// remove VERTICES
		vertex_batch_cnt -= current_los.removed_batch_vertex_count;

		object_vertex_list[current_los.vid1] = current_los.v3;
		vertex_normal[current_los.vid1] = current_los.v3_nid;
		vertex_D_coefficient[current_los.vid1] = current_los.v3_D;

		object_vertex_cnt--;
		ASSERT(current_los.vid2 == object_vertex_cnt);

		// morph VERTICES
		ASSERT(current_los.first_vertex + current_los.morphed_vertex_chain_count <= lod_vertex_cnt);
		for(int mv_id = current_los.first_vertex;
				mv_id < current_los.first_vertex + current_los.morphed_vertex_chain_count;
				mv_id++)
		{
			int v_id = lod_vertex_list[mv_id];

			ASSERT(v_id < vertex_batch_cnt);

			ASSERT(vertex_batch_list[v_id] == current_los.vid2);
			vertex_batch_list[v_id] = current_los.vid1;
		}

	/*
		// remove EDGES
		edge_cnt -= current_los.removed_edge_count;

		// morph EDGES
		ASSERT(current_los.first_edge + current_los.morphed_edge_count <= lod_edge_cnt);
		for(int me_id=0; me_id < current_los.morphed_edge_count; me_id++)
		{
			
			int e_id = lod_edge_list[current_los.first_edge + me_id];

			ASSERT(e_id < edge_cnt);

			// TODO: store which of the two vertices is changed
			if(edge_vertex_chain[2*e_id] == current_los.vid2)
			{
				edge_vertex_chain[2*e_id] = current_los.vid1;
			}
			else
			{
				ASSERT(edge_vertex_chain[2*e_id+1] == current_los.vid2);
				edge_vertex_chain[2*e_id+1] = current_los.vid1;
			}
		}
	*/
		
		result = GR_OK;
		return result;
	}

	GENRESULT interpolate_lod_step(const int step_id, const float fraction)
	{
		GENRESULT result = GR_GENERIC;
		ASSERT(lod_step_list);
		ASSERT(step_id < lod_step_cnt);

		const float one_minus_fraction = 1.0f - fraction;

		const lod_step& new_los = lod_step_list[step_id];

		object_vertex_list[new_los.vid1] =
			fraction * new_los.v1 +
			one_minus_fraction * new_los.v3;

		object_vertex_list[new_los.vid2] =
			fraction * new_los.v2 +
			one_minus_fraction * new_los.v3;

		vertex_D_coefficient[new_los.vid1] =
			fraction * new_los.v1_D +
			one_minus_fraction * new_los.v3_D;

		vertex_D_coefficient[new_los.vid2] =
			fraction * new_los.v2_D +
			one_minus_fraction * new_los.v3_D;

		// TODO: interpolate normal smoothly
		if(fraction >= 0.0f)
		{
			vertex_normal[new_los.vid1] = new_los.v1_nid;
			vertex_normal[new_los.vid2] = new_los.v2_nid;
		}
		else
		{
			vertex_normal[new_los.vid1] = 
			vertex_normal[new_los.vid2] = new_los.v3_nid;
		}

		// UV's
		if(lod_uv_step_list)
		{
			for(int i = 0, j = lod_uv_step_list[step_id].first_morphed_uv;
					i < 2 * lod_uv_step_list[step_id].morphed_uv_count;
					i+=2, j++)
			{
				const TexCoord tmp ( one_minus_fraction * texture_vertex_list[ lod_uv_low_id[j] ] );

				texture_vertex_list[i] = 
					tmp + fraction * texture_vertex_list[ lod_uv_high_id1[j] ];
				texture_vertex_list[i + 1] = 
					tmp + fraction * texture_vertex_list[ lod_uv_high_id2[j] ];
			}
		}

		result = GR_OK;
		return result;
	}

	void set_uv_indices(const int step_id)
	{
		if(lod_uv_step_list)
		{
			for(int i = 0, j = lod_uv_step_list[step_id].first_morphed_uv;
					i < 2 * lod_uv_step_list[step_id].morphed_uv_count;
					i+=2, j++)
			{
				int k;
				for(k = lod_uv_batch_first1[j];
						k < lod_uv_batch_first1[j] + lod_uv_batch_cnt1[j];
						k++)
				{
					texture_batch_list[ lod_uv_batch_chain[k] ] = i;
				}

				for(	k = lod_uv_batch_first2[j];
						k < lod_uv_batch_first2[j] + lod_uv_batch_cnt2[j];
						k++)
				{
					texture_batch_list[ lod_uv_batch_chain[k] ] = i + 1;
				}
			}
		}
	}

	void restore_uv_indices(const int step_id)
	{
		// reset uv indices back from interpolation
		if(lod_uv_step_list)
		{
			const lod_uv_step & current_uv_los = lod_uv_step_list[step_id];
			for(int i = current_uv_los.first_morphed_uv; 
					i < current_uv_los.first_morphed_uv + current_uv_los.morphed_uv_count;
					i++)
			{
				int j;
				for(j = lod_uv_batch_first1[i];
						j < lod_uv_batch_first1[i] + lod_uv_batch_cnt1[i];
						j++)
				{
					texture_batch_list[ lod_uv_batch_chain[j] ] = lod_uv_high_id1[i];
				}

				for(	j = lod_uv_batch_first2[i];
						j < lod_uv_batch_first2[i] + lod_uv_batch_cnt2[i];
						j++)
				{
					texture_batch_list[ lod_uv_batch_chain[j] ] = lod_uv_high_id2[i];
				}
			}
		}
	}

	GENRESULT set_lod(float fraction)
	{
		GENRESULT result = GR_GENERIC;

		if(lod_step_list && allow_lod_change)
		{
			// guard against invalid range
			if(fraction < 0.0f)
			{
				fraction = 0.0f;
			}
			else
			if(fraction > 1.0f)
			{
				fraction = 1.0f;
			}

			if(fabs(fraction - lod_fraction) > .25f && fabs(lod_fraction - lod_fraction_previous) > .25f)
			{
				GENERAL_TRACE_1("Thrashing cont. LOD\n");
			}

			// don't update if change is infenticimal
			if( (fabs(fraction - lod_fraction) > lod_threshold) || 
				((fraction == 1.0f || fraction == 0.0f) && fraction != lod_fraction) )
			{
				const int new_step =  __max(0, (int)ceil(fraction	  * lod_step_cnt) - 1);
				const int last_step = __max(0, (int)ceil(lod_fraction * lod_step_cnt) - 1);

				const float sub_fraction = (fraction * lod_step_cnt) - new_step;

				// special case for increasing from minimum
				if(lod_fraction == 0.0f && fraction > 0.0f)
				{
					add_lod_step(0);
					set_uv_indices(0);
					sorted_faces = 0;
				}

				if(new_step == last_step)  // interpolation only
				{
					interpolate_lod_step(new_step, sub_fraction);
				}
				else
				// avoiding VC++ 6.0 bug //PCI, ms
				if(new_step <= last_step)  // collapse some edges
				{
					for(int step_id = last_step; step_id > new_step; step_id--)
					{
						remove_lod_step(step_id);	
					}

					set_uv_indices(new_step);
					interpolate_lod_step(new_step, sub_fraction);
				}
				else  //if(new_step > last_step) // split some vertices
				{
					interpolate_lod_step(last_step, 1.0f);
					restore_uv_indices(last_step);

					for(int step_id = last_step+1; step_id <= new_step; step_id++)
					{
						add_lod_step(step_id);
					}

					set_uv_indices(new_step);
					interpolate_lod_step(new_step, sub_fraction);
				}

				// special case for going to minimum
				if(fraction == 0.0f && lod_fraction > 0.0f)
				{
					remove_lod_step(0);
					sorted_faces = 1;
				}

				lod_fraction_previous = lod_fraction;
				lod_fraction = fraction;
			}

			result = GR_OK;
		}

		return result;
	}
};

//

#endif