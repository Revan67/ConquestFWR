#include "pch.h"
#include "myvertex.h"

void PrimitiveBuilder2::Vertex3f( float _x, float _y, float _z )
{
	if( watch_quad ) {
		if( (quad_vert_count%4) == 3 ) {
			current_vertex += 2;
			VerifyBuffer();
			PB_COPY_VERTEX(current_vertex-2,current_vertex-5);	// vert 0 of quad
			PB_COPY_VERTEX(current_vertex-1,current_vertex-3);	// vert 2 of quad
		}
		quad_vert_count++;
	}

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

void PrimitiveBuilder2::MulCoord2f( float _u, float _v)
{
	last_s2 = _u;
	last_t2 = _v;
}
