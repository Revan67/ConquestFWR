//---------------------------------------------------------------------------
/*
	CONTEXT.CPP

	Copyright (C) 1997 Digital Anvil, Inc.

	Created: October 1997

	Author: Paul Isaac
*/
//---------------------------------------------------------------------------

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "context.h"
#include "IDumpText.h"

#include <limits.h>
#include <stdio.h>	// sprintf()
#include <math.h>	// tan()

struct IDumpText * DUMP;

extern char Implementation[];

extern DrawMgr DRAW;

U32 GetDrawColor (const PIXELFORMATDESCRIPTOR *pix, byte r, byte g, byte b, byte a);

bool modify_misc = false;

void DrawPoints (int n, const VINDEX *output, _VERTEX *vScreen, int vCount);
void DrawLines (int n, const VINDEX *output, _VERTEX *vScreen, int vCount);
void DrawTriangles (int n, const VINDEX *output, _VERTEX *vScreen, int vCount);
void DrawQuads (int n, const VINDEX *output, _VERTEX *vScreen, int vCount);
void DrawPolygon (int n, const VINDEX *output, _VERTEX *vScreen, int vCount);

#define SUBMIT_BLOCKS (3*48)	// good number of polygons to start rendering mid-stream
//
// Note: it is better to break the render load into smaller blocks
// so the 3D card can be rendering while the CPU is doing geometry.
// This block size is arbitrary, but seems to work well.
//

//---------------------------------------------------------------------------
// GLOBALS
//---------------------------------------------------------------------------

int Verbosity = 1;			// how much info to send to OutputDebugString

// Temporary = use during Begin/End
VECTOR vEye[MAX_VERTS];		// vertex in camera space
VECTOR vClip[MAX_VERTS];	// vertex in clip space
uchar vClipFlag[MAX_VERTS];	// vb.clip_list
VECTOR	nEye[MAX_VERTS];	// normal in camera space
uchar vFog[MAX_VERTS];		// vertex fog values in case of per-vertex fogging.
VINDEX vChains[MAX_CHAINS];	// vertex reference chains
_VERTEX	vScreen[MAX_VERTS];	// vb.vlist

//---------------------------------------------------------------------------
// DEBUG
//---------------------------------------------------------------------------

void DebugPrint (char *fmt, ...)
{
	if (fmt)
	{
		char work[256];

		va_list va;
		va_start(va,fmt);
		vsprintf(work,fmt,va);
		va_end(va);

		if (DUMP)
			DUMP->debug_printf(work);
		else
			OutputDebugString(work);
	}
}

void DebugAlert (char *title, char *fmt, ...)
{
	OutputDebugString("Alert Box: ");
	if (title) OutputDebugString(title);
	OutputDebugString("\n");

	if (fmt)
	{
		char work[256];

		va_list va;
		va_start(va,fmt);
		vsprintf(work,fmt,va);
		va_end(va);

		if (DUMP)
			DUMP->alert_box(title, work);
		else
		{
			MessageBox(0,work,title,MB_OK);
			OutputDebugString(work);
		}
	}
}

//

bool DebugAlertYesNo(char * title, char * fmt, ...)
{
	OutputDebugString("Alert Box: ");
	if (title) OutputDebugString(title);
	OutputDebugString("\n");

	int result = IDYES;
	if (fmt)
	{
		char work[256];

		va_list va;
		va_start(va,fmt);
		vsprintf(work,fmt,va);
		va_end(va);

		if (DUMP)
		{
			DUMP->bomb(title, work);
			result = IDYES;
		}
		else
		{
			result = MessageBox(0,work,title,MB_YESNO);
			OutputDebugString(work);
		}
	}

	return (result == IDYES);
}


//---------------------------------------------------------------------------
// Render Statistics
//---------------------------------------------------------------------------

#define STATS_RESET()
#define STATS_VERTEX(n)
#define STATS_TRANSFORM(n)
#define STATS_PROJECT(n)
#define STATS_CLIP(n)
#define STATS_CULL(n)
#define STATS_DRAW(n) primitive_count += n;

//---------------------------------------------------------------------------
// Misc. OpenGL
//---------------------------------------------------------------------------

int TypeSize (GLenum type)
{
	int size;
	switch (type)
	{
	case GL_BYTE:
	case GL_UNSIGNED_BYTE:
		size = 1;
		break;

	case GL_SHORT:
	case GL_UNSIGNED_SHORT:
	case GL_2_BYTES:
		size = 2;
		break;

	case GL_3_BYTES:
		size = 3;
		break;

	case GL_INT:
	case GL_UNSIGNED_INT:
	case GL_FLOAT:
	case GL_4_BYTES:
		size = 4;
		break;

	default:
		size = 0;
		break;
	}
	return size;
}

EnableIndex LookupEnable (GLenum flag)
{
	switch (flag)
	{
		case GL_FOG:
			return ENABLE_FOG;

		case GL_DEPTH_TEST:
			return ENABLE_DEPTH_TEST;

		case GL_CULL_FACE:
			return ENABLE_CULL_FACE;

		case GL_TEXTURE_2D:
			return ENABLE_TEXTURE_2D;

		case GL_LIGHTING:
			return ENABLE_LIGHTING;

		case GL_LIGHT0:
			return ENABLE_LIGHT0;

		case GL_LIGHT1:
			return ENABLE_LIGHT1;

		case GL_LIGHT2:
			return ENABLE_LIGHT2;

		case GL_LIGHT3:
			return ENABLE_LIGHT3;

		case GL_LIGHT4:
			return ENABLE_LIGHT4;

		case GL_LIGHT5:
			return ENABLE_LIGHT5;

		case GL_LIGHT6:
			return ENABLE_LIGHT6;

		case GL_LIGHT7:
			return ENABLE_LIGHT7;

		case GL_SCISSOR_TEST:
			return ENABLE_SCISSOR_TEST;

		case GL_CULL_VERTEX:
			return ENABLE_CULL_VERTEX;

		case GL_BLEND:
			return ENABLE_BLEND;

		case GL_LINE_SMOOTH:
			return ENABLE_LINE_SMOOTH;

		case GL_DITHER:
			return ENABLE_DITHER;
	}
	return ENABLE_NONE;
}

//---------------------------------------------------------------------------
// GL_CONTEXT
//---------------------------------------------------------------------------

GL_CONTEXT::GL_CONTEXT (void)
{
	vertex_buffer.alloc(MAX_VERTS);
	reset();
}

GL_CONTEXT::~GL_CONTEXT (void)
{
	//draw_context?
	DebugPrint("GL: context destroyed.\n");
}

//---------------------------------------------------------------------------

void GL_CONTEXT::gl_error (GLenum code, const char *msg, const char *spec)
{
err = code;

	char *code_name;
	switch (code)
	{
	case GL_NO_ERROR:
		code_name = "GL_NO_ERROR"; break;
	case GL_INVALID_ENUM:
		code_name = "GL_INVALID_ENUM"; break;
	case GL_INVALID_VALUE:
		code_name = "GL_INVALID_VALUE"; break;
	case GL_INVALID_OPERATION:
		code_name = "GL_INVALID_OPERATION"; break;
	case GL_STACK_OVERFLOW:
		code_name = "GL_STACK_OVERFLOW"; break;
	case GL_STACK_UNDERFLOW:
		code_name = "GL_STACK_UNDERFLOW"; break;
	case GL_OUT_OF_MEMORY:
		code_name = "GL_OUT_OF_MEMORY"; break;
	default:
		code_name = "UNKNOWN?"; break;
	}

	if (msg == 0)
		msg = "UNKNOWN";
	if (spec == 0)
		spec = "?";
	err_string = msg;

	char full[256];
	sprintf(full,"GL: %s\n   gl_error(%s) %s\n",msg,code_name,spec);
	DebugPrint(full);
}

//---------------------------------------------------------------------------

	int get_word (char *arg, const char *&src)
	{
		while (*src == ' ')
			src++;

		int i = 0;
		while (*src && *src != ' ')
		{
			arg[i++] = *src++;
		}
		arg[i] = 0;

		return i;
	}

bool GL_CONTEXT::configure (const char *description)
{
// IF NO DESCRIPTION ASSUME WE'RE OKAY

	if (description)
	{
		char arg[256];
		const char *ptr = description;

	// SEE IF IMPLEMENTATION MATCHES DESCRIPTION 

		if (get_word(arg,ptr))
		{
			if (strcmp(arg,"?") == 0 ||
				strcmp(arg,Implementation) == 0)
			{
			// SUPPORT MISCELLANEOUS DISPLAY OPTIONS

				while (get_word(arg,ptr))
				{
					if (arg[0] == '-') // switch?
					{
						if (strcmp(arg+1,"mipmaps") == 0)
						{
							TextureMgr::use_mipmaps = false;
							DebugPrint("GL: MIPMAPS off\n");
						}
						else if (arg[1] == 'v')
						{
							Verbosity = atoi(arg+2);
							DebugPrint("GL: VERBOSE = %d\n",Verbosity);
						}
						else
						{
							// FUTURE: allow user to select which 3D device
							DebugPrint("GL: unknown switch = %s\n",arg);
						}
					}
				}
			}
			else
			{
				DebugPrint("GL: implementation declined. (%s)\n",Implementation);
				return false;
			}
		}
	}
	return true;
}

//---------------------------------------------------------------------------

void GL_CONTEXT::render (int num_chains, bool flush)
// Note: "flush" is true for non-reference submissions (ie. glVertex)
{
	// ASSUME: ROUNDED TO EVEN POLY-UNITS

	if (num_chains == 0)	// any work to do?
		return;

	if (active_list)
	{
	}
	else
	{
		int remainder = vertex_buffer.num_chains - num_chains;

		assert(remainder == 0); // new requirement!

		if (flush)
		{
			assert(vertex_buffer.vcount == vertex_buffer.num_chains);	// before clipping adds to vcount!
		}

//DebugPrint("render %d (%d)\n",num_chains,remainder);

		transform(vertex_buffer);

		uint *chain_list = vertex_buffer.vchain_list;

		DrawCmd(Begin_mode,vertex_buffer,num_chains,chain_list);

		vertex_buffer.flush_chains(num_chains);

		if (flush)
		{
			vertex_buffer.flush_vertices( vertex_buffer.vcount );
		}
	}
}

//---------------------------------------------------------------------------

void colorize_vertices (uint start, uint count, const COLOR_VECTOR &color)
{
	_VERTEX *v = vScreen + start;

	uint stop = start + count;

	DWORD c = color.packed();

	for (uint i=start; i<stop; i++, v++)
	{
		v->color = c;
	}
}

//---------------------------------------------------------------------------

bool GL_CONTEXT::perform (DrawList *list, uint &i)
{
	bool ok = false;

	uint *cmd_list = list->cmd_list;
	uint cmd = cmd_list[i];
	switch (cmd)
	{
		case CMD_TRANSLATE:
			{
				float x = ((float *)cmd_list)[i+1];
				float y = ((float *)cmd_list)[i+2];
				float z = ((float *)cmd_list)[i+3];
				cmd_translate(x,y,z);
				i += 4;
				ok = true;
			}
			break;

		case CMD_ENABLE:
			{
				EnableIndex f = (EnableIndex)cmd_list[i+1];
				cmd_enable(f);
				i += 2;
				ok = true;
			}
			break;

		case CMD_DISABLE:
			{
				EnableIndex f = (EnableIndex)cmd_list[i+1];
				cmd_disable(f);
				i += 2;
				ok = true;
			}
			break;

		case CMD_BEGIN:
			{
				GLenum type = (GLenum)cmd_list[i+1];

				Begin_mode = type;
				cmd_begin();

				int vcount = cmd_list[i+2];
				int chain_offset = cmd_list[i+3];

				// FUTURE: CMD_COLOR vs. glColor before glCall
				colorize_vertices(chain_offset,vcount, vcolor);

				VINDEX *vchain = list->vchain_list + chain_offset;
				DrawCmd(type,*(VertexBuffer *)list,vcount,vchain);
				i += 4;
				ok = true;

				Begin_mode = GL_INVALID_ENUM;
			}
			break;

		case CMD_BIND_TEXTURE:
			{
				int texture = cmd_list[i+1];
				cmd_bind_texture(texture);
				i += 2;
				ok = true;
			}
			break;

		default:
			return false;

	}
	return ok;
}

//---------------------------------------------------------------------------

//#define ZMETHOD // DISTRIBUTE 1/Z MORE EVENLY (accomodate 16-bit fixed-point zbuffers)


void GL_CONTEXT::build_vertices (VertexBuffer &vb, uint start, uint count, const VECTOR *vClip)
{
	if (count == 0)
		return;

	const VECTOR *src = vClip + start;
	const uchar * f = vFog + start;

	_VERTEX *v = vScreen + start;
#ifdef USE_VERT_STRUCT
	VBVertex * vsrc = vb.vert_list.ptr + start;
#else
	COLOR_VECTOR *c = (COLOR_VECTOR *)vb.color_list + start;
	TEX_VECTOR * t= (TEX_VECTOR *)vb.texcoord_list + start;
#endif

	uint stop = start + count;

	if (ProjectionMatrix.type == MATRIX_ORTHO)
	{
#ifdef USE_VERT_STRUCT
		for (uint i=start; i<stop; i++, src++, v++, vsrc++, f++)
#else
		for (uint i=start; i<stop; i++, src++, v++, c++, t++, f++)
#endif
		{
			if (vClipFlag[i] == 0) // project UNCLIPPED points
			{
				v->sx = src->x*h_scale + h_offset;
				v->sy = src->y*v_scale + v_offset;
				//float z = znear - src->z;
				//double w = 1.0 / z;
				float w = (1 + src->z) / 2.0;
				if (w == 1.0)
					w = 0.999;
				if (w == 0.0)
					w = 0.0001;
				v->rhw = w;				// (0.0001 to 0.999) used for perspective
				v->sz = w;				// (0 to 0.999) used for Z-Buffering
			}

#ifdef USE_VERT_STRUCT
			v->color = vsrc->color.packed();
#else
			v->color = c->packed();
#endif

			if (enable[ENABLE_FOG] && DRAW.use_vertex_fog)
			{
				v->specular = *f << 24;
			}

#ifdef USE_VERT_STRUCT
			v->tu = vsrc->texcoord.s;
			v->tv = vsrc->texcoord.t;
#else
			v->tu = t->s;
			v->tv = t->t;
#endif
		}
	}
	else
	{
		uchar *clip = vClipFlag + start;

#if ZMETHOD
		float m0 = 1.0 / zfar;
#endif

// pci - still working here!

		uint i=0;
		while(1)
		//for (uint i=0; i<count; )// i++, src++, v++, c++, t++)
		{
			//if (vClipFlag[i] == 0) // project UNCLIPPED points
			if (clip[i] == 0)
			{
				double w = 1.0 / src->w;

				//clip++;

#ifdef USE_VERT_STRUCT
				v->color = vsrc->color.packed();
#else
				v->color = c->packed();
#endif

				if (enable[ENABLE_FOG] && DRAW.use_vertex_fog)
				{
					v->specular = *f << 24;
				}

#ifdef USE_VERT_STRUCT
				v->tu = vsrc->texcoord.s;
				v->tv = vsrc->texcoord.t;
#else
				v->tu = t->s;
				v->tv = t->t;
#endif

#if ZMETHOD
				float zb;
				zb = m0*src->z;
#endif

#ifdef USE_VERT_STRUCT
				i++, vsrc++, f++;
#else
				i++; c++, t++, f++;
#endif
				v++; src++;

			// Note: delay use of divide result 'w'

				v[-1].sx = w * src[-1].x * h_scale + h_offset;
				v[-1].sy = w * src[-1].y * v_scale + v_offset;
#if ZMETHOD
				v[-1].sz = zb;		// (0 to 0.999) used for Z-Buffering
#else
				//float zb = (zfar / (zfar-znear)) * (1 - (znear/src->w));
			/*
				float zb = 1.0 - w * 100;
				if (zb < 0) zb = 0;
				v[-1].sz = zb;
			*/
				v[-1].sz = 1.0 - w;
#endif
				v[-1].rhw = w;		// (0 to 1.0) used for perspective

				if (i>=count)
					break;
			}
			else
			{
				//clip++;
#ifdef USE_VERT_STRUCT
				i++, vsrc++, f++;
#else
				i++, c++, t++, f++;
#endif
				v++; src++;

				if (i>=count)
					break;
			}
		}
	}
}

//---------------------------------------------------------------------------

void GL_CONTEXT::transform (VertexBuffer &vb)
{
	// fyi - uses global buffers = vClip, vWin

	int start = vb.start;
	int count = vb.vcount - start;

// TRANSFORM VERTICES FROM OBJECT TO EYE SPACE

	VECTOR * v_eye = vEye+start;
#ifdef USE_VERT_STRUCT
	transform_points(count,v_eye,vb.vert_list.ptr+start);
#else
	transform_points(count,v_eye,(const VECTOR *)vb.obj_list.ptr+start);
#endif

	if (enable[ENABLE_LIGHTING])
	{
	// transform normals to eye space.
#ifdef USE_VERT_STRUCT
		transform_normals(count, nEye+start, vb.vert_list.ptr + start);
		light_vertices(count, vb.vert_list.ptr+start, v_eye, nEye+start);
#else
		VECTOR * vNormal = vb.normal_list.ptr;
		transform_normals(count, nEye+start, (const VECTOR *) vNormal+start);
		COLOR_VECTOR * vColor = vb.color_list.ptr;
		light_vertices(count, vColor+start, v_eye, nEye+start);
#endif

	}

	if (enable[ENABLE_FOG] && DRAW.use_vertex_fog)
	{
	// compute fog value per vertex.
		fog_vertices(count, vFog+start, (const VECTOR *) v_eye);
	}

// PROJECT VERTICES EYE SPACE TO CLIP SPACE

	project_clip(count,vClipFlag+start,vClip+start,v_eye);

	if (all_clip != 0)
		return;

// PROJECT VERTICES TO 2D SCREEN POINTS
// SETUP FINAL VERTEX STRUCTURES

	build_vertices(vb,start,count,vClip);

	vb.start = start + count; // mark vertices as transformed!
}

//---------------------------------------------------------------------------

void GL_CONTEXT::build_active_light_list(void)
{
	active_lights = NULL;
	Light * prev = NULL;
	Light * l = lights;
	for (int i = 0; i < 8; i++, l++)
	{
		if (enable[ENABLE_LIGHT0 + i])
		{
			if (!active_lights)
			{
				active_lights = l;
			}

			if (prev)
			{
				prev->next = l;
			}

			prev = l;
			l->next = NULL;
		}
	}
}

//---------------------------------------------------------------------------

void GL_CONTEXT::light_vertices(int n, COLOR_VECTOR * dst, const VECTOR * src, const VECTOR * normal)
{
//
// Lights, vertices, and normals are all in eye space.
//
	int base_r = mat->emission.r + (global_ambient.r * mat->ambient.r) >> 8;
	int base_g = mat->emission.g + (global_ambient.g * mat->ambient.g) >> 8;
	int base_b = mat->emission.b + (global_ambient.b * mat->ambient.b) >> 8;
	int base_a = mat->diffuse.a;

	for (int i = 0; i < n; i++, dst++, src++, normal++)
	{
		int r = base_r;
		int g = base_g;
		int b = base_b;
		int a = base_a;

		Light * l = active_lights;
		while (l)
		{
			VECTOR L;
			float attenuation;
			float spot = 1.0;

			if (l->pos.w == 0.0)
			{
			// Directional light.
				L.x = l->dir.x;
				L.y = l->dir.y;
				L.z = l->dir.z;

				attenuation = 1.0;
			}
			else
			{
			// Positional light.
				L.x = l->pos.x - src->x;
				L.y = l->pos.y - src->y;
				L.z = l->pos.z - src->z;
				float d = (float) sqrt(L.x * L.x + L.y * L.y + L.z * L.z);
				if (d > 0.001F)
				{
					float invd = 1.0 / d;
					L.x *= invd;
					L.y *= invd;
					L.z *= invd;
				}

				attenuation = 1.0 / (l->kc + d * (l->kl + d * l->kq));
			}

			int ambient_r = (mat->ambient.r * l->ambient.r) >> 8;
			int ambient_g = (mat->ambient.g * l->ambient.g) >> 8;
			int ambient_b = (mat->ambient.b * l->ambient.b) >> 8;

			float dot = normal->x * L.x + normal->y * L.y + normal->z * L.z;

			if (dot <= 0.0)
			{
			// Faces away from light, gets ambient only.
				float t = attenuation * spot;
				r += t * ambient_r;
				g += t * ambient_g;
				b += t * ambient_b;
			}
			else
			{
				int diffuse_r = dot * ((mat->diffuse.r * l->diffuse.r) >> 8);
				int diffuse_g = dot * ((mat->diffuse.g * l->diffuse.g) >> 8);
				int diffuse_b = dot * ((mat->diffuse.b * l->diffuse.b) >> 8);

				float t = attenuation * spot;

				r += t * (ambient_r + diffuse_r);
				g += t * (ambient_g + diffuse_g);
				b += t * (ambient_b + diffuse_b);
			}

			l = l->next;
		}

		dst->r = __min(r, 255);
		dst->g = __min(g, 255);
		dst->b = __min(b, 255);
		dst->a = __min(a, 255);
	}
}

//

#ifdef USE_VERT_STRUCT
void GL_CONTEXT::light_vertices(int n, VBVertex * dst, const VECTOR * src, const VECTOR * normal)
{
//
// Lights, vertices, and normals are all in eye space.
//
	int base_r = mat->emission.r + (global_ambient.r * mat->ambient.r) >> 8;
	int base_g = mat->emission.g + (global_ambient.g * mat->ambient.g) >> 8;
	int base_b = mat->emission.b + (global_ambient.b * mat->ambient.b) >> 8;
	int base_a = mat->diffuse.a;

	for (int i = 0; i < n; i++, dst++, src++, normal++)
	{
		int r = base_r;
		int g = base_g;
		int b = base_b;
		int a = base_a;

		Light * l = active_lights;
		while (l)
		{
			VECTOR L;
			float attenuation;
			float spot = 1.0;

			if (l->pos.w == 0.0)
			{
			// Directional light.
				L.x = l->dir.x;
				L.y = l->dir.y;
				L.z = l->dir.z;

				attenuation = 1.0;
			}
			else
			{
			// Positional light.
				L.x = l->pos.x - src->x;
				L.y = l->pos.y - src->y;
				L.z = l->pos.z - src->z;
				float d = (float) sqrt(L.x * L.x + L.y * L.y + L.z * L.z);
				if (d > 0.001F)
				{
					float invd = 1.0 / d;
					L.x *= invd;
					L.y *= invd;
					L.z *= invd;
				}

				attenuation = 1.0 / (l->kc + d * (l->kl + d * l->kq));
			}

			int ambient_r = (mat->ambient.r * l->ambient.r) >> 8;
			int ambient_g = (mat->ambient.g * l->ambient.g) >> 8;
			int ambient_b = (mat->ambient.b * l->ambient.b) >> 8;

			float dot = normal->x * L.x + normal->y * L.y + normal->z * L.z;

			if (dot <= 0.0)
			{
			// Faces away from light, gets ambient only.
				float t = attenuation * spot;
				r += t * ambient_r;
				g += t * ambient_g;
				b += t * ambient_b;
			}
			else
			{
				int diffuse_r = dot * ((mat->diffuse.r * l->diffuse.r) >> 8);
				int diffuse_g = dot * ((mat->diffuse.g * l->diffuse.g) >> 8);
				int diffuse_b = dot * ((mat->diffuse.b * l->diffuse.b) >> 8);

				float t = attenuation * spot;

				r += t * (ambient_r + diffuse_r);
				g += t * (ambient_g + diffuse_g);
				b += t * (ambient_b + diffuse_b);
			}

			l = l->next;
		}

		dst->color.r = __min(r, 255);
		dst->color.g = __min(g, 255);
		dst->color.b = __min(b, 255);
		dst->color.a = __min(a, 255);
	}
}
#endif

//---------------------------------------------------------------------------

void GL_CONTEXT::fog_vertices(int n, uchar * dst, const VECTOR * src)
{
	uchar * fog = dst;

	switch (fog_mode)
	{
		case GL_LINEAR:
		{
			float d = 1.0 / (fog_end - fog_start);
			for (int i = 0; i < n; i++, dst++, src++)
			{
				float f = (fog_end - fabs(src->z)) * d;
			// Convert to int, clamp.
				f = __max(0, __min(f, 1));
				*dst = int(f * 255);
			}
			break;
		}
		case GL_EXP:
		{
			float d = -fog_density;
			for (int i = 0; i < n; i++, dst++, src++)
			{
				float f = exp(d * fabs(src->z));
				f = __max(0, __min(f, 1));
				*dst = int(f * 255);
			}
			break;
		}
		case GL_EXP2:
		{
			float d = -(fog_density * fog_density);
			for (int i = 0; i < n; i++, dst++, src++)
			{
				float z = fabs(src->z);
				float f = exp(d * z * z);
				f = __max(0, __min(f, 1));
				*dst = int(f * 255);
			}
			break;
		}
	}
}

//---------------------------------------------------------------------------

void GL_CONTEXT::set_clear_color (float r, float g, float b, float a)
{
	clear_color.r = clamp_color(r);
	clear_color.g = clamp_color(g);
	clear_color.b = clamp_color(b);
	clear_color.a = clamp_color(a);

	clear_rgb = screen_pixel_format.compute(clear_color.r, clear_color.g, clear_color.b, clear_color.a);
}

//---------------------------------------------------------------------------

void GL_CONTEXT::set_clear_depth (float d)
{
	if (d < 0)
		d = 0;
	else if (d > 1)
		d = 1;

	clear_depth = d;

	U32 zz = d*0xFFFF;
	zz = (zz << 16) | zz;	// two 16-bit values

	clear_z = zz;
}

//---------------------------------------------------------------------------

void GL_CONTEXT::cmd_begin (void)
{
	switch (Begin_mode)
	{
		// ROUND TO EVEN POLY-UNITS

		case GL_POINTS:		submit_count = SUBMIT_BLOCKS; break;
		case GL_LINES:		submit_count = (SUBMIT_BLOCKS/2)*2; break;
		case GL_TRIANGLES:	submit_count = (SUBMIT_BLOCKS/3)*3; break;
		case GL_QUADS:		submit_count = (SUBMIT_BLOCKS/4)*4; break;

		default:			submit_count = vertex_buffer.max; break;
	}
		
	if (!in_scene)
	{
		begin_scene();
		in_scene = true;
	}

// UPDATE STATES THAT HAVE CHANGED

	// fyi - for some reason this always has to happen?
	if (true) //modify_misc)
	{
		if (line_antialias)
		{
			//draw_context.set_render_state(D3DRS_ANTIALIAS, D3DANTIALIAS_SORTINDEPENDENT);
			draw_context.lpD3DDevice->SetRenderState(D3DRS_EDGEANTIALIAS, TRUE);
			draw_context.lpD3DDevice->SetRenderState(D3DRS_ANTIALIAS, D3DANTIALIAS_SORTINDEPENDENT);
		}
		else
		{
			//draw_context.set_render_state(D3DRS_ANTIALIAS, D3DANTIALIAS_NONE);
			draw_context.lpD3DDevice->SetRenderState(D3DRS_EDGEANTIALIAS, FALSE);
			draw_context.lpD3DDevice->SetRenderState(D3DRS_ANTIALIAS, D3DANTIALIAS_NONE);
		}

		modify_misc = false;
	}

	if (modify_lighting)
	{
		setup_lighting();
		modify_lighting = false;
	}

	if (modify_pixels)
	{
		setup_pixels();
		modify_pixels = false;
	}

	if (modify_fog)
	{
		setup_fog();
		modify_fog = false;
	}

	TextureMgr::set_active(enable[ENABLE_TEXTURE]);

	TextureMgr::begin();
}

//---------------------------------------------------------------------------

//---------------------------------------------------------------------------

	#define LINTERP(T,A,B)   ((A)+(T)*((B)-(A)))

	void interpolate_material (VertexBuffer &vb, uint dst, float t, int in, int out)
	{
		if (true) // clip color?
		{
#ifdef USE_VERT_STRUCT
			COLOR_VECTOR *v = &(vb.vert_list.ptr->color);
			COLOR_VECTOR *v0 = &(vb.vert_list.ptr + in)->color;
			COLOR_VECTOR *v1 = &(vb.vert_list.ptr + out)->color;
#else
			COLOR_VECTOR *v = vb.color_list;
			COLOR_VECTOR *v0 = v + in;
			COLOR_VECTOR *v1 = v + out;
#endif
			v += dst;

			v->r = LINTERP(t, v0->r, v1->r);
			v->g = LINTERP(t, v0->g, v1->g);
			v->b = LINTERP(t, v0->b, v1->b);
			v->a = LINTERP(t, v0->a, v1->a);
		}

		if (true) // clip texture?
		{
#ifdef USE_VERT_STRUCT
			TEX_VECTOR *v = &(vb.vert_list.ptr->texcoord);
			TEX_VECTOR *v0 = &(vb.vert_list.ptr + in)->texcoord;
			TEX_VECTOR *v1 = &(vb.vert_list.ptr + out)->texcoord;
#else
			TEX_VECTOR *v = vb.texcoord_list;
			TEX_VECTOR *v0 = v + in;
			TEX_VECTOR *v1 = v + out;
#endif
			v += dst;

			v->s = LINTERP(t, v0->s, v1->s);
			v->t = LINTERP(t, v0->t, v1->t);
			v->r = LINTERP(t, v0->r, v1->r);
			v->q = LINTERP(t, v0->q, v1->q);
		}
	}

//---------------------------------------------------------------------------

int GL_CONTEXT::PolySide (int vcount, _VERTEX **vlist)
// return 0 if front facing, 1 if back facing
{
	if (vcount >= 3) // filled polygons
	{
		if (enable[ENABLE_CULL_FACE]) // post culling of back faces
		{
			_VERTEX *v0 = vlist[0];
			_VERTEX *v1 = vlist[1];
			_VERTEX *v2 = vlist[2];

			// compute screen area of polygon
			float a = v0->sx*v1->sy - v1->sx*v0->sy +
						v1->sx*v2->sy - v2->sx*v1->sy + 
						v2->sx*v0->sy - v0->sx*v2->sy;

			if (front_face != GL_CW)
				a = -a;

			return (a < 0);	// 1 = polygon is back facing
		}
	}

	return 0; // FRONT side
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

bool GL_CONTEXT::PolyClip (int vcount, const VertexBuffer &vb, const VINDEX *vchain)
{
	int idx = vchain[0];
	int all = vClipFlag[idx];
	int any = all;
	for (int i=1; i<vcount; i++)
	{
		idx = vchain[i];
		all &= vClipFlag[idx];
		any |= vClipFlag[idx];
	}
	if (all != 0)
		return true;	// polygon is totally off ONE side

	if (any)
	{
		return true; // FUTURE: render clipped polygons
	}

	return false; // totally unclipped
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

int GL_CONTEXT::compute_side (_VERTEX *v0, _VERTEX *v1, _VERTEX *v2)
{
	// compute screen area of polygon

	float x0 = v0->sx;
	float y0 = v0->sy;
	float x1 = v1->sx;
	float y1 = v1->sy;
	float x2 = v2->sx;
	float y2 = v2->sy;

	float a = (x1-x0)*(y2-y0) - (x2-x0)*(y1-y0);

	if (front_face != GL_CW)
		a = -a;

	return (a < 0); // polygon is front/back facing
}

//---------------------------------------------------------------------------

#define LINE_CLIP() \
{															\
	int tmp = 0;											\
	output = (output == tmp_chains)	? tmp_poly : tmp_chains;\
	int v0 = input[0];										\
	int v1 = input[1];										\
	int c = OUTSIDE(v0) + OUTSIDE(v1)*2;					\
	if (c == 0)	/* Neither clipped? */						\
	{														\
		output[tmp++] = v0;									\
		output[tmp++] = v1;									\
	}														\
	else if (c == 3) /* Both clipped? */					\
	{														\
		return 0;											\
	}														\
	else /* partially clipped */							\
	{														\
		int NEW = more++;									\
		int in,out;											\
		if (c == 1)	/* Current clipped, next not */			\
		{													\
			in = v1;										\
			out = v0;										\
			output[tmp++] = NEW;							\
			output[tmp++] = in;								\
		}													\
		else /* Next clipped, current not */				\
		{													\
			in = v0;										\
			out = v1;										\
			output[tmp++] = in;								\
			output[tmp++] = NEW;							\
		}													\
		INTERP;												\
		interpolate_material(vb,NEW,t,in,out);				\
		vClipFlag[NEW] = 0;								\
	}														\
	count = tmp;											\
	input = output;											\
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

int clip_line (VertexBuffer &vb, VINDEX *tmp_chains, VINDEX *chains)
{
	int count = 2;

// ANALYZE CLIPPING COMMONALITIES

	int any;
	int all;
	{
		int clip = vClipFlag[ chains[0] ];
		all = clip;
		any = clip;
		clip = vClipFlag[ chains[1] ];
		all &= clip;
		any |= clip;
	}

	if (any == 0)	// anything to clip?
	{
		return -1;	// polygon is totally un-clipped
	}

	STATS_CLIP(1);

	if (all != 0)
	{
		return 0;	// polygon is totally off ONE side
	}

// Polygon partially clipped

	#define CX(i) vClip[i].x
	#define CY(i) vClip[i].y
	#define CZ(i) vClip[i].z
	#define CW(i) vClip[i].w

	VINDEX tmp_poly[MAX_POLY_SIDES];

	VINDEX *input = chains;
	VINDEX *output = 0;

	int more = vb.vcount;

// Clip against -Z side

	#define OUTSIDE(i) (CZ(i) < -CW(i))

	#define INTERP \
		double dz = CZ(out) - CZ(in);			\
		double dw = CW(out) - CW(in);			\
		double t = -(CZ(in) + CW(in)) / (dw+dz);\
		CX(NEW) = LINTERP(t,CX(in),CX(out));	\
		CY(NEW) = LINTERP(t,CY(in),CY(out));	\
		CZ(NEW) = CZ(in) + t * dz;				\
		CW(NEW) = CW(in) + t * dw;				\

	if (any & CLIP_NEAR)
	{
		LINE_CLIP();

        // Everything can change behind the near plane!
		any |= CLIP_LEFT|CLIP_RIGHT|CLIP_TOP|CLIP_BOTTOM;
	}

	#undef INTERP
	#undef OUTSIDE

// Clip against +Z side

	#define OUTSIDE(i) (CZ(i) > CW(i))

	#define INTERP \
		double dz = CZ(out) - CZ(in);			\
		double dw = CW(out) - CW(in);			\
		double t = (CZ(in) - CW(in)) / (dw-dz);	\
		CX(NEW) = LINTERP(t,CX(in),CX(out));	\
		CY(NEW) = LINTERP(t,CY(in),CY(out));	\
		CZ(NEW) = CZ(in) + t * dz;				\
		CW(NEW) = CW(in) + t * dw;

	if (any & CLIP_FAR)
	{
		LINE_CLIP();
	}

	#undef INTERP
	#undef OUTSIDE

// Clip against +X side

	#define OUTSIDE(i) (CX(i) > CW(i))

	#define INTERP \
		double dx = CX(out) - CX(in);			\
		double dw = CW(out) - CW(in);			\
		double t = (CX(in) - CW(in)) / (dw-dx);	\
		float new_w = CW(in) + t*dw;			\
		CX(NEW) = new_w;						\
		CY(NEW) = LINTERP(t,CY(in),CY(out));	\
		CZ(NEW) = LINTERP(t,CZ(in),CZ(out));	\
		CW(NEW) = new_w;

	if (any & CLIP_RIGHT)
	{
		LINE_CLIP();
	}

	#undef INTERP
	#undef OUTSIDE

// Clip against -X side

	#define OUTSIDE(i) (CX(i) < -CW(i))

	#define INTERP \
		double dx = CX(out) - CX(in);			\
		double dw = CW(out) - CW(in);			\
		double t = -(CX(in) + CW(in)) / (dw+dx);\
		float new_w = CW(in) + t*dw;			\
		CX(NEW) = -new_w;						\
		CY(NEW) = LINTERP(t,CY(in),CY(out));	\
		CZ(NEW) = LINTERP(t,CZ(in),CZ(out));	\
		CW(NEW) = new_w;

	if (any & CLIP_LEFT)
	{
		LINE_CLIP();
	}

	#undef INTERP
	#undef OUTSIDE

// Clip against +Y side

	#define OUTSIDE(i) (CY(i) > CW(i))

	#define INTERP \
		double dy = CY(out) - CY(in);			\
		double dw = CW(out) - CW(in);			\
		double t = (CY(in) - CW(in)) / (dw-dy);	\
		float new_w = CW(in) + t*dw;			\
		CX(NEW) = LINTERP(t,CX(in),CX(out));	\
		CY(NEW) = new_w;						\
		CZ(NEW) = LINTERP(t,CZ(in),CZ(out));	\
		CW(NEW) = new_w;

	if (any & CLIP_TOP)
	{
		LINE_CLIP();
	}

	#undef INTERP
	#undef OUTSIDE

// Clip against -Y side

	#define OUTSIDE(i) (CY(i) < -CW(i))

	#define INTERP \
		double dy = CY(out) - CY(in);			\
		double dw = CW(out) - CW(in);			\
		double t = -(CY(in) + CW(in)) / (dw+dy);\
		float new_w = CW(in) + t*dw;			\
		CX(NEW) = LINTERP(t,CX(in),CX(out));	\
		CY(NEW) = -new_w;						\
		CZ(NEW) = LINTERP(t,CZ(in),CZ(out));	\
		CW(NEW) = new_w;

	if (any & CLIP_BOTTOM)
	{
		LINE_CLIP();
	}

	#undef INTERP
	#undef OUTSIDE

// FINALIZE OUTPUT LIST

	assert(output != 0);	// must be clipped!
	assert(count == 2);		// must be a line!

	if (output != tmp_chains)
	{
		tmp_chains[0] = output[0];
		tmp_chains[1] = output[1];
	}

	vb.vcount = more;

	return count; // partially clipped
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

#define GENERAL_CLIP() \
{															\
	int tmp = 0;											\
	output = (output == tmp_chains)	? tmp_poly : tmp_chains;\
	int v = count-1;										\
	for (int vn=0; vn<count; v=vn++)						\
	{														\
		int v0 = input[v];									\
		int v1 = input[vn];									\
		int c = OUTSIDE(v0) + OUTSIDE(v1)*2;				\
		if (c == 0)	/* Neither clipped */					\
		{													\
			output[tmp++] = input[vn];						\
		}													\
		else if (c == 3)									\
		{													\
			/* Both clipped. */								\
		}													\
		else												\
		{													\
			int NEW = more++;								\
			output[tmp++] = NEW;							\
			vClipFlag[NEW] = 0;							\
			int in,out;										\
			if (c == 1)	/* Current clipped, next not */		\
			{												\
				in = input[vn];								\
				out = input[v];								\
				output[tmp++] = in;							\
			}												\
			else /* Next clipped, current not */			\
			{												\
				in = input[v];								\
				out = input[vn];							\
			}												\
			INTERP;											\
			interpolate_material(vb,NEW,t,in,out);			\
		}													\
	}														\
	count = tmp; \
	input = output; \
}


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -


int clip_poly (VertexBuffer &vb, int count, VINDEX *tmp_chains, VINDEX *chains)
{
// ANALYZE CLIPPING COMMONALITIES

	int any = vClipFlag[ chains[0] ];
	int all = any;
	for (int i=1; i<count; i++)
	{
		int clip = vClipFlag[ chains[i] ];
		all &= clip;
		any |= clip;
	}

	if (any == 0)	// anything to clip?
	{
		return -1;	// polygon is totally un-clipped
	}

	STATS_CLIP(1);

	if (all != 0)
	{
		return 0;	// polygon is totally off ONE side
	}

// Polygon partially clipped

	#define CX(i) vClip[i].x
	#define CY(i) vClip[i].y
	#define CZ(i) vClip[i].z
	#define CW(i) vClip[i].w

	VINDEX tmp_poly[MAX_POLY_SIDES];

	VINDEX *input = chains;
	VINDEX *output = 0;

	int more = vb.vcount;

// Clip against -Z side

	#define OUTSIDE(i) (CZ(i) < -CW(i))

	// WARNING: rearranging INTERP's inards fixed a compile bug in the Release version! (pci)
	#define INTERP \
		double dz = CZ(out) - CZ(in);			\
		double dw = CW(out) - CW(in);			\
		double t = -(CZ(in) + CW(in)) / (dw+dz);\
		CX(NEW) = LINTERP(t,CX(in),CX(out));	\
		CY(NEW) = LINTERP(t,CY(in),CY(out));	\
		CZ(NEW) = CZ(in) + t * dz;				\
		CW(NEW) = CW(in) + t * dw;				\

	if (any & CLIP_NEAR)
	{
		GENERAL_CLIP();
		// Everything can change behind the near plane!
		any |= CLIP_LEFT|CLIP_RIGHT|CLIP_TOP|CLIP_BOTTOM;
	}

	#undef INTERP
	#undef OUTSIDE

// Clip against +Z side

	#define OUTSIDE(i) (CZ(i) > CW(i))

	#define INTERP \
		double dz = CZ(out) - CZ(in);			\
		double dw = CW(out) - CW(in);			\
		double t = (CZ(in) - CW(in)) / (dw-dz);	\
		CX(NEW) = LINTERP(t,CX(in),CX(out));	\
		CY(NEW) = LINTERP(t,CY(in),CY(out));	\
		CZ(NEW) = CZ(in) + t * dz;				\
		CW(NEW) = CW(in) + t * dw;

	if (any & CLIP_FAR)
	{
		GENERAL_CLIP();
	}

	#undef INTERP
	#undef OUTSIDE

// Clip against +X side

	#define OUTSIDE(i) (CX(i) > CW(i))

	#define INTERP \
		double dx = CX(out) - CX(in);			\
		double dw = CW(out) - CW(in);			\
		double t = (CX(in) - CW(in)) / (dw-dx);	\
		float new_w = CW(in) + t*dw;			\
		CX(NEW) = new_w;						\
		CY(NEW) = LINTERP(t,CY(in),CY(out));	\
		CZ(NEW) = LINTERP(t,CZ(in),CZ(out));	\
		CW(NEW) = new_w;

	if (any & CLIP_RIGHT)
	{
		GENERAL_CLIP();
	}

	#undef INTERP
	#undef OUTSIDE

// Clip against -X side

	#define OUTSIDE(i) (CX(i) < -CW(i))

	#define INTERP \
		double dx = CX(out) - CX(in);			\
		double dw = CW(out) - CW(in);			\
		double t = -(CX(in) + CW(in)) / (dw+dx);\
		float new_w = CW(in) + t*dw;			\
		CX(NEW) = -new_w;						\
		CY(NEW) = LINTERP(t,CY(in),CY(out));	\
		CZ(NEW) = LINTERP(t,CZ(in),CZ(out));	\
		CW(NEW) = new_w;

	if (any & CLIP_LEFT)
	{
		GENERAL_CLIP();
	}

	#undef INTERP
	#undef OUTSIDE

// Clip against +Y side

	#define OUTSIDE(i) (CY(i) > CW(i))

	#define INTERP \
		double dy = CY(out) - CY(in);			\
		double dw = CW(out) - CW(in);			\
		double t = (CY(in) - CW(in)) / (dw-dy);	\
		float new_w = CW(in) + t*dw;			\
		CX(NEW) = LINTERP(t,CX(in),CX(out));	\
		CY(NEW) = new_w;						\
		CZ(NEW) = LINTERP(t,CZ(in),CZ(out));	\
		CW(NEW) = new_w;

	if (any & CLIP_TOP)
	{
		GENERAL_CLIP();
	}

	#undef INTERP
	#undef OUTSIDE

// Clip against -Y side

	#define OUTSIDE(i) (CY(i) < -CW(i))

	#define INTERP \
		double dy = CY(out) - CY(in);			\
		double dw = CW(out) - CW(in);			\
		double t = -(CY(in) + CW(in)) / (dw+dy);\
		float new_w = CW(in) + t*dw;			\
		CX(NEW) = LINTERP(t,CX(in),CX(out));	\
		CY(NEW) = -new_w;						\
		CZ(NEW) = LINTERP(t,CZ(in),CZ(out));	\
		CW(NEW) = new_w;

	if (any & CLIP_BOTTOM)
	{
		GENERAL_CLIP();
	}

	#undef INTERP
	#undef OUTSIDE

// FINALIZE OUTPUT LIST

	assert(output != 0);

	if (output != tmp_chains)
	{
		memcpy(tmp_chains,output,count*sizeof(output[0]));
	}

	vb.vcount = more;

	return count; // partially clipped
}

//---------------------------------------------------------------------------

void GL_CONTEXT::DrawCmd (GLenum type, VertexBuffer &vb, int vcount, const VINDEX *vchain)
// vb->vlist[ vchain[0 .. vcount-1] ]
{
	if (all_clip != 0)
		return;

	switch (type)
	{
	/////////////////////////////////////////////////////////////////////
	// 1-point
	/////////////////////////////////////////////////////////////////////

		case GL_POINTS:
			{
				VINDEX *input = (VINDEX *)vchain; // this won't be modified!
				int n = vcount;

				VINDEX *output = input;

			// OPTIONAL - CLIP POLYGONS TO VIEW VOLUME

				if (any_clip)
				{
					output = vChains; // it should be safe to input/output same list

					int start = vb.vcount;

					int t = 0; // num entries in tmp_chains[]

					for (int i=0; i<n; i+=1)
					{
						int p = input[i];
						if (vClipFlag[p] == 0) // unclipped
						{
							if (i != t || output != input)
							{
								output[t+0] = input[i+0];
							}
							t += 1;
						}
						else
						{
							STATS_CLIP(1);
						}
					}
					n = t;
					input = output;
				}

			// DRAW PRIMITIVES

				if (n <= 0) // anything to draw?
					return;

				DrawPoints(n, output, vScreen, vb.vcount);
				STATS_DRAW(n);
			}
			break;

	/////////////////////////////////////////////////////////////////////
	// 2-points
	/////////////////////////////////////////////////////////////////////

		case GL_LINES:
			{
				VINDEX *input = (VINDEX *)vchain; // this won't be modified!
				int n = vcount;

				VINDEX *output = input;

			// OPTIONAL - CLIP POLYGONS TO VIEW VOLUME

				if (any_clip)
				{
					output = vChains; // it should be safe to input/output same list

					int start = vb.vcount;

					int t = 0; // num entries in tmp_chains[]

					for (int i=0; i<n-1; i+=2)
					{
						VINDEX poly[2];
						int c = clip_line(vb,poly,input+i);
						if (c == -1) // unclipped
						{
							if (i != t || output != input)
							{
								output[t+0] = input[i+0];
								output[t+1] = input[i+1];
							}
							t += 2;
						}
						else if (c != 0) // clipped fragment
						{
							assert(c==2);			// clipped lines are also lines!
							output[t++] = poly[0];
							output[t++] = poly[1];
						}
					}
					n = t;
					input = output;

					int new_count = vb.vcount - start;
					if (new_count > 0)
					{
						// FUTURE: ignore vClipFlag
						build_vertices(vb,start,new_count,vClip);
					}
				}

			// DRAW PRIMITIVES

				if (n <= 0) // anything to draw?
					return;

				DrawLines(n,output,vScreen,vb.vcount);
				STATS_DRAW(n/2);
			}
			break;

		case GL_LINE_LOOP:
			{
				if (vcount < 2)
					return;

				// reconnect to begining
				VINDEX tmp_chains[2];
				tmp_chains[0] = vchain[0];
				tmp_chains[1] = vchain[vcount-1];

				DrawCmd( GL_LINES, vb, (vcount) & (0xFFFFFFFE), vchain );
				DrawCmd( GL_LINES, vb, (vcount-1) & (0xFFFFFFFE) , vchain+1 );
				DrawCmd( GL_LINES, vb, 2, tmp_chains );
			}
			break;

		case GL_LINE_STRIP:
			{
				if (vcount < 2)
					return;

				DrawCmd( GL_LINES, vb, (vcount) & (0xFFFFFFFE), vchain );
				DrawCmd( GL_LINES, vb, (vcount-1) & (0xFFFFFFFE) , vchain+1 );
			}
			break;

	/////////////////////////////////////////////////////////////////////
	// 3-sided
	/////////////////////////////////////////////////////////////////////

		case GL_TRIANGLES:
			{
				VINDEX *input = (VINDEX *)vchain; // this won't be modified!
				int n = vcount;

				VINDEX *output = input;

			// OPTIONAL - REMOVE BACK FACING POLYGONS FROM DRAW CHAINS

			// Unfortunately, we have to do clipping before culling.
			// OPTIONAL - CLIP POLYGONS TO VIEW VOLUME
				if (any_clip)
				{
					output = vChains;

					int start = vb.vcount;

					int t = 0; // num entries in tmp_chains[]

					for (int i=0; i<n-2; i+=3)
					{
						VINDEX poly[MAX_POLY_SIDES];
						int c = clip_poly(vb,3,poly,input+i);
						if (c == -1) // unclipped
						{
							if (i != t || output != input)
							{
								output[t+0] = input[i+0];
								output[t+1] = input[i+1];
								output[t+2] = input[i+2];
							}
							t += 3;
						}
						else if (c > 0)
						{
							// convert N-side to Triangles
							for (int j=2; j<c; j++)
							{
								output[t+0] = poly[0];
								output[t+1] = poly[j-1];
								output[t+2] = poly[j+0];
								t+= 3;
							}
						}
					}
					n = t;
					input = output;

					int new_count = vb.vcount - start;
					if (new_count > 0)
					{
						// FUTURE: ignore vClipFlag
						build_vertices(vb,start,new_count,vClip);
					}
				}

				if (enable[ENABLE_CULL_FACE])
				{
				// ASSUMES 2n < MAX_VERTS.
					output = vChains + n;

					int t = 0; // num entries in tmp_chains[]

					for (int i=0; i<n-2; i+=3)
					{
						_VERTEX *v0 = vScreen + input[i+0];
						_VERTEX *v1 = vScreen + input[i+1];
						_VERTEX *v2 = vScreen + input[i+2];

						int side = compute_side(v0,v1,v2);

						if (!cull_face[side])
						{
							//FUTURE: temp_flags[idx] |= 0x100; // side = FRONT/BACK
							output[t++] = input[i+0];
							output[t++] = input[i+1];
							output[t++] = input[i+2];
						}
						else
						{
							STATS_CULL(1);
						}
					}
					n = t;
					input = output;
				}

			// DRAW PRIMITIVES

				if (n <= 0) // anything to draw?
					return;

				DrawTriangles(n,output,vScreen,vb.vcount);
				STATS_DRAW(n/3);
			}
			break;


		case GL_TRIANGLE_STRIP:
			break;

		case GL_TRIANGLE_FAN:
			break;

	/////////////////////////////////////////////////////////////////////
	// 4 sided
	/////////////////////////////////////////////////////////////////////

		case GL_QUADS:
			{
				VINDEX *input = (VINDEX *)vchain; // this won't be modified!
				int n = vcount;

				VINDEX *output = input;

			// OPTIONAL - REMOVE BACK FACING POLYGONS FROM DRAW CHAINS

				if (enable[ENABLE_CULL_FACE]) // post culling of back faces
				{
					output = vChains;

					int t = 0; // num entries in tmp_chains[]

					for (int i=0; i<n-3; i+=4)
					{
						_VERTEX *v0 = vScreen + input[i+0];
						_VERTEX *v1 = vScreen + input[i+1];
						_VERTEX *v2 = vScreen + input[i+2];

						int side = compute_side(v0,v1,v2);

						if (!cull_face[side])
						{
							//FUTURE: temp_flags[idx] |= 0x100; // side = FRONT/BACK
							output[t+0] = input[i+0];
							output[t+1] = input[i+1];
							output[t+2] = input[i+2];
							output[t+3] = input[i+3];
							t += 4;
						}
						else
						{
							STATS_CULL(1);
						}
					}
					n = t;
					input = output;
				}

			// OPTIONAL - CLIP POLYGONS TO VIEW VOLUME

				if (any_clip)
				{
					output = vChains; // it should be safe to input/output same list

					int start = vb.vcount;

					int t = 0; // num entries in tmp_chains[]

					for (int i=0; i<n-3; i+=4)
					{
						VINDEX poly[MAX_POLY_SIDES];
						int c = clip_poly(vb,4,poly,input+i);
						if (c == -1) // unclipped
						{
							if (i != t || output != input)
							{
								output[t+0] = input[i+0];
								output[t+1] = input[i+1];
								output[t+2] = input[i+2];
								output[t+3] = input[i+3];
							}
							t += 4;
						}
						else if (c > 0)
						{
							// convert N-sides to Quads
							for (int j=3; j<c; j+=2)
							{
								output[t++] = poly[0];
								output[t++] = poly[j-2];
								output[t++] = poly[j-1];
								output[t++] = poly[j-0];
							}
							if (j-1<c) // unclosed?
							{
								// TRI = SIMULATED QUAD
								output[t++] = poly[0];
								output[t++] = poly[j-2];
								output[t++] = poly[j-1];
								output[t++] = poly[0];
							}
						}
					}
					n = t;
					input = output;

					int new_count = vb.vcount - start;
					if (new_count > 0)
					{
						// FUTURE: ignore vClipFlag
						build_vertices(vb,start,new_count,vClip);
					}
				}

			// DRAW PRIMITIVES

				if (n <= 0) // anything to draw?
					return;

				DrawQuads(n,output,vScreen,vb.vcount);
				STATS_DRAW(n/2);	// really drawing triangles.
			}
			break;

		case GL_QUAD_STRIP:
			break;

	/////////////////////////////////////////////////////////////////////
	// N-sided
	/////////////////////////////////////////////////////////////////////

		case GL_POLYGON:
			{
				if (vcount < 3)
					break;

				if (PolyClip(vcount,vb,vchain))
				{
					STATS_CLIP(1);
					break;
				}

				_VERTEX *v[3];

				v[0] = vScreen + vchain[0];
				v[1] = vScreen + vchain[1];
				v[2] = vScreen + vchain[2];

				if (enable[ENABLE_CULL_FACE]) // post culling of back faces
				{
					int side = PolySide(3, v);
					
					if (cull_face[side])
					{
						STATS_CULL(1);
						break;
					}
				}
				// FUTURE: GL_FILL, GL_LINE

				DrawPolygon(vcount,vchain,vScreen,vb.vcount);
				STATS_DRAW(vcount - 2);	// num triangles drawn.
			}
			break;

	} // switch
}

//---------------------------------------------------------------------------

	void GL_CONTEXT::update_palette (void)
	{
		PIXELFORMATDESCRIPTOR *pix = DRAW.get_format(DRAW.current_format);

		for (int i=0; i<256; i++)
		{
			uchar r = pixel_map[i*3 + 0];
			uchar g = pixel_map[i*3 + 1];
			uchar b = pixel_map[i*3 + 2];

			native_palette[i] = GetDrawColor(pix,r,g,b,0);
		}
	}

//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// GL Methods
//---------------------------------------------------------------------------

struct SYSTEM
{
	GL_CONTEXT *active_context;

	int num_contexts;
	GL_CONTEXT *context[8];

	SYSTEM (void)
	{
		active_context = 0;
		num_contexts = 0;
	}

	~SYSTEM (void)
	{
		// free context list?
	}

	GL_CONTEXT *find_context (HDC hDC)
	{
		for (int i=0; i<num_contexts; i++)
		{
			if (context[i]->hDC == hDC)
				return context[i];
		}
		return 0;
	}

	void remove_context (GL_CONTEXT *c)
	{
		for (int i=0; i<num_contexts; i++)
		{
			if (c == context[i])
			{
				int last = --num_contexts;
				context[i] = context[last];
				context[last] = 0;
				return;
			}
		}
	}

	GL_CONTEXT *new_context (HDC hDC)
	{
		GL_CONTEXT *w = 0;
		if (num_contexts < countof(context))
		{
			w = new GL_CONTEXT;
			if (w)
			{
				context[num_contexts++] = w;
				w->hDC = hDC;
			}
		}
		return w;
	}

	GL_CONTEXT *verify_context (HGLRC c)
	{
		for (int i=0; i<num_contexts; i++)
		{
			if (context[i] == c)
				return context[i];
		}
		return 0;
	}

	void sys_error (char *name, char *msg)
	{
		char full[256];
		sprintf(full,"GL: sys_error(%s)\n   %s\n",name,msg);
		DebugPrint(full);
	}

// Higher Level

	bool make_current (HDC hdc, HGLRC c)
	{
		GL_CONTEXT *context = verify_context(c);

		if (c != 0 && context == 0)
			return false;

		if (active_context)
		{
			// shutdown
			DRAW.active_context = 0;
			active_context = 0;
		}

		if (context)
		{
			// startup
			DRAW.active_context = &context->draw_context;
			context->check_ready();
		}
		active_context = context;

		return true;
	}

	bool delete_context (HGLRC c)
	{
		bool ok = false;
		if (verify_context(c))
		{
			GL_CONTEXT *context = (GL_CONTEXT *)c;
			DRAW.destroy_context(&context->draw_context);
			remove_context(context);
			ok = true;
		}
		return ok;
	}

	GL_CONTEXT *create_context (HDC hdc)
	{
		if (num_contexts != 0)
		{
			sys_error("CreateContext", "Multiple contexts not supported.");
			return 0;
		}

		PIXELFORMATDESCRIPTOR *pf = DRAW.get_format(DRAW.current_format);

		if (hdc == 0)
		{
			sys_error("CreateContext", "Invalid hdc specified.");
			return 0;
		}
		else if (pf == 0)
		{
			sys_error("CreateContext", "No pixel format selected.");
			return 0;
		}

		GL_CONTEXT *context = new_context(hdc);
		if (context)
		{
			HWND hWnd = WindowFromDC(hdc);		// window handle is more useful

			RECT box;
			GetClientRect(hWnd,&box);			// use window client for surface dimensions

			int w = box.right - box.left;
			int h = box.bottom - box.top;
			int bpp = pf->cColorBits;

			context->window_w = w;
			context->window_h = h;
			context->window_bpp = bpp;

			DrawContext *window = &context->draw_context;

			window->hWnd = hWnd;
			window->pixel_format = DRAW.current_format; 
			window->width = w;
			window->height = h;

			context->viewport(0,0,w,h);

			if (!DRAW.create_context(window))
			{
				sys_error("CreateContext", "failed to create draw surface.");
				remove_context(context);
				context = 0;
			}
		}

		return context;
	}

	void startup (void)
	{
		DebugPrint("GL: startup.\n");
		DRAW.startup();
	}

	void shutdown (void)
	{
		DebugPrint("GL: shutdown.\n");
		DRAW.shutdown();
	}
};

// GLOBAL
SYSTEM TheSystem;

//---------------------------------------------------------------------------
// GL Methods
//---------------------------------------------------------------------------

#undef GLMETHOD
#define GLMETHOD(type) type __stdcall

#define WARN_CONTEXT() \
	DebugPrint("GL: no active context\n")

#define GET_CONTEXT(v) \
	GL_CONTEXT *v = TheSystem.active_context; \
	if (v == 0) { WARN_CONTEXT(); return; }

#define GET_CONTEXT_RET(v,value) \
	GL_CONTEXT *v = TheSystem.active_context; \
	if (v == 0) { WARN_CONTEXT(); return value; }

//#define _GL_ERROR_(err,name) { context->gl_error(err,#name); return; }

//#define _GL_ERROR_MSG_(err,name,msg) { context->gl_error(err,#name,msg); return; }

#define ERR_NAME(func) const char *ErrorName = #func
#define ERR_MSG(msg) const char *ErrorMsg = msg

ERR_NAME(gl?);
ERR_MSG(0);

#define ERR_RETURN return
#define ERR_INVALID_OPERATION() { context->gl_error(GL_INVALID_OPERATION,ErrorName,ErrorMsg); ERR_RETURN; }
#define ERR_INVALID_ENUM() { context->gl_error(GL_INVALID_ENUM,ErrorName,ErrorMsg); ERR_RETURN; }
#define ERR_INVALID_VALUE() { context->gl_error(GL_INVALID_VALUE,ErrorName,ErrorMsg); ERR_RETURN; }
#define ERR_UNSUPPORTED() { context->gl_error(GL_INVALID_OPERATION,ErrorName,"minimal support"); ERR_RETURN; }
#define ERR_CODE(err) { context->gl_error(err,ErrorName,ErrorMsg); ERR_RETURN; }

//---------------------------------------------------------------------------
// DA Extensions
//---------------------------------------------------------------------------

GLMETHOD(void) glLockBufferEXT (void ** lfb, GLint * stride)
{
	GET_CONTEXT(context);
	DrawContext *window = &context->draw_context;

	if (window && window->lpDDSPrimary)	 // idiot check
	{
		if (context->in_scene)
		{
			context->end_scene();
			context->in_scene = false;
		}

		DRAW.lock_surface(1, lfb, stride);
	}
	else
	{
		if (lfb)
			*lfb = 0;
	}
}

GLMETHOD(void) glUnlockBufferEXT(void)
{
	GET_CONTEXT(context);
	DrawContext *window = &context->draw_context;

	if (window && window->lpDDSPrimary)  // idiot check
		DRAW.unlock_surface(1);
}

GLMETHOD(void) glClearCountEXT(void)
{
	GET_CONTEXT(context);
	context->primitive_count = 0;
}

GLMETHOD(GLint) glGetCountEXT(void)
{
	GET_CONTEXT_RET(context, 0);
	return context->primitive_count;
}

GLMETHOD(void) glSetDumpTextEXT (IDumpText * _dump)
{
	DUMP = _dump;
}
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

GLMETHOD(GLuint) glGetResidentTextureSizeEXT(void)
{
	GET_CONTEXT_RET(context, 0);
	GLuint result = context->texture_memory_used(false);
	return result;
}

GLMETHOD(GLuint) glGetTotalTextureSizeEXT(void)
{
	GET_CONTEXT_RET(context, 0);
	GLuint result = context->texture_memory_used(true);
	return result;
}

GLMETHOD(GLuint) glGetTextureSizeEXT(GLsizei n, const GLuint * textures)
{
	GET_CONTEXT_RET(context, 0);
	GLuint result = context->texture_memory_used(n, textures);
	return result;
}

GLMETHOD(GLuint) glGetResidentTextureCountEXT(void)
{
	GET_CONTEXT_RET(context, 0);
	GLuint result = context->texture_count(false);
	return result;
}


GLMETHOD(GLuint) glGetTotalTextureCountEXT(void)
{
	GET_CONTEXT_RET(context, 0);
	GLuint result = context->texture_count(true);
	return result;
}

GLMETHOD(void) glGetTotalTexturesEXT(GLuint * textures)
{
	GET_CONTEXT(context);
	context->get_texture_ids(textures, true);
}

GLMETHOD(void) glGetResidentTexturesEXT(GLuint * textures)
{
	GET_CONTEXT(context);
	context->get_texture_ids(textures, false);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

GLMETHOD(void) glLockArrays (int first, int count)
{
	ERR_NAME(gl);
	GET_CONTEXT(context);
	if (context->enable_vertex)
	{
		context->arrays_locked = true;
		context->reset_vertices();
	}
	else
	{
		context->gl_error(GL_INVALID_OPERATION, "LockArrays"); 
	}
}

GLMETHOD(void) glUnlockArrays(void)
{
	ERR_NAME(gl);
	GET_CONTEXT(context);
	if (context->enable_vertex)
	{
		context->arrays_locked = false;
	}
	else
	{
		context->gl_error(GL_INVALID_OPERATION, "UnlockArrays"); 
	}
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

/*
GLMETHOD(void) CullParameter(GLenum pname, GLfloat * params)
{
	switch (pname)
	{
		case GL_CULL_VERTEX_EYE_POSITION:
			break;

		case GL_CULL_VERTEX_OBJECT_POSITION:
			break;
	}
}
*/

struct DXStruct
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

GLMETHOD(void) glGetDirectXObjectsEXT(DXStruct * dx)
{
	GET_CONTEXT(context);
	dx->lpDDSPrimary	= context->draw_context.lpDDSPrimary;
	dx->lpDDSBack		= context->draw_context.lpDDSBack;
	dx->lpZBuffer		= context->draw_context.lpZBuffer;

	dx->lpDD			= context->draw_context.lpDD;
	dx->lpD3D			= context->draw_context.lpD3D;
	dx->lpD3DDevice		= context->draw_context.lpD3DDevice;
	dx->lpD3DViewport	= context->draw_context.lpD3DViewport;

	dx->num_texture_formats = texture_format_cnt;
	dx->texture_formats		= texture_formats;

	dx->screen_pixel_format	= &screen_pixel_format;

}

//---------------------------------------------------------------------------
// wgl Extensions
//---------------------------------------------------------------------------

#pragma warning(disable:4273)	// allow GDI overrides

GLMETHOD(PROC) wglGetProcAddress (LPCSTR name)
{
	if (strcmp(name,"glSetDumpTextEXT") == 0)
	{
		return (PROC)glSetDumpTextEXT;
	}
	if (strcmp(name,"glLockBufferEXT") == 0)
	{
		return (PROC)glLockBufferEXT;
	}
	else if (strcmp(name,"glUnlockBufferEXT") == 0)
	{
		return (PROC)glUnlockBufferEXT;
	}
	else if (strcmp(name,"glColorTableEXT") == 0)
	{
		return (PROC)glColorTableEXT;
	}
	else if (strcmp(name, "glClearCountEXT") == 0)
	{
		return (PROC)glClearCountEXT;
	}
	else if (strcmp(name, "glGetCountEXT") == 0)
	{
		return (PROC)glGetCountEXT;
	}
	else if (strcmp(name, "glGetTotalTextureSizeEXT") == 0)
	{
		return (PROC)glGetTotalTextureSizeEXT;
	}
	else if (strcmp(name, "glGetResidentTextureSizeEXT") == 0)
	{
		return (PROC)glGetResidentTextureSizeEXT;
	}
	else if (strcmp(name, "glGetTextureSizeEXT") == 0)
	{
		return (PROC)glGetTextureSizeEXT;
	}
	else if (strcmp(name, "glGetTotalTextureCountEXT") == 0)
	{
		return (PROC)glGetTotalTextureCountEXT;
	}
	else if (strcmp(name, "glGetResidentTextureCountEXT") == 0)
	{
		return (PROC)glGetResidentTextureCountEXT;
	}
	else if (strcmp(name, "glGetTotalTexturesEXT") == 0)
	{
		return (PROC)glGetTotalTexturesEXT;
	}
	else if (strcmp(name, "glGetResidentTexturesEXT") == 0)
	{
		return (PROC)glGetResidentTexturesEXT;
	}
	else if (strcmp(name, "glGetDirectXObjectsEXT") == 0)
	{
		return (PROC)glGetDirectXObjectsEXT;
	}

	return 0;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

GLMETHOD(HGLRC) wglGetCurrentContext (void)
{
	return TheSystem.active_context;
}

GLMETHOD(HDC) wglGetCurrentDC (void)
{
	if (TheSystem.active_context)
	{
		return TheSystem.active_context->hDC;
	}
	return 0;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

GLMETHOD(BOOL) wglSwapBuffers (HDC dc)
{
	#ifdef STATS
	TheStats.end();
	#endif

	GET_CONTEXT_RET(context,false);

	DRAW.flip_surface();

	return true;
}

GLMETHOD(int) wglChoosePixelFormat (HDC dc, const PIXELFORMATDESCRIPTOR *pf)
{
	int best = 0;
	int least = 0x7FFFFFFF;

	if (pf->nSize != sizeof(PIXELFORMATDESCRIPTOR))
		return 0;

	for (int i=0; i<DRAW.num_formats; i++)
	{
		PIXELFORMATDESCRIPTOR *f = &DRAW.pixel_formats[i];
		int delta = 0;
		if (f->iPixelType != pf->iPixelType)
			continue;
		delta += abs(f->cColorBits - pf->cColorBits) * 32;
		delta += abs(f->cDepthBits - pf->cDepthBits);
		if (delta < least)
		{
			best = i+1;
			least = delta;
		}
	}

	return best;
}

GLMETHOD(int) wglDescribePixelFormat (HDC dc, int iPixelFormat, UINT nBytes, PIXELFORMATDESCRIPTOR *pf)
{
	PIXELFORMATDESCRIPTOR *src = DRAW.get_format(iPixelFormat);

	if (src == 0 || nBytes != sizeof(PIXELFORMATDESCRIPTOR))
		return 0;

	*pf = *src;

	return DRAW.num_formats;
}

GLMETHOD(BOOL) wglSetPixelFormat (HDC dc, int iPixelFormat, const PIXELFORMATDESCRIPTOR *pf)
{
	PIXELFORMATDESCRIPTOR *dst = DRAW.get_format(iPixelFormat);
	if (dst == 0 || pf->nSize != sizeof(PIXELFORMATDESCRIPTOR)|| pf->nVersion != 1)
	{
		DebugPrint("GL: failed to SetPixelFormat\n");
		return false;
	}
	DRAW.current_format = iPixelFormat;
	// COPY CONTEXT OPTIONS
	dst->iPixelType = pf->iPixelType;
	dst->dwFlags = pf->dwFlags;
	dst->cDepthBits = pf->cDepthBits;
//	dst->cStencilBits = pf->cStencilBits;
//	dst->cAccumBits = pf->AccumBits;
	return true;
}

GLMETHOD(int) wglGetPixelFormat (HDC dc)
{
	return DRAW.current_format;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

GLMETHOD(HGLRC) wglCreateContext (HDC hdc)
{
	return TheSystem.create_context(hdc);
}

GLMETHOD(BOOL) wglDeleteContext (HGLRC c)
{
	return TheSystem.delete_context(c);
}

GLMETHOD(BOOL) wglMakeCurrent (HDC hdc, HGLRC c)
{
	return TheSystem.make_current(hdc,c);
}

#pragma warning(default:4273)	// allow GDI overrides

//---------------------------------------------------------------------------
// OpenGL Methods (standard)
//---------------------------------------------------------------------------

// STATE FLAGS

GLMETHOD(GLenum) glGetError (void)
{
	GET_CONTEXT_RET(context, GL_INVALID_OPERATION);
	return context->get_error();
}

GLMETHOD(void) glEnable (GLenum flag)
{
	ERR_NAME(glEnable);	GET_CONTEXT(context);

	EnableIndex i = LookupEnable(flag);

	if (i == ENABLE_NONE)
		ERR_INVALID_ENUM();

	if (context->active_list)
	{
		context->active_list->Enable(i);
	}
	else
	{
		context->cmd_enable(i);
	}
}

GLMETHOD(void) glDisable (GLenum flag)
{
	ERR_NAME(glDisable); GET_CONTEXT(context);

	EnableIndex i = LookupEnable(flag);

	if (i == ENABLE_NONE)
		ERR_INVALID_ENUM();

	if (context->active_list)
	{
		context->active_list->Disable(i);
	}
	else
	{
		context->cmd_disable(i);
	}
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

#undef ERR_RETURN
#define ERR_RETURN return false

GLMETHOD(GLboolean) glIsEnabled (GLenum flag)
{
	ERR_NAME(glIsEnabled);
	GET_CONTEXT_RET(context,false);

	EnableIndex i = LookupEnable(flag);

	if (i == ENABLE_NONE)
		ERR_INVALID_ENUM();

	return context->enable[i];
}
#undef ERR_RETURN
#define ERR_RETURN return

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

GLMETHOD(void) glGetBooleanv (GLenum pname, char *result)
{
	ERR_NAME(glGetBooleanv);
	GET_CONTEXT(context);

	switch (pname)
	{
		case GL_CULL_FACE:
			result[0] = context->enable[ENABLE_CULL_FACE];
			break;

		default:
			ERR_INVALID_ENUM();
	}
}

GLMETHOD(void) glGetIntegerv (GLenum pname, int *result)
{
	ERR_NAME(glGetIntegerv);
	GET_CONTEXT(context);

	PIXELFORMATDESCRIPTOR *pf = DRAW.get_format(DRAW.current_format);

	switch (pname)
	{
		case GL_POLYGON_MODE:
			result[0] = context->Polygon_mode[0];
			result[1] = context->Polygon_mode[1];
			break;

		case GL_CULL_FACE_MODE:
			if (context->cull_face[0] && context->cull_face[1])
				result[0]= GL_FRONT_AND_BACK;
			else 
				result[0]= context->cull_face[0] ? GL_FRONT : GL_BACK;
			break;

		case GL_FRONT_FACE:
			result[0] = context->front_face;
			break;

		case GL_RED_BITS:
			result[0] = (pf != 0) ? pf->cRedBits : 0;
			break;

		case GL_GREEN_BITS:
			result[0] = (pf != 0) ? pf->cGreenBits : 0;
			break;

		case GL_BLUE_BITS:
			result[0] = (pf != 0) ? pf->cBlueBits : 0;
			break;

		case GL_ALPHA_BITS:
			result[0] = (pf != 0) ? pf->cAlphaBits : 0;
			break;

		case GL_MAX_LIGHTS:
			result[0] = 8;
			break;

		case GL_TEXTURE_BINDING_2D:
			result[0] = context->get_texture_binding();
			break;

		case GL_VIEWPORT:
			result[0] = context->window_x;
			result[1] = context->window_y;
			result[2] = context->window_w;
			result[3] = context->window_h;
			break;

		default:
			ERR_INVALID_ENUM();
	}
}

GLMETHOD(void) glGetFloatv (GLenum pname, float *result)
{
	ERR_NAME(glGetFloatv);
	GET_CONTEXT(context);

	switch (pname)
	{
		case GL_COLOR_CLEAR_VALUE:
			result[0] = context->clear_color.r * i2f;
			result[1] = context->clear_color.g * i2f;
			result[2] = context->clear_color.b * i2f;
			result[3] = context->clear_color.a * i2f;
			break;

		case GL_DEPTH_CLEAR_VALUE:
			result[0] = context->clear_depth;
			break;

		case GL_LIGHT_MODEL_AMBIENT:
			result[0] = context->global_ambient.r * i2f;
			result[1] = context->global_ambient.g * i2f;
			result[2] = context->global_ambient.b * i2f;
			result[3] = context->global_ambient.a * i2f;
			break;

		case GL_MODELVIEW_MATRIX:
			memcpy(result, context->ModelViewMatrix.m, sizeof(float) * 16);
			break;

		case GL_PROJECTION_MATRIX:
			memcpy(result, context->ProjectionMatrix.m, sizeof(float) * 16);
			break;

		case GL_VIEWPORT:
			result[0] = context->window_x;
			result[1] = context->window_y;
			result[2] = context->window_w;
			result[3] = context->window_h;
			break;

		default:
			ERR_INVALID_ENUM();
	}
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

GLMETHOD(const GLubyte *) glGetString (GLenum name)
{
	const char *string;

	switch (name)
	{
	case GL_VENDOR:
		string = "Digital Anvil";
		break;

	case GL_RENDERER:
		string = Implementation;
		break;

	case GL_VERSION:
		string = "IDisplay 2.0";
		break;

	case GL_EXTENSIONS:
		string = "GL_EXT_paletted_texture GL_DA_lock_buffer GL_DA_primitive_count";
		break;
	}

	return (const GLubyte *)string;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

GLMETHOD(void) glPushAttrib (GLbitfield mask)
{
	ERR_NAME(glPushAttrib);
	GET_CONTEXT(context);
	ERR_UNSUPPORTED();
}

GLMETHOD(void) glPopAttrib (GLbitfield mask)
{
	ERR_NAME(glPopAttrib);
	GET_CONTEXT(context);
	ERR_UNSUPPORTED();
}

//---------------------------------------------------------------------------

// BUFFER

GLMETHOD(void) glClear (GLbitfield mask)
{
	ERR_NAME(glClear);
	GET_CONTEXT(context);

	if (!context->ready_cmd)
		ERR_INVALID_OPERATION();

	RECT *box = context->enable[ENABLE_SCISSOR_TEST] ? &context->scissor : 0;

	if (mask & GL_COLOR_BUFFER_BIT)
	{
		DRAW.clear_color(1,context->clear_rgb,box);
	}

	if (mask & GL_DEPTH_BUFFER_BIT)
	{
		DRAW.clear_zbuffer(context->clear_z,box);
	}

	#ifdef STATS
	TheStats.begin();
	#endif
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

GLMETHOD(void) glClearColor (float r, float g, float b, float a)
{
	ERR_NAME(glClearColor);
	GET_CONTEXT(context);

	if (context->inside_begin_end())
		ERR_INVALID_OPERATION();

	context->set_clear_color(r,g,b,a);
}

GLMETHOD(void) glClearDepth (float depth)
{
	ERR_NAME(glClearDepth);
	GET_CONTEXT(context);

	if (context->inside_begin_end())
		ERR_INVALID_OPERATION();

	context->set_clear_depth(depth);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

GLMETHOD(void) glFlush (void)
{
	ERR_NAME(glFlush);
	GET_CONTEXT(context);

	if (!context->ready_cmd)
		ERR_INVALID_OPERATION();

// SEND ANY BUFFERED COMMANDS

	if (context->in_scene)
	{
		context->end_scene();
		context->in_scene = false;
	}
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

GLMETHOD(void) glFinish (void)
{
	ERR_NAME(glFinish);
	GET_CONTEXT(context);

	if (!context->ready_cmd)
		ERR_INVALID_OPERATION();

// SEND ANY BUFFERED COMMANDS

	if (context->in_scene)
	{
		context->end_scene();
		context->in_scene = false;
	}

// WAIT FOR RENDERING TO COMPLETE?

	void *bfr;
	int stride;
	DRAW.lock_surface(1, &bfr,&stride);
	DRAW.unlock_surface(1);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

GLMETHOD(void) glScissor (int x, int y, int w, int h)
// Note: x,y = left,bottom corner
{
	ERR_NAME(glScissor);
	GET_CONTEXT(context);

	if (!context->ready_cmd)
		ERR_INVALID_OPERATION();

	y = context->draw_context.height - y - h;	// GL meaning to Top-Left corner

	context->scissor.left = x;
	context->scissor.top = y;
	context->scissor.right = x+w-1;
	context->scissor.bottom = y+h-1;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

/*


GLMETHOD(GLint) glRenderMode (GLenum mode)
{
	if (mode != GL_RENDER)
		ERR_UNSUPPORTED();

	return 0;
}

GLsizei select_size;
GLuint *select_buffer;

GLMETHOD(void) glSelectBuffer (GLsizei size, GLuint *buffer)
{
	select_size = size;
	select_buffer = buffer;
}

*/

//---------------------------------------------------------------------------

// FEATURES

GLMETHOD(void) glCullFace (GLenum side)
{
	ERR_NAME(glCullFace);
	GET_CONTEXT(context);

	if (!context->ready_cmd)
		ERR_INVALID_OPERATION();

	switch (side)
	{
		case GL_FRONT:
			context->cull_face[0] = true;
			context->cull_face[1] = false;
			break;

		case GL_BACK:
			context->cull_face[0] = false;
			context->cull_face[1] = true;
			break;

		case GL_FRONT_AND_BACK:
			context->cull_face[0] = true;
			context->cull_face[1] = true;
			break;

		default:
			ERR_INVALID_ENUM();
	}
}

GLMETHOD(void) glFrontFace (GLenum dir)
{
	ERR_NAME(glFrontFace);
	GET_CONTEXT(context);

	if (!context->ready_cmd)
		ERR_INVALID_OPERATION();

	if (dir != GL_CW && dir != GL_CCW)
		ERR_INVALID_ENUM();

	context->front_face = dir;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

GLMETHOD(void) glPolygonMode (GLenum side, GLenum mode)
{
	ERR_NAME(glPolygonMode);
	GET_CONTEXT(context);

	if (context->inside_begin_end())
		ERR_INVALID_OPERATION();

	if (mode != GL_POINT && mode != GL_LINE && mode != GL_FILL)
		ERR_INVALID_ENUM();

	switch (side)
	{
		case GL_FRONT:
			context->Polygon_mode[0] = mode;
			break;

		case GL_BACK:
			context->Polygon_mode[0] = mode;
			break;

		case GL_FRONT_AND_BACK:
			context->Polygon_mode[0] = mode;
			context->Polygon_mode[1] = mode;
			break;

		default:
			ERR_INVALID_ENUM();
	}
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

GLMETHOD(void) glShadeModel (GLenum m)
{
	ERR_NAME(glShadeModel);
	GET_CONTEXT(context);

	if (!context->ready_cmd)
		ERR_INVALID_OPERATION();

	switch (m)
	{
		case GL_FLAT:
			context->shade_model = m;
			break;

		case GL_SMOOTH:
			context->shade_model = m;
			break;

		default:
			ERR_INVALID_OPERATION();
	}
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

GLMETHOD(void) glBlendFunc (GLenum src, GLenum dst)
{
	ERR_NAME(glBlendFunc);
	GET_CONTEXT(context);

	if (!context->ready_cmd)
		ERR_INVALID_OPERATION();

	context->set_blend(src,dst);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

GLMETHOD(void) glAlphaFunc (GLenum func, GLclampf ref)
{
	ERR_NAME(glAlphaFunc);
	GET_CONTEXT(context);
	ERR_UNSUPPORTED();
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

GLMETHOD(void) glDepthFunc (GLenum func)
{
	ERR_NAME(glDepthFunc);
	GET_CONTEXT(context);

	if (!context->ready_cmd)
		ERR_INVALID_OPERATION();

	context->depth_func = func;
	context->modify_pixels = true;
}

GLMETHOD(void) glDepthRange (GLclampf znear, GLclampf zfar)
{
	ERR_NAME(glDepthRange);
	GET_CONTEXT(context);
	ERR_UNSUPPORTED();
}

GLMETHOD(void) glDepthMask (GLboolean flag)
{
	ERR_NAME(glDepthMask);
	GET_CONTEXT(context);

	if (!context->ready_cmd)
		ERR_INVALID_OPERATION();

	context->depth_mask = flag != 0;
	context->modify_pixels = true;
}

GLMETHOD(void) glHint (GLenum target, GLenum mode)
{
	ERR_NAME(glHint);
	GET_CONTEXT(context);

	if (!context->ready_cmd)
		ERR_INVALID_OPERATION();

	switch (target)
	{
		case GL_VOLUME_CLIPPING_HINT:
			switch (mode)
			{
				case GL_FASTEST:
					context->enable_clipping = false;
					break;

				case GL_DONT_CARE:
				case GL_NICEST:
					context->enable_clipping = true;
					break;
			};
			break;

		case GL_FOG_HINT:
		// Unsupported, but don't give an error message.
			break;

		case GL_LINE_SMOOTH_HINT:
			switch(mode)
			{
				case GL_DONT_CARE:
				case GL_FASTEST:
					context->line_antialias = false;
					break;

				case GL_NICEST:
					context->line_antialias = true;
					break;
			}
			modify_misc = true;
			break;

		case GL_PERSPECTIVE_CORRECTION_HINT:
		// Unsupported, but don't give an error message.
			break;

		default:
			ERR_INVALID_ENUM();
	}
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

GLMETHOD(void) glClipPlane (GLenum plane, const GLfloat *equation)
{
	ERR_NAME(glClipPlane);
	GET_CONTEXT(context);
	ERR_UNSUPPORTED();

/*
	if (inside_begin_end())
	{ gl_error(GL_INVALID_OPERATION,"ClipPlane"); return; }

	uint p = (uint)plane - GL_CLIP_PLANE0;

	if (p >= MAX_CLIP_PLANES)
	{ gl_error(GL_INVALID_ENUM,"ClipPlane"); return; }

	VECTOR vObj;
	vObj.x = equation[0];
	vObj.y = equation[1];
	vObj.z = equation[2];
	vObj.w = equation[3];

	transform_points(1, clip_plane+p, &vObj);
*/
}

GLMETHOD(void) glGetClipPlane (GLenum plane, GLfloat *equation)
{
	ERR_NAME(glGetClipPlane);
	GET_CONTEXT(context);
	ERR_UNSUPPORTED();

/*
	if (inside_begin_end())
	{ gl_error(GL_INVALID_OPERATION,"ClipPlane"); return; }

	uint p = (uint)plane - GL_CLIP_PLANE0;

	if (p >= MAX_CLIP_PLANES)
	{ gl_error(GL_INVALID_ENUM,"ClipPlane"); return; }

	equation[0] = clip_plane[p].x;
	equation[1] = clip_plane[p].y;
	equation[2] = clip_plane[p].z;
	equation[3] = clip_plane[p].w;
*/
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

GLMETHOD(void) glLineWidth (GLfloat width)
{
	ERR_NAME(glLineWidth);
	GET_CONTEXT(context);

	context->line_width = width;
	ERR_UNSUPPORTED();
}

GLMETHOD(void) glPointSize (GLfloat size)
{
	ERR_NAME(glPointSize);
	GET_CONTEXT(context);

	context->point_size = size;
	ERR_UNSUPPORTED();
}

//---------------------------------------------------------------------------

// PERSPECTIVE

GLMETHOD(void) glViewport (GLint x, GLint y, GLsizei width, GLsizei height)
{
	ERR_NAME(glViewport);
	GET_CONTEXT(context);

	if (!context->ready_cmd)
		ERR_INVALID_OPERATION();

	context->viewport(x,y,width,height);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

GLMETHOD(void) glFrustum (double left, double right, double bottom, double top, double z0, double z1)
{
	ERR_NAME(glFrustum);
	GET_CONTEXT(context);

	if (!context->ready_cmd)
		ERR_INVALID_OPERATION();

	if (z0<=0.0 || z1<=0.0)
		ERR_INVALID_VALUE();

	context->znear = z0;
	context->zfar = z1;

	context->frustum_matrix(left,right,bottom,top,z0,z1);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

GLMETHOD(void) glOrtho (double left, double right, double bottom, double top, double z0, double z1)
{
	GET_CONTEXT(context);

	context->znear = z0;
	context->zfar = z1;

	context->ortho_matrix(left,right,bottom,top,z0,z1);
}

//---------------------------------------------------------------------------

// MATRIX MATH

GLMETHOD(void) glMatrixMode (GLenum mode)
{
	ERR_NAME(glMatrixMode);
	GET_CONTEXT(context);

	if (context->inside_begin_end())
		ERR_INVALID_OPERATION();

	switch (mode)
	{
		case GL_MODELVIEW:
		case GL_PROJECTION:
		case GL_TEXTURE:
			context->Matrix_mode = mode;
			break;
		default:
			ERR_INVALID_ENUM();
	}
}

GLMETHOD(void) glLoadIdentity (void)
{
	ERR_NAME(glLoadIdentity);
	GET_CONTEXT(context);

	if (context->inside_begin_end())
		ERR_INVALID_OPERATION();

	context->set_identity(context->Matrix_mode);
}

GLMETHOD(void) glLoadMatrixf (const float *object_to_view)
{
	ERR_NAME(glLoadMatrixf);
	GET_CONTEXT(context);

	if (context->inside_begin_end())
		ERR_INVALID_OPERATION();

	context->load_matrix(object_to_view);
}

GLMETHOD(void) glMultMatrixf (const GLfloat *m)
{
	ERR_NAME(glMultMatrixf);
	GET_CONTEXT(context);

	if (context->inside_begin_end())
		ERR_INVALID_OPERATION();

	context->matrix_mul(context->Matrix_mode,m);
}

GLMETHOD(void) glPushMatrix (void)
{
	ERR_NAME(glPushMatrix);
	GET_CONTEXT(context);

	if (context->inside_begin_end())
		ERR_INVALID_OPERATION();

	GLenum err = context->push_matrix();
	if (err != GL_NO_ERROR)
		ERR_CODE(GL_STACK_UNDERFLOW);
}

GLMETHOD(void) glPopMatrix (void)
{
	ERR_NAME(glPopMatrix);
	GET_CONTEXT(context);

	if (context->inside_begin_end())
		ERR_INVALID_OPERATION();

	GLenum err = context->pop_matrix();
	if (err != GL_NO_ERROR)
		ERR_CODE(GL_STACK_UNDERFLOW);
}

GLMETHOD(void) glRotatef (GLfloat angle, GLfloat x, GLfloat y, GLfloat z)
{
	ERR_NAME(glRotatef);
	GET_CONTEXT(context);

	if (context->inside_begin_end())
		ERR_INVALID_OPERATION();

	context->matrix_rotate(angle,x,y,z);
}

GLMETHOD(void) glScalef (GLfloat x, GLfloat y, GLfloat z)
{
	ERR_NAME(glScalef);
	GET_CONTEXT(context);

	if (context->inside_begin_end())
		ERR_INVALID_OPERATION();

	context->matrix_scale(x,y,z);
}

GLMETHOD(void) glTranslatef (GLfloat x, GLfloat y, GLfloat z)
{
	ERR_NAME(glTranslatef);
	GET_CONTEXT(context);

	if (context->inside_begin_end())
		ERR_INVALID_OPERATION();

	if (context->active_list)
	{
		context->active_list->Translate(x,y,z);
	}
	else
	{
		context->cmd_translate(x,y,z);
	}
}

//---------------------------------------------------------------------------

// LIST

GLMETHOD(GLuint) glGenLists (GLsizei range)
{
	GET_CONTEXT_RET(context,0);

	static GLuint ListBase = 0xFEED0000;
	GLuint first = ListBase;
	ListBase += range;
	return first;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

GLMETHOD(GLboolean) glIsList (GLuint id)
{
	GET_CONTEXT_RET(context,false);

	return context->find_list(id) != 0;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

GLMETHOD(void) glNewList (GLuint id, GLenum mode)
{
	ERR_NAME(glNewList);
	GET_CONTEXT(context);

	if (context->inside_begin_end())
		ERR_INVALID_OPERATION();

	if (context->inside_draw_list())
		ERR_INVALID_OPERATION();

	context->vertex_buffer.reset();
	context->active_list = context->new_list(id);

	if (context->active_list == 0)
		ERR_CODE(GL_OUT_OF_MEMORY);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

GLMETHOD(void) glEndList (void)
{
	ERR_NAME(glEndList);
	GET_CONTEXT(context);

	if (context->active_list == 0)
		ERR_INVALID_OPERATION();

	// ASSUME GL_COMPILE

	static_cast<VertexBuffer *>(context->active_list)->alloc(context->vertex_buffer);

	context->active_list = 0;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

GLMETHOD(void) glDeleteLists (GLuint id, GLsizei range)
{
	GET_CONTEXT(context);

	for (int i=0; i<range; i++)
	{
		context->remove_list(id+i);
	}
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

GLMETHOD(void) glCallList (GLuint id)
{
	ERR_NAME(glCallList);
	GET_CONTEXT(context);

	if (context->inside_begin_end())
		ERR_INVALID_OPERATION();

	DrawList *list = context->find_list(id);
	if (list)
	{
		context->reset_vertices();

		list->start = 0;
		context->transform(*list);
		//context->arrays_locked = true;

		for (uint i=0; i<list->num_cmds; )
		{
			if (!context->perform(list,i))
			{
				ERR_INVALID_OPERATION();
			}
		}

		//context->arrays_locked = false;
	}
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

GLMETHOD(void) glListBase (GLuint base)
{
	ERR_NAME(glListBase);
	GET_CONTEXT(context);

	if (context->inside_begin_end())
		ERR_INVALID_OPERATION();

	context->list_base = base;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

GLMETHOD(void) glCallLists (int count, GLenum type, const void *lists)
{
	ERR_NAME(glCallLists);
	GET_CONTEXT(context);

	if (context->inside_begin_end())
		ERR_INVALID_OPERATION();

	int i;
	switch (type)
	{
		case GL_UNSIGNED_BYTE:
			for (i=0; i<count; i++)
				glCallList(context->list_base + ((GLubyte*)lists)[i]);
			break;

		case GL_UNSIGNED_INT:
			for (i=0; i<count; i++)
				glCallList(context->list_base + ((GLuint*)lists)[i]);
			break;

		default:
			ERR_UNSUPPORTED();
	}
}

//---------------------------------------------------------------------------

// POLYGON INFO

GLMETHOD(void) glColor3ub (GLubyte r, GLubyte g, GLubyte b)
{
	GET_CONTEXT(context);
	context->set_color(r,g,b,255);
}
GLMETHOD(void) glColor3ubv (const GLubyte *rgb)
{
	GET_CONTEXT(context);
	context->set_color(rgb[0],rgb[1],rgb[2],255);
}

GLMETHOD(void) glColor3f (float r, float g, float b)
{
	GET_CONTEXT(context);
	context->set_color(r, g, b, 1.0);
}
GLMETHOD(void) glColor3fv (const GLfloat *rgb)
{
	GET_CONTEXT(context);
	context->set_color(rgb[0],rgb[1],rgb[2],1.0);
}

GLMETHOD(void) glColor4ub (GLubyte r, GLubyte g, GLubyte b, GLubyte a)
{
	GET_CONTEXT(context);
	context->set_color(r,g,b,a);
}
GLMETHOD(void) glColor4ubv (const GLubyte *rgba)
{
	GET_CONTEXT(context);
	context->set_color(rgba[0],rgba[1],rgba[2],rgba[3]);
}

GLMETHOD(void) glColor4f (GLfloat r, GLfloat g, GLfloat b, GLfloat a)
{
	GET_CONTEXT(context);
	context->set_color(r,g,b,a);
}
GLMETHOD(void) glColor4fv (const GLfloat *rgba)
{
	GET_CONTEXT(context);
	context->set_color(rgba[0],rgba[1],rgba[2],rgba[3]);
}

GLMETHOD(void) glIndexi (GLint index)
{
	GET_CONTEXT(context);
	// FUTURE: support 8-bit mode?
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

GLMETHOD(void) glTexCoord2f (float u, float v)
{
	GET_CONTEXT(context);
	context->set_uv(u,v);
}
GLMETHOD(void) glTexCoord2fv (const float *uv)
{
	GET_CONTEXT(context);
	context->set_uv(uv[0],uv[1]);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

GLMETHOD(void) glNormal3f (float x, float y, float z)
{
	GET_CONTEXT(context);
	context->set_normal(x,y,z);
}

GLMETHOD(void) glNormal3fv (const float *xyz)
{
	GET_CONTEXT(context);
	context->set_normal(xyz[0],xyz[1],xyz[2]);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

GLMETHOD(void) glVertex2i (GLint x, GLint y)
{
	ERR_NAME(glVertex2i);
	GET_CONTEXT(context);

	if (!context->ready_vertex)
		ERR_INVALID_OPERATION();

	context->vertex(x,y,0,0);
}

GLMETHOD(void) glVertex2f (float x, float y)
{
	ERR_NAME(glVertex2f);
	GET_CONTEXT(context);

	if (!context->ready_vertex)
		ERR_INVALID_OPERATION();

	context->vertex(x,y,0,0);
}
GLMETHOD(void) glVertex2fv (const float *xy)
{
	glVertex2f(xy[0],xy[1]);
}

GLMETHOD(void) glVertex3i (int x, int y, int z)
{
	ERR_NAME(glVertex3i);
	GET_CONTEXT(context);
	
	if (!context->ready_vertex)
		ERR_INVALID_OPERATION();

	context->vertex(x, y, z, 0);
}
GLMETHOD(void) glVertex3f (float x, float y, float z)
{
	ERR_NAME(glVertex3f);
	GET_CONTEXT(context);

	if (!context->ready_vertex)
		ERR_INVALID_OPERATION();

	context->vertex(x,y,z,0);
}
GLMETHOD(void) glVertex3fv (const float *xyz)
{
	glVertex3f(xyz[0],xyz[1],xyz[2]);
}

GLMETHOD(void) glVertex4f (float x, float y, float z, float w)
{
	ERR_NAME(glVertex4f);
	GET_CONTEXT(context);

	if (!context->ready_vertex)
		ERR_INVALID_OPERATION();

	context->vertex(x,y,z,w);
}
GLMETHOD(void) glVertex4fv (const float * xyzw)
{
	glVertex4f(xyzw[0], xyzw[1], xyzw[2], xyzw[3]);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

GLMETHOD(void) glBegin (GLenum mode)
{
	ERR_NAME(glBegin);
	GET_CONTEXT(context);

	if (!context->ready_cmd)
		ERR_INVALID_OPERATION();

	if (invalid_draw_mode(mode))
		ERR_INVALID_VALUE();

	context->Begin_mode = mode;

	if (context->active_list)
	{
		context->active_list->Begin(mode);
	}
	else
	{
		context->cmd_begin();

		if (context->arrays_locked)
		{
			// PARTIAL RESET
			context->vertex_buffer.reset_chains();
		}
		else
		{
			context->reset_vertices();
		}
	}

	context->check_ready();
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

GLMETHOD(void) glEnd (void)
{
	ERR_NAME(glEnd);
	GET_CONTEXT(context);

	if (context->ready_cmd)
		ERR_INVALID_OPERATION();

	if (context->active_list)
	{
		context->active_list->End();
	}
	else // ASSUME GL_COMPILE
	{
		context->render(context->vertex_buffer.num_chains);
	}

	context->Begin_mode = GL_INVALID_ENUM;

	context->check_ready();
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

GLMETHOD(void) glRectf (GLfloat x1,GLfloat y1,  GLfloat x2, GLfloat y2)
{
	glBegin(GL_POLYGON); 
		glVertex3f(x1, y1, 0); 
		glVertex3f(x2, y1, 0); 
		glVertex3f(x2, y2, 0); 
		glVertex3f(x1, y2, 0); 
	glEnd(); 
}

//---------------------------------------------------------------------------

// ARRAY - POINTERS

GLMETHOD(void) glEnableClientState (GLenum array)
{
	GET_CONTEXT(context);
	context->set_client_state(array,true);

	if (array == GL_VERTEX_ARRAY)
		glLockArrays(0,0);
}

GLMETHOD(void) glDisableClientState (GLenum array)
{
	if (array == GL_VERTEX_ARRAY)
		glUnlockArrays();

	GET_CONTEXT(context);
	context->set_client_state(array,false);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

GLMETHOD(void) glColorPointer (GLint size, GLenum type, GLsizei stride, const GLvoid *pointer)
{
	ERR_NAME(glColorPointer);
	GET_CONTEXT(context);
	if (size < 3 || size > 4)
		ERR_INVALID_VALUE();

	if (stride < 0)
		ERR_INVALID_VALUE();

	if (type != GL_UNSIGNED_BYTE)	// Minimal support!
		ERR_UNSUPPORTED();

	if (stride == 0) stride = size*TypeSize(type);	// bytes to next

	context->client_color.size = size;
	context->client_color.type = type;
	context->client_color.stride = stride;
	context->client_color.pointer = pointer;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

GLMETHOD(void) glNormalPointer (GLenum type, GLsizei stride, const GLvoid *pointer)
// what happened? "GLsizei count"
{
	ERR_NAME(glNormalPointer);
	GET_CONTEXT(context);

	if (stride < 0)
		ERR_INVALID_VALUE();

	if (type != GL_FLOAT)	// Minimal support!
		ERR_UNSUPPORTED();

	if (stride == 0) stride = 3*TypeSize(type);	// bytes to next

	context->client_normal.size = 3;		// constant
	context->client_normal.type = type;
	context->client_normal.stride = stride;
	context->client_normal.pointer = pointer;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

GLMETHOD(void) glTexCoordPointer (GLint size, GLenum type, GLsizei stride, const GLvoid *pointer)
{
	ERR_NAME(glTexCoordPointer);
	GET_CONTEXT(context);

	if (size < 1 || size > 4)
		ERR_INVALID_VALUE();

	if (stride < 0)
		ERR_INVALID_VALUE();

	if (type != GL_FLOAT || size != 2)	// Minimal support!
		ERR_UNSUPPORTED();

	if (stride == 0) stride = size*TypeSize(type);	// bytes to next

	context->client_texcoord.size = size;
	context->client_texcoord.type = type;
	context->client_texcoord.stride = stride;
	context->client_texcoord.pointer = pointer;

	context->vertex_buffer.clear_flags(BIT_TEXCOORD);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

GLMETHOD(void) glVertexPointer (GLint size, GLenum type, GLsizei stride, const GLvoid *pointer)
{
	ERR_NAME(glVertexPointer);
	GET_CONTEXT(context);

	if (size < 2 || size > 4)
		ERR_INVALID_VALUE();

	if (stride < 0)
		ERR_INVALID_VALUE();

	if (type != GL_FLOAT)	// Minimal support!
		ERR_UNSUPPORTED();

	if (stride == 0) stride = size*TypeSize(type);	// bytes to next

	context->client_vertex.size = size;
	context->client_vertex.type = type;
	context->client_vertex.stride = stride;
	context->client_vertex.pointer = pointer;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

GLMETHOD(void) glInterleavedArrays (GLenum format, GLsizei stride, const GLvoid *pointer)
{
	ERR_NAME(glInterleavedArrays);
	GET_CONTEXT(context);

	if (format != GL_T2F_C4UB_V3F)
		ERR_UNSUPPORTED();

	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);

	int tsize = 2*sizeof(float);
	int csize = 4*sizeof(char);
	int vsize = 3*sizeof(float);

	if (stride == 0) stride = tsize + csize + vsize;

	char * ptr = (char *)pointer;
	glTexCoordPointer(2, GL_FLOAT, stride, ptr);
	ptr += tsize;
	glColorPointer(4, GL_UNSIGNED_BYTE, stride, ptr);
	ptr += csize;
	glVertexPointer(3, GL_FLOAT, stride, ptr);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

GLMETHOD(void) glGetPointer (GLenum pname, GLvoid **params)
{
	ERR_NAME(glGetPointer);
	GET_CONTEXT(context);
	*params = 0;
	ERR_UNSUPPORTED();
}

//---------------------------------------------------------------------------

// ARRAY - DRAWS

GLMETHOD(void) glArrayElement (GLint index)
{
	ERR_NAME(glArrayElement);
	GET_CONTEXT(context);

	if (!context->inside_begin_end())
		ERR_INVALID_OPERATION();

	context->use_client_vertex(index);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

GLMETHOD(void) glDrawArrays (GLenum mode, GLint first, GLsizei count)
// Ex: GL->DrawArrays(GL_TRIANGLES, 0, 3*16);
{
	ERR_NAME(glDrawArrays);
	GET_CONTEXT(context);

	if (count < 0)
		ERR_INVALID_VALUE();

	glBegin(mode);
	for (int i=0; i<count; i++)
	{
		context->use_client_vertex(first+i);
	}
	glEnd();
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

GLMETHOD(void) glDrawElements (GLenum mode, GLsizei count, GLenum type, const GLvoid *indices)
// Ex: GL->DrawElements(GL_TRIANGLES,3, GL_INT, vchain_list);
{
	ERR_NAME(glDrawElements);
	GET_CONTEXT(context);

	if (count < 0)
		ERR_INVALID_VALUE();

	if (type != GL_INT && type != GL_UNSIGNED_INT)	// Minimal support!
		ERR_UNSUPPORTED();

	uint *index_list = (uint *)indices;

	glBegin(mode);

	// Optimization for POLYMESH useage

	if (context->enable_vertex &&
		context->client_vertex.type == GL_FLOAT &&
		context->client_vertex.size == 3 &&
		context->enable_texcoord &&
		context->client_texcoord.type == GL_FLOAT &&
		context->client_texcoord.size == 2 &&
		context->enable_color &&
		context->client_color.type == GL_UNSIGNED_BYTE &&
		context->client_color.size == 4
		)
	{
		VertexBuffer &vb = context->vertex_buffer;

		for (int n=0; n<count; n++)
		{
			uint i = ((uint *)index_list)[n];

			// Make sure index is valid
//			if (i >= context->client_vertex.count)
//			{ context->gl_error(GL_INVALID_ENUM,"Draw Array/Elements"); break; }

			uint v = context->client_vlist[i];

			if (v == NULL_VERTEX)	// not already copied?
			{
				v = vb.new_vertex();

				if (v == NULL_VERTEX)
				{ context->gl_error(GL_INVALID_ENUM,"Draw Array/Elements"); break; }

				context->client_vlist[i] = v;

			// (enable_color)
				uchar *color = (uchar *)context->client_color.index(i);
				vb.set_color(v, color[0],color[1],color[2], color[3]);

			// (enable_texcoord)
				float *texcoord = (float *)context->client_texcoord.index(i);
				vb.set_texcoord(v, texcoord[0],texcoord[1]);

			// (enable_normal)
//				float *normal = (float *)context->client_normal.index(i);
//				vb.set_normal(v, normal);

			// (enable_vertex)

				float *vertex = (float *)context->client_vertex.index(i);
				vb.set_vertex3(v, vertex);
				//vertex_buffer.set_vertex0(v, vertex[0],vertex[1],vertex[2]);
				//vertex_buffer.set_vertex(v, vertex[0],vertex[1],vertex[2]);

				vb.set_flag(v,BIT_COLOR|BIT_NORMAL|BIT_TEXCOORD);
			}
			else if (!vb.test_flag(v,BIT_COLOR|BIT_NORMAL|BIT_TEXCOORD))
			{
				vb.set_flag(v,BIT_COLOR|BIT_NORMAL|BIT_TEXCOORD);

				uchar *color = (uchar *)context->client_color.index(i);
				vb.set_color(v, color[0],color[1],color[2], color[3]);

				float *texcoord = (float *)context->client_texcoord.index(i);
				vb.set_texcoord(v, texcoord[0],texcoord[1]);

				context->build_vertices(vb,v,1,vClip);
			}

			context->use_vertex(v);
		}
	}
	else // GENERAL CASE
	{
		for (int i=0; i<count; i++)
		{
			context->use_client_vertex(((uint *)index_list)[i]);
		}
	}

	glEnd();
}

//---------------------------------------------------------------------------

// LIGHT

GLMETHOD(void) glLightf (GLenum light, GLenum pname, GLfloat param)
{
	glLightfv(light, pname, &param);
}


#define TRANSFORM_POINT( Q, M, P )					\
   Q[0] = M[0] * P[0] + M[4] * P[1] + M[8] *  P[2] + M[12] * P[3];	\
   Q[1] = M[1] * P[0] + M[5] * P[1] + M[9] *  P[2] + M[13] * P[3];	\
   Q[2] = M[2] * P[0] + M[6] * P[1] + M[10] * P[2] + M[14] * P[3];	\
   Q[3] = M[3] * P[0] + M[7] * P[1] + M[11] * P[2] + M[15] * P[3];


GLMETHOD(void) glLightfv (GLenum light, GLenum pname, const GLfloat *params)
{
	GET_CONTEXT(context);

	uint l = light - GL_LIGHT0;
	if (l >= MAX_LIGHTS)
	{
		context->gl_error(GL_INVALID_ENUM,"Light","invalid light index");
	}


	Light * lt = context->lights + l;

	switch (pname)
	{
		case GL_AMBIENT:	
		case GL_SPECULAR:
		case GL_SPOT_DIRECTION:
		case GL_SPOT_EXPONENT:
		case GL_SPOT_CUTOFF:
			break;

		case GL_CONSTANT_ATTENUATION:
			lt->kc = *params;
			break;

		case GL_LINEAR_ATTENUATION:
			lt->kl = *params;
			break;

		case GL_QUADRATIC_ATTENUATION:
			lt->kq = *params;
			break;

		case GL_DIFFUSE:
			lt->diffuse.set(params[0], params[1], params[2], params[3]);
			break;

		case GL_POSITION:
			//context->transform_points(1, &lt->pos, (VECTOR *) params);
			TRANSFORM_POINT(lt->pos, context->ModelViewMatrix.m, params);
			if (lt->pos.w == 0.0)
			{
			// Directional light.
				float mag = sqrt(lt->pos.x * lt->pos.x + lt->pos.y * lt->pos.y + lt->pos.z * lt->pos.z);
				float inv_mag = 1.0 / mag;
				lt->dir.x = inv_mag * lt->pos.x;
				lt->dir.y = inv_mag * lt->pos.y;
				lt->dir.z = inv_mag * lt->pos.z;
			}
			break;

		default:
			context->gl_error(GL_INVALID_ENUM, "Lightfv", "Unsupported.");
			break;
	}
}

GLMETHOD(void) glGetLightfv (GLenum light, GLenum pname, GLfloat *params)
{
	GET_CONTEXT(context);
	context->gl_error(GL_INVALID_OPERATION,"GetLight");
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

GLMETHOD(void) glLightModelfv (GLenum pname, const GLfloat *params)
{
	GET_CONTEXT(context);

	switch (pname)
	{
		case GL_LIGHT_MODEL_AMBIENT:
			context->global_ambient.set(params[0], params[1], params[2], params[3]);
			break;

		default:
			context->gl_error(GL_INVALID_OPERATION, "LightModel", "Unsupported.");
	}
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

GLMETHOD(void) glFogf (GLenum pname, GLfloat param)
{
	GET_CONTEXT(context);

	glFogfv(pname, &param);
}

GLMETHOD(void) glFogi (GLenum pname, GLint param)
{
	GET_CONTEXT(context);

	float fparam = (float) param;
	glFogfv(pname, &fparam);
}

// STOLEN FROM MESA.
/* Convert GLint in [-2147483648,2147483647] to GLfloat in [-1.0,1.0] */
#define INT_TO_FLOAT(I)		((2.0F * (I) + 1.0F) * (1.0F/4294967294.0F))

GLMETHOD(void) glFogiv(GLenum pname, GLint * params)
{
	float fparams[4];

	switch (pname)
	{
		case GL_FOG_MODE:
		case GL_FOG_DENSITY:
		case GL_FOG_START:
		case GL_FOG_END:
		case GL_FOG_INDEX:
			*fparams = (float) *params;
			break;

		case GL_FOG_COLOR:
			fparams[0] = INT_TO_FLOAT(params[0]);
			fparams[1] = INT_TO_FLOAT(params[1]);
			fparams[2] = INT_TO_FLOAT(params[2]);
			fparams[3] = INT_TO_FLOAT(params[3]);
			break;
	}

	glFogfv(pname, fparams);
}

GLMETHOD(void) glFogfv (GLenum pname, const GLfloat *params)
{
	GET_CONTEXT(context);
	switch (pname)
	{
		case GL_FOG_MODE:
		{
			GLenum mode = (GLenum) (int) *params;
			if ((mode == GL_LINEAR) || (mode == GL_EXP) || (mode == GL_EXP2))
			{
				context->fog_mode = mode;
			}
			else
			{
				context->gl_error(GL_INVALID_ENUM, "Fog", "Invalid fog mode.");
			}
			break;
		}

		case GL_FOG_START:
			context->fog_start = *params;
			break;

		case GL_FOG_END:
			context->fog_end = *params;
			break;

		case GL_FOG_DENSITY:
			context->fog_density = *params;
			break;

		case GL_FOG_COLOR:
			context->fog_color.set(params[0], params[1], params[2]);
			break;

		default:
			context->gl_error(GL_INVALID_ENUM, "Fogfv", "Unsupported param.");
			break;
	}

	context->modify_fog = true;
}

//---------------------------------------------------------------------------

// MATERIALS

GLMETHOD(void) glMaterialf (GLenum face, GLenum pname, GLfloat param)
{
	ERR_NAME(glMaterialf);
	GET_CONTEXT(context);

	if (context->inside_begin_end())
		ERR_INVALID_OPERATION();

	if (pname != GL_SHININESS)
		ERR_INVALID_ENUM();

	switch (face)
	{
	case GL_FRONT:
		context->mat[0].shininess = param;
		break;
	case GL_BACK:
		context->mat[1].shininess = param;
		break;
	case GL_FRONT_AND_BACK:
		context->mat[0].shininess =
		context->mat[1].shininess = param;
		break;
	default:
		ERR_INVALID_ENUM();
	}
}

GLMETHOD(void) glMaterialfv (GLenum face, GLenum pname, const GLfloat *param)
{
	ERR_NAME(glMaterialfv);
	GET_CONTEXT(context);

	if (context->inside_begin_end())
		ERR_INVALID_OPERATION();

	if (pname == GL_AMBIENT_AND_DIFFUSE)
	{
		glMaterialfv(face,GL_AMBIENT,param);
		glMaterialfv(face,GL_DIFFUSE,param);
		return;
	}

	MATERIAL *m1,*m2;
	switch (face)
	{
	case GL_FRONT:
		m1 = m2 = context->mat+0;
		break;
	case GL_BACK:
		m1 = m2 = context->mat+1;
		break;
	case GL_FRONT_AND_BACK:
		m1 = context->mat+0;
		m2 = context->mat+1;
		break;
	default:
		ERR_INVALID_ENUM();
		return;
	}

	switch (pname)
	{
	case GL_SHININESS:
		m1->shininess =
		m2->shininess = param[0];
		break;

	case GL_AMBIENT:
		m1->ambient.set(param[0], param[1], param[2], param[3]);
		m2->ambient.set(param[0], param[1], param[2], param[3]);
		break;

	case GL_DIFFUSE:
		m1->diffuse.set(param[0], param[1], param[2], param[3]);
		m2->diffuse.set(param[0], param[1], param[2], param[3]);
		break;

	case GL_SPECULAR:
		m1->specular.set(param[0], param[1], param[2], param[3]);
		m2->specular.set(param[0], param[1], param[2], param[3]);
		break;

	default:
		ERR_INVALID_ENUM();
		return;
	}
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

GLMETHOD(void) glColorMaterial (GLenum face, GLenum mode)
{
	ERR_NAME(glColorMaterial);
	GET_CONTEXT(context);
	ERR_UNSUPPORTED();
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

GLMETHOD(void) glGetMaterial (GLenum face, GLenum pname, GLfloat * params)
{
	ERR_NAME(glGetMaterial);
	GET_CONTEXT(context);
	ERR_UNSUPPORTED();
}

//---------------------------------------------------------------------------

// TEXTURE

GLMETHOD(void) glGenTextures (GLsizei n, GLuint *textures)
{
	ERR_NAME(glGenTextures);
	GET_CONTEXT(context);

	if (context->inside_begin_end())
		ERR_INVALID_OPERATION();

	if (n < 0)
		ERR_INVALID_VALUE();

	for (int i = 0; i < n; i++)
	{
		while (glIsTexture(context->texture_generator))
			context->texture_generator++;

		textures[i] = context->texture_generator++;
	}
}

GLMETHOD(GLboolean) glIsTexture (GLuint texture)
{
	GET_CONTEXT_RET(context,false);
	return (texture != 0) && (context->TextureMgr::find_texture(texture) != 0);
}

GLMETHOD(GLboolean) glAreTexturesResident (GLsizei n, GLuint *textures, GLboolean *residences) 
{
	GET_CONTEXT_RET(context,false);
	GLboolean result = context->are_textures_resident(n, textures, residences);
	return result;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

GLMETHOD(void) glPrioritizeTextures (GLsizei n, GLuint *textures, GLclampf *priorities)
{
	ERR_NAME(glPrioritizeTextures);
	GET_CONTEXT(context);

	if (context->inside_begin_end())
		ERR_INVALID_OPERATION();

	if (n < 0)
		ERR_INVALID_VALUE();

	for (int i=0; i<n; i++)
	{
		TextureObject *txm = context->find_texture(textures[i]);
		if (txm)
		{
			txm->set_priority(priorities[i]);
		}
	}
// FUTURE: when is the best time to shuffle memory?
//	TextureMgr::analyze_priority();
}

GLMETHOD(void) glDeleteTextures (GLsizei n, const GLuint *textures)
{
	ERR_NAME(glDeleteTextures);
	GET_CONTEXT(context);

	if (!context->ready_cmd)
		ERR_INVALID_OPERATION();

	const GLuint * src = textures;
	for (int i = 0; i < n; i++, src++)
	{
		if (*src < context->texture_generator)
		{
			context->texture_generator = *src;
		}

		TextureObject *obj = context->find_texture(*src);
		if (obj)
		{
			context->delete_texture(obj);
		}
		else
		{
		//
		// NOT NECESSARILY AN ERROR. Someone could've glGenTexture()'d
		// a new ID but never used it.
		//
			ERR_INVALID_VALUE(); // "invalid texture specified."
		}
	}
}

GLMETHOD(void) glGetTexParameter (GLenum target, GLenum pname, GLint *params)
{
	ERR_NAME(glGetTexParameter);
	GET_CONTEXT(context);
	ERR_UNSUPPORTED();
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

GLMETHOD(void) glBindTexture (GLenum target, GLuint texture)
{
	ERR_NAME(glBindTexture);
	GET_CONTEXT(context);

	if (context->inside_begin_end())
		ERR_INVALID_OPERATION();

	if (target != GL_TEXTURE_2D)	// Minimal support!
		ERR_UNSUPPORTED();

	context->cmd_bind_texture(texture);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

GLMETHOD(void) glPixelMapfv (GLenum map, GLint mapsize, const GLfloat *values)
// const GLfloat *values
{
	ERR_NAME(glPixelMapfv);
	GET_CONTEXT(context);

	if (context->inside_begin_end())
		ERR_INVALID_OPERATION();

	if (mapsize != 256)	// TEMPORARY: strict limitations
	if (mapsize > 256)
		ERR_INVALID_VALUE();

	int pos;

	switch (map)
	{
		case GL_PIXEL_MAP_I_TO_R:
			pos = 0;
			break;
		case GL_PIXEL_MAP_I_TO_G:
			pos = 1;
			break;
		case GL_PIXEL_MAP_I_TO_B:
			pos = 2;
			break;
		case GL_PIXEL_MAP_I_TO_A:
			{
				for (int i=0; i<mapsize; i++)
					context->alpha_map[i] = clamp_color(values[i]);
				return;
			}
//			pos = 3;
			break;

		default:
			ERR_INVALID_ENUM();
			return;
	}

	for (int i=0; i<mapsize; i++)
	{
		context->pixel_map[i*3+pos] = clamp_color(values[i]);
	}
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

GLMETHOD(void) glTexParameteri (GLenum target, GLenum pname, GLint param)
{
	ERR_NAME(glTexParameteri);
	GET_CONTEXT(context);

	if (context->inside_begin_end())
		ERR_INVALID_OPERATION();

	if (target != GL_TEXTURE_2D)	// Minimal support!
		ERR_UNSUPPORTED();

	switch (pname)
	{
	case GL_TEXTURE_MIN_FILTER:
		context->TextureMgr::set_min_filter((GLenum)param);
		break;
	case GL_TEXTURE_MAG_FILTER:
		context->TextureMgr::set_mag_filter((GLenum)param);
		break;

	case GL_TEXTURE_WRAP_S:
		context->TextureMgr::set_wrap_s((GLenum)param);
		break;

	case GL_TEXTURE_WRAP_T:
		context->TextureMgr::set_wrap_t((GLenum)param);
		break;
	}
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

GLMETHOD(void) glTexImage2D (GLenum target, GLint level, GLint internal_format,
	GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels)
{
	ERR_NAME(glTexImage2D);
	GET_CONTEXT(context);

	if (!context->ready_cmd)
		ERR_INVALID_OPERATION();

	if (target != GL_TEXTURE_2D ||
		type != GL_UNSIGNED_BYTE ||
		border != 0)
		ERR_INVALID_ENUM();

	if (!(internal_format == 3 || internal_format == GL_RGB || internal_format == GL_RGBA4 || internal_format == GL_RGB5_A1 || internal_format == GL_COLOR_INDEX8_EXT))
		ERR_INVALID_ENUM();

	context->register_texture(level, internal_format, width, height, format, (U8 *)pixels, (RGB *)context->pixel_map, context->alpha_map);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

GLMETHOD(void) glTexEnvi (GLenum target, GLenum pname, GLint param)
{
	ERR_NAME(glTexEnvi);
	GET_CONTEXT(context);

	if (!context->ready_cmd)
		ERR_INVALID_OPERATION();

	if (target != GL_TEXTURE_ENV || pname != GL_TEXTURE_ENV_MODE)
		ERR_INVALID_ENUM();

	context->env_mode = (GLenum)param;
	context->modify_pixels = true;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

GLMETHOD(void) glGetTexParameteriv (GLenum target, GLenum pname, GLint *params)
{
	GET_CONTEXT(context);
	if (target == GL_TEXTURE_2D)
	{
		context->get_tex_param(pname, params);
	}
	else
	{
		ERR_UNSUPPORTED();
	}
}

GLMETHOD(void) glGetTexLevelParameteriv (GLenum target, GLint level, GLenum pname, GLint *params)
{
	GET_CONTEXT(context);
	if (target == GL_TEXTURE_2D)
	{
		context->get_tex_level_param(pname, level, params);
	}
	else
	{
		ERR_UNSUPPORTED();
	}
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

GLMETHOD(void) glCopyTexImage2D (GLenum target, GLint level, GLenum internalFormat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border)
{
	GET_CONTEXT(context);
	ERR_UNSUPPORTED();
}

GLMETHOD(void) glCopyTexSubImage2D (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height)
{
	ERR_NAME(glCopyTexSubImage2D);
	GET_CONTEXT(context);
	ERR_UNSUPPORTED();
}

GLMETHOD(void) glTexSubImage2D (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels)
{
	ERR_NAME(glTexSubImage2D);
	GET_CONTEXT(context);
	ERR_UNSUPPORTED();
}

//---------------------------------------------------------------------------

// Bitmaps & Pixels

GLMETHOD(void) glRasterPos3f (float x, float y, float z)
{
	GET_CONTEXT(context);

	context->raster_pos.x = x;
	context->raster_pos.y = y;
	context->raster_pos.z = z;

	context->raster_color = context->vcolor;
}

GLMETHOD(void) glDrawBuffer(GLenum mode)
{
// unsupported.
}

//

#if 0

	GLMETHOD(void) Bitmap (int width, int height, GLfloat xorig, GLfloat yorig, GLfloat xmove, GLfloat ymove, const GLubyte * bitmap)
	{
		ERR_UNSUPPORTED();

		if (width<0 || height<0)
		{ gl_error(GL_INVALID_VALUE,"Bitmap"); return; }

		if (inside_begin_end())
		{ gl_error(GL_INVALID_OPERATION,"glBitmap"); return; }
	}

	GLMETHOD(void) UseFontBitmaps (int width, int height, GLfloat xorig, GLfloat yorig, GLfloat xmove, GLfloat ymove, const GLubyte * bitmap)
	{
		gl_error(GL_INVALID_OPERATION); // Not yet!
	}

	//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

	GLenum draw_buffer;
	GLenum read_buffer;

	GLMETHOD(void) glDrawBuffer (GLenum mode)
	{
		draw_buffer = mode;
		ERR_UNSUPPORTED();
	}

	GLMETHOD(void) glReadBuffer (GLenum mode)
	{
		read_buffer = mode;
		ERR_UNSUPPORTED();
	}

	//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

	GLMETHOD(void) glCopyPixels (GLint x, GLint y, GLsizei width, GLsizei height, GLenum type)
	{
		ERR_UNSUPPORTED();
	}

	GLMETHOD(void) glReadPixels (GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels)
	{
		ERR_UNSUPPORTED();
	}

#endif

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

GLMETHOD(void) glDrawPixels (GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels)
{
	ERR_NAME(glDrawPixels);
	GET_CONTEXT(context);

	if (!context->ready_cmd)
		ERR_INVALID_OPERATION();

// CLIPPING

	uchar *src = (uchar *)pixels;

	int x0 = context->raster_pos.x;
	int y0 = context->raster_pos.y;

	int x1 = x0 + width;
	int y1 = y0 + height;

	int l = context->window_x;
	int r = l + context->window_w;
	int t = context->window_y;
	int b = t + context->window_h;

	if (x0 >= r)
	{
		return;
	}
	if (x0 < l)
	{
		src = (uchar *)BYTE_OFFSET(src,l-x0);
		x0 = l;
	}
	if (x1 < l)
	{
		return;
	}
	if (x1 > r)
	{
		x1 = r;
	}

	if (y0 >= b)
	{
		return;
	}
	if (y0 < t)
	{
		src = (uchar *)BYTE_OFFSET(src,(t-y0)*width);
		y0 = t;
	}
	if (y1 < t)
	{
		return;
	}
	if (y1 > b)
	{
		y1 = b;
	}

	int w = x1 - x0;
	int h = y1 - y0;

// HANDLE PIXEL FORMAT

	switch (format)
	{
		case GL_RGB:
			if (type != GL_UNSIGNED_BYTE)
				ERR_UNSUPPORTED();

			// TODO...
			break;

		case GL_COLOR_INDEX:
		{
			if (type != GL_UNSIGNED_BYTE)
				ERR_UNSUPPORTED();

			context->update_palette();

			void *buffer;
			int stride;

			glLockBufferEXT(&buffer,&stride);
			if (buffer)
			{
				WORD *dst = (WORD *)buffer + x0;
				dst = (WORD *)BYTE_OFFSET(dst,y0*stride);

				for (int yy=0; yy<h; yy++)
				{
					int xx = w;
					while (--xx >= 0)
					{
						dst[xx] = context->native_palette[ src[xx] ];
					}
					dst = (WORD *)BYTE_OFFSET(dst,stride);
					src = (uchar *)BYTE_OFFSET(src,width);
				}
			}
			glUnlockBufferEXT();

			break;
		}

		default:
			ERR_UNSUPPORTED();
	}
}

//---------------------------------------------------------------------------
// GLU Extensions
//---------------------------------------------------------------------------

GLMETHOD(void) gluOrtho2D (double l, double r, double b, double t)
{
	GET_CONTEXT(context);
	context->ortho_matrix(l,r,b,t,-1,+1);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

GLMETHOD(void) gluPerspective (double fovy, double aspect, double z0, double z1)
{
	float fy = z0 * tan(fovy * DEG2RAD);
	float fx = fy * aspect;

	glFrustum(-fx,fx,-fy,fy, z0,z1);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

GLMETHOD(void) glColorTableEXT (GLenum target, GLenum internal_format, GLsizei width, GLenum format, GLenum type, const GLvoid * data)
{
	ERR_NAME(glColorTableEXT);
	GET_CONTEXT(context);

	if (!context->ready_cmd)
		ERR_INVALID_OPERATION();

	if ((target != GL_TEXTURE_2D) ||
		(internal_format != GL_RGB8) ||
		(width > 256) ||
		(format != GL_RGB) ||
		(type != GL_UNSIGNED_BYTE))
	{
		ERR_UNSUPPORTED();	// Minimal support!
	}

	int size = width * 3;
	memcpy(context->pixel_map, data, size);
}

//
// Extra functions used for communication with IDisplay...
//
//#error IN BLITTING CASE, maybe we don't need to throw away surfaces

void SetFullscreen(bool full)
{
	DRAW.fullscreen = full;
}

void FreeContextSurfaces(void)
{
	GET_CONTEXT(context);
	DRAW.free_surfaces(&context->draw_context);
	context->flush_textures(INT_MAX);
}

//

void AllocContextSurfaces(void)
{
	GET_CONTEXT(context);
	DRAW.alloc_surfaces(&context->draw_context);
	
}
//---------------------------------------------------------------------------
//
// DLLMain() called on startup/shutdown
//
//---------------------------------------------------------------------------

BOOL COMAPI DllMain (HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	switch (fdwReason)
	{
		case DLL_PROCESS_ATTACH:
			TheSystem.startup();
			break;

		case DLL_PROCESS_DETACH:
			TheSystem.shutdown();
			break;
	}

	return TRUE;
}

//---------------------------------------------------------------------------
