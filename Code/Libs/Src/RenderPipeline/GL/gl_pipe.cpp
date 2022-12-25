// gl_pipe.cpp
//
// A RenderPipeline class component that renders using an OpenGL device.  
// Supports ONLY win32 opengl (OpenGL32.dll) 
//

#pragma warning( disable : 4018 )	// signed/unsigned mismatch
#pragma warning( disable : 4100 )	// unreferenced formal parameter
#pragma warning( disable : 4239 )
#pragma warning( disable : 4245 )	// conversion from int to long
#pragma warning( disable : 4530 )   // exceptions disabled
#pragma warning( disable : 4702 )   // unreachable code
#pragma warning( disable : 4710 )   // function 'foo' not inlined
#pragma warning( disable : 4786 )   // identifier truncated

//

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <assert.h>
#include <GL/gl.h>
#include <GL/glu.h>

//#define INITGUID
#include <ddraw.h>

#include "dacom.h"
#include "tcomponent.h"
#include "TSmartPointer.h"
#include "stddat.h"
#include "3dmath.h"
#include "pixel.h"
#include "rendpipeline.h"
#include "IRenderPrimitive.h"
#include "IProfileParser_Utility.h"
#include "RPUL.h"
#include "packed_argb.h"
#include "da_heap_utility.h"

#include "../../Include/handlemap.h"

#define RP_DEFINE_GLOBALS	1
#include "debug.h"
#include "FVF.h"

#define R(color) (((color)>>16) & 0xFF)
#define G(color) (((color)>> 8) & 0xFF)
#define B(color) (((color)    ) & 0xFF)
#define A(color) (((color)>>24)       )

#define RI(color) (((color) & 0x00FF0000) >> 16)
#define GI(color) (((color) & 0x0000FF00)      )
#define BI(color) (((color) & 0x000000FF) << 16)
#define AI(color) (((color) & 0xFF000000)      )

struct GLVertex
{
	union {
		struct{
			float u, v;
		};
		float uv[2];
	};

	union {
		struct {
			unsigned char	r, g, b, a;
		};
		unsigned int color;
	};

	Vector pos;
};


// ------------------------------------------------------------------
// Global Variables
// ------------------------------------------------------------------

const char *CLSID_OGL_RenderPipeline= "OGL_RenderPipeline";
const char *CLSID_IRenderPipeline	= "IRenderPipeline";
const char *DeviceProfile_Default	= "RenderPipeline";
const char *DeviceClass_Default		= "OpenGL";
const char *DeviceId_Default		= "opengl32";
const char *DeviceType_Default		= "Hardware";

const U32 OGLRP_MAX_STRING_LEN = 128;


GLenum d3dprim_to_glprm[] = { GL_POINTS, GL_POINTS, GL_LINES, GL_LINE_STRIP, GL_TRIANGLES, GL_TRIANGLE_STRIP, GL_TRIANGLE_FAN };



// --------------------------------------------------------------------------
// RenderPipeline
//
// 
//
//
struct RenderPipeline : IRenderPipeline, 
						IRenderPrimitive,
						IAggregateComponent
{
	BEGIN_DACOM_MAP_INBOUND(RenderPipeline)
	DACOM_INTERFACE_ENTRY(IRenderPipeline)
	DACOM_INTERFACE_ENTRY(IRenderPrimitive)
	DACOM_INTERFACE_ENTRY(IAggregateComponent)
	DACOM_INTERFACE_ENTRY2(IID_IRenderPipeline,IRenderPipeline)
	DACOM_INTERFACE_ENTRY2(IID_IRenderPrimitive,IRenderPrimitive)
	DACOM_INTERFACE_ENTRY2(IID_IAggregateComponent,IAggregateComponent)
	END_DACOM_MAP()

public:		// public interface

    // IAggregateComponent interface
	//
	GENRESULT COMAPI Initialize(void);

	// IRenderPipeline interface
	//
	GENRESULT COMAPI startup( const char *device_id_persist=NULL ) ;
	GENRESULT COMAPI shutdown( void  ) ;
	GENRESULT COMAPI get_device_info( RPDEVICEINFO *info  ) ;
	GENRESULT COMAPI get_device_stats( RPDEVICESTATS *stat  ) ;
	GENRESULT COMAPI get_num_display_modes( U32 *num_modes ) ;
	GENRESULT COMAPI get_display_mode( RPDISPLAYMODEINFO *mode, U32 mode_num  ) ;
	GENRESULT COMAPI get_num_device_texture_formats( U32 *num_formats ) ;
	GENRESULT COMAPI get_device_texture_format( PixelFormat *texture_pf, U32 format_num ) ;
	GENRESULT COMAPI query_device_ability( RPDEVICEABILITY ability, U32 *out_answer ) ;
	GENRESULT COMAPI create_buffers( HWND hwnd, int hres, int vres ) ;
	GENRESULT COMAPI destroy_buffers( void  ) ;
	GENRESULT COMAPI clear_buffers( U32 rp_clear_flags, RECT *viewport_sub_rect  ) ;
	GENRESULT COMAPI swap_buffers( void  ) ;
	GENRESULT COMAPI lock_buffer( RPLOCKDATA *lockData  ) ;
	GENRESULT COMAPI unlock_buffer( void  ) ;
	GENRESULT COMAPI get_buffer_interface( const char *iid, void **out_iif  ) ;
	GENRESULT COMAPI get_buffer_dc( HDC *dc  ) ;
	GENRESULT COMAPI release_buffer_dc( HDC dc  ) ;
	GENRESULT COMAPI set_pipeline_state( RPPIPELINESTATE state, U32 value  ) ;
	GENRESULT COMAPI get_pipeline_state( RPPIPELINESTATE state, U32 *value  ) ;
	GENRESULT COMAPI set_modelview( const Transform & modelview  ) ;
	GENRESULT COMAPI get_modelview( Transform & modelview  ) ;
	GENRESULT COMAPI set_projection( const Matrix4 & projection  ) ;
	GENRESULT COMAPI get_projection( Matrix4 & projection  ) ;
	GENRESULT COMAPI set_viewport( int x, int y, int w, int h  ) ;
	GENRESULT COMAPI get_viewport( int *x, int *y, int *w, int *h  ) ;
	GENRESULT COMAPI set_window( HWND wnd, int x, int y, int w, int h  ) ;
	GENRESULT COMAPI get_window( HWND *out_wnd, int *out_x, int *out_y, int *out_w, int *out_h  ) ;
	GENRESULT COMAPI set_depth_range( float lower_z_bound, float upper_z_bound ) ;
	GENRESULT COMAPI get_depth_range( float *out_lower_z_bound, float *out_upper_z_bound ) ;
	GENRESULT COMAPI set_lookat( float eyex, float eyey, float eyez, float centerx, float centery, float centerz, float upx, float upy, float upz   ) ;
	GENRESULT COMAPI set_ortho( float left, float right, float bottom, float top, float nearval=-1.0, float farval=1.0  ) ;
	GENRESULT COMAPI set_perspective( float fovy, float aspect, float znear, float zfar  ) ;
	GENRESULT COMAPI set_light( U32 light_index, D3DLIGHT7 *light_values );
	GENRESULT COMAPI get_light( U32 light_index, D3DLIGHT7 *out_light_values );
	GENRESULT COMAPI set_light_enable( U32 light_index, U32 enable );
	GENRESULT COMAPI get_light_enable( U32 light_index, U32 *out_enable );
	GENRESULT COMAPI set_material( U32 material_index, D3DMATERIAL7 *material_values );
	GENRESULT COMAPI get_material( U32 material_index, D3DMATERIAL7 *out_material_values );
	GENRESULT COMAPI begin_scene( void  ) ;
	GENRESULT COMAPI end_scene( void  ) ;
	GENRESULT COMAPI set_render_state( D3DRENDERSTATETYPE state, U32 value  ) ;
	GENRESULT COMAPI get_render_state( D3DRENDERSTATETYPE state, U32 *value  ) ;
	GENRESULT COMAPI set_texture_stage_state( U32 stage, D3DTEXTURESTAGESTATETYPE, U32 value  ) ;
	GENRESULT COMAPI get_texture_stage_state( U32 stage, D3DTEXTURESTAGESTATETYPE, U32 *value  ) ;
	GENRESULT COMAPI set_texture_stage_transform( U32 stage, Matrix4 &mat4 ) ;
	GENRESULT COMAPI get_texture_stage_transform( U32 stage, Matrix4 &out_mat4 ) ;
	GENRESULT COMAPI set_texture_stage_texture( U32 stage, U32 htexture  ) ;
	GENRESULT COMAPI get_texture_stage_texture( U32 stage, U32 *htexture  ) ;
	GENRESULT COMAPI verify_state( void ) ;
	GENRESULT COMAPI draw_primitive( D3DPRIMITIVETYPE type, U32 vertex_format, const void *verts, int num_verts, U32 flags  ) ;
	GENRESULT COMAPI draw_indexed_primitive( D3DPRIMITIVETYPE type, U32 vertex_format, const void *verts, int num_verts, const U16 * indices, int num_indices, U32 flags  ) ;
	GENRESULT COMAPI create_texture( int width, int height, const PixelFormat &desiredformat, int num_lod, U32 irp_ctf_flags, U32 &out_htexture  ) ;
	GENRESULT COMAPI destroy_texture( U32 htexture  ) ;
	GENRESULT COMAPI is_texture( U32 htexture  ) ;
	GENRESULT COMAPI lock_texture( U32 htexture, int level, RPLOCKDATA *lockData  ) ;
	GENRESULT COMAPI unlock_texture( U32 htexture, int level  ) ;
	GENRESULT COMAPI get_texture_format( U32 htexture, PixelFormat *out_pf  ) ;
	GENRESULT COMAPI get_texture_dim( U32 htexture, U32 *out_width, U32 *out_height, U32 *out_num_lod  ) ;
	GENRESULT COMAPI get_texture_interface( U32 htexture, const char *iid, void **out_iif  ) ;
	GENRESULT COMAPI set_texture_palette( U32 htexture, int start, int length, const RGB *colors  ) ;
	GENRESULT COMAPI get_texture_palette( U32 htexture, int start, int length, RGB *colors  ) ;
	GENRESULT COMAPI set_texture_level_data( U32 htexture, int level, int src_width, int src_height, int src_stride, const PixelFormat &src_format, const void *src_pixel, const void *src_alpha, const RGB *src_palette ) ;
	GENRESULT COMAPI blit_texture( U32 hDest, U32 destLevel, RECT destRect, U32 hSrc, U32 srcLevel, RECT srcRect  ) ;
	GENRESULT COMAPI get_num_textures( U32 *out_num_textures  ) ;
	GENRESULT COMAPI get_texture( U32 texture_num, U32 *out_htexture  ) ;
	GENRESULT COMAPI create_vertex_buffer( U32 vertex_format, int num_verts, U32 irp_vbf_flags, IRP_VERTEXBUFFERHANDLE *out_vb_handle ) ;
	GENRESULT COMAPI destroy_vertex_buffer( IRP_VERTEXBUFFERHANDLE vb_handle ) ;
	GENRESULT COMAPI lock_vertex_buffer( IRP_VERTEXBUFFERHANDLE vb_handle, U32 ddlock_flags, void **out_vertex_buffer, U32 *out_vertex_buffer_size ) ;
	GENRESULT COMAPI unlock_vertex_buffer( IRP_VERTEXBUFFERHANDLE vb_handle ) ;
	GENRESULT COMAPI optimize_vertex_buffer( IRP_VERTEXBUFFERHANDLE vb_handle ) ;
	GENRESULT COMAPI get_processed_vertex_buffer( IRP_VERTEXBUFFERHANDLE vb_handle, int first_vertex_index, int num_verts, U32 irp_pvbf_flags, IRP_VERTEXBUFFERHANDLE *out_vb_handle ) ;
	GENRESULT COMAPI release_processed_vertex_buffer( IRP_VERTEXBUFFERHANDLE vb_handle ) ;

	// IRenderPrimitive
	GENRESULT COMAPI set_state( RPRSTATE state, U32 value );
	GENRESULT COMAPI get_state( RPRSTATE state, U32 *value );
	GENRESULT COMAPI flush( U32 flags );

	RenderPipeline(void);
	~RenderPipeline(void);
	GENRESULT init(AGGDESC *desc);
	GENRESULT init(RPUL_DACOMDESC *desc);

protected:	// protected interface

	HWND	internal_get_hwnd( void );
	void	internal_set_default_pipeline_state( void );
	void	internal_set_default_render_state( void );
	void	internal_set_default_transform_state( void );
	void	verify_packed_buffer(int size );
	bool	pack(U32 vertex_format, const void *_verts, int num_verts);

protected:	// protected data

	struct GLWINDOWDATA
	{
		//

		HWND  wnd;
		HDC	  hdc;
		HGLRC glrc;
		U32   win_x,win_y,win_w,win_h;
		
		//

		GLWINDOWDATA()
		{
			wnd = 0;
			hdc = 0;
			glrc = 0;
			win_x = win_y = 0;
			win_w = win_h = 0;
		}
	};

	typedef handlemap<HWND,GLWINDOWDATA> WindowMap;

	COMPTR<IProfileParser>		profile_parser;

	char					ini_device_profile[OGLRP_MAX_STRING_LEN];	
	char					current_device_profile[OGLRP_MAX_STRING_LEN];	
	char					current_device_type[128];	
	RPDEVICEINFO			current_device_info;

	U32						pipeline_state[RP_MAX_PIPELINE_STATE];
	U32						render_state[D3DRS_MAX_STATE];
	U32						rpr_state[RPR_MAX_STATE];

	PIXELFORMATDESCRIPTOR	pixel_format;
	U32						pixel_format_num;

	WindowMap				windows;
	GLWINDOWDATA			*current_window;

	unsigned char *			frame_buffer_mirror;
	U32						frame_buffer_mirror_stretch_hack;
	U32						frame_buffer_mirror_no_read_on_lock;

	int						packed_length;
	GLVertex				*packed_buffer;
};


// Macro to define supported methods on this interface
//
#undef DA_METHOD
#undef DA_COMPONENT_NAME

#define DA_COMPONENT_NAME RenderPipeline
#define DA_METHOD(name,params) GENRESULT COMAPI DA_COMPONENT_NAME :: name params


// ------------------------------------------------------------------
// Render Pipeline lookup tables
// ------------------------------------------------------------------

HWND RenderPipeline::internal_get_hwnd( void )
{
	if( current_window ) {
		return current_window->wnd;
	}
	return NULL;
}

//

inline void RenderPipeline::internal_set_default_pipeline_state( void )
{
	memset( &pipeline_state[0], 0xFF, sizeof(pipeline_state[0]) * D3DRS_MAX_STATE );

	set_pipeline_state( RP_BUFFERS_COLOR_BPP,			16 );
	set_pipeline_state( RP_BUFFERS_DEPTH_BPP,			16 );
	set_pipeline_state( RP_BUFFERS_STENCIL_BPP,			0 );
	set_pipeline_state( RP_BUFFERS_COUNT,				2 );
	set_pipeline_state( RP_BUFFERS_HWFLIP,				TRUE );
	set_pipeline_state( RP_BUFFERS_FULLSCREEN,			FALSE );
	set_pipeline_state( RP_BUFFERS_VSYNC,				TRUE );
	set_pipeline_state( RP_BUFFERS_OFFSCREEN,			FALSE );
	set_pipeline_state( RP_BUFFERS_HWMEM,				TRUE );
	set_pipeline_state( RP_BUFFERS_WIDTH,				0 );
	set_pipeline_state( RP_BUFFERS_HEIGHT,				0 );
	set_pipeline_state( RP_BUFFERS_DEPTH_AUTOW,			FALSE );
	set_pipeline_state( RP_BUFFERS_SWAP_STALL,			FALSE );

	set_pipeline_state( RP_DEVICE_FPU_SETUP,			TRUE );
	set_pipeline_state( RP_DEVICE_MULTITHREADED,		FALSE );

	set_pipeline_state( RP_TEXTURE,						TRUE );
	set_pipeline_state( RP_TEXTURE_LOD,					TRUE );
	
	set_pipeline_state( RP_CLEAR_DEPTH,					0xFFFFFFFF );
	set_pipeline_state( RP_CLEAR_COLOR,					0x00000000 );
	set_pipeline_state( RP_CLEAR_STENCIL,				0xFFFFFFFF );
																											
	set_pipeline_state( RP_DEBUG_PROFILE_LOG,			FALSE );
													
	return;
}

//

inline void RenderPipeline::internal_set_default_render_state( void )
{
	memset( &render_state[0], 0xFF, sizeof(render_state[0]) * D3DRS_MAX_STATE );

	set_render_state( D3DRS_FILLMODE           , D3DFILL_SOLID				 );
	set_render_state( D3DRS_SHADEMODE          , D3DSHADE_GOURAUD			 );

	set_render_state( D3DRS_ZENABLE            , TRUE						 );
	set_render_state( D3DRS_ZWRITEENABLE       , TRUE						 );
	set_render_state( D3DRS_ZFUNC              , D3DCMP_LESS				 );
	set_render_state( D3DRS_ZBIAS              , 0							 );

	set_render_state( D3DRS_DITHERENABLE       , TRUE						 );
	set_render_state( D3DRS_CULLMODE           , D3DCULL_NONE				 );
	
	set_render_state( D3DRS_ALPHABLENDENABLE   , FALSE						 );
	set_render_state( D3DRS_SRCBLEND           , D3DBLEND_ONE				 );
	set_render_state( D3DRS_DESTBLEND          , D3DBLEND_ZERO				 );

	set_render_state( D3DRS_FOGENABLE          , FALSE						 );
	set_render_state( D3DRS_FOGCOLOR           , 0							 );
	set_render_state( D3DRS_FOGTABLEMODE       , D3DFOG_EXP				 );

	set_render_state( D3DRS_FOGSTART			, 0							 );
	set_render_state( D3DRS_FOGEND				, 1							 );
	set_render_state( D3DRS_FOGDENSITY			, 1							 );


	set_render_state( D3DRS_ANTIALIAS          , D3DANTIALIAS_NONE			 );
	set_render_state( D3DRS_EDGEANTIALIAS      , FALSE						 );
	set_render_state( D3DRS_STIPPLEDALPHA      , FALSE						 );
	set_render_state( D3DRS_ZVISIBLE           , FALSE						 );
	set_render_state( D3DRS_RANGEFOGENABLE     , FALSE						 );
	set_render_state( D3DRS_ALPHATESTENABLE    , FALSE						 );
	set_render_state( D3DRS_ALPHAREF           , 0							 );
	set_render_state( D3DRS_ALPHAFUNC          , D3DCMP_ALWAYS				 );
	set_render_state( D3DRS_SPECULARENABLE     , FALSE						 );
	set_render_state( D3DRS_COLORKEYENABLE     , FALSE						 );
	set_render_state( D3DRS_LASTPIXEL          , TRUE						 );
	set_render_state( D3DRS_LINEPATTERN        , 0							 );
	
	return;
}

//

inline void RenderPipeline::verify_packed_buffer(int size)
{
	if( packed_length < size )
	{
		packed_length = size;
		delete [] packed_buffer;
		packed_buffer = new GLVertex[packed_length];
	}
}

//

inline void RenderPipeline::internal_set_default_transform_state( void )
{
	glMatrixMode( GL_MODELVIEW );
	glLoadIdentity();
	glMatrixMode( GL_PROJECTION );
	glLoadIdentity();

	if( current_window ) {
		set_viewport( 0, 0, current_window->win_w, current_window->win_h );
	}
	else {
		set_viewport( 0, 0, 0, 0 );
	}

	return;
}



// ------------------------------------------------------------------
// Starup/shutdown Related Code
// ------------------------------------------------------------------

RenderPipeline::RenderPipeline(void)
{
	current_window = NULL;

	ini_device_profile[0] = 0;
	current_device_profile[0] = 0;
	current_device_type[0] = 0;

	memset( pipeline_state, 0xFF, sizeof(pipeline_state) );
	memset( render_state, 0xFF, sizeof(render_state) );
	memset( rpr_state, 0xFF, sizeof(rpr_state) );
	
	frame_buffer_mirror = NULL;
	frame_buffer_mirror_stretch_hack = 1;
	frame_buffer_mirror_no_read_on_lock = 1;

	packed_length = 0;
	packed_buffer = NULL;
}

//

RenderPipeline::~RenderPipeline(void)
{
	shutdown();
}

//

DA_METHOD(	startup,(const char *profile_name))
{
	shutdown();

	internal_set_default_pipeline_state();

	if( profile_name == NULL ) {
		profile_name = ini_device_profile;
	}

	if( strcmp( profile_name, current_device_profile ) != 0 ) {
		// TODO: should read some options out of the ini file
		// TODO: and create software/hardware/etc
		opt_get_string( DACOM_Acquire(), profile_parser, current_device_profile, "DeviceType", DeviceType_Default, current_device_type, 128 );

		strcpy( current_device_profile, profile_name );
	}

	return GR_OK;
}

//

DA_METHOD(	shutdown,(void ))
{
	destroy_buffers();
	return GR_OK;
}

//

GENRESULT COMAPI RenderPipeline::Initialize(void)
{ 
	opt_get_u32( DACOM_Acquire(), NULL, current_device_profile, "FrameBufferMirrorHack", TRUE, &frame_buffer_mirror_stretch_hack );
	opt_get_u32( DACOM_Acquire(), NULL, current_device_profile, "NoReadOnLock", TRUE, &frame_buffer_mirror_no_read_on_lock );
	
	return GR_OK; 
}

//

// This is called during normal use.  We are an aggregate.
//
GENRESULT RenderPipeline::init(AGGDESC *desc)
{ 
	if( desc->description!=NULL && strlen( desc->description ) > 0 ) {
		strcpy( ini_device_profile, desc->description );
	}
	else {
		strcpy( ini_device_profile, DeviceProfile_Default );
	}

	GENERAL_TRACE_1( _MS(( "Direct3D_RenderPipeline: init(AGG): using profile '%s'\n", ini_device_profile )) );

	return GR_OK;
}

//

// This is called during enumeration.  We are !not! an aggregate.
//
GENRESULT RenderPipeline::init( RPUL_DACOMDESC *desc)
{ 
	profile_parser = desc->profile_parser;

	if( desc->device_id != NULL && strlen( desc->device_id ) > 0 ) {
		strcpy( ini_device_profile, desc->device_id );
	}
	else {
		strcpy( ini_device_profile, DeviceProfile_Default );
	}

	GENERAL_TRACE_1( _MS(( "Direct3D_RenderPipeline: init(RP): using profile '%s'\n", ini_device_profile )) );
	
	return GR_OK;
}

//
DA_METHOD( query_device_ability,(RPDEVICEABILITY ability, U32 *out_answer ))
//DA_METHOD( query_device_ability,(RPDEVICEABILITY ability, U32 *out_answer, U32 *out_nearest))
{
#define ANS(X) { if(out_answer) { *out_answer = (X); } }

	switch(ability)
	{
		case RP_A_DEVICE_GAMMA:
		case RP_A_DEVICE_SOFTWARE:
		case RP_A_BUFFERS_DEPTH_LINEAR:
		case RP_A_TEXTURE_SQUARE_ONLY:
			ANS( FALSE )
			break;
		case RP_A_DEVICE_MEMORY:
			ANS( 4 * 1024 * 1024 )
			break;
		case RP_A_TEXTURE_MAX_WIDTH:
			ANS( 256 )
			break;
		case RP_A_TEXTURE_MAX_HEIGHT:
			ANS( 256 )
			break;
		case RP_A_TEXTURE_COORDINATES:
			ANS( 8 )
			break;
		case RP_A_BLEND_MUL_SRC:
			ANS( D3DBLEND_DESTCOLOR )
			break;
		case RP_A_BLEND_MUL_DST:
			ANS( D3DBLEND_ZERO )
			break;
		case RP_A_BLEND_ADD_SRC:
			ANS( D3DBLEND_ONE )
			break;
		case RP_A_BLEND_ADD_DST:
			ANS( D3DBLEND_ONE )
			break;
		case RP_A_BLEND_TRANSPARENCY_SRC:
			ANS( D3DBLEND_SRCALPHA )
			break;
		case RP_A_BLEND_TRANSPARENCY_DST:
			ANS( D3DBLEND_INVSRCALPHA )
			break;
		case RP_A_BLEND_MATRIX:
			if(out_answer)
			{
				out_answer[0] = out_answer[1] = out_answer[2] = out_answer[3] = -1;
			}
			break;
		default:
			ANS( TRUE )
	}

#undef ANS

	return GR_OK;
}

//

DA_METHOD(	get_device_info,(RPDEVICEINFO *i ))
{
	sprintf( i->device_description, "%s %s", (const char*)glGetString( GL_VENDOR ), (const char*)glGetString( GL_RENDERER ) );

	strcpy( i->device_class, DeviceClass_Default );
	strcpy( i->device_id_persist, DeviceId_Default );
	strcpy( i->device_type, DeviceType_Default );

	i->device_chipset_id = RP_D_GENERIC;
	return GR_OK;
}

//

DA_METHOD(	get_device_stats,(RPDEVICESTATS *stat ))
{

#if RP_PROFILE_STATS
	stat->available_texture_memory = 0;
	
	stat->num_texture_changes = dbg_num_txm_calls_activate;
	stat->num_texture_device = dbg_num_txm_device;
	stat->num_texture_system = dbg_num_txm_system;
	stat->num_texture_tosses = dbg_num_txm_device_destroy;
	stat->num_texture_new = dbg_num_txm_device_create;
	
	stat->num_txm_calls = dbg_num_txm_calls;
	stat->num_txm_calls_zero = dbg_num_txm_calls_zero;
	stat->num_txm_calls_nonzero = dbg_num_txm_calls_nonzero ;
	stat->num_txm_calls_override = dbg_num_txm_calls_override ;
	stat->num_txm_calls_activate = dbg_num_txm_calls_activate ;
	stat->num_txm_calls_failed = dbg_num_txm_calls_failed ;

	stat->num_draw_primitive = dbg_num_draw_primitive;
	stat->num_draw_indexed_primitive = dbg_num_draw_indexed_primitive;
	stat->num_prim_submitted = dbg_num_prim_submitted;
	stat->num_prim_batched = dbg_num_prim_batched;
	stat->num_prim_direct = dbg_num_prim_direct;
	stat->num_prim_dp = dbg_num_prim_dp;
	stat->num_prim_dip = dbg_num_prim_dip;
	
	return GR_OK;
#else
	memset( stat, 0, sizeof( RPDEVICESTATS ) );
	return GR_GENERIC;
#endif
}

//

DA_METHOD(	get_num_display_modes,(U32 *num_modes))
{
	*num_modes = 1;
	return GR_OK;
}

//

DA_METHOD(	get_display_mode,(RPDISPLAYMODEINFO *mode, U32 mode_num))
{
	mode->mode_num = 0;
	mode->width = 640;
	mode->height = 480;
	mode->render_pf.init( PixelFormat(16,5,6,5,0) );
	return GR_OK;
}

//

DA_METHOD(	get_num_device_texture_formats,(U32 *num_formats))
{
	*num_formats = 0;
	return GR_OK;
}

//

DA_METHOD(	get_device_texture_format,(PixelFormat *texture_pf, U32 format_num))
{
	texture_pf->init( PixelFormat() );
	return GR_GENERIC;
}

// ------------------------------------------------------------------
// Transformation Pipeline Related Code
// ------------------------------------------------------------------

void SetTransform( const Transform &t )
{
	Matrix4 m( t );
	m.transpose();

	glLoadMatrixf( &(m[0][0]) );
}

//

DA_METHOD(	get_window,( HWND *out_wnd, int *out_x, int *out_y, int *out_w, int *out_h  ))
{
	if( current_window == NULL ) {
		return GR_GENERIC;
	}

	*out_wnd = current_window->wnd;
	*out_x = current_window->win_x;
	*out_y = current_window->win_y;
	*out_w = current_window->win_w;
	*out_h = current_window->win_h;

	return GR_GENERIC;
}

//

DA_METHOD(	set_window,(HWND wnd, int _x, int _y, int _w, int _h ))
{
	if( wnd == NULL ) {
		return GR_GENERIC;
	}

	WindowMap::iterator window;

	if( (window = windows.find( wnd )) == windows.end() ) {

		GLWINDOWDATA new_win;
		
		new_win.wnd = wnd;
		new_win.hdc = GetDC( wnd );
		new_win.win_x = _x;
		new_win.win_y = _y;
		new_win.win_w = _w;
		new_win.win_h = _h;
		
		pixel_format_num = ChoosePixelFormat( new_win.hdc, &pixel_format );
		if( !SetPixelFormat( new_win.hdc, pixel_format_num, &pixel_format ) ) {
			return GR_GENERIC;
		}

		if( (new_win.glrc = wglCreateContext( new_win.hdc )) == NULL ) {
			return GR_GENERIC;
			
		}

		window = windows.insert( wnd, new_win );
	}

	current_window = &window->second;

	current_window->win_x = _x;
	current_window->win_y = _y;
	current_window->win_w = _w;
	current_window->win_h = _h;

	wglMakeCurrent( current_window->hdc, current_window->glrc );

	glPixelStorei( GL_PACK_ALIGNMENT, 1 );
	glPixelStorei( GL_UNPACK_ALIGNMENT, 1 );

	return GR_OK;
}

//

DA_METHOD(	set_viewport,(int x, int y, int w, int h ))
{
	if( current_window ) {
		// convert RP top-left to GL bottom-left
		glViewport( x, current_window->win_h - y - h, w, h );
		return GR_OK;
	}
	return GR_GENERIC;
}

//

DA_METHOD(	get_viewport,(int *_x, int *_y, int *_w, int *_h ))
{
	int v[4];
	
	glGetIntegerv( GL_VIEWPORT, v );
	
	*_x = v[0];
	*_y = v[1];
	*_w = v[2];
	*_h = v[3];

	return GR_OK;
}

//

DA_METHOD(get_modelview,( Transform & modelview ))
{
	Matrix4 m;
	glGetFloatv(GL_MODELVIEW_MATRIX, &(m[0][0]));

	m.transpose();
	modelview = m.get_transform();

	return GR_OK;
}

//

DA_METHOD(get_projection,( Matrix4 & projection ))
{
	glGetFloatv( GL_PROJECTION_MATRIX, &(projection[0][0]) );
	projection.transpose();
	return GR_OK;
}

//

DA_METHOD(	set_lookat,(float eyex, float eyey, float eyez, float centerx, float centery, float centerz, float upx, float upy, float upz  ))
{
	glMatrixMode( GL_MODELVIEW );
	glLoadIdentity();
	gluLookAt( eyex, eyey, eyez, centerx, centery, centerz, upx, upy, upz );
	return GR_OK;
}

//

DA_METHOD(	set_modelview,(const Transform & modelview ) )
{
	glMatrixMode( GL_MODELVIEW );
	SetTransform( modelview );
	return GR_OK;
}

//

DA_METHOD(	set_projection,(const Matrix4 & projection ) )
{
	glMatrixMode( GL_PROJECTION );
	glLoadMatrixf( &(projection.get_transpose()[0][0]) );
	return GR_OK;
}

//

DA_METHOD(	set_ortho,(float left, float right, float bottom, float top, float nearval, float farval ))
{
	glMatrixMode( GL_PROJECTION );
	glLoadIdentity();
	glOrtho( left, right, bottom, top, nearval, farval );
	return GR_OK;
}

//

#ifndef M_PI
#define M_PI  3.14159265358979323846
#endif


DA_METHOD(	set_perspective,(float fovy, float aspect, float nearval, float farval ) )
{
	if (nearval<=0.0 || farval<=0.0) {
		return GR_GENERIC;
	}

	// NOTE: we use 1/2 fov but GL uses full fov
	glMatrixMode( GL_PROJECTION );
	glLoadIdentity();
	gluPerspective( 2.0f * fovy, aspect, nearval, farval );

	return GR_OK;
}



// ------------------------------------------------------------------
// State/Scene Management Related Code
// ------------------------------------------------------------------

DA_METHOD(	set_pipeline_state,(RPPIPELINESTATE state, DWORD value ))
{
	ASSERT( state >= 0 && state <= RP_MAX_PIPELINE_STATE );
	pipeline_state[state] = value;
	return GR_OK;
}

//

DA_METHOD(	get_pipeline_state,(RPPIPELINESTATE state, DWORD *value ))
{
	ASSERT( state >= 0 && state <= RP_MAX_PIPELINE_STATE );
	*value = pipeline_state[state];
	return GR_OK;
}

//

DA_METHOD(  set_texture_stage_state,(U32 stage, D3DTEXTURESTAGESTATETYPE state, U32 value ))
{
	GENRESULT result = GR_NOT_IMPLEMENTED;

	if(stage == 0)
	{
		switch(state)
		{
			case D3DSAMP_MINFILTER:
			{
				GLint current;
				glGetTexParameteriv(GL_TEXTURE_2D,    GL_TEXTURE_MIN_FILTER, &current);

				switch(current)
				{
					case GL_NEAREST:
					case GL_LINEAR:
						switch(value)
						{
							case D3DTEXF_POINT:
								glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
								break;
							case D3DTEXF_LINEAR:
							case D3DTEXF_ANISOTROPIC:
								glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
								break;
						}
					break;

					case GL_NEAREST_MIPMAP_NEAREST:
					case GL_LINEAR_MIPMAP_NEAREST:
						switch(value)
						{
							case D3DTEXF_POINT:
								glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
								break;
							case D3DTEXF_LINEAR:
							case D3DTEXF_ANISOTROPIC:
								glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
								break;
						}
					break;

					case GL_NEAREST_MIPMAP_LINEAR:
					case GL_LINEAR_MIPMAP_LINEAR:
						switch(value)
						{
							case D3DTEXF_POINT:
								glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
								break;
							case D3DTEXF_LINEAR:
							case D3DTEXF_ANISOTROPIC:
								glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
								break;
						}
					break;
				}
				
				result = GR_OK;
				break;
			}
			case D3DSAMP_MAGFILTER:
			{
				switch(value)
				{
					case D3DTEXF_POINT:
						glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
						break;
					case D3DTEXF_LINEAR:
					case D3DTEXF_FLATCUBIC:
					case D3DTEXF_GAUSSIANCUBIC:
					case D3DTEXF_ANISOTROPIC:
						glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
						break;
				}
				result = GR_OK;
				break;
			}
			case D3DSAMP_MIPFILTER:
			{
				// NOTE: turning on a MIP filter for a non mip texture DISABLES texturing
				// so we safeguard against it -ms
				GLint width_1;
				glGetTexLevelParameteriv(GL_TEXTURE_2D, 1, GL_TEXTURE_WIDTH, &width_1);
				if(width_1 <= 0)	// NO mip levels available
				{
					value = D3DTEXF_NONE;
				}

				GLint current;
				glGetTexParameteriv(GL_TEXTURE_2D,    GL_TEXTURE_MIN_FILTER, &current);

				switch(value)
				{
					case D3DTEXF_NONE:
						switch(current)
						{
							case GL_NEAREST:
							case GL_NEAREST_MIPMAP_NEAREST:
							case GL_NEAREST_MIPMAP_LINEAR:
								glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
								break;
							case GL_LINEAR:
							case GL_LINEAR_MIPMAP_NEAREST:
							case GL_LINEAR_MIPMAP_LINEAR:
								glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
								break;
						}
						break;
					case D3DTEXF_POINT:
						switch(current)
						{
							case GL_NEAREST:
							case GL_NEAREST_MIPMAP_NEAREST:
							case GL_NEAREST_MIPMAP_LINEAR:
								glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
								break;
							case GL_LINEAR:
							case GL_LINEAR_MIPMAP_NEAREST:
							case GL_LINEAR_MIPMAP_LINEAR:
								glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
								break;
						}
						break;
					case D3DTEXF_LINEAR:
						switch(current)
						{
							case GL_NEAREST:
							case GL_NEAREST_MIPMAP_NEAREST:
							case GL_NEAREST_MIPMAP_LINEAR:
								glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
								break;
							case GL_LINEAR:
							case GL_LINEAR_MIPMAP_NEAREST:
							case GL_LINEAR_MIPMAP_LINEAR:
								glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
								break;
						}
						break;
				}
				result = GR_OK;
				break;
			}
			case D3DTSS_ADDRESSU:
			{
				switch(value)
				{
					case D3DTADDRESS_WRAP:
						glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
						result = GR_OK;
						break;
					case D3DTADDRESS_CLAMP:
						glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
						result = GR_OK;
						break;
					case D3DTADDRESS_MIRROR:
					case D3DTADDRESS_BORDER:
						result = GR_GENERIC;
						break;
					default:
						result = GR_GENERIC;
				}
				break;
			}
			case D3DTSS_ADDRESSV:
			{
				switch(value)
				{
					case D3DTADDRESS_WRAP:
						glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
						result = GR_OK;
						break;
					case D3DTADDRESS_CLAMP:
						glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
						result = GR_OK;
						break;
					case D3DTADDRESS_MIRROR:
					case D3DTADDRESS_BORDER:
						result = GR_GENERIC;
						break;
					default:
						result = GR_GENERIC;
				}
				break;
			}
			case D3DTSS_ADDRESS:
			{
				switch(value)
				{
					case D3DTADDRESS_WRAP:
						glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
						glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
						result = GR_OK;
						break;
					case D3DTADDRESS_CLAMP:
						glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
						glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
						result = GR_OK;
						break;
					case D3DTADDRESS_MIRROR:
					case D3DTADDRESS_BORDER:
						result = GR_GENERIC;
						break;
					default:
						result = GR_GENERIC;
				}
				break;
			}
		}
	}

	return result;
}

//

DA_METHOD(  get_texture_stage_state,(U32 stage, D3DTEXTURESTAGESTATETYPE, U32 *value ))
{
	return GR_GENERIC;
}

//


DA_METHOD(  set_texture_stage_texture,(U32 stage, U32 htexture ))
{
	if(stage == 0)
	{
		if( htexture )
		{
			GLenum err;
			if( (err = glGetError()) != GL_NO_ERROR )
			{
				GENERAL_TRACE_1( _MS(("GLRenderPipe: set_render_state: gl error: %d\n", err)) );
			}

			glEnable( GL_TEXTURE_2D );
			glBindTexture( GL_TEXTURE_2D, htexture );
			if( (err = glGetError()) != GL_NO_ERROR )
			{
				GENERAL_TRACE_1( _MS(("GLRenderPipe: set_render_state: gl error: %d\n", err)) );
			}
		}
		else
		{
			glDisable( GL_TEXTURE_2D );
			glBindTexture( GL_TEXTURE_2D, 0 );
		}

		return GR_OK;
	}

	return GR_GENERIC;
}

//


DA_METHOD(  get_texture_stage_texture,(U32 stage, U32 *htexture ))
{
	if(stage == 0)
	{
		glGetIntegerv(GL_TEXTURE_BINDING_2D, (int*)htexture);
		return GR_OK;
	}
	
	*htexture = 0;
	return GR_GENERIC;
}

//

DA_METHOD(	verify_state,())
{
	return GR_GENERIC;
}

//

DA_METHOD(	begin_scene,(void ) )
{
	RP_PROFILE_CLR( dbg_num_prim_submitted		);	// # of total prims submitted
	RP_PROFILE_CLR( dbg_num_prim_batched		);	// # of prims submitted as batched
	RP_PROFILE_CLR( dbg_num_prim_direct			);	// # of prims submitted as direct (non-batched)
	RP_PROFILE_CLR( dbg_num_prim_dp				);	// # of prims submitted as direct (non-batched)
	RP_PROFILE_CLR( dbg_num_prim_dip			);	// # of prims submitted as direct (non-batched)

	RP_PROFILE_CLR( dbg_num_draw_primitive			);	// # of times draw_primitive was called
	RP_PROFILE_CLR( dbg_num_draw_indexed_primitive	);	// # of times draw_indexed_primitive was called

	RP_PROFILE_CLR( dbg_num_txm_calls			);	// # of times set_render_state( _TEXTUREHANDLE foo ) called
	RP_PROFILE_CLR( dbg_num_txm_calls_zero		);	// # of times when 'foo' == 
	RP_PROFILE_CLR( dbg_num_txm_calls_nonzero	);	// # of times when 'foo' != 
	RP_PROFILE_CLR( dbg_num_txm_calls_override	);	// # of times when 'foo' is already set
	RP_PROFILE_CLR( dbg_num_txm_calls_activate	);	// # of times when 'foo' is not set
	RP_PROFILE_CLR( dbg_num_txm_calls_failed	);	// # of times when 'foo' is not set
//	RP_PROFILE_CLR( dbg_num_txm_calls_unique	);	// # of unique textures set
	RP_PROFILE_CLR( dbg_num_txm_system			);	// # of txms in system memory
	RP_PROFILE_CLR( dbg_num_txm_system_create	);	// # of times CreateSurface was called to create a system surface
	RP_PROFILE_CLR( dbg_num_txm_system_destroy	);	// # of times Release was called on a system surface
	RP_PROFILE_CLR( dbg_num_txm_device			);	// # of txms in device memory
	RP_PROFILE_CLR( dbg_num_txm_device_create	);	// # of times CreateSurface was called to create a device surface
	RP_PROFILE_CLR( dbg_num_txm_device_destroy	);	// # of times Release was called on a device surface

	return GR_OK;
}

//

DA_METHOD(	end_scene,(void ))
{
	return GR_OK;
}

//

DA_METHOD(	get_render_state,(D3DRENDERSTATETYPE state, DWORD *value ))
{
	*value = render_state[state];
	return GR_OK;
}


//

DA_METHOD(	set_render_state,(D3DRENDERSTATETYPE state, DWORD value ))
{
	GLenum current;

	switch( state ) {
	
	case D3DRS_ZENABLE:
		if( value ) {
			glEnable( GL_DEPTH_TEST );
		}
		else {
			glDisable( GL_DEPTH_TEST );
		}
		break;

	case D3DRS_ZWRITEENABLE:
		if( value ) {
			glDepthMask( GL_TRUE );
		}
		else {
			glDepthMask( GL_FALSE );
		}
		break;

	case D3DRS_ZFUNC:
		switch( value ) {
		case D3DCMP_NEVER:			
			glDepthFunc( GL_NEVER );	break;
		case D3DCMP_LESS:			
			glDepthFunc( GL_LESS );		break;
		case D3DCMP_EQUAL:			
			glDepthFunc( GL_EQUAL );	break;
		case D3DCMP_LESSEQUAL:		
			glDepthFunc( GL_LEQUAL );	break;
		case D3DCMP_GREATER:		
			glDepthFunc( GL_GREATER );	break;
		case D3DCMP_NOTEQUAL:		
			glDepthFunc( GL_NOTEQUAL );	break;
		case D3DCMP_GREATEREQUAL:	
			glDepthFunc( GL_GEQUAL );	break;
		case D3DCMP_ALWAYS:			
			glDepthFunc( GL_ALWAYS );	break;
		}
		break;

	case D3DRS_DITHERENABLE:
		if( value ) {
			glEnable( GL_DITHER );
		}
		else {
			glDisable( GL_DITHER );
		}
		break;

	case D3DRS_ALPHABLENDENABLE:
		if( value ) {
			glEnable( GL_BLEND );
		}
		else {
			glDisable( GL_BLEND );
		}
		break;

	case D3DRS_SRCBLEND:
		glGetIntegerv( GL_BLEND_DST, (int*)&current );
		switch( value ) {
		case D3DBLEND_ZERO:				glBlendFunc( GL_ZERO, current );								break;
		case D3DBLEND_ONE:				glBlendFunc( GL_ONE, current );									break;
		case D3DBLEND_SRCCOLOR:			glBlendFunc( GL_SRC_COLOR, current );							break;
		case D3DBLEND_INVSRCCOLOR:		glBlendFunc( GL_ONE_MINUS_SRC_COLOR, current );					break;
		case D3DBLEND_SRCALPHA:			glBlendFunc( GL_SRC_ALPHA, current );							break;
		case D3DBLEND_INVSRCALPHA:		glBlendFunc( GL_ONE_MINUS_SRC_ALPHA, current );					break;
		case D3DBLEND_DESTALPHA:		glBlendFunc( GL_DST_ALPHA, current );							break;
		case D3DBLEND_INVDESTALPHA:		glBlendFunc( GL_ONE_MINUS_DST_ALPHA, current );					break;
		case D3DBLEND_DESTCOLOR:		glBlendFunc( GL_DST_COLOR, current );							break;
		case D3DBLEND_INVDESTCOLOR:		glBlendFunc( GL_ONE_MINUS_DST_COLOR, current );					break;
		case D3DBLEND_BOTHSRCALPHA:		glBlendFunc( GL_SRC_ALPHA, GL_SRC_ALPHA );						break;
		case D3DBLEND_BOTHINVSRCALPHA:	glBlendFunc( GL_ONE_MINUS_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );	break;
		}
		break;

	case D3DRS_DESTBLEND:
		glGetIntegerv( GL_BLEND_SRC, (int*)&current );
		switch( value ) {
		case D3DBLEND_ZERO:				glBlendFunc( current, GL_ZERO );								break;
		case D3DBLEND_ONE:				glBlendFunc( current, GL_ONE );									break;
		case D3DBLEND_SRCCOLOR:			glBlendFunc( current, GL_SRC_COLOR );							break;
		case D3DBLEND_INVSRCCOLOR:		glBlendFunc( current, GL_ONE_MINUS_SRC_COLOR );					break;
		case D3DBLEND_SRCALPHA:			glBlendFunc( current, GL_SRC_ALPHA );							break;
		case D3DBLEND_INVSRCALPHA:		glBlendFunc( current, GL_ONE_MINUS_SRC_ALPHA );					break;
		case D3DBLEND_DESTALPHA:		glBlendFunc( current, GL_DST_ALPHA );							break;
		case D3DBLEND_INVDESTALPHA:		glBlendFunc( current, GL_ONE_MINUS_DST_ALPHA );					break;
		case D3DBLEND_DESTCOLOR:		glBlendFunc( current, GL_DST_COLOR );							break;
		case D3DBLEND_INVDESTCOLOR:		glBlendFunc( current, GL_ONE_MINUS_DST_COLOR );					break;
		case D3DBLEND_BOTHSRCALPHA:		glBlendFunc( GL_SRC_ALPHA, GL_SRC_ALPHA );						break;
		case D3DBLEND_BOTHINVSRCALPHA:	glBlendFunc( GL_ONE_MINUS_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );	break;
		}
		break;

	case D3DRS_CULLMODE:
		switch( value ) {
		case D3DCULL_NONE:	
			glDisable( GL_CULL_FACE );							
			break;
		case D3DCULL_CCW:	
			glCullFace( GL_FRONT );	
//			glFrontFace( GL_CCW );
			glEnable( GL_CULL_FACE );	
			break;
		case D3DCULL_CW:	
			glCullFace( GL_BACK );	
//			glFrontFace( GL_CCW );
			glEnable( GL_CULL_FACE );	
			break;
		}
		break;

	case D3DRS_SHADEMODE:
		switch( value ) {
		case D3DSHADE_FLAT:		glShadeModel( GL_FLAT );	break;
		case D3DSHADE_GOURAUD:	glShadeModel( GL_SMOOTH );	break;
		}
		break;

	case D3DRS_FILLMODE:
		switch( value ) {
		case D3DFILL_SOLID:		glPolygonMode( GL_FRONT_AND_BACK, GL_FILL ); break;
		case D3DFILL_WIREFRAME:	glPolygonMode( GL_FRONT_AND_BACK, GL_LINE ); break;
		case D3DFILL_POINT:		glPolygonMode( GL_FRONT_AND_BACK, GL_POINT ); break;
		}
		break;

	case D3DRS_FOGENABLE:
		switch( value ) {
		case TRUE:				glEnable( GL_FOG ); break;
		case FALSE:				glDisable( GL_FOG ); break;
		}
		break;

	case D3DRS_FOGCOLOR:
		{
			unsigned char *vpt = (unsigned char*)&value;

			GLfloat cv[4] = { (1.0/255.0) * vpt[2],
							  (1.0/255.0) * vpt[1],
							  (1.0/255.0) * vpt[0],
							  (1.0/255.0) * vpt[3] };
			glFogfv( GL_FOG_COLOR, cv);
		}
		break;

	case D3DRS_FOGTABLEMODE:
		switch( value ) {
		case D3DFOG_NONE:
			glDisable( GL_FOG );
			break;
		case D3DFOG_EXP:
			glEnable( GL_FOG );
			glFogi( GL_FOG_MODE, GL_EXP);
			break;
		case D3DFOG_EXP2:
			glEnable( GL_FOG );
			glFogi( GL_FOG_MODE, GL_EXP2);
			break;
		case D3DFOG_LINEAR:
			glEnable( GL_FOG );
			glFogi( GL_FOG_MODE, GL_LINEAR);
			break;
		}
		break;

	case D3DRS_FOGSTART:
		glFogf( GL_FOG_START, *(float*)&(value));
		break;

	case D3DRS_FOGEND:
		glFogf( GL_FOG_END, *(float*)&(value));
		break;

	case D3DRS_FOGDENSITY:
		glFogf( GL_FOG_DENSITY, *(float*)&(value));
		break;

	case D3DRS_SPECULARENABLE:
		// avoid GENERAL_TRACE_5 below
		break;

	default:
		GENERAL_TRACE_5( _MS(( "GLRenderPipeline: set_render_state: %d %d unsupported\n", state, value)) );
		break;
	}
	render_state[state] = value;

	return GR_OK;
}


// ------------------------------------------------------------------
// Primitive/List Rendering/Management Related Code
// ------------------------------------------------------------------

inline void Swap8(U8 * const a, U8 * const b)
{	
	*a ^= *b;
	*b ^= *a;
	*a ^= *b;
}

//

// packs vertices from D3D(vertex,color,uv) format to GL(uv,color,vertex) format
bool RenderPipeline::pack(U32 vertex_format, const void *_verts, int num_verts)
{
	bool result = false;

	verify_packed_buffer( num_verts );

	const int step_size = FVF_SIZEOF_VERT(vertex_format);
	const int col_ofs = FVF_COLOR_OFS(vertex_format);
	const int tex_ofs = FVF_TEXCOORD_U0_OFS(vertex_format);

	if( tex_ofs > 0 && col_ofs > 0 )
	{
		GLVertex *pb = packed_buffer;

		const U8 * const end = ((const U8 const *)_verts) + num_verts * step_size;
		for( const U8 * vert = (const U8*)_verts;
			 vert < end;
			 vert += step_size, pb++ )
		{
			const float *uv = (const float*)(vert + tex_ofs);
			pb->u = uv[0];
			pb->v = uv[1];

			const U32 *color = (const U32*)(vert + col_ofs);
			pb->color = RI(*color) | GI(*color) | BI(*color) | AI(*color);

			pb->pos = *(const Vector*)vert;
		}

		result = true;
	}
	
	return result;
}

//

DA_METHOD(	draw_primitive,(D3DPRIMITIVETYPE type, U32 vertex_format, const void *_verts, int num_verts, U32 flags ))
{
	if( !_verts || !num_verts ) {
		return GR_GENERIC;
	}

#if 0 // uses packed GL arrays
	if( pack( vertex_format, _verts, num_verts ) )
	{
		glInterleavedArrays(GL_T2F_C4UB_V3F, 0, packed_buffer);
 
		glDrawArrays(d3dprim_to_glprm[type], 0, num_verts);

		return GR_OK;
	}
#endif

	const int step_size = FVF_SIZEOF_VERT(vertex_format);
	const int col_ofs = FVF_COLOR_OFS(vertex_format);
	const int tex_ofs = FVF_TEXCOORD_U0_OFS(vertex_format);

#if 0 // uses unpacked GL arrays
	glEnableClientState (GL_VERTEX_ARRAY);
	glVertexPointer (3, GL_FLOAT, step_size, _verts );

	if( col_ofs > 0 )
	{
		// BGRA to RGBA  (this violates the const void *_verts)
		U8 * col_verts = (U8*)_verts + col_ofs;
		const U8 * const col_end = col_verts + num_verts * step_size;
		for(; col_verts < col_end; col_verts += step_size)
		{
			Swap8(col_verts, col_verts + 2);
		}

		glEnableClientState (GL_COLOR_ARRAY);
		glColorPointer (4, GL_UNSIGNED_BYTE, step_size, (U8*)_verts + col_ofs);
	}

	if( tex_ofs > 0 )
	{
		glEnableClientState (GL_TEXTURE_COORD_ARRAY);
		glTexCoordPointer (2, GL_FLOAT, step_size, (U8*)_verts + tex_ofs);
	}

	glDrawArrays(d3dprim_to_glprm[type], 0, num_verts);

	glDisableClientState (GL_VERTEX_ARRAY);
	if( col_ofs )
		glDisableClientState (GL_COLOR_ARRAY);
	if( tex_ofs )
		glDisableClientState (GL_TEXTURE_COORD_ARRAY);

#else

	const U8 * const end = ((const U8 const *)_verts) + num_verts * step_size;

	glBegin( d3dprim_to_glprm[type] );
	for( const U8 * vert = (const U8*)_verts;
		 vert < end;
		 vert += step_size )
	{
		const U32 *color = (const U32*)(vert + col_ofs);
		glColor4ub( R(*color), G(*color), B(*color), A(*color) );

		glTexCoord2fv( (const float*)(vert + tex_ofs) );

		glVertex3fv( (const float*)vert );
	}
	glEnd();
#endif
	return GR_OK;
}

//

DA_METHOD(	draw_indexed_primitive,(D3DPRIMITIVETYPE type, U32 vertex_format, const void *_verts, int num_verts, const U16 * indices, int num_indices, U32 flags ))
{
	if (!_verts || !num_verts || !indices || !num_indices) {
		return GR_GENERIC;
	}
	
#if 0 // uses packed GL arrays
	if( pack( vertex_format, _verts, num_verts ) )
	{
		glInterleavedArrays(GL_T2F_C4UB_V3F, 0, packed_buffer);
 
		glDrawElements (d3dprim_to_glprm[type], num_indices, GL_UNSIGNED_SHORT, indices);

		return GR_OK;
	}
#endif

	const int step_size = FVF_SIZEOF_VERT(vertex_format);
	const int col_ofs = FVF_COLOR_OFS(vertex_format);
	const int tex_ofs = FVF_TEXCOORD_U0_OFS(vertex_format);

#if 0 // uses unpacked GL arrays
	glEnableClientState (GL_VERTEX_ARRAY);
	glVertexPointer (3, GL_FLOAT, step_size, _verts );

	if( col_ofs > 0 )
	{
		// BGRA to RGBA (this violates the const void *_verts)
		U8 * col_verts = (U8*)_verts + col_ofs;
		const U8 * const col_end = col_verts + num_verts * step_size;
		for(; col_verts < col_end; col_verts += step_size)
		{
			Swap8(col_verts, col_verts + 2);
		}
		
		glEnableClientState (GL_COLOR_ARRAY);
		glColorPointer (4, GL_UNSIGNED_BYTE, step_size, (U8*)_verts + col_ofs);
	}

	if( tex_ofs > 0 )
	{
		glEnableClientState (GL_TEXTURE_COORD_ARRAY);
		glTexCoordPointer (2, GL_FLOAT, step_size, (U8*)_verts + tex_ofs);
	}

	glDrawElements (d3dprim_to_glprm[type], num_indices, GL_UNSIGNED_SHORT, indices);

	glDisableClientState (GL_VERTEX_ARRAY);
	if( col_ofs )
		glDisableClientState (GL_COLOR_ARRAY);
	if( tex_ofs )
		glDisableClientState (GL_TEXTURE_COORD_ARRAY);

#else
	
	const U16 * const end = indices + num_indices;
	glBegin( d3dprim_to_glprm[type] );
	for( const U16 *idx = indices; idx < end; idx++ ) {

		const U8 * const vert = ((const U8 const *)_verts) + *idx * step_size;

		const U32 *color = (const U32*)(vert + col_ofs);
		glColor4ub( R(*color), G(*color), B(*color), A(*color) );

		glTexCoord2fv( (const float*)(vert + tex_ofs) );

		glVertex3fv( (const float*)vert );
	}
	glEnd();
#endif
	return GR_OK;
}


// ------------------------------------------------------------------
// Textures and TextureManagement Related code
// ------------------------------------------------------------------

DA_METHOD(  create_texture,(int width, int height, const PixelFormat &desiredformat, int numMipLevels, U32 irp_ctf_flags, U32 &out_handle  ))
{
	unsigned int h;
	glGenTextures( 1, &h );
	GLenum err = glGetError();
	if( err != GL_NO_ERROR ) {
		GENERAL_TRACE_1( _MS(("GLRenderPipe: create_texture: gl error: %d\n", err)) );
	}
	out_handle = (U32)h;
	return GR_OK;
}

//

DA_METHOD(	destroy_texture,(U32 htexture ) )
{
#if 0
	glDeleteTextures( 1, (const unsigned int*)&htexture );
#endif
	return GR_GENERIC;
}

//

DA_METHOD(	is_texture,	(U32 htexture ))
{
	ASSERT( htexture != RP_CURRENT );

	if( glIsTexture( htexture ) ) {
		return GR_OK;
	}
	
	return GR_GENERIC;
}

//

DA_METHOD(	get_texture_format,( U32 htexture, PixelFormat *out_pf ))
{	
	ASSERT( htexture != RP_CURRENT );

	glPushAttrib( GL_TEXTURE_BIT );
	glBindTexture( GL_TEXTURE_2D, htexture );

	GLint comp;
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_COMPONENTS, &comp);

	switch(comp)
	{
		case GL_COLOR_INDEX:
			out_pf->init( PF_COLOR_INDEX );
			break;
		case GL_RGB:
			out_pf->init( PF_RGB );
			break;
		case GL_RGBA:
			out_pf->init( PF_RGBA );
			break;
		default:
			out_pf->init( PixelFormat() );
	}

	glPopAttrib();
	
	return GR_OK;
}

//

DA_METHOD(	get_texture_dim,(U32 htexture, U32 *out_width, U32 *out_height, U32 *out_num_lod ))
{
	ASSERT( htexture != RP_CURRENT );

	glPushAttrib( GL_TEXTURE_BIT );
	glBindTexture( GL_TEXTURE_2D, htexture );
	glGetTexLevelParameteriv( GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, (int*)out_width );
	glGetTexLevelParameteriv( GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, (int*)out_height );
	
	U32 w = *out_width;
	U32 h = *out_height;
	U32 l = 0;
	while( w>1 && h>1 ) {
		l++;
		w>>=1;
		h>>=1;
	}
	*out_num_lod = l;

	glPopAttrib();

	return GR_OK;
}


//

DA_METHOD(	lock_texture,(U32 htexture, int level, RPLOCKDATA *lockData ))
{
	return GR_GENERIC;
}

//
DA_METHOD(	unlock_texture,(U32 htexture, int level ))
{
	return GR_GENERIC;
}

//

DA_METHOD(	set_texture_palette,(U32 htexture, int start, int length, const RGB *colors ))
{
	return GR_GENERIC;
}

//

DA_METHOD(	get_texture_palette,(U32 htexture, int start, int length, RGB *colors ))
{
	return GR_GENERIC;
}

//

DA_METHOD(	get_texture_interface,	(U32 htexture, const char *iid, void **out_iif ))
{
	return GR_NOT_IMPLEMENTED;
}

//

DA_METHOD(	get_buffer_interface,		(const char *iid, void **out_iif ))
{
	return GR_NOT_IMPLEMENTED;
}

//

DA_METHOD(	blit_texture,(U32 hDest, U32 destLevel, RECT destRect, U32 hSrc, U32 srcLevel, RECT srcRect ))
{
	return GR_GENERIC;
}

/*
void __stdcall SetColorTables (GLsizei count, GLenum fmt, const GLvoid *data)
{
	if (count > 256)
		count = 256;

	if (fmt == GL_RGB)
	{
		const tagRGB *palette = (const tagRGB *)data;

		float table[256];
		double i2f = 1.0/255;
		int i;
		for (i=0; i<count; i++) { table[i] = palette[i].r * i2f; }
		glPixelMapfv(GL_PIXEL_MAP_I_TO_R,count,table);
		for (i=0; i<count; i++) { table[i] = palette[i].g * i2f; }
		glPixelMapfv(GL_PIXEL_MAP_I_TO_G,count,table);
		for (i=0; i<count; i++) { table[i] = palette[i].b * i2f; }
		glPixelMapfv(GL_PIXEL_MAP_I_TO_B,count,table);
	}
	else
	if (fmt == GL_RGBA)
	{
		const tagRGBA *palette = (const tagRGBA *)data;

		float table[256];
		double i2f = 1.0/255;
		int i;
		for (i=0; i<count; i++) { table[i] = palette[i].r * i2f; }
		glPixelMapfv(GL_PIXEL_MAP_I_TO_R,count,table);
		for (i=0; i<count; i++) { table[i] = palette[i].g * i2f; }
		glPixelMapfv(GL_PIXEL_MAP_I_TO_G,count,table);
		for (i=0; i<count; i++) { table[i] = palette[i].b * i2f; }
		glPixelMapfv(GL_PIXEL_MAP_I_TO_B,count,table);
		for (i=0; i<count; i++) { table[i] = palette[i].a * i2f; }
		glPixelMapfv(GL_PIXEL_MAP_I_TO_A,count,table);
	}
	else
	{
		DebugPrint("Display: dgluSetColorTables\n   parameters not supported?\n");
	}
}
*/

//
DA_METHOD(	set_texture_level_data,(U32 htexture, int level, int srcWidth, int srcHeight, int srcStride, const PixelFormat &srcFormat, const void *srcPixels, const void *srcAlphaMap, const RGB *srcPalette ))
{
	ASSERT( htexture != RP_CURRENT );

	U8 *img_data = new unsigned char[4*srcWidth*srcHeight];
	if( img_data == NULL ) {
		return GR_GENERIC;

	}
	U8 r,g,b,a = 0xFF;
	U8 *source = (U8*)srcPixels;
	U8 *src_map = NULL;
	U8 *pxl;
	U8 *dst_map = (U8*)img_data;
	const U8 *a_map;
	U32 sbd = ((srcFormat.ddpf.dwRGBBitCount+7)>>3);	// src byte depth
	SHORT pxl_16b;
		
	// stride is the number of bytes beyond the width of the surface
	for( S32 y=0; y<srcHeight; y++ ) {
		
		src_map = &source[srcStride * y];
		if( srcAlphaMap ) {
			a_map = &((U8*)srcAlphaMap)[srcWidth * y];
		}

		for( S32 x=0; x<srcWidth; x++ ) {
			pxl = &src_map[ sbd * x ];
			a = 0xFF;
			if( srcAlphaMap ) {
				a = a_map[ x ];
			}

			switch( srcFormat.ddpf.dwRGBBitCount ) {
			case 8:	
				r = srcPalette[*pxl].r;
				g = srcPalette[*pxl].g;
				b = srcPalette[*pxl].b;
				break;

			case 16:
				// extract the rgb
				//
				pxl_16b = (*((SHORT*)pxl));
				r = ((pxl_16b & srcFormat.ddpf.dwRBitMask) >> srcFormat.rl) << srcFormat.rr;
				g = ((pxl_16b & srcFormat.ddpf.dwGBitMask) >> srcFormat.gl) << srcFormat.gr;
				b = ((pxl_16b & srcFormat.ddpf.dwBBitMask) >> srcFormat.bl) << srcFormat.br;
				if( !srcAlphaMap && srcFormat.has_alpha_channel() ) {
					a = (pxl_16b & srcFormat.ddpf.dwRGBAlphaBitMask) >> srcFormat.al;
				}
				pxl += 2;
				break;

			case 24:
				r = *pxl; pxl++;
				g = *pxl; pxl++;
				b = *pxl; pxl++;
				break;

			case 32:
				r = *pxl; pxl++;
				g = *pxl; pxl++;
				b = *pxl; pxl++;
				a = *pxl; pxl++;
				break;
			}

			*dst_map = r; dst_map++;
			*dst_map = g; dst_map++;
			*dst_map = b; dst_map++;
			*dst_map = a; dst_map++;
		}
	} 

	GLenum err;

	glBindTexture( GL_TEXTURE_2D, htexture );
	if( (err = glGetError()) != GL_NO_ERROR ) {
		GENERAL_TRACE_1( _MS(("GLRenderPipe: set_texture_level_data: gl error: %d\n", err)) );
	}
//	glPixelStorei( GL_UNPACK_ALIGNMENT, 1 );

	// set filter defaults
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	if(level > 0)
	{
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
	}
	else
	{
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	}

	/* // add later for 8 bit support
	SetColorTables(256, GL_RGB, bmp.palette);
	internal_fmt = (GLenum)3;
	glTexImage2D(GL_TEXTURE_2D, level, internal_fmt, bmp.width, bmp.height, 0, GL_COLOR_INDEX,
		GL_UNSIGNED_BYTE, bmp.pixels);
	*/

	if( srcAlphaMap || srcFormat.has_alpha_channel() ) {
		glTexImage2D( GL_TEXTURE_2D, level, GL_RGBA, srcWidth, srcHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, img_data );
	}
	else {
		glTexImage2D( GL_TEXTURE_2D, level, GL_RGB, srcWidth, srcHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, img_data );
	}
	
	if( (err = glGetError()) != GL_NO_ERROR ) {
		GENERAL_TRACE_1( _MS(("GLRenderPipe: set_texture_level_data: gl error: %d\n", err)) );
	}

	delete[] img_data;

	return GR_OK;
}


DA_METHOD(  get_num_textures,(U32 *out_num_textures ))
{
	*out_num_textures = 0;
	return GR_OK;
}


DA_METHOD(  get_texture,(U32 texture_num, U32 *out_htexture ))
{
	*out_htexture = 0;
	return GR_OK;
}


// ------------------------------------------------------------------
// Render Device/Surface Related code
// ------------------------------------------------------------------


DA_METHOD(	get_buffer_dc,(HDC *dc ))
{
//	*dc = GetDC( internal_get_hwnd() );
	*dc = NULL;
	return GR_GENERIC;
}

DA_METHOD(	release_buffer_dc,(HDC dc ))
{
//	ReleaseDC( internal_get_hwnd(), dc );
	return GR_OK;
}


DA_METHOD(	clear_buffers,(DWORD flag, RECT *rect ))
{
	U32 flags = 0;

	if( flag & RP_CLEAR_COLOR_BIT ) {
		flags |= GL_COLOR_BUFFER_BIT;
	}
	if( flag & RP_CLEAR_DEPTH_BIT ) {
		flags |= GL_DEPTH_BUFFER_BIT;
	}

	glClearColor( ((float)R(pipeline_state[RP_CLEAR_COLOR]) / 255.0),
		          ((float)G(pipeline_state[RP_CLEAR_COLOR]) / 255.0),
				  ((float)B(pipeline_state[RP_CLEAR_COLOR]) / 255.0),
				  ((float)A(pipeline_state[RP_CLEAR_COLOR]) / 255.0) );

	double depth = (double)((double)pipeline_state[RP_CLEAR_DEPTH]/(double)0xFFFFFFFF);

	glPushAttrib( GL_DEPTH_BUFFER_BIT  );
	glDepthMask( GL_TRUE );

	glClearDepth( depth );
	ASSERT( GL_NO_ERROR == glGetError() );

	glClear( flags );
	ASSERT( GL_NO_ERROR == glGetError() );

	glPopAttrib( );

	return GR_OK;
}


DA_METHOD(	swap_buffers,(void ))
{
	HDC hdc = GetDC( internal_get_hwnd() );
	SwapBuffers( hdc );
	ReleaseDC( internal_get_hwnd(), hdc );
	return GR_OK;
}


DA_METHOD(	lock_buffer,(RPLOCKDATA *lockData ))
{
	lockData->width = pipeline_state[RP_BUFFERS_WIDTH];
	lockData->height = pipeline_state[RP_BUFFERS_HEIGHT];
	lockData->pitch = pipeline_state[RP_BUFFERS_WIDTH] * 4;
	lockData->pixels = frame_buffer_mirror;
	if( frame_buffer_mirror_stretch_hack ) {
		lockData->pf.init( 16, 5,6,5,0 );
	}
	else {
		lockData->pf.init(PF_BGRA_EXT);
	}

	if( !frame_buffer_mirror_no_read_on_lock ) {
		glPushClientAttrib( GL_CLIENT_PIXEL_STORE_BIT );
		glPixelStorei( GL_UNPACK_ALIGNMENT, 1 );
		glPixelStorei( GL_UNPACK_ROW_LENGTH, pipeline_state[RP_BUFFERS_WIDTH] );
		
		glReadPixels( 0, 0, lockData->width, lockData->height, GL_BGRA_EXT, GL_UNSIGNED_BYTE, frame_buffer_mirror );
		
		glPopClientAttrib( );
	
		ASSERT( GL_NO_ERROR == glGetError() );
	}

	return GR_OK;
}

//

DA_METHOD(	unlock_buffer,(void ))
{
	int w = pipeline_state[RP_BUFFERS_WIDTH];
	int h = pipeline_state[RP_BUFFERS_HEIGHT];

	glPushClientAttrib( GL_CLIENT_PIXEL_STORE_BIT );
	glPixelStorei( GL_UNPACK_ALIGNMENT, 1 );
	glPixelStorei( GL_UNPACK_ROW_LENGTH, w );

	glViewport( 0, 0, w, h );
	glMatrixMode( GL_PROJECTION );
	glPushMatrix();
	glLoadIdentity();
	gluOrtho2D( 0.0, (GLdouble)w, (GLdouble)h, (GLdouble)0 );

	glMatrixMode( GL_MODELVIEW );
	glPushMatrix();
	glLoadIdentity();
	
	glPushAttrib( GL_PIXEL_MODE_BIT | GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_VIEWPORT_BIT | GL_PIXEL_MODE_BIT | GL_CURRENT_BIT );
	glDisable( GL_DEPTH_TEST );
	
	glPixelZoom( 1.0, -1.0 );
	glDisable(GL_BLEND);
	glDisable(GL_TEXTURE_2D);
	glRasterPos2i( 0, 0 );

	if( frame_buffer_mirror_stretch_hack )
	{
		// convert from current screen format to 32 bit BGRA
		// for now assume GL RP only supports one display mode 16:5650
		//
		for(int row = h - 1; row >= 0; row--)
		{
			const U16 *src = (U16*)(frame_buffer_mirror + 4 * row * w + 2 * (w - 1));
				  U32 *dst = (U32*)(frame_buffer_mirror + 4 * row * w + 4 * (w - 1));

			for(int col = w - 1; col >= 0; col--)
			{
				*dst = ARGB_MAKE( ((*src & 0xF800) >> 6+5) << 3, ((*src & 0x07E0) >> 5) << 2, ((*src & 0x001F) >> 0) << 3, 0xFF );
			
				dst--;
				src--;
			}
		}
	}

	glDrawPixels( w, h, GL_BGRA_EXT, GL_UNSIGNED_BYTE, frame_buffer_mirror );

	glPopAttrib();
	glPopClientAttrib();
	glPopMatrix();
	glMatrixMode( GL_PROJECTION );
	glPopMatrix();

	ASSERT( GL_NO_ERROR == glGetError() );

	return GR_OK;
}


DA_METHOD(	create_buffers,(HWND hwnd, int hres, int vres ))
{
	destroy_buffers();

	// create GL context
	PIXELFORMATDESCRIPTOR pfd = {
		sizeof(PIXELFORMATDESCRIPTOR),
		1,
		PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_GENERIC_FORMAT,
		PFD_TYPE_RGBA,
		pipeline_state[RP_BUFFERS_COLOR_BPP],
		0,0,0,0,0,0,
		0,
		0,
		0,
		0,0,0,0,
		pipeline_state[RP_BUFFERS_DEPTH_BPP],
		pipeline_state[RP_BUFFERS_STENCIL_BPP],
		0,
		PFD_MAIN_PLANE,
		0,
		0,0,0
	};

	if( pipeline_state[RP_BUFFERS_COUNT] > 1 ) {
		pfd.dwFlags |= PFD_DOUBLEBUFFER;
	}

	if( strcmp( current_device_type, "Software" ) == 0 ) {
		// TODO: force software somehow
	}

	pixel_format = pfd;

	RECT r;

	GetClientRect( hwnd, &r );
	set_window( hwnd, r.left, r.top, r.right, r.bottom );

	glEnable( GL_TEXTURE_2D );

	internal_set_default_render_state();
	internal_set_default_transform_state();

	pipeline_state[RP_BUFFERS_WIDTH] = hres;
	pipeline_state[RP_BUFFERS_HEIGHT] = vres;

	frame_buffer_mirror = new unsigned char[hres * vres * 4];
	
	return GR_OK;
}


DA_METHOD(	destroy_buffers,(void ))
{
	current_window = NULL;

	WindowMap::iterator window;

	while( (window = windows.begin()) != windows.end() ) {

		wglMakeCurrent( window->second.hdc, window->second.glrc );
		wglMakeCurrent( window->second.hdc, NULL );
		wglDeleteContext( window->second.glrc );
		ReleaseDC( window->second.wnd, window->second.hdc );
		
		windows.erase( window );
	}

	delete [] frame_buffer_mirror;
	frame_buffer_mirror = NULL;

	packed_length = 0;
	delete [] packed_buffer;
	packed_buffer = NULL;

	return GR_OK;
}


DA_METHOD(	set_depth_range,( float lower_z_bound, float upper_z_bound ))
{
	glDepthRange(lower_z_bound, upper_z_bound);
 
	return GR_OK;
}

DA_METHOD(	get_depth_range,( float *out_lower_z_bound, float *out_upper_z_bound ))
{
	float range[2];

	glGetFloatv(GL_DEPTH_RANGE, range);

	*out_lower_z_bound = range[0];
	*out_upper_z_bound = range[1];

	return GR_OK;
}

//
// IRenderPrimitive
//

DA_METHOD(	set_state,(RPRSTATE state, U32 value ))
{
	rpr_state[state] = value;
	return GR_OK;
}

//

DA_METHOD(	get_state,(RPRSTATE state, U32 *value ))
{
	*value = rpr_state[state];
	return GR_OK;
}

//

DA_METHOD(	flush,(U32 flags))
{
	return GR_OK;
}


// ------------------------------------------------------------------
// DLL Related code
// ------------------------------------------------------------------


BOOL COMAPI DllMain( HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved )
{
	ICOManager *DACOM = NULL;				// Handle to component manager
	IComponentFactory *server1;

	switch (fdwReason)
	{
		case DLL_PROCESS_ATTACH:
			DA_HEAP_ACQUIRE_HEAP(HEAP);
			DA_HEAP_DEFINE_HEAP_MESSAGE(hinstDLL);

			DACOM = DACOM_Acquire();
			if( DACOM == NULL ) {
				GENERAL_TRACE_1( "OpenGL_RenderPipeline: DllMain: unable to get DACOM!\n" );
				break;
			}

			if( (server1 = new DAComponentFactory2<DAComponentAggregate<RenderPipeline>, AGGDESC>( CLSID_OGL_RenderPipeline )) != NULL ) {
				DACOM->RegisterComponent( server1, CLSID_OGL_RenderPipeline, DACOM_LOW_PRIORITY );
				server1->Release();
			}

			if( (server1 = new DAComponentFactory2<DAComponentAggregate<RenderPipeline>, AGGDESC>( CLSID_IRenderPipeline )) != NULL ) {
				DACOM->RegisterComponent( server1, CLSID_IRenderPipeline, DACOM_LOW_PRIORITY );
				server1->Release();
			}

			break;
	}

	return TRUE;
}

//

// EOF
