//--------------------------------------------------------------------------//
//                                                                          //
//                            CQPipeline.cpp                                //
//                                                                          //
//               COPYRIGHT (C) 1997 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
   $Header: /Conquest/App/Src/cqpipeline.cpp 9     8/14/00 6:03p Jasony $
*/
//--------------------------------------------------------------------------//

#include "pch.h"
#include <globals.h>

#include <rendpipeline.h>
#include <FVF.h>

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//

#define PIPETHROUGH(x)	\
	if (criticalSection)	\
	{	\
		GENRESULT result;	\
		EnterCriticalSection(criticalSection);	\
		result = pipe->##x;	\
		LeaveCriticalSection(criticalSection);	\
		return result;	\
	}	\
	else	\
	{	\
		return pipe->##x;	\
	}	


//--------------------------------------------------------------------------//
//
struct CQPipeline : IRenderPipeline
{
	IDAComponent * container;
	IRenderPipeline * pipe;
	CRITICAL_SECTION * criticalSection;
	DAComponentInner<CQPipeline> innerComponent;

	BEGIN_DACOM_MAP_INBOUND(CQPipeline)
	DACOM_INTERFACE_ENTRY(IRenderPipeline)
	DACOM_INTERFACE_ENTRY2(IID_IRenderPipeline, IRenderPipeline)
	END_DACOM_MAP()

    void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}
	
	CQPipeline (void) : innerComponent(this)
	{
	}

	/* IDAComponent methods */

	virtual GENRESULT COMAPI QueryInterface (const C8 *interface_name, void **instance)
	{
		return container->QueryInterface(interface_name, instance);
	}
	
	virtual U32 COMAPI AddRef (void)
	{
		return container->AddRef();
	}
	
	virtual U32 COMAPI Release (void)
	{
		return container->Release();
	}

	/* IRenderPipeline methods */
	
	ID3DXEffect** load_effect(const char * filename,IComponentFactory *DIR)
	{
		if (criticalSection)	
		{	
			ID3DXEffect** result;	
			EnterCriticalSection(criticalSection);	
			result = pipe->load_effect(filename,DIR);	
			LeaveCriticalSection(criticalSection);	
			return result;	
		}	
		else	
		{	
			return pipe->load_effect(filename,DIR);
		}	
	}
	GENRESULT COMAPI set_vs_constants(UINT StartRegister,CONST FLOAT* pConstantData,UINT RegisterCount)
	{
		PIPETHROUGH ( set_vs_constants(StartRegister, pConstantData,RegisterCount));
	}

	
	GENRESULT COMAPI set_default_constants( const Transform &W, Vector eye, U32 lightCount)
	{
		PIPETHROUGH ( set_default_constants(W, eye, lightCount));
	}

	GENRESULT COMAPI set_ps_constants(UINT StartRegister,CONST FLOAT* pConstantData,UINT RegisterCount)
	{
		PIPETHROUGH ( set_ps_constants(StartRegister, pConstantData,RegisterCount));
	}
	GENRESULT COMAPI set_vertex_declaration(IDirect3DVertexDeclaration9 *pDecl)
	{
		PIPETHROUGH ( set_vertex_declaration(pDecl) );
	}


	virtual GENRESULT COMAPI startup( const char *profile_name)
	{
		TEXMEMORYUSED = 0;
		VBMEMORYUSED = 0;
		PIPETHROUGH ( startup(profile_name) )
	}

	virtual GENRESULT COMAPI shutdown( void  )
	{
		PIPETHROUGH ( shutdown() )
	}

	virtual GENRESULT COMAPI get_device_info( RPDEVICEINFO *info  )
	{
		PIPETHROUGH ( get_device_info(info) )
	}

	virtual GENRESULT COMAPI get_num_display_modes( U32 *num_modes )
	{
		PIPETHROUGH ( get_num_display_modes(num_modes) )
	}

	virtual GENRESULT COMAPI get_display_mode( RPDISPLAYMODEINFO *mode, U32 mode_num  )
	{
		PIPETHROUGH ( get_display_mode(mode, mode_num) )
	}

	virtual GENRESULT COMAPI query_device_ability( RPDEVICEABILITY ability, U32 *out_answer )
	{
		PIPETHROUGH ( query_device_ability(ability, out_answer) )
	}

	virtual GENRESULT COMAPI set_pipeline_state( RPPIPELINESTATE state, U32 value  )
	{
		PIPETHROUGH ( set_pipeline_state(state, value) )
	}

	virtual GENRESULT COMAPI get_pipeline_state( RPPIPELINESTATE state, U32 *value  )
	{
		PIPETHROUGH ( get_pipeline_state(state, value) )
	}

	virtual GENRESULT COMAPI create_buffers( HWND hwnd, int hres, int vres )
	{
		PIPETHROUGH ( create_buffers(hwnd, hres, vres) )
	}

	virtual GENRESULT COMAPI destroy_buffers( void  )
	{
		PIPETHROUGH ( destroy_buffers() )
	}

	virtual GENRESULT COMAPI get_num_device_texture_formats( U32 *num_formats )
	{
		PIPETHROUGH ( get_num_device_texture_formats(num_formats) )
	}

	virtual GENRESULT COMAPI get_device_texture_format( PixelFormat *texture_pf, U32 format_num )
	{
		PIPETHROUGH ( get_device_texture_format(texture_pf, format_num) )
	}

	virtual GENRESULT COMAPI get_device_stats( RPDEVICESTATS *stat  )
	{
		PIPETHROUGH ( get_device_stats(stat) )
	}

	virtual GENRESULT COMAPI clear_buffers( U32 rp_clear_flags, RECT *viewport_sub_rect  )
	{
		PIPETHROUGH ( clear_buffers(rp_clear_flags, viewport_sub_rect) )
	}

	virtual GENRESULT COMAPI swap_buffers( void  )
	{
		PIPETHROUGH ( swap_buffers() )
	}

	virtual GENRESULT COMAPI lock_buffer( RPLOCKDATA *lockData  )
	{
		PIPETHROUGH ( lock_buffer(lockData) )
	}

	virtual GENRESULT COMAPI unlock_buffer( void  )
	{
		PIPETHROUGH ( unlock_buffer() )
	}

	virtual GENRESULT COMAPI get_buffer_interface( const char *iid, void **out_iif  )
	{
		PIPETHROUGH ( get_buffer_interface(iid, out_iif) )
	}

	virtual GENRESULT COMAPI get_buffer_dc( HDC *dc  )
	{
		PIPETHROUGH ( get_buffer_dc(dc) )
	}

	virtual GENRESULT COMAPI release_buffer_dc( HDC dc  )
	{
		PIPETHROUGH ( release_buffer_dc(dc) )
	}

	virtual GENRESULT COMAPI create_vertex_buffer( U32 vertex_format, int num_verts, U32 irp_vbf_flags, IRP_VERTEXBUFFERHANDLE *out_vb_handle )
	{
		U32 amount = num_verts*FVF_SIZEOF_VERT(vertex_format);
		amount += VBMEMORYUSED;
		VBMEMORYUSED = amount;
		PIPETHROUGH ( create_vertex_buffer(vertex_format, num_verts, irp_vbf_flags, out_vb_handle) )
	}

	virtual GENRESULT COMAPI destroy_vertex_buffer( IRP_VERTEXBUFFERHANDLE vb_handle )
	{
		PIPETHROUGH ( destroy_vertex_buffer(vb_handle) )
	}

	virtual GENRESULT COMAPI lock_vertex_buffer( IRP_VERTEXBUFFERHANDLE vb_handle, U32 ddlock_flags, void **out_vertex_buffer, U32 *out_vertex_buffer_size )
	{
		PIPETHROUGH ( lock_vertex_buffer(vb_handle, ddlock_flags, out_vertex_buffer, out_vertex_buffer_size) )
	}

	virtual GENRESULT COMAPI unlock_vertex_buffer( IRP_VERTEXBUFFERHANDLE vb_handle )
	{
		PIPETHROUGH ( unlock_vertex_buffer(vb_handle) )
	}

	virtual GENRESULT COMAPI optimize_vertex_buffer( IRP_VERTEXBUFFERHANDLE vb_handle )
	{
		PIPETHROUGH ( optimize_vertex_buffer(vb_handle) )
	}

	virtual GENRESULT COMAPI process_vertex_buffer( IRP_VERTEXBUFFERHANDLE dst_vb_handle, IRP_VERTEXBUFFERHANDLE src_vb_handle, int first_vertex_index, int num_verts, U32 irp_pvbf_flags )
	{
		PIPETHROUGH ( process_vertex_buffer(dst_vb_handle, src_vb_handle, first_vertex_index, num_verts, irp_pvbf_flags) )
	}

	virtual GENRESULT COMAPI set_modelview( const Transform & modelview  )
	{
		PIPETHROUGH ( set_modelview(modelview) )
	}

	virtual GENRESULT COMAPI get_modelview( Transform & modelview  )
	{
		PIPETHROUGH ( get_modelview(modelview) )
	}

	virtual GENRESULT COMAPI set_projection( const Matrix4 & projection  )
	{
		PIPETHROUGH ( set_projection(projection) )
	}

	virtual GENRESULT COMAPI get_projection( Matrix4 & projection  )
	{
		PIPETHROUGH ( get_projection(projection) )
	}

	virtual GENRESULT COMAPI set_viewport( int x, int y, int w, int h  )
	{
		PIPETHROUGH ( set_viewport(x, y, w, h) )
	}

	virtual GENRESULT COMAPI get_viewport( int *x, int *y, int *w, int *h  )
	{
		PIPETHROUGH ( get_viewport(x, y, w, h) )
	}

	virtual GENRESULT COMAPI set_window( HWND wnd, int x, int y, int w, int h  )
	{
		PIPETHROUGH ( set_window(wnd, x, y, w, h) )
	}

	virtual GENRESULT COMAPI get_window( HWND *out_wnd, int *out_x, int *out_y, int *out_w, int *out_h  )
	{
		PIPETHROUGH ( get_window(out_wnd, out_x, out_y, out_w, out_h) )
	}
	
	virtual GENRESULT COMAPI set_depth_range( float lower_z_bound, float upper_z_bound )
	{
		PIPETHROUGH ( set_depth_range(lower_z_bound, upper_z_bound) )
	}

	virtual GENRESULT COMAPI get_depth_range( float *out_lower_z_bound, float *out_upper_z_bound )
	{
		PIPETHROUGH ( get_depth_range(out_lower_z_bound, out_upper_z_bound) )
	}

	virtual GENRESULT COMAPI set_lookat( float eyex, float eyey, float eyez, float centerx, float centery, float centerz, float upx, float upy, float upz   )
	{
		PIPETHROUGH ( set_lookat(eyex, eyey, eyez, centerx, centery, centerz, upx, upy, upz) )
	}

	virtual GENRESULT COMAPI set_ortho( float left, float right, float bottom, float top, float nearval, float farval )
	{
		PIPETHROUGH ( set_ortho(left, right, bottom, top, nearval, farval) )
	}

	virtual GENRESULT COMAPI set_perspective( float fovy, float aspect, float znear, float zfar  )
	{
		PIPETHROUGH ( set_perspective(fovy, aspect, znear, zfar) )
	}

	virtual GENRESULT COMAPI set_light( U32 light_index, D3DLIGHT9 *light_values )
	{
		PIPETHROUGH ( set_light(light_index, light_values) )
	}

	virtual GENRESULT COMAPI get_light( U32 light_index, D3DLIGHT9 *out_light_values )
	{
		PIPETHROUGH ( get_light(light_index, out_light_values) )
	}

	virtual GENRESULT COMAPI set_light_enable( U32 light_index, U32 enable )
	{
		PIPETHROUGH ( set_light_enable(light_index, enable) )
	}

	virtual GENRESULT COMAPI get_light_enable( U32 light_index, U32 *out_enable )
	{
		PIPETHROUGH ( get_light_enable(light_index, out_enable) )
	}

	virtual GENRESULT COMAPI get_num_lights( U32 *out_num_lights )
	{
		PIPETHROUGH ( get_num_lights(out_num_lights) )
	}

	virtual GENRESULT COMAPI get_lights( U32 num_start, U32 num_lights, U32 *out_light_indices )
	{
		PIPETHROUGH ( get_lights(num_start, num_lights, out_light_indices) )
	}

	virtual GENRESULT COMAPI set_material( D3DMATERIAL9 *material_values )
	{
		PIPETHROUGH ( set_material(material_values) )
	}

	virtual GENRESULT COMAPI get_material( D3DMATERIAL9 *out_material_values )
	{
		PIPETHROUGH ( get_material(out_material_values) )
	}

	virtual GENRESULT COMAPI begin_scene( void  )
	{
		PIPETHROUGH ( begin_scene() )
	}

	virtual GENRESULT COMAPI end_scene( void  )
	{
		PIPETHROUGH ( end_scene() )
	}

	virtual GENRESULT COMAPI set_render_state( D3DRENDERSTATETYPE state, U32 value  )
	{
		PIPETHROUGH ( set_render_state(state, value) )
	}

	virtual GENRESULT COMAPI get_render_state( D3DRENDERSTATETYPE state, U32 *value  )
	{
		PIPETHROUGH ( get_render_state(state, value) )
	}

	virtual GENRESULT COMAPI set_texture_stage_state( U32 stage, D3DTEXTURESTAGESTATETYPE type, U32 value  )
	{
		PIPETHROUGH ( set_texture_stage_state(stage, type, value) )
	}

	virtual GENRESULT COMAPI get_texture_stage_state( U32 stage, D3DTEXTURESTAGESTATETYPE type, U32 *value  )
	{
		PIPETHROUGH ( get_texture_stage_state(stage, type, value) )
	}


	virtual GENRESULT COMAPI set_sampler_state( U32 stage, D3DSAMPLERSTATETYPE type, U32 value  )
	{
		PIPETHROUGH ( set_sampler_state(stage, type, value) )
	}

	virtual GENRESULT COMAPI get_sampler_state( U32 stage, D3DSAMPLERSTATETYPE type, U32 *value  )
	{
		PIPETHROUGH ( get_sampler_state(stage, type, value) )
	}

	virtual GENRESULT COMAPI set_texture_stage_transform( U32 stage, Matrix4 &mat4 )
	{
		PIPETHROUGH ( set_texture_stage_transform(stage, mat4) )
	}

	virtual GENRESULT COMAPI get_texture_stage_transform( U32 stage, Matrix4 &out_mat4 )
	{
		PIPETHROUGH ( get_texture_stage_transform(stage, out_mat4) )
	}

	virtual GENRESULT COMAPI set_texture_stage_texture( U32 stage, U32 htexture  )
	{
		PIPETHROUGH ( set_texture_stage_texture(stage, htexture) )
	}

	virtual GENRESULT COMAPI get_texture_stage_texture( U32 stage, U32 *htexture  )
	{
		PIPETHROUGH ( get_texture_stage_texture(stage, htexture) )
	}

	virtual GENRESULT COMAPI verify_state( void )
	{
		PIPETHROUGH ( verify_state() )
	}

	virtual GENRESULT COMAPI create_state_block( D3DSTATEBLOCKTYPE type, U32 *out_sbhandle )
	{
		PIPETHROUGH ( create_state_block(type, out_sbhandle) )
	}

	virtual GENRESULT COMAPI update_state_block( U32 sbhandle )
	{
		PIPETHROUGH ( update_state_block(sbhandle) )
	}

	virtual GENRESULT COMAPI begin_state_block( void )
	{
		PIPETHROUGH ( begin_state_block() )
	}

	virtual GENRESULT COMAPI end_state_block( U32 *out_sbhandle )
	{
		PIPETHROUGH ( end_state_block(out_sbhandle) )
	}

	virtual GENRESULT COMAPI apply_state_block( U32 sbhandle )
	{
		PIPETHROUGH ( apply_state_block(sbhandle) )
	}

	virtual GENRESULT COMAPI delete_state_block( U32 sbhandle )
	{
		PIPETHROUGH ( delete_state_block(sbhandle) )
	}

	virtual GENRESULT COMAPI draw_primitive( D3DPRIMITIVETYPE type, U32 vertex_format, const void *verts, int num_verts, U32 flags  )
	{
		PIPETHROUGH ( draw_primitive(type, vertex_format, verts, num_verts, flags) )
	}

	virtual GENRESULT COMAPI draw_indexed_primitive( D3DPRIMITIVETYPE type, U32 vertex_format, const void *verts, int num_verts, const U16 * indices, int num_indices, U32 flags  )
	{
		PIPETHROUGH ( draw_indexed_primitive(type, vertex_format, verts, num_verts, indices, num_indices, flags) )
	}

	virtual GENRESULT COMAPI draw_primitive_vb( D3DPRIMITIVETYPE type, IRP_VERTEXBUFFERHANDLE vbhandle, int start_vert, int num_verts, U32 flags  )
	{
		PIPETHROUGH ( draw_primitive_vb(type, vbhandle, start_vert, num_verts, flags) )
	}

	virtual GENRESULT COMAPI draw_indexed_primitive_vb( D3DPRIMITIVETYPE type, IRP_VERTEXBUFFERHANDLE vbhandle, int start_vert, int num_verts, const U16 * indices, int num_indices, U32 flags  )
	{
		PIPETHROUGH ( draw_indexed_primitive_vb(type, vbhandle, start_vert, num_verts, indices, num_indices, flags) )
	}
	virtual GENRESULT COMAPI draw_indexed_primitive_vb( D3DPRIMITIVETYPE type, IRP_VERTEXBUFFERHANDLE vbhandle, int start_vert, int num_verts,  IDirect3DIndexBuffer9* IB, int num_indices, U32 flags  )
	{
		PIPETHROUGH ( draw_indexed_primitive_vb(type, vbhandle, start_vert, num_verts, IB, num_indices, flags) )
	}

	virtual GENRESULT COMAPI create_index_buffer(UINT Length, IDirect3DIndexBuffer9** ppIndexBuffer)
	{
		PIPETHROUGH ( create_index_buffer(Length, ppIndexBuffer) )
	}
 	
	virtual GENRESULT COMAPI create_cube_texture_from_file(const char* filename, IComponentFactory * DIR,U32 &out_htexture)
	{
		PIPETHROUGH ( create_cube_texture_from_file(filename,DIR,out_htexture));

		// toDo: check texture size and increase TEXMEMORYUSED?
	}

	virtual GENRESULT COMAPI create_texture( int width, int height, const PixelFormat &desiredformat, int num_lod, U32 irp_ctf_flags, U32 &out_htexture  )
	{
		U32 amount = (width * height * 2 * 133) / 100;		// increase by 2/3 for mip levels
		amount += TEXMEMORYUSED;
		TEXMEMORYUSED = amount;
		PIPETHROUGH ( create_texture(width, height, desiredformat, num_lod, irp_ctf_flags, out_htexture ) )
	}

	virtual GENRESULT COMAPI destroy_texture( U32 htexture  )
	{
		PIPETHROUGH ( destroy_texture(htexture) )
	}

	virtual GENRESULT COMAPI is_texture( U32 htexture  )
	{
		PIPETHROUGH ( is_texture(htexture) )
	}

	virtual GENRESULT COMAPI lock_texture( U32 htexture, int level, RPLOCKDATA *lockData  )
	{
		PIPETHROUGH ( lock_texture(htexture, level, lockData) )
	}

	virtual GENRESULT COMAPI unlock_texture( U32 htexture, int level  )
	{
		PIPETHROUGH ( unlock_texture(htexture, level) )
	}

	virtual GENRESULT COMAPI get_texture_format( U32 htexture, PixelFormat *out_pf  )
	{
		PIPETHROUGH ( get_texture_format(htexture, out_pf) )
	}

	virtual GENRESULT COMAPI get_texture_dim( U32 htexture, U32 *out_width, U32 *out_height, U32 *out_num_lod  )
	{
		PIPETHROUGH ( get_texture_dim(htexture, out_width, out_height, out_num_lod) )
	}

	virtual GENRESULT COMAPI get_texture_interface( U32 htexture, const char *iid, void **out_iif  )
	{
		PIPETHROUGH ( get_texture_interface(htexture, iid, out_iif) )
	}

	virtual GENRESULT COMAPI get_texture_dc( U32 htexture, HDC *out_hdc )
	{
		PIPETHROUGH ( get_texture_dc(htexture, out_hdc) )
	}

	virtual GENRESULT COMAPI release_texture_dc( U32 htexture, HDC hdc )
	{
		PIPETHROUGH ( release_texture_dc(htexture, hdc) )
	}

	virtual GENRESULT COMAPI set_texture_palette( U32 htexture, int start, int length, const RGB *colors  )
	{
		PIPETHROUGH ( set_texture_palette(htexture, start, length, colors) )
	}

	virtual GENRESULT COMAPI get_texture_palette( U32 htexture, int start, int length, RGB *colors  )
	{
		PIPETHROUGH ( get_texture_palette(htexture, start, length, colors) )
	}

	virtual GENRESULT COMAPI set_texture_level_data( U32 htexture, int level, int src_width, int src_height, int src_stride, const PixelFormat &src_format, const void *src_pixel, const void *src_alpha, const RGB *src_palette )
	{
		PIPETHROUGH ( set_texture_level_data(htexture, level, src_width, src_height, src_stride, src_format, src_pixel, src_alpha, src_palette) )
	}

	virtual GENRESULT COMAPI blit_texture( U32 hDest, U32 destLevel, RECT destRect, U32 hSrc, U32 srcLevel, RECT srcRect  )
	{
		PIPETHROUGH ( blit_texture(hDest, destLevel, destRect, hSrc, srcLevel, srcRect ) )
	}

	virtual GENRESULT COMAPI get_num_textures( U32 *out_num_textures  )
	{
		PIPETHROUGH ( get_num_textures(out_num_textures) )
	}

	virtual GENRESULT COMAPI get_texture( U32 texture_num, U32 *out_htexture  )
	{
		PIPETHROUGH ( get_texture(texture_num, out_htexture) )
	}
};
static CQPipeline * cqpipeline;
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
void __stdcall SetPipelineCriticalSection (CRITICAL_SECTION * criticalSection)
{
	cqpipeline->criticalSection = criticalSection;
}
//--------------------------------------------------------------------------//
//
void __stdcall CreateCQPipeline (IDAComponent * container, void ** instance)
{
	cqpipeline = new CQPipeline;

	cqpipeline->container = container;
	if (container->QueryInterface(IID_IRenderPipeline, (void **) &cqpipeline->pipe) == GR_OK)
		cqpipeline->pipe->Release();		// release the extra reference
	*instance = &cqpipeline->innerComponent;
}

//--------------------------------------------------------------------------//
//----------------------------End CQPipeline.cpp----------------------------//
//--------------------------------------------------------------------------//
