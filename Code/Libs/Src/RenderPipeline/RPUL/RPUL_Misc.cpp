// RPUL_Misc.cpp
//
//
//

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <string.h>
#include <stdio.h>

#include "FDump.h"
#include "RPUL/RPUL_Misc.h"

#define ID_NULLGUID "{00000000-0000-0000-0000-000000000000}"

static char ID_DefaultDeviceClass[] = "Direct3D";
static char ID_DefaultDeviceId[] = ID_NULLGUID;
static GUID GUID_Null = { 0, 0, 0, {0,0,0,0,0,0,0,0} };

// ------------------------------------------------------------------
//
// Converts GUID into a string like this: "{8BA700E0-2A25-11d2-AE71-0000F4A24D28}"
//
char *ConvertGUIDToString( LPGUID guid, char *string )
{
	if( !guid ) {
		strcpy( string, ID_NULLGUID );	
	}
	else {
		wsprintf( string, "{%08lX-%04X-%04x-%02X%02X-%02X%02X%02X%02X%02X%02X}", 
				  guid->Data1, guid->Data2, guid->Data3, 
				  guid->Data4[0], guid->Data4[1], guid->Data4[2], guid->Data4[3],
				guid->Data4[4], guid->Data4[5], guid->Data4[6], guid->Data4[7] );
	}

	return string;
}


// ------------------------------------------------------------------
//
// Converts string like this: "{8BA700E0-2A25-11d2-AE71-0000F4A24D28}"
// into a GUID.
//
// Note that a GUID of all zeros is equivelent to the NULL guid.
//
LPGUID ConvertStringToGUID( char *string, LPGUID guid )
{
	
	DWORD d1,d2,d3;
	char d4[8], sz[200], s[3], *p, *start;
	
	p = string;
	while( *p && *p!='{' ) p++;
		
	if( !strncmp( string, ID_NULLGUID, strlen(ID_NULLGUID) ) ) {
		memset( guid, 0, sizeof(GUID) );
		return NULL;
	}
	else {
		strcpy( sz, ++p );

		start = p;
		while( *p && ((*p>='0' && *p<='9')||(*p>='a' && *p<='f')||(*p>='A' && *p<='F')) ) p++;
		p++;

		d1 = strtoul( start, NULL, 16 );

		start = p;
		while( *p && ((*p>='0' && *p<='9')||(*p>='a' && *p<='f')||(*p>='A' && *p<='F')) ) p++;
		p++;

		d2 = strtoul( start, NULL, 16 );

		start = p;
		while( *p && ((*p>='0' && *p<='9')||(*p>='a' && *p<='f')||(*p>='A' && *p<='F')) ) p++;
		p++;

		d3 = strtoul( start, NULL, 16 );

		s[0] = *p++;
		s[1] = *p++;
		s[2] = 0;

		d4[0] = (char)strtoul( s, NULL, 16 );

		s[0] = *p++;
		s[1] = *p++;
		s[2] = 0;

		d4[1] = (char)strtoul( s, NULL, 16 );

		p++;

		for( int i=0; i<6; i++ ) {
			s[0] = *p++;
			s[1] = *p++;
			s[2] = 0;

			d4[2+i] = (char)strtoul( s, NULL, 16 );
		}

		guid->Data1 = d1;
		guid->Data2 = (unsigned short)d2;
		guid->Data3 = (unsigned short)d3;
		memcpy( guid->Data4, d4, 8 );

	}
	
	if( memcmp( &guid, &GUID_Null, sizeof(GUID) ) == 0 ) {
		return NULL;
	}

	return guid;
}


// ------------------------------------------------------------------
//
GENRESULT CreateDeviceDescription( const char *device_class, const char *device_id, char *out_desc )
{
	strcpy( out_desc, "{" );
	strcat( out_desc, device_class );
	strcat( out_desc, "," );
	strcat( out_desc, device_id );
	strcat( out_desc, "}" );

	return GR_OK;
}


// ------------------------------------------------------------------
//
GENRESULT GetDeviceClassAndId( const char *interface_desc, char *device_class, char *device_id )
{
	strcpy( device_class, ID_DefaultDeviceClass );
	strcpy( device_id, ID_DefaultDeviceId );

	if( interface_desc == NULL ) {
		return GR_OK;
	}

	char *sz = new char[strlen(interface_desc)+1];
	strcpy( sz, interface_desc );


	// This code attempts to parse a line in the format (items in []'s are optional):
	//
	//[<ws>][<IRenderPipeline>[<ws>]<=>[<ws>]]<open><device_class>[<comma><device_id>]<close>
	//
	// <ws> ::= " " | "\t"
	// <string> ::= Anything but null.
	// <open> ::= "{"
	// <close> ::= "}"
	// <IRenderPipeline> ::= "IRenderPipeline"
	// <=> ::= "="
	// <device_class> ::= <string>
	// <comma> ::= ","
	// <device_id> ::= <string>|
	//
	// If there is no <comma><device_id>, device_id will be VS_DEFAULT_ID
	// <close> represents the *last* '}' found.  i.e. {{aba},{bab}} will return {aba} as the class
	// and {bab} as the id.
	//
	char *p, *rollback, *start;
	const char *start2=NULL;
	p = sz;
		
	// eat pre-whitespace
	//
	while( *p && ((*p==' ') || (*p=='\t')) ) p++;	
	
	// eat interface name
	//
	if( strncmp( p, IID_IRenderPipeline, strlen(IID_IRenderPipeline) ) == 0 ) {
		rollback = p += strlen(IID_IRenderPipeline); 
	}
		
	while( (*p && *p!='\r' && *p!='\n') && (*p==' ' || *p=='\t' || *p=='=') ) p++;	// find start of interface desc
	
	// if we don't find an '=', then we need to start again just after the Interface name 
	if( *p==0 || *p=='\r' || *p=='\n' ) {	
		p = rollback;
	}
		
	// p is the descriptor part of the interface definition
	//
	
	// find start of {foo,bar} pair
	//
	while( (*p && *p!='\r' && *p!='\n') && *p!='{' ) p++;
			
	if( *p && *p!='\r' && *p!='\n' ) { // p is the start of the {foo,bar} part
			
		start = ++p;
				
		char *last_close = NULL;
		while( *p && *p!='\r' && *p!='\n' ) {
			while( (*p && *p!='\r' && *p!='\n') && *p!='}' ) p++;
			if( *p == '}' ) {
				last_close = p++;
			}
		}
		p = last_close;

		if( *p && *p!='\r' && *p!='\n' ) { // p is the end of the {foo,bar} part
					
			*p = 0;					
			p = start;
					
			while( *p && *p!=',' ) p++;
					
			if( *p ) {
				*p = 0;
				start2 = ++p;
			}
			else {
				start2 = ID_DefaultDeviceId;
			}

			strcpy( device_class, start );
			strcpy( device_id, start2 );
		}
	}

	delete[] sz;

	return GR_OK;
}

// ------------------------------------------------------------------
//
GENRESULT GetDeviceInfoInFile( const char *ini_file, char *dclass, char *did)
{
	FILE *in;

	if( (in = fopen( ini_file, "rt" )) == NULL ) {
		return GR_GENERIC;
	}

	char sz[200+1];
	while( fgets( sz, 200, in ) ) {
		if( strstr( sz, IID_IRenderPipeline ) ) {
			GetDeviceClassAndId( sz, dclass, did );
		}
	}

	fclose( in );
	return GR_OK;
}

// ------------------------------------------------------------------
//
GENRESULT SetDeviceInfoInFile( const char *ini_file, const char *device_class, const char *device_id )
{
	FILE *in, *out;

	if( (in = fopen( ini_file, "rt" )) == NULL ) {
		return GR_GENERIC;
	}

	if( (out = fopen( "ini_file.tmp", "wt" )) == NULL ) {
		return GR_GENERIC;
	}

	char sz[200+1], *p;
	while( fgets( sz, 200, in ) ) {
		p = sz;
		while( *p && ((*p==' ') || (*p=='\t')) ) p++;	
		if( strncmp( p, IID_IRenderPipeline, strlen(IID_IRenderPipeline) ) == 0 ) {
			*p = 0;
			fprintf( out, "%s%s = {%s,%s}\r\n", sz, IID_IRenderPipeline, device_class, device_id );
		}
		else {
			fprintf( out, "%s", sz );
		}
	}

	fclose( in );
	fclose( out );

	unlink( ini_file );

	if( (in = fopen( "ini_file.tmp", "rt" )) == NULL ) {
		return GR_GENERIC;
	}

	if( (out = fopen( ini_file, "wt" )) == NULL ) {
		return GR_GENERIC;
	}

	while( fgets( sz, 200, in ) ) {
		fprintf( out, "%s", sz );
	}

	fclose( in );
	fclose( out );

	unlink( "ini_file.tmp" );

	return GR_OK;
}

inline void rp_dd_surface_put_pixel( U8 *dst_bits, PixelFormat &dst_format, U8 r, U8 g, U8 b, U8 a )
{
#if 0
	switch( dst_format.num_bits() ) {
	case 32: dst_bits[3] = ((char*)&dst_pxl)[3];	
	case 24: dst_bits[2] = ((char*)&dst_pxl)[2];	
	case 16: dst_bits[1] = ((char*)&dst_pxl)[1];	
	case  8: dst_bits[0] = ((char*)&dst_pxl)[0];	
	}
#else
	U32 dst_pxl = dst_format.compute( r,g,b,a );		
	switch( dst_format.num_bits() ) {
	
	case 32: 	dst_bits[3] = ((char*)&dst_pxl)[3];
	case 24: 	dst_bits[2] = ((char*)&dst_pxl)[2];
				dst_bits[1] = ((char*)&dst_pxl)[1];
				dst_bits[0] = ((char*)&dst_pxl)[0];
				break;

	case  8: 	dst_bits[0] = r;	
				break;

	case 16:	*((U16*)dst_bits) = (U16)dst_pxl;
				break;
	}
#endif
}

HRESULT mem_bitblt(	void *dst_bits,
						int dst_width,
						int dst_height,
						int dst_stride,
						PixelFormat & dst_format,
						const void *src_bits,
						int src_width,
						int src_height,
						int src_stride,
						const PixelFormat & src_format_in,
						const RGB *src_palette,
						const U8 *src_alpha )
{
	U32			 dst_bd = ((dst_format.ddpf.dwRGBBitCount+7)>>3);	// dst byte depth
	U8			*dst_bits_u8 = (U8*)dst_bits;
	U32			 dst_past_stride = dst_stride - dst_width * dst_bd;  // dst_past_stride is the number of bytes *beyond* the width of the dst surface

	PixelFormat  src_format( src_format_in );
	U8			*src_bits_u8 = (U8*)src_bits;
	U8			*src_bits_line = NULL;
	U32			 src_alpha_stride;
	const U8	*src_alpha_map = NULL;
	U32			 src_bd = ((src_format.ddpf.dwRGBBitCount+7)>>3);		// src byte depth
	U32			 src_pxl_u32;
	U8			*src_bits_u8_p;

	U8 r,g,b,a = 0;

	const int max_alpha_width = 2048;
	static bool init_default_alpha_map = false;
	static U8 default_alpha_map[max_alpha_width];

	U32 r_mask = src_format.get_r_mask();
	U32 g_mask = src_format.get_g_mask();
	U32 b_mask = src_format.get_b_mask();
	U32 a_mask = src_format.get_a_mask();

	// Fixup 8bit true textures to pull r g & b out of r
	//
	if( src_format.num_r_bits() == 8 && src_format.num_g_bits() == 0 && src_format.num_b_bits() == 0 && !src_format.is_indexed() ) {
		g_mask			= b_mask		= r_mask;
		src_format.gl	= src_format.bl = src_format.rl;
		src_format.gr	= src_format.br = src_format.rr;
	}
	
	// Set up source alpha map
	//
	if( src_alpha ) {
		src_alpha_map = src_alpha;
		src_alpha_stride = src_width;
	}
	else {
		if( a_mask == 0 && init_default_alpha_map == false ) {
			ASSERT( src_width < max_alpha_width );
			init_default_alpha_map = true; 
			memset( default_alpha_map, 0xFF, max_alpha_width );
		}
		src_alpha_map = default_alpha_map;
		src_alpha_stride = 0;
	}


	if( dst_width == src_width && dst_height == src_height ) {
		
		if( dst_format.is_equal( src_format ) && dst_format.num_bits() == src_format.num_bits() ) {
			if( dst_stride == src_stride ) {
				// Do block copy
				//
				memcpy( dst_bits_u8, src_bits_u8, src_stride*src_height );
			}
			else {
				// Do block copy, mind the pitch
				//
				for( int y=0; y<dst_height; y++ ) {
					memcpy( dst_bits_u8, src_bits_u8, src_width*src_bd );
					src_bits_u8 += src_stride;
					dst_bits_u8 += dst_stride;
				}
			}
		}
		else {
			// Do copy with format conversion
			//

			// change src_stride to be the number of bytes 
			// *beyond* the width of the src surface
			//
            src_stride -= src_width * src_bd;						

			if( src_format.is_indexed() ) {
				// Indexed source to RGBA 8/16/24/32 bit dest.
				//
				for( int y=0; y < dst_height; y++ ) {
										
					for( int x=0; x < dst_width; x++ ) {
						r = src_palette[*src_bits_u8].r;
						g = src_palette[*src_bits_u8].g;
						b = src_palette[*src_bits_u8].b;
						a = src_alpha_map[x];
						src_bits_u8++;	

						rp_dd_surface_put_pixel( dst_bits_u8, dst_format, r, g, b, a );
						dst_bits_u8 += dst_bd;					
					}

					src_alpha_map += src_alpha_stride;
					src_bits_u8 += src_stride;
					dst_bits_u8 += dst_past_stride;
				}
			}
			else {
				// 8/16/24/32 bit source to 8/16/24/32 bit dest
				//
				for(int y=0;y < dst_height-1;y++)
				{
					for(int x=0;x < dst_width;x++)
					{
						a			=src_alpha_map[x];
						src_pxl_u32	=*((U32*)src_bits_u8);

						r	=((src_pxl_u32 & r_mask) >> src_format.rl);
						r	=(r << src_format.rr) | (r & ((1<<src_format.rr)-1));
						g	=((src_pxl_u32 & g_mask) >> src_format.gl);
						g	=(g << src_format.gr) | (g & ((1<<src_format.gr)-1));
						b	=((src_pxl_u32 & b_mask) >> src_format.bl);
						b	=(b << src_format.br) | (b & ((1<<src_format.br)-1));

						if(a_mask)
						{
							a	=((src_pxl_u32 & a_mask) >> src_format.al);
							a	=(a << src_format.ar) | (a & ((1<<src_format.ar)-1));
						}
						
						src_bits_u8	+=src_bd;
						
						rp_dd_surface_put_pixel(dst_bits_u8, dst_format, r, g, b, a);
						dst_bits_u8	+=dst_bd;
					}

					src_alpha_map	+=src_alpha_stride;
					src_bits_u8		+=src_stride;
					dst_bits_u8		+=dst_past_stride;
				}

				//last span needs to avoid the dword read
				for(int x=0;x < dst_width;x++)
				{
					a	=src_alpha_map[x];

					for(unsigned int i=0;i < src_bd;i++)
					{
						*(((U8 *)&src_pxl_u32)+i)	=*(src_bits_u8+i);
					}

					r	=((src_pxl_u32 & r_mask) >> src_format.rl);
					r	=(r << src_format.rr) | (r & ((1<<src_format.rr)-1));
					g	=((src_pxl_u32 & g_mask) >> src_format.gl);
					g	=(g << src_format.gr) | (g & ((1<<src_format.gr)-1));
					b	=((src_pxl_u32 & b_mask) >> src_format.bl);
					b	=(b << src_format.br) | (b & ((1<<src_format.br)-1));

					if(a_mask)
					{
						a	=((src_pxl_u32 & a_mask) >> src_format.al);
						a	=(a << src_format.ar) | (a & ((1<<src_format.ar)-1));
					}
					
					src_bits_u8	+=src_bd;
					
					rp_dd_surface_put_pixel(dst_bits_u8, dst_format, r, g, b, a);
					dst_bits_u8	+=dst_bd;
				}

				src_alpha_map	+=src_alpha_stride;
				src_bits_u8		+=src_stride;
				dst_bits_u8		+=dst_past_stride;
			}
		}

	}
	else {	

		// Do stretch/shrink with format conversion
		//

		U32  u = 0, u32;
		U32  v = 0, v32;
		U32 du = (U32)( (float(src_width)  / float(dst_width)) * float(1<<16) );
		U32 dv = (U32)( (float(src_height)  / float(dst_height)) * float(1<<16) );

		if( src_format.is_indexed() ) {
			// Indexed source to RGBA 8/16/24/32 bit dest.
			//
			for( int y=0; y < dst_height; y++, v+=dv ) {

				v32 = (v>>16);
				src_bits_line = &src_bits_u8[src_stride * v32];
				u = 0;

				for( int x=0; x < dst_width; x++, u+=du ) {

					u32 = (u>>16);
					src_bits_u8_p = &src_bits_line[ src_bd * u32 ];

					r = src_palette[*src_bits_u8_p].r;
					g = src_palette[*src_bits_u8_p].g;
					b = src_palette[*src_bits_u8_p].b;
					a = src_alpha_map[x];

					rp_dd_surface_put_pixel( dst_bits_u8, dst_format, r, g, b, a );
					dst_bits_u8 += dst_bd;					
				}

				src_alpha_map += src_alpha_stride;
				dst_bits_u8 += dst_past_stride;
			}
		}
		else {
			// RGBA 8/16/24/32 bit src to RGBA 8/16/24/32 bit dest.
			//

			for( int y=0; y<dst_height-1; y++, v+=dv ) {
				
				v32 = (v>>16);
				src_bits_line = &src_bits_u8[src_stride * v32];
				u = 0;

				for( int x=0; x<dst_width; x++, u+=du ) {

					u32 = (u>>16);

					a = src_alpha_map[x];

					src_pxl_u32 = *((U32*)&src_bits_line[ src_bd * u32 ]);

					r = ((src_pxl_u32 & r_mask) >> src_format.rl);
					r = (r << src_format.rr) | (r & ((1<<src_format.rr)-1));
					g = ((src_pxl_u32 & g_mask) >> src_format.gl);
					g = (g << src_format.gr) | (g & ((1<<src_format.gr)-1));
					b = ((src_pxl_u32 & b_mask) >> src_format.bl);
					b = (b << src_format.br) | (b & ((1<<src_format.br)-1));
					if( a_mask ) {
						a = ((src_pxl_u32 & a_mask) >> src_format.al);
						a = (a << src_format.ar) | (a & ((1<<src_format.ar)-1));
					}

					rp_dd_surface_put_pixel( dst_bits_u8, dst_format, r, g, b, a );
					dst_bits_u8 += dst_bd;					
				}

				src_alpha_map += src_alpha_stride;
				dst_bits_u8 += dst_past_stride;
			} 

			//last line avoid dword read
			v32 = (v>>16);
			src_bits_line = &src_bits_u8[src_stride * v32];
			u = 0;

			for( int x=0; x<dst_width; x++, u+=du ) {

				u32 = (u>>16);

				a = src_alpha_map[x];

				for(unsigned int i=0;i < src_bd;i++)
				{
					*(((U8 *)&src_pxl_u32)+i)	=src_bits_line[u32+i];
				}
//				src_pxl_u32 = *((U32*)&src_bits_line[ src_bd * u32 ]);

				r = ((src_pxl_u32 & r_mask) >> src_format.rl);
				r = (r << src_format.rr) | (r & ((1<<src_format.rr)-1));
				g = ((src_pxl_u32 & g_mask) >> src_format.gl);
				g = (g << src_format.gr) | (g & ((1<<src_format.gr)-1));
				b = ((src_pxl_u32 & b_mask) >> src_format.bl);
				b = (b << src_format.br) | (b & ((1<<src_format.br)-1));
				if( a_mask ) {
					a = ((src_pxl_u32 & a_mask) >> src_format.al);
					a = (a << src_format.ar) | (a & ((1<<src_format.ar)-1));
				}

				rp_dd_surface_put_pixel( dst_bits_u8, dst_format, r, g, b, a );
				dst_bits_u8 += dst_bd;					
			}

			src_alpha_map += src_alpha_stride;
			dst_bits_u8 += dst_past_stride;
		}
	}	

	return S_OK;
}


HRESULT mem_bitblt_invert(	void *dst_bits,
						int dst_width,
						int dst_height,
						int dst_stride,
						PixelFormat & dst_format,
						const void *src_bits,
						int src_width,
						int src_height,
						int src_stride,
						const PixelFormat & src_format_in,
						const RGB *src_palette,
						const U8 *src_alpha )
{
	U32			 dst_bd = ((dst_format.ddpf.dwRGBBitCount+7)>>3);	// dst byte depth
	U8			*dst_bits_u8 = (U8*)dst_bits;
	U32			 dst_past_stride = dst_stride - dst_width * dst_bd;  // dst_past_stride is the number of bytes *beyond* the width of the dst surface

	PixelFormat  src_format( src_format_in );
	U8			*src_bits_u8 = (U8*)src_bits + (src_stride * (src_height-1));	//adjust to last line
	U8			*src_bits_line = NULL;
	U32			 src_alpha_stride;
	const U8	*src_alpha_map = NULL;
	U32			 src_bd = ((src_format.ddpf.dwRGBBitCount+7)>>3);		// src byte depth
	U32			 src_pxl_u32;
	U8			*src_bits_u8_p;

	U8 r,g,b,a = 0;

	const int max_alpha_width = 2048;
	static bool init_default_alpha_map = false;
	static U8 default_alpha_map[max_alpha_width];

	U32 r_mask = src_format.get_r_mask();
	U32 g_mask = src_format.get_g_mask();
	U32 b_mask = src_format.get_b_mask();
	U32 a_mask = src_format.get_a_mask();

	// Fixup 8bit true textures to pull r g & b out of r
	//
	if( src_format.num_r_bits() == 8 && src_format.num_g_bits() == 0 && src_format.num_b_bits() == 0 && !src_format.is_indexed() ) {
		g_mask			= b_mask		= r_mask;
		src_format.gl	= src_format.bl = src_format.rl;
		src_format.gr	= src_format.br = src_format.rr;
	}
	
	// Set up source alpha map
	//
	if( src_alpha ) {
		src_alpha_map = src_alpha + (src_stride * (src_height-1));	//adjust to last line
		src_alpha_stride = src_width;
	}
	else {
		if( a_mask == 0 && init_default_alpha_map == false ) {
			ASSERT( src_width < max_alpha_width );
			init_default_alpha_map = true; 
			memset( default_alpha_map, 0xFF, max_alpha_width );
		}
		src_alpha_map = default_alpha_map;
		src_alpha_stride = 0;
	}


	if(dst_width == src_width && dst_height == src_height)
	{
		if(dst_format.is_equal(src_format) && dst_format.num_bits() == src_format.num_bits())
		{
			for(int y = 0;y < dst_height;y++)
			{
				memcpy(dst_bits_u8, src_bits_u8, src_width * src_bd);
				src_bits_u8	-=src_stride;
				dst_bits_u8	+=dst_stride;
			}
		}
		else
		{
			// Do copy with format conversion
			//

			// change src_stride to be the number of bytes 
			// *beyond* the width of the src surface + original stride
			//
            src_stride += src_width * src_bd;						

			if( src_format.is_indexed() ) {
				// Indexed source to RGBA 8/16/24/32 bit dest.
				//
				for( int y=0; y < dst_height; y++ ) {
										
					for( int x=0; x < dst_width; x++ ) {
						r = src_palette[*src_bits_u8].r;
						g = src_palette[*src_bits_u8].g;
						b = src_palette[*src_bits_u8].b;
						a = src_alpha_map[x];
						src_bits_u8++;	

						rp_dd_surface_put_pixel( dst_bits_u8, dst_format, r, g, b, a );
						dst_bits_u8 += dst_bd;					
					}

					src_alpha_map -= src_alpha_stride;
					src_bits_u8 -= src_stride;
					dst_bits_u8 += dst_past_stride;
				}
			}
			else {
				// 8/16/24/32 bit source to 8/16/24/32 bit dest
				//
				for(int y=0;y < dst_height-1;y++)
				{
					for(int x=0;x < dst_width;x++)
					{
						a			=src_alpha_map[x];
						src_pxl_u32	=*((U32*)src_bits_u8);

						r	=((src_pxl_u32 & r_mask) >> src_format.rl);
						r	=(r << src_format.rr) | (r & ((1<<src_format.rr)-1));
						g	=((src_pxl_u32 & g_mask) >> src_format.gl);
						g	=(g << src_format.gr) | (g & ((1<<src_format.gr)-1));
						b	=((src_pxl_u32 & b_mask) >> src_format.bl);
						b	=(b << src_format.br) | (b & ((1<<src_format.br)-1));

						if(a_mask)
						{
							a	=((src_pxl_u32 & a_mask) >> src_format.al);
							a	=(a << src_format.ar) | (a & ((1<<src_format.ar)-1));
						}
						
						src_bits_u8	+=src_bd;
						
						rp_dd_surface_put_pixel(dst_bits_u8, dst_format, r, g, b, a);
						dst_bits_u8	+=dst_bd;
					}

					src_alpha_map	-=src_alpha_stride;
					src_bits_u8		-=src_stride;
					dst_bits_u8		+=dst_past_stride;
				}

				//last span needs to avoid the dword read
				for(int x=0;x < dst_width;x++)
				{
					a	=src_alpha_map[x];

					for(unsigned int i=0;i < src_bd;i++)
					{
						*(((U8 *)&src_pxl_u32)+i)	=*(src_bits_u8+i);
					}

					r	=((src_pxl_u32 & r_mask) >> src_format.rl);
					r	=(r << src_format.rr) | (r & ((1<<src_format.rr)-1));
					g	=((src_pxl_u32 & g_mask) >> src_format.gl);
					g	=(g << src_format.gr) | (g & ((1<<src_format.gr)-1));
					b	=((src_pxl_u32 & b_mask) >> src_format.bl);
					b	=(b << src_format.br) | (b & ((1<<src_format.br)-1));

					if(a_mask)
					{
						a	=((src_pxl_u32 & a_mask) >> src_format.al);
						a	=(a << src_format.ar) | (a & ((1<<src_format.ar)-1));
					}
					
					src_bits_u8	+=src_bd;
					
					rp_dd_surface_put_pixel(dst_bits_u8, dst_format, r, g, b, a);
					dst_bits_u8	+=dst_bd;
				}

				src_alpha_map	-=src_alpha_stride;
				src_bits_u8		-=src_stride;
				dst_bits_u8		+=dst_past_stride;
			}
		}

	}
	else {	

		// Do stretch/shrink with format conversion
		//

		U32  u = 0, u32;
		U32  v = src_height - 1, v32;
		U32 du = (U32)( (float(src_width)  / float(dst_width)) * float(1<<16) );
		U32 dv = (U32)( (float(src_height)  / float(dst_height)) * float(1<<16) );

		if( src_format.is_indexed() ) {
			// Indexed source to RGBA 8/16/24/32 bit dest.
			//
			for( int y=0; y < dst_height; y++, v+=dv ) {

				v32 = (v>>16);
				src_bits_line = &src_bits_u8[src_stride * v32];
				u = 0;

				for( int x=0; x < dst_width; x++, u+=du ) {

					u32 = (u>>16);
					src_bits_u8_p = &src_bits_line[ src_bd * u32 ];

					r = src_palette[*src_bits_u8_p].r;
					g = src_palette[*src_bits_u8_p].g;
					b = src_palette[*src_bits_u8_p].b;
					a = src_alpha_map[x];

					rp_dd_surface_put_pixel( dst_bits_u8, dst_format, r, g, b, a );
					dst_bits_u8 += dst_bd;					
				}

				src_alpha_map -= src_alpha_stride;
				dst_bits_u8 += dst_past_stride;
			}
		}
		else {
			// RGBA 8/16/24/32 bit src to RGBA 8/16/24/32 bit dest.
			//

			for( int y=0; y<dst_height-1; y++, v+=dv ) {
				
				v32 = (v>>16);
				src_bits_line = &src_bits_u8[src_stride * v32];
				u = 0;

				for( int x=0; x<dst_width; x++, u+=du ) {

					u32 = (u>>16);

					a = src_alpha_map[x];

					src_pxl_u32 = *((U32*)&src_bits_line[ src_bd * u32 ]);

					r = ((src_pxl_u32 & r_mask) >> src_format.rl);
					r = (r << src_format.rr) | (r & ((1<<src_format.rr)-1));
					g = ((src_pxl_u32 & g_mask) >> src_format.gl);
					g = (g << src_format.gr) | (g & ((1<<src_format.gr)-1));
					b = ((src_pxl_u32 & b_mask) >> src_format.bl);
					b = (b << src_format.br) | (b & ((1<<src_format.br)-1));
					if( a_mask ) {
						a = ((src_pxl_u32 & a_mask) >> src_format.al);
						a = (a << src_format.ar) | (a & ((1<<src_format.ar)-1));
					}

					rp_dd_surface_put_pixel( dst_bits_u8, dst_format, r, g, b, a );
					dst_bits_u8 += dst_bd;					
				}

				src_alpha_map -= src_alpha_stride;
				dst_bits_u8 += dst_past_stride;
			} 

			//last line avoid dword read
			v32 = (v>>16);
			src_bits_line = &src_bits_u8[src_stride * v32];
			u = 0;

			for( int x=0; x<dst_width; x++, u+=du ) {

				u32 = (u>>16);

				a = src_alpha_map[x];

				for(unsigned int i=0;i < src_bd;i++)
				{
					*(((U8 *)&src_pxl_u32)+i)	=src_bits_line[u32+i];
				}
//				src_pxl_u32 = *((U32*)&src_bits_line[ src_bd * u32 ]);

				r = ((src_pxl_u32 & r_mask) >> src_format.rl);
				r = (r << src_format.rr) | (r & ((1<<src_format.rr)-1));
				g = ((src_pxl_u32 & g_mask) >> src_format.gl);
				g = (g << src_format.gr) | (g & ((1<<src_format.gr)-1));
				b = ((src_pxl_u32 & b_mask) >> src_format.bl);
				b = (b << src_format.br) | (b & ((1<<src_format.br)-1));
				if( a_mask ) {
					a = ((src_pxl_u32 & a_mask) >> src_format.al);
					a = (a << src_format.ar) | (a & ((1<<src_format.ar)-1));
				}

				rp_dd_surface_put_pixel( dst_bits_u8, dst_format, r, g, b, a );
				dst_bits_u8 += dst_bd;					
			}

			src_alpha_map -= src_alpha_stride;
			dst_bits_u8 += dst_past_stride;
		}
	}	

	return S_OK;
}
