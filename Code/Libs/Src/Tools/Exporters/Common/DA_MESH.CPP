#include <assert.h>
#include <string.h>
#include "misc.h"
#include "da_mesh.h"

// TODO:
// fix swaping lod vertex normals

#ifndef SGI
#pragma warning( disable : 4514 ) // unreferenced inline function has been removed
#endif

extern float face_normal_tolerance;
extern float vertex_normal_tolerance;

extern void  __cdecl sortQ(void *, size_t, size_t, 
					 int (__cdecl *comp)(const void *, const void *, const void *),
					 const void *comp_lib,
					 void (__cdecl *swap)(void *, const int id1, const int id2),
					 void *swap_lib);

inline int DoubleNormalize( Vector & normal )
{
	const double mag_sq = (double)normal.x * (double)normal.x +
						  (double)normal.y * (double)normal.y +
						  (double)normal.z * (double)normal.z;
	if( mag_sq > 0.0 )
	{
		const double inv_mag = 1.0 / sqrt( mag_sq );
		normal.x = (float)((double)normal.x * inv_mag);
		normal.y = (float)((double)normal.y * inv_mag);
		normal.z = (float)((double)normal.z * inv_mag);
		return 0;
	}
	else  // bad normal
	{
		normal.x = 1.0f;
		normal.y = 0.0f;
		normal.z = 0.0f;
		fprintf(stderr, "Bad normal.\n");
	}

	return -1;
}

// largest to smallest
int CompareEdgesByAngle(const void *pt1, const void *pt2, const void *)
{
  if( ((Eedge*)pt1)->angle < ((Eedge*)pt2)->angle)
  {
	  return 1;
  }
  else
  if(((Eedge*)pt1)->angle > ((Eedge*)pt2)->angle)
  {
	  return -1;
  }
  
  return 0;
}

// largest to smallest
int CompareEdgesByError(const void *pt1, const void *pt2, const void *)
{
  const Eedge *e1 = (Eedge*)pt1;
  const Eedge *e2 = (Eedge*)pt2;

  if(e1->error < e2->error)
  {
	  return 1;
  }
  else
  if(e1->error > e2->error)
  {
	  return -1;
  }
  
  return 0;
}

static void SwapEdges(void *e_list, const int id1, const int id2)
{
    if(id1 != id2)
	{
		MemSwap(((Eedge*)e_list) + id1, ((Eedge*)e_list) + id2, sizeof(Eedge));
	}
}

static int CalcNormal(const Vector & p0, const Vector & p1, const Vector & p2, Vector & normal)
{
	normal = cross_product( p1 - p0, p2 - p0 );
	return DoubleNormalize( normal );
}

int DAMesh::GetGroupID( const int mtl_id, const int api_node_id ) const
{
	for(int i = 0; i < grp_cnt; i++)
	{
		if( grp_list[i].mtl_id == mtl_id &&
			grp_list[i].api_node_id == api_node_id )
		{
			return i;
		}
	}

	return -1;
}

inline Vector GetNormal(const Vector & v0, const Vector & v1, const Vector & v2)
{
	Vector normal( cross_product( v1 - v0, v2 - v0 ) );

	DoubleNormalize( normal );

	return normal;
}

int DAMesh::GetNormalID( const Vector & normal, const Vector *n_list, const int length, const float tol_angle ) const
{
	const float cos_tolerance = (float)cos(tol_angle * D2R);
	float max_cos_angle = -1.0f;

	int n_id = -1;
	for(int i = 0; i < length; i++)
	{
		const float cos_angle = dot_product( normal, n_list[i] );

		if(cos_angle > max_cos_angle)
		{
			max_cos_angle = cos_angle;
			n_id = i;
		}
	}

	if( max_cos_angle > cos_tolerance )
	{
		return n_id;
	}
	else
	{
		return -1;
	}
}


int DAMesh::InsertFlatFNormal( const Vector & normal )
{
	assert( fabs(1.0f - normal.magnitude()) < .0001f );

	int n_id = GetNormalID( normal, flat_f_n_list, flat_f_n_cnt, FACE_NORMAL_TOLERANCE );

	if( n_id < 0 )
	{
		n_id = flat_f_n_cnt;
		flat_f_n_cnt++;
		flat_f_n_list = (Vector*)Realloc(flat_f_n_list, flat_f_n_cnt * sizeof(*flat_f_n_list));
		flat_f_n_list[n_id] = normal;
	}

	return n_id;
}

int DAMesh::InsertVertexNormal( const Vector & normal )
{
	assert( fabs(1.0f - normal.magnitude()) < .0001f );

	int n_id = GetNormalID( normal, vn_list, vn_cnt, VERTEX_NORMAL_TOLERANCE );

	if( n_id < 0 )
	{
		n_id = vn_cnt;
		vn_cnt++;
		vn_list = (Vector*)Realloc(vn_list, vn_cnt * sizeof(*vn_list));
		vn_list[n_id] = normal;
	}

	return n_id;
}

float Acos(const float cos_angle)
{
#ifdef _DEBUG
  if( cos_angle > 1.01f || cos_angle < -1.01f)
  {
	  Winprint("Warning: input %f to acos() out of range!\n", cos_angle);
  }
#endif

  if(cos_angle >= 1.0f)
  {
	return 0.0f; //angle = 1.0f;
  }else
  if(cos_angle <= -1.0f)
  {
	return (float)M_PI; //angle = -1.0f;
  }

  return (float)acos((double)cos_angle);
}

inline float GetAngleVec(const Vector & e0, const Vector & e1)
{
	return Acos( (float)(dot_product(e0, e1) / sqrt( e0.magnitude_squared() * e1.magnitude_squared() )) );
}

inline float GetAnglePoint(const Vector & v0, const Vector & v1, const Vector & v2)
{
	return GetAngleVec( v1 - v0, v2 - v0 );
}

float DAMesh::GetAngleWeight( const int gid, const int fid, const int vid,
							  const int vid0, const int vid1, const int vid2 ) const
{
	const DAMeshGroup & group = grp_list[gid];
	if( vid == vid0 )
	{
		return GetAnglePoint( v_list[vid0], v_list[vid1], v_list[vid2] );
	}else
	if( vid == vid1 )
	{
		return GetAnglePoint( v_list[vid1], v_list[vid0], v_list[vid2] );
	}
	else
	{
		assert( vid == vid2 );
		return GetAnglePoint( v_list[vid2], v_list[vid0], v_list[vid1] );
	}
}

void DAMesh::CalcVertexBatchNormals(void)
{
	if( type == DEF_MESH ) // one normal per xyz vertex 
	{
		vn_cnt = v_cnt;
		
		Free(vn_list);
		vn_list = (Vector*)Malloc(vn_cnt * sizeof(*vn_list));
		for(int vid = 0; vid < v_cnt; vid++)
		{
			CalcVertexXYZNormal( vid );
		}

		Free(nb_idx);
		nb_idx = (int*)Malloc(batch_index_cnt * sizeof(*nb_idx));
		memcpy(nb_idx, vb_idx, batch_index_cnt * sizeof(*nb_idx));
	}
	else // one normal per batch index
	{
		vn_cnt = 0;
		Free(vn_list);
		Free(nb_idx);
		nb_idx = (int*)Malloc(batch_index_cnt * sizeof(*nb_idx));
		for(int bid = 0; bid < batch_index_cnt; bid++)
		{
			CalcVertexBatchNormal( bid );
		}
	}
}

void DAMesh::CalcVertexXYZNormal( const int vid )
{
	Vector & v_normal = vn_list[vid];
	v_normal.zero();

	for(int g_id = 0; g_id < grp_cnt; g_id++)
	{
		const DAMeshGroup & fg = grp_list[g_id];
		
		for(int f_id = 0; f_id < fg.f_cnt; f_id++)
		{
			const int vid0 = vb_idx[ fg.f_list[3*f_id+0] ];
			const int vid1 = vb_idx[ fg.f_list[3*f_id+1] ];
			const int vid2 = vb_idx[ fg.f_list[3*f_id+2] ];

			if( vid0 == vid || vid1 == vid || vid2 == vid )
			{
				Vector f_normal;
				CalcFaceGroupNormal(g_id, f_id, f_normal);
				v_normal += GetAngleWeight( g_id, f_id, vid, vid0, vid1, vid2 ) * f_normal;
			}
		}
	}

	DoubleNormalize( v_normal );
}


void DAMesh::CalcVertexBatchNormal( const int bid )
{
	const int vid = vb_idx[bid];
	const int b_smg_id = smgb_idx[bid];
	const int b_n_id = fnb_idx[bid];

	Vector v_normal;
	v_normal.zero();

	for(int g_id = 0; g_id < grp_cnt; g_id++)
	{
		const DAMeshGroup & fg = grp_list[g_id];
		
		for(int f_id = 0; f_id < fg.f_cnt; f_id++)
		{
			const int vid0 = vb_idx[ fg.f_list[3*f_id  ] ];
			const int vid1 = vb_idx[ fg.f_list[3*f_id+1] ];
			const int vid2 = vb_idx[ fg.f_list[3*f_id+2] ];

			if( vid0 == vid || vid1 == vid || vid2 == vid )
			{
																// this face's bid
				if( b_smg_id == 0 && fg.f_list_smg[f_id] == 0 && fg.f_list_flat_n[f_id] == b_n_id ) // flat
				{			
					Vector f_normal;
					CalcFaceGroupNormal(g_id, f_id, f_normal);
#ifdef _DEBUG
					if( !v_normal.equal(Vector(0,0,0),0) )
					{
						Vector tmp_normal ( v_normal );
						tmp_normal.normalize();
						assert( fabs(dot_product(f_normal, tmp_normal) - 1.0f) < .01f || face_cnt != max_face_cnt );
					}
#endif
					v_normal += GetAngleWeight( g_id, f_id, vid, vid0, vid1, vid2 ) * f_normal;
					// we could break here except after LOD started
				}
				else
				if( fg.f_list_smg[f_id] & b_smg_id )
				{
					assert(fg.f_list_flat_n[f_id] == -1 && b_n_id == -1);
					Vector f_normal;
					CalcFaceGroupNormal(g_id, f_id, f_normal);
					v_normal += GetAngleWeight( g_id, f_id, vid, vid0, vid1, vid2 ) * f_normal;
				}
				//else vace uses this xyz but does not participate in this bid's nornmal
			}
		}
	}

	DoubleNormalize( v_normal );

	// insert normal
	nb_idx[bid] = InsertVertexNormal( v_normal );
}

void DAMesh::CalcGroupEdges(void)
{
	bool *used_edge = new bool[e_cnt];

	for(int gid = 0; gid < grp_cnt; gid++)
	{	
		memset(used_edge, 0, e_cnt *sizeof(bool));

		DAMeshGroup & group = grp_list[gid];

		group.e_cnt = 0;
		Free(group.e_list);
		Free(group.e_angles);

		for(int fid = 0; fid < group.f_cnt; fid++)
		{
			const int vid0 = vb_idx[ group.f_list[3*fid  ] ];
			const int vid1 = vb_idx[ group.f_list[3*fid+1] ];
			const int vid2 = vb_idx[ group.f_list[3*fid+2] ];

			int e_id0 = GetEdgeID(vid0, vid1, e_cnt);
			int e_id1 = GetEdgeID(vid1, vid2, e_cnt);
			int e_id2 = GetEdgeID(vid2, vid0, e_cnt);

			if( e_id0 >= 0 && !used_edge[e_id0])
			{
				used_edge[e_id0] = true;
				group.e_cnt++;
			}
			if( e_id1 >= 0 && !used_edge[e_id1])
			{
				used_edge[e_id1] = true;
				group.e_cnt++;
			}
			if( e_id2 >= 0 && !used_edge[e_id2])
			{
				used_edge[e_id2] = true;
				group.e_cnt++;
			}
		}

		group.e_list = (U16*)Malloc(2 * group.e_cnt * sizeof(U16));
		group.e_angles = (float*)Malloc(group.e_cnt * sizeof(float));

		int index = 0;
		for(int e_id = 0; e_id < e_cnt; e_id++)
		{
			if( used_edge[e_id] )
			{
				group.e_list[2*index  ] = e_list[e_id].v1;
				group.e_list[2*index+1] = e_list[e_id].v2;
				group.e_angles[index] = e_list[e_id].angle;

				index++;
			}
		}
		assert(index == group.e_cnt);
	}

	delete [] used_edge;
}

void DAMesh::CalcEdgeAngles(void)
{
	// if all face normals are ~ the same then the edge is considered TRIANGULATING
	for(int e_id = 0; e_id < e_cnt; e_id++)
	{
		// if we only have one face we can't be triangulating
		if(e_list[e_id].face_count == 1)
		{
			e_list[e_id].angle = (float)M_PI;
		}
		else
		{
			e_list[e_id].angle = 0.0f;

			for(int rf_id1=0; rf_id1 < e_list[e_id].face_count-1; rf_id1++)
			{
				int g_id1 = e_list[e_id].group_list[rf_id1];
				int f_id1 = e_list[e_id].face_list[rf_id1];

				Vector n_1;
				CalcFaceGroupNormal(g_id1, f_id1, n_1);
				for(int rf_id2=rf_id1+1; rf_id2 < e_list[e_id].face_count; rf_id2++)
				{
					int g_id2 = e_list[e_id].group_list[rf_id2];
					int f_id2 = e_list[e_id].face_list[rf_id2];
					
					Vector n_2;
					CalcFaceGroupNormal(g_id2, f_id2, n_2);
				
					e_list[e_id].angle =
						_MAX(e_list[e_id].angle, GetAngleVec(n_1, n_2));
				}
			}
		}
	}

	sortQ(e_list, e_cnt, sizeof(Eedge), CompareEdgesByAngle, NULL, SwapEdges, e_list);
}

void DAMesh::CalcEdges(void)
{
	for(int e_id = 0; e_id < max_e_cnt; e_id++)
	{
		e_list[e_id].Release();
	}
	Free(e_list);
	Free(sync_e_list);
	Free(sync_e_angles);
	e_cnt =
	max_e_cnt = 0;

	for(int gid = 0; gid < grp_cnt; gid++)
	{
		const DAMeshGroup & group = grp_list[gid];
		for(int fid = 0; fid < group.f_cnt; fid++)
		{
			const int vid0 = vb_idx[ group.f_list[3*fid  ] ];
			const int vid1 = vb_idx[ group.f_list[3*fid+1] ];
			const int vid2 = vb_idx[ group.f_list[3*fid+2] ];

			InsertEdge(vid0, vid1);
			InsertEdge(vid1, vid2);
			InsertEdge(vid2, vid0);
		}
	}

	for(int eid = 0; eid < e_cnt; eid++)
	{
		SetEdge_Face_UV( eid );
	}
}

void DAMesh::SetEdge_Face_UV( const int e_id )
{
	assert(e_id < e_cnt);

	Eedge & e = e_list[e_id];

	const int vid1 = e.v1;
	const int vid2 = e.v2;

	e.face_count = 0;
	Free(e.face_list);
	Free(e.group_list);
	e.uv_pair_count = 0;
	Free(e.uv_dest);
	Free(e.uv_pairs);

	for(int gid = 0; gid < grp_cnt; gid++)
	{
		const DAMeshGroup *fg = grp_list + gid;
		cq2Mtl * m = ml->cq2Materials[fg->mtl_id];

		for(int fid = 0; fid < fg->f_cnt; fid++)
		{
			const int bid1 = fg->f_list[3*fid];
			const int bid2 = fg->f_list[3*fid+1];
			const int bid3 = fg->f_list[3*fid+2];

			const int v1 = vb_idx[bid1];
			const int v2 = vb_idx[bid2];
			const int v3 = vb_idx[bid3];

			int uv1 = -1;
			int uv2 = -1;
			int e_o_f = 0; // edge of face
			if(vid1 == v1 && vid2 == v2)
			{
				if( uvb0_idx != NULL )
				{
					uv1 = uvb0_idx[bid1];
					uv2 = uvb0_idx[bid2];
				}
				e_o_f = 1;
			}
			else
			if(vid1 == v2 && vid2 == v1)
			{
				if( uvb0_idx != NULL )
				{
					uv1 = uvb0_idx[bid2];
					uv2 = uvb0_idx[bid1];
				}
				e_o_f = -1;
			}
			else
			if(vid1 == v2 && vid2 == v3)
			{
				if( uvb0_idx != NULL )
				{
					uv1 = uvb0_idx[bid2];
					uv2 = uvb0_idx[bid3];
				}
				e_o_f = 2;
			}
			else
			if(vid1 == v3 && vid2 == v2)
			{
				if( uvb0_idx != NULL )
				{
					uv1 = uvb0_idx[bid3];
					uv2 = uvb0_idx[bid2];
				}
				e_o_f = -2;
			}
			else
			if(vid1 == v3 && vid2 == v1)
			{
				if( uvb0_idx != NULL )
				{
					uv1 = uvb0_idx[bid3];
					uv2 = uvb0_idx[bid1];
				}
				e_o_f = 3;
			}
			else
			if(vid1 == v1 && vid2 == v3)
			{
				if( uvb0_idx != NULL )
				{
					uv1 = uvb0_idx[bid1];
					uv2 = uvb0_idx[bid3];
				}
				e_o_f = -3;
			}

			if( e_o_f != 0 )
			{
				// add face & group id's
				bool duplicate = false;
				for(int i = 0; i < e.face_count; i++)
				{
					if(e.group_list[i] == gid && e.face_list[i] == fid)
					{
						duplicate = true;
						break;
					}
				}
				if(!duplicate)
				{
					assert((e.face_count && e.group_list) || (!e.face_count && !e.group_list));
					e.face_count++;
					e.group_list = (int*)Realloc(e.group_list, e.face_count * sizeof(int));
					e.group_list[e.face_count-1] = gid;
					e.face_list = (int*)Realloc(e.face_list, e.face_count * sizeof(int));
					e.face_list[e.face_count-1] = fid;
				}
			}

			// add indices
			if(uv1 >= 0 && uv2 >= 0)
			{
				//add UV id's
				bool duplicate = false;
				for(int i = 0; i < e.uv_pair_count; i++)
				{
					if(e.uv_pairs[i].uvid1 == uv1 && e.uv_pairs[i].uvid2 == uv2 &&
					   e.uv_pairs[i].m == m)
					{
						duplicate = true;
						break;
					}
				}
				if(!duplicate)
				{
					e.uv_pair_count++;
					e.uv_pairs = (uv_mtl*)Realloc(e.uv_pairs, e.uv_pair_count * sizeof(uv_mtl));
					e.uv_pairs[e.uv_pair_count-1].uvid1 = uv1;
					e.uv_pairs[e.uv_pair_count-1].uvid2 = uv2;
					e.uv_pairs[e.uv_pair_count-1].m = m;

					e.uv_dest = (float*)Realloc(e.uv_dest, 2 * e.uv_pair_count * sizeof(float));
				}
			}
		}
	}
}

void DAMesh::SyncEdges(void)
{
	sync_e_list = (int*)Realloc(sync_e_list, 2 * e_cnt * sizeof(int));
	sync_e_angles = (float*)Realloc(sync_e_angles, e_cnt * sizeof(float));
	for(int eid = 0; eid < e_cnt; eid++)
	{
		sync_e_list[2 * eid    ] = e_list[eid].v1;
		sync_e_list[2 * eid + 1] = e_list[eid].v2;
		sync_e_angles[eid] = e_list[eid].angle; 
	}
}

int DAMesh::GetEdgeID(const int vid1, const int vid2, int count) const
{
	assert( count <= max_e_cnt );
	for(int i = 0; i < count; i++)
	{
		if( (e_list[i].v1 == vid1 && e_list[i].v2 == vid2) ||
			(e_list[i].v1 == vid2 && e_list[i].v2 == vid1) )
		{
			return i;
		}
	}

	return -1;
}

int DAMesh::InsertEdge(const int vid1, const int vid2)
{
	assert(vid1 != vid2);
	int e_id = GetEdgeID(vid1, vid2, e_cnt);

	if( e_id < 0 )
	{
		e_id = e_cnt;
		e_cnt++;
		max_e_cnt++;
		e_list = (Eedge*)Realloc(e_list, e_cnt * sizeof(Eedge));
		e_list[e_id].Init();

		bool swapped = (vid1 > vid2);
		e_list[e_id].v1 = (swapped) ? vid2 : vid1;
		e_list[e_id].v2 = (swapped) ? vid1 : vid2;
	}

	return e_id;
}

void DAMesh::Init( mtl_lib * in_ml )
{
	memset(this, 0, sizeof(*this));
	type = INVALID;
	lol.Init();
	ml = in_ml;
}

void DAMesh::Release(void)
{
	for(int i = 0; i < grp_cnt; i++)
	{
		grp_list[i].Release();
	}
	Free(grp_list);

	Free(vb_idx);
	Free(uvb0_idx);
	Free(uvb1_idx);
	Free(colb_idx);
	Free(mtlb_idx);
	Free(smgb_idx);
	Free(fnb_idx);
	Free(nb_idx);

	Free(v_list);
	Free(v_list_api_id);
	Free(v_list_api_node_id);
	Free(frozen_list);
	if( v_err )
	{
		for(int i = 0; i < max_v_cnt; i++)
		{
			v_err[i].Release();
		}
		Free(v_err);
	}
	assert(b_v_list == NULL);
	Free(bone_first_list);
	Free(bone_count_list);
	Free(bone_id_chain);
	Free(bone_weight_chain);

	Free(uv0_list);
	Free(uv0_list_api_id);
	Free(uv1_list);
	Free(uv1_list_api_id);
	Free(vn_list);
	Free(flat_f_n_list);
	Free(e_list);
	Free(sync_e_list);
	Free(sync_e_angles);

	lol.Release();

	Init(NULL);
}

//TODO: sort groups by material,texture, alpha last 
void DAMesh::PostProcess(const float percent, const float _closest, const float _furthest)
{
	CleanUnusedData();
	CalcVertexBatchNormals();
	CalcEdges();
	CalcEdgeAngles();
	CalcGroupEdges();

	lol.closest = _closest;
	lol.furthest = _furthest;
	CollapseEdges( (int)((.01f * percent) * face_cnt + .5f) );
}

// remove second set of uv's or colors if not used/valid
void DAMesh::CleanUnusedData(void)
{
	flat_f_n_cnt = 0;
	Free(flat_f_n_list);

	bool use_uv0 = false;
	bool have_uv0_holes = false;
	for(int i = 0; i < batch_index_cnt; i++)
	{
		if( uvb0_idx[i] >= 0 )
		{
			use_uv0 = true;
		}
		else
		{
			have_uv0_holes = true;
		}
	}
	if( !use_uv0 )
	{
		Free(uvb0_idx);
	}
	else if( have_uv0_holes ) // point holes to .5f
	{
		const int uv_idx = uv0_cnt;
		uv0_cnt++;
		uv0_list = (TexCoord*)Realloc(uv0_list, uv0_cnt * sizeof(*uv0_list));
		uv0_list[uv_idx] = TexCoord( .5f, .5f );

		for(int i = 0; i < batch_index_cnt; i++)
		{
			if( uvb0_idx[i] < 0 )
			{
				uvb0_idx[i] = uv_idx;
			}
		}
	}

	bool use_uv1 = false;
	bool have_uv1_holes = false;
	for(i = 0; i < batch_index_cnt; i++)
	{
		if( uvb1_idx[i] >= 0 )
		{
			use_uv1 = true;
		}
		else
		{
			have_uv1_holes = true;
		}
	}
	if( !use_uv1 )
	{
		Free(uvb1_idx);
	}
	else if( have_uv1_holes ) // point holes to .5f
	{
		const int uv_idx = uv1_cnt;
		uv1_cnt++;
		uv1_list = (TexCoord*)Realloc(uv1_list, uv1_cnt * sizeof(*uv1_list));
		uv1_list[uv_idx] = TexCoord( .5f, .5f );

		for(int i = 0; i < batch_index_cnt; i++)
		{
			if( uvb1_idx[i] < 0 )
			{
				uvb1_idx[i] = uv_idx;
			}
		}
	}

	bool use_col = false;
	bool all_white = true;
	for( i = 0; i < batch_index_cnt; i++)
	{
		if( colb_idx[i] >= 0 )
		{
			use_col = true;

			if( color_list[colb_idx[i]].bgra != 0xFFFFFFFF)
			{
				all_white = false;
				break;
			}
		}
	}
	if( !use_col || all_white )
	{
		Free(colb_idx);

		color_cnt = 0;
		Free(color_list);
		Free(color_list_api_id);
	}
}

int DAMesh::InsertBatchIndex( const RPIndex & rp_idx )
{
	int b_id = Get_BtachID( rp_idx );

	if( b_id < 0 )
	{
		b_id = batch_index_cnt;
		batch_index_cnt++;
		max_batch_index_cnt++;

		vb_idx = (int*)Realloc(vb_idx, batch_index_cnt * sizeof(int));
		uvb0_idx = (int*)Realloc(uvb0_idx, batch_index_cnt * sizeof(int));
		uvb1_idx = (int*)Realloc(uvb1_idx, batch_index_cnt * sizeof(int));
		colb_idx = (int*)Realloc(colb_idx, batch_index_cnt * sizeof(int));
		mtlb_idx = (int*)Realloc(mtlb_idx, batch_index_cnt * sizeof(int));
		smgb_idx = (int*)Realloc(smgb_idx, batch_index_cnt * sizeof(int));
		fnb_idx = (int*)Realloc(fnb_idx, batch_index_cnt * sizeof(int));

		vb_idx[b_id] = rp_idx.v_i;
		uvb0_idx[b_id] = rp_idx.uv_i0;
		uvb1_idx[b_id] = rp_idx.uv_i1;
		colb_idx[b_id] = rp_idx.color_i;
		mtlb_idx[b_id] = rp_idx.mtl_i;
		smgb_idx[b_id] = rp_idx.smg_i;
		fnb_idx[b_id] = rp_idx.n_id;
	}

	return b_id;
}

int DAMesh::Get_BtachID( const RPIndex & rp_idx ) const
{
	for(int i = 0; i < batch_index_cnt; i++)
	{
		if( vb_idx[i] == rp_idx.v_i &&
			uvb0_idx[i] == rp_idx.uv_i0 &&
			uvb1_idx[i] == rp_idx.uv_i1 &&
			colb_idx[i] == rp_idx.color_i &&
			mtlb_idx[i] == rp_idx.mtl_i &&
			smgb_idx[i] == rp_idx.smg_i &&
			fnb_idx[i] == rp_idx.n_id )
		{
			return i;
		}
	}

	return -1;
}

RPIndex DAMesh::InsertVertex( const RPVertex & rp_vtx, const int node_id )
{
	RPIndex rp_idx;
	rp_idx.mtl_i = -1;
	rp_idx.n_id = -1;
	rp_idx.smg_i = -1;

	rp_idx.v_i = InsertXYZ( rp_vtx.pos, rp_vtx.frozen, rp_vtx.api_pos_id, node_id );
	if( rp_vtx.use_uv0 )
	{
		rp_idx.uv_i0 = InsertUV0( rp_vtx.uv0, rp_vtx.api_uv0_id );
	}
	else
	{
		rp_idx.uv_i0 = -1;
	}
	
	if( rp_vtx.use_uv1 )
	{
		rp_idx.uv_i1 = InsertUV1( rp_vtx.uv1, rp_vtx.api_uv1_id );
	}
	else
	{
		rp_idx.uv_i1 = -1;
	}

	if( rp_vtx.use_color )
	{
		rp_idx.color_i = InsertCol( rp_vtx.color, rp_vtx.api_col_id );
	}
	else
	{
		rp_idx.color_i = -1;
	}

	return rp_idx;
}

int DAMesh::GetXYZ_ID( const Vector & pos, const int api_id, const int node_id ) const
{
	if( api_id >= 0 )
	{
		assert( node_id );
		for( int i = 0; i < v_cnt; i++ )
		{
			if( v_list_api_id[i] == api_id && v_list_api_node_id[i] == node_id )
			{
				return i;
			}
		}
	}
	else
	{
		for( int i = 0; i < v_cnt; i++ )
		{
			if( v_list_api_id[i] == -1 && v_list_api_id[i] == 0 && pos.equal( v_list[i], XYZ_TOLERANCE ) )
			{
				return i;
			}
		}
	}

	return -1;
}

int DAMesh::InsertXYZ( const Vector & pos, const Vector & frozen, const int api_id, const int node_id )
{
	int v_id = GetXYZ_ID( pos, api_id, node_id );

	if( v_id < 0 )
	{
		v_id = v_cnt;
		v_cnt++;
		max_v_cnt++;
		v_list = (Vector*)Realloc(v_list, v_cnt * sizeof(*v_list));
		v_list[v_id] = pos;
		frozen_list = (Vector*)Realloc(frozen_list, v_cnt * sizeof(*frozen_list));
		frozen_list[v_id] = frozen;
		v_list_api_id = (int*)Realloc(v_list_api_id, v_cnt * sizeof(*v_list_api_id));
		v_list_api_id[v_id] = api_id;
		v_list_api_node_id = (int*)Realloc(v_list_api_node_id, v_cnt * sizeof(*v_list_api_node_id));
		v_list_api_node_id[v_id] = node_id;
	}

	return v_id;
}

int DAMesh::GetUV0_ID( const TexCoord & uv, const int api_id ) const
{
	if( api_id >= 0 )
	{
		return Get_ID( uv0_list_api_id, uv0_cnt, api_id );
	}
	else
	{
		for( int i = 0; i < uv0_cnt; i++ )
		{
			if( uv0_list_api_id[i] == -1 && uv.equal( uv0_list[i], UV_TOLERANCE ) )
			{
				return i;
			}
		}
	}

	return -1;
}

int DAMesh::GetUV1_ID( const TexCoord & uv, const int api_id ) const
{
	if( api_id >= 0 )
	{
		return Get_ID( uv1_list_api_id, uv1_cnt, api_id );
	}
	else
	{
		for( int i = 0; i < uv1_cnt; i++ )
		{
			if( uv1_list_api_id[i] == -1 && uv.equal( uv1_list[i], UV_TOLERANCE ) )
			{
				return i;
			}
		}
	}

	return -1;
}

int DAMesh::InsertUV0( const TexCoord & uv, const int api_id )
{
	int uv_id = GetUV0_ID( uv, api_id );

	if( uv_id < 0 )
	{
		uv_id = uv0_cnt;
		uv0_cnt++;
		uv0_list = (TexCoord*)Realloc(uv0_list, uv0_cnt * sizeof(*uv0_list));
		uv0_list[uv_id] = uv;
		uv0_list_api_id = (int*)Realloc(uv0_list_api_id, uv0_cnt * sizeof(*uv0_list_api_id));
		uv0_list_api_id[uv_id] = api_id;
	}

	return uv_id;
}

int DAMesh::InsertUV1( const TexCoord & uv, const int api_id )
{
	int uv_id = GetUV1_ID( uv, api_id );

	if( uv_id < 0 )
	{
		uv_id = uv1_cnt;
		uv1_cnt++;
		uv1_list = (TexCoord*)Realloc(uv1_list, uv1_cnt * sizeof(*uv1_list));
		uv1_list[uv_id] = uv;
		uv1_list_api_id = (int*)Realloc(uv1_list_api_id, uv1_cnt * sizeof(*uv1_list_api_id));
		uv1_list_api_id[uv_id] = api_id;
	}

	return uv_id;
}

int DAMesh::GetCol_ID( const Cbgra & color, const int api_id ) const
{
	if( api_id >= 0 )
	{
		return Get_ID( color_list_api_id, color_cnt, api_id );
	}
	else
	{
		for( int i = 0; i < color_cnt; i++ )
		{
			if( color_list_api_id[i] == -1 && color.bgra == color_list[i].bgra )
			{
				return i;
			}
		}
	}

	return -1;
}

int DAMesh::InsertCol( const Cbgra & color, const int api_id )
{
	int col_id = GetCol_ID(color, api_id);

	if( col_id < 0 )
	{
		col_id = color_cnt;
		color_cnt++;
		color_list = (Cbgra*)Realloc(color_list, color_cnt * sizeof(*color_list));
		color_list[col_id] = color;
		color_list_api_id = (int*)Realloc(color_list_api_id, color_cnt * sizeof(*color_list_api_id));
		color_list_api_id[col_id] = api_id;
	}

	return col_id;
}

void DAMesh::InsertFace( const RPFace & face )
{
	if( !face.IsValid() )
	{
		return;
	}

	Vector f_normal;
	if( -1 == CalcNormal( face.verts[0].pos, face.verts[1].pos, face.verts[2].pos, f_normal ) )
	{
		return;
	}

	int grp_id = GetGroupID( face.mtl_id, face.api_node_id );

	if( grp_id < 0 )
	{
		grp_id = grp_cnt;
		grp_cnt++;
		grp_list = (DAMeshGroup*)Realloc(grp_list, grp_cnt * sizeof(*grp_list));
		grp_list[grp_id].Init();
		grp_list[grp_id].mtl_id = face.mtl_id;
		grp_list[grp_id].mtl_name = _strdup( ml->cq2Materials[face.mtl_id]->name);
		grp_list[grp_id].api_node_id = face.api_node_id;
	}

	RPIndex idx0 ( InsertVertex(face.verts[0], face.api_node_id) );
	RPIndex idx1 ( InsertVertex(face.verts[1], face.api_node_id) );
	RPIndex idx2 ( InsertVertex(face.verts[2], face.api_node_id) );

	idx0.mtl_i = idx1.mtl_i = idx2.mtl_i = face.mtl_id;

	int flat_n_id = (face.api_sm_grp == 0 ) ? InsertFlatFNormal( f_normal ) : -1;
	idx0.n_id = idx1.n_id = idx2.n_id = flat_n_id;

	idx0.smg_i = idx1.smg_i = idx2.smg_i = face.api_sm_grp; 

	int bid[3];
	bid[0] = InsertBatchIndex( idx0 );
	bid[1] = InsertBatchIndex( idx1 );
	bid[2] = InsertBatchIndex( idx2 );
	
	int f_id = grp_list[grp_id].InsertFace( bid, face.api_face_id, face.api_sm_grp, flat_n_id );
	face_cnt++;
	max_face_cnt++;
}

int DAMeshGroup::InsertFace( const int bid[3], const int api_id, const int sm_gid, const int flat_n_id )
{	
	const int f_id = f_cnt;
	f_cnt++;
	max_f_cnt++;

	f_list = (U16*)Realloc(f_list, 3 * f_cnt * sizeof(U16));
	f_list_api_id = (int*)Realloc(f_list_api_id, f_cnt * sizeof(int));
	f_list_smg = (int*)Realloc(f_list_smg, f_cnt * sizeof(int));
	f_list_flat_n = (int*)Realloc(f_list_flat_n, f_cnt * sizeof(int));
	
	f_list[3*f_id  ] = bid[0];
	f_list[3*f_id+1] = bid[1];
	f_list[3*f_id+2] = bid[2];
	f_list_api_id[f_id] = api_id;
	f_list_smg[f_id] = sm_gid; //( sm_gid ) ? sm_gid : -1;
	f_list_flat_n[f_id] = flat_n_id;

	return f_id;
}

int DAMesh::Get_ID( const int *list, const int length, const int value ) const
{
	assert( value >= 0 );
	for(int i = 0;  i < length; i++)
	{
		if( list[i] == value )
		{
			return i;
		}
	}

	return -1;
}

void DAMeshGroup::Init(void)
{
	memset(this, 0, sizeof(*this));
	mtl_id = -1;
	api_node_id = 0;
}

void DAMeshGroup::Release(void)
{
	Free(f_list);
	Free(f_list_api_id);
	Free(f_list_smg);
	Free(f_list_flat_n);
	Free(mtl_name);
	Free(quadric_list);

	Free(e_list);
	Free(e_angles);
	
	Init();
}

void DAMesh::CollapseEdges( const int target_face_cnt )
{
	if(face_cnt == target_face_cnt)
		return;

	TrapFpu(true);
	assert(face_cnt > target_face_cnt);

#ifdef _DEBUG
	assert(max_face_cnt == face_cnt);
	assert(grp_list);
	int fg_id;
	for(fg_id=0; fg_id<grp_cnt; fg_id++)
	{
		assert(grp_list[fg_id].max_f_cnt == grp_list[fg_id].f_cnt);
	}
	assert(batch_index_cnt == max_batch_index_cnt);
    assert(v_cnt == max_v_cnt);
    assert(e_cnt == max_e_cnt);
#endif

    CalcEdgeErrors(); // sorts

	//checksorting(&(obj->el));

	while(face_cnt > target_face_cnt)
    {
		RemoveLastEdge();
    }

	// necessary to create edge_chain and get counts back up
	SplitVertices();

	AddUVSpaces();  // to allow runtime UV interpolation

#ifdef _DEBUG
	for(fg_id=0; fg_id<grp_cnt; fg_id++)
	{
		assert(grp_list[fg_id].max_f_cnt == grp_list[fg_id].f_cnt);
	}
	assert(max_batch_index_cnt == batch_index_cnt);
	assert(max_v_cnt == v_cnt);
	assert(max_e_cnt == e_cnt);

	// sanity check
	for(int i=0; i < lol.count; i++)
	{
		assert(lol.step_list[i].vid1 < lol.step_list[i].vid2);
	}
#endif

	lol.ReverseStepList();

	CalcEdges();
	CalcEdgeAngles();
	CalcGroupEdges();

	TrapFpu(false);
}

void DAMesh::CalcEdgeErrors(void)
{
	CalcQuadrics();

	// compute errors
	for(int e_id = 0; e_id < e_cnt; e_id++)
	{
		e_list[e_id].error = QuadricEdgeError(e_id) * UVEdgeError(e_id);
	}

	sortQ(e_list, e_cnt, sizeof(Eedge), CompareEdgesByError, NULL, SwapEdges, e_list);
}

void DAMesh::CalcQuadrics(void)
{

#ifdef _DEBUG
	int tmp_count = 0;
	for(int i = 0; i < grp_cnt; i++)
	{
		tmp_count += grp_list[i].f_cnt;
	}
	assert(tmp_count == face_cnt);
#endif

	// compute initial quadric for each face/plane
	{
		for(int g_id = 0; g_id < grp_cnt; g_id++)
		{
			DAMeshGroup *fg = grp_list + g_id;

			fg->quadric_list = (Matrix4*)Realloc(fg->quadric_list, fg->f_cnt * sizeof(Matrix4));

			for(int f_id=0; f_id < fg->f_cnt; f_id++)
			{
				Matrix4 *m = fg->quadric_list + f_id;

				int vb1 = fg->f_list[3*f_id];
				int vb2 = fg->f_list[3*f_id+1];
				int vb3 = fg->f_list[3*f_id+2];

				vb1 = vb_idx[vb1];
				vb2 = vb_idx[vb2];
				vb3 = vb_idx[vb3];

				// we recompute normals since we want them to be accurate (0 tolerance)
				Vector normal;
				int n_success = CalcNormal(v_list[vb1],
										   v_list[vb2],
										   v_list[vb3], 
										   normal);
				if(n_success == -1)
				{
					Winprint("Error: bad normal!\n");
				}

				float D = ( -dot_product(v_list[vb1], normal) -
							 dot_product(v_list[vb2], normal) -
							 dot_product(v_list[vb3], normal) ) * (1.0f / 3.0f);

				m->quadric(normal.x, normal.y, normal.z, D);
			}
		}
	}


	{
		// initialize vertex error struct
		v_err = (vertex_error*)Realloc(v_err, v_cnt * sizeof(vertex_error));

		for(int vi = 0; vi < v_cnt; vi++)
		{
			v_err[vi].Init();
		}
	}

	// add face contribution to each vertex
	{
		for(int g_id = 0; g_id < grp_cnt; g_id++)
		{
			DAMeshGroup *fg = grp_list + g_id;

			for(int f_id=0; f_id < fg->f_cnt; f_id++)
			{
				int vb1 = fg->f_list[3*f_id];
				int vb2 = fg->f_list[3*f_id+1];
				int vb3 = fg->f_list[3*f_id+2];

				vb1 = vb_idx[vb1];
				vb2 = vb_idx[vb2];
				vb3 = vb_idx[vb3];

				assert(vb1 < v_cnt);
				assert(vb2 < v_cnt);
				assert(vb3 < v_cnt);

				// TODO: get rid of plane_list and just sum quadrics
				AddVertexQuadric(vb1, g_id, f_id);
				AddVertexQuadric(vb2, g_id, f_id);
				AddVertexQuadric(vb3, g_id, f_id);		
			}
		}
	}

	{	// add perpendicular planes to open boundary edges; or material boundaries

		for(int e_id=0; e_id < e_cnt; e_id++)
		{
			const Eedge & e = e_list[e_id];

			assert (e.face_count > 0);

			bool mat_boundary = false;
			int first_g = e.group_list[0];
			for(int f_id=1; f_id < e.face_count; f_id++)
			{
				if(e.group_list[f_id] != first_g)
				{
					mat_boundary = true;
					break;
				}
			}

			if(e.face_count == 1 || mat_boundary)
			{
				for(int ef_id=0; ef_id < e.face_count; ef_id++)
				{
					// compute perpendicular plane
					int g_id = e.group_list[ef_id];
					int f_id = e.face_list[ef_id];

					DAMeshGroup *fg = grp_list + g_id;
					
					int vb1 = fg->f_list[3*f_id];
					int vb2 = fg->f_list[3*f_id+1];
					int vb3 = fg->f_list[3*f_id+2];

					vb1 = vb_idx[vb1];
					vb2 = vb_idx[vb2];
					vb3 = vb_idx[vb3];
					
					assert(e.v1 == vb1 || e.v1 == vb2 || e.v1 == vb3);
					assert(e.v2 == vb1 || e.v2 == vb2 || e.v2 == vb3);

					/// we recompute normals since we want them to be accurate (0 tolerance)
					const Vector old_normal = GetNormal( v_list[vb1], v_list[vb2], v_list[vb3] );
					

					const Vector ve = v_list[e.v2] - v_list[e.v1];
					
					Vector new_normal = cross_product(old_normal, ve);
					DoubleNormalize(new_normal);

					const float D = (-dot_product(v_list[e.v1], new_normal) -
									  dot_product(v_list[e.v2], new_normal) ) * .5f;
					Matrix4 q;
					q.quadric(new_normal, D);

					if(mat_boundary)
					{
						q *= lod_mtl_weight;
					}
					else
					{
						q *= 400.0f;
					}

					v_err[e.v1].quadric += q;
					v_err[e.v2].quadric += q;
				}
			}
		}
	}
}

inline float new_lock_v(const float x1, const float x2, const float x3,
						const float l1, const float l2)
{
	// NOTE: could use higher power than 2
	if(l1 + l2 > 0.0f)
	{
		const float l1_sq = l1 * l1;
		const float l2_sq = l2 * l2;

		return (l1_sq * (l1 * x1 + (1.0f - l1) * x3) + l2_sq * (l2 * x2 + (1.0f - l2) * x3) ) / (l1_sq + l2_sq);
	}
	else
	{
		return x3;
	}
}

inline float CalcPenalty(const float l1, const float l2)
{
	if( (l1 >= 1.0f && l2 > 0.0f) || (l2 >= 1.0f && l1 > 0.0f) )
	{
		// note l1 or l2 will be 1.0 so it's just the min(l1, l2) * 1000000
		return 1000000.0f * l1 * l2;
		//return 1000000.0f * l1 * l1 * l2 * l2;
	}
	else
	{
		return 0.0f;
	}
}

float DAMesh::QuadricEdgeError(const int e_id)
{
	assert(e_id < e_cnt);

	const int vid1 = e_list[e_id].v1;
	const int vid2 = e_list[e_id].v2;
	assert(vid1 != vid2);

	int total_plane_count;
    Matrix4 q ( MergeQuadrics(vid1, vid2, false, &total_plane_count) );

	float current_uv_weight = lod_uv_weight;

	Vector4 *new_v = &(e_list[e_id].v_dest); //el.v_list + e_id;

	const Vector & v1 = v_list[vid1];
	const Vector & v2 = v_list[vid2];
	const Vector & fr1 = frozen_list[vid1];
	const Vector & fr2 = frozen_list[vid2];

	float path_length = FLT_MAX;
	float quad_error = FLT_MAX;
	float min_quad_error = FLT_MAX;
	float error = FLT_MAX;
	float step;

	Vector4 midpoint_v(.5f*(v1.x + v2.x), .5f*(v1.y + v2.y), .5f*(v1.z + v2.z), 1.0f);
	
	Vector4 quad_v;
	if( GetNewV(q, &quad_v) ) // inversion worked
	{
		step = 0.0f;
		//assert(fabs(1.0f - new_v->w) < .001f);
		//new_v->x /= new_v->w;
		//new_v->y /= new_v->w;
		//new_v->z /= new_v->w;
		//new_v->w = 1.0f;
	}
	else
	{
		quad_v = Vector4(0.0f, 0.0f, 0.0f, 1.0f);
		step = 1.0f;
	}


	float frozen_error_multiplier = 1.0f;
	// interpolate from calculated point to midpoint (if inversion failed just use midpoint)
	for(; step < 1.00001f; step += 0.1f)
	{
		Vector4 tmp_v ( (1.0f - step) * quad_v + step * midpoint_v);		

		tmp_v.x = new_lock_v( v1.x, v2.x, tmp_v.x, fr1.x, fr2.x );
		tmp_v.y = new_lock_v( v1.y, v2.y, tmp_v.y, fr1.y, fr2.y );
		tmp_v.z = new_lock_v( v1.z, v2.z, tmp_v.z, fr1.z, fr2.z );

		frozen_error_multiplier +=
			CalcPenalty( fr1.x, fr2.x ) +
			CalcPenalty( fr1.y, fr2.y ) +
			CalcPenalty( fr1.z, fr2.z );

		// original guess gets a slight preference
		quad_error = GetQuadricVectorErr(tmp_v, q);

		if(quad_error == 0.0f || !is_float( quad_error ) )
		{
			//quad_error = FLT_MAX / 1e5f;
			Winprint("Warning: Bad quadric error %f!\n", quad_error);
		}	

		Vector tmp_vec;
		tmp_vec.x = tmp_v.x - v1.x;
		tmp_vec.y = tmp_v.y - v1.y;
		tmp_vec.z = tmp_v.z - v1.z;
		const float dist1_sq = tmp_vec.magnitude_squared();
		tmp_vec.x = tmp_v.x - v2.x;
		tmp_vec.y = tmp_v.y - v2.y;
		tmp_vec.z = tmp_v.z - v2.z;
		const float dist2_sq = tmp_vec.magnitude_squared();
		path_length = (float)sqrt(dist1_sq + dist2_sq);

		const float tmp_error =
			(float)pow(quad_error, (1.0f - current_uv_weight)) * (float)pow(path_length, current_uv_weight);
		

		if(quad_error < min_quad_error)
		{
			min_quad_error = quad_error;
			error = tmp_error;
			*new_v = tmp_v;
		}
	}

	error *= frozen_error_multiplier;
		
	assert(error < FLT_MAX);


	// 1) check for flipped normals and penalize * 50
	// 2) check for orphan vertices and penalize * 8
	// TODO: keep correct track of faces in each edge as we progress
	// and use it here instead of searching each time
	{
		for(int g_id=0; g_id < grp_cnt; g_id++)
		{
			const DAMeshGroup *fg = grp_list + g_id;

			for(int f_id=0; f_id < fg->f_cnt; f_id++)
			{
				int vb1 = fg->f_list[3*f_id];
				int vb2 = fg->f_list[3*f_id+1];
				int vb3 = fg->f_list[3*f_id+2];

				vb1 = vb_idx[vb1];
				vb2 = vb_idx[vb2];
				vb3 = vb_idx[vb3];

				int swap_id = -1;

				if( (vb1 == vid1 || vb2 == vid1 || vb3 == vid1) &&
					(vb1 == vid2 || vb2 == vid2 || vb3 == vid2) )
				{
					// check for orphan vertices (can still be used by two opposite faces)
					if( (vb1 != vid1) && (vb1 != vid2) )
					{
						if( IsVertexUsed_N_or_Less_Times(vb1, 2) )
						{
							error *= 8;
							//goto label1;
						}
					}
					else
					if( (vb2 != vid1) && (vb2 != vid2) )
					{
						if( IsVertexUsed_N_or_Less_Times(vb2, 2) )
						{
							error *= 8;
							//goto label1;
						}
					}
					else
					if( (vb3 != vid1) && (vb3 != vid2) )
					{
						if( IsVertexUsed_N_or_Less_Times(vb3, 2) )
						{
							error *= 8;
							//goto label1;
						}
					}
					else
					{
						Winprint("Error: IsUniqueVertex out of synch!");
					}
				}
				else
				if( vb1 == vid1 || vb2 == vid1 || vb3 == vid1 )
				{
					swap_id = vid1;
				}
				else
				if( vb1 == vid2 || vb2 == vid2 || vb3 == vid2 )
				{
					swap_id = vid2;
				}

				
				if(swap_id != -1)
				{
					Vector old_normal;
					Vector new_normal;		
					
					float tmp_v[3] = {new_v->x, new_v->y, new_v->z}; 

					int succes1 = CalcFaceGroupNormal(g_id, f_id, old_normal);
					if(succes1 != -1)
					{
						MemSwap(tmp_v, v_list + swap_id, 3 * sizeof(float));
						int success2 = CalcFaceGroupNormal(g_id, f_id, new_normal);
						MemSwap(tmp_v, v_list + swap_id, 3 * sizeof(float));

						if(success2 != -1 && succes1 != -1)
						{					
							float dot = dot_product(new_normal, old_normal);

							if(dot < -0.98f) // 180
							{
								error *= 50.0f;  // penalty fro flipping normals
								goto label1;
							}
						}
					}	
				}
		
			}
		}
	}

label1:;

	return error;
}

float DAMesh::UVEdgeError(const int e_id) const
{
	assert(e_id < e_cnt);

//return 1.0f;

	if( uv0_list == NULL )
		return 1.0f;

	const Eedge & e = e_list[e_id];
	const int vid1 = e.v1;
	const int vid2 = e.v2;
	assert(vid1 != vid2);
	
	// calculate resulting uv's
	const Vector & dest_v = *(Vector*)&(e.v_dest);

	{
		const Vector & v1 = v_list[vid1];
		const Vector & v2 = v_list[vid2];
			
		const float dist = (v2 - v1).magnitude();
		const float d1 = dot_product( (dest_v - v1), (v2 - v1) / dist );
		const float d2 = dot_product( (dest_v - v2), (v1 - v2) / dist );

		float fraction2 = d1 / dist;
		float fraction1 = d2 / dist;

		float inv_mag = 1.0f / (fraction1 + fraction2);
		fraction1 *= inv_mag;
		fraction2 *= inv_mag;

		// clamp
		if(fraction1 > 1.0f)
		{
			fraction1 = 1.0f;
		}
		else
		if(fraction1 < 0.0f)
		{
			fraction1 = 0.0f;
		}

		if(fraction2 > 1.0f)
		{
			fraction2 = 1.0f;
		}
		else
		if(fraction2 < 0.0f)
		{
			fraction2 = 0.0f;
		}

		inv_mag = 1.0f / (fraction1 + fraction2);
		fraction1 *= inv_mag;
		fraction2 *= inv_mag;

		for(int i = 0; i < e.uv_pair_count; i++)
		{
			if(e.uv_pairs[i].uvid1 == e.uv_pairs[i].uvid2)
			{
				e.uv_dest[2*i] =   uv0_list[e.uv_pairs[i].uvid1].u;
				e.uv_dest[2*i+1] = uv0_list[e.uv_pairs[i].uvid1].v;
			}
			else
			{
				e.uv_dest[2*i] = fraction1 * uv0_list[e.uv_pairs[i].uvid1].u +
								 fraction2 * uv0_list[e.uv_pairs[i].uvid2].u;
				e.uv_dest[2*i+1] = fraction1 * uv0_list[e.uv_pairs[i].uvid1].v +
								   fraction2 * uv0_list[e.uv_pairs[i].uvid2].v;
			}
		}
	}

	float error = 1.0f;
	float penalty = 0.1f;
	float decrement = 0.5f;

	// see if there are any faces whose uv's can't be interpolated and penalize them
	for(int gid = 0; gid < grp_cnt; gid++)
	{
		const DAMeshGroup *fg = grp_list + gid;
		const cq2Mtl *m = ml->cq2Materials[fg->mtl_id];

		for(int fid = 0; fid < fg->f_cnt; fid++)
		{
			const int bid1 = fg->f_list[3*fid];
			const int bid2 = fg->f_list[3*fid+1];
			const int bid3 = fg->f_list[3*fid+2];

			const int v_id1 = vb_idx[bid1];
			const int v_id2 = vb_idx[bid2];
			const int v_id3 = vb_idx[bid3];

			const int uv_id1 = uvb0_idx[bid1];
			const int uv_id2 = uvb0_idx[bid2];
			const int uv_id3 = uvb0_idx[bid3];

			bool used = false;
			if(v_id1 == vid1)
			{
				for(int i = 0; i < e.uv_pair_count; i++)
				{
					if(e.uv_pairs[i].uvid1 == uv_id1 && e.uv_pairs[i].m == m)
					{
						used = true;
						break;
					}
				}

				if(!used)
				{
					error += penalty;
					penalty *= decrement;
				}
			}
			else
			if(v_id1 == vid2)
			{
				for(int i = 0; i < e.uv_pair_count; i++)
				{
					if(e.uv_pairs[i].uvid2 == uv_id1 && e.uv_pairs[i].m == m)
					{
						used = true;
						break;
					}
				}

				if(!used)
				{
					error += penalty;
					penalty *= decrement;
				}
			}

			used = false;
			if(v_id2 == vid1)
			{
				for(int i = 0; i < e.uv_pair_count; i++)
				{
					if(e.uv_pairs[i].uvid1 == uv_id2 && e.uv_pairs[i].m == m)
					{
						used = true;
						break;
					}
				}

				if(!used)
				{
					error += penalty;
					penalty *= decrement;
				}
			}
			else
			if(v_id2 == vid2)
			{
				for(int i = 0; i < e.uv_pair_count; i++)
				{
					if(e.uv_pairs[i].uvid2 == uv_id2 && e.uv_pairs[i].m == m)
					{
						used = true;
						break;
					}
				}

				if(!used)
				{
					error += penalty;
					penalty *= decrement;
				}
			}

			used = false;
			if(v_id3 == vid1)
			{
				for(int i = 0; i < e.uv_pair_count; i++)
				{
					if(e.uv_pairs[i].uvid1 == uv_id3 && e.uv_pairs[i].m == m)
					{
						used = true;
						break;
					}
				}

				if(!used)
				{
					error += penalty;
					penalty *= decrement;
				}
			}
			else
			if(v_id3 == vid2)
			{
				for(int i = 0; i < e.uv_pair_count; i++)
				{
					if(e.uv_pairs[i].uvid2 == uv_id3 && e.uv_pairs[i].m == m)
					{
						used = true;
						break;
					}
				}

				if(!used)
				{
					error += penalty;
					penalty *= decrement;
				}
			}
		}
	}

	return error;
}

void DAMesh::AddVertexQuadric(int vid, int gid, int fid)
{
	vertex_error *err = v_err + vid;

	for(int i=0; i < err->plane_count; i++)
	{
		if( (err->plane_face_list[i] == fid) && (err->plane_group_list[i] == gid) )
		{
			Winprint("Error: duplicate face/vertex quadric!\n");
			exit(1);
		}
	}

	err->plane_count++;
	err->plane_face_list = (int*)Realloc(err->plane_face_list, err->plane_count * sizeof(int));
	err->plane_face_list[err->plane_count - 1] = fid;

	err->plane_group_list = (int*)Realloc(err->plane_group_list, err->plane_count * sizeof(int));
	err->plane_group_list[err->plane_count - 1] = gid;

	const DAMeshGroup *fg = grp_list + gid;
	err->quadric += fg->quadric_list[fid];
}

Matrix4 DAMesh::MergeQuadrics(int vid1, int vid2, bool add_lists, int *sum_count)
{
	Matrix4 q (v_err[vid1].quadric + v_err[vid2].quadric);

	      vertex_error *err1 = v_err + vid1;
	const vertex_error *err2 = v_err + vid2;

	int original_count1 = err1->plane_count;
	if(sum_count)
		*sum_count = err1->plane_count + err2->plane_count;

	for(int i=0; i < err2->plane_count; i++)
	{
		bool hit = false;
		for(int j=0; j < original_count1; j++)
		{
			if((err2->plane_group_list[i] == err1->plane_group_list[j]) &&
			   (err2->plane_face_list[i]  == err1->plane_face_list[j]) )
			{
				const DAMeshGroup *fg = grp_list + err2->plane_group_list[i];
				q -= fg->quadric_list[ err2->plane_face_list[i] ];

				if(sum_count)
					(*sum_count)--;

				hit = true;
				break;
			}	
		}

		if(!hit && add_lists)
		{	
			err1->plane_count++;

			err1->plane_group_list = (int*)Realloc(err1->plane_group_list,
				err1->plane_count * sizeof(int));
			err1->plane_group_list[err1->plane_count - 1] = err2->plane_group_list[i];

			err1->plane_face_list = (int*)Realloc(err1->plane_face_list,
				err1->plane_count * sizeof(int));
			err1->plane_face_list[err1->plane_count - 1] = err2->plane_face_list[i];
		}
	}

	if(sum_count && add_lists)
	{
		assert(*sum_count == err1->plane_count);
	}

	return q;
}

bool DAMesh::IsVertexUsed_N_or_Less_Times(const int vid, const int t_count) const
{
	int count = 0;

	for(int fg_id=0; fg_id < grp_cnt; fg_id++)
	{
		const DAMeshGroup *fg = grp_list + fg_id;

		const U16 *bc_pt = fg->f_list;
		const U16 *end_pt = fg->f_list + 3 * fg->f_cnt;

		for( ; bc_pt < end_pt; bc_pt++)
		{
			if(vb_idx[*bc_pt] == vid)
			{
				count++;
				if(count > t_count)
					return false;
			}
		}
	}

	return true;
}

bool DAMesh::IsBatchUsed_N_or_Less_Times(const int vbid, const int t_count)
{
	int count = 0;

	for(int fg_id=0; fg_id < grp_cnt; fg_id++)
	{
		const DAMeshGroup *fg = grp_list + fg_id;

		for(int f_id=0; f_id < 3 * fg->f_cnt; f_id++)
		{
			if( fg->f_list[f_id] == vbid )
			{
				count++;
				if(count > t_count)
					return false;
			}
		}
	}

	return true;
}

int DAMesh::CalcFaceGroupNormal(const int g_id, const int f_id, Vector & normal) const
{
	const DAMeshGroup *fg = grp_list + g_id;

	int v_id1 = vb_idx[ fg->f_list[3*f_id  ] ];
	int v_id2 = vb_idx[ fg->f_list[3*f_id+1] ];
	int v_id3 = vb_idx[ fg->f_list[3*f_id+2] ];

	return CalcNormal(v_list[v_id1],
					  v_list[v_id2],
					  v_list[v_id3],
					  normal);
}

bool GetNewV(Matrix4 quadric, Vector4 *new_v) 
{
	quadric.d[3][0] = quadric.d[3][1] = quadric.d[3][2] = 0.0f;
	quadric.d[3][3] = 1.0f;

	bool result = false;

	if(quadric.inverse())
	{
		*new_v = quadric * Vector4(0.0f, 0.0f, 0.0f, 1.0f);
		result = true;
	}
	
	return result;
}

void DAMesh::RemoveLastEdge(void)
{
	const Eedge & e = e_list[e_cnt - 1];

	// save lod data in these
    TriLODStep lstep;
    lstep.Init();

    const int v_id1 = (e.v1 < e.v2)
                       ? e.v1 : e.v2;
          int v_id2 = (e.v1 < e.v2)
                       ? e.v2 : e.v1;

    assert(v_id1 != v_id2);

    // swap second vertex for last vertex & reindex object
    SwapVertices(v_id2, v_cnt - 1); 

    v_id2 = v_cnt - 1;
    
    assert(v_id1 != v_id2);

    // store v_id1 & v_id2 data in lod_step
    lstep.vid1 = v_id1;
    lstep.v1 = v_list[v_id1];

    lstep.vid2 = v_id2;
    lstep.v2 = v_list[v_id2];
	
	// assign new vertex position
	v_list[v_id1].x = lstep.v3.x = e.v_dest.x;
	v_list[v_id1].y = lstep.v3.y = e.v_dest.y;
	v_list[v_id1].z = lstep.v3.z = e.v_dest.z;

	// sum up freeze weights
	frozen_list[v_id1].x = Clamp01( new_lock( frozen_list[v_id1].x, frozen_list[v_id2].x ) );
	frozen_list[v_id1].y = Clamp01( new_lock( frozen_list[v_id1].y, frozen_list[v_id2].y ) );
	frozen_list[v_id1].z = Clamp01( new_lock( frozen_list[v_id1].z, frozen_list[v_id2].z ) ); 

	v_err[v_id1].quadric = MergeQuadrics(v_id1, v_id2, true, NULL);

	v_cnt--;

	bool *potential_batch_list = (bool*)Malloc(batch_index_cnt * sizeof(bool));
	memset(potential_batch_list, 0, batch_index_cnt * sizeof(bool));


	// move deleted faces to the end
	for(int g_id=0; g_id < grp_cnt; g_id++)
	{
		DAMeshGroup *fg = grp_list + g_id;
		for(int f_id = 0; f_id < fg->f_cnt; f_id++)
		{
f_label:
			const int bid1 = fg->f_list[3*f_id];
			const int bid2 = fg->f_list[3*f_id + 1];
			const int bid3 = fg->f_list[3*f_id + 2];

			const int f_vid1 = vb_idx[ bid1 ];
			const int f_vid2 = vb_idx[ bid2 ];
			const int f_vid3 = vb_idx[ bid3 ];
	
			// batch indices that point to interpolated uv's
			int uv_b_id1 = -1;
			int uv_b_id2 = -1;

			if( f_vid1 == v_id1 && f_vid2 == v_id2 )
			{
				uv_b_id1 = bid1;
				uv_b_id2 = bid2;
			}else
			if( f_vid1 == v_id2 && f_vid2 == v_id1 )
			{
				uv_b_id1 = bid2;
				uv_b_id2 = bid1;
			}
			else
			if( f_vid1 == v_id1 && f_vid3 == v_id2 )
			{
				uv_b_id1 = bid1;
				uv_b_id2 = bid3;
			}else
			if( f_vid1 == v_id2 && f_vid3 == v_id1 )
			{
				uv_b_id1 = bid3;
				uv_b_id2 = bid1;
			}else
			if( f_vid2 == v_id1 && f_vid3 == v_id2 )
			{
				uv_b_id1 = bid2;
				uv_b_id2 = bid3;
			}else
			if( f_vid2 == v_id2 && f_vid3 == v_id1 )
			{
				uv_b_id1 = bid3;
				uv_b_id2 = bid2;
			}

			if(uv_b_id1 != -1)
			{
				assert(uv_b_id2 != -1);
				assert(uv_b_id1 != uv_b_id2);

				active_group = g_id;
				SwapLodFaces(f_id, fg->f_cnt - 1);

				potential_batch_list[ fg->f_list[ 3 * (fg->f_cnt - 1) ] ] = true;
				potential_batch_list[ fg->f_list[ 3 * (fg->f_cnt - 1) + 1] ] = true;
				potential_batch_list[ fg->f_list[ 3 * (fg->f_cnt - 1) + 2] ] = true;

				fg->f_cnt--;
				face_cnt--;

				// store the group id of the removed face
				if(lstep.first_removed_face == -1)
				{
					lstep.first_removed_face = lol.removed_face_count;
				}

				lstep.removed_face_count++;
				lol.removed_face_count++;
				lol.removed_face_list = (int*)Realloc(lol.removed_face_list,
					lol.removed_face_count * sizeof(int));
				lol.removed_face_list[lol.removed_face_count - 1] = g_id;

                if(f_id != fg->f_cnt)
                {
                    goto f_label;
                }
                else // face is last already so we are done
                {
                    break;
                }
			}
		}
	}

	// UV stuff
	int uv_b_count1;
	int uv_b_count2;
	int *uv_bid1 = NULL;
	int *uv_bid2 = NULL;
	for(int pid = 0; pid < e.uv_pair_count; pid++)
	{
		uv_b_count1 = 0;
		uv_b_count2 = 0;

		const int uvid1 = e.uv_pairs[pid].uvid1;
		const int uvid2 = e.uv_pairs[pid].uvid2;
		//assert(uvid1 != uvid2);
		const cq2Mtl *m = e.uv_pairs[pid].m;

		// get batch indices for this uv pair
		for(int bid = 0; bid < batch_index_cnt; bid++)
		{	
			if(vb_idx[bid] == v_id1 && ml->cq2Materials[mtlb_idx[bid]] == m)
			{
				if(uvb0_idx[bid] == uvid1)
				{
					uv_b_count1++;
					uv_bid1 = (int*)Realloc(uv_bid1, uv_b_count1 * sizeof(int));
					uv_bid1[uv_b_count1 - 1] = bid;
				}
			}
			else
			if(vb_idx[bid] == v_id2 && ml->cq2Materials[mtlb_idx[bid]] == m)
			{
				if(uvb0_idx[bid] == uvid2)
				{
					uv_b_count2++;
					uv_bid2 = (int*)Realloc(uv_bid2, uv_b_count2 * sizeof(int));
					uv_bid2[uv_b_count2 - 1] = bid;
				}
			}
		}

		
		// insert lod uv pair
		if(lstep.first_morphed_uv == -1)
		{
			assert(lstep.morphed_uv_count == 0);
			lstep.first_morphed_uv = lol.uv_count;
		}
		lstep.morphed_uv_count++;

		lol.uv_count++;
		

		// insert new uv
		lol.low_uv_id = (int*)Realloc(lol.low_uv_id, lol.uv_count * sizeof(int));
		lol.low_uv_id[lol.uv_count - 1] = InsertUV0( TexCoord(e.uv_dest[2*pid], e.uv_dest[2*pid+1]), -1 ); //InsertUVVertex(v, &(e.uv_dest[2*pid]), 0, -1, 0);

	
		lol.high_uv_id1 = (int*)Realloc(lol.high_uv_id1 , lol.uv_count * sizeof(int));
		lol.high_uv_id1[lol.uv_count - 1] = uvid1;

		lol.high_batch_count1 = (int*)Realloc(lol.high_batch_count1 , lol.uv_count * sizeof(int));
		lol.high_batch_count1[lol.uv_count - 1] = 0;
		lol.high_batch_first1 = (int*)Realloc(lol.high_batch_first1 , lol.uv_count * sizeof(int));
		lol.high_batch_first1[lol.uv_count - 1] = -1;

		for(int j = 0; j < uv_b_count1; j++)
		{
			// assign new uv
			uvb0_idx[ uv_bid1[j] ] = lol.low_uv_id[lol.uv_count - 1];

			// store changed batch locations
			if(lol.high_batch_first1[lol.uv_count - 1] == -1)
			{
				lol.high_batch_first1[lol.uv_count - 1] = lol.uv_chain_count;
			}
			lol.high_batch_count1[lol.uv_count - 1]++;

			lol.uv_chain_count++;
			lol.batch_uv_id_chain = (int*)Realloc(lol.batch_uv_id_chain, lol.uv_chain_count * sizeof(int));
			lol.batch_uv_id_chain[lol.uv_chain_count - 1] = uv_bid1[j];
		}
		assert(lol.high_batch_count1[lol.uv_count - 1] == uv_b_count1);


		lol.high_uv_id2 = (int*)Realloc(lol.high_uv_id2 , lol.uv_count * sizeof(int));
		lol.high_uv_id2[lol.uv_count - 1] = uvid2;

		lol.high_batch_count2 = (int*)Realloc(lol.high_batch_count2 , lol.uv_count * sizeof(int));
		lol.high_batch_count2[lol.uv_count - 1] = 0;
		lol.high_batch_first2 = (int*)Realloc(lol.high_batch_first2 , lol.uv_count * sizeof(int));
		lol.high_batch_first2[lol.uv_count - 1] = -1;

		for(j = 0; j < uv_b_count2; j++)
		{
			// assign new uv
			uvb0_idx[ uv_bid2[j] ] = lol.low_uv_id[lol.uv_count - 1];

			// store changed batch locations
			if(lol.high_batch_first2[lol.uv_count - 1] == -1)
			{
				lol.high_batch_first2[lol.uv_count - 1] = lol.uv_chain_count;
			}
			lol.high_batch_count2[lol.uv_count - 1]++;

			lol.uv_chain_count++;
			lol.batch_uv_id_chain = (int*)Realloc(lol.batch_uv_id_chain, lol.uv_chain_count * sizeof(int));
			lol.batch_uv_id_chain[lol.uv_chain_count - 1] = uv_bid2[j];
		}
		assert(lol.high_batch_count2[lol.uv_count - 1] == uv_b_count2);
	}

	// max two per face belonging to the collapsed edge
	if(e.uv_pair_count > lol.max_uv_per_step)
	{
		lol.max_uv_per_step = e.uv_pair_count;
	}
	assert(lstep.morphed_uv_count == e.uv_pair_count);

	Free(uv_bid1);
	Free(uv_bid2);


	// remove batch indices
	for(int pbid = batch_index_cnt - 1; pbid >= 0; pbid--)
	{
		if(potential_batch_list[pbid])
		{
			if( IsBatchUsed_N_or_Less_Times(pbid, 0) )
			{
				SwapBatchIndecies(pbid, batch_index_cnt - 1);
	
				batch_index_cnt--;
				lstep.removed_batch_vertex_count++;
			}
		}
	}

	Free(potential_batch_list);

	// make every reference to v_id2 point to v_id1 & store those changed in lod_step
	for(int vb_id=0; vb_id < batch_index_cnt; vb_id++)
	{
		if(vb_idx[vb_id] == v_id2)
		{
			// save
            if(lstep.first_vertex == -1)
            {
                lstep.first_vertex = lol.vertex_count;
            }
            lol.vertex_count++;
            lol.vertex_list = (int*)Realloc(lol.vertex_list, lol.vertex_count * sizeof(int));
            lol.vertex_list[lol.vertex_count-1] = vb_id;

            lstep.morphed_vertex_chain_count++;
			
			// update
			vb_idx[vb_id] = v_id1;
		}
	}

    // drop edge count (the edge we collapsed)
    e_cnt--;
	
    lstep.removed_edge_count++;


	int affected_count = 0;
	int *affected_ev_list = NULL;
	for(int e_id=0; e_id < e_cnt; e_id++)
	{
		if(e_list[e_id].v1 == v_id1)
        {
            affected_count++;
			affected_ev_list = (int*)Realloc(affected_ev_list, affected_count * sizeof(int));
            affected_ev_list[affected_count-1] = e_list[e_id].v2;
        }
        else
        if(e_list[e_id].v2 == v_id1)
        {
            affected_count++;
			affected_ev_list = (int*)Realloc(affected_ev_list, affected_count * sizeof(int));
            affected_ev_list[affected_count-1] = e_list[e_id].v1;
        }
	}

	
	for(int ii=0; ii<affected_count; ii++)
	{
		for(int e_id=0; e_id < e_cnt; e_id++)
		{
e_label:
			if( ((e_list[e_id].v1 == v_id2) && 
				 (e_list[e_id].v2 == affected_ev_list[ii]) ) ||
				((e_list[e_id].v2 == v_id2) && 
				 (e_list[e_id].v1 == affected_ev_list[ii]) ) )
			{
				
				PutEdgeLast(e_id);
				e_cnt--;
                lstep.removed_edge_count++;
				if(e_id < e_cnt)
				{
					goto e_label;
				}
			}
		}
	}

	Free(affected_ev_list);
	
            
    // update affected edge errors & resort
    for(e_id=0; e_id<e_cnt; e_id++)
    {
redo_e_label:
		if(e_list[e_id].v1 == v_id2)
		{
			e_list[e_id].v1 = v_id1;
			assert(e_list[e_id].v2 != v_id1);
		}
		else
		if(e_list[e_id].v2 == v_id2)
		{
			e_list[e_id].v2 = v_id1;
			assert(e_list[e_id].v1 != v_id1);
		}
		
        if((e_list[e_id].v1 == v_id1) ||
           (e_list[e_id].v2 == v_id1))
        {
			SetEdge_Face_UV(e_id);
			e_list[e_id].error = QuadricEdgeError(e_id) * UVEdgeError(e_id);

			if(AllignEdgeByError(e_id) == 1)
			{
				goto redo_e_label;
			}
        }
    }

	// calcVertexNormal & D based on new faces instead of averaging
//TODO
/*
	Vector normal;
	int n_success = CalcVertexNormal(0, v_id1, normal);
	if(n_success == -1 ) // new normal is degenerate so just average (can happen for things like
						 // the spikes of dreadnought)
	{
		int n_id1 = v->normal[v_id1];
		int n_id2 = v->normal[v_id2];
		normal[0] = n->list[3 * n_id1] + n->list[3 * n_id2];
		normal[1] = n->list[3 * n_id1 + 1] + n->list[3 * n_id2 + 1];
		normal[2] = n->list[3 * n_id1 + 2] + n->list[3 * n_id2 + 2];
		Normalize3(normal);
	}

	v->normal[v_id1] = InsertNormal(n, normal, 1.5f * vertex_normal_tolerance);
	v->D_coefficient[v_id1] = -Dot3(normal, &(v->object_list[3*v_id1]));

	// store new normal & D
	lstep.v3_nid = v->normal[v_id1];
	lstep.v3_D = v->D_coefficient[v_id1];
*/

	// save step in library
    lol.count++;
    lol.step_list = (TriLODStep*)Realloc(lol.step_list, lol.count * sizeof(TriLODStep));
    lol.step_list[lol.count-1] = lstep;

	//lol.uv_step_list = (DAlod_uv_step*)Realloc(lol.uv_step_list, lol.count * sizeof(DAlod_uv_step));
	//lol.uv_step_list[lol.count-1] = uv_step;
}

void DAMesh::SwapVertices(const int id1, const int id2)
{
    if(id1 == id2) return;

	Swap96(v_list + id1, v_list + id2);
	Swap96(frozen_list + id1, frozen_list + id2);

	// reindex object
    for(int v_id = 0; v_id < max_batch_index_cnt; v_id++)
	{
		if(vb_idx[v_id] == id1)
		{
			vb_idx[v_id] = id2;
		}
		else
		if(vb_idx[v_id] == id2)
		{
			vb_idx[v_id] = id1;
		}
	}

    for(int e_id =0 ; e_id < max_e_cnt; e_id++)
    {
        if(e_list[e_id].v1 == id1)
        {
            e_list[e_id].v1 = id2;
        }
        else
        if(e_list[e_id].v1 == id2)
        {
            e_list[e_id].v1 = id1;
        }

        if(e_list[e_id].v2 == id1)
        {
            e_list[e_id].v2 = id2;
        }
        else
        if(e_list[e_id].v2 == id2)
        {
            e_list[e_id].v2 = id1;
        }

        assert(e_list[e_id].v1 != e_list[e_id].v2);
    }

	if(lol.count)
	{
		for(int step_id = 0; step_id < lol.count; step_id++)
		{
			TriLODStep & lstep = lol.step_list[step_id];

			if(lstep.vid1 == id1)
			{
				lstep.vid1 = id2;
			}
			else
			if(lstep.vid1 == id2)
			{
				lstep.vid1 = id1;    
			}

			if(lstep.vid2 == id1)
			{
				lstep.vid2 = id2;
			}
			else
			if(lstep.vid2 == id2)
			{
				lstep.vid2 = id1;    
			}
		}

		for(e_id = 0; e_id < lol.tmp_edge_count; e_id++)
		{
			if(lol.tmp_edge_list[2*e_id] == id1)
			{
				lol.tmp_edge_list[2*e_id] = id2;
			}
			else
			if(lol.tmp_edge_list[2*e_id] == id2)
			{
				lol.tmp_edge_list[2*e_id] = id1;
			}

			if(lol.tmp_edge_list[2*e_id+1] == id1)
			{
				lol.tmp_edge_list[2*e_id+1] = id2;
			}
			else
			if(lol.tmp_edge_list[2*e_id+1] == id2)
			{
				lol.tmp_edge_list[2*e_id+1] = id1;
			}

			assert(lol.tmp_edge_list[2*e_id] != lol.tmp_edge_list[2*e_id+1]);
		}
	}

	// swap err
	if(v_err)
	{
		MemSwap(v_err + id1, v_err + id2, sizeof(vertex_error));
	}
}

void DAMesh::SwapBatchIndecies(const int id1, const int id2)
{
	if(id1 == id2) return;
	
	Swap32(vb_idx + id1, vb_idx + id2);
	if( uvb0_idx )
	{
		Swap32(uvb0_idx + id1, uvb0_idx + id2);
	}
	if( uvb1_idx )
	{
		Swap32(uvb1_idx + id1, uvb1_idx + id2);
	}
	if( colb_idx )
	{
		Swap32(colb_idx + id1, colb_idx + id2);
	}
	Swap32(mtlb_idx + id1, mtlb_idx + id2);
	Swap32(smgb_idx + id1, smgb_idx + id2);
	Swap32(fnb_idx + id1, fnb_idx + id2);
	Swap32(nb_idx + id1, nb_idx + id2);

	for(int fg_id=0; fg_id < grp_cnt; fg_id++)
	{
		DAMeshGroup *fg = grp_list + fg_id;

		for(int f_id=0; f_id < fg->max_f_cnt; f_id++)
		{
			for(int i=0; i<3; i++)
			{
				if( fg->f_list[3*f_id + i] == id1 )
				{
					fg->f_list[3*f_id + i] = id2;
				}
				else
				if( fg->f_list[3*f_id + i] == id2 )
				{
					fg->f_list[3*f_id + i] = id1;
				}
			}
		}
	}

	/* // animated UV's
	for(int uvc_id=0; uvc_id < obj->uvcl.count; uvc_id++)
	{
		UVChannel *uvc = obj->uvcl.list + uvc_id;

		for(int vt_id=0; vt_id < uvc->vertex_count; vt_id++)
		{
			if(uvc->vertex_lookup[vt_id] == id1)
			{
				uvc->vertex_lookup[vt_id] = id2;
			}
			else
			if(uvc->vertex_lookup[vt_id] == id2)
			{
				uvc->vertex_lookup[vt_id] = id1;
			}
		}
	}
	*/

	for(int v_id=0; v_id < lol.vertex_count; v_id++)
	{
		if(lol.vertex_list[v_id] == id1)
		{
			lol.vertex_list[v_id] = id2;		

		}
		else
		if(lol.vertex_list[v_id] == id2)
		{
			lol.vertex_list[v_id] = id1;
		}
	}
	
	for(int i=0; i < lol.uv_chain_count; i++)
	{
		if(lol.batch_uv_id_chain[i] == id1)
		{
			lol.batch_uv_id_chain[i] = id2;

		}else
		if(lol.batch_uv_id_chain[i] == id2)
		{
			lol.batch_uv_id_chain[i] = id1;
		}
	}
}

void DAMesh::SwapLodFaces(const int id1, const int id2)
{
	const int g_id = active_group;
	DAMeshGroup *fg = grp_list + g_id;
	
	assert(id1 < fg->max_f_cnt && id2 < fg->max_f_cnt);
	assert(g_id < grp_cnt);

    if(id1 == id2) return;

	Swap16(fg->f_list + 3*id1,     fg->f_list + 3*id2);
	Swap16(fg->f_list + 3*id1 + 1, fg->f_list + 3*id2 + 1);
	Swap16(fg->f_list + 3*id1 + 2, fg->f_list + 3*id2 + 2);

	
	Swap32(fg->f_list_api_id + id1, fg->f_list_api_id + id2);
	Swap32(fg->f_list_smg + id1, fg->f_list_smg + id2);
	Swap32(fg->f_list_flat_n + id1, fg->f_list_flat_n + id2);

	for(int e_id=0; e_id < max_e_cnt; e_id++)
	{
		Eedge *e = e_list + e_id;
		
		for(int fr_id=0; fr_id < e->face_count; fr_id++)
		{
			if(e->group_list[fr_id] == g_id)
			{
				if(e->face_list[fr_id] == id1)
				{
					e->face_list[fr_id] = id2;
				}
				else
				if(e->face_list[fr_id] == id2)
				{
					e->face_list[fr_id] = id1;
				}
			}
		}
	}
}

void DAMesh::PutEdgeLast(const int eid)
{
	if(eid < e_cnt - 1)
	{
		Eedge tmp ( e_list[eid] );
		memmove(e_list + eid, e_list + eid + 1, ((e_cnt - 1) - eid) * sizeof(*(e_list)));
		e_list[e_cnt - 1] = tmp;
	}
}

// longest to shortest (list must be sorted except for e_id)
int DAMesh::AllignEdgeByError(const int eid)
{
	int i = eid;

	while((i < e_cnt - 1) &&
		  (e_list[eid].error < e_list[i+1].error))
	{
		i++;
	}
	if(i > eid)
	{
		Eedge tmp ( e_list[eid] );
		memmove(e_list + eid, e_list + eid + 1, (i - eid) * sizeof(*(e_list)));
		e_list[i] = tmp;

		return 1;
	}

	while((i > 0) && (e_list[eid].error > e_list[i-1].error))
	{
		i--;
	}
	if(i < eid)
	{
		Eedge tmp ( e_list[eid] );
		memmove(e_list + i + 1, e_list + i, (eid - i) * sizeof(*(e_list)));
		e_list[i] = tmp;

		return -1;
	}
	
	return 0;
}

void DAMesh::SplitVertices(void)
{
    for(int step = lol.count-1; step >= 0; step--)
    {
        AddLastEdge(step);
    }

    assert(lol.edge_count == lol.tmp_edge_count);
}

void DAMesh::AddLastEdge(const int step_id)
{
    const TriLODStep & lstep = lol.step_list[step_id];
	//const DAlod_uv_step & uv_step = lol.uv_step_list[step_id];
    
    // restore removed edges
    e_cnt += lstep.removed_edge_count;
	int index;
    int me_id;
    for(me_id=0, index=lstep.first_edge; me_id<lstep.morphed_edge_count; me_id++, index++)
    {
        if((lol.tmp_edge_list[2*index]   == lstep.vid1) ||
           (lol.tmp_edge_list[2*index+1] == lstep.vid1))
        {
            int e_id = GetEdgeID(lol.tmp_edge_list[2*index], lol.tmp_edge_list[2*index+1], max_e_cnt);
            assert(e_id != -1);
            lol.edge_count++;
            lol.edge_list = (int*)Realloc(lol.edge_list, lol.edge_count * sizeof(int));
            lol.edge_list[lol.edge_count-1] = e_id;

            if(e_list[e_id].v1 == lstep.vid1)
            {
                e_list[e_id].v1 = lstep.vid2;
            }
            else
            if(e_list[e_id].v2 == lstep.vid1)
            {
                e_list[e_id].v2 = lstep.vid2;
            }
            else    
            {
                Winprint("Error: edges out of synch!\n");
            }
        }
    }
/*
    // restore morphed faces
    int mf_id;
    for(mf_id=0, index=lstep.first_morphed_face; mf_id < lstep.morphed_face_count; mf_id++, index++)
    {
        int face_id = lol.face_list[index];
		int g_id = lol.group_list[index];

		DAMeshGroup *fg = grp_list + g_id;

		assert(fg->f_normal[ face_id ] == lol.low_normal_list[index]);
        fg->f_normal[ face_id ] = lol.high_normal_list[index];
    }
*/  
	// restore removed batch vertices
	batch_index_cnt += lstep.removed_batch_vertex_count;

    // restore affected batch list indices
	int v_ref;
    for(v_ref=0, index=lstep.first_vertex; v_ref < lstep.morphed_vertex_chain_count; v_ref++, index++)
    {
		assert(lol.vertex_list[index] < batch_index_cnt);
        vb_idx[ lol.vertex_list[index] ] = lstep.vid2;
    }

	// restore UV's
	for(int i = 0; i < lstep.morphed_uv_count; i++)
	{
		for(int j = 0; j < lol.high_batch_count1[lstep.first_morphed_uv + i]; j++)
		{
			const int bid = lol.batch_uv_id_chain[ lol.high_batch_first1[lstep.first_morphed_uv + i] + j ];
			assert(bid < batch_index_cnt);
			uvb0_idx[ bid ] = lol.high_uv_id1[ lstep.first_morphed_uv + i ];
		}

		for(j = 0; j < lol.high_batch_count2[lstep.first_morphed_uv + i]; j++)
		{
			const int bid = lol.batch_uv_id_chain[ lol.high_batch_first2[lstep.first_morphed_uv + i] + j ];
			assert(bid < batch_index_cnt);
			uvb0_idx[ bid ] = lol.high_uv_id2[ lstep.first_morphed_uv + i ];
		}
	}

//TODO
    // restore vertex 1
    v_list[lstep.vid1] = lstep.v1;
    //v->normal[lstep.vid1] = lstep.v1_nid;
    v_cnt++;

	// restore face count
	int f_ref;
	for(f_ref=0, index=lstep.first_removed_face; f_ref < lstep.removed_face_count; f_ref++, index++)
    {
		int g_id = lol.removed_face_list[index];
		DAMeshGroup *fg = grp_list + g_id;

		fg->f_cnt++;
		face_cnt++;
    }
}

void DAMesh::AddUVSpaces(void)
{
	if(lol.step_list && lol.max_uv_per_step > 0)
	{
		const int count = 2 * lol.max_uv_per_step;
		
		uv0_cnt += count;
		uv0_list = (TexCoord*)Realloc(uv0_list, uv0_cnt * sizeof(TexCoord));

		memmove(uv0_list + count, uv0_list, (uv0_cnt - count) * sizeof(TexCoord));
		memset(uv0_list, 0, count * sizeof(TexCoord));

		for(int i = 0; i < max_batch_index_cnt; i++)
		{
			uvb0_idx[i] += count;
		}		

		for(i = 0; i < lol.uv_count; i++)
		{
			lol.high_uv_id1[i] += count;
			lol.high_uv_id2[i] += count;
			lol.low_uv_id[i] += count;
		}
	}
}

void DAlod_lib::Init(void)
{
	closest = -1.0f;
	furthest = -1.0f;

    count = 0;
    step_list = NULL;
	//uv_step_list = NULL;

	removed_face_count = 0;
	removed_face_list = NULL;

    vertex_count = 0;
    vertex_list = NULL;

    tmp_edge_count = 0;
    tmp_edge_list = NULL;
    edge_list = NULL;
    edge_count = 0;

	max_uv_per_step = 0;
	uv_chain_count = 0;
	batch_uv_id_chain = NULL;
	//batch_uv_id_chain2 = NULL;
	uv_count = 0;
	high_uv_id1 = NULL;
	high_batch_count1 = NULL;
	high_batch_first1 = NULL;
	high_uv_id2 = NULL;
	high_batch_count2 = NULL;
	high_batch_first2 = NULL;
	low_uv_id = NULL;
}

void DAlod_lib::Release(void)
{
    Free(step_list);
	//Free(uv_step_list);
	Free(removed_face_list);

    Free(vertex_list);
    Free(tmp_edge_list);
    Free(edge_list);

	Free(batch_uv_id_chain);
	//Free(batch_uv_id_chain2);
	Free(high_uv_id1);
	Free(high_batch_count1);
	Free(high_batch_first1);
	Free(high_uv_id2);
	Free(high_batch_count2);
	Free(high_batch_first2);
	Free(low_uv_id);

	count = 0;
}

void DAlod_lib::ReverseStepList(void)
{
	for(int i = 0; i < count / 2; i++)
	{
		MemSwap(step_list + i, step_list + (count - 1) - i, sizeof(TriLODStep));
		//MemSwap(uv_step_list + i, uv_step_list + (count - 1) - i, sizeof(DAlod_uv_step));
	}
}