#ifndef __BEZIERMESH_H
#define __BEZIERMESH_H

#include "TComponent.h"
#include "ITXMLib.h"
#include "stddat.h"
#include "Bezier.h"
#include "IRenderComponent.h"
#include "lightman.h"
#include "heapobj.h"

#include "rendpipeline.h"
#include "IRenderPrimitive.h"
#include "RPUL/PrimitiveBuilder.h"
#include "Tfuncs.h"

#include "fdump.h"

#define MAX_SUB_DIV_CNT 3 // this makes 4^4 = 8x8 = 256 patches out of one patch
#define EDGE_LENGTH 9 // one side of 8 by 8

//#define MAX_SUB_DIV_CNT 4
//#define EDGE_LENGTH 17

void TrapFpu(bool on);

struct BezierInstance
{
	BezierMesh *patch;
	Matrix4 view_transform;
	
	BezierInstance(void);
	void initialize (void);

	~BezierInstance(void);
	void free (void);
};

//

struct f_LightRGB
{
	float r, g, b;
};

typedef Vector Vector_4[4];
typedef float float_4[4];
typedef U8 Corner[6]; // 01 - horizontal \; 23 - vertical |; 34 - unused /

struct StitchEdge
{
	int pid1;				// source (uber) patch id 1
	int pid2;				// source (uber) patch id 2
	int el1[EDGE_LENGTH];
	int el2[EDGE_LENGTH];

	int depth1;
	int depth2;
};

//

extern struct IEngine *	ENG;

//****************************************************************************
//*                                                                          *
//*  BEZIERMESH class declaration                                            *
//*                                                                          *
//****************************************************************************

struct DACOM_NO_VTABLE BEZIERMESH : public IRenderComponent
{
	typedef enum {NON_RATIONAL, RATIONAL} ev_type;

	private:

		ITXMLib *		txm_lib;
		bool			txm_lib_owned;

		ILightManager *	LIGHTMAN;

		IRenderPipeline	*PIPE;
		IRenderPrimitive *BATCH;

		//set of archetypes that are used in response to calls via IRenderComponent
		DynamicArray < TPointer <BezierMesh> > render_archetypes;

		//really only object instances.
		DynamicArray <BezierInstance> instances;

		PrimitiveBuilder pb;

		// pool of patches being rendered
		int							patch_list_len;
		int							patch_list_index;
		BezierPatch	*				patch_list;
		Vector_4 *					patch_normals;
		float_4	*					patch_D;
		Corner *					patch_corners;
		int	*						patch_depth;
		const int **				split_direction;
		bool *						patch_alt_tri_style;

		// patch edges used to stitch cracks
		int							edge_list_len;
		int							edge_list_index;
		StitchEdge *				edge_list;

		// buffers to renderpipe
		int							vertex_pool_len;
		int							vertex_pool_index;
		RPVertex *					vertex_pool;
		Vector *					normal_pool;
		int *						normal_pool_cnt;	// number of contributing patches
		U32 *						normal_index_pool;	// used by light manager
		LightRGB *					light_pool;

		// scratch buffers
		int							scratch_xyz_index;
		Vector *					scratch_xyz;
		int *						scratch_xyz_rp_idx;
		int *						scratch_xyz_uv_idx;
		int							scratch_uv_index;
		TexCoord *					scratch_uv;

		// triangle indices into vertex_pool
		int							index_list_len;
		int							index_list_index;
		U16 *						index_list;

		int							sub_div_cnt;
		float						pixel_error;

		BEGIN_DACOM_MAP_INBOUND(BEZIERMESH)
		DACOM_INTERFACE_ENTRY(IRenderComponent)
		END_DACOM_MAP()

		BEZIERMESH (void);
		~BEZIERMESH (void);

		void * operator new (size_t size)
		{
			return HEAP->ClearAllocateMemory(size, "BEZIERMESH");
		}
		
		GENRESULT init (RendCompDesc* info);
		bool InitPolynomials(void);

		vis_state render(ICamera * camera, INSTANCE_INDEX index, U32 flags, const Transform *tr,
			float lod_fraction);
		
	public:
		// IRenderComponent stuff
			
		GENRESULT COMAPI set_render_property( const RenderProp name, DACOM_VARIANT value );
		GENRESULT COMAPI get_render_property( const RenderProp name, DACOM_VARIANT out_value );
		void COMAPI update( float dt );

		bool COMAPI create_archetype( RENDER_ARCHETYPE render_arch_index, IFileSystem *filesys );
		bool COMAPI duplicate_archetype( RENDER_ARCHETYPE new_render_arch_index, RENDER_ARCHETYPE old_render_arch_index );
		void COMAPI destroy_archetype( RENDER_ARCHETYPE render_arch_index );
		bool COMAPI split_archetype( RENDER_ARCHETYPE render_arch_index, const Vector& normal, float d, RENDER_ARCHETYPE r0, RENDER_ARCHETYPE r1, U32 sa_flags, INSTANCE_INDEX inst_index );
		bool COMAPI get_archetype_statistics( RENDER_ARCHETYPE render_arch_index, float lod_fraction, enum StatType statistic, DACOM_VARIANT out_value );
		bool COMAPI get_archetype_bounding_box( RENDER_ARCHETYPE render_arch_index, float lod_fraction, SINGLE out_box[6] );
		bool COMAPI get_archetype_centroid( RENDER_ARCHETYPE render_arch_index, float lod_fraction, Vector& out_centroid );
		bool COMAPI query_archetype_interface( RENDER_ARCHETYPE render_arch_index, const char *iid, IDAComponent **out_iid );

		bool COMAPI create_instance( INSTANCE_INDEX inst_index, RENDER_ARCHETYPE render_arch_index );
		void COMAPI destroy_instance( INSTANCE_INDEX inst_index );
		void COMAPI update_instance( INSTANCE_INDEX inst_index, float dt );
		vis_state COMAPI render_instance( INSTANCE_INDEX inst_index, RENDER_ARCHETYPE render_arch_index, struct ICamera *camera, float lod_fraction, U32 flags, const Transform *tr );
		bool COMAPI get_instance_bounding_box( INSTANCE_INDEX inst_index, RENDER_ARCHETYPE render_arch_index, float lod_fraction, SINGLE out_box[6] );		
		bool COMAPI query_instance_interface( INSTANCE_INDEX inst_index, RENDER_ARCHETYPE render_arch_index, const char *iid, IDAComponent **out_iid );


		// These will eventually go away, feel free to implement them in the slowest way
		// possible.
		//
		struct Mesh * COMAPI get_instance_mesh( INSTANCE_INDEX inst_index, RENDER_ARCHETYPE render_arch_index ) { return NULL; }
		struct Mesh * COMAPI get_unique_instance_mesh( INSTANCE_INDEX inst_index, RENDER_ARCHETYPE render_arch_index ) { return NULL; }
		GENRESULT COMAPI release_unique_instance_mesh( INSTANCE_INDEX inst_index, RENDER_ARCHETYPE render_arch_index ) { return GR_GENERIC; }
		struct Mesh * COMAPI get_archetype_mesh( RENDER_ARCHETYPE render_arch_index ) { return NULL; }

// Render stuff
	private:
		
		void verify_pools( const int size );
		void delete_pools( void );
		void verify_lists( const int size );
		void delete_lists( void );
		void verify_edge_lists( const int size );
		void delete_edge_lists( void );
		void verify_patch_lists( const int size );
		void delete_patch_lists( void );
		void EvalPatches( const PatchGroup & group );
		void SplitPatches( const PatchGroup & group, const float tolerance );
		void GetPatchError( const int id, float err[3], Vector & normal, float & D );
		void CalcPatchPlane( const int pid, Vector & normal, float & D );
		int SplitPatch4Vertical( const int id1, const int id2, const int src_pid );
		int SplitPatch4Horizontal( const int id1, const int id2, const int src_pid );
		int SplitPatch3( const int id1, const int id2, const int direction, const int src_pid );
		void CalcPatchNormals( const int pid );
		void TrapFpu(bool on);

		void CheckEdges( const PatchGroup & group );
};

#endif
