//---------------------------------------------------------------------------
//
// VERTEX.H = Software Version
//
//---------------------------------------------------------------------------

// Direct3D
//#include <d3d.h>		// D3DTLVERTEX

#define C5(r) ((int(r)>>3)&0x1F)
#define C6(r) ((int(r)>>2)&0x3F)
#define RGB_TRIPLET(r,g,b) (C5(r)<<11)|(C6(g)<<5)|(C5(b))

//---------------------------------------------------------------------------

struct _VERTEX
{
	float sx,sy,sz;
	float rhw;
	float tu,tv;
	int color;

	void set_RGBA(float _r, float _g, float _b, float _a)
	{
		int r = _r * 255;
		int g = _g * 255;
		int b = _b * 255;
		int a = _a * 255;
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
