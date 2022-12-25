//---------------------------------------------------------------------------
/*
	VBUFFER.H

	Copyright (C) 1997 Digital Anvil, Inc.

	Created: October 1997

	Author: Paul Isaac
*/
//---------------------------------------------------------------------------

#ifndef _VBUFFER_H
#define _VBUFFER_H
//---------------------------------------------------------------------------

#define f2i (255.0F)
#define i2f (1.0F/f2i)
// useful for converting colors components from integer (0,255) to float (0,1.0)

#include <malloc.h>		// malloc(), free()
#include <assert.h>		// assert()

#define VINDEX uint

#include "vertex.h"		// _VERTEX

#define MAX_POLY_SIDES	16
#define MAX_VERTS		6144
#define MAX_CHAINS		6144

//---------------------------------------------------------------------------
// ListPtr
//---------------------------------------------------------------------------

template <typename Type> struct ListPtr
{
	Type *ptr;
	int max;

	void set (int _count, Type *_ptr)
	{
		free();
		ptr = _ptr;
		max = _count;
	}

	ListPtr (void)
	{
		ptr = 0;
		max = 0;
	}

	~ListPtr (void)
	{
		free();
	}

	inline void copy (int count, const ListPtr &other, int i=0)
	{
		count = min(count,max);
		memcpy(ptr,other.ptr+i,count*sizeof(Type));
	}

	inline operator Type * (void) const
	{
	 	return (Type *) ptr;
	}
/*
	inline Type * operator -> (void) const
	{
	 	return ptr;
	}
*/
	inline operator bool (void) const 
	{
		return (ptr != 0);
	}

	void free (void)
	{
		if (ptr)
		{
			::free(ptr);
			max = 0;
			ptr = 0;
		}
	}

	void release (void)
	{
		ptr = 0;
	}

	const void *alloc (int count)
	{
		if (ptr) free();
		ptr = (Type *)::malloc(count*sizeof(Type));
		max = count;
		return ptr;
	}
};

//

//#define USE_VERT_STRUCT
#ifdef USE_VERT_STRUCT
struct VBVertex
{
	VECTOR			obj;
	COLOR_VECTOR	color;
	VECTOR			normal;
	TEX_VECTOR		texcoord;
	uint			flag;
};
#endif
//

//---------------------------------------------------------------------------
// VertexBuffer
//---------------------------------------------------------------------------

#define NULL_VERTEX		0xFFFF	// limit VertexBuffer index range to short?

#define BIT_VERTEX		1		// ie. transformed
#define BIT_COLOR		2
#define BIT_TEXCOORD	4
#define BIT_NORMAL		8

struct VertexBuffer
{
	uint		max;		// max allocated vertices
	uint		vcount;		// number vertices used

	uint		start;		// first untransformed vertex

	uint		num_chains;
	VINDEX		vchain_list[MAX_CHAINS];	// used by non-list Vertex() and DrawElements()

// parallel arrays[vcount] outside vlist
#ifdef USE_VERT_STRUCT
	ListPtr<VBVertex>		vert_list;
#else
	ListPtr<VECTOR>			obj_list;		// vertex in object space

	ListPtr<COLOR_VECTOR>	color_list;		// RGBA vector

	ListPtr<VECTOR>			normal_list;	// vector in object space

	ListPtr<TEX_VECTOR>		texcoord_list;	// 2d-point in texture space

	ListPtr<uint>			flag_list;		// BIT_VERTEX, etc.
#endif
// CONSTRUCTION

	VertexBuffer (void)
	{
		max = 0;
		reset();
	}

	~VertexBuffer(void)
	{
	}

	void reset_chains (void)
	{
		start = vcount;
		num_chains = 0;
	}

	void reset (void)
	{
		vcount = 0;
		start = 0;
		num_chains = 0;
	}

	void free (void)
	{
#ifdef USE_VERT_STRUCT
		vert_list.free();
#else
		obj_list.free();

		color_list.free();
		normal_list.free();
		texcoord_list.free();
		flag_list.free();
#endif
		max = 0;
		vcount = 0;
	}

	void alloc (uint num)
	{
		assert(num < NULL_VERTEX);

		free();

		max = num;
		vcount = 0;
#ifdef USE_VERT_STRUCT
		vert_list.alloc(num);
#else
		obj_list.alloc(num);

		color_list.alloc(num);
		normal_list.alloc(num);
		texcoord_list.alloc(num);
		flag_list.alloc(num);
#endif
	}

	void copy (uint num, VertexBuffer &vb, int vb_first=0)
	{
		assert(num <= max);
		vcount = num;
#ifdef USE_VERT_STRUCT
		vert_list.copy(vcount, vb.vert_list, vb_first);
#else
		obj_list.copy(vcount, vb.obj_list,vb_first);
		color_list.copy(vcount, vb.color_list,vb_first);
		normal_list.copy(vcount, vb.normal_list,vb_first);
		texcoord_list.copy(vcount, vb.texcoord_list,vb_first);
		flag_list.copy(vcount, vb.flag_list,vb_first);
#endif
	}

	void flush_chains (uint count)
	// called when using Vertex arrays
	{
		int remainder = num_chains - count;
		if (remainder > 0)
		{
			memcpy(vchain_list,vchain_list+count,remainder*sizeof(vchain_list[0]));
			num_chains = remainder;
		}
		else
		{
			num_chains = 0;
		}
	}

	void flush_vertices (uint count)
	// called when using glVertex cmds
	{
		int remainder = vcount - count;
		if (remainder > 0)
		{
			copy(remainder, *this,count);	// may skip new clipping vertices
			start -= count;
			if (start < 0)
				start = 0;
			for (uint i=0; i<num_chains; i++)
			{
				assert(vchain_list[i] < count);
				vchain_list[i] = vchain_list[i] - count;
			}
		}
		else
		{
			reset();
		}
	}

	void alloc (VertexBuffer &vb)
	{
		alloc(vb.vcount);
		copy(vb.vcount,vb);
	}

	inline void use_vertex (VINDEX vi)
	{
		assert(num_chains < countof(vchain_list));
		vchain_list[num_chains++] = vi;
	}

	inline void set_vertex3 (uint index, float *v)
	{
#ifdef USE_VERT_STRUCT
		VECTOR *dst = &(vert_list.ptr + index)->obj;
#else
		VECTOR *dst = obj_list.ptr + index;
#endif
		dst->x = v[0];
		dst->y = v[1];
		dst->z = v[2];
		dst->w = 1.0;
	}

	inline void set_vertex (uint index, float x, float y, float z, float w=1.0)
	{
		assert(index < max);
#ifdef USE_VERT_STRUCT
		VECTOR *dst = &(vert_list.ptr + index)->obj;
#else
		VECTOR *dst = obj_list.ptr + index;
#endif
		dst->x = x;
		dst->y = y;
		dst->z = z;
		dst->w = w;
	}

	inline void set_vertex (uint index, const VECTOR &v)
	{
		assert(index < max);
#ifdef USE_VERT_STRUCT
		VECTOR *dst = &(vert_list.ptr + index)->obj;
#else
		VECTOR *dst = obj_list.ptr + index;
#endif
		dst->x = v.x;
		dst->y = v.y;
		dst->z = v.z;
		dst->w = v.w;
	}

	inline void set_color (uint index, float r, float g, float b, float a=1.0)
	{
		assert(index < max);
#ifdef USE_VERT_STRUCT
		vert_list[index].color.set(r, g, b, a);
#else
		color_list[index].set(r, g, b, a);
#endif
	}
	inline void set_color (uint index, uchar r, uchar g, uchar b, uchar a=255)
	{
		//set_color(index, r*i2f, g*i2f, b*i2f, a*i2f);
#ifdef USE_VERT_STRUCT
		vert_list[index].color.set(r, g, b, a);
#else
		color_list[index].set(r, g, b, a);
#endif
	}
	inline void set_color (uint index, const COLOR_VECTOR &v)
	{
		//set_color(index, v.r,v.g,v.b,v.a);
#ifdef USE_VERT_STRUCT
		vert_list[index].color.set(v.r, v.g, v.b, v.a);
#else
		color_list[index].set(v.r, v.g, v.b, v.a);
#endif
	}

	inline void set_normal (uint index, const VECTOR &v)
	{
		assert(index < max);
#ifdef USE_VERT_STRUCT
		VECTOR *dst = &(vert_list.ptr + index)->normal;
#else
		VECTOR *dst = normal_list.ptr + index;
#endif
		dst->x = v.x;
		dst->y = v.y;
		dst->z = v.z;
		dst->w = 1.0;
	}

	inline void set_texcoord (uint index, float s, float t, float r=0, float q=0)
	{
		assert(index < max);
#ifdef USE_VERT_STRUCT
		TEX_VECTOR *dst = &(vert_list.ptr + index)->texcoord;
#else
		TEX_VECTOR *dst = texcoord_list.ptr + index;
#endif
		dst->s = s;
		dst->t = t;
		dst->r = r;
		dst->q = q;
	}

	inline void set_texcoord (uint index, const TEX_VECTOR &v)
	{
		assert(index < max);
#ifdef USE_VERT_STRUCT
		TEX_VECTOR *dst = &(vert_list.ptr + index)->texcoord;
#else
		TEX_VECTOR *dst = texcoord_list.ptr + index;
#endif
		dst->s = v.s;
		dst->t = v.t;
		dst->r = v.r;
		dst->q = v.q;
	}

	inline uint new_vertex (void)
	{
		return ((vcount < max) ? vcount++ : NULL_VERTEX);
	}

	inline void set_flag (uint index, uint f)
	{
		assert(index < max);
#ifdef USE_VERT_STRUCT
		vert_list.ptr[index].flag = f;
#else
		flag_list.ptr[index] = f;
#endif
	}

	inline uint test_flag (uint index, uint f)
	{
		assert(index < max);
#ifdef USE_VERT_STRUCT
		return (vert_list.ptr[index].flag & f) == f;
#else
		return (flag_list.ptr[index] & f) == f;
#endif
	}

	void clear_flags (uint f)
	{
		for (uint i=0; i<vcount; i++)
		{
#ifdef USE_VERT_STRUCT
			vert_list.ptr[i].flag &= ~f;
#else
			flag_list.ptr[i] &= ~f;
#endif
		}
	}
};

//---------------------------------------------------------------------------

enum ListCmd
{
	CMD_COLOR,
	CMD_TEXCOORD,
	CMD_VERTEX,

	CMD_BEGIN,
	CMD_END,

	CMD_BIND_TEXTURE,
	CMD_TRANSLATE,

	CMD_ENABLE,
	CMD_DISABLE,
};

//---------------------------------------------------------------------------

enum EnableIndex
{
	ENABLE_NONE=0,		// Unknown Error?

	ENABLE_CULL_FACE,
	ENABLE_SCISSOR_TEST,
	ENABLE_LIGHTING,

	ENABLE_TEXTURE,
	ENABLE_TEXTURE_1D=ENABLE_TEXTURE,
	ENABLE_TEXTURE_2D=ENABLE_TEXTURE,

	ENABLE_BLEND,
	ENABLE_FOG,
	ENABLE_DEPTH_TEST,

	ENABLE_LIGHT0,
	ENABLE_LIGHT1,
	ENABLE_LIGHT2,
	ENABLE_LIGHT3,
	ENABLE_LIGHT4,
	ENABLE_LIGHT5,
	ENABLE_LIGHT6,
	ENABLE_LIGHT7,

	ENABLE_CULL_VERTEX,
	ENABLE_LINE_SMOOTH,
	ENABLE_DITHER,

	ENABLE_MAX
};

//---------------------------------------------------------------------------
// DrawList
//---------------------------------------------------------------------------

struct DrawList : VertexBuffer
{
	DrawList	*next;
	int			id;

	uint		num_cmds;
	uint		cmd_list[4*256];	// FUTURE - remove this limitation!

	int			cmd_begin;

	DrawList (void)
	{
		next = 0;
		id = -1;
		cmd_begin = -1; // NONE

		num_cmds = 0;
	}

	void BindTexture (int texture)
	{
		if (num_cmds+2 > countof(cmd_list))
			return; // overflow!

		cmd_list[num_cmds++] = CMD_BIND_TEXTURE;
		cmd_list[num_cmds++] = texture;
	}

	void Translate (float x, float y, float z)
	{
		if (num_cmds+4 > countof(cmd_list))
			return; // overflow!

		cmd_list[num_cmds++] = CMD_TRANSLATE;
		((float *)cmd_list)[num_cmds++] = x;
		((float *)cmd_list)[num_cmds++] = y;
		((float *)cmd_list)[num_cmds++] = z;
	}

	void Enable (EnableIndex index)
	{
		if (num_cmds+2 > countof(cmd_list))
			return; // overflow!

		cmd_list[num_cmds++] = CMD_ENABLE;
		cmd_list[num_cmds++] = index;
	}

	void Disable (EnableIndex index)
	{
		if (num_cmds+2 > countof(cmd_list))
			return; // overflow!

		cmd_list[num_cmds++] = CMD_DISABLE;
		cmd_list[num_cmds++] = index;
	}

	void Begin (GLenum t)
	{
		if (num_cmds+4 > countof(cmd_list))
			return; // overflow!

		if (cmd_begin != -1)
			return; // already in command, recursive?

		cmd_begin = num_cmds;

		cmd_list[num_cmds++] = CMD_BEGIN;
		cmd_list[num_cmds++] = t;
		cmd_list[num_cmds++] = num_chains;	// v_begin = vchain offset
		cmd_list[num_cmds++] = 0;			// vchain count (calculated at end)
	}

	void use_vertex (VINDEX v_index)
	{
		if (cmd_begin == -1)
			return; // not in a command?
		VertexBuffer::use_vertex(v_index);
	}

	void End (void)
	{
		if (cmd_begin == -1)
			return; // not in a command?

		if (cmd_list[cmd_begin] != CMD_BEGIN)
			return; // wrong command?

		int v_begin = cmd_list[cmd_begin+2];
		int count = num_chains - v_begin;
		if (count > 0)
		{
			cmd_list[cmd_begin+2] = count;
		}
		else
		{
		// Roll back.
			num_cmds = cmd_begin;
		}
		cmd_begin = -1;
	}

	void free (void)
	{
		num_cmds = 0;
		// FUTURE: alloc/free cmd_list
	}
};

//---------------------------------------------------------------------------
// ListMgr
//---------------------------------------------------------------------------

struct ListMgr
{
	DrawList *head;

	ListMgr (void)
	{
		head = 0;
	}

	~ListMgr(void)
	{
		DrawList * list = head;
		while (list)
		{
			DrawList * next = list->next;
			delete list;
			list = next;
		}
	}

	DrawList *find_list (int id)
	{
		DrawList *p;
		for (p=head; p; p=p->next)
		{
			if (p->id == id)
				break;
		}
		return p;
	}

	DrawList *new_list (int id)
	{
		if (find_list(id))
			return 0; // Error!

		DrawList *p = new DrawList;
		if (p)
		{
			p->id = id;
			p->next = head;
			head = p;
		}
		return p;
	}

	void unlink (DrawList *l, DrawList *prev)
	{
		if (head == l)
		{
			head = l->next;
		}
		else
		{
			assert(prev);
			prev->next = l->next;
		}
		l->next = 0;
	}

	void remove_list (int id)
	{
		DrawList *prev = 0;
		for (DrawList *l=head; l; l=l->next)
		{
			if (l->id == id)
			{
				l->free();
				unlink(l,prev);
				delete l;
				break;
			}
			prev = l;
		}
	}
};

//---------------------------------------------------------------------------

#endif // _VBUFFER_H
