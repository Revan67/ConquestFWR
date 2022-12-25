#ifndef __NURBMESH_H
#define __NURBMESH_H

#define RATIONAL_SUPPORT 1
#define DIVIDE_BY_W (1 && RATIONAL_SUPPORT)
#define USE_CACHE 1
#define CHORD_NORMALIZE 1

#define UV_ON		0x1
#define LIGHT_ON	0x2
//#define DEFAULT_PIXEL_ERROR 0.005f // view dependent

#include "TComponent.h"
#include "ITextureLibrary.h"
#include "stddat.h"
#include "XNURB.h"
#include "IRenderComponent.h"
#include "lightman.h"
#include "heapobj.h"
#include "engine.h"

#include "rendpipeline.h"
#include "IRenderPrimitive.h"
#ifdef _DEBUG
#include "RPUL/PrimitiveBuilder.h"
#endif

#include "iigloo/iigloo_util.h"
#include "fdump.h"

//

#include "handlemap.h"
#include "Tfuncs.h"

//


#if DA_ERROR_LEVEL >= __SEV_TRACE_5
inline void ii_result ( const HRESULT result )
{
	if(S_OK != result)
	{
		const HRESULT decoded_result = (result & 0xffff) - 0x8000;
		GENERAL_FATAL("IIGLOO failed!\n"); // IIGLOOERROR_IO_DATA_UNAVAILABLE
	}
}
#else
inline void ii_result ( const HRESULT result ){ ASSERT_FATAL( S_OK == result); }
#endif

void TrapFpu(bool on);

struct NURBInstance
{
	XNURB *xnurb;
	Matrix4 view_transform;
	IIEvaluationHandle **iicache_hangle_list;
	IIPolynomialEvaluator *ieval_r;
	IIPolynomialEvaluator *ieval_nr;

	NURBInstance(void);
	void initialize (void);

	~NURBInstance(void);
	void free (void);
#if USE_CACHE
	void AssignCacheHandles(void);
#endif
};

//

struct f_LightRGB
{
	float r, g, b;
};

//

extern struct IEngine *	ENG;

//

typedef inst_handlemap< NURBInstance* >	inst_map;
typedef rarch_handlemap< XNURB* >		rarch_map;

//


//****************************************************************************
//*                                                                          *
//*  NURBMESH class declaration                                              *
//*                                                                          *
//****************************************************************************

struct DACOM_NO_VTABLE NURBMESH : public IRenderComponent
{
	typedef enum {NON_RATIONAL, RATIONAL} ev_type;

	protected:

		ITextureLibrary *txm_lib;
		bool			txm_lib_owned;

		ILightManager *	LIGHTMAN;

		IRenderPipeline	*PIPE;
		IRenderPrimitive *BATCH;

		IIGLOO *		iigloo;

		IIPolynomialEvaluator *	evaluator_nr;	// non rational
		IIPolynomialEvaluator *	evaluator_r;	// rational

		ev_type					active_type;
		IIPolynomialEvaluator * evaluator;		// one of evaluator_nr or evaluator_r

		// used for lighting
		IIPolynomial *	r_polynom[NUM_ORDERS][NUM_ORDERS];
		IIPolynomial *	g_polynom[NUM_ORDERS][NUM_ORDERS];
		IIPolynomial *	b_polynom[NUM_ORDERS][NUM_ORDERS];

		PolynomialEnumData	poly_data_user[NUM_ORDERS * NUM_ORDERS]; // NURB
		PolynomialEnumData	poly_data_linear;  // UV

		//IIView	*		*iview; // could be used for culling but we do our own
	
		//set of archetypes that are used in response to calls via IRenderComponent
		//DynamicArray < TPointer <XNURB> > render_archetypes;
		rarch_map render_archetypes;

		//really only object instances.
		//DynamicArray <NURBInstance> instances;
		inst_map instances;

#ifdef _DEBUG
		PrimitiveBuilder pb;
#endif

		BEGIN_DACOM_MAP_INBOUND(NURBMESH)
		DACOM_INTERFACE_ENTRY(IRenderComponent)
		DACOM_INTERFACE_ENTRY2(IID_IRenderComponent,IRenderComponent)
		END_DACOM_MAP()

		NURBMESH (void);
		~NURBMESH (void);

		DA_HEAP_DEFINE_NEW_OPERATOR(NURBMESH);

		
		GENRESULT init (RendCompDesc* info);
		bool InitPolynomials(void);
		bool PolyEnumData(void);

		vis_state render(ICamera * camera, INSTANCE_INDEX index, U32 flags, const Transform *tr,
			float lod_fraction);
		void render_faces(const IITessellationType tessellation_type, const IIUInt point_count, 
			const IIUInt index_count, const IIInt generated_point_offset, 
			const IIInt generated_point_count, const Material & mat,
			XNURB * xnurb, const U32 clip);

		void AssignDataToPolynomials(const NURBPatch & patch, const IIUInt row, const IIUInt column,
									const int s_basis, const int t_basis, const int which);
		void AssignPolynomialsToEvaluator(const NURBPatch & patch, const int s_basis, const int t_basis,
									const int which);
		void SetPatchEvaluatorState(const NURBPatch & patch);
		void LightPatch (const Transform & light_world_to_object, const NURBPatch & patch,
									  const Material & mat, const ClampFlags light_clamp);
		vis_state CullPatch(const NURBPatch & patch, const Vector & cam_pos_in_object_space);
		
		void GenerateNormals(XNURB * xnurb);
#if CHORD_NORMALIZE
		void GenerateUVs(XNURB * xnurb);
#endif

		void SetEvaluatorDefaults(void);
		void ActivateEvaluator(const ev_type type);
		
	public:
	// IRenderComponent stuff
	GENRESULT COMAPI set_render_property( const RenderProp name, DACOM_VARIANT value );
	GENRESULT COMAPI get_render_property( const RenderProp name, DACOM_VARIANT out_value );
	void COMAPI update( float dt );

	bool COMAPI create_archetype( RENDER_ARCHETYPE render_arch_index, IFileSystem *filesys ) ;
	bool COMAPI duplicate_archetype( RENDER_ARCHETYPE new_render_arch_index, RENDER_ARCHETYPE old_render_arch_index ) ;
	void COMAPI destroy_archetype( RENDER_ARCHETYPE render_arch_index ) ;
	bool COMAPI split_archetype( RENDER_ARCHETYPE render_arch_index, const Vector& normal, float d, RENDER_ARCHETYPE r0, RENDER_ARCHETYPE r1, U32 sa_flags, INSTANCE_INDEX inst_index ) ;
	bool COMAPI get_archetype_statistics( RENDER_ARCHETYPE render_arch_index, float lod_fraction, enum StatType statistic, DACOM_VARIANT out_value ) ;
	bool COMAPI get_archetype_bounding_box( RENDER_ARCHETYPE render_arch_index, float lod_fraction, SINGLE out_box[6] ) ;
	bool COMAPI get_archetype_centroid( RENDER_ARCHETYPE render_arch_index, float lod_fraction, Vector& out_centroid ) ;
	bool COMAPI query_archetype_interface( RENDER_ARCHETYPE render_arch_index, const char *iid, IDAComponent **out_iid )
	{
		return false;
	}

	bool COMAPI create_instance( INSTANCE_INDEX inst_index, RENDER_ARCHETYPE render_arch_index ) ;
	void COMAPI destroy_instance( INSTANCE_INDEX inst_index ) ;
	void COMAPI update_instance( INSTANCE_INDEX inst_index, float dt ) ;
	vis_state COMAPI render_instance( INSTANCE_INDEX inst_index, RENDER_ARCHETYPE render_arch_index, struct ICamera *camera, float lod_fraction, U32 flags, const Transform *tr ) ;
	bool COMAPI get_instance_bounding_box( INSTANCE_INDEX inst_index, RENDER_ARCHETYPE render_arch_index, float lod_fraction, SINGLE out_box[6] ) ;		
	bool COMAPI query_instance_interface( INSTANCE_INDEX inst_index, RENDER_ARCHETYPE render_arch_index, const char *iid, IDAComponent **out_iid ) 
	{
		return false;
	}

	// These will eventually go away, feel free to implement them in the slowest way
	// possible.
	//
	struct Mesh * COMAPI get_instance_mesh( INSTANCE_INDEX inst_index, RENDER_ARCHETYPE render_arch_index )
		{ return NULL; }
	struct NURB * COMAPI get_instance_NURB( INSTANCE_INDEX inst_index, RENDER_ARCHETYPE render_arch_index );
	struct Mesh * COMAPI get_unique_instance_mesh( INSTANCE_INDEX inst_index, RENDER_ARCHETYPE render_arch_index )
		{ return NULL; }
	GENRESULT COMAPI release_unique_instance_mesh( INSTANCE_INDEX inst_index, RENDER_ARCHETYPE render_arch_index )
		{ return GR_GENERIC; }
	struct Mesh * COMAPI get_archetype_mesh( RENDER_ARCHETYPE render_arch_index )
		{ return NULL; }
	struct NURB * COMAPI get_archetype_NURB( RENDER_ARCHETYPE render_arch_index );

// Render stuff
	protected:
		int				vertex_pool_len;
		RPVertex *		vertex_pool;	// holds vertices that will be submitted to IRP
		Vector *		light_pool;
		
		int				index_list_len;
		U16 *			index_list;		// holds indices into vertex_pool to submit to IRP	
	
		void verify_pools( int size );
		void delete_pools( void );
		void verify_lists( int size );
		void delete_lists( void );
		
		int				visible_vertex_cnt;
		bool *			visible_vertex; // keeps track of visible vertices of a patch
		f_LightRGB *	light_points;
		LightRGB *		tmp_lights;	// should go away by making light_vertices handle floats
		U32 *			light_indices;

		int				visible_patch_cnt;
		vis_state *		visible_patch;  // keeps track of visible sub patches
		float			pixel_error;
};

#endif
