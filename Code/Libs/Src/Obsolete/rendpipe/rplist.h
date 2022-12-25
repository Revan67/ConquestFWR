//
//
//

#ifndef RPLIST_H
#define RPLIST_H

//

//
// Standard vertex structure used to store batch lists. Doesn't allow
// for glNormal().
//
struct RPListVertex1
{
	float			u, v;
	unsigned char	r, g, b, a;
	float			x, y, z, w;
	int				clip;
};

//
// RPList - poly batch list struct. Stores the equivalent
// of one glBegin()/glEnd() pair as an indexed vertex array.
//
struct RPList
{
#ifdef DX_TRANSFORMS
	D3DMATRIX		modelview;
	D3DMATRIX		projection;
	DWORD			x, y, w, h;		// viewport settings.
#endif

	bool			is_multi;

	unsigned int	texture;
	unsigned int	multitexture;

	D3DCMPFUNC		depth_func;

	bool			blend;
	D3DBLEND		src_blend_func;
	D3DBLEND		dst_blend_func;

	bool			multi_blend;
	D3DBLEND		multi_src_blend_func;
	D3DBLEND		multi_dst_blend_func;

	unsigned char	r, g, b, a;
	float			u, v;
	float			mu, mv;

	D3DPRIMITIVETYPE	type;		// GL_TRIANGLES, etc.
	unsigned int		num_verts;

#ifdef DX_TRANSFORMS
	D3DLVERTEX *		verts;
#else
	RPListVertex1 *		verts;
#endif

	unsigned int		num_indices;
	U16 *				indices;	// into vertex list.

	bool				clip;

	RPList(void)
	{
		init();
	}

	virtual ~RPList(void)
	{
	}

	void init(void)
	{
		is_multi = false;

		texture = 0;
		multitexture = 0;
		depth_func = D3DCMP_LESS;

		blend = multi_blend = false;
		src_blend_func = multi_src_blend_func = D3DBLEND_ONE;
		dst_blend_func = multi_dst_blend_func = D3DBLEND_ZERO;

		type = D3DPT_FORCE_DWORD;

		num_verts = 0;
		verts = NULL;

		num_indices = 0;
		indices = NULL;

		clip = false;
	}

	inline bool is_indexed(void) const
	{
		return (num_indices != 0);
	}
};

//

#endif
