//---------------------------------------------------------------------------
/*
	DRAW.H

	Copyright (C) 1997 Digital Anvil, Inc.

	Created: November 1997

	Authors: Paul Isaac & Bill Baldwin
*/
//---------------------------------------------------------------------------

#ifndef _DRAW_H
#define _DRAW_H

//---------------------------------------------------------------------------

//#include "display.h"	// PIXELFORMAT

#include <ddraw.h>		// IDirectDraw2, etc.
#include <d3d.h>

#include "typedefs.h"	// U32
#include "pixel.h"

// LINE.H
typedef unsigned short word;
extern void DrawLine (void * buffer, int x1, int y1, int x2, int y2, word color, unsigned int stride);

#define DEBUG 0 // pci - no D3D display

//---------------------------------------------------------------------------
// GLOBALS
//---------------------------------------------------------------------------

extern LPDIRECTDRAW2		lpDD;

extern int texture_format_cnt;
extern PixelFormat texture_formats[64];

extern PixelFormat screen_pixel_format;

//---------------------------------------------------------------------------
// DrawContext
//---------------------------------------------------------------------------

struct DrawContext
{
	HWND	hWnd;

	int		pixel_format;

	int		width;
	int		height;
	int		bpp;

	int		flip;
	int		pages;
	int		zbits;

	LPDIRECTDRAWSURFACE3	lpDDSPrimary;
	LPDIRECTDRAWSURFACE3	lpDDSBack;
	LPDIRECTDRAWSURFACE3	lpZBuffer;

	LPDIRECTDRAW2			lpDD;
	LPDIRECT3D2				lpD3D;
	LPDIRECT3DDEVICE2		lpD3DDevice;
	LPDIRECT3DVIEWPORT2		lpD3DViewport;

#ifdef RSTATE_CHECK
	// state tracking for this draw context
	// watch out for more states under dx6

	DWORD render_states[50];
#endif

	// Lock state
	void *buffer;
	int pitch;

	bool is_ready (void)
	{
		return lpD3DDevice != 0;
	}

	int calculate_pitch (void) const
	{
		return width * (bpp / 8);
	}

	void init (HWND h)
	{
		hWnd = h;

		pixel_format = 0;

		width = height = bpp = 0;

		flip = pages = zbits = 0;

		lpDDSPrimary	= NULL;
		lpDDSBack		= NULL;

		lpZBuffer		= NULL;
		lpD3D			= NULL;
		lpD3DDevice		= NULL;
		lpD3DViewport	= NULL;

		buffer = 0;
		pitch = 0;
	}

	DrawContext (void)
	{
		init(0);
	}

	bool is_locked (void)
	{
		return buffer != 0;
	}

	void set_background(float r, float g, float b, float a)
	{
#if 0
		D3DMATERIAL mat;
		memset(&mat, 0, sizeof(mat));
		mat.dwSize = sizeof(mat);
		mat.emissive.r = r;
		mat.emissive.g = g;
		mat.emissive.b = b;
		mat.emissive.a = 1.0;

		HRESULT ddrval = lpD3DMaterial->SetMaterial(&mat);
		if (ddrval == DD_OK)
		{
			D3DMATERIALHANDLE hmat;
			ddrval = lpD3DMaterial->GetHandle(lpD3DDevice, &hmat);
			if (ddrval == DD_OK)
			{
				lpD3DViewport->SetBackground(hmat);
			}
		}
#endif
	}

	inline void set_render_state( D3DRENDERSTATETYPE which, DWORD newstate )
	{
#ifdef RSTATE_CHECK
		if (render_states[(int) which] != newstate)
		{
			render_states[(int) which]= newstate;
			
			lpD3DDevice->SetRenderState(which, newstate);
		}
#else
	lpD3DDevice->SetRenderState(which, newstate);
#endif
	}
};

//---------------------------------------------------------------------------
// DrawMgr
//---------------------------------------------------------------------------

struct DrawMgr
{
	DrawContext *active_context;	// context[] pointer

	bool		DDraw_active;
	HINSTANCE	DDraw_lib_handle;	// handle to ddraw library
	DWORD		lock_flags;			// Flags passed to IDDSurface::Lock

	bool		fullscreen;

	HWND		desktop_wnd;	// save off handle of window that changes display mode.
	int			desktop_w;			// Windows desktop
	int			desktop_h;
	int			desktop_bpp;

	static DDCAPS	hw_caps;		// HW caps.
	static DDCAPS	hel_caps;		// Software emulation caps.

	bool		use_vertex_fog;		// From D3D caps.

	#define MAX_FORMATS	1
	int			current_format;
	int			num_formats;
	PIXELFORMATDESCRIPTOR pixel_formats[MAX_FORMATS];

	PIXELFORMATDESCRIPTOR *get_format (int iPixelFormat)
	{
		if (iPixelFormat < 1 || iPixelFormat > num_formats)
			return 0;
		return pixel_formats+iPixelFormat-1;
	}

// CONSTRUCTION

	DrawMgr (void);

// SYSTEM

	bool startup (void);
	void shutdown (void);

// DISPLAY MODE

	bool set_display_mode (HWND wnd, int w, int h, int bpp);
	bool restore_display_mode (HWND wnd);

	void set_fullscreen(bool yesno);

// CONTEXT

	bool context_ready (void)
	{
		return active_context != 0 && active_context->lpDDSPrimary != 0 && active_context->lpDDSBack != 0;
	}

	bool create_context (DrawContext *ctx);
	void destroy_context (DrawContext *context);

	HRESULT alloc_surfaces (DrawContext *dst);
	void free_surfaces (DrawContext *dst);

// SURFACE

	HRESULT restore_surface (LPDIRECTDRAWSURFACE3 lpdds);

	LPDIRECTDRAWSURFACE3 get_surface (int index);

	void lock_surface (int index, void **ptr, int *pitch);
	void unlock_surface (int index);

	void flip_surface (void);

	void clear_color (int index, U32 color, RECT *box=0);
	void clear_zbuffer (U32 depth=0xFFFFFFFF, RECT *box=0);

// DRAW METHODS

	void draw_lock (void)
	{
		DrawContext *w = active_context;
		lock_surface(1,&w->buffer,&w->pitch);
	}

	void draw_unlock (void)
	{
		DrawContext *w = active_context;
		unlock_surface(1);
		w->buffer = 0;
	}

	void draw_line (int x0, int y0, int x1, int y1, int color)
	{
		DrawContext *w = active_context;
		if (w->is_locked())
		{
			DrawLine(w->buffer, x0,y0, x1,y1, color, w->pitch);
		}
	}

	HRESULT check_D3D_caps(void);
};

//void DebugPrint (char *fmt, ...);
char *DD_message(HRESULT error);

//---------------------------------------------------------------------------

#endif // _DRAW_H
