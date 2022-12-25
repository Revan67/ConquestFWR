//$Header: /Libs/Src/TXMLib/TXMLoad.cpp 25    8/18/98 3:27p Pbleisch $
//Copyright 1997 (c) Digital Anvil, Inc.

#include <windows.h>
#include "TXMLoad.h"
#include "FileSys.h"
#include "rendpipeline.h"
#include "fdump.h"

#if !USE_NWO
	#include "display.h"
	#define USE_PALETTE_EXT 1
#endif

extern IRenderPipeline *PIPE;

//---------------------------------------------------------------------------

bool Read4ByteVal (IFileSystem* fs, const char* name, long& out)
{
	bool result = false;
	DAFILEDESC desc (name);
	desc.lpImplementation = "DOS";

	HANDLE h;

	if (INVALID_HANDLE_VALUE != (h = fs->OpenChild (&desc)))
	{
		ASSERT (4 == fs->GetFileSize (h));

		S32 num;
		DWORD read;
		CHECK (fs->ReadFile (h, &num, 4, &read, NULL))

		CHECK (fs->CloseHandle (h))

		result = true;

		out = num;
	}

	return result;
}

//---------------------------------------------------------------------------

static unsigned char* HeapStart = NULL;

static int HeapLen = 0;
static int HeapCur = 0;

//EMAURER after each call to AllocMem (), all ptrs obtained using MemPtr () are
//possibly invalid and should not be used.

OFFSET AllocMem (int req)
{
	ASSERT (req > 0);

	OFFSET result;

	int heap_space = HeapLen - HeapCur;

	if (heap_space < req)
	{
		//EMAURER grow heap.
		HeapStart = (unsigned char*) realloc (HeapStart, HeapLen + (req - heap_space));
		ASSERT (HeapStart);

		HeapLen += (req - heap_space);
	}

	result = HeapCur;
	HeapCur += req;

	return result;
}

unsigned char* MemPtr (OFFSET offset)
{
	return (INVALID_OFFSET == offset) ? NULL : HeapStart + offset;
}

void HeapReset (void)
{
	HeapCur = 0;
}

void StartupScratchHeap (void)
{
	//EMAURER big enough for 16 bit texture w/alpha.

	int bytes = 0;
	int size = 256;

	while (size > 0)
	{
		bytes += size * size * 2 + size * size * 1;
		size /= 2;
	}

	HeapLen = bytes;
	HeapStart = (unsigned char*)malloc (HeapLen);
}

void ShutdownScratchHeap (void)
{
	if (HeapStart)
	{
		free (HeapStart);
		HeapStart = NULL;
	}

	HeapLen = 0;
}

//---------------------------------------------------------------------------

bool AlphaIsRange (const unsigned char* abits, int len)
{
	bool range = false;

	const unsigned char* const aend = abits + len;

	while (abits < aend)
	{
		if ((*abits != 0) && (*abits != 0xff))
		{
			range = true;	// Range of alpha values. Use 4444 format.
			break;
		}
		abits++;
	}

	return range;
}

OFFSET ReadAlpha (IFileSystem* fs, int x_size, int y_size)
{
	DAFILEDESC desc ("Alpha 8 bit");
	desc.lpImplementation = "DOS";

	HANDLE h = fs->OpenChild (&desc);

	OFFSET alpha = INVALID_OFFSET;

	if (INVALID_HANDLE_VALUE != h)
	{
		DWORD read;

		unsigned int alpha_size = x_size * y_size * 1;

		alpha = AllocMem (alpha_size);

		ASSERT (fs->GetFileSize (h) == alpha_size);
		CHECK (fs->ReadFile (h, MemPtr (alpha), alpha_size, &read))
		CHECK (fs->CloseHandle (h))
	}

	return alpha;
}

//---------------------------------------------------------------------------

struct MIPData
{
	OFFSET palette;
	OFFSET image;
	OFFSET alpha;
	int x_size;
	int y_size;
};

const unsigned int MAX_MIP_LEVELS = 16;

//---------------------------------------------------------------------------
// 8 bit load
//---------------------------------------------------------------------------

static OFFSET ReadPalette (IFileSystem* fs)
{
	OFFSET result = INVALID_OFFSET;

	DAFILEDESC desc ("Palette RGB 888");
	desc.lpImplementation = "DOS";

	HANDLE h = fs->OpenChild (&desc);

	if (INVALID_HANDLE_VALUE != h)
	{
		DWORD bufsize = fs->GetFileSize (h);

		ASSERT (bufsize % sizeof (RGB_888) == 0);

		DWORD read;

		result = AllocMem (bufsize);

		CHECK (fs->ReadFile (h, MemPtr (result), bufsize, &read))
		CHECK (fs->CloseHandle (h))
	}

	return result;
}

static OFFSET ReadIndices (IFileSystem* fs, int x_size, int y_size)
{
	DAFILEDESC desc ("Image indices");
	desc.lpImplementation = "DOS";

	unsigned int image_size = x_size * y_size * 1;

	OFFSET image_indices = AllocMem (image_size);

	HANDLE h = fs->OpenChild (&desc);
	ASSERT (h != INVALID_HANDLE_VALUE);

	DWORD read;
	ASSERT (fs->GetFileSize (h, NULL) == image_size);
	CHECK (fs->ReadFile (h, MemPtr (image_indices), image_size, &read, NULL))
	CHECK (fs->CloseHandle (h))

	return image_indices;
}

static unsigned int Read8BitData (IFileSystem* fs, 
									MIPData* mip_data, 
									unsigned int max_mip_entries, 
									unsigned int x_size, 
									unsigned int y_size,
									bool load_mipmaps)
{
	//read palette
	OFFSET global_palette = ReadPalette (fs);

	ASSERT (x_size && y_size);

	WIN32_FIND_DATA	find_data;

	HANDLE srch = fs->FindFirstFile ("MIP*", &find_data);

	unsigned int num_MIP_levels = 0;

	if (INVALID_HANDLE_VALUE != srch)
	{
		do
		{
			unsigned int MIP_level = atoi (find_data.cFileName + 3);

			if (((MIP_level == 0) || load_mipmaps) && (MIP_level < max_mip_entries))
			{
				ASSERT (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);

				CHECK (fs->SetCurrentDirectory (find_data.cFileName))

				OFFSET local_palette = ReadPalette (fs);

				mip_data[MIP_level].x_size = x_size;
				mip_data[MIP_level].y_size = y_size;

				mip_data[MIP_level].image = ReadIndices (fs, x_size, y_size);
				mip_data[MIP_level].alpha = ReadAlpha (fs, x_size, y_size);

				mip_data[MIP_level].palette = (INVALID_OFFSET != local_palette) ? local_palette : global_palette;
				ASSERT (mip_data[MIP_level].palette != INVALID_OFFSET);

				x_size /= 2;
				y_size /= 2;

				num_MIP_levels++;

				CHECK (fs->SetCurrentDirectory (".."))
			}

			if (!fs->FindNextFile (srch, &find_data))
			{
				ASSERT (ERROR_NO_MORE_FILES == fs->GetLastError ());
				break;
			}
		}
		while (true);
	}

	return num_MIP_levels;
}

#if !USE_NWO
static void SubmitGL8Bit (MIPData* mip_data, unsigned int mip_cnt, bool alpha_range)
{
	//EMAURER I'm sure that all of this GL submission could be a bit more economically done,
	//but this code is only needed to support our NT tools until DX6 shows up on NT.

	for (unsigned int i = 0; i < mip_cnt; i++)
	{
		if (INVALID_OFFSET == mip_data[i].alpha)
		{
		#if USE_PALETTE_EXT
			if (glColorTableEXT)
			{
				ASSERT (MemPtr (mip_data[i].palette));
				ASSERT (MemPtr (mip_data[i].image));
				glColorTableEXT (GL_TEXTURE_2D, GL_RGB8, 256, GL_RGB, GL_UNSIGNED_BYTE, (GLubyte*)MemPtr (mip_data[i].palette));
				::glTexImage2D(GL_TEXTURE_2D, i, GL_COLOR_INDEX8_EXT, mip_data[i].x_size, mip_data[i].y_size, 0, GL_COLOR_INDEX, GL_UNSIGNED_BYTE, MemPtr (mip_data[i].image));
			}
			else
		#endif
			{
				ASSERT (MemPtr (mip_data[i].palette));
				ASSERT (MemPtr (mip_data[i].image));
				dgluSetColorTables(256, GL_RGB, MemPtr (mip_data[i].palette));
				::glTexImage2D(GL_TEXTURE_2D, i, (GLenum)3, mip_data[i].x_size, mip_data[i].y_size, 0, GL_COLOR_INDEX, GL_UNSIGNED_BYTE, MemPtr (mip_data[i].image));
			}
		}
		else	//EMAURER there is an alpha channel
		{
			//EMAURER convert to 32bit and submit.  

			OFFSET out32 = AllocMem (mip_data[i].x_size * mip_data[i].y_size * 4);

			{
				const unsigned char* indices = MemPtr (mip_data[i].image);
				const RGB_888* pal = (const RGB_888*)MemPtr (mip_data[i].palette);
				unsigned char* out = MemPtr (out32);

				const unsigned char* abits = MemPtr (mip_data[i].alpha);

				unsigned int image_size = mip_data[i].x_size * mip_data[i].y_size;
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
			GLenum internal_fmt = (alpha_range) ? GL_RGBA4 : GL_RGB5_A1;

			::glTexImage2D(GL_TEXTURE_2D, i, internal_fmt, mip_data[i].x_size, mip_data[i].y_size, 0, GL_RGBA, GL_UNSIGNED_BYTE, MemPtr (out32));
		}
	}
}
#endif

unsigned int Load8BitSimple (IFileSystem* fs, 
								bool& has_alpha, 
								unsigned int x_size,
								unsigned int y_size,
								unsigned int hints)
{
	MIPData mip_data[MAX_MIP_LEVELS];

	unsigned int mip_cnt = Read8BitData (fs, mip_data, MAX_MIP_LEVELS, x_size, y_size, !(hints & HINT_NO_MIPMAPS));

	//EMAURER analyze alpha on all mip levels.  all levels must have same pixel fmt,
	//so if any level has an alpha range as opposed to just transparency, all
	//levels need to have an accomodating pixel fmt

	//EMAURER once any alpha range is found, that's enough info, stop.
	bool alpha_range = false;
	has_alpha = false;

	for (unsigned int i = 0; i < mip_cnt; i++)
	{
		if (mip_data[i].alpha != INVALID_OFFSET)
		{
			has_alpha = true;

			if (AlphaIsRange (MemPtr (mip_data[i].alpha), mip_data[i].x_size * mip_data[i].y_size))
			{
				alpha_range = true;
				break;
			}
		}
	}

#if !USE_NWO
	if (!PIPE)
		SubmitGL8Bit (mip_data, mip_cnt, alpha_range);
	else
#endif
	{
		//EMAURER describe source format

		PixelFormat src_fmt;
		
		DDPIXELFORMAT ddpf;

		memset (&ddpf, 0, sizeof(ddpf));
		ddpf.dwSize = sizeof(ddpf);

		ddpf.dwFlags = DDPF_PALETTEINDEXED8;
		ddpf.dwRGBBitCount = 8;

		src_fmt.init (ddpf);

		//EMAURER choose internal format based on input hints.

		PixelFormat dst_fmt;

		memset (&ddpf, 0, sizeof(ddpf));
		ddpf.dwSize = sizeof(ddpf);

		if (has_alpha)
		{
			ddpf.dwFlags = DDPF_RGB | DDPF_ALPHAPIXELS;

			if (HINT_MAX_FIDELITY & hints)
			{
				ddpf.dwRGBBitCount = 32;

				ddpf.dwRBitMask = 0x00FF0000;
				ddpf.dwGBitMask = 0x0000FF00;
				ddpf.dwBBitMask = 0x000000FF;
				ddpf.dwRGBAlphaBitMask = 0xFF000000;
			}
			else if (alpha_range)
			{
				//EMAURER if there is a range of alpha values, use 4444.
				//otherwise, use 5551.

				ddpf.dwRGBBitCount = 16;
				ddpf.dwRBitMask = 0x00000F00;
				ddpf.dwGBitMask = 0x000000F0;
				ddpf.dwBBitMask = 0x0000000F;
				ddpf.dwRGBAlphaBitMask = 0x0000F000;
			}
			else
			{
				ddpf.dwRGBBitCount = 16;
				ddpf.dwRBitMask = 0x00007C00;
				ddpf.dwGBitMask = 0x000003E0;
				ddpf.dwBBitMask = 0x0000001F;
				ddpf.dwRGBAlphaBitMask = 0x00008000;
			}

			dst_fmt.init (ddpf);
		}
		else
		{
			if (HINT_MAX_FIDELITY & hints)
			{
				//EMAURER need 24 bit format.  Maybe should use 32 bit fmt
				//because 24 bit is lame?
				ddpf.dwFlags = DDPF_RGB;
				ddpf.dwRGBBitCount = 24;
				ddpf.dwRBitMask = 0x00FF0000;
				ddpf.dwGBitMask = 0x0000FF00;
				ddpf.dwBBitMask = 0x000000FF;

				dst_fmt.init (ddpf);
			}
			else if (HINT_NO_PALETTES & hints)
			{
				//EMAURER xlate to 16 bit
				ddpf.dwFlags = DDPF_RGB;
				ddpf.dwRGBBitCount = 16;
				ddpf.dwRBitMask = 0x0000F800;
				ddpf.dwGBitMask = 0x000007E0;
				ddpf.dwBBitMask = 0x0000001F;

				dst_fmt.init (ddpf);
			}
			else
			{
				//EMAURER ask for the format that was read in.
				dst_fmt = src_fmt;
			}
		}

		//EMAURER if this assertion doesn't hold true, some code needs to be
		//written to convert RGB_888 to RGB.

		ASSERT (sizeof (RGB) == sizeof (RGB_888));

		for (unsigned int i = 0; i < mip_cnt; i++)
		{
			PIPE->set_texture_level_data (RP_CURRENT, 
											i, 
											dst_fmt,
											mip_data[i].x_size, 
											mip_data[i].y_size, 
											mip_data[i].x_size,
											src_fmt,
											MemPtr (mip_data[i].image), 
											MemPtr (mip_data[i].alpha), 
											(const RGB*)MemPtr (mip_data[i].palette));
		}
	}

	return mip_cnt;
}

//---------------------------------------------------------------------------
// 16 bit load
//---------------------------------------------------------------------------

static OFFSET ReadColors (IFileSystem* fs, unsigned int x_size, unsigned int y_size)
{
	DWORD read;

	unsigned int image_size = x_size * y_size * 2;

	OFFSET image_colors = AllocMem (image_size);

	DAFILEDESC desc ("Image colors");

	HANDLE h = fs->OpenChild (&desc);

	ASSERT (INVALID_HANDLE_VALUE != h);
	ASSERT (fs->GetFileSize (h) == image_size);

	CHECK (fs->ReadFile (h, MemPtr (image_colors), image_size, &read))
	CHECK (fs->CloseHandle (h))

	return image_colors;
}

static unsigned int Read16BitData (IFileSystem* fs, 
									MIPData* mip_data, 
									unsigned int max_mip_entries, 
									unsigned int x_size,
									unsigned int y_size,
									bool load_mipmaps)
{
	ASSERT (x_size && y_size);

	WIN32_FIND_DATA	find_data;

	HANDLE srch = fs->FindFirstFile ("MIP*", &find_data);

	unsigned int num_MIP_levels = 0;

	if (INVALID_HANDLE_VALUE != srch)
	{
		do
		{
			unsigned int MIP_level = atoi (find_data.cFileName + 3);

			if (((MIP_level == 0) || load_mipmaps) && (MIP_level < max_mip_entries))
			{
				ASSERT (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);

				CHECK (fs->SetCurrentDirectory (find_data.cFileName))

				mip_data[MIP_level].x_size = x_size;
				mip_data[MIP_level].y_size = y_size;

				mip_data[MIP_level].image = ReadColors (fs, x_size, y_size);
				mip_data[MIP_level].alpha = ReadAlpha (fs, x_size, y_size);

				x_size /= 2;
				y_size /= 2;

				num_MIP_levels++;

				CHECK (fs->SetCurrentDirectory (".."))
			}

			if (!fs->FindNextFile (srch, &find_data))
			{
				ASSERT (ERROR_NO_MORE_FILES == fs->GetLastError ());
				break;
			}
		}
		while (true);
	}

	return num_MIP_levels;
}

#if !USE_NWO
static void SubmitGL16Bit (MIPData* mip_data, unsigned int mip_cnt, bool alpha_range)
{
	//EMAURER I'm sure that all of this GL submission could be a bit more economically done,
	//but this code is only needed to support our NT tools until DX6 shows up on NT.

	for (unsigned int i = 0; i < mip_cnt; i++)
	{
		if (INVALID_OFFSET != mip_data[i].alpha)
		{
			//EMAURER convert to 32bit and submit.  

			OFFSET out32 = AllocMem (mip_data[i].x_size * mip_data[i].y_size * 4);
			ASSERT (out32 != INVALID_OFFSET);

			{
				const U16* colors = (const U16*)MemPtr (mip_data[i].image);
				unsigned char* out = MemPtr (out32);

				const unsigned char* abits = MemPtr (mip_data[i].alpha);

				const U16* const stop = colors + (mip_data[i].x_size * mip_data[i].y_size);

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
			GLenum internal_fmt = (alpha_range) ? GL_RGBA4 : GL_RGB5_A1;

			::glTexImage2D(GL_TEXTURE_2D, i, internal_fmt, mip_data[i].x_size, mip_data[i].y_size, 0, GL_RGBA, GL_UNSIGNED_BYTE, MemPtr (out32));
		}
		else
		{
			//EMAURER convert to 24bit and submit

			OFFSET out24 = AllocMem (mip_data[i].x_size * mip_data[i].y_size * 3);
			ASSERT (out24 != INVALID_OFFSET);

			{
				const U16* colors = (const U16*)MemPtr (mip_data[i].image);
				unsigned char* out = MemPtr (out24);

				const U16* const stop = colors + (mip_data[i].x_size * mip_data[i].y_size);

				while (colors < stop)
				{
					U32 rgb = *colors;

					*out++ = RGB_565_R (rgb);
					*out++ = RGB_565_G (rgb);
					*out++ = RGB_565_B (rgb);
					colors++;
				}
			}

			::glTexImage2D(GL_TEXTURE_2D, i, GL_RGB, mip_data[i].x_size, mip_data[i].y_size, 0, GL_RGB, GL_UNSIGNED_BYTE, MemPtr (out24));
		}
	}
}
#endif

unsigned int Load16BitSimple (IFileSystem* fs, 
								bool& has_alpha, 
								unsigned int x_size,
								unsigned int y_size,
								unsigned int hints)
{
	MIPData mip_data[MAX_MIP_LEVELS];

	unsigned int mip_cnt = Read16BitData (fs, mip_data, MAX_MIP_LEVELS, x_size, y_size, !(hints & HINT_NO_MIPMAPS));

	//EMAURER analyze alpha on all mip levels.  all levels must have same pixel fmt,
	//so if any level has an alpha range as opposed to just transparency, all
	//levels need to have an accomodating pixel fmt

	//EMAURER once any alpha range is found, that's enough info, stop.
	bool alpha_range = false;
	has_alpha = false;

	for (unsigned int i = 0; i < mip_cnt; i++)
	{
		if (mip_data[i].alpha != INVALID_OFFSET)
		{
			has_alpha = true;

			if (AlphaIsRange (MemPtr (mip_data[i].alpha), mip_data[i].x_size * mip_data[i].y_size))
			{
				alpha_range = true;
				break;
			}
		}
	}

#if !USE_NWO
	if (!PIPE)
		SubmitGL16Bit (mip_data, mip_cnt, alpha_range);
	else
#endif
	{
		//EMAURER describe source format

		PixelFormat src_fmt;
		
		DDPIXELFORMAT ddpf;

		memset (&ddpf, 0, sizeof(ddpf));
		ddpf.dwSize = sizeof(ddpf);

		ddpf.dwFlags = DDPF_RGB;
		ddpf.dwRGBBitCount = 16;
		ddpf.dwRBitMask = 0x0000F800;
		ddpf.dwGBitMask = 0x000007E0;
		ddpf.dwBBitMask = 0x0000001F;

		src_fmt.init (ddpf);

		//EMAURER choose internal format based on input hints.

		PixelFormat dst_fmt;

		memset (&ddpf, 0, sizeof(ddpf));
		ddpf.dwSize = sizeof(ddpf);

		if (has_alpha)
		{
			ddpf.dwFlags = DDPF_RGB | DDPF_ALPHAPIXELS;

			if (HINT_MAX_FIDELITY & hints)
			{
				ddpf.dwRGBBitCount = 32;

				ddpf.dwRBitMask = 0x00FF0000;
				ddpf.dwGBitMask = 0x0000FF00;
				ddpf.dwBBitMask = 0x000000FF;
				ddpf.dwRGBAlphaBitMask = 0xFF000000;
			}
			else if (alpha_range)
			{
				//EMAURER if there is a range of alpha values, use 4444.
				//otherwise, use 5551.
				ddpf.dwRGBBitCount = 16;
				ddpf.dwRBitMask = 0x00000F00;
				ddpf.dwGBitMask = 0x000000F0;
				ddpf.dwBBitMask = 0x0000000F;
				ddpf.dwRGBAlphaBitMask = 0x0000F000;
			}
			else
			{
				ddpf.dwRGBBitCount = 16;
				ddpf.dwRBitMask = 0x00007C00;
				ddpf.dwGBitMask = 0x000003E0;
				ddpf.dwBBitMask = 0x0000001F;
				ddpf.dwRGBAlphaBitMask = 0x00008000;
			}

			dst_fmt.init (ddpf);
		}
		else
		{
			if (HINT_MAX_FIDELITY & hints)
			{
				//EMAURER need 24 bit format.  Maybe should use 32 bit fmt
				//because 24 bit is lame?
				ddpf.dwFlags = DDPF_RGB;
				ddpf.dwRGBBitCount = 24;
				ddpf.dwRBitMask = 0x00FF0000;
				ddpf.dwGBitMask = 0x0000FF00;
				ddpf.dwBBitMask = 0x000000FF;

				dst_fmt.init (ddpf);
			}
			else
			{
				//EMAURER ask for the format that was read in.
				dst_fmt = src_fmt;
			}
		}

		for (unsigned int i = 0; i < mip_cnt; i++)
		{
			PIPE->set_texture_level_data (RP_CURRENT, 
											i, 
											dst_fmt,
											mip_data[i].x_size, 
											mip_data[i].y_size, 
											mip_data[i].x_size * 2,
											src_fmt,
											MemPtr (mip_data[i].image), 
											MemPtr (mip_data[i].alpha), 
											NULL);
		}
	}

	return mip_cnt;
}
