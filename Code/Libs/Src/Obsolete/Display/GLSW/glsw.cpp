//---------------------------------------------------------------------------
/*
	GLSW.CPP

	Copyright (C) 1997 Digital Anvil, Inc.

	Created: October 1997

	Author: Paul Isaac
*/
//---------------------------------------------------------------------------

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "context.h"

//---------------------------------------------------------------------------
// GLOBALS
//---------------------------------------------------------------------------

char Implementation[] = "SW";

DrawMgr DRAW; // GLOBAL

#define VINDEX WORD	// int

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

void DrawPoints (int n, const VINDEX *output, const _VERTEX *vScreen, int vCount)
{
	DRAW.draw_lock();

	for (int i=0; i<n; i+=1)
	{
		const _VERTEX *v0 = vScreen + output[i];
		int color = v0->color;
		DRAW.draw_line(v0->sx,v0->sy, v0->sx,v0->sy, color);
	}

	DRAW.draw_unlock();
}

//---------------------------------------------------------------------------

void DrawLines (int n, const VINDEX *output, const _VERTEX *vScreen, int vCount)
{
	DRAW.draw_lock();

	for (int i=0; i<n-1; i+=2)
	{
		const _VERTEX *v0,*v1;
		v0 = vScreen + output[i+0];
		v1 = vScreen + output[i+1];
		int color = v0->color;
		DRAW.draw_line(v0->sx,v0->sy, v1->sx,v1->sy, color);
	}

	DRAW.draw_unlock();
}

//---------------------------------------------------------------------------

void DrawTriangles (int n, const VINDEX *output, const _VERTEX *vScreen, int vCount)
{
	DRAW.draw_lock();

	for (int i=0; i<n-2; i+=3)
	{
		const _VERTEX *v0,*v1,*v2;

		v0 = vScreen + output[i+0];
		v1 = vScreen + output[i+1];
		v2 = vScreen + output[i+2];
/*
		FUTURE?
		int side = 0; // FUTURE: temp side_list[i]; 
		GLenum mode = Polygon_mode[side]; // FUTURE: GL_FILL, GL_LINE
*/
		int color = v0->color;
		DRAW.draw_line(v0->sx,v0->sy, v1->sx,v1->sy, color);
		DRAW.draw_line(v1->sx,v1->sy, v2->sx,v2->sy, color);
		DRAW.draw_line(v2->sx,v2->sy, v0->sx,v0->sy, color);
	}

	DRAW.draw_unlock();
}

//---------------------------------------------------------------------------

void DrawQuads (int n, const VINDEX *output, const _VERTEX *vScreen, int vCount)
{
	DRAW.draw_lock();

	for (int i=0; i<n-3; i+=4)
	{
		const _VERTEX *v0,*v1,*v2,*v3;

		v0 = vScreen + output[i+0];
		v1 = vScreen + output[i+1];
		v2 = vScreen + output[i+2];
		v3 = vScreen + output[i+3];
/*
		FUTURE?
		int side = 0; // FUTURE: temp side_list[i]; 
		GLenum mode = Polygon_mode[side]; // FUTURE: GL_FILL, GL_LINE
*/
		int color = v0->color;

		DRAW.draw_line(v0->sx,v0->sy, v1->sx,v1->sy, color);
		DRAW.draw_line(v1->sx,v1->sy, v2->sx,v2->sy, color);
		DRAW.draw_line(v2->sx,v2->sy, v3->sx,v3->sy, color);
		DRAW.draw_line(v3->sx,v3->sy, v0->sx,v0->sy, color);
	}

	DRAW.draw_unlock();
}

//---------------------------------------------------------------------------

void DrawPolygon (int n, const VINDEX *output, const _VERTEX *vScreen, int vCount)
{
	DRAW.draw_lock();

	const _VERTEX *v0,*v1;

	v0 = vScreen + output[0];

	int color = v0->color;

	for (int i=0; i<n-1; i+=1)
	{
		v1 = vScreen + output[i+1];
		DRAW.draw_line(v0->sx,v0->sy, v1->sx,v1->sy, color);
		v0 = v1; // loop thru list
	}

	v0 = vScreen + output[0];
	v1 = vScreen + output[n-1];
	DRAW.draw_line(v0->sx,v0->sy, v1->sx,v1->sy, color);

	DRAW.draw_unlock();
}

//---------------------------------------------------------------------------

//---------------------------------------------------------------------------

//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
