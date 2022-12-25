// TriMeshArchetype.cpp
//
//
//

//

#include <windows.h>

//

#include "dacom.h"
#include "FDUMP.h"
#include "FileSys_Utility.h"
#include "engine.h"
#include "renderer.h"
#include "tfuncs.h"
#include "TextureCoord.h"
#include "packed_argb.h"

//

#include "TriMeshArchetype.h"
#include "TriMeshInstance.h"
#include "VertexBufferDescUtil.h"

//

TriMeshArchetype::TriMeshArchetype()
{
	locked_facegroup = facegroups.end();
	locked_index_elements = 0;
	locked_point_indices = 0;
	locked_normal_indices = 0;
	locked_uv0_indices = 0;
	locked_uv1_indices = 0;
	locked_color0_indices = 0;
	locked_edge_angle_indices = 0;

	memset( &vertex_buffer_desc, 0, sizeof(vertex_buffer_desc) );

	lod = NULL;

	initialize(  );
}

//

TriMeshArchetype::~TriMeshArchetype()
{
	cleanup();
}

//

GENRESULT TriMeshArchetype::initialize( ) 
{
	if( FAILED( cleanup() ) ) {
		return GR_GENERIC;
	}	

	return GR_OK;
}

//

GENRESULT TriMeshArchetype::verify( void ) 
{
	return GR_OK;	
}

//

GENRESULT TriMeshArchetype::optimize( IM_OPTIMIZE_FLAG_BIT op_mask ) 
{
	optimize_data_sharing();

	return GR_OK;	
}

//

GENRESULT TriMeshArchetype::acquire_unique( void ) 
{
	return GR_GENERIC;
}

//

GENRESULT COMAPI TriMeshArchetype::release_unique( void ) 
{
	return GR_GENERIC;
}

//

GENRESULT TriMeshArchetype::get_num_facegroups( U32 *out_num_facegroups ) 
{
	*out_num_facegroups = facegroups.size();
	return GR_OK;	
}

//

GENRESULT TriMeshArchetype::lock_facegroup( U32 facegroup_num, bool read_only ) 
{
	if( locked_facegroup != facegroups.end() ) {
		return GR_GENERIC;	// only one facegroup allowed to be locked at once.
	}

	if( facegroup_num >= facegroups.size() ) {
		return GR_GENERIC;	// invalid facegroup number
	}

	if( read_only == false )
	{
		if( lod )
		{
			//set_lod( 1.0f );  // this would lock it at the highest detail instead of current
			lod->Release();
			lod = NULL;
		}

		TriMeshFaceGroup * fg = *(&facegroups[ facegroup_num ]);

		// this should go away when edges become writeable
		fg->set_edges( 0, NULL, NULL );

		fg->make_used_data_local();
	}

	locked_facegroup = &facegroups[ facegroup_num ];
	locked_read_only = read_only;

	return GR_OK;	
}

//

GENRESULT TriMeshArchetype::unlock_facegroup( void ) 
{
	if( locked_facegroup == facegroups.end() ) {
		return GR_GENERIC;
	}

	unlock_facegroup_data();
	unlock_facegroup_indices();

	locked_facegroup = facegroups.end();

	optimize_data_sharing( );

	return GR_OK;	
}

//

GENRESULT TriMeshArchetype::add_facegroup( D3DPRIMITIVETYPE primitive_type ) 
{
	if( locked_facegroup == facegroups.end() )
	{
		TriMeshFaceGroup *fg = new TriMeshFaceGroup;

		ASSERT( fg );

		fg->primitive_type = primitive_type;

		facegroups.push_back( fg );
		locked_facegroup = facegroups.end();
		
		return GR_OK;	
	}
	else
	{
		return GR_GENERIC;
	}
}

//

GENRESULT TriMeshArchetype::remove_facegroup( U32 facegroup_num ) 
{
	if( locked_facegroup = facegroups.end() )
	{
		FaceGroupList::iterator fg = &facegroups[ facegroup_num ];

		facegroups.erase( fg );

		locked_facegroup = facegroups.end();

		if( lod )
		{
			lod->Release();
			lod = NULL;
		}

		return GR_OK;	
	}
	else
	{
		return GR_GENERIC;
	}
}

//

GENRESULT TriMeshArchetype::get_facegroup_primitive_type( D3DPRIMITIVETYPE *out_primitive_type ) 
{
	if( locked_facegroup == facegroups.end() ) {
		return GR_GENERIC;
	}

	*out_primitive_type = (*locked_facegroup)->primitive_type;

	return GR_OK;	
}

//

GENRESULT TriMeshArchetype::get_facegroup_material( IMaterial **out_material ) 
{
	if( locked_facegroup == facegroups.end() ) {
		return GR_GENERIC;
	}

	if( (*locked_facegroup)->material_name[0] != 0 ) {
		if( (*out_material = (*locked_facegroup)->material) != NULL ) {
			(*out_material)->AddRef();
		}
	}
	else {
		*out_material = NULL;	
	}

	return GR_OK;	
}

//

GENRESULT TriMeshArchetype::set_facegroup_material( IMaterial *material ) 
{
	if( locked_facegroup == facegroups.end() ) {
		return GR_GENERIC;
	}

	if( locked_read_only ) {
		return GR_GENERIC;
	}

	if( material == (*locked_facegroup)->material ) {
		material->get_name( (*locked_facegroup)->material_name, IM_MAX_NAME_LEN );
	}
	else {

		if( ((*locked_facegroup)->material = material) != NULL ) {
			material->AddRef();
			material->get_name( (*locked_facegroup)->material_name, IM_MAX_NAME_LEN );
		}
		else {
			(*locked_facegroup)->material_name[0] = 0;
		}

		DACOM_RELEASE( (*locked_facegroup)->material );
	}

	return GR_OK;	
}

//

GENRESULT TriMeshArchetype::get_facegroup_material_context( void **out_context ) 
{
	if( locked_facegroup == facegroups.end() ) {
		return GR_GENERIC;
	}

	*out_context = (*locked_facegroup)->extended_context;
	
	return GR_OK;
}

//

GENRESULT TriMeshArchetype::set_facegroup_material_context( void *context ) 
{
	if( locked_facegroup == facegroups.end() ) {
		return GR_GENERIC;
	}

	if( locked_read_only ) {
		return GR_GENERIC;
	}

	(*locked_facegroup)->extended_context = context;
	
	return GR_OK;
}

//

GENRESULT TriMeshArchetype::lock_facegroup_data( IM_ELEMENT element_mask ) 
{
	if( locked_facegroup == facegroups.end() ) {
		return GR_GENERIC;
	}

	// Edges are read only until IMesh changes
	if( !locked_read_only && (element_mask & (IM_E_EDGES|IM_E_EDGE_ANGLES)) )
	{
		return GR_GENERIC;
	}

	if( !(*locked_facegroup)->has_elements( element_mask ) )
	{
		return GR_DATA_NOT_FOUND;
	}

	return GR_OK;	
}

//

GENRESULT TriMeshArchetype::unlock_facegroup_data( void ) 
{
	if( locked_facegroup == facegroups.end() ) {
		return GR_GENERIC;
	}
	return GR_OK;	
}

//

GENRESULT TriMeshArchetype::lock_facegroup_indices( IM_ELEMENT element_mask ) 
{
	if( locked_facegroup == facegroups.end() ) {
		return GR_GENERIC;
	}

	if( !(*locked_facegroup)->has_elements( element_mask ) )
	{
		return GR_DATA_NOT_FOUND;
	}

	GENRESULT result = GR_GENERIC;

	if( element_mask & IM_E_POINTS && !(locked_index_elements & IM_E_POINTS) ) {
		if( !FAILED(lock_indices( IM_E_POINTS )) ) {
			result = GR_OK;
		}
	}

	if( element_mask & IM_E_NORMALS && !(locked_index_elements & IM_E_NORMALS) ) {
		if( !FAILED(lock_indices( IM_E_NORMALS )) ) {
			result = GR_OK;
		}
	}

	if( element_mask & IM_E_UV0S && !(locked_index_elements & IM_E_UV0S) ) {
		if( !FAILED(lock_indices( IM_E_UV0S )) ) {
			result = GR_OK;
		}
	}

	if( element_mask & IM_E_UV1S && !(locked_index_elements & IM_E_UV1S) ) {
		if( !FAILED(lock_indices( IM_E_UV1S )) ) {
			result = GR_OK;
		}
	}

	if( element_mask & IM_E_COLOR0S && !(locked_index_elements & IM_E_COLOR0S) ) {
		if( !FAILED(lock_indices( IM_E_COLOR0S )) ) {
			result = GR_OK;
		}
	}

	// Edge stuff is read only until IMesh changes
	if( locked_read_only )
	{
		if( element_mask & IM_E_EDGES && !(locked_index_elements & IM_E_EDGES) ) {
			result = GR_OK;
		}

		if( element_mask & IM_E_EDGE_ANGLES && !(locked_index_elements & IM_E_EDGE_ANGLES) ) {
			if( !FAILED(lock_edge_angle_indices()) ) {
				result = GR_OK;
			}
		}
	}

	if( result == GR_OK )
	{
		locked_index_elements |= element_mask;
	}

	return result;	
}

//

GENRESULT TriMeshArchetype::unlock_facegroup_indices( void ) 
{
	if( locked_facegroup == facegroups.end() ) {
		return GR_GENERIC;
	}

	if( locked_index_elements & IM_E_POINTS )
		unlock_indices( IM_E_POINTS );

	if( locked_index_elements & IM_E_NORMALS )
		unlock_indices( IM_E_NORMALS );

	if( locked_index_elements & IM_E_UV0S )
		unlock_indices( IM_E_UV0S );

	if( locked_index_elements & IM_E_UV1S )
		unlock_indices( IM_E_UV1S );

	if( locked_index_elements & IM_E_COLOR0S )
		unlock_indices( IM_E_COLOR0S );

	if( locked_index_elements & IM_E_EDGE_ANGLES )
		unlock_edge_angle_indices();

	locked_index_elements = 0;

	return GR_OK;	
}

//

GENRESULT TriMeshArchetype::get_facegroup_data( IM_ELEMENT element_bit, void const **out_data, U32 *out_stride, U32 *out_size, U32 *out_count ) 
{
	if( locked_facegroup == facegroups.end() ) {
		return GR_GENERIC;
	}

	TriMeshFaceGroup & fg = **locked_facegroup;

	if( !fg.has_elements( element_bit ) )
	{
		return GR_GENERIC;
	}

	switch( element_bit ) {

	case IM_E_POINTS:
		*out_data   = fg.current_vertex_desc->Ps.data;
		*out_stride = fg.current_vertex_desc->Ps.stride;
		*out_count  = fg.current_vertex_desc->Ps.count;
		*out_size   = fg.current_vertex_desc->Ps.size;
		break;

	case IM_E_NORMALS:
		*out_data   = fg.current_vertex_desc->Ns.data;
		*out_stride = fg.current_vertex_desc->Ns.stride;
		*out_count  = fg.current_vertex_desc->Ns.count;
		*out_size   = fg.current_vertex_desc->Ns.size;
		break;

	case IM_E_UV0S:
		*out_data   = fg.current_vertex_desc->MC0s.data;
		*out_stride = fg.current_vertex_desc->MC0s.stride;
		*out_count  = fg.current_vertex_desc->MC0s.count;
		*out_size   = fg.current_vertex_desc->MC0s.size;
		break;

	case IM_E_UV1S:
		*out_data   = fg.current_vertex_desc->MC1s.data;
		*out_stride = fg.current_vertex_desc->MC1s.stride;
		*out_count  = fg.current_vertex_desc->MC1s.count;
		*out_size   = fg.current_vertex_desc->MC1s.size;
		break;

	case IM_E_COLOR0S:
		*out_data   = fg.current_vertex_desc->C0s.data;
		*out_stride = fg.current_vertex_desc->C0s.stride;
		*out_count  = fg.current_vertex_desc->C0s.count;
		*out_size   = fg.current_vertex_desc->C0s.size;
		break;

	case IM_E_EDGES:
		if( locked_read_only )
		{
			*out_data   = fg.current_vertex_desc->Ps.data;
			*out_stride = fg.current_vertex_desc->Ps.stride;
			*out_count  = fg.current_vertex_desc->Ps.count;
			*out_size   = fg.current_vertex_desc->Ps.size;
		}
		else
		{
			return GR_GENERIC;
		}
		break;

	case IM_E_EDGE_ANGLES:
		if( locked_read_only )
		{
			*out_data   = fg.edge_angles;
			*out_stride = sizeof(float);
			*out_count  = fg.num_edge_indices / 2;
			*out_size   = sizeof(float);
		}
		else
		{
			return GR_GENERIC;
		}
		break;

	default:
		return GR_GENERIC;

	}

	return GR_OK;	
}

//

GENRESULT TriMeshArchetype::set_facegroup_data( IM_ELEMENT element_bit, const void *data, U32 stride, U32 size, U32 count ) 
{
	if( locked_facegroup == facegroups.end() ) {
		return GR_GENERIC;
	}

	if( locked_read_only ) {
		return GR_GENERIC;
	}

	TriMeshFaceGroup & fg = **locked_facegroup;

	ASSERT( fg.current_vertex_desc != fg.shared_vertex_desc );

	switch( element_bit ) {

	case IM_E_POINTS:
		if( count * size > fg.current_vertex_desc->Ps.count * fg.current_vertex_desc->Ps.size )
		{
			delete [] fg.current_vertex_desc->Ps.data;
			fg.current_vertex_desc->Ps.data = new U8[count * size];
		}

		if( stride == size )
		{
			memcpy(fg.current_vertex_desc->Ps.data, data, count * stride);
		}
		else
		{
			for(int i = 0; i < count; i++)
			{
				memcpy((U8*)fg.current_vertex_desc->Ps.data + i * size,
					(U8*)data + i * stride, size);
			}
		}
		
		fg.current_vertex_desc->Ps.stride = stride;
		fg.current_vertex_desc->Ps.count = count;
		fg.current_vertex_desc->Ps.size = size;
		break;

	case IM_E_NORMALS:
		if( count * size > fg.current_vertex_desc->Ns.count * fg.current_vertex_desc->Ns.size )
		{
			delete [] fg.current_vertex_desc->Ns.data;
			fg.current_vertex_desc->Ns.data = new U8[count * size];
		}

		if( stride == size )
		{
			memcpy(fg.current_vertex_desc->Ns.data, data, count * stride);
		}
		else
		{
			for(int i = 0; i < count; i++)
			{
				memcpy((U8*)fg.current_vertex_desc->Ns.data + i * size,
					(U8*)data + i * stride, size);
			}
		}
		
		fg.current_vertex_desc->Ns.stride = stride;
		fg.current_vertex_desc->Ns.count = count;
		fg.current_vertex_desc->Ns.size = size;
		break;

	case IM_E_UV0S:
		if( count * size > fg.current_vertex_desc->MC0s.count * fg.current_vertex_desc->MC0s.size )
		{
			delete [] fg.current_vertex_desc->MC0s.data;
			fg.current_vertex_desc->MC0s.data = new U8[count * size];
		}

		if( stride == size )
		{
			memcpy(fg.current_vertex_desc->MC0s.data, data, count * stride);
		}
		else
		{
			for(int i = 0; i < count; i++)
			{
				memcpy((U8*)fg.current_vertex_desc->MC0s.data + i * size,
					(U8*)data + i * stride, size);
			}
		}
		
		fg.current_vertex_desc->MC0s.stride = stride;
		fg.current_vertex_desc->MC0s.count = count;
		fg.current_vertex_desc->MC0s.size = size;
		break;

	case IM_E_UV1S:
		if( count * size > fg.current_vertex_desc->MC1s.count * fg.current_vertex_desc->MC1s.size )
		{
			delete [] fg.current_vertex_desc->MC1s.data;
			fg.current_vertex_desc->MC1s.data = new U8[count * size];
		}

		if( stride == size )
		{
			memcpy(fg.current_vertex_desc->MC1s.data, data, count * stride);
		}
		else
		{
			for(int i = 0; i < count; i++)
			{
				memcpy((U8*)fg.current_vertex_desc->MC1s.data + i * size,
					(U8*)data + i * stride, size);
			}
		}
		
		fg.current_vertex_desc->MC1s.stride = stride;
		fg.current_vertex_desc->MC1s.count = count;
		fg.current_vertex_desc->MC1s.size = size;
		break;

	case IM_E_COLOR0S:
		if( count * size > fg.current_vertex_desc->C0s.count * fg.current_vertex_desc->C0s.size )
		{
			delete [] fg.current_vertex_desc->C0s.data;
			fg.current_vertex_desc->C0s.data = new U8[count * size];
		}

		if( stride == size )
		{
			memcpy(fg.current_vertex_desc->C0s.data, data, count * stride);
		}
		else
		{
			for(int i = 0; i < count; i++)
			{
				memcpy((U8*)fg.current_vertex_desc->C0s.data + i * size,
					(U8*)data + i * stride, size);
			}
		}
		
		fg.current_vertex_desc->C0s.stride = stride;
		fg.current_vertex_desc->C0s.count = count;
		fg.current_vertex_desc->C0s.size = size;
		break;

	case IM_E_EDGES:
		return GR_GENERIC;

	case IM_E_EDGE_ANGLES:
		return GR_GENERIC;

	default:
		return GR_GENERIC;

	}

	return GR_OK;	
}

//
// IMesh needs to be changed to take a parameter as to which indices (face or edge)
GENRESULT TriMeshArchetype::get_facegroup_indices_count( U32 *out_num_indices ) 
{
	if( locked_facegroup == facegroups.end() ) {
		return GR_GENERIC;
	}

	*out_num_indices = (*locked_facegroup)->num_face_indices;

	return GR_OK;	
}

//
// IMesh needs to be changed to take a parameter as to which indices (face or edge)
GENRESULT TriMeshArchetype::set_facegroup_indices_count( U32 num_indices ) 
{
	if( locked_facegroup == facegroups.end() )
	{
		return GR_GENERIC;
	}

	if( locked_read_only )
	{
		return GR_GENERIC;
	}

	if( locked_index_elements != 0 )
	{
		return GR_GENERIC;
	}

	if( (*locked_facegroup)->num_face_indices < num_indices ) // growing is slower
	{
		delete [] (*locked_facegroup)->face_indices;
		(*locked_facegroup)->face_indices = new U16[num_indices];
	}

	(*locked_facegroup)->num_face_indices = num_indices;
	//memset((*locked_facegroup)->face_indices, 0xff, (*locked_facegroup)->num_face_indices * sizeof(U16) );

	return GR_OK;	
}

//

GENRESULT TriMeshArchetype::get_facegroup_indices( IM_ELEMENT element_bit, U16 const **out_indices, U32 *out_count ) 
{
	if( locked_facegroup == facegroups.end() ) {
		return GR_GENERIC;
	}

	ASSERT( locked_index_elements & element_bit );
	ASSERT( (*locked_facegroup)->has_elements( element_bit ) );

	switch( element_bit ) {

	case IM_E_POINTS:
		*out_indices = locked_point_indices;
		*out_count   = (*locked_facegroup)->num_face_indices;
		break;

	case IM_E_NORMALS:
		*out_indices = locked_normal_indices;
		*out_count   = (*locked_facegroup)->num_face_indices;
		break;

	case IM_E_UV0S:
		*out_indices = locked_uv0_indices;
		*out_count   = (*locked_facegroup)->num_face_indices;
		break;

	case IM_E_EDGES:
		ASSERT( locked_read_only );
		*out_indices = (*locked_facegroup)->edge_indices;
		*out_count   = (*locked_facegroup)->num_edge_indices;
		break;

	case IM_E_EDGE_ANGLES:
		ASSERT( locked_read_only );
		*out_indices = locked_edge_angle_indices;
		*out_count   = (*locked_facegroup)->num_edge_indices / 2;
		break;

	default:
		return GR_GENERIC;

	}

	return GR_OK;	
}

//

GENRESULT TriMeshArchetype::set_facegroup_indices( IM_ELEMENT element_bit, const U16 *indices, U32 count ) 
{
	if( locked_facegroup == facegroups.end() ) {
		return GR_GENERIC;
	}

	if( locked_read_only ) {
		return GR_GENERIC;
	}

	if( !(locked_index_elements & element_bit) ) {
		return GR_GENERIC;
	}

	if( count != (*locked_facegroup)->num_face_indices ) {
		GENERAL_TRACE_1( "Number of indices does not equal number of facegroup indices\n" );
		return GR_GENERIC;
	}

	switch( element_bit ) {

	case IM_E_POINTS:
		memcpy( locked_point_indices, indices, count * sizeof(U16) );
		break;

	case IM_E_NORMALS:
		memcpy( locked_normal_indices, indices, count * sizeof(U16) );
		break;

	case IM_E_UV0S:
		memcpy( locked_uv0_indices, indices, count * sizeof(U16) );
		break;

	case IM_E_EDGES:
		ASSERT(0);
		break;

	case IM_E_EDGE_ANGLES:
		ASSERT(0);
		break;

	default:
		return GR_GENERIC;

	}

	return GR_OK;	
}

//

GENRESULT TriMeshArchetype::add_elements( IM_ELEMENT element_bits )
{
	if( locked_facegroup == facegroups.end() )
	{
		return GR_GENERIC;
	}

	if( locked_read_only )
	{
		return GR_GENERIC;
	}

	if( locked_index_elements != 0 )
	{
		return GR_GENERIC;
	}

	GENRESULT result = GR_GENERIC;

	VertexBufferDesc & vbd = *((*locked_facegroup)->current_vertex_desc);

	if( element_bits & IM_E_POINTS ) 
	{
		result = GR_OK;
		if( vbd.Ps.indices == NULL )
		{
			vbd.Ps.indices = new U32[vbd.num_vertices];
			vbd.vertex_format |= D3DFVF_XYZ;
		}
	}

	if( element_bits & IM_E_NORMALS )
	{
		result = GR_OK;
		if( vbd.Ns.indices == NULL )
		{
			vbd.Ns.indices = new U32[vbd.num_vertices];
			vbd.vertex_format |= D3DFVF_NORMAL;
		}
	}

	if( element_bits & IM_E_UV0S )
	{
		result = GR_OK;
		if( vbd.MC0s.indices == NULL )
		{
			vbd.MC0s.indices = new U32[vbd.num_vertices];
			vbd.vertex_format |= D3DFVF_TEX1;
		}
	}

	if( element_bits & IM_E_UV1S )
	{
		result = GR_OK;
		if( vbd.MC1s.indices == NULL )
		{
			vbd.MC1s.indices = new U32[vbd.num_vertices];
			vbd.vertex_format |= D3DFVF_TEX2;
		}
	}

	if( element_bits & IM_E_COLOR0S )
	{
		result = GR_OK;
		if( vbd.C0s.indices == NULL )
		{
			vbd.C0s.indices = new U32[vbd.num_vertices];
			vbd.vertex_format |= D3DFVF_DIFFUSE;
		}
	}

//TODO when IMesh allows edge writting
	if( element_bits & IM_E_EDGES )
	{
		result = GR_GENERIC;
	}

	if( element_bits & IM_E_EDGE_ANGLES )
	{
		result = GR_GENERIC;
	}

	return result;
}

//

GENRESULT TriMeshArchetype::remove_elements( IM_ELEMENT element_bits )
{
	if( locked_facegroup == facegroups.end() )
	{
		return GR_GENERIC;
	}

	if( locked_read_only )
	{
		return GR_GENERIC;
	}

	if( locked_index_elements != 0 )
	{
		return GR_GENERIC;
	}

	GENRESULT result = GR_GENERIC;

	VertexBufferDesc & vbd = *((*locked_facegroup)->current_vertex_desc);

	if( element_bits & IM_E_POINTS )
	{
		FreeVertexBufferItemDesc( vbd.Ps );
		vbd.vertex_format &= ~D3DFVF_XYZ;
		result = GR_OK;
	}

	if( element_bits & IM_E_NORMALS )
	{
		FreeVertexBufferItemDesc( vbd.Ns );
		vbd.vertex_format &=~ D3DFVF_NORMAL;
		result = GR_OK;
	}

	if( element_bits & IM_E_UV0S )
	{
		FreeVertexBufferItemDesc( vbd.MC0s );
		vbd.vertex_format &= ~D3DFVF_TEX1;
		result = GR_OK;
	}

	if( element_bits & IM_E_UV1S )
	{
		FreeVertexBufferItemDesc( vbd.MC1s );
		vbd.vertex_format &= ~D3DFVF_TEX2;
		result = GR_OK;
	}

	if( element_bits & IM_E_COLOR0S )
	{
		FreeVertexBufferItemDesc( vbd.C0s );
		vbd.vertex_format &= ~D3DFVF_DIFFUSE;
		result = GR_OK;
	}

	if( element_bits & IM_E_EDGES )
	{
		TriMeshFaceGroup & fg = **locked_facegroup;
		delete [] fg.edge_indices;
		fg.edge_indices = NULL;
		if( fg.edge_angles == NULL )
		{
			fg.num_edge_indices = 0;
		}

		result = GR_OK;
	}

	if( element_bits & IM_E_EDGE_ANGLES )
	{
		TriMeshFaceGroup & fg = **locked_facegroup;
		delete [] fg.edge_angles;
		fg.edge_angles = NULL;
		if( fg.edge_indices == NULL )
		{
			fg.num_edge_indices = 0;
		}

		result = GR_OK;
	}

	return result;
}

//

HRESULT TriMeshArchetype::cleanup( void )
{
	unlock_facegroup();

	centroid.zero();
	
	memset( bounding_box, 0, sizeof(float) * 6 );
	
	bounding_sphere_center.zero();
	bounding_sphere_radius = -1.0f;

	FaceGroupList::iterator beg = facegroups.begin();
	FaceGroupList::iterator end = facegroups.end();
	FaceGroupList::iterator fg;

	for( fg=beg; fg!=end; fg++ ) {
		delete (*fg);
	}

	facegroups.clear();

	FreeVertexBufferDesc( vertex_buffer_desc );

	if( lod ) {
		lod->Release();
		lod = NULL;
	}

	tm_f_flags = 0;

	fraction =
	fraction_previous = 1.0f;
	threshold = 0.0001f;

	return S_OK;
}

//

HRESULT TriMeshArchetype::load( IFileSystem *ifs, IRenderPipeline *renderpipeline, IVertexBufferManager *vbuffermanager, IMaterialLibrary *material_library )
{
	char buffer[MAX_PATH];
	TriMeshFaceGroup *facegroup;

	ifs->GetAbsolutePath( path, "", MAX_PATH );

	tm_f_flags &= ~(TM_F_MESH_VALID_BIT);

	if( !ifs->SetCurrentDirectory( "Mesh" ) ) {
		return E_FAIL;
	}

	// Set up the vertex buffer descriptor, the code below fills in
	// in the data and indices
	//
	memset( &vertex_buffer_desc, 0, sizeof(VertexBufferDesc) );

	vertex_buffer_desc.flags |= VBD_F_INDEXED;

	if( ifs->SetCurrentDirectory( "Geometry" ) ) {

		TexCoord *uv;
		Vector *v;
		PACKEDARGB *col;

		ReadAllocVector<Vector>( ifs, "Points", vertex_buffer_desc.Ps.count, v );
		vertex_buffer_desc.Ps.data = v;
		vertex_buffer_desc.vertex_format = D3DFVF_XYZ;
		vertex_buffer_desc.Ps.stride = sizeof(Vector);
		vertex_buffer_desc.Ps.size = sizeof(Vector);
		ReadAllocVector<U32>( ifs, "Point_indices", vertex_buffer_desc.num_vertices, vertex_buffer_desc.Ps.indices );

		ReadAllocVector<Vector>( ifs, "Vertex_normals", vertex_buffer_desc.Ns.count, v );
		if( vertex_buffer_desc.Ns.count )
		{
			vertex_buffer_desc.Ns.data = v;
			vertex_buffer_desc.vertex_format |= D3DFVF_NORMAL;
			vertex_buffer_desc.Ns.stride = sizeof(Vector);
			vertex_buffer_desc.Ns.size = sizeof(Vector);
			ReadAllocVector<U32>( ifs, "Vertex_normal_indices", vertex_buffer_desc.num_vertices, vertex_buffer_desc.Ns.indices );
		}

		ReadAllocVector<TexCoord>( ifs, "UV0", vertex_buffer_desc.MC0s.count, uv );
		if( vertex_buffer_desc.MC0s.count )
		{
			vertex_buffer_desc.MC0s.data = uv;
			vertex_buffer_desc.vertex_format |= D3DFVF_TEX1;
			vertex_buffer_desc.MC0s.stride = sizeof(float[2]);
			vertex_buffer_desc.MC0s.size = sizeof(float[2]);
			ReadAllocVector<U32>( ifs, "UV0_indices", vertex_buffer_desc.num_vertices, vertex_buffer_desc.MC0s.indices );
		}

		ReadAllocVector<TexCoord>( ifs, "UV1", vertex_buffer_desc.MC1s.count, uv );
		if( vertex_buffer_desc.MC1s.count )
		{
			vertex_buffer_desc.MC1s.data = uv;
			vertex_buffer_desc.vertex_format |= D3DFVF_TEX2;
			vertex_buffer_desc.MC1s.stride = sizeof(float[2]);
			vertex_buffer_desc.MC1s.size = sizeof(float[2]);
			ReadAllocVector<U32>( ifs, "UV1_indices", vertex_buffer_desc.num_vertices, vertex_buffer_desc.MC1s.indices );
		}

		ReadAllocVector<PACKEDARGB>( ifs, "Color0", vertex_buffer_desc.C0s.count, col );
		if( vertex_buffer_desc.C0s.count )
		{
			vertex_buffer_desc.C0s.data = col;
			vertex_buffer_desc.vertex_format |= D3DFVF_DIFFUSE;
			vertex_buffer_desc.C0s.stride = sizeof(PACKEDARGB);
			vertex_buffer_desc.C0s.size = sizeof(PACKEDARGB);
			ReadAllocVector<U32>( ifs, "Color0_indices", vertex_buffer_desc.num_vertices, vertex_buffer_desc.C0s.indices );
		}

		ifs->SetCurrentDirectory( ".." );	// out of "Geometry"
	}

	if( ifs->SetCurrentDirectory( "Face_groups" ) ) {

		U32 cnt;

		if( SUCCEEDED( read_type<U32>( ifs, "Count", &cnt ) ) ) {
			
			for( U32 fg=0; fg<cnt; fg++ ) {

				sprintf( buffer, "Group%d", fg );

				if( ifs->SetCurrentDirectory( buffer ) ) {

					facegroup = new TriMeshFaceGroup;
					ASSERT( facegroup );

					if( SUCCEEDED( read_string( ifs, "Material_name", IM_MAX_NAME_LEN, facegroup->material_name ) ) ) {
						material_library->find_material( facegroup->material_name, &facegroup->material );
					}

					ReadAllocVector<U16>( ifs, "Face_indices", facegroup->num_face_indices, facegroup->face_indices );

					ReadAllocVector<U16>( ifs, "Edge_indices", facegroup->num_edge_indices, facegroup->edge_indices );
					U32 num_angles;
					ReadAllocVector<float>( ifs, "Edge_angles", num_angles, facegroup->edge_angles );

					facegroup->set_min_max_info();

					facegroup->shared_vertex_desc =
					facegroup->current_vertex_desc = &vertex_buffer_desc;

					facegroups.push_back( facegroup );
					locked_facegroup = facegroups.end();	// update iterator

					ifs->SetCurrentDirectory( ".." ); // out of "Group#" 
				}
			}
		}

		ifs->SetCurrentDirectory( ".." );		// out of "FaceGroups"
	}
	
	if( ifs->SetCurrentDirectory( "Lod_library" ) && (NULL != (lod = new TriLODLib)) ) {

		lod->Init();

		read_type<int>( ifs, "Step_count", &(lod->step_cnt) );
		ReadAllocVector<TriLODStep>( ifs, "Step_list", (U32&)lod->step_cnt, lod->step_list );

		ReadAllocVector<int>( ifs, "Removed_face_list", (U32&)lod->removed_face_cnt, lod->removed_face_list );
		ReadAllocVector<int>( ifs, "Vertex_list", (U32&)lod->vertex_cnt, lod->vertex_list );
		ReadAllocVector<int>( ifs, "UV_batch_chain", (U32&)lod->uv_chain_count, lod->uv_batch_chain );

		ReadAllocVector<int>( ifs, "High_UV_id_1", (U32&)lod->uv_count, lod->uv_high_id1 );
		ReadAllocVector<int>( ifs, "High_batch_count_1", (U32&)lod->uv_count, lod->uv_batch_cnt1 );
		ReadAllocVector<int>( ifs, "High_batch_first_1", (U32&)lod->uv_count, lod->uv_batch_first1 );
		ReadAllocVector<int>( ifs, "High_UV_id_2", (U32&)lod->uv_count, lod->uv_high_id2 );
		ReadAllocVector<int>( ifs, "High_batch_count_2", (U32&)lod->uv_count, lod->uv_batch_cnt2 );
		ReadAllocVector<int>( ifs, "High_batch_first_2", (U32&)lod->uv_count, lod->uv_batch_first2 );
		ReadAllocVector<int>( ifs, "Low_UV_id", (U32&)lod->uv_count, lod->uv_low_id );
		
		ifs->SetCurrentDirectory( ".." );	// out of "Lod library"
	}
	
	ifs->SetCurrentDirectory( ".." );		// out of "Mesh"

	if( lod )
	{
		// set up uv indexing for lod interpolation
		set_uv_indices(lod->step_cnt - 1);
		interpolate_lod_step(lod->step_cnt - 1, 1.0f);
	}

	// Now we are valid
	//
	tm_f_flags |= TM_F_MESH_VALID_BIT;

	return S_OK;
}

//

HRESULT TriMeshArchetype::clone( TriMeshArchetype **out_archetype  )
{
	ASSERT( out_archetype );

	*out_archetype = new TriMeshArchetype;
	TriMeshArchetype & dest = **out_archetype;

	float current_lod_fraction;
	if( lod )
	{
		current_lod_fraction = fraction;
		set_lod( 1.0f );	// has to be copied in the highest LOD state
	}

	CopyVertexBufferDesc( vertex_buffer_desc, dest.vertex_buffer_desc );	

	FaceGroupList::const_iterator beg = facegroups.begin();
	FaceGroupList::const_iterator end = facegroups.end();
	FaceGroupList::const_iterator fg;
	for( fg = beg; fg != end; fg++ )
	{
		TriMeshFaceGroup *new_fg = new TriMeshFaceGroup( **fg );

		if( new_fg->current_vertex_desc == new_fg->shared_vertex_desc )
		{
			new_fg->shared_vertex_desc =
			new_fg->current_vertex_desc = &dest.vertex_buffer_desc;
		}
		else
		{
			new_fg->shared_vertex_desc = &dest.vertex_buffer_desc;
		}

		dest.facegroups.push_back( new_fg );
	}

	dest.tm_f_flags = tm_f_flags;
	dest.centroid = centroid;
	memcpy(dest.bounding_box, bounding_box, 6 * sizeof(float));
	dest.bounding_sphere_center = bounding_sphere_center;
	dest.bounding_sphere_radius = bounding_sphere_radius;
	strncpy(dest.path, path, MAX_PATH);
	
	if( lod )
	{
		fraction_previous = current_lod_fraction; // suppress thrashing warning
		set_lod( current_lod_fraction );
		lod->AddRef();
		dest.lod = lod;

		dest.set_lod( dest.fraction );
	}

	dest.fraction = fraction;
	dest.fraction_previous = fraction_previous;
	dest.threshold = threshold;

	return E_FAIL;
}

//

// returns E_FAIL if plane does not disect mesh	
HRESULT TriMeshArchetype::split( const Vector & plane_normal, float plane_d, U32 split_flags,
								TriMeshArchetype **out_archetype_0, TriMeshArchetype **out_archetype_1 ) const
{
	ASSERT(out_archetype_0 && out_archetype_1);

	*out_archetype_0 = new TriMeshArchetype;
	*out_archetype_1 = new TriMeshArchetype;
	
	FaceGroupList::const_iterator beg = facegroups.begin();
	FaceGroupList::const_iterator end = facegroups.end();
	FaceGroupList::const_iterator fgi;

	int g_id = 0;
	for( fgi = beg; fgi != end; fgi++, g_id++ )
	{
		int front_face_cnt = 0;
		int back_face_cnt = 0;
		int split_face_cnt = 0;

		TriMeshFaceGroup & src_fg = **fgi;
		const int face_cnt = src_fg.num_face_indices/3;

		FaceStatus *f_status = new FaceStatus[face_cnt];

		for(int f_id = 0; f_id < face_cnt; f_id++)
		{
			f_status[f_id] = src_fg.get_face_side( f_id, plane_normal, plane_d );

			if( f_status[f_id] == PS_FRONT )
			{
				front_face_cnt++;
			}else
			if( f_status[f_id] == PS_BACK )
			{
				back_face_cnt++;
			}else
			if( !(split_flags & SA_KEEP_NONE) && (f_status[f_id] & PS_SPAN) )
			{
				if( split_flags & (SA_SPLIT_JAGGED | SA_SPLIT_EXACT) )
				{
					split_face_cnt++;
				}
				else // we make SA_SPLIT_NONE the default even if no flags are specified
				{
					if( f_status[f_id] & (PS_SPAN01_2 | PS_SPAN02_1 | PS_SPAN12_0) )
					{
						f_status[f_id] = PS_FRONT;
						front_face_cnt++;
					}
					else
					{
						f_status[f_id] = PS_BACK;
						back_face_cnt++;
					}
				}
			}
		}

		TriMeshFaceGroup *new_fg_front = new TriMeshFaceGroup;
		new_fg_front->copy_face_group_style( src_fg );
		new_fg_front->shared_vertex_desc =
		new_fg_front->current_vertex_desc = &((*out_archetype_0)->vertex_buffer_desc);

		TriMeshFaceGroup *new_fg_back = new TriMeshFaceGroup;
		new_fg_back->copy_face_group_style( src_fg );
		new_fg_back->shared_vertex_desc =
		new_fg_back->current_vertex_desc = &((*out_archetype_1)->vertex_buffer_desc);

		if( front_face_cnt > 0 )
		{
			new_fg_front->shared_vertex_desc =
			new_fg_front->current_vertex_desc = src_fg.current_vertex_desc;
			new_fg_front->copy_selected_faces( src_fg, f_status, PS_FRONT, front_face_cnt );
			new_fg_front->shared_vertex_desc = &((*out_archetype_0)->vertex_buffer_desc);
		}

		if( back_face_cnt > 0 )
		{
			new_fg_back->shared_vertex_desc =
			new_fg_back->current_vertex_desc = src_fg.current_vertex_desc;
			new_fg_back->copy_selected_faces( src_fg, f_status, PS_BACK, back_face_cnt );
			new_fg_back->shared_vertex_desc = &((*out_archetype_1)->vertex_buffer_desc);
		}

		if( split_face_cnt > 0 )
		{
			src_fg.distribute_split_faces(*new_fg_front, *new_fg_back, f_status, split_face_cnt);
		}

		// insert
		if( new_fg_front->num_face_indices > 0 )
		{
			(*out_archetype_0)->facegroups.push_back( new_fg_front );
			(*out_archetype_0)->locked_facegroup = (*out_archetype_0)->facegroups.end();
		}
		else
		{
			delete new_fg_front;
		}

		// insert
		if( new_fg_back->num_face_indices > 0 )
		{
			(*out_archetype_1)->facegroups.push_back( new_fg_back );
			(*out_archetype_1)->locked_facegroup = (*out_archetype_1)->facegroups.end();
		}
		else
		{
			delete new_fg_back;
		}

		delete [] f_status;
	}

	if( (*out_archetype_0)->facegroups.size() && (*out_archetype_1)->facegroups.size() )
	{
		(*out_archetype_0)->optimize_data_sharing();
		(*out_archetype_1)->optimize_data_sharing();

		(*out_archetype_0)->tm_f_flags = TM_F_MESH_VALID_BIT;
		(*out_archetype_1)->tm_f_flags = TM_F_MESH_VALID_BIT;

		(*out_archetype_0)->path[0] = 0;
		(*out_archetype_1)->path[0] = 0;

		return S_OK;
	}
	else
	{
		delete *out_archetype_0;
		*out_archetype_0 = NULL;
		delete *out_archetype_1;
		*out_archetype_1 = NULL;

		return E_FAIL;
	}
}

//

// this will free shared data if it's not being used by any face group
// and then recreate new shared data from the individual face groups
HRESULT TriMeshArchetype::optimize_data_sharing( void )
{
	// free shared vertex buffer data if no face group is using it any more
	FaceGroupList::iterator beg = facegroups.begin();
	FaceGroupList::iterator end = facegroups.end();
	FaceGroupList::iterator fgi;

	TriMeshFaceGroup & fg0 = **beg;

	bool can_merge = true;
	int index_cnt = 0;
	int point_cnt = 0;
	int normal_cnt = 0;
	int mc0_cnt = 0;
	int mc1_cnt = 0;
	int c0_cnt = 0;

	for( fgi = beg; fgi != end; fgi++ )
	{
		if( (*fgi)->current_vertex_desc == &vertex_buffer_desc )
		{
			return S_OK; // shared buffer is still being used
		}

		TriMeshFaceGroup & fg = **fgi;

		if( fg0.current_vertex_desc->vertex_format != fg.current_vertex_desc->vertex_format ||
			fg0.current_vertex_desc->flags != fg.current_vertex_desc->flags )
		{
			can_merge = false;
		}
		else
		{
			index_cnt += fg.current_vertex_desc->num_vertices;

			if( fg.current_vertex_desc->vertex_format & D3DFVF_XYZ )
			{
				point_cnt += fg.current_vertex_desc->Ps.count;
			}
			if( fg.current_vertex_desc->vertex_format & D3DFVF_NORMAL )
			{
				normal_cnt += fg.current_vertex_desc->Ns.count;
			}
			if( fg.current_vertex_desc->vertex_format & D3DFVF_TEX1 )
			{
				mc0_cnt += fg.current_vertex_desc->MC0s.count;
			}
			if( fg.current_vertex_desc->vertex_format & D3DFVF_TEX2 )
			{
				mc1_cnt += fg.current_vertex_desc->MC1s.count;
			}
			if( fg.current_vertex_desc->vertex_format & D3DFVF_DIFFUSE )
			{
				c0_cnt += fg.current_vertex_desc->C0s.count;
			}
		}
	}

	FreeVertexBufferDesc( vertex_buffer_desc );

	if( !can_merge )
		return S_OK;

	// go ahead and merge


	vertex_buffer_desc.flags = fg0.current_vertex_desc->flags;
	vertex_buffer_desc.vertex_format = fg0.current_vertex_desc->vertex_format;
	vertex_buffer_desc.num_vertices = index_cnt;

	vertex_buffer_desc.Ps.count = point_cnt;
	vertex_buffer_desc.Ps.size = fg0.current_vertex_desc->Ps.size;
	vertex_buffer_desc.Ps.stride = fg0.current_vertex_desc->Ps.stride;
	vertex_buffer_desc.Ps.indices = new U32[index_cnt];
	vertex_buffer_desc.Ps.data = new U8[point_cnt * fg0.current_vertex_desc->Ps.stride];

	if( normal_cnt )
	{
		vertex_buffer_desc.Ns.count = normal_cnt;
		vertex_buffer_desc.Ns.size = fg0.current_vertex_desc->Ns.size;
		vertex_buffer_desc.Ns.stride = fg0.current_vertex_desc->Ns.stride;
		vertex_buffer_desc.Ns.indices = new U32[index_cnt];
		vertex_buffer_desc.Ns.data = new U8[normal_cnt * fg0.current_vertex_desc->Ns.stride];
	}

	if( mc0_cnt )
	{
		vertex_buffer_desc.MC0s.count = mc0_cnt;
		vertex_buffer_desc.MC0s.size = fg0.current_vertex_desc->MC0s.size;
		vertex_buffer_desc.MC0s.stride = fg0.current_vertex_desc->MC0s.stride;
		vertex_buffer_desc.MC0s.indices = new U32[index_cnt];
		vertex_buffer_desc.MC0s.data = new U8[mc0_cnt * fg0.current_vertex_desc->MC0s.stride];
	}

	if( mc1_cnt )
	{
		vertex_buffer_desc.MC1s.count = mc1_cnt;
		vertex_buffer_desc.MC1s.size = fg0.current_vertex_desc->MC1s.size;
		vertex_buffer_desc.MC1s.stride = fg0.current_vertex_desc->MC1s.stride;
		vertex_buffer_desc.MC1s.indices = new U32[index_cnt];
		vertex_buffer_desc.MC1s.data = new U8[mc1_cnt * fg0.current_vertex_desc->MC1s.stride];
	}

	if( c0_cnt )
	{
		vertex_buffer_desc.C0s.count = c0_cnt;
		vertex_buffer_desc.C0s.size = fg0.current_vertex_desc->C0s.size;
		vertex_buffer_desc.C0s.stride = fg0.current_vertex_desc->C0s.stride;
		vertex_buffer_desc.C0s.indices = new U32[index_cnt];
		vertex_buffer_desc.C0s.data = new U8[c0_cnt * fg0.current_vertex_desc->C0s.stride];
	}

	int index_offset = 0;
	int point_offset = 0;
	int normal_offset = 0;
	int mc0_offset = 0;
	int mc1_offset = 0;
	int c0_offset = 0;
	int gid = 0;
	for( fgi = beg; fgi != end; fgi++, gid++ )
	{
		TriMeshFaceGroup & fg = **fgi;

		// indices
		for(int fi = 0; fi < fg.num_face_indices; fi++)
		{
			fg.face_indices[fi] = fg.face_indices[fi] + index_offset;
		}

		// points
		for(int i = 0; i < fg.current_vertex_desc->num_vertices; i++)
		{
			vertex_buffer_desc.Ps.indices[i + index_offset] = fg.current_vertex_desc->Ps.indices[i] + point_offset;
		}
		for(i = 0; i < fg.current_vertex_desc->Ps.count; i++)
		{
			*(Vector*)((U8*)vertex_buffer_desc.Ps.data + (i + point_offset) * vertex_buffer_desc.Ps.stride) =
				*(Vector*)((U8*)fg.current_vertex_desc->Ps.data + i * fg.current_vertex_desc->Ps.stride);
		}

		// normals
		if( normal_cnt )
		{
			for(int i = 0; i < fg.current_vertex_desc->num_vertices; i++)
			{
				vertex_buffer_desc.Ns.indices[i + index_offset] = fg.current_vertex_desc->Ns.indices[i] + normal_offset;
			}
			for(i = 0; i < fg.current_vertex_desc->Ns.count; i++)
			{
				*(Vector*)((U8*)vertex_buffer_desc.Ns.data + (i + normal_offset) * vertex_buffer_desc.Ns.stride) =
					*(Vector*)((U8*)fg.current_vertex_desc->Ns.data + i * fg.current_vertex_desc->Ns.stride);
			}
		}

		if( mc0_cnt )
		{
			for(int i = 0; i < fg.current_vertex_desc->num_vertices; i++)
			{
				vertex_buffer_desc.MC0s.indices[i + index_offset] = fg.current_vertex_desc->MC0s.indices[i] + mc0_offset;
			}
			for(i = 0; i < fg.current_vertex_desc->MC0s.count; i++)
			{
				memcpy((U8*)vertex_buffer_desc.MC0s.data + (i + mc0_offset) * vertex_buffer_desc.MC0s.stride,
					(U8*)fg.current_vertex_desc->MC0s.data + i * fg.current_vertex_desc->MC0s.stride, vertex_buffer_desc.MC0s.size);
			}
		}

		if( mc1_cnt )
		{
			for(int i = 0; i < fg.current_vertex_desc->num_vertices; i++)
			{
				vertex_buffer_desc.MC1s.indices[i + index_offset] = fg.current_vertex_desc->MC1s.indices[i] + mc1_offset;
			}
			for(i = 0; i < fg.current_vertex_desc->MC1s.count; i++)
			{
				memcpy((U8*)vertex_buffer_desc.MC1s.data + (i + mc1_offset) * vertex_buffer_desc.MC1s.stride,
					(U8*)fg.current_vertex_desc->MC1s.data + i * fg.current_vertex_desc->MC1s.stride, vertex_buffer_desc.MC1s.size);
			}
		}

		if( c0_cnt )
		{
			for(int i = 0; i < fg.current_vertex_desc->num_vertices; i++)
			{
				vertex_buffer_desc.C0s.indices[i + index_offset] = fg.current_vertex_desc->C0s.indices[i] + c0_offset;
			}
			for(i = 0; i < fg.current_vertex_desc->C0s.count; i++)
			{
				memcpy((U8*)vertex_buffer_desc.C0s.data + (i + c0_offset) * vertex_buffer_desc.C0s.stride,
					(U8*)fg.current_vertex_desc->C0s.data + i * fg.current_vertex_desc->C0s.stride, vertex_buffer_desc.C0s.size);
			}
		}

		index_offset += fg.current_vertex_desc->num_vertices;
		point_offset += fg.current_vertex_desc->Ps.count;
		normal_offset += fg.current_vertex_desc->Ns.count;
		mc0_offset += fg.current_vertex_desc->MC0s.count;
		mc1_offset += fg.current_vertex_desc->MC1s.count;
		c0_offset += fg.current_vertex_desc->C0s.count;

		FreeVertexBufferDesc( *fg.current_vertex_desc );
		delete fg.current_vertex_desc;
		fg.current_vertex_desc = &vertex_buffer_desc;
		ASSERT( fg.current_vertex_desc == fg.shared_vertex_desc );

		fg.set_min_max_info();
	}

	return S_OK;
}

//

HRESULT TriMeshArchetype::create_instance( INSTANCE_INDEX inst_index, struct TriMeshInstance **out_instance )
{
	TriMeshInstance *mesh;

	*out_instance = NULL;

	if( (mesh = new TriMeshInstance()) == NULL ) {
		return E_FAIL;
	}

	if( FAILED( mesh->initialize( this, inst_index ) ) ) {
		return E_FAIL;
	}

	*out_instance = mesh;

	return S_OK;
}

//

HRESULT TriMeshArchetype::get_bounding_box( float *out_box )
{
	if( !(tm_f_flags & TM_F_MESH_VALID_BIT) ) {
		return E_FAIL;
	}

	if( tm_f_flags & TM_F_BOX_VALID_BIT ) {
		memcpy( out_box, bounding_box, sizeof(float)*6 );
		return S_OK;
	}

	memset( bounding_box, 0, sizeof(float)*6 );

	if( vertex_buffer_desc.Ps.count ) {

		Vector *P = (Vector *)vertex_buffer_desc.Ps.data;

		for( U32 cnt = 0; cnt < vertex_buffer_desc.Ps.count; cnt++ ) {
		
			if( P->x < bounding_box[BBOX_MIN_X] ) {
				bounding_box[BBOX_MIN_X] = P->x;
			}
			else if( P->x > bounding_box[BBOX_MAX_X] ) {
				bounding_box[BBOX_MAX_X] = P->x;
			}

			if( P->y < bounding_box[BBOX_MIN_Y] ) {
				bounding_box[BBOX_MIN_Y] = P->y;
			}
			else if( P->y > bounding_box[BBOX_MAX_Y] ) {
				bounding_box[BBOX_MAX_Y] = P->y;
			}

			if( P->z < bounding_box[BBOX_MIN_Z] ) {
				bounding_box[BBOX_MIN_Z] = P->z;
			}
			else if( P->z > bounding_box[BBOX_MAX_Z] ) {
				bounding_box[BBOX_MAX_Z] = P->z;
			}
		}

		tm_f_flags |= TM_F_BOX_VALID_BIT;
	}

	memcpy( out_box, bounding_box, sizeof(float)*6 );

	return S_OK;
}

//

HRESULT TriMeshArchetype::get_bounding_sphere( Vector *out_center, float *out_radius )
{
	if( !(tm_f_flags & TM_F_MESH_VALID_BIT) ) {
		return E_FAIL;
	}

	if( tm_f_flags & TM_F_SPHERE_VALID_BIT ) {
		*out_center = bounding_sphere_center;
		*out_radius = bounding_sphere_radius;
		return S_OK;
	}

	bounding_sphere_center.zero();
	bounding_sphere_radius = 0.0f;

	if( vertex_buffer_desc.Ps.count ) {

		const Vector *P;
		const Vector *Ps;
		int i, mini, maxi;
		int min_xi = 0, max_xi = 0;
		int min_yi = 0, max_yi = 0;
		int min_zi = 0, max_zi = 0;

		Ps = (Vector*)vertex_buffer_desc.Ps.data;

		// initial guess
		//
		for( P=Ps, i=0; i < vertex_buffer_desc.Ps.count; i++, P++ ) {

			if( P->x < bounding_box[BBOX_MIN_X] ) {
				min_xi = i;
			}
			else if( P->x > bounding_box[BBOX_MAX_X] ) {
				max_xi = i;
			}

			if( P->y < bounding_box[BBOX_MIN_Y] ) {
				min_yi = i;
			}
			else if( P->y > bounding_box[BBOX_MAX_Y] ) {
				max_yi = i;
			}

			if( P->z < bounding_box[BBOX_MIN_Z] ) {
				min_zi = i;
			}
			else if( P->z > bounding_box[BBOX_MAX_Z] ) {
				max_zi = i;
			}

			float dx = Ps[max_xi].x - Ps[min_xi].x;
			float dy = Ps[max_yi].y - Ps[min_yi].y;
			float dz = Ps[max_zi].z - Ps[min_zi].z;

			if( dx >= dy && dx >= dz) {
				mini = min_xi;
				maxi = max_xi;
			}
			else if( dy >= dx && dy >= dz) {
				mini = min_yi;
				maxi = max_yi;
			}
			else {
				mini = min_zi;
				maxi = max_zi;
			}
		}

		Vector delta( Ps[maxi] - Ps[mini] );
		bounding_sphere_center = Ps[mini] + 0.5f * delta;
		bounding_sphere_radius = 0.5f * delta.magnitude();
		float rad_sq = bounding_sphere_radius * bounding_sphere_radius;

		// refine guess
		//
		for( P=Ps, i=0; i<vertex_buffer_desc.Ps.count; i++, P++ ) {
	
			delta = *P - bounding_sphere_center;

			float old_sq = dot_product( delta, delta );
	
			if( old_sq > rad_sq ) {
				//
				// Point is outside current bounding sphere, need to update.
				//

				// save last sphere
				Vector min_center = bounding_sphere_center;
				float min_rad = bounding_sphere_radius;

				// update
				float old = (float) sqrt( old_sq );
				bounding_sphere_radius = 0.5f * (bounding_sphere_radius + old);
				rad_sq = bounding_sphere_radius * bounding_sphere_radius;

				float offset = old - bounding_sphere_radius;
				bounding_sphere_center = (bounding_sphere_radius * bounding_sphere_center + offset * *P) / old;
			}
		}

		tm_f_flags |= TM_F_SPHERE_VALID_BIT;
	}

	*out_center = bounding_sphere_center;
	*out_radius = bounding_sphere_radius;

	return S_OK;
}

//

HRESULT TriMeshArchetype::get_centroid( Vector *out_centroid )
{
	if( !(tm_f_flags & TM_F_MESH_VALID_BIT) ) {
		return E_FAIL;
	}

	if( tm_f_flags & TM_F_SPHERE_VALID_BIT ) {
		*out_centroid = centroid;
		return S_OK;
	}

	centroid.zero();

	if( vertex_buffer_desc.Ps.count ) {

		Vector *P = (Vector*)vertex_buffer_desc.Ps.data;

		for( U32 cnt = 0; cnt < vertex_buffer_desc.Ps.count; cnt++ ) {
			centroid += *P;
		}

		centroid /= vertex_buffer_desc.Ps.count;

		tm_f_flags |= TM_F_CENTROID_VALID_BIT;
	}

	*out_centroid = centroid;

	return S_OK;
}

//

HRESULT TriMeshArchetype::lock_indices( IM_ELEMENT element_mask )
{
	VertexBufferDesc & vbd = *((*locked_facegroup)->current_vertex_desc);

	U32 **indices = NULL;
	U16 **locked_indices = NULL;

	switch( element_mask )
	{
		case IM_E_POINTS:
			indices = &(vbd.Ps.indices);
			locked_indices = &locked_point_indices;
			break;
		case IM_E_NORMALS:
			indices = &(vbd.Ns.indices);
			locked_indices = &locked_normal_indices;
			break;
		case IM_E_UV0S:
			indices = &(vbd.MC0s.indices);
			locked_indices = &locked_uv0_indices;
			break;
		case IM_E_UV1S:
			indices = &(vbd.MC1s.indices);
			locked_indices = &locked_uv1_indices;
			break;
		case IM_E_COLOR0S:
			indices = &(vbd.C0s.indices);
			locked_indices = &locked_color0_indices;
			break;
	}

	if( *indices )
	{
		*locked_indices = new U16[(*locked_facegroup)->num_face_indices];

		U16 *li = *locked_indices;
		U16 *fi = (*locked_facegroup)->face_indices;
		U32 min = (*locked_facegroup)->min_vertex_index;
		
		for( U32 i=0; i<(*locked_facegroup)->num_face_indices; i++, li++ )
		{
			*li = (*indices)[ fi[i] + min ];
		}
		
		return S_OK;
	}
	else
	{
		return E_FAIL;
	}
}

//

HRESULT TriMeshArchetype::unlock_indices( IM_ELEMENT element_mask )
{
	VertexBufferDesc & vbd = *((*locked_facegroup)->current_vertex_desc);

	U32 **indices = NULL;
	U16 **locked_indices = NULL;

	switch( element_mask )
	{
		case IM_E_POINTS:
			indices = &(vbd.Ps.indices);
			locked_indices = &locked_point_indices;
			break;
		case IM_E_NORMALS:
			indices = &(vbd.Ns.indices);
			locked_indices = &locked_normal_indices;
			break;
		case IM_E_UV0S:
			indices = &(vbd.MC0s.indices);
			locked_indices = &locked_uv0_indices;
			break;
		case IM_E_UV1S:
			indices = &(vbd.MC1s.indices);
			locked_indices = &locked_uv1_indices;
			break;
		case IM_E_COLOR0S:
			indices = &(vbd.C0s.indices);
			locked_indices = &locked_color0_indices;
			break;
	}

	if( !locked_read_only )
	{
		ASSERT( *indices != NULL );
		
		U16 *li = *locked_indices;
		U16 *fi = (*locked_facegroup)->face_indices;
		U32 min = (*locked_facegroup)->min_vertex_index;

		for( U32 i=0; i<(*locked_facegroup)->num_face_indices; i++, li++ )
		{
			(*indices)[ fi[i] + min ] = *li;
		}
	}

	delete[] (*locked_indices);
	*locked_indices = NULL;
	return S_OK;
}

//

HRESULT TriMeshArchetype::lock_edge_angle_indices( void )
{
	if( (*locked_facegroup)->edge_angles )
	{
		// generate parallel indices
		int edge_angle_index_count = (*locked_facegroup)->num_edge_indices / 2;
		locked_edge_angle_indices = new U16[edge_angle_index_count];
		for(int i = 0; i < edge_angle_index_count; i++)
		{
			locked_edge_angle_indices[i] = i;
		}

		return S_OK;
	}
	else
	{
		return E_FAIL;
	}
}

//

HRESULT TriMeshArchetype::unlock_edge_angle_indices( void )
{
	if( !locked_read_only )
	{
		//TODO: update
	}

	delete[] locked_edge_angle_indices;
	locked_edge_angle_indices = NULL;
	return S_OK;
}

//

U32 TriMeshArchetype::get_num_mesh_faces( void )
{
	int fc = 0;

	FaceGroupList::const_iterator beg = facegroups.begin();
	FaceGroupList::const_iterator end = facegroups.end();
	FaceGroupList::const_iterator fg;

	for( fg = beg; fg != end; fg++ ) {
		fc += (*fg)->num_face_indices;
	}
	
	fc /= 3;

	return fc;
}

//

U32 TriMeshArchetype::get_num_mesh_vertices( void )
{
	FaceGroupList::const_iterator beg = facegroups.begin();
	FaceGroupList::const_iterator end = facegroups.end();
	FaceGroupList::const_iterator fgi;

	bool counted_shared = false;
	U32 count = 0;
	for( fgi = beg; fgi != end; fgi++ )
	{
		if( (*fgi)->current_vertex_desc != (*fgi)->shared_vertex_desc )
		{
			count += (*fgi)->current_vertex_desc->num_vertices;
		}
		else if( !counted_shared )
		{
			count += (*fgi)->shared_vertex_desc->num_vertices;
			counted_shared = true;
		}
	}

	return count;
}

//

GENRESULT TriMeshArchetype::QueryInterface( const C8 *IID, void **iif ) 
{
	if( (strcmp( IID, IID_IMesh ) == 0) || (strcmp( IID, "IMesh") == 0) ) {
		*iif = static_cast<IMesh*>( this );
		return GR_OK;
	}
	else if( strcmp( IID, "IDAComponent" ) == 0 ) {
		*iif = static_cast<IMesh*>( this );
		return GR_OK;
	}

	return GR_GENERIC;
}

//

U32 TriMeshArchetype::AddRef( void ) 
{
	return 1;	// artificially ref counted.  
}

//

U32 TriMeshArchetype::Release( void ) 
{
	return 1;
}

//

HRESULT TriMeshArchetype::set_lod( float new_fraction )
{
	if( lod != NULL )
	{
		// guard against invalid range
		if(new_fraction < 0.0f)
		{
			new_fraction = 0.0f;
		}
		else
		if(new_fraction > 1.0f)
		{
			new_fraction = 1.0f;
		}

		if(fabs(new_fraction - fraction) > .25f && fabs(fraction - fraction_previous) > .25f)
		{
			GENERAL_TRACE_1("Thrashing cont. LOD\n");
		}

		// don't update if change is infenticimal
		if( (fabs(new_fraction - fraction) > threshold) || 
			((new_fraction == 1.0f || new_fraction == 0.0f) && new_fraction != fraction) )
		{
			const int new_step =  Tmax(0, (int)ceil(new_fraction * lod->step_cnt) - 1);
			const int last_step = Tmax(0, (int)ceil(fraction * lod->step_cnt) - 1);

			const float sub_fraction = (new_fraction * lod->step_cnt) - new_step;

			// special case for increasing from minimum
			if(fraction == 0.0f && new_fraction > 0.0f)
			{
				add_lod_step(0);
				set_uv_indices(0);
			}

			if(new_step == last_step)  // interpolation only
			{
				interpolate_lod_step(new_step, sub_fraction);
			}
			else
			// avoiding VC++ 6.0 bug //PCI, ms
			if(new_step <= last_step)  // collapse some edges
			{
				for(int step_id = last_step; step_id > new_step; step_id--)
				{
					remove_lod_step(step_id);	
				}

				set_uv_indices(new_step);
				interpolate_lod_step(new_step, sub_fraction);
			}
			else  //if(new_step > last_step) // split some vertices
			{
				interpolate_lod_step(last_step, 1.0f);
				restore_uv_indices(last_step);

				for(int step_id = last_step + 1; step_id <= new_step; step_id++)
				{
					add_lod_step(step_id);
				}

				set_uv_indices(new_step);
				interpolate_lod_step(new_step, sub_fraction);
			}

			// special case for going to minimum
			if(new_fraction == 0.0f && fraction > 0.0f)
			{
				remove_lod_step(0);
			}

			fraction_previous = fraction;
			fraction = new_fraction;
		}
	}

	return S_OK;
}

//

void TriMeshArchetype::remove_lod_step(const int step_id)
{
	ASSERT(lod);
	ASSERT(step_id < lod->step_cnt);

	const TriLODStep & current_los = lod->step_list[step_id];

	// remove FACES
	ASSERT(current_los.first_removed_face + current_los.removed_face_count <= lod->removed_face_cnt);
	for(int rf_id = current_los.first_removed_face;
			rf_id < current_los.first_removed_face + current_los.removed_face_count;
			rf_id++)
	{
		facegroups[ lod->removed_face_list[rf_id] ]->num_face_indices -= 3;
	}

	// UV's
	for(int i = current_los.first_morphed_uv; 
			i < current_los.first_morphed_uv + current_los.morphed_uv_count;
			i++)
	{
		for(int j = lod->uv_batch_first1[i];
				j < lod->uv_batch_first1[i] + lod->uv_batch_cnt1[i]; j++)
		{
			vertex_buffer_desc.MC0s.indices[ lod->uv_batch_chain[j] ] = lod->uv_low_id[i];
		}

		for(	j = lod->uv_batch_first2[i];
				j < lod->uv_batch_first2[i] + lod->uv_batch_cnt2[i]; j++)
		{
			vertex_buffer_desc.MC0s.indices[ lod->uv_batch_chain[j] ] = lod->uv_low_id[i];
		}
	}
	
	// remove VERTICES
	vertex_buffer_desc.num_vertices -= current_los.removed_batch_vertex_count;
	
	((Vector*)vertex_buffer_desc.Ps.data)[current_los.vid1] = current_los.v3;
	
	vertex_buffer_desc.Ps.count--;
	ASSERT(current_los.vid2 == vertex_buffer_desc.Ps.count);

	// morph VERTICES
	ASSERT(current_los.first_vertex + current_los.morphed_vertex_chain_count <= lod->vertex_cnt);
	for(int mv_id = current_los.first_vertex;
			mv_id < current_los.first_vertex + current_los.morphed_vertex_chain_count;
			mv_id++)
	{
		int v_id = lod->vertex_list[mv_id];

		ASSERT(v_id < vertex_buffer_desc.num_vertices);

		ASSERT(vertex_buffer_desc.Ps.indices[v_id] == current_los.vid2);
		vertex_buffer_desc.Ps.indices[v_id] = current_los.vid1;
	}
}

//

void TriMeshArchetype::add_lod_step(const int step_id)
{
	ASSERT(lod);
	ASSERT(step_id < lod->step_cnt);

	const TriLODStep & current_los = lod->step_list[step_id];

	// add FACES
	ASSERT(current_los.first_removed_face + current_los.removed_face_count <= lod->removed_face_cnt);
	for(int rf_id = current_los.first_removed_face;
			rf_id < current_los.first_removed_face + current_los.removed_face_count;
			rf_id++)
	{
		facegroups[ lod->removed_face_list[rf_id] ]->num_face_indices += 3;
	}

	// add VERTICES
	vertex_buffer_desc.num_vertices += current_los.removed_batch_vertex_count;

	((Vector*)vertex_buffer_desc.Ps.data)[current_los.vid1] = current_los.v1;
	((Vector*)vertex_buffer_desc.Ps.data)[current_los.vid2] = current_los.v2;

	// morph VERTICES
	ASSERT(current_los.first_vertex + current_los.morphed_vertex_chain_count <= lod->vertex_cnt);
	for(int mv_id = current_los.first_vertex;
			mv_id < current_los.first_vertex + current_los.morphed_vertex_chain_count;
			mv_id++)
	{
		int v_id = lod->vertex_list[mv_id];

		ASSERT(v_id < vertex_buffer_desc.num_vertices);

		ASSERT(vertex_buffer_desc.Ps.indices[v_id] == current_los.vid1);
		vertex_buffer_desc.Ps.indices[v_id] = current_los.vid2;
	}

	ASSERT(current_los.vid2 == vertex_buffer_desc.Ps.count);
	vertex_buffer_desc.Ps.count++;

	// UV's
	for(int i = current_los.first_morphed_uv; 
			i < current_los.first_morphed_uv + current_los.morphed_uv_count;
			i++)
	{
		for(int j = lod->uv_batch_first1[i];
				j < lod->uv_batch_first1[i] + lod->uv_batch_cnt1[i];
				j++)
		{
			vertex_buffer_desc.MC0s.indices[ lod->uv_batch_chain[j] ] = lod->uv_high_id1[i];
		}

		for(	j = lod->uv_batch_first2[i];
				j < lod->uv_batch_first2[i] + lod->uv_batch_cnt2[i];
				j++)
		{
			vertex_buffer_desc.MC0s.indices[ lod->uv_batch_chain[j] ] = lod->uv_high_id2[i];
		}
	}
}

//

void TriMeshArchetype::interpolate_lod_step(const int step_id, const float fraction)
{
	ASSERT(lod);
	ASSERT(step_id < lod->step_cnt);

	const float one_minus_fraction = 1.0f - fraction;

	const TriLODStep & new_los = lod->step_list[step_id];

	((Vector*)vertex_buffer_desc.Ps.data)[new_los.vid1] =
		fraction * new_los.v1 +
		one_minus_fraction * new_los.v3;

	((Vector*)vertex_buffer_desc.Ps.data)[new_los.vid2] =
		fraction * new_los.v2 +
		one_minus_fraction * new_los.v3;

	// UV's
	for(int i = 0, j = lod->step_list[step_id].first_morphed_uv;
			i < 2 * lod->step_list[step_id].morphed_uv_count;
			i+=2, j++)
	{
		TexCoord *texture_vertex_list = (TexCoord*)(vertex_buffer_desc.MC0s.data);

		const TexCoord tmp ( one_minus_fraction * texture_vertex_list[ lod->uv_low_id[j] ] );

		texture_vertex_list[i] = 
			tmp + fraction * texture_vertex_list[ lod->uv_high_id1[j] ];
		texture_vertex_list[i + 1] = 
			tmp + fraction * texture_vertex_list[ lod->uv_high_id2[j] ];
	}
}

//

void TriMeshArchetype::set_uv_indices(const int step_id)
{
	const TriLODStep & current_los = lod->step_list[step_id];
	for(int i = 0, j = current_los.first_morphed_uv;
			i < 2 * current_los.morphed_uv_count;
			i+=2, j++)
	{
		for(int k = lod->uv_batch_first1[j];
				k < lod->uv_batch_first1[j] + lod->uv_batch_cnt1[j];
				k++)
		{
			vertex_buffer_desc.MC0s.indices[ lod->uv_batch_chain[k] ] = i;
		}

		for(	k = lod->uv_batch_first2[j];
				k < lod->uv_batch_first2[j] + lod->uv_batch_cnt2[j];
				k++)
		{
			vertex_buffer_desc.MC0s.indices[ lod->uv_batch_chain[k] ] = i + 1;
		}
	}
}

//

void TriMeshArchetype::restore_uv_indices(const int step_id)
{
	// reset uv indices back from interpolation
	const TriLODStep & current_los = lod->step_list[step_id];
	for(int i = current_los.first_morphed_uv; 
			i < current_los.first_morphed_uv + current_los.morphed_uv_count;
			i++)
	{
		for(int j = lod->uv_batch_first1[i];
				j < lod->uv_batch_first1[i] + lod->uv_batch_cnt1[i];
				j++)
		{
			vertex_buffer_desc.MC0s.indices[ lod->uv_batch_chain[j] ] = lod->uv_high_id1[i];
		}

		for(	j = lod->uv_batch_first2[i];
				j < lod->uv_batch_first2[i] + lod->uv_batch_cnt2[i];
				j++)
		{
			vertex_buffer_desc.MC0s.indices[ lod->uv_batch_chain[j] ] = lod->uv_high_id2[i];
		}
	}
}
