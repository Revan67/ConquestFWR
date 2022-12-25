//
//
//

#ifndef PBLIST_H
#define PBLIST_H

//

//
// Standard vertex structure used to store batch lists. Doesn't allow
// for glNormal().
//
struct PBListVertex
{
	float	u, v;
	GLubyte	r, g, b, a;
	float	x, y, z;
	float	mu, mv;		// multitexture coords.
};

//
// PBList - poly batch list struct. Stores the equivalent
// of one glBegin()/glEnd() pair as an indexed vertex array.
//
struct PBList
{
	bool			is_multi;

	GLuint			texture;
	GLuint			multitexture;

	GLenum			depth_func;

	bool			blend;
	GLenum			src_blend_func;
	GLenum			dst_blend_func;

	bool			multi_blend;
	GLenum			multi_src_blend_func;
	GLenum			multi_dst_blend_func;

	GLubyte			r, g, b, a;
	float			u, v;
	float			mu, mv;

	GLenum			type;		// GL_TRIANGLES, etc.
	GLuint			num_verts;
	PBListVertex *	verts;

	GLuint			num_indices;
	GLuint *		indices;	// into vertex list.

	PBList(void)
	{
		init();
	}

	virtual ~PBList(void)
	{
	}

	void init(void)
	{
		is_multi = false;

		texture = 0;
		multitexture = 0;
		depth_func = GL_LESS;

		blend = multi_blend = false;
		src_blend_func = multi_src_blend_func = GL_ONE;
		dst_blend_func = multi_dst_blend_func = GL_ZERO;

		type = GL_NONE;

		num_verts = 0;
		verts = NULL;

		num_indices = 0;
		indices = NULL;
	}

	inline bool is_indexed(void) const
	{
		return (num_indices != 0);
	}

	inline void BindTexture(GLuint txm)
	{
		texture = txm;
	}

	inline void MultiBindTexture(GLuint txm)
	{
		multitexture = txm;
	}

	inline void BlendFunc(GLenum src, GLenum dst)
	{
		blend = true;
		src_blend_func = src;
		dst_blend_func = dst;
	}

	inline void MultiBlendFunc(GLenum src, GLenum dst)
	{
		multi_blend = true;
		multi_src_blend_func = src;
		multi_dst_blend_func = dst;
	}

	inline void DepthFunc(GLenum func)
	{
		depth_func = func;
	}

	inline void Color3ub(GLubyte _r, GLubyte _g, GLubyte _b)
	{
	 	r = _r;
		g = _g;
		b = _b;
		a = 255;
	}

	inline void Color3ubv(const GLfloat * vec)
	{
		Color3ub(vec[0], vec[1], vec[2]);
	}

	inline void Color4ub(GLubyte _r, GLubyte _g, GLubyte _b, GLubyte _a)
	{
	 	r = _r;
		g = _g;
		b = _b;
		a = _a;
	}

	inline void Color4ubv(const GLfloat * vec)
	{
		Color4ub(vec[0], vec[1], vec[2], vec[3]);
	}

	inline void TexCoord2f(float s, float t)
	{
		u = s;
		v = t;
	}

	inline void TexCoord2fv(const GLfloat * vec)
	{
		TexCoord2f(vec[0], vec[1]);
	}

	inline void MultiTexCoord2f(float s, float t)
	{
		mu = s;
		mv = t;
	}

	inline void MultiTexCoord2fv(const GLfloat * vec)
	{
		MultiTexCoord2f(vec[0], vec[1]);
	}

	inline void Vertex3f(float x, float y, float z)
	{
		verts[num_verts].r = r;
		verts[num_verts].g = g;
		verts[num_verts].b = b;
		verts[num_verts].a = a;
		verts[num_verts].u = u;
		verts[num_verts].v = v;
		verts[num_verts].x = x;
		verts[num_verts].y = y;
		verts[num_verts].z = z;
		num_verts++;
	}

	inline void Vertex3fv(const GLfloat * vec)
	{
		Vertex3f(vec[0], vec[1], vec[2]);
	}
};

//

#endif
