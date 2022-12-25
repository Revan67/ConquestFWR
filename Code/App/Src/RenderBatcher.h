#ifndef RENDERBATCHER_H
#define RENDERBATCHER_H

const U32 RPR_OPAQUE_POOL_SIZE_DEFAULT		= 512*1024;
const U32 RPR_TRANSLUCENT_POOL_SIZE_DEFAULT	= 64*1024;

//
const unsigned int DefaultListLength = 1024 * 16;
static RPVertex lvertex_list[DefaultListLength];
static U16 index_list[DefaultListLength];

//

struct PRIMITIVETYPEINFO {
	U32 vert_fixed_cost;		//	           (num_vertices - vert_fixed_cost) 
								// num_prims =  ------------------------------           
	U32 vert_per_prim_cost;		//                    vert_per_prim_cost
	D3DPRIMITIVETYPE type;		// type used when rendering
	U32	vert_count;				// count used when rendering
};

static PRIMITIVETYPEINFO primitive_type_info[7] = {
	{1,1,D3DPT_POINTLIST,1},
	{0,1,D3DPT_POINTLIST,1},
	{0,2,D3DPT_LINELIST,2},
	{1,1,D3DPT_LINELIST,2},
	{0,3,D3DPT_TRIANGLELIST,3},
	{2,1,D3DPT_TRIANGLELIST,3},
	{2,1,D3DPT_TRIANGLELIST,3}
};

//

#define BM_M_F_HAS_BLEND_ENABLE	(1<<0)
#define BM_M_F_HAS_BLEND_FUNC	(1<<1)
#define BM_M_F_HAS_BLEND		(BM_M_F_HAS_BLEND_ENABLE|BM_M_F_HAS_BLEND_FUNC)
#define BM_M_F_HAS_TEXTURE		(1<<2)
#define BM_M_F_HAS_DEPTH_ENABLE	(1<<3)
#define BM_M_F_HAS_DEPTH_FUNC	(1<<4)
#define BM_M_F_HAS_DEPTH		(BM_M_F_HAS_DEPTH_ENABLE|BM_M_F_HAS_DEPTH_FUNC)
#define BM_M_F_HAS_CULL			(1<<5)
#define BM_M_F_HAS_DITHER		(1<<6)
#define BM_M_F_HAS_FOG			(1<<7)
#define BM_M_F_DELAY_ON			(1<<8)
#define BM_M_F_HAS_STENCIL		(1<<9)
#define BM_M_F_HAS_ZBIAS		(1<<10)
#define BM_M_F_HAS_INDICES		(1<<11)

//

#define BM_NUM_STAGES 4

//

struct BATCHEDCALLPRIMITIVE
{
public:	// Data

	Transform	modelview;

	BATCHEDCALLPRIMITIVE *next,*prev;

	D3DPRIMITIVETYPE	 p_type;
	U32					 p_flags;
	
	U32					 verts_cnt;
//	U32 				verts;
	U32					 verts_type;

	U32					 indices_cnt;
//	U32					indices;

	float				 p_depth;

	BMPOOLJR			*vpool;
	BMPOOLJR			*ipool;

	bool bOwnsPools:1;

public:	// Interface

	//

	void initialize( );

	void render( );
	
/*	float dip_tri_depth( U16 *v0_idx, U32 sizeof_vert, float *MT );

	void sort_dip( int first, int last, float *MT );

	float dp_tri_depth( Vector *v0, U32 sizeof_vert, float *MT );

	void sort_dp( int first, int last, float *MT );

	void sort();

	float depth();*/

};

#define BATCH_HASH_SIZE 100
#define STATE_ID_LIST_LENGTH 100

struct STATE_ID
{
	U32 id;
	STATE_ID *next,*prev;
	struct BATCHEDMATERIAL *owner;
};

static STATE_ID state_id_list[STATE_ID_LIST_LENGTH];
static U32 current_state_id=0;

#define GROW_SIZE 16192

struct BATCHEDMATERIAL
{
public:	// Data

	U32 flags;

	U32 blend_enable;
	U32 blend_src;
	U32 blend_dst;

#define _ALPHAFUNC
#ifdef _ALPHAFUNC
	U32 alpha_test_enable;
	U32 alpha_ref;
	U32 alpha_func;
#endif

	U32 texture_stage_cnt;
	U32 texture_stage_state[BM_NUM_STAGES][D3DTSS_MAX_STATE];
//	U32 texture_stage_wrap[BM_NUM_STAGES];
	U32 texture_stage_texture[BM_NUM_STAGES];

	U32 depth_enable;
	U32 depth_write_enable;
	U32 depth_func;

	U32 cull_mode;

	U32 dither_enable;

	U32 wrap0,wrap1;

#ifdef _FOG
	U32 fog_enable;
	U32 fog_start;
	U32 fog_end;
	U32 fog_density;
	U32 fog_mode;
	U32 fog_color;
#endif

//#define _STENCIL
#ifdef _STENCIL
	U32 stencil_enable;
	U32 stencil_ref;
	U32 stencil_func;
	U32 stencil_pass;
#endif

	U32 delay;
	U32 lastDrawTick;

	U32 z_bias;

	//U32 vertex_format;
	//D3DPRIMITIVETYPE type;

	bool bPreTransform:1;
	bool bLocked:1;

#ifdef MODELVIEW_STRAT
	Transform inv;
#endif

	//LList<struct BATCHEDCALLPRIMITIVE> primitives;
	struct BATCHEDCALLPRIMITIVE prim;

	// used for hash table
	LList<STATE_ID> state_id[BATCH_HASH_SIZE];
	BATCHEDMATERIAL *next, *prev;
	//DEBUG ONLY
//	struct BMPOOL *owner;
private:

//	BMPOOLJR *vpool;
//	BMPOOLJR *ipool;

public:
	BATCHEDMATERIAL()
	{
	//	vpool.set_size(GROW_SIZE*2);
	//	ipool.set_size(GROW_SIZE);
//		vpool = ipool = 0;
		prim.initialize();
		delay=0;
		lastDrawTick=0;
		bLocked = false;
//		modelView.set_identity();
	}

	~BATCHEDMATERIAL()
	{
		for( U32 b=0; b<BATCH_HASH_SIZE; b++ ) {
			state_id[b].free();
		}
	}

public: // Interface

	bool is_free ();

	bool is_equal( BATCHEDMATERIAL *material );

	void save(struct RenderBatcher *batcher, U32 save_flags , D3DPRIMITIVETYPE type, U32 vertex_format);

	void restore( U32 restore_flags );

	void empty_primitive();

	void reset_primitives();

	bool owns_state_id(U32 id);
	
	void add_state_id(U32 id);

	void remove_state_id(U32 id);

	void free_state_ids();

/*	BMPOOLJR *get_v_pool();

	BMPOOLJR *get_i_pool();*/

	void get_pools(BMPOOLJR **vpool,BMPOOLJR **ipool);

	void set_modelview(const Transform &trans);

};
//


struct PRIMITIVESORTDATA
{
	struct BATCHEDMATERIAL		 *material;
	struct BATCHEDCALLPRIMITIVE *primitive;
};

//

//
struct BMPOOL 
{
	struct BATCHEDMATERIAL *materials;				// Saved material/primitive state.
	U32				 materials_cnt;			// Number of materials in materials array
	U32				 materials_max_cnt;		// Max number of materials used in a frame so far
	U32				 materials_used_cnt;	// Number of materials used this frame
	
	//

#define BM_P_F_ENABLE	(1<<0)
#define BM_P_F_EMPTY	(1<<1)

#define MH_MAX_BUCKETS 121

//
//	LList<BATCHEDMATERIAL> materials_hash[MH_MAX_BUCKETS];

/*	unsigned char	*pool;
	U32				 pool_used_cnt;			// index
	U32				 pool_size;*/
	U32				 pool_flags;
	U32 mats_rendered;

	const char		*name;
	
	//

	struct RenderBatcher *batcher;

/*	U32 pc_save ;
	U32 pc_render ;
	U32 pc_restore ;*/

//	U32 maxDelay;
	U32 hint_mat;

public:	// Interface

	BMPOOL( )
	{
		name = "";

		materials_used_cnt = 0;
		materials_cnt = 0;
		materials = NULL;

	//	set_materials_size( 0 );

	/*	pool = NULL;
		pool_used_cnt = 0;
		pool_size = 0;*/
		pool_flags = BM_P_F_EMPTY;

	/*	pc_render = 0;
		pc_save = 0;
		pc_restore = 0;*/

//		maxDelay = 0;

		hint_mat = 0xffffffff;

		mats_rendered = 0;

	}

	//

	~BMPOOL()
	{
		reset();
		delete[] materials;
		materials = NULL;
		materials_cnt = 0;

//		delete[] pool;
//		pool = NULL;
	}

	void set_name( const char *_name );

//	void set_size( U32 new_pool_size );

	void set_materials_size( U32 new_materials_size );

	void set_enabled( bool enabled );

	bool is_enabled( );

	bool is_empty( );

/*	U32 materials_hash_val( U32 state_id );

	void materials_hash_reset( );

	void materials_hash_link( BATCHEDMATERIAL *material );

	void materials_hash_unlink( BATCHEDMATERIAL *material );*/

//	void *allocate( U32 size, bool set_non_empty=false );

//	void free_all();

	void reset();

	void update( void );

	U32 find_material( U32 state_id );

	U32 allocate_material( U32 state_id );

	U32 find_matching_material( BATCHEDMATERIAL *tmp_mat );

	//U32 install_new_state_id( U32 state_id, D3DPRIMITIVETYPE type, U32 vertex_format);

	U32 get_hint_from_state_id(U32 state_id, D3DPRIMITIVETYPE type, U32 vertex_format);

	BATCHEDMATERIAL * get_material_from_state_id(U32 state_id, D3DPRIMITIVETYPE type, U32 vertex_format);

	GENRESULT add_primitive(  U32 state_id, U32 save_flags, D3DPRIMITIVETYPE type, U32 vertex_format, const void *verts, int num_verts, const U16 * indices, int num_indices, U32 flags );

	void render_no_sort(  );

	void render_depth_sort( DynamicArray<PRIMITIVESORTDATA> &sort_pool );

};

#endif