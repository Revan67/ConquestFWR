//---------------------------------------------------------------------------
//
// TEXTURE.H
//
//---------------------------------------------------------------------------

#include "draw.h"
//#include <d3d.h>

#include "matrix.h"	// VECTOR

#define RELEASE(x)			if(x) {(x)->Release();(x)=0;}

#define TF_CHROMA	1

struct RGB
{
	U8 r,g,b;
};

//---------------------------------------------------------------------------
// MATERIAL
//---------------------------------------------------------------------------

struct MATERIAL
{
	float shininess;
	VECTOR ambient;
	VECTOR diffuse;
	VECTOR specular;
	VECTOR emission;
};

//---------------------------------------------------------------------------
// TextureObject
//---------------------------------------------------------------------------

class TextureObject
{
protected:

	friend class TextureMgr;

	TextureObject *next;

	int id;					// glBindTexture(id)

	int priority;			// glTexParameter(priority)

	int width,height;		// Size (should be square)
	int bpp;

// Material

	MATERIAL material[2];	// front and back

#if 0
// D3D

	D3DTEXTUREHANDLE	Handle;
	LPDIRECTDRAWPALETTE	Palette;

	DDPIXELFORMAT		Format;
	LPDIRECTDRAWSURFACE	MemorySurface;
	LPDIRECTDRAWSURFACE	DeviceSurface;
#endif

	U32					Flags;		// misc features
	int					Chroma;		// palette index

public:

// CONSTRUCTION

	int is_indexed (void) const
	{
		return false; //(Format.dwFlags & DDPF_PALETTEINDEXED8);
	}

	int is_rgb (void) const
	{
		return false; //(Format.dwFlags & DDPF_RGB);
	}

	TextureObject (void)
	{
		next = 0;

		id = -1;
		priority = 0;
#if 0
		Handle = 0;
		Palette = 0;
#endif
		Flags = 0;
		Chroma = -1;
	}

	void set_priority (float p)
	{
		if (p < 0)
			p = 0;
		if (p > 1)
			p = 1;
		priority = int(p * 65535);
	}

	HRESULT allocate_video_surface (void);
	// Create video memory surface for texture

	bool allocate_memory_surface (void);
	// Create system memory backing store (can't be lost) for texture

	bool verify_video (void);
	// if suurface IsLost then Restore

	void set_palette (RGB *bitmap_palette);
	// attach color map to video surface

	bool upload_texture (void);
	// copy system memory to video-memory
};

//---------------------------------------------------------------------------
// TextureMgr
//---------------------------------------------------------------------------

class TextureMgr
{
protected:

	TextureObject *head;
	TextureObject *tail;

	TextureObject *active;
	TextureObject txm_temp;		// STATE variable

	U32 R_mask;
	U32 G_mask;
	U32 B_mask;

#if 0
	int texture_format_cnt;
	DDPIXELFORMAT texture_formats[64];
#endif

public:

	TextureMgr (void)
	{
		head = tail = 0;
		active = 0;

		// FUTURE: SAL?
		R_mask = 0xF800;
		G_mask = 0x07C0;
		B_mask = 0x001F;
	}

	TextureObject *find_texture (int id)
	{
		// FUTURE: use hash or binary search
		TextureObject *obj;
		for (obj=head; obj; obj=obj->next)
		{
			if (obj->id == id)
				break;
		}
		return obj;
	}

	void add_texture (TextureObject *txm)
	{
		// FUTURE: maybe insert in PRIORITY order?

		if (head == 0)
			head = txm;
		if (tail)
			tail->next = txm;
		tail = txm;
		tail->next = 0; // MAKE SURE THIS IS THE END!
	}

	TextureObject *new_texture (int id)
	{
		TextureObject *txm = new TextureObject;
		if (txm)
		{
			txm->id = id;
			// FUTURE: use STATE to initialize
			add_texture(txm);
		}
		return txm;
	}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

	void set_active (bool b)
	{
	//	active = b;
	}

	void begin (void)
	{
	}

	bool flush_textures (int priority, int size=0)
	{
		return false;
	}

	bool verify_texture (TextureObject *txm)
	{
		return false;
	}

	void bind_texture (int id)
	{
	}

	bool load_texture (TextureObject *txm, void *_bitmap, int bitmap_width, int bitmap_height, RGB *bitmap_palette)
	{
		return false;
	}

	int register_texture (U8 *bitmap, int bitmap_width, int bitmap_height, RGB *bitmap_palette)
	{
		return 0;
	}

	void set_min_filter (int param)
	{
	}

	void set_mag_filter (int param)
	{
	}

	void set_wrap_s (int param)
	{
	}

	void set_wrap_t (int param)
	{
	}
};

//---------------------------------------------------------------------------

