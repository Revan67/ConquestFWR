// TriMeshFaceGroup.cpp
//
//
//

//

#include "FDUMP.h"

//

#include "TriMeshFaceGroup.h"
#include "VertexBufferDescUtil.h"
#include "packed_argb.h"
//

//

TriMeshFaceGroup::TriMeshFaceGroup()
{
	material_name[0] = 0;
	material = NULL;
	extended_context = NULL;

	min_vertex_index = 0;
	max_vertex_index = 0;

	face_indices = NULL;
	num_face_indices = 0;

	edge_indices = NULL;
	edge_angles = NULL;
	num_edge_indices = 0;
	
	primitive_type = D3DPT_TRIANGLELIST;	

	current_vertex_desc = NULL;
	shared_vertex_desc = NULL;
}

//

TriMeshFaceGroup::TriMeshFaceGroup( const TriMeshFaceGroup &fg )
{
	material = NULL;
	face_indices = NULL;

	operator=( fg );
}

//

TriMeshFaceGroup::~TriMeshFaceGroup()
{
	min_vertex_index = 0;
	max_vertex_index = 0;

	extended_context = NULL;

	set_face_indices( 0, NULL );
	set_edges( 0, NULL, NULL );

	DACOM_RELEASE( material );

	strcpy( material_name, "@@@DELETED@@@" );

	if(current_vertex_desc != shared_vertex_desc)
	{
		FreeVertexBufferDesc( *current_vertex_desc );
		delete current_vertex_desc;
	}

	current_vertex_desc = NULL;
	shared_vertex_desc = NULL;
}

//

TriMeshFaceGroup &TriMeshFaceGroup::operator=( const TriMeshFaceGroup &fg )
{
	set_face_indices( fg.num_face_indices, fg.face_indices );
	set_edges( fg.num_edge_indices, fg.edge_indices, fg.edge_angles );
	
	primitive_type = fg.primitive_type;

	min_vertex_index = fg.min_vertex_index;
	max_vertex_index = fg.max_vertex_index;

	strcpy( material_name, fg.material_name );
	
	DACOM_RELEASE( material );	
	if( (material = fg.material) != NULL ) {
		material->AddRef();
	}

	extended_context = fg.extended_context;

	shared_vertex_desc = fg.shared_vertex_desc;

	if( fg.current_vertex_desc != fg.shared_vertex_desc )
	{
		current_vertex_desc = new VertexBufferDesc;
		memset(current_vertex_desc, 0, sizeof(VertexBufferDesc));

		CopyVertexBufferDesc( *fg.current_vertex_desc, *current_vertex_desc );
	}
	else
	{
		current_vertex_desc = fg.current_vertex_desc;
	}
	
	return *this;
}

//

void TriMeshFaceGroup::set_face_indices( U32 num_indices, const U16 *indices )
{
	delete[] face_indices;
	face_indices = NULL;

	if( num_indices && (face_indices = new U16[num_indices]) ) {
		memcpy( face_indices, indices, num_indices*sizeof(U16) );
	}

	min_vertex_index = 0;
	max_vertex_index = 0;

	num_face_indices = num_indices;
}

//

void TriMeshFaceGroup::set_edges( U32 num_indices, const U16 *indices, const float *angles )
{
	delete[] edge_indices;
	edge_indices = NULL;

	delete[] edge_angles;
	edge_angles = NULL;

	num_edge_indices = 0;

	if( num_indices )
	{
		edge_indices = new U16[num_indices];
		memcpy( edge_indices, indices, num_indices * sizeof(U16) );

		if( angles )
		{
			edge_angles = new float[num_indices / 2];
			memcpy( edge_angles, angles, (num_indices / 2) * sizeof(float) );
		}
		
		num_edge_indices = num_indices;
	}
}

//

void TriMeshFaceGroup::set_min_max_info( void )
{
	U32 fi;

	// this initialization is just to make less of the if's below be true
	min_vertex_index =
		(face_indices[0] < face_indices[num_face_indices-1]) ? face_indices[0] : face_indices[num_face_indices-1];
	max_vertex_index =
		(face_indices[0] < face_indices[num_face_indices-1]) ? face_indices[num_face_indices-1] : face_indices[0];

	for( fi = 0; fi < num_face_indices; fi++ ) {

		if( face_indices[fi] < min_vertex_index ) {
			min_vertex_index = face_indices[fi] ;
		}else
		if( face_indices[fi] > max_vertex_index ) {
			max_vertex_index = face_indices[fi] ;
		}
	}

	for( fi = 0; fi < num_face_indices; fi++ ) {
		face_indices[fi] -= min_vertex_index;
	}
}

//

void TriMeshFaceGroup::copy_face_group_style( const TriMeshFaceGroup & src )
{
	primitive_type = src.primitive_type;

	strcpy( material_name, src.material_name );
	
	if( (material = src.material) != NULL )
	{
		material->AddRef();
	}

	extended_context = src.extended_context;
}

//

int MakeMap(int *list, const int count, const int key)
{
	int new_id = 0;
	for(int i = 0; i < count; i++)
	{
		if( list[i] == key )
		{
			list[i] = new_id;
			new_id++;
		}
	}
	
	return new_id;
}

//

void Remap( const VertexBufferItemDesc & src_idesc, const int src_idx_cnt,
		   VertexBufferItemDesc & dest_idesc, const int dest_idx_cnt, const int dest_data_cnt,
		   const int *remap_indices, const int *remap_data )
{
	dest_idesc.size = src_idesc.size;
	dest_idesc.stride = src_idesc.size; // NOT stride!
	dest_idesc.indices = new U32[dest_idx_cnt];
	dest_idesc.count = dest_data_cnt;
	dest_idesc.data = new U8[dest_idesc.count * dest_idesc.stride];
	
	// indices
	for(int id = 0; id < src_idx_cnt; id++)
	{
		if( remap_indices[id] >= 0 )
		{
			dest_idesc.indices[ remap_indices[id] ] = remap_data[ src_idesc.indices[id] ];
		}
	}

	// data
	for( id = 0; id < src_idesc.count; id++)
	{
		if( remap_data[id] >= 0 )
		{
			memcpy( (U8*)dest_idesc.data + remap_data[id] * dest_idesc.stride,
					(U8*)src_idesc.data + id * src_idesc.stride, src_idesc.size);
		}
	}	
}

//
// TODO: handle edges
void TriMeshFaceGroup::make_used_data_local( void )
{
	if( current_vertex_desc == shared_vertex_desc )
	{
		const VertexBufferItemDesc & points = shared_vertex_desc->Ps;
		const VertexBufferItemDesc & normals = shared_vertex_desc->Ns;
		const VertexBufferItemDesc & mc0s = shared_vertex_desc->MC0s;
		const VertexBufferItemDesc & mc1s = shared_vertex_desc->MC1s;
		const VertexBufferItemDesc & c0s = shared_vertex_desc->C0s;

		current_vertex_desc = new VertexBufferDesc;
		memset(current_vertex_desc, 0, sizeof(VertexBufferDesc));

		current_vertex_desc->flags = shared_vertex_desc->flags;
		current_vertex_desc->vertex_format = shared_vertex_desc->vertex_format;

		int *remap_indices = new int[shared_vertex_desc->num_vertices];
		memset(remap_indices, 0xff, shared_vertex_desc->num_vertices * sizeof(int));

		int *remap_points = NULL;
		if(shared_vertex_desc->vertex_format & D3DFVF_XYZ)
		{
			remap_points = new int[points.count];
			memset(remap_points, 0xff, points.count * sizeof(int));
		}

		int *remap_normals = NULL;
		if(shared_vertex_desc->vertex_format & D3DFVF_NORMAL)
		{
			remap_normals = new int[normals.count];
			memset(remap_normals, 0xff, normals.count * sizeof(int));
		}

		int *remap_mc0s = NULL;
		if(shared_vertex_desc->vertex_format & D3DFVF_TEX1)
		{
			remap_mc0s = new int[mc0s.count];
			memset(remap_mc0s, 0xff, mc0s.count * sizeof(int));
		}

		int *remap_mc1s = NULL;
		if(shared_vertex_desc->vertex_format & D3DFVF_TEX2)
		{
			remap_mc1s = new int[mc1s.count];
			memset(remap_mc1s, 0xff, mc1s.count * sizeof(int));
		}

		int *remap_c0s = NULL;
		if(shared_vertex_desc->vertex_format & D3DFVF_DIFFUSE)
		{
			remap_c0s = new int[c0s.count];
			memset(remap_c0s, 0xff, c0s.count * sizeof(int));
		}

		// mark used indices and data
		int used_index_cnt = 0;
		int used_point_cnt = 0;
		int used_normal_cnt = 0;
		int used_mc0_cnt = 0;
		int used_mc1_cnt = 0;
		int used_c0_cnt = 0;
		for(int fi = 0; fi < num_face_indices; fi++)
		{
			int id = face_indices[fi];
			if( remap_indices[id] == -1 )
			{
				remap_indices[id] = -2;
				used_index_cnt++;

				if( remap_points && remap_points[ points.indices[id] ] == -1 )
				{
					remap_points[ points.indices[id] ] = -2;
					used_point_cnt++;
				}

				if( remap_normals && remap_normals[ normals.indices[id] ] == -1 )
				{
					remap_normals[ normals.indices[id] ] = -2;
					used_normal_cnt++;
				}

				if( remap_mc0s && remap_mc0s[ mc0s.indices[id] ] == -1 )
				{
					remap_mc0s[ mc0s.indices[id] ] = -2;
					used_mc0_cnt++;
				}

				if( remap_mc1s && remap_mc1s[ mc1s.indices[id] ] == -1 )
				{
					remap_mc1s[ mc1s.indices[id] ] = -2;
					used_mc1_cnt++;
				}

				if( remap_c0s && remap_c0s[ c0s.indices[id] ] == -1 )
				{
					remap_c0s[ c0s.indices[id] ] = -2;
					used_c0_cnt++;
				}
			} 
		}

		//create remap tables
	
		int tmp_cnt;

		tmp_cnt = MakeMap(remap_indices, shared_vertex_desc->num_vertices, -2);
		ASSERT( tmp_cnt == used_index_cnt );

		tmp_cnt = MakeMap(remap_points, points.count, -2);
		ASSERT( tmp_cnt == used_point_cnt );

		tmp_cnt = MakeMap(remap_normals, normals.count, -2);
		ASSERT( tmp_cnt == used_normal_cnt );

		tmp_cnt = MakeMap(remap_mc0s, mc0s.count, -2);
		ASSERT( tmp_cnt == used_mc0_cnt );

		tmp_cnt = MakeMap(remap_mc1s, mc1s.count, -2);
		ASSERT( tmp_cnt == used_mc1_cnt );

		tmp_cnt = MakeMap(remap_c0s, c0s.count, -2);
		ASSERT( tmp_cnt == used_c0_cnt );

		// reindex face group		
		for( fi = 0; fi < num_face_indices; fi++)
		{
			face_indices[fi] = remap_indices[ face_indices[fi] ];
		}

		current_vertex_desc->num_vertices = used_index_cnt;

		// create and reindex point stuff
		if( remap_points )
		{
			Remap( points, shared_vertex_desc->num_vertices,
				   current_vertex_desc->Ps, used_index_cnt, used_point_cnt,
				   remap_indices, remap_points );
		}

		if( remap_normals )
		{
			Remap( normals, shared_vertex_desc->num_vertices,
				   current_vertex_desc->Ns, used_index_cnt, used_normal_cnt,
				   remap_indices, remap_normals );
		}

		if( remap_mc0s )
		{
			Remap( mc0s, shared_vertex_desc->num_vertices,
				   current_vertex_desc->MC0s, used_index_cnt, used_mc0_cnt,
				   remap_indices, remap_mc0s );
		}

		if( remap_mc1s )
		{
			Remap( mc1s, shared_vertex_desc->num_vertices,
				   current_vertex_desc->MC1s, used_index_cnt, used_mc1_cnt,
				   remap_indices, remap_mc1s );
		}

		if( remap_c0s )
		{
			Remap( c0s, shared_vertex_desc->num_vertices,
				   current_vertex_desc->C0s, used_index_cnt, used_c0_cnt,
				   remap_indices, remap_c0s );
		}

		delete [] remap_indices;
		delete [] remap_points;
		delete [] remap_normals;
		delete [] remap_mc0s;
		delete [] remap_mc1s;
		delete [] remap_c0s;

		set_min_max_info();	
	}
}

//

void TriMeshFaceGroup::copy_selected_faces( const TriMeshFaceGroup & src, const FaceStatus * status_list,
										   FaceStatus selected, int count )
{
	num_face_indices = 3 * count;
	face_indices = new U16[num_face_indices];
	
	int f_id = 0;
	for(int src_f_id = 0; src_f_id < src.num_face_indices/3; src_f_id++)
	{
		if( status_list[src_f_id] == selected )
		{
			face_indices[3 * f_id    ] = src.face_indices[3 * src_f_id    ];
			face_indices[3 * f_id + 1] = src.face_indices[3 * src_f_id + 1];
			face_indices[3 * f_id + 2] = src.face_indices[3 * src_f_id + 2];

			f_id++;
		}
	}

	ASSERT(f_id == count );

	make_used_data_local();
}

//

FaceStatus TriMeshFaceGroup::get_face_side( int f_id, const Vector & plane_normal, float plane_d )
{
	const int stride = current_vertex_desc->Ps.stride;
	const U32 *indices = current_vertex_desc->Ps.indices;
	const U8 *data = (U8*)current_vertex_desc->Ps.data;
	const U16 *f_indices = face_indices + 3 * f_id;

	const Vector & v0 = *(Vector*)(data + stride * indices[f_indices[0]]);
	const Vector & v1 = *(Vector*)(data + stride * indices[f_indices[1]]);
	const Vector & v2 = *(Vector*)(data + stride * indices[f_indices[2]]);
	
	FaceStatus vs0 = (dot_product( plane_normal, v0 ) > -plane_d) ? PS_FRONT : PS_BACK;
	FaceStatus vs1 = (dot_product( plane_normal, v1 ) > -plane_d) ? PS_FRONT : PS_BACK;
	FaceStatus vs2 = (dot_product( plane_normal, v2 ) > -plane_d) ? PS_FRONT : PS_BACK;

	if( vs0 == PS_FRONT )
	{
		if( vs1 == PS_FRONT )
		{
			if( vs2 == PS_FRONT )
			{
				return PS_FRONT;
			}
			else
			{
				return PS_SPAN | PS_SPAN01_2;
			}
		}
		else
		{
			if( vs2 == PS_FRONT )
			{
				return PS_SPAN | PS_SPAN02_1;
			}
			else
			{
				return PS_SPAN | PS_SPAN0_12;
			}
		}
	}
	else
	{
		if( vs1 == PS_BACK )
		{
			if( vs2 == PS_BACK )
			{
				return PS_BACK;
			}
			else
			{
				return PS_SPAN | PS_SPAN2_01;
			}
		}
		else
		{
			if( vs2 == PS_BACK )
			{
				return PS_SPAN | PS_SPAN1_02;
			}
			else
			{
				return PS_SPAN | PS_SPAN12_0;
			}
		}
	}
}

//
// this is not a general function (only use w/ distribute_split_faces())
static void GrowVertexBufferItemDesc( VertexBufferItemDesc & vbi, const int old_cnt, const int new_cnt )
{
	const int add_cnt = new_cnt - old_cnt;

	U8 *new_data = new U8[(vbi.count + add_cnt) * vbi.stride];
	memcpy(new_data, vbi.data, vbi.count * vbi.stride);
	delete [] vbi.data;
	vbi.data = new_data;

	U32 *new_idx = new U32[new_cnt];
	memcpy(new_idx, vbi.indices, old_cnt * sizeof(U32));
	delete [] vbi.indices;
	vbi.indices = new_idx;
}

inline void AddToVertexBufferItemDesc(VertexBufferItemDesc & vbi, const void *data, const U32 index)
{
	memcpy((U8*)vbi.data + vbi.count * vbi.stride, data, vbi.size);
	vbi.indices[index] = vbi.count;
	vbi.count++;
}

void TriMeshFaceGroup::distribute_split_faces(TriMeshFaceGroup & fg_0, TriMeshFaceGroup & fg_1, const FaceStatus * status_list, int count) const
{
	if( count < 1 )
		return;

typedef float UV2[2];

	U16 *tmp_fi;
	tmp_fi = new U16[fg_0.num_face_indices + 3 * count];
	memcpy(tmp_fi, fg_0.face_indices, fg_0.num_face_indices * sizeof(U16));
	delete [] fg_0.face_indices;
	fg_0.face_indices = tmp_fi;

	tmp_fi = new U16[fg_1.num_face_indices + 3 * count];
	memcpy(tmp_fi, fg_1.face_indices, fg_1.num_face_indices * sizeof(U16));
	delete [] fg_1.face_indices;
	fg_1.face_indices = tmp_fi;

	// make dest vertex buffers unique
	if( fg_0.shared_vertex_desc == fg_0.current_vertex_desc )
	{
		fg_0.current_vertex_desc = new VertexBufferDesc;
		memset(fg_0.current_vertex_desc, 0, sizeof(VertexBufferDesc));

		fg_0.current_vertex_desc->flags = current_vertex_desc->flags;
		fg_0.current_vertex_desc->vertex_format = current_vertex_desc->vertex_format;

		fg_0.current_vertex_desc->Ps.size = current_vertex_desc->Ps.size;
		fg_0.current_vertex_desc->Ps.stride = current_vertex_desc->Ps.stride;
		fg_0.current_vertex_desc->Ns.size = current_vertex_desc->Ns.size;
		fg_0.current_vertex_desc->Ns.stride = current_vertex_desc->Ns.stride;
		fg_0.current_vertex_desc->MC0s.size = current_vertex_desc->MC0s.size;
		fg_0.current_vertex_desc->MC0s.stride = current_vertex_desc->MC0s.stride;
		fg_0.current_vertex_desc->MC1s.size = current_vertex_desc->MC1s.size;
		fg_0.current_vertex_desc->MC1s.stride = current_vertex_desc->MC1s.stride;
		fg_0.current_vertex_desc->C0s.size = current_vertex_desc->C0s.size;
		fg_0.current_vertex_desc->C0s.stride = current_vertex_desc->C0s.stride;
		fg_0.current_vertex_desc->C1s.size = current_vertex_desc->C1s.size;
		fg_0.current_vertex_desc->C1s.stride = current_vertex_desc->C1s.stride;
	}

	if( fg_1.shared_vertex_desc == fg_1.current_vertex_desc )
	{
		fg_1.current_vertex_desc = new VertexBufferDesc;
		memset(fg_1.current_vertex_desc, 0, sizeof(VertexBufferDesc));

		fg_1.current_vertex_desc->flags = current_vertex_desc->flags;
		fg_1.current_vertex_desc->vertex_format = current_vertex_desc->vertex_format;

		fg_1.current_vertex_desc->Ps.size = current_vertex_desc->Ps.size;
		fg_1.current_vertex_desc->Ps.stride = current_vertex_desc->Ps.stride;
		fg_1.current_vertex_desc->Ns.size = current_vertex_desc->Ns.size;
		fg_1.current_vertex_desc->Ns.stride = current_vertex_desc->Ns.stride;
		fg_1.current_vertex_desc->MC0s.size = current_vertex_desc->MC0s.size;
		fg_1.current_vertex_desc->MC0s.stride = current_vertex_desc->MC0s.stride;
		fg_1.current_vertex_desc->MC1s.size = current_vertex_desc->MC1s.size;
		fg_1.current_vertex_desc->MC1s.stride = current_vertex_desc->MC1s.stride;
		fg_1.current_vertex_desc->C0s.size = current_vertex_desc->C0s.size;
		fg_1.current_vertex_desc->C0s.stride = current_vertex_desc->C0s.stride;
		fg_1.current_vertex_desc->C1s.size = current_vertex_desc->C1s.size;
		fg_1.current_vertex_desc->C1s.stride = current_vertex_desc->C1s.stride;
	}

	VertexBufferDesc & vbd0 = *fg_0.current_vertex_desc;
	VertexBufferDesc & vbd1 = *fg_1.current_vertex_desc;

	const bool has_normals = ( 0 != (current_vertex_desc->vertex_format & D3DFVF_NORMAL) );
	const bool has_mc0s = ( 0 != (current_vertex_desc->vertex_format & D3DFVF_TEX1) );
	const bool has_mc1s = ( 0 != (current_vertex_desc->vertex_format & D3DFVF_TEX2) );
	const bool has_c0s = ( 0 != (current_vertex_desc->vertex_format & D3DFVF_DIFFUSE) );

	GrowVertexBufferItemDesc( vbd0.Ps, vbd0.num_vertices, vbd0.num_vertices + 3 * count);
	GrowVertexBufferItemDesc( vbd1.Ps, vbd1.num_vertices, vbd1.num_vertices + 3 * count);

	if( has_normals )
	{
		GrowVertexBufferItemDesc( vbd0.Ns, vbd0.num_vertices, vbd0.num_vertices + 3 * count);
		GrowVertexBufferItemDesc( vbd1.Ns, vbd1.num_vertices, vbd1.num_vertices + 3 * count);
	}

	if( has_mc0s )
	{
		GrowVertexBufferItemDesc( vbd0.MC0s, vbd0.num_vertices, vbd0.num_vertices + 3 * count);
		GrowVertexBufferItemDesc( vbd1.MC0s, vbd1.num_vertices, vbd1.num_vertices + 3 * count);
	}

	if( has_mc1s )
	{
		GrowVertexBufferItemDesc( vbd0.MC1s, vbd0.num_vertices, vbd0.num_vertices + 3 * count);
		GrowVertexBufferItemDesc( vbd1.MC1s, vbd1.num_vertices, vbd1.num_vertices + 3 * count);
	}

	if( has_c0s )
	{
		GrowVertexBufferItemDesc( vbd0.C0s, vbd0.num_vertices, vbd0.num_vertices + 3 * count);
		GrowVertexBufferItemDesc( vbd1.C0s, vbd1.num_vertices, vbd1.num_vertices + 3 * count);
	}
	
	for(int f_id = 0; f_id < num_face_indices/3; f_id++)
	{
		if( status_list[f_id] & PS_SPAN )
		{
			const U16 *f_indices = face_indices + 3 * f_id;

			const int p_stride = current_vertex_desc->Ps.stride;
			const U32 *p_indices = current_vertex_desc->Ps.indices;
			const U8 *p_data = (U8*)current_vertex_desc->Ps.data;

			const int n_stride = current_vertex_desc->Ns.stride;
			const U32 *n_indices = current_vertex_desc->Ns.indices;
			const U8 *n_data = (U8*)current_vertex_desc->Ns.data;

			const int mc0_stride = current_vertex_desc->MC0s.stride;
			const U32 *mc0_indices = current_vertex_desc->MC0s.indices;
			const U8 *mc0_data = (U8*)current_vertex_desc->MC0s.data;

			const int mc1_stride = current_vertex_desc->MC1s.stride;
			const U32 *mc1_indices = current_vertex_desc->MC1s.indices;
			const U8 *mc1_data = (U8*)current_vertex_desc->MC1s.data;

			const int c0_stride = current_vertex_desc->C0s.stride;
			const U32 *c0_indices = current_vertex_desc->C0s.indices;
			const U8 *c0_data = (U8*)current_vertex_desc->C0s.data;
			
			const Vector *points_1[3];
			const Vector *normals_1[3];
			const UV2 *mc0s_1[3];
			const UV2 *mc1s_1[3];
			const PACKEDARGB *c0s_1[3];

			const Vector *points_2[3];
			const Vector *normals_2[3];
			const UV2 *mc0s_2[3];
			const UV2 *mc1s_2[3];
			const PACKEDARGB *c0s_2[3];

			Vector split_p;
			Vector split_n;
			UV2 split_mc0;
			UV2 split_mc1;
			PACKEDARGB split_c0;

			bool flip; // use x_1 as x_2 and vice versa
			if((status_list[f_id] & PS_SPAN0_12) || (status_list[f_id] & PS_SPAN12_0))
			{
				// 0 & 1
				points_1[0] = (Vector*)(p_data + p_stride * p_indices[f_indices[0]]);
				points_1[1] = &split_p;
				points_1[2] = (Vector*)(p_data + p_stride * p_indices[f_indices[2]]);

				points_2[0] = &split_p;
				points_2[1] = (Vector*)(p_data + p_stride * p_indices[f_indices[1]]);
				points_2[2] = (Vector*)(p_data + p_stride * p_indices[f_indices[2]]);

				split_p = .5f * (*(points_1[0]) + *(points_2[1]));

				if( has_normals )
				{
					normals_1[0] = (Vector*)(n_data + n_stride * n_indices[f_indices[0]]);
					normals_1[1] = &split_p;
					normals_1[2] = (Vector*)(n_data + n_stride * n_indices[f_indices[2]]);

					normals_2[0] = &split_p;
					normals_2[1] = (Vector*)(n_data + n_stride * n_indices[f_indices[1]]);
					normals_2[2] = (Vector*)(n_data + n_stride * n_indices[f_indices[2]]);

					split_n = (*(normals_1[0]) + *(normals_2[1]));
					split_n.normalize();
				}

				if( has_mc0s )
				{
					mc0s_1[0] = (UV2*)(mc0_data + mc0_stride * mc0_indices[f_indices[0]]);
					mc0s_1[1] = &split_mc0;
					mc0s_1[2] = (UV2*)(mc0_data + mc0_stride * mc0_indices[f_indices[2]]);

					mc0s_2[0] = &split_mc0;
					mc0s_2[1] = (UV2*)(mc0_data + mc0_stride * mc0_indices[f_indices[1]]);
					mc0s_2[2] = (UV2*)(mc0_data + mc0_stride * mc0_indices[f_indices[2]]);

					split_mc0[0] = .5f * (*(mc0s_1[0][0]) + *(mc0s_2[1][0]));
					split_mc0[1] = .5f * (*(mc0s_1[0][1]) + *(mc0s_2[1][1]));
				}

				if( has_mc1s )
				{
					mc1s_1[0] = (UV2*)(mc1_data + mc1_stride * mc1_indices[f_indices[0]]);
					mc1s_1[1] = &split_mc1;
					mc1s_1[2] = (UV2*)(mc1_data + mc1_stride * mc1_indices[f_indices[2]]);

					mc1s_2[0] = &split_mc1;
					mc1s_2[1] = (UV2*)(mc1_data + mc1_stride * mc1_indices[f_indices[1]]);
					mc1s_2[2] = (UV2*)(mc1_data + mc1_stride * mc1_indices[f_indices[2]]);

					split_mc1[0] = .5f * (*(mc1s_1[0][0]) + *(mc1s_2[1][0]));
					split_mc1[1] = .5f * (*(mc1s_1[0][1]) + *(mc1s_2[1][1]));
				}

				if( has_c0s )
				{
					c0s_1[0] = (PACKEDARGB*)(c0_data + c0_stride * c0_indices[f_indices[0]]);
					c0s_1[1] = &split_c0;
					c0s_1[2] = (PACKEDARGB*)(c0_data + c0_stride * c0_indices[f_indices[2]]);

					c0s_2[0] = &split_c0;
					c0s_2[1] = (PACKEDARGB*)(c0_data + c0_stride * c0_indices[f_indices[1]]);
					c0s_2[2] = (PACKEDARGB*)(c0_data + c0_stride * c0_indices[f_indices[2]]);

					split_c0 = ( (*(c0s_1[0]) + *(c0s_2[1])) ) / 2;
				}

				flip = ( status_list[f_id] & PS_SPAN0_12 ) ? false : true;

			}else
			if((status_list[f_id] & PS_SPAN1_02) || (status_list[f_id] & PS_SPAN02_1))
			{
				// 1 & 2
				points_1[0] = (Vector*)(p_data + p_stride * p_indices[f_indices[0]]);
				points_1[1] = (Vector*)(p_data + p_stride * p_indices[f_indices[1]]);
				points_1[2] = &split_p;

				points_2[0] = &split_p;
				points_2[1] = (Vector*)(p_data + p_stride * p_indices[f_indices[2]]);
				points_2[2] = (Vector*)(p_data + p_stride * p_indices[f_indices[0]]);

				split_p = .5f * (*(points_1[1]) + *(points_2[1]));

				if( has_normals )
				{
					normals_1[0] = (Vector*)(n_data + n_stride * n_indices[f_indices[0]]);
					normals_1[1] = (Vector*)(n_data + n_stride * n_indices[f_indices[1]]);
					normals_1[2] = &split_p;

					normals_2[0] = &split_p;
					normals_2[1] = (Vector*)(n_data + n_stride * n_indices[f_indices[2]]);
					normals_2[2] = (Vector*)(n_data + n_stride * n_indices[f_indices[0]]);

					split_n = (*(normals_1[1]) + *(normals_2[1]));
					split_n.normalize();
				}

				if( has_mc0s )
				{
					mc0s_1[0] = (UV2*)(mc0_data + mc0_stride * mc0_indices[f_indices[0]]);
					mc0s_1[1] = (UV2*)(mc0_data + mc0_stride * mc0_indices[f_indices[1]]);
					mc0s_1[2] = &split_mc0;

					mc0s_2[0] = &split_mc0;
					mc0s_2[1] = (UV2*)(mc0_data + mc0_stride * mc0_indices[f_indices[2]]);
					mc0s_2[2] = (UV2*)(mc0_data + mc0_stride * mc0_indices[f_indices[0]]);
					
					split_mc0[0] = .5f * (*(mc0s_1[1][0]) + *(mc0s_2[1][0]));
					split_mc0[1] = .5f * (*(mc0s_1[1][1]) + *(mc0s_2[1][1]));
				}

				if( has_mc1s )
				{
					mc1s_1[0] = (UV2*)(mc1_data + mc1_stride * mc1_indices[f_indices[0]]);
					mc1s_1[1] = (UV2*)(mc1_data + mc1_stride * mc1_indices[f_indices[1]]);
					mc1s_1[2] = &split_mc1;

					mc1s_2[0] = &split_mc1;
					mc1s_2[1] = (UV2*)(mc1_data + mc1_stride * mc1_indices[f_indices[2]]);
					mc1s_2[2] = (UV2*)(mc1_data + mc1_stride * mc1_indices[f_indices[0]]);
					
					split_mc1[0] = .5f * (*(mc1s_1[1][0]) + *(mc1s_2[1][0]));
					split_mc1[1] = .5f * (*(mc1s_1[1][1]) + *(mc1s_2[1][1]));
				}

				if( has_c0s )
				{
					c0s_1[0] = (PACKEDARGB*)(c0_data + c0_stride * c0_indices[f_indices[0]]);
					c0s_1[1] = (PACKEDARGB*)(c0_data + c0_stride * c0_indices[f_indices[1]]);
					c0s_1[2] = &split_c0;

					c0s_2[0] = &split_c0;
					c0s_2[1] = (PACKEDARGB*)(c0_data + c0_stride * c0_indices[f_indices[2]]);
					c0s_2[2] = (PACKEDARGB*)(c0_data + c0_stride * c0_indices[f_indices[0]]);
					
					split_c0 = ( (*(c0s_1[1]) + *(c0s_2[1])) ) / 2;
				}

				flip = ( status_list[f_id] & PS_SPAN1_02 ) ? false : true;	
			}
			else //if((status_list[f_id] & PS_SPAN2_01) || (status_list[f_id] & PS_SPAN01_2))
			{
				// 0 & 2
				points_1[0] = (Vector*)(p_data + p_stride * p_indices[f_indices[1]]);
				points_1[1] = (Vector*)(p_data + p_stride * p_indices[f_indices[2]]);
				points_1[2] = &split_p;

				points_2[0] = &split_p;
				points_2[1] = (Vector*)(p_data + p_stride * p_indices[f_indices[0]]);
				points_2[2] = (Vector*)(p_data + p_stride * p_indices[f_indices[1]]);

				split_p = .5f * (*(points_1[1]) + *(points_2[1]));

				if( has_normals )
				{
					normals_1[0] = (Vector*)(n_data + n_stride * n_indices[f_indices[1]]);
					normals_1[1] = (Vector*)(n_data + n_stride * n_indices[f_indices[2]]);
					normals_1[2] = &split_p;

					normals_2[0] = &split_p;
					normals_2[1] = (Vector*)(n_data + n_stride * n_indices[f_indices[0]]);
					normals_2[2] = (Vector*)(n_data + n_stride * n_indices[f_indices[1]]);

					split_n = (*(normals_1[1]) + *(normals_2[1]));
					split_n.normalize();
				}

				if( has_mc0s )
				{
					mc0s_1[0] = (UV2*)(mc0_data + mc0_stride * mc0_indices[f_indices[1]]);
					mc0s_1[1] = (UV2*)(mc0_data + mc0_stride * mc0_indices[f_indices[2]]);
					mc0s_1[2] = &split_mc0;

					mc0s_2[0] = &split_mc0;
					mc0s_2[1] = (UV2*)(mc0_data + mc0_stride * mc0_indices[f_indices[0]]);
					mc0s_2[2] = (UV2*)(mc0_data + mc0_stride * mc0_indices[f_indices[1]]);

					split_mc0[0] = .5f * (*(mc0s_1[1][0]) + *(mc0s_2[1][0]));
					split_mc0[1] = .5f * (*(mc0s_1[1][1]) + *(mc0s_2[1][1]));
				}

				if( has_mc1s )
				{
					mc1s_1[0] = (UV2*)(mc1_data + mc1_stride * mc1_indices[f_indices[1]]);
					mc1s_1[1] = (UV2*)(mc1_data + mc1_stride * mc1_indices[f_indices[2]]);
					mc1s_1[2] = &split_mc1;

					mc1s_2[0] = &split_mc1;
					mc1s_2[1] = (UV2*)(mc1_data + mc1_stride * mc1_indices[f_indices[0]]);
					mc1s_2[2] = (UV2*)(mc1_data + mc1_stride * mc1_indices[f_indices[1]]);

					split_mc1[0] = .5f * (*(mc1s_1[1][0]) + *(mc1s_2[1][0]));
					split_mc1[1] = .5f * (*(mc1s_1[1][1]) + *(mc1s_2[1][1]));
				}

				if( has_c0s )
				{
					c0s_1[0] = (PACKEDARGB*)(c0_data + c0_stride * c0_indices[f_indices[1]]);
					c0s_1[1] = (PACKEDARGB*)(c0_data + c0_stride * c0_indices[f_indices[2]]);
					c0s_1[2] = &split_c0;

					c0s_2[0] = &split_c0;
					c0s_2[1] = (PACKEDARGB*)(c0_data + c0_stride * c0_indices[f_indices[0]]);
					c0s_2[2] = (PACKEDARGB*)(c0_data + c0_stride * c0_indices[f_indices[1]]);

					split_c0 = ( (*(c0s_1[1]) + *(c0s_2[1])) ) / 2;
				}

				flip = ( status_list[f_id] & PS_SPAN2_01 ) ? false : true;	
			}

			for(int i = 0; i < 3; i++)
			{
				// add to group 0
				if( !flip )
				{
					AddToVertexBufferItemDesc( vbd0.Ps, points_1[i], vbd0.num_vertices );
					if( has_normals )
						AddToVertexBufferItemDesc( vbd0.Ns, normals_1[i], vbd0.num_vertices );
					if( has_mc0s )
						AddToVertexBufferItemDesc( vbd0.MC0s, mc0s_1[i], vbd0.num_vertices );
					if( has_mc1s )
						AddToVertexBufferItemDesc( vbd0.MC1s, mc1s_1[i], vbd0.num_vertices );
					if( has_c0s )
						AddToVertexBufferItemDesc( vbd0.C0s, c0s_1[i], vbd0.num_vertices );
				}
				else
				{
					AddToVertexBufferItemDesc( vbd0.Ps, points_2[i], vbd0.num_vertices );
					if( has_normals )
						AddToVertexBufferItemDesc( vbd0.Ns, normals_2[i], vbd0.num_vertices );
					if( has_mc0s )
						AddToVertexBufferItemDesc( vbd0.MC0s, mc0s_2[i], vbd0.num_vertices );
					if( has_mc1s )
						AddToVertexBufferItemDesc( vbd0.MC1s, mc1s_2[i], vbd0.num_vertices );
					if( has_c0s )
						AddToVertexBufferItemDesc( vbd0.C0s, c0s_2[i], vbd0.num_vertices );
				}
				fg_0.face_indices[fg_0.num_face_indices] = vbd0.num_vertices;
				fg_0.num_face_indices++;
				vbd0.num_vertices++;

				// add to group 1
				if( !flip )
				{
					AddToVertexBufferItemDesc( vbd1.Ps, points_2[i], vbd1.num_vertices );
					if( has_normals )
						AddToVertexBufferItemDesc( vbd1.Ns, normals_2[i], vbd1.num_vertices );
					if( has_mc0s )
						AddToVertexBufferItemDesc( vbd1.MC0s, mc0s_2[i], vbd1.num_vertices );
					if( has_mc1s )
						AddToVertexBufferItemDesc( vbd1.MC1s, mc1s_2[i], vbd1.num_vertices );
					if( has_c0s )
						AddToVertexBufferItemDesc( vbd1.C0s, c0s_2[i], vbd1.num_vertices );
				}
				else
				{
					AddToVertexBufferItemDesc( vbd1.Ps, points_1[i], vbd1.num_vertices );
					if( has_normals )
						AddToVertexBufferItemDesc( vbd1.Ns, normals_1[i], vbd1.num_vertices );
					if( has_mc0s )
						AddToVertexBufferItemDesc( vbd1.MC0s, mc0s_1[i], vbd1.num_vertices );
					if( has_mc1s )
						AddToVertexBufferItemDesc( vbd1.MC1s, mc1s_1[i], vbd1.num_vertices );
					if( has_c0s )
						AddToVertexBufferItemDesc( vbd1.C0s, c0s_1[i], vbd1.num_vertices );
				}
				fg_1.face_indices[fg_1.num_face_indices] = vbd1.num_vertices;
				fg_1.num_face_indices++;
				vbd1.num_vertices++;
			}
		}
	}
}

//

bool TriMeshFaceGroup::has_elements( IM_ELEMENT element_mask ) const
{
	if( (element_mask & IM_E_EDGES) && !edge_indices )
		return false;
	if( (element_mask & IM_E_EDGE_ANGLES) && !edge_angles )
		return false;
		
	return HasElements( *current_vertex_desc, element_mask );
}

//

// EOF
