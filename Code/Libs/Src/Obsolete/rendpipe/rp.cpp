//
// new render pipeline.
//

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>

#define INITGUID
#include <ddraw.h>

#include "stddat.h"
#include "display.h"
#include "rp.h"
#include "pixel.h"

//

C8 InterfaceName[] = "IRenderPipeline";	// Interface name used for registration     
ICOManager *DACOM = NULL;				// Handle to component manager

//

#define CLIP_RIGHT	0x01
#define CLIP_LEFT	0x02
#define CLIP_TOP	0x04
#define CLIP_BOTTOM	0x08
#define CLIP_NEAR	0x10
#define CLIP_FAR	0x20

//

void SortedList::insert(const RPList * list)
{
	ListNode * node = get_free_entry();
	assert(node);

// Assume node->prev == node->next == NULL.
	node->list = list;

	if (used_entries)
	{
		ListNode * prev = NULL;
		ListNode * curr = used_entries;
		while (curr && (curr->list->texture < list->texture))
		{
			prev = curr;
			curr = curr->next;
		}

	// Sort by type within texture.
		while (curr && (curr->list->texture == list->texture) && (curr->list->type < list->type))
		{
			prev = curr;
			curr = curr->next;
		}

	// Link it in.
		if (prev)
		{
			prev->next = node;
		}
		else
		{
			used_entries = node;
		}
		node->prev = prev;
		node->next = curr;
		if (curr)
		{
			curr->prev = node;
		}
	}
	else
	{
	// List is empty, place at head.
		used_entries = node;
	}
}

//

char *DD_message (HRESULT error);
void DebugPrint (char *fmt, ...);
void DebugAlert (char *title, char *fmt, ...);

//

BOOL CALLBACK DD_enumerate(GUID *lpGUID, LPSTR szName, LPSTR szDevice, LPVOID lpContext)
{
	BOOL result;
	LPDIRECTDRAW DD;

	RenderPipeline * rp = (RenderPipeline *) lpContext;

	HRESULT ddrval = DirectDrawCreate(lpGUID, &DD, NULL);
	if (ddrval == DD_OK)
	{
		if (DD->QueryInterface(IID_IDirectDraw2, (void **) &rp->lpDD) != DD_OK)
		{
			DD->Release();
			rp->lpDD = NULL;
			result = DDENUMRET_OK;
		}
		else
		{
		//
		// See if it supports hardware rendering
		//
			memset(&rp->hw_caps, 0, sizeof(DDCAPS));
			rp->hw_caps.dwSize = sizeof(DDCAPS);

			memset(&rp->hel_caps, 0, sizeof(DDCAPS));
			rp->hel_caps.dwSize = sizeof(DDCAPS);

			if (rp->lpDD->GetCaps(&rp->hw_caps, &rp->hel_caps) != DD_OK)
			{
				rp->lpDD->Release();
				rp->lpDD = NULL;
				result = DDENUMRET_OK;
			}
			else if (rp->hw_caps.dwCaps & DDCAPS_3D)
			{
				DebugPrint("DD <= %X '%s' '%s'\n", lpGUID, szName, szDevice);
				result = DDENUMRET_CANCEL;
			}
			else
			{
			//
			// Driver does not support 3D rendering -- release it and continue
			// with enumeration process
			//
				rp->lpDD->Release();
				rp->lpDD = NULL;
				result = DDENUMRET_OK;
			}
		}
	}
	else
	{
		result = DDENUMRET_OK;
	}

	return result;
}

//

bool RenderPipeline::setupD3D(void)
{
	bool result;

	DirectDrawEnumerate(DD_enumerate, this);
	if (lpDD)
	{
		if (lpDD->SetCooperativeLevel(NULL, DDSCL_NORMAL) != DD_OK)
		{
			lpDD->Release();
			lpDD = NULL;
		}
		else
		{
			result = true;
		}
	}
	else
	{
		result = false;
	}

	return result;
}

//

void RenderPipeline::shutdownD3D(void)
{
	if (lpDD)
	{
		lpDD->Release();
		lpDD = NULL;
	}
}

//

//HRESULT CALLBACK TF_enumerate(DDPIXELFORMAT * ddpf, LPVOID lpContext)
HRESULT CALLBACK TF_enumerate(DDSURFACEDESC * desc, LPVOID lpContext)
{
	DDPIXELFORMAT * ddpf = &desc->ddpfPixelFormat;
	DebugPrint("   %2dbpp %s%s%s %08X %08X %08X %08X\n",
		ddpf->dwRGBBitCount,
		(ddpf->dwFlags & (DDPF_RGB)) ? "RGB" : "   ",
		(ddpf->dwFlags & (DDPF_ALPHAPIXELS)) ? "A " : "  ",
		(ddpf->dwFlags & (DDPF_PALETTEINDEXED8 | DDPF_PALETTEINDEXED4)) ? "PAL " : "    ",
		ddpf->dwRBitMask,
		ddpf->dwGBitMask,
		ddpf->dwBBitMask,
		ddpf->dwRGBAlphaBitMask);

//
// Add this format's pixel descriptor to the list
// 
	RenderPipeline * rp = (RenderPipeline *) lpContext;
	rp->texture_formats[rp->num_texture_formats++].init(*ddpf);

//
// Signal request for next format
//
	return DDENUMRET_OK;
}

//

HRESULT CALLBACK ZB_enumerate(DDPIXELFORMAT * ddpf, LPVOID lpContext)
{
	DebugPrint("   %2dbpp %08X\n", ddpf->dwZBufferBitDepth, ddpf->dwZBitMask);
//
// Add this format's pixel descriptor to the list
// 
	RenderPipeline * rp = (RenderPipeline *) lpContext;
	rp->zbuffer_formats[rp->num_zbuffer_formats++]= *ddpf;
//
// Signal request for next format
//
	return DDENUMRET_OK;
}

//

bool RenderPipeline::create_surfaces(void)
{
	HRESULT result;

// SETUP CONTEXT SURFACE INFO

	int width = 640;
	int height = 480;
	int bpp = 16;

// Set up requested fullscreen display mode

	bool flipping = false;
	bool depth = true;

// Get Direct3D interface.
	result = lpDD->QueryInterface(IID_IDirect3D2, (void **) &lpD3D);
	if (result != DD_OK)
	{
		DebugAlert(NULL, "RP: This application requires DirectX 5 or greater\n");
		return false;
	}

// OPTIONAL - CREATE Z-BUFFER (16 BIT)

	if (depth)
	{
	/*
		num_zbuffer_formats = 0;
		DebugPrint("ZBuffer formats:\n");
		result = lpD3D->EnumZBufferFormats(IID_IDirect3DHALDevice, ZB_enumerate, this);
		if (result != DD_OK)
		{
			DebugAlert(NULL, "RP: EnumTextureFormats() failed: %s",DD_message(result));
			return false;
		}

		DWORD zdepth;
		DWORD zmask;
		if (num_zbuffer_formats == 0)
		{
			DebugAlert(NULL, "RP: No Z-buffer formats supported.\n");
			return false;
		}
		else
		{
			for (int i = 0; i < num_zbuffer_formats; i++)
			{
				if (zbuffer_formats[i].dwZBufferBitDepth == 16)
				{
					zdepth = 16;
					zmask = zbuffer_formats[i].dwZBitMask;
					break;
				}
			}
		}
	*/
		DDSURFACEDESC/*2*/ ddsd;
		memset(&ddsd, 0, sizeof(ddsd));
		ddsd.dwSize = sizeof(ddsd);

		ddsd.dwFlags =	DDSD_WIDTH  | 
						DDSD_HEIGHT | 
						DDSD_CAPS   | 
						DDSD_ZBUFFERBITDEPTH;
						//DDSD_PIXELFORMAT;

		ddsd.ddsCaps.dwCaps = DDSCAPS_ZBUFFER | DDSCAPS_VIDEOMEMORY;

		ddsd.dwWidth           = width;
		ddsd.dwHeight          = height;
		ddsd.dwZBufferBitDepth = 16;
		//ddsd.ddpfPixelFormat.dwSize = sizeof(ddsd.ddpfPixelFormat);
		//ddsd.ddpfPixelFormat.dwFlags = DDPF_ZBUFFER;
		//ddsd.ddpfPixelFormat.dwZBufferBitDepth = zdepth;
		//ddsd.ddpfPixelFormat.dwZBitMask = zmask;

		LPDIRECTDRAWSURFACE Z;
		result = lpDD->CreateSurface(&ddsd, &Z, NULL);
		if (result == DD_OK)
		{
			DebugPrint("RP: Z-buffer created.\n");

			Z->QueryInterface(IID_IDirectDrawSurface3, (void **) &lpZBuffer);
			Z->Release();
		}
		else
		{
			DebugAlert(NULL, "RP: Z-buffer creation failed: %s (%d x %d x %d)",DD_message(result), width, height, bpp);
			return false;
		}
	}

// CREATE PRIMARY SURFACE (POSSIBLY WITH ATTACHED BACK BUFFERS)

	DDSURFACEDESC/*2*/ ddsd;
	memset(&ddsd, 0, sizeof(ddsd));

	ddsd.dwSize			= sizeof(ddsd);
	ddsd.ddsCaps.dwCaps	= DDSCAPS_PRIMARYSURFACE | DDSCAPS_VIDEOMEMORY;
	ddsd.dwFlags		= DDSD_CAPS;

	if (flipping)
	{
		result = lpDD->SetCooperativeLevel(hWnd, DDSCL_EXCLUSIVE | DDSCL_ALLOWREBOOT | DDSCL_FULLSCREEN);

		ddsd.dwFlags |= DDSD_BACKBUFFERCOUNT;
		ddsd.ddsCaps.dwCaps |= DDSCAPS_COMPLEX | DDSCAPS_FLIP | DDSCAPS_3DDEVICE;
		ddsd.dwBackBufferCount = 1;
	}

	bool complex = false;

	LPDIRECTDRAWSURFACE primary;
	DebugPrint("RP: Creating primary surface:\n");
	result = lpDD->CreateSurface(&ddsd, &primary, NULL);
	if (result == DD_OK)
	{
		primary->QueryInterface(IID_IDirectDrawSurface3, (void **) &lpDDSPrimary);
		primary->Release();
		DebugPrint("RP: Created primary surface%s\n", (flipping) ? " with 1 back buffer." : ".");
		if (flipping)
		{
			complex = true;
		}
	}
	else // CREATE NON-FLIP SURFACE
	{
		DebugPrint("RP: Complex surface creation failed: %s\n", DD_message(result));
		DebugPrint("RP: Unable to create primary surface%s\n", (flipping) ? ": trying non-flipping surface." : ". Aborting.");

		if (flipping)
		{
			flipping = false;

		// Couldn't create complex (flipping) surface, try simple surface with separate back buffer.
			memset(&ddsd, 0, sizeof(ddsd));
			ddsd.dwSize			= sizeof(ddsd);
			ddsd.ddsCaps.dwCaps	= DDSCAPS_PRIMARYSURFACE | DDSCAPS_VIDEOMEMORY;// | DDSCAPS_3DDEVICE;
			ddsd.dwFlags		= DDSD_CAPS;

			result = lpDD->CreateSurface(&ddsd, &primary, NULL);
			if (result == DD_OK)
			{
				primary->QueryInterface(IID_IDirectDrawSurface3, (void **) &lpDDSPrimary);
				primary->Release();

				DebugPrint("RP: Created simple (non-flipping) primary surface.\n");
				complex = false;
			}
			else
			{
				DebugAlert("RP: Unable to create simple (non-flipping) surface: %s.\n", DD_message(result));
				return false;
			}
		}
		else
		{
			return false;
		}
	}

// CREATE BACK BUFFER (OR GET ATTACHED BACK BUFFER)
	LPDIRECTDRAWSURFACE back;

	if (flipping && complex)
	{
		DDSCAPS/*2*/ ddscaps;
		ddscaps.dwCaps = DDSCAPS_BACKBUFFER;

		result = lpDDSPrimary->GetAttachedSurface(&ddscaps, &lpDDSBack);
		if (result == DD_OK)
		{
			DebugPrint("RP: Got pointer to attached back buffer.\n");
		}
		else
		{
			DebugAlert(NULL, "RP: DD->GetAttachedSurface() failed: %s.\n", DD_message(result));
			return false;
		}
	}
	else
	{
	//
	// Allocate single back buffer surface from video memory.
	//
		DDSURFACEDESC/*2*/ ddsd;
		memset(&ddsd, 0, sizeof(ddsd));

		ddsd.dwSize = sizeof(ddsd);
		ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
		ddsd.dwWidth  = width;
		ddsd.dwHeight = height;
		ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_VIDEOMEMORY | DDSCAPS_3DDEVICE;

		DebugPrint("RP: Creating back buffer: %d, %d, %d\n", width, height, bpp);

		result = lpDD->CreateSurface(&ddsd, &back, NULL);
		if (result == DD_OK)
		{
			back->QueryInterface(IID_IDirectDrawSurface3, (void **) &lpDDSBack);
			back->Release();
			DebugPrint("RP: Created back buffer.\n");
		}
		else
		{
			DebugAlert("Error", "RP: Unable to create back buffer (%d x %d x %d): %s.\n", width, height, bpp, DD_message(result));
			return false;
		}
	}

//
// Set up pixel description.
//
	ddsd.dwSize = sizeof(ddsd);
	lpDDSBack->GetSurfaceDesc(&ddsd);
	if (ddsd.dwFlags & DDSD_PIXELFORMAT)
	{
		screen_pixel_format.init(ddsd.ddpfPixelFormat);

		DebugPrint("RP: screen pixel format = RGBA(%d%d%d%d)\n", screen_pixel_format.rwidth,
			screen_pixel_format.gwidth, screen_pixel_format.bwidth, screen_pixel_format.awidth);
	}
	else
	{
		DebugAlert(NULL, "RP: Unable to get pixel format for back buffer.\n");
	}

// Attach Z-buffer to back buffer

	if (lpZBuffer)
	{
		if (lpZBuffer->IsLost())
		{
			lpZBuffer->Restore();
		}
		if (lpDDSBack->IsLost())
		{
			lpDDSBack->Restore();
		}

		result = lpDDSBack->AddAttachedSurface(lpZBuffer);
		if (result != DD_OK)
		{
			DebugAlert(NULL, "RP: Z-buffer attach failed: %s", DD_message(result));
			return false;
		}
	}

//
// If blitting as opposed to flipping, create a clipper.
//
	if (!complex)
	{
		IDirectDrawClipper  * lpDDClipper;
		result = lpDD->CreateClipper(0, &lpDDClipper, NULL);
		if (result != DD_OK)
		{
			DebugAlert(NULL,"RP: DD->CreateClipper() failed, code %X\n",result);
			return false;
		}

		result = lpDDClipper->SetHWnd(0, hWnd);
		if (result != DD_OK)
		{
			DebugAlert(NULL,"RP: Clipper->SetHWnd() failed, code %X\n",result);
			return false;
		}

		result = lpDDSPrimary->SetClipper(lpDDClipper);
		if (result != DD_OK)
		{
			DebugAlert(NULL,"RP: Surface->SetClipper failed, code %X\n",result);
			return false;
		}
		lpDDClipper->Release();
	}

//
// Create Direct3D HAL device
//
// If native hardware support not available from this driver, abort
//
	result = lpD3D->CreateDevice(IID_IDirect3DHALDevice, (LPDIRECTDRAWSURFACE) lpDDSBack, &lpD3DDevice);
	if (result != DD_OK)
	{
		DebugAlert(NULL, "RP: D3D device creation failed: %s",DD_message(result));
		return false;
	}
//
// Create Direct3D viewport object and associate it with the device 
// just created
//
	result = lpD3D->CreateViewport(&lpD3DViewport, NULL);
	if (result != DD_OK)
	{
		DebugAlert(NULL, "RP: CreateViewport() failed: %s",DD_message(result));
		return false;
	}

	result = lpD3DDevice->AddViewport(lpD3DViewport);
	if (result != DD_OK)
	{
		DebugAlert(NULL, "RP: AddViewport() failed: %s",DD_message(result));
		return false;
	}

	D3DVIEWPORT2 viewData;

	memset(&viewData, 0, sizeof(D3DVIEWPORT2));
	viewData.dwSize         =   sizeof(D3DVIEWPORT2);
	viewData.dwX            =   0;
	viewData.dwY            =   0;
	viewData.dwWidth        =   width;
	viewData.dwHeight       =   height;
	viewData.dvClipX        =   -1;
	viewData.dvClipY        =   1;
	viewData.dvClipWidth    =   2;
	viewData.dvClipHeight   =   2;
	viewData.dvMinZ         =   0.0f;
	viewData.dvMaxZ         =   1.0f;

	result = lpD3DViewport->SetViewport2(&viewData);
	if (result != DD_OK)
	{
		DebugAlert(NULL, "RP: SetViewport2() failed: %s",DD_message(result));
		return false;
	}

	last_x = 0;
	last_y = 0;
	last_w = width;
	last_h = height;

	result = lpD3DDevice->SetCurrentViewport(lpD3DViewport);
	if (result != DD_OK)
	{
		DebugAlert(NULL, "RP: SetCurrentViewport() failed: %s",DD_message(result));
		return false;
	}

	num_texture_formats = 0;

	DebugPrint("Texture Formats:\n");
	result = lpD3DDevice->EnumTextureFormats(TF_enumerate, this);
	if (result != DD_OK)
	{
		DebugAlert(NULL, "RP: EnumTextureFormats() failed: %s",DD_message(result));
		return false;
	}

	if (num_texture_formats == 0)
	{
		DebugAlert(NULL, "RP: No texture formats supported\n");
		return false;
	}

// EXTRACT TEXTURE SUPPORT INFO
	{
		D3DDEVICEDESC d3dhwcaps, d3dhelcaps;
		d3dhwcaps.dwSize = d3dhelcaps.dwSize = sizeof(D3DDEVICEDESC);
		if (lpD3DDevice->GetCaps(&d3dhwcaps, &d3dhelcaps) == DD_OK)
		{
			min_txm_width = d3dhwcaps.dwMinTextureWidth;
			min_txm_height = d3dhwcaps.dwMinTextureHeight;

			max_txm_width = d3dhwcaps.dwMaxTextureWidth;
			max_txm_height = d3dhwcaps.dwMaxTextureHeight;
		}
	}

	if (use_mipmaps)
	{
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
				DebugPrint("RP: Hardware doesn't support mipmapping. Disabling.\n");
				use_mipmaps = false;
			}
		}
		else
		{
			DebugPrint("RP: Hardware supports mipmapping.\n");
			use_mipmaps = true;
			surf->Release();
		}
	}

// SET DEFAULT RENDER STATES
/*	
	lpD3DDevice->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);

	lpD3DDevice->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
	lpD3DDevice->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);

	lpD3DDevice->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
	lpD3DDevice->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
	lpD3DDevice->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_CURRENT);

	lpD3DDevice->SetTextureStageState(0, D3DTSS_ADDRESS, D3DTADDRESS_WRAP);
*/
	lpD3DDevice->SetRenderState(D3DRS_SPECULARENABLE, FALSE);

	lpD3DDevice->SetRenderState(D3DRS_TEXTUREMAPBLEND, D3DTBLEND_MODULATE);
	lpD3DDevice->SetRenderState(D3DRS_TEXTUREPERSPECTIVE,TRUE);
/*
	lpD3DDevice->SetTextureStageState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
	lpD3DDevice->SetTextureStageState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);


	for (int i = 0; i < 8; i++)
	{
		LPDIRECT3DTEXTURE2 txm;
		lpD3DDevice->GetTexture(i, &txm);
		lpD3DDevice->SetTexture(i, NULL);
	}
*/

	lpD3DDevice->SetRenderState(D3DRS_TEXTUREMIN, D3DFILTER_LINEAR);
	lpD3DDevice->SetRenderState(D3DRS_TEXTUREMAG, D3DFILTER_LINEAR);

	lpD3DDevice->SetRenderState(D3DRS_WRAP0, 0);

	lpD3DDevice->SetRenderState(D3DRS_TEXTUREADDRESSU, D3DTADDRESS_WRAP);
	lpD3DDevice->SetRenderState(D3DRS_TEXTUREADDRESSV, D3DTADDRESS_WRAP);

	lpD3DDevice->SetRenderState(D3DRS_WRAPU, 0);
	lpD3DDevice->SetRenderState(D3DRS_WRAPV, 0);

	lpD3DDevice->SetRenderState(D3DRS_SHADEMODE, D3DSHADE_GOURAUD);
	lpD3DDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);

	lpD3DDevice->SetRenderState(D3DRS_STIPPLEENABLE, FALSE);
	lpD3DDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
	lpD3DDevice->SetRenderState(D3DRS_DITHERENABLE, TRUE);

	lpD3DDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
	lpD3DDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);

	lpD3DDevice->SetRenderState(D3DRS_COLORKEYENABLE, FALSE);

	lpD3DDevice->SetRenderState(D3DRS_ANTIALIAS, D3DANTIALIAS_NONE);
	lpD3DDevice->SetRenderState(D3DRS_FOGENABLE, FALSE);

	lpD3DDevice->SetRenderState(D3DRS_ZENABLE, 1);
	lpD3DDevice->SetRenderState(D3DRS_ZFUNC, D3DCMP_LESS);
	lpD3DDevice->SetRenderState(D3DRS_ZWRITEENABLE, 1);

	return true;
}

//

void RenderPipeline::destroy_surfaces(void)
{
	if (lpDD)
	{
		lpD3DViewport->Release();
		lpD3DViewport = NULL;

		lpD3DDevice->Release();
		lpD3DDevice = NULL;

		lpD3D->Release();
		lpD3D = NULL;

		lpZBuffer->Release();
		lpZBuffer = NULL;

		lpDDSBack->Release();
		lpDDSBack = NULL;

		lpDDSPrimary->Release();
		lpDDSPrimary = NULL;
	}
}

//

void COMAPI RenderPipeline::getDXinfo(DirectXInfo * dxinfo)
{
	if (dxinfo)
	{
		dxinfo->lpDDSPrimary	= lpDDSPrimary;
		dxinfo->lpDDSBack		= lpDDSBack;
		dxinfo->lpZBuffer		= lpZBuffer;

		dxinfo->lpDD			= lpDD;
		dxinfo->lpD3D			= lpD3D;
		dxinfo->lpD3DDevice		= lpD3DDevice;
		dxinfo->lpD3DViewport	= lpD3DViewport;

		dxinfo->num_texture_formats = num_texture_formats;
		memcpy(dxinfo->texture_formats, texture_formats, sizeof(PixelFormat) * num_texture_formats);

		dxinfo->screen_pixel_format = &screen_pixel_format;
	}
}

//

void COMAPI RenderPipeline::set_display_mode(int hres, int vres, int bpp)
{
	if (!exclusive_mode_set)
	{
		HRESULT ddr = lpDD->SetCooperativeLevel(hWnd, DDSCL_EXCLUSIVE | DDSCL_ALLOWREBOOT | DDSCL_FULLSCREEN);
		if (ddr != DD_OK)
		{
			DebugPrint("RenderPipeline: SetCooperativeLevel() failed: %s\n", DD_message(ddr));
			return;
		}

		exclusive_mode_set = true;
	}


	if (DX_objects_owned)
	{
		destroy_surfaces();
	}

	HRESULT ddr = lpDD->SetDisplayMode(hres, vres, bpp, 0, 0);
	if (ddr != DD_OK)
	{
	  	DebugPrint("RenderPipeline: SetDisplayMode() failed: %s\n", DD_message(ddr));
		return;
	}

	display_mode_set = true;

	if (DX_objects_owned)
	{
		create_surfaces();
	}
}

//

void COMAPI RenderPipeline::restore_display_mode(void)
{
	if (display_mode_set)
	{
		if (DX_objects_owned)
		{
			destroy_surfaces();
		}

		HRESULT ddr = lpDD->RestoreDisplayMode();
		if (ddr != DD_OK)
		{
		  	DebugPrint("RenderPipeline: RestoreDisplayMode() failed: %s\n", DD_message(ddr));
			return;

		}
		display_mode_set = false;

		if (exclusive_mode_set)
		{
			HRESULT ddr = lpDD->SetCooperativeLevel(NULL, DDSCL_NORMAL);
			if (ddr != DD_OK)
			{
				DebugPrint("RenderPipeline: SetCooperativeLevel() failed: %s\n", DD_message(ddr));
				return;
			}

			exclusive_mode_set = false;
		}

		if (DX_objects_owned)
		{
			create_surfaces();
		}
	}
}

//

BOOL32 COMAPI RenderPipeline::startup(HWND _hWnd, const DirectXInfo * dxinfo)
{
	hWnd = _hWnd;
	lpDDSPrimary	= dxinfo->lpDDSPrimary;
	lpDDSBack		= dxinfo->lpDDSBack;
	lpZBuffer		= dxinfo->lpZBuffer;

	lpDD			= dxinfo->lpDD;
	lpD3D			= dxinfo->lpD3D;
	lpD3DDevice		= dxinfo->lpD3DDevice;
	lpD3DViewport	= dxinfo->lpD3DViewport;

	num_texture_formats = dxinfo->num_texture_formats;
	memcpy(texture_formats, dxinfo->texture_formats, sizeof(PixelFormat) * num_texture_formats);

	screen_pixel_format = *dxinfo->screen_pixel_format;

	DX_objects_owned = false;

#ifdef DX_TRANSFORMS

	D3DMATRIX I;
	for (int i = 0; i < 4; i++)
	{
		for (int j = 0; j < 4; j++)
		{
			I.m[i][j] = (i == j) ? 1.0 : 0.0;
		}
	}

//
// DIRECT3D USES A LEFT-HANDED COORDINATE SYSTEM.
// Use the view matrix to scale everything by -1 in the z direction.
//
// It's cheaper to leave the view matrix constant and use the world matrix as
// modelview because of the way D3D computes things internally.
//
	D3DMATRIX I_flip_z;
	memcpy(&I_flip_z, &I, sizeof(D3DMATRIX));
	I_flip_z._33 = -1;

	lpD3DDevice->SetTransform(D3DTRANSFORMSTATE_WORLD,		&I);
	lpD3DDevice->SetTransform(D3DTRANSFORMSTATE_VIEW,		&I_flip_z);
	lpD3DDevice->SetTransform(D3DTRANSFORMSTATE_PROJECTION,	&I);

#endif

	return TRUE;
}

//

BOOL32 COMAPI RenderPipeline::startup(HWND _hWnd, int hres, int vres, int bpp, BOOL32 set_display_mode, BOOL32 flip_if_possible)
{
	BOOL32 result = FALSE;

	hWnd = _hWnd;
	if (setupD3D())
	{
		if (set_display_mode)
		{
			HRESULT ddr = lpDD->SetCooperativeLevel(hWnd, DDSCL_EXCLUSIVE | DDSCL_ALLOWREBOOT | DDSCL_FULLSCREEN);
			if (ddr != DD_OK)
			{
				DebugPrint("RenderPipeline: SetCooperativeLevel() failed: %s\n", DD_message(ddr));
				return FALSE;
			}

			exclusive_mode_set = true;

			ddr = lpDD->SetDisplayMode(hres, vres, bpp, 0, 0);
			if (ddr != DD_OK)
			{
				DebugPrint("RenderPipeline: SetDisplayMode() failed: %s\n", DD_message(ddr));
				return FALSE;
			}

			display_mode_set = true;
		}

		if (create_surfaces())
		{
			result = TRUE;
		}
		DX_objects_owned = true;
	}

	return result;
}

//

void COMAPI RenderPipeline::shutdown(void)
{
	if (DX_objects_owned)
	{
		destroy_surfaces();

		if (display_mode_set)
		{
			lpDD->RestoreDisplayMode();
		}

		shutdownD3D();
	}
}

//

RenderPipeline::RenderPipeline(void)
{
	lpDDSPrimary = NULL;
	lpDDSBack = NULL;
	lpZBuffer = NULL;

	lpDD = NULL;
	lpD3D = NULL;
	lpD3DDevice = NULL;
	lpD3DViewport = NULL;

	DX_objects_owned = false;

	num_texture_formats = 0;

	texture = 0;
	blend = false;
	src_func = D3DBLEND_ONE;
	dst_func = D3DBLEND_ZERO;
	depth_func = D3DCMP_LESS;

	float h_fov = 45.0 * 3.14159 / 180.;

	float aspect = 4.0/3;

	float near_plane_w = 1.0 * tan(h_fov);
	float near_plane_h = near_plane_w / aspect;

	screen_w = 640;
	screen_h = 480;

	h_scale = screen_w / 2;
	v_scale = screen_h / 2;

	h_offset = h_scale;
	v_offset = v_scale;

	v_scale = -v_scale;

	opaque_index = 0;
	opaque_pool_size = 512 * 1024;
	opaque_pool = new unsigned char[opaque_pool_size];

	alpha_index = 0;
	alpha_pool_size = 64 * 1024;
	alpha_pool = new unsigned char[alpha_pool_size];

	texture = 0;
	blend = false;
	src_func = D3DBLEND_ONE;
	dst_func = D3DBLEND_ZERO;
	depth_func = D3DCMP_LESS;

	depth_sort_alpha = true;

	auto_flush = 0;

	for (int i = 0; i < 4; i++)
	{
		for (int j = 0; j < 4; j++)
		{
			if (i == j)
			{
#ifdef DX_TRANSFORMS
				Mworld.m[i][j] = Mview.m[i][j] = Mproj.m[i][j] = 1.0;
#else
				Mview[i][j] = Mproj[i][j] = M[i][j] = 1.0;
#endif
			}
			else
			{
#ifdef DX_TRANSFORMS
				Mworld.m[i][j] = Mview.m[i][j] = Mproj.m[i][j] = 0.0;
#else
				Mview[i][j] = Mproj[i][j] = M[i][j] = 0.0;
#endif
			}
		}
	}

	display_mode_set = false;
	exclusive_mode_set = false;

	// Texture variables
	int min_txm_width = 0;
	int min_txm_height = 0;
	int max_txm_width = 0;
	int max_txm_height = 0;
	bool use_mipmaps = true;
}

//

void RenderPipeline::set_viewport(int _x, int _y, int _w, int _h)
{
#ifdef DX_TRANSFORMS

	x = _x;
	y = _y;
	w = _w;
	h = _h;
/*
	D3DVIEWPORT2 viewData;
	memset(&viewData, 0, sizeof(D3DVIEWPORT2));
	viewData.dwSize         =   sizeof(D3DVIEWPORT2);
	viewData.dwX            =   x;
	viewData.dwY            =   y;
	viewData.dwWidth        =   w;
	viewData.dwHeight       =   h;
	viewData.dvClipX        =   -1;
	viewData.dvClipY        =   1;
	viewData.dvClipWidth    =   2;
	viewData.dvClipHeight   =   2;
	viewData.dvMinZ         =   0.0f;
	viewData.dvMaxZ         =   1.0f;

	HRESULT result = lpD3DViewport->SetViewport2(&viewData);
	if (result != DD_OK)
	{
		DebugAlert(NULL, "RP: SetViewport2() failed: %s",DD_message(result));
	}
*/
#else
	y = screen_h - y - h;

	screen_w = w;
	screen_h = h;

	h_scale = screen_w / 2;
	v_scale = screen_h / 2;

	h_offset = x + h_scale;
	v_offset = y + v_scale;

	v_scale = -v_scale;
#endif
}

//
// Concatenate modelview and projection matrices.
//
void RenderPipeline::compute_M(void)
{
#ifndef DX_TRANSFORMS
	for (int i = 0; i < 4; i++)
	{
		for (int j = 0; j < 4; j++)
		{
			M[i][j] = 0;
			for (int k = 0; k < 4; k++)
			{
				M[i][j] += Mproj[i][k] * Mview[k][j];
			}
		}
	}
#endif
}

//

void COMAPI RenderPipeline::set_modelview(const Transform & modelview)
{
#ifdef DX_TRANSFORMS

	Transform2D3D(Mworld, modelview);

#else
	for (int i = 0; i < 3; i++)
	{
		for (int j = 0; j < 3; j++)
		{
			Mview[i][j] = modelview.d[i][j];
		}
	}

	Mview[0][3] = modelview.translation.x;
	Mview[1][3] = modelview.translation.y;
	Mview[2][3] = modelview.translation.z;
	Mview[3][0] = Mview[3][1] = Mview[3][2] = 0;
	Mview[3][3] = 1;

	compute_M();
#endif
}

//

void RenderPipeline::set_ortho(float x, float y, float w, float h, float _znear, float _zfar)
{
#ifdef DX_TRANSFORMS

	znear = _znear;
	zfar = _zfar;
	memset(&Mproj, 0, sizeof(D3DMATRIX));

	Mproj(0, 0) = 2.0 / w;
	Mproj(1, 1) = 2.0 / h;
	Mproj(2, 2) = 1.0 / (zfar - znear);
	Mproj(3, 2) = -znear / (zfar - znear);
	Mproj(3, 3) = 1.0;

//	lpD3DDevice->SetTransform(D3DTRANSFORMSTATE_PROJECTION, &Mproj);
#else
	compute_M();
#endif
}

//

void RenderPipeline::set_perspective(float fovy, float aspect, float _znear, float _zfar)
{
#ifdef DX_TRANSFORMS

	znear = _znear;
	zfar = _zfar;

	float fy = 2.0 * znear * tan(fovy * 3.14159/180);
	float fx = aspect * fy;

	memset(&Mproj, 0, sizeof(D3DMATRIX));
	Mproj(0, 0) = 2.0 * znear / fx; 
	Mproj(1, 1) = 2.0 * znear / fy;
	Mproj(2, 2) = zfar / (zfar - znear);
	Mproj(2, 3) = 1.0;
	Mproj(3, 2) = -zfar * znear / (zfar - znear);

//	lpD3DDevice->SetTransform(D3DTRANSFORMSTATE_PROJECTION, &Mproj);

#else
	znear = _znear;
	zfar = _zfar;

	float fy = znear * tan(fovy * 3.14159/180);
	float fx = aspect * fy;

	float right = fx;
	float left = -fx;
	float top = fy;
	float bottom = -fy;

	float x = 2.0 * znear / (right-left);
	float y = 2.0 * znear / (top-bottom);
	float a = (right+left) / (right-left);
	float b = (top+bottom) / (top-bottom);
	float c = -(zfar + znear) / (zfar - znear);
	float d = -(2.0 * zfar * znear) / (zfar - znear);

	Mproj[0][0] = x;  Mproj[0][1] = 0;  Mproj[0][2] = a;   Mproj[0][3] = 0;
	Mproj[1][0] = 0;  Mproj[1][1] = y;  Mproj[1][2] = b;   Mproj[1][3] = 0;
	Mproj[2][0] = 0;  Mproj[2][1] = 0;  Mproj[2][2] = c;   Mproj[2][3] = d;
	Mproj[3][0] = 0;  Mproj[3][1] = 0;  Mproj[3][2] = -1;  Mproj[3][3] = 0;

	compute_M();
#endif
}

//

RenderPipeline::~RenderPipeline(void)
{
	delete [] alpha_pool;
	alpha_pool = NULL;

	delete [] opaque_pool;
	opaque_pool = NULL;
}

//

void COMAPI RenderPipeline::enable(int state)
{
}

//

void COMAPI RenderPipeline::disable(int state)
{
}

//

BOOL32 COMAPI RenderPipeline::is_enabled(int state)
{
	return FALSE;
}

//

void COMAPI RenderPipeline::begin_scene(void)
{
	lpD3DDevice->BeginScene();
}

//

void COMAPI COMAPI RenderPipeline::end_scene(void)
{
	lpD3DDevice->EndScene();
}

//

void COMAPI RenderPipeline::set_render_state(D3DRENDERSTATETYPE state, DWORD value)
{
	lpD3DDevice->SetRenderState(state, value);
}

//

void COMAPI RenderPipeline::set_list_render_state(D3DRENDERSTATETYPE state, DWORD value)
{
	switch (state)
	{
		case D3DRS_ALPHABLENDENABLE:
			blend = (value != 0);
			break;

		case D3DRS_SRCBLEND:
			src_func = (D3DBLEND) value;
			break;

		case D3DRS_DESTBLEND:
			dst_func = (D3DBLEND) value;
			break;

		case D3DRS_ZFUNC:
			depth_func = (D3DCMPFUNC) value;
			break;

		case D3DRS_TEXTUREHANDLE:
			texture = value;
			break;
	}
}

//

void COMAPI RenderPipeline::submit_list(D3DPRIMITIVETYPE type, const RPVertex1 * verts, int num_verts, bool clip)         
{
#ifdef DX_TRANSFORMS

#else
	set_modelview(modelview);
#endif

	if (!verts || !num_verts)
	{
		return;
	}

	int cnt;
	switch (type)
	{
		case D3DPT_POINTLIST:
			cnt = 1;
			break;
		case D3DPT_LINELIST:
			cnt = 2;
			break;
		case D3DPT_TRIANGLELIST:
			cnt = 3;
			break;
		default:
			cnt = 1;	// bogus, but prevents div by zero.
			break;
	}

	RPList * list;
	if (blend)
	{
		U32 new_index = alpha_index + sizeof(RPList) + sizeof(D3DTLVERTEX) * num_verts;
		if (new_index > alpha_pool_size)
		{
			DebugPrint("RenderPipeline: Alpha pool size exceeded; use set_alpha_pool_size().\n");
			return;
		}

		list = (RPList *) (alpha_pool + alpha_index);
		list->type = type;
		list->texture = texture;
		list->blend = blend;
		list->src_blend_func = src_func;
		list->dst_blend_func = dst_func;
		list->depth_func = depth_func;
		list->num_indices = 0;
		list->indices = NULL;

		alpha_index += sizeof(RPList);

#ifdef DX_TRANSFORMS
		list->verts = (D3DLVERTEX *) (alpha_pool + alpha_index);
		list->num_verts = num_verts;

		alpha_index += sizeof(D3DLVERTEX) * num_verts;
#else
		list->verts = (RPListVertex1 *) (alpha_pool + alpha_index);
		list->num_verts = num_verts;

		alpha_index += sizeof(RPListVertex1) * num_verts;
#endif

		alpha_lists.insert(list);
		num_alpha_polys += num_verts / cnt;
	}
	else
	{
		U32 new_index = opaque_index + sizeof(RPList) + sizeof(D3DTLVERTEX) * num_verts;
		if (new_index > opaque_pool_size)
		{
			DebugPrint("RenderPipeline: Opaque pool size exceeded; use set_opaque_pool_size().\n");
			return;
		}

		list = (RPList *) (opaque_pool + opaque_index);
		list->type = type;
		list->texture = texture;
		list->blend = blend;
		list->src_blend_func = src_func;
		list->dst_blend_func = dst_func;
		list->depth_func = depth_func;
		list->num_indices = 0;
		list->indices = NULL;

		opaque_index += sizeof(RPList);

#ifdef DX_TRANSFORMS
		list->verts = (D3DLVERTEX *) (opaque_pool + opaque_index);
		list->num_verts = num_verts;

		opaque_index += sizeof(D3DLVERTEX) * num_verts;
#else
		list->verts = (RPListVertex1 *) (opaque_pool + opaque_index);
		list->num_verts = num_verts;

		opaque_index += sizeof(RPListVertex1) * num_verts;
#endif
		opaque_lists.insert(list);
		num_opaque_polys += num_verts / cnt;
	}

	list->clip = clip;

#ifdef DX_TRANSFORMS

	list->modelview = Mworld;
	list->projection = Mproj;
	list->x = x;
	list->y = y;
	list->w = w;
	list->h = h;

	const RPVertex1 * src = verts;
	D3DLVERTEX * dst = list->verts;
	for (int i = 0; i < num_verts; i++, src++, dst++)
	{
		dst->x = src->pos.x;
		dst->y = src->pos.y;
		dst->z = src->pos.z;
		dst->dwReserved = 0;
		dst->color = (src->a << 24) | (src->r << 16) | (src->g << 8) | src->b;
		dst->specular = 0;
		dst->tu = src->u;
		dst->tv = src->v;
	}

#else
	if (list->clip)
	{
		bool any_clipped = false;

		const RPVertex1 * src = verts;
		RPListVertex1 * dst = list->verts;
		for (int i = 0; i < num_verts; i++, src++, dst++)
		{
			dst->x = M[0][0] * src->pos.x + M[0][1] * src->pos.y + M[0][2] * src->pos.z + M[0][3];
			dst->y = M[1][0] * src->pos.x + M[1][1] * src->pos.y + M[1][2] * src->pos.z + M[1][3];
			dst->z = M[2][0] * src->pos.x + M[2][1] * src->pos.y + M[2][2] * src->pos.z + M[2][3];
			dst->w = M[3][0] * src->pos.x + M[3][1] * src->pos.y + M[3][2] * src->pos.z + M[3][3];
			dst->r = src->r;
			dst->g = src->g;
			dst->b = src->b;
			dst->a = src->a;
			dst->u = src->u;
			dst->v = src->v;

			dst->clip = 0;

			float pos = dst->w;
			float neg = -pos;
			if (dst->x > pos)
			{
				dst->clip |= CLIP_RIGHT;
			}
			else if (dst->x < neg)
			{
				dst->clip |= CLIP_LEFT;
			}
			if (dst->y > pos)
			{
				dst->clip |= CLIP_TOP;
			}
			else if (dst->y < neg)
			{
				dst->clip |= CLIP_BOTTOM;
			}
			if (dst->z > pos)
			{
				dst->clip |= CLIP_FAR;
			}
			else if (dst->z < neg)
			{
				dst->clip |= CLIP_NEAR;
			}

			if (dst->clip)
			{
				any_clipped = true;
			}
		}

	// No clipped vertices in list.
		if (!any_clipped)
		{
			list->clip = false;
		}
	}
	else
	{
		const RPVertex1 * src = verts;
		RPListVertex1 * dst = list->verts;
		for (int i = 0; i < num_verts; i++, src++, dst++)
		{
			dst->x = M[0][0] * src->pos.x + M[0][1] * src->pos.y + M[0][2] * src->pos.z + M[0][3];
			dst->y = M[1][0] * src->pos.x + M[1][1] * src->pos.y + M[1][2] * src->pos.z + M[1][3];
			dst->z = M[2][0] * src->pos.x + M[2][1] * src->pos.y + M[2][2] * src->pos.z + M[2][3];
			dst->w = M[3][0] * src->pos.x + M[3][1] * src->pos.y + M[3][2] * src->pos.z + M[3][3];
			dst->r = src->r;
			dst->g = src->g;
			dst->b = src->b;
			dst->a = src->a;
			dst->u = src->u;
			dst->v = src->v;
			dst->clip = 0;
		}
	}
#endif
}

//

void COMAPI RenderPipeline::submit_indexed_list(D3DPRIMITIVETYPE type, const RPVertex1 * verts, int num_verts, const U16 * indices, int num_indices, bool clip)
{
#ifndef DX_TRANSFORMS
	set_modelview(modelview);
#endif

	if (!verts || !num_verts || !indices || !num_indices)
	{
		return;
	}

	int cnt;
	switch (type)
	{
		case D3DPT_POINTLIST:
			cnt = 1;
			break;
		case D3DPT_LINELIST:
			cnt = 2;
			break;
		case D3DPT_TRIANGLELIST:
			cnt = 3;
			break;
		default:
			cnt = 1;	// bogus, but prevents div by zero.
			break;
	}

	RPList * list;
	if (blend)
	{
		U32 new_index = alpha_index + sizeof(RPList) + sizeof(D3DTLVERTEX) * num_verts + sizeof(U16) * num_indices;
		if (new_index > alpha_pool_size)
		{
			DebugPrint("RenderPipeline: Alpha pool size exceeded; use set_alpha_pool_size().\n");
			return;
		}


		list = (RPList *) (alpha_pool + alpha_index);
		list->type = type;
		list->texture = texture;
		list->blend = blend;
		list->src_blend_func = src_func;
		list->dst_blend_func = dst_func;
		list->depth_func = depth_func;

		alpha_index += sizeof(RPList);

#ifdef DX_TRANSFORMS
		list->verts = (D3DLVERTEX *) (alpha_pool + alpha_index);
		list->num_verts = num_verts;

		alpha_index += sizeof(D3DLVERTEX) * num_verts;
#else
		list->verts = (RPListVertex1 *) (alpha_pool + alpha_index);
		list->num_verts = num_verts;

		alpha_index += sizeof(RPListVertex1) * num_verts;
#endif
		alpha_lists.insert(list);
		num_alpha_polys += num_indices / cnt;

		list->indices = (U16 *) (alpha_pool + alpha_index);
		list->num_indices = num_indices;
		alpha_index += sizeof(U16) * num_indices;
	}
	else
	{
		U32 new_index = opaque_index + sizeof(RPList) + sizeof(D3DTLVERTEX) * num_verts + sizeof(U16) * num_indices;
		if (new_index > opaque_pool_size)
		{
			DebugPrint("RenderPipeline: Opaque pool size exceeded; use set_opaque_pool_size().\n");
			return;
		}

		list = (RPList *) (opaque_pool + opaque_index);
		list->type = type;
		list->texture = texture;
		list->blend = blend;
		list->src_blend_func = src_func;
		list->dst_blend_func = dst_func;
		list->depth_func = depth_func;

		opaque_index += sizeof(RPList);

#ifdef DX_TRANSFORMS
		list->verts = (D3DLVERTEX *) (opaque_pool + opaque_index);
		list->num_verts = num_verts;

		opaque_index += sizeof(D3DLVERTEX) * num_verts;
#else
		list->verts = (RPListVertex1 *) (opaque_pool + opaque_index);
		list->num_verts = num_verts;

		opaque_index += sizeof(RPListVertex1) * num_verts;
#endif
		opaque_lists.insert(list);
		num_opaque_polys += num_indices / cnt;

		list->indices = (U16 *) (opaque_pool + opaque_index);
		list->num_indices = num_indices;
		opaque_index += sizeof(U16) * num_indices;
	}

	list->clip = clip;

#ifdef DX_TRANSFORMS

	list->modelview = Mworld;
	list->projection = Mproj;
	list->x = x;
	list->y = y;
	list->w = w;
	list->h = h;

	const RPVertex1 * src = verts;
	D3DLVERTEX * dst = list->verts;
	for (int i = 0; i < num_verts; i++, src++, dst++)
	{
		dst->x = src->pos.x;
		dst->y = src->pos.y;
		dst->z = src->pos.z;
		dst->dwReserved = 0;
		dst->color = (src->a << 24) | (src->r << 16) | (src->g << 8) | src->b;
		dst->specular = 0;
		dst->tu = src->u;
		dst->tv = src->v;
	}

#else
	if (list->clip)
	{
		bool any_clipped = false;

		const RPVertex1 * src = verts;
		RPListVertex1 * dst = list->verts;
		for (int i = 0; i < num_verts; i++, src++, dst++)
		{
			dst->x = M[0][0] * src->pos.x + M[0][1] * src->pos.y + M[0][2] * src->pos.z + M[0][3];
			dst->y = M[1][0] * src->pos.x + M[1][1] * src->pos.y + M[1][2] * src->pos.z + M[1][3];
			dst->z = M[2][0] * src->pos.x + M[2][1] * src->pos.y + M[2][2] * src->pos.z + M[2][3];
			dst->w = M[3][0] * src->pos.x + M[3][1] * src->pos.y + M[3][2] * src->pos.z + M[3][3];
			dst->r = src->r;
			dst->g = src->g;
			dst->b = src->b;
			dst->a = src->a;
			dst->u = src->u;
			dst->v = src->v;

#define CLIP_RIGHT	0x01
#define CLIP_LEFT	0x02
#define CLIP_TOP	0x04
#define CLIP_BOTTOM	0x08
#define CLIP_NEAR	0x10
#define CLIP_FAR	0x20
			dst->clip = 0;

			float pos = dst->w;
			float neg = -pos;
			if (dst->x > pos)
			{
				dst->clip |= CLIP_RIGHT;
			}
			else if (dst->x < neg)
			{
				dst->clip |= CLIP_LEFT;
			}
			if (dst->y > pos)
			{
				dst->clip |= CLIP_TOP;
			}
			else if (dst->y < neg)
			{
				dst->clip |= CLIP_BOTTOM;
			}
			if (dst->z > pos)
			{
				dst->clip |= CLIP_FAR;
			}
			else if (dst->z < neg)
			{
				dst->clip |= CLIP_NEAR;
			}

			if (dst->clip)
			{
				any_clipped = true;
			}
		}

	// No clipped vertices in list.
		if (!any_clipped)
		{
			list->clip = false;
		}
	}
	else
	{
		const RPVertex1 * src = verts;
		RPListVertex1 * dst = list->verts;
		for (int i = 0; i < num_verts; i++, src++, dst++)
		{
			dst->x = M[0][0] * src->pos.x + M[0][1] * src->pos.y + M[0][2] * src->pos.z + M[0][3];
			dst->y = M[1][0] * src->pos.x + M[1][1] * src->pos.y + M[1][2] * src->pos.z + M[1][3];
			dst->z = M[2][0] * src->pos.x + M[2][1] * src->pos.y + M[2][2] * src->pos.z + M[2][3];
			dst->w = M[3][0] * src->pos.x + M[3][1] * src->pos.y + M[3][2] * src->pos.z + M[3][3];
			dst->r = src->r;
			dst->g = src->g;
			dst->b = src->b;
			dst->a = src->a;
			dst->u = src->u;
			dst->v = src->v;
			dst->clip = 0;
		}
	}
#endif

	memcpy(list->indices, indices, sizeof(U16) * num_indices);

	if (auto_flush)
	{
		if (num_opaque_polys > auto_flush)
		{
			flush_opaque();
		}
	}
}

//

#define D3DVT_RPVERTEX (D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1)

struct RPVERTEX
{
	float	x, y, z, rhw;
	DWORD	dwDiffuseRGBA;
	DWORD	dwSpecularRGBA;	// REMOVE IF USING FVF
	float	tu1, tv1;

};

const unsigned int DefaultListLength = 1024 * 16;
static RPVERTEX vertex_list[DefaultListLength];
static D3DLVERTEX lvertex_list[DefaultListLength];
static U16 index_list[DefaultListLength];

//

#define LINTERP(T,A,B)   ((A)+(T)*((B)-(A)))

#define GENERAL_CLIP() \
{															\
	int tmp = 0;											\
	output = (output == tmp_chains)	? tmp_poly : tmp_chains;\
	int v = count-1;										\
	for (int vn=0; vn<count; v=vn++)						\
	{														\
		int v0 = input[v];									\
		int v1 = input[vn];									\
		int c = OUTSIDE(v0) + OUTSIDE(v1)*2;				\
		if (c == 0)	/* Neither clipped */					\
		{													\
			output[tmp++] = input[vn];						\
		}													\
		else if (c == 3)									\
		{													\
			/* Both clipped. */								\
		}													\
		else												\
		{													\
			int NEW = more++;								\
			output[tmp++] = NEW;							\
			vsrc[NEW].clip = 0;								\
			int in,out;										\
			if (c == 1)	/* Current clipped, next not */		\
			{												\
				in = input[vn];								\
				out = input[v];								\
				output[tmp++] = in;							\
			}												\
			else /* Next clipped, current not */			\
			{												\
				in = input[v];								\
				out = input[vn];							\
			}												\
			INTERP;											\
			MINTERP;										\
		}													\
	}														\
	count = tmp; \
	input = output; \
}


int clip_poly(RPListVertex1 * vsrc, int * tmp_chains, int count, int any)
{
// Polygon partially clipped

	#define CX(i) vsrc[i].x
	#define CY(i) vsrc[i].y
	#define CZ(i) vsrc[i].z
	#define CW(i) vsrc[i].w

	#define R(i) vsrc[i].r
	#define G(i) vsrc[i].g
	#define B(i) vsrc[i].b
	#define A(i) vsrc[i].a

	#define U(i) vsrc[i].u
	#define V(i) vsrc[i].v

#define MAX_POLY_SIDES	16

	int tmp_input[MAX_POLY_SIDES];
	for (int i = 0; i < count; i++)
	{
		tmp_input[i] = i;
	}
	int * input = tmp_input;

	int tmp_poly[MAX_POLY_SIDES];
	int more = count;

	int * output = NULL;

// Clip against -Z side

	#define OUTSIDE(i) (CZ(i) < -CW(i))

	// WARNING: rearranging INTERP's innards fixed a compile bug in the Release version! (pci)
	#define INTERP \
		double dz = CZ(out) - CZ(in);			\
		double dw = CW(out) - CW(in);			\
		double t = -(CZ(in) + CW(in)) / (dw+dz);\
		CX(NEW) = LINTERP(t,CX(in),CX(out));	\
		CY(NEW) = LINTERP(t,CY(in),CY(out));	\
		CZ(NEW) = CZ(in) + t * dz;				\
		CW(NEW) = CW(in) + t * dw;				\

	#define MINTERP \
		R(NEW) = LINTERP(t, R(in), R(out));		\
		G(NEW) = LINTERP(t, G(in), G(out));		\
		B(NEW) = LINTERP(t, B(in), B(out));		\
		A(NEW) = LINTERP(t, A(in), A(out));		\
		U(NEW) = LINTERP(t, U(in), U(out));		\
		V(NEW) = LINTERP(t, V(in), V(out));		\

	if (any & CLIP_NEAR)
	{
		GENERAL_CLIP();
		// Everything can change behind the near plane!
		any |= CLIP_LEFT|CLIP_RIGHT|CLIP_TOP|CLIP_BOTTOM;
	}

	#undef INTERP
	#undef OUTSIDE

// Clip against +Z side

	#define OUTSIDE(i) (CZ(i) > CW(i))

	#define INTERP \
		double dz = CZ(out) - CZ(in);			\
		double dw = CW(out) - CW(in);			\
		double t = (CZ(in) - CW(in)) / (dw-dz);	\
		CX(NEW) = LINTERP(t,CX(in),CX(out));	\
		CY(NEW) = LINTERP(t,CY(in),CY(out));	\
		CZ(NEW) = CZ(in) + t * dz;				\
		CW(NEW) = CW(in) + t * dw;

	if (any & CLIP_FAR)
	{
		GENERAL_CLIP();
	}

	#undef INTERP
	#undef OUTSIDE

// Clip against +X side

	#define OUTSIDE(i) (CX(i) > CW(i))

	#define INTERP \
		double dx = CX(out) - CX(in);			\
		double dw = CW(out) - CW(in);			\
		double t = (CX(in) - CW(in)) / (dw-dx);	\
		float new_w = CW(in) + t*dw;			\
		CX(NEW) = new_w;						\
		CY(NEW) = LINTERP(t,CY(in),CY(out));	\
		CZ(NEW) = LINTERP(t,CZ(in),CZ(out));	\
		CW(NEW) = new_w;

	if (any & CLIP_RIGHT)
	{
		GENERAL_CLIP();
	}

	#undef INTERP
	#undef OUTSIDE

// Clip against -X side

	#define OUTSIDE(i) (CX(i) < -CW(i))

	#define INTERP \
		double dx = CX(out) - CX(in);			\
		double dw = CW(out) - CW(in);			\
		double t = -(CX(in) + CW(in)) / (dw+dx);\
		float new_w = CW(in) + t*dw;			\
		CX(NEW) = -new_w;						\
		CY(NEW) = LINTERP(t,CY(in),CY(out));	\
		CZ(NEW) = LINTERP(t,CZ(in),CZ(out));	\
		CW(NEW) = new_w;

	if (any & CLIP_LEFT)
	{
		GENERAL_CLIP();
	}

	#undef INTERP
	#undef OUTSIDE

// Clip against +Y side

	#define OUTSIDE(i) (CY(i) > CW(i))

	#define INTERP \
		double dy = CY(out) - CY(in);			\
		double dw = CW(out) - CW(in);			\
		double t = (CY(in) - CW(in)) / (dw-dy);	\
		float new_w = CW(in) + t*dw;			\
		CX(NEW) = LINTERP(t,CX(in),CX(out));	\
		CY(NEW) = new_w;						\
		CZ(NEW) = LINTERP(t,CZ(in),CZ(out));	\
		CW(NEW) = new_w;

	if (any & CLIP_TOP)
	{
		GENERAL_CLIP();
	}

	#undef INTERP
	#undef OUTSIDE

// Clip against -Y side

	#define OUTSIDE(i) (CY(i) < -CW(i))

	#define INTERP \
		double dy = CY(out) - CY(in);			\
		double dw = CW(out) - CW(in);			\
		double t = -(CY(in) + CW(in)) / (dw+dy);\
		float new_w = CW(in) + t*dw;			\
		CX(NEW) = LINTERP(t,CX(in),CX(out));	\
		CY(NEW) = -new_w;						\
		CZ(NEW) = LINTERP(t,CZ(in),CZ(out));	\
		CW(NEW) = new_w;

	if (any & CLIP_BOTTOM)
	{
		GENERAL_CLIP();
	}

	#undef INTERP
	#undef OUTSIDE

	if (output != tmp_chains)
	{
		memcpy(tmp_chains,output,count*sizeof(output[0]));
	}

	return count;
}


//

#ifndef DX_TRANSFORMS
void RenderPipeline::do_unclipped_list(const RPList * list, RPVERTEX *& v_ptr)
{
	RPListVertex1 * vsrc = list->verts;
	for (unsigned int j = 0; j < list->num_verts; )
	{
		float one_over_w = 1.0 / vsrc->w;

	// do as much as possible while waiting for divide.
		v_ptr->dwDiffuseRGBA = (vsrc->a << 24) | (vsrc->r << 16) | (vsrc->g << 8) | vsrc->b;
		v_ptr->tu1 = vsrc->u;
		v_ptr->tv1 = vsrc->v;

		j++;
		RPListVertex1 * vs = vsrc++;
		RPVERTEX * vd = v_ptr++;

		vd->x = vs->x * one_over_w * h_scale + h_offset;
		vd->y = vs->y * one_over_w * v_scale + v_offset;
		vd->z = 1.0 - one_over_w;
//		vd->z = vs->z * one_over_w;
		vd->rhw = one_over_w;
	}
}

//

void RenderPipeline::do_unclipped_indexed_list(const RPList * list, RPVERTEX *& v_ptr, U16 *& i_ptr)
{
// Build up vertex/index lists...
	unsigned int offset = v_ptr - vertex_list;

	RPListVertex1 * vsrc = list->verts;
	for (unsigned int i = 0; i < list->num_verts; i++, vsrc++, v_ptr++)
	{
		float w = 1.0 / vsrc->w;

		v_ptr->dwDiffuseRGBA = (vsrc->a << 24) | (vsrc->r << 16) | (vsrc->g << 8) | vsrc->b;
		v_ptr->tu1 = vsrc->u;
		v_ptr->tv1 = vsrc->v;

		v_ptr->x = vsrc->x * w * h_scale + h_offset;
		v_ptr->y = vsrc->y * w * v_scale + v_offset;
		v_ptr->z = 1.0 - w;
		v_ptr->rhw = w;
	}

	assert((v_ptr - vertex_list) < DefaultListLength);	// check overrun.

// Need to adjust indices...
	U16 * isrc = list->indices;
	for (i = 0; i < list->num_indices; i++, i_ptr++, isrc++)
	{
		*i_ptr = *isrc + offset;
	}

	assert((i_ptr - index_list) < DefaultListLength);	// check overrun.
}

//

void RenderPipeline::clip_list(const RPList * list, RPVERTEX *& v_ptr, int cnt)
{
	int num_prims = list->num_verts / cnt;
	RPListVertex1 * vsrc = list->verts;
	for (int i = 0; i < num_prims; i++)
	{
		int any_clipped = vsrc->clip;
		int all_clipped = any_clipped;
		for (int j = 1; j < cnt; j++)
		{
			int clip = vsrc[j].clip;
			any_clipped |= clip;
			all_clipped &= clip;
		}

		if (!any_clipped)
		{
			for (int j = 0; j < cnt; )
			{
				float w = 1.0 / vsrc->w;

			// do as much as possible while waiting for divide.
				v_ptr->dwDiffuseRGBA = (vsrc->a << 24) | (vsrc->r << 16) | (vsrc->g << 8) | vsrc->b;
				v_ptr->tu1 = vsrc->u;
				v_ptr->tv1 = vsrc->v;

				j++;
				RPListVertex1 * vs = vsrc++;
				RPVERTEX * vd = v_ptr++;

				vd->x = vs->x * w * h_scale + h_offset;
				vd->y = vs->y * w * v_scale + v_offset;
				vd->z = 1.0 - w;
				vd->rhw = w;
			}
		}
		else if (!all_clipped)
		{
		//
		// GOTTA CLIP.
		//
			RPListVertex1 poly[16];
			RPVERTEX clipped[16];
			int index[16];

			memcpy(poly, vsrc, sizeof(RPListVertex1) * cnt);
			int num_verts = clip_poly(poly, index, cnt, any_clipped);
			int * idx = index;

			RPVERTEX * c_ptr = clipped;
			for (j = 0; j < num_verts; )
			{
				RPListVertex1 * src = poly + *idx;
				float w = 1.0 / src->w;

			// do as much as possible while waiting for divide.
				c_ptr->dwDiffuseRGBA = (src->a << 24) | (src->r << 16) | (src->g << 8) | src->b;
				c_ptr->tu1 = src->u;
				c_ptr->tv1 = src->v;

				j++, idx++;
				RPListVertex1 * vs = src++;
				RPVERTEX * cd = c_ptr++;

				cd->x = vs->x * w * h_scale + h_offset;
				cd->y = vs->y * w * v_scale + v_offset;
				cd->z = 1.0 - w;
				cd->rhw = w;
			}

			for (j = 2; j < num_verts; j++)
			{
				*(v_ptr++) = clipped[0];
				*(v_ptr++) = clipped[j-1];
				*(v_ptr++) = clipped[j];
			}


			vsrc += cnt;
		}
	}
}

//

void RenderPipeline::clip_indexed_list(const RPList * list, RPVERTEX *& v_ptr, U16 *& idx_ptr, int cnt)
{
	U16 * istart = idx_ptr;
	int offset = v_ptr - vertex_list;

//
// Copy all existing verts to draw buffer.
//
	do_unclipped_indexed_list(list, v_ptr, idx_ptr);

	int num_prims = list->num_indices / cnt;
	idx_ptr = istart;
	U16 * ilist = list->indices;

	for (int i = 0; i < num_prims; i++, ilist += cnt)
	{
		int any_clipped = list->verts[ilist[0]].clip;
		int all_clipped = any_clipped;
		for (int j = 1; j < cnt; j++)
		{
			int clip = list->verts[ilist[j]].clip;
			any_clipped |= clip;
			all_clipped &= clip;
		}

		if (!any_clipped)
		{
			for (int v = 0; v < cnt; v++)
			{
				*(idx_ptr++) = offset + ilist[v];
			}
		}
		else if (!all_clipped)
		{
		//
		// Clip poly, which may generate new vertices.
		//
			RPListVertex1 poly[16];
			int index[16];

			for (int v = 0; v < cnt; v++)
			{
				poly[v] = list->verts[ilist[v]];
			}

			int num_verts = clip_poly(poly, index, cnt, any_clipped);

		//
		// Add new vertices to end of vlist using existing v_ptr.
		//
			int index_remap[16];

			for (v = 0; v < num_verts; v++)
			{
				int iv = index[v];
				if (iv < cnt)
				{
					index_remap[v] = offset + ilist[iv];
				}
				else	// new vertex.
				{
					RPListVertex1 * vsrc = poly + iv;

					float w = 1.0 / vsrc->w;

				// do as much as possible while waiting for divide.
					v_ptr->dwDiffuseRGBA = (vsrc->a << 24) | (vsrc->r << 16) | (vsrc->g << 8) | vsrc->b;
					v_ptr->tu1 = vsrc->u;
					v_ptr->tv1 = vsrc->v;

					index_remap[v] = v_ptr - vertex_list;
					j++;
					RPListVertex1 * vs = vsrc++;
					RPVERTEX * vd = v_ptr++;

					assert((v_ptr - vertex_list) < DefaultListLength);

					vd->x = vs->x * w * h_scale + h_offset;
					vd->y = vs->y * w * v_scale + v_offset;
					vd->z = 1.0 - w;
					vd->rhw = w;
				}
			}

		// triangulate.
			for (j = 2; j < num_verts; j++)
			{
				*(idx_ptr++) = index_remap[0];
				*(idx_ptr++) = index_remap[j-1];
				*(idx_ptr++) = index_remap[j];

				assert((idx_ptr - index_list) < DefaultListLength);
			}
		}
	}
}
#endif
//
#ifdef DX_TRANSFORMS
void RenderPipeline::render_opaque_lists(void)
{
	glEnable(GL_TEXTURE_2D);
	ListNode * node = opaque_lists.used_entries;
	if (node)
	{
		set_render_state(D3DRS_ALPHABLENDENABLE, FALSE);
		set_render_state(D3DRS_ZENABLE, TRUE);
		glBindTexture(GL_TEXTURE_2D,0);
		glBegin(GL_TRIANGLES);
		glEnd();

	//
	// GOAL: coalesce adjacent lists that have same D3D states.
	// Minimize state changes.
	//
//		LPDIRECT3DTEXTURE2 txm = 0;
		unsigned int txm = 0;
		D3DCMPFUNC depth_func = D3DCMP_FORCE_DWORD;

		while (node)
		{
		// 
		// Get next list.
		//
			const RPList * list = node->list;

			DWORD flags = D3DDP_DONOTUPDATEEXTENTS;
			if (!list->clip)
			{
				flags |= D3DDP_DONOTCLIP;
			}

		//
		// Check for state change.
		//
			if (list->texture != txm)
			{
				txm = list->texture;
			 	glBindTexture(GL_TEXTURE_2D, txm);
			 	glBegin(GL_TRIANGLES);
			 	glEnd();
			}
			if (list->depth_func != depth_func)
			{
				depth_func = list->depth_func;
				set_render_state(D3DRS_ZFUNC, depth_func);
			}

		//
		// Render list.
		//

		//
		// Set up matrices & viewport. We should find a way to see if they really need to change or not.
		//
			lpD3DDevice->SetTransform(D3DTRANSFORMSTATE_WORLD, const_cast<D3DMATRIX *>(&list->modelview));
			lpD3DDevice->SetTransform(D3DTRANSFORMSTATE_PROJECTION, const_cast<D3DMATRIX *>(&list->projection));

			if ((list->x != last_x) || (list->y != last_y) || (list->w != last_w) || (list->h != last_h))
			{
				D3DVIEWPORT2 viewData;
				memset(&viewData, 0, sizeof(D3DVIEWPORT2));
				viewData.dwSize         =   sizeof(D3DVIEWPORT2);
				viewData.dwX            =   list->x;
				viewData.dwY            =   list->y;
				viewData.dwWidth        =   list->w;
				viewData.dwHeight       =   list->h;
				viewData.dvClipX        =   -1;
				viewData.dvClipY        =   1;
				viewData.dvClipWidth    =   2;
				viewData.dvClipHeight   =   2;
				viewData.dvMinZ         =   0.0f;
				viewData.dvMaxZ         =   1.0f;

				HRESULT result = lpD3DViewport->SetViewport2(&viewData);
				if (result != DD_OK)
				{
					DebugAlert(NULL, "RP: SetViewport2() failed: %s",DD_message(result));
				}

				last_x = list->x;
				last_y = list->y;
				last_w = list->w;
				last_h = list->h;
			}

			if (list->is_indexed())
			{
				if (list->num_verts && list->num_indices)
				{
					HRESULT dd = lpD3DDevice->DrawIndexedPrimitive(list->type, D3DVT_LVERTEX, list->verts, list->num_verts, list->indices, list->num_indices, flags);
					if (dd != DD_OK)
					{
						DebugPrint("RP: DrawIndexedPrimitive() failed: %s\n", DD_message(dd));
					}
				}
			}
			else
			{
				if (list->num_verts)
				{
					HRESULT ddrval = lpD3DDevice->DrawPrimitive(list->type, D3DVT_LVERTEX, list->verts, list->num_verts, flags);
					if (ddrval != DD_OK)
					{
						DebugPrint("RP: DrawPrimitive() failed: %s\n", DD_message(ddrval));
					}
				}
			}

			node->list = NULL;
			node = node->next;
		}
	}
}
#else
void RenderPipeline::render_opaque_lists(void)
{
	glEnable(GL_TEXTURE_2D);
	ListNode * node = opaque_lists.used_entries;
	if (node)
	{
		enum
		{
			SC_TXM		= 0x01,
			SC_TYPE		= 0x02,
			SC_SRC		= 0x04,
			SC_DST		= 0x08,
			SC_DEPTH	= 0x10
		};

		set_render_state(D3DRS_ALPHABLENDENABLE, FALSE);
		set_render_state(D3DRS_ZENABLE, TRUE);
		glBindTexture(GL_TEXTURE_2D,0);
		glBegin(GL_TRIANGLES);
		glEnd();


		RPVERTEX * v_ptr = vertex_list;
		U16 * idx_ptr = index_list;
	//
	// GOAL: coalesce adjacent lists that have same D3D states.
	// Minimize state changes.
	//
		enum
		{
			BL_NORMAL = 1,
			BL_INDEXED
		};

		unsigned int inside_block = 0;
//		LPDIRECT3DTEXTURE2 txm = 0;
		unsigned int txm = 0;
		D3DPRIMITIVETYPE type = D3DPT_FORCE_DWORD;	// init to invalid values.
		D3DCMPFUNC depth_func = D3DCMP_FORCE_DWORD;

		DWORD flags = D3DDP_DONOTCLIP | D3DDP_DONOTUPDATEEXTENTS;

		while (node)
		{
		// 
		// Get next list.
		//
			const RPList * list = node->list;

			int cnt;
			switch (list->type)
			{
				case D3DPT_POINTLIST:
					cnt = 1;
					break;
				case D3DPT_LINELIST:
					cnt = 2;
					break;
				case D3DPT_TRIANGLELIST:
					cnt = 3;
					break;
				default:
					cnt = 1;	// bogus, but prevents div by zero.
					break;
			}

			unsigned int state_change = 0;
		/*
			if (list->clip)
			{
				flags &= ~D3DDP_DONOTCLIP;
			}
		*/
		//
		// Check for state change.
		//
			if (list->texture != txm)
			{
				txm = list->texture;
				state_change |= SC_TXM;
			}
			if (list->type != type)
			{
				type = list->type;
				state_change |= SC_TYPE;
			}
			if (list->depth_func != depth_func)
			{
				depth_func = list->depth_func;
				state_change |= SC_DEPTH;
			}

		//
		// Do we need to flush? Flush conditions:
		//
		// 1. state change.
		// 2. list length overrun.
		// 3. indexed vs. non-indexed blocks.
		//
			unsigned int vertex_limit = (v_ptr - vertex_list) + list->num_verts;
			unsigned int index_limit = (idx_ptr - index_list) + list->num_indices;
			bool overrun = (vertex_limit >= DefaultListLength) || (index_limit >= DefaultListLength);

			bool index_change;
			if (list->is_indexed())
			{
				if (inside_block == BL_NORMAL)
				{
					index_change = true;
				}
				else
				{
					index_change = false;
				}
			}
			else if (inside_block == BL_INDEXED)
			{
				index_change = true;
			}
			else 
			{
				index_change = false;
			}

			bool flush = state_change || overrun || index_change;

			if (flush)
			{
			//
			// Flush previous list.
			//
				if (inside_block == BL_NORMAL)
				{
					DWORD num_verts = v_ptr - vertex_list;
					HRESULT ddrval = lpD3DDevice->DrawPrimitive(type, D3DVT_TLVERTEX, vertex_list, num_verts, flags);

					if (ddrval != DD_OK)
					{
						DebugPrint("RP: DrawPrimitive() failed: %s\n", DD_message(ddrval));
					}

					inside_block = 0;
				}
				else if (inside_block == BL_INDEXED)
				{
					DWORD num_verts = v_ptr - vertex_list;
					DWORD num_indices = idx_ptr - index_list;

					if (num_verts && num_indices)
					{
						HRESULT dd = lpD3DDevice->DrawIndexedPrimitive(type, D3DVT_TLVERTEX, vertex_list, num_verts, index_list, num_indices, flags);
						if (dd != DD_OK)
						{
							DebugPrint("RP: DrawIndexedPrimitive() failed: %s\n", DD_message(dd));
						}
					}
					inside_block = 0;
				}

				flags = D3DDP_DONOTCLIP | D3DDP_DONOTUPDATEEXTENTS;
			}

			if (state_change)
			{
			//
			// Register state change.
			//
				if (state_change & SC_TXM)
				{
				/*
					DWORD h;
					txm->GetHandle(lpD3DDevice, &h);
					lpD3DDevice->SetRenderState(D3DRS_TEXTUREHANDLE, h);
				*/
					glBindTexture(GL_TEXTURE_2D, txm);
					glBegin(GL_TRIANGLES);
					glEnd();
				}
				if (state_change & SC_DEPTH)
				{
					set_render_state(D3DRS_ZFUNC, depth_func);
				}
			}

			if (flush)
			{
			//
			// Begin new D3D block.
			//
				if (list->is_indexed())
				{
					inside_block = BL_INDEXED;
				}
				else
				{
					inside_block = BL_NORMAL;
				}
			}

		//
		// Submit list's polys to D3D.
		//
			if (list->is_indexed())
			{
				if (list->clip)
				{
					clip_indexed_list(list, v_ptr, idx_ptr, cnt);
				}
				else
				{
					do_unclipped_indexed_list(list, v_ptr, idx_ptr);
				}
			}
			else
			{
				if (list->clip)
				{
					clip_list(list, v_ptr, cnt);
				}
				else
				{
				//
				// List contains no clipped vertices.
				//
					do_unclipped_list(list, v_ptr);
				}
			}

			node->list = NULL;
			node = node->next;
		}

	//
	// Flush last block.
	// 
		if (inside_block == BL_NORMAL)
		{
			DWORD num_verts = v_ptr - vertex_list;
			if (num_verts)
			{
				HRESULT ddrval = lpD3DDevice->DrawPrimitive(type, D3DVT_TLVERTEX, vertex_list, num_verts, flags);
				if (ddrval != DD_OK)
				{
					DebugPrint("RP: DrawPrimitive() failed: %s\n", DD_message(ddrval));
				}
			}
		}
		else if (inside_block == BL_INDEXED)
		{
			DWORD num_verts = v_ptr - vertex_list;
			DWORD num_indices = idx_ptr - index_list;
			if (num_verts && num_indices)
			{
				HRESULT dd = lpD3DDevice->DrawIndexedPrimitive(type, D3DVT_TLVERTEX, vertex_list, num_verts, index_list, num_indices, flags);
				if (dd != DD_OK)
				{
					DebugPrint("RP: DrawIndexedPrimitive() failed: %s\n", DD_message(dd));
				}
			}
		}
	}
}
#endif
//

struct Poly
{
	const RPList *	list;
	int				index;
	float			min_z;
};

//

int ComparePolys(const void *elem1, const void *elem2)
{
	int result;

	Poly * p1 = (Poly *) elem1;
	Poly * p2 = (Poly *) elem2;
	
	if (p1->min_z < p2->min_z)
	{
		result = -1;
	}
	else if (p1->min_z > p2->min_z)
	{
		result = 1;
	}
	else
	{
		result = 0;
	}

	return result;
}

//
#ifdef DX_TRANSFORMS

//
// CORRECT METHOD: depth sort all polys, render back to front regardless of state changes.
// CHEAPO METHOD: within each texture, sort all polys, render back to front.
//
void RenderPipeline::render_alpha_lists_depth_sorted(void)
{
	ListNode * node = alpha_lists.used_entries;
	if (node)
	{
		DynamicArray<Poly> poly_list(256);

	// Collect polygons for sorting.
		int num_polys = 0;
		while (node)
		{
			const RPList * list = node->list;
			int count = (list->is_indexed()) ? list->num_indices : list->num_verts;
			for (int i = 0; i < count; )
			{
				int cnt;
				switch (list->type)
				{
					case D3DPT_POINTLIST:
						cnt = 1;
						break;
					case D3DPT_LINELIST:
						cnt = 2;
						break;
					case D3DPT_TRIANGLELIST:
						cnt = 3;
						break;
				}

				poly_list[num_polys].list = list;
				poly_list[num_polys].index = i;

				float Mx = list->modelview._31;
				float My = list->modelview._32;
				float Mz = list->modelview._33;
				float Tz = list->modelview._43;

				float min = -FLT_MAX;
				if (list->is_indexed())
				{
					U16 * idx = list->indices + i;
					for (int v = 0; v < cnt; v++, idx++)
					{
						D3DLVERTEX * vert = list->verts + *idx;

					// Need z in camera space.
						float vz = Mx * vert->x + My * vert->y + Mz * vert->z + Tz;
						if (vz > min)
						{
							min = vz;
						}
					}
				}
				else
				{
					D3DLVERTEX * vert = list->verts + i;
					for (int v = 0; v < cnt; v++, vert++)
					{
						float vz = Mx * vert->x + My * vert->y + Mz * vert->z + Tz;
						if (vz > min)
						{
							min = vz;
						}
					}
				}

				poly_list[num_polys].min_z = min;
				num_polys++;
				i += cnt;
			}

			//delete list;
			node->list = NULL;

			node = node->next;
		}

	// Sort polys.
		Poly * p = &(poly_list[0]);
		qsort(p, num_polys, sizeof(Poly), ComparePolys);

	// Make z-buffer read-only.
		set_render_state(D3DRS_ZWRITEENABLE, TRUE);
		set_render_state(D3DRS_ZENABLE, TRUE);
		set_render_state(D3DRS_ALPHABLENDENABLE, TRUE);
		glBindTexture(GL_TEXTURE_2D,0);
		glBegin(GL_TRIANGLES);
		glEnd();

		//LPDIRECT3DTEXTURE2 txm = 0;
		unsigned int txm = 0;
		D3DBLEND src_func = D3DBLEND_FORCE_DWORD;
		D3DBLEND dst_func = D3DBLEND_FORCE_DWORD;
		D3DCMPFUNC depth_func = D3DCMP_FORCE_DWORD;

		p = &(poly_list[0]);

		D3DLVERTEX * v_ptr = lvertex_list;
		U16 * idx_ptr = index_list;

		const RPList * prev_list = p->list;

		for (int i = 0; i < num_polys; i++, p++)
		{
			if (p->list != prev_list)
			{
			//
			// flush previous list.
			//
				if (txm != prev_list->texture)
				{
					txm = prev_list->texture;
					glBindTexture(GL_TEXTURE_2D, txm);
					glBegin(GL_TRIANGLES);
					glEnd();
				}
				if (src_func != prev_list->src_blend_func)
				{
					src_func = prev_list->src_blend_func;
					set_render_state(D3DRS_SRCBLEND, src_func);
				}
				if (dst_func != prev_list->dst_blend_func)
				{
					dst_func = prev_list->dst_blend_func;
					set_render_state(D3DRS_DESTBLEND, dst_func);
				}
				if (depth_func != prev_list->depth_func)
				{
					depth_func = prev_list->depth_func;
					set_render_state(D3DRS_ZFUNC, depth_func);
				}

				DWORD flags = D3DDP_DONOTUPDATEEXTENTS;
				if (!prev_list->clip)
				{
					flags |= D3DDP_DONOTCLIP;
				}

			//
			// Set up matrices & viewport. We should find a way to see if they really need to change or not.
			//
				lpD3DDevice->SetTransform(D3DTRANSFORMSTATE_WORLD, const_cast<D3DMATRIX *>(&prev_list->modelview));
				lpD3DDevice->SetTransform(D3DTRANSFORMSTATE_PROJECTION, const_cast<D3DMATRIX *>(&prev_list->projection));

				if ((prev_list->x != last_x) || 
					(prev_list->y != last_y) || 
					(prev_list->w != last_w) || 
					(prev_list->h != last_h))
				{
					D3DVIEWPORT2 viewData;
					memset(&viewData, 0, sizeof(D3DVIEWPORT2));
					viewData.dwSize         =   sizeof(D3DVIEWPORT2);
					viewData.dwX            =   prev_list->x;
					viewData.dwY            =   prev_list->y;
					viewData.dwWidth        =   prev_list->w;
					viewData.dwHeight       =   prev_list->h;
					viewData.dvClipX        =   -1;
					viewData.dvClipY        =   1;
					viewData.dvClipWidth    =   2;
					viewData.dvClipHeight   =   2;
					viewData.dvMinZ         =   0.0f;
					viewData.dvMaxZ         =   1.0f;

					HRESULT result = lpD3DViewport->SetViewport2(&viewData);
					if (result != DD_OK)
					{
						DebugAlert(NULL, "RP: SetViewport2() failed: %s",DD_message(result));
					}

					last_x = prev_list->x;
					last_y = prev_list->y;
					last_w = prev_list->w;
					last_h = prev_list->h;
				}

				if (prev_list->is_indexed())
				{
					int num_indices = idx_ptr - index_list;
					if (num_indices)
					{
						HRESULT ddr = lpD3DDevice->DrawIndexedPrimitive(prev_list->type, D3DVT_LVERTEX, prev_list->verts, prev_list->num_verts, index_list, num_indices, flags);
						if (ddr != DD_OK)
						{
							DebugPrint("RenderPipeline: DrawIndexedPrimitive() failed: %s\n", DD_message(ddr));
						}
					}
				}
				else
				{
					int num_verts = v_ptr - lvertex_list;
					if (num_verts)
					{
						HRESULT ddr = lpD3DDevice->DrawPrimitive(prev_list->type, D3DVT_LVERTEX, lvertex_list, num_verts, flags);
						if (ddr != DD_OK)
						{
							DebugPrint("RenderPipeline: DrawPrimitive() failed: %s\n", DD_message(ddr));
						}
					}
				}

				prev_list = p->list;

				idx_ptr = index_list;
				v_ptr = lvertex_list;
			}

		// build up new polys.

			int cnt;
			switch (p->list->type)
			{
				case D3DPT_POINTLIST:
					cnt = 1;
					break;
				case D3DPT_LINELIST:
					cnt = 2;
					break;
				case D3DPT_TRIANGLELIST:
					cnt = 3;
					break;
			}

			if (p->list->is_indexed())
			{
			//
			// ONE POLY AT A TIME HERE.
			//
			// Build up vertex/index lists.

				U16 * isrc = p->list->indices + p->index;
				for (int i = 0; i < cnt; i++, idx_ptr++, isrc++)
				{
					*idx_ptr = *isrc;
				}
			}
			else
			{
				D3DLVERTEX * vsrc = p->list->verts + p->index;
				for (int i = 0; i < cnt; i++, vsrc++, v_ptr++)
				{
					*v_ptr = *vsrc;
				}
			}
		}

	//
	// Flush last block.
	// 
		if (txm != prev_list->texture)
		{
			txm = prev_list->texture;
			glBindTexture(GL_TEXTURE_2D, txm);
			glBegin(GL_TRIANGLES);
			glEnd();
		}
		if (src_func != prev_list->src_blend_func)
		{
			src_func = prev_list->src_blend_func;
			set_render_state(D3DRS_SRCBLEND, src_func);
		}
		if (dst_func != prev_list->dst_blend_func)
		{
			dst_func = prev_list->dst_blend_func;
			set_render_state(D3DRS_DESTBLEND, dst_func);
		}
		if (depth_func != prev_list->depth_func)
		{
			depth_func = prev_list->depth_func;
			set_render_state(D3DRS_ZFUNC, depth_func);
		}

		DWORD flags = D3DDP_DONOTUPDATEEXTENTS;
		if (!prev_list->clip)
		{
			flags |= D3DDP_DONOTCLIP;
		}

	//
	// Set up matrices & viewport. We should find a way to see if they really need to change or not.
	//
		lpD3DDevice->SetTransform(D3DTRANSFORMSTATE_WORLD, const_cast<D3DMATRIX *>(&prev_list->modelview));
		lpD3DDevice->SetTransform(D3DTRANSFORMSTATE_PROJECTION, const_cast<D3DMATRIX *>(&prev_list->projection));

		if ((prev_list->x != last_x) || 
			(prev_list->y != last_y) || 
			(prev_list->w != last_w) || 
			(prev_list->h != last_h))
		{
			D3DVIEWPORT2 viewData;
			memset(&viewData, 0, sizeof(D3DVIEWPORT2));
			viewData.dwSize         =   sizeof(D3DVIEWPORT2);
			viewData.dwX            =   prev_list->x;
			viewData.dwY            =   prev_list->y;
			viewData.dwWidth        =   prev_list->w;
			viewData.dwHeight       =   prev_list->h;
			viewData.dvClipX        =   -1;
			viewData.dvClipY        =   1;
			viewData.dvClipWidth    =   2;
			viewData.dvClipHeight   =   2;
			viewData.dvMinZ         =   0.0f;
			viewData.dvMaxZ         =   1.0f;

			HRESULT result = lpD3DViewport->SetViewport2(&viewData);
			if (result != DD_OK)
			{
				DebugAlert(NULL, "RP: SetViewport2() failed: %s",DD_message(result));
			}

			last_x = prev_list->x;
			last_y = prev_list->y;
			last_w = prev_list->w;
			last_h = prev_list->h;
		}


		if (prev_list->is_indexed())
		{
			int num_indices = idx_ptr - index_list;
			if (num_indices)
			{
				HRESULT ddr = lpD3DDevice->DrawIndexedPrimitive(prev_list->type, D3DVT_LVERTEX, prev_list->verts, prev_list->num_verts, index_list, num_indices, flags);
				if (ddr != DD_OK)
				{
					DebugPrint("RenderPipeline: DrawIndexedPrimitive() failed: %s\n", DD_message(ddr));
				}
			}
		}
		else
		{
			int num_verts = v_ptr - lvertex_list;
			if (num_verts)
			{
				HRESULT ddr = lpD3DDevice->DrawPrimitive(prev_list->type, D3DVT_LVERTEX, lvertex_list, num_verts, flags);
				if (ddr != DD_OK)
				{
					DebugPrint("RenderPipeline: DrawPrimitive() failed: %s\n", DD_message(ddr));
				}
			}
		}

	// Turn z-buffer writes back on.
		set_render_state(D3DRS_ZWRITEENABLE, TRUE);
	}
}
#else
void RenderPipeline::render_alpha_lists_depth_sorted(void)
{
	ListNode * node = alpha_lists.used_entries;
	if (node)
	{
		DynamicArray<Poly> poly_list(256);

	// Collect polygons for sorting.
		int num_polys = 0;
		while (node)
		{
			const RPList * list = node->list;
			int count = (list->is_indexed()) ? list->num_indices : list->num_verts;
			for (int i = 0; i < count; )
			{
				int cnt;
				switch (list->type)
				{
					case D3DPT_POINTLIST:
						cnt = 1;
						break;
					case D3DPT_LINELIST:
						cnt = 2;
						break;
					case D3DPT_TRIANGLELIST:
						cnt = 3;
						break;
				}

				poly_list[num_polys].list = list;
				poly_list[num_polys].index = i;

				float min = -FLT_MAX;
				if (list->is_indexed())
				{
					U16 * idx = list->indices + i;
					for (int v = 0; v < cnt; v++, idx++)
					{
						RPListVertex1 * vert = list->verts + *idx;
						if (vert->z > min)
						{
							min = vert->z;
						}
					}
				}
				else
				{
					RPListVertex1 * vert = list->verts + i;
					for (int v = 0; v < cnt; v++, vert++)
					{
						if (vert->z > min)
						{
							min = vert->z;
						}
					}
				}

				poly_list[num_polys].min_z = min;
				num_polys++;
				i += cnt;
			}

			//delete list;
			node->list = NULL;

			node = node->next;
		}

	// Sort polys.
		Poly * p = &(poly_list[0]);
		qsort(p, num_polys, sizeof(Poly), ComparePolys);

	// Make z-buffer read-only.
		set_render_state(D3DRS_ZWRITEENABLE, FALSE);
		set_render_state(D3DRS_ZENABLE, TRUE);
		set_render_state(D3DRS_ALPHABLENDENABLE, TRUE);
		glBindTexture(GL_TEXTURE_2D,0);
		glBegin(GL_TRIANGLES);
		glEnd();

		//LPDIRECT3DTEXTURE2 txm = 0;
		unsigned int txm = 0;
		D3DPRIMITIVETYPE type = D3DPT_FORCE_DWORD;
		D3DBLEND src_func = D3DBLEND_FORCE_DWORD;
		D3DBLEND dst_func = D3DBLEND_FORCE_DWORD;
		D3DCMPFUNC depth_func = D3DCMP_FORCE_DWORD;

		enum
		{
			SC_TXM		= 0x01,
			SC_TYPE		= 0x02,
			SC_SRC		= 0x04,
			SC_DST		= 0x08,
			SC_DEPTH	= 0x10
		};

		RPVERTEX * v_ptr = vertex_list;
		U16 * idx_ptr = index_list;

		enum
		{
			BL_NORMAL = 1,
			BL_INDEXED
		};

		DWORD flags = D3DDP_DONOTCLIP | D3DDP_DONOTUPDATEEXTENTS;
		unsigned int inside_block = 0;

		p = &(poly_list[0]);
		for (int i = 0; i < num_polys; i++, p++)
		{
		// See if state change is required...
			unsigned int state_change = 0;
			if (p->list->texture != txm)
			{
				txm = p->list->texture;
				state_change |= SC_TXM;
			}
			if (p->list->type != type)
			{
				type = p->list->type;
				state_change |= SC_TYPE;
			}
			if (p->list->src_blend_func != src_func)
			{
				src_func = p->list->src_blend_func;
				state_change |= SC_SRC;
			}
			if (p->list->dst_blend_func != dst_func)
			{
				dst_func = p->list->dst_blend_func;
				state_change |= SC_DST;
			}
			if (p->list->depth_func != depth_func)
			{
				depth_func = p->list->depth_func;
				state_change |= SC_DEPTH;
			}

			if (state_change)
			{
			// Flush previous block.
				if (inside_block == BL_NORMAL)
				{
					DWORD num_verts = v_ptr - vertex_list;
					HRESULT ddrval = lpD3DDevice->DrawPrimitive(type, D3DVT_TLVERTEX, vertex_list, num_verts, flags);
					inside_block = 0;
				}
				else if (inside_block == BL_INDEXED)
				{
					DWORD num_verts = v_ptr - vertex_list;
					DWORD num_indices = idx_ptr - index_list;

					if (num_verts && num_indices)
					{
						HRESULT dd = lpD3DDevice->DrawIndexedPrimitive(type, D3DVT_TLVERTEX, vertex_list, num_verts, index_list, num_indices, flags);
						if (dd != DD_OK)
						{
							DebugPrint("RP: DrawIndexedPrimitive() failed: %s\n", DD_message(dd));
						}
					}
					inside_block = 0;
				}

				flags = D3DDP_DONOTCLIP | D3DDP_DONOTUPDATEEXTENTS;

				if (state_change & SC_TXM)
				{
					if (txm != 0)
					{
					/*
						unsigned long h;
						txm->GetHandle(lpD3DDevice, &h);
						set_render_state(D3DRS_TEXTUREHANDLE, h);
					*/
						glBindTexture(GL_TEXTURE_2D, txm);
						glBegin(GL_TRIANGLES);
						glEnd();
					}
					else
					{
						//set_render_state(D3DRS_TEXTUREHANDLE, 0);
						glBindTexture(GL_TEXTURE_2D,0);
						glBegin(GL_TRIANGLES);
						glEnd();

					}
				}
				if (state_change & (SC_SRC | SC_DST))
				{
					set_render_state(D3DRS_SRCBLEND, src_func);
					set_render_state(D3DRS_DESTBLEND, dst_func);
				}
				if (state_change & SC_DEPTH)
				{
					set_render_state(D3DRS_ZFUNC, depth_func);
				}

			//
			// Start new block.
			//
				if (p->list->is_indexed())
				{
					inside_block = BL_INDEXED;
				}
				else
				{
					inside_block = BL_NORMAL;
				}
			}											  

		/*
			if (p->list->clip)
			{
				flags &= ~D3DDP_DONOTCLIP;
			}
		*/

			int cnt;
			switch (type)
			{
				case D3DPT_POINTLIST:
					cnt = 1;
					break;
				case D3DPT_LINELIST:
					cnt = 2;
					break;
				case D3DPT_TRIANGLELIST:
					cnt = 3;
					break;
			}

		//
		// Submit poly to GL.
		//
			if (p->list->is_indexed())
			{
			//
			// ONE POLY AT A TIME HERE.
			//
			// Build up vertex/index lists.
				int offset = v_ptr - vertex_list;

				if (p->list->clip)
				{
					U16 * isrc = p->list->indices + p->index;
					U16 * ilist = isrc;

					int any_clipped = p->list->verts[ilist[0]].clip;
					int all_clipped = any_clipped;
					for (int j = 1; j < cnt; j++)
					{
						int clip = p->list->verts[ilist[j]].clip;
						any_clipped |= clip;
						all_clipped &= clip;
					}

					if (!any_clipped)
					{
						for (int v = 0; v < cnt; )
						{
							RPListVertex1 * vsrc = p->list->verts + ilist[v];

							float w = 1.0 / vsrc->w;

						// do as much as possible while waiting for divide.
							v_ptr->dwDiffuseRGBA = (vsrc->a << 24) | (vsrc->r << 16) | (vsrc->g << 8) | vsrc->b;
							v_ptr->tu1 = vsrc->u;
							v_ptr->tv1 = vsrc->v;

							*(idx_ptr++) = v_ptr - vertex_list;
							j++;
							RPListVertex1 * vs = vsrc++;
							RPVERTEX * vd = v_ptr++;
							v++;

							assert((v_ptr - vertex_list) < DefaultListLength);

							vd->x = vs->x * w * h_scale + h_offset;
							vd->y = vs->y * w * v_scale + v_offset;
							vd->z = 1.0 - w;
							vd->rhw = w;
						}
					}
					else if (!all_clipped)
					{
					//
					// Clip poly, which may generate new vertices.
					//
						RPListVertex1 poly[16];
						int index[16];

						for (int v = 0; v < cnt; v++)
						{
							poly[v] = p->list->verts[ilist[v]];
						}

						int num_verts = clip_poly(poly, index, cnt, any_clipped);

					//
					// Add new vertices to end of vlist using existing v_ptr.
					//
						int index_remap[16];

						for (v = 0; v < num_verts; )
						{
							RPListVertex1 * vsrc = poly + index[v];

							float w = 1.0 / vsrc->w;

						// do as much as possible while waiting for divide.
							v_ptr->dwDiffuseRGBA = (vsrc->a << 24) | (vsrc->r << 16) | (vsrc->g << 8) | vsrc->b;
							v_ptr->tu1 = vsrc->u;
							v_ptr->tv1 = vsrc->v;

							index_remap[v] = v_ptr - vertex_list;
							j++;
							RPListVertex1 * vs = vsrc++;
							RPVERTEX * vd = v_ptr++;
							v++;

							assert((v_ptr - vertex_list) < DefaultListLength);

							vd->x = vs->x * w * h_scale + h_offset;
							vd->y = vs->y * w * v_scale + v_offset;
							vd->z = 1.0 - w;
							vd->rhw = w;
						}

					// triangulate.
						for (j = 2; j < num_verts; j++)
						{
							*(idx_ptr++) = index_remap[0];
							*(idx_ptr++) = index_remap[j-1];
							*(idx_ptr++) = index_remap[j];

							assert((idx_ptr - index_list) < DefaultListLength);
						}
					}
				}
				else
				{
					U16 * isrc = p->list->indices + p->index;
					for (int i = 0; i < cnt; )
					{
						RPListVertex1 * vsrc = p->list->verts + *isrc;
						float w = 1.0 / vsrc->w;

						v_ptr->dwDiffuseRGBA = (vsrc->a << 24) | (vsrc->r << 16) | (vsrc->g << 8) | vsrc->b;
						v_ptr->tu1 = vsrc->u;
						v_ptr->tv1 = vsrc->v;

						*(idx_ptr++) = v_ptr - vertex_list;
						RPVERTEX * vd = v_ptr++;
						i++;
						isrc++;

						vd->x = vsrc->x * w * h_scale + h_offset;
						vd->y = vsrc->y * w * v_scale + v_offset;
						vd->z = 1.0 - w;
						vd->rhw = w;
					}
				}
			}
			else
			{
				if (p->list->clip)
				{
					RPListVertex1 * vsrc = p->list->verts + p->index;

					int any_clipped = vsrc->clip;
					int all_clipped = any_clipped;
					for (int j = 1; j < cnt; j++)
					{
						int clip = vsrc[j].clip;
						any_clipped |= clip;
						all_clipped &= clip;
					}

					if (!any_clipped)
					{
						for (int i = 0; i < cnt; i++, vsrc++, v_ptr++)
						{
							float w = 1.0 / vsrc->w;

							v_ptr->dwDiffuseRGBA = (vsrc->a << 24) | (vsrc->r << 16) | (vsrc->g << 8) | vsrc->b;
							v_ptr->tu1 = vsrc->u;
							v_ptr->tv1 = vsrc->v;

							v_ptr->x = vsrc->x * w * h_scale + h_offset;
							v_ptr->y = vsrc->y * w * v_scale + v_offset;
							v_ptr->z = 1.0 - w;
							v_ptr->rhw = w;
						}
					}
					else if (!all_clipped)
					{
						RPListVertex1 poly[16];
						RPVERTEX clipped[16];
						int index[16];

						memcpy(poly, vsrc, sizeof(RPListVertex1) * cnt);
						int num_verts = clip_poly(poly, index, cnt, any_clipped);
						int * idx = index;

						RPVERTEX * c_ptr = clipped;
						for (j = 0; j < num_verts; )
						{
							RPListVertex1 * src = poly + *idx;
							float w = 1.0 / src->w;

						// do as much as possible while waiting for divide.
							c_ptr->dwDiffuseRGBA = (src->a << 24) | (src->r << 16) | (src->g << 8) | src->b;
							c_ptr->tu1 = src->u;
							c_ptr->tv1 = src->v;

							j++, idx++;
							RPListVertex1 * vs = src++;
							RPVERTEX * cd = c_ptr++;

							cd->x = vs->x * w * h_scale + h_offset;
							cd->y = vs->y * w * v_scale + v_offset;
							cd->z = 1.0 - w;
							cd->rhw = w;
						}

						for (j = 2; j < num_verts; j++)
						{
							*(v_ptr++) = clipped[0];
							*(v_ptr++) = clipped[j-1];
							*(v_ptr++) = clipped[j];
						}
					}
				}
				else
				{
					RPListVertex1 * vsrc = p->list->verts + p->index;
					for (int i = 0; i < cnt; i++, vsrc++, v_ptr++)
					{
						float w = 1.0 / vsrc->w;

						v_ptr->dwDiffuseRGBA = (vsrc->a << 24) | (vsrc->r << 16) | (vsrc->g << 8) | vsrc->b;
						v_ptr->tu1 = vsrc->u;
						v_ptr->tv1 = vsrc->v;

						v_ptr->x = vsrc->x * w * h_scale + h_offset;
						v_ptr->y = vsrc->y * w * v_scale + v_offset;
						v_ptr->z = 1.0 - w;
						v_ptr->rhw = w;
					}
				}
			}
		}

	//
	// Flush last block.
	// 
		if (inside_block == BL_NORMAL)
		{
			DWORD num_verts = v_ptr - vertex_list;
			HRESULT ddrval = lpD3DDevice->DrawPrimitive(type, D3DVT_TLVERTEX, vertex_list, num_verts, /*D3DDP_DONOTCLIP |*/ D3DDP_DONOTUPDATEEXTENTS);
		}
		else if (inside_block == BL_INDEXED)
		{
			DWORD num_verts = v_ptr - vertex_list;
			DWORD num_indices = idx_ptr - index_list;

			if (num_verts && num_indices)
			{
				HRESULT dd = lpD3DDevice->DrawIndexedPrimitive(type, D3DVT_TLVERTEX, vertex_list, num_verts, index_list, num_indices, /*D3DDP_DONOTCLIP |*/ D3DDP_DONOTUPDATEEXTENTS);
				if (dd != DD_OK)
				{
					DebugPrint("RP: DrawIndexedPrimitive() failed: %s\n", DD_message(dd));
				}
			}
			inside_block = 0;
		}

	// Turn z-buffer writes back on.
		set_render_state(D3DRS_ZWRITEENABLE, TRUE);
	}
}
#endif
//


#ifdef DX_TRANSFORMS
void RenderPipeline::render_alpha_lists_unsorted(void)
{
}
#else
void RenderPipeline::render_alpha_lists_unsorted(void)
{
	ListNode * node = alpha_lists.used_entries;
	if (node)
	{
		enum
		{
			SC_TXM		= 0x01,
			SC_TYPE		= 0x02,
			SC_SRC		= 0x04,
			SC_DST		= 0x08,
			SC_DEPTH	= 0x10
		};

		set_render_state(D3DRS_ZWRITEENABLE, FALSE);
		set_render_state(D3DRS_ZENABLE, TRUE);
		set_render_state(D3DRS_ALPHABLENDENABLE, TRUE);
		glBindTexture(GL_TEXTURE_2D,0);
		glBegin(GL_TRIANGLES);
		glEnd();

		RPVERTEX * v_ptr = vertex_list;
		U16 * idx_ptr = index_list;
	//
	// GOAL: coalesce adjacent lists that have same GL states.
	// Minimize state changes and glBegin()/glEnd() pairs.
	//
		enum
		{
			BL_NORMAL = 1,
			BL_INDEXED
		};

		unsigned int inside_block = 0;
		//LPDIRECT3DTEXTURE2 txm = 0;
		unsigned int txm = 0;
		D3DPRIMITIVETYPE type = D3DPT_FORCE_DWORD;
		D3DCMPFUNC depth_func = D3DCMP_FORCE_DWORD;
		D3DBLEND src_func = D3DBLEND_FORCE_DWORD;
		D3DBLEND dst_func = D3DBLEND_FORCE_DWORD;

		DWORD flags = D3DDP_DONOTCLIP | D3DDP_DONOTUPDATEEXTENTS;

		while (node)
		{
		// 
		// Get next list.
		//
			const RPList * list = node->list;
			unsigned int state_change = 0;

			int cnt;
			switch (list->type)
			{
				case D3DPT_POINTLIST:
					cnt = 1;
					break;
				case D3DPT_LINELIST:
					cnt = 2;
					break;
				case D3DPT_TRIANGLELIST:
					cnt = 3;
					break;
				default:
					cnt = 1;	// bogus, but prevents div by zero.
					break;
			}


		/*
			if (list->clip)
			{
				flags &= ~D3DDP_DONOTCLIP;
			}
		*/

		//
		// Check for state change.
		//
			if (list->texture != txm)
			{
				txm = list->texture;
				state_change |= SC_TXM;
			}
			if (list->type != type)
			{
				type = list->type;
				state_change |= SC_TYPE;
			}
			if (list->depth_func != depth_func)
			{
				depth_func = list->depth_func;
				state_change |= SC_DEPTH;
			}
			if (list->src_blend_func != src_func)
			{
				src_func = list->src_blend_func;
				state_change |= SC_SRC;
			}
			if (list->dst_blend_func != dst_func)
			{
				dst_func = list->dst_blend_func;
				state_change |= SC_DST;
			}

		//
		// Do we need to flush? Flush conditions:
		//
		// 1. state change.
		// 2. list length overrun.
		// 3. indexed vs. non-indexed blocks.
		//
			unsigned int vertex_limit = (v_ptr - vertex_list) + list->num_verts;
			U16 index_limit = (idx_ptr - index_list) + list->num_indices;
			bool overrun = (vertex_limit >= DefaultListLength) || (index_limit >= DefaultListLength);

			bool index_change;
			if (list->is_indexed())
			{
				if (inside_block == BL_NORMAL)
				{
					index_change = true;
				}
				else
				{
					index_change = false;
				}
			}
			else if (inside_block == BL_INDEXED)
			{
				index_change = true;
			}
			else 
			{
				index_change = false;
			}

			bool flush = state_change || overrun || index_change;

			if (flush)
			{
			//
			// Flush previous list.
			//
				if (inside_block == BL_NORMAL)
				{
					DWORD num_verts = v_ptr - vertex_list;
					HRESULT ddrval = lpD3DDevice->DrawPrimitive(type, D3DVT_TLVERTEX, vertex_list, num_verts, flags);

					if (ddrval != DD_OK)
					{
						DebugPrint("RP: DrawPrimitive() failed: %s\n", DD_message(ddrval));
					}

					inside_block = 0;
				}
				else if (inside_block == BL_INDEXED)
				{
					DWORD num_verts = v_ptr - vertex_list;
					DWORD num_indices = idx_ptr - index_list;

					if (num_verts && num_indices)
					{
						HRESULT dd = lpD3DDevice->DrawIndexedPrimitive(type, D3DVT_TLVERTEX, vertex_list, num_verts, index_list, num_indices, flags);
						if (dd != DD_OK)
						{
							DebugPrint("RP: DrawIndexedPrimitive() failed: %s\n", DD_message(dd));
						}
					}

					inside_block = 0;
				}

				flags = D3DDP_DONOTCLIP | D3DDP_DONOTUPDATEEXTENTS;

			}

			if (state_change)
			{
			//
			// Register state change.
			//
				if (state_change & SC_TXM)
				{
					if (txm != 0)
					{
					/*
						DWORD h;
						txm->GetHandle(lpD3DDevice, &h);
						set_render_state(D3DRS_TEXTUREHANDLE, h);
					*/
						glBindTexture(GL_TEXTURE_2D, txm);
						glBegin(GL_TRIANGLES);
						glEnd();
					}
					else
					{
						//set_render_state(D3DRS_TEXTUREHANDLE, 0);
						glBindTexture(GL_TEXTURE_2D,0);
						glBegin(GL_TRIANGLES);
						glEnd();
					}
				}
				if (state_change & SC_DEPTH)
				{
					set_render_state(D3DRS_ZFUNC, depth_func);
				}
				if (state_change & (SC_SRC | SC_DST))
				{
					set_render_state(D3DRS_SRCBLEND, src_func);
					set_render_state(D3DRS_DESTBLEND, dst_func);
				}
				if (state_change & SC_DEPTH)
				{
					set_render_state(D3DRS_ZFUNC, depth_func);
				}
			}

			if (flush)
			{
			//
			// Begin new GL block.
			//
				if (list->is_indexed())
				{
					inside_block = BL_INDEXED;
				}
				else
				{
					inside_block = BL_NORMAL;
				}
			}

		//
		// Submit list's polys to D3D.
		//
			if (list->is_indexed())
			{
				if (list->clip)
				{
					clip_indexed_list(list, v_ptr, idx_ptr, cnt);
				}
				else
				{
					do_unclipped_indexed_list(list, v_ptr, idx_ptr);
				}
			}
			else
			{
				if (list->clip)
				{
					clip_list(list, v_ptr, cnt);
				}
				else
				{
					do_unclipped_list(list, v_ptr);
				}
			}

			node->list = NULL;
			node = node->next;
		}

	//
	// Flush last block.
	// 
		if (inside_block == BL_NORMAL)
		{
			DWORD num_verts = v_ptr - vertex_list;
			HRESULT ddrval = lpD3DDevice->DrawPrimitive(type, D3DVT_TLVERTEX, vertex_list, num_verts, flags);

			if (ddrval != DD_OK)
			{
				DebugPrint("RP: DrawPrimitive() failed: %s\n", DD_message(ddrval));
			}
		}
		else if (inside_block == BL_INDEXED)
		{
			DWORD num_verts = v_ptr - vertex_list;
			DWORD num_indices = idx_ptr - index_list;

			if (num_verts && num_indices)
			{
				HRESULT dd = lpD3DDevice->DrawIndexedPrimitive(type, D3DVT_TLVERTEX, vertex_list, num_verts, index_list, num_indices, flags);
				if (dd != DD_OK)
				{
					DebugPrint("RP: DrawIndexedPrimitive() failed: %s\n", DD_message(dd));
				}
			}
		}

		set_render_state(D3DRS_ZWRITEENABLE, TRUE);
	}
}
#endif

//

int RenderPipeline::flush_opaque(void)
{
	int result = num_opaque_polys;

	render_opaque_lists();

	opaque_index = 0;
	num_opaque_polys = 0;
	opaque_lists.reset();

	return result;
}

//

int RenderPipeline::flush_translucent(bool depth_sort)
{
	int result = num_alpha_polys;

	if (depth_sort)
	{
		render_alpha_lists_depth_sorted();
	}
	else
	{
		render_alpha_lists_unsorted();
	}

	alpha_index = 0;
	num_alpha_polys = 0;
	alpha_lists.reset();

	return result;
}

//

int COMAPI RenderPipeline::flush(DWORD flags)
{
	int total_polys = 0;

	if (flags & RP_OPAQUE)
	{
		total_polys += flush_opaque();
	}
	if (flags & RP_TRANSLUCENT_UNSORTED)
	{
		total_polys += flush_translucent(false);
	}
	else if (flags & RP_TRANSLUCENT_DEPTH_SORTED)
	{
		total_polys += flush_translucent(true);
	}

	return total_polys;
}

//


//
// WHAT IF RENDERPIPELINE DOESN'T OWN THE SURFACES?
//
HRESULT RenderPipeline::restore_surface(LPDIRECTDRAWSURFACE3 surface)
{
	HRESULT result = DD_OK;

	DebugPrint("restore_surface()\n");

	DDSCAPS caps;
	LPDIRECTDRAWSURFACE3 front = lpDDSPrimary;
	front->GetCaps(&caps);

	// Avoid if (result == DDERR_IMPLICITLYCREATED)
	if (caps.dwCaps & DDSCAPS_FLIP)
		result = front->Restore();	// restore attached surfaces through primary
	else
		result = surface->Restore();

	if (result == DDERR_WRONGMODE)
	{
		DebugPrint("GL: RECREATE SURFACES\n");
		destroy_surfaces();
		create_surfaces();
	}

	if (result != DD_OK)
	{
		DebugPrint("Restore() failed.\n=>%s\n",DD_message(result));
		return result;
	}

	return result;
}

//

void COMAPI RenderPipeline::clear_buffers(void)
{
	DDBLTFX ddbltfx;
	ddbltfx.dwSize		= sizeof(ddbltfx);
	ddbltfx.dwFillColor	= screen_pixel_format.compute(128, 128, 128);;

	if (lpDDSBack->IsLost() == DDERR_SURFACELOST)
	{
		if (restore_surface(lpDDSBack) != DD_OK)
			return;
	}

	HRESULT result = lpDDSBack->Blt(NULL, NULL, NULL, DDBLT_COLORFILL|DDBLT_WAIT, &ddbltfx);
	if (result != DD_OK)
	{
		DebugPrint("clear_color Blt() failed.\n=>%s\n",DD_message(result));
	}

	if (lpZBuffer->IsLost() == DDERR_SURFACELOST)
	{
		// EXPLICIT SURFACE MUST BE RESTORED
		result = lpZBuffer->Restore();
		if (result != DD_OK)
		{
			DebugPrint("Can't restore z-buffer.\n=>%s\n",DD_message(result));
			return;
		}
	}

	ddbltfx.dwSize = sizeof(ddbltfx);
	// Note: 0.0 (znear) to 1.0 (far)
	ddbltfx.dwFillDepth = 0xffffffff;

	result = lpZBuffer->Blt(NULL, NULL, NULL, DDBLT_DEPTHFILL | DDBLT_DDFX | DDBLT_WAIT, &ddbltfx);
	if (result != DD_OK)
	{
		DebugPrint("Can't clear z-buffer.\n=>%s\n",DD_message(result));
	}
}

//

RECT client_screen_rect (HWND hWnd)
{
   RECT rect;
   POINT ul,lr;

   GetClientRect(hWnd, &rect);

   ul.x = rect.left; 
   ul.y = rect.top; 
   lr.x = rect.right; 
   lr.y = rect.bottom; 

   ClientToScreen(hWnd, &ul); 
   ClientToScreen(hWnd, &lr); 

   SetRect(&rect, ul.x, ul.y, lr.x-1, lr.y-1); 

   return rect;
}

//

void COMAPI RenderPipeline::swap_buffers(void)
{
	HRESULT result;

	LPDIRECTDRAWSURFACE3 front = lpDDSPrimary;

	if (front->IsLost() == DDERR_SURFACELOST)
	{
		if (restore_surface(front) != DD_OK)
			return;
		front = lpDDSPrimary;	// did pointer change?
	}

	LPDIRECTDRAWSURFACE3 back = lpDDSBack;

	DDSCAPS caps;
	front->GetCaps(&caps);

	if (caps.dwCaps & DDSCAPS_FLIP)
	{
	// FLIP BETWEEN FRONT AND BACK SURFACES
		result = front->Flip(NULL, DDFLIP_WAIT);
		if (result != DD_OK)
		{
			DebugPrint("RP: flip failed?\n=>%s\n",DD_message(result));
		}
	}
	else
	{
	// BACK SURFACE IS NOT ATTACHED SO IT MAY NEED TO BE RESTORED
		if (back->IsLost() == DDERR_SURFACELOST)
		{
			if (restore_surface(back) != DD_OK)
				return;
		}

	// BLIT BACK SURFACE TO FRONT
	// weak attempt to determine if in full screen mode
		if (GetWindowLong(hWnd, GWL_STYLE) & WS_POPUP)
		{
			result = front->Blt(NULL, back, NULL, DDBLT_WAIT, NULL);
		}
		else
		{
		// In windowed mode, use the Blt() function for proper
		// clipping (e.g., if the window is moved partially offscreen)
		// 
		// Destination rectangle must be explicitly specified -- note 
		// extra pixel/column in rectangle for Blt()'s benefit
			RECT dest_rect = client_screen_rect(hWnd);

			++dest_rect.right;
			++dest_rect.bottom;

			result = front->Blt(&dest_rect, back, NULL, DDBLT_WAIT, NULL);
		}

		if (result != DD_OK)
		{
			DebugPrint("RP: blit failed?\n=>%s\n",DD_message(result));
		}
	}
}

//
TxHandle COMAPI RenderPipeline::create_texture (int width, int height, const PixelFormat &format, int numMipLevels)
{
	return HTX_INVALID;
}

void COMAPI RenderPipeline::destroy_texture (TxHandle which)
{
}

BOOL32 COMAPI RenderPipeline::lock_texture (TxHandle which, int level, TxLock *lockData)
{
	return false;
}

void COMAPI RenderPipeline::unlock_texture (TxHandle which, int level)
{
}

void COMAPI RenderPipeline::set_texture_render_state(TxHandle which, D3DRENDERSTATETYPE state, DWORD value)
{
}

BOOL32 COMAPI RenderPipeline::get_texture_render_state(TxHandle which, D3DRENDERSTATETYPE state, DWORD *valuePtr)
{
	return false;
}

void COMAPI RenderPipeline::clear_texture_render_state (TxHandle which, D3DRENDERSTATETYPE state)
{
}

BOOL32 COMAPI RenderPipeline::set_texture_palette (TxHandle which, int start, int length, const TxRGB *colors)
{
	return false;
}

BOOL32 COMAPI RenderPipeline::get_texture_palette (TxHandle which, int start, int length, TxRGB *colors)
{
	return false;
}

BOOL32 COMAPI RenderPipeline::blit_texture (TxHandle hDest, RECT destRect, TxHandle hSrc, RECT srcRect)
{
	return false;
}

void COMAPI RenderPipeline::set_texture_global_state (TmGlobalState &newState, DWORD newValue)
{
}

BOOL32 COMAPI RenderPipeline::get_texture_device_info (TxDeviceInfo &info)
{
	return false;
}

BOOL32 COMAPI RenderPipeline::set_texture_level_data (TxHandle which, int level, int srcWidth, int srcHeight, int srcStride, const PixelFormat &srcFormat, void *srcPixels)
{
	return false;
}

void COMAPI RenderPipeline::activate_texture (TxHandle which, int stage)
{
}

//

char *DD_message (HRESULT error)
{
   static char buff[128];

   switch (error) 
      {
      case DD_OK:
          return "No error.\0";
      case DDERR_ALREADYINITIALIZED:
          return "This object is already initialized.\0";
      case DDERR_BLTFASTCANTCLIP:
          return "Return if a clipper object is attached to the source surface passed into a BltFast call.\0";
      case DDERR_CANNOTATTACHSURFACE:
          return "This surface can not be attached to the requested surface.\0";
      case DDERR_CANNOTDETACHSURFACE:
          return "This surface can not be detached from the requested surface.\0";
      case DDERR_CANTCREATEDC:
          return "Windows can not create any more DCs.\0";
      case DDERR_CANTDUPLICATE:
          return "Can't duplicate primary & 3D surfaces, or surfaces that are implicitly created.\0";
      case DDERR_CLIPPERISUSINGHWND:
          return "An attempt was made to set a cliplist for a clipper object that is already monitoring an hwnd.\0";
      case DDERR_COLORKEYNOTSET:
          return "No src color key specified for this operation.\0";
      case DDERR_CURRENTLYNOTAVAIL:
          return "Support is currently not available.\0";
      case DDERR_DIRECTDRAWALREADYCREATED:
          return "A DirectDraw object representing this driver has already been created for this process.\0";
      case DDERR_EXCEPTION:
          return "An exception was encountered while performing the requested operation.\0";
      case DDERR_EXCLUSIVEMODEALREADYSET:
          return "An attempt was made to set the cooperative level when it was already set to exclusive.\0";
      case DDERR_GENERIC:
          return "Generic failure.\0";
      case DDERR_HEIGHTALIGN:
          return "Height of rectangle provided is not a multiple of reqd alignment.\0";
      case DDERR_HWNDALREADYSET:
          return "The CooperativeLevel HWND has already been set. It can not be reset while the process has surfaces or palettes created.\0";
      case DDERR_HWNDSUBCLASSED:
          return "HWND used by DirectDraw CooperativeLevel has been subclassed, this prevents DirectDraw from restoring state.\0";
      case DDERR_IMPLICITLYCREATED:
          return "This surface can not be restored because it is an implicitly created surface.\0";
      case DDERR_INCOMPATIBLEPRIMARY:
          return "Unable to match primary surface creation request with existing primary surface.\0";
      case DDERR_INVALIDCAPS:
          return "One or more of the caps bits passed to the callback are incorrect.\0";
      case DDERR_INVALIDCLIPLIST:
          return "DirectDraw does not support the provided cliplist.\0";
      case DDERR_INVALIDDIRECTDRAWGUID:
          return "The GUID passed to DirectDrawCreate is not a valid DirectDraw driver identifier.\0";
      case DDERR_INVALIDMODE:
          return "DirectDraw does not support the requested mode.\0";
      case DDERR_INVALIDOBJECT:
          return "DirectDraw received a pointer that was an invalid DIRECTDRAW object.\0";
      case DDERR_INVALIDPARAMS:
          return "One or more of the parameters passed to the function are incorrect.\0";
      case DDERR_INVALIDPIXELFORMAT:
          return "The pixel format was invalid as specified.\0";
      case DDERR_INVALIDPOSITION:
          return "Returned when the position of the overlay on the destination is no longer legal for that destination.\0";
      case DDERR_INVALIDRECT:
          return "Rectangle provided was invalid.\0";
      case DDERR_LOCKEDSURFACES:
          return "Operation could not be carried out because one or more surfaces are locked.\0";
      case DDERR_NO3D:
          return "There is no 3D present.\0";
      case DDERR_NOALPHAHW:
          return "Operation could not be carried out because there is no alpha accleration hardware present or available.\0";
      case DDERR_NOBLTHW:
          return "No blitter hardware present.\0";
      case DDERR_NOCLIPLIST:
          return "No cliplist available.\0";
      case DDERR_NOCLIPPERATTACHED:
          return "No clipper object attached to surface object.\0";
      case DDERR_NOCOLORCONVHW:
          return "Operation could not be carried out because there is no color conversion hardware present or available.\0";
      case DDERR_NOCOLORKEY:
          return "Surface doesn't currently have a color key\0";
      case DDERR_NOCOLORKEYHW:
          return "Operation could not be carried out because there is no hardware support of the destination color key.\0";
      case DDERR_NOCOOPERATIVELEVELSET:
          return "Create function called without DirectDraw object method SetCooperativeLevel being called.\0";
      case DDERR_NODC:
          return "No DC was ever created for this surface.\0";
      case DDERR_NODDROPSHW:
          return "No DirectDraw ROP hardware.\0";
      case DDERR_NODIRECTDRAWHW:
          return "A hardware-only DirectDraw object creation was attempted but the driver did not support any hardware.\0";
      case DDERR_NOEMULATION:
          return "Software emulation not available.\0";
      case DDERR_NOEXCLUSIVEMODE:
          return "Operation requires the application to have exclusive mode but the application does not have exclusive mode.\0";
      case DDERR_NOFLIPHW:
          return "Flipping visible surfaces is not supported.\0";
      case DDERR_NOGDI:
          return "There is no GDI present.\0";
      case DDERR_NOHWND:
          return "Clipper notification requires an HWND or no HWND has previously been set as the CooperativeLevel HWND.\0";
	  case DDERR_NOMIPMAPHW:
		  return "Hardware doesn't support mipmapping.\0";
      case DDERR_NOMIRRORHW:
          return "Operation could not be carried out because there is no hardware present or available.\0";
      case DDERR_NOOVERLAYDEST:
          return "Returned when GetOverlayPosition is called on an overlay that UpdateOverlay has never been called on to establish a destination.\0";
      case DDERR_NOOVERLAYHW:
          return "Operation could not be carried out because there is no overlay hardware present or available.\0";
      case DDERR_NOPALETTEATTACHED:
          return "No palette object attached to this surface.\0";
      case DDERR_NOPALETTEHW:
          return "No hardware support for 16 or 256 color palettes.\0";
      case DDERR_NORASTEROPHW:
          return "Operation could not be carried out because there is no appropriate raster op hardware present or available.\0";
      case DDERR_NOROTATIONHW:
          return "Operation could not be carried out because there is no rotation hardware present or available.\0";
      case DDERR_NOSTRETCHHW:
          return "Operation could not be carried out because there is no hardware support for stretching.\0";
      case DDERR_NOT4BITCOLOR:
          return "DirectDrawSurface is not in 4 bit color palette and the requested operation requires 4 bit color palette.\0";
      case DDERR_NOT4BITCOLORINDEX:
          return "DirectDrawSurface is not in 4 bit color index palette and the requested operation requires 4 bit color index palette.\0";
      case DDERR_NOT8BITCOLOR:
          return "DirectDrawSurface is not in 8 bit color mode and the requested operation requires 8 bit color.\0";
      case DDERR_NOTAOVERLAYSURFACE:
          return "Returned when an overlay member is called for a non-overlay surface.\0";
      case DDERR_NOTEXTUREHW:
          return "Operation could not be carried out because there is no texture mapping hardware present or available.\0";
      case DDERR_NOTFLIPPABLE:
          return "An attempt has been made to flip a surface that is not flippable.\0";
      case DDERR_NOTFOUND:
          return "Requested item was not found.\0";
      case DDERR_NOTLOCKED:
          return "Surface was not locked.  An attempt to unlock a surface that was not locked at all, or by this process, has been attempted.\0";
      case DDERR_NOTPALETTIZED:
          return "The surface being used is not a palette-based surface.\0";
      case DDERR_NOVSYNCHW:
          return "Operation could not be carried out because there is no hardware support for vertical blank synchronized operations.\0";
      case DDERR_NOZBUFFERHW:
          return "Operation could not be carried out because there is no hardware support for zbuffer blitting.\0";
      case DDERR_NOZOVERLAYHW:
          return "Overlay surfaces could not be z layered based on their BltOrder because the hardware does not support z layering of overlays.\0";
      case DDERR_OUTOFCAPS:
          return "The hardware needed for the requested operation has already been allocated.\0";
      case DDERR_OUTOFMEMORY:
          return "DirectDraw does not have enough memory to perform the operation.\0";
      case DDERR_OUTOFVIDEOMEMORY:
          return "DirectDraw does not have enough memory to perform the operation.\0";
      case DDERR_OVERLAYCANTCLIP:
          return "The hardware does not support clipped overlays.\0";
      case DDERR_OVERLAYCOLORKEYONLYONEACTIVE:
          return "Can only have ony color key active at one time for overlays.\0";
      case DDERR_OVERLAYNOTVISIBLE:
          return "Returned when GetOverlayPosition is called on a hidden overlay.\0";
      case DDERR_PALETTEBUSY:
          return "Access to this palette is being refused because the palette is already locked by another thread.\0";
      case DDERR_PRIMARYSURFACEALREADYEXISTS:
          return "This process already has created a primary surface.\0";
      case DDERR_REGIONTOOSMALL:
          return "Region passed to Clipper::GetClipList is too small.\0";
      case DDERR_SURFACEALREADYATTACHED:
          return "This surface is already attached to the surface it is being attached to.\0";
      case DDERR_SURFACEALREADYDEPENDENT:
          return "This surface is already a dependency of the surface it is being made a dependency of.\0";
      case DDERR_SURFACEBUSY:
          return "Access to this surface is being refused because the surface is already locked by another thread.\0";
      case DDERR_SURFACEISOBSCURED:
          return "Access to surface refused because the surface is obscured.\0";
      case DDERR_SURFACELOST:
          return "Access to this surface is being refused because the surface memory is gone. The DirectDrawSurface object representing this surface should have Restore called on it.\0";
      case DDERR_SURFACENOTATTACHED:
          return "The requested surface is not attached.\0";
      case DDERR_TOOBIGHEIGHT:
          return "Height requested by DirectDraw is too large.\0";
      case DDERR_TOOBIGSIZE:
          return "Size requested by DirectDraw is too large, but the individual height and width are OK.\0";
      case DDERR_TOOBIGWIDTH:
          return "Width requested by DirectDraw is too large.\0";
      case DDERR_UNSUPPORTED:
          return "Action not supported.\0";
      case DDERR_UNSUPPORTEDFORMAT:
          return "FOURCC format requested is unsupported by DirectDraw.\0";
      case DDERR_UNSUPPORTEDMASK:
          return "Bitmask in the pixel format requested is unsupported by DirectDraw.\0";
      case DDERR_VERTICALBLANKINPROGRESS:
          return "Vertical blank is in progress.\0";
      case DDERR_WASSTILLDRAWING:
          return "Informs DirectDraw that the previous Blt which is transfering information to or from this Surface is incomplete.\0";
      case DDERR_WRONGMODE:
          return "This surface can not be restored because it was created in a different mode.\0";
      case DDERR_XALIGN:
          return "Rectangle provided was not horizontally aligned on required boundary.\0";
      case D3DERR_BADMAJORVERSION:
          return "D3DERR_BADMAJORVERSION\0";
      case D3DERR_BADMINORVERSION:
          return "D3DERR_BADMINORVERSION\0";
      case D3DERR_EXECUTE_LOCKED:
          return "D3DERR_EXECUTE_LOCKED\0";
      case D3DERR_EXECUTE_NOT_LOCKED:
          return "D3DERR_EXECUTE_NOT_LOCKED\0";
      case D3DERR_EXECUTE_CREATE_FAILED:
          return "D3DERR_EXECUTE_CREATE_FAILED\0";
      case D3DERR_EXECUTE_DESTROY_FAILED:
          return "D3DERR_EXECUTE_DESTROY_FAILED\0";
      case D3DERR_EXECUTE_LOCK_FAILED:
          return "D3DERR_EXECUTE_LOCK_FAILED\0";
      case D3DERR_EXECUTE_UNLOCK_FAILED:
          return "D3DERR_EXECUTE_UNLOCK_FAILED\0";
      case D3DERR_EXECUTE_FAILED:
          return "D3DERR_EXECUTE_FAILED\0";
      case D3DERR_EXECUTE_CLIPPED_FAILED:
          return "D3DERR_EXECUTE_CLIPPED_FAILED\0";
      case D3DERR_TEXTURE_NO_SUPPORT:
          return "D3DERR_TEXTURE_NO_SUPPORT\0";
      case D3DERR_TEXTURE_NOT_LOCKED:
          return "D3DERR_TEXTURE_NOT_LOCKED\0";
      case D3DERR_TEXTURE_LOCKED:
          return "D3DERR_TEXTURELOCKED\0";
      case D3DERR_TEXTURE_CREATE_FAILED:
          return "D3DERR_TEXTURE_CREATE_FAILED\0";
      case D3DERR_TEXTURE_DESTROY_FAILED:
          return "D3DERR_TEXTURE_DESTROY_FAILED\0";
      case D3DERR_TEXTURE_LOCK_FAILED:
          return "D3DERR_TEXTURE_LOCK_FAILED\0";
      case D3DERR_TEXTURE_UNLOCK_FAILED:
          return "D3DERR_TEXTURE_UNLOCK_FAILED\0";
      case D3DERR_TEXTURE_LOAD_FAILED:
          return "D3DERR_TEXTURE_LOAD_FAILED\0";
      case D3DERR_MATRIX_CREATE_FAILED:
          return "D3DERR_MATRIX_CREATE_FAILED\0";
      case D3DERR_MATRIX_DESTROY_FAILED:
          return "D3DERR_MATRIX_DESTROY_FAILED\0";
      case D3DERR_MATRIX_SETDATA_FAILED:
          return "D3DERR_MATRIX_SETDATA_FAILED\0";
      case D3DERR_SETVIEWPORTDATA_FAILED:
          return "D3DERR_SETVIEWPORTDATA_FAILED\0";
      case D3DERR_MATERIAL_CREATE_FAILED:
          return "D3DERR_MATERIAL_CREATE_FAILED\0";
      case D3DERR_MATERIAL_DESTROY_FAILED:
          return "D3DERR_MATERIAL_DESTROY_FAILED\0";
      case D3DERR_MATERIAL_SETDATA_FAILED:
          return "D3DERR_MATERIAL_SETDATA_FAILED\0";
      case D3DERR_LIGHT_SET_FAILED:
          return "D3DERR_LIGHT_SET_FAILED\0";
      case D3DERR_ZBUFF_NEEDS_VIDEOMEMORY:
          return "D3DERR_ZBUFF_NEEDS_VIDEOMEMORY\0";
      default:
          wsprintf(buff,"Unrecognized DirectX error=%X\0",error);
          return buff;
      }
}

//

static void DebugPrint (char *fmt, ...)
{
	if (fmt)
	{
		char work[256];

		va_list va;
		va_start(va,fmt);
		vsprintf(work,fmt,va);
		va_end(va);

		OutputDebugString(work);
	}
}

//

void DebugAlert (char *title, char *fmt, ...)
{
	OutputDebugString("Alert Box: ");
	if (title) OutputDebugString(title);
	OutputDebugString("\n");

	if (fmt)
	{
		char work[256];

		va_list va;
		va_start(va,fmt);
		vsprintf(work,fmt,va);
		va_end(va);

		MessageBox(0,work,title,MB_OK);

		OutputDebugString(work);
	}
}

//

void COMAPI RenderPipeline::set_auto_flush(int max_opaque_polys)
{
	auto_flush = max_opaque_polys;
}

//

void COMAPI RenderPipeline::set_opaque_pool_size(U32 bytes)
{
	opaque_pool_size = bytes;
	unsigned char * new_opaque_pool = new unsigned char[opaque_pool_size];
	memcpy(new_opaque_pool, opaque_pool, opaque_index);

	delete [] opaque_pool;
	opaque_pool = new_opaque_pool;
}

//

void COMAPI RenderPipeline::set_alpha_pool_size(U32 bytes)
{
	alpha_pool_size = bytes;

	unsigned char * new_alpha_pool = new unsigned char[alpha_pool_size];
	memcpy(new_alpha_pool, alpha_pool, alpha_index);

	delete [] alpha_pool;
	alpha_pool = new_alpha_pool;
}

//

U32 COMAPI RenderPipeline::get_opaque_pool_size(void)
{
	return opaque_pool_size;
}

//

U32 COMAPI RenderPipeline::get_alpha_pool_size(void)
{
	return alpha_pool_size;
}

//

BOOL COMAPI DllMain (HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	switch (fdwReason)
	{
	//
	// DLL_PROCESS_ATTACH: Create object server component and register it with DACOM manager
	//
		case DLL_PROCESS_ATTACH:
		{
			#if USE_HEAP
			HEAP = HEAP_Acquire();
			SetDllHeapMsg(hinstDLL);
			#endif
			CoInitialize(0);	// start up MSCOM system

			IComponentFactory *server1, *server2;
			server1 = new DAComponentFactory2<DAComponentAggregate<RenderPipeline>, AGGDESC> (InterfaceName);
			server2 = new DAComponentFactory2<DAComponentAggregate<RenderPipeline>, AGGDESC> ("ITextureManager");

			if (server1 != NULL && server2 != NULL)
			{
				DACOM = DACOM_Acquire();

				if (DACOM != NULL)
				{
					DACOM->RegisterComponent(server1, InterfaceName, DACOM_LOW_PRIORITY);
					DACOM->RegisterComponent(server2, "ITextureManager", DACOM_LOW_PRIORITY);
				}
				server2->Release();
				server1->Release();
			}
			break;
		}

		case DLL_PROCESS_DETACH:
			CoUninitialize();	// undo MSCOM init
			break;
	}

	return TRUE;
}

//

//