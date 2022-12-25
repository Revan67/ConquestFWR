// DirectDrawTexture.h
//
// DirectDraw Texture Manager Utility functions
// 
//


#ifndef DA_DIRECTDRAWTEXTURE_H
#define DA_DIRECTDRAWTEXTURE_H

//

#define WINDOWS_LEAN_AND_MEAN
#include <windows.h>
#include <ddraw.h>
#include <d3d9.h>

#include "typedefs.h"
#include "pixel.h"
#include "rendpipeline.h"
#include "rpul.h"
#include "IVideoStreamControl.h"

#include "DirectDraw.h"


//

struct RPTEXTUREFORMATCLASS
{
	RPTEXTUREFORMATCLASS( U32 _fourcc = 0 )
	{
		fourcc = _fourcc;
		num_formats = 0;
		formats = NULL;
	}

	//

	RPTEXTUREFORMATCLASS( const RPTEXTUREFORMATCLASS &tfc )
	{
		fourcc = tfc.fourcc;
		num_formats = tfc.num_formats;
		formats = tfc.formats;
				
		const_cast<RPTEXTUREFORMATCLASS&>(tfc).formats = NULL;	// take ownership
	}

	//

	~RPTEXTUREFORMATCLASS()
	{
		clear_formats();
	}

	//

	void clear_formats( void )
	{
		delete[] formats;
		formats = NULL;
		num_formats = 0;
	}

	//

	void add_format( PixelFormat &format )
	{
		PixelFormat *newpf;

		if( (newpf = new PixelFormat[ num_formats + 1 ]) != NULL ) {
			if( num_formats ) {
				memcpy( newpf, formats, num_formats * sizeof(PixelFormat) );
				delete[] formats;
			}
			formats = newpf;
			formats[num_formats].init( format );
			num_formats++;
		}
	}

	//

	U32 fourcc;		// fourcc code
	U32 num_formats;
	PixelFormat *formats;

};


//
// Texture Related Stuff
//

HRESULT rp_tm_texture_create_system_surface		( LPDIRECTDRAW7 lpDD, LPDIRECTDRAWSURFACE7 *out_texture_object, U32 width, U32 height, U32 lod, PixelFormat &pf, U32 irp_ctf_flags, U32 rpdd_csf_flags );
HRESULT rp_tm_texture_set_level_data			( LPDIRECTDRAW7 lpDD, LPDIRECTDRAWSURFACE7 surface, U32 level, U32 wi, U32 hi, U32 stride, const PixelFormat &inpf, const void *bits1, const unsigned char *bits2, const RGB *color_table );
												
HRESULT rp_tm_texture_adjust_dims				( U32 *inout_width, U32 *inout_height, U32 *inout_lod_count );
HRESULT rp_tm_texture_choose_format				( PixelFormat *in_format, RPTEXTUREFORMATCLASS *texture_format_classes, U32 num_texture_format_classes, PixelFormat *texture_formats, U32 num_texture_formats, U32 flags, PixelFormat *out_format );
HRESULT rp_tm_get_subsurface					( LPDIRECTDRAWSURFACE7 surface, U32 subsurface, LPDIRECTDRAWSURFACE7 *out_surface );
HRESULT	rp_tm_texture_blit_texture				( LPDIRECTDRAWSURFACE7 dst, U32 dst_subsurface, RECT dstrect, LPDIRECTDRAWSURFACE7 src, U32 src_subsurface, RECT srcrect);

#endif
