#include "IMesh.h"
#include "VertexBufferDesc.h"

static void CopyVertexBufferItemDesc( const VertexBufferItemDesc & src, VertexBufferItemDesc & dest, const int idx_cnt )
{
	dest.size = src.size;
	dest.stride = src.stride;
	dest.count = src.count;

	if( src.data )
	{
		dest.data = new char[ src.count * src.stride ];
		memcpy(dest.data, src.data, src.count * src.stride);
	}
	else
	{
		dest.data = NULL;
	}

	if( src.indices )
	{
		dest.indices = new unsigned long[ idx_cnt ];
		memcpy(dest.indices, src.indices, idx_cnt * sizeof(unsigned long));
	}
	else
	{
		dest.indices = NULL;
	}
}

//

static void CopyVertexBufferDesc( const VertexBufferDesc & src, VertexBufferDesc & dest )
{
	dest.flags = src.flags;
	dest.vertex_format = src.vertex_format;
	dest.num_vertices = src.num_vertices;

	CopyVertexBufferItemDesc( src.Ps, dest.Ps, src.num_vertices );
	CopyVertexBufferItemDesc( src.Ns, dest.Ns, src.num_vertices );
	CopyVertexBufferItemDesc( src.C0s, dest.C0s, src.num_vertices );
	CopyVertexBufferItemDesc( src.C1s, dest.C1s, src.num_vertices );
	CopyVertexBufferItemDesc( src.MC0s, dest.MC0s, src.num_vertices );
	CopyVertexBufferItemDesc( src.MC1s, dest.MC1s, src.num_vertices );
}

//

static void FreeVertexBufferItemDesc( VertexBufferItemDesc & di )
{
	delete [] di.data;
	delete [] di.indices;
	memset( &di, 0, sizeof(VertexBufferItemDesc) );
}

//

static void FreeVertexBufferDesc( VertexBufferDesc & desc )
{
	FreeVertexBufferItemDesc( desc.Ps );
	FreeVertexBufferItemDesc( desc.Ns );
	FreeVertexBufferItemDesc( desc.C0s );
	FreeVertexBufferItemDesc( desc.C1s );
	FreeVertexBufferItemDesc( desc.MC0s );
	FreeVertexBufferItemDesc( desc.MC1s );

	memset(&desc, 0, sizeof(VertexBufferDesc));
}

//

static bool HasElements( const VertexBufferDesc & desc, IM_ELEMENT element_mask )
{
	if( (element_mask & IM_E_POINTS) && (desc.Ps.indices == NULL) )
		return false;

	if( (element_mask & IM_E_NORMALS) && (desc.Ns.indices == NULL) )
		return false;

	if( (element_mask & IM_E_UV0S) && (desc.MC0s.indices == NULL) )
		return false;

	if( (element_mask & IM_E_UV1S) && (desc.MC1s.indices == NULL) )
		return false;

	if( (element_mask & IM_E_COLOR0S) && (desc.C0s.indices == NULL) )
		return false;

	if( (element_mask & IM_E_COLOR1S) && (desc.C1s.indices == NULL) )
		return false;

	return true;
}

//