//---------------------------------------------------------------------------
/*
	TEXTURE.CPP

	Copyright (C) 1997 Digital Anvil, Inc.

	Created: October 1997

	Author: Bill Baldwin
*/
//---------------------------------------------------------------------------

//
// If a given TextureObject is resident, ALL its mipmap levels must be resident.
// To make it easier to manage, we currently load and unload ALL mipmap levels
// for a given texture object. In the future we should be clever and allow
// individual mipmap levels to be resident or not. 
//

#define DIRECTDRAW_VERSION 0x0500	// Ensure we can run with DDraw 5 and up

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <limits.h>

#include "texture.h"
#include "draw.h"
#include "pixel.h"

extern LPDIRECTDRAW2		lpDD;
extern LPDIRECT3D2			lpD3D;
extern LPDIRECT3DDEVICE2	D3DDevice;
#ifdef RSTATE_CHECK
extern DWORD				*render_states;    // state tracking of global d3d device
#endif

int texture_format_cnt;
PixelFormat texture_formats[64];

//

void DebugPrint (char *fmt, ...);

//

TextureMgr *TextureObject::tex_mgr = NULL;
bool TextureMgr::use_mipmaps = true;

//

#define PIXEL_VALUE(x) (((((U32) (x)->r) >> (red_right)) << red_left) | \
                        ((((U32) (x)->g) >> (grn_right)) << grn_left) | \
                        ((((U32) (x)->b) >> (blu_right)) << blu_left))

#define PIXEL_RGBA(r, g, b, a) ((((r) >> (red_right)) << red_left) | \
								(((g) >> (grn_right)) << grn_left) | \
								(((b) >> (blu_right)) << blu_left) | \
								(((a) >> (alp_right)) << alp_left))

inline void set_render_state( D3DRENDERSTATETYPE which, DWORD newstate )
{
#ifdef RSTATE_CHECK
	if (render_states[(int) which] != newstate)
	{
		render_states[(int) which]= newstate;
		D3DDevice->SetRenderState(which, newstate);
	}
#else
	D3DDevice->SetRenderState(which, newstate);
#endif
}

//

void TextureMgr::get_caps(DDCAPS & hwcaps, DDCAPS & swcaps)
{
	if (D3DDevice)
	{
		D3DDEVICEDESC d3dhwcaps, d3dhelcaps;
		d3dhwcaps.dwSize = d3dhelcaps.dwSize = sizeof(D3DDEVICEDESC);
		if (D3DDevice->GetCaps(&d3dhwcaps, &d3dhelcaps) == DD_OK)
		{
			min_txm_width = d3dhwcaps.dwMinTextureWidth;
			min_txm_height = d3dhwcaps.dwMinTextureHeight;

			max_txm_width = d3dhwcaps.dwMaxTextureWidth;
			max_txm_height = d3dhwcaps.dwMaxTextureHeight;
		}
	}

	if (!use_mipmaps)
		return;

// Try creating mipmap surface to see if mipmapping supported in HW.
	DDSURFACEDESC desc;
	memset(&desc, 0, sizeof(DDSURFACEDESC));

	desc.dwSize          = sizeof(DDSURFACEDESC);
	desc.dwFlags         = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT | DDSD_MIPMAPCOUNT;
	desc.ddsCaps.dwCaps  = DDSCAPS_COMPLEX | DDSCAPS_TEXTURE | DDSCAPS_MIPMAP | DDSCAPS_VIDEOMEMORY | DDSCAPS_ALLOCONLOAD;
	desc.dwWidth         = 128;
	desc.dwHeight        = 128;
	desc.ddpfPixelFormat = texture_formats[0].ddpf;
	desc.dwMipMapCount   = 3;

	LPDIRECTDRAWSURFACE surf;
	HRESULT ddrval = lpDD->CreateSurface(&desc, &surf, NULL);
	if (ddrval != DD_OK)
	{
		if (ddrval == DDERR_NOMIPMAPHW)
		{
			DebugPrint("GLD3D: Hardware doesn't support mipmapping. Disabling.\n");
			use_mipmaps = false;
		}
	}
	else
	{
		DebugPrint("GLD3D: Hardware supports mipmapping.\n");
		use_mipmaps = true;
		surf->Release();
	}
}

//

int AdjustTextureSize(int size)
{
	int result;

	if (size <= 1)
	{
		result = 1;
	}
	else if (size <= 2)
	{
		result = 2;
	}
	else if (size <= 4)
	{
		result = 4;
	}
	else if (size <= 8)
	{
		result = 8;
	}
	else if (size <= 16)
	{
		result = 16;
	}
	else if (size <= 32)
	{
		result = 32;
	}
	else if (size <= 64)
	{
		result = 64;
	}
	else if (size <= 128)
	{
		result = 128;
	}
	else 
	{
		result = 256;
	}

	return result;
}

//---------------------------------------------------------------------------
// MipmapLevel
//---------------------------------------------------------------------------

bool MipmapLevel::create_system_surface(const PixelFormat * format)
{
//
// Deal with size issues.
//
	int surface_w = AdjustTextureSize(width);
	int surface_h = AdjustTextureSize(height);

//
// Create system memory backing store (can't be lost) for texture
// We use this system memory surface as the source surface for ::Load
//
	DDSURFACEDESC desc;
	memset(&desc, 0, sizeof(DDSURFACEDESC));

	desc.dwSize          = sizeof(DDSURFACEDESC);
	desc.dwFlags         = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT; 
	desc.ddsCaps.dwCaps  = DDSCAPS_TEXTURE | DDSCAPS_SYSTEMMEMORY;
	desc.dwWidth         = surface_w;
	desc.dwHeight        = surface_h;
	desc.ddpfPixelFormat = format->ddpf;

	if (TextureMgr::use_mipmaps)
	{
		desc.ddsCaps.dwCaps |= DDSCAPS_MIPMAP;
	}

	LPDIRECTDRAWSURFACE surf;
	HRESULT ddrval = lpDD->CreateSurface(&desc, &surf, NULL);

	if (ddrval != DD_OK)
	{
		DebugPrint("GLD3D: %s\n",DD_message(ddrval));
		surf = NULL;
	}

	if (surf)
	{
		surf->QueryInterface(IID_IDirectDrawSurface3, (void **) &memory_surface);
		surf->Release();
	}

	return (memory_surface != NULL);
}

//

bool TextureMgr::allocate_texture(TextureObject * txm)
{
	if (!txm->device_surface)
	{
		DebugPrint("GLD3D: Allocating %d bytes of video memory for texture %d.\n", txm->get_size(), txm->id);
		while (!txm->create_video_surface())
		{
			int dumped = 0;

		// Start at the beginning of the list, which is the LRU texture. Everytime we
		// select a new texture, we move it to the end.

			TextureObject * obj = tlist.first();
			while (obj)
			{
				if (obj->device_surface && (obj->priority < txm->priority))
				{
					obj->destroy_video_surface();
					dumped++;
					DebugPrint("GLD3D:    Dumping texture %d (%d bytes).\n", obj->id, obj->get_size());
					break;
				}

				obj = obj->next;
			}

			if (!dumped)
			{
				obj = tlist.first();
				while (obj)
				{
					if (obj->device_surface)
					{
						obj->destroy_video_surface();
						DebugPrint("GLD3D:    Dumping texture %d (%d bytes).\n", obj->id, obj->get_size());
						break;
					}

					obj = obj->next;
				}
			}
		}

		DebugPrint("GLD3D: Successfully allocated video memory for texture %d.\n", txm->id);
	}
// Foolishly assume it will always load eventually.
	return true;
}

//

unsigned int TextureMgr::texture_memory_used(bool all)
{
	unsigned int result = 0;

	TextureObject * txm = tlist.first();
	while (txm)
	{
		if (txm->device_surface || all)
		{
			result += txm->get_size();
		}

		txm = txm->next;
	}

	return result;
}

//

unsigned int TextureMgr::texture_memory_used(unsigned int n, const unsigned int * textures)
{
	unsigned int result = 0;
	
	for (unsigned int i = 0; i < n; i++, textures++)
	{
		TextureObject * txm = find_texture(*textures);
		if (txm)
		{
			result += txm->get_size();
		}
	}

	return result;
}

unsigned int TextureMgr::texture_count(bool all) const
{
	unsigned int result = 0;
	if (all)
	{
		result = tlist.count();
	}
	else
	{
		TextureObject * txm = tlist.first();
		while (txm)
		{
			if (txm->device_surface)
			{
				result++;
			}
			txm = txm->next;
		}
	}
	return result;
}

void TextureMgr::get_texture_ids(unsigned int * ids, bool all) const
{
	TextureObject * txm = tlist.first();
	while (txm)
	{
		if (txm->device_surface || all)
		{
			*ids++ = txm->id;
		}
		txm = txm->next;
	}
}

bool TextureMgr::are_textures_resident(unsigned int n, unsigned int * textures, unsigned char * residences)
{
// Returns true ONLY IF ALL TEXTURES ARE RESIDENT, false otherwise.
// Only fills in "residences" array if returning FALSE.

	bool all_resident = true;
	if (n > 0)
	{
		unsigned char * res = new unsigned char[n];
		unsigned char * dst = res;
		unsigned int * id = textures;
		for (unsigned int i = 0; i < n; i++, id++, dst++)
		{
			TextureObject * txm = find_texture(*id);
			if (txm)
			{
				if (txm->device_surface)
				{
					*dst = GL_TRUE;
				}
				else
				{
					*dst = GL_FALSE;
					all_resident = false;
				}
			}
			else
			{
				*dst = GL_FALSE;	// ERROR condition?
			}
		}

		if (!all_resident)
		{
			memcpy(residences, res, sizeof(unsigned char) * n);
		}

		delete [] res;
	}

	return all_resident;
}

//

enum TVResult
{
	TV_ERROR,		// device surface not present, or lost but not restored.
	TV_RESTORED,	// was lost, but restored.
	TV_OK			// present, not lost.
};

//

int TextureObject::verify(void)
{
	int result = TV_ERROR;

// Check IsLost() on video surface, restore if necessary.
	if (device_surface)
	{
		HRESULT ddrval = device_surface->IsLost();
		if (ddrval == DDERR_SURFACELOST)
		{
			if (device_surface->Restore() == DD_OK)
			{
				result = TV_RESTORED;
			}
			else
			{
			// Can't restore surface, possibly because video mode has changed.
			// Rebuild it instead.
				device_surface->Release();
				device_surface = NULL;
				//if (create_video_surface())
				if (tex_mgr->allocate_texture(this))
				{
					result = TV_RESTORED;
				}
			}
		}
		else if (ddrval == DD_OK)
		{
			result = TV_OK;
		}
	}

	return result;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void MipmapLevel::set_palette (const RGB *bitmap_palette)
{
//
// Delete existing palette, if any
//
	if (palette != NULL)
	{
		palette->Release();
	}
//
// Create a DirectDraw palette corresponding to the input VFX_RGB array
//
	PALETTEENTRY entries[256];
	memset(entries, 0, sizeof(PALETTEENTRY) * 256);

	for (int i = 0; i < 256; i++)
	{
		entries[i].peRed   = bitmap_palette[i].r;
		entries[i].peGreen = bitmap_palette[i].g;
		entries[i].peBlue  = bitmap_palette[i].b;
	}

	HRESULT ddrval = lpDD->CreatePalette(DDPCAPS_8BIT | DDPCAPS_ALLOW256, entries, &palette, NULL);
	if (ddrval != DD_OK)
	{
		DebugPrint("GLD3D: %s\n",DD_message(ddrval));
	}

	memory_surface->SetPalette(palette);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void TextureObject::upload_texture (void)
{
	if (!device_surface)
	{
	// This is an error condition. device_surface should have been allocated
	// before this is called.
		DebugPrint("GLD3D: No device surface for texture.\n");
		return;
	}

	DDSCAPS caps;
	caps.dwCaps = DDSCAPS_TEXTURE;
	if (TextureMgr::use_mipmaps)
	{
		caps.dwCaps |= DDSCAPS_MIPMAP;
	}

	LPDIRECTDRAWSURFACE3 source_surface = NULL;

	if (num_mipmap_levels > 1)
	{
	//
	// Deal with mipmaps.
	//
		if (memory_surface)
		{
		// Be sure memory_surface mipmap count corresponds to actual number of mipmap levels.

			int num_surface_levels = -1;

			DDSURFACEDESC desc;
			memset(&desc, 0, sizeof(desc));
			desc.dwSize = sizeof(desc);
			memory_surface->GetSurfaceDesc(&desc);
			if (desc.dwFlags & DDSD_MIPMAPCOUNT)
			{
				num_surface_levels = desc.dwMipMapCount;
			}
			if (num_mipmap_levels != num_surface_levels)
			{
				DebugPrint("GLD3D: Mipmap level mismatch.\n");
			}
		}
		else
		{
		// Create complex memory surface to hold all mipmap levels.
			LPDIRECTDRAWSURFACE mem;
			DDSURFACEDESC desc;
			memset(&desc, 0, sizeof(DDSURFACEDESC));

			int w = mipmaps[0]->width;
			int h = mipmaps[0]->height;
			desc.dwSize          = sizeof(DDSURFACEDESC);
			desc.dwFlags         = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT | DDSD_MIPMAPCOUNT; 
			desc.ddsCaps.dwCaps  = DDSCAPS_COMPLEX | DDSCAPS_TEXTURE | DDSCAPS_SYSTEMMEMORY | DDSCAPS_MIPMAP;
			desc.dwWidth         = w;
			desc.dwHeight        = h;
			desc.ddpfPixelFormat = format->ddpf;
			desc.dwMipMapCount	 = num_mipmap_levels;

			HRESULT err = lpDD->CreateSurface(&desc, &mem, NULL);
			if (err != DD_OK)
			{
		 		DebugPrint("GLD3D: Failed to create complex memory surface: %s\n", DD_message(err));
			}
			err = mem->QueryInterface(IID_IDirectDrawSurface3, (void **) &memory_surface);
			if (err != DD_OK)
			{
		 		DebugPrint("GLD3D: Failed to get DirectDrawSurface3 for memory surface: %s\n", DD_message(err));
			}

			if (format->is_indexed())
			{
				err = memory_surface->SetPalette(mipmaps[0]->palette);
				if (err != DD_OK)
				{
					DebugPrint("GLD3D: SetPalette() failed: %s\n", DD_message(err));
				}
			}

		// Copy individual mipmap levels to complex mipmap surface.
			MipmapLevel ** mm_ptr = mipmaps;
			LPDIRECTDRAWSURFACE3 surf = memory_surface;

			for (int i = 0; i < num_mipmap_levels; i++)
			{
				MipmapLevel * mm = *(mm_ptr++);
			
				RECT rect;
				rect.left	= 0;
				rect.top	= 0;
				rect.right	= mm->width;
				rect.bottom	= mm->height;
			
			// Blt mipmap surface to complex surface.
				err = surf->Blt(&rect, mm->memory_surface, &rect, DDBLT_WAIT, NULL);
				if (err != DD_OK)
				{
			 		DebugPrint("GLD3D: Error: %s\n", DD_message(err));
				}

			// Get rid of memory surface in mipmap level since we've got a copy of it in
			// the complex surface.
				mm->memory_surface->Release();
				mm->memory_surface = NULL;

			// Get next surface.
				if (i < num_mipmap_levels - 1)
				{
					err = surf->GetAttachedSurface(&caps, &surf);
					if (err != DD_OK)
					{
			 			DebugPrint("GLD3D: Error: %s\n", DD_message(err));
					}
				}
			}
		}

		source_surface = memory_surface;
	}
	else
	{
		source_surface = mipmaps[0]->memory_surface;
	}

	IDirect3DTexture2 * src;
	IDirect3DTexture2 * dst;

	source_surface->QueryInterface(IID_IDirect3DTexture2, (void **) &src);
	device_surface->QueryInterface(IID_IDirect3DTexture2, (void **) &dst);

	HRESULT ddrval = dst->Load(src);
	if (ddrval != DD_OK)
	{
		DebugPrint("GLD3D: upload_texture(): %s\n", DD_message(ddrval));
	}

	dst->Release();
	src->Release();

	state = TOS_READY;
}

//---------------------------------------------------------------------------
// TextureObject
//---------------------------------------------------------------------------

MipmapLevel * TextureObject::create_mipmap(int level, int w, int h, const PixelFormat * pixel_format)
{
	if (num_mipmap_levels == 0)
	{
		format = pixel_format;
	}

// Delete any existing mipmap.
	if (mipmaps[level] != NULL)
	{
		delete_mipmap(level);
	}
	else
	{
		num_mipmap_levels++;
	}

	mipmaps[level] = new MipmapLevel;

	MipmapLevel * mm = mipmaps[level];

	mm->width = w;
	mm->height = h;

	mm->create_system_surface(format);

	state = TOS_LEVELS_PRESENT;

	return mipmaps[level];
}

//

void TextureObject::delete_mipmap(int level)
{
	MipmapLevel * mm = mipmaps[level];
	if (mm)
	{
	// Unlink mipmap's device surface from chain.

	// Destroy mipmap.
		delete mm;
	}
}

//

void TextureObject::load_mipmap(int level, int bitmap_format, const void *_bitmap, const RGB *bitmap_palette, const U8 *alpha_map)
{
	const void *bitmap = _bitmap;

	MipmapLevel * mm = mipmaps[level];
//
// Copy texture bitmap to system memory surface
//
	if (bitmap != NULL)
	{
	//
	// Get scaling constants for copy to system memory surface
	//
		int surface_w = AdjustTextureSize(mm->width);
		int surface_h = AdjustTextureSize(mm->height);

		float du = float(mm->width)  / float(surface_w);
		float dv = float(mm->height) / float(surface_h);
	//
	// Copy texture bitmap to system memory surface
	//
		void *screen;
		S32   stride;

		DDSURFACEDESC ddsd;
		memset(&ddsd, 0, sizeof(ddsd));
		ddsd.dwSize = sizeof(ddsd);

	//
	// Lock surface.
	//
		HRESULT ddrval;
		do
		{
			ddrval = mm->memory_surface->Lock(NULL, &ddsd, 0, NULL);
		} 
		while (ddrval != DD_OK);

		screen = (void *) ddsd.lpSurface;
		stride = ddsd.lPitch;

	// 
	// Cases handled:
	//
	// 8-bit palettized --> RGB.
	// 8-bit palettized --> 8-bit palettized.
	// 32-bit RGBA      --> 5551 RGBA.
	// 32-bit RGBA		--> 4444 RGBA.
	//
		if (bitmap_format == GL_COLOR_INDEX)
		{
			if (format->is_indexed())
			{
				SINGLE v = 0.0F;
				for (S32 y = 0; y < surface_h; y++, v += dv)
				{
					U8 *src_map = &((U8 *) bitmap)[mm->width * (S32) v];

					SINGLE u = 0.0F;
					for (S32 x = 0; x < surface_w; x++, u += du)
					{
						((U8 *) screen)[x] = src_map[(S32) u];
					}

					screen = ((U8 *) screen) + stride;
				}

				//if (level == 0)
				{
					mm->set_palette(bitmap_palette);
				}
			}
			else
			{
				SINGLE v = 0.0F;
				for (S32 y = 0; y < surface_h; y++, v += dv)
				{
					U8 *src_map = &((U8 *) bitmap)[mm->width * (S32) v];

					SINGLE u = 0.0F;

					for (S32 x=0; x < surface_w; x++,u += du)
					{
						//((U16 *) screen)[x] = (U16) PIXEL_VALUE(&bitmap_palette[src_map[(S32) u]]);
						const RGB * pal = bitmap_palette + src_map[(S32) u];
						((U16 *) screen)[x] = format->compute(pal->r, pal->g, pal->b, alpha_map[src_map[(S32) u]]);
					}

					screen = ((U8 *) screen) + stride;
				}
			}
		}
		else if (bitmap_format == GL_RGBA)
		{
		// Copying from 32-bit RGBA buffer to 16-bit surface.
			SINGLE v = 0.0f;
			for (S32 y = 0; y < surface_h; y++, v += dv)
			{
				SINGLE u = 0.0f;

				U32 *src_map = &((U32 *) bitmap)[mm->width * (S32) v];

				for (S32 x = 0; x < surface_w; x++, u += du)
				{
				// COPY FROM RGBA.
					U8 * pix = (U8 *) (src_map + S32(u));
					U32 r = *pix++;
					U32 g = *pix++;
					U32 b = *pix++;
					U32 a = *pix++;
					((U16 *) screen)[x] = (U16) format->compute(r, g, b, a);
				}

				screen = ((U8 *) screen) + stride;
			}
		}
		else if (bitmap_format == GL_RGB)
		{
		// Copying from 24-bit RGB buffer to 16-bit surface.
			SINGLE v = 0.0f;
			for (S32 y = 0; y < surface_h; y++, v += dv)
			{
				SINGLE u = 0.0f;

				U8 *src_map = &((U8 *) bitmap)[mm->width * 3 * (S32) v];

				for (S32 x = 0; x < surface_w; x++, u += du)
				{
				// COPY FROM RGB.
					U8 * pix = src_map + S32(u) * 3;
					U32 r = *pix++;
					U32 g = *pix++;
					U32 b = *pix++;
					((U16 *) screen)[x] = (U16) format->compute(r, g, b, 0);
				}

				screen = ((U8 *) screen) + stride;
			}
		}

	// Adjust dimensions to reflect reality.
		mm->width = surface_w;
		mm->height = surface_h;

	//
	// UNLOCK surface.
	//
		do
		{
			ddrval = mm->memory_surface->Unlock(NULL);
		} 
		while (ddrval != DD_OK);
	}
}

//

void TextureObject::activate (void)
{
	set_render_state(D3DRS_TEXTUREHANDLE, handle);
	set_render_state(D3DRS_COLORKEYENABLE, 0);

	set_render_state(D3DRS_TEXTUREMIN, min_filter);
	set_render_state(D3DRS_TEXTUREMAG, mag_filter);

	if (wrap_u)
	{
		set_render_state(D3DRS_TEXTUREADDRESSU, D3DTADDRESS_WRAP);
	}
	else
	{
		set_render_state(D3DRS_TEXTUREADDRESSU, D3DTADDRESS_CLAMP);
	}
	if (wrap_v)
	{
		set_render_state(D3DRS_TEXTUREADDRESSV, D3DTADDRESS_WRAP);
	}
	else
	{
		set_render_state(D3DRS_TEXTUREADDRESSV, D3DTADDRESS_CLAMP);
	}

//   	set_render_state(D3DRS_WRAPU, 0);
//   	set_render_state(D3DRS_WRAPV, 0);
}

//

void TextureObject::flush(void)
{
	if (device_surface != NULL)
	{
		device_surface->Release();
		device_surface = NULL;
		state = TOS_LEVELS_PRESENT;
	}
}

//

//---------------------------------------------------------------------------
// TextureMgr
//---------------------------------------------------------------------------

bool TextureMgr::flush_textures (int priority, int size)
{
	TextureObject *obj = tlist.first();
	while (obj)
	{
		if (obj->priority < priority)
		{
			// FUTURE: should size matter?
			obj->flush();
			// FUTURE: flush one and try again?
		}
		obj = obj->next;
	}
	return false;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void TextureMgr::bind_texture (int id)
{
	if (!selected || (selected->id != id))
	{
		TextureObject *txm = find_texture(id);

		if (txm == 0)
		{
			txm = new_texture(id);
			if (txm == 0)
			{
				DebugPrint("Failed to create TextureObject in bind_texture.\n");
				return;
			}
		}

		selected = txm;

	// Move selected texture to the end of the list, so LRU is always at the head.
		tlist.unlink(selected);
	// The NULL parameter means it gets inserted at the end of the list.
		tlist.link(selected, NULL);
	}
}

//

void TextureMgr::delete_texture(TextureObject * obj)
{
	if (obj == selected)
	{
		set_render_state(D3DRS_TEXTUREHANDLE, 0);
		selected = NULL;
		current = NULL;
	}
	tindex[obj->id] = NULL;
	tlist.free(obj);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void TextureMgr::begin (void)
{
// Make sure current texture is ready to go.

	if (enable_texture)
	{
		if (selected)
		{
			int reactivate = selected->finalize();
			if ((current != selected) || reactivate)
			{
				selected->activate();
				current = selected;
			}
		}
	}
	else // turn off texturing
	{
#if !DEBUG
		set_render_state(D3DRS_TEXTUREHANDLE, 0);
#endif
		current = 0;
	}
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

//****************************************************************************
//*                                                                          *
//*  Private method: return a surface's native color DWORD value for a       *
//*  given RGB value                                                         *
//*                                                                          *
//****************************************************************************

U32 native_color_value (IDirectDrawSurface *pdds, S32 r, S32 g, S32 b)
{
	COLORREF      rgbT;
	HDC           hdc;
	DWORD         dw = CLR_INVALID;
	DDSURFACEDESC ddsd;
	HRESULT       hres;

//
// Get device context for surface
//
	if (pdds->GetDC(&hdc) != DD_OK)
	{
		return -1;
	}

//
// Save current pixel value at (0,0)
//
	rgbT = GetPixel(hdc, 0, 0);

//
// Store our RGB value at (0,0)
//
	SetPixel(hdc, 0, 0, RGB(r,g,b));

//
// Release surface HDC to permit lock
//
	pdds->ReleaseDC(hdc);

//
// Lock the surface so we can read back the converted color
//
	memset(&ddsd, 0, sizeof(ddsd));
	ddsd.dwSize = sizeof(ddsd);

	while ((hres = pdds->Lock(NULL, &ddsd, 0, NULL)) != DD_OK)
	{
	}

//
// Read color DWORD and mask extraneous bits
//
	dw = *(U32 *) ddsd.lpSurface;                   
	dw &= (1 << ddsd.ddpfPixelFormat.dwRGBBitCount)-1;

//
// Release the surface
//
	pdds->Unlock(NULL);

//
// Reacquire DC and restore the original color at (0,0)
//
	if (pdds->GetDC(&hdc) != DD_OK)
	{
		return dw;
	}

	SetPixel(hdc, 0, 0, rgbT);

//
// Release surface HDC and return color
//
	pdds->ReleaseDC(hdc);

	return dw;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

int TextureMgr::register_texture (int level, int internal_format, int bitmap_width, int bitmap_height, int format, const U8 * bitmap, const RGB *bitmap_palette, const U8 *alpha_map)
{
	if ((level < 0) || (level >= MAX_MIPMAP_LEVELS))
	{
		DebugPrint("GLD3D: Invalid mipmap level specified.\n");
		return -1;
	}

	if (!use_mipmaps && (level > 0))
	{
		return -1;
	}

	TextureObject *txm = (selected == NULL) ? &txm_temp : selected;

//
// Select storage format for texture
// 
	int format_index = -1;
	switch (internal_format)
	{
		case 3:
		case GL_RGB:
		case GL_COLOR_INDEX8_EXT:
		{
			PixelFormat * pf;
			if (format == GL_COLOR_INDEX)
			{
			// Palettized image. Look for 8-bit palettized format.
				pf = texture_formats;
				for (int i = 0; i < texture_format_cnt; i++, pf++)
				{
					if ((pf->ddpf.dwRGBBitCount == 8) && (pf->ddpf.dwFlags & DDPF_PALETTEINDEXED8))
					{
						format_index = i;
						break;
					}
				}
			}

			if (format_index == -1)
			{
			// No palettized format available. Find format matching current video mode?
				pf = texture_formats;
				for (int i = 0; i < texture_format_cnt; i++, pf++)
				{
					if ((pf->ddpf.dwRGBBitCount > 8) && (pf->ddpf.dwFlags & DDPF_RGB))
					{
						PixelFormat * spf = &screen_pixel_format;
						if ((pf->get_r_mask() == spf->get_r_mask()) &&
							(pf->get_g_mask() == spf->get_g_mask()) &&
							(pf->get_b_mask() == spf->get_b_mask()))
						{
							format_index = i;
							break;
						}
					}
				}
			}

			if (format_index == -1)
			{
			// Settle for whatever 15- or 16-bit RGB format is available.
				pf = texture_formats;
				for (int i = 0; i < texture_format_cnt; i++, pf++)
				{
					if (((pf->ddpf.dwRGBBitCount == 15) || (pf->ddpf.dwRGBBitCount == 16)) && (pf->ddpf.dwFlags & DDPF_RGB))
					{
						format_index = i;
						break;
					}

				}
			}
			break;
		}

		case GL_RGB5_A1:
		{
		// RGBA texture with 1-bit alpha channel.
			PixelFormat * pf = texture_formats;
			for (int i = 0; i < texture_format_cnt; i++, pf++)
			{
			// SOME CARDS, e.g. Permedia2, support 2321 RGBA texture formats. Make sure we're
			// getting a 16-bit format.
				if (pf->ddpf.dwRGBBitCount > 8)
				{
					if ((pf->ddpf.dwFlags & DDPF_RGB) && (pf->ddpf.dwFlags & DDPF_ALPHAPIXELS))
					{
						DWORD alpha_mask = pf->ddpf.dwRGBAlphaBitMask;
						while (!(alpha_mask & 1))
						{
							alpha_mask >>= 1;
						}

						if (alpha_mask == 1)
						{
						// 1-bit RGBA format found.
							format_index = i;
							break;
						}
					}
				}
			}
			break;
		}

		case GL_RGBA4:
		{
		// RGBA texture with 4-bit alpha channel. 
			PixelFormat * pf = texture_formats;
			for (int i = 0; i < texture_format_cnt; i++, pf++)
			{
				if ((pf->ddpf.dwFlags & DDPF_RGB) && (pf->ddpf.dwFlags & DDPF_ALPHAPIXELS))
				{
					DWORD alpha_mask = pf->ddpf.dwRGBAlphaBitMask;
					while (!(alpha_mask & 1))
					{
						alpha_mask >>= 1;
					}

					if (alpha_mask == 0xf)
					{
					// 4-bit RGBA format found.
						format_index = i;
						break;
					}
				}
			}
			break;
		}
	}

	if (format_index == -1)
	{
		DebugPrint("GLD3D: No suitable texture format available.\n");
		return -1;
	}

	txm->internal_format = (GLenum) internal_format;
	MipmapLevel * mm = txm->create_mipmap(level, bitmap_width, bitmap_height, texture_formats + format_index);
//
// Set texture map
//
	txm->load_mipmap(level, format, bitmap, bitmap_palette, alpha_map);

	return (int) txm;
}

//

bool TextureObject::create_video_surface(void)
{
	if (device_surface != NULL)
	{
	// TODO: maybe we don't need to release and start over if old surface properties
	// are still correct.
		device_surface->Release();
		device_surface = NULL;
	}
//
// Create video memory surface for texture
//
	int width = mipmaps[0]->width;
	int height = mipmaps[0]->height;

	DDSURFACEDESC desc;
	memset(&desc, 0, sizeof(DDSURFACEDESC));

	desc.dwSize          = sizeof(DDSURFACEDESC);
	desc.dwFlags         = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT;
	desc.ddsCaps.dwCaps  = DDSCAPS_TEXTURE | DDSCAPS_VIDEOMEMORY | DDSCAPS_ALLOCONLOAD;
	desc.dwWidth         = width;
	desc.dwHeight        = height;
	desc.ddpfPixelFormat = format->ddpf;

	if ((num_mipmap_levels > 1) && TextureMgr::use_mipmaps)
	{
		desc.dwFlags		|= DDSD_MIPMAPCOUNT;
		desc.ddsCaps.dwCaps |= DDSCAPS_COMPLEX | DDSCAPS_MIPMAP;
		desc.dwMipMapCount   = num_mipmap_levels;
	}

	LPDIRECTDRAWSURFACE surf;
	HRESULT ddrval = lpDD->CreateSurface(&desc, &surf, NULL);

	if (ddrval != DD_OK)
	{
		DebugPrint("GLD3D: %s\n", DD_message(ddrval));
		surf = NULL;
	}

	if (surf)
	{
		if (format->is_indexed())
		{
			if (mipmaps[0]->palette)
			{
				HRESULT ddrval = surf->SetPalette(mipmaps[0]->palette);
				if (ddrval != DD_OK)
				{
					DebugPrint("GLD3D: SetPalette() failed: %s\n", DD_message(ddrval));
				}
			}
		}

		if (surf->QueryInterface(IID_IDirectDrawSurface3, (void **) &device_surface) == DD_OK)
		{
			surf->Release();

			state = TOS_DEVICE_PRESENT;

			IDirect3DTexture2 * texture;
			if (device_surface->QueryInterface(IID_IDirect3DTexture2, (void **) &texture) == DD_OK)
			{
				texture->GetHandle(D3DDevice, &handle);
				texture->Release();
			}
			else
			{
				DebugPrint("GLD3D: Can't get Texture interface: %s\n", DD_message(ddrval));
				handle = 0;
			}
		}
		else
		{
		 	DebugPrint("GLD3D: Can't get DirectDrawSurface3 interface: %s\n", DD_message(ddrval));
			device_surface = NULL;
			handle = 0;
		}
	}
	else
	{
		device_surface = NULL;
	}

	return (device_surface != NULL);
}

//

void TextureObject::destroy_video_surface(void)
{
	if (device_surface)
	{
		device_surface->Release();
		device_surface = NULL;

		state = TOS_LEVELS_PRESENT;
	}
}

//

int TextureObject::finalize(void)
{
	int reactivate = 0;

	switch (state)
	{
		case TOS_LEVELS_PRESENT:
			tex_mgr->allocate_texture(this);
			upload_texture();
			reactivate = 1;
			break;

		case TOS_DEVICE_PRESENT:
		{
		// Make sure device surface is okay, then upload texture.
			int vr = verify();
			if (vr != TV_ERROR)
			{
				upload_texture();
				reactivate = 1;
			}
			break;
		}

		case TOS_READY:
		{
		// If device surface is lost, restore it and re-upload the texture.
			int vr = verify();
			if (vr == TV_RESTORED)
			{
				upload_texture();
				reactivate = 1;
			}
			break;
		}
	}

	return reactivate;
}

//



//

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -


//
// Set chroma-key for texture surface, if enabled
//

/*
NO CHROMAKEY FOR NOW.
	if (mm->Flags & TF_CHROMA)
	{
		DDCOLORKEY ddck;

		if (mm->is_indexed())
		{
			ddck.dwColorSpaceLowValue = ddck.dwColorSpaceHighValue = mm->Chroma;
		}
		else
		{
			const RGB *key = bitmap_palette + mm->Chroma;

// pci - Why isn't this a palette index?

			ddck.dwColorSpaceLowValue =
			ddck.dwColorSpaceHighValue = 
				native_color_value(mm->memory_surface, key->r,key->g,key->b);
		}

		mm->memory_surface->SetColorKey(DDCKEY_SRCBLT, &ddck);

		if (mm->device_surface != NULL)
		{
			mm->device_surface->SetColorKey(DDCKEY_SRCBLT, &ddck);
		}
	}
*/

