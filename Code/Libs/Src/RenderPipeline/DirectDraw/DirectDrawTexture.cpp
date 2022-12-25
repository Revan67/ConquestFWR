// DirectDrawTexture.cpp
//
//
// 

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

//

#include "DACOM.h"
#include "FDUMP.h"
#include "TEMPSTR.h"

//


#include "DirectDrawTexture.h"

//

const char *rp_rd_ddmessage( HRESULT error );


//


//
// Texture Related Stuff
//

//

HRESULT rp_tm_texture_adjust_dims( U32 *inout_width, U32 *inout_height, U32 *inout_lod_count )
{
	ASSERT( inout_width );
	ASSERT( inout_height );
	ASSERT( inout_lod_count );

	U32 high_bit=0, bit_cnt=1;

	if( *inout_width & (*inout_width - 1) ) {
		GENERAL_TRACE_1( "rp_tm_texture_adjust_dims: texture width is not power of two\n" );
		// not a power of two
		high_bit = *inout_width;
		while( high_bit != 1 ) {
			high_bit >>= 1;
			bit_cnt++;
		}
		*inout_width = 1<<(bit_cnt+1);
	}

	if( *inout_height & (*inout_height - 1) ) {
		GENERAL_TRACE_1( "rp_tm_texture_adjust_dims: texture height is not power of two\n" );
		// not a power of two
		high_bit = *inout_height;
		while( high_bit != 1 ) {
			high_bit >>= 1;
			bit_cnt++;
		}
		*inout_height = 1<<(bit_cnt+1);
	}

	return S_OK;
}

//

HRESULT rp_tm_texture_choose_format( PixelFormat *in_format, RPTEXTUREFORMATCLASS *texture_format_classes, U32 num_texture_format_classes, PixelFormat *supported_texture_formats, U32 num_supported_texture_formats, U32 flags, PixelFormat *out_format )
{
	ASSERT( in_format );
	ASSERT( texture_format_classes );
	ASSERT( supported_texture_formats );
	ASSERT( out_format );

	out_format->init( 0,0,0,0,0 );

	if( in_format->is_fourcc() ) {

		for( U32 fcc=0; fcc<num_texture_format_classes; fcc++ ) {
			
			if( texture_format_classes[fcc].fourcc == in_format->get_fourcc() ) {

				U32 tf = 0;
				PixelFormat *rpf = texture_format_classes[fcc].formats;

				while( tf<texture_format_classes[fcc].num_formats ) {
					
					for( U32 sf=0; sf<num_supported_texture_formats; sf++ ) {

						if( supported_texture_formats[sf].is_indexed() && !(flags & RPRD_F_TEXTURE_8BIT) ) {
							continue;	// don't allow palettized textures if flag is set
						}
						else if( !supported_texture_formats[sf].is_indexed() && supported_texture_formats[sf].num_bits() < 16 ) {
							continue;	// don't allow non-palettized textures less than 16bpp ever
						}
						else if( supported_texture_formats[sf].is_fourcc() && !(flags & RPRD_F_TEXTURE_DXT) ) {
							continue;	// don't allow fourcc codes
						}
						else if( supported_texture_formats[sf].is_equal( *rpf ) ) {
							out_format->init( supported_texture_formats[sf] );
							return S_OK;
						}
					}

					tf++;
					rpf++;
				}

				break; // break out of texture format class check
			}
		}
	}

	// Find exact match to the in_format, fail if not found.
	//
	for( U32 i=0; i<num_supported_texture_formats; i++ ) {

		if( supported_texture_formats[i].is_indexed() && !(flags & RPRD_F_TEXTURE_8BIT) ) {
			continue;	// don't allow palettized textures if flag is set
		}
		else if( !supported_texture_formats[i].is_indexed() && supported_texture_formats[i].num_bits() < 16 ) {
			continue;	// don't allow non-palettized textures less than 16bpp ever
		}
		else if( supported_texture_formats[i].is_fourcc() && !(flags & RPRD_F_TEXTURE_DXT) ) {
			continue;	// don't allow fourcc codes
		}
		else if( supported_texture_formats[i].is_equal( *in_format ) ) {
			out_format->init( supported_texture_formats[i] );
			return S_OK;
		}
	}

#if !defined(FINAL_RELEASE)
	if( in_format->is_indexed() ) {
		GENERAL_TRACE_1( "Unable to find texture format for indexed format.\n" );
	}
	else if( in_format->is_fourcc() ) {
		U32 fcc = in_format->get_fourcc();
		GENERAL_TRACE_1( TEMPSTR( "Unable to find texture format for %c%c%c%c\n", (fcc>>24)&0xFF, (fcc>>16)&0xFF, (fcc>>8)&0xFF, (fcc>>0)&0xFF ) );
	}
	else {
		GENERAL_TRACE_1( TEMPSTR( "Unable to find texture format for: %d:%d:%d:%d:%d\n", in_format->num_bits(), in_format->num_r_bits(), in_format->num_g_bits(), in_format->num_b_bits(), in_format->num_a_bits() ) );
	}
#endif
	return E_FAIL;
}

//

HRESULT rp_tm_texture_create_system_surface( LPDIRECTDRAW7 lpDD, LPDIRECTDRAWSURFACE7 *to, U32 width, U32 height, U32 lod_num, PixelFormat &pf, U32 irp_cf_flags, U32 rpdd_csf_flags )
{
	ASSERT( to );

	if( *to != NULL ) {
		return S_OK;
	}

	HRESULT hr;
	LPDIRECTDRAWSURFACE7 SS = NULL;

	if( irp_cf_flags & IRP_CTF_STATIC ) {
		rpdd_csf_flags &= ~(RPDD_CSF_DYNAMIC);
	}
	else {
		rpdd_csf_flags |= (RPDD_CSF_DYNAMIC);
	}


	if( irp_cf_flags & IRP_CTF_CM_ALL ) {
		
		rpdd_csf_flags |= (irp_cf_flags & IRP_CTF_CM_POS_X)? RPDD_CSF_CM_POS_X : 0;
		rpdd_csf_flags |= (irp_cf_flags & IRP_CTF_CM_NEG_X)? RPDD_CSF_CM_NEG_X : 0;
		rpdd_csf_flags |= (irp_cf_flags & IRP_CTF_CM_POS_Y)? RPDD_CSF_CM_POS_Y : 0;
		rpdd_csf_flags |= (irp_cf_flags & IRP_CTF_CM_NEG_Y)? RPDD_CSF_CM_NEG_Y : 0;
		rpdd_csf_flags |= (irp_cf_flags & IRP_CTF_CM_POS_Z)? RPDD_CSF_CM_POS_Z : 0;
		rpdd_csf_flags |= (irp_cf_flags & IRP_CTF_CM_NEG_Z)? RPDD_CSF_CM_NEG_Z : 0;
	}


	if( FAILED( hr = rp_dd_create_surface( lpDD, rpdd_csf_flags, DDSCAPS_TEXTURE, width, height, lod_num, &pf, &SS ) ) ) {
		GENERAL_TRACE_1( TEMPSTR( "rp_tm_texture_create_system_surface: unable to create system surface\n" ) );
		return hr;
	}

	*to = SS;

	ASSERT( *to );
	return S_OK;
}

//
struct rp_tm_enum_mip
{
	int lod_num;
	LPDIRECTDRAWSURFACE7 surface;
};

HRESULT WINAPI rp_tm_enum_surface( LPDIRECTDRAWSURFACE7 lpDDSurface, LPDDSURFACEDESC2, LPVOID lpContext )
{
	rp_tm_enum_mip *data = (rp_tm_enum_mip*)lpContext;

	if( --(data->lod_num) <= 0 ) { 
		data->surface = lpDDSurface;
		return DDENUMRET_CANCEL;
	}

	return lpDDSurface->EnumAttachedSurfaces( lpContext, rp_tm_enum_surface ) ;
}

//

HRESULT rp_tm_get_subsurface( LPDIRECTDRAWSURFACE7 surface, U32 subsurface, LPDIRECTDRAWSURFACE7 *out_surface )
{
	LPDIRECTDRAWSURFACE7 top_surface;

	if( subsurface == 0 ) {
		*out_surface = surface;
	}
	
	if( subsurface & RPDD_CSF_CM_ALL ) {

		DDSCAPS2 ddscaps;

		memset( &ddscaps, 0, sizeof(ddscaps) );

	    ddscaps.dwCaps   = DDSCAPS_COMPLEX|DDSCAPS_TEXTURE;
		ddscaps.dwCaps2 |= DDSCAPS2_CUBEMAP;
		ddscaps.dwCaps2 |= (subsurface & RPDD_CSF_CM_POS_X)? DDSCAPS2_CUBEMAP_POSITIVEX : 0;
		ddscaps.dwCaps2 |= (subsurface & RPDD_CSF_CM_NEG_X)? DDSCAPS2_CUBEMAP_NEGATIVEX : 0;
		ddscaps.dwCaps2 |= (subsurface & RPDD_CSF_CM_POS_Y)? DDSCAPS2_CUBEMAP_POSITIVEY : 0;
		ddscaps.dwCaps2 |= (subsurface & RPDD_CSF_CM_NEG_Y)? DDSCAPS2_CUBEMAP_NEGATIVEY : 0;
		ddscaps.dwCaps2 |= (subsurface & RPDD_CSF_CM_POS_Z)? DDSCAPS2_CUBEMAP_POSITIVEZ : 0;
		ddscaps.dwCaps2 |= (subsurface & RPDD_CSF_CM_NEG_Z)? DDSCAPS2_CUBEMAP_NEGATIVEZ : 0;

		if( FAILED( surface->GetAttachedSurface( &ddscaps, &top_surface ) ) ) {

			DDSURFACEDESC2 ddsd;

			memset( &ddsd, 0, sizeof(ddsd) );
			ddsd.dwSize = sizeof(ddsd);

			if( FAILED( surface->GetSurfaceDesc( &ddsd ) ) ) {
				return E_FAIL;
			}

			if( ddsd.ddsCaps.dwCaps2 & ddscaps.dwCaps2 ) {
				// the toplevel surface is the surface requested
				//
				top_surface = surface;
			}
		}
	}
	else {
		top_surface = surface;
	}

	U32 mip_level = subsurface & ~(RPDD_CSF_CM_ALL);

	if( mip_level == 0 ) {
		*out_surface = top_surface;
	}
	else {
		rp_tm_enum_mip data;
		
		data.lod_num = mip_level;
		data.surface = NULL;
		
		if( FAILED( top_surface->EnumAttachedSurfaces( (void*)&data, rp_tm_enum_surface ) ) && data.surface ) {
			return E_FAIL;
		}

		*out_surface = data.surface;
	}

	return S_OK;
}

//

HRESULT rp_tm_texture_set_level_data( LPDIRECTDRAW7 lpDD, LPDIRECTDRAWSURFACE7 to, U32 subsurface, U32 wi, U32 hi, U32 stride, const PixelFormat &inpf, const void *bits1, const unsigned char *bits2, const RGB *color_table )
{
	// create surface if necessary
	//
	if( to == NULL ) {
		ASSERT( 0 );
		return GR_GENERIC;
	}

	DDSURFACEDESC2 ddsd2;
	memset( &ddsd2, 0, sizeof(ddsd2) );
	ddsd2.dwSize = sizeof(ddsd2);
	to->GetSurfaceDesc( &ddsd2 );

	PixelFormat format( ddsd2.ddpfPixelFormat );

	if( format.is_indexed() ) {
		if( color_table != NULL ) {
			rp_dd_surface_set_palette( lpDD, to, 0, 256, (RGB*)color_table );
		}
		else {
			GENERAL_TRACE_1( "rp_tm_texture_set_level_data: color_table is null when texture is palettized\n" );
		}
	}

	LPDIRECTDRAWSURFACE7 surface;

	if( SUCCEEDED( rp_tm_get_subsurface( to, subsurface, &surface ) ) ) {
		rp_dd_surface_bitblt( surface, bits1, wi, hi, stride, inpf, color_table, bits2 );
		return S_OK;
	}
/*
	if( !subsurface ) {
	}
	else {
		rp_tm_enum_mip data;
		data.lod_num = subsurface;
		data.surface = NULL;
		if( SUCCEEDED( to->EnumAttachedSurfaces( (void*)&data, rp_tm_enum_surface ) ) && data.surface ) {
			rp_dd_surface_bitblt( data.surface, bits1, wi, hi, stride, inpf, color_table, bits2 );
		}
	}
*/
	return E_FAIL;
}

//

HRESULT	rp_tm_texture_blit_texture(LPDIRECTDRAWSURFACE7 dst,
								   U32					dst_subsurface,
								   RECT					dstrect,
								   LPDIRECTDRAWSURFACE7 src,
								   U32					src_subsurface,
								   RECT					srcrect)
{
	DDPIXELFORMAT src_ddpf, dst_ddpf;
	PixelFormat src_pf, dst_pf;
	LPDIRECTDRAWSURFACE7 src_surf, dst_surf;
	HRESULT hr;

    dst_ddpf.dwSize	=sizeof(dst_ddpf);
    if( FAILED( hr = dst->GetPixelFormat( &dst_ddpf ) ) ) {
        return	hr;
    }

	dst_pf.init( dst_ddpf );

	src_ddpf.dwSize	=sizeof(src_ddpf);

	if( FAILED( hr = src->GetPixelFormat( &src_ddpf ) ) ) {
		return hr;
	}

	src_pf.init( src_ddpf );

	if( src_pf.is_indexed() && dst_pf.is_indexed() ) {
		LPDIRECTDRAWPALETTE	pal;
		if( FAILED( hr = src->GetPalette( &pal ) ) ) {
			return hr;
		}
		if( FAILED( hr = dst->SetPalette( pal ) ) ) {
			return hr;
		}
	}

	if( FAILED( hr = rp_tm_get_subsurface( dst, dst_subsurface, &dst_surf ) ) ) {
		return hr;
	}

	if( FAILED( hr = rp_tm_get_subsurface( src, src_subsurface, &src_surf ) ) ) {
		return hr;
	}

	if( src_pf.is_equal( dst_pf ) ) {
		return dst_surf->Blt( &dstrect, src_surf, &srcrect, 0, NULL );
	}
	else {
		DDSURFACEDESC2 dst_ddsd, src_ddsd;

		if( FAILED( hr = dst_surf->Lock( NULL, &dst_ddsd, 0, 0 ) ) ) {
			return hr;
		}
		if( FAILED( hr = src_surf->Lock( NULL, &src_ddsd, 0, 0 ) ) ) {
			dst_surf->Unlock( NULL );
			return hr;
		}

		hr = mem_bitblt( dst_ddsd.lpSurface, dst_ddsd.dwWidth, dst_ddsd.dwHeight, dst_ddsd.lPitch, dst_pf,
						 src_ddsd.lpSurface, src_ddsd.dwWidth, src_ddsd.dwHeight, src_ddsd.lPitch, src_pf,
						 NULL, NULL );

		dst_surf->Unlock( NULL );
		src_surf->Unlock( NULL );

		return hr;
	}

	return	S_OK;
}