#include "matrix.h"		// VECTOR

// Direct3D
#include <d3d.h>		// D3DTLVERTEX

//---------------------------------------------------------------------------
// VERTEX INFO
//---------------------------------------------------------------------------

struct COLOR_VECTOR
{
	//float r,g,b,a;
	uchar r, g, b, a;
	// FUTURE: order for BGRA access?

	void set (float _r, float _g, float _b, float _a=1.0)
	{
		r = _r * f2i;
		g = _g * f2i;
		b = _b * f2i;
		a = _a * f2i;
	}

	void set (uchar _r, uchar _g, uchar _b, uchar _a=255)
	{
		r = _r;
		g = _g;
		b = _b;
		a = _a;
	}

	DWORD packed (void) const
	{
		return (a << 24) | (r << 16) | (g << 8) | b;
	}
};

//---------------------------------------------------------------------------

struct TEX_VECTOR
{
	float s,t,r,q;

	void set (float _s, float _t=0, float _r=0, float _q=1.0)
	{
		s = _s; // U
		t = _t; // V
		r = _r;
		q = _q;
	}
};

//---------------------------------------------------------------------------
// VERTEX
//---------------------------------------------------------------------------

struct _VERTEX : public D3DTLVERTEX
{
	void set_RGBA(GLubyte r, GLubyte g, GLubyte b, GLubyte a)
	{
		color = (a << 24) | (r << 16) | (g << 8) | b;
	}

	int get_rub(void) const
	{
		return (color >> 16) & 0xff;
	}

	int get_gub(void) const
	{
		return (color >> 8) & 0xff;
	}

	int get_bub(void) const
	{
		return color & 0xff;
	}

	int get_ai(void) const
	{
		return color >> 24;
	}
};

//---------------------------------------------------------------------------
