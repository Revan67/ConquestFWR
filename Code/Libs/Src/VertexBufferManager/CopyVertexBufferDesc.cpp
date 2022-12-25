// CopyVertexBufferDesc.cpp
//
//
//

//

#include "stdlib.h"
#include "memory.h"

//

#include "DACOM.h"
#include "FDUMP.h"
#include "FVF.h"
#include "rendpipeline.h"

//

#include "VertexBufferDesc.h"

#include <d3dx9.h>

//

void copy_vertex_buffer_desc( void *dst_buffer, U32 dst_vertex_format, VertexBufferDesc *vb_desc );

//

typedef void (*cvbd_copier)( void *dst_buffer, U32 dst_vertex_format, VertexBufferDesc *src_vb_desc );

//
// vertex copier declarations
//
static void cvbd_assert( void *, U32 , VertexBufferDesc * );
static void cvbd_i_n_p( void *, U32 , VertexBufferDesc * );
static void cvbd_i_uv0_n_p( void *, U32 , VertexBufferDesc * );
static void cvbd_i_uv0_c0_n_p( void *dst_buffer, U32 dst_vertex_format, VertexBufferDesc *src_buffer );

//

struct cvbd_entry
{
	cvbd_copier	vertex_copier;
	U32			vertex_format;
};

//

const U32 RHW	= (1<<0);
const U32 N		= (1<<1);
const U32 C0	= (1<<2);
const U32 C1	= (1<<3);
const U32 UV0	= (1<<4);
const U32 UV1	= (1<<5);

//

const U32 _RHW	= 0;
const U32 _N	= 0;
const U32 _C0	= 0;
const U32 _C1	= 0;
const U32 _UV0	= 0;
const U32 _UV1	= 0;

//
// This is the lookup table for the indexed SOA copiers.
//
// NOTE: An underscore before an item signifies that that
// item does NOT exist in the vertex format.  An item WITHOUT
// an underscore signifies that the item DOES exist.
// 
// Note that some entries in the table are invalid and map
// to an assert.
//
cvbd_entry indexed_lookup[] = 
{
	/*	0	*/	{	cvbd_assert,		_UV1 | _UV0 | _C1 | _C0 | _N | _RHW },
	/*	1	*/	{	cvbd_assert,		_UV1 | _UV0 | _C1 | _C0 | _N |  RHW },
	/*	2	*/	{	cvbd_i_n_p,			_UV1 | _UV0 | _C1 | _C0 |  N | _RHW },
	/*	3	*/	{	cvbd_assert,		_UV1 | _UV0 | _C1 | _C0 |  N |  RHW },
	/*	4	*/	{	cvbd_assert,		_UV1 | _UV0 | _C1 |  C0 | _N | _RHW },
	/*	5	*/	{	cvbd_assert,		_UV1 | _UV0 | _C1 |  C0 | _N |  RHW },
	/*	6	*/	{	cvbd_assert,		_UV1 | _UV0 | _C1 |  C0 |  N | _RHW },
	/*	7	*/	{	cvbd_assert,		_UV1 | _UV0 | _C1 |  C0 |  N |  RHW },
										
	/*	8	*/	{	cvbd_assert,		_UV1 | _UV0 |  C1 | _C0 | _N | _RHW },
	/*	9	*/	{	cvbd_assert,		_UV1 | _UV0 |  C1 | _C0 | _N |  RHW },
	/*	10	*/	{	cvbd_assert,		_UV1 | _UV0 |  C1 | _C0 |  N | _RHW },
	/*	11	*/	{	cvbd_assert,		_UV1 | _UV0 |  C1 | _C0 |  N |  RHW },
	/*	12	*/	{	cvbd_assert,		_UV1 | _UV0 |  C1 |  C0 | _N | _RHW },
	/*	13	*/	{	cvbd_assert,		_UV1 | _UV0 |  C1 |  C0 | _N |  RHW },
	/*	14	*/	{	cvbd_assert,		_UV1 | _UV0 |  C1 |  C0 |  N | _RHW },
	/*	15	*/	{	cvbd_assert,		_UV1 | _UV0 |  C1 |  C0 |  N |  RHW },
										
	/*	16	*/	{	cvbd_assert,		_UV1 |  UV0 | _C1 | _C0 | _N | _RHW },
	/*	17	*/	{	cvbd_assert,		_UV1 |  UV0 | _C1 | _C0 | _N |  RHW },
	/*	18	*/	{	cvbd_i_uv0_n_p,		_UV1 |  UV0 | _C1 | _C0 |  N | _RHW },
	/*	19	*/	{	cvbd_assert,		_UV1 |  UV0 | _C1 | _C0 |  N |  RHW },
	/*	20	*/	{	cvbd_assert,		_UV1 |  UV0 | _C1 |  C0 | _N | _RHW },
	/*	21	*/	{	cvbd_assert,		_UV1 |  UV0 | _C1 |  C0 | _N |  RHW },
	/*	22	*/	{	cvbd_i_uv0_c0_n_p,	_UV1 |  UV0 | _C1 |  C0 |  N | _RHW },
	/*	23	*/	{	cvbd_assert,		_UV1 |  UV0 | _C1 |  C0 |  N |  RHW },
										
	/*	24	*/	{	cvbd_assert,		_UV1 |  UV0 |  C1 | _C0 | _N | _RHW },
	/*	25	*/	{	cvbd_assert,		_UV1 |  UV0 |  C1 | _C0 | _N |  RHW },
	/*	26	*/	{	cvbd_assert,		_UV1 |  UV0 |  C1 | _C0 |  N | _RHW },
	/*	27	*/	{	cvbd_assert,		_UV1 |  UV0 |  C1 | _C0 |  N |  RHW },
	/*	28	*/	{	cvbd_assert,		_UV1 |  UV0 |  C1 |  C0 | _N | _RHW },
	/*	29	*/	{	cvbd_assert,		_UV1 |  UV0 |  C1 |  C0 | _N |  RHW },
	/*	30	*/	{	cvbd_assert,		_UV1 |  UV0 |  C1 |  C0 |  N | _RHW },
	/*	31	*/	{	cvbd_assert,		_UV1 |  UV0 |  C1 |  C0 |  N |  RHW },
										
	/*	32	*/	{	cvbd_assert,		 UV1 | _UV0 | _C1 | _C0 | _N | _RHW },
	/*	33	*/	{	cvbd_assert,		 UV1 | _UV0 | _C1 | _C0 | _N |  RHW },
	/*	34	*/	{	cvbd_assert,		 UV1 | _UV0 | _C1 | _C0 |  N | _RHW },
	/*	35	*/	{	cvbd_assert,		 UV1 | _UV0 | _C1 | _C0 |  N |  RHW },
	/*	36	*/	{	cvbd_assert,		 UV1 | _UV0 | _C1 |  C0 | _N | _RHW },
	/*	37	*/	{	cvbd_assert,		 UV1 | _UV0 | _C1 |  C0 | _N |  RHW },
	/*	38	*/	{	cvbd_assert,		 UV1 | _UV0 | _C1 |  C0 |  N | _RHW },
	/*	39	*/	{	cvbd_assert,		 UV1 | _UV0 | _C1 |  C0 |  N |  RHW },
										
	/*	40	*/	{	cvbd_assert,		 UV1 | _UV0 |  C1 | _C0 | _N | _RHW },
	/*	41	*/	{	cvbd_assert,		 UV1 | _UV0 |  C1 | _C0 | _N |  RHW },
	/*	42	*/	{	cvbd_assert,		 UV1 | _UV0 |  C1 | _C0 |  N | _RHW },
	/*	43	*/	{	cvbd_assert,		 UV1 | _UV0 |  C1 | _C0 |  N |  RHW },
	/*	44	*/	{	cvbd_assert,		 UV1 | _UV0 |  C1 |  C0 | _N | _RHW },
	/*	45	*/	{	cvbd_assert,		 UV1 | _UV0 |  C1 |  C0 | _N |  RHW },
	/*	46	*/	{	cvbd_assert,		 UV1 | _UV0 |  C1 |  C0 |  N | _RHW },
	/*	47	*/	{	cvbd_assert,		 UV1 | _UV0 |  C1 |  C0 |  N |  RHW },
										
	/*	48	*/	{	cvbd_assert,		 UV1 |  UV0 | _C1 | _C0 | _N | _RHW },
	/*	49	*/	{	cvbd_assert,		 UV1 |  UV0 | _C1 | _C0 | _N |  RHW },
	/*	50	*/	{	cvbd_assert,		 UV1 |  UV0 | _C1 | _C0 |  N | _RHW },
	/*	51	*/	{	cvbd_assert,		 UV1 |  UV0 | _C1 | _C0 |  N |  RHW },
	/*	52	*/	{	cvbd_assert,		 UV1 |  UV0 | _C1 |  C0 | _N | _RHW },
	/*	53	*/	{	cvbd_assert,		 UV1 |  UV0 | _C1 |  C0 | _N |  RHW },
	/*	54	*/	{	cvbd_assert,		 UV1 |  UV0 | _C1 |  C0 |  N | _RHW },
	/*	55	*/	{	cvbd_assert,		 UV1 |  UV0 | _C1 |  C0 |  N |  RHW },
										
	/*	56	*/	{	cvbd_assert,		 UV1 |  UV0 |  C1 | _C0 | _N | _RHW },
	/*	57	*/	{	cvbd_assert,		 UV1 |  UV0 |  C1 | _C0 | _N |  RHW },
	/*	58	*/	{	cvbd_assert,		 UV1 |  UV0 |  C1 | _C0 |  N | _RHW },
	/*	59	*/	{	cvbd_assert,		 UV1 |  UV0 |  C1 | _C0 |  N |  RHW },
	/*	60	*/	{	cvbd_assert,		 UV1 |  UV0 |  C1 |  C0 | _N | _RHW },
	/*	61	*/	{	cvbd_assert,		 UV1 |  UV0 |  C1 |  C0 | _N |  RHW },
	/*	62	*/	{	cvbd_assert,		 UV1 |  UV0 |  C1 |  C0 |  N | _RHW },
	/*	63	*/	{	cvbd_assert,		 UV1 |  UV0 |  C1 |  C0 |  N |  RHW }
};

//
// This is the lookup table for the non-indexed SOA copiers.
//
// NOTE: An underscore before an item signifies that that
// item does NOT exist in the vertex format.  An item WITHOUT
// an underscore signifies that the item DOES exist.
// 
// Note that some entries in the table are invalid and map
// to an assert.
//
cvbd_entry nonindexed_lookup[] = 
{
	/*	0	*/	{	cvbd_assert,	_UV1 | _UV0 | _C1 | _C0 | _N | _RHW },
	/*	1	*/	{	cvbd_assert,	_UV1 | _UV0 | _C1 | _C0 | _N |  RHW },
	/*	2	*/	{	cvbd_assert,	_UV1 | _UV0 | _C1 | _C0 |  N | _RHW },
	/*	3	*/	{	cvbd_assert,	_UV1 | _UV0 | _C1 | _C0 |  N |  RHW },
	/*	4	*/	{	cvbd_assert,	_UV1 | _UV0 | _C1 |  C0 | _N | _RHW },
	/*	5	*/	{	cvbd_assert,	_UV1 | _UV0 | _C1 |  C0 | _N |  RHW },
	/*	6	*/	{	cvbd_assert,	_UV1 | _UV0 | _C1 |  C0 |  N | _RHW },
	/*	7	*/	{	cvbd_assert,	_UV1 | _UV0 | _C1 |  C0 |  N |  RHW },

	/*	8	*/	{	cvbd_assert,	_UV1 | _UV0 |  C1 | _C0 | _N | _RHW },
	/*	9	*/	{	cvbd_assert,	_UV1 | _UV0 |  C1 | _C0 | _N |  RHW },
	/*	10	*/	{	cvbd_assert,	_UV1 | _UV0 |  C1 | _C0 |  N | _RHW },
	/*	11	*/	{	cvbd_assert,	_UV1 | _UV0 |  C1 | _C0 |  N |  RHW },
	/*	12	*/	{	cvbd_assert,	_UV1 | _UV0 |  C1 |  C0 | _N | _RHW },
	/*	13	*/	{	cvbd_assert,	_UV1 | _UV0 |  C1 |  C0 | _N |  RHW },
	/*	14	*/	{	cvbd_assert,	_UV1 | _UV0 |  C1 |  C0 |  N | _RHW },
	/*	15	*/	{	cvbd_assert,	_UV1 | _UV0 |  C1 |  C0 |  N |  RHW },

	/*	16	*/	{	cvbd_assert,	_UV1 |  UV0 | _C1 | _C0 | _N | _RHW },
	/*	17	*/	{	cvbd_assert,	_UV1 |  UV0 | _C1 | _C0 | _N |  RHW },
	/*	18	*/	{	cvbd_assert,	_UV1 |  UV0 | _C1 | _C0 |  N | _RHW },
	/*	19	*/	{	cvbd_assert,	_UV1 |  UV0 | _C1 | _C0 |  N |  RHW },
	/*	20	*/	{	cvbd_assert,	_UV1 |  UV0 | _C1 |  C0 | _N | _RHW },
	/*	21	*/	{	cvbd_assert,	_UV1 |  UV0 | _C1 |  C0 | _N |  RHW },
	/*	22	*/	{	cvbd_assert,	_UV1 |  UV0 | _C1 |  C0 |  N | _RHW },
	/*	23	*/	{	cvbd_assert,	_UV1 |  UV0 | _C1 |  C0 |  N |  RHW },

	/*	24	*/	{	cvbd_assert,	_UV1 |  UV0 |  C1 | _C0 | _N | _RHW },
	/*	25	*/	{	cvbd_assert,	_UV1 |  UV0 |  C1 | _C0 | _N |  RHW },
	/*	26	*/	{	cvbd_assert,	_UV1 |  UV0 |  C1 | _C0 |  N | _RHW },
	/*	27	*/	{	cvbd_assert,	_UV1 |  UV0 |  C1 | _C0 |  N |  RHW },
	/*	28	*/	{	cvbd_assert,	_UV1 |  UV0 |  C1 |  C0 | _N | _RHW },
	/*	29	*/	{	cvbd_assert,	_UV1 |  UV0 |  C1 |  C0 | _N |  RHW },
	/*	30	*/	{	cvbd_assert,	_UV1 |  UV0 |  C1 |  C0 |  N | _RHW },
	/*	31	*/	{	cvbd_assert,	_UV1 |  UV0 |  C1 |  C0 |  N |  RHW },

	/*	32	*/	{	cvbd_assert,	 UV1 | _UV0 | _C1 | _C0 | _N | _RHW },
	/*	33	*/	{	cvbd_assert,	 UV1 | _UV0 | _C1 | _C0 | _N |  RHW },
	/*	34	*/	{	cvbd_assert,	 UV1 | _UV0 | _C1 | _C0 |  N | _RHW },
	/*	35	*/	{	cvbd_assert,	 UV1 | _UV0 | _C1 | _C0 |  N |  RHW },
	/*	36	*/	{	cvbd_assert,	 UV1 | _UV0 | _C1 |  C0 | _N | _RHW },
	/*	37	*/	{	cvbd_assert,	 UV1 | _UV0 | _C1 |  C0 | _N |  RHW },
	/*	38	*/	{	cvbd_assert,	 UV1 | _UV0 | _C1 |  C0 |  N | _RHW },
	/*	39	*/	{	cvbd_assert,	 UV1 | _UV0 | _C1 |  C0 |  N |  RHW },

	/*	40	*/	{	cvbd_assert,	 UV1 | _UV0 |  C1 | _C0 | _N | _RHW },
	/*	41	*/	{	cvbd_assert,	 UV1 | _UV0 |  C1 | _C0 | _N |  RHW },
	/*	42	*/	{	cvbd_assert,	 UV1 | _UV0 |  C1 | _C0 |  N | _RHW },
	/*	43	*/	{	cvbd_assert,	 UV1 | _UV0 |  C1 | _C0 |  N |  RHW },
	/*	44	*/	{	cvbd_assert,	 UV1 | _UV0 |  C1 |  C0 | _N | _RHW },
	/*	45	*/	{	cvbd_assert,	 UV1 | _UV0 |  C1 |  C0 | _N |  RHW },
	/*	46	*/	{	cvbd_assert,	 UV1 | _UV0 |  C1 |  C0 |  N | _RHW },
	/*	47	*/	{	cvbd_assert,	 UV1 | _UV0 |  C1 |  C0 |  N |  RHW },

	/*	48	*/	{	cvbd_assert,	 UV1 |  UV0 | _C1 | _C0 | _N | _RHW },
	/*	49	*/	{	cvbd_assert,	 UV1 |  UV0 | _C1 | _C0 | _N |  RHW },
	/*	50	*/	{	cvbd_assert,	 UV1 |  UV0 | _C1 | _C0 |  N | _RHW },
	/*	51	*/	{	cvbd_assert,	 UV1 |  UV0 | _C1 | _C0 |  N |  RHW },
	/*	52	*/	{	cvbd_assert,	 UV1 |  UV0 | _C1 |  C0 | _N | _RHW },
	/*	53	*/	{	cvbd_assert,	 UV1 |  UV0 | _C1 |  C0 | _N |  RHW },
	/*	54	*/	{	cvbd_assert,	 UV1 |  UV0 | _C1 |  C0 |  N | _RHW },
	/*	55	*/	{	cvbd_assert,	 UV1 |  UV0 | _C1 |  C0 |  N |  RHW },

	/*	56	*/	{	cvbd_assert,	 UV1 |  UV0 |  C1 | _C0 | _N | _RHW },
	/*	57	*/	{	cvbd_assert,	 UV1 |  UV0 |  C1 | _C0 | _N |  RHW },
	/*	58	*/	{	cvbd_assert,	 UV1 |  UV0 |  C1 | _C0 |  N | _RHW },
	/*	59	*/	{	cvbd_assert,	 UV1 |  UV0 |  C1 | _C0 |  N |  RHW },
	/*	60	*/	{	cvbd_assert,	 UV1 |  UV0 |  C1 |  C0 | _N | _RHW },
	/*	61	*/	{	cvbd_assert,	 UV1 |  UV0 |  C1 |  C0 | _N |  RHW },
	/*	62	*/	{	cvbd_assert,	 UV1 |  UV0 |  C1 |  C0 |  N | _RHW },
	/*	63	*/	{	cvbd_assert,	 UV1 |  UV0 |  C1 |  C0 |  N |  RHW }
};

//

//
// This creates a 6-bit index that serves as a lookup 
// into the copier table.  The resulting 6-bits are
// from the _XYZRHW, _NORMAL, _DIFFUSE, _SPECULAR, and
// the first two bits of the texture coordinate specifier.
//
// The resulting bits are those specified above.
//
inline U32 cvbd_fvf_to_index( U32 vertex_format ) 
{
	U32 index = 0;
	
	index  |= ((vertex_format >> 8) & 0x03) << 4;	// get uv0 and uv1 bits
	index  |= ((vertex_format >> 6) & 0x03) << 2;	// get c0 and c1 bits
	index  |= ((vertex_format >> 4) & 0x01) << 1;	// get Normal bit
	index  |= ((vertex_format >> 2) & 0x01) << 0;	// get RHW bit

	return index;
}

//

void copy_vertex_buffer_desc( void *dst_buffer, U32 dst_vertex_format, VertexBufferDesc *src_vb_desc )
{
	// TODO: put in a bunch of assertion code to make sure formats are compatible.
	// assert that no beta weights are used, only two sets of texture coordinates
	// assert that AOS is not used with INDEXED

	U32 copier_index;

	// create a copier index that will tell us which copy path to take
	// this is based on the vertex format similarity, whether the source is indexed,
	// and whether the source is AOS or SOA
	//
	// NOTE: right now, we actually ignore whether the formats are equal in the SOA
	// cases because the only difference is whether to initialize additional fields in 
	// the destination buffer that do not have data in the source buffer.
	//
	copier_index  = (src_vb_desc->vertex_format == dst_vertex_format) ? VBD_F_BIT_ZERO : 0;
	copier_index |= (src_vb_desc->flags & 0x6);

	switch( copier_index ) {
	
	case 0:	// non-indexed, soa, diff format
	case 1:	// non-indexed, soa, same format
		copier_index = cvbd_fvf_to_index( dst_vertex_format );
		ASSERT( nonindexed_lookup[copier_index].vertex_format == dst_vertex_format );
		nonindexed_lookup[copier_index].vertex_copier( dst_buffer, dst_vertex_format, src_vb_desc );
		break;

	case 2:	// non-indexed, aos, diff format
		// TODO: copy aos with different structure sizes.
		ASSERT(0);
		break;
	
	case 3:	// non-indexed, aos, same format
		memcpy( dst_buffer, src_vb_desc->Ps.data, FVF_SIZEOF_VERT(dst_vertex_format) * (src_vb_desc->num_vertices) );
		break;

	case 4:	//     indexed, soa, diff format
	case 5:	//     indexed, soa, same format
		copier_index = cvbd_fvf_to_index( dst_vertex_format );
		ASSERT( indexed_lookup[copier_index].vertex_format == copier_index );
		indexed_lookup[copier_index].vertex_copier( dst_buffer, dst_vertex_format, src_vb_desc );
		break;

	default: 	
		ASSERT( 0 );
	}
}

//

void cvbd_assert( void *, U32 , VertexBufferDesc * )
{
	ASSERT( 0 );
}

//

void cvbd_i_n_p( void *dst_buffer, U32 dest_fmt, VertexBufferDesc *src_buffer )
{
	// ASSUMES:
	//
	// *  P and N size and strides are Vector
	// *  MC0 exist in source
	//

	const Vector * const src_P = (Vector*)src_buffer->Ps.data;
	const U32 *src_Pi = src_buffer->Ps.indices;

	const Vector * const src_N = (Vector*)src_buffer->Ns.data;
	const U32 *src_Ni = src_buffer->Ns.indices;

	struct PN { float x, y, z; float nx, ny, nz; };

	const PN * const dst_end = (PN*)dst_buffer + src_buffer->num_vertices;

	for( PN *dst = (PN*)dst_buffer; dst < dst_end; dst++ )
	{
		*((Vector*)&dst->x) = src_P[*src_Pi++];
		*((Vector*)&dst->nx) = src_N[*src_Ni++];
	}
}

//

void cvbd_i_uv0_n_p( void *dst_buffer, U32 dest_fmt, VertexBufferDesc *src_buffer )
{
	// ASSUMES:
	//
	// *  P and N size and strides are Vector
	// *  MC0 size and stride is 2 floats
	// *  N and MC0 exist in source
	//

	const Vector * const src_P = (Vector*)src_buffer->Ps.data;
	const U32 *src_Pi = src_buffer->Ps.indices;

	const Vector * const src_N = (Vector*)src_buffer->Ns.data;
	const U32 *src_Ni = src_buffer->Ns.indices;

	struct MCVector { float u, v; };
	const MCVector * const src_MC0 = (MCVector*)src_buffer->MC0s.data;
	const U32 *src_MC0i = src_buffer->MC0s.indices;

	const D3DVERTEX * const end2 = (D3DVERTEX*)dst_buffer + src_buffer->num_vertices;
	const D3DVERTEX * const end1 = end2 - 3;
	D3DVERTEX *dst;
	for(dst = (D3DVERTEX*)dst_buffer; dst < end1; dst+=4, src_Pi+=4, src_Ni+=4, src_MC0i+=4 )
	{
		*((Vector*)&dst[0].x) = src_P[src_Pi[0]];
		*((Vector*)&dst[0].nx) = src_N[src_Ni[0]];
		*((MCVector*)&dst[0].tu) = src_MC0[src_MC0i[0]];
		
		*((Vector*)&dst[1].x) = src_P[src_Pi[1]];
		*((Vector*)&dst[1].nx) = src_N[src_Ni[1]];
		*((MCVector*)&dst[1].tu) = src_MC0[src_MC0i[1]];

		*((Vector*)&dst[2].x) = src_P[src_Pi[2]];
		*((Vector*)&dst[2].nx) = src_N[src_Ni[2]];
		*((MCVector*)&dst[2].tu) = src_MC0[src_MC0i[2]];
		
		*((Vector*)&dst[3].x) = src_P[src_Pi[3]];
		*((Vector*)&dst[3].nx) = src_N[src_Ni[3]];
		*((MCVector*)&dst[3].tu) = src_MC0[src_MC0i[3]];
	}
	for( ; dst < end2; dst++ )
	{
		*((Vector*)&dst->x) = src_P[*src_Pi++];
		*((Vector*)&dst->nx) = src_N[*src_Ni++];
		*((MCVector*)&dst->tu) = src_MC0[*src_MC0i++];	
	}
}

//

void cvbd_i_uv0_c0_n_p( void *dst_buffer, U32, VertexBufferDesc *src_buffer )
{
	char *dst = (char*)dst_buffer;

	const U32  src_MC0_size = src_buffer->MC0s.size;

	const U32 src_P_stride = src_buffer->Ps.stride;
	const U32 src_N_stride = src_buffer->Ns.stride;
	const U32 src_C0_stride = src_buffer->C0s.stride;
	const U32 src_MC0_stride = src_buffer->MC0s.stride;

	const U32 *src_Pi = src_buffer->Ps.indices;
	const U32 *src_Ni = src_buffer->Ns.indices;
	const U32 *src_C0i = src_buffer->C0s.indices;
	const U32 *src_MC0i = src_buffer->MC0s.indices;

	const bool src_N = src_buffer->Ns.indices != NULL;
	const bool src_C0 = src_buffer->C0s.indices != NULL;
	const bool src_MC0= src_buffer->MC0s.indices != NULL;

	for( U32 v=0; v<src_buffer->num_vertices; v++ ) {
	
		memcpy( dst, ((char*)src_buffer->Ps.data) + (*src_Pi * src_P_stride), 3 * sizeof(Vector) );
		dst += 3 * sizeof(Vector) ;
		src_Pi++;

		if( src_N ) {
			memcpy( dst, ((char*)src_buffer->Ns.data) + (*src_Ni * src_N_stride), 3 * sizeof(Vector) );
			src_Ni++;
		}
		dst += 3 * sizeof(Vector);

		if( src_C0 ) {
			memcpy( dst, ((char*)src_buffer->C0s.data) + (*src_C0i * src_C0_stride), sizeof(U32) );
			src_C0i++;
		}
		dst += sizeof(U32);

		if( src_MC0 ) {
			memcpy( dst, ((char*)src_buffer->MC0s.data) + (*src_MC0i * src_MC0_stride), src_MC0_size );
			src_MC0i++;
		}
		dst += src_MC0_size ;
	}
}