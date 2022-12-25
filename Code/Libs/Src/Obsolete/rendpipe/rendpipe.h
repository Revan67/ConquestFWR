#ifndef RENDPIPE_H
#define RENDPIPE_H

//

#include <d3d.h>
#include "dacom.h"
#include "pixel.h"

//

struct DirectXInfo
{
	LPDIRECTDRAWSURFACE3	lpDDSPrimary;
	LPDIRECTDRAWSURFACE3	lpDDSBack;
	LPDIRECTDRAWSURFACE3	lpZBuffer;

	LPDIRECTDRAW2			lpDD;
	LPDIRECT3D2				lpD3D;
	LPDIRECT3DDEVICE2		lpD3DDevice;
	LPDIRECT3DVIEWPORT2		lpD3DViewport;

	int						num_texture_formats;
	PixelFormat *			texture_formats;

	PixelFormat *			screen_pixel_format;
};

//

struct RPVertex1
{
	Vector			pos;
	unsigned char	r, g, b, a;
	float			u, v;
};

//

typedef RPVertex1 RPVertex;

//

struct RPVertex2
{
	Vector			pos;
	unsigned char	r, g, b, a;
	float			u1, v1;
	float			u2, v2;
};

//

typedef enum
{
	RP_OPAQUE					= 0x01,
	RP_TRANSLUCENT_UNSORTED		= 0x02,
	RP_TRANSLUCENT_DEPTH_SORTED	= 0x04
} RPenum;

//

struct IRenderPipeline : public IDAComponent
{
//
// Creates surfaces, etc. Only use this if you want IRenderPipeline to create and manage all surfaces.
//
	virtual BOOL32 COMAPI startup(HWND hWnd, int hres, int vres, int bpp, BOOL32 set_display_mode, BOOL32 flip_if_possible) = 0;

// 
// Uses someone else's surfaces. Use this with GLD3D for example.
//
	virtual BOOL32 COMAPI startup(HWND hWnd, const DirectXInfo * dxinfo) = 0;

	virtual void COMAPI shutdown(void) = 0;

//
// Fills in DirectXInfo struct for you.
//
	virtual void COMAPI getDXinfo(DirectXInfo * dxinfo) = 0;

	virtual void COMAPI set_display_mode(int hres, int vres, int bpp) = 0;
	virtual void COMAPI restore_display_mode(void) = 0;

	virtual void COMAPI	enable(int state) = 0;
	virtual void COMAPI	disable(int state) = 0;
	virtual BOOL32 COMAPI is_enabled(int state) = 0;

	virtual void COMAPI set_opaque_pool_size(U32 bytes) = 0;
	virtual void COMAPI set_alpha_pool_size(U32 bytes) = 0;

	virtual U32 COMAPI get_opaque_pool_size(void) = 0;
	virtual U32 COMAPI get_alpha_pool_size(void) = 0;
//
// This state goes straight through to D3D immediately. 
//
	virtual void COMAPI	set_render_state(D3DRENDERSTATETYPE state, DWORD value) = 0;

//
// Set to automatically flush opaque polys after a certain number of polys has been
// submitted. Currently the smallest unit to get rendered is a list, so if you have
// auto-flush set to 5 polys and you submit a 500-poly list, it won't break the list
// up, it will still flush all 500 polys at once.
//
	virtual void COMAPI set_auto_flush(int max_opaque_polys) = 0;

//
// Clears back-buffer and depth buffer. Need to be able to select buffer, clear values, etc.
//
	virtual void COMAPI clear_buffers(void) = 0;

//
// Flips or blits as appropriate.
//
	virtual void COMAPI swap_buffers(void) = 0;


//
// The following calls to set viewport, modelview, and projection matrices STICK to the list
// so that whatever these are set to at the time a list is submitted will be recalled when
// that list is flushed (rendered).
//
	virtual void COMAPI	set_viewport(int x, int y, int w, int h) = 0;
	virtual void COMAPI set_modelview(const Transform & modelview) = 0;
	virtual void COMAPI set_ortho(float x, float y, float w, float h, float znear = -1.0, float zfar = +1.0) = 0;
	virtual void COMAPI set_perspective(float fovy, float aspect, float znear, float zfar) = 0;

//
// This state gets stored with the list and called up when the list polys
// are rendered. Currently supported properties are blend enable, source/dest blend functions,
// and depth compare function.
//
	virtual void COMAPI set_list_render_state(D3DRENDERSTATETYPE state, DWORD value) = 0;

//
// Unfortunately Direct3D requires this and some cards, e.g. PowerVR,
// actually depend on it.
//
	virtual void COMAPI begin_scene(void) = 0;

		virtual void COMAPI submit_list(D3DPRIMITIVETYPE type, const RPVertex1 * verts, int num_verts, bool clip) = 0;
		virtual void COMAPI submit_indexed_list(D3DPRIMITIVETYPE type, const RPVertex1 * verts, int num_verts, const U16 * indices, int num_indices, bool clip) = 0;

		virtual int COMAPI flush(DWORD flags = RP_OPAQUE | RP_TRANSLUCENT_DEPTH_SORTED) = 0;

//
// Direct3D EndScene().
//
	virtual void COMAPI end_scene(void) = 0;
};

//

#endif
