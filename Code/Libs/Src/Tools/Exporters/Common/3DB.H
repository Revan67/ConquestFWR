/* 3DB data structures */
#ifndef THREEDB_H
#define THREEDB_H
#include <math.h>
#include "vector.h"
#include "matrix.h"
#include "xform.h"
#include "persistmath.h"
#include "extents.h"
#include "faceprop.h"
#include "persisthardpoint.h"
#include "sgi_utf.h"
#include "cont_lod.h" // also used by runtime
#include "matrix4.h"
#include "eigen.h"
#include "minbox3.h"
#include "volint.h"
#include "names.h"
#include "mtl_txt.h"
#include "persistmisc.h"
#include "daprop.h"
#include "da_mesh.h"

#define DENSITY .001f

extern float face_normal_tolerance;
extern float vertex_normal_tolerance;

extern bool exporting_deformable;

extern bool remove_constant_channels;
extern int convex_hull_flag;
extern float scale_factor;
extern int no_physics;
extern int center_mass;
extern int export_vertex_colors;
extern int ik_extents;

typedef enum {SPHERE, TUBE, BOX, CYLINDER, HULL, END6=INT_MAX} ext_type;

typedef enum {INVALID_V_FLAG=0, RIGHT_EYE=24, LEFT_EYE=25, END8=INT_MAX} vert_flag;

#define MAX_POLY_VERTEX_COUNT 32

typedef struct _poly
{
	// in
	int vertex_count;
	float vertices[3 * MAX_POLY_VERTEX_COUNT];
	float frozen[3 * MAX_POLY_VERTEX_COUNT];
	int color[3 * MAX_POLY_VERTEX_COUNT]; //RGB
	int flags[MAX_POLY_VERTEX_COUNT];
	float uv[2 * MAX_POLY_VERTEX_COUNT];
	float uv2[2 * MAX_POLY_VERTEX_COUNT];
	
	int material_id;
	FACE_PROPERTY property;

	// optional to keep a back cross reference
	int api_node_id;
	int api_face_id;
	int api_mtl_id;
	int api_smg_id;
	int api_uv_id[MAX_POLY_VERTEX_COUNT];
	int api_uv_id2[MAX_POLY_VERTEX_COUNT];
	int api_xyz_id[MAX_POLY_VERTEX_COUNT];
	
	// out
	int group_id;
	int face_id;

	_poly(void)
	{
		vertex_count = 0;
		material_id = -1;
		property = 0;

		group_id = -1;
		face_id = -1;

		api_node_id = (int)NULL;
		api_face_id = -1;
		api_mtl_id = -1;
		api_smg_id = 0;	// 0 means no group

		for(int i=0; i<MAX_POLY_VERTEX_COUNT; i++)
		{
			api_uv_id[i] = -1;
			api_uv_id2[i] = -1;
			api_xyz_id[i] = -1;

			frozen[3*i  ] =
			frozen[3*i+1] =
			frozen[3*i+2] = 0.0f;

			color[3*i  ] =
			color[3*i+1] =
			color[3*i+2] = -1;

			flags[i] = 0;

			uv[2*i  ] =
			uv[2*i+1] = 0.0f;

			uv2[2*i  ] =
			uv2[2*i+1] = FLT_MAX;
		}
	}

	void zero_uv(void)
	{
		uv[2*0]=0.5f;
		uv[2*0+1]=0.5f;
		api_uv_id[0] = -1;

		uv[2*1]=0.5f;
		uv[2*1+1]=0.5f;
		api_uv_id[1] = -1;

		uv[2*2]=0.5f;
		uv[2*2+1]=0.5f;
		api_uv_id[2] = -1;
	}

	void zero_uv2(void)
	{
		uv2[2*0]=
		uv2[2*0+1]=FLT_MAX;
		api_uv_id2[0] = -1;

		uv2[2*1]=
		uv2[2*1+1]=FLT_MAX;
		api_uv_id2[1] = -1;

		uv2[2*2]=
		uv2[2*2+1]=FLT_MAX;
		api_uv_id2[2] = -1;
	}

}poly;

typedef struct _nurb
{
	char *name;
	int s_order;		// order 4 = degree 3 = cubic
	int t_order;

	bool s_closed;
	bool t_closed;

	int s_knot_count;
	int t_knot_count;
	float *s_knot_list;
	float *t_knot_list;
	int s_basis_cnt;	// point_cnt - (order - 1) - (duplicate knots)
	int t_basis_cnt;

	int s_point_count;
	int t_point_count;
	float *point_list;	// u_ctrl_count * v_ctrl_count long
	float *weight_list;
	int *api_s_id;
	int *api_t_id;
	int api_st_offset;	// however many other CV's this node has listed before this surface
	int api_node_id;
	int api_node_cv_total;

	int s_uv_count;		// basis_cnt + 1
	int t_uv_count;

	float *uv_point_list;
	float *uv_list;		// take out & replace by uv_point_list above when chord stuff is fixed
	

	bone_vertex *b_v_list;   // holds bone control point assignment info before syncing into arrays
	int *vertex_bone_count;  // number of bones contributing to this cv
	int *first_vertex;	     // offset of first cv
	int *bone_id_list;       // the index of each bone contributing
	float *bone_vertex_list; // cv x,y,z in bone's coord system
	float *bone_rational_list; // w coordinate
	float *bone_weight_list; // fraction of bone's contribution

	// NOTE: patch_count = (point_count - order + 1)
	// knot_count - order = point_count

	int mtl_id;
}nurb;

struct Bezier_patch
{
	int type;			// 3 tri; 4 quad;

	int	v[4];			// Can have 3 or 4 vertices
	int	vec[9];			// Can have 6 or 8 vector points // big enough to hold aux indices for 4D tri patch
	int	interior[4];	// Can have 3 or 4 interior vertices

	int aux_index;		// Used for triangular patches only -- index to Degree 4 control points
	
	int	tv[4];			// Texture verts 3 or 4

	int edges[4];		// indices into edge list

	void Init(void)
	{
		memset(this, 0xff, sizeof(*this));
	}
};

struct Bezier_patch_group
{
	int patch_cnt;
	Bezier_patch *patch_list;
	int *api_id_list;

	int mtl_id;
	//int api_mtl_id;

	void Init(void)
	{
		memset(this, 0, sizeof(*this));
	}

	void Release(void)
	{
		Free(patch_list);
		Free(api_id_list);
	}

	void InsertPatch( const Bezier_patch & patch, const int api_id );

};

typedef struct _Bezier_edge
{
	int v1;			// Index of first vertex
	//int vec12;		// Vector from v1 to v2
	//int vec21;		// Vector from v2 to v1
	int v2;			// Index of second vertex
	int patch1;		// Index of first patch
	int patch1_grp; // Index of first patch group
	int patch2;		// Index of second patch
	int patch2_grp; // Index of second patch group
}Bezier_edge;


typedef struct _Bezier_mesh
{
	int group_cnt;
	int patch_cnt;	// total count for all the groups
	Bezier_patch_group *groups;

	int vertex_cnt;
	Vector *vertices;

	int		vector_cnt;
	Vector	*vectors;			// Location
	//int		*vector_owner;	// Vertex which owns this vector
	int		aux_cnt;
	Vector *aux;				//Used for triangular patches only -- Degree 4 control points
	
	int uv_cnt;
	TexCoord *uvs;

	int		edge_cnt;
	Bezier_edge	*edges;


	// temporary stuff used for deformable export
	int api_node_count;			// number if INodes this skin came from
	int *api_node_list;			// list of *INode
	int *api_node_ver_offset;	// starting index into *vertices for this node
	int *api_node_vec_offset;	// starting index into *vector for this node
	int *api_node_aux_offset;	// starting index into *aux for this node

	bone_vertex *b_v_list_ver;   // holds bone vertex assignment info before syncing into arrays
	int *vertex_bone_count_ver;  // number of bones contributing to this vertex
	int *first_vertex_ver;       // offset of first vertex
	int *bone_id_list_ver;       // the index of each bone contributing
	float *bone_weight_list_ver; // fraction of bone's contribution
	Vector *bone_vertex_list_ver; // vertex x,y,z in bone's coord system
	
	bone_vertex *b_v_list_vec;   // holds bone vertex assignment info before syncing into arrays
	int *vertex_bone_count_vec;  // number of bones contributing to this vertex
	int *first_vertex_vec;       // offset of first vertex
	int *bone_id_list_vec;       // the index of each bone contributing
	float *bone_weight_list_vec; // fraction of bone's contribution
	Vector *bone_vertex_list_vec; // vertex x,y,z in bone's coord system

	bone_vertex *b_v_list_aux;   // holds bone vertex assignment info before syncing into arrays
	int *vertex_bone_count_aux;  // number of bones contributing to this vertex
	int *first_vertex_aux;       // offset of first vertex
	int *bone_id_list_aux;       // the index of each bone contributing
	float *bone_weight_list_aux; // fraction of bone's contribution
	Vector *bone_vertex_list_aux; // vertex x,y,z in bone's coord system

	int final_vertex_count;
	Vector *final_vertices;		// not bone assigned
	int *final_vertex_bone_count;
	int *final_first_vertex;
	int final_vertex_length;
	int	*final_bone_id_list;
	float *final_weight_list;
	Vector *final_vertex_list;

	//int			edge_cnt;
	//Bezier_edge *edges;

	void Init(void)
	{
		memset(this, 0, sizeof(*this));
	}

	void Release(void)
	{
		for(int i = 0; i < group_cnt; i++)
		{
			groups[i].Release();
		}
		Free(groups);

		Free(vertices);
		Free(vectors);
		//Free(vector_owner);
		Free(aux);
		Free(uvs);
		Free(edges);
		Free(final_vertices);

		Free(api_node_list);
		Free(api_node_ver_offset);
		Free(api_node_vec_offset);
		Free(api_node_aux_offset);

		if(b_v_list_ver) // deformable
		{
			for(i = 0; i < vertex_cnt; i++)
			{
				b_v_list_ver[i].Release();
			}
			Free(b_v_list_ver);
			Free(vertex_bone_count_ver);
			Free(first_vertex_ver);
			Free(bone_id_list_ver);
			Free(bone_weight_list_ver);
			Free(bone_vertex_list_ver);

			for(i = 0; i < vector_cnt; i++)
			{
				b_v_list_vec[i].Release();
			}
			Free(b_v_list_vec);
			Free(vertex_bone_count_vec);
			Free(first_vertex_vec);
			Free(bone_id_list_vec);
			Free(bone_weight_list_vec);
			Free(bone_vertex_list_vec);

			for(i = 0; i < aux_cnt; i++)
			{
				b_v_list_aux[i].Release();
			}
			Free(b_v_list_aux);
			Free(vertex_bone_count_aux);
			Free(first_vertex_aux);
			Free(bone_id_list_aux);
			Free(bone_weight_list_aux);
			Free(bone_vertex_list_aux);

			Free(final_vertex_bone_count);
			Free(final_first_vertex);
			Free(final_bone_id_list);
			Free(final_weight_list);
			Free(final_vertex_list);
		}
	}

	void InsertPatch( Bezier_patch & patch, const int mtl_id, const int api_id, const Vector aux9[9] );
	int AppendAux(const Vector aux9[9]);
	void CalcEdges();
	int InsertEdge(const int v1, const int v2, const int gid, const int pid );
	int GetEdge(const int v1, const int v2);

}Bezier_mesh;


typedef struct _face_group
{
	int material_id;
	int count;
	int max_count;
	int *batch_chain;		// 3 indices per triangle into vertex batch_list;
	int *normal;
	float *D_coefficient;
	FACE_PROPERTY *property;

	int *api_node_id;
	int *api_face_id;
	int *api_smg_id;
	int api_mtl_id;

	Matrix4 *quadric_list;	// stores error quadric for each original plane
							// never gets reindexed
	
	int *adjacent_face_list; // 3 per face; not used for now

}face_group;

typedef struct _normals {
  int count;                // # of normals (may be < # of faces)
  float *list;              // xyz list of normals
}normals;

typedef struct _vertices {
  int batch_count;
  int max_batch_count;     // maximum count (used by lod)
  int *object_batch_list;  // lists of unique xyz, u,v, m vertices
  int *texture_batch_list;
  int *texture_batch_list2;
  int *mtl_batch_list;

  int object_count;        // # of physical vertices (geometry)
  int max_object_count;	   // maximum count (used by lod)
  float *object_list;      // xyz vertex data
  int *flags;
  int *normal;             // index into normal list for each vertex // computed at the end
  float *D_coefficient;    // D value for vertex & it's normal
  float *frozen_list;      // marks vertices "frozen" in either x,y,z directions for LOD purposes
  unsigned char *color;	   // vertex color optional // RGB
  int *api_node_xyz_id;
  int *api_xyz_id;
  int *api_smg_id;
  
  vertex_error *err;	   // vertex's quadric error (used by lod)

  bone_vertex *b_v_list;   // holds bone vertex assignment info before syncing into arrays
  int *vertex_bone_count;  // number of bones contributing to this vertex
  int *first_vertex;	   // offset of first vertex
  int *bone_id_list;       // the index of each bone contributing
  float *bone_weight_list; // fraction of bone's contribution
  float *bone_vertex_list; // vertex x,y,z in bone's coord system
  float *bone_normal_list; // normal x,y,z in bone's coord system

  int texture_count;       // # of uv pairs
  float *texture_list;     // uv data
  int *uv_flags;
  int *api_node_uv_id;
  int *api_uv_id;

  int uv_bone_count;		// number of bones affecting UV's
  int *uv_bone_id;
  int *uv_vertex_count;		// number of vertices per bone
  float *uv_plane_distance;	// plane to bone distance in -Z
  float *x_to_u_scale;
  float *y_to_v_scale;
  float *min_du;
  float *max_du;
  float *min_dv;
  float *max_dv;

  int uv_list_length;		// sum of all uv_vertex_count[]
  int *uv_vertex_id_list;	// list of texture vertices
  float *uv_list;			// list of default uv values
    
}vertices;

typedef struct _Eedge_lib
{
	int count;					// number of edges
	int max_count;
	Eedge *list;		
}Eedge_lib;

typedef struct _edge
{
	int count;					// number of edges
	int *vertex_list;			// indices into vertex_list (2 per edge)
	float *angle;				// delta angle of face normals 
}edges;

typedef struct _lod_lib
{
	float closest;
	float furthest;

	int count;
	lod_step *step_list;
	lod_uv_step *uv_step_list;

	int removed_face_count;
	int *removed_face_list; // group id's for which to drop one face

	int face_count;			// number of total face morphs
	int *face_list;
	int *group_list;
	int *high_normal_list;
	float *high_D_coefficient_list;
	int *low_normal_list;
	float *low_D_coefficient_list;
	
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

} lod_lib;

typedef struct _UVChannel
{
  char *name;
  int frame_count;
  float fps;
  int *uv_chain;		// indices for v.object_chain (length=frame_count*vertex_count)
  int vertex_count;
  int *vertex_lookup;   // length = vertex_count
  int interpolate;
}UVChannel;

typedef struct _UVChanel_lib {
	int count;
	UVChannel *list;
}UVChannel_lib;

typedef struct _light {
	float color[3];
	float direction[3];
	float range;  // -1 means infinite
	float cutoff; // 1/2 cone angle in radians
	float hotspot;
	int parallel;
	char *texture_name;
}light;

typedef struct _camera {
	float fovx;
	float fovy;
	float znear;
	float zfar;
}camera;

struct object {
  object_type type;

  int face_count;
  int max_face_count;
  int face_group_count;
  face_group *face_group_list;
  int active_group;			// used by sortq for sorting faces

  vertices v;
  normals n;
  edges e;
  Eedge_lib el;  // el is used during export and later to generate edges e

  lod_lib lol; 

  mtl_lib *ml;
  
//  txt_lib *tl;			// this is a pointer because it can export as part of the object or 
//  anim_txt_lib *atl;	// as part of the compound

  UVChannel_lib uvcl;
  Extents extents;

  // hard point data
  int hp_fix_count;
  PersistHPFixed *hp_fix_list;
  int hp_pris_count;
  PersistHPPrismatic *hp_pris_list;
  int hp_rev_count;
  PersistHPRevolute *hp_rev_list;

  light l;
  camera c;

  int nurb_count;
  nurb *nurb_list;

  Bezier_mesh b_mesh;

  PersistPropHeader prop_hdr;
  PersistProperty2 *prop_list;
  int bin_prop_size;
  unsigned char *bin_prop;

  DAMesh da_mesh;

  object & operator = (const object & obj) // this is same as default but fixes a compiler bug?
  {
	  memcpy(this, &obj, sizeof(*this));
	  return *this;
  }

};

typedef struct _lod_object {
  int count;
  float *switch_list;
  float mim_max_dist[2];
  object *obj_list;
  char file_name[256];
  int export_flag;
  //txt_lib *tl;
  //anim_txt_lib *atl;
  mtl_lib *ml;
}lod_object;

Vector Transform_vector(const Transform & t, const Vector & v);

int IsRightHanded(const PersistMatrix *pm);

void InitHardPoints(object *obj);
void InitHPFixed(PersistHPFixed *fixed);
void InsertHPFixed(object *obj, const PersistHPFixed& hp_fixed);
void InsertHPPrismatic(object *obj, const PersistHPPrismatic& hp_prismatic);
void InsertHPRevolute(object *obj, const PersistHPRevolute& hp_revolute);

file_node* CreateDAFaceGroup(DAMeshGroup & group, int id);
file_node*  CreateDAFaceGroups( DAMesh & da_mesh );
//file_node*  CreateDABatchIndices( DAMesh & da_mesh );
file_node*  CreateDAGeometry( DAMesh & da_mesh );
//void CopyDAMesh( object *obj );
//void WriteObject(object& obj, const char *file_name, int txt_flag);
void WriteObject(object *obj, const char *file_name, int txt_flag);
void WriteLodObject(lod_object *l_obj, int txt_flag);
file_node* CreateObject(object *obj, int txt_flag);
file_node* CreateLodObject(lod_object *lod_obj, int txt_flag);
file_node* CreateLodLevels(lod_object *lod_obj);
file_node* CreateSwitch(lod_object *lod_obj);
file_node* CreateFaceGroups(object *obj);
file_node* CreateFaceGroup(face_group *fg, int id);
file_node* CreateNurbSet(object *obj);
file_node* CreateNurb(nurb *nr, int id);
file_node* CreateNormals(normals *n);
file_node* CreateVertices(vertices *v);
file_node* CreateEdges(const edges *e);
file_node* CreateLodLib(const lod_lib *lol);
file_node* CreateUVChannelLib(const UVChannel_lib *uvcl);
file_node* CreateUVChannel(const UVChannel *uvc, int id);

file_node* CreateDALodLib(const DAlod_lib *lol);

void SynchPatchMesh(Bezier_mesh *bm);
file_node* CreatePatchGroups(Bezier_mesh *bm);
file_node* CreatePatchGroup(Bezier_patch_group *pg, int id);
file_node* CreatePatchVertices(Bezier_mesh *bm);
file_node* CreatePatchEdges(Bezier_mesh *bm);

file_node* CreateHardPoints(object *obj);
void AddFixedHP(file_node *node, const PersistHPFixed & hp);
void AddPrisHP(file_node *node, const PersistHPPrismatic & hp);
void AddRevHP(file_node *node, const PersistHPRevolute & hp);
file_node* CreateProperties(object *obj);
file_node* CreateRigidBody(const Extents& extents);
file_node* CreateMass(const MassData& mass);
file_node* CreateExtent(const Extents& extents);
file_node* CreateSphere(const ExtentSphere& sphere);
file_node* CreateRenderSphere(const Extents & ext);
file_node* CreateBox(const ExtentBox& box);
file_node* CreateCylinder(const Cylinder& cylinder);
file_node* CreateTube(const Cylinder& cylinder);
file_node* CreateConvexHull(const CollisionMesh& mesh);

//void InitLodObject(lod_object *lobj, txt_lib *tl, anim_txt_lib *atl, mtl_lib *ml);
void InitLodObject(lod_object *lobj, mtl_lib *ml);
//void InitObject(object *obj, txt_lib *tl, anim_txt_lib *atl, mtl_lib *ml);
void InitObject(object *obj, mtl_lib *ml);

void InitProps(object *obj);
void InitLight(light *l);
void InitCamera(camera *c);
void InitUVChannelLib(UVChannel_lib *uvlib);
void InitUVChannel(UVChannel *uvc);
void InitLodLib(lod_lib *lol);

void InitGeometry(object *obj);
void InitNurb(nurb *nr);
void InitFaceGroup(face_group *fg);
void InitNormal(normals *n);
void InitVertex(vertices *v);
void InitEdges(edges *e);
void InitEedgeLib(Eedge_lib *el);

void InitExtents(Extents *extents);
//void ZeroExtents(Extents *extents);
void InitMatrix(Matrix *m);
Matrix MatrixTranspose(const Matrix& m);

void FreeLodObject(lod_object *lod_obj);
void FreeObject(object *obj);
void FreeProps(object *obj);
void FreeFaceGroups(object *obj);
void FreeFaceGroup(face_group *fg);
void FreeNurbs(object *obj);
void FreeNurb(nurb *nr);
void FreeLight(light *l);
void FreeVertices(vertices *v);
void FreeVertices(void *v);
void FreeEdges(edges *e);
void FreeEedge(Eedge *ee);
void FreeEedgeLib(Eedge_lib *el);
void FreeUVChannelLib(UVChannel_lib *uvlib);
void FreeUVChannel(UVChannel *uvc);
void FreeLodLib(lod_lib *lol);
void FreeLodStep(lod_step *los);


int GetVertexID(const vertices * const vl, const float * const v, const int api_node_id,
				const int api_v_id, const int api_smg_id);

inline float square( const float f ) { return f * f; }
inline float my_fabs( float f )
{
	*(unsigned long *)&f &= 0x7FFFFFFF;
	return f;
}

inline bool SameXYZ(const float * const p1, const float * const p2, const float tolerance)
{
#if 0
	return ( my_fabs(p1[0] - p2[0]) < tolerance &&
			 my_fabs(p1[1] - p2[1]) < tolerance &&
			 my_fabs(p1[2] - p2[2]) < tolerance );
#endif
#if 1
	const float tolerance_sq = tolerance * tolerance;

	return ( square(p1[0] - p2[0]) < tolerance_sq &&
			 square(p1[1] - p2[1]) < tolerance_sq &&
			 square(p1[2] - p2[2]) < tolerance_sq );
#endif
#if 0
	return ( fabs(p1[0] - p2[0]) < tolerance &&
			 fabs(p1[1] - p2[1]) < tolerance &&
			 fabs(p1[2] - p2[2]) < tolerance );
#endif

#if 0
    const float dx = p1[0] - p2[0];
    const float dy = p1[1] - p2[1];
    const float dz = p1[2] - p2[2];
       
    return (dx*dx + dy*dy + dz*dz < tolerance * tolerance);
#endif

#if 0
	const float tolerance_sq = tolerance * tolerance;

	float dd = p1[0] - p2[0];
	if( dd * dd < tolerance_sq )
	{
		dd = p1[1] - p2[1];
		if( dd * dd < tolerance_sq )
		{
			dd = p1[2] - p2[2];
			return ( dd * dd < tolerance_sq );
		}
		else
		{
			return false;
		}
	}
	else
	{
		return false;
	}
#endif
}

int GetFirstUVtoXYZ_ID(const vertices *v, const int uv_id);
void GetVID3(object *obj, int g_id, int f_id, int *v_id1, int *v_id2, int *v_id3);
int InsertFaceGroup(object *obj, int m_id);
//int InsertBatchVertex(vertices *vl, int v_id, int uv_id, int m_id, int api_uv_id, int api_face_id);
int InsertBatchVertex(vertices *vl, const int xyz_id, const int uv_id, const int uv_id2, const int m_id);
//int GetBatchID(const vertices *vl, int v_id, int uv_id, int m_id, int api_uv_id);
int GetBatchID(const vertices *vl, const int xyz_id, const int uv_id, const int uv_id2, const int m_id);
//int GetUVID(const vertices *vl, float u, float v);
int GetUVID(const vertices *vl, const float uv[2], const int api_node_id, const int api_uv_id);
//int SameUV(const float u1, const float v1, const float u2, const float v2, const float tolerance);

// mod 1.0 u & v separately might not work because of repeat/clamp issues
inline bool SameUV(const float * const uv1, const float * const uv2, const float tolerance)
{
	return ( (fabs(uv1[0] - uv2[0]) <= tolerance) && (fabs(uv1[1] - uv2[1]) <= tolerance) );
}

void AdjustUV(float *uv_list, int count);
//int InsertUV(vertices *vl, float u, float v);
int InsertUVVertex(vertices *vl, const float uv[2], const int api_node_id, const int api_uv_id,
				   const int flags);
//int InsertVertex(vertices *vl, const float v[3], const float frozen[3], const int color[3]);
int InsertXYZVertex(vertices *vl, const float v[3], const int api_node_id, const int api_v_id,
					const int api_smg_id, const float frozen[3], const int color[3], const int flags);
int InsertPoly(object *obj, poly *p);
void InsertPolyTriangles(object *obj, poly *p);
//int calcFaceNormal(const faces *f, const vertices *v, int f_id, float normal[3]);
int calcFaceGroupNormal(const face_group *fg, const vertices *v, int f_id, float normal[3]);
int calcNormal(const float p1[3], const float p2[3], const float p3[3],
               float normal[3]);
//int CheckIndexing(vertices *v);
int GetDupVertexID(const poly *p);
//void RemoveVertex(vertices *v, int id);
int CheckPoly(poly *p);
int InsertNormal(normals *nl, float n[3], const float tolerance);

void PostProcessMesh(object *obj, const float nDensity, const float nLodPercent, const float nLodClosestDist,
					 const float nLodFurthestDist, const bool rigid_flag, const char *name,
					 const float scale);
void RemoveUnusedFaceGroups(object *obj);
void RemoveUnusedMaterials(object *obj);
void RemoveUnusedNormals(object *obj);
void SwapNormal(object *obj, const int id1, const int id2);
void SwapMaterial(object *obj, int id1, int id2);
//void SortGroupsByTexture(object *obj);
void MergeGroupsByMaterial(object *obj);
void MergeGroups(object *obj, const int id1, const int id2);
void RotateObjectRoot(object *obj, const Matrix& m);
//float GetTriangleArea(const faces *f, const vertices *v, int id);
void SwapLodFaces(void *swap_lib, const int id1, const int id2);
//void AutoSmooth(object *obj, float thresh_angle);
void calcVertexNormals(object *obj, int type);
int calcVertexNormal(object *obj, int type, int v_id, float vertex_normal[3]);
//void calcLODIndices(lod_object *l_obj);

//int CompareUVBonesByID(const void *b1, const void *b2);
//void SwapUVBones(void *v, const int id1, const int id2);
//void SwapUVBones(vertices *v, const int id1, const int id2);

float GetNewVertexAngle(const object *obj, int g_id, int f_id, int v_id);
//float GetVertexAngle(const faces *f, const vertices *v, int f_id, int v_id);
//float GetVertexAngle(int v_o_id, int v_count, const vertices *v, int v_id);
float GetVertexAngle(const float vertices[9], const int id /* 0-2 */);

void ScaleObject(object *obj, const float scale_factor);
void ScaleObjectHP(object *obj, const float scale_factor);
void ScaleObjectDeformable(object *obj, const float scale_factor);
void ScaleLodObject(lod_object *lod_obj, float scale_factor, float density);
void calcRigidBody(object *obj, float density /*=DENSITY*/, float mass/*=-1*/, const char name[64], bool tmp_flag);
void calcRigidBody(Extents& extents, const Vector *orig_vlist, const int orig_vcount, float density,
				   const char name[64]);
void GetVertsFromCylinder(Extents & extents, Vector **vlist, int *vcount);
void GetVertsFromBox(Extents & extents, Vector **vlist, int *vcount);
void GetVertsFromTube(Extents & extents, Vector **vlist, int *vcount);

bool FixIfFlat(Vector **vlist, int *vcount);
void RemoveLines(Vector **vlist, int *vcount, bool *is_flat);
bool IsFlatHull(const CollisionMesh & ch);
void AdjustHull(CollisionMesh *ch, const int vcount, const Vector *vlist, float scale);
void LoadPolyhedron(POLYHEDRON *p, const CollisionMesh *ch);
void ZeroBox(ExtentBox *box);
int OutOfSphere(float cx, float cy, float cz, float x, float y, float z, float r);

//int GetFaceID(const object *obj, int g_id, int f_id);
float QuadricEdgeError(object *obj, const int e_id);
float UVEdgeError(object *obj, const int e_id);
Matrix4 MergeQuadrics(object *obj, int v_id1, int v_id2, bool add_lists, int *sum_count);
void AddVertexQuadric(object *obj, int vid, int gid, int fid);
void calcEdgeErrors(object *obj);
void calcQuadrics(object *obj);

void SwapBatchIndecies(object *obj, const int id1, const int id2);
bool IsBatchUsed_N_or_Less_Times(const object *obj, const int vbid, const int t_count);
bool IsVertexUsed_N_or_Less_Times(const object *obj, const int vid, const int t_count);
void ReverseStepList(lod_lib *lol);
void SynchLodLib(lod_lib *lol, const Eedge_lib& el);
void AddLastEdge(object *obj, int step_id);
void SplitVertices(object *obj);
void AddUVSpaces(object *obj);
void SynchProperties(object *obj);
void SynchEdges(object *obj);
void SynchFreeBoneMeshVertices(vertices *v);
void SynchFreeBoneMeshVertices(DAMesh & da_mesh);
void SynchFreeBoneVertices(const int v_count, bone_vertex **bv_list, int **first_list, int **count_list,
							int **id_list, float **xyz_list, float **normal_list, float **weight_list,
							float **rational_list);
void SynchFreeBoneNurbCV(object *obj);
void SynchFreeBoneBezCV(Bezier_mesh *b_mesh);
void MergeBoneVertex(bone_vertex & dst, const bone_vertex & src1, const bone_vertex & src2,
					 const float w1, const float w2);
//int GetExceptFaceID(const Eedge_lib& el, int e_id, int not_fid);
void CollapseEdges(object *obj, int target_face_count);
float EdgeLength(const object *obj, int e_id);
void calcEdges(object *obj);
void calcEdgeAngles(object *obj);
//void SortEdgesByAngle(Eedge_lib *el);

void SortFacesByArea(object *obj);
//void StreamlineVertices(object *obj);
//void SwapFaces(void *swap_lib, const int id1, const int id2);
int CompareFacesByArea(const void *pt1, const void *pt2, const void *cmp_lib);

int CompareEdgesByAngle(const void *pt1, const void *pt2, const void *);
int CompareEdgesByError(const void *pt1, const void *pt2, const void *);
int InsertEdge(Eedge_lib *el, int v_id1, int v_id2);//, int g_id, int f_id, int uv_id1, int uv_id2);
void SetEdge_Face_UV(object *obj, const int e_id);
int GetEdgeID(const Eedge_lib& el, int v_id1, int v_id2, int count, int *swapped);
void BubbleSortEdgesByError(Eedge_lib *el);
bool checksorting(Eedge_lib *el);
int AllignEdgeByError(Eedge_lib *el, const int eid);
void SwapEdges(Eedge_lib *el, const int id1, const int id2);
void PutEdgeLast(Eedge_lib *el, const int eid);
void SwapEdges(void *el, const int id1, const int id2);
void RemoveLastEdge(object *obj);
void SwapVertices(object *obj, const int id1, const int id2);
void CombineDefVertices(object *obj, const int id1, const int id2);
void ConsolidateDuplicateAssignments(object *obj);
float AverageUV(float v1, float v2);

inline float Dot3(const float * const v1, const float * const v2)
	{ return(v1[0]*v2[0] + v1[1]*v2[1] + v1[2]*v2[2]); }

inline double Magnitude3(const float * const v)
	{ return sqrt(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]); }
inline double Magnitude3_sq(const float * const v)
	{ return (v[0]*v[0] + v[1]*v[1] + v[2]*v[2]); }

#ifndef SGI
#pragma warning( disable : 4244 ) // onversion from 'double' to 'float', possible loss of data
#endif

inline int Normalize3(float * const v)
{
  double magnitude_sq = v[0]*v[0] + v[1]*v[1] + v[2]*v[2];

  // avoid renormalizing already normalized values
  if(magnitude_sq < 1.000001 && magnitude_sq > 0.999999)
  {
	  return 0;
  }

  if(magnitude_sq > 0.0)
  {
    magnitude_sq = 1.0 / sqrt(magnitude_sq);
    v[0] *= magnitude_sq;
    v[1] *= magnitude_sq;
    v[2] *= magnitude_sq;

	return 0;
	//return (magnitude_sq > .0000001) ? 0 : -1;
  }

  //fprintf(stderr, "Zero length Normalize3!\n");
  return -1;
}

#ifndef SGI
#pragma warning( default : 4244 ) 
// conversion from 'double' to 'float', possible loss of data
#endif

void Cross(const float * const V1, const float * const V2, float * const V3);
float GetAngle(const float * const vec1, const float * const vec2);
float DistSq3(const float p1[3], const float p2[3]);
float Dist3(const float p1[3], const float p2[3]);
unsigned char nf2uc(float f);
void ReplaceChar(char *name, const char c1, const char c2);
void ReplaceFirstChar(char *name, const char c1, const char c2);
void ReplaceBeforeEqual(char *name);
int StripPrefix(char *name, const char *prefix);
void FixName(char *name);

void LoadLodObject(lod_object *l_obj, file_node *root);
void LoadHP(object *obj, file_node *root);
void Load3DBObject(object *obj, file_node *root);
void LoadVertices(vertices *v, file_node *root);
void LoadNormals(normals *n, file_node *root);
void LoadRigidBody(Extents *extents, file_node *root);
void Old2New(object *obj);

void Matrix_to_PersistMatrix(Matrix *m, PersistMatrix *pm);
void DumpLodLib(const lod_lib& lol);
void DumpLodStep(const lod_lib& lol, int step_id);

#endif // THREEDB_H
