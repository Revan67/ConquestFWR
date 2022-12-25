// VertexBufferDesc.h
//
//
//


#ifndef __VertexBufferDesc_h__
#define __VertexBufferDesc_h__

//

#include "typedefs.h"

//

typedef U32 VertexBufferFlag;

const VertexBufferFlag	VBD_F_BIT_ZERO	= (1<<0);	// This bit is reserved.
const VertexBufferFlag	VBD_F_AOS		= (1<<1);	// VBD describes an array-of-structures, if
													// this flag is not set, the data is assumed to be
													// interleaved.
const VertexBufferFlag	VBD_F_INDEXED	= (1<<2);	// Use the VertexBufferItemDesc indices values.

//

struct VertexBufferItemDesc
{
	void *data;						// Pointer to database of values
	unsigned long size;				// Size in bytes of each individual item
	unsigned long stride;			// Stride in bytes from start of one item to the start of the next item
	unsigned long *indices;			// Indices for the given item.
	unsigned long count;			// Number of elements in 'data'
};

//

struct VertexBufferDesc
{
	VertexBufferFlag	flags;		// VBD_F_* flags

	VertexBufferItemDesc Ps;		// Vertex Points		(Vector)
	VertexBufferItemDesc Ns;		// Vertex Normals		(Vector)
	VertexBufferItemDesc C0s;		// Vertex Color 0		(PACKEDARGB)
	VertexBufferItemDesc C1s;		// Vertex Color 1		(PACKEDARGB)
	VertexBufferItemDesc MC0s;		// Vertex Map Coord 0	(n-floats, where n is 1,2,3,4)
	VertexBufferItemDesc MC1s;		// Vertex Map Coord 1	(n-floats, where n is 1,2,3,4)

	unsigned long vertex_format;	// Flexible Vertex Format Flags
									// This determines which of the above vertex items are
									// actually accessed.  Enabling a vertex item (via some
									// FVF flag) when the item is not available (the above
									// pointers are NULL) will caused undefined behavior.
	
	unsigned long num_vertices;		// Total number of vertices.  This is also the number
									// of entries in the 'indices' arrays for each item.
									// i.e. VertexCount == length(Ps.indices)
									//
									// When an 'indices' array is NULL, VertexCount specifies
									// the length of the 'data' arrays for the item.
									// i.e. VertexCount == length(Ps.data)
};


#endif // EOF
