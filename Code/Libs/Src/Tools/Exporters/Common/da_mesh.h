#ifndef __DA_MESH_H
#define __DA_MESH_H

// TODO:
// add edges
// add support for multiple source objects
// weight vertex normals by interior face angle

#include "vector.h"
#include "vector4.h"
#include "matrix4.h"
#include "texturecoord.h"
#include "mtl_txt.h"
#include "trimeshlod.h"  // also used by runtime

#define XYZ_TOLERANCE 0.000001f
#define FACE_NORMAL_TOLERANCE 1.0f // 2
#define VERTEX_NORMAL_TOLERANCE 2.0f // 5
#define UV_TOLERANCE .005f
#define UV_STEP .5f/512.0f

const float D2R=(float)(M_PI/180.0);
const float R2D=(float)(180.0/M_PI);

extern float lod_mtl_weight;
extern float lod_uv_weight;

typedef enum {INVALID=0, FIXED_MESH, DEF_MESH, LIGHT, CAMERA, FIXED_NURB, DEF_NURB, FIXED_PATCH,
				DEF_PATCH, FIXED_OBJ, DEF_OBJ, END2=INT_MAX} object_type;

struct Cbgra
{
	union {
		struct {
		unsigned char	b, g, r, a;
		};
		unsigned int bgra;
	};
};

struct uv_mtl
{
	int uvid1;
	int uvid2;
	cq2Mtl *m;

	void Init(void)
	{
		uvid1 = -1;
		uvid2 = -1;
		m = NULL;
	}
};

struct bone_vertex
{
	int bone_count;
	int *bone_id_list;
	float *xyz_list;
	float *normal_list;
	float *weight_list;
	float *rational_list;

	void Init(void)
	{
		bone_count = 0;
		bone_id_list = NULL;
		xyz_list = NULL;
		normal_list = NULL;
		weight_list = NULL;
		rational_list = NULL;
	}

	void Release(void)
	{
		bone_count = 0;
		Free(bone_id_list);
		Free(xyz_list);
		Free(normal_list);
		Free(weight_list);
		Free(rational_list);
	}
};

struct Eedge
{
	int v1;
	int v2;
	float error;
	float angle;		// max angle between faces

	int face_count;		// faces on this edge
	int *face_list;
	int *group_list;

	Vector4	v_dest;		// new vertex to collapse edge to

	// uv lod stuff
	int uv_pair_count;  // no more than face_count
	uv_mtl *uv_pairs;	// indices into texture_list
	float *uv_dest;	// new uv coordinate

	void Init(void)
	{
		v1 =
		v2 = -1;
		error = -1.0f;
		angle = 0.0f;
		face_count = 0;
		face_list = NULL;
		group_list = NULL;
		v_dest.zero();

		uv_pair_count = 0;
		uv_pairs = NULL;
		uv_dest = NULL;
	}

	void Release(void)
	{
		face_count = 0;
		Free(face_list);
		Free(group_list);
		uv_pair_count = 0;
		Free(uv_pairs);
		Free(uv_dest);
	}

	Eedge ( const Eedge & e )
	{
		memcpy(this, &e, sizeof(*this));
	}

	Eedge & operator = (const Eedge & e)
	{
		memcpy(this, &e, sizeof(*this));
		return *this;
	}
};

struct RPVertex
{
	Vector pos;
	int api_pos_id;
	Vector frozen;

	TexCoord uv0;
	int api_uv0_id;

	TexCoord uv1;
	int api_uv1_id;

	Cbgra color;
	int api_col_id;

	// flags for optional data
	bool use_uv0;
	bool use_uv1;
	bool use_color;

	void Init(void)
	{
		pos.zero();
		frozen.zero();
		api_pos_id = -1;
		
		uv0 = TexCoord( .5f, .5f );
		api_uv0_id = -1;

		uv1 = TexCoord( .5f, .5f );
		api_uv1_id = -1;

		color.bgra = 0xFFFFFFFF;
		api_col_id = -1;

		use_uv0 = false;
		use_uv1 = false;
		use_color = false;
	}
};

struct RPFace
{
	RPVertex verts[3];
	int api_face_id;
	int api_sm_grp;
	int api_node_id;
	int mtl_id;

	void Init(void)
	{
		verts[0].Init();
		verts[1].Init();
		verts[2].Init();
		api_face_id = -1;
		api_sm_grp = 0;
		api_node_id = 0;
		mtl_id = -1;
	}

	bool IsValid(void) const
	{
		return (!verts[0].pos.equal( verts[1].pos, 0.0001f ) &&
				!verts[0].pos.equal( verts[2].pos, 0.0001f ) &&
				!verts[1].pos.equal( verts[2].pos, 0.0001f ) );
	}
};

struct RPIndex
{
	int v_i;
	int uv_i0;
	int uv_i1;
	int color_i;
	int mtl_i;
	int smg_i;
	int n_id; // flat case only (i.e., when smg_i is 0)
};

struct DAMeshGroup
{
	// topology
	int f_cnt;
	int max_f_cnt;
	U16 *f_list;			// 3 indices per face into batch indices
	int *f_list_api_id;
	int *f_list_smg;		// bits for smoothing groups
	int *f_list_flat_n;

	Matrix4 *quadric_list;	// stores error quadric for each original plane; never gets reindexed

	// material
	int mtl_id;
	char *mtl_name;
	int api_node_id;

	int e_cnt;
	U16 *e_list;
	float *e_angles;

	int InsertFace( const int bid[3], const int api_id, const int sm_gid, const int flat_n_id );
	void Init(void);
	void Release(void);
};

typedef struct _vertex_error{
	int plane_count;
	int *plane_group_list;
	int *plane_face_list;
	Matrix4 quadric;

	void Init(void)
	{
		plane_count = 0;
		plane_group_list = NULL;
		plane_face_list = NULL;
		quadric.zero();
	}

	void Release(void)
	{
		Free(plane_group_list);
		Free(plane_face_list);
		Init();
	}

}vertex_error;

struct DAlod_lib
{
	float closest;
	float furthest;

	int count;
	TriLODStep *step_list;
	//DAlod_uv_step *uv_step_list;

	int removed_face_count;
	int *removed_face_list; // group id's for which to drop one face
	
	int vertex_count;	// number of total modified chain indices
	int *vertex_list;	// list of which chain vertices were modified (id2->id1) chain indices

	int edge_count;
	int *edge_list;		// final indexes of reindexed edges into el since we resort edges each time
						// edge_list has to store vertex id's and not edge id's

	int *tmp_edge_list;	// vid's of reindexed edges 2*edge_count
	int tmp_edge_count;	// temporary

	int max_uv_per_step;		// the most uv's morphed by any single edge collapse
								// used to create blank spaces in texture_list for interpolation
	
	int uv_chain_count;			// length of affected batch index chain
	int *batch_uv_id_chain;		// positios in batch list that changed

	int uv_count;
	int *high_uv_id1;			// indices to change them to
	int *high_batch_count1;
	int *high_batch_first1;

	int *high_uv_id2;			// indices to change them to
	int *high_batch_count2;
	int *high_batch_first2;

	int *low_uv_id;

	void Init(void);
	void Release(void);
	void ReverseStepList(void);
};

struct DAMesh
{
	object_type type;

	// topology
	int face_cnt;
	int max_face_cnt;
	int grp_cnt;
	DAMeshGroup *grp_list;
	int active_group;			// used by sortq for sorting faces

	// batch indices
	int batch_index_cnt;
	int max_batch_index_cnt;
	int *vb_idx;
	int *uvb0_idx;
	int *uvb1_idx;
	int *colb_idx;
	int *mtlb_idx;
	int *smgb_idx;	// used to make unique based on smoothing groups / vertex normals
	int *fnb_idx;	// used to make unique based on face nornals if there are no smoothing groups
	int *nb_idx;	// computed last (for deformables this is the same as vb_idx)

	// geometry
	int v_cnt;
	int max_v_cnt;
	Vector *v_list;
	int *v_list_api_id;
	int *v_list_api_node_id;
	vertex_error *v_err;	// vertex's quadric error (used by lod)
	Vector *frozen_list;
	bone_vertex *b_v_list; // used by deformables
	int *bone_first_list;
	int *bone_count_list;
	int *bone_id_chain;
	float *bone_weight_chain;

	int uv0_cnt;
	TexCoord *uv0_list;
	int *uv0_list_api_id;

	int uv1_cnt;
	TexCoord *uv1_list;
	int *uv1_list_api_id;

	int color_cnt;
	Cbgra *color_list;
	int *color_list_api_id;

	int vn_cnt;		
	Vector *vn_list;	// vertex normal list (computed last)
						// for deformables this is parallel to v_list

	int flat_f_n_cnt;	// used to make batch indices unique for flat faces
	Vector *flat_f_n_list;

	int e_cnt;			// edge count
	int max_e_cnt;
	Eedge *e_list;
	int *sync_e_list;		// list of edges 2 indices into v_list per edge
	float *sync_e_angles;	// angles between the edge faces (180 if only one face)

	mtl_lib *ml;

	DAlod_lib lol;

	void InsertFace( const RPFace & face );
	int  InsertFaceData( const RPFace & face );
	int  GetGroupID( const int mtl_id, const int api_node_id ) const;

	int GetXYZ_ID( const Vector & pos, const int api_id, const int node_id ) const;
	int InsertXYZ( const Vector & pos, const Vector & frozen, const int v_api_id, const int node_id );
	int GetUV0_ID( const TexCoord & uv, const int api_id ) const;
	int InsertUV0( const TexCoord & uv, const int api_id );
	int GetUV1_ID( const TexCoord & uv, const int api_id ) const;
	int InsertUV1( const TexCoord & uv, const int api_id );
	int GetCol_ID( const Cbgra & color, const int api_id ) const;
	int InsertCol( const Cbgra & color, const int api_id );

	//int InsertSmg( const int sm_grp );
	int CalcFaceGroupNormal(const int g_if, const int f_id, Vector & normal) const;
	int InsertFlatFNormal( const Vector & normal );
	int InsertVertexNormal( const Vector & normal );
	int GetNormalID( const Vector & normal, const Vector *n_list, const int length, const float tol_angle /*in deg*/ ) const;
	float GetAngleWeight( const int gid, const int fid, const int vid,
						  const int vid0, const int vid1, const int vid2 ) const;

	RPIndex InsertVertex( const RPVertex & rp_vtx, const int node_id );
	int Get_ID( const int *list, const int length, const int value ) const;
	int InsertBatchIndex( const RPIndex & rp_idx );
	int Get_BtachID( const RPIndex & rp_idx ) const;
	
	int GetEdgeID(const int vid1, const int vid2, int count) const;
	int InsertEdge(const int vid1, const int vid2);
	void SetEdge_Face_UV( const int e_id );

	// LOD related
	void CollapseEdges( const int target_face_cnt );
	void CalcEdgeErrors(void);
	void CalcQuadrics(void);
	float QuadricEdgeError(const int e_id);
	float UVEdgeError(const int e_id) const;
	void AddVertexQuadric(int vid, int gid, int fid);
	Matrix4 MergeQuadrics(int vid1, int vid2, bool add_lists, int *sum_count);
	bool IsVertexUsed_N_or_Less_Times(const int vid, const int t_count) const;
	bool IsBatchUsed_N_or_Less_Times(const int vbid, const int t_count);
	void RemoveLastEdge(void);
	void SwapBatchIndecies(const int id1, const int id2);
	void SwapVertices(const int id1, const int id2);
	void SwapLodFaces(const int id1, const int id2);
	void PutEdgeLast(const int eid);
	int AllignEdgeByError(const int eid);
	void SplitVertices(void);
	void AddLastEdge(const int step_id);
	void AddUVSpaces(void);

	void PostProcess(const float percent, const float closest, const float furthest);
	void CalcVertexBatchNormals( void );
	void CalcVertexBatchNormal( const int bid );
	void CalcVertexXYZNormal( const int vid ); // used by deforms
	void CalcEdges(void);
	void CalcEdgeAngles(void);
	void CalcGroupEdges(void);
	void SyncEdges(void);
	void CleanUnusedData(void);
	void Init( mtl_lib * in_ml );
	void Release(void);
};

inline float GetQuadricVectorErr(const Vector4 & v, const Matrix4 & m)
{	return (float)fabs( dot4((m * v), v) );	}

inline float new_lock(const float l1, const float l2)
{
//	return l1 + (1.0f - l1) * l2;
//	return l2 + (1.0f - l2) * l1;
	return l1 + l2 - l1 * l2;
}

float Acos(const float angle);
bool GetNewV(Matrix4 quadric, Vector4 *new_v);

#endif
