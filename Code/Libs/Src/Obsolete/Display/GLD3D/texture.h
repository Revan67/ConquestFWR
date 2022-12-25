//---------------------------------------------------------------------------
/*
	TEXTURE.H

	Copyright (C) 1997 Digital Anvil, Inc.

	Created: October 1997

	Author: Bill Baldwin
*/
//---------------------------------------------------------------------------

#ifndef TEXTURE_H
#define TEXTURE_H

#include "draw.h"
#include <d3d.h>
#include "matrix.h"
#include "stddat.h"
#include "pixel.h"
#include "vbuffer.h"

#define RELEASE(x)			if(x) {(x)->Release();(x)=0;}


struct RGB
{
	U8 r,g,b;
};

//---------------------------------------------------------------------------
// MATERIAL
//---------------------------------------------------------------------------

struct MATERIAL
{
	float			shininess;
	COLOR_VECTOR	ambient;
	COLOR_VECTOR	diffuse;
	COLOR_VECTOR	specular;
	COLOR_VECTOR	emission;
};

//
// RULES: All mipmap levels must have same pixel format, palette.
//

struct MipmapLevel
{
	int						width;
	int						height;		// Size (should be square)
	LPDIRECTDRAWSURFACE3	memory_surface;
	LPDIRECTDRAWPALETTE		palette;

	MipmapLevel(void)
	{
		memory_surface = NULL;
		palette = NULL;
	}

	~MipmapLevel(void)
	{
		if (palette)
		{
			palette->Release();
			palette = NULL;
		}
		if (memory_surface)
		{
			memory_surface->Release();
			memory_surface = NULL;
		}
	}

// Create the system memory surface for this texture.
	bool create_system_surface(const PixelFormat * pixel_format);

	void set_palette(const RGB *bitmap_palette);
};

//---------------------------------------------------------------------------
// TextureObject
//---------------------------------------------------------------------------

#define MAX_MIPMAP_LEVELS	9

//

typedef enum
{
	TOS_NEW,				// no mipmap levels, no device surface, nothing.
	TOS_LEVELS_PRESENT,		// mipmap levels present, no device surface.
	TOS_DEVICE_PRESENT,		// device surface exists with correct number of mipmap levels.
	TOS_READY				// device surface loaded from mipmap levels.
} TextureObjectState;

//

class TextureMgr;

//

struct TextureObject
{
	static TextureMgr * tex_mgr;

	TextureObject *		prev;
	TextureObject *		next;

	int					id;					// glBindTexture(id)

	int					priority;
	D3DTEXTUREHANDLE	handle;

	TextureObjectState	state;

	int					num_mipmap_levels;
	MipmapLevel	*		mipmaps[MAX_MIPMAP_LEVELS];

	DWORD				min_filter;
	DWORD				mag_filter;

	bool				wrap_u;
	bool				wrap_v;

	unsigned int		size_in_bytes;

// D3D
	LPDIRECTDRAWSURFACE3	memory_surface;		// master complex memory surface for mip-mapped textures.
	LPDIRECTDRAWSURFACE3	device_surface;

	const PixelFormat *		format;
	GLenum					internal_format;

	TextureObject (void)
	{
		prev = next = 0;

		id = -1;
		priority = 0;
		state = TOS_NEW;

		memory_surface = NULL;
		device_surface = NULL;
		handle = NULL;

		format = NULL;
		internal_format = GL_NONE;

		num_mipmap_levels = 0;
		memset(mipmaps, 0, sizeof(MipmapLevel *) * MAX_MIPMAP_LEVELS);

		min_filter =
		mag_filter = D3DFILTER_LINEAR;

		wrap_u = 
		wrap_v = true;

		size_in_bytes = 0;
	}

	~TextureObject(void)
	{
		for (int i = 0; i < num_mipmap_levels; i++)
		{
			delete mipmaps[i];
			mipmaps[i] = NULL;
		}

		num_mipmap_levels = 0;
		id = 0;
		size_in_bytes = 0;
		format = NULL;
		handle = 0;

		if (memory_surface)
		{
			memory_surface->Release();
			memory_surface = NULL;
		}

		if (device_surface)
		{
			device_surface->Release();
			device_surface = NULL;
		}
	}

	MipmapLevel * create_mipmap(int level, int w, int h, const PixelFormat * format);
	void load_mipmap(int level, int bitmap_format, const void *_bitmap, const RGB *bitmap_palette = NULL, const U8 *alpha_map=0);
	void delete_mipmap(int level);

	void upload_texture(void);

	int verify(void);

	bool create_video_surface(void);
	void destroy_video_surface(void);

	void activate (void);
	void flush(void);

// Do whatever it takes to get this texture ready to use. Make sure
// all surfaces are present and loaded, etc. Returns 1 if it had to
// do anything significant (Restore surfaces, etc.) which means you
// should probably re-activate() the texture.
	int finalize(void);

	void set_priority(float p)
	{
		priority = p * 65535;
	}

	unsigned int get_size(void)
	{
		if (size_in_bytes == 0)
		{
			MipmapLevel ** mm_ptr = mipmaps;
			for (int i = 0; i < num_mipmap_levels; i++, mm_ptr++)
			{
				MipmapLevel * mm = *mm_ptr;
				unsigned int txm_size = mm->width * mm->height;
				if (!format->is_indexed())
				{
					txm_size *= 2;
				}

				size_in_bytes += txm_size; 
			}

		// Add palette size.
			if (format && format->is_indexed())
			{
				size_in_bytes += 256 * 3;
			}
		}

		return size_in_bytes;
	}
};

//---------------------------------------------------------------------------
// TextureMgr
//---------------------------------------------------------------------------

class TextureMgr
{
protected:

	LList<TextureObject>						tlist;
	DynamicArray< TPointer< TextureObject > >	tindex;

	TextureObject *selected;
	TextureObject *current;
	TextureObject txm_temp;		// default texture.

	bool		enable_texture;

	int min_txm_width, max_txm_width;
	int min_txm_height, max_txm_height;

public:

	static bool	use_mipmaps;

	TextureMgr (void)
	{
		enable_texture = false;

		selected = &txm_temp;
		current = NULL;

		use_mipmaps = true;

		TextureObject::tex_mgr = this;
	}

	~TextureMgr(void)
	{
		tlist.free();
	}

	void get_caps(DDCAPS & hwcaps, DDCAPS & swcaps);

	TextureObject *find_texture (int id)
	{
		return tindex[id];
/*  
		// FUTURE: use hash or binary search
		TextureObject *obj = tlist.first();
		while (obj)
		{
			if (obj->id == id)
			{
				break;
			}

			obj = obj->next;
		}
		return obj;
*/
	}

	TextureObject *new_texture (int id)
	{
		TextureObject * txm = tlist.alloc();
		if (txm)
		{
			txm->id = id;
			tindex[id] = txm;
		}
		return txm;
	}

	void delete_texture(TextureObject * obj);
//
// Texture memory management.
//
	bool allocate_texture(TextureObject * obj);

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

	void bind_texture (int id);

	int register_texture (int level, int internal_format, int bitmap_width, int bitmap_height, int bitmap_format, const U8 * bitmap, const RGB *bitmap_palette, const U8 *alpha_map);
	//int register_texture (int level, const U8 *bitmap, int bitmap_width, int bitmap_height, const RGB *bitmap_palette);
	bool flush_textures (int priority, int size=0);

	void set_active (bool e)
	{
		enable_texture = e;
	}

	void begin (void);

// Returns resident memory footprint by default, TOTAL texture footprint if
// total == true.
	unsigned int texture_memory_used(bool all = false);
	unsigned int texture_memory_used(unsigned int n, const unsigned int * textures);

	unsigned int texture_count(bool all = false) const;
	void get_texture_ids(unsigned int * ids, bool all = false) const;

	bool are_textures_resident(unsigned int n, unsigned int * textures, unsigned char * residences);

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

	void set_min_filter (int param)
	{
		DWORD state;
		switch (param)
		{
			case GL_NEAREST:
				state = D3DFILTER_NEAREST;
				break;
			case GL_LINEAR:
				state = D3DFILTER_LINEAR;
				break;
			case GL_NEAREST_MIPMAP_NEAREST:
				state = D3DFILTER_MIPNEAREST;
				break;
			case GL_LINEAR_MIPMAP_NEAREST:
				state = D3DFILTER_MIPLINEAR;
				break;
			case GL_NEAREST_MIPMAP_LINEAR:
				state = D3DFILTER_LINEARMIPNEAREST;
				break;
			case GL_LINEAR_MIPMAP_LINEAR:
				state = D3DFILTER_LINEARMIPLINEAR;
				break;
			default:
				state = D3DFILTER_LINEAR;
				break;
		}

		if (selected && (selected->min_filter != state))
		{
			selected->min_filter = state;
			current = NULL;					// reactivate.
		}
	}

	void set_mag_filter (int param)
	{
		DWORD state;
		switch (param)
		{
			case GL_NEAREST:
				state = D3DFILTER_NEAREST;
				break;
			case GL_LINEAR:
				state = D3DFILTER_LINEAR;
				break;
			default:
				state = D3DFILTER_LINEAR;
				break;
		}

		if (selected && (selected->mag_filter != state))
		{
			selected->mag_filter = state;
			current = NULL;					// reactivate.
		}
	}

	void set_wrap_s(int param)
	{
		bool state;
		switch (param)
		{
		case GL_CLAMP:
			state = false;
			break;
		case GL_REPEAT:
			state = true;
			break;
		}

		if (selected && (selected->wrap_u != state))
		{
			selected->wrap_u = state;
			current = NULL;				// reactivate.
		}
	}

	void set_wrap_t(int param)
	{
		bool state;
		switch (param)
		{
		case GL_CLAMP:
			state = false;
			break;
		case GL_REPEAT:
			state = true;
			break;
		}

		if (selected && (selected->wrap_v != state))
		{
			selected->wrap_v = state;
			current = NULL;				// reactivate.
		}
	}

	void get_tex_param(GLenum name, int * param)
	{
		if (selected)
		{
			switch (name)
			{
				case GL_TEXTURE_MIN_FILTER:
				{
					int state = selected->min_filter;
					switch (state)
					{
						case D3DFILTER_NEAREST:
							*param = GL_NEAREST;
							break;
						case D3DFILTER_LINEAR:
							*param = GL_LINEAR;
							break;
						case D3DFILTER_MIPNEAREST:
							*param = GL_NEAREST_MIPMAP_NEAREST;
							break;
						case D3DFILTER_MIPLINEAR:
							*param = GL_LINEAR_MIPMAP_NEAREST;
							break;
						case D3DFILTER_LINEARMIPNEAREST:
							*param = GL_NEAREST_MIPMAP_LINEAR;
							break;
						case D3DFILTER_LINEARMIPLINEAR:
							*param = GL_LINEAR_MIPMAP_LINEAR;
							break;
						default:
							state = GL_NONE;
							break;
					}
					break;
				}
				case GL_TEXTURE_MAG_FILTER:
				{
					int state = selected->min_filter;
					switch (state)
					{
						case D3DFILTER_NEAREST:
							*param = GL_NEAREST;
							break;
						case D3DFILTER_LINEAR:
							*param = GL_LINEAR;
							break;
						default:
							*param = GL_NONE;
							break;
					}
					break;
				}
			}
		}
		else
		{
			*param = GL_NONE;
		}
	}

	void get_tex_level_param(GLenum pname, GLint level, int * params)
	{
		if (selected)
		{
			switch (pname)
			{
				case GL_TEXTURE_WIDTH:
					if (level >= 0 && level < selected->num_mipmap_levels)
					{
						*params = selected->mipmaps[level]->width;
					}
					else
					{
						*params = 0;
					}
					break;
				case GL_TEXTURE_HEIGHT:
					if (level >= 0 && level < selected->num_mipmap_levels)
					{
						*params = selected->mipmaps[level]->width;
					}
					else
					{
						*params = 0;
					}
					break;
				case GL_TEXTURE_INTERNAL_FORMAT:
					*params = selected->internal_format;
					break;
				case GL_TEXTURE_RED_SIZE:
					*params = selected->format->rwidth;
					break;
				case GL_TEXTURE_GREEN_SIZE:
					*params = selected->format->gwidth;
					break;
				case GL_TEXTURE_BLUE_SIZE:
					*params = selected->format->bwidth;
					break;
				case GL_TEXTURE_ALPHA_SIZE:
					*params = selected->format->awidth;
					break;

				default:
					*params = -1;
			}
		}
		else
		{
			*params = 0;
		}
	}


	GLuint get_texture_binding(void) const
	{
		GLuint result;
		if (selected)
		{
			result = selected->id;
		}
		else
		{
			result = 0;
		}
		return result;
	}
};

//---------------------------------------------------------------------------

#endif