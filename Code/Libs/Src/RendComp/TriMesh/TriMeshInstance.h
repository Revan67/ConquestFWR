// TriMeshInstance.h
//
//
//

#ifndef __TriMeshInstance_h__
#define __TriMeshInstance_h__

//

#include "IVertexBufferManager.h"
#include "engine.h"

//

#include "TriMeshArchetype.h"

//

struct TriMeshInstance : IMesh
{
public: // Interface

	// IDAComponent
	GENRESULT COMAPI QueryInterface( const C8 *IID, void **iif ) ;
	U32 COMAPI AddRef( void ) ;
	U32 COMAPI Release( void ) ;

	// IMesh
	GENRESULT COMAPI initialize( ) ;
	GENRESULT COMAPI verify( void ) ;
	GENRESULT COMAPI optimize( IM_OPTIMIZE_FLAG_BIT op_mask ) ;
	GENRESULT COMAPI acquire_unique( void ) ;
	GENRESULT COMAPI release_unique( void ) ;
	GENRESULT COMAPI get_num_facegroups( U32 *out_num_facegroups ) ;
	GENRESULT COMAPI lock_facegroup( U32 facegroup_num, bool read_only ) ;
	GENRESULT COMAPI unlock_facegroup( void ) ;
	GENRESULT COMAPI add_facegroup( D3DPRIMITIVETYPE primitive_type ) ;
	GENRESULT COMAPI remove_facegroup( U32 facegroup_num ) ;
	GENRESULT COMAPI get_facegroup_primitive_type( D3DPRIMITIVETYPE *out_primitive_type ) ;
	GENRESULT COMAPI get_facegroup_material( IMaterial **out_material ) ;
	GENRESULT COMAPI set_facegroup_material( IMaterial *material ) ;
	GENRESULT COMAPI get_facegroup_material_context( void **out_context ) ;
	GENRESULT COMAPI set_facegroup_material_context( void *context ) ;
	GENRESULT COMAPI lock_facegroup_data( IM_ELEMENT element_mask ) ;
	GENRESULT COMAPI unlock_facegroup_data( void ) ;
	GENRESULT COMAPI lock_facegroup_indices( IM_ELEMENT element_mask ) ;
	GENRESULT COMAPI unlock_facegroup_indices( void ) ;
	GENRESULT COMAPI get_facegroup_data( IM_ELEMENT element_bit, void const **out_data, U32 *out_stride, U32 *out_size, U32 *out_count ) ;
	GENRESULT COMAPI set_facegroup_data( IM_ELEMENT element_bit, const void *data, U32 stride, U32 size, U32 count ) ;
	GENRESULT COMAPI get_facegroup_indices( IM_ELEMENT element_bit, U16 const **out_indices, U32 *out_count ) ;
	GENRESULT COMAPI set_facegroup_indices( IM_ELEMENT element_bit, const U16 *indices, U32 count ) ;
	GENRESULT COMAPI get_facegroup_indices_count( U32 *out_num_indicess ) ;
	GENRESULT COMAPI set_facegroup_indices_count( U32 num_indicess ) ;
	GENRESULT COMAPI add_elements( IM_ELEMENT element_bits ) ;
	GENRESULT COMAPI remove_elements( IM_ELEMENT element_bits ) ;

	//

	HRESULT initialize( TriMeshArchetype *loaded_archetype, INSTANCE_INDEX inst_index );
	HRESULT cleanup( void );

	HRESULT update( float dt );
	HRESULT render( IRenderPipeline *renderpipeline, IVertexBufferManager *vbuffermanager, IRenderPrimitive *renderprim, IEngine *engine, ICamera *camera, float lod_fraction, U32 rf_flags, const Transform *transform, vis_state *out_visstate );

	TriMeshInstance();
	~TriMeshInstance();

public: // Data

	TriMeshArchetype *loaded_archetype;
	TriMeshArchetype *current_archetype;	// may or may not be the same as loaded_archetype

	INSTANCE_INDEX inst_index;

	// TODO: Animated values (UVs, etc...)
};

//

#endif // EOF

