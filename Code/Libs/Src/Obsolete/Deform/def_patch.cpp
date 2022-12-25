#include "fdump.h"
#include "eng.h"
#include "deform.h"
#include "material.h"
#include "inv_sqrt.h"

static ISQRT isqrt;

// NOTE: if anything is added here it should also go into eng.h
namespace Deform
{
	ICOManager *		DACOM = NULL;
	IEngine *			ENG = NULL;
	IModel *			MODEL = NULL;
	ILightManager *		LIGHT = NULL;
	ITXMLib *			TXMLIB = NULL;
	IAnimation *		ANIM = NULL;
	IChannel *			CHANNEL = NULL;
	IHardpoint *		HARDPOINT = NULL;
	IPhysics *			PHYSICS = NULL;
	ICollision *		COLLIDE = NULL;
	IDumpText *			DUMP = NULL;
	IRenderPipeline *	PIPE = NULL;
	IRenderPrimitive *	BATCH = NULL;

	MTPrimitiveBuilder	pb;

	int						vertex_pool_len = 0;
	int						vertex_pool_index = 0;
	MTVERTEX *				vertex_pool = NULL;
	Vector *				normal_pool = NULL;
	U32 *					normal_index_pool = NULL;
	LightRGB *				light_pool = NULL;
	
	int						index_list_len = 0;
	int						index_list_index = 0;
	U16 *					index_list = NULL;
	U16 *					vertex_slot = NULL;

	// PATCH stuff
	int						sub_div_cnt = 0;			// normal range from 1 - N
	float *					div_weights = NULL;
	float *					div_n_weights = NULL;
	Vector					patch_aux[9];


	bool					active = false;

	U32						default_material_flags;
	U32						device_supports_uvchannel1;
	U32						device_num_tss;

	U32						specular_mode;				// 0, 1, or 2
	char					specular_texture_name[64];

	U32						diffuse2_fallback_blend[2];	// src,dst framebuffer blend modes for Diffuse1*Diffuse2
	U32						emissive_fallback_blend[2];	// src,dst framebuffer blend modes for + Emissive
	U32						specular_fallback_blend[2];

	float					min_poly_size = .15f;
	
	bool					got_ini_info = false;
};

using namespace Deform;

extern bool use_constant_alpha;
extern U8	constant_alpha;

#define IRP_CALL( call )	\
	if( BATCH ) {			\
		BATCH->call ;		\
	}						\
	else if( PIPE ) {		\
		PIPE->call ;		\
	}

void CompPatchAux(const BezierPatch & patch, const Vector *vertices, Vector aux[9])
{
	aux[0] = .25f * vertices[patch.v[0]]   + .75f * vertices[patch.vec[0]];
	aux[1] = .50f * vertices[patch.vec[0]] + .50f * vertices[patch.vec[1]];
	aux[2] = .25f * vertices[patch.v[1]]   + .75f * vertices[patch.vec[1]];

	aux[3] = .25f * vertices[patch.v[1]]   + .75f * vertices[patch.vec[2]];
	aux[4] = .50f * vertices[patch.vec[2]] + .50f * vertices[patch.vec[3]];
	aux[5] = .25f * vertices[patch.v[2]]   + .75f * vertices[patch.vec[3]];

	aux[6] = .25f * vertices[patch.v[2]]   + .75f * vertices[patch.vec[4]];
	aux[7] = .50f * vertices[patch.vec[4]] + .50f * vertices[patch.vec[5]];
	aux[8] = .25f * vertices[patch.v[0]]   + .75f * vertices[patch.vec[5]];
}

Vector interp3(const BezierPatch & patch, const float pu, const float pv,
			   const Vector *vertices, const Vector aux[9])
{
	// It had better be a triangular patch!
	ASSERT(patch.type == 3);

	const float pu2 = pu * pu;
	const float pu3 = pu2 * pu;
	//float pu4 = pu3 * pu;

	const float pv2 = pv * pv;
	const float pv3 = pv2 * pv;
	//float pv4 = pv3 * pv;

	const float pw = 1.0f - pu - pv;
	const float pw2 = pw * pw;
	const float pw3 = pw2 * pw;
	//float pw4 = pw3 * pw;

	const int *v = patch.v;
	//const int *vec = patch.vec;
	const int *interior = patch.interior;

	Vector p = 
	  pw * (
	  pw * (
	  pw * (
	  vertices[v[0]]               * pw  +
      aux[0] * 4.0f  * pu               )+
      aux[1] * 6.0f  * pu2              )+
      aux[2] * 4.0f  * pu3              )+

	  pu * (
	  pu * (
	  pu * (
      vertices[v[1]] * pu                +
      aux[3] * 4.0f          * pv       )+
      aux[4] * 6.0f          * pv2      )+
      aux[5] * 4.0f          * pv3      )+

	  pv * (
	  pv * (
	  pv * (
	  vertices[v[2]]         * pv        +
      aux[6] * 4.0f                * pw )+
      aux[7] * 6.0f                * pw2)+
      aux[8] * 4.0f                * pw3)+

	  12.0f * pu * pv * pw * (
	  vertices[interior[0]] * pw         +
      vertices[interior[1]] * pu         +
	  vertices[interior[2]] * pv        );

	return p;
}

inline Vector interp3du(const BezierPatch & patch, const float pu, const float pv,
						const Vector *vertices, const Vector aux[9]) 
{
	ASSERT(patch.type == 3);

	const float pu2 = pu  * pu;
	const float pu3 = pu2 * pu;
	
	const float pv2 = pv  * pv;
	const float pv3 = pv2 * pv;

	const float puv  = 1.0f - pu - pv;
	const float puv2 = puv  * puv;
	const float puv3 = puv2 * puv;

	const int *v = patch.v;
	const int *interior = patch.interior;

	return
	  vertices[v[0]]	* -4.0f * puv3 +
	  vertices[v[1]]	*  4.0f * pu3  +
	  //vertices[v[2]]	*  0.0f

	  aux[0] *   4.0f * (puv3 - 3.0f * pu * puv2) +
	  aux[8] * -12.0f * pv * puv2 +
	  
	  aux[1] *  12.0f * pu * (puv2 - pu * puv) +
	  aux[7] * -12.0f * pv2 * puv +

	  aux[2] *   4.0f * (3.0f * pu2 * puv - pu3) +
	  aux[6] *  -4.0f * pv3 +

	  aux[3] *  12.0f * pu2 * pv +
	  aux[5] *   4.0f * pv3 +
	  aux[4] *  12.0f * pu * pv2 +

	  vertices[interior[0]]		    *  12.0f * pv * (puv2 - 2.0f * pu * puv) +
	  vertices[interior[1]]		    *  12.0f * pv * (2.0f * pu * puv - pu2) +
	  vertices[interior[2]]		    *  12.0f * pv2 * (puv - pu);
}

inline Vector interp3dv(const BezierPatch & patch, const float pu, const float pv,
						const Vector *vertices, const Vector aux[9]) 
{
	ASSERT(patch.type == 3);

	const float pu2 = pu  * pu;
	const float pu3 = pu2 * pu;

	const float pv2 = pv  * pv;
	const float pv3 = pv2 * pv;

	const float puv  = 1.0f - pu - pv;
	const float puv2 = puv  * puv;
	const float puv3 = puv2 * puv;

	const int *v = patch.v;
	const int *interior = patch.interior;

	return
	  vertices[v[0]]	*  -4.0f * puv3 +
	  //vertices[v[1]]	*   0.0f
	  vertices[v[2]]	*   4.0f * pv3 +

	  aux[0] * -12.0f * pu * puv2 +
	  aux[8] *   4.0f * ( puv3 - 3.0f * pv * puv2 ) +
	  
	  aux[1] * -12.0f * pu2 * puv +
	  aux[7] *  12.0f * pv * (puv2 - pv * puv) +

	  aux[2] *  -4.0f * pu3 +
	  aux[6] *   4.0f * (3.0f * pv2 * puv - pv3) +
	  
	  aux[3] *   4.0f * pu3 +
	  aux[5] *  12.0f * pu * pv2 +
	  aux[4] *  12.0f * pv * pu2 +

	  vertices[interior[0]]		    *  12.0f * pu * (puv2 - 2.0f * pv * puv) +
	  vertices[interior[1]]		    *  12.0f * pu2 * (puv - pv) +
	  vertices[interior[2]]		    *  12.0f * pu * (2.0f * pv * puv - pv2);
}

inline Vector bezier_interp(const Vector & v1, const Vector & v2, const Vector & v3, const Vector & v4,
							const float weights[4])
{
	return v1 * weights[0] +
		   v2 * weights[1] +
		   v3 * weights[2] +
		   v4 * weights[3];
}

void DeformablePart::render_patches(struct ICamera * camera, DeformablePartMesh * mesh, int tessellation_cnt)
{
	U32 rwm_flags;
	PIPE->get_pipeline_state( RP_TEXTURE, &rwm_flags );
	if( !rwm_flags )
	{
		rwm_flags = RWM_DONT_TEXTURE;
	}
	else
	{
		rwm_flags = 0;
	}

	DeformablePartArchetype *arch = mesh->arch;

	// light here
	// check clipping and back facing

	SetDivWeights( tessellation_cnt );

	int last_face_cnt = 0;
	int last_vertex_cnt = 0;
	const TexCoord *uv_list = arch->texture_vertex_list;
	const Vector *vertices = transformed_vertices;
	for(int i = 0; i < arch->patch_group_cnt; i++)
	{
		const PatchGroup & group = arch->patch_groups[i];

		Material *mat = arch->material_list + group.mtl_id;

		const int divs = sub_div_cnt; // minimum 1
		float inv_divs = 1.0f / (float)divs;

		vertex_pool_index = 0;
		index_list_index = 0;

		int i_cnt = 6 * divs * divs;
		verify_lists(group.patch_cnt * i_cnt);

		int v_cnt = (divs+1) * (divs+1);
		verify_pools(group.patch_cnt * v_cnt);
		
		for(int pid = 0; pid < group.patch_cnt; pid++)
		{
			const BezierPatch & patch = group.patch_list[pid];

			const int *v_id = patch.v;
			const int *vec_id = patch.vec;
			const int *int_id = patch.interior;

			const int pool_offset = vertex_pool_index;

			switch(patch.type)
			{
				case 3:
				{
					// compute 4th degree points
					CompPatchAux(patch, vertices, patch_aux);
					
					for(int ss = 0; ss <= divs; ss++)
					{
						for(int tt = 0; tt <= ss; tt++)
						{
							float s1 = (float)tt * inv_divs;
							float s2 = (float)(divs - ss) * inv_divs;
							float s3 = 1.0f - s1 - s2;

							assert(vertex_pool_index < vertex_pool_len);
							vertex_pool[vertex_pool_index].pos =
								interp3(patch, s1, s2, vertices, patch_aux);

							const Vector c1 ( interp3du(patch, s1, s2, vertices, patch_aux) );
							const Vector c2 ( interp3dv(patch, s1, s2, vertices, patch_aux) );

							//normal_pool[vertex_pool_index] = cross_product( c1, c2 ).normalize();
							normal_pool[vertex_pool_index] = cross_product( c1, c2 );
							normal_pool[vertex_pool_index] *= 
								isqrt.InvSqrt( normal_pool[vertex_pool_index].magnitude_squared() );

							//vertex_pool[vertex_pool_index].b = 255;//mat.diffuse.b;
							//vertex_pool[vertex_pool_index].g = 255;//mat.diffuse.g;
							//vertex_pool[vertex_pool_index].r = 255;//mat.diffuse.r;
							//vertex_pool[vertex_pool_index].a = 255;

							vertex_pool[vertex_pool_index].u =
								s1 * uv_list[patch.tv[1]].u +
								s2 * uv_list[patch.tv[2]].u +
								s3 * uv_list[patch.tv[0]].u;
							vertex_pool[vertex_pool_index].v =
								s1 * uv_list[patch.tv[1]].v +
								s2 * uv_list[patch.tv[2]].v +
								s3 * uv_list[patch.tv[0]].v;
							vertex_pool_index++;
						}
					}

					int base = 0;
					int next_base = 0;
					for(int layer = 0; layer < divs; layer++)
					{
						base = next_base;
						next_base += (layer+1);
						int top_id = base;
						int bot_id = next_base;
						for(int t_id = 0; t_id < 2 * layer + 1; t_id++)
						{
							if(t_id & 1)	// downright
							{
								index_list[index_list_index] = pool_offset + bot_id;
								index_list_index++;
								index_list[index_list_index] = pool_offset + top_id;
								index_list_index++;
								index_list[index_list_index] = pool_offset + (top_id+1);
								index_list_index++;

								top_id++;
							}
							else			// upright
							{
								index_list[index_list_index] = pool_offset + bot_id;
								index_list_index++;
								index_list[index_list_index] = pool_offset + top_id;
								index_list_index++;
								index_list[index_list_index] = pool_offset + (bot_id+1);
								index_list_index++;

								bot_id++;
							}
						}
					}
				}
				break;
				case 4:
				{
					for(int ss = 0; ss <= divs; ss++)
					{
						float s = (float)ss * inv_divs;
						float s1 = 1.0f - s;

						Vector ss0 = (	bezier_interp(
										vertices[v_id[0]],
										vertices[vec_id[7]],
										vertices[vec_id[6]],
										vertices[v_id[3]],
										div_weights + 4 * ss) );

						Vector ss1 = (	bezier_interp(
										vertices[vec_id[0]],
										vertices[int_id[0]],
										vertices[int_id[3]],
										vertices[vec_id[5]],
										div_weights + 4 * ss) );

						Vector ss2 = (	bezier_interp(
										vertices[vec_id[1]],
										vertices[int_id[1]],
										vertices[int_id[2]],
										vertices[vec_id[4]],
										div_weights + 4 * ss) );

						Vector ss3 = (	bezier_interp(
										vertices[v_id[1]],
										vertices[vec_id[2]],
										vertices[vec_id[3]],
										vertices[v_id[2]],
										div_weights + 4 * ss) );

						Vector dss0 = (	bezier_interp(
										vertices[v_id[0]],
										vertices[vec_id[7]],
										vertices[vec_id[6]],
										vertices[v_id[3]],
										div_n_weights + 4 * ss) );

						Vector dss1 = (	bezier_interp(
										vertices[vec_id[0]],
										vertices[int_id[0]],
										vertices[int_id[3]],
										vertices[vec_id[5]],
										div_n_weights + 4 * ss) );

						Vector dss2 = (	bezier_interp(
										vertices[vec_id[1]],
										vertices[int_id[1]],
										vertices[int_id[2]],
										vertices[vec_id[4]],
										div_n_weights + 4 * ss) );

						Vector dss3 = (	bezier_interp(
										vertices[v_id[1]],
										vertices[vec_id[2]],
										vertices[vec_id[3]],
										vertices[v_id[2]],
										div_n_weights + 4 * ss) );

						for(int tt = 0; tt <= divs; tt++)
						{
							float t = (float)tt * inv_divs;
							float t1 = 1.0f - t;
						
							assert(vertex_pool_index < vertex_pool_len);

							vertex_pool[vertex_pool_index].pos =
							bezier_interp( ss0, ss1, ss2, ss3, div_weights + 4 * tt);
							//bez_mesh->interp4(patch, s, t);

							const Vector c1 ( bezier_interp( ss0, ss1, ss2, ss3, div_n_weights + 4 * tt) );
							const Vector c2 ( bezier_interp( dss0, dss1, dss2, dss3, div_weights + 4 * tt) );

							//normal_pool[vertex_pool_index] = cross_product(c1, c2).normalize();
							normal_pool[vertex_pool_index] = cross_product(c1, c2);
							normal_pool[vertex_pool_index] *= 
								isqrt.InvSqrt( normal_pool[vertex_pool_index].magnitude_squared() );

							//vertex_pool[vertex_pool_index].b = 255;//mat.diffuse.b;
							//vertex_pool[vertex_pool_index].g = 255;//mat.diffuse.g;
							//vertex_pool[vertex_pool_index].r = 255;//mat.diffuse.r;
							//vertex_pool[vertex_pool_index].a = 255;

							vertex_pool[vertex_pool_index].u =
								(uv_list[patch.tv[0]].u * t1 + uv_list[patch.tv[1]].u * t) * s1 +
								(uv_list[patch.tv[3]].u * t1 + uv_list[patch.tv[2]].u * t) * s;

							vertex_pool[vertex_pool_index].v =
								(uv_list[patch.tv[0]].v * t1 + uv_list[patch.tv[1]].v * t) * s1 +
								(uv_list[patch.tv[3]].v * t1 + uv_list[patch.tv[2]].v * t) * s;

							vertex_pool_index++;
						}
					}

					const int i_divs = divs + 1;
					for(ss = 0; ss < divs; ss++)
					{
						for(int tt = 0; tt < divs; tt++)
						{
							// triangle 1
							index_list[index_list_index] = pool_offset + ss * i_divs + tt;
							index_list_index++;
							index_list[index_list_index] = pool_offset + (ss+1) * i_divs + tt;
							index_list_index++;
							index_list[index_list_index] = pool_offset + (ss+1) * i_divs + (tt+1);
							index_list_index++;

							// triangle 2
							index_list[index_list_index] = index_list[index_list_index-3];
							index_list_index++;
							index_list[index_list_index] = index_list[index_list_index-2];
							index_list_index++;
							index_list[index_list_index] = pool_offset + ss * i_divs + (tt+1);
							index_list_index++;
						}
					}
				}
				break;
				default:
					ASSERT(1);
			}
		}
		
		const ClampFlags light_clamp = CF_COLOR; //(flags & RF_CLAMP_COLOR) ? CF_COLOR : CF_INTENSITY;
		// light
		LIGHT->light_vertices_strided(  light_pool, sizeof(LightRGB),
										(Vector*)&(vertex_pool[0].pos), sizeof(*vertex_pool),
										normal_pool, sizeof(Vector),
										normal_index_pool, sizeof(U32),
										vertex_pool_index, &Transform(), light_clamp);

		if (mat->flags & MF_EMITTER)
		{
			for(int vid = 0; vid < vertex_pool_index; vid++)
			{
				vertex_pool[vid].r = _MIN(255, mat->emission.r + ((mat->diffuse.r * light_pool[vid].r) >> 8));
				vertex_pool[vid].g = _MIN(255, mat->emission.g + ((mat->diffuse.g * light_pool[vid].g) >> 8));
				vertex_pool[vid].b = _MIN(255, mat->emission.b + ((mat->diffuse.b * light_pool[vid].b) >> 8));
				vertex_pool[vid].a = mat->transparency;
			}
		}
		else
		{
			for(int vid = 0; vid < vertex_pool_index; vid++)
			{
				vertex_pool[vid].r = _MIN(255, (mat->diffuse.r * light_pool[vid].r) >> 8);
				vertex_pool[vid].g = _MIN(255, (mat->diffuse.g * light_pool[vid].g) >> 8);
				vertex_pool[vid].b = _MIN(255, (mat->diffuse.b * light_pool[vid].b) >> 8);
				vertex_pool[vid].a = mat->transparency;
			}
		}

		last_face_cnt += index_list_index / 3;
		last_vertex_cnt += vertex_pool_index;

		IRP_CALL ( set_render_state( D3DRS_CULLMODE, D3DCULL_CCW ) )
		mtl_render_indexed_primitive_list(mat, 1, rwm_flags);
	}
}
