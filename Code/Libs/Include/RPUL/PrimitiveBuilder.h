// PrimitiveBuilder.h
//
//
//

#ifndef PRIMITIVEBUILDER_H
#define PRIMITIVEBUILDER_H

#include "rendpipeline.h"
#include "IRenderPrimitive.h"


typedef enum {
	PB_NONE				= 0,
	PB_POINTS			= 0,
	PB_LINES			= 1,
	PB_LINE_LOOP		= 2,
	PB_LINE_STRIP		= 3,
	PB_TRIANGLES		= 4,
	PB_TRIANGLE_STRIP	= 5,
	PB_TRIANGLE_FAN		= 6,
	PB_QUADS			= 7,
	PB_QUAD_STRIP		= 8,
	PB_POLYGON			= 9
} PBenum;


template <class FVFVERTEXTYPE, U32 FVFVERTEXTYPEFLAGS>
class TPrimitiveBuilder
{
public:
	TPrimitiveBuilder( IRenderPipeline *pipeline=NULL, UINT blocksize=128 );
	TPrimitiveBuilder( IRenderPrimitive *primitive, UINT blocksize=128 );
	~TPrimitiveBuilder();

	void SetPipeline( IRenderPipeline *pipeline );
	void SetIRenderPrimitive( IRenderPrimitive *primitive );
	void Begin( D3DPRIMITIVETYPE type );
	void Begin( PBenum type );
	void Color4f( float r, float g, float b, float a );
	void Color3f( float r, float g, float b );
	void Color4ub( unsigned char r, unsigned char g, unsigned char b, unsigned char a );
	void Color3ub( unsigned char r, unsigned char g, unsigned char b );
	void TexCoord2f( float s, float t );
	void Vertex3f( float x, float y, float z );
	void Vertex3fv( const float * v);
	void Vertex2f( float x, float y );
	void Vertex2fv( const float * v );
	void End();
	void Draw( D3DPRIMITIVETYPE type );
	void Reset(void);

	void VerifyBuffer( void );

	D3DPRIMITIVETYPE type;
	FVFVERTEXTYPE *vertex_buffer;
	UINT current_vertex;
	U32  fvf_vertex_flags;
protected:
	IRenderPipeline *render_pipeline;
	IRenderPrimitive *render_primitive;
	UINT block_size;
	UINT num_vertex;

	bool insert_at_end;
	bool watch_quad;
	UINT quad_vert_count;

	unsigned char last_r,last_g,last_b,last_a;
	float last_s, last_t;
};

//

/*class PrimitiveBuilder : public TPrimitiveBuilder<RPVertex,D3DVT_RPVERTEX>
{
public:
	PrimitiveBuilder( IRenderPipeline *pipeline=NULL, UINT blocksize=128 ) : TPrimitiveBuilder<RPVertex,D3DFVF_RPVERTEX>( pipeline, blocksize ) {}
	PrimitiveBuilder( IRenderPrimitive *primitive, UINT blocksize=128 ) : TPrimitiveBuilder<RPVertex,D3DFVF_RPVERTEX>( primitive, blocksize ) {}
	~PrimitiveBuilder() {}
};*/

typedef TPrimitiveBuilder<RPVertex,D3DFVF_RPVERTEX> PrimitiveBuilder;

#include "PrimitiveBuilder_inl.cpp"

#endif

