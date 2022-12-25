// TriMeshInstance.cpp
//
//
//

//

#include "DACOM.h"
#include "FDUMP.h"
#include "engine.h"
#include "renderer.h"
#include "icamera.h"
#include "IRenderPrimitive.h"
#include "IVertexBufferManager.h"

//

#include "Tfuncs.h"

//

#include "TriMeshInstance.h"

//

TriMeshInstance::TriMeshInstance()
{
	current_archetype = NULL;
	loaded_archetype = NULL;
	inst_index = INVALID_INSTANCE_INDEX;
}

//

TriMeshInstance::~TriMeshInstance()
{
	cleanup();
}

//

HRESULT TriMeshInstance::initialize( TriMeshArchetype *_loaded_archetype, INSTANCE_INDEX _inst_index )
{
	current_archetype = _loaded_archetype;
	loaded_archetype = _loaded_archetype;
	inst_index = _inst_index;

	return S_OK;
}

//

HRESULT TriMeshInstance::cleanup( void )
{
	current_archetype = NULL;
	loaded_archetype = NULL;
	inst_index = INVALID_INSTANCE_INDEX;

	return S_OK;
}

//

HRESULT TriMeshInstance::update( float dt )
{
	FaceGroupList::iterator beg = current_archetype->facegroups.begin();
	FaceGroupList::iterator end = current_archetype->facegroups.end();
	FaceGroupList::iterator fg;

	for( fg=beg; fg!=end; fg++ ) {
		
		if( (*fg)->material ) {
			(*fg)->material->update( dt );
		}
	}

	return S_OK;
}

//

HRESULT TriMeshInstance::render( IRenderPipeline *renderpipeline, IVertexBufferManager *vbuffermanager, IRenderPrimitive *renderprim, IEngine *engine, ICamera *camera, float lod_fraction, U32 rf_flags, const Transform *transform, vis_state *out_visstate )
{
	if( current_archetype == NULL || inst_index == INVALID_INSTANCE_INDEX ) {
		*out_visstate = VS_UNKNOWN;
		return E_FAIL;
	}

	current_archetype->set_lod( lod_fraction );

	// Determine transforms
	//
	const Transform &world_to_view = camera->get_inverse_transform();
	const Transform &object_to_world = engine->get_transform( inst_index );
	Transform object_to_view( false );

	if( transform != NULL ) {
		object_to_view = world_to_view * (*transform * object_to_world);
	}
	else {
		object_to_view = world_to_view * object_to_world;
	}

	renderprim->set_modelview( object_to_view ) ;

	// Setup facegroup invariant vertex buffer
	//
	VertexBufferDesc *vbd = &current_archetype->vertex_buffer_desc;
	IRP_VERTEXBUFFERHANDLE vb;
	void *vbmem;
	U32 vertex_format;
	U32 num_verts;

	if( SUCCEEDED( vbuffermanager->acquire_vertex_buffer( vbd->vertex_format, 
														  vbd->num_vertices, 
														  0, 
														  DDLOCK_WAIT|DDLOCK_WRITEONLY,
														  0,
														  &vb,
														  &vbmem,
														  &vertex_format,
														  &num_verts ) ) ) {
		
		vbuffermanager->copy_vertex_data( vbmem, vertex_format, vbd );
		
		renderpipeline->unlock_vertex_buffer( vb );

	}
	else {
		__asm int 3;
	}
	

	// Setup and render facegroups
	//
	FaceGroupList::iterator beg = current_archetype->facegroups.begin();
	FaceGroupList::iterator end = current_archetype->facegroups.end();
	FaceGroupList::iterator fg;
	TriMeshFaceGroup *fgp;
	MaterialContext mc;

	mc.inst_index = inst_index;
	mc.object_to_view = &object_to_view;
	mc.object_to_world = &object_to_world;
	//mc.vertex_buffer = vb;
	mc.world_to_view = &world_to_view;

	for( fg=beg; fg!=end; fg++ ) {
		
		fgp = (*fg);

		if( fgp->current_vertex_desc == fgp->shared_vertex_desc )
		{
			mc.vertex_buffer = vb;
		}
		else
		{
			mc.vertex_buffer = IRP_INVALID_VB_HANDLE;
		}

		if( fgp->num_face_indices > 0 && fgp->material ) {
			
			fgp->material->apply();

			mc.extended_context = fgp->extended_context;
		
			fgp->material->render( &mc,
								   fgp->primitive_type, 
								   fgp->current_vertex_desc,
								   fgp->min_vertex_index, 
								   (fgp->max_vertex_index-fgp->min_vertex_index)+1, 
								   fgp->face_indices, 
								   fgp->num_face_indices, 
								   0 );
		}
		
	}


	vbuffermanager->release_vertex_buffer( vb );

	return S_OK;
}

//

GENRESULT TriMeshInstance::initialize( ) 
{
	ASSERT( current_archetype );
	return current_archetype->initialize();
}

//

GENRESULT TriMeshInstance::verify( void ) 
{
	ASSERT( current_archetype );
	return current_archetype->verify();
}

//

GENRESULT TriMeshInstance::optimize( IM_OPTIMIZE_FLAG_BIT op_mask ) 
{
	ASSERT( current_archetype );
	return current_archetype->optimize( op_mask );
}

//

GENRESULT TriMeshInstance::acquire_unique( void ) 
{
	ASSERT( current_archetype );

	if( current_archetype == loaded_archetype )
	{
		loaded_archetype->clone( &current_archetype );
	}

	return GR_OK;
}

//

GENRESULT TriMeshInstance::release_unique( void ) 
{
	ASSERT( current_archetype );

	if( current_archetype != loaded_archetype )
	{
		delete current_archetype;
		current_archetype = loaded_archetype;
		return GR_OK;
	}
	else
	{
		return GR_GENERIC;
	}
}

//

GENRESULT TriMeshInstance::get_num_facegroups( U32 *out_num_facegroups ) 
{
	ASSERT( current_archetype );
	return current_archetype->get_num_facegroups( out_num_facegroups );
}

//

GENRESULT TriMeshInstance::lock_facegroup( U32 facegroup_num, bool read_only ) 
{
	ASSERT( current_archetype );
	return current_archetype->lock_facegroup( facegroup_num, read_only );
}

//

GENRESULT TriMeshInstance::unlock_facegroup( void ) 
{
	ASSERT( current_archetype );
	return current_archetype->unlock_facegroup();
}

//

GENRESULT TriMeshInstance::add_facegroup( D3DPRIMITIVETYPE primitive_type ) 
{
	ASSERT( current_archetype );
	return current_archetype->add_facegroup( primitive_type );
}

//

GENRESULT TriMeshInstance::remove_facegroup( U32 facegroup_num ) 
{
	ASSERT( current_archetype );
	return current_archetype->remove_facegroup( facegroup_num );
}

//

GENRESULT TriMeshInstance::get_facegroup_primitive_type( D3DPRIMITIVETYPE *out_primitive_type ) 
{
	ASSERT( current_archetype );
	return current_archetype->get_facegroup_primitive_type( out_primitive_type );
}

//

GENRESULT TriMeshInstance::get_facegroup_material( IMaterial **out_material ) 
{
	ASSERT( current_archetype );
	return current_archetype->get_facegroup_material( out_material );
}

//

GENRESULT TriMeshInstance::set_facegroup_material( IMaterial *material ) 
{
	ASSERT( current_archetype );
	return current_archetype->set_facegroup_material( material );
}

//

GENRESULT TriMeshInstance::get_facegroup_material_context( void **out_context ) 
{
	ASSERT( current_archetype );
	return current_archetype->get_facegroup_material_context( out_context );
}

//

GENRESULT TriMeshInstance::set_facegroup_material_context( void *context ) 
{
	ASSERT( current_archetype );
	return current_archetype->set_facegroup_material_context( context );
}

//

GENRESULT TriMeshInstance::lock_facegroup_data( IM_ELEMENT element_mask ) 
{
	ASSERT( current_archetype );
	return current_archetype->lock_facegroup_data( element_mask );
}

//

GENRESULT TriMeshInstance::unlock_facegroup_data( void ) 
{
	ASSERT( current_archetype );
	return current_archetype->unlock_facegroup_data();
}

//

GENRESULT TriMeshInstance::lock_facegroup_indices( IM_ELEMENT element_mask ) 
{
	ASSERT( current_archetype );
	return current_archetype->lock_facegroup_indices( element_mask );
}

//

GENRESULT TriMeshInstance::unlock_facegroup_indices( void ) 
{
	ASSERT( current_archetype );
	return current_archetype->unlock_facegroup_indices();
}

//

GENRESULT TriMeshInstance::get_facegroup_data( IM_ELEMENT element_bit, void const **out_data, U32 *out_stride, U32 *out_size, U32 *out_count ) 
{
	ASSERT( current_archetype );
	return current_archetype->get_facegroup_data( element_bit, out_data, out_stride, out_size, out_count );
}

//

GENRESULT TriMeshInstance::set_facegroup_data( IM_ELEMENT element_bit, const void *data, U32 stride, U32 size, U32 count ) 
{
	ASSERT( current_archetype );
	return current_archetype->set_facegroup_data( element_bit, data, stride, size, count );
}

//

GENRESULT TriMeshInstance::get_facegroup_indices( IM_ELEMENT element_bit, U16 const **out_indices, U32 *out_count ) 
{
	ASSERT( current_archetype );
	return current_archetype->get_facegroup_indices( element_bit, out_indices, out_count );
}

//

GENRESULT TriMeshInstance::set_facegroup_indices( IM_ELEMENT element_bit, const U16 *indices, U32 count ) 
{
	ASSERT( current_archetype );
	return current_archetype->set_facegroup_indices( element_bit, indices, count );
}

//

GENRESULT TriMeshInstance::get_facegroup_indices_count( U32 *out_num_indices ) 
{
	ASSERT( current_archetype );
	return current_archetype->get_facegroup_indices_count( out_num_indices );
}

//

GENRESULT TriMeshInstance::set_facegroup_indices_count( U32 num_indices ) 
{
	ASSERT( current_archetype );
	return current_archetype->set_facegroup_indices_count( num_indices );
}

//

GENRESULT TriMeshInstance::add_elements( IM_ELEMENT element_bits )
{
	ASSERT( current_archetype );
	return current_archetype->add_elements( element_bits );
}

//

GENRESULT TriMeshInstance::remove_elements( IM_ELEMENT element_bits )
{
	ASSERT( current_archetype );
	return current_archetype->remove_elements( element_bits );
}

//

GENRESULT TriMeshInstance::QueryInterface( const C8 *IID, void **iif ) 
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

U32 TriMeshInstance::AddRef( void ) 
{
	return 1;	// artificially ref counted.  
}

//

U32 TriMeshInstance::Release( void ) 
{
	return 1;
}

//
