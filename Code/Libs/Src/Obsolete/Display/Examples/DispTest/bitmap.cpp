#include "project.h"
#include "bitmap.h"
#include "file.h"

typedef unsigned char byte;
typedef unsigned short word;
typedef unsigned long dword;

#define GET_BYTE(ptr) (*(byte *)(ptr))
#define GET_WORD(ptr) (*(word *)(ptr))
#define GET_DWORD(ptr) (*(dword *)(ptr))

struct E1
{
	byte value[1];
	operator int (void)
	{
		return *(byte *)value;
	}
};
struct E2
{
	byte value[2];
	operator int (void)
	{
		return *(word *)value;
	}
};
struct E4
{
	byte value[4];
	operator int (void)
	{
		return *(dword *)value;
	}
};


struct TGA_Header
// TARGA FILE HEADER = 18 bytes
{
	E1 identsize;             // size of ID field that follows 18 byte header (0 usually)
	E1 palette;				// type of color map 0=none, 1=has palette
	E1 imagetype;             // type of image 0=none,1=indexed,2=rgb,3=grey,+8=rle packed

	E2 color_start;			// first color map entry in palette
	E2 num_palette_colors;	// number of colors in palette
	E1 bits_per_color;		// number of bits per palette entry 15,16,24,32

	E2 xstart;             // image x origin
	E2 ystart;             // image y origin
	E2 width;              // image width in pixels
	E2 height;             // image height in pixels
	E1 bits_per_pixel;		// image bits per pixel 8,16,24,32
	E1 descriptor;			// image descriptor bits (vh flip bits)

	int verify (void)
	{
		if (
			(palette <= 1) &&
			(imagetype & ~8) <= 3 && 
			(
			bits_per_pixel== 8 ||
			bits_per_pixel==15 || bits_per_pixel==16 ||
			bits_per_pixel==24 || bits_per_pixel==32
			)
		   )
		{
			return TRUE;
		}
		return FALSE;
	}

// MISCELLANEOUS

	int is_packed (void)
	{
		return (imagetype & 8);
	}

	int is_x_flipped (void)
	{
		return (descriptor & 0x10);
	}

	int is_y_flipped (void)
	{
		return !(descriptor & 0x20);
	}

// Note: the TGA format has redundant information on whether or
// not a palette exists... has_palette,is_indexed,num_colors==0

	int has_palette (void)
	{
		return (palette == 1);
	}
	int is_indexed (void)
	{
		return (imagetype & 7) == 1;
	}

// GREY SCALE is a unusual feature to avoid storing the palette!

	int is_grey (void)
	{
		return (imagetype & 7) == 3;
	}

// an 8-bit ALPHA can be stored in the palette or individual pixels

	int is_alpha (void)
	{
		int a = 0;
		if (is_indexed())
		{
			if (bits_per_color == 32)
				a = 8;
		}
		else if (bits_per_pixel == 32)
		{
			a = 8;
		}
		return (a > 0);
	}

	int num_colors (void)
	{
		int colors = 0;
		if (is_indexed())
		{
			colors = color_start + num_palette_colors;
		}
		else if (is_grey())
		{
			colors = 256; // fixed RGB palette
		}
		return (colors);
	}

};

//---------------------------------------------------------------------------

bool TGA_Load (Bitmap *bitmap, const char *filename)
{
	int ok = 0;
	File file;
	if (file.open(filename))
	{
		TGA_Header header;
		file.read(&header,sizeof(header));
		if (header.verify())
		{
			if (header.is_indexed() && header.has_palette())
			{
				int width = header.width;
				int height = header.height;
				int bpp = header.bits_per_pixel;
				int offset = sizeof(header)+header.identsize;	// skip header & optional
				int num_colors = header.num_colors();

				assert(header.color_start == 0);

				ok = (header.bits_per_color == 24);
				if (!ok) goto done;
				ok = !header.is_packed();
				if (!ok) goto done;

				char *pixels = bitmap->alloc(width,height,bpp);
				if (pixels)
				{
					file.seek(offset);

					unsigned char palette[256*3];
					ok = file.read(palette, num_colors*3);		// bits_per_color = 24/8 = 3
					for (int i=0; i<num_colors; i++)			// r,g,b order
					{
						unsigned char r,b;
						r = palette[i*3+2];
						b = palette[i*3+0];
						palette[i*3+0] = r;
						palette[i*3+2] = b;
					}
					bitmap->set_rgb_palette(palette,num_colors);

					char *dst_line = pixels;					// start at top
					int next_line = bitmap->line_size();

					if (header.is_y_flipped())
					{
						dst_line += next_line*(height-1);		// start at bottom
						next_line = -next_line;
					}

					int src_line_size = width * ((bpp+7)/8);
					char *src_line = (char *)::malloc(src_line_size);
					if (src_line)
					{
						for (int y=0; y<height; y++)
						{
							char *s = src_line;
							char *d = dst_line;

							file.read(src_line,src_line_size);

							// ASSUME UNCOMPRESSED

							int dx = 1;
							if (header.is_x_flipped())
							{
								dx = -1;
								d += width-1;					// start at right
							}
							
							// COPY 8-bit indexed colors
							for (int i=0; i<width; ++i)
							{
		    					*d = *s++;
								d += dx;
							}

							dst_line += next_line;
						}
						::free(src_line);

						ok = 1;
					}
				} // pixels
			} // indexed
		} // verify
done:
		file.close();
	}
	return ok != 0;
}

//---------------------------------------------------------------------------

bool Bitmap::load (const char *filename)
{
	return TGA_Load(this,filename);
}

//---------------------------------------------------------------------------
