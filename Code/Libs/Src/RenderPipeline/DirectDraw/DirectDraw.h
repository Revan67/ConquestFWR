// DirectDraw.h
//
// DirectDraw Utility functions
// 
//


#ifndef DA_DIRECTDRAW_H
#define DA_DIRECTDRAW_H

//

#define WINDOWS_LEAN_AND_MEAN
#include <windows.h>
#include <ddraw.h>
#include <d3d9.h>

#include "typedefs.h"
#include "pixel.h"
#include "rendpipeline.h"
#include "rpul.h"

//

#ifndef RELEASE
#define RELEASE(iff) if( iff ) { iff->Release(); iff = NULL; }
#endif

// 

#define _R(color) (((color)>>16) & 0xFF)
#define _G(color) (((color)>> 8) & 0xFF)
#define _B(color) (((color)>> 0) & 0xFF)
#define _A(color) (((color)>>24) & 0xFF)

//

//
// DirectDraw Related Stuff
//

#define RPRD_F_HWFLIP				(1<<0)
#define RPRD_F_HWVSYNC				(1<<1)
#define RPRD_F_FULLSCREEN			(1<<2)
#define RPRD_F_FULLSCREEN_NOW		(1<<3)
#define RPRD_F_FULLSCREEN_ALWAYS	(1<<4)
#define RPRD_F_OFFSCREEN			(1<<5)
#define RPRD_F_HWVIDMEM				(1<<6)
#define RPRD_F_HW3D					(1<<7)
#define RPRD_F_ANTIALIAS_HINT		(1<<8)
#define RPRD_F_TEXTURE_8BIT			(1<<9)
#define RPRD_F_TEXTURE_DXT			(1<<10)
#define RPRD_F_FLIP					(1<<11)
#define RPRD_F_HWTHREADED			(1<<12)
#define RPRD_F_FLIP_STALLS			(1<<13)
#define RPRD_F_BROKEN_MULTITEXTURE	(1<<14)
#define RPRD_F_GAMMA_CALIBRATION	(1<<15)
#define RPRD_F_FPUPRESERVE			(1<<16)
#define RPRD_F_FPUPRESETUP			(1<<17)

struct RPDDCREATEBUFFERSDATA 
{
	U32 buf_width;			// (0, max_display_width]
	U32 buf_height;			// (0, max_display_height]
	U32	buf_color_buffers;	// total number of color buffers
	U32	buf_color_bpp;		// 
	U32	buf_depth_bpp;		// non-zero for depth buffer
	U32	buf_stencil_bpp;	// non-zero for stencil buffer
	U32	rprd_flags;
};

//

extern U32 rp_dd_debug_fill_color;

//

#define RPDD_CSF_FOR_SOFTWARE	(1<<0)
#define RPDD_CSF_DYNAMIC		(1<<1)
#define RPDD_CSF_ANTIALIAS_HINT	(1<<2)
#define RPDD_CSF_CM_POS_X		(1<<16)
#define RPDD_CSF_CM_NEG_X		(1<<17)
#define RPDD_CSF_CM_POS_Y		(1<<18)
#define RPDD_CSF_CM_NEG_Y		(1<<19)
#define RPDD_CSF_CM_POS_Z		(1<<20)
#define RPDD_CSF_CM_NEG_Z		(1<<21)
#define RPDD_CSF_CM_ALL			(0x3F0000)


//

HRESULT rp_dd_is_directx7				( void );

HRESULT rp_dd_find_device_guid			( const char *device_guid, const char *dll_name, char *out_device_guid );
HRESULT rp_dd_init_directdraw			( LPGUID lpGuid, const char *dll_name, LPDIRECTDRAW7 *out_lpDD );
HRESULT rp_dd_cleanup_directdraw		( void );

HRESULT rp_dd_get_display_mode			( LPDIRECTDRAW7 lpDD, RPDISPLAYMODEINFO *out_display_mode );
HRESULT rp_dd_get_display_modes			( LPDIRECTDRAW7 lpDD, RPDISPLAYMODEINFO **modes, U32 *num_modes );
HRESULT rp_dd_get_driver_id				( LPDIRECTDRAW7 lpDD, LPGUID out_lpGuid );
HRESULT rp_dd_get_driver_info			( LPDIRECTDRAW7 lpDD, char *out_device_name, char *out_device_desc );
HRESULT rp_dd_get_device_id				( LPDIRECTDRAW7 lpDD, RPDEVICEID &id );
HRESULT rp_dd_is_nvidia					( LPDIRECTDRAW7 lpDD );
HRESULT rp_dd_get_memory				( LPDIRECTDRAW7 lpDD, U32 caps, U32 *out_total_bytes, U32 *out_free_bytes );
HRESULT rp_dd_get_syslock				( LPDIRECTDRAW7 lpDD, LPDIRECTDRAWSURFACE7 surface );
HRESULT rp_dd_get_2d_only				( LPDIRECTDRAW7 lpDD );
HRESULT rp_dd_get_fullscreen_override	( LPDIRECTDRAW7 lpDD );

HRESULT rp_dd_set_fullscreen			( LPDIRECTDRAW7 lpDD, HWND hwnd, U32 width, U32 height, U32 bpp, U32 rpdd_flags );
HRESULT rp_dd_set_windowed				( LPDIRECTDRAW7 lpDD, HWND hwnd, U32 rpdd_flags );
HRESULT rp_dd_create_basic_surfaces		( LPDIRECTDRAW7 lpDD, RPDDCREATEBUFFERSDATA *init_data, LPDIRECTDRAWSURFACE7 *out_color_buffers );
HRESULT rp_dd_swap_buffers				( LPDIRECTDRAW7 lpDD, LPDIRECTDRAWSURFACE7 *color_buffers, U32 num_color_buffers, RECT *src, RECT *dst, U32 rprd_flags );

HRESULT rp_dd_create_surface			( LPDIRECTDRAW7 lpDD, U32 rpdd_csf_flags, U32 caps, U32 w, U32 h, U32 lod, PixelFormat *pf, LPDIRECTDRAWSURFACE7 *out_surface );
HRESULT rp_dd_surface_set_palette		( LPDIRECTDRAW7 lpDD, LPDIRECTDRAWSURFACE7 surface, U32 start_colors, U32 num_colors, RGB *colors);
HRESULT rp_dd_surface_get_palette		( LPDIRECTDRAWSURFACE7 surface, U32 start_colors, U32 max_num_colors, RGB *out_colors);
HRESULT rp_dd_surface_get_mipmaps		( LPDIRECTDRAWSURFACE7 surface, U32 *out_mipcount, LPDIRECTDRAWSURFACE7 *out_mipmaps ); 
HRESULT rp_dd_surface_get_dims			( LPDIRECTDRAWSURFACE7 surface, U32 *out_width, U32 *out_height, U32 *out_mipcount ); 
HRESULT rp_dd_surface_verify			( LPDIRECTDRAWSURFACE7 surface );
HRESULT rp_dd_surface_restore			( LPDIRECTDRAWSURFACE7 surface );
HRESULT rp_dd_surface_bitblt			( LPDIRECTDRAWSURFACE7 surface, const void *src_bits, int src_width, int src_height, int src_stride, const PixelFormat &src_format, const RGB *src_palette, const U8 *src_alpha );
HRESULT rp_dd_surface_fill				( LPDIRECTDRAWSURFACE7 surface, PixelFormat *pf, U32 value, U32 flags, RECT *scissor );


//HRESULT rp_d3d_is_device_type_supported	( LPDIRECTDRAW7 lpDD, LPGUID device_type_guid );
//HRESULT rp_d3d_get_texture_formats		( LPDIRECT3DDEVICE9 lpD3D, PixelFormat **formats, U32 *num_formats );
//HRESULT rp_d3d_get_blend_matrix			( LPDIRECTDRAW7 lpDD, LPD3DDEVICEDESC lpD3DCaps, U32 *out_modes );

struct RPDDCLIPPERDATA
{
	HWND hwnd;
	LPDIRECTDRAWCLIPPER clipper;
	U32 win_x, win_y, win_w, win_h;
};


//-----------------------------------------------------------------
// inlines
//-----------------------------------------------------------------

template <class type> 
inline U32 verify_buffer( U32 num_wanted, U32 &num_there, type **buffer )
{
	if( num_wanted > num_there ) {
		type *temp = *buffer;
		if( (*buffer = new type[num_wanted]) != NULL ) {
			memcpy( buffer, temp, sizeof(type)*num_there );
			delete[] temp;
			num_there = num_wanted;
			return num_wanted;
		}
		else {
			*buffer = temp;
			return num_there;
		}
	}
	return num_there;
}


inline void Transform2D3D(D3DMATRIX & dst, const Transform & src)
{
	//
	// Transpose rotation.
	//
		dst.m[0][0] = src.d[0][0];
		dst.m[1][0] = src.d[0][1];
		dst.m[2][0] = src.d[0][2];
		dst.m[0][1] = src.d[1][0];
		dst.m[1][1] = src.d[1][1];
		dst.m[2][1] = src.d[1][2];
		dst.m[0][2] = src.d[2][0];
		dst.m[1][2] = src.d[2][1];
		dst.m[2][2] = src.d[2][2];

		dst.m[3][0] = src.translation.x;
		dst.m[3][1] = src.translation.y;
		dst.m[3][2] = src.translation.z;
		dst.m[0][3] = 
		dst.m[1][3] = 
		dst.m[2][3] = 0;
		dst.m[3][3] = 1;
}

inline void D3D2Transform(const D3DMATRIX & src, Transform & dest)
{
	//
	// Transpose rotation.
	//
		dest.d[0][0] = src.m[0][0] ;
		dest.d[0][1] = src.m[1][0] ;
		dest.d[0][2] = src.m[2][0] ;
		dest.d[1][0] = src.m[0][1] ;
		dest.d[1][1] = src.m[1][1] ;
		dest.d[1][2] = src.m[2][1] ;
		dest.d[2][0] = src.m[0][2] ;
		dest.d[2][1] = src.m[1][2] ;
		dest.d[2][2] = src.m[2][2] ;

		dest.translation.x = src.m[3][0];
		dest.translation.y = src.m[3][1]; 
		dest.translation.z = src.m[3][2];  
}

inline void Matrix2D3D(D3DMATRIX & dst, const Matrix4 & src)
{
	//
	// Transpose rotation.
	//
		dst.m[0][0] = src.d[0][0];
		dst.m[1][0] = src.d[0][1];
		dst.m[2][0] = src.d[0][2];
		dst.m[3][0] = src.d[0][3];
		
		dst.m[0][1] = src.d[1][0];
		dst.m[1][1] = src.d[1][1];
		dst.m[2][1] = src.d[1][2];
		dst.m[3][1] = src.d[1][3];
		
		dst.m[0][2] = src.d[2][0];
		dst.m[1][2] = src.d[2][1];
		dst.m[2][2] = src.d[2][2];
		dst.m[3][2] = src.d[2][3];

		dst.m[0][3] = src.d[3][0];
		dst.m[1][3] = src.d[3][1];
		dst.m[2][3] = src.d[3][2];
		dst.m[3][3] = src.d[3][3];
}

inline void D3D2Matrix(const D3DMATRIX & src, Matrix4 & dest)
{
	//
	// Transpose rotation.
	//
		dest.d[0][0] = src.m[0][0] ;
		dest.d[0][1] = src.m[1][0] ;
		dest.d[0][2] = src.m[2][0] ;
		dest.d[0][3] = src.m[3][0] ;

		dest.d[1][0] = src.m[0][1] ;
		dest.d[1][1] = src.m[1][1] ;
		dest.d[1][2] = src.m[2][1] ;
		dest.d[1][3] = src.m[3][1] ;
		
		dest.d[2][0] = src.m[0][2] ;
		dest.d[2][1] = src.m[1][2] ;
		dest.d[2][2] = src.m[2][2] ;
		dest.d[2][3] = src.m[3][2] ;

		dest.d[3][0] = src.m[0][3] ;
		dest.d[3][1] = src.m[1][3] ;
		dest.d[3][2] = src.m[2][3] ;
		dest.d[3][3] = src.m[3][3] ;
}

#endif // EOF
