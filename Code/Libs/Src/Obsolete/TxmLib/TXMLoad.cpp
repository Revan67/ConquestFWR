//$Header: /Libs/Src/TXMLib/TXMLoad.cpp 25    8/18/98 3:27p Pbleisch $
//Copyright 1997 (c) Digital Anvil, Inc.

#include <windows.h>
#include "TXMLoad.h"
#include "FileSys.h"
#include "rendpipeline.h"
#include "fdump.h"

extern IRenderPipeline *PIPE;

#if !USE_NWO
	#include "display.h"
	#define USE_PALETTE_EXT 1
#endif


//---------------------------------------------------------------------------
//EMAURER old-stylee

static bool Submit8BitTexture (IFileSystem* fs, int MIP_level, long x_size, long y_size)
{
	bool r;

	long num_colors = 0;

	DWORD read;

// read palette

	r = Read4ByteVal (fs, "Palette color count", num_colors);
	ASSERT (r);
	ASSERT (num_colors <= 256);

	DAFILEDESC desc ("Palette RGB 888");

	OFFSET palette = INVALID_OFFSET;

	HANDLE h = fs->OpenChild (&desc);

	if (INVALID_HANDLE_VALUE != h)
	{
		int bufsize = num_colors * sizeof (RGB_888);

		palette = AllocMem (bufsize);

		CHECK (fs->ReadFile (h, MemPtr (palette), bufsize, &read))
		CHECK (fs->CloseHandle (h))
	}
	//else check for other palette formats

//check for 8 bit alpha channel

	desc.lpFileName = "Alpha 8 bit";

	h = fs->OpenChild (&desc);

	OFFSET alpha = INVALID_OFFSET;

	if (INVALID_HANDLE_VALUE != h)
	{
		unsigned int alpha_size = x_size * y_size * 1;

		alpha = AllocMem (alpha_size);

		ASSERT (fs->GetFileSize (h) == alpha_size);
		CHECK (fs->ReadFile (h, MemPtr (alpha), alpha_size, &read))
		CHECK (fs->CloseHandle (h))
	}

//read the image indices

	desc.lpFileName = "Image indices";

	unsigned int image_size = x_size * y_size * 1;

	OFFSET image_indices = AllocMem (image_size);

	h = fs->OpenChild (&desc);
	ASSERT (h != INVALID_HANDLE_VALUE);

	ASSERT (fs->GetFileSize (h, NULL) == image_size);
	CHECK (fs->ReadFile (h, MemPtr (image_indices), image_size, &read, NULL))
	CHECK (fs->CloseHandle (h))

//submit texture

#if !USE_NWO
	if (!PIPE)
	{
		//EMAURER I'm sure that all of this GL submission could be a bit more economically done,
		//but this code is only needed to support our NT tools until DX6 shows up on NT.

		if (INVALID_OFFSET == alpha)
		{
		#if USE_PALETTE_EXT
			if (glColorTableEXT)
			{
				ASSERT (MemPtr (palette));
				ASSERT (MemPtr (image_indices));
				glColorTableEXT (GL_TEXTURE_2D, GL_RGB8, 256, GL_RGB, GL_UNSIGNED_BYTE, (GLubyte*)MemPtr (palette));
				::glTexImage2D(GL_TEXTURE_2D, MIP_level, GL_COLOR_INDEX8_EXT, x_size, y_size, 0, GL_COLOR_INDEX, GL_UNSIGNED_BYTE, MemPtr (image_indices));
			}
			else
		#endif
			{
				ASSERT (MemPtr (palette));
				ASSERT (MemPtr (image_indices));
				dgluSetColorTables(256, GL_RGB, MemPtr (palette));
				::glTexImage2D(GL_TEXTURE_2D, MIP_level, (GLenum)3, x_size, y_size, 0, GL_COLOR_INDEX, GL_UNSIGNED_BYTE, MemPtr (image_indices));
			}
		}
		else	//EMAURER there is an alpha channel
		{
			//EMAURER convert to 32bit and submit.  

			OFFSET out32 = AllocMem (x_size * y_size * 4);

			{
				const unsigned char* indices = MemPtr (image_indices);
				const RGB_888* pal = (const RGB_888*)MemPtr (palette);
				unsigned char* out = MemPtr (out32);

				const unsigned char* abits = MemPtr (alpha);

				const unsigned char* const stop = indices + image_size;

				while (indices < stop)
				{
					*out++ = pal[*indices].R;
					*out++ = pal[*indices].G;
					*out++ = pal[*indices].B;
					*out++ = *abits++;
					indices++;
				}
			}

			//EMAURER determine internal fmt by checking if alpha map is grey scale or only black/white.
			GLenum internal_fmt = AlphaIsRange (MemPtr (alpha), x_size * y_size * 1) ? GL_RGBA4 : GL_RGB5_A1;

			::glTexImage2D(GL_TEXTURE_2D, MIP_level, internal_fmt, x_size, y_size, 0, GL_RGBA, GL_UNSIGNED_BYTE, MemPtr (out32));
		}
	}
	else
#endif
	{
		PixelFormat src_fmt;

	//EMAURER describe current texture.
		
		DDPIXELFORMAT ddpf;

		memset (&ddpf, 0, sizeof(ddpf));
		ddpf.dwSize = sizeof(ddpf);

		ddpf.dwFlags = DDPF_PALETTEINDEXED8;
		ddpf.dwRGBBitCount = 8;

		src_fmt.init (ddpf);

	//EMAURER describe what the desired texture format is.

		PixelFormat dst_fmt;

		//EMAURER if there is an alpha channel, ask for conversion to 16bit
		if (alpha != INVALID_OFFSET)
		{
			//EMAURER hard-coded choice being made here.  16 bit data with
			//alpha is always requested to be stored internally in 16 bits.
			//could be requested as 32 bits for fidelity's sake.

			memset (&ddpf, 0, sizeof(ddpf));
			ddpf.dwSize = sizeof(ddpf);

			ddpf.dwFlags = DDPF_RGB | DDPF_ALPHAPIXELS;
			ddpf.dwRGBBitCount = 16;

			//EMAURER if there is a range of alpha values, use 4444.
			//otherwise, use 5551.

			if (AlphaIsRange (MemPtr (alpha), x_size * y_size * 1))
			{
				ddpf.dwRBitMask = 0x00000F00;
				ddpf.dwGBitMask = 0x000000F0;
				ddpf.dwBBitMask = 0x0000000F;
				ddpf.dwRGBAlphaBitMask = 0x0000F000;
			}
			else
			{
				ddpf.dwRBitMask = 0x00007C00;
				ddpf.dwGBitMask = 0x000003E0;
				ddpf.dwBBitMask = 0x0000001F;
				ddpf.dwRGBAlphaBitMask = 0x00008000;
			}

			dst_fmt.init (ddpf);
		}
		else	//EMAURER no alpha, submit as palettized.  RenderPipe will do the right thing.
			dst_fmt = src_fmt;

		//EMAURER if this assertion doesn't hold true, some code needs to be
		//written to convert RGB_888 to RGB.

		ASSERT (sizeof (RGB) == sizeof (RGB_888));

		PIPE->set_texture_level_data (RP_CURRENT, 
										MIP_level, 
										dst_fmt,
										x_size, 
										y_size, 
										x_size,
										src_fmt,
										MemPtr (image_indices), 
										MemPtr (alpha), 
										(const RGB*)MemPtr (palette));
	}

	HeapReset ();

	return alpha != INVALID_OFFSET;
}

//---------------------------------------------------------------------------

static bool Submit16BitTexture (IFileSystem* fs, int MIP_level, long x_size, long y_size)
{
	//EMAURER image colors read from file are assumed to be of format 5R 6G 5B
	DWORD read;

// read pixel image

	unsigned int image_size = x_size * y_size * 2;

	OFFSET image_colors = AllocMem (image_size);

	DAFILEDESC desc ("Image colors");

	HANDLE h = fs->OpenChild (&desc);

	ASSERT (INVALID_HANDLE_VALUE != h);
	ASSERT (fs->GetFileSize (h) == image_size);

	CHECK (fs->ReadFile (h, MemPtr (image_colors), image_size, &read))
	CHECK (fs->CloseHandle (h))

// read alpha channel

	desc.lpFileName = "Alpha 8 bit";

	h = fs->OpenChild (&desc);

	OFFSET alpha = INVALID_OFFSET;

	if (INVALID_HANDLE_VALUE != h)
	{
		unsigned int alpha_size = x_size * y_size * 1;

		alpha = AllocMem (alpha_size);

		ASSERT (fs->GetFileSize (h) == alpha_size);
		CHECK (fs->ReadFile (h, MemPtr (alpha), alpha_size, &read))
		CHECK (fs->CloseHandle (h))
	}

//submit texture

#if !USE_NWO
	if (!PIPE)
	{
		//EMAURER I'm sure that all of this GL submission could be a bit more economically done,
		//but this code is only needed to support our NT tools until DX6 shows up on NT.

		if (INVALID_OFFSET != alpha)
		{
			//EMAURER convert to 32bit and submit.  

			OFFSET out32 = AllocMem (x_size * y_size * 4);
			ASSERT (out32 != INVALID_OFFSET);

			{
				const U16* colors = (const U16*)MemPtr (image_colors);
				unsigned char* out = MemPtr (out32);

				const unsigned char* abits = MemPtr (alpha);

				const U16* const stop = colors + (x_size * y_size);

				while (colors < stop)
				{
					U32 rgb = *colors;
					*out++ = RGB_565_R (rgb);
					*out++ = RGB_565_G (rgb);
					*out++ = RGB_565_B (rgb);
					*out++ = *abits++;
					colors++;
				}
			}

			//EMAURER determine internal fmt by checking if alpha map is grey scale or only black/white.
			GLenum internal_fmt = AlphaIsRange (MemPtr (alpha), x_size * y_size * 1) ? GL_RGBA4 : GL_RGB5_A1;

			::glTexImage2D(GL_TEXTURE_2D, MIP_level, internal_fmt, x_size, y_size, 0, GL_RGBA, GL_UNSIGNED_BYTE, MemPtr (out32));
		}
		else
		{
			//EMAURER convert to 24bit and submit

			OFFSET out24 = AllocMem (x_size * y_size * 3);
			ASSERT (out24 != INVALID_OFFSET);

			{
				const U16* colors = (const U16*)MemPtr (image_colors);
				unsigned char* out = MemPtr (out24);

				const U16* const stop = colors + (x_size * y_size);

				while (colors < stop)
				{
					U32 rgb = *colors;

					*out++ = RGB_565_R (rgb);
					*out++ = RGB_565_G (rgb);
					*out++ = RGB_565_B (rgb);
					colors++;
				}
			}

			::glTexImage2D(GL_TEXTURE_2D, MIP_level, GL_RGB, x_size, y_size, 0, GL_RGB, GL_UNSIGNED_BYTE, MemPtr (out24));
		}
	}
	else
#endif
	{
		PixelFormat src_fmt;

	//EMAURER describe current texture.
		
		DDPIXELFORMAT ddpf;

		memset (&ddpf, 0, sizeof(ddpf));
		ddpf.dwSize = sizeof(ddpf);

		ddpf.dwFlags = DDPF_RGB;
		ddpf.dwRGBBitCount = 16;
		ddpf.dwRBitMask = 0x0000F800;
		ddpf.dwGBitMask = 0x000007E0;
		ddpf.dwBBitMask = 0x0000001F;

		src_fmt.init (ddpf);

	//EMAURER describe what the desired texture format is.

		PixelFormat dst_fmt;

		if (INVALID_OFFSET != alpha)
		{
			//EMAURER hard-coded choice being made here.  16 bit data with
			//alpha is always requested to be stored internally in 16 bits.
			//could be requested as 32 bits for fidelity's sake.

			memset (&ddpf, 0, sizeof(ddpf));
			ddpf.dwSize = sizeof(ddpf);

			ddpf.dwFlags = DDPF_RGB | DDPF_ALPHAPIXELS;
			ddpf.dwRGBBitCount = 16;

			//EMAURER if there is a range of alpha values, use 4444.
			//otherwise, use 5551.

			if (AlphaIsRange (MemPtr (alpha), x_size * y_size * 1))
			{
				ddpf.dwRBitMask = 0x00000F00;
				ddpf.dwGBitMask = 0x000000F0;
				ddpf.dwBBitMask = 0x0000000F;
				ddpf.dwRGBAlphaBitMask = 0x0000F000;
			}
			else
			{
				ddpf.dwRBitMask = 0x00007C00;
				ddpf.dwGBitMask = 0x000003E0;
				ddpf.dwBBitMask = 0x0000001F;
				ddpf.dwRGBAlphaBitMask = 0x00008000;
			}

			dst_fmt.init (ddpf);
		}
		else
			dst_fmt = src_fmt;

		//EMAURER if this assertion doesn't hold true, some code needs to be
		//written to convert RGB_888 to RGB.

		ASSERT (sizeof (RGB) == sizeof (RGB_888));

		PIPE->set_texture_level_data (RP_CURRENT, 
										MIP_level, 
										dst_fmt,
										x_size, 
										y_size, 
										x_size * 2,
										src_fmt,
										MemPtr (image_colors), 
										MemPtr (alpha), 
										NULL);
	}

	HeapReset ();

	return alpha != INVALID_OFFSET;
}

//---------------------------------------------------------------------------

bool SubmitTexture (IFileSystem* fs, int MIP_level, bool& has_alpha)
{
	bool result = true;
	bool r;

	long x_size;
	
	r = Read4ByteVal (fs, "Image X size", x_size);
	ASSERT (r);

	long y_size;

	r = Read4ByteVal (fs, "Image Y size", y_size);
	ASSERT (r);

	ASSERT (x_size && y_size);

	if (fs->SetCurrentDirectory ("Palette 8 bit"))
	{
		has_alpha = Submit8BitTexture (fs, MIP_level, x_size, y_size);
		fs->SetCurrentDirectory ("..");
	}
	else if (fs->SetCurrentDirectory ("True RGB 565"))
	{
		has_alpha = Submit16BitTexture(fs, MIP_level, x_size, y_size);
		fs->SetCurrentDirectory ("..");
	}
	else
		result = false;

	return result;
}

//---------------------------------------------------------------------------

enum WRAP_MODE 
{
	REPEAT,
	CLAMP
};

int SubmitMIPTexture (IFileSystem* fs, bool & alpha, bool load_mipmaps, bool use_mipmaps,
					  int & u_mode, int & v_mode)
{
	long w_mode;

	if (Read4ByteVal (fs, "U wrap mode", w_mode))
	{
		switch (w_mode)
		{
			case CLAMP:
		#if !USE_NWO
				if (!PIPE)
					u_mode = GL_CLAMP;
				else
		#endif
				{
					u_mode = D3DTADDRESS_CLAMP;
				}
			break;

			case REPEAT:
			default:
		#if !USE_NWO
				if (!PIPE)
					u_mode = GL_REPEAT;
				else
		#endif
				{
					u_mode = D3DTADDRESS_WRAP;
				}
			break;
		}		
	}

	if (Read4ByteVal (fs, "V wrap mode", w_mode))
	{
		switch(w_mode)
		{
			case CLAMP:
		#if !USE_NWO
				if (!PIPE)
					v_mode = GL_CLAMP;
				else
		#endif
				{
					v_mode = D3DTADDRESS_CLAMP;
				}
			break;

			case REPEAT:
			default:
		#if !USE_NWO
				if (!PIPE)
					v_mode = GL_REPEAT;
				else
		#endif
				{
					v_mode = D3DTADDRESS_WRAP;
				}
			break;
		}
	}


	WIN32_FIND_DATA	find_data;

	HANDLE srch = fs->FindFirstFile ("MIP*", &find_data);

	int num_MIP_levels = 0;

	if (INVALID_HANDLE_VALUE != srch)
	{
		do
		{
			int MIP_level = atoi (find_data.cFileName + 3);

			if ((MIP_level == 0) || load_mipmaps)
			{
				ASSERT (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);

				CHECK (fs->SetCurrentDirectory (find_data.cFileName))

				CHECK (SubmitTexture (fs, MIP_level, alpha))

				CHECK (fs->SetCurrentDirectory (".."))

				num_MIP_levels++;
			}

			if (!fs->FindNextFile (srch, &find_data))
			{
				ASSERT (ERROR_NO_MORE_FILES == fs->GetLastError ());
				break;
			}
		}
		while (true);
	}
	else if ((srch = fs->FindFirstFile ("Palette 8 Bit", &find_data)) != INVALID_HANDLE_VALUE)
	{
		HeapReset ();

		//read dimensions
		bool r;

		long x_size;
			
		r = Read4ByteVal (fs, "Image X size", x_size);
		ASSERT (r);

		long y_size;

		r = Read4ByteVal (fs, "Image Y size", y_size);
		ASSERT (r);

		ASSERT (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);

		CHECK (fs->SetCurrentDirectory (find_data.cFileName))

		num_MIP_levels = Load8BitSimple (fs, alpha, x_size, y_size);

		CHECK (fs->SetCurrentDirectory (".."))
	}
	else if ((srch = fs->FindFirstFile ("True RGB 565", &find_data)) != INVALID_HANDLE_VALUE)
	{
		HeapReset ();

		//read dimensions
		bool r;

		long x_size;
			
		r = Read4ByteVal (fs, "Image X size", x_size);
		ASSERT (r);

		long y_size;

		r = Read4ByteVal (fs, "Image Y size", y_size);
		ASSERT (r);

		ASSERT (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);

		CHECK (fs->SetCurrentDirectory (find_data.cFileName))

		num_MIP_levels = Load16BitSimple (fs, alpha, x_size, y_size);

		CHECK (fs->SetCurrentDirectory (".."))
	}

	if (INVALID_HANDLE_VALUE != srch)
	{
	// Set default filtering modes.
		fs->FindClose(srch);

	#if !USE_NWO
		if (!PIPE)
		{
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

			if ((num_MIP_levels > 1) && use_mipmaps)
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
			else
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, u_mode);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, v_mode);
		}
		else
	#endif
		{
			// is this setting global state or the bound state?
#if IRP_NO_TXM_STATE
			PIPE->set_texture_render_state( RP_CURRENT, D3DRS_TEXTUREMAG, D3DFILTER_LINEAR );

			U32 txmin = ((num_MIP_levels > 1) && use_mipmaps) ? D3DFILTER_MIPLINEAR : D3DFILTER_LINEAR;

			PIPE->set_texture_render_state( RP_CURRENT, D3DRS_TEXTUREMIN, txmin);
			PIPE->set_texture_render_state( RP_CURRENT, D3DRS_TEXTUREADDRESSU, u_mode );
			PIPE->set_texture_render_state( RP_CURRENT, D3DRS_TEXTUREADDRESSV, v_mode );
#endif
		}
	}

	HeapReset ();
		
	return num_MIP_levels;
}

