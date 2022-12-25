// 
// TriMeshFaceGroup
//
//

#ifndef __TriMeshFaceGroup_h__
#define __TriMeshFaceGroup_h__

//
#pragma warning( disable : 4530 )
#include <vector>

//

#include "da_d3dtypes.h"
#include "IMaterial.h"
#include "IMesh.h"
#include "VertexBufferDesc.h"

//

typedef U32 FaceStatus; // used for splitting
const FaceStatus PS_FRONT =		0x001;
const FaceStatus PS_BACK =		0x002;
//const FaceStatus PS_ON =		0x004;
const FaceStatus PS_SPAN =		0x008;

const FaceStatus PS_SPAN0_12 =	0x010;
const FaceStatus PS_SPAN1_02 =	0x020;
const FaceStatus PS_SPAN2_01 =	0x040;

const FaceStatus PS_SPAN01_2 =	0x080;
const FaceStatus PS_SPAN02_1 =	0x100;
const FaceStatus PS_SPAN12_0 =	0x200;

//

struct TriMeshFaceGroup
{

public:		// Data

	D3DPRIMITIVETYPE primitive_type;

	char	   material_name[IM_MAX_NAME_LEN];
	IMaterial *material;
	void	  *extended_context;			// application specific context

	U32	 min_vertex_index;
	U32  max_vertex_index;

	U16  *face_indices;
	U32  num_face_indices;

	U16  *edge_indices;						// 2 per edge, into current_vertex_desc->Ps.data
	float *edge_angles;
	U32  num_edge_indices;

	VertexBufferDesc *current_vertex_desc;	// may or may not be the same as shared_vertex_desc
	const VertexBufferDesc *shared_vertex_desc;

public:		// Interface

	TriMeshFaceGroup();
	TriMeshFaceGroup( const TriMeshFaceGroup &fg );
	~TriMeshFaceGroup();
	TriMeshFaceGroup &operator=( const TriMeshFaceGroup &fg );

	void set_face_indices( U32 num_indices, const U16 *indices );
	void set_edges( U32 num_indices, const U16 *indices, const float *angles );
	void set_min_max_info( void );
	void copy_face_group_style( const TriMeshFaceGroup & src );
	void copy_selected_faces( const TriMeshFaceGroup & src, const FaceStatus * status_list, FaceStatus selected, int count );
	void make_used_data_local( void );
	FaceStatus get_face_side( int f_id, const Vector & plane_normal, float plane_d );
	void distribute_split_faces(TriMeshFaceGroup & fg_front, TriMeshFaceGroup & fg_back, const FaceStatus * status_list, int count) const;
	bool has_elements( IM_ELEMENT element_mask ) const;
};

//

typedef std::vector<TriMeshFaceGroup*> FaceGroupList;

//

#endif // EOF
