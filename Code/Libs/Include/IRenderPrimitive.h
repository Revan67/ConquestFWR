// IRenderPrimitive.h
//
//

#ifndef IRenderPrimitive_H
#define IRenderPrimitive_H

#include "rendpipeline.h"

//

typedef enum {

	RPR_BATCH					,
	RPR_BATCH_POOLS				,
	RPR_BATCH_OPAQUE_FLUSH		,
	RPR_BATCH_OPAQUE_POOL		,
	RPR_BATCH_TRANSLUCENT_POOL	,
	RPR_BATCH_TRANSLUCENT_NONZ_POOL	,
	RPR_BATCH_TRANSLUCENT_MODE	,

	RPR_STATE_FLAGS				,
	RPR_STATE_ID				,

	/* Leave this as last */
	RPR_MAX_STATE
} RPRSTATE;

//

typedef enum
{
	RPR_OPAQUE							= 0x01,
	RPR_TRANSLUCENT_UNSORTED			= 0x02,
	RPR_TRANSLUCENT_DEPTH_SORTED		= 0x04,
	RPR_TRANSLUCENT_UNSORTED_ONLY		= 0x08,
	RPR_TRANSLUCENT_DEPTH_SORTED_ONLY	= 0x10,
} RPRENUM;


//

#define IID_IRenderPrimitive MAKE_IID( "IRenderPrimitive", 1 )

//
// IRenderPrimitive
//
//
//
struct IRenderPrimitive : public IDAComponent
{
	virtual GENRESULT COMAPI set_state( RPRSTATE state, U32 value ) = 0;
	virtual GENRESULT COMAPI get_state( RPRSTATE state, U32 *value ) = 0;

	virtual GENRESULT COMAPI set_modelview( const Transform & modelview ) = 0;
	virtual GENRESULT COMAPI get_modelview( Transform & modelview ) = 0;

	virtual GENRESULT COMAPI set_viewport( int x, int y, int w, int h ) = 0;
	virtual GENRESULT COMAPI get_viewport( int *x, int *y, int *w, int *h ) = 0;

	virtual GENRESULT COMAPI set_ortho( float left, float right, float bottom, float top, float nearval=-1.0, float farval=1.0 ) = 0;
	virtual GENRESULT COMAPI set_perspective( float fovy, float aspect, float znear, float zfar ) = 0;
	virtual GENRESULT COMAPI set_projection( const Matrix4 &projection ) = 0;
	virtual GENRESULT COMAPI get_projection( Matrix4 &projection ) = 0;

	virtual GENRESULT COMAPI set_material( D3DMATERIAL9 *material_values ) = 0;
	virtual GENRESULT COMAPI get_material( D3DMATERIAL9 *out_material_values ) = 0;

	virtual GENRESULT COMAPI begin_scene( void ) = 0;
	virtual GENRESULT COMAPI end_scene( void ) = 0;

	virtual GENRESULT COMAPI set_render_state( D3DRENDERSTATETYPE state, U32 value ) = 0;
	virtual GENRESULT COMAPI get_render_state( D3DRENDERSTATETYPE state, U32 *value ) = 0;
	virtual GENRESULT COMAPI set_texture_stage_state( U32 stage, D3DTEXTURESTAGESTATETYPE, U32 value ) = 0;
	virtual GENRESULT COMAPI get_texture_stage_state( U32 stage, D3DTEXTURESTAGESTATETYPE, U32 *value ) = 0;
	
	virtual GENRESULT COMAPI set_sampler_state( U32 stage, D3DSAMPLERSTATETYPE, U32 value ) = 0;
	virtual GENRESULT COMAPI get_sampler_state( U32 stage, D3DSAMPLERSTATETYPE, U32 *value ) = 0;
	
	virtual GENRESULT COMAPI set_texture_stage_texture( U32 stage, U32 htexture ) = 0;
	virtual GENRESULT COMAPI get_texture_stage_texture( U32 stage, U32 *htexture ) = 0;
	virtual GENRESULT COMAPI verify_state(  void ) = 0;
	
	virtual GENRESULT COMAPI draw_primitive( D3DPRIMITIVETYPE type, U32 vertex_format, const void *verts, int num_verts, U32 flags ) = 0;
	virtual GENRESULT COMAPI draw_indexed_primitive( D3DPRIMITIVETYPE type, U32 vertex_format, const void *verts, int num_verts, const U16 * indices, int num_indices, U32 flags ) = 0;

	virtual GENRESULT COMAPI draw_primitive_vb( D3DPRIMITIVETYPE type, IRP_VERTEXBUFFERHANDLE vbhandle, int start_vert, int num_verts, U32 flags  ) = 0;
	virtual GENRESULT COMAPI draw_indexed_primitive_vb( D3DPRIMITIVETYPE type, IRP_VERTEXBUFFERHANDLE vbhandle, int start_vert, int num_verts, const U16 * indices, int num_indices, U32 flags  ) = 0;

	virtual GENRESULT COMAPI flush( U32 rprenum_f_flags ) = 0;
};


#endif // EOF
