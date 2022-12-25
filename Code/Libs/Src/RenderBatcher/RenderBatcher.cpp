// RenderBatcher.cpp
//
//

#define WIN32_LEAN_AND_MEAN
//#define INITGUID

#include <windows.h>
#include <stdio.h>
#include <assert.h>
#include <ddraw.h>

#include "dacom.h"
#include "TSmartPointer.h"
#include "tcomponent.h"
#include "FDump.h"
#include "TempStr.h"
#include "FVF.h"
#include "da_d3dtypes.h"
#include "da_heap_utility.h"
#include "pixel.h"
#include "3dmath.h"
#include "stddat.h"
#include "RPUL.h"
#include "rendpipeline.h"
#include "IRenderPrimitive.h"
#include "IProfileParser_Utility.h"


//

const U32 RPR_OPAQUE_POOL_SIZE_DEFAULT		= 512*1024;
const U32 RPR_TRANSLUCENT_POOL_SIZE_DEFAULT	= 64*1024;

//

const unsigned int DefaultListLength = 1024 * 16;
static RPVertex lvertex_list[DefaultListLength];
static U16 index_list[DefaultListLength];

//

struct PRIMITIVETYPEINFO {
	U32 vert_fixed_cost;		//	           (num_vertices - vert_fixed_cost) 
								// num_prims =  ------------------------------           
	U32 vert_per_prim_cost;		//                    vert_per_prim_cost
	D3DPRIMITIVETYPE type;		// type used when rendering
	U32	vert_count;				// count used when rendering
};

static PRIMITIVETYPEINFO primitive_type_info[7] = {
	{1,1,D3DPT_POINTLIST,1},
	{0,1,D3DPT_POINTLIST,1},
	{0,2,D3DPT_LINELIST,2},
	{1,1,D3DPT_LINELIST,2},
	{0,3,D3DPT_TRIANGLELIST,3},
	{2,1,D3DPT_TRIANGLELIST,3},
	{2,1,D3DPT_TRIANGLELIST,3}
};


//

struct BATCHEDCALLPRIMITIVE
{
public:	// Data

	Transform	modelview;

	BATCHEDCALLPRIMITIVE *next,*prev;

	D3DPRIMITIVETYPE	 p_type;
	U32					 p_flags;
	
	U32					 verts_cnt;
	void				*verts;
	U32					 verts_type;

	U32					 indices_cnt;
	U16					*indices;

	float				 p_depth;

public:	// Interface

	//

	void initialize( )
	{
		indices = NULL;
		next = NULL;
		prev = NULL;
		p_depth = -FLT_MAX;
	}

	//

	void render( IRenderPipeline *renderpipe )
	{
		renderpipe->set_modelview( modelview );

		if( indices_cnt ) {
			renderpipe->draw_indexed_primitive( p_type, verts_type, verts, verts_cnt, indices, indices_cnt, p_flags );
		}
		else {
			renderpipe->draw_primitive( p_type, verts_type, verts, verts_cnt, p_flags );
		}
	}

	//
		
	float dip_tri_depth( U16 *v0_idx, U32 sizeof_vert, float *MT )
	{
		// TODO: this should not recalc the depths of vertices
		// TODO: each time, we should calc all the depths once,
		// TODO: then use those pre-calced ones.
		//
		Vector *P;
		
		P = ((Vector*)(((char*)verts) + (*v0_idx) * sizeof_vert));
		float v0d = MT[0] * P->x + MT[1] * P->y + MT[2] * P->z + MT[3];
		
		v0_idx++;
		P = ((Vector*)(((char*)verts) + (*v0_idx) * sizeof_vert));
		float v1d = MT[0] * P->x + MT[1] * P->y + MT[2] * P->z + MT[3];

		v0_idx++;
		P = ((Vector*)(((char*)verts) + (*v0_idx) * sizeof_vert));
		float v2d = MT[0] * P->x + MT[1] * P->y + MT[2] * P->z + MT[3];

		return (v0d+v1d+v2d) * (1.0/3.0);
	}

	//

	void sort_dip( int first, int last, float *MT )
	{
		U32 sizeof_vert = FVF_SIZEOF_VERT(verts_type);

		U16 swap_buffer[3];		// triangle swap space

		int lb = first, ub = last;				
		int pivot = (first+last)/2;
		
		U16 *t_lb    = &indices[lb*3];		// lower bound tri
		U16 *t_ub    = &indices[ub*3];		// upper bound tri
		U16 *t_pivot = &indices[pivot*3]; 	// pivot tri

		#define T_DIP_DEPTH(I) (dip_tri_depth(I,sizeof_vert,MT))
		
		float t_pivot_depth = T_DIP_DEPTH(t_pivot);
		float t_lb_depth, t_ub_depth;

		do {

			t_lb_depth = T_DIP_DEPTH(t_lb);
			while( t_lb_depth < t_pivot_depth ) {	// while t_lb is less than t_pivot
				lb++;
				t_lb = &indices[lb*3];
				t_lb_depth = T_DIP_DEPTH(t_lb);
			}

			t_ub_depth = T_DIP_DEPTH(t_ub);
			while( t_ub_depth > t_pivot_depth ) {	// while t_ub is greater than t_pivot
				ub--;
				t_ub = &indices[ub*3];
				t_ub_depth = T_DIP_DEPTH(t_ub);
			}

			if( lb <= ub ) {
				swap_buffer[0] = t_lb[0];	swap_buffer[1] = t_lb[1];	swap_buffer[2] = t_lb[2];
				t_lb[0] = t_ub[0];			t_lb[1] = t_ub[1];			t_lb[2] = t_ub[2];
				t_ub[0] = swap_buffer[0];	t_ub[1] = swap_buffer[1];	t_ub[2] = swap_buffer[2];
				lb++;								
				ub--;								
			}
		} while ( lb <= ub );						

		if( first < ub ) {
			sort_dip( first, ub, MT );
		}

		if ( lb < last ) {							
			sort_dip( lb, last, MT );
		}
	}

	//
		
	float dp_tri_depth( Vector *v0, U32 sizeof_vert, float *MT )
	{
		Vector *P;
		
		P = v0;
		float v0d = MT[0] * P->x + MT[1] * P->y + MT[2] * P->z + MT[3];
		
		P = ((Vector*)(((char*)v0) + sizeof_vert));
		float v1d = MT[0] * P->x + MT[1] * P->y + MT[2] * P->z + MT[3];

		P = ((Vector*)(((char*)v0) + sizeof_vert));
		float v2d = MT[0] * P->x + MT[1] * P->y + MT[2] * P->z + MT[3];


		return (v0d+v1d+v2d) * (1.0/3.0);
	}

	//

	void sort_dp( int first, int last, float *MT )
	{
		U32 sizeof_vert = FVF_SIZEOF_VERT(verts_type);
		U32 sizeof_tri = 3 * sizeof_vert;

		ASSERT( sizeof_vert <= 64 );	

		char swap_buffer[64*3];		// triangle swap space

		int lb = first, ub = last;				
		int pivot = (first+last)/2;
		
		Vector *t_lb    = ((Vector*)(((char*)verts) + lb*sizeof_tri)); 
		Vector *t_ub    = ((Vector*)(((char*)verts) + ub*sizeof_tri)); 
		Vector *t_pivot = ((Vector*)(((char*)verts) + pivot*sizeof_tri)); 

		#define T_DP_DEPTH(P) (dp_tri_depth(P,sizeof_vert,MT))
		
		float t_pivot_depth = T_DP_DEPTH(t_pivot);
		float t_lb_depth, t_ub_depth;

		do {

			t_lb_depth = T_DP_DEPTH(t_lb);
			while( T_DP_DEPTH(t_lb) < t_pivot_depth ) {	// while t_lb is less than t_pivot
				lb++;
				t_lb = ((Vector*)(((char*)verts) + lb*sizeof_tri));
				t_lb_depth = T_DP_DEPTH(t_lb);
			}

			t_ub_depth = T_DP_DEPTH(t_ub);
			while( T_DP_DEPTH(t_ub) > t_pivot_depth ) {	// while t_ub is greater than t_pivot
				ub--;
				t_ub = ((Vector*)(((char*)verts) + ub*sizeof_tri));
				t_ub_depth = T_DP_DEPTH(t_ub);
			}

			if( lb <= ub ) {	
				memcpy( swap_buffer, t_lb, sizeof_tri );
				memcpy( t_lb, t_ub, sizeof_tri );
				memcpy( t_ub, swap_buffer, sizeof_tri );
				lb++;								
				ub--;								
			}
		} while ( lb <= ub );						

		if( first < ub ) {
			sort_dp( first, ub, MT );
		}

		if ( lb < last ) {							
			sort_dp( lb, last, MT );
		}
	}

	//

	void sort()
	{
		return;

		if( p_type != D3DPT_TRIANGLELIST ) {
			return;
		}

		float mt[4];

		mt[0] = modelview.d[0][2];
		mt[1] = modelview.d[1][2];
		mt[2] = modelview.d[2][2];
		mt[3] = modelview.translation.z;

		if( indices_cnt ) {
			sort_dip( 0, indices_cnt/3 - 1, mt );

		}
		else {
			sort_dp( 0, verts_cnt/3 - 1, mt );
		}
	}

	//

	float depth()
	{
		if( p_depth == -FLT_MAX ) {
			
			// find depth of this primitive in camera space
			//
			
			float	Mx = modelview.d[0][2];
			float	My = modelview.d[1][2];
			float	Mz = modelview.d[2][2];
			float	Tz = modelview.translation.z;
			float	v_depth;

			U32		sizeof_vert = FVF_SIZEOF_VERT(verts_type);

			Vector *P = (Vector*)(verts);

			for( U32 v=0; v<verts_cnt; v++ ) {

				if( (v_depth = (Mx * P->x + My * P->y + Mz * P->z + Tz)) > p_depth ) {
					p_depth = v_depth;
				}

				P = (Vector*)( ((char*)verts) + (v+1)*sizeof_vert );
			}
		}

		return p_depth;
	}
};

//

#define BM_M_F_HAS_BLEND_ENABLE	(1<<0)
#define BM_M_F_HAS_BLEND_FUNC	(1<<1)
#define BM_M_F_HAS_BLEND		(BM_M_F_HAS_BLEND_ENABLE|BM_M_F_HAS_BLEND_FUNC)
#define BM_M_F_HAS_TEXTURE		(1<<2)
#define BM_M_F_HAS_DEPTH_ENABLE	(1<<3)
#define BM_M_F_HAS_DEPTH_FUNC	(1<<4)
#define BM_M_F_HAS_DEPTH		(BM_M_F_HAS_DEPTH_ENABLE|BM_M_F_HAS_DEPTH_FUNC)
#define BM_M_F_HAS_CULL			(1<<5)
#define BM_M_F_HAS_DITHER		(1<<6)
#define BM_M_F_HAS_FOG			(1<<7)
#define BM_M_F_HAS_ATEST		(1<<8)

//

#define BM_NUM_STAGES 4

//

struct BATCHEDMATERIAL
{
public:	// Data

	U32 stamp;
	U32 flags;

	U32 blend_enable;
	U32 blend_src;
	U32 blend_dst;

	U32 atest_ref;
	U32 atest_enable;
	U32 atest_func;

	U32 texture_stage_cnt;
	U32 texture_stage_state[BM_NUM_STAGES][D3DTSS_MAX_STATE];
//	U32 texture_stage_wrap[BM_NUM_STAGES];
	U32 texture_stage_texture[BM_NUM_STAGES];

	U32 depth_enable;
	U32 depth_write_enable;
	U32 depth_func;

	U32 cull_mode;

	U32 dither_enable;

	U32 fog_enable;
	U32 fog_start;
	U32 fog_end;
	U32 fog_density;
	U32 fog_mode;
	U32 fog_color;

	LList<BATCHEDCALLPRIMITIVE> primitives;

	// used for hash table
	U32 state_id;
	BATCHEDMATERIAL *next, *prev;

public: // Interface

	bool is_equal( BATCHEDMATERIAL *material )
	{
		if( flags != material->flags ) {
			return false;
		}

		if( flags & BM_M_F_HAS_BLEND ) {
			if( blend_enable != material->blend_enable ) {
				return false;
			}
			if( blend_enable && ((blend_src != material->blend_src) || (blend_dst != material->blend_dst)) ) {
				return false;					
			}
		}

		if( flags & BM_M_F_HAS_ATEST ) {
			if( atest_enable != material->atest_enable ) {
				return false;
			}
			if( atest_ref != material->atest_ref ) {
				return false;
			}
			if( atest_func != material->atest_func ) {
				return false;
			}
		}

		if( flags & BM_M_F_HAS_TEXTURE ) {

			if( texture_stage_cnt != material->texture_stage_cnt ) {
				return false;
			}

			for( U32 ts=0; ts<texture_stage_cnt; ts++ ) {

				if( texture_stage_texture[ts] != material->texture_stage_texture[ts] ) {
					return false;
				}

				// COLOROP is 1, zero is invalid;
				for( U32 tss=1; tss<D3DTSS_MAX_STATE; tss++ ) {
					if( texture_stage_state[ts][tss] != material->texture_stage_state[ts][tss] ) {
						return false;
					}
				}
			} 
		}

		if( (flags & BM_M_F_HAS_DEPTH) && ((depth_enable != material->depth_enable) || (depth_write_enable != material->depth_write_enable) || (depth_func != material->depth_func)) ) {
			return false;
		}

		if( (flags & BM_M_F_HAS_CULL) && (cull_mode != material->cull_mode) ) {
			return false;
		}

		if( (flags & BM_M_F_HAS_DITHER) && (dither_enable != material->dither_enable) ) {
			return false;
		}	

		if( flags & BM_M_F_HAS_FOG ) {
			if( fog_enable != material->fog_enable ) {
				return false;
			}

			if( fog_enable && ((fog_start != material->fog_start) || (fog_end != material->fog_end) || (fog_density != material->fog_density) || (fog_mode != material->fog_mode) || (fog_color != material->fog_color)) ) {
				return false;
			}
		}

		return true;
	}

	//

	void save( IRenderPipeline *renderpipe, U32 save_flags )
	{
		flags = save_flags;

		if( flags & BM_M_F_HAS_BLEND ) {
			renderpipe->get_render_state( D3DRS_ALPHABLENDENABLE,		&blend_enable );
			if( blend_enable ) {												
				renderpipe->get_render_state( D3DRS_SRCBLEND,			&blend_src );
				renderpipe->get_render_state( D3DRS_DESTBLEND,			&blend_dst );
			}																	
		}							
		
		if( flags & BM_M_F_HAS_ATEST ) { 
			renderpipe->get_render_state( D3DRS_ALPHATESTENABLE,		&atest_enable );
			if( atest_enable ) {
				renderpipe->get_render_state( D3DRS_ALPHAREF,			&atest_ref );
				renderpipe->get_render_state( D3DRS_ALPHAFUNC,			&atest_func );
			}
		}
		if( flags & BM_M_F_HAS_DEPTH ) {										
			renderpipe->get_render_state( D3DRS_ZENABLE,				&depth_enable );
			renderpipe->get_render_state( D3DRS_ZWRITEENABLE,			&depth_write_enable );
			renderpipe->get_render_state( D3DRS_ZFUNC,					&depth_func );
		}																		
																				
		if( flags & BM_M_F_HAS_CULL ) {											
			renderpipe->get_render_state( D3DRS_CULLMODE,				&cull_mode );
		}																		
																				
		if( flags & BM_M_F_HAS_DITHER ) {										
			renderpipe->get_render_state( D3DRS_DITHERENABLE,			&dither_enable );
		}																		
																				
		if( flags & BM_M_F_HAS_FOG ) {											
			renderpipe->get_render_state( D3DRS_FOGENABLE,				&fog_enable );
			if( fog_enable ) {
				renderpipe->get_render_state( D3DRS_FOGSTART,		&fog_start );
				renderpipe->get_render_state( D3DRS_FOGEND,		&fog_end );
				renderpipe->get_render_state( D3DRS_FOGDENSITY,	&fog_density );
				renderpipe->get_render_state( D3DRS_FOGTABLEMODE,		&fog_mode );
				renderpipe->get_render_state( D3DRS_FOGCOLOR,			&fog_color );
			}
		}

		if( flags & BM_M_F_HAS_TEXTURE ) {
			texture_stage_cnt = 0;
			while( texture_stage_cnt < BM_NUM_STAGES ) {
				
				renderpipe->get_texture_stage_state( texture_stage_cnt, D3DTSS_COLOROP, &texture_stage_state[texture_stage_cnt][D3DTSS_COLOROP] );
				
				if( texture_stage_state[texture_stage_cnt][D3DTSS_COLOROP] == D3DTOP_DISABLE ) {
					
					texture_stage_texture[texture_stage_cnt] = 0;
					texture_stage_state[texture_stage_cnt][D3DTSS_ALPHAOP] = D3DTOP_DISABLE ;

					texture_stage_cnt++;

					break;
				}

				renderpipe->get_texture_stage_texture( texture_stage_cnt, &texture_stage_texture[texture_stage_cnt] );
				
				// start at 2, COLOROP is 1, zero is invalid;
				for( U32 tss=2; tss<D3DTSS_MAX_STATE; tss++ ) {
					renderpipe->get_texture_stage_state( texture_stage_cnt, (D3DTEXTURESTAGESTATETYPE)tss, &texture_stage_state[texture_stage_cnt][tss] );
				}

				texture_stage_cnt++;
			} 
/*	-- PLB
			//disable remaining states
			int t = texture_stage_cnt;
			while (t < BM_NUM_STAGES)
			{
				texture_stage_state[t][D3DTSS_COLOROP] = D3DTOP_DISABLE;
				t++;
			}
*/
		}
	}

	//

	void restore( IRenderPipeline *renderpipe, U32 restore_flags )
	{
		U32 rflags = flags & restore_flags;

		if( rflags & BM_M_F_HAS_BLEND ) {
			if( rflags & BM_M_F_HAS_BLEND_ENABLE ) {
				renderpipe->set_render_state( D3DRS_ALPHABLENDENABLE,	blend_enable );
			}
			if( rflags & BM_M_F_HAS_BLEND_FUNC ) {
				if( blend_enable ) {											
					renderpipe->set_render_state( D3DRS_SRCBLEND,		blend_src );
					renderpipe->set_render_state( D3DRS_DESTBLEND,		blend_dst );
				}																	
			}
		}																		
																				
		if( rflags & BM_M_F_HAS_DEPTH ) {										
			if( rflags & BM_M_F_HAS_DEPTH_ENABLE ) {
				renderpipe->set_render_state( D3DRS_ZENABLE,			depth_enable );
				renderpipe->set_render_state( D3DRS_ZWRITEENABLE,		depth_write_enable );
			}
			if( rflags & BM_M_F_HAS_DEPTH_FUNC ) {
				renderpipe->set_render_state( D3DRS_ZFUNC,				depth_func );
			}
		}																		

		if( rflags & BM_M_F_HAS_ATEST ) {
			renderpipe->set_render_state( D3DRS_ALPHATESTENABLE,		atest_enable );
			if( atest_enable ) {
				renderpipe->set_render_state( D3DRS_ALPHAREF,			atest_ref );
				renderpipe->set_render_state( D3DRS_ALPHAFUNC,			atest_func );
			}
		}
		if( rflags & BM_M_F_HAS_CULL ) {											
			renderpipe->set_render_state( D3DRS_CULLMODE,				cull_mode );
		}																		
																				
		if( rflags & BM_M_F_HAS_DITHER ) {										
			renderpipe->set_render_state( D3DRS_DITHERENABLE,			dither_enable );
		}																		
																				
		if( rflags & BM_M_F_HAS_FOG ) {											
			renderpipe->set_render_state( D3DRS_FOGENABLE,				fog_enable );
			if( fog_enable ) {
				renderpipe->set_render_state( D3DRS_FOGSTART,			fog_start );
				renderpipe->set_render_state( D3DRS_FOGEND,			fog_end );
				renderpipe->set_render_state( D3DRS_FOGDENSITY,		fog_density );
				renderpipe->set_render_state( D3DRS_FOGTABLEMODE,		fog_mode );
				renderpipe->set_render_state( D3DRS_FOGCOLOR,			fog_color );
			}
		}

		if( rflags & BM_M_F_HAS_TEXTURE ) {
			//must set ALL stages to at least disable COLOROP - rmarr
			//i don't think so, i have never seen this before - pbleisch
			//
			for( U32 ts=0; ts<texture_stage_cnt; ts++ ) {
				
				renderpipe->set_texture_stage_state( ts, D3DTSS_COLOROP, texture_stage_state[ts][D3DTSS_COLOROP] );
				renderpipe->set_texture_stage_state( ts, D3DTSS_ALPHAOP, texture_stage_state[ts][D3DTSS_ALPHAOP] );

				renderpipe->set_texture_stage_texture( ts, texture_stage_texture[ts] );

				if( texture_stage_state[ts][D3DTSS_COLOROP] == D3DTOP_DISABLE ) {
					break;
				}
		
				// start at 2, COLOROP is 1, zero is invalid;
				for( U32 tss=2; tss<D3DTSS_MAX_STATE; tss++ ) {
					renderpipe->set_texture_stage_state( ts, (D3DTEXTURESTAGESTATETYPE)tss, texture_stage_state[ts][tss] );
				}
			} 
		}
	}

	//

	void reset_primitives()
	{
		primitives.reset();
	}

	//
};

//

struct PRIMITIVESORTDATA
{
	BATCHEDMATERIAL		 *material;
	BATCHEDCALLPRIMITIVE *primitive;
};

//

static int psd_compare( const void *elem1, const void *elem2 )
{
	PRIMITIVESORTDATA* p1 = (PRIMITIVESORTDATA*) elem1;
	PRIMITIVESORTDATA* p2 = (PRIMITIVESORTDATA*) elem2;

	int result = 0;
	if( p1->primitive->depth() < p2->primitive->depth() ) {
		result = -1;
	}
	else if (p1->primitive->depth() > p2->primitive->depth() ) {
		result = 1;
	}
	else if( p1->material->stamp < p2->material->stamp ) {
		result = -1;
	}
	else if( p1->material->stamp > p2->material->stamp ) {
		result = 1;
	}

//	static char buffer[1024];
//	sprintf( buffer, "psd_compare: %d op %d == %d (%s)\n", p1->material->depth_func, p2->material->depth_func, result, (abs(result)==2)? "stamp" : "depth" );
//	GENERAL_TRACE_1( buffer );

	return result;
}

//

#define BM_P_F_ENABLE	(1<<0)
#define BM_P_F_EMPTY	(1<<1)

//

#define MH_MAX_BUCKETS 121

//

struct BMPOOL 
{
	BATCHEDMATERIAL *materials;				// Saved material/primitive state.
	U32				 materials_cnt;			// Number of materials in materials array
	U32				 materials_max_cnt;		// Max number of materials used in a frame so far
	U32				 materials_used_cnt;	// Number of materials used this frame
	
	LList<BATCHEDMATERIAL> materials_hash[MH_MAX_BUCKETS];

	unsigned char	*pool;
	U32				 pool_used_cnt;			// index
	U32				 pool_size;
	U32				 pool_flags;

	const char		*name;
	
	//

	U32 pc_save ;
	U32 pc_render ;
	U32 pc_restore ;

	U32 material_stamp;

public:	// Interface

	BMPOOL( )
	{
		name = "";

		material_stamp = 0;

		materials_used_cnt = 0;
		materials_cnt = 0;
		materials = NULL;

		set_materials_size( 128 );

		pool = NULL;
		pool_used_cnt = 0;
		pool_size = 0;
		pool_flags = BM_P_F_EMPTY;

		pc_render = 0;
		pc_save = 0;
		pc_restore = 0;
	}

	//

	~BMPOOL()
	{
		delete[] materials;
		materials = NULL;
		materials_cnt = 0;

		delete[] pool;
		pool = NULL;
	}

	//

	void set_name( const char *_name ) 
	{
		name = _name;
	}

	//

	void set_size( U32 new_pool_size ) 
	{
		delete[] pool;
		pool_used_cnt = 0;
		pool_size = new_pool_size;
		pool = new unsigned char[pool_size];
		pool_flags |= BM_P_F_EMPTY;

		update();
	}

	//

	void set_materials_size( U32 new_materials_size ) 
	{
		delete[] materials;
		materials_max_cnt = new_materials_size;
		materials = new BATCHEDMATERIAL[materials_max_cnt];
	}

	//

	void set_enabled( bool enabled )
	{
		if( enabled ) {
			pool_flags |= BM_P_F_ENABLE;
		}
		else {
			pool_flags &= ~(BM_P_F_ENABLE);
		}
	}

	//

	bool is_enabled( )
	{
		return (pool_flags & BM_P_F_ENABLE)? true : false;
	}

	//

	bool is_empty( )
	{
		return (pool_flags & BM_P_F_EMPTY)? true : false;
	}

	//

	U32 materials_hash_val( U32 state_id )
	{
		return state_id % MH_MAX_BUCKETS;
	}

	// 

	void materials_hash_reset( )
	{
		for( U32 b=0; b<MH_MAX_BUCKETS; b++ ) {
			materials_hash[b].reset();
		}
	}

	//

	void materials_hash_link( BATCHEDMATERIAL *material )
	{
		U32 hash = materials_hash_val( material->state_id );
		materials_hash[hash].link( material );
	}

	//

	void materials_hash_unlink( BATCHEDMATERIAL *material )
	{
		U32 hash = materials_hash_val( material->state_id );
		materials_hash[hash].unlink( material );
	}

	//

	void *allocate( U32 size, bool set_non_empty=false )
	{
		if( (pool_used_cnt+size) > pool_size ) {
			GENERAL_WARNING( TEMPSTR( "Pool size exceeded in pool '%s'\n", name ) );
			return NULL;
		}

		void *ret = &pool[pool_used_cnt];
		pool_used_cnt += size;
		if( set_non_empty ) {
			pool_flags &= ~(BM_P_F_EMPTY);
		}

		return ret;
	}

	//

	void free_all()
	{
		pool_used_cnt = 0;
		pool_flags |= BM_P_F_EMPTY;
	}

	//

	void update( void )
	{
		materials_max_cnt = __max( materials_max_cnt, materials_used_cnt );

//		if( materials_used_cnt || pc_render || pc_save || pc_restore ) {
//			GENERAL_TRACE_1( TEMPSTR( "RB: materials:%d/%d/%d  pool:%d/%d  d:%d  s:%d  r:%d  :%s\n", materials_used_cnt, materials_max_cnt, materials_cnt, pool_used_cnt, pool_size, pc_render, pc_save, pc_restore, name ) ) ;
//		}

		if( materials_cnt < materials_max_cnt ) {
			delete[] materials;
			materials = new BATCHEDMATERIAL[materials_max_cnt];
			materials_cnt = materials_max_cnt;
		}

		ASSERT( materials );

		materials_used_cnt = 0;

		materials_hash_reset();

		free_all();

		pc_render = pc_save = pc_restore = 0;
	}

	//

	BATCHEDMATERIAL *find_material( U32 state_id )
	{
		BATCHEDMATERIAL *material = NULL;
		
#if 0
		if( state_id ) {
			U32 hash = materials_hash_val( state_id );
			material = materials_hash[hash].first();
			while( material ) {
				if( material->state_id == state_id ) {
					return material;
				}
				material = material->next;
			}
		}
#endif

		return material;
	}

	//

	BATCHEDMATERIAL *allocate_material( U32 state_id, U32 stamp )
	{

		ASSERT( (materials_used_cnt) < materials_cnt );

		BATCHEDMATERIAL *material = &materials[materials_used_cnt];
		
		materials_used_cnt++;

		material->stamp = stamp;
		material->flags = 0;
		material->next = NULL;
		material->prev = NULL;
		material->state_id = state_id;
		
		materials_hash_link( material );
		
		return material;
	}

	//

	GENRESULT add_primitive( IRenderPipeline *renderpipe, U32 state_id, U32 save_flags, D3DPRIMITIVETYPE type, U32 vertex_format, const void *verts, int num_verts, const U16 * indices, int num_indices, U32 flags )
	{
		if( num_verts == 0 ) {
			return GR_GENERIC;
		}

		BATCHEDMATERIAL *material;
		bool material_save = false;

		if( !state_id || (material = find_material( state_id )) == NULL ) {
			if( (material = allocate_material( state_id, material_stamp++ )) == NULL ) {
				return GR_GENERIC;
			}

			material->save( renderpipe, save_flags );
			pc_save++;
		}

		U32 sizeof_vert = FVF_SIZEOF_VERT(vertex_format);
		U8 *prim_mem;
		
		if( (prim_mem = (U8*)allocate( sizeof(BATCHEDCALLPRIMITIVE) + sizeof_vert * num_verts + sizeof(U16) * num_indices, true )) == NULL ) {
			return GR_GENERIC;
		}

		BATCHEDCALLPRIMITIVE *prim = (BATCHEDCALLPRIMITIVE*)prim_mem;
		prim_mem += sizeof(BATCHEDCALLPRIMITIVE);

		prim->initialize();
		
		prim->verts = prim_mem;
		prim_mem += sizeof_vert * num_verts;

		prim->verts_cnt = num_verts;
		memcpy( prim->verts, verts, sizeof_vert * num_verts );

		prim->indices_cnt = num_indices;
		if( indices && num_indices ) {
			prim->indices = (U16 *)prim_mem;
			prim_mem += sizeof(U16) * num_indices;
			
			memcpy( prim->indices, indices, sizeof(U16) * num_indices );
		}

		renderpipe->get_modelview( prim->modelview );
		
		prim->p_type = type;
		prim->p_flags = flags;
		prim->verts_type = vertex_format;

		material->primitives.link( prim );

		return GR_OK;
	}

	//

	void render_no_sort( IRenderPipeline *renderpipe )
	{
		BATCHEDMATERIAL *material = materials;
		for( U32 idx=0; idx<materials_used_cnt; idx++, material++ ) {

			material->restore( renderpipe, ~(0) );
			pc_restore++;

			BATCHEDCALLPRIMITIVE *prim = material->primitives.first();
			while( prim ) {
				pc_render++;
				prim->render( renderpipe );
				prim = prim->next;
			}

			material->reset_primitives();
		}
	}

	//

	void render_depth_sort( IRenderPipeline *renderpipe, DynamicArray<PRIMITIVESORTDATA> &sort_pool )
	{

		//render_no_sort( renderpipe );
		//return;

		if( materials_used_cnt == 0 ) {
			return;
		}

		BATCHEDMATERIAL *material;
		BATCHEDCALLPRIMITIVE *prim;
		U32 idx;


		// Collect material/primitive information and sort data
		// in each primitive. i.e. do per-triangle sort.
		//
		if( materials_used_cnt > 1 ) {

			U32 sort_pool_used_cnt = 0;
			material = materials;
			for( idx=0; idx<materials_used_cnt; idx++, material++ ) {

				prim = material->primitives.first();
				while( prim ) {

					prim->sort();

					sort_pool[sort_pool_used_cnt].material = material;
					sort_pool[sort_pool_used_cnt].primitive = prim;
					sort_pool_used_cnt++;

					if( material->depth_enable == false ) {
						prim->p_depth = 0;
					}

					prim = prim->next;
				}
			}

			if( sort_pool_used_cnt == 0 ) {
				GENERAL_TRACE_1( "Materials, but no primitives\n" );
				return;
			}

			// Sort all material/primitive info.  i.e. do per-object sort
			//
			qsort( &(sort_pool[0]), sort_pool_used_cnt, sizeof(PRIMITIVESORTDATA), psd_compare );

			// Render in sorted order, render all primitives in a material
			// before continuing.  Once a material is rendered, clear the
			// primitives so we don't render it again.
			//
			renderpipe->set_render_state( D3DRS_ALPHABLENDENABLE, TRUE );
			renderpipe->set_render_state( D3DRS_ZWRITEENABLE, FALSE );
			renderpipe->set_render_state( D3DRS_ZENABLE, TRUE );
		
			for( idx=0; idx<sort_pool_used_cnt; idx++ ) {
				
				material = sort_pool[idx].material;

				material->restore( renderpipe, ~(BM_M_F_HAS_BLEND_ENABLE) );
				pc_restore++;

				BATCHEDCALLPRIMITIVE *prim = material->primitives.first();
				while( prim ) {
					pc_render++;
					prim->render( renderpipe );
					prim = prim->next;
				}

				material->reset_primitives();
			} 
		}
		else {
			renderpipe->set_render_state( D3DRS_ALPHABLENDENABLE, TRUE );
			renderpipe->set_render_state( D3DRS_ZWRITEENABLE, FALSE );
			renderpipe->set_render_state( D3DRS_ZENABLE, TRUE );
		
			materials->restore(  renderpipe, ~(BM_M_F_HAS_BLEND_ENABLE) );
			pc_restore++;

			BATCHEDCALLPRIMITIVE *prim = materials->primitives.first();
			while( prim ) {
				pc_render++;
				prim->render( renderpipe );
				prim = prim->next;
			}

			materials->reset_primitives();
		}
	}

};






//

// Macro to define supported methods on this interface
//
#undef DA_METHOD
#define DA_METHOD(name,params) virtual GENRESULT COMAPI name params;

// --------------------------------------------------------------------------
//
// RenderBatcher
//
//

struct RenderBatcher : 	IRenderPrimitive,
						IAggregateComponent

{
	BEGIN_DACOM_MAP_INBOUND(RenderBatcher)
	DACOM_INTERFACE_ENTRY(IRenderPrimitive)
	DACOM_INTERFACE_ENTRY2(IID_IRenderPrimitive,IRenderPrimitive)
	DACOM_INTERFACE_ENTRY(IAggregateComponent)
	DACOM_INTERFACE_ENTRY2(IID_IAggregateComponent,IAggregateComponent)
	END_DACOM_MAP()

public:		// public interface
    
	// IAggregateComponent 
	GENRESULT COMAPI Initialize(void);
	GENRESULT init(AGGDESC *desc);

	// IRenderPrimitive
	GENRESULT COMAPI set_state( RPRSTATE state, U32 value ) ;
	GENRESULT COMAPI get_state( RPRSTATE state, U32 *value ) ;
	GENRESULT COMAPI set_modelview( const Transform & modelview ) ;
	GENRESULT COMAPI get_modelview( Transform & modelview ) ;
	GENRESULT COMAPI set_viewport( int x, int y, int w, int h ) ;
	GENRESULT COMAPI get_viewport( int *x, int *y, int *w, int *h ) ;
	GENRESULT COMAPI set_ortho( float left, float right, float bottom, float top, float nearval=-1.0, float farval=1.0 ) ;
	GENRESULT COMAPI set_perspective( float fovy, float aspect, float znear, float zfar ) ;
	GENRESULT COMAPI set_projection( const Matrix4 &projection ) ;
	GENRESULT COMAPI get_projection( Matrix4 &projection ) ;
	GENRESULT COMAPI set_material( D3DMATERIAL9 *material_values ) ;
	GENRESULT COMAPI get_material( D3DMATERIAL9 *out_material_values ) ;
	GENRESULT COMAPI begin_scene( void ) ;
	GENRESULT COMAPI end_scene( void ) ;
	GENRESULT COMAPI set_render_state( D3DRENDERSTATETYPE state, U32 value ) ;
	GENRESULT COMAPI get_render_state( D3DRENDERSTATETYPE state, U32 *value ) ;
	GENRESULT COMAPI set_texture_stage_state( U32 stage, D3DTEXTURESTAGESTATETYPE, U32 value ) ;
	GENRESULT COMAPI get_texture_stage_state( U32 stage, D3DTEXTURESTAGESTATETYPE, U32 *value ) ;
	GENRESULT COMAPI set_sampler_state( U32 stage, D3DSAMPLERSTATETYPE, U32 value ) ;
	GENRESULT COMAPI get_sampler_state( U32 stage, D3DSAMPLERSTATETYPE, U32 *value ) ;
	GENRESULT COMAPI set_texture_stage_texture( U32 stage, U32 htexture ) ;
	GENRESULT COMAPI get_texture_stage_texture( U32 stage, U32 *htexture ) ;
	GENRESULT COMAPI verify_state(  void ) ;
	GENRESULT COMAPI draw_primitive( D3DPRIMITIVETYPE type, U32 vertex_format, const void *verts, int num_verts, U32 flags ) ;
	GENRESULT COMAPI draw_indexed_primitive( D3DPRIMITIVETYPE type, U32 vertex_format, const void *verts, int num_verts, const U16 * indices, int num_indices, U32 flags ) ;
	GENRESULT COMAPI draw_primitive_vb( D3DPRIMITIVETYPE type, IRP_VERTEXBUFFERHANDLE vbhandle, int start_vert, int num_verts, U32 flags  ) ;
	GENRESULT COMAPI draw_indexed_primitive_vb( D3DPRIMITIVETYPE type, IRP_VERTEXBUFFERHANDLE vbhandle, int start_vert, int num_verts, const U16 * indices, int num_indices, U32 flags  ) ;
	GENRESULT COMAPI flush( U32 rprenum_f_flags ) ;

	RenderBatcher(void);
	~RenderBatcher(void);

	DA_DEFINE_NEW_OPERATOR(RenderBatcher);


protected:	// protected interface
//	BOOL		bm_initialize();
//	void		bm_cleanup();
//	void		bm_render_unsorted( SortedList &list );
//	void		bm_render_zsorted( SortedList &list );
//	BOOL		bm_set_pool_size( U32 pool, U32 val );
//	GENRESULT	bm_batch_prim( U32 pool, D3DPRIMITIVETYPE type, U32 vertex_format, const void *verts, int num_verts, const U16 * indices, int num_indices, U32 flags, U32 cnt );

protected:	// protected data
	IRenderPipeline			*render_pipe;
	
	DynamicArray<PRIMITIVESORTDATA> prim_sort_pool;

	U32						state[RPR_MAX_STATE];

#define BM_P_OPAQUE			0
#define BM_P_ALPHA_DEPTH	1
#define BM_P_ALPHA_NOSORT	2
	BMPOOL					bm_pools[3];		
};


// Macro to define supported methods on this interface
//
#undef DA_METHOD
#undef DA_COMPONENT_NAME

#define DA_COMPONENT_NAME RenderBatcher
#define DA_METHOD(name,params) GENRESULT COMAPI DA_COMPONENT_NAME :: name params

//

DA_METHOD(	set_state,(RPRSTATE _state, U32 value ))
{
	if( state[_state] != value ) {
		switch( _state ) {

		case RPR_BATCH:
			break;

		case RPR_BATCH_POOLS:
			bm_pools[BM_P_OPAQUE].set_enabled( (value & RPR_OPAQUE) );
			bm_pools[BM_P_ALPHA_DEPTH].set_enabled( (value & RPR_TRANSLUCENT_DEPTH_SORTED) != 0 );
			bm_pools[BM_P_ALPHA_NOSORT].set_enabled( (value & RPR_TRANSLUCENT_UNSORTED) != 0  );
			break;

		case RPR_BATCH_OPAQUE_POOL:
			bm_pools[BM_P_OPAQUE].set_size( value );
			break;

		case RPR_BATCH_TRANSLUCENT_POOL:
			bm_pools[BM_P_ALPHA_DEPTH].set_size( value );
			break;

		case RPR_BATCH_TRANSLUCENT_NONZ_POOL:
			bm_pools[BM_P_ALPHA_NOSORT].set_size( value );
			break;
		}
		state[_state] = value;
	}

	return GR_OK;
}

//

DA_METHOD(	get_state,(RPRSTATE _state, U32 *value ))
{
	*value = state[_state];
	return GR_OK;
}

//

DA_METHOD(	set_modelview,(const Transform & modelview ))
{
	return render_pipe->set_modelview( modelview );
}

//

DA_METHOD(	get_modelview,(Transform & modelview ))
{
	return render_pipe->get_modelview( modelview );
}

//

DA_METHOD(	set_viewport,(int x, int y, int w, int h ))
{
	return render_pipe->set_viewport( x,y,w,h );
}

//

DA_METHOD(	get_viewport,(int *x, int *y, int *w, int *h ))
{
	return render_pipe->get_viewport( x, y, w, h );
}

//

DA_METHOD(	set_ortho,(float left, float right, float bottom, float top, float nearval, float farval ))
{
	return render_pipe->set_ortho( left, right, bottom, top, nearval, farval );
}

//

DA_METHOD(	set_perspective,(float fovy, float aspect, float nearval, float farval ) )
{
	return render_pipe->set_perspective( fovy, aspect, nearval, farval );
}

//

DA_METHOD(  set_projection,(const Matrix4 &projection ))
{
	return render_pipe->set_projection( projection );
}

//

DA_METHOD(  get_projection,(Matrix4 &projection ))
{
	return render_pipe->get_projection( projection );
}

//

GENRESULT RenderBatcher::set_material( D3DMATERIAL9 *material_values ) 
{
	return render_pipe->set_material( material_values );
}

//

GENRESULT RenderBatcher::get_material( D3DMATERIAL9 *out_material_values ) 
{
	return render_pipe->get_material( out_material_values );
}

//

DA_METHOD(	begin_scene,(void ))
{
	return render_pipe->begin_scene();
}

//

DA_METHOD(	end_scene,(void ))
{
	return render_pipe->end_scene();
}

//

DA_METHOD(	set_render_state,(D3DRENDERSTATETYPE _state, U32 value ))
{
	return render_pipe->set_render_state( _state, value );
}

//

DA_METHOD(	get_render_state,(D3DRENDERSTATETYPE _state, U32 *value ))
{
	return render_pipe->get_render_state( _state, value );
}

//

DA_METHOD(  set_texture_stage_state,(U32 stage, D3DTEXTURESTAGESTATETYPE tss, U32 value ))
{
	return render_pipe->set_texture_stage_state( stage, tss, value );
}

DA_METHOD(  set_sampler_state,(U32 stage, D3DSAMPLERSTATETYPE ss, U32 value ))
{
	return render_pipe->set_sampler_state( stage, ss, value );
}


//

DA_METHOD(  get_texture_stage_state,(U32 stage, D3DTEXTURESTAGESTATETYPE tss, U32 *value ))
{
	return render_pipe->get_texture_stage_state( stage, tss, value );
}

DA_METHOD(  get_sampler_state,(U32 stage, D3DSAMPLERSTATETYPE ss, U32 *value ))
{
	return render_pipe->get_sampler_state( stage, ss, value );
}

//

DA_METHOD(  set_texture_stage_texture,(U32 stage, U32 htexture ))
{
	return render_pipe->set_texture_stage_texture( stage, htexture );
}

//

DA_METHOD(  get_texture_stage_texture,(U32 stage, U32 *htexture ))
{
	return render_pipe->get_texture_stage_texture( stage, htexture );
}

//

DA_METHOD(	verify_state,())
{
	return render_pipe->verify_state( );
}

//

DA_METHOD(	draw_primitive,(D3DPRIMITIVETYPE type, U32 vertex_format, const void *verts, int num_verts, U32 flags ))
{
	if( num_verts == 0 ) {
		return GR_GENERIC;
	}

	if( state[RPR_BATCH] ) {
		U32 alpha;
		render_pipe->get_render_state( D3DRS_ALPHABLENDENABLE, &alpha );
		U32 depth = (state[RPR_BATCH_TRANSLUCENT_MODE]==RPR_TRANSLUCENT_UNSORTED)?0:1;
		U32 pool  = (alpha)?(depth)?1:2:0; 
		if( bm_pools[pool].is_enabled() ) {
			return bm_pools[pool].add_primitive( render_pipe, state[RPR_STATE_ID], state[RPR_STATE_FLAGS], type, vertex_format, verts, num_verts, NULL, 0, flags );
		}
	}

	return render_pipe->draw_primitive( type, vertex_format, verts, num_verts, flags );
}

//

DA_METHOD(	draw_indexed_primitive,(D3DPRIMITIVETYPE type, U32 vertex_format, const void *verts, int num_verts, const U16 * indices, int num_indices, U32 flags ))
{
	if( num_verts == 0 || num_indices == 0 ) {
		return GR_GENERIC;
	}

	if( state[RPR_BATCH] ) {
		U32 alpha;
		render_pipe->get_render_state( D3DRS_ALPHABLENDENABLE, &alpha );
		U32 depth = (state[RPR_BATCH_TRANSLUCENT_MODE]==RPR_TRANSLUCENT_UNSORTED)?0:1;
		U32 pool  = (alpha)?(depth)?1:2:0; 
		if( bm_pools[pool].is_enabled() ) {
			return bm_pools[pool].add_primitive( render_pipe, state[RPR_STATE_ID], state[RPR_STATE_FLAGS], type, vertex_format, verts, num_verts, indices, num_indices, flags );
		}
	}

	return render_pipe->draw_indexed_primitive( type, vertex_format, verts, num_verts, indices, num_indices, flags );
}

//

GENRESULT RenderBatcher::draw_primitive_vb( D3DPRIMITIVETYPE type, IRP_VERTEXBUFFERHANDLE vbhandle, int start_vert, int num_verts, U32 flags  ) 
{
	if( num_verts == 0 ) {
		return GR_GENERIC;
	}

	if( state[RPR_BATCH] ) {
#if 1
		GENERAL_TRACE_1( "RenderBatcher: dip_vb is unsupported\n" );
#else
		U32 alpha;
		render_pipe->get_render_state( D3DRS_ALPHABLENDENABLE, &alpha );
		U32 depth = (state[RPR_BATCH_TRANSLUCENT_MODE]==RPR_TRANSLUCENT_UNSORTED)?0:1;
		U32 pool  = (alpha)?(depth)?1:2:0; 
		if( bm_pools[pool].is_enabled() ) {
			return bm_pools[pool].add_primitive( render_pipe, state[RPR_STATE_ID], state[RPR_STATE_FLAGS], type, vertex_format, verts, num_verts, NULL, 0, flags );
		}
#endif
	}

	return render_pipe->draw_primitive_vb( type, vbhandle, start_vert, num_verts, flags );
}

//

GENRESULT RenderBatcher::draw_indexed_primitive_vb( D3DPRIMITIVETYPE type, IRP_VERTEXBUFFERHANDLE vbhandle, int start_vert, int num_verts, const U16 * indices, int num_indices, U32 flags  ) 
{
	if( num_verts == 0 || num_indices == 0 ) {
		return GR_GENERIC;
	}

	if( state[RPR_BATCH] ) {
#if 1
		GENERAL_TRACE_1( "RenderBatcher: dip_vb is unsupported\n" );
#else
		U32 alpha;
		render_pipe->get_render_state( D3DRS_ALPHABLENDENABLE, &alpha );
		U32 depth = (state[RPR_BATCH_TRANSLUCENT_MODE]==RPR_TRANSLUCENT_UNSORTED)?0:1;
		U32 pool  = (alpha)?(depth)?1:2:0; 
		if( bm_pools[pool].is_enabled() ) {
			return bm_pools[pool].add_primitive( render_pipe, state[RPR_STATE_ID], state[RPR_STATE_FLAGS], type, vertex_format, verts, num_verts, indices, num_indices, flags );
		}
#endif
	}

	return render_pipe->draw_indexed_primitive_vb( type, vbhandle, start_vert, num_verts, indices, num_indices, flags );
}

//

DA_METHOD(	flush,(DWORD flags ))
{

	if( !flags || (bm_pools[0].is_empty() && bm_pools[1].is_empty() && bm_pools[2].is_empty() ) ) {
		return GR_GENERIC;
	}

	if( !bm_pools[0].is_empty() && (flags & RPR_OPAQUE) ) {
		bm_pools[0].render_no_sort( render_pipe );
		bm_pools[0].update();
	}

	if( flags & (RPR_TRANSLUCENT_DEPTH_SORTED|RPR_TRANSLUCENT_UNSORTED) ) {

		// treat two alpha pools as one and render them as requested
		// (this mimics the original batching mechanism)
		//
		if( flags & RPR_TRANSLUCENT_DEPTH_SORTED ) {
			if( !bm_pools[2].is_empty() ) {
				bm_pools[2].render_depth_sort( render_pipe, prim_sort_pool );
			}
			if( !bm_pools[1].is_empty() ) {
				bm_pools[1].render_depth_sort( render_pipe, prim_sort_pool );
			}
		}
		else {
			if( !bm_pools[1].is_empty() ) {
				bm_pools[1].render_no_sort( render_pipe );
			}
			if( !bm_pools[2].is_empty() ) {
				bm_pools[2].render_no_sort( render_pipe );
			}
		}
		
		bm_pools[2].update();
		bm_pools[1].update();
	}
	else if( flags & (RPR_TRANSLUCENT_DEPTH_SORTED_ONLY|RPR_TRANSLUCENT_UNSORTED_ONLY) ) {

		// render two alpha pools as seperate entities
		//
		if( !bm_pools[1].is_empty() && (flags & RPR_TRANSLUCENT_DEPTH_SORTED_ONLY) ) {
			bm_pools[1].render_depth_sort( render_pipe, prim_sort_pool );
			bm_pools[1].update();
		}
		if( !bm_pools[2].is_empty() && (flags & RPR_TRANSLUCENT_UNSORTED_ONLY) ) {
			bm_pools[2].render_no_sort( render_pipe );
			bm_pools[2].update();
		}
	}

	return GR_OK;
}

//

// ------------------------------------------------------------------
// Batching Related code
// ------------------------------------------------------------------


// ------------------------------------------------------------------
// Startup/shutdown Related Code
// ------------------------------------------------------------------

RenderBatcher::RenderBatcher(void)
{
	render_pipe = NULL;

	memset( state, 0xFF, sizeof(state) );


}

//

RenderBatcher::~RenderBatcher(void)
{
	render_pipe = NULL;
}

//

GENRESULT COMAPI RenderBatcher::Initialize(void)
{ 
	IAggregateComponent *IAC = static_cast<IAggregateComponent*>(this);

	if( FAILED( IAC->QueryInterface( IID_IRenderPipeline, (void**) &render_pipe ) ) ) {
		return GR_GENERIC;
	}

	render_pipe->Release();

	U32 value;

	ICOManager *DACOM = DACOM_Acquire();

	opt_get_u32( DACOM, NULL, "RenderBatcher", "MaterialsMaxCount", 128, &value );

	bm_pools[0].set_materials_size( value );
	bm_pools[1].set_materials_size( value );
	bm_pools[2].set_materials_size( value );

	bm_pools[0].set_name( "BM_P_OPAQUE" );
	bm_pools[1].set_name( "BM_P_ALPHA_DEPTH" );
	bm_pools[2].set_name( "BM_P_ALPHA_NOSORT" );
	
	
	set_state( RPR_BATCH,						opt_get_u32( DACOM, NULL, "RenderBatcher", "BATCH",							FALSE, &value ) );
	set_state( RPR_BATCH_POOLS,					opt_get_u32( DACOM, NULL, "RenderBatcher", "BATCH_POOLS",					RPR_TRANSLUCENT_DEPTH_SORTED|RPR_OPAQUE|RPR_TRANSLUCENT_UNSORTED, &value ) ); 
	set_state( RPR_BATCH_OPAQUE_POOL,			opt_get_u32( DACOM, NULL, "RenderBatcher", "BATCH_OPAQUE_POOL",				RPR_OPAQUE_POOL_SIZE_DEFAULT, &value ) ); 
	set_state( RPR_BATCH_TRANSLUCENT_POOL,		opt_get_u32( DACOM, NULL, "RenderBatcher", "BATCH_TRANSLUCENT_POOL",		RPR_TRANSLUCENT_POOL_SIZE_DEFAULT, &value ) );
	set_state( RPR_BATCH_TRANSLUCENT_NONZ_POOL,	opt_get_u32( DACOM, NULL, "RenderBatcher", "BATCH_TRANSLUCENT_NONZ_POOL",	RPR_TRANSLUCENT_POOL_SIZE_DEFAULT, &value ) );
	set_state( RPR_BATCH_OPAQUE_FLUSH,			opt_get_u32( DACOM, NULL, "RenderBatcher", "BATCH_OPAQUE_FLUSH",			0, &value ) ); 
	set_state( RPR_BATCH_TRANSLUCENT_MODE,		opt_get_u32( DACOM, NULL, "RenderBatcher", "BATCH_TRANSLUCENT_MODE",		RPR_TRANSLUCENT_DEPTH_SORTED, &value ) );
	set_state( RPR_STATE_FLAGS,					opt_get_u32( DACOM, NULL, "RenderBatcher", "STATE_FLAGS",					~(0), &value ) ); 
	set_state( RPR_STATE_ID,					0 );

	return GR_OK;
}

//

GENRESULT RenderBatcher::init(AGGDESC *desc)
{ 
	// This is called during normal use.  We are an aggregate.
	return GR_OK;
}

// ------------------------------------------------------------------
// DLL Related code
// ------------------------------------------------------------------

// linker bug
void main (void)
{
}

//--------------------------------------------------------------------------
// 
 
BOOL COMAPI DllMain (HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	switch (fdwReason)
	{
	//
	// DLL_PROCESS_ATTACH: Create object server component and register it with DACOM manager
	//
		case DLL_PROCESS_ATTACH:
		{
			DA_HEAP_ACQUIRE_HEAP(HEAP);
			DA_HEAP_DEFINE_HEAP_MESSAGE(hinstDLL);

			ICOManager *DACOM = DACOM_Acquire();
			IComponentFactory *server1;
			if( DACOM && (server1 = new DAComponentFactory2<DAComponentAggregate<RenderBatcher>, AGGDESC>("IRenderPrimitive")) != NULL ) {
				DACOM->RegisterComponent( server1, "IRenderPrimitive", DACOM_HIGH_PRIORITY );
				server1->Release();
			}
			break;
		}

	}

	return TRUE;
}


// EOF







#if 0
//
// RPList - poly batch list struct. Stores the equivalent
// of one glBegin()/glEnd() pair as an indexed vertex array.
//
struct RPList
{
	Transform		modelview;
	Matrix4			projection;
	DWORD			x, y, w, h;		// viewport settings.

	U32 state[D3DRS_MAX_STATE];

	D3DPRIMITIVETYPE	type;		// D3DPT_*
	unsigned int		num_verts;
	RPVertex	*		verts;
	U32					vert_type;

	unsigned int		num_indices;
	U16 *				indices;	// into vertex list.

	U32					flags;

	RPList(void)
	{
		init();
	}

	virtual ~RPList(void)
	{
	}

	void init(void)
	{
		type = D3DPT_POINTLIST;
		num_verts = 0;
		verts = NULL;
		num_indices = 0;
		indices = NULL;
		flags = 0;
	}

	inline bool is_indexed(void) const
	{
		return (num_indices != 0);
	}
};

//

struct ListNode
{
	const RPList *	list;
	ListNode *		prev;
	ListNode *		next;
};

//
// Ad hoc insertion-sorted list with no delete/unlink facility.
// Very specifically suited for what we need here.
//
struct SortedList
{
	int			max_entries;
	ListNode *	entries;
	ListNode *	used_entries;
	ListNode *	free_entries;

	SortedList::SortedList(void)
	{
		max_entries = 128;
		entries = new ListNode[max_entries];
		reset();
	}

	//

	SortedList::~SortedList(void)
	{
		delete[] entries;
		entries = NULL;
	}

	//

	void SortedList::reset(void)
	{
		used_entries = NULL;
		free_entries = entries;

		ListNode * node = free_entries;
		ListNode * prev = NULL;

		for (int i = 0; i < max_entries; i++, node++)
		{
			node->prev = prev;
			node->next = node + 1;
			prev = node;
		}

		prev->next = NULL;
	}

	// Unlinks entry from free list.

	ListNode *SortedList::get_free_entry(void)
	{
		ListNode * result;
		if( free_entries ) {
			result = free_entries;
			free_entries = result->next;
			if( free_entries ) {
				free_entries->prev = NULL;
			}
			else {
			// Out of free entries. Expand list.
				max_entries <<= 1;
				ListNode * new_entries = new ListNode[max_entries];
				ListNode * new_node = new_entries;

				ListNode * prev = NULL;
				ListNode * node = used_entries;
				while( node ) {
					new_node->list = node->list;
					new_node->prev = prev;
					new_node->next = new_node + 1;
					prev = new_node;

					node = node->next;
					new_node++;
				}

				free_entries = prev->next;
				prev->next = NULL;

				used_entries = new_entries;

				node = free_entries;
				for( int i = free_entries - new_entries; i < max_entries; i++, node++ ) {
					node->prev = prev;
					node->next = node + 1;
					prev = node;
				}

				prev->next = NULL;

				delete [] entries;
				entries = new_entries;

				result = free_entries;
				free_entries = result->next;

			}

			result->prev = result->next = NULL;
		}
		else {
			// Should never happen.
			result = NULL;
		}

		return result;
	}

	//

	void SortedList::insert(const RPList * list)
	{
		ListNode * node = get_free_entry();
		assert(node);

		const U32 txm_idx = BM_D3D_TO_IDX(D3DRS_TEXTUREHANDLE);

		node->list = list;

		if( used_entries ) {
			ListNode * prev = NULL;
			ListNode * curr = used_entries;

			while( curr && (curr->list->state[txm_idx] < list->state[txm_idx]) ) {
				prev = curr;
				curr = curr->next;
			}

		// Sort by type within texture.
			while( curr && (curr->list->state[txm_idx] == list->state[txm_idx]) && (curr->list->type < list->type) ) {
				prev = curr;
				curr = curr->next;
			}

		// Link it in.
			if( prev ) {
				prev->next = node;
			}
			else {
				used_entries = node;
			}
			node->prev = prev;
			node->next = curr;
			if( curr ) {
				curr->prev = node;
			}
		}
		else {
			// List is empty, place at head.
			used_entries = node;
		}
	}
};

#endif
/*

				float Mx = prim->modelview.d[0][2];
				float My = prim->modelview.d[1][2];
				float Mz = prim->modelview.d[2][2];
				float Tz = prim->modelview.d[2][3];

				U32 sizeof_vert = FVF_SIZEOF_VERT(prim->verts_type);

				int nverts = (prim->indices_cnt) ? prim->indices_cnt : prim->verts_cnt;
				
				int poly_prim_cnt = (nverts - primitive_type_info[prim->p_type].vert_fixed_cost) / 
									   primitive_type_info[prim->p_type].vert_per_prim_cost;

				U32 poly_start = 0;

				for( int poly=0; poly<poly_prim_cnt; poly++ ) {

					float depth_poly, depth_prim = -FLT_MAX;

					// Find depth in camera space.
					//
					if( prim->indices_cnt )	{
						
						U16 *idx = prim->indices + poly_start;

						for( U32 v=0; v<primitive_type_info[prim->p_type].vert_count; v++ ) {
							
							Vector *P = (Vector*)( ((char*)prim->verts) + (*idx)*sizeof_vert );
							
							if( (depth_poly = (Mx * P->x + My * P->y + Mz * P->z + Tz)) > depth_prim ) {
								depth_prim = depth_poly;
							}

							idx++;
						}
					}
					else {
						Vector *P = (Vector*)( ((char*)prim->verts) + poly_start*sizeof_vert );
						for( U32 v=0; v<primitive_type_info[prim->p_type].vert_count; v++ ) {

							if( (depth_poly = (Mx * P->x + My * P->y + Mz * P->z + Tz)) > depth_prim ) {
								depth_prim = depth_poly;
							}

							P = (Vector*)( ((char*)prim->verts) + (poly_start+v+1)*sizeof_vert );
						}
					}

					poly_sort_pool[poly_used_cnt].material = material;
					poly_sort_pool[poly_used_cnt].depth = depth_prim;

					poly_start += primitive_type_info[prim->p_type].vert_per_prim_cost;
					
					poly_used_cnt++;
				}
*/
