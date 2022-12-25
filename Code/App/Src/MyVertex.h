#ifndef MYVERTEX_H
#define MYVERTEX_H

//--------------------------------------------------------------------------//
//                                                                          //
//                               MyVertex.h                                 //
//                                                                          //
//                  COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/MyVertex.h 15    8/14/00 10:51p Rmarr $
*/			    
//--------------------------------------------------------------------------//


#ifndef PRIMITIVEBUILDER_H
#include <RPUL\PrimitiveBuilder.h>
#endif

#ifndef CQTRACE_H
#include "CQTrace.h"
#endif

#undef MYVERTEXEXTERN
#ifdef BUILD_TRIM
#define MYVERTEXEXTERN __declspec(dllexport)
#else
#define MYVERTEXEXTERN __declspec(dllimport)
#endif


struct Vertex2
{
	Vector			pos;
	union {
		struct {
		unsigned char	b, g, r, a;
		};
		unsigned int color;
	};
	float			u,v;
	float			u2,v2;
};

struct PartVertex
{
	union {
		struct {
		unsigned char	b, g, r, a;
		};
		unsigned int color;
	};
	float			u,v;
};

typedef RPVertex FVFVERTEXTYPE;

//#define PB_COPY_VERTEX(didx, sidx) 	vertex_buffer[(didx)] = vertex_buffer[(sidx)];

#define D3DFVF_RPVERTEX2 (D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX2)
//typedef TPrimitiveBuilder<Vertex2,D3DFVF_RPVERTEX2> 
struct PrimitiveBuilder2 : PartVertex
{
	public:
	PrimitiveBuilder2( UINT blocksize=128 );
//	PrimitiveBuilder2( IRenderPipeline *pipeline=NULL, UINT blocksize=128 );
//	PrimitiveBuilder2( IRenderPrimitive *primitive, UINT blocksize=128 );
	MYVERTEXEXTERN ~PrimitiveBuilder2();

	void SetPipeline( IRenderPipeline *pipeline );
	void SetIRenderPrimitive( IRenderPrimitive *primitive );
	void Begin( D3DPRIMITIVETYPE type );
	MYVERTEXEXTERN void Begin( PBenum type );
	MYVERTEXEXTERN void Begin( PBenum type, U32 num_verts);
	void Color4f( float r, float g, float b, float a );
	void Color3f( float r, float g, float b );
	int  MakeColor4ub( unsigned char r, unsigned char g, unsigned char b, unsigned char a );
	void Color ( int _color );
	void Color4ub( unsigned char r, unsigned char g, unsigned char b, unsigned char a );
	void Color3ub( unsigned char r, unsigned char g, unsigned char b );
	void TexCoord2f( float s, float t );
	void Vertex3fv( const float * v);
	void Vertex2f( float x, float y );
	void Vertex2fv( const float * v );
	void End();
	void Draw( D3DPRIMITIVETYPE type );
	void Reset(void);
	void Vertex3f( float _x, float _y, float _z );
	void Vertex3f_NC( float _x, float _y, float _z );
	void Vertex3f( const Vector &vec );
	void Vertex3f_NC( const Vector &vec );

	MYVERTEXEXTERN void VerifyBuffer( void );

	D3DPRIMITIVETYPE type;
	RPVertex *vertex_buffer;
	U8 *vertex_buffer_real;
	UINT current_vertex;
	FVFVERTEXTYPE *current_vertex_ptr;
	//U32  fvf_vertex_flags;
protected:
	IRenderPipeline *render_pipeline;
	IRenderPrimitive *render_primitive;
	UINT block_size;
	UINT num_vertex;

	bool insert_at_end;
	bool watch_quad;
	UINT quad_vert_count;

//	unsigned char last_r,last_g,last_b,last_a;
/*	int last_color;
	float last_s, last_t;
	float last_s2, last_t2;*/
};

inline void PrimitiveBuilder2::Vertex3f( float _x, float _y, float _z )
{
	if( watch_quad ) {
		if( (quad_vert_count&3) == 3 ) {
			current_vertex += 2;
			current_vertex_ptr += 2;
			VerifyBuffer();
			PB_COPY_VERTEX(current_vertex-2,current_vertex-5);	// vert 0 of quad
			PB_COPY_VERTEX(current_vertex-1,current_vertex-3);	// vert 2 of quad
		}
		quad_vert_count++;
	}

	current_vertex_ptr->pos.set(_x,_y,_z);
	//unsafe casting going on to do the minimum memory movement into the vertex buffer
	*(PartVertex *)((U8 *)(current_vertex_ptr)+sizeof(Vector)) = *this;

	current_vertex++;
	current_vertex_ptr++;
	VerifyBuffer();
}

inline void PrimitiveBuilder2::Vertex3f_NC( float _x, float _y, float _z )
{
	current_vertex_ptr->pos.set(_x,_y,_z);
	//unsafe casting going on to do the minimum memory movement into the vertex buffer
	*(PartVertex *)((U8 *)(current_vertex_ptr)+sizeof(Vector)) = *this;
	current_vertex++;
	current_vertex_ptr++;
}

inline void PrimitiveBuilder2::Vertex3f( const Vector &vec )
{
	if( watch_quad ) {
		if( (quad_vert_count&3) == 3 ) {
			current_vertex += 2;
			current_vertex_ptr +=2;
			VerifyBuffer();
			PB_COPY_VERTEX(current_vertex-2,current_vertex-5);	// vert 0 of quad
			PB_COPY_VERTEX(current_vertex-1,current_vertex-3);	// vert 2 of quad
		}
		quad_vert_count++;
	}



//	vertex_buffer[current_vertex] = *this;
	current_vertex_ptr->pos = vec;
	//unsafe casting going on to do the minimum memory movement into the vertex buffer
	*(PartVertex *)((U8 *)(current_vertex_ptr)+sizeof(Vector)) = *this;
	current_vertex++;
	current_vertex_ptr++;
	VerifyBuffer();
}

inline void PrimitiveBuilder2::Vertex3f_NC( const Vector &vec )
{

	current_vertex_ptr->pos = vec;
	//unsafe casting going on to do the minimum memory movement into the vertex buffer
	*(PartVertex *)((U8 *)(current_vertex_ptr)+sizeof(Vector)) = *this;
	current_vertex++;
	current_vertex_ptr++;
}

/*inline void PrimitiveBuilder2::MulCoord2f( float _u, float _v)
{
	u2 = _u;
	v2 = _v;
}*/

inline PrimitiveBuilder2::PrimitiveBuilder2(UINT blocksize)
{
	vertex_buffer = NULL;
	vertex_buffer_real = NULL;
	num_vertex = 0;
	block_size = blocksize;
	type = D3DPT_POINTLIST;
//	last_r = last_g = last_b = last_a = 255;
	color = -1;
	u = v = 0.00;
}
// --------------------------------------------------------------------------
//
inline void PrimitiveBuilder2::SetPipeline( IRenderPipeline *pipeline )
{
	if( render_pipeline ) {
		render_pipeline->Release();
		render_pipeline = NULL;
	}
	if( pipeline ) {
		render_pipeline = pipeline;
		render_pipeline->AddRef();
	}
}
// --------------------------------------------------------------------------
//
//
//
inline void PrimitiveBuilder2::SetIRenderPrimitive( IRenderPrimitive *primitive )
{
	if( render_primitive ) {
		render_primitive->Release();
		render_primitive = NULL;
	}
	if( primitive ) {
		render_primitive = primitive;
		render_primitive->AddRef();
	}
}
// --------------------------------------------------------------------------
//
inline void PrimitiveBuilder2::Reset( void )
{
	current_vertex = 0;
	current_vertex_ptr = vertex_buffer;
	quad_vert_count = 0;
	watch_quad = false;
	insert_at_end = false;
	quad_vert_count = 0;

	VerifyBuffer();
}
// --------------------------------------------------------------------------
//
inline void PrimitiveBuilder2::Begin( D3DPRIMITIVETYPE _type )
{
	Reset();
	type = _type;
}

// --------------------------------------------------------------------------
//
inline void PrimitiveBuilder2::Color4f( float r, float g, float b, float a )
{
	/**last_r = (unsigned char)(r*255.0);
	last_g = (unsigned char)(g*255.0);
	last_b = (unsigned char)(b*255.0);
	last_a = (unsigned char)(a*255.0);*/
	color = (unsigned char)(a*255.0)<<24|(unsigned char)(r*255.0)<<16|(unsigned char)(g*255.0)<<8|(unsigned char)(b*255.0);
}


// --------------------------------------------------------------------------
//
inline void PrimitiveBuilder2::Color3f( float r, float g, float b )
{
	/*last_r = (unsigned char)(r*255.0);
	last_g = (unsigned char)(g*255.0);
	last_b = (unsigned char)(b*255.0);
	last_a = 255;*/
	color = 255<<24|(unsigned char)(r*255.0)<<16|(unsigned char)(g*255.0)<<8|(unsigned char)(b*255.0);
}


// --------------------------------------------------------------------------
//
inline void PrimitiveBuilder2::Color4ub( unsigned char r, unsigned char g, unsigned char b, unsigned char a )
{
	/*last_r = r;
	last_g = g;
	last_b = b;
	last_a = a;*/
	color = a<<24|r<<16|g<<8|b;
}

inline int  PrimitiveBuilder2::MakeColor4ub( unsigned char r, unsigned char g, unsigned char b, unsigned char a )
{
	return a<<24|r<<16|g<<8|b;
}

inline void PrimitiveBuilder2::Color ( int _color )
{
	color = _color;
}
// --------------------------------------------------------------------------
//
inline void PrimitiveBuilder2::Color3ub( unsigned char r, unsigned char g, unsigned char b )
{
	/*last_r = r;
	last_g = g;
	last_b = b;
	last_a = 255;*/
	color = 0xff<<24|r<<16|g<<8|b;
}


// --------------------------------------------------------------------------
//
inline void PrimitiveBuilder2::TexCoord2f( float s, float t )
{
	u = s;
	v = t;
}


inline void PrimitiveBuilder2::Vertex3fv( const float * _v )
{
	Vertex3f(_v[0], _v[1], _v[2]);
}

//

inline void PrimitiveBuilder2::Vertex2f( float _x, float _y )
{
	Vertex3f(_x, _y, 0.0f);
}

//
inline void PrimitiveBuilder2::Vertex2fv( const float * _v )
{
	Vertex3f(_v[0], _v[1], 0.0f);
}

// --------------------------------------------------------------------------
//
inline void PrimitiveBuilder2::End( void )
{
	if( current_vertex == 0 ) {
		return;
	}

	if( insert_at_end ) {
		PB_COPY_VERTEX(current_vertex,0);
		current_vertex++;
		current_vertex_ptr++;

	}

	if( render_primitive ) {
		render_primitive->draw_primitive( type, D3DFVF_RPVERTEX, vertex_buffer, current_vertex, 0 );
	}
	else if( render_pipeline ) {
		render_pipeline->draw_primitive( type, D3DFVF_RPVERTEX, vertex_buffer, current_vertex, 0 );
	}
}

// --------------------------------------------------------------------------
//
inline void PrimitiveBuilder2::Draw( D3DPRIMITIVETYPE _type )
{
	if( render_primitive ) {
		render_primitive->draw_primitive( type, D3DFVF_RPVERTEX, vertex_buffer, current_vertex, 0 );
	}
	else if( render_pipeline ) {
		render_pipeline->draw_primitive( _type, D3DFVF_RPVERTEX, vertex_buffer, current_vertex, 0 );
	}
}

//This may not belong here

namespace RadixSort
{
	extern U32 * sort_temp;
	extern U32 * index_temp;
	extern U32 * index_list;

	extern void set_sort_size(U32 n);

	
	// complicated expression better fits as macro (or inline in C++)
	#define byte_of(x) (((x) >> bitsOffset) & 0xff)

	extern void radix (short bitsOffset, U32 N, U32 * source, U32 * dest, U32 * isrc, U32 * idst);

	// n is number of ints
	// source is array of ints
	// index_list (U32[n]) will fill in a list of indices which will tell you which slot each unsorted item will be moved to
	
	extern void sort (U32 * source, U32 n);

}


#endif
