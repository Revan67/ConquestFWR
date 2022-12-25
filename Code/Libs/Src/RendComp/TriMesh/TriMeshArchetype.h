// TriMeshArchetype.h
//
//
//

#ifndef __TriMeshArchetype_h__
#define __TriMeshArchetype_h__

#pragma warning( disable : 4530 )

//

#include <windows.h>
#include <vector>

//

#include "dacom.h"
#include "3dmath.h"
#include "IVertexBufferManager.h"
#include "rendpipeline.h"
#include "IMaterialLibrary.h"
#include "IMesh.h"
#include "TriMeshLOD.h"

//

#include "TriMeshFaceGroup.h"

//

#define TM_F_MESH_VALID_BIT		(1<<0)
#define TM_F_SPHERE_VALID_BIT	(1<<1)
#define TM_F_BOX_VALID_BIT		(1<<2)
#define TM_F_CENTROID_VALID_BIT	(1<<3)
#define TM_F_VBUFFER_VALID_BIT	(1<<4)

//

//

struct TriMeshArchetype : public IMesh
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

	// internal methods

	HRESULT cleanup( void );

	HRESULT load( IFileSystem *ifs, IRenderPipeline *renderpipeline, IVertexBufferManager *vbuffermanager, IMaterialLibrary *material_library );
	HRESULT clone( TriMeshArchetype **out_archetype  );
	HRESULT split( const Vector& plane_normal, float plane_d, U32 split_flags, TriMeshArchetype **out_archetype_0, TriMeshArchetype **out_archetype_1 ) const;
	HRESULT optimize_data_sharing( void );

	HRESULT create_instance( INSTANCE_INDEX inst_index, struct TriMeshInstance **out_instance );

	HRESULT get_bounding_box( float *out_box );
	HRESULT get_bounding_sphere( Vector *out_center, float *out_radius );
	HRESULT get_centroid( Vector *out_centroid );

	HRESULT lock_indices( IM_ELEMENT element_mask );
	HRESULT unlock_indices( IM_ELEMENT element_mask );
	HRESULT lock_edge_angle_indices( void );
	HRESULT unlock_edge_angle_indices( void );

	U32 get_num_mesh_faces( void );
	U32 get_num_mesh_vertices( void );
	
	HRESULT set_lod( float new_fraction );
	
	TriMeshArchetype();
	~TriMeshArchetype();

public:	// Data

	FaceGroupList facegroups;
	VertexBufferDesc vertex_buffer_desc;	// can be empty if each group uses it's own buffer
	
	// static LOD data
	TriLODLib *lod;

	// dynamic LOD data
	float	fraction;				// amount of current detail 0 to 1
	float	fraction_previous;		// last detail (used to detect thrashing)
	float	threshold;              // minimal change before any work is done

protected: // Interface

	// these should never be called directly (only by set_lod())
	void remove_lod_step(const int step_id);
	void add_lod_step(const int step_id);
	void interpolate_lod_step(const int step_id, const float fraction);
	void set_uv_indices(const int step_id);
	void restore_uv_indices(const int step_id);

protected:	// Data

	U32 tm_f_flags;

	char path[MAX_PATH];

	Vector centroid;
	float  bounding_box[6];
	Vector bounding_sphere_center;
	float  bounding_sphere_radius;

	FaceGroupList::iterator locked_facegroup;
	IM_ELEMENT				locked_index_elements;
	U16					   *locked_point_indices;
	U16					   *locked_normal_indices;
	U16					   *locked_uv0_indices;
	U16					   *locked_uv1_indices;
	U16					   *locked_color0_indices;
	U16					   *locked_edge_angle_indices;
	bool					locked_read_only;		// indicates whether the locked group is locked read only
};

//

#endif // EOF

