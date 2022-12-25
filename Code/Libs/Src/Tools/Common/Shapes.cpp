// Shapes.cpp
//
//
//

//

#include <memory.h>

//

#include "FDUMP.h"
#include "3dmath.h"

//

#include "Shapes.h"

//

#ifndef M_PI
#define M_PI 3.1415f
#endif

//
// CreateBox
//
//
void CreateBox( SHAPE_TYPE type, U32 color, float width, float height, float depth, Shape *out_shape )
{
	// Points
	static float Ps[8][3] = {
		{ -1.0f,  1.0f, -1.0f }, //0
		{  1.0f,  1.0f, -1.0f }, //1
		{  1.0f, -1.0f, -1.0f }, //2
		{ -1.0f, -1.0f, -1.0f }, //3

		{ -1.0f,  1.0f,  1.0f }, //4
		{ -1.0f, -1.0f,  1.0f }, //5
		{  1.0f, -1.0f,  1.0f }, //6
		{  1.0f,  1.0f,  1.0f }	 //7
	};

	static U32 Pis[24] = { 0, 1, 2, 3, 4, 5, 6, 7, 4, 7, 1, 0, 5, 3, 2, 6, 1, 7, 6, 2, 0, 3, 5, 4 };

	// Normals
	static float Ns[6][3] = {
		{  0.0f,  0.0f, -1.0f },
		{  0.0f,  0.0f,  1.0f },
		{  0.0f, -1.0f,  0.0f },
		{  0.0f,  1.0f,  0.0f },
		{ -1.0f,  0.0f,  0.0f },
		{  1.0f,  0.0f,  0.0f }
	};

	static U32 Nis[24] = { 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5 };

	// Colors
	static U32 C0s[1] = {
		0xFFFFFFFF
	};

	static U32 C0is[24] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

	// MC0s
	static float MC0s[8][2] = {
		
		{ 0.01f, 0.99f }, //0
		{ 0.99f, 0.99f }, //1
		{ 0.99f, 0.01f }, //2
		{ 0.01f, 0.01f }  //3
	};

	static U32 MC0is[24] = { 0, 1, 2, 3, 1, 2, 3, 0, 0, 1, 2, 3, 0, 3, 2, 1, 0, 1, 2, 3, 1, 2, 3, 0 };

	// Face indices
	//
	static U16 TriFaces[36] = {
		 0+0,    0+1,    0+2,
		 0+2,    0+3,    0+0,
		 4+0,    4+1,    4+2,
		 4+2,    4+3,    4+0,
		 8+0,    8+1,    8+2,
		 8+2,    8+3,    8+0,
		12+0,   12+1,   12+2,
		12+2,   12+3,   12+0,
		16+0,   16+1,   16+2,
		16+2,   16+3,   16+0,
		20+0,   20+1,   20+2,
		20+2,   20+3,   20+0

	};

	static U16 LineFaces[24] = {
		// top
		3,2,
		2,6,
		6,7,
		7,3,

		// bottom
		0,4,
		4,5,
		5,1,
		1,0,

		// sides
		0,3,

		1,2,
		
		4,7,
		
		5,6
	};

	//

//	C0s[0] = color;

	for( U32 p=0; p<8; p++ ) {
		Ps[p][0] /= fabs(Ps[p][0]);
		Ps[p][0] *= width/2;

		Ps[p][1] /= fabs(Ps[p][1]);
		Ps[p][1] *= height/2;
		
		Ps[p][2] /= fabs(Ps[p][2]);
		Ps[p][2] *= depth/2;
	}

	//

	memset( &out_shape->vbd, 0, sizeof(out_shape->vbd) );

	out_shape->vbd.flags = VBD_F_INDEXED;
	out_shape->vbd.num_vertices = 24;
	out_shape->vbd.vertex_format = D3DFVF_XYZ | D3DFVF_TEX1 | D3DFVF_NORMAL ;

	out_shape->vbd.Ps.data = Ps;
	out_shape->vbd.Ps.indices = Pis;
	out_shape->vbd.Ps.size = sizeof(Vector);
	out_shape->vbd.Ps.stride = sizeof(Vector);

	out_shape->vbd.Ns.data = Ns;
	out_shape->vbd.Ns.indices = Nis;
	out_shape->vbd.Ns.size = sizeof(Vector);
	out_shape->vbd.Ns.stride = sizeof(Vector);

//	out_shape->vbd.C0s.data = C0s;
//	out_shape->vbd.C0s.indices = C0is;
//	out_shape->vbd.C0s.size = sizeof(U32);
//	out_shape->vbd.C0s.stride = sizeof(U32);

	out_shape->vbd.MC0s.data = MC0s;
	out_shape->vbd.MC0s.indices = MC0is;
	out_shape->vbd.MC0s.size = sizeof(float)*2;
	out_shape->vbd.MC0s.stride = sizeof(float)*2;

	switch( type ) {

	case ST_WIREFRAME:
		out_shape->primitive_type = D3DPT_LINELIST;
		out_shape->num_indices = 24;
		out_shape->i_buffer = LineFaces;
		break;

	case ST_SOLID:
		out_shape->primitive_type = D3DPT_TRIANGLELIST;
		out_shape->num_indices = 36;
		out_shape->i_buffer = TriFaces;
		break;
	}
}

//
// CreateCylinder
//
// UNDEBUGGED
//
void CreateCylinder( SHAPE_TYPE type, U32 color, float radius, float length, int radius_segments, int length_segments, Shape *out_shape )
{
	if( out_shape->i_buffer ) {
		return;
	}

	double da, dz;
	float x, y, z; 
	int i, j;

	//

	memset( &out_shape->vbd, 0, sizeof(out_shape->vbd) );

	out_shape->vbd.flags = VBD_F_INDEXED;
	out_shape->vbd.vertex_format = D3DFVF_XYZ | D3DFVF_DIFFUSE ;
	out_shape->vbd.num_vertices = radius_segments * 2 * (1 + length_segments);

	Vector *v = (Vector*)malloc( sizeof(Vector) * (out_shape->vbd.num_vertices) );
	ASSERT(v);

	out_shape->vbd.Ps.data = v;
	out_shape->vbd.Ps.indices = NULL;
	out_shape->vbd.Ps.size = sizeof(Vector);
	out_shape->vbd.Ps.stride = sizeof(Vector);

	U32 *c = (U32*)malloc( sizeof(U32) * 1 );
	ASSERT(c);

	c[0] = 0;

	out_shape->vbd.C0s.data = c;
	out_shape->vbd.C0s.indices = NULL;
	out_shape->vbd.C0s.size = sizeof(U32);
	out_shape->vbd.C0s.stride = sizeof(U32);

	da = 2.0*M_PI / radius_segments;
	dz = length / length_segments;

	Vector *Pn, *Pn_1;

	switch( type ) {

	case ST_WIREFRAME:
	
		out_shape->primitive_type = D3DPT_LINELIST;
		
		// rings
		//
		z = -length/2.0;

		Pn = v;
		Pn_1 = v;

		for( j=0; j <= length_segments; j++ ) {
			for( i=0; i < radius_segments; i++ ) {

				x = cos( i*da );
				y = sin( i*da );

				Pn[0] = Vector( x*radius, y*radius, z );		
				Pn[1] = *Pn_1;						

				Pn_1 = Pn;
				Pn += 2;
			}

			z += dz;
		}

		// draw length lines 
		//
		for( i=0; i<radius_segments; i++ ) { 
			
			x = cos( i*da );
			y = sin( i*da );

			Pn[0] = Vector( x*radius, y*radius, -length/2.0 );
			Pn[1] = Vector( x*radius, y*radius,  length/2.0 );

			Pn += 2;
		}

		break;

	case ST_SOLID:

		out_shape->primitive_type = D3DPT_TRIANGLELIST;

		break;
	}
}

//

void CreateTube( SHAPE_TYPE type, U32 color, float radius, float length, Shape *out_shape )
{

}

//

void CreateSphere( SHAPE_TYPE type, U32 color, float radius, int radius_segments, Shape *out_shape )
{
}

//
