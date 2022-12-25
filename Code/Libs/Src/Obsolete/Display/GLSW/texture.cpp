//---------------------------------------------------------------------------
//
// TEXTURE.CPP
//
//---------------------------------------------------------------------------

#define DIRECTDRAW_VERSION 0x0500	// Ensure we can run with DDraw 5 and up

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "texture.h"
//#define INITGUID
#include "draw.h"

extern LPDIRECTDRAW			lpDD;
extern LPDIRECT3D2			lpD3D;
extern LPDIRECT3DDEVICE2	lpD3DDevice;

extern void DebugPrint (...);
extern char *DD_message (int id);

//---------------------------------------------------------------------------
// TextureObject
//---------------------------------------------------------------------------

HRESULT TextureObject::allocate_video_surface (void)
{
//
// Create video memory surface for texture
//
// (OK for this to fail -- surface will be assigned on demand)
//
	HRESULT ddrval;

	DDSURFACEDESC vidmem;
	memset(&vidmem, 0, sizeof(DDSURFACEDESC));

	vidmem.dwSize          = sizeof(DDSURFACEDESC);
	vidmem.dwFlags         = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT; 
	vidmem.ddsCaps.dwCaps  = DDSCAPS_TEXTURE  | DDSCAPS_VIDEOMEMORY | DDSCAPS_ALLOCONLOAD;
	vidmem.dwWidth         = width;
	vidmem.dwHeight        = height;
	vidmem.ddpfPixelFormat = Format;

	ddrval = lpDD->CreateSurface(&vidmem, &DeviceSurface, NULL);

	if (ddrval == DD_OK)
	{
	//
	// Get the texture handle, if a valid video surface was assigned
	// (Otherwise this can be postponed until the texture is used)
	//
		IDirect3DTexture2 *Texture;
		DeviceSurface->QueryInterface(IID_IDirect3DTexture2, (void **) &Texture);
		Texture->GetHandle(lpD3DDevice, &Handle);
		Texture->Release();
	}
	else
	{
		DeviceSurface = NULL;
	}

//		Palette = NULL;

	return ddrval;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

bool TextureObject::allocate_memory_surface (void)
{
	HRESULT ddrval;
//
// Create system memory backing store (can't be lost) for texture
//
// We use this system memory surface as the source surface for ::Load
//
	DDSURFACEDESC sysmem;
	memset(&sysmem, 0, sizeof(DDSURFACEDESC));

	sysmem.dwSize          = sizeof(DDSURFACEDESC);
	sysmem.dwFlags         = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT; 
	sysmem.ddsCaps.dwCaps  = DDSCAPS_TEXTURE  | DDSCAPS_SYSTEMMEMORY;
	sysmem.dwWidth         = width;
	sysmem.dwHeight        = height;
	sysmem.ddpfPixelFormat = Format;

	ddrval = lpDD->CreateSurface(&sysmem, &MemorySurface, NULL);

	if (ddrval != DD_OK)
	{
		DebugPrint("%s\n",DD_message(ddrval));
		return false;
	}

	return (MemorySurface != 0);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

bool TextureObject::verify_video (void)
{
	if (DeviceSurface == NULL)
		return false;

	// Is VIDEO surface ready?

	if (DeviceSurface->IsLost() != DD_OK)
	{
	// Restore surface and setup reload

//		reload = true;

		HRESULT ddrval;
		if ((ddrval = DeviceSurface->Restore()) != DD_OK) 
		{
			DebugPrint("%s\n",DD_message(ddrval));
			RELEASE(DeviceSurface);
			return false;
		}
	}

	return true;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void TextureObject::set_palette (RGB *bitmap_palette)
{
// pci - madness?
#if 0
	S32     C0,C254,C255;
	S32     slot = texture_ID;
#endif

	//
	// Create a palette for the texture if supported
	//

	if (is_indexed())
	{
	//
	// Delete existing palette, if any
	//
		if (Palette != NULL)
		{
			Palette->Release();
		}

	//
	// Create a DirectDraw palette corresponding to the input VFX_RGB array
	//

		PALETTEENTRY entries[256];

		for (int i=0; i < 256; i++)
		{
			entries[i].peRed   = bitmap_palette[i].r;
			entries[i].peGreen = bitmap_palette[i].g;
			entries[i].peBlue  = bitmap_palette[i].b;
		}

		HRESULT ddrval;
		ddrval = lpDD->CreatePalette(DDPCAPS_8BIT, entries, &Palette, NULL);

		if (ddrval != DD_OK)
		{
			DebugPrint("%s\n",DD_message(ddrval));
//			return FALSE;
		}

	//
	// Attach palette to both the system and video memory surfaces
	// 
		MemorySurface->SetPalette(Palette);

		if (DeviceSurface != NULL)
		{
			DeviceSurface->SetPalette(Palette);
		}

	//
	// Find closest-possible < 254 remap value for colors 0, 254 and 255, 
	// to work around Hercules driver bug
	// 
//pci - this is madness?
#if 0
		S32 chroma = -1;

		if (Flags & TF_CHROMA)
		{
			chroma = Chroma;
		}

		S32 DD0   = LONG_MAX;
		S32 DD254 = LONG_MAX;
		S32 DD255 = LONG_MAX;

		for (i=1; i < 254; i++)
		{
			if (i == chroma)
			{
				continue;
			}

			S32 dr0 = bitmap_palette[0].r - bitmap_palette[i].r;
			S32 dg0 = bitmap_palette[0].g - bitmap_palette[i].g;
			S32 db0 = bitmap_palette[0].b - bitmap_palette[i].b;

			S32 D0 = (dr0*dr0) + (dg0*dg0) + (db0*db0);

			S32 dr1 = bitmap_palette[254].r - bitmap_palette[i].r;
			S32 dg1 = bitmap_palette[254].g - bitmap_palette[i].g;
			S32 db1 = bitmap_palette[254].b - bitmap_palette[i].b;

			S32 D254 = (dr1*dr1) + (dg1*dg1) + (db1*db1);

			S32 dr2 = bitmap_palette[255].r - bitmap_palette[i].r;
			S32 dg2 = bitmap_palette[255].g - bitmap_palette[i].g;
			S32 db2 = bitmap_palette[255].b - bitmap_palette[i].b;

			S32 D255 = (dr2*dr2) + (dg2*dg2) + (db2*db2);

			if (D0 < DD0)
			{
				C0 = i;
				DD0 = D0;
			}

			if (D254 < DD254)
			{
				C254 = i;
				DD254 = D254;
			}

			if (D255 < DD255)
			{
				C255 = i;
				DD255 = D255;
			}
		}
#endif
	}

}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

bool TextureObject::upload_texture (void)
// refresh contents of video-memory texture surface from system-memory
{
	if (DeviceSurface == NULL || MemorySurface == NULL)
	{
		return false; // Failed: texture missing device or memory surface
	}

// COPY TEXTURE FROM SYSTEM TO VIDEO MEMORY

	IDirect3DTexture2 *Memory;
	IDirect3DTexture2 *Device;

// pci - Why aren't these always around?

	DeviceSurface->QueryInterface(IID_IDirect3DTexture2, (void**) &Device);
	MemorySurface->QueryInterface(IID_IDirect3DTexture2, (void**) &Memory);

	HRESULT err = Device->Load(Memory);

	Device->Release();
	Memory->Release();

	if (err != DD_OK)
	{
		DebugPrint("Texture load failed: %s\n",DD_message(err));
		RELEASE(DeviceSurface);
	}

	return err == DD_OK;
}

//---------------------------------------------------------------------------
// TextureMgr
//---------------------------------------------------------------------------

bool TextureMgr::flush_textures (int priority, int size)
{
	TextureObject *obj;
	for (obj=head; obj; obj=obj->next)
	{
		if (obj->priority < priority)
		{
			// FUTURE: should size matter?
			RELEASE(obj->DeviceSurface);
			// FUTURE: flush one and try again?
		}
	}
	return false;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

bool TextureMgr::verify_texture (TextureObject *txm)
{
	bool reload = false;

// Is VIDEO surface allocated?

	if (txm->DeviceSurface != NULL)
	{
		if (!txm->verify_video())
			return false;
	}
	else // need to allocate VIDEO surface
	{
		reload = true;

		HRESULT ok = txm->allocate_video_surface();

		while (ok != DD_OK)
		{
			if ((ok == DDERR_OUTOFVIDEOMEMORY) || (ok == DDERR_OUTOFMEMORY))
			{
			// Insufficient video memory for texture... discard LOWER priority textures

				if (!flush_textures(txm->priority))
				{
					return false; // nothing left to flush, and still unable to load texture?
				}

				ok = txm->allocate_video_surface();
			}
			else
			{
				DebugPrint("%s\n",DD_message(ok));
				txm->DeviceSurface = NULL;
				return false;
			}
		}

	// Texture video surface was created

		if (txm->is_indexed())
		{
			txm->DeviceSurface->SetPalette(txm->Palette);
		}
	}

// If texture was Restored or Created, then LOAD it into video memory

	if (reload)
	{
		if (!txm->upload_texture())
		{
			return false;
		}

		lpD3DDevice->SetRenderState(D3DRS_TEXTUREHANDLE, txm->Handle);

		if (txm->Flags & TF_CHROMA)
		{
			lpD3DDevice->SetRenderState(D3DRS_COLORKEYENABLE, 1);
		}
		else
		{
			lpD3DDevice->SetRenderState(D3DRS_COLORKEYENABLE, 0);
		}
	}

	return true;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void TextureMgr::bind_texture (int id)
{
	if (active && active->id == id)
		return;	// already selected

	TextureObject *txm = find_texture(id);

// IF IT DOESN'T EXIST, CREATE A NEW TEXTURE

	if (txm == 0)
	{
		txm = new_texture(id);
		if (txm == 0)
		{
			return;	// Warning: failed to create?
		}
	}

// ACTIVATE NEW CURRENT TEXTURE

	if (txm)
	{
		active = txm;

		if (verify_texture(txm))
			active = txm;
	}
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

#define PIXEL_VALUE(x) (((((U32) (x)->r) >> (red_right)) << red_left) | \
                        ((((U32) (x)->g) >> (grn_right)) << grn_left) | \
                        ((((U32) (x)->b) >> (blu_right)) << blu_left))

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

bool TextureMgr::load_texture (TextureObject *txm, void *_bitmap, int bitmap_width, int bitmap_height, RGB *bitmap_palette)
{
	HRESULT ddrval;

	const void *bitmap = _bitmap;

	//
	// Set flags
	//
/*
	if (ENG->get_state(LOADSTATE_ENABLE_TEXTURE_CHROMA_KEY).longVal)
	{
	txm->Flags |= TF_CHROMA;
	txm->Chroma = ENG->get_state(LOADSTATE_TEXTURE_CHROMA_KEY).longVal;
	}
*/
	//
	// Derive RGB shift, width values from masks
	//

	S32 red_left;
	S32 red_right;
	S32 red_width;
	S32 grn_left;
	S32 grn_right;
	S32 grn_width;
	S32 blu_left;
	S32 blu_right;
	S32 blu_width;

// pci - Why isn't this done ONCE?

	if (txm->is_rgb())
	{
		U32 red_mask = txm->Format.dwRBitMask;
		U32 grn_mask = txm->Format.dwGBitMask;
		U32 blu_mask = txm->Format.dwBBitMask;

		for (S32 i=31; i >= 0; i--)
		{
			if (red_mask & (1 << i))
			{
				red_left = i;
			}
			if (grn_mask & (1 << i))
			{
				grn_left = i;
			}
			if (blu_mask & (1 << i))
			{
				blu_left = i;
			}
		}

		for (i=0; i <= 31; i++)
		{
			if (red_mask & (1 << i))
			{
				red_width = i - red_left + 1;
			}

			if (grn_mask & (1 << i))
			{
				grn_width = i - grn_left + 1;
			}

			if (blu_mask & (1 << i))
			{
				blu_width = i - blu_left + 1;
			}
		}

		red_right = 8 - red_width;
		grn_right = 8 - grn_width;
		blu_right = 8 - blu_width;
	}

//
// Copy texture bitmap to system memory surface
//

	if (bitmap != NULL)
	{
	//
	// Get scaling constants for copy to system memory surface
	//
		SINGLE du = (SINGLE) bitmap_width  / (SINGLE) txm->width;
		SINGLE dv = (SINGLE) bitmap_height / (SINGLE) txm->height;

	//
	// Copy texture bitmap to system memory surface
	//
		void *screen;
		S32   stride;

		DDSURFACEDESC ddsd;

		memset(&ddsd, 0, sizeof(ddsd));
		ddsd.dwSize = sizeof(ddsd);

		do
		{
			ddrval = txm->MemorySurface->Lock(NULL, &ddsd, 0, NULL);
		} 
		while (ddrval != DD_OK);

		screen = (void *) ddsd.lpSurface;
		stride = ddsd.lPitch;

		SINGLE v = 0.0F;

	//
	// Case 1: 8-bpp source texture with palette being copied to RGB surface
	//
		if (txm->is_rgb())
		{
			for (S32 y=0; y < txm->height; y++,v += dv)
			{
				U8 *src_map = &((U8 *) bitmap)[bitmap_width * (S32) v];

				SINGLE u = 0.0F;

				for (S32 x=0; x < txm->width; x++,u += du)
				{
					((U16 *) screen)[x] = (U16) PIXEL_VALUE(&bitmap_palette[src_map[(S32) u]]);
				}

				screen = ((U8 *) screen) + stride;
			}
		}

	//
	// Case 2: 8-bpp source texture with palette being copied to 8-bpp palettized surface
	//
	// Replace colors 254 and 255 to avoid weird Hercules driver bug
	// (palette entries 254 and 255 show up as white)
	//

		if (txm->is_indexed())
		{
			for (S32 y=0; y < txm->height; y++,v += dv)
			{
				U8 *src_map = &((U8 *) bitmap)[bitmap_width * (S32) v];

				SINGLE u = 0.0F;

				for (S32 x=0; x < txm->width; x++,u += du)
				{
					U8 color = src_map[(S32) u];
#if 0
					if (color == 254) color = C254;
					else if (color == 255) color = C255;
					else if (color == 0)   color = C0;
#endif
					((U8 *) screen)[x] = color;
				}

				screen = ((U8 *) screen) + stride;
			}
		}

		do
		{
			ddrval = txm->MemorySurface->Unlock(NULL);
		} 
		while (ddrval != DD_OK);
	}

//
// Set chroma-key for texture surface, if enabled
//

	if (txm->Flags & TF_CHROMA)
	{
		DDCOLORKEY ddck;

		if (txm->is_indexed())
		{
			ddck.dwColorSpaceLowValue = ddck.dwColorSpaceHighValue = txm->Chroma;
		}
		else
		{
			RGB *key = bitmap_palette + txm->Chroma;

// pci - Why isn't this a palette index?

			ddck.dwColorSpaceLowValue =
			ddck.dwColorSpaceHighValue = 
				native_color_value(txm->MemorySurface, key->r,key->g,key->b);
		}

		txm->MemorySurface->SetColorKey(DDCKEY_SRCBLT, &ddck);

		if (txm->DeviceSurface != NULL)
		{
			txm->DeviceSurface->SetColorKey(DDCKEY_SRCBLT, &ddck);
		}
	}

//
// If a valid device surface exists, update its copy of the bitmap now
//
	if (txm->DeviceSurface != NULL)
	{
		txm->upload_texture();
	}

	return TRUE;	// Success
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

int TextureMgr::register_texture (const U8 *bitmap, int bitmap_width, int bitmap_height, const RGB *bitmap_palette)
{
	TextureObject *txm;

	if (active)
		txm = active;
	else
		txm = &txm_temp;

//
// Palette is mandatory
//
	if (bitmap_palette == NULL)
	{
		DebugPrint("D3DPOLY: Missing texture palette\n");
		return -1;
	}

//
// Set flags
//
	txm->Flags = 0;

//
// Get storage size for texture
//
	txm->width = bitmap_width;
	txm->height = bitmap_height;

	// FUTURE: fix 2^N size


// pci?
// mode_set_handler(this);

// pci - why can't this be done once?

//
// Select storage format for texture
// 
// First look for an 8-bit palettized format supported by hardware -- 
// otherwise, fall through to attempt to find a 16-bpp format that 
// matches the native video output mode
//
	S32 format_index = -1;

	for (int i=0; i<texture_format_cnt; i++)
	{
		DDPIXELFORMAT ddpf = texture_formats[i];

		if ((ddpf.dwRGBBitCount == 8) && (ddpf.dwFlags & DDPF_PALETTEINDEXED8))
		{
			format_index = i;
			break;
		}
	}

//
// If palettized textures are not supported by hardware, search for a 
// high-color texture format that matches the current video mode
//
	if (format_index == -1)
	{
		for (int i=0; i < texture_format_cnt; i++)
		{
			DDPIXELFORMAT ddpf = texture_formats[i];

			if ((ddpf.dwRGBBitCount > 8) && (ddpf.dwFlags & DDPF_RGB))
			{
				if ((ddpf.dwRBitMask == R_mask) && (ddpf.dwGBitMask == G_mask) && (ddpf.dwBBitMask == B_mask))
				{
					format_index = i;
					break;
				}
			}
		}
	}

//
// If no high-color format matching the current video mode is available,
// settle for whatever 15- or 16-bit format the card supports...
//

	if (format_index == -1)
	{
		for (int i=0; i < texture_format_cnt; i++)
		{
			DDPIXELFORMAT ddpf = texture_formats[i];

			if (((ddpf.dwRGBBitCount == 15) || (ddpf.dwRGBBitCount == 16)) && (ddpf.dwFlags & DDPF_RGB))
			{
//					txm->Type = TT_RGB;
				format_index = i;
				break;
			}
		}
	}

//
// Fail this call if no compatible texture format support available
//

	if (format_index == -1)
	{
		DebugPrint("D3DPOLY::register_texture(): No compatible format available\n");
		return -1;
	}

	txm->Format = texture_formats[format_index];

	txm->allocate_video_surface();
	txm->allocate_memory_surface();

//
// Set texture map
//
	load_texture(txm, bitmap, bitmap_width, bitmap_height, bitmap_palette);

	return (int) txm;
}

//---------------------------------------------------------------------------
// DISPLAY methods
//---------------------------------------------------------------------------

/*
	VMETHOD(void) BindTexture (GLenum target, GLuint texture)
	{
		if (INSIDE_BEGIN_END)
		{ gl_error(GL_INVALID_OPERATION,"BindTexture"); return; }	// ERROR!

		switch (target)
		{
		case GL_TEXTURE_1D:
		default:
			// Nothing else supported yet!
			if (target != GL_TEXTURE_2D)
			{ gl_error(GL_INVALID_OPERATION,"BindTexture"); return; }	// ERROR!

		case GL_TEXTURE_2D:
			TextureMgr::bind_texture(texture);
			break;
		}
	}

	VMETHOD(void) DISPLAY::TexParameter (GLenum txm, GLenum param, GLenum value)
	{
		if (INSIDE_BEGIN_END)
		{ gl_error(GL_INVALID_OPERATION,"TexParameter"); return; }

		switch (txm)
		{
		case GL_TEXTURE_2D:
		{
			_D3DRENDERSTATETYPE wrap,addr;

			switch (param)
			{
			case GL_TEXTURE_WRAP_S:
				wrap = D3DRS_WRAPU;
				addr = D3DRS_TEXTUREADDRESSU;
				break;

			case GL_TEXTURE_WRAP_T:
				wrap = D3DRS_WRAPV;
				addr = D3DRS_TEXTUREADDRESSV;
				break;

			default:
				gl_error(GL_INVALID_ENUM,"TexParameter");
				return;
			} // param

			switch (value)
			{
			case GL_CLAMP:
				d3dDevice->SetRenderState(wrap, FALSE);
				d3dDevice->SetRenderState(addr, D3DTADDRESS_CLAMP);
				break;

			case GL_REPEAT:
				d3dDevice->SetRenderState(wrap, TRUE);
				d3dDevice->SetRenderState(addr, D3DTADDRESS_WRAP);
				break;

			default:
				gl_error(GL_INVALID_ENUM,"TexParameter");
				return;
			} // value

			break;
		}

		case GL_TEXTURE_1D:
			
			// Minimal Support (tm)

		default:
			gl_error(GL_INVALID_ENUM,"TexParameter");
			return;

		} // switch
	}

	VMETHOD(void) DISPLAY::ShadeModel (GLenum m)
	{
		if (INSIDE_BEGIN_END)
		{ gl_error(GL_INVALID_OPERATION,"TexParameter"); return; }

		switch (m)
		{
			case GL_FLAT:
				d3dDevice->SetRenderState(D3DRS_SHADEMODE,D3DSHADE_FLAT);
				break;

			case GL_SMOOTH:
				d3dDevice->SetRenderState(D3DRS_SHADEMODE,D3DSHADE_GOURAUD);
				break;

//			case GL_PHONG:
//				d3dDevice->SetRenderState(D3DRS_SHADEMODE,D3DSHADE_PHONG);
//				break;
		}
	}

//---------------------------------------------------------------------------

	VMETHOD(void) DISPLAY::BlendFunc (GLenum src, GLenum dst)
	{
		if (INSIDE_BEGIN_END)
		{ gl_error(GL_INVALID_OPERATION,"BlendFunc"); return; }

		int blend_src;
		int blend_dst;

		switch (src)
		{
			case GL_ZERO:
				blend_src = D3DBLEND_ZERO; break;
			case GL_ONE:
				blend_src = D3DBLEND_ONE; break;
			case GL_DST_COLOR:
				blend_src = D3DBLEND_DESTCOLOR; break;
			case GL_ONE_MINUS_DST_COLOR:
				blend_src = D3DBLEND_INVDESTCOLOR; break;
			case GL_SRC_ALPHA:
				blend_src = D3DBLEND_SRCALPHA; break;
			case GL_ONE_MINUS_SRC_ALPHA:
				blend_src = D3DBLEND_INVSRCALPHA; break;
			case GL_DST_ALPHA:
				blend_src = D3DBLEND_DESTALPHA; break;
			case GL_ONE_MINUS_DST_ALPHA:
				blend_src = D3DBLEND_INVDESTALPHA; break;
			case GL_SRC_ALPHA_SATURATE:
				blend_src = D3DBLEND_SRCALPHASAT; break;
				break;

			default:
				gl_error(GL_INVALID_ENUM,"BlendFunc");
				return;
		}

		switch (dst)
		{
			case GL_ZERO:
				blend_dst = D3DBLEND_ZERO; break;
			case GL_ONE:
				blend_dst = D3DBLEND_ONE; break;
			case GL_SRC_COLOR:
				blend_dst = D3DBLEND_SRCCOLOR; break;
			case GL_ONE_MINUS_SRC_COLOR:
				blend_dst = D3DBLEND_INVSRCCOLOR; break;
			case GL_SRC_ALPHA:
				blend_dst = D3DBLEND_SRCALPHA; break;
			case GL_ONE_MINUS_SRC_ALPHA:
				blend_dst = D3DBLEND_INVSRCALPHA; break;
			case GL_DST_ALPHA:
				blend_dst = D3DBLEND_DESTALPHA; break;
			case GL_ONE_MINUS_DST_ALPHA:
				blend_dst = D3DBLEND_INVDESTALPHA; break;

			default:
				gl_error(GL_INVALID_ENUM,"BlendFunc");
				return;
		}

		d3dDevice->SetRenderState(D3DRS_SRCBLEND, blend_src);
		d3dDevice->SetRenderState(D3DRS_DESTBLEND, blend_dst);
	}

*/

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

