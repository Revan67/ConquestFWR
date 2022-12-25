//---------------------------------------------------------------------------
/*
	OpenGL.CPP

	(c) 1998 Digital Anvil

	08-04-98 created (pci)

	$Header: /Tools/Physics Editor/OpenGL.cpp 5     12/16/99 6:28p Kbaird $
*/
//---------------------------------------------------------------------------

#include "3dmath.h"
#include "rendpipeline.h"
#include "opengl.h"

#include <assert.h>

#pragma warning (disable : 4800)

//#define assert assert

typedef unsigned int uint;


__inline unsigned char CLAMP (int c)
{
	if (c < 0)
		c = 0;
	else if (c > 255)
		c = 255;
	return c;
}

__inline unsigned char CLAMP (float f)
{
	return CLAMP(int(f*255));
}

//---------------------------------------------------------------------------
// MISC
//---------------------------------------------------------------------------

	void SetFormat (PixelFormat &pf, GLenum fmt)
	{
		DDPIXELFORMAT ddpf;

		memset(&ddpf, 0, sizeof(ddpf) );
		ddpf.dwSize = sizeof(ddpf);

		switch (fmt)
		{
		case GL_COLOR_INDEX:
			ddpf.dwFlags = DDPF_PALETTEINDEXED8;
			ddpf.dwRGBBitCount = 8;
			break;

		case 3:
		case GL_RGB:
			ddpf.dwFlags = DDPF_RGB;
			ddpf.dwRGBBitCount = 24;
			ddpf.dwRBitMask = 0x00FF0000;
			ddpf.dwGBitMask = 0x0000FF00;
			ddpf.dwBBitMask = 0x000000FF;
			ddpf.dwRGBAlphaBitMask = 0x00000000;
			break;

		case 4:
		case GL_RGBA:
			ddpf.dwFlags = DDPF_RGB|DDPF_ALPHAPIXELS;
			ddpf.dwRGBBitCount = 32;
			ddpf.dwRBitMask = 0x00FF0000;
			ddpf.dwGBitMask = 0x0000FF00;
			ddpf.dwBBitMask = 0x000000FF;
			ddpf.dwRGBAlphaBitMask = 0xFF000000;
			break;
		
		case GL_RGB5_A1:
			ddpf.dwFlags = DDPF_RGB|DDPF_ALPHAPIXELS;
			ddpf.dwRGBBitCount = 16;
			ddpf.dwRBitMask = 0x00007C00;
			ddpf.dwGBitMask = 0x000003E0;
			ddpf.dwBBitMask = 0x0000001F;
			ddpf.dwRGBAlphaBitMask = 0x00008000;
			break;
		
		case GL_RGBA4:
			ddpf.dwFlags = DDPF_RGB|DDPF_ALPHAPIXELS;
			ddpf.dwRGBBitCount = 16;
			ddpf.dwRBitMask = 0x00000F00;
			ddpf.dwGBitMask = 0x000000F0;
			ddpf.dwBBitMask = 0x0000000F;
			ddpf.dwRGBAlphaBitMask = 0x0000F000;
			break;

		default:
			assert(0);
		}

		pf.init(ddpf);
	}

//---------------------------------------------------------------------------
// OpenGL
//---------------------------------------------------------------------------

typedef Transform GL_MATRIX;

// defined outside namespace for nicer Debug sessions

struct Context
{
	GLenum			begin_type;

	RPVertex		vertex;

	int				vmax;
	int				vcount;
	bool			vlock;		// has glVertexArray been called?

	int				imax;
	int				icount;

	GLenum			matrix_mode;

	int				stk_modelview;

	int				stk_projection;

	bool			blend_enable;
	GLenum			blend_src;
	GLenum			blend_dst;

	bool			ztest_enable;
	bool			zwrite_enable;
	GLenum			depth_func;

	bool			texture_enable;
	int				texture_handle;

	bool			cull_enable;
	int				back_face;

	bool			scissor_enable;
	RECT			scissor_box;

	bool			clip_enable;	// tell RP to clip vertices to window?

	DWORD			clear_color;
	U32				clear_depth;

	GL_MATRIX		modelview[16];
	GL_MATRIX		projection[16];

	RPVertex		vlist[8192];
	U16				ilist[8192];

	void set_vertex_mode (GLenum m)
	{
		begin_type = m;
	}
	void clear_vertex_mode (void)
	{
		begin_type = GLenum(-1);
	}
	bool is_vertex_mode (void) const
	{
		return begin_type != -1;
	}

	void reset (void)
	{
		clear_vertex_mode();

		vmax = 8192;
		vcount = 0;
		vlock = false;

		imax = 8192;
		icount = 0;

		matrix_mode = GL_MODELVIEW;

		stk_modelview=1;

		stk_projection=1;

		blend_enable = false;
		blend_src = GL_ONE;
		blend_dst = GL_ONE;

		ztest_enable = true;
		zwrite_enable = true;
		depth_func = GL_LEQUAL;

		texture_enable = false;
		texture_handle = 0;

		cull_enable = false;
		back_face = D3DCULL_CCW;

		scissor_enable = false;

		clip_enable = true;	// clip vertices to window?

		clear_color = RGBA_MAKE(255,255,255,255);
		clear_depth = 0xFFFFFFFF;
	}

	Context (void)
	{
		reset();
	}

	~Context (void)
	{
	}
};

	U32 TextureId = 0;
	RGB Palette[256];

//---------------------------------------------------------------------------
// OpenGL
//---------------------------------------------------------------------------

	Context context;

namespace OpenGL
{
// Internal functions

	inline void set_color (U8 r, U8 g, U8 b, U8 a=255)
	{
		Context *rc = &context;
		rc->vertex.color = RGBA_MAKE(r,g,b,a);
	}

	void setup_blend (void)
	{
		Context *rc = &context;
		{
			int src = D3DBLEND_ONE;
			switch (rc->blend_src)
			{
				case GL_ZERO:					src = D3DBLEND_ZERO; break;
				case GL_ONE:					src = D3DBLEND_ONE; break;
				case GL_DST_COLOR:				src = D3DBLEND_DESTCOLOR; break;
				case GL_ONE_MINUS_DST_COLOR:	src = D3DBLEND_INVDESTCOLOR; break;
				case GL_SRC_ALPHA:				src = D3DBLEND_SRCALPHA; break;
				case GL_ONE_MINUS_SRC_ALPHA:	src = D3DBLEND_INVSRCALPHA; break;
				case GL_DST_ALPHA:				src = D3DBLEND_DESTALPHA; break;
				case GL_ONE_MINUS_DST_ALPHA:	src = D3DBLEND_INVDESTALPHA; break;
				case GL_SRC_ALPHA_SATURATE:		src = D3DBLEND_SRCALPHASAT; break;

				default:
					assert(0);
			}
//list?
			RP->set_render_state(D3DRS_SRCBLEND, src);

			int dst = D3DBLEND_ZERO;
			switch (rc->blend_dst)
			{
				case GL_ZERO:					dst = D3DBLEND_ZERO; break;
				case GL_ONE:					dst = D3DBLEND_ONE; break;
				case GL_SRC_COLOR:				dst = D3DBLEND_SRCCOLOR; break;
				case GL_ONE_MINUS_SRC_COLOR:	dst = D3DBLEND_INVSRCCOLOR; break;
				case GL_SRC_ALPHA:				dst = D3DBLEND_SRCALPHA; break;
				case GL_ONE_MINUS_SRC_ALPHA:	dst = D3DBLEND_INVSRCALPHA; break;
				case GL_DST_ALPHA:				dst = D3DBLEND_DESTALPHA; break;
				case GL_ONE_MINUS_DST_ALPHA:	dst = D3DBLEND_INVDESTALPHA; break;

				default:
					assert(0);
			}
			RP->set_render_state(D3DRS_DESTBLEND, dst);

			if (rc->blend_enable)
				RP->set_render_state(D3DRS_ALPHABLENDENABLE, TRUE);
			else
				RP->set_render_state(D3DRS_ALPHABLENDENABLE, FALSE);
		}
	}

	void setup_texture (void)
	{
		Context *rc = &context;
		{
			if (rc->texture_enable)
			{
				// RP->set_render_state(D3DRS_TEXTUREHANDLE, rc->texture_handle);
				// RP->set_texture_stage_texture(0,rc->texture_handle);
			}
			else
			{
				RP->set_texture_stage_texture(0,0);
				RP->set_texture_stage_state(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
				// RP->set_render_state(D3DRS_TEXTUREHANDLE, 0);
			}
		}
	}

	void setup_depth (void)
	{
		Context *rc = &context;
		{
		// glDepthFunc

			int func;

			if (!rc->ztest_enable)
				func = D3DCMP_ALWAYS;
			else switch (rc->depth_func)
			{
				case GL_NEVER:		func = D3DCMP_NEVER;		break;
				case GL_ALWAYS:		func = D3DCMP_ALWAYS;		break;
				case GL_LESS:		func = D3DCMP_LESS;			break;
				case GL_LEQUAL:		func = D3DCMP_LESSEQUAL;	break;
				case GL_EQUAL:		func = D3DCMP_EQUAL;		break;
				case GL_GEQUAL:		func = D3DCMP_GREATEREQUAL;	break;
				case GL_GREATER:	func = D3DCMP_GREATER;		break;
				case GL_NOTEQUAL:	func = D3DCMP_NOTEQUAL;		break;

				default:
					assert(0);
					func = D3DCMP_ALWAYS;
			}

			RP->set_render_state(D3DRS_ZFUNC, func);

		// glEnable/glDisable (GL_DEPTH_TEST)

			RP->set_render_state(D3DRS_ZENABLE, rc->ztest_enable);

		// glDepthMask

			RP->set_render_state(D3DRS_ZWRITEENABLE, rc->zwrite_enable);
		}
	}

	void setup (void)
	{
		setup_blend();
		setup_texture();
		setup_depth();
	}

// BUFFERS

	void glClearColor (float r, float g, float b, float a)
	{
		
	}

	void glClearDepth (float depth)
	{
		
	}

	void glClear (int bits)
	{

	}

	// TEMPORARY!
	int viewport[4] = { 0,0, 0,0 };

// BEGIN / END

	void glBegin (GLenum m)
	{
		Context *rc = &context;
		assert(!rc->is_vertex_mode());
		rc->set_vertex_mode(m);
		if (rc->vlock)
		{
			assert(rc->vcount > 0);
		}
		else
		{
			rc->vcount = 0;
		}
		rc->icount = 0;
	}
	void glEnd (void)
	{
		Context *rc = &context;
		assert(rc->is_vertex_mode());

		U16 relist[4096];

		if (rc->vcount > 0)
		switch (rc->begin_type)
		{
			case GL_POINTS:
				// Note: this is NOT correct, but INDEXED points don't work on some cards? (pci)
				RP->draw_primitive(D3DPT_POINTLIST, D3DFVF_RPVERTEX, rc->vlist, rc->vcount, 0);
				//RP->draw_indexed_primitive(D3DPT_POINTLIST, vlist, vcount, ilist, icount, true);
				break;

			case GL_LINES:
				if (rc->vlock)
					RP->draw_indexed_primitive(D3DPT_LINELIST, D3DFVF_RPVERTEX, rc->vlist, rc->vcount, rc->ilist, rc->icount, 0);
				else
					RP->draw_primitive(D3DPT_LINELIST, D3DFVF_RPVERTEX, rc->vlist, rc->vcount, 0);
				break;

			case GL_LINE_STRIP:
				RP->draw_primitive(D3DPT_LINESTRIP, D3DFVF_RPVERTEX, rc->vlist,rc->vcount, 0);
				break;

			case GL_LINE_LOOP:
				RP->draw_primitive(D3DPT_LINESTRIP, D3DFVF_RPVERTEX, rc->vlist,rc->vcount, 0);
				// no D3DPT_LINELOOP so close loop manually
				relist[0] = rc->vcount-1;
				relist[1] = 0;
				RP->draw_indexed_primitive(D3DPT_LINELIST, D3DFVF_RPVERTEX, rc->vlist,rc->vcount, relist,2, 0);
			break;

			case GL_TRIANGLES:
				if (rc->vlock)
					RP->draw_indexed_primitive(D3DPT_TRIANGLELIST, D3DFVF_RPVERTEX, rc->vlist, rc->vcount, rc->ilist, rc->icount, 0);
				else
					RP->draw_primitive(D3DPT_TRIANGLELIST, D3DFVF_RPVERTEX, rc->vlist, rc->vcount, 0);
				break;

			case GL_TRIANGLE_STRIP:
				RP->draw_primitive(D3DPT_TRIANGLESTRIP, D3DFVF_RPVERTEX, rc->vlist, rc->vcount, 0);
				break;

			case GL_QUADS:
			{
				int s,d;
				assert(rc->icount/4 <= countof(relist)/6);
				int icount = min(rc->icount,(countof(relist)*4)/6);	// do not overflow!
				for (s=0,d=0; (s+3)<icount; s+=4,d+=6)
				{
					relist[d+0] = rc->ilist[s+0];
					relist[d+1] = rc->ilist[s+1];
					relist[d+2] = rc->ilist[s+2];

					relist[d+3] = rc->ilist[s+0];
					relist[d+4] = rc->ilist[s+2];
					relist[d+5] = rc->ilist[s+3];
				}
				assert(d <= countof(relist));
				RP->draw_indexed_primitive(D3DPT_TRIANGLELIST, D3DFVF_RPVERTEX, rc->vlist, rc->vcount, relist,d, 0);
				break;
			}

			case GL_POLYGON:
				RP->draw_primitive(D3DPT_TRIANGLEFAN, D3DFVF_RPVERTEX, rc->vlist, rc->vcount, 0);
				break;
		}
		rc->clear_vertex_mode();
	}

// VERTICES

	void glColor3ub (U8 r, U8 g, U8 b)
	{
		set_color(r,g,b);
	}
	void glColor3ubv (const U8 *c)
	{
		set_color(c[0],c[1],c[2]);
	}
	void glColor3f (float rr, float gg, float bb)
	{
		U8 r = CLAMP(rr);
		U8 g = CLAMP(gg);
		U8 b = CLAMP(bb);
		set_color(r,g,b);
	}
	void glColor3fv (const float *rgb)
	{
		U8 r = CLAMP(rgb[0]);
		U8 g = CLAMP(rgb[1]);
		U8 b = CLAMP(rgb[2]);
		set_color(r,g,b);
	}
	void glColor4ub (U8 r, U8 g, U8 b, U8 a)
	{
		set_color(r,g,b,a);
	}
	void glColor4f (float rr, float gg, float bb, float aa)
	{
		U8 r = CLAMP(rr);
		U8 g = CLAMP(gg);
		U8 b = CLAMP(bb);
		U8 a = CLAMP(aa);
		set_color(r,g,b,a);
	}

	void glTexCoord2f (float u, float v)
	{
		Context *rc = &context;
		rc->vertex.u = u;
		rc->vertex.v = v;
	}
	void glTexCoord2fv (const float *uv)
	{
		Context *rc = &context;
		rc->vertex.u = uv[0];
		rc->vertex.v = uv[1];
	}

	void glVertex3f (float x, float y, float z)
	{
		Context *rc = &context;
		assert(rc->icount < rc->imax);
		rc->ilist[rc->icount++] = rc->vcount;
		assert(rc->vcount < rc->vmax);
		RPVertex *v = rc->vlist + rc->vcount++;
		v->pos.x = x;
		v->pos.y = y;
		v->pos.z = z;
		v->color = rc->vertex.color;
		v->u = rc->vertex.u;
		v->v = rc->vertex.v;
	}
	void glVertex3fv (const float *xyz)
	{
		glVertex3f(xyz[0],xyz[1],xyz[2]);
	}

	void glVertex2f (float x, float y)
	{
		glVertex3f(x,y,0);
	}

// STATES

/*	void glHint (GLenum target, GLenum mode)
	{
		Context *rc = &context;
		assert(!rc->is_vertex_mode());

		switch (target)
		{
			case GL_LINE_SMOOTH_HINT:
				break;

			case GL_VOLUME_CLIPPING_HINT:
				if (mode == GL_NICEST)
				{
					rc->clip_enable = true;
				}
				else
				{
					rc->clip_enable = false;
				}
				break;

			default:
				assert(0);
		}
		// no support?
	}
*/
	void glEnable (GLenum x)
	{
		Context *rc = &context;
		assert(!rc->is_vertex_mode());
		switch (x)
		{
			case GL_BLEND:
				rc->blend_enable = true;
				setup_blend();
				break;

			case GL_DEPTH_TEST:
				rc->ztest_enable = true;
				setup_depth();
				break;

			case GL_TEXTURE_2D:
				rc->texture_enable = true;
				setup_texture();
				break;

			case GL_CULL_FACE:
				rc->cull_enable = true;
				RP->set_render_state(D3DRS_CULLMODE, rc->back_face);
				break;

			case GL_DITHER:
				//rc->dither_enable = true;
				RP->set_render_state(D3DRS_DITHERENABLE, TRUE);
				break;

			case GL_LIGHTING:
				assert(0);
				break;

			case GL_FOG:
				RP->set_render_state(D3DRS_FOGENABLE, TRUE);
				break;

			case GL_SCISSOR_TEST:
				rc->scissor_enable = true;
				break;

			default:
				assert(0);
		}
	}

	void glDisable (GLenum x)
	{
		Context *rc = &context;
		assert(!rc->is_vertex_mode());
		switch (x)
		{
			case GL_BLEND:
				rc->blend_enable = false;
				setup_blend();
				break;

			case GL_DEPTH_TEST:
				rc->ztest_enable = false;
				setup_depth();
				break;

			case GL_TEXTURE_2D:
				rc->texture_enable = false;
				setup_texture();
				break;

			case GL_CULL_FACE:
				rc->cull_enable = false;
				RP->set_render_state(D3DRS_CULLMODE, D3DCULL_NONE);
				break;

			case GL_DITHER:
				//rc->dither_enable = false;
				RP->set_render_state(D3DRS_DITHERENABLE, FALSE);
				break;

			case GL_LIGHTING:
				// good!
				break;

			case GL_FOG:
				RP->set_render_state(D3DRS_FOGENABLE, FALSE);
				break;

			case GL_SCISSOR_TEST:
				rc->scissor_enable = false;
				break;

			default:
				assert(0);
		}
	}
/*
	void glBindTexture (GLenum x, GLuint id)
	{
		Context *rc = &context;
		assert(!rc->is_vertex_mode());
		assert(x==GL_TEXTURE_2D);
		rc->texture_handle = id;
		TextureId = id;
		setup_texture();
	}
*/
/*
	void glBlendFunc (GLenum src, GLenum dst)
	{
		Context *rc = &context;
		assert(!rc->is_vertex_mode());
		rc->blend_src = src;
		rc->blend_dst = dst;
		setup_blend();
	}
*/
	void glDepthFunc (GLenum func)
	{
		Context *rc = &context;
		assert(!rc->is_vertex_mode());
		rc->depth_func = func;
		setup_depth();
	}

	void glDepthMask (GLboolean b)
	{
		Context *rc = &context;
		assert(!rc->is_vertex_mode());
		rc->zwrite_enable = b;
		setup_depth();
	}

// TEXTURE
/*
	void glTexEnvi (GLenum t, GLenum pname, GLenum value)
	{
		assert(t == GL_TEXTURE_ENV);
		if (pname == GL_TEXTURE_ENV_MODE)
		{
			switch (value)
			{
				case GL_MODULATE:
					RP->set_render_state(D3DRS_TEXTUREMAPBLEND, D3DTBLEND_MODULATEALPHA);
					break;
					
				case GL_DECAL:
					RP->set_render_state(D3DRS_TEXTUREMAPBLEND, D3DTBLEND_DECALALPHA);
					break;
					
				case GL_BLEND:
					RP->set_render_state(D3DRS_TEXTUREMAPBLEND, D3DTBLEND_DECALALPHA);
					break;

				case GL_REPLACE:
					RP->set_render_state(D3DRS_TEXTUREMAPBLEND, D3DTBLEND_DECAL);
					break;

				default:
					assert(0);
			}
		}
		else if (pname == GL_TEXTURE_ENV_COLOR)
		{
		}
		else
		{
			assert(0);
		}
	}
*/
/*
	void glTexParameteri (GLenum target, GLenum pname, GLint param)
	{
		Context *rc = &context;
		assert(!rc->is_vertex_mode());

		int d = 0;
		if (param == GL_NEAREST)
		{
			d = D3DFILTER_NEAREST;
		}
		else if (param == GL_LINEAR)
		{
			d = D3DFILTER_LINEAR;
		}
		else if (param == GL_LINEAR_MIPMAP_NEAREST)
		{
			d = D3DFILTER_LINEARMIPNEAREST;
		}

		assert(target == GL_TEXTURE_2D);
		switch (pname)
		{
		case GL_TEXTURE_MAG_FILTER:
			RP->set_render_state(D3DRS_TEXTUREMAG, d);
			break;

		case GL_TEXTURE_MIN_FILTER:
			RP->set_render_state(D3DRS_TEXTUREMIN, d);
			break;
		}
	}
*/
// MATH
/*
	void glScalef (float x, float y, float z)
	{
		Context *rc = &context;
		switch (rc->matrix_mode)
		{
			case GL_MODELVIEW:
				rc->modelview[0].set_i( x * rc->modelview[0].get_i() );
				rc->modelview[0].set_j( y * rc->modelview[0].get_j() );
				rc->modelview[0].set_k( z * rc->modelview[0].get_k() );
				RP->set_modelview( rc->modelview[0] );
				break;

			default:
				assert(0);
		}
	}
*/
/*
	void glRotatef (float angle, float x, float y, float z)
	{
		Context *rc = &context;

		Quaternion qR(Vector(x,y,z),angle*MUL_DEG_TO_RAD);

		switch (rc->matrix_mode)
		{
			case GL_MODELVIEW:
				rc->modelview[0].set_orientation( rc->modelview[0] * qR );
				RP->set_modelview( rc->modelview[0] );
				break;

			default:
				assert(0);
		}
	}
*/
// MISCELLANEOUS
  
/*
    void glCullFace(GLenum mode)
    {
        if (mode == GL_FRONT)
        {
            RP->set_render_state(D3DRS_CULLMODE, D3DCULL_CW);
        }
        else
        {
            RP->set_render_state(D3DRS_CULLMODE, D3DCULL_CCW);
        }
    }
*/

	void glFlush (void)
	{
//		RP->flush(0);
	}


//---------------------------------------------------------------------------
// NON-STANDARD
//---------------------------------------------------------------------------

/*
	void glVertexArray (Vector *list, int count)
	{
		Context *rc = &context;
		assert(!rc->is_vertex_mode());
		if (list)
		{
			assert(rc->vcount == 0);
			assert(count < rc->vmax);
			for (int i=0; i<count; i++)
			{
				rc->vertex.pos = list[i];
				rc->vlist[i] = rc->vertex;
			}
			rc->vcount = count;
			rc->vlock = true;
		}
		else
		{
			rc->vcount = 0;
			rc->vlock = false;
		}
	}
*/
/*
	void glArrayElement (int v)
	{
		Context *rc = &context;
		assert(rc->icount < rc->imax);
		assert(v < rc->vcount);
		rc->ilist[rc->icount++] = v;
	}
*/
// WORKING?
/*
	struct ELEMENT_4UB_V2F
	{
		U8 r,g,b,a;
		float x,y;
	};

	void glInterleavedArrays (GLenum format, GLsizei stride, const GLvoid *pointer, int count)
	{
		switch (format)
		{
			case GL_C4UB_V2F:
			{
				//glEnableClientState(GL_COLOR_ARRAY);
				//glEnableClientState(GL_VERTEX_ARRAY);
				int csize = 4*sizeof(char);
				int vsize = 2*sizeof(float);

				if (stride==0)		// is this handled right?
					stride = csize + vsize;

				//char *ptr = (char *)pointer;
				//glColorPointer(4, GL_UNSIGNED_BYTE, stride, ptr);
				//ptr += csize;
				//glVertexPointer(2, GL_FLOAT, stride, ptr);

				ELEMENT_4UB_V2F *e = (ELEMENT_4UB_V2F *)pointer;

				for (int i=0; i<count; i++)
				{
					glColor4ub(e[i].r,e[i].g,e[i].b,e[i].a);
					glVertex2f(e[i].x,e[i].y);
				}

				break;
			}

			default:
				assert(0);
		}
	}
*/

	void glCube (Vector center, SINGLE xd, SINGLE yd, SINGLE zd)
	{
		#define LINE_DRAW(a, b) { glVertex3f(cube[a].x, cube[a].y, cube[a].z); glVertex3f(cube[b].x, cube[b].y, cube[b].z); }
		
		Vector cube[8];

		cube[0].x = -xd; cube[0].y =  yd; cube[0].z = -zd;
		cube[1].x =  xd; cube[1].y =  yd; cube[1].z = -zd;
		cube[2].x = -xd; cube[2].y =  yd; cube[2].z =  zd;
		cube[3].x =  xd; cube[3].y =  yd; cube[3].z =  zd;
		cube[4].x = -xd; cube[4].y = -yd; cube[4].z = -zd;
		cube[5].x =  xd; cube[5].y = -yd; cube[5].z = -zd;
		cube[6].x = -xd; cube[6].y = -yd; cube[6].z =  zd;
		cube[7].x =  xd; cube[7].y = -yd; cube[7].z =  zd;

		for (int i = 0; i < 8; i++)
		{
			cube[i] += center;
		}

		glBegin(GL_LINES);
		
		LINE_DRAW(0, 2); LINE_DRAW(2, 3); LINE_DRAW(3, 1); LINE_DRAW(1, 0);
		LINE_DRAW(4, 6); LINE_DRAW(6, 7); LINE_DRAW(7, 5); LINE_DRAW(5, 4);
		LINE_DRAW(2, 6); LINE_DRAW(3, 7); LINE_DRAW(0, 4); LINE_DRAW(1, 5);

		glEnd();
	}

}
