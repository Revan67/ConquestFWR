//$Header: /Libs/dev/Src/EngComps/RenderMgr/RenderMgr.cpp 69    3/21/00 4:30p Pbleisch $

#define WINDOWS_LEAN_AND_MEAN
#include <windows.h>
#pragma warning (disable : 4786 4530)

#include <limits.h>
#include <float.h>
#include <limits>
#include <map>
#include <list>

#include "dacom.h"
#include "TComponent.h"
#include "TSmartPointer.h"
#include "3DMath.h"
#include "FDump.h"
#include "TempStr.h"
#include "SysConsumerDesc.h"
#include "da_heap_utility.h"
#include "AllocLite.h"
#include "persistmisc.h"
#include "IRenderComponent.h"
#include "FileSys_Utility.h"
#include "IProfileParser_Utility.h"
#include "Engine.h"
#include "EngComp.h"
#include "Renderer.h"
#include "ICamera.h"
#include "IRenderPrimitive.h"

#include "Tfuncs.h"

#include "chull.h"
#include "RenderArchetype.h"

//

//EMAURER thanks windows.h...
//PLB This is required so some of the STL code below works.
//PLB If you need min() and max(), use Tmin, Tmax defined in Tfuncs.
#undef max
#undef min

//

const char *CLSID_RenderManager = "RenderManager";

//

//typedef AllocLite<RenderArchetype> APPR_ARCH_ALLOC;
//typedef std::map<ARCHETYPE_INDEX, RenderArchetype, std::less<ARCHETYPE_INDEX>, APPR_ARCH_ALLOC> APPR_ARCH_MAP;
//ALLOCLITE_GLOBAL (APPR_ARCH_ALLOC);
typedef std::map<ARCHETYPE_INDEX, RenderArchetype> APPR_ARCH_MAP;

//typedef AllocLite<ARCHETYPE_INDEX> INST_ALLOC;
//typedef std::map<INSTANCE_INDEX, ARCHETYPE_INDEX, std::less<INSTANCE_INDEX>, INST_ALLOC> INST_MAP;
//ALLOCLITE_GLOBAL (INST_ALLOC);
typedef std::map<INSTANCE_INDEX, ARCHETYPE_INDEX> INST_MAP;

//

typedef std::list<IRenderComponent*> COMPONENT_LIST;

//

struct RenderMgr : public IEngineComponent, 
				   public IRenderer
{
public:	// Data

	BEGIN_DACOM_MAP_INBOUND(RenderMgr)
	DACOM_INTERFACE_ENTRY(IRenderer)
	DACOM_INTERFACE_ENTRY(IEngineComponent)
	DACOM_INTERFACE_ENTRY (IAggregateComponent)
	DACOM_INTERFACE_ENTRY2(IID_IRenderer,IRenderer)
	DACOM_INTERFACE_ENTRY2(IID_IEngineComponent,IEngineComponent)
	DACOM_INTERFACE_ENTRY2(IID_IAggregateComponent,IAggregateComponent)
	END_DACOM_MAP()


protected: // Data

	//EMAURER an archetype entry exists for every Engine archetype.  Used to map one
	//archetype index to multiple appearances.
	APPR_ARCH_MAP archetypes;

	//EMAURER map of instances to archetypes.
	INST_MAP instances;

	//list of components that responded to the enumeration call
	COMPONENT_LIST components;

	RENDER_ARCHETYPE next_render_arch_index;

	IEngine *engine;
	IDAComponent *system;

	IRenderPrimitive *render_primitive;

	bool render_components_loaded;

protected: // Interface

	GENRESULT load_render_components( void );
	GENRESULT unload_render_components( void );

	GENRESULT create_discrete_lod_archetype( ARCHETYPE_INDEX arch_index, IFileSystem *filesys );
	GENRESULT create_continuous_lod_archetype( ARCHETYPE_INDEX arch_index, IFileSystem *filesys );
	GENRESULT create_simple_archetype( ARCHETYPE_INDEX arch_index, IFileSystem *filesys );

	BOOL32 find_visible_rect( INSTANCE_INDEX inst_index, ICamera *camera, float lod_fraction, ViewRect *rect, float & depth );
	BOOL32 find_visible_polys( INSTANCE_INDEX inst_index, ICamera * camera, int &out_num_points, ViewPoint *out_points );

	const RenderArchetypeLod *compute_lod_level( INSTANCE_INDEX inst_index, RenderArchetype *render_arch, ICamera *camera, float distance_scale, float *out_abs_lod_fraction  );
	
	void project_point_list( Vector *dst, const Vector *src, const int count, const ICamera *camera ) const;

public: // Interface

	RenderMgr( void );
	~RenderMgr( void );

	DA_HEAP_DEFINE_NEW_OPERATOR(RenderMgr);

	GENRESULT init( SYSCONSUMERDESC *info );

//IAggregateComponent
	GENRESULT COMAPI Initialize( void );

//IEngineComponent
	void			COMAPI update( SINGLE dt ) ;
	BOOL32			COMAPI create_archetype( ARCHETYPE_INDEX arch_index, struct IFileSystem *filesys ) ;
	void			COMAPI duplicate_archetype( ARCHETYPE_INDEX new_arch_index, ARCHETYPE_INDEX old_arch_index ) ;
	void			COMAPI destroy_archetype( ARCHETYPE_INDEX arch_index ) ;
	GENRESULT		COMAPI query_archetype_interface( ARCHETYPE_INDEX arch_index, const char *iid, IDAComponent **out_iif ) ;
	BOOL32			COMAPI create_instance( INSTANCE_INDEX inst_index, ARCHETYPE_INDEX arch_index ) ;
	void			COMAPI destroy_instance( INSTANCE_INDEX inst_index ) ;
	void			COMAPI update_instance( INSTANCE_INDEX inst_index, SINGLE dt ) ;
	enum vis_state	COMAPI render_instance( struct ICamera *camera, INSTANCE_INDEX inst_index, float lod_fraction, U32 flags, const Transform *modifier_transform ) ;
	GENRESULT		COMAPI query_instance_interface( INSTANCE_INDEX inst_index, const char *iid, IDAComponent **out_iif ) ;

//IRenderer
	GENRESULT	COMAPI set_render_property( const RenderProp name, DACOM_VARIANT value ) ;
	GENRESULT	COMAPI get_render_property( const RenderProp name, DACOM_VARIANT ou_value ) ;
	BOOL32		COMAPI get_archetype_statistics( ARCHETYPE_INDEX arch_index, float lod_fraction, StatType statistic, DACOM_VARIANT out_value ) ;
	BOOL32		COMAPI get_archetype_bounding_box( ARCHETYPE_INDEX arch_index, float lod_fraction, SINGLE *out_box ) ;
	BOOL32		COMAPI get_archetype_bounding_sphere( ARCHETYPE_INDEX arch_index, struct ICamera *camera, float lod_fraction, float& center_x, float& center_y, float& radius, float& depth ) ;		
	BOOL32		COMAPI get_archetype_centroid( ARCHETYPE_INDEX arch_index, float lod_fraction, Vector& out_centroid ) ;
	BOOL32		COMAPI split_archetype( ARCHETYPE_INDEX arch_index, const Vector& normal, float d, ARCHETYPE_INDEX r0, ARCHETYPE_INDEX r1, U32 sa_flags, INSTANCE_INDEX inst_index );
	float		COMAPI compute_lod_fraction( INSTANCE_INDEX inst_index, ICamera *camera, float distance_scale );
	BOOL32		COMAPI get_instance_projected_bounding_box( INSTANCE_INDEX inst_index, struct ICamera *camera, float lod_fraction, struct ViewRect *out_rect, float &out_depth) ;
	BOOL32		COMAPI get_instance_projected_bounding_sphere( INSTANCE_INDEX inst_index, struct ICamera *camera, float lod_fraction, float& center_x, float& center_y, float& radius, float& depth ) ;		
	BOOL32		COMAPI get_instance_projected_bounding_polygon( INSTANCE_INDEX inst_index, struct ICamera *camera, float lod_fraction, int &out_num_vertices, struct ViewPoint *out_vertices, int max_num_vertices, float &out_depth) ;
	// These will eventually go away
	struct Mesh * COMAPI get_instance_mesh( INSTANCE_INDEX inst_index, const ICamera *camera ) ;
	struct Mesh * COMAPI get_unique_instance_mesh( INSTANCE_INDEX inst_index, const ICamera *camera ) ;
	GENRESULT COMAPI release_unique_instance_mesh( INSTANCE_INDEX inst_index ) ;
	struct Mesh * COMAPI get_archetype_mesh( ARCHETYPE_INDEX arch_index, unsigned int level_of_detail ) ;
};

//

RenderMgr::RenderMgr( void )
{
	render_components_loaded = false ;
	
	next_render_arch_index = 0;

	system = NULL;
	engine = NULL;

	render_primitive = NULL;
}

//

RenderMgr::~RenderMgr( void )
{

	INST_MAP::iterator ibeg = instances.begin();
	INST_MAP::iterator iend = instances.end();
	INST_MAP::iterator inst;

	for( inst=ibeg; inst!=iend; inst++ ) {
		GENERAL_NOTICE( TEMPSTR( "RenderMgr: dtor: Instance %d has dangling render archetype %d\n", (*inst).first, (*inst).second ) );
	}

	APPR_ARCH_MAP::iterator abeg = archetypes.begin();
	APPR_ARCH_MAP::iterator aend = archetypes.end();
	APPR_ARCH_MAP::iterator arch;

	for( arch=abeg; arch!=aend; arch++ ) {
		GENERAL_NOTICE( TEMPSTR( "RenderMgr: dtor: Archetype %d has dangling render data\n", (*arch).first ) );
	}


	DACOM_RELEASE( render_primitive );
	DACOM_RELEASE( system );

	engine = NULL;

	FreeConvexHullScratchBuffer ();
	unload_render_components ();
}

//

GENRESULT RenderMgr::init( SYSCONSUMERDESC *info )
{
	if( info == NULL || info->system == NULL ) {
		return GR_INVALID_PARMS;
	}

	system = info->system;
	system->AddRef();

	return GR_OK;
}

//

GENRESULT RenderMgr::Initialize( void )
{
	if( system == NULL ) {
		return GR_GENERIC;
	}

	if( FAILED( system->QueryInterface( IID_IRenderPrimitive, (void**) &render_primitive ) ) ){
		GENERAL_WARNING( "RenderMgr: Initialize: unable to acquire IID_IRenderPrimitive\n" );
		return GR_GENERIC;
	}

	if( FAILED( static_cast<IEngineComponent*>( this )->QueryInterface( IID_IEngine, (void**) &engine ) ) ){
		GENERAL_WARNING( "RenderMgr: Initialize: unable to acquire IID_IEngine\n" );
		return GR_GENERIC;
	}
	engine->Release();

	if( FAILED( load_render_components() ) ) {
		return GR_GENERIC;
	}

	return GR_OK;
}

//

void COMAPI RenderMgr::update (SINGLE dt)
{
	COMPONENT_LIST::iterator cmp;
	COMPONENT_LIST::iterator beg = components.begin();
	COMPONENT_LIST::iterator end = components.end();

	for( cmp = beg; cmp != end; cmp++ ) {
		(*cmp)->update( dt );
	}
}

//

BOOL32 RenderMgr::create_archetype( ARCHETYPE_INDEX arch_index, struct IFileSystem *filesys )
{
	ASSERT( arch_index != INVALID_ARCHETYPE_INDEX );
	ASSERT( filesys != NULL );

	if( archetypes.insert( APPR_ARCH_MAP::value_type( arch_index, RenderArchetype() ) ).second == false ) {
		return FALSE;
	}

	if( SUCCEEDED( create_discrete_lod_archetype( arch_index, filesys ) ) ) {
		return TRUE;
	}
	else if( SUCCEEDED( create_continuous_lod_archetype( arch_index, filesys ) ) ) {
		return TRUE;
	}
	else if( SUCCEEDED( create_simple_archetype( arch_index, filesys ) ) ) {
		return TRUE;
	}

	archetypes.erase( arch_index );

	return FALSE;
}

//

void COMAPI	RenderMgr::duplicate_archetype( ARCHETYPE_INDEX new_arch_index, ARCHETYPE_INDEX old_arch_index )
{
	ASSERT( new_arch_index != INVALID_ARCHETYPE_INDEX );
	ASSERT( old_arch_index != INVALID_ARCHETYPE_INDEX );

	APPR_ARCH_MAP::const_iterator old_arch;
	APPR_ARCH_MAP::const_iterator new_arch;
	RenderArchetype *old_render_arch = NULL;
	RenderArchetype *new_render_arch = NULL;
	RenderArchetypeLod *old_render_lod = NULL;
	RenderArchetypeLod *new_render_lod = NULL;
	
	if( (old_arch = archetypes.find( old_arch_index )) == archetypes.end() ) {
		return;
	}

	if( archetypes.insert( APPR_ARCH_MAP::value_type( new_arch_index, RenderArchetype() ) ).second == false ) {
		return ;
	}

	if( (new_arch = archetypes.find( new_arch_index )) == archetypes.end() ) {
		return;
	}

	old_render_arch = const_cast<RenderArchetype*>( &(*old_arch).second );
	new_render_arch = const_cast<RenderArchetype*>( &(*new_arch).second );

	int num_lods = old_render_arch->get_num_lod_levels();

	new_render_arch->set_num_lod_levels( num_lods );
	new_render_arch->set_maximum_render_distance( old_render_arch->get_maximum_render_distance() );
	new_render_arch->set_minimum_render_distance( old_render_arch->get_minimum_render_distance() );

	new_render_arch->lod_start_dist = old_render_arch->lod_start_dist;
	new_render_arch->lod_stop_dist = old_render_arch->lod_stop_dist;
	for( int lod=0; lod < num_lods; lod++ ) {
		
		old_render_lod = const_cast<RenderArchetypeLod*>( old_render_arch->get_lod_at_level( lod ) );
		new_render_lod = const_cast<RenderArchetypeLod*>( new_render_arch->get_lod_at_level( lod ) );

		new_render_lod->render_arch_index = next_render_arch_index;
		new_render_lod->render_component = old_render_lod->render_component;
		new_render_lod->switch_in_fraction = old_render_lod->switch_in_fraction;

		new_render_lod->render_component->duplicate_archetype( new_render_lod->render_arch_index, old_render_lod->render_arch_index );

		next_render_arch_index++;
	}

}

//

void COMAPI RenderMgr::destroy_archetype( ARCHETYPE_INDEX arch_index )
{
	ASSERT( arch_index != INVALID_ARCHETYPE_INDEX );

	APPR_ARCH_MAP::iterator ai;
	RenderArchetype *render_arch = NULL;
	RenderArchetypeLod *render_lod = NULL;

	if( (ai = archetypes.find( arch_index )) != archetypes.end() ) {
		
		render_arch = const_cast<RenderArchetype*>( &(*ai).second );

		int num_lods = render_arch->get_num_lod_levels();

		for( int lod=0; lod < num_lods; lod++ ) {
			render_lod = const_cast<RenderArchetypeLod*>( render_arch->get_lod_at_level( lod ) );
			render_lod->render_component->destroy_archetype( render_lod->render_arch_index );
		}

		archetypes.erase( ai );
	}
}

//

GENRESULT COMAPI RenderMgr::query_archetype_interface( ARCHETYPE_INDEX arch_index, const char *iid, IDAComponent **out_iif ) 
{
	ASSERT( arch_index != INVALID_ARCHETYPE_INDEX );

	const RenderArchetypeLod *render_lod;
	APPR_ARCH_MAP::iterator ai;

	if( (ai = archetypes.find( arch_index )) == archetypes.end() ) {
		return GR_GENERIC;
	}

	if( (render_lod = (*ai).second.get_lod_at_fraction( 1.0f )) == NULL ) {
		return GR_GENERIC;
	}

	if( !render_lod->render_component->query_archetype_interface( render_lod->render_arch_index, iid, out_iif ) ) {
		return GR_GENERIC;
	}

	return GR_OK;
}

//

BOOL32 COMAPI RenderMgr::create_instance( INSTANCE_INDEX inst_index, ARCHETYPE_INDEX arch_index )
{
	ASSERT( inst_index != INVALID_INSTANCE_INDEX );
	ASSERT( arch_index != INVALID_ARCHETYPE_INDEX );

	const RenderArchetypeLod *render_lod;
	const RenderArchetype    *render_arch;
	APPR_ARCH_MAP::iterator ai;
	INST_MAP::iterator ii;

	if( (ai = archetypes.find( arch_index )) == archetypes.end() ) {
		return FALSE;
	}
	
	if( (ii = instances.insert( INST_MAP::value_type( inst_index, arch_index ) ).first) == instances.end() ) {
		return FALSE;
	}
	
	render_arch = &((*ai).second);

	//
	int num_lods = render_arch->get_num_lod_levels();

	for( int lod=0; lod < num_lods; lod++ ) {
		render_lod = render_arch->get_lod_at_level( lod );
		if( render_lod->render_component->create_instance( inst_index, render_lod->render_arch_index ) == FALSE ) {
			lod--;
			while( lod >= 0 ) {
				render_lod = render_arch->get_lod_at_level( lod );
				render_lod->render_component->destroy_instance( inst_index ) ;
				lod--;
			}
			instances.erase( inst_index );
			return FALSE;
		}
	}

	instances[inst_index] = arch_index;

	return TRUE;
}

//

void COMAPI RenderMgr::destroy_instance( INSTANCE_INDEX inst_index ) 
{
	ASSERT( inst_index != INVALID_INSTANCE_INDEX );

	const RenderArchetypeLod *render_lod;
	const RenderArchetype    *render_arch;
	APPR_ARCH_MAP::iterator ai;
	INST_MAP::iterator ii;

	if( (ii = instances.find( inst_index )) == instances.end() ) {
		return ;
	}

	if( (ai = archetypes.find( ii->second )) == archetypes.end() ) {
		return ;
	}
	
	render_arch = &((*ai).second);

	//
	int num_lods = render_arch->get_num_lod_levels();

	for( int lod=0; lod < num_lods; lod++ ) {
		render_lod = render_arch->get_lod_at_level( lod );
		render_lod->render_component->destroy_instance( inst_index ) ;
	}

	instances.erase( inst_index );
}

//

void COMAPI RenderMgr::update_instance( INSTANCE_INDEX inst_index, SINGLE dt )
{
	COMPONENT_LIST::iterator cmp;
	COMPONENT_LIST::iterator beg = components.begin();
	COMPONENT_LIST::iterator end = components.end();

	// Tells each render component to update the given instance.
	for( cmp = beg; cmp != end; cmp++ ) {
		(*cmp)->update_instance( inst_index, dt );
	}
}

//

vis_state COMAPI RenderMgr::render_instance( ICamera *camera, INSTANCE_INDEX inst_index, float lod_fraction, U32 flags, const Transform *tr )
{
	ASSERT( camera != NULL );
	ASSERT( inst_index != INVALID_INSTANCE_INDEX );
	ASSERT( lod_fraction <= 1.0001f );	
	
	INST_MAP::iterator inst;
	APPR_ARCH_MAP::iterator arch;
	vis_state vs = VS_UNKNOWN;

	if( lod_fraction < 0.0f ) {
		// This can be caused by code that looks like:
		// render_lod_instance( camera, inst_index, compute_lod_fraction(...), ... );
		// i.e. compute_lod_fraction returned saying the object should not be rendered.
		return VS_NOT_VISIBLE;
	}

	// Next, try to cull ourselves + our children
	//
	const Transform & inst_xform = engine->get_transform( inst_index );
	const float max_scale = (tr == NULL)? 1.0F : Tmax( Tmax( tr->get_i().magnitude(), tr->get_j().magnitude() ), tr->get_k().magnitude() );

	if( !(flags & RF_DONT_CULL_FRUSTUM) ) {
		Vector center;
		float radius;
		engine->get_instance_bounding_sphere( inst_index, 0, &radius, &center );
		const Vector view_pos ( camera->get_inverse_transform() * (inst_xform * center) );
			
		vs = camera->object_visibility( view_pos, max_scale * radius );
		if( vs == VS_NOT_VISIBLE || vs == VS_SUB_PIXEL ) {
			return vs;
		}else
		if( vs == VS_FULLY_VISIBLE )
		{
			flags |= (RF_DONT_CULL_FRUSTUM | RF_DONT_CLIP);
		}
	}
	else
	{
		vs = VS_FULLY_VISIBLE;
	}

	Transform *world_adjust_tr_pt = (Transform*)tr;
	Transform world_adjust_tr( false );
		
	// Next, calculate modifier transform as requested.
	//
	if( tr && !(flags & RF_DONT_RECURSE_TRANSFORM) ) {

		if( flags & RF_TRANSLATE_FIRST ) {
			world_adjust_tr = Transform( *(Matrix*)tr, tr->rotate( tr->translation ) );
			flags &= ~RF_TRANSLATE_FIRST;
		}
		else {
			world_adjust_tr = *tr;
		}

		if( flags & RF_TRANSFORM_LOCAL ) {
			// convert to world space
			world_adjust_tr =	inst_xform * world_adjust_tr * inst_xform.get_inverse();
			flags &= ~RF_TRANSFORM_LOCAL;
		}

		flags |= RF_DONT_RECURSE_TRANSFORM;
		world_adjust_tr_pt = &world_adjust_tr;
	}

	const bool has_children =
		(engine->get_instance_child_next( inst_index, EN_DONT_RECURSE, INVALID_INSTANCE_INDEX ) != INVALID_INSTANCE_INDEX);

	// NOTE: this instance or archetype might be invalid (if it has nothing to render like a bone) but children might be valid
	if( (inst = instances.find( inst_index )) != instances.end() ) {
		if( (arch = archetypes.find( inst->second )) != archetypes.end() ) {

			U32 abs_flags = flags;

			// see if we can get culled out w/o children
			if( !(flags & RF_DONT_CULL_FRUSTUM) && has_children ) {
				Vector center;
				float radius;
				engine->get_instance_bounding_sphere( inst_index, EN_DONT_RECURSE, &radius, &center );
				const Vector view_pos ( camera->get_inverse_transform() * (inst_xform * center) );
				
				vs = camera->object_visibility( view_pos, max_scale * radius );
				if( vs == VS_FULLY_VISIBLE ) {
					abs_flags |= RF_DONT_CLIP | RF_DONT_CULL_FRUSTUM; // this is for the benefit of render components that still cull
				}
			}
			//else we already know the vs from above since it's the same regradless of EN_DONT_RECURSE

			if( vs >= VS_PARTIALLY_VISIBLE ) {

				// calc lod if necessary 
				//
				const RenderArchetypeLod *render_lod;
				float abs_lod_fraction;

				if( (flags & RF_RELATIVE_LOD) && !(flags & RF_SAME_LOD) ) {
					abs_lod_fraction = compute_lod_fraction( inst_index, camera, lod_fraction );
					abs_flags &= ~(RF_RELATIVE_LOD);
				}
				else
				{
					abs_lod_fraction = lod_fraction;
				}
					
				if( (render_lod = arch->second.get_lod_at_fraction( abs_lod_fraction )) != NULL ) {

					if( abs_flags & RF_DONT_CLIP ) {
						render_primitive->set_render_state( D3DRS_CLIPPING, FALSE );
					}
					else {
						render_primitive->set_render_state( D3DRS_CLIPPING, TRUE );
					}

					render_lod->render_component->render_instance( inst_index, render_lod->render_arch_index, camera, abs_lod_fraction, abs_flags, world_adjust_tr_pt );
				}
				else
				{
					vs = VS_UNKNOWN;
				}
			}
		}
	}

	// render children
	//
	if( has_children ) {
		INSTANCE_INDEX child = INVALID_INSTANCE_INDEX;
		vis_state child_vs;
		
		while( INVALID_INSTANCE_INDEX != (child = engine->get_instance_child_next( inst_index, EN_DONT_RECURSE, child )) ) {
			
			child_vs = render_instance( camera, child, lod_fraction, flags, world_adjust_tr_pt );

			if( vs != child_vs ) {
				vs = Tmin( Tmax( vs, child_vs ), VS_PARTIALLY_VISIBLE );
			}
		}
	}

	render_primitive->set_render_state( D3DRS_CLIPPING, TRUE );

	return vs;
}

//

GENRESULT COMAPI RenderMgr::query_instance_interface( INSTANCE_INDEX inst_index, const char *iid, IDAComponent **out_iif ) 
{
	ASSERT( inst_index != INVALID_INSTANCE_INDEX );

	const RenderArchetypeLod *render_lod;
	APPR_ARCH_MAP::iterator ai;
	INST_MAP::iterator ii;

	if( (ii = instances.find( inst_index)) == instances.end() ) {
		return GR_GENERIC;
	}

	if( (ai = archetypes.find( (*ii).second )) == archetypes.end() ) {
		return GR_GENERIC;
	}

	if( (render_lod = (*ai).second.get_lod_at_fraction( 1.0f )) == NULL ) {
		return GR_GENERIC;
	}

	if( !render_lod->render_component->query_instance_interface( inst_index, render_lod->render_arch_index, iid, out_iif ) ) {
		return GR_GENERIC;
	}

	return GR_OK;
}

//

GENRESULT RenderMgr::set_render_property( const RenderProp render_prop, DACOM_VARIANT value )
{
	GENRESULT result = GR_GENERIC;

	COMPONENT_LIST::iterator cmp;
	COMPONENT_LIST::iterator beg = components.begin();
	COMPONENT_LIST::iterator end = components.end();

	for( cmp = beg; cmp != end; cmp++ ) {
		if( SUCCEEDED( (*cmp)->set_render_property( render_prop, value ) ) ) {
			result = GR_OK;
		}
	}

	return result;
}

GENRESULT RenderMgr::get_render_property( const RenderProp render_prop, DACOM_VARIANT out_value )
{
	COMPONENT_LIST::iterator cmp;
	COMPONENT_LIST::iterator beg = components.begin();
	COMPONENT_LIST::iterator end = components.end();

	for( cmp = beg; cmp != end; cmp++ ) {
		if( SUCCEEDED( (*cmp)->get_render_property( render_prop, out_value ) ) ) {
			return GR_OK;
		}	
	}

	return GR_GENERIC;
}

//

BOOL32 COMAPI RenderMgr::get_archetype_statistics( ARCHETYPE_INDEX arch_index, float lod_fraction, StatType statistic, DACOM_VARIANT out_value ) 
{
	ASSERT( arch_index != INVALID_ARCHETYPE_INDEX );
	ASSERT( lod_fraction >= 0.0f );
	ASSERT( lod_fraction < 1.0001f );

	APPR_ARCH_MAP::iterator ai;
	const RenderArchetypeLod *render_lod; 
	Vector centroid;

	if( (ai = archetypes.find( arch_index )) == archetypes.end() ) {
		return FALSE;
	}

	if( (render_lod = (*ai).second.get_lod_at_fraction( lod_fraction )) == NULL ) {
		return FALSE;
	}

	return render_lod->render_component->get_archetype_statistics( render_lod->render_arch_index, lod_fraction, statistic, out_value );
}

//

BOOL32 RenderMgr::get_archetype_bounding_box( ARCHETYPE_INDEX arch_index, float lod_fraction, SINGLE *box )
{
	ASSERT( arch_index != INVALID_ARCHETYPE_INDEX );
	ASSERT( box != NULL );
	ASSERT( lod_fraction >= 0.0f );
	ASSERT( lod_fraction < 1.0001f );

	APPR_ARCH_MAP::iterator ai;
	const RenderArchetypeLod *render_lod; 

	if( (ai = archetypes.find( arch_index )) == archetypes.end() ) {
		return FALSE;
	}

	if( (render_lod = (*ai).second.get_lod_at_fraction( lod_fraction )) == NULL ) {
		return FALSE;
	}

	return render_lod->render_component->get_archetype_bounding_box( render_lod->render_arch_index, lod_fraction, box );
}

//

BOOL32 RenderMgr::get_archetype_centroid( ARCHETYPE_INDEX arch_index, float lod_fraction, Vector& centroid )
{
	ASSERT( arch_index != INVALID_ARCHETYPE_INDEX );
	ASSERT( lod_fraction >= 0.0f );
	ASSERT( lod_fraction < 1.0001f );

	APPR_ARCH_MAP::iterator ai;
	const RenderArchetypeLod *render_lod; 

	if( (ai = archetypes.find( arch_index )) == archetypes.end() ) {
		return FALSE;
	}

	if( (render_lod = (*ai).second.get_lod_at_fraction( lod_fraction )) == NULL ) {
		return FALSE;
	}

	return render_lod->render_component->get_archetype_centroid( render_lod->render_arch_index, lod_fraction, centroid ) ;
}

//

BOOL32 RenderMgr::split_archetype( ARCHETYPE_INDEX arch_index, const Vector& plane_normal, float plane_d, ARCHETYPE_INDEX new_arch_index_0, ARCHETYPE_INDEX new_arch_index_1, U32 sa_flags, INSTANCE_INDEX inst_index )
{
	ASSERT( arch_index != INVALID_ARCHETYPE_INDEX );
	ASSERT( new_arch_index_0 != INVALID_ARCHETYPE_INDEX );
	ASSERT( new_arch_index_1 != INVALID_ARCHETYPE_INDEX );

	BOOL32 ret;

	APPR_ARCH_MAP::iterator ai;
	APPR_ARCH_MAP::iterator nai_0;
	APPR_ARCH_MAP::iterator nai_1;

	const RenderArchetypeLod *render_lod;
	const RenderArchetype    *render_arch;

	RENDER_ARCHETYPE new_render_arch_0;
	RENDER_ARCHETYPE new_render_arch_1;

	if( (ai = archetypes.find( arch_index )) == archetypes.end() ) {
		return FALSE;
	}

	// clean in case the new archetypes already have data
	destroy_archetype( new_arch_index_0 );
	destroy_archetype( new_arch_index_1 );


	render_arch = &((*ai).second);

	// first clone the archetypes, then we will overwrite the necessary data
	//
	nai_0 = archetypes.insert( APPR_ARCH_MAP::value_type( new_arch_index_0, *render_arch ) ).first;
	nai_1 = archetypes.insert( APPR_ARCH_MAP::value_type( new_arch_index_1, *render_arch ) ).first;

	// now, go through all of the lod levels and replace the old render archetype
	// with the new split one(s) for each new archetype
	//
	int num_lods = render_arch->get_num_lod_levels();

	for( int lod=0; lod < num_lods; lod++ ) {
		
		render_lod = render_arch->get_lod_at_level( lod );

		// Allocate new archetype indices
		//
		new_render_arch_0 = next_render_arch_index++;
		new_render_arch_1 = next_render_arch_index++;

		ret = render_lod->render_component->split_archetype(	render_lod->render_arch_index, 
																plane_normal, 
																plane_d, 
																new_render_arch_0,
																new_render_arch_1,
																sa_flags,
																inst_index );

		if( ret ) {
			(*nai_0).second.set_render_archetype_at_level( lod, new_render_arch_0 );
			(*nai_1).second.set_render_archetype_at_level( lod, new_render_arch_1 );
		}
		else {
			archetypes.erase( nai_0 );
			archetypes.erase( nai_1 );
			return FALSE;
		}
	}

	return TRUE;
}

//

float RenderMgr::compute_lod_fraction( INSTANCE_INDEX inst_index, ICamera *camera, float distance_scale )
{
	ASSERT( inst_index != INVALID_INSTANCE_INDEX );
	ASSERT( camera != NULL );
	ASSERT( distance_scale >= 0.0f );

	INST_MAP::iterator ii;
	APPR_ARCH_MAP::iterator ai;
	Vector centroid;

	if( (ii = instances.find( inst_index )) == instances.end() ) {
		return -1.0f;
	}

	if( (ai = archetypes.find( (*ii).second )) == archetypes.end() ) {
		return -1.0f;
	}

	// this is bad data but it was used by particle editor to mean always render
	float drop_dist_min = distance_scale * ai->second.get_minimum_render_distance();
	float drop_dist_max = distance_scale * ai->second.get_maximum_render_distance();
	if( drop_dist_min >= drop_dist_max ) {
		return 1.0f;
	}

	float dist = (engine->get_position( inst_index ) - camera->get_position()).magnitude();
	// outside of range; don't render
	if( dist < drop_dist_min || dist > drop_dist_max ) {
		return -1.0f;
	}

	float d_min = distance_scale * ai->second.lod_start_dist;
	float d_max = distance_scale * ai->second.lod_stop_dist;

	// relative to 45Deg fovx; 1.0/tan(45.0) -> 1.0
	float inv_zoom;
	if( fabs( 45.0f - camera->get_fovx() ) > .001f ) { // avoid computing tan in most cases
		inv_zoom = (float)tan( MUL_DEG_TO_RAD * camera->get_fovx() );
	}
	else {
		inv_zoom = 1.0f;
	}

	const ViewRect *p = camera->get_pane();
	float res_adjust = 640.0f / ( ((float)(p->x1) + 1.0f) - (float)(p->x0) );

	float fraction = (d_max - res_adjust * inv_zoom * dist) / (d_max - d_min);

	return Tclamp( 0.0f, 1.0f, fraction );
}

//  since optimizations mangle the floating point data in this function, I had to turn them off - rmarr
#pragma optimize( "g", off )
void RenderMgr::project_point_list( Vector *dst, const Vector *src, const int count, const ICamera *camera ) const
{
	MATH_ENGINE()->transform_list( dst, camera->get_inverse_transform(), src, count );

	float znear = camera->get_znear();
	const ViewRect *vr = camera->get_pane();

	float pane_x0 = vr->x0;
	float pane_y0 = vr->y0;

	float x_screen_center = .5f * (vr->x1 - vr->x0);
	float y_screen_center = .5f * (vr->y1 - vr->y0);

	float hpc = camera->get_hpc();
	float vpc = camera->get_vpc();

	for(int i = 0; i < count; i++)
	{
		if( dst[i].z <= -znear )
		{
			float w = -znear / dst[i].z;
			//float w = -1.0f / view_vector.z;
			dst[i].x = pane_x0 + x_screen_center + dst[i].x * w * hpc;
			dst[i].y = pane_y0 + y_screen_center + dst[i].y * w * vpc;
			dst[i].z = -dst[i].z;
		}
		else
		{
			dst[i].z = 0.0f; // signals failure
		}
	}
}
#pragma optimize( "g", on )

BOOL32 RenderMgr::find_visible_rect( INSTANCE_INDEX inst_index, ICamera *camera, float lod_fraction, ViewRect *rect, float & depth )
{
	BOOL32 result = FALSE;

	const Vector &pos = engine->get_position( inst_index );

	rect->x0 = rect->y0 = LONG_MAX;
	rect->x1 = rect->y1 = LONG_MIN;
	depth = 0.0f;

	float x, y;

	if( !camera->point_to_screen( x, y, depth, pos ) ) {
		return result;
	}

	INST_MAP::iterator inst;
	if( (inst = instances.find( inst_index )) == instances.end() ) {
		return result;
	}

	APPR_ARCH_MAP::iterator arch;
	if( (arch = archetypes.find( inst->second )) == archetypes.end() ) {
		return result;
	}

	const RenderArchetypeLod *render_lod = arch->second.get_lod_at_fraction( lod_fraction );

	if( !render_lod )
	{
		return result;
	}

	SINGLE box[6];
	if( render_lod->render_component->get_instance_bounding_box( inst_index, render_lod->render_arch_index, lod_fraction, box ) == FALSE )
	{
		return result;
	}

	const Transform & xform = engine->get_transform( inst_index );

	Vector corners[8] = {
		xform * Vector(box[1], box[2], box[4]),
		xform * Vector(box[0], box[2], box[4]),
		xform * Vector(box[1], box[3], box[4]),
		xform * Vector(box[0], box[3], box[4]),

		xform * Vector(box[0], box[2], box[5]),
		xform * Vector(box[1], box[2], box[5]),
		xform * Vector(box[0], box[3], box[5]),
		xform * Vector(box[1], box[3], box[5]) };

	// Project bounding box to screen coords.
	Vector projected[8];
	project_point_list( projected, corners, 8, camera );
	for( int i = 0; i < 8; i++)
	{
		if( projected[i].z != 0.0f )
		{
			rect->x0 = Tmin<long>( rect->x0, projected[i].x );
			rect->y0 = Tmin<long>( rect->y0, projected[i].y );
			rect->x1 = Tmax<long>( rect->x1, projected[i].x );
			rect->y1 = Tmax<long>( rect->y1, projected[i].y );
			depth = Tmax(depth, projected[i].z);

			result = TRUE;
		}
	}

	return result;
}

void ExpandViewRect( ViewRect & dst, const ViewRect & src )
{
	dst.x0 = Tmin(dst.x0, src.x0);
	dst.y0 = Tmin(dst.y0, src.y0);
	dst.x1 = Tmax(dst.x1, src.x1);
	dst.y1 = Tmax(dst.y1, src.y1);
}

BOOL32 RenderMgr::get_instance_projected_bounding_box( INSTANCE_INDEX inst_index, ICamera *camera, float lod_fraction,
													  ViewRect *rect, float & depth )
{
	ASSERT( inst_index != INVALID_INSTANCE_INDEX );
	ASSERT( camera != NULL );
	ASSERT( rect != NULL );

	BOOL32 result = find_visible_rect(inst_index, camera, lod_fraction, rect, depth);

	ViewRect child_rect;
	float child_depth;
	INSTANCE_INDEX child = INVALID_INSTANCE_INDEX;
	
	while( INVALID_INSTANCE_INDEX != (child = engine->get_instance_child_next( inst_index, EN_DONT_RECURSE, child )) )
	{
		if( TRUE == find_visible_rect(child, camera, lod_fraction, &child_rect, child_depth) )
		{
			result = TRUE;
			ExpandViewRect( *rect, child_rect );
			depth = Tmax(depth, child_depth);
		}
	}

	return result;
}

//

BOOL32 COMAPI RenderMgr::get_instance_projected_bounding_sphere( INSTANCE_INDEX inst_index, ICamera *camera,
																float lod_fraction, float &center_x, float &center_y, float &radius,
																float & depth )
{
	ASSERT( inst_index != INVALID_INSTANCE_INDEX );
	ASSERT( camera != NULL );

	const RenderArchetypeLod *render_lod;
	Vector center;
	float c_radius;

	if( (render_lod = compute_lod_level( inst_index, NULL, camera, 1.0f, NULL )) == NULL ) {
		return FALSE;
	}

//	engine->get_compound_radius(inst_index, &c_radius, &center );
	engine->get_instance_bounding_sphere( inst_index, 0, &c_radius, &center );

	// project sphere
	//
	const Transform &cam2world = camera->get_transform();
	Vector wcenter = engine->get_transform( inst_index ) * center;
	Vector vcenter = cam2world.inverse_rotate_translate( wcenter ); // center in camera

	// Make sure object is in front of near plane.
	if( !(vcenter.z < camera->get_znear()) ) {
		return FALSE;
	}

	depth = vcenter.z;

	const struct ViewRect *pane = camera->get_pane();

	float x_screen_center = float(pane->x1 - pane->x0) * 0.5f;
	float y_screen_center = float(pane->y1 - pane->y0) * 0.5f;
	float screen_center_x = pane->x0 + x_screen_center;
	float screen_center_y = pane->y0 + y_screen_center;

	float w = -1.0 / vcenter.z;
	float sphere_center_x = vcenter.x * w;
	float sphere_center_y = vcenter.y * w;

	center_x = screen_center_x + sphere_center_x * camera->get_hpc() * camera->get_znear();
	center_y = screen_center_y + sphere_center_y * camera->get_vpc() * camera->get_znear();

	const Matrix &Rc = cam2world.get_orientation();

	float center_distance = vcenter.magnitude();

	if( center_distance >= c_radius ) {

		float dx = fabs( center_x - screen_center_x );
		float dy = fabs( center_y - screen_center_y );

		//changes 1/26 - rmarr
		//function should now not return TRUE with obscene radii
		float outer_angle = asin( c_radius / center_distance );
		sphere_center_x = fabs( sphere_center_x );
		float inner_angle = atan( sphere_center_x );

		float near_plane_radius = tan( inner_angle - outer_angle );
		near_plane_radius = sphere_center_x-near_plane_radius;
		radius = near_plane_radius * camera->get_hpc() *camera->get_znear();

		int view_w = (pane->x1 - pane->x0 + 1) >> 1;
		int view_h = (pane->y1 - pane->y0 + 1) >> 1;

		if( (dx < (view_w + radius)) && (dy < (view_h + radius)) ) {
			return TRUE;
		}
	}

	return FALSE;
}

//

BOOL32 COMAPI RenderMgr::get_instance_projected_bounding_polygon( INSTANCE_INDEX inst_index,
																 ICamera *camera, float lod_fraction,
																 int &out_num_points, ViewPoint *out_points,
																 int max_num_points, float &out_depth )
{
	ASSERT( inst_index != INVALID_INSTANCE_INDEX );
	ASSERT( camera != NULL );
	ASSERT( out_points != NULL );

	
	const Vector &pos = engine->get_position( inst_index );
	float x, y;
	if( !camera->point_to_screen( x, y, out_depth, pos ) )
	{
		return FALSE;
	}


	int points = 0;
	ViewPoint all_points[1024];
	find_visible_polys( inst_index, camera, points, all_points );

	if( points == 0 )
	{
		return FALSE;
	}

	const struct ViewRect *pane = camera->get_pane();
	int i;
	ViewPoint *pt;
	for( i=0, pt=all_points; i < points; i++, pt++ )
	{
		if( (pt->x >= pane->x0) && (pt->x <= pane->x1) && (pt->y >= pane->y0) && (pt->y <= pane->y1) )
		{
			break;
		}
	}
	
	// if we hit the break above, i will be less than points;
	// if not, i will be == points
	//
	if( i != points )
	{
		out_num_points = ComputeConvexHull( out_points, max_num_points, all_points, points );
		 return (out_num_points != 0);
	}

	return FALSE;
}

//

BOOL32 RenderMgr::find_visible_polys( INSTANCE_INDEX inst_index, ICamera *camera,
									 int &out_num_points,
									 ViewPoint *out_points )
{
	const RenderArchetypeLod * render_lod;
	if( (render_lod = compute_lod_level( inst_index, NULL, camera, 1.0f, NULL )) == NULL )
	{
		return FALSE;
	}

	SINGLE box[6];
	render_lod->render_component->get_instance_bounding_box( inst_index, render_lod->render_arch_index, 1.0f, box );
		
	// project points
	const Transform & xform = engine->get_transform( inst_index );

	Vector corners[8] = {
		xform * Vector(box[1], box[2], box[4]),
		xform * Vector(box[0], box[2], box[4]),
		xform * Vector(box[1], box[3], box[4]),
		xform * Vector(box[0], box[3], box[4]),

		xform * Vector(box[0], box[2], box[5]),
		xform * Vector(box[1], box[2], box[5]),
		xform * Vector(box[0], box[3], box[5]),
		xform * Vector(box[1], box[3], box[5]) };

	// Project bounding box to screen coords.
	Vector projected[8];
	project_point_list( projected, corners, 8, camera );

	for(int i = 0; i < 8; i++)
	{
		if( projected[i].z != 0.0f )
		{
			out_points[out_num_points].x = projected[i].x;
			out_points[out_num_points].y = projected[i].y;
			out_num_points++;
		}
	}

	INSTANCE_INDEX child = INVALID_INSTANCE_INDEX;
	while( INVALID_INSTANCE_INDEX != (child = engine->get_instance_child_next( inst_index, EN_DONT_RECURSE, child )) ) {
		find_visible_polys( child, camera, out_num_points, out_points );
	}

	return TRUE;
}

//

struct Mesh * COMAPI RenderMgr::get_instance_mesh( INSTANCE_INDEX inst_index, const ICamera *camera )
{
	ASSERT( inst_index != INVALID_INSTANCE_INDEX );

	const RenderArchetypeLod *render_lod = NULL;
	APPR_ARCH_MAP::iterator ai;
	INST_MAP::iterator ii;
	float lod_fraction ;

	if( (ii = instances.find( inst_index )) == instances.end() ) {
		return NULL;
	}

	if( (ai = archetypes.find( ii->second )) == archetypes.end() ) {
		return NULL;
	}

	if( camera != NULL ) {
		lod_fraction = compute_lod_fraction( inst_index, const_cast<ICamera*>(camera), 1.0f );
	}
	else {
		lod_fraction = 1.0f;
	}

	if( (render_lod = ai->second.get_lod_at_fraction( lod_fraction )) ) {
		return render_lod->render_component->get_instance_mesh( inst_index, render_lod->render_arch_index );
	}

	return NULL;
}

//

struct Mesh * COMAPI RenderMgr::get_unique_instance_mesh( INSTANCE_INDEX inst_index, const ICamera *camera )
{
	ASSERT( inst_index != INVALID_INSTANCE_INDEX );

	const RenderArchetypeLod *render_lod = NULL;
	APPR_ARCH_MAP::iterator ai;
	INST_MAP::iterator ii;
	float lod_fraction ;

	if( (ii = instances.find( inst_index )) == instances.end() ) {
		return NULL;
	}

	if( (ai = archetypes.find( ii->second )) == archetypes.end() ) {
		return NULL;
	}

	if( camera != NULL ) {
		lod_fraction = compute_lod_fraction( inst_index, const_cast<ICamera*>(camera), 1.0f );
	}
	else {
		lod_fraction = 1.0f;
	}

	if( (render_lod = ai->second.get_lod_at_fraction( lod_fraction )) ) {
		return render_lod->render_component->get_unique_instance_mesh( inst_index, render_lod->render_arch_index );
	}

	return NULL;
}

//

GENRESULT COMAPI RenderMgr::release_unique_instance_mesh( INSTANCE_INDEX inst_index )
{
	ASSERT( inst_index != INVALID_INSTANCE_INDEX );

	const RenderArchetypeLod *render_lod = NULL;
	APPR_ARCH_MAP::iterator ai;
	INST_MAP::iterator ii;

	if( (ii = instances.find( inst_index )) == instances.end() ) {
		return GR_GENERIC;
	}

	if( (ai = archetypes.find( ii->second )) == archetypes.end() ) {
		return GR_GENERIC;
	}

	if( (render_lod = ai->second.get_lod_at_fraction( 1.0f )) ) {
		return render_lod->render_component->release_unique_instance_mesh( inst_index, render_lod->render_arch_index );
	}

	return GR_GENERIC;
}

//

struct Mesh * COMAPI RenderMgr::get_archetype_mesh( ARCHETYPE_INDEX arch_index, unsigned int  )
{
	ASSERT( arch_index != INVALID_ARCHETYPE_INDEX );

	const RenderArchetypeLod *render_lod = NULL;
	APPR_ARCH_MAP::iterator ai;

	if( (ai = archetypes.find( arch_index )) == archetypes.end() ) {
		return NULL;
	}

	if( (render_lod = ai->second.get_lod_at_fraction( 1.0f )) ) {
		return render_lod->render_component->get_archetype_mesh( render_lod->render_arch_index );
	}

	return NULL;
}

//

GENRESULT RenderMgr::load_render_components (void)
{
	if( render_components_loaded ) {
		unload_render_components();
	}

	COMPTR<IProfileParser> IPP;
	HANDLE hSection;
	char line[1024+1], *clsid, *p;
	U32 line_num;

	RendCompDesc info;
	IRenderComponent *render_comp;

	if( FAILED( DACOM_Acquire()->QueryInterface( IID_IProfileParser, IPP ) ) ) {
		GENERAL_WARNING( "RenderMgr: load_engine_components: unable to acquire IID_IProfileParser\n" );
		return GR_GENERIC;
	}

	if( (hSection = IPP->CreateSection( CLSID_RenderManager ) ) == 0 ) {
		GENERAL_WARNING( "RenderMgr: load_engine_components: [RenderManager] section not found!" );
		return GR_GENERIC;
	}

	for( line_num = 0; IPP->ReadProfileLine( hSection, line_num, line, 1024 ); line_num++ ) {

		// remove leading whitespace
		for( p = line; *p && (*p == '\t' && *p == ' '); p++ );

		// check for comment
		if( *p == ';' ) {
			continue;
		}

		clsid = p;

		// find end of clsid
		for( p = clsid; *p && (*p != '\t' && *p != ' ' && *p != ';' ); p++ );

		*p = 0;

		info.interface_name = clsid;
		info.engine_services = engine;
		info.system_services = system;

		// create clsid and aggregate it.
		if( FAILED( DACOM_Acquire()->CreateInstance( &info, (void **)&render_comp ) ) ) {
			GENERAL_TRACE_1( TEMPSTR( "RenderMgr: load_render_components: '%s' is not available\n", clsid ) );
		}
		else {
			components.push_back( render_comp );
		}
	}

	IPP->CloseSection( hSection );

	render_components_loaded = true;

	return GR_OK;
}

//

GENRESULT RenderMgr::unload_render_components (void)
{
	COMPONENT_LIST::iterator cmp;
	COMPONENT_LIST::iterator beg = components.begin();
	COMPONENT_LIST::iterator end = components.end();

	for( cmp = beg; cmp != end; cmp++ ) {
		(*cmp)->Release ();
	}

	components.clear ();

	render_components_loaded = false;
	
	return GR_OK;
}

//

GENRESULT RenderMgr::create_discrete_lod_archetype( ARCHETYPE_INDEX arch_index, IFileSystem *filesys )
{
	COMPTR<IFileSystem> lodDir;
	DAFILEDESC discrete_lod( MULTI_APPEARANCE_DIR );
	APPR_ARCH_MAP::iterator ai;
	RenderArchetype *render_arch;
	SWITCH_DISTANCE_TYPE min_max_render[2];
	bool got_min_max = true;

	if( (ai = archetypes.find( arch_index )) == archetypes.end() ) {
		return GR_GENERIC;
	}
	
	render_arch = const_cast<RenderArchetype*>( &((*ai).second) );

	if( FAILED( filesys->CreateInstance( &discrete_lod, lodDir ) ) ) {
		return GR_GENERIC;
	}

	SWITCH_DISTANCE_TYPE switch_in_distance;
	SWITCH_DISTANCE_TYPE *switch_in_distances;
	unsigned long num_switch_in_distances;

	if( FAILED( ReadAllocVector( lodDir, SWITCH_DIST_FILE, num_switch_in_distances, switch_in_distances ) ) ){
		GENERAL_TRACE_1( "RenderMgr: create_discrete_lod_archetype: no switches chunk, defaulting to minimum distance and one (1) lod level\n" );
		switch_in_distances = &switch_in_distance;
		num_switch_in_distances = 0;
	}
	
	if( FAILED( read_type_array( lodDir, MIN_MAX_DIST_FILE, 2, min_max_render ) ) ) {
		GENERAL_TRACE_1( "RenderMgr: create_discrete_lod_archetype: no minmax chunk, defaulting to minimum and maximum render distances\n" );
		min_max_render[0] = 0.0f;
		min_max_render[1] = std::numeric_limits<SWITCH_DISTANCE_TYPE>::max();
		got_min_max = false;
	}

	switch_in_distance = min_max_render[0];

	float lod_fraction, inv_lod_fraction_span;

	if( got_min_max ) {
		inv_lod_fraction_span = 1.0f / ( min_max_render[1] - min_max_render[0] ) ;
	}
	else if( num_switch_in_distances ) {
		inv_lod_fraction_span = 1.0f / ( switch_in_distances[num_switch_in_distances-1] - min_max_render[0] ) ;
	}
	else {
		inv_lod_fraction_span = 0.0f;
	}
	
	render_arch->set_minimum_render_distance( min_max_render[0] );
	render_arch->set_maximum_render_distance( min_max_render[1] );
	render_arch->lod_start_dist = 0.0f;
	render_arch->lod_stop_dist = (num_switch_in_distances)
		? switch_in_distances[num_switch_in_distances-1] 
		: std::numeric_limits<SWITCH_DISTANCE_TYPE>::max();

	render_arch->set_num_lod_levels( num_switch_in_distances + 1 );
	render_arch->set_switch_in_fraction_at_level( 0, 0.0f );	// HMMM???
	for( int lod = 1; lod < num_switch_in_distances + 1; lod++ ) {
		lod_fraction = 0.99999f - inv_lod_fraction_span * ( switch_in_distances[lod-1] - min_max_render[0] );
		render_arch->set_switch_in_fraction_at_level( lod, lod_fraction );
	}

	// load all detail levels by entering directories named "Level0", "Level1", ...
	// and reading the contents.

	COMPTR<IFileSystem> levelDir;

	COMPONENT_LIST::iterator cmp;
	COMPONENT_LIST::iterator beg = components.begin();
	COMPONENT_LIST::iterator end = components.end();

	char LevelStr[8] = { 'L', 'e', 'v', 'e', 'l', 0, 0, 0 };
	char *num_pt = LevelStr + strlen( LevelStr );
	bool succeeded;

	for( int lod_level = 0; lod_level < render_arch->get_num_lod_levels(); lod_level++ ) {
		
		_itoa( lod_level, num_pt, 10 );

		DAFILEDESC arch_dir( LevelStr );

		succeeded = false;
		
		if( SUCCEEDED( lodDir->CreateInstance( &arch_dir, levelDir ) ) ) {
			for( cmp = beg; cmp != end; cmp++ ) {
				if( (*cmp)->create_archetype( next_render_arch_index, levelDir ) ) {
					render_arch->set_render_archetype_at_level( 0, next_render_arch_index );
					render_arch->set_render_component_at_level( 0, (*cmp) );
					next_render_arch_index++;	
					succeeded = true;
				}
			}
		}

		if( !succeeded ) {
			// destroy all lods
			//
			lod_level--;
			while( lod_level >= 0 ) {
				const RenderArchetypeLod *render_lod = render_arch->get_lod_at_level( lod_level );
				render_lod->render_component->destroy_archetype( render_lod->render_arch_index );
				lod_level--;
			}

			render_arch->set_num_lod_levels( 0 );
			
			return GR_GENERIC;
		}
	}

	return GR_OK;
}

//

GENRESULT RenderMgr::create_continuous_lod_archetype( ARCHETYPE_INDEX arch_index, IFileSystem *filesys )
{
	#define CONTINUOUS_LOD_DIR "openFLAME 3D N-mesh\\Lod library"

	COMPTR<IFileSystem> lodDir;
	DAFILEDESC cont_lod( CONTINUOUS_LOD_DIR );
	APPR_ARCH_MAP::iterator ai;
	RenderArchetype *render_arch;
	
	if( (ai = archetypes.find( arch_index )) == archetypes.end() ) {
		return GR_GENERIC;
	}
	
	render_arch = const_cast<RenderArchetype*>( &((*ai).second) );

	// these could be read in if they were exported
	SWITCH_DISTANCE_TYPE min_render = 0.0f;
	SWITCH_DISTANCE_TYPE max_render = std::numeric_limits<SWITCH_DISTANCE_TYPE>::max();

	SINGLE closest_d;
	SINGLE furthest_d;

	if( SUCCEEDED( filesys->CreateInstance( &cont_lod, lodDir ) ) ) {
		if( FAILED( read_type( lodDir, "Closest distance", &closest_d ) ) ) {
			GENERAL_TRACE_1( "RenderMgr: create_continuous_lod_archetype: 'Closest distance' is missing, defaulting to minimum distance possible\n" );
			closest_d = 0.0f;
		}

		if( FAILED( read_type( lodDir, "Furthest distance", &furthest_d ) ) ) {
			GENERAL_TRACE_1( "RenderMgr: create_continuous_lod_archetype: 'Furthest distance' is missing, defaulting to maximum distance possible.\n" );
			furthest_d = std::numeric_limits<SWITCH_DISTANCE_TYPE>::max();
		}
	}
	else {
		return GR_GENERIC;
	}

	COMPONENT_LIST::iterator cmp;
	COMPONENT_LIST::iterator beg = components.begin();
	COMPONENT_LIST::iterator end = components.end();

	for( cmp = beg; cmp != end; cmp++ ) {
		if( (*cmp)->create_archetype( next_render_arch_index, filesys ) ) {
		
			// Note that we delay setting this stuff until here in an 
			// attempt to not have to undo the state of a partially created
			// renderarchetype in the event that no render component
			// creates this archetype
			//
			render_arch->set_minimum_render_distance( min_render );
			render_arch->set_maximum_render_distance( max_render );
			render_arch->lod_start_dist = closest_d;
			render_arch->lod_stop_dist = furthest_d;

			render_arch->set_num_lod_levels( 1 );
			render_arch->set_switch_in_fraction_at_level( 0, 0.0f ) ;//-std::numeric_limits<SWITCH_DISTANCE_TYPE>::max() );	// HMMM???
			render_arch->set_render_archetype_at_level( 0, next_render_arch_index );
			render_arch->set_render_component_at_level( 0, (*cmp) );

			next_render_arch_index++;	

			return GR_OK;
		}
	}

	return GR_GENERIC;
}

//

GENRESULT RenderMgr::create_simple_archetype( ARCHETYPE_INDEX arch_index, IFileSystem *filesys )
{
	APPR_ARCH_MAP::iterator ai;
	RenderArchetype *render_arch;
	
	if( (ai = archetypes.find( arch_index )) == archetypes.end() ) {
		return GR_GENERIC;
	}
	
	render_arch = const_cast<RenderArchetype*>( &((*ai).second) );

	COMPONENT_LIST::iterator cmp;
	COMPONENT_LIST::iterator beg = components.begin();
	COMPONENT_LIST::iterator end = components.end();

	for( cmp = beg; cmp != end; cmp++ ) {
		if( (*cmp)->create_archetype( next_render_arch_index, filesys ) ) {
		
			// Note that we delay setting this stuff until here in an 
			// attempt to not have to undo the state of a partially created
			// renderarchetype in the event that no render component
			// creates this archetype
			//
			render_arch->set_minimum_render_distance( 0.0f );
			render_arch->set_maximum_render_distance( std::numeric_limits<SWITCH_DISTANCE_TYPE>::max() );
			render_arch->lod_start_dist = 0.0f;
			render_arch->lod_stop_dist = std::numeric_limits<SWITCH_DISTANCE_TYPE>::max();

			render_arch->set_num_lod_levels( 1 );
			render_arch->set_switch_in_fraction_at_level( 0, 0.0f ) ;
			render_arch->set_render_archetype_at_level( 0, next_render_arch_index );
			render_arch->set_render_component_at_level( 0, (*cmp) );

			next_render_arch_index++;	

			return GR_OK;
		}
	}

	return GR_GENERIC;
}

//

const RenderArchetypeLod *RenderMgr::compute_lod_level( INSTANCE_INDEX inst_index, RenderArchetype *render_arch, ICamera *camera, float distance_scale, float *out_abs_lod_fraction )
{
	INST_MAP::iterator inst;
	APPR_ARCH_MAP::iterator arch;

	float abs_lod_fraction = compute_lod_fraction( inst_index, camera, distance_scale );

	if( out_abs_lod_fraction ) {
		*out_abs_lod_fraction = abs_lod_fraction;
	}

	if( abs_lod_fraction < 0.0 ) {
		return NULL;
	}

	if( render_arch == NULL ) {
		if( (inst = instances.find( inst_index )) == instances.end() ) {
			return NULL;
		}
		if( (arch = archetypes.find( inst->second )) == archetypes.end() ) {
			return NULL;
		}

		render_arch = &(arch->second);
	}

	return render_arch->get_lod_at_fraction( abs_lod_fraction );
}

// ......................................................................
//
// DllMain
//
//
BOOL COMAPI DllMain( HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved )
{
	IComponentFactory *server;

	if( DLL_PROCESS_ATTACH == fdwReason ) {

		DA_HEAP_ACQUIRE_HEAP(HEAP);
		DA_HEAP_DEFINE_HEAP_MESSAGE(hinstDLL);
		
		if( (server = new DAComponentFactory2<DAComponentAggregate<RenderMgr>, SYSCONSUMERDESC>( "IRenderer" )) != NULL ) {
			DACOM_Acquire()->RegisterComponent( server, "IRenderer", DACOM_NORMAL_PRIORITY );
			server->Release();
		}
	}

	return TRUE;
}

// EOF














