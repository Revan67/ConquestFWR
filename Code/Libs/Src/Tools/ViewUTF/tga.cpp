#include "stdafx.h"
#include "tga.h"

#include "document.h"
#include "xfile.h"
#include "DABitmap.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

int TGA_page_size (int d)
{
	int shift;
	for (shift=6; d > (1<<shift); )
		shift++;
	return (1<<shift);
}

int TGA_line_size (int page_w, int bpp)
{
	return (page_w*bpp+7)/8;
}


//---------------------------------------------------------------------------

TGA::TGA(void)
{
	m_Pixels = NULL;
	m_Palette = NULL;
}

//---------------------------------------------------------------------------

TGA::~TGA()
{
	close();
}

//---------------------------------------------------------------------------

void TGA::close()
{
	if( m_Pixels )
	{
		free(m_Pixels);
		m_Pixels = NULL;
	}

	if( m_Palette )
	{
		free(m_Palette);
		m_Palette = NULL;
	}
}

//---------------------------------------------------------------------------

char * TGA::alloc ()
{
	if( m_Pixels )
	{
		free(m_Pixels);
		m_Pixels = NULL;
	}

	int page_w = TGA_page_size(width);
	int page_h = TGA_page_size(height);

	int size = page_h * TGA_line_size(page_w,bits_per_pixel);

	m_Pixels = (char *)::malloc(size);

	return (char*)m_Pixels;
}

//---------------------------------------------------------------------------

bool TGA::Load( const char* _filename )
{
	DAFILEDESC desc(_filename);
	desc.lpImplementation = "DOS";
	IFileSystem* file = FS_Open(&desc, "r");

	if( file )
	{
		Load(file);
		file->CloseHandle(0);
	}

	return true;
}

//---------------------------------------------------------------------------

bool TGA::Load( struct IFileSystem* _file )
{
	{
		DWORD numBytes;

		// read in header

		_file->ReadFile(0,&identsize,1,&numBytes);
		_file->ReadFile(0,&palette,1,&numBytes);
		_file->ReadFile(0,&imagetype,1,&numBytes);

		_file->ReadFile(0,&color_start,2,&numBytes);
		_file->ReadFile(0,&num_palette_colors,2,&numBytes);
		_file->ReadFile(0,&bits_per_color,1,&numBytes);

		_file->ReadFile(0,&xstart,2,&numBytes);
		_file->ReadFile(0,&ystart,2,&numBytes);
		_file->ReadFile(0,&width,2,&numBytes);
		_file->ReadFile(0,&height,2,&numBytes);

		_file->ReadFile(0,&bits_per_pixel,1,&numBytes);
		_file->ReadFile(0,&descriptor,1,&numBytes);

		is_x_flipped();
		is_y_flipped();
		is_alpha();
	}

	//	descriptor: 00vhaaaa
	//		h horizontal flip = 0x20
	//		v vertical flip = 0x10
	//		a alpha bits = 0x0F

	DWORD dwRead = 0;
	bool ok = 0;
	if( verify() )
	{
		char name[256];
		_file->GetFileName(name,256);
		m_Name = name;

		int bpp    = bits_per_pixel;
		int offset = sizeof(TGA_Header) + identsize;	// skip header & optional

		if( is_indexed() && has_palette() )
		{
			if( m_Palette )
			{
				free(m_Palette);
				m_Palette = NULL;
			}

			int paletteSize = num_palette_colors * (bits_per_color/8);
			m_Palette = malloc(paletteSize);

			if( m_Palette )
			{
				_file->SetFilePointer(0, offset, 0, FILE_BEGIN);
				_file->ReadFile(0, m_Palette, paletteSize, &dwRead);
			}
		}

		char * pixels = alloc();
		if( pixels )
		{
			int pixelSize = width * height * (bits_per_pixel/8);
			if( _file->ReadFile(0, pixels, pixelSize, &dwRead) )
			{
				ok = true;
			}
		}
	}
	return ok != false;
}

//---------------------------------------------------------------------------

bool TGA::Save( const char* _filename )
{
	if( verify() && m_Pixels && m_Palette )
	{
		DOCDESC desc(_filename);
		desc.lpImplementation = "DOS";
		IFileSystem* file = FS_Create(&desc,0);

		if( file )
		{
			Save(file);
			file->CloseHandle(0);
		}
	}
	return true;
}

//---------------------------------------------------------------------------

bool TGA::Save( struct IFileSystem* _file )
{
	if( verify() && m_Pixels && m_Palette && _file )
	{
		DWORD dwNumberOfBytesWritten;

		TGA_Header& header = *this;

		_file->WriteFile(0, &header, sizeof(TGA_Header), &dwNumberOfBytesWritten);

		if( has_palette() )
		{
			int paletteSize = num_colors() * (bits_per_color / 8); // undefined if this is 15!
			_file->WriteFile(0, m_Palette, paletteSize, &dwNumberOfBytesWritten);
		}

		int pixelSize = (width * height) * (bits_per_pixel / 8);
		_file->WriteFile(0, m_Pixels, pixelSize, &dwNumberOfBytesWritten);
	}
	return false;
}

//---------------------------------------------------------------------------

void TGA::ConvertToBitmap( DABitmap& _bitmap )
{
//	DWORD dwRead = 0;
//	int ok = 0;
//	if( verify() )
//	{
//		int bpp	   = bits_per_pixel;
//		int offset = sizeof(TGA_Header) + identsize;	// skip header & optional
//		int page_w = TGA_page_size(width);
//		int page_h = TGA_page_size(height);
//
//		if( is_indexed() && has_palette() )
//		{
//			assert(color_start == 0);
//			assert(bits_per_color != 24);
//			assert(!is_packed());
//
//			BYTE* palette;
//			_bitmap.palette = palette = new BYTE[ num_palette_colors * (bits_per_color/8) ];
//
//			for (int i=0; i<num_colors(); i++)			// r,g,b order
//			{
//				palette[i] =
//			}
//		}
//
//		_bitmap.pixels = new DWORD[ width * height ];
//		if( _bitmap.pixels )
//		{
//			char* dst_line = (char *)_bitmap.pixels; // start at top
//			int next_line = TGA_line_size (page_w, bpp); // bitmap->line_size();
//
//			if (is_y_flipped())
//			{
//				dst_line += next_line*(height-1);		// start at bottom
//				next_line = -next_line;
//			}
//
//			char* pix_line = (char* )m_Pixels;
//
//			int src_line_size = width * ((bpp+7)/8);
//			char* src_line = (char *)::malloc(src_line_size);
//			if (src_line)
//			{
//				for (int y=0; y<height; y++)
//				{
//					char *s = src_line;
//					char *d = dst_line;
//
//					memcpy( src_line, pix_line + next_line, src_line_size );
//
//					// ASSUME UNCOMPRESSED
//
//					int dx = 1;
//					if (is_x_flipped())
//					{
//						dx = -1;
//						d += width-1;					// start at right
//					}
//
//					// COPY 8-bit indexed colors
//					for (int i=0; i<width; ++i)
//					{
//						*d = *s++;
//						d += dx;
//					}
//
//					dst_line += next_line;
//				}
//			}
//			::free(src_line);
//		}
//	}
}

//---------------------------------------------------------------------------

void TGA::ConvertFromBitmap( DABitmap& _bitmap )
{
}
