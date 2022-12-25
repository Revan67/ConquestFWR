//
//
//

#ifndef PIXEL_H
#define PIXEL_H

//

#include "typedefs.h"
#include <ddraw.h>
//

typedef unsigned char	byte;
typedef unsigned short	word;
typedef unsigned long	dword;

//

struct PixelFormat
{
	DDPIXELFORMAT	ddpf;
	int				rr, rl, rwidth;
	int				gr, gl, gwidth;
	int				br, bl, bwidth;
	int				ar, al, awidth;

	void init(DDPIXELFORMAT format)
	{
		ddpf = format;
		if (!is_indexed())
		{
			U32 rmask = format.dwRBitMask;
			U32 gmask = format.dwGBitMask;
			U32 bmask = format.dwBBitMask;
			U32 amask = format.dwRGBAlphaBitMask;

			for (int i = 31; i >= 0; i--)
			{
				if (rmask & (1 << i))
				{
					rl = i;
				}
				if (gmask & (1 << i))
				{
					gl = i;
				}
				if (bmask & (1 << i))
				{
					bl = i;
				}
				if (amask & (1 << i))
				{
					al = i;
				}
			}

			for (i = 0; i <= 31; i++)
			{
				if (rmask & (1 << i))
				{
					rwidth = i - rl + 1;
				}

				if (gmask & (1 << i))
				{
					gwidth = i - gl + 1;
				}

				if (bmask & (1 << i))
				{
					bwidth = i - bl + 1;
				}

				if (amask & (1 << i))
				{
					awidth = i - al + 1;
				}
			}

			rr = 8 - rwidth;
			gr = 8 - gwidth;
			br = 8 - bwidth;
			ar = 8 - awidth;

			if (!has_alpha_channel())
			{
				al = ar = awidth = 0;
			}
		}
	}

	inline dword compute(byte r, byte g, byte b, byte a = 0) const
	{
		dword result;
		if (is_indexed())
		{
			result = 0xffffffff;
		}
		else
		{
			result = (((r >> rr) << rl) |
					  ((g >> gr) << gl) |
					  ((b >> br) << bl));
			if (awidth)
			{
				result |= ((a >> ar) << al);
			}
		}
		return result;
	}

	inline bool is_indexed(void) const
	{
		return ((ddpf.dwFlags & DDPF_PALETTEINDEXED8) != 0);
	}

	inline bool has_alpha_channel(void) const
	{
		return ((ddpf.dwFlags & DDPF_ALPHAPIXELS) != 0);
	}

	inline dword get_r_mask(void) const
	{
		return ddpf.dwRBitMask;
	}

	inline dword get_g_mask(void) const
	{
		return ddpf.dwGBitMask;
	}

	inline dword get_b_mask(void) const
	{
		return ddpf.dwBBitMask;
	}

	inline dword get_a_mask(void) const
	{
		return ddpf.dwRGBAlphaBitMask;
	}

	inline dword num_r_bits(void) const
	{
		return rwidth;
	}

	inline dword num_g_bits(void) const
	{
		return gwidth;
	}

	inline dword num_b_bits(void) const
	{
		return bwidth;
	}

	inline dword num_a_bits(void) const
	{
		return awidth;
	}

	// determine if the given pixelformat is the same as this one.
	inline dword is_equal( const PixelFormat &pf )
	{
		dword yup = 0;

		if( pf.is_indexed() ) {
			return is_indexed();
		}
		else if( !is_indexed() ) {
			
			if( (num_r_bits() == pf.num_r_bits()) &&
				(get_r_mask() == pf.get_r_mask()) &&
				(num_g_bits() == pf.num_g_bits()) &&
				(get_g_mask() == pf.get_g_mask()) &&
				(num_b_bits() == pf.num_b_bits()) &&
				(get_b_mask() == pf.get_b_mask()) &&
				(num_a_bits() == pf.num_a_bits()) && 
				(get_a_mask() == pf.get_a_mask()) ) {
				
				return 1;

			}
		}

		return 0;
	}

	// determine if this pixelformat is compatible with
	// the given one.  This pixelformat is 'compatible' 
	// iff this pixel format can describe a surface that
	// can contain pixeldata in the given format without
	// loss of data.
	// i.e. An RGB pixelformat is compatible with an indexed 
	// format because an RGB surface can hold indexed data
	// without loss of data.  However, an indexed format
	// is not compatible with an RGB format because an
	// indexed surface cannot hold RGB data without quantization
	//
	inline dword is_compatible( const PixelFormat &pf )
	{
		dword yup = 0;

		if( is_indexed() && pf.is_indexed() ) {		
			// both indexed
			return 1;
		}
		else if( (!is_indexed() && pf.is_indexed()) ) { //&& !has_alpha_channel()) ) {
			// this is rgb, that is indexed
			return 1;
		}
		else {
#if 0
			// both are rgb(a), this must have at least as many bits as that.
			// if that has alpha, this must have it too and at least as many
			// bits.
			yup =	(num_r_bits() >= pf.num_r_bits()) &&
					(num_g_bits() >= pf.num_g_bits()) &&
					(num_b_bits() >= pf.num_b_bits()) ;
			
			if( yup && pf.has_alpha_channel() ) {
				return yup && (num_a_bits() >= pf.num_a_bits());
			}
#endif
			return (has_alpha_channel() == pf.has_alpha_channel());
		}

		return 0;
	}

	PixelFormat()
	{
		memset( &ddpf, 0, sizeof(ddpf) );
		ddpf.dwSize = sizeof(ddpf);
		rr = rl = rwidth = 0;
		gr = gl = gwidth = 0;
		br = bl = bwidth = 0;
		ar = al = awidth = 0;
	}

	inline dword extract(char *src, byte r, byte g, byte b, byte a = 0) const
	{
		dword result;
		if (is_indexed())
		{
			result = 0xffffffff;
		}
		else
		{
			result = (((r >> rr) << rl) |
					  ((g >> gr) << gl) |
					  ((b >> br) << bl));
			if (awidth)
			{
				result |= ((a >> ar) << al);
			}
		}
		return result;
	}

	void init( U32 bpp, U32 rbits, U32 gbits, U32 bbits, U32 abits )
	{
		DDPIXELFORMAT _ddpf;
		
		memset( &_ddpf,0,sizeof(_ddpf) );
		_ddpf.dwSize = sizeof(_ddpf);

		if( bpp < 15 ) {
			_ddpf.dwFlags = DDPF_PALETTEINDEXED8;
			_ddpf.dwRGBBitCount = 8;
		}
		else {
			_ddpf.dwFlags = DDPF_RGB;
			_ddpf.dwRGBBitCount = bpp;
			_ddpf.dwRBitMask = ((1<<rbits)-1)<<(gbits+bbits);
			_ddpf.dwGBitMask = ((1<<gbits)-1)<<(bbits);
			_ddpf.dwBBitMask = ((1<<bbits)-1);

			if( abits ) {
				_ddpf.dwFlags |= DDPF_ALPHAPIXELS;
				_ddpf.dwRGBAlphaBitMask = ((1<<abits)-1)<<(rbits+gbits+bbits);
			}
		}

		init( _ddpf );
	}

	PixelFormat( DDPIXELFORMAT _ddpf )
	{
		init(_ddpf);
	}

	PixelFormat( U32 bpp, U32 rbits, U32 gbits, U32 bbits, U32 abits )
	{
		init(bpp,rbits,gbits,bbits,abits);
	}

	inline dword num_bits(void) const
	{
		return ddpf.dwRGBBitCount;
	}


};

//

#endif