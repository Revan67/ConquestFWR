// render_buffers.h
//
//
//
//

#ifndef __render_buffers_h__
#define __render_buffers_h__

//***************************************************************************************
//  render buffers
//***************************************************************************************

Vector *pe_vector_buffer		= NULL;
static U32 pe_vector_buffer_size	= 0;
static U32 pe_vector_buffer_start	= 312;
static int pe_vector_buffer_ref		= 0;

inline void verify_vector_buffer( U32 size, U32 copy = 0 )
{
	if( pe_vector_buffer_start > size ) {
		size = pe_vector_buffer_start;
	}

	if( pe_vector_buffer_size < size ) {
		Vector *newbuf = new Vector[size];
		ASSERT_FATAL( newbuf );
		if( pe_vector_buffer && copy ) {
			memcpy( newbuf, pe_vector_buffer, sizeof(Vector)*pe_vector_buffer_size );
			delete[] pe_vector_buffer;
		}
		pe_vector_buffer = newbuf;
		pe_vector_buffer_size = size;
	}
}

inline void add_ref_vector_buffer( )
{
	pe_vector_buffer_ref++;
}

inline void del_ref_vector_buffer( )
{
	pe_vector_buffer_ref--;
	if( pe_vector_buffer_ref <= 0 ) {
		delete[] pe_vector_buffer;
		pe_vector_buffer = NULL;
		pe_vector_buffer_size = 0;
		pe_vector_buffer_ref = 0;
	}
}


RPVertex *pe_vertex_buffer	= NULL;
static U32 pe_vertex_buffer_size	= 0;
static U32 pe_vertex_buffer_start	= 512;
static int pe_vertex_buffer_ref		= 0;

inline void verify_vertex_buffer( U32 size, U32 copy = 0 )
{
	if( pe_vertex_buffer_start > size ) {
		size = pe_vertex_buffer_start;
	}

	if( pe_vertex_buffer_size < size ) {
		RPVertex *newbuf = new RPVertex[size];
		ASSERT_FATAL( newbuf );
		if( pe_vertex_buffer)
		{
			if (copy )
				memcpy( newbuf, pe_vertex_buffer, sizeof(Vector)*pe_vertex_buffer_size );
			delete[] pe_vertex_buffer;
		}
		pe_vertex_buffer = newbuf;
		pe_vertex_buffer_size = size;
	}
}

inline void add_ref_vertex_buffer( )
{
	pe_vertex_buffer_ref++;
}

inline void del_ref_vertex_buffer( )
{
	pe_vertex_buffer_ref--;
	if( pe_vertex_buffer_ref <= 0 ) {
		delete[] pe_vertex_buffer;
		pe_vertex_buffer = NULL;
		pe_vertex_buffer_size = 0;
		pe_vertex_buffer_ref = 0;
	}
}




#endif // EOF
