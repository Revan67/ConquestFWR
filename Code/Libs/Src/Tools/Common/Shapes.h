// Shapes.h
//
//
//

#ifndef __Shapes_h__
#define __Shapes_h__

//

#include <stdlib.h>
#include <stdio.h>

//

#include "da_d3dtypes.h"
#include "VertexBufferDesc.h"

//

struct Shape
{
	U16  *i_buffer;
	U32  num_indices;

	VertexBufferDesc vbd;

	D3DPRIMITIVETYPE primitive_type;

	//

	Shape()
	{
		i_buffer = NULL;
	}

	//

	~Shape()
	{
		delete[] i_buffer;
		i_buffer = NULL;
	}

	//
};

//

typedef enum SHAPE_TYPE
{
	ST_SOLID,
	ST_WIREFRAME
};

//

void CreateBox( SHAPE_TYPE type, U32 color, float width, float height, float depth, Shape *out_shape );
void CreateCylinder( SHAPE_TYPE type, U32 color, float radius, float length, int radius_segments, int length_segments, Shape *out_shape );
void CreateTube( SHAPE_TYPE type, U32 color, float radius, float length, int radius_segments, int length_segments, Shape *out_shape );
void CreateSphere( SHAPE_TYPE type, U32 color, float radius, int radius_segments, Shape *out_shape );
void CreateFrustum( SHAPE_TYPE type, U32 color, float left, float right, float bottom, float top, float nearval, float farval, Shape *out_shape );

//

#endif // EOF
