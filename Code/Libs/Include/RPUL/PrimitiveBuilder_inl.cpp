// PrimitiveBuilder_inl.cpp
//
//
//



#define PB_COPY_VERTEX(didx, sidx) 	vertex_buffer[(didx)] = vertex_buffer[(sidx)];

// --------------------------------------------------------------------------
//
//
//
template <class FVFVERTEXTYPE, U32 FVFVERTEXTYPEFLAGS>
inline TPrimitiveBuilder<FVFVERTEXTYPE,FVFVERTEXTYPEFLAGS>::TPrimitiveBuilder( IRenderPipeline *pipeline, UINT blocksize )
{
	vertex_buffer = NULL;
	num_vertex = 0;
	block_size = blocksize;
	type = D3DPT_POINTLIST;
	last_r = last_g = last_b = last_a = 255;
	last_s = last_t = 0.00;
	fvf_vertex_flags = FVFVERTEXTYPEFLAGS;

	Reset();

	render_pipeline = pipeline;
	if( render_pipeline ) {
		render_pipeline->AddRef();
	}

	render_primitive = NULL;
}


// --------------------------------------------------------------------------
//
//
//
template <class FVFVERTEXTYPE, U32 FVFVERTEXTYPEFLAGS>
inline TPrimitiveBuilder<FVFVERTEXTYPE,FVFVERTEXTYPEFLAGS>::TPrimitiveBuilder( IRenderPrimitive *primitive, UINT blocksize )
{
	vertex_buffer = NULL;
	num_vertex = 0;
	block_size = blocksize;
	type = D3DPT_POINTLIST;
	last_r = last_g = last_b = last_a = 255;
	last_s = last_t = 0.00;
	fvf_vertex_flags = FVFVERTEXTYPEFLAGS;

	Reset();

	render_primitive = primitive;
	if( render_primitive ) {
		render_primitive->AddRef();
	}

	render_pipeline = NULL;
}


// --------------------------------------------------------------------------
//
//
//
template <class FVFVERTEXTYPE, U32 FVFVERTEXTYPEFLAGS>
inline TPrimitiveBuilder<FVFVERTEXTYPE,FVFVERTEXTYPEFLAGS>::~TPrimitiveBuilder( )
{
	if( render_pipeline ) {
		render_pipeline->Release();
		render_pipeline = NULL;
	}

	if( render_primitive ) {
		render_primitive->Release();
		render_primitive = NULL;
	}

	delete[] vertex_buffer;
	vertex_buffer = NULL;
}


// --------------------------------------------------------------------------
//
//
//
template <class FVFVERTEXTYPE, U32 FVFVERTEXTYPEFLAGS>
inline void TPrimitiveBuilder<FVFVERTEXTYPE,FVFVERTEXTYPEFLAGS>::SetPipeline( IRenderPipeline *pipeline )
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
template <class FVFVERTEXTYPE, U32 FVFVERTEXTYPEFLAGS>
inline void TPrimitiveBuilder<FVFVERTEXTYPE,FVFVERTEXTYPEFLAGS>::SetIRenderPrimitive( IRenderPrimitive *primitive )
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
//
//
template <class FVFVERTEXTYPE, U32 FVFVERTEXTYPEFLAGS>
inline void TPrimitiveBuilder<FVFVERTEXTYPE,FVFVERTEXTYPEFLAGS>::VerifyBuffer( void )
{
	if( current_vertex >= num_vertex ) {
		FVFVERTEXTYPE *v;
		if( (v= new FVFVERTEXTYPE[num_vertex+block_size]) != NULL ) {
			if (vertex_buffer != NULL) {
				memcpy( v, vertex_buffer, sizeof(FVFVERTEXTYPE)*num_vertex );			
			}
			delete[] vertex_buffer;
			vertex_buffer = v;
			num_vertex += block_size;
		}
	}
}

// --------------------------------------------------------------------------
//
//
//
template <class FVFVERTEXTYPE, U32 FVFVERTEXTYPEFLAGS>
inline void TPrimitiveBuilder<FVFVERTEXTYPE,FVFVERTEXTYPEFLAGS>::Reset( void )
{
	current_vertex = 0;
	quad_vert_count = 0;
	watch_quad = false;
	insert_at_end = false;
	quad_vert_count = 0;

	VerifyBuffer();
}

// --------------------------------------------------------------------------
//
//
//
template <class FVFVERTEXTYPE, U32 FVFVERTEXTYPEFLAGS>
inline void TPrimitiveBuilder<FVFVERTEXTYPE,FVFVERTEXTYPEFLAGS>::Begin( PBenum _type )
{
	Reset();
	switch( _type ) {
	case PB_POINTS:		
		type = D3DPT_POINTLIST;		
		insert_at_end = false;
		watch_quad = false;
		break;
	case PB_LINES:		
		type = D3DPT_LINELIST;		
		insert_at_end = false;
		watch_quad = false;
		break;
	case PB_LINE_LOOP:
		type = D3DPT_LINESTRIP;		
		insert_at_end = true;
		watch_quad = false;
		break;
	case PB_LINE_STRIP:
		type = D3DPT_LINESTRIP;		
		insert_at_end = false;
		watch_quad = false;
		break;
	case PB_TRIANGLES:
		type = D3DPT_TRIANGLELIST;
		insert_at_end = false;
		watch_quad = false;
		break;
	case PB_TRIANGLE_STRIP:
		type = D3DPT_TRIANGLESTRIP;
		insert_at_end = false;
		watch_quad = false;
		break;
	case PB_POLYGON:
	case PB_TRIANGLE_FAN:
		type = D3DPT_TRIANGLEFAN;
		insert_at_end = false;
		watch_quad = false;
		break;
	case PB_QUAD_STRIP:
	case PB_QUADS:
		type = D3DPT_TRIANGLELIST;
		insert_at_end = false;
		watch_quad = true;
		break;

	default:
		break;
	}
}

// --------------------------------------------------------------------------
//
//
//
template <class FVFVERTEXTYPE, U32 FVFVERTEXTYPEFLAGS>
inline void TPrimitiveBuilder<FVFVERTEXTYPE,FVFVERTEXTYPEFLAGS>::Begin( D3DPRIMITIVETYPE _type )
{
	Reset();
	type = _type;
}

// --------------------------------------------------------------------------
//
//
//
template <class FVFVERTEXTYPE, U32 FVFVERTEXTYPEFLAGS>
inline void TPrimitiveBuilder<FVFVERTEXTYPE,FVFVERTEXTYPEFLAGS>::Color4f( float r, float g, float b, float a )
{
	last_r = (unsigned char)(r*255.0);
	last_g = (unsigned char)(g*255.0);
	last_b = (unsigned char)(b*255.0);
	last_a = (unsigned char)(a*255.0);
}


// --------------------------------------------------------------------------
//
//
//
template <class FVFVERTEXTYPE, U32 FVFVERTEXTYPEFLAGS>
inline void TPrimitiveBuilder<FVFVERTEXTYPE,FVFVERTEXTYPEFLAGS>::Color3f( float r, float g, float b )
{
	last_r = (unsigned char)(r*255.0);
	last_g = (unsigned char)(g*255.0);
	last_b = (unsigned char)(b*255.0);
	last_a = 255;
}


// --------------------------------------------------------------------------
//
//
//
template <class FVFVERTEXTYPE, U32 FVFVERTEXTYPEFLAGS>
inline void TPrimitiveBuilder<FVFVERTEXTYPE,FVFVERTEXTYPEFLAGS>::Color4ub( unsigned char r, unsigned char g, unsigned char b, unsigned char a )
{
	last_r = r;
	last_g = g;
	last_b = b;
	last_a = a;
}


// --------------------------------------------------------------------------
//
//
//
template <class FVFVERTEXTYPE, U32 FVFVERTEXTYPEFLAGS>
inline void TPrimitiveBuilder<FVFVERTEXTYPE,FVFVERTEXTYPEFLAGS>::Color3ub( unsigned char r, unsigned char g, unsigned char b )
{
	last_r = r;
	last_g = g;
	last_b = b;
	last_a = 255;
}


// --------------------------------------------------------------------------
//
//
//
template <class FVFVERTEXTYPE, U32 FVFVERTEXTYPEFLAGS>
inline void TPrimitiveBuilder<FVFVERTEXTYPE,FVFVERTEXTYPEFLAGS>::TexCoord2f( float s, float t )
{
	last_s = s;
	last_t = t;
}


// --------------------------------------------------------------------------
//
//
//
template <class FVFVERTEXTYPE, U32 FVFVERTEXTYPEFLAGS>
inline void TPrimitiveBuilder<FVFVERTEXTYPE,FVFVERTEXTYPEFLAGS>::Vertex3f( float _x, float _y, float _z )
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
	vertex_buffer[current_vertex].pos.x = _x;
	vertex_buffer[current_vertex].pos.y = _y;
	vertex_buffer[current_vertex].pos.z = _z;
	current_vertex++;
	VerifyBuffer();
}

template <class FVFVERTEXTYPE, U32 FVFVERTEXTYPEFLAGS>
inline void TPrimitiveBuilder<FVFVERTEXTYPE,FVFVERTEXTYPEFLAGS>::Vertex3fv( const float * _v )
{
	Vertex3f(_v[0], _v[1], _v[2]);
}

//

template <class FVFVERTEXTYPE, U32 FVFVERTEXTYPEFLAGS>
inline void TPrimitiveBuilder<FVFVERTEXTYPE,FVFVERTEXTYPEFLAGS>::Vertex2f( float _x, float _y )
{
	Vertex3f(_x, _y, 0.0f);
}

//

template <class FVFVERTEXTYPE, U32 FVFVERTEXTYPEFLAGS>
inline void TPrimitiveBuilder<FVFVERTEXTYPE,FVFVERTEXTYPEFLAGS>::Vertex2fv( const float * _v )
{
	Vertex3f(_v[0], _v[1], 0.0f);
}

// --------------------------------------------------------------------------
//
//
//
template <class FVFVERTEXTYPE, U32 FVFVERTEXTYPEFLAGS>
inline void TPrimitiveBuilder<FVFVERTEXTYPE,FVFVERTEXTYPEFLAGS>::End( void )
{
	if( current_vertex == 0 ) {
		return;
	}

	if( insert_at_end ) {
		PB_COPY_VERTEX(current_vertex,0);
		current_vertex++;
	}

	if( render_primitive ) {
		render_primitive->draw_primitive( type, fvf_vertex_flags, vertex_buffer, current_vertex, 0 );
	}
	else if( render_pipeline ) {
		render_pipeline->draw_primitive( type, fvf_vertex_flags, vertex_buffer, current_vertex, 0 );
	}
}

// --------------------------------------------------------------------------
//
//
//
template <class FVFVERTEXTYPE, U32 FVFVERTEXTYPEFLAGS>
inline void TPrimitiveBuilder<FVFVERTEXTYPE,FVFVERTEXTYPEFLAGS>::Draw( D3DPRIMITIVETYPE _type )
{
	if( render_primitive ) {
		render_primitive->draw_primitive( type, fvf_vertex_flags, vertex_buffer, current_vertex, 0 );
	}
	else if( render_pipeline ) {
		render_pipeline->draw_primitive( _type, fvf_vertex_flags, vertex_buffer, current_vertex, 0 );
	}
}
