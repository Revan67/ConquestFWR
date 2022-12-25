//---------------------------------------------------------------------------
/*
	GLD3D.CPP

	Copyright (C) 1997 Digital Anvil, Inc.

	Created: October 1997

	Author: Paul Isaac
*/
//---------------------------------------------------------------------------

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "context.h"

void DebugPrint (char *fmt, ...);

#define DD_VERIFY(dd) if((dd)!=DD_OK) { DebugPrint("%s\n", DD_message(dd)); }

//#define PERMEDIA2

//---------------------------------------------------------------------------
// GLOBALS
//---------------------------------------------------------------------------

char Implementation[] = "D3D";

DrawMgr DRAW; // GLOBAL

//---------------------------------------------------------------------------
// RASTERIZER Draw Commands
//---------------------------------------------------------------------------

U32 GetDrawColor (const PIXELFORMATDESCRIPTOR *pix, byte r, byte g, byte b, byte a)
{
	U32 result;
	if (pix->iPixelType == PFD_TYPE_COLORINDEX)
	{
		result = 0xffffffff;
	}
	else
	{
		#define C(c8,bits,shift) (((c8) >> (8-(bits))) << (shift))

		result =(	C(r,pix->cRedBits,pix->cRedShift) |
					C(g,pix->cGreenBits,pix->cGreenShift) |
					C(b,pix->cBlueBits,pix->cBlueShift) |
					C(a,pix->cAlphaBits,pix->cAlphaShift) );

		#undef C
	}
	return result;
}

//---------------------------------------------------------------------------

	void i2w_copy (WORD *dst, const VINDEX *src, uint count)
	// copy INT to WORD
	{
		assert((int(dst)&3) == 0);
		assert((int(src)&3) == 0);

		int pairs = count/2;
		while (pairs-- > 0)
		{
			*(DWORD *)dst = src[0] | (src[1]<<16);
			dst+=2;
			src+=2;
		}
		if (count & 1)
		{
			*dst = WORD(*src);
		}
	}

//---------------------------------------------------------------------------

void DrawPoints (int n, const VINDEX *output, _VERTEX *vScreen, int vCount)
{
	LPDIRECT3DDEVICE2 lpD3DDevice = DRAW.active_context->lpD3DDevice;

	WORD vDraw[MAX_CHAINS];

#if 0	// Note: D3DPT_POINTLIST = D3DERR_INVALIDPRIMITIVETYPE?
	i2w_copy(vDraw,output,n);

	HRESULT dd = lpD3DDevice->DrawIndexedPrimitive( D3DPT_POINTLIST,
#else
	int more = vCount;

	// DOUBLE REFERENCES
	for (int i=n-1; i>=0; i--)
	{
		int v0 = output[i];
		int v1 = more++;
		vScreen[v1] = vScreen[v0];
		vScreen[v1].sx += 1.0;		// force D3D to draw a point?
		vDraw[i*2+0] = v0;
		vDraw[i*2+1] = v1;
	}
	n *= 2;
	vCount = more;

	HRESULT dd = lpD3DDevice->DrawIndexedPrimitive( D3DPT_LINELIST,
#endif
							D3DVT_TLVERTEX, vScreen, vCount,
							vDraw, n,
							D3DDP_DONOTCLIP | D3DDP_DONOTUPDATEEXTENTS);
	DD_VERIFY(dd);
}

//---------------------------------------------------------------------------

void DrawLines (int n, const VINDEX *output, _VERTEX *vScreen, int vCount)
{
	LPDIRECT3DDEVICE2 lpD3DDevice = DRAW.active_context->lpD3DDevice;
	
	WORD vDraw[MAX_CHAINS];

	i2w_copy(vDraw,output,n);

	HRESULT dd = lpD3DDevice->DrawIndexedPrimitive( D3DPT_LINELIST,
							D3DVT_TLVERTEX, vScreen, vCount,
							vDraw, n,
							D3DDP_DONOTCLIP | D3DDP_DONOTUPDATEEXTENTS);
	DD_VERIFY(dd);
}

//---------------------------------------------------------------------------

void DrawTriangles (int n, const VINDEX *output, _VERTEX *vScreen, int vCount)
{
	LPDIRECT3DDEVICE2 lpD3DDevice = DRAW.active_context->lpD3DDevice;

	WORD vDraw[MAX_CHAINS];

	i2w_copy(vDraw,output,n);

	HRESULT dd = lpD3DDevice->DrawIndexedPrimitive( D3DPT_TRIANGLELIST,
							D3DVT_TLVERTEX, vScreen, vCount,
							vDraw, n,
							D3DDP_DONOTCLIP | D3DDP_DONOTUPDATEEXTENTS);

if (dd != DD_OK)
{
	DebugPrint("ouch\n");
}
	DD_VERIFY(dd);

}

//---------------------------------------------------------------------------

void DrawQuads (int n, const VINDEX *output, _VERTEX *vScreen, int vCount)
{
	LPDIRECT3DDEVICE2 lpD3DDevice = DRAW.active_context->lpD3DDevice;

	WORD tri_list[MAX_CHAINS];

	// DO NOT OVERFLOW LIST
	if (n > (MAX_CHAINS/6)*4)
		n = (MAX_CHAINS/6)*4;

	int t = 0;

	for (int i=0; i<n-3; i+=4)
	{
		DWORD *dst = (DWORD *)(tri_list+t);
		#if 1 // optimize i2w_copy
		dst[0] = output[i+0] | (output[i+1]<<16);
		dst[1] = output[i+2] | (output[i+0]<<16);
		dst[2] = output[i+2] | (output[i+3]<<16);
		#else
		tri_list[t+0] = output[i+0];
		tri_list[t+1] = output[i+1];
		tri_list[t+2] = output[i+2];

		tri_list[t+3] = output[i+0];
		tri_list[t+4] = output[i+2];
		tri_list[t+5] = output[i+3];
		#endif

		t += 3+3;
	}

	HRESULT dd = lpD3DDevice->DrawIndexedPrimitive( D3DPT_TRIANGLELIST,
							D3DVT_TLVERTEX, vScreen, vCount,
							tri_list, t,
							D3DDP_DONOTCLIP | D3DDP_DONOTUPDATEEXTENTS);
	DD_VERIFY(dd);
}

//---------------------------------------------------------------------------

void DrawPolygon (int n, const VINDEX *output, _VERTEX *vScreen, int vCount)
// Temporary: wireframe for now...
{
	LPDIRECT3DDEVICE2 lpD3DDevice = DRAW.active_context->lpD3DDevice;

	WORD vDraw[MAX_POLY_SIDES*2];

	for (int i=0; i<n-1; i++)
	{
		vDraw[i*2+0] = output[i];
		vDraw[i*2+1] = output[i+1];
	}
	vDraw[i*2+0] = output[i];
	vDraw[i*2+1] = output[0];

	HRESULT dd = lpD3DDevice->DrawIndexedPrimitive( D3DPT_LINELIST,
							D3DVT_TLVERTEX, vScreen, vCount,
							vDraw, n*2,
							D3DDP_DONOTCLIP | D3DDP_DONOTUPDATEEXTENTS);
	DD_VERIFY(dd);
}

//---------------------------------------------------------------------------
// GL_CONTEXT (D3D - specific)
//---------------------------------------------------------------------------

void GL_CONTEXT::begin_scene (void)
{
	draw_context.lpD3DDevice->BeginScene();
}

//---------------------------------------------------------------------------

void GL_CONTEXT::end_scene (void)
{
	draw_context.lpD3DDevice->EndScene();
}

//---------------------------------------------------------------------------

void GL_CONTEXT::setup_lighting (void)
{
	build_active_light_list();
//	draw_context.set_render_state(D3DRS_ALPHABLENDENABLE, enable[ENABLE_BLEND]);
}

//---------------------------------------------------------------------------

void GL_CONTEXT::setup_pixels (void)
{
// glEnable/glDisable

	draw_context.set_render_state(D3DRS_ALPHABLENDENABLE, enable[ENABLE_BLEND]);
	draw_context.set_render_state(D3DRS_DITHERENABLE, enable[ENABLE_DITHER]);

// glBlendFunc

	int src,dst;

#ifdef PERMEDIA2
	if (blend_src == GL_SRC_ALPHA && blend_dst == GL_ONE)
		blend_src = GL_ONE;
#endif

	switch (blend_src)
	{
		case GL_ZERO:
			src = D3DBLEND_ZERO; break;
		case GL_ONE:
			src = D3DBLEND_ONE; break;
		case GL_DST_COLOR:
			src = D3DBLEND_DESTCOLOR; break;
		case GL_ONE_MINUS_DST_COLOR:
			src = D3DBLEND_INVDESTCOLOR; break;
		case GL_SRC_ALPHA:
			src = D3DBLEND_SRCALPHA; break;
		case GL_ONE_MINUS_SRC_ALPHA:
			src = D3DBLEND_INVSRCALPHA; break;
		case GL_DST_ALPHA:
			src = D3DBLEND_DESTALPHA; break;
		case GL_ONE_MINUS_DST_ALPHA:
			src = D3DBLEND_INVDESTALPHA; break;
		case GL_SRC_ALPHA_SATURATE:
			src = D3DBLEND_SRCALPHASAT; break;
			break;

		default:
			gl_error(GL_INVALID_ENUM,"BlendFunc");
			src = -1;
	}
	if (src != -1)
	draw_context.set_render_state(D3DRS_SRCBLEND, src);

	switch (blend_dst)
	{
		case GL_ZERO:
			dst = D3DBLEND_ZERO; break;
		case GL_ONE:
			dst = D3DBLEND_ONE; break;
		case GL_SRC_COLOR:
			dst = D3DBLEND_SRCCOLOR; break;
		case GL_ONE_MINUS_SRC_COLOR:
			dst = D3DBLEND_INVSRCCOLOR; break;
		case GL_SRC_ALPHA:
			dst = D3DBLEND_SRCALPHA; break;
		case GL_ONE_MINUS_SRC_ALPHA:
			dst = D3DBLEND_INVSRCALPHA; break;
		case GL_DST_ALPHA:
			dst = D3DBLEND_DESTALPHA; break;
		case GL_ONE_MINUS_DST_ALPHA:
			dst = D3DBLEND_INVDESTALPHA; break;

		default:
			gl_error(GL_INVALID_ENUM,"BlendFunc");
			dst = -1;
	}
	if (dst != -1)
	draw_context.set_render_state(D3DRS_DESTBLEND, dst);

// glTexEnv

	switch (env_mode)
	{
		case GL_MODULATE:
			draw_context.set_render_state(D3DRS_TEXTUREMAPBLEND, D3DTBLEND_MODULATEALPHA);
			break;
			
		case GL_DECAL:
			draw_context.set_render_state(D3DRS_TEXTUREMAPBLEND, D3DTBLEND_DECALALPHA);
			break;
			
		case GL_BLEND:
			draw_context.set_render_state(D3DRS_TEXTUREMAPBLEND, D3DTBLEND_DECALALPHA);
			break;

		case GL_REPLACE:
			draw_context.set_render_state(D3DRS_TEXTUREMAPBLEND, D3DTBLEND_DECAL);
			break;
		
		default:
			gl_error(GL_INVALID_ENUM,"TexEnv");
	}

// glDepthFunc

	// Note: this should work but RIVA driver seems broken?
	draw_context.set_render_state(D3DRS_ZENABLE, enable[ENABLE_DEPTH_TEST]);

	DWORD df;
	if (0) //!enable[ENABLE_DEPTH_TEST])	// same as ZENABLE = 0
	{
		df = D3DCMP_ALWAYS;
	}
	else switch (depth_func)
	{
		case GL_NEVER:
			df = D3DCMP_NEVER;
			break;
		case GL_ALWAYS:
			df = D3DCMP_ALWAYS;
			break;
		case GL_LESS:
			df = D3DCMP_LESS;
			break;
		case GL_LEQUAL:
			df = D3DCMP_LESSEQUAL;
			break;
		case GL_EQUAL:
			df = D3DCMP_EQUAL;
			break;
		case GL_GEQUAL:
			df = D3DCMP_GREATEREQUAL;
			break;
		case GL_GREATER:
			df = D3DCMP_GREATER;
			break;
		case GL_NOTEQUAL:
			df = D3DCMP_NOTEQUAL;
			break;

		default:
			gl_error(GL_INVALID_ENUM,"glDepthFunc");
			df = -1;
	}

	if (df != -1)
	draw_context.set_render_state(D3DRS_ZFUNC, df);

// glDepthMask

	draw_context.set_render_state(D3DRS_ZWRITEENABLE, depth_mask);
}

//---------------------------------------------------------------------------

void GL_CONTEXT::setup_fog (void)
{
	draw_context.set_render_state(D3DRS_FOGENABLE, enable[ENABLE_FOG]);

	if (enable[ENABLE_FOG])
	{
		draw_context.set_render_state(D3DRS_FOGCOLOR, fog_color.packed());

		if (DRAW.use_vertex_fog)
		{
		// USING VERTEX FOG: Need FOGENABLE = TRUE, FOGTABLEMODE = NONE.
			draw_context.set_render_state(D3DRS_FOGTABLEMODE, D3DFOG_NONE);
		}
		else
		{
		// USING TABLE FOG: Set up fog table parameters.
			DWORD d3dfogmode;
			switch (fog_mode)
			{
				case GL_LINEAR:
					d3dfogmode = D3DFOG_LINEAR;
					break;
				case GL_EXP:
					d3dfogmode = D3DFOG_EXP;
					break;
				case GL_EXP2:
					d3dfogmode = D3DFOG_EXP2;
					break;
			}

			draw_context.set_render_state(D3DRS_FOGTABLEMODE, d3dfogmode);

			if (fog_mode == GL_LINEAR)
			{
				draw_context.set_render_state(D3DRS_FOGTABLESTART, *((DWORD *) &fog_start));
				draw_context.set_render_state(D3DRS_FOGTABLEEND, *((DWORD *) &fog_end));
			}
			else
			{
				draw_context.set_render_state(D3DRS_FOGTABLEDENSITY, *((DWORD *) &fog_density));
			}
		}
	}
}

//---------------------------------------------------------------------------


