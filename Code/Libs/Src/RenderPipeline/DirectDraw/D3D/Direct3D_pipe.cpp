// Direct3D_pipe.cpp
//
// A Direct3D_RenderPipeline class component that renders using
// a Direct3D device.  Supports all available DX6 D3D devices
// including the software renderer, but not including the
// DX5 ramp renderer.
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

#define DIRECT3D_VERSION         0x0900
#include <d3d9.h>
#include <d3dx9.h>


#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
//#define INITGUID
//#include <ddraw.h>
//#include <d3d9types.h>

#include <vector>
#include <map>

#include "dacom.h"
#include "FDump.h"
#include "FVF.h"
#include "da_heap_utility.h"
#include "tcomponent.h"
#include "tsmartpointer.h"
#include "IProfileParser_Utility.h"
#include "rendpipeline.h"
#include "IRenderPrimitive.h"
#include "IGammaControl.h"
#include "IDDBackDoor.h"
#include "IVideoStreamControl.h"
#include "IRenderDebugger.h"
#include "RPUL.h"

#include "../../../include/handlemap.h"
#include "../../../include/Tfuncs.h"

//



#if !defined(FINAL_RELEASE)
#define ENABLE_STARTUP_CHECK		1
#define ENABLE_CREATE_BUFFERS_CHECK 1
#define ENABLE_RENDERSTATE_CHECK	1
#else
#define ENABLE_STARTUP_CHECK		0
#define ENABLE_CREATE_BUFFERS_CHECK 0
#define ENABLE_RENDERSTATE_CHECK	0
#endif


//

#include "RenderDebugger.h"
#include "StateInfo.h"
#include "StateCache.h"

//#include "../DirectDraw.h"
//#include "../DirectDrawTexture.h"
#include "../DirectShow.h"
#include "../VideoForWindows.h"

//

//

#if (ENABLE_STARTUP_CHECK)
#define CHECK_STARTUP(function)	if( (d3drp_f_flags & D3DRP_F_CHECK_STARTUP) && !(d3drp_f_flags & D3DRP_F_PIPELINE_CREATED) ) { GENERAL_WARNING( "" # function "() called outside of startup/shutdown" ); return GR_GENERIC; }
#else
#define CHECK_STARTUP(function)
#endif

//

#if (ENABLE_CREATE_BUFFERS_CHECK)
#define CHECK_CREATE_BUFFERS(function)	if( (d3drp_f_flags & D3DRP_F_CHECK_CREATE_BUFFERS) && !(d3drp_f_flags & D3DRP_F_BUFFERS_CREATED) ) { GENERAL_WARNING( "" # function "() called outside of create_buffers/destroy_buffers" ); return GR_GENERIC; }
#else
#define CHECK_CREATE_BUFFERS(function)
#endif

//

#define INTERNAL_DEFINE_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8)  EXTERN_C const GUID name = { l, w1, w2, { b1, b2,  b3,  b4,  b5,  b6,  b7,  b8 } }

// Type ID used as the key when associating the videostream with a DD surface.
INTERNAL_DEFINE_GUID( TID_IVideoStreamControl, 0xe763cb30, 0x1d22, 0x45b7, 0xbc, 0xfb, 0x12, 0x7d, 0x78, 0x91, 0xd8, 0x74 );

// GUID of the Blade device used in CreateDevice
INTERNAL_DEFINE_GUID( IID_IDirect3DBladeDevice, 0x706E9520, 0x6DB1, 0x11D2, 0xB9, 0x01, 0x00, 0x10, 0x4B, 0x36, 0xAE, 0x4B );

//	// Type ID used as the key when associating the videostream with a DD surface.
//	DEFINE_GUID( TID_IVideoStreamControl, 0xe763cb30, 0x1d22, 0x45b7, 0xbc, 0xfb, 0x12, 0x7d, 0x78, 0x91, 0xd8, 0x74 );
//
//	// GUID of the Blade device used in CreateDevice
//	DEFINE_GUID( IID_IDirect3DBladeDevice, 0x706E9520, 0x6DB1, 0x11D2, 0xB9, 0x01, 0x00, 0x10, 0x4B, 0x36, 0xAE, 0x4B );



const char *CLSID_IRenderPipeline	= "IRenderPipeline";
const char *CLSID_D3D_RenderPipeline= "D3D_RenderPipeline";
const char *DeviceProfile_Default	= "RenderPipeline";
const char *DeviceClass_Default		= "Direct3D";
const char *DeviceId_Default		= "{00000000-0000-0000-0000-000000000000}";
const char *DeviceType_Default		= "Hardware";
const char *DirectXDllName			= "ddraw.dll";
const char *BladeDllName			= "blade.dll";

//

const U32 D3DRP_MAX_STRING_LEN = 128;

LPDIRECT3DINDEXBUFFER9 scratchIB = 0;

//

#define D3DRP_DTF_SOFTWARE	(1<<0)

struct D3DRPDEVICETYPE
{
	const char   *nickname;
	const char   *fullname;
	const char   *dll_name;
	GUID		  type_guid;
	U32			  d3drp_dtf_flags;

	D3DRPDEVICETYPE( const char *_nickname=NULL, const char *_fullname=NULL, const GUID *_type_guid=NULL, U32 _flags = 0, const char *_dll_name = NULL )
	{
		nickname = _nickname;
		fullname = _fullname;
		dll_name = _dll_name;
		d3drp_dtf_flags = _flags;
		if( _type_guid ) {
			memcpy( &type_guid, _type_guid, sizeof(type_guid) );
		}
	}
};


//

typedef std::vector<PixelFormat>			TextureFormatArray;
typedef std::vector<RPTEXTUREFORMATCLASS>	TextureFormatClassArray;
typedef std::vector<RPDISPLAYMODEINFO>		DisplayModeArray;
//typedef std::vector<D3DRPDEVICETYPE>		DeviceTypeArray;
typedef handlemap<HWND,RPDDCLIPPERDATA>		ClipperMap;

//


// Macro to define supported methods on this interface
#undef DA_METHOD
#define DA_METHOD(name,params) virtual GENRESULT COMAPI name params;

//

static int PixelShaderVersion = 0;

static LPDIRECT3DDEVICE9			direct3d_device = 0;

struct Direct3D_RenderPipeline: IRenderPipeline, 
								IRenderPrimitive,
								IDDBackDoor,
								IGammaControl,
#if RP_RD_DEBUG
								IRenderDebugger,
#endif
								IAggregateComponent

{
public: // Data
	BEGIN_DACOM_MAP_INBOUND(Direct3D_RenderPipeline)
	DACOM_INTERFACE_ENTRY2(IID_IRenderPipeline,IRenderPipeline)
	DACOM_INTERFACE_ENTRY2(IID_IRenderPrimitive,IRenderPrimitive)
	DACOM_INTERFACE_ENTRY2(IID_IDDBackDoor,IDDBackDoor)
	DACOM_INTERFACE_ENTRY2(IID_IGammaControl,IGammaControl)
	DACOM_INTERFACE_ENTRY2(IID_IAggregateComponent,IAggregateComponent)
#if RP_RD_DEBUG
	DACOM_INTERFACE_ENTRY2(IID_IRenderDebugger,IRenderDebugger)
#endif
	END_DACOM_MAP()

public:	// Interface

    // IAggregateComponent interface
	//
	GENRESULT COMAPI Initialize(void);

	// IRenderPipeline interface
	//

	ID3DXEffect** load_effect(const char * filename, IComponentFactory * DIR);
	GENRESULT COMAPI create_cube_texture_from_file(const char* filename, IComponentFactory * DIR,U32 &out_htexture);
	GENRESULT COMAPI set_vs_constants(UINT StartRegister,CONST FLOAT* pConstantData,UINT RegisterCount);
	GENRESULT COMAPI set_ps_constants(UINT StartRegister,CONST FLOAT* pConstantData,UINT RegisterCount);

	GENRESULT COMAPI set_default_constants( const Transform &W, Vector eye, U32 lightsNeeded);
	
	GENRESULT COMAPI set_vertex_declaration(IDirect3DVertexDeclaration9 *pDecl);



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
	GENRESULT COMAPI set_light( U32 light_index, D3DLIGHT9 *light_values );
	GENRESULT COMAPI get_light( U32 light_index, D3DLIGHT9 *out_light_values );
	GENRESULT COMAPI set_light_enable( U32 light_index, U32 enable );
	GENRESULT COMAPI get_light_enable( U32 light_index, U32 *out_enable );
	GENRESULT COMAPI get_num_lights( U32 *out_num_lights ) ;
	GENRESULT COMAPI get_lights( U32 num_start, U32 num_lights, U32 *out_light_indices ) ;
	GENRESULT COMAPI set_material( D3DMATERIAL9 *material_values );
	GENRESULT COMAPI get_material( D3DMATERIAL9 *out_material_values );
	GENRESULT COMAPI begin_scene( void  ) ;
	GENRESULT COMAPI end_scene( void  ) ;
	GENRESULT COMAPI set_render_state( D3DRENDERSTATETYPE state, U32 value  ) ;
	GENRESULT COMAPI get_render_state( D3DRENDERSTATETYPE state, U32 *value  ) ;
	
	GENRESULT COMAPI set_sampler_state( U32 stage, D3DSAMPLERSTATETYPE, U32 value );
	GENRESULT COMAPI get_sampler_state( U32 stage, D3DSAMPLERSTATETYPE, U32 *value );

	GENRESULT COMAPI set_texture_stage_state( U32 stage, D3DTEXTURESTAGESTATETYPE, U32 value  ) ;
	GENRESULT COMAPI get_texture_stage_state( U32 stage, D3DTEXTURESTAGESTATETYPE, U32 *value  ) ;
	GENRESULT COMAPI set_texture_stage_transform( U32 stage, Matrix4 &mat4 ) ;
	GENRESULT COMAPI get_texture_stage_transform( U32 stage, Matrix4 &out_mat4 ) ;
	GENRESULT COMAPI set_texture_stage_texture( U32 stage, U32 htexture  ) ;
	GENRESULT COMAPI get_texture_stage_texture( U32 stage, U32 *htexture  ) ;
	GENRESULT COMAPI verify_state( void ) ;
	GENRESULT COMAPI create_state_block( D3DSTATEBLOCKTYPE type, U32*out_sbhandle ) ;
	GENRESULT COMAPI update_state_block( U32 sbhandle ) ;
	GENRESULT COMAPI begin_state_block( void ) ;
	GENRESULT COMAPI end_state_block( U32*out_sbhandle ) ;
	GENRESULT COMAPI apply_state_block( U32 sbhandle ) ;
	GENRESULT COMAPI delete_state_block( U32 sbhandle ) ;
	GENRESULT COMAPI draw_primitive( D3DPRIMITIVETYPE type, U32 vertex_format, const void *verts, int num_verts, U32 flags  ) ;
	GENRESULT COMAPI draw_indexed_primitive( D3DPRIMITIVETYPE type, U32 vertex_format, const void *verts, int num_verts, const U16 * indices, int num_indices, U32 flags  ) ;
	GENRESULT COMAPI draw_primitive_vb( D3DPRIMITIVETYPE type, IRP_VERTEXBUFFERHANDLE vbhandle, int start_vert, int num_verts, U32 flags  ) ;
	GENRESULT COMAPI draw_indexed_primitive_vb( D3DPRIMITIVETYPE type, IRP_VERTEXBUFFERHANDLE vbhandle, int start_vert, int num_verts, const U16 * indices, int num_indices, U32 flags  ) ;

	GENRESULT COMAPI draw_indexed_primitive_vb( D3DPRIMITIVETYPE type, IRP_VERTEXBUFFERHANDLE vbhandle, int start_vert, int num_verts,  IDirect3DIndexBuffer9* IB, int num_indices, U32 flags  ) ;
	GENRESULT COMAPI create_index_buffer(UINT Length, IDirect3DIndexBuffer9** ppIndexBuffer);
 		
	GENRESULT COMAPI create_texture( int width, int height, const PixelFormat &desiredformat, int num_lod, U32 irp_ctf_flags, U32 &out_htexture  ) ;
	GENRESULT COMAPI destroy_texture( U32 htexture  ) ;
	GENRESULT COMAPI is_texture( U32 htexture  ) ;
	GENRESULT COMAPI lock_texture( U32 htexture, int subsurface, RPLOCKDATA *lockData  ) ;
	GENRESULT COMAPI unlock_texture( U32 htexture, int subsurface  ) ;
	GENRESULT COMAPI get_texture_format( U32 htexture, PixelFormat *out_pf  ) ;
	GENRESULT COMAPI get_texture_dim( U32 htexture, U32 *out_width, U32 *out_height, U32 *out_num_lod  ) ;
	GENRESULT COMAPI get_texture_interface( U32 htexture, const char *iid, void **out_iif  ) ;
	GENRESULT COMAPI get_texture_dc( U32 htexture, HDC *out_hdc ) ;
	GENRESULT COMAPI release_texture_dc( U32 htexture, HDC hdc ) ;
	GENRESULT COMAPI set_texture_palette( U32 htexture, int start, int length, const RGB *colors  ) ;
	GENRESULT COMAPI get_texture_palette( U32 htexture, int start, int length, RGB *colors  ) ;
	GENRESULT COMAPI set_texture_level_data( U32 htexture, int subsurface, int src_width, int src_height, int src_stride, const PixelFormat &src_format, const void *src_pixel, const void *src_alpha, const RGB *src_palette ) ;
	GENRESULT COMAPI blit_texture( U32 hDest, U32 dst_subsurface, RECT destRect, U32 hSrc, U32 src_subsurface, RECT srcRect  ) ;
	GENRESULT COMAPI get_num_textures( U32 *out_num_textures  ) ;
	GENRESULT COMAPI get_texture( U32 texture_num, U32 *out_htexture  ) ;
	GENRESULT COMAPI create_vertex_buffer( U32 vertex_format, int num_verts, U32 irp_vbf_flags, IRP_VERTEXBUFFERHANDLE *out_vb_handle ) ;
	GENRESULT COMAPI destroy_vertex_buffer( IRP_VERTEXBUFFERHANDLE vb_handle ) ;
	GENRESULT COMAPI lock_vertex_buffer( IRP_VERTEXBUFFERHANDLE vb_handle, U32 ddlock_flags, void **out_vertex_buffer, U32 *out_vertex_buffer_size ) ;
	GENRESULT COMAPI unlock_vertex_buffer( IRP_VERTEXBUFFERHANDLE vb_handle ) ;
	GENRESULT COMAPI optimize_vertex_buffer( IRP_VERTEXBUFFERHANDLE vb_handle ) ;
	GENRESULT COMAPI process_vertex_buffer( IRP_VERTEXBUFFERHANDLE dst_vb_handle, IRP_VERTEXBUFFERHANDLE src_vb_handle, int first_vertex_index, int num_verts, U32 irp_pvbf_flags ) ;


#if RP_RD_DEBUG
	// IRenderDebugger
	void COMAPI dump_current_state( const IRD_STATETYPE state_traces, const char *directory ) ;
	void COMAPI set_trace_enable( const IRD_STATETYPE state_traces ) ;
	void COMAPI set_trace_output_dir( const char *directory );
	void COMAPI add_trace_message( const char *fmt, ... ) ;
	void COMAPI begin_trace_section( const char *fmt, ... ) ;
	void COMAPI end_trace_section( void ) ;
	void COMAPI clear_trace_capture( void ) ;	
	void COMAPI save_trace_capture( const char *filename ) ;	
	void COMAPI set_frustum_view_enable( bool onoff ) ;
	void COMAPI set_frustum_view_options( void ) ;
	void COMAPI set_sb_delay( float milliseconds ) ;	
	void COMAPI set_dp_delay( float milliseconds ) ;
	void COMAPI save_texture( const char *filename, U32 irp_texture_handle ) ;
	void COMAPI save_screen_capture( const char *filename, bool delay_to_sb ) ;
	void COMAPI set_ref_enable( bool onoff ) ;
#endif

	// IRenderPrimitive
	GENRESULT COMAPI set_state( RPRSTATE state, U32 value );
	GENRESULT COMAPI get_state( RPRSTATE state, U32 *value );
	GENRESULT COMAPI flush( U32 flags );

	// IGammaControl
	GENRESULT COMAPI set_gamma_function( IGC_COMPONENT which, float display_gamma, float bias, float slope, float black_offset );
	GENRESULT COMAPI set_gamma_ramp( IGC_COMPONENT which, WORD *ramp );
	GENRESULT COMAPI get_gamma_ramp( IGC_COMPONENT which, WORD *out_ramp );
	GENRESULT COMAPI set_calibration_enable( bool enabled );
	GENRESULT COMAPI get_calibration_enable( void );

	// IDDBackDoor
	GENRESULT COMAPI get_dd_provider( DDBACKDOORPROVIDER provider, IUnknown ** pUnknown );

	// Required C++ and TComponent methods
	//
	Direct3D_RenderPipeline(void);
	~Direct3D_RenderPipeline(void);
	DA_HEAP_DEFINE_NEW_OPERATOR(Direct3D_RenderPipeline);
	GENRESULT init(AGGDESC *desc);
	GENRESULT init(RPUL_DACOMDESC *desc);

protected:	// Interface

//

#define D3DRP_F_PIPELINE_CREATED		(1<<0)
#define D3DRP_F_BUFFERS_CREATED			(1<<1)
#define D3DRP_F_STENCIL_CREATED			(1<<2)
#define D3DRP_F_DEPTH_CREATED			(1<<3)
#define D3DRP_F_CHECK_STARTUP			(1<<4)
#define D3DRP_F_CHECK_CREATE_BUFFERS	(1<<5)
#define D3DRP_F_USE_GAMMA_CALIBRATION	(1<<6)
#define D3DRP_F_USE_DIRECTSHOW			(1<<7)

// 

	void	internal_reset_all_caches( void );

	void	internal_set_default_pipeline_state( void );
	void	internal_set_default_render_state( void );
	void	internal_set_default_transform_state( void );
	
	void	internal_set_abilities( void );
	void	internal_calulate_fog( U32 vert_type, void *verts, U32 num_verts );
	HRESULT	internal_find_depth_buffer_format( U32 color_bpp, U32 depth_bpp, U32 stencil_bpp, PixelFormat *pf );

	void					internal_initialize_texture_format_classes( void );
	RPTEXTUREFORMATCLASS   *internal_add_texture_format_class( U32 fourcc );
	RPTEXTUREFORMATCLASS   *internal_find_texture_format_class( U32 fourcc );

protected:	// protected data

	U32							d3drp_f_flags;

	COMPTR<IProfileParser>		profile_parser;
//	DeviceTypeArray				device_types;
	char						ini_device_profile[D3DRP_MAX_STRING_LEN];	

	RPDEVICEINFO				current_device_info;
//	DeviceTypeArray::iterator	current_device_type;
	char						current_device_profile[D3DRP_MAX_STRING_LEN];	
	
	//LPDIRECTDRAW7				directdraw;
	//LPDIRECTDRAWSURFACE7		directdraw_color_buffers[2];	// [0] == front buffer, [1] == back buffer
//	LPDIRECTDRAWSURFACE7		direct3d_depth_buffer;
	IVideoStreamControl			*buffers_video_stream;

	U32							rprd_lock_flags;
	U32							rprd_device_flags;	// RPRD_F_*	// based on the device/persist configuration
	U32							rprd_buffers_flags;	// RPRD_F_*	// based on the current pipeline state

	TextureFormatArray			texture_formats;
	TextureFormatClassArray		texture_format_classes;

	DisplayModeArray			display_modes;
	DisplayModeArray::iterator	current_display_mode;

	ClipperMap					clippers;
	ClipperMap::iterator		current_clipper;

	typedef std::map<U32,U32>	LightInfoMap;

	LightInfoMap				lights;

	U32							device_abilities[RP_A_MAX_ABILITY];
	U32							direct3d_blend_matrix[4];

	U32							batch_state[RPR_MAX_STATE];

	AbilitiesArray			abilities_info;
	RenderStateArray		render_state_info;
	PipelineStateArray		pipeline_state_info;
	TextureStageStateArray	texture_stage_state_info[D3DTSS_NUM_STAGES];

	//

	bool				disable_hw_caches;		// used when recording state blocks

	//
	
	CACHED_PIPELINE_STATE curr_pipeline_state[RP_MAX_PIPELINE_STATE];
	
	CACHED_RENDERSTATE	curr_hw_render_state[D3DRS_MAX_STATE];				// render state
	
	CACHED_TEXTURE		curr_hw_texture[D3DTSS_NUM_STAGES];							// texture stage texture
	CACHED_TEXTURESTATE curr_hw_texture_state[D3DTSS_NUM_STAGES][D3DTSS_MAX_STATE];	// texture stage state
	CACHED_MATRIX		curr_hw_texture_transform[D3DTSS_NUM_STAGES];				// texture stage transforms
	
	CACHED_MATRIX		curr_hw_world;												// modelview
	CACHED_MATRIX		curr_hw_projection;											// projection
	CACHED_VIEWPORT		curr_hw_viewport;											// viewport 

};

//

#undef DA_METHOD
#undef DA_COMPONENT_NAME

#define DA_COMPONENT_NAME Direct3D_RenderPipeline
#define DA_METHOD(name,params) GENRESULT COMAPI DA_COMPONENT_NAME :: name params

//


ID3DXEffect * loadEffectHelper(char* filename,IComponentFactory * DIR)
{
		//ICOManager *DACOM = DACOM_Acquire();
		bool tripAssertion = false;
		LPD3DXEFFECT effect = 0;
		while (effect == NULL)
		{
			if (tripAssertion) assert(false);
			tripAssertion = true;
			DAFILEDESC fdesc = filename;
			COMPTR<IFileSystem> file;
			if (DIR->CreateInstance(&fdesc, file) != GR_OK)
			{
				GENERAL_TRACE_1("Failed to find an effect file! ");
				GENERAL_TRACE_1(filename);
				GENERAL_TRACE_1("\n");
				continue;
			}
			
			U32 fileSize = file->GetFileSize();
			char * data = new char[fileSize+1];
			U32 dwRead = 0;
			file->ReadFile(0, data, fileSize,&dwRead,0);

			data[fileSize] = 0;
			
			LPD3DXBUFFER pBufferErrors = NULL;
			if (FAILED(D3DXCreateEffect( direct3d_device, 
				data, 
				fileSize, 
				NULL, 
				NULL, 
				NULL, 
				NULL, 
				&effect, 
				&pBufferErrors )))
				{
				GENERAL_TRACE_1("Failed to create effect: ");
				GENERAL_TRACE_1("\n");
			}
			if( pBufferErrors != NULL )
			{
				GENERAL_TRACE_1("\n");
				GENERAL_TRACE_1((CHAR*)pBufferErrors->GetBufferPointer());
				GENERAL_TRACE_1("\n");
			}
			if (effect == NULL)	continue;
			D3DXHANDLE hTech = NULL;
			switch (PixelShaderVersion)
			{
			case 0:
				hTech = effect->GetTechniqueByName("PS00");
				break;
			case 1:
				hTech = effect->GetTechniqueByName("PS11");
				break;
			case 2:
				hTech = effect->GetTechniqueByName("PS20");
				break;
			}
			effect->SetTechnique(hTech);
			delete data;
		}
		return effect;
}

struct effectList
{
	char fileName[256];
	IComponentFactory * fileDir;
	ID3DXEffect* effect;
	effectList * next;
	effectList()
	{
		effect = 0;
		next = 0;
	}
	void loadEffect()
	{
		if (effect) (effect)->Release();
		effect =  loadEffectHelper(fileName, fileDir);;
	}
};

effectList * effectsForRestoration = 0;


ID3DXEffect** Direct3D_RenderPipeline::load_effect(const char * filename, IComponentFactory * DIR)
{
	effectList * tmp = effectsForRestoration;
	while (tmp)
	{
		if (strcmp(filename, tmp->fileName) == 0)
		{
#if _DEBUG
			tmp->loadEffect();
#endif
			return &tmp->effect;
		}
		tmp = tmp->next;
	}

	tmp = new effectList();
	strcpy(tmp->fileName, filename);
	tmp->fileDir = DIR;
	tmp->loadEffect();
	tmp->next = effectsForRestoration;
	effectsForRestoration = tmp;
	return &tmp->effect;
}


U32 enabledLightIndices[9];
U32 enabledLightCount = 0;

GENRESULT COMAPI Direct3D_RenderPipeline::set_default_constants( const Transform &worldTrans, Vector eye,U32 lightCount)
{

	//Transform worldInv = worldTrans.get_inverse();
	float w;
	Transform worldInv = worldTrans.get_general_inverse(w);
	D3DXMATRIX matMV;
	curr_hw_world.get(direct3d_device, D3DTS_WORLD, &matMV);

	D3DXMATRIX matMV2;
	D3DXMatrixTranspose(&matMV2, &matMV);
	direct3d_device->SetVertexShaderConstantF(62, (float*)&matMV2, 4);


	D3DXMATRIX P;
	curr_hw_projection.get(direct3d_device, D3DTS_PROJECTION, &P);
	D3DXMATRIX matViewProj;
	D3DXMatrixMultiply(&matViewProj, &matMV, &P);


	D3DXMatrixTranspose(&matViewProj, &matViewProj);
	direct3d_device->SetVertexShaderConstantF(66, (float*)&matViewProj, 4);

	direct3d_device->SetVertexShaderConstantF(79, (float*) &eye, 1);

#define MAXLIGHTSFOUND 4

	int lightsNeeded = min(enabledLightCount,MAXLIGHTSFOUND);
	lightsNeeded = min(lightsNeeded, lightCount-1);

	D3DLIGHT9 lightData[MAXLIGHTSFOUND];
	memset(&lightData,0,sizeof(D3DLIGHT9) * MAXLIGHTSFOUND);
	float Atten[MAXLIGHTSFOUND];
	memset(&Atten,0,MAXLIGHTSFOUND * sizeof(float));

	for (int i = 0; i < enabledLightCount; i++)
	{
		D3DLIGHT9 tmp;
		get_light(enabledLightIndices[i],&tmp);
		if ((tmp.Type == D3DLIGHT_POINT) && (tmp.Diffuse.r > 0 || tmp.Diffuse.g > 0 || tmp.Diffuse.b > 0))
		{
			Vector lightWorldPos(tmp.Position.x, tmp.Position.y, tmp.Position.z);
			float dist = (worldTrans.translation - lightWorldPos).fast_magnitude();
			float newAtten =  1.0 - (dist/tmp.Range);
			float worstAtten = Atten[0];
			int worstIndex = 0;
			int j;
			for (j = 1; j < lightsNeeded; j++)
			{
				if (Atten[j] < worstAtten)
				{
					worstAtten = Atten[j];
					worstIndex = j;
				}
			}
			if (newAtten > Atten[j])
			{
				Atten[worstIndex] = newAtten;
				lightData[worstIndex] = tmp;
			}
		}
		else if ((tmp.Type == D3DLIGHT_DIRECTIONAL) && (tmp.Diffuse.r > 0 || tmp.Diffuse.g > 0 || tmp.Diffuse.b > 0))
		{
			float newAtten =  1.0 * (tmp.Diffuse.r + tmp.Diffuse.g + tmp.Diffuse.b);
			float worstAtten = Atten[0];
			int worstIndex = 0;
			int j;
			for (j = 1; j < lightsNeeded; j++)
			{
				if (Atten[j] < worstAtten)
				{
					worstAtten = Atten[j];
					worstIndex = j;
				}
			}
			if (newAtten > Atten[j])
			{
				Atten[worstIndex] = newAtten;
				lightData[worstIndex] = tmp;
			}
		}
	}

	float bestAtten = 0; // choose the best key-light
	int bestKey = 0;
	for (int i = 0; i < enabledLightCount; i++)
	{
		D3DLIGHT9 tmp;
		get_light(enabledLightIndices[i],&tmp);
		if (Atten[i] > bestAtten)
		{
			bestAtten = Atten[i];
			bestKey = i;
		}
	}

	if (bestKey != 0)
	{
		D3DLIGHT9 tmp = lightData[bestKey];
		lightData[bestKey] = lightData[0];
		lightData[0] = tmp;
		float tmpAtten = Atten[bestKey];
		Atten[bestKey] = Atten[0];
		Atten[0] = tmpAtten;
	}


	for(int i = 0; i < MAXLIGHTSFOUND; i++)
	{
		Vector lightObjPos;
		float diffuseColor [] = {0,0,0,1};
		if (lightData[i].Type == D3DLIGHT_DIRECTIONAL)
		{
			diffuseColor[0] = lightData[i].Diffuse.r;
			diffuseColor[1] = lightData[i].Diffuse.g;
			diffuseColor[2] = lightData[i].Diffuse.b;
			Vector lightWorldPos(lightData[i].Direction.x, lightData[i].Direction.y, lightData[i].Direction.z);
			lightWorldPos = worldTrans.translation - lightWorldPos * 2000000;
			lightObjPos = worldInv.rotate_translate(*(Vector*)&lightWorldPos);
		}
		else
		{
			if (Atten[i] < 0) Atten[i] = 0;
			diffuseColor[0] = lightData[i].Diffuse.r * Atten[i]*2;
			diffuseColor[1] = lightData[i].Diffuse.g * Atten[i]*2;
			diffuseColor[2] = lightData[i].Diffuse.b * Atten[i]*2;
			Vector lightWorldPos(lightData[i].Position.x, lightData[i].Position.y, lightData[i].Position.z);

			lightObjPos = worldInv.rotate_translate(*(Vector*)&lightWorldPos);
		}
		direct3d_device->SetPixelShaderConstantF(i, (float*) diffuseColor, 1);
		direct3d_device->SetVertexShaderConstantF(9+i, (float*) &lightObjPos, 1);
	}
	//Vector lightPos(1000000,-1000000,240000);
	//Vector lightPos(0,240000,0);
	//Vector lightObjPos = worldInv.rotate_translate(lightPos);
	//direct3d_device->SetVertexShaderConstantF(7, (float*) &lightObjPos, 1);
	U8 ambientBytes[4];
	get_render_state(D3DRS_AMBIENT,(U32*)ambientBytes);

	float ambientColor [] = {  ambientBytes[2]/255.0, 
							   ambientBytes[1]/255.0,
							   ambientBytes[0]/255.0,
							   0};
			
	direct3d_device->SetPixelShaderConstantF(6, (float*) ambientColor, 1);

	return GR_OK;
}

GENRESULT COMAPI Direct3D_RenderPipeline::set_vs_constants(UINT StartRegister,CONST FLOAT* pConstantData,UINT RegisterCount)
{
	
	direct3d_device->SetVertexShaderConstantF(StartRegister, pConstantData, RegisterCount);
	return GR_OK;
}

GENRESULT COMAPI Direct3D_RenderPipeline::set_ps_constants(UINT StartRegister,CONST FLOAT* pConstantData,UINT RegisterCount)
{
	direct3d_device->SetPixelShaderConstantF(StartRegister, pConstantData, RegisterCount);
	return GR_OK;
}

GENRESULT COMAPI Direct3D_RenderPipeline::set_vertex_declaration(IDirect3DVertexDeclaration9 *pDecl)
{
	return GR_OK;
}

// ------------------------------------------------------------------
// Startup/shutdown Related Code
// ------------------------------------------------------------------

Direct3D_RenderPipeline::Direct3D_RenderPipeline(void)
{
	ini_device_profile[0] = 0;
	current_device_profile[0] = 0;

	memset( &current_device_info, 0, sizeof(current_device_info) );

//	directdraw = NULL;
//	directdraw_color_buffers[0] = NULL;
//	directdraw_color_buffers[1] = NULL;
	direct3d_device = NULL;
//	direct3d_depth_buffer = NULL;

	rprd_device_flags = 0;
	rprd_buffers_flags = 0;
	rprd_lock_flags = 0;

	d3drp_f_flags = 0;

	current_clipper = clippers.end();
	current_display_mode = display_modes.end();

	// these are in order of "preference" in case one fails
	//
	
//	device_types.push_back( D3DRPDEVICETYPE( "T&L", "Hardware",		&IID_IDirect3DTnLHalDevice,	0,					DirectXDllName	) );
//	device_types.push_back( D3DRPDEVICETYPE( "HAL", "Old-Hardware",	&IID_IDirect3DHALDevice,	0,					DirectXDllName	) );
//	device_types.push_back( D3DRPDEVICETYPE( "SFT", "Software",		&IID_IDirect3DBladeDevice,	D3DRP_DTF_SOFTWARE, BladeDllName	) );
//	device_types.push_back( D3DRPDEVICETYPE( "MMX", "MMX-Software",	&IID_IDirect3DMMXDevice,	D3DRP_DTF_SOFTWARE, DirectXDllName	) );
//	device_types.push_back( D3DRPDEVICETYPE( "RGB", "RGB-Software",	&IID_IDirect3DRGBDevice,	D3DRP_DTF_SOFTWARE, DirectXDllName	) );
//	device_types.push_back( D3DRPDEVICETYPE( "REF", "Reference",	&IID_IDirect3DRefDevice,	D3DRP_DTF_SOFTWARE, DirectXDllName	) );
//	device_types.push_back( D3DRPDEVICETYPE( "NUL", "Null",			&IID_IDirect3DNullDevice,	D3DRP_DTF_SOFTWARE, DirectXDllName	) );

	
	//current_device_type = device_types.end(); 

	internal_reset_all_caches();

	
	d3drp_f_flags |= D3DRP_F_CHECK_STARTUP|D3DRP_F_CHECK_CREATE_BUFFERS;
}

//

Direct3D_RenderPipeline::~Direct3D_RenderPipeline(void)
{
	shutdown();

	rp_rd_cleanup();

//	device_types.clear();
}

//

// This is called during normal use.  We are an aggregate.
//
GENRESULT Direct3D_RenderPipeline::init( AGGDESC *desc )
{ 
	if( desc->description!=NULL && strlen( desc->description ) > 0 ) {
		strcpy( ini_device_profile, desc->description );
	}
	else {
		strcpy( ini_device_profile, DeviceProfile_Default );
	}

	GENERAL_NOTICE( _MS(( "Direct3D_RenderPipeline: init(AGG): using profile '%s'\n", ini_device_profile )) );

	return GR_OK;
}

//

// This is called during enumeration.  We are !not! an aggregate.
//
GENRESULT Direct3D_RenderPipeline::init( RPUL_DACOMDESC *desc)
{ 
	profile_parser = desc->profile_parser;

	if( desc->device_id != NULL && strlen( desc->device_id ) > 0 ) {
		strcpy( ini_device_profile, desc->device_id );
	}
	else {
		strcpy( ini_device_profile, DeviceProfile_Default );
	}

	GENERAL_NOTICE( _MS(( "Direct3D_RenderPipeline: init(RP): using profile '%s'\n", ini_device_profile )) );


	return GR_OK;
}

//

GENRESULT COMAPI Direct3D_RenderPipeline::Initialize(void)
{ 
	if( profile_parser == NULL ) {
		DACOM_Acquire()->QueryInterface( IID_IProfileParser, profile_parser );
	}

	MATH_ENGINE();

	// build internal info lookup tables
	rpsi_build_render_state_info( render_state_info );
	rpsi_build_pipeline_state_info( pipeline_state_info );
	rpsi_build_abilities_info( abilities_info );
	rpsi_build_texture_stage_state_info( texture_stage_state_info[0], true );
	for (int i = 1; i < D3DTSS_NUM_STAGES; i++)
	{
		rpsi_build_texture_stage_state_info( texture_stage_state_info[i], false );
	}


	rp_rd_init( &render_state_info, &texture_stage_state_info[0] );


	return GR_OK; 
}

//

inline void Direct3D_RenderPipeline::internal_set_default_pipeline_state( void )
{
	set_state( RPR_BATCH,						FALSE );				
	set_state( RPR_BATCH_POOLS,					0 );				
	set_state( RPR_BATCH_OPAQUE_FLUSH,			0 );
	set_state( RPR_BATCH_OPAQUE_POOL,			0 );
	set_state( RPR_BATCH_TRANSLUCENT_POOL,		0 );
	set_state( RPR_BATCH_TRANSLUCENT_NONZ_POOL,	0 );
	set_state( RPR_BATCH_TRANSLUCENT_MODE,		0 );

	U32 ps_enum, default_value;

	PipelineStateArray::iterator beg = pipeline_state_info.begin();
	PipelineStateArray::iterator end = pipeline_state_info.end();
	PipelineStateArray::iterator ps;

	for( ps=beg; ps!=end; ps++ ) {

		if( ps->second.get_enum_and_default( &ps_enum, &default_value ) ) {

			ASSERT( ps_enum < RP_MAX_PIPELINE_STATE );

			curr_pipeline_state[ps_enum].invalidate();

			set_pipeline_state( (RPPIPELINESTATE)ps_enum, default_value );
		}
	}

	return;
}

//

inline void Direct3D_RenderPipeline::internal_set_default_render_state( void )
{
	ASSERT( direct3d_device );

	D3DMATRIX I;

	memset( &I, 0, sizeof(I) );

	I.m[0][0] = I.m[1][1] = I.m[2][2] = I.m[3][3] = 1.0f;


	U32 _enum, _value;

	RenderStateArray::iterator tss_beg, tss_end, tss;

	for( U32 stage=0; stage<D3DTSS_NUM_STAGES; stage++ ) {

		curr_hw_texture[stage].invalidate();
		curr_hw_texture[stage].set( direct3d_device, stage, 0 );

		curr_hw_texture_transform[stage].invalidate();
		curr_hw_texture_transform[stage].set( direct3d_device, D3DTS_TEXTURE0 + stage, &I );

		tss_beg = texture_stage_state_info[stage].begin();
		tss_end = texture_stage_state_info[stage].end();

		for( tss=tss_beg; tss!=tss_end; tss++ ) {

			if( tss->second.get_enum_and_default( &_enum, &_value ) ) {
			
				ASSERT( _enum < D3DTSS_MAX_STATE );

				curr_hw_texture_state[stage][_enum].invalidate();
				curr_hw_texture_state[stage][_enum].set( direct3d_device, stage, _enum, _value );
			}
		}
	}

	RenderStateArray::iterator beg = render_state_info.begin();
	RenderStateArray::iterator end = render_state_info.end();
	RenderStateArray::iterator rs;

	for( rs=beg; rs!=end; rs++ ) {

		if( rs->second.get_enum_and_default( &_enum, &_value ) ) {
		
			ASSERT( _enum < D3DRS_MAX_STATE );

			curr_hw_render_state[_enum].invalidate();
			curr_hw_render_state[_enum].set( direct3d_device, _enum, _value );
		}
	}

	return;
}

//

inline void Direct3D_RenderPipeline::internal_set_default_transform_state( void )
{
	ASSERT( direct3d_device );

	D3DMATRIX M,V,P;
	
	memset( &V, 0, sizeof(V) );
	memset( &M, 0, sizeof(M) );
	memset( &P, 0, sizeof(P) );

	M.m[0][0] = V.m[0][0] = P.m[0][0] = 1.0;
	M.m[1][1] = V.m[1][1] = P.m[1][1] = 1.0;
	M.m[2][2] = V.m[2][2] = P.m[2][2] = 1.0;
	M.m[3][3] = V.m[3][3] = P.m[3][3] = 1.0;

	M.m[2][2] = -M.m[2][2];

	direct3d_device->SetTransform( D3DTS_VIEW, &V );

	curr_hw_world.invalidate();
	curr_hw_world.set( direct3d_device, D3DTS_WORLD, &M );
	
	curr_hw_projection.invalidate();
	curr_hw_projection.set( direct3d_device, D3DTS_PROJECTION, &P );

	curr_hw_viewport.invalidate();
	curr_hw_viewport.set_viewport( direct3d_device, 0, 0, current_clipper->second.win_w, current_clipper->second.win_h );
	curr_hw_viewport.set_depth_range( direct3d_device, 0.0f, 1.0f );

	return;
}

//

void Direct3D_RenderPipeline::internal_reset_all_caches( void )
{
	curr_hw_world.invalidate();
	curr_hw_projection.invalidate();
	curr_hw_viewport.invalidate();


	for( int ps=0; ps<RP_MAX_PIPELINE_STATE; ps++ ) {
		curr_pipeline_state[ps].invalidate();
	}

	for( int rs=0; rs<D3DRS_MAX_STATE; rs++ ) {
		curr_hw_render_state[rs].invalidate();
	}

	for( int s=0; s<D3DTSS_NUM_STAGES; s++ ) {
		
		curr_hw_texture[s].invalidate();
		curr_hw_texture_transform[s].invalidate();

		for( int st=0; st<D3DTSS_MAX_STATE; st++ ) {

			curr_hw_texture_state[s][st].invalidate();

		}
	}

}

//

typedef struct {
	U32 equal;
	U32 depth_bpp;
	U32 stencil_bpp;
	PixelFormat *out_format;
} D3DRPZENUMDATA;

//

static HRESULT CALLBACK D3DRP_FindZFormat(LPDDPIXELFORMAT pf,LPVOID lpContext )
{
	/*
	D3DRPZENUMDATA *d = (D3DRPZENUMDATA*)lpContext;

	U32 num_s_bits = pf->dwStencilBitDepth;
	U32 num_z_bits = pf->dwZBufferBitDepth - num_s_bits;

	if( d->equal ) {
		if( num_z_bits == d->depth_bpp && num_s_bits == d->stencil_bpp ) {
			memcpy( &d->out_format->ddpf, pf, sizeof(DDPIXELFORMAT) );
			return 0;
		}
	}	
	else {
		if( num_z_bits >= d->depth_bpp && num_s_bits >= d->stencil_bpp ) {
			memcpy( &d->out_format->ddpf, pf, sizeof(DDPIXELFORMAT) );
			return 0;
		}
	}
*/
	return D3D_OK;
}

//

HRESULT	Direct3D_RenderPipeline::internal_find_depth_buffer_format( U32 color_bpp, U32 depth_bpp, U32 stencil_bpp, PixelFormat *pf )
{
	/*
	memset( &pf->ddpf, 0, sizeof(pf->ddpf) );
	pf->ddpf.dwSize = sizeof(pf->ddpf);

	if( directdraw != NULL ) {
		if( SUCCEEDED( rp_dd_is_nvidia( directdraw ) ) ) {
			
			U32 sbpp = (stencil_bpp? 8 : 0); // NV only supports 8bit stencil anyway.
			U32 dbpp = depth_bpp;	
			
			if( color_bpp != (dbpp+sbpp) ) {

				GENERAL_TRACE_1( "NVIDIA Hardware: forcing depth_bpp+stencil_bpp == color_bpp\n" );

				if( color_bpp == 16 ) {
				
					depth_bpp = 16;

					if( sbpp ) {
						GENERAL_TRACE_1( "NVIDIA Hardware: no stencil in 16-bit, disabling\n" );
						stencil_bpp = 0;
					}
				}
				else if( color_bpp == 24 || color_bpp == 32 ) {
					depth_bpp = 24;
				}
			}
		}
		COMPTR<IDirect3D9> ID3D;
		D3DRPZENUMDATA	zed;

		if( SUCCEEDED( directdraw->QueryInterface( IID_IDirect3D9, ID3D ) ) ) {
				
			zed.equal = 1;
			zed.out_format = pf;
			zed.depth_bpp = depth_bpp;
 			zed.stencil_bpp = stencil_bpp;
			
			ID3D->EnumZBufferFormats( current_device_type->type_guid, D3DRP_FindZFormat, &zed );
			if( pf->ddpf.dwZBufferBitDepth != 0 ) {
				return S_OK;
			}

			zed.equal = 0;
			ID3D->EnumZBufferFormats( current_device_type->type_guid, D3DRP_FindZFormat, &zed );
			if( pf->ddpf.dwZBufferBitDepth != 0 ) {
				return S_OK;
			}
		}
	}

	pf->ddpf.dwFlags = DDPF_ZBUFFER;
	pf->ddpf.dwZBufferBitDepth = 16;
	pf->ddpf.dwZBitMask = 0xFFFF;
*/
	return S_OK;
}

//

RPTEXTUREFORMATCLASS *Direct3D_RenderPipeline::internal_find_texture_format_class( U32 fourcc )
{
	TextureFormatClassArray::iterator beg = texture_format_classes.begin();
	TextureFormatClassArray::iterator end = texture_format_classes.end();
	TextureFormatClassArray::iterator tfc;

	for( tfc = beg; tfc != end; tfc++ ) 
	{
		if( fourcc == tfc->fourcc ) 
		{
			RPTEXTUREFORMATCLASS * clazz = &(*tfc);
			return clazz;
		}
	}

	return NULL;
}

//

RPTEXTUREFORMATCLASS *Direct3D_RenderPipeline::internal_add_texture_format_class( U32 fourcc )
{
	RPTEXTUREFORMATCLASS *tfc;

	ASSERT( fourcc );

	if( (tfc = internal_find_texture_format_class( fourcc )) != NULL ) {
		return tfc;
	}

	texture_format_classes.push_back( RPTEXTUREFORMATCLASS( fourcc ) );

	return &texture_format_classes[ texture_format_classes.size()-1 ];
}

//

void Direct3D_RenderPipeline::internal_initialize_texture_format_classes( void )
{
	RPTEXTUREFORMATCLASS *tfc = NULL;

	texture_format_classes.clear();

	// DAOP = 8:3:8:8:8, 16:3:5:6:5, 16:3:5:5:5, 24:3:8:8:8, 32:4:8:8:8:8
	//
	tfc = internal_add_texture_format_class( MAKEFOURCC( 'D', 'A', 'O', 'P' ) );
	tfc->add_format( PixelFormat(  8, 8,8,8,0 ) );
	tfc->add_format( PixelFormat( 16, 5,6,5,0 ) );
	tfc->add_format( PixelFormat( 16, 5,5,5,0 ) );
	tfc->add_format( PixelFormat( 16, 5,5,5,1 ) );
	tfc->add_format( PixelFormat( 24, 8,8,8,0 ) );
	tfc->add_format( PixelFormat( 32, 8,8,8,8 ) );

	// DAOT = 16:3:5:6:5, 16:3:5:5:5, 24:3:8:8:8, 32:4:8:8:8:8
	//
	tfc = internal_add_texture_format_class( MAKEFOURCC( 'D', 'A', 'O', 'T' ) );
	tfc->add_format( PixelFormat( 16, 5,6,5,0 ) );
	tfc->add_format( PixelFormat( 16, 5,5,5,0 ) );
	tfc->add_format( PixelFormat( 16, 5,5,5,1 ) );
	tfc->add_format( PixelFormat( 24, 8,8,8,0 ) );
	tfc->add_format( PixelFormat( 32, 8,8,8,8 ) );
	
	// DAA1 = 16:4:5:5:5:1, 16:4:4:4:4:4, 32:4:8:8:8:8
	//
	tfc = internal_add_texture_format_class( MAKEFOURCC( 'D', 'A', 'A', '1' ) );
	tfc->add_format( PixelFormat( 16, 5,5,5,1 ) );
	tfc->add_format( PixelFormat( 16, 4,4,4,4 ) );
	tfc->add_format( PixelFormat( 32, 8,8,8,8 ) );
	
	// DAA4 = 16:4:4:4:4:4, 32:4:8:8:8:8, 16:4:5:5:5:1
	//
	tfc = internal_add_texture_format_class( MAKEFOURCC( 'D', 'A', 'A', '4' ) );
	tfc->add_format( PixelFormat( 16, 4,4,4,4 ) );
	tfc->add_format( PixelFormat( 32, 8,8,8,8 ) );
	tfc->add_format( PixelFormat( 16, 5,5,5,1 ) );
	
	// DAA8 = 32:4:8:8:8:8, 16:4:4:4:4:4, 16:4:5:5:5:1
	//
	tfc = internal_add_texture_format_class( MAKEFOURCC( 'D', 'A', 'A', '8' ) );
	tfc->add_format( PixelFormat( 32, 8,8,8,8 ) );
	tfc->add_format( PixelFormat( 16, 4,4,4,4 ) );
	tfc->add_format( PixelFormat( 16, 5,5,5,1 ) );

#if 0
	// DAAA = 8:1:8, 16:3:5:6:5, 16:3:5:5:5, 24:3:8:8:8, 32:4:8:8:8:8
	//
	tfc = internal_add_texture_format_class( MAKEFOURCC( 'D', 'A', 'A', 'A' ) );
	tfc->add_format( PixelFormat(  8, 8,0,0,0 ) );
	tfc->add_format( PixelFormat( 16, 5,6,5,0 ) );
	tfc->add_format( PixelFormat( 16, 5,5,5,0 ) );
	tfc->add_format( PixelFormat( 24, 8,8,8,0 ) );
	tfc->add_format( PixelFormat( 32, 8,8,8,8 ) );
	
	// DAAL = 16:2:8:8, 32:4:8:8:8:8, 16:4:4:4:4:4, 16:4:5:5:5:1
	//
	tfc = internal_add_texture_format_class( MAKEFOURCC( 'D', 'A', 'A', 'L' ) );
	tfc->add_format( PixelFormat( 16, 8,8,0,0 ) );
	tfc->add_format( PixelFormat( 32, 8,8,8,8 ) );
	tfc->add_format( PixelFormat( 16, 4,4,4,4 ) );
	tfc->add_format( PixelFormat( 16, 5,5,5,1 ) );
#endif
	
	// Read optional overrides from the ini file
	//

	ICOManager *DACOM = DACOM_Acquire();
	COMPTR<IProfileParser> IPP;
	
	char *p, szBuffer[1024+1], *fourcc, *pf;
	int line = 0;
	HANDLE hTFC;

	if( SUCCEEDED( DACOM->QueryInterface( IID_IProfileParser, (void**) &IPP ) ) ) {
		if( (hTFC = IPP->CreateSection( "TextureFormatClasses" )) != 0 ) {
			
			while( IPP->ReadProfileLine( hTFC, line, szBuffer, 1024 ) ) {
				
				line++;

				if( (p = strchr( szBuffer, ';' )) != NULL ) {	// remove in-line comments
					*p = 0;
				}

				p = szBuffer;

				while( *p && strchr( " \t", *p ) ) p++;		// remove leading whitespace of lvalue

				fourcc = p;

				if( strnicmp( fourcc, "clear", 3 ) == 0 ) {
					texture_format_classes.clear();
					continue; // continue with next profile line
				}

				if( (tfc = internal_add_texture_format_class( MAKEFOURCC( fourcc[0], fourcc[1], fourcc[2], fourcc[3] ) )) != NULL ) {

					if( tfc->num_formats ) {
						tfc->clear_formats();
					}

					while( *p && (*p != '=') ) p++;				// 
					p++;										// skip '=', get rvalue

					while( *p ) {
						while( *p && strchr( " \t", *p ) ) p++;	// remove leading whitespace of pf
						pf = p;
						while( *p && (*p != ',') ) p++;			// get end of current pf
						if( *p ) {
							*p = 0;		// terminate current pf
							p++;		// advance to next pf
						}

						if( pf != p ) {
							U32 bpp=0, r=0,g=0,b=0,a=0;
							sscanf( pf, "%d:%d:%d:%d:%d", &bpp, &r, &g, &b, &a );
							tfc->add_format( PixelFormat( bpp, r, g, b, a ) );
						}
					}
				}
			}
			
			IPP->CloseSection( hTFC );
		}
	}

}


//

DA_METHOD(	startup,(const char *profile_name))
{
	ICOManager *DACOM;
	HRESULT hr;
	GUID guid_buffer, *device_guid_ptr;
	char *s, *c;
	DACOM = DACOM_Acquire();
	d3drp_f_flags &= ~(D3DRP_F_CHECK_STARTUP);
	shutdown();
//	internal_set_abilities();	// 
// moved this call to after device creation   -Ryan


	// If the device can do windowed rendering, set the compile-time-default 
	// color bit depth to be the bpp of the desktop in the current video mode.
	// Otherwise, use 16.
	//
	//if( device_abilities[RP_A_DEVICE_WINDOWED] ) 
	{
	//	pipeline_state_info[RP_BUFFERS_COLOR_BPP].ct_default_value = display_mode.render_pf.num_bits();
	}

	// Initialize other options from profile section of the ini
	//

	/*RenderStateArray::iterator rs_beg = render_state_info.begin();
	RenderStateArray::iterator rs_end = render_state_info.end();
	RenderStateArray::iterator rs;

	for( rs=rs_beg; rs!=rs_end; rs++ ) {
		if( rs->second.is_valid() ) {
			opt_get_u32( DACOM, profile_parser, profile_name, rs->second.key_name, rs->second.ct_default_value, &rs->second.rt_default_value );
		}
	}

	PipelineStateArray::iterator beg = pipeline_state_info.begin();
	PipelineStateArray::iterator end = pipeline_state_info.end();
	PipelineStateArray::iterator ps;

	for( ps=beg; ps!=end; ps++ ) {
		if( ps->second.is_valid() ) {
			opt_get_u32( DACOM, profile_parser, profile_name, ps->second.key_name, ps->second.ct_default_value, &ps->second.rt_default_value );
		}
	}*/

	// Set up default state
	//
	internal_set_default_pipeline_state();

	// Signal all clear
	//
	d3drp_f_flags |= D3DRP_F_CHECK_STARTUP;
	d3drp_f_flags |= D3DRP_F_PIPELINE_CREATED;

	return GR_OK;
}

//

DA_METHOD(	shutdown,(void ))
{
	U32 local_flags;

	local_flags = d3drp_f_flags & D3DRP_F_CHECK_STARTUP;
	d3drp_f_flags &= ~(D3DRP_F_CHECK_STARTUP);

	//destroy_buffers();

	display_modes.clear();
	texture_formats.clear();

	//RELEASE( directdraw );

	current_device_profile[0] = 0;

	//rp_dd_cleanup_directdraw();

	d3drp_f_flags &= ~(D3DRP_F_PIPELINE_CREATED);
	d3drp_f_flags |= local_flags;
	
	return GR_OK;
}

//

DA_METHOD( query_device_ability,(RPDEVICEABILITY ability, U32 *out_answer ))
{
	CHECK_STARTUP(query_device_ability);
	
	if( ((((U32)ability) > ((U32)RP_A_MAX_ABILITY))) || out_answer == NULL ) {
		return GR_INVALID_PARMS;
	}

	if( ability == RP_A_TEXTURE_MAX_HEIGHT
		|| ability == RP_A_TEXTURE_MAX_WIDTH) 
	{
		*out_answer = 1024;
	}
	else if( ability != RP_A_BLEND_MATRIX ) {
		*out_answer = device_abilities[ability];
	}
	else {
		out_answer[0] = direct3d_blend_matrix[0];
		out_answer[1] = direct3d_blend_matrix[1];
		out_answer[2] = direct3d_blend_matrix[2];
		out_answer[3] = direct3d_blend_matrix[3];
	}
	
	return GR_OK;
}

//

DA_METHOD( get_dd_provider,	(DDBACKDOORPROVIDER provider,  IUnknown ** pUnknown ))
{
	return GR_OK;
	CHECK_STARTUP(get_dd_provider);
/*
	switch( provider ) {
	//case DDBD_P_DIRECTDRAW:
	//	if( directdraw ) {
	//		if( SUCCEEDED( directdraw->QueryInterface( IID_IUnknown, (void**) pUnknown ) ) ) {
	//			return GR_OK;
	//		}
	//	}
		break;

	case DDBD_P_PRIMARYSURFACE:
//		if( directdraw_color_buffers[0] ) {
//			if( SUCCEEDED( directdraw_color_buffers[0]->QueryInterface( IID_IUnknown, (void**) pUnknown ) ) ) {
//				return GR_OK;
//			}
		}
		break;
	
	case DDBD_P_BACKSURFACE:
		//if( directdraw_color_buffers[1] ) {
		//	if( SUCCEEDED( directdraw_color_buffers[1]->QueryInterface( IID_IUnknown, (void**) pUnknown ) ) ) {
		//		return GR_OK;
		//	}
		}
		break;

	default:
		GENERAL_TRACE_1( _MS(( "Direct3D_RenderPipeline: get_dd_provider: unknown provider %d\n", provider )) );
	}
*/
	return GR_GENERIC;
}

//

GENRESULT COMAPI Direct3D_RenderPipeline::set_calibration_enable( bool enabled ) 
{
	if( !device_abilities[RP_A_DEVICE_GAMMA] ) {
		return GR_GENERIC;
	}
	if( !(rprd_device_flags & RPRD_F_GAMMA_CALIBRATION) ) {
		return GR_GENERIC;
	}

	if( enabled ) {
		d3drp_f_flags |= D3DRP_F_USE_GAMMA_CALIBRATION;
	}
	else {
		d3drp_f_flags &= ~(D3DRP_F_USE_GAMMA_CALIBRATION);
	}

	return GR_OK;
}

//

GENRESULT COMAPI Direct3D_RenderPipeline::get_calibration_enable( void ) 
{
	if( !device_abilities[RP_A_DEVICE_GAMMA] ) {
		return GR_GENERIC;
	}
	return (d3drp_f_flags & D3DRP_F_USE_GAMMA_CALIBRATION)? GR_OK : GR_GENERIC;
}

//

DA_METHOD( set_gamma_function,( IGC_COMPONENT which, float display_gamma, float bias, float slope, float black_offset ))
{
	/*
	CHECK_STARTUP(set_gamma_function);
	if( !device_abilities[RP_A_DEVICE_GAMMA] ) {
		return GR_GENERIC;
	}

	COMPTR<IDirectDrawGammaControl> IDDGC;
	if( FAILED( directdraw_color_buffers[0]->QueryInterface( IID_IDirectDrawGammaControl, (void**) &IDDGC ) ) ) {
		return GR_INTERFACE_UNSUPPORTED;
	}

	DDGAMMARAMP ramp;
	float gamma_correction = 1.0/display_gamma;
	float ramp_map = 256.0 * 256.0 - 1.0;

	for( U32 entry=0; entry<256; entry++ ) {
		float in = ((float)entry) / 255.00;
		float out= bias + pow( slope*in + black_offset, gamma_correction );
		if( out < 0.0F ) {
			out = 0.0F;
		}
		if( out > 1.0F ) {
			out = 1.0F;
		}
		if( which & IGC_RED ) {
			ramp.red[entry]		= ((WORD) ( out * ramp_map ) );
		}
		if( which & IGC_GREEN ) {
			ramp.green[entry]	= ((WORD) ( out * ramp_map ) );
		}
		if( which & IGC_BLUE ) {
			ramp.blue[entry]	= ((WORD) ( out * ramp_map ) );
		}

	}

	U32 flags = (d3drp_f_flags & D3DRP_F_USE_GAMMA_CALIBRATION)? DDSGR_CALIBRATE : 0;

	if( FAILED( IDDGC->SetGammaRamp( flags, &ramp ) ) ) {
		return GR_GENERIC;
	}
*/
	return GR_OK;
}

//

DA_METHOD( set_gamma_ramp,( IGC_COMPONENT which, WORD *_ramp ))
{
	/*
	CHECK_STARTUP(set_gamma_ramp);

	if( !device_abilities[RP_A_DEVICE_GAMMA] ) {
		return GR_GENERIC;
	}
	COMPTR<IDirectDrawGammaControl> IDDGC;
	if( FAILED( directdraw_color_buffers[0]->QueryInterface( IID_IDirectDrawGammaControl, (void**) &IDDGC ) ) ) {
		return GR_INTERFACE_UNSUPPORTED;
	}

	DDGAMMARAMP ramp;

	if( which != IGC_ALL ) {
		if( FAILED( IDDGC->GetGammaRamp( 0, &ramp ) ) ) {
			return GR_GENERIC;
		}
	}

	if( which & IGC_RED ) {
		memcpy( ramp.red, _ramp, sizeof(ramp.red) );	
	}

	if( which & IGC_GREEN ) {
		memcpy( ramp.green, _ramp, sizeof(ramp.green) );	
	}

	if( which & IGC_BLUE ) {
		memcpy( ramp.blue, _ramp, sizeof(ramp.blue) );	
	}

	U32 flags = (d3drp_f_flags & D3DRP_F_USE_GAMMA_CALIBRATION)? DDSGR_CALIBRATE : 0;

	if( FAILED( IDDGC->SetGammaRamp( flags, &ramp ) ) ) {
		return GR_GENERIC;
	}
*/
	return GR_OK;
}

//

DA_METHOD( get_gamma_ramp,( IGC_COMPONENT which, WORD *out_ramp ))
{
	/*
	CHECK_STARTUP(get_gamma_ramp);

	if( !device_abilities[RP_A_DEVICE_GAMMA] ) {
		return GR_GENERIC;
	}
	COMPTR<IDirectDrawGammaControl> IDDGC;
	if( FAILED( directdraw_color_buffers[0]->QueryInterface( IID_IDirectDrawGammaControl, (void**) &IDDGC ) ) ) {
		return GR_INTERFACE_UNSUPPORTED;
	}

	DDGAMMARAMP ramp;
	if( FAILED( IDDGC->GetGammaRamp( 0, &ramp ) ) ) {
		return GR_GENERIC;
	}

	switch( which ) {
	case IGC_RED:	memcpy( out_ramp, ramp.red, sizeof(ramp.red) );		break;
	case IGC_GREEN:	memcpy( out_ramp, ramp.green, sizeof(ramp.green) );	break;
	case IGC_BLUE:	memcpy( out_ramp, ramp.blue, sizeof(ramp.blue) );	break;
	default:		
		GENERAL_WARNING( "get_gamma_ramp: bad gamma ramp requested\n" );
		return GR_GENERIC;
	}
*/
	return GR_OK;
}

//

DA_METHOD(	get_device_info,(RPDEVICEINFO *i ))
{
	CHECK_STARTUP(get_device_info);
	ASSERT( i );

	memcpy( i, &current_device_info, sizeof(RPDEVICEINFO) );

	return GR_OK;
}

//

DA_METHOD(	get_device_stats,(RPDEVICESTATS *stat ))
{
	/*
	CHECK_CREATE_BUFFERS(get_device_stats);
	ASSERT( stat );

	D3DDEVINFO_TEXTUREMANAGER info;

	if( SUCCEEDED( direct3d_device->GetInfo( D3DDEVINFOID_TEXTUREMANAGER, &info, sizeof(info) ) ) ) {

		stat->is_thrashing = info.bThrashing;
		stat->num_texture_managed = info.dwTotalManaged;			
		stat->num_texture_activated = info.dwNumTexturesUsed;		
		stat->num_texture_vidmem_activated = info.dwNumUsedTexInVid;	
		stat->num_texture_vidmem_created = info.dwNumVidCreates;	
		stat->num_texture_vidmem_evicted = info.dwNumEvicts;	
		stat->num_texture_vidmem = info.dwWorkingSet;			
		stat->sizeof_texture_vidmem = info.dwWorkingSetBytes;		
		stat->sizeof_texture_sysmem = info.dwTotalBytes;		

#if RP_RD_DEBUG
		U32 a,b,c,d;

		rp_rd_get_prim_counts( &stat->num_dp_calls, 
							   &stat->num_dp_primitives, 
							   &stat->num_dip_calls, 
							   &stat->num_dip_primitives,
							   &a,
							   &b,
							   &c,
							   &d );

		stat->num_dp_calls += a;
		stat->num_dp_primitives += b;
		stat->num_dip_calls += c;
		stat->num_dip_primitives += d;
#else
		stat->num_dp_calls = 0;
		stat->num_dp_primitives = 0;
		stat->num_dip_calls = 0;
		stat->num_dip_primitives = 0;
#endif

		return GR_OK;
	}
*/
	return GR_GENERIC;
}

//

DA_METHOD(	get_num_display_modes,(U32 *num_modes))
{
	CHECK_STARTUP(get_num_display_modes);

	*num_modes = display_modes.size();
	return GR_OK;
}

//

DA_METHOD(	get_display_mode,(RPDISPLAYMODEINFO *mode, U32 mode_num))
{
	/*
	CHECK_STARTUP(get_display_mode);

	if( mode_num == RP_CURRENT ) {
		mode_num = current_display_mode->mode_num;
	}

	if( !(mode_num < display_modes.size()) ) {
		return GR_INVALID_PARMS;
	}

	*mode = display_modes[mode_num];
	*/
	return GR_OK;
}

//

DA_METHOD(	get_num_device_texture_formats,(U32 *num_formats))
{
	CHECK_CREATE_BUFFERS(get_num_device_texture_formats);

	*num_formats = texture_formats.size();
	return GR_OK;
}

//

DA_METHOD(	get_device_texture_format,(PixelFormat *texture_pf, U32 format_num))
{
	CHECK_CREATE_BUFFERS(get_device_texture_format);

	if( !(format_num < texture_formats.size()) ) {
		return GR_INVALID_PARMS;
	}

	texture_pf->init( texture_formats[format_num] );
	return GR_OK;
}

// ------------------------------------------------------------------
// Transformation Pipeline Related Code
// ------------------------------------------------------------------

//

DA_METHOD(	set_window,(HWND wnd, int _x, int _y, int _w, int _h ))
{
	/*
	CHECK_CREATE_BUFFERS(set_window);
	ClipperMap::iterator it;
	RPDDCLIPPERDATA *cl, new_cl;
	if( (it = clippers.find( wnd )) == clippers.end() ) {
		new_cl.hwnd = wnd;
		new_cl.clipper = NULL;
		if( (it = clippers.insert( wnd, new_cl )) == clippers.end() ) {
			GENERAL_TRACE_1( _MS(("Direct3D_RenderPipeline: set_window:  could not create clipper\n")) );
			return GR_GENERIC;
		}
	}

	cl = &it->second;

	cl->win_x = _x;
	cl->win_y = _y;
	cl->win_w = _w;
	cl->win_h = _h;

	//if( cl->clipper==NULL && directdraw ) {
	//	if( FAILED( directdraw->CreateClipper( 0, &cl->clipper, NULL ) ) ) {
	//		GENERAL_TRACE_1( _MS(("Direct3D_RenderPipeline: set_window:  could not create clipper\n")) );
	//		return GR_GENERIC;
	//	}

	//	if( FAILED( cl->clipper->SetHWnd( 0, wnd ) ) ) {
	//		GENERAL_TRACE_1( _MS(("Direct3D_RenderPipeline: set_window:  could not set clipper hwnd\n")) );
	//		return GR_GENERIC;
	//	}

	}

	current_clipper = it;

	//if( directdraw_color_buffers[0] && FAILED( directdraw_color_buffers[0]->SetClipper( cl->clipper ) ) ) {
	//	GENERAL_TRACE_1( _MS(("Direct3D_RenderPipeline: set_window:  could not set clipper\n")) );
	//	return GR_GENERIC;
	//}
*/
	return GR_OK;
}

//

DA_METHOD(	get_window,( HWND *out_wnd, int *out_x, int *out_y, int *out_w, int *out_h  ))
{
	CHECK_CREATE_BUFFERS(get_window);

	if( current_clipper == clippers.end() ) {
		return GR_GENERIC;
	}

	*out_wnd = current_clipper->second.hwnd;

	*out_x = current_clipper->second.win_x;
	*out_y = current_clipper->second.win_y;
	*out_w = current_clipper->second.win_w;
	*out_h = current_clipper->second.win_h;

	return GR_OK;
}

//

DA_METHOD(	set_depth_range,( float lower_z_bound, float upper_z_bound ))
{
	CHECK_CREATE_BUFFERS(set_depth_range);
	
	curr_hw_viewport.set_depth_range( direct3d_device, lower_z_bound, upper_z_bound, disable_hw_caches );
	
	return GR_OK;
}

//

DA_METHOD(	get_depth_range,( float *out_lower_z_bound, float *out_upper_z_bound ))
{
	CHECK_CREATE_BUFFERS(get_depth_range);

	curr_hw_viewport.get_depth_range( direct3d_device, out_lower_z_bound, out_upper_z_bound ) ;

	return GR_OK;
}

//

DA_METHOD(	set_viewport,(int x, int y, int w, int h ))
{
	CHECK_CREATE_BUFFERS(set_viewport);

	curr_hw_viewport.set_viewport( direct3d_device, x, y, w, h, disable_hw_caches );

	return GR_OK;
}

//

DA_METHOD(	get_viewport,(int *_x, int *_y, int *_w, int *_h ))
{
	CHECK_CREATE_BUFFERS(get_viewport);

	curr_hw_viewport.get_viewport( direct3d_device, (U32*)_x, (U32*)_y, (U32*)_w, (U32*)_h );

	return GR_OK;
}

//

DA_METHOD(get_modelview,( Transform & modelview ))
{
	CHECK_CREATE_BUFFERS(get_modelview);

	D3DMATRIX M;
	Transform T(false);

	curr_hw_world.get( direct3d_device, D3DTS_WORLD, &M );

	D3D2Transform( M, modelview );

	T.set_identity();
	T.d[2][2] *= -1.0f;

	modelview = T * modelview;

	return GR_OK;
}

//

DA_METHOD(get_projection,( Matrix4 & projection ))
{
	CHECK_CREATE_BUFFERS(get_projection);

	D3DMATRIX P;

	curr_hw_projection.get( direct3d_device, D3DTS_PROJECTION, &P );

	D3D2Matrix( P, projection );

	return GR_OK;
}

//

DA_METHOD(	set_lookat,(float eyex, float eyey, float eyez, float centerx, float centery, float centerz, float upx, float upy, float upz  ))
{
	CHECK_CREATE_BUFFERS(set_lookat);

	Vector target(centerx, centery, centerz);
	Vector pcam(eyex, eyey, eyez);

	Vector look = target - pcam;
	look.normalize();

	// We want the camera's -z axis parallel to look.

	Vector k = -look;
	Vector j(upx, upy, upz);

	// Should add some extra checks here in case j and k are parallel.

	Vector i = cross_product(j, k);
	i.normalize();
	j = cross_product(k, i);
	j.normalize();

	// Now we have our basis vectors for the camera's orientation:
	Matrix cam2world(i, j, k);

	// Get world --> camera.
	Matrix world2cam = cam2world.get_transpose();

	// Express camera position in camera frame:
	Vector xlat = -(world2cam * pcam);

	Transform modelview(world2cam, xlat);
	
	Transform T(false), M(false);
	D3DMATRIX W;

	T.set_identity();
	T.d[2][2] *= -1.0f;
	M = T * modelview ;

	Transform2D3D( W, M );

	curr_hw_world.set( direct3d_device, D3DTS_WORLD, &W, disable_hw_caches );

	return GR_OK;
}

//

DA_METHOD(	set_modelview,(const Transform & modelview ) )
{
	CHECK_CREATE_BUFFERS(set_modelview);

	D3DMATRIX W;

	Transform T(false), M(false);

	T.set_identity();
	T.d[2][2] *= -1.0f;
	M = T * modelview ;

	Transform2D3D( W, M );
	
	curr_hw_world.set( direct3d_device, D3DTS_WORLD, &W, disable_hw_caches );

	return GR_OK;
}

//

DA_METHOD(	set_projection,(const Matrix4 & projection ) )
{
	CHECK_CREATE_BUFFERS(set_projection);

	D3DMATRIX P;

	Matrix2D3D( P, projection );
	
	curr_hw_projection.set( direct3d_device, D3DTS_PROJECTION, &P, disable_hw_caches );

	return GR_OK;
}

//

DA_METHOD(	set_ortho,(float left, float right, float bottom, float top, float nearval, float farval ))
{
	CHECK_CREATE_BUFFERS(set_ortho);

	D3DMATRIX P;

	memset( &P, 0, sizeof(D3DMATRIX) );

	P.m[0][0]	= 2.0 / (right-left);
	P.m[1][1]	= 2.0 / (top-bottom);
	P.m[2][2]	= 2.0 / (farval-nearval);
	P.m[3][0]	= -(right+left) / (right-left);
	P.m[3][1]	= -(bottom+top) / (top-bottom);
	P.m[3][2]	= -(nearval) / (farval-nearval);
	P.m[3][3]	= 1.0;

	rp_rd_ortho( left, right, bottom, top, nearval, farval );

	curr_hw_projection.set( direct3d_device, D3DTS_PROJECTION, &P, disable_hw_caches );

	return GR_OK;
}

//

DA_METHOD(	set_perspective,(float fovy_halfangle, float aspect, float nearval, float farval ) )
{
	CHECK_CREATE_BUFFERS(set_perspective);

	if( nearval<=0.0 || farval<=0.0 || farval == nearval ) {
		GENERAL_TRACE_1( "Direct3D_RenderPipeline: set_perspective: invalid parameters\n" );
		return GR_GENERIC;
	}

	D3DMATRIX P;

	float cot_fovy = 1.0f / tan( Tdeg2rad( fovy_halfangle ) ) ;
	float inv_f_n = 1.0f / (farval - nearval);

	memset( &P, 0, sizeof(D3DMATRIX) );
	
	P.m[0][0] = cot_fovy / aspect; 
	P.m[1][1] = cot_fovy ;
	P.m[2][2] = farval * inv_f_n;
	P.m[2][3] = 1.0f;											
	P.m[3][2] = - farval * nearval * inv_f_n;

	rp_rd_perspective( fovy_halfangle, aspect, nearval, farval );

	curr_hw_projection.set( direct3d_device, D3DTS_PROJECTION, &P, disable_hw_caches );
	

	return GR_OK;
}



// ------------------------------------------------------------------
// State/Scene Management Related Code
// ------------------------------------------------------------------

DA_METHOD(	set_state,(RPRSTATE state, U32 value ))
{
	CHECK_STARTUP(set_state);

	batch_state[state] = value;

	return GR_OK;
}

//

DA_METHOD(	get_state,(RPRSTATE state, U32 *value ))
{
	CHECK_STARTUP(get_state);

	*value = batch_state[state];
	return GR_OK;
}

//

DA_METHOD(	flush,(U32 flags))
{
	CHECK_STARTUP(flush);
	return GR_OK;
}

//

DA_METHOD(	set_pipeline_state,(RPPIPELINESTATE state, DWORD value ))
{
	CHECK_STARTUP(set_pipeline_state);

	ASSERT( state >= 0 && state <= RP_MAX_PIPELINE_STATE );

	if( curr_pipeline_state[state].is_different( value ) ) {

		switch( state ) {

		case RP_BUFFERS_VSYNC:
			if( value ) {
				rprd_buffers_flags |= RPRD_F_HWVSYNC;
			}
			else {
				rprd_buffers_flags &= ~RPRD_F_HWVSYNC;
			}
			break;
			
		case RP_BUFFERS_HWFLIP:
			if( value ) {
				rprd_buffers_flags |= RPRD_F_HWFLIP;
			}
			else {
				rprd_buffers_flags &= ~RPRD_F_HWFLIP;
			}
			break;

		case RP_BUFFERS_SWAP_STALL:
			if( value ) {
				rprd_buffers_flags |= RPRD_F_FLIP_STALLS;
			}
			else {
				rprd_buffers_flags &= ~(RPRD_F_FLIP_STALLS);
			}
			break;

		case RP_BUFFERS_HWMEM:
			if( value ) {
				rprd_buffers_flags |= RPRD_F_HWVIDMEM;
			}
			else {
				rprd_buffers_flags &= ~RPRD_F_HWVIDMEM;
			}
			break;

		case RP_BUFFERS_FULLSCREEN:
			if( value || (rprd_device_flags & RPRD_F_FULLSCREEN_ALWAYS ) ) {
				rprd_buffers_flags |= RPRD_F_FULLSCREEN;
			}
			else {
				rprd_buffers_flags &= ~RPRD_F_FULLSCREEN;
			}
			break;

		case RP_BUFFERS_OFFSCREEN:
			if( value ) {
				rprd_buffers_flags |= RPRD_F_OFFSCREEN;
			}
			else {
				rprd_buffers_flags &= ~RPRD_F_OFFSCREEN;
			}
			break;

		case RP_DEVICE_FPU_SETUP:
			if( value ) {
				rprd_buffers_flags |= RPRD_F_FPUPRESETUP;
				rprd_buffers_flags &= ~(RPRD_F_FPUPRESERVE);
			}
			else {
				rprd_buffers_flags |= RPRD_F_FPUPRESERVE;
				rprd_buffers_flags &= ~(RPRD_F_FPUPRESETUP);
			}
			break;

		case RP_DEVICE_MULTITHREADED:
			if( value ) {
				rprd_buffers_flags |= RPRD_F_HWTHREADED;
			}
			else {
				rprd_buffers_flags &= ~RPRD_F_HWTHREADED;
			}
			break;

		case RP_TEXTURE_ALLOW_8BIT:
			if( value ) {
				rprd_device_flags |= RPRD_F_TEXTURE_8BIT;
			}
			else {
				rprd_device_flags &= ~RPRD_F_TEXTURE_8BIT;
			}
			break;

		case RP_TEXTURE_ALLOW_DXT:
			if( value ) {
				rprd_device_flags |= RPRD_F_TEXTURE_DXT;
			}
			else {
				rprd_device_flags &= ~RPRD_F_TEXTURE_DXT;
			}
			break;

		case RP_DEBUG_FILL_COLOR:
			rp_dd_debug_fill_color = value;
			break;

		case RP_BUFFERS_DEPTH_AUTOW:
			// invalidate the ZENABLE state
			curr_hw_render_state[D3DRS_ZENABLE].invalidate();
			break;

		case RP_BUFFERS_ANTIALIAS:
			if( value ) {
				rprd_buffers_flags |= RPRD_F_ANTIALIAS_HINT;
			}
			else {
				rprd_buffers_flags &= ~(RPRD_F_ANTIALIAS_HINT);
			}
			break;
		case RP_DEVICE_VIDEO_STREAM_MODE:
			if( value ) {
				d3drp_f_flags |= D3DRP_F_USE_DIRECTSHOW;
			}
			else {
				d3drp_f_flags &= ~(D3DRP_F_USE_DIRECTSHOW);
			}
			break;
		}

		curr_pipeline_state[state].set( value );
	}

	return GR_OK;
}

//

DA_METHOD(	get_pipeline_state,(RPPIPELINESTATE state, DWORD *value ))
{
	CHECK_STARTUP(get_pipeline_state);

	ASSERT( state >= 0 && state <= RP_MAX_PIPELINE_STATE );

	switch( state ) {
	
	case RP_BUFFERS_FULLSCREEN:
		*value = (rprd_buffers_flags & RPRD_F_FULLSCREEN_NOW)? 1 : 0;
		break;

	default:
		*value = curr_pipeline_state[state].get();
	}

	return GR_OK;
}

//

DA_METHOD(set_sampler_state,( U32 stage, D3DSAMPLERSTATETYPE state, U32 value ))
{
	direct3d_device->SetSamplerState(stage, state, value);
	return GR_OK;
}

DA_METHOD(get_sampler_state,( U32 stage, D3DSAMPLERSTATETYPE state, U32 *value ))
{
	direct3d_device->GetSamplerState(stage, state, value);
	return GR_OK;
}

DA_METHOD(  set_texture_stage_state,(U32 stage, D3DTEXTURESTAGESTATETYPE state, U32 value ))
{
	CHECK_CREATE_BUFFERS(set_texture_stage_state);
	ASSERT( direct3d_device );

	curr_hw_texture_state[stage][state].set( direct3d_device, stage, state, value, disable_hw_caches );

	return GR_OK;
}

//

DA_METHOD(  get_texture_stage_state,(U32 stage, D3DTEXTURESTAGESTATETYPE state, U32 *value ))
{
	CHECK_CREATE_BUFFERS(get_texture_stage_state);
	ASSERT( direct3d_device );

	*value = curr_hw_texture_state[stage][state].get( direct3d_device, stage, state );

	return GR_OK;
}

//

DA_METHOD(  set_texture_stage_transform, ( U32 stage, Matrix4 &mat4 ) )
{
	CHECK_CREATE_BUFFERS(set_texture_stage_transform);
	ASSERT( direct3d_device );

	D3DMATRIX m;
	
	Matrix2D3D( m, mat4 );

	curr_hw_texture_transform[stage].set( direct3d_device, (D3DTRANSFORMSTATETYPE)(D3DTS_TEXTURE0 + stage), &m, disable_hw_caches );

	return GR_OK;
}

//

DA_METHOD(  get_texture_stage_transform, ( U32 stage, Matrix4 &out_mat4 ) )
{
	CHECK_CREATE_BUFFERS(get_texture_stage_transform);
	ASSERT( direct3d_device );
	
	D3DMATRIX m;

	curr_hw_texture_transform[stage].get( direct3d_device, (D3DTRANSFORMSTATETYPE)(D3DTS_TEXTURE0 + stage), &m );

	D3D2Matrix( m, out_mat4 );

	return GR_OK;
}

//

DA_METHOD(  set_texture_stage_texture,(U32 stage, U32 htexture ))
{
	CHECK_CREATE_BUFFERS(set_texture_stage_texture);
	ASSERT( direct3d_device );

	//if( curr_pipeline_state[RP_TEXTURE].is_enabled() ) 
	{

		if( htexture && FAILED( is_texture( htexture ) ) ) {
			return GR_GENERIC;
		}

		curr_hw_texture[stage].set( direct3d_device, stage, htexture, disable_hw_caches );
	
	}

	return GR_OK;
}

//


DA_METHOD(  get_texture_stage_texture,(U32 stage, U32 *htexture ))
{
	CHECK_CREATE_BUFFERS(get_texture_stage_texture);

	//if( curr_pipeline_state[RP_TEXTURE].is_enabled() ) 
	{
		
		*htexture = curr_hw_texture[stage].get( direct3d_device, stage );

	}
//	else {
//		*htexture = 0;
//	}

	return GR_OK;
}

//

DA_METHOD(	verify_state,( void ))
{
	CHECK_CREATE_BUFFERS(verify_state);
	ASSERT( direct3d_device );

	U32 ret;

	U32 errCode = direct3d_device->ValidateDevice( &ret );

	if( SUCCEEDED( errCode) ) {

		// Put a little extra effort in for DX5 HALs
		//
		// Versions 6.x and higher of Microsoft's Direct3D Runtime 
		// (which talks to the DD/D3D HAL) does not really handle DX5 
		// HALs well.  Invalid multitexture state validates correctly
		// in some cases, so we try to fix that here by explicitly
		// disallowing any multitexture on DX5 HALs.
		//
		if( rprd_device_flags & RPRD_F_BROKEN_MULTITEXTURE ) {

			if( curr_hw_texture_state[1][D3DTSS_COLOROP].get( direct3d_device, 1, D3DTSS_COLOROP ) != D3DTOP_DISABLE ) {
				return GR_GENERIC;
			}

			if( curr_hw_texture_state[1][D3DTSS_ALPHAOP].get( direct3d_device, 1, D3DTSS_ALPHAOP ) != D3DTOP_DISABLE ) {
				return GR_GENERIC;
			}
		}


		return GR_OK;
	}

	return GR_GENERIC;
}

//

GENRESULT Direct3D_RenderPipeline::create_state_block( D3DSTATEBLOCKTYPE type, U32*out_sbhandle ) 
{
	CHECK_CREATE_BUFFERS(create_state_block);
	ASSERT( direct3d_device );	

	if( FAILED( direct3d_device->CreateStateBlock( type, (IDirect3DStateBlock9**)out_sbhandle ) ) ) {
		return GR_GENERIC;
	}

	return GR_OK;	
}

//

GENRESULT Direct3D_RenderPipeline::update_state_block( U32 sbhandle ) 
{
	CHECK_CREATE_BUFFERS(update_state_block);
	ASSERT( direct3d_device );	

	if( FAILED( ((IDirect3DStateBlock9*)sbhandle)->Capture() )) {
		return GR_GENERIC;
	}
	
	return GR_OK;	
}

//

GENRESULT Direct3D_RenderPipeline::begin_state_block( void ) 
{
	CHECK_CREATE_BUFFERS(begin_state_block);
	ASSERT( direct3d_device );	

	if( FAILED( direct3d_device->BeginStateBlock( ) ) ) {
		return GR_GENERIC;
	}

	disable_hw_caches = true;

	return GR_OK;	
}

//

GENRESULT Direct3D_RenderPipeline::end_state_block( U32*out_sbhandle ) 
{
	
	CHECK_CREATE_BUFFERS(end_state_block);
	ASSERT( direct3d_device );	

	disable_hw_caches = false;

	if( FAILED( direct3d_device->EndStateBlock( (IDirect3DStateBlock9**)out_sbhandle ) ) ) {
		return GR_GENERIC;
	}
	return GR_OK;	
}

//

GENRESULT Direct3D_RenderPipeline::apply_state_block( U32 sbhandle ) 
{
	CHECK_CREATE_BUFFERS(apply_state_block);
	ASSERT( direct3d_device );	

	if( FAILED( ((IDirect3DStateBlock9*)sbhandle)->Apply() )) {
		return GR_GENERIC;
	}
	return GR_OK;	
}

//

GENRESULT Direct3D_RenderPipeline::delete_state_block( U32 sbhandle ) 
{
	CHECK_CREATE_BUFFERS(delete_state_block);
	ASSERT( direct3d_device );	
	((IDirect3DStateBlock9*)sbhandle)->Release();
	return GR_OK;	
}

//

DA_METHOD(	set_light,( U32 light_index, D3DLIGHT9 *light_values ))
{
	if (light_values && light_values->Type == D3DLIGHT_POINT && light_index != 7)
	{
		int i = 42;
	}
	CHECK_CREATE_BUFFERS(set_light);
	ASSERT( direct3d_device );	// assert after the create_buffers check

	if( FAILED( direct3d_device->SetLight( light_index, light_values ) ) ) { 
		return GR_GENERIC;	
	}

	if( !disable_hw_caches ) {
		lights[light_index] = light_index;
	}

	rp_rd_light( light_index, light_values );

	return GR_OK;
}

//

DA_METHOD(	get_light,( U32 light_index, D3DLIGHT9 *out_light_values ))
{
	CHECK_CREATE_BUFFERS(get_light);
	ASSERT( direct3d_device );	// assert after the create_buffers check

	if( FAILED( direct3d_device->GetLight( light_index, out_light_values ) ) ) { 
		return GR_GENERIC;	
	}

	return GR_OK;
}

//

DA_METHOD(	set_light_enable,( U32 light_index, U32 enable ))
{
	CHECK_CREATE_BUFFERS(set_light_enable);
	ASSERT( direct3d_device );	// assert after the create_buffers check

	if (!enable) enabledLightCount = 0;
	else
	{
		ASSERT(enabledLightCount < 8);
		enabledLightIndices[enabledLightCount] = light_index;
		enabledLightCount++;
	}

	if( FAILED( direct3d_device->LightEnable( light_index, enable ) ) ) { 
		return GR_GENERIC;	
	}

	if( !disable_hw_caches ) {
		lights[light_index] = light_index;
	}

	rp_rd_light_enable( light_index, enable );

	return GR_OK;
}

//

DA_METHOD(	get_light_enable,( U32 light_index, U32 *out_enable ))
{
	CHECK_CREATE_BUFFERS(get_light_enable);
	ASSERT( direct3d_device );	// assert after the create_buffers check

	if( FAILED( direct3d_device->GetLightEnable( light_index, (int*)out_enable ) ) ) { 
		return GR_GENERIC;	
	}

	return GR_OK;
}

//

GENRESULT Direct3D_RenderPipeline::get_num_lights( U32 *out_num_lights ) 
{
	*out_num_lights = lights.size();
	return GR_OK;
}

//

GENRESULT Direct3D_RenderPipeline::get_lights( U32 num_start, U32 num_lights, U32 *out_light_indices ) 
{
	LightInfoMap::iterator beg = lights.begin();
	LightInfoMap::iterator end = lights.end();
	LightInfoMap::iterator lit;

	U32 cnt;

	// find starting light
	for( cnt=0, lit=beg; cnt<num_start; cnt++,lit++ ) {
		if( lit == end ) {
			return GR_GENERIC;
		}
	}

	// return num_lights
	for(  ; cnt<(num_start+num_lights); cnt++,lit++ ) {

		if( lit == end ) {
			return GR_GENERIC;
		}

		*out_light_indices = lit->second;
		
		out_light_indices++;
	}

	return GR_OK;
}

//

DA_METHOD(	set_material,( D3DMATERIAL9 *material_values ))
{
	CHECK_CREATE_BUFFERS(set_material);
	ASSERT( direct3d_device );	// assert after the create_buffers check

	if( FAILED( direct3d_device->SetMaterial( material_values ) ) ) {
		return GR_GENERIC;
	}

	rp_rd_material( material_values );

	return GR_OK;	
}

//

DA_METHOD(	get_material,( D3DMATERIAL9 *out_material_values ))
{
	CHECK_CREATE_BUFFERS(get_material);
	ASSERT( direct3d_device );	// assert after the create_buffers check

	if( FAILED( direct3d_device->GetMaterial( out_material_values ) ) ) {
		return GR_GENERIC;
	}

	return GR_OK;	
}

//

DA_METHOD(	begin_scene,(void ) )
{
	CHECK_CREATE_BUFFERS(begin_scene);
	ASSERT( direct3d_device );	// assert after the create_buffers check

	if( SUCCEEDED( direct3d_device->BeginScene() ) ) {
		rp_rd_begin_scene();
		rp_rd_clear_prim_counts();
		return GR_OK;
	}

	return GR_GENERIC;
}

//

DA_METHOD(	end_scene,(void ))
{
	CHECK_CREATE_BUFFERS(end_scene);
	ASSERT( direct3d_device );	// assert after the create_buffers check

	if( SUCCEEDED( direct3d_device->EndScene() ) ) {
		rp_rd_end_scene();
		return GR_OK;
	}

	return GR_GENERIC;
}

//

DA_METHOD(	get_render_state,(D3DRENDERSTATETYPE state, DWORD *value ))
{
	CHECK_CREATE_BUFFERS(get_render_state);

	*value = curr_hw_render_state[state].get( direct3d_device, state );

	return GR_OK;
}


//

DA_METHOD(	set_render_state,(D3DRENDERSTATETYPE state, DWORD value ))
{

	CHECK_CREATE_BUFFERS(set_render_state);
	ASSERT( direct3d_device );	// assert after the create_buffers check

#if (ENABLE_RENDERSTATE_CHECK)
	RenderStateArray::iterator rs;
	if( (rs = render_state_info.find( state )) == render_state_info.end() || (rs->second.enum_value != state) ) {
		GENERAL_ERROR( _MS(( "Direct3D_RenderPipeline: set_render_state: %08X (%d) is not a valid D3DRS_!\n", state, state )) );
		return GR_GENERIC;
	}
#endif

/*	if( (state == D3DRS_ANTIALIAS) ) {
		if( !device_abilities[RP_A_DEVICE_FULLSCENE_ANTIALIAS] ) {
			value = 0;
		}
	} */

	curr_hw_render_state[state].set( direct3d_device, state, value, disable_hw_caches );

	return GR_OK;
}





// ------------------------------------------------------------------
// Primitive/List Rendering/Management Related Code
// ------------------------------------------------------------------


U32 GetPrimCount(D3DPRIMITIVETYPE type, U32 indexCount)
{
	switch (type)
	{
	case D3DPT_POINTLIST:
		return indexCount;
	case D3DPT_LINELIST:
		return indexCount/2;
	case D3DPT_LINESTRIP:
		return indexCount - 1;
	case D3DPT_TRIANGLELIST:
		return indexCount/3;
	case D3DPT_TRIANGLESTRIP:
	case D3DPT_TRIANGLEFAN:
		return indexCount-2;
		return indexCount-2;
	default:
		ASSERT(false);
	}
	return 0;
}


DA_METHOD(	draw_primitive,(D3DPRIMITIVETYPE type, U32 vertex_format, const void *_verts, int num_verts, U32 flags ))
{
	CHECK_CREATE_BUFFERS(draw_primitive);
	if( !_verts || !num_verts || !direct3d_device ) {
		return GR_GENERIC;
	}
	U32 stride = FVF_SIZEOF_VERT(vertex_format);
	if( FAILED( direct3d_device->SetFVF(vertex_format)))
	{
		return GR_GENERIC;
	}	
	U32 primCount = GetPrimCount(type, num_verts);
	
	U32 vpx, vpy, vpw, vph;
	curr_hw_viewport.get_viewport( direct3d_device, &vpx, &vpy, &vpw, &vph );
		
	if( FAILED( direct3d_device->DrawPrimitiveUP( type,primCount,(void*)_verts,  stride) ) ) 
	{
		return GR_OK;//GENERIC;
	}
	rp_rd_dp( type, vertex_format, _verts, num_verts, flags );
	return GR_OK;
}

//

DA_METHOD(	draw_indexed_primitive,(D3DPRIMITIVETYPE type, U32 vertex_format, const void *_verts, int num_verts, const U16 * indices, int num_indices, U32 flags ))
{
	CHECK_CREATE_BUFFERS(draw_indexed_primitive);

	if (!_verts || !num_verts || !indices || !num_indices || !direct3d_device ) {
		return GR_GENERIC;
	}

	U32 primCount = GetPrimCount(type, num_indices);
	U32 stride = FVF_SIZEOF_VERT(vertex_format);

	if( FAILED( direct3d_device->SetFVF(vertex_format)))
	{
		return GR_GENERIC;
	}	

	if( FAILED( direct3d_device->DrawIndexedPrimitiveUP(type,0,num_verts,primCount,indices,D3DFMT_INDEX16,_verts,stride) ) ) {
		return GR_GENERIC;
	}

	rp_rd_dip( type, vertex_format, _verts, num_verts, indices, num_indices, flags );

	return GR_OK;
}

//

GENRESULT Direct3D_RenderPipeline::draw_primitive_vb( D3DPRIMITIVETYPE type, IRP_VERTEXBUFFERHANDLE vbhandle, int start_vert, int num_verts, U32 flags  ) 
{
	CHECK_CREATE_BUFFERS(draw_primitive_vb);


	if( !vbhandle || !num_verts || !direct3d_device ) {
		return GR_GENERIC;
	}

	LPDIRECT3DVERTEXBUFFER9 vb = (LPDIRECT3DVERTEXBUFFER9)vbhandle;

	D3DVERTEXBUFFER_DESC desc;
	if( FAILED(vb->GetDesc(&desc))) 
	{
		return GR_GENERIC;
	}

	U32 stride = FVF_SIZEOF_VERT( desc.FVF );
	if( FAILED(direct3d_device->SetStreamSource(0,vb,0,stride)))
	{
		return GR_GENERIC;
	}

	if( FAILED( direct3d_device->SetFVF(desc.FVF)))
	{
		return GR_GENERIC;
	}

	U32 primCount = GetPrimCount(type, num_verts);
	if( FAILED( direct3d_device->DrawPrimitive( type,start_vert, primCount) ) ) {
		return GR_GENERIC;
	}

	rp_rd_dp_vb( type, vbhandle, start_vert, num_verts, flags );

	return GR_OK;
}

//

GENRESULT Direct3D_RenderPipeline::draw_indexed_primitive_vb( D3DPRIMITIVETYPE type, IRP_VERTEXBUFFERHANDLE vbhandle, int start_vert, int num_verts, const U16 * indices, int num_indices, U32 flags  ) 
{
	CHECK_CREATE_BUFFERS(draw_indexed_primitive_vb);

	if( !vbhandle || !num_verts || !indices || !num_indices || !direct3d_device ) {
		return GR_GENERIC;
	}

	

	LPDIRECT3DVERTEXBUFFER9 vb = (LPDIRECT3DVERTEXBUFFER9)vbhandle;

	D3DVERTEXBUFFER_DESC desc;
	if( FAILED(vb->GetDesc(&desc))) 
	{
		return GR_GENERIC;
	}

	U32 stride = FVF_SIZEOF_VERT( desc.FVF );
	if( FAILED(direct3d_device->SetStreamSource(0,vb,0,stride)))
	{
		return GR_GENERIC;
	}
#define SCRATCHICOUNT 3000
	if (scratchIB == 0)
	{
		direct3d_device->CreateIndexBuffer(SCRATCHICOUNT * 2,0,
			D3DFMT_INDEX16,
			D3DPOOL_MANAGED,
			(IDirect3DIndexBuffer9**) &scratchIB,
			NULL);
	}

	ASSERT(num_indices < SCRATCHICOUNT);

	void* lockedInds;
	scratchIB->Lock(0,num_indices * sizeof(U16),&lockedInds,0);
	memcpy(lockedInds, indices, num_indices * sizeof(U16));
	scratchIB->Unlock();

	if( FAILED(direct3d_device->SetIndices( scratchIB)))
	{
			return GR_GENERIC;
	}

	if( FAILED( direct3d_device->SetFVF(desc.FVF)))
	{
		return GR_GENERIC;
	}

	U32 primCount = GetPrimCount(type, num_indices);
	if( FAILED( direct3d_device->DrawIndexedPrimitive(type,start_vert,0,num_verts,0,primCount))) {
		return GR_GENERIC;
	}

	rp_rd_dip_vb( type, vbhandle, start_vert, num_verts, indices, num_indices, flags );

	return GR_OK;
}


GENRESULT Direct3D_RenderPipeline::draw_indexed_primitive_vb( D3DPRIMITIVETYPE type, IRP_VERTEXBUFFERHANDLE vbhandle, int start_vert, int num_verts, IDirect3DIndexBuffer9* IB, int num_indices, U32 flags  ) 
{
	CHECK_CREATE_BUFFERS(draw_indexed_primitive_vb);
	if( !vbhandle || !num_verts || !IB || !num_indices || !direct3d_device ) {
		return GR_GENERIC;
	}


	LPDIRECT3DVERTEXBUFFER9 vb = (LPDIRECT3DVERTEXBUFFER9)vbhandle;

	D3DVERTEXBUFFER_DESC desc;
	if( FAILED(vb->GetDesc(&desc))) 
	{
		return GR_GENERIC;
	}

	U32 stride = FVF_SIZEOF_VERT( desc.FVF );
	if( FAILED(direct3d_device->SetStreamSource(0,vb,0,stride)))
	{
		return GR_GENERIC;
	}

	if( FAILED(direct3d_device->SetIndices(IB)))
	{
			return GR_GENERIC;
	}
	if( FAILED( direct3d_device->SetFVF(desc.FVF)))
	{
		return GR_GENERIC;
	}
	U32 primCount = GetPrimCount(type, num_indices);
	if( FAILED( direct3d_device->DrawIndexedPrimitive(type,start_vert,0,num_verts,0,primCount))) {
		return GR_GENERIC;
	}
	//rp_rd_dip_vb( type, vbhandle, start_vert, num_verts, indices, num_indices, flags );
	return GR_OK;
}


DA_METHOD( create_index_buffer(UINT Length, IDirect3DIndexBuffer9** ppIndexBuffer))
{
	direct3d_device->CreateIndexBuffer(Length, 0,D3DFMT_INDEX16, D3DPOOL_MANAGED,ppIndexBuffer, NULL);
	return GR_OK;
}
 	


//

DA_METHOD(	get_buffer_interface,(const char *iid, void **out_iif ))
{
	/*
	CHECK_CREATE_BUFFERS(get_buffer_interface);

	if( strcmp( iid, IID_IVideoStreamControl ) == 0 ) {

		if( buffers_video_stream == NULL ) {
			if( d3drp_f_flags & D3DRP_F_USE_DIRECTSHOW ) {
				if( FAILED( rp_ds_vstream_control_create( directdraw, directdraw_color_buffers[1], 0, &buffers_video_stream ) ) ) {
					GENERAL_TRACE_1( "Direct3D_RenderPipeline: get_buffer_interface: failed to create ds vstream control\n" );
					return GR_GENERIC;
				}
			}
			else {
				if( FAILED( rp_vfw_vstream_control_create( directdraw, directdraw_color_buffers[1], 0, &buffers_video_stream ) ) ) {
					GENERAL_TRACE_1( "Direct3D_RenderPipeline: get_buffer_interface: failed to create vfw vstream control\n" );
					return GR_GENERIC;
				}
			}
		}

		ASSERT( buffers_video_stream != NULL );

		IVideoStreamControl **ivsc = (IVideoStreamControl **)out_iif;
		*ivsc = buffers_video_stream;
		buffers_video_stream->AddRef();
	
		return GR_OK;
	}*/

	return GR_GENERIC;
}

// ------------------------------------------------------------------
// Textures and TextureManagement Related code
// ------------------------------------------------------------------

DA_METHOD(	get_texture_interface,(U32 htexture, const char *iid, void **out_iif ))
{
	/*
	CHECK_CREATE_BUFFERS(get_texture_interface);
	ASSERT( (htexture != RP_CURRENT) && "RP_CURRENT as a texture handle not supported anymore" );

	*out_iif = NULL;

	if( htexture == 0 ) {
		return GR_GENERIC;
	}

	LPDIRECTDRAWSURFACE7 surface = (LPDIRECTDRAWSURFACE7)htexture;
	IVideoStreamControl *video_stream = NULL;
	unsigned long sizeof_data = sizeof(video_stream);

	if( FAILED( surface->GetPrivateData( TID_IVideoStreamControl, &video_stream, &sizeof_data ) ) ) {
		GENERAL_TRACE_1( "Direct3D_RenderPipeline: get_texture_interface: failed to retrieve vstream control\n" );
		return GR_GENERIC;
	}

	*out_iif = video_stream;
	video_stream->AddRef();
*/
	return GR_OK;
}

//

DA_METHOD(	get_texture_dc,( U32 htexture, HDC *dc ))
{
	CHECK_CREATE_BUFFERS(get_dc);
	ASSERT( htexture != RP_CURRENT );

	*dc = NULL;

	if( htexture ) {
		IDirect3DSurface9 * surface;
		((IDirect3DTexture9*)htexture)->GetSurfaceLevel(0,&surface);
		if( SUCCEEDED(surface->GetDC(dc))) 
		{
			return GR_OK;
		}
	}
	return GR_GENERIC;
}

//

DA_METHOD(	release_texture_dc,(U32 htexture, HDC dc ))
{
	CHECK_CREATE_BUFFERS(get_dc);
	ASSERT( htexture != RP_CURRENT );
	if( htexture ) 
	{
		IDirect3DSurface9 * surface;
		((IDirect3DTexture9*)htexture)->GetSurfaceLevel(0,&surface);
		if( SUCCEEDED( surface->ReleaseDC( dc ) ) ) 
		{
			return GR_OK;
		}
	}
	return GR_GENERIC;
}

//

DA_METHOD(	create_texture,(int width, int height, const PixelFormat &desiredformat, int numTextureLevels, U32 irp_cf_flags, U32 &out_htexture ))
{
	CHECK_CREATE_BUFFERS(create_texture);
	ASSERT( width!=0 );
	ASSERT( height!=0 );
	
	// we always want a specified number of mip levels
	if (numTextureLevels == 0) numTextureLevels = 1;

#pragma warning( disable : 4800 )
	bool has_video_stream = ((bool)(irp_cf_flags & IRP_CTF_VIDEO_TARGET));
	bool is_actual_texture = !((bool)(irp_cf_flags & IRP_CTF_NON_TEXTURE));

	D3DFORMAT texFmt;
	if (desiredformat.ddpf.dwFourCC == 1414480196) 
	{
		texFmt = D3DFMT_X8R8G8B8;
	}
	else 
	{
		texFmt = D3DFMT_A8R8G8B8;
	}
	

	
	D3DXCreateTexture(direct3d_device,width, height, numTextureLevels, 0,texFmt, D3DPOOL_MANAGED,(LPDIRECT3DTEXTURE9 *) (&out_htexture));

	//D3DXCreateTextureFromFile(direct3d_device, "Z:\\CQ2\\test\\test.bmp", 	(LPDIRECT3DTEXTURE9 *) (&out_htexture));

	return GR_OK;
}


DA_METHOD(create_cube_texture_from_file,(const char* filename, IComponentFactory * DIR,U32 &out_htexture))
{
	CHECK_CREATE_BUFFERS(create_texture);
	DAFILEDESC fdesc = filename;
	COMPTR<IFileSystem> file;
	if (DIR->CreateInstance(&fdesc, file) != GR_OK)
	{
		GENERAL_TRACE_1("Failed to find a cube map! ");
		GENERAL_TRACE_1(filename);
		GENERAL_TRACE_1("\n");
		out_htexture = 0;
		return GR_GENERIC;
	}

	U32 fileSize = file->GetFileSize();
	char * data = new char[fileSize+1];
	U32 dwRead = 0;
	file->ReadFile(0, data, fileSize,&dwRead,0);
	D3DXCreateCubeTextureFromFileInMemory(direct3d_device,data,fileSize,(LPDIRECT3DCUBETEXTURE9*) (&out_htexture));
	delete data;
	return GR_OK;
}


//

DA_METHOD(	destroy_texture,(U32 htexture ) )
{
	if (!htexture) return GR_OK;
	((LPDIRECT3DTEXTURE9)htexture)->Release();
	return GR_OK;
}

//

DA_METHOD(	is_texture,	(U32 htexture ))
{
	/*
	CHECK_CREATE_BUFFERS(is_texture);
	ASSERT( htexture != RP_CURRENT );

	if( htexture ) {

		LPDIRECTDRAWSURFACE7 surface = ((LPDIRECTDRAWSURFACE7)htexture);
		DDSURFACEDESC2 ddsd2;
		
		ddsd2.dwSize = sizeof(ddsd2);

		if( IsBadReadPtr( surface, sizeof(DWORD)*3 ) ) {
			// pointer is no longer valid
			return GR_GENERIC;
		}

		if( *((U32*)surface) == 0 ) {
			// interface has been released
			return GR_GENERIC;
		}

		if( SUCCEEDED( surface->GetSurfaceDesc( &ddsd2 ) ) ) {
			return (ddsd2.ddsCaps.dwCaps & DDSCAPS_TEXTURE)? GR_OK : GR_GENERIC;
		}
	}
*/
	return GR_OK;

}


//

DA_METHOD(	get_texture_format,( U32 htexture, PixelFormat *out_pf ))
{
	CHECK_CREATE_BUFFERS(get_texture_format);
	ASSERT( htexture != RP_CURRENT );
	IDirect3DTexture9* tex = (IDirect3DTexture9*) htexture;
	D3DSURFACE_DESC desc;
	tex->GetLevelDesc(0, &desc);
	switch (desc.Format)
	{
	case D3DFMT_A8R8G8B8:
		out_pf->init(32,8,8,8,8);
		break;
	case D3DFMT_X8R8G8B8:
		out_pf->init(PF_4CC_DAOT);
		break;
	default:
		int i = 42;
		break;
	}
	return GR_OK;
}

//

DA_METHOD(	get_texture_dim,(U32 htexture, U32 *out_width, U32 *out_height, U32 *out_num_lod ))
{
	if (!htexture) return GR_GENERIC;
	CHECK_CREATE_BUFFERS(get_texture_dim);
	ASSERT( htexture != RP_CURRENT );
	IDirect3DTexture9* tex = (IDirect3DTexture9*) htexture;
	*out_num_lod = tex->GetLevelCount();
	D3DSURFACE_DESC desc;
	tex->GetLevelDesc(0, &desc);
	*out_width = desc.Width;
	*out_height = desc.Height;
	return GR_OK;
}


//

DA_METHOD(	lock_texture,(U32 htexture, int subsurface, RPLOCKDATA *lockData ))
{
	CHECK_CREATE_BUFFERS(lock_texture);
	ASSERT( htexture != RP_CURRENT );
	if (!htexture) return GR_GENERIC;
	IDirect3DTexture9 * tex = (IDirect3DTexture9*) htexture;
	D3DLOCKED_RECT rect;
	tex->LockRect(subsurface, &rect,NULL,0);
	D3DSURFACE_DESC desc;
	tex->GetLevelDesc(0, &desc);
	switch (desc.Format)
	{
	case D3DFMT_A8R8G8B8:
		lockData->pf.init(32,8,8,8,8);
		break;
	case D3DFMT_X8R8G8B8:
		lockData->pf.init(32,8,8,8,0);
		break;
	default:
		int i = 42;
		break;
	}
	
	lockData->width = desc.Width;
	lockData->height = desc.Height;
	lockData->pitch = rect.Pitch;
	lockData->pixels = rect.pBits;

	return GR_GENERIC;
}

//
DA_METHOD(	unlock_texture,(U32 htexture, int subsurface ))
{
	if (!htexture) return GR_GENERIC;
	IDirect3DTexture9 * tex = (IDirect3DTexture9*) htexture;
	tex->UnlockRect(subsurface);
	return GR_GENERIC;
}

//

DA_METHOD(	set_texture_palette,(U32 htexture, int start, int length, const RGB *colors ))
{
	/*
	CHECK_CREATE_BUFFERS(set_texture_palette);
	ASSERT( htexture != RP_CURRENT );

	if( htexture ) {

		LPDIRECTDRAWSURFACE7 surface = ((LPDIRECTDRAWSURFACE7)htexture);
		
		if( SUCCEEDED( rp_dd_surface_set_palette( directdraw, surface, start, length, (RGB*)colors ) ) ) {
		
			for( int s=0; s<D3DTSS_NUM_STAGES; s++ ) {
				if( curr_hw_texture[s].get( direct3d_device, s ) == htexture ) {
					curr_hw_texture[s].invalidate();
				}
			}
			
			return GR_OK;
		}
	}
	*/
	return GR_GENERIC;
}

//
DA_METHOD(	get_texture_palette,(U32 htexture, int start, int length, RGB *colors ))
{
	/*
	CHECK_CREATE_BUFFERS(get_texture_palette);
	ASSERT( htexture != RP_CURRENT );

	if( htexture ) {

		LPDIRECTDRAWSURFACE7 surface = ((LPDIRECTDRAWSURFACE7)htexture);
		
		if( SUCCEEDED( rp_dd_surface_get_palette( surface, start, length, (RGB*)colors ) ) ) {
		
			for( int s=0; s<D3DTSS_NUM_STAGES; s++ ) {
				if( curr_hw_texture[s].get( direct3d_device, s ) == htexture ) {
					curr_hw_texture[s].invalidate();
				}
			}
			
			return GR_OK;
		}
	}
	*/
	return GR_GENERIC;
}

//

DA_METHOD(	blit_texture,(U32 hDest, U32 dst_subsurface, RECT destRect, U32 hSrc, U32 src_subsurface, RECT srcRect ))
{
	static bool bSkip = 0;
	if (bSkip) return GR_OK;

	CHECK_CREATE_BUFFERS(blit_texture);
	ASSERT( hDest != RP_CURRENT );
	ASSERT( hSrc != RP_CURRENT );

	if (!hDest) return GR_GENERIC;
	if (!hSrc) return GR_GENERIC;

	static bool bFastMethod = 1;
	if (bFastMethod)
	{
		HDC destDC, srcDC;
		get_texture_dc(hDest,&destDC);
		get_texture_dc(hSrc,&srcDC);


		StretchBlt( destDC, // handle to destination DC
					destRect.left ,  // x-coord of destination upper-left corner
					destRect.top,  // y-coord of destination upper-left corner
					destRect.right - destRect.left,  // width of destination rectangle
					destRect.bottom - destRect.top, // height of destination rectangle
					srcDC,  // handle to source DC
					srcRect.left ,   // x-coordinate of source upper-left corner
					srcRect.top ,   // y-coordinate of source upper-left corner
					srcRect.right - srcRect.left,  // width of source rectangle
					srcRect.bottom - srcRect.top, // height of source rectangle
					SRCCOPY);
		release_texture_dc(hDest,destDC);
		release_texture_dc(hSrc,srcDC);
		return GR_OK;
	}
	// slow-but-safe method:
	
	IDirect3DTexture9 * destTex = (IDirect3DTexture9*) hDest;
	IDirect3DTexture9 * srcTex = (IDirect3DTexture9*) hSrc;

	D3DLOCKED_RECT rectSrc, rectDest;
	destTex->LockRect(dst_subsurface, &rectDest,NULL,0);
	srcTex->LockRect(src_subsurface, &rectSrc,NULL,0);

	U8			*dst_bits  = static_cast<byte *>(rectDest.pBits);
	U32			 dst_bd = 32;	// dst byte depth
	U32			 dst_stride = rectDest.Pitch;
	PixelFormat  dst_format(32,8,8,8,8);
	U32			 dst_width = destRect.right;
	U32			 dst_height = destRect.bottom;

	U8			*src_bits  = static_cast<byte *>(rectSrc.pBits);
	U32			 srcStride = rectSrc.Pitch;
	PixelFormat  srcFormat(32,8,8,8,8);
	U32			 srcWidth = srcRect.right;
	U32			 srcHeight = srcRect.bottom;

	mem_bitblt( dst_bits, dst_width, dst_height, dst_stride, dst_format,
				src_bits, srcWidth, srcHeight,  srcStride,  srcFormat,
				NULL,NULL);

	destTex->UnlockRect(dst_subsurface);
	srcTex->UnlockRect(src_subsurface);
	return GR_OK;
}

//

DA_METHOD(	set_texture_level_data,(U32 htexture, int subsurface, int srcWidth, int srcHeight, int srcStride, const PixelFormat &srcFormat, const void *srcPixels, const void *srcAlphaMap, const RGB *srcPalette ))
{
	CHECK_CREATE_BUFFERS(set_texture_level_data);
	ASSERT( htexture != RP_CURRENT );
	IDirect3DTexture9 * tex = (IDirect3DTexture9*) htexture;
	if (!tex) return GR_GENERIC;
	D3DLOCKED_RECT rect;
	tex->LockRect(subsurface, &rect,NULL,0);
	U8* pTextureBuffer = static_cast<byte *>(rect.pBits);
	LONG lTexturePitch = rect.Pitch;
	U8 * pSourceBuffer = (byte*) srcPixels;


	U8			*dst_bits = pTextureBuffer;
	U32			 dst_bd = 32;	// dst byte depth
	U32			 dst_stride = lTexturePitch; // - ddsd2.dwWidth * dst_bd;	
	PixelFormat  dst_format(32,8,8,8,8);
	U32			 dst_width = srcWidth;
	U32			 dst_height = srcHeight;
    mem_bitblt( dst_bits, dst_width, dst_height, dst_stride, dst_format,
				srcPixels, srcWidth, srcHeight, srcStride, srcFormat,
				srcPalette, (const U8*)srcAlphaMap);

/*
	if (!srcPalette || srcStride == srcWidth * 3)
	{
		
		static int test = srcStride;

		if (test != srcStride)
		{
			test = srcStride;
		}
		if (srcAlphaMap)
		{
			ASSERT(!srcAlphaMap);	// for debugging
		}
		for(U32 row = 0; row < srcHeight; row++ ) 
		{
			U8 *pTxtBufferOld = pTextureBuffer;
			for (U32 col = 0; col < srcWidth; col++) 
			{
				pTextureBuffer[0] = pSourceBuffer[2];
				pTextureBuffer[1] = pSourceBuffer[1];
				pTextureBuffer[2] = pSourceBuffer[0];
				pTextureBuffer[3] = pSourceBuffer[3];
				pTextureBuffer += 4;
				pSourceBuffer += 4;
			}
			pTextureBuffer = pTxtBufferOld + lTexturePitch;
		}
	}
	else
	{
		static int test = srcStride;

		if (test != srcStride)
		{
			test = srcStride;
		}

		U8 * alphaMap = (U8*) srcAlphaMap;
		for(U32 row = 0; row < srcHeight; row++ ) 
		{
			U8 *pTxtBufferOld = pTextureBuffer;
			for (U32 col = 0; col < srcWidth; col++) 
			{
				U8 source[3];
				if (srcFormat.ddpf.dwRGBBitCount == 16)
				{
					U16 tmp = ((U16*)srcPalette)[pSourceBuffer[0]];
					source[0] = 0;
					source[1] = 255;
					source[2] = 255;
				}
				else
				{
					U8* tmp = (U8*)&(srcPalette[pSourceBuffer[0]]);
					source[0] = tmp[0];
					source[1] = tmp[1];
					source[2] = tmp[2];
				}
				pTextureBuffer[0] = source[2];
				pTextureBuffer[1] = source[1];
				pTextureBuffer[2] = source[0];
				if (alphaMap)
				{
					pTextureBuffer[3] = alphaMap[0];
					alphaMap++;
				}
				else
				{
					pTextureBuffer[3] = 255;
				}
				pTextureBuffer += 4;
				pSourceBuffer ++;
			}
			pTextureBuffer = pTxtBufferOld + lTexturePitch;
		}
	}
*/
	tex->UnlockRect(subsurface);
	return GR_OK;
}


DA_METHOD(  get_num_textures,(U32 *out_num_textures ))
{
	CHECK_CREATE_BUFFERS(get_num_textures);

	*out_num_textures = 0;

	return GR_GENERIC;
}


DA_METHOD(  get_texture,(U32 texture_num, U32 *out_htexture ))
{
	CHECK_CREATE_BUFFERS(get_texture);

	*out_htexture = 0;

	return GR_GENERIC;
}


// ------------------------------------------------------------------
// Render Device/Surface Related code
// ------------------------------------------------------------------


DA_METHOD(	get_buffer_dc,(HDC *dc ))
{
		return GR_GENERIC;
		/*
	CHECK_CREATE_BUFFERS(get_dc);
//	ASSERT( directdraw_color_buffers[1] );	// assert after the create_buffers check

	*dc = NULL;

	if( SUCCEEDED( directdraw_color_buffers[1]->GetDC( dc ) ) ) {
		return GR_OK;
	}
	
	return GR_GENERIC;
	*/
}

DA_METHOD(	release_buffer_dc,(HDC dc ))
{
	/*
	CHECK_CREATE_BUFFERS(release_dc);
	ASSERT( directdraw_color_buffers[1] );	// assert after the create_buffers check

	directdraw_color_buffers[1]->ReleaseDC( dc );
*/
	return GR_OK;
}


DA_METHOD(	clear_buffers,(DWORD flag, RECT *rect ))
{
	CHECK_CREATE_BUFFERS(clear_buffers);

	ASSERT( (flag&(RP_CLEAR_COLOR_BIT|RP_CLEAR_DEPTH_BIT|RP_CLEAR_STENCIL_BIT)) );

	U32 vpx, vpy, vpw, vph;
	RECT scissor;

	curr_hw_viewport.get_viewport( direct3d_device, &vpx, &vpy, &vpw, &vph );

	SetRect( &scissor, vpx, vpy, vpx+vpw, vpy+vph );

	if( rect ) {
		scissor.left   += rect->left;
		scissor.top    += rect->top;
		scissor.right   = scissor.left + (rect->right - rect->left);
		scissor.bottom  = scissor.top + (rect->bottom - rect->top);
	}

	U32 cflags = 0;
	float zclear = 1.0f;

	if( !(d3drp_f_flags & D3DRP_F_STENCIL_CREATED) ) {
		flag &= ~(RP_CLEAR_STENCIL_BIT);
	}
	if( !(d3drp_f_flags & D3DRP_F_DEPTH_CREATED) ) {
		flag &= ~(RP_CLEAR_DEPTH_BIT);
	}

	if( flag & RP_CLEAR_COLOR_BIT ) {
		cflags |= D3DCLEAR_TARGET;
	}
	if( flag & RP_CLEAR_DEPTH_BIT ) {
		cflags |= D3DCLEAR_ZBUFFER;
		zclear = (float)( ((double)(curr_pipeline_state[RP_CLEAR_DEPTH].get())) / ((double)(0xFFFFFFFF)) );
	}
	if( flag & RP_CLEAR_STENCIL_BIT ) {
		cflags |= D3DCLEAR_STENCIL;
	}

	HRESULT hr;

	hr = direct3d_device->Clear( 0, 
								NULL,
								// (D3DRECT*)&scissor, 
								 cflags, 
								 //curr_pipeline_state[RP_CLEAR_COLOR].get(), 
								 D3DCOLOR_XRGB(0,0,0),
								 1.0, 
								 curr_pipeline_state[RP_CLEAR_STENCIL].get() );

	if( FAILED( hr ) ) {
		return GR_GENERIC;
	}

	rp_rd_clear( (D3DRECT*)&scissor, cflags, curr_pipeline_state[RP_CLEAR_COLOR].get(), zclear, curr_pipeline_state[RP_CLEAR_STENCIL].get() );

	return GR_OK;
}


DA_METHOD(	swap_buffers,(void ))
{
	direct3d_device->Present(NULL,NULL,NULL,NULL);
	/*
	CHECK_CREATE_BUFFERS(swap_buffers);

	RECT src, dst, r;
	
	U32 src_w = curr_pipeline_state[RP_BUFFERS_WIDTH].get();
	U32 src_h = curr_pipeline_state[RP_BUFFERS_HEIGHT].get();

	if( current_clipper != clippers.end() ) {
		src_w = __min( current_clipper->second.win_w, src_w );
		src_h = __min( current_clipper->second.win_h, src_h );
		GetClientRect( current_clipper->second.hwnd, &r );
		ClientToScreen( current_clipper->second.hwnd, (POINT*)&r );
		SetRect( &dst,	r.left + current_clipper->second.win_x, 
						r.top  + current_clipper->second.win_y, 
						r.left + current_clipper->second.win_x + current_clipper->second.win_w, 
						r.top  + current_clipper->second.win_y + current_clipper->second.win_h );
	}
	else {
		GENERAL_TRACE_1( "Direct3D_RenderPipeline: swap_buffers: called with no current clipper\n" );
		SetRect( &dst, 0, 0, src_w, src_h );
	}
	
	SetRect( &src, 0, 0, src_w, src_h );

	rp_rd_swap();

	HRESULT hr;
	if( FAILED( hr = rp_dd_swap_buffers( directdraw, directdraw_color_buffers, 2, &src, &dst, rprd_buffers_flags ) ) ) {
		
		if( hr == DDERR_SURFACELOST ) {
		
			COMPTR<IDirect3D9> direct3d;

			//rp_dd_surface_verify( directdraw_color_buffers[0] );
			//rp_dd_surface_verify( directdraw_color_buffers[1] );
//			rp_dd_surface_verify( direct3d_depth_buffer );

			for( int s=0; s<D3DTSS_NUM_STAGES; s++ ) {
				curr_hw_texture[s].invalidate();
			}

	//		if( direct3d_device && SUCCEEDED( direct3d_device->GetDirect3D( direct3d ) ) ) {
	//			direct3d->EvictManagedTextures();
			//}
		}
	}
*/
	return GR_OK;
}


DA_METHOD(	lock_buffer,(RPLOCKDATA *lockData ))
{
	/*
	CHECK_CREATE_BUFFERS(lock_buffer);
	ASSERT( directdraw_color_buffers[1] ) ;

	DDSURFACEDESC2 ddsd2;
	memset( &ddsd2, 0, sizeof(ddsd2) );
	ddsd2.dwSize = sizeof(ddsd2);

	if( SUCCEEDED( directdraw_color_buffers[1]->Lock( NULL, &ddsd2, rprd_lock_flags|DDLOCK_WAIT, 0 ) ) ) {
		lockData->pixels = ddsd2.lpSurface;
		lockData->width = ddsd2.dwWidth;
		lockData->pitch = ddsd2.lPitch;
		lockData->height = ddsd2.dwHeight;
		lockData->pf.init( ddsd2.ddpfPixelFormat );
	
		return GR_OK;
	}
*/
	return GR_GENERIC;
}


DA_METHOD(	unlock_buffer,(void ))
{
//	CHECK_CREATE_BUFFERS(unlock_buffer);
//	ASSERT( directdraw_color_buffers[1] );

//	directdraw_color_buffers[1]->Unlock( NULL );
	
	return GR_OK;
}


static HWND devWindow = 0;

DA_METHOD(	create_buffers,(HWND hwnd, int hres, int vres ))
{
	CHECK_STARTUP(create_buffers);
	HRESULT hr;
	d3drp_f_flags &= ~(D3DRP_F_CHECK_CREATE_BUFFERS);
	destroy_buffers();
	
	devWindow = hwnd;
	direct3d_device = NULL ;
	d3drp_f_flags |= D3DRP_F_BUFFERS_CREATED;
	d3drp_f_flags |= D3DRP_F_DEPTH_CREATED;

	D3DPRESENT_PARAMETERS params;
	params.BackBufferWidth = 0;
	params.BackBufferHeight = 0;
    params.BackBufferFormat = D3DFMT_UNKNOWN ;
    params.BackBufferCount = 1;
    params.MultiSampleType = D3DMULTISAMPLE_NONE;
    params.MultiSampleQuality = 0;
    params.SwapEffect = D3DSWAPEFFECT_COPY;
    params.hDeviceWindow = hwnd;
    params.Windowed = true;
    params.EnableAutoDepthStencil = true;
    params.AutoDepthStencilFormat = D3DFMT_D24X8;
    params.Flags = 0;
    params.FullScreen_RefreshRateInHz = 0;
    params.PresentationInterval = D3DPRESENT_INTERVAL_DEFAULT;

	IDirect3D9 *direct3d = Direct3DCreate9(D3D_SDK_VERSION);
	direct3d->CreateDevice(0,D3DDEVTYPE_HAL,hwnd,D3DCREATE_HARDWARE_VERTEXPROCESSING | D3DCREATE_MULTITHREADED,&params,&direct3d_device);
	

	LPD3DXBUFFER pBufferErrors = NULL;
	ID3DXEffect* effect = 0;

	internal_set_abilities();

	lights.clear();
	d3drp_f_flags |= D3DRP_F_CHECK_CREATE_BUFFERS;
	return GR_OK;
}


DA_METHOD(	destroy_buffers,(void ))
{

	if (!devWindow) return GR_OK;
	D3DPRESENT_PARAMETERS params;
	params.BackBufferWidth = 0;
	params.BackBufferHeight = 0;
    params.BackBufferFormat = D3DFMT_UNKNOWN ;
    params.BackBufferCount = 1;
    params.MultiSampleType = D3DMULTISAMPLE_NONE;
    params.MultiSampleQuality = 0;
    params.SwapEffect = D3DSWAPEFFECT_COPY;
    params.hDeviceWindow = devWindow;
    params.Windowed = true;
    params.EnableAutoDepthStencil = true;
    params.AutoDepthStencilFormat = D3DFMT_D24X8;
    params.Flags = 0;
    params.FullScreen_RefreshRateInHz = 0;
    params.PresentationInterval = D3DPRESENT_INTERVAL_DEFAULT;
	
	
    while(FAILED(direct3d_device->Reset(&params)))
	{
		Sleep(100);
	}
	
	effectList* tmp = effectsForRestoration;

	while (tmp)
	{
		tmp->loadEffect();
		tmp = tmp->next;
	}

	return GR_OK;

	/*
	// DONT CALL CHECK_STARTUP(destroy_buffers), as destroy_buffers() is called by startup()

	COMPTR<IDirect3D9> direct3d;
//	if( direct3d_device && SUCCEEDED( direct3d_device->GetDirect3D( direct3d ) ) ) {
//		direct3d->EvictManagedTextures();
//	}

	DACOM_RELEASE( buffers_video_stream );
	if( d3drp_f_flags & D3DRP_F_BUFFERS_CREATED ) {
		if( rprd_buffers_flags & RPRD_F_FULLSCREEN_NOW ) {
			if( FAILED( rp_dd_set_windowed( directdraw, current_clipper->second.hwnd, rprd_buffers_flags ) ) ) {
				GENERAL_TRACE_1( "Direct3D_RenderPipeline: create_buffers: failed to set window\n" );
				return GR_GENERIC;
			}
			rprd_buffers_flags &= ~RPRD_F_FULLSCREEN_NOW;
		}
	}


	ClipperMap::iterator cl;

	while( (cl = clippers.begin()) != clippers.end() ) {
		RELEASE( cl->second.clipper );
		clippers.erase( cl );
	}

	memset( batch_state, 0, sizeof(batch_state) );	

	//

	buffers_video_stream = NULL;

	texture_format_classes.clear();
	texture_formats.clear();

	if( d3drp_f_flags & D3DRP_F_BUFFERS_CREATED ) {
		RELEASE( direct3d_device );
//		RELEASE( direct3d_depth_buffer );
//		RELEASE( directdraw_color_buffers[1] );
//		RELEASE( directdraw_color_buffers[0] );

	}
	

	curr_pipeline_state[RP_BUFFERS_WIDTH].set( 0 );
	curr_pipeline_state[RP_BUFFERS_HEIGHT].set( 0 );
	*/
	//d3drp_f_flags &= ~(D3DRP_F_BUFFERS_CREATED);
	
}

//

GENRESULT COMAPI Direct3D_RenderPipeline::create_vertex_buffer( U32 vertex_format, int num_verts, U32 irp_vbf_flags, IRP_VERTEXBUFFERHANDLE *out_vb_handle ) 
{
	CHECK_CREATE_BUFFERS(create_vertex_buffer);
	ASSERT( direct3d_device != NULL );
	U32 stride = FVF_SIZEOF_VERT(vertex_format);
	if( FAILED( direct3d_device-> CreateVertexBuffer(stride * num_verts,0,vertex_format,D3DPOOL_MANAGED,(IDirect3DVertexBuffer9**)out_vb_handle,NULL))) {
		return GR_GENERIC;
	}
	return GR_OK;
}

//

GENRESULT COMAPI Direct3D_RenderPipeline::destroy_vertex_buffer( IRP_VERTEXBUFFERHANDLE vb_handle ) 
{
	CHECK_CREATE_BUFFERS(destroy_vertex_buffer);

	if( vb_handle == IRP_INVALID_VB_HANDLE ) {
		return GR_GENERIC;
	}
	
	((IDirect3DVertexBuffer9*)vb_handle)->Release();

	return GR_OK;
}

//

GENRESULT COMAPI Direct3D_RenderPipeline::lock_vertex_buffer( IRP_VERTEXBUFFERHANDLE vb_handle, U32 ddlock_flags, void **out_vertex_buffer, U32 *out_vertex_buffer_size ) 
{
	CHECK_CREATE_BUFFERS(lock_vertex_buffer);
	if( vb_handle == IRP_INVALID_VB_HANDLE ) {
		return GR_GENERIC;
	}
	D3DVERTEXBUFFER_DESC desc;
	if( FAILED( ((IDirect3DVertexBuffer9*)vb_handle)->GetDesc(&desc))) 
	{
		return GR_GENERIC;
	}
	*out_vertex_buffer_size = desc.Size/FVF_SIZEOF_VERT( desc.FVF );

	if( FAILED( ((IDirect3DVertexBuffer9*)vb_handle)->Lock(0,0,out_vertex_buffer,0) ) ) 
	{
		return GR_GENERIC;
	}
	return GR_OK;
}

//

GENRESULT COMAPI Direct3D_RenderPipeline::unlock_vertex_buffer( IRP_VERTEXBUFFERHANDLE vb_handle ) 
{
	CHECK_CREATE_BUFFERS(unlock_vertex_buffer);

	if( vb_handle == IRP_INVALID_VB_HANDLE ) {
		return GR_GENERIC;
	}
	
	if( FAILED( ((IDirect3DVertexBuffer9*)vb_handle)->Unlock() ) ) {
		return GR_GENERIC;
	}

	return GR_OK;
}

//

GENRESULT COMAPI Direct3D_RenderPipeline::optimize_vertex_buffer( IRP_VERTEXBUFFERHANDLE vb_handle ) 
{
	/*CHECK_CREATE_BUFFERS(optimize_vertex_buffer);
	ASSERT( direct3d_device );

	if( vb_handle == IRP_INVALID_VB_HANDLE ) {
		return GR_GENERIC;
	}

	if( FAILED( ((IDirect3DVertexBuffer9*)vb_handle)->Optimize( direct3d_device, 0 ) ) ) {
		return GR_GENERIC;
	}
*/
	return GR_OK;
}

//

GENRESULT COMAPI Direct3D_RenderPipeline::process_vertex_buffer( IRP_VERTEXBUFFERHANDLE dst_vb_handle, IRP_VERTEXBUFFERHANDLE src_vb_handle, int first_vertex_index, int num_verts, U32 irp_pvbf_flags ) 
{
	CHECK_CREATE_BUFFERS(get_processed_vertex_buffer);

	if( src_vb_handle == IRP_INVALID_VB_HANDLE ) {
		return GR_GENERIC;
	}
	if( dst_vb_handle == IRP_INVALID_VB_HANDLE ) {
		return GR_GENERIC;
	}

	return GR_GENERIC;
}

//

#if RP_RD_DEBUG

// IRenderDebugger
void Direct3D_RenderPipeline::set_trace_enable( const IRD_STATETYPE state_traces) 
{
	rp_rd_set_trace_enable( state_traces );
}

//

void Direct3D_RenderPipeline::set_trace_output_dir( const char *directory )
{
	rp_rd_set_trace_output_dir( directory );
}

//

HRESULT WINAPI save_texture_cb( LPDIRECTDRAWSURFACE7 lpDDSurface, LPDDSURFACEDESC2 lpDDSurfaceDesc, LPVOID lpContext )
{
	char *dir = (char*)lpContext;
	char filename[ MAX_PATH ];

	if(  (lpDDSurfaceDesc->ddsCaps.dwCaps & (DDSCAPS_TEXTURE|DDSCAPS_MIPMAP|DDSCAPS_SYSTEMMEMORY)) &&
		!(lpDDSurfaceDesc->ddsCaps.dwCaps2 & DDSCAPS2_MIPMAPSUBLEVEL) ) {

		sprintf( filename, "%s\\tex%08X.dds", dir, lpDDSurface );
		rp_rd_save_surface_dds( filename, lpDDSurface );
		rp_rd_add_trace_message( "-- saved texture %s", filename );
	}

	return DDENUMRET_OK;
}

//

void Direct3D_RenderPipeline::dump_current_state( const IRD_STATETYPE state_traces, const char *directory )
{
	/*
	ASSERT( direct3d_device );

	U32 saved_mask;
	U32 s=0;
	D3DMATRIX M;

	saved_mask = rp_rd_set_trace_enable( state_traces );

	if( state_traces & IRD_ST_RENDER_STATE ) {
		for( s=1; s<D3DRS_MAX_STATE; s++ ) {
			RenderStateArray::iterator rs;
			if( (rs = render_state_info.find( s )) != render_state_info.end() && (rs->second.enum_value == s) ) {
				rp_rd_render_state( (D3DRENDERSTATETYPE)s, curr_hw_render_state[s].get( direct3d_device, s ) );
			}
		}
	}

	if( state_traces & IRD_ST_TEXTURE ) {
		for( s=0; s<D3DTSS_NUM_STAGES; s++ ) {
			rp_rd_texture( s, (LPDIRECTDRAWSURFACE7)curr_hw_texture[s].get( direct3d_device, s ) );
		}
	}

	if( state_traces & IRD_ST_TEXTURE_TRANSFORM ) {
		for( s=0; s<D3DTSS_NUM_STAGES; s++ ) {
			curr_hw_texture_transform[s].get( direct3d_device, (D3DTRANSFORMSTATETYPE)(D3DTS_TEXTURE0 + s), &M );
			rp_rd_transform( (D3DTRANSFORMSTATETYPE)(D3DTS_TEXTURE0 + s), &M );
		}
	}

	if( state_traces & IRD_ST_TEXTURE_STATE ) {
		for( s=0; s<D3DTSS_NUM_STAGES; s++ ) {
			for( U32 ss=1; ss<D3DTSS_MAX_STATE; ss++ ) {
				rp_rd_texture_state( s, (D3DTEXTURESTAGESTATETYPE)ss, (D3DTEXTURESTAGESTATETYPE)curr_hw_texture_state[s][ss].get( direct3d_device, (D3DTEXTURESTAGESTATETYPE)s, ss ) );
			}
		}
	}

	// transform

	if( state_traces & IRD_ST_TRANSFORM ) {
		direct3d_device->GetTransform( D3DTS_VIEW, &M );
		rp_rd_transform( D3DTS_VIEW, &M );

		curr_hw_world.get( direct3d_device, D3DTS_WORLD, &M );
		rp_rd_transform( D3DTS_WORLD, &M );

		curr_hw_projection.get( direct3d_device, D3DTS_PROJECTION, &M );	
		rp_rd_transform( D3DTS_PROJECTION, &M );

		D3DVIEWPORT9 vp;

		curr_hw_viewport.get( direct3d_device, &vp );
		rp_rd_viewport( &vp );
	}

	if( state_traces & IRD_ST_LIGHT ) {
		LightInfoMap::iterator beg = lights.begin();
		LightInfoMap::iterator end = lights.end();
		LightInfoMap::iterator lit;

		// lights

		D3DLIGHT9 light;
		int light_enabled;

		for( lit=beg; lit!=end; lit++ ) {

			if( SUCCEEDED( direct3d_device->GetLight( lit->second, &light) ) ) { 
				rp_rd_light( lit->second, &light );
			}
			
			if( SUCCEEDED( direct3d_device->GetLightEnable( lit->second, &light_enabled ) ) ) { 
				rp_rd_light_enable( lit->second, light_enabled? 1 : 0 );
			}
		}
	}

	// textures
	if( state_traces & IRD_ST_TEXTURE_DATA ) {
		directdraw->EnumSurfaces( DDENUMSURFACES_DOESEXIST|DDENUMSURFACES_ALL, NULL, (void*)directory, save_texture_cb );
	}

	rp_rd_set_trace_enable( saved_mask );
	*/
}

//

void Direct3D_RenderPipeline::add_trace_message( const char *fmt, ... ) 
{
	static char buffer[4096];

	va_list args;

	va_start( args, fmt );
	vsprintf( buffer, fmt, args );
	va_end( args );

	rp_rd_add_trace_message( buffer );
}

//

void Direct3D_RenderPipeline::begin_trace_section( const char *fmt, ... ) 
{
	static char buffer[4096];

	va_list args;

	va_start( args, fmt );
	vsprintf( buffer, fmt, args );
	va_end( args );


	rp_rd_begin_trace_section( buffer );
}

//

void Direct3D_RenderPipeline::end_trace_section( void ) 
{
	rp_rd_end_trace_section();
}

//

void Direct3D_RenderPipeline::save_trace_capture( const char *filename ) 
{
	rp_rd_save_trace_capture( filename );
}

//

void Direct3D_RenderPipeline::clear_trace_capture( void ) 
{
	rp_rd_clear_trace_capture( );
}

//

void Direct3D_RenderPipeline::set_frustum_view_enable( bool onoff ) 
{
	rp_rd_set_frustum_view_enable( onoff );
}

//

void Direct3D_RenderPipeline::set_frustum_view_options( void ) 
{
	rp_rd_set_frustum_view_options();
}

//

void Direct3D_RenderPipeline::set_sb_delay( float milliseconds ) 
{
	rp_rd_set_sb_delay( milliseconds );
}

//

void Direct3D_RenderPipeline::set_dp_delay( float milliseconds ) 
{
	rp_rd_set_dp_delay( milliseconds );
}

//

void Direct3D_RenderPipeline::save_texture( const char *filename, U32 irp_texture_handle ) 
{
	//if( SUCCEEDED( is_texture( irp_texture_handle ) ) ) {
		//rp_rd_save_surface_dds( filename, (LPDIRECTDRAWSURFACE7)irp_texture_handle );
	//}
}

//

void Direct3D_RenderPipeline::save_screen_capture( const char *filename, bool delay_to_sb ) 
{
	//rp_rd_save_screen_capture( filename, directdraw_color_buffers[1], delay_to_sb );
}

//

void Direct3D_RenderPipeline::set_ref_enable( bool onoff ) 
{

}

//

#endif // RP_RD_DEBUG


//

/*struct D3DENUMDATA {
	bool ok;
	GUID device_type;
	D3DDEVICEDESC9 d3dcaps;
};*/

/*HRESULT CALLBACK D3DRP_FindDeviceCaps( LPSTR, LPSTR, LPD3DDEVICEDESC7 lpD3DHWDeviceDesc, LPVOID lpContext )
{
	D3DENUMDATA *data = (D3DENUMDATA*)lpContext;

	if( memcmp( &data->device_type, &lpD3DHWDeviceDesc->deviceGUID, sizeof(GUID) ) == 0 ) {
		memcpy( &data->d3dcaps, lpD3DHWDeviceDesc, D3DDEVICEDESC7SIZE );
		data->ok = true;
		return D3DENUMRET_CANCEL;
	}

	return D3DENUMRET_OK;
}
*/

//

inline U32 d3d_caps_check_zero( U32 value, U32 def )
{
	return value ? value : def;
}

//

void Direct3D_RenderPipeline::internal_set_abilities( void )
{
//	ASSERT( directdraw );

	memset( device_abilities, 0, sizeof(device_abilities) );

	// 2D CAPS
	//
	DDCAPS ddhwcaps, ddhelcaps;
	memset( &ddhwcaps, 0, sizeof(ddhwcaps) );
	memset( &ddhelcaps, 0, sizeof(ddhelcaps) );
	ddhwcaps.dwSize = ddhelcaps.dwSize = sizeof(DDCAPS);
	
	DDCAPS *dd_caps = &ddhwcaps;

	/*
	if( SUCCEEDED( directdraw->GetCaps( &ddhwcaps, &ddhelcaps ) ) ) {

		rp_dd_get_memory( directdraw, DDSCAPS_VIDEOMEMORY, &device_abilities[RP_A_DEVICE_MEMORY], NULL );

		device_abilities[RP_A_DEVICE_2D_ONLY] = 0;
//		if( !(dd_caps->dwCaps & DDCAPS_3D) && !(current_device_type->d3drp_dtf_flags & D3DRP_DTF_SOFTWARE) ) {
//			device_abilities[RP_A_DEVICE_2D_ONLY] = 1;
//		}

		device_abilities[RP_A_DEVICE_GAMMA] = 0;
		if( dd_caps->dwCaps2 & DDCAPS2_PRIMARYGAMMA ) {
			device_abilities[RP_A_DEVICE_GAMMA] = 1;
		}

		if( dd_caps->dwCaps2 & DDCAPS2_CANCALIBRATEGAMMA ) {
			rprd_device_flags |= RPRD_F_GAMMA_CALIBRATION;
		}
		else {
			rprd_device_flags &= ~(RPRD_F_GAMMA_CALIBRATION);
		}

		device_abilities[RP_A_DEVICE_WINDOWED] = 0;
		if( dd_caps->dwCaps2 & DDCAPS2_CANRENDERWINDOWED ) {
			device_abilities[RP_A_DEVICE_WINDOWED] = 1;
		}
	}
*/
	// 3D CAPS

	if( device_abilities[RP_A_DEVICE_2D_ONLY] ) {
		
		device_abilities[RP_A_DEVICE_3D_ONLY]			= 0;
		device_abilities[RP_A_DEVICE_SOFTWARE]			= 0;
		device_abilities[RP_A_DEVICE_GEOMETRY]			= 0;
														
		device_abilities[RP_A_BUFFERS_DEPTH_LINEAR]		= 0;
														
		device_abilities[RP_A_TEXTURE_MAX_WIDTH]		= 0;
		device_abilities[RP_A_TEXTURE_MAX_HEIGHT]		= 0;
		device_abilities[RP_A_TEXTURE_STAGES]			= 0;
		device_abilities[RP_A_TEXTURE_NONLOCAL]			= 0;
		device_abilities[RP_A_TEXTURE_NUMA]				= 0;
		device_abilities[RP_A_TEXTURE_SQUARE_ONLY]		= 0;
		device_abilities[RP_A_TEXTURE_BILINEAR]			= 0;
		device_abilities[RP_A_TEXTURE_TRILINEAR]		= 0;
		device_abilities[RP_A_TEXTURE_LOD]				= 0;
		device_abilities[RP_A_TEXTURE_SIMULTANEOUS]		= 0;
		device_abilities[RP_A_TEXTURE_COORDINATES]		= 0;
														
		device_abilities[RP_A_ALPHA_ITERATED]			= 0;
		device_abilities[RP_A_ALPHA_TEST]				= 0;
		device_abilities[RP_A_ALPHA_TEST_ALL]			= 0;
														
		device_abilities[RP_A_BLEND_TRANSPARENCY_SRC]	= 0;
		device_abilities[RP_A_BLEND_TRANSPARENCY_DST]	= 0;
		device_abilities[RP_A_BLEND_ADD_SRC]			= 0;
		device_abilities[RP_A_BLEND_ADD_DST]			= 0;
		device_abilities[RP_A_BLEND_MUL_SRC]			= 0;
		device_abilities[RP_A_BLEND_MUL_DST]			= 0;

		device_abilities[RP_A_DEPTH_BIAS]				= 0;

		device_abilities[RP_A_TEXTURE_OPS]				= 0;
		device_abilities[RP_A_TEXTURE_ARG_FLAGS]		= 0;

		device_abilities[RP_A_DEVICE_FULLSCENE_ANTIALIAS]= 0;
		
	}
	else {

		D3DCAPS9 data;

		direct3d_device->GetDeviceCaps(&data);
		
		if( true ) {
			if (data.PixelShaderVersion >= D3DPS_VERSION(2,0))
				PixelShaderVersion = 2;
			else if (data.PixelShaderVersion >= D3DPS_VERSION(1,1))
				PixelShaderVersion = 1;
			else PixelShaderVersion = 0;

			bool vShader = data.VertexShaderVersion >= D3DVS_VERSION(1,1);

//			RPDEVICEID id;
//			rp_dd_get_device_id( directdraw, id );

			device_abilities[RP_A_TEXTURE_MAX_WIDTH]	= d3d_caps_check_zero( data.MaxTextureWidth, 256 );
			device_abilities[RP_A_TEXTURE_MAX_HEIGHT]	= d3d_caps_check_zero( data.MaxTextureHeight, 256 );
			device_abilities[RP_A_TEXTURE_COORDINATES]	= d3d_caps_check_zero( ((WORD)(data.FVFCaps & D3DFVFCAPS_TEXCOORDCOUNTMASK)), 1 );

			bool dx5 = (data.DevCaps & D3DDEVCAPS_DRAWPRIMTLVERTEX);
			bool dx6 = (data.DevCaps & D3DDEVCAPS_DRAWPRIMITIVES2);
			bool dx7 = (data.DevCaps & D3DDEVCAPS_DRAWPRIMITIVES2EX);
		
			if( dx5 && !(dx6 || dx7) ) {
				rprd_device_flags |= RPRD_F_BROKEN_MULTITEXTURE;
				device_abilities[RP_A_TEXTURE_STAGES]		= 1;
				device_abilities[RP_A_TEXTURE_SIMULTANEOUS] = 1;
			}
			else {
				rprd_device_flags &= ~(RPRD_F_BROKEN_MULTITEXTURE);
				device_abilities[RP_A_TEXTURE_STAGES]		= d3d_caps_check_zero( data.MaxTextureBlendStages, 1 );
				device_abilities[RP_A_TEXTURE_SIMULTANEOUS] = d3d_caps_check_zero( data.MaxSimultaneousTextures, 1 );
			}
		
			device_abilities[RP_A_ALPHA_ITERATED] = 1;
/*			if( id == RP_D_RIVA128 ||
				id == RP_D_PERMEDIA_2 || 
				!(data.ShadeCaps & D3DPSHADECAPS_ALPHAGOURAUDBLEND) ) {
				
				device_abilities[RP_A_ALPHA_ITERATED] = 0;
			}*/

			device_abilities[RP_A_DEVICE_3D_ONLY] = 0;
			if( !(dd_caps->dwCaps & DDCAPS_GDI) ) {
				device_abilities[RP_A_DEVICE_3D_ONLY] = 1;
			}

			device_abilities[RP_A_DEVICE_SOFTWARE] = 0;
//			if( current_device_type->d3drp_dtf_flags & D3DRP_DTF_SOFTWARE ) {
//				device_abilities[RP_A_DEVICE_SOFTWARE] = 1;
//			}

			device_abilities[RP_A_TEXTURE_NONLOCAL] = 0;
			if ( data.DevCaps & D3DDEVCAPS_TEXTURENONLOCALVIDMEM ) {
				device_abilities[RP_A_TEXTURE_NONLOCAL] = 1;
			}

			device_abilities[RP_A_TEXTURE_NUMA] = 0;
			if ( data.DevCaps & D3DDEVCAPS_SEPARATETEXTUREMEMORIES ) {
				device_abilities[RP_A_TEXTURE_NUMA] = 1;
			}

			device_abilities[RP_A_TEXTURE_SQUARE_ONLY] = 0;
			if ( data.TextureCaps & D3DPTEXTURECAPS_SQUAREONLY ) {
				device_abilities[RP_A_TEXTURE_SQUARE_ONLY] = 1;
			}

			device_abilities[RP_A_TEXTURE_BILINEAR] = 0;
			if ( data.TextureFilterCaps & D3DPTFILTERCAPS_MIPFLINEAR ) {
				device_abilities[RP_A_TEXTURE_BILINEAR] = 1;
			}

			device_abilities[RP_A_TEXTURE_TRILINEAR] = 0;
//			if ( data.TextureFilterCaps & D3DPTFILTERCAPS_LINEARMIPLINEAR ) {
//				device_abilities[RP_A_TEXTURE_TRILINEAR] = 1;
//			}

			device_abilities[RP_A_DEVICE_GEOMETRY] = 0;
			if( data.DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT ) {
				device_abilities[RP_A_DEVICE_GEOMETRY] = 1;
			}

			U32 all =	D3DPCMPCAPS_EQUAL			|
						D3DPCMPCAPS_GREATER			|
						D3DPCMPCAPS_GREATEREQUAL	|
						D3DPCMPCAPS_LESS			|
						D3DPCMPCAPS_LESSEQUAL		|
						D3DPCMPCAPS_NEVER			|
						D3DPCMPCAPS_NOTEQUAL		|
						0;

			U32 some =	D3DPCMPCAPS_EQUAL			|
						D3DPCMPCAPS_NOTEQUAL;

			device_abilities[RP_A_ALPHA_TEST] = 0;
			device_abilities[RP_A_ALPHA_TEST_ALL] = 0;
			if( (data.AlphaCmpCaps & (all)) == all ) {
				device_abilities[RP_A_ALPHA_TEST_ALL] = 1;
				device_abilities[RP_A_ALPHA_TEST] = 1;
			}
			else if( (data.AlphaCmpCaps & (some)) == some ) {
				device_abilities[RP_A_ALPHA_TEST_ALL] = 0;
				device_abilities[RP_A_ALPHA_TEST] = 1;
			}

			U32 mip_mask =	
							D3DPTFILTERCAPS_MIPFPOINT |
							D3DPTFILTERCAPS_MIPFLINEAR	|
							0;

			device_abilities[RP_A_TEXTURE_LOD] = 0;
			if( data.TextureFilterCaps & mip_mask ) {
				device_abilities[RP_A_TEXTURE_LOD] = 1;
			}

			device_abilities[RP_A_BUFFERS_DEPTH_LINEAR] = 0;
			if( data.RasterCaps & D3DPRASTERCAPS_WBUFFER ) {
				device_abilities[RP_A_BUFFERS_DEPTH_LINEAR] = 1;
			}

//TODO - fix this for dx9			rp_d3d_get_blend_matrix( directdraw, &data, direct3d_blend_matrix );

			// determine MUL blend mode
			//
/*			if( rp_a_is_blend_supported( D3DBLEND_ZERO, D3DBLEND_SRCCOLOR, direct3d_blend_matrix ) ) {
				device_abilities[RP_A_BLEND_MUL_SRC] = D3DBLEND_ZERO;
				device_abilities[RP_A_BLEND_MUL_DST] = D3DBLEND_SRCCOLOR;
			}
			else if( rp_a_is_blend_supported( D3DBLEND_DESTCOLOR, D3DBLEND_ZERO, direct3d_blend_matrix ) ) {
				device_abilities[RP_A_BLEND_MUL_SRC] = D3DBLEND_DESTCOLOR;
				device_abilities[RP_A_BLEND_MUL_DST] = D3DBLEND_ZERO;
			}
			else if( rp_a_is_blend_supported( D3DBLEND_ONE, D3DBLEND_ONE, direct3d_blend_matrix ) ) {
				device_abilities[RP_A_BLEND_MUL_SRC] = D3DBLEND_ONE;
				device_abilities[RP_A_BLEND_MUL_DST] = D3DBLEND_ONE;
			}
			else {
				GENERAL_WARNING( "Unable to determine MUL blend mode.\n" );
				device_abilities[RP_A_BLEND_MUL_SRC] = 0;
				device_abilities[RP_A_BLEND_MUL_DST] = 0;
			}

			// determine ADD blend mode
			//
			if( rp_a_is_blend_supported( D3DBLEND_ONE, D3DBLEND_ONE, direct3d_blend_matrix ) ) {
				device_abilities[RP_A_BLEND_ADD_SRC] = D3DBLEND_ONE;
				device_abilities[RP_A_BLEND_ADD_DST] = D3DBLEND_ONE;
			}
			else {
				GENERAL_WARNING( "Unable to determine ADD blend mode.\n" );
				device_abilities[RP_A_BLEND_ADD_SRC] = 0;
				device_abilities[RP_A_BLEND_ADD_DST] = 0;
			}

			// determine TRANSPARENCY blend mode
			//
			if( rp_a_is_blend_supported( D3DBLEND_SRCALPHA, D3DBLEND_INVSRCALPHA, direct3d_blend_matrix ) ) {
				device_abilities[RP_A_BLEND_TRANSPARENCY_SRC] = D3DBLEND_SRCALPHA;
				device_abilities[RP_A_BLEND_TRANSPARENCY_DST] = D3DBLEND_INVSRCALPHA;
			}
			else if( rp_a_is_blend_supported( D3DBLEND_SRCALPHA, D3DBLEND_ONE, direct3d_blend_matrix ) ) {		
				device_abilities[RP_A_BLEND_TRANSPARENCY_SRC] = D3DBLEND_SRCALPHA;
				device_abilities[RP_A_BLEND_TRANSPARENCY_DST] = D3DBLEND_ONE;
			}
			else if( rp_a_is_blend_supported( D3DBLEND_ONE, D3DBLEND_INVSRCALPHA, direct3d_blend_matrix ) ) {
				device_abilities[RP_A_BLEND_TRANSPARENCY_SRC] = D3DBLEND_ONE;
				device_abilities[RP_A_BLEND_TRANSPARENCY_DST] = D3DBLEND_INVSRCALPHA;
			}
			else {
				GENERAL_WARNING( "Unable to determine TRANSPARENCY blend mode.\n" );
				device_abilities[RP_A_BLEND_TRANSPARENCY_SRC] = 0;
				device_abilities[RP_A_BLEND_TRANSPARENCY_DST] = 0;
			}
*/
			device_abilities[RP_A_TEXTURE_OPS] = data.TextureOpCaps ;
/*			switch( id ) {
			case RP_D_VOODOO_1:
			case RP_D_VOODOO_2:
			case RP_D_VOODOO_3:			// 3dfx drivers seem to think they can support 2/4x
			case RP_D_VOODOO_RUSH:
			case RP_D_VOODOO_BANSHEE:
				device_abilities[RP_A_TEXTURE_OPS] &= ~(D3DTEXOPCAPS_MODULATE2X|D3DTEXOPCAPS_MODULATE4X);
				break;				
			}*/

			device_abilities[RP_A_TEXTURE_ARG_FLAGS] = 0xFFFFFFFF;
			/*
			switch( id ) {
			case RP_D_SAVAGE_4:
				device_abilities[RP_A_TEXTURE_ARG_FLAGS] &= ~(RP_TA_COMPLEMENT);
				break;
			}*/

			device_abilities[RP_A_DEPTH_BIAS] = (data.RasterCaps & D3DPRASTERCAPS_DEPTHBIAS)? 1 : 0;

			//device_abilities[RP_A_DEVICE_FULLSCENE_ANTIALIAS] = (data.RasterCaps & D3DPRASTERCAPS_ANTIALIASSORTINDEPENDENT)? 1 : 0;

		}
		else {
			GENERAL_TRACE_1( "Device does not have Direct3D Device available\n" );
		}

	}

	// read device abilities overrides from the ini file.
	//
	U32 _enum, _value;
	AbilitiesArray::iterator beg = abilities_info.begin();
	AbilitiesArray::iterator end = abilities_info.end();
	AbilitiesArray::iterator ai;

	for( ai=beg; ai!=end; ai++ ) {
		if( ai->second.get_enum_and_default( &_enum, &_value ) ) {
			opt_get_u32( DACOM_Acquire(), profile_parser, current_device_profile, ai->second.key_name, device_abilities[_enum], &device_abilities[_enum] );
		}
	}
}

// ------------------------------------------------------------------
// DLL Related code
// ------------------------------------------------------------------

//

void main( void ){} // linker bug

//


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
				GENERAL_TRACE_1( "Direct3D_RenderPipeline: DllMain: unable to get DACOM!\n" );
				break;
			}

			if( (server1 = new DAComponentFactory2<DAComponentAggregate<Direct3D_RenderPipeline>, AGGDESC>( CLSID_D3D_RenderPipeline )) != NULL ) {
				DACOM->RegisterComponent( server1, CLSID_D3D_RenderPipeline, DACOM_LOW_PRIORITY );
				server1->Release();
			}

			if( (server1 = new DAComponentFactory2<DAComponentAggregate<Direct3D_RenderPipeline>, AGGDESC>( CLSID_IRenderPipeline )) != NULL ) {
				DACOM->RegisterComponent( server1, CLSID_IRenderPipeline, DACOM_LOW_PRIORITY );
				server1->Release();
			}

			if( (server1 = new DAComponentFactory<DAComponent<Direct3D_RenderPipeline>, RPUL_DACOMDESC>( "IDAComponent" )) != NULL ) {
				DACOM->RegisterComponent( server1, "IDAComponent", DACOM_LOW_PRIORITY );
				server1->Release();
			}
			break;
	}

	return TRUE;
}


// EOF
