//---------------------------------------------------------------------------
/*
	CONTEXT.H

	Copyright (C) 1997 Digital Anvil, Inc.

	Created: October 1997

	Author: Paul Isaac
*/
//---------------------------------------------------------------------------

#define BUILD_DISPLAY -1
#include "display.h"	// GLenum etc.

#include "matrix.h"		// MatrixMgr, VECTOR
#include "texture.h"	// TextureMgr
#include "vbuffer.h"	// VertexBuffer, DrawList, ListMgr

#include "draw.h"		// DrawContext

#define BYTE_OFFSET(ptr,offset) (((char*)(ptr))+(offset))

//---------------------------------------------------------------------------
// Misc. OpenGL
//---------------------------------------------------------------------------

inline bool invalid_draw_mode (GLenum m)
{
	return m<GL_POINTS || m>GL_POLYGON;
}

inline uchar clamp_color (float rgb)
{
	int c = rgb*255.0;
	if (c < 0)
		c = 0;
	else if (c > 255)
		c = 255;
	return uchar(c);
}

//---------------------------------------------------------------------------
// Light
//---------------------------------------------------------------------------

struct Light
{
	COLOR_VECTOR	ambient;
	COLOR_VECTOR	diffuse;
	VECTOR			pos;
	VECTOR			dir;

// Attenuation factors.
	float			kc;	// constant.
	float			kl;	// linear.
	float			kq; // quadratic.

	Light *			next;

	Light(void)
	{
		ambient.set(0.0f, 0.0f, 0.0f, 1.0f);
		diffuse.set(1.0f, 1.0f, 1.0f, 1.0f);
		pos.set(0.0f, 0.0f, 1.0f, 0.0f);

		kc = 1.0;
		kl = 0.0;
		kq = 0.0;
	}
};

//---------------------------------------------------------------------------
// GL_CONTEXT
//---------------------------------------------------------------------------

struct GL_CONTEXT : ListMgr, MatrixMgr, TextureMgr
{
// Data members

	HDC			hDC;				// original creation value

	DrawContext	draw_context;

	GLenum		err;
	const char *err_string;			// a useful identifier

	GLenum		Polygon_mode[2];	// [0]=front [1]=back face render style

	uint		submit_count;		// max vertices to accumulate before rendering

	GLenum		Begin_mode;			// glBegin
	bool		in_scene;			// BeginScene/EndScene

	bool		ready;				// valid active context
	bool		ready_cmd;			// not in Begin/End and active context
	bool		ready_vertex;		// inside Begin/End and active context

	bool		enable[ENABLE_MAX];

	bool		modify_lighting;
	bool		modify_pixels;
	bool		modify_fog;

	GLenum		front_face;		// glFrontFace; GL_CCW, GL_CW
	bool		cull_face[2];	// glCullFace ; [0]=cull front [1]=cull back; both cant be false
	
	GLenum		blend_src;
	GLenum		blend_dst;

	GLenum		env_mode;

	bool		depth_mask;
	GLenum		depth_func;

	GLenum		shade_model;

	GLint		primitive_count;

	GLuint		texture_generator;

	U32				clear_rgb;		// Internal value
	COLOR_VECTOR	clear_color;

	U32				clear_z;		// Internal value
	float			clear_depth;

	float		line_width;
	float		point_size;
	bool		line_antialias;

// ARRAY STATE

	bool	enable_vertex:1;
	bool	enable_normal:1;
	bool	enable_color:1;
	bool	enable_index:1;
	bool	enable_texcoord:1;
	bool	enable_edgeflag:1;

	U16		client_vlist[MAX_VERTS];	// allocated index in vertex buffer

	struct CLIENT_ARRAY
	{
		uint		size;			// 3
		GLenum		type;			// GL_FLOAT
		uint		stride;			// 3*sizeof(float)
		//uint		count;
		const void	*pointer;

		void *index (int i)
		{
			return BYTE_OFFSET(pointer,i*stride);
		}

		CLIENT_ARRAY (void)
		{
			size = 0;
			type = GL_FLOAT;
			stride = 0;
			pointer = 0;
		}
	};

	CLIENT_ARRAY	client_color;
	CLIENT_ARRAY	client_normal;
	CLIENT_ARRAY	client_texcoord;
	CLIENT_ARRAY	client_edgeflag;
	CLIENT_ARRAY	client_vertex;

	bool			arrays_locked;

// LIST STATE

	DrawList	*active_list;		// glNewList, glEndList
	int			list_base;			// glListBase, glCallLists

// glFrustum, glPerpsective

	float		znear,zfar;
	float		left,top;
	float		right,bottom;

// Clipping

	RECT		scissor;

	//bool		clip_to_view_volume;

	int			any_planes;

	#define		MAX_CLIP_PLANES 6

	VECTOR		clip_plane[MAX_CLIP_PLANES];

// Creation

	int window_x, window_y;
	int window_w, window_h;
	int window_bpp;
	int	window_mode;

// GL STATE = Vertex etc.

	COLOR_VECTOR	vcolor;
	VECTOR			vnormal;
	VECTOR			vpoint;
	TEX_VECTOR		vtexcoord;

	VertexBuffer	vertex_buffer;

// Material

	COLOR_VECTOR	global_ambient;

	MATERIAL mat[2];

	GLubyte pixel_map[3*256];
	GLubyte alpha_map[1*256];

	uint native_palette[256];

// Light

	#define MAX_LIGHTS 8

	Light	lights[MAX_LIGHTS];
	Light *	active_lights;

// Fog

	GLenum			fog_mode;
	COLOR_VECTOR	fog_color;
	float			fog_start;
	float			fog_end;
	float			fog_density;

// Pixel Transfer

	VECTOR			raster_pos;
	COLOR_VECTOR	raster_color;

// ERROR methods

	inline void clear_error (void)
	{
		err = GL_NO_ERROR;
	}

	inline GLenum get_error (void)
	{
		GLenum e = err;
		clear_error();
		return e;
	}

	void gl_error (GLenum code, const char *msg=0, const char *spec=0);

// MISC. TESTS

	inline bool inside_begin_end (void) const
	{
		return Begin_mode != GL_INVALID_ENUM;
	}

	inline bool inside_draw_list (void) const
	{
		return active_list != 0;
	}

	void check_ready (void)
	{
		ready = draw_context.is_ready();
		ready_cmd = ready && !inside_begin_end();
		ready_vertex = ready && inside_begin_end();
	}

	operator HGLRC (void)
	{
		return HGLRC(this);
	}

// CONSTRUCTION

	void reset (void)
	{
		err = GL_NO_ERROR;

		submit_count = 0;
		Begin_mode = GL_INVALID_ENUM;
		in_scene = false;

		Polygon_mode[0] = GL_FILL;	// FRONT
		Polygon_mode[1] = GL_FILL;	// BACK

		vertex_buffer.reset();

		active_list = 0;
		list_base = 0;

		enable[ENABLE_DEPTH_TEST] = false;
		enable[ENABLE_BLEND] = false;
		enable[ENABLE_FOG] = false;
		enable[ENABLE_CULL_FACE] = false;
		enable[ENABLE_SCISSOR_TEST] = false;
		enable[ENABLE_TEXTURE] = false;
		enable[ENABLE_LINE_SMOOTH] = false;
		enable[ENABLE_LIGHTING] = false;
		enable[ENABLE_DITHER] = true;			// Enabled by default, see red book p. 159.

		enable[ENABLE_LIGHT0] = false;
		enable[ENABLE_LIGHT1] = false;
		enable[ENABLE_LIGHT2] = false;
		enable[ENABLE_LIGHT3] = false;
		enable[ENABLE_LIGHT4] = false;
		enable[ENABLE_LIGHT5] = false;
		enable[ENABLE_LIGHT6] = false;
		enable[ENABLE_LIGHT7] = false;

		// Make sure D3D gets initialized
		modify_lighting = true;
		modify_pixels = true;
		modify_fog = true;

		blend_src = GL_ONE;
		blend_dst = GL_ZERO;

		env_mode = GL_MODULATE;
		depth_mask = true;
		depth_func = GL_LESS;

		front_face = GL_CCW;
		cull_face[0] = false;		// dont cull front
		cull_face[1] = true;		// do cull back

		scissor.left = 0;
		scissor.top = 0;
		scissor.right = 0;
		scissor.bottom = 0;

		texture_generator = 1;

		shade_model = GL_FLAT;

		set_clear_color(0,0,0,1);
		set_clear_depth(1.0);

		line_width = 1.0;
		point_size = 1.0;
		line_antialias = false;

		enable_vertex =
		enable_normal =
		enable_color = 
		enable_index = 
		enable_texcoord = 
		enable_edgeflag = false;

		arrays_locked = false;

		window_x = window_y = 0;
		window_w = window_h = 0;
		window_bpp = 0;
		window_mode = 0;

		vpoint.set(0,0,0);
		vcolor.set(1.0F,1.0F,1.0F,1.0F);
		vtexcoord.set(0,0,0,1);
		vnormal.set(0,0,0);

		active_lights = NULL;
		global_ambient.set(0.2f, 0.2f, 0.2f, 1.0f);

		raster_pos.set(0,0,0);
		raster_color.set(1.0F,1.0F,1.0F);

		check_ready();

		fog_mode = GL_LINEAR;
		fog_density = 1.0;
		fog_start = 0.0;
		fog_end = 1.0;

		mat[0].ambient.set(0.2f, 0.2f, 0.2f, 1.0f);
		mat[0].diffuse.set(0.8f, 0.8f, 0.8f, 1.0f);
		mat[0].specular.set(0.0f, 0.0f, 0.0f, 1.0f);
		mat[0].shininess = 0;
		mat[0].emission.set(0.0f, 0.0f, 0.0f, 1.0f);
							   
		mat[1].ambient.set(0.2f, 0.2f, 0.2f, 1.0f);
		mat[1].diffuse.set(0.8f, 0.8f, 0.8f, 1.0f);
		mat[1].specular.set(0.0f, 0.0f, 0.0f, 1.0f);
		mat[1].shininess = 0;
		mat[1].emission.set(0.0f, 0.0f, 0.0f, 1.0f);
	}

	GL_CONTEXT (void);

	~GL_CONTEXT (void);

//---------------------------------------------------------------------------

	bool configure (const char *description);

	void set_color (GLubyte r, GLubyte g, GLubyte b, GLubyte a=255)
	{
		vcolor.set(r,g,b,a);
	}

	void set_color (float r, float g, float b, float a=1.0)
	{
		vcolor.set(clamp_color(r),clamp_color(g),clamp_color(b),clamp_color(a));
	}

	void set_uv (float u, float v)
	{
		vtexcoord.set(u, v, 0.0, 1.0);
	}

	void set_normal (float x, float y, float z)
	{
		vnormal.set(x,y,z);
	}

	void set_clear_color (float r, float g, float b, float a);

	void set_clear_depth (float depth);

//---------------------------------------------------------------------------

// MODIFY ATTRIBUTES

	void set_blend (GLenum src, GLenum dst)
	{
		blend_src = src;
		blend_dst = dst;

		modify_pixels = true;
	}

//---------------------------------------------------------------------------

// TRANSFORM, PROJECT, CLIP, WINDOW

	void transform (VertexBuffer &vb);

	void build_vertices (VertexBuffer &vb, uint start, uint count, const VECTOR *vClip);

	void build_active_light_list (void);

	void light_vertices(int n, COLOR_VECTOR * dst, const VECTOR * src, const VECTOR * normal);
	void light_vertices(int n, VBVertex * dst, const VECTOR * src, const VECTOR * normal);

	void fog_vertices(int n, uchar * dst, const VECTOR * src);

//---------------------------------------------------------------------------

// DRAW

	int PolySide (int vcount, _VERTEX **vlist);

	bool PolyClip (int vcount, const VertexBuffer &vb, const VINDEX *vchain);

	int compute_side (_VERTEX *v0, _VERTEX *v1, _VERTEX *v2);

	//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

	void DrawCmd (GLenum type, VertexBuffer &vb, int vcount, const VINDEX *vchain);

//---------------------------------------------------------------------------
// IDisplay methods
//---------------------------------------------------------------------------

// STATE

	void set_modify (EnableIndex i)
	{
		switch (i)
		{
			case ENABLE_BLEND:
			case ENABLE_DITHER:
				modify_pixels = true;
				break;

			case ENABLE_LIGHTING:
			case ENABLE_LIGHT0:
			case ENABLE_LIGHT1:
			case ENABLE_LIGHT2:
			case ENABLE_LIGHT3:
			case ENABLE_LIGHT4:
			case ENABLE_LIGHT5:
			case ENABLE_LIGHT6:
			case ENABLE_LIGHT7:
				modify_lighting = true;
				break;

			case ENABLE_FOG:
				modify_fog = true;
				break;
		}
	}

	void cmd_enable (EnableIndex i)
	{
		set_modify(i);
		enable[i] = true;
	}

	void cmd_disable (EnableIndex i)
	{
		set_modify(i);
		enable[i] = false;
	}

// VERTEX 

	void use_vertex (uint vi, bool flush=false)
	// note: set "flush" true for raw glVertex submissions
	{
		assert(vi != NULL_VERTEX);  // gl_error(GL_OUT_OF_MEMORY,"Vertex");

		if (active_list)
			active_list->use_vertex(vi);
		else
		{
			vertex_buffer.use_vertex(vi);

			if (vertex_buffer.num_chains >= submit_count)
			{
				render(submit_count,flush);
			}
		}
	}

	void vertex (float x, float y, float z=0, float w=0)
	{
		vpoint.set(x,y,z,w);

		uint vi = vertex_buffer.new_vertex();

		if (vi == NULL_VERTEX)
		{ gl_error(GL_OUT_OF_MEMORY,"Vertex"); return; }

		vertex_buffer.set_flag(vi,BIT_COLOR|BIT_NORMAL|BIT_TEXCOORD);
		vertex_buffer.set_vertex(vi, vpoint);
		vertex_buffer.set_color(vi, vcolor);
		vertex_buffer.set_normal(vi, vnormal);
		vertex_buffer.set_texcoord(vi, vtexcoord);

		use_vertex(vi,true);
	}

//---------------------------------------------------------------------------

// PERSPECTIVE

	void viewport (GLint x, GLint y, GLsizei width, GLsizei height)
	{
		// (jy) convert y from GL meaning to ours
		y = draw_context.height - y - height;

		window_x = x;
		window_y = y;
		window_w = width;
		window_h = height;
/*
		h_scale = (window_w-1)/2.0;
		v_scale = (window_h-1)/2.0;

		h_offset = window_x + h_scale;
		v_offset = window_y + v_scale;
*/
		h_scale = (window_w)/2.0;
		v_scale = (window_h)/2.0;

		h_offset = window_x + h_scale;
		v_offset = window_y + v_scale;

		v_scale = -v_scale;
	}

	//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

	void frustum (float left, float right, float bottom, float top, float z0, float z1)
	{
		if (z0<=0.0 || z1<=0.0)
		{ gl_error(GL_INVALID_VALUE, "Frustum", "near or far"); return; }

		znear = z0;
		zfar = z1;

		frustum_matrix(left,right,bottom,top,z0,z1);
	}

	void cmd_translate (float x, float y, float z)
	{
		matrix_translate(x,y,z);
	}

//---------------------------------------------------------------------------

// BEGIN / END

	void reset_vertices (void)
	{
		vertex_buffer.reset();

		any_clip =  0;
		all_clip = -1;

		// FUTURE: re-use transformed vertices 
		if (enable_vertex) // If client arrays in use.
		{
			//for (uint i=0; i<client_vertex.count; i++)
			//	client_vlist[i] = NULL_VERTEX;
// pci - find a better way?
			int count = countof(client_vlist);
			memset(client_vlist, 0xff, sizeof(client_vlist[0]) * count);
		}
	}

	// IMPLEMENTATION
	void begin_scene (void);
	void end_scene (void);
	void setup_pixels (void);
	void setup_lighting (void);
	void setup_fog(void);

	void cmd_begin (void);

	//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

	void render (int num_chains, bool flush=false);

	//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

	bool perform (DrawList *list, uint &i);

	//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

	void update_palette (void);

	//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

	void cmd_bind_texture (int texture)
	{
		TextureMgr::bind_texture(texture);
	}

	//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

	void set_client_state (GLenum array, bool b)
	{
		switch (array)
		{
		case GL_VERTEX_ARRAY:
			enable_vertex = b;
			break;
		case GL_NORMAL_ARRAY:
			enable_normal = b;
			break;
		case GL_COLOR_ARRAY:
			enable_color = b;
			break;
		case GL_INDEX_ARRAY:
			enable_index = b;
			break;
		case GL_TEXTURE_COORD_ARRAY:
			enable_texcoord = b;
			break;
		case GL_EDGE_FLAG_ARRAY:
			enable_edgeflag = b;
			break;
		default:
			gl_error(GL_INVALID_ENUM,"Enable/Disable ClientState");
		}
	}

	//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

	uint use_client_vertex (uint i)
	{
		uint v = client_vlist[i];

		if (v != NULL_VERTEX)	// already copied?
			goto re_use;

	// LOAD Color

		if (enable_color)
		{
			switch (client_color.type)
			{
				case GL_BYTE:	// signed colors?
				case GL_UNSIGNED_BYTE:
				{
					uchar *list = (uchar *)client_color.index(i);

					switch (client_color.size)
					{
						case 3:
							vcolor.set(list[0],list[1],list[2]);
							break;

						case 4:
							vcolor.set(list[0],list[1],list[2],list[3]);
							break;
					} // size
					break;
				}
				
				case GL_FLOAT:
				{
					float *list = (float *)client_color.index(i);

					switch (client_color.size)
					{
						case 3:
							vcolor.set(list[0],list[1],list[2]);
							break;

						case 4:
							vcolor.set(list[0],list[1],list[2],list[3]);
							break;
					} // size
					break;
				}

				default: // Minimal support! (tm)
					gl_error(GL_INVALID_ENUM,"Draw Array/Elements");
					return NULL_VERTEX;
			} // type
		} // enable_color

	// LOAD Normal

		if (enable_normal)
		{
			//vertex_buffer.set_normal();
		}

	// LOAD TexCoord

		if (enable_texcoord)
		{
			//vertex_buffer.set_texcoord();
		}

	// LOAD EdgeFlag

		if (enable_edgeflag)
		{
			//vertex_buffer.set_edgeflag();
		}

	// LOAD Vector

		if (enable_vertex)
		{
			switch (client_vertex.type)
			{
				case GL_FLOAT:
				{
					float *list = (float *)client_vertex.index(i);

					switch (client_vertex.size)
					{
						case 2:
							vpoint.set(list[0],list[1],0,1);
							break;

						case 3:
							vpoint.set(list[0],list[1],list[2],1);
							break;

						case 4:
							vpoint.set(list[0],list[1],list[2],list[3]);
							break;
					} // size
					break;
				}

				default: // Minimal support! (tm)
					gl_error(GL_INVALID_ENUM,"Draw Array/Elements");
					return NULL_VERTEX;
			} // type
		}

	// CREATE VERTEX

		if (enable_vertex)
		{
			v = vertex_buffer.new_vertex();

			if (v == NULL_VERTEX)
				return v; // failed!

			vertex_buffer.set_flag(v,BIT_COLOR|BIT_NORMAL|BIT_TEXCOORD);
			vertex_buffer.set_color(v, vcolor);
			vertex_buffer.set_normal(v, vnormal);
			vertex_buffer.set_texcoord(v, vtexcoord);
			vertex_buffer.set_vertex(v, vpoint);

			assert(v <= 0xFFFE);		// reserve 0xFFFF
			client_vlist[i] = U16(v);		// pack?
		}

re_use:

	// USE VERTEX

		use_vertex(v);

		return v;
	}

//---------------------------------------------------------------------------

}; // IDisplay
