//
// RendPipeline.h
//
//
//
//


#ifndef RENDPIPELINE_H
#define RENDPIPELINE_H

//

#include "dacom.h"
#include "pixel.h"
#include "3dmath.h"
#include "matrix4.h"
#include "da_d3dtypes.h"
#include "IProfileParser.h"

#include "RPVertex1.h"

//

#pragma warning( disable: 4201 )	// unnamed struct is a non-standard extension

//

typedef enum
{
	RP_CLEAR_COLOR_BIT		= 0x01,		// Use with clear_buffers to enable clearing of color buffers.
	RP_CLEAR_DEPTH_BIT		= 0x02,		// Use with clear_buffers to enable clearing of depth buffers.
	RP_CLEAR_STENCIL_BIT	= 0x04,		// Use with clear_buffers to enable clearing of stencil buffers.

	RP_CURRENT				= 0xFEEDBABE,

	// LEAVE ME LAST
	RP_MAX_ENUM
} RPenum;

//

typedef enum {
	
	RP_BUFFERS_COLOR_BPP=0		,	// Bits per pixel of the color buffers (i.e. 16, 24, 32)
	RP_BUFFERS_DEPTH_BPP		,	// Bits per pixel of the depth buffer *not* including stencil (i.e. 0, 16, 24, 32)
	RP_BUFFERS_STENCIL_BPP		,	// Bits per pixel of the stencil buffer (i.e. 0,1,8)
	RP_BUFFERS_COUNT			,	// Number of buffers (i.e. 1,2,3)
	RP_BUFFERS_HWFLIP			,	// Enable/disable hardware flipping.
	RP_BUFFERS_FULLSCREEN		,	// Enable/disable fullscreen buffers.
	RP_BUFFERS_VSYNC			,	// Enable/disable vertical trace syncing.
	RP_BUFFERS_OFFSCREEN		,	// Enable/disable offscreen buffers.
	RP_BUFFERS_HWMEM			,	// Enable/disable use of video memory for surfaces
	RP_BUFFERS_WIDTH			,	// Current width of the buffers. (ReadOnly)
	RP_BUFFERS_HEIGHT			,	// Current height of the buffers. (ReadOnly)
	RP_BUFFERS_DEPTH_AUTOW		,	// Enable/disable use of linear depth buffer if available
	RP_BUFFERS_SWAP_STALL		,	// Enable/disable automatic pipeline stall during swap_buffers.  This fixes
									// problems on some (bad) drivers that (erroneously) queue up flips and blts.

	RP_TEXTURE					,	// Enable/disable texturing.
	RP_TEXTURE_LOD				,	// Enable/disable creation of texture level of detail.
	RP_TEXTURE_ALLOW_8BIT		,	// Enable/disable palettized texture formats.
	RP_TEXTURE_ALLOW_DXT		,	// Enable/disable compressed texture formats.

	RP_CLEAR_COLOR				,	// Packed 32-bit BGRA (0xARGB) color to use when clearing the viewport.
	RP_CLEAR_DEPTH				,	// Unsigned 32-bit value representing the depth to use when clearing the viewport.
									// This value is divided by 0xFFFFFFFF to produce a value in the range [0.0,1,0]
	RP_CLEAR_STENCIL			,	// Unsigned 32-bit value representing the stencil mask to use when clearing the viewport.

	RP_DEVICE_FPU_SETUP			,	// Control D3D FPU munging.  0 == _FPUPRESERVE, 1 == _FPUSETUP.  Default is 0.
	RP_DEVICE_MULTITHREADED		,	// Enable/disable multithreaded access.

	RP_DEBUG_PROFILE_LOG		,	// Enable/disable profile logging to FDUMP.
	RP_DEBUG_FILL_COLOR			,	// Packed 32-bit BGRA (0xARGB) color to use to clear new texture surfaces.
	RP_DEVICE_VIDEO_STREAM_MODE	,	// Use DShow (default), or Video For Windows

	RP_BUFFERS_ANTIALIAS		,	// Enable/disable the buffers to support full-scene antialiasing if possible.  
									// NOTE: this does *NOT* enable full-scene antialiasing, it tells RP to create
									// buffers that are compatible with FSAA.  Use the _ANTIALIAS renderstate to
									// control actual use of FSAA.
	// LEAVE ME LAST
	RP_MAX_PIPELINE_STATE		
} RPPIPELINESTATE;

//

struct RGB
{
	U8 r,g,b;
};

// 
// RPLOCKDATA
//
// This structure is used when locking textures or the render 
// buffers via lock_buffers or lock_texture.
//
struct RPLOCKDATA
{
	U32			width;		// Width of the surface in pixels (texels).
	U32			height;		// Height of the surface in pixels (texels).
	U32			pitch;		// Width of the surface in bytes.
	void *		pixels;		// Pointer to pixel (texel) data.
	PixelFormat	pf;			// Format of the data referenced by pixels.
};

//
// RPDISPLAYMODEINFO
//
// This structure is used to retrieve information about a
// particular display mode via get_display_mode.
//
struct RPDISPLAYMODEINFO 
{
	U32			mode_num;	// An index in the range [0,num_modes) where num_modes is 
							// the value returned by get_num_display_modes.
	U32			width;		// Width of the mode in pixels.
	U32			height;		// Height of the mode in pixels.
	PixelFormat render_pf;	// Format of the color buffers in this display mode.
};

//

enum RPDEVICEID
{
	RP_D_UNKNOWN,
	RP_D_GENERIC,
	RP_D_RIVA128,
	RP_D_RIVATNT,
	RP_D_RIVATNT2,
	RP_D_VOODOO_1,
	RP_D_VOODOO_2,
	RP_D_VOODOO_3,
	RP_D_VOODOO_RUSH,
	RP_D_VOODOO_BANSHEE,
	RP_D_PERMEDIA_2,
	RP_D_SAVAGE_4,
	RP_D_MYSTIQUE,
	RP_D_MILLENIUM,
	RP_D_MILLENIUM_2
};

//
// RPDEVICEINFO
//
// This structure is used to retrieve information about the device
// via get_device_info.
//
struct RPDEVICEINFO
{
	char		device_class[64];			// The class of the device (OpenGL, Direct3D, etc)
	char		device_id_persist[64];		// The full id to use in a .ini to get the current device.
	char		device_type[64];			// The device type (Hardware, Software, Reference, etc...)
	char		device_description[128];	// A short description of the device.
	RPDEVICEID	device_chipset_id;			// Useful for some things, but use sparingly (PLEASE!)
};

//
// RP_TA_ masks
//
#define RP_TA_TFACTOR			(1<<0)	// supports D3DTA_TFACTOR
#define RP_TA_ALPHAREPLICATE	(1<<1)	// supports D3DTA_ALPHAREPLICATE
#define RP_TA_COMPLEMENT		(1<<2)	// supports D3DTA_COMPLEMENT


//
// RPDEVICESTATS
//
// This structure is used to retrieve profile statistics from the
// current rendering device via get_device_stats.
//
struct RPDEVICESTATS
{
	// note for this structure that "last frame" means the
	// time between the last begin_scene() and end_scene().

	U32	is_thrashing;					// non-zero if last frame caused thrashing.
	U32	num_texture_managed;			// number of textures currently being managed by the system.
	U32	num_texture_activated;			// number of textures that were activated (used) last frame.
	U32	num_texture_vidmem_activated;	// number of textures that were activated (used) last frame that were in video memory.
	U32	num_texture_vidmem_created;		// number of textures that were created in video memory last frame.
	U32	num_texture_vidmem_evicted;		// number of textures that were removed from video memory last frame.
	U32	num_texture_vidmem;				// number of textures currently in video memory.
	U32	sizeof_texture_vidmem;			// number of video memory bytes used by current textures.
	U32	sizeof_texture_sysmem;			// number of system memory bytes used by current textures.
	
	U32	num_dp_calls;					// number of draw_primitive calls
	U32	num_dp_primitives;				// number of primitives submitted (tris+points+lines)
	U32	num_dip_calls;					// number of draw_indexed_primitive calls
	U32	num_dip_primitives;				// number of primitives submitted (tris+points+lines)
};

//
// RPDEVICEABILITY
//
//
//
typedef enum {

	RP_A_DEVICE_2D_ONLY		= 1	,		// device has no 3d capability
	RP_A_DEVICE_3D_ONLY			,		// device has no 2d capability
	RP_A_DEVICE_GAMMA			,		// device supports gamma control
	RP_A_DEVICE_WINDOWED		,		// device supports windowed rendering
	RP_A_DEVICE_MEMORY			,		// device has xxx bytes of videomemory
	RP_A_DEVICE_SOFTWARE		,		// device is a software renderer
	RP_A_DEVICE_GEOMETRY		,		// device supports hardware transformation
		
	RP_A_TEXTURE_NONLOCAL		,		// device supports AGP texture memory
	RP_A_TEXTURE_SQUARE_ONLY	,		// device supports only square textures
	RP_A_TEXTURE_STAGES			,		// number of texture blend stages supported
	RP_A_TEXTURE_MAX_WIDTH		,		// max texture width supported
	RP_A_TEXTURE_MAX_HEIGHT		,		// max texture height supported
	RP_A_TEXTURE_NUMA			,		// device is non-uniform memory access (separate TMUs)
	RP_A_TEXTURE_BILINEAR		,		// device supports bilinear texture filtering
	RP_A_TEXTURE_TRILINEAR		,		// device supports trilinear mipmap filtering
	RP_A_TEXTURE_LOD			,		// device supports mipmaps.
	RP_A_TEXTURE_SIMULTANEOUS	,		// Number of simultaneous textures supported
	RP_A_TEXTURE_COORDINATES	,		// Number of simultaneous texture coordinates supported
	
	RP_A_BUFFERS_DEPTH_LINEAR	,		// device supports W-buffering

	RP_A_ALPHA_ITERATED			,		// device supports iterated alpha
	RP_A_ALPHA_TEST				,		// device supports at least the equal and notequal alpha test comparison
	RP_A_ALPHA_TEST_ALL			,		// device supports all alpha test compare functions

	RP_A_BLEND_MUL_SRC			,		// Recommended SRCBLEND func for src*dst
	RP_A_BLEND_MUL_DST			,		// Recommended DESTBLEND func for src*dst
	RP_A_BLEND_ADD_SRC			,		// Recommended SRCBLEND func for src + dst
	RP_A_BLEND_ADD_DST			,		// Recommended DESTBLEND func for src + dst
	RP_A_BLEND_TRANSPARENCY_SRC	,		// Recommended SRCBLEND func for blending transparency
	RP_A_BLEND_TRANSPARENCY_DST	,		// Recommended DESTBLEND func for blending transparency
	RP_A_BLEND_MATRIX			,		// Returns the matrix of valid blend modes bitencoded in **** 4 U32 values ****

	RP_A_DEPTH_BIAS				,		// device supports ZBIAS

	RP_A_TEXTURE_OPS			,		// supported texture ops (use D3DTEXOPCAPS_ masks)
	RP_A_TEXTURE_ARG_FLAGS		,		// supported texture arg flags (use RP_TA_ masks)

	RP_A_DEVICE_FULLSCENE_ANTIALIAS,	// supports fullscene antialiasing

	// LEAVE ME LAST
	RP_A_MAX_ABILITY
} RPDEVICEABILITY;

//

// 
// rp_a_is_blend_supported()
//
// src	- source blend factor to test
// dst	- destination blend factor to test
// modes- blend matrix returned by query_device_ability( RP_A_BLEND_MATRIX, ... )
//        (This must be 4 U32s.)
//
//	i.e. 
//	U32 modes[4];
//
//	irp->query_device_ability( RP_A_BLEND_MATRIX, &modes[0], NULL );
//	if( rp_a_is_blend_supported( D3DBLEND_SRCALPHA, D3DBLEND_INVSRCALPHA, modes ) ) {...
//
//
inline bool rp_a_is_blend_supported( D3DBLEND src, D3DBLEND dst, U32 *modes )
{
#define NUM_FUNCS 10
	if( src > NUM_FUNCS || dst > NUM_FUNCS ) {
		return true;
	}

	U32 pos = ( (((U32)src) - 1) * NUM_FUNCS + ((U32)dst) - 1 );
	U32 idx = pos / 32;
	U32 bit = pos % 32;

	return (modes[idx] & (1<<bit))? true : false;
}

//

const U32 HTX_INVALID = 0;

//

typedef U32 IRP_VERTEXBUFFERHANDLE;
const IRP_VERTEXBUFFERHANDLE IRP_INVALID_VB_HANDLE = 0;

//

// Flags used with irp_vbf_flags parameter of create_vertex_buffer()
//
#define IRP_VBF_READ	(1<<0)		// client would like read access to the vertex buffer
#define IRP_VBF_NO_CLIP	(1<<1)		// no need to clip data in the vertex buffer
#define IRP_VBF_SYSTEM	(1<<2)		// force buffer into system memory

//

#define IRP_PVBF_TRANSFORM	(1<<0)	// transform and clip the vertex data
#define IRP_PVBF_LIGHT		(1<<1)	// light the vertex data
#define IRP_PVBF_SMART_COPY	(1<<2)	// only write vertices into destination that are transformed.

//

// Flags used with the irp_ctf_flags parameter
// of create_texture()
//
#define IRP_CTF_NON_TEXTURE			(1<<0)		// This surface will be **NOT** used as a texture map.
#define IRP_CTF_RENDER_TARGET		(1<<1)		// This surface will be used as the target of 3D rendering. (currently unsupported)
#define IRP_CTF_VIDEO_TARGET		(1<<2)		// This surface will be used as the target of video playback.
#define IRP_CTF_STATIC				(1<<3)		// This surface will not be accessed other than as a texture (not lockable).
#define IRP_CTF_DYNAMIC				(1<<4)		// This surface may be accessed (locked) often.
												
#define IRP_CTF_CM_POS_X			(1<<16)		// This surface should have the positive x cubic-environment-map sub-surface
#define IRP_CTF_CM_NEG_X			(1<<17)		// This surface should have the negative x cubic-environment-map sub-surface
#define IRP_CTF_CM_POS_Y			(1<<18)		// This surface should have the positive y cubic-environment-map sub-surface
#define IRP_CTF_CM_NEG_Y			(1<<19)		// This surface should have the negative y cubic-environment-map sub-surface
#define IRP_CTF_CM_POS_Z			(1<<20)		// This surface should have the positive z cubic-environment-map sub-surface
#define IRP_CTF_CM_NEG_Z			(1<<21)		// This surface should have the negative z cubic-environment-map sub-surface
#define IRP_CTF_CM_ALL				(0x3F0000)	// This surface should have all of the cubic-environment-map sub-surfaces 


//

#define IID_IRenderPipeline MAKE_IID("IRenderPipeline", 2)

// 
// IRenderPipeline
//
// Used to interface to the underlying rendering device.
//
struct IRenderPipeline : public IDAComponent
{
	virtual ID3DXEffect** load_effect(const char * filename,IComponentFactory *DIR)  = 0;
	virtual GENRESULT COMAPI set_vs_constants(UINT StartRegister,CONST FLOAT* pConstantData,UINT RegisterCount) = 0;
	virtual GENRESULT COMAPI set_ps_constants(UINT StartRegister,CONST FLOAT* pConstantData,UINT RegisterCount) = 0;
	virtual GENRESULT COMAPI set_vertex_declaration(IDirect3DVertexDeclaration9 *pDecl) = 0;
	virtual GENRESULT COMAPI set_default_constants( const Transform &W, Vector eye, U32 lightCount) = 0;
	// startup
	//
	// "Start up" a rendering device.  Note that this does not 
	// actually create any buffers (or the actual device), but it
	// does collect metadata about the device (abilities, names, etc.).
	//
	// profile_name defines an optional section in the profile (.ini)
	// that contains the device id and type to use.  If profile_name
	// is NULL, the section that is pre-defined in the profile will
	// be used.  i.e. Suppose the profile contains the following:
	//
	// [System]
	// IRenderPipeline = PimpedOut
	//
	// [PimpedOut]
	// DeviceId = {01010101-0101-0101-0101-010101010101}
	// DeviceType = Hardware
	//
	// [WimpedOut]
	// DeviceId = {00000000-0000-0000-0000-000000000000}
	// DeviceType = Software
	//
	// A call to startup( NULL ) would use the "PimpedOut" profile, while
	// a call to startup( "WimpedOut" ) would use the "WimpedOut" profile.
	//
	// **NO** other methods on this interface can be called before a
	// successful startup(), doing so will cause undefined behavior.
	//
	// Calling startup() will shutdown any previously "started" device.
	// 
	virtual GENRESULT COMAPI startup( const char *profile_name=NULL ) = 0;

	// shutdown
	//
	// Cleanup the device created with the most recent startup().
	//
	// **NO** other methods on this interface can be called after a
	// successful shutdown(), doing so will cause undefined behavior.
	//
	// shutdown() will automatically be called before the component
	// that implements this interface is destroyed.  Hence, it is not
	// strictly necessary to call shutdown() unless you want to explicitly
	// shutdown the current device (i.e. to clear all of the device data out).
	//
	virtual GENRESULT COMAPI shutdown( void  ) = 0;

	// get_device_info
	// 
	// Retrieves information about the device currently in use, if no
	// device has been "started", this method returns an error.
	//
	virtual GENRESULT COMAPI get_device_info( RPDEVICEINFO *info  ) = 0;

	// get_num_display_modes
	//
	// Retrieve the number of display modes that the current device 
	// supports.
	//
	virtual GENRESULT COMAPI get_num_display_modes( U32 *num_modes ) = 0;

	// get_display_mode
	//
	// Retrieve a description of a particular device display mode.
	//
	// mode_num is the value RP_CURRENT or a value in the 
	// range [0, num_modes) where num_modes is the value returned 
	// by get_num_display_modes(...).
	//
	// mode cannot be NULL
	//
	virtual GENRESULT COMAPI get_display_mode( RPDISPLAYMODEINFO *mode, U32 mode_num  ) = 0;

	// query_device_ability
	//
	// Determine whether the current device supports a given feature (ability).
	// 
	// ability is one of the defined device abilities.
	// out_answer points to the buffer to place the response into.  The length
	// of this buffer depends on the ability being queried.
	//
	virtual GENRESULT COMAPI query_device_ability( RPDEVICEABILITY ability, U32 *out_answer ) = 0;

	// set_pipeline_state
	//
	// Set the current value of a piece of pipeline state.  
	//
	virtual GENRESULT COMAPI set_pipeline_state( RPPIPELINESTATE state, U32 value  ) = 0;

	// get_pipeline_state
	//
	// Retrieve the current value of a piece of pipeline state.
	//
	virtual GENRESULT COMAPI get_pipeline_state( RPPIPELINESTATE state, U32 *value  ) = 0;

	// create_buffers
	//
	// Creates render buffers according to the input parameters and the
	// currently configured pipeline state, associates the buffers with the
	// given window, and sets all of the transform and render state to their 
	// default values.
	//
	// Note that any previously created buffers are destroyed.  
	//
	// **NOTE** Some methods on this interface are **ONLY** available (i.e.
	// return success) after a successful create_buffers call.  Such methods
	// are noted in their description.
	//
	// If this method returns failure, then there are no render buffers 
	// available (even if a previous call to create_buffers() succeeded).
	//
	virtual GENRESULT COMAPI create_buffers( HWND hwnd, int hres, int vres ) = 0;

	// destroy_buffers
	// 
	// Destroy the render buffers created with a previous call to create_buffers.
	//
	// **NOTE** Some methods on this interface are **NO LONGER** available (i.e.
	// always return failure) after a successful destroy_buffers call.  Such 
	// methods are noted in their description.
	//
	virtual GENRESULT COMAPI destroy_buffers( void  ) = 0;

	// get_num_device_texture_formats
	//
	// Retrieve the number of texture formats that the current device
	// supports.
	//
	// This method will always return failure 'outside' of successful create_buffers
	// and destroy_buffers calls.
	//
	virtual GENRESULT COMAPI get_num_device_texture_formats( U32 *num_formats ) = 0;

	// get_device_texture_format
	//
	// Retrieve a description of a particular device texture format.
	//
	// format_num is value in the  range [0, num_formats) where num_formats
	// is the value returned by get_num_device_texture_formats(...).
	//
	// texture_pf cannot be NULL
	//
	// This method will always return failure 'outside' of successful create_buffers
	// and destroy_buffers calls.
	//
	virtual GENRESULT COMAPI get_device_texture_format( PixelFormat *texture_pf, U32 format_num ) = 0;

	// get_device_stats
	//
	// Retrieves statistics about the current device such as texture
	// usage, primitive counts, etc.  Note that this method can stall
	// the pipeline and may have other performance side effects.
	//
	// This method will always return failure 'outside' of successful create_buffers
	// and destroy_buffers calls.
	//
	virtual GENRESULT COMAPI get_device_stats( RPDEVICESTATS *stat  ) = 0;

	// clear_buffers
	//
	// Clear the render buffers with the currently configured clear values.
	//
	// rp_clear_flags is a combination (bitwise-or) of the RP_CLEAR_*_BIT 
	// values defined above.
	//
	// If viewport_sub_rect is NULL, the entire viewport is cleared.
	// If viewport_sub_rect is not NULL, only the specified portion of the
	// viewport is cleared.
	//
	// This method will always return failure 'outside' of successful create_buffers
	// and destroy_buffers calls.
	//
	virtual GENRESULT COMAPI clear_buffers( U32 rp_clear_flags, RECT *viewport_sub_rect  ) = 0;

	// swap_buffers
	//
	// This is called when a client wants to make the current frame's 
	// rendering visible.  One should always call this at the end of a frame
	// after everything has been rendered and end_scene() is called.
	//
	// This method will always return failure 'outside' of successful create_buffers
	// and destroy_buffers calls.
	//
	virtual GENRESULT COMAPI swap_buffers( void  ) = 0;

	// lock_buffer
	//
	// Acquire a pointer to the pixel data in the render buffer.
	//
	// This method will always return failure 'outside' of successful create_buffers
	// and destroy_buffers calls.
	//
	virtual GENRESULT COMAPI lock_buffer( RPLOCKDATA *lockData  ) = 0;

	// unlock_buffer
	//
	// Release lock acquired in lock_buffer.
	//
	// This method will always return failure 'outside' of successful create_buffers
	// and destroy_buffers calls.
	//
	virtual GENRESULT COMAPI unlock_buffer( void  ) = 0;

	// get_buffer_interface
	//
	// Acquire an interface to some extended functionality attached
	// to the render buffers.  i.e. The video stream control, etc...
	//
	// This method will always return failure 'outside' of successful create_buffers
	// and destroy_buffers calls.
	//
	virtual GENRESULT COMAPI get_buffer_interface( const char *iid, void **out_iif  ) = 0;

	// get_buffer_dc
	//
	// Acquire a handle to a Windows(tm) Device Context associated with
	// the render buffers.
	//
	// This method will always return failure 'outside' of successful create_buffers
	// and destroy_buffers calls.
	//
	virtual GENRESULT COMAPI get_buffer_dc( HDC *dc  ) = 0;

	// release_buffer_dc
	//
	// Release the handle aquired with get_buffer_dc();
	//
	// This method will always return failure 'outside' of successful create_buffers
	// and destroy_buffers calls.
	//
	virtual GENRESULT COMAPI release_buffer_dc( HDC dc  ) = 0;

	// create_vertex_buffer
	//
	// Create a vertex buffer for use with vertex data.
	//
	// This method will always return failure 'outside' of successful create_buffers
	// and destroy_buffers calls.
	//
	virtual GENRESULT COMAPI create_vertex_buffer( U32 vertex_format, int num_verts, U32 irp_vbf_flags, IRP_VERTEXBUFFERHANDLE *out_vb_handle ) = 0;

	// destroy_vertex_buffer
	//
	//
	// This method will always return failure 'outside' of successful create_buffers
	// and destroy_buffers calls.
	//
	virtual GENRESULT COMAPI destroy_vertex_buffer( IRP_VERTEXBUFFERHANDLE vb_handle ) = 0;

	// lock_vertex_buffer
	//
	// Lock the given vertex buffer and return a pointer to the data.
	// Use ddlock_flags to optimize the lock for reading and writing.
	//
	// *out_vertex_buffer_size will contain the number of vertices in
	// the vertex buffer on return.
	//
	// This method will always return failure 'outside' of successful create_buffers
	// and destroy_buffers calls.
	//
	virtual GENRESULT COMAPI lock_vertex_buffer( IRP_VERTEXBUFFERHANDLE vb_handle, U32 ddlock_flags, void **out_vertex_buffer, U32 *out_vertex_buffer_size ) = 0;

	// unlock_vertex_buffer
	//
	// Unlock a vertex buffer previously locked with lock_vertex_buffer.
	//
	// This method will always return failure 'outside' of successful create_buffers
	// and destroy_buffers calls.
	//
	virtual GENRESULT COMAPI unlock_vertex_buffer( IRP_VERTEXBUFFERHANDLE vb_handle ) = 0;

	// optimize_vertex_buffer
	//
	// Convert the given vertex buffer to a device specific optimized vertex buffer.
	// 
	// NOTE: optimized vertex buffers CANNOT be locked after being optimized.
	//
	// This method will always return failure 'outside' of successful create_buffers
	// and destroy_buffers calls.
	//
	virtual GENRESULT COMAPI optimize_vertex_buffer( IRP_VERTEXBUFFERHANDLE vb_handle ) = 0;

	// process_vertex_buffer
	//
	// Transform, clip, and/or light a vertex buffer for later use by draw_*_primitive_vb.
	//
	// dst_vb_handle must point to a vertex buffer that supports transformed and lit
	// vertices.
	//
	// This method will always return failure 'outside' of successful create_buffers
	// and destroy_buffers calls.
	//
	virtual GENRESULT COMAPI process_vertex_buffer( IRP_VERTEXBUFFERHANDLE dst_vb_handle, IRP_VERTEXBUFFERHANDLE src_vb_handle, int first_vertex_index, int num_verts, U32 irp_pvbf_flags ) = 0;

	// set_modelview
	//
	// Set the current modelview transform
	//
	// This method will always return failure 'outside' of successful create_buffers
	// and destroy_buffers calls.
	//
	virtual GENRESULT COMAPI set_modelview( const Transform & modelview  ) = 0;

	// get_modelview
	//
	// Retrieve the current modelview transform
	//
	// This method will always return failure 'outside' of successful create_buffers
	// and destroy_buffers calls.
	//
	virtual GENRESULT COMAPI get_modelview( Transform & modelview  ) = 0;

	// set_projection
	//
	// Set the current projection transform.
	//
	// This method will always return failure 'outside' of successful create_buffers
	// and destroy_buffers calls.
	//
	virtual GENRESULT COMAPI set_projection( const Matrix4 & projection  ) = 0;

	// get_projection
	//
	// Get the current projection transform.
	//
	// This method will always return failure 'outside' of successful create_buffers
	// and destroy_buffers calls.
	//
	virtual GENRESULT COMAPI get_projection( Matrix4 & projection  ) = 0;

	// set_viewport
	//
	// Set the current viewport transform.
	//
	// This method will always return failure 'outside' of successful create_buffers
	// and destroy_buffers calls.
	//
	virtual GENRESULT COMAPI set_viewport( int x, int y, int w, int h  ) = 0;

	// get_viewport
	//
	// Get the current viewport transform.
	//
	// This method will always return failure 'outside' of successful create_buffers
	// and destroy_buffers calls.
	//
	virtual GENRESULT COMAPI get_viewport( int *x, int *y, int *w, int *h  ) = 0;

	// set_window
	//
	// Set the current window transform
	//
	// This method will always return failure 'outside' of successful create_buffers
	// and destroy_buffers calls.
	//
	virtual GENRESULT COMAPI set_window( HWND wnd, int x, int y, int w, int h  ) = 0;

	// get_window
	//
	// Get the current window transform
	//
	// This method will always return failure 'outside' of successful create_buffers
	// and destroy_buffers calls.
	//
	virtual GENRESULT COMAPI get_window( HWND *out_wnd, int *out_x, int *out_y, int *out_w, int *out_h  ) = 0;
	
	// set_depth_range
	//
	// Set the range of (normalized) z values that will be used when
	// rendering primitives. 
	//
	// The range is typically set to lower = 0.0, upper = 1.0 which 
	// means that the entire range of the depth buffer should be used.
	// However, one could use other values to "dice" up the depth buffer
	// into sectors to control depth buffer aliasing.
	//
	// This method will always return failure 'outside' of successful create_buffers
	// and destroy_buffers calls.
	//
	virtual GENRESULT COMAPI set_depth_range( float lower_z_bound, float upper_z_bound ) = 0;
	
	// get_depth_range
	//
	// Get the range of (normalized) z values that will be used when
	// rendering primitives. 
	//
	// This method will always return failure 'outside' of successful create_buffers
	// and destroy_buffers calls.
	//
	virtual GENRESULT COMAPI get_depth_range( float *out_lower_z_bound, float *out_upper_z_bound ) = 0;

	// set_lookat
	//
	// Set the current modelview transform such that it simulates an eye
	// at E=(eyex, eyey, eyez) looking toward C=(centerx,centery,centerz) with
	// a *world space* up vector approximating U=(upx, upy, upz).
	//
	// Note that if (V = E-C) == zero or V is parallel to U, then the resulting
	// modelview is undefined.
	//
	// This method will always return failure 'outside' of successful create_buffers
	// and destroy_buffers calls.
	//
	virtual GENRESULT COMAPI set_lookat( float eyex, float eyey, float eyez, float centerx, float centery, float centerz, float upx, float upy, float upz   ) = 0;

	// set_ortho
	//
	// Set the current projection transform to be an orthographic projection
	// with the view bounds set to the rectangle upperleft = (left,top) and
	// lowerright = (right,bottom) and a near clip plane set to nearval and
	// the far clip plane set to 1.0.
	//
	// This method will always return failure 'outside' of successful create_buffers
	// and destroy_buffers calls.
	//
	virtual GENRESULT COMAPI set_ortho( float left, float right, float bottom, float top, float nearval=-1.0, float farval=1.0  ) = 0;

	// set_perspective
	//
	// Set the current projection transform to be a perspective projection
	// with a vertical field-of-view of 'fovy' and a horizontal-to-vertical
	// aspect ratio of 'aspect'.
	//
	// This method will always return failure 'outside' of successful create_buffers
	// and destroy_buffers calls.
	//
	virtual GENRESULT COMAPI set_perspective( float fovy, float aspect, float znear, float zfar  ) = 0;

	// set_light
	//
	// Set the light properties associated with the given light_index.  The specified
	// index can be any valid unsigned 32-bit value.
	//
	// NOTE: this method does not affect whether the given light is enabled or 
	// NOTE: not...  i.e. lights can be set and enabled orthogonally.
	//
	// This method will always return failure 'outside' of successful create_buffers
	// and destroy_buffers calls.
	//
	virtual GENRESULT COMAPI set_light( U32 light_index, D3DLIGHT9 *light_values ) = 0;

	// get_light
	//
	// Get the light properties associated with the given light index.
	//
	// This method will always return failure 'outside' of successful create_buffers
	// and destroy_buffers calls.
	//
	virtual GENRESULT COMAPI get_light( U32 light_index, D3DLIGHT9 *out_light_values ) = 0;

	// set_light_enable
	//
	// Enable the given light.
	//
	// This method will always return failure 'outside' of successful create_buffers
	// and destroy_buffers calls.
	//
	virtual GENRESULT COMAPI set_light_enable( U32 light_index, U32 enable ) = 0;

	// get_light_enable 
	//
	// Determine whether the given light is enabled.
	//
	// This method will always return failure 'outside' of successful create_buffers
	// and destroy_buffers calls.
	//
	virtual GENRESULT COMAPI get_light_enable( U32 light_index, U32 *out_enable ) = 0;

	// get_num_lights
	//
	// Get the light number of lights that have been set with set_light()
	//
	// This method will always return failure 'outside' of successful create_buffers
	// and destroy_buffers calls.
	//
	virtual GENRESULT COMAPI get_num_lights( U32 *out_num_lights ) = 0;

	// get_lights
	//
	// Get the 'num_lights' light_indices (for use with set_light) of the lights 
	// in the starting with num_start.
	//
	// This method will always return failure 'outside' of successful create_buffers
	// and destroy_buffers calls.
	//
	virtual GENRESULT COMAPI get_lights( U32 num_start, U32 num_lights, U32 *out_light_indices ) = 0;

	// set_material
	//
	// Set the current material properties.
	//
	// This method will always return failure 'outside' of successful create_buffers
	// and destroy_buffers calls.
	//
	virtual GENRESULT COMAPI set_material( D3DMATERIAL9 *material_values ) = 0;

	// get_material
	//
	// Get the current material properties.
	//
	// This method will always return failure 'outside' of successful create_buffers
	// and destroy_buffers calls.
	//
	virtual GENRESULT COMAPI get_material( D3DMATERIAL9 *out_material_values ) = 0;


	// begin_scene
	//
	// Delimits the beginning of a scene frame.  Call this only once
	// per frame per render target.
	//
	// This method will always return failure 'outside' of successful create_buffers
	// and destroy_buffers calls.
	//
	virtual GENRESULT COMAPI begin_scene( void  ) = 0;

	// end_scene
	//
	// Delimits the end of a scene frame.  Call this only once per 
	// frame per render target.
	//
	// This method will always return failure 'outside' of successful create_buffers
	// and destroy_buffers calls.
	//
	virtual GENRESULT COMAPI end_scene( void  ) = 0;

	// set_render_state
	//
	// Set the current value of a render state.
	//
	// This method will always return failure 'outside' of successful create_buffers
	// and destroy_buffers calls.
	//
	virtual GENRESULT COMAPI set_render_state( D3DRENDERSTATETYPE state, U32 value  ) = 0;

	// get_render_state
	//
	// Get the current value of a render state.
	//
	// This method will always return failure 'outside' of successful create_buffers
	// and destroy_buffers calls.
	//
	virtual GENRESULT COMAPI get_render_state( D3DRENDERSTATETYPE state, U32 *value  ) = 0;

	// set_texture_stage_state 
	//
	// Set the value of a piece of texture stage state.
	//
	// This method will always return failure 'outside' of successful create_buffers
	// and destroy_buffers calls.
	//
	virtual GENRESULT COMAPI set_sampler_state( U32 stage, D3DSAMPLERSTATETYPE, U32 value  ) = 0;

	virtual GENRESULT COMAPI get_sampler_state( U32 stage, D3DSAMPLERSTATETYPE, U32 *value  ) = 0;


	virtual GENRESULT COMAPI set_texture_stage_state( U32 stage, D3DTEXTURESTAGESTATETYPE, U32 value  ) = 0;

	// get_texture_stage_state
	//
	// Get the value of a piece of texture stage state.
	//
	// This method will always return failure 'outside' of successful create_buffers
	// and destroy_buffers calls.
	//
	virtual GENRESULT COMAPI get_texture_stage_state( U32 stage, D3DTEXTURESTAGESTATETYPE, U32 *value  ) = 0;

	// set_texture_stage_transform
	//
	// Set the value of the texture coordinate transform for the given texture stage.
	//
	// This method will always return failure 'outside' of successful create_buffers
	// and destroy_buffers calls.
	//
	virtual GENRESULT COMAPI set_texture_stage_transform( U32 stage, Matrix4 &mat4 ) = 0;

	// get_texture_stage_transform
	//
	// Get the value of the texture coordinate transform for the given texture stage.
	//
	// This method will always return failure 'outside' of successful create_buffers
	// and destroy_buffers calls.
	//
	virtual GENRESULT COMAPI get_texture_stage_transform( U32 stage, Matrix4 &out_mat4 ) = 0;

	// set_texture_stage_texture
	//
	// Set the current texture of a texture stage.  
	// If htexture == 0, no texture is available in the stage.
	//
	// This method will always return failure 'outside' of successful create_buffers
	// and destroy_buffers calls.
	//
	virtual GENRESULT COMAPI set_texture_stage_texture( U32 stage, U32 htexture  ) = 0;

	// get_texture_stage_texture
	//
	// Get the current texture of a texture stage.
	//
	// This method will always return failure 'outside' of successful create_buffers
	// and destroy_buffers calls.
	//
	virtual GENRESULT COMAPI get_texture_stage_texture( U32 stage, U32 *htexture  ) = 0;

	// verify_state
	//
	// Determine whether a primitive could be rendered with the currently 
	// set render and texture stage state values.
	//
	// This method will always return failure 'outside' of successful create_buffers
	// and destroy_buffers calls.
	//
	virtual GENRESULT COMAPI verify_state( void ) = 0;

	//
	// create a state block and record the current values of the specified states.
	// return the new state block in the given handle.
	//
	// This method will always return failure 'outside' of successful create_buffers
	// and destroy_buffers calls.
	//
	virtual GENRESULT COMAPI create_state_block( D3DSTATEBLOCKTYPE type, U32*out_sbhandle ) = 0;

	//
	// update the given state block with the current values of the recorded states
	// in the stateblock.
	//
	// This method will always return failure 'outside' of successful create_buffers
	// and destroy_buffers calls.
	//
	virtual GENRESULT COMAPI update_state_block( U32 sbhandle ) = 0;

	//
	// begin recording a new state block
	//
	// The following state-related calls produce state changes that are recorded
	// in the state block:
	// 
	// set_light
	// set_light_enable
	// set_material
	// set_render_state
	// set_texture
	// set_texture_stage_state
	// set_texture_stage_texture
	// set_modelview
	// set_projection
	// set_lookat
	// set_perspective
	// set_ortho
	// set_viewport
	// set_depth_range
	//
	//
	// This method will always return failure 'outside' of successful create_buffers
	// and destroy_buffers calls.
	//
	virtual GENRESULT COMAPI begin_state_block( void ) = 0;

	//
	// stop recording a stateblock and return the handle of the created stateblock.
	//
	// This method will always return failure 'outside' of successful create_buffers
	// and destroy_buffers calls.
	//
	virtual GENRESULT COMAPI end_state_block( U32*out_sbhandle ) = 0;

	//
	// apply a previously recorded or created stateblock
	//
	// This method will always return failure 'outside' of successful create_buffers
	// and destroy_buffers calls.
	//
	virtual GENRESULT COMAPI apply_state_block( U32 sbhandle ) = 0;

	//
	// delete a state block
	//
	// This method will always return failure 'outside' of successful create_buffers
	// and destroy_buffers calls.
	//
	virtual GENRESULT COMAPI delete_state_block( U32 sbhandle ) = 0;

	// draw_primitive
	//
	// Render a primitive of the given type with the given vertices.
	//
	// This method will always return failure 'outside' of successful create_buffers
	// and destroy_buffers calls.
	//
	virtual GENRESULT COMAPI draw_primitive( D3DPRIMITIVETYPE type, U32 vertex_format, const void *verts, int num_verts, U32 flags  ) = 0;

	// draw_indexed_primitive
	//
	// Render an indexed primitive of the given type with the given vertices.
	//
	// This method will always return failure 'outside' of successful create_buffers
	// and destroy_buffers calls.
	//
	virtual GENRESULT COMAPI draw_indexed_primitive( D3DPRIMITIVETYPE type, U32 vertex_format, const void *verts, int num_verts, const U16 * indices, int num_indices, U32 flags  ) = 0;
	
	// draw_primitive
	//
	// Render a primitive of the given type with the given vertices.
	//
	// This method will always return failure 'outside' of successful create_buffers
	// and destroy_buffers calls.
	//
	virtual GENRESULT COMAPI draw_primitive_vb( D3DPRIMITIVETYPE type, IRP_VERTEXBUFFERHANDLE vbhandle, int start_vert, int num_verts, U32 flags  ) = 0;

	// draw_indexed_primitive
	//
	// Render an indexed primitive of the given type with the given vertices.
	//
	// This method will always return failure 'outside' of successful create_buffers
	// and destroy_buffers calls.
	//
	virtual GENRESULT COMAPI draw_indexed_primitive_vb( D3DPRIMITIVETYPE type, IRP_VERTEXBUFFERHANDLE vbhandle, int start_vert, int num_verts, const U16 * indices, int num_indices, U32 flags  ) = 0;
	virtual GENRESULT COMAPI draw_indexed_primitive_vb( D3DPRIMITIVETYPE type, IRP_VERTEXBUFFERHANDLE vbhandle, int start_vert, int num_verts,  IDirect3DIndexBuffer9* IB, int num_indices, U32 flags  )=0;
	virtual GENRESULT COMAPI create_index_buffer(UINT Length, IDirect3DIndexBuffer9** ppIndexBuffer)=0;
 	
	// create_texture
	//
	// Create a texture surface.  
	//
	// width, height - The dimensions of the texture.  If these values are larger
	//                 than the maximum dimensions supported by the device, this  
	//                 method will fail.
	//
	// desiredformat - The desired format of the new texture.  This can either be
	//                 a fully-specified PixelFormat or a PixelFormat that specifies
	//                 a texture format class.  If no suitable format is available
	//                 (i.e. the fully-specified format is not available nor are any
	//                 of the formats in the texture format class), this method will 
	//                 fail.
	//
	// num_lod       - The number of levels of detail in the texture.  If this value
	//                 is non-zero, a mipmap chain of num_lod length will be created.
	//                 If this value is zero, a non-mipmapped texture will be created.
	//                 
	// irp_ctf_flags - Flags that control the type of texture to create.  This can 
	//                 either be zero (0) to create a normal texture, or a combination
	//                 of the IRP_CTF_* flags defined above.
	//
	// out_handle    - Reference to the place to store the handle to the created texture.
	//
	// This method will always return failure 'outside' of successful create_buffers
	// and destroy_buffers calls.
	//

	virtual GENRESULT COMAPI create_cube_texture_from_file(const char* filename, IComponentFactory * DIR,U32 &out_htexture) = 0;

	virtual GENRESULT COMAPI create_texture( int width, int height, const PixelFormat &desiredformat, int num_lod, U32 irp_ctf_flags, U32 &out_htexture  ) = 0;

	// destroy_texture
	//
	// Destroy a texture created with create_texture(). 
	//
	// htexture cannot be zero (0) nor RP_CURRENT.
	//
	// This method will always return failure 'outside' of successful create_buffers
	// and destroy_buffers calls.
	//
	virtual GENRESULT COMAPI destroy_texture( U32 htexture  ) = 0;

	// is_texture
	//
	// Determine whether a given texture handle references a valid texture.
	//
	// htexture cannot be zero (0) nor RP_CURRENT.
	//
	// This method will always return failure 'outside' of successful create_buffers
	// and destroy_buffers calls.
	//
	virtual GENRESULT COMAPI is_texture( U32 htexture  ) = 0;

	// lock_texture
	//
	// Retrieve a pointer to the texel data of the given level of detail of a texture.
	// One can lock (and unlock) different levels of detail independently.
	//
	// This method will always return failure 'outside' of successful create_buffers
	// and destroy_buffers calls.
	//
	virtual GENRESULT COMAPI lock_texture( U32 htexture, int level, RPLOCKDATA *lockData  ) = 0;

	// unlock_texture
	//
	// Release lock acquired in lock_texture.
	//
	// This method will always return failure 'outside' of successful create_buffers
	// and destroy_buffers calls.
	//
	virtual GENRESULT COMAPI unlock_texture( U32 htexture, int level  ) = 0;

	// get_texture_format
	//
	// Get the texture format of a given texture.
	//
	// This method will always return failure 'outside' of successful create_buffers
	// and destroy_buffers calls.
	//
	virtual GENRESULT COMAPI get_texture_format( U32 htexture, PixelFormat *out_pf  ) = 0;

	// get_texture_dim
	//
	// Get the width, height, and the number of levels of detail of a given texture.
	//
	// This method will always return failure 'outside' of successful create_buffers
	// and destroy_buffers calls.
	//
	virtual GENRESULT COMAPI get_texture_dim( U32 htexture, U32 *out_width, U32 *out_height, U32 *out_num_lod  ) = 0;

	// get_texture_interface
	//
	// Retrieve extension interfaces from a particular texture.
	//
	// This method will always return failure 'outside' of successful create_buffers
	// and destroy_buffers calls.
	//
	virtual GENRESULT COMAPI get_texture_interface( U32 htexture, const char *iid, void **out_iif  ) = 0;

	// get_texture_dc
	//
	// Acquire a handle to a Windows(tm) Device Context associated with
	// the given texture.
	//
	// This method will always return failure 'outside' of successful create_buffers
	// and destroy_buffers calls.
	//
	virtual GENRESULT COMAPI get_texture_dc( U32 htexture, HDC *out_hdc ) = 0;

	// release_texture_dc
	//
	// Release the handle aquired with get_buffer_dc();
	//
	// This method will always return failure 'outside' of successful create_buffers
	// and destroy_buffers calls.
	//
	virtual GENRESULT COMAPI release_texture_dc( U32 htexture, HDC hdc ) = 0;

	// set_texture_palette
	//
	// Set the palette of a given texture.  If the format of the texture does not
	// require a palette, this method does nothing and returns success.
	//
	// This method will always return failure 'outside' of successful create_buffers
	// and destroy_buffers calls.
	//
	virtual GENRESULT COMAPI set_texture_palette( U32 htexture, int start, int length, const RGB *colors  ) = 0;

	// get_texture_palette
	//
	// Retrieve the palette of the given texture.  If the format of the texture does
	// not require a palette, this method fails.
	//
	// This method will always return failure 'outside' of successful create_buffers
	// and destroy_buffers calls.
	//
	virtual GENRESULT COMAPI get_texture_palette( U32 htexture, int start, int length, RGB *colors  ) = 0;

	// set_texture_level_data
	//
	// Set the texel data for the given level of detail of the given texture. 
	//
	// src_width, src_height - The width and height (in texels) of the source texel and alpha data.
	// src_stride            - The width (in bytes) of the source texel data.
	// src_format            - The format of the source texel data.
	// src_pixel             - The source texel data.
	// src_alpha             - The source alpha data.  If not NULL, the 8-bit values in this buffer
	//                         are used as the alpha channel of the texel data (even if src_pixels 
	//                         contains alpha data.)  If this value is NULL and src_format specifies
	//                         a format with an alpha channel, alpha values of 255 will be generated.
	// src_palette           - The palette for the source data.  If src_format specifies palettized
	//                         data, this palette is used.  It is the responsibility of the client
	//                         to make sure that the data in src_pixel does not reference any entry
	//                         that does not exist in src_palette.
	//
	// This method will always return failure 'outside' of successful create_buffers
	// and destroy_buffers calls.
	//
	virtual GENRESULT COMAPI set_texture_level_data( U32 htexture, int level, int src_width, int src_height, int src_stride, const PixelFormat &src_format, const void *src_pixel, const void *src_alpha, const RGB *src_palette ) = 0;

	// blit_texture
	//
	// 
	//
	// This method will always return failure 'outside' of successful create_buffers
	// and destroy_buffers calls.
	//
	virtual GENRESULT COMAPI blit_texture( U32 hDest, U32 destLevel, RECT destRect, U32 hSrc, U32 srcLevel, RECT srcRect  ) = 0;

	// get_num_textures
	//
	// Retrieve the number of currently managed textures in the system.
	//
	// This method will always return failure 'outside' of successful create_buffers
	// and destroy_buffers calls.
	//
	virtual GENRESULT COMAPI get_num_textures( U32 *out_num_textures  ) = 0;

	// get_texture
	//
	// Retrieve a handle to a particular managed texture.  texture_num is
	// in the range [0, num_textures) where num_textures is the value specified
	// by a call to get_num_textures().
	//
	// NOTE that adding or removing textures will cause index values (values of
	// texture_num) to be meaningless.  i.e. do not modify the texture database
	// while enumerating the database values.
	//
	// This method will always return failure 'outside' of successful create_buffers
	// and destroy_buffers calls.
	//
	virtual GENRESULT COMAPI get_texture( U32 texture_num, U32 *out_htexture  ) = 0;
};

//

//
// RPUL_DACOMDESC
// 
// This is used to create a renderpipe device outside of the 
// system aggregate.  RPUL uses this when enumerating available
// renderpipeline devices.
//
struct RPUL_DACOMDESC : public DACOMDESC
{
	const C8 *device_class;
	const C8 *device_id;
	IProfileParser *profile_parser;
	
	RPUL_DACOMDESC( const C8 *_dc, const C8 *_di, IProfileParser *_profile_parser=NULL ) 
	{
		device_class = _dc;
		device_id = _di;
		profile_parser = _profile_parser; 
		interface_name = "IDAComponent";
		size = sizeof(*this);
	}
};

//

#endif	// EOF
