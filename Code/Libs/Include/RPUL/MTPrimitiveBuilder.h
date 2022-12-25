// MTPrimitiveBuilder.h
//
//
//

#ifndef MTPRIMITIVEBUILDER_H
#define MTPRIMITIVEBUILDER_H

#include "PrimitiveBuilder.h"

struct MTVERTEX
{
	Vector			pos;
	union {
		struct {
		unsigned char	b, g, r, a;
		};
		unsigned int color;
	};
	union {
		struct {
		unsigned char	sb, sg, sr, sa;
		};
		unsigned int scolor;
	};
	float			u,  v;
	float			u2, v2;
};

const U32 MTVERTEX_FVFFLAGS = D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_SPECULAR | D3DFVF_TEX2;

class MTPrimitiveBuilder : public TPrimitiveBuilder<MTVERTEX,MTVERTEX_FVFFLAGS>
{
public:
	MTPrimitiveBuilder( IRenderPipeline *pipeline=NULL, UINT blocksize=128 );
	MTPrimitiveBuilder( IRenderPrimitive *primitive, UINT blocksize=128 );

	void MultiTexCoord2f( float s, float t );
	void Vertex3f( float x, float y, float z );
	void Vertex3fv( const float * v);
	void Vertex2f( float x, float y );
	void Vertex2fv( const float * v );
	float last_s2, last_t2;
};

//

inline MTPrimitiveBuilder::MTPrimitiveBuilder( IRenderPipeline *pipeline, UINT blocksize )
{
	last_s2 = last_t2 = 0.0f;
	fvf_vertex_flags = MTVERTEX_FVFFLAGS;
}


// --------------------------------------------------------------------------
//
//
//
inline MTPrimitiveBuilder::MTPrimitiveBuilder( IRenderPrimitive *primitive, UINT blocksize )
{
	last_s2 = last_t2 = 0.0f;
	fvf_vertex_flags = MTVERTEX_FVFFLAGS;
}

//

inline void MTPrimitiveBuilder::MultiTexCoord2f( float s, float t )
{
	last_s2 = s;
	last_t2 = t;
}

//

inline void MTPrimitiveBuilder::Vertex3f( float _x, float _y, float _z )
{
	if( watch_quad ) {
		if( (quad_vert_count & 3) == 3 ) {
			current_vertex += 2;
			VerifyBuffer();
			PB_COPY_VERTEX(current_vertex-2,current_vertex-5);	// vert 0 of quad
			PB_COPY_VERTEX(current_vertex-1,current_vertex-3);	// vert 2 of quad
		}
		quad_vert_count++;
	}

//	vertex_buffer[current_vertex].dummy0 = 0;
//	vertex_buffer[current_vertex].dummy1 = 0;

	vertex_buffer[current_vertex].r = last_r;
	vertex_buffer[current_vertex].g = last_g;
	vertex_buffer[current_vertex].b = last_b;
	vertex_buffer[current_vertex].a = last_a;
	vertex_buffer[current_vertex].u = last_s;
	vertex_buffer[current_vertex].v = last_t;
	vertex_buffer[current_vertex].u2 = last_s2;
	vertex_buffer[current_vertex].v2 = last_t2;
	vertex_buffer[current_vertex].pos.x = _x;
	vertex_buffer[current_vertex].pos.y = _y;
	vertex_buffer[current_vertex].pos.z = _z;
	current_vertex++;
	VerifyBuffer();
}
//

inline void MTPrimitiveBuilder::Vertex3fv( const float * _v )
{
	Vertex3f(_v[0], _v[1], _v[2]);
}

//

inline void MTPrimitiveBuilder::Vertex2f( float _x, float _y )
{
	Vertex3f(_x, _y, 0.0f);
}

//

inline void MTPrimitiveBuilder::Vertex2fv( const float * _v )
{
	Vertex3f(_v[0], _v[1], 0.0f);
}



#endif

