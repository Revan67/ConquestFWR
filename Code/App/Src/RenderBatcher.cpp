// RenderBatcher.cpp
//
//

#include "pch.h"
#include <globals.h>

#include "CQTrace.h"
#include "tcomponent.h"
#include "stddat.h"
#include "FDump.h"
#include "pixel.h"
#include "3dmath.h"
#include "rendpipeline.h"
#include "IRenderPrimitive.h"
//#include "RPUL.h"
#include "MyVertex.h"
#include "TSmartPointer.h"
#include "IProfileParser_Utility.h"
#include "BMPoolJr.h"
#include "RenderBatcher.h"
#include "CQBatch.h"
#include <DEffectOpts.h>

#include <IVertexBufferManager.h>

#include "TempStr.h"

#ifndef EDITOR
	#define USE_HEAP 1
#endif

#if USE_HEAP
#include "HeapObj.h"
#endif


#include "FVF.h"
#define BIG_BATCH 202
#define SCRATCH_BUFF_SIZE 12000

U8 vert_buff[SCRATCH_BUFF_SIZE];
int buff_pos=0;
int buffer_ref_cnt=0;
U16 id_list1[10000];
U16 id_list2[10000];
int id_list_pos1=0,id_list_pos2=0;
//
static int in=0;

#define FLUSH_INTERNAL 0x00010000

//U32 activeStatesFlags = 0xffffffff;

// Macro to define supported methods on this interface
//
#undef DA_METHOD
#define DA_METHOD(name,params) virtual GENRESULT COMAPI name params;

unsigned int delayStage=0,maxDelay=0;
RenderBatcher *BATCHER = 0;

//Vertex Buffers
RPVertex *vb1;
Vertex2 *vb2;
IRP_VERTEXBUFFERHANDLE vb_handle1,vb_handle2;
int vb_used1,vb_used2;

struct QueuedBatch
{
	int start_vert;
	Transform modelView;
	BATCHEDMATERIAL *material;
	int num_verts;
	int id_offset;
	int num_indices;
	D3DPRIMITIVETYPE prim_type;
};

#define QUEUE_LENGTH 30
QueuedBatch batchQueue1[QUEUE_LENGTH];
QueuedBatch batchQueue2[QUEUE_LENGTH];
int q_cnt1,q_cnt2;



struct RenderBatcher : 	IRenderPrimitive,
						IAggregateComponent,
						ICQBatch

{
	BEGIN_DACOM_MAP_INBOUND(RenderBatcher)
	DACOM_INTERFACE_ENTRY(IRenderPrimitive)
	DACOM_INTERFACE_ENTRY2(IID_IRenderPrimitive,IRenderPrimitive)
	DACOM_INTERFACE_ENTRY(IAggregateComponent)
	DACOM_INTERFACE_ENTRY2(IID_IAggregateComponent,IAggregateComponent)
	DACOM_INTERFACE_ENTRY(ICQBatch)
	DACOM_INTERFACE_ENTRY2(IID_ICQBatch,ICQBatch)
	END_DACOM_MAP()

public:		// public interface
    
	// IAggregateComponent 
	GENRESULT COMAPI Initialize(void);
	GENRESULT init(AGGDESC *desc);

	// IRenderPrimitive
	DA_METHOD(	set_state,					(RPRSTATE state, U32 value ));
	DA_METHOD(	get_state,					(RPRSTATE state, U32 *value ));

	DA_METHOD(	set_modelview,				(const Transform & modelview ));
	DA_METHOD(	get_modelview,				(Transform & modelview ));

	DA_METHOD(	set_viewport,				(int x, int y, int w, int h ));
	DA_METHOD(	get_viewport,				(int *x, int *y, int *w, int *h ));

	DA_METHOD(	set_ortho,					(float left, float right, float bottom, float top, float nearval=-1.0, float farval=1.0 ));
	DA_METHOD(	set_perspective,			(float fovy, float aspect, float znear, float zfar ));
	DA_METHOD(  set_projection,				(const Matrix4 &projection ));
	DA_METHOD(  get_projection,				(Matrix4 &projection ));

	virtual GENRESULT COMAPI set_material( D3DMATERIAL9 *material_values ) { return GR_GENERIC;}
	virtual GENRESULT COMAPI get_material( D3DMATERIAL9 *out_material_values ) {return GR_GENERIC;}

	DA_METHOD(	begin_scene,				(void ));
	DA_METHOD(	end_scene,					(void ));

	DA_METHOD(	set_render_state,			(D3DRENDERSTATETYPE state, U32 value ));
	DA_METHOD(	get_render_state,			(D3DRENDERSTATETYPE state, U32 *value ));
	DA_METHOD(  set_texture_stage_state,	(U32 stage, D3DTEXTURESTAGESTATETYPE, U32 value ));
	DA_METHOD(  get_texture_stage_state,	(U32 stage, D3DTEXTURESTAGESTATETYPE, U32 *value ));
	
	DA_METHOD(  set_sampler_state,	(U32 stage, D3DSAMPLERSTATETYPE, U32 value ));
	DA_METHOD(  get_sampler_state,	(U32 stage, D3DSAMPLERSTATETYPE, U32 *value ));
	
	DA_METHOD(  set_texture_stage_texture,	(U32 stage, U32 htexture ));
	DA_METHOD(  get_texture_stage_texture,	(U32 stage, U32 *htexture ));
	DA_METHOD(	verify_state,				());
	
	DA_METHOD(	draw_primitive,				(D3DPRIMITIVETYPE type, U32 vertex_format, const void *verts, int num_verts, U32 flags ));
	DA_METHOD(	draw_indexed_primitive,		(D3DPRIMITIVETYPE type, U32 vertex_format, const void *verts, int num_verts, const U16 * indices, int num_indices, U32 flags ));

	virtual GENRESULT COMAPI draw_primitive_vb( D3DPRIMITIVETYPE type, IRP_VERTEXBUFFERHANDLE vbhandle, int start_vert, int num_verts, U32 flags  ) { return GR_GENERIC;}
	virtual GENRESULT COMAPI draw_indexed_primitive_vb( D3DPRIMITIVETYPE type, IRP_VERTEXBUFFERHANDLE vbhandle, int start_vert, int num_verts, const U16 * indices, int num_indices, U32 flags  ) {return GR_GENERIC;}


	DA_METHOD(	flush,						(U32 flags));

	//ICQBatch methods

	virtual unsigned __int64 GetTicks()
	{
		return ticks;
	}

	virtual void ClearTicks()
	{
		ticks = 0;
	}

	virtual void Reset()
	{
		for (int i=0;i<3;i++)
		{
			bm_pools[i].reset();
		}

		for (int i=0;i<BM_NUM_STAGES;i++)
			currentTextureID[i] = -1;
	}

	virtual U32 CreateMaterial (U32 stateID,D3DPRIMITIVETYPE _type,U32 _vertex_format)
	{
		U32 alpha;
		PIPE->get_render_state( D3DRS_ALPHABLENDENABLE, &alpha );
		U32 depth = (state[RPR_BATCH_TRANSLUCENT_MODE]==RPR_TRANSLUCENT_UNSORTED)?0:1;
		U32 pool  = (alpha)?(depth)?1:2:0;
		U32 hintID = bm_pools[pool].get_hint_from_state_id(stateID,_type,_vertex_format);
		hintID |= pool << 30;  //put the pool in the two high bits
		return hintID;
	}

	//true means the material was successfully loaded
	//false means user must recreate it
	virtual bool ActivateMaterial (U32 stateID,U32 hintID)
	{
		U32 pool = hintID >> 30;
		U32 matID = hintID & 0x3fffffff;
		if (matID < bm_pools[pool].materials_used_cnt && bm_pools[pool].materials[matID].owns_state_id(stateID))
		{
			activeMaterialHint = hintID;
			return true;
		}
		else
			return false;
	}

	virtual void InvalidateMaterial (U32 stateID)
	{
		for (int i=0;i<3;i++)
		{
			U32 matID;
			matID = bm_pools[i].find_material (stateID);
			if (matID != U32(-1))
			{
				bm_pools[i].materials[matID].remove_state_id(stateID);
			}
		}
	}

	virtual void SetMStateID(U32 stateID,U32 &_hintID,D3DPRIMITIVETYPE _type,U32 _vertex_format)
	{
		state[RPR_STATE_ID] = stateID;
		if (stateID == 0)
		{
			_hintID = 0xffffffff;
			return;
		}
		U32 alpha;
		PIPE->get_render_state( D3DRS_ALPHABLENDENABLE, &alpha );
		U32 depth = (state[RPR_BATCH_TRANSLUCENT_MODE]==RPR_TRANSLUCENT_UNSORTED)?0:1;
		U32 pool  = (alpha)?(depth)?1:2:0;

		BATCHEDMATERIAL *material=0;
		if (_hintID < bm_pools[pool].materials_used_cnt)
			material = &bm_pools[pool].materials[_hintID];
		if(material && material->owns_state_id(stateID) )
		{
			bm_pools[pool].hint_mat = _hintID;
		}
		else
		{
			if( bm_pools[pool].is_enabled() ) 
			{
			//	_hintID = (U32)bm_pools[pool].install_new_state_id(stateID,_type,_vertex_format);
				_hintID = bm_pools[pool].get_hint_from_state_id(stateID,_type,_vertex_format);
				bm_pools[pool].hint_mat = _hintID;
			}
		}
	}

/*	virtual void SetVertexType(D3DPRIMITIVETYPE _type,U32 _vertex_format)
	{
		type = _type;
		vertex_format = _vertex_format;
	}*/

	virtual void SetIdentityModelview()
	{
		bIdentityModelview = true;
	}

	virtual bool GetPrimBuffer (BATCHDESC *desc,bool bAllowFailure);//,U32 *hintID);

	virtual void ReleasePrimBuffer (BATCHDESC *desc);

	virtual void Startup ();

	virtual void Shutdown ();

	virtual void CommitState ();

	virtual void RestoreSurfaces ();

	S32 num_pools,num_active_pools;
	BMPOOLJR *vpools[128],*ipools[128];

/*	BMPOOLJR *get_vpool()
	{
		num_active_v_pools++;
		CQASSERT(num_active_v_pools < 128);
		if (num_v_pools < num_active_v_pools)
		{
			vpools[num_active_v_pools-1] = new BMPOOLJR;
			num_v_pools++;
		}

		CQASSERT(vpools[num_active_v_pools-1]->is_empty());
		return vpools[num_active_v_pools-1];
	}

	BMPOOLJR *get_ipool()
	{
		num_active_i_pools++;
		CQASSERT(num_active_i_pools < 128);
		if (num_i_pools < num_active_i_pools)
		{
			ipools[num_active_i_pools-1] = new BMPOOLJR;
			num_i_pools++;
		}

		return ipools[num_active_i_pools-1];
	}*/

	void get_pools(BMPOOLJR **vpool,BMPOOLJR **ipool)
	{
		//keep the same pool
		if (*vpool && (*vpool)->bAssigned == 0)
		{
			(*vpool)->bAssigned = 1;
			CQASSERT((*vpool)->is_empty());
			CQASSERT((*vpool)->old_used_cnt == 0);
			CQASSERT((*ipool)->bAssigned == 0);
			(*ipool)->bAssigned = 1;
			CQASSERT((*ipool)->is_empty());
//			(*vpool)->source = 1;
			return;
		}

		//CQASSERT((*vpool == 0) || (*vpool)->pool_used_cnt != 0);

		//assign an old pool
		for (int i=0;i<num_pools;i++)
		{
			if (vpools[i]->bAssigned == FALSE && vpools[i]->bLastRendered == FALSE)
			{
				*vpool = vpools[i];
				*ipool = ipools[i];
				CQASSERT((*vpool)->is_empty());
				CQASSERT((*ipool)->is_empty());
				CQASSERT((*ipool)->bAssigned == 0);
				(*vpool)->bAssigned = 1;
				(*ipool)->bAssigned = 1;
//				(*vpool)->source = 2;
				return;
			}
		}
		
		//make a new pool
		CQASSERT(num_pools < 128);
		vpools[num_pools] = new BMPOOLJR;
		ipools[num_pools] = new BMPOOLJR;
		*vpool = vpools[num_pools];
		CQASSERT((*vpool)->is_empty());
		*ipool = ipools[num_pools];
		CQASSERT((*ipool)->is_empty());
		(*vpool)->bAssigned = 1;
		(*ipool)->bAssigned = 1;
//		(*vpool)->source = 3;
		num_pools++;
	}

	void reset_pools(bool bInternal)
	{
		for (int i=0;i<num_pools;i++)
		{
			vpools[i]->update(bInternal);
			ipools[i]->update(bInternal);
		}
	}

	void free_all_pools()
	{
		for (int i=0;i<num_pools;i++)
		{
			delete vpools[i];
			delete ipools[i];
		}
		num_active_pools = num_pools = 0;
	}

	RenderBatcher(void);
	~RenderBatcher(void);


protected:	// protected data

	
	DynamicArray<struct PRIMITIVESORTDATA> prim_sort_pool;

#define BM_P_OPAQUE			0
#define BM_P_ALPHA_DEPTH	1
#define BM_P_ALPHA_NOSORT	2
	struct BMPOOL					bm_pools[3];
	
	public:
	U32						state[RPR_MAX_STATE+EXTRA_STATES];
	U32						currentTextureID[BM_NUM_STAGES];
	U32						commitedTextureID[BM_NUM_STAGES];
	U32 texture_stage_state[BM_NUM_STAGES][D3DTSS_MAX_STATE];
	Transform modelview;
	unsigned __int64 ticks;
	//D3DPRIMITIVETYPE type;
	//U32 vertex_format;
	bool bIdentityModelview:1;
	bool bBufferLocked:1;
	bool bQueueSetup:1;
	Transform id_trans;
	U32 activeMaterialHint;

//	U32 timer;


};

void FlushQueue1()
{
	if (q_cnt1==0)
		return;

	GENRESULT result;
	PIPE->unlock_vertex_buffer(vb_handle1);
	for (int i=0;i<q_cnt1;i++)
	{
		for (int j=i;j<q_cnt1;j++)
		{
			if (batchQueue1[j].num_verts && (batchQueue1[i].material == batchQueue1[j].material))
			{
				PIPE->set_modelview(batchQueue1[j].modelView);
				if (i == j)
					batchQueue1[j].material->restore(BATCHER->state[RPR_STATE_FLAGS]);

				if (batchQueue1[j].num_indices)
					result = PIPE->draw_indexed_primitive_vb( batchQueue1[j].prim_type, vb_handle1, batchQueue1[j].start_vert, batchQueue1[j].num_verts, &id_list1[batchQueue1[j].id_offset], batchQueue1[j].num_indices, 0 );
				else
					result = PIPE->draw_primitive_vb( batchQueue1[j].prim_type, vb_handle1, batchQueue1[j].start_vert, batchQueue1[j].num_verts, 0 );
				
				batchQueue1[j].num_verts = 0;

				CQASSERT(result == GR_OK);
			}
		}
	}
	q_cnt1 = 0;
	U32 size;
	result = PIPE->lock_vertex_buffer(vb_handle1,DDLOCK_WRITEONLY | DDLOCK_DISCARDCONTENTS,(void **)&vb1,&size);
	vb_used1 = 0;
	id_list_pos1 = 0;
	CQASSERT(result == GR_OK && size == 2730);
}

void FlushQueue2()
{
	if (q_cnt2==0)
		return;

	GENRESULT result;
	PIPE->unlock_vertex_buffer(vb_handle2);
	for (int i=0;i<q_cnt2;i++)
	{
		for (int j=i;j<q_cnt2;j++)
		{
			if (batchQueue2[i].num_verts && (batchQueue2[i].material == batchQueue2[j].material))
			{
				PIPE->set_modelview(batchQueue2[i].modelView);
				if (i==j) //if this is the first element to use this material
					batchQueue2[i].material->restore(BATCHER->state[RPR_STATE_FLAGS]);
				
				if (batchQueue2[i].num_indices)
					result = PIPE->draw_indexed_primitive_vb( batchQueue2[i].prim_type, vb_handle2, batchQueue2[i].start_vert, batchQueue2[i].num_verts, &id_list2[batchQueue2[i].id_offset], batchQueue2[i].num_indices, 0 );
				else
					result = PIPE->draw_primitive_vb( batchQueue2[i].prim_type, vb_handle2, batchQueue2[i].start_vert, batchQueue2[i].num_verts, 0 );
				
				batchQueue2[j].num_verts = 0;
				
				CQASSERT(result == GR_OK);
			}
		}
	}
	q_cnt2 = 0;
	U32 size;
	PIPE->lock_vertex_buffer(vb_handle2,DDLOCK_WRITEONLY | DDLOCK_DISCARDCONTENTS,(void **)&vb2,&size);
	vb_used2 = 0;
	id_list_pos2 = 0;
	CQASSERT(size == 2048);
}

void BATCHEDCALLPRIMITIVE::initialize( )
	{
//		indices = NULL;
//		verts = NULL;
		next = NULL;
		prev = NULL;
		p_depth = -FLT_MAX;
		indices_cnt = 0;
		verts_cnt = 0;
		vpool = ipool = 0;
		modelview.set_identity();
		bOwnsPools = false;
	}

	//

	void BATCHEDCALLPRIMITIVE::render( )
	{
	//	PIPE->set_modelview( modelview );
		void *pverts = (void *)(vpool->pool);
		U16 *pindices = (U16 *)(ipool->pool);
		GENRESULT result;

		if( indices_cnt ) {
			result = PIPE->draw_indexed_primitive( p_type, verts_type, pverts, verts_cnt, pindices, indices_cnt, p_flags );
		}
		else {
			result = PIPE->draw_primitive( p_type, verts_type, pverts, verts_cnt, p_flags );
		}

		CQASSERT(result == GR_OK);
	//	vpool = ipool = 0;
	}

	//
#if 0	
	float BATCHEDCALLPRIMITIVE::dip_tri_depth( U16 *v0_idx, U32 sizeof_vert, float *MT )
	{
		// TODO: this should not recalc the depths of vertices
		// TODO: each time, we should calc all the depths once,
		// TODO: then use those pre-calced ones.
		//
		Vector *P;
		
		P = ((Vector*)(((char*)verts) + (*v0_idx) * sizeof_vert));
		float v0d = MT[0] * P->x + MT[1] * P->y + MT[2] * P->z + MT[3];
		
		v0_idx++;
		P = ((Vector*)(((char*)verts) + (*v0_idx) * sizeof_vert));
		float v1d = MT[0] * P->x + MT[1] * P->y + MT[2] * P->z + MT[3];

		v0_idx++;
		P = ((Vector*)(((char*)verts) + (*v0_idx) * sizeof_vert));
		float v2d = MT[0] * P->x + MT[1] * P->y + MT[2] * P->z + MT[3];

		return (v0d+v1d+v2d) * (1.0/3.0);
	}

	//

	void BATCHEDCALLPRIMITIVE::sort_dip( int first, int last, float *MT )
	{
		U32 sizeof_vert = FVF_SIZEOF_VERT(verts_type);

		U16 swap_buffer[3];		// triangle swap space

		int lb = first, ub = last;				
		int pivot = (first+last)/2;

		U16 *pindices = (U16 *)(ipool->pool+indices);
		
		U16 *t_lb    = &pindices[lb*3];		// lower bound tri
		U16 *t_ub    = &pindices[ub*3];		// upper bound tri
		U16 *t_pivot = &pindices[pivot*3]; 	// pivot tri

		#define T_DIP_DEPTH(I) (dip_tri_depth(I,sizeof_vert,MT))
		
		float t_pivot_depth = T_DIP_DEPTH(t_pivot);
		float t_lb_depth, t_ub_depth;

		do {

			t_lb_depth = T_DIP_DEPTH(t_lb);
			while( t_lb_depth < t_pivot_depth ) {	// while t_lb is less than t_pivot
				lb++;
				t_lb = &pindices[lb*3];
				t_lb_depth = T_DIP_DEPTH(t_lb);
			}

			t_ub_depth = T_DIP_DEPTH(t_ub);
			while( t_ub_depth > t_pivot_depth ) {	// while t_ub is greater than t_pivot
				ub--;
				t_ub = &pindices[ub*3];
				t_ub_depth = T_DIP_DEPTH(t_ub);
			}

			if( lb <= ub ) {
				swap_buffer[0] = t_lb[0];	swap_buffer[1] = t_lb[1];	swap_buffer[2] = t_lb[2];
				t_lb[0] = t_ub[0];			t_lb[1] = t_ub[1];			t_lb[2] = t_ub[2];
				t_ub[0] = swap_buffer[0];	t_ub[1] = swap_buffer[1];	t_ub[2] = swap_buffer[2];
				lb++;								
				ub--;								
			}
		} while ( lb <= ub );						

		if( first < ub ) {
			sort_dip( first, ub, MT );
		}

		if ( lb < last ) {							
			sort_dip( lb, last, MT );
		}
	}

	//
		
	float BATCHEDCALLPRIMITIVE::dp_tri_depth( Vector *v0, U32 sizeof_vert, float *MT )
	{
		Vector *P;
		
		P = v0;
		float v0d = MT[0] * P->x + MT[1] * P->y + MT[2] * P->z + MT[3];
		
		P = ((Vector*)(((char*)v0) + sizeof_vert));
		float v1d = MT[0] * P->x + MT[1] * P->y + MT[2] * P->z + MT[3];

		P = ((Vector*)(((char*)v0) + sizeof_vert));
		float v2d = MT[0] * P->x + MT[1] * P->y + MT[2] * P->z + MT[3];


		return (v0d+v1d+v2d) * (1.0/3.0);
	}

	//

	void BATCHEDCALLPRIMITIVE::sort_dp( int first, int last, float *MT )
	{
		U32 sizeof_vert = FVF_SIZEOF_VERT(verts_type);
		U32 sizeof_tri = 3 * sizeof_vert;

		CQASSERT( sizeof_vert <= 64 );	

		char swap_buffer[64*3];		// triangle swap space

		int lb = first, ub = last;				
		int pivot = (first+last)/2;
		
		Vector *t_lb    = ((Vector*)(((char*)verts) + lb*sizeof_tri)); 
		Vector *t_ub    = ((Vector*)(((char*)verts) + ub*sizeof_tri)); 
		Vector *t_pivot = ((Vector*)(((char*)verts) + pivot*sizeof_tri)); 

		#define T_DP_DEPTH(P) (dp_tri_depth(P,sizeof_vert,MT))
		
		float t_pivot_depth = T_DP_DEPTH(t_pivot);
		float t_lb_depth, t_ub_depth;

		do {

			t_lb_depth = T_DP_DEPTH(t_lb);
			while( T_DP_DEPTH(t_lb) < t_pivot_depth ) {	// while t_lb is less than t_pivot
				lb++;
				t_lb = ((Vector*)(((char*)verts) + lb*sizeof_tri));
				t_lb_depth = T_DP_DEPTH(t_lb);
			}

			t_ub_depth = T_DP_DEPTH(t_ub);
			while( T_DP_DEPTH(t_ub) > t_pivot_depth ) {	// while t_ub is greater than t_pivot
				ub--;
				t_ub = ((Vector*)(((char*)verts) + ub*sizeof_tri));
				t_ub_depth = T_DP_DEPTH(t_ub);
			}

			if( lb <= ub ) {	
				memcpy( swap_buffer, t_lb, sizeof_tri );
				memcpy( t_lb, t_ub, sizeof_tri );
				memcpy( t_ub, swap_buffer, sizeof_tri );
				lb++;								
				ub--;								
			}
		} while ( lb <= ub );						

		if( first < ub ) {
			sort_dp( first, ub, MT );
		}

		if ( lb < last ) {							
			sort_dp( lb, last, MT );
		}
	}

	//

	void BATCHEDCALLPRIMITIVE::sort()
	{
		return;

		if( p_type != D3DPT_TRIANGLELIST ) {
			return;
		}

		float mt[4];

		mt[0] = modelview.d[0][2];
		mt[1] = modelview.d[1][2];
		mt[2] = modelview.d[2][2];
		mt[3] = modelview.d[2][3];

		if( indices_cnt ) {
			sort_dip( 0, indices_cnt/3 - 1, mt );

		}
		else {
			sort_dp( 0, verts_cnt/3 - 1, mt );
		}
	}

	//

	float BATCHEDCALLPRIMITIVE::depth()
	{
		if( p_depth == -FLT_MAX ) {
			
			// find depth of this primitive in camera space
			//
			
			float	Mx = modelview.d[0][2];
			float	My = modelview.d[1][2];
			float	Mz = modelview.d[2][2];
			float	Tz = modelview.d[2][3];
			float	v_depth;

			U32		sizeof_vert = FVF_SIZEOF_VERT(verts_type);

			Vector *P = (Vector*)(verts);

			for( U32 v=0; v<verts_cnt; v++ ) {

				if( (v_depth = (Mx * P->x + My * P->y + Mz * P->z + Tz)) > p_depth ) {
					p_depth = v_depth;
				}

				P = (Vector*)( ((char*)verts) + (v+1)*sizeof_vert );
			}
		}

		return p_depth;
	}
///////////////////////////////////////////////
//
//
#endif

bool BATCHEDMATERIAL::is_free ()
{
	return (prim.verts_cnt == 0 && bLocked == false);
}

bool BATCHEDMATERIAL::is_equal( BATCHEDMATERIAL *material )
	{
		if (prim.verts_type != material->prim.verts_type)
			return false;

		if (prim.p_type != material->prim.p_type)
			return false;

		if( flags != material->flags ) {
			return false;
		}

		if( flags & BM_M_F_HAS_BLEND ) {
			if( blend_enable != material->blend_enable ) {
				return false;
			}
			if( blend_enable && ((blend_src != material->blend_src) || (blend_dst != material->blend_dst)) ) {
				return false;					
			}

#ifdef _ALPHAFUNC
			if( blend_enable && alpha_test_enable && ((alpha_func != material->alpha_func) || (alpha_ref != material->alpha_ref))  )
			{
				return false;
			}
#endif
		}

		if( flags & BM_M_F_HAS_TEXTURE ) {

			if( texture_stage_cnt != material->texture_stage_cnt ) {
				return false;
			}

			for( U32 ts=0; ts<texture_stage_cnt; ts++ ) {

				if( texture_stage_texture[ts] != material->texture_stage_texture[ts] ) {
					return false;
				}

				// must start at 1, this is a compare - COLOROP is 1, zero is invalid;
				for( U32 tss=1; tss<D3DTSS_MAX_STATE; tss++ ) {
					if( texture_stage_state[ts][tss] != material->texture_stage_state[ts][tss] ) {
						return false;
					}
				}
			} 
		}

		if( (flags & BM_M_F_HAS_DEPTH) && ((depth_enable != material->depth_enable) || (depth_write_enable != material->depth_write_enable) || (depth_func != material->depth_func)) ) {
			return false;
		}

		if( (flags & BM_M_F_HAS_CULL) && (cull_mode != material->cull_mode) ) {
			return false;
		}

		if( (flags & BM_M_F_HAS_DITHER) && (dither_enable != material->dither_enable) ) {
			return false;
		}	
#ifdef _FOG
		if( flags & BM_M_F_HAS_FOG ) {
			if( fog_enable != material->fog_enable ) {
				return false;
			}

			if( fog_enable && ((fog_start != material->fog_start) || (fog_end != material->fog_end) || (fog_density != material->fog_density) || (fog_mode != material->fog_mode) || (fog_color != material->fog_color)) ) {
				return false;
			}
		}
#endif

#ifdef _STENCIL
		if ( flags & BM_M_F_HAS_STENCIL ) {
			if (stencil_enable != material->stencil_enable ) {
				return false;
			}

			if (stencil_enable && ((stencil_ref != material->stencil_ref) || (stencil_func != material->stencil_func) || (stencil_pass != material->stencil_pass)))
			{
				return false;
			}
		}
#endif

		if (flags) {
			if (material->wrap0 != wrap0 || material->wrap1 != wrap1)
				return false;
		}

		if (flags & BM_M_F_DELAY_ON) {
			if (delay != material->delay)
				return false;
		}

		if (flags & BM_M_F_HAS_ZBIAS)
			if (z_bias != material->z_bias)
				return false;

		return true;
	}

	//

	void BATCHEDMATERIAL::save(struct RenderBatcher *batcher, U32 save_flags, D3DPRIMITIVETYPE _type, U32 _vertex_format )
	{
		prim.verts_type = _vertex_format;
		prim.p_type = _type;
		flags = save_flags;

		if( flags & BM_M_F_HAS_BLEND ) {
			PIPE->get_render_state( D3DRS_ALPHABLENDENABLE,		&blend_enable );
			if( blend_enable ) {												
				PIPE->get_render_state( D3DRS_SRCBLEND,			&blend_src );
				PIPE->get_render_state( D3DRS_DESTBLEND,			&blend_dst );

#ifdef _ALPHAFUNC
				PIPE->get_render_state (D3DRS_ALPHATESTENABLE,   &alpha_test_enable);
				if ( alpha_test_enable ) {
					PIPE->get_render_state (D3DRS_ALPHAFUNC,     &alpha_func);
					PIPE->get_render_state (D3DRS_ALPHAREF,      &alpha_ref);
				}
#endif
			}	
		}																		
																				
		if( flags & BM_M_F_HAS_DEPTH ) {										
			PIPE->get_render_state( D3DRS_ZENABLE,				&depth_enable );
			PIPE->get_render_state( D3DRS_ZWRITEENABLE,			&depth_write_enable );
			PIPE->get_render_state( D3DRS_ZFUNC,					&depth_func );
		}																		
																				
		if( flags & BM_M_F_HAS_CULL ) {											
			PIPE->get_render_state( D3DRS_CULLMODE,				&cull_mode );
		}																		
																				
		if( flags & BM_M_F_HAS_DITHER ) {										
			PIPE->get_render_state( D3DRS_DITHERENABLE,			&dither_enable );
		}																		
#ifdef _FOG																		
		if( flags & BM_M_F_HAS_FOG ) {											
			PIPE->get_render_state( D3DRS_FOGENABLE,				&fog_enable );
			if( fog_enable ) {
				PIPE->get_render_state( D3DRS_FOGTABLESTART,		&fog_start );
				PIPE->get_render_state( D3DRS_FOGTABLEEND,		&fog_end );
				PIPE->get_render_state( D3DRS_FOGTABLEDENSITY,	&fog_density );
				PIPE->get_render_state( D3DRS_FOGTABLEMODE,		&fog_mode );
				PIPE->get_render_state( D3DRS_FOGCOLOR,			&fog_color );
			}
		}
#endif

#ifdef _STENCIL
		if ( flags & BM_M_F_HAS_STENCIL ) {
			PIPE->get_render_state ( D3DRS_STENCILENABLE,		&stencil_enable);
			if (stencil_enable) {
				PIPE->get_render_state( D3DRS_STENCILREF,		&stencil_ref);
				PIPE->get_render_state( D3DRS_STENCILFUNC,		&stencil_func);
				PIPE->get_render_state( D3DRS_STENCILPASS,		&stencil_pass);
			}
		}
#endif

		if (flags) {
		//	PIPE->get_render_state(D3DRS_WRAPU,  &wrapU);
		//	PIPE->get_render_state(D3DRS_WRAPV,  &wrapV);
			PIPE->get_render_state(D3DRS_WRAP0, &wrap0);
			PIPE->get_render_state(D3DRS_WRAP1, &wrap1);
		}

		if( flags & BM_M_F_HAS_TEXTURE ) {
			texture_stage_cnt = 0;
			while( texture_stage_cnt < BM_NUM_STAGES ) {
				
			//	PIPE->get_texture_stage_state( texture_stage_cnt, D3DTSS_COLOROP, &texture_stage_state[texture_stage_cnt][D3DTSS_COLOROP] );
				texture_stage_state[texture_stage_cnt][D3DTSS_COLOROP] = batcher->texture_stage_state[texture_stage_cnt][D3DTSS_COLOROP];

				if( texture_stage_state[texture_stage_cnt][D3DTSS_COLOROP] == D3DTOP_DISABLE ) {
					break;
				}

				//PIPE->get_texture_stage_texture( texture_stage_cnt, &texture_stage_texture[texture_stage_cnt] );
				texture_stage_texture[texture_stage_cnt] = batcher->currentTextureID[texture_stage_cnt];
				
				// start at 2, COLOROP is 1, zero is invalid;
				for( U32 tss=2; tss<D3DTSS_MAX_STATE; tss++ ) {
				//	PIPE->get_texture_stage_state( texture_stage_cnt, (D3DTEXTURESTAGESTATETYPE)tss, &texture_stage_state[texture_stage_cnt][tss] );
					texture_stage_state[texture_stage_cnt][tss] = batcher->texture_stage_state[texture_stage_cnt][tss];
				}

				texture_stage_cnt++;
			} 
			//disable remaining states
			int t = texture_stage_cnt;
			while (t < BM_NUM_STAGES)
			{
				texture_stage_state[t][D3DTSS_COLOROP] = D3DTOP_DISABLE;
				t++;
			}
		}

		if (flags & BM_M_F_DELAY_ON) {
			delay = batcher->state[RPR_DELAY];
		}

		if (flags & BM_M_F_HAS_ZBIAS)
			PIPE->get_render_state( D3DRS_DEPTHBIAS ,			&z_bias );
	}

	//

	void BATCHEDMATERIAL::restore( U32 restore_flags )
	{
		U32 rflags = flags & restore_flags;

		if (CQEFFECTS.bTextures == 0)
			PIPE->set_pipeline_state(RP_TEXTURE,0);

		if( rflags & BM_M_F_HAS_BLEND ) {
			if( rflags & BM_M_F_HAS_BLEND_ENABLE ) {
				PIPE->set_render_state( D3DRS_ALPHABLENDENABLE,	blend_enable );
			}
			if( rflags & BM_M_F_HAS_BLEND_FUNC ) {
				if( blend_enable ) {											
					PIPE->set_render_state( D3DRS_SRCBLEND,		blend_src );
					PIPE->set_render_state( D3DRS_DESTBLEND,		blend_dst );
					
#ifdef _ALPHAFUNC
					PIPE->set_render_state (D3DRS_ALPHATESTENABLE,   alpha_test_enable);
					if ( alpha_test_enable ) {
						PIPE->set_render_state (D3DRS_ALPHAFUNC,     alpha_func);
						PIPE->set_render_state (D3DRS_ALPHAREF,      alpha_ref);
					}
#endif

				}																	
			}
		}																		
																				
		if( rflags & BM_M_F_HAS_DEPTH ) {										
			if( rflags & BM_M_F_HAS_DEPTH_ENABLE ) {
				PIPE->set_render_state( D3DRS_ZENABLE,			depth_enable );
				PIPE->set_render_state( D3DRS_ZWRITEENABLE,		depth_write_enable );
			}
			if( rflags & BM_M_F_HAS_DEPTH_FUNC ) {
				PIPE->set_render_state( D3DRS_ZFUNC,				depth_func );
			}
		}																		
																				
		if( rflags & BM_M_F_HAS_CULL ) {											
			PIPE->set_render_state( D3DRS_CULLMODE,				cull_mode );
		}																		
																				
		if( rflags & BM_M_F_HAS_DITHER ) {										
			PIPE->set_render_state( D3DRS_DITHERENABLE,			dither_enable );
		}																		
						
#ifdef _FOG
		if( rflags & BM_M_F_HAS_FOG ) {											
			PIPE->set_render_state( D3DRS_FOGENABLE,				fog_enable );
			if( fog_enable ) {
				PIPE->set_render_state( D3DRS_FOGTABLESTART,		fog_start );
				PIPE->set_render_state( D3DRS_FOGTABLEEND,		fog_end );
				PIPE->set_render_state( D3DRS_FOGTABLEDENSITY,	fog_density );
				PIPE->set_render_state( D3DRS_FOGTABLEMODE,		fog_mode );
				PIPE->set_render_state( D3DRS_FOGCOLOR,			fog_color );
			}
		}
#endif

#ifdef _STENCIL
		if( rflags & BM_M_F_HAS_STENCIL ) {
			PIPE->set_render_state( D3DRS_STENCILENABLE,			stencil_enable );
			if (stencil_enable)
			{
				PIPE->set_render_state( D3DRS_STENCILREF,		stencil_ref );
				PIPE->set_render_state( D3DRS_STENCILFUNC,		stencil_func );
				PIPE->set_render_state( D3DRS_STENCILPASS,		stencil_pass );
				//PIPE->set_render_state( D3DRS_
			}
		}
#endif

		if (flags) {
//			PIPE->set_render_state(D3DRS_WRAPU,  wrapU);
//			PIPE->set_render_state(D3DRS_WRAPV,  wrapV);
			PIPE->set_render_state(D3DRS_WRAP0, wrap0);
			PIPE->set_render_state(D3DRS_WRAP1, wrap1);
		}

		if( rflags & BM_M_F_HAS_TEXTURE ) {
			
			for( U32 ts=0; ts<BM_NUM_STAGES; ts++ ) {
				
				PIPE->set_texture_stage_state( ts, D3DTSS_COLOROP, texture_stage_state[ts][D3DTSS_COLOROP] );

				if( texture_stage_state[ts][D3DTSS_COLOROP] != D3DTOP_DISABLE ) {

					if (texture_stage_texture[ts] != -1)
					{
						if (BATCHER->commitedTextureID[ts] != texture_stage_texture[ts])
						{
#ifndef _ROB
							CQASSERT(texture_stage_texture[ts] == 0 || PIPE->is_texture(texture_stage_texture[ts]) == GR_OK);
#endif			
							PIPE->set_texture_stage_texture( ts, texture_stage_texture[ts]);
							BATCHER->commitedTextureID[ts] = texture_stage_texture[ts];
						}
					}
				
					// start at 2, COLOROP is 1, zero is invalid;
					for( U32 tss=2; tss<D3DTSS_MAX_STATE; tss++ ) {
						PIPE->set_texture_stage_state( ts, (D3DTEXTURESTAGESTATETYPE)tss, texture_stage_state[ts][tss] );
					}
				}
				else if (texture_stage_texture[ts] == 0)
				{
					PIPE->set_texture_stage_texture( ts, 0 );
					BATCHER->commitedTextureID[ts] = 0;
				}
			} 
		}

	//	if (rflags & BM_M_F_DELAY_ON) {
	//		delay = batcher->state[RPR_DELAY];
	//	}

		if (flags & BM_M_F_HAS_ZBIAS)
			PIPE->set_render_state( D3DRS_DEPTHBIAS ,			z_bias );
	}

	//
	void BATCHEDMATERIAL::empty_primitive()
	{
		CQASSERT(prim.vpool);

		prim.indices_cnt = 0;
		prim.verts_cnt = 0;
			
		prim.vpool->free_all();
		prim.ipool->free_all();
	}

	void BATCHEDMATERIAL::reset_primitives()
	{
		CQASSERT(prim.vpool);

		prim.indices_cnt = 0;
		prim.verts_cnt = 0;

		prim.vpool->free_all();
		prim.vpool->bAssigned = 0;
		prim.ipool->free_all();
		prim.ipool->bAssigned = 0;

		prim.bOwnsPools = false;
	}

	bool BATCHEDMATERIAL::owns_state_id(U32 id)
	{
	/*	for (unsigned int i=0;i<id_cnt;i++)
		{
			if (state_id[i] == id)
				return TRUE;
		}*/
		U32 hash = (id >> 16) % BATCH_HASH_SIZE;
		STATE_ID *ids = state_id[hash].first();
		while (ids)
		{
			if (ids->id == id)
				return true;

			ids = ids->next;
		}

		return false;
	}

	void BATCHEDMATERIAL::add_state_id(U32 id)
	{
	/*	for (unsigned int i=0;i<id_cnt;i++)
	{
	if (state_id[i] == id)
				CQASSERT(0 && "This sucks");
				}
				CQASSERT(id_cnt<DUP_MATERIALS);
		state_id[id_cnt++] = id;*/
		U32 hash = (id >> 16) % BATCH_HASH_SIZE;
		STATE_ID *newID = &state_id_list[current_state_id];
		current_state_id = (current_state_id+1)%STATE_ID_LIST_LENGTH;
		if (newID->owner)
		{
//			CQASSERT(newID->owner->prim.verts_cnt == 0);
			//reusing a dead material state
			U32 hash2 = (newID->id >> 16) % BATCH_HASH_SIZE;
			newID->owner->state_id[hash2].unlink(newID);
		}
		newID->id = id;
		newID->owner = this;
		state_id[hash].link(newID);
	}

	void BATCHEDMATERIAL::remove_state_id(U32 id)
	{
		U32 hash = (id >> 16) % BATCH_HASH_SIZE;
		STATE_ID *ids = state_id[hash].first();
		while (ids)
		{
			if (ids->id == id)
			{
				state_id[hash].unlink(ids);
				ids->owner = 0;
				return;
			}

			ids = ids->next;
		}
	}

	void BATCHEDMATERIAL::free_state_ids()
	{
		for (int b=0;b<BATCH_HASH_SIZE;b++)
		{
			STATE_ID *list = state_id[b].first();
			while (list)
			{
				list->owner = 0;
				list = list->next;
			}
			state_id[b].reset();
		}
	}

	
/*	BMPOOLJR *BATCHEDMATERIAL::get_v_pool()
	{
		//if this is my first time getting called this frame
		if (prim.verts_cnt == 0)
		{
			vpool = BATCHER->get_vpool(vpool);
			CQASSERT(vpool->is_empty());
		}
		CQASSERT(vpool);
		return vpool;
	}

	BMPOOLJR *BATCHEDMATERIAL::get_i_pool()
	{
		if (ipool == 0)
			ipool = BATCHER->get_ipool();
		return ipool;
	}*/
	
	void BATCHEDMATERIAL::get_pools(BMPOOLJR **_vpool,BMPOOLJR **_ipool)
	{
		*_vpool = prim.vpool;
		*_ipool = prim.ipool;
		//if this is my first time getting called this frame
		if (prim.bOwnsPools == 0)
		{
			BATCHER->get_pools(_vpool,_ipool);
			prim.vpool = *_vpool;
			prim.ipool = *_ipool;
			CQASSERT((*_vpool)->is_empty());
			prim.bOwnsPools = true;
		}
		CQASSERT(*_vpool);
	}

	void BATCHEDMATERIAL::set_modelview(const Transform &trans)
	{
		prim.modelview = trans;
#ifdef MODELVIEW_STRAT
		inv = trans.get_inverse();
#endif
	}
/////////////////////////////////

#if 0
static int psd_compare( const void *elem1, const void *elem2 )
{
	PRIMITIVESORTDATA* p1 = (PRIMITIVESORTDATA*) elem1;
	PRIMITIVESORTDATA* p2 = (PRIMITIVESORTDATA*) elem2;
	
	int result = 0;
	if( p1->primitive->depth() < p2->primitive->depth() ) {
		result = -1;
	}
	else if (p1->primitive->depth() > p2->primitive->depth() ) {
		result = 1;
	}

	return result;
}
#endif

//

void BMPOOL::set_name( const char *_name ) 
	{
		name = _name;
	}

	//

/*	void BMPOOL::set_size( U32 new_pool_size ) 
	{
		delete[] pool;
		pool_used_cnt = 0;
		pool_size = new_pool_size;
		pool = new unsigned char[pool_size];
		pool_flags |= BM_P_F_EMPTY;

		update();
	}*/

	//

	void BMPOOL::set_materials_size( U32 new_materials_size ) 
	{
		reset();
		delete[] materials;
		materials_max_cnt = new_materials_size;
		materials = new BATCHEDMATERIAL[materials_max_cnt];

		update();
	}

	//

	void BMPOOL::set_enabled( bool enabled )
	{
		if( enabled ) {
			pool_flags |= BM_P_F_ENABLE;
		}
		else {
			pool_flags &= ~(BM_P_F_ENABLE);
		}
	}

	//

	bool BMPOOL::is_enabled ( )
	{
		return (pool_flags & BM_P_F_ENABLE)? true : false;
	}

	//

	bool BMPOOL::is_empty ( )
	{
		return (pool_flags & BM_P_F_EMPTY)? true : false;
	}

	//

/*	U32 BMPOOL::materials_hash_val( U32 state_id )
	{
		return state_id % MH_MAX_BUCKETS;
	}

	// 

	void BMPOOL::materials_hash_reset( )
	{
		for( U32 b=0; b<MH_MAX_BUCKETS; b++ ) {
			materials_hash[b].reset();
		}
	}

	//

	void BMPOOL::materials_hash_link( BATCHEDMATERIAL *material, U32 id )
	{
		U32 hash = materials_hash_val( id );
		materials_hash[hash].link( material );
	}

	//

	void BMPOOL::materials_hash_unlink( BATCHEDMATERIAL *material, U32 id )
	{
		U32 hash = materials_hash_val( id );
		materials_hash[hash].unlink( material );
	}*/

	//
/*
	void *BMPOOL::allocate( U32 size, bool set_non_empty)
	{
		if( (pool_used_cnt+size) > pool_size ) {
			GENERAL_WARNING( TEMPSTR( "Pool size exceeded in pool '%s'\n", name ) );
			return NULL;
		}

		void *ret = &pool[pool_used_cnt];
		pool_used_cnt += size;
		if( set_non_empty ) {
			pool_flags &= ~(BM_P_F_EMPTY);
		}

		return ret;
	}

	//

	void BMPOOL::free_all()
	{
		pool_used_cnt = 0;
		pool_flags |= BM_P_F_EMPTY;
	}*/

	//

	void BMPOOL::reset ()
	{
		for (unsigned int i=0;i<materials_used_cnt;i++)
		{
			materials[i].free_state_ids();
		}

		materials_used_cnt = 0;
	}

	void BMPOOL::update ( void )
	{


	/*	materials_used_cnt = 0;

		materials_hash_reset();*/

	/*	for (int i=0;i<materials_used_cnt;i++)
		{
			for (int b=0;b<BATCH_HASH_SIZE;b++)
				materials[i].state_id[b].reset();
		}*/
		
	//	materials_used_cnt = 0;

//		free_all();

//		pc_render = pc_save = pc_restore = 0;
	}

	//
#if 0
	BATCHEDMATERIAL *BMPOOL::find_material ( U32 state_id )
	{
		BATCHEDMATERIAL *material = NULL;
		
		if( state_id ) {
			U32 hash = materials_hash_val( state_id );
			material = materials_hash[hash].first();
			while( material ) {
				if( material->owns_state_id(state_id) ) {
					return material;
				}
				material = material->next;
			}
		}

		return material;
	}

#endif

	//no hashing version, since materials have multiple state id's
	U32 BMPOOL::find_material( U32 state_id )
	{
		if (hint_mat != 0xffffffff && hint_mat < materials_used_cnt && materials[hint_mat].owns_state_id(state_id))
			return hint_mat;

		for (unsigned int i=0;i<materials_used_cnt;i++)
		{
			if( materials[i].owns_state_id(state_id) ) {
				return i;
			}
		}

		return U32(-1);
	}
	//

	U32 BMPOOL::allocate_material( U32 state_id )
	{

		CQASSERT( (materials_used_cnt) <= materials_max_cnt );
		
	//	if (materials_used_cnt == materials_max_cnt)
	//		materials_max_cnt = __max( materials_max_cnt, materials_used_cnt+32 );

		U32 chosenMaterial = materials_used_cnt;

		if( materials_used_cnt == materials_max_cnt ) {
			/*BATCHEDMATERIAL *new_mat;
			new_mat = new BATCHEDMATERIAL[materials_max_cnt];
			memcpy(new_mat,materials,sizeof(BATCHEDMATERIAL)*materials_cnt);
			delete[] materials;
			materials = new_mat;
			materials_cnt = materials_max_cnt;*/
			//materials_used_cnt = 0;
			unsigned int i=0;
			while (materials[i].is_free() == false && i < materials_max_cnt)
			{
				i++;
			}

			if (i==materials_max_cnt)
			{
				//CQBOMB0("Out of materials");
				CQTRACE10("RB : ran out of materials - Evicting material 0");
				CQASSERT (materials[0].prim.vpool);

				materials[0].restore( BATCHER->state[RPR_STATE_FLAGS] );
				PIPE->set_modelview( materials[0].prim.modelview);
						
				BATCHEDCALLPRIMITIVE *prim = &materials[0].prim;
						
				prim->render( );
						
				materials[0].reset_primitives();

				i = 0;
			}

			chosenMaterial = i;
			materials[i].free_state_ids();
			//should be redundant
	//		materials[i].reset_primitives();
		}
		else
			materials_used_cnt++;

		CQASSERT( materials );

/*		if (materials_used_cnt == materials_cnt)
			CQASSERT("Out of materials");*/

		BATCHEDMATERIAL *material = &materials[chosenMaterial];
		
		

		material->flags = 0;
		material->next = NULL;
		material->prev = NULL;
	//	material->state_id[0] = state_id;
		if (state_id)
			material->add_state_id(state_id);

		//debug
//		material->owner = this;
		
		return chosenMaterial;
	}

	U32 BMPOOL::find_matching_material( BATCHEDMATERIAL *tmp_mat )
	{
		BATCHEDMATERIAL *material;

		CQASSERT(tmp_mat);

		material = materials;
		for (unsigned int i=0;i<materials_used_cnt;i++,material++)
		{
			if( material->is_equal(tmp_mat) ) {
				return i;
			}
		}

		return 0xffffffff;
	}

	//
/*	U32 BMPOOL::install_new_state_id( U32 state_id, D3DPRIMITIVETYPE type, U32 vertex_format)
	{
		BATCHEDMATERIAL tmp_material;// = new BATCHEDMATERIAL;
		U32 matID;
		tmp_material.save( batcher, activeStatesFlags, type, vertex_format);
		if ((matID = find_matching_material(&tmp_material)) == 0xffffffff)
		{
			if( (matID = allocate_material( state_id )) == 0xffffffff ) {
				return 0;
			}
			
			materials[matID].save( batcher, activeStatesFlags, type, vertex_format);
			materials[matID].bPreTransform = true;
			//				pc_save++;
		}
		else
		{
			if (state_id)
			{	
				materials[matID].add_state_id(state_id);
				materials[matID].bPreTransform = true;
			}
		}

		return matID;
	}*/
	U32 BMPOOL::get_hint_from_state_id(U32 state_id, D3DPRIMITIVETYPE type, U32 vertex_format)
	{
	
		U32 matID= U32(-1);
		CQASSERT(state_id);
		if((matID = find_material( state_id )) == -1 ) {
			
			BATCHEDMATERIAL tmp_material;
			tmp_material.save( batcher, BATCHER->state[RPR_STATE_FLAGS], type, vertex_format);
			if ((matID = find_matching_material(&tmp_material)) == 0xffffffff)
			{
				if( (matID = allocate_material( state_id )) == 0xffffffff ) 
				{
					CQTRACE10("RB : Couldn't make material!");
					return U32(-1);
				}
				
				BATCHEDMATERIAL * material = &materials[matID];
				
				material->save( batcher, BATCHER->state[RPR_STATE_FLAGS], type, vertex_format);
				material->bPreTransform = true;
			}
			else
			{
				BATCHEDMATERIAL * material = &materials[matID];
				if (state_id)
				{	
					material->add_state_id(state_id);
					material->bPreTransform = true;
				}
			}
		}

		return matID;
	}

	BATCHEDMATERIAL * BMPOOL::get_material_from_state_id(U32 state_id, D3DPRIMITIVETYPE type, U32 vertex_format)
	{
	
		U32 matID=U32(-1);
		if(state_id == 0 || (matID = find_material( state_id )) == U32(-1) ) 
		{
			BATCHEDMATERIAL tmp_material;
			tmp_material.save( batcher, BATCHER->state[RPR_STATE_FLAGS], type, vertex_format);
			if ((matID = find_matching_material(&tmp_material)) == 0xffffffff)
			{
				if( (matID = allocate_material( state_id )) == 0xffffffff ) {
					return 0;
				}
				
				BATCHEDMATERIAL *material = &materials[matID];
				
				material->save( batcher, BATCHER->state[RPR_STATE_FLAGS], type, vertex_format);
				material->bPreTransform = true;
			}
			else
			{
				BATCHEDMATERIAL *material = &materials[matID];
				if (state_id)
				{	
					material->add_state_id(state_id);
					material->bPreTransform = true;
				}
			}
		}

		return &materials[matID];
	}


	GENRESULT BMPOOL::add_primitive( U32 state_id, U32 save_flags, D3DPRIMITIVETYPE type, U32 vertex_format, const void *verts, int num_verts, const U16 * indices, int num_indices, U32 flags )
	{
		CQASSERT(in==0);

		if( num_verts == 0 ) {
			return GR_GENERIC;
		}

		maxDelay = max(batcher->state[RPR_DELAY],maxDelay);

	//	__int64 time1,time2;

	//	QueryPerformanceCounter((LARGE_INTEGER *)&time1);

		BATCHEDMATERIAL *material;
		U32 matID;

		if( !state_id || (matID = find_material( state_id )) == U32(-1) ) 
		{
		//	material = &materials[matID]; //this is bad news.  matID is -1 or undefined here
			if (0)//num_verts > BIG_BATCH && state_id)
			{
				if( (matID = allocate_material( state_id )) == 0xffffffff ) {
					return GR_GENERIC;
				}
				
				material = &materials[matID];

				material->save( batcher, save_flags,type, vertex_format);
				material->set_modelview(batcher->modelview);
				material->bPreTransform = false;
			}
			else
			{
				BATCHEDMATERIAL tmp_material;
				tmp_material.save( batcher,  save_flags, type, vertex_format);
				if ((matID = find_matching_material(&tmp_material)) == 0xffffffff)
				{
					if( (matID = allocate_material( state_id )) == 0xffffffff ) {
						return GR_GENERIC;
					}

					material = &materials[matID];
					
					material->save( batcher, save_flags, type, vertex_format);
					material->bPreTransform = true;
				}
				else
				{
					if (state_id)
					{	
						material = &materials[matID];
						material->add_state_id(state_id);
						material->bPreTransform = true;
					}
				}
			}
		}

		material = &materials[matID];

		CQASSERT(material->bLocked == false && "You are already rendering to this material");
		U32 sizeof_vert = FVF_SIZEOF_VERT(vertex_format);
		U8 *prim_mem;
		U32 prim_mem_offset,i_mem_offset;
		


		BATCHEDCALLPRIMITIVE *prim = &material->prim;
		BMPOOLJR *vpool;// = material->get_v_pool();
		BMPOOLJR *ipool;
		material->get_pools(&vpool,&ipool);
	//	Transform currentModelView=batcher->modelview;


		//if we can't cram any more in, go ahead and render it
		if ( (prim_mem_offset = vpool->allocate( sizeof_vert * num_verts)) == 0xffffffff || (i_mem_offset = ipool->allocate( sizeof(U16)*num_indices)) == 0xffffffff) 
		{
		/*	material->restore( activeStatesFlags );

			PIPE->set_modelview(material->prim.modelview);// itrans);
			prim->render( );
			PIPE->set_render_state(D3DRS_WRAP0, 0);
			PIPE->set_render_state(D3DRS_WRAP1, 0);
			material->empty_primitive();*/
			if (prim_mem_offset != U32(-1))
				vpool->set_pool_used_cnt(0);   //unallocate vpool if it succeeded but ipool didn't

			batcher->flush(~0);
			material->get_pools(&vpool,&ipool);
			if ( (prim_mem_offset = vpool->allocate( sizeof_vert * num_verts)) == 0xffffffff || (i_mem_offset = ipool->allocate( sizeof(U16)*num_indices)) == 0xffffffff)
			{
				CQTRACE10("RB : add_primitive failed");
				return GR_GENERIC;
			}
		}

//		vpool->owner = material;

		U16 idx_cnt = prim->verts_cnt;
		prim->verts_cnt += num_verts;
		if (prim->vpool == 0)
		{
			CQASSERT(prim_mem_offset == 0);
//			prim->verts = prim_mem_offset;
			prim->vpool = vpool;
		}
		prim_mem = prim_mem_offset+vpool->pool;
		memcpy(prim_mem, verts, sizeof_vert*num_verts);
		
		//trying this out!
		if (batcher->bIdentityModelview)
			material->bPreTransform = false;

		if (material->bPreTransform)
		{
			Transform trans;
			//trans = material->inv*batcher->modelview;
			trans = batcher->modelview;
			U8 *buff = prim_mem;
			Vector *vec;
			Vector temp;
			for (int i=0;i<num_verts;i++)
			{
				vec = (Vector *)buff;
				MATH_ENGINE()->transform(temp, trans, *vec);
				*vec = temp;
				buff += sizeof_vert;
			}
		}
		else
		{
			material->set_modelview(batcher->modelview);
		}

		//BMPOOLJR *ipool=material->get_i_pool();
	/*	if ( (prim_mem_offset = ipool->allocate( sizeof(U16)*num_indices)) == 0xffffffff) {
			CQASSERT(0 && "Bleah");
			return GR_GENERIC;
		}*/

		if (prim->ipool == 0)
		{
			CQASSERT(i_mem_offset == 0);
//			prim->indices = prim_mem_offset;
			prim->ipool = ipool;
		}

		prim_mem = i_mem_offset+ipool->pool;
		CQASSERT(prim->indices_cnt == 0 || num_indices != 0);
		
		pool_flags &= ~BM_P_F_EMPTY;
		if (indices && num_indices) {
			CQASSERT(prim->indices_cnt || idx_cnt == 0);
			prim->indices_cnt += num_indices;
			if (idx_cnt)
			{
				U16 *dest = (U16 *)prim_mem;
				const U16 *src = indices;
				for (int i=0;i<num_indices;i++)
				{
					*dest = *src+idx_cnt;
					CQASSERT(*dest < prim->verts_cnt);
					src++;
					dest++;
				}
			}
			else
				memcpy(  prim_mem,  indices,  sizeof(U16) * num_indices );
		}

		//PIPE->get_modelview( prim->modelview );
		
		prim->p_type = type;
		prim->p_flags = flags;
		prim->verts_type = vertex_format;

	//	material->primitives.link( prim );
	//	QueryPerformanceCounter((LARGE_INTEGER *)&time2);

	//	batcher->ticks += time2-time1;
		CQASSERT(prim->vpool->pool_used_cnt == sizeof_vert*prim->verts_cnt);

		//this can be called for debugging
		//if (prim->verts_type)
		//	prim->render(PIPE);

		return GR_OK;
	}

	//
	
	void BMPOOL::render_no_sort( )
	{
		
		BATCHEDMATERIAL *material = materials;
		for( U32 idx=0; idx<materials_used_cnt; idx++, material++ )
		{
			if (material->delay == delayStage)
			{
				mats_rendered++;
				
				if (material->bLocked) //not ready to render
				{
					material->prim.vpool->set_pool_used_cnt(0);
					material->prim.ipool->set_pool_used_cnt(0);
					material->bLocked = 0;
					CQTRACE10("RB : Forcibly unlocking a material");
				}
				
				if (material->prim.verts_cnt)
				{
					material->restore( BATCHER->state[RPR_STATE_FLAGS] );
					
					PIPE->set_modelview( material->prim.modelview);
					
					BATCHEDCALLPRIMITIVE *prim = &material->prim;
					
					prim->render( );
					PIPE->set_render_state(D3DRS_WRAP0, 0);
					PIPE->set_render_state(D3DRS_WRAP1, 0);
					
					material->reset_primitives();
				}
				else
					CQASSERT((material->prim.bOwnsPools==false) || (material->prim.vpool->pool_used_cnt==0));
			}
			CQASSERT(material->delay < 10);
			if (maxDelay < material->delay)
			{
				maxDelay = material->delay;
				CQTRACE10("RB : maxDelay was incorrect");
			}
		}
		if (mats_rendered == materials_used_cnt)
		{
			mats_rendered=0;
			pool_flags |= BM_P_F_EMPTY;
		}
	}

	//

	void BMPOOL::render_depth_sort( DynamicArray<PRIMITIVESORTDATA> &sort_pool )
	{

		render_no_sort( );
		pool_flags |= BM_P_F_EMPTY;
		return;
#if 0
		if( materials_used_cnt == 0 ) {
			return;
		}

		BATCHEDMATERIAL *material;
		BATCHEDCALLPRIMITIVE *prim;
		U32 idx;


		// Collect material/primitive information and sort data
		// in each primitive. i.e. do per-triangle sort.
		//
		if( materials_used_cnt > 1 ) {

			U32 sort_pool_used_cnt = 0;
			material = materials;
			for( idx=0; idx<materials_used_cnt; idx++, material++ ) {

				prim = material->primitives.first();
				while( prim ) {

					prim->sort();

					sort_pool[sort_pool_used_cnt].material = material;
					sort_pool[sort_pool_used_cnt].primitive = prim;
					sort_pool_used_cnt++;

					prim = prim->next;
				}
			}

			if( sort_pool_used_cnt == 0 ) {
				GENERAL_TRACE_1( "Materials, but no primitives\n" );
				return;
			}

			// Sort all material/primitive info.  i.e. do per-object sort
			//
			qsort( &(sort_pool[0]), sort_pool_used_cnt, sizeof(PRIMITIVESORTDATA), psd_compare );

			// Render in sorted order, render all primitives in a material
			// before continuing.  Once a material is rendered, clear the
			// primitives so we don't render it again.
			//
			PIPE->set_render_state( D3DRS_ALPHABLENDENABLE, TRUE );
			PIPE->set_render_state( D3DRS_ZWRITEENABLE, FALSE );
			PIPE->set_render_state( D3DRS_ZENABLE, TRUE );
		
			for( idx=0; idx<sort_pool_used_cnt; idx++ ) {
				
				material = sort_pool[idx].material;

				material->restore( PIPE, ~(BM_M_F_HAS_BLEND_ENABLE|BM_M_F_HAS_DEPTH_ENABLE) );
				pc_restore++;
				
				BATCHEDCALLPRIMITIVE *prim = material->primitives.first();
				while( prim ) {
					pc_render++;
					prim->render();
					prim = prim->next;
				}
				
				//clean dirty states
//				PIPE->set_render_state(D3DRS_WRAPU,FALSE);
//				PIPE->set_render_state(D3DRS_WRAPV,FALSE);
				PIPE->set_render_state(D3DRS_WRAP0, 0);
				PIPE->set_render_state(D3DRS_WRAP1, 0);
				material->reset_primitives();
			} 
		}
		else {
			PIPE->set_render_state( D3DRS_ALPHABLENDENABLE, TRUE );
			PIPE->set_render_state( D3DRS_ZWRITEENABLE, FALSE );
			PIPE->set_render_state( D3DRS_ZENABLE, TRUE );
		
			materials->restore(  PIPE, ~(BM_M_F_HAS_BLEND_ENABLE|BM_M_F_HAS_DEPTH_ENABLE) );
			pc_restore++;

			BATCHEDCALLPRIMITIVE *prim = materials->primitives.first();
			while( prim ) {
				pc_render++;
				prim->render( PIPE );
				prim = prim->next;
				CQASSERT(prim == 0);
			}
			
			//clean dirty states
//			PIPE->set_render_state(D3DRS_WRAPU,FALSE);
//			PIPE->set_render_state(D3DRS_WRAPV,FALSE);
			PIPE->set_render_state(D3DRS_WRAP0, 0);
			PIPE->set_render_state(D3DRS_WRAP1, 0);
			materials->reset_primitives();
		}
#endif
	}

// --------------------------------------------------------------------------
//
// RenderBatcher
//
//




// Macro to define supported methods on this interface
//
#undef DA_METHOD
#undef DA_COMPONENT_NAME

#define DA_COMPONENT_NAME RenderBatcher
#define DA_METHOD(name,params) GENRESULT COMAPI DA_COMPONENT_NAME :: name params

//

DA_METHOD(	set_state,(RPRSTATE _state, U32 value ))
{
	//if (_state == RPR_BATCH) 	value = 0;	// disable batching for debug

	if( state[_state] != value ) {
		switch( _state ) {
		case RPR_BATCH:
			if (value)
			{
				for (int texture_stage_cnt=0;texture_stage_cnt < BM_NUM_STAGES;texture_stage_cnt++ ) {
					for( U32 tss=1; tss<D3DTSS_MAX_STATE; tss++ ) {
						PIPE->get_texture_stage_state( texture_stage_cnt, (D3DTEXTURESTAGESTATETYPE)tss, &texture_stage_state[texture_stage_cnt][tss] );
					}
				}
			} 
			break;

		case RPR_BATCH_POOLS:
			bm_pools[BM_P_OPAQUE].set_enabled( (value & RPR_OPAQUE) );
			bm_pools[BM_P_ALPHA_DEPTH].set_enabled( (value & RPR_TRANSLUCENT_DEPTH_SORTED) != 0 );
			bm_pools[BM_P_ALPHA_NOSORT].set_enabled( (value & RPR_TRANSLUCENT_UNSORTED) != 0);
			break;

	/*	case RPR_BATCH_OPAQUE_POOL:
			bm_pools[BM_P_OPAQUE].set_size( value );
			break;

		case RPR_BATCH_TRANSLUCENT_POOL:
			bm_pools[BM_P_ALPHA_DEPTH].set_size( value );
			break;

		case RPR_BATCH_TRANSLUCENT_NONZ_POOL:
			bm_pools[BM_P_ALPHA_NOSORT].set_size( value );
			break;*/
		}
		state[_state] = value;
	}

	return GR_OK;
}

//

DA_METHOD(	get_state,(RPRSTATE _state, U32 *value ))
{
	*value = state[_state];
	return GR_OK;
}

//

DA_METHOD(	set_modelview,(const Transform & _modelview ))
{
	if ((memcmp(&_modelview, &id_trans, sizeof(id_trans)) == 0))
	{
		if (bIdentityModelview)
			return GR_OK;
		bIdentityModelview = true;
	}
	else
		bIdentityModelview = false;
	modelview = _modelview;
	if( !state[RPR_BATCH] )
		return PIPE->set_modelview( _modelview );
	else
		return GR_OK;
}

//

DA_METHOD(	get_modelview,(Transform & _modelview ))
{
	if ( !state[RPR_BATCH] )
		return PIPE->get_modelview( _modelview );
	else
	{
		modelview = _modelview;
		return GR_OK;
	}
}

//

DA_METHOD(	set_viewport,(int x, int y, int w, int h ))
{
	return PIPE->set_viewport( x,y,w,h );
}

//

DA_METHOD(	get_viewport,(int *x, int *y, int *w, int *h ))
{
	return PIPE->get_viewport( x, y, w, h );
}

//

DA_METHOD(	set_ortho,(float left, float right, float bottom, float top, float nearval, float farval ))
{
	return PIPE->set_ortho( left, right, bottom, top, nearval, farval );
}

//

DA_METHOD(	set_perspective,(float fovy, float aspect, float nearval, float farval ) )
{
	return PIPE->set_perspective( fovy, aspect, nearval, farval );
}

//

DA_METHOD(  set_projection,(const Matrix4 &projection ))
{
	return PIPE->set_projection( projection );
}

//

DA_METHOD(  get_projection,(Matrix4 &projection ))
{
	return PIPE->get_projection( projection );
}

//

DA_METHOD(	begin_scene,(void ))
{
	return PIPE->begin_scene();
}

//

DA_METHOD(	end_scene,(void ))
{
	return PIPE->end_scene();
}

//

DA_METHOD(	set_render_state,(D3DRENDERSTATETYPE _state, U32 value ))
{
	return PIPE->set_render_state( _state, value );
}

//

DA_METHOD(	get_render_state,(D3DRENDERSTATETYPE _state, U32 *value ))
{
	return PIPE->get_render_state( _state, value );
}

//

DA_METHOD(  set_sampler_state,	(U32 stage, D3DSAMPLERSTATETYPE ss, U32 value ))
{
	GENRESULT result;
	//texture_stage_state[stage][ss] = value;
	//if( !state[RPR_BATCH] )
		result =  PIPE->set_sampler_state( stage, ss, value );
	//else
	//	return GR_OK;

	return result;
}
DA_METHOD(  get_sampler_state,	(U32 stage, D3DSAMPLERSTATETYPE ss, U32 *value ))
{
	return PIPE->get_sampler_state( stage, ss, value );
}
DA_METHOD(  set_texture_stage_state,(U32 stage, D3DTEXTURESTAGESTATETYPE tss, U32 value ))
{
	GENRESULT result;
	
	texture_stage_state[stage][tss] = value;
	if( !state[RPR_BATCH] )
		result =  PIPE->set_texture_stage_state( stage, tss, value );
	else
		return GR_OK;

	return result;
}

//

DA_METHOD(  get_texture_stage_state,(U32 stage, D3DTEXTURESTAGESTATETYPE tss, U32 *value ))
{
	return PIPE->get_texture_stage_state( stage, tss, value );
}

//

DA_METHOD(  set_texture_stage_texture,(U32 stage, U32 htexture ))
{
	GENRESULT result = GR_OK;
#ifndef _ROB
	CQASSERT(htexture == 0 || PIPE->is_texture(htexture) == GR_OK);
#endif

	currentTextureID[stage] = htexture;
	if( !state[RPR_BATCH] )
	{
		if (commitedTextureID[stage] != htexture)
		{
			result = PIPE->set_texture_stage_texture( stage, htexture );
			commitedTextureID[stage] = htexture;
		}
	}

	return result;
}

//

DA_METHOD(  get_texture_stage_texture,(U32 stage, U32 *htexture ))
{
	return PIPE->get_texture_stage_texture( stage, htexture );
}

//

DA_METHOD(	verify_state,())
{
	if( state[RPR_BATCH] )
	{
		U32 texture_stage_cnt = 2;
		for( U32 ts=0; ts<BM_NUM_STAGES; ts++ ) {
			
			PIPE->set_texture_stage_state( ts, D3DTSS_COLOROP, texture_stage_state[ts][D3DTSS_COLOROP] );
			
			if( texture_stage_state[ts][D3DTSS_COLOROP] != D3DTOP_DISABLE ) {
				
				if (currentTextureID[ts] != -1)
				{
					if (commitedTextureID[ts] != currentTextureID[ts])
					{
						PIPE->set_texture_stage_texture( ts, currentTextureID[ts]);
						commitedTextureID[ts] = currentTextureID[ts];
					}
				}
				
				// start at 2, COLOROP is 1, zero is invalid;
				for( U32 tss=2; tss<D3DTSS_MAX_STATE; tss++ ) {
					PIPE->set_texture_stage_state( ts, (D3DTEXTURESTAGESTATETYPE)tss, texture_stage_state[ts][tss] );
				}
			}
			else if (currentTextureID[ts] == 0)
			{
				PIPE->set_texture_stage_texture( ts, 0 );
				commitedTextureID[ts] = 0;
			}
		}
	}
	
	return PIPE->verify_state();
}

//

DA_METHOD(	draw_primitive,(D3DPRIMITIVETYPE type, U32 vertex_format, const void *verts, int num_verts, U32 flags ))
{
	if( num_verts == 0 ) {
		return GR_GENERIC;
	}

	CQASSERT(CQFLAGS.b3DEnabled);
//	__int64 time1,time2;

//	QueryPerformanceCounter((LARGE_INTEGER *)&time1);
	
	if( state[RPR_BATCH] && state[RPR_STATE_ID] != 0) {
		U32 alpha;
		PIPE->get_render_state( D3DRS_ALPHABLENDENABLE, &alpha );
		U32 depth = (state[RPR_BATCH_TRANSLUCENT_MODE]==RPR_TRANSLUCENT_UNSORTED)?0:1;
		U32 pool  = (alpha)?(depth)?1:2:0; 
		if( bm_pools[pool].is_enabled() ) {
			return bm_pools[pool].add_primitive(state[RPR_STATE_ID], state[RPR_STATE_FLAGS], type, vertex_format, verts, num_verts, NULL, 0, flags );
		}
	}
	
	if( state[RPR_BATCH])
	{
		//commit render states
		U32 texture_stage_cnt = 2;
		for( U32 ts=0; ts<BM_NUM_STAGES; ts++ ) {
			
			PIPE->set_texture_stage_state( ts, D3DTSS_COLOROP, texture_stage_state[ts][D3DTSS_COLOROP] );
			
			if( texture_stage_state[ts][D3DTSS_COLOROP] != D3DTOP_DISABLE ) {
				
				if (currentTextureID[ts] != -1)
					if (commitedTextureID[ts] != currentTextureID[ts])
					{
						PIPE->set_texture_stage_texture( ts, currentTextureID[ts]);
						commitedTextureID[ts] = currentTextureID[ts];
					}
					
					// start at 2, COLOROP is 1, zero is invalid;
					for( U32 tss=2; tss<D3DTSS_MAX_STATE; tss++ ) {
						PIPE->set_texture_stage_state( ts, (D3DTEXTURESTAGESTATETYPE)tss, texture_stage_state[ts][tss] );
					}
			}
			else if (currentTextureID[ts] == 0)
			{
				PIPE->set_texture_stage_texture( ts, 0 );
				commitedTextureID[ts] = 0;
			}
		}
		PIPE->set_modelview(modelview);
	}
	
	GENRESULT result = PIPE->draw_primitive( type, vertex_format, verts, num_verts, flags );

//	QueryPerformanceCounter((LARGE_INTEGER *)&time2);

//	ticks += time2-time1;

	CQASSERT(result == GR_OK);

	return result;
}

//

DA_METHOD(	draw_indexed_primitive,(D3DPRIMITIVETYPE type, U32 vertex_format, const void *verts, int num_verts, const U16 * indices, int num_indices, U32 flags ))
{
	if( num_verts == 0 || num_indices == 0 ) {
		return GR_GENERIC;
	}
	
//	__int64 time1,time2;

//	QueryPerformanceCounter((LARGE_INTEGER *)&time1);

	CQASSERT(CQFLAGS.b3DEnabled);

	if( state[RPR_BATCH] && state[RPR_STATE_ID] != 0) {
		U32 alpha;
		PIPE->get_render_state( D3DRS_ALPHABLENDENABLE, &alpha );
		if ((num_verts<BIG_BATCH || alpha || state[RPR_DELAY] != 0)) {
			U32 depth = (state[RPR_BATCH_TRANSLUCENT_MODE]==RPR_TRANSLUCENT_UNSORTED)?0:1;
			U32 pool  = (alpha)?(depth)?1:2:0; 
			if( bm_pools[pool].is_enabled() ) {
				return bm_pools[pool].add_primitive( state[RPR_STATE_ID], state[RPR_STATE_FLAGS], type, vertex_format, verts, num_verts, indices, num_indices, flags );
			}
		}
	}
	
	if( state[RPR_BATCH])
	{
		//commit render states
		U32 texture_stage_cnt = 2;
		for( U32 ts=0; ts<BM_NUM_STAGES; ts++ ) {
			
			PIPE->set_texture_stage_state( ts, D3DTSS_COLOROP, texture_stage_state[ts][D3DTSS_COLOROP] );
			
			if( texture_stage_state[ts][D3DTSS_COLOROP] != D3DTOP_DISABLE ) {
				
				if (currentTextureID[ts] != -1)
					if (commitedTextureID[ts] != currentTextureID[ts])
					{
						PIPE->set_texture_stage_texture( ts, currentTextureID[ts]);
						commitedTextureID[ts] = currentTextureID[ts];
					}
					
					// start at 2, COLOROP is 1, zero is invalid;
					for( U32 tss=2; tss<D3DTSS_MAX_STATE; tss++ ) {
						PIPE->set_texture_stage_state( ts, (D3DTEXTURESTAGESTATETYPE)tss, texture_stage_state[ts][tss] );
					}
			}
			else if (currentTextureID[ts] == 0)
			{
				PIPE->set_texture_stage_texture( ts, 0 );
				commitedTextureID[ts] = 0;
			}
		}
		PIPE->set_modelview(modelview);
	}
	
	GENRESULT result = PIPE->draw_indexed_primitive( type, vertex_format, verts, num_verts, indices, num_indices, flags );
	CQASSERT(result == GR_OK);
	//	QueryPerformanceCounter((LARGE_INTEGER *)&time2);

//	ticks += time2-time1;

	return result;
}

//

DA_METHOD(	flush,(DWORD flags ))
{
	if (bQueueSetup)
	{
		FlushQueue1();
		FlushQueue2();
	}
	//currently I have wired this to auto flush everything.
	if( !flags || (bm_pools[0].is_empty() && bm_pools[1].is_empty() && bm_pools[2].is_empty() ) ) {
		return GR_GENERIC;
	}
	
	TheBigPointer = 0; //we may do some allocating actually during the flush

	CQASSERT((bm_pools[0].mats_rendered==0) && (bm_pools[1].mats_rendered==0) && (bm_pools[0].mats_rendered==0));
	delayStage = 0;
	while (delayStage <= maxDelay)
	{
		if( !bm_pools[0].is_empty()) {
			bm_pools[0].render_no_sort( );
		}
		
		// render two alpha pools as separate entities
		//
		if( !bm_pools[1].is_empty() ) {
			bm_pools[1].render_depth_sort( prim_sort_pool );
		}
		if( !bm_pools[2].is_empty() ) {
			bm_pools[2].render_no_sort( );
		}
		delayStage++;
	}

	CQASSERT((bm_pools[0].mats_rendered==0) && (bm_pools[1].mats_rendered==0) && (bm_pools[0].mats_rendered==0));

	reset_pools((flags & FLUSH_INTERNAL) != 0);



	return GR_OK;
}

bool RenderBatcher::GetPrimBuffer (BATCHDESC *desc,bool bAllowFailure)
{
	in++;
	CQASSERT(in < 2 || bAllowFailure);

#if (defined(_ROB) && defined(_DEBUG))
	CQASSERT(HEAP->EnumerateBlocks());
#endif
	CQASSERT(state[RPR_BATCH]);
	CQASSERT(bBufferLocked == false && "You are already batching one buffer with 0 ID");
	CQASSERT(desc->num_verts);

	//batch only if there is a state ID
	if (state[RPR_STATE_ID] != 0 || activeMaterialHint != U32(-1) || CQRENDERFLAGS.bSoftwareRenderer)
	{
		maxDelay = max(state[RPR_DELAY],maxDelay);

		BATCHEDMATERIAL *material;
		U32 pool;
		U32 alpha;
		if (activeMaterialHint != U32(-1))
		{
		//	SetMStateID(state[RPR_STATE_ID],*hintID,desc->type,desc->vertex_format);
		//	material = &bm_pools[pool].materials[*hintID];
			pool = activeMaterialHint >> 30;
			U32 matID = activeMaterialHint & 0x3fffffff;
			material = &bm_pools[pool].materials[matID];
			activeMaterialHint = U32(-1);
		}
		else
		{
			PIPE->get_render_state( D3DRS_ALPHABLENDENABLE, &alpha );
			U32 depth = (state[RPR_BATCH_TRANSLUCENT_MODE]==RPR_TRANSLUCENT_UNSORTED)?0:1;
			pool  = (alpha)?(depth)?1:2:0; 
			material = bm_pools[pool].get_material_from_state_id(state[RPR_STATE_ID],desc->type,desc->vertex_format);
		}
		alpha = (pool != 0);

		CQASSERT(material->bLocked == 0);

		//batch only if it's alpha, NOT a lot of verts, or delayed
		if (bm_pools[pool].is_enabled() && (alpha || desc->num_verts < BIG_BATCH || state[RPR_DELAY] != 0) || CQRENDERFLAGS.bSoftwareRenderer)
		{
			U32 sizeof_vert = FVF_SIZEOF_VERT(desc->vertex_format);
			U32 prim_mem_offset,i_mem_offset;

			BATCHEDCALLPRIMITIVE *prim = &material->prim;
			BMPOOLJR *vpool;// = material->get_v_pool();
			BMPOOLJR *ipool;
			material->get_pools(&vpool,&ipool);
//			vpool->owner = material;

			bool bMustBatch = (alpha || state[RPR_DELAY] != 0);
			//if we can't cram any more in, flush - no longer appropriate to cry like a baby
			if ( ((prim_mem_offset = vpool->allocate( sizeof_vert * desc->num_verts)) == 0xffffffff) || (i_mem_offset = ipool->allocate( sizeof(U16)*desc->num_indices)) == 0xffffffff) 
			{
				//flush case
				/*	material->restore( activeStatesFlags );
				PIPE->set_modelview(material->prim.modelview);
				prim->render( );
				PIPE->set_render_state(D3DRS_WRAP0, 0);
				PIPE->set_render_state(D3DRS_WRAP1, 0);
				material->empty_primitive();*/
				if (prim_mem_offset != U32(-1))
					vpool->set_pool_used_cnt(0); //unallocate vpool if it succeeded but ipool didn't

				flush(~0);
				if (bAllowFailure)
				{
					in-=2;
					return false;
				}
				material->get_pools(&vpool,&ipool);
				if ( (prim_mem_offset = vpool->allocate( sizeof_vert * desc->num_verts)) == 0xffffffff || (i_mem_offset = ipool->allocate( sizeof(U16)*desc->num_indices)) == 0xffffffff)
				{
					CQBOMB0("Failed to allocate render buffer - see Rob");
					return false;
				}
			}
			
			material->set_modelview(modelview);
			material->bLocked = true;

			if (prim->vpool == 0)
			{
				CQASSERT(prim_mem_offset == 0);
//				prim->verts = prim_mem_offset;
				prim->vpool = vpool;
			}
			desc->verts = prim_mem_offset+vpool->pool;
			
			if (prim->ipool == 0)
			{
				CQASSERT(i_mem_offset == 0);
//				prim->indices = prim_mem_offset;
				prim->ipool = ipool;
			}

			bm_pools[pool].pool_flags &= ~BM_P_F_EMPTY;


			prim->p_type = desc->type;
			prim->p_flags = 0; //?
			CQASSERT(prim->verts_type == desc->vertex_format);
			//prim->verts_type = desc->vertex_format;

			desc->indices = (U16 *)(i_mem_offset+ipool->pool);
			desc->material = material;
			desc->bHasRef = false;

		}
		else
		{
			desc->verts = 0;
			material->set_modelview(modelview);
			U32 sizeof_vert = FVF_SIZEOF_VERT(desc->vertex_format);
			//desc->verts = &vert_buff[buff_pos];
			//			U32 vb_num_verts;
			//			U32 vert_format;
			
			if (sizeof_vert == 24)
			{
				if ((desc->num_verts+vb_used1)*sizeof_vert > 65536 || q_cnt1 == QUEUE_LENGTH || desc->num_indices+id_list_pos1 > 10000)
				{
					//have to flush the vertex buffer here
					FlushQueue1();
				}
				desc->verts = &vb1[vb_used1];
				batchQueue1[q_cnt1].start_vert = vb_used1;
				batchQueue1[q_cnt1].material = material;
				batchQueue1[q_cnt1].modelView = modelview;
				batchQueue1[q_cnt1].id_offset = id_list_pos1;
				batchQueue1[q_cnt1].prim_type = desc->type;
				vb_used1 += desc->num_verts;
				desc->handle = (U32)q_cnt1;
				q_cnt1++;
				desc->indices = &id_list1[id_list_pos1];
				id_list_pos1 += desc->num_indices;
				//CQASSERT(id_list_pos1 < 10000);
			}
			else
			{
				if ((desc->num_verts+vb_used2)*sizeof_vert > 65536 || q_cnt2 == QUEUE_LENGTH || desc->num_indices+id_list_pos2 > 10000)
				{
					//have to flush the vertex buffer here
					FlushQueue2();
				}
				
				desc->verts = &vb2[vb_used2];
				batchQueue2[q_cnt2].start_vert = vb_used2;
				batchQueue2[q_cnt2].material = material;
				batchQueue2[q_cnt2].modelView = modelview;
				batchQueue2[q_cnt2].id_offset = id_list_pos2;
				batchQueue2[q_cnt2].prim_type = desc->type;
				vb_used2 += desc->num_verts;
				desc->handle = (U32)q_cnt2;
				q_cnt2++;
				desc->indices = &id_list2[id_list_pos2];
				id_list_pos2 += desc->num_indices;
			}
			//	VB_MANAGER->acquire_vertex_buffer(desc->vertex_format,desc->num_verts,0,DDLOCK_WRITEONLY | DDLOCK_DISCARDCONTENTS,
			//				IVBM_AVBF_EXACT,&desc->vb_handle,&desc->verts,&vert_format,&vb_num_verts);

		//	CQASSERT(vert_format == desc->vertex_format);

			
			//non-ideal, since we will no longer always actually be using the buffer
	/*		buffer_ref_cnt++;
			if (desc->num_indices)
			{
				desc->indices = (U16 *)&vert_buff[buff_pos];
				buff_pos += 2*desc->num_indices;
				CQASSERT(buff_pos < SCRATCH_BUFF_SIZE && "You ask too much!!");
			}*/


			
			desc->material=material;
			desc->bHasRef = true;
		}
	}
	else
	{
	//	desc->verts = vert_buff;
		U32 vb_num_verts;
		U32 vert_format;	
		VB_MANAGER->acquire_vertex_buffer(desc->vertex_format,desc->num_verts,0,DDLOCK_WRITEONLY | DDLOCK_DISCARDCONTENTS,
					IVBM_AVBF_EXACT,&desc->vb_handle,&desc->verts,&vert_format,&vb_num_verts);

		CQASSERT(vert_format == desc->vertex_format);
		bBufferLocked = true;
		desc->material=0;
		//non-ideal, since we will no longer always actually be using the buffer
			buffer_ref_cnt++;
		if (desc->num_indices)
		{
			desc->indices = (U16 *)&vert_buff[buff_pos];
			buff_pos += 2*desc->num_indices;
			CQASSERT(buff_pos < SCRATCH_BUFF_SIZE && "You ask too much!!");
		}
	}

	return true;

#if (defined(_ROB) && defined(_DEBUG))
	CQASSERT(HEAP->EnumerateBlocks());
#endif
}

void RenderBatcher::ReleasePrimBuffer (BATCHDESC *desc)
{
	in--;
	CQASSERT(in<2);
	if (bBufferLocked == true || desc->bHasRef)
	{
		if (bBufferLocked)
		{
			//commit render states
			U32 texture_stage_cnt = 2;
			for( U32 ts=0; ts<BM_NUM_STAGES; ts++ ) {
				
				PIPE->set_texture_stage_state( ts, D3DTSS_COLOROP, texture_stage_state[ts][D3DTSS_COLOROP] );
				
				if( texture_stage_state[ts][D3DTSS_COLOROP] != D3DTOP_DISABLE ) {
					
					if (currentTextureID[ts] != -1)
					{
						if (commitedTextureID[ts] != currentTextureID[ts])
						{
							PIPE->set_texture_stage_texture( ts, currentTextureID[ts]);
							commitedTextureID[ts] = currentTextureID[ts];
						}
					}
					
					// start at 2, COLOROP is 1, zero is invalid;
					for( U32 tss=2; tss<D3DTSS_MAX_STATE; tss++ ) 
					{
						PIPE->set_texture_stage_state( ts, (D3DTEXTURESTAGESTATETYPE)tss, texture_stage_state[ts][tss] );
					}
				}
				else if (currentTextureID[ts] == 0)
				{
					PIPE->set_texture_stage_texture( ts, 0 );
					commitedTextureID[ts] = 0;
				}
			}
			PIPE->set_modelview(modelview);
			//this is a batch of vertices with no state_id
			bBufferLocked = false;
			GENRESULT result;
			
			PIPE->unlock_vertex_buffer(desc->vb_handle);
			
			if (desc->num_verts)
			{
				if (desc->num_indices)
					result = PIPE->draw_indexed_primitive_vb( desc->type, desc->vb_handle, 0, desc->num_verts, desc->indices, desc->num_indices, 0 );
				else
					result = PIPE->draw_primitive_vb( desc->type, desc->vb_handle, 0, desc->num_verts, 0 );
				
				CQASSERT(result == GR_OK);
			}
			
			VB_MANAGER->release_vertex_buffer(desc->vb_handle);
			
			buffer_ref_cnt--;
			if (buffer_ref_cnt == 0)
			{
				buff_pos = 0;
			}
		}
		else
		{
			desc->material->bLocked = false;
		//	desc->material->restore(activeStatesFlags);
		//	PIPE->set_modelview(desc->material->prim.modelview);
			desc->material->prim.modelview.set_identity();
			//this is a "big" batch of vertices we didn't want to batch
			if (FVF_SIZEOF_VERT(desc->vertex_format) == 24)
			{
				batchQueue1[desc->handle].num_verts = desc->num_verts;
				batchQueue1[desc->handle].num_indices = desc->num_indices;
			}
			else
			{
				batchQueue2[desc->handle].num_verts = desc->num_verts;
				batchQueue2[desc->handle].num_indices = desc->num_indices;
			}
		}
	}
	else
	{
		BATCHEDMATERIAL *material = desc->material;
		
		material->bLocked = false;
		//need to transform vertices
		U32 sizeof_vert = FVF_SIZEOF_VERT(desc->vertex_format);
		
		BATCHEDCALLPRIMITIVE *prim = &material->prim;
		
		U16 idx_cnt = prim->verts_cnt;
		prim->verts_cnt += desc->num_verts;
		
		prim->vpool->set_pool_used_cnt(sizeof_vert*desc->num_verts);
		CQASSERT(prim->vpool->pool_used_cnt == sizeof_vert*prim->verts_cnt);
		
		prim->ipool->set_pool_used_cnt(2*desc->num_indices);
		prim->indices_cnt += desc->num_indices;
		
		CQASSERT(prim->ipool->pool_used_cnt == 2*prim->indices_cnt);

		if (1)//CQRENDERFLAGS.bSoftwareRenderer == false)
		{
			//trying this out!
			if (bIdentityModelview)
				material->bPreTransform = false;
			
			if (material->bPreTransform)
			{
				Transform trans;
				//trans = material->inv*batcher->modelview;
				trans = prim->modelview;
				U8 *buff = (U8 *)desc->verts;
				Vector *vec;
				Vector temp;
				for (int i=0;i<desc->num_verts;i++)
				{
					vec = (Vector *)buff;
					MATH_ENGINE()->transform(temp, trans, *vec);
					*vec = temp;
					buff += sizeof_vert;
				}
				prim->modelview.set_identity();
			}
			
			if (idx_cnt && desc->indices && desc->num_indices) 
			{
				U16 *dest = desc->indices;
				for (int i=0;i<desc->num_indices;i++)
				{
					*dest += idx_cnt;
					CQASSERT(*dest < prim->verts_cnt);
					dest++;
				}
			}
		}
		else
		{
		//this can be called for debugging
			if (prim->verts_cnt)
			{
				material->restore(state[RPR_STATE_FLAGS]);
				PIPE->set_modelview(prim->modelview);
				prim->render();
				material->empty_primitive();
			}
		}
	}

}

void RenderBatcher::Startup()
{
	U32 size;
	U32 flags=0;
	if (CQRENDERFLAGS.bHardwareGeometry==0)
		flags = IRP_VBF_SYSTEM;
	GENRESULT result = PIPE->create_vertex_buffer( D3DFVF_RPVERTEX, 2730, flags, &vb_handle1 );
	CQASSERT(result == GR_OK);
	result = PIPE->lock_vertex_buffer(vb_handle1,DDLOCK_WRITEONLY | DDLOCK_DISCARDCONTENTS,(void **)&vb1,&size);
	CQASSERT(result == GR_OK);
	vb_used1 = 0;
	CQASSERT(size == 2730);
	result = PIPE->create_vertex_buffer( D3DFVF_RPVERTEX2, 2048, flags, &vb_handle2 );
	CQASSERT(result == GR_OK);
	result = PIPE->lock_vertex_buffer(vb_handle2,DDLOCK_WRITEONLY | DDLOCK_DISCARDCONTENTS,(void **)&vb2,&size);
	CQASSERT(result == GR_OK);
	vb_used2 = 0;
	CQASSERT(size == 2048);
	q_cnt1 = q_cnt2 = 0;
	bQueueSetup = true;

	U32 yes;
	PIPE->query_device_ability(RP_A_DEPTH_BIAS,&yes);
	if (yes)
		state[RPR_STATE_FLAGS] |= BM_M_F_HAS_ZBIAS;
	else
		state[RPR_STATE_FLAGS] &= ~BM_M_F_HAS_ZBIAS;
}

void RenderBatcher::Shutdown()
{
	PIPE->unlock_vertex_buffer(vb_handle1);
	PIPE->unlock_vertex_buffer(vb_handle2);
	PIPE->destroy_vertex_buffer(vb_handle1);
	PIPE->destroy_vertex_buffer(vb_handle2);
	VB_MANAGER->remove_all_vertex_buffers();
	bQueueSetup = false;
}

void RenderBatcher::CommitState ()
{
	for( U32 ts=0; ts<BM_NUM_STAGES; ts++ ) {
		
	//	PIPE->set_texture_stage_state( ts, D3DTSS_COLOROP, texture_stage_state[ts][D3DTSS_COLOROP] );
		
//		if( BATCHER->texture_stage_state[ts][D3DTSS_COLOROP] != D3DTOP_DISABLE ) {
			
			if (BATCHER->currentTextureID[ts] != -1)
			{
				if (BATCHER->commitedTextureID[ts] != BATCHER->currentTextureID[ts])
				{
#ifndef _ROB
					CQASSERT(BATCHER->currentTextureID[ts] == 0 || PIPE->is_texture(BATCHER->currentTextureID[ts]) == GR_OK);
#endif		
					PIPE->set_texture_stage_texture( ts, BATCHER->currentTextureID[ts]);
					BATCHER->commitedTextureID[ts] = BATCHER->currentTextureID[ts];
				}
			}
			
			// start at 2, COLOROP is 1, zero is invalid;
			for( U32 tss=1; tss<D3DTSS_MAX_STATE; tss++ ) {
				PIPE->set_texture_stage_state( ts, (D3DTEXTURESTAGESTATETYPE)tss, texture_stage_state[ts][tss] );
			}
		//}
	} 

	PIPE->set_modelview(modelview);
}

void RenderBatcher::RestoreSurfaces ()
{
	Shutdown();
	Startup();
}
//

// ------------------------------------------------------------------
// Batching Related code
// ------------------------------------------------------------------


// ------------------------------------------------------------------
// Startup/shutdown Related Code
// ------------------------------------------------------------------

RenderBatcher::RenderBatcher(void)
{
	memset( state, 0xFF, sizeof(state) );

	num_active_pools = num_pools = 0;

	BATCHER = this;

	bBufferLocked = 0;

	for (int i=0;i<BM_NUM_STAGES;i++)
	{
		commitedTextureID[i] = -1;
	}

	bQueueSetup = false;

	activeMaterialHint = U32(-1);
}

//

RenderBatcher::~RenderBatcher(void)
{
	PIPE = NULL;
	free_all_pools();
}

//

GENRESULT COMAPI RenderBatcher::Initialize(void)
{ 
	IAggregateComponent *IAC = static_cast<IAggregateComponent*>(this);

	U32 value;

	ICOManager *DACOM = DACOM_Acquire();

//	opt_get_u32( DACOM, NULL, "RenderBatcher", "MaterialsMaxCount", 16, &value );

	bm_pools[0].set_materials_size(64);
	bm_pools[1].set_materials_size(3);
	bm_pools[2].set_materials_size(64);

	bm_pools[0].set_name("BM_P_OPAQUE");
	bm_pools[1].set_name("BM_P_ALPHA_DEPTH");
	bm_pools[2].set_name("BM_P_ALPHA_NOSORT");

	bm_pools[0].batcher = this;
	bm_pools[1].batcher = this;
	bm_pools[2].batcher = this;
	
	
	set_state( RPR_BATCH,						opt_get_u32( DACOM, NULL, "RenderBatcher", "BATCH",							FALSE, &value ) );
	set_state( RPR_BATCH_POOLS,					opt_get_u32( DACOM, NULL, "RenderBatcher", "BATCH_POOLS",					RPR_TRANSLUCENT_DEPTH_SORTED|RPR_OPAQUE|RPR_TRANSLUCENT_UNSORTED, &value ) ); 
	set_state( RPR_BATCH_OPAQUE_POOL,			opt_get_u32( DACOM, NULL, "RenderBatcher", "BATCH_OPAQUE_POOL",				RPR_OPAQUE_POOL_SIZE_DEFAULT, &value ) ); 
	set_state( RPR_BATCH_TRANSLUCENT_POOL,		opt_get_u32( DACOM, NULL, "RenderBatcher", "BATCH_TRANSLUCENT_POOL",		RPR_TRANSLUCENT_POOL_SIZE_DEFAULT, &value ) );
	set_state( RPR_BATCH_TRANSLUCENT_NONZ_POOL,	opt_get_u32( DACOM, NULL, "RenderBatcher", "BATCH_TRANSLUCENT_NONZ_POOL",	RPR_TRANSLUCENT_POOL_SIZE_DEFAULT, &value ) );
	set_state( RPR_BATCH_OPAQUE_FLUSH,			opt_get_u32( DACOM, NULL, "RenderBatcher", "BATCH_OPAQUE_FLUSH",			0, &value ) ); 
	set_state( RPR_BATCH_TRANSLUCENT_MODE,		opt_get_u32( DACOM, NULL, "RenderBatcher", "BATCH_TRANSLUCENT_MODE",		RPR_TRANSLUCENT_DEPTH_SORTED, &value ) );
	set_state( RPR_STATE_FLAGS,					opt_get_u32( DACOM, NULL, "RenderBatcher", "STATE_FLAGS",					~(0), &value ) ); 
	set_state( RPR_STATE_ID,					0 );
	set_state( RPR_DELAY,						0 );

	return GR_OK;
}

//

GENRESULT RenderBatcher::init(AGGDESC *desc)
{ 
	// This is called during normal use.  We are an aggregate.
	return GR_OK;
}

// ------------------------------------------------------------------
// DLL Related code
// ------------------------------------------------------------------

// linker bug
void main (void)
{
}

#if USE_HEAP
//--------------------------------------------------------------------------
//  
static void SetDllHeapMsg (HINSTANCE hInstance)
{
	DWORD dwLen;
	char buffer[260];
	
	dwLen = GetModuleFileName(hInstance, buffer, sizeof(buffer));
 
	while (dwLen > 0)
	{
		if (buffer[dwLen] == '\\')
		{
			dwLen++;
			break;
		}
		dwLen--;
	}

	SetDefaultHeapMsg(buffer+dwLen);
}
#endif

//--------------------------------------------------------------------------
//  dummy proc to force an imp lib creation
//
extern "C"
__declspec(dllexport) IHeap * __stdcall GetBatchHeap (void)
{
#if USE_HEAP
	return HEAP;
	//return HEAP->GetHeapSize()-HEAP->GetAvailableMemory();
#else
	return NULL;
#endif
}
//--------------------------------------------------------------------------
// 

#if defined(EDITOR)
	BOOL WINAPI DllMain( HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
#else
	BOOL COMAPI DllMain (HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
#endif
{
	switch (fdwReason)
	{
	//
	// DLL_PROCESS_ATTACH: Create object server component and register it with DACOM manager
	//
		case DLL_PROCESS_ATTACH:
		{
#if USE_HEAP
			HEAP = HEAP_Acquire();
	#if defined(NDEBUG)
			InitializeDAHeap(0x080000, 0x080000, DAHEAPFLAG_PRIVATE|DAHEAPFLAG_GROWHEAP|DAHEAPFLAG_NOMSGS);
	#elif defined(_ROB)
			InitializeDAHeap(0x080000, 0x080000, DAHEAPFLAG_PRIVATE|DAHEAPFLAG_DEBUGFILL_SNAN|DAHEAPFLAG_GROWHEAP|DAHEAPFLAG_NOMSGS);
	#else
			InitializeDAHeap(0x080000, 0x080000, DAHEAPFLAG_PRIVATE|DAHEAPFLAG_DEBUGFILL_SNAN|DAHEAPFLAG_GROWHEAP|DAHEAPFLAG_NOMSGS);
	#endif
			SetDllHeapMsg(hinstDLL);
#endif

#if defined(EDITOR)
			OutputDebugString("Loading RenderBatcher for Editor\n");
#endif

			ICOManager *DACOM = DACOM_Acquire();
			IComponentFactory *server1;
			if( DACOM && (server1 = new DAComponentFactory2<DAComponentAggregate<RenderBatcher>, AGGDESC>("IRenderPrimitive")) != NULL ) {
				DACOM->RegisterComponent( server1, "IRenderPrimitive", DACOM_HIGH_PRIORITY );
				server1->Release();
			}
			break;
		}

	}

	return TRUE;
}

// EOF






